//==============================================================================
//! \addtogroup common PARAMETRES COMMUNS
//! \{
//!
//! \file   protocoleDSP.h
//! \brief  definition des elements communs DSP/STM
//!            - definition protocole SPI
//!            - definition protocole UART
//!
//! Historique :
//!   - 02/12/2015 : Creation par Adeunis RF (SFBC)
//!
//! \}
//==============================================================================
#ifndef _PROTOCOLE_DSP_H_
#define _PROTOCOLE_DSP_H_

#pragma pack(1)

#include "commParam.h"
#include "commonMco1.h"

//------------------------- Options de compilation -----------------------------

//------------------------ Definitions/Constantes ------------------------------

//================================================================
// protocole SPI
//================================================================
#define PROTDSP_SPI_DEBIT        (4000000) //< Debit SPI en Hz

//!< structure des donnees des trames SPI Rx et Tx entre DSP et STM
//!<================================================================
//!<  -------------------------------  ------------------------------------
//!<  | header | Frame 0 | Frame 1 | ... | Frame N | Extended data | xxxx |  x = non defini, presentes pour remplir la trame SPI
//!<  -------------------------------  ------------------------------------

//! Liste des codes fonction de la trame SPI(protdspSpiHeader_t)
typedef enum
{
   PROTDSP_SPI_HDR_CODE_NULL = 0,   //!< Code invalide
   PROTDSP_SPI_HDR_CODE_AUDIO1,     //!< Trame audio compressee avec data supplementaires
}PROTDSP_SPI_HDR_CODE_LIST;

typedef enum
{
   PROTDSP_SPI_AUDIO_EXTDATA_CODE_NULL = 0,
   PROTDSP_SPI_AUDIO_EXTDATA_CODE_MCO = 1,
   PROTDSP_SPI_AUDIO_EXTDATA_STM_TO_DSP = 2, // Standard extra data payload sent from STM to DSP
}PROTDSP_SPI_AUDIO_EXTDATA_CODE_LIST;

//! Header des trames  SPI (protdspSpiFrame_t)
#define PROTDSP_SPI_HEADER_VERSION  (0x01)
typedef struct
{  // !! DSP : aligner les uint32_t sur @4 et les uint16_t sur @2
   uint8_t hdrVersion;  //!< Version du header
   uint8_t hdrCode;     //!< Code fonction du header (PROTDSP_SPI_HDR_CODE_LIST)
   uint16_t frameSize;  //!< Taille de la trame de donnees qui suit
   uint8_t frameNb;     //!< Nombre de trame de donnee qui suivent
   uint8_t extSize;     //!< Nombre de data qui suivent l'ensemble des buffer de données
   uint8_t index;       //!< Index de la trame : Compteur SPI de l'expediteur       (was : Index de la trame : compteur de cycles radio : fourni par le STM vers DSP)
   uint8_t libre;
}protdspSpiHeader_t;

typedef struct
{
   union // Reception slot will be filled in relation to audstrInfo_t<etat> (AUDSTR_ETAT_LIST)
   {
	  uint8_t u8;
	  struct
	  {
		 uint8_t audio : 1;
		 uint8_t repetition : 1;
		 uint8_t audioErr : 1;
		 uint8_t repetitionErr : 1;
		 uint8_t rfu : 4;
	  };
   };
}protdspAudioSlotInfo_t;

//! Header de la trame SPI audio (protdspSpiAudioFrame_t)
#define PROTDSP_SPI_AUDIO_VERSION   0x01
#define PROTDSP_SPI_SLOT_ALLOC_DYNAMIC       0
#define PROTDSP_SPI_SLOT_ALLOC_PRIVILEGED1   1
#define PROTDSP_SPI_SLOT_ALLOC_PRIVILEGED2   2

typedef struct
{ // !! DSP : aligner les uint32_t sur @4 et les uint16_t sur @2
   uint8_t audioVersion;   //!< Version de la trame audio
   uint8_t NumVoix;        //!< Indice de la voix recue + startOfVocoder + RESERVED //old code compatibility
   uint16_t AudioBuffSize; //!< Taille du buffer Audio[] : pour s'adapter au codec
   union
   {
      struct{
         uint8_t plc_lookhead : 3;
         uint8_t audioMode    : 2;
         uint8_t whistle		: 1;
         uint8_t duckMode		: 1;
         uint8_t unused       : 1;
      };
      uint8_t AudioCtrl;      //!< Controle audio : audioHorizon (bits 0-2 : plc_lookhead) + audio mode [bit4-5] : 8/16/24kHz
   };
   union    //must be identical to audstrInfo_t<union>
   {
      struct{
         uint8_t id              :4;   //!< Identifiant du terminal
         uint8_t role            :3;   //!< Role du terminal
         uint8_t startOfVocoder  :1;   //!< Nombre de data qui suivent l'ensemble des buffer de donnees
      };
      uint8_t userId;                  //!< Identifiant utilisateur (pour le mixage en RX) (/!\ Id != role)
   };
   protdspAudioSlotInfo_t rxSlot;      //!< Need to be sizeof(uint8_t) for compatibility
   uint8_t slotAllocation;             //!< If non-zero privileged slot allocation number 
} protdspSpiAudioHdrFrame_t;

//! Trame SPI audio
typedef struct
{ // !! DSP : aligner les uint32_t sur @4 et les uint16_t sur @2
   protdspSpiAudioHdrFrame_t hdr;               //!< Header de la trame
   uint8_t Audio[COMMPAR_AUDIO_DATA_SIZE_MAX];  //!< Donnees compresse audio pour une voix
} protdspSpiAudioFrame_t;

#define PROTDSP_AUDIO_CTRL_PLC_POS        (0)   //!< bit<2:0>
#define PROTDSP_AUDIO_CTRL_PLC_MASK       (7<<PROTDSP_AUDIO_CTRL_PLC_POS)	// mask to extract plc from AudioCtrl
#define PROTDSP_AUDIO_CTRL_PLCMAX         (PROTDSP_AUDIO_CTRL_PLC_MASK)
#define PROTDSP_AUDIO_CTRL_TYPE_POS       (3)   //!< bit<4:3>
#define PROTDSP_AUDIO_CTRL_TYPE_MASK      (3<<PROTDSP_AUDIO_CTRL_TYPE_POS)	// mask to extract type (audioMode) from AudioCtrl
#define PROTDSP_AUDIO_CTRL_TYPE_8K        (0<<PROTDSP_AUDIO_CTRL_TYPE_POS)
#define PROTDSP_AUDIO_CTRL_TYPE_16K       (1<<PROTDSP_AUDIO_CTRL_TYPE_POS)
#define PROTDSP_AUDIO_CTRL_TYPE_24K       (2<<PROTDSP_AUDIO_CTRL_TYPE_POS)	// mask to extract whistle from AudioCtrl
#define PROTDSP_AUDIO_CTRL_WHISTLE_POS    (5)   //!< bit<5>
#define PROTDSP_AUDIO_CTRL_WHISTLE_MASK   (1<<PROTDSP_AUDIO_CTRL_WHISTLE_POS)
#define PROTDSP_AUDIO_CTRL_DUCK_POS       (6)   //!< bit<6>
#define PROTDSP_AUDIO_CTRL_DUCK_MASK      (1<<PROTDSP_AUDIO_CTRL_DUCK_POS)

//! Macro to read "plc_lookhead" in "AudioCtrl"
#define ProtDspAudioGetFrameAudioCtrl_plc(ptr)		((ptr)->hdr.AudioCtrl & PROTDSP_AUDIO_CTRL_PLC_MASK)

//! Macro pour lire "le type de trame audio"
#define ProtDspAudioGetFrameAudioCtrl_Type(ptr)     ((ptr)->hdr.AudioCtrl & PROTDSP_AUDIO_CTRL_TYPE_MASK)

//! Macro to read "whistle" in "AudioCtrl"
#define ProtDspAudioGetFrameAudioCtrl_whistle(ptr)	((ptr)->hdr.AudioCtrl & PROTDSP_AUDIO_CTRL_WHISTLE_MASK)

//! Macro to set "duck" in "AudioCtrl"
#define ProtDspAudioGetFrameAudioCtrl_duck(ptr)	((ptr)->hdr.AudioCtrl |= PROTDSP_AUDIO_CTRL_DUCK_MASK)

#define PROTDSP_AUDIO_CTRL_NO_AUDIO       (0x00)   //!< Valeur de "audioCtrl" pour preciser que la trame audio ne contient pas d'audio
#define PROTDSP_AUDIO_CTRL_MASK			  (0x07)   //!< Bits 0 a 3 de hdr.AudioCtrl = masque pour extraire "audioCtrl" ???? ETRANGE ????

//! Macro pour verifier si la trame audio contient des donnees audio
#define ProtDspAudioFrameIsEmpty(ptr)              (PROTDSP_AUDIO_CTRL_NO_AUDIO == ((ptr)->hdr.AudioCtrl & PROTDSP_AUDIO_CTRL_MASK) )
//! Macro pour ecrire "audioCtrl" et preciser que la trame audio ne contient pas d'audio (on ne connait donc par le type : il reste a '0')
#define ProtDspAudioSetFrameNoAudio(ptr)                 ((ptr)->hdr.AudioCtrl = PROTDSP_AUDIO_CTRL_NO_AUDIO)
//! Macro pour ecrire "audioCtrl"
#define ProtDspAudioSetFrameAudioControl(ptr, plc, type, whistle, duck) {	(ptr)->hdr.AudioCtrl =\
																		(plc & PROTDSP_AUDIO_CTRL_PLC_MASK) |\
																		(type & PROTDSP_AUDIO_CTRL_TYPE_MASK) |\
																		(whistle & PROTDSP_AUDIO_CTRL_WHISTLE_MASK) |\
                                                      (duck & PROTDSP_AUDIO_CTRL_DUCK_MASK); }

//! Donnees des trame SPI audio : donnees supplementaires communes a toutes les trames
#define PROTDSP_SPI_AUDIO_EXT_VERSION  (0x01)
#define PROTDSP_CYCLE_NB_TX_MAX        (3)   //!< nb max d'emission par cycle radio : beacon + audio + repet
#define PROTDSP_EXTDATA_SIZE           (28)  //!< nb de data dans les info etendues : 7 mot 32bits

typedef struct _protdspVumetreValues
{
   int16_t currentValue;    //!< vumetre : valeur courante
   int16_t min;             //!< vumetre : valeur min
   int16_t max;             //!< vumetre : valeur max
   int16_t lowThreshold;     //!< vumetre : low threshold
   int16_t highThreshold;    //!< vumetre : high threshold
}protdspVumetreValues;

typedef struct //!! attention verifier que la taille de cette structure ne depasse pas PROTDSP_EXTDATA_SIZE
{ // !! DSP : aligner les uint32_t sur @4 et les uint16_t sur @2
	comMco1_t mco1;         // /!\ 3 x uint16_t 
	protdspVumetreValues vu_mic;
	protdspVumetreValues vu_hp;
	union{
		uint8_t record_reg;
		struct{
			uint8_t unused_b : 6;
			uint8_t rec_status : 2;  //0: record stop; 1: recording; 2: SDcard 80% full; 3: SDcard error
		};
	};
	uint8_t unused[1];
}protdspSpiAudioExtDataDsp2Stm_mco_t;

typedef struct //!! attention verifier que la taille de cette structure ne depasse pas PROTDSP_EXTDATA_SIZE
{ // !! DSP : aligner les uint32_t sur @4 et les uint16_t sur @2
   int32_t StartTx[PROTDSP_CYCLE_NB_TX_MAX];  //!< Position du debut de l'emission par rapport au top 10ms (StartCycle10ms) en us
   int32_t DureeTx[PROTDSP_CYCLE_NB_TX_MAX];  //!< Duree de l'emission en us
   uint16_t pio_field; //!< !! USED ONLY BY ESG mco-audioApp !! <bit n> pio state from terminal with userId = n + 1 (15 >= n >= 0) 
   uint16_t pio_valid;
}protdspSpiAudioExtDataSTMtoDSP_t;

typedef struct
{ // !! DSP : aligner les uint32_t sur @4 et les uint16_t sur @2
   uint8_t audioExtVersion; //!< Audio extended data frame version, change value only on frame format change
   uint8_t dataCode;		//!< See PROTDSP_SPI_AUDIO_EXTDATA_CODE_LIST to define data payload type /!\ only from DSP to STM
   uint8_t libre2;
   uint8_t libre3;
   union					//!< DSP event
   {
      uint32_t u32;
      struct 
      {
         uint32_t pio : 1; // pio input state (Set by DSP)
         uint32_t vorTech_nVAR : 1; //!< On ESG define who speak VORTech (1) or VAR (0) (Set by ESG audioApp )
         uint32_t reserved : 30;
      }bit;
   }event;
   union
   {   // Data payload, max size must be PROTDSP_EXTDATA_SIZE, data type defined by dataCode field (see above)
	   uint8_t data[PROTDSP_EXTDATA_SIZE];		//!< Generic data payload, it is the maximum allowed payload

      // From STM to DSP data payload (only one frame allowed at this time
      protdspSpiAudioExtDataSTMtoDSP_t ext_data;
      
      // From DSP to STM data payload in relation to dataCode
      protdspSpiAudioExtDataDsp2Stm_mco_t mco; //!< Mco data payload
   };
}protdspSpiAudioExtData_t;

//! Taille du champ frame de la trame SPI (protdspSpiFrame_t)
#define PROTDSP_SPI_FRAME_SIZE      (COMMPAR_AUDIO_VOIX_MAX * sizeof(protdspSpiAudioFrame_t) + sizeof(protdspSpiAudioExtData_t))

typedef uint32_t protdspcrc16_t;
//! Trame complete SPI
typedef struct
{  // !! DSP : aligner les uint32_t sur @4 et les uint16_t sur @2
   protdspcrc16_t crc16;
   protdspSpiHeader_t header;
   uint8_t frame[PROTDSP_SPI_FRAME_SIZE]; // 5 trames audio (TX0, RX{0..3}), extradata
}protdspSpiFrame_t;

//! Taille MAX de la trame SPI (protdspSpiFrame_t)
#define PROTDSP_SPI_TOTAL_SIZE      ( sizeof(protdspSpiFrame_t) )


//================================================================
// protocole UART
//================================================================
#define PROTDSP_UART_DEBIT                (115200) //!< Debit UART en communiction en Hz

//! liste des bips
typedef enum _DSP_BIP_LIST
{
   BIP_NONE = 0,     //!< Arret du bip en cours
   BIP_APPUI,        //!< Bip appui touche
   BIP_LIMITE,       //!< Bip reglage en limite
   BIP_CONTINU,      //!< Bip continu
   BIP_DISCONTINU,   //!< Bip discontinu : train de bips
   BIP_APPEL,        //!< Generation tonalite lors d'un appel telephone entrant
   BIP_NB,
   
   BIP_BUZ_LIST = 30,
   BIP_BUZ_NONE = BIP_BUZ_LIST,     //!< Arret du bip (buzzer) en cours
   BIP_BUZ_1KHZ,                    //!< Generation d'un bip 1khz sur le buzzer
   BIP_BUZ_LIST_END,                //!< Fin de la liste des bip buzzer

} DSP_BIP_LIST ;
#define DSP_BIP_DUREE_INFINI        (0)

typedef enum _DSP_LED_MODE_LIST
{
   LED_DSP_MODE_OFF = 0, 	//!< extinction de la led
   LED_DSP_MODE_ON,      	//!< Allumage de la led
   LED_DSP_MODE_TGL,     //!< Clignottement de la led selon une periode specifiee
   LED_DSP_MODE_REC_OFF,	//!< Recorder Off
	//priority modes
	LED_DSP_MODE_PRIORITY,	//!< Recorder ON, priority mode, this mode is momentarily interruptible by LED_DSP_MODE_TGL with a limited duration
	LED_DSP_MODE_REC_ON = LED_DSP_MODE_PRIORITY,  //!< Recorder ON, priority mode, this mode is momentarily interruptible by LED_DSP_MODE_TGL with a limited duration
   LED_DSP_MODE_REC_TGL, 	//!< Recorder TGL, priority mode, this mode is momentarily interruptible by LED_DSP_MODE_TGL with a limited duration
   LED_DSP_MODE_NBR,
   // WIV2
   LED_WI2_ACCESS = LED_DSP_MODE_NBR,
   LED_WI2_NETWORK,
   LED_WI2_MAIN,
   LED_WI2_BACKUP
} DSP_LED_MODE_LIST ;

typedef enum
{
   LED_RED = 1<<0,
   LED_GREEN = 1<<1,
   LED_BLUE = 1<<2,
} DSP_LED_RGB;

typedef struct _protdspParam_t
{  // ATTENTION A ALIGNER les 32 bits et les 16 bits
   // modes de fonctionnement du TS
   uint8_t  aesKey[COMMPAR_AES128_KEY_LEN];  //<! Chiffrement AES de l'audio
   uint32_t dureeCycleRadio;     //!< duree du cycle radio en us
   // aligne 32 bits
   uint8_t  rb;                  //!< type de rb (voir COMMPAR_RB_MODE_LIST)
   uint8_t  vad;                 //!< type de vad (voir COMMPAR_VAD_MODE_LIST)
   uint8_t  antiLarsen;          //!< mode d'anti larsen (voir COMMPAR_LARSEN_MODE_LIST)
   uint8_t  audioMode;           //!< mode de test audio (voir COMMPAR_AUDIOMODE_LIST)
   // aligne 32 bits
   uint8_t  userId;              //!< Id utilisateur (/!\ Id != role)
   uint8_t  nbVoixMax;           //!< Nb de voix max
   uint8_t  aec ;                //!< type d'annuleur d'echo (voir COMMPAR_AEC_MODE_LIST)
   uint8_t  antiBurst;           //!< mode d'anti burst (voir COMMPAR_BURST_MODE_LIST)
   // aligne 32 bits
   uint8_t  farVoice;            //!< mode de gestion du far voice (voir COMMPAR_FARVOICE_MODE_LIST)
   uint8_t  micCag;              //!< mode de gestion de la CAG microphone(voir COMMPAR_CAGMODE_MODE_LIST)   
   uint8_t  micEqualizer;        //!< mode d'egalisation du microphone
   uint8_t  plc;                 //!< mode du plc
   // aligne 32 bits
   uint8_t  hpEqualizer;         //!< mode d'egalisation du HP
   uint8_t  mixMode;             //!< mode de mixage
   int16_t  Gadc;                //!< Gain ADC en dB
   // aligne 32 bits
   int16_t  Gdac;                //!< Gain DAC en dB
   uint8_t  audioPath;           //!< Configuration du PATH audio du codec
   uint8_t  sidetone;            //!< niveau de sidetone en cours
   // aligne 32 bits
   uint8_t  sidetoneMode;        //!< mode de sidetone
   uint8_t  volume;              //!< volume en cours
   uint8_t  langue;              //!< langue en cours d'utilisation
   uint8_t  vocoderFrameTime;     //!< Input frame len in ms (10, 20, )
   // aligne 32 bits
   uint16_t vocoderEncodBitRate;  //!< Number of bit/s of the coded stream
   uint8_t  vocoderComplexity;    //!< Complexity of calculation during encodding and the decodding process (0-10 for OPUS)
   uint8_t  vocoderSignalType;    //!< Vocoder input type of signal (1 = OPUS_SIGNAL_VOICE, 2 = OPUS_SIGNAL_MUSIC, 0 = OPUS_AUTO)
   // aligne 32 bits
   uint32_t dbgDsp;    //!< variables pour debug
   // aligne 32 bits
   uint8_t  vocalSynthesisMode;  //<! mde de gestion de la synthese vocale
   uint8_t  audioAppMode;        //!< audio application mode : 0 normal, 1 duck, etc...
   uint16_t recorderOpt;			//!< Recorder options (See rec_opt_t)
   // aligne 32 bits
   uint8_t trace_level;			   //!< trace_level
   uint8_t audioMatrixMode;            //!< Used only by mco-audioApp (ESG)
   uint16_t audioMatrix;               //!< Used only by mco-audioApp (ESG)
   // aligne 32 bits
   uint16_t audioMatrixOnPioPushed;    //!< Used only by mco-audioApp (ESG)
   uint16_t audioMatrixOnPioReleased;  //!< Used only by mco-audioApp (ESG)
   // aligne 32 bits
   uint8_t pioBeepMode;
   uint8_t libre[7]; //!< le reste est libre pour plus tard
}protdspParam_t;

typedef struct _protdspVolumeParam_t
{  // ATTENTION A ALIGNER les 32 bits et les 16 bits
   // configuration volume et sidetone
   int16_t  volume;               //!< volume
   int16_t  sidetone;             //!< sidetone
   uint8_t  libre[16];           //!< le reste est libre pour plus tard
}protdspVolumeParam_t;

typedef struct _uartDspBpStruct_t
{
   uint16_t bps;              //!< valeur du bp
   uint8_t appui;             //!< 0 = relache, sinon appui
   uint8_t appuiLongCpt;      //!< 0 = appui court, sinon duree d'appuis en Nx500ms
   uint8_t detectionDoubleClic; //!< 1 = double clic
   uint8_t keyboardLock;      //!< 1 keyboard is locked
   uint8_t libre[16];
}uartDspBpStruct_t;

// ESG PA
typedef enum
{
   // mode de gestion du pio beepMode
   COMMPAR_PIO_BEEP_OFF = 0,
   COMMPAR_PIO_BEEP_ON = 1,
   COMMPAR_PIO_VOCAL_SYNTHESIS_ON = 2,
   COMMPAR_PIO_BEEP_NB = 3
} COMM_PIO_BEEP_MODE_LIST;

// Cas particulier du bootloader DSP
//----------------------------------
#define PROTDSP_UART_BOOT_SERIAL_DEBIT    (19200)  //!< Debit UART en serial boot (Etage 1) en Hz
#define PROTDSP_UART_BOOT_FW2_DEBIT       (230400) //!< Debit UART du bootloader (Etage 2) en Hz
#define PROTDSP_UART_BOOT_TEST_WD         {0xBF}
#define PROTDSP_UART_BOOT_REP_SIZE        4     // taille de la reponse au caractere '@'
#define PROTDSP_UART_BOOT_TEST_WD_END     "BLDSP1:OK" // envoye en fin par le bootloader en plus de 0xBF+ x + x + x
#define PROTDSP_UART_BOOT_REP_SIZE_END    9     // taille de la reponse en fin d'envoi bootloader 1er etage

//------------------------------- Warning --------------------------------------

//------------------------------ Variables -------------------------------------

//------------------------------- Macros ---------------------------------------

//----------------------------- Prototypes -------------------------------------

#endif //_PROTOCOLE_DSP_H_
