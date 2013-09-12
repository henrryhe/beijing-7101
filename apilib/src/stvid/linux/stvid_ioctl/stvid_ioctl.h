/*****************************************************************************
 *
 *  Module      : stvid_ioctl
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description :
 *  Copyright   : STMicroelectronics (c)2006
 *****************************************************************************/


#ifndef STVID_IOCTL_H
#define STVID_IOCTL_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */


#include "stvid.h"

#include "vid_rev.h"
#include "stos.h"

/*** IOCtl defines ***/

#define STVID_MAGIC_NUMBER      63

/* Use STVID_MAGIC_NUMBER as magic number */
#define STVID_IOC_GETREVISION                   _IO(STVID_MAGIC_NUMBER, 0)
#define STVID_IOC_GETCAPABILITY                 _IO(STVID_MAGIC_NUMBER, 1)
#define STVID_IOC_INIT                          _IO(STVID_MAGIC_NUMBER, 2)
#define STVID_IOC_OPEN                          _IO(STVID_MAGIC_NUMBER, 3)
#define STVID_IOC_CLOSE                         _IO(STVID_MAGIC_NUMBER, 4)
#define STVID_IOC_TERM                          _IO(STVID_MAGIC_NUMBER, 5)
#define STVID_IOC_GETSTATISTICS                 _IO(STVID_MAGIC_NUMBER, 6)
#define STVID_IOC_RESETSTATISTICS               _IO(STVID_MAGIC_NUMBER, 7)
#define STVID_IOC_CLEAR                         _IO(STVID_MAGIC_NUMBER, 8)
#define STVID_IOC_CLOSEVIEWPORT                 _IO(STVID_MAGIC_NUMBER, 9)
#define STVID_IOC_CONFIGUREEVENT                _IO(STVID_MAGIC_NUMBER, 10)
#define STVID_IOC_DATAINJECTIONCOMPLETED        _IO(STVID_MAGIC_NUMBER, 11)
#define STVID_IOC_DELETEDATAINPUTINTERFACE      _IO(STVID_MAGIC_NUMBER, 12)
#define STVID_IOC_DISABLEBORDERALPHA            _IO(STVID_MAGIC_NUMBER, 13)
#define STVID_IOC_DISABLECOLORKEY               _IO(STVID_MAGIC_NUMBER, 14)
#define STVID_IOC_DISABLEFRAMERATECONVERSION    _IO(STVID_MAGIC_NUMBER, 15)
#define STVID_IOC_DISABLEOUTPUTWINDOW           _IO(STVID_MAGIC_NUMBER, 16)
#define STVID_IOC_DISABLESYNCHRONISATION        _IO(STVID_MAGIC_NUMBER, 17)
#define STVID_IOC_DUPLICATEPICTURE              _IO(STVID_MAGIC_NUMBER, 18)
#define STVID_IOC_ENABLEBORDERALPHA             _IO(STVID_MAGIC_NUMBER, 19)
#define STVID_IOC_ENABLECOLORKEY                _IO(STVID_MAGIC_NUMBER, 20)
#define STVID_IOC_ENABLEFRAMERATECONVERSION     _IO(STVID_MAGIC_NUMBER, 21)
#define STVID_IOC_ENABLEOUTPUTWINDOW            _IO(STVID_MAGIC_NUMBER, 22)
#define STVID_IOC_ENABLESYNCHRONISATION         _IO(STVID_MAGIC_NUMBER, 23)
#define STVID_IOC_FREEZE                        _IO(STVID_MAGIC_NUMBER, 24)
#define STVID_IOC_GETALIGNIOWINDOWS             _IO(STVID_MAGIC_NUMBER, 25)
#define STVID_IOC_GETBITBUFFERFREESIZE          _IO(STVID_MAGIC_NUMBER, 26)
#define STVID_IOC_GETDATAINPUTBUFFERPARAMS      _IO(STVID_MAGIC_NUMBER, 28)
#define STVID_IOC_GETDECODEDPICTURES            _IO(STVID_MAGIC_NUMBER, 29)
#define STVID_IOC_GETDISPLAYASPECTRATIOCONVERSION  _IO(STVID_MAGIC_NUMBER, 30)
#define STVID_IOC_GETERRORRECOVERYMODE          _IO(STVID_MAGIC_NUMBER, 31)
#define STVID_IOC_VID_GETHDPIPPARAMS            _IO(STVID_MAGIC_NUMBER, 32)
#define STVID_IOC_GETINPUTWINDOWMODE            _IO(STVID_MAGIC_NUMBER, 33)
#define STVID_IOC_GETIOWINDOWS                  _IO(STVID_MAGIC_NUMBER, 34)
#define STVID_IOC_GETMEMORYPROFILE              _IO(STVID_MAGIC_NUMBER, 35)
#define STVID_IOC_GETOUTPUTWINDOWMODE           _IO(STVID_MAGIC_NUMBER, 36)
#define STVID_IOC_GETPICTUREALLOCPARAMS         _IO(STVID_MAGIC_NUMBER, 37)
#define STVID_IOC_GETPICTUREPARAMS              _IO(STVID_MAGIC_NUMBER, 38)
#define STVID_IOC_GETPICTUREINFOS               _IO(STVID_MAGIC_NUMBER, 39)
#define STVID_IOC_GETSPEED                      _IO(STVID_MAGIC_NUMBER, 40)
#define STVID_IOC_GETVIEWPORTALPHA              _IO(STVID_MAGIC_NUMBER, 41)
#define STVID_IOC_GETVIEWPORTCOLORKEY           _IO(STVID_MAGIC_NUMBER, 42)
#define STVID_IOC_GETVIEWPORTPSI                _IO(STVID_MAGIC_NUMBER, 43)
#define STVID_IOC_GETVIEWPORTSPECIALMODE        _IO(STVID_MAGIC_NUMBER, 44)
#define STVID_IOC_HIDEPICTURE                   _IO(STVID_MAGIC_NUMBER, 45)
#define STVID_IOC_INJECTDISCONTINUITY           _IO(STVID_MAGIC_NUMBER, 46)
#define STVID_IOC_OPENVIEWPORT                  _IO(STVID_MAGIC_NUMBER, 47)
#define STVID_IOC_PAUSE                         _IO(STVID_MAGIC_NUMBER, 48)
#define STVID_IOC_PAUSESYNCHRO                  _IO(STVID_MAGIC_NUMBER, 49)
#define STVID_IOC_RESUME                        _IO(STVID_MAGIC_NUMBER, 50)
#define STVID_IOC_SETDATAINPUTINTERFACE         _IO(STVID_MAGIC_NUMBER, 51)
#define STVID_IOC_SETDECODEDPICTURES            _IO(STVID_MAGIC_NUMBER, 52)
#define STVID_IOC_SETDISPLAYASPECTRATIOCONVERSION  _IO(STVID_MAGIC_NUMBER, 53)
#define STVID_IOC_SETERRORRECOVERYMODE          _IO(STVID_MAGIC_NUMBER, 54)
#define STVID_IOC_SETHDPIPPARAMS                _IO(STVID_MAGIC_NUMBER, 55)
#define STVID_IOC_SETINPUTWINDOWMODE            _IO(STVID_MAGIC_NUMBER, 56)
#define STVID_IOC_SETIOWINDOWS                  _IO(STVID_MAGIC_NUMBER, 57)
#define STVID_IOC_SETMEMORYPROFILE              _IO(STVID_MAGIC_NUMBER, 58)
#define STVID_IOC_SETOUTPUTWINDOWMODE           _IO(STVID_MAGIC_NUMBER, 59)
#define STVID_IOC_SETSPEED                      _IO(STVID_MAGIC_NUMBER, 60)
#define STVID_IOC_SETUP                         _IO(STVID_MAGIC_NUMBER, 61)
#define STVID_IOC_SETVIEWPORTALPHA              _IO(STVID_MAGIC_NUMBER, 62)
#define STVID_IOC_SETVIEWPORTCOLORKEY           _IO(STVID_MAGIC_NUMBER, 63)
#define STVID_IOC_SETVIEWPORTPSI                _IO(STVID_MAGIC_NUMBER, 64)
#define STVID_IOC_SETVIEWPORTSPECIALMODE        _IO(STVID_MAGIC_NUMBER, 65)
#define STVID_IOC_SHOWPICTURE                   _IO(STVID_MAGIC_NUMBER, 66)
#define STVID_IOC_SKIPSYNCHRO                   _IO(STVID_MAGIC_NUMBER, 67)
#define STVID_IOC_START                         _IO(STVID_MAGIC_NUMBER, 68)
#define STVID_IOC_STARTUPDATINGDISPLAY          _IO(STVID_MAGIC_NUMBER, 69)
#define STVID_IOC_STEP                          _IO(STVID_MAGIC_NUMBER, 70)
#define STVID_IOC_STOP                          _IO(STVID_MAGIC_NUMBER, 71)
#define STVID_IOC_GETSYNCHRONIZATIONDELAY       _IO(STVID_MAGIC_NUMBER, 72)
#define STVID_IOC_SETSYNCHRONIZATIONDELAY       _IO(STVID_MAGIC_NUMBER, 73)
#define STVID_IOC_GETPICTUREBUFFER              _IO(STVID_MAGIC_NUMBER, 74)
#define STVID_IOC_RELEASEPICTUREBUFFER          _IO(STVID_MAGIC_NUMBER, 75)
#define STVID_IOC_DISPLAYPICTUREBUFFER          _IO(STVID_MAGIC_NUMBER, 76)
#define STVID_IOC_TAKEPICTUREBUFFER             _IO(STVID_MAGIC_NUMBER, 77)
#define STVID_IOC_ENABLEDEBLOCKING              _IO(STVID_MAGIC_NUMBER, 78)
#define STVID_IOC_DISABLEDEBLOCKING             _IO(STVID_MAGIC_NUMBER, 79)
#ifdef STUB_INJECT
#define STVID_IOC_VID_STREAMSIZE                _IO(STVID_MAGIC_NUMBER, 80)
#define STVID_IOC_STUBINJ_GETBBINFO             _IO(STVID_MAGIC_NUMBER, 81)
#endif /*STUB_INJECT */
#define STVID_IOC_DISABLEDISPLAY                _IO(STVID_MAGIC_NUMBER, 82)
#define STVID_IOC_ENABLEDISPLAY                 _IO(STVID_MAGIC_NUMBER, 83)
#ifdef STVID_USE_CRC
#define STVID_IOC_CRCSTARTQUEUEING              _IO(STVID_MAGIC_NUMBER, 84)
#define STVID_IOC_CRCSTOPQUEUEING               _IO(STVID_MAGIC_NUMBER, 85)
#define STVID_IOC_CRCGETQUEUE                   _IO(STVID_MAGIC_NUMBER, 86)
#define STVID_IOC_CRCSTARTCHECK                 _IO(STVID_MAGIC_NUMBER, 87)
#define STVID_IOC_CRCSTOPCHECK                  _IO(STVID_MAGIC_NUMBER, 88)
#define STVID_IOC_CRCGETCURRENTRESULTS          _IO(STVID_MAGIC_NUMBER, 89)
#endif
#ifdef ST_XVP_ENABLE_FLEXVP
#define STVID_IOC_ACTIXVPPROC                   _IO(STVID_MAGIC_NUMBER, 90)
#define STVID_IOC_DEACXVPPROC                   _IO(STVID_MAGIC_NUMBER, 91)
#define STVID_IOC_SHOWXVPPROC                   _IO(STVID_MAGIC_NUMBER, 92)
#define STVID_IOC_HIDEXVPPROC                   _IO(STVID_MAGIC_NUMBER, 93)
#endif /* ST_XVP_ENABLE_FLEXVP */
#define STVID_IOC_GETBITBUFFERPARAMS            _IO(STVID_MAGIC_NUMBER, 94)
#define STVID_IOC_GETDISPLAYPICTUREINFO         _IO(STVID_MAGIC_NUMBER, 95)
#define STVID_IOC_GETSTATUS                     _IO(STVID_MAGIC_NUMBER, 96)
#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
#define STVID_IOC_SETREQUESTEDDEINTERLACINGMODE _IO(STVID_MAGIC_NUMBER, 97)
#define STVID_IOC_GETREQUESTEDDEINTERLACINGMODE _IO(STVID_MAGIC_NUMBER, 98)
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */
#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
#define STVID_IOC_GETVIDEODISPLAYPARAMS         _IO(STVID_MAGIC_NUMBER, 99)
#endif
#define STVID_IOC_SETVIEWPORTQUALITYOPTIMIZATIONS   _IO(STVID_MAGIC_NUMBER, 100)
#define STVID_IOC_GETVIEWPORTQUALITYOPTIMIZATIONS   _IO(STVID_MAGIC_NUMBER, 101)
#define STVID_IOC_AWAIT_CALLBACK                    _IO(STVID_MAGIC_NUMBER, 102)
#define STVID_IOC_RELEASE_CALLBACK                  _IO(STVID_MAGIC_NUMBER, 103)
#define STVID_IOC_FORCEDECIMATIONFACTOR             _IO(STVID_MAGIC_NUMBER, 104)
#define STVID_IOC_GETDECIMATIONFACTOR               _IO(STVID_MAGIC_NUMBER, 105)

#define STVID_ALREADY_MAPPED						        1

/* =================================================================
   Global types declaration
   ================================================================= */
typedef struct VID_Ioctl_GetRevision_s
{
    /* Error code retrieved by STAPI function */
    ST_Revision_t  Revision;

} VID_Ioctl_GetRevision_t;

typedef struct VID_Ioctl_GetCapability_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVID_Capability_t  Capability;

} VID_Ioctl_GetCapability_t;

typedef struct VID_Ioctl_Init_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVID_InitParams_t InitParams;

} VID_Ioctl_Init_t;

typedef struct VID_Ioctl_Open_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVID_OpenParams_t OpenParams;
    STVID_Handle_t Handle;

} VID_Ioctl_Open_t;

typedef struct VID_Ioctl_Close_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_Close_t;

typedef struct VID_Ioctl_Term_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVID_TermParams_t TermParams;

} VID_Ioctl_Term_t;

#ifdef STVID_DEBUG_GET_STATISTICS

typedef struct VID_Ioctl_GetStatistics_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVID_Statistics_t Statistics;

} VID_Ioctl_GetStatistics_t;

typedef struct VID_Ioctl_ResetStatistics_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVID_Statistics_t Statistics;

} VID_Ioctl_ResetStatistics_t;

#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS

typedef struct VID_Ioctl_GetStatus_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STVID_Status_t  Status;

} VID_Ioctl_GetStatus_t;

#endif /* STVID_DEBUG_GET_STATUS */


typedef struct VID_Ioctl_Clear_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_ClearParams_t Params;

} VID_Ioctl_Clear_t;

typedef struct VID_Ioctl_CloseViewPort_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;

} VID_Ioctl_CloseViewPort_t;

typedef struct VID_Ioctl_ConfigureEvent_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STEVT_EventID_t Event;
    STVID_ConfigureEventParams_t Params;

} VID_Ioctl_ConfigureEvent_t;

typedef struct VID_Ioctl_DataInjectionCompleted_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_DataInjectionCompletedParams_t Params;

} VID_Ioctl_DataInjectionCompleted_t;

typedef struct VID_Ioctl_DeleteDataInputInterface_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_DeleteDataInputInterface_t;

typedef struct VID_Ioctl_DisableBorderAlpha_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;

} VID_Ioctl_DisableBorderAlpha_t;

typedef struct VID_Ioctl_DisableColorKey_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;

} VID_Ioctl_DisableColorKey_t;

typedef struct VID_Ioctl_DisableFrameRateConversion_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_DisableFrameRateConversion_t;

typedef struct VID_Ioctl_DisableOutputWindow_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;

} VID_Ioctl_DisableOutputWindow_t;

typedef struct VID_Ioctl_DisableSynchronisation_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_DisableSynchronisation_t;

typedef struct VID_Ioctl_DuplicatePicture_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_PictureParams_t Source;
    STVID_PictureParams_t Dest;

} VID_Ioctl_DuplicatePicture_t;

typedef struct VID_Ioctl_EnableBorderAlpha_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;

} VID_Ioctl_EnableBorderAlpha_t;

typedef struct VID_Ioctl_EnableColorKey_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;

} VID_Ioctl_EnableColorKey_t;

typedef struct VID_Ioctl_EnableFrameRateConversion_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_EnableFrameRateConversion_t;

typedef struct VID_Ioctl_EnableOutputWindow_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;

} VID_Ioctl_EnableOutputWindow_t;

typedef struct VID_Ioctl_EnableSynchronisation_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_EnableSynchronisation_t;

typedef struct VID_Ioctl_Freeze_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_Freeze_t Freeze;

} VID_Ioctl_Freeze_t;

typedef struct VID_Ioctl_GetAlignIOWindows_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    S32 InputWinX;
    S32 InputWinY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    S32 OutputWinX;
    S32 OutputWinY;
    U32 OutputWinWidth;
    U32 OutputWinHeight;


} VID_Ioctl_GetAlignIOWindows_t;

typedef struct VID_Ioctl_GetBitBufferFreeSize_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    U32 FreeSize;

} VID_Ioctl_GetBitBufferFreeSize_t;

typedef struct VID_Ioctl_GetBitBufferParams_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    void * BaseAddress_p;
    U32 InitSize;

} VID_Ioctl_GetBitBufferParams_t;

typedef struct VID_Ioctl_GetDataInputBufferParams_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    void * BaseAddress_p;
    U32 Size;

} VID_Ioctl_GetDataInputBufferParams_t;

typedef struct VID_Ioctl_GetDecodedPictures_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_DecodedPictures_t DecodedPictures;

} VID_Ioctl_GetDecodedPictures_t;

typedef struct VID_Ioctl_GetDisplayAspectRatioConversion_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STVID_DisplayAspectRatioConversion_t Conversion;

} VID_Ioctl_GetDisplayAspectRatioConversion_t;

typedef struct VID_Ioctl_GetErrorRecoveryMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_ErrorRecoveryMode_t Mode;

} VID_Ioctl_GetErrorRecoveryMode_t;

typedef struct VID_Ioctl_GetHDPIPParams_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_HDPIPParams_t HDPIPParams;

} VID_Ioctl_GetHDPIPParams_t;

typedef struct VID_Ioctl_GetInputWindowMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    BOOL AutoMode;
    STVID_WindowParams_t WinParams;

} VID_Ioctl_GetInputWindowMode_t;

typedef struct VID_Ioctl_GetIOWindows_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    S32 InputWinX;
    S32 InputWinY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    S32 OutputWinX;
    S32 OutputWinY;
    U32 OutputWinWidth;
    U32 OutputWinHeight;

} VID_Ioctl_GetIOWindows_t;

typedef struct VID_Ioctl_GetMemoryProfile_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_MemoryProfile_t MemoryProfile;

} VID_Ioctl_GetMemoryProfile_t;

typedef struct VID_Ioctl_GetOutputWindowMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    BOOL AutoMode;
    STVID_WindowParams_t WinParams;

} VID_Ioctl_GetOutputWindowMode_t;

typedef struct VID_Ioctl_GetPictureAllocParams_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_PictureParams_t Params;
    STAVMEM_AllocBlockParams_t AllocParams;

} VID_Ioctl_GetPictureAllocParams_t;

typedef struct VID_Ioctl_GetPictureParams_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_Picture_t PictureType;
    STVID_PictureParams_t Params;

} VID_Ioctl_GetPictureParams_t;

typedef struct VID_Ioctl_GetPictureInfos_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_Picture_t PictureType;
    STVID_PictureInfos_t PictureInfos;

} VID_Ioctl_GetPictureInfos_t;

typedef struct VID_Ioctl_GetDisplayPictureInfo_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_PictureBufferHandle_t PictureBufferHandle;
    STVID_DisplayPictureInfos_t DisplayPictureInfos;

} VID_Ioctl_GetDisplayPictureInfo_t;

typedef struct VID_Ioctl_GetSpeed_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    S32 Speed;

} VID_Ioctl_GetSpeed_t;

typedef struct VID_Ioctl_GetViewPortAlpha_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STLAYER_GlobalAlpha_t GlobalAlpha;

} VID_Ioctl_GetViewPortAlpha_t;

typedef struct VID_Ioctl_GetViewPortColorKey_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STGXOBJ_ColorKey_t ColorKey;

} VID_Ioctl_GetViewPortColorKey_t;

typedef struct VID_Ioctl_GetViewPortPSI_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STLAYER_PSI_t VPPSI;

} VID_Ioctl_GetViewPortPSI_t;

typedef struct VID_Ioctl_GetViewPortSpecialMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STLAYER_OutputMode_t OuputMode;
    STLAYER_OutputWindowSpecialModeParams_t Params;

} VID_Ioctl_GetViewPortSpecialMode_t;

typedef struct VID_Ioctl_HidePicture_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;

} VID_Ioctl_HidePicture_t;

typedef struct VID_Ioctl_InjectDiscontinuity_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_InjectDiscontinuity_t;

typedef struct VID_Ioctl_OpenViewPort_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t VideoHandle;
    STVID_ViewPortParams_t ViewPortParams;
    STVID_ViewPortHandle_t ViewPortHandle;

} VID_Ioctl_OpenViewPort_t;

typedef struct VID_Ioctl_Pause_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_Freeze_t Freeze;

} VID_Ioctl_Pause_t;

typedef struct VID_Ioctl_PauseSynchro_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STTS_t Time;

} VID_Ioctl_PauseSynchro_t;

typedef struct VID_Ioctl_Resume_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_Resume_t;

typedef ST_ErrorCode_t (*STVID_GetWriteAddress_t)(
    void * const Handle,
    void ** const Address_p
);

typedef void (*STVID_InformReadAddress_t)(
    void * const Handle,
    void * const Address_p
);

typedef struct VID_Ioctl_SetDataInputInterface_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_GetWriteAddress_t GetWriteAddress;
    STVID_InformReadAddress_t InformReadAddress;
    void * FunctionsHandle;

} VID_Ioctl_SetDataInputInterface_t;

typedef struct VID_Ioctl_SetDecodedPictures_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_DecodedPictures_t DecodedPictures;

} VID_Ioctl_SetDecodedPictures_t;

typedef struct VID_Ioctl_SetDisplayAspectRatioConversion_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STVID_DisplayAspectRatioConversion_t Mode;

} VID_Ioctl_SetDisplayAspectRatioConversion_t;

typedef struct VID_Ioctl_SetErrorRecoveryMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_ErrorRecoveryMode_t Mode;

} VID_Ioctl_SetErrorRecoveryMode_t;


typedef struct VID_Ioctl_GetViewPortQualityOptimizations_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t  ViewPortHandle;
    STLAYER_QualityOptimizations_t   Optimizations;

} VID_Ioctl_GetViewPortQualityOptimizations_t;

typedef struct VID_Ioctl_SetViewPortQualityOptimizations_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t  ViewPortHandle;
    STLAYER_QualityOptimizations_t       Optimizations;

} VID_Ioctl_SetViewPortQualityOptimizations_t;

typedef struct VID_Ioctl_GetRequestedDeinterlacingMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t  ViewPortHandle;
    STLAYER_DeiMode_t       RequestedMode;

} VID_Ioctl_GetRequestedDeinterlacingMode_t;

typedef struct VID_Ioctl_SetRequestedDeinterlacingMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t  ViewPortHandle;
    STLAYER_DeiMode_t       RequestedMode;

} VID_Ioctl_SetRequestedDeinterlacingMode_t;

typedef struct VID_Ioctl_SetHDPIPParams_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_HDPIPParams_t HDPIPParams;

} VID_Ioctl_SetHDPIPParams_t;

typedef struct VID_Ioctl_SetInputWindowMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    BOOL AutoMode;
    STVID_WindowParams_t WinParams;

} VID_Ioctl_SetInputWindowMode_t;

typedef struct VID_Ioctl_SetIOWindows_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    S32 InputWinX;
    S32 InputWinY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    S32 OutputWinX;
    S32 OutputWinY;
    U32 OutputWinWidth;
    U32 OutputWinHeight;

} VID_Ioctl_SetIOWindows_t;

typedef struct VID_Ioctl_SetMemoryProfile_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_MemoryProfile_t MemoryProfile;

} VID_Ioctl_SetMemoryProfile_t;

typedef struct VID_Ioctl_SetOutputWindowMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    BOOL AutoMode;
    STVID_WindowParams_t WinParams;

} VID_Ioctl_SetOutputWindowMode_t;

typedef struct VID_Ioctl_SetSpeed_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    S32 Speed;

} VID_Ioctl_SetSpeed_t;

typedef struct VID_Ioctl_Setup_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_SetupParams_t SetupParams;

} VID_Ioctl_Setup_t;

typedef struct VID_Ioctl_SetViewPortAlpha_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STLAYER_GlobalAlpha_t GlobalAlpha;

} VID_Ioctl_SetViewPortAlpha_t;

typedef struct VID_Ioctl_SetViewPortColorKey_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STGXOBJ_ColorKey_t ColorKey;

} VID_Ioctl_SetViewPortColorKey_t;

typedef struct VID_Ioctl_SetViewPortPSI_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STLAYER_PSI_t VPPSI;

} VID_Ioctl_SetViewPortPSI_t;

typedef struct VID_Ioctl_SetViewPortSpecialMode_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STLAYER_OutputMode_t OuputMode;
    STLAYER_OutputWindowSpecialModeParams_t Params;

} VID_Ioctl_SetViewPortSpecialMode_t;

typedef struct VID_Ioctl_ShowPicture_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STVID_PictureParams_t Params;
    STVID_Freeze_t Freeze;

} VID_Ioctl_ShowPicture_t;

typedef struct VID_Ioctl_SkipSynchro_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STTS_t Time;

} VID_Ioctl_SkipSynchro_t;

typedef struct VID_Ioctl_Start_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_StartParams_t Params;

} VID_Ioctl_Start_t;

typedef struct VID_Ioctl_StartUpdatingDisplay_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_StartUpdatingDisplay_t;

typedef struct VID_Ioctl_Step_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;

} VID_Ioctl_Step_t;

typedef struct VID_Ioctl_Stop_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_Stop_t StopMode;
    STVID_Freeze_t Freeze;

} VID_Ioctl_Stop_t;

typedef struct VID_Ioctl_EnableDeblocking_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
} VID_Ioctl_EnableDeblocking_t;

typedef struct VID_Ioctl_DisableDeblocking_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
} VID_Ioctl_DisableDeblocking_t;

typedef struct VID_Ioctl_EnableDisplay_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
} VID_Ioctl_EnableDisplay_t;

typedef struct VID_Ioctl_DisableDisplay_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
} VID_Ioctl_DisableDisplay_t;


#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
typedef struct VID_Ioctl_GetSynchronizationDelay_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t VideoHandle;
    S32 SyncDelay;

} VID_Ioctl_GetSynchronizationDelay_t;

typedef struct VID_Ioctl_SetSynchronizationDelay_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t VideoHandle;
    S32 SyncDelay;

} VID_Ioctl_SetSynchronizationDelay_t;
#endif /* STVID_ENABLE_SYNCHRONIZATION_DELAY */

typedef struct VID_Ioctl_GetPictureBuffer_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_GetPictureBufferParams_t Params;
    STVID_PictureBufferDataParams_t PictureBufferParams;
    STVID_PictureBufferHandle_t PictureBufferHandle;

} VID_Ioctl_GetPictureBuffer_t;


typedef struct VID_Ioctl_GetVideoDisplayParams_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t ViewPortHandle;
    STLAYER_VideoDisplayParams_t Params;

} VID_Ioctl_GetVideoDisplayParams_t;

typedef struct VID_Ioctl_ReleasePictureBuffer_t
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_PictureBufferHandle_t  PictureBufferHandle;

} VID_Ioctl_ReleasePictureBuffer_t;

typedef struct VID_Ioctl_DisplayPictureBuffer_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_PictureBufferHandle_t  PictureBufferHandle;
    STVID_PictureInfos_t PictureInfos;

} VID_Ioctl_DisplayPictureBuffer_t;

typedef struct VID_Ioctl_TakePictureBuffer_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_PictureBufferHandle_t  PictureBufferHandle;

} VID_Ioctl_TakePictureBuffer_t;

#ifdef STUB_INJECT
typedef struct VID_Ioctl_StreamSize_s
{
    U32         Size;
} VID_Ioctl_StreamSize_t;

typedef struct VID_Ioctl_StubInjectGetBBInfo_s
{
    void       *BaseAddress_p;
    U32         Size;
} VID_Ioctl_StubInjectGetBBInfo_t;
#endif  /* STUB_INJECT */

#ifdef ST_XVP_ENABLE_FLEXVP
typedef struct VID_Ioctl_XVP_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_ViewPortHandle_t  ViewPortHandle;
    STLAYER_ProcessId_t     ProcessID;
} VID_Ioctl_XVP_t;
#endif /* ST_XVP_ENABLE_FLEXVP */

#ifdef STVID_USE_CRC
typedef struct VID_Ioctl_CRCStartQueueing_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
} VID_Ioctl_CRCStartQueueing_t;

typedef struct VID_Ioctl_CRCStopQueueing_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
} VID_Ioctl_CRCStopQueueing_t;

typedef struct VID_Ioctl_CRCGetQueue_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_CRCReadMessages_t  CRCReadMessages;
} VID_Ioctl_CRCGetQueue_t;

typedef struct VID_Ioctl_CRCStartCheck_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_CRCStartParams_t  StartParams;
} VID_Ioctl_CRCStartCheck_t;

typedef struct VID_Ioctl_CRCStopCheck_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
} VID_Ioctl_CRCStopCheck_t;

typedef struct VID_Ioctl_CRCGetCurrentResults_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_CRCCheckResult_t  CRCCheckResult;
} VID_Ioctl_CRCGetCurrentResults_t;
#endif

typedef enum
{
    STVID_CB_WRITE_PTR,
    STVID_CB_READ_PTR
} STVID_Ioctl_CallbackType_t;

typedef struct
{
    /* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    STVID_Ioctl_CallbackType_t  CBType;
    BOOL                        Terminate;
    void*                Handle;
    void*                Address;
} STVID_Ioctl_CallbackData_t;

typedef struct VID_Ioctl_ForceDecimationFactor_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_ForceDecimationFactorParams_t ForceDecimationFactorParams;
} VID_Ioctl_ForceDecimationFactor_t;

typedef struct VID_Ioctl_GetDecimationFactor_s
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STVID_Handle_t Handle;
    STVID_DecimationFactor_t DecimationFactor;
} VID_Ioctl_GetDecimationFactor_t;

#endif  /* STVID_IOCTL_H */
