/*$Id: acc_defines.h,v 1.6 2004/01/06 14:34:31 lassureg Exp $*/

#ifndef _ACC_DEFINES_H_
#define _ACC_DEFINES_H_

#ifdef WIN32
#define __restrict__
#endif

#ifdef _LXAUDIO_API_
#ifndef DP

#ifdef _DEBUG
#define DP printf
#else		/* #ifdef _DEBUG*/

#if !WIN32
void DebugPrintEmpty(const char * szFormat, ...);
#endif	/* #if !WIN32*/

#define DP while (0) DebugPrintEmpty

#endif	/* #ifdef _DEBUG*/

#endif	/* #ifndef DP    */
#endif  /* _LXAUDIO_API_ */
#define SYS_NB_MAX_CHANNELS         (SYS_NPCMCHANS + SYS_NVCRCHANS)
#define SYS_NVCRCHANS                      2
#define SYS_NPCMCHANS                      8
#define SYS_NFRONTCHANS                    3
#define SYS_NREARCHANS                     4
#define SYS_NB_MAX_BITSTREAM_INFO      5
#define SYS_NB_MAX_PCMIN               4
#define SYS_NB_MAX_PCMOUT              4


enum eCodecCmd
{
    CMD_PLAY = 0,
    CMD_MUTE = 1,
	CMD_SKIP = 2,
    CMD_PAUSE= 4
};

enum eProcessType
{
    UNDEFINED,
    PCM_PROCESS,
    ENC_PROCESS,
    TC_PROCESS,
    DEC_PROCESS
};

enum eCodecCode
{
    _UNKNOWN_CODEC_,
    _AC3_,
    _DTS_,
    _MLP_,
    _PCM_,
    _MPGa_,
    _MP2a_,
    _MP2a_MC_,
    _MP2a_DAB_,
    _MP1_L3_,
    _MP2_L3_,
    _MP25_L3_,
    _MP2a_AAC_,
    _LPCM_VIDEO_,
    _LPCM_AUDIO_,
    _DV_,
    _WMA_,
    _HDCD_,
	_OGG_VORBIS_,
	_G729AD_,
	_MAD_,
	_WMAPROLSL_,
	_SBC_,
	_DTSHD_,
	_AMRWBP_,
	_DDPLUS_,
	_MP4_AAC_,
    _LAST_CODEC_

};

enum ePacketCode
{
    _ES_,
    _PES_MP2_,
    _PES_MP1_,
    _PES_DVD_VIDEO_,
    _PES_DVD_AUDIO_,
    _IEC_61937_,
    _PES_WMA_,
	_PES_WAV_,
    _UNKNOWN_PACKET_

};

enum ePktStatus
{
    PACKET_SYNCHRONIZED,
    PACKET_NOT_SYNCHRONIZED,
    PACKET_SYNCHRO_NOT_FOUND
};

enum eMainChannelIdx
{
    MAIN_LEFT,
    MAIN_RGHT,
    MAIN_CNTR,
    MAIN_LFE,
    MAIN_LSUR,
    MAIN_RSUR,
    MAIN_CSURL,
    MAIN_CSURR,
    AUX_LEFT,
    AUX_RGHT

};
#define MAIN_MSUR MAIN_LSUR
#define MAIN_M    MAIN_CNTR
#define MAIN_V1   MAIN_LSUR
#define MAIN_V2   MAIN_RSUR

#define DEFAULT_INPUT_FILE         "../test_vectors/test.pcm"
#define DEFAULT_OUTPUT_FILE        "out.pcm"
#define DEFAULT_MAIN_OUTPUT_FILE   "main_out.pcm"
#define DEFAULT_REPORT_FILE        "../report/Lxaudio_Report.txt"
#define MAX_STRING_LENGTH        80

#define AC3_BLOCK_SIZE            256
enum eFsCode
{
    /* Range : 2^0 */
    FS48k,
    FS44k,
    FS32k,
    FS_reserved_3,
    /* Range : 2^1 */
    FS96k,
    FS88k,
    FS64k,
    FS_reserved_7,
    /* Range : 2^2 */
    FS192k,
    FS176k,
    FS128k,
    FS_reserved_11,
    /* Range : 2^3 */
    FS384k,
    FS352k,
    FS256k,
    FS_reserved_15,
    /* Range : 2^-2 */
    FS12k,
    FS11k,
    FS8k,
    FS_reserved_19,
    /* Range : 2^-1 */
    FS24k,
    FS22k,
    FS16k,
    FS_reserved_23,

    FS_reserved,
	FS_ID = 0xFF

};



#define SYS_NB_DEFAULT_ACMOD  16
#define MODE11                 0
enum eAcMode
{
    MODE20t,           /*  0 */
    MODE10,            /*  1 */
    MODE20,            /*  2 */
    MODE30,            /*  3 */
    MODE21,            /*  4 */
    MODE31,            /*  5 */
    MODE22,            /*  6 */
    MODE32,            /*  7 */
    MODE23,            /*  8 */
    MODE33,            /*  9 */
    MODE24,            /*  A */
    MODE34,            /*  B */
    MODE11p20,         /*  C */
    MODE10p20,         /*  D */
    MODE20p20,         /*  E */
    MODE30p20,         /*  F */

    MODEk10_V1V2OFF,   /* 10  */
    MODEk10_V1ON,      /* 11 */
    MODEk10_V2ON,      /* 12 */
    MODEk10_V1V2ON,    /* 13 */

    MODE_undefined_14, /* 14 */
    MODE_undefined_15, /* 15 */
    MODE_undefined_16, /* 16 */
    MODE_undefined_17, /* 17 */
    MODE_undefined_18, /* 18 */
    MODE_undefined_19, /* 19 */

    MODEk_AWARE,       /* 1A */ /* Default 2/0 */
    MODEk_AWARE10,     /* 1B */
    MODEk_AWARE20,     /* 1C */
    MODEk_AWARE30,     /* 1D */

    MODE_undefined_1E, /* 1E */
    MODE_undefined_1F, /* 1F */

    MODEk20_V1V2OFF,   /* 20 */
    MODEk20_V1ON,      /* 21 */
    MODEk20_V2ON,      /* 22 */
    MODEk20_V1V2ON,    /* 23 */
    MODEk20_V1Left,    /* 24 */
    MODEk20_V2Right,   /* 25 */

    MODE_undefined_26,
    MODE_undefined_27,
    MODE_undefined_28,
    MODE_undefined_29,
    MODE_undefined_2A,
    MODE_undefined_2B,
    MODE_undefined_2C,
    MODE_undefined_2D,
    MODE_undefined_2E,
    MODE_undefined_2F,

    MODEk30_V1V2OFF,   /* 30 */
    MODEk30_V1ON,      /* 31 */
    MODEk30_V2ON,      /* 32 */
    MODEk30_V1V2ON,    /* 33 */
    MODEk30_V1Left,    /* 34 */
    MODEk30_V2Right,   /* 35 */

    MODE_undefined_36,
    MODE_undefined_37,
    MODE_undefined_38,
    MODE_undefined_39,
    MODE_undefined_3A,
    MODE_undefined_3B,
    MODE_undefined_3C,
    MODE_undefined_3D,
    MODE_undefined_3E,
    MODE_undefined_3F,

    MODE20LPCMA = 0x40,/* 40 */
    MODE_1p1    = 0x60,/* 60 */
    MODE20t_LFE = 0x80,/* 80 */
    MODE10_LFE,        /* 81 */
    MODE20_LFE,        /* 82 */
    MODE30_LFE,        /* 83 */
    MODE21_LFE,        /* 84 */
    MODE31_LFE,        /* 85 */
    MODE22_LFE,        /* 86 */
    MODE32_LFE,        /* 87 */
    MODE23_LFE,        /* 88 */
    MODE33_LFE,        /* 89 */
    MODE24_LFE,        /* 8A */
    MODE34_LFE,        /* 8B */
    MODE11p20_LFE,     /* 8C */
    MODE10p20_LFE,     /* 8D */
    MODE20p20_LFE,     /* 8E */
    MODE30p20_LFE,     /* 8F */
    MODE_ALL = 0xA0,
    MODE_ALL1,
    MODE_ALL2,
    MODE_ALL3,
    MODE_ALL4,
    MODE_ALL5,
    MODE_ALL6,
    MODE_ALL7,
    MODE_ALL8,
    MODE_ALL9,
    MODE_ALL10,
    MODE_ID  = 0xFF    /* FF */
};

#define MODE_SECOND_STEREO MODE11p20
#define ACMOD_BIT_LFE      7
#define ACMOD_BIT_AUX      15
#define MODE_LFE           (1 << ACMOD_BIT_LFE)
#define MODE_AUX           (1 << ACMOD_BIT_AUX)
#define MODE_AUX_BASE      (MODE_AUX >> 8)
#define ACMOD_RESET_AUX(outmode)          outmode &= 0xFF
#define ACMOD_GET_AUX(acmod)              (acmod >> 8)
#define ACMOD_SET_AUX(outmode, mode_aux)  outmode = (mode_aux != 0) ? outmode | ((mode_aux | MODE_AUX_BASE ) << 8) : outmode
#define ACMOD_SET_LFE(outmode, inuse_lfe) outmode = (inuse_lfe == ACC_FALSE) ? outmode : (outmode | MODE_LFE)

#define ACMOD_CHECK_LFE(acmod)           (acmod & MODE_LFE)
#define ACMOD_CHECK_AUX(acmod)           (acmod & MODE_AUX)
/* dual modes definitions */
enum eDualMode
{
    DUAL_LR,
    DUAL_LEFT_MONO,
    DUAL_RIGHT_MONO,
    DUAL_MIX_LR_MONO
};

enum eMixOutput
{
    MIX_MAIN,
    MIX_AUX,
    MIX_MAIN_AND_AUX
};

#define BYTES_PER_32BIT_WORD	(4)
#define BYTES_PER_16BIT_WORD	(2)
enum eWordSizeCode
{
    WS32,
    WS16,
	WS8
};

#define BYTES_PER_WORD(x) ( 4 >> (x))

enum eBoolean
{
    ACC_FALSE,
    ACC_TRUE
};

enum
{
    NO,
    YES
};

enum eProcessApply
{
    ACC_DISABLED,
    ACC_ENABLED,
    ACC_AUTO
};

enum eEndianess
{
    ACC_BIG_ENDIAN16,
    ACC_LITTLE_ENDIAN16
};

enum eErrorCode
{
    ACC_EC_OK                  = 0,
    ACC_EC_ERROR               = 1,
    ACC_EC_ERROR_SYNC          = 2,
    ACC_EC_ERROR_CMP           = 4,
    ACC_EC_END_OF_FILE         = 8,
    ACC_EC_ERROR_MEM_ALLOC     = 16,
    ACC_EC_ERROR_PARSE_COMMAND = 32,
    ACC_EC_EXIT                = 64,
    ACC_EC_ERROR_CRC           = 128,
	ACC_EC_ERROR_FATAL = 1024
};

#define NB_ERROR_MSG 8

enum ePtsDts
{
    NO_PTS_DTS,
    PTS_DTS_FLAG_FORBIDDEN,
    PTS_PRESENT,
    PTS_DTS_PRESENT
};

enum ePtsValidity
{
	PTS_INVALID,
	//PTS_VALID,
	PTS_COMPUTED
};


enum eErrorStrmBuf
{
    STRMBUF_OK,
    STRMBUF_EOF
};

#define DEFAULT_FS              FS48k
#define DEFAULT_BLOCK_SIZE      AC3_BLOCK_SIZE
#define DEFAULT_NCH             2
#define DEFAULT_AUDIO_MODE      2

#define Q31_to_Q23(x)           (x>>8)
#define Q23_to_Q31(x)           (x<<8)


/* scaling factors ordering in tab downmix_value[3] */
/**** downmix scaling factors ****/

enum eAttenuation
{
    UNITY   = 0x7FFF, /* DSPf2w16(1.0) */
    M3DB    = 0x5A83, /* DSPf2w16(0.707106781) */
    M6DB    = 0x4000, /* DSPf2w16(0.5) */
    M9DB    = 0x2D41, /* DSPf2w16(0.35) */
    M4_5DB  = 0x4C1C, /* DSPf2w16(0.594603557) */
    M1_5DB  = 0x68F6, /* DSPf2w16(0.82) */
    M4_75DB = 0x4A3D  /* DSPf2w16(0.58) */

};
enum eAttenuationsIdx
{
    unity,
    m3db,
    m6db,
    m9db,
    m4_5db,
    m1_5db,
    m4_75db
};


enum eMonitorMode
{
    MONITOR_DEFAULT,
    MONITOR_USER_DEFINED
};

enum eEventMonitoring
{
    TIM_DHIT,
    TIM_DMISS,
    TIM_DMISS_CYCLES,
    TIM_PFT_ISSUED,
    TIM_PFT_HITS,
    TIM_WB_HITS,
    TIM_IHIT,
    TIM_IMISS,
    TIM_IMISS_CYCLES,
    TIM_IBUF_INVALID,
    TIM_BUNDLES,
    TIM_LDST,
    TIM_TAKEN_BR,
    TIM_NOT_TAKEN_BR,
    TIM_EXCEPTIONS,
    TIM_INTERRUPTS,
    TIM_RESERVED_EVENT
};



enum eIecValidity
{
    IEC_BURST_VALID,
    IEC_BURST_NOT_VALID
};


#ifdef _little_endian
#define FOURCC(a,b,c,d) ( ( (U32) a <<  0 ) +\
                          ( (U32) b <<  8 ) +\
                          ( (U32) c << 16 ) +\
                          ( (U32) d << 24 )  \
                        )
#else
#define FOURCC(a,b,c,d) ( ( (U32) a << 24 ) +\
                          ( (U32) b << 16 ) +\
                          ( (U32) c <<  8 ) +\
                          ( (U32) d <<  0 )  \
                        )
#endif /* _little_endian */

enum eAccPurgeCache
{
	ACC_PURGE_OFF,
    ACC_PURGE_IN,
    ACC_PURGE_OUT,
    ACC_PURGE_IO
};

#ifndef _EXTERN_C_
#ifdef __cplusplus
#define _EXTERN_C_ extern "C"
#else
#define _EXTERN_C_ extern
#endif
#endif

#ifdef _ACC_AMP_SYSTEM_

#ifdef _LX_INLINE_

#include "io_inline.c"

#else


_EXTERN_C_ short int Q31_to_Q15(int var32);

#ifdef _little_endian
_EXTERN_C_ int big_endian_int(int var32);
_EXTERN_C_ int big_endian_short_int(short int var16);

#define little_endian_int(x) x
#define little_endian_short_int(x) x

#else

_EXTERN_C_ int little_endian_int(int var32);
_EXTERN_C_ int little_endian_short_int(short int var16);

#define big_endian_int(x) x
#define big_endian_short_int(x) x

#endif /* _little_endian   */
#endif /* _LX_INLINE_      */
#endif /* _ACC_AMP_SYSTEM_ */


#endif /* _ACC_DEFINES_H_  */
