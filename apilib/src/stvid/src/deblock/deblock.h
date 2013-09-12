/*******************************************************************************

File name   : deblock.h

Description : Mpeg2 deblocking feature  file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
24 Nov 2006        Created                                           IK
*******************************************************************************/
/* Define to prevent recursive inclusion */

#ifndef __DEBLOCK_H
#define __DEBLOCK_H

#if !defined ST_OSLINUX
#include <string.h>
#include <assert.h>
#include <ctype.h>
#endif

#include "stdevice.h"
#include "stsys.h"
#include "vid_ctcm.h"

#include "mpeg2_videodeblocktransformertypes.h"
/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


#ifndef MME_VIDEO_TRANSPORT_NAME
#define MME_VIDEO_TRANSPORT_NAME "ups"
#endif /* not MME_VIDEO_TRANSPORT_NAME */

typedef void * VID_Deblocking_Handle_t;

typedef void * VID_Deblocking_StreamInfo_t;

typedef struct
{
    ST_Partition_t		          *CPUPartition_p;
    ST_DeviceName_t		          VideoName;
    STVID_DeviceType_t          DeviceType;
    void *                      DeltaBaseAddress_p;
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;
    void                        *EndDeblockingCallbackFuction;
} VID_Deblocking_InitParams_t;

ST_ErrorCode_t VID_DeblockingInit (VID_Deblocking_InitParams_t * const InitParams_p,VID_Deblocking_Handle_t * const DeblockingHandle_p);
void VID_DeblockingTerm(VID_Deblocking_Handle_t * const DeblockHandle);
ST_ErrorCode_t VID_DeblockingSetParams ( VID_Deblocking_Handle_t * const DeblockHandle,VID_Deblocking_StreamInfo_t  StreamInfo,MPEG2Deblock_InitTransformerParam_t TransformerInitParams);
void VID_DeblockingStart(VID_Deblocking_Handle_t * const DeblockHandle,MPEG2Deblock_TransformParam_t DeblockTransfromParams);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DEBLOCK_H */

/* End of deblock.h */
