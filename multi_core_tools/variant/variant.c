//==============================================================================
//! \addtogroup parametres PARAMETRES
//! \{
//!
//! \file   variant.c
//! \brief  manage product variant
//!
//! History :
//!   - 28/08/2024 : Creation by Vokkero by VOGO
//!
//! \}
//==============================================================================

#include "variant.h"


// Audio-app use, variant comes from STM32

//------------------------ Variables ------------------------------------------
uint8_t variant_dsp[VARIANT_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
