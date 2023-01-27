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

#endif /*ALSA_AUDIO_RUNNER*/