/*******************************************************

 File Name: staudlx.h

 Description: Header File for STAUDLX

 Copyright: STMicroelectronics (c) 2005

*******************************************************/

#ifndef __STAUDLX_H
#define __STAUDLX_H

/* Includes ----------------------------------------- */

#include "stddefs.h"
#include "stavmem.h"
#include "stevt.h"
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
#include "stclkrv.h"
#endif
#define MUTE_AT_DECODER 0
#ifdef __cplusplus
extern "C" {
#endif

    /* Exported Types ----------------------------------- */

    /* Constants ---------------------------------------- */
#define ST_MAX_DEVICE_NAME_TOCOPY (ST_MAX_DEVICE_NAME-1)  /* 15 characters used in strcpy */
#define STAUD_DRIVER_ID       16
#define STAUD_DRIVER_BASE     (STAUD_DRIVER_ID << 16)
#define STAUD_MAX_DEVICE      1
#define STAUD_MAX_OPEN	      1
#define MAX_AUDIO_INSTANCE    4
    /* Error return codes */
#define STAUD_ERROR_BASE     STAUD_DRIVER_BASE
    enum
    {
        STAUD_ERROR_DECODER_RUNNING = STAUD_DRIVER_BASE,
        STAUD_ERROR_DECODER_STOPPED,
        STAUD_ERROR_DECODER_PREPARING,
        STAUD_ERROR_DECODER_PREPARED,
        STAUD_ERROR_DECODER_PAUSING,
        STAUD_ERROR_DECODER_NOT_PAUSING,
        STAUD_ERROR_INVALID_STREAM_ID,
        STAUD_ERROR_INVALID_STREAM_TYPE,
        STAUD_ERROR_INVALID_DEVICE,
        STAUD_ERROR_EVT_REGISTER,
        STAUD_ERROR_EVT_UNREGISTER,
        STAUD_ERROR_CLKRV_OPEN,
        STAUD_ERROR_CLKRV_CLOSE,
        STAUD_ERROR_AVSYNC_TASK,
        STAUD_ERROR_MEMORY_DEALLOCATION_FAILED,
        STAUD_ERROR_INVALID_INPUT_TYPE,
        STAUD_MEMORY_GET_ERROR,
        STAUD_FRAME_ERROR,
        STAUD_ASF_ERROR_INVALID_TAG,
        STAUD_ERROR_IO_REMAP_FAILED,
        STAUD_ERROR_INVALID_STATE,
        STAUD_ERROR_DECODER_START,
        STAUD_ERROR_DECODER_STOP,
        STAUD_ERROR_MME_TRANSFORM,
        STAUD_ERROR_WRONG_CMD_PARAM,
        STAUD_ERROR_INVALID_ENC_CONFIG,
        STAUD_ERROR_ENCODER_START,
        STAUD_ERROR_NOT_ENOUGH_BUFFERS
    };

    /* Events */
    enum
    {
        STAUD_NEW_FRAME_EVT = STAUD_DRIVER_BASE,
        STAUD_DATA_ERROR_EVT,
        STAUD_EMPHASIS_EVT,
        STAUD_PREPARED_EVT,
        STAUD_RESUME_EVT,
        STAUD_STOPPED_EVT,
        STAUD_PCM_UNDERFLOW_EVT,
        STAUD_PCM_BUF_COMPLETE_EVT,
        STAUD_FIFO_OVERF_EVT,
        STAUD_NEW_ROUTING_EVT,
        STAUD_NEW_FREQUENCY_EVT,
        STAUD_CDFIFO_UNDERFLOW_EVT,
        STAUD_SYNC_ERROR_EVT,
        STAUD_LOW_DATA_LEVEL_EVT,

		STAUD_AVSYNC_SKIP_EVT,
		STAUD_AVSYNC_PAUSE_EVT,
		STAUD_IN_SYNC,
		STAUD_OUTOF_SYNC,
		STAUD_PTS_EVT,
		STAUD_EOF_EVT,					//Will be gnerated only for WMA/WMAPro
		STAUD_TEST_NODE_EVT,
		STAUD_TEST_PCM_START_EVT,
		STAUD_TEST_PCM_RESTART_EVT,
		STAUD_TEST_PCM_PAUSE_EVT,
		STAUD_TEST_PCM_STOP_EVT,
		STAUD_TEST_DECODER_STOP_EVT,
		STAUD_RESERVED_EVT

    };

typedef enum
{
	DCREMOVE_DISABLED,
	DCREMOVE_ENABLED,
	DCREMOVE_AUTO,
	// Do not edit below this line
	DCREMOVE_END_OF_PROCESS_APPLY

}STAUD_DCRemove_t;

typedef enum STAUD_PCMMode_e
{
	PCM_ON,
	PCM_OFF

} STAUD_PCMMode_t;

    /* Object type basic masks */
	/* MSB 4 nibbles for CLASS*/
#define    STAUD_CLASS_INPUT			(0x1 << 16) //0x00010000
#define    STAUD_CLASS_DECODER			(0x1 << 17) //0x00020000
#define    STAUD_CLASS_POSTPROC			(0x1 << 18) //0x00040000
#define    STAUD_CLASS_MIXER			(0x1 << 19) //0x00080000
#define    STAUD_CLASS_OUTPUT			(0x1 << 20) //0x00100000
#define    STAUD_CLASS_FORMATTER		(0x1 << 21) //0x00200000
#define    STAUD_CLASS_BLOCKMNG			(0x1 << 22) //0x00400000
#define    STAUD_CLASS_ENCODER			(0x1 << 23) //0x00800000

    /* Added to get complied */
#define     STAUD_TYPE_PPROC_GENERIC			0
#define		STAUD_TYPE_MIXER_GENERIC			0
#define		STAUD_TYPE_OUTPUT_PCM				0

    /* End to added for the compile */

/* Next 2 Nibbles for TYPE*/
#define    STAUD_TYPE_INPUT_COMPRESSED		(0x1 << 8) //0x00000000
#define    STAUD_TYPE_INPUT_PCM				(0x1 << 9) //0x00000100
#define    STAUD_TYPE_INPUT_PCM_READER		(0x1 << 10)//0x00000200

#define    STAUD_TYPE_DECODER_COMPRESSED	(0x1 << 8) //0X00000000
#define    STAUD_TYPE_DECODER_PCMPLAYER		(0x1 << 9) //0X00000100
#define    STAUD_TYPE_DECODER_EXTERNAL		(0x1 << 10) //0X00000200

#define    STAUD_TYPE_ENCODER_COMPRESSED	(0x1 << 8) //0X00000000

#define    STAUD_TYPE_OUTPUT_SPDIF			(0x1 << 8) //0x00000000
#define    STAUD_TYPE_OUTPUT_MULTICHANNEL	(0x1 << 9) //0x00000100
#define    STAUD_TYPE_OUTPUT_STEREO			(0x1 << 10) //0x00000200
#define    STAUD_TYPE_OUTPUT_ANALOG			(0x1 << 11) //0x00000300

#define    STAUD_TYPE_PPROC_TWOCHAN1		(0x1 << 8) //0x00000000
#define    STAUD_TYPE_PPROC_GENERIC1		(0x1 << 9) //0x00000100

#define    STAUD_TYPE_MIXER_2INPUT1			(0x1 << 8) //0x00000100
#define    STAUD_TYPE_MIXER_PCM				(0x1 << 9) //0x00000200

/* Next 2 Nibbles for Instance*/
#define    STAUD_INSTANCE_0			(0x1 << 0) //0x00000000
#define    STAUD_INSTANCE_1			(0x1 << 1) //0x00000001
#define    STAUD_INSTANCE_2			(0x1 << 2) //0x00000002
/* ++NEW_DECODER_ARCHITECTURE */
#define    STAUD_INSTANCE_3			(0x1 << 3) //0x00000003
#define    STAUD_INSTANCE_4			(0x1 << 4) //0x00000004
/* --NEW_DECODER_ARCHITECTURE */

    /* Constants used in STAUD_Capability_t */
#define STAUD_MAX_PCM_FREQS		16
#define STAUD_MAX_TRICK_SPEEDS		6
#define STAUD_MAX_INPUT_OBJECTS		4
#define STAUD_MAX_DECODER_OBJECTS	3
#define STAUD_MAX_POST_PROC_OBJECTS	2
#define STAUD_MAX_MIXER_OBJECTS		1
#define STAUD_MAX_OUTPUT_OBJECTS	5
#define STAUD_MAX_ENCODER_OBJECTS	1

    /* Others */
#define STAUD_IGNORE_ID			0xFF
#define STAUD_EVENT_NUMBER		14

#ifdef ST_OSLINUX
/* This defines the size of the mme buffer which will be used for mme wrapper
     implementation for STLinux
*/
#define MME_DATA_DUMP_SIZE	(3072 * 1024)
#endif

    typedef U32 STAUD_Handle_t;

    typedef ST_ErrorCode_t
    (*GetWriteAddress_t)(void * const handle, void** const address_p);

    typedef ST_ErrorCode_t
    (*InformReadAddress_t)(void * const handle, void* const address);

    typedef enum STAUD_PlayerID_e
    {
        STAUD_PLAYER_ID_1,
        STAUD_PLAYER_ID_2,
        STAUD_PLAYER_ID_3,
        STAUD_PLAYER_ID_4,
        STAUD_PLAYER_ID_MAX,
        STAUD_PLAYER_DEFAULT
    } STAUD_AudioPlayerID_t;

    typedef enum STAUD_InterruptID_e
    {
        STAUD_INT_DECODER_1,
        STAUD_INT_DECODER_2,
        STAUD_INT_SPDIF_PLAYER,
        STAUD_INT_PCM_PLAYER_1,
        STAUD_INT_PCM_PLAYER_2,
        STAUD_INT_PCM_READER,
        STAUD_INTERRUPT_ID_MAX
    }STAUD_InterruptID_t;



   typedef struct STAUD_BufferParams_s
    {
        void* BufferBaseAddr_p;
        U32   BufferSize;
        U32   NumBuffers;

    } STAUD_BufferParams_t;

    typedef enum STAUD_BufferID_e
    {
        STAUD_BUFFER_CD1,
        STAUD_BUFFER_CD2,
        STAUD_BUFFER_PCM1,
        STAUD_BUFFER_PCM2,
        STAUD_BUFFER_ID_MAX
    } STAUD_BufferID_t;

    typedef enum STAUD_WordWidth_e
    {
        STAUD_WORD_WIDTH_8,
        STAUD_WORD_WIDTH_16,
        STAUD_WORD_WIDTH_32

    } STAUD_WordWidth_t;

    typedef enum STAUD_Karaoke_e
    {
        STAUD_KARAOKE_NONE = 0,
        STAUD_KARAOKE_AWARE = 1,
        STAUD_KARAOKE_MUSIC = 2,
        STAUD_KARAOKE_VOCAL1 = 4,
        STAUD_KARAOKE_VOCAL2 = 8,
        STAUD_KARAOKE_VOCALS = 16

    } STAUD_Karaoke_t;


	typedef enum STAUD_Object_e
	{

	/* ++NEW_DECODER_ARCHITECTURE*/

	/* These will be used to uniquely identify the chain components */

		STAUD_OBJECT_INPUT_CD0 =           (U32) (STAUD_CLASS_INPUT |
														STAUD_TYPE_INPUT_COMPRESSED |
														STAUD_INSTANCE_0),

		STAUD_OBJECT_INPUT_CD1 =           (U32) (STAUD_CLASS_INPUT |
														STAUD_TYPE_INPUT_COMPRESSED |
														STAUD_INSTANCE_1),

		STAUD_OBJECT_INPUT_CD2 =           (U32) (STAUD_CLASS_INPUT |
														STAUD_TYPE_INPUT_COMPRESSED |
														STAUD_INSTANCE_2),

		STAUD_OBJECT_INPUT_PCM0 =          (U32) (STAUD_CLASS_INPUT |
														STAUD_TYPE_INPUT_PCM |
														STAUD_INSTANCE_0),

        STAUD_OBJECT_INPUT_PCMREADER0 =     (U32) (STAUD_CLASS_INPUT |
														STAUD_TYPE_INPUT_PCM_READER |
														STAUD_INSTANCE_0),

		STAUD_OBJECT_DECODER_COMPRESSED0 = (U32) (STAUD_CLASS_DECODER |
														STAUD_TYPE_DECODER_COMPRESSED |
														STAUD_INSTANCE_0  ),

		STAUD_OBJECT_DECODER_COMPRESSED1 = (U32) (STAUD_CLASS_DECODER |
														STAUD_TYPE_DECODER_COMPRESSED |
														STAUD_INSTANCE_1  ),

	STAUD_OBJECT_DECODER_COMPRESSED2 = (U32) (STAUD_CLASS_DECODER |
														STAUD_TYPE_DECODER_COMPRESSED |
														STAUD_INSTANCE_2  ),

		STAUD_OBJECT_ENCODER_COMPRESSED0 = (U32) (STAUD_CLASS_ENCODER |
														STAUD_TYPE_ENCODER_COMPRESSED |
														STAUD_INSTANCE_0  ),

		STAUD_OBJECT_MIXER0 =				(U32) (STAUD_CLASS_MIXER |
                                           STAUD_TYPE_MIXER_PCM |
                                           STAUD_INSTANCE_0),

		STAUD_OBJECT_POST_PROCESSOR0 =      (U32) (STAUD_CLASS_POSTPROC |
                                           STAUD_TYPE_PPROC_GENERIC1 |
                                           STAUD_INSTANCE_0),

		STAUD_OBJECT_POST_PROCESSOR1 =      (U32) (STAUD_CLASS_POSTPROC |
                                           STAUD_TYPE_PPROC_GENERIC1 |
                                           STAUD_INSTANCE_1),

		STAUD_OBJECT_POST_PROCESSOR2 =      (U32) (STAUD_CLASS_POSTPROC |
                                           STAUD_TYPE_PPROC_GENERIC1 |
                                           STAUD_INSTANCE_2),

		STAUD_OBJECT_OUTPUT_PCMP0 =  (U32) (STAUD_CLASS_OUTPUT |
                                           STAUD_TYPE_OUTPUT_ANALOG |
                                           STAUD_INSTANCE_0  ),

		STAUD_OBJECT_OUTPUT_PCMP1 =  (U32) (STAUD_CLASS_OUTPUT |
                                           STAUD_TYPE_OUTPUT_ANALOG |
                                           STAUD_INSTANCE_1  ),

       STAUD_OBJECT_OUTPUT_PCMP2 =  (U32) (STAUD_CLASS_OUTPUT |
                                           STAUD_TYPE_OUTPUT_ANALOG |
                                           STAUD_INSTANCE_2  ),
       STAUD_OBJECT_OUTPUT_PCMP3 =  (U32) (STAUD_CLASS_OUTPUT |
                                           STAUD_TYPE_OUTPUT_ANALOG |
                                           STAUD_INSTANCE_3  ),

       STAUD_OBJECT_OUTPUT_HDMI_PCMP0 =  (U32) (STAUD_CLASS_OUTPUT |
                                           STAUD_TYPE_OUTPUT_ANALOG |
                                           STAUD_INSTANCE_4  ),

       STAUD_OBJECT_OUTPUT_SPDIF0 =       (U32) (STAUD_CLASS_OUTPUT |
                                           STAUD_TYPE_OUTPUT_SPDIF |
                                           STAUD_INSTANCE_0  ),
       STAUD_OBJECT_OUTPUT_HDMI_SPDIF0 =  (U32) (STAUD_CLASS_OUTPUT |
                                           STAUD_TYPE_OUTPUT_SPDIF |
                                           STAUD_INSTANCE_1  ),

	   STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER = 	(U32) (STAUD_CLASS_FORMATTER |
															STAUD_INSTANCE_0),

	   STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER = (U32) (STAUD_CLASS_FORMATTER |
															STAUD_INSTANCE_1),

       STAUD_OBJECT_FRAME_PROCESSOR				= 	(U32) (STAUD_CLASS_FORMATTER |
															STAUD_INSTANCE_2),
       STAUD_OBJECT_FRAME_PROCESSOR1			= 	(U32) (STAUD_CLASS_FORMATTER |
															STAUD_INSTANCE_3),

       STAUD_OBJECT_BLCKMGR = 		(U32)(STAUD_CLASS_BLOCKMNG|
															STAUD_INSTANCE_0)

	/* --NEW_DECODER_ARCHITECTURE */

    } STAUD_Object_t;



    typedef enum STAUD_PESParserMode_e
    {
        STAUD_PES_MODE_DISABLED,
        STAUD_PES_MODE_PES,
        STAUD_PES_MODE_ES_1,
        STAUD_PES_MODE_ES_2

    } STAUD_PESParserMode_t;

    typedef enum STAUD_SpeakerType_e
    {
        STAUD_SPEAKER_TYPE_SMALL = 1,
        STAUD_SPEAKER_TYPE_LARGE = 2

    } STAUD_SpeakerType_t;

    typedef enum STAUD_StereoMode_e
    {
        STAUD_STEREO_MODE_STEREO = 1,
        STAUD_STEREO_MODE_PROLOGIC = 2,
        STAUD_STEREO_MODE_DUAL_LEFT = 4,
        STAUD_STEREO_MODE_DUAL_RIGHT = 8,
        STAUD_STEREO_MODE_DUAL_MONO = 16,
#if defined(ST_OS20)

        STAUD_STEREO_MODE_SECOND_STEREO = 32,
#else
        STAUD_STEREO_MODE_AUTO = 32,
#endif

#if defined(ST_OS20)

        STAUD_STEREO_MODE_LR_MIX = 64,
#else
        STAUD_STEREO_MODE_SECOND_STEREO = 64,
#endif
       STAUD_STEREO_MODE_MONO   = 128

    }STAUD_StereoMode_t;

	typedef enum STAUD_Mp2EncStereoMode_e
	{
		MP2E_STEREO      = 0,
		MP2E_JOINTSTEREO = 1,
		MP2E_DUALMONO    = 2,
		MP2E_MONO        = 3
	}STAUD_Mp2EncStereoMode_t;

    typedef enum STAUD_Stop_e
    {
        STAUD_STOP_WHEN_END_OF_DATA = 1,
        STAUD_STOP_NOW = 2

    } STAUD_Stop_t;

    typedef enum STAUD_StreamContent_e
    {
        STAUD_STREAM_CONTENT_AC3		= (1 << 0),
        STAUD_STREAM_CONTENT_DTS		= (1 << 1),
        STAUD_STREAM_CONTENT_MPEG1		= (1 << 2),
        STAUD_STREAM_CONTENT_MPEG2		= (1 << 3),
        STAUD_STREAM_CONTENT_CDDA		= (1 << 4),
        STAUD_STREAM_CONTENT_PCM		= (1 << 5),
        STAUD_STREAM_CONTENT_LPCM		= (1 << 6),
        STAUD_STREAM_CONTENT_PINK_NOISE = (1 << 7),
        STAUD_STREAM_CONTENT_MP3		= (1 << 8),
        STAUD_STREAM_CONTENT_MLP		= (1 << 9),
        STAUD_STREAM_CONTENT_BEEP_TONE	= (1 << 10),
        STAUD_STREAM_CONTENT_MPEG_AAC	= (1 << 11),
        STAUD_STREAM_CONTENT_WMA		= (1 << 12),
        STAUD_STREAM_CONTENT_DV			= (1 << 13),
        STAUD_STREAM_CONTENT_CDDA_DTS	= (1 << 14),
        STAUD_STREAM_CONTENT_LPCM_DVDA  = (1 << 15),
		  STAUD_STREAM_CONTENT_HE_AAC		= (1 << 16),
		  STAUD_STREAM_CONTENT_DDPLUS		= (1 << 17),
		  STAUD_STREAM_CONTENT_WMAPROLSL	= (1 << 18),
		  STAUD_STREAM_CONTENT_ALSA 		= (1 << 19),
		  STAUD_STREAM_CONTENT_MP4_FILE	= (1 << 20),
		  STAUD_STREAM_CONTENT_ADIF		= (1 << 21),
		  STAUD_STREAM_CONTENT_RAW_AAC	= (1 << 22),
		  STAUD_STREAM_CONTENT_WAVE		=(1<<23),
	  	  STAUD_STREAM_CONTENT_AIFF		=(1<<24),
		  STAUD_STREAM_CONTENT_NULL		= (1 << 25)
         // DO NOT EDIT BELOW THIS LINE
    } STAUD_StreamContent_t;

    typedef enum STAUD_StreamType_e
    {
        STAUD_STREAM_TYPE_ES			= (1 << 0),
        STAUD_STREAM_TYPE_PES			= (1 << 1),
		STAUD_STREAM_TYPE_PES_AD		= (1 << 2),
        STAUD_STREAM_TYPE_MPEG1_PACKET	= (1 << 3),
        STAUD_STREAM_TYPE_PES_DVD		= (1 << 4),
        STAUD_STREAM_TYPE_PES_DVD_AUDIO = (1 << 5),
        STAUD_STREAM_TYPE_SPDIF			= (1 << 6),
        STAUD_STREAM_TYPE_PES_ST        = (1 << 7)
    }STAUD_StreamType_t;

    typedef enum STAUD_BroadcastProfile_e
    {
        STAUD_BROADCAST_DVB,
        STAUD_BROADCAST_DIRECTV,
        STAUD_BROADCAST_ATSC,
        STAUD_BROADCAST_DVD

    }STAUD_BroadcastProfile_t;

    typedef enum STAUD_Configuration_e
    {
        STAUD_CONFIG_DVD_COMPACT,
        STAUD_CONFIG_DVD_TYPICAL,
        STAUD_CONFIG_DVD_FULL,
        STAUD_CONFIG_DVB_COMPACT,
        STAUD_CONFIG_DVB_TYPICAL,
        STAUD_CONFIG_DVB_FULL,
        STAUD_CONFIG_DIRECTV_COMPACT,
        STAUD_CONFIG_DIRECTV_TYPICAL,
        STAUD_CONFIG_DIRECTV_FULL,
	/* ++NEW_DECODER_ARCHITECTURE */

	/* 	STAUD_OBJECT_INPUT_CD3 +
		STAUD_OBJECT_DECODER_COMPRESSED2 +
		STAUD_OBJECT_OUTPUT_PCMP0||STAUD_OBJECT_OUTPUT_SPDIF2 */
        STAUD_CONFIG_STB_COMPACT

	/* --NEW_DECODER_ARCHITECTURE */

    } STAUD_Configuration_t;

    typedef enum STAUD_CopyRightMode_e
    {
        STAUD_COPYRIGHT_MODE_NO_RESTRICTION,
        STAUD_COPYRIGHT_MODE_ONE_COPY,
        STAUD_COPYRIGHT_MODE_NO_COPY

    } STAUD_CopyRightMode_t;

     typedef enum STAUD_DACDataAlignment_e
    {
    		STAUD_DAC_DATA_ALIGNMENT_LEFT,
			STAUD_DAC_DATA_ALIGNMENT_RIGHT

    } STAUD_DACDataAlignment_t;



    typedef enum STAUD_DACDataFormat_e
    {
        STAUD_DAC_DATA_FORMAT_I2S,
        STAUD_DAC_DATA_FORMAT_STANDARD		// SONY format

    } STAUD_DACDataFormat_t;

    typedef enum STAUD_DACDataPrecision_e
    {
		STAUD_DAC_DATA_PRECISION_24BITS,
		STAUD_DAC_DATA_PRECISION_20BITS,
		STAUD_DAC_DATA_PRECISION_18BITS,
		STAUD_DAC_DATA_PRECISION_16BITS

    }STAUD_DACDataPrecision_t;

    typedef enum STAUD_DACNumBitsSubframe_e
    {
        STAUD_DAC_NBITS_SUBFRAME_32,
        STAUD_DAC_NBITS_SUBFRAME_16
    } STAUD_DACNumBitsSubframe_t;

	typedef enum STAUD_PCMReaderMemFmt_e
	{
		STAUD_PCMR_BITS_16_0,
		STAUD_PCMR_BITS_16_16
	}STAUD_PCMReaderMemFmt_t;

	typedef enum STAUD_PCMReaderRnd_e
	{
		STAUD_NO_ROUNDING_PCMR,
		STAUD_BIT16_ROUNDING_PCMR
	}STAUD_PCMReaderRnd_t;

	 typedef enum STAUD_SPDIFDataPrecision_e
    {
		STAUD_SPDIF_DATA_PRECISION_24BITS,
		STAUD_SPDIF_DATA_PRECISION_23BITS,
		STAUD_SPDIF_DATA_PRECISION_22BITS,
		STAUD_SPDIF_DATA_PRECISION_21BITS,
		STAUD_SPDIF_DATA_PRECISION_20BITS,
		STAUD_SPDIF_DATA_PRECISION_19BITS,
		STAUD_SPDIF_DATA_PRECISION_18BITS,
		STAUD_SPDIF_DATA_PRECISION_17BITS,
		STAUD_SPDIF_DATA_PRECISION_16BITS

    }STAUD_SPDIFDataPrecision_t;

	typedef enum STAUD_SPDIFEmphasis_e
	{
		STAUD_SPDIF_EMPHASIS_NOT_INDICATED,
		STAUD_SPDIF_EMPHASIS_CD_TYPE
	}STAUD_SPDIFEmphasis_t;

	typedef enum STAUD_Device_e
	{
		STAUD_DEVICE_STI8010,
		STAUD_DEVICE_STI7100,
		STAUD_DEVICE_STI7109,
		STAUD_DEVICE_STI7200,
		STAUD_DEVICE_STI5525,
		STAUD_DEVICE_STI5105,
		STAUD_DEVICE_STI5107,
		STAUD_DEVICE_STI5188,
		STAUD_DEVICE_STI5162
	}STAUD_Device_t;

    typedef enum STAUD_Effect_e
    {
        STAUD_EFFECT_NONE = (1 << 0),
        STAUD_EFFECT_SRS_3D = (1 << 1),
        STAUD_EFFECT_TRUSURROUND = (1 << 2),
        STAUD_EFFECT_SRS_TRUBASS = (1 << 3),
        STAUD_EFFECT_SRS_FOCUS = (1 << 4),
        STAUD_EFFECT_SRS_TRUSUR_XT = (1 << 5),
        STAUD_EFFECT_SRS_BYPASS		=(1<<6)

    }STAUD_Effect_t;

	typedef enum STAUD_EffectSRSParams_e
	{
		STAUD_EFFECT_FOCUS_ELEVATION = (1 << 0),
		STAUD_EFFECT_FOCUS_TWEETER_ELEVATION = (1 << 1),
		STAUD_EFFECT_TRUBASS_LEVEL = (1 << 2),
		STAUD_EFFECT_INPUT_GAIN = (1 << 3),
		STAUD_EFFECT_TSXT_HEADPHONE = (1 << 4),
		STAUD_EFFECT_TRUBASS_SIZE = (1 << 5),
		STAUD_EFFECT_TXST_MODE = (1 << 6)
	}STAUD_EffectSRSParams_t;

typedef enum STAUD_TruBass_Size_e
{
		/*For Tru Bass Size Following Values are support */
		/*Set the valus of TBsize as */
		SpeakerLFResponse40Hz=0,
		SpeakerLFResponse60Hz,
		SpeakerLFResponse100Hz,
		SpeakerLFResponse150Hz,
		SpeakerLFResponse200Hz,
		SpeakerLFResponse250Hz ,
		SpeakerLFResponse300Hz,
		SpeakerLFResponse400Hz,
		NumSpeakerTypes
} STAUD_TruBassSize_t;

typedef enum STAUD_CSiiOutPutMode_e
{
	ST_STEREO,
	ST_MULTICHANNEL
}STAUD_CSiiOutPutMode_t;

typedef enum STAUD_CSiiMode_e
{
	ST_CINEMA,
	ST_MUSIC,
	ST_MONO
}STAUD_CSiiMode_t;


	typedef enum BassMgtType
	{
		BASS_MGT_OFF=0,
		BASS_MGT_DOLBY_1,
		BASS_MGT_DOLBY_2,
		BASS_MGT_DOLBY_3,
		BASS_MGT_SIMPLIFIED,
		BASS_MGT_DOLBY_CERT,/*Bass 0*/
		BASS_MGT_Philips
		/*Do not edit below this line*/

	}STAUD_BassMgtType_t;


    typedef enum STAUD_Emphasis_e
    {
        STAUD_EMPHASIS_NONE,
        STAUD_EMPHASIS_FILTER1,
        STAUD_EMPHASIS_FILTER2

    }STAUD_Emphasis_t;

    typedef enum STAUD_MP2EncEmphasis_e
	{
		STAUD_NO_EMPHASIS,
		STAUD_EMPHASIS_50_15_US,
		STAUD_EMPHASIS_RESERVED,
		STAUD_EMPHASIS_CCITT,
		STAUD_EMPHASIS_PROCESSED_DISABLED = 4,
		STAUD_EMPHASIS_PROCESSED_50_15_US,
		STAUD_EMPHASIS_PROCESSED_RESERVED,
		STAUD_EMPHASIS_PROCESSED_CCITT
	}STAUD_MP2EncEmphasis_t;

    typedef enum STAUD_InputMode_e
    {
        STAUD_INPUT_MODE_DMA,
        STAUD_INPUT_MODE_I2S,
        STAUD_INPUT_MODE_I2S_DIRECT

    } STAUD_InputMode_t;

    typedef enum STAUD_InterfaceType_e
    {
        STAUD_INTERFACE_EMI,
        STAUD_INTERFACE_I2C

    }STAUD_InterfaceType_t;

    typedef enum STAUD_DigitalMode_e
    {
		STAUD_DIGITAL_MODE_OFF = 0,
        STAUD_DIGITAL_MODE_NONCOMPRESSED = 3,
		STAUD_DIGITAL_MODE_COMPRESSED = 4

    } STAUD_DigitalMode_t;

    typedef enum STAUD_Stereo_e
    {
        STAUD_STEREO_MONO = 1,
        STAUD_STEREO_STEREO = 2,
        STAUD_STEREO_PROLOGIC = 4,
        STAUD_STEREO_DUAL_LEFT = 8,
        STAUD_STEREO_DUAL_RIGHT = 16,
        STAUD_STEREO_DUAL_MONO = 32,
        STAUD_STEREO_SECOND_STEREO = 64

    }STAUD_Stereo_t;


    typedef enum STAUD_Fade_e
    {
        STAUD_FADE_SOFT_MUTE,
        STAUD_FADE_VOL,
        STAUD_FADE_NONE
    } STAUD_Fade_ID_t;


    typedef enum STAUD_CompMode_e
    {
        STAUD_CUSTOM_ANALOG,
        STAUD_CUSTOM_DIGITAL,
		STAUD_LINE_OUT,
		STAUD_RF_MODE
    } STAUD_CompressionMode_t;

	typedef enum STAUD_AudioCodingMode_e
    {
		STAUD_ACC_MODE20t,           /*  0 */
		STAUD_ACC_MODE10,            /*  1 */
		STAUD_ACC_MODE20,            /*  2 */
		STAUD_ACC_MODE30,            /*  3 */
		STAUD_ACC_MODE21,            /*  4 */
		STAUD_ACC_MODE31,            /*  5 */
		STAUD_ACC_MODE22,            /*  6 */
		STAUD_ACC_MODE32,            /*  7 */
		STAUD_ACC_MODE23,            /*  8 */
		STAUD_ACC_MODE33,            /*  9 */
		STAUD_ACC_MODE24,            /*  A */
		STAUD_ACC_MODE34,            /*  B */

		STAUD_ACC_MODE10p20,         /*  D */
		STAUD_ACC_MODE20p20,         /*  E */
		STAUD_ACC_MODE30p20,         /*  F */

		STAUD_ACC_MODEk10_V1V2OFF,   /* 10  */
		STAUD_ACC_MODEk10_V1ON,      /* 11 */
		STAUD_ACC_MODEk10_V2ON,      /* 12 */
		STAUD_ACC_MODEk10_V1V2ON,    /* 13 */

		STAUD_ACC_MODE_undefined_14, /* 14 */
		STAUD_ACC_MODE_undefined_15, /* 15 */
		STAUD_ACC_MODE_undefined_16, /* 16 */
		STAUD_ACC_MODE_undefined_17, /* 17 */
		STAUD_ACC_MODE_undefined_18, /* 18 */
		STAUD_ACC_MODE_undefined_19, /* 19 */

		STAUD_ACC_MODEk_AWARE,       /* 1A */ /* Default 2/0 */
		STAUD_ACC_MODEk_AWARE10,     /* 1B */
		STAUD_ACC_MODEk_AWARE20,     /* 1C */
		STAUD_ACC_MODEk_AWARE30,     /* 1D */

		STAUD_ACC_MODE_undefined_1E, /* 1E */
		STAUD_ACC_MODE_undefined_1F, /* 1F */

		STAUD_ACC_MODEk20_V1V2OFF,   /* 20 */
		STAUD_ACC_MODEk20_V1ON,      /* 21 */
		STAUD_ACC_MODEk20_V2ON,      /* 22 */
		STAUD_ACC_MODEk20_V1V2ON,    /* 23 */
		STAUD_ACC_MODEk20_V1Left,    /* 24 */
		STAUD_ACC_MODEk20_V2Right,   /* 25 */

		STAUD_ACC_MODE_undefined_26,
		STAUD_ACC_MODE_undefined_27,
		STAUD_ACC_MODE_undefined_28,
		STAUD_ACC_MODE_undefined_29,
		STAUD_ACC_MODE_undefined_2A,
		STAUD_ACC_MODE_undefined_2B,
		STAUD_ACC_MODE_undefined_2C,
		STAUD_ACC_MODE_undefined_2D,
		STAUD_ACC_MODE_undefined_2E,
		STAUD_ACC_MODE_undefined_2F,

		STAUD_ACC_MODEk30_V1V2OFF,   /* 30 */
		STAUD_ACC_MODEk30_V1ON,      /* 31 */
		STAUD_ACC_MODEk30_V2ON,      /* 32 */
		STAUD_ACC_MODEk30_V1V2ON,    /* 33 */
		STAUD_ACC_MODEk30_V1Left,    /* 34 */
		STAUD_ACC_MODEk30_V2Right,   /* 35 */

		STAUD_ACC_MODE_undefined_36,
		STAUD_ACC_MODE_undefined_37,
		STAUD_ACC_MODE_undefined_38,
		STAUD_ACC_MODE_undefined_39,
		STAUD_ACC_MODE_undefined_3A,
		STAUD_ACC_MODE_undefined_3B,
		STAUD_ACC_MODE_undefined_3C,
		STAUD_ACC_MODE_undefined_3D,
		STAUD_ACC_MODE_undefined_3E,
		STAUD_ACC_MODE_undefined_3F,

		STAUD_ACC_MODE20LPCMA = 0x40,/* 40 */
		STAUD_ACC_MODE_1p1    = 0x60,/* 60 */
		STAUD_ACC_MODE20t_LFE = 0x80,/* 80 */
		STAUD_ACC_MODE10_LFE,        /* 81 */
		STAUD_ACC_MODE20_LFE,        /* 82 */
		STAUD_ACC_MODE30_LFE,        /* 83 */
		STAUD_ACC_MODE21_LFE,        /* 84 */
		STAUD_ACC_MODE31_LFE,        /* 85 */
		STAUD_ACC_MODE22_LFE,        /* 86 */
		STAUD_ACC_MODE32_LFE,        /* 87 */
		STAUD_ACC_MODE23_LFE,        /* 88 */
		STAUD_ACC_MODE33_LFE,        /* 89 */
		STAUD_ACC_MODE24_LFE,        /* 8A */
		STAUD_ACC_MODE34_LFE,        /* 8B */
		STAUD_ACC_MODE11p20_LFE,     /* 8C */
		STAUD_ACC_MODE10p20_LFE,     /* 8D */
		STAUD_ACC_MODE20p20_LFE,     /* 8E */
		STAUD_ACC_MODE30p20_LFE,     /* 8F */
		STAUD_ACC_MODE_ALL = 0xA0,
		STAUD_ACC_MODE_ALL1,
		STAUD_ACC_MODE_ALL2,
		STAUD_ACC_MODE_ALL3,
		STAUD_ACC_MODE_ALL4,
		STAUD_ACC_MODE_ALL5,
		STAUD_ACC_MODE_ALL6,
		STAUD_ACC_MODE_ALL7,
		STAUD_ACC_MODE_ALL8,
		STAUD_ACC_MODE_ALL9,
		STAUD_ACC_MODE_ALL10,
		STAUD_ACC_MODE_ID  = 0xFF    /* FF */
    } STAUD_AudioCodingMode_t;

    /* This enum is maped from LX code */
   typedef enum STAUD_PLIIDecMode_e
    {
		STAUD_DECMODE_PROLOGIC = 0,
		STAUD_DECMODE_VIRTUAL,
		STAUD_DECMODE_MUSIC,
		STAUD_DECMODE_MOVIE,
		STAUD_DECMODE_MATRIX,
		STAUD_DECMODE_RESERVED_1,
		STAUD_DECMODE_RESERVED_2,
		STAUD_DECMODE_CUSTOM
    } STAUD_PLIIDecMode_t;

   typedef enum STAUD_PLIIOutMode_e
    {
		STAUD_PLII_MODE22,
		STAUD_PLII_MODE30,
		STAUD_PLII_MODE32
    } STAUD_PLIIOutMode_t;

   typedef enum STAUD_PLIIApply_e
    {
		STAUD_PLII_DISABLE=0,
		STAUD_PLII_ENABLE,
		STAUD_PLII_AUTO
    } STAUD_PLIIApply_t;

	/* Structures */
	typedef struct STAUD_Event_s
    {
		STAUD_Object_t  ObjectID;
#ifdef ST_OSLINUX
        U8              EventData[16];
#else
        void* EventData;
#endif

    } STAUD_Event_t;


	/* ++NEW_DECODER_ARCHITECTURE */

	/* Link list for containing all the constituents of a driver chain */
	typedef struct STAUD_DrvChainConstituent_s
	{
		/* Handle to this constituent */
		STAUD_Handle_t	Handle;

		/* Current constituents object type */
		STAUD_Object_t	Object_Instance;

		/* Pointer to next contituent in chain */
		struct STAUD_DrvChainConstituent_s	*Next_p;

	} STAUD_DrvChainConstituent_t;
	/* --NEW_DECODER_ARCHITECTURE */

	/* Driver capability */
	typedef struct STAUD_IPCapability_s
	{
		BOOL PESParserCapable;
		BOOL I2SInputCapable;
		BOOL PCMBufferPlayback;

	} STAUD_IPCapability_t;

	typedef struct STAUD_DRCapability_s
	{
		BOOL DynamicRangeCapable;
		BOOL FadingCapable;
		BOOL TempoCapable;
		BOOL RunTimeSwitch;
		BOOL DeEmphasisCapable;
		BOOL PrologicDecodingCapable;
		S32	SupportedTrickSpeeds[STAUD_MAX_TRICK_SPEEDS];
		STAUD_Stop_t SupportedStopModes;
		STAUD_StreamContent_t SupportedStreamContents;
		STAUD_StreamType_t	SupportedStreamTypes;
		STAUD_Effect_t SupportedEffects;
		STAUD_StereoMode_t SupportedStereoModes;

	} STAUD_DRCapability_t;

	typedef struct STAUD_ENCCapability_s
	{
		STAUD_StreamContent_t SupportedStreamContents;
		STAUD_StreamType_t	SupportedStreamTypes;

	} STAUD_ENCCapability_t;

	typedef struct STAUD_MXCapability_s
	{
		U8 NumberOfInputs;
		BOOL MixPerInputCapable;

	} STAUD_MXCapability_t;

	typedef struct STAUD_PPCapability_s
	{
		BOOL DownsamplingCapable;
		BOOL ChannelAttenuationCapable;
		BOOL DelayCapable;
		BOOL MultiChannelOutputCapable;
		U16 MaxAttenuation;
		U16 AttenuationPrecision;
		U16 MaxDelay;
		STAUD_Karaoke_t SupportedKaraokeModes;
		STAUD_SpeakerType_t SupportedSpeakerTypes;

	} STAUD_PPCapability_t;

	typedef struct STAUD_OPCapability_s
	{
		BOOL ChannelMuteCapable;
		S32 MaxSyncOffset;
		S32 MinSyncOffset;

	} STAUD_OPCapability_t;

	typedef struct STAUD_ReaderCapability_s
	{
		U16 NumChannels;
		BOOL I2SInputCapable;

	} STAUD_ReaderCapability_t;

	typedef struct STAUD_Capability_s
	{
		/* Module level Capabilities */
		STAUD_IPCapability_t IPCapability;
		STAUD_DRCapability_t DRCapability;
		STAUD_MXCapability_t MixerCapability;
		STAUD_PPCapability_t PostProcCapability;
		STAUD_OPCapability_t OPCapability;
		STAUD_ENCCapability_t ENCCapability;
		STAUD_ReaderCapability_t ReaderCapability;

		U8 NumberOfInputObjects;
		U8 NumberOfDecoderObjects;
		U8 NumberOfPostProcObjects;
		U8 NumberOfMixerObjects;
		U8 NumberOfOutputObjects;
		U8 NumberOfEncoderObjects;

		STAUD_Object_t InputObjects[STAUD_MAX_INPUT_OBJECTS];
		STAUD_Object_t DecoderObjects[STAUD_MAX_DECODER_OBJECTS];
		STAUD_Object_t PostProcObjects[STAUD_MAX_POST_PROC_OBJECTS];
		STAUD_Object_t MixerObjects[STAUD_MAX_MIXER_OBJECTS];
		STAUD_Object_t OutputObjects[STAUD_MAX_OUTPUT_OBJECTS];
		STAUD_Object_t EncoderObjects[STAUD_MAX_ENCODER_OBJECTS];

	} STAUD_Capability_t;

    typedef union STAUD_InterfaceParams_u
    {
        struct
        {
            void *BaseAddress_p;
            STAUD_WordWidth_t RegisterWordWidth;
        } EMIParams;
        struct
        {
            ST_DeviceName_t I2CDevice;
            U16 SlaveAddress;
        } I2CParams;

    } STAUD_InterfaceParams_t;

	typedef struct STAUD_Attenuation_s
	{
		U8 Left;
		U8 Right;
		U8 LeftSurround;
		U8 RightSurround;
		U8 Center;
		U8 Subwoofer;
		U8 CsLeft;
		U8 CsRight;
		U8 VcrLeft;
		U8 VcrRight;

	} STAUD_Attenuation_t;

	typedef struct STAUD_Delay_s
	{
		U8 Left;
		U8 Right;
		U8 LeftSurround;
		U8 RightSurround;
		U8 Center;
		U8 Subwoofer;
		U8 CsLeft;
		U8 CsRight;
		/*U8 VcrLeft;
		U8 VcrRight;*/

	} STAUD_Delay_t;

	typedef struct STAUD_DynamicRange_s
	{
		BOOL Enable;
		U8 CutFactor;
		U8 BoostFactor;

	} STAUD_DynamicRange_t;

	typedef struct STAUD_DownmixParams_s
	{
		U16				Apply;
		BOOL				stereoUpmix;
		BOOL				monoUpmix;
		BOOL				meanSurround;
		BOOL				secondStereo;
		BOOL				normalize;
		U32				normalizeTableIndex;
		BOOL				dialogEnhance;
	}STAUD_DownmixParams_t;

	typedef struct STAUD_EvtParams_s
	{
		STAUD_Handle_t Handle;
		void *Data;

	} STAUD_EvtParams_t;

	typedef struct STAUD_Equalization_s
	{
		BOOL	enable;
		U8		equalizerBand_0;
		U8		equalizerBand_1;
		U8		equalizerBand_2;
		U8		equalizerBand_3;
		U8		equalizerBand_4;
		U8		equalizerBand_5;
		U8		equalizerBand_6;
		U8		equalizerBand_7;

	} STAUD_Equalization_t;


	typedef struct STAUD_Omni_s
	{
		U8		omniEnable;
		U8		omniInputMode;
		U8		omniSurroundMode;
		U8		omniDialogMode;
		U8		omniLfeMode;
		U8		omniDialogLevel;

	} STAUD_Omni_t;

typedef struct STAUD_CircleSurrondII_s
	{

		BOOL 		Phantom;
		BOOL 		CenterFB;
		BOOL 		IS525Mode;
		BOOL		CenterRear;
		BOOL		RCenterFB;
		BOOL		TBSub;
		BOOL		TBFront;
		BOOL 		FocusEnable;
		STAUD_TruBassSize_t	TBSize;
		STAUD_CSiiOutPutMode_t OutMode;
		STAUD_CSiiMode_t		Mode;
		S16          InputGain;
		S16         TBLevel;
		S16         FocusElevation;

	} STAUD_CircleSurrondII_t;


    typedef struct STAUD_Fade_s
    {
        STAUD_Fade_ID_t FadeType;

    } STAUD_Fade_t;


	typedef struct STAUD_I2SParams_s
	{
		U32 I2SInputId;
		BOOL InvertWordClock;
		BOOL InvertBitClock;
		STAUD_DACDataFormat_t Format;
		STAUD_DACDataPrecision_t Precision;
		STAUD_DACDataAlignment_t Alignment;
		BOOL MSBFirst;

	} STAUD_I2SParams_t;

	typedef struct STAUD_PESParserParams_s
	{
		STAUD_PESParserMode_t Mode;
		U8 FilterStreamId;
		U8 FilterSubStreamId;
		U8 StreamId;
		U8 SubStreamId;

	} STAUD_PESParserParams_t;

	typedef struct STAUD_CDFIFOParams_s
	{
		STAUD_InputMode_t InputMode;
		STAUD_I2SParams_t I2SParams;
		STAUD_PESParserParams_t PESParserParams;

	} STAUD_CDFIFOParams_t;

	typedef struct STAUD_PCMDataConf_s
	{
		BOOL						InvertWordClock;
		BOOL						InvertBitClock;
		STAUD_DACDataFormat_t		Format;
		STAUD_DACDataPrecision_t	Precision;
		STAUD_DACDataAlignment_t	Alignment;
		BOOL						MSBFirst;
		U32							PcmPlayerFrequencyMultiplier;		// 384 Fs, 256 Fs etc...
		BOOL						MemoryStorageFormat16;              // Data given to PCM player is in 16/16 format
	} STAUD_PCMDataConf_t;

	typedef struct STAUD_SPDIFOutParams_s
	{
		BOOL						AutoLatency;
		U16							Latency;
		STAUD_CopyRightMode_t		CopyPermitted;
		BOOL						AutoCategoryCode;
		U8							CategoryCode;
		BOOL						AutoDTDI;
		U8							DTDI;
		STAUD_SPDIFEmphasis_t		Emphasis;
		STAUD_SPDIFDataPrecision_t  SPDIFDataPrecisionPCMMode;			// SPDIF PCM mode data precision
		U32							SPDIFPlayerFrequencyMultiplier;		// 128 Fs, 256 Fs etc...
	} STAUD_SPDIFOutParams_t;

	typedef struct STAUD_PCMReaderConf_s
	{
		BOOL								MSBFirst; //MSB first/LSB first
		STAUD_DACDataAlignment_t	Alignment; //left aligned/right aligned wrt LR clock
		BOOL 								Padding;
		BOOL								FallingSCLK; //sclk edge
		BOOL								LeftLRHigh; //LR polarity
		STAUD_DACDataPrecision_t	Precision; //Data precision
		STAUD_DACNumBitsSubframe_t	BitsPerSubFrame; // 32/16 bits per subframe
		STAUD_PCMReaderRnd_t			Rounding; // Rounding
		STAUD_PCMReaderMemFmt_t		MemFormat; // 16_0/16_16
		U32								Frequency; // Frequency of DAC

	} STAUD_PCMReaderConf_t;

    typedef struct STAUD_InitParams_s
    {
        STAUD_Device_t			DeviceType;
        STAUD_Configuration_t	Configuration;
        ST_Partition_t			*CPUPartition_p;
        U32						InterruptNumber;
        U32						InterruptLevel;
        ST_DeviceName_t			RegistrantDeviceName;
        U32						MaxOpen;
        ST_DeviceName_t			EvtHandlerName;
        STAVMEM_PartitionHandle_t AVMEMPartition;
		STAVMEM_PartitionHandle_t BufferPartition;
		BOOL					AllocateFromEMBX;
        ST_DeviceName_t			ClockRecoveryName;
        U32						DACClockToFsRatio;
        STAUD_PCMDataConf_t		PCMOutParams;
		STAUD_SPDIFOutParams_t	SPDIFOutParams;
		STAUD_DigitalMode_t		SPDIFMode;
		STAUD_PCMMode_t			PCMMode;
		STAUD_PCMReaderConf_t   PCMReaderMode;
		U8						DriverIndex;
		U8						NumChannels;
		U8						NumChPCMReader;
		BOOL                    EnableMSPPSupport;
    } STAUD_InitParams_t;


    typedef struct STAUD_DataInterfaceParams_s
    {
        void *Destination;
        U8 Holdoff;
        U8 WriteLength;

    } STAUD_DataInterfaceParams_t;

    typedef struct STAUD_PCMBufParams_s
    {
        U8 SlotSize;
        U8 DataPrecision;
        BOOL InvertWordClock;
        BOOL ByteSwapData;
        BOOL UnsignedData;
        BOOL MonoData;
        BOOL MSBFirst;
        U32 Reserved; /*Kept for backwards compatibility  */
    } STAUD_PCMBufParams_t;

	typedef struct STAUD_PCMInputParams_s
    {
        U32	Frequency;
        U32	DataPrecision;
		U8  NumChannels;
    } STAUD_PCMInputParams_t;

    typedef union STAUD_InputParams_u
    {
        STAUD_CDFIFOParams_t CDFIFOParams;
        STAUD_PCMBufParams_t PCMBufParams;

    } STAUD_InputParams_t;

    typedef struct STAUD_MixerInputParams_s
    {
        U8 MixLevel;

    } STAUD_MixerInputParams_t;

    typedef struct STAUD_OpenParams_s
    {
        U32 SyncDelay;

    } STAUD_OpenParams_t;


	typedef union STAUD_OutputParams_u
	{
		STAUD_PCMDataConf_t PCMOutParams;
		STAUD_SPDIFOutParams_t SPDIFOutParams;

	} STAUD_OutputParams_t;

	typedef struct STAUD_PCMBuffer_s
	{
		U32 Block;
		U32 StartOffset;
		U32 Length;

	} STAUD_PCMBuffer_t;

	typedef struct STAUD_GetBufferParam_s
	{
		void* StartBase;
		U32 Length;

	} STAUD_GetBufferParam_t;

	typedef struct STAUD_PTS_s
	{
		U32 Low;
		U32 High;
		BOOL Interpolated;

	} STAUD_PTS_t;

	typedef struct STAUD_SpeakerEnable_s
	{
		BOOL Left;
		BOOL Right;
		BOOL LeftSurround;
		BOOL RightSurround;
		BOOL Center;
		BOOL Subwoofer;
		BOOL CsLeft;
		BOOL CsRight;
		BOOL VcrLeft;
		BOOL VcrRight;

	} STAUD_SpeakerEnable_t;

	typedef struct STAUD_SpeakerConfiguration_s
	{
		STAUD_SpeakerType_t Front;
		STAUD_SpeakerType_t Center;
		STAUD_SpeakerType_t LeftSurround;
		STAUD_SpeakerType_t RightSurround;
		STAUD_SpeakerType_t CsLeft;
		STAUD_SpeakerType_t CsRight;
		STAUD_SpeakerType_t VcrLeft;
		STAUD_SpeakerType_t  VcrRight;
		BOOL				LFEPresent;

	} STAUD_SpeakerConfiguration_t;


	typedef enum STAUD_MPEGLayer_e
	{
		STAUD_MPEG_LAYER_I = 1,
        STAUD_MPEG_LAYER_II,
        STAUD_MPEG_LAYER_III,
        STAUD_MPEG_LAYER_NONE

	}
	STAUD_MPEG_Layer_t;




	typedef struct STAUD_StreamInfo_s
	{
		STAUD_StreamContent_t	StreamContent;
//		STAUD_StereoMode_t		AudioMode;
		U32						BitRate;
		U32						SamplingFrequency;
		STAUD_MPEG_Layer_t		LayerNumber;
		BOOL					Emphasis;
		BOOL					CopyRight;
		U8						Mode;
		U8						Bit_Rate_Code;
		U8						Bit_Stream_Mode;
		U8						Audio_Coding_Mode;
	} STAUD_StreamInfo_t;



	typedef struct STAUD_StreamParams_s
	{
		STAUD_StreamContent_t StreamContent;
		STAUD_StreamType_t StreamType;
		U32 SamplingFrequency;
		U8 StreamID;

	} STAUD_StreamParams_t;

	typedef struct STAUD_SynchroUnit_s
	{
		U32 SkipPrecision;
		U32 PausePrecision;

	} STAUD_SynchroUnit_t;

	typedef struct STAUD_TermParams_s
	{
		BOOL ForceTerminate;

	} STAUD_TermParams_t;

	typedef struct STAUD_DigitalOutputConfiguration_s
	{
		STAUD_DigitalMode_t DigitalMode;
		STAUD_CopyRightMode_t Copyright;
		U32 Latency;

	} STAUD_DigitalOutputConfiguration_t;


	typedef struct STAUD_Prepare_s
	{
		STAUD_StreamContent_t StreamContent;
		STAUD_StreamType_t StreamType;
		U32 SamplingFrequency;

	} STAUD_Prepare_t;

	typedef struct STAUD_Speaker_s
	{
		BOOL Front;
		BOOL LeftSurround;
		BOOL RightSurround;
		BOOL Center;
		BOOL Subwoofer;
		BOOL CsLeft;
		BOOL CsRight;
		BOOL VcrLeft;
		BOOL VcrRight;

	} STAUD_Speaker_t;

	typedef struct STAUD_StartParams_s
	{
		STAUD_StreamContent_t StreamContent;
		STAUD_StreamType_t StreamType;
		U32 SamplingFrequency;
		U8 StreamID;

	} STAUD_StartParams_t;

	typedef struct STAUD_ENCOutputParams_s
	{
	    BOOL    AutoOutputParam;
		U32	    BitRate;
		U32		OutputFrequency;

		union
		{
			struct
			{
				BOOL CompOn;
				BOOL CompOnSec;
			}DDConfig;

			struct
			{
				U32		bandWidth;		/* BandWidth */
				BOOL	Intensity;	/* Intensity Stereo */
				BOOL	VbrMode;		/* VBR Mode */
				U32     VbrQuality;		/* VBR Quality */
				BOOL	FullHuffman;
				BOOL	paddingMode;
				BOOL	Crc;
				BOOL	privateBit;
				BOOL	copyRightBit;
				BOOL	originalCopyBit;
				BOOL	EmphasisFlag;
				BOOL	downmix;
			}MP3Config;

			struct
			{
				STAUD_Mp2EncStereoMode_t	Mode;
				BOOL						CrcOn;
				STAUD_MP2EncEmphasis_t		Emphasis;
				BOOL						Copyrighted;
				BOOL						Original;
			}MP2Config;

			struct
			{
				unsigned long quantqual;
				int VbrOn;
			}AacConfig;

		}EncoderStreamSpecificParams;

	} STAUD_ENCOutputParams_s;

	/* This structure is used for advanced PLII settings (example for certifications) */
	typedef struct STAUD_PLIIParams_s
	{
		STAUD_PLIIApply_t 	Apply;
		STAUD_PLIIDecMode_t	DecMode;
		S8					DimensionControl; /* Valid range -3 to +3 */
		U8					CentreWidth;	   /* Valid range 0 to 7 */
		BOOL				Panaroma;
		BOOL				ChannelPolarity;
		BOOL				SurroundFilter;
		STAUD_PLIIOutMode_t  OutMode;
	} STAUD_PLIIParams_t;

	typedef struct STAUD_DRInitParams_s
	{
		BOOL SBRFlag;


	} STAUD_DRInitParams_t;
/*This is added for DTS Neo Params
	typedef enum STAUD_DTSNeoBW_e
	{
		STAUD_DTSNEOBW_16BITS,
		STAUD_DTSNEOBW_24BITS
	}STAUD_DTSNeoBW_t;
*/
	typedef enum STAUD_DTSNeoMode_e
	{
		STAUD_DTSNEOMODE_CINEMA=1,
		STAUD_DTSNEOMODE_MUSIC,
		STAUD_DTSNEOMODE_WIDE
	}STAUD_DTSNeoMode_t;

	typedef enum STAUD_DTSNeoAUXMode_e
	{
		STAUD_DTSNEOMODE_BACK,
		STAUD_DTSNEOMODE_SIDE
	}STAUD_DTSNeoAUXMode_t;

	typedef struct STAUD_DTSNeo_s
	{
		BOOL Enable;
/*		STAUD_DTSNeoBW_t NeoBW;  Neo Bit Width is not required */
		U8				    CenterGain;
		STAUD_DTSNeoMode_t	NeoMode;
		STAUD_DTSNeoAUXMode_t  NeoAUXMode;
		U8 				OutputChanels;

	}STAUD_DTSNeo_t;

	typedef struct STAUD_BTSC_s
	{
		BOOL  	Enable;
		S16		InputGain;
		S16		TXGain;
		BOOL      DualSignal;
	} STAUD_BTSC_t;

    /* Kindly Refer to 7200 DataSheet to understand the scenarios */
	typedef enum STAUD_Scenario_e
	{
		STAUD_SET_SENARIO_0 = 0,
		STAUD_SET_SENARIO_1 ,
		STAUD_SET_SENARIO_2
	}STAUD_Scenario_t;

    /* Exported Variables ------------------------------- */



    /* Exported Macros ---------------------------------- */


    /* Exported Functions ------------------------------- */

    ST_ErrorCode_t STAUD_OPSetAttenuation (STAUD_Handle_t Handle,\
    STAUD_Object_t OutputObject,\
    STAUD_Attenuation_t * Attenuation_p);

	ST_ErrorCode_t STAUD_DRSetDecodingType(STAUD_Handle_t Handle,
	STAUD_Object_t OutputObject);


    ST_ErrorCode_t STAUD_Close(const STAUD_Handle_t Handle);

	 ST_ErrorCode_t STAUD_DRConnectSource(STAUD_Handle_t Handle,
                                         STAUD_Object_t DecoderObject,
                                         STAUD_Object_t InputSource);

    ST_ErrorCode_t STAUD_ConnectSrcDst (STAUD_Handle_t Handle,
                                      STAUD_Object_t DestObject,U32 DstId,
                                      STAUD_Object_t SrcObject,U32 SrcId);

	 ST_ErrorCode_t STAUD_DisconnectInput(STAUD_Handle_t Handle,
                                      STAUD_Object_t Object,U32 InputId);

    ST_ErrorCode_t STAUD_PPDisableDownsampling(STAUD_Handle_t Handle,
											            STAUD_Object_t PostProcObject);


    ST_ErrorCode_t STAUD_PPEnableDownsampling(STAUD_Handle_t Handle,
            STAUD_Object_t PostProcObject,U32 );

	ST_ErrorCode_t STAUD_DRSetBeepToneFrequency (STAUD_Handle_t Handle,
		STAUD_Object_t DecoderObject,U32 BeepToneFrequency);

	ST_ErrorCode_t STAUD_DRGetBeepToneFrequency (STAUD_Handle_t Handle,
		STAUD_Object_t DecoderObject,U32 *BeepToneFreq_p);


    ST_ErrorCode_t STAUD_IPGetBroadcastProfile(STAUD_Handle_t Handle,
            STAUD_Object_t InputObject,
            STAUD_BroadcastProfile_t *
            BroadcastProfile_p);

    ST_ErrorCode_t STAUD_DRGetCapability(const ST_DeviceName_t DeviceName,
                                         STAUD_Object_t DecoderObject,
                                         STAUD_DRCapability_t *Capability_p);

    ST_ErrorCode_t STAUD_DRGetDynamicRangeControl(STAUD_Handle_t Handle,
            STAUD_Object_t DecoderObject,
            STAUD_DynamicRange_t *
            DynamicRange_p);

	ST_ErrorCode_t STAUD_DRSetDownMix (STAUD_Handle_t Handle,
			STAUD_Object_t DecoderObject,
			STAUD_DownmixParams_t * DownMixParam_p);

	ST_ErrorCode_t STAUD_DRGetDownMix (STAUD_Handle_t Handle,
			STAUD_Object_t DecoderObject,
			STAUD_DownmixParams_t * DownMixParam_p);

    ST_ErrorCode_t STAUD_IPGetSamplingFrequency(STAUD_Handle_t Handle,
            STAUD_Object_t InputObject,
            U32 *SamplingFrequency_p);

	ST_ErrorCode_t STAUD_DRGetStreamInfo (STAUD_Handle_t Handle,
			STAUD_Object_t DecoderObject,
			STAUD_StreamInfo_t * StreamInfo_p);



    ST_ErrorCode_t STAUD_DRGetSpeed(STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject, S32 *Speed_p);

    ST_ErrorCode_t STAUD_IPGetStreamInfo(STAUD_Handle_t Handle,
                                         STAUD_Object_t InputObject,
                                         STAUD_StreamInfo_t *StreamInfo_p);


    ST_ErrorCode_t STAUD_DRGetSyncOffset(STAUD_Handle_t Handle,
                                         STAUD_Object_t DecoderObject,
                                         S32 *Offset_p);

    ST_ErrorCode_t STAUD_DRPause(STAUD_Handle_t Handle,
                                 STAUD_Object_t DecoderObject,
                                 STAUD_Fade_t *Fade_p);

    ST_ErrorCode_t STAUD_DRPrepare(STAUD_Handle_t Handle,
                                   STAUD_Object_t DecoderObject,
                                   STAUD_StreamParams_t *StreamParams_p);

    ST_ErrorCode_t STAUD_DRResume(STAUD_Handle_t Handle,
                                  STAUD_Object_t DecoderObject);


    ST_ErrorCode_t STAUD_OPGetCapability(const ST_DeviceName_t DeviceName,
														STAUD_Object_t OutputObject,
														STAUD_OPCapability_t *Capability_p);

    ST_ErrorCode_t STAUD_OPGetParams(STAUD_Handle_t Handle,
                                     STAUD_Object_t OutputObject,
                                     STAUD_OutputParams_t *Params_p);

	ST_ErrorCode_t STAUD_OPEnableSynchronization(STAUD_Handle_t Handle,
			STAUD_Object_t OutputObject);

	ST_ErrorCode_t STAUD_OPDisableSynchronization(STAUD_Handle_t Handle,
			STAUD_Object_t OutputObject);

	ST_ErrorCode_t STAUD_OPEnableHDMIOutput (STAUD_Handle_t Handle,
											STAUD_Object_t OutputObject);
	ST_ErrorCode_t STAUD_OPDisableHDMIOutput (STAUD_Handle_t Handle,
											STAUD_Object_t OutputObject);

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
	ST_ErrorCode_t STAUD_DRSetClockRecoverySource (STAUD_Handle_t Handle,
			STAUD_Object_t Object,
			STCLKRV_Handle_t ClkSource);
#endif

    ST_ErrorCode_t STAUD_OPSetDigitalMode (STAUD_Handle_t Handle,
            STAUD_Object_t OutputObject,
            STAUD_DigitalMode_t OutputMode);

    ST_ErrorCode_t STAUD_OPSetAnalogMode (STAUD_Handle_t Handle,
            STAUD_Object_t OutputObject,
            STAUD_PCMMode_t OutputMode);
    ST_ErrorCode_t STAUD_OPSetEncDigitalOutput(const STAUD_Handle_t Handle,
            STAUD_Object_t OutputObject,
            BOOL EnableDisableEncOutput);

    ST_ErrorCode_t STAUD_OPGetDigitalMode (STAUD_Handle_t Handle,
            STAUD_Object_t OutputObject,
            STAUD_DigitalMode_t * OutputMode);
    ST_ErrorCode_t STAUD_OPGetAnalogMode (STAUD_Handle_t Handle,
            STAUD_Object_t OutputObject,
            STAUD_PCMMode_t * OutputMode);

    ST_ErrorCode_t STAUD_OPMute(STAUD_Handle_t Handle,
                                STAUD_Object_t OutputObject);

    ST_ErrorCode_t STAUD_OPSetParams(STAUD_Handle_t Handle,
                                     STAUD_Object_t OutputObject,
                                     STAUD_OutputParams_t *Params_p);

    ST_ErrorCode_t STAUD_OPUnMute(STAUD_Handle_t Handle,
                                  STAUD_Object_t OutputObject);


    ST_ErrorCode_t STAUD_DRSetDynamicRangeControl(STAUD_Handle_t Handle,
            STAUD_Object_t DecoderObject,
            STAUD_DynamicRange_t *
            DynamicRange_p);
	ST_ErrorCode_t STAUD_DRSetCompressionMode (STAUD_Handle_t Handle,
			STAUD_Object_t DecoderObject,
			STAUD_CompressionMode_t
			CompressionMode_p);

	ST_ErrorCode_t  STAUD_DRSetAudioCodingMode(STAUD_Handle_t  Handle,
				STAUD_Object_t DecoderObject,
				STAUD_AudioCodingMode_t
				AudioCodingMode_p);

    ST_ErrorCode_t STAUD_DRSetSpeed(STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject, S32 Speed);

    ST_ErrorCode_t STAUD_IPSetStreamID(STAUD_Handle_t Handle,
                                       STAUD_Object_t InputObject,
                                       U8 StreamID);

    ST_ErrorCode_t STAUD_DRSetSyncOffset(STAUD_Handle_t Handle,
                                         STAUD_Object_t DecoderObject,
                                         S32 Offset);

	ST_ErrorCode_t STAUD_OPSetLatency (STAUD_Handle_t Handle,
										STAUD_Object_t OutputObject,
										U32 Latency);



    ST_ErrorCode_t STAUD_DRStart(STAUD_Handle_t Handle,
                                 STAUD_Object_t DecoderObject,
                                 STAUD_StreamParams_t *StreamParams_p);

    ST_ErrorCode_t STAUD_DRStop(STAUD_Handle_t Handle,
                                STAUD_Object_t DecoderObject,
                                STAUD_Stop_t StopMode, STAUD_Fade_t *Fade_p);

    ST_ErrorCode_t STAUD_GetCapability(const ST_DeviceName_t DeviceName,
                                       STAUD_Capability_t *Capability_p);

    ST_Revision_t STAUD_GetRevision(void);

    ST_ErrorCode_t STAUD_Init(const ST_DeviceName_t DeviceName,
                              STAUD_InitParams_t *InitParams_p);


    ST_ErrorCode_t STAUD_IPGetCapability(const ST_DeviceName_t DeviceName,
                                    STAUD_Object_t InputObject,
                                    STAUD_IPCapability_t *Capability_p);

    ST_ErrorCode_t STAUD_IPGetParams(STAUD_Handle_t Handle,
                                     STAUD_Object_t InputObject,
                                     STAUD_InputParams_t *InputParams_p);

    ST_ErrorCode_t STAUD_IPGetInputBufferParams(STAUD_Handle_t Handle,
												STAUD_Object_t InputObject,
												STAUD_BufferParams_t* DataParams_p);

	ST_ErrorCode_t  STAUD_IPSetDataInputInterface(STAUD_Handle_t Handle,
												STAUD_Object_t InputObject,
												GetWriteAddress_t   GetWriteAddress_p,
												InformReadAddress_t InformReadAddress_p,
												void * const BufferHandle_p);

    ST_ErrorCode_t STAUD_IPQueuePCMBuffer(STAUD_Handle_t Handle,
                                    STAUD_Object_t InputObject,
                                    STAUD_PCMBuffer_t *PCMBuffer_p,
                                    U32 NumBuffers, U32 *NumQueued_p);

	ST_ErrorCode_t STAUD_IPGetPCMBuffer (STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_PCMBuffer_t * PCMBuffer_p);

	ST_ErrorCode_t STAUD_IPGetPCMBufferSize (STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									U32 * BufferSize);

    ST_ErrorCode_t STAUD_IPSetLowDataLevelEvent(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									U8 Level);

    ST_ErrorCode_t STAUD_IPSetParams(STAUD_Handle_t Handle,
                                     STAUD_Object_t InputObject,
                                     STAUD_InputParams_t *InputParams_p);

	ST_ErrorCode_t STAUD_IPSetPCMParams(STAUD_Handle_t Handle,
                                     STAUD_Object_t InputObject,
                                     STAUD_PCMInputParams_t *InputParams_p);

	ST_ErrorCode_t STAUD_IPSetBroadcastProfile (STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_BroadcastProfile_t
									BroadcastProfile);

	ST_ErrorCode_t STAUD_IPSetPCMReaderParams(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_PCMReaderConf_t *InputParams_p);

	ST_ErrorCode_t STAUD_IPGetPCMReaderParams(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_PCMReaderConf_t *InputParams_p);

	ST_ErrorCode_t STAUD_IPGetPCMReaderCapability(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_ReaderCapability_t *InputParams_p);

	ST_ErrorCode_t STAUD_IPGetBitBufferFreeSize(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									U32 * Size_p);

	ST_ErrorCode_t STAUD_IPGetSynchroUnit (STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_SynchroUnit_t *SynchroUnit_p);

	ST_ErrorCode_t STAUD_IPSkipSynchro(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									U32 Delay);

	ST_ErrorCode_t STAUD_IPPauseSynchro(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									U32 Delay);

	ST_ErrorCode_t STAUD_IPSetWMAStreamID(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									U8 WMAStreamNumber);

    ST_ErrorCode_t STAUD_MXConnectSource(STAUD_Handle_t Handle,
                                         STAUD_Object_t MixerObject,
                                         STAUD_Object_t *InputSources_p,
                                         STAUD_MixerInputParams_t *
                                         InputParams_p, U32 NumInputs);

    ST_ErrorCode_t STAUD_MXGetMixLevel (STAUD_Handle_t Handle,
						STAUD_Object_t MixerObject,
						U32 InputID,
						U16 *MixLevel_p);

    ST_ErrorCode_t STAUD_MXGetCapability(const ST_DeviceName_t DeviceName,
                                         STAUD_Object_t MixerObject,
                                         STAUD_MXCapability_t *Capability_p);

    ST_ErrorCode_t STAUD_MXSetInputParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t MixerObject,
                                          STAUD_Object_t InputSource,
                                          STAUD_MixerInputParams_t *
                                          InputParams_p);

    ST_ErrorCode_t  STAUD_MXSetMixLevel (STAUD_Handle_t Handle,
						STAUD_Object_t MixerObject,
						U32 InputID,
						U16 MixLevel);

    ST_ErrorCode_t STAUD_Open(const ST_DeviceName_t DeviceName,
														const STAUD_OpenParams_t *Params_p,
														STAUD_Handle_t *Handle_p);

    ST_ErrorCode_t STAUD_PPGetAttenuation(STAUD_Handle_t Handle,
														STAUD_Object_t PostProcObject,
														STAUD_Attenuation_t *Attenuation_p);

    ST_ErrorCode_t STAUD_PPGetDelay(STAUD_Handle_t Handle,
                                    STAUD_Object_t OutputObject,
                                    STAUD_Delay_t *Delay_p);

	ST_ErrorCode_t STAUD_PPGetEqualizationParams(STAUD_Handle_t Handle,
												 STAUD_Object_t PostProcObject,
												 STAUD_Equalization_t *Equalization_p);


    ST_ErrorCode_t STAUD_PPGetSpeakerEnable(STAUD_Handle_t Handle,
                                            STAUD_Object_t PostProcObject,
                                            STAUD_SpeakerEnable_t *SpeakerEnable_p);

    ST_ErrorCode_t STAUD_PPGetSpeakerConfig(STAUD_Handle_t Handle,
                                            STAUD_Object_t PostProcObject,
                                            STAUD_SpeakerConfiguration_t *
                                            SpeakerConfig_p);


    ST_ErrorCode_t STAUD_PPSetAttenuation(STAUD_Handle_t Handle,
                                          STAUD_Object_t PostProcObject,
                                          STAUD_Attenuation_t *Attenuation_p);

    ST_ErrorCode_t STAUD_PPSetDelay(STAUD_Handle_t Handle,
                                    STAUD_Object_t PostProcObject,
                                    STAUD_Delay_t *Delay_p);

	ST_ErrorCode_t STAUD_PPSetEqualizationParams(STAUD_Handle_t Handle,
												 STAUD_Object_t PostProcObject,
												 STAUD_Equalization_t *Equalization_p);


    ST_ErrorCode_t STAUD_PPSetSpeakerEnable(STAUD_Handle_t Handle,
                                            STAUD_Object_t OutputObject,
                                            STAUD_SpeakerEnable_t *SpeakerEnable_p);

    ST_ErrorCode_t STAUD_PPSetSpeakerConfig(STAUD_Handle_t Handle,
                                            STAUD_Object_t OutputObject,
                                            STAUD_SpeakerConfiguration_t *
                                            SpeakerConfig_p,
											STAUD_BassMgtType_t BassType);


    ST_ErrorCode_t STAUD_PPConnectSource(STAUD_Handle_t Handle,
                                         STAUD_Object_t PostProcObject,
                                         STAUD_Object_t InputSource);

    ST_ErrorCode_t STAUD_PPGetCapability(const ST_DeviceName_t DeviceName,
                                         STAUD_Object_t PostProcObject,
                                         STAUD_PPCapability_t *Capability_p);


    ST_ErrorCode_t STAUD_PPGetKaraoke(STAUD_Handle_t Handle,
                                      STAUD_Object_t PostProcObject,
                                      STAUD_Karaoke_t *Karaoke_p);


    ST_ErrorCode_t STAUD_PPSetKaraoke(STAUD_Handle_t Handle,
                                      STAUD_Object_t PostProcObject,
                                      STAUD_Karaoke_t Karaoke);


    ST_ErrorCode_t STAUD_Term(const ST_DeviceName_t DeviceName,
                              const STAUD_TermParams_t *TermParams_p);

	 /* ++ Decoder level PP */
	 ST_ErrorCode_t STAUD_DRSetDeEmphasisFilter (STAUD_Handle_t Handle,
							STAUD_Object_t PostProcObject,BOOL Emphasis);

	 ST_ErrorCode_t STAUD_DRGetDeEmphasisFilter (STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject,BOOL *Emphasis_p);

	 ST_ErrorCode_t STAUD_DRSetDolbyDigitalEx(STAUD_Handle_t Handle,
							STAUD_Object_t DecoderObject,BOOL DolbyDigitalEx);

	 ST_ErrorCode_t STAUD_DRGetDolbyDigitalEx(STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject,BOOL * DolbyDigitalEx_p);

	 ST_ErrorCode_t STAUD_DRSetEffect (STAUD_Handle_t Handle,
									STAUD_Object_t DecoderObject,STAUD_Effect_t Effect);

	 ST_ErrorCode_t STAUD_DRGetEffect (STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject, STAUD_Effect_t * Effect_p);

	 ST_ErrorCode_t STAUD_DRSetOmniParams (STAUD_Handle_t Handle,
									STAUD_Object_t DecoderObject,STAUD_Omni_t Omni);

	 ST_ErrorCode_t STAUD_DRGetOmniParams (STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject, STAUD_Omni_t * Omni_p);

	 ST_ErrorCode_t STAUD_DRSetCircleSurroundParams (STAUD_Handle_t Handle,
									STAUD_Object_t DecoderObject,STAUD_CircleSurrondII_t CSii);

	 ST_ErrorCode_t STAUD_DRGetCircleSurroundParams  (STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject, STAUD_CircleSurrondII_t * CSii_p);



	 ST_ErrorCode_t STAUD_DRSetSRSEffectParams (STAUD_Handle_t Handle,
									STAUD_Object_t DecoderObject,STAUD_EffectSRSParams_t ParamType,S16 Value);

	 ST_ErrorCode_t STAUD_DRGetSRSEffectParams (STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject, STAUD_EffectSRSParams_t ParamType,S16 *Value);

	 ST_ErrorCode_t STAUD_DRSetPrologic (STAUD_Handle_t Handle,
							STAUD_Object_t DecoderObject,BOOL Prologic);

	 ST_ErrorCode_t STAUD_DRGetPrologic(STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject,BOOL * Prologic_p);

	 ST_ErrorCode_t STAUD_DRSetPrologicAdvance (STAUD_Handle_t Handle,
							STAUD_Object_t DecoderObject, STAUD_PLIIParams_t PLIIParams);

	 ST_ErrorCode_t STAUD_DRGetPrologicAdvance (STAUD_Handle_t Handle,
							STAUD_Object_t DecoderObject, STAUD_PLIIParams_t *PLIIParams);

	 ST_ErrorCode_t STAUD_DRSetStereoMode (STAUD_Handle_t Handle,
				STAUD_Object_t DecoderObject,STAUD_StereoMode_t StereoMode);

	 ST_ErrorCode_t STAUD_DRGetStereoMode (STAUD_Handle_t Handle,
			STAUD_Object_t DecoderObject,STAUD_StereoMode_t * StereoMode_p);

	 ST_ErrorCode_t STAUD_DRGetInStereoMode (STAUD_Handle_t Handle,
			STAUD_Object_t DecoderObject,STAUD_StereoMode_t * StereoMode_p);

	 ST_ErrorCode_t STAUD_DRMute(STAUD_Handle_t Handle,
                                STAUD_Object_t OutputObject);

	 ST_ErrorCode_t STAUD_DRUnMute(STAUD_Handle_t Handle,
                                STAUD_Object_t OutputObject);

	 ST_ErrorCode_t STAUD_DRSetDDPOP (STAUD_Handle_t Handle,
											STAUD_Object_t DecoderObject,
											U32 DDPOPSetting);
	 ST_ErrorCode_t STAUD_DRGetSamplingFrequency (STAUD_Handle_t Handle,
											STAUD_Object_t InputObject,
											U32 * SamplingFrequency_p);

	 ST_ErrorCode_t STAUD_DRSetInitParams (STAUD_Handle_t Handle,
											STAUD_Object_t InputObject,
											STAUD_DRInitParams_t * InitParams);

    ST_ErrorCode_t STAUD_ENCGetCapability(const ST_DeviceName_t DeviceName,
                                         STAUD_Object_t EncoderObject,
                                         STAUD_ENCCapability_t *Capability_p);

	ST_ErrorCode_t STAUD_ENCGetOutputParams(STAUD_Handle_t Handle,
                                 STAUD_Object_t EncoderObject,
                                 STAUD_ENCOutputParams_s *EncoderOPParams_p);

	ST_ErrorCode_t STAUD_ENCSetOutputParams(STAUD_Handle_t Handle,
                                 STAUD_Object_t EncoderObject,
                                 STAUD_ENCOutputParams_s EncoderOPParams);


	ST_ErrorCode_t STAUD_ModuleStart (STAUD_Handle_t Handle,
								STAUD_Object_t ModuleObject,
								STAUD_StreamParams_t * StreamParams_p);

	ST_ErrorCode_t STAUD_GenericStart (STAUD_Handle_t Handle);




    ST_ErrorCode_t STAUD_ModuleStop (STAUD_Handle_t Handle,STAUD_Object_t ModuleObject);

    ST_ErrorCode_t STAUD_GenericStop(STAUD_Handle_t Handle);

    ST_ErrorCode_t STAUD_SetModuleAttenuation(STAUD_Handle_t Handle, STAUD_Object_t ModuleObject,
                                          STAUD_Attenuation_t *Attenuation_p);
    ST_ErrorCode_t STAUD_GetModuleAttenuation (STAUD_Handle_t Handle, STAUD_Object_t ModuleObject,
                                          STAUD_Attenuation_t * Attenuation_p);
	 /* -- Decoder Level PP */


    /* Wrapper Layer function prototypes */

#ifndef STAUD_NO_WRAPPER_LAYER

    ST_ErrorCode_t STAUD_DisableDeEmphasis(const STAUD_Handle_t Handle);

    ST_ErrorCode_t STAUD_DisableSynchronisation(const STAUD_Handle_t Handle);

    ST_ErrorCode_t STAUD_EnableDeEmphasis(const STAUD_Handle_t Handle);

    ST_ErrorCode_t STAUD_EnableSynchronisation(const STAUD_Handle_t Handle);

    ST_ErrorCode_t STAUD_GetAttenuation(const STAUD_Handle_t Handle,
                                        STAUD_Attenuation_t *Attenuation);

    ST_ErrorCode_t STAUD_GetChannelDelay(const STAUD_Handle_t Handle,
                                         STAUD_Delay_t *Delay);

    ST_ErrorCode_t STAUD_GetDigitalOutput(const STAUD_Handle_t Handle,
                                          STAUD_DigitalOutputConfiguration_t *
                                          Mode);

    ST_ErrorCode_t STAUD_GetDynamicRangeControl(const STAUD_Handle_t Handle,
            U8 *Control);

    ST_ErrorCode_t STAUD_GetEffect(const STAUD_Handle_t Handle,
                                   STAUD_Effect_t *Mode);

    ST_ErrorCode_t STAUD_GetKaraokeOutput(const STAUD_Handle_t Handle,
                                          STAUD_Karaoke_t *Mode);

    ST_ErrorCode_t STAUD_GetPrologic(const STAUD_Handle_t Handle,
                                     BOOL *PrologicState);

    ST_ErrorCode_t STAUD_GetSpeaker(const STAUD_Handle_t Handle,
                                    STAUD_Speaker_t *Speaker);

    ST_ErrorCode_t STAUD_GetSpeakerConfiguration(const STAUD_Handle_t Handle,
 												STAUD_SpeakerConfiguration_t * Speaker);

    ST_ErrorCode_t STAUD_GetStereoOutput(const STAUD_Handle_t Handle,
												STAUD_Stereo_t *Mode);

    ST_ErrorCode_t STAUD_GetSynchroUnit(STAUD_Handle_t Handle,
												STAUD_SynchroUnit_t *SynchroUnit_p);

    ST_ErrorCode_t STAUD_Mute(const STAUD_Handle_t Handle,
                              		BOOL AnalogueOutput, BOOL DigitalOutput);

    ST_ErrorCode_t STAUD_Pause(const STAUD_Handle_t Handle,
                               		STAUD_Fade_t *Fade);

    ST_ErrorCode_t STAUD_PauseSynchro(const STAUD_Handle_t Handle,
	 											U32 Unit);

    ST_ErrorCode_t STAUD_Play(const STAUD_Handle_t Handle);

    ST_ErrorCode_t STAUD_Prepare(const STAUD_Handle_t Handle,
                                 STAUD_Prepare_t *Params);

    ST_ErrorCode_t STAUD_Resume(const STAUD_Handle_t Handle);

    ST_ErrorCode_t STAUD_SetAttenuation(const STAUD_Handle_t Handle,
                                        STAUD_Attenuation_t Attenuation);

    ST_ErrorCode_t STAUD_SetChannelDelay(const STAUD_Handle_t Handle,
                                         STAUD_Delay_t Delay);

    ST_ErrorCode_t STAUD_SetDigitalOutput(const STAUD_Handle_t Handle,
                                          STAUD_DigitalOutputConfiguration_t
                                          Mode);

    ST_ErrorCode_t STAUD_SetDynamicRangeControl(const STAUD_Handle_t Handle,
            U8 Control);

    ST_ErrorCode_t STAUD_SetEffect(const STAUD_Handle_t Handle,
													const STAUD_Effect_t Mode);

    ST_ErrorCode_t STAUD_SetKaraokeOutput(const STAUD_Handle_t Handle,
													const STAUD_Karaoke_t Mode);

    ST_ErrorCode_t STAUD_SetPrologic(const STAUD_Handle_t Handle);

    ST_ErrorCode_t STAUD_SetSpeaker(const STAUD_Handle_t Handle,
                                    	const STAUD_Speaker_t Speaker);

    ST_ErrorCode_t STAUD_SetSpeakerConfiguration(const STAUD_Handle_t Handle,
	            								STAUD_SpeakerConfiguration_t Speaker,
            									STAUD_BassMgtType_t BassType);

    ST_ErrorCode_t STAUD_SetStereoOutput(const STAUD_Handle_t Handle,
                                         const STAUD_Stereo_t Mode);

    ST_ErrorCode_t STAUD_SkipSynchro(STAUD_Handle_t Handle,
                                     STAUD_AudioPlayerID_t PlayerID,
                                     U32 Delay);

    ST_ErrorCode_t STAUD_Start(const STAUD_Handle_t Handle,
                               STAUD_StartParams_t *Params);

    ST_ErrorCode_t STAUD_Stop(const STAUD_Handle_t Handle,
                              const STAUD_Stop_t StopMode,
                              STAUD_Fade_t *Fade);

    ST_ErrorCode_t STAUD_PlayerStart(const STAUD_Handle_t Handle);
    ST_ErrorCode_t STAUD_PlayerStop(const STAUD_Handle_t Handle);

   ST_ErrorCode_t STAUD_GetInputBufferParams(STAUD_Handle_t Handle,
            STAUD_AudioPlayerID_t PlayerID,
            STAUD_BufferParams_t* DataParams_p);

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
   ST_ErrorCode_t STAUD_SetClockRecoverySource (STAUD_Handle_t Handle, STCLKRV_Handle_t ClkSource);
#endif


	ST_ErrorCode_t AUD_IntHandlerInstall(int InterruptNumber,
													int InterruptLevel); /* For time being */

	ST_ErrorCode_t STAUD_PPDcRemove(STAUD_Handle_t Handle,
												STAUD_Object_t PPObject,
												STAUD_DCRemove_t *Params_p);
#endif

#ifdef ST_OSLINUX

    ST_ErrorCode_t STAUD_MapBufferToUserSpace(STAUD_BufferParams_t* DataParams_p);
    ST_ErrorCode_t STAUD_UnmapBufferFromUserSpace(STAUD_BufferParams_t* DataParams_p);

#endif

typedef struct BufferMetaData_s
{
	U32 PTS;
	U32 PTS33;
	BOOL isWMAEncASFHeaderData;
	BOOL isDummyBuffer;
} BufferMetaData_t;

typedef ST_ErrorCode_t (* FrameDeliverFunc_t)
(
	U8 * MemoryStart,
	U32 Size,
	BufferMetaData_t bufferMetaData,
	void * clientInfo
);

ST_ErrorCode_t STAUD_DPSetCallback (STAUD_Handle_t Handle,
												STAUD_Object_t DPObject,
												FrameDeliverFunc_t Func_fp,
												void * clientInfo);

ST_ErrorCode_t STAUD_MXUpdatePTSStatus(STAUD_Handle_t Handle,
												STAUD_Object_t MixerObject,
												U32 InputID,
												BOOL PTSStatus);

ST_ErrorCode_t STAUD_MXUpdateMaster(STAUD_Handle_t Handle,
												STAUD_Object_t MixerObject,
												U32 InputID,
												BOOL FreeRun);

ST_ErrorCode_t STAUD_DRGetCompressionMode (STAUD_Handle_t Handle,
												STAUD_Object_t DecoderObject,
												STAUD_CompressionMode_t *CompressionMode_p);

ST_ErrorCode_t STAUD_DRGetAudioCodingMode (STAUD_Handle_t Handle,
												STAUD_Object_t DecoderObject,
												STAUD_AudioCodingMode_t * AudioCodingMode_p);

ST_ErrorCode_t STAUD_IPHandleEOF (STAUD_Handle_t Handle,
												STAUD_Object_t InputObject);

ST_ErrorCode_t STAUD_IPGetDataInterfaceParams (STAUD_Handle_t Handle,
												STAUD_Object_t InputObject,
												STAUD_DataInterfaceParams_t *DMAParams_p);

ST_ErrorCode_t STAUD_GetBufferParam (STAUD_Handle_t Handle,
										STAUD_Object_t InputObject,
										STAUD_GetBufferParam_t *BufferParam);


  ST_ErrorCode_t STAUD_DRSetDTSNeoParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t DecoderObject,
                                          STAUD_DTSNeo_t *DTSNeo_p);

    ST_ErrorCode_t STAUD_DRGetDTSNeoParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t DecoderObject,
                                          STAUD_DTSNeo_t *DTSNeo_p);


 ST_ErrorCode_t STAUD_PPSetBTSCParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t PostProcObject,
                                          STAUD_BTSC_t *BTSC_p);

 ST_ErrorCode_t STAUD_PPGetBTSCParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t PostProcObject,
                                          STAUD_BTSC_t *BTSC_p);

ST_ErrorCode_t STAUD_SetScenario(STAUD_Handle_t Handle, STAUD_Scenario_t Scenario);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STAUDLX_H */





