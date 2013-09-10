/*******************************************************
COPYRIGHT (C) STMicroelectronics 2005,2006,2007.  All rights reserved.

   File Name: stptiint.h
 Description: component part of stpti.h

*******************************************************/


#ifndef _STPTIINT_H_
 #define _STPTIINT_H_

/*NB STPTI_REVISION should be 30 chars long.
This is precisely this long:".............................."*/
#if defined(STAPIREF_COMPAT)
#define STPTI_REVISION      "STPTI_DVB-REL_6.8.4           "
#else
#define STPTI_REVISION      "STPTI_DVB-FAE_6.8.4           "
#endif /* STAPIREF_COMPAT */

/* PTI Architecture -------------------------------------------------------- */
/*
    Supported Architectures:

        STPTI_ARCHITECTURE_PTI4
        STPTI_ARCHITECTURE_PTI4_LITE
        STPTI_ARCHITECTURE_PTI4_SECURE_LITE
*/

#if defined(ST_5528)

    #define STPTI_ARCHITECTURE_PTI4

#else
#if defined(ST_5100) || defined(ST_5101) || defined(ST_5200) || defined(ST_5301) || defined(ST_7710) || defined(ST_7100)

    #define STPTI_ARCHITECTURE_PTI4_LITE

#else
#if defined(ST_5202) || defined(ST_7101) || defined(ST_7109)

    #define STPTI_ARCHITECTURE_PTI4_SECURE_LITE 1

#else
#if defined(ST_7105) || defined(ST_7111) || defined(ST_7141) || defined(ST_7200)

    #define STPTI_ARCHITECTURE_PTI4_SECURE_LITE 2

#else

    #error Please supply architecture information!

#endif
#endif
#endif
#endif


/* FrontEnd Architecture ---------------------------------------------------- */
#if defined(ST_5528) || defined(ST_5100) || defined(ST_5101) || defined(ST_5200) ||     defined(ST_5301) || defined(ST_7710) || defined(ST_7100) || defined(ST_5202) ||     defined(ST_7101) || defined(ST_7105) || defined(ST_7109) || defined(ST_7111) ||     defined(ST_7141)

    #define STPTI_FRONTEND_TSMERGER

#else
#if defined(ST_7200)
    /* hybrid frontend is STFE & TSGDMA */
    #define STPTI_FRONTEND_HYBRID

#else
    #error Please supply architecture information!
#endif
#endif



/* PTI Cores --------------------------------------------------------------- */
/*
    Describes the number of physical PTI cores on a given device
*/
#if defined(ST_5100) || defined(ST_5101) || defined(ST_5200) || defined(ST_5301) || defined(ST_7710) || defined(ST_7100) || defined(ST_7120) || defined(ST_7111)

    #define STPTI_CORE_COUNT    1

#else
#if defined(ST_5528) || defined(ST_5202) || defined(ST_7101) || defined(ST_7105) || defined(ST_7109) || defined(ST_7141) || defined(ST_7200)

    #define STPTI_CORE_COUNT    2

#else

    #error Please supply architecture information!

#endif
#endif


/* PTI Hardware CD FIFO / Multi DMA Support -------------------------------- */
/*
    Determines whether the device contains hardware CD FIFOs
*/
#if defined( STPTI_ARCHITECTURE_PTI4 ) || defined( STPTI_ARCHITECTURE_PTI4_LITE )

#if !defined( ST_7100 )

        #define STPTI_HARDWARE_CDFIFO_SUPPORT

    #endif

    #define STPTI_MULTI_DMA_SUPPORT

#endif


/* PTI Descrambler Powerkey Support ---------------------------------------- */
/*
    Determines whether the device supports the Powerkey descrambling method
*/
#if defined(ST_5100) || defined(ST_5101) || defined(ST_5200) || defined(ST_5202) || defined(ST_5301) || defined(ST_7100) ||     defined(ST_7101) || defined(ST_7105) || defined(ST_7109) || defined(ST_7111) || defined(ST_7141) || defined(ST_7200) ||     defined(ST_7710)

    #define STPTI_POWERKEY_SUPPORT

#endif


/* Due to caching invalidation limitation under ST20/OS20, PTI buffer caching is not used for this platform */
#if defined( ST_OS20 ) && !defined( STPTI_SUPPRESS_CACHING )
#define STPTI_SUPPRESS_CACHING
#endif




/* Includes ---------------------------------------------------------------- */

#include "stos.h"
#include "stddefs.h"

#if defined (ST_5528)
#if defined(STPTI_GPDMA_SUPPORT)
 #include "stgpdma.h"
#endif
#else
#undef STPTI_GPDMA_SUPPORT
#endif

#if defined (ST_5100) || defined (ST_5301) || defined (ST_5524) || defined (ST_5525) ||     defined (ST_7710) || defined (ST_7100) || defined (ST_7101) || defined (ST_7109) ||     defined (ST_7200)
#if defined(STPTI_FDMA_SUPPORT)
 #include "stfdma.h"
#endif
#else
#if defined(STPTI_FDMA_SUPPORT)
 #error Platform does not support FDMA.  Please undefine STPTI_FDMA_SUPPORT.
#endif
#endif

#if defined(STPTI_PCP_SUPPORT)
 #include "stpcp.h"
#endif

#include "stevt.h"

/* ------------------------------------------------------------------------- */
#define STPTI_DRIVER_ID     13
#define STPTI_DRIVER_BASE   (STPTI_DRIVER_ID << 16)
#define STPTI_ERROR_BASE     STPTI_DRIVER_BASE
#define STPTI_EVENT_BASE     STPTI_DRIVER_BASE

#define STPTI_TIMEOUT_IMMEDIATE  ((U32)0)
#define STPTI_TIMEOUT_INFINITY   ((U32)0XFFFFFFFF)


/* Enumerated types -------------------------------------------------------- */


typedef enum STPTI_ErrorType_s
{
    STPTI_ERROR_ALREADY_WAITING_ON_SLOT = (STPTI_ERROR_BASE + 1),
    STPTI_ERROR_ALT_OUT_ALREADY_IN_USE,
    STPTI_ERROR_ALT_OUT_TYPE_NOT_SUPPORTED,
    STPTI_ERROR_BUFFER_NOT_LINKED,
    STPTI_ERROR_CAROUSEL_ALREADY_ALLOCATED,
    STPTI_ERROR_CAROUSEL_ENTRY_INSERTED,
    STPTI_ERROR_CAROUSEL_LOCKED_BY_DIFFERENT_SESSION,
    STPTI_ERROR_CAROUSEL_NOT_LOCKED,
    STPTI_ERROR_CAROUSEL_OUTPUT_ONLY_ON_NULL_SLOT,
    STPTI_ERROR_COLLECT_FOR_ALT_OUT_ONLY_ON_NULL_SLOT,
    STPTI_ERROR_CORRUPT_DATA_IN_BUFFER,
    STPTI_ERROR_DESCRAMBLER_ALREADY_ASSOCIATED,
    STPTI_ERROR_DESCRAMBLER_NOT_ASSOCIATED,
    STPTI_ERROR_DESCRAMBLER_TYPE_NOT_SUPPORTED,
    STPTI_ERROR_DMA_UNAVAILABLE,
    STPTI_ERROR_ENTRY_ALREADY_INSERTED,
    STPTI_ERROR_ENTRY_IN_USE,
    STPTI_ERROR_ENTRY_NOT_IN_CAROUSEL,
    STPTI_ERROR_EVENT_QUEUE_EMPTY,
    STPTI_ERROR_EVENT_QUEUE_FULL,
    STPTI_ERROR_FILTER_ALREADY_ASSOCIATED,
    STPTI_ERROR_FLUSH_FILTERS_NOT_SUPPORTED,
    STPTI_ERROR_FUNCTION_NOT_SUPPORTED,
    STPTI_ERROR_INCOMPLETE_PES_IN_BUFFER,
    STPTI_ERROR_INCOMPLETE_SECTION_IN_BUFFER,
    STPTI_ERROR_INDEX_INVALID_ASSOCIATION,
    STPTI_ERROR_INDEX_INVALID_HANDLE,
    STPTI_ERROR_INDEX_NONE_FREE,
    STPTI_ERROR_INDEX_NOT_ASSOCIATED,
    STPTI_ERROR_INDEX_PID_ALREADY_ASSOCIATED,
    STPTI_ERROR_INDEX_SLOT_ALREADY_ASSOCIATED,
    STPTI_ERROR_INTERRUPT_QUEUE_EMPTY,
    STPTI_ERROR_INTERRUPT_QUEUE_FULL,
    STPTI_ERROR_INVALID_ALLOW_OUTPUT_TYPE,
    STPTI_ERROR_INVALID_ALTERNATE_OUTPUT_TYPE,
    STPTI_ERROR_INVALID_BUFFER_HANDLE,
    STPTI_ERROR_INVALID_CAROUSEL_ENTRY_HANDLE,
    STPTI_ERROR_INVALID_CAROUSEL_HANDLE,
    STPTI_ERROR_INVALID_CD_FIFO_ADDRESS,
    STPTI_ERROR_INVALID_DESCRAMBLER_ASSOCIATION,
    STPTI_ERROR_INVALID_DESCRAMBLER_HANDLE,
    STPTI_ERROR_INVALID_DEVICE,
    STPTI_ERROR_INVALID_FILTER_DATA,
    STPTI_ERROR_INVALID_FILTER_HANDLE,
    STPTI_ERROR_INVALID_FILTER_OPERATING_MODE,
    STPTI_ERROR_INVALID_FILTER_REPEAT_MODE,
    STPTI_ERROR_INVALID_FILTER_TYPE,
    STPTI_ERROR_INVALID_INJECTION_TYPE,
    STPTI_ERROR_INVALID_KEY_USAGE,
    STPTI_ERROR_INVALID_LOADER_OPTIONS,
    STPTI_ERROR_INVALID_PARITY,
    STPTI_ERROR_INVALID_PES_START_CODE,
    STPTI_ERROR_INVALID_PID,
    STPTI_ERROR_INVALID_SESSION_HANDLE,
    STPTI_ERROR_INVALID_SIGNAL_HANDLE,
    STPTI_ERROR_INVALID_SLOT_HANDLE,
    STPTI_ERROR_INVALID_SLOT_TYPE,
    STPTI_ERROR_INVALID_SUPPORTED_TC_CODE,
    STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION,
    STPTI_ERROR_NOT_INITIALISED,
    STPTI_ERROR_NOT_ON_SAME_DEVICE,
    STPTI_ERROR_NOT_SUPPORTED_FOR_DTV,
    STPTI_ERROR_NO_FREE_DESCRAMBLERS,
    STPTI_ERROR_NO_FREE_DMAS,
    STPTI_ERROR_NO_FREE_FILTERS,
    STPTI_ERROR_NO_FREE_SLOTS,
    STPTI_ERROR_NO_PACKET,
    STPTI_ERROR_OFFSET_EXCEEDS_PACKET_SIZE,
    STPTI_ERROR_ONLY_ONE_SIGNAL_PER_BUFFER,
    STPTI_ERROR_ONLY_ONE_SIGNAL_PER_SLOT,
    STPTI_ERROR_PID_ALREADY_COLLECTED,
    STPTI_ERROR_SET_MATCH_ACTION_NOT_SUPPORTED,
    STPTI_ERROR_SIGNAL_ABORTED,
    STPTI_ERROR_SIGNAL_EVERY_PACKET_ONLY_ON_PES_SLOT,
    STPTI_ERROR_SLOT_FLAG_NOT_SUPPORTED,
    STPTI_ERROR_SLOT_NOT_ASSOCIATED,
    STPTI_ERROR_SLOT_NOT_RAW_MODE,
    STPTI_ERROR_SLOT_NOT_SIGNAL_EVERY_PACKET,
    STPTI_ERROR_STORE_LAST_HEADER_ONLY_ON_RAW_SLOT,
    STPTI_ERROR_UNABLE_TO_ENABLE_FILTERS,
    STPTI_ERROR_USER_BUFFER_NOT_ALIGNED,
    STPTI_ERROR_WILDCARD_PID_ALREADY_SET,
    STPTI_ERROR_WILDCARD_PID_NOT_SUPPORTED,
    STPTI_ERROR_SLOT_ALREADY_LINKED,
    STPTI_ERROR_NOT_A_MANUAL_BUFFER,
    STPTI_ERROR_CAROUSEL_ENTRY_ALREADY_ALLOCATED,
    STPTI_ERROR_IOREMAPFAILED,
    STPTI_ERROR_FRONTEND_ALREADY_LINKED,
    STPTI_ERROR_FRONTEND_NOT_LINKED,
    STPTI_ERROR_INVALID_FRONTEND_TYPE,
    STPTI_ERROR_INVALID_STREAM_ID,
    STPTI_ERROR_FILTER_NOT_ASSOCIATED,
    STPTI_ERROR_END   /* Must be last. This is used to determine the number of STPTI error codes. */
}STPTI_ErrorType_t;


typedef enum STPTI_Event_s
{
    STPTI_EVENT_BUFFER_OVERFLOW_EVT = STPTI_EVENT_BASE,
    STPTI_EVENT_CAROUSEL_CYCLE_COMPLETE_EVT,
    STPTI_EVENT_CAROUSEL_ENTRY_TIMEOUT_EVT,
    STPTI_EVENT_CAROUSEL_TIMED_ENTRY_EVT,
    STPTI_EVENT_CC_ERROR_EVT,
    STPTI_EVENT_CLEAR_TO_SCRAMBLED_EVT,
    STPTI_EVENT_CWP_BLOCK_EVT,
    STPTI_EVENT_DMA_COMPLETE_EVT,
    STPTI_EVENT_INDEX_MATCH_EVT,
    STPTI_EVENT_INTERRUPT_FAIL_EVT,
    STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT,
    STPTI_EVENT_INVALID_LINK_EVT,
    STPTI_EVENT_INVALID_PARAMETER_EVT,
    STPTI_EVENT_PACKET_ERROR_EVT,
    STPTI_EVENT_PCR_RECEIVED_EVT,
    STPTI_EVENT_SCRAMBLED_TO_CLEAR_EVT,
    STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT,
    STPTI_EVENT_TC_CODE_FAULT_EVT,
    STPTI_EVENT_PES_ERROR_EVT,
    STPTI_EVENT_BUFFER_LEVEL_CHANGE_EVT,
    STPTI_EVENT_END /* Must be last. This is used to determine the number of STPTI events. */

}STPTI_Event_t;


typedef enum STPTI_CDReqLine_s
{
    STPTI_CDREQ_UNUSED = 0,
    STPTI_CDREQ_VIDEO,
    STPTI_CDREQ_AUDIO,
    STPTI_CDREQ_SUBP,
    STPTI_CDREQ_PCMO,
    STPTI_CDREQ_PCMI,
    STPTI_CDREQ_SWTS,
    STPTI_CDREQ_EXT_0,
    STPTI_CDREQ_EXT_1,
    STPTI_CDREQ_EXT_2,
    STPTI_CDREQ_EXT_3,
    STPTI_CDREQ_EXT_4,
    STPTI_CDREQ_EXT_5,
    STPTI_CDREQ_EXT_6,
    STPTI_CDREQ_VIDEO2,
    STPTI_CDREQ_AUDIO2,
    STPTI_CDREQ_AUDIO3
}STPTI_CDReqLine_t;


typedef enum STPTI_Timer_s
{
    STPTI_AUDIO_TIMER0,
    STPTI_VIDEO_TIMER0
} STPTI_Timer_t;


#ifdef STPTI_CAROUSEL_SUPPORT
typedef enum  STPTI_AllowOutput_s
{
    STPTI_ALLOW_OUTPUT_UNDEFINED,
    STPTI_ALLOW_OUTPUT_UNMATCHED_PIDS,
    STPTI_ALLOW_OUTPUT_SELECTED_SLOTS
} STPTI_AllowOutput_t;


typedef enum STPTI_CarouselEntryType_s
{
    STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE,
    STPTI_CAROUSEL_ENTRY_TYPE_TIMED
} STPTI_CarouselEntryType_t;
#endif


typedef enum STPTI_AlternateOutputType_s
{
    STPTI_ALTERNATE_OUTPUT_TYPE_NO_OUTPUT,
    STPTI_ALTERNATE_OUTPUT_TYPE_OUTPUT_AS_IS,
    STPTI_ALTERNATE_OUTPUT_TYPE_DESCRAMBLED
} STPTI_AlternateOutputType_t;

typedef enum STPTI_ArrivalTimeSource_s
{
    STPTI_ARRIVAL_TIME_SOURCE_PTI,
    STPTI_ARRIVAL_TIME_SOURCE_TSMERGER
}
STPTI_ArrivalTimeSource_t;

#if defined (STPTI_FRONTEND_HYBRID)
#define STPTI_ARRIVAL_TIME_SOURCE_STFE (STPTI_ARRIVAL_TIME_SOURCE_TSMERGER)
#endif

typedef enum STPTI_Copy_s
{
    STPTI_COPY_TRANSFER_BY_DMA,
    STPTI_COPY_TRANSFER_BY_GPDMA,
    STPTI_COPY_TRANSFER_BY_MEMCPY,
    STPTI_COPY_TRANSFER_BY_FDMA,
    STPTI_COPY_TRANSFER_BY_PCP
#if defined(ST_OSLINUX)
#ifdef MODULE
    ,
    STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL
#endif /* MODULE */
#endif /* ST_OSLINUX */
} STPTI_Copy_t;


typedef enum STPTI_DescramblerAssociation_s
{
    STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_PIDS,
    STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS
} STPTI_DescramblerAssociation_t;


typedef enum STPTI_DescramblerType_s
{
    STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_DES_ECB_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_DES_CBC_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_DES_ECB_DVS042_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_DES_CBC_DVS042_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_DES_OFB_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_DES_CTS_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_FASTI_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DVS042_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DVS042_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_MULTI2_OFB_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_MULTI2_CTS_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_ECB_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_CBC_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_ECB_DVS042_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_CBC_DVS042_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_OFB_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_CTS_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_NSA_MDD_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_NSA_MDI_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_IPTV_CSA_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_DVB_EAVS_DESCRAMBLER
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    ,
    STPTI_DESCRAMBLER_TYPE_AES_ECB_LR_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_AES_CBC_LR_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_3DES_ECB_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_3DES_CBC_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_3DES_ECB_DVS042_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_3DES_CBC_DVS042_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_3DES_OFB_DESCRAMBLER,
    STPTI_DESCRAMBLER_TYPE_3DES_CTS_DESCRAMBLER
#endif
} STPTI_DescramblerType_t;

#define STPTI_DESCRAMBLER_TYPE_DES_DESCRAMBLER      STPTI_DESCRAMBLER_TYPE_DES_ECB_DESCRAMBLER
#define STPTI_DESCRAMBLER_TYPE_MULTI2_DESCRAMBLER   STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DESCRAMBLER
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
#define STPTI_DESCRAMBLER_TYPE_AES_SYN_DESCRAMBLER  STPTI_DESCRAMBLER_TYPE_AES_ECB_LR_DESCRAMBLER
#endif

typedef enum STPTI_NSAMode_s
{
    STPTI_NSA_MODE_MDI,
    STPTI_NSA_MODE_MDD
} STPTI_NSAMode_t;



typedef enum STPTI_Device_s
{
    STPTI_DEVICE_LINK_INTERFACE,
    STPTI_DEVICE_PTI_1,
    STPTI_DEVICE_PTI_3,
    STPTI_DEVICE_PTI_4
} STPTI_Device_t;


typedef enum STPTI_DMABurstMode_s
{
    STPTI_DMA_BURST_MODE_FOUR_BYTE = 0,
    STPTI_DMA_BURST_MODE_ONE_BYTE
} STPTI_DMABurstMode_t;


typedef enum STPTI_FilterOperatingMode_s
{
    STPTI_FILTER_OPERATING_MODE_64x16,
    STPTI_FILTER_OPERATING_MODE_64x8,
    STPTI_FILTER_OPERATING_MODE_32x8,
    STPTI_FILTER_OPERATING_MODE_32xANY,
    STPTI_FILTER_OPERATING_MODE_8x8,
    STPTI_FILTER_OPERATING_MODE_16x8,
    STPTI_FILTER_OPERATING_MODE_8x16,
    STPTI_FILTER_OPERATING_MODE_16x16,
    STPTI_FILTER_OPERATING_MODE_32x16,
    STPTI_FILTER_OPERATING_MODE_48x16,
    STPTI_FILTER_OPERATING_MODE_48x8,
    STPTI_FILTER_OPERATING_MODE_ANYx8,
    STPTI_FILTER_OPERATING_MODE_ANYx16,
    STPTI_FILTER_OPERATING_MODE_NONE
} STPTI_FilterOperatingMode_t;


typedef enum STPTI_FilterRepeatMode_s
{
    STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_ONE_SHOT,
    STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED
} STPTI_FilterRepeatMode_t;


typedef enum STPTI_FilterType_s
{
    STPTI_FILTER_TYPE_EMM_FILTER,
    STPTI_FILTER_TYPE_ECM_FILTER,
#ifdef STPTI_DC2_SUPPORT
    STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER,
    STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER,
    STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER,
#endif
    STPTI_FILTER_TYPE_SECTION_FILTER,
    STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE,
    STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
    STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE,
    STPTI_FILTER_TYPE_PES_FILTER,
    STPTI_FILTER_TYPE_TSHEADER_FILTER,
    STPTI_FILTER_TYPE_PES_STREAMID_FILTER
} STPTI_FilterType_t;


#ifndef STPTI_NO_INDEX_SUPPORT
typedef enum STPTI_IndexAssociation_s
{
    STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_PIDS,
    STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS
} STPTI_IndexAssociation_t;
#endif


#ifdef STPTI_CAROUSEL_SUPPORT
typedef enum STPTI_InjectionType_s
{
    STPTI_INJECTION_TYPE_ONE_SHOT_INJECT,
    STPTI_INJECTION_TYPE_REPEATED_INJECT_AS_IS,
    STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP
} STPTI_InjectionType_t;
#endif


typedef enum STPTI_KeyParity_s
{
    STPTI_KEY_PARITY_EVEN_PARITY,
    STPTI_KEY_PARITY_ODD_PARITY,
    STPTI_KEY_PARITY_GENERIC_KEY
} STPTI_KeyParity_t;


typedef enum STPTI_KeyUsage_s
{
    STPTI_KEY_USAGE_INVALID,
    STPTI_KEY_USAGE_VALID_FOR_PES,
    STPTI_KEY_USAGE_VALID_FOR_TRANSPORT,
    STPTI_KEY_USAGE_VALID_FOR_ALL
} STPTI_KeyUsage_t;

#define STPTI_KEY_USAGE_MASK        0x0000000f

typedef enum STPTI_ScrambleState_s
{
    STPTI_SCRAMBLE_STATE_UNKNOWN,
    STPTI_SCRAMBLE_STATE_CLEAR,
    STPTI_SCRAMBLE_STATE_TRANSPORT_SCRAMBLED,
    STPTI_SCRAMBLE_STATE_PES_SCRAMBLED
}STPTI_ScrambleState_t;


typedef enum STPTI_SlotType_s
{
    STPTI_SLOT_TYPE_NULL,
    STPTI_SLOT_TYPE_SECTION,
    STPTI_SLOT_TYPE_PES,
    STPTI_SLOT_TYPE_RAW,
    STPTI_SLOT_TYPE_EMM,
    STPTI_SLOT_TYPE_ECM,
#ifdef STPTI_DC2_SUPPORT
    STPTI_SLOT_TYPE_DC2_PRIVATE,
    STPTI_SLOT_TYPE_DC2_MIXED,
#endif
    STPTI_SLOT_TYPE_PCR
} STPTI_SlotType_t;

#if defined (STPTI_FRONTEND_HYBRID)
typedef enum STPTI_StreamIDType_s
{
    STPTI_STREAM_IDTYPE_TSIN = 1,
    STPTI_STREAM_IDTYPE_SWTS,
    STPTI_STREAM_IDTYPE_ALTOUT,
    STPTI_STREAM_IDTYPE_NONE,
}STPTI_StreamIDType_t;
#endif

typedef enum STPTI_StreamID_s
{
#if defined (ST_7200)
    STPTI_STREAM_ID_TSIN0 = (STPTI_STREAM_IDTYPE_TSIN << 8),
    STPTI_STREAM_ID_TSIN1,
    STPTI_STREAM_ID_TSIN2,
    STPTI_STREAM_ID_TSIN3,
    STPTI_STREAM_ID_TSIN4,
    STPTI_STREAM_ID_TSIN5,
    STPTI_STREAM_ID_SWTS0 = (STPTI_STREAM_IDTYPE_SWTS << 8),
    STPTI_STREAM_ID_SWTS1,
    STPTI_STREAM_ID_SWTS2,
    STPTI_STREAM_ID_ALTOUT = (STPTI_STREAM_IDTYPE_ALTOUT << 8),
    STPTI_STREAM_ID_NONE = (STPTI_STREAM_IDTYPE_NONE << 8),      /*Use to deactivate a virtual PTI*/
    STPTI_STREAM_ID_NOTAGS = ((STPTI_STREAM_IDTYPE_NONE << 8) | 0xFF)
#else
    STPTI_STREAM_ID_TSIN0 = 0x20,
    STPTI_STREAM_ID_TSIN1,
    STPTI_STREAM_ID_TSIN2,
#if defined(ST_7111)
    STPTI_STREAM_ID_TSIN3,
#endif
#if defined(ST_5528)
    STPTI_STREAM_ID_TSIN3,
    STPTI_STREAM_ID_TSIN4,
    STPTI_STREAM_ID_TSIN5,
    STPTI_STREAM_ID_SWTS0,
    STPTI_STREAM_ID_SWTS1,
#endif

#if defined(ST_5100) || defined(ST_5301) || defined(ST_7710) || defined(ST_7100)
    STPTI_STREAM_ID_SWTS0,
#endif

#if defined(ST_5524) || defined(ST_5525)
    STPTI_STREAM_ID_TSIN3,
    STPTI_STREAM_ID_TSIN4,
    STPTI_STREAM_ID_TSIN5,
    STPTI_STREAM_ID_TSIN6,
    STPTI_STREAM_ID_TSIN7,
    STPTI_STREAM_ID_SWTS0,
    STPTI_STREAM_ID_SWTS1,
    STPTI_STREAM_ID_SWTS2,
    STPTI_STREAM_ID_SWTS3,
#endif

#if defined(ST_7101) || defined(ST_7109) || defined (ST_7111) || defined (ST_7141)
    STPTI_STREAM_ID_SWTS0,
    STPTI_STREAM_ID_SWTS1,
    STPTI_STREAM_ID_SWTS2,
#endif

    STPTI_STREAM_ID_ALTOUT,

    STPTI_STREAM_ID_NOTAGS = 0x80,  /* if tsmerger not configured to emit tag bytes */
    STPTI_STREAM_ID_NONE  /*Use to deactivate a virtual PTI*/
#endif /* #if defined (ST_7200) */
}STPTI_StreamID_t;

/* Note: STPTI_STREAM_ID_NOTAGS has an equivalent in TCCode,
   SessionTSMergerTagNoTag, which MUST be maintained as the same value
*/



typedef enum STPTI_SupportedTCCodes_s
{
    STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB
} STPTI_SupportedTCCodes_t;


#ifdef STPTI_SP_SUPPORT     /* New for STPTI version 6 */
typedef enum STPTI_PassageMode_e
{
    STPTI_PASSAGE_MODE_SUBSTITUTION = 1,
    STPTI_PASSAGE_MODE_INSERTION,
    STPTI_PASSAGE_MODE_INSERTDELETE
} STPTI_PassageMode_t;
#endif

#if defined (STPTI_FRONTEND_HYBRID)
typedef enum STPTI_FrontendPacketlength_e
{
    STPTI_FRONTEND_PACKET_LENGTH_DVB = 188,
}STPTI_FrontendPacketlength_t;

typedef enum STPTI_FrontendType_e
{
    STPTI_FRONTEND_TYPE_TSIN,
    STPTI_FRONTEND_TYPE_SWTS,
    STPTI_FRONTEND_TYPE_NONE
}STPTI_FrontendType_t;

typedef enum STPTI_FrontendClkRcvSrc_e
{
    STPTI_FRONTEND_CLK_RCV0,
    STPTI_FRONTEND_CLK_RCV1,
    STPTI_FRONTEND_CLK_RCV2,
    STPTI_FRONTEND_CLK_RCV_END   /* Always Leave Last in the List !! */
}STPTI_FrontendClkRcvSrc_t;


typedef struct STPTI_FrontendTSIN_s
{
    BOOL                      SyncLDEnable;
    BOOL                      SerialNotParallel;
    BOOL                      AsyncNotSync;
    BOOL                      AlignByteSOP;
    BOOL                      InvertTSClk;
    BOOL                      IgnoreErrorInByte;
    BOOL                      IgnoreErrorInPkt;
    BOOL                      IgnoreErrorAtSOP;
    BOOL                      InputBlockEnable;
    U32                       MemoryPktNum;
    STPTI_FrontendClkRcvSrc_t ClkRvSrc;
}STPTI_FrontendTSIN_t;

typedef struct STPTI_FrontendSWTS_s
{
    U32     PaceBitRate;
}STPTI_FrontendSWTS_t;

typedef struct STPTI_FrontendParams_s
{
    STPTI_FrontendPacketlength_t PktLength;
    union
    {
        STPTI_FrontendTSIN_t TSIN;
        STPTI_FrontendSWTS_t SWTS;
    }u;
}STPTI_FrontendParams_t;

typedef struct STPTI_FrontendStatus_s
{
    BOOL                Lock;
    BOOL                FifoOverflow;
    BOOL                BufferOverflow;
    BOOL                OutOfOrderRP;
    BOOL                PktOverflow;
    BOOL                DMAPointerError;
    BOOL                DMAOverflow;
    U8                  ErrorPackets;
    U8                  ShortPackets;
    STPTI_StreamID_t    StreamID;
}STPTI_FrontendStatus_t;

typedef struct STPTI_FrontendBuffer_s
{
    U8 *                             BufferBase_p;
    U32                              BufferSize;
    struct STPTI_FrontendBuffer_st * NextNode_p;
}STPTI_FrontendBuffer_t;

typedef struct STPTI_FrontendSWTSNode_s
{
    U8 * Data_p;
    U32  SizeOfData;
}
STPTI_FrontendSWTSNode_t;

#define STPTI_SWTS_MIN_RATE  (1000000)
#define STPTI_SWTS_MAX_RATE  (100000000)

#endif /* #if defined (STPTI_FRONTEND_HYBRID) ... #endif */

/* Handles and misc types -------------------------------------------------- */


typedef U32 STPTI_Handle_t;

typedef STPTI_Handle_t STPTI_Buffer_t;
typedef STPTI_Handle_t STPTI_Descrambler_t;
typedef STPTI_Handle_t STPTI_Filter_t;
typedef STPTI_Handle_t STPTI_Frontend_t;
typedef STPTI_Handle_t STPTI_Signal_t;
typedef STPTI_Handle_t STPTI_Slot_t;
#ifdef STPTI_CAROUSEL_SUPPORT
typedef STPTI_Handle_t STPTI_Carousel_t;
typedef STPTI_Handle_t STPTI_CarouselEntry_t;
#endif
#ifndef STPTI_NO_INDEX_SUPPORT
typedef STPTI_Handle_t STPTI_Index_t;
#endif

#ifndef _STPTI_DEVICE_PTR_T_
 #define _STPTI_DEVICE_PTR_T_
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
 #pragma ST_device(STPTI_DevicePtr_t)
 #endif
 typedef volatile U32 *STPTI_DevicePtr_t;
#endif

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(stpti_device_word_t)
#endif
typedef volatile U32 stpti_device_word_t;


typedef U16 STPTI_Pid_t;
typedef U8 STPTI_AlternateOutputTag_t;
typedef U8 STPTI_TSPacket_t[188];       /* DVB packet */


/* data structures --------------------------------------------------------- */



typedef struct STPTI_Capability_s
{
    STPTI_Device_t                 Device;
    U32                            TCDeviceAddress;
    STPTI_DescramblerAssociation_t Association;
    ST_DeviceName_t                EventHandlerName;

    U16                            NumberSlots;
    U16                            NumberDMAs;
    U16                            NumberKeys;
    U16                            NumberSectionFilters;

    BOOL                           AlternateOutputSupport;
    BOOL                           AutomaticSectionFiltering;
#ifdef STPTI_CAROUSEL_SUPPORT
    BOOL                           CarouselOutputSupport;
#endif
    BOOL                           ManualSectionFiltering;
    BOOL                           PidWildcardingSupport;
    BOOL                           RawStreamDescrambling;
    STPTI_SupportedTCCodes_t       TCCodes;
} STPTI_Capability_t;


#ifdef STPTI_CAROUSEL_SUPPORT

typedef struct STPTI_CarouselEntryEventData_s
{
    U32                   PrivateData;
    STPTI_CarouselEntry_t EntryHandle;
    U32                   BufferCount;
} STPTI_CarouselEntryEventData_t;


typedef struct STPTI_CarouselEntryTimeoutEventData_s
{
    U32                   PrivateData;
    STPTI_CarouselEntry_t EntryHandle;
} STPTI_CarouselEntryTimeoutEventData_t;


typedef struct STPTI_CarouselTimedEntryEventData_s
{
    U32                   PrivateData;
    STPTI_CarouselEntry_t EntryHandle;
} STPTI_CarouselTimedEntryEventData_t;

#endif  /* STPTI_CAROUSEL_SUPPORT */

typedef struct STPTI_DMAParams_s
{
    U32                  Destination;
    U8                   Holdoff;
    U8                   WriteLength;
    STPTI_CDReqLine_t    CDReqLine;
    U32                 *DMAUsed;
    STPTI_DMABurstMode_t BurstSize;
} STPTI_DMAParams_t;


typedef struct STPTI_TimeStamp_s
{
    U32 LSW;
    U8  Bit32;
} STPTI_TimeStamp_t;


#define STPTI_MAX_START_CODES_SUPPORTED 7

#ifndef STPTI_NO_INDEX_SUPPORT

typedef union STPTI_IndexDefinition_s
{
    struct
    {
        U32 PayloadUnitStartIndicator:1;
        U32 ScramblingChangeToClear:1;
        U32 ScramblingChangeToEven:1;
        U32 ScramblingChangeToOdd:1;

        U32 DiscontinuityIndicator:1;
        U32 RandomAccessIndicator:1;
        U32 PriorityIndicator:1;
        U32 PCRFlag:1;

        U32 OPCRFlag:1;
        U32 SplicingPointFlag:1;
        U32 TransportPrivateDataFlag:1;
        U32 AdaptationFieldExtensionFlag:1;

        U32 FirstRecordedPacket:1;
        U32 MPEGStartCode:1;
        U32 IFrameStart:1; /* STRASP generated */
        U32 PFrameStart:1; /* STRASP generated */

        U32 BFrameStart:1; /* STRASP generated */
        U32 IFieldStart:1; /* STRASP generated */
        U32 PFieldStart:1; /* STRASP generated */
        U32 BFieldStart:1; /* STRASP generated */

        U32 IDRFrameStart:1; /* STRASP generated */
        U32 IDRFieldStart:1; /* STRASP generated */
        U32 SequenceStart:1; /* STRASP generated */
        U32 Reserved:7;
        U32 Reserved2:2;
    } s;
    U32 word;
} STPTI_IndexDefinition_t;


typedef struct STPTI_StartCode_s
{
    U8  MPEGStartCodeValue;
    U8  MPEGStartCodeOffsetInToTSPacket;
    U16 AuxillaryData;
} STPTI_StartCode_t;


typedef struct STPTI_IndexEventData_s
{
    STPTI_IndexDefinition_t IndexBitMap;
    U32                     PacketCount;
    U32                     BufferPacketAddress;
    U32                     RecordBufferPacketCount;

    size_t                  OffsetIntoBuffer;
    STPTI_Buffer_t          BufferHandle;

    STPTI_TimeStamp_t       PCRTime;
    U16                     PCRTimeExtension;

    STPTI_TimeStamp_t       PacketArrivalTime;
    U16                     ArrivalTimeExtension;

    U8                      NumberStartCodesDetected;
    STPTI_StartCode_t       MPEGStartCode[STPTI_MAX_START_CODES_SUPPORTED];
} STPTI_IndexEventData_t;


#define STPTI_MPEG_START_CODE_00       0x0001
#define STPTI_MPEG_START_CODE_B0       0x0002
#define STPTI_MPEG_START_CODE_B1       0x0004
#define STPTI_MPEG_START_CODE_B2       0x0008
#define STPTI_MPEG_START_CODE_B3       0x0010
#define STPTI_MPEG_START_CODE_B4       0x0020
#define STPTI_MPEG_START_CODE_B5       0x0040
#define STPTI_MPEG_START_CODE_B6       0x0080
#define STPTI_MPEG_START_CODE_B7       0x0100
#define STPTI_MPEG_START_CODE_B8       0x0200
#define STPTI_MPEG_START_CODE_SPECIAL  0x0400

#define STPTI_MPEG_START_CODE_H264     0x80000000
#define STPTI_MPEG_START_CODE_MPEG2    0x00000000

#endif  /* STPTI_NO_INDEX_SUPPORT */




typedef struct STPTI_DMAEventData_s
{
    U32 Destination;
} STPTI_DMAEventData_t;


typedef struct STPTI_PCREventData_s
{
    BOOL              DiscontinuityFlag;
    STPTI_TimeStamp_t PCRArrivalTime;
    U16               PCRArrivalTimeExtension;
    STPTI_TimeStamp_t PCRBase;
    U16               PCRExtension;
} STPTI_PCREventData_t;


typedef struct STPTI_ErrEventData_s
{
    U32 DMANumber;
    U32 BytesRemaining;
} STPTI_ErrEventData_t;

typedef struct STPTI_BufferLevelData_s
{
    STPTI_Buffer_t BufferHandle;
    U32 FreeLength;
} STPTI_BufferLevelData_t;

typedef struct STPTI_EventData_s
{
    union
    {
        STPTI_DMAEventData_t           DMAEventData;
        STPTI_PCREventData_t           PCREventData;
        STPTI_ErrEventData_t           ErrEventData;
#ifndef STPTI_NO_INDEX_SUPPORT
        STPTI_IndexEventData_t         IndexEventData;
#endif
#ifdef STPTI_CAROUSEL_SUPPORT
        STPTI_CarouselEntryEventData_t CarouselEntryEventData;
#endif
        STPTI_BufferLevelData_t        BufferLevelData;
    } u;

    STPTI_Slot_t SlotHandle;

} STPTI_EventData_t;


typedef struct STPTI_PESFilter_s
{
    STPTI_TimeStamp_t *PTSValueMax_p;
    STPTI_TimeStamp_t *DTSValueMax_p;
    STPTI_TimeStamp_t *PTSValueMin_p;
    STPTI_TimeStamp_t *DTSValueMin_p;

    U8                 PesHeaderFlags;
    U8                 PesHeaderFlagsMask;
    U8                 PesTrickModeFlags;
    U8                 PesTrickModeFlagsMask;
} STPTI_PESFilter_t;


typedef struct STPTI_PESStreamIDFilter_s
{
    U8 StreamID;
}STPTI_PESStreamIDFilter_t;


typedef struct STPTI_SectionFilter_s
{
    BOOL DiscardOnCrcError;
    U8  *ModePattern_p;
    BOOL NotMatchMode;
    BOOL OverrideSSIBit;
} STPTI_SectionFilter_t;


typedef struct  STPTI_TransportFilter_s
{
    void *dummy_p;                               /* No additional parameters required */
} STPTI_TransportFilter_t;




#ifdef STPTI_DC2_SUPPORT

typedef struct STPTI_DC2Multicast16Filter_s
{
    U16 FilterBytes0;
}STPTI_DC2Multicast16Filter_t;


typedef struct STPTI_DC2Multicast32Filter_s
{
    U32 FilterBytes0;
}STPTI_DC2Multicast32Filter_t;


typedef struct STPTI_DC2Multicast48Filter_s
{
    U32 FilterBytes0;
    U32 FilterBytes1:16;
    U32 Padding:16;
}STPTI_DC2Multicast48Filter_t;

#endif  /* STPTI_DC2_SUPPORT */


typedef struct STPTI_FilterData_s
{
    STPTI_FilterType_t       FilterType;
    STPTI_FilterRepeatMode_t FilterRepeatMode;
    BOOL                     InitialStateEnabled;
    U8                      *FilterBytes_p;
    U8                      *FilterMasks_p;

    union
    {
        STPTI_SectionFilter_t        SectionFilter;
        STPTI_TransportFilter_t      TransportFilter;
        STPTI_PESFilter_t            PESFilter;
#ifdef STPTI_DC2_SUPPORT
        STPTI_DC2Multicast16Filter_t DC2Multicast16Filter;
        STPTI_DC2Multicast32Filter_t DC2Multicast32Filter;
        STPTI_DC2Multicast48Filter_t DC2Multicast48Filter;
#endif
        STPTI_PESStreamIDFilter_t    PESStreamIDFilter;
    } u;

} STPTI_FilterData_t;


#if defined(STPTI_GPDMA_SUPPORT)
typedef struct STPTI_GPDMAParams_s
{
    STGPDMA_Handle_t Handle;
    STGPDMA_Timing_t TimingModel;
    U32 EventLine;
    U32 TimeOut;
} STPTI_GPDMAParams_t;
#endif

#if defined(STPTI_PCP_SUPPORT)
typedef struct STPTI_PCPParams_s
{
    STPCP_Handle_t Handle;
    STPCP_EngineMode_t EngineModeEncrypt;
} STPTI_PCPParams_t;
#endif

typedef struct STPTI_InitParams_s
{
    STPTI_Device_t                 Device;
    STPTI_DevicePtr_t              TCDeviceAddress_p;
    ST_ErrorCode_t               (*TCLoader_p) (STPTI_DevicePtr_t, void *);
    STPTI_SupportedTCCodes_t       TCCodes;
    STPTI_DescramblerAssociation_t DescramblerAssociation;
    ST_Partition_t                *Partition_p;
    ST_Partition_t                *NCachePartition_p;
    ST_DeviceName_t                EventHandlerName;
    S32                            EventProcessPriority;
    S32                            InterruptProcessPriority;
#ifdef STPTI_CAROUSEL_SUPPORT
    S32                            CarouselProcessPriority;
    STPTI_CarouselEntryType_t      CarouselEntryType;
#endif
    S32                            InterruptLevel;
    S32                            InterruptNumber;
    U32                            SyncLock;
    U32                            SyncDrop;
    STPTI_FilterOperatingMode_t    SectionFilterOperatingMode;
#ifndef STPTI_NO_INDEX_SUPPORT
    STPTI_IndexAssociation_t       IndexAssociation;
    S32                            IndexProcessPriority;
#endif
    STPTI_StreamID_t               StreamID;
    U16                            NumberOfSlots;
    U8                             AlternateOutputLatency;
    BOOL                           DiscardOnCrcError;
    STPTI_ArrivalTimeSource_t      PacketArrivalTimeSource;
    U8                             NumberOfSectionFilters;
} STPTI_InitParams_t;


typedef struct STPTI_OpenParams_s
{
    ST_Partition_t *DriverPartition_p;
    ST_Partition_t *NonCachedPartition_p;
} STPTI_OpenParams_t;


typedef struct STPTI_SlotFlags_s
{
    BOOL SignalOnEveryTransportPacket;
    BOOL CollectForAlternateOutputOnly;
    BOOL AlternateOutputInjectCarouselPacket;
    BOOL StoreLastTSHeader;
    BOOL InsertSequenceError;
    BOOL OutPesWithoutMetadata;
    BOOL ForcePesLengthToZero;
    BOOL AppendSyncBytePrefixToRawData;
    BOOL SoftwareCDFifo;
} STPTI_SlotFlags_t;


typedef struct STPTI_SlotData_s
{
    STPTI_SlotType_t SlotType;
    STPTI_SlotFlags_t SlotFlags;
} STPTI_SlotData_t;


typedef union STPTI_SlotOrPid_s
{
    STPTI_Slot_t Slot;
    STPTI_Pid_t Pid;
} STPTI_SlotOrPid_t;

typedef enum STPTI_Feature_e
{
    STPTI_FEATURE_MINIMUM_ENTRY,

    /* Buffer Features......................*/

    /* Descrambler Features.................*/

    /* Filter Features......................*/

    /* Session Features.....................*/

    /* Signal Features......................*/

    /* Slot Features........................*/
    STPTI_SLOT_FEATURE_DLNA_FORMATTED_OUTPUT,
    STPTI_SLOT_FEATURE_OUTPUT_WITH_TAG_BYTES,

    /* Carousel Features....................*/
    STPTI_CAROUSEL_FEATURE_DLNA_FORMATTED_OUTPUT,

    /* Index Features.......................*/

    STPTI_FEATURE_MAXIMUM_ENTRY
}STPTI_Feature_t;

typedef struct STPTI_FeatureInfo_s
{
    STPTI_Feature_t Feature;
    void* Params_p;
} STPTI_FeatureInfo_t;

typedef struct STPTI_TermParams_s
{
    BOOL ForceTermination;
} STPTI_TermParams_t;

#ifdef STPTI_DEBUG_SUPPORT

typedef struct
{
    U32 Status;
    U32 SlotHandle;
    U32 BufferHandle;
    U32 PTIBaseAddress;
    U32 InterruptCount;
    U32 DMANumber;
    U32 Time;
} STPTI_DebugInterruptStatus_t;

typedef struct
{
    U32 BufferOverflowEventCount;
    U32 CarouselCycleCompleteEventCount;
    U32 CarouselEntryTimeoutEventCount;
    U32 CarouselTimedEntryEventCount;
    U32 CCErrorEventCount;
    U32 ClearChangeToScrambledEventCount;
    U32 DMACompleteEventCount;
    U32 IndexMatchEventCount;
    U32 InterruptFailEventCount;
    U32 InvalidDescramblerKeyEventCount;
    U32 InvalidLinkEventCount;
    U32 InvalidParameterEventCount;
    U32 PacketErrorEventCount;
    U32 PCRReceivedEventCount;
    U32 ScrambledChangeToClearEventCount;
    U32 SectionDiscardedOnCRCCheckEventCount;
    U32 TCCodeFaultEventCount;
    U32 PacketSignalCount;
    U32 InterruptBufferOverflowCount;
    U32 InterruptBufferErrorCount;
    U32 InterruptStatusCorruptionCount;
    U32 PESErrorEventCount;
} STPTI_DebugStatistics_t;

#endif

/* Macros ------------------------------------------------------------------ */


#define STPTI_NullHandle()  0x000FC000
#define STPTI_WildCardPid() 0xffff
#define STPTI_NegativePid() 0xbeef
#define STPTI_InvalidPid()  0xe000



/* Exported Variables ------------------------------------------------------ */

#ifdef STPTI_DEBUG_SUPPORT

extern STPTI_DebugStatistics_t      DebugStatistics;
extern STPTI_DebugInterruptStatus_t *DebugInterruptStatus;
extern int  IntInfoCapacity;
extern BOOL IntInfoStart;
extern int  IntInfoIndex;

#endif


/* Functions --------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif


ST_ErrorCode_t STPTI_AlternateOutputSetDefaultAction(STPTI_Handle_t Handle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag);

ST_ErrorCode_t STPTI_BufferAllocate(STPTI_Handle_t Handle, U32 RequiredSize, U32 NumberOfPacketsInMultiPacket, STPTI_Buffer_t * BufferHandle_p);
ST_ErrorCode_t STPTI_BufferAllocateManual(STPTI_Handle_t Handle,U8* Base_p, U32 RequiredSize, U32 NumberOfPacketsInMultiPacket, STPTI_Buffer_t * BufferHandle_p);

#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode_t STPTI_BufferClearGPDMAParams(STPTI_Buffer_t BufferHandle);
#endif

#if defined(STPTI_PCP_SUPPORT)
ST_ErrorCode_t STPTI_BufferClearPCPParams(STPTI_Buffer_t BufferHandle);
#endif

ST_ErrorCode_t STPTI_BufferDeallocate(STPTI_Buffer_t BufferHandle);

ST_ErrorCode_t STPTI_BufferExtractData(STPTI_Buffer_t BufferHandle, U32 Offset, U32 NumBytesToExtract,
                                       U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p,
                                       U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemCpy);


#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t STPTI_BufferExtractPartialPesPacketData(STPTI_Buffer_t BufferHandle, BOOL *PayloadUnitStart_p,
                                                       BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p,
                                                       U16 *DataLength_p);

ST_ErrorCode_t STPTI_BufferExtractPesPacketData(STPTI_Buffer_t BufferHandle, U8 *PesFlags_p, U8 *TrickModeFlags_p,
                                                U32 *PESPacketLength_p, STPTI_TimeStamp_t * PTSValue_p,
                                                STPTI_TimeStamp_t * DTSValue_p);
#else
#define STPTI_BufferExtractPartialPesPacketData( a, b, c, d, e )        ST_NO_ERROR
#define STPTI_BufferExtractPesPacketData( a, b, c, d, e, f )        ST_NO_ERROR
#endif /* #if defined (STPTI_BSL_SUPPORT ) */

ST_ErrorCode_t STPTI_BufferExtractSectionData(STPTI_Buffer_t BufferHandle, STPTI_Filter_t MatchedFilterList[],
                                              U16 MaxLengthofFilterList, U16 *NumOfFilterMatches_p, BOOL *CRCValid_p,
                                              U32 *SectionHeader_p);

ST_ErrorCode_t STPTI_BufferExtractTSHeaderData(STPTI_Buffer_t BufferHandle, U32 *TSHeader_p);

ST_ErrorCode_t STPTI_BufferFlush(STPTI_Buffer_t BufferHandle);

ST_ErrorCode_t STPTI_BufferLevelSignalEnable( STPTI_Buffer_t BufferHandle, U32 BufferLevelThreshold );

ST_ErrorCode_t STPTI_BufferLevelSignalDisable( STPTI_Buffer_t BufferHandle );

ST_ErrorCode_t STPTI_BufferGetFreeLength(STPTI_Buffer_t BufferHandle, U32 *FreeLength_p);

ST_ErrorCode_t STPTI_BufferGetWritePointer(STPTI_Buffer_t BufferHandle, void **Write_p);

ST_ErrorCode_t STPTI_BufferLinkToCdFifo(STPTI_Buffer_t Buffer, STPTI_DMAParams_t * CdFifoParams_p);

ST_ErrorCode_t STPTI_BufferPacketCount(STPTI_Buffer_t BufferHandle, U32 *Count_p);

ST_ErrorCode_t STPTI_BufferRead(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);

ST_ErrorCode_t STPTI_BufferReadNBytes(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p,
                                STPTI_Copy_t DmaOrMemcpy, U32 BytesToCopy);

#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t STPTI_BufferReadPartialPesPacket(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                                U8 *Destination1_p, U32 DestinationSize1, BOOL *PayloadUnitStart_p,
                                                BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p, U32 *DataSize_p,
                                                STPTI_Copy_t DmaOrMemcpy);

ST_ErrorCode_t STPTI_BufferReadPes(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                   U8 *Destination1_p, U32 DestinationSize1, U32 *Datasize_p, STPTI_Copy_t DmaOrMemcpy);
#else
#define STPTI_BufferReadPartialPesPacket( a, b, c, d, e, f, g, h, i, j )    ST_NO_ERROR
#define STPTI_BufferReadPes( a, b, c, d, e, f, g  )    ST_NO_ERROR
#endif /* #if defined (STPTI_BSL_SUPPORT ) */

ST_ErrorCode_t STPTI_BufferReadSection(STPTI_Buffer_t BufferHandle, STPTI_Filter_t MatchedFilterList[],
                                       U32 MaxLengthofFilterList, U32 *NumOfFilterMatches_p, BOOL *CRCValid_p,
                                       U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p,
                                       U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);

ST_ErrorCode_t STPTI_BufferReadTSPacket(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                        U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p,
                                        STPTI_Copy_t DmaOrMemcpy);


#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode_t STPTI_BufferSetGPDMAParams(STPTI_Buffer_t BufferHandle, STPTI_GPDMAParams_t GPDMAParams);
#endif

#if defined(STPTI_PCP_SUPPORT)
ST_ErrorCode_t STPTI_BufferSetPCPParams(STPTI_Buffer_t BufferHandle, STPTI_PCPParams_t PCPParams);
#endif

ST_ErrorCode_t STPTI_BufferSetMultiPacket(STPTI_Buffer_t BufferHandle, U32 NumberOfPacketsInMultiPacket);

ST_ErrorCode_t STPTI_BufferSetOverflowControl(STPTI_Buffer_t BufferHandle, BOOL IgnoreOverflow);

ST_ErrorCode_t STPTI_BufferSetReadPointer(STPTI_Buffer_t BufferHandle, void* Read_p);

ST_ErrorCode_t STPTI_BufferTestForData(STPTI_Buffer_t BufferHandle, U32 *BytesInBuffer_p);

ST_ErrorCode_t STPTI_BufferUnLink(STPTI_Buffer_t Buffer);


#ifdef STPTI_CAROUSEL_SUPPORT
ST_ErrorCode_t STPTI_CarouselAllocate(STPTI_Handle_t Handle, STPTI_Carousel_t * Carousel_p);

ST_ErrorCode_t STPTI_CarouselDeallocate(STPTI_Carousel_t CarouselHandle);

ST_ErrorCode_t STPTI_CarouselEntryAllocate(STPTI_Handle_t Handle, STPTI_CarouselEntry_t * CarouselEntryHandle_p);

ST_ErrorCode_t STPTI_CarouselEntryDeallocate(STPTI_CarouselEntry_t CarouselEntryHandle);

ST_ErrorCode_t STPTI_CarouselEntryLoad(STPTI_CarouselEntry_t CarouselEntry, STPTI_TSPacket_t Packet, U8 FromByte);

ST_ErrorCode_t STPTI_CarouselGetCurrentEntry(STPTI_Carousel_t CarouselHandle, STPTI_CarouselEntry_t *Entry_p, U32 *PrivateData_p);

ST_ErrorCode_t STPTI_CarouselInsertEntry(STPTI_Carousel_t CarouselHandle, STPTI_AlternateOutputTag_t AlternateOutputTag,
                                         STPTI_InjectionType_t InjectionType, STPTI_CarouselEntry_t Entry);

ST_ErrorCode_t STPTI_CarouselInsertTimedEntry(STPTI_Carousel_t CarouselHandle, U8 *TSPacket_p, U8 CCValue,
                                              STPTI_InjectionType_t InjectionType, STPTI_TimeStamp_t MinOutputTime,
                                              STPTI_TimeStamp_t MaxOutputTime, BOOL EventOnOutput, BOOL EventOnTimeout,
                                              U32 PrivateData, STPTI_CarouselEntry_t Entry, U8 FromByte);

ST_ErrorCode_t STPTI_CarouselLinkToBuffer(STPTI_Carousel_t CarouselHandle, STPTI_Buffer_t BufferHandle);

ST_ErrorCode_t STPTI_CarouselLock(STPTI_Carousel_t CarouselHandle);

ST_ErrorCode_t STPTI_CarouselRemoveEntry(STPTI_Carousel_t CarouselHandle, STPTI_CarouselEntry_t Entry);

ST_ErrorCode_t STPTI_CarouselSetAllowOutput(STPTI_Carousel_t CarouselHandle, STPTI_AllowOutput_t OutputAllowed);

ST_ErrorCode_t STPTI_CarouselSetBurstMode(STPTI_Carousel_t CarouselHandle, U32 PacketTimeMs, U32 CycleTimeMs);

ST_ErrorCode_t STPTI_CarouselUnlinkBuffer(STPTI_Carousel_t CarouselHandle, STPTI_Buffer_t BufferHandle);

ST_ErrorCode_t STPTI_CarouselUnLock(STPTI_Carousel_t CarouselHandle);
#endif

ST_ErrorCode_t STPTI_Close(STPTI_Handle_t Handle);

ST_ErrorCode_t STPTI_SetSystemKey(ST_DeviceName_t DeviceName, U8 *Data);

#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t STPTI_DescramblerAllocate(STPTI_Handle_t Handle, STPTI_Descrambler_t * DescramblerObject_p, STPTI_DescramblerType_t DescramblerType);
ST_ErrorCode_t STPTI_DescramblerAssociate(STPTI_Descrambler_t DescramblerHandle, STPTI_SlotOrPid_t SlotOrPid);
ST_ErrorCode_t STPTI_DescramblerDeallocate(STPTI_Descrambler_t DescramblerHandle);
ST_ErrorCode_t STPTI_DescramblerDisassociate(STPTI_Descrambler_t DescramblerHandle, STPTI_SlotOrPid_t SlotOrPid);
ST_ErrorCode_t STPTI_DescramblerSet(STPTI_Descrambler_t DescramblerHandle, STPTI_KeyParity_t Parity, STPTI_KeyUsage_t Usage, U8 *Data_p);
ST_ErrorCode_t STPTI_DescramblerSetType(STPTI_Descrambler_t DescramblerHandle, STPTI_DescramblerType_t DescramblerType);
ST_ErrorCode_t STPTI_DescramblerSetSVP(STPTI_Descrambler_t DescHandle, BOOL Clear_SCB, STPTI_NSAMode_t mode);
#else
#define STPTI_DescramblerAllocate( a , b , c )      ST_NO_ERROR
#define STPTI_DescramblerAssociate( a , b )         ST_NO_ERROR
#define STPTI_DescramblerDeallocate( a )            ST_NO_ERROR
#define STPTI_DescramblerDisassociate( a , b )      ST_NO_ERROR
#define STPTI_DescramblerSet( a , b , c , d )       ST_NO_ERROR
#define STPTI_DescramblerSetType( a , b )           ST_NO_ERROR
#define STPTI_DescramblerSetSVP( a , b , c )        ST_NO_ERROR
#endif /* #if defined ( STPTI_BSL_SUPPORT ) */

ST_ErrorCode_t STPTI_FilterAllocate(STPTI_Handle_t Handle, STPTI_FilterType_t FilterType,
                                    STPTI_Filter_t * FilterObject_p);

ST_ErrorCode_t STPTI_FilterAssociate(STPTI_Filter_t FilterHandle, STPTI_Slot_t SlotHandle);

ST_ErrorCode_t STPTI_FilterDeallocate(STPTI_Filter_t FilterHandle);

ST_ErrorCode_t STPTI_FilterDisassociate(STPTI_Filter_t FilterHandle, STPTI_Slot_t SlotHandle);

ST_ErrorCode_t STPTI_FilterSet(STPTI_Filter_t FilterHandle, STPTI_FilterData_t * FilterData_p);

ST_ErrorCode_t STPTI_FilterSetMatchAction(STPTI_Filter_t FilterHandle, STPTI_Filter_t FiltersToEnable[], U16 NumOfFiltersToEnable);

ST_ErrorCode_t STPTI_FiltersFlush(STPTI_Buffer_t BufferHandle, STPTI_Filter_t Filters[], U16 NumOfFiltersToFlush);

ST_ErrorCode_t STPTI_GetCapability(ST_DeviceName_t DeviceName, STPTI_Capability_t * DeviceCapability_p);

ST_ErrorCode_t STPTI_GetCurrentPTITimer(ST_DeviceName_t DeviceName, STPTI_TimeStamp_t * TimeStamp);

ST_ErrorCode_t STPTI_GetGlobalPacketCount(ST_DeviceName_t DeviceName, U32 *Count_p);

#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t STPTI_GetInputPacketCount(ST_DeviceName_t DeviceName, U16 *Count_p);
#else
#define STPTI_GetInputPacketCount( a , b )    ST_NO_ERROR
#endif

ST_ErrorCode_t STPTI_GetPacketArrivalTime(STPTI_Handle_t Handle, STPTI_TimeStamp_t * ArrivalTime_p, U16 *ArrivalTimeExtension_p);

ST_ErrorCode_t STPTI_GetPacketErrorCount(ST_DeviceName_t DeviceName, U32 *Count_p);

ST_ErrorCode_t STPTI_GetPresentationSTC(ST_DeviceName_t DeviceName, STPTI_Timer_t Timer, STPTI_TimeStamp_t * TimeStamp);

ST_ErrorCode_t STPTI_HardwarePause(ST_DeviceName_t DeviceName);

ST_ErrorCode_t STPTI_HardwareFIFOLevels(ST_DeviceName_t DeviceName, BOOL *Overflow, U16 *InputLevel, U16 *AltLevel, U16 *HeaderLevel);

ST_ErrorCode_t STPTI_HardwareReset(ST_DeviceName_t DeviceName);

ST_ErrorCode_t STPTI_HardwareResume(ST_DeviceName_t DeviceName);


#ifndef STPTI_NO_INDEX_SUPPORT
ST_ErrorCode_t STPTI_IndexAllocate(STPTI_Handle_t SessionHandle, STPTI_Index_t * IndexHandle_p);

ST_ErrorCode_t STPTI_IndexAssociate(STPTI_Index_t IndexHandle, STPTI_SlotOrPid_t SlotOrPid);

ST_ErrorCode_t STPTI_IndexDeallocate(STPTI_Index_t IndexHandle);

ST_ErrorCode_t STPTI_IndexDisassociate(STPTI_Index_t IndexHandle, STPTI_SlotOrPid_t SlotOrPid);

ST_ErrorCode_t STPTI_IndexSet(STPTI_Index_t IndexHandle, STPTI_IndexDefinition_t IndexMask, U32 MPEGStartCode, U32 MPEGStartCodeMode );

ST_ErrorCode_t STPTI_IndexChain(STPTI_Index_t *IndexHandles, U32 NumberOfHandles);

ST_ErrorCode_t STPTI_IndexReset(STPTI_Index_t IndexHandle);

ST_ErrorCode_t STPTI_IndexStart(ST_DeviceName_t DeviceName);

ST_ErrorCode_t STPTI_IndexStop(ST_DeviceName_t DeviceName);

#endif /* ifndef STPTI_NO_INDEX_SUPPORT */


ST_ErrorCode_t STPTI_Init(ST_DeviceName_t DeviceName, const STPTI_InitParams_t * InitParams_p);

ST_ErrorCode_t STPTI_ModifyGlobalFilterState(STPTI_Filter_t FiltersToDisable[], U16 NumOfFiltersToDisable,
                                             STPTI_Filter_t  FiltersToEnable[], U16 NumOfFiltersToEnable);

ST_ErrorCode_t STPTI_ModifySyncLockAndDrop(ST_DeviceName_t DeviceName, U8 SyncLock, U8 SyncDrop);

ST_ErrorCode_t STPTI_Open(ST_DeviceName_t DeviceName, const STPTI_OpenParams_t * OpenParams_p, STPTI_Handle_t * Handle_p);

ST_ErrorCode_t STPTI_ObjectEnableFeature(STPTI_Handle_t ObjectHandle, STPTI_FeatureInfo_t FeatureInfo);

ST_ErrorCode_t STPTI_ObjectDisableFeature(STPTI_Handle_t ObjectHandle, STPTI_Feature_t Feature);

ST_ErrorCode_t STPTI_PidQuery(ST_DeviceName_t Device, STPTI_Pid_t Pid, STPTI_Slot_t * Slot_p);

ST_ErrorCode_t STPTI_SetStreamID(ST_DeviceName_t Device, STPTI_StreamID_t StreamID);

ST_ErrorCode_t STPTI_SetDiscardParams(ST_DeviceName_t Device, U8 NumberOfDiscardBytes);

#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t STPTI_SignalAbort(STPTI_Signal_t SignalHandle);

ST_ErrorCode_t STPTI_SignalAllocate(STPTI_Handle_t Handle, STPTI_Signal_t * SignalHandle_p);

ST_ErrorCode_t STPTI_SignalAssociateBuffer(STPTI_Signal_t SignalHandle, STPTI_Buffer_t BufferHandle);

ST_ErrorCode_t STPTI_SignalDeallocate(STPTI_Signal_t SignalHandle);

ST_ErrorCode_t STPTI_SignalDisassociateBuffer(STPTI_Signal_t SignalHandle, STPTI_Buffer_t BufferHandle);

ST_ErrorCode_t STPTI_SignalWaitBuffer(STPTI_Signal_t SignalHandle, STPTI_Buffer_t * BufferHandle_p, U32 TimeoutMS);
#else
#define STPTI_SignalAbort( a )         ST_NO_ERROR
#define STPTI_SignalAllocate( a, b )         ST_NO_ERROR
#define STPTI_SignalAssociateBuffer( a, b )         ST_NO_ERROR
#define STPTI_SignalDeallocate( a )         ST_NO_ERROR
#define STPTI_SignalDisassociateBuffer( a, b )         ST_NO_ERROR
#define STPTI_SignalWaitBuffer( a, b , c )         ST_NO_ERROR
#endif

ST_ErrorCode_t STPTI_SlotAllocate(STPTI_Handle_t Handle, STPTI_Slot_t * SlotHandle_p, STPTI_SlotData_t * SlotData_p);

ST_ErrorCode_t STPTI_SlotClearPid( STPTI_Slot_t SlotHandle );

ST_ErrorCode_t STPTI_SlotDeallocate(STPTI_Slot_t SlotHandle);

#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t STPTI_SlotDescramblingControl(STPTI_Slot_t SlotHandle, BOOL EnableDescramblingControl);
#else
#define STPTI_SlotDescramblingControl( a, b )      ST_NO_ERROR
#endif

ST_ErrorCode_t STPTI_SlotLinkToBuffer(STPTI_Slot_t Slot, STPTI_Buffer_t Buffer);

ST_ErrorCode_t STPTI_SlotLinkToRecordBuffer(STPTI_Slot_t Slot, STPTI_Buffer_t Buffer, BOOL DescrambleTS);

ST_ErrorCode_t STPTI_SlotLinkToCdFifo(STPTI_Slot_t Slot, STPTI_DMAParams_t * CdFifoParams_p);

#ifdef STPTI_SP_SUPPORT     /* New as of STPTI version 6 */
ST_ErrorCode_t STPTI_SlotLinkShadowToLegacy  (STPTI_Slot_t ShadowSlotHandle, STPTI_Slot_t LegacySlotHandle, STPTI_PassageMode_t PassageMode );

ST_ErrorCode_t STPTI_SlotUnLinkShadowToLegacy(STPTI_Slot_t ShadowSlotHandle, STPTI_Slot_t LegacySlotHandle );
#endif

#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t STPTI_SlotPacketCount(STPTI_Slot_t SlotHandle, U16 *Count_p);
#else
#define STPTI_SlotPacketCount( a , b )   ST_NO_ERROR
#endif

ST_ErrorCode_t STPTI_SlotQuery(STPTI_Slot_t SlotHandle, BOOL *PacketsSeen_p, BOOL *TransportScrambledPacketsSeen_p,
                               BOOL *PESScrambledPacketsSeen_p, STPTI_Pid_t * Pid_p);

ST_ErrorCode_t STPTI_SlotSetAlternateOutputAction(STPTI_Slot_t SlotHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag);

ST_ErrorCode_t STPTI_SlotSetCCControl(STPTI_Slot_t SlotHandle, BOOL SetOnOff);

ST_ErrorCode_t STPTI_SlotSetCorruptionParams(STPTI_Slot_t SlotHandle, U8 Offset, U8 Value);

ST_ErrorCode_t STPTI_SlotSetPid(STPTI_Slot_t SlotHandle, STPTI_Pid_t Pid);

ST_ErrorCode_t STPTI_SlotSetPidAndRemap( STPTI_Slot_t SlotHandle, STPTI_Pid_t InputPid, STPTI_Pid_t OutputPid );

ST_ErrorCode_t STPTI_SlotState(STPTI_Slot_t SlotHandle, U32 *SlotCount_p, STPTI_ScrambleState_t * ScrambleState_p, STPTI_Pid_t * Pid_p);

ST_ErrorCode_t STPTI_SlotUnLink(STPTI_Slot_t Slot);

ST_ErrorCode_t STPTI_SlotUnLinkRecordBuffer(STPTI_Slot_t Slot);

ST_ErrorCode_t STPTI_SlotUpdatePid( STPTI_Slot_t SlotHandle, STPTI_Pid_t Pid );

ST_ErrorCode_t STPTI_Term(ST_DeviceName_t DeviceName, const STPTI_TermParams_t * TermParams_p);

ST_ErrorCode_t STPTI_UserDataBlockMove(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p);

ST_ErrorCode_t STPTI_UserDataCircularAppend(U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p);

ST_ErrorCode_t STPTI_UserDataCircularSetup(U8 *Buffer_p, U32 BufferSize, U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p);

ST_ErrorCode_t STPTI_UserDataRemaining(STPTI_DMAParams_t *DMAParams_p, U32 *UserDataRemaining);

ST_ErrorCode_t STPTI_UserDataSynchronize(STPTI_DMAParams_t * DMAParams_p);

ST_ErrorCode_t STPTI_UserDataWrite(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t * DMAParams_p);

#if !defined ( STPTI_BSL_SUPPORT )
ST_Revision_t  STPTI_GetRevision(void);
#else
#define  STPTI_GetRevision()     STPTI_REVISION
#endif

ST_ErrorCode_t STPTI_EnableErrorEvent(ST_DeviceName_t DeviceName, STPTI_Event_t EventName);

ST_ErrorCode_t STPTI_DisableErrorEvent(ST_DeviceName_t DeviceName, STPTI_Event_t EventName);

ST_ErrorCode_t STPTI_EnableScramblingEvents(STPTI_Slot_t SlotHandle);

ST_ErrorCode_t STPTI_DisableScramblingEvents(STPTI_Slot_t SlotHandle);

#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t STPTI_Debug(ST_DeviceName_t DeviceName, char *dbg_class, char *string, int string_size);

ST_ErrorCode_t STPTI_PrintDebug(ST_DeviceName_t DeviceName);

ST_ErrorCode_t STPTI_DumpInputTS(STPTI_Buffer_t BufferHandle, U16 bytes_to_capture_per_packet);
#endif

ST_ErrorCode_t STPTI_GetBuffersFromSlot(STPTI_Slot_t SlotHandle, STPTI_Buffer_t *BufferHandle_p, STPTI_Buffer_t *RecordBufferHandle_p);

ST_ErrorCode_t STPTI_BufferLevelChangeEvent(STPTI_Handle_t BufferHandle, BOOL Enable);


#if defined (STPTI_FRONTEND_HYBRID)
/* Frontend extensions */
ST_ErrorCode_t STPTI_FrontendAllocate   ( STPTI_Handle_t PTIHandle, STPTI_Frontend_t * FrontendHandle_p, STPTI_FrontendType_t FrontendType);
ST_ErrorCode_t STPTI_FrontendDeallocate ( STPTI_Frontend_t FrontendHandle );
ST_ErrorCode_t STPTI_FrontendGetParams  ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendType_t * FrontendType_p, STPTI_FrontendParams_t * FrontendParams_p );
ST_ErrorCode_t STPTI_FrontendGetStatus  ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendStatus_t * FrontendStatus_p );
ST_ErrorCode_t STPTI_FrontendInjectData ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendSWTSNode_t *FrontendSWTSNode_p, U32 NumberOfSWTSNodes );
ST_ErrorCode_t STPTI_FrontendLinkToPTI  ( STPTI_Frontend_t FrontendHandle, STPTI_Handle_t PTIHandle);
ST_ErrorCode_t STPTI_FrontendReset      ( STPTI_Frontend_t FrontendHandle );
ST_ErrorCode_t STPTI_FrontendSetParams  ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendParams_t * FrontendParams_p );
ST_ErrorCode_t STPTI_FrontendUnLink     ( STPTI_Frontend_t FrontendHandle );
#endif /* #if defined (STPTI_FRONTEND_HYBRID) ... #endif */


#ifdef ST_OSLINUX
ST_ErrorCode_t STPTI_TermAll(void);
void STPTI_TEST_ResetAllPti(void);
/* These functions are part of user space library. They are defined in here
   so that the interface is consistent with the STPTI interface. */
ST_ErrorCode_t STPTI_TEST_ForceLoader(int Variant);
ST_ErrorCode_t STPTI_GetSWCDFifoCfg(void** GetWritePtrFn, void** SetReadPtrFn);
/* Usage note for STPTI_GetSWCDFifoCfg.
   The returned function pointers should not be called from user space.
   STPTI_BufferSetReadPointer() and STPTI_BufferGetWritePointer() use and
   return physical addresses. This is also the same for STPTI_BufferAllocateManual().
*/

/* Linux implementation for waiting for events. */
ST_ErrorCode_t STPTI_WaitEventList( const STPTI_Handle_t Handle,
                                  U32 NbEvents,
                                  U32 *Events_p,
                                  U32 *Event_That_Has_Occurred,
                                  U32 EventDataSize, /* Size of Event data pointed to by EventData_p */
                                  void *EventData_p );

/* These event functions are for testing only */
ST_ErrorCode_t STPTI_SubscribeEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID );
ST_ErrorCode_t STPTI_UnsubscribeEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID );
ST_ErrorCode_t STPTI_SendEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID, STPTI_EventData_t *EventData );
ST_ErrorCode_t STPTI_WaitForEvent(STPTI_Handle_t Session, STPTI_Event_t *Event_ID, STPTI_EventData_t *EventData );

/* Contiguous memory allocator for use from user space (only available via ioctl layer) */
ST_ErrorCode_t STPTI_DmaMalloc(U32 Size, void **Buffer_p, BOOL UseBigPhysArea);
ST_ErrorCode_t STPTI_DmaFree(void *Buffer_p);
ST_ErrorCode_t STPTI_BufferAllocateManualUser( STPTI_Handle_t SessionHandle,
                                               U8 *Base_p,
                                               U32 RequiredSize,
                                               U32 NumberOfPacketsInMultiPacket,
                                               STPTI_Buffer_t * BufferHandle );

typedef ST_ErrorCode_t (*STPTI_LoaderFunctionPointer_t)(STPTI_DevicePtr_t, void *);
STPTI_LoaderFunctionPointer_t STPTI_GetDefaultLoaderFunctionPointer(void);


#endif

#ifdef STPTI_DEBUG_SUPPORT

/* Exported Function Prototypes ---------------------------------------- */
ST_ErrorCode_t STPTI_DebugInterruptHistoryStart(ST_DeviceName_t DeviceName, STPTI_DebugInterruptStatus_t *History_p, U32 Size);
ST_ErrorCode_t STPTI_DebugGetInterruptHistory(ST_DeviceName_t DeviceName, U32 *NoOfStructsStored, char *buf, int *len);
ST_ErrorCode_t STPTI_DebugStatisticsStart(ST_DeviceName_t DeviceName);
ST_ErrorCode_t STPTI_DebugGetStatistics(ST_DeviceName_t DeviceName, STPTI_DebugStatistics_t *Statistics_p,char *buf,int *len);

#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _STPTIINT_H_ */



/* TC Loaders */

/* Choose your loader by first looking to see what PTI architecture you have, and then select
   The correct loader from the list depending on the transport protocol and features you need.  */

/* PTI4 Architectures (Arch) for each st device...
     PTI4    TSMRG  is for... 5528
     PTI4L   TSMRG  is for... 5100, 5101, 5200, 5301, 7100, 7710
     PTI4UL  TSMRG  is for... 5105, 5107 (this driver does not support this architecture use STPTI4LT)
     PTI4SL  TSMRG  is for... 5202, 7101, 7109
     PTI4SL2 TSMRG  is for... 7105, 7111, 7141
     PTI4SL2 TSGDMA is for... 7200
*/

#if defined( STPTI_DVB_SUPPORT )
/* DVB loaders...
ID  Loader_function         file             Arch    TSMUX  Description
 0  STPTI_DVBTCLoader       DVB_Ldr.c        PTI4    TSMRG  DVB 3 vPTI loader
 8  STPTI_DVBTCLoaderG2     DVB_LdrG2.c      PTI4    TSMRG  G2  3 vPTI loader
 1  STPTI_DVBTCLoaderL      DVB_LdrL.c       PTI4L   TSMRG  DVB 3 vPTI loader
 2  STPTI_DVBTCLoaderLRL    DVB_LdrLRL.c     PTI4L   TSMRG  DVB 3 vPTI loader with Leading Residue for AES CBC/ECB (7100 and PES only) - replaces DVB_LdrKTL.c
 4  STPTI_DVBTCLoader4L     DVB_Ldr4L.c      PTI4L   TSMRG  DVB 4 vPTI loader
 9  STPTI_DVBTCLoaderG2L    DVB_LdrG2L.c     PTI4L   TSMRG  G2  3 vPTI loader
18  STPTI_DVBTCLoaderAL     DVB_LdrAL.c      PTI4L   TSMRG  DVB 3 vPTI loader with AES (reduced # of descramblers, 7100 only, descrambles PES only)
19  STPTI_DVBTCLoaderIPL    DVB_LdrIPL.c     PTI4L   TSMRG  DVB 3 vPTI loader with AES and AES residue whitener (reduced # of descramblers, 7100 only, and descrambles PES only)
20  STPTI_DVBTCLoaderIPSL   DVB_LdrIPSL.c    PTI4L   TSMRG  DVB 3 vPTI loader with AES and AES residue whitener (descrambles PES only)
 5  STPTI_DVBTCLoaderSL     DVB_LdrSL.c      PTI4SL  TSMRG  DVB 4 vPTI loader with AES
10  STPTI_DVBTCLoaderG2SL   DVB_LdrG2SL.c    PTI4SL  TSMRG  G2  4 vPTI loader
21  STPTI_DVBTCLoaderSL2    DVB_LdrSL2.c     PTI4SL2 TSGDMA DVB 4 vPTI loader with AES
31  STPTI_DVBTCLoaderSL3    DVB_LdrSL3.c     PTI4SL2 TSMRG  DVB 4 vPTI loader with AES

17  STPTI_DVBTCLoaderBSLL   DVBBSL_LdrL.c    PTI4L   TSMRG  DVB 3 vPTI bootstrap loader (a cut down loader - you must define STPTI_BSL_SUPPORT)
16  STPTI_DVBTCLoaderBSLSL  DVBBSL_LdrSL.c   PTI4SL  TSMRG  DVB 4 vPTI bootstrap loader (a cut down loader - you must define STPTI_BSL_SUPPORT)
23  STPTI_DVBTCLoaderBSLSL2 DVBBSL_LdrSL2.c  PTI4SL2 TSGDMA DVB 4 vPTI bootstrap loader (a cut down loader - you must define STPTI_BSL_SUPPORT)
33  STPTI_DVBTCLoaderBSLSL3 DVBBSL_LdrSL3.c  PTI4SL2 TSMRG  DVB 4 vPTI bootstrap loader (a cut down loader - you must define STPTI_BSL_SUPPORT)
*/
#endif




#ifdef __cplusplus
extern "C" {
#endif
 
#ifndef _TCPROTO_H
#define _TCPROTO_H
 
#ifdef STPTI_DVB_SUPPORT
ST_ErrorCode_t STPTI_DVBTCLoader(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoader4L(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderAL(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderBSLL(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderBSLSL(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderBSLSL2(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderBSLSL3(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderG2(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderG2L(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderG2SL(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderIPL(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderIPSL(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderL(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderLRL(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderSL(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderSL2(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderSL3(STPTI_DevicePtr_t CodeStart, void * Params_p);
ST_ErrorCode_t STPTI_DVBTCLoaderSLS(STPTI_DevicePtr_t CodeStart, void * Params_p);
#endif  /* STPTI_DVB_SUPPORT */
 
#endif  /* _TCPROTO_H */
 
#if defined( STPTI_TCLOADER )
ST_ErrorCode_t STPTI_TCLOADER(STPTI_DevicePtr_t CodeStart, void * Params_p);
#endif /* !defined( STPTI_TCLOADER ) */
 
#ifdef __cplusplus
}
#endif
 
