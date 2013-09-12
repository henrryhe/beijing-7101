/*******************************************************************************

File name   : crc.h

Description : CRC computation/Check module external header file

COPYRIGHT (C) STMicroelectronics 2006.

Date               Modification                                     Name
----               ------------                                     ----
24 Aug 2006        Created                                           PC
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CRC_H
#define __CRC_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* Public types ------------------------------------------------------------- */

typedef struct
{
    ST_Partition_t *    CPUPartition_p;
    ST_DeviceName_t     EventsHandlerName;
    ST_DeviceName_t     VideoName;
    STVID_DeviceType_t  DeviceType;
    U8                  DecoderNumber;
} VIDCRC_InitParams_t;

typedef void * VIDCRC_Handle_t;

/* Public functions --------------------------------------------------------- */

ST_ErrorCode_t VIDCRC_Init(const VIDCRC_InitParams_t * const InitParams_p,
                           VIDCRC_Handle_t * const CRCHandle_p);
ST_ErrorCode_t VIDCRC_Term(const VIDCRC_Handle_t CRCHandle);
#ifdef VALID_TOOLS
ST_ErrorCode_t VIDCRC_SetMSGLOGHandle(const VIDCRC_Handle_t CRCHandle, MSGLOG_Handle_t MSGLOG_Handle);
#endif /* VALID_TOOLS */
ST_ErrorCode_t VIDCRC_CRCStartCheck(const VIDCRC_Handle_t CRCHandle, STVID_CRCStartParams_t *StartParams_p);
BOOL VIDCRC_IsCRCCheckRunning(const VIDCRC_Handle_t CRCHandle);
BOOL VIDCRC_IsCRCModeField(const VIDCRC_Handle_t CRCHandle,
                           STVID_PictureInfos_t *PictureInfos_p,
                           STVID_MPEGStandard_t MPEGStandard);
ST_ErrorCode_t VIDCRC_CheckCRC(const VIDCRC_Handle_t CRCHandle,
                                STVID_PictureInfos_t *PictureInfos_p,
                                VIDCOM_PictureID_t ExtendedPresentationOrderPictureID,
                                U32 DecodingOrderFrameID, U32 PictureNumber,
                                BOOL IsRepeatedLastPicture, BOOL IsRepeatedLastButOnePicture,
                                void *LumaAddress, void *ChromaAddress,
                                U32 LumaCRC, U32 ChromaCRC);
ST_ErrorCode_t VIDCRC_CRCStopCheck(const VIDCRC_Handle_t CRCHandle);
ST_ErrorCode_t VIDCRC_CRCGetCurrentResults(const VIDCRC_Handle_t CRCHandle, STVID_CRCCheckResult_t *CRCCheckResult_p);

BOOL VIDCRC_IsCRCRunning(const VIDCRC_Handle_t CRCHandle);
BOOL VIDCRC_IsCRCQueueingRunning(const VIDCRC_Handle_t CRCHandle);
ST_ErrorCode_t VIDCRC_CRCStartQueueing(const VIDCRC_Handle_t CRCHandle);
ST_ErrorCode_t VIDCRC_CRCStopQueueing(const VIDCRC_Handle_t CRCHandle);
ST_ErrorCode_t VIDCRC_CRCEnqueue(const VIDCRC_Handle_t CRCHandle, STVID_CRCDataMessage_t DataToEnqueue);
ST_ErrorCode_t VIDCRC_CRCGetQueue(const VIDCRC_Handle_t CRCHandle, STVID_CRCReadMessages_t *ReadCRC_p);

#ifdef VALID_TOOLS
ST_ErrorCode_t VIDCRC_RegisterTrace(void);
#endif /* VALID_TOOLS */
#endif /* #ifndef __CRC_H */

/* End of crc.h */
