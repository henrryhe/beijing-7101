/*******************************************************************************

File name   : hal_dec2.c

Description : HAL video decode omega 2 family source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
23 Feb 2000        Created                                          PLe
25 Jan 2001        Changed CPU memory access to device access        HG
13 Mar 2001        Prototype changes of EnableSecondaryDecode       GGn
15 May 2001        First debug with 7015 cut 1.1                     HG
14 Apr 2003        STEM7020 adaptation :                            GGn
                     - All write 8 or 16 bits forbidden.
                     - VIDn_HDF bug work around.
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

#ifdef STEM_7020
#define WA_HEADER_FIFO_ACCESS   /* GNBvd20614 In 5516 mode, reading VIDn_HDF in two 16bit read looses 1 word    */
                                /* over 2.                                                                      */
#endif /* STEM_7020 */

/* Temporary: work-arounds 7020 */
#define WA_GNBvd18453       /* HDPip decode hangs if horiz. size not multiple of 4 MB.                          */
#define WA_GNBvd16741       /* Pipe reset sometimes crashes the IC.                                             */
#define WA_GNBvd12779       /* Main or Aux Display randomly crashes on some streams.                            */
#define WA_GNBvd13097       /* VIDn_PSCPTR value is sometimes outside the bit buffer                            */
/*#define WA_GNBvd06249*/   /* Only necessary for STi7015                                                       */
#define WA_GNBvd06252       /* Although it has been put in the datasheet of the STi7015, it is still considered */
                            /* as a bug that it returns a value in 16bits words instead of byte.                */
#define WA_GNBvd06350       /* Video decoder crashes if too many data in bitstream after end of picture.        */
#define WA_GNBvd07112       /* Spurious Decoding Overflow Error (DUE) may happen.                               */
/*#define WA_GNBvd08438*/   /* PTS association is not correct on some decode startup conditions                 */
                            /* Removed for cut 2.0.                                                             */
/*#define WA_GNBvd16407*/   /* Decoder stops in mixed field/frame stream with secondary reconstruction.         */
/* Work-around removed for cut 2.1                                                                              */
#ifdef WA_GNBvd16407
# define DBG_INS1            0x00180   /* Debug register used to know the current field/frame decoder mode.     */
# define DBG_INS1_FIELD_MASK 0x100000  /* field bit status (set to "1" if field mode in course).                */
#endif

#define WA_READ_BBL_TWICE
#define WA_DONT_FILL_BIT_BUFFER
#define WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE
#define WA_BBL_CALCULATED_WITH_VLD_SC_POINTER       /* Work around to calculate the bit buffer level thanks     */
                                                    /* to VLDRPTR and SCRPTR register.                          */

#define WA_GNBvd16165       /* VIDn_PSCPTR is wrong if Picture Start code is parsed manually.                   */
#ifdef WA_GNBvd16165
# define WA_HEADER_FIFO_LENGTH  (256+4+12)      /* SCDRPTR relative error.                                      */
                                                /* -> Header fifo length + SC size + security size              */
#endif

/* PID missing: 1 corrects it with PID simulation, 2 corrects it with pipeline reset */
/*#define WA_PID_MISSING 1*/
/*#define WA_PID_MISSING 2*/

/* Workarounds disappearing on cut 1.2 */
/*#define WA_GNBvd07572*/


/*#define WA_DONT_LAUNCH_SC_WHEN_BBL_LOW*/
/*#define DISABLE_MAIN_DECODE_WHEN_USING_SECONDARY*/

#define WA_SC_NOT_LAUNCHED_ON_DECODE_START

/* Don't define this one: STi7015 finally doesn't need SW controller, HW is capable of doing it by itself ! */
/*#define USE_MULTI_DECODE_SW_CONTROLLER*/

#define WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD /* Work around to manage progressive HD stream      */
                                                   /* like interlaced stream (du to Hardware limits.). */
#ifdef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
#define FIRST_PROGRESSIVE_HEIGHT_TO_MANAGE_AS_INTERLACED 1080
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */

/* Define to add debug info and traces */
/*#define TRACE_DECODE*/
/*#define TRACE_MULTI_DECODE*/

#if defined TRACE_DECODE || defined TRACE_MULTI_DECODE
#define TRACE_UART
#endif /* TRACE_DECODE || TRACE_MULTI_DECODE */

#ifdef TRACE_UART
#include "trace.h"
#else
#define TraceBuffer(x)
#endif /* TRACE_UART */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"

#include "vid_com.h"
#include "vid_ctcm.h"

#include "hv_dec2.h"

#include "hvr_dec2.h"

#include "stsys.h"
#include "stcommon.h"
#include "sttbx.h"

#ifdef STVID_INJECTION_BREAK_WORKAROUND
#include "stevt.h"
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

/* Private Types ------------------------------------------------------------ */

typedef enum
{
    MULTI_DECODE_STATUS_IDLE,
    MULTI_DECODE_STATUS_READY_NOT_STARTED,
    MULTI_DECODE_STATUS_STARTED_NOT_FINISHED,
    MULTI_DECODE_STATUS_FINISHED_NOT_POSTED
} haldec_MultiDecodeStatus_t;


typedef struct {
    BOOL IsSCDValid;
    U32  PreviousSCD26bitValue;
    U32  LatchedSCD26bitValue;
    U32  PreviousSCD32bitValue;
    U32  LatchedSCD32bitValue;
    U32  BitBufferSize;
    U32  BitBufferBase;
    BOOL IsDecodingInTrickMode;
    U8   LastStartCode;
    U32  Last32HeaderData;              /* Init with one byte not 0 in LSB */
    BOOL StartCodeDetectorIdle;
    U32  MissingHWStatus;
#if defined WA_GNBvd06350 || defined WA_GNBvd16165 || defined WA_GNBvd13097
    void * LastPictureSCAddress;        /* Address of last picture SC */
    void * LastSlicePictureSCAddress;   /* Address of picture SC before last slice SC */
#endif /* WA_GNBvd06350 */
#if defined WA_GNBvd06249 || defined WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE
    BOOL IsAnyDecodeLaunchedSinceLastReset;
#endif /* WA_GNBvd06249 or WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE*/
#if defined WA_BBL_CALCULATED_WITH_VLD_SC_POINTER
    BOOL IsAnyDecodeLaunchedSinceLastResetOrSkip;
#endif /* WA_BBL_CALCULATED_WITH_VLD_SC_POINTER */
#if defined WA_GNBvd06249
    void * LastManuallyDecodedPictureAddress_p;
    void * LastButOneSlicePictureSCAddress;
#endif /* WA_GNBvd06249 */
#ifdef WA_GNBvd07572
    clock_t MaxStartCodeSearchTime;
    BOOL SimulatedSCH;
    BOOL StartCodeOnMSB;
/*    U32  LastBitBufferLevelWhenSimulatingSCH;*/
#endif /* WA_GNBvd07572 */
#if defined WA_PID_MISSING || defined STVID_HARDWARE_ERROR_EVENT
    clock_t MaxDecodeTime;
    U32  LastBitBufferLevelWhenSimulatingPID;
#endif /* WA_PID_MISSING || STVID_HARDWARE_ERROR_EVENT */
#if defined WA_PID_MISSING || defined WA_GNBvd07112 || defined STVID_HARDWARE_ERROR_EVENT
    BOOL DecoderIdle;
#endif /* WA_PID_MISSING || WA_GNBvd07112 || STVID_HARDWARE_ERROR_EVENT */
#if defined WA_GNBvd07112
    BOOL HasConcealedDUE;
#endif /* WA_GNBvd07112 */
#ifdef WA_DONT_LAUNCH_SC_WHEN_BBL_LOW
    BOOL StartCodeSearchPending;
#endif /* WA_DONT_LAUNCH_SC_WHEN_BBL_LOW */
#ifdef STVID_INJECTION_BREAK_WORKAROUND
    STEVT_EventID_t EventIDPleaseStopInjection;
    STEVT_EventID_t EventIDThanksInjectionCanGoOn;
/*    BOOL            SkipFirstStartCodeAfterSoftReset;*/
#endif /* STVID_INJECTION_BREAK_WORKAROUND */
#ifdef WA_GNBvd08438
    BOOL IsInjectionES;
#endif /* WA_GNBvd08438 */
/*    BOOL NotifiedBBFInterrupt;*/
#ifdef USE_MULTI_DECODE_SW_CONTROLLER
    VIDDEC_MultiDecodeReadyParams_t MultiDecodeReadyParams;
    U32     MultiDecodeExecutionTis;
    haldec_MultiDecodeStatus_t MultiDecodeStatus;
    BOOL    MustLaunchSCDetector;
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */

#ifdef STVID_HARDWARE_ERROR_EVENT
    STVID_HardwareError_t   DetectedHardwareError;
#endif /* STVID_HARDWARE_ERROR_EVENT */

    HALDEC_DecodeSkipConstraint_t DecodeSkipConstraint;
    BOOL FirstDecodeAfterSoftResetDone;
#if defined WA_GNBvd16407 || defined WA_GNBvd16741
    void *  ConfigurationBaseAddress_p;
#endif
#ifdef WA_GNBvd16407
    U32     NumberOfUselessPSD;             /* Work-around counter.                             */
    BOOL    IsNextDecodeField;              /* Set if the next decode will be a field decode.   */
    U32     InitialSDmod32RegisterValue;    /* SDMod register value before the work-arond.      */
    U32     InitialDmod32RegisterValue;     /* DMod register value before the work-arond.       */
#endif /* WA_GNBvd16407 */
    BOOL    isHDPIPEnabled;                 /* HD-PIP activity status.                          */
#ifdef WA_GNBvd18453
    U32     WidthInMacroBlock;              /* Horizontal picture size in MB.                   */
    U32     HeightInMacroBlock;             /* Vertical picture size in MB.                     */
#endif /* WA_GNBvd16741 */
#ifdef WA_HEADER_FIFO_ACCESS
    BOOL    isPictureStartCodeDetected;     /* Flag that remember the fact a PSC has been seen  */
                                            /* and we're simulating SC until first slice.       */
    BOOL    isStartCodeSimulated;           /* Flag to remember a start code has to be simulated*/
    BOOL    isStartCodeOnMSB;               /* Flag to remember the start code position.        */
    U32     StartCodeFound;                 /* Last start code found manually.                  */
#endif /* WA_HEADER_FIFO_ACCESS */
} Omega2Properties_t;


/* Private Constants -------------------------------------------------------- */

/* Max start code search time: 8 ms for HD, 2 ms for SD */
#define MAX_START_CODE_SEARCH_TIME (9 * (ST_GetClocksPerSecond() / 1000))

/* Max decode time for biggest HD full picture: 2 Vsync's */
#define MAX_DECODE_SKIP_TIME (STVID_MAX_VSYNC_DURATION * 2)

/* Max HD size with small security */
#define MAX_SIZE_IN_MACROBLOCKS (1920 * 1152 / 16 / 16)


/* Enable/Disable */
#define HALDEC_DISABLE  0
#define HALDEC_ENABLE   1

/* ERROR */
#define HALDEC_ERROR               0xFF

/* Skip Picture */
#define HALDEC_SKIP_MAX_REGVALUE        3

/* H & V Decimations */
#define HALDEC_HDECIMATION_MAX_REGVALUE 2
#define HALDEC_VDECIMATION_MAX_REGVALUE 2

/* Compressions */
#define HALDEC_COMPRESSION_MAX_REGVALUE 2

#ifdef WA_READ_BBL_TWICE
#define READ_BBL_TWICE_THRESHOLD 512
#endif /* WA_READ_BBL_TWICE */

#if defined WA_GNBvd06249
#define WA_GNBvd06249_SCD_ACCURACY_MARGIN 384
#endif /* WA_GNBvd06249 */

#if defined WA_DONT_FILL_BIT_BUFFER
#define WA_DONT_FILL_BIT_BUFFER_MINIMUM_BBT_MARGIN 4096
#endif /* WA_DONT_FILL_BIT_BUFFER */

#ifdef WA_DONT_LAUNCH_SC_WHEN_BBL_LOW
#define WA_DONT_LAUNCH_SC_WHEN_BBL_LOW_THRESHOLD 500
#endif /* WA_DONT_LAUNCH_SC_WHEN_BBL_LOW */

#define CFG_BASE_OFFSET_DECODER_0   0x00000200
#define CFG_BASE_OFFSET_DECODER_1   0x00000400
#define CFG_BASE_OFFSET_DECODER_2   0x00000600
#define CFG_BASE_OFFSET_DECODER_3   0x00000800
#define CFG_BASE_OFFSET_DECODER_4   0x00000a00


/* Private Variables (static)------------------------------------------------ */
#ifdef WA_GNBvd08438
#define GNBvd08438_Concat8to32(a,b) \
((a) = ( (*(b) & 0x000000FF) | ((*((b)+1) << 8) & 0x0000FF00) | \
((*((b)+2) << 16) & 0x00FF0000) |  ((*((b)+3) << 24) & 0xFF000000) ))
#define GNBvd08438_DUMMY_PES_SIZE 64
#define GNBvd08438_DUMMY_PES_ES_SIZE 38
static U8 GNBvd08438_DummyPESArray[GNBvd08438_DUMMY_PES_SIZE] =
{
  0x00,
  0x00,
  0x01,/*    --Packet_Start_Code_Prefix  */
  0xE0,/*    --Video Stream_Id for video 0: $E0  */
  0x00,
  0x3A,/*        --PES_packet_length= $003A */
  0x81,
  0xE8,/*        --PTS_DTS_flags="11" */
  0x11,/*        --Pes_header_data_length = $11 */
  0x31,/*         --Marker_bits="0011";PTS[32:30]="000";Marker_bit='1'. */
  0x00,/*         --PTS[29:22]=$00; */
  0x01,/*         --PTS[21:15]="0000000";Marker_bit='1'. */
  0x00,/*         --PTS[14:7]=$00. */
  0x01,/*         --PTS[6:0]="0000000";Marker_bit='1'. */
  0x1F,/*         --Marker_bits="0001";DTS[32:30]="111";Marker_bit='1'. */
  0xFF,/*         --DTS[29:22]=$FF. */
  0xFF,/*         --DTS[21:15]="1111111";Marker_bit='1'. */
  0xE2,/*         --DTS[14:7]=$E2. */
  0xAD,/*         --DTS[6:0]="1010110";Marker_bit='1'. => DTS=$1FFFFF156. */
  0x37,
  0xEC,
  0xFD,
  0x67,
  0x14,
  0x01,
  0x21,
  0x00,
  0xFF,
  0xFF,
  0xB3,/*    --SC_Sequence removed */
  0x16,
  0x00,
  0xF0,
  0xC1,
  0x08,
  0x8B,
  0xA0,
  0xA0,
  0x00,
  0x00,
  0x01,
  0xB8,
  0x00,
  0x08,
  0x00,
  0x40,
  0x00,
  0x00,
  0x01,
  0x00,/*    --SC_picture */
  0x00,
  0x89,
  0x06,
  0xC0,
  0x00,
  0x00,
  0x01,
  0x01,/*    --SC_Slice #1 */
  0x1B,
  0xF9,
  0xC0,
  0x20,
  0xF7,
  0x00
};
#endif /* WA_GNBvd08438 */


/* Private Function prototypes ---------------------------------------------- */

static void InternalProcessInterruptStatus(const HALDEC_Handle_t HALDecodeHandle, const U32 Status);
static void NewStartCode(const HALDEC_Handle_t HALDecodeHandle, const U8 StartCode);

#ifdef DEBUG
static void HALDEC_Error(void);
#endif /* DEBUG */

static void DecodingSoftReset(const HALDEC_Handle_t HALDecodeHandle, const BOOL IsRealTime);
static void DisableSecondaryDecode(const HALDEC_Handle_t  HALDecodeHandle);
static void EnableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle,
                         const STVID_DecimationFactor_t  H_Decimation,
                         const STVID_DecimationFactor_t  V_Decimation,
                         const STVID_ScanType_t          ScanType);
static void DisablePrimaryDecode(const HALDEC_Handle_t HALDecodeHandle);
static void EnablePrimaryDecode(const HALDEC_Handle_t HALDecodeHandle);
static void DisableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p);
static void EnableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p);
static void FlushDecoder(const HALDEC_Handle_t HALDecodeHandle, BOOL * const DiscontinuityStartCodeInserted_p);
static U16 Get16BitsStartCode(const HALDEC_Handle_t  HALDecodeHandle,
                              BOOL * const           StartCodeOnMSB_p);
static U16 Get16BitsHeaderData(const HALDEC_Handle_t  HALDecodeHandle);
static U32 Get32BitsHeaderData(const HALDEC_Handle_t  HALDecodeHandle);
static U32 GetAbnormallyMissingInterruptStatus(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t GetDataInputBufferParams(
                                const HALDEC_Handle_t HALDecodeHandle,
                                void ** const BaseAddress_p,
                                U32 * const Size_p);
static void GetDecodeSkipConstraint(const HALDEC_Handle_t HALDecodeHandle, HALDEC_DecodeSkipConstraint_t * const HALDecodeSkipConstraint_p);
static U32 GetBitBufferOutputCounter(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t GetCDFifoAlignmentInBytes(const HALDEC_Handle_t HALDecodeHandle, U32 * const CDFifoAlignment_p);
static U32  GetDecodeBitBufferLevel(const HALDEC_Handle_t HALDecodeHandle);
#ifdef STVID_HARDWARE_ERROR_EVENT
static STVID_HardwareError_t  GetHardwareErrorStatus(const HALDEC_Handle_t HALDecodeHandle);
#endif /* STVID_HARDWARE_ERROR_EVENT  */
static U32  GetInterruptStatus(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t GetLinearBitBufferTransferedDataSize(const HALDEC_Handle_t HALDecodeHandle, U32 * const TransferedDataSize_p);
static BOOL GetPTS(const HALDEC_Handle_t HALDecodeHandle, U32 * const PTS_p, BOOL * const PTS33_p);
static ST_ErrorCode_t GetSCDAlignmentInBytes(const HALDEC_Handle_t HALDecodeHandle, U32 * const SCDAlignment_p);
static ST_ErrorCode_t GetStartCodeAddress(const HALDEC_Handle_t HALDecodeHandle, void ** const BufferAddress_p);
static U32 GetStatus(const HALDEC_Handle_t  HALDecodeHandle);
static BOOL IsHeaderDataFIFOEmpty(const HALDEC_Handle_t  HALDecodeHandle);
static ST_ErrorCode_t Init(const HALDEC_Handle_t  HALDecodeHandle);
#ifdef USE_MULTI_DECODE_SW_CONTROLLER
static void MultiDecodeExecuteDecode(void * const ExecuteParam_p);
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */
static void PipelineReset(const HALDEC_Handle_t  HALDecodeHandle);
static void PrepareDecode(const HALDEC_Handle_t HALDecodeHandle, void * const DecodeAddressFrom_p, const U16 pTemporalReference, BOOL * const WaitForVLD_p);
static void ResetPESParser(const HALDEC_Handle_t HALDecodeHandle);
static void LoadIntraQuantMatrix(
                         const HALDEC_Handle_t   HALDecodeHandle,
                         const QuantiserMatrix_t *Matrix_p,
                         const QuantiserMatrix_t *ChromaMatrix_p);
static void LoadNonIntraQuantMatrix(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         const QuantiserMatrix_t *Matrix_p,
                         const QuantiserMatrix_t *ChromaMatrix_p);
static void SearchNextStartCode(
                         const HALDEC_Handle_t               HALDecodeHandle,
                         const HALDEC_StartCodeSearchMode_t  SearchMode,
                         const BOOL                          FirstSliceDetect,
                         void * const                        SearchAddressFrom_p);
static void SetInterruptMask(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         const U32              Mask);
static void SetDecodeLumaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p);
static void SetDecodeChromaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p);
static void SetSecondaryDecodeLumaFrameBuffer(const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p);
static void SetSecondaryDecodeChromaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p);
static void SetBackwardLumaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p);
static void SetBackwardChromaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p);
static ST_ErrorCode_t SetDataInputInterface(
           const HALDEC_Handle_t HALDecodeHandle,
           ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle,
                                            void ** const Address_p),
           void (*InformReadAddress)(void * const Handle, void * const Address),
           void * const Handle );
static void SetForwardLumaFrameBuffer(
                         HALDEC_Handle_t  HALDecodeHandle,
                         void * const     BufferAddress_p);
static void SetForwardChromaFrameBuffer(
                         HALDEC_Handle_t  HALDecodeHandle,
                         void * const     BufferAddress_p);
static void SetFoundStartCode(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         const U8               StartCode,
                         BOOL * const FirstPictureStartCodeFound_p);
static void SetDecodeBitBuffer(
                         const HALDEC_Handle_t          HALDecodeHandle,
                         void * const                   BufferAddressStart_p,
                         const U32                      BufferSize,
                         const HALDEC_BitBufferType_t   BufferType);
static void SetDecodeBitBufferThreshold(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         const U32              OccupancyThreshold);
static void SetMainDecodeCompression(
                         const HALDEC_Handle_t           HALDecodeHandle,
                         const STVID_CompressionLevel_t  Compression);
static void SetDecodeConfiguration(
                         const HALDEC_Handle_t               HALDecodeHandle,
                         const StreamInfoForDecode_t * const StreamInfo_p);
static void SetSkipConfiguration(
                         const HALDEC_Handle_t                  HALDecodeHandle,
                         const STVID_PictureStructure_t * const PictureStructure_p);
static ST_ErrorCode_t SetSmoothBackwardConfiguration(
                         const HALDEC_Handle_t               HALDecodeHandle,
                         const BOOL                          SetNotReset);
static void SetStreamID(const HALDEC_Handle_t HALDecodeHandle,
                        const U8 StreamID);
static void SetStreamType(const HALDEC_Handle_t    HALDecodeHandle,
                          const STVID_StreamType_t StreamType);
static ST_ErrorCode_t Setup(const HALDEC_Handle_t HALDecodeHandle,
                            const STVID_SetupParams_t * const SetupParams_p);
static void SkipPicture(const HALDEC_Handle_t     HALDecodeHandle,
                        const BOOL                      OnlyOnNextVsync,
                        const HALDEC_SkipPictureMode_t  Skip);
static void StartDecodePicture(const HALDEC_Handle_t              HALDecodeHandle,
                         const BOOL                         OnlyOnNextVsync,
                         const BOOL                         MainDecodeOverWrittingDisplay,
                         const BOOL                         SecondaryDecodeOverWrittingDisplay);
static void Term(const HALDEC_Handle_t  HALDecodeHandle);
#ifdef STVID_DEBUG_GET_STATUS
static void GetDebugStatus(const HALDEC_Handle_t  HALDecodeHandle,
            STVID_Status_t* const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
static void WriteStartCode(const HALDEC_Handle_t  HALDecodeHandle,
            const U32             SCVal,
            const void * const    SCAdd_p);



/* Global Variables --------------------------------------------------------- */

const HALDEC_FunctionsTable_t HALDEC_Omega2Functions =
{
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
#endif /* STVID_HARDWARE_ERROR_EVENT  */
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

/* Private Macros ----------------------------------------------------------- */

#define HAL_Read8(Address_p)     STSYS_ReadRegDev8((void *) (Address_p))
#define HAL_Read16(Address_p)    STSYS_ReadRegDev16LE((void *) (Address_p))
#define HAL_Read32(Address_p)    STSYS_ReadRegDev32LE((void *) (Address_p))

/* Warning : For compliancy constraint, only 32BITS write are allowed         */
/* (see 5517 + STEM 7020 for more details).                                   */
#define HAL_Write32(Address_p, Value) STSYS_WriteRegDev32LE((void *) (Address_p), (Value))

#define HAL_SetRegister32Value(Address_p, RegMask, Mask, Emp,Value)                                         \
{                                                                                                           \
    U32 tmp32;                                                                                              \
    tmp32 = HAL_Read32 (Address_p);                                                                         \
    HAL_Write32 (Address_p, ((tmp32) & (RegMask) & (~((U32)(Mask) << (Emp))) ) | ((U32)(Value) << (Emp)));  \
}

#define HAL_GetRegister16Value(Address_p, Mask, Emp)  (((U16)(HAL_Read16((Address_p))) & ((U16)((Mask)) << (Emp))) >> (Emp))
#define HAL_GetRegister8Value(Address_p, Mask, Emp)   (((U8) (HAL_Read8( (Address_p))) & ((U8) ((Mask)) << (Emp))) >> (Emp))

#define  Min(Val1, Val2) (((Val1) < (Val2))?(Val1):(Val2))
#define  Max(Val1, Val2) (((Val1) > (Val2))?(Val1):(Val2))

#define DistanceU32(U1, U2) (((U2)>(U1))?((U2)-(U1)):((U1)-(U2)))

#ifdef TRACE_MULTI_DECODE
#define HALDEC_MULTI_DECODE_TRACE_DECODER_NONE (-1)
#define HALDEC_MULTI_DECODE_TRACE_DECODER_ALL (-2)
/* Select below before ALL, NONE, or just a decoder number : */
/*#define HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE HALDEC_MULTI_DECODE_TRACE_DECODER_ALL*/
/*#define HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE HALDEC_MULTI_DECODE_TRACE_DECODER_NONE*/
#define HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE HALDEC_MULTI_DECODE_TRACE_DECODER_ALL
#ifdef TRACE_UART
#define TraceBufferDecoderNumber(HALDecodeHandle) TraceBuffer(("\r\nD%d-%x", ((HALDEC_Properties_t *) (HALDecodeHandle))->DecoderNumber, time_now() & 0xFFFFFF))
#endif /* TRACE_UART */
#endif /* TRACE_MULTI_DECODE */

#define SwapBytesIntoLong(a,b) \
((a) = ((((b) & 0x000000FF)  << 24) | (((b) & 0x0000FF00)  << 8) | (((b) & 0x00FF0000) >> 8) | (((b) & 0xFF000000) >> 24)))

/* Functions ---------------------------------------------------------------- */

#ifdef  WA_HEADER_FIFO_ACCESS
/*******************************************************************************
Name        : Next_start_code
Description : Parse manually the header fifo until a next start code or header
              fifo empty event. Returns as output parameters the startc code
              found (if any, and its position
Parameters  : HAL Decode manager handle, ...
Assumptions :
Limitations :
Returns     : N.A.
*******************************************************************************/
static void Next_start_code (const HALDEC_Handle_t  HALDecodeHandle,
        BOOL *isStartCodeFound_p,
        BOOL *isStartCodeOnMSB_p,
        U32  *StartCodeFound_p)
{
    #define NB_MAX_HDF_TRY  500

    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
    BOOL                 ContinueStartCodePooling = TRUE;
    U32                  HdfEmptyMaxTry;
    U32                  Buffer;
    U32                  ReadVal;

    /* Default value of result. */
    *isStartCodeFound_p = FALSE;

    /* Sets Buffer with LSB byte read from header data                 */
    /* This byte is used to be sure we do not loose bytes              */
    /* during manual parsing when seeking SC 0x000001xx                */
    /* NB: this byte is used even if already consumed for header data  */
    /* This can be done because we are sure that if we find 0x000001xx */
    /* it is a Start Code */
    Buffer= 0xFFFFFF00 | ((U8) Data_p->Last32HeaderData);	/* Keeps only LSB byte of previously read data */

    do
    {
        /* Just set a max try number, not to be locked for ever !!! */
        HdfEmptyMaxTry = NB_MAX_HDF_TRY;

        while ( (IsHeaderDataFIFOEmpty(HALDecodeHandle)) && (HdfEmptyMaxTry != 0) )
        {
            TraceBuffer(("\r\n****HDF EMPTY (%d)!!!****", (NB_MAX_HDF_TRY - HdfEmptyMaxTry)));

            /* temporary stupid wait.       */
            ReadVal= 100;
            ReadVal *= ((ReadVal * ReadVal) / ReadVal) * ReadVal;

            /* Decrease max try counter.    */
            HdfEmptyMaxTry --;
        }

        if (HdfEmptyMaxTry != 0)
        {
          /* NO Header fifo empty time-out */
          /* => data readable in fifo */

          /* Get next 16 bits from header fifo.                               */
          ReadVal = HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) +
                                   VIDn_HDF_32);
          TraceBuffer(("_%04x", (U16) ReadVal));

          /* Skipps data until StartCode (0x000001xx) is found */
          if (   ((U16)Buffer == 0x0000)		 /* previous word is 0x0000     */
              && ((((U16)ReadVal) & 0xFF00) == 0x0100))  /* and read word is 0x01xx     */
          {                                              /* => SC found                 */
              TraceBuffer(("\r\nNSC(%04x)", ReadVal));

              /* Start code is on LSB.       */
              *isStartCodeOnMSB_p = FALSE;

              /* OK, next start code is found.    */
              *isStartCodeFound_p = TRUE;
              /* Set it.                          */
              *StartCodeFound_p   = ReadVal;
              /* Stop start code search by pooling*/
              ContinueStartCodePooling = FALSE;
          }
          else						  /* Else SC is not word aligned */
          if ((Buffer & 0x00FFFFFF) == 0x00000001)	  /* SC is dispatched over 3 words */
          {						  /* Start Code byte is on the read word MSB */
              TraceBuffer(("\r\nNSC(%04x)", ReadVal));

              /* Start code is on MSB.       */
              *isStartCodeOnMSB_p = TRUE;

              /* OK, next start code is found.    */
              *isStartCodeFound_p = TRUE;
              /* Set it.                          */
              *StartCodeFound_p   = ReadVal;
              /* Stop start code search by pooling*/
              ContinueStartCodePooling = FALSE;
          }
          else						  /* SC is not found */
          {						  /* => skipps data  */
            Buffer = (Buffer << 16) | ((U16) (ReadVal));
          }
        }
        else
        {
            /* Header fifo empty time-out. Exit without solving next start code.    */
            ContinueStartCodePooling = FALSE;
        }
    } while (ContinueStartCodePooling);

} /* End of Next_start_code */
#endif



/*******************************************************************************
Name        : DecodingSoftReset
Description :
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DecodingSoftReset(const HALDEC_Handle_t HALDecodeHandle, const BOOL IsRealTime)
{
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
    U32 Tmp32;
#ifdef WA_GNBvd08438
    U32 k;
    BOOL PESInject;
#endif /* WA_GNBvd08438 */

#if defined TRACE_UART && defined TRACE_MULTI_DECODE
    TraceBufferDecoderNumber(HALDecodeHandle);
    TraceBuffer(("SoftReset********\r\n"));
#endif /* TRACE_UART && TRACE_MULTI_DECODE */

#ifdef WA_GNBvd08438
    /* Do workaround if PES or TS data */
    if (!(Data_p->IsInjectionES))
    {
/*        TraceBuffer(("Stop Injec evt\r\n"));*/
#ifdef STVID_INJECTION_BREAK_WORKAROUND
        /* Ask application to stop injection */
        STEVT_Notify(((HALDEC_Properties_t *) HALDecodeHandle)->EventsHandle, Data_p->EventIDPleaseStopInjection, STEVT_EVENT_DATA_TYPE_CAST NULL);
#endif /* STVID_INJECTION_BREAK_WORKAROUND */
        /* Ensure pair of event notification */
        PESInject = TRUE;
    }
    else
    {
        PESInject = FALSE;
    }
#endif /* WA_GNBvd08438 */

#if defined TRACE_UART && !defined TRACE_MULTI_DECODE
    TraceBuffer(("\r\nSoftReset********\r\n"));
#endif /* TRACE_UART */

    /* Reset PES parser. Otherwise PTS association in PES data will be corrupted. */
    ResetPESParser(HALDecodeHandle);

#ifndef WA_GNBvd16741
    /* reset pipeline : only the decoding of the current picture */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SRS, 0x1);
#else /* WA_GNBvd16741 */
    /* - stop the decoder (write 0 to CFG_VDCTL.EDC)        */
    Tmp32 = HAL_Read32(((U8 *) Data_p->ConfigurationBaseAddress_p) + CFG_VDCTL);
    Tmp32 &= ~CFG_VDCTL_EDC;
    HAL_Write32(((U8 *) Data_p->ConfigurationBaseAddress_p) + CFG_VDCTL, Tmp32);
    /* - wait for 10 us                                     */
    STTBX_WaitMicroseconds(10);

    /* reset pipeline : only the decoding of the current picture */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SRS, 0x1);

    /* - reenable the decoder (write 1 to CFG_VDCTL.EDC)    */
    Tmp32 |= CFG_VDCTL_EDC;
    HAL_Write32(((U8 *) Data_p->ConfigurationBaseAddress_p) + CFG_VDCTL, Tmp32);
#endif /* WA_GNBvd16741 */
    /* Is it useful to wait one microsecond for the reset to be completed ? */
    STTBX_WaitMicroseconds(1);

    /* Clear interrupts for events that occured prior to the SoftReset. Do that before first SCD launch, not to loose any IT. */
    Tmp32 = (U32) (HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_ITS_16) & VIDn_ITS_MASK);

#ifdef WA_GNBvd08438
    /* Do workaround if PES or TS data */
    if (PESInject)
    {
        /* Inject dummy PES packet*/
        for (k = 0; k < (GNBvd08438_DUMMY_PES_SIZE / 4); /* U8 -> U32 */ k ++)
        {
            GNBvd08438_Concat8to32(Tmp32, (GNBvd08438_DummyPESArray + (k * 4)));
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDI_32, Tmp32);
        }

/*        TraceBuffer(("Start Injec evt\r\n"));*/

#ifdef STVID_INJECTION_BREAK_WORKAROUND
        /* Now allow injection to go-on */
        STEVT_Notify(((HALDEC_Properties_t *) HALDecodeHandle)->EventsHandle, Data_p->EventIDThanksInjectionCanGoOn, STEVT_EVENT_DATA_TYPE_CAST NULL);
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

    }
#endif /* WA_GNBvd08438 */

    Data_p->StartCodeDetectorIdle = TRUE;
#if defined WA_PID_MISSING || defined WA_GNBvd07112 || defined STVID_HARDWARE_ERROR_EVENT
    Data_p->DecoderIdle = TRUE;
#endif /* WA_PID_MISSING || WA_GNBvd07112 || STVID_HARDWARE_ERROR_EVENT */
#if defined WA_PID_MISSING || defined STVID_HARDWARE_ERROR_EVENT
    Data_p->LastBitBufferLevelWhenSimulatingPID = 0;
#endif /* WA_PID_MISSING || STVID_HARDWARE_ERROR_EVENT */
#if defined WA_GNBvd07112
    Data_p->HasConcealedDUE = FALSE;
#endif /* WA_GNBvd07112 */

    /* Invalidate SCD: 4 values to reset */
    Data_p->IsSCDValid = FALSE;
    Data_p->PreviousSCD26bitValue = 0;
/*    Data_p->PreviousSCD32bitValue  = Data_p->BitBufferBase - 8;*/
    Data_p->PreviousSCD32bitValue  = 0;

    Data_p->LastStartCode = SEQUENCE_END_CODE;
    Data_p->Last32HeaderData = 0xFFFFFFFF; /* Init with one byte not 0 in LSB */
#if defined WA_GNBvd06350 || defined WA_GNBvd16165 || defined WA_GNBvd13097
    Data_p->LastPictureSCAddress = (void *) Data_p->BitBufferBase;
    Data_p->LastSlicePictureSCAddress = (void *) Data_p->BitBufferBase;
#endif /* WA_GNBvd06350 */
#if defined WA_GNBvd06249 || defined WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE
    Data_p->IsAnyDecodeLaunchedSinceLastReset = FALSE;
#endif /* WA_GNBvd06249 or WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE */
#if defined WA_BBL_CALCULATED_WITH_VLD_SC_POINTER
    Data_p->IsAnyDecodeLaunchedSinceLastResetOrSkip = FALSE;
#endif /* WA_BBL_CALCULATED_WITH_VLD_SC_POINTER */
#if defined WA_GNBvd06249
    Data_p->LastManuallyDecodedPictureAddress_p = (void *) Data_p->BitBufferBase;
    Data_p->LastButOneSlicePictureSCAddress = (void *) Data_p->BitBufferBase;
#endif /* WA_GNBvd06249 */
#ifdef WA_GNBvd07572
    Data_p->SimulatedSCH = FALSE;
/*    Data_p->LastBitBufferLevelWhenSimulatingSCH = 0;*/
#endif /* WA_GNBvd07572 */
#ifdef WA_GNBvd16407
    Data_p->NumberOfUselessPSD = 0;
#endif /* WA_GNBvd16407 */
#ifdef WA_DONT_LAUNCH_SC_WHEN_BBL_LOW
    Data_p->StartCodeSearchPending = FALSE;
#endif /* WA_DONT_LAUNCH_SC_WHEN_BBL_LOW */

    /* Simulate PEI and DEI because we don't do a dummy EXE so they won't occur.
    Also clear other pending flags */
    Data_p->MissingHWStatus = HALDEC_DECODER_IDLE_MASK | HALDEC_PIPELINE_IDLE_MASK;

    /* We are obliged to decode after a Soft Reset. */
    Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_MUST_DECODE;
    Data_p->FirstDecodeAfterSoftResetDone = FALSE;

#ifdef WA_HEADER_FIFO_ACCESS
    Data_p->isPictureStartCodeDetected  = FALSE;
    Data_p->isStartCodeSimulated        = FALSE;
#endif /* WA_HEADER_FIFO_ACCESS */

#ifdef STVID_INJECTION_BREAK_WORKAROUND
/*    Data_p->SkipFirstStartCodeAfterSoftReset = TRUE;*/
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

    /* Launch SC detector first time */
    SearchNextStartCode(HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);

} /* End of DecodingSoftReset() function */


/*******************************************************************************
Name        : DisableSecondaryDecode
Description :
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableSecondaryDecode(const HALDEC_Handle_t  HALDecodeHandle)
{
#ifdef WA_GNBvd16407
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
#endif /* WA_GNBvd16407 */

    /* Disable Secondary Decode */
    HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD,
                         VIDn_SDMOD_MASK,
                         VIDn_MOD_EN_MASK,
                         VIDn_MOD_EN_EMP,
                         HALDEC_DISABLE);
#ifdef WA_GNBvd16407
    /* Test if this work-around is activated. If yes, abort it because  */
    /* no secondary reconstruction is enabled.                          */
    Data_p->NumberOfUselessPSD = 0;
#endif /* WA_GNBvd16407 */
} /* End of DisableSecondaryDecode() function */

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
    /* Disable Secondary Decode */
    HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD,
                         VIDn_DMOD_MASK,
                         VIDn_MOD_EN_MASK,
                         VIDn_MOD_EN_EMP,
                         HALDEC_DISABLE);
} /* End of DisablePrimaryDecode() function */

/*******************************************************************************
Name        : EnableSecondaryDecode
Description : Enables Secondary Decode
Parameters  : HAL Decode manager handle
              Vertical decimation,
              Horizontal decimation,
              Scan type of the picture (progressive or interlaced).
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnableSecondaryDecode(
                         const HALDEC_Handle_t       HALDecodeHandle,
                         const STVID_DecimationFactor_t  H_Decimation,
                         const STVID_DecimationFactor_t  V_Decimation,
                         const STVID_ScanType_t          ScanType)
{
    U32 SDmod;

    SDmod = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_SDMOD) & VIDn_SDMOD_MASK;
    SDmod &= ~(VIDn_SDMOD_DEC_MASK);

    switch (H_Decimation)
    {
        case STVID_DECIMATION_FACTOR_H2 :
            SDmod |= VIDn_SDMOD_HDEC_H2;
            break;

        case STVID_DECIMATION_FACTOR_H4 :
            SDmod |= VIDn_SDMOD_HDEC_H4;
            break;

/*        case STVID_DECIMATION_FACTOR_NONE :*/
        default :
            SDmod |= VIDn_SDMOD_HDEC_NONE;
            break;
    }
    switch (V_Decimation)
    {
        case STVID_DECIMATION_FACTOR_V2 :
            SDmod |= VIDn_SDMOD_VDEC_V2;
            break;

        case STVID_DECIMATION_FACTOR_V4 :
            SDmod |= VIDn_SDMOD_VDEC_V4;
            break;

/*        case STVID_DECIMATION_FACTOR_NONE :*/
        default :
            SDmod |= VIDn_SDMOD_VDEC_NONE;
            break;
    }

    if ( (ScanType == STVID_SCAN_TYPE_PROGRESSIVE) && (V_Decimation != STVID_DECIMATION_FACTOR_NONE) )
    {
        SDmod |= (VIDn_MOD_PS_MASK << VIDn_MOD_PS_EMP);
    }
    else
    {
        SDmod &= ~(VIDn_MOD_PS_MASK << VIDn_MOD_PS_EMP);
    }

    /* enable decode decimation */
    SDmod |= (VIDn_MOD_EN_MASK << VIDn_MOD_EN_EMP);

    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_SDMOD, SDmod);

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
    HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD,
                         VIDn_DMOD_MASK,
                         VIDn_MOD_EN_MASK,
                         VIDn_MOD_EN_EMP,
                         HALDEC_ENABLE);
} /* End of EnableSecondaryDecode() function */

/*******************************************************************************
Name        : DisableHDPIPDecode
Description : Disables HDPIP decode.
Parameters  : HAL Decode manager handle, HDPIP parameters.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableHDPIPDecode(const HALDEC_Handle_t HALDecodeHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    Omega2Properties_t * Data_p =
            (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);

    /* HD-PIP special management.                           */
    /* Default state : disable.                             */
    if ( (Data_p->isHDPIPEnabled) && (!HDPIPParams_p->Enable) )
    {
        /* HD-PIP feature is activated. Forbid it.          */
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) +
                VIDn_VPP_CFG, 0);

        /* Remember its state.                              */
        Data_p->isHDPIPEnabled = FALSE;
    }

    HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD,
                         VIDn_DMOD_MASK,
                         VIDn_MOD_HPD_MASK,
                         VIDn_MOD_HPD_EMP,
                         HALDEC_DISABLE);

} /* End of DisableHDPIPDecode() function */

/*******************************************************************************
Name        : EnableHDPIPDecode
Description : Enables HDPIP decode.
Parameters  : HAL Decode manager handle
              HDPIP parameters.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p)
{
#ifdef WA_GNBvd18453
    U32 TotalInMacroBlock;
#endif /* WA_GNBvd18453 */

    Omega2Properties_t * Data_p =
            (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);

    /* HD-PIP special management.                           */
    /* Default state : disable.                             */
    if ( (!Data_p->isHDPIPEnabled) && (HDPIPParams_p->Enable) )
    {
        /* HD-PIP feature is not activated. Allow it.       */
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) +
                VIDn_VPP_CFG,
                (VIDn_VPP_CFG_EN_MASK << VIDn_VPP_CFG_EN_EMP)  | /* HD-PIP enable.   */
                /*(VIDn_VPP_CFG_FP_MASK << VIDn_VPP_CFG_FP_EMP) | *//* Force parsing.   */
                (VIDn_VPP_CFG_PAR_DEFAULT_VALUE << VIDn_VPP_CFG_PAR_EMP) |
                (VIDn_VPP_CFG_FI_MASK << VIDn_VPP_CFG_FI_EMP) );/* Interlaced decode*/

        /* Remember its state.                              */
        Data_p->isHDPIPEnabled = TRUE;
    }
    /* Also update HD-PIP parameters (each decode action).  */
    HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD,
                         VIDn_DMOD_MASK,
                         VIDn_MOD_HPD_MASK,
                         VIDn_MOD_HPD_EMP,
                         HALDEC_ENABLE);

    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) +
            VIDn_VPP_DFHT, HDPIPParams_p->WidthThreshold & VIDn_VPP_DFHT_MASK);
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) +
            VIDn_VPP_DFWT, HDPIPParams_p->HeightThreshold & VIDn_VPP_DFWT_MASK);
#ifdef WA_GNBvd18453
    if ( (Data_p->WidthInMacroBlock & 0xfffffffc) != Data_p->WidthInMacroBlock)
    {
        /* Width is not multiple of 4 Macro blocks : Modify the total number */
        /* of macro blocks(MB), and align it to the nearest lower number of  */
        /* MB multiple of 4 (e.g. for 720x480 stream input, i.e. 45x30=1350MB*/
        /* prefer the following setting : ((45x30+3) % 4) - 4 = 1348.        */
        TotalInMacroBlock = ((( Data_p->WidthInMacroBlock * Data_p->HeightInMacroBlock) + 3) & 0xfffffffc) - 4;
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
                + VIDn_DFS, TotalInMacroBlock & VIDn_DFS_MASK);
    }

#endif /* WA_GNBvd18453 */


} /* End of EnableHDPIPDecode() function */

/*******************************************************************************
Name        : FlushDecoder
Description : Flush the decoder data buffer
              Caution: sends a SC, which may help the decoder to decode the last
              picture in buffer when flushing. Choice of the SC: SEQUENCE_END_CODE.
              Problem: this doesn't allow decoder to go on with the next coming data, as it says: end of sequence !
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void FlushDecoder(const HALDEC_Handle_t HALDecodeHandle, BOOL * const DiscontinuityStartCodeInserted_p)
{
    U32 k;
    U32 Tmp32;
    U8  Tmp8;
    #define FLUSH_SIZE 256 /* On Omega2 chips, CD-FIFO are 256 bytes wide !!! */

    Tmp8 = HAL_Read8((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG_8);

    /* For ES: we want to inject 0xAFAFAFAF000001B6 + 0xFF * FLUSH_SIZE
       For PES: we want to inject 0x000001E000nn800000AFAFAF000001B6 + 0xFF * FLUSH_SIZE, where nn is the size of the packet (byte after nn until last 0xFF)
                                    \__ PES HEADER __/\__data that will be seen in bit buffer__/
       Note: we inject two more 0xAF in ES and one more 0xAF in PES compared to the minimum, this is for alignment on U32 for better understanding. */
    if ((Tmp8 & VIDn_PES_CFG_SS_SYSTEM) != 0)
    {
        /* System stream: inject PES header before injection data to flush, otherwise data will be filtered and lost with no flush ! */
        SwapBytesIntoLong(Tmp32, (U32)(0x00000100 | 0xE0));
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDI_32, Tmp32);
        /* On 2 bytes: size of data in PES packet in bytes. On 3 bytes: end of PES info */
        SwapBytesIntoLong(Tmp32, (((U32)(3 + 7 + FLUSH_SIZE)) << 16) | ((U32)(0x8000)));
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDI_32, Tmp32);
        /* One byte from 3 bytes of end of PES data + start by inserting dummy 0xAF value for the case last data was 0x000001, better have a slice than a picture... */
        SwapBytesIntoLong(Tmp32, (((U32) GREATEST_SLICE_START_CODE) | ((U32) GREATEST_SLICE_START_CODE << 8) | ((U32) GREATEST_SLICE_START_CODE << 16)));
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDI_32, Tmp32);
    }
    else
    {
        /* Start by inserting dummy 0xAF value for the case last data was 0x000001, better have a slice than a picture... */
        SwapBytesIntoLong(Tmp32, (((U32) GREATEST_SLICE_START_CODE) | ((U32) GREATEST_SLICE_START_CODE << 8) | ((U32) GREATEST_SLICE_START_CODE << 16) | ((U32) GREATEST_SLICE_START_CODE << 24)));
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDI_32, Tmp32);
    }

    /* Insert invalid start code: this will trigger start code detector. */
    SwapBytesIntoLong(Tmp32, (U32)(0x00000100 | DISCONTINUITY_START_CODE));
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDI_32, Tmp32);

    /* Flush by filling with 0xFFFFFFFF */
    for (k = 0; k < FLUSH_SIZE; k += 4)
    {
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDI_32, 0xFFFFFFFF);
    }

    /* Indicate that a Discontinuity Start Code has been inserted. */
    *DiscontinuityStartCodeInserted_p = TRUE;
} /* End of FlushDecoder() function */


/*******************************************************************************
Name        : Get16BitsHeaderData
Description : Get 16bit word of header data
Parameters  : HAL Decode manager handle
Assumptions : Header FIFO is not empty, must be checked with IsHeaderDataFIFOEmpty()
Limitations :
Returns     :
*******************************************************************************/
static U16 Get16BitsHeaderData(const HALDEC_Handle_t  HALDecodeHandle)
{
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
    U16                  Tmp16;

/*    TraceBuffer(("]"));*/
    /* Header FIFO is not empty: get data */
#ifdef WA_HEADER_FIFO_ACCESS
    Tmp16 = (U16) (HAL_Read16(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF_32) & VIDn_HDF_HDF);
    TraceBuffer(("_%04x", Tmp16));
#else /* not WA_HEADER_FIFO_ACCESS */
    Tmp16 = (U16) (HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF_32) & VIDn_HDF_HDF);
#endif /* not WA_HEADER_FIFO_ACCESS */

    /* Try to detect user data process eating data: to catch a picture SC never arriving in Get16BitsStartCode() */
    Data_p->Last32HeaderData = (Data_p->Last32HeaderData << 8) | ((U16) ((Tmp16 >> 8) & 0xFF));
    if ((Data_p->Last32HeaderData & 0xffffff00) == 0x00000100)
    {
        /* Got a start code in header data: detected user data process eating data ! */
        NewStartCode(HALDecodeHandle, (U8) (Data_p->Last32HeaderData & 0xFF));
    }
    /* Do the same with 2nd byte... */
    Data_p->Last32HeaderData = (Data_p->Last32HeaderData << 8) | ((U16) ((Tmp16     ) & 0xFF));
    if ((Data_p->Last32HeaderData & 0xffffff00) == 0x00000100)
    {
        /* Got a start code in header data: detected user data process eating data ! */
        NewStartCode(HALDecodeHandle, (U8) (Data_p->Last32HeaderData & 0xFF));
    }

/*    return((U16) (HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF_32) & VIDn_HDF_HDF));*/
    return(Tmp16);
} /* End of Get16BitsHeaderData() function */


/*******************************************************************************
Name        : Get16BitsStartCode
Description : Get 16 bit word of start code, says if start code is on MSB/LSB
Parameters  : HAL Decode manager handle, ..
Assumptions : Header FIFO is not empty, must be checked with IsHeaderDataFIFOEmpty()
Limitations :
Returns     : U16 Value from the bit buffer
*******************************************************************************/
static U16 Get16BitsStartCode(const HALDEC_Handle_t  HALDecodeHandle,
                         BOOL * const           StartCodeOnMSB_p)
{
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
    U32 Tmp32;

    while (IsHeaderDataFIFOEmpty(HALDecodeHandle))
    {
        TraceBuffer(("\r\n\r\n****HDF EMPTY !!!****\r\n"));

        /* temporary stupid wait */
        Tmp32 = 100;
        Tmp32 = ((Tmp32 * Tmp32) / Tmp32) * Tmp32;
    }

/*    TraceBuffer(("]"));*/

#ifdef WA_HEADER_FIFO_ACCESS

    if (Data_p->isStartCodeSimulated)
    {
        /* Reset Start code simulation flag. */
        Data_p->isStartCodeSimulated = FALSE;

        /* Get start code found last manual parse.  */
        Tmp32             = Data_p->StartCodeFound;
        *StartCodeOnMSB_p = Data_p->isStartCodeOnMSB;
    }
    else
    {
        /* Read */
        Tmp32 = HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF_32);
        if ( (Tmp32 >> 8) == 0x01)
        {
            /* LSB id start code. */
            *StartCodeOnMSB_p = FALSE;
        }
        else
        {
            *StartCodeOnMSB_p = TRUE;
        }
    }

#else  /* not WA_HEADER_FIFO_ACCESS */
    Tmp32 = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF_32);

#ifdef WA_GNBvd07572
    if (Data_p->SimulatedSCH)
    {
        *StartCodeOnMSB_p = Data_p->StartCodeOnMSB;
        Data_p->SimulatedSCH = FALSE;
    }
    else
    {
        *StartCodeOnMSB_p = ((Tmp32 & VIDn_HDF_SCM) != 0);
    }
#else /* WA_GNBvd07572 */
    *StartCodeOnMSB_p = ((Tmp32 & VIDn_HDF_SCM) != 0);
#endif /* not WA_GNBvd07572 */
#endif /* not WA_HEADER_FIFO_ACCESS */

    /* Put 16 bits value in 16 LSB (mask and shift) */
    Tmp32 = (Tmp32 & (VIDn_HDF_HDF_MASK << VIDn_HDF_HDF_EMP)) >> VIDn_HDF_HDF_EMP;

    /* Update to current start code. Problem: we miss all start codes following GOP's
     because of manual header data process ! So we read them in Get16BitsHeaderData(). */
    if (*StartCodeOnMSB_p)
    {
        /* Start code on MSB of the 16 LSB's */
        NewStartCode(HALDecodeHandle, (U8) ((Tmp32 & 0xFF00) >> 8));
        /* Memorize the last 8 bits got from header fifo.   */
        /* And "reset" the other bits (in term of start code*/
        /* detection : set it to "FF..."                    */
        Data_p->Last32HeaderData |= (0xFFFFFF00 | (Tmp32 & 0xFF));
    }
    else
    {
        /* Start code on LSB */
        NewStartCode(HALDecodeHandle, (U8) (Tmp32 & 0xFF));
        /* Clear header data for start code detection */
        Data_p->Last32HeaderData = 0xFFFFFFFF; /* Init with one byte not 0 in LSB */
    }
    return((U16) Tmp32);
} /* End of Get16BitsStartCode() function */


/*******************************************************************************
Name        : Get32BitsHeaderData
Description : Get 32bit word of header data
Parameters  : HAL Decode manager handle
Assumptions : Header FIFO is not empty, must be checked with IsHeaderDataFIFOEmpty()
Limitations :
Returns     :
*******************************************************************************/
static U32 Get32BitsHeaderData(const HALDEC_Handle_t  HALDecodeHandle)
{
    U32 Tmp32 = 0;

/*    TraceBuffer(("]"));*/
    /* Header FIFO is not empty: get data */
#ifdef WA_HEADER_FIFO_ACCESS
    Tmp32 = ((HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF_32) & VIDn_HDF_HDF) << 16);
    Tmp32 |= (HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF_32) & VIDn_HDF_HDF);
    TraceBuffer(("\r\nHDF32->%08x", Tmp32));
#else /* not WA_HEADER_FIFO_ACCESS */
    Tmp32 = ((HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF_32) & VIDn_HDF_HDF) << 16);
    Tmp32 |= (HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_HDF_32) & VIDn_HDF_HDF);
#endif /* not WA_HEADER_FIFO_ACCESS */

    /* Set Last32HeaderData: may be used in Next_start_code() */
    ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->Last32HeaderData= Tmp32;

    return(Tmp32);
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
    U32 MissingStatus;
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
#if defined WA_GNBvd07572
    U16 Tmp16;
#endif /* WA_GNBvd07572 */
#if defined WA_GNBvd07572 || defined WA_PID_MISSING || defined STVID_HARDWARE_ERROR_EVENT
    U32 Level = GetDecodeBitBufferLevel(HALDecodeHandle);
    U32 HWStatus = GetStatus(HALDecodeHandle);
#endif /* WA_GNBvd07572 || WA_PID_MISSING || STVID_HARDWARE_ERROR_EVENT */
#ifdef WA_HEADER_FIFO_ACCESS
    BOOL isStartCodeFound;
#endif

/*    TraceBuffer(("\r\nAbnormaleHwStatus..."));*/
#if defined TRACE_UART && defined TRACE_MULTI_DECODE
/*    TraceBufferDecoderNumber(HALDecodeHandle);*/
#endif /* TRACE_UART  && TRACE_MULTI_DECODE*/

#ifdef USE_MULTI_DECODE_SW_CONTROLLER
    if (Data_p->MultiDecodeStatus == MULTI_DECODE_STATUS_FINISHED_NOT_POSTED)
    {
#if defined TRACE_UART && defined TRACE_MULTI_DECODE
        TraceBufferDecoderNumber(HALDecodeHandle);
#endif /* TRACE_UART && TRACE_MULTI_DECODE */
/*        TraceBuffer(("DecFinishA"));*/
        Data_p->MultiDecodeStatus = MULTI_DECODE_STATUS_IDLE;
        VIDDEC_MultiDecodeFinished(((HALDEC_Properties_t *) HALDecodeHandle)->MultiDecodeHandle);
    }
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */

/*    if ((HWStatus & HALDEC_BIT_BUFFER_NEARLY_FULL_MASK) != 0)
    {
        if (!(Data_p->NotifiedBBFInterrupt))
        {
            Data_p->MissingHWStatus |= HALDEC_BIT_BUFFER_NEARLY_FULL_MASK;
            Data_p->NotifiedBBFInterrupt = TRUE;
        }
    }
    else
    {
        Data_p->NotifiedBBFInterrupt = FALSE;
    }*/

#ifdef WA_HEADER_FIFO_ACCESS
    if (Data_p->isStartCodeSimulated)
    {
        /* Perform a pooling to catch next start code.  */
        Next_start_code (HALDecodeHandle, &isStartCodeFound,
            &Data_p->isStartCodeOnMSB, &Data_p->StartCodeFound);

        if (isStartCodeFound)
        {
            Data_p->MissingHWStatus |= HALDEC_START_CODE_HIT_MASK;
            TraceBuffer(("- SCH simu !"));
        }
        else
        {
            TraceBuffer(("- SC not found in header fifo !!!"));
        }
    }
#endif /* WA_HEADER_FIFO_ACCESS */

#if defined WA_GNBvd07572
    if ((!(Data_p->StartCodeDetectorIdle)) && /* Still expecting SCH */
        ((HWStatus & HALDEC_START_CODE_HIT_MASK) == 0) && /* Not SCH in status */
        (Data_p->LastStartCode != SEQUENCE_END_CODE) /* Don't simulate SCH if expecting first sequence ! */
       )
    {
        /* Looking for a start code (but not first one) */
        if ((Level == 0) || (IsHeaderDataFIFOEmpty(HALDecodeHandle))) /* Not if no data in HDF */
        {
            /* Bit buffer empty for decoder or SC detector: wait more with full timer ... */
            Data_p->MaxStartCodeSearchTime = time_plus(time_now(), MAX_START_CODE_SEARCH_TIME);
        }
        else
        {
            /* Start code detector not empty: simulate SCH if time has gone. */
/*            if (Level != Data_p->LastBitBufferLevelWhenSimulatingSCH)*/
            if (time_after(time_now(), Data_p->MaxStartCodeSearchTime) != 0) /* time gone ! */
            {
                /* Start code search was launched since too long: there must be a problem ! */
/*                Data_p->LastBitBufferLevelWhenSimulatingSCH = Level;*/
                /* Add HALDEC_START_CODE_HIT_MASK to next status */
                Data_p->MissingHWStatus |= HALDEC_START_CODE_HIT_MASK;
                TraceBuffer(("\r\n**SCH simu !**"));
                Data_p->SimulatedSCH = TRUE;
                /* Must have check IsHeaderDataFIFOEmpty() before calling Get16BitsHeaderData ! */
                Tmp16 = Get16BitsHeaderData(HALDecodeHandle);
                if (Tmp16 == 0)
                {
                    Data_p->StartCodeOnMSB = FALSE;
                }
                else
                {
                    Data_p->StartCodeOnMSB = TRUE;
                }
            }
        } /* Bit buffer not empty */
    } /* Conditions of the work-around */
#endif /* WA_GNBvd07572 */
#if defined WA_PID_MISSING || defined STVID_HARDWARE_ERROR_EVENT
    if ((!(Data_p->DecoderIdle)) &&
        (time_after(time_now(), Data_p->MaxDecodeTime) != 0) &&
        ((HWStatus & HALDEC_PIPELINE_IDLE_MASK) == 0)
       )
    {
        /* Decode/skip was launched since too long: there must be a problem ! */
        if (Level == 0)
        {
            /* Bit buffer empty: don't simulate PID interrupt, wait more... */
            /* Could divide time by 2 for fields !!! */
            ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MaxDecodeTime =
                time_plus(time_now(), MAX_DECODE_SKIP_TIME *
                (HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DFS_16) & VIDn_DFS_MASK) /
                MAX_SIZE_IN_MACROBLOCKS);
        }
        else
        {
#if 0
            TraceBuffer(("\r\n**MissingPID:STA=0x%x,TIS=0x%x,INS1=0x%x,INS2=0x%x", HWStatus,
            HAL_Read16(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS_16) & VIDn_TIS_MASK,
            HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) - 0x200 + 0x180), /* test register */
            HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) - 0x200 + 0x184)  /* test register */
            ));
#endif /* 0 */
#if defined WA_GNBvd07112
            TraceBuffer(("\r\n**MissingPID:STA=0x%x,HideDue=%c", HWStatus,
            (Data_p->HasConcealedDUE?'Y':'N')
            ));
#endif /* WA_GNBvd07112 */
            /* Inform upper layer we want to simulate PID: but not many times if BBL is not changing */
            if (Level != Data_p->LastBitBufferLevelWhenSimulatingPID)
            {
                Data_p->LastBitBufferLevelWhenSimulatingPID = Level;

#ifdef WA_PID_MISSING
#if (WA_PID_MISSING == 1)
                Data_p->MissingHWStatus |= HALDEC_PIPELINE_IDLE_MASK;
                TraceBuffer(("**PIDsimu **"));
#if defined WA_GNBvd06350
                Data_p->MissingHWStatus |= HALDEC_DECODER_IDLE_MASK;
#endif /* WA_GNBvd06350 */
#else /* (WA_PID_MISSING == 1) */
                TraceBuffer(("**overtime %d**",
                time_minus(time_now(), ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MaxDecodeTime)
                ));
#if defined WA_PID_MISSING && (WA_PID_MISSING == 2)
                PipelineReset(HALDecodeHandle);
                /* Twice ? */
                PipelineReset(HALDecodeHandle);
#endif /* (WA_PID_MISSING == 2) */
#endif
#endif /* WA_PID_MISSING */

#ifdef STVID_HARDWARE_ERROR_EVENT
                /* Hardware event traces activated, Store PID missing occured.  */
                Data_p->DetectedHardwareError |= STVID_HARDWARE_ERROR_MISSING_PID;
#endif /* STVID_HARDWARE_ERROR_EVENT */
            }
        }
    }
#endif
#ifdef WA_DONT_LAUNCH_SC_WHEN_BBL_LOW
    if (Level > WA_DONT_LAUNCH_SC_WHEN_BBL_LOW_THRESHOLD)
    {
        /* !!! Caution: should use same params as when requested by user ! */
        SearchNextStartCode(HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);
    }
#endif /* WA_DONT_LAUNCH_SC_WHEN_BBL_LOW */

    /* Process interrupt status internally */
    InternalProcessInterruptStatus(HALDecodeHandle, Data_p->MissingHWStatus);

    /* Dummy store to clear data */
    MissingStatus = Data_p->MissingHWStatus;
    Data_p->MissingHWStatus = 0;

    return(MissingStatus);
} /* End of GetAbnormallyMissingInterruptStatus() function */

/*******************************************************************************
Name        : GetDataInputBufferParams
Description : not used in 70xx where injection is not controlled by video
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
Name        : GetDecodeSkipConstraint
Description : We ask the HAL which is its possible action.
Parameters  : IN :HAL decode handle,
              OUT : possible Action.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void GetDecodeSkipConstraint(const HALDEC_Handle_t HALDecodeHandle, HALDEC_DecodeSkipConstraint_t * const HALDecodeSkipConstraint_p)
{
    Omega2Properties_t * Data_p;

    Data_p = (Omega2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    *HALDecodeSkipConstraint_p = Data_p->DecodeSkipConstraint;
} /* End of GetDecodeSkipConstraint() function */


/*******************************************************************************
Name        : GetBitBufferOutputCounter
Description : Gets the valu of SDC_count register
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : 32bit word OutputCounter value in units of bytes
*******************************************************************************/
static U32 GetBitBufferOutputCounter(const HALDEC_Handle_t HALDecodeHandle)
{
    return(((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->LatchedSCD32bitValue);
} /* End of GetBitBufferOutputCounter() function */


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
    *CDFifoAlignment_p = 256;
    return(ST_NO_ERROR);
} /* End of GetCDFifoAlignmentInBytes() function */


/*******************************************************************************
Name        : GetDecodeBitBufferLevel
Description : Get decoder hardware bit buffer level
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Level in bytes, with granularity of 256 bytes
*******************************************************************************/
static U32 GetDecodeBitBufferLevel(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 Level;
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
#ifdef WA_BBL_CALCULATED_WITH_VLD_SC_POINTER
    U32 CDWPtr1;
    U32 SCDRPtr1;
    U32 VLDRPtr1;
    U32 CDWPtr2;
    U32 SCDRPtr2;
    U32 VLDRPtr2;
#endif /* WA_BBL_CALCULATED_WITH_VLD_SC_POINTER */
#if defined WA_READ_BBL_TWICE && !defined WA_BBL_CALCULATED_WITH_VLD_SC_POINTER
    U32 Level2;
#endif /* WA_READ_BBL_TWICE */

#ifdef WA_BBL_CALCULATED_WITH_VLD_SC_POINTER

    do
    {
        CDWPtr1   = STSYS_ReadRegDev32LE(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDWPTR_32);
        SCDRPtr1  = STSYS_ReadRegDev32LE(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCDRPTR_32);
        VLDRPtr1  = STSYS_ReadRegDev32LE(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDRPTR_32);
        CDWPtr2   = STSYS_ReadRegDev32LE(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDWPTR_32);
        SCDRPtr2  = STSYS_ReadRegDev32LE(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCDRPTR_32);
        VLDRPtr2  = STSYS_ReadRegDev32LE(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDRPTR_32);
    } while ((CDWPtr1   != CDWPtr2)  ||
             (SCDRPtr1  != SCDRPtr2) ||
             (VLDRPtr1  != VLDRPtr2) );

    if (!Data_p->IsAnyDecodeLaunchedSinceLastResetOrSkip)
    {
        /* We're waiting for the very first decode. */
        if(SCDRPtr1 <= CDWPtr1)
            Level = CDWPtr1 - SCDRPtr1;
        else
            Level = CDWPtr1 + Data_p->BitBufferSize
            - SCDRPtr1;

/*        TraceBuffer(("\r\n1-CDW=%08x, SCR=%08x, VLD=%08x -> BBL=%d", CDWPtr1, SCDRPtr1, VLDRPtr1, Level));*/
    }
    else
    {
#ifdef WA_GNBvd16407
        /* Test if the WA_GNBvd16407 is in progress.    */
        if (Data_p->NumberOfUselessPSD != 0)
        {
            /* Yes, so don't use VLDRPTR to calculate bit buffer level, but last picture start code position.*/
            VLDRPtr1 = (U32) (Data_p->LastSlicePictureSCAddress);
        }
#endif /* WA_GNBvd16407 */

        if(VLDRPtr1 <= CDWPtr1)
            Level = CDWPtr1 - VLDRPtr1;
        else
            Level = CDWPtr1 + Data_p->BitBufferSize - VLDRPtr1;
/*        TraceBuffer(("\r\n2-CDWPTR=%08x, SCRPTR=%08x, VLDRPTR=%08x, level=%d", CDWPtr1, SCDRPtr1, VLDRPtr1, Level));*/
    }

#else /* WA_BBL_CALCULATED_WITH_VLD_SC_POINTER */

#ifdef WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE
    if (!Data-p->IsAnyDecodeLaunchedSinceLastReset)
    {
        /* As level is invalid before first decode, let think it is 0 */
        return(0);
    }
#endif /* WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE */

    /* No need to divide by 256 because of the structure of the register */
    Level = HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32) & VIDn_BB_MASK;
#ifdef WA_READ_BBL_TWICE
    /* BBL values can be sometimes completely wrong. But two consecutive 'good' reads should be almost equals, that's how we will detect them ! */
    Level2 = HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32) & VIDn_BB_MASK;
    while (DistanceU32(Level, Level2) > READ_BBL_TWICE_THRESHOLD)
    {
        /* Values should not differ too much. If so: re-read until good ! */
        Level = HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32) & VIDn_BB_MASK;
        Level2 = HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32) & VIDn_BB_MASK;
    }
#endif /* WA_READ_BBL_TWICE */

/*    TraceBuffer(("-%2d%%,BBL=%d", (Level) * 100 /
        ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->BitBufferSize,
        Level));*/

#endif /* WA_BBL_CALCULATED_WITH_VLD_SC_POINTER */

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
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
    STVID_HardwareError_t HwErrorStatus;

    /* status Protection (begin)        */
    interrupt_lock();
    /* Get the hardware error status.   */
    HwErrorStatus = Data_p->DetectedHardwareError;
    /* Reset it.                        */
    Data_p->DetectedHardwareError = STVID_HARDWARE_NO_ERROR_DETECTED;
    /* status Protection (end)          */
    interrupt_unlock();
    /* and then, return it.             */
    return(HwErrorStatus);

} /* End of GetHardwareErrorStatus() function */
#endif /* STVID_HARDWARE_ERROR_EVENT  */

/*******************************************************************************
Name        : GetInterruptStatus
Description : Get decoder Interrupt status
Parameters  : HAL Decode manager handle
Assumptions : This is called inside interrupt context
Limitations : Beware that this function is called under interrupt context.
              Should stay indie the shortest time as possible !
Returns     : Decoder status
*******************************************************************************/
static U32 GetInterruptStatus(const HALDEC_Handle_t  HALDecodeHandle)
{
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
    U32 HWStatus;
    U32 Mask;
#ifdef WA_GNBvd16407
    U32 Tis;
#endif /* WA_GNBvd16407 */

    Mask = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_ITM) & VIDn_ITS_MASK;

    /* Prevent from loosing interrupts ! */
    SetInterruptMask(HALDecodeHandle, 0);

    HWStatus = (U32) (HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_ITS_16) & VIDn_ITS_MASK);

    /* Release video interrupts ! */
    SetInterruptMask(HALDecodeHandle, Mask);

#if defined TRACE_UART && defined TRACE_MULTI_DECODE
    if ((HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == ((HALDEC_Properties_t *) HALDecodeHandle)->DecoderNumber) ||
        (HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == HALDEC_MULTI_DECODE_TRACE_DECODER_ALL))
    {
    if (HWStatus != HALDEC_START_CODE_HIT_MASK)
    {
        /* Trace if not only SCH */
        TraceBufferDecoderNumber(HALDecodeHandle);
    }
    }
#endif /* TRACE_UART && TRACE_MULTI_DECODE */

#ifdef WA_GNBvd16407
    if ((HWStatus & HALDEC_PIPELINE_START_DECODING_MASK) != 0)
    {
        /* Test if a field/frame change has been detected.  */
        if (Data_p->NumberOfUselessPSD != 0)
        {
            /* Decrement the counter.                       */
            Data_p->NumberOfUselessPSD --;
            if (Data_p->NumberOfUselessPSD == 0)
            {
                /* Last PSD interrupt. Reset PID and DID status bits.       */
                HWStatus &= ~(HALDEC_PIPELINE_IDLE_MASK | HALDEC_DECODER_IDLE_MASK);
            }
            else
            {
                TraceBuffer(("-Cancel PSD!**"));
                PipelineReset (HALDecodeHandle);
                Tis = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
                        + VIDn_TIS) & VIDn_TIS_MASK;
                Tis = Tis | VIDn_TIS_EXE | VIDn_TIS_FIS;
                HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
                        + VIDn_TIS, Tis);


                /* Last pipereset and decode launch action. Restore the DMod and SDMod registers values.    */
                PipelineReset (HALDecodeHandle);
                HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD,
                        Data_p->InitialSDmod32RegisterValue);
                HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD,
                        Data_p->InitialDmod32RegisterValue);

                Data_p->NumberOfUselessPSD = 1;

                /* Last PSD interrupt. Reset PID and DID status bits.       */
                HWStatus &= ~(HALDEC_PIPELINE_START_DECODING_MASK | HALDEC_PIPELINE_IDLE_MASK | HALDEC_DECODER_IDLE_MASK);

                Tis = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
                        + VIDn_TIS) & VIDn_TIS_MASK;
                Tis = Tis | VIDn_TIS_EXE | VIDn_TIS_FIS;
                HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
                        + VIDn_TIS, Tis);
            }
        }
    }
#endif /* WA_GNBvd16407 */
#if defined TRACE_UART
#if defined TRACE_MULTI_DECODE
    if ((HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == ((HALDEC_Properties_t *) HALDecodeHandle)->DecoderNumber) ||
        (HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == HALDEC_MULTI_DECODE_TRACE_DECODER_ALL))
    {
#endif /* TRACE_MULTI_DECODE */
    if ((HWStatus & HALDEC_PIPELINE_START_DECODING_MASK) != 0)
    {
        TraceBuffer(("-PSD"));
    }
#if defined TRACE_MULTI_DECODE
    }
#endif /* TRACE_MULTI_DECODE */
#endif /* TRACE_UART */
#if defined WA_PID_MISSING || defined STVID_HARDWARE_ERROR_EVENT
    if ((HWStatus & HALDEC_PIPELINE_START_DECODING_MASK) != 0)
    {
        /* Re-load timer for better precision: avoid thinking that PID missing occurs,
        when it is not, but due to SW overhead, we obtain a to long decode time. */
        ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MaxDecodeTime =
            time_plus(time_now(), MAX_DECODE_SKIP_TIME *
            (HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DFS_16) & VIDn_DFS_MASK) /
            MAX_SIZE_IN_MACROBLOCKS);
    }
#endif /* WA_PID_MISSING or STVID_HARDWARE_ERROR_EVENT */
#if defined WA_GNBvd06249 || defined WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE
    if ( ((HWStatus & HALDEC_PIPELINE_START_DECODING_MASK) != 0) && (!Data_p->IsAnyDecodeLaunchedSinceLastReset) )
    {
        Data_p->IsAnyDecodeLaunchedSinceLastReset = TRUE;
    }
#endif /* WA_GNBvd06249 or WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE */
#if defined WA_BBL_CALCULATED_WITH_VLD_SC_POINTER
    if (!Data_p->IsAnyDecodeLaunchedSinceLastResetOrSkip)
    {
        if ((HWStatus & HALDEC_PIPELINE_START_DECODING_MASK) != 0)
        {
            Data_p->IsAnyDecodeLaunchedSinceLastResetOrSkip = TRUE;
        }
    }

#endif /* WA_BBL_CALCULATED_WITH_VLD_SC_POINTER */


    /* Change masks according to HAL */
#ifdef WA_GNBvd07112
    /* HALDEC_DECODING_UNDERFLOW_MASK may appear without reasons ! */
    if ((HWStatus & HALDEC_DECODING_UNDERFLOW_MASK) != 0)
    {
        HWStatus &= ~ HALDEC_DECODING_UNDERFLOW_MASK;
        Data_p->HasConcealedDUE = TRUE;
        TraceBuffer(("-Cancel DUE!**"));
    }
#endif /* WA_GNBvd07112 */

#if defined WA_GNBvd06350
    if ((HWStatus & HALDEC_PIPELINE_IDLE_MASK) != 0)
    {
        /* Simulate DEI as it may not appear ! */
        HWStatus |= HALDEC_DECODER_IDLE_MASK;
    }
#endif /* WA_GNBvd06350 */

#ifdef TRACE_UART
    /* Trace bit buffer level in IT */
/*    {
        U32 Level;
        U8 Percent;
        Level = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32);
        Percent = (Level & VIDn_BB_MASK) * 100 / Data_p->BitBufferSize;
        if ((Percent == 0) && (Level != 0))
        {
            Percent = 1;
        }
        TraceBuffer(("-%2d%%", Percent));
    }*/
#endif /* TRACE_UART */

#ifdef STVID_HARDWARE_ERROR_EVENT
    if ((HWStatus & HALDEC_DECODING_SYNTAX_ERROR_MASK) != 0)
    {
        /* Hardware event traces activated, Store PID missing occured.  */
        Data_p->DetectedHardwareError |= STVID_HARDWARE_ERROR_SYNTAX_ERROR;
    }
#endif /* STVID_HARDWARE_ERROR_EVENT */

#ifdef TRACE_UART
    if ((HWStatus & HALDEC_DECODING_SYNTAX_ERROR_MASK) != 0)
    {
/*        HWStatus &= ~ HALDEC_DECODING_SYNTAX_ERROR_MASK;*/
        TraceBuffer(("-SYE!**"));
    }
#endif /* TRACE_UART */

    /* Not TRUE!: On STi7015, should not do pipe reset after DOE: just consider it as SYE for simplification ??? */
    if ((HWStatus & HALDEC_DECODING_OVERFLOW_MASK) != 0)
    {
/*        HWStatus &= ~ HALDEC_DECODING_OVERFLOW_MASK;*/
/*        HWStatus |= HALDEC_DECODING_SYNTAX_ERROR_MASK;*/
#if defined TRACE_UART
/*        TraceBuffer(("-DOE>SYE!**"));*/
/*        TraceBuffer(("-DOE!**"));*/
        TraceBuffer(("-DOE!**:STA=0x%x,INS1=0x%x,INS2=0x%x",
        HAL_Read16(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STA_16) & VIDn_STA_MASK,
        HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) - 0x200 + 0x180), /* test register */
        HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) - 0x200 + 0x184)  /* test register */
        ));
#endif /* TRACE_UART */
    }
#if defined TRACE_UART
#if defined TRACE_MULTI_DECODE
    if ((HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == ((HALDEC_Properties_t *) HALDecodeHandle)->DecoderNumber) ||
        (HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == HALDEC_MULTI_DECODE_TRACE_DECODER_ALL))
    {
#endif /* TRACE_MULTI_DECODE */
    if ((HWStatus & HALDEC_PIPELINE_IDLE_MASK) != 0)
    {
        TraceBuffer(("-PII"));
    }

    if ((HWStatus & HALDEC_DECODER_IDLE_MASK) != 0)
    {
        TraceBuffer(("-DEI"));
    }
#if defined TRACE_MULTI_DECODE
    }
#endif /* TRACE_MULTI_DECODE */
#endif /* TRACE_UART */

#ifdef STVID_INJECTION_BREAK_WORKAROUND
/*    Data_p->SkipFirstStartCodeAfterSoftReset = FALSE;*/
/*    if (((HWStatus & HALDEC_START_CODE_HIT_MASK) != 0) && (Data_p->SkipFirstStartCodeAfterSoftReset))*/
/*    {*/
        /* Found first start code to skip ! */
/*        HWStatus &= ~ HALDEC_START_CODE_HIT_MASK;*/
/*        Data_p->SkipFirstStartCodeAfterSoftReset = FALSE;*/

        /* Launch SC detector again */
/*        SearchNextStartCode(HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);*/
/*    }*/
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

#if defined WA_GNBvd07572
    if (((HWStatus & HALDEC_START_CODE_HIT_MASK) != 0) && ((Data_p->MissingHWStatus & HALDEC_START_CODE_HIT_MASK) != 0))
    {
        TraceBuffer(("\r\n-Cancel SCH simu!*****"));
        /* Got SCH interrupt: cancel eventual SCH simulation */
        Data_p->MissingHWStatus &= ~HALDEC_START_CODE_HIT_MASK;
    }
#endif /* WA_GNBvd07572 */

#ifdef TRACE_UART
    if ((HWStatus & HALDEC_BIT_BUFFER_NEARLY_FULL_MASK) != 0)
    {
        TraceBuffer(("-BBF!!!***"));
#ifdef WA_GNBvd06252
        TraceBuffer(("(BBT=%d,2*SCD=%d,CD=%d)",
            HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBT_32) & 0x3FFFFFFF,
            (HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCDC_32) & 0x1FFFFFFF ) * 2,
            (HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDC_32) & 0x1FFFFFFF)
            ));
#else /* WA_GNBvd06252 */
        TraceBuffer(("(BBT=%d,2*SCD=%d,CD=%d)",
            HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBT_32) & 0x3FFFFFFF,
            (HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCDC_32) & 0x1FFFFFFF ),
            (HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDC_32) & 0x1FFFFFFF)
            ));
#endif /* WA_GNBvd06252 */
    }
#endif /* TRACE_UART */

    /* Process interrupt status internally */
    InternalProcessInterruptStatus(HALDecodeHandle, HWStatus);

    return(HWStatus);
} /* End of GetInterruptStatus() function */


/*******************************************************************************
Name        : GetLinearBitBufferTransferedDataSize
Description :
Parameters  : HAL decode handle
Assumptions : Level in bytes, with granularity of 256 bytes
Limitations : Returns     : 32bit size
*******************************************************************************/
static ST_ErrorCode_t GetLinearBitBufferTransferedDataSize(const HALDEC_Handle_t HALDecodeHandle, U32 * const LinearBitBufferTransferedDataSize_p)
{
/* HG: NOT CHECKED !!! */
    U32 Level;
#ifdef WA_READ_BBL_TWICE
    U32 Level2;
#endif /* WA_READ_BBL_TWICE */

    Level = HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32) & VIDn_BB_MASK;
#ifdef WA_READ_BBL_TWICE
    /* BBL values can be sometimes completely wrong. But two consecutive 'good' reads should be almost equals, that's how we will detect them ! */
    Level2 = HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32) & VIDn_BB_MASK;
    while (DistanceU32(Level, Level2) > READ_BBL_TWICE_THRESHOLD)
    {
        /* Values should not differ too much. If so: re-read until good ! */
        Level = HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32) & VIDn_BB_MASK;
        Level2 = HAL_Read32(((U8 *) ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32) & VIDn_BB_MASK;
    }
#endif /* WA_READ_BBL_TWICE */

    *LinearBitBufferTransferedDataSize_p = Level;

    return (ST_NO_ERROR);
} /* End of GetLinearBitBufferTransferedDataSize() function */


/*******************************************************************************
Name        : GetPTS
Description : Gets current picture time stamp information
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : TRUE : There is a time stamp available. FALSE : No time stamp available
*******************************************************************************/
static BOOL GetPTS(const HALDEC_Handle_t HALDecodeHandle,
        U32 *  const PTS_p,
        BOOL * const PTS33_p)
{
    U8  Tmp8;
    U16 Tmp16;

    if ((PTS_p == NULL) || (PTS33_p == NULL))
    {
        /* Error cases */
        return(FALSE);
    }

    /* Check if there is a PTS available */
    Tmp8 = HAL_Read8(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_ASS_8);
    if ((Tmp8 & VIDn_PES_ASS_TSA_MASK) == 0)
    {
        /* No time stamp available: quit */
        return(FALSE);
    }

    /* There is a time stamp available: copy it */
    *PTS_p = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_TS0_32);
    Tmp16 = HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_TS1_16);
    *PTS33_p = ((Tmp16 & VIDn_PES_TS1_TS_MASK) != 0);

    return(TRUE);
} /* Enf of GetPTS() function */


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
    *SCDAlignment_p = 2;
    return (ST_NO_ERROR);
} /* End of GetSCDAlignmentInBytes() function */


/*******************************************************************************
Name        : GetStartCodeAddress
Description : Get a pointer on the address of the Start Code if available
Parameters  : Decoder registers address, pointer on buffer address
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetStartCodeAddress(const HALDEC_Handle_t HALDecodeHandle, void ** const BufferAddress_p)
{
/* HG: NOT CHECKED !!! */

    if (BufferAddress_p == NULL)
    {
        /* Error cases */
        return(ST_ERROR_BAD_PARAMETER);
    }

    *BufferAddress_p = NULL;

    return(ST_NO_ERROR);
} /* End of GetStartCodeAddress() function */

/*******************************************************************************
Name        : GetStatus
Description : Get decoder status information
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Decoder status
*******************************************************************************/
static U32 GetStatus(const HALDEC_Handle_t  HALDecodeHandle)
{
    return (((U32)HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STA_16)) & VIDn_STA_MASK);
} /* End of GetStatus() function */


/*******************************************************************************
Name        : Init
Description : Initialisation of the private datas
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALDEC_Handle_t  HALDecodeHandle)
{
    Omega2Properties_t * Data_p;
    void * ConfigurationBaseAddress_p;
    U16 Tmp16;
#ifdef STVID_INJECTION_BREAK_WORKAROUND
    ST_ErrorCode_t ErrorCode;
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

    /* Allocate a Omega2Properties_t structure */
    SAFE_MEMORY_ALLOCATE(((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p, void *,
                         ((HALDEC_Properties_t *) HALDecodeHandle)->CPUPartition_p,
                         sizeof(Omega2Properties_t));
    if (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);

#ifdef STVID_INJECTION_BREAK_WORKAROUND
    /* Register injection control events */
    ErrorCode = STEVT_RegisterDeviceEvent(((HALDEC_Properties_t *) HALDecodeHandle)->EventsHandle,
                                          ((HALDEC_Properties_t *) HALDecodeHandle)->VideoName,
                                          STVID_WORKAROUND_PLEASE_STOP_INJECTION_EVT,
                                          &(Data_p->EventIDPleaseStopInjection));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(((HALDEC_Properties_t *) HALDecodeHandle)->EventsHandle,
                                              ((HALDEC_Properties_t *) HALDecodeHandle)->VideoName,
                                              STVID_WORKAROUND_THANKS_INJECTION_CAN_GO_ON_EVT,
                                              &(Data_p->EventIDThanksInjectionCanGoOn));
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Display init: could not register event !"));
        Term(HALDecodeHandle);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

    /* Invalidate SCD: 4 values to reset */
    Data_p->IsSCDValid = FALSE;
    Data_p->PreviousSCD26bitValue = 0;
/*    Data_p->PreviousSCD32bitValue  = Data_p->BitBufferBase - 8;*/
    Data_p->PreviousSCD32bitValue  = 0;
    Data_p->BitBufferSize = 0;
    Data_p->BitBufferBase = 0;
    Data_p->IsDecodingInTrickMode = FALSE;
    Data_p->LastStartCode = SEQUENCE_END_CODE;
    Data_p->Last32HeaderData = 0xFFFFFFFF; /* Init with one byte not 0 in LSB */
    Data_p->StartCodeDetectorIdle = TRUE;
    Data_p->MissingHWStatus = 0;
#if defined WA_GNBvd06350 || defined WA_GNBvd16165 || defined WA_GNBvd13097
    Data_p->LastPictureSCAddress = (void *) Data_p->BitBufferBase;
    Data_p->LastSlicePictureSCAddress = (void *) Data_p->BitBufferBase;
#endif /* WA_GNBvd06350 */
#if defined WA_GNBvd06249 || defined WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE
    Data_p->IsAnyDecodeLaunchedSinceLastReset = FALSE;
#endif /* WA_GNBvd06249 or WA_BBL_NOT_VALID_BEFORE_FIRST_DECODE */
#if defined WA_BBL_CALCULATED_WITH_VLD_SC_POINTER
    Data_p->IsAnyDecodeLaunchedSinceLastResetOrSkip = FALSE;
#endif /* WA_BBL_CALCULATED_WITH_VLD_SC_POINTER */
#if defined WA_GNBvd06249
    Data_p->LastManuallyDecodedPictureAddress_p = (void *) Data_p->BitBufferBase;
    Data_p->LastButOneSlicePictureSCAddress = (void *) Data_p->BitBufferBase;
#endif /* WA_GNBvd06249 */
#ifdef WA_GNBvd07572
    Data_p->SimulatedSCH = FALSE;
/*    Data_p->LastBitBufferLevelWhenSimulatingSCH = 0;*/
#endif /* WA_GNBvd07572 */
#if defined WA_PID_MISSING || defined WA_GNBvd07112 || defined STVID_HARDWARE_ERROR_EVENT
    Data_p->DecoderIdle = TRUE;
#endif /* WA_PID_MISSING || WA_GNBvd07112 || STVID_HARDWARE_ERROR_EVENT */
#if defined WA_GNBvd07112
    Data_p->HasConcealedDUE = FALSE;
#endif /* WA_GNBvd07112 */
#if defined WA_PID_MISSING || defined STVID_HARDWARE_ERROR_EVENT
    Data_p->LastBitBufferLevelWhenSimulatingPID = 0;
#endif /* WA_PID_MISSING || STVID_HARDWARE_ERROR_EVENT */
#ifdef WA_DONT_LAUNCH_SC_WHEN_BBL_LOW
    Data_p->StartCodeSearchPending = FALSE;
#endif /* WA_DONT_LAUNCH_SC_WHEN_BBL_LOW */
/*    Data_p->NotifiedBBFInterrupt = FALSE;*/
#ifdef WA_GNBvd08438
/*    Data_p->SkipFirstStartCodeAfterSoftReset = FALSE;*/
    Data_p->IsInjectionES = TRUE;
#endif /* WA_GNBvd08438 */

#ifdef USE_MULTI_DECODE_SW_CONTROLLER
    /* Don't write TIS: call multi-decode software controller instead ! */
    Data_p->MultiDecodeReadyParams.DecoderNumber = ((HALDEC_Properties_t *) HALDecodeHandle)->DecoderNumber;
    Data_p->MultiDecodeReadyParams.ExecuteFunction = MultiDecodeExecuteDecode;
    Data_p->MultiDecodeReadyParams.ExecuteFunctionParam = (void *) HALDecodeHandle; /* Pass HAL decode handle as parameter */
    Data_p->MultiDecodeStatus = MULTI_DECODE_STATUS_IDLE;
    Data_p->MustLaunchSCDetector = FALSE;
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */

    Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_MUST_DECODE;
    Data_p->FirstDecodeAfterSoftResetDone = FALSE;

#ifdef STVID_HARDWARE_ERROR_EVENT
    /* No hardware error detected for the moment.   */
    Data_p->DetectedHardwareError = STVID_HARDWARE_NO_ERROR_DETECTED;
#endif /* STVID_HARDWARE_ERROR_EVENT  */
    /* Relative access to registers, to know ConfigurationBaseAddress_p: substract video offset and add cfg offset */
    ConfigurationBaseAddress_p = ((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p;
    switch (((HALDEC_Properties_t *) HALDecodeHandle)->DecoderNumber)
    {
        case 0 :
            ConfigurationBaseAddress_p = (void *) (((U32) ConfigurationBaseAddress_p) - CFG_BASE_OFFSET_DECODER_0);
            break;

        case 1 :
            ConfigurationBaseAddress_p = (void *) (((U32) ConfigurationBaseAddress_p) - CFG_BASE_OFFSET_DECODER_1);
            break;

        case 2 :
            ConfigurationBaseAddress_p = (void *) (((U32) ConfigurationBaseAddress_p) - CFG_BASE_OFFSET_DECODER_2);
            break;

        case 3 :
            ConfigurationBaseAddress_p = (void *) (((U32) ConfigurationBaseAddress_p) - CFG_BASE_OFFSET_DECODER_3);
            break;

        default :
            ConfigurationBaseAddress_p = (void *) (((U32) ConfigurationBaseAddress_p) - CFG_BASE_OFFSET_DECODER_4);
            break;
    }
    ConfigurationBaseAddress_p = (void *) (((U32) ConfigurationBaseAddress_p) + CFG_OFFSET);

#if defined WA_GNBvd16407 || defined WA_GNBvd16741
    /* Remember the chip configuration base adress for futur use.       */
    Data_p->ConfigurationBaseAddress_p = ConfigurationBaseAddress_p;
#endif /* WA_GNBvd16407 || WA_GNBvd16741 */
#ifdef WA_GNBvd16407
    /* Initialize the number of PSD interrupt to considr as uselss.     */
    Data_p->NumberOfUselessPSD = 0;
#endif /* WA_GNBvd16407 */

#ifdef WA_GNBvd07112
    HAL_Write32(((U8 *) ConfigurationBaseAddress_p) + CFG_VDCTL, CFG_VDCTL_MVC | /*CFG_VDCTL_ERO | CFG_VDCTL_ERU | !!! */ CFG_VDCTL_EDC);
#else /* WA_GNBvd07112 */
    HAL_Write32(((U8 *) ConfigurationBaseAddress_p) + CFG_VDCTL, CFG_VDCTL_MVC | CFG_VDCTL_ERO | CFG_VDCTL_ERU | CFG_VDCTL_EDC);
#endif /* not WA_GNBvd07112 */

    /* Disable the VSync synchronization of decoder sate machin control.    */
    HAL_Write32(((U8 *) ConfigurationBaseAddress_p) + CFG_VSEL, 0x2);

#ifndef WA_GNBvd12779
    /* Set Decoding processes priority order to default */
    HAL_Write32(((U8 *) ConfigurationBaseAddress_p) + CFG_PORD, 0);
#else /* WA_GNBvd12779 */
    HAL_Write32(((U8 *) ConfigurationBaseAddress_p) + CFG_PORD, CFG_PORD_MAD_MASK | CFG_PORD_MMD_MASK);
#endif /* WA_GNBvd12779 */

    /* Force manual priority (default order) */
    /* because of a bug which prevent use of automatic mode */
    /*    stvid_Write32(CFG_BASE_ADDRESS + CFG_DECPR, CFG_DECPR_FP); */
    HAL_Write32(((U8 *) ConfigurationBaseAddress_p) + CFG_DECPR, 0);

    /* Ensure good initialisation */
    /* PBO must not be set because otherwise in real-time it leads to a loose of data. Morover hw bug DDTSGNBvd15841    */
    /* 'PBO bit stops the CD FIFO but not the PES Parser' gives disruption in PTS values.                               */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STP, 0);
#if defined DISABLE_MAIN_DECODE_WHEN_USING_SECONDARY
    /* Disable reconstruction in main buffers. */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD, 0);
#else /* DISABLE_MAIN_DECODE_WHEN_USING_SECONDARY */
    /* Enable reconstruction in main buffers. */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD, (VIDn_MOD_EN_MASK << VIDn_MOD_EN_EMP));
#endif /* not DISABLE_MAIN_DECODE_WHEN_USING_SECONDARY */
    /* Disable reconstruction in secondary buffers. */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD, 0);

    /* Useless !!! */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG, 0);
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_FSI, 0);
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_SI, 0);
        /* Set TFI to 0xFF when not in trick modes ! */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_TFI, 0xFF);

    /* Reset compressed data only from host interface.  */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDR_32, 0);
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_CDWL_32, (VIDn_CDWL_MASK << VIDn_CDWL_EMP));

    /* HD-PIP special management.                       */
    /* Default state : disable.                         */
    Data_p->isHDPIPEnabled = FALSE;

    /* Set interrupt mask to 0: disable interrupts */
    SetInterruptMask(HALDecodeHandle, 0);

    /* Clear pending interrupts. */
    Tmp16 = HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_ITS_16);

    return(ST_NO_ERROR);
} /* End of Init() function */


/*******************************************************************************
Name        : IsHeaderDataFIFOEmpty
Description : Tells whether the header data FIFO is empty
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : TRUE is empty, FALSE if not empty
*******************************************************************************/
static BOOL IsHeaderDataFIFOEmpty(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 DecodeStatus;

    DecodeStatus = GetStatus(HALDecodeHandle);
    if ((DecodeStatus & HALDEC_HEADER_FIFO_EMPTY_MASK) != 0)
    {
        TraceBuffer(("#_#"));
       return (TRUE);
    }

/*    TraceBuffer(("["));*/
    return (FALSE);
} /* End of IsHeaderDataFIFOEmpty() function */


/*******************************************************************************
Name        : LoadIntraQuantMatrix
Description : Load quantization matrix for intra picture decoding
Parameters  : HAL Decode manager handle, pointer on matrix values
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LoadIntraQuantMatrix(
                         const HALDEC_Handle_t   HALDecodeHandle,
                         const QuantiserMatrix_t *Matrix_p,
                         const QuantiserMatrix_t *ChromaMatrix_p)
{
    U32 i, Coef;

    #ifdef DEBUG
    if (Matrix_p == NULL)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */

    if (Matrix_p != NULL)
    {
        for (i = 0; i < 64; i += 4)
        {
            Coef  = (U32) (*Matrix_p)[i    ];
            Coef |= (U32) (*Matrix_p)[i + 1] <<  8;
            Coef |= (U32) (*Matrix_p)[i + 2] << 16;
            Coef |= (U32) (*Matrix_p)[i + 3] << 24;
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_QMWI_32 + i, Coef);
        }
    }

    /* The 7020 maintains 1 bit wich record wether the IntraMatrix has been modified */
    /* Shall we enable this bit by ourselves ? */
} /* End of LoadIntraQuantMatrix() function */


/*******************************************************************************
Name        : LoadNonIntraQuantMatrix
Description : Load quantization matrix for non-intra picture decoding
Parameters  : HAL Decode manager handle, pointer on matrix values
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LoadNonIntraQuantMatrix(
                         const HALDEC_Handle_t   HALDecodeHandle,
                         const QuantiserMatrix_t *Matrix_p,
                         const QuantiserMatrix_t *ChromaMatrix_p)
{
    U32 i, Coef;

    #ifdef DEBUG
    if (Matrix_p == NULL)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */

    if (Matrix_p != NULL)
    {
        for (i = 0; i < 64; i += 4)
        {
            Coef  = (U32) (*Matrix_p)[i    ];
            Coef |= (U32) (*Matrix_p)[i + 1] <<  8;
            Coef |= (U32) (*Matrix_p)[i + 2] << 16;
            Coef |= (U32) (*Matrix_p)[i + 3] << 24;
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_QMWNI_32 + i, Coef);
        }
    }

    /* The 7020 maintains 1 bit wich record wether the non IntraMatrix has been modified */
    /* Shall we enable this bit by ourselves ? */
} /* End of LoadNonIntraQuantMatrix() function */


#ifdef USE_MULTI_DECODE_SW_CONTROLLER
/*******************************************************************************
Name        : MultiDecodeExecuteDecode
Description : Executes decode (called by multi-decode software controller)
Parameters  : param is HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void MultiDecodeExecuteDecode(void * const ExecuteParam_p)
{
    ((Omega2Properties_t *) (((HALDEC_Properties_t *) ExecuteParam_p)->PrivateData_p))->MultiDecodeStatus = MULTI_DECODE_STATUS_STARTED_NOT_FINISHED;

    HAL_Write32((U8 *) (((HALDEC_Properties_t *) ExecuteParam_p)->RegistersBaseAddress_p) + VIDn_TIS,
        ((Omega2Properties_t *) (((HALDEC_Properties_t *) ExecuteParam_p)->PrivateData_p))->MultiDecodeExecutionTis);

    if (((Omega2Properties_t *) (((HALDEC_Properties_t *) ExecuteParam_p)->PrivateData_p))->MustLaunchSCDetector)
    {
        /* Launch start code search together with decode. */
        ((Omega2Properties_t *) (((HALDEC_Properties_t *) ExecuteParam_p)->PrivateData_p))->MustLaunchSCDetector = FALSE;
        SearchNextStartCode((HALDEC_Handle_t) ExecuteParam_p, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);
    }

} /* end of MultiDecodeExecuteDecode() function */
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */


/*******************************************************************************
Name        : PipelineReset
Description : Pipeline Reset
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PipelineReset(const HALDEC_Handle_t  HALDecodeHandle)
{
#ifdef WA_GNBvd16741
    U32 Tmp32;
#endif
#if defined WA_GNBvd16407 || defined WA_GNBvd16741
    Omega2Properties_t * Data_p;
    Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
#endif
#if defined WA_GNBvd16407
    /* Initialize the number of PSD interrupt to considr as uselss. */
    Data_p->NumberOfUselessPSD = 0;
#endif /* WA_GNBvd16407 */

    TraceBuffer(("-PipeReset **"));

#ifndef WA_GNBvd16741
    /* Writing to the least significant byte of this register starts the pipeline reset  */
    /* for the corresponding video channel. The reset is just activated once on writing. */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PRS, 0xFF);
#else /* WA_GNBvd16741 */
    /* - stop the decoder (write 0 to CFG_VDCTL.EDC)        */
    Tmp32 = HAL_Read32(((U8 *) Data_p->ConfigurationBaseAddress_p) + CFG_VDCTL);
    Tmp32 &= ~CFG_VDCTL_EDC;
    HAL_Write32(((U8 *) Data_p->ConfigurationBaseAddress_p) + CFG_VDCTL, Tmp32);
    /* - wait for 10 us                                     */
    STTBX_WaitMicroseconds(10);
    /* - generate the pipe reset                            */

    /* Writing to the least significant byte of this register starts the pipeline reset  */
    /* for the corresponding video channel. The reset is just activated once on writing. */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PRS, 0xFF);
    /* - reenable the decoder (write 1 to CFG_VDCTL.EDC)    */
    Tmp32 |= CFG_VDCTL_EDC;
    HAL_Write32(((U8 *) Data_p->ConfigurationBaseAddress_p) + CFG_VDCTL, Tmp32);

#endif /* WA_GNBvd16741 */

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
#if defined WA_GNBvd06249 || defined WA_GNBvd13097
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
#endif /* WA_GNBvd06249 || WA_GNBvd13097 */
    void * CorrectDecodeAddressFrom_p;
    U32 Tis;
#if defined WA_GNBvd06249
    U32 AdrToReg;
    void * MaxLastPictureAddress_p;
#endif /* WA_GNBvd06249 */

    /* If decode launched from address, VLD is already ready to decode. */
    *WaitForVLD_p = FALSE;

    /* By default, DecodeAddressFrom_p is correct (inside bit buffer).          */
    CorrectDecodeAddressFrom_p = DecodeAddressFrom_p;

#if defined WA_GNBvd13097
    /* Test the address of the picture (DecodeAddressFrom_p).                   */
    /* It could be outside bit buffer.                                          */
    if ( (S32)(DecodeAddressFrom_p) < (S32)(Data_p->BitBufferBase) )
    {
        /* The address is out of bit buffer area : just add bit buffer size.    */
        CorrectDecodeAddressFrom_p = (void *) ((S32)((S32)(DecodeAddressFrom_p) + (S32)(Data_p->BitBufferSize)));
    }
#endif /* WA_GNBvd13097 */

    /* write Address in VLDPTR : VLD Start Pointer. Memory address pointer, in byte, starting
     point of decoding when VIDn_TIS.LDP and VIDn_TIS.LFR are both set. */
#if defined WA_GNBvd06249
    /* Not first decode: program address minus margin. But care of previous
    picture address because of small pictures which cause wrong picture decode */
    /* Previous picture start code address should be LastManuallyDecodedPictureAddress_p,
    but if a decode was skipped, it could be LastButOneSlicePictureSCAddress ! */
    if (!Data_p->IsAnyDecodeLaunchedSinceLastReset)
    {
        /* First decode: program margin normally, but care of bit buffer base */
        if((S32)((U32) CorrectDecodeAddressFrom_p - WA_GNBvd06249_SCD_ACCURACY_MARGIN) >= (S32)(Data_p->BitBufferBase))
        {
            /* First decode somewhere in bit buffer higher than margin: program margin normally */
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, (U32) CorrectDecodeAddressFrom_p - WA_GNBvd06249_SCD_ACCURACY_MARGIN);
        }
        else
        {
            /* First decode at very beginning of bit buffer: program bit buffer base instead of margin because we don't know what's in the bit buffer above ! */
#ifdef WA_GNBvd08438
            if (!(Data_p->IsInjectionES))
            {
                HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, Data_p->BitBufferBase + GNBvd08438_DUMMY_PES_ES_SIZE);
            }
            else
            {
                HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, Data_p->BitBufferBase);
            }
#else /* WA_GNBvd08438 */
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, Data_p->BitBufferBase);
#endif /* not WA_GNBvd08438 */
        }
    } /* End first decode */
    else
    {
        /* Not first decode: care about small pictures. */
        MaxLastPictureAddress_p = Max(Data_p->LastManuallyDecodedPictureAddress_p, Data_p->LastButOneSlicePictureSCAddress);
        if (((U32)CorrectDecodeAddressFrom_p < (U32)(MaxLastPictureAddress_p)) &&
                ((U32)CorrectDecodeAddressFrom_p + Data_p->BitBufferSize - (U32)(MaxLastPictureAddress_p) < WA_GNBvd06249_SCD_ACCURACY_MARGIN))
        {
            /* Case where CorrectDecodeAddressFrom_p and MaxLastPictureAddress_p are less far
            than WA_GNBvd06249_SCD_ACCURACY_MARGIN from each other, but MaxLastPictureAddress_p
            is above at the other side of the bit buffer (because of bit buffer wrap-around) */
#ifdef STVID_HARDWARE_ERROR_EVENT
            /* Hardware event traces activated, Store too small picture occured.    */
            Data_p->DetectedHardwareError |= STVID_HARDWARE_ERROR_TOO_SMALL_PICTURE;
#endif /* STVID_HARDWARE_ERROR_EVENT */

            AdrToReg = (U32)(MaxLastPictureAddress_p) + 1;
            if (AdrToReg > Data_p->BitBufferBase + Data_p->BitBufferSize)
            {
                AdrToReg -= Data_p->BitBufferSize;
            }
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, AdrToReg);
        }
        else if (((U32)CorrectDecodeAddressFrom_p > (U32)(MaxLastPictureAddress_p)) &&
                ((S32)((U32)CorrectDecodeAddressFrom_p - (U32)(MaxLastPictureAddress_p)) < (S32)(WA_GNBvd06249_SCD_ACCURACY_MARGIN)))
        {
            /* Case where CorrectDecodeAddressFrom_p and MaxLastPictureAddress_p are less far
            than WA_GNBvd06249_SCD_ACCURACY_MARGIN from each other, both in same area of bit buffer */
#ifdef STVID_HARDWARE_ERROR_EVENT
            /* Hardware event traces activated, Store too small picture occured.    */
            Data_p->DetectedHardwareError |= STVID_HARDWARE_ERROR_TOO_SMALL_PICTURE;
#endif /* STVID_HARDWARE_ERROR_EVENT */
            AdrToReg = (U32)(MaxLastPictureAddress_p) + 1;
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, AdrToReg);
        }
        else if ((U32)CorrectDecodeAddressFrom_p == (U32)(MaxLastPictureAddress_p))
        {
            /* Case where CorrectDecodeAddressFrom_p and MaxLastPictureAddress_p are equal: don't
            jump negative otherwise we will decode previous picture */
#ifdef STVID_HARDWARE_ERROR_EVENT
            /* Hardware event traces activated, Store too small picture occured.    */
            Data_p->DetectedHardwareError |= STVID_HARDWARE_ERROR_TOO_SMALL_PICTURE;
#endif /* STVID_HARDWARE_ERROR_EVENT */
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, (U32)CorrectDecodeAddressFrom_p);
        }
        else
        {
            /* Not in the case of small pictures: normal margin, but care of bit buffer loop */
            if((S32)((U32) CorrectDecodeAddressFrom_p - WA_GNBvd06249_SCD_ACCURACY_MARGIN) >= (S32)(Data_p->BitBufferBase))
            {
                /* CorrectDecodeAddressFrom_p - WA_GNBvd06249_SCD_ACCURACY_MARGIN more than BBG: program margin normally */
                HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, (U32) CorrectDecodeAddressFrom_p - WA_GNBvd06249_SCD_ACCURACY_MARGIN);
            }
            else
            {
                /* CorrectDecodeAddressFrom_p - WA_GNBvd06249_SCD_ACCURACY_MARGIN less than BBG: must loop */
                HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, (U32) CorrectDecodeAddressFrom_p - WA_GNBvd06249_SCD_ACCURACY_MARGIN + Data_p->BitBufferSize);
            }
        } /* end not small picture */
    } /* end not first decode */
#else /* WA_GNBvd06249 */
    /* Give to the VLD the address of the picture. */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, (U32) CorrectDecodeAddressFrom_p);
#endif /* not WA_GNBvd06249 */

/*    TraceBuffer(("-wriVLDPRT=0x%x", HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32)));*/

    /* configure VIDn_TIS register so that next start code is searched from BufferAddress_p */
    Tis = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_TIS) & VIDn_TIS_MASK;
    Tis &= ~ VIDn_TIS_EXE;
    /* LDP : load pointer. When this bit is set, the decoding of the picture will start from a location specified by a pointer. */
    /* LFR : load from register. When this bit is set, the decoding start-pointer used when VIDn_TIS.LDP is set is taken from register */
    /* VIDn_VLDPTR. Otherwise the start will be done according to value automatically computed by start code detector */
    /* and available in VIDn_PSCPTR register. */
    Tis |= VIDn_TIS_LDP | VIDn_TIS_LFR;

/*    TraceBuffer(("-TISprep0x%x", Tis));*/

    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS, Tis);

#if defined WA_GNBvd06249
    /* remember the address where this decode has been launched from */
    Data_p->LastManuallyDecodedPictureAddress_p = CorrectDecodeAddressFrom_p;
#endif /* WA_GNBvd06249 */

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
    U32 Tmp32;


    Tmp32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_PES_CFG);

    /* Setting this bit to 0 resets the PES parser. Then it must be put back 1 for non-ES data */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_PES_CFG, Tmp32 & ~(VIDn_PES_CFG_SS_SYSTEM));

    STTBX_WaitMicroseconds(1); /* Needed ? */

    /* Write back value: to restore SS configuration. */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_PES_CFG, Tmp32);
} /* End of ResetPESParser() function */


/*******************************************************************************
Name        : SearchNextStartCode
Description : Launch HW search for next start code.
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SearchNextStartCode(
                         const HALDEC_Handle_t               HALDecodeHandle,
                         const HALDEC_StartCodeSearchMode_t  SearchMode,
                         const BOOL                          FirstSliceDetect,
                         void * const                        SearchAddressFrom_p)
{
#ifdef WA_HEADER_FIFO_ACCESS
    Omega2Properties_t * Data_p = (Omega2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;
#endif /* WA_HEADER_FIFO_ACCESS */

#ifdef WA_DONT_LAUNCH_SC_WHEN_BBL_LOW
    U32 Level = GetDecodeBitBufferLevel(HALDecodeHandle);
#endif /* WA_DONT_LAUNCH_SC_WHEN_BBL_LOW */
#ifdef WA_GNBvd07572
    ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MaxStartCodeSearchTime =
            time_plus(time_now(), MAX_START_CODE_SEARCH_TIME);
#endif /* WA_GNBvd07572 */
    ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->StartCodeDetectorIdle = FALSE;

/*    TraceBuffer(("-GoSC"));*/

#ifdef WA_HEADER_FIFO_ACCESS
    if (Data_p->isPictureStartCodeDetected)
    {
        /* After PSC (Picture Starts Code) stage. parse manually    */
        /* header fifo in order to get start code...                */

        /* Remember to simulate a new start code hit.               */
        Data_p->isStartCodeSimulated = TRUE;
        TraceBuffer(("\r\nSearchNextSC cancelled"));
        /* exit and do not launch a start code search.              */
        return;
    }
#endif /* WA_HEADER_FIFO_ACCESS */

#ifdef WA_DONT_LAUNCH_SC_WHEN_BBL_LOW
    if (Level < WA_DONT_LAUNCH_SC_WHEN_BBL_LOW_THRESHOLD)
    {
        ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->StartCodeSearchPending = TRUE;
        return;
    }
    ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->StartCodeSearchPending = FALSE;
#endif /* WA_DONT_LAUNCH_SC_WHEN_BBL_LOW */

    if (FirstSliceDetect)
    {
       HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STP,
                         VIDn_STP_MASK,
                         VIDn_STP_SSC_MASK,
                         VIDn_STP_SSC_EMP,
                         HALDEC_ENABLE);
    }
    else
    {
       HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STP,
                         VIDn_STP_MASK,
                         VIDn_STP_SSC_MASK,
                         VIDn_STP_SSC_EMP,
                         HALDEC_DISABLE);
    }

   switch(SearchMode)
   {
       case HALDEC_START_CODE_SEARCH_MODE_NORMAL :
           HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
                + VIDn_HLNC, VIDn_HLNC_SC_SEARCH_NORMALLY_LAUNCHED);
           break;

       case HALDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS :
           HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)  + VIDn_SCDPTR_32, (U32)(SearchAddressFrom_p) & VIDn_SCDPTR_MASK);
           HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)  + VIDn_HLNC, VIDn_HLNC_SC_SEARCH_FROM_ADDR_LAUNCHED);
           break;

       default :
           #ifdef DEBUG
           HALDEC_Error();
           #endif /* DEBUG */
           break;
   }

} /* End of SearchNextStartCode() function */


/*******************************************************************************
Name        : SetBackwardChromaFrameBuffer
Description : Set decoder backward reference chroma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBackwardChromaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p)
{
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~VIDn_FP_MASK)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */
    /* Beginning of the chroma frame buffer where to decode, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BCHP_32, (U32) BufferAddress_p);
} /* End of SetBackwardChromaFrameBuffer() function */


/*******************************************************************************
Name        : SetBackwardLumaFrameBuffer
Description : Set decoder backward reference luma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBackwardLumaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p)
{
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~VIDn_FP_MASK)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */
    /* Beginning of the luma frame buffer where to decode, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BFP_32, (U32) BufferAddress_p);
} /* End of SetBackwardLumaFrameBuffer() function */

/*******************************************************************************
Name        : SetDataInputInterface
Description : not used in 70xx where injection is not controlled by video
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
Parameters  : HAL Decode manager handle, buffer address, buffer size in bytes
Assumptions : Address_p and size must be aligned on 256 bits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeBitBuffer(
                         const HALDEC_Handle_t          HALDecodeHandle,
                         void * const                   BufferAddressStart_p,
                         const U32                      BufferSize,
                         const HALDEC_BitBufferType_t   BufferType)
{
/* HG: NOT CHECKED !!! */
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);

    #ifdef DEBUG
    /* ensure that it is 256 bytes aligned */
    if ((((U32) BufferAddressStart_p) & (~VIDn_BB_MASK)) || (BufferSize & (~VIDn_BB_MASK)))
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */

    switch(BufferType)
    {

        case HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR :
            /* Disable bit TM in Video Setup register : when set it indicates that hardware manages circularity. */
            HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STP,
                             VIDn_STP_MASK,
                             VIDn_STP_TM_MASK,
                             VIDn_STP_TM_EMP,
                             (U8)(HALDEC_DISABLE));
            Data_p->IsDecodingInTrickMode = FALSE;
            break;
        case HALDEC_BIT_BUFFER_HW_MANAGED_LINEAR :
        case HALDEC_BIT_BUFFER_SW_MANAGED_CIRCULAR :
            /* Features not supported by the software */
#ifdef DEBUG
            HALDEC_Error();
#endif /* DEBUG */
            break;
        case HALDEC_BIT_BUFFER_SW_MANAGED_LINEAR :
            /* Enable bit TM in Video Setup register */
            HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STP,
                             VIDn_STP_MASK,
                             VIDn_STP_TM_MASK,
                             VIDn_STP_TM_EMP,
                             (U8)(HALDEC_ENABLE));
            Data_p->IsDecodingInTrickMode = TRUE;
            break;

        default:
            break;
    }

    /* Beginning of the bit buffer, in unit of 256 bytes */
    /* No need to divide by 256 because of the register  */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBG_32, (U32) BufferAddressStart_p);
    /* End of the bit buffer, in unit of 256 bytes */
    /* No need to divide by 256 because of the register */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBS_32, (U32) BufferAddressStart_p + BufferSize - 256);

    Data_p->BitBufferBase = (U32) BufferAddressStart_p;
    Data_p->BitBufferSize = BufferSize;

    /* On STi7015, need to soft reset so that HW takes it into account ! */
    DecodingSoftReset(HALDecodeHandle, TRUE/* Dummy value*/);

} /* End of SetDecodeBitBuffer() function */


/*******************************************************************************
Name        : SetDecodeBitBufferThreshold
Description : Set the threshold of the decoder bit buffer causing interrupt
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeBitBufferThreshold(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         const U32              OccupancyThreshold)
{
    U32 Threshold = OccupancyThreshold;
    #ifdef DEBUG
    /* ensure that it is 256 bytes aligned */
    if ((OccupancyThreshold) & (~VIDn_BB_MASK))
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */

    /* Threshold of the bit buffer, in unit of 256 bytes */
    /* No need to divide by 256 because of the structure of the register        */
#if defined WA_DONT_FILL_BIT_BUFFER
    if (OccupancyThreshold > (((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->BitBufferSize - WA_DONT_FILL_BIT_BUFFER_MINIMUM_BBT_MARGIN))
    {
        Threshold = (((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->BitBufferSize -
                       WA_DONT_FILL_BIT_BUFFER_MINIMUM_BBT_MARGIN);
    }
#endif /* WA_DONT_FILL_BIT_BUFFER */
#if defined WA_GNBvd06249
    /* Ensure that threshold is not smaller than WA_GNBvd06249_SCD_ACCURACY_MARGIN, to avoid overwrite of data not yet eaten by the decoder. */
    if (Threshold > (((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->BitBufferSize - WA_GNBvd06249_SCD_ACCURACY_MARGIN))
    {
        Threshold = ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->BitBufferSize - WA_GNBvd06249_SCD_ACCURACY_MARGIN;
    }
#endif /* WA_GNBvd06249 */

    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBT_32, Threshold);

} /* End of SetDecodeBitBufferThreshold() function */


/*******************************************************************************
Name        : SetDecodeChromaFrameBuffer
Description : Set decoder chroma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions : 7015: BufferAddress_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeChromaFrameBuffer(const HALDEC_Handle_t  HALDecodeHandle,
                                       void * const           BufferAddress_p)
{
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~VIDn_FP_MASK)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */
    /* Beginning of the chroma frame buffer where to decode, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_RCHP_32, (U32) BufferAddress_p);
} /* End of SetDecodeChromaFrameBuffer() function */


/*******************************************************************************
Name        : SetDecodeConfiguration
Description :
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetDecodeConfiguration(
                         const HALDEC_Handle_t      HALDecodeHandle,
                         const StreamInfoForDecode_t * const StreamInfo_p)
{
    const PictureCodingExtension_t * PicCodExt_p = &(StreamInfo_p->PictureCodingExtension);
    const Picture_t * Pic_p = &(StreamInfo_p->Picture);
    U32 WidthInMacroBlock, HeightInMacroBlock;
    U32 TotalInMacroBlock;
    U32 Dmod, SDmod, Tis;
    U32 TmpPpr32 = 0;
    Omega2Properties_t * Data_p = (Omega2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

    /* Hardware constraint : after decoding a field, we must decode a 2nd field to keep the right polarity. */
    if ((!(StreamInfo_p->MPEG1BitStream)) && (PicCodExt_p->picture_structure != PICTURE_STRUCTURE_FRAME_PICTURE))
    {
#ifdef WA_GNBvd16407
        /* The next decode will be a field. */
        Data_p->IsNextDecodeField = TRUE;
#endif /* WA_GNBvd16407 */
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
#ifdef WA_GNBvd16407
        /* The next decode won't be a field.    */
        Data_p->IsNextDecodeField = FALSE;
#endif /* WA_GNBvd16407 */
        Data_p->DecodeSkipConstraint = HALDEC_DECODE_SKIP_CONSTRAINT_NONE;
    }

#ifdef USE_MULTI_DECODE_SW_CONTROLLER
    if (((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MultiDecodeStatus == MULTI_DECODE_STATUS_FINISHED_NOT_POSTED)
    {
#if defined TRACE_UART && defined TRACE_MULTI_DECODE
/*        TraceBufferDecoderNumber(HALDecodeHandle);*/
#endif /* TRACE_UART && TRACE_MULTI_DECODE */
/*        TraceBuffer(("DecFinishB"));*/
        ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MultiDecodeStatus = MULTI_DECODE_STATUS_IDLE;
        VIDDEC_MultiDecodeFinished(((HALDEC_Properties_t *) HALDecodeHandle)->MultiDecodeHandle);
    }
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */

    /* Write width in macroblocks */
    WidthInMacroBlock = ((U32)StreamInfo_p->HorizontalSize + 15) / 16;

    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_DFW, WidthInMacroBlock & VIDn_DFW_MASK);

    /* Write height in macroblocks */
    HeightInMacroBlock = ((U32)StreamInfo_p->VerticalSize + 15) / 16;
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_DFH, HeightInMacroBlock & VIDn_DFH_MASK);

    /* Write total number of macroblocks */
    TotalInMacroBlock = HeightInMacroBlock * WidthInMacroBlock;

#ifdef WA_GNBvd18453
    /* Just remember */
    Data_p->WidthInMacroBlock   = WidthInMacroBlock;
    Data_p->HeightInMacroBlock  = HeightInMacroBlock;
#endif /* WA_GNBvd18453 */

    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_DFS, TotalInMacroBlock & VIDn_DFS_MASK);

    /* Same for MPEG1/MPEG2 : two least Significant bits of Coding Type */
    TmpPpr32 = ((Pic_p->picture_coding_type & VIDn_PPR_PCT_MASK) << VIDn_PPR_PCT_EMP);

    /* HD-PIP frame coding types management.    */
    if ( (Pic_p->picture_coding_type == PICTURE_CODING_TYPE_I) ||
         (Pic_p->picture_coding_type == PICTURE_CODING_TYPE_P) )
    {
        TmpPpr32 |= (1 << VIDn_PPR_ULO_EMP);
    }

        TraceBuffer(("\r\nPrep(%c%d)", (Pic_p->picture_coding_type == 3?'B':((Pic_p->picture_coding_type == 2?'P':'I'))),
                             StreamInfo_p->Picture.temporal_reference));

    /* Read registers to be updated. */
    Tis     = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS) & VIDn_TIS_MASK;
    Dmod    = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD) & VIDn_DMOD_MASK;
    SDmod   = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD) & VIDn_SDMOD_MASK;

    Tis &= ~ VIDn_TIS_EXE;

    /* Depending on MPEG1/MPEG2 */
    if (StreamInfo_p->MPEG1BitStream)
    {
        /* Setup for MPEG1 streams */
        Tis &= ~ (VIDn_TIS_MP2 | VIDn_TIS_DPR); /* MPEG-1 decode */

        /* Progressive scan type is expected. */
        if (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType == STVID_DEVICE_TYPE_7015_MPEG)
        {
            /* MPEG1 only support progressive scan type. */
            Dmod |= (VIDn_MOD_PS_MASK << VIDn_MOD_PS_EMP);
            SDmod |= (VIDn_MOD_PS_MASK << VIDn_MOD_PS_EMP);
        }
        /* Setup for MPEG1 streams */
        TmpPpr32 |= (((Pic_p->backward_f_code) & VIDn_PPR_BFH_MPG1_BFC_MASK) << VIDn_PPR_BFH_EMP);
        TmpPpr32 |= (((Pic_p->forward_f_code) & VIDn_PPR_FFH_MPG1_FFC_MASK) << VIDn_PPR_FFH_EMP);

        if (Pic_p->full_pel_backward_vector)
        {
            TmpPpr32 |= (U32) (1 << VIDn_PPR_BFH_MPG1_FPBV_EMP);
        }
        if (Pic_p->full_pel_forward_vector)
        {
            TmpPpr32 |= (U32) (1 << VIDn_PPR_FFH_MPG1_FPFV_EMP);
        }

#ifdef USE_MULTI_DECODE_SW_CONTROLLER
        ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MultiDecodeReadyParams.DecodeScanType = STGXOBJ_PROGRESSIVE_SCAN;
        ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MultiDecodeReadyParams.DecodePictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */
    } /* end MPEG-1 */
    else
    {
        /* Setup for MPEG-2 streams */
        Tis |= VIDn_TIS_MP2; /* MPEG-2 decode */
        if ((PicCodExt_p->picture_structure & 3) != PICTURE_STRUCTURE_FRAME_PICTURE)
        {
            Tis |= VIDn_TIS_DPR; /* field picture */
#ifdef USE_MULTI_DECODE_SW_CONTROLLER
            ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MultiDecodeReadyParams.DecodePictureStructure = STVID_PICTURE_STRUCTURE_TOP_FIELD; /* top or bottom doesn't care */
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */
        }
        else
        {
            Tis &= ~VIDn_TIS_DPR; /* frame picture */
            /* Should treat case of 60Hz progressive as field !!! */
#ifdef USE_MULTI_DECODE_SW_CONTROLLER
            ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MultiDecodeReadyParams.DecodePictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */
        }

        /* Check for the scan type of the decoded picture (contained in picture coding extension). */
#ifndef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
        if (PicCodExt_p->progressive_frame)
#else /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */
        /* Work around activated. Test if the picture to decode is progressive and small enough */
        /* to be considered as progressive.                                                     */
        if ( (PicCodExt_p->progressive_frame) && (StreamInfo_p->VerticalSize < FIRST_PROGRESSIVE_HEIGHT_TO_MANAGE_AS_INTERLACED) )
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */
        {
            /* Progressive scan type is expected. */
            if (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType == STVID_DEVICE_TYPE_7015_MPEG)
            {
                Dmod |= (VIDn_MOD_PS_MASK << VIDn_MOD_PS_EMP);
            }
            SDmod |= (VIDn_MOD_PS_MASK << VIDn_MOD_PS_EMP);
#ifdef USE_MULTI_DECODE_SW_CONTROLLER
            ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MultiDecodeReadyParams.DecodeScanType = STGXOBJ_PROGRESSIVE_SCAN;
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */
        }
        else
        {
            /* Interlaced scan type is expected. */
            Dmod &= ~(VIDn_MOD_PS_MASK << VIDn_MOD_PS_EMP);
            SDmod &= ~(VIDn_MOD_PS_MASK << VIDn_MOD_PS_EMP);
#ifdef USE_MULTI_DECODE_SW_CONTROLLER
            ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MultiDecodeReadyParams.DecodeScanType = STGXOBJ_INTERLACED_SCAN;
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */
        }

        if (PicCodExt_p->intra_dc_precision != 0x3)
        {
            TmpPpr32 |= (U32) (PicCodExt_p->intra_dc_precision & VIDn_PPR_DCP_MASK) << VIDn_PPR_DCP_EMP;
        }
        #ifdef DEBUG
        else  HALDEC_Error();
        #endif /* DEBUG */

        TmpPpr32 |= (U32) (PicCodExt_p->picture_structure & VIDn_PPR_PST_MASK) << VIDn_PPR_PST_EMP;

        if (PicCodExt_p->top_field_first)
        {
            TmpPpr32 |= (U32) (1 << VIDn_PPR_TFF_EMP);
        }
        if (PicCodExt_p->frame_pred_frame_dct)
        {
            TmpPpr32 |= (U32) (1 << VIDn_PPR_FRM_EMP);
        }
        if (PicCodExt_p->concealment_motion_vectors)
        {
            TmpPpr32 |= (U32) (1 << VIDn_PPR_CMV_EMP);
        }
        if (PicCodExt_p->q_scale_type)
        {
            TmpPpr32 |= (U32) (1 << VIDn_PPR_QST_EMP);
        }
        if (PicCodExt_p->intra_vlc_format)
        {
            TmpPpr32 |= (U32) (1 << VIDn_PPR_IVF_EMP);
        }
        if (PicCodExt_p->alternate_scan)
        {
            TmpPpr32 |= (U32) (1 << VIDn_PPR_AZZ_EMP);
        }

        TmpPpr32 |= ((U32)(PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL]) & VIDn_PPR_BFH_MPG2_BHFC_MASK) << VIDn_PPR_BFH_EMP;
        TmpPpr32 |= ((U32)(PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL]) & VIDn_PPR_FFH_MPG2_FHFC_MASK) << VIDn_PPR_FFH_EMP;

        TmpPpr32 |= ((U32)(PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL]) & VIDn_PPR_BFV_MASK) << VIDn_PPR_BFV_EMP;
        TmpPpr32 |= ((U32)(PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL]) & VIDn_PPR_FFV_MASK) << VIDn_PPR_FFV_EMP;
    } /* end MPEG-2 */

    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD, Dmod);
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD, SDmod);
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS, Tis);

    /* Write configuration */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PPR_32, TmpPpr32);

/*    TraceBuffer(("*PrepareDEC:DFW=0x%x,DFH=0x%x,DFS=0x%x,PPR=0x%x",*/
/*    WidthInMacroBlock & VIDn_DFW_MASK,*/
/*    HeightInMacroBlock & VIDn_DFH_MASK,*/
/*    TotalInMacroBlock & VIDn_DFS_MASK,*/
/*    TmpPpr32));*/

    /* temporal reference ?: only B-on-the-fly */
} /* End of SetDecodeConfiguration() function */


/*******************************************************************************
Name        : SetDecodeLumaFrameBuffer
Description : Set decoder luma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions : 7015: BufferAddress_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeLumaFrameBuffer(const HALDEC_Handle_t  HALDecodeHandle,
                                     void * const           BufferAddress_p)
{
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~VIDn_FP_MASK)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */
    /* Beginning of the luma frame buffer where to decode, in unit of 512 bytes */
    /* No need to divide by 512 because of the structure of the register        */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_RFP_32, (U32) BufferAddress_p);

/*    TraceBuffer(("*,RFP=0x%x", (U32) BufferAddress_p));*/


} /* End of SetDecodeLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetForwardChromaFrameBuffer
Description : Set decoder forward reference chroma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetForwardChromaFrameBuffer(
                         HALDEC_Handle_t  HALDecodeHandle,
                         void * const     BufferAddress_p)
{
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~VIDn_FP_MASK)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */
    /* Beginning of the chroma frame buffer where to decode, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_FCHP_32, (U32) BufferAddress_p);
} /* End of SetForwardChromaFrameBuffer() function */

/*******************************************************************************
Name        : SetForwardLumaFrameBuffer
Description : Set decoder forward reference luma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetForwardLumaFrameBuffer(
                         HALDEC_Handle_t  HALDecodeHandle,
                         void * const     BufferAddress_p)
{
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~VIDn_FP_MASK)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */
    /* Beginning of the luma frame buffer where to decode, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_FFP_32, (U32) BufferAddress_p);
} /* End of SetForwardLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetFoundStartCode
Description : Used for the Pipe Mis-alignment detection.
Parameters  : HAL decode handle, StartCode found
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetFoundStartCode(const HALDEC_Handle_t HALDecodeHandle, const U8 StartCode, BOOL * const FirstPictureStartCodeFound_p)
{
    *FirstPictureStartCodeFound_p = FALSE;
    /* Feature not supported */
} /* End of SetFoundStartCode() function */


/*******************************************************************************
Name        : SetInterruptMask
Description : Set Interrupt Mask
Parameters  : HAL Decode manager handle, Mask to be written
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetInterruptMask(const HALDEC_Handle_t  HALDecodeHandle,
                             const U32              Mask)
{
    #ifdef DEBUG
    if (Mask & (~(U32)VIDn_ITM_MASK))
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_ITM, (Mask & VIDn_ITM_MASK));
} /* End of SetInterruptMask() function */


/*******************************************************************************
Name        : SetMainDecodeCompression
Description : Set main decode compression
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetMainDecodeCompression(
                         const HALDEC_Handle_t           HALDecodeHandle,
                         const STVID_CompressionLevel_t  Compression)
{
    U32 Dmod;

    if (((HALDEC_Properties_t *) HALDecodeHandle)->DeviceType == STVID_DEVICE_TYPE_7015_MPEG)
    {
        Dmod = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD) & VIDn_DMOD_MASK;

        Dmod &= ~(VIDn_DMOD_COMP_MASK);

        switch (Compression)
        {
            case STVID_COMPRESSION_LEVEL_1 :
                Dmod |= VIDn_DMOD_COMP_HQ;
                break;

            case STVID_COMPRESSION_LEVEL_2 :
                Dmod |= VIDn_DMOD_COMP_SQ;
                break;

    /*        case STVID_COMPRESSION_LEVEL_NONE :*/
            default :
                Dmod |= VIDn_DMOD_COMP_NONE;
                break;
        }

        HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD, Dmod);
    }

} /* End of SetMainDecodeCompression() function */


/*******************************************************************************
Name        : SetSecondaryDecodeChromaFrameBuffer
Description : Set secondary decoder chroma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions : 7015: BufferAddress_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryDecodeChromaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p)
{
/* HG: NOT CHECKED !!! */
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~VIDn_FP_MASK)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */
    /* Beginning of the chroma frame buffer where to decode, in unit of 512 bytes */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SRCHP_32, (U32) BufferAddress_p);
} /* End of SetSecondaryDecodeChromaFrameBuffer() function */


/*******************************************************************************
Name        : SetSecondaryDecodeLumaFrameBuffer
Description : Set secondary decode luma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions : 7015: BufferAddress_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryDecodeLumaFrameBuffer(const HALDEC_Handle_t  HALDecodeHandle,
                                              void * const           BufferAddress_p)
{
/* HG: NOT CHECKED !!! */
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~VIDn_FP_MASK)
    {
        HALDEC_Error();
    }
    #endif /* DEBUG */
    /* Beginning of the luma frame buffer where to decode, in unit of 512 bytes */
    /* No need to divide by 512 because of the structure of the register        */
    HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SRFP_32, (U32) BufferAddress_p);
} /* End of SetSecondaryDecodeLumaFrameBuffer() function */


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
    Omega2Properties_t * Data_p = (Omega2Properties_t *) ((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p;

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
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
} /* End of SetSmoothBackwardConfiguration() function */


/*******************************************************************************
Name        : SetStreamID
Description : Sets the stream ID(s) to be filtered
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetStreamID(
                         const HALDEC_Handle_t HALDecodeHandle,
                         const U8              StreamID)
{
    /* The PES parser filters data and transmit them when the following equation is true  */
    /* (for video decoders) : (Stream ID & VIDn_PES_FSI) = (VIDn_PES_SI & VIDn_PES_FSI) */

    /* 0xE0 to 0xEF are video streams ID */
    /* if Stream Id == Ignore, then 0xE0 in FSI and 0xFF in SI. Thus, All the Video stream will de filtered */
    /* else 0xFF in FSI and Stream ID in SI */

    if (StreamID == STVID_IGNORE_ID)
    {
        /* Filter all the video streams */
/*        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_FSI, 0xE0);*/
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_FSI, 0xF0);
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_SI, 0xE0);
        /* Set TFI to 0xFF when not in trick modes ! */
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_TFI, 0xFF);
    }
    else
    {
        /* Set the stream ID */
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_SI, (StreamID|0xE0));
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_FSI, 0xFF);
        /* Set TFI to 0xFF when not in trick modes ! */
        HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_TFI, 0xFF);
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
static void SetStreamType(
                         const HALDEC_Handle_t    HALDecodeHandle,
                         const STVID_StreamType_t StreamType)
{
    /* MODE[1:0]: Mode. Selects which kind of stream is expected. */
        /* 00 : automatic configuration. Depending on the syntax of the stream, MPEG2 PES stream or MPEG1 SYSTEM stream is selected. */
        /* 01 : MPEG1 system stream expected */
        /* 10 : MPEG2 PES stream expected */
    /* OFS[1:0] : Output Format Selection. This field selects which kind of bistream is transmitted to the video bit buffer and decoder. */
        /* 00 : PES stream transmitted */
        /* 01 : Elementary Stream transmitted, Stream ID and Sub-Stream ID layers removed (mode reserved for stream beeing encoded with private SubStream ID) */
        /* 11 : Elementary Stream transmitted, Stream ID layer removed */
    /* SS : System Stream. This bit, when set, switches on the parser and a system stream (MPEG-2 video PES stream or MPEG-2  */
    /* program stream) can be decoded. When not set, the parser is disabled and pure video ele-mentary streams only can be decoded. */

    U32 Tmp32;

    Tmp32 = HAL_Read32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG);
    Tmp32 = Tmp32 & (~(((U32)VIDn_PES_CFG_SS_MASK << VIDn_PES_CFG_SS_EMP) |
            ((U32)VIDn_PES_CFG_MODE_MASK << VIDn_PES_CFG_MODE_EMP) | ((U32)VIDn_PES_CFG_OFS_MASK << VIDn_PES_CFG_OFS_EMP)));

    switch(StreamType)
    {
        case STVID_STREAM_TYPE_ES :
            /* SS = 0, MOD = 0, OFS = 3 */
            Tmp32 = Tmp32 | (VIDn_PES_CFG_MODE_AUTO | VIDn_PES_CFG_OFS_ES);
#ifdef WA_GNBvd08438
            ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->IsInjectionES = TRUE;
#endif /* WA_GNBvd08438 */
            break;

        case STVID_STREAM_TYPE_PES :
            /* SS = 1, MOD = 2, OFS = 3 */
            Tmp32 = Tmp32 | (VIDn_PES_CFG_MODE_MP2 | VIDn_PES_CFG_OFS_ES | VIDn_PES_CFG_SS_SYSTEM);
#ifdef WA_GNBvd08438
            ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->IsInjectionES = FALSE;
#endif /* WA_GNBvd08438 */
            break;

        case STVID_STREAM_TYPE_MPEG1_PACKET :
            /* SS = 1, MOD = 1, OFS = 3 */
            Tmp32 = Tmp32 | (VIDn_PES_CFG_MODE_MP1 | VIDn_PES_CFG_OFS_ES | VIDn_PES_CFG_SS_SYSTEM);
#ifdef WA_GNBvd08438
            ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->IsInjectionES = FALSE;
#endif /* WA_GNBvd08438 */
            break;

/*        case PROGRAM : !!! NOT USED */
            /* SS = 1, MOD = 3 */
/*            break;*/

        default :
            #ifdef DEBUG
            HALDEC_Error();
            #endif /* DEBUG */
            break;
    }

    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PES_CFG, Tmp32);
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
Description : Skip one picture in incoming data
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SkipPicture(const HALDEC_Handle_t           HALDecodeHandle,
                        const BOOL                      OnlyOnNextVsync,
                        const HALDEC_SkipPictureMode_t  SkipMode)
{
#if defined WA_GNBvd06350

    ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MissingHWStatus |= HALDEC_PIPELINE_IDLE_MASK;
    ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MissingHWStatus |= HALDEC_DECODER_IDLE_MASK;

#ifdef WA_BBL_CALCULATED_WITH_VLD_SC_POINTER
    ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->IsAnyDecodeLaunchedSinceLastResetOrSkip
            = FALSE;
#endif

#if defined TRACE_UART && defined TRACE_MULTI_DECODE
    TraceBufferDecoderNumber(HALDecodeHandle);
#endif /* TRACE_UART && TRACE_MULTI_DECODE */

    TraceBuffer(("-HSkip/simuPID"));

    return;

#else /* WA_GNBvd06350 */
/* HG: NOT CHECKED !!! */
    U32 Tis;

    TraceBuffer(("-HSkip/skipTIS"));
    /* configure VIDn_TIS register to launch decode */
    Tis = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_TIS) & VIDn_TIS_MASK;
    Tis &= ~VIDn_TIS_SKP;
    Tis |= VIDn_TIS_EXE;

    switch (SkipMode)
    {
        case HALDEC_SKIP_PICTURE_1_THEN_DECODE :
            Tis |= SKIP_ONE_DECODE_NEXT;
            break;

        case HALDEC_SKIP_PICTURE_2_THEN_DECODE :
            Tis |= SKIP_TWO_DECODE_NEXT;
            break;

        case HALDEC_SKIP_PICTURE_1_THEN_STOP :
            Tis |= SKIP_ONE_STOP_DECODE;
            break;

        default :
            /* Bad parameter: skip one and stop */
            Tis |= SKIP_ONE_STOP_DECODE;
            break;
    }

    /* If Force Instruction is asked */
    if (! (OnlyOnNextVsync))
    {
       /* Set the task to launched immediatly. This bit is reset after its action, no need to disable it */
        Tis |= VIDn_TIS_FIS;
    }

    TraceBuffer(("-TISskip0x%x", Tis));

#if defined WA_PID_MISSING || defined WA_GNBvd07112 || defined STVID_HARDWARE_ERROR_EVENT
    ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->DecoderIdle = FALSE;
#endif /* WA_PID_MISSING || WA_GNBvd07112 || STVID_HARDWARE_ERROR_EVENT */

#if defined WA_PID_MISSING || defined STVID_HARDWARE_ERROR_EVENT
    /* Could divide time by 2 for fields !!! */
    ((Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p))->MaxDecodeTime =
        time_plus(time_now(), MAX_DECODE_SKIP_TIME *
        (HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DFS_16) & VIDn_DFS_MASK) /
        MAX_SIZE_IN_MACROBLOCKS);
#endif /* WA_PID_MISSING || STVID_HARDWARE_ERROR_EVENT */

    /* Write task instruction value back: it has EXE bit, so do it at last because IT's will be raised ! */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS, Tis);
#endif /* not WA_GNBvd06350 */
} /* End of SkipPicture() function */


/*******************************************************************************
Name        : StartDecodePicture
Description : Launch decode of a picture
Parameters  : HAL Decode manager handle, flag telling whether the decode takes place
              on the same picture that is on display
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void StartDecodePicture(
                             const HALDEC_Handle_t              HALDecodeHandle,
                             const BOOL                         OnlyOnNextVsync,
                             const BOOL                         MainDecodeOverWrittingDisplay,
                             const BOOL                         SecondaryDecodeOverWrittingDisplay)
{
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
    U32 Tis;
    BOOL BoolTmp;
#ifdef WA_GNBvd16407
    U32  INS1_DebugRegister;
    BOOL IsInternalDecoderStateField;
    U32 SDmod;
    U32 Dmod;
#endif /* WA_GNBvd16407 */
#if defined DISABLE_MAIN_DECODE_WHEN_USING_SECONDARY
    U32 Tmp32;
#endif /* DISABLE_MAIN_DECODE_WHEN_USING_SECONDARY */

#if defined TRACE_UART && defined TRACE_MULTI_DECODE
    if ((HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == ((HALDEC_Properties_t *) HALDecodeHandle)->DecoderNumber) ||
        (HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == HALDEC_MULTI_DECODE_TRACE_DECODER_ALL))
    {
    TraceBufferDecoderNumber(HALDecodeHandle);
    TraceBuffer(("-GoDec"));
    }
#endif /* TRACE_UART && TRACE_MULTI_DECODE */

    /* main decode */
    if (MainDecodeOverWrittingDisplay)
    {
        HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD,
                             VIDn_DMOD_MASK,
                             VIDn_MOD_OVW_MASK,
                             VIDn_MOD_OVW_EMP,
                             HALDEC_ENABLE);
    }
    else
    {
        HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD,
                             VIDn_DMOD_MASK,
                             VIDn_MOD_OVW_MASK,
                             VIDn_MOD_OVW_EMP,
                             HALDEC_DISABLE);
    }

    /* secondary decode */
    if (SecondaryDecodeOverWrittingDisplay)
    {
        HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD,
                             VIDn_SDMOD_MASK,
                             VIDn_MOD_OVW_MASK,
                             VIDn_MOD_OVW_EMP,
                             HALDEC_ENABLE);
    }
    else
    {
        HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD,
                            VIDn_SDMOD_MASK,
                            VIDn_MOD_OVW_MASK,
                            VIDn_MOD_OVW_EMP,
                            HALDEC_DISABLE);
    }


#if 0
    switch (DecodeMode)
    {
        case HALDEC_DECODE_PICTURE_MODE_NORMAL :
            /* LDP : load pointer. When this bit is set, the decoding of the picture will start from a location specified by a pointer. */
            HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS,
                             VIDn_TIS_MASK,
                             VIDn_TIS_LDP_MASK,
                             VIDn_TIS_LDP_EMP,
                             (U8)(HALDEC_DISABLE));

            break;

        case HALDEC_DECODE_PICTURE_MODE_FROM_ADDRESS :
            /* write Address in VLDPTR : VLD Start Pointer. Memory address pointer, in byte, start-ing */
            /* point of decoding when VIDn_TIS.LDP and VIDn_TIS.LFR are both set. */
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32, (U32) DecodeAddressFrom_p);

            /* configure VIDn_TIS register so that next start code is searched from BufferAddress_p */

            /* LDP : load pointer. When this bit is set, the decoding of the picture will start from a location specified by a pointer. */
            HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS,
                             VIDn_TIS_MASK,
                             VIDn_TIS_LDP_MASK,
                             VIDn_TIS_LDP_EMP,
                             (U8)(HALDEC_ENABLE));

            /* LFR : load from register. When this bit is set, the decoding start-pointer used when VIDn_TIS.LDP is set is taken from register */
            /* VIDn_VLDPTR. Otherwise the start will be done according to value automatically computed by start code detector */
            /* and available in VIDn_PSCPTR register. */
            HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS,
                             VIDn_TIS_MASK,
                             VIDn_TIS_LFR_MASK,
                             VIDn_TIS_LFR_EMP,
                             (U8)(HALDEC_ENABLE));
        default :
            break;
    }
#endif /* 0 */

#if defined DISABLE_MAIN_DECODE_WHEN_USING_SECONDARY
    /* Enable main reconstruction for I or P pictures or for all if no secondary,
       Disable it for B pictures if secondary. */
    Tis16 = HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD_16);
    Tmp32 = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PPR_32);
    if (((Tis16 & (VIDn_MOD_EN_MASK << VIDn_MOD_EN_EMP)) != 0) && /* Secondary recontruction enabled */
        (((Tmp32 >> VIDn_PPR_PCT_EMP) & VIDn_PPR_PCT_MASK) == PICTURE_CODING_TYPE_B)
       )
    {
        /* Disable main reconstruction */
        HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD,
                         VIDn_DMOD_MASK,
                         VIDn_MOD_EN_MASK,
                         VIDn_MOD_EN_EMP,
                         HALDEC_DISABLE);
    }
    else
    {
        /* Enable main reconstruction */
        HAL_SetRegister32Value(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD,
                         VIDn_DMOD_MASK,
                         VIDn_MOD_EN_MASK,
                         VIDn_MOD_EN_EMP,
                         HALDEC_ENABLE);
    }
#endif /* DISABLE_MAIN_DECODE_WHEN_USING_SECONDARY */

#if defined WA_GNBvd06350
    if (!(Data_p->IsDecodingInTrickMode))
    {
        /* Normal decode: each time reload decode address ! */
/*        TraceBuffer(("-@0x%x", Data_p->LastSlicePictureSCAddress));*/
        PrepareDecode(HALDecodeHandle, Data_p->LastSlicePictureSCAddress, 0 /* TemporalReference */, &BoolTmp);
    }
#endif /* WA_GNBvd06350 */
    /* Launches the decode  */
    /* When EXE bit is not set, no decoding or skipping task is executed for one or two VSYNC periods, depending on */
    /* the state of VIDn_TIS.RPT. If set, the next task (decoding.or skipping) is executed. EXE is internally cleared when */
    /* the task starts its execution, i.e. setting this bit activates only one execution. */

    /* configure VIDn_TIS register to launch decode */
    Tis = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
            + VIDn_TIS) & VIDn_TIS_MASK;
    Tis &= ~VIDn_TIS_SKP;
    Tis |= NO_SKIP | VIDn_TIS_LDP | VIDn_TIS_EXE;

    /* Hardware constraint : must decode after a reset. */
    if (Data_p->FirstDecodeAfterSoftResetDone == FALSE)
    {
        Data_p->FirstDecodeAfterSoftResetDone = TRUE;
    }

    /* If Force Instruction Is asked */
    if (!(OnlyOnNextVsync))
    {
       /* Set the task to launched immediatly. This bit is reset after its action, no need to disable it */
        Tis |= VIDn_TIS_FIS;
    }

#ifdef TRACE_UART
/*    {
        U32 Level;
        U8 Percent;
        Level = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BBL_32);
        Percent = (Level & VIDn_BB_MASK) * 100 / Data_p->BitBufferSize;
        if ((Percent == 0) && (Level != 0))
        {
            Percent = 1;
        }
        TraceBuffer(("-%2d%%", Percent));
    }*/
#endif /* TRACE_UART */

#ifdef TRACE_UART
/*    TraceBuffer(("-TISdec0x%x", Tis));*/
#endif /* TRACE_UART */

#if defined WA_PID_MISSING || defined WA_GNBvd07112 || defined STVID_HARDWARE_ERROR_EVENT
    Data_p->DecoderIdle = FALSE;
#endif /* WA_PID_MISSING || WA_GNBvd07112 || STVID_HARDWARE_ERROR_EVENT */
#if defined WA_PID_MISSING || defined STVID_HARDWARE_ERROR_EVENT
    /* Could divide time by 2 for fields !!! */
    Data_p->MaxDecodeTime =
        time_plus(time_now(), MAX_DECODE_SKIP_TIME *
        (HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DFS_16) & VIDn_DFS_MASK) /
        MAX_SIZE_IN_MACROBLOCKS);
#endif /* WA_PID_MISSING || STVID_HARDWARE_ERROR_EVENT */

#ifdef TRACE_UART
/*    TraceBuffer(("\r\n -STP=0x%x", HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STP)));*/
/*    TraceBuffer((" -DMOD=0x%x", HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD)));*/
/*    TraceBuffer((" -PPR=0x%x", HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PPR_32)));*/
/*    TraceBuffer((" -RFP=0x%x", HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_RFP_32)));*/
/*    TraceBuffer((" -FFP=0x%x", HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_FFP_32)));*/
/*    TraceBuffer((" -BFP=0x%x", HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_BFP_32)));*/
/*    TraceBuffer((" -RCHP=0x%x", HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_RCHP_32)));*/
/*    TraceBuffer((" -STA=0x%x", HAL_Read16(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_STA_16)));*/
/*    TraceBuffer((" -VLDPTR=0x%x", HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_VLDPTR_32)));*/
#endif /* TRACE_UART */

#if defined WA_GNBvd07112
    Data_p->HasConcealedDUE = FALSE;
#endif /* WA_GNBvd07112 */

#ifdef USE_MULTI_DECODE_SW_CONTROLLER
#if defined WA_SC_NOT_LAUNCHED_ON_DECODE_START
    /* Determine if SC search must be launched with decode. */
    if (!(Data_p->IsDecodingInTrickMode))
    {
        Data_p->MustLaunchSCDetector = TRUE;
    }
    else
    {
        Data_p->MustLaunchSCDetector = FALSE;
    }
#endif /* WA_SC_NOT_LAUNCHED_ON_DECODE_START */

    /* Don't write TIS: store it and call multi-decode software controller instead ! */
    Data_p->MultiDecodeExecutionTis = Tis;
    if (Data_p->MultiDecodeStatus == MULTI_DECODE_STATUS_FINISHED_NOT_POSTED)
    {
/*        TraceBuffer(("DecFinishC"));*/
        Data_p->MultiDecodeStatus = MULTI_DECODE_STATUS_IDLE;
        VIDDEC_MultiDecodeFinished(((HALDEC_Properties_t *) HALDecodeHandle)->MultiDecodeHandle);
    }
    if (Data_p->MultiDecodeStatus == MULTI_DECODE_STATUS_IDLE) /* Should always be TRUE ! */
    {
/*        TraceBuffer(("DecReady"));*/
        Data_p->MultiDecodeStatus = MULTI_DECODE_STATUS_READY_NOT_STARTED;
        VIDDEC_MultiDecodeReady(((HALDEC_Properties_t *) HALDecodeHandle)->MultiDecodeHandle, &(Data_p->MultiDecodeReadyParams));
    }
    else
    {
        TraceBuffer(("-MultiDec NOT IDLE !**"));
    }
#else /* USE_MULTI_DECODE_SW_CONTROLLER */
#ifdef WA_GNBvd16407

    SDmod = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD)
            & VIDn_SDMOD_MASK;

    if (SDmod & (VIDn_MOD_EN_MASK << VIDn_MOD_EN_EMP))
    {
        /* The secondary reconstruction is enabled. Test and apply the work-around. */

        /* Get the INS1 debug register.*/
        INS1_DebugRegister = HAL_Read32(((U8 *) Data_p->ConfigurationBaseAddress_p) + DBG_INS1);
        /* Is it in field mode ?       */
        IsInternalDecoderStateField = ((INS1_DebugRegister & DBG_INS1_FIELD_MASK) ? TRUE : FALSE);

        /* Test if the field/Frame mode is changing.    */
        if ( Data_p->IsNextDecodeField == IsInternalDecoderStateField )
        {
            /* Same field/frame than current state of internal decoder. */
            Data_p->NumberOfUselessPSD = 0;
            TraceBuffer((" Same %s", (IsInternalDecoderStateField ? "fld" : "FRM")));
        }
        else
        {
            /* Field/frame mode is changing.                                */

            /* Remember the initial DMod and SDMod registers values.        */
            Data_p->InitialSDmod32RegisterValue = SDmod;
            SDmod &= ~(VIDn_MOD_EN_MASK << VIDn_MOD_EN_EMP);
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SDMOD,
                    SDmod);
            Dmod = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD)
                    & VIDn_DMOD_MASK;
            Data_p->InitialDmod32RegisterValue = Dmod ;
            Dmod &= ~(VIDn_MOD_EN_MASK << VIDn_MOD_EN_EMP);
            HAL_Write32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_DMOD, Dmod);

            /* Set the number of dummy PSD interrupt to manage.             */
            Data_p->NumberOfUselessPSD = 2;
            TraceBuffer((" %s", (IsInternalDecoderStateField ? "fld->FRM" : "FRM->fld")));
        }
    }
    else
    {
        Data_p->NumberOfUselessPSD = 0;
    }
#endif /* WA_GNBvd16407 */

    /* Write task instruction value back: it has EXE bit, so do it at last because IT's will be raised ! */
    HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_TIS, Tis);

#endif /* not USE_MULTI_DECODE_SW_CONTROLLER */

#if defined WA_SC_NOT_LAUNCHED_ON_DECODE_START
#ifndef USE_MULTI_DECODE_SW_CONTROLLER
    if (!(Data_p->IsDecodingInTrickMode))
    {
        /* Launch SC detector when launching decode */
        SearchNextStartCode(HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);
    }
#endif /* not USE_MULTI_DECODE_SW_CONTROLLER */
#endif /* WA_SC_NOT_LAUNCHED_ON_DECODE_START */

} /* End of StartDecodePicture() function */


/*******************************************************************************
Name        : Term
Description : Actions to do on chip when terminating
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void Term(const HALDEC_Handle_t HALDecodeHandle)
{
   /* memory deallocation of the private datas if necessary */
    SAFE_MEMORY_DEALLOCATE(((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p,
                           ((HALDEC_Properties_t *) HALDecodeHandle)->CPUPartition_p,
                           sizeof(Omega2Properties_t));

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





/* Private Functions -------------------------------------------------------- */


/*******************************************************************************
Name        : NewStartCode
Description : Process new start code arrival
Parameters  : start code
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void NewStartCode(const HALDEC_Handle_t HALDecodeHandle, const U8 StartCode)
{
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
#ifdef WA_GNBvd16165
    void * TempLastPictureSCAddress;
#endif /* WA_GNBvd16165 */

#if defined TRACE_UART && defined TRACE_MULTI_DECODE
/*    TraceBufferDecoderNumber(HALDecodeHandle);*/
#endif /* TRACE_UART && TRACE_MULTI_DECODE */
    TraceBuffer(("\r\nNew SC(%x)\r\n", StartCode));


#if defined WA_GNBvd06350 || defined WA_GNBvd16165 || defined WA_GNBvd13097
    /* Check previous start code */
    if (Data_p->LastStartCode == PICTURE_START_CODE)
    {
#if defined WA_GNBvd06249
        Data_p->LastButOneSlicePictureSCAddress = Data_p->LastSlicePictureSCAddress;
#endif /* WA_GNBvd06249 */
        Data_p->LastSlicePictureSCAddress = Data_p->LastPictureSCAddress;

#if defined TRACE_UART && defined TRACE_MULTI_DECODE
        if ((HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == ((HALDEC_Properties_t *) HALDecodeHandle)->DecoderNumber) ||
            (HALDEC_DECODER_TO_TRACE_IN_MULTI_DECODE == HALDEC_MULTI_DECODE_TRACE_DECODER_ALL))
        {
            TraceBufferDecoderNumber(HALDecodeHandle);
            TraceBuffer(("-PicSC"));
        }
#endif /* TRACE_UART && TRACE_MULTI_DECODE */
    }
#endif /* WA_GNBvd06350 */

#if defined WA_GNBvd06350 || defined WA_GNBvd16165 || defined WA_GNBvd13097
    /* Store picture address if current start code is a picture start code */
    if (StartCode == PICTURE_START_CODE)
    {
#ifndef WA_GNBvd16165
        Data_p->LastPictureSCAddress =
                (void *) HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_PSCPTR_32);
#else
        if (Data_p->LastStartCode == USER_DATA_START_CODE)
        {
            /* Get the current Start code pointer position.             */
            TempLastPictureSCAddress =
                   (void *)HAL_Read32(((U8 *)((HALDEC_Properties_t *)HALDecodeHandle)->RegistersBaseAddress_p)+VIDn_SCDRPTR_32);
            /* Apply the header fifo length .... correction             */
            TempLastPictureSCAddress = (void *)((U32)(TempLastPictureSCAddress) - WA_HEADER_FIFO_LENGTH);
            /* Check for very small pictures case.                      */
            if (TempLastPictureSCAddress <= Data_p->LastSlicePictureSCAddress)
            {
                /* The current picture position is before the last decoded picture position.                    */
                /* Correct it to be juste after the previous one (LastSlicePictureSCAddress + StartCodeSize (4).*/
                TempLastPictureSCAddress = (void *)((U32)(Data_p->LastSlicePictureSCAddress) + 4);
            }
            /* OK, picture start code position estimated.       */
            Data_p->LastPictureSCAddress = TempLastPictureSCAddress;
        }
        else
        {
            /* Normal case. The picture have been detected thanks to SCH interrupt. The register VIDn_PSCPTR    */
            /* is valid. No need to estimate the picture StartCode position.                                    */
            Data_p->LastPictureSCAddress =
                    (void *)HAL_Read32(((U8 *)((HALDEC_Properties_t *)HALDecodeHandle)->RegistersBaseAddress_p)+VIDn_PSCPTR_32);
        }
#endif /* WA_GNBvd16165 */

#if defined WA_GNBvd13097
    /* Test the address LastPictureSCAddress.   */
    /* It could be outside bit buffer.          */
    if ( (S32)(Data_p->LastPictureSCAddress) < (S32)(Data_p->BitBufferBase) )
    {
        /* The address is out of bit buffer area : just add bit buffer size.    */
        Data_p->LastPictureSCAddress = (void *) ((S32)((S32)(Data_p->LastPictureSCAddress) + (S32)(Data_p->BitBufferSize)));
    }
#endif /* WA_GNBvd13097 */

/*        TraceBuffer((" -PicSc@0x%x", Data_p->LastPictureSCAddress));*/
    }
#endif /* WA_GNBvd06350 */

#ifdef WA_HEADER_FIFO_ACCESS
    if (Data_p->isPictureStartCodeDetected)
    {
        if ( (StartCode >= FIRST_SLICE_START_CODE) && (StartCode <= GREATEST_SLICE_START_CODE) )
        {
            Data_p->isPictureStartCodeDetected = FALSE;
            TraceBuffer(("\r\nPSC detected (%d)", Data_p->isPictureStartCodeDetected));
        }
    }
    else
    {
        if (StartCode == PICTURE_START_CODE)
        {
            /* Remember a picture start code occured !!!    */
            Data_p->isPictureStartCodeDetected = TRUE;
            TraceBuffer(("\r\nPSC detected (%d)", Data_p->isPictureStartCodeDetected));
        }
    }
#endif /* WA_HEADER_FIFO_ACCESS */

    /* Remember the current start code for the next time a start code is detected.  */
    Data_p->LastStartCode = StartCode;

} /* End of NewStartCode() function */


/*******************************************************************************
Name        : InternalProcessInterruptStatus
Description : Process interrupt status for internal purposes
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void InternalProcessInterruptStatus(const HALDEC_Handle_t HALDecodeHandle, const U32 Status)
{
    Omega2Properties_t * Data_p = (Omega2Properties_t *) (((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p);
    U32 DiffPSC;
#ifndef WA_GNBvd06350
    U32 Tis;
#endif

    /* Get SCD count if SCH interrupt, so that it is sure it is valid */
    if ((Status & HALDEC_START_CODE_HIT_MASK) != 0)
    {
        /* Got SCH interrupt */
/*        TraceBuffer(("-SCH"));*/
        Data_p->StartCodeDetectorIdle = TRUE;

        /* SCDCount is in 16-bit units */
        Data_p->LatchedSCD26bitValue = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p) + VIDn_SCDC_32) & VIDn_SCDC_32_SCDC;
#ifdef WA_GNBvd06252
        Data_p->LatchedSCD26bitValue <<= 1;
#endif /* WA_GNBvd06252 */
        Data_p->LatchedSCD26bitValue ++;
/*        if (SC on LSB)
        {
            Data_p->LatchedSCD26bitValue ++;
        }*/
        if (Data_p->PreviousSCD26bitValue > Data_p->LatchedSCD26bitValue)
        {
            DiffPSC = 0x4000000 - Data_p->PreviousSCD26bitValue + Data_p->LatchedSCD26bitValue;
        }
        else
        {
            DiffPSC = Data_p->LatchedSCD26bitValue - Data_p->PreviousSCD26bitValue;
        }
        Data_p->LatchedSCD32bitValue = Data_p->PreviousSCD32bitValue + DiffPSC;
/*        while (Data_p->LatchedSCD32bitValue >= (Data_p->BitBufferBase + Data_p->BitBufferSize - 1))*/
/*         {*/
/*            Data_p->LatchedSCD32bitValue -= Data_p->BitBufferSize;*/
/*        }*/
        Data_p->PreviousSCD32bitValue = Data_p->LatchedSCD32bitValue;
        Data_p->PreviousSCD26bitValue = Data_p->LatchedSCD26bitValue;
        Data_p->IsSCDValid = TRUE;
/*        TraceBuffer(("-SCDOff=0x%x/SCDAdd=0x%x", Data_p->PreviousSCD26bitValue, Data_p->PreviousSCD32bitValue));*/
    }

#if defined WA_PID_MISSING || defined WA_GNBvd07112 || defined STVID_HARDWARE_ERROR_EVENT
    if ((Status & HALDEC_PIPELINE_IDLE_MASK) != 0)
    {
        Data_p->DecoderIdle = TRUE;
    }
#endif /* WA_PID_MISSING || WA_GNBvd07112 || STVID_HARDWARE_ERROR_EVENT */
#ifdef USE_MULTI_DECODE_SW_CONTROLLER
    if (((Status & HALDEC_PIPELINE_IDLE_MASK) != 0) && (Data_p->MultiDecodeStatus == MULTI_DECODE_STATUS_STARTED_NOT_FINISHED))
    {
        Data_p->MultiDecodeStatus = MULTI_DECODE_STATUS_FINISHED_NOT_POSTED;
    }
#endif /* USE_MULTI_DECODE_SW_CONTROLLER */

    /* Reset decode config if DEI interrupt, so that default behavior in non-trick mode is ensured */
#ifndef WA_GNBvd06350
    if ((Status & HALDEC_DECODER_IDLE_MASK) != 0)
    {
        if (!(Data_p->IsDecodingInTrickMode))
        {
            /* Update task instruction value */
            Tis = HAL_Read32(((U8 *)((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
                    + VIDn_TIS) & VIDn_TIS_MASK;
            Tis &= ~ (VIDn_TIS_LFR | VIDn_TIS_EXE);
            HAL_Write32((U8 *) (((HALDEC_Properties_t *) HALDecodeHandle)->RegistersBaseAddress_p)
                    + VIDn_TIS, Tis);
/*            TraceBuffer(("-TISdei0x%x", Tis));*/
        }
    } /* End DEI interrupt */
#endif /* WA_GNBvd06350 */

} /* End of InternalProcessInterruptStatus() function */


/*******************************************************************************
Name        : HALDEC_Error
Description : Function called every time an error occurs. A breakpoint here
              should facilitate the debug
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#ifdef DEBUG
void HALDEC_Error(void)
{
    while (1);
}
#endif /* DEBUG */

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
/* End of hv_dec2.c */
