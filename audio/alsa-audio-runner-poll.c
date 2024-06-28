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
static AlsaDevice_t *audio_dev = NULL;

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
#if defined(AUDIO_TEST_SAMPLE_ACCESS) && AUDIO_TEST_SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED
		alsa_device_startn(audio_dev, ch_bufs);
#else
		alsa_device_starti(audio_dev, buf);
#endif

#define SELECT_nPOLL
#ifdef SELECT_nPOLL
		fd_set read_fds, write_fds;

		DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("START"), DLT_UINT32(nb_loops), DLT_STRING("(select)"));
#else
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("START"), DLT_UINT32(nb_loops), DLT_STRING("(poll)"));
#endif

		while ((0 < nb_loops--) && (0 <= ret))
		{
			int avail;

#ifdef SELECT_nPOLL

			/* RTFM : masks are modified in place by (shitty) select, hence this must reinit for each loop */
			FD_ZERO(&read_fds);
			FD_ZERO(&write_fds);
			FD_SET(pfds[CAPTURE_FD_INDEX].fd, &read_fds);
			FD_SET(pfds[PLAYBACK_FD_INDEX].fd, &write_fds);

			ret = select(pfds[PLAYBACK_FD_INDEX].fd + 1 /*highest fd, plus one because 'select' is P.O.S*/,
						 &read_fds,
						 &write_fds,
						 NULL,
						 NULL);

			/*	DLT_HEX32(read_fds.__fds_bits[0]),
				DLT_HEX32(write_fds.__fds_bits[0]),*/

			DLT_LOG(dlt_ctxt_audio, DLT_LOG_VERBOSE, DLT_STRING("select (#fds/play-is-set/capture-is-set)"),
					DLT_UINT32(ret),
					DLT_UINT32(FD_ISSET(pfds[PLAYBACK_FD_INDEX].fd, &write_fds)),
					DLT_UINT32(FD_ISSET(pfds[CAPTURE_FD_INDEX].fd, &read_fds)));

			if (0 > ret)
			{
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("select failed with"), DLT_UINT32(errno));
			}
			else
			{
				/* Audio available from the soundcard (capture) */
				if (FD_ISSET(pfds[CAPTURE_FD_INDEX].fd, &read_fds))
				{
#if defined(AUDIO_TEST_SAMPLE_ACCESS) && AUDIO_TEST_SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED
					/* Get audio from the soundcard */
					ret = alsa_device_readn(audio_dev, ch_bufs, AUDIO_TEST_PERIOD_SZ_FRAMES);
#else
					/* Get audio from the soundcard */
					ret = alsa_device_readi(audio_dev, buf, AUDIO_TEST_PERIOD_SZ_FRAMES);
#endif
				}

				/* Ready to play a frame (playback) */
				if (FD_ISSET(pfds[PLAYBACK_FD_INDEX].fd, &write_fds))
				{
#if defined(AUDIO_TEST_SAMPLE_ACCESS) && AUDIO_TEST_SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED
					/* Playback the audio and reset the echo canceller if we got an underrun */
					ret = alsa_device_writen(audio_dev, ch_bufs, AUDIO_TEST_PERIOD_SZ_FRAMES);
#else
					/* Playback the audio and reset the echo canceller if we got an underrun */
					ret = alsa_device_writei(audio_dev, buf, AUDIO_TEST_PERIOD_SZ_FRAMES);
#endif
				}
			}
#else
			ret = poll(pfds, nfds, -1);

			if (0 > ret)
			{
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("poll failed with"), DLT_UINT32(errno));
			}
			else
			{
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("poll (err/ret)"), DLT_UINT32(errno), DLT_UINT32(ret));

				/* Audio available from the soundcard (capture) */
				ret = alsa_device_capture_ready(audio_dev, pfds, nfds);
				if (0 < ret)
				{
#if defined(AUDIO_TEST_SAMPLE_ACCESS) && AUDIO_TEST_SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED
					/* Get audio from the soundcard */
					ret = alsa_device_readn(audio_dev, ch_bufs, AUDIO_TEST_PERIOD_SZ_FRAMES);
#else
					/* Get audio from the soundcard */
					ret = alsa_device_readi(audio_dev, buf, AUDIO_TEST_PERIOD_SZ_FRAMES);
#endif
				}

				/* Ready to play a frame (playback) */
				ret = alsa_device_playback_ready(audio_dev, pfds, nfds);
				if (0 < ret)
				{
#if defined(AUDIO_TEST_SAMPLE_ACCESS) && AUDIO_TEST_SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED
					/* Playback the audio and reset the echo canceller if we got an underrun */
					ret = alsa_device_writen(audio_dev, ch_bufs, AUDIO_TEST_PERIOD_SZ_FRAMES);
#else
					ret = alsa_device_writei(audio_dev, buf, AUDIO_TEST_PERIOD_SZ_FRAMES);
#endif
				}
			}
#endif

			/* Stress (full) pause/resume cycle */
			if ((0U < settings->pauses) && (4U == (nb_loops & 0xFF)))
			{
#if defined(AUDIO_TEST_SAMPLE_ACCESS) && AUDIO_TEST_SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED
				 int paused = alsa_device_pausen(audio_dev, 1 /*pause*/, ch_bufs);
#else
				int paused = alsa_device_pausei(audio_dev, 1 /*pause*/, buf);
#endif
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("pausing"));

				(void)alsa_device_state(audio_dev, 0 /*play*/ );
				(void)alsa_device_state(audio_dev, 1 /*rec*/ );
#if defined(AUDIO_TEST_SAMPLE_ACCESS) && AUDIO_TEST_SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED
				alsa_device_pausen(audio_dev, 0, ch_bufs);
#else
				alsa_device_pausei(audio_dev, 0, buf);
#endif
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("resuming"));

				(void)alsa_device_state(audio_dev, 0);
				(void)alsa_device_state(audio_dev, 1 /*rec*/ );

				settings->pauses--;
			}
		}

		// alsa_device_close(audio_dev);
	}

	DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("EXIT"), DLT_UINT32(ret));

	return (void *)ret;
}

int audio_runner_init_poll(pthread_t *runner, ebt_settings_t *settings)
{
	int ret = (NULL != settings) ? EXIT_SUCCESS : -EINVAL;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_audio, "AUDI", "ESG BSP Audio Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	if (EXIT_SUCCESS == ret)
	{
		audio_dev = alsa_device_open(settings);
		if (NULL == audio_dev)
		{
			ret = -EINVAL;
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		/* Setup all file descriptors for poll()ing */
		nfds = alsa_device_nfds(audio_dev);
		pfds = malloc(sizeof(*pfds) * (nfds));

		alsa_device_getfds(audio_dev, pfds, nfds);
	}

	if (EXIT_SUCCESS == ret)
	{
		/* actual sample buffer */
		memset(buf, 0, sizeof(buf));

		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("audio_runner_init_poll: creating runner"));

		/* non-interleaved channel buffer offets */
		for (int c = 0; c < AUDIO_TEST_CHANNELS; c++)
		{
			ch_bufs[c] = (void *)((unsigned char *)buf + (c * AUDIO_TEST_SAMPLE_SZ_BYTES * AUDIO_TEST_PERIOD_SZ_FRAMES));
		}

		ret = pthread_create(runner, NULL, audio_runner, (void *)settings);
	}

	if (0 > ret)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("audio_runner_init_poll: failed to creating runner"), DLT_INT32(ret));
	}

	return ret;
}
