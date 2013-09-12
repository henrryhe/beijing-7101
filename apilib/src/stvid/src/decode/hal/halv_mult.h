/*******************************************************************************

File name   : halv_mult.h

Description : Multi-decode HAL video decode header file

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
30 Jul 2001        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __MULTI_DECODE_HAL_VIDEO_DECODE_H
#define __MULTI_DECODE_HAL_VIDEO_DECODE_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "vid_mpeg.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Decode Handle type */
typedef void * HALDEC_MultiDecodeHandle_t;

/* Parameters required to initialise HALDEC */
typedef struct
{
    ST_Partition_t *    CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STVID_DeviceType_t  DeviceType;
} HALDEC_MultiDecodeInitParams_t;

typedef struct
{
/*    const HALDEC_MultiDecodeFunctionsTable_t * FunctionsTable_p;*/
    ST_Partition_t *        CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STVID_DeviceType_t      DeviceType;
    U32                     ValidityCheck;
} HALDEC_MultiDecodeProperties_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */


#ifdef ST_multidecodeswcontroller
/* To be called by a unique init */
ST_ErrorCode_t HALDEC_MultiDecodeInit(const HALDEC_MultiDecodeInitParams_t * const InitParams_p, HALDEC_MultiDecodeHandle_t * const MultiDecodeHandle_p);
ST_ErrorCode_t HALDEC_MultiDecodeTerm(const HALDEC_MultiDecodeHandle_t MultiDecodeHandle);
#endif /* ST_multidecodeswcontroller */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MULTI_DECODE_HAL_VIDEO_DECODE_H */

/* End of halv_mult.h */
