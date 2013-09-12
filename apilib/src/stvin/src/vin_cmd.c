/*******************************************************************************

File name   : vin_cmd.c

Description : VIN module commands

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 Sep 2000        Created                                           JA
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>
#endif /*ST_OSLINUX*/

#include "sttbx.h"

#include "stvin.h"
#include "vin_drv.h"
#include "halv_vin.h"
#include "vin_cmd.h"

/* Private Types ------------------------------------------------------------ */

typedef struct
{
    U16 FrameWidth;
    U16 FrameHeight;
} STVIN_CmdStandardSize_t;

/* Private Constants -------------------------------------------------------- */
static const STVIN_CmdStandardSize_t CmdStandardSize[] =
{
    { 720, 576 },       /* STVIN_STANDARD_PAL */
    { 768, 576 },       /* STVIN_STANDARD_PAL_SQ */
    { 720, 480 },       /* STVIN_STANDARD_NTSC */
    { 640, 480 }        /* STVIN_STANDARD_NTSC_SQ */
};

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */
/*******************************************************************************
Name        : STVIN_GetParams
Description :
Parameters  : Video Input Handle, Pointer to
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED,
*******************************************************************************/
ST_ErrorCode_t STVIN_GetParams(const STVIN_Handle_t Handle, STVIN_VideoParams_t * const VideoParams_p, STVIN_DefInputMode_t * const DefInputMode_p)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (VideoParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);


    /* Copy all Params */
    memcpy(VideoParams_p, &(Device_p->VideoParams), sizeof(STVIN_VideoParams_t));
    *DefInputMode_p = HALVIN_CheckInputParam(Device_p->DeviceType, VideoParams_p->InputType, VideoParams_p->SyncType);

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIN_SetSyncType
Description : Define the stream synchronisation type
Parameters  : Video Input Handle, Synchronisation type
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED,
*******************************************************************************/
ST_ErrorCode_t STVIN_SetSyncType(const STVIN_Handle_t Handle, const STVIN_SyncType_t SyncType)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    HALVIN_Handle_t HALHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);
    HALHandle = (HALVIN_Handle_t)(Device_p->HALInputHandle);

    if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible while Input running !"));
        return(ST_ERROR_DEVICE_BUSY);
    }

    /* TODO: make some check on parameters */
    if (Device_p->VideoParams.SyncType != SyncType)
    {
        ErrorCode = HALVIN_SetSyncType(HALHandle, SyncType);
        if (ErrorCode == ST_NO_ERROR)
        {
            Device_p->VideoParams.SyncType = SyncType;
        }
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIN_SetFrameRate
Description : Set the input frame rate according to the input mode
Parameters  : Video Input Handle, FrameRate
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED,
*******************************************************************************/
ST_ErrorCode_t STVIN_SetFrameRate(const STVIN_Handle_t Handle, const U32 FrameRate)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);

    if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible while Input is running !"));
        return(ST_ERROR_DEVICE_BUSY);
    }

    if (Device_p->InputFrameRate != FrameRate)
    {
        /* 7109 */
        if (Device_p->DeviceType == STVIN_DEVICE_TYPE_ST7109_DVP_INPUT)
        {
            /* 1920x1080 input mode */
            if (Device_p->InputMode == STVIN_HD_YCbCr_1920_1080_I_CCIR)
            {
                if((FrameRate != STVIN_FRAME_RATE_50I) && (FrameRate != STVIN_FRAME_RATE_60I))
                {
                     STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Input Frame Rate not supported with HD_YCbCr_1920_1080_I_CCIR input mode!"));
                     return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }

            /* 1280x720 input mode */
            if (Device_p->InputMode == STVIN_HD_YCbCr_1280_720_P_CCIR)
            {
                if((FrameRate != STVIN_FRAME_RATE_50P) && (FrameRate != STVIN_FRAME_RATE_60P))
                {
                     STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Input Frame Rate not supported with HD_YCbCr_1280_720_P_CCIR input mode!"));
                     return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }

            /* Setting new frame rate value */
            Device_p->InputFrameRate = FrameRate;
        }
    }

    return(ErrorCode);
} /* End of STVIN_SetFrameRate */


/*******************************************************************************
Name        : STVIN_GetFrameRate
Description : Get the input frame rate
Parameters  : Video Input Handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVIN_GetFrameRate(const STVIN_Handle_t Handle, U32 * const FrameRate_p)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Bad parameters */
    if (FrameRate_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvin_Unit_t *) Handle;
        Device_p = (stvin_Device_t *)(Unit_p->Device_p);

        *FrameRate_p = Device_p->InputFrameRate;
    }

    return(ErrorCode);
} /* End of STVIN_GetFrameRate*/


/*******************************************************************************
Name        : STVIN_SetInputWindowMode
Description : Set the input window mode
Parameters  : Video Input Handle, input automode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_DEVICE_BUSY
*******************************************************************************/
ST_ErrorCode_t STVIN_SetInputWindowMode(const STVIN_Handle_t Handle, const BOOL AutoMode)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);

    if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible while Input is running !"));
        return(ST_ERROR_DEVICE_BUSY);
    }

    Device_p->InputWinAutoMode = AutoMode;

    return(ErrorCode);

} /* End of STVIN_SetInputWindowMode */


/*******************************************************************************
Name        : STVIN_GetInputWindowMode
Description : Get the Input Window Mode
Parameters  :
Assumptions :
Limitations :
Returns     :  ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVIN_GetInputWindowMode( const STVIN_Handle_t Handle, BOOL * const AutoMode_p )
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (AutoMode_p == NULL)
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);

    *AutoMode_p = Device_p->InputWinAutoMode;

    return(ErrorCode);
} /* End of STVIN_GetInputWindowMode() function */


/*******************************************************************************
Name        : STVIN_SetOutputWindowMode
Description : Set the output window mode
Parameters  : Video Output Handle, output automode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_DEVICE_BUSY
*******************************************************************************/
ST_ErrorCode_t STVIN_SetOutputWindowMode(const STVIN_Handle_t Handle, const BOOL AutoMode)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);

    if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible while Input is running !"));
        return(ST_ERROR_DEVICE_BUSY);
    }

    Device_p->OutputWinAutoMode = AutoMode;

    return(ErrorCode);

} /* End of STVIN_SetOutputWindowMode */


/*******************************************************************************
Name        : STVIN_GetOutputWindowMode
Description : Get the Output Window Mode
Parameters  :
Assumptions :
Limitations :
Returns     :  ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVIN_GetOutputWindowMode( const STVIN_Handle_t Handle, BOOL * const AutoMode_p )
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (AutoMode_p == NULL)
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);

    *AutoMode_p = Device_p->OutputWinAutoMode;

    return(ErrorCode);
} /* End of STVIN_GetOutputWindowMode() function */


/*******************************************************************************
Name        : STVIN_SetIOWindows
Description : To set the input & output windows.
Parameters  : Video Input Handle, input & output sizes.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_DEVICE_BUSY
*******************************************************************************/
ST_ErrorCode_t STVIN_SetIOWindows(
        const STVIN_Handle_t            Handle,
        S32 const InputWinX,            S32 const InputWinY,
        U32 const InputWinWidth,        U32 const InputWinHeight,
        U32 const OutputWinWidth,       U32 const OutputWinHeight)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALVIN_Handle_t HALHandle;
    HALVIN_Properties_t *HALVIN_Data_p;
    S32 IWinX, IWinY;
    U32 IWinWidth, IWinHeight, OWinWidth, OWinHeight;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);

    if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible while Input is running !"));
        return(ST_ERROR_DEVICE_BUSY);
    }

    /* Input or Output Manual Mode */
    if ((Device_p->InputWinAutoMode != TRUE) || (Device_p->OutputWinAutoMode != TRUE))
    {
        HALHandle = (HALVIN_Handle_t)Device_p->HALInputHandle;
        HALVIN_Data_p = (HALVIN_Properties_t *) HALHandle;

        IWinX = HALVIN_Data_p->InputWindow.InputWinX;
        IWinY = HALVIN_Data_p->InputWindow.InputWinY;
        IWinWidth = HALVIN_Data_p->InputWindow.InputWinWidth;
        IWinHeight = HALVIN_Data_p->InputWindow.InputWinHeight;
        OWinWidth = HALVIN_Data_p->InputWindow.OutputWinWidth;
        OWinHeight = HALVIN_Data_p->InputWindow.OutputWinHeight;

        /* Input Manual Mode */
        if (Device_p->InputWinAutoMode != TRUE)
        {
            IWinX = InputWinX;
            IWinY = InputWinY;
            IWinWidth = InputWinWidth;
            IWinHeight = InputWinHeight;
        }

        /* Output Manual Mode */
        if (Device_p->OutputWinAutoMode != TRUE)
        {
            OWinWidth = OutputWinWidth;
            OWinHeight = OutputWinHeight;
        }

        switch (Device_p->DeviceType)
        {
            case STVIN_DEVICE_TYPE_ST7710_DVP_INPUT   :
            case STVIN_DEVICE_TYPE_ST7100_DVP_INPUT   :
                if( (IWinWidth != OWinWidth) || (IWinHeight != OWinHeight) )
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
                break;

            case STVIN_DEVICE_TYPE_ST7109_DVP_INPUT   :
                if( IWinHeight != OWinHeight )
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
                break;

            default:
                break;
        }

        ErrorCode = HALVIN_SetIOWindow(HALHandle, IWinX, IWinY, IWinWidth, IWinHeight, OWinWidth, OWinHeight);

        /* SetReconstructedFrameSize */
        if ( ErrorCode == ST_NO_ERROR)
        {
           if (Device_p->OutputWinAutoMode != TRUE)
           {
              switch (Device_p->DeviceType)
              {
                case STVIN_DEVICE_TYPE_ST5528_DVP_INPUT   :
                case STVIN_DEVICE_TYPE_ST7710_DVP_INPUT   :
                case STVIN_DEVICE_TYPE_ST7100_DVP_INPUT   :
                case STVIN_DEVICE_TYPE_ST7109_DVP_INPUT   :
                    switch (Device_p->InputMode) {
                        case STVIN_HD_YCbCr_1280_720_P_CCIR :
                        case STVIN_HD_YCbCr_720_480_P_CCIR :
                        case STVIN_HD_RGB_1024_768_P_EXT :
                        case STVIN_HD_RGB_800_600_P_EXT :
                        case STVIN_HD_RGB_640_480_P_EXT :
                            HALVIN_SetReconstructedFrameSize(Device_p->HALInputHandle,
                                2 * OWinWidth,
                                Device_p->VideoParams.FrameHeight);
                            break;

                        case STVIN_SD_NTSC_720_480_I_CCIR :
                        case STVIN_SD_NTSC_640_480_I_CCIR :
                        case STVIN_SD_PAL_720_576_I_CCIR :
                        case STVIN_SD_PAL_768_576_I_CCIR :
                        case STVIN_HD_YCbCr_1920_1080_I_CCIR :
                        default :
                            HALVIN_SetReconstructedFrameSize(Device_p->HALInputHandle,
                                4 * OWinWidth,
                                Device_p->VideoParams.FrameHeight);
                            break;
                    }
                    break;

                default :
                    HALVIN_SetReconstructedFrameSize(Device_p->HALInputHandle,
                                                    Device_p->VideoParams.FrameWidth,
                                                    Device_p->VideoParams.FrameHeight);
                    break;
              }
           }
           /* This change must be applyed at next VSync */
           PushVSyncControllerCommand(Device_p, SYNC_SET_IO_WINDOW);
        }
    }

    return(ErrorCode);
} /* End of STVIN_SetIOWindows */


/*******************************************************************************
Name        : STVIN_GetIOWindows
Description : Get the IO Window
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVIN_GetIOWindows(
        const STVIN_Handle_t            Handle,
        S32 * const InputWinX_p,        S32 * const InputWinY_p,
        U32 * const InputWinWidth_p,    U32 * const InputWinHeight_p,
        U32 * const OutputWinWidth_p,   U32 * const OutputWinHeight_p)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALVIN_Handle_t HALHandle;
    HALVIN_Properties_t *HALVIN_Data_p;


    if ((InputWinX_p == NULL) || (InputWinY_p == NULL) ||
        (InputWinWidth_p == NULL) || (InputWinHeight_p == NULL) ||
        (OutputWinWidth_p == NULL) || (OutputWinHeight_p == NULL))
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the Vin handle */
    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvin_Unit_t *) Handle;
        Device_p = (stvin_Device_t *)(Unit_p->Device_p);
        HALHandle = (HALVIN_Handle_t)Device_p->HALInputHandle;
        HALVIN_Data_p = (HALVIN_Properties_t *) HALHandle;

        *InputWinX_p      = HALVIN_Data_p->InputWindow.InputWinX;
        *InputWinY_p      = HALVIN_Data_p->InputWindow.InputWinY;
        *InputWinWidth_p  = HALVIN_Data_p->InputWindow.InputWinWidth;
        *InputWinHeight_p = HALVIN_Data_p->InputWindow.InputWinHeight;
        *OutputWinWidth_p = HALVIN_Data_p->InputWindow.OutputWinWidth;
        *OutputWinHeight_p = HALVIN_Data_p->InputWindow.OutputWinHeight;
    }

    return(ErrorCode);
} /* End of STVIN_GetIOWindows() function */


/*******************************************************************************
Name        : STVIN_SetScanType
Description : Defines the possible television display and stream scan types
Parameters  : Video Input Handle, Defines the possible television
              display and stream scan types
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED,
*******************************************************************************/
ST_ErrorCode_t STVIN_SetScanType(const STVIN_Handle_t Handle, const STGXOBJ_ScanType_t ScanType)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    HALVIN_Handle_t HALHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVID_MemoryProfile_t STVID_MemoryProfile;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);
    HALHandle = (HALVIN_Handle_t)(Device_p->HALInputHandle);

    if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible while Input running !"));
        return(ST_ERROR_DEVICE_BUSY);
    }

    /* TODO: make some check on parameters */
    if (Device_p->VideoParams.ScanType != ScanType)
    {
        /* Check Memory Profile */
        /* Get the memory profile of THIS decode/display process. */
        ErrorCode = STVID_GetMemoryProfile(Device_p->VideoHandle, &STVID_MemoryProfile);
        if (ErrorCode!=ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "STVIN_SetScanType() : Impossible to get memory profile from STVID !"));
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Update ScanType:  */
        ErrorCode = HALVIN_SetReconstructedStorageMemoryMode(
            Device_p->HALInputHandle,
            STVID_MemoryProfile.DecimationFactor & ~(STVID_DECIMATION_FACTOR_V2|STVID_DECIMATION_FACTOR_V4),
            STVID_MemoryProfile.DecimationFactor & ~(STVID_DECIMATION_FACTOR_H2|STVID_DECIMATION_FACTOR_H4),
            ScanType);

        if (ErrorCode!=ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "STVIN_SetScanType() : HALVIN_SetReconstructedStorageMemoryMode !"));
            return(ST_ERROR_BAD_PARAMETER);
        }

        ErrorCode = HALVIN_SetScanType(HALHandle, ScanType);
        if (ErrorCode == ST_NO_ERROR)
        {
            Device_p->VideoParams.ScanType = ScanType;
        }
    }
    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIN_SetInputType
Description : Defines the operational mode of the interface
Parameters  : Video Input Handle, Mode of the interface
Assumptions :
Limitations : The InputType must be STVIN_INPUT_DIGITAL_YCbCr422_8_MULT in SD mode.
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED,
*******************************************************************************/
ST_ErrorCode_t STVIN_SetInputType(const STVIN_Handle_t Handle,
                                  const STVIN_InputType_t InputType)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    HALVIN_Handle_t HALHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);
    HALHandle = (HALVIN_Handle_t)(Device_p->HALInputHandle);

    if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible while Input running !"));
        return(ST_ERROR_DEVICE_BUSY);
    }

    if (Device_p->VideoParams.InputType != InputType)
    {
        STVIN_DefInputMode_t DefInputMode;
        DefInputMode = HALVIN_CheckInputParam(Device_p->DeviceType, InputType, Device_p->VideoParams.SyncType);
        if (DefInputMode == STVIN_INPUT_INVALID)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }

        switch (InputType)
        {
            case STVIN_INPUT_DIGITAL_YCbCr422_8_MULT:
                ErrorCode = HALVIN_SelectInterfaceMode(HALHandle, HALVIN_SD_PIXEL_INTERFACE);
                if (ErrorCode == ST_NO_ERROR)
                {
                    Device_p->VideoParams.InputType = InputType;
                }
                break;
            case STVIN_INPUT_DIGITAL_YCbCr422_16_CHROMA_MULT:
                ErrorCode = HALVIN_SelectInterfaceMode(HALHandle, HALVIN_EMBEDDED_SYNC_MODE);
                if (ErrorCode == ST_NO_ERROR)
                {
                    ErrorCode = HALVIN_SetConversionMode(HALHandle, HALVIN_DISABLE_RGB_TO_YCC_CONVERSION);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        Device_p->VideoParams.InputType = InputType;
                    }
                }
                break;
            case STVIN_INPUT_DIGITAL_YCbCr444_24_COMP:
                ErrorCode = HALVIN_SelectInterfaceMode(HALHandle, HALVIN_LUMA_CHROMA_MODE);
                if (ErrorCode == ST_NO_ERROR)
                {
                    ErrorCode = HALVIN_SetConversionMode(HALHandle, HALVIN_DISABLE_RGB_TO_YCC_CONVERSION);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        Device_p->VideoParams.InputType = InputType;
                    }
                }
                break;
            case STVIN_INPUT_DIGITAL_RGB888_24_COMP_TO_YCbCr422:
                ErrorCode = HALVIN_SelectInterfaceMode(HALHandle, HALVIN_LUMA_CHROMA_MODE);
                if (ErrorCode == ST_NO_ERROR)
                {
                    ErrorCode = HALVIN_SetConversionMode(HALHandle, HALVIN_ENABLED_RGB_TO_YCC_CONVERSION);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        Device_p->VideoParams.InputType = InputType;
                    }
                }
                break;
            case STVIN_INPUT_DIGITAL_RGB888_24_COMP:
                ErrorCode = HALVIN_SelectInterfaceMode(HALHandle, HALVIN_RGB_MODE);
                if (ErrorCode == ST_NO_ERROR)
                {
                    ErrorCode = HALVIN_SetConversionMode(HALHandle, HALVIN_DISABLE_RGB_TO_YCC_CONVERSION);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        Device_p->VideoParams.InputType = InputType;
                    }
                }
                break;
            case STVIN_INPUT_DIGITAL_CD_SERIAL_MULT:
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
            default:
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }
        if (ErrorCode == ST_NO_ERROR)
        {
            /* TODO: replace this by HAL call*/
            /* STEVT_Notify(Device_p->EventsHandle, Device_p->RegisteredEventsID[STVIN_CONFIGURATION_CHANGE_EVT_ID], NULL); */
        }
    }
    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIN_SetAncillaryParameters
Description : Defines the format and the size of the incoming ancillary data
Parameters  : Video Input Handle,
              Format of the incoming ancillary data,
              Size of the incoming ancillary data
Assumptions :
Limitations : The DataCaptureLength has effect only if DataType is
              STVIN_ANC_DATA_CAPTURE (Raw Mode) and if DataCaptureLength is
              null, then any ancillary data capture will be cancelled.
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED,
*******************************************************************************/
ST_ErrorCode_t STVIN_SetAncillaryParameters(const STVIN_Handle_t Handle,
                                            const STVIN_AncillaryDataType_t DataType,
                                            const U16 DataCaptureLength)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    HALVIN_Handle_t HALHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);
    HALHandle = (HALVIN_Handle_t)(Device_p->HALInputHandle);

    if (Device_p->VideoParams.SyncType == STVIN_SYNC_TYPE_EXTERNAL)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible while Input running !"));
        return(ST_ERROR_DEVICE_BUSY);
    }

    if (!Device_p->AncillaryDataTableAllocated)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "No ancillary data table allocated. Can't set any mode"));
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    switch (DataType)
    {
        case STVIN_ANC_DATA_RAW_CAPTURE:
            /* Default case : reset ancillary data capture flag.    */
            Device_p->AncillaryDataEventEnabled = FALSE;

            if (DataCaptureLength == 0)
            {
                HALVIN_DisableAncillaryDataCapture (HALHandle);
                Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataType
                        = DataType;
                Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength
                        = DataCaptureLength;
            }
            else
            {

                /* Test if the wanted length is greater than buffer's size. */
                if (DataCaptureLength > Device_p->AncillaryDataBufferSize)
                {
                    return(ST_ERROR_BAD_PARAMETER);
                }

                ErrorCode = HALVIN_SetAncillaryDataType(HALHandle,
                        HALVIN_ANCILLARY_DATA_CAPTURE_MODE);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Size of the Data Capture / Only pertinent in "STVIN_ANC_DATA_CAPTURE" mode */
                    ErrorCode = HALVIN_SetAncillaryDataCaptureLength(HALHandle, DataCaptureLength);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataType
                                = DataType;
                        Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength
                                = DataCaptureLength;

                        Device_p->AncillaryDataEventEnabled = TRUE;
                    }
                }
            }
            break;
        case STVIN_ANC_DATA_PACKET_DIRECT:
            ErrorCode = HALVIN_SetAncillaryDataType(HALHandle, HALVIN_ANCILLARY_DATA_PACKETS_MODE);
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = HALVIN_SetAncillaryEncodedMode(HALHandle, HALVIN_ANCILLARY_DATA_STD_ENCODED);
                if (ErrorCode == ST_NO_ERROR)
                {
                    Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataType = DataType;
                    Device_p->AncillaryDataEventEnabled = TRUE;
                }
            }
            break;
        case STVIN_ANC_DATA_PACKET_NIBBLE_ENCODED:
            ErrorCode = HALVIN_SetAncillaryDataType(HALHandle, HALVIN_ANCILLARY_DATA_PACKETS_MODE);
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = HALVIN_SetAncillaryEncodedMode(HALHandle, HALVIN_ANCILLARY_DATA_NIBBLE_ENCODED);
                if (ErrorCode == ST_NO_ERROR)
                {
                    Device_p->VideoParams.ExtraParams.StandardDefinitionParams.AncillaryDataType = DataType;
                    Device_p->AncillaryDataEventEnabled = TRUE;
                }
            }
            break;
        default:
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    if ( (ErrorCode == ST_NO_ERROR) && (Device_p->AncillaryDataEventEnabled == TRUE) )
    {
        Device_p->LastPositionInAncillaryDataBuffer = 0;
#ifdef WA_WRONG_CIRCULAR_DATA_WRITE
        Device_p->CircularLoopDone = FALSE;
#endif /* WA_WRONG_CIRCULAR_DATA_WRITE */

        /* TODO: replace this by HAL call*/
        /* STEVT_Notify(Device_p->EventsHandle, Device_p->RegisteredEventsID[STVIN_CONFIGURATION_CHANGE_EVT_ID], NULL); */
        HALVIN_EnableAncillaryDataCapture (HALHandle);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVIN_SetParams
Description :
Parameters  : Video Input Handle, Pointer to
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED,
*******************************************************************************/
ST_ErrorCode_t STVIN_SetParams(const STVIN_Handle_t Handle, STVIN_VideoParams_t * const VideoParams_p)
{
    stvin_Unit_t *Unit_p;
    stvin_Device_t *Device_p;
    HALVIN_Handle_t HALHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVIN_VideoParams_t *CurrentVideoParams_p;
    int CodeRet;
    STVIN_DefInputMode_t CurrentInputMode, NewInputMode;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (VideoParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Unit_p = (stvin_Unit_t *) Handle;
    Device_p = (stvin_Device_t *)(Unit_p->Device_p);
    HALHandle = (HALVIN_Handle_t)(Device_p->HALInputHandle);

    if (Device_p->Status == VIN_DEVICE_STATUS_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible while Input running !"));
        return(ST_ERROR_DEVICE_BUSY);
    }

    CurrentVideoParams_p = &(Device_p->VideoParams);

    /* We Must Check all changed Params */
    CodeRet = memcmp((const char *) VideoParams_p, (const char *) CurrentVideoParams_p, sizeof(STVIN_VideoParams_t));
    if (CodeRet != 0) {

        /* We have to check one by one all the parameters...*/

        /* Check Init parameter */
        CurrentInputMode = HALVIN_CheckInputParam(Device_p->DeviceType, CurrentVideoParams_p->InputType, CurrentVideoParams_p->SyncType);
        NewInputMode = HALVIN_CheckInputParam(Device_p->DeviceType, VideoParams_p->InputType, VideoParams_p->SyncType);

        if (CurrentInputMode != NewInputMode)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "You can't switch SD <=> HD !!"));
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Input Type -  RGB / YCbCr */
        if (CurrentVideoParams_p->InputType != VideoParams_p->InputType) {
            ErrorCode = STVIN_SetInputType(Handle, VideoParams_p->InputType);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change InputType!"));
                return(ErrorCode);
            }
        }

        /* ScanType - Interlace / Progressif */
        if (CurrentVideoParams_p->ScanType != VideoParams_p->ScanType) {
            ErrorCode = STVIN_SetScanType(Handle, VideoParams_p->ScanType);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change ScanType!"));
                return(ErrorCode);
            }
        }

        /* SyncType -  CCIR / External */
        if (CurrentVideoParams_p->SyncType != VideoParams_p->SyncType) {
            ErrorCode = STVIN_SetSyncType(Handle, VideoParams_p->SyncType);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change SyncType!"));
                return(ErrorCode);
            }
        }

        /* DVO/PAD */
        if (CurrentVideoParams_p->InputPath != VideoParams_p->InputPath) {
            ErrorCode = HALVIN_SetInputPath(HALHandle, VideoParams_p->InputPath);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change InputPath!"));
                return(ErrorCode);
            }
            CurrentVideoParams_p->InputPath = VideoParams_p->InputPath;
        }

        /* Size of the Frame -  Width & Height */
        if ( ((CurrentVideoParams_p->FrameWidth != VideoParams_p->FrameWidth) ||
              (CurrentVideoParams_p->FrameHeight != VideoParams_p->FrameHeight)) &&
             ((VideoParams_p->FrameWidth != 0 ) && (VideoParams_p->FrameHeight != 0)) ) {
            ErrorCode = HALVIN_SetSizeOfTheFrame(HALHandle, VideoParams_p->FrameWidth, VideoParams_p->FrameHeight);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change Size of the Frame!"));
                return(ErrorCode);
            }
            CurrentVideoParams_p->FrameWidth = VideoParams_p->FrameWidth;
            CurrentVideoParams_p->FrameHeight = VideoParams_p->FrameHeight;
        }

        /* Size of blancking Offset -  HorizontalBlankingOffset & VerticalBlankingOffset */
        if ( (CurrentVideoParams_p->HorizontalBlankingOffset != VideoParams_p->HorizontalBlankingOffset) ||
             (CurrentVideoParams_p->VerticalBlankingOffset != VideoParams_p->VerticalBlankingOffset)) {
            /* No Check! - So no failed! */
            HALVIN_SetBlankingOffset(HALHandle, VideoParams_p->VerticalBlankingOffset, VideoParams_p->HorizontalBlankingOffset);
            CurrentVideoParams_p->HorizontalBlankingOffset = VideoParams_p->HorizontalBlankingOffset;
            CurrentVideoParams_p->VerticalBlankingOffset = VideoParams_p->VerticalBlankingOffset;
        }

        /* Polarity of HSync & VSync - HorizontalSyncActiveEdge & VerticalSyncActiveEdge */
        if ( (CurrentVideoParams_p->HorizontalSyncActiveEdge != VideoParams_p->HorizontalSyncActiveEdge) ||
             (CurrentVideoParams_p->VerticalSyncActiveEdge != VideoParams_p->VerticalSyncActiveEdge)) {
            /* No Check! - So no failed! */
            HALVIN_SetSyncActiveEdge(HALHandle, VideoParams_p->HorizontalSyncActiveEdge, VideoParams_p->VerticalSyncActiveEdge);
            CurrentVideoParams_p->HorizontalSyncActiveEdge = VideoParams_p->HorizontalSyncActiveEdge;
            CurrentVideoParams_p->VerticalSyncActiveEdge = VideoParams_p->VerticalSyncActiveEdge;
        }

        /* Need to be hard fixed! - HSyncOutHorizontalOffset */
        if (CurrentVideoParams_p->HSyncOutHorizontalOffset != VideoParams_p->HSyncOutHorizontalOffset) {
            /* No Check! - So no failed! */
            HALVIN_SetHSyncOutHorizontalOffset(HALHandle, VideoParams_p->HSyncOutHorizontalOffset);
            CurrentVideoParams_p->HSyncOutHorizontalOffset = VideoParams_p->HSyncOutHorizontalOffset;
        }

        /* Need to be hard fixed! - HorizontalVSyncOutLineOffset & VerticalVSyncOutLineOffset */
        if ( (CurrentVideoParams_p->HorizontalVSyncOutLineOffset != VideoParams_p->HorizontalVSyncOutLineOffset) ||
             (CurrentVideoParams_p->VerticalVSyncOutLineOffset != VideoParams_p->VerticalVSyncOutLineOffset)) {
            /* No Check! - So no failed! */
            HALVIN_SetVSyncOutLineOffset(HALHandle, VideoParams_p->HorizontalVSyncOutLineOffset, VideoParams_p->VerticalVSyncOutLineOffset);
            CurrentVideoParams_p->HorizontalVSyncOutLineOffset = VideoParams_p->HorizontalVSyncOutLineOffset;
            CurrentVideoParams_p->VerticalVSyncOutLineOffset = VideoParams_p->VerticalVSyncOutLineOffset;
        }

        /* Perform some specifics initialisations */
        switch (NewInputMode)
        {
            case STVIN_INPUT_MODE_SD:
                if ( (CurrentVideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataType !=
                      VideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataType) ||
                     (CurrentVideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength !=
                      VideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength) )
                {
                    ErrorCode = STVIN_SetAncillaryParameters(
                        Handle,
                        VideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataType,
                        VideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to set Ancillary parameters!"));
                        return(ErrorCode);
                    }
                }

                /* Set The Size Of the Image *Only* if FrameWidth & FrameHeight is Null */
                if ( (VideoParams_p->FrameWidth == 0 ) && (VideoParams_p->FrameHeight == 0) ) {
                    U16 FrameWidth, FrameHeight;

                    FrameWidth = CmdStandardSize[VideoParams_p->ExtraParams.StandardDefinitionParams.StandardType].FrameWidth;
                    FrameHeight = CmdStandardSize[VideoParams_p->ExtraParams.StandardDefinitionParams.StandardType].FrameHeight;

                    ErrorCode = HALVIN_SetSizeOfTheFrame(HALHandle, FrameWidth, FrameHeight);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change Size of the Frame!"));
                        return(ErrorCode);
                    }
                    CurrentVideoParams_p->FrameWidth = FrameWidth;
                    CurrentVideoParams_p->FrameHeight = FrameHeight;
                }

                if ( (CurrentVideoParams_p->HorizontalLowerLimit != VideoParams_p->HorizontalLowerLimit) ||
                     (CurrentVideoParams_p->HorizontalUpperLimit != VideoParams_p->HorizontalUpperLimit) )
                {
                    ErrorCode = HALVIN_SetFieldDetectionMethod(
                        HALHandle,
                        STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES,
                        VideoParams_p->HorizontalLowerLimit,
                        VideoParams_p->HorizontalUpperLimit);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change Field DetectionMethod !"));
                        return(ErrorCode);
                    }
                    CurrentVideoParams_p->HorizontalLowerLimit = VideoParams_p->HorizontalLowerLimit;
                    CurrentVideoParams_p->HorizontalUpperLimit = VideoParams_p->HorizontalUpperLimit;
                }

                /* Indeed, the STi5514 send Y0 Cr0 Y1 Cb2 Y2 Cr2  => STVIN_FIRST_PIXEL_IS_NOT_COMPLETE */
		/* On the other chip, it's correct => STVIN_FIRST_PIXEL_IS_COMPLETE */
                if (CurrentVideoParams_p->ExtraParams.StandardDefinitionParams.FirstPixelSignification !=
                    VideoParams_p->ExtraParams.StandardDefinitionParams.FirstPixelSignification)
                {
                    CurrentVideoParams_p->ExtraParams.StandardDefinitionParams.FirstPixelSignification =
                        VideoParams_p->ExtraParams.StandardDefinitionParams.FirstPixelSignification;
                    /* This command *must* be done at VSync */
                    PushVSyncControllerCommand(Device_p, SYNC_SET_IO_WINDOW);
                }


                break;
            case STVIN_INPUT_MODE_HD:
                if ((((HALVIN_Properties_t *)(Device_p->HALInputHandle))->DeviceType == STVIN_DEVICE_TYPE_ST7100_DVP_INPUT)  ||
                    (((HALVIN_Properties_t *)(Device_p->HALInputHandle))->DeviceType == STVIN_DEVICE_TYPE_ST7109_DVP_INPUT))
                {
                    if ( (CurrentVideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataType !=
                        VideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataType) ||
                        (CurrentVideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength !=
                        VideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength) )
                    {
                        ErrorCode = STVIN_SetAncillaryParameters(
                            Handle,
                            VideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataType,
                            VideoParams_p->ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to set Ancillary parameters!"));
                            return(ErrorCode);
                        }
                    }

                    /* Set The Size Of the Image *Only* if FrameWidth & FrameHeight is Null */
                    if ( (VideoParams_p->FrameWidth == 0 ) && (VideoParams_p->FrameHeight == 0) ) {
                        U16 FrameWidth, FrameHeight;

                        FrameWidth = CmdStandardSize[VideoParams_p->ExtraParams.StandardDefinitionParams.StandardType].FrameWidth;
                        FrameHeight = CmdStandardSize[VideoParams_p->ExtraParams.StandardDefinitionParams.StandardType].FrameHeight;

                        ErrorCode = HALVIN_SetSizeOfTheFrame(HALHandle, FrameWidth, FrameHeight);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change Size of the Frame!"));
                            return(ErrorCode);
                        }
                        CurrentVideoParams_p->FrameWidth = FrameWidth;
                        CurrentVideoParams_p->FrameHeight = FrameHeight;
                    }

                    if ( (CurrentVideoParams_p->HorizontalLowerLimit != VideoParams_p->HorizontalLowerLimit) ||
                        (CurrentVideoParams_p->HorizontalUpperLimit != VideoParams_p->HorizontalUpperLimit) )
                    {
                        ErrorCode = HALVIN_SetFieldDetectionMethod(
                            HALHandle,
                            STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES,
                            VideoParams_p->HorizontalLowerLimit,
                            VideoParams_p->HorizontalUpperLimit);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change Field DetectionMethod !"));
                            return(ErrorCode);
                        }
                        CurrentVideoParams_p->HorizontalLowerLimit = VideoParams_p->HorizontalLowerLimit;
                        CurrentVideoParams_p->HorizontalUpperLimit = VideoParams_p->HorizontalUpperLimit;
                    }

                    /* Indeed, the STi5514 send Y0 Cr0 Y1 Cb2 Y2 Cr2  => STVIN_FIRST_PIXEL_IS_NOT_COMPLETE */
            /* On the other chip, it's correct => STVIN_FIRST_PIXEL_IS_COMPLETE */
                    if (CurrentVideoParams_p->ExtraParams.StandardDefinitionParams.FirstPixelSignification !=
                        VideoParams_p->ExtraParams.StandardDefinitionParams.FirstPixelSignification)
                    {
                        CurrentVideoParams_p->ExtraParams.StandardDefinitionParams.FirstPixelSignification =
                            VideoParams_p->ExtraParams.StandardDefinitionParams.FirstPixelSignification;
                        /* This command *must* be done at VSync */
                        PushVSyncControllerCommand(Device_p, SYNC_SET_IO_WINDOW);
                    }
                }
                else
                {
                    if ( (CurrentVideoParams_p->ExtraParams.HighDefinitionParams.ClockActiveEdge!=
                        VideoParams_p->ExtraParams.HighDefinitionParams.ClockActiveEdge) ||
                        (CurrentVideoParams_p->ExtraParams.HighDefinitionParams.Polarity!=
                        VideoParams_p->ExtraParams.HighDefinitionParams.Polarity) )
                    {
                        ErrorCode = HALVIN_SetClockActiveEdge(
                            HALHandle,
                            VideoParams_p->ExtraParams.HighDefinitionParams.ClockActiveEdge,
                            VideoParams_p->ExtraParams.HighDefinitionParams.Polarity);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change HighDefinition Extra Parameters!"));
                            return(ErrorCode);
                        }
                    }

                    if ( (CurrentVideoParams_p->ExtraParams.HighDefinitionParams.DetectionMethod !=
                        VideoParams_p->ExtraParams.HighDefinitionParams.DetectionMethod) ||
                        (CurrentVideoParams_p->HorizontalLowerLimit != VideoParams_p->HorizontalLowerLimit) ||
                        (CurrentVideoParams_p->HorizontalUpperLimit != VideoParams_p->HorizontalUpperLimit) )
                    {
                        ErrorCode = HALVIN_SetFieldDetectionMethod(
                            HALHandle,
                            VideoParams_p->ExtraParams.HighDefinitionParams.DetectionMethod,
                            VideoParams_p->HorizontalLowerLimit,
                            VideoParams_p->HorizontalUpperLimit);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot Change Field DetectionMethod !"));
                            return(ErrorCode);
                        }
                    }
                }

                break;
            case STVIN_INPUT_INVALID:
            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        memcpy(&(Device_p->VideoParams), VideoParams_p, sizeof(STVIN_VideoParams_t));
        if (ErrorCode == ST_NO_ERROR)
        {
            /* TODO: replace this by HAL call*/
            /* STEVT_Notify(Device_p->EventsHandle, Device_p->RegisteredEventsID[STVIN_CONFIGURATION_CHANGE_EVT_ID], NULL); */
        }
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : stvin_IOWindowsChange
Description :
Parameters  : Video Input Handle, Pointer to
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED,
*******************************************************************************/
void stvin_IOWindowsChange(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData_p,
                 void *SubscriberData_p)
{
    S32                         InputWinX;
    S32                         InputWinY;
    U32                         InputWinWidth;
    U32                         InputWinHeight;
    U32                         OutputWinWidth;
    U32                         OutputWinHeight;
    stvin_Device_t              *Device_p;
    HALVIN_Handle_t             HALHandle;
    ST_ErrorCode_t              ErrorCode;
    const STVID_DigitalInputWindows_t *DigitalInputWindows_p;
    HALVIN_Properties_t *HALVIN_Data_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    Device_p = (stvin_Device_t * )SubscriberData_p;
    HALHandle = (HALVIN_Handle_t)Device_p->HALInputHandle;
    HALVIN_Data_p = (HALVIN_Properties_t *) HALHandle;
    DigitalInputWindows_p   = (const STVID_DigitalInputWindows_t * )EventData_p;

    /* Manual Input Mode */
    if (Device_p->InputWinAutoMode != TRUE)
    {
        InputWinX               = HALVIN_Data_p->InputWindow.InputWinX;
        InputWinY               = HALVIN_Data_p->InputWindow.InputWinY;
        InputWinWidth           = HALVIN_Data_p->InputWindow.InputWinWidth;
        InputWinHeight          = HALVIN_Data_p->InputWindow.InputWinHeight;
    }
    else /* Automatic Input Mode */
    {
        InputWinX               = DigitalInputWindows_p->InputRectangle.PositionX;
        InputWinY               = DigitalInputWindows_p->InputRectangle.PositionY;
        InputWinWidth           = DigitalInputWindows_p->InputRectangle.Width;
        InputWinHeight          = DigitalInputWindows_p->InputRectangle.Height;
    }

    /* Manual Output Mode */
    if (Device_p->OutputWinAutoMode != TRUE)
    {
        OutputWinWidth          = HALVIN_Data_p->InputWindow.OutputWinWidth;
        OutputWinHeight         = HALVIN_Data_p->InputWindow.OutputWinHeight;
    }
    else /* Automatic Output Mode */
    {
         OutputWinWidth          = DigitalInputWindows_p->OutputWidth;
         OutputWinHeight         = DigitalInputWindows_p->OutputHeight;
     }

    /* If automatic mode is True for input or/and output */
    if ((Device_p->InputWinAutoMode == TRUE) || (Device_p->OutputWinAutoMode == TRUE))
    {
        STTBX_Print(("New VIN/DVP Windows : \n"));
        STTBX_Print(("In: x=%i; y=%i; w=%i; h=%i\n",InputWinX,InputWinY,
                                                InputWinWidth,InputWinHeight));
        STTBX_Print(("Out: w=%i; h=%i\n",OutputWinWidth, OutputWinHeight));

        ErrorCode = HALVIN_SetIOWindow(HALHandle, InputWinX, InputWinY,
                                InputWinWidth, InputWinHeight,
                                OutputWinWidth, OutputWinHeight);
        switch (Device_p->DeviceType)
       {
            case STVIN_DEVICE_TYPE_ST5528_DVP_INPUT   :
            case STVIN_DEVICE_TYPE_ST7710_DVP_INPUT   :
            case STVIN_DEVICE_TYPE_ST7100_DVP_INPUT   :
            case STVIN_DEVICE_TYPE_ST7109_DVP_INPUT   :
                switch (Device_p->InputMode) {
                    case STVIN_HD_YCbCr_1280_720_P_CCIR :
                    case STVIN_HD_YCbCr_720_480_P_CCIR :
                    case STVIN_HD_RGB_1024_768_P_EXT :
                    case STVIN_HD_RGB_800_600_P_EXT :
                    case STVIN_HD_RGB_640_480_P_EXT :
                        HALVIN_SetReconstructedFrameSize(HALHandle,
                            2 * OutputWinWidth,
                            Device_p->VideoParams.FrameHeight);
                        break;

                    case STVIN_SD_NTSC_720_480_I_CCIR :
                    case STVIN_SD_NTSC_640_480_I_CCIR :
                    case STVIN_SD_PAL_720_576_I_CCIR :
                    case STVIN_SD_PAL_768_576_I_CCIR :
                    case STVIN_HD_YCbCr_1920_1080_I_CCIR :
                    default :
                        HALVIN_SetReconstructedFrameSize(HALHandle,
                            4 * OutputWinWidth,
                            Device_p->VideoParams.FrameHeight);
                        break;
                }
                break;
            case STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT  :
            case STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT :
            case STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT :
            default :
                HALVIN_SetReconstructedFrameSize(HALHandle,
                                            Device_p->VideoParams.FrameWidth,
                                            Device_p->VideoParams.FrameHeight);
                break;
        }

        /* This command *must* be done at VSync */
        PushVSyncControllerCommand(Device_p, SYNC_SET_IO_WINDOW);
    }

    return;
}

/*
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
*/


/* End of vin_cmd.c */
