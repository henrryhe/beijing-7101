/*****************************************************************************
 *
 *  Module      : stvtg_ioctl
 *  Date        : 15-10-2005
 *  Author      : ATROUSWA
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef STVTG_IOCTL_H
#define STVTG_IOCTL_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */


#include "stvtg.h"

#include "vtg_rev.h"

/*** IOCtl defines ***/

#define STVTG_IOCTL_TYPE                    0XFF     /* Unique module id - See 'ioctl-number.txt' */


#define STVTG_MAGIC_NUMBER                  15

/* Use STVTG_MAGIC_NUMBER as magic number */
#define STVTG_IOC_INIT                          _IO(STVTG_MAGIC_NUMBER, 0)
#define STVTG_IOC_TERM                          _IO(STVTG_MAGIC_NUMBER, 1)
#define STVTG_IOC_OPEN                          _IO(STVTG_MAGIC_NUMBER, 2)
#define STVTG_IOC_CLOSE                         _IO(STVTG_MAGIC_NUMBER, 3)
#define STVTG_IOC_GETVTGID                      _IO(STVTG_MAGIC_NUMBER, 4)
#define STVTG_IOC_SETMODE                       _IO(STVTG_MAGIC_NUMBER, 5)
#define STVTG_IOC_SETOPTIONALCONFIGURATION      _IO(STVTG_MAGIC_NUMBER, 6)
#define STVTG_IOC_SETSLAVEMODEPARAMS            _IO(STVTG_MAGIC_NUMBER, 7)
#define STVTG_IOC_GETMODE                       _IO(STVTG_MAGIC_NUMBER, 8)
#define STVTG_IOC_GETOPTIONALCONFIGURATION      _IO(STVTG_MAGIC_NUMBER, 9)
#define STVTG_IOC_GETSLAVEMODEPARAMS            _IO(STVTG_MAGIC_NUMBER, 10)
#define STVTG_IOC_GETCAPABILITY                 _IO(STVTG_MAGIC_NUMBER, 11)
#define STVTG_IOC_CALIBRATESYNCLEVEL            _IO(STVTG_MAGIC_NUMBER, 12)
#define STVTG_IOC_GETLEVELSETTINGS              _IO(STVTG_MAGIC_NUMBER, 13)
#define	STVTG_IOC_GETMODESYNCPARAMS             _IO(STVTG_MAGIC_NUMBER, 14)



/* =================================================================
   Global variables declaration
   ================================================================= */

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STVTG_InitParams_t      InitParams;
} STVTG_Ioctl_Init_t;



typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STVTG_TermParams_t      TermParams;
} STVTG_Ioctl_Term_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STVTG_OpenParams_t      OpenParams;
    STVTG_Handle_t          Handle;
} STVTG_Ioctl_Open_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVTG_Handle_t         Handle;
} STVTG_Ioctl_Close_t;



typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t             DeviceName ;
    STVTG_Capability_t          Capability;
} STVTG_Ioctl_GetCapability_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t             DeviceName;
    STVTG_VtgId_t          VtgId;
} STVTG_Ioctl_GetVtgId_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVTG_Handle_t             Handle;
    STVTG_TimingMode_t          Mode;
} STVTG_Ioctl_SetMode_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVTG_Handle_t             Handle;
    STVTG_OptionalConfiguration_t          OptionalConfiguration;
} STVTG_Ioctl_SetOptionalConfiguration_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVTG_Handle_t                  Handle;
    STVTG_SlaveModeParams_t         SlaveModeParams;
} STVTG_Ioctl_SetSlaveModeParams_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STVTG_Handle_t         Handle;
    STVTG_SyncParams_t       SyncParams;
} STVTG_Ioctl_GetModeSyncParams_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVTG_Handle_t             Handle;
    STVTG_TimingMode_t          TimingMode;
    STVTG_ModeParams_t          ModeParams;
} STVTG_Ioctl_GetMode_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVTG_Handle_t             Handle;
    STVTG_OptionalConfiguration_t          OptionalConfiguration;
} STVTG_Ioctl_GetOptionalConfiguration_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVTG_Handle_t             Handle;
    STVTG_SlaveModeParams_t    SlaveModeParams;
} STVTG_Ioctl_GetSlaveModeParams_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVTG_Handle_t             Handle;
    STVTG_SyncLevel_t          SyncLevel;
} STVTG_Ioctl_CalibrateSyncLevel_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVTG_Handle_t             Handle;
    STVTG_SyncLevel_t          SyncLevel;
} STVTG_Ioctl_GetLevelSettings_t;

#endif
