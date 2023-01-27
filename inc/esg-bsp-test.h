#ifndef ESG_BSP_TEST
#define ESG_BSP_TEST
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

#include "dlt-client.h"

#define DLT_VERBOSITY_LOOP_THRESHOLD 100000U

typedef struct esg_bsp_test_settings
{
    uint32_t nb_loops;
    uint32_t verbosity;
} ebt_settings_t ;


int audio_init(pthread_t *runner, ebt_settings_t *settings);
int elite_tdma_init(pthread_t *runner, ebt_settings_t *settings);
int elite_uart_dsp_init(pthread_t *runner, ebt_settings_t *settings);

DLT_IMPORT_CONTEXT(dlt_ctxt_btst);

#endif /*ESG_BSP_TEST*/