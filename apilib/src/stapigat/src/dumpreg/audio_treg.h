/*****************************************************************************

File name   : audio_treg.h

Description : Audio register description for test

COPYRIGHT (C) ST-Microelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
01-Sep-99          Created                      FC
27-Avr-01          define as static array       XD

TODO:
-----
TODO: Complete interrupt registers masks (Nbr of interrupts ?)
TODO: Duplicate registers which are used in AUD2
TODO: Check AUD_SETUP: Only one bit ?

*****************************************************************************/

#ifndef __AUDIO_TREG_H
#define __AUDIO_TREG_H

/* Includes --------------------------------------------------------------- */

#include "dumpreg.h"
#include "audio_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */
/*char   *Name;                       Name */
/*U32    Base;                        Base address of the Array */
/*U32    Offset;                      Offset from Base address */
/*U32    RdMask;                      Mask for read operations */
/*U32    WrMask;                      Mask for write operations */
/*U32    RstVal;                      Reset state */
/*U32    Type;                        Register characteristics */
/*U32    Size;                   	  Circular or Array register size */

/* Exported Constants ----------------------------------------------------- */

/* Registers description */
static const Register_t AudioReg[] =
{
	/* Audio 1 --------------------------------------------------------- */
    {"AUD1_BBG",                      		/* Audio bit buffer Begin */
     AUD1,
     AUD_BBG,
     0x3ffff00,
     0x3ffff00,
     0,
     REG_32B,
     0
    },
    {"AUD1_BBS",                      		/* Audio bit buffer Stop */
     AUD1,
     AUD_BBS,
     0x3ffff00,
     0x3ffff00,
     0,
     REG_32B,
     0
    },
    {"AUD1_BBT",                      		/* Audio bit buffer Threshold */
     AUD1,
     AUD_BBT,
     0x3ffff00,
     0x3ffff00,
     0,
     REG_32B,
     0
    },
    {"AUD1_BBL",                      		/* Audio bit buffer Level */
     AUD1,
     AUD_BBL,
     0x3ffff00,
     0x0000000,
     0,
     REG_32B,
     0
    },
    {"AUD1_STP",                      		/* Audio Setup */
     AUD1,
     AUD_STP,
     0x1,
     0x1,
     0,
     REG_32B,
     0
    },
    {"AUD1_CDI",                      		/* CD Input from host interface */
     AUD1,
     AUD_CDI,
     0x0,
     0xffffffff,
     0,
     REG_32B,
     0
    },
    {"AUD1_SRS",                      		/* Decoding Soft Reset */
     AUD1,
     AUD_SRS,
     0x0,
     0x1,
     0,
     REG_32B | REG_EXCL_FROM_TEST,
     0
    },
    {"AUD1_ITM",                      		/* Interrupt mask */
     AUD1,
     AUD_ITM,
     0x3C00,
     0x3C00,
     0,
     REG_32B,
     0
    },
    {"AUD1_ITS",                      		/* Interrupt status */
     AUD1,
     AUD_ITS,
     0x3C00,
     0x0000,
     0,
     REG_32B | REG_SPECIAL | REG_SIDE_READ,
     0
    },
    {"AUD1_STA",                      		/* Status register */
     AUD1,
     AUD_STA,
     0x3C00,
     0x0000,
     0,
     REG_32B,
     0
    },
    {"AUD1_PES_CFG",                      	/* PES Parser Configuration */
     AUD1,
     AUD_PES_CFG,
     0x3d,
     0x3d,
     0,
     REG_32B,
     0
    },
    {"AUD1_PES_SI",                      	/* PES Stream Identification */
     AUD1,
     AUD_PES_SI,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"AUD1_PES_FSI",                      	/* Filter PES Stream ID */
     AUD1,
     AUD_PES_FSI,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"AUD1_PES_SSI",                      	/* PES SubStream Identification */
     AUD1,
     AUD_PES_SSI,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"AUD1_PES_FSSI",                      	/* Filter PES SubStream ID */
     AUD1,
     AUD_PES_FSSI,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
	{"AUD1_PLY",                      		/* Audio Play */
     AUD1,
     AUD_PLY,
     0x1,
     0x1,
     0,
     REG_32B,
     0
    },
	{"AUD1_SETUP",                      	/* Audio Setup */
     AUD1,
     AUD_SETUP,
     0x1f,
     0x1f,
     0,
     REG_32B,
     0
    },

	/* PCM Player --------------------------------------------------------- */
	{"AUD_PCM_CFG",                      	/* PCM player module configuration */
     AUDPCM,
     AUD_PCM_CFG,
     0x1f,
     0x1f,
     0,
     REG_32B,
     0
    },
	{"AUD_PCM_ITM",                      	/* PCM interrupt mask */
     AUDPCM,
     AUD_PCM_ITM,
     0x3,
     0x3,
     0,
     REG_32B,
     0
    },
	{"AUD_PCM_ITS",                      	/* PCM interrupt status */
     AUDPCM,
     AUD_PCM_ITS,
     0x3,
     0x0,
     0,
     REG_32B | REG_SPECIAL | REG_SIDE_READ,
     0
    },
	{"AUD_PCM_STA",                      	/* PCM status register */
     AUDPCM,
     AUD_PCM_STA,
     0x3c00,
     0x0,
     0,
     REG_32B,
     0
    },
	{"AUD_PCM_MOD",                      	/* PCM player mode */
     AUDPCM,
     AUD_PCM_MOD,
     0x1,
     0x1,
     0,
     REG_32B | REG_EXCL_FROM_TEST,
     0
    },
	{"AUD_PCM_READ",                      	/* PCM buffer read pointer */
     AUDPCM,
     AUD_PCM_READ,
     0x3ffff80,
     0x3ffff80,
     0,
     REG_32B,
     0
    },
	{"AUD_PCM_PLY",                      	/* PCM play control */
     AUDPCM,
     AUD_PCM_PLY,
     0x1,
     0x1,
     0,
     REG_32B | REG_EXCL_FROM_TEST,
     0
    },
	{"AUD_PCM_NXT",                      	/* PCM buffer next read pointer */
     AUDPCM,
     AUD_PCM_NXT,
     0x3ffff80,
     0x3ffff80,
     0,
     REG_32B,
     0
    },
	{"AUD_PCM_REF",                      	/* PCM buffer reference pointer */
     AUDPCM,
     AUD_PCM_REF,
     0x3ffff80,
     0x3ffff80,
     0,
     REG_32B,
     0
    },
	{"AUD_PCM_SRS",                      	/* PCM player soft reset */
     AUDPCM,
     AUD_PCM_SRS,
     0x0,
     0x1,
     0,
     REG_32B | REG_EXCL_FROM_TEST,
     0
    },
	{"AUD_PCM_WRITE",                      	/* PCM player write threshold pointer */
     AUDPCM,
     AUD_PCM_WRITE,
     0x3ffff80,
     0x3ffff80,
     0,
     REG_32B,
     0
    },

	/* Audio 2 --------------------------------------------------------- */
    {"AUD2_BBG",                      		/* Audio bit buffer Begin */
     AUD2,
     AUD_BBG,
     0x3ffff00,
     0x3ffff00,
     0,
     REG_32B,
     0
    },
    {"AUD2_BBS",                      		/* Audio bit buffer Stop */
     AUD2,
     AUD_BBS,
     0x3ffff00,
     0x3ffff00,
     0,
     REG_32B,
     0
    },
    {"AUD2_BBT",                      		/* Audio bit buffer Threshold */
     AUD2,
     AUD_BBT,
     0x3ffff00,
     0x3ffff00,
     0,
     REG_32B,
     0
    },
    {"AUD2_BBL",                      		/* Audio bit buffer Level */
     AUD2,
     AUD_BBL,
     0x3ffff00,
     0x0000000,
     0,
     REG_32B,
     0
    },
    {"AUD2_STP",                      		/* Audio Setup */
     AUD2,
     AUD_STP,
     0x1,
     0x1,
     0,
     REG_32B,
     0
    },
    {"AUD2_CDI",                      		/* CD Input from host interface */
     AUD2,
     AUD_CDI,
     0x0,
     0xffffffff,
     0,
     REG_32B,
     0
    },
    {"AUD2_SRS",                      		/* Decoding Soft Reset */
     AUD2,
     AUD_SRS,
     0x0,
     0x1,
     0,
     REG_32B | REG_EXCL_FROM_TEST,
     0
    },
    {"AUD2_ITM",                      		/* Interrupt mask */
     AUD2,
     AUD_ITM,
     0x3C00,
     0x3C00,
     0,
     REG_32B,
     0
    },
    {"AUD2_ITS",                      		/* Interrupt status */
     AUD2,
     AUD_ITS,
     0x3C00,
     0x0000,
     0,
     REG_32B | REG_SPECIAL | REG_SIDE_READ,
     0
    },
    {"AUD2_STA",                      		/* Status register */
     AUD2,
     AUD_STA,
     0x3C00,
     0x0000,
     0,
     REG_32B,
     0
    },
    {"AUD2_PES_CFG",                      	/* PES Parser Configuration */
     AUD2,
     AUD_PES_CFG,
     0x3d,
     0x3d,
     0,
     REG_32B,
     0
    },
    {"AUD2_PES_SI",                      	/* PES Stream Identification */
     AUD2,
     AUD_PES_SI,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"AUD2_PES_FSI",                      	/* Filter PES Stream ID */
     AUD2,
     AUD_PES_FSI,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"AUD2_PES_SSI",                      	/* PES SubStream Identification */
     AUD2,
     AUD_PES_SSI,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"AUD2_PES_FSSI",                      	/* Filter PES SubStream ID */
     AUD2,
     AUD_PES_FSSI,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
	{"AUD2_PLY",                      		/* Audio Play */
     AUD2,
     AUD_PLY,
     0x1,
     0x1,
     0,
     REG_32B,
     0
    },
	{"AUD2_SETUP",                      	/* Audio Setup */
     AUD2,
     AUD_SETUP,
     0x3f,
     0x3f,
     0,
     REG_32B,
     0
    },

	/* MMDSP --------------------------------------------------------- */
	/* Version ------------------------------------------------------- */
	{"AUD_IDENT",                      		/* Identity */
     MMDSPBASE,
     AUD_IDENT,
     0xff,
     0x00,
     0xAC,
     REG_32B,
     0x0
    },
	{"AUD_SOFTVER",                      	/* Soft Version */
     MMDSPBASE,
     AUD_SOFTVER,
     0xff,
     0xff,
     0x4D,
     REG_32B,
     0x0
    },
	{"AUD_VERSION",                      	/* Version */
     MMDSPBASE,
     AUD_VERSION,
     0xff,
     0x0,
     0x30,
     REG_32B,
     0x0
    },
	{"AUD_SIN_SETUP",                      	/* Input data setup */
     MMDSPBASE,
     AUD_SIN_SETUP,
     0x0f,
     0x0f,
     0,
     REG_32B,
     0x0
    },
	{"AUD_CAN_SETUP",                      	/* Input A/D converter setup */
     MMDSPBASE,
     AUD_CAN_SETUP,
     0x0f,
     0x0f,
     0,
     REG_32B,
     0x0
    },
	/* PCM configuration ------------------------------------------------------- */
	{"AUD_PCMDIVIDER",                      /* Divider for PCM clock */
     MMDSPBASE,
     AUD_PCMDIVIDER,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_PCMCONF",                      	/* PCM configuration */
     MMDSPBASE,
     AUD_PCMCONF,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_PCMCROSS",                      	/* Cross PCM channels */
     MMDSPBASE,
     AUD_PCMCROSS,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	/* DAC and PLL configuration ------------------------------------------------------- */
	{"AUD_SFREQ",                      		/* Sampling frequency */
     MMDSPBASE,
     AUD_SFREQ,
     0x3f,
     0x3f,
     0,
     REG_32B,
     0x0
    },
	{"AUD_EMPH",                      		/* Emphasis */
     MMDSPBASE,
     AUD_EMPH,
     0x3,
     0x3,
     0,
     REG_32B,
     0x0
    },
	{"AUD_PLLPCM",                      	/* PCM PLL Disable */
     MMDSPBASE,
     AUD_PLLPCM,
     0x7,
     0x7,
     0x1,
     REG_32B,
     0x0
    },
	{"AUD_PLLMASK",                      	/* PCM PLL mask */
     MMDSPBASE,
     AUD_PLLMASK,
     0x0,
     0x1,
     0,
     REG_32B,
     0x0
    },
	/* Channel delay setup ------------------------------------------------------- */
	{"AUD_LDLY",                      		/* Left channel delay */
     MMDSPBASE,
     AUD_LDLY,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_RDLY",                      		/* Right channel delay */
     MMDSPBASE,
     AUD_RDLY,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_CDLY",                      		/* Center channel delay */
     MMDSPBASE,
     AUD_CDLY,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SUBDLY",                      	/* Subwoofer channel delay */
     MMDSPBASE,
     AUD_SUBDLY,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_LSDLY",                      		/* Left surround channel delay */
     MMDSPBASE,
     AUD_LSDLY,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_RSDLY",                      		/* Right surround channel delay */
     MMDSPBASE,
     AUD_RSDLY,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_LVCR_DLY", 	                	/* Left surround channel delay */
     MMDSPBASE,
     AUD_LVCR_DLY,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_RVCR_DLY", 	                	/* Right surround channel delay */
     MMDSPBASE,
     AUD_RVCR_DLY,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_DLYUPDATE",                      	/* PCM delay update */
     MMDSPBASE,
     AUD_DLYUPDATE,
     0x3,
     0x3,
     0,
     REG_32B | REG_EXCL_FROM_TEST,
     0x0
    },
	/* S/PDIF output setup ------------------------------------------------------- */
	{"AUD_SPDIF_CAT",                      	/* SPDIF category code */
     MMDSPBASE,
     AUD_SPDIF_CAT,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SPDIF_STATUS",                     /* SPDIF status */
     MMDSPBASE,
     AUD_SPDIF_STATUS,
     0x7f,
     0x7f,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SPDIF_REP_TIME",                    /* SPDIF repetition time */
     MMDSPBASE,
     AUD_SPDIF_REP_TIME,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SPDIF_LATENCY",                    /* SPDIF latency value */
     MMDSPBASE,
     AUD_SPDIF_LATENCY,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SPDIF_CMD",                      	/* SPDIF control */
     MMDSPBASE,
     AUD_SPDIF_CMD,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SPDIF_CONF",                      /* SPDIF conf */
     MMDSPBASE,
     AUD_SPDIF_CONF,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SPDIF_DTDI",                    	/* SPDIF data type information */
     MMDSPBASE,
     AUD_SPDIF_DTDI,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	/* Audio command ------------------------------------------------------- */
	{"AUD_SOFTRESET",                    	/* Audio reset */
     MMDSPBASE,
     AUD_SOFTRESET,
     0x0,
     0x1,
     0,
     REG_32B | REG_EXCL_FROM_TEST,
     0x0
    },
	{"AUD_PLAY",                    		/* Audio play */
     MMDSPBASE,
     AUD_PLAY,
     0x1,
     0x1,
     0,
     REG_32B,
     0x0
    },
	{"AUD_MUTE",                    		/* Audio mute */
     MMDSPBASE,
     AUD_MUTE,
     0x1,
     0x1,
     0,
     REG_32B,
     0x0
    },
	{"AUD_RUN",                    			/* Audio run */
     MMDSPBASE,
     AUD_RUN,
     0x1,
     0x1,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SKIP_MUTE_CMD",                   /* Skip or mute commands */
     MMDSPBASE,
     AUD_SKIP_MUTE_CMD,
     0x0f,
     0x0f,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SKIP_MUTE_VALUE",                 /* Skip frames or mute blocks of frame */
     MMDSPBASE,
     AUD_SKIP_MUTE_VALUE,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	#if 0 /*replaced by INTEH and INTEL*/
	{"AUD_INTE",                 			/* Interrupt enable */
     MMDSPBASE,
     AUD_INTEL,
     0xff,
     0xff,
     0,
     REG_32B|REG_ARRAY,
     2
    },  /*replaced by INTH and INTL*/
	{"AUD_INT",                 			/* Interrupt */
     MMDSPBASE,
     AUD_INTL,
     0xff,
     0xff,
     0,
     REG_32B|REG_ARRAY,
     2
    },
	#endif
	{"AUD_INTEH",                 			/* Interrupt enable */
     MMDSPBASE,
     AUD_INTEH,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
	{"AUD_INTEL",                 			/* Interrupt enable */
     MMDSPBASE,
     AUD_INTEL,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
	{"AUD_INTH",                 			/* Interrupt enable */
     MMDSPBASE,
     AUD_INTH,
     0xff,
     0x00,
     0,
     REG_32B,
     0
    },
	{"AUD_INTL",                 			/* Interrupt enable */
     MMDSPBASE,
     AUD_INTL,
     0xff,
     0x00,
     0,
     REG_32B,
     0
    },
	/* ES audio parser information extraction ------------------------------------------------------- */
	{"AUD_SYNC_STATUS",                 	/* Synchronization status */
     MMDSPBASE,
     AUD_SYNC_STATUS,
     0xf,
     0x0,
     0,
     REG_32B,
     0x0
    },
	{"AUD_ANCCOUNT",                 		/* Ancillary data */
     MMDSPBASE,
     AUD_ANCCOUNT,
     0xff,
     0x0,
     0,
     REG_32B,
     0x0
    },
	{"AUD_HEAD1",                 			/* Header 1 */
     MMDSPBASE,
     AUD_HEAD1,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_HEAD0",                 			/* Header 0 */
     MMDSPBASE,
     AUD_HEAD0,
     0x1f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_HEADLENH",                 		/* Frame length */
     MMDSPBASE,
     AUD_HEADLENH,
     0xff,
     0x00,
     0,
     REG_32B,
     0
    },
	{"AUD_HEADLENL",                 		/* Frame length */
     MMDSPBASE,
     AUD_HEADLENL,
     0xff,
     0x00,
     0,
     REG_32B,
     0
    },
	/* Presenting time stamp ------------------------------------------------------- */
	{"AUD_PTS4",                 			/* PTS */
     MMDSPBASE,
     AUD_PTS4,
     0x01,
     0x01,
     0,
     REG_32B,
     0
    },
	{"AUD_PTS3",                 			/* PTS */
     MMDSPBASE,
     AUD_PTS3,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
	{"AUD_PTS2",                 			/* PTS */
     MMDSPBASE,
     AUD_PTS2,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
	{"AUD_PTS1",                 			/* PTS */
     MMDSPBASE,
     AUD_PTS1,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
	{"AUD_PTS0",                 			/* PTS */
     MMDSPBASE,
     AUD_PTS0,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
	{"AUD_ERROR",                 			/* Error code */
     MMDSPBASE,
     AUD_ERROR,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	/* Decoding algorithm ------------------------------------------------------- */
	{"AUD_DECODESEL",                 		/* Decoding algorithm */
     MMDSPBASE,
     AUD_DECODESEL,
     0xf,
     0xf,
     0,
     REG_32B,
     0x0
    },
	{"AUD_STREAMSEL",                 		/* Stream selection */
     MMDSPBASE,
     AUD_STREAMSEL,
     0x7,
     0x7,
     0,
     REG_32B,
     0x0
    },
	/* System synchronization ------------------------------------------------------- */
	{"AUD_PACKET_LOCK",                 	/* Packet lock */
     MMDSPBASE,
     AUD_PACKET_LOCK,
     0x01,
     0x01,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AUDIO_ID_EN",                 	/* Enable audio ID */
     MMDSPBASE,
     AUD_AUDIO_ID_EN,
     0x01,
     0x01,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AUDIO_ID",                 		/* Audio ID */
     MMDSPBASE,
     AUD_AUDIO_ID,
     0x1f,
     0x1f,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AUDIO_ID_EXT",                 	/* Audio ID extension */
     MMDSPBASE,
     AUD_AUDIO_ID_EXT,
     0x07,
     0x07,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SYNC_LOCK",                 		/* Sync lock */
     MMDSPBASE,
     AUD_SYNC_LOCK,
     0x03,
     0x03,
     0,
     REG_32B,
     0x0
    },
	/* Post decoding and Prologic ------------------------------------------------------- */
	{"AUD_PDEC",                 			/* Post decoder control */
     MMDSPBASE,
     AUD_PDEC,
     0x31,
     0x31,
     0,
     REG_32B,
     0x0
    },
	{"AUD_PL_AB",                 			/* Prologic auto balance */
     MMDSPBASE,
     AUD_PL_AB,
     0x01,
     0x01,
     0,
     REG_32B,
     0x0
    },
	{"AUD_PL_DWNX",			    			/* Prologic decoder downmix */
     MMDSPBASE,
     AUD_PL_DWNX,
     0x07,
     0x07,
     0,
     REG_32B,
     0x0
    },
	{"AUD_DWSMODE",                 		/* Downsampling filter */
     MMDSPBASE,
     AUD_DWSMODE,
     0x03,
     0x03,
     0,
     REG_32B,
     0x0
    },
	/* Bass redirection and volume control ------------------------------------------------------- */
	{"AUD_VOLUME0",                 		/* Volume of first channel */
     MMDSPBASE,
     AUD_VOLUME0,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_VOLUME1",                 		/* Volume of second channel */
     MMDSPBASE,
     AUD_VOLUME1,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_OCFG",                 			/* Output configuration */
     MMDSPBASE,
     AUD_OCFG,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_CHAN_IDX",                 		/* Channel index */
     MMDSPBASE,
     AUD_CHAN_IDX,
     0x07,
     0x07,
     0,
     REG_32B | REG_EXCL_FROM_TEST,
     0x0
    },
	/* Dolby digital configuration ------------------------------------------------------- */
	{"AUD_AC3_DECODE_LFE",                 	/* Decode LFE */
     MMDSPBASE,
     AUD_AC3_DECODE_LFE,
     0x01,
     0x01,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_COMP_MOD",                 	/* Compression mode */
     MMDSPBASE,
     AUD_AC3_COMP_MOD,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_HDR",                 		/* High dynamic range */
     MMDSPBASE,
     AUD_AC3_HDR,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_LDR",                 		/* Low dynamic range */
     MMDSPBASE,
     AUD_AC3_LDR,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_RPC",                 		/* Repeat count */
     MMDSPBASE,
     AUD_AC3_RPC,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_KARAMODE",                 	/* Karaoke downmix */
     MMDSPBASE,
     AUD_AC3_KARAMODE,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_DUALMODE",                 	/* Dual downmix */
     MMDSPBASE,
     AUD_AC3_DUALMODE,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_DOWNMIX",                 	/* Downmix */
     MMDSPBASE,
     AUD_AC3_DOWNMIX,
     0x0f,
     0x0f,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_STATUS0",                 	/* Dolby Digital status register 0 */
     MMDSPBASE,
     AUD_AC3_STATUS0,
     0x7f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_STATUS1",                 	/* Dolby Digital status register 1 */
     MMDSPBASE,
     AUD_AC3_STATUS1,
     0x0f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_STATUS2",                 	/* Dolby Digital status register 2 */
     MMDSPBASE,
     AUD_AC3_STATUS2,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_STATUS3",                 	/* Dolby Digital status register 3 */
     MMDSPBASE,
     AUD_AC3_STATUS3,
     0x0f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_STATUS4",                 	/* Dolby Digital status register 4 */
     MMDSPBASE,
     AUD_AC3_STATUS4,
     0x1f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_STATUS5",                 	 /* Dolby Digital status register 5 */
     MMDSPBASE,
     AUD_AC3_STATUS5,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_STATUS6",                    	/* Dolby Digital status register 6 */
     MMDSPBASE,
     AUD_AC3_STATUS6,
     0x1f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_AC3_STATUS7",                    	/* Dolby Digital status register 7 */
     MMDSPBASE,
     AUD_AC3_STATUS7,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_MP_SKIP_LFE",                 	/* Channel skip */
     MMDSPBASE,
     AUD_MP_SKIP_LFE,
     0x01,
     0x01,
     0,
     REG_32B | REG_EXCL_FROM_TEST, 			/*Excluded because already tested with AUD_AC3_ registers*/
     0x0
    },
	{"AUD_MP_PROG_NUMBER",                 	/* Programm number */
     MMDSPBASE,
     AUD_MP_PROG_NUMBER,
     0x01,
     0x01,
     0,
     REG_32B | REG_EXCL_FROM_TEST, 			/*Excluded because already tested with AUD_AC3_ registers*/
     0x0
    },
	{"AUD_MP_DUALMODE",                 	/* MPEG setup dual mode */
     MMDSPBASE,
     AUD_MP_DUALMODE,
     0xff,
     0xff,
     0,
     REG_32B | REG_EXCL_FROM_TEST, 			/*Excluded because already tested with AUD_AC3_ registers*/
     0x0
    },
	{"AUD_MP_DRC",                 			/* Dynamic range control */
     MMDSPBASE,
     AUD_MP_DRC,
     0x01,
     0x01,
     0,
     REG_32B | REG_EXCL_FROM_TEST, 			/*Excluded because already tested with AUD_AC3_ registers*/
     0x0
    },
	{"AUD_MP_CRC_OFF",                 		/* CRC check off */
     MMDSPBASE,
     AUD_MP_CRC_OFF,
     0xff,
     0xff,
     0,
     REG_32B | REG_EXCL_FROM_TEST, 			/*Excluded because already tested with AUD_AC3_ registers*/
     0x0
    },
	{"AUD_MP_MC_OFF",                 		/* Multi-channel */
     MMDSPBASE,
     AUD_MP_MC_OFF,
     0x1f,
     0x1f,
     0,
     REG_32B | REG_EXCL_FROM_TEST,			/*Excluded because already tested with AUD_AC3_ registers*/
     0x0
    },
	{"AUD_MP_DOWNMIX",                 		/* MPEG downmix */
     MMDSPBASE,
     AUD_MP_DOWNMIX,
     0x1f,
     0x1f,
     0,
     REG_32B | REG_EXCL_FROM_TEST, 			/*Excluded because already tested with AUD_AC3_ registers*/
     0x0
    },
	{"AUD_MP_STATUS0",                 		/* MPEG status register 0 */
     MMDSPBASE,
     AUD_MP_STATUS0,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_MP_STATUS1",                 		/* MPEG status register 1 */
     MMDSPBASE,
     AUD_MP_STATUS1,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_MP_STATUS2",                 		/* MPEG status register 2 */
     MMDSPBASE,
     AUD_MP_STATUS2,
     0x0f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_MP_STATUS3",                 	  	/* MPEG status register 3 */
     MMDSPBASE,
     AUD_MP_STATUS3,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_MP_STATUS4",                 	  	/* MPEG status register 4 */
     MMDSPBASE,
     AUD_MP_STATUS4,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_MP_STATUS5",                 		/* MPEG status register 5 */
     MMDSPBASE,
     AUD_MP_STATUS5,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	/* Pink noise ------------------------------------------------------- */
/* Only available on STi7015 chip. */
#if defined (ST_7015)
    {"AUD_PN_DOWNMIX",                      /* Pink noise downmix */
     MMDSPBASE,
     AUD_PN_DOWNMIX,
     0x3f,
     0x3f,
     0,
     REG_32B | REG_EXCL_FROM_TEST, 			/*Excluded because already tested with AUD_AC3_ registers*/
     0x0
    },
#endif
    /* DTS configuration ------------------------------------------------------- */
	{"AUD_DTS_STATUS0",                 	/* DTS status 0 register */
     MMDSPBASE,
     AUD_DTS_STATUS0,
     0xff,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_DTS_STATUS1",                 	/* DTS status 1 register */
     MMDSPBASE,
     AUD_DTS_STATUS1,
     0x3f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_DTS_STATUS2",                 	/* DTS status 2 register */
     MMDSPBASE,
     AUD_DTS_STATUS2,
     0x0f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	{"AUD_DTS_STATUS3",                 	/* DTS status 3 register */
     MMDSPBASE,
     AUD_DTS_STATUS3,
     0x1f,
     0x00,
     0,
     REG_32B,
     0x0
    },
	/* PCM beep-tone configuration ------------------------------------------------------- */
	{"AUD_PCM_BTONE",                 		/* PCM beep tone frequency */
     MMDSPBASE,
     AUD_PCM_BTONE,
     0xff,
     0xff,
     0,
     REG_32B | REG_EXCL_FROM_TEST, 			/*Excluded because already tested with AUD_AC3_ registers*/
     0x0
    },
	/* Audio trick-mode configuration ------------------------------------------------------- */
/* Only available on STi7015 chip. */
#if defined (ST_7015)
    {"AUD_TM_SPEED",                        /* Audio trick mode speed */
     MMDSPBASE,
     AUD_TM_SPEED,
     0xff,
     0xff,
     0,
     REG_32B | REG_EXCL_FROM_TEST, 			/*Excluded because already tested with AUD_LDLY register*/
     0x0
    },
#endif
    /* PCM Mixing ------------------------------------------------------- */
	{"AUD_MIX_UPDATE", 	                	/* DSP configuration updates */
     MMDSPBASE,
     AUD_MIX_UPDATE,
     0x1f,
     0x1f,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SFREQ2", 	                		/* PCM sampling frequency for the 2nd input */
     MMDSPBASE,
     AUD_SFREQ2,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_MIX_LEVEL", 	                	/* Mixer level input */
     MMDSPBASE,
     AUD_MIX_LEVEL,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SRC_HANDSHAKE", 	                /* Handshake for SRC coefficient downloading */
     MMDSPBASE,
     AUD_SRC_HANDSHAKE,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SRC_MSB", 	                	/* 8 MSB coefficient */
     MMDSPBASE,
     AUD_SRC_MSB,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_SRC_LSB", 	                	/* 8 LSB coefficient  */
     MMDSPBASE,
     AUD_SRC_LSB,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_INPUT2_PLAY", 	                /* PCM input processing on/off */
     MMDSPBASE,
     AUD_INPUT2_PLAY,
     0x01,
     0x01,
     0,
     REG_32B,
     0x0
    },
	{"AUD_INPUT2_CFG", 	                	/* PCM input configuration */
     MMDSPBASE,
     AUD_INPUT2_CFG,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_INPUT2_MODE", 	                /* PCM input mode */
     MMDSPBASE,
     AUD_INPUT2_MODE,
     0xff,
     0xff,
     0,
     REG_32B,
     0x0
    },
	{"AUD_ACK", 	                		/* Acknowledge */
     MMDSPBASE,
     AUD_ACK,
     0x01,
     0x01,
     0,
     REG_32B | REG_EXCL_FROM_TEST,
     0x0
    },
	/*JUST FOR DEBUG PURPOSE :*/
    {"CFG_AUD_IO",                              /*  */
#if defined (ST_7015)
     STI7015_BASE_ADDRESS,
#else
     STI7020_BASE_ADDRESS,
#endif
     CFG_AUD_IO,
     0x07,
     0x07,
     0,
     REG_32B,
     0x0
    }
};

/* Nbr of registers */
#define AUDIO_REGISTERS (sizeof(AudioReg)/sizeof(Register_t))



#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_TREG_H */
/* ------------------------------- End of file ---------------------------- */


