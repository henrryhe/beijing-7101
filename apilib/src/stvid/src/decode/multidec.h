/*******************************************************************************

File name   : multidec.h

Description : Video multi-instance software controller header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
30 Aug 2001        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_MULTI_INSTANCE_SW_CONTROLLER_H
#define __VIDEO_MULTI_INSTANCE_SW_CONTROLLER_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "vid_mpeg.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Max number of multi-decode software controller connections: this is the max
possible number of decoders per chip (today 5 for STi7015 and STi7020) */
#define VIDDEC_MAX_MULTI_DECODE_CONNECT 5

/* Exported Types ----------------------------------------------------------- */

typedef void * VIDDEC_MultiDecodeHandle_t;

typedef struct
{
    ST_Partition_t *    CPUPartition_p;
    STVID_DeviceType_t  DeviceType;
    void *              DeviceBaseAddress_p;
} VIDDEC_MultiDecodeInitParams_t;


/* Type of the function that to call to execute the decode */
typedef void (*VIDDEC_MultiDecodeExecuteDecode_t) (void * const ExecuteParam_p);

/* Parameters required to decode a picture */
typedef struct
{
    STGXOBJ_ScanType_t                  DecodeScanType;         /* Interlaced or progressive */
    STVID_PictureStructure_t            DecodePictureStructure; /* Frame or field */
    U8                                  DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    VIDDEC_MultiDecodeExecuteDecode_t   ExecuteFunction;        /* Function to call to execute the decode */
    void *                              ExecuteFunctionParam;   /* Parameter to pass to the execute function */
} VIDDEC_MultiDecodeReadyParams_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

#ifdef ST_multidecodeswcontroller
/* Muti-decode software controller functions */
ST_ErrorCode_t VIDDEC_MultiDecodeConnect(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle);
ST_ErrorCode_t VIDDEC_MultiDecodeFinished(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle);
ST_ErrorCode_t VIDDEC_MultiDecodeDisconnect(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle);
ST_ErrorCode_t VIDDEC_MultiDecodeInit(const VIDDEC_MultiDecodeInitParams_t * const InitParams_p, VIDDEC_MultiDecodeHandle_t * const MultiDecodeHandle_p);
ST_ErrorCode_t VIDDEC_MultiDecodeReady(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle, const VIDDEC_MultiDecodeReadyParams_t * const DecodeParams_p);
ST_ErrorCode_t VIDDEC_MultiDecodeTerm(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle);
#endif /* ST_multidecodeswcontroller */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_MULTI_INSTANCE_SW_CONTROLLER_H */

/* End of multidec.h */
