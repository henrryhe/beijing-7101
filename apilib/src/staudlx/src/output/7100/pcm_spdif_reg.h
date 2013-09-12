
/*
******************************************************************************
STAUDREG.H - (C) Copyright STMicroelectronics
Register mapping for 5500,5505,5508,5518,5580
******************************************************************************
*/

/* Define to prevent recursive inclusion */
#ifndef __7100_REG_H
#define __7100_REG_H

/* Includes --------------------------------------------------------------- */
#if !defined( ST_OSLINUX )
#include "stsys.h"
#endif
#include "stdevice.h"

/* Exported Types --------------------------------------------------------- */

/*
===============================================================================
	Configuration registers
The ICMonitorBaseAddress is:0x2040 2000.
The ICControlBaseAddress is:0x2030 3000.
==============================================================================
*/
#define ICMONITOR_BASE_ADDRESS		0x20402000 /* NOTE PC : Should be replaced by some Base address in stdevice.h */
#define CONFIG_MONITOR_E  			0x0028
#define CONFIG_MONITOR_G  			0x0030

#define ICCONTROL_BASE_ADDRESS		0x20402000 /* NOTE PC : Should be replaced by some Base address in stdevice.h */
#define CONFIG_CTRL_A				0x0000
#define CONFIG_CTRL_B				0x0000
#define CONFIG_CTRL_C				0x0000
#define CONFIG_CTRL_D				0x0004
#define CONFIG_CTRL_E				0x0008
#define CONFIG_CTRL_F				0x000C
#define CONFIG_CTRL_G				0x0010

#define ADAC_RST_N 		0x2000
#define ADAC_MODE_SELECT	0x4000
#define ADAC_BG_POWER_DOWN	0x8000
#define ADAC_ANALOG_POWER_DOWN	0x10000
#define ADAC_DIGITAL_POWER_DOWN	0x20000
#define ADAC_SOFT_MUTE		0x40000
#define FDMA_CLK_SELECT		0x80000
#define PCM_IRLOCK_INVERT	0x400000
#define PCM_SOURCE_SELECT	0x800000
#define PCM_CHANNEL_SELECT	0x03000000

#define PCM_CHANNEL_SELECT_PCMOUT1		0x00
#define PCM_CHANNEL_SELECT_PCMOUT2		0x01
#define PCM_CHANNEL_SELECT_PCMOUT3		0x02
#define PCM_CHANNEL_SELECT_PCMOUT4		0x03
#define PCM_DATA_CLOCK_FROM_PCM_PLAYER		0x00
#define PCM_DATA_CLOCK_FROM_PAD			0x01
#define FDMA_CLK_SELECT_FROM_PLL		0x00
#define FDMA_CLK_SELECT_FROM_FRESYNTHESIZER	0x01
#define ADAC_MODE_SELECT_NORMAL_MODE		0x00
#define ADAC_MODE_SELECT_DOUBLE_MODE		0x01




/*
===============================================================================
audio frequency synthesizer,The SysServBaseAddress is:
0x20F0 0000.
==============================================================================
*/
#define SYS_SERVICES_BASE_ADDRESS   0x20F00000/* NOTE PC : Should be replaced by some Base address in stdevice.h */
#define CKG_PCM_PLAYER_CLK_SETUP0	0x020
#define CKG_PCM_PLAYER_CLK_SETUP1	0x024
#define CKG_SPDIF_PLAYER__CLK_SETUP0	0x030
#define CKG_SPDIF_PLAYER_CLK_SETUP1	0x034

/*
===============================================================================
PTI The PTIBaseAddress is:
0x20E0 0000.
==============================================================================
*/
/*#define PTI_BASE_ADDRESS 		0x20E00000*/
#define PTI_DMA0BASE			0x1000
#define PTI_DMA0_READ			0x100C
#define PTI_DMA0WRITE			0x1008
#define CKG_SPDIF_PLAYER_CLK_SETUP1	0x034

/*
=============================================================================
Audio decoder registers AudioBaseAddress 0x20700000.
=============================================================================
*/

#define AUDIO_BASE_ADDRESS		0x20700000 /* NOTE PC ; SHould not be used for STi7100 as decoder is accessed through Multicom */
#define AUD_DEC_INT_ENABLE		0x008

enum
{
 AUD_GLOBAL_ENB_INT=0x01,
 AUD_SW_RESET_ENB_INT=0x02, /* Interrut After the Soft Reset */
 AUD_FRAME_LOCKED_ENB_INT=0x04,
 AUD_CRC_ERROR_ENB_INT=0x08,
 AUD_ORIGINAL_ENB_INT=0x10,
 AUD_EMPHASIS_ENB_INT=0x20,
 AUD_CDFIFO_EMPTY_ENB_INT=0x40,
 AUD__PMCFIFO_FULL_ENB_INT=0x80,
 AUD_FS_CHANGE_ENB_INT=0x100
 };

#define AUD_DEC_INT_STATUS       	0x00C

enum
{
 AUD_INT_PENDING=0x01,
 AUD_SW_RESET_INT=0x02,
 AUD_FRAME_LOCKED_INT=0x04,
 AUD_CRC_ERROR_INT=0x08,
 AUD_ORIGINAL_INT=0x10,
 AUD_EMPHASIS_INT=0x20,
 AUD_CDFIFO_EMPTY_INT=0x40,
 AUD__PMCFIFO_FULL_INT=0x80,
 AUD_FS_CHANGE_INT=0x100
 };
#define AUD_DEC_CONFIG		0x000
#define AUD_DEC_STATUS		0x004

#define	AUD_SW_RESET 		0x02
#define AUD_FRAME_LOCKED	0x04
#define  AUD_CRC_ERROR		0x08
#define  AUD_ORIGINAL		0x10
#define AUD_EMPHASIS		0x60
#define  AUD_VERSION_ID		0x80
#define  AUD_LAYER_ID		0x300
#define AUD_BITRATE_INDEX	0x3C00
#define  AUD_SAMPFREQ__INDEX	0xC000
#define AUD_AUDIO_MODE		0x30000
#define AUD_MODE_EXTN		0xC0000
#define AUD_COPY_RIGHT		0x100000
#define AUD_CDFIFO_STATUS	0x7100000
#define AUD_PCMFIFO_STATUS	0xF0000000



#define AUD_DEC_INT_CLEAR		0x010
enum
{
CLR_SW_RESET_INT =0x02,
CLR_FRAME_LOCKED_INT=0x04,
CLR_CRC_ERROR_INT=0x08,
CLR_ORIGINAL_INT=0x10,
CLR_EMPHASIS_INT=0x20,
CLR_CDFIFO_EMPTY_INT=0x40,
CLR_PCMFIFO_FULL_INT=0x80,
CLR_FS_CHANGE_INT=0x100
};

#define AUD_DEC_VOLUME_CONTROL		0x014
#define AUD_DEC_CD_IN			0x040
#define AUD_DEC_PCM_OUT			0x060
/*
=============================================================================
PCM player registers and PCM base address 0x2140 0000.
=============================================================================
*/

#define PCMP_CONTROL			0x01C
enum
{
OPERATION=0x03,
FORMAT =0x04,
ROUNDING=0x08,
DIVIDER=0xFF0,
WAIT_EOLATENCY=0x100
};
#define PCMP_STATUS			0x020
enum
{
RUNSTOP=0x01,
UNDERFLOW=0x02,
AUDIO_SAMPLES_MEMORY_FULLY_READ=0x04
};

#define PCMP_FORMAT			0x024
enum
{
BITPERSUBFRAME=0x01,
DATALENGTH=0x060,
LRLEVEL=0x080,
SCLKEDGE=0x10,
PADDING=0x20,
ALIGN=0x40,
ORDER=0x80
};

#define PCMP_DATA			0x004

/*
==========================================================================
S/PDIF player and GPFIFO registers S/PDIFBaseAddress 0x2060 0000.
==========================================================================
*/
#define SPDIF_CTRL			0x001C

#define SPDIF_OPERATION		=0x07
#define IDLESTATE		=0x08
#define SPDIF_DIVIDER		=0x1FE0
#define BYTE_SWAP		=0x2000
#define SAMPLES			=0xFFFF8000


#define SPDIF_STATUS			0x0020
enum
{
SPDIF_RUNSTOP=0x01,
SPDIF_UNDERFLOW=0x02,
EO_BURST=0x04,
EO_BLOCK=0x08,
LATENCY=0x10,
PD_DATABURST=0x20,
AUDIO_SAMPLES=0x40,
PA_C_BIT_NUMBER=0x7F80,
PD_PAUSEBURST=0x8000
};

#define AUDIO_IOCTL_OFFSET  0x0200
#define SPDIF_PA_PB			0x0024
#define SPDIF_PC_PD			0x0028
#define SPDIF_CL1			0x002C
#define SPDIF_CR1			0x0030
#define SPDIF_CL2_CR2_U_V	0x0034

#define CHANNEL_STATUS_LEFT_SUBFRAME		0x0F
#define CHANNEL_STATUS_RIGHT_SUBFRAME		0x0F00
#define USER_VALUE_LEFT_SUBFRAME		0x10000
#define USER_VALUE_RIGHT_SUBFRAME		0x20000
#define VALIDITY_LEFT_SUBFRAME			0x40000
#define VALIDITY_RIGHT_SUBFRAME			0x80000

#define SPDIF_PAUSE_LAT			0x0038
#define SPDIF_BURST_LEN			0x003C
#define SPDIF_SOFTRESET			0x0000
#define SPDIF_FIFO_DATA			0x0004
#define SPDIF_INT_STATUS		0x0008
#define SPDIF_INT_STATUS_CLR		0x000C
#define SPDIF_INT_EN			0x0010
#define SPDIF_INT_EN_SET		0x0014
#define SPDIF_INT_EN_CLR		0x0018

#endif /* __7100_REG_H */

