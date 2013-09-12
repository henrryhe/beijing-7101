/*****************************************************************************
 *
 *  Module      : stvout_ioctl
 *  Date        : 23-10-2005
 *  Author      : ATROUSWA
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef STVOUT_IOCTL_H
#define STVOUT_IOCTL_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */


#include "stvout.h"
#include "vout_rev.h"

/*** IOCtl defines ***/

#define STVOUT_IOCTL_TYPE   0XFF     /* Unique module id - See 'ioctl-number.txt' */

#define STVOUT_MAGIC_NUMBER     16
#define STVOUT_IOC_INIT                     _IO(STVOUT_MAGIC_NUMBER, 0)
#define STVOUT_IOC_TERM                     _IO(STVOUT_MAGIC_NUMBER, 1)
#define STVOUT_IOC_OPEN                     _IO(STVOUT_MAGIC_NUMBER, 2)
#define STVOUT_IOC_CLOSE                    _IO(STVOUT_MAGIC_NUMBER, 3)
#define STVOUT_IOC_GETCAPABILITY            _IO(STVOUT_MAGIC_NUMBER, 4)
#define STVOUT_IOC_DISABLE                  _IO(STVOUT_MAGIC_NUMBER, 5)
#define STVOUT_IOC_ENABLE                   _IO(STVOUT_MAGIC_NUMBER, 6)
#define STVOUT_IOC_SETOUTPUTPARAMS          _IO(STVOUT_MAGIC_NUMBER, 7)
#define STVOUT_IOC_GETOUTPUTPARAMS          _IO(STVOUT_MAGIC_NUMBER, 8)
#define STVOUT_IOC_SETOPTION                _IO(STVOUT_MAGIC_NUMBER, 9)
#define STVOUT_IOC_GETOPTION                _IO(STVOUT_MAGIC_NUMBER, 10)
#define STVOUT_IOC_SETINPUTSOURCE           _IO(STVOUT_MAGIC_NUMBER, 11)
#define STVOUT_IOC_GETDACSELECT             _IO(STVOUT_MAGIC_NUMBER, 12)
#define STVOUT_IOC_GETSTATE                 _IO(STVOUT_MAGIC_NUMBER, 13)
#define STVOUT_IOC_GETTARGETINFORMATION     _IO(STVOUT_MAGIC_NUMBER, 14)
#define STVOUT_IOC_SENDDATA                 _IO(STVOUT_MAGIC_NUMBER, 15)
#define STVOUT_IOC_SETHDCPPARAMS            _IO(STVOUT_MAGIC_NUMBER, 16)
#define STVOUT_IOC_START                    _IO(STVOUT_MAGIC_NUMBER, 17)
#define STVOUT_IOC_GETHDCPSINKPARAMS        _IO(STVOUT_MAGIC_NUMBER, 18)
#define STVOUT_IOC_UPDATEFORBIDDENKSVS      _IO(STVOUT_MAGIC_NUMBER, 19)
#define STVOUT_IOC_ENABLEDEFAULTOUTPUT      _IO(STVOUT_MAGIC_NUMBER, 20)
#define STVOUT_IOC_DISABLEDEFAULTOUTPUT     _IO(STVOUT_MAGIC_NUMBER, 21)
#define STVOUT_IOC_GETSTATISTICS            _IO(STVOUT_MAGIC_NUMBER, 22)
#define STVOUT_IOC_SETHDMIOUTPUTTYPE        _IO(STVOUT_MAGIC_NUMBER, 23)
#define STVOUT_IOC_ADJUSTCHROMALUMADELAY    _IO(STVOUT_MAGIC_NUMBER, 24)
#define STVOUT_IOC_SENDCECMESSAGE           _IO(STVOUT_MAGIC_NUMBER, 25)
#define STVOUT_IOC_CECPHYSICALADDRAVAILABLE _IO(STVOUT_MAGIC_NUMBER, 26)
#define STVOUT_IOC_CECSETADDITIONALADDRESS  _IO(STVOUT_MAGIC_NUMBER, 27)



typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STVOUT_InitParams_t     InitParams;
} STVOUT_Ioctl_Init_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STVOUT_TermParams_t     TermParams;
} STVOUT_Ioctl_Term_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STVOUT_OpenParams_t     OpenParams;
    STVOUT_Handle_t         Handle;
} STVOUT_Ioctl_Open_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
} STVOUT_Ioctl_Close_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
} STVOUT_Ioctl_Disable_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t   ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t  Handle;
} STVOUT_Ioctl_Enable_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
     ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
     ST_DeviceName_t         DeviceName ;
     STVOUT_Capability_t     Capability ;
} STVOUT_Ioctl_GetCapability_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_OutputParams_t   OutputParams;
} STVOUT_Ioctl_SetOutputParams_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_OutputParams_t   OutputParams;
} STVOUT_Ioctl_GetOutputParams_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_OptionParams_t   OptionParams;
} STVOUT_Ioctl_SetOption_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_OptionParams_t   OptionParams;
} STVOUT_Ioctl_GetOption_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_Source_t         Source;
} STVOUT_Ioctl_SetInputSource_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    U8                      DacSelect;
} STVOUT_Ioctl_GetDacSelect_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_State_t          State;
} STVOUT_Ioctl_GetState_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_TargetInformation_t TargetInformation;
    U8  *Buffer_p;
} STVOUT_Ioctl_GetTargetInformation_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_InfoFrameType_t InfoFrameType;
    U8                      Buffer;
    U32                     Size;
} STVOUT_Ioctl_SendData_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_HDCPParams_t     HDCPParams;
} STVOUT_Ioctl_SetHDCPParams_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t           Handle;
    STVOUT_HDCPSinkParams_t   HDCPSinkParams;

} STVOUT_Ioctl_GetHDCPSinkParams_t;


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
} STVOUT_Ioctl_Start_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    U8                      KSVList;
    U32                     KSVNumber;
} STVOUT_Ioctl_UpdateForbiddenKSVs_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_DefaultOutput_t  DefaultOutput;

} STVOUT_Ioctl_EnableDefaultOutput_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;

} STVOUT_Ioctl_DisableDefaultOutput_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_Statistics_t     Statistics;

} STVOUT_Ioctl_GetStatistics_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_OutputType_t     OutputType;

} STVOUT_Ioctl_SetHDMIOutputType_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t           Handle;
    STVOUT_ChromaLumaDelay_t  CLDelay;
} STVOUT_Ioctl_AdjustChromaLumaDelay_t;
#endif


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_CECMessage_t     Message;

} STVOUT_Ioctl_SendCECMessage_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;

} STVOUT_Ioctl_CECPhysicalAddressAvailable_t;

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVOUT_Handle_t         Handle;
    STVOUT_CECRole_t        Role;

} STVOUT_Ioctl_CECSetAdditionalAddress_t;




