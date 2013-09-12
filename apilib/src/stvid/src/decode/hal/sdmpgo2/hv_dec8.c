/*******************************************************************************

File name   : hv_dec8.c

Description : HAL video decode SDMpegO2 IP source file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
24 fev 2003        Created                                          PLe
*******************************************************************************/


/* Private preliminary definitions (internal use only) ---------------------- */

/* #define WA_GNBvd24093 */  /* Motion vector check feature (MVC) corrupts correct field pictures */

#define WA_GNBvd27888  /* DecodeTimeOut error (dual-dec sometimes frozen) */
/* #define MULTI_DECODE_SEMAPHORE */ /* Issue 571 / Gnbvd27888. solved if VLD access protected by semaphore. */

/* Define to add debug info and traces */
/* #define TRACE_UART */

#ifdef TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "vid_com.h"
#include "vid_ctcm.h"

#include "hv_dec8.h"
#include "hvr_dec8.h"

#ifdef STVID_STVAPI_ARCHITECTURE
#include "hv_mb2r.h"
#endif

#include "stsys.h"

typedef enum Xdmpgo2_DecoderId_s {
    SDMPGO2_DECODER,
    HDMPGO2_DECODER_0,
    HDMPGO2_DECODER_1
} Xdmpgo2_DecoderId_t;

typedef struct Sdmpgo2Properties_s {
    HALDEC_DecodeSkipConstraint_t DecodeSkipConstraint;
    void * LinearBitBufferStartAddress_p;
    semaphore_t * PESParserAccessSemaphore_p;
    BOOL PESParserIsEnabled;
    BOOL FirstPictureSCAfterSofwareResetFound;
    /* stream pictures properties */
    void * LastPictureFoundAddress_p;
    void * LastButOnePictureFoundAddress_p;
    /* bit buffer properties */
    void * BitBufferBaseAddress_p;
    U32 BitBufferSize;
    U32 LastBitBufferOutput24BitsValue;
    U32 BitBufferOutput32BitsValue;
    /* Start code properties */
    void * LastFoundStartCodeAddress_p;
    U8 LastFoundStartCode;
    BOOL FirstSearchFollowingFirstSequence;
    /* VLD Properties */
    BOOL IsDecodingInTrickMode;
    /* interrupts properties */
    U32  MissingHWStatus;
    /* PBO */
    BOOL IsPBOenabled;
#ifdef MULTI_DECODE_SEMAPHORE
    BOOL OneDecodeIsOnGoing;
#endif
    /* enum identifing the decoder */
    Xdmpgo2_DecoderId_t  DecoderId;
#ifdef STVID_STVAPI_ARCHITECTURE
    void * MB2RBaseAddress_p;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
} Sdmpgo2Properties_t;

/* Private Constants -------------------------------------------------------- */

/* Enable/Disable */
#define HALDEC_DISABLE  0
#define HALDEC_ENABLE   1

#define CFG_BASE_OFFSET_DECODER     0x00000300
#define MB2R_BASE_ADDRESS_OFFSET    0x00080000

/* Private Variables (static)------------------------------------------------ */
#ifdef MULTI_DECODE_SEMAPHORE
static semaphore_t * MultiDecodeSemaphore_p;
static BOOL MultiDecodeSemaphoreInitialised = FALSE;
static U8 InitialisedInstanceNb = 0;
#endif

/* Private Macros ----------------------------------------------------------- */
#define HAL_Read16(Address_p)    STSYS_ReadRegDev32LE((void *) (Address_p))
#define HAL_Read32(Address_p)    STSYS_ReadRegDev32LE((void *) (Address_p))
#define HAL_Write16(Address_p, Value) STSYS_WriteRegDev16LE((void *) (Address_p), (Value))
#define HAL_Write32(Address_p, Value) STSYS_WriteRegDev32LE((void *) (Address_p), (Value))

#define SwapBytesIntoLong(a,b) \
((a) = ((((b) & 0x000000FF)  << 24) | (((b) & 0x0000FF00)  << 8) | (((b) & 0x00FF0000) >> 8) | (((b) & 0xFF000000) >> 24)))

/* Private Function prototypes ---------------------------------------------- */

#ifdef STVID_STVAPI_ARCHITECTURE
/* ----------------------------------------------------------------------
 * MacroBlock reconstruction
   - primary
     * I/P picture
       - must be enabled : VIDN_DMOD.EN = 1 / Reconst_cfg.Main_MB = 1
     * B picture
       - must be disabled : VIDN_DMOD.EN = x / Reconst_cfg.Main_MB = 0
                                           |
                                           '-> 1 if raster reconst is required
                                               0 or 1 if raster reconst is not required
                                               ==> 1 covers all the cases
    - secondary
      * I/P/B picture
        - not possible.

 * raster reconstruction
    - primary
      * I/P/B picture
        - enable : VIDN_DMOD.EN = 1 / Reconst_cfg.Main_Raster = 1
        - disable : VIDN_DMOD.EN = x / Reconst_cfg.Main_Raster = 0
                                   |
                                   '-> 1 if MB reconst is required
                                       0 or 1 if MB reconst is not required
                                       ==> 1 covers all the cases
    - secondary
      * I/P/B picture
        - enable : VIDN_SDMOD.EN = 1
        - disable : VIDN_SDMOD.EN = 0
------------------------------------------------------------------------*/
static void MacroBlockToRasterInit(const Sdmpgo2Properties_t * Data_p);
static void MacroBlockToRasterTerm();
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

/* Functions in HALDEC_FunctionsTable_t */
#ifdef STVID_STVAPI_ARCHITECTURE
static void EnablePrimaryRasterReconstruction(const HALDEC_Handle_t HALDecodeHandle);
static void DisablePrimaryRasterReconstruction(const HALDEC_Handle_t HALDecodeHandle);
static void EnableSecondaryRasterReconstruction(const HALDEC_Handle_t HALDecodeHandle, const STVID_DecimationFactor_t HDecimation, const STVID_DecimationFactor_t VDecimation, const STVID_ScanType_t ScanType);
static void DisableSecondaryRasterReconstruction(const HALDEC_Handle_t HALDecodeHandle);
static U32 GetMacroBlockToRasterStatus(const HALDEC_Handle_t HALDecodeHandle);
static void SetPrimaryRasterReconstructionLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p, const U32 Pitch);
static void SetPrimaryRasterReconstructionChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
static void SetSecondaryRasterReconstructionLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p, const U32 Pitch);
static void SetSecondaryRasterReconstructionChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
static void DecodingSoftReset(const HALDEC_Handle_t HALDecodeHandle, const BOOL IsRealTime);
static void DisableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle);
static void EnableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle, const STVID_DecimationFactor_t HDecimation, const STVID_DecimationFactor_t VDecimation, const STVID_ScanType_t ScanType);
static void DisablePrimaryDecode(const HALDEC_Handle_t HALDecodeHandle);
static void EnablePrimaryDecode(const HALDEC_Handle_t HALDecodeHandle);
static void DisableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle, const STVID_HDPIPParams_t * const HDPIPParams_p);
static void EnableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle, const STVID_HDPIPParams_t * const HDPIPParams_p);
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
static ST_ErrorCode_t GetLinearBitBufferTransferedDataSize(const HALDEC_Handle_t HALDecodeHandle, U32 * const LinearBitBufferTransferedDataSize_p);
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
static ST_ErrorCode_t SetDataInputInterface(
           const HALDEC_Handle_t HALDecodeHandle,
           ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle,
                                            void ** const Address_p),
           void (*InformReadAddress)(void * const Handle, void * const Address),
           void * const Handle );
static void SetDecodeBitBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p, const U32 BufferSize, const HALDEC_BitBufferType_t BufferType);
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
#ifdef ST_SecondInstanceSharingTheSameDecoder
static void StartDecodeStillPicture(const HALDEC_Handle_t HALDecodeHandle, const void * BitBufferAddress_p, const VIDBUFF_PictureBuffer_t * DecodedPicture_p, const StreamInfoForDecode_t * StreamInfo_p);
#endif /* ST_SecondInstanceSharingTheSameDecoder */
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
const HALDEC_FunctionsTable_t HALDEC_Sdmpgo2Functions = {
#ifdef STVID_STVAPI_ARCHITECTURE
    EnablePrimaryRasterReconstruction,
    DisablePrimaryRasterReconstruction,
    EnableSecondaryRasterReconstruction,
    DisableSecondaryRasterReconstruction,
    GetMacroBlockToRasterStatus,
    SetPrimaryRasterReconstructionLumaFrameBuffer,
    SetPrimaryRasterReconstructionChromaFrameBuffer,
    SetSecondaryRasterReconstructionLumaFrameBuffer,
    SetSecondaryRasterReconstructionChromaFrameBuffer,
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
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
#ifdef ST_SecondInstanceSharingTheSameDecoder
    StartDecodeStillPicture,
#endif /* ST_SecondInstanceSharingTheSameDecoder */
    Term,
#ifdef STVID_DEBUG_GET_STATUS
	GetDebugStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    WriteStartCode
};

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : TranslateToDecodeStatus
Description : Translate HW status to HAL decode status (SDMPGO2 registers to HAL flags)
Parameters  : HW status of SDMPGO2 chip
Assumptions :
Limitations :
Returns     : HAL decode status
*******************************************************************************/
static U32 TranslateToDecodeStatus(const HALDEC_Handle_t HALDecodeHandle, const U32 HWStatus)
{
    U32 Status = 0;

    /* HFE, HFF, SCH and DOE flags are placed as Omega2 VID_STA register */
    Status |= HWStatus & (VIDn_ITX_HFE | VIDn_ITX_HFF | VIDn_ITX_SCH | VIDn_ITX_DOE);

    /* all the others flags have to be translated */
    /* if (HWStatus & VIDn_ITX_SRR) */
    /* { */
        /* Status |= HADEC_; */
    /* } */
    /* if (HWStatus & VIDn_ITX_R_OPC) */
    /* { */
        /* Status |= HADEC_; */
    /* } */
    /* if (HWStatus & VIDn_ITX_VLDRL) */
    /* { */
        /* Status |= HADEC_; */
    /* } */
    /* if (HWStatus & VIDn_ITX_SCDRL) */
    /* { */
        /* Status |= HADEC_; */
    /* } */
    /* if (HWStatus & VIDn_ITX_CDWL) */
    /* { */
        /* Status |= HADEC_; */
    /* } */
    if (HWStatus & VIDn_ITX_SBE)
    {
        Status |= HALDEC_START_CODE_DETECTOR_BIT_BUFFER_EMPTY_MASK;
    }
    if (HWStatus & VIDn_ITX_BBE)
    {
        Status |= HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK;
    }
    /* if (HWStatus & VIDn_ITX_BBNE) */
    /* { */
        /* Status |= HADEC_; */
    /* } */
    if (HWStatus & VIDn_ITX_BBF)
    {
        Status |= HALDEC_BIT_BUFFER_NEARLY_FULL_MASK;
    }
    /* if (HWStatus & VIDn_ITX_PAN) */
    /* { */
        /* Status |= HADEC_; */
    /* } */
    if (HWStatus & VIDn_ITX_DSE)
    {
        Status |= HALDEC_DECODING_SYNTAX_ERROR_MASK;
    }
    /* if (HWStatus & VIDn_ITX_MSE) */
    /* { */
        /* Status |= HADEC_; */
    /* } */
    if (HWStatus & VIDn_ITX_DUE)
    {
        Status |= HALDEC_DECODING_UNDERFLOW_MASK;
    }
    if (HWStatus & VIDn_ITX_OTE)
    {
        Status |= HALDEC_OVERTIME_ERROR_MASK;
    }
    if (HWStatus & VIDn_ITX_DID)
    {
        /* Accroding to STi5528 data sheet DID=1 implies PID=1 */
        Status |= HALDEC_DECODER_IDLE_MASK;
        Status |= HALDEC_PIPELINE_IDLE_MASK;
    }

    if (HWStatus & VIDn_ITX_PSD)
    {
        Status |= HALDEC_PIPELINE_START_DECODING_MASK;
    }
    if (HWStatus & VIDn_ITX_BFF)
    {
        Status |= HALDEC_BITSTREAM_FIFO_FULL_MASK;
    }
    if (HWStatus & VIDn_ITX_IER)
    {
        Status |= HALDEC_INCONSISTENCY_ERROR_MASK;
    }

    return(Status);
} /* End of TranslateToDecodeStatus() function */


/*******************************************************************************
Name        : TranslateToHWStatus
Description : Translate HAL decode status to HW status (HAL flags to SDMPGO2 registers)
Parameters  : HAL decode status
Assumptions :
Limitations :
Returns     : HW status
*******************************************************************************/
static U32 TranslateToHWStatus(const HALDEC_Handle_t HALDecodeHandle, const U32 Status)
{
    U32 HWStatus = 0;

    /* HFE, HFF, SCH and DOE flags are placed as Omega2 VID_STA register */
    HWStatus |= Status & (HALDEC_DECODING_OVERFLOW_MASK | HALDEC_HEADER_FIFO_EMPTY_MASK | HALDEC_HEADER_FIFO_NEARLY_FULL_MASK |
            HALDEC_START_CODE_HIT_MASK);

    /* all the others flags have to be translated */
    if (Status & HALDEC_START_CODE_DETECTOR_BIT_BUFFER_EMPTY_MASK)
    {
        HWStatus |= VIDn_ITX_SBE;
    }
    if (Status & HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK)
    {
        HWStatus |= VIDn_ITX_BBE;
    }
    if (Status & HALDEC_BIT_BUFFER_NEARLY_FULL_MASK)
    {
        HWStatus |= VIDn_ITX_BBF;
    }
    if (Status & HALDEC_DECODING_SYNTAX_ERROR_MASK)
    {
        HWStatus |= VIDn_ITX_DSE;
    }
    if (Status & HALDEC_DECODING_UNDERFLOW_MASK)
    {
        HWStatus |= VIDn_ITX_DUE;
    }
    if (Status & HALDEC_OVERTIME_ERROR_MASK)
    {
        HWStatus |= VIDn_ITX_OTE;
    }
    if (Status & HALDEC_DECODER_IDLE_MASK)
    {
        HWStatus |= VIDn_ITX_DID;
    }
    if (Status & HALDEC_PIPELINE_IDLE_MASK)
    {
        HWStatus |= VIDn_ITX_PID;
    }
    if (Status & HALDEC_PIPELINE_START_DECODING_MASK)
    {
        HWStatus |= VIDn_ITX_PSD;
    }
    if (Status & HALDEC_BITSTREAM_FIFO_FULL_MASK)
    {
        HWStatus |= VIDn_ITX_BFF;
    }
    if (Status & HALDEC_INCONSISTENCY_ERROR_MASK)
    {
        HWStatus |= VIDn_ITX_IER;
    }

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
    Sdmpgo2Properties_t * Data_p;
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

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
    U32 Tmp32, Scd32;
    Sdmpgo2Properties_t * Data_p;
    /* U32 Cdb32 = 0; */
    /* U32 Scdb32 = 0; */
    U32 Hlnc32 = 0;

    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

 #ifdef TRACE_UART
    TraceBuffer(("Soft Reset \r\n"));
#endif

    /* initialise HAL sdmpego2 private datas */
    Data_p->LastPictureFoundAddress_p = NULL;
    Data_p->LastButOnePictureFoundAddress_p = NULL;
    Data_p->MissingHWStatus = 0;
    Data_p->FirstPictureSCAfterSofwareResetFound = FALSE;
    Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_NONE;
    Data_p->BitBufferOutput32BitsValue = 0;
    /* preparing first start code sequence search */
    Data_p->FirstSearchFollowingFirstSequence = TRUE;

    /* Reset PES parser. Otherwise PTS association in PES data will be corrupted. */
    ResetPESParser(HALDecodeHandle);

    /* Scd has to find first a sequence start code : Enable sequence search, disable the others */
    Scd32 = (VIDn_SCD_MASK) & (~VIDn_SCD_SHC_DISCARD_EN);
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCD, Scd32);

    /* Video Soft Reset */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SRS, VIDn_SRS_SOFT_RESET);

#ifdef MULTI_DECODE_SEMAPHORE
    if (Data_p->OneDecodeIsOnGoing)
    {
        Data_p->OneDecodeIsOnGoing = FALSE;
        semaphore_signal(MultiDecodeSemaphore_p);
    }
#endif /* MULTI_DECODE_SEMAPHORE */

    /* Clear interrupts for events that occured prior to the SoftReset. Do that before first exe, not to loose any IT. */
    Tmp32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_ITS);

    /* Seek sequence by launching start code detector */
    if (!Data_p->IsDecodingInTrickMode)
    {
        Scd32 = 0x7FF;
        Scd32 &= (~VIDn_SCD_SHC_DISCARD_EN);
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCD, Scd32);

        /* RTA does not need to be written because of the normally start code search */
        Hlnc32 = VIDn_HLNC_LNC_NORMALLY_LAUNCHED;
        /* SCNT (not always mentionned in datasheets) : must be 0. number of bytes read by start code detector */
        /* will be in VIDn_SCDC */
        Hlnc32 &= (~VIDn_HLNC_SCNT_TS_ASS_FROZEN);
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HLNC, Hlnc32);
    }
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
    /* if macro block to raster, the secondary decode is not possible */
    /* SDMOD.EN has to be accessed only by the raster reconstruction functions */
#ifndef STVID_STVAPI_ARCHITECTURE
    /* disabling secondary decode makes all previous other fields of the register unuseful : register can be set to 0. */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD, 0);
#endif /* end of ndef STVID_STVAPI_ARCHITECTURE */

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
static void EnableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle, const STVID_DecimationFactor_t H_Decimation, const STVID_DecimationFactor_t V_Decimation, const STVID_ScanType_t ScanType)
{
    /* if macro block to raster, the secondary decode is not possible */
    /* SDMOD.EN has to be accessed only by the raster reconstruction functions */

#ifndef STVID_STVAPI_ARCHITECTURE
    U32 SDmod32 = 0;

    switch (H_Decimation)
    {
        case STVID_DECIMATION_FACTOR_H2 :
            SDmod32 |= VIDn_SDMOD_HDEC_H2;
            break;

        case STVID_DECIMATION_FACTOR_H4 :
            SDmod32 |= VIDn_SDMOD_HDEC_H4;
            break;

        default :
            /* VIDn_SDMOD_HDEC_NONE is 0 : nothing to do */
            /* SDmod32 |= VIDn_SDMOD_HDEC_NONE; */
            break;
    }
    switch (V_Decimation)
    {
        case STVID_DECIMATION_FACTOR_V2 :
            SDmod32 |= VIDn_SDMOD_VDEC_V2;
            break;

        case STVID_DECIMATION_FACTOR_V4 :
            SDmod32 |= VIDn_SDMOD_VDEC_V4;
            break;

        default :
            /* VIDn_SDMOD_VDEC_NONE is 0 : nothing to do */
            /* SDmod32 |= VIDn_SDMOD_VDEC_NONE; */
            break;
    }

    if (ScanType == STVID_SCAN_TYPE_PROGRESSIVE)
    {
        SDmod32 |= VIDn_SDMOD_PS_EN;
    }
    else
    {
        /* nothing to do, bit already set to 0 above */
    }

    /* enable decode decimation */
    SDmod32 |= VIDn_SDMOD_2nd_REC_EN;

    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD, SDmod32);

#endif /* end of ndef STVID_STVAPI_ARCHITECTURE */

} /* End of EnableSecondaryDecode() function */

/*******************************************************************************
Description : Enables Primary Decode
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnablePrimaryDecode(const HALDEC_Handle_t       HALDecodeHandle)
{
    U32 Dmod32;

#ifdef STVID_STVAPI_ARCHITECTURE
    U32 Reconst_cfg32;
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    Reconst_cfg32 = HAL_Read32((U8 *) (Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CFG);
    Reconst_cfg32 |= MB2R_RECONST_CFG_MAIN_MB;
    HAL_Write32((U8 *) (Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CFG, Reconst_cfg32);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

    Dmod32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD);
    Dmod32 |= VIDn_DMOD_PRI_REC_EN;

    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD, Dmod32);

} /* End of EnableSecondaryDecode() function */

/*******************************************************************************
Name        : DisableHDPIPDecode
Description : Disables HDPIP decode.
Parameters  : HAL Decode manager handle, HDPIP parameters
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableHDPIPDecode(const HALDEC_Handle_t HALDecodeHandle, const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    /* Feature not available on SDMPGO2 */
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
static void EnableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle, const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    /* Feature not available on SDMPGO2 */
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
#ifdef STVID_STVAPI_ARCHITECTURE
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;
    U32 Reconst_cfg32;

    /* MB2R ==> DMOD.EN must not be disabled : let it in its current state ! */
    /* MB2R register : disable Reconst_cfg.Main_MB */
    Reconst_cfg32 = HAL_Read32((U8 *) (Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CFG);
    Reconst_cfg32 &= (~MB2R_RECONST_CFG_MAIN_MB);
    HAL_Write32((U8 *) (Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CFG, Reconst_cfg32);

#else /* end of def STVID_STVAPI_ARCHITECTURE */
    U32 Dmod32;

    Dmod32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD);
    Dmod32 &= (~VIDn_DMOD_PRI_REC_EN);
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD, Dmod32);
#endif /* end of ndef STVID_STVAPI_ARCHITECTURE */


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
    U32 Tmp32, PesCfg32 = 0, i;
    Sdmpgo2Properties_t * Data_p;

    /* The video input CD fifo is 128 bytes + 4 more bytes due to PES parser */
    #define FLUSH_SIZE                           256

    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* Protect PES parser access by semaphore */
    semaphore_wait(Data_p->PESParserAccessSemaphore_p);

    /* Disable PES Parser */
    if (Data_p->PESParserIsEnabled)
    {
        PesCfg32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG);
        PesCfg32 &= (~VIDn_PES_CFG_PARSER_EN);
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG, PesCfg32);
    }

    /* flush CD fifo with 256 bytes (= discontinuity start code + stuffing) */
    Tmp32 = (0x00) | (0x00 << 8) | (0x01 << 16) | ((U32)DISCONTINUITY_START_CODE << 24);
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->CompressedDataInputBaseAddress_p) + VIDn_CDI, Tmp32);
    for (i=0; i< (FLUSH_SIZE - 4) ; i+=4)
    {
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->CompressedDataInputBaseAddress_p) + VIDn_CDI, 0xFFFFFFFF);
    }

    if (Data_p->PESParserIsEnabled)
    {
        PesCfg32 |= VIDn_PES_CFG_PARSER_EN;
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG, PesCfg32);
    }

    /* Un Protect PES parser access */
    semaphore_signal(Data_p->PESParserAccessSemaphore_p);

    *DiscontinuityStartCodeInserted_p = TRUE;

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
    U32 Hdf32;

    Hdf32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF);

    return((U16)(Hdf32 & VIDn_HDF_HEADER_DATA_MASK));

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
    U32 Hdf32;

    Hdf32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF);

    *StartCodeOnMSB_p = (BOOL)((Hdf32 & VIDn_HDF_SCM_SC_on_MSB) >> VIDn_HDF_SCM_SC_on_MSB_SHIFT);

    return((U16)(Hdf32 & VIDn_HDF_HEADER_DATA_MASK));

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
    U32 HeaderData = 0;
    U32 Hdf32;

    /* Header FIFO is not empty: get data */
    /* Lock for security because 2 reads in cycle register must be done together */
#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    STOS_InterruptLock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */

    Hdf32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF);
    HeaderData = (Hdf32 << 16) & 0xFFFF0000;

    Hdf32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF);
    HeaderData |= (Hdf32 & 0x0000FFFF);

#ifdef HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS
    STOS_InterruptUnlock();
#endif /* HALDEC_LOCK_ACCESS_TO_CYCLIC_REGISTERS */

    return(HeaderData);
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
    /* Sdmpgo2Properties_t * Data_p; */
    /* Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p; */

    return(0);
} /* End of GetAbnormallyMissingInterruptStatus() function */

/*******************************************************************************
Name        : GetDataInputBufferParams
Description : not used in 5528 where injection is not controlled by video
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
} /* End of GetDataInputBufferParams() function. */

/*******************************************************************************
Name        : GetBitBufferOutputCounter
Description : Gets the number of bytes of data which have gone through the SC detector
              since the init of the HAL.
Parameters  : HAL decode handle
Assumptions :
Limitations : Last value compared to take care of 24bit register value.
              Be careful with loops to 0 after 0xffffff ! (user should remember last
              value and test: if last value >= current value there was a loop to 0)
Returns     : 32bit word OutputCounter value in units of bytes
*******************************************************************************/
static U32 GetBitBufferOutputCounter(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 Counter = 0;
    U32 DiffPSC;
    Sdmpgo2Properties_t * Data_p;

    Data_p  = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;
    Counter = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCDC);

    /* Apply the mask. Although bit 0 is reserved, there is no need to shift to the left(divide by 2)
     * because "This register holds the number of 16-bit words processed by the start code detector" */
    Counter = Counter & VIDn_SCDC_MASK;

    /* Compute the difference between the last register read value and the current.
     * We must also manage the case of a loop in the counter. */
    if(Counter < Data_p->LastBitBufferOutput24BitsValue)
    {
        /* Loop has occured in counter */
        DiffPSC = (2 << VIDn_SCDC_BITs) - Data_p->LastBitBufferOutput24BitsValue + Counter;
    }
    else
    {
        DiffPSC = Counter - Data_p->LastBitBufferOutput24BitsValue;
    }

    /* Compute the bitbuffer output value */
    Data_p->BitBufferOutput32BitsValue += DiffPSC;
    /* Recording register value for next time */
    Data_p->LastBitBufferOutput24BitsValue = Counter;

    return(Data_p->BitBufferOutput32BitsValue);

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
    /* VIDn_VLDPTR has to be set with a Memory address pointer, in byte ==> CDFifoAlignment = 1 */
    *CDFifoAlignment_p = 1;

    /* temp : trickmode debug only. */
    *CDFifoAlignment_p = 256;

    return(ST_NO_ERROR);
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
    U32 BitBufferLevel = 0;

    /* The bit buffer level indicates a coherent value only when all processes are working in the same */
    /* bit buffer (i) circular bit buffer between BBG and BBS or (ii) linear bit buffer when the size of the */
    /* linear bit buffer is lower than the size of the bit buffer defines by BBG/BBS. Indeed, BBL is */
    /* computed modulus BBG-BBS size. */
    BitBufferLevel = (HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL));
#ifdef TRACE_UART
    /* TraceBuffer(("BBL=%d\r\n", BitBufferLevel)); */
#endif

    return(BitBufferLevel);

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
    /* Not yet implemented on sdmpgo2. Just return no error. */
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
    U32 HWStatus, Mask, Status;
    Sdmpgo2Properties_t * Data_p;

    /* retrieve private datas */
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* retrieve mask */
    Mask = (HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_ITM));

    /* translate it into HAL-indepedent */
    Mask = TranslateToDecodeStatus(HALDecodeHandle, Mask);

    /* Prevent from loosing interrupts ! */
    SetInterruptMask(HALDecodeHandle, 0);

    HWStatus = (HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_ITS));

    /* Release video interrupts ! */
    SetInterruptMask(HALDecodeHandle, Mask);

#ifdef MULTI_DECODE_SEMAPHORE
    if (Data_p->OneDecodeIsOnGoing)
    {
        if ((HWStatus & VIDn_ITX_DID) != 0)
        {
            Data_p->OneDecodeIsOnGoing = FALSE;
            semaphore_signal(MultiDecodeSemaphore_p);
        }
    }
#endif
    /* Tracnslate into HAL-indepedent status */
    Status = TranslateToDecodeStatus(HALDecodeHandle, HWStatus);


    /* include missing its */
    Status |= Data_p->MissingHWStatus;
    Data_p->MissingHWStatus = 0;

#ifdef STVID_STVAPI_ARCHITECTURE
    /* following code is commented because some MB2R interrupts seem */
    /* to be missing. For the moment, only the MB reconstruction decides */
    /* of the completion of the raster reconstruction */
    /*
    if (((Status & HALDEC_PIPELINE_IDLE_MASK) != 0) | ((Status & HALDEC_DECODER_IDLE_MASK) != 0))
    { */
        /* retrieve the HALDEC_PIPELINE_IDLE_MASK it. only the MB2R is responsible for */
        /* raising an it telling that the decode of the picture is completed. */
        /* Status &= (~(HALDEC_PIPELINE_IDLE_MASK | HALDEC_DECODER_IDLE_MASK));
    } */
#endif

    return(Status);
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
    Sdmpgo2Properties_t * Data_p;
    void * LevelAddress_p;
    void * StartAddress_p;

    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* load value into VIDn_CPTR */
    /* HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDLP, VIDn_CDLP_RESET_PARSER); */

    /* calculate tarnsfered data size */
    StartAddress_p = Data_p->LinearBitBufferStartAddress_p;
    LevelAddress_p = (void *)(HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDPTR_CUR));

    if ((U32)LevelAddress_p > (U32)StartAddress_p)
    {
        *LinearBitBufferTransferedDataSize_p = (U32)LevelAddress_p - (U32)StartAddress_p;
        return(ST_NO_ERROR);
    }

    *LinearBitBufferTransferedDataSize_p = 0;
    return(ST_ERROR_BAD_PARAMETER);

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
    U32 PesAss32;
    U32 PesTs32;
    U32 PesTs31_0;

    if ((PTS_p == NULL) || (PTS33_p == NULL))
    {
        /* Error cases */
        return(FALSE);
    }

    /* Check if there is a PTS available */
    PesAss32 = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_ASS);

    if (!((BOOL)(PesAss32 & VIDn_PES_ASS_TSA)))
    {
        /* No time stamp available: quit */
        return(FALSE);
    }

    PesTs32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_TS_TS_32);
    PesTs31_0 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_TS_TS_31_0);

    *PTS_p = PesTs31_0;
    *PTS33_p = (BOOL)(PesTs32);

/* #ifdef TRACE_UART */
    /* TraceBuffer(("PTS:%d\r\n", (*PTS_p))); */
/* #endif */

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
    /* VIDn_SCPTR has to be set with a Memory address pointer, in byte ==> SCDAlignment = 1 */
    *SCDAlignment_p = 1;

    /* temp : trickmode debug only. */
    *SCDAlignment_p = 256;
    /* end temp */


    return(ST_NO_ERROR);
} /* End of GetSCDAlignmentInBytes() function */

/*******************************************************************************
Name        : GetStartCodeAddress
Description : Get a pointer on the address of the Start Code if available
Parameters  : Decoder registers address, pointer on buffer address
Assumptions :
Limitations :
Returns     : TRUE if StartCode Address FALSE if no
*******************************************************************************/
static ST_ErrorCode_t GetStartCodeAddress(const HALDEC_Handle_t HALDecodeHandle, void ** const BufferAddress_p)
{
    U32 StartCodeAddress;

    /* read Start code pointer : SCD must be idle ! */
    StartCodeAddress = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCPTR);
    *BufferAddress_p = (void *)(StartCodeAddress /* + ((U32)((HALDEC_Properties_t *) HALDecodeHandle)->SDRAMMemoryBaseAddress_p) */ );

    return(ST_NO_ERROR);
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
    Sdmpgo2Properties_t * Data_p;

    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    HWStatus = ((HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STA)) & VIDn_STA_MASK);

#ifdef MULTI_DECODE_SEMAPHORE
    if (Data_p->OneDecodeIsOnGoing)
    {
        if ((HWStatus & VIDn_ITX_DID) != 0)
        {
            Data_p->OneDecodeIsOnGoing = FALSE;
            semaphore_signal(MultiDecodeSemaphore_p);
        }
    }
#endif /* MULTI_DECODE_SEMAPHORE */

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
   ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
   void * ConfigurationBaseAddress_p;
   U32 Cdb32 = 0;
   U32 Scdb32 = 0;
   U32 PesCfg32 = 0;
   U32 CfgDecpr32 = 0;
   U32 CfgPord32 = 0;
   U32 CfgVdctl32 = 0;
   U32 CfgVidic32 = 0;
   U32 Vldb32 = 0;

   Sdmpgo2Properties_t * Data_p;

   SAFE_MEMORY_ALLOCATE(((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p, void *,
                        ((HALDEC_Properties_t *) HALDecodeHandle)->CPUPartition_p,
                        sizeof(Sdmpgo2Properties_t));
   if (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p == NULL)
   {
       return(ST_ERROR_NO_MEMORY);
   }

   Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

   switch (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType)
   {
       case STVID_DEVICE_TYPE_5528_MPEG :
           Data_p->DecoderId = SDMPGO2_DECODER;
   switch (((HALDEC_Properties_t *) HALDecodeHandle)->DecoderNumber)
   {
       case 0 :
           ConfigurationBaseAddress_p = ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p;
           break;
              case 1 :
                  ConfigurationBaseAddress_p = (void *)((U32)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p - CFG_BASE_OFFSET_DECODER);
                  break;
              default :
                  return(ST_ERROR_BAD_PARAMETER);
                  break;
           }
           break;

       case STVID_DEVICE_TYPE_STD2000_MPEG :
           switch (((HALDEC_Properties_t *) HALDecodeHandle)->DecoderNumber)
           {
              case 0 :
                  ConfigurationBaseAddress_p = ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p;
                  Data_p->DecoderId = HDMPGO2_DECODER_0;
                  break;
              case 1 :
                  ConfigurationBaseAddress_p = (void *)((U32)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p - CFG_BASE_OFFSET_DECODER);
                  Data_p->DecoderId = HDMPGO2_DECODER_1;
                  break;
              default :
                  return(ST_ERROR_BAD_PARAMETER);
                  break;
           }
           break;

       default :
           return(ST_ERROR_BAD_PARAMETER);
           break;
   }


   /* Private datas initialisation */
   Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_NONE;
   Data_p->PESParserAccessSemaphore_p = semaphore_create_priority(1);
   Data_p->PESParserIsEnabled = FALSE;
   Data_p->FirstPictureSCAfterSofwareResetFound = FALSE;
   Data_p->LastPictureFoundAddress_p = NULL;
   Data_p->LastButOnePictureFoundAddress_p = NULL;
   Data_p->BitBufferBaseAddress_p = NULL;
   Data_p->BitBufferSize = 0;
   Data_p->BitBufferOutput32BitsValue = 0;
   Data_p->FirstSearchFollowingFirstSequence = TRUE;
   Data_p->LastFoundStartCodeAddress_p = NULL;
   Data_p->LastFoundStartCode = GREATEST_SLICE_START_CODE;
   Data_p->IsDecodingInTrickMode = FALSE;
   Data_p->MissingHWStatus = 0;

#ifdef STVID_STVAPI_ARCHITECTURE
   Data_p->MB2RBaseAddress_p = (void *) ((U32)(((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
                               + MB2R_BASE_ADDRESS_OFFSET);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

#ifdef MULTI_DECODE_SEMAPHORE
   Data_p->OneDecodeIsOnGoing = FALSE;

   if (!MultiDecodeSemaphoreInitialised)
   {
       MultiDecodeSemaphore_p = semaphore_create_priority(1);
       MultiDecodeSemaphoreInitialised = TRUE;
   }
   InitialisedInstanceNb ++;
#endif /* MULTI_DECODE_SEMAPHORE */

   /* ------------------ CFG registers -------------------------- */

   /* all decodes priorities between VID1 and VID2 must be internally choosen */
   CfgDecpr32 &= (~CFG_DECPR_FORCE_PRIORITY);
   HAL_Write32((U8 *)ConfigurationBaseAddress_p + CFG_DECPR, CfgDecpr32);

   /* hardware priorities between SC1, SCD2, DP */
   CfgPord32 |= CFG_PORD_CD_ROUND_ROBIN | CFG_PORD_VD_DP_SCD1_SCD2;
   HAL_Write32((U8 *)ConfigurationBaseAddress_p + CFG_PORD, CfgPord32);

   /* set reference period T != 0 for panic time THS=(T-R)/T */
   HAL_Write32((U8 *)ConfigurationBaseAddress_p + CFG_THP, 0xFFFF);

   /* video decoder control */
#ifdef WA_GNBvd24093
   CfgVdctl32  &= (~CFG_VDCTL_MVC_EN);
#endif
   CfgVdctl32 |= CFG_VDCTL_RMM_EN;
   CfgVdctl32 &= (~(CFG_VDCTL_ERU_EN | CFG_VDCTL_ERO_EN));
   HAL_Write32((U8 *)ConfigurationBaseAddress_p + CFG_VDCTL, CfgVdctl32);

   if ((Data_p->DecoderId == HDMPGO2_DECODER_0) | (Data_p->DecoderId == HDMPGO2_DECODER_1))
   {
     CfgVidic32 = CFG_VIDIC_PDRC_2 | CFG_VIDIC_PDRL_2 | CFG_VIDIC_LP_2 | CFG_VIDIC_PFO_EN;
   }
   else
   {
   /* Vidic : set all packets size set to default, PFO set to 1 (Will PTS be lost if CD fifo is full ?) */
     CfgVidic32 = CFG_VIDIC_PDRC_DEFAULT | CFG_VIDIC_PDRL_DEFAULT | CFG_VIDIC_LP_DEFAULT | CFG_VIDIC_PFO_EN;
   }
   HAL_Write32((U8 *)ConfigurationBaseAddress_p + CFG_VIDIC, CfgVidic32);


   /* decode will be done with task force : no Vsync selection */
   HAL_Write32((U8 *)ConfigurationBaseAddress_p + CFG_VSEL, CFG_VSEL_VSYNC_DISCARDED);

   /* ------------------ VIDn registers -------------------------- */

   /* Treshold low : set 0 */
   /* HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBTL, 0); */

   /* CDB : enable circular mode, enable PBO, disable Write Limit Enable. */
   Cdb32 = VIDn_CDB_CM_EN /* | VIDn_CDB_PBO_EN */;
   /* Cdb32 &= (~VIDn_CDB_WLE_EN); */ /* instruction not needed thanks to initial null value of Cdb32 */
   HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDB, Cdb32);

   /* SCDB : enable circular mode, disable SCD can not go over VIDn_SCDRL, enable SCD can not go over CD write pointer  */
   Scdb32 |= VIDn_SCDB_CM_EN | VIDn_SCDB_PRNW_EN;
   /* Scdb32 &= (~VIDn_SCDB_RLE_EN); */ /* instruction not needed thanks to initial null value of Scdb32 */
   HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCDB, Scdb32);

   /* VLDB : enable CM, diasble RLE, enable PRNW */
   Vldb32 |= VIDn_VLDB_PRNW_EN | VIDn_VLDB_CM_EN;
   /* Vldb32 &= (~VIDn_VLDB_RLE_EN); */  /* instruction not needed thanks to initial null value of Vldb32 */
   HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDB, Vldb32);

   /* All time stamps read are PTS and not DTS */
   PesCfg32 &= (~VIDn_PES_CFG_DTS_STORE_EN);
   HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG, PesCfg32);

#ifdef STVID_STVAPI_ARCHITECTURE
   MacroBlockToRasterInit(Data_p);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

   return(ErrorCode);

}/* End of Init() function */


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

#ifdef TRACE_UART
    if ((Status & HALDEC_HEADER_FIFO_NEARLY_FULL_MASK) != 0)
    {
        /* TraceBuffer(("%HnF%")); */
    }
#endif /* TRACE_UART */

    if ((Status & HALDEC_HEADER_FIFO_EMPTY_MASK) != 0)
    {
#ifdef TRACE_UART
        TraceBuffer(("%HfE!%"));
#endif /* TRACE_UART */
        return(TRUE);
    }


    return(FALSE);
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
    U32 i, Coef;
    void * RegistersBaseAddress_p, * IntraQuantMatrixAddress_p;

    if (Matrix_p != NULL)
    {
        RegistersBaseAddress_p =  ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p;
        if ((((U32)(RegistersBaseAddress_p)) & VID2_OFFSET) != 0)
        {
            /* decoder 2 ! */
            IntraQuantMatrixAddress_p = (void *)((U32)RegistersBaseAddress_p - CFG_BASE_OFFSET_DECODER + VID2_QMWI);
        }
        else
        {
            /* decoder 1 ! */
            IntraQuantMatrixAddress_p = (void *)((U32)RegistersBaseAddress_p +  VID1_QMWI);
        }

        for (i = 0; i < 64; i += 4)
        {
            Coef  = (U32) (*Matrix_p)[i    ];
            Coef |= (U32) (*Matrix_p)[i + 1] <<  8;
            Coef |= (U32) (*Matrix_p)[i + 2] << 16;
            Coef |= (U32) (*Matrix_p)[i + 3] << 24;
            HAL_Write32((U8 *)IntraQuantMatrixAddress_p + i, Coef);
        }
    }
    /* Commented because nothing to load chroma intra quant matrix on STi5528 ! */
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

    U32 i, Coef;
    void * RegistersBaseAddress_p, * IntraNonQuantMatrixAddress_p;

    if (Matrix_p != NULL)
    {
        RegistersBaseAddress_p =  ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p;
        if ((((U32)(RegistersBaseAddress_p)) & VID2_OFFSET) != 0)
        {
            /* decoder 2 ! */
            IntraNonQuantMatrixAddress_p = (void *)((U32)RegistersBaseAddress_p - CFG_BASE_OFFSET_DECODER + VID2_QMWNI);
        }
        else
        {
            /* decoder 1 ! */
            IntraNonQuantMatrixAddress_p = (void *)((U32)RegistersBaseAddress_p +  VID1_QMWNI);
        }

        for (i = 0; i < 64; i += 4)
        {
            Coef  = (U32) (*Matrix_p)[i    ];
            Coef |= (U32) (*Matrix_p)[i + 1] <<  8;
            Coef |= (U32) (*Matrix_p)[i + 2] << 16;
            Coef |= (U32) (*Matrix_p)[i + 3] << 24;

            HAL_Write32((U8 *)IntraNonQuantMatrixAddress_p + i, Coef);
        }
    }
    /* Commented because nothing to load chroma intra quant matrix on STi5528 ! */
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
    Sdmpgo2Properties_t * Data_p;
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

#ifdef MULTI_DECODE_SEMAPHORE
    if (Data_p->OneDecodeIsOnGoing)
    {
        Data_p->OneDecodeIsOnGoing = FALSE;
        semaphore_signal(MultiDecodeSemaphore_p);
    }
#endif /* MULTI_DECODE_SEMAPHORE */

    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PRS, VIDn_PRS_PIPELINE_RESET);
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
    Sdmpgo2Properties_t * Data_p;
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* update VIDn_VLDPTR  */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR, (U32)DecodeAddressFrom_p);

    *WaitForVLD_p = FALSE;

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
    Sdmpgo2Properties_t * Data_p;

    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* A PES parser reset resets also the output counter register, so we have to
     * recompute the outputcounter before resetting its value to zero */
    GetBitBufferOutputCounter(HALDecodeHandle);
    Data_p->LastBitBufferOutput24BitsValue = 0;
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDLP, VIDn_CDLP_RESET_PARSER);
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
    Sdmpgo2Properties_t * Data_p;
    U32 Scd32 = 0;
    U32 Hlnc32 = 0;

    /* return; */

    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* preparing first start code sequence search */
    if (Data_p->FirstSearchFollowingFirstSequence)
    {
        /* Scd32 = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCD); */
        /* first start code search after first sequence found : enable all start codes except non first slice start codes. */
        Scd32 &= (~VIDn_SCD_RSC_DISCARD_EN) & (~VIDn_SCD_SYSC_DISCARD_EN) & (~VIDn_SCD_GSC_DISCARD_EN)
                     & (~VIDn_SCD_SENC_DISCARD_EN) & (~VIDn_SCD_ESC_DISCARD_EN) & (~VIDn_SCD_SEC_DISCARD_EN) & (~VIDn_SCD_SHC_DISCARD_EN)
                     & (~VIDn_SCD_UDSC_DISCARD_EN) & (~VIDn_SCD_PSC_DISCARD_EN);
        Data_p->FirstSearchFollowingFirstSequence = FALSE;

        if (FirstSliceDetect)
        {
            /* disable all slice start codes discard */
            Scd32 &= (~VIDn_SCD_SSC_DISCARD_EN);
            /* enable all slice except first slice start codes discard */
            Scd32 |= VIDn_SCD_SSCF_DISCARD_EN;
        }
        else
        {
            /* enable all slice start codes discard */
            Scd32 |= VIDn_SCD_SSC_DISCARD_EN;
        }

        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCD, Scd32);
    }

    switch (SearchMode)
    {
        case HALDEC_START_CODE_SEARCH_MODE_NORMAL :
            /* RTA does not need to be written because of the normally start code search */
            Hlnc32 |= VIDn_HLNC_LNC_NORMALLY_LAUNCHED;
            /* SCNT (not always mentionned in datasheets) : must be 0. number of bytes read by start code detector */
            /* will be in VIDn_SCDC */
            Hlnc32 &= (~VIDn_HLNC_SCNT_TS_ASS_FROZEN);

            break;

        case HALDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS :
            /* write address the start code detector has to be launched from */
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCDPTR, SearchAddress_p);
            /* configure SCD to search from the given address */
            Hlnc32 |= VIDn_HLNC_LNC_FROM_ADDRESS_LAUNCHED;

            /* if (ParsingStillPicture) */
            /* { */
                /* disable PTS association */
                /* Hlnc32 &= (~VIDn_HLNC_RTA); */
                /* Hlnc32 |= VIDn_HLNC_SCNT_TS_ASS_FROZEN; */
            /* } */
            /* else */
            /* { */
                /* enable PTS association */
                Hlnc32 |= VIDn_HLNC_RTA;
                Hlnc32 &= (~VIDn_HLNC_SCNT_TS_ASS_FROZEN);
            /* } */
            break;

        default :
            break;
    }

    /* temp : search not hing in order to investiagte bbf fill */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HLNC, Hlnc32);
    /* end temp */

} /* End of SearchNextStartCode() function */


/*******************************************************************************
Name        : SetBackwardLumaFrameBuffer
Description : Set decoder backward reference luma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBackwardLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BFP, (U32) BufferAddress_p);
} /* End of SetBackwardLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetBackwardChromaFrameBuffer
Description : Set decoder backward reference chroma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBackwardChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
   HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BCHP, (U32) BufferAddress_p);
} /* End of SetBackwardChromaFrameBuffer() function */

/*******************************************************************************
Name        : SetDataInputInterface
Description : not used in 5528 where injection is not controlled by video
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
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/

static void SetDecodeBitBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p, const U32 BufferSize, const HALDEC_BitBufferType_t BufferType)
{
    Sdmpgo2Properties_t * Data_p;
    U32 Bbs32;
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

#ifdef TRACE_UART
    TraceBuffer(("SetDecodeBitBuffer\r\n"));
#endif

    switch (BufferType)
    {
        case HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR :
            Data_p->BitBufferBaseAddress_p = BufferAddress_p;
            break;
        case HALDEC_BIT_BUFFER_HW_MANAGED_LINEAR :
            /* Feature not supported by the software */
            return;
            break;
        case HALDEC_BIT_BUFFER_SW_MANAGED_CIRCULAR :
            /* Feature not supported by the software */
            return ;
            break;
        case HALDEC_BIT_BUFFER_SW_MANAGED_LINEAR :
            Data_p->LinearBitBufferStartAddress_p = BufferAddress_p;
            break;
        default:
            /* Nothing to do */
            break;
    }
    /* Store bit buffer size  */
    Data_p->BitBufferSize = BufferSize;

    /* Beginning of the bit buffer in unit of 256 bytes */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBG, ((U32) BufferAddress_p));
    /* Low threshold */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBTL, (((U32) BufferAddress_p) + 256));
    /* Beginning of the bit buffer in unit of 256 bytes */
    /* VIDn_BBS holds the ending address of the bit buffer in unit of 256 bytes. The last */
    /* address of the bit buffer is {(BBS + 1) x 256 -1} in byte units. ==>  BBS must be written with bit buffer address base + size - 256 */
    Bbs32 = (U32) BufferAddress_p +  BufferSize - 256;
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBS, Bbs32);

    /* A soft reset resets also the output counter register, so we have to recompute the OutputCounter32BitsValue before
     * resetting the output counter register */
    GetBitBufferOutputCounter(HALDecodeHandle);
    Data_p->LastBitBufferOutput24BitsValue = 0;

    /* soft reset */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SRS, VIDn_SRS_SOFT_RESET);
} /* End of SetDecodeBitBuffer() function */


/*******************************************************************************
Name        : SetDecodeBitBufferThreshold
Description : Set the threshold of the decoder bit buffer causing interrupt
Parameters  : HAL decode handle, threshold
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeBitBufferThreshold(const HALDEC_Handle_t HALDecodeHandle, const U32 BitBufferThreshold)
{
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBT, ((U32) BitBufferThreshold));
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
    U8 WidthInMacroBlock;
    U8 HeightInMacroblock;
    U16 TotalInMacroBlock;
    Sdmpgo2Properties_t * Data_p;
    const PictureCodingExtension_t * PicCodExt_p = &(StreamInfo_p->PictureCodingExtension);
    const Picture_t * Pic_p = &(StreamInfo_p->Picture);
    U32 Ppr32 = 0;
    U32 Tis32 = 0;
    U32 Tmp32;

#ifdef STVID_STVAPI_ARCHITECTURE
    U32 MainParamCfg32 = 0;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

#ifdef MULTI_DECODE_SEMAPHORE
    semaphore_wait(MultiDecodeSemaphore_p);
    Data_p->OneDecodeIsOnGoing = TRUE;
#endif

    Tis32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS);

    /* Write width in macroblocks */
    WidthInMacroBlock = (StreamInfo_p->HorizontalSize + 15) / 16;
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DFW, ((U32) WidthInMacroBlock));

    /* Write height in macroblocks */
    HeightInMacroblock = (StreamInfo_p->VerticalSize + 15) / 16;
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DFH, ((U32)HeightInMacroblock));

    /* write total number of macroblocks */
    TotalInMacroBlock = (U16)(HeightInMacroblock) * (U16)(WidthInMacroBlock);
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DFS, ((U32)TotalInMacroBlock));

    /* Same for MPEG1/MPEG2 */
    Ppr32 |= VIDn_PPR_FFN_INTERNAL;
    Tmp32 = (U32)Pic_p->picture_coding_type;
    Ppr32 |= (Tmp32 << VIDn_PPR_PCT_SHIFT) & VIDn_PPR_PCT_MASK;

    /* Exe must not be launched now */
    Tis32 &= (~VIDn_TIS_EXE);

    /* Depending on MPEG1/MPEG2 */
    if (StreamInfo_p->MPEG1BitStream)
    {
        /* Setup for MPEG1 streams */
        Tis32 &= (~VIDn_TIS_MP2_MPEG2_EN);

        /* Ppr32 |= ((((U32)Pic_p->full_pel_backward_vector << 3) | (U32)(Pic_p->backward_f_code)) << VIDn_PPR_BFH_SHIFT) & VIDn_PPR_BFH_MASK; */
        /* Ppr32 |= ((((U32)Pic_p->full_pel_forward_vector << 3) | (U32)(Pic_p->forward_f_code))) & VIDn_PPR_FFH_MASK; */

        Tmp32 = (U32)Pic_p->full_pel_backward_vector;
        Tmp32 |= Tmp32 << 3;
        Tmp32 |= (U32)Pic_p->backward_f_code;
        Ppr32 |= (Tmp32 << VIDn_PPR_BFH_SHIFT) & VIDn_PPR_BFH_MASK;

        Tmp32 = (U32)Pic_p->full_pel_forward_vector;
        Tmp32 |= Tmp32 << 3;
        Tmp32 |= (U32)(Pic_p->forward_f_code);
        Ppr32 |= Tmp32 & VIDn_PPR_FFH_MASK;
    }
    else
    {
        /* Setup for MPEG2 streams */
        Tis32 |= VIDn_TIS_MP2_MPEG2_EN;

        /* Ppr32 |= ((U32)PicCodExt_p->top_field_first << VIDn_PPR_TFF_SHIFT) */
                    /* | ((U32)PicCodExt_p->frame_pred_frame_dct << VIDn_PPR_FRM_SHIFT) */
                    /* | ((U32)PicCodExt_p->concealment_motion_vectors << VIDn_PPR_CMV_SHIFT) */
                    /* | ((U32)PicCodExt_p->q_scale_type << VIDn_PPR_QST_SHIFT) */
                    /* | ((U32)PicCodExt_p->intra_vlc_format << VIDn_PPR_IVF_SHIFT) */
                    /* | ((U32)PicCodExt_p->alternate_scan << VIDn_PPR_AZZ_SHIFT); */
        Tmp32 = (U32)PicCodExt_p->top_field_first;
        Ppr32 |= ((Tmp32 << VIDn_PPR_TFF_SHIFT) &  VIDn_PPR_TFF);
        Tmp32 = (U32)PicCodExt_p->frame_pred_frame_dct;
        Ppr32 |= ((Tmp32 << VIDn_PPR_FRM_SHIFT) &  VIDn_PPR_FRM);
        Tmp32 = (U32)PicCodExt_p->concealment_motion_vectors;
        Ppr32 |= ((Tmp32 << VIDn_PPR_CMV_SHIFT) &  VIDn_PPR_CMV);
        Tmp32 = (U32)PicCodExt_p->q_scale_type;
        Ppr32 |= ((Tmp32 << VIDn_PPR_QST_SHIFT) &  VIDn_PPR_QST);
        Tmp32 = (U32)PicCodExt_p->intra_vlc_format;
        Ppr32 |= ((Tmp32 << VIDn_PPR_IVF_SHIFT) &  VIDn_PPR_IVF);
        Tmp32 = (U32)PicCodExt_p->alternate_scan;
        Ppr32 |= ((Tmp32 << VIDn_PPR_AZZ_SHIFT) &  VIDn_PPR_AZZ);

        /* Ppr32 |= ((U32)PicCodExt_p->picture_structure << VIDn_PPR_PST_SHIFT) && VIDn_PPR_PST_MASK; */
        Tmp32 = (U32)PicCodExt_p->picture_structure;
        Ppr32 |= ((Tmp32 << VIDn_PPR_PST_SHIFT) & VIDn_PPR_PST_MASK);

        /* Ppr32 |= ((U32)PicCodExt_p->intra_dc_precision << VIDn_PPR_DCP_SHIFT) & VIDn_PPR_DCP_MASK; */
        Tmp32 = (U32)PicCodExt_p->intra_dc_precision;
        Ppr32 |= ((Tmp32 << VIDn_PPR_DCP_SHIFT) & VIDn_PPR_DCP_MASK);

        /* Ppr32 |= ((U32)PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL] << VIDn_PPR_BFV_SHIFT) & VIDn_PPR_BFV_MASK; */
        Tmp32 = (U32)PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL];
        Ppr32 |= ((Tmp32 << VIDn_PPR_BFV_SHIFT) & VIDn_PPR_BFV_MASK);

        /* Ppr32 |= ((U32)PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL] << VIDn_PPR_FFV_SHIFT) & VIDn_PPR_FFV_MASK; */
        Tmp32 = (U32)PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL];
        Ppr32 |= ((Tmp32 << VIDn_PPR_FFV_SHIFT) & VIDn_PPR_FFV_MASK);

        /* Ppr32 |= ((U32)PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL] << VIDn_PPR_BFH_SHIFT) & VIDn_PPR_BFH_MASK; */
        Tmp32 = (U32)PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL];
        Ppr32 |= ((Tmp32 << VIDn_PPR_BFH_SHIFT) & VIDn_PPR_BFH_MASK);

        /* Ppr32 |= (U32)PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL] & VIDn_PPR_FFH_MASK; */
        Tmp32 = (U32)PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL];
        Ppr32 |= (Tmp32 & VIDn_PPR_FFH_MASK);
    }

    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS, Tis32);
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PPR, Ppr32);

#ifdef STVID_STVAPI_ARCHITECTURE
    /* MB2R register : fill Main_Param_cfg (width, height, picture structure) */
    /* height */
    MainParamCfg32 = (U32)HeightInMacroblock;
    /* Width */
    Tmp32 = (U32)WidthInMacroBlock;
    MainParamCfg32 |= (Tmp32 << MB2R_MAIN_PARAM_CFG_FRAME_WIDTH_SHIFT) & MB2R_MAIN_PARAM_CFG_FRAME_WIDTH_MASK;
    /* Picture structure */
    Tmp32 = (U32)PicCodExt_p->picture_structure;
    MainParamCfg32 |= (Tmp32 << MB2R_MAIN_PARAM_CFG_PICT_STRUCT_SHIFT) & MB2R_MAIN_PARAM_CFG_PICT_STRUCT_MASK;
    /* write register */
    HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_MAIN_PARAM_CFG, MainParamCfg32);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

} /* End of SetDecodeConfiguration() function */


/*******************************************************************************
Name        : SetDecodeLumaFrameBuffer
Description : Set decoder luma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_RFP , (U32) BufferAddress_p);
} /* End of SetDecodeLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetDecodeChromaFrameBuffer
Description : Set decoder chroma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_RCHP,  (U32) BufferAddress_p);
} /* End of SetDecodeChromaFrameBuffer() function */


/*******************************************************************************
Name        : SetForwardLumaFrameBuffer
Description : Set decoder forward reference luma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetForwardLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_FFP,  (U32) BufferAddress_p);
} /* End of SetForwardLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetForwardChromaFrameBuffer
Description : Set decoder forward reference chroma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetForwardChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_FCHP, (U32) BufferAddress_p);
}


/*******************************************************************************
Name        : SetFoundStartCode
Description : Used for the Pipe Mis-alignment detection, or to inform if the start code address location has to be retrieved
Parameters  : HAL decode handle, StartCode found
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetFoundStartCode(const HALDEC_Handle_t HALDecodeHandle, const U8 StartCode, BOOL * const FirstPictureStartCodeFound_p)
{
    Sdmpgo2Properties_t * Data_p;
    /* U32 PscPtr32; */
    U32 ScPtr32;

    /* retrieve private datas */
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* init returned value */
    *FirstPictureStartCodeFound_p = FALSE;

    /* remember found start code */
    Data_p->LastFoundStartCode = StartCode;

    /* retrieve just found start code address : useful for SkipPicture function. */
    /* ScPtr32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCPTR); */
    /* Data_p->LastFoundStartCodeAddress_p = (void *)ScPtr32; */

    if (StartCode == FIRST_SLICE_START_CODE)
    {
        if (!Data_p->IsDecodingInTrickMode)
        {
            /* retrieve picture Start Code address */
            /* PscPtr32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PSCPTR); */
            ScPtr32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCPTR);

            Data_p->LastButOnePictureFoundAddress_p = Data_p->LastPictureFoundAddress_p;
            /* Data_p->LastPictureFoundAddress_p = (void *)PscPtr32; */
            Data_p->LastPictureFoundAddress_p = (void *)ScPtr32;

            if ((!Data_p->FirstPictureSCAfterSofwareResetFound))
            {
                *FirstPictureStartCodeFound_p = TRUE;
                Data_p->FirstPictureSCAfterSofwareResetFound = TRUE;
            }
        }
    }
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
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_ITM, HWMask);

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
    /* Feature not available on SDMPGO2 */
} /* End of SetMainDecodeCompression() function */


/*******************************************************************************
Name        : SetSecondaryDecodeLumaFrameBuffer
Description : Set secondary decoder luma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryDecodeLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SRFP, (U32) BufferAddress_p);

} /* End of SetSecondaryDecodeLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetSecondaryDecodeChromaFrameBuffer
Description : Set secondary decoder chroma frame buffer
Parameters  : HAL decode handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryDecodeChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SRCHP, (U32) BufferAddress_p);
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

    Sdmpgo2Properties_t * Data_p;
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_NONE;

} /* End of SetSkipConfiguration() function */


/*******************************************************************************
Name        : SetSmoothBackwardConfiguration
Description : Gives the information that the vid_dec algorithm is from now slave of trick mode one : all
              bit buffer should be from now linear and SCD and VLD will work independently.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetSmoothBackwardConfiguration(const HALDEC_Handle_t HALDecodeHandle, const BOOL SetNotReset)
{
    Sdmpgo2Properties_t * Data_p;
    U32 Cdb32;
    U32 Scdb32;
    U32 Vldb32;
    /* U32 Tis32 = 0; */

    /* retrieve private datas */
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

#ifdef TRACE_UART
    TraceBuffer(("SetSmoothBackwardConf\r\n"));
#endif

    if (SetNotReset)
    {
        /* PBO is forbidden if trick mode because its usage is based on VIB_BBL which has no meaning in trick mode. */
        /* PBO may has been changed by the application after video init : remember its value */
        /* in order to set it to its original value, when changing from Trick mode mode to automatic mode. */
        Cdb32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDB);
        /* init */
        Data_p->IsPBOenabled = FALSE;
        if ((Cdb32 & VIDn_CDB_PBO_EN) != 0)
        {
            Data_p->IsPBOenabled = TRUE;
        }
        /* CDB : disable circular mode, disable Write Limit Enable. */
        /* Cdb32 = (~VIDn_CDB_WLE_EN) & (~VIDn_CDB_CM_EN); */
        Cdb32 = 0;

        /* SCDB : Disable circular mode, disable SCD can not go over CD write pointer, disable Read limit */
        /* Scdb32 &= (~VIDn_SCDB_CM_EN) & (~VIDn_SCDB_PRNW_EN) & (~VIDn_SCDB_RLE_EN); */
        Scdb32 = 0;

        /* VLDB : diable  CM, disable RLE, disable VLG read in the bit buffer is stopped when the */
        /* VLD read pointer is equal to the CD write pointer*/
        /* Vldb32 &= (~VIDn_VLDB_PRNW_EN) & (~VIDn_VLDB_RLE_EN) & (~VIDn_VLDB_CM_EN); */
        Vldb32 = 0;

        Data_p->IsDecodingInTrickMode = TRUE;
    }
    else
    {
        /* CDB : enable circular mode, enable PBO, disable Write Limit Enable. */
        Cdb32 = VIDn_CDB_CM_EN ;
        Cdb32 &= (~VIDn_CDB_WLE_EN);
        if (Data_p->IsPBOenabled)
        {
            Cdb32 |= VIDn_CDB_PBO_EN;
            /* Data_p->IsPBOenabled is now unuseful. dont change it. */
        }

        /* SCDB : enable circular mode, disable SCD can not go over VIDn_SCDRL, enable SCD can not go over CD write pointer  */
        Scdb32 = VIDn_SCDB_CM_EN | VIDn_SCDB_PRNW_EN;
        Scdb32 &= (~VIDn_SCDB_RLE_EN);

        /* VLDB : enable CM, diasble RLE, enable PRNW */
        Vldb32 = VIDn_VLDB_PRNW_EN | VIDn_VLDB_CM_EN;
        Vldb32 &= (~VIDn_VLDB_RLE_EN);

        Data_p->IsDecodingInTrickMode = FALSE;
    }

    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDB, Cdb32);
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCDB, Scdb32);
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDB, Vldb32);

    return(ST_NO_ERROR);

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
    /* The PES parser filters data and transmit them when the following equation is true  */
    /* (for video decoders) : (Stream ID & VIDn_PES_FSI) = (VIDn_PES_SI & VIDn_PES_FSI) */

    if (StreamID == STVID_IGNORE_ID)
    {
        /* Filter all the video streams : VIDn_PES_FSI and VIDn_PES_SI contain 0xE0 following ISO/IEC 13818-1 */
        /* All the Video stream will de filtered */
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_FSI, 0x000000F0);
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_SI, 0x000000E0);
    }
    else
    {
        /* Set the stream ID */
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_SI, (U32)(StreamID|0xE0));
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_FSI, 0x000000FF);
        /* Set TFI to 0xFF when not in trick modes ! */
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_TFI, 0x000000FF);
    }
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
    /* MODE[1:0]: Mode. Selects which kind of stream is expected. */
        /* 00 : automatic configuration. Depending on the syntax of the stream, MPEG2 PES stream or MPEG1 SYSTEM stream is selected. */
        /* 01 : MPEG1 system stream or MPEG1 packet expected */
        /* 1x : MPEG2 program stream or MPEG2 PES packet expected */
    /* OFS[1:0] : Output Format Selection. This field selects which kind of bistream is transmitted to the video bit buffer and decoder. */
        /* 00 : PES stream transmitted */
        /* 11 : Elementary Stream transmitted, Stream ID layer removed */
    /* SS : System Stream. This bit, when set, switches on the parser and a system stream (MPEG-2 video PES stream or MPEG-2  */
    /* program stream) can be decoded. When not set, the parser is disabled and pure video ele-mentary streams only can be decoded. */

    U32 PesCfg32;
    Sdmpgo2Properties_t * Data_p;
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* Protect PES parser access by semaphore */
    semaphore_wait(Data_p->PESParserAccessSemaphore_p);

    PesCfg32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG);
    PesCfg32 &= ((~VIDn_PES_CFG_MODE_MASK) & (~VIDn_PES_CFG_OFS_MASK) & (~VIDn_PES_CFG_PARSER_EN));

    switch(StreamType)
    {
        case STVID_STREAM_TYPE_ES :
            /* SS = 0, MOD = 0, OFS = 3 */
            PesCfg32 |= VIDn_PES_CFG_OFS_ES;
            PesCfg32 &= (~VIDn_PES_CFG_PARSER_EN);
            Data_p->PESParserIsEnabled = FALSE;
            break;

        case STVID_STREAM_TYPE_PES :
            /* SS = 1, MOD = 0x1x, OFS = 3 */
            PesCfg32 |= VIDn_PES_CFG_PARSER_EN | VIDn_PES_CFG_MODE_MPEG2_PES | VIDn_PES_CFG_OFS_ES;
            Data_p->PESParserIsEnabled = TRUE;
            break;

        case STVID_STREAM_TYPE_MPEG1_PACKET :
            /* SS = 1, MOD = 1, OFS = 3 */
            PesCfg32 |= VIDn_PES_CFG_PARSER_EN | VIDn_PES_CFG_MODE_MPEG1 | VIDn_PES_CFG_OFS_ES;
            Data_p->PESParserIsEnabled = TRUE;
            break;

        default :
            break;
    }

    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG, PesCfg32);

    /* Un Protect PES parser access by semaphore */
    semaphore_signal(Data_p->PESParserAccessSemaphore_p);

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
    U32 Tis32;
    void * PictureToSkipAddress_p;
    Sdmpgo2Properties_t * Data_p;
    BOOL PID_DID_NotReset = FALSE;

    /* retrieve private datas */
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    if (Data_p->IsDecodingInTrickMode)
    {
        /* This function has no sense if decoding in trickmode */
        return;
    }

    /* read VIDn_TIS */
    Tis32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS);

    /* MAJ of last found pictures */
    if (Data_p->LastButOnePictureFoundAddress_p != NULL)
    {
        PictureToSkipAddress_p = Data_p->LastButOnePictureFoundAddress_p;
        Data_p->LastButOnePictureFoundAddress_p = NULL;
        /* if LastButOnePictureFoundAddress_p != NULL, BBL has be decreased of LastButOnePictureFoundAddress size */
    }
    else
    {
        PictureToSkipAddress_p = Data_p->LastPictureFoundAddress_p;
        Data_p->LastPictureFoundAddress_p = NULL;
        /* if LastButOnePictureFoundAddress_p == NULL, BBL has be decreased of the size between this last found */
        /* picture start address and last start code found. */
    }

    /* MAJ of VLD pointer in order to decrease BBL. */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDUPTR, (U32)(PictureToSkipAddress_p));
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDUP, 0x1);

#ifdef TRACE_UART
    TraceBuffer(("Skip Pic at 0x%x\r\n ", PictureToSkipAddress_p ));
#endif


    switch (SkipMode)
    {
        case HALDEC_SKIP_PICTURE_1_THEN_DECODE :
            Tis32 |= VIDn_TIS_SKP_SK_1_PIC_DEC_NXT;
            break;

        case HALDEC_SKIP_PICTURE_2_THEN_DECODE :
            Tis32 |= VIDn_TIS_SKP_SK_2_PIC_DEC_NXT;
            break;

        case HALDEC_SKIP_PICTURE_1_THEN_STOP :
            /* Tis32 |= VIDn_TIS_SKP_SK_1_PIC_STOP; */
            PID_DID_NotReset = TRUE;
            break;

        default :
            /* Bad parameter: skip one and stop */
            /* Tis32 |= VIDn_TIS_SKP_SK_1_PIC_STOP; */
            PID_DID_NotReset = TRUE;
            break;
    }

    if (PID_DID_NotReset)
    {
        /* PID and DID are not reset when skip task begins ==> they have to be simulated */
        Data_p->MissingHWStatus |= HALDEC_DECODER_IDLE_MASK | HALDEC_PIPELINE_IDLE_MASK;
    }
    else
    {
        /* FIS bit depends on launch task on Next Vsync */
        if (!ExecuteOnlyOnNextVSync)
        {
            Tis32 |= VIDn_TIS_FIS_EN;
        }

        Tis32 &= (~VIDn_TIS_SKP_MASK);
        Tis32 |= VIDn_TIS_EXE;

        HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR ,
                 (((U32)(PictureToSkipAddress_p))));
        HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS, Tis32);
    }

} /* End of SkipPicture() function */


#ifdef ST_SecondInstanceSharingTheSameDecoder
/*******************************************************************************
Name        : StartDecodeStillPicture
Description : Launch decode of a still picture

Parameters  : HAL decode handle, BitBufferAddress_p , DecodedPicture_p, StreamInfoForDecode
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void StartDecodeStillPicture(const HALDEC_Handle_t HALDecodeHandle, const void * BitBufferAddress_p , const VIDBUFF_PictureBuffer_t * DecodedPicture_p, const StreamInfoForDecode_t * StreamInfo_p)
{
    U32  TisStill32, Tis32        ;
    U32  Pscptr32          ;
    U32  Status            ;

    /* prepare Still picture decode  */
    SetDecodeConfiguration(HALDecodeHandle, StreamInfo_p)  ;
    SetDecodeLumaFrameBuffer(HALDecodeHandle, DecodedPicture_p->FrameBuffer_p->Allocation.Address_p);
    SetDecodeChromaFrameBuffer(HALDecodeHandle, DecodedPicture_p->FrameBuffer_p->Allocation.Address2_p);
    EnablePrimaryDecode(HALDecodeHandle);

    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)+ VIDn_VLDPTR, (U32)(BitBufferAddress_p));

    /* save task instruction state */
    Tis32 = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)+ VIDn_TIS           );
    TisStill32 = Tis32 ;
    TisStill32   |= (VIDn_TIS_SPD | VIDn_TIS_STD | VIDn_TIS_EXE  | VIDn_TIS_FIS_EN  | VIDn_TIS_LFR_EN | VIDn_TIS_LDP_EN | VIDn_TIS_SCN_EN);
    TisStill32   &=~VIDn_TIS_EOD  ;

     /* launch still picture decode */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)+ VIDn_TIS   , TisStill32);   /* BC3 */

    Status = 0 ;
    while((Status & (HALDEC_DECODER_IDLE_MASK|HALDEC_PIPELINE_IDLE_MASK))==0)
    {
        Status = GetStatus(HALDecodeHandle); /* Wait decode of still is finished    */
        STOS_TaskDelayUs(250);  /* waits 250 microsec. */
    }
    /* put back the old task instruction state */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)+ VIDn_TIS   , Tis32);

} /* End of StartDecodeStillPicture() function */
#endif /* ST_SecondInstanceSharingTheSameDecoder */

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
    U32 Tis32;
    U32 Dmod32;
    /* U32 Vldptr32, Vldptrcur32; */
    Sdmpgo2Properties_t * Data_p;
    void * AddressToDecodeFrom_p;
    /* static U8 Error = 0; */

    /* retrieve private datas */
    Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* SecondaryDecodeOverWrittingDisplay is not a feature sdmpgo2 nor hdmpgo2 */
    /* MainDecodeOverWrittingDisplay is a feature of only hdmpgo2 (AND ONLY the PRIMARY DECODER !) */
    if ((SecondaryDecodeOverWrittingDisplay) ||
       ((MainDecodeOverWrittingDisplay) && (Data_p->DecoderId != HDMPGO2_DECODER_0)))
    {
        return;
    }

    /* manage the OVW feature (HDMPGO2_DECODER_0 only) */
    if (Data_p->DecoderId == HDMPGO2_DECODER_0)
    {
       Dmod32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD);
       if (MainDecodeOverWrittingDisplay)
       {
           Dmod32 |= VIDn_DMOD_OVW_EN;
       }
       else
       {
           Dmod32 &= (~VIDn_DMOD_OVW_EN);
       }
       HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD, Dmod32);
    }

    Tis32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS);

    /* first picture after Sofware reset hes to be decoded from the First picture start code address */
    if (!Data_p->IsDecodingInTrickMode)
    {
        if (Data_p->LastButOnePictureFoundAddress_p != NULL)
        {
            AddressToDecodeFrom_p = Data_p->LastButOnePictureFoundAddress_p;
            Data_p->LastButOnePictureFoundAddress_p = NULL;
        }
        else
        {
            AddressToDecodeFrom_p = Data_p->LastPictureFoundAddress_p;
            Data_p->LastPictureFoundAddress_p = NULL;
        }

        /* Launch start code detector with decode launch */
        Tis32 &= (~VIDn_TIS_SCN_EN);

        HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR ,
                    (((U32)(AddressToDecodeFrom_p))));

        /* Vldptr32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR); */
        /* if (Vldptr32 != (U32)AddressToDecodeFrom_p) */
        /* { */
            /* HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR , */
                    /* (((U32)(AddressToDecodeFrom_p)))); */
        /* } */
    }
    else
    {
        /* prevent Launch start code detector with decode launch */
        Tis32 |= VIDn_TIS_SCN_EN;
    }

#ifdef TRACE_UART
    /* TraceBuffer(("Dec. 0x%x\r\n", AddressToDecodeFrom_p)); */
#endif

    /* VIDn_Tis has to be set with its -decode from address- flags */
    Tis32 |= VIDn_TIS_LDP_EN | VIDn_TIS_LFR_EN;

#ifdef WA_GNBvd27888
    /* EOD : make the VLD enter in a idle state after a decoding task on the first start code which is not a slice start code. */
    Tis32 &= (~VIDn_TIS_EOD);
    /* SPD : make decoding task finish when VIDn_STA.DID is set. */
    Tis32 |= VIDn_TIS_SPD;
#endif

    /* Bit FIS : depends on Force Instruction parameter */
    if (!ExecuteOnlyOnNextVSync)
    {
        Tis32 |= VIDn_TIS_FIS_EN;
    }

    Tis32 |= VIDn_TIS_EXE;

#ifdef STVID_STVAPI_ARCHITECTURE
    HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CTL, MB2R_RECONST_CTL_START);
#endif /* enf of def STVID_STVAPI_ARCHITECTURE */

    /* launch decode */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS, Tis32);


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
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;
    U32 Tmp32;

#ifdef MULTI_DECODE_SEMAPHORE
    if (InitialisedInstanceNb == 1)
    {
        semaphore_delete(MultiDecodeSemaphore_p);
        MultiDecodeSemaphoreInitialised = FALSE;
    }
    InitialisedInstanceNb --;
#endif /* MULTI_DECODE_SEMAPHORE */

    /* Disable all interrupts */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_ITM, 0);
    /* Clear pending interrupts. */
    Tmp32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_ITS);

#ifdef STVID_STVAPI_ARCHITECTURE
    MacroBlockToRasterTerm();
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

    semaphore_delete(Data_p->PESParserAccessSemaphore_p);

    SAFE_MEMORY_DEALLOCATE(((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p,
                           ((HALDEC_Properties_t *) HALDecodeHandle)->CPUPartition_p,
                           sizeof(Sdmpgo2Properties_t));
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

#ifdef STVID_STVAPI_ARCHITECTURE
/*******************************************************************************
Name        : SetPrimaryRasterReconstructionLumaFrameBuffer
Description :
Parameters  : HAL decode handle, Buffer to be filled start address, pitch.
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPrimaryRasterReconstructionLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p, const U32 Pitch)
{
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* MB2R register : fill Main_LBA_cfg */
    HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_MAIN_LBA_CFG, (U32)BufferAddress_p);

    /* MB2R register : fill Main_MLL_cfg */
    HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_MAIN_MLL_CFG, Pitch);
}

/*******************************************************************************
Name        : SetPrimaryRasterReconstructionChromaFrameBuffer
Description :
Parameters  : HAL decode handle, Buffer to be filled start address, pitch.
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPrimaryRasterReconstructionChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* MB2R register : fill Main_CBA_cfg  */
    HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_MAIN_CBA_CFG, (U32)BufferAddress_p);
}

/*******************************************************************************
Name        : SetSecondaryRasterReconstructionLumaFrameBuffer
Description :
Parameters  : HAL decode handle, Buffer to be filled start address, pitch.
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryRasterReconstructionLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p, const U32 Pitch)
{
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* MB2R register : fill Second_LBA_cfg  */
    HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_SECOND_LBA_CFG, (U32)BufferAddress_p);

    /* MB2R register : fill Second_MLL_cfg */
    HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_SECOND_MLL_CFG, Pitch);
}

/*******************************************************************************
Name        : SetSecondaryRasterReconstructionChromaFrameBuffer
Description :
Parameters  : HAL decode handle, Buffer to be filled start address, pitch.
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryRasterReconstructionChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle, void * const BufferAddress_p)
{
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* MB2R register : fill Second_CBA_cfg  */
    HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_SECOND_CBA_CFG, (U32)BufferAddress_p);
}

/*******************************************************************************
Name        : EnablePrimaryRasterReconstruction
Description :
Parameters  : HAL Decode manager handle
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     :
*******************************************************************************/
static void EnablePrimaryRasterReconstruction(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 Dmod32;
    U32 Reconst_cfg32;
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* MB2R register : set Reconst_cfg.Main_Raster to 1 */
    Reconst_cfg32 = HAL_Read32((U8 *) (Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CFG);
    Reconst_cfg32 |= MB2R_RECONST_CFG_MAIN_RASTER;
    HAL_Write32((U8 *) (Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CFG, Reconst_cfg32);

    /* Set DMOD.EN to 1 */
    Dmod32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD);
    Dmod32 |= VIDn_DMOD_PRI_REC_EN;
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD, Dmod32);
}
/*******************************************************************************
Name        : DisablePrimaryRasterReconstruction
Description : (DTV - STD2000 only)
Parameters  : HAL Decode manager handle,
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     :
*******************************************************************************/
static void DisablePrimaryRasterReconstruction(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 Reconst_cfg32;
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* do not change DMOD.EN  */
    /* MB2R register : set Reconst_cfg.Main_Raster to 0 */
    Reconst_cfg32 = HAL_Read32((U8 *) (Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CFG);
    Reconst_cfg32 &= (~MB2R_RECONST_CFG_MAIN_RASTER);
    HAL_Write32((U8 *) (Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CFG, Reconst_cfg32);
}
/*******************************************************************************
Name        : EnableSecondaryRasterReconstruction
Description :
Parameters  : HAL Decode manager handle,
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     :
*******************************************************************************/
static void EnableSecondaryRasterReconstruction(const HALDEC_Handle_t HALDecodeHandle, const STVID_DecimationFactor_t H_Decimation, const STVID_DecimationFactor_t V_Decimation, const STVID_ScanType_t ScanType)
{
    U32 SDmod32;
    U32 Second_param_cfg32 = 0 ;
    Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* SDMOD must be updated with the HDEC/VDEC and SDMOD.EN has to be set to 1 */
    /* MB2R register update Second Param cfg. */

    switch (H_Decimation)
    {
        case STVID_DECIMATION_FACTOR_H2 :
            SDmod32 |= VIDn_SDMOD_HDEC_H2;
            Second_param_cfg32 |= MB2R_SECOND_PARAM_CFG_HDEC_H2;
            break;

        case STVID_DECIMATION_FACTOR_H4 :
            SDmod32 |= VIDn_SDMOD_HDEC_H4;
            Second_param_cfg32 |= MB2R_SECOND_PARAM_CFG_HDEC_H4;
            break;

        default :
            /* VIDn_SDMOD_HDEC_NONE is 0 : nothing to do */
            /* SDmod32 |= VIDn_SDMOD_HDEC_NONE; */
            /* Second_param_cfg32 |= MB2R_SECOND_PARAM_CFG_HDEC_NONE; */
            break;
    }
    switch (V_Decimation)
    {
        case STVID_DECIMATION_FACTOR_V2 :
            SDmod32 |= VIDn_SDMOD_VDEC_V2;
            Second_param_cfg32 |= MB2R_SECOND_PARAM_CFG_VDEC_V2;
            break;

        case STVID_DECIMATION_FACTOR_V4 :
            SDmod32 |= VIDn_SDMOD_VDEC_V4;
            Second_param_cfg32 |= MB2R_SECOND_PARAM_CFG_VDEC_V4;
            break;

        default :
            /* VIDn_SDMOD_VDEC_NONE is 0 : nothing to do */
            /* SDmod32 |= VIDn_SDMOD_VDEC_NONE; */
            /* Second_param_cfg32 |= MB2R_SECOND_PARAM_CFG_VDEC_NONE; */
            break;
    }

    if (ScanType == STVID_SCAN_TYPE_PROGRESSIVE)
    {
        SDmod32 |= VIDn_SDMOD_PS_EN;
    }
    else
    {
        /* nothing to do, bit alreday set to 0 above */
    }

    /* enable decode decimation */
    SDmod32 |= VIDn_SDMOD_2nd_REC_EN;

    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD, SDmod32);
    HAL_Write32((U8 *)(Data_p->MB2RBaseAddress_p) + MB2R_SECOND_PARAM_CFG , Second_param_cfg32);

}
/*******************************************************************************
Name        : DisableSecondaryRasterReconstruction
Description :
Parameters  : HAL Decode manager handle,
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DisableSecondaryRasterReconstruction(const HALDEC_Handle_t HALDecodeHandle)
{
    /* MB2R register : nothing to do  */

    /* Set SDMOD.EN has to be set to 0 */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD, 0);
}

/*******************************************************************************
Name        : GetMacroBlockToRasterStatus
Description :
Parameters  : HAL Decode manager handle,
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     : Macroblock to raster status
*******************************************************************************/
static U32 GetMacroBlockToRasterStatus(const HALDEC_Handle_t HALDecodeHandle)
{
   U32 Reconst_sta32;
   Sdmpgo2Properties_t * Data_p = (Sdmpgo2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

   Reconst_sta32 = HAL_Read32((U8 *) (Data_p->MB2RBaseAddress_p) + MB2R_RECONST_STA);

   /* Clear the its */
   HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_RECONST_CLEAR_IT, (HALDEC_MACROBLOCK_TO_RASTER_IS_IDLE | HALDEC_END_OF_MAIN_RASTER_RECONSTRUCTION));

   return (Reconst_sta32);
}

/*******************************************************************************
Name        : MacroBlockToRasterInit
Description : Initializes the macroblock to raster cell (DTV - STD2000 only)
Parameters  :
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void MacroBlockToRasterInit(const Sdmpgo2Properties_t * Data_p)
{
   /* Init of the macroblock to raster cell */
   /* MB2R register : fill Mode Register. The mode to be applied is CbCr. */
   HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_MODE, (~MB2R_MODE_CRCB_NOT_CBCR));

   /* first version : un mask only the main reconstruction it */
   HAL_Write32((U8 * )(Data_p->MB2RBaseAddress_p) + MB2R_RECONST_IT_MASK, 2);

}

/*******************************************************************************
Name        : MacroBlockToRasterTerm
Description : Terminates the macroblock to raster cell (DTV - STD2000 only)
Parameters  :
Assumptions : (DTV - STD2000 only)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void MacroBlockToRasterTerm()
{
   /* Term of the macroblock to raster cell */

}
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

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
/* End of hv_dec8.c */


