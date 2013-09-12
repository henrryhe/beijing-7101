/*******************************************************************************

File name   : hxvp_main.h

Description : XVP header file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
19 Feb 2007        Created                                           JPB
                   
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HXVP_MAIN_H
#define __HXVP_MAIN_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stos.h"
#include "stlayer.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  XVP_TOP_FIELD,
  XVP_BOTTOM_FIELD
}XVP_FieldType_t;

typedef void* ProcessHandle_t;

/* XVP definition */
typedef void * XVPHandle_t;

/* PLUGIN structure definition */
typedef struct
{
    ST_ErrorCode_t (*Init)(const XVPHandle_t XVPHandle, ProcessHandle_t *ProcessHandle_p);
    ST_ErrorCode_t (*Term)(ProcessHandle_t ProcessHandle);
    ST_ErrorCode_t (*SetParam)(ProcessHandle_t ProcessHandle, STLAYER_XVPParam_t *Param_p);
    ST_ErrorCode_t (*SetExtraParam)(ProcessHandle_t ProcessHandle, STLAYER_XVPExtraParam_t *ExtraParam_p);
    ST_ErrorCode_t (*Compute)(ProcessHandle_t ProcessHandle);
} XVP_PluginFuncTable_t;

typedef struct XVP_CommonData_s
{
    ST_Partition_t              *CPUPartition_p;
    ST_Partition_t              *NCachePartition_p;
    STLAYER_ViewPortHandle_t    VPHandle;
    ProcessHandle_t             ProcessHandle;
    STLAYER_Handle_t            LayerHandle;
    XVP_PluginFuncTable_t       *PluginFunc_p;
} XVP_CommonData_t;
                          
/* prototype of functions */                            
ST_ErrorCode_t  XVP_Init(
    const STLAYER_ViewPortHandle_t      VPHandle,
    STLAYER_ProcessId_t                 processId,
    STLAYER_XVPHandle_t                 *XVPHandle);

ST_ErrorCode_t  XVP_SetParam(
    const STLAYER_XVPHandle_t           XVPHandle,
    STLAYER_XVPParam_t                  *Param_p);                            

ST_ErrorCode_t  XVP_SetExtraParam(
    const STLAYER_XVPHandle_t           XVPHandle,
    STLAYER_XVPExtraParam_t             *ExtraParam_p);                            

ST_ErrorCode_t  XVP_Term(
    const STLAYER_XVPHandle_t           XVPHandle);

ST_ErrorCode_t  XVP_Compute(
    const STLAYER_XVPHandle_t           XVPHandle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __HXVP_MAIN_H */
