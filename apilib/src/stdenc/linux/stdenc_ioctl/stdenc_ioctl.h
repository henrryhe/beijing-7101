/*****************************************************************************
 *
 *  Module      : stdenc_ioctl
 *  Date        : 21-10-2005
 *  Author      : AYARITAR
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef STDENC_IOCTL_H
#define STDENC_IOCTL_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */


#include "stdenc.h"
#include "denc_rev.h"



/*** IOCtl defines ***/

#define STDENC_IOCTL_TYPE   0XFF     /* Unique module id - See 'ioctl-number.txt' */
/* Use magic number as STDENC Major */
#define STDENC_MAGIC_NUMBER     14

/* Exported Constants ----------------------------------------------------- */

#define STDENC_LINUX_DEVICE_NAME                  "denc"

#define STDENC_IOC_INIT                     _IO(STDENC_MAGIC_NUMBER, 0)
#define STDENC_IOC_TERM                     _IO(STDENC_MAGIC_NUMBER, 1)
#define STDENC_IOC_OPEN                     _IO(STDENC_MAGIC_NUMBER, 2)
#define STDENC_IOC_CLOSE                    _IO(STDENC_MAGIC_NUMBER, 3)
#define STDENC_IOC_GETCAPABILITY            _IO(STDENC_MAGIC_NUMBER, 4)
#define STDENC_IOC_SETENCODINGMODE          _IO(STDENC_MAGIC_NUMBER, 5)
#define STDENC_IOC_GETENCODINGMODE          _IO(STDENC_MAGIC_NUMBER, 6)
#define STDENC_IOC_SETMODEOPTION            _IO(STDENC_MAGIC_NUMBER, 7)
#define STDENC_IOC_GETMODEOPTION            _IO(STDENC_MAGIC_NUMBER, 8)
#define STDENC_IOC_SETCOLORBARPATTERN       _IO(STDENC_MAGIC_NUMBER, 9)

/*for testtool commands*/
#define STDENC_IOC_CHECKHANDLE              _IO(STDENC_MAGIC_NUMBER, 10)
#define STDENC_IOC_REGACCESSLOCK            _IO(STDENC_MAGIC_NUMBER, 11)
#define STDENC_IOC_REGACCESSUNLOCK          _IO(STDENC_MAGIC_NUMBER, 12)
#define STDENC_IOC_READREG8                 _IO(STDENC_MAGIC_NUMBER, 13)
#define STDENC_IOC_WRITEREG8                _IO(STDENC_MAGIC_NUMBER, 14)
#define STDENC_IOC_ANDREG8                  _IO(STDENC_MAGIC_NUMBER, 15)
#define STDENC_IOC_ORREG8                   _IO(STDENC_MAGIC_NUMBER, 16)
#define STDENC_IOC_MASKREG8                 _IO(STDENC_MAGIC_NUMBER, 17)



typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STDENC_InitParams_t     InitParams;
} STDENC_Ioctl_Init_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STDENC_TermParams_t     TermParams;
} STDENC_Ioctl_Term_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STDENC_OpenParams_t     OpenParams;
    STDENC_Handle_t         Handle;
} STDENC_Ioctl_Open_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
} STDENC_Ioctl_Close_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
     ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
     ST_DeviceName_t          DeviceName ;
     STDENC_Capability_t     Capability ;
} STDENC_Ioctl_GetCapability_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    STDENC_EncodingMode_t   EncodingMode;
} STDENC_Ioctl_SetEncodingMode_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    STDENC_EncodingMode_t   EncodingMode;
} STDENC_Ioctl_GetEncodingMode_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    STDENC_ModeOption_t     ModeOption;
} STDENC_Ioctl_SetModeOption_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    STDENC_ModeOption_t     ModeOption;
} STDENC_Ioctl_GetModeOption_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    BOOL                    ColorBarPattern;
} STDENC_Ioctl_SetColorBarPattern_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    U32                     Addr;
    U8                     Value;
} STDENC_Ioctl_ReadReg8_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    U32                     Addr;
    U8                      Value;
} STDENC_Ioctl_WriteReg8_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
} STDENC_Ioctl_CheckHandle_t;

typedef struct
{
    /* Parameters to the function */
    STDENC_Handle_t         Handle;
} STDENC_Ioctl_RegAccessLock_t;

typedef struct
{
    /* Parameters to the function */
    STDENC_Handle_t         Handle;
} STDENC_Ioctl_RegAccessUnlock_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    U32                     Addr;
    U8                      AndMask;
} STDENC_Ioctl_AndReg8_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    U32                     Addr;
    U8                      OrMask;
} STDENC_Ioctl_OrReg8_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STDENC_Handle_t         Handle;
    U32                     Addr;
    U8                      AndMask;
    U8                      OrMask;
} STDENC_Ioctl_MaskReg8_t;





#endif     /** STDENC_IOCTL_H **/


