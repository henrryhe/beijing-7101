/*****************************************************************************
 *
 *  Module      : stvmix_ioctl
 *  Date        : 30-10-2005
 *  Author      : AYARITAR
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef STVMIX_IOCTL_H
#define STVMIX_IOCTL_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */


#include "stvmix.h"
#include "vmix_rev.h"


/*** IOCtl defines ***/

#define STVMIX_IOCTL_TYPE   0XFF     /* Unique module id - See 'ioctl-number.txt' */

/* Use magic number as STVMIX Major */
#define STVMIX_MAGIC_NUMBER     17

#define STVMIX_IOC_INIT                     _IO(STVMIX_MAGIC_NUMBER, 0)
#define STVMIX_IOC_TERM                     _IO(STVMIX_MAGIC_NUMBER, 1)
#define STVMIX_IOC_OPEN                     _IO(STVMIX_MAGIC_NUMBER, 2)
#define STVMIX_IOC_CLOSE                    _IO(STVMIX_MAGIC_NUMBER, 3)
#define STVMIX_IOC_GETCAPABILITY            _IO(STVMIX_MAGIC_NUMBER, 4)
#define STVMIX_IOC_DISABLE                  _IO(STVMIX_MAGIC_NUMBER, 5)
#define STVMIX_IOC_ENABLE                   _IO(STVMIX_MAGIC_NUMBER, 6)
#define STVMIX_IOC_DISCONNECTLAYERS         _IO(STVMIX_MAGIC_NUMBER, 7)
#define STVMIX_IOC_SETBACKGROUNDCOLOR       _IO(STVMIX_MAGIC_NUMBER, 8)
#define STVMIX_IOC_SETSCREENOFFSET          _IO(STVMIX_MAGIC_NUMBER, 9)
#define STVMIX_IOC_SETSCREENPARAMS          _IO(STVMIX_MAGIC_NUMBER, 10)
#define STVMIX_IOC_SETTIMEBASE              _IO(STVMIX_MAGIC_NUMBER, 11)
#define STVMIX_IOC_GETSCREENPARAMS          _IO(STVMIX_MAGIC_NUMBER, 12)
#define STVMIX_IOC_GETSCREENOFFSET          _IO(STVMIX_MAGIC_NUMBER, 13)
#define STVMIX_IOC_GETCONNECTEDLAYERS       _IO(STVMIX_MAGIC_NUMBER, 14)
#define STVMIX_IOC_GETBACKGROUNDCOLOR       _IO(STVMIX_MAGIC_NUMBER, 15)
#define STVMIX_IOC_CONNECTLAYERS            _IO(STVMIX_MAGIC_NUMBER, 16)
#define STVMIX_IOC_GETTIMEBASE              _IO(STVMIX_MAGIC_NUMBER, 17)

#define MAX_LAYER 10
#define MAX_VOUT  6

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    ST_DeviceName_t         VoutName[MAX_VOUT];
    U16 NbVoutParams;
    STVMIX_InitParams_t     InitParams;
} STVMIX_Ioctl_Init_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STVMIX_TermParams_t     TermParams;
} STVMIX_Ioctl_Term_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STVMIX_OpenParams_t     OpenParams;
    STVMIX_Handle_t         Handle;
} STVMIX_Ioctl_Open_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
} STVMIX_Ioctl_Close_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
} STVMIX_Ioctl_Disable_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
} STVMIX_Ioctl_Enable_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
     ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
     ST_DeviceName_t          DeviceName ;
     STVMIX_Capability_t     Capability ;
} STVMIX_Ioctl_GetCapability_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
} STVMIX_Ioctl_DisconnectLayers_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
    STGXOBJ_ColorRGB_t RGB888;
    BOOL               Enable;
} STVMIX_Ioctl_SetBackgroundColor_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
    S8                Horizontal;
    S8                Vertical ;
} STVMIX_Ioctl_SetScreenOffset_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
    STVMIX_ScreenParams_t ScreenParams;
} STVMIX_Ioctl_SetScreenParams_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
    ST_DeviceName_t  VTGDriver;
} STVMIX_Ioctl_SetTimeBase_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
    STVMIX_ScreenParams_t   ScreenParams;
} STVMIX_Ioctl_GetScreenParams_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
    S8                      Horizontal;
    S8                      Vertical;
} STVMIX_Ioctl_GetScreenOffset_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
    U16              LayerPosition;
    STVMIX_LayerDisplayParams_t LayerParams;
} STVMIX_Ioctl_GetConnectedLayers_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
    STGXOBJ_ColorRGB_t      RGB888;
    BOOL                    Enable;
} STVMIX_Ioctl_GetBackgroundColor_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t             Handle;
    STVMIX_LayerDisplayParams_t LayerDisplayParams[MAX_LAYER];
    U16                         NbLayerParams;

} STVMIX_Ioctl_ConnectLayers_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVMIX_Handle_t         Handle;
    ST_DeviceName_t  VTGDriver;
} STVMIX_Ioctl_GetTimeBase_t;

#endif /** STVMIX_IOCTL_H **/
