/*******************************************************************************

File name   : hv_hd_in.c

Description : Input HAL (Hardware Abstraction Layer) access to hardware source file
              High Definition Video Input

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
04/10/2000         Created                                          JA
03/10/2000         Add somes comments                               JA
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdio.h>
#endif /* ST_OSLINUX */

#include "stsys.h"
#include "sttbx.h"
#include "stddefs.h"

#include "hv_hd_in.h"
#include "hv_reg.h"

#define WA_GNBvd13788   /* "Vertical decimation is not functionnal". No vertical decimation will be         */
                        /* supported.                                                                       */
#define WA_GNBvd12749   /* HDin storage in progressive mode is not correct.                                 */
/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
/* Enable/Disable */
#define HALVIN_DISABLE               0
#define HALVIN_ENABLE                1

/* ERROR */
#define HALVIN_ERROR               0xFF

/* H & V Decimations */
#define        HALVIN_HDECIMATION_MAX_REGVALUE                   2
#define        HALVIN_VDECIMATION_MAX_REGVALUE                   2

/* Scan Type */
#define        HALVIN_SCANTYPE_MAX_REGVALUE                      2

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
static U8 halvin_RegValueVDecimation[] = {
    STVID_DECIMATION_FACTOR_NONE,
    STVID_DECIMATION_FACTOR_V2,
    STVID_DECIMATION_FACTOR_V4};
static U8 halvin_RegValueScanType[] = {
    STGXOBJ_INTERLACED_SCAN,
    STGXOBJ_PROGRESSIVE_SCAN};
static U8 halvin_RegValueMDE[] = {
    HALVIN_EMBEDDED_SYNC_MODE,
    HALVIN_RGB_MODE,
    HALVIN_LUMA_CHROMA_MODE,
    HALVIN_SD_PIXEL_INTERFACE};
static U8 halvin_RegValueCNV[] = {
    HALVIN_DISABLE_RGB_TO_YCC_CONVERSION,
    HALVIN_ENABLED_RGB_TO_YCC_CONVERSION};
static U8 halvin_RegValueCAE[] = {
    STVIN_RISING_EDGE,
    STVIN_FALLING_EDGE};
static U8 halvin_RegValueVAE[] = {
    STVIN_RISING_EDGE,
    STVIN_FALLING_EDGE};
static U8 halvin_RegValueHAE[] = {
    STVIN_RISING_EDGE,
    STVIN_FALLING_EDGE};
static U8 halvin_RegValueTBM[] = {
    STVIN_FIELD_DETECTION_METHOD_LINE_COUNTING,
    STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES};

static U8 halvin_RegValueUFP[] = {
    STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH,
    STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_LOW};

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
const HALVIN_FunctionsTable_t HALVIN_HDOmega2Functions =
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

#define HAL_SetRegister32Value(Address_p, RegMask, Mask, Emp,Value)                                         \
{                                                                                                           \
    U32 tmp32;                                                                                              \
    tmp32 = HAL_Read32 (Address_p);                                                                         \
    HAL_Write32 (Address_p, ((tmp32) & (RegMask) & (~((U32)(Mask) << (Emp))) ) | ((U32)(Value) << (Emp)));  \
}


#define HAL_GetRegister16Value(Address_p, Mask, Emp)  (((U16)(HAL_Read16(Address_p))&((U16)(Mask)<<Emp))>>Emp)
#define HAL_GetRegister8Value(Address_p, Mask, Emp)   (((U8)(HAL_Read8(Address_p))&((U8)(Mask)<<Emp))>>Emp)

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : DisableAncillaryDataCapture
Description : Disable the Ancillary data capture.
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableAncillaryDataCapture(const HALVIN_Handle_t InputHandle)
{
    UNUSED_PARAMETER(InputHandle);
} /* End of DisableAncillaryDataCapture() function. */

/*******************************************************************************
Name        : DisableInterface
Description : Disable Interface
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void DisableInterface(const HALVIN_Handle_t InputHandle)
{

    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_CTRL,
        HDIN_CTRL_MASK,
        HDIN_CTRL_EN_MASK,
        HDIN_CTRL_EN_EMP,
        HALVIN_DISABLE);

}

/*******************************************************************************
Name        : EnableAncillaryDataCapture
Description : Enable the Ancillary data capture.
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnableAncillaryDataCapture(const HALVIN_Handle_t InputHandle)
{
    UNUSED_PARAMETER(InputHandle);
} /* End of EnableAncillaryDataCapture() function. */

/*******************************************************************************
Name        : EnabledInterface
Description : Enabled Interface
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void EnabledInterface(const HALVIN_Handle_t InputHandle)
{

    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_CTRL,
        HDIN_CTRL_MASK,
        HDIN_CTRL_EN_MASK,
        HDIN_CTRL_EN_EMP,
        HALVIN_ENABLE);

}

/*******************************************************************************
Name        : GetInterruptStatus
Description : Get Input Interrupt status
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : Input status
*******************************************************************************/
static U8 GetInterruptStatus(const HALVIN_Handle_t  InputHandle)
{

    return (((U8)HAL_Read8(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + HDIN_ITS_8)) & HDIN_ITS_MASK);
}


/*******************************************************************************
Name        : GetLineCounter
Description : this register holds the value of the RGB Interface's 11-bit line counter
Parameters  : HAL Input manager handle
Assumptions : For "External Sync" only
Limitations :
Returns     : U32 LineCounter
*******************************************************************************/
static U32 GetLineCounter(const HALVIN_Handle_t InputHandle)
{

    /* Line Counter */
    return (((U32)HAL_Read32(((U32 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + HDIN_LNCNT)) & HDIN_LNCNT_MASK);

}

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
    return (((U8)HAL_Read8(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + HDIN_STA_8)) & HDIN_STA_MASK);
}

/*******************************************************************************
Name        : Init
Description : Initialisation of the private datas
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALVIN_Handle_t  InputHandle)
{
    UNUSED_PARAMETER(InputHandle);
    /* allocate memory for private datas if necessary */
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SelectInterfaceMode
Description : Defines the operational mode of the interface
Parameters  : HAL Input manager handle, Interface Mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SelectInterfaceMode(const HALVIN_Handle_t InputHandle,
                                          const HALVIN_InterfaceMode_t InterfaceMode)
{
    U16 Interface_Mode_To_Reg;

    Interface_Mode_To_Reg = (U16)RegValueFromEnum(halvin_RegValueMDE, InterfaceMode, HALVIN_INTERFACE_MAX_REGVALUE);

    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_CTRL,
        HDIN_CTRL_MASK,
        HDIN_CTRL_MDE_MASK,
        HDIN_CTRL_MDE_EMP,
        Interface_Mode_To_Reg);

    return(ST_NO_ERROR);

}
/*******************************************************************************
Name        : SetAncillaryDataCaptureLength
Description : Stub
Parameters  : HAL Input Manager
Assumptions :
Limitations :
Returns     : ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetAncillaryDataCaptureLength(const HALVIN_Handle_t InputHandle,
                                                    const U16 DataCaptureLength)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(DataCaptureLength);
    /* Nothing to do in HD mode  */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}


/*******************************************************************************
Name        : SetAncillaryDataPointer
Description : Set the start adress of reconstructed ancillary data buffer
Parameters  : HAL Input manager handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetAncillaryDataPointer(const HALVIN_Handle_t  InputHandle,
                                    void * const BufferAddress_p,
                                    const U32 DataBufferLength)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(BufferAddress_p);
    UNUSED_PARAMETER(DataBufferLength);
#ifdef DEBUG
    HALVIN_Error();
#endif
}

/*******************************************************************************
Name        : SetAncillaryDataType
Description : Stub
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetAncillaryDataType(const HALVIN_Handle_t InputHandle,
                                           const HALVIN_AncillaryDataType_t DataType)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(DataType);
    /* Nothing to do in HD mode  */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}
/*******************************************************************************
Name        : SetAncillaryEncodedMode
Description : Stub
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetAncillaryEncodedMode(const HALVIN_Handle_t InputHandle,
                                              const HALVIN_AncillaryEncodedMode_t EncodedMode)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(EncodedMode);
    /* Nothing to do in HD mode  */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

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

    /* Vertical */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_VBLANK, (U32) Vertical);

    /* Horizontal */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_HBLANK, (U32) Horizontal);

}
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
Description : Determines the active edge of clock that is used to register data at the input
Parameters  : HAL Input manager handle
              Active edge of the clock that is used to register data at the input
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetClockActiveEdge(const HALVIN_Handle_t InputHandle,
                                         const STVIN_ActiveEdge_t ClockEdge,
                                         const STVIN_PolarityOfUpperFieldOutput_t Polarity)
{

    U32 Clock_Edge_To_Reg, Polarity_Output_To_Reg;

    Clock_Edge_To_Reg = (U32)RegValueFromEnum(halvin_RegValueCAE, ClockEdge, HALVIN_EDGE_SYNC_MAX_REGVALUE);
    Polarity_Output_To_Reg = (U32)RegValueFromEnum(halvin_RegValueUFP, Polarity, HALVIN_EDGE_SYNC_MAX_REGVALUE);

    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_CTRL,
        HDIN_CTRL_MASK,
        HDIN_CTRL_CAE_MASK,
        HDIN_CTRL_CAE_EMP,
        Clock_Edge_To_Reg);

    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_CTRL,
        HDIN_CTRL_MASK,
        HDIN_CTRL_UFP_MASK,
        HDIN_CTRL_UFP_EMP,
        Polarity_Output_To_Reg);

    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : SetConversionMode
Description :
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetConversionMode(const HALVIN_Handle_t InputHandle,
                                        const HALVIN_ConvertMode_t ConvertMode)
{
    U32 Convert_Mode_To_Reg;

    Convert_Mode_To_Reg = (U32)RegValueFromEnum(halvin_RegValueCNV, ConvertMode, HALVIN_EDGE_SYNC_MAX_REGVALUE);

    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_CTRL,
        HDIN_CTRL_MASK,
        HDIN_CTRL_CNV_MASK,
        HDIN_CTRL_CNV_EMP,
        Convert_Mode_To_Reg);

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SetFieldDetectionMethod
Description : Select the top/bottom field detection method,
              Set the polarity of the UPPER_FIELD output signal
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetFieldDetectionMethod(const HALVIN_Handle_t InputHandle,
                                              const STVIN_FieldDetectionMethod_t DetectionMethod,
                                              const U16 LowerLimit, const U16 UpperLimit)
{

    U32 Field_Detection_To_Reg;

    Field_Detection_To_Reg = (U32)RegValueFromEnum(halvin_RegValueTBM, DetectionMethod, HALVIN_EDGE_SYNC_MAX_REGVALUE);

    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_CTRL,
        HDIN_CTRL_MASK,
        HDIN_CTRL_TBM_MASK,
        HDIN_CTRL_TBM_EMP,
        Field_Detection_To_Reg);


    /* Lower Horizontal Limit */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_LL, (U32) LowerLimit);

    /* Upper Horizontal Limit */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_UL, (U32) UpperLimit);

    return(ST_NO_ERROR);
}

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

    /*  Horizontal Offset for HSyncOut */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_HOFF, (U32) HorizontalOffset);

}

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
    if (Mask & (~(U8)HDIN_ITM_MASK))
    {
        HALVIN_Error();
    }
#endif
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + HDIN_ITM,  (U32)Mask & HDIN_ITM_MASK);
}

/*******************************************************************************
Name        : SetReconstructedFramePointer
Description : Set the start adress of reconstructed (stored) chroma picture buffer
Parameters  : HAL Input manager handle, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes. If not, it is done as if they were
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
    if (((U32) BufferAddress1_p) & ~HDIN_RFP_MASK)
    {
        HALVIN_Error();
    }
#endif
    /* Beginning of the reconstructed luma picture buffer, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + HDIN_RFP_32, (U32) BufferAddress1_p);

#ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress2_p) & ~HDIN_RCHP_MASK)
    {
        HALVIN_Error();
    }
#endif
    /* Beginning of the reconstructed chroma picture buffer, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + HDIN_RCHP_32, (U32) BufferAddress2_p);



}

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
    U32 FrameWidth32 = (U32)((FrameWidth+15)/16);

    UNUSED_PARAMETER(FrameHeight);
    /* Beginning of the reconstructed luma picture buffer, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + HDIN_DFW, FrameWidth32);
}

/*******************************************************************************
Name        : SetReconstructedFrameMaxSize
Description : Set up the horizontal size of the reconstructed picture
Parameters  : HAL Input manager handle, Frame Width in unit of 16 pixels
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetReconstructedFrameMaxSize(const HALVIN_Handle_t  InputHandle,
                                         const U16 FrameWidth,
                                         const U16 FrameHeight)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(FrameWidth);
    UNUSED_PARAMETER(FrameHeight);
}

/*******************************************************************************
Name        :
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetReconstructedStorageMemoryMode(const HALVIN_Handle_t InputHandle,
                                                        const STVID_DecimationFactor_t  H_Decimation,
                                                        const STVID_DecimationFactor_t  V_Decimation,
                                                        const STGXOBJ_ScanType_t ScanType)
{

    U32 H_Decimation_To_Reg, V_Decimation_To_Reg, ScanType_To_Reg;

    H_Decimation_To_Reg = (U32)RegValueFromEnum(halvin_RegValueHDecimation, H_Decimation, HALVIN_HDECIMATION_MAX_REGVALUE);
#ifdef WA_GNBvd13788
    V_Decimation_To_Reg = (U32)RegValueFromEnum(halvin_RegValueVDecimation, V_Decimation, HALVIN_VDECIMATION_MAX_REGVALUE);
#else
    V_Decimation_To_Reg = (U32)RegValueFromEnum(halvin_RegValueVDecimation, STVID_DECIMATION_FACTOR_NONE,
            HALVIN_VDECIMATION_MAX_REGVALUE);
#endif /* WA_GNBvd13788 */

#ifdef WA_GNBvd12749
    UNUSED_PARAMETER(ScanType);
    ScanType_To_Reg = (U32)RegValueFromEnum(halvin_RegValueScanType, STGXOBJ_INTERLACED_SCAN, HALVIN_SCANTYPE_MAX_REGVALUE);
#else
    ScanType_To_Reg = (U32)RegValueFromEnum(halvin_RegValueScanType, ScanType, HALVIN_SCANTYPE_MAX_REGVALUE);
#endif

    /* set decimation input */
    HAL_SetRegister32Value(((U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p) + HDIN_DMOD,
                           HDIN_DMOD_MASK,
                           HDIN_DMOD_DEC_MASK,
                           HDIN_DMOD_DEC_EMP,
                           (ScanType_To_Reg)<<4 | (H_Decimation_To_Reg)<<2 | V_Decimation_To_Reg);

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SetScanType
Description : Stub
Parameters  : HAL Input manager handle, Scan Input Mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetScanType(const HALVIN_Handle_t InputHandle,
                                  const STGXOBJ_ScanType_t ScanType)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(ScanType);
    /* Nothing to do, this must be done in D1 block */
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Scan type in HD must be done in D1 block"));

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : SetActiveLineLength
Description : Number of samples per input line
Parameters  : HAL Input manager handle, Number of samples
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

    /* FrameWidth - Defines the active line length of the input video  */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_HSIZE, (U32) FrameWidth);

    /* FrameHeight - Top & Bottom */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_TOP_VEND,
            (U32)((U32) FrameHeightTop + ((HALVIN_Properties_t *)InputHandle)->VideoParams_p->VerticalBlankingOffset));
    if (FrameHeightBottom != 0)
    {
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_BOTTOM_VEND,
            (U32)((U32) FrameHeightBottom + ((HALVIN_Properties_t *)InputHandle)->VideoParams_p->VerticalBlankingOffset));
    }
    else
    {
        /* Progressive input case. Set this register to value zero. */
        HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_BOTTOM_VEND, (U32) 0x00);
    }

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SetSyncActiveEdge
Description : Determines the active edge of VSI and HSI
Parameters  : HAL Input manager handle
              Active edge of HSI that is used for horizontal line synchronisation,
              Active edge of VSI that is used to resolve the Pixel Port interface line count and upper field values
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void SetSyncActiveEdge(const HALVIN_Handle_t InputHandle,
                              const STVIN_ActiveEdge_t HorizontalSyncEdge,
                              const STVIN_ActiveEdge_t VerticalSyncEdge)
{

    U32 Vertical_Sync_To_Reg, Horizontal_Sync_To_Reg;

    Vertical_Sync_To_Reg = (U32)RegValueFromEnum(halvin_RegValueVAE, VerticalSyncEdge, HALVIN_EDGE_SYNC_MAX_REGVALUE);
    Horizontal_Sync_To_Reg = (U32)RegValueFromEnum(halvin_RegValueHAE, HorizontalSyncEdge, HALVIN_EDGE_SYNC_MAX_REGVALUE);

    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_CTRL,
        HDIN_CTRL_MASK,
        HDIN_CTRL_SYNC_MASK,
        HDIN_CTRL_SYNC_EMP,
        (Vertical_Sync_To_Reg)<<1 | Horizontal_Sync_To_Reg);

}

/*******************************************************************************
Name        : SetSyncMode
Description : Stub
Parameters  : HAL Input manager handle, Synchro Type
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetSyncType(const HALVIN_Handle_t InputHandle,
                                  const STVIN_SyncType_t SyncType)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(SyncType);
    /* Nothing to do in HD mode / Only Check if possible !  */
/*    return(ST_ERROR_FEATURE_NOT_SUPPORTED); */
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : SetVSyncOutLineOffset
Description :
Parameters  : HAL Input manager handle, Vertical, Horizontal
Assumptions : For "External Sync" only
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetVSyncOutLineOffset( const HALVIN_Handle_t  InputHandle,
                                   const U16 Horizontal,
                                   const U16 Vertical)
{

    U16 Temp16 = Vertical;

    if (Temp16 == 0)
    {
        Temp16 ++;
    }
    /* Vertical Line Offset for VSyncOut */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_VLOFF, (U32) Temp16);

    /* Horizontal Line Offset for VSyncOu */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + HDIN_VHOFF, (U32) Horizontal);

}

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
