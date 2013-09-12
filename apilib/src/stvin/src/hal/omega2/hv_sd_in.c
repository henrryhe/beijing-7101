/*******************************************************************************

File name   : hv_sd_in.c

Description : Input HAL (Hardware Abstraction Layer) access to hardware source file
              Standard Definition Video Input

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
11/10/2000         Created                                          JA
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
#include <stdio.h>
#endif /* ST_OSLINUX */

#include "stsys.h"
#include "sttbx.h"
#include "stddefs.h"

#include "hv_sd_in.h"
#include "hv_reg.h"

/* Private Types ------------------------------------------------------------ */
/* -- STi7015 only Work around ---------------------------------------------- */
#define WA_GNBvd07867

/* -- STi7020 only Work around ---------------------------------------------- */
#define WA_GNBvd13841   /* "With external syncs, top input window has to be   */
                        /* shifted down by 2 lines".                          */

#ifdef STVIN_WA_SAA7114
#define WA_BAD_SAA7114_PROGRAMMATION/* Due to bad programmation of SAA7114chip*/
                                    /* SAV and EAV signals are inverted.      */
#endif

#ifdef WA_BAD_SAA7114_PROGRAMMATION
#define SAV_PIXEL_DURATION      2       /* SAV duration (pixel unit)          */
#define EAV_PIXEL_DURATION      2       /* EAV duration (pixel unit)          */
#define HBLK_PIXEL_DURATION   134       /* HBLK duration (pixel unit)         */

                                        /* Horizontal offset:SEA+HBLK+EAV     */
#define WA_SAA7114_HORIZONTAL_OFFSET (SAV_PIXEL_DURATION + EAV_PIXEL_DURATION +\
                                     HBLK_PIXEL_DURATION - 2)
#define WA_SAA7114_HORIZONTAL_OFFSET_NTSC (HBLK_PIXEL_DURATION + SAV_PIXEL_DURATION)
#define WA_SAA7114_HORIZONTAL_OFFSET_PAL  (HBLK_PIXEL_DURATION + SAV_PIXEL_DURATION+\
                                     EAV_PIXEL_DURATION + 4 )
#endif

/* Private Constants -------------------------------------------------------- */
/* Enable/Disable */
#define HALVIN_DISABLE               0
#define HALVIN_ENABLE                1

/* ERROR */
#define HALVIN_ERROR               0xFF

/* H & V Decimations */
#define        HALVIN_HDECIMATION_MAX_REGVALUE                   2
#define        HALVIN_VDECIMATION_MAX_REGVALUE                   2

/* Interface Type */
#define        HALVIN_INTERFACE_MAX_REGVALUE                      4
#define        HALVIN_EDGE_SYNC_MAX_REGVALUE                      4


/* Private Variables (static)------------------------------------------------ */
static U8 RegValueFromEnum(U8 *Reg_Value_Tab, U8 Enum_Value, U8 Max);

#ifdef DEBUG
static void HALVIN_Error(void);
#endif

static U8 halvin_RegValueHDecimation[] = {
    STVID_DECIMATION_FACTOR_NONE,
    STVID_DECIMATION_FACTOR_H2,
    STVID_DECIMATION_FACTOR_H4};

static U8 halvin_RegValueScanType[] = {
    STGXOBJ_INTERLACED_SCAN,
    STGXOBJ_PROGRESSIVE_SCAN};

static U8 halvin_RegValueANC[] = {
    HALVIN_ANCILLARY_DATA_STD_ENCODED,
    HALVIN_ANCILLARY_DATA_NIBBLE_ENCODED};

static U8 halvin_RegValueDC[] = {
    HALVIN_ANCILLARY_DATA_PACKETS_MODE,
    HALVIN_ANCILLARY_DATA_CAPTURE_MODE};

static U8 halvin_RegValueVED[] = {
    STVIN_RISING_EDGE,
    STVIN_FALLING_EDGE};

static U8 halvin_RegValueHED[] = {
    STVIN_RISING_EDGE,
    STVIN_FALLING_EDGE};

static U8 halvin_RegValueMDE[] = {
    STVIN_SYNC_TYPE_CCIR,
    STVIN_SYNC_TYPE_EXTERNAL};

static void DisableAncillaryDataCapture(const HALVIN_Handle_t InputHandle);
static void DisableInterface(const HALVIN_Handle_t InputHandle);
static void EnableAncillaryDataCapture(const HALVIN_Handle_t InputHandle);
static void EnabledInterface(const HALVIN_Handle_t InputHandle);
static U8  GetInterruptStatus(const HALVIN_Handle_t InputHandle);
static U32 GetLineCounter(const HALVIN_Handle_t InputHandle);
static U8  GetStatus(const HALVIN_Handle_t InputHandle);
static ST_ErrorCode_t Init(const HALVIN_Handle_t  InputHandle);
static ST_ErrorCode_t SelectInterfaceMode(const HALVIN_Handle_t InputHandle, const HALVIN_InterfaceMode_t InterfaceMode);
static ST_ErrorCode_t SetAncillaryDataCaptureLength(const HALVIN_Handle_t InputHandle, const U16 DataCaptureLength);
static void SetAncillaryDataPointer(const HALVIN_Handle_t  InputHandle, void * const BufferAddress_p, const U32 DataBufferLength);
static ST_ErrorCode_t SetAncillaryDataType(const HALVIN_Handle_t InputHandle, const HALVIN_AncillaryDataType_t DataType);
static ST_ErrorCode_t SetAncillaryEncodedMode(const HALVIN_Handle_t InputHandle, const HALVIN_AncillaryEncodedMode_t EncodedMode);
static void SetBlankingOffset(const HALVIN_Handle_t  InputHandle, const U16 Vertical, const U16 Horizontal);
static ST_ErrorCode_t SetInputPath(const HALVIN_Handle_t InputHandle, STVIN_InputPath_t InputPath);
static ST_ErrorCode_t SetClockActiveEdge(const HALVIN_Handle_t InputHandle, const STVIN_ActiveEdge_t ClockEdge, const STVIN_PolarityOfUpperFieldOutput_t Polarity);
static ST_ErrorCode_t SetConversionMode(const HALVIN_Handle_t InputHandle, const HALVIN_ConvertMode_t ConvertMode);
static ST_ErrorCode_t SetFieldDetectionMethod(const HALVIN_Handle_t InputHandle, const STVIN_FieldDetectionMethod_t DetectionMethod, const U16 LowerLimit, const U16 UpperLimit);
static void SetHSyncOutHorizontalOffset(const HALVIN_Handle_t  InputHandle, const U16 HorizontalOffset);
static void SetInterruptMask(const HALVIN_Handle_t InputHandle, const U8 Mask);
static void SetReconstructedFrameMaxSize(const HALVIN_Handle_t InputHandle, const U16 FrameWidth, const U16 FrameHeight);
static void SetReconstructedFramePointer(const HALVIN_Handle_t  InputHandle, const STVID_PictureStructure_t PictureStructure, void * const BufferAddress1_p, void * const BufferAddress2_p);
static void SetReconstructedFrameSize(const HALVIN_Handle_t InputHandle, const U16 FrameWidth, const U16 FrameHeight);
static ST_ErrorCode_t SetReconstructedStorageMemoryMode(const HALVIN_Handle_t InputHandle,
                                                        const STVID_DecimationFactor_t  H_Decimation,
                                                        const STVID_DecimationFactor_t  V_Decimation,
                                                        const STGXOBJ_ScanType_t ScanType);
static ST_ErrorCode_t SetScanType(const HALVIN_Handle_t InputHandle, const STGXOBJ_ScanType_t ScanType);
static ST_ErrorCode_t SetSizeOfTheFrame(const HALVIN_Handle_t  InputHandle, const U16 FrameWidth, const U16 FrameHeightTop, const U16 FrameHeightBottom);
static void SetSyncActiveEdge(const HALVIN_Handle_t InputHandle, const STVIN_ActiveEdge_t HorizontalSyncEdge, const STVIN_ActiveEdge_t VerticalSyncEdge);
static ST_ErrorCode_t SetSyncType(const HALVIN_Handle_t InputHandle, const STVIN_SyncType_t SyncMode);
static void SetVSyncOutLineOffset(const HALVIN_Handle_t  InputHandle, const U16 Horizontal, const U16 Vertical);
static void Term(const HALVIN_Handle_t InputHandle);

/* Global Variables --------------------------------------------------------- */
const HALVIN_FunctionsTable_t HALVIN_SDOmega2Functions =
{
    DisableAncillaryDataCapture,
    DisableInterface,
    EnableAncillaryDataCapture,
    EnabledInterface,
    GetInterruptStatus,
    GetLineCounter,
    GetStatus,
    Init,
    SelectInterfaceMode,
    SetAncillaryDataCaptureLength,
    SetAncillaryDataPointer,
    SetAncillaryDataType,
    SetAncillaryEncodedMode,
    SetBlankingOffset,
    SetInputPath,
    SetClockActiveEdge,
    SetConversionMode,
    SetFieldDetectionMethod,
    SetHSyncOutHorizontalOffset,
    SetInterruptMask,
    SetReconstructedFrameMaxSize,
    SetReconstructedFramePointer,
    SetReconstructedFrameSize,
    SetReconstructedStorageMemoryMode,
    SetScanType,
    SetSizeOfTheFrame,
    SetSyncActiveEdge,
    SetSyncType,
    SetVSyncOutLineOffset,
    Term
};

/* Private Macros ----------------------------------------------------------- */
#define HAL_Read8(Address_p)     STSYS_ReadRegDev8((void *) (Address_p))
#define HAL_Read16(Address_p)    STSYS_ReadRegDev16LE((void *) (Address_p))
#define HAL_Read32(Address_p)    STSYS_ReadRegDev32LE((void *) (Address_p))

#define HAL_Write32(Address_p, Value) STSYS_WriteRegDev32LE((void *) (Address_p), (Value))


#define HAL_SetRegister32Value(Address_p, RegMask, Mask, Emp,Value)                             \
{                                                                                               \
    U32 tmp32;                                                                                  \
    tmp32 = HAL_Read32 (Address_p);                                                             \
    HAL_Write32 (Address_p, (tmp32 & RegMask & (~((U32)Mask << Emp)) ) | ((U32)Value << Emp));  \
}

#define HAL_GetRegister16Value(Address_p, Mask, Emp)  (((U16)(HAL_Read16(Address_p))&((U16)(Mask)<<Emp))>>Emp)
#define HAL_GetRegister8Value(Address_p, Mask, Emp)   (((U8)(HAL_Read8(Address_p))&((U8)(Mask)<<Emp))>>Emp)

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : DisableAncillaryDataCapture
Description : Disable the SD Ancillary data capture.
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableAncillaryDataCapture(const HALVIN_Handle_t InputHandle)
{
    /* Disable ancillary data interface */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTL,
        SDINn_CTL_MASK,
        SDINn_CTL_ADE_MASK,
        SDINn_CTL_ADE_EMP,
        HALVIN_DISABLE);

} /* End of DisableAncillaryDataCapture() function. */

/*******************************************************************************
Name        : DisableInterface
Description : Disable the SD input interface.
Parameters  : HAL Input manager handle
Assumptions :
Limitations : Reseting bit SDE reset all the logic in the interface, but
              doesn't reset the registers of SD pixel port.
Returns     :
*******************************************************************************/
static void DisableInterface(const HALVIN_Handle_t InputHandle)
{

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTRL,
            SDINn_CTRL_MASK,
            SDINn_CTRL_SDE_MASK,
            SDINn_CTRL_SDE_EMP,
            HALVIN_DISABLE);
    }
    else
    {
        /*- STi7020 case ---------------*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTL,
            SDINn_CTL_MASK,
            SDINn_CTL_SDE_MASK,
            SDINn_CTL_SDE_EMP,
            HALVIN_DISABLE);
    }
} /* End of DisableInterface() function. */

/*******************************************************************************
Name        : EnableAncillaryDataCapture
Description : Enable the SD Ancillary data capture.
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnableAncillaryDataCapture(const HALVIN_Handle_t InputHandle)
{
    /* Enable ancillary data interface */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTL,
        SDINn_CTL_MASK,
        SDINn_CTL_ADE_MASK,
        SDINn_CTL_ADE_EMP,
        HALVIN_ENABLE);

} /* End of EnableAncillaryDataCapture() function. */

/*******************************************************************************
Name        : EnabledInterface
Description : Enable the SD input interface.
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnabledInterface(const HALVIN_Handle_t InputHandle)
{

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTRL,
            SDINn_CTRL_MASK,
            SDINn_CTRL_SDE_MASK,
            SDINn_CTRL_SDE_EMP,
            HALVIN_ENABLE);
    }
    else
    {
        /*- STi7020 case ---------------*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTL,
            SDINn_CTL_MASK,
            SDINn_CTL_SDE_MASK,
            SDINn_CTL_SDE_EMP,
            HALVIN_ENABLE);
    }
} /* End of EnabledInterface() function. */



/*******************************************************************************
Name        : GetInterruptStatus
Description : Get Input Interrupt status
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : Input interrupt status
*******************************************************************************/
static U8 GetInterruptStatus(const HALVIN_Handle_t  InputHandle)
{
    return (((U8)HAL_Read8(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_ITS_8)) & SDINn_ITS_MASK);
} /* End of GetInterruptStatus() function. */

/*******************************************************************************
Name        : GetLineCounter
Description : this register holds the value of the RGB Interface's 11-bit line counter
Parameters  : HAL Input manager handle
Assumptions : For "External Sync" only. No equivalent register on STi7020.
Limitations :
Returns     :
*******************************************************************************/
static U32 GetLineCounter(const HALVIN_Handle_t InputHandle)
{

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        return (((U32)HAL_Read32(((U32 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_LNCNT)) & SDINn_LNCNT_MASK);
    }
    else
    {
        /*- STi7020 case ---------------*/
        return(0);
    }
} /* End of GetLineCounter() function */

/*******************************************************************************
Name        : GetStatus
Description : Get decoder status information
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : Decoder status
*******************************************************************************/
static U8 GetStatus(const HALVIN_Handle_t  InputHandle)
{
    return (((U8)HAL_Read8(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_STA_8)) & SDINn_STA_MASK);
} /* End of GetStatus() function. */

/*******************************************************************************
Name        : Init
Description : Initialisation of the private datas
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t Init(const HALVIN_Handle_t  InputHandle)
{
    U32 ControlRegisterValue = 0;

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT)
    {
        /*- STi7020 case ---------------*/

        /* sync phase not OK enable : External vert. and hor. sync  */
        /* signals may be out of phase.                             */
        ControlRegisterValue |= (HALVIN_ENABLE << SDINn_CTL_SPN_EMP);

        /* Extended-1_254 enable : 1/254 for both luma & chroma     */
        ControlRegisterValue |= (HALVIN_ENABLE << SDINn_CTL_E254_EMP);

#ifdef WA_BAD_SAA7114_PROGRAMMATION
        /* VRef polarity enable : The positive edge is taken as ref.*/
        ControlRegisterValue |= (HALVIN_ENABLE << SDINn_CTL_VRP_EMP);

        /* HRef polarity enable : The positive edge is taken as ref.*/
        ControlRegisterValue |= (HALVIN_ENABLE << SDINn_CTL_HRP_EMP);
#endif

        /* Write control register....                               */
        HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                        InputHandle)->RegistersBaseAddress_p) + SDINn_CTL,
                        ControlRegisterValue & SDINn_CTL_MASK);

        /* No Vertical synchronisation delay */
        HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                        InputHandle)->RegistersBaseAddress_p) + SDINn_VSD,
                        (U32)0 & SDINn_VSD_MASK);

        /* No Horizontal synchronisation delay */
        HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                        InputHandle)->RegistersBaseAddress_p) + SDINn_HSD,
                        (U32)0 & SDINn_HSD_MASK);
    }
    return(ST_NO_ERROR);
} /* End of Init() function. */

/*******************************************************************************
Name        : SelectInterfaceMode
Description : Defines the operational mode of the interface
Parameters  : HAL Input manager handle, Interface Mode
Assumptions : Only available for HALVIN_SD_PIXEL_INTERFACE mode.
Limitations :
Returns     : ST_ERROR_FEATURE_NOT_SUPPORTED if not supported
              ST_NO_ERROR otherwise
*******************************************************************************/
static ST_ErrorCode_t SelectInterfaceMode(const HALVIN_Handle_t InputHandle,
                                          const HALVIN_InterfaceMode_t InterfaceMode)
{
    UNUSED_PARAMETER(InputHandle);
    /* Nothing to do in SD mode / Check Params */
    if (InterfaceMode != HALVIN_SD_PIXEL_INTERFACE)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        return(ST_NO_ERROR);
    }
} /* End of SelectInterfaceMode() function. */

/*******************************************************************************
Name        : SetAncillaryDataCaptureLength
Description : Indicate the number of data bytes following an ancillary
              data packet header.
Parameters  : HAL Input Manager
Assumptions : the DC bit of the SDINn_CTRL (or SDINn_CTL) register
              must be "1" (HALVIN_ANCILLARY_DATA_PACKETS_MODE)
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetAncillaryDataCaptureLength(const HALVIN_Handle_t InputHandle,
                                                    const U16 DataCaptureLength)
{
    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_LENGTH, (U32) DataCaptureLength);
    }
    else
    {
        /* Set SD input ancillary data page size */
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_APS,
                        (U32) DataCaptureLength & SDINn_APS_MASK);
    }

    return(ST_NO_ERROR);
} /* End of SetAncillaryDataCaptureLength() function. */

/*******************************************************************************
Name        : SetAncillaryDataPointer
Description : Set the start adress of reconstructed ancillary data buffer
Parameters  : HAL Input manager handle, buffer address
Assumptions : STi7015/STi7020: Address_p and size must be aligned on 256 bytes.
              If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetAncillaryDataPointer(const HALVIN_Handle_t  InputHandle,
                                    void * const BufferAddress_p,
                                    const U32 DataBufferLength)
{
    UNUSED_PARAMETER(DataBufferLength);
#ifdef DEBUG
    /* ensure that it is 256 bytes aligned */
    if (((U32) BufferAddress_p) & ~SDINn_ANC_MASK)
    {
        HALVIN_Error();
    }
#endif
    /* Beginning of the reconstructed ancillary data buffer, in unit of 256 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_ANC_32, (U32) BufferAddress_p);
} /* End of SetAncillaryDataPointer() function. */

/*******************************************************************************
Name        : SetAncillaryDataType
Description : Set the data capture mode
Parameters  : HAL Input manager handle, Data type
Assumptions : If data type is "HALVIN_ANCILLARY_DATA_PACKETS_MODE", be sure
              packet size have been set (register SDINn_APS_MASK (STi7020) or
              SDINn_LENGTH (STi7015))
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetAncillaryDataType(const HALVIN_Handle_t InputHandle,
                                           const HALVIN_AncillaryDataType_t DataType)
{

    U32 Data_Type_To_Reg;

    Data_Type_To_Reg = (U32)RegValueFromEnum(halvin_RegValueDC, DataType, HALVIN_EDGE_SYNC_MAX_REGVALUE);

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTRL,
            SDINn_CTRL_MASK,
            SDINn_CTRL_DC_MASK,
            SDINn_CTRL_DC_EMP,
            Data_Type_To_Reg);
    }
    else
    {
        /*- STi7020 case ---------------*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTL,
            SDINn_CTL_MASK,
            SDINn_CTL_DC_MASK,
            SDINn_CTL_DC_EMP,
            Data_Type_To_Reg);
    }
    return(ST_NO_ERROR);
} /* End of SetAncillaryDataType() function. */


/*******************************************************************************
Name        : SetAncillaryEncodedMode
Description : Set the ancillary encoded mode.
Parameters  : HAL Input manager handle, Encoded mode
Assumptions :
Limitations : Nothing to do for STi7020
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetAncillaryEncodedMode(const HALVIN_Handle_t InputHandle,
                                              const HALVIN_AncillaryEncodedMode_t EncodedMode)
{

    U32 Encoded_Mode_To_Reg;

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        Encoded_Mode_To_Reg = (U32)RegValueFromEnum(halvin_RegValueANC, EncodedMode, HALVIN_EDGE_SYNC_MAX_REGVALUE);

        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTRL,
            SDINn_CTRL_MASK,
            SDINn_CTRL_ANC_MASK,
            SDINn_CTRL_ANC_EMP,
            Encoded_Mode_To_Reg);
    }

    return(ST_NO_ERROR);
} /* End of SetAncillaryEncodedMode() function. */

/*******************************************************************************
Name        : SetBlankingOffset
Description : Vertical Offset for Active Video and Time between Horizontal Sync and active video
Parameters  : HAL Input manager handle, Vertical, Horizontal
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBlankingOffset(
    const HALVIN_Handle_t  InputHandle,
    const U16 Vertical, const U16 Horizontal)
{

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/

        /*- Vertical -------------------*/
        /* 7015 Hard limitation... */
        if (Vertical < 14)
        {
            HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_VBLANK, (U32) 0);
        }
        else
        {
            HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_VBLANK, (U32) (Vertical-14));
        }

        /*- Horizontal -----------------*/
#ifdef WA_GNBvd07867
        /* Digital input SD 2 in external Sync */
        /* Work around: have value equal to 2+n*4 (multiple of 4 plus 2 with n > 1) */
        if (((HALVIN_Properties_t *)InputHandle)->InputNumber == 1)
        {
            U16 NewHorizontal;

            if ((Horizontal*2) >= 6)        /* 2+n*4 with n=1 */
            {
                NewHorizontal = ((((Horizontal*2)-2)/4)*4)+2;
            }
            else
            {
                NewHorizontal = 6;
            }
            HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_HBLANK, (U32) (NewHorizontal));
        }
        else
        {
            HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_HBLANK, (U32) (Horizontal*2));
        }
#else
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_HBLANK, (U32) (Horizontal*2));
#endif
    } /* STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT */

} /* End of SetBlankingOffset() function. */

/*******************************************************************************
Name        :SetCaptureMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetInputPath(const HALVIN_Handle_t InputHandle, STVIN_InputPath_t InputPath)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(InputPath);
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SetClockActiveEdge
Description : Determines the active edge of clock that is used to register data
              at the input
Parameters  : HAL Input manager handle, Clock edge, polarity
Assumptions :
Limitations :
Returns     : ST_ERROR_FEATURE_NOT_SUPPORTED, because no supported in SD mode.
*******************************************************************************/
static ST_ErrorCode_t SetClockActiveEdge(const HALVIN_Handle_t InputHandle,
                                         const STVIN_ActiveEdge_t ClockEdge,
                                         const STVIN_PolarityOfUpperFieldOutput_t Polarity)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(ClockEdge);
    UNUSED_PARAMETER(Polarity);
    /* Nothing to do in SD mode */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
} /* End of SetClockActiveEdge() function. */

/*******************************************************************************
Name        : SetConversionMode
Description : Determines the convertion mode.
Parameters  : HAL Input manager handle, Convertion mode
Assumptions :
Limitations :
Returns     : ST_ERROR_FEATURE_NOT_SUPPORTED, because no supported in SD mode.
*******************************************************************************/
static ST_ErrorCode_t SetConversionMode(const HALVIN_Handle_t InputHandle,
                                        const HALVIN_ConvertMode_t ConvertMode)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(ConvertMode);
    /* Nothing to do in SD mode */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
} /* End of SetConversionMode() function. */

/*******************************************************************************
Name        : SetFieldDetectionMethod
Description : Set a window of 'time' that is used to determine field type
Parameters  : HAL Input manager handle, DetectionMethod, Lower Horizontal Limit,
              Upper Horizontal Limit
Assumptions :
Limitations : DetectionMethod is not used in SD mode
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetFieldDetectionMethod(const HALVIN_Handle_t InputHandle,
                                              const STVIN_FieldDetectionMethod_t DetectionMethod,
                                              const U16 LowerLimit, const U16 UpperLimit)
{
    UNUSED_PARAMETER(DetectionMethod);
    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        /* Lower Horizontal Limit */
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_LL, (U32) LowerLimit);

        /* Upper Horizontal Limit */
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_UL, (U32) UpperLimit);
    }
    return(ST_NO_ERROR);
} /* End of SetFieldDetectionMethod() function. */


/*******************************************************************************
Name        : SetHSyncOutHorizontalOffset
Description : Horizontal Offset for HSyncOut
Parameters  : HAL Input manager handle, Horizontal Offset
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetHSyncOutHorizontalOffset(
    const HALVIN_Handle_t  InputHandle,
    const U16 HorizontalOffset)
{

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/

        /*  Horizontal Offset for HSyncOut */
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_HOFF, (U32) HorizontalOffset);
    }
} /* End of SetHSyncOutHorizontalOffset() function. */


/*******************************************************************************
Name        : SetInterruptMask
Description : Set Interrupt Mask
Parameters  : HAL Input manager handle, Mask to be written
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetInterruptMask(const HALVIN_Handle_t  InputHandle,
                             const U8               Mask)
{
#ifdef DEBUG
    if (Mask & (~(U8)SDINn_ITM_MASK))
    {
        HALVIN_Error();
    }
#endif
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_ITM,  (U32)Mask & SDINn_ITM_MASK);
} /* End of SetInterruptMask() function. */

/*******************************************************************************
Name        : SetReconstructedFramePointer
Description : Set the start adress of reconstructed (stored) chroma picture buffer
Parameters  : HAL Input manager handle, buffer address
Assumptions : STi7015/STi7020: Address_p and size must be aligned on 512 bytes.
              If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetReconstructedFramePointer(
    const HALVIN_Handle_t  InputHandle,
    const STVID_PictureStructure_t PictureStructure,
    void * const BufferAddress1_p,
    void * const BufferAddress2_p)
{

    UNUSED_PARAMETER(PictureStructure);
#ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress1_p) & ~SDINn_RFP_MASK)
    {
        HALVIN_Error();
    }
#endif
    /* Beginning of the reconstructed luma picture buffer, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_RFP_32, (U32) BufferAddress1_p);

#ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress2_p) & ~SDINn_RCHP_MASK)
    {
        HALVIN_Error();
    }
#endif
    /* Beginning of the reconstructed chroma picture buffer, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_RCHP_32, (U32) BufferAddress2_p);
} /* End of SetReconstructedFramePointer() function */

/*******************************************************************************
Name        : SetReconstructedFrameSize
Description : Set up the horizontal size of the reconstructed picture
Parameters  : HAL Input manager handle, Frame Width in unit of 16 pixels
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetReconstructedFrameSize(const HALVIN_Handle_t  InputHandle,
                                      const U16 FrameWidth,
                                      const U16 FrameHeight)
{

    /* Size of the Frame Width in unit of 16 pixels */
    U32 FrameWidth32 = ((U32)FrameWidth + 15)/16;
    UNUSED_PARAMETER(FrameHeight);

    /* Beginning of the reconstructed luma picture buffer, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_DFW, FrameWidth32);

} /* End of SetReconstructedFrameSize() function. */

/*******************************************************************************
Name        : SetReconstructedFrameMaxSize
Description : Set up the horizontal size of the reconstructed picture
Parameters  : HAL Input manager handle, Frame Width in unit of 16 pixels
Assumptions :
Limitations : Only available on STi7020 chip
Returns     : Nothing
*******************************************************************************/
static void SetReconstructedFrameMaxSize(const HALVIN_Handle_t  InputHandle,
                                         const U16 FrameWidth,
                                         const U16 FrameHeight)
{
    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT)
    {
        /*- STi7020 case ---------------*/
        HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_HLL,
                        (U32) FrameWidth);

        HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_HLFLN,
                        (U32) FrameHeight);
    }
} /* End of SetReconstructedFrameMaxSize() function. */

/*******************************************************************************
Name        : SetReconstructedStorageMemoryMode
Description : Set the storage type of the reconstructed picture
Parameters  : HAL Input manager handle, Horizontal decimation, Vertical
              decimation, scan type.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetReconstructedStorageMemoryMode(const HALVIN_Handle_t InputHandle,
                                                        const STVID_DecimationFactor_t  H_Decimation,
                                                        const STVID_DecimationFactor_t  V_Decimation,
                                                        const STGXOBJ_ScanType_t ScanType)
{

    U32 H_Decimation_To_Reg;

    UNUSED_PARAMETER(V_Decimation);
    UNUSED_PARAMETER(ScanType);
    H_Decimation_To_Reg = (U32)RegValueFromEnum(halvin_RegValueHDecimation,
            H_Decimation, HALVIN_HDECIMATION_MAX_REGVALUE);

    /* set decimation input */
    HAL_SetRegister32Value(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + SDINn_DMOD,
                           SDINn_DMOD_MASK,
                           SDINn_DMOD_HDEC_MASK,
                           SDINn_DMOD_HDEC_EMP,
                           H_Decimation_To_Reg);

    return(ST_NO_ERROR);
} /* End of SetReconstructedStorageMemoryMode() function. */

/*******************************************************************************
Name        : SetScanType
Description : Set the source scan type.
Parameters  : HAL Input manager handle, scan type
Assumptions :
Limitations : Only available on STi7015 chip
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetScanType(const HALVIN_Handle_t InputHandle,
                                  const STGXOBJ_ScanType_t ScanType)
{

    U32 Scan_Type_To_Reg;

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        Scan_Type_To_Reg = (U32)RegValueFromEnum(halvin_RegValueScanType, ScanType, HALVIN_EDGE_SYNC_MAX_REGVALUE);

        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTRL,
            SDINn_CTRL_MASK,
            SDINn_CTRL_TVS_MASK,
            SDINn_CTRL_TVS_EMP,
            Scan_Type_To_Reg);
    }
    return(ST_NO_ERROR);
} /* End of SetScanType() function */


/*******************************************************************************
Name        : SetSizeOfTheFrame
Description : Set the size of input frames
Parameters  : HAL Input manager handle, frame width, frame height top, frame
              height bottom
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static ST_ErrorCode_t SetSizeOfTheFrame(
    const HALVIN_Handle_t  InputHandle,
    const U16 FrameWidth,
    const U16 FrameHeightTop,
    const U16 FrameHeightBottom)
{
    S32 InputWinX, InputWinY;
    U32 Xdo = 0, Xds = 0;
    U32 Xdo2= 0, Xds2= 0;
    U32 Ydo = 0, Yds = 0;
    U32 YdoTop = 0, YdsTop = 0, YdoBot = 0, YdsBot = 0;
    U32 InputWinWidth, InputWinHeight;
    STVIN_VideoParams_t *VideoParams_p;
    HALVIN_Properties_t *HALVIN_Properties_p;

    UNUSED_PARAMETER(FrameHeightBottom);
    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT)
    {
        /*- STi7020 case ---------------*/
        HALVIN_Properties_p = (HALVIN_Properties_t *)InputHandle;
        VideoParams_p = HALVIN_Properties_p->VideoParams_p;

        InputWinX = VideoParams_p->HorizontalBlankingOffset;
        InputWinY = VideoParams_p->VerticalBlankingOffset;

        InputWinWidth  = HALVIN_Properties_p->InputWindow.InputWinWidth;
        InputWinHeight = HALVIN_Properties_p->InputWindow.InputWinHeight;

        if ( (HALVIN_Properties_p->InputWindow.InputWinWidth > FrameWidth) ||
            (InputWinHeight > FrameHeightTop) )
        {
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* ---------------------------------  X AXIS management  ---------------------------------- */
        /* Calculate the starting window constraints( IN PIXEL UNIT).                               */
        /* to support CCIR-656 or CCIR-601                                                          */
        if (VideoParams_p->SyncType == STVIN_SYNC_TYPE_EXTERNAL)
        {
            /* CCIR 601 */
            Xdo =  VideoParams_p->HorizontalBlankingOffset;             /* X Blanking offset.       */
        }
#ifdef WA_BAD_SAA7114_PROGRAMMATION
        else
        {
            /* CCIR 656 */
            if ((HALVIN_Properties_p->VideoParams_p->ExtraParams.StandardDefinitionParams.StandardType
                     == STVIN_STANDARD_NTSC) ||
                (HALVIN_Properties_p->VideoParams_p->ExtraParams.StandardDefinitionParams.StandardType
                     == STVIN_STANDARD_NTSC_SQ) )
            {
                Xdo =  WA_SAA7114_HORIZONTAL_OFFSET_NTSC;   /* Insert adding offset for horizontal  */
            }
            else if ((HALVIN_Properties_p->VideoParams_p->ExtraParams.StandardDefinitionParams.StandardType
                     == STVIN_STANDARD_PAL) ||
                (HALVIN_Properties_p->VideoParams_p->ExtraParams.StandardDefinitionParams.StandardType
                     == STVIN_STANDARD_PAL_SQ) )
            {
                Xdo =  WA_SAA7114_HORIZONTAL_OFFSET_PAL;    /* Insert adding offset for horizontal  */
            }
            else
            {
                Xdo =  WA_SAA7114_HORIZONTAL_OFFSET;             /* Insert adding offset for horizontal  */
            }
        }
#endif

        Xdo += HALVIN_Properties_p->InputWindow.InputWinX;              /* X input window offset.   */
        /* Take care Xdo MUST be divisible by 4 in sample unit, so divisible by 2 in pixel unit.    */
        Xdo &= 0xFFFFFFFE;

        /* Calculate the starting window constraints (IN SAMPLE UNIT).                              */
        Xdo2 = (Xdo << 1);

        /* Calculate the ending window constraints (IN PIXEL UNIT).                                 */
        Xds = Xdo + HALVIN_Properties_p->InputWindow.InputWinWidth - 1;     /* X input window width.*/
        /* Calculate the ending window constraints( IN SAMPLE UNIT).                                */
        Xds2 = (Xds << 1);

        /* ---------------------------------  Y AXIS management  ---------------------------------- */
        /* Calculate the starting window constraints (IN PIXEL UNIT).                               */
        /* to support CCIR-656 or CCIR-601                                                          */
        if (VideoParams_p->SyncType == STVIN_SYNC_TYPE_EXTERNAL)
        {
            /* CCIR 601 */
            Ydo = VideoParams_p->VerticalBlankingOffset;                /* Y Blanking offset.       */
#ifdef WA_BAD_SAA7114_PROGRAMMATION
            if (Ydo > SAV_PIXEL_DURATION)
            {
                Ydo -= 2 * SAV_PIXEL_DURATION;  /* Substract SAV duration (in sample unit).         */
            }
#endif
        }
        else
        {
            /* CCIR 656 */
#ifdef WA_BAD_SAA7114_PROGRAMMATION
            /* Start at position 2 (in 2x pixel unit)   */
            Ydo = 2;
#else
            Ydo = (VideoParams_p->VerticalBlankingOffset >> 1);         /* Y Blanking offset.       */
            /* Ydo has not to be set as it's the first active video line is just */
            /* after the falling edge of VSync.                                  */
#endif
        }

        Ydo += HALVIN_Properties_p->InputWindow.InputWinY;              /* Y input window offset.   */
        /* Calculate the ending window constraints( IN PIXEL UNIT).                                 */
        Yds = Ydo + HALVIN_Properties_p->InputWindow.InputWinHeight;

        YdoTop = (Ydo >> 1);
        YdsTop = (Yds >> 1);

#ifdef WA_GNBvd13841
        if (VideoParams_p->SyncType == STVIN_SYNC_TYPE_EXTERNAL)
        {
            YdoBot = (Ydo >> 1) - 2;
            YdsBot = (Yds >> 1) - 2;
        }
        else
        {
            YdoBot = (Ydo >> 1);
            YdsBot = (Yds >> 1);
        }
#else  /* WA_GNBvd13841 */
        YdoBot = (Ydo >> 1);
        YdsBot = (Yds >> 1);
#endif /* WA_GNBvd13841 */

        /* Field Top - Xdo & Ydo */
        HAL_SetRegister32Value(
            (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + SDINn_TFO,
            SDINn_TFO_MASK,
            SDINn_XDO_MASK,
            SDINn_XDO_EMP,
            Xdo2);

        HAL_SetRegister32Value(
            (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + SDINn_TFO,
            SDINn_TFO_MASK,
            SDINn_YDO_MASK,
            SDINn_YDO_EMP,
            YdoTop);

        /* Field Bottom - Xdo & Ydo */
        HAL_SetRegister32Value(
            (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + SDINn_BFO,
            SDINn_BFO_MASK,
            SDINn_XDO_MASK,
            SDINn_XDO_EMP,
            Xdo2);

        HAL_SetRegister32Value(
            (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + SDINn_BFO,
            SDINn_BFO_MASK,
            SDINn_YDO_MASK,
            SDINn_YDO_EMP,
            YdoBot);

        /* Field Top - Xds & Yds */
        HAL_SetRegister32Value(
            (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + SDINn_TFS,
            SDINn_TFS_MASK,
            SDINn_XDS_MASK,
            SDINn_XDS_EMP,
            Xds2);

        HAL_SetRegister32Value(
            (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + SDINn_TFS,
            SDINn_TFS_MASK,
            SDINn_YDS_MASK,
            SDINn_YDS_EMP,
            YdsTop);

        /* Field Bottom - Xdo & Ydo */
        HAL_SetRegister32Value(
            (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + SDINn_BFS,
            SDINn_BFS_MASK,
            SDINn_XDS_MASK,
            SDINn_XDS_EMP,
            Xds2);

        HAL_SetRegister32Value(
            (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + SDINn_BFS,
            SDINn_BFS_MASK,
            SDINn_YDS_MASK,
            SDINn_YDS_EMP,
            YdsBot);
    } /* STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT */

    return(ST_NO_ERROR);
} /* End of SetSizeOfTheFrame() function. */

/*******************************************************************************
Name        : SetSyncActiveEdge
Description : Set the external sync polarity
Parameters  : HAL Input manager handle, Horizontal polarity, Vertical polarity
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetSyncActiveEdge(const HALVIN_Handle_t InputHandle,
                              const STVIN_ActiveEdge_t HorizontalSyncEdge,
                              const STVIN_ActiveEdge_t VerticalSyncEdge)
{

    U32 Vertical_Sync_To_Reg, Horizontal_Sync_To_Reg;

    Vertical_Sync_To_Reg = (U32)RegValueFromEnum(halvin_RegValueVED, VerticalSyncEdge, HALVIN_EDGE_SYNC_MAX_REGVALUE);
    Horizontal_Sync_To_Reg = (U32)RegValueFromEnum(halvin_RegValueHED, HorizontalSyncEdge, HALVIN_EDGE_SYNC_MAX_REGVALUE);

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTRL,
            SDINn_CTRL_MASK,
            SDINn_CTRL_SYNC_MASK,
            SDINn_CTRL_SYNC_EMP,
            (Vertical_Sync_To_Reg)<<1 | Horizontal_Sync_To_Reg);
    }
    else
    {
        /*- STi7020 case ---------------*/
        if (HorizontalSyncEdge == VerticalSyncEdge)
        {
            HAL_SetRegister32Value(
                (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTL,
                SDINn_CTL_MASK,
                SDINn_CTL_ESP_MASK,
                SDINn_CTL_ESP_EMP,
                Vertical_Sync_To_Reg);
        }
    }
} /* End of SetSyncActiveEdge() function. */

/*******************************************************************************
Name        : SetSyncMode
Description : Set the mode of operation of the interface.
Parameters  : HAL Input manager handle, Sync type
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetSyncType(const HALVIN_Handle_t InputHandle,
                                  const STVIN_SyncType_t SyncType)
{

    U32 Sync_Mode_To_Reg;

    Sync_Mode_To_Reg = (U32)RegValueFromEnum(halvin_RegValueMDE, SyncType, HALVIN_EDGE_SYNC_MAX_REGVALUE);

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTRL,
            SDINn_CTRL_MASK,
            SDINn_CTRL_MDE_MASK,
            SDINn_CTRL_MDE_EMP,
            Sync_Mode_To_Reg);
    }
    else
    {
        /*- STi7020 case ---------------*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_CTL,
            SDINn_CTL_MASK,
            SDINn_CTL_MDE_MASK,
            SDINn_CTL_MDE_EMP,
            Sync_Mode_To_Reg);
    }
    return(ST_NO_ERROR);
} /* End of SetSyncType() function. */


/*******************************************************************************
Name        : SetVSyncOutLineOffset
Description : Set the vertical and horizontal line offset for VSync
Parameters  : HAL Input manager handle, Vertical, Horizontal
Assumptions : For "External Sync" only
Limitations : Only available for STi7015
Returns     : Nothing
*******************************************************************************/
static void SetVSyncOutLineOffset(const HALVIN_Handle_t  InputHandle,
                                  const U16 Horizontal,
                                  const U16 Vertical)
{

    /* Test the target chip. */
    if (((HALVIN_Properties_t *)InputHandle)->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT)
    {
        /*- STi7015 case ---------------*/
        /* Vertical Line Offset for VSyncOut */
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_VLOFF, (U32) Vertical);

        /* Horizontal Line Offset for VSyncOut */
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + SDINn_VHOFF, (U32) Horizontal);
    }
} /* End of SetVSyncOutLineOffset() function. */


/*******************************************************************************
Name        : Term
Description : Actions to do on chip when terminating
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void Term(const HALVIN_Handle_t InputHandle)
{
    UNUSED_PARAMETER(InputHandle);
   /* memory deallocation of the private datas if necessary */
}

/*******************************************************************************
Name        : RegValueFromEnum
Description : Finds the value to write in the corresponding register from The enum Value parameter
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U8 RegValueFromEnum (U8 *RegValueTab, U8 EnumValue, U8 RegValueMax)
{
    U8 i=0;
    while (i <= RegValueMax)
    {
        if (RegValueTab[i] == EnumValue)
        {
            return (i);
        }
        i ++ ;
    }
#ifdef DEBUG
    HALVIN_Error();
#endif
    return (HALVIN_ERROR);
}

/*******************************************************************************
Name        : HALVIN_Error
Description : Function called every time an error occurs. A breakpoint here
              should facilitate the debug
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#ifdef DEBUG
static void HALVIN_Error(void)
{
    while (1);
}
#endif



