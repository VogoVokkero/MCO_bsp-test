#ifndef ALSA_AUDIO_RUNNER
#define ALSA_AUDIO_RUNNER
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
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


typedef struct{
   struct pollfd *ufds;
   unsigned int count;
   //void (*callback)(void);
}pcmAlsa_event_t;

typedef struct{
   const char *name;                 //ex: "plughw:0,0";  Audio device
   unsigned int rate;            //ex: 48000;   stream rate ( 4000 <= rate <= 196000) 
   unsigned int channels;        //ex: 1;       count of channels ( 1 <= channels <= 1024) 
   unsigned int buffer_time;     //ex: 20000;   ring buffer length in us ( 1000 <= buffer_time <= 1000000) 
   unsigned int period_time;     //ex: 10000;   period time in us ( 1000 <= period_time <= 1000000) 
   snd_pcm_format_t format;      //ex: SND_PCM_FORMAT_S16;    sample format
   int period_event;             //ex: 0;       produce poll (or select) event after each period
   int resample;                 //ex: 1;       enable alsa-lib resampling
   snd_pcm_hw_params_t *hwparams;
   snd_pcm_sw_params_t *swparams;

   pcmAlsa_event_t fds;          // file descriptors
   snd_pcm_t *handle;

   snd_input_t *input;

   snd_pcm_stream_t stream_direction;  //Capture or playback
   snd_pcm_access_t  access;        //SND_PCM_ACCESS_RW_NONINTERLEAVED or SND_PCM_ACCESS_RW_INTERLEAVED --> reading of each samples
   uint8_t bytes_per_sample;         //Sample size in byte

   snd_pcm_sframes_t buffer_size;   //Buffer size (periode*sample_size )
   snd_pcm_sframes_t period_size;   //Samples by periodes
   snd_pcm_channel_area_t *areas;   //description of the under samples buffer (chanels, sample size...)
   /*signed short **/uint8_t *samples;           //buffer to fill if playback, buffer that will be writen if capture
   void **buff_none_interleave_ptr;
}pcmAlsa_device_t;


#endif /*ALSA_AUDIO_RUNNER*/