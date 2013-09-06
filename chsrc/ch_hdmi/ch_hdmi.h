/*******************************************************************************

File Name   : hdmi.h
Description : Header file for hdmi.c source file

COPYRIGHT (C) STMicroelectronics 2006.

*******************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __HDMI_H
#define __HDMI_H

/* Includes ----------------------------------------------------------------- */
#include "sthdmi.h"

/* Exported Constants -------------------------------------------------- */
#define HDMI_MAXOPEN     1
#define EVT_MAXOPEN           20
#define VOUT_MAXOPEN     3
#define VTG_MAXOPEN       3
#define HDMI_DEVICE_NUMBER 1
/* Exported Macros ----------------------------------------------------- */
#define HDMI_HANDLE_NULL                (STHDMI_Handle_t)NULL

/* Exported Types ----------------------------------------------------------- */
typedef enum
{
    HDMI = 0,
    HDMI_MAXDEVICE = 1
} HDMI_DeviceId_t;

/* HandleId for multi-instance access */
typedef U32 HDMI_HandleId_t;

/* HandleId for multi-instance access */
typedef U32 EVT_HandleId_t;

/* HandleId for multi-instance access */
typedef U32 VOUT_HandleId_t;

/* HandleId for multi-instance access */
typedef U32 VTG_HandleId_t;

/* -------------------------- EVT ----------------------------------------- */
/* DeviceId EVT_xxxx (quick access to hardware) */
typedef enum
{
    EVT_BACK = 0,
    EVT_MAXDEVICE = 1                  /* for range test */
} EVT_DeviceId_t;


/* -------------------------- VOUT ---------------------------------------- */
/* DeviceId VOUT_xxxx (quick access to hardware) */
typedef enum
{
    VOUT_SD_D = 0,
    VOUT_HD_D = 1,
    VOUT_HDMI_D = 2,
    VOUT_MAXDEVICE_D = 3                 /* for range test */
} VOUT_DeviceId_t;


/* -------------------------- VTG ----------------------------------------- */
/* DeviceId VTG_xxxx (quick access to hardware) */
typedef enum
{
    VTG_MAIN_D = 0,
    VTG_AUX_D = 1,
    VTG_MAXDEVICE_D = 2                 /* for range test */
} VTG_DeviceId_t;


/* -------------------------- HDMI ---------------------------------------- */
typedef struct HDMI_DeviceHandle_s
{
    ST_DeviceName_t          DeviceName;
    STHDMI_Handle_t          *Handle_p;
    STHDMI_DeviceType_t      DeviceType;
    EVT_DeviceId_t           EVT_DeviceId;
    VOUT_DeviceId_t          VOUT_DeviceId;
    VTG_DeviceId_t           VTG_DeviceId;
    ST_DeviceName_t       AUD_DeviceName;
    STVOUT_State_t       VOUT_State;
    BOOL                       HDMI_IsEnabled;
    U32                          HDMI_AudioFrequency;
    semaphore_t *            TaskWakeUp_p ;
} HDMI_DeviceHandle_t;

/* Exported Variables ------------------------------------------------------- */
extern HDMI_DeviceHandle_t HdmiDeviceHandle[];

/* Exported Functions ------------------------------------------------------- */
/*---------------------------------------------------------------------
 *            Get HDMI Capabilities.
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_GetCapability(ST_DeviceName_t HDMI_DeviceName);

/*---------------------------------------------------------------------
 *            Get HDMI Sink Capabilities.
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_GetSinkCapability(ST_DeviceName_t HDMI_DeviceName);

/*---------------------------------------------------------------------
 *            Get HDMI Source Capabilities.
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_GetSourceCapability(ST_DeviceName_t HDMI_DeviceName);

/*---------------------------------------------------------------------
 *            Close HDMI handle.
 * ----------------------------------------------------------------- */
ST_ErrorCode_t THDMI_Close(HDMI_HandleId_t HDMI_HandleId);

/*---------------------------------------------------------------------
 *            Terminates HDMI
 * ----------------------------------------------------------------- */
ST_ErrorCode_t THDMI_Term(HDMI_DeviceId_t HDMI_DeviceId);

ST_ErrorCode_t HDMI_FillAVI(HDMI_HandleId_t HDMI_HandleId);

ST_ErrorCode_t HDMI_FillSPD(HDMI_HandleId_t HDMI_HandleId);

ST_ErrorCode_t HDMI_FillAudio(HDMI_HandleId_t HDMI_HandleId);

ST_ErrorCode_t HDMI_FillMS(HDMI_HandleId_t HDMI_HandleId);

ST_ErrorCode_t HDMI_FillVS(HDMI_HandleId_t HDMI_HandleId);

/*--------------------------------------------------------------------------- */

ST_ErrorCode_t HDMI_EnableOutput(U32 DeviceId);
ST_ErrorCode_t HDMI_DisableOutput(U32 DeviceId);
ST_ErrorCode_t HDMI_EnableInfoframe(U32 DeviceId);
#if defined (TESTAPP_HDCP_ON)
    ST_ErrorCode_t EnableDefaultOutput(STVOUT_Handle_t *VOUT_Handle);
#endif

#endif /*#ifndef __HDMI_H*/

/* End of hdmi.h */

