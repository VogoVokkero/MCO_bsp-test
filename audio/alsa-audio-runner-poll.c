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

DLT_DECLARE_CONTEXT(dlt_ctxt_audio);

uint8_t buf[AUDIO_TEST_BUFFER_SZ_BYTES];
void *ch_bufs[AUDIO_TEST_CHANNELS] = {0};

static unsigned int rate = AUDIO_TEST_RATE;
static unsigned int format = SND_PCM_FORMAT_S32_LE;

static unsigned long int buffer_sz_frames = AUDIO_TEST_BUFFER_SZ_FRAMES;

pcmAlsa_device_t captureDevice = {0};
pcmAlsa_device_t playbackDevice = {0};

static int open_stream(pcmAlsa_device_t *device, int mode)
{
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	int ret;

	if ((ret = snd_pcm_open(&(device->handle), device->name, device->stream_direction, mode)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot open audio device "));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot allocate hardware parameter structure"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_any(device->handle, hw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot initialize hardware parameter structure"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_access(device->handle, hw_params, AUDIO_TEST_SAMPLE_ACCESS)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set access type"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_format(device->handle, hw_params, format)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set sample format"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_rate_near(device->handle, hw_params, &rate, NULL)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set sample rate"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_channels(device->handle, hw_params, AUDIO_TEST_CHANNELS)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set channel count"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_buffer_size_near(device->handle, hw_params, &buffer_sz_frames)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("set_buffer_time"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params(device->handle, hw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set parameters"));
		return ret;
	}

	snd_pcm_hw_params_free(hw_params);

	if ((ret = snd_pcm_sw_params_malloc(&sw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot allocate software parameters structure"));
		return ret;
	}
	if ((ret = snd_pcm_sw_params_current(device->handle, sw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot initialize software parameters structure"));
		return ret;
	}
	if ((ret = snd_pcm_sw_params_set_avail_min(device->handle, sw_params, AUDIO_TEST_PERIOD_SZ_FRAMES)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set minimum available count"));
		return ret;
	}

	if ((ret = snd_pcm_sw_params_set_start_threshold(device->handle, sw_params, 0U)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set start mode"));
		return ret;
	}
	if ((ret = snd_pcm_sw_params(device->handle, sw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set software parameters"));
		return ret;
	}

	if ((0 == ret) && (NULL != device) && (NULL != device->handle))
	{
		device->fds.count = snd_pcm_poll_descriptors_count(device->handle);
		device->fds.ufds = malloc(sizeof(struct pollfd) * device->fds.count);

		ret = snd_pcm_poll_descriptors(device->handle, device->fds.ufds, device->fds.count);

		if (0 > ret)
		{
			DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_poll_descriptors failed"));
		}
	}

	return 0;
}

static void *audio_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;
	ebt_settings_t *settings = (ebt_settings_t *)p_data;
	fd_set read_fds, write_fds, except_fds;

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("START failed, ebt_settings_t null"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		uint32_t nb_loops = settings->nb_loops;

		DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("START"), DLT_UINT32(nb_loops));

		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		FD_ZERO(&except_fds);

		FD_SET(captureDevice.fds.ufds->fd, &read_fds);
		FD_SET(playbackDevice.fds.ufds->fd, &write_fds);
		//  FD_SET(TSK_Info[taskId].fd_watch[i].id, &except_fds);

		while ((0 < nb_loops--) && (EXIT_SUCCESS == ret))
		{
			int avail;
			snd_pcm_sframes_t r = 0;

			// Attente passive
			ret = select(1, &read_fds, &write_fds, &except_fds, NULL);
			if (0 > ret)
			{
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("select failed"));
			}
			else
			{
				DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("select ready decriptors"), DLT_INT32(ret));

				if (FD_ISSET(captureDevice.fds.ufds->fd, &read_fds))
				{
						r = snd_pcm_readn(captureDevice.handle, ch_bufs, avail);

						if (0 == (nb_loops & 0x1F))
						{
							unsigned long *pbuf = (unsigned long *)(buf);

							DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO,
									DLT_STRING("snd_pcm_readn"),
									DLT_UINT32(r),
									DLT_HEX32(pbuf[0]),
									DLT_HEX32(pbuf[1*AUDIO_TEST_PERIOD_SZ_FRAMES]),
									DLT_HEX32(pbuf[2*AUDIO_TEST_PERIOD_SZ_FRAMES]),
									DLT_HEX32(pbuf[3*AUDIO_TEST_PERIOD_SZ_FRAMES]),
									DLT_UINT32(nb_loops));
						}

					DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("snd_pcm_read"), DLT_INT32(r));
				}

				if (FD_ISSET(playbackDevice.fds.ufds->fd, &read_fds))
				{
						unsigned long *pbuf = (unsigned long *)(buf);

						/* constant value, to quickly visualize output channel swap */
						memset(&(pbuf[2 * AUDIO_TEST_PERIOD_SZ_FRAMES]), 0xAA, sizeof(uint32_t) * AUDIO_TEST_PERIOD_SZ_FRAMES);

						if (0 == (nb_loops & 0x1F))
						{
							DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO,
									DLT_STRING("snd_pcm_writen"),
									DLT_UINT32(r),
									DLT_HEX32(pbuf[0]),
									DLT_HEX32(pbuf[1 * AUDIO_TEST_PERIOD_SZ_FRAMES]),
									DLT_HEX32(pbuf[2 * AUDIO_TEST_PERIOD_SZ_FRAMES]),
									DLT_HEX32(pbuf[3 * AUDIO_TEST_PERIOD_SZ_FRAMES]),
									DLT_UINT32(nb_loops));
						}
						r = snd_pcm_writen(playbackDevice.handle, ch_bufs, avail);

					DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("snd_pcm_write"), DLT_INT32(r));
				}
			}
		}

		snd_pcm_close(playbackDevice.handle);
		snd_pcm_close(captureDevice.handle);

		free(captureDevice.fds.ufds);
		free(playbackDevice.fds.ufds);
	}

	DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("EXIT"), DLT_UINT32(ret));

	return (void *)ret;
}

int audio_init_poll(pthread_t *runner, ebt_settings_t *settings)
{
	int ret = EXIT_SUCCESS;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_audio, "AUDI", "ESG BSP Audio Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	playbackDevice.name = ALSA_DEVICE;
	playbackDevice.stream_direction = SND_PCM_STREAM_PLAYBACK;

	captureDevice.name = ALSA_DEVICE;
	captureDevice.stream_direction = SND_PCM_STREAM_CAPTURE;

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("audio_init_poll: invalid settings"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("audio_init_poll: Using "), DLT_STRING((AUDIO_TEST_SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED) ? "non-interleaved" : "interleaved"));

		ret = open_stream(&playbackDevice, SND_PCM_NONBLOCK);
		if (0 > ret)
		{
			DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot open_stream SND_PCM_STREAM_PLAYBACK"));
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		if ((ret = open_stream(&captureDevice, 0)) < 0)
		{
			DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot open_stream SND_PCM_STREAM_CAPTURE"));
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		if ((ret = snd_pcm_prepare(playbackDevice.handle)) < 0)
		{
			DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot prepare audio interface for use"));
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		if ((ret = snd_pcm_prepare(captureDevice.handle)) < 0)
		{
			DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot prepare audio interface for use"));
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		if ((ret = snd_pcm_start(captureDevice.handle)) < 0)
		{
			DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot start audio interface for use"));
		}
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
