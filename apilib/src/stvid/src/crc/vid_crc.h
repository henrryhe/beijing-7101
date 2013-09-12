/*******************************************************************************
File name   : vid_crc.h

Description : CRC computation/Check internal header file

COPYRIGHT (C) STMicroelectronics 2006.

Date               Modification                                     Name
----               ------------                                     ----
24 Aug 2006        Created                                           PC
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VID_CRC_H
#define __VID_CRC_H

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include "stdio.h"
#endif

#include "vid_com.h"
#include "stddefs.h"
#include "stevt.h"
#include "stvid.h"
#include "crc.h"

#if defined(TRACE_VIDCRC)
#define TRACE_UART
#endif /* TRACE_VIDCRC */

#if defined TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */

#ifndef FILENAME_MAX                    /* for gcc only ----------------------------------------------------------- */
#define FILENAME_MAX 1024
#endif

/* Exported Constants ------------------------------------------------------- */

#define VIDEO_DECODER_CRC_MAX_Q_SIZE  128

#ifdef ST_7100
    #define FIRST_FIELD_ISSUE_WA
#endif /* ST_7100 */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
#define TRACE_VIDCRC_COLOR (TRC_ATTR_BACKGND_BLACK | TRC_ATTR_FORGND_GREEN | TRC_ATTR_BRIGHT)

#define VIDCRC_TRACE_BLOCKID   (0x7015)    /* 0x7000 | STVID_DRIVER_ID */

/* !!!! WARNING !!!! : Any modification of this emun should be reported in emun in vid_disp.c */
enum
{
    VIDCRC_TRACE_GLOBAL = 0,
    VIDCRC_TRACE_CHECK,
    VIDCRC_TRACE_VD_CHECK
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

enum
{
    VIDCRC_END_OF_CRC_CHECK_EVT_ID,
    VIDCRC_NEW_CRC_QUEUED_EVT_ID,
    VIDCRC_NB_REGISTERED_EVENTS_IDS
};

typedef enum VIDCRC_state_e
{
    VIDCRC_STATE_STOPPED,
    VIDCRC_STATE_CRC_CHECKING,
    VIDCRC_STATE_PERF_CHECKING,
    VIDCRC_STATE_CRC_QUEUEING
} VIDCRC_state_t;


/* This type is an extension of STVID_MPEGStandard_t from stvid.h to add definition for
  different IPs */
typedef enum VIDCRC_VideoStandard_e
{
    VIDCRC_VIDEO_STANDARD_MPEG2 = STVID_MPEG_STANDARD_ISO_IEC_13818,
    VIDCRC_VIDEO_STANDARD_H264 = STVID_MPEG_STANDARD_ISO_IEC_14496,
    VIDCRC_VIDEO_STANDARD_VC1 = STVID_MPEG_STANDARD_SMPTE_421M,
    VIDCRC_VIDEO_STANDARD_MPEG4P2 = STVID_MPEG_STANDARD_ISO_IEC_14496_2,
    VIDCRC_VIDEO_STANDARD_MPEG2_DELTA,
    VIDCRC_VIDEO_STANDARD_UNKNOWN
} VIDCRC_VideoStandard_t;

typedef struct VIDCRC_YUVBuffer_s
{
    U32 BufferSize;
    U8 *Buffer;
    U8 *CbAddress;
    U8 *CrAddress;
} VIDCRC_YUVBuffer_t;

typedef struct
{
/* Init params */
    ST_Partition_t                          *CPUPartition_p;
    STVID_DeviceType_t                      DeviceType;
    U8                                      DecoderNumber;
    STEVT_Handle_t                          EventsHandle;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
/* Events, Tasks and semaphores */
    STEVT_EventID_t                         RegisteredEventsID[VIDCRC_NB_REGISTERED_EVENTS_IDS];
/* Report/dump File names */
    char                                    LogFileName[FILENAME_MAX];
    char                                    CsvResultFileName[FILENAME_MAX];
    char                                    DumpFileName[FILENAME_MAX];
/* CRC module mode */
    VIDCRC_VideoStandard_t                  VideoStandard;
    BOOL                                    DumpCRC;
    BOOL                                    DumpFailingFrames;
/* CRC module state */
#ifdef VALID_TOOLS
    MSGLOG_Handle_t                         MSGLOG_Handle;
#endif /* VALID_TOOLS */
    U32                                     NbPicturesToCheck;
    VIDCRC_state_t                          State;
    U32                                     LastDecodingOrderFrameID;
    U32                                     NextRefCRCIndex;
    U32                                     NextCompCRCIndex;
    U32                                     NextErrorLogIndex;
    U32                                     NbErrors;
 /* CRC tables */
    U32                                     NbPictureInRefCRC;
    STVID_RefCRCEntry_t                     *RefCRCTable;
    U32                                     MaxNbPictureInCompCRC;
    STVID_CompCRCEntry_t                    *CompCRCTable;
    U32                                     MaxNbPictureInErrorLog;
    STVID_CRCErrorLogEntry_t                *ErrorLogTable;
#ifdef FIRST_FIELD_ISSUE_WA
    BOOL                                    IsFirstPicture;
#endif /* FIRST_FIELD_ISSUE_WA */
/* Software CRC Specific data */
#if defined(STVID_SOFTWARE_CRC_MODE)
    U32                                     crc32_table[256];
    VIDCRC_YUVBuffer_t                      YUVBuffer;
#endif /* defined(STVID_SOFTWARE_CRC_MODE) */
    struct
    {
        STVID_CRCDataMessage_t CRCData [VIDEO_DECODER_CRC_MAX_Q_SIZE];
        U8 ReadIndex;
        U8 WriteIndex;
        U8 DataCount;
    } CRCMessageQueue;
} VIDCRC_Data_t;

#endif /* #ifndef __VID_CRC_H */

/* End of vid_crc.h */
