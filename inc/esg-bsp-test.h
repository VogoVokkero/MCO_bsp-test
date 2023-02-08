#ifndef ESG_BSP_TEST
#define ESG_BSP_TEST
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "dlt-client.h"

/* GENERAL TEST PARAMETERS */

#define DLT_VERBOSITY_LOOP_THRESHOLD 100000U

/* AUDIO TEST PARAMETERS */
#define AUDIO_TEST_RATE 48000U
#define AUDIO_TEST_PERIODS 2
#define AUDIO_TEST_PERIOD_TIME_US 20000U
#define AUDIO_TEST_SAMPLE_SZ_BYTES 4U
#define AUDIO_TEST_CHANNELS 4U
#define AUDIO_TEST_SAMPLE_FORMAT SND_PCM_FORMAT_S32_LE

#define AUDIO_TEST_FRAME_SZ_BYTES (AUDIO_TEST_CHANNELS * AUDIO_TEST_SAMPLE_SZ_BYTES)
#define AUDIO_TEST_BUFFER_TIME_US (AUDIO_TEST_PERIODS * AUDIO_TEST_PERIOD_TIME_US)
#define AUDIO_TEST_PERIOD_SZ_FRAMES (AUDIO_TEST_RATE * AUDIO_TEST_PERIOD_TIME_US / 1000000)
#define AUDIO_TEST_BUFFER_SZ_FRAMES (AUDIO_TEST_RATE * AUDIO_TEST_BUFFER_TIME_US / 1000000)
#define AUDIO_TEST_BUFFER_SZ_BYTES (AUDIO_TEST_BUFFER_SZ_FRAMES * AUDIO_TEST_FRAME_SZ_BYTES)

#if 0
#define ALSA_DEVICE "hw:0,0"
#define AUDIO_TEST_SAMPLE_ACCESS SND_PCM_ACCESS_RW_INTERLEAVED
#else
#define ALSA_DEVICE "sysdefault:CARD=axcavb"
#define AUDIO_TEST_SAMPLE_ACCESS SND_PCM_ACCESS_RW_NONINTERLEAVED
#endif


typedef struct esg_bsp_test_settings
{
    uint32_t nb_loops;
    uint32_t verbosity;
    uint32_t pause_stress;
} ebt_settings_t ;


int audio_init_wait(pthread_t *runner, ebt_settings_t *settings);
int audio_init_poll(pthread_t *runner, ebt_settings_t *settings);
int elite_tdma_init(pthread_t *runner, ebt_settings_t *settings);
int elite_uart_dsp_init(pthread_t *runner, ebt_settings_t *settings);

DLT_IMPORT_CONTEXT(dlt_ctxt_btst);

#endif /*ESG_BSP_TEST*/