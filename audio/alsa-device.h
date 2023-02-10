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

#ifndef ALSA_DEVICE_H
#define ALSA_DEVICE_H

#include "esg-bsp-test.h"
#include <alsa/asoundlib.h>
#include <sys/poll.h>

#define CAPTURE_FD_INDEX 0U
#define PLAYBACK_FD_INDEX 1U

#ifdef __cplusplus
extern "C"
{
#endif

   typedef struct AlsaDevice_
   {
      //  int channels;
      int period;
      snd_pcm_t *pcm_handle;
      int nb_poll_fd;
      struct pollfd *poll_fd;

      snd_pcm_stream_t direction;

   } AlsaDevice_t;

   /* init and setup */
   AlsaDevice_t *alsa_device_open_play(ebt_settings_t *settings);
   AlsaDevice_t *alsa_device_open_rec(ebt_settings_t *settings);

   int alsa_device_link(AlsaDevice_t *play_dev, AlsaDevice_t *rec_dev);

   void alsa_device_close(AlsaDevice_t *dev);

   int alsa_device_pause(AlsaDevice_t *dev, const uint8_t pause_nResume, void **ch_buf);

   void alsa_device_recover(AlsaDevice_t *dev, void **ch_buf, int err);

   snd_pcm_state_t alsa_device_state(AlsaDevice_t *dev);

   snd_pcm_sframes_t alsa_device_readn(AlsaDevice_t *dev, void **ch_buf,  const snd_pcm_uframes_t frames);

   snd_pcm_sframes_t alsa_device_writen(AlsaDevice_t *dev, void **ch_buf,  const snd_pcm_uframes_t frames);

   int alsa_device_ready(AlsaDevice_t *dev);

   void alsa_device_startn(AlsaDevice_t *dev, void **ch_buf);



#ifdef __cplusplus
}
#endif

#endif
