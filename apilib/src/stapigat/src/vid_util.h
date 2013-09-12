/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : vid_util.h
Description : Definition of video extra commands (utilities)
 *            in order to test the features of STVID API 3.0.0 or later
 *            with Testtool
 *
 *
Date          Modification                                      Initials
----          ------------                                      --------
14 march 2002 New. Structure is shared by multiple files           BS
************************************************************************/
#ifndef __VID_UTIL_H__
#define __VID_UTIL_H__

#include "testcfg.h"
#include "stvid.h"
#include "vid_inj.h"

/*   PTI1                                VID1                          */
/*   --------------                     -----------                    */
/*  | Video Slot 0 | ----------------> | cd fifo 1 |                   */
/*   --------------         |           -----------                    */
/*  | Video Slot 1 | -----  |            VID2                          */
/*   --------------       | |           -----------                    */
/*  | Audio Slot 0 |----   ----------> | cd fifo 2 |                   */
/*   --------------     |   |  |        -----------                    */
/*  | PCR   Slot 0 |    |   |  |         AUD1                          */
/*   --------------     |   |  |        -----------                    */
/*                       ------------> | cd fifo 1 |                   */
/*   PTI2                   |  |        -----------                    */
/*   --------------         |  |                                       */
/*  | Video Slot 0 | -------   |                                       */
/*   --------------            |                                       */
/*  | Video Slot 1 | ----------                                        */
/*   --------------                                                    */
/*  | Audio Slot 0 |                                                   */
/*   --------------                                                    */
/*  | PCR   Slot 0 |                                                   */
/*   --------------                                                    */
/* --- Extern functions prototypes --- */
BOOL VID_RegisterCmd2 (void);
#if defined(ST_7020) && defined(ST_5528)
#define VIDEO_MAX_DEVICE                7
#elif defined(ST_7015) || defined(ST_7020)
#define VIDEO_MAX_DEVICE                5
#elif defined (ST_5528)
#define VIDEO_MAX_DEVICE                2
#elif defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_7710) || defined(ST_7100)  || defined (ST_7109)|| defined(ST_5301)|| defined(ST_5525) || defined(ST_ZEUS)|| defined (ST_7200)
#define VIDEO_MAX_DEVICE                3
#else
#define VIDEO_MAX_DEVICE                1
#endif

#define VID_MAX_BUFFER_LOAD             3
#define VID_MAX_STREAM_NAME             256

#define VID_MAX_USER_DATA               75  /* limited by FQ (instead of 255) for overflow test in m_event12 */

#define STLAYER_DEVICE_NAME1    "LAYER1"

/* Types and variables for external bit buffer */
typedef struct VID_ExternalBitBuffer_t
{
    BOOL                    IsAllocated;
    STAVMEM_BlockHandle_t   AvmemBlockHandle;
    STAVMEM_PartitionHandle_t PartitionHandle;
    void *                  Address_p;
    U32                     Size;
} VID_ExternalBitBuffer_t;

extern VID_ExternalBitBuffer_t GlobalExternalBitBuffer;

typedef struct
{
    void* AllocatedBuffer_p;
    void* AlignedBuffer_p;
    ST_Partition_t* Partition_p;
#if defined (mb411) || defined (mb8kg5) || defined(mb519)

    STAVMEM_BlockHandle_t BlockHandle;
#endif /* mb411 || mb519 || mb8kg5 */
    U32 NbBytes;
    U8 UsedCounter;
    char StreamName[VID_MAX_STREAM_NAME];
#ifdef USE_OSPLUS_FOR_INJECT
    void    *FileHandle;
#endif /* USE_OSPLUS_FOR_INJECT */
} VID_BufferLoad_t;

extern VID_BufferLoad_t VID_BufferLoad[VID_MAX_BUFFER_LOAD];   /* address of stream buffer (defined in vid_util.c) */

#if defined(STVID_USE_CRC) || defined(STVID_VALID)

#ifndef FILENAME_MAX                    /* for gcc only ----------------------------------------------------------- */
#define FILENAME_MAX 64
#endif

/* Data structure for CRC control */
typedef struct VID_CRC_s
{
    STVID_Handle_t              VideoHandle;
    STVID_DeviceType_t          DeviceType;
    void                       *BaseAddress_p;
    ST_Partition_t             *CPUPartition_p;
#ifdef VALID_TOOLS
    MSGLOG_Handle_t             MSGLOG_Handle;
#endif /* VALID_TOOLS */
    STVID_CRCCheckResult_t      CRCCheckResult;
    char                        CsvResultFileName[FILENAME_MAX];
    char                        DumpFileName[FILENAME_MAX];
    char                        DecTimeFileName[FILENAME_MAX];
    char                        LogFileName[FILENAME_MAX];
    U32                         NbPictureInRefCRC;
    STVID_RefCRCEntry_t        *RefCRCTable;
    U32                         MaxNbPictureInCompCRC;
    STVID_CompCRCEntry_t        *CompCRCTable;
    U32                         MaxNbPictureInErrorLog;
    STVID_CRCErrorLogEntry_t    *ErrorLogTable;
} VID_CRC_t;

extern VID_CRC_t VID_CRC[VIDEO_MAX_DEVICE];
#endif /* defined(STVID_USE_CRC) || defined(STVID_VALID) */

#if defined (mb411) || defined (mb8kg5) || defined(mb519)
ST_ErrorCode_t vidutil_Allocate(STAVMEM_PartitionHandle_t PartitionHandle, U32 Size, U32 Alignment, void **Address_p, STAVMEM_BlockHandle_t *Handle_p );
void vidutil_Deallocate(STAVMEM_PartitionHandle_t PartitionHandle, STAVMEM_BlockHandle_t *Handle_p, void * Address_p);
#endif  /* mb411  || mb8kg5 || mb519 */

#if defined ST_OSLINUX && defined MODULE
BOOL VID_Inj_RegisterCmd (void);

BOOL VID_MemInject(STVID_Handle_t CurrentHandle, S32 InjectLoop, S32 FifoNb, S32 BufferNb, void * AlignedBuffer_p, U32 NbBytes);
BOOL VIDDecodeFromMemory(STVID_Handle_t CurrentHandle, S32 FifoNb, S32 BuffNb, S32 NbLoop, void * AlignedBuffer_p, U32 NbBytes);
BOOL VID_KillTask(S32 FifoNb, BOOL WaitEnd);
#endif  /* LINUX & KERNEL & MODULE */

#endif /* #ifndef __VID_UTIL_H__ */

/* End of vid_util.h */
