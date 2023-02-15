/*
 ============================================================================
 Name        : avxDefs.h
 Author      : Remi Peuvergne
 Version     :
 Copyright   : Closed
 Description : Definitions for the Auvitran Rack
 ============================================================================
 */
#ifndef __AVX_DEFS_H__
#define __AVX_DEFS_H__

#define AVX_SPI_LOW_SPEED   	100000  // 100 kHz (startup speed)
#define AVX_SPI_HIGH_SPEED	4000000 // 4 MHz

#define PAGE_0		0
#define PAGE_1		1
#define PAGE_2		2
#define AXC_PAGE_SIZE	32

#define UNITARY_READ    0b11000000
#define UNITARY_WRITE   0b10000000
#define TEST_AND_RESET	0b01000000
#define TEST_AND_SET	0b00000000
#define OFFSET_MASK     0b00011111
#define OFFSET_LENGTH   5
#define MXPR_CHG_BIT	1

#define FIRST_SLOT	1
#define LAST_SLOT	7
#define MATRIX_SLOT	14
#define AVBX7_SLOT	15
#define MAX_SLOT_NUMBERS 16
#define SLOT_MASK	0b00001111

// Registers in page #0
#define REG_SR0	0x04
#define REG_SR1	0x05
#define REG_EVR	0x06
#define REG_EVMR	0x07
#define REG_MBX_CMD_0	0x08
#define REG_MBX_CMD_1	0x09
#define REG_MBX_ANS_0	0x0A
#define REG_MBX_ANS_1	0x0B
#define REG_MBX_ECS_0	0x0C
#define REG_MBX_ECS_1	0x0D
#define REG_MBX_MOV_0	0x0E
#define REG_MBX_MOV_1	0x0F
#define REG_BUS_ERR	0x11
#define REG_PAGE	0x1C

// Registers in page #1
#define REG_MIR	0x00
#define REG_PIR	0x01
#define REG_FIR	0x02
#define REG_EXT	0x03
#define REG_IPR	0x0A
#define REG_OPR	0x0B
#define REG_GPIOPR	0x0C
#define REG_CLKPR	0x0D
#define REG_CSPR	0x0E
#define REG_CNAPR	0x0F
#define REG_FPPR	0x10
#define REG_DMAPR	0x11
#define REG_MXPR	0x12

// Registers in page MXPR
#define REG_MXPR_ALLOW	0x08

// Registers in page FPPR
#define REG_FPPR_FSDR	0x05
#define REG_FPPR_FUCKR	0x0C
#define REG_FPPR_FBAR	0x10
#define REG_FPPR_FPR	0x14
#define REG_FPPR_FCR	0x17
#define REG_FPPR_FCAR	0x18
#define REG_FPPR_FCRCR	0x1C

// Registers in page CLKPR
#define REG_CLKPR_SRCAP 0x19
#define REG_CLKPR_CLKSR 0x05
#define REG_CLKPR_SRCFG 0x1A

// CLKPR-SRCAP flags
#define CLK_SRCAP_ASRC  (1 << 3)
#define CLK_SRCAP_x4    (1 << 2)
#define CLK_SRCAP_x2    (1 << 1)
#define CLK_SRCAP_BASE  (1 << 0)

// CLKPR-SRCAP constants
#define CLK_SRCAP_44_1_only 0
#define CLK_SRCAP_44_1_48   1

// CLKPR-SRCFG constants
#define CLK_SRCFG_48k	0
#define CLK_SRCFG_44_1k 1
#define CLK_SRCFG_96k	2
#define CLK_SRCFG_88_2k 3

// Registers in Dante-CSPR
#define DANTE_CSPR_CLK_STATUS	0x0C

// Flags in Dante-CSPR-CLK-STATUS
#define DANTE_CLK_PREFERRED_MASTER	(1 << 0)
#define DANTE_CLK_CLK_SRC		(1 << 2)
#define DANTE_CLK_INTERNAL		0
#define DANTE_CLK_EXTERNAL		1
#define DANTE_CLK_PREF_NO		0
#define DANTE_CLK_PREF_MASTER		1

#define SR1_BURST_MASK	(1 << 6)
#define SR1_FAST_MASK	(1 << 5)
#define SR1_MBOX_MASK	(1 << 4)

#define PAGE0_BIT	(1 << 5)

// Mailbox commands
#define MBX_TAS_PAGE_0		0
#define MBX_TAS_PAGED		1
#define MBX_TAR_PAGE_0		2
#define MBX_TAR_PAGED		3
#define MBX_WRITE_PAGE_0	4
#define MBX_WRITE_PAGED	5
#define MBX_READ_PAGED		6

// Mailbox flags
#define MBX_WP0	(1 << 7)
#define MBX_MOV	(1 << 6)

// Flash commands
#define FLASH_UNLOCK		0x04
#define FLASH_LOAD		0x05
#define FLASH_SAVE		0x06
#define FLASH_FACTORY		0x07
#define FLASH_RESET		0x08

// Flash flags
#define FLASH_UNLOCKED_FLAG	0x01
#define FLASH_FCR_IN_FCAR	24
#define FLASH_SAVE_NUMBER	8

// Manufacturer IDs
#define MID_TOOLBOX_AUVITRAN        0x01    // AuviTran Manufacturer
#define MID_EXT_AUVITRAN            0x03    // AuviTran Manufacturer

// Product IDs
#define PID_AxC_ES100               0x01    // AuviTran AudioToolBox eXtension Card: ES100 EtherSound interface
#define PID_AxC_GP16IO              0x02    // AuviTran AudioToolBox eXtension Card: 16 GPIO with EuroBloc connectors
#define PID_AxC_AX4I                0x03    // AuviTran AudioToolBox eXtension Card:  4 Analog inputs with XLR connectors
#define PID_AxC_AX40                0x04    // AuviTran AudioToolBox eXtension Card:  4 Analog outputs with XLR connectors
#define PID_AxC_AE8IO               0x05    // AuviTran AudioToolBox eXtension Card:  8 Analog I/O (4 inputs + 4 outputs) with Eurobloc connectors
#define PID_AxC_AX4M                0x06    // AuviTran AudioToolBox eXtension Card:  4 Microphone inputs with XLR connectors
#define PID_AxC_DX8I                0x07    // AuviTran AudioToolBox eXtension Card: 8 digital (4xAES/EBU) inputs with XLR connectors
#define PID_AxC_DX8O                0x08    // AuviTran AudioToolBox eXtension Card: 8 digital (4xAES/EBU) outputs with XLR connectors
#define PID_AxC_DS32IO              0x09    // AuviTran AudioToolBox eXtension Card: 16 digital I/O (2x8xAES/EBU) inputs with Sub-D connectors
#define PID_AxC_MADI                0x0A    // AuviTran AudioToolBox eXtension Card: MADI interface
#define PID_AxC_ADAT                0x0B    // AuviTran Audiotoolbox eXtension Card: ADAT interface
#define PID_AxC_DANTE               0x0C    // AuviTran AudioToolBox eXtension Card: DANTE interface
#define PID_AxC_CN32IO              0x0D    // AuviTran AudioToolBox  Internal Card: CobraNet interface
#define PID_AxC_SWD5G               0x0F    // AuviTran Audiotoolbox eXtension Card: Double Switch 5G interface
#define PID_AxC_ASRC                0x10    // AuviTran Audiotoolbox eXtension Card: Asynchronous SRC 128 channels interface
#define PID_AxC_VOGO                0x23    // AuviTran Audiotoolbox rack
#define PID_AiC_GPIO                0x81    // AuviTran GPIO
#define PID_AiC_CORE_MINI_AVBx3     0x82    // AuviTran Audiotoolbox Internal Card: Core Mini for AVBx3
#define PID_AiC_CORE_MAXI_AVBx7     0x83    // AuviTran Audiotoolbox Internal Card: Core Maxi for AVBx7 or AVBx3M
#define PID_AVDT_BOB_AE4IO          0x1     // AuviTran AVDT-BreakOut Box
#define PID_AVDT_BOB_AE8IO          0x2     // AuviTran AVDT-BreakOut Box + daughter board 4IO
#define PID_AVDT_BOB_AS8IO          0x3     // AuviTran AVDT-BreakOut Box + daughter board SFP + Neutrik
#define PID_AVDT_BOB_ADE8IO         0x4     // AuviTran AVDT-BreakOut Box + daughter board AESIn/Out
#define PID_AVDT_BOB_ADX8IO         0x5     // AuviTran Dante Breakout Box with SFP + Neutrik EtherCon + POE + 2 mic/line + 2 Analog out + 1 AES In + 1 AES out
#define PID_AVDT_BOB_DS8IO          0x6     // TODO VIRIFIED!!!!!  AuviTran Dante Breakout Box with 4IO MotherBoard + AES DautherBoard --> 4IO + (2 AES + 2  ) + Neutrik

// Registers in AxC-AX4I
#define REG_AX4I_GAIN_BASE	8
#define REG_AX4I_PAD		16
#define REG_AX4I_VU_BASE	12

// Registers in AxC-AX4O
#define REF_AX4O_GAIN_BASE	12
#define REG_AX4O_PAD		17
#define REG_AX4O_VU_BASE	12

// Registers in AxC-AX4M
#define REG_AX4M_GAIN_BASE	8
#define REG_AX4M_PARAM_BASE	12
#define REG_AX4M_VU_BASE	12

// Register in AxC-DANTE CNAPR
#define REG_CNAPR_CARD_NAME 0x4
#define REG_CNAPR_INPUT0_NAME 0x20
#define REG_CNAPR_OUTPUT0_NAME (REG_CNAPR_INPUT0_NAME+(0x20*64))
#define REG_CNAPR_VERSION 0x1

// Parameters for all cards and conversion macros
#define GAIN_MUTE		0
#define GAIN_MIN_AX4I_O		-115.0
#define GAIN_MAX_AX4I_O		12.0
#define DB_FLOAT_TO_INT_AX4I_O(x)	((115.5 + x) * 2)
#define DB_INT_TO_FLOAT_AX4I_O(x)	(-115.5 + x * 0.5)
#define GAIN_MIN_AX4M		-24.0
#define GAIN_MAX_AX4M		55.0
#define DB_FLOAT_TO_INT_AX4M(x)	((24.5 + x) * 2)
#define DB_INT_TO_FLOAT_AX4M(x)	(-24.5 + x * 0.5)
#define PAD_BITFIELD_10DBU	0b01
#define PAD_BITFIELD_24DBU	0b11
#define PAD_BITFIELD_MASK	0b11
#define MAX_CHANNELS_NB	4
#define MATRIX_SLOT_ENABLE	(1 << 7)

// Registers in AxC-VOGO CSPR page
#define REG_VOGO_CSPR_FACTORY_RESET 0x08

typedef struct{
    union{
        uint8_t all_reg;
        struct{
            uint8_t bit_factory_reset   : 1;
            uint8_t FOR_FUTUR_USE       : 7;
        };
    };
}reg_vogo_CSPR_factory_reset_t;

#endif //__AVX_DEFS_H__
