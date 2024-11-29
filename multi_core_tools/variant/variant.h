//==============================================================================
//! \addtogroup parametres PARAMETRES
//! \{
//!
//! \file   variant.h
//! \brief  manage product variant
//!
//! History :
//!   - 24/11/2023 : Creation by Vokkero by VOGO
//!
//! \}
//==============================================================================
#ifndef VARIANT_H
#define VARIANT_H


#include "variant-common.h"
#include <stdint.h>
// Audio-app use, variant comes from STM32

//------------------------ Variables ------------------------------------------
extern uint8_t variant_dsp[VARIANT_SIZE];

//----------------------------- Inline Functions--------------------------------
inline uint8_t variant_get_hw_product(void)
{
   return variant_dsp[VARIANT_HW_PRODUCT];
}

inline uint8_t variant_received(void)
{
   return variant_get_hw_product();
}

inline uint8_t variant_get_rd_trace(void)
{
   return ((variant_dsp[VARIANT_RD_7_0] & VARIANT_RD_7_0_TRACE_MASK)? 1 : 0);
}

inline uint8_t variant_get_rd_dbg_radio(void)
{
   return ((variant_dsp[VARIANT_RD_7_0] & VARIANT_RD_7_0_DBG_RADIO_MASK)? 1 : 0);
}

inline uint8_t variant_get_rd_dbg_dsp(void)
{
   return ((variant_dsp[VARIANT_RD_7_0] & VARIANT_RD_7_0_DBG_DSP_MASK)? 1 : 0);
}

inline uint8_t variant_get_rd_dbg_stm(void)
{
   return ((variant_dsp[VARIANT_RD_7_0] & VARIANT_RD_7_0_DBG_STM_MASK)? 1 : 0);
}

inline uint8_t variant_get_rd_15_8(void)
{
   return variant_dsp[VARIANT_RD_15_8];
}

inline uint8_t variant_get_rd_7_0(void)
{
   return variant_dsp[VARIANT_RD_7_0];
}

inline uint8_t variant_get_board(void)
{
   return variant_dsp[VARIANT_BOARD];
}

inline void variant_set_board(uint8_t board)
{
    variant_dsp[VARIANT_BOARD] = board;
}

inline uint8_t variant_get_functionsBitField_31_24(void)
{
   return variant_dsp[VARIANT_FUNCTION_BIT_FIELD_31_24];
}

inline uint8_t variant_get_functionsBitField_23_16(void)
{
   return variant_dsp[VARIANT_FUNCTION_BIT_FIELD_23_16];
}

inline uint8_t variant_get_functionsBitField_15_8(void)
{
   return variant_dsp[VARIANT_FUNCTION_BIT_FIELD_15_8];
}

inline uint8_t variant_get_functionsBitField_7_0(void)
{
   return variant_dsp[VARIANT_FUNCTION_BIT_FIELD_7_0];
}

inline uint8_t variant_get_gateway(void)
{
   return ((variant_dsp[VARIANT_FUNCTION_BIT_FIELD_31_24] & VARIANT_FCT_BIT_31_24_GTWY_MASK) ? 1 : 0);
}

inline uint8_t variant_get_wi(void)
{
   return ((variant_dsp[VARIANT_FUNCTION_BIT_FIELD_31_24] & VARIANT_FCT_BIT_31_24_WI_MASK) ? 1 : 0);
}

inline uint8_t variant_get_terminal(void)
{
   return ((variant_get_wi() || variant_get_gateway()) ? 0 : 1);
}

inline uint8_t variant_get_hw_lna_atten(void)
{
   return ((variant_dsp[VARIANT_FUNCTION_BIT_FIELD_7_0] & VARIANT_FCT_BIT_7_0_LNA_ATTEN_MASK) ? 1 : 0);
}

inline uint8_t variant_get_bt_tel_over_radio(void)
{
   return ((variant_dsp[VARIANT_FUNCTION_BIT_FIELD_7_0] & VARIANT_FCT_BIT_7_0_TEL_RADIO_MASK) ? 1 : 0);
}

inline uint8_t variant_get_mm_disabled(void)
{
   return ((variant_dsp[VARIANT_FUNCTION_BIT_FIELD_7_0] & VARIANT_FCT_BIT_7_0_MM_MASK) ? 1 : 0);
}

inline uint8_t variant_get_bt_enabled(void)
{
   return ((variant_dsp[VARIANT_FUNCTION_BIT_FIELD_7_0] & VARIANT_FCT_BIT_7_0_BT_MASK) ? 1 : 0);
}

inline uint8_t variant_get_audio_16k(void)
{
   return ((2==(variant_dsp[VARIANT_FUNCTION_BIT_FIELD_7_0] & VARIANT_FCT_BIT_7_0_AUDIO_MASK)) ? 1 : 0);
}

inline uint8_t variant_get_audio_8k(void)
{
   return ((1==(variant_dsp[VARIANT_FUNCTION_BIT_FIELD_7_0] & VARIANT_FCT_BIT_7_0_AUDIO_MASK)) ? 1 : 0);
}

inline uint8_t variant_get_hw_radio(void)
{
   return variant_dsp[VARIANT_HW_RADIO];
}

inline uint8_t variant_get_hw_audio(void)
{
   return variant_dsp[VARIANT_HW_AUDIO];
}

inline uint8_t variant_get_sub_type(void)
{
   return variant_dsp[VARIANT_SUB_TYPE];
}

inline uint8_t variant_get_freq_type(void)
{
   return variant_dsp[VARIANT_FREQ_TYPE];
}

#endif // VARIANT_H

