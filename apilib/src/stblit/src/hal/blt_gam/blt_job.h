/*******************************************************************************

File name   : blt_job.h

Description : job header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
20 Jun 2000        Created                                           TM
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_JOB_H
#define __BLT_JOB_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stblit.h"
#include "blt_init.h"
#include "blt_com.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t  stblit_AddNodeListInJob ( stblit_Device_t*       Device_p,
                                          STBLIT_JobHandle_t     JobHandle,
                                          stblit_NodeHandle_t    FirstNodeHandle,
                                          stblit_NodeHandle_t    LastNodeHandle,
                                          U32                    NbNodesInList);
ST_ErrorCode_t  stblit_AddJBlitInJob ( stblit_Device_t*      Device_p,
                                              STBLIT_JobHandle_t    JobHandle,
                                              stblit_NodeHandle_t   FirstNodeHandle,
                                              stblit_NodeHandle_t   LastNodeHandle,
                                              U32                   NbNodesInList,
                                              stblit_OperationConfiguration_t*   Config_p);
ST_ErrorCode_t  stblit_AddLockInJob (STBLIT_JobHandle_t     JobHandle);
ST_ErrorCode_t  stblit_AddUnlockInJob (STBLIT_JobHandle_t     JobHandle);
ST_ErrorCode_t stblit_DeleteAllJobFromList(stblit_Unit_t* Unit_p,STBLIT_JobHandle_t JobHandle);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_JOB_H  */

/* End of blt_job.h */
