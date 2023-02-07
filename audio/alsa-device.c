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

#include "esg-bsp-test.h"
DLT_IMPORT_CONTEXT(dlt_ctxt_audio);

struct AlsaDevice_
{
   char *device_name;
   int channels;
   int period;
   snd_pcm_t *capture_handle;
   snd_pcm_t *playback_handle;
   int readN, writeN;
   struct pollfd *read_fd, *write_fd;
};

AlsaDevice *alsa_device_open(char *device_name, unsigned int rate, int channels, int period)
{
   int dir;
   int err;
   snd_pcm_hw_params_t *hw_params;
   snd_pcm_sw_params_t *sw_params;
   snd_pcm_uframes_t period_size = period;
   snd_pcm_uframes_t buffer_size = AUDIO_TEST_PERIODS * period;
   static snd_output_t *jcd_out;
   AlsaDevice *dev = malloc(sizeof(*dev));
   if (!dev)
      return NULL;
   dev->device_name = malloc(1 + strlen(device_name));
   if (!dev->device_name)
   {
      free(dev);
      return NULL;
   }
   strcpy(dev->device_name, device_name);
   dev->channels = channels;
   dev->period = period;
   err = snd_output_stdio_attach(&jcd_out, stdout, 0);

   if ((err = snd_pcm_open(&dev->capture_handle, dev->device_name, SND_PCM_STREAM_CAPTURE, 0)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_open capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_malloc capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_any(dev->capture_handle, hw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_any capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_set_access(dev->capture_handle, hw_params, AUDIO_TEST_SAMPLE_ACCESS)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_access capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_set_format(dev->capture_handle, hw_params, AUDIO_TEST_SAMPLE_FORMAT)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_format capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_set_rate_near(dev->capture_handle, hw_params, &rate, 0)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_rate_near capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   /*fprintf (stderr, "rate = %d\n", rate);*/

   if ((err = snd_pcm_hw_params_set_channels(dev->capture_handle, hw_params, channels)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_channels capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   period_size = period;
   dir = 0;
   if ((err = snd_pcm_hw_params_set_period_size_near(dev->capture_handle, hw_params, &period_size, &dir)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_period_size_near capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_set_periods(dev->capture_handle, hw_params, AUDIO_TEST_PERIODS, 0)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_period_size_near capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   buffer_size = AUDIO_TEST_PERIODS * period_size;
   dir = 0;
   if ((err = snd_pcm_hw_params_set_buffer_size_near(dev->capture_handle, hw_params, &buffer_size)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_buffer_size_near capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params(dev->capture_handle, hw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   /*snd_pcm_dump_setup(dev->capture_handle, jcd_out);*/
   snd_pcm_hw_params_free(hw_params);

   if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_malloc capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   if ((err = snd_pcm_sw_params_current(dev->capture_handle, sw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_current capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   if ((err = snd_pcm_sw_params_set_avail_min(dev->capture_handle, sw_params, period)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_set_avail_min capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   if ((err = snd_pcm_sw_params(dev->capture_handle, sw_params)) < 0)
   {
      fprintf(stderr, "cannot set software parameters (%s)\n",
              snd_strerror(err));
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params capture"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_open(&dev->playback_handle, dev->device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_open play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_malloc play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_any(dev->playback_handle, hw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_any play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_set_access(dev->playback_handle, hw_params, AUDIO_TEST_SAMPLE_ACCESS)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_access play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_set_format(dev->playback_handle, hw_params, AUDIO_TEST_SAMPLE_FORMAT)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_format play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params_set_rate_near(dev->playback_handle, hw_params, &rate, 0)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_rate_near play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   /*fprintf (stderr, "rate = %d\n", rate);*/

   if ((err = snd_pcm_hw_params_set_channels(dev->playback_handle, hw_params, channels)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_channels play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   period_size = period;
   dir = 0;
   if ((err = snd_pcm_hw_params_set_period_size_near(dev->playback_handle, hw_params, &period_size, &dir)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_period_size_near play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   if ((err = snd_pcm_hw_params_set_periods(dev->playback_handle, hw_params, AUDIO_TEST_PERIODS, 0)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_periods play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   buffer_size = period_size * 2;
   dir = 0;
   if ((err = snd_pcm_hw_params_set_buffer_size_near(dev->playback_handle, hw_params, &buffer_size)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params_set_buffer_size_near play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   if ((err = snd_pcm_hw_params(dev->playback_handle, hw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_hw_params play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

   /*snd_pcm_dump_setup(dev->playback_handle, jcd_out);*/
   snd_pcm_hw_params_free(hw_params);

   if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_malloc play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   if ((err = snd_pcm_sw_params_current(dev->playback_handle, sw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_current play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   if ((err = snd_pcm_sw_params_set_avail_min(dev->playback_handle, sw_params, period)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_set_avail_min play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   if ((err = snd_pcm_sw_params_set_start_threshold(dev->playback_handle, sw_params, period)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params_set_start_threshold play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   if ((err = snd_pcm_sw_params(dev->playback_handle, sw_params)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_sw_params play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

#if 0
   if ((err = snd_pcm_link(dev->capture_handle, dev->playback_handle)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_link failed"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
#endif

   if ((err = snd_pcm_prepare(dev->capture_handle)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_prepare capture_handle"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }
   if ((err = snd_pcm_prepare(dev->playback_handle)) < 0)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_prepare play"), DLT_STRING(snd_strerror(err)));
      assert(0);
   }

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

void alsa_device_close(AlsaDevice *dev)
{
   DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("alsa_device_close"));

   snd_pcm_close(dev->capture_handle);
   snd_pcm_close(dev->playback_handle);
   free(dev->device_name);
   free(dev);
}

snd_pcm_sframes_t alsa_device_readn(AlsaDevice *dev, void **ch_buf, int len)
{
   snd_pcm_sframes_t err;

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_DEBUG, DLT_STRING("alsa_device_readn"));

   if ((err = snd_pcm_readn(dev->capture_handle, ch_buf, len)) != len)
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
   }
   return err;
}

snd_pcm_sframes_t alsa_device_writen(AlsaDevice *dev, void **ch_buf, int len)
{
   snd_pcm_sframes_t err;

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_DEBUG, DLT_STRING("alsa_device_writen"));

   if ((err = snd_pcm_writen(dev->playback_handle, ch_buf, len)) != len)
   {
      if (err < 0)
      {
         if (err == -EPIPE)
         {
            fprintf(stderr, "An underrun has occured, reseting playback, len=%d\n", len);
            DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen X-RUN"));
         }
         else
         {
            fprintf(stderr, "write to audio interface failed (%s)\n",
                    snd_strerror(err));
            DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen failed"));
         }
         if ((err = snd_pcm_prepare(dev->playback_handle)) < 0)
         {
            fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                    snd_strerror(err));
            DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen snd_pcm_prepare failed"));
         }
      }
      else
      {
         fprintf(stderr, "Couldn't write as many samples as I wanted (%d instead of %d)\n", err, len);
         DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_writen snd_pcm_prepare failed"), DLT_UINT32(err));
      }
   }
   return err;
}

int alsa_device_readi(AlsaDevice *dev, void *buf, int len)
{
   int err;
   /*fprintf (stderr, "-");*/
   if ((err = snd_pcm_readi(dev->capture_handle, buf, len)) != len)
   {
      if (err < 0)
      {
         // fprintf(stderr, "error %d, EPIPE = %d\n", err, EPIPE);
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
         /*alsa_device_read(dev,pcm,len);*/
      }
      else
      {
         fprintf(stderr, "Couldn't read as many samples as I wanted (%d instead of %d)\n", err, len);
      }
      return 1;
   }
   return 0;
}

int alsa_device_writei(AlsaDevice *dev, const void *buf, int len)
{
   int err;
   /*fprintf (stderr, "+");*/
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

int alsa_device_capture_ready(AlsaDevice *dev, struct pollfd *pfds, unsigned int nfds)
{
   unsigned short revents = 0;
   int err;

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_DEBUG, DLT_STRING("alsa_device_capture_ready"));

   if ((err = snd_pcm_poll_descriptors_revents(dev->capture_handle, pfds, dev->readN, &revents)) < 0)
   {
      // cerr << "error in snd_pcm_poll_descriptors_revents for capture: " << snd_strerror (err) << endl;
      // FIXME: This is a kludge
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_poll_descriptors_revents C failed"), DLT_STRING(snd_strerror(err)));

      return pfds[0].revents & POLLIN;
   }
   // cerr << (revents & POLLERR) << endl;
   return revents & POLLIN;
}

int alsa_device_playback_ready(AlsaDevice *dev, struct pollfd *pfds, unsigned int nfds)
{
   unsigned short revents = 0;
   int err;

   DLT_LOG(dlt_ctxt_audio, DLT_LOG_DEBUG, DLT_STRING("alsa_device_playback_ready"));

   if ((err = snd_pcm_poll_descriptors_revents(dev->playback_handle, pfds + dev->readN, dev->writeN, &revents)) < 0)
   {
      // cerr << "error in snd_pcm_poll_descriptors_revents for playback: " << snd_strerror (err) << endl;
      // FIXME: This is a kludge
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("snd_pcm_poll_descriptors_revents P failed"), DLT_STRING(snd_strerror(err)));
      return pfds[1].revents & POLLOUT;
   }
   // cerr << (revents & POLLERR) << endl;
   return revents & POLLOUT;
}

void alsa_device_startn(AlsaDevice *dev, void **ch_buf)
{
   DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("alsa_device_startn"));

   if (NULL == dev)
   {
      DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("alsa_device_startn dev not init"));
   }
   else
   {
      snd_pcm_sframes_t r = alsa_device_writen(dev, ch_buf, dev->period);

      alsa_device_writen(dev, ch_buf, dev->period);

      DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("snd_pcm_start"));

      snd_pcm_start(dev->capture_handle);
      snd_pcm_start(dev->playback_handle);
   }
}

int alsa_device_nfds(AlsaDevice *dev)
{
   return dev->writeN + dev->readN;
}

void alsa_device_getfds(AlsaDevice *dev, struct pollfd *pfds, unsigned int nfds)
{
   int i;
   assert(nfds >= dev->writeN + dev->readN);
   for (i = 0; i < dev->readN; i++)
      pfds[i] = dev->read_fd[i];
   for (i = 0; i < dev->writeN; i++)
      pfds[i + dev->readN] = dev->write_fd[i];
}
