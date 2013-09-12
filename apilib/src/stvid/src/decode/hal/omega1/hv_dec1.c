/*******************************************************************************

File name   : hv_dec1.c

Description : HAL video decode omega 1 family source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
06 Jul 2000        Created                                           HG
25 Jan 2001        Changed CPU memory access to device access        HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Define to protect accee to cyclic registers with interrupt_lock()/interrupt_unlock() */
/* Seems to cause trouble on STi5508 ! Linked to link ?*/
#define HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS

/* This problem has been seen first on STi5518: when decoding field and frame pictures
 alternatively for a long time, at some time a PEI occurs after a decode, but not
 followed by a DEI, altough the bit buffer is filling (up to be full). A pipeline
 reset recovers the situation without side effect. */
 /* Problem under investigation: when no DEI is there:
 * either DEI interrupt is missing (same as SCH missing problem
 * either the VLD is locked and DEI was not raised
 * either the VLD is eating all data very quickly and doesn't see start codes.
 TO BE DEFINED */
#define WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES

/* This problem has been seen on STi5508/18 and confirmed by validation:
randomly (around every 10 hours), a SCH interrupt is not raised, although the start
code detector has found a start code. This seems to always occurs following a
start code search launched by a decode launch.
 The fix to that problem is to read the SCH flag in the status register when having
 the PII interrupt. If it is 1 and no SCH occured, set SCH flag to 1 in interrupt status. */
#define WA_PROBLEM_SCH_MISSING_RANDOMLY

/* Problem reported on STi55xx */
#ifdef ST_5514
#define WA_PROBLEM_OVERWRITE_FIELDS
#endif /* ST_5514 */

/* Problem reported on STi5508/18: sometimes reading register VID_STL always returns 0x00 ! */
#define WA_PROBLEM_STL_CANNOT_BE_READ

/* Define to check pipe misalignment. Issue 398 : PARTIAL error recovery mode */
#define CHECK_SCD_PIPE_MISALIGNMENT

/* Define to add debug info and traces */
/*#define TRACE_DECODE*/

#ifdef TRACE_DECODE
#define TRACE_VSYNCS
#define TRACE_UART
#ifdef TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */
#endif /* TRACE_DECODE */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"

#include "vid_com.h"
#include "vid_ctcm.h"

#include "hv_dec1.h"
#include "hvr_dec1.h"

#include "stsys.h"
#include "sttbx.h"
#include "stcommon.h"
#include "stvtg.h"

#include "stevt.h"


/* Private Types ------------------------------------------------------------ */

typedef struct Omega1Properties_s {
#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    BOOL IsDecoderIdle;
    BOOL IsVLDValid;
    BOOL HadPreviousSCAfterSlice;
    U32 PreviousSCAfterSliceSCD25bitValue;
    U32 PreviousVLDOffset;
    U32 LatchedVLDOffset;
    U32 PreviousVLDAddress;
    U32 LatchedVLDAddress;
    U32 ESBufferLevel;
    BOOL IsCorrectingMisalignment;
    BOOL AlreadyMisaligned;
    BOOL ResetJustDone;
    U8 StartCodeFound;
    BOOL HasGotSlice;
#endif /* CHECK_SCD_PIPE_MISALIGNMENT */
#ifdef WA_PROBLEM_OVERWRITE_FIELDS
    U8   PreviousPictureStructure;
#endif /* WA_PROBLEM_OVERWRITE_FIELDS */
#ifdef WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES
    clock_t MaxDEIMissingTime;
    BOOL GotPipelineIdleWithNoDecoderIdle;
#endif /* WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES */
#if defined WA_PROBLEM_SCH_MISSING_RANDOMLY
    BOOL IsStartCodeDetectorIdle;
    BOOL IsStartCodeExpectedAfterLaunchByDecode;
#endif /* WA_PROBLEM_SCH_MISSING_RANDOMLY */
#if defined WA_PROBLEM_STL_CANNOT_BE_READ
    U8  STLValue;
#endif /* WA_PROBLEM_STL_CANNOT_BE_READ */
    BOOL IsDecodingInTrickMode;
    U32 BitBufferSize;
    U32 BitBufferBase;
    BOOL IsSCDValid;
    U32 PreviousSCD25bitValue;
    U32 LatchedSCD25bitValue;
    U32 PreviousSCD32bitValue;
    U32 LatchedSCD32bitValue;
    HALDEC_DecodeSkipConstraint_t DecodeSkipConstraint;
    BOOL FirstDecodeAfterSoftResetDone;
    STEVT_EventID_t EventIDFlushDecoderPatternToBeInjected;
    U32 CDCountValueBeforeLastFlushBegin;
} Omega1Properties_t;

/* Private Constants -------------------------------------------------------- */

/* Max time where PII can occur with no DEI */
#define MAX_DEI_MISING_ON_FIELD_PICTURES_TIME (STVID_MAX_VSYNC_DURATION * 2)

#define CD_FIFO_ALIGNMENT 64

/* PLE 1 dec 2003 : DECODE_FROM_ADDRESS_AREA_TO_RETRIEVE is the VIDTRICK_SHIFT_BEFORE_STARTCODE that */
/* was before in vid_tric.c. if DECODE_FROM_ADDRESS_AREA_TO_RETRIEVE is not retrieved from the address */
/* to launch the decode from, the decode is bad. This is still not understood. */
#define DECODE_FROM_ADDRESS_AREA_TO_RETRIEVE                            264

/* Private Variables (static)------------------------------------------------ */

#ifdef TRACE_DECODE
static BOOL WasLastVSyncTop = FALSE;
#endif /* TRACE_DECODE */

/* Private Macros ----------------------------------------------------------- */

#define HALDEC_Read8(Address)     STSYS_ReadRegDev8((void *) (Address))
#define HALDEC_Read16(Address)    STSYS_ReadRegDev16BE((void *) (Address))
#define HALDEC_Read24(Address)    STSYS_ReadRegDev24BE((void *) (Address))
#define HALDEC_Read32(Address)    STSYS_ReadRegDev32BE((void *) (Address))

#define HALDEC_Write8(Address, Value)  STSYS_WriteRegDev8((void *) (Address), (Value))
#define HALDEC_Write16(Address, Value) STSYS_WriteRegDev16BE((void *) (Address), (Value))
#define HALDEC_Write24(Address, Value) STSYS_WriteRegDev24BE((void *) (Address), (Value))
#define HALDEC_Write32(Address, Value) STSYS_WriteRegDev32BE((void *) (Address), (Value))

/* Private Function prototypes ---------------------------------------------- */

/* Used to generate VSync event (patch omega1 which has no VTG hardware) */
extern void stvtg_SignalVSYNC(STVTG_VSYNC_t Type);

/* Functions in HALDEC_FunctionsTable_t */
static void DecodingSoftReset(const HALDEC_Handle_t HALDecodeHandle, const BOOL IsRealTime);
static void DisableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle);
static void EnableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle, const STVID_DecimationFactor_t HDecimation, const STVID_DecimationFactor_t VDecimation, const STVID_ScanType_t ScanType);
static void DisablePrimaryDecode(const HALDEC_Handle_t DecodeHandle);
static void EnablePrimaryDecode(const HALDEC_Handle_t DecodeHandle);
static void DisableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle, const STVID_HDPIPParams_t * const HDPIPParams_p);
static void EnableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p);
static void FlushDecoder(const HALDEC_Handle_t HALDecodeHandle, BOOL * const DiscontinuityStartCodeInserted_p);
static U16  Get16BitsHeaderData(const HALDEC_Handle_t HALDecodeHandle);
static U16  Get16BitsStartCode(const HALDEC_Handle_t HALDecodeHandle, BOOL * const StartCodeOnMSB_p);
static U32  Get32BitsHeaderData(const HALDEC_Handle_t HALDecodeHandle);
static U32 GetAbnormallyMissingInterruptStatus(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t GetDataInputBufferParams(
                                const HALDEC_Handle_t HALDecodeHandle,
                                void ** const BaseAddress_p,
                                U32 * const Size_p);
static U32  GetBitBufferOutputCounter(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t GetCDFifoAlignmentInBytes(const HALDEC_Handle_t HALDecodeHandle, U32 * const CDFifoAlignment_p);
static U32  GetDecodeBitBufferLevel(const HALDEC_Handle_t HALDecodeHandle);
#ifdef STVID_HARDWARE_ERROR_EVENT
static STVID_HardwareError_t  GetHardwareErrorStatus(const HALDEC_Handle_t HALDecodeHandle);
#endif /* STVID_HARDWARE_ERROR_EVENT */
static U32  GetInterruptStatus(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t GetLinearBitBufferTransferedDataSize(const HALDEC_Handle_t DecodeHandle, U32 * const LinearBitBufferTransferedDataSize_p);
static void GetDecodeSkipConstraint(const HALDEC_Handle_t HALDecodeHandle, HALDEC_DecodeSkipConstraint_t * const DecodeSkipConstraint_p);
static BOOL GetPTS(const HALDEC_Handle_t HALDecodeHandle, U32 * const PTS_p, BOOL * const PTS33_p);
static ST_ErrorCode_t GetSCDAlignmentInBytes(const HALDEC_Handle_t HALDecodeHandle, U32 * const SCDAlignment_p);
static ST_ErrorCode_t GetStartCodeAddress(const HALDEC_Handle_t HALDecodeHandle, void ** const BufferAddress_p);
static U32  GetStatus(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t Init(const HALDEC_Handle_t HALDecodeHandle);
static BOOL IsHeaderDataFIFOEmpty(const HALDEC_Handle_t HALDecodeHandle);
static void LoadIntraQuantMatrix(const HALDEC_Handle_t HALDecodeHandle, const QuantiserMatrix_t * Matrix_p, const QuantiserMatrix_t * ChromaMatrix_p);
static void LoadNonIntraQuantMatrix(const HALDEC_Handle_t HALDecodeHandle, const QuantiserMatrix_t * Matrix_p, const QuantiserMatrix_t * ChromaMatrix_p);
static void PipelineReset(const HALDEC_Handle_t HALDecodeHandle);
static void PrepareDecode(const HALDEC_Handle_t HALDecodeHandle, void * const DecodeAddressFrom_p, const U16 pTemporalReference, BOOL * const WaitForVLD_p);
static void ResetPESParser(const HALDEC_Handle_t HALDecodeHandle);
static void SearchNextStartCode(const HALDEC_Handle_t HALDecodeHandle, const HALDEC_StartCodeSearchMode_t SearchMode, const BOOL FirstSliceDetect, void * const SearchAddress_p);
static void SetBackwardLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
static void SetBackwardChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
static void SetDecodeBitBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p, const U32 BufferSize, const HALDEC_BitBufferType_t BufferType);
static ST_ErrorCode_t SetDataInputInterface(
          const HALDEC_Handle_t HALDecodeHandle,
          ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle,
                                          void ** const Address_p),
          void (*InformReadAddress)(void * const Handle, void * const Address),
          void * const Handle );
static void SetDecodeBitBufferThreshold(const HALDEC_Handle_t HALDecodeHandle, const U32 BitBufferThreshold);
static void SetDecodeConfiguration(const HALDEC_Handle_t HALDecodeHandle, const StreamInfoForDecode_t * const StreamInfo_p);
static void SetDecodeLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
static void SetDecodeChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
static void SetForwardLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
static void SetForwardChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
static void SetFoundStartCode(const HALDEC_Handle_t HALDecodeHandle, const U8 StartCode, BOOL * const FirstPictureStartCodeFound_p);
static void SetInterruptMask(const HALDEC_Handle_t HALDecodeHandle, const U32 Mask);
static void SetMainDecodeCompression(const HALDEC_Handle_t HALDecodeHandle, const STVID_CompressionLevel_t Compression);
static void SetSecondaryDecodeLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
static void SetSecondaryDecodeChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
static void SetSkipConfiguration(const HALDEC_Handle_t HALDecodeHandle, const STVID_PictureStructure_t * const PictureStructure_p);
static ST_ErrorCode_t SetSmoothBackwardConfiguration(const HALDEC_Handle_t HALDecodeHandle, const BOOL SetNotReset);
static void SetStreamID(const HALDEC_Handle_t HALDecodeHandle, const U8 StreamID);
static void SetStreamType(const HALDEC_Handle_t HALDecodeHandle, const STVID_StreamType_t StreamType);
static ST_ErrorCode_t Setup(const HALDEC_Handle_t HALDecodeHandle, const STVID_SetupParams_t * const SetupParams_p);
static void SkipPicture(const HALDEC_Handle_t HALDecodeHandle, const BOOL ExecuteOnlyOnNextVSync, const HALDEC_SkipPictureMode_t SkipMode);
static void StartDecodePicture(const HALDEC_Handle_t HALDecodeHandle, const BOOL ExecuteOnlyOnNextVSync, const BOOL MainDecodeOverWrittingDisplay, const BOOL SecondaryDecodeOverWrittingDisplay);
static void Term(const HALDEC_Handle_t HALDecodeHandle);
#ifdef STVID_DEBUG_GET_STATUS
static void GetDebugStatus(const HALDEC_Handle_t  HALDecodeHandle,
            STVID_Status_t* const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
static void WriteStartCode(const HALDEC_Handle_t  HALDecodeHandle,
            const U32             SCVal,
            const void * const    SCAdd_p);

/* Other functions */
static U32 TranslateToDecodeStatus(const HALDEC_Handle_t HALDecodeHandle, const U32 HWStatus);
static U32 TranslateToHWStatus(const HALDEC_Handle_t HALDecodeHandle, const U32 Status);

/* Global Variables --------------------------------------------------------- */

const HALDEC_FunctionsTable_t HALDEC_Omega1Functions = {
    DecodingSoftReset,
    DisableSecondaryDecode,
    EnableSecondaryDecode,
    DisablePrimaryDecode,
    EnablePrimaryDecode,
    DisableHDPIPDecode,
    EnableHDPIPDecode,
    FlushDecoder,
    Get16BitsHeaderData,
    Get16BitsStartCode,
    Get32BitsHeaderData,
    GetAbnormallyMissingInterruptStatus,
    GetDataInputBufferParams,
    GetDecodeSkipConstraint,
    GetBitBufferOutputCounter,
    GetCDFifoAlignmentInBytes,
    GetDecodeBitBufferLevel,
#ifdef STVID_HARDWARE_ERROR_EVENT
    GetHardwareErrorStatus,
#endif /* STVID_HARDWARE_ERROR_EVENT */
    GetInterruptStatus,
    GetLinearBitBufferTransferedDataSize,
    GetPTS,
    GetSCDAlignmentInBytes,
    GetStartCodeAddress,
    GetStatus,
    Init,
    IsHeaderDataFIFOEmpty,
    LoadIntraQuantMatrix,
    LoadNonIntraQuantMatrix,
    PipelineReset,
    PrepareDecode,
    ResetPESParser,
    SearchNextStartCode,
    SetBackwardChromaFrameBuffer,
    SetBackwardLumaFrameBuffer,
    SetDataInputInterface,
    SetDecodeBitBuffer,
    SetDecodeBitBufferThreshold,
    SetDecodeChromaFrameBuffer,
    SetDecodeConfiguration,
    SetDecodeLumaFrameBuffer,
    SetForwardChromaFrameBuffer,
    SetForwardLumaFrameBuffer,
    SetFoundStartCode,
    SetInterruptMask,
    SetMainDecodeCompression,
    SetSecondaryDecodeChromaFrameBuffer,
    SetSecondaryDecodeLumaFrameBuffer,
    SetSkipConfiguration,
    SetSmoothBackwardConfiguration,
    SetStreamID,
    SetStreamType,
    Setup,
    SkipPicture,
    StartDecodePicture,
    Term,
#ifdef STVID_DEBUG_GET_STATUS
	GetDebugStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    WriteStartCode
};

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : TranslateToDecodeStatus
Description : Translate HW status to HAL decode status (55xx registers to HAL flags)
Parameters  : HW status of 55xx chip
Assumptions :
Limitations :
Returns     : HAL decode status
*******************************************************************************/
static U32 TranslateToDecodeStatus(const HALDEC_Handle_t HALDecodeHandle, const U32 HWStatus)
{
    U32 Status;

    Status = (HWStatus & (VID_ITX_HFE | VID_ITX_SCH));
    if ((HWStatus & VID_ITX_HFF) != 0)
    {
        Status |= HALDEC_HEADER_FIFO_NEARLY_FULL_MASK;
    }
    Status |= ((HWStatus & (VID_ITX_DEI | VID_ITX_PII | VID_ITX_PSD)) >> 4);
    if ((HWStatus & VID_ITX_PDE) != 0)
    {
        Status |= HALDEC_DECODING_UNDERFLOW_MASK;
    }
    /* OTE not present */
    if ((HWStatus & VID_ITX_ERC) != 0)
    {
        Status |= HALDEC_DECODING_SYNTAX_ERROR_MASK;
    }
    if ((HWStatus & VID_ITX_SER) != 0)
    {
        Status |= HALDEC_DECODING_OVERFLOW_MASK;
    }
    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            if ((HWStatus & VID_ITX_TR_OK) != 0)
            {
                Status |= HALDEC_VLD_READY_MASK;
            }
            break;
        default :
            break;
    }
    Status |= ((HWStatus & (VID_ITX_BBE | VID_ITX_BBF)) << 7);
    if ((HWStatus & VID_ITX_ERR) != 0)
    {
        Status |= HALDEC_INCONSISTENCY_ERROR_MASK;
    }
    if ((HWStatus & VID_ITX_BFF) != 0)
    {
        Status |= HALDEC_BITSTREAM_FIFO_FULL_MASK;
    }
    /* SBE not present */

    return(Status);
} /* End of TranslateToDecodeStatus() function */


/*******************************************************************************
Name        : TranslateToHWStatus
Description : Translate HAL decode status to HW status (HAL flags to 55xx registers)
Parameters  : HAL decode status
Assumptions :
Limitations :
Returns     : HW status
*******************************************************************************/
static U32 TranslateToHWStatus(const HALDEC_Handle_t HALDecodeHandle, const U32 Status)
{
    U32 HWStatus;

    /* OTE not present */

    HWStatus = (Status & (HALDEC_HEADER_FIFO_EMPTY_MASK | HALDEC_START_CODE_HIT_MASK)); /* SCH and HFE */
    if ((Status & HALDEC_HEADER_FIFO_NEARLY_FULL_MASK) != 0)
    {
        HWStatus |= VID_ITX_HFF;
    }
    HWStatus |= ((Status & (HALDEC_DECODER_IDLE_MASK | HALDEC_PIPELINE_IDLE_MASK | HALDEC_PIPELINE_START_DECODING_MASK)) << 4); /* DEI, PII and PSD */
    if ((Status & HALDEC_DECODING_SYNTAX_ERROR_MASK) != 0)
    {
        HWStatus |= VID_ITX_ERC;
    }
    if ((Status & HALDEC_DECODING_UNDERFLOW_MASK) != 0)
    {
        HWStatus |= VID_ITX_PDE;
    }
    if ((Status & HALDEC_DECODING_OVERFLOW_MASK) != 0)
    {
        HWStatus |= VID_ITX_SER;
    }
    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            if ((Status & HALDEC_VLD_READY_MASK) != 0)
            {
                HWStatus |= VID_ITX_TR_OK;
            }
            break;
        default :
            break;
    }
    HWStatus |= ((Status & (HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK | HALDEC_BIT_BUFFER_NEARLY_FULL_MASK)) >> 7); /* BBE and BBF */
    if ((Status & HALDEC_INCONSISTENCY_ERROR_MASK) != 0)
    {
        HWStatus |= VID_ITX_ERR;
    }
    if ((Status & HALDEC_BITSTREAM_FIFO_FULL_MASK) != 0)
    {
        HWStatus |= VID_ITX_BFF;
    }
    /* SBE not present */

    return(HWStatus);
} /* End of TranslateToHWStatus() function */


/*******************************************************************************
Name        : GetDecodeSkipConstraint
Description : We ask the HAL which is its possible action.
Parameters  : IN :HAL decode handle,
              OUT : possible Action.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void GetDecodeSkipConstraint(const HALDEC_Handle_t HALDecodeHandle, HALDEC_DecodeSkipConstraint_t * const DecodeSkipConstraint_p)
{
    Omega1Properties_t * Data_p;

    Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    *DecodeSkipConstraint_p = Data_p->DecodeSkipConstraint;
} /* End of GetDecodeSkipConstraint() function */


/*******************************************************************************
Name        : DecodingSoftReset
Description : Reset decoding
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DecodingSoftReset(const HALDEC_Handle_t HALDecodeHandle, const BOOL IsRealTime)
{
    U32 HWStatus;
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* Reset PES parser. Otherwise PTS association in PES data will be corrupted. */
    ResetPESParser(HALDecodeHandle);

    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_SRV, VID_SRV_SET);
    STTBX_WaitMicroseconds(1);
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_SRV, VID_SRV_RESET);

    /* Clear interrupts for events that occured prior to the SoftReset. Do that before first exe, not to loose any IT. */
    HWStatus  = ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS2)) << 16;
    HWStatus |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS0));
    HWStatus |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS1)) <<  8;

#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    /* Initialise variables */
    Data_p->IsDecoderIdle = FALSE;
    Data_p->IsVLDValid = FALSE;
/*    Data_p->PreviousVLDOffset   = 0;*/
/*    Data_p->PreviousVLDAddress  = Data_p->BitBufferBase;*/
    Data_p->IsCorrectingMisalignment = FALSE;
    Data_p->AlreadyMisaligned   = FALSE;
    Data_p->ResetJustDone = TRUE;
    Data_p->HasGotSlice = FALSE;
    Data_p->StartCodeFound = 0xFF;
    Data_p->HadPreviousSCAfterSlice = FALSE;
    Data_p->PreviousSCAfterSliceSCD25bitValue = 0;
#endif /* CHECK_SCD_PIPE_MISALIGNMENT */
#ifdef WA_PROBLEM_OVERWRITE_FIELDS
    Data_p->PreviousPictureStructure = PICTURE_STRUCTURE_FRAME_PICTURE;
#endif /* WA_PROBLEM_OVERWRITE_FIELDS */
    /* Invalidate SCD: 4 values to reset */
    Data_p->IsSCDValid = FALSE;
    Data_p->PreviousSCD25bitValue = 0;
/*    Data_p->PreviousSCD32bitValue  = Data_p->BitBufferBase - 8;*/
    Data_p->PreviousSCD32bitValue  = 0;
    Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_MUST_DECODE;
    Data_p->FirstDecodeAfterSoftResetDone = FALSE;
#ifdef WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES
    /* No DEI missing after PEI */
    Data_p->GotPipelineIdleWithNoDecoderIdle = FALSE;
#endif /* WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES */
#ifdef WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES
    /* No DEI missing after PEI */
    Data_p->GotPipelineIdleWithNoDecoderIdle = FALSE;
#endif /* WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES */
#if defined WA_PROBLEM_SCH_MISSING_RANDOMLY
    Data_p->IsStartCodeExpectedAfterLaunchByDecode = FALSE;
    if ((!(((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->IsDecodingInTrickMode)) ||
#if defined WA_PROBLEM_STL_CANNOT_BE_READ
        ((Data_p->STLValue & VID_STL_INR_SET) == 0))
#else /* WA_PROBLEM_STL_CANNOT_BE_READ */
        ((HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STL) & VID_STL_INR_SET) == 0))
#endif /* not WA_PROBLEM_STL_CANNOT_BE_READ */
    {
        /* Not in trick modes or SC detector launched automatically: SC detector no more idle on launch of decode */
        Data_p->IsStartCodeDetectorIdle = FALSE;
    }
    else
    {
        Data_p->IsStartCodeDetectorIdle = TRUE;
    }
#endif /* WA_PROBLEM_SCH_MISSING_RANDOMLY */
    /* Seek sequence before become idle: do this as last action. */
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TIS, VID_TIS_EXE | VID_TIS_FIS);

} /* End of DecodingSoftReset() function */


/*******************************************************************************
Name        : DisableSecondaryDecode
Description : Disable secondary decode
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DisableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle)
{
    /* Feature not available on STi55xx */
} /* End of DisableSecondaryDecode() function */


/*******************************************************************************
Name        : EnableSecondaryDecode
Description : Enable secondary decode
Parameters  : HAL Decode manager handle
              Vertical decimation,
              Horizontal decimation,
              Scan type of the picture (progressive or interlaced).
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle, const STVID_DecimationFactor_t HDecimation, const STVID_DecimationFactor_t VDecimation, const STVID_ScanType_t          ScanType)
{
    /* Feature not available on STi55xx */
} /* End of EnableSecondaryDecode() function */

/*******************************************************************************
Name        : EnablePrimaryDecode
Description : Enables Primary Decode
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnablePrimaryDecode(const HALDEC_Handle_t       HALDecodeHandle)
{
    /* Feature not available on STi55xx */
} /* End of EnableSecondaryDecode() function */

/*******************************************************************************
Name        : DisableHDPIPDecode
Description : Disables HDPIP decode.
Parameters  : HAL Decode manager handle, HDPIP parameters
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableHDPIPDecode(const HALDEC_Handle_t HALDecodeHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    /* Feature not available on STi55xx */
} /* End of DisableHDPIPDecode() function */

/*******************************************************************************
Name        : EnableHDPIPDecode
Description : Enables HDPIP decode.
Parameters  : HAL Decode manager handle, HDPIP parameters.
              FrameWidthThreshold, FrameHeightThreshold
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    /* Feature not available on STi55xx */

} /* End of EnableHDPIPDecode() function */

/*******************************************************************************
Name        : DisablePrimaryDecode
Description :
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisablePrimaryDecode(const HALDEC_Handle_t  HALDecodeHandle)
{
    /* Feature not available on STi55xx */
} /* End of DisablePrimaryDecode() function */

/*******************************************************************************
Name        : FlushDecoder
Description : Flush the decoder data buffer
              Caution: sends a SC, which may help the decoder to decode the last
              picture in buffer when flushing. Choice of the SC: SEQUENCE_END_CODE.
              Problem: this doesn't allow decoder to go on with the next coming data, as it says: end of sequence !!!
              SEQUENCE_ERROR_CODE can't be used because it may cause errors in the HW of the decoder ! (experienced)
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : DiscontinuityStartCodeInserted if TRUE CDFIFIO Flush is working correctly.
*******************************************************************************/
static void FlushDecoder(const HALDEC_Handle_t HALDecodeHandle, BOOL * const DiscontinuityStartCodeInserted_p)
{
    U32 i;
    U8 Gcf8 = 0, Cfg2;
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;
    STVID_ProvidedDataToBeInjected_t VideoDataToBeInjected;

    /* The video input CD fifo is 128 bytes + 4 more bytes due to PES parser */
    #define FLUSH_SIZE                           132
    #define FLUSH_PES_HEADER_SIZE                9
    #define FLUSH_PES_HEADER_TO_BE_CONSIDERED_SIZE  3
    #define FLUSH_ES_HEADER_SIZE                 6

    /* For ES: we want to inject 0xAFAF000001B6 + 0xFF * FLUSH_SIZE
    For PES: we want to inject 0x000001E000nn800000AFAF000001B6 + 0xFF * FLUSH_SIZE, where nn is the size of the packet (byte after nn until last 0xFF)
                                \__ PES HEADER __/\__data that will be seen in bit buffer__/ */
    static const U8 FlushDecoderSequence[FLUSH_PES_HEADER_SIZE + FLUSH_ES_HEADER_SIZE + FLUSH_SIZE] =
    {
        /* System stream: inject PES header before injection data to flush, otherwise data will be filtered and lost with no flush ! */
        0x00, 0x00, 0x01, 0xE0,
        /* On 2 bytes: size of data in PES packet in bytes */
        0x00, FLUSH_SIZE + FLUSH_PES_HEADER_TO_BE_CONSIDERED_SIZE + FLUSH_ES_HEADER_SIZE,
        /* 3 bytes end of PES info */
        0x80, 0x00, 0x00,
        GREATEST_SLICE_START_CODE, GREATEST_SLICE_START_CODE, 0x00, 0x00, 0x01, DISCONTINUITY_START_CODE,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };

    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
             /* The register is not documented because the data insertion is not available on 5508/5518. */
             /* ====> DiscontinuityStartCodeInserted is FALSE for these Device Types.                    */
             /* Notice that the flush of the stream remaining in the CDFIFO is working.                  */
             *DiscontinuityStartCodeInserted_p = TRUE;
            break;

        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :

            *DiscontinuityStartCodeInserted_p = TRUE;

             /* Mask selection for Compressed Data Register */
             Gcf8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + CFG_GCF) & ~CFG_GCF_VID_INPUT;

             /* Select Video input for CDR */
             HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + CFG_GCF, Gcf8 | CFG_GCF_VID_INPUT);
             break;
        default:
             /* Should not occur */
             break;
    }


    Cfg2 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_CF2);
    if ((Cfg2 & PES_CF2_SS) != 0)
    {
        /* system stream discontinuity pattern has to be injected */
        VideoDataToBeInjected.SizeInBytes = FLUSH_SIZE + FLUSH_PES_HEADER_SIZE + FLUSH_ES_HEADER_SIZE;
        VideoDataToBeInjected.Address_p = (void *)(&FlushDecoderSequence[0]);
    }
    else
    {
        /* pure video stream discontinuity pattern has to be injected */
        VideoDataToBeInjected.SizeInBytes = FLUSH_SIZE + FLUSH_ES_HEADER_SIZE;
        VideoDataToBeInjected.Address_p = (void *)(&FlushDecoderSequence[FLUSH_PES_HEADER_SIZE]);
    }

    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :

            /* Ask application injection the provided sequence */
            STEVT_Notify(((HALDEC_Properties_t *) HALDecodeHandle)->EventsHandle, Data_p->EventIDFlushDecoderPatternToBeInjected, STEVT_EVENT_DATA_TYPE_CAST (&VideoDataToBeInjected));

             break;

        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :

             /* WARNING : this registers MAY be accessed by other process (ie audio driver)
             Since this is NOT the case for the moment (and nothing is plan in the futur)
             access to those registers are not protected against concurent access */

             /* PES data for flush */
             if ((Cfg2 & PES_CF2_SS) != 0)
             {
                for (i = 0; i < FLUSH_PES_HEADER_SIZE; i++)
                {
                    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + CFG_CDR, FlushDecoderSequence[i]);
                }
             }

             /* ES data for flush */
             for (i = FLUSH_PES_HEADER_SIZE; i < (FLUSH_PES_HEADER_SIZE + FLUSH_ES_HEADER_SIZE + FLUSH_SIZE) ; i++)
             {
                 HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + CFG_CDR, FlushDecoderSequence[i]);
             }


             /* External strobes selected */
             HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + CFG_GCF, Gcf8 | CFG_GCF_EXT_INPUT);

             break;

        default :
             *DiscontinuityStartCodeInserted_p = FALSE;
            break;
    }

} /* End of FlushDecoder() function */


/*******************************************************************************
Name        : Get16BitsHeaderData
Description : Get 16 bit word of header data
Parameters  : HAL decode handle
Assumptions : Header FIFO is not empty, must be checked with IsHeaderDataFIFOEmpty()
Limitations :
Returns     : 16bit word of header data
*******************************************************************************/
static U16 Get16BitsHeaderData(const HALDEC_Handle_t HALDecodeHandle)
{
    U16 Val16;

    /* Header FIFO is not empty: get data */
    /* Lock for security because 2 reads in cycle register must be done together */
#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    interrupt_lock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */
    Val16 =        (((U16) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDF)) <<  8);
    Val16 = Val16 | ((U16) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDF));
#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    interrupt_unlock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */

    return(Val16);
} /* End of Get16BitsHeaderData() function */


/*******************************************************************************
Name        : Get16BitsStartCode
Description : Get 16 bit word of start code, says if start code is on MSB/LSB
Parameters  : HAL decode handle, pointer to variable saying if SC is on MSB/LSB
Assumptions : Header FIFO is not empty, must be checked with IsHeaderDataFIFOEmpty()
Limitations :
Returns     : 16bit word of start code, FALSE if SC on LSB (MSB is meaningless), FALSE if SC is on MSB (LSB is next data)
*******************************************************************************/
static U16 Get16BitsStartCode(const HALDEC_Handle_t HALDecodeHandle, BOOL * const StartCodeOnMSB_p)
{
    U16 Val16;

    *StartCodeOnMSB_p = (((HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDS)) & VID_HDS_SCM) != 0);

    /* Header FIFO is not empty: get data */
    /* Lock for security because 2 reads in cycle register must be done together */
#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    interrupt_lock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */
    Val16 =        (((U16) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDF)) <<  8);
    Val16 = Val16 | ((U16) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDF));
#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    interrupt_unlock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */

    return(Val16);
} /* End of Get16BitsStartCode() function */


/*******************************************************************************
Name        : Get32BitsHeaderData
Description : Get 32bit word of header data
Parameters  : HAL decode handle
Assumptions : Header FIFO is not empty, must be checked with IsHeaderDataFIFOEmpty()
Limitations : Be careful: reading too much bytes can cause problem: if there is the beginning
              of a start code inside (like xx 00 01 xx), this will cause a SCH interrupt
              even if it was not requested !
Returns     : 32bit word of header data
*******************************************************************************/
static U32 Get32BitsHeaderData(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 Val32;

    /* Header FIFO is not empty: get data */
    /* Lock for security because 2 reads in cycle register must be done together */
#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    interrupt_lock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */
    Val32 =         (((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDF)) << 24);
    Val32 = Val32 | (((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDF)) << 16);
    Val32 = Val32 | (((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDF)) <<  8);
    Val32 = Val32 | (((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDF))      );
#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    interrupt_unlock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */

    return(Val32);
} /* End of Get32BitsHeaderData() function */


/*******************************************************************************
Name        : GetAbnormallyMissingInterruptStatus
Description : Get interrupt status that could not be get through interrupt
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Decoder status missing in interrupts
*******************************************************************************/
static U32 GetAbnormallyMissingInterruptStatus(const HALDEC_Handle_t HALDecodeHandle)
{
#ifdef WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES
    U32 Tmp32;
#endif /* WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES */
#if defined WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES || defined WA_PROBLEM_SCH_MISSING_RANDOMLY
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;
#endif /* WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES */
    U32 MissingStatus = 0;

#ifdef WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES
    Tmp32 = GetDecodeBitBufferLevel(HALDecodeHandle);
    if ((Data_p->GotPipelineIdleWithNoDecoderIdle) && /* Got PII with no DEI */
        (time_after(time_now(), Data_p->MaxDEIMissingTime) != 0) && /* Long time since last PII occured */
        (Data_p->BitBufferSize > 256) && /* Size is high enough to avoid division by 0 */
        ((Tmp32 / (Data_p->BitBufferSize >> 8)) > 230) /* Level is more than 90% : (Level > 90% size) i.e. (256 * level / Size > 256 * 90%) */
       )
    {
        /* There must be a problem: do a pipe reset ! */
        PipelineReset(HALDecodeHandle);
#ifdef TRACE_UART
        TraceBuffer(("-**** ACTION FOLLOWING PEI MISSING !!! (time=%d) *****", time_now()));
#endif /* TRACE_UART */
    }
#endif /* WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES */

#if defined WA_PROBLEM_SCH_MISSING_RANDOMLY
    /* Check if SCH interrupt is missing */
    if (Data_p->IsStartCodeExpectedAfterLaunchByDecode)
    {
        /* Fix for missing SCH problem on STi5508/18: probably HW problem causing that... */
        Tmp32 = GetStatus(HALDecodeHandle);
        if ((Tmp32 & HALDEC_START_CODE_HIT_MASK) != 0)
        {
#ifdef TRACE_UART
            /* debug only */
            TraceBuffer(("MISSING SCH interrupt ?!?!?!?!?!?!?!?!?! (time=%d)", time_now()));
#endif /* TRACE_UART */
            /* Check flag again: maybe interrupt has occured since then, and flag is FALSE. */
            if (Data_p->IsStartCodeExpectedAfterLaunchByDecode)
            {
                MissingStatus |= HALDEC_START_CODE_HIT_MASK;
                Data_p->IsStartCodeExpectedAfterLaunchByDecode = FALSE;
            }
        }
    }
#endif /* WA_PROBLEM_SCH_MISSING_RANDOMLY */

    return(MissingStatus);
} /* End of GetAbnormallyMissingInterruptStatus() function */

/*******************************************************************************
Name        : GetDataInputBufferParams
Description : not used in 55xx where injection is not controlled by video
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetDataInputBufferParams(
                                const HALDEC_Handle_t HALDecodeHandle,
                                void ** const BaseAddress_p,
                                U32 * const Size_p)
{
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*******************************************************************************
Name        : GetBitBufferOutputCounter
Description : Gets the number of bytes of data which have gone through the SC detector
              since the init of the HAL.
Parameters  : HAL decode handle
Assumptions :
Limitations : Be careful with loops to 0 after 0xffffffff ! (user should remember last
              value and test: if last value >= current value there was a loop to 0)
Returns     : 32bit word OutputCounter value in units of bytes
*******************************************************************************/
static U32 GetBitBufferOutputCounter(const HALDEC_Handle_t HALDecodeHandle)
{
    /* Returned value obtained at last SCH interrupt, as SCDcount can only be read when no SC search on-going */
    return(((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->LatchedSCD32bitValue);
}

/*******************************************************************************
Name        : GetCDFifoAlignmentInBytes
Description :
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetCDFifoAlignmentInBytes(const HALDEC_Handle_t HALDecodeHandle, U32 * const CDFifoAlignment_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            *CDFifoAlignment_p = CD_FIFO_ALIGNMENT;
            break;

        default :
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }

    return(ErrorCode);
} /* End of GetCDFifoAlignmentInBytes() function */


/*******************************************************************************
Name        : GetDecodeBitBufferLevel
Description : Get decoder hardware bit buffer level in bytes
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Level in bytes, with granularity of 256 bytes
*******************************************************************************/
static U32 GetDecodeBitBufferLevel(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 Level, CDCount;
    U8 V01, V00, V10, V11;
    S32 Distance;
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    U32 DiffPSC, FirstRead, SecondRead;
#endif /* CHECK_SCD_PIPE_MISALIGNMENT */

    if (Data_p->IsDecodingInTrickMode)
    {
        CDCount   = ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT)) << 16;
        CDCount  |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT)) << 8;
        CDCount  |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT));

        if (Data_p->CDCountValueBeforeLastFlushBegin > CDCount)
        {
            /* CDCount < last read value means that the value has looped the max value (=0xFFFFFF) */
            return((0xFFFFFF - Data_p->CDCountValueBeforeLastFlushBegin) + CDCount);
        }
        return(CDCount - Data_p->CDCountValueBeforeLastFlushBegin);
    }

    /* Read registers twice, because this is always changing, so need to handle overlaps ! */
    /* Warning : the order to read the registers is important ! */
    V00 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_VBL0);
    V01 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_VBL1) & 0x7F;
    V10 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_VBL0);
    V11 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_VBL1) & 0x7F;

#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    if (Data_p->IsDecoderIdle)
    {
        /* warning, while reading the 3 cycles, carry may be propagated from                */
        /* one register to the following one                                                */
        /* example : 0x40 FF FF -> 0x41 00 00                                               */
        /* FirstRead = 0x40 FF FF, SecondRead = 0x40 00 00 use First Read                   */
        /* (this means carry has been latched while reading second cycle of the second read)*/
        /* FirstRead = 0x40 00 00, SecondRead=  0x41 00 00 use second read                  */
        /* (this means carry has been latched while reading second cycle of first cycle )   */

        /* CDCount is in 16-bit units */
        FirstRead   = ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT)) << 16;
        FirstRead  |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT)) << 8;
        FirstRead  |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT));
        SecondRead  = ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT)) << 16;
        SecondRead |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT)) << 8;
        SecondRead |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT));

        /* --- Deduce the real CD count pointer --- */
        if ((FirstRead > 128) && (SecondRead > 128))
        {
            if (FirstRead > SecondRead)
            {   /* it means carry has been propagated during second read */
                /* so use the first read */
                Data_p->LatchedVLDOffset = FirstRead;
            }
            else
            {
                Data_p->LatchedVLDOffset = SecondRead;
            }
        }
        else
        {
            Data_p->LatchedVLDOffset = SecondRead;
        }
        if (Data_p->PreviousVLDOffset > Data_p->LatchedVLDOffset)
        {
            DiffPSC = 0x1000000 - Data_p->PreviousVLDOffset + Data_p->LatchedVLDOffset;
        }
        else
        {
            DiffPSC = Data_p->LatchedVLDOffset - Data_p->PreviousVLDOffset;
        }
        Data_p->LatchedVLDAddress = Data_p->PreviousVLDAddress + DiffPSC;
        while (Data_p->LatchedVLDAddress >= (Data_p->BitBufferBase + Data_p->BitBufferSize - 1))
        {
            Data_p->LatchedVLDAddress -= Data_p->BitBufferSize;
        }
        Data_p->PreviousVLDOffset = Data_p->LatchedVLDOffset;
        Data_p->PreviousVLDAddress = Data_p->LatchedVLDAddress;

        Data_p->IsVLDValid = TRUE;

#ifdef TRACE_UART
/*        TraceBuffer(("-VLDOff=0x%x/VLDAdd=0x%x", Data_p->PreviousVLDOffset, Data_p->PreviousVLDAddress));*/
#endif /* TRACE_UART */
    }
#endif /* CHECK_SCD_PIPE_MISALIGNMENT */

    /* Deduce the real bit buffer level: take first read and check if it's valid */
    Level = (((U32) V01) << 16) | (((U32) V00) << 8);
    if (((V00 == 0x00) || (V00 == 0xFF)) && (V11 == V01))
    {
        /* It is possible that first read was spoiled (V01 does not corresponds to V00) */
        Distance = ((S32) V00) - ((S32) V10);
        if ((Distance > 128) || (Distance < -127))
        {
            /* Use the second read in this case */
            Level = (((U32) V11) << 16) | (((U32) V10) << 8);
        }
    }

#ifdef TRACE_UART
/*     TraceBuffer(("-BBL=%d", Level));*/
#endif /* TRACE_UART */

    return(Level);
} /* End of GetDecodeBitBufferLevel() function */

#ifdef STVID_HARDWARE_ERROR_EVENT
/*******************************************************************************
Name        : GetHardwareErrorStatus
Description : Get the harware error status. That means that if the softaware
              detects any hardware error, and performs a work-around, it saves
              all work-around activated status.
              A read of this status make a reset of it.
Parameters  : HAL Decode manager handle
Assumptions : The result should manage masks. So, if the hardware error mecanism
              is not applied, just return the zero value for no hardware error.
Limitations :
Returns     : Harware error status.
*******************************************************************************/
static STVID_HardwareError_t GetHardwareErrorStatus(const HALDEC_Handle_t HALDecodeHandle)
{
    /* Not yet implemented on Omega1. Just return no error. */
    return(STVID_HARDWARE_NO_ERROR_DETECTED);

} /* End of GetHardwareErrorStatus() function */
#endif /* STVID_HARDWARE_ERROR_EVENT */

/*******************************************************************************
Name        : GetInterruptStatus
Description : Get decoder interrupt status information
Parameters  : HAL decode handle
Assumptions : Must be called inside interrupt context
Limitations :
Returns     : Decoder interrupt status
*******************************************************************************/
static U32 GetInterruptStatus(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 HWStatus;
    U32 Mask;
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;
    U32 DiffPSC;
#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    S32 SuspectedCD, Drift;
    U32 MissingStatus = 0;
#endif /* CHECK_SCD_PIPE_MISALIGNMENT */
#if defined WA_PROBLEM_SCH_MISSING_RANDOMLY
    U32 Tmp32;
#endif /* WA_PROBLEM_SCH_MISSING_RANDOMLY */

    Mask  = ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITM2)) << 16;
    Mask |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITM0));
    Mask |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITM1)) <<  8;

    Mask = TranslateToDecodeStatus(HALDecodeHandle, Mask);

    /* Prevent from loosing interrupts ! Disable all interrupts */
    HALDEC_Write8( (U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITM2, 0);
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITM16, 0);

    /* Most significant byte must be read last, because it clears the register */
    HWStatus  = ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS2)) << 16;
    HWStatus |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS0));
    HWStatus |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS1)) <<  8;

    /* Release video interrupts ! */
    SetInterruptMask(HALDecodeHandle, Mask);

    /* Call hidden VTG function which generates VSync events (patch omega1 which has no VTG hardware) */
    if ((HWStatus & VID_ITX_VST) != 0)
    {
#ifdef TRACE_DECODE
#ifdef TRACE_VSYNCS
#ifdef TRACE_UART
         TraceBuffer(("\r\n VST-"));
         {
            U32 Level = GetDecodeBitBufferLevel(HALDecodeHandle);
            U8 Percent;
            Percent = Level * 100 / 0x5D000;
            if ((Percent == 0) && (Level != 0))
            {
                Percent = 1;
            }
            TraceBuffer(("-%2d%%", Percent));
         }
/*         TraceBuffer(("T"));*/
        if (WasLastVSyncTop)
        {
/*           TraceBuffer(("******* MISSING VSYNC!!! ********\a\a\a"));*/
        }
#endif /* TRACE_UART */
#endif /* TRACE_VSYNCS */
        WasLastVSyncTop = TRUE;
#endif /* TRACE_DECODE */
        stvtg_SignalVSYNC(STVTG_TOP);
    }
    if ((HWStatus & VID_ITX_VSB) != 0)
    {
#ifdef TRACE_DECODE
#ifdef TRACE_VSYNCS
#ifdef TRACE_UART
         TraceBuffer(("\r\n  VSB"));
         {
            U32 Level = GetDecodeBitBufferLevel(HALDecodeHandle);
            U8 Percent;
            Percent = Level * 100 / 0x5D000;
            if ((Percent == 0) && (Level != 0))
            {
                Percent = 1;
            }
            TraceBuffer(("-%2d%%", Percent));
         }
/*         TraceBuffer(("B"));*/
        if (!(WasLastVSyncTop))
        {
           TraceBuffer(("******* MISSING VSYNC!!! ********\a\a\a"));
        }
#endif /* TRACE_UART */
#endif /* TRACE_VSYNCS */
        WasLastVSyncTop = FALSE;
#endif /* TRACE_DECODE */
        stvtg_SignalVSYNC(STVTG_BOTTOM);
    }

#if defined WA_PROBLEM_SCH_MISSING_RANDOMLY
    /* Check if SCH interrupt is missing */
    if (((HWStatus & VID_ITX_PII) != 0) && (Data_p->IsStartCodeExpectedAfterLaunchByDecode))
    {
        /* Fix for missing SCH problem on STi5508/18: probably HW problem causing that... */
        Tmp32 = GetStatus(HALDecodeHandle);
        if ((Tmp32 & HALDEC_START_CODE_HIT_MASK) != 0)
        {
#ifdef TRACE_UART
            if ((HWStatus & VID_ITX_SCH) == 0)
            {
                /* debug only */
                TraceBuffer(("MISSING SCH interrupt ?!?!?!?!?!?!?!?!?! (time=%d)", time_now()));
            }
#endif /* TRACE_UART */
            HWStatus |= VID_ITX_SCH;
        }
    }
#endif /* WA_PROBLEM_SCH_MISSING_RANDOMLY */

    /* Get SCD count if SCH interrupt, so that it is sure it is valid */
    if ((HWStatus & VID_ITX_SCH) != 0)
    {
        /* Got SCH interrupt */
#ifdef TRACE_UART
        /*TraceBuffer(("-SCH"));*/
#endif /* TRACE_UART */
#if defined WA_PROBLEM_SCH_MISSING_RANDOMLY
        /* Start code detector idle ! */
        Data_p->IsStartCodeDetectorIdle = TRUE;
        Data_p->IsStartCodeExpectedAfterLaunchByDecode = FALSE;
#endif /* WA_PROBLEM_SCH_MISSING_RANDOMLY */

        /* SCDCount is in 16-bit units */
        Data_p->LatchedSCD25bitValue  = ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_SCD_COUNT)) << 17;
        Data_p->LatchedSCD25bitValue |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_SCD_COUNT)) << 9;
        Data_p->LatchedSCD25bitValue |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_SCD_COUNT)) << 1;
        if (((HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDS)) & VID_HDS_SCM) != 0)
        {
            Data_p->LatchedSCD25bitValue ++;
        }
        else
        {
            Data_p->LatchedSCD25bitValue += 2;
        }
        if (Data_p->PreviousSCD25bitValue > Data_p->LatchedSCD25bitValue)
        {
            DiffPSC = 0x2000000 - Data_p->PreviousSCD25bitValue + Data_p->LatchedSCD25bitValue;
        }
        else
        {
            DiffPSC = Data_p->LatchedSCD25bitValue - Data_p->PreviousSCD25bitValue;
        }
        Data_p->LatchedSCD32bitValue = Data_p->PreviousSCD32bitValue + DiffPSC;
/*        while (Data_p->LatchedSCD32bitValue >= (Data_p->BitBufferBase + Data_p->BitBufferSize - 1))*/
/*        {*/
/*            Data_p->LatchedSCD32bitValue -= Data_p->BitBufferSize;*/
/*        }*/
        Data_p->PreviousSCD32bitValue = Data_p->LatchedSCD32bitValue;
        Data_p->PreviousSCD25bitValue = Data_p->LatchedSCD25bitValue;
        Data_p->IsSCDValid = TRUE;
#ifdef TRACE_UART
/*        TraceBuffer(("-SCDOff=0x%x/SCDAdd=0x%x", Data_p->PreviousSCD25bitValue, Data_p->PreviousSCD32bitValue));*/
#endif /* TRACE_UART */
    } /* End SCH interrupt */

#ifdef WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES
    if ((HWStatus & VID_ITX_PII) != 0)
    {
        /* PII: set flag to see if DEI is missing */
        Data_p->GotPipelineIdleWithNoDecoderIdle = TRUE;
        Data_p->MaxDEIMissingTime = time_plus(time_now(), MAX_DEI_MISING_ON_FIELD_PICTURES_TIME);
    }
    if (((HWStatus & VID_ITX_DEI) != 0) && (Data_p->GotPipelineIdleWithNoDecoderIdle))
    {
        /* No DEI missing after PEI */
        Data_p->GotPipelineIdleWithNoDecoderIdle = FALSE;
    }
#endif /* WA_PROBLEM_DEI_MISSING_ON_FIELD_PICTURES */

#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    /* When decoder idle */
    if ((HWStatus & VID_ITX_DEI) != 0)
    {
        Data_p->IsDecoderIdle = TRUE;
        if (Data_p->IsDecoderIdle)
        {
            if (Data_p->ResetJustDone)
            {
                Data_p->ResetJustDone = FALSE;
            }
            else
            {
                if (!(Data_p->IsVLDValid))
                {
                    Data_p->PreviousVLDOffset   = 0;
                    Data_p->PreviousVLDAddress  = Data_p->BitBufferBase;
                }

                /* Check alignment */
                if ((Data_p->IsSCDValid) && (Data_p->HadPreviousSCAfterSlice))
                {
                    /* Get ESBufferLevel and VLD values */
                    Data_p->ESBufferLevel = GetDecodeBitBufferLevel(HALDecodeHandle);
#ifdef TRACE_UART
        /*        TraceBuffer (("-LEVEL=0x%x", Data_p->ESBufferLevel));*/
#endif /* TRACE_UART */

                    /* Check pipe/SCD misalignment */
                    SuspectedCD = ((S32) Data_p->PreviousSCAfterSliceSCD25bitValue & 0xFFFFFF) + (S32) Data_p->ESBufferLevel;
                    if (SuspectedCD > 0x1000000)
                    {
                        SuspectedCD -= 0x1000000;
                    }
                    if ((SuspectedCD > 0x0C00000) && (Data_p->PreviousVLDOffset < 0x400000))
                    {
                        Drift = SuspectedCD - ((S32) 0x1000000 + (S32) Data_p->PreviousVLDOffset);
                    }
                    else
                    {
                        Drift = SuspectedCD - (S32) Data_p->PreviousVLDOffset;
                    }
#ifdef TRACE_UART
            /*        TraceBuffer (("-SuspectedCD=0x%x(drift=%d)", SuspectedCD, Drift));*/
#endif /* TRACE_UART */
                    if (Drift < -4096)
                    {
                        if (Data_p->AlreadyMisaligned)
                        {
                            /* May have spurious misalignment detection because of skip after a slice with truncated picture (VLD goes before SCD): so act after 2 times */
                            if (!(Data_p->IsCorrectingMisalignment))
                            {
#ifdef TRACE_UART
                                TraceBuffer (("-MISALIGNMENT: Pipe advance on header %d -> SC Search.\a", Drift));
#endif /* TRACE_UART */
                                MissingStatus |= HALDEC_MISALIGNMENT_PIPELINE_AHEAD_MASK;
    /*                            SearchNextStartCode(HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);*/
                                /* Don't do this twice ! */
                                Data_p->IsCorrectingMisalignment = TRUE;
                            }
                        }
                        Data_p->AlreadyMisaligned = TRUE;
                    }
                    else if (Drift > 4096)
                    {
                        if (Drift < Data_p->BitBufferSize)
                        {
                            if (Data_p->AlreadyMisaligned)
                            {
#ifdef TRACE_UART
                                TraceBuffer(("-MISALIGNMENT: Header advance on pipe %d -> Skip.\a", Drift));
#endif /* TRACE_UART */
                                /* May have spurious misalignment detection because of skip after a slice with truncated picture (VLD goes before SCD): so act after 2 times */
                                MissingStatus |= HALDEC_MISALIGNMENT_START_CODE_DETECTOR_AHEAD_MASK;
    /*                            SkipPicture(HALDecodeHandle, FALSE, HALDEC_SKIP_PICTURE_1_THEN_STOP);*/
                            }
                        }
                        Data_p->AlreadyMisaligned = TRUE;
                    }
                    else
                    {
                        Data_p->AlreadyMisaligned = FALSE;
                    }
                } /* end check alignment */
            } /* end not ResetJustDone */
        }
    }
#endif  /* CHECK_SCD_PIPE_MISALIGNMENT */

    return(TranslateToDecodeStatus(HALDecodeHandle, HWStatus)
#ifdef CHECK_SCD_PIPE_MISALIGNMENT
            | MissingStatus
#endif  /* CHECK_SCD_PIPE_MISALIGNMENT */
            );
} /* End of GetInterruptStatus() function */


/*******************************************************************************
Name        : GetLinearBitBufferTransferedDataSize
Description :
Parameters  : HAL decode handle
Assumptions : Size in bytes, with granularity of 8 bytes
Limitations : Returns     : 32bit size
*******************************************************************************/
static ST_ErrorCode_t GetLinearBitBufferTransferedDataSize(const HALDEC_Handle_t HALDecodeHandle, U32 * const LinearBitBufferTransferedDataSize_p)
{
    U32  LevelAddress, StartAddress;

    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
            LevelAddress = (((U32) ((U8*) HALDEC_Read24((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_CD_RD24)) & 0x000FFFFF) << 3);
            StartAddress = (((U32) ((U8*) HALDEC_Read24((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_CD24)) & 0x0001FFFF) << 6);
            break;

        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            LevelAddress = (((U32)(HALDEC_Read24((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_CD_RD24)) & 0x001FFFFF) << 3);
            StartAddress = (((U32)(HALDEC_Read24((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_CD24)) & 0x0003FFFF) << 6);
            break;

        default :
            *LinearBitBufferTransferedDataSize_p = 0;
            return (ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }

    *LinearBitBufferTransferedDataSize_p = LevelAddress - StartAddress; /* in byte. */

    return(ST_NO_ERROR);
} /* End of GetLinearBitBufferTransferedDataSize() function */


/*******************************************************************************
Name        : GetPTS
Description : Get current picture time stamp information if available
Parameters  : Decoder registers address, pointer on matrix values
Assumptions :
Limitations :
Returns     : TRUE if PTS available (loaded), FALSE if no PTS
*******************************************************************************/
static BOOL GetPTS(const HALDEC_Handle_t HALDecodeHandle, U32 * const PTS_p, BOOL * const PTS33_p)
{
    U8 TS33;
    U32 TS;

    if ((PTS_p == NULL) || (PTS33_p == NULL))
    {
        /* Error cases */
        return(FALSE);
    }

    /* Check if there is a PTS available */
    TS33 = STSYS_ReadRegDev8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_TS4);
    if ((TS33 & PES_TS_TSA) == 0)
    {
        /* No time stamp available: quit */
        return(FALSE);
    }

    /* There is a time stamp available: copy it */
    TS =       ((U32) STSYS_ReadRegDev8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_TS0));
    TS = TS | (((U32) STSYS_ReadRegDev8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_TS1)) << 8);
    TS = TS | (((U32) STSYS_ReadRegDev8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_TS2)) << 16);
    TS = TS | (((U32) STSYS_ReadRegDev8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_TS3)) << 24);
    *PTS_p = TS;
    *PTS33_p = ((TS33 & 1) != 0);

    return(TRUE);
} /* End of GetPTS() function */


/*******************************************************************************
Name        : GetSCDAlignmentInBytes
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetSCDAlignmentInBytes(const HALDEC_Handle_t HALDecodeHandle, U32 * const SCDAlignment_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            *SCDAlignment_p = 128;
            break;

        default :
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }

    return(ErrorCode);
} /* End of GetSCDAlignmentInBytes() function */

/*******************************************************************************
Name        : GetStartCodeAddress
Description : Get a pointer on the address of the Start Code if available
Parameters  : Decoder registers address, pointer on buffer address
Assumptions :
Limitations :
Returns     : TRUE if StartCode Address available FALSE if no
*******************************************************************************/
static ST_ErrorCode_t GetStartCodeAddress(const HALDEC_Handle_t HALDecodeHandle, void ** const BufferAddress_p)
{
    U32             SCDPointer;

    if (BufferAddress_p != NULL)
    {
        switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
        {
            case STVID_DEVICE_TYPE_5508_MPEG :
            case STVID_DEVICE_TYPE_5518_MPEG :
            case STVID_DEVICE_TYPE_5514_MPEG :
                SCDPointer = (U32)(STSYS_ReadRegDev16BE((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_SCD_RD16));
                break;

            case STVID_DEVICE_TYPE_5516_MPEG :
            case STVID_DEVICE_TYPE_5517_MPEG :
                SCDPointer = (U32)(STSYS_ReadRegDev24BE((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_SCD_RD24));
                break;

            default :
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }

        *BufferAddress_p = (void*) (((U32) SCDPointer) << 7);
        *BufferAddress_p =  (void *)((U32)(*BufferAddress_p) + ((U32)((HALDEC_Properties_t *) HALDecodeHandle)->SDRAMMemoryBaseAddress_p));
    }

    return (ST_NO_ERROR);
} /* End of GetStartCodeAddress() function */


/*******************************************************************************
Name        : GetStatus
Description : Get decoder status information
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Decoder status
*******************************************************************************/
static U32 GetStatus(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 HWStatus;

    HWStatus =   ((U32) HALDEC_Read16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STA16));
    HWStatus |= (((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STA2)) << 16);

    return(TranslateToDecodeStatus(HALDecodeHandle, HWStatus));
} /* End of GetStatus() function */


/*******************************************************************************
Name        : Init
Description : Initialise chip
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t Init(const HALDEC_Handle_t HALDecodeHandle)
{
    U8 Tmp8;
    Omega1Properties_t * Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Allocate a Omega1Properties_t structure */
    SAFE_MEMORY_ALLOCATE(((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p, void *,
                         ((HALDEC_Properties_t *) HALDecodeHandle)->CPUPartition_p,
                         sizeof(Omega1Properties_t));

    if (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :

            /* register the need event to have discontinuity pattern injectd by the application  */
            ErrorCode = STEVT_RegisterDeviceEvent(((HALDEC_Properties_t *) HALDecodeHandle)->EventsHandle,
                                                ((HALDEC_Properties_t *) HALDecodeHandle)->VideoName,
                                                STVID_PROVIDED_DATA_TO_BE_INJECTED_EVT,
                                                &(Data_p->EventIDFlushDecoderPatternToBeInjected));
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error: registration failed, undo initialisations done */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module hal decode init: could not register event !"));
                Term(HALDecodeHandle);
                return(STVID_ERROR_EVENT_REGISTRATION);
            }
            break;

        default :
            break;
    }

#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    /* Initialise values  */
    Data_p->IsVLDValid = FALSE;
    Data_p->IsCorrectingMisalignment = FALSE;
    Data_p->AlreadyMisaligned   = FALSE;
    Data_p->ResetJustDone = TRUE;
    Data_p->HasGotSlice = FALSE;
    Data_p->StartCodeFound = 0xFF;
    Data_p->IsDecoderIdle = FALSE;
    Data_p->HadPreviousSCAfterSlice = FALSE;
    Data_p->PreviousSCAfterSliceSCD25bitValue = 0;
#endif /* CHECK_SCD_PIPE_MISALIGNMENT */
#ifdef WA_PROBLEM_OVERWRITE_FIELDS
    Data_p->PreviousPictureStructure = PICTURE_STRUCTURE_FRAME_PICTURE;
#endif /* WA_PROBLEM_OVERWRITE_FIELDS */
    Data_p->BitBufferSize       = 0;
    Data_p->BitBufferBase       = 0;
    /* Invalidate SCD: 4 values to reset */
    Data_p->IsSCDValid = FALSE;
    Data_p->PreviousSCD25bitValue = 0;
/*    Data_p->PreviousSCD32bitValue  = Data_p->BitBufferBase - 8;*/
    Data_p->PreviousSCD32bitValue  = 0;
#if defined WA_PROBLEM_SCH_MISSING_RANDOMLY
    Data_p->IsStartCodeDetectorIdle = TRUE;
    Data_p->IsStartCodeExpectedAfterLaunchByDecode = FALSE;
#endif /* WA_PROBLEM_SCH_MISSING_RANDOMLY */
#if defined WA_PROBLEM_STL_CANNOT_BE_READ
    Data_p->STLValue = 0;
#endif /* WA_PROBLEM_STL_CANNOT_BE_READ */
    Data_p->IsDecodingInTrickMode = FALSE;
    Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_MUST_DECODE;
    Data_p->FirstDecodeAfterSoftResetDone = FALSE;
    Data_p->CDCountValueBeforeLastFlushBegin = 0;

    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CTL, VID_CTL_ERS | VID_CTL_ERU | VID_CTL_EDC);

    /* Clear pending interrupts. Most significant byte must be read last, because it clears the register */
    Tmp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS2);
    Tmp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS0);
    Tmp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS1);

    /* MPEG control !!! */
/*    HALDEC_Write8((U8 *) 0x5001, 0x70);*/

    /* Config !!! */
    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
            Tmp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + CFG_CCF);
            HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + CFG_CCF, Tmp8 &~ CFG_CCF_EOU);
            break;

        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            /* No trick mode by default */
            HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_LDP, 0);
            break;

        default :
            break;
    }

    /* Set interrupt mask to 0: disable interrupts */
    /* Set interrupt mask to enable VSYNC interrupts ! (patch omega1 which has no VTG hardware) */
    SetInterruptMask(HALDecodeHandle, 0);

    return(ErrorCode);
 /* End of Init() function */}


/*******************************************************************************
Name        : IsHeaderDataFIFOEmpty
Description : Tells whether the header data FIFO is empty
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : TRUE is empty, FALSE if not empty
*******************************************************************************/
static BOOL IsHeaderDataFIFOEmpty(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 Status;

    Status = GetStatus(HALDecodeHandle);

    return((Status & HALDEC_HEADER_FIFO_EMPTY_MASK) != 0);
} /* End of IsHeaderDataFIFOEmpty() function */


/*******************************************************************************
Name        : LoadIntraQuantMatrix
Description : Load quantization matrix for intra picture decoding
Parameters  : HAL decode handle, pointers on matrix values (if NULL: no matrix loaded)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LoadIntraQuantMatrix(const HALDEC_Handle_t HALDecodeHandle, const QuantiserMatrix_t * Matrix_p, const QuantiserMatrix_t * ChromaMatrix_p)
{
    U32 i;

    if (Matrix_p != NULL)
    {
        /* Load intra quant matrix */
        HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDS, VID_HDS_QMI | VID_HDS_SOS);
        for (i = 0; i < 64; i++)
        {
            HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_QMW, (*Matrix_p)[i]);
        }
    }

    /* Commented because nothing to load chroma intra quant matrix on STi55xx ! */
/*    if (ChromaMatrix_p != NULL)*/
/*    {*/
        /* Nothing to load chroma intra quant matrix on STi55xx ! */
/*    }*/
} /* End of LoadIntraQuantMatrix() function */


/*******************************************************************************
Name        : LoadNonIntraQuantMatrix
Description : Load quantization matrix for non-intra picture decoding
Parameters  : HAL decode handle, pointers on matrix values (if NULL: no matrix loaded)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LoadNonIntraQuantMatrix(const HALDEC_Handle_t HALDecodeHandle, const QuantiserMatrix_t * Matrix_p, const QuantiserMatrix_t * ChromaMatrix_p)
{
    U32 i;

    if (Matrix_p != NULL)
    {
        /* Load non intra quant matrix */
        HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDS, 0 | VID_HDS_SOS);
        for (i = 0; i < 64; i++)
        {
            HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_QMW, (*Matrix_p)[i]);
        }
    }

    /* Commented because nothing to load chroma intra quant matrix on STi55xx ! */
/*    if (ChromaMatrix_p != NULL)*/
/*    {*/
        /* Nothing to load chroma intra quant matrix on STi55xx ! */
/*    }*/
} /* End of LoadNonIntraQuantMatrix() function */


/*******************************************************************************
Name        : PipelineReset
Description :
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PipelineReset(const HALDEC_Handle_t HALDecodeHandle)
{
    U8 Tmp8;

    Tmp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CTL) | VID_CTL_PRS;
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CTL, Tmp8);
    STTBX_WaitMicroseconds(1);
    Tmp8 &= ~VID_CTL_PRS;
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CTL, Tmp8);
} /* End of PipelineReset() function */


/*******************************************************************************
Name        : PrepareDecode
Description : Prepare decode of a picture
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PrepareDecode(const HALDEC_Handle_t HALDecodeHandle, void * const DecodeAddressFrom_p, const U16 pTemporalReference, BOOL * const WaitForVLD_p)
{
    U8 Value8;
    U16 Value16;
    void * PictureAddressToDecodeFrom_p;

    /* If decode launched from address, VLD is ready only when TR_OK it occurs : Wait for this IT before decode */
    *WaitForVLD_p = TRUE;

    /* done now in the HALDEC_SetSmoothBackwardConfiguration function called when switching from auto to manual. */
    /* Set bit INR : do not launch the start code detector when the decode is launched. */
#if defined WA_PROBLEM_STL_CANNOT_BE_READ
    Value8 = ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->STLValue;
#else /* WA_PROBLEM_STL_CANNOT_BE_READ */
    Value8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STL);
#endif /* not WA_PROBLEM_STL_CANNOT_BE_READ */
    Value8 |= VID_STL_INR_SET;
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STL, Value8);
#if defined WA_PROBLEM_STL_CANNOT_BE_READ
    ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->STLValue = Value8;
#endif /* WA_PROBLEM_STL_CANNOT_BE_READ */

    /* Set register bit Trick Mode Enable. */
    /* Two lines below should removed !!! */
    Value8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_LDP);
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_LDP, Value8 | VID_TP_LDP_TM_SET);

    /* Give to the VLD the address of the picture. */
    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
            HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_VLD16,
                    (((U32) (DecodeAddressFrom_p) - DECODE_FROM_ADDRESS_AREA_TO_RETRIEVE -
                    ((U32)((HALDEC_Properties_t *) HALDecodeHandle)->SDRAMMemoryBaseAddress_p))) / 128);

            break;

        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            PictureAddressToDecodeFrom_p = (void*)(((U32)(DecodeAddressFrom_p) - DECODE_FROM_ADDRESS_AREA_TO_RETRIEVE -
                    ((U32)((HALDEC_Properties_t *) HALDecodeHandle)->SDRAMMemoryBaseAddress_p)) / 128);
            HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_VLD24,
                     (U8)((U32)(PictureAddressToDecodeFrom_p) >> 16));
            HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_VLD16,
                     (U16)((U32)PictureAddressToDecodeFrom_p));
            break;

        default:
            break;
    }

    /* Write the temporal reference. */
    Value16 = HALDEC_Read16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TRF16);
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TRF16, (Value16 & (U16)(~(U16)(VID_TRF_MASK))) | (pTemporalReference & VID_TRF_MASK));

    /* Set bit TRF_TML : trick mode launch. */
    Value16 = HALDEC_Read16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TRF16);
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TRF16, Value16 | VID_TRF_TML );

} /* End of PrepareDecode() function */


/*******************************************************************************
Name        : ResetPESParser
Description :
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ResetPESParser(const HALDEC_Handle_t HALDecodeHandle)
{
    U8 cf2;

    cf2 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_CF2);

    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_CF2, cf2 & ~PES_CF2_SS);
    STTBX_WaitMicroseconds(1); /* Needed ? */

    /* Write back value: to restore SS configuration. */
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_CF2, cf2);
} /* End of ResetPESParser() function */


/*******************************************************************************
Name        : SearchNextStartCode
Description : Launch HW search for next start code. SCH interrupt occurs when found.
Parameters  : Decoder registers address, search mode,
              address for search mode VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS
Assumptions :
Limitations : Only VIDDEC_START_CODE_SEARCH_MODE_NORMAL supported, parameter SearchAddress_p not used
Returns     : Nothing
*******************************************************************************/
static void SearchNextStartCode(const HALDEC_Handle_t HALDecodeHandle, const HALDEC_StartCodeSearchMode_t SearchMode, const BOOL FirstSliceDetect, void * const SearchAddress_p)
{
    U8 Tmp8 = VID_HDS_START;
    U8 Value8;

#if defined WA_PROBLEM_SCH_MISSING_RANDOMLY
    ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->IsStartCodeDetectorIdle = FALSE;
#endif /* WA_PROBLEM_SCH_MISSING_RANDOMLY */

    if (FirstSliceDetect)
    {
        Tmp8 |= VID_HDS_SOS;
    }

    switch (SearchMode)
    {
        case HALDEC_START_CODE_SEARCH_MODE_NORMAL :
            break;

        case HALDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS :
            /* Set register bit Trick Mode Enable. */
            /* 2 lines below should be removed !!! */
            Value8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_LDP);
            Value8 |= VID_TP_LDP_TM_SET;
            HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_LDP, Value8);
            /* Give to the SCD the address of the data. */
            HALDEC_Write24((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_SCD24, ((U32) SearchAddress_p) / 64);

            /* Set bit STL : reset CD FIFO. */
#if defined WA_PROBLEM_STL_CANNOT_BE_READ
            Value8 = ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->STLValue;
#else /* WA_PROBLEM_STL_CANNOT_BE_READ */
            Value8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STL);
#endif /* not WA_PROBLEM_STL_CANNOT_BE_READ */
            Value8 |= VID_STL_STL_SET;
            if ((((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType == STVID_DEVICE_TYPE_5517_MPEG) ||
                    (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType == STVID_DEVICE_TYPE_5516_MPEG))
            {
                Value8 &= ~VID_STL_DSS_SET;
            }
            HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STL, Value8 );
#if defined WA_PROBLEM_STL_CANNOT_BE_READ
            /* Don't update STLValue with VID_STL_STL_SET, as it is reset at execution... */
#endif /* WA_PROBLEM_STL_CANNOT_BE_READ */
            break;
        default :
            break;
    }

    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_HDS, Tmp8);
} /* End of SearchNextStartCode() function */


/*******************************************************************************
Name        : SetBackwardLumaFrameBuffer
Description : Set decoder backward reference luma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBackwardLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    /* Beginning of the luma frame buffer where to decode, in unit of 2 Kbits */
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_BFP16, ((U32) BufferAddress_p) / 256);
} /* End of SetBackwardLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetBackwardChromaFrameBuffer
Description : Set decoder backward reference chroma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBackwardChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    /* Beginning of the chroma frame buffer where to decode, in unit of 2 Kbits */
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_BFC16 , ((U32) BufferAddress_p) / 256);
} /* End of SetBackwardChromaFrameBuffer() function */

/*******************************************************************************
Name        : SetDataInputInterface
Description : not used in 55xx where injection is not controlled by video
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetDataInputInterface(
          const HALDEC_Handle_t HALDecodeHandle,
          ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle,
                                          void ** const Address_p),
          void (*InformReadAddress)(void * const Handle, void * const Address),
          void * const Handle )
{
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*******************************************************************************
Name        : SetDecodeBitBuffer
Description : Set the location of the decoder bit buffer
Parameters  : HAL decode handle, buffer address, buffer size in bytes
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeBitBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p, const U32 BufferSize, const HALDEC_BitBufferType_t BufferType)
{
    U8 Tmp8;
    U32 CDCount;
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;


    switch (BufferType)
    {
        case HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR :
            /* Beginning of the bit buffer in unit of 2 Kbits */
            HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_VBG16, ((U32) BufferAddress_p) / 256);
            /* End of the bit buffer in unit of 2 Kbits */
            HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_VBS16, ((U32) BufferAddress_p + (U32) BufferSize - 1) / 256 - 1);

            /* Need to soft reset so that HW takes it into account ? */
            DecodingSoftReset(HALDecodeHandle, TRUE/* Dummy value */);
            break;
        case HALDEC_BIT_BUFFER_HW_MANAGED_LINEAR :
            /* Feature not supported by the software */
            break;
        case HALDEC_BIT_BUFFER_SW_MANAGED_CIRCULAR :
            /* Feature not supported by the software */
            break;
        case HALDEC_BIT_BUFFER_SW_MANAGED_LINEAR :
            switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
            {
                case STVID_DEVICE_TYPE_5508_MPEG :
                case STVID_DEVICE_TYPE_5518_MPEG :
                case STVID_DEVICE_TYPE_5514_MPEG :
                case STVID_DEVICE_TYPE_5516_MPEG :
                case STVID_DEVICE_TYPE_5517_MPEG :
                    /* Set register bit Trick Mode Enable. */
                    /* 3 lines below should removed !!! */
                    Tmp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_LDP);
                    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_LDP, Tmp8 | VID_TP_LDP_TM_SET);
                    Data_p->IsDecodingInTrickMode = TRUE;

                    /* the CD FIFO should be then be flushed to avoid to have a linear bit buffer have its firts datas with the last */
                    /* data of the previous linear bit buffer flush request. But it may give too many constrainst to the appli ? */
                    /* FlushDecoder(HALDecodeHandle, &TmpBool); */

                    /* Give to the CDFIFO the address where to output its data. */
                    HALDEC_Write24((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_CD24,
                             (((U32) (BufferAddress_p) - ((U32)((HALDEC_Properties_t *) HALDecodeHandle)->SDRAMMemoryBaseAddress_p ))) >> 6);

                    /* Write the address of the end of the buffer in the pointer of the CDFIFO */
                    if (   (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType == STVID_DEVICE_TYPE_5508_MPEG)
                        || (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType == STVID_DEVICE_TYPE_5518_MPEG))
                    {
                        HALDEC_Write24((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_CDLIMIT24,
                                 ((((U32) BufferAddress_p - ((U32)((HALDEC_Properties_t *) HALDecodeHandle)->SDRAMMemoryBaseAddress_p) + (U32) BufferSize - 1) >> 6) & 0x01FFFF));
                    }
                    else
                    {
                        HALDEC_Write24((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_CDLIMIT24,
                                 ((((U32) BufferAddress_p - ((U32)((HALDEC_Properties_t *) HALDecodeHandle)->SDRAMMemoryBaseAddress_p) + (U32) BufferSize - 1) >> 6) & 0x03FFFF));
                    }

                    /* Set bit CWL : reset CD FIFO. */
                    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CWL, VID_CWL_SET);

                    CDCount  = ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT)) << 16;
                    CDCount |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT)) << 8;
                    CDCount |= ((U32) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_CD_COUNT));

                    Data_p->CDCountValueBeforeLastFlushBegin = CDCount;

                    break;

                default :
                    break;
            }
            break;

        default:
            /* Nothing to do */
            break;
    }

    ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->BitBufferBase = (U32) BufferAddress_p;
    ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->BitBufferSize = BufferSize;

} /* End of SetDecodeBitBuffer() function */


/*******************************************************************************
Name        : SetDecodeBitBufferThreshold
Description : Set the threshold of the decoder bit buffer causing interrupt
Parameters  : HAL decode handle, threshold
Assumptions : 5510: Threshold must be aligned on 2 Kbits. If not, it is done as if it were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeBitBufferThreshold(const HALDEC_Handle_t HALDecodeHandle, const U32 BitBufferThreshold)
{
    /* Threshold of the bit buffer in unit of 2 Kbits */
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_VBT16, ((U32) BitBufferThreshold) / 256);
} /* End of SetDecodeBitBufferThreshold() function */


/*******************************************************************************
Name        : SetDecodeConfiguration
Description : Set configuration for decode
Parameters  : HAL decode handle, stream information
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeConfiguration(const HALDEC_Handle_t HALDecodeHandle, const StreamInfoForDecode_t * const StreamInfo_p)
{
    U8 TmpPpr1, TmpPpr2, TmpPfh, TmpPfv;
    const PictureCodingExtension_t * PicCodExt_p = &(StreamInfo_p->PictureCodingExtension);
    const Picture_t * Pic_p = &(StreamInfo_p->Picture);
    U8 WidthInMacroBlock;
    U16 TotalInMacroBlock;
    U16 Tmp16;
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;


    /* Hardware constraint : after decoding a field, we must decode a 2nd field to keep the right polarity. */
    if ((!(StreamInfo_p->MPEG1BitStream)) && (PicCodExt_p->picture_structure != PICTURE_STRUCTURE_FRAME_PICTURE))
    {
        if (!Data_p->FirstDecodeAfterSoftResetDone)
        {
            Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_MUST_DECODE;
        }
        else
        {
            if (Data_p->DecodeSkipConstraint == HALDEC_DECODE_SKIP_CONSTRAINT_MUST_DECODE)
            {
                Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_NONE;
            }
            else if (Data_p->DecodeSkipConstraint == HALDEC_DECODE_SKIP_CONSTRAINT_NONE)
            {
                Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_MUST_DECODE;
            }
        }
    }
    else
    {
        Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_NONE;
    }

    /* Write width in macroblocks */
    WidthInMacroBlock = (StreamInfo_p->HorizontalSize + 15) / 16;
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_DFW, WidthInMacroBlock);

    /* Write total number of macroblocks */
    TotalInMacroBlock = ((StreamInfo_p->VerticalSize + 15) / 16) * WidthInMacroBlock;
    /* Lock for security because 2 reads in cycle register must be done together */
#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    interrupt_lock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */
    /* On 5510 and 5512: report bit CIF not to disturb it ! */
    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
            Tmp16 = ((U16) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_DFS)) << 8;
            Tmp16 |= ((U16) HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_DFS));
            if ((Tmp16 & VID_DFS_CIF) != 0)
            {
                TotalInMacroBlock |= VID_DFS_CIF;
            }
            break;

        default :
            break;
    }
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_DFS, (U8) (TotalInMacroBlock >> 8));
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_DFS, (U8) TotalInMacroBlock);
#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    interrupt_unlock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */

    /* Same for MPEG1/MPEG2 */
    TmpPpr1 = ((Pic_p->picture_coding_type & 0x3) << VID_PPR1_PCT_SHIFT);
    TmpPpr2 = 0;

    /* Depending on MPEG1/MPEG2 */
    if (StreamInfo_p->MPEG1BitStream)
    {
        /* Setup for MPEG1 streams */
        TmpPfh = ((Pic_p->backward_f_code) << VID_PFH_BFH_SHIFT) | (Pic_p->forward_f_code);
        if (Pic_p->full_pel_backward_vector)
        {
            TmpPfh |= (VID_PFH_FULL_PEL_VECTOR << VID_PFH_BFH_SHIFT);
        }
        if (Pic_p->full_pel_forward_vector)
        {
            TmpPfh |= VID_PFH_FULL_PEL_VECTOR;
        }
        TmpPfv = 0;
    }
    else
    {
        /* Setup for MPEG2 streams */
        if (PicCodExt_p->intra_dc_precision != 0x3)
        {
            TmpPpr1 |= ((PicCodExt_p->intra_dc_precision) << 2);
        }
        TmpPpr1 |= PicCodExt_p->picture_structure;
#ifdef WA_PROBLEM_OVERWRITE_FIELDS
        ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->PreviousPictureStructure = PicCodExt_p->picture_structure;
#endif /* WA_PROBLEM_OVERWRITE_FIELDS */
        TmpPpr2 |= VID_PPR2_MP2;
        if (PicCodExt_p->top_field_first)
        {
            TmpPpr2 |= VID_PPR2_TFF;
        }
        if (PicCodExt_p->frame_pred_frame_dct)
        {
            TmpPpr2 |= VID_PPR2_FRM;
        }
        if (PicCodExt_p->concealment_motion_vectors)
        {
            TmpPpr2 |= VID_PPR2_CMV;
        }
        if (PicCodExt_p->q_scale_type)
        {
            TmpPpr2 |= VID_PPR2_QST;
        }
        if (PicCodExt_p->intra_vlc_format)
        {
            TmpPpr2 |= VID_PPR2_IVF;
        }
        if (PicCodExt_p->alternate_scan)
        {
            TmpPpr2 |= VID_PPR2_AZZ;
        }
        TmpPfh = ((PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL]) << VID_PFH_BFH_SHIFT) | (PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL]);
        TmpPfv = ((PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL  ]) << VID_PFV_BFV_SHIFT) | (PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL  ]);
    }

    /* Write configurations */
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_PPR1, TmpPpr1);
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_PPR2, TmpPpr2);
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_PFH,  TmpPfh);
    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_PFV,  TmpPfv);

    /* temporal reference: only B-on-the-fly, not ued for now */
/*    HALDEC_write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TRF16, ucVideoTrf);*/
} /* End of SetDecodeConfiguration() function */


/*******************************************************************************
Name        : SetDecodeLumaFrameBuffer
Description : Set decoder luma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    /* Beginning of the luma frame buffer where to decode, in unit of 2 Kbits */
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_RFP16, ((U32) BufferAddress_p) / 256);
} /* End of SetDecodeLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetDecodeChromaFrameBuffer
Description : Set decoder chroma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    /* Beginning of the chroma frame buffer where to decode, in unit of 2 Kbits */
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_RFC16 , ((U32) BufferAddress_p) / 256);
} /* End of SetDecodeChromaFrameBuffer() function */


/*******************************************************************************
Name        : SetForwardLumaFrameBuffer
Description : Set decoder forward reference luma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetForwardLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    /* Beginning of the luma frame buffer where to decode, in unit of 2 Kbits */
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_FFP16, ((U32) BufferAddress_p) / 256);
} /* End of SetForwardLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetForwardChromaFrameBuffer
Description : Set decoder forward reference chroma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetForwardChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    /* Beginning of the chroma frame buffer where to decode, in unit of 2 Kbits */
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_FFC16 , ((U32) BufferAddress_p) / 256);
} /* End of SetForwardChromaFrameBuffer() function */


/*******************************************************************************
Name        : SetFoundStartCode
Description : Informs the HAL of the startcode found. Store the SCD pointer
              after a start decode and until the slice is found.
Parameters  : HAL decode handle, StartCode found
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetFoundStartCode(const HALDEC_Handle_t HALDecodeHandle, const U8 StartCode, BOOL * const FirstPictureStartCodeFound_p)
{
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    *FirstPictureStartCodeFound_p = FALSE;

#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    Data_p->StartCodeFound = StartCode;

    if (!Data_p->HasGotSlice)
    {
       Data_p->PreviousSCAfterSliceSCD25bitValue = Data_p->PreviousSCD25bitValue;
    }
    if ((U8)StartCode == FIRST_SLICE_START_CODE)
    {
        Data_p->HasGotSlice = TRUE;
    }
#endif /* CHECK_SCD_PIPE_MISALIGNMENT */
} /* End of SetFoundStartCode() function */


/*******************************************************************************
Name        : SetInterruptMask
Description : Set decode interrupt mask
Parameters  : Decoder registers address, interrupt mask to apply
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetInterruptMask(const HALDEC_Handle_t HALDecodeHandle, const U32 Mask)
{
    U32 HWMask;

    HWMask = TranslateToHWStatus(HALDecodeHandle, Mask);

    HALDEC_Write8( (U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITM2, HWMask >> 16);
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITM16, (U16) HWMask | VID_ITX_VST | VID_ITX_VSB);
} /* End of SetInterruptMask() function */


/*******************************************************************************
Name        : SetMainDecodeCompression
Description : Set main decode compression
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetMainDecodeCompression(const HALDEC_Handle_t HALDecodeHandle, const STVID_CompressionLevel_t Compression)
{
    /* Feature not available on STi55xx */
} /* End of SetMainDecodeCompression() function */


/*******************************************************************************
Name        : SetSecondaryDecodeLumaFrameBuffer
Description : Set secondary decoder luma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryDecodeLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    /* Beginning of the luma frame buffer where to decode, in unit of 2 Kbits */
/*    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_RFP16, ((U32) BufferAddress_p) / 256);*/
    /* No secondary in STi55xx ! */
} /* End of SetSecondaryDecodeLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetSecondaryDecodeChromaFrameBuffer
Description : Set secondary decoder chroma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryDecodeChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    /* Beginning of the chroma frame buffer where to decode, in unit of 2 Kbits */
/*    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_RFC16 , ((U32) BufferAddress_p) / 256);*/
    /* No secondary in STi55xx ! */
} /* End of SetSecondaryDecodeChromaFrameBuffer() function */


/*******************************************************************************
Name        : SetSkipConfiguration
Description : Set configuration for skip. HAL needs to know which kind of
              picture is skipped. Because of hardware constraints with field
              pictures : same action for 2nd field than the 1st field.
Parameters  : HAL decode handle, Picture Structure information
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSkipConfiguration(const HALDEC_Handle_t HALDecodeHandle, const STVID_PictureStructure_t * const PictureStructure_p)
{
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* Hardware constraint : after skipping a 1st field, we must skip the 2nd field because it is not decodable. */
    if ((*PictureStructure_p) != STVID_PICTURE_STRUCTURE_FRAME)
    {
        if (Data_p->DecodeSkipConstraint == HALDEC_DECODE_SKIP_CONSTRAINT_MUST_SKIP)
        {
            Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_NONE;
        }
        else if (Data_p->DecodeSkipConstraint == HALDEC_DECODE_SKIP_CONSTRAINT_NONE)
        {
            Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_MUST_SKIP;
        }
    }
    else
    {
        Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_NONE;
    }

} /* End of SetSkipConfiguration() function */


/*******************************************************************************
Name        : SetSmoothBackwardConfiguration
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetSmoothBackwardConfiguration(const HALDEC_Handle_t HALDecodeHandle, const BOOL SetNotReset)
{
    U8 Stl8, Ldp8;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :

#if defined WA_PROBLEM_STL_CANNOT_BE_READ
            Stl8 = ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->STLValue;
#else /* WA_PROBLEM_STL_CANNOT_BE_READ */
            Stl8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STL);
#endif /* not WA_PROBLEM_STL_CANNOT_BE_READ */

            Ldp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_LDP);

            if ((((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType == STVID_DEVICE_TYPE_5516_MPEG) ||
                    (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType == STVID_DEVICE_TYPE_5517_MPEG))
            {
                Ldp8 &= (~VID_TP_LDP_SBS);
            }

            if (SetNotReset)
            {
                /* start code never launched automatically */
                Stl8 |= VID_STL_INR_SET;

                /* Enable smooth */
                Ldp8 |= VID_TP_LDP_TM_SET;
                ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->IsDecodingInTrickMode = TRUE;
            }
            else
            {
                /* start code launched automatically */
                Stl8 &= (~VID_STL_INR_SET);

                /* Disable smooth */
                Ldp8 &= (~VID_TP_LDP_TM_SET);
                ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->IsDecodingInTrickMode = FALSE;
            }

            /* STL */
            HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STL, Stl8);
#if defined WA_PROBLEM_STL_CANNOT_BE_READ
            ((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->STLValue = Stl8;
#endif /* WA_PROBLEM_STL_CANNOT_BE_READ */

            /* LDP */
            HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TP_LDP, Ldp8);

            break;

        default :
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }

    return(ErrorCode);
} /* End of SetSmoothBackwardConfiguration() function */


/*******************************************************************************
Name        : SetStreamID
Description : Set stream ID for the PES parser
Parameters  : HAL decode handle, stream type
Assumptions :
Limitations : Consider only for LSB of StreamID
Returns     : Nothing
*******************************************************************************/
static void SetStreamID(const HALDEC_Handle_t HALDecodeHandle, const U8 StreamID)
{
    U8 cf2;

    cf2 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_CF2);

    if (StreamID == STVID_IGNORE_ID)
    {
        cf2 |= PES_CF2_IVI;
    }
    else
    {
        /* Enable Video ID selection */
        cf2 &= ~PES_CF2_IVI;
        /* Set the stream ID */
        cf2 = (cf2 & PES_CF2_VID_ID_MASK) | (StreamID & ~PES_CF2_VID_ID_MASK);
    }

    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_CF2, cf2);
} /* End of SetStreamID() function */


/*******************************************************************************
Name        : SetStreamType
Description : Set stream type for the PES parser
Parameters  : HAL decode handle, stream type
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetStreamType(const HALDEC_Handle_t HALDecodeHandle, const STVID_StreamType_t StreamType)
{
    U8 cf2;

    cf2 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_CF2);

    switch(StreamType)
    {
        case STVID_STREAM_TYPE_ES :
            /* SS = 0, MOD = 0 */
            cf2 = (cf2 & PES_CF2_MOD_MASK & ~PES_CF2_SS) | (PES_CF2_MOD_00);
            break;

        case STVID_STREAM_TYPE_PES :
            /* SS = 1, MOD = 2 */
            cf2 = (cf2 & PES_CF2_MOD_MASK) | PES_CF2_SS | (PES_CF2_MOD_10);
            break;

        case STVID_STREAM_TYPE_MPEG1_PACKET :
            /* SS = 1, MOD = 1 */
            cf2 = (cf2 & PES_CF2_MOD_MASK) | PES_CF2_SS | (PES_CF2_MOD_01);
            break;

/*        case PROGRAM : !!! NOT USED */
            /* SS = 1, MOD = 3 */
/*            cf2 = (cf2 & PES_CF2_MOD_MASK) | PES_CF2_SS | (PES_CF2_MOD_11);*/
/*            break;*/

        default :
            /* Nothing to do */
            break;
    }

    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + PES_CF2, cf2);
} /* End of SetStreamType() function */

/*******************************************************************************
Name        : Setup
Description :
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Setup(const HALDEC_Handle_t     HALDecodeHandle,
                            const STVID_SetupParams_t * const SetupParams_p)
{
    /* Not implemented in this HAL */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
} /* End of Setup() function */


/*******************************************************************************
Name        : SkipPicture
Description : Skip one or more picture in incoming data
              This doesn't launch start code detector !
Parameters  : HAL decode handle, skip mode (number of pictures to skip, what to do after),
              flag FALSE to execute now and TRUE to execute only on next VSync
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SkipPicture(const HALDEC_Handle_t HALDecodeHandle, const BOOL ExecuteOnlyOnNextVSync, const HALDEC_SkipPictureMode_t SkipMode)
{
    U8 Tmp8 = VID_TIS_EXE;


#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    Omega1Properties_t * Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    Data_p->HadPreviousSCAfterSlice = TRUE;
    Data_p->IsDecoderIdle = FALSE;
    Data_p->HasGotSlice = FALSE;
    Data_p->PreviousSCAfterSliceSCD25bitValue = Data_p->PreviousSCD25bitValue;
#endif /* CHECK_SCD_PIPE_MISALIGNMENT */

    if (!(ExecuteOnlyOnNextVSync))
    {
        Tmp8 |= VID_TIS_FIS;
    }

    switch (SkipMode)
    {
        case HALDEC_SKIP_PICTURE_1_THEN_DECODE :
            Tmp8 |= SKIP_ONE_DECODE_NEXT;
            break;

        case HALDEC_SKIP_PICTURE_2_THEN_DECODE :
            Tmp8 |= SKIP_TWO_DECODE_NEXT;
            break;

        case HALDEC_SKIP_PICTURE_1_THEN_STOP :
            Tmp8 |= SKIP_ONE_STOP_DECODE;
            break;

        default :
            /* Bad parameter: skip one and stop */
            Tmp8 |= SKIP_ONE_STOP_DECODE;
            break;
    }

    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TIS, Tmp8);
} /* End of SkipPicture() function */


/*******************************************************************************
Name        : StartDecodePicture
Description : Launch decode of a picture
              If not in trick modes, also launches start code detector !
Parameters  : HAL decode handle, flag FALSE to execute now and TRUE to execute only on next VSync
              flag telling whether the decode takes place on the same picture that is on display
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void StartDecodePicture(const HALDEC_Handle_t HALDecodeHandle, const BOOL ExecuteOnlyOnNextVSync, const BOOL MainDecodeOverWrittingDisplay, const BOOL SecondaryDecodeOverWrittingDisplay)
{
    U8 Tmp8 = VID_TIS_EXE;
    Omega1Properties_t * Data_p;

    Data_p = (Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

#ifdef CHECK_SCD_PIPE_MISALIGNMENT
    Data_p->HadPreviousSCAfterSlice = TRUE;
    Data_p->IsDecoderIdle = FALSE;
    Data_p->HasGotSlice = FALSE;
    Data_p->PreviousSCAfterSliceSCD25bitValue = Data_p->PreviousSCD25bitValue;
#endif /* CHECK_SCD_PIPE_MISALIGNMENT */

#if defined WA_PROBLEM_SCH_MISSING_RANDOMLY
    if ((!(((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->IsDecodingInTrickMode)) ||
#if defined WA_PROBLEM_STL_CANNOT_BE_READ
        ((((Omega1Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p)->STLValue & VID_STL_INR_SET) == 0))
#else /* WA_PROBLEM_STL_CANNOT_BE_READ */
        ((HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_STL) & VID_STL_INR_SET) == 0))
#endif /* not WA_PROBLEM_STL_CANNOT_BE_READ */
    {
        /* Not in trick modes or SC detector launched automatically: SC detector no more idle on launch of decode */
        Data_p->IsStartCodeDetectorIdle = FALSE;
        Data_p->IsStartCodeExpectedAfterLaunchByDecode = TRUE;
    }
#endif /* WA_PROBLEM_SCH_MISSING_RANDOMLY */

    /* If frame where to decode is the currently displayed one: set overwrite bit */
    if (MainDecodeOverWrittingDisplay)
    {
#ifdef WA_PROBLEM_OVERWRITE_FIELDS
        if (Data_p->PreviousPictureStructure == PICTURE_STRUCTURE_FRAME_PICTURE)
        {
            /* Only set overwrite bit if frame picture, not if field picture ! */
            Tmp8 |= VID_TIS_OVW;
        }
#else /* WA_PROBLEM_OVERWRITE_FIELDS */
        Tmp8 |= VID_TIS_OVW;
#endif /* not WA_PROBLEM_OVERWRITE_FIELDS */
    }

    /* Hardware constraint : must decode after a reset. */
    if (Data_p->FirstDecodeAfterSoftResetDone == FALSE)
    {
        Data_p->FirstDecodeAfterSoftResetDone = TRUE;
    }

    if (!(ExecuteOnlyOnNextVSync))
    {
        Tmp8 |= VID_TIS_FIS;
    }

    HALDEC_Write8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_TIS, Tmp8);
} /* End of StartDecodePicture() function */


/*******************************************************************************
Name        : Term
Description : Actions to do on chip when terminating
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : TRUE is empty, FALSE if not empty
*******************************************************************************/
static void Term(const HALDEC_Handle_t HALDecodeHandle)
{
    U8 Tmp8;

    /* Disable all interrupts */
    HALDEC_Write8( (U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITM2, 0);
    HALDEC_Write16((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITM16, 0);
    /* Clear pending interrupts. Most significant byte must be read last, because it clears the register */
    Tmp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS2);
    Tmp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS0);
    Tmp8 = HALDEC_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VID_ITS1);

    SAFE_MEMORY_DEALLOCATE(((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p,
                           ((HALDEC_Properties_t *) HALDecodeHandle)->CPUPartition_p,
                           sizeof(Omega1Properties_t));
} /* End of Term() function */

/*******************************************************************************
Name        : WriteStartCode
Description : Writes a new startcode in the bitbuffer
Parameters  : HAL Decode manager handle, startcode value, startcode address
Assumptions : Limitations :
Returns     :
*******************************************************************************/
static void WriteStartCode(
                    const HALDEC_Handle_t  HALDecodeHandle,
                    const U32             SCVal,
                    const void * const    SCAdd_p)
{
    U8*  Temp8;

    /* Write the startcode in the bitbuffer */
    Temp8 = (U8*) SCAdd_p;
    STSYS_WriteRegMemUncached8(Temp8    , 0x0);
    STSYS_WriteRegMemUncached8(Temp8 + 1, 0x0);
    STSYS_WriteRegMemUncached8(Temp8 + 2, 0x1);
    STSYS_WriteRegMemUncached8(Temp8 + 3, SCVal);
} /* End of WriteStartCode() function */

#ifdef STVID_DEBUG_GET_STATUS
/*******************************************************************************
Name        : GetDebugStatus
Description : Set the Debug Status related to the decoder
Parameters  : HAL Decode manager handle, status address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void GetDebugStatus(const HALDEC_Handle_t        HALDecodeHandle,
                           STVID_Status_t* const        Status_p)
{
	/* Get Injector Status */

	/* TBD */

} /* End of SetDecodeBitBuffer() function */
#endif /* STVID_DEBUG_GET_STATUS */
/* End of hv_dec1.c */
