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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <alsa/asoundlib.h>

#define RATE 48000U
#define PERIODS 2
#define PERIOD_TIME_US 20000U
#define SAMPLE_SZ_BYTES 4U
#define CHANNELS 4U

#define FRAME_SZ_BYTES (CHANNELS * SAMPLE_SZ_BYTES)
#define BUFFER_TIME_US (PERIODS * PERIOD_TIME_US)
#define PERIOD_SZ_FRAMES (RATE * PERIOD_TIME_US / 1000000)
#define BUFFER_SZ_FRAMES (RATE * BUFFER_TIME_US / 1000000)
#define BUFFER_SZ_BYTES (BUFFER_SZ_FRAMES * FRAME_SZ_BYTES)

#if 0
#define ALSA_DEVICE "hw:0,0"
#define SAMPLE_ACCESS SND_PCM_ACCESS_RW_INTERLEAVED
#else
#define ALSA_DEVICE "sysdefault:CARD=axcavb"
#define SAMPLE_ACCESS SND_PCM_ACCESS_RW_NONINTERLEAVED
#endif

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
	int err;
	int dummy;

	if ((err = snd_pcm_open(handle, name, dir, mode)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot open audio device (%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot allocate hardware parameter structure(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_any(*handle, hw_params)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot initialize hardware parameter structure(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_access(*handle, hw_params, SAMPLE_ACCESS)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot set access type(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_format(*handle, hw_params, format)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot set sample format(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_rate_near(*handle, hw_params, &rate, NULL)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot set sample rate(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_channels(*handle, hw_params, CHANNELS)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot set channel count(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_buffer_size_near(*handle, hw_params, &buffer_sz_frames)) < 0)
	{
		fprintf(stderr, "set_buffer_time: %s\n", snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params(*handle, hw_params)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot set parameters(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot allocate software parameters structure(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}
	if ((err = snd_pcm_sw_params_current(*handle, sw_params)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot initialize software parameters structure(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}
	if ((err = snd_pcm_sw_params_set_avail_min(*handle, sw_params, PERIOD_SZ_FRAMES)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot set minimum available count(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_sw_params_set_start_threshold(*handle, sw_params, 0U)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot set start mode(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}
	if ((err = snd_pcm_sw_params(*handle, sw_params)) < 0)
	{
		fprintf(stderr, "%s (%s): cannot set software parameters(%s)\n",
				name, dirname, snd_strerror(err));
		return err;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int err;
	unsigned int period_cnt = 0;

	printf("using " ALSA_DEVICE "\n");
	printf("Using %s access\n", (SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED) ? "non-interleaved" : "interleaved");

	if ((err = open_stream(&playback_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
		return err;

	if ((err = open_stream(&capture_handle, ALSA_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0)
		return err;

	if ((err = snd_pcm_prepare(playback_handle)) < 0)
	{
		fprintf(stderr, "cannot prepare audio interface for use(%s)\n",
				snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_prepare(capture_handle)) < 0)
	{
		fprintf(stderr, "cannot prepare audio interface for use(%s)\n",
				snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_start(capture_handle)) < 0)
	{
		fprintf(stderr, "cannot start audio interface for use(%s)\n",
				snd_strerror(err));
		return err;
	}

	/* actual sample buffer */
	memset(buf, 0, sizeof(buf));

	/* non-interleaved channel buffer offets */
	for (int c = 0; c < CHANNELS; c++)
	{
		ch_bufs[c] = (void *)((unsigned char *)buf + (c * SAMPLE_SZ_BYTES * PERIOD_SZ_FRAMES));
	}

	while (1)
	{
		int avail;
		snd_pcm_sframes_t r = 0;

		period_cnt++;

		if ((err = snd_pcm_wait(playback_handle, 1000)) < 0)
		{
			fprintf(stderr, "poll failed(%s)\n", strerror(errno));
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

			if (0 == (period_cnt & 0xF))
			{
				unsigned long *pbuf = (unsigned long*)(buf);
				printf("r = %04ld:%08lx-%08lx-%08lx-%08lx\n", r, pbuf[0], pbuf[1], pbuf[2], pbuf[3]);
			}
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
		}
	}

	snd_pcm_close(playback_handle);
	snd_pcm_close(capture_handle);
	return 0;
}
