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

DLT_DECLARE_CONTEXT(dlt_ctxt_audio);

snd_pcm_t *playback_handle, *capture_handle;

uint8_t buf[BUFFER_SZ_BYTES];
void *ch_bufs[CHANNELS] = {0};

static unsigned int rate = RATE;
static unsigned int format = SND_PCM_FORMAT_S32_LE;

static unsigned long int buffer_sz_frames = BUFFER_SZ_FRAMES;

static int open_stream(snd_pcm_t **handle, const char *name, int dir, int mode)
{
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	const char *dirname = (dir == SND_PCM_STREAM_PLAYBACK) ? "PLAYBACK" : "CAPTURE";
	int ret;
	int dummy;

	if ((ret = snd_pcm_open(handle, name, dir, mode)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot open audio device "));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot allocate hardware parameter structure"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_any(*handle, hw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot initialize hardware parameter structure"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_access(*handle, hw_params, SAMPLE_ACCESS)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set access type"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_format(*handle, hw_params, format)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set sample format"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_rate_near(*handle, hw_params, &rate, NULL)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set sample rate"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_channels(*handle, hw_params, CHANNELS)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set channel count"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params_set_buffer_size_near(*handle, hw_params, &buffer_sz_frames)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("set_buffer_time"));
		return ret;
	}

	if ((ret = snd_pcm_hw_params(*handle, hw_params)) < 0)
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
	if ((ret = snd_pcm_sw_params_current(*handle, sw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot initialize software parameters structure"));
		return ret;
	}
	if ((ret = snd_pcm_sw_params_set_avail_min(*handle, sw_params, PERIOD_SZ_FRAMES)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set minimum available count"));
		return ret;
	}

	if ((ret = snd_pcm_sw_params_set_start_threshold(*handle, sw_params, 0U)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set start mode"));
		return ret;
	}
	if ((ret = snd_pcm_sw_params(*handle, sw_params)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot set software parameters"));
		return ret;
	}

	return 0;
}



static void *audio_runner(void * p_data)
{
	int ret = EXIT_SUCCESS;
	uint32_t nb_loops = (uint32_t)p_data;

	int verbosity = (nb_loops < DLT_VERBOSITY_LOOP_THRESHOLD) ? DLT_LOG_VERBOSE : DLT_LOG_INFO ;

	DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("START"));

	while (0 < nb_loops--)
	{
		int avail;
		snd_pcm_sframes_t r = 0;

		DLT_LOG(dlt_ctxt_audio, verbosity, DLT_STRING("audio_runner"), DLT_UINT32(nb_loops));

		if ((ret = snd_pcm_wait(playback_handle, 1000)) < 0)
		{
			DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("poll failed"));
			break;
		}

		avail = snd_pcm_avail_update(capture_handle);
		if (avail > 0)
		{
			if (avail > BUFFER_SZ_BYTES)
				avail = BUFFER_SZ_BYTES;

			if (SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED)
			{
				r = snd_pcm_readn(capture_handle, ch_bufs, avail);
			}
			else
			{
				r = snd_pcm_readi(capture_handle, buf, avail);
			}

			if (0 == (nb_loops & 0xF))
			{
				unsigned long *pbuf = (unsigned long*)(buf);

				DLT_LOG(dlt_ctxt_audio, verbosity,
					DLT_STRING("snd_pcm_read"),
					DLT_UINT32(r),
					DLT_HEX32(pbuf[0]),
					DLT_HEX32(pbuf[1]),
					DLT_HEX32(pbuf[2]),
					DLT_HEX32(pbuf[3])
					);
			}
		}
		else
		{
			DLT_LOG(dlt_ctxt_audio, verbosity, DLT_STRING("avail READ =< 0"));
		}

		avail = snd_pcm_avail_update(playback_handle);
		if (avail > 0)
		{
			if (avail > BUFFER_SZ_BYTES)
				avail = BUFFER_SZ_BYTES;

			if (SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED)
			{
				r = snd_pcm_writen(playback_handle, ch_bufs, avail);
			}
			else
			{
				r = snd_pcm_writei(playback_handle, buf, avail);
			}
			DLT_LOG(dlt_ctxt_audio, verbosity, DLT_STRING("snd_pcm_write"), DLT_INT32(r));
		}
		else
		{
			DLT_LOG(dlt_ctxt_audio, verbosity, DLT_STRING("avail WRITE =< 0"));
		}
	}

	snd_pcm_close(playback_handle);
	snd_pcm_close(capture_handle);

	DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("EXIT"));

	return (void *)ret;
}


int audio_init(pthread_t *runner, uint32_t nb_loops)
{
	int ret = EXIT_SUCCESS;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_audio,"AUDI","ESG BSP Audio Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);
	
	DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("audio_init: Using "), DLT_STRING((SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED) ? "non-interleaved" : "interleaved"));

	if ((ret = open_stream(&playback_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
		return ret;

	if ((ret = open_stream(&capture_handle, ALSA_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0)
		return ret;

	if ((ret = snd_pcm_prepare(playback_handle)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot prepare audio interface for use"));
		return ret;
	}

	if ((ret = snd_pcm_prepare(capture_handle)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot prepare audio interface for use"));
		return ret;
	}

	if ((ret = snd_pcm_start(capture_handle)) < 0)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("cannot start audio interface for use"));
		return ret;
	}

	/* actual sample buffer */
	memset(buf, 0, sizeof(buf));

	/* non-interleaved channel buffer offets */
	for (int c = 0; c < CHANNELS; c++)
	{
		ch_bufs[c] = (void *)((unsigned char *)buf + (c * SAMPLE_SZ_BYTES * PERIOD_SZ_FRAMES));
	}

	ret = pthread_create(runner, NULL, audio_runner, nb_loops);

	if (EXIT_SUCCESS != ret)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("audio_init: failed to creating running"));
	}

   return ret;
}
