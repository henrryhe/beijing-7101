/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : Aud_cmd.h
Description : Used STAUD definition

Date          Modification
----          ------------
June 2002    Creation
************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __AUD_CMD_H
#define __AUD_CMD_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define STAUD_DEVICE_NAME1              "LONGAUDDEVNAME"   /* device name */


#if defined (ST_7100) || defined (ST_7109)
typedef struct
{
    unsigned int *P_Audio_pointer;
    unsigned int Audio_Size;
    unsigned int Samples;
    char AudioTerminate;
    unsigned int Loop;
}AudioStream_Player;

typedef enum
{
   subframebit_32,
   subframebit_16
}PCMPlayer_Format_Subframe;

typedef enum
{
   bit_24,
   bit_20,
   bit_18,
   bit_16
}PCMPlayer_Format_DataLength;

typedef enum
{
   leftword_lrclklow,
   leftword_lrclkhigh
}PCMPlayer_Format_Lrlevel;

typedef enum
{
   risingedge_pcmsclk,
   fallingedge_pcmsclk
}PCMPlayer_Format_SClkedge;

typedef enum
{
   delay_data_onebit_clk,
   nodelay_data_bit_clk
}PCMPlayer_Format_Padding;

typedef enum
{
   data_first_sclkcycle_lrclk,
   data_last_sclkcycle_lrclk
}PCMPlayer_Format_Align;

typedef enum
{
   data_lsbfirst,
   data_msbfirst
}PCMPlayer_Format_Order;

typedef struct
{
   PCMPlayer_Format_Subframe Bit_SubFrame;
   PCMPlayer_Format_DataLength DataLength;
   PCMPlayer_Format_Lrlevel LRlevel;
   PCMPlayer_Format_SClkedge SClkedge;
   PCMPlayer_Format_Padding Padding;
   PCMPlayer_Format_Align Align;
   PCMPlayer_Format_Order Order;
}PCMPlayer_Format;

typedef enum
{
   pcm_off,
   pcm_mute,
   pcm_mode_on_16,
   cd_mode_on
}PCMPlayer_Control_Operation;

typedef enum
{
   bits_16_0,
   bits_16_16
}PCMPlayer_Control_Memory;

typedef enum
{
   no_rounding_pcm,
   bit16_rounding_pcm
}PCMPlayer_Control_Rounding;


typedef enum
{
   pcm_sclk_64=0,
   fs_128_64=1,
   fs_256_64=2,
   fs_384_64=3,
   fs_512_64=4,
   fs_768_64=6
}PCMPlayer_Control_Divider64;

typedef enum
{
   pcm_sclk_32=0,
   fs_128_32=2,
   fs_192_32=3,
   fs_256_32=4,
   fs_384_32=6,
   fs_512_32=8,
   fs_768_32=12
}PCMPlayer_Control_Divider32;

typedef enum
{
   donot_wait_spdif_latency,
   wait_spdif_latency
}PCMPlayer_Control_Waiteol;

typedef struct
{
   PCMPlayer_Control_Operation Operation;
   PCMPlayer_Control_Memory Memory_Storage_Format;
   PCMPlayer_Control_Rounding Rounding;
   PCMPlayer_Control_Divider64 Divider64;
   PCMPlayer_Control_Divider32 Divider32;
   PCMPlayer_Control_Waiteol Wait_Eolatency;
   unsigned int Samples_Read;
}PCMPlayer_Control;

/*****************************************************************/
/*  THIS STRUCTURE WILL HOLD AUDIO STREAM RELATED INFORMATION    */
/*****************************************************************/
 typedef struct
 {
     unsigned int *p_audio_pointer;
     unsigned int audio_size;
     unsigned int samples;
     char audioterminate;
     unsigned int loop;
 }audiostream_player;

/******************************************************************/
/*            SPDIF PLAYER CONTROL REGISTER BITS                  */
/******************************************************************/

typedef enum
	{
		spdif_off,
		spdif_pcm_null_mute,
		spdif_pause_burst_mute,
		spdif_audio_data,
		spdif_encoded_data
	}spdifplayer_control_operation;

typedef enum
	{
		idle_false,
		idle_true
	}spdifplayer_control_idlestate;

typedef enum
	{
		no_rounding_spdif,
		bit16_rounding_spdif
	}spdifplayer_control_rounding;

typedef enum
	{
		spdif_sclk_64=0,
		fs1_128_64=1,
		fs1_256_64=2,
		fs1_384_64=3,
		fs1_512_64=4,
		fs1_768_64=6
	}spdifplayer_control_divider64;

typedef enum
	{
		spdif_sclk_32=0,
		fs1_128_32=2,
		fs1_192_32=3,
		fs1_256_32=4,
		fs1_384_32=6,
		fs1_512_32=8,
		fs1_768_32=12
	}spdifplayer_control_divider32;

typedef enum
	{
		nobyte_swap_encoded_mode,
		byte_swap_encoded_mode
	}spdifplayer_control_byteswap;

typedef enum
	{
		stuffing_software_encoded_mode,
		stuffing_hardware_encoded_mode
	}spdifplayer_control_stuffing_hard_soft;

typedef struct
	{
		spdifplayer_control_operation operation;
		spdifplayer_control_idlestate idlestate;
		spdifplayer_control_rounding rounding;
		spdifplayer_control_divider64 divider64;
		spdifplayer_control_divider32 divider32;
		spdifplayer_control_byteswap byteswap;
		spdifplayer_control_stuffing_hard_soft stuffing_hard_soft;
		unsigned int samples_read;
	}spdifplayer_control;

#endif



/* Exported Functions ------------------------------------------------------- */

BOOL AUD_RegisterCmd(void);
BOOL AUD_CmdStart(void);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __AUD_CMD_H */

