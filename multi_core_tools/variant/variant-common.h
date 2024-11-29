//==============================================================================
//! \addtogroup variant COMMON
//! \{
//!
//! \file 	variant-common.h
//! \brief  multicore common variant definition
//!
//! \}
//==============================================================================
#ifndef VARIANT_COMMON_H
#define VARIANT_COMMON_H

//------------------------ Define / Constant -----------------------------------
#define VARIANT_SIZE (16)

// Variant byte index
#define VARIANT_RD_15_8 (15)
//--
#define VARIANT_RD_7_0  (14)
#define VARIANT_RD_7_0_TRACE_MASK (1<<3)
#define VARIANT_RD_7_0_DBG_RADIO_MASK (1<<2)
#define VARIANT_RD_7_0_DBG_DSP_MASK (1<<1)
#define VARIANT_RD_7_0_DBG_STM_MASK (1<<0)
//--
#define VARIANT_TBD_4   (13)
//--
#define VARIANT_TBD_3   (12)
//--
#define VARIANT_TBD_2   (11)
//--
#define VARIANT_TBD_1   (10)
//--
#define VARIANT_BOARD   (9)
//--
#define VARIANT_FUNCTION_BIT_FIELD_31_24  (8)
#define VARIANT_FCT_BIT_31_24_GTWY_MASK (1<<7)
#define VARIANT_FCT_BIT_31_24_WI_MASK (1<<6)
//--
#define VARIANT_FUNCTION_BIT_FIELD_23_16  (7)
//--
#define VARIANT_FUNCTION_BIT_FIELD_15_8   (6)
//--
#define VARIANT_FUNCTION_BIT_FIELD_7_0    (5)
#define VARIANT_FCT_BIT_7_0_LNA_ATTEN_MASK (1<<5)
#define VARIANT_FCT_BIT_7_0_TEL_RADIO_MASK (1<<4)
#define VARIANT_FCT_BIT_7_0_MM_MASK (1<<3)
#define VARIANT_FCT_BIT_7_0_BT_MASK (1<<2)
#define VARIANT_FCT_BIT_7_0_AUDIO_MASK (3<<0)
//--
#define VARIANT_HW_PRODUCT (4)
//--
#define VARIANT_HW_RADIO   (3)
//--
#define VARIANT_HW_AUDIO   (2)
//--
#define VARIANT_SUB_TYPE   (1)
//--
#define VARIANT_FREQ_TYPE  (0)

// Board variant code
#define VARIANT_BOARD_INVALID (0)
#define VARIANT_BOARD_8097    (1)
#define VARIANT_BOARD_8350CDE (2)
#define VARIANT_BOARD_8350F   (3)
//---- Max Board variant code is (255)
//---- Board variant code end

// Product variant code
#define VARIANT_PRODUCT_LEGACY         (0)
#define VARIANT_PRODUCT_GUARDIAN       (1)
#define VARIANT_PRODUCT_GUARDIANSHOW   (2)
#define VARIANT_PRODUCT_ELITE          (3)
#define VARIANT_PRODUCT_UNITY          (4)
#define VARIANT_PRODUCT_ELITE_PLUS     (5)
//---- Max Product variant code is (255)
//---- Product variant code end

#endif //VARIANT_COMMON_H


