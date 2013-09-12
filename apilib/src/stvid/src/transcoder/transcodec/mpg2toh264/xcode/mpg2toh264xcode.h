/*!
 ************************************************************************
 * \file mpg2toh264xcode.h
 *
 * \brief MPEG2 to H264 xcode decoder data structures and functions prototypes
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion */

#ifndef __MPG2TOH264XCODE_H
#define __MPG2TOH264XCODE_H

/* Includes ----------------------------------------------------------------- */
#include "mpg2toh264.h"

#include "stfdma.h"
#include "inject.h"

#ifdef ST_OSLINUX
#include "compat.h"
#else

#ifndef STTBX_REPORT
    #define STTBX_REPORT
#endif
#ifndef STTBX_PRINT    
    #define STTBX_PRINT
#endif    
#include "sttbx.h"
#include "stsys.h"
#endif

#include "vid_ctcm.h"

#include "MP2H264Xcoder_interface.h"

#include "mme.h"
#include "embx.h"

#if defined(STVID_VALID) && defined(XCODE_DUMP)
#include "mpg2toh264xcode_dump.h"
#endif

#ifndef MME_VIDEO_TRANSPORT_NAME
#define MME_VIDEO_TRANSPORT_NAME "ups"
#endif /* not MME_VIDEO_TRANSPORT_NAME */

#ifndef MP2H264_XCODE_MME_TRANSFORMER_NAME
#define MP2H264_XCODE_MME_TRANSFORMER_NAME "MP2H264XCODER"
#endif

/* Constants ------------------------------------------------------- */
#define NUM_MME_MP2TOH264_PARAM_BUFFERS 1
#define NUM_MME_MP2TOH264_TRANSFORM_BUFFERS 2

typedef enum mpg2toh264XcodeCommandID_e
{
  GLOBAL_PARAM,
  TRANSFORM
} mpg2toh264XcodeCommandID_t;

#ifdef XCODE_DEMO
typedef enum H264ReconBufferIdx_e
{
  OLDEST_REF  = 0,
  NEWEST_REF  = 1,
  CURRENT_PIC = 2,
  NOT_USED    = 3,
  MAX_RECONBUFFER_IDX   = NOT_USED
} H264ReconBufferIdx_t;

typedef struct H264ReconBuffer_s
{
  void * Luma_p;
  void * Chroma_p;
  void * MBDescr_p;
  STAVMEM_BlockHandle_t LumaHdl;
  STAVMEM_BlockHandle_t ChromaHdl;
  STAVMEM_BlockHandle_t MBDescrHdl;
} H264ReconBuffer_t;

#endif

/* Exported types ----------------------------------------------------------- */
typedef struct mpg2toh264XcoderContext_s
{
    BOOL   XcoderIsRunning; /* TRUE When waiting for Xcoder transform; FALSE when Xcoder transform is completed */
  	XCODER_TranscodingJobResults_t  XCODER_TranscodingJobResults;
} mpg2toh264XcoderContext_t;


/*! Source decoder top level data structure */
/* Shared with the companion */
typedef struct mpg2toh264xcode_PrivateData_s
{
    /* MME initialisation */
    MME_TransformerHandle_t MME_TransformerHandle;
    Xcode_MPEG2H264_InitTransformerParam_t Xcode_MPEG2H264_InitTransformerParam;

    /* Global Parameters */
    MME_Command_t                         CmdInfoParam; /* used for MME_SET_GLOBAL_TRANSFORM_PARAMS command */
    Xcode_GlobalParams_t                  XcodeGlobalParams;
    Xcode_MPEG2H264_GlobalParams_t        Xcode_MPEG2H264_GlobalParams; /* Passed as *DataBuffer_p[0] */

    /* Transform parameters */
    MME_Command_t                         CmdInfoFmw;   /* used for MME_TRANSFORM for Firmware commands */
    Xcode_TransformParams_t               Xcode_TransformParams;
    Xcode_MPEG2H264_TransformParams_t     Xcode_MPEG2H264_TransformParams;
    XCode_TransformStatusAdditionalInfo_t TransformStatusAdditionalInfo;
    /* Call back job completed */
    void  (* XcoderJobCompletedCB) (void * TranscoderHandle, XCODER_TranscodingJobResults_t XCODER_TranscodingJobResults);

    /* semaphores */
    semaphore_t * GlobalTransformParamsCompletedSemaphore_p;
    semaphore_t * TransformCompletedSemaphore_p;

    /* General */
    mpg2toh264XcoderContext_t             mpg2toh264XcoderContext;
    void *                                TranscoderHandle;
#ifdef XCODE_DEMO
    
#if 0
    H264ReconBufferIdx_t OldestReconBufferIdx;
    H264ReconBufferIdx_t NewestReconBufferIdx;
    H264ReconBufferIdx_t CurPicReconBufferIdx;
#endif

    H264ReconBufferIdx_t ReconBufferIdx[3];

    H264ReconBuffer_t ReconBuffer[3];
#endif
        
} mpg2toh264xcode_PrivateData_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t mpg2toh264Xcode_GetPictureMaxSizeInByte(const XCODE_Handle_t XcodeHandle);
ST_ErrorCode_t mp2toh264_AllocateH264FrameBuffersForDemo(const XCODE_Handle_t XcodeHandle);
ST_ErrorCode_t mp2toh264_DeAllocateH264FrameBuffersForDemo(const XCODE_Handle_t XcodeHandle);

#endif /* #ifdef __MPG2TOH264XCODE_H */

