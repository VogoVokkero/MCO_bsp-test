//==============================================================================
//! \addtogroup common PARAMETRES COMMUNS
//! \{
//!
//! \file   commParam.h
//! \brief  definition de tous les element communs au plusieurs logiciels : STM, DSP, PC, TEST, ...
//!
//! Historique :
//!   - 02/12/2015 : Creation par Adeunis RF (SFBC)
//!
//! \}
//==============================================================================
#ifndef _COMMPARAM_H_
#define _COMMPARAM_H_

#pragma pack(1)


//------------------------- Options de compilation -----------------------------

//------------------------ Definitions/Constantes ------------------------------
#ifdef GUARDIAN_SHOW
#define VERSION_MAJ_DSP_STM         "03" //!< version majeur partage avec le STM : pour test de compatibilite
#else //ELITE
#define VERSION_MAJ_DSP_STM         "01" //!< version majeur partage avec le STM : pour test de compatibilite
#endif //ELITE

#define COMMPAR_MIN_USERID              (0)
#define COMMPAR_MAX_USERID              (15)
#define COMMPAR_MAX_ID_NBR          (COMMPAR_MAX_USERID+1)
#define COMMPAR_ID_DEF              (COMMPAR_MIN_USERID)

#define COMMPAR_OPUS16_AUDIO_SIZE   (20)  //!< Nombre d'octets par trame opus @16kHz
#define COMMPAR_OPUS24_AUDIO_SIZE   ((30%4)+30)  //!< Nombre d'octets par trame opus @24kHz     //%4  --> 32

//! taille des donnes MAX audio dans les trames de communication compressees
#define COMMPAR_AUDIO_DATA_SIZE_MAX (COMMPAR_OPUS24_AUDIO_SIZE)

#define COMMPAR_AUDIO_VOIX_MAX      (5)   //!< Nombre MAX de voix recues pouvant etre traitee simultannement
                                          //!< Attention a la correspondance avec la valeur defini dans le DSP
#define COMMPAR_AUDIO_VOIX_MAX_MIX (5)

typedef enum _COMMPAR_ROLE_LIST
{
   COMMPAR_ROLE_MIN = 0,
   COMMPAR_ROLE_ANY = COMMPAR_ROLE_MIN,      //!< Role listener : en ecoute, ne transmet jamais de trames audio ou MCO
   COMMPAR_ROLE_MASTER1,            //!< Role MASTER1 : numero de role du produit en charge de la synchronisation radio du system, peut etre attribue a une WI !!
   COMMPAR_ROLE_WI,                 //!< Role WI main : numero de role du produit en charge de la WI main
   COMMPAR_ROLE_TECH,               //!< Role TECHnicien : numero de role du produit en charge du MCO2, ne transmet jamais d'audio,
   COMMPAR_ROLE_TALKER,             //!< Role talker : peut parler et ecouter
   COMMPAR_ROLE_LISTENER,       //!< Role MCO Listener : en ecoute, ne transmet jamais d'audio, mais repond aux demandes MCO1 et MCO2
   COMMPAR_ROLE_WI_BACKUP,          //!< Role WI backup : numero de role du produit en charge de la WI backup
   COMMPAR_ROLE_NB,                 //!< Nombre de roles (ne pas confondre avec COMMPAR_AUDIO_VOIX_MAX)
}COMMPAR_ROLE_LIST;
#define COMMPAR_ROLE_DEF            (COMMPAR_ROLE_ANY)

//! Liste des modes d'AEC (Anti-echo)
typedef enum _COMMPAR_AEC_MODE_LIST
{
   COMMPAR_AECM_OFF = 0,         //!< AEC desactive
   COMMPAR_LPM1,                 //!< mode : priorité à l'écoute (simple duplex) (ex : SAX020) - fade out lent du volume
   COMMPAR_LPM2,                 //!< mode : priorité à l'écoute (simple duplex) (ex : SAX020) - fade out instantannée du volume pour éviter les échos
   COMMPAR_SPM,                  //!< mode : priorité à la parole (simple duplex)
   COMMPAR_AECM_ON,              //!< mode : AEC NLMS activé
}COMMPAR_AEC_MODE_LIST;
#define COMMPAR_AEC_MODE_DEF    (COMMPAR_AECM_OFF) //!< valeur par defaut

//! Liste des modes du filtrage de bruit
typedef enum _COMMPAR_RB_MODE_LIST
{
   COMMPAR_RBM_UNKNOWN = 0xFF,
   COMMPAR_RBM_OFF = 0,    //!< Noise off, Tone off
   COMMPAR_RBM_ON_VAD = 1, //!< Noise on, Tone off
   COMMPAR_RBM_ALWAYS_ON = 2, //!< Noise off, Tone on
   COMMPAR_RBM_OFF_DUCK_LOW = 10,    //!< Noise off, Tone off
   COMMPAR_RBM_ON_VAD_DUCK_LOW = 11, //!< Noise on, Tone off
   COMMPAR_RBM_ALWAYS_ON_DUCK_LOW = 12, //!< Noise off, Tone on
   COMMPAR_RBM_OFF_DUCK_HIGH = 20,    //!< Noise off, Tone off
   COMMPAR_RBM_ON_VAD_DUCK_HIGH = 21, //!< Noise on, Tone off
   COMMPAR_RBM_ALWAYS_ON_DUCK_HIGH = 22, //!< Noise off, Tone on
}COMMPAR_RB_MODE_LIST;
#define COMMPAR_RBM_DEF          (COMMPAR_RBM_ALWAYS_ON) //!< valeur par defaut

//! Liste des mode de fonctionnement du VAD (Voice analysis detector)
typedef enum _COMMPAR_VAD_MODE_LIST
{
   COMMPAR_VADM_UNKNOWN = 0xFF,
   COMMPAR_OFFM = 0,               //!< VAD desactive
   COMMPAR_GATEM = 1,          //!< GATE actif avec rétro-action
   COMMPAR_VADM = 2,            //!< VAD Actif en mode normal
   COMMPAR_OFFM_RETROACTIF = 10,      //!< GATE actif avec rétro-action
   COMMPAR_GATEM_RETROACTIF = 11,      //!< GATE actif avec rétro-action
   COMMPAR_VADM_RETROACTIF = 12,        //!< VAD actif et rétro-actif de ICFG_HISTORY_LEN_DEFAULT
   COMMPAR_OFFM_RETROACTIF_LONG = 20,      //!< GATE actif avec rétro-action
   COMMPAR_GATEM_RETROACTIF_LONG = 21,      //!< GATE actif avec rétro-action
   COMMPAR_VADM_RETROACTIF_LONG = 22,        //!< VAD actif et rétro-actif de ICFG_HISTORY_LEN_DEFAULT
   COMMPAR_DUCKM = 3,        //!< VAD actif et rétro-actif de ICFG_HISTORY_LEN_DEFAULT
}COMMPAR_VAD_MODE_LIST;
#define COMMPAR_VADM_DEF         (COMMPAR_GATEM) //!< valeur par defaut

//! Liste des mode anti-larsen (non implémenté @21/07/2016)
typedef enum _COMMPAR_LARSEN_MODE_LIST
{
   COMMPAR_LARSENM_OFF = 0,   //!< Anti-larsen desactive
   COMMPAR_LARSENM_ON_NO_ATTENUATION = 1,
   COMMPAR_LARSENM_ON_LOW_ATTENUATION = 2,
   COMMPAR_LARSENM_ON_HIGH_ATTENUATION = 3,
   COMMPAR_LARSENM_ON_TOTAL_ATTENUATION = 4,
}COMMPAR_LARSEN_MODE_LIST;
#define COMMPAR_LARSENM_DEF      (COMMPAR_LARSENM_OFF) //!< valeur par defaut

//! Liste des mode anti-burst
typedef enum _COMMPAR_BURST_MODE_LIST
{
   COMMPAR_BURSTM_OFF = 0,   //!< Anti-burst desactive
   COMMPAR_BURSTM_ON = 1,    //!< Anti-burst active
}COMMPAR_BURST_MODE_LIST;
#define COMMPAR_BURSTM_DEF       (COMMPAR_BURSTM_OFF) //!< valeur par defaut

//! Liste des mode de contrôle de voix lointaine (far-voice)
typedef enum _COMMPAR_FARVOICE_MODE_LIST
{
   COMMPAR_FARVOICEM_OFF = 0,        //!< Far-voice desactive - total sensitivity
   COMMPAR_FARVOICEM_ON_LOW = 1,
   COMMPAR_FARVOICEM_ON_MEDIUM = 2,
   COMMPAR_FARVOICEM_ON_HIGH = 3,
   COMMPAR_FARVOICEM_END = COMMPAR_FARVOICEM_ON_HIGH
}COMMPAR_FARVOICE_MODE_LIST;
#define COMMPAR_FARVOICEM_DEF       (COMMPAR_FARVOICEM_ON_LOW) //!< valeur par defaut
#define COMMPAR_BT_FARVOICEM_DEF    (COMMPAR_FARVOICEM_OFF)     //!< valeur par defaut accessoire Bluetooth

//! Liste des mode de CAG
//#define COMMPAR_CAGM_ON_NUM_MASK (1)
//#define COMMPAR_CAGM_ON_ANA_LIM_MASK (2)
//#define COMMPAR_CAGM_ON_ANA_ALC_MASK (4)
typedef enum _COMMPAR_CAG_MODE_LIST
{
   COMMPAR_CAGM_OFF = 0,            //!< CAG desactive
   COMMPAR_CAGM_ON_NUM_STANDARD = 1,
   COMMPAR_CAGM_ON_NUM_DUCK_VOICE = 2,
   COMMPAR_CAGM_ON_NUM_DUCK_BACKGROUND = 3,
}COMMPAR_CAG_MODE_LIST;
#define COMMPAR_CAGM_DEF         (COMMPAR_CAGM_ON_NUM_STANDARD) //!< valeur par defaut


//! Liste des modes d'égalisation
typedef enum _COMMPAR_EQ_MODE_LIST
{
   COMMPAR_EQM_OFF = 0,           //!< Egaliseur désactivé
   COMMPAR_EQM_PHO020 = 1,        //!< profil de correction pour PHO020
   COMMPAR_EQM_SEN010 = 2,        //!< profil de correction pour PHO020
   COMMPAR_EQM_CEO010 = 3,        //!< profil de correction pour CEO010
   COMMPAR_EQM_PEL010 = 4,        //!< profil de correction pour PEL010
   COMMPAR_EQM_PEL020 = 5,        //!< profil de correction pour PEL020
   COMMPAR_EQM_MAE020 = 6,        //!< profil de correction pour MAE020
   COMMPAR_EQM_KEN020 = 7,        //!< profil de correction pour KEN020
   COMMPAR_EQM_SAX020 = 8,        //!< profil de correction pour SAX020
   COMMPAR_EQM_QBW,               //!< profil de test 1/4 bande passante pour check
   COMMPAR_EQM_DEFAULT            //!< profil par défaut
}COMMPAR_EQ_MODE_LIST;
#define COMMPAR_EQM_DEF          (COMMPAR_EQM_OFF) //!< valeur par defaut

//! Liste des modes de mixage
typedef enum _COMMPAR_MIX_MODE_LIST
{
   COMMPAR_MIXM_BASIC_NOBLEND = 0,     //!< Mixeur RVT, default, duck voices with no priority
   COMMPAR_MIXM_BASIC_BLEND = 1,       //!< bips are blended with the current received speech speech
   COMMPAR_MIXM_DUCK_NOBLEND = 10,     //!< no blend, with agc on vad on role 1
   COMMPAR_MIXM_DUCK_BLEND = 11,       //!< bip blend, with agc on vad on role 1
   COMMPAR_MIXM_PRIORITY_NOBLEND = 20, //!< do not duck voices with no priority
   COMMPAR_MIXM_PRIORITY_BLEND = 21,   //!< idem and blends bips
}COMMPAR_MIX_MODE_LIST;
#define COMMPAR_MIXM_DEF    (COMMPAR_MIXM_BASIC_BLEND) //!< valeur par defaut


/* [RVT @16/09/2016] CONFIGURATION PLC */
//! Liste des Packet Loss concealer
typedef enum _COMMPAR_PLC_MODE_LIST
{
   COMMPAR_PLC_OFF = 0,       //!< PLC désactivé
   COMMPAR_PLC_ON = 1,          //!< PLC activé
} COMMPAR_PLC_MODE_LIST;
#define COMMPAR_PLC_DEF    (COMMPAR_PLC_OFF) //!< valeur par defaut

//! Liste des mode de sidetone
typedef enum _COMMPAR_SIDETONE_MODE_LIST
{
   COMMPAR_SIDETONEM_DIRECT_VAD = 0, //!< Sidetone dans le CODEC audio conditionn� par le VAD
   COMMPAR_SIDETONEM_DIRECT,     //!< Sidetone dans le CODEC audio non conditionn� au VAD
   COMMPAR_SIDETONEM_NUMERIC,    //!< Sidetone a travers le signal processing
   COMMPAR_SIDETONEM_DISABLED,   //!< Sidetone disabled
   COMMPAR_SIDETONEM_NB          //!< limit
} COMMPAR_SIDETONE_MODE_LIST;
#define COMMPAR_SIDETONEM_DEF    (COMMPAR_SIDETONEM_DISABLED) //!< valeur par defaut

//! Liste des modes de test audio
typedef enum _COMMPAR_AUDIOMODE_LIST
{
   COMMPAR_AUDIOMODE_NONE = 0,                        //!< Mode invalid
   COMMPAR_AUDIOMODE_8KHZ,                            //!< Mode audio 8kHz
   COMMPAR_AUDIOMODE_16KHZ,                           //!< Mode audio 16kHz
   COMMPAR_AUDIOMODE_24KHZ,                           //!< Mode audio 24kHz

   COMMPAR_AUDIOTEST_OFF                           = 30, //!< debut de la liste des modes de test
   COMMPAR_AUDIOTEST_ANALOG_LOOPBACK_MIC_TO_SPK    = 31, //!< analog loopback dans le codec : MIC in --> SPK out
   COMMPAR_AUDIOTEST_DIG_LOOPBACK                  = 32, //!< digital loopback : L'acquisition (In ou MIC) est rejoue sur la restitution (OUT ou HP)
   COMMPAR_AUDIOTEST_DIG_LOOPBACK_WITH_SP_MIC      = 33, //!< Idem COMMPAR_AUDIOTEST_DIG_LOOPBACK avec traitement spProcessMic
   COMMPAR_AUDIOTEST_DIG_LOOPBACK_WITH_SP_MIC_MIX  = 34, //!< Idem COMMPAR_AUDIOTEST_DIG_LOOPBACK_WITH_SP_MIC avec MIX HP
   COMMPAR_AUDIOTEST_DIG_LOOPBACK_WITH_CODER       = 35, //!< Idem COMMPAR_AUDIOTEST_DIG_LOOPBACK_WITH_SP_MIC avec compression + decompression (G729 ou autre)
   COMMPAR_AUDIOTEST_SWEEP                         = 36, //!< génération un sweep sur le HP
   COMMPAR_AUDIOTEST_RAMPUP                        = 37, //!< genere un rampe a l'emission du vocoder (VOCODER_G729_TEST), rejoue cette rampe a la reception
   COMMPAR_AUDIOTEST_TRACE_BURST                   = 38, //!< remplace le signal HP enregistre par la signature du burst
   COMMPAR_AUDIOTEST_PLC_SPI_ERR                   = 39, //!< genere des pertes audio toutes les 100ms
   COMMPAR_AUDIOTEST_WTIHOUT_CODER_END_OF_LIST,

   //--------------------------------
   // fin des test avec comm Radio

   // Fin de la list des tests
   //--------------------------------

   // Liste des test a 16kHz
   COMMPAR_AUDIOTEST_16KHZ_OFF = 50,                 //!< pour communication entre DSP et STM : si le parametre audio mode recu est > a COMMPAR_AUDIOTEST_16KHZ
                                                     //!< alors COMMPAR_AUDIOTEST_xxx = (audioMode recu - (COMMPAR_AUDIOTEST_16KHZ + COMMPAR_AUDIOTEST_OFF)
   // Liste des test a 24kHz
   COMMPAR_AUDIOTEST_24KHZ_OFF = 70,                 //!< pour communication entre DSP et STM : si le parametre audio mode recu est > a COMMPAR_AUDIOTEST_24KHZ
                                                     //!< alors COMMPAR_AUDIOTEST_xxx = (audioMode recu - (COMMPAR_AUDIOTEST_24KHZ + COMMPAR_AUDIOTEST_OFF)
}COMMPAR_AUDIOMODE_LIST;

#define COMMPAR_VOLUME_MIN    (1)      //!< volume min
#ifdef GUARDIAN_SHOW
// GUARDIAN
#define COMMPAR_VOLUME_MAX    (10)      //!< volume max
#define COMMPAR_VOLUME_DEF    (5)      //!< volume par defaut
#else
// SHOW
#define COMMPAR_VOLUME_MAX    (5)      //!< volume max
#define COMMPAR_VOLUME_DEF    (3)      //!< volume par defaut
#endif

#define COMMPAR_SIDETONE_0dB              (2)   //! sidetone ON 0dB sur sortie SPK
#define COMMPAR_SIDETONE_MOUT_M10dB       (1)   //! sidetone ON -10dB sur sortie MOUT + gain rebouclage
#define COMMPAR_SIDETONE_MOUT_0dB         (3)   //! sidetone ON 0dB sur sortie MOUT + gain rebouclage
#define COMMPAR_SIDETONE_MUTE             (0)   //!< sidetone en mute
#define COMMPAR_SIDETONE_DEF  (COMMPAR_SIDETONE_MUTE) //!< sidetone par defaut

#define COMMPAR_SIDETONE_LEVEL1 (COMMPAR_SIDETONE_MOUT_M10dB)  // renommage pour le configurateur
#define COMMPAR_SIDETONE_LEVEL2 (COMMPAR_SIDETONE_MOUT_0dB)
#define COMMPAR_SIDETONE_LEVEL3 (COMMPAR_SIDETONE_0dB)

#define COMMPAR_SIDETONE_VOL_MIN   (0)             //!< sidetone min
#define COMMPAR_SIDETONE_NUMERIC_VOL_MAX  (10)     //!< sidetone max

typedef enum _COMMPAR_AUDIOPATH
{
   COMMPAR_AUDIOPATH_NULL = 0,            //!< Audio invalid pour update gains in et out uniquement
   COMMPAR_AUDIOPATH_COMM,                //!< Config audio en communication filaire : MIC ON + HP ON / AUX_IN OFF + AUX_OUT OFF
   COMMPAR_AUDIOPATH_COMM_BT,             //!< Config audio en communication BT : MIC OFF + HP OFF / AUX_IN ON + AUX_OUT ON
   COMMPAR_AUDIOPATH_COMM_TEL,            //!< Config audio en communication reponse tel : MIC ON + HP ON / AUX_IN ON + AUX_OUT ON
} COMMPAR_AUDIOPATH_LIST;

#define COMMPAR_AES128_KEY_LEN      (16) //!< taille de la cle AES128
#define COMM_ENCRYPT_KEY_NONE       (0)  //!< pas d'encription

typedef enum _COMMPAR_SV_MODE
{
   // modes de geqstion de la synthese vocale
   COMMPAR_SV_M_UNKNOWN = 0xFF,
   COMMPAR_SV_M_ON = 0,         // Synthese vocale on
   COMMPAR_SV_M_BIP,            // Synthese vocale remplacee par un bip
   COMMPAR_SV_M_OFF,            // Synthese vocale off
   COMMPAR_SV_M_NB
} COMM_SV_MODE;
#define COMM_VOCAL_SYNTH_MODE_DEF      (COMMPAR_SV_M_ON)

typedef enum _COMMPAR_AUDIO_APP_MODE
{
   // Mode gestion audio
   COMMPAR_AUDIO_APP_M_NORMAL = 0,
   COMMPAR_AUDIO_APP_M_DUCK = 1, // DUCK mode, TX
   COMMPAR_AUDIO_APP_M_SHOW_PRIORITY = 2, // unused, could be used to set the PRIORITY TX mode for SHOW
   COMMPAR_AUDIO_APP_M_NB
} COMMPAR_AUDIO_APP_MODE;
#define COMM_AUDIO_APP_MODE_DEF      (COMMPAR_AUDIO_APP_M_NORMAL)

typedef enum
{//(0 is forbidden)
   vocal_feedback_1 = 1,
   vocal_feedback_2,
   vocal_feedback_3,
   vocal_feedback_4,
   vocal_feedback_5,
   vocal_feedback_level,
   vocal_feedback_max_vol,
   vocal_feedback_min_vol,
   vocal_feedback_low_battery,
   vocal_feedback_netloss,    //10
   vocal_feedback_rec_end,
   vocal_feedback_rec_start,
   vocal_feedback_licensed_channel,
   vocal_feedback_mic_on,
   vocal_feedback_mic_off,
	vocal_feedback_MLB_strike,				//MLB    //14
	vocal_feedback_MLB_ball,				//MLB
}selecao_vocal_feedback_t;

typedef enum{
	rec_opt_enable = 1<<0,						//Enable record
	rec_opt_unsync = 1<<1,						//Allow to record unsync mic stream
	rec_opt_mix_mic_out_hp_out = 1<<2,		//Record basic conference stream
	rec_opt_mic_in = 1<<3,						//Record microphone input stream (before audio processing)
	rec_opt_mic_out = 1<<4,						//Record microphone output stream (after audio processing)
	rec_opt_hp_out = 1<<5,						//Record radio mixed output streams (after audio processing)
	rec_opt_tst = 1<<6,					   	//Test record to test wav file for test playback (test bench)
}rec_opt_t;


//------------------------------- Warning --------------------------------------

//------------------------------ Variables -------------------------------------

//------------------------------- Macros ---------------------------------------

//----------------------------- Prototypes -------------------------------------




#endif //_COMMPARAM_H_
