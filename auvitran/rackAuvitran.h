
#ifndef RACK_RUNNER
#define RACK_RUNNER
#pragma once

#include "dlt-client.h"

#define RACK_SPIDEV  "/dev/spidev1.0"

static const float DEFAULT_GAIN_AX4M_CHAN1 = -12.0;  //<! dB
static const float DEFAULT_GAIN_AX4M_CHAN4 = +35.0;  //<! dB

DLT_IMPORT_CONTEXT(dlt_ctxt_rack);

typedef enum
{
   IN = 0,
   OUT = 1,
   DIRECTION_INVALID = 2
} direction_t;

typedef enum
{
   FREQ_44_1k,
   FREQ_48k,
   FREQ_88_2k,
   FREQ_96k,
} sampling_rate_t;


uint32_t rack_initialize(void);
void rack_release(void);

int32_t rack_set_gain(direction_t direction, int channel, float gain);
int32_t rack_get_gain(direction_t direction, int channel, float *gain);
int32_t rack_set_pad_level(direction_t direction, int channel, uint8_t pad_level);
int32_t rack_get_pad_level(direction_t direction, int channel, uint8_t *pad_level);
int32_t rack_get_vumeter(direction_t direction, int channel, int *vu_pre, int *vu_post);
int32_t rack_set_sampling_rate(sampling_rate_t sampling_rate);

int32_t rack_get_card_version(int slot, uint8_t *pFIR, uint8_t *pEXT);

#endif //RACK_RUNNER