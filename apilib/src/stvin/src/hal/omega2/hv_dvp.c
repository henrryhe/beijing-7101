/*******************************************************************************

File name   : hv_dvp.c

Description : Input HAL (Hardware Abstraction Layer) access to hardware source file
              Digital Video Port Input (DVP)

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
06/07/2001         Created                                          JA
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdio.h>
#endif /* ST_OSLINUX */

#include "stsys.h"
#include "sttbx.h"
#include "stddefs.h"
#include "hv_dvp.h"
#include "hv_reg.h"
#include "dvp_flt.h"
#include "stdevice.h"
#include "stos.h"

/* Private Constants -------------------------------------------------------- */
/*********************************/
/* Clock Generator Configuration */
/*********************************/
#if defined (ST_7109)
  #define CKGB_LOCK               0x010
  #define CKGB_CLK_SRC            0x0a8
#endif /*ST_7109*/

/* Enable/Disable */
#define HALVIN_DISABLE                  0
#define HALVIN_ENABLE                   1

/* ERROR */
#define HALVIN_ERROR                    0xFF

/* H & V Decimations */
#define HALVIN_HDECIMATION_MAX_REGVALUE 2
#define HALVIN_VDECIMATION_MAX_REGVALUE 2

/* Interface Type */
#define HALVIN_INTERFACE_MAX_REGVALUE   4
#define HALVIN_EDGE_SYNC_MAX_REGVALUE   4

#ifdef ST_7109
#define SYS_CFG3 0x10C
#endif

static const U8 halvin_RegValueHDecimation[] = {
    STVID_DECIMATION_FACTOR_NONE,
    STVID_DECIMATION_FACTOR_H2,
    STVID_DECIMATION_FACTOR_H4};

static const U8 halvin_RegValueScanType[] = {
    STGXOBJ_INTERLACED_SCAN,
    STGXOBJ_PROGRESSIVE_SCAN};

/* GGi (06Dec2002) : Warning removed.       */
/*static const U8 halvin_RegValueANC[] = {  */
/*    HALVIN_ANCILLARY_DATA_STD_ENCODED,    */
/*    HALVIN_ANCILLARY_DATA_NIBBLE_ENCODED};*/

/*static const U8 halvin_RegValueDC[] = {   */
/*    HALVIN_ANCILLARY_DATA_PACKETS_MODE,   */
/*    HALVIN_ANCILLARY_DATA_CAPTURE_MODE};  */

static const U8 halvin_RegValueSyncPol[] = {
    STVIN_RISING_EDGE,
    STVIN_FALLING_EDGE};

static const U8 halvin_RegValueMDE[] = {
    STVIN_SYNC_TYPE_CCIR,
    STVIN_SYNC_TYPE_EXTERNAL};

/* Private Function prototypes ---------------------------------------------- */

static void DisableAncillaryDataCapture(const HALVIN_Handle_t InputHandle);
static void DisableInterface(const HALVIN_Handle_t InputHandle);
static void EnableAncillaryDataCapture(const HALVIN_Handle_t InputHandle);
static void EnabledInterface(const HALVIN_Handle_t InputHandle);
static U8  GetInterruptStatus(const HALVIN_Handle_t InputHandle);
static U32 GetLineCounter(const HALVIN_Handle_t InputHandle);
static U8  GetStatus(const HALVIN_Handle_t InputHandle);
static ST_ErrorCode_t Init(const HALVIN_Handle_t  InputHandle);
static ST_ErrorCode_t SelectInterfaceMode(const HALVIN_Handle_t InputHandle,
                const HALVIN_InterfaceMode_t InterfaceMode);
static ST_ErrorCode_t SetAncillaryDataCaptureLength(
                const HALVIN_Handle_t InputHandle, const U16 DataCaptureLength);
static void SetAncillaryDataPointer(const HALVIN_Handle_t  InputHandle,
                void * const BufferAddress_p, const U32 DataBufferLength);
static ST_ErrorCode_t SetAncillaryDataType(const HALVIN_Handle_t InputHandle,
                const HALVIN_AncillaryDataType_t DataType);
static ST_ErrorCode_t SetAncillaryEncodedMode(const HALVIN_Handle_t InputHandle,
                const HALVIN_AncillaryEncodedMode_t EncodedMode);
static void SetBlankingOffset(const HALVIN_Handle_t  InputHandle,
                const U16 Vertical, const U16 Horizontal);
static ST_ErrorCode_t SetInputPath(const HALVIN_Handle_t InputHandle,
                const STVIN_InputPath_t InputPath);
static ST_ErrorCode_t SetClockActiveEdge(const HALVIN_Handle_t InputHandle,
                const STVIN_ActiveEdge_t ClockEdge,
                const STVIN_PolarityOfUpperFieldOutput_t Polarity);
static ST_ErrorCode_t SetConversionMode(const HALVIN_Handle_t InputHandle,
                const HALVIN_ConvertMode_t ConvertMode);
static ST_ErrorCode_t SetFieldDetectionMethod(const HALVIN_Handle_t InputHandle,
                const STVIN_FieldDetectionMethod_t DetectionMethod,
                const U16 LowerLimit, const U16 UpperLimit);
static void SetHSyncOutHorizontalOffset(const HALVIN_Handle_t  InputHandle,
                const U16 HorizontalOffset);
static void SetInterruptMask(const HALVIN_Handle_t InputHandle, const U8 Mask);
static void SetReconstructedFrameMaxSize(const HALVIN_Handle_t InputHandle,
                const U16 FrameWidth, const U16 FrameHeight);
static void SetReconstructedFramePointer(const HALVIN_Handle_t  InputHandle,
                const STVID_PictureStructure_t PictureStructure,
                void * const BufferAddress1_p, void * const BufferAddress2_p);
static void SetReconstructedFrameSize(const HALVIN_Handle_t InputHandle,
                const U16 FrameWidth, const U16 FrameHeight);
static ST_ErrorCode_t SetReconstructedStorageMemoryMode(
                const HALVIN_Handle_t InputHandle,
                const STVID_DecimationFactor_t  H_Decimation,
                const STVID_DecimationFactor_t  V_Decimation,
                const STGXOBJ_ScanType_t ScanType);
static ST_ErrorCode_t SetScanType(const HALVIN_Handle_t InputHandle,
                const STGXOBJ_ScanType_t ScanType);
static ST_ErrorCode_t SetSizeOfTheFrame(const HALVIN_Handle_t  InputHandle,
                const U16 FrameWidth, const U16 FrameHeightTop,
                const U16 FrameHeightBottom);
static void SetSyncActiveEdge(const HALVIN_Handle_t InputHandle,
                const STVIN_ActiveEdge_t HorizontalSyncEdge,
                const STVIN_ActiveEdge_t VerticalSyncEdge);
static ST_ErrorCode_t SetSyncType(const HALVIN_Handle_t InputHandle,
                const STVIN_SyncType_t SyncMode);
static void SetVSyncOutLineOffset(const HALVIN_Handle_t  InputHandle,
                const U16 Horizontal, const U16 Vertical);
static void Term(const HALVIN_Handle_t InputHandle);
#if !defined (ST_7109)
static void HAL_SetVerticalResize(void * RegistersBaseAddress_p,U32 InputHeight,
                U32 OutputWinHeight);
#endif /*ST_7109*/
static void HAL_SetHorizontalResize(void * RegistersBaseAddress_p,
                U32 InputWidth,U32 OutputWinWidth);
static U8 RegValueFromEnum(const U8 *Reg_Value_Tab, U8 Enum_Value, U8 Max);
#ifdef DEBUG
static void HALVIN_Error(void);
#endif

/* Global Variables --------------------------------------------------------- */

const HALVIN_FunctionsTable_t HALVIN_DVPOmega2Functions =
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

#define HAL_Write32(Address_p, Value)                                       \
                STSYS_WriteRegDev32LE((void *) (Address_p), (Value))

#define HAL_SetRegister32Value(Address_p, RegMask, Mask, Emp,Value)         \
{                                                                           \
    U32 tmp32;                                                              \
    tmp32 = HAL_Read32 (Address_p);                                         \
    HAL_Write32 (Address_p, (tmp32 & RegMask & (~((U32)Mask << Emp)) )      \
                            | ((U32)Value << Emp));                         \
}

#define HAL_GetRegister16Value(Address_p, Mask, Emp)                        \
             (((U16)(HAL_Read16(Address_p))&((U16)(Mask)<<Emp))>>Emp)

#define HAL_GetRegister8Value(Address_p, Mask, Emp)                         \
            (((U8)(HAL_Read8(Address_p))&((U8)(Mask)<<Emp))>>Emp)


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
    /* Disable interface */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_ANC_DATA_MASK,
        GAM_DVPn_CTRL_ANC_DATA_EMP,
        HALVIN_DISABLE);

} /* End of DisableAncillaryDataCapture() function. */

/*******************************************************************************
Name        :DisableInterface
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableInterface(const HALVIN_Handle_t InputHandle)
{
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_EN_MASK,
        GAM_DVPn_CTRL_EN_EMP,
        HALVIN_DISABLE);

    /* 0: Synchro inputs enabled  */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_RST_MASK,
        GAM_DVPn_CTRL_RST_EMP,
        HALVIN_ENABLE);

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
    /* Enable interface */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_ANC_DATA_MASK,
        GAM_DVPn_CTRL_ANC_DATA_EMP,
        HALVIN_ENABLE);

} /* End of EnableAncillaryDataCapture() function. */

/*******************************************************************************
Name        :EnabledInterface
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnabledInterface(const HALVIN_Handle_t InputHandle)
{

    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_EN_MASK,
        GAM_DVPn_CTRL_EN_EMP,
        HALVIN_ENABLE);

    if (((HALVIN_Properties_t *)InputHandle)->InputMode == STVIN_HD_YCbCr_720_480_P_CCIR)
    {
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_BNL_MASK,
            GAM_DVPn_CTRL_BNL_EMP,
            HALVIN_ENABLE);
    }
    else
    {
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_BNL_MASK,
            GAM_DVPn_CTRL_BNL_EMP,
            HALVIN_DISABLE);
    }

    /* 0: Synchro inputs enabled  */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_RST_MASK,
        GAM_DVPn_CTRL_RST_EMP,
        HALVIN_DISABLE);

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
    U32 InterruptStatus = ((U32)HAL_Read32(((U8 *)((HALVIN_Properties_t *)
                        InputHandle)->RegistersBaseAddress_p) + GAM_DVPn_ITS))
                        & GAM_DVPn_ITS_MASK;
    return (InterruptStatus>>3);
}

/*******************************************************************************
Name        : GetLineCounter
Description : this register holds the value of the RGB Interface's 11-bit line counter
Parameters  : HAL Input manager handle
Assumptions : For "External Sync" only
Limitations :
Returns     :
*******************************************************************************/
static U32 GetLineCounter(const HALVIN_Handle_t InputHandle)
{
    /* Line Counter */
    return (((U32)HAL_Read32(((U8 *)((HALVIN_Properties_t *)
                            InputHandle)->RegistersBaseAddress_p)
                            + GAM_DVPn_LNSTA)) & GAM_DVPn_LNSTA_MASK);
}

/*******************************************************************************
Name        : GetStatus
Description : Get input status information
Parameters  : HAL Input manager handle
Assumptions :
Limitations :
Returns     : Decoder status
*******************************************************************************/
static U8 GetStatus(const HALVIN_Handle_t  InputHandle)
{
    return (((U8)HAL_Read8(((U8 *)((HALVIN_Properties_t *)
                            InputHandle)->RegistersBaseAddress_p)
                            + GAM_DVPn_ITS)) & GAM_DVPn_STA_MASK);
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
    /* allocate memory for private datas if necessary */

    /* No video data are captured, Disabled Ancillary Data,
       Disabled Vertical and Horizontal resize engine*/
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_FULL_MASK,
        GAM_DVPn_CTRL_EN_EMP,
        HALVIN_DISABLE);

    /* Vsync_bot_half_line_en */
    /* 1: vsout starts at the middle of the last top field line*/
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_VSOUT_MASK,
        GAM_DVPn_CTRL_VSOUT_EMP,
        HALVIN_ENABLE);

    /* Oddeven_not_vsync */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_ODDEVEN_EXT_SYNC_MASK,
        GAM_DVPn_CTRL_ODDEVEN_EXT_SYNC_EMP,
        HALVIN_ENABLE);


    /* External vertical and horizontal synchronization signals may be out */
    /* of phase, in this case the vertical synchronization signal must be  */
    /* earlier or later than horizontal synchronization signal with a maximum */
    /* of 1/4 of line length */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_SYNC_PHASE_MASK,
        GAM_DVPn_CTRL_SYNC_PHASE_EMP,
        HALVIN_ENABLE);

    /* Extended-1_254 : 1/254 for both luma & chroma */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_INPUT_CLIPPING_MASK,
        GAM_DVPn_CTRL_INPUT_CLIPPING_EMP,
        HALVIN_ENABLE);

    /* No Vertical synchronisation delay */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p) + GAM_DVPn_VSD,
                    (U32)0 & GAM_DVPn_VSD_MASK);

    /* Horizontal synchronisation delay */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p) + GAM_DVPn_HSD,
                    (U32)0 & GAM_DVPn_HSD_MASK);

    /* 0: Synchro inputs enabled  */
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_RST_MASK,
        GAM_DVPn_CTRL_RST_EMP,
        HALVIN_DISABLE);

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SelectInterfaceMode
Description : stub
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SelectInterfaceMode(const HALVIN_Handle_t InputHandle,
                                  const HALVIN_InterfaceMode_t InterfaceMode)
{
#if defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
    if (InterfaceMode != HALVIN_SD_PIXEL_INTERFACE)
    {
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_HD_EN_MASK,
            GAM_DVPn_CTRL_HD_EN_EMP,
            HALVIN_ENABLE);
    }
    else
    {
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_HD_EN_MASK,
            GAM_DVPn_CTRL_HD_EN_EMP,
            HALVIN_DISABLE);
    }
    return(ST_NO_ERROR);
#else
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
#endif
}

/*******************************************************************************
Name        : SetAncillaryDataCaptureLength
Description : Indicate the number of data bytes following an ancillary
              data packet header.
Parameters  : HAL Input Manager
Assumptions : the DC bit of the SDDP_CTRLx register must be "1"
                        (HALVIN_ANCILLARY_DATA_CAPTURE_MODE})
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetAncillaryDataCaptureLength(
                        const HALVIN_Handle_t InputHandle,
                        const U16 DataCaptureLength)
{
    /* Data Lenght  */
#ifndef WA_WRONG_DATA_WRITE
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p + GAM_DVPn_APS,
                    (U32) DataCaptureLength & GAM_DVPn_APS_MASK);
#else
    /* For 7710 the hard writes the next page on the (last writing adress + DataCaptureLength - random value less than 32)
     * The work arround for that is to use greater DataCaptureLength (DataCaptureLength + 32) for data the page size */
    HAL_Write32( (U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p + GAM_DVPn_APS,
                    (U32) (DataCaptureLength + 32) & GAM_DVPn_APS_MASK);
#endif

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SetAncillaryDataPointer
Description : Set the start adress of reconstructed ancillary data buffer
Parameters  : HAL Input manager handle, buffer address
Assumptions : 7015: Address_p and size must be aligned on 256 bytes. If not,
                it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetAncillaryDataPointer(const HALVIN_Handle_t  InputHandle,
                                    void * const BufferAddress_p, const U32 DataBufferLength)
{

#ifdef DEBUG
    /* ensure that it is 256 bytes aligned */
    if (((U32) BufferAddress_p) & ~GAM_DVPn_ABA_MASK)
    {
        HALVIN_Error();
    }
#endif
    /* Beginning of the reconstructed ancillary data buffer, */
    /* in unit of 256 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p)
                    + GAM_DVPn_ABA, (U32) BufferAddress_p);
    /* End of the reconstructed ancillary data buffer, */
    /* in unit of 256 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p)
                    + GAM_DVPn_AEA, (U32) ((U8 *)BufferAddress_p + DataBufferLength));
}

/*******************************************************************************
Name        : SetAncillaryDataType
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetAncillaryDataType(const HALVIN_Handle_t InputHandle,
                             const HALVIN_AncillaryDataType_t DataType)
{
    if (DataType == HALVIN_ANCILLARY_DATA_CAPTURE_MODE)
    {
        /* Fixed page size */
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_ANC_SIZE_EXT_MASK,
            GAM_DVPn_CTRL_ANC_SIZE_EXT_EMP,
            HALVIN_ENABLE);
    }
    else
    {
        /* Page size extracted from the header of the ancillary packet*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_ANC_SIZE_EXT_MASK,
            GAM_DVPn_CTRL_ANC_SIZE_EXT_EMP,
            HALVIN_DISABLE);
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SetAncillaryEncodedMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetAncillaryEncodedMode(const HALVIN_Handle_t InputHandle,
                                const HALVIN_AncillaryEncodedMode_t EncodedMode)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(EncodedMode);
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SetBlankingOffset
Description : Vertical Offset for Active Video and Time between Horizontal
                Sync and active video
Parameters  : HAL Input manager handle, Vertical, Horizontal
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBlankingOffset(
    const HALVIN_Handle_t  InputHandle,
    const U16 Vertical, const U16 Horizontal)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(Vertical);
    UNUSED_PARAMETER(Horizontal);
}

/*******************************************************************************
Name        :SetInputPath
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetInputPath(const HALVIN_Handle_t InputHandle, const STVIN_InputPath_t InputPath)
{
#ifdef ST_7109
    U32 Value;
#endif

    switch (InputPath) {
        case STVIN_InputPath_PAD :
#ifdef ST_7109
            STOS_InterruptLock();
            /* Enabling PAD path */
            Value = STSYS_ReadRegDev32LE(CFG_BASE_ADDRESS + SYS_CFG3);
            Value = Value & 0xFFFFFBFF;
            STSYS_WriteRegDev32LE(CFG_BASE_ADDRESS + SYS_CFG3,Value);

            /* Disable CLK_DVP_CPT to have CLK_DVP been sourced from VIDINCLKIN */
            /* UNLOCK clock register*/
            STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc0de);
            /*CKGB_CLK_SRC to be updated*/
            Value = STSYS_ReadRegDev32LE( CKG_BASE_ADDRESS + CKGB_CLK_SRC);
            /* Unset bit CLK_DVP_CPT [3] */
            Value &= 0xFFFFFFF7;
            STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_CLK_SRC, Value);
            /* LOCK clock register*/
            STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc1a0);

            STOS_InterruptUnlock();
#endif
            break;

        case STVIN_InputPath_DVO :
            if (((HALVIN_Properties_t *)InputHandle)->InputMode == STVIN_HD_YCbCr_720_480_P_CCIR)
            {
                HAL_SetRegister32Value(
                    (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + GAM_DVPn_CTRL,
                    GAM_DVPn_CTRL_MASK,
                    GAM_DVPn_CTRL_BNL_MASK,
                    GAM_DVPn_CTRL_BNL_EMP,
                    HALVIN_ENABLE);
            }
            else
            {
                HAL_SetRegister32Value(
                    (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p + GAM_DVPn_CTRL,
                    GAM_DVPn_CTRL_MASK,
                    GAM_DVPn_CTRL_BNL_MASK,
                    GAM_DVPn_CTRL_BNL_EMP,
                    HALVIN_DISABLE);
            }
#ifdef ST_7109
            STOS_InterruptLock();
            /* Enabling DVO */
            Value = STSYS_ReadRegDev32LE(CFG_BASE_ADDRESS + SYS_CFG3);
            Value = Value | 0x400;
            STSYS_WriteRegDev32LE(CFG_BASE_ADDRESS + SYS_CFG3,Value);

            /* Set CLK_DVP to be sourced from frequency synthesizer */
            /* UNLOCK clock register*/
            STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc0de);
            /*CKGB_CLK_SRC to be updated*/
            Value = STSYS_ReadRegDev32LE( CKG_BASE_ADDRESS + CKGB_CLK_SRC);
            Value &= 0xFFFFFFF3;
            Value |= 0x8; /*CLK_DVP sourced from Frequency Synthesizer FS0: 1 on [3] CLK_DVP_CPT*/
            STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_CLK_SRC, Value);
            /* LOCK clock register*/
            STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc1a0);

            /* Unprotect access */
            STOS_InterruptUnlock();
#endif
            break;

        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SetClockActiveEdge
Description : stub
Parameters  :
Assumptions :
Limitations :
Returns     :
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
}

/*******************************************************************************
Name        : SetConversionMode
Description : stub
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetConversionMode(const HALVIN_Handle_t InputHandle,
                                        const HALVIN_ConvertMode_t ConvertMode)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(ConvertMode);
    /* Nothing to do in SD mode */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*******************************************************************************
Name        : SetFieldDetectionMethod
Description : Set a window of 'time' that is used to determine field type
Parameters  : HAL Input manager handle, DetectionMethod, Lower Horizontal Limit
                 Upper Horizontal Limit
Assumptions :
Limitations : DetectionMethod is not used in SD mode
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetFieldDetectionMethod(const HALVIN_Handle_t InputHandle,
                            const STVIN_FieldDetectionMethod_t DetectionMethod,
                            const U16 LowerLimit, const U16 UpperLimit)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(DetectionMethod);
    UNUSED_PARAMETER(LowerLimit);
    UNUSED_PARAMETER(UpperLimit);
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
static void SetHSyncOutHorizontalOffset(const HALVIN_Handle_t  InputHandle,
                                        const U16 HorizontalOffset)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(HorizontalOffset);
    /*  Horizontal Offset for HSyncOut */
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
    U32 HalMask = (U32)(Mask<<3);
#ifdef DEBUG
    if (HalMask & (~(U8)GAM_DVPn_ITM_MASK))
    {
        HALVIN_Error();
    }
#endif
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p) + GAM_DVPn_ITM,
                    HalMask);
}

/*******************************************************************************
Name        : SetReconstructedChromaFramePointer
Description : Set the start adress of reconstructed (stored) chroma picture
                buffer
Parameters  : HAL Input manager handle, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes.
                If not, it is done as if they were
              WARNING! This pointer is not for LUMA but the Top field !
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetReconstructedFramePointer(
    const HALVIN_Handle_t  InputHandle,
    const STVID_PictureStructure_t PictureStructure,
    void * const BufferAddress1_p,
    void * const BufferAddress2_p)

{

    if (PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD ||
        PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
    {
#ifdef DEBUG
        /* ensure that it is 512 bytes aligned */
        if (((U32) BufferAddress1_p) & ~GAM_DVPn_VTP_MASK)
        {
            HALVIN_Error();
        }
#endif
        /* Beginning of the reconstructed top picture buffer, */
        /* in unit of 512 bytes */
        HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                        InputHandle)->RegistersBaseAddress_p) + GAM_DVPn_VTP,
                        (U32) BufferAddress1_p);
    }

    if (PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD ||
        PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
    {
#ifdef DEBUG
        /* ensure that it is 512 bytes aligned */
        if (((U32) BufferAddress2_p) & ~GAM_DVPn_VBP_MASK)
        {
            HALVIN_Error();
        }
#endif
        /* Beginning of the reconstructed chroma picture buffer, */
        /* in unit of 512 bytes */
        HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                        InputHandle)->RegistersBaseAddress_p) + GAM_DVPn_VBP,
                        (U32) BufferAddress2_p);
    }
}

/*******************************************************************************
Name        : SetReconstructedFrameSize
Description : Set up the horizontal size of the reconstructed picture
Parameters  : HAL Input manager handle, Frame Width in pixels
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetReconstructedFrameSize(const HALVIN_Handle_t  InputHandle,
                                      const U16 FrameWidth,
                                      const U16 FrameHeight)
{
    UNUSED_PARAMETER(FrameHeight);
    /* Beginning of the picture buffer, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p) + GAM_DVPn_VMP,
                    (U32) FrameWidth);
}

/*******************************************************************************
Name        : SetReconstructedFrameMaxSize
Description : Set up the horizontal size of the reconstructed picture
Parameters  : HAL Input manager handle, Frame Width in pixels
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetReconstructedFrameMaxSize(const HALVIN_Handle_t  InputHandle,
                                         const U16 FrameWidth,
                                         const U16 FrameHeight)
{
    /* */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p) + GAM_DVPn_HLL,
                    (U32) FrameWidth);
    /* */
    HAL_Write32(((U8 *)((HALVIN_Properties_t *)
                    InputHandle)->RegistersBaseAddress_p) + GAM_DVPn_HLFLN,
                    (U32) FrameHeight);
}

/*******************************************************************************
Name        :
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetReconstructedStorageMemoryMode(
                        const HALVIN_Handle_t InputHandle,
                        const STVID_DecimationFactor_t  H_Decimation,
                        const STVID_DecimationFactor_t  V_Decimation,
                        const STGXOBJ_ScanType_t ScanType)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(H_Decimation);
    UNUSED_PARAMETER(V_Decimation);
    UNUSED_PARAMETER(ScanType);

    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : SetScanType
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetScanType(const HALVIN_Handle_t InputHandle,
                                  const STGXOBJ_ScanType_t ScanType)
{
    U32 Scan_Type_To_Reg;

    Scan_Type_To_Reg = (U32)RegValueFromEnum(halvin_RegValueScanType,
            ScanType, HALVIN_EDGE_SYNC_MAX_REGVALUE);

    if (ScanType == STGXOBJ_INTERLACED_SCAN)
    {
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_FORCE_INTER_EN_MASK,
            GAM_DVPn_CTRL_FORCE_INTER_EN_EMP,
            HALVIN_ENABLE);
    }
    else
    {
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_FORCE_INTER_EN_MASK,
            GAM_DVPn_CTRL_FORCE_INTER_EN_EMP,
            HALVIN_DISABLE);
    }

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : SetSizeOfTheFrame
Description : stub
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
    S32 InputWinX, InputWinY;
    U32 Xdo, Xds, Xdo2, Xds2;
    U32 Ydo, Yds, YdoTop, YdsTop, YdoBot, YdsBot;
    U32 InputWinWidth, InputWinHeight;
    STVIN_VideoParams_t *VideoParams_p;
    HALVIN_Properties_t *HALVIN_Properties_p;
    U32 BotSize;

    UNUSED_PARAMETER(FrameHeightBottom);
    HALVIN_Properties_p = (HALVIN_Properties_t *)InputHandle;
    VideoParams_p = HALVIN_Properties_p->VideoParams_p;

    /* to support CCIR-656 or CCIR-601 */
    if (VideoParams_p->SyncType == STVIN_SYNC_TYPE_EXTERNAL)
    {
        /* CCIR 601 */
        /* TODO: find why we must do /2 for horizontal blanking */
        InputWinX = (VideoParams_p->HorizontalBlankingOffset >> 1);
        InputWinY = (VideoParams_p->VerticalBlankingOffset >> 1);
    }
    else
    {
        /* CCIR 656 */
        InputWinX = 0;
        InputWinY = 0;
    }

    InputWinWidth = HALVIN_Properties_p->InputWindow.InputWinWidth;
    InputWinHeight = HALVIN_Properties_p->InputWindow.InputWinHeight;

    if ( (InputWinWidth > FrameWidth) ||
         (InputWinHeight > FrameHeightTop) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* add starting window contraint */
    Xdo = InputWinX;
    Xdo += HALVIN_Properties_p->InputWindow.InputWinX;
    Xds = Xdo + InputWinWidth - 1;

    if (Xdo & 1)   /* First pixel is not complete */
    {
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_PHASE_Y1_MASK,
            GAM_DVPn_CTRL_PHASE_Y1_EMP,
            HALVIN_ENABLE);
    }
    else
    {
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_PHASE_Y1_MASK,
            GAM_DVPn_CTRL_PHASE_Y1_EMP,
            HALVIN_DISABLE);
    }

    /* Horizontal resize */
#if defined (ST_7109)
    if(InputWinWidth !=  HALVIN_Properties_p->InputWindow.OutputWinWidth)
    {
#endif /*ST_7109*/
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_HRESIZE_MASK,
            GAM_DVPn_CTRL_HRESIZE_EMP,
            HALVIN_ENABLE);
        HAL_SetHorizontalResize(((HALVIN_Properties_t *)
                InputHandle)->RegistersBaseAddress_p,
                InputWinWidth,HALVIN_Properties_p->InputWindow.OutputWinWidth);
#if defined (ST_7109)
    }
    else
    {
        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_HRESIZE_MASK,
            GAM_DVPn_CTRL_HRESIZE_EMP,
            HALVIN_DISABLE);
    }
#endif /*ST_7109*/

    /* Vertical resize */
#if !defined (ST_7109)
    HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_VRESIZE_MASK,
            GAM_DVPn_CTRL_VRESIZE_EMP,
            HALVIN_ENABLE);
    HAL_SetVerticalResize(((HALVIN_Properties_t *)
               InputHandle)->RegistersBaseAddress_p,
               InputWinHeight,HALVIN_Properties_p->InputWindow.OutputWinHeight);
#endif /*!ST_7109*/

    /*  0   1  2   3  4  5   6  */
    /* Cb0 Y0 Cr0 Y1 Cb2 Y2 Cr2 */
    /*  |                    |  */
    /*  +- Xdo=0      Xds=6 -+  */

    Xdo2 = (Xdo << 1) + ((Xdo & 1) ? 1 : 0);   /* odd or even */
    Xds2 = ((HALVIN_Properties_p->DefInputMode == STVIN_INPUT_MODE_SD)? ((Xds << 1) + ((Xds & 1) ? 1 : 2)):Xds);
        /* In SD mode XDS= XDO + (pixel width * 2) , where as in HD mode XDS = XDO + pixel width*/
        /* pixel of the viewport (=> Right) */

    if (VideoParams_p->SyncType == STVIN_SYNC_TYPE_EXTERNAL)
    {
        /* CCIR 601 */
        if (VideoParams_p->ExtraParams.StandardDefinitionParams.
                FirstPixelSignification == STVIN_FIRST_PIXEL_IS_NOT_COMPLETE)
        {
            Xdo2 += 3;      /* Indeed, the STi5514 send Y0 Cr0 Y1 Cb2 Y2 Cr2 */
            Xds2 += 3;      /* So we have to add 3, in order to start on a */
                            /* complete pixel, starting with Cb */
        }
    }

    Ydo = InputWinY;
    Ydo += HALVIN_Properties_p->InputWindow.InputWinY;
    Yds = Ydo + InputWinHeight - 1;

    YdoTop = (Ydo >> 1);   /* Alway even */
    YdoBot = (Ydo >> 1);

    switch (HALVIN_Properties_p->InputMode) {
        case STVIN_HD_YCbCr_1280_720_P_CCIR :
        case STVIN_HD_YCbCr_720_480_P_CCIR :
        case STVIN_HD_RGB_1024_768_P_EXT :
        case STVIN_HD_RGB_800_600_P_EXT :
        case STVIN_HD_RGB_640_480_P_EXT :
                YdsTop = Yds;
                YdsBot = Yds - ((InputWinHeight & 1) ? 1 : 0);       /* GNBvd50157 STVIN ""uninitialised"" line */
            break;

        case STVIN_SD_NTSC_720_480_I_CCIR :
        case STVIN_SD_NTSC_640_480_I_CCIR :
        case STVIN_SD_PAL_720_576_I_CCIR :
        case STVIN_SD_PAL_768_576_I_CCIR :
        case STVIN_HD_YCbCr_1920_1080_I_CCIR :
        default :
                YdsTop = (Yds >> 1);
                YdsBot = (Yds >> 1) - ((InputWinHeight & 1) ? 1 : 0);       /* GNBvd50157 STVIN ""uninitialised"" line */
            break;
    }

    /*STTBX_Print(("\nYDO TOP: %d ,YDO BOT: %d, YDS TOP: %d, YDS BOT\n",
                 YdoTop,YdoBot ,YdsTop ,YdsBot));*/

    /* Field Top - Xdo & Ydo */
    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_TFO,
        GAM_DVPn_TFO_MASK,
        GAM_DVPn_TFO_XDO_MASK,
        GAM_DVPn_TFO_XDO_EMP,
        Xdo2);

    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_TFO,
        GAM_DVPn_TFO_MASK,
        GAM_DVPn_TFO_YDO_MASK,
        GAM_DVPn_TFO_YDO_EMP,
        YdoTop);

    /* Field Bottom - Xdo & Ydo */
    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_BFO,
        GAM_DVPn_BFO_MASK,
        GAM_DVPn_BFO_XDO_MASK,
        GAM_DVPn_BFO_XDO_EMP,
        Xdo2);

    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_BFO,
        GAM_DVPn_BFO_MASK,
        GAM_DVPn_BFO_YDO_MASK,
        GAM_DVPn_BFO_YDO_EMP,
        YdoBot);

    /* Field Top - Xds & Yds */
    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_TFS,
        GAM_DVPn_TFS_MASK,
        GAM_DVPn_TFS_XDS_MASK,
        GAM_DVPn_TFS_XDS_EMP,
        Xds2);

    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_TFS,
        GAM_DVPn_TFS_MASK,
        GAM_DVPn_TFS_YDS_MASK,
        GAM_DVPn_TFS_YDS_EMP,
        YdsTop);

    /* Field Bottom - Xdo & Ydo */
    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_BFS,
        GAM_DVPn_BFS_MASK,
        GAM_DVPn_BFS_XDS_MASK,
        GAM_DVPn_BFS_XDS_EMP,
        Xds2);

    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_BFS,
        GAM_DVPn_BFS_MASK,
        GAM_DVPn_BFS_YDS_MASK,
        GAM_DVPn_BFS_YDS_EMP,
        YdsBot);


    /* Captured video window size */
    /*----------------------------*/

    /* Pixmap width */
    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_CVS,
        GAM_DVPn_CVS_MASK,
        GAM_DVPn_CVS_WIDTH_MASK,
        GAM_DVPn_CVS_WIDTH_EMP,
        HALVIN_Properties_p->InputWindow.OutputWinWidth);
    /* top Video window height */
    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_CVS,
        GAM_DVPn_CVS_MASK,
        GAM_DVPn_CVS_HEIGHT_MASK,
        GAM_DVPn_CVS_HEIGHT_EMP,
        HALVIN_Properties_p->InputWindow.OutputWinHeight / 2);

    /*STTBX_Print(("\n CVS:  Y: %d, YBOT",
                 HALVIN_Properties_p->InputWindow.OutputWinWidth,
                 HALVIN_Properties_p->InputWindow.OutputWinHeight /2 ));*/

    /* bot_size relation */
    /* 00 : bottom size equals top size
       01 : bottom size equals top size plus 1
       10 : bottom size equals top size minus 1 */
    BotSize = (HALVIN_Properties_p->InputWindow.OutputWinHeight % 2 ==0) ?
                0x00 : 0x10;
    HAL_SetRegister32Value(
        (U8 *)(HALVIN_Properties_p)->RegistersBaseAddress_p + GAM_DVPn_CVS,
        GAM_DVPn_CVS_MASK,
        GAM_DVPn_CVS_TOPBOT_REL_MASK,
        GAM_DVPn_CVS_TOPBOT_REL_EMP,
        BotSize);
    /*STTBX_Print(("\n CVS:  Y: %d, YBOT\n",
                 HALVIN_Properties_p->InputWindow.OutputWinHeight /2,
                 BotSize ));*/

    return(ST_NO_ERROR);

}

/*******************************************************************************
Name        : SetSyncActiveEdge
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetSyncActiveEdge(const HALVIN_Handle_t InputHandle,
                              const STVIN_ActiveEdge_t HorizontalSyncEdge,
                              const STVIN_ActiveEdge_t VerticalSyncEdge)
{

    U32 Both_Sync_To_Reg;

    if (HorizontalSyncEdge == VerticalSyncEdge)
    {
        Both_Sync_To_Reg = (U32)RegValueFromEnum(halvin_RegValueSyncPol,
                HorizontalSyncEdge, HALVIN_EDGE_SYNC_MAX_REGVALUE);

        HAL_SetRegister32Value(
            (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                        + GAM_DVPn_CTRL,
            GAM_DVPn_CTRL_MASK,
            GAM_DVPn_CTRL_EXT_SYNC_POL_MASK,
            GAM_DVPn_CTRL_EXT_SYNC_POL_EMP,
            Both_Sync_To_Reg);
    }
    else
    {
        /* return (ST_ERROR_FEATURE_NOT_SUPPORTED); */
    }
}

/*******************************************************************************
Name        : SetSyncMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetSyncType(const HALVIN_Handle_t InputHandle,
                                  const STVIN_SyncType_t SyncType)
{
    U32 Sync_Mode_To_Reg;

    Sync_Mode_To_Reg = (U32)RegValueFromEnum(halvin_RegValueMDE, SyncType,
                                            HALVIN_EDGE_SYNC_MAX_REGVALUE);
    HAL_SetRegister32Value(
        (U8 *)((HALVIN_Properties_t *)InputHandle)->RegistersBaseAddress_p
                                                    + GAM_DVPn_CTRL,
        GAM_DVPn_CTRL_MASK,
        GAM_DVPn_CTRL_MDE_MASK,
        GAM_DVPn_CTRL_MDE_EMP,
        Sync_Mode_To_Reg);

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
static void SetVSyncOutLineOffset(const HALVIN_Handle_t  InputHandle,
                                  const U16 Horizontal,
                                  const U16 Vertical)
{
    UNUSED_PARAMETER(InputHandle);
    UNUSED_PARAMETER(Horizontal);
    UNUSED_PARAMETER(Vertical);
    /* Vertical Line Offset for VSyncOut */
    /* Horizontal Line Offset for VSyncOut */
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
static U8 RegValueFromEnum (const U8 *RegValueTab, U8 EnumValue, U8 RegValueMax)
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
Name        : HAL_SetHorizontalResize
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void HAL_SetHorizontalResize(void * RegistersBaseAddress_p,
                                    U32 InputWidth, U32 OutputWinWidth)
{
    U32     Increment;
    U32     FirstCoeff;
    U32     Val,Add,i;
    U32     Ratio;

    Increment   = (InputWidth - 1) * 256
                    / (OutputWinWidth - 1);
    Ratio = OutputWinWidth * 1000 / InputWidth;


    HAL_SetRegister32Value((U32)RegistersBaseAddress_p + GAM_DVPn_HSRC,
            GAM_DVPn_HSRC_MASK,
            GAM_DVPn_HSRC_INC_MASK,
            GAM_DVPn_HSRC_INC_EMP,
            Increment);

    HAL_SetRegister32Value((U32)RegistersBaseAddress_p + GAM_DVPn_HSRC,
            GAM_DVPn_HSRC_MASK,
            GAM_DVPn_HSRC_IP_MASK,
            GAM_DVPn_HSRC_IP_EMP,
            0);

    /* Filter selection */
    if(Ratio > 850)
    {
        FirstCoeff = 0 * NB_HSRC_COEFFS;
    }
    else if (Ratio > 600)
    {
        FirstCoeff = 1 * NB_HSRC_COEFFS;
    }
    else if (Ratio > 375)
    {
        FirstCoeff = 2 * NB_HSRC_COEFFS;
    }
    else if (Ratio > 180 )
    {
        FirstCoeff = 3 * NB_HSRC_COEFFS;
    }
    else
    {
        FirstCoeff = 4 * NB_HSRC_COEFFS;
    }

    /* Set filter coeffs */
    for( i=0; i<(NB_HSRC_COEFFS/4); i++)
    {
        Add = (U32)RegistersBaseAddress_p + GAM_DVPn_HFC + (i*4);
        Val = ((U8)(stvin_HSRC_Coeffs[FirstCoeff + (i*4) + 0]) <<  0)
            | ((U8)(stvin_HSRC_Coeffs[FirstCoeff + (i*4) + 1]) <<  8)
            | ((U8)(stvin_HSRC_Coeffs[FirstCoeff + (i*4) + 2]) << 16)
            | ((U8)(stvin_HSRC_Coeffs[FirstCoeff + (i*4) + 3]) << 24);
        HAL_Write32(Add,Val);
    }

    HAL_SetRegister32Value((U32)RegistersBaseAddress_p + GAM_DVPn_HSRC,
            GAM_DVPn_HSRC_MASK,
            GAM_DVPn_HSRC_FILTER_MASK,
            GAM_DVPn_HSRC_FILTER_EMP,
            HALVIN_ENABLE );
}

#if !defined (ST_7109)
/*******************************************************************************
Name        : HAL_SetVerticalResize
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void HAL_SetVerticalResize(void * RegistersBaseAddress_p,U32 InputHeight,
                                                      U32 OutputWinHeight)
{
    U32     Increment;
    U32     Ratio;
    U32     FirstCoeff;
    U32     Val,Add,i;

    Increment   = (InputHeight - 1) * 256
                    /(OutputWinHeight - 1);
    Ratio = OutputWinHeight * 1000 / InputHeight;

    HAL_SetRegister32Value((U32)RegistersBaseAddress_p + GAM_DVPn_VSRC,
            GAM_DVPn_VSRC_MASK,
            GAM_DVPn_VSRC_INC_MASK,
            GAM_DVPn_VSRC_INC_EMP,
            Increment);

    HAL_SetRegister32Value((U32)RegistersBaseAddress_p + GAM_DVPn_VSRC,
            GAM_DVPn_VSRC_MASK,
            GAM_DVPn_VSRC_IPT_MASK,
            GAM_DVPn_VSRC_IPT_EMP,
            0);

    /* Filter selection */
    if(Ratio > 850)
    {
        FirstCoeff = 0 * NB_VSRC_COEFFS;
    }
    else if (Ratio > 600)
    {
        FirstCoeff = 1 * NB_VSRC_COEFFS;
    }
    else if (Ratio > 375)
    {
        FirstCoeff = 2 * NB_VSRC_COEFFS;
    }
    else if (Ratio > 180 )
    {
        FirstCoeff = 3 * NB_VSRC_COEFFS;
    }
    else
    {
        FirstCoeff = 4 * NB_VSRC_COEFFS;
    }

    /* Set filter coeffs */
    for( i=0; i<(NB_VSRC_COEFFS/4); i++)
    {
        Add = (U32)RegistersBaseAddress_p + GAM_DVPn_VFC + (i*4);
        Val = ((U8)(stvin_VSRC_Coeffs[FirstCoeff + (i*4) + 0]) <<  0)
            | ((U8)(stvin_VSRC_Coeffs[FirstCoeff + (i*4) + 1]) <<  8)
            | ((U8)(stvin_VSRC_Coeffs[FirstCoeff + (i*4) + 2]) << 16)
            | ((U8)(stvin_VSRC_Coeffs[FirstCoeff + (i*4) + 3]) << 24);
        HAL_Write32(Add,Val);

    }
    HAL_SetRegister32Value((U32)RegistersBaseAddress_p + GAM_DVPn_VSRC,
            GAM_DVPn_VSRC_MASK,
            GAM_DVPn_VSRC_FILTER_MASK,
            GAM_DVPn_VSRC_FILTER_EMP,
            HALVIN_ENABLE);
}
#endif /*ST_7109*/

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



