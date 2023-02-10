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

static AlsaDevice_t *alsa_device_open(ebt_settings_t *settings, snd_pcm_stream_t direction)
{
   int err = (NULL != settings) ? EXIT_SUCCESS : -EINVAL;

   AlsaDevice_t *dev = malloc(sizeof(*dev));
   if (!dev)
      return NULL;

   if ((err = snd_pcm_open(&dev->pcm_handle, AUDIO_TEST_DEVICE_NAME, direction, 0)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_open (CnP/err)"), DLT_UINT8(dev->direction), DLT_STRING(snd_strerror(err)));
   }

   if (0 <= err)
   {
      err = alsa_device_hw_params(dev->pcm_handle, settings);
      if (0 <= err)
      {
         /* got period OK */
         dev->period = AUDIO_TEST_PERIOD_SZ_FRAMES;
      }
   }

   if (0 <= err)
   {
      snd_pcm_uframes_t avail_min = (SND_PCM_STREAM_PLAYBACK == direction) ? AUDIO_TEST_PERIOD_SZ_FRAMES : 0;

      err = alsa_device_sw_params(dev->pcm_handle, avail_min, settings);
   }

   dev->nb_poll_fd = snd_pcm_poll_descriptors_count(dev->pcm_handle);

   dev->poll_fd = malloc(dev->nb_poll_fd * sizeof(*dev->poll_fd));

   if (snd_pcm_poll_descriptors(dev->pcm_handle, dev->poll_fd, dev->nb_poll_fd) != dev->nb_poll_fd)
   {

      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_poll_descriptors failed"), DLT_STRING(snd_strerror(err)));
   }

   return dev;
}

AlsaDevice_t *alsa_device_open_play(ebt_settings_t *settings)
{
   AlsaDevice_t *dev = alsa_device_open(settings, SND_PCM_STREAM_PLAYBACK);

   return dev;
}

AlsaDevice_t *alsa_device_open_rec(ebt_settings_t *settings)
{
   AlsaDevice_t *dev = alsa_device_open(settings, SND_PCM_STREAM_CAPTURE);

   return dev;
}

int alsa_device_link(AlsaDevice_t *play_dev, AlsaDevice_t *rec_dev)
{
   int ret = ((NULL != play_dev) && (NULL != rec_dev)) ? EXIT_SUCCESS : -EINVAL;

   if (EXIT_SUCCESS == ret)
   {
      ret = snd_pcm_link(play_dev->pcm_handle, rec_dev->pcm_handle);
   }

   if (EXIT_SUCCESS != ret)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_link failed"), DLT_STRING(snd_strerror(ret)));
   }

   return ret;
}

void alsa_device_close(AlsaDevice_t *dev)
{
   DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("alsa_device_close"), DLT_UINT8(dev->direction));

   snd_pcm_close(dev->pcm_handle);
   free(dev);
}

snd_pcm_sframes_t alsa_device_readn(AlsaDevice_t *dev, void **ch_buf, const snd_pcm_uframes_t frames)
{
   snd_pcm_sframes_t err = EXIT_SUCCESS;

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_VERBOSE, DLT_STRING("alsa_device_readn"));

   if ((NULL == dev) || (SND_PCM_STREAM_CAPTURE != dev->direction))
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("invalid capture device!"));
      err = -EINVAL;
   }

   if (EXIT_SUCCESS == err)
   {
      err = snd_pcm_readn(dev->pcm_handle, ch_buf, frames);
   }

   if (frames > err)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_readn failed"), DLT_INT32(err));
#if 0
         if ((err = snd_pcm_prepare(dev->pcm_handle)) < 0)
         {
            DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_readn snd_pcm_prepare failed"), DLT_STRING(snd_strerror(err)));
         }
         if ((err = snd_pcm_start(dev->pcm_handle)) < 0)
         {
            DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_readn snd_pcm_start (recover) failed"), DLT_STRING(snd_strerror(err)));
         }
#else
      err = snd_pcm_recover(dev->pcm_handle, err, 1);
#endif
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

snd_pcm_sframes_t alsa_device_writen(AlsaDevice_t *dev, void **ch_buf, const snd_pcm_uframes_t frames)
{
   snd_pcm_sframes_t err;

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_VERBOSE, DLT_STRING("alsa_device_writen"));

   if ((NULL == dev) || (SND_PCM_STREAM_PLAYBACK != dev->direction))
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("invalid playback device!"));
      err = -EINVAL;
   }

   if (EXIT_SUCCESS == err)
   {
      err = snd_pcm_writen(dev->pcm_handle, ch_buf, frames);
   }

   if (frames > err)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen failed"), DLT_INT32(err));
#if 0
         /* try to recover */
         if ((err = snd_pcm_prepare(dev->playback_handle)) < 0)
         {
            DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen snd_pcm_prepare failed"), DLT_STRING(snd_strerror(err)));
         }

#else
      err = snd_pcm_recover(dev->pcm_handle, err, 1);
#endif
   }

   return err;
}

int alsa_device_ready(AlsaDevice_t *dev)
{
   unsigned short revents = 0;
   int ret = snd_pcm_poll_descriptors_revents(dev->pcm_handle, dev->poll_fd, dev->nb_poll_fd, &revents);

   uint8_t poll_dir = (SND_PCM_STREAM_PLAYBACK == dev->direction) ? POLLOUT : POLLIN;

   if (0 > ret)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_poll_descriptors_revents failed"), DLT_STRING(snd_strerror(ret)));
      return dev->poll_fd->revents & poll_dir;
   }
   else
   {
      ret = !!(revents & poll_dir);
   }

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("alsa_device_ready"), DLT_UINT32(ret));

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
      if ((ret = snd_pcm_prepare(dev->pcm_handle)) < 0)
      {
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_prepare"), DLT_STRING(snd_strerror(ret)));
      }

      if (SND_PCM_STREAM_PLAYBACK == dev->direction)
      {
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
      }

      snd_pcm_start(dev->pcm_handle);
   }
}

snd_pcm_state_t alsa_device_state(AlsaDevice_t *dev)
{
   return snd_pcm_state(dev->pcm_handle);
}

int alsa_device_pause(AlsaDevice_t *dev, const uint8_t pause_nResume, void **ch_buf)
{
   int ret = 0;
   if (NULL != dev)
   {
      if (pause_nResume)
      {
         snd_pcm_prepare(dev->pcm_handle);
         ret = snd_pcm_drain(dev->pcm_handle);
#ifndef USE_SND_PCM_LINK
         err
#endif
             DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("snd_pcm_drop"), DLT_STRING(snd_strerror(ret)));
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
   snd_pcm_recover(dev->pcm_handle, -ESTRPIPE, 1);
#endif
}
