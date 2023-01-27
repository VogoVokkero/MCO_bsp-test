#ifndef ESG_BSP_TEST
#define ESG_BSP_TEST
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

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