/*
 * Copyright (c) 2012 Daniel Mack
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

/*
 * See README
 */
#include "esg-bsp-test.h"
#include "alsa-audio-runner.h"
#include "alsa-device.h"

DLT_IMPORT_CONTEXT(dlt_ctxt_audio);

static uint8_t buf[AUDIO_TEST_BUFFER_SZ_BYTES];
static void *ch_bufs[AUDIO_TEST_CHANNELS] = {0};

static unsigned int nfds = 0;
static struct pollfd *pfds = NULL;

static AlsaDevice_t *audio_out = NULL;
static AlsaDevice_t *audio_in = NULL;

static void *audio_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;

	ebt_settings_t *settings = (ebt_settings_t *)p_data;

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("START failed, ebt_settings_t null"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		uint32_t nb_loops = settings->nb_loops;

		/* linked start, fetch buffers to output first */
		alsa_device_startn(audio_out, ch_bufs);

#define SELECT_nPOLL
#ifdef SELECT_nPOLL
		fd_set read_fds;

		DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("START"), DLT_UINT32(nb_loops), DLT_STRING("(select)"));
#else
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("START"), DLT_UINT32(nb_loops), DLT_STRING("(poll)"));
#endif

		while ((0 < nb_loops--) && (0 <= ret))
		{
#ifdef SELECT_nPOLL

			/* RTFM : masks are modified in place by (shitty) select, hence this must reinit for each loop */
			FD_ZERO(&read_fds);
			//	FD_ZERO(&write_fds);
			FD_SET(audio_in->nb_poll_fd, &read_fds);
			//	FD_SET(audio_out->nb_poll_fd, &write_fds);

			ret = select(audio_in->nb_poll_fd + 1 /*highest fd, plus one because 'select' is P.O.S*/,
						 &read_fds,
						 NULL,
						 NULL,
						 NULL);

			/*	DLT_HEX32(read_fds.__fds_bits[0]),
				DLT_HEX32(write_fds.__fds_bits[0]),*/
			//		DLT_UINT32(FD_ISSET(audio_out->nb_poll_fd, &write_fds)),

			DLT_LOG(dlt_ctxt_audio, DLT_LOG_VERBOSE, DLT_STRING("select (#fds/capture-is-set)"),
					DLT_UINT32(ret),
					DLT_UINT32(FD_ISSET(audio_in->nb_poll_fd, &read_fds)));

			if (0 > ret)
			{
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("select failed with"), DLT_UINT32(errno));
			}
			else
			{
				/* check device state */
				snd_pcm_state_t state = alsa_device_state(audio_in);

				if (SND_PCM_STATE_SETUP == state)
				{
					/* resume */
					//	int paused = alsa_device_pause(audio_dev, 0 /*resume*/, ch_bufs);
					DLT_LOG(dlt_ctxt_audio, DLT_LOG_WARN, DLT_STRING("|>"));
				}

				/* Audio available from the soundcard (capture) */
				if (FD_ISSET(audio_in->nb_poll_fd, &read_fds))
				{
					/* Get audio from the soundcard */
					ret = alsa_device_readn(audio_in, ch_bufs, AUDIO_TEST_PERIOD_SZ_FRAMES);
				}

				/* Ready to play a frame (playback) */
				//	if (FD_ISSET(audio_out->nb_poll_fd, &write_fds))
				//	{
				/* Playback the audio and reset the echo canceller if we got an underrun */
				ret = alsa_device_writen(audio_out, ch_bufs, AUDIO_TEST_PERIOD_SZ_FRAMES);
				//	}
			}
#else
			ret = poll(audio_in->poll_fd, audio_in->nb_poll_fd, -1);

			if (0 > ret)
			{
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("poll failed with"), DLT_UINT32(errno));
			}
			else
			{
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("poll (err/ret)"), DLT_UINT32(errno), DLT_UINT32(ret));

				/* Audio available from the soundcard (capture) */
				ret = alsa_device_ready(audio_in, pfds, nfds);
				if (0 < ret)
				{
					/* Get audio from the soundcard */
					ret = alsa_device_readn(audio_in, ch_bufs, AUDIO_TEST_PERIOD_SZ_FRAMES);
				}

				/* Ready to play a frame (playback) */
				ret = alsa_device_ready(audio_out, pfds, nfds);
				if (0 < ret)
				{
					/* Playback the audio and reset the echo canceller if we got an underrun */
					ret = alsa_device_writen(audio_out, ch_bufs, AUDIO_TEST_PERIOD_SZ_FRAMES);
				}
			}
#endif

			if ((0U < settings->pauses) && (4U == (nb_loops & 0x7F)))
			{
				/* assuming linked devices */
				int paused = alsa_device_pause(audio_out, 1 /*pause*/, ch_bufs);
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_VERBOSE, DLT_STRING("||"), DLT_UINT32(paused));

				settings->pauses--;
			}
		}
	}

	DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("EXIT"), DLT_UINT32(ret));

	return (void *)ret;
}

int audio_init_poll(pthread_t *runner, ebt_settings_t *settings)
{
	int ret = (NULL != settings) ? EXIT_SUCCESS : -EINVAL;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_audio, "AUDI", "ESG BSP Audio Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	if (EXIT_SUCCESS == ret)
	{
		audio_in = alsa_device_open_rec(settings);
		if (NULL == audio_in)
		{
			ret = -EINVAL;
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		audio_out = alsa_device_open_play(settings);
		if (NULL == audio_out)
		{
			ret = -EINVAL;
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		ret = alsa_device_link(audio_in, audio_out);
	}

	if (EXIT_SUCCESS == ret)
	{
		/* actual sample buffer */
		memset(buf, 0, sizeof(buf));

		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("audio_init_poll: creating runner"));

		/* non-interleaved channel buffer offets */
		for (int c = 0; c < AUDIO_TEST_CHANNELS; c++)
		{
			ch_bufs[c] = (void *)((unsigned char *)buf + (c * AUDIO_TEST_SAMPLE_SZ_BYTES * AUDIO_TEST_PERIOD_SZ_FRAMES));
		}

		ret = pthread_create(runner, NULL, audio_runner, (void *)settings);
	}

	if (0 > ret)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("audio_init_poll: failed to creating runner"), DLT_INT32(ret));
	}

	return ret;
}
