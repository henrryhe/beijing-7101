#ifndef __AUDIOREG_H
#define __AUDIOREG_H

/*************************************************************************
  File Name : audio_reg.h
  Description: definition of the register address and bit field
  ST Microelectronics Copyright 1999

 Date               Modification                      Name
----               ------------                       ----
 ??                Creation
 26/07             Merge from @@\main\prj-vali7020:   LJ
                    - new REGISTER_BASE
 27/04/01          Use new base address system        XD             - new 7015 registers
**************************************************************************/

/*-----------------7/26/00 2:28PM-------------------
 * STi70xx particularities :
 * --------------------------------------------------*/
#if (defined ST_7015 && !defined STI7015)
#define STI7015
#endif

#if (defined ST_7020 && !defined STI7020)
#define STI7020
#endif

#if defined(STI7015) || defined(STI7020)
	#include "stdevice.h"
  	#include "dumpreg.h"
#endif /* #ifdef STI7015 */

#ifdef __cplusplus
extern "C" {
#endif

#define FILENAME_AUDIO_MAX 128

#ifdef STI7015
/*	#define GAMMA_GFX1_ACTIVE
	#define GAMMA_GFX1_ACTIVE	0*/
#endif	/* AUDIO_7015 */

#ifdef STI7020
#endif	/* AUDIO_7020 */

/*-----------------7/26/00 2:28PM-------------------
 * definition of _REGISTERS_BASE
 * used by HALAUD_SetAudioReg() & HALAUD_GetAudioReg()
 * --------------------------------------------------*/

/*Just for debug : */
#ifdef MAP_7015_TO_RAM
  	#undef STI7015_BASE_ADDRESS
	#define STI7015_BASE_ADDRESS	0x51000000
#endif
#if defined (ST_7015)
#define AUD1   (STI7015_BASE_ADDRESS+ST7015_AUD1_OFFSET)            /* AUDIO 0 block base address */
#define AUD2   (STI7015_BASE_ADDRESS+ST7015_AUD2_OFFSET)     		/* AUDIO 1 block base address */
#define AUDPCM (STI7015_BASE_ADDRESS+ST7015_AUDPCM_OFFSET)     		/* AUDIO PCM block base address */
#define MMDSPBASE  (STI7015_BASE_ADDRESS+ST7015_MMDSP_OFFSET)           /* MMDSP block base address */
#define AUDIO_REGISTERS_BASE (STI7015_BASE_ADDRESS+ST7015_MMDSP_OFFSET) /* Register Base used by*/
#else
#define AUD1   (STI7020_BASE_ADDRESS+ST7020_AUD1_OFFSET)            /* AUDIO 0 block base address */
#define AUD2   (STI7020_BASE_ADDRESS+ST7020_AUD2_OFFSET)            /* AUDIO 1 block base address */
#define AUDPCM (STI7020_BASE_ADDRESS+ST7020_AUDPCM_OFFSET)          /* AUDIO PCM block base address */
#define MMDSPBASE  (STI7020_BASE_ADDRESS+ST7020_MMDSP_OFFSET)           /* MMDSP block base address */
#define AUDIO_REGISTERS_BASE (STI7020_BASE_ADDRESS+ST7020_MMDSP_OFFSET) /* Register Base used by*/
#endif

#ifdef EXTERNAL_MMDSP
	#define EXT_MMDSP_REGISTERS_BASE     0x70600000
#endif

/*-----------------2/1/00 5:18PM--------------------
 * Modification done by LC
 * --------------------------------------------------*/
#define Video_Base_Address  0x00000000
#define MASK_BIT_0  0x01

/*-----------------7/26/00 2:33PM-------------------
 *  STI7015 : Register offsets
 * --------------------------------------------------*/
#define AUD_SRS         0x90                /* Decoding Soft Reset */
#if defined STI7015 | defined STI7020
#define AUD_BUF_MASK     0x03ffff00   		/* Address on 18 bits   */
#define AUD_BUF_PCM_MASK 0x03ffff80   		/* Address on 19 bits   */

/*-----------------7/31/00 11:56AM------------------
 * Audio Bit buffer and PES parser registers
 * --------------------------------------------------*/
#define AUD_BBG         0x40                /* Audio bit buffer Begin */
#define AUD_BBS         0x44                /* Audio bit buffer Stop */
#define AUD_BBT         0x48                /* Audio bit buffer Threshold */
#define AUD_BBL         0x4c                /* Audio bit buffer Level */
#define AUD_STP         0x78                /* Audio Setup Complement */
#define AUD_CDI         0x7c                /* CD Input from host interface */
#define AUD_SRS         0x90                /* Decoding Soft Reset */
#define AUD_ITM         0x98                /* Interrupt mask */
#define AUD_ITS         0x9c                /* Interrupt status */
#define AUD_STA         0xa0                /* Status register */
#define AUD_PES_CFG     0xc0                /* PES Parser Configuration */
#define AUD_PES_SI      0xc4                /* PES Stream Identification */
#define AUD_PES_FSI     0xc8                /* Filter PES Stream ID */
#define AUD_PES_SSI     0xcc                /* PES SubStream Identification */
#define AUD_PES_FSSI    0xd0                /* Filter PES SubStream ID */

/*-----------------7/31/00 11:57AM-----------------*/
#define AUD_PLY         0x5c                /* Audio Play */
#define AUD_SETUP       0x58                /* Audio Setup */
#define CFG_AUD_IO      0xC60               /* */

#define AUD_PLY_ENABLE	 	0x01			/* Enable capture to DMA */
#define AUD_PLY_DISABLE  	0x00			/* Disable capture to DMA */
#define AUDPCM_PLY_PAUSE	0x01  			/* Enable capture to DMA */
#define AUDPCM_PLY_UNPAUSE 	0x00  			/* Disable capture to DMA */


/*-----------------7/31/00 11:57AM------------------
 * PCM Out Frequency Synthesizer
 * --------------------------------------------------*/
#define CKG_PCMO_SDIV   0xC64               /* PCM Out Frequency Synthesizer divider programmation */
#define CKG_PCMO_PE     0xC68               /* PCM Out Frequency Synthesizer fine selector programmation */
#define CKG_PCMO_PE_LSB CKG_PCMO_PE         /* CKG_PCM0_PE  LSB */
#define CKG_PCMO_PE_MSB ( CKG_PCMO_PE + 1 ) /* CKG_PCM0_PE  MSB */
#define CKG_PCMO_MD     0xC6C               /* PCM Out Frequency Synthesizer coarse selector programmation */
#define CKG_PCMO_PSEL   0xC70               /* PCM Out Frequency Synthesizer Parameter selection */
#define CKG_PCMO_CFG    0xC74               /* PCM Output Clock Generation Configuration */

/*-----------------7/31/00 11:57AM------------------
 * PCM player
 * --------------------------------------------------*/
#define AUD_PCM_CFG     0x58               /* PCM player module configuration */
#define AUD_PCM_ITM     0x98               /* PCM interrupt mask */
#define AUD_PCM_ITS     0x9C               /* PCM interrupt status */
#define AUD_PCM_STA     0xA0               /* PCM status register */
#define AUD_PCM_MOD     0x40               /* PCM player mode */
#define AUD_PCM_READ    0x54               /* PCM buffer read pointer */
#define AUD_PCM_PLY     0x5C               /* PCM play control */
#define AUD_PCM_NXT     0x50               /* PCM buffer next read pointer */
#define AUD_PCM_REF     0x48               /* PCM buffer reference pointer */
#define AUD_PCM_SRS     0x90               /* PCM player soft reset */
#define AUD_PCM_WRITE   0xA8               /* PCM player write threshold pointer */

#endif /* #if defined STI7015 | defined STI7020*/

#if defined STI7015
/* MMDSP registers for STI7015 */
/* 5508 8 bits registers mapped on LSB of 32 bits registers  */
#define AUD_VERSION   		/*0x00*/ 0x0000
#define AUD_IDENT     		/*0x01*/ 0x0004
#define AUD_FBADRL    		/*0x02*/ 0x0008
#define AUD_FBADRH    		/*0x03*/ 0x000C
#define AUD_FBDATA    		/*0x04*/ 0x0010
#define AUD_SFREQ     		/*0x05*/ 0x0014
#define AUD_EMPH      		/*0x06*/ 0x0018
#define AUD_INTEL     		/*0x07*/ 0x001C
#define AUD_INTEH     		/*0x08*/ 0x0020
#define AUD_INTL      		/*0x09*/ 0x0024
#define AUD_INTH      		/*0x0A*/ 0x0028
#define AUD_SETINT    		/*0x0B*/ 0x002C
#define AUD_SIN_SETUP 		/*0x0C*/ 0x0030
#define AUD_CAN_SETUP 		/*0x0D*/ 0x0034
#define AUD_DATAIN    		/*0x0E*/ 0x0038
#define AUD_ERROR     		/*0x0F*/ 0x003C
#define AUD_SOFTRESET 		/*0x10*/ 0x0040
#define AUD_PLLSYS    		/*0x11*/ 0x0044
#define AUD_PLLPCM    		/*0x12*/ 0x0048
#define AUD_PLAY      		/*0x13*/ 0x004C
#define AUD_MUTE      		/*0x14*/ 0x0050
#define AUD_REQ       		/*0x15*/ 0x0054
#define AUD_ACK       		/*0x16*/ 0x0058
#define AUD_TESTRAM   		/*0x17*/ 0x005C
#define AUD_PLLMASK   		/*0x18*/ 0x0060

#define AUD_CLRBREAK  		/*0x1E*/ 0x0078
#define AUD_DEBUGRAMADR		/*0x20*/ 0x0080

#define AUD_SYNC_STATUS   	/*0x40*/ 0x0100
#define AUD_ANCCOUNT      	/*0x41*/ 0x0104
#define AUD_HEAD1         	/*0x42*/ 0x0108
#define AUD_HEAD0         	/*0x43*/ 0x010C
#define AUD_HEADLENH      	/*0x44*/ 0x0110
#define AUD_HEADLENL      	/*0x45*/ 0x0114
#define AUD_PTS4          	/*0x46*/ 0x0118
#define AUD_PTS3          	/*0x47*/ 0x011C
#define AUD_PTS2          	/*0x48*/ 0x0120
#define AUD_PTS1          	/*0x49*/ 0x0124
#define AUD_PTS0          	/*0x4A*/ 0x0128

#define AUD_STREAMSEL     	/*0x4C*/ 0x0130
#define AUD_DECODESEL     	/*0x4D*/ 0x0134

#define AUD_VOLUME0       	/*0x4E*/ 0x0138

#define AUD_PACKET_LOCK   	/*0x4F*/ 0x013C
#define AUD_AUDIO_ID_EN   	/*0x50*/ 0x0140
#define AUD_AUDIO_ID      	/*0x51*/ 0x0144
#define AUD_AUDIO_ID_EXT  	/*0x52*/ 0x0148
#define AUD_SYNC_LOCK     	/*0x53*/ 0x014C
#define AUD_PCMDIVIDER    	/*0x54*/ 0x0150
#define AUD_PCMCONF       	/*0x55*/ 0x0154
#define AUD_PCMCROSS      	/*0x56*/ 0x0158
#define AUD_LDLY          	/*0x57*/ 0x015C
#define AUD_RDLY          	/*0x58*/ 0x0160
#define AUD_CDLY          	/*0x59*/ 0x0164
#define AUD_SUBDLY        	/*0x5A*/ 0x0168
#define AUD_LSDLY         	/*0x5B*/ 0x016C
#define AUD_RSDLY         	/*0x5C*/ 0x0170
#define AUD_DLYUPDATE     	/*0x5D*/ 0x0174

#define AUD_SPDIF_CMD       /*0x5E*/ 0x0178
#define AUD_SPDIF_CAT       /*0x5F*/ 0x017C
#define AUD_SPDIF_CONF      /*0x60*/ 0x0180
#define AUD_SPDIF_STATUS    /*0x61*/ 0x0184
#define AUD_PDEC            /*0x62*/ 0x0188

#define AUD_VOLUME1         /*0x63*/ 0x018C

#define AUD_PL_AB           /*0x64*/ 0x0190
#define AUD_PL_DWNX         /*0x65*/ 0x0194
#define AUD_OCFG            /*0x66*/ 0x0198
#define AUD_CHAN_IDX        /*0x67*/ 0x019C
#define AUD_PCM_BTONE       /*0x68*/ 0x01A0
#define AUD_AC3_DECODE_LFE  /*0x68*/ 0x01A0
#define AUD_MP_SKIP_LFE     /*0x68*/ 0x01A0

#define AUD_AC3_COMP_MOD    /*0x69*/ 0x01A4
#define AUD_MP_PROG_NUMBER  /*0x69*/ 0x01A4

#define AUD_AC3_HDR         /*0x6A*/ 0x01A8
#define AUD_MP_DRC          /*0x6A*/ 0x01A8

#define AUD_AC3_LDR         /*0x6B*/ 0x01AC

#define AUD_AC3_RPC         /*0x6C*/ 0x01B0
#define AUD_MP_CRC_OFF      /*0x6C*/ 0x01B0

#define AUD_AC3_KARAMODE    /*0x6D*/ 0x01B4
#define AUD_MP_MC_OFF       /*0x6D*/ 0x01B4

#define AUD_AC3_DUALMODE    /*0x6E*/ 0x01B8
#define AUD_MP_DUALMODE     /*0x6E*/ 0x01B8

#define AUD_AC3_DOWNMIX     /*0x6F*/ 0x01BC
#define AUD_MP_DOWNMIX      /*0x6F*/ 0x01BC
#define AUD_PN_DOWNMIX      /*0x6F*/ 0x01BC

#define AUD_DA_LPCM_DOWNMIX /*0x6F*/ 0x01BC
#define AUD_MLP_DOWNMIX     /*0x6F*/ 0x01BC
#define AUD_DTS_DOWNMIX     /*0x6F*/ 0x01BC

#define AUD_DWSMODE         /*0x70*/ 0x01C0
#define AUD_SOFTVER         /*0x71*/ 0x01C4
#define AUD_RUN             /*0x72*/ 0x01C8

#define AUD_SKIP_MUTE_CMD   /*0x73*/ 0x01CC
#define AUD_SKIP_MUTE_VALUE /*0x74*/ 0x01D0
#define AUD_SPDIF_REP_TIME  /*0x75*/ 0x01D4

#define AUD_AC3_STATUS0     /*0x76*/ 0x01D8
#define AUD_MP_STATUS0      /*0x76*/ 0x01D8
#define AUD_DTS_STATUS0     /*0x76*/ 0x01D8
#define AUD_LPCM_STATUS0    /*0x76*/ 0x01D8

#define AUD_AC3_STATUS1     /*0x77*/ 0x01DC
#define AUD_MP_STATUS1      /*0x77*/ 0x01DC
#define AUD_DTS_STATUS1     /*0x77*/ 0x01DC
#define AUD_LPCM_STATUS1    /*0x77*/ 0x01DC

#define AUD_AC3_STATUS2     /*0x78*/ 0x01E0
#define AUD_MP_STATUS2      /*0x78*/ 0x01E0
#define AUD_DTS_STATUS2     /*0x78*/ 0x01E0

#define AUD_LPCM_STATUS2    /*0x78*/ 0x01E0

#define AUD_AC3_STATUS3     /*0x79*/ 0x01E4
#define AUD_MP_STATUS3      /*0x79*/ 0x01E4
#define AUD_DTS_STATUS3     /*0x79*/ 0x01E4

#define AUD_AC3_STATUS4     /*0x7A*/ 0x01E8
#define AUD_MP_STATUS4      /*0x7A*/ 0x01E8

#define AUD_AC3_STATUS5     /*0x7B*/ 0x01EC
#define AUD_MP_STATUS5      /*0x7B*/ 0x01EC

#define AUD_AC3_STATUS6     /*0x7C*/ 0x01F0
#define AUD_MP_STATUS6      /*0x7C*/ 0x01F0

#define AUD_AC3_STATUS7     /*0x7D*/ 0x01F4

#define AUD_SPDIF_LATENCY   /*0x7E*/ 0x01F8
#define AUD_SPDIF_DTDI      /*0x7F*/ 0x01FC

#define AUD_TM_SPEED        /*0x57*/ 0x015C

#define AUD_MIX_LEVEL       /*0x80*/ 0x0200
#define AUD_SRC_LSB         /*0x81*/ 0x0204
#define AUD_SRC_MSB         /*0x82*/ 0x0208
#define AUD_SRC_HANDSHAKE   /*0x83*/ 0x020C
#define AUD_MIX_UPDATE      /*0x84*/ 0x0210
#define AUD_INPUT2_PLAY     /*0x85*/ 0x0214
#define AUD_INPUT2_CFG      /*0x86*/ 0x0218
#define AUD_INPUT2_MODE     /*0x87*/ 0x021C
#define AUD_SFREQ2          /*0x89*/ 0x0224
#define AUD_LVCR_DLY        /*0x8A*/ 0x0228
#define AUD_RVCR_DLY        /*0x8B*/ 0x022C
#endif  /* #if defined STI7015 */

#if defined STI7020
/* MMDSP+ registers for STI7020 */
/* 5508 8 bits registers mapped on LSB of 32 bits registers  */
#define AUD_VERSION   			/*0x00*/ 0x0000
#define AUD_IDENT     			/*0x01*/ 0x0004
#define AUD_FBADRL    			/*0x02*/ 0x0008
#define AUD_FBADRH    			/*0x03*/ 0x000C
#define AUD_FBDATA    			/*0x04*/ 0x0010
#define AUD_SFREQ     			/*0x05*/ 0x0014
#define AUD_INTEL     			/*0x07*/ 0x001C
#define AUD_INTEH     			/*0x08*/ 0x0020
#define AUD_INTL      			/*0x09*/ 0x0024
#define AUD_INTH      			/*0x0A*/ 0x0028
#define AUD_SETINT    			/*0x0B*/ 0x002C
#define AUD_SIN_SETUP 			/*0x0C*/ 0x0030
#define AUD_CAN_SETUP 			/*0x0D*/ 0x0034
#define AUD_DATAIN    			/*0x0E*/ 0x0038
#define AUD_ERROR     			/*0x0F*/ 0x003C
#define AUD_SOFTRESET 			/*0x10*/ 0x0040
#define AUD_PLLSYS    			/*0x11*/ 0x0044
#define AUD_PLLPCM    			/*0x12*/ 0x0048
#define AUD_PLAY      			/*0x13*/ 0x004C
#define AUD_MUTE      			/*0x14*/ 0x0050
#define AUD_REQ       			/*0x15*/ 0x0054
#define AUD_ACK       			/*0x16*/ 0x0058
#define AUD_TESTRAM   			/*0x17*/ 0x005C
#define AUD_PLLMASK   			/*0x18*/ 0x0060

#define AUD_CLRBREAK  			/*0x1E*/ 0x0078
#define AUD_DEBUGRAMADR			/*0x20*/ 0x0080

#define AUD_SYNC_STATUS   		/*0x40*/ 0x0100
#define AUD_ANCCOUNT      		/*0x41*/ 0x0104
#define AUD_HEAD1         		/*0x42*/ 0x0108
#define AUD_HEAD0         		/*0x43*/ 0x010C
#define AUD_HEADLENH      		/*0x44*/ 0x0110
#define AUD_HEADLENL      		/*0x45*/ 0x0114
#define AUD_PTS4          		/*0x46*/ 0x0118
#define AUD_PTS3          		/*0x47*/ 0x011C
#define AUD_PTS2          		/*0x48*/ 0x0120
#define AUD_PTS1          		/*0x49*/ 0x0124
#define AUD_PTS0          		/*0x4A*/ 0x0128

#define AUD_STREAMSEL     		/*0x4C*/ 0x0130
#define AUD_DECODESEL     		/*0x4D*/ 0x0134

#define AUD_VOLUME0       		/*0x4E*/ 0x0138

#define AUD_PACKET_LOCK   		/*0x4F*/ 0x013C
#define AUD_AUDIO_ID_EN   		/*0x50*/ 0x0140
#define AUD_AUDIO_ID      		/*0x51*/ 0x0144
#define AUD_AUDIO_ID_EXT  		/*0x52*/ 0x0148
#define AUD_SYNC_LOCK     		/*0x53*/ 0x014C
#define AUD_PCMDIVIDER    		/*0x54*/ 0x0150
#define AUD_PCMCONF       		/*0x55*/ 0x0154
#define AUD_PCMCROSS      		/*0x56*/ 0x0158
#define AUD_LDLY          		/*0x57*/ 0x015C
#define AUD_RDLY          		/*0x58*/ 0x0160
#define AUD_CDLY          		/*0x59*/ 0x0164
#define AUD_SUBDLY        		/*0x5A*/ 0x0168
#define AUD_LSDLY         		/*0x5B*/ 0x016C
#define AUD_RSDLY         		/*0x5C*/ 0x0170
#define AUD_DLYUPDATE     		/*0x5D*/ 0x0174

#define AUD_SPDIF_CMD       	/*0x5E*/ 0x0178
#define AUD_SPDIF_CAT       	/*0x5F*/ 0x017C
#define AUD_SPDIF_CONF      	/*0x60*/ 0x0180
#define AUD_SPDIF_STATUS    	/*0x61*/ 0x0184
#define AUD_PDEC            	/*0x62*/ 0x0188

#define AUD_VOLUME1         	/*0x63*/ 0x018C

#define AUD_PL_AB           	/*0x64*/ 0x0190
#define AUD_SRS_MODE        	/*0x64*/ 0x0190
#define AUD_VMAX_MODE       	/*0x64*/ 0x0190
#define AUD_PL_DWNX         	/*0x65*/ 0x0194
#define AUD_OCFG            	/*0x66*/ 0x0198
#define AUD_CHAN_IDX        	/*0x67*/ 0x019C
#define AUD_PCM_BTONE       	/*0x68*/ 0x01A0
#define AUD_AC3_DECODE_LFE  	/*0x68*/ 0x01A0
#define AUD_MP_SKIP_LFE     	/*0x68*/ 0x01A0

#define AUD_AC3_COMP_MOD    	/*0x69*/ 0x01A4
#define AUD_MP_PROG_NUMBER  	/*0x69*/ 0x01A4
#define AUD_DTS_1416BITS_MODE	/*0x69*/ 0x01A4
#define AUD_AAC_FORMAT	    	/*0x69*/ 0x01A4
#define AUD_BT_CHANNELCONF  	/*0x69*/ 0x01A4
#define AUD_PN_CHANNELCONF  	/*0x69*/ 0x01A4

#define AUD_AC3_HDR         	/*0x6A*/ 0x01A8
#define AUD_MP_DRC          	/*0x6A*/ 0x01A8

#define AUD_AC3_LDR         	/*0x6B*/ 0x01AC

#define AUD_AC3_RPC         	/*0x6C*/ 0x01B0
#define AUD_MP_CRC_OFF      	/*0x6C*/ 0x01B0
#define AUD_AAC_CRC_OFF     	/*0x6C*/ 0x01B0
#define AUD_CRC_OFF      		/*0x6C*/ 0x01B0

#define AUD_AC3_KARAMODE    	/*0x6D*/ 0x01B4
#define AUD_MP_MC_OFF       	/*0x6D*/ 0x01B4
#define AUD_AAC_MIX	        	/*0x6D*/ 0x01B4

#define AUD_AC3_DUALMODE    	/*0x6E*/ 0x01B8
#define AUD_MP_DUALMODE     	/*0x6E*/ 0x01B8
#define AUD_DUALMODE    		/*0x6E*/ 0x01B8

#define AUD_AC3_DOWNMIX     	/*0x6F*/ 0x01BC
#define AUD_MP_DOWNMIX      	/*0x6F*/ 0x01BC
#define AUD_DA_LPCM_DOWNMIX 	/*0x6F*/ 0x01BC
#define AUD_DOWNMIX      		/*0x6F*/ 0x01BC

#define AUD_DWSMODE         	/*0x70*/ 0x01C0
#define AUD_LPCM_DOWNSAMPLING   /*0x70*/ 0x01C0
#define AUD_SOFTVER         	/*0x71*/ 0x01C4
#define AUD_RUN             	/*0x72*/ 0x01C8
#define AUD_SKIP_MUTE_CMD   	/*0x73*/ 0x01CC
#define AUD_SKIP_MUTE_VALUE 	/*0x74*/ 0x01D0
#define AUD_SPDIF_REP_TIME  	/*0x75*/ 0x01D4

#define AUD_AC3_STATUS0     	/*0x76*/ 0x01D8
#define AUD_MP_STATUS0      	/*0x76*/ 0x01D8
#define AUD_DTS_STATUS0     	/*0x76*/ 0x01D8
#define AUD_AAC_STATUS0     	/*0x76*/ 0x01D8

#define AUD_AC3_STATUS1     	/*0x77*/ 0x01DC
#define AUD_MP_STATUS1      	/*0x77*/ 0x01DC
#define AUD_DTS_STATUS1     	/*0x77*/ 0x01DC
#define AUD_ACC_STATUS1     	/*0x77*/ 0x01DC

#define AUD_AC3_STATUS2     	/*0x78*/ 0x01E0
#define AUD_MP_STATUS2      	/*0x78*/ 0x01E0
#define AUD_DTS_STATUS2     	/*0x78*/ 0x01E0
#define AUD_ACC_STATUS2     	/*0x78*/ 0x01E0

#define AUD_AC3_STATUS3     	/*0x79*/ 0x01E4
#define AUD_MP_STATUS3      	/*0x79*/ 0x01E4
#define AUD_DTS_STATUS3     	/*0x79*/ 0x01E4
#define AUD_AAC_STATUS3     	/*0x79*/ 0x01E4

#define AUD_AC3_STATUS4     	/*0x7A*/ 0x01E8
#define AUD_MP_STATUS4      	/*0x7A*/ 0x01E8
#define AUD_AAC_STATUS4     	/*0x7A*/ 0x01E8

#define AUD_AC3_STATUS5     	/*0x7B*/ 0x01EC
#define AUD_MP_STATUS5      	/*0x7B*/ 0x01EC
#define AUD_AAC_STATUS5     	/*0x7B*/ 0x01EC

#define AUD_AC3_STATUS6     	/*0x7C*/ 0x01F0
#define AUD_MP_STATUS6      	/*0x7C*/ 0x01F0

#define AUD_AC3_STATUS7     	/*0x7D*/ 0x01F4

#define AUD_SPDIF_LATENCY   	/*0x7E*/ 0x01F8
#define AUD_SPDIF_DTDI      	/*0x7F*/ 0x01FC

#define AUD_GAIN_L_INPUT2		/*0x88*/ 0x0220
#define AUD_GAIN_R_INPUT2       /*0x89*/ 0x0224

#define AUD_SFREQ2          	/*0x94*/ 0x0250
#define AUD_INPUT2_MODE     	/*0x95*/ 0x0254
#define AUD_LPCM_PH_COEF_L      /*0x96*/ 0x0258
#define AUD_LPCM_PH_COEF_R      /*0x97*/ 0x025C
#define AUD_LPCM_G_COEFF_LF_L   /*0x98*/ 0x0260
#define AUD_LPCM_G_COEFF_LF_R   /*0x99*/ 0x0264
#define AUD_LPCM_G_COEFF_RF_L   /*0x9A*/ 0x0268
#define AUD_LPCM_G_COEFF_RF_R   /*0x9B*/ 0x026c
#define AUD_LPCM_G_COEFF_C_L    /*0x9C*/ 0x0270
#define AUD_LPCM_G_COEFF_C_R    /*0x9D*/ 0x0274
#define AUD_LPCM_G_COEFF_LS_L   /*0x9E*/ 0x0278
#define AUD_LPCM_G_COEFF_LS_R   /*0x9F*/ 0x027C
#define AUD_LPCM_G_COEFF_RS_L   /*0xA0*/ 0x0280
#define AUD_LPCM_G_COEFF_RS_R   /*0xA1*/ 0x0284
#define AUD_LPCM_G_COEFF_LFE_L  /*0xA2*/ 0x0288
#define AUD_LPCM_G_COEFF_LFE_R  /*0xA3*/ 0x028C

#define AUD_DV_LPCM_CH_ASSIGN	/*0xA8*/ 0x02A0
#define AUD_DV_LPCM_MC          /*0xA9*/ 0x02A4

#define AUD_PCMMIX_USERSETUP    /*0xAB*/ 0x02AC
#define AUD_PCMMIX_USERSETUP2   /*0xAC*/ 0x02B0
#define AUD_INPUT2_PLAY     	/*0xAD*/ 0x02B4
#define AUD_VCR_OUTPUT      	/*0xAE*/ 0x02B8
#define AUD_LVCR_DLY        	/*0xAF*/ 0x02BC
#define AUD_RVCR_DLY        	/*0xB0*/ 0x02C0
#define AUD_INPUT2_CFG      	/*0xB1*/ 0x02C4
#define AUD_MIX_UPDATE      	/*0xB2*/ 0x02C8
#define AUD_MIX_INPUTVOL1   	/*0xB3*/ 0x02CC
#define AUD_EMPH      			/*0xB5*/ 0x02D4
#define AUD_SRC_HANDSHAKE   	/*0xB7*/ 0x02DC
#define AUD_SRC_MSB     		/*0xB8*/ 0x02E0
#define AUD_SRC_LSB     		/*0xB9*/ 0x02E4
#define AUD_MIX_LEVEL       	/*0xBA*/ 0x02E8

#define AUD_SHIFT_INPUT2        /*0xFC*/ 0x03F0
#endif  /* #if defined EXTERNAL_MMDSP */

#endif /* #ifndef __AUDIOREG_H */

#ifdef __cplusplus
}
#endif


