//==============================================================================
//! \addtogroup radio MCO
//! \{
//!
//! \file 	commonMco1.h
//! \brief  structure d'echange STM/DSP
//!
//! Historique :
//! 	- 05/09/2019 : Creation par Adeunis
//!
//! \}
//==============================================================================
#ifndef _COMMON_MCO_1_H_
#define _COMMON_MCO_1_H_

//-------------------------------- Includes ------------------------------------

//------------------------- Options de compilation -----------------------------

//------------------------ Definitions/Constantes ------------------------------
// Size of exchange structure between STM32 and DSP
#define MCO1_X_STRUCT_SIZE (6) // Bytes, can be extended to 28 bytes => voir protdspSpiAudioExtData_t (protocoleDSP.h)

typedef enum
{
   record_off = 0,
   record_on,
   sdcard_80,
   record_error
} mco_record_status_t;

typedef struct
{
   union
   {
      uint16_t u16;
      struct
      {
         uint16_t id1 : 2;
         uint16_t id2 : 2;
         uint16_t id3 : 2;
         uint16_t id4 : 2;
         uint16_t id5 : 2;
         uint16_t id6 : 2;
         uint16_t id7 : 2;
         uint16_t id8 : 2;
      };
   };
} mco1AudioFrameQuality_1_8_t; //!< set by DSP

typedef struct
{
   union
   {
      uint16_t u16;
      struct
      {
         uint16_t id9 : 2;
         uint16_t id10 : 2;
         uint16_t id11 : 2;
         uint16_t id12 : 2;
         uint16_t id13 : 2;
         uint16_t id14 : 2;
         uint16_t id15 : 2;
         uint16_t rfu : 2;
      };
   };
} mco1AudioFrameQuality_9_15_t; //!< set by DSP

typedef struct
{
   union
   {
      uint16_t u16;
      struct
      {
         uint16_t recordingState : 2; //!< set by DSP // voir --> mco_record_status_t (ci-dessus)
         uint16_t micLevel : 2;       //!< set by DSP
         uint16_t micSat : 1;         //!< set by DSP
         uint16_t speakerLevel : 1;   //!< set by DSP
         uint16_t batteryLevel : 2;   //!< set by STM
         uint16_t alarm : 6;          //!< set by STM
         uint16_t dspDebug : 1;       //!< set by STM
         uint16_t rfu_1 : 1;          //!< set by STM
      };
   };
} mco1Miscellaneous_t;

typedef struct
{
   union
   {
      uint8_t buffer[MCO1_X_STRUCT_SIZE];
      struct
      {
         /** Pour une raison inexpliquee on ne peut creer une structure mco1AudioFrameQuality_t contenant un uint32_t sans que cela ne provoque un realignement 
 * de la strcuture protdspSpiAudioExtDataDsp2Stm_mco_t dans la quelle on integre comMco1_t
 * mco1Miscellaneous_t misc aligne sur 32 bits alors qu'apparemment il pourrait etre aligne sur 16 ??
 * Avec 2 structures mco1AudioFrameQuality_t 16 bits consecutives, mco1Miscellaneous_t est bien aligne sur 16 bits !!
 * On en peut malheureusement pas revenir en arriere sans crer une rupture de compatibilite entre les terminaux V01.08.00 et les terminaux de version inferieure         
 */
         mco1AudioFrameQuality_1_8_t audioQ;
         mco1AudioFrameQuality_9_15_t audioQ2;
         mco1Miscellaneous_t misc;
      };
   };
} comMco1_t;

//------------------------------- Warning --------------------------------------

//------------------------------ Variables -------------------------------------

//------------------------------- Macros ---------------------------------------

//----------------------------- Prototypes -------------------------------------

#endif //_COMMON_MCO_1_H_
