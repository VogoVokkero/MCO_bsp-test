/*
   Copyright (C) 2004-2006 Jean-Marc Valin
   Copyright (C) 2006 Commonwealth Scientific and Industrial Research
                      Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#include "alsa-device.h"
#include <stdlib.h>

DLT_IMPORT_CONTEXT(dlt_ctxt_audio);

#define USE_SILENCE /* use silence setytings, to handle x-run*/

static int alsa_device_hw_params(snd_pcm_t *pcm_handle, ebt_settings_t *settings)
{
   int err = ((NULL != pcm_handle) && (NULL != settings)) ? 0 : -EINVAL;

   snd_pcm_hw_params_t *hw_params;
   snd_pcm_hw_params_alloca(&hw_params);

   if (0 <= err)
   {
      err = snd_pcm_hw_params_any(pcm_handle, hw_params);
      if (0 > err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_any capture"), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, AUDIO_TEST_SAMPLE_ACCESS);
      if (0 > err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_access capture"), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, AUDIO_TEST_SAMPLE_FORMAT);
      if (0 > err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_format capture"), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      uint32_t rate = AUDIO_TEST_RATE;
      err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0);
      if (0 > err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_rate_near capture"), DLT_UINT32(rate), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, AUDIO_TEST_CHANNELS);
      if (0 > err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_channels capture"), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      err = snd_pcm_hw_params_set_period_size(pcm_handle, hw_params, AUDIO_TEST_PERIOD_SZ_FRAMES, 0);
      if (0 > err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_period_size constraint failed"), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      err = snd_pcm_hw_params_set_periods(pcm_handle, hw_params, AUDIO_TEST_PERIODS, 0);
      if (0 > err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_period_size_near capture"), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      snd_pcm_uframes_t buffer_size = AUDIO_TEST_PERIODS * AUDIO_TEST_PERIOD_SZ_FRAMES;

      err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hw_params, &buffer_size);
      if (0 > err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_buffer_size_near capture"), DLT_UINT32(buffer_size), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      err = snd_pcm_hw_params(pcm_handle, hw_params);
      if (0 > err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params"), DLT_STRING(snd_strerror(err)));
      }
   }

   if ((0U < settings->pauses) && (0 == snd_pcm_hw_params_can_pause(hw_params)))
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_WARN, DLT_STRING("snd_pcm_hw_params_can_pause : hardware does not support pause"), DLT_STRING(snd_strerror(err)));
   }

   return err;
}

static int alsa_device_sw_params(snd_pcm_t *pcm_handle, snd_pcm_uframes_t avail_min, ebt_settings_t *settings)
{
   int err = ((NULL != pcm_handle) && (NULL != settings)) ? 0 : -EINVAL;

   snd_pcm_sw_params_t *sw_params;
   snd_pcm_sw_params_alloca(&sw_params);

   if (0 <= err)
   {
      err = snd_pcm_sw_params_current(pcm_handle, sw_params);
      if (0 < err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_current capture"), DLT_STRING(snd_strerror(err)));
      }
   }

   if ((0 <= err) && (0 != avail_min))
   {
      err = snd_pcm_sw_params_set_avail_min(pcm_handle, sw_params, avail_min);
      if (0 < err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_set_avail_min capture"), DLT_STRING(snd_strerror(err)));
      }
   }

#ifdef USE_SILENCE
   /* Handle X-run with silence */
   if (0 <= err)
   {
      err = snd_pcm_sw_params_set_stop_threshold(pcm_handle, sw_params, INT32_MAX);
      if (0 < err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_set_stop_threshold"), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      err = snd_pcm_sw_params_set_silence_threshold(pcm_handle, sw_params, 0U);
      if (0 < err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_set_silence_threshold"), DLT_STRING(snd_strerror(err)));
      }
   }

   if (0 <= err)
   {
      err = snd_pcm_sw_params_set_silence_size(pcm_handle, sw_params, INT32_MAX);
      if (0 < err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_set_silence_size"), DLT_STRING(snd_strerror(err)));
      }
   }
#endif

   if (0 <= err)
   {
      err = snd_pcm_sw_params(pcm_handle, sw_params);
      if (0 < err)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params capture"), DLT_STRING(snd_strerror(err)));
      }
   }

   return err;
}

//= alsa_device_open(AUDIO_TEST_DEVICE_NAME, AUDIO_TEST_RATE, AUDIO_TEST_CHANNELS, AUDIO_TEST_PERIOD_SZ_FRAMES, settings);

AlsaDevice_t *alsa_device_open(ebt_settings_t *settings)
{
   int err = (NULL != settings) ? EXIT_SUCCESS : -EINVAL;
   static snd_output_t *jcd_out;

   AlsaDevice_t *dev = malloc(sizeof(*dev));
   if (!dev)
      return NULL;

   snd_output_stdio_attach(&jcd_out, stdout, 0);

   if ((err = snd_pcm_open(&dev->capture_handle, AUDIO_TEST_DEVICE_NAME, SND_PCM_STREAM_CAPTURE, 0)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_open capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if (0 <= err)
   {
      err = alsa_device_hw_params(dev->capture_handle, settings);
      if (0 <= err)
      {
         /* got period OK */
         dev->period = AUDIO_TEST_PERIOD_SZ_FRAMES;
      }
   }

   if (0 <= err)
   {
      err = alsa_device_sw_params(dev->capture_handle, 0, settings);
   }

   if ((err = snd_pcm_open(&dev->playback_handle, AUDIO_TEST_DEVICE_NAME, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_open play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if (0 <= err)
   {
      err = alsa_device_hw_params(dev->playback_handle, settings);
   }

   if (0 <= err)
   {
      err = alsa_device_sw_params(dev->playback_handle, /* avail min*/ AUDIO_TEST_PERIOD_SZ_FRAMES, settings);
   }

#define USE_SND_PCM_LINK
#ifdef USE_SND_PCM_LINK
   if ((err = snd_pcm_link(dev->capture_handle, dev->playback_handle)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_link failed"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   else
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_link OK"));
   }
#endif

   dev->readN = snd_pcm_poll_descriptors_count(dev->capture_handle);
   dev->writeN = snd_pcm_poll_descriptors_count(dev->playback_handle);

   dev->read_fd = malloc(dev->readN * sizeof(*dev->read_fd));
   /*printf ("descriptors: %d %d\n", dev->readN, dev->writeN);*/
   if (snd_pcm_poll_descriptors(dev->capture_handle, dev->read_fd, dev->readN) != dev->readN)
   {
      fprintf(stderr, "cannot obtain capture file descriptors (%s)\n",
              snd_strerror(err));
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_poll_descriptors C failed"));
      assert(0);
   }

   dev->write_fd = malloc(dev->writeN * sizeof(*dev->read_fd));
   if (snd_pcm_poll_descriptors(dev->playback_handle, dev->write_fd, dev->writeN) != dev->writeN)
   {
      fprintf(stderr, "cannot obtain playback file descriptors (%s)\n",
              snd_strerror(err));
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_poll_descriptors P failed"));
      assert(0);
   }
   return dev;
}

void alsa_device_close(AlsaDevice_t *dev)
{
   DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("alsa_device_close"));

   snd_pcm_close(dev->capture_handle);
   snd_pcm_close(dev->playback_handle);
   free(dev);
}

snd_pcm_sframes_t alsa_device_readn(AlsaDevice_t *dev, void **ch_buf, int len)
{
   snd_pcm_sframes_t err;

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_VERBOSE, DLT_STRING("alsa_device_readn"));

   if ((err = snd_pcm_readn(dev->capture_handle, ch_buf, len)) != len)
   {
      if (err < 0)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_readn failed"), DLT_STRING(snd_strerror(err)));
#if 0
         if ((err = snd_pcm_prepare(dev->capture_handle)) < 0)
         {
            DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_readn snd_pcm_prepare failed"), DLT_STRING(snd_strerror(err)));
         }
         if ((err = snd_pcm_start(dev->capture_handle)) < 0)
         {
            DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_readn snd_pcm_start (recover) failed"), DLT_STRING(snd_strerror(err)));
         }
#else
         err = snd_pcm_recover(dev->capture_handle, err, 1);
#endif
      }
      else
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_readn wrong len (err != len) "), DLT_UINT32(err), DLT_UINT32(len));
      }
   }
   else
   {
      if (abs(((int32_t *)ch_buf[0])[0]) < 0x00002000U)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO,
                 DLT_STRING("IN1 level too low ? snd_pcm_readn"),
                 DLT_STRING(snd_strerror(err)),
                 DLT_HEX32(((uint32_t *)ch_buf[0])[0]),
                 DLT_HEX32(((uint32_t *)ch_buf[1])[0]),
                 DLT_HEX32(((uint32_t *)ch_buf[2])[0]),
                 DLT_HEX32(((uint32_t *)ch_buf[3])[0]));
      }
   }
   return err;
}

snd_pcm_sframes_t alsa_device_writen(AlsaDevice_t *dev, void **ch_buf, int len)
{
   snd_pcm_sframes_t err;

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_VERBOSE, DLT_STRING("alsa_device_writen"));

   if ((err = snd_pcm_writen(dev->playback_handle, ch_buf, len)) != len)
   {
      if (err < 0)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen failed"), DLT_STRING(snd_strerror(err)));
#if 0
         /* try to recover */
         if ((err = snd_pcm_prepare(dev->playback_handle)) < 0)
         {
            DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen snd_pcm_prepare failed"), DLT_STRING(snd_strerror(err)));
         }

#else
         err = snd_pcm_recover(dev->capture_handle, err, 1);
#endif
      }
      else
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_writen wrong len (err != len) "), DLT_UINT32(err), DLT_UINT32(len));
      }
   }
   return err;
}

int alsa_device_readi(AlsaDevice_t *dev, void *buf, int len)
{
   int err;
   /*fprintf (stderr, "-");*/
   if ((err = snd_pcm_readi(dev->capture_handle, buf, len)) != len)
   {
      if (err < 0)
      {
         if (err == -EPIPE)
         {
            fprintf(stderr, "An overrun has occured, reseting capture\n");
         }
         else
         {
            fprintf(stderr, "read from audio interface failed (%s)\n",
                    snd_strerror(err));
         }
         if ((err = snd_pcm_prepare(dev->capture_handle)) < 0)
         {
            fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                    snd_strerror(err));
         }
         if ((err = snd_pcm_start(dev->capture_handle)) < 0)
         {
            fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                    snd_strerror(err));
         }
      }
      else
      {
         fprintf(stderr, "Couldn't read as many samples as I wanted (%d instead of %d)\n", err, len);
      }
      return 1;
   }
   return 0;
}

int alsa_device_writei(AlsaDevice_t *dev, const void *buf, int len)
{
   int err;

   if ((err = snd_pcm_writei(dev->playback_handle, buf, len)) != len)
   {
      if (err < 0)
      {
         if (err == -EPIPE)
         {
            fprintf(stderr, "An underrun has occured, reseting playback, len=%d\n", len);
         }
         else
         {
            fprintf(stderr, "write to audio interface failed (%s)\n",
                    snd_strerror(err));
         }
         if ((err = snd_pcm_prepare(dev->playback_handle)) < 0)
         {
            fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                    snd_strerror(err));
         }
      }
      else
      {
         fprintf(stderr, "Couldn't write as many samples as I wanted (%d instead of %d)\n", err, len);
      }
      return 1;
   }
   return 0;
}

int alsa_device_capture_ready(AlsaDevice_t *dev, struct pollfd *pfds, unsigned int nfds)
{
   unsigned short revents = 0;
   int ret = snd_pcm_poll_descriptors_revents(dev->capture_handle, pfds, dev->readN, &revents);

   if (0 > ret)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_poll_descriptors_revents C failed"), DLT_STRING(snd_strerror(ret)));

      return pfds[CAPTURE_FD_INDEX].revents & POLLIN;
   }
   else
   {
      ret = !!(revents & POLLIN);
   }

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("alsa_device_capture_ready"), DLT_UINT32(ret));

   return ret;
}

int alsa_device_playback_ready(AlsaDevice_t *dev, struct pollfd *pfds, unsigned int nfds)
{
   unsigned short revents = 0;
   int ret = snd_pcm_poll_descriptors_revents(dev->playback_handle, pfds + dev->readN, dev->writeN, &revents);

   if (0 > ret)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_poll_descriptors_revents P failed"), DLT_STRING(snd_strerror(ret)));
      return pfds[PLAYBACK_FD_INDEX].revents & POLLOUT;
   }
   else
   {
      ret = !!(revents & POLLOUT);
   }

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("alsa_device_playback_ready"), DLT_UINT32(ret));

   return ret;
}

void alsa_device_startn(AlsaDevice_t *dev, void **ch_buf)
{
   int ret = 0;

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("alsa_device_startn"));

   if (NULL == dev)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_startn dev not init"));
   }
   else
   {
#ifndef USE_SND_PCM_LINK
      if ((ret = snd_pcm_prepare(dev->capture_handle)) < 0)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_prepare capture_handle"), DLT_STRING(snd_strerror(ret)));
      }

#endif
      if ((ret = snd_pcm_prepare(dev->playback_handle)) < 0)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_prepare play"), DLT_STRING(snd_strerror(ret)));
      }

//#ifdef USE_SILENCE
      ret = alsa_device_writen(dev, ch_buf, dev->period);
      if (0 > ret)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen pre-roll failed, PERIOD0"));
      }

      ret = alsa_device_writen(dev, ch_buf, dev->period);
      if (0 > ret)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen pre-roll failed, PERIOD1"));
      }
//#endif

#ifndef USE_SND_PCM_LINK
      /* it seem pcm are already running at this point and start is not needed in link mode.
       * I assume writen get things rolling. */
      snd_pcm_start(dev->capture_handle);

#endif
      snd_pcm_start(dev->playback_handle);
   }
}

snd_pcm_state_t alsa_device_state(AlsaDevice_t *dev, uint8_t rec_nPlay)
{
   snd_pcm_t *pcm = NULL;
   snd_pcm_state_t state = SND_PCM_STATE_LAST;

   if (NULL != dev)
   {
      pcm = (0 != rec_nPlay) ? dev->capture_handle : dev->playback_handle;

      if (NULL != pcm)
      {
         state = snd_pcm_state(pcm);
      }
   }

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_WARN, DLT_STRING("alsa_device_state (pcm/PnR/state)"), DLT_HEX32((uint32_t)pcm), DLT_UINT8(rec_nPlay), DLT_UINT32(state));

   return state;
}

int alsa_device_pause(AlsaDevice_t *dev, const uint8_t pause_nResume, void **ch_buf)
{
   int ret = 0;
   if (NULL != dev)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("alsa_device_pause"), DLT_UINT8(pause_nResume));

      if (pause_nResume)
      {
         ret = snd_pcm_drain(dev->playback_handle);
#ifndef USE_SND_PCM_LINK
         snd_pcm_drain(dev->playback_handle);
#endif
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("snd_pcm_drain"), DLT_STRING(snd_strerror(ret)));
      }
      else
      {
         /* feed silence buffer to restart */
         alsa_device_startn(dev, ch_buf);
      }
   }

   return ret;
}

void alsa_device_recover(AlsaDevice_t *dev, void **ch_buf, int err)
{
#if 0
   snd_pcm_prepare(dev->playback_handle);

   snd_pcm_writen(dev->playback_handle, ch_buf, AUDIO_TEST_PERIOD_SZ_FRAMES);

   snd_pcm_start(dev->playback_handle);
#else
   snd_pcm_recover(dev->playback_handle, -ESTRPIPE, 1);
#endif
}

int alsa_device_nfds(AlsaDevice_t *dev)
{
   return dev->writeN + dev->readN;
}

void alsa_device_getfds(AlsaDevice_t *dev, struct pollfd *pfds, unsigned int nfds)
{
   int i;
   assert(nfds >= dev->writeN + dev->readN);
   for (i = 0; i < dev->readN; i++)
      pfds[i] = dev->read_fd[i];
   for (i = 0; i < dev->writeN; i++)
      pfds[i + dev->readN] = dev->write_fd[i];
}
