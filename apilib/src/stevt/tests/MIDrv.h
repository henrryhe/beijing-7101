/*****************************************************************************
File Name   : MIDrv.h

Description : 

Copyright (C) 1999 STMicroelectronics

Reference   :

*****************************************************************************/

#ifndef __MI_DRV_H
#define __MI_DRV_H

#include "stddefs.h"  /* standard definitions                    */
#include "stevt.h"
#if !defined(ST_OSLINUX)
#include "stboot.h"
#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif
#endif

#define STDRVA_INVALID_HANDLE  0
#define STDRVB_INVALID_HANDLE  0 
#define STDRVA_VALID_HANDLE          0xFEDCBA98
#define STDRVB_VALID_HANDLE          0xFEDCBA98
#define STDRVA_MAX_DRIVER  10
#define STDRVB_MAX_DRIVER  10 

#define INVALID_EVENT      0
#define STDRVA_EVENTA      1
#define STDRVB_EVENTA      1

typedef U32 ST_Handle_t; 
typedef U32 STDRVA_Handle_t; 
typedef U32 STDRVB_Handle_t; 

typedef struct STDRVA_InitParams_s
{
    ST_DeviceName_t EHName;
    U32             DriverMaxNum;
} STDRVA_InitParams_t;

typedef struct STDRVB_InitParams_s
{
    ST_DeviceName_t EHName;
    U32             DriverMaxNum;
    ST_DeviceName_t RegName;
} STDRVB_InitParams_t;

typedef struct STDRVA_Driver_s
{
    ST_DeviceName_t EHName;
    ST_DeviceName_t DriverName; 
    STDRVA_Handle_t Handle; 
    U32             DriverMaxNum;
    U32             Initialized;
    U32             Event;
    STEVT_Handle_t  EHHandle; 
    STEVT_SubscriberID_t SubscriberID;
    U32             Flag;
} STDRVA_Driver_t;

typedef struct STDRVB_Driver_s
{
    ST_DeviceName_t EHName;
    ST_DeviceName_t DriverName; 
    ST_DeviceName_t RegName;
    STDRVB_Handle_t Handle; 
    U32             DriverMaxNum;
    U32             Initialized;
    U32             Event;
    STEVT_Handle_t  EHHandle; 
    U32             Flag;
} STDRVB_Driver_t;


/*=========================================================================
    Global Variables declarations
=========================================================================== */

/*=========================================================================
    Global function declarations
=========================================================================== */
ST_ErrorCode_t STDRVA_Init ( ST_DeviceName_t DeviceName, 
                             STDRVA_InitParams_t *InitParams_p);
ST_ErrorCode_t STDRVA_Open ( ST_DeviceName_t  DriverName, 
                             ST_Handle_t *Handle_p);
ST_ErrorCode_t STDRVA_EvtHndlOpen ( ST_Handle_t Handle );
ST_ErrorCode_t STDRVA_EvtHndlClose ( ST_Handle_t Handle );
ST_ErrorCode_t STDRVA_RegisterEvent ( STEVT_Handle_t EHHandle );
ST_ErrorCode_t STDRVA_RegisterDeviceEvent ( STEVT_Handle_t EHHandle );
ST_ErrorCode_t STDRVA_UnregisterDeviceEvent ( STEVT_Handle_t EHHandle );
ST_ErrorCode_t STDRVA_NotifyEvent ( STEVT_Handle_t  EHHandle );
ST_ErrorCode_t STDRVA_NotifySubscriberEvent ( STEVT_Handle_t  EHHandle );
ST_ErrorCode_t STDRVA_PutSubscriberID ( STDRVA_Handle_t  DRVAHandle,
                                        STEVT_SubscriberID_t SubscriberID );

ST_ErrorCode_t STDRVB_Init ( ST_DeviceName_t DeviceName, 
                             STDRVB_InitParams_t *InitParams_p);
ST_ErrorCode_t STDRVB_Open ( ST_DeviceName_t  DriverName, 
                             ST_Handle_t *Handle_p);
ST_ErrorCode_t STDRVB_EvtHndlOpen ( ST_Handle_t Handle );
ST_ErrorCode_t STDRVB_EvtHndlClose ( ST_Handle_t Handle );
ST_ErrorCode_t STDRVB_SubscribeEvent ( STEVT_Handle_t EHHandle );
ST_ErrorCode_t STDRVB_SubscribeDeviceEvent ( STEVT_Handle_t EHHandle );
ST_ErrorCode_t STDRVB_UnsubscribeDeviceEvent ( STEVT_Handle_t EHHandle,
                                               ST_DeviceName_t  DriverName );
void STDRVB_NotifyCallBack( STEVT_CallReason_t  Reason, 
                            STEVT_EventConstant_t Event,
                            const void* EventData );

#endif
