/*******************************************************************************

File name   : lcmpegx1.c

Description : HAL video decode lcmpegs1 and lcmpegh1 family source file
              NB: lcmpegh1 is same as lcmpegs1 + HD support + decimation.

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
 May 2003            Created                                      Michel Bruant

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Should be removed for official releases. */
/* Overall debug activation...  */
/* Enable uart traces...        */

/* Usual traces */

/*#define TRACE_HAL_DECODE*/
/*#define TRACE_WITH_TIME*/
/*#define TRACE_UART*/
/*#define TRACE_PTS*/
/*#define TRACE_SC*/
/*#define TRACE_DECLIST*/
/*#define TRACE_PERFORMANCES*/
/*#define TRACE_INTERRUPT*/
/*#define SEMAPHORE_TRACE*/

/* Traces formatted for Visual Debugger */

/* #define TRACE_LCMPEG_PICTURE_TYPE */
/* #define TRACE_LCMPEG_FRAME_BUFFER */
/* #define TRACE_LCMPEG_DURATION */
#ifdef ST_deblock
#define WA_GNBvd63015
#endif

#if defined(TRACE_LCMPEG_PICTURE_TYPE) || defined(TRACE_LCMPEG_FRAME_BUFFER) || defined(TRACE_LCMPEG_DURATION) || defined(TRACE_HAL_DECODE) || defined(TRACE_WITH_TIME) || defined(TRACE_UART) || defined(TRACE_PTS) || defined(TRACE_SC) || defined(TRACE_DECLIST) || defined(TRACE_PERFORMANCES) || defined(TRACE_INTERRUPT) || defined(SEMAPHORE_TRACE)
#define TRACE_UART
#include "trace.h"
#endif /* defined(TRACE_LCMPEG_PICTURE_TYPE) || defined(TRACE_LCMPEG_FRAME_BUFFER) || defined(TRACE_LCMPEG_DURATION) || defined(TRACE_HAL_DECODE) || defined(TRACE_WITH_TIME) || defined(TRACE_UART) || defined(TRACE_PTS) || defined(TRACE_SC) || defined(TRACE_DECLIST) || defined(TRACE_PERFORMANCES) || defined(TRACE_INTERRUPT) || defined(SEMAPHORE_TRACE) */

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "stsys.h"
#include "sttbx.h"

#include "stfdma.h"

#ifdef ST_OSWINCE
#include "stdevice.h"
#endif

#include "vid_ctcm.h"
#include "vid_com.h"
#include "lcmp_reg.h"
#include "lcmpegx1.h"
#include "halv_dec.h"
#include "inject.h"
#ifdef ST_deblock
#include "deblock.h"
#endif /* ST_deblock */

/* Private Constants -------------------------------------------------------- */

/* Max time for searching a start code : it is set equal to the frequency of
 * the wake up of the decode task, because when the startcode detector is
 * launched, if we don't find a new startcode immediately, we check the
 * startcode list each time the decode task is woken up. */
#define MAX_START_CODE_SEARCH_TIME (STVID_MAX_VSYNC_DURATION / STVID_DECODE_TASK_FREQUENCY_BY_VSYNC)

/* Startcode list properties : we assume that SCLIST_SIZE_STANDARD entries are needed if the bitbuffer size
 * is equal to BITBUFFER_SIZE_STANDARD */
#define SCLIST_SIZE_STANDARD    4000
#define BITBUFFER_SIZE_STANDARD 380928
#define SCLIST_ALIGNEMENT       256

/* Input buffer size is computed according 2 considerations :
 * First condition :
 * (transfer periode) x (max bit rate) = 44Kb
 * 44Kb x 2 to bufferize enough = 88Kb
 * Second condition : (for HDD injections)
 * 4 x (transport packet) x (sector size) = 4 x 188 x 512
 *
 * So the input buffer size is the max of both values. */

#if defined (ST_trickmod)
#define INPUT_BUFFER_SIZE  ((188*512*4) & ALIGNEMENT_FOR_TRANSFERS)  /* explicit the constraint */
#else /* !defined(ST_trickmod) */
#define INPUT_BUFFER_SIZE  ((100*1024) & ALIGNEMENT_FOR_TRANSFERS)  /* explicit the constraint */
#endif /* defined(ST_trickmod) */

#define CD_FIFO_ALIGNEMENT 128

/* Size of the array where are copied PTS informations  */
#ifndef PTS_SCLIST_SIZE
#define PTS_SCLIST_SIZE     10
#endif /* PTS_SCLIST_SIZE */

#if defined(LCMPEGX1_WA_GNBvd45748)
#define WA_GNBvd45748_STREAM_SIZE 244
static U8 RecoveryStream[WA_GNBvd45748_STREAM_SIZE] = {
       0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xb3, 0x02, 0x00, 0x20, 0x27, 0x91, 0x4a, 0xa7, 0x18, 0x00,
       0x00, 0x01, 0xb5, 0x14, 0x82, 0x06, 0xf1, 0x09, 0x00, 0x00, 0x00, 0x01, 0xb8, 0x44, 0xbb, 0x6d,
       0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x0f, 0xff, 0xf8, 0x00, 0x00, 0x01, 0xb5, 0x8f, 0xff, 0xf3,
       0xd3, 0x80, 0x00, 0x00, 0x01, 0x01, 0x0e, 0x01, 0xad, 0x4c, 0xd4, 0x89, 0x5a, 0xd2, 0x31, 0x9c,
       0x40, 0x00, 0x00, 0x01, 0x02, 0x0b, 0x58, 0x42, 0x91, 0x4d, 0x89, 0x34, 0xa5, 0xad, 0x70, 0x00,
       0x00, 0x01, 0x00, 0x00, 0x17, 0xff, 0xfb, 0x80, 0x00, 0x00, 0x01, 0xb5, 0x86, 0x3f, 0xf3, 0xd1,
       0x80, 0x00, 0x00, 0x01, 0x01, 0x0c, 0x01, 0x1b, 0x10, 0x00, 0x05, 0xc0, 0x38, 0x80, 0x09, 0x00,
       0x06, 0xc0, 0x34, 0xad, 0x00, 0x1b, 0x00, 0x41, 0x40, 0x0f, 0x80, 0x58, 0x00, 0x82, 0x08, 0xc3,
       0x89, 0xc0, 0x03, 0xcb, 0x60, 0x01, 0xd0, 0x00, 0x80, 0x02, 0x00, 0x19, 0x54, 0x2e, 0x12, 0x03,
       0xd0, 0x11, 0x80, 0x0f, 0xc3, 0xe8, 0x00, 0x76, 0x00, 0x37, 0xa0, 0x17, 0x80, 0xd6, 0xc8, 0xa0,
       0x03, 0x98, 0x00, 0x00, 0x01, 0x02, 0x0a, 0x35, 0x27, 0x0a, 0x00, 0xf8, 0x00, 0x55, 0x40, 0x1e,
       0x00, 0xd0, 0x00, 0x64, 0x02, 0xc9, 0x48, 0xc0, 0x34, 0x00, 0x1c, 0x40, 0x00, 0x64, 0x00, 0x28,
       0x01, 0x65, 0x90, 0x06, 0xc4, 0x68, 0x02, 0x40, 0x1f, 0x00, 0x79, 0x46, 0xa1, 0x01, 0x00, 0x68,
       0x03, 0xe0, 0x01, 0xa5, 0x00, 0x44, 0x00, 0x2a, 0x01, 0xe5, 0x00, 0x58, 0x00, 0x3a, 0xab, 0x00,
       0x1a, 0x00, 0xb8, 0x06, 0x90, 0x02, 0xe0, 0x01, 0xf0, 0x00, 0xd2, 0xa0, 0x1f, 0x00, 0x0f, 0x20,
       0x00, 0x00, 0x01, 0xb7
       };
#endif /* defined(LCMPEGX1_WA_GNBvd45748) */

/* Private Types ------------------------------------------------------------ */

/* Found PTS informations*/
typedef struct
{
    U32     PTS32;      /* Least 32bits of the PTS value*/
    BOOL    PTS33;      /* 33bit of the PTS value */
    void*   Address_p;  /* Address in the ES buffer from where was removed the PTS informations */
} PTS_t;

/* Structure used to store all the PTS informations found in the StartCodeList for reuse when a Picture StartCode is found*/
typedef struct
{
    PTS_t    SCList[PTS_SCLIST_SIZE];      /* Array where the PTS entries will be copied */
    PTS_t*   Write_p;                      /* Next entry will be copied here */
    PTS_t*   Read_p;                       /* Next entry should be read from here */
} PTS_SCList_t;


/* Structure used to define a ES bit buffer and associated SC-List */
typedef struct
{
    U8 *                    ES_Base_p;              /* ES buffer limit        */
    U8 *                    ES_Top_p;               /* ES buffer limit        */
    U8 *                    ES_Write_p;             /* pointer in ES buffer   */
    U8 *                    ES_Read_p;              /* pointer in ES buffer   */
    U8 *                    LastSCAdd_p;            /* the address of the last startcode found */

    /* associated SC-List */
    STFDMA_SCEntry_t   *    SCList_Base_p;          /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    SCList_Top_p;           /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    SCList_Loop_p;          /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    SCList_Write_p;         /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    NextSCEntry_p;          /* pointer in SC List     */
#ifdef TRACE_UART
    STFDMA_SCEntry_t   *    LastSCEntry_p;          /* pointer in SC List     */
#endif
} ES_BitBuffer_t;

typedef struct
{
    void *                  BackwardChromaFrameBuffer;
    void *                  BackwardLumaFrameBuffer;
    void *                  ForwardChromaFrameBuffer;
    void *                  ForwardLumaFrameBuffer;
    void *                  DecodeChromaFrameBuffer;
    void *                  DecodeLumaFrameBuffer;
    U32                     WidthInMacroBlock;
    U32                     HeightInMacroBlock;
    U32                     TotalInMacroBlock;
    U32                     PPR_ToWrite;
    U32                     IntraCoefs[16];
    U32                     NonIntraCoefs[16];
#if defined(LCMPEGH1)
    void *                  SecondaryDecodeChromaFrameBuffer;
    void *                  SecondaryDecodeLumaFrameBuffer;
    STVID_DecimationFactor_t  HDecimation;
    STVID_DecimationFactor_t  VDecimation;
    STVID_ScanType_t        SourceScanType;
#endif /* LCMPEGH1 */
#ifdef ST_deblock
    BOOL                    CurrentDeblockingState;
#endif /* ST_deblock */
} DecodeParams_t;

/* Structure used to store the context of a hal-lcmpeg instance: */
typedef struct
{
#ifdef WA_GNBvd63015
    semaphore_t *           SharedAccessToMME_p;
    VIDCOM_Task_t           SharedAccessToMMETask;
    BOOL                    ProcessSharedVideoDeblockingInterruptIsRunning;
#ifdef ST_deblock
    MPEG2Deblock_TransformParam_t DeblockTransfromParams;
#endif /* ST_deblock */
#endif /* WA_GNBvd63015 */
    semaphore_t *           SharedAccess_p;
    ES_BitBuffer_t          ES_LinearBitBufferA;
    ES_BitBuffer_t          ES_LinearBitBufferB;
    ES_BitBuffer_t          ES_CircularBitBuffer;
    ES_BitBuffer_t *        InjBuffer_p;      /* buffer under injection    */
    ES_BitBuffer_t *        DecBuffer_p;      /* buffer under parse/decode */
    U8 *                    NextDataPointer_p;/* pointer in ES a buffer    */
    U32                     OutputCounter;
    U32                     InjectHdl;
    /* input buffer (PES or ES) */
    STAVMEM_BlockHandle_t   InputBufHdl;
    void *                  InputBuffBase_p;
    void *                  InputBuffTop_p;
    /* decoder */
    /* Enabling primary decode and secondary decode */
    BOOL                    DecodeEnabled;
#if defined(LCMPEGH1)
    BOOL                    SecondaryDecodeEnabled;
#endif /* LCMPEGH1 */
    BOOL                    OverWriteEnabled;
    /* next decode configuration */
    U32                     HWInterruptMask;    /* Compatible with VID_STA register.    */
    U32                     SWInterruptMask;    /* Compatible with haldec.h header file */
    U32                     SWStatus; /* The set of interrupts that was notified on the last SW interrupt(simulation */
    U32                     HWStatus; /* The set of interrupts that was notified on the last HW interrupt */
                            /* SWInterruptMask is used to simulate interrupt            */
    /* One decode parameters */
    DecodeParams_t          DecodeParams;
    void *                  DecodeAdd; /* This corresponds to the last picture startcode PARSED address */
#ifdef TRACE_UART
    void*                   LastDecodeAdd;
#endif /* TRACE_UART */
    /* parser, sc detect simulation */
    BOOL                    FirstSequenceReached;
    BOOL                    SCDetectSimuLaunched;
    U32                     BitBufferThreshold;
    STEVT_EventID_t         InterruptEvtID;
    PTS_t                   CurrentPicturePTSEntry;
    PTS_t                   LastPTSStored;
    PTS_SCList_t            PTS_SCList;
    BOOL                    EndOfSlicesSC;
    BOOL                    SkipNextStartCode;
    void *                  PictureAdd;
    void *                  EndOfPictureAdd;
    HALDEC_BitBufferType_t  BitBufferType;
    /* Avmem mapping informations */
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* SCList allocation */
    STAVMEM_BlockHandle_t   SCListHdl;
    void*                   AlignedSCList_p;
#if defined(LCMPEGX1_WA_GNBvd45748)
    STAVMEM_BlockHandle_t   WAGNBvd45748BuffersHdl;
    void*                   RecoveryStream_p;
    void*                   RecoveryLuma_p;
    void*                   RecoveryChroma_p;
#endif /* defined(LCMPEGX1_WA_GNBvd45748) */
#ifdef ST_deblock
    VID_Deblocking_Handle_t DeblockingHandle;
    U32                     Deblock_HWStatus;
    BOOL                    DeblockingEnabled;
    int                     DeblockingStrength;
    U32                     PictureMeanQP;
    U32                     PictureVarianceQP;
#endif  /* ST_deblock */
} LcmpegContext_t;

/* Private Variables (static)------------------------------------------------ */

/* static variable = shared by all instances even in case of multi-init */
/* Multi-decode controller context */
/*---------------------------------*/

typedef struct
{
    U32                     NbInstalledIRQ;
    /* interrupt dispatch */
    HALDEC_Handle_t         HALDecodeHandleForNextIT;
    /* decoder / registers access */
    semaphore_t *           HW_DecoderAccess_p;
    U8*                     RegistersBaseAddress_p;
} MultiDecodeController_t;

static MultiDecodeController_t MultiDecodeController = { 0,
                                                         (HALDEC_Handle_t)NULL,
                                                         (semaphore_t *)NULL,
                                                         (U8*)NULL
                                                        };

/* Functions exported thanks to fct pointers -------------------------------- */
static void DecodingSoftReset(const HALDEC_Handle_t DecodeHandle, const BOOL IsRealTime);
static void DisableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle);
static void EnableSecondaryDecode(const HALDEC_Handle_t HALDecodeHandle,
            const STVID_DecimationFactor_t  H_Decimation,
            const STVID_DecimationFactor_t  V_Decimation,
            const STVID_ScanType_t          ScanType);
static void DisablePrimaryDecode(const HALDEC_Handle_t HALDecodeHandle);
static void EnablePrimaryDecode(const HALDEC_Handle_t HALDecodeHandle);
static void DisableHDPIPDecode(const HALDEC_Handle_t HALDecodeHandle,
            const STVID_HDPIPParams_t * const HDPIPParams_p);
static void EnableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle,
            const STVID_HDPIPParams_t * const HDPIPParams_p);
static void FlushDecoder(const HALDEC_Handle_t HALDecodeHandle,
            BOOL * const DiscontinuityStartCodeInserted_p);
static U16 Get16BitsStartCode(const HALDEC_Handle_t  HALDecodeHandle,
            BOOL * const StartCodeOnMSB_p);
static U16 Get16BitsHeaderData(const HALDEC_Handle_t  HALDecodeHandle);
static U32 Get32BitsHeaderData(const HALDEC_Handle_t  HALDecodeHandle);
static U32 GetAbnormallyMissingInterruptStatus(
           const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t GetDataInputBufferParams(
           const HALDEC_Handle_t HALDecodeHandle,
           void ** const BaseAddress_p,
           U32 * const Size_p);
static void GetDecodeSkipConstraint(const HALDEC_Handle_t HALDecodeHandle,
           HALDEC_DecodeSkipConstraint_t * const HALDecodeSkipConstraint_p);
static U32 GetBitBufferOutputCounter(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t GetCDFifoAlignmentInBytes(
           const HALDEC_Handle_t HALDecodeHandle,
           U32 * const CDFifoAlignment_p);
static U32 GetDecodeBitBufferLevel(const HALDEC_Handle_t HALDecodeHandle);
#ifdef STVID_HARDWARE_ERROR_EVENT
static STVID_HardwareError_t  GetHardwareErrorStatus(
           const HALDEC_Handle_t HALDecodeHandle);
#endif /* STVID_HARDWARE_ERROR_EVENT  */
static U32 GetInterruptStatus(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t GetLinearBitBufferTransferedDataSize(
           const HALDEC_Handle_t HALDecodeHandle,
           U32 * const TransferedDataSize_p);
static BOOL GetPTS(const HALDEC_Handle_t HALDecodeHandle, U32 * const PTS_p,
           BOOL * const PTS33_p);
static ST_ErrorCode_t GetSCDAlignmentInBytes(
           const HALDEC_Handle_t HALDecodeHandle, U32 * const SCDAlignment_p);
static ST_ErrorCode_t GetStartCodeAddress(
           const HALDEC_Handle_t HALDecodeHandle,
           void ** const BufferAddress_p);
static U32 GetStatus(const HALDEC_Handle_t HALDecodeHandle);
static BOOL IsHeaderDataFIFOEmpty(const HALDEC_Handle_t HALDecodeHandle);
static ST_ErrorCode_t Init(const HALDEC_Handle_t HALDecodeHandle);
static void PipelineReset(const HALDEC_Handle_t HALDecodeHandle);
static void PrepareDecode(const HALDEC_Handle_t HALDecodeHandle,
           void * const DecodeAddressFrom_p, const U16 pTemporalReference,
           BOOL * const WaitForVLD_p);
static void ResetPESParser(const HALDEC_Handle_t HALDecodeHandle);
static void LoadIntraQuantMatrix(const HALDEC_Handle_t HALDecodeHandle,
           const QuantiserMatrix_t *Matrix_p,
           const QuantiserMatrix_t *ChromaMatrix_p);
static void LoadNonIntraQuantMatrix(const HALDEC_Handle_t HALDecodeHandle,
           const QuantiserMatrix_t *Matrix_p,
           const QuantiserMatrix_t *ChromaMatrix_p);
static void SearchNextStartCode(const HALDEC_Handle_t HALDecodeHandle,
           const HALDEC_StartCodeSearchMode_t SearchMode,
           const BOOL FirstSliceDetect,
           void * const SearchAddressFrom_p);
static void SetInterruptMask(const HALDEC_Handle_t HALDecodeHandle,
           const U32 Mask);
static void SetDecodeLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle,
           void * const BufferAddress_p);
static void SetDecodeChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle,
           void * const BufferAddress_p);
static void SetSecondaryDecodeLumaFrameBuffer(
           const HALDEC_Handle_t HALDecodeHandle,
           void * const BufferAddress_p);
static void SetSecondaryDecodeChromaFrameBuffer(
           const HALDEC_Handle_t HALDecodeHandle,
           void * const BufferAddress_p);
static void SetBackwardLumaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle,
           void * const BufferAddress_p);
static void SetBackwardChromaFrameBuffer(const HALDEC_Handle_t HALDecodeHandle,
           void * const BufferAddress_p);
static void SetForwardLumaFrameBuffer(HALDEC_Handle_t HALDecodeHandle,
           void * const BufferAddress_p);
static void SetForwardChromaFrameBuffer(HALDEC_Handle_t HALDecodeHandle,
           void * const BufferAddress_p);
#ifdef ST_deblock
static void SetDeblockingMode(HALDEC_Handle_t HALDecodeHandle,
           const BOOL DeblockingEnabled);
#ifdef VIDEO_DEBLOCK_DEBUG
static void SetDeblockingStrength(HALDEC_Handle_t HALDecodeHandle,
           const int DeblockingStrength);
#endif /* VIDEO_DEBLOCK_DEBUG  */
#endif /* ST_deblock */
static ST_ErrorCode_t GetPictureDescriptors(const HALDEC_Handle_t HALDecodeHandle,
            STVID_PictureDescriptors_t * PictureDescriptors_p);
static ST_ErrorCode_t SetDataInputInterface(
           const HALDEC_Handle_t HALDecodeHandle,
           ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle,
                                            void ** const Address_p),
           void (*InformReadAddress)(void * const Handle, void * const Address),
           void * const Handle );
static void SetFoundStartCode(const HALDEC_Handle_t HALDecodeHandle,
           const U8 StartCode,
           BOOL * const FirstPictureStartCodeFound_p);
static void SetDecodeBitBuffer(const HALDEC_Handle_t HALDecodeHandle,
           void * const BufferAddressStart_p,
           const U32 BufferSize,
           const HALDEC_BitBufferType_t BufferType);
static void SetDecodeBitBufferThreshold(const HALDEC_Handle_t HALDecodeHandle,
           const U32 OccupancyThreshold);
static void SetMainDecodeCompression(const HALDEC_Handle_t HALDecodeHandle,
           const STVID_CompressionLevel_t Compression);
static void SetDecodeConfiguration(const HALDEC_Handle_t HALDecodeHandle,
           const StreamInfoForDecode_t * const StreamInfo_p);
static void SetSkipConfiguration(
           const HALDEC_Handle_t HALDecodeHandle,
           const STVID_PictureStructure_t * const PictureStructure_p);
static ST_ErrorCode_t SetSmoothBackwardConfiguration(
           const HALDEC_Handle_t HALDecodeHandle,
           const BOOL  SetNotReset);
static void SetStreamID(const HALDEC_Handle_t HALDecodeHandle,
           const U8 StreamID);
static void SetStreamType(const HALDEC_Handle_t HALDecodeHandle,
           const STVID_StreamType_t StreamType);
static ST_ErrorCode_t Setup(const HALDEC_Handle_t HALDecodeHandle,
           const STVID_SetupParams_t * const SetupParams_p);
static void SkipPicture(const HALDEC_Handle_t HALDecodeHandle,
           const BOOL OnlyOnNextVsync,
           const HALDEC_SkipPictureMode_t  Skip);
static void StartDecodePicture(const HALDEC_Handle_t HALDecodeHandle,
           const BOOL OnlyOnNextVsync,
           const BOOL MainDecodeOverWrittingDisplay,
           const BOOL SecondaryDecodeOverWrittingDisplay);
static void Term(const HALDEC_Handle_t HALDecodeHandle);
#ifdef STVID_DEBUG_GET_STATUS
static void GetDebugStatus(const HALDEC_Handle_t  HALDecodeHandle,
            STVID_Status_t* const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
static void WriteStartCode(const HALDEC_Handle_t  HALDecodeHandle,
            const U32             SCVal,
            const void * const    SCAdd_p);


#if defined(LCMPEGX1_WA_GNBvd45748)
    void HALDEC_LCMPEGX1GNBvd45748WA(const HALDEC_Handle_t  HALDecodeHandle);
#endif
#ifdef WA_GNBvd63015
    static void ProcessSharedVideoDeblockingInterrupt(const HALDEC_Handle_t  HALDecodeHandle);
#endif /* WA_GNBvd63015 */

/* Private fct protos --------------------------------------------------------*/
static BOOL IsThereANewStartCode(const HALDEC_Handle_t HALDecodeHandle);
static void ProcessSC(const HALDEC_Handle_t  HALDecodeHandle);
static void SearchForAssociatedPTS(const HALDEC_Handle_t HALDecodeHandle);
static void SavePTSEntry(const HALDEC_Handle_t HALDecodeHandle, STFDMA_SCEntry_t* CPUPTSEntry_p);
static void ExecuteDecodeNow(const HALDEC_Handle_t HALDecodeHandle, void* DecodeAdd);
static void InternalProcessInterruptStatus(
           const HALDEC_Handle_t HALDecodeHandle, const U32 Status);
#if defined(DVD_SECURED_CHIP)
static void DMATransferDoneFct(U32 UserIdent, void * ES_Write_p, void * SCListWrite_p,void * SCListLoop_p, void * ESCopy_Write_p);
#else
static void DMATransferDoneFct(U32 UserIdent, void * ES_Write_p, void * SCListWrite_p,void * SCListLoop_p);
#endif /* defined(DVD_SECURED_CHIP)*/
static U8 GetAByteFromES(const HALDEC_Handle_t  HALDecodeHandle);
static void PointOnNextSC(LcmpegContext_t * LcmpegContext_p);

static STOS_INTERRUPT_DECLARE(SharedVideoInterruptHandler, Param);

#ifdef ST_deblock
static void HALDEC_TransformerCallback_deblocking(U32 Event, void *CallbackData, void * UserData);
#endif /* ST_deblock */

/* Global Variables --------------------------------------------------------- */

const HALDEC_FunctionsTable_t HALDEC_LcmpegFunctions =
{
    DecodingSoftReset,                  DisableSecondaryDecode,
    EnableSecondaryDecode,              DisablePrimaryDecode,
    EnablePrimaryDecode,                DisableHDPIPDecode,
    EnableHDPIPDecode,                  FlushDecoder,
    Get16BitsHeaderData,                Get16BitsStartCode,
    Get32BitsHeaderData,                GetAbnormallyMissingInterruptStatus,
    GetDataInputBufferParams,
    GetDecodeSkipConstraint,            GetBitBufferOutputCounter,
    GetCDFifoAlignmentInBytes,          GetDecodeBitBufferLevel,
#ifdef STVID_HARDWARE_ERROR_EVENT
    GetHardwareErrorStatus,
#endif /* STVID_HARDWARE_ERROR_EVENT  */
    GetInterruptStatus,                 GetLinearBitBufferTransferedDataSize,
    GetPTS,                             GetSCDAlignmentInBytes,
    GetStartCodeAddress,                GetStatus,
    Init,                               IsHeaderDataFIFOEmpty,
    LoadIntraQuantMatrix,               LoadNonIntraQuantMatrix,
    PipelineReset,                      PrepareDecode,
    ResetPESParser,                     SearchNextStartCode,
    SetBackwardChromaFrameBuffer,       SetBackwardLumaFrameBuffer,
    SetDataInputInterface,
    SetDecodeBitBuffer,                 SetDecodeBitBufferThreshold,
    SetDecodeChromaFrameBuffer,         SetDecodeConfiguration,
    SetDecodeLumaFrameBuffer,           SetForwardChromaFrameBuffer,
    SetForwardLumaFrameBuffer,
#ifdef ST_deblock
    SetDeblockingMode,
#ifdef VIDEO_DEBLOCK_DEBUG
    SetDeblockingStrength,
#endif /* VIDEO_DEBLOCK_DEBUG */
#endif /* ST_deblock */
    GetPictureDescriptors,
    SetFoundStartCode,
    SetInterruptMask,                   SetMainDecodeCompression,
    SetSecondaryDecodeChromaFrameBuffer,SetSecondaryDecodeLumaFrameBuffer,
    SetSkipConfiguration,               SetSmoothBackwardConfiguration,
    SetStreamID,                        SetStreamType,
    Setup,
    SkipPicture,                        StartDecodePicture,
    Term,
#ifdef STVID_DEBUG_GET_STATUS
	GetDebugStatus,
#endif /* STVID_DEBUG_GET_STATUS */
	WriteStartCode
};

#ifndef TRACE_UART
#define TraceBuffer(x)
#endif /* TRACE_UART */

#ifdef TRACE_LCMPEG_PICTURE_TYPE
#define TracePictureType(x) TraceMessage x
#else
#define TracePictureType(x)
#endif /* TRACE_DISPLAY_PICTURE_TYPE */


#ifdef TRACE_LCMPEG_FRAME_BUFFER
#define TraceFrameBuffer(x) TraceState x
#else
#define TraceFrameBuffer(x)
#endif /* TRACE_DISPLAY_FRAME_BUFFER */

#ifdef TRACE_LCMPEG_DURATION
#define TraceDuration(x) TraceState x
#else
#define TraceDuration(x)
#endif /* TRACE_DISPLAY_FRAME_BUFFER */

#ifdef TRACE_INTERRUPT
#define TraceInterrupt(x) TraceBuffer(x)
#else
#define TraceInterrupt(x)
#endif /* TRACE_INTERRUPT */

#ifdef TRACE_PERFORMANCES
#define TracePerf(x) TraceBuffer(x)
#else
#define TracePerf(x)
#endif /* TRACE_INTERRUPT */

#if defined (TRACE_SC) || defined (TRACE_HAL_DECODE)
#define TraceSC(x) TraceBuffer(x)
#else
#define TraceSC(x)
#endif /* (TRACE_SC) || (TRACE_HAL_DECODE) */

#ifdef TRACE_PTS
#define TracePTS(x) TraceBuffer(x)
#else
#define TracePTS(x)
#endif /* TRACE_PTS */

#ifdef TRACE_DECLIST
#define TraceDecList(x) TraceBuffer(x)
#else
#define TraceDecList(x)
#endif /* TRACE_DECLIST */

#ifdef SEMAPHORE_TRACE
static S32 MutexSmaphoreCount = 1;
#define SEMAPHORE_WAIT(x)   do\
                        {\
                            if((x) == MultiDecodeController.HW_DecoderAccess_p)\
                            {\
                                MutexSmaphoreCount--;\
                                TraceBuffer(("W%d C%d\r\n",__LINE__,MutexSmaphoreCount));\
                                semaphore_wait(x);\
                            }\
                            else\
                            {\
                                TraceBuffer(("W%d\r\n",__LINE__));\
                                semaphore_wait(x);\
                            }\
                        } while(0)
#define SEMAPHORE_SIGNAL(x)   do\
                        {\
                            if((x) == MultiDecodeController.HW_DecoderAccess_p)\
                            {\
                                MutexSmaphoreCount++;\
                                TraceBuffer(("S%d C%d\r\n",__LINE__,MutexSmaphoreCount));\
                                semaphore_signal(x);\
                            }\
                            else\
                            {\
                                TraceBuffer(("S%d\r\n",__LINE__));\
                                semaphore_signal(x);\
                            }\
                        } while(0)
#else /* !SEMAPHORE_TRACE */
#define SEMAPHORE_WAIT(x)   semaphore_wait(x)
#define SEMAPHORE_SIGNAL(x) semaphore_signal(x)
#endif /* SEMAPHORE_TRACE */

/* Private Macros ----------------------------------------------------------- */

/* shortcut to hals common data */
#define HAL_CONTEXT_P ((HALDEC_Properties_t *) HALDecodeHandle)

/* shortcut to lcmpeg context */
#define LCMPEG_CONTEXT_P ((LcmpegContext_t *)(HAL_CONTEXT_P)->PrivateData_p)

/* shortcut to read registers */
#define HAL_ReadReg32(Offset)   \
                STSYS_ReadRegDev32LE((void *) (((U8 *)HAL_CONTEXT_P\
                                    ->RegistersBaseAddress_p) + Offset))
/* shortcut to write registers */
#define HAL_WriteReg32(Offset, Value)\
                STSYS_WriteRegDev32LE((void *) (((U8 *)HAL_CONTEXT_P\
                                    ->RegistersBaseAddress_p) + Offset), (U32)(Value))

/* shortcut to sc entries fields */
#if defined(ST_7100) ||  defined(ST_7109)
/* On STi7100 Entry->SC.SCValue is defined as a bitfield, so we can't get its address with the & operator */
#define GetSCVal(Entry)  (STSYS_ReadRegMemUncached8((U8*)((U32)(Entry) + 8)))
#else
#define GetSCVal(Entry)  (STSYS_ReadRegMemUncached8((U8*)&((Entry)->SC.SCValue)))
#endif /* ST_7100 || ST_7109 */
#define GetSCAdd(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr)))
#define GetPTS32(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->PTS.PTS0)))
#define GetPTS33(Entry)  ((STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->PTS.PTS1)) == 1) ? TRUE : FALSE)
/* The Entry->SC.Type field is a bitfield, so we can't get its address with the & operator */
#define IsPTS(Entry)     (((STSYS_ReadRegMemUncached32LE((U32*)(Entry)) & 3) != STFDMA_SC_ENTRY )? TRUE : FALSE)

#define INSIDE_TASK() (task_context(NULL, NULL) == task_context_task)

/* PTS List access shortcuts */
#define PTSLIST_BASE     (LCMPEG_CONTEXT_P->PTS_SCList.SCList)
#define PTSLIST_READ     (LCMPEG_CONTEXT_P->PTS_SCList.Read_p)
#define PTSLIST_WRITE    (LCMPEG_CONTEXT_P->PTS_SCList.Write_p)
#define PTSLIST_TOP      (&LCMPEG_CONTEXT_P->PTS_SCList.SCList[PTS_SCLIST_SIZE])


/* Functions -(exported with the fct table)---------------------------------- */
#ifdef WA_GNBvd63015
/*******************************************************************************
Name        : ProcessSharedVideoDeblockingInterrupt
Description : Process Shared Video Deblocking Interrupt
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ProcessSharedVideoDeblockingInterrupt(const HALDEC_Handle_t  HALDecodeHandle)
{
    LcmpegContext_t *       LcmpegContext_p;
    HALDEC_Properties_t *   Hal_Context_p;

    Hal_Context_p = (HALDEC_Properties_t *) HALDecodeHandle;
    LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
    /* Wait Info from SharedVideoInterruptHandler */
    while(LcmpegContext_p->ProcessSharedVideoDeblockingInterruptIsRunning)
    {
    SEMAPHORE_WAIT(LcmpegContext_p->SharedAccessToMME_p);
        if (LcmpegContext_p->ProcessSharedVideoDeblockingInterruptIsRunning)
        {
            VID_DeblockingStart(&LcmpegContext_p->DeblockingHandle,LcmpegContext_p->DeblockTransfromParams);
        }
    }
}
#endif /* WA_GNBvd63015 */
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
    VIDINJ_GetInjectParams_t                Params;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    STAVMEM_FreeBlockParams_t               FreeParams;
    void *                                  VirtualAddress;
    ST_ErrorCode_t                          ErrorCode;
    U32                                     SCList_Size;
    static char                             IrqName[] = STVID_IRQ_NAME;
#if defined(LCMPEGX1_WA_GNBvd45748)
    void *                                  RecoveryStreamVirtualAddr_p;
#endif /* defined(LCMPEGX1_WA_GNBvd45748) */
#ifdef ST_deblock
    VID_Deblocking_InitParams_t                 DeblockInitParams_p;
#endif /* ST_deblock */
#ifdef WA_GNBvd63015
    VIDCOM_Task_t *                        SharedAccessToMMETask_p;
    char TaskName[23] = "STVID[?].DeblockingTask";
#endif /* WA_GNBvd63015 */


    /* Allocate a LcmpegContext_t structure that holds the current instance datas */
    /*----------------------------------------------------------------------------*/
    SAFE_MEMORY_ALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                         void *,
                         HAL_CONTEXT_P->CPUPartition_p,
                         sizeof(LcmpegContext_t));
    if (LCMPEG_CONTEXT_P == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Initialize the LcmpegContext_t structure*/
    /*---------------------------------------*/
    memset(LCMPEG_CONTEXT_P, 0, sizeof(LcmpegContext_t));
    LCMPEG_CONTEXT_P->PTS_SCList.Write_p                 = (PTS_t*)LCMPEG_CONTEXT_P->PTS_SCList.SCList;
    LCMPEG_CONTEXT_P->PTS_SCList.Read_p                  = (PTS_t*)LCMPEG_CONTEXT_P->PTS_SCList.SCList;
    LCMPEG_CONTEXT_P->SCDetectSimuLaunched               = FALSE;
    LCMPEG_CONTEXT_P->SkipNextStartCode                  = FALSE;
    LCMPEG_CONTEXT_P->SharedAccess_p = STOS_SemaphoreCreateFifo(HAL_CONTEXT_P->CPUPartition_p, 1) ;

#ifdef BENCHMARK_WINCESTAPI
	P_ADDSEMAPHORE(LCMPEG_CONTEXT_P->SharedAccess_p , "LCMPEG_CONTEXT_P->SharedAccess_p");
#endif

    ErrorCode = STAVMEM_GetSharedMemoryVirtualMapping(&LCMPEG_CONTEXT_P->VirtualMapping);
    if (ErrorCode != ST_NO_ERROR)
    {
        SAFE_MEMORY_DEALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                               HAL_CONTEXT_P->CPUPartition_p,
                               sizeof(LcmpegContext_t));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Register the event used to notify an interrupt to decode/top level */
    /*--------------------------------------------------------------------*/
    ErrorCode = STEVT_RegisterDeviceEvent(HAL_CONTEXT_P->EventsHandle,
                              HAL_CONTEXT_P->VideoName,
                              HAL_CONTEXT_P->InterruptEvt,
                              &LCMPEG_CONTEXT_P->InterruptEvtID);

    if (ErrorCode != ST_NO_ERROR)
    {
        SAFE_MEMORY_DEALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                               HAL_CONTEXT_P->CPUPartition_p,
                               sizeof(LcmpegContext_t));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate an Input Buffer used for PES injection*/
    /*------------------------------------------------*/
    AllocBlockParams.PartitionHandle          = HAL_CONTEXT_P->AvmemPartition;
    AllocBlockParams.Alignment                = 128; /* limit for FDMA */
    AllocBlockParams.Size                     = INPUT_BUFFER_SIZE;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &LCMPEG_CONTEXT_P->InputBufHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        SAFE_MEMORY_DEALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                               HAL_CONTEXT_P->CPUPartition_p,
                               sizeof(LcmpegContext_t));
        return(ST_ERROR_NO_MEMORY);
    }

    ErrorCode = STAVMEM_GetBlockAddress(LCMPEG_CONTEXT_P->InputBufHdl, (void **)&VirtualAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        FreeParams.PartitionHandle = HAL_CONTEXT_P->AvmemPartition;
        STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->InputBufHdl));
        SAFE_MEMORY_DEALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                               HAL_CONTEXT_P->CPUPartition_p,
                               sizeof(LcmpegContext_t));
        return(ST_ERROR_NO_MEMORY);
    }

    /* Remember the input buffer extents :
     * - InputBuffBase_p = pointer on the first byte of the buffer
     * - InputBuffTop_p  = pointer on the last byte of the buffer */
    LCMPEG_CONTEXT_P->InputBuffBase_p  = STAVMEM_VirtualToDevice(VirtualAddress, &LCMPEG_CONTEXT_P->VirtualMapping);
    LCMPEG_CONTEXT_P->InputBuffTop_p   = (void *) ((U32)LCMPEG_CONTEXT_P->InputBuffBase_p + INPUT_BUFFER_SIZE - 1);

    /* Allocate the startcode list*/
    SCList_Size = ((SCLIST_SIZE_STANDARD * HAL_CONTEXT_P->BitBufferSize) + (BITBUFFER_SIZE_STANDARD - 1)) / BITBUFFER_SIZE_STANDARD ;

    AllocBlockParams.PartitionHandle          = HAL_CONTEXT_P->AvmemPartition;
    AllocBlockParams.Alignment                = SCLIST_ALIGNEMENT; /* limit for FDMA */
    AllocBlockParams.Size                     = SCList_Size * sizeof(STFDMA_SCEntry_t);
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &LCMPEG_CONTEXT_P->SCListHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        FreeParams.PartitionHandle = HAL_CONTEXT_P->AvmemPartition;
        STAVMEM_FreeBlock(&FreeParams, &(LCMPEG_CONTEXT_P->InputBufHdl));
        SAFE_MEMORY_DEALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                               HAL_CONTEXT_P->CPUPartition_p,
                               sizeof(LcmpegContext_t));
        return(ST_ERROR_NO_MEMORY);
    }

    ErrorCode = STAVMEM_GetBlockAddress(LCMPEG_CONTEXT_P->SCListHdl, (void **)&LCMPEG_CONTEXT_P->AlignedSCList_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        FreeParams.PartitionHandle = HAL_CONTEXT_P->AvmemPartition;
        STAVMEM_FreeBlock(&FreeParams, &(LCMPEG_CONTEXT_P->SCListHdl));
        STAVMEM_FreeBlock(&FreeParams, &(LCMPEG_CONTEXT_P->InputBufHdl));
        SAFE_MEMORY_DEALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                               HAL_CONTEXT_P->CPUPartition_p,
                               sizeof(LcmpegContext_t));
        return(ST_ERROR_NO_MEMORY);
    }

    /* Convert AlignedSCList_p value to a physical address */
    LCMPEG_CONTEXT_P->AlignedSCList_p = STAVMEM_VirtualToDevice(LCMPEG_CONTEXT_P->AlignedSCList_p, &LCMPEG_CONTEXT_P->VirtualMapping);

    /* The current allocation of the startcode list for the linear bitbuffers supposes that one linear bitbuffer size, is
     * equal to the half of the circular bitbuffer size. So, the circular bitbuffer startcode list is re-used for the linear
     * bitbuffers. */
    LCMPEG_CONTEXT_P->ES_CircularBitBuffer.SCList_Base_p = (STFDMA_SCEntry_t*)LCMPEG_CONTEXT_P->AlignedSCList_p;
    LCMPEG_CONTEXT_P->ES_CircularBitBuffer.SCList_Top_p  = (STFDMA_SCEntry_t*)(  ((U32)LCMPEG_CONTEXT_P->AlignedSCList_p)
                                                                   + ((SCList_Size-1) * sizeof(STFDMA_SCEntry_t)));

    LCMPEG_CONTEXT_P->ES_LinearBitBufferA.SCList_Base_p = (STFDMA_SCEntry_t*)(LCMPEG_CONTEXT_P->AlignedSCList_p);
    LCMPEG_CONTEXT_P->ES_LinearBitBufferA.SCList_Top_p  = (STFDMA_SCEntry_t*)(  ((U32)LCMPEG_CONTEXT_P->AlignedSCList_p)
                                                                                + (((SCList_Size/2)-1) * sizeof(STFDMA_SCEntry_t)));

    LCMPEG_CONTEXT_P->ES_LinearBitBufferB.SCList_Base_p = (STFDMA_SCEntry_t*)(  ((U32)LCMPEG_CONTEXT_P->AlignedSCList_p)
                                                                                + ((SCList_Size/2) * sizeof(STFDMA_SCEntry_t)));
    LCMPEG_CONTEXT_P->ES_LinearBitBufferB.SCList_Top_p  = (STFDMA_SCEntry_t*)(  ((U32)LCMPEG_CONTEXT_P->AlignedSCList_p)
                                                                              + ((SCList_Size-1) * sizeof(STFDMA_SCEntry_t)));

/* Map decoder registers */
#ifdef ST_OSWINCE
    /* check that the base address is as expected for ST7100 */
    WCE_VERIFY(HAL_CONTEXT_P->RegistersBaseAddress_p == (void*)VIDEO_BASE_ADDRESS);

    HAL_CONTEXT_P->RegistersBaseAddress_p = MapPhysicalToVirtual(HAL_CONTEXT_P->RegistersBaseAddress_p,
                                                                 VIDEO_ADDRESS_WIDTH);
	if (HAL_CONTEXT_P->RegistersBaseAddress_p == NULL)
	{
		WCE_ERROR("MapPhysicalToVirtual(ST710x_VIDEO_BASE_ADDRESS)");
		return ST_ERROR_NO_MEMORY;
	}
#endif /* ST_OSWINCE */

    /* link this init with a fdma channel : call inject sub-module */
    /*-------------------------------------------------------------*/
    Params.TransferDoneFct          = DMATransferDoneFct;
    Params.UserIdent                = (U32)HALDecodeHandle;
    Params.AvmemPartition           = AllocBlockParams.PartitionHandle;
    Params.CPUPartition             = HAL_CONTEXT_P->CPUPartition_p;
    LCMPEG_CONTEXT_P->InjectHdl     = VIDINJ_TransferGetInject(HAL_CONTEXT_P->InjecterHandle, &Params);
    if (LCMPEG_CONTEXT_P->InjectHdl == BAD_INJECT_NUM)
    {
        FreeParams.PartitionHandle = HAL_CONTEXT_P->AvmemPartition;
        STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->SCListHdl));
        STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->InputBufHdl));
        SAFE_MEMORY_DEALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                               HAL_CONTEXT_P->CPUPartition_p,
                               sizeof(LcmpegContext_t));
        return(ST_ERROR_NO_MEMORY);
    }

#ifdef ST_deblock
    LCMPEG_CONTEXT_P->DeblockingEnabled = FALSE;
    LCMPEG_CONTEXT_P->DecodeParams.CurrentDeblockingState = FALSE;
    LCMPEG_CONTEXT_P->DeblockingStrength = 15;

    /*            link this init with deblocking module            */
    /*-------------------------------------------------------------*/
    DeblockInitParams_p.CPUPartition_p = HAL_CONTEXT_P->CPUPartition_p;
    DeblockInitParams_p.DeltaBaseAddress_p = (void *)DELTA_BASE_ADDRESS;
    DeblockInitParams_p.AvmemPartitionHandle = HAL_CONTEXT_P->AvmemPartition;
    DeblockInitParams_p.EndDeblockingCallbackFuction = &HALDEC_TransformerCallback_deblocking;

    ErrorCode = VID_DeblockingInit (&DeblockInitParams_p, &LCMPEG_CONTEXT_P->DeblockingHandle);

    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error when initilizing Deblocking module \n"));
    }
#endif /* ST_deblock */

#if defined(LCMPEGX1_WA_GNBvd45748)
    LCMPEG_CONTEXT_P->RecoveryStream_p = NULL;
    LCMPEG_CONTEXT_P->RecoveryLuma_p = NULL;
    LCMPEG_CONTEXT_P->RecoveryChroma_p = NULL;
    AllocBlockParams.PartitionHandle          = HAL_CONTEXT_P->AvmemPartition;
    AllocBlockParams.Alignment                = 1024;
    AllocBlockParams.Size                     = 6 * 1024;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &LCMPEG_CONTEXT_P->WAGNBvd45748BuffersHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        FreeParams.PartitionHandle = HAL_CONTEXT_P->AvmemPartition;
        STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->SCListHdl));
        STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->InputBufHdl));
        SAFE_MEMORY_DEALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                               HAL_CONTEXT_P->CPUPartition_p,
                               sizeof(LcmpegContext_t));
        return(ST_ERROR_NO_MEMORY);
    }

    ErrorCode = STAVMEM_GetBlockAddress(LCMPEG_CONTEXT_P->WAGNBvd45748BuffersHdl, (void **)&LCMPEG_CONTEXT_P->RecoveryStream_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        FreeParams.PartitionHandle = HAL_CONTEXT_P->AvmemPartition;
        STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->WAGNBvd45748BuffersHdl));
        STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->SCListHdl));
        STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->InputBufHdl));
        SAFE_MEMORY_DEALLOCATE(HAL_CONTEXT_P->PrivateData_p,
                               HAL_CONTEXT_P->CPUPartition_p,
                               sizeof(LcmpegContext_t));
        return(ST_ERROR_NO_MEMORY);
    }

    RecoveryStreamVirtualAddr_p = STAVMEM_CPUToVirtual(&RecoveryStream[0], &LCMPEG_CONTEXT_P->VirtualMapping);
    ErrorCode = STAVMEM_CopyBlock1D(RecoveryStreamVirtualAddr_p, LCMPEG_CONTEXT_P->RecoveryStream_p, WA_GNBvd45748_STREAM_SIZE);
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Convert RecoveryStream_p value to a physical address */
        LCMPEG_CONTEXT_P->RecoveryStream_p = STAVMEM_VirtualToDevice(LCMPEG_CONTEXT_P->RecoveryStream_p, &LCMPEG_CONTEXT_P->VirtualMapping);
        LCMPEG_CONTEXT_P->RecoveryLuma_p = (void*)((U32)LCMPEG_CONTEXT_P->RecoveryStream_p + 2048);
        LCMPEG_CONTEXT_P->RecoveryChroma_p = (void*)((U32)LCMPEG_CONTEXT_P->RecoveryStream_p + 2048 + 2048);
    }
#endif /* defined(LCMPEGX1_WA_GNBvd45748) */

#ifdef WA_GNBvd63015
    TaskName[6] = (char) ((U8) '0' + HAL_CONTEXT_P->DecoderNumber);
    LCMPEG_CONTEXT_P->SharedAccessToMME_p = STOS_SemaphoreCreateFifo(HAL_CONTEXT_P->CPUPartition_p, 0);
    LCMPEG_CONTEXT_P->ProcessSharedVideoDeblockingInterruptIsRunning = TRUE;
    SharedAccessToMMETask_p = &(LCMPEG_CONTEXT_P->SharedAccessToMMETask);
    ErrorCode = STOS_TaskCreate ((void (*) (void*))ProcessSharedVideoDeblockingInterrupt,
                            HALDecodeHandle,
                            HAL_CONTEXT_P->CPUPartition_p,
                            STVID_TASK_STACK_SIZE_DECODE,
                            &(SharedAccessToMMETask_p->TaskStack),
                            HAL_CONTEXT_P->CPUPartition_p,
                            &(SharedAccessToMMETask_p->Task_p),
                            &(SharedAccessToMMETask_p->TaskDesc),
                            STVID_TASK_PRIORITY_DECODE,
                            TaskName ,
                            task_flags_no_min_stack_size);
#endif /* WA_GNBvd63015 */


    /* Init the multi-decode controller only for the first instance */
    /*--------------------------------------------------------------*/
    /* First, we initialize the inter-instance semaphore */
    if (MultiDecodeController.HW_DecoderAccess_p == NULL)
    {
        semaphore_t *TemporaryHW_DecoderAccess_p;
        BOOL         TemporarySemaphoreNeedsToBeDeleted;

        TemporaryHW_DecoderAccess_p = STOS_SemaphoreCreatePriority(HAL_CONTEXT_P->CPUPartition_p, 1);
#ifdef BENCHMARK_WINCESTAPI
		P_ADDSEMAPHORE(TemporaryHW_DecoderAccess_p , "TemporaryHW_DecoderAccess_p");
#endif


        STOS_TaskLock();

        if (MultiDecodeController.HW_DecoderAccess_p == NULL)
        {
            MultiDecodeController.HW_DecoderAccess_p = TemporaryHW_DecoderAccess_p;
            TemporarySemaphoreNeedsToBeDeleted = FALSE;
        }
        else
        {
            /* Remember to delete the temporary semaphore, if the MultiDecodeController.HW_DecoderAccess_p
                * was already created by another video instance */
            TemporarySemaphoreNeedsToBeDeleted = TRUE;
        }

        STOS_TaskUnlock();

        /* Delete the temporary semaphore */
        if(TemporarySemaphoreNeedsToBeDeleted)
        {
           STOS_SemaphoreDelete(HAL_CONTEXT_P->CPUPartition_p, TemporaryHW_DecoderAccess_p);
        }
    }

    /* Second, we initialize the structure */
    SEMAPHORE_WAIT(MultiDecodeController.HW_DecoderAccess_p);

    /* Only emulates shared IRQ instead of using the feature since not available yet for all OSes in STOS (2.2.3) */
    /* once this feature is available in all OSes, this should be modified for a cleaner code ... */
    if (MultiDecodeController.NbInstalledIRQ == 0)
    {
        MultiDecodeController.RegistersBaseAddress_p = (U8 *)HAL_CONTEXT_P->RegistersBaseAddress_p;
        /* Disable the interrupts at init time, interrupt mask will be
            programmed just before starting the decode. */
        HAL_WriteReg32(VID_ITM, 0x0);
#if defined(ST_5100) || defined(ST_7710)
        /* Interconnect access configuration */
        HAL_WriteReg32(CFG_VIDIC, 0x54);
#endif /* ST_5100 || ST_7710*/

        /* install the unique It handler for all instances */
        STOS_InterruptLock();
        STOS_InterruptInstall(
                        HAL_CONTEXT_P->InterruptNumber,
                        HAL_CONTEXT_P->InterruptLevel,
                        SharedVideoInterruptHandler,
                        IrqName,
                        (void *) NULL);
        STOS_InterruptUnlock();

        if (STOS_InterruptEnable(HAL_CONTEXT_P->InterruptNumber, HAL_CONTEXT_P->InterruptLevel) != STOS_SUCCESS)
        {
            MultiDecodeController.NbInstalledIRQ ++;
            SEMAPHORE_SIGNAL(MultiDecodeController.HW_DecoderAccess_p);
            return(ST_ERROR_BAD_PARAMETER);
        }
    }

    MultiDecodeController.NbInstalledIRQ ++;
    SEMAPHORE_SIGNAL(MultiDecodeController.HW_DecoderAccess_p);


    return(ST_NO_ERROR);
} /* End of Init() function */

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
    HALDEC_Properties_t *   Hal_Context_p;
    LcmpegContext_t *       LcmpegContext_p;
#ifdef ST_deblock
    BOOL                    DeblockingEnabled;
    int                     DeblockingStrength;
#endif /* ST_deblock */
    BOOL                    HardResetDone = FALSE;

    TraceBuffer((">> SOFT RESET <<\r\n"));

    /* Ensure no FDMA transfer is done while setting the decode buffer */
    VIDINJ_EnterCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);

    /* Ensure that injection module is stopped to have no update of SCLIST and ES write pointers, while resetting them */
    /* VIDINJ_TransferStop(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl); */
    /* Removed because unusefull, as we will start the injecter later under the
     * same critical section. */

    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    /* Avoid that MultiDecodeController.HALDecodeHandleForNextIT is changed
        by the interrupt context */
    STOS_InterruptLock();
    if(MultiDecodeController.HALDecodeHandleForNextIT == HALDecodeHandle)
    {
        /* The current instance holds the hardware video decoder, so we have to reset the hardware decoder
            in order to release the mutex. */

        /* Reset the hardware */
        HAL_WriteReg32(VID_SRS, 0xFF);
        HardResetDone = TRUE;
    }
    STOS_InterruptUnlock();

    /* Reset ES and StartCodeList pointers  */
    LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p      = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
    LCMPEG_CONTEXT_P->DecBuffer_p->ES_Write_p     = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
    LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Write_p = LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Base_p;
    LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Loop_p  = LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Base_p;
    LCMPEG_CONTEXT_P->DecBuffer_p->LastSCAdd_p    = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
    LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p  = NULL;

    LCMPEG_CONTEXT_P->InjBuffer_p->ES_Read_p      = LCMPEG_CONTEXT_P->InjBuffer_p->ES_Base_p;
    LCMPEG_CONTEXT_P->InjBuffer_p->ES_Write_p     = LCMPEG_CONTEXT_P->InjBuffer_p->ES_Base_p;
    LCMPEG_CONTEXT_P->InjBuffer_p->SCList_Write_p = LCMPEG_CONTEXT_P->InjBuffer_p->SCList_Base_p;
    LCMPEG_CONTEXT_P->InjBuffer_p->SCList_Loop_p  = LCMPEG_CONTEXT_P->InjBuffer_p->SCList_Base_p;
    LCMPEG_CONTEXT_P->InjBuffer_p->LastSCAdd_p    = LCMPEG_CONTEXT_P->InjBuffer_p->ES_Base_p;
    LCMPEG_CONTEXT_P->InjBuffer_p->NextSCEntry_p  = NULL;

    LCMPEG_CONTEXT_P->NextDataPointer_p           = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
    LCMPEG_CONTEXT_P->PictureAdd                  = NULL;
    LCMPEG_CONTEXT_P->PTS_SCList.Write_p          = (PTS_t*)LCMPEG_CONTEXT_P->PTS_SCList.SCList;
    LCMPEG_CONTEXT_P->PTS_SCList.Read_p           = (PTS_t*)LCMPEG_CONTEXT_P->PTS_SCList.SCList;
    LCMPEG_CONTEXT_P->LastPTSStored.Address_p     = NULL;

    /* Reset flags */
    LCMPEG_CONTEXT_P->DecodeEnabled               = FALSE;
#if defined(LCMPEGH1)
    LCMPEG_CONTEXT_P->SecondaryDecodeEnabled      = FALSE;
#endif /* LCMPEGH1 */
    LCMPEG_CONTEXT_P->OverWriteEnabled            = FALSE;
    LCMPEG_CONTEXT_P->EndOfSlicesSC               = FALSE;
    LCMPEG_CONTEXT_P->FirstSequenceReached        = FALSE;
    LCMPEG_CONTEXT_P->SkipNextStartCode           = FALSE;
    LCMPEG_CONTEXT_P->SCDetectSimuLaunched        = FALSE;

    /* save DeblockingEnabled and DeblockingStrength values before reseting the decode params */
#ifdef ST_deblock
    DeblockingEnabled = LCMPEG_CONTEXT_P->DeblockingEnabled;
    DeblockingStrength = LCMPEG_CONTEXT_P->DeblockingStrength;
#endif /* ST_deblock */
    /* Reset the decode params structure */
    memset(&LCMPEG_CONTEXT_P->DecodeParams, 0, sizeof(LCMPEG_CONTEXT_P->DecodeParams));

#ifdef ST_deblock
    /* rewrite the correct value of DeblockingEnabled */
    LCMPEG_CONTEXT_P->DeblockingEnabled =  DeblockingEnabled;
    LCMPEG_CONTEXT_P->DecodeParams.CurrentDeblockingState = FALSE;
    LCMPEG_CONTEXT_P->DeblockingStrength = DeblockingStrength;

#endif /* ST_deblock */

    /* Reset pointers in injection module */
    VIDINJ_TransferReset(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    /* Start injection now */
    VIDINJ_TransferStart(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl, IsRealTime);

    /* Notify a DECODER IDLE INTERRUPT in a task context only if a hard reset
     * wasn't done because a hard reset will notify the DECODER IDLE INTERRUPT
     * with an interruption. */
    if(!HardResetDone)
    {
        /* Simulate a decoder idle interrupt */
        Hal_Context_p = (HALDEC_Properties_t *) HALDecodeHandle;
        LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
        LCMPEG_CONTEXT_P->SWStatus |= HALDEC_DECODER_IDLE_MASK;
        LCMPEG_CONTEXT_P->SWStatus |= HALDEC_PIPELINE_IDLE_MASK;
        if((LCMPEG_CONTEXT_P->SWStatus & LCMPEG_CONTEXT_P->SWInterruptMask) != 0)
        {
            STEVT_Notify(Hal_Context_p->EventsHandle,
                LcmpegContext_p->InterruptEvtID,
                STEVT_EVENT_DATA_TYPE_CAST NULL);
        }
    }
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);

    /* Launch startcode detection */
    if(LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
    {
        SearchNextStartCode(HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);
    }
} /* End of DecodingSoftReset() function */

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
    HALDEC_Properties_t  *  Hal_Context_p;
    LcmpegContext_t *       LcmpegContext_p;
    BOOL                  HardResetDone = FALSE;

    TraceBuffer((">> PIPELINE RESET <<\r\n"));

    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    /* Avoid that MultiDecodeController.HALDecodeHandleForNextIT is changed
        by the interrupt context */
    STOS_InterruptLock();
    if(MultiDecodeController.HALDecodeHandleForNextIT == HALDecodeHandle)
    {
        /* The current instance holds the hardware video decoder, so we have to reset the hardware decoder
            in order to release the mutex. */

        /* Reset the hardware */
        HAL_WriteReg32(VID_SRS, 0xFF);
        HardResetDone = TRUE;
    }
    STOS_InterruptUnlock();

    /* Reset flags */
    LCMPEG_CONTEXT_P->OverWriteEnabled   = FALSE;

    /* Notify a DECODER IDLE INTERRUPT in a task context only if a hard reset
        * wasn't done because a hard reset will notify the DECODER IDLE INTERRUPT
        * with an interruption. */
    if(!HardResetDone)
    {
        /* Simulate a decoder idle interrupt */
        Hal_Context_p = (HALDEC_Properties_t *) HALDecodeHandle;
        LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
        /* Raise a Decoder Idle interrupt */
        LCMPEG_CONTEXT_P->SWStatus |= HALDEC_PIPELINE_IDLE_MASK;
        LCMPEG_CONTEXT_P->SWStatus |= HALDEC_DECODER_IDLE_MASK;
        if((LCMPEG_CONTEXT_P->SWStatus & LCMPEG_CONTEXT_P->SWInterruptMask) != 0)
        {
            STEVT_Notify(Hal_Context_p->EventsHandle,
                LcmpegContext_p->InterruptEvtID,
                STEVT_EVENT_DATA_TYPE_CAST NULL);
        }
    }
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
} /* End of PipelineReset() function */

#if defined(LCMPEGX1_WA_GNBvd45748)
/*******************************************************************************
Name        : HALDEC_LCMPEGX1GNBvd45748WA
Description :
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HALDEC_LCMPEGX1GNBvd45748WA(const HALDEC_Handle_t  HALDecodeHandle)
{
    U32                     i;
    U32 TryLoops;
    U32 InterruptMask;
    U32 vidreq, vidits;
    BOOL    CurrentInstanceHoldingTheHardwareDecoder = FALSE;

    TraceBuffer((">> PIPE RESET PENDING<<\r\n"));
    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    /* Avoid that MultiDecodeController.HALDecodeHandleForNextIT is changed
     by the interrupt context */
    STOS_InterruptLock();
    if(MultiDecodeController.HALDecodeHandleForNextIT == HALDecodeHandle)
    {
        /* The current instance holds the hardware video decoder, but might never release it because
            the hardware is in an inconsistent state.*/

        /* Disable the processing of the interrupts by setting no instance to handle the following interrupts) */
        /* This will avoid that we make a lot of semaphore signal on the mutex protecting the hardware, because
            of the actions done in the WA. */
        /* Be carefull, because the hardware decoder is still locked by the semaphore for the current instance. */
        MultiDecodeController.HALDecodeHandleForNextIT = NULL;

        /* Remember to apply the WA */
        CurrentInstanceHoldingTheHardwareDecoder = TRUE;
    }
    STOS_InterruptUnlock();

    if (!CurrentInstanceHoldingTheHardwareDecoder)
    {
        /* Lock the hardware */
        SEMAPHORE_WAIT(MultiDecodeController.HW_DecoderAccess_p);
    }

    /* Apply the workaround */
    if ((LCMPEG_CONTEXT_P->RecoveryStream_p != NULL) && (LCMPEG_CONTEXT_P->RecoveryLuma_p != NULL) && (LCMPEG_CONTEXT_P->RecoveryChroma_p != NULL))
    {
        /* Get the interrupt mask */
        InterruptMask = HAL_ReadReg32(VID_ITM);
        /* Mask all ITs to prevent any SW instance to catch unexpected interrupts */
        HAL_WriteReg32(VID_ITM, 0x0);

        for (i=0; i < 10 ;i++)
        {
            /* Reset decoder */
            HAL_WriteReg32(VID_SRS, 0xFF);
            HAL_WriteReg32(VID_TIS, 0x00);

            /* Prepare decoder */
            HAL_WriteReg32(VID_DFH, 0x02);
            HAL_WriteReg32(VID_DFW, 0x02);
            HAL_WriteReg32(VID_DFS, 0x04);

            HAL_WriteReg32(VID_RFP, ((U32)LCMPEG_CONTEXT_P->RecoveryLuma_p));
            HAL_WriteReg32(VID_FFP, ((U32)LCMPEG_CONTEXT_P->RecoveryLuma_p));
            HAL_WriteReg32(VID_RCHP, ((U32)LCMPEG_CONTEXT_P->RecoveryChroma_p));
            HAL_WriteReg32(VID_FCHP, ((U32)LCMPEG_CONTEXT_P->RecoveryChroma_p));
            HAL_WriteReg32(VID_VLDPTR, ((U32)LCMPEG_CONTEXT_P->RecoveryStream_p));
            HAL_WriteReg32(VID_BBS, ((U32)LCMPEG_CONTEXT_P->RecoveryStream_p));
            HAL_WriteReg32(VID_BBE, ((U32)LCMPEG_CONTEXT_P->RecoveryStream_p+512));
            HAL_WriteReg32(VID_VLDRL, ((U32)LCMPEG_CONTEXT_P->RecoveryStream_p+512));
#if defined(LCMPEGH1)
            HAL_WriteReg32(VID_RCM, 0x04);
#endif /* LCMPEGH1 */
            HAL_WriteReg32(VID_PPR, 0x4d13ffff);

            /* Start decoder */
            HAL_WriteReg32(VID_EXE, 0x000000ff);
            TryLoops = 0;
            do
            {
                TryLoops++;
                vidits = HAL_ReadReg32(VID_ITS);
            }
            while (((vidits & 0x1) == 0x0) && (TryLoops < 11));

            vidreq = HAL_ReadReg32(VID_REQS);
            if (vidreq == 0) break;
        }
        /* Finish with a SRS to clean up IT status */
        HAL_WriteReg32(VID_SRS, 0xFF);
        vidits = HAL_ReadReg32(VID_ITS);
        /* restore interrupt mask */
        HAL_WriteReg32(VID_ITM, InterruptMask);
    }

    SEMAPHORE_SIGNAL(MultiDecodeController.HW_DecoderAccess_p);
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
} /* End of HALDEC_LCMPEGX1GNBvd45748WA() function */
#endif /* defined(LCMPEGX1_WA_GNBvd45748) */

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
    /* No pes parser in this IP: nothing to do ? */
    UNUSED_PARAMETER(HALDecodeHandle);
} /* End of ResetPESParser() function */

/*******************************************************************************
Name        : SetDecodeBitBuffer
Description : Set the location of the decoder bit buffer
Parameters  : HAL Decode manager handle, buffer address, buffer size in bytes
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeBitBuffer(
                         const HALDEC_Handle_t        HALDecodeHandle,
                         void * const                 BufferAddressStart_p,
                         const U32                    BufferSize,
                         const HALDEC_BitBufferType_t BufferType)
{
    U8 * ESCopy_Base_p;
    U8 * ESCopy_Top_p;

    /* Ensure no FDMA transfer is done while setting a new decode buffer */
    VIDINJ_EnterCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    LCMPEG_CONTEXT_P->BitBufferType =  BufferType;
    switch (BufferType)
    {
        case HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR :
            /* we assume that the circular bit buffer is used at speed 100  */
            /* normal case: buffer under parse-decode = buffer under inject */
            LCMPEG_CONTEXT_P->InjBuffer_p = &LCMPEG_CONTEXT_P->ES_CircularBitBuffer;
            LCMPEG_CONTEXT_P->DecBuffer_p = LCMPEG_CONTEXT_P->InjBuffer_p;
            LCMPEG_CONTEXT_P->FirstSequenceReached  = FALSE;
            TracePerf((":%3d:BUFFC\r\n", time_now() / (ST_GetClocksPerSecond() / 1000)));
			TraceBuffer(("===Buffer C===\r\n"));
        break;

        case HALDEC_BIT_BUFFER_SW_MANAGED_LINEAR:
            /* we assume that in this case; one buffer is under injection */
            /* while the other is under parse-decode                      */
            /* Case 1 : The HAL already knows this linear buffer, so we can easily choose
             * between buffer A and buffer B */
            if(BufferAddressStart_p == LCMPEG_CONTEXT_P->ES_LinearBitBufferA.ES_Base_p)
            {
                LCMPEG_CONTEXT_P->InjBuffer_p = &LCMPEG_CONTEXT_P->ES_LinearBitBufferA;
                TracePerf((":%3d:BUFFA\r\n", time_now() / (ST_GetClocksPerSecond() / 1000)));
                TraceBuffer(("===Buffer A : %08x===\r\n", BufferAddressStart_p));
            }
            else if(BufferAddressStart_p == LCMPEG_CONTEXT_P->ES_LinearBitBufferB.ES_Base_p)
            {
                LCMPEG_CONTEXT_P->InjBuffer_p = &LCMPEG_CONTEXT_P->ES_LinearBitBufferB;
                TracePerf((":%3d:BUFFB\r\n", time_now() / (ST_GetClocksPerSecond() / 1000)));
                TraceBuffer(("===Buffer B : %08x===\r\n", BufferAddressStart_p));
            }
            /* Case 2 : This linear buffer is a new allocated buffer, so we have to avoid
             * to choose the same injection buffer as before */
            else if(LCMPEG_CONTEXT_P->InjBuffer_p == &LCMPEG_CONTEXT_P->ES_LinearBitBufferA)
            {
                LCMPEG_CONTEXT_P->InjBuffer_p = &LCMPEG_CONTEXT_P->ES_LinearBitBufferB;
                TracePerf((":%3d:BUFFB\r\n", time_now() / (ST_GetClocksPerSecond() / 1000)));
                TraceBuffer(("===Buffer B : %08x===\r\n", BufferAddressStart_p));
            }
            else
            {
                LCMPEG_CONTEXT_P->InjBuffer_p = &LCMPEG_CONTEXT_P->ES_LinearBitBufferA;
                TracePerf((":%3d:BUFFA\r\n", time_now() / (ST_GetClocksPerSecond() / 1000)));
                TraceBuffer(("===Buffer A : %08x===\r\n", BufferAddressStart_p));
            }
            break;
        default:
            break;
    }
    /* Set default values for pointers */
    LCMPEG_CONTEXT_P->InjBuffer_p->ES_Base_p        = BufferAddressStart_p;
    LCMPEG_CONTEXT_P->InjBuffer_p->ES_Top_p         = (void *)((U32)BufferAddressStart_p + BufferSize -1);
    LCMPEG_CONTEXT_P->InjBuffer_p->ES_Read_p        = LCMPEG_CONTEXT_P->InjBuffer_p->ES_Base_p;
    LCMPEG_CONTEXT_P->InjBuffer_p->NextSCEntry_p    = NULL;
    if(LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
    {
        LCMPEG_CONTEXT_P->NextDataPointer_p         = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
    }
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);

    #if defined(DVD_SECURED_CHIP)
        /* In addition to passing ES_Base_p and ES_Top_p to VIDINJ_TransferLimits, give it ESCopy_Base_p and ESCopy_Top_p */
        ESCopy_Base_p = LCMPEG_CONTEXT_P->InjBuffer_p->ES_Base_p; /* DG TO DO: replace by LCMPEG_CONTEXT_P->InjBuffer_p->ESCopy_Base_p; */
        ESCopy_Top_p  = LCMPEG_CONTEXT_P->InjBuffer_p->ES_Top_p;  /* DG TO DO: replace by LCMPEG_CONTEXT_P->InjBuffer_p->ESCopy_Top_p;*/
    #else
        ESCopy_Base_p = NULL;
        ESCopy_Top_p  = NULL;
    #endif /* DVD_SECURED_CHIP && 7109 */

    /* Inform the injecter about the new bitbuffer */
    VIDINJ_TransferLimits(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl,
         /* BB */         LCMPEG_CONTEXT_P->InjBuffer_p->ES_Base_p,
                          LCMPEG_CONTEXT_P->InjBuffer_p->ES_Top_p,
         /* SC */         LCMPEG_CONTEXT_P->InjBuffer_p->SCList_Base_p,
                          LCMPEG_CONTEXT_P->InjBuffer_p->SCList_Top_p,
         /* PES */        LCMPEG_CONTEXT_P->InputBuffBase_p,
                          LCMPEG_CONTEXT_P->InputBuffTop_p
#if defined(DVD_SECURED_CHIP)
         /* ES Copy */    ,ESCopy_Base_p, ESCopy_Top_p
#endif /* Secured chip */
                          );

    STTBX_Print(("ES buffer Base:0x%.8x - Top:0x%.8x\nSC buffer Base:0x%.8x - Top:0x%.8x\nPES buffer Base:0x%.8x - Top:0x%.8x\n",
         /* BB */       (U32)(LCMPEG_CONTEXT_P->InjBuffer_p->ES_Base_p),
                        (U32)(LCMPEG_CONTEXT_P->InjBuffer_p->ES_Top_p),
         /* SC */       (U32)LCMPEG_CONTEXT_P->InjBuffer_p->SCList_Base_p,
                        (U32)LCMPEG_CONTEXT_P->InjBuffer_p->SCList_Top_p,
         /* PES */      (U32)(LCMPEG_CONTEXT_P->InputBuffBase_p),
                        (U32)(LCMPEG_CONTEXT_P->InputBuffTop_p)));

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
#ifdef TRACE_WITH_TIME
    InitialTime = time_now();
#endif /* TRACE_WITH_TIME */
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
    LCMPEG_CONTEXT_P->BitBufferThreshold = OccupancyThreshold;
} /* End of SetDecodeBitBufferThreshold() function */

/*******************************************************************************
Name        : SetSmoothBackwardConfiguration
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetSmoothBackwardConfiguration(
            const HALDEC_Handle_t HALDecodeHandle, const BOOL SetNotReset)
{
    /* this hal is always in manual mode: smooth backard has no effect */
    UNUSED_PARAMETER(HALDecodeHandle);
    UNUSED_PARAMETER(SetNotReset);
    /*TraceBuffer(("SetSmoothBackwardConfiguration \r\n"));*/
    return(ST_NO_ERROR);
} /* End of SetSmoothBackwardConfiguration() function */

/*******************************************************************************
Name        : SetStreamID
Description : Sets the stream ID(s) to be filtered
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetStreamID(const HALDEC_Handle_t HALDecodeHandle,
                        const U8              StreamID)
{
    UNUSED_PARAMETER(HALDecodeHandle);
    UNUSED_PARAMETER(StreamID);
} /* End of SetStreamID() function */

/*******************************************************************************
Name        : SetStreamType
Description : Set stream type for the PES parser
Parameters  : HAL decode handle, stream type
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetStreamType(const HALDEC_Handle_t    HALDecodeHandle,
                          const STVID_StreamType_t StreamType)
{
    /* This function has no effect as fdma engine manages either ES or PES  */
    /* data without any specific setting.                                   */
    UNUSED_PARAMETER(HALDecodeHandle);
    UNUSED_PARAMETER(StreamType);
} /* End of SetStreamType() function */

/*******************************************************************************
Name        : SetDataInputInterface
Description : cascaded to injection interface
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
    ST_ErrorCode_t Err;

    /* Ensure no FDMA transfer is done while setting a new data input interface */
    VIDINJ_EnterCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    Err = VIDINJ_Transfer_SetDataInputInterface(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl,
                                    GetWriteAddress,InformReadAddress,Handle);
    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);

    return(Err);
}

/*******************************************************************************
Name        : GetDataInputBufferParams
Description : Export the PES-Input Buffer add/size
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetDataInputBufferParams(
                    const HALDEC_Handle_t HALDecodeHandle,
                    void **  BaseAddress, U32 * Size)
{
    *BaseAddress    = STAVMEM_DeviceToVirtual(LCMPEG_CONTEXT_P->InputBuffBase_p,&LCMPEG_CONTEXT_P->VirtualMapping);
    *Size           = (U32)LCMPEG_CONTEXT_P->InputBuffTop_p
                    - (U32)LCMPEG_CONTEXT_P->InputBuffBase_p
                    + 1;
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : FlushDecoder
Description : Flush the decoder data buffer
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void FlushDecoder(const HALDEC_Handle_t HALDecodeHandle,
                    BOOL * const DiscontinuityStartCodeInserted_p)
{
    U8                      Pattern[]={0xAF, 0xAF, 0x00, 0x00, 0x01, 0xB6};
    U8                      Size = 6;
    /* Ensure no FDMA transfer is done while flushing the injecter */
    VIDINJ_EnterCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    VIDINJ_TransferFlush(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl, TRUE, Pattern, Size);
    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    (*DiscontinuityStartCodeInserted_p) = TRUE;
} /* End of FlushDecoder() function */

/*******************************************************************************
Name        : SearchNextStartCode
Description :
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
    STFDMA_SCEntry_t   *    CPUScanedEntry_p;
    STFDMA_SCEntry_t   *    CPUMatchingEntry_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    UNUSED_PARAMETER(FirstSliceDetect);

    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    switch(SearchMode)
    {
        case HALDEC_START_CODE_SEARCH_MODE_NORMAL :
            /* Get the next sc from internal variables (simu auto mode) */
            /* Step our pointer in sc-list */
            /*TraceSC(("Search SC...NORMAL\r\n"));*/
            PointOnNextSC(LCMPEG_CONTEXT_P);
        break;

        case HALDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS :
            TraceBuffer(("Search SC...from %x\r\n", SearchAddressFrom_p));

            if(((U32)SearchAddressFrom_p >= ((U32)LCMPEG_CONTEXT_P->ES_LinearBitBufferB.ES_Base_p - CD_FIFO_ALIGNEMENT)) &&
               ((U32)SearchAddressFrom_p <= ((U32)LCMPEG_CONTEXT_P->ES_LinearBitBufferB.ES_Base_p)))
            {
                LCMPEG_CONTEXT_P->DecBuffer_p = &LCMPEG_CONTEXT_P->ES_LinearBitBufferB;
                TraceBuffer(("Search in buffer B\r\n"));
            }
            else
            {
                LCMPEG_CONTEXT_P->DecBuffer_p = &LCMPEG_CONTEXT_P->ES_LinearBitBufferA;
                TraceBuffer(("Search in buffer A\r\n"));
            }
            /* Get the next sc from specified add : scan the sc list */
            /* Trickmode issue : A sequence startcode is put by trickmode,
             * directly in the linearbuffer. This startcode will never be
             * detected by the FDMA as it will not be transfered. So set the flag
             * SCStartBuffer to notify this startcode. */
            LCMPEG_CONTEXT_P->NextDataPointer_p     = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
            LCMPEG_CONTEXT_P->DecodeAdd             = NULL;
            LCMPEG_CONTEXT_P->PictureAdd            = NULL;

            STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
#ifdef ST_5100
            /* This chip dependency should be removed once addresses management is generalized */
            CPUMatchingEntry_p = LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Base_p;
#else
            CPUMatchingEntry_p = STAVMEM_DeviceToCPU(LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Base_p, &VirtualMapping);
#endif
            CPUScanedEntry_p   = CPUMatchingEntry_p;

            while(((GetSCAdd(CPUScanedEntry_p)) < (U32)SearchAddressFrom_p)
               && (CPUScanedEntry_p < LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Top_p))
            {
                if(!IsPTS(CPUScanedEntry_p))
                {
                    CPUMatchingEntry_p   = CPUScanedEntry_p;
                }
                CPUScanedEntry_p     ++;
            }

#ifdef ST_5100
            /* This chip dependency should be removed once addresses management is generalized */
            LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p = CPUMatchingEntry_p;
#else
            LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p = STAVMEM_CPUToDevice(CPUMatchingEntry_p, &VirtualMapping);
#endif
            LCMPEG_CONTEXT_P->DecBuffer_p->LastSCAdd_p   = (void*)GetSCAdd(CPUMatchingEntry_p);
        break;
    }
    /* Try to find immediatly a startcode and notify it */
    if(IsThereANewStartCode(HALDecodeHandle))
    {
        HALDEC_Properties_t  * Hal_Context_p;
        LcmpegContext_t *      LcmpegContext_p;

        /* The event will be catched by the instance identified */
        Hal_Context_p = (HALDEC_Properties_t *) HALDecodeHandle;
        LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
        LCMPEG_CONTEXT_P->SWStatus |= HALDEC_START_CODE_HIT_MASK;
        if((LCMPEG_CONTEXT_P->SWStatus & LCMPEG_CONTEXT_P->SWInterruptMask) != 0)
        {
            STEVT_Notify(Hal_Context_p->EventsHandle,
                LcmpegContext_p->InterruptEvtID,
                STEVT_EVENT_DATA_TYPE_CAST NULL);
        }
        LCMPEG_CONTEXT_P->SCDetectSimuLaunched = FALSE;
    }
    else /* Startcode can't be found immediatly, so remember we need a startcode, to notify it on the next call to
          * GetAbnormallyMissingInterruptStatus */
    {
        LCMPEG_CONTEXT_P->SCDetectSimuLaunched = TRUE;
    }
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
} /* End of SearchNextStartCode() function */

/*******************************************************************************
Name        : Get16BitsHeaderData
Description : Get 16bit word of header data
Parameters  : HAL Decode manager handle
Assumptions : Header FIFO is not empty, must be checked with
                IsHeaderDataFIFOEmpty()
Limitations :
Returns     :
*******************************************************************************/
static U16 Get16BitsHeaderData(const HALDEC_Handle_t  HALDecodeHandle)
{
    U16 Tmp16;

    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    Tmp16 =              GetAByteFromES(HALDecodeHandle);
    Tmp16 = (Tmp16<<8) | GetAByteFromES(HALDecodeHandle);

    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
    return(Tmp16);
} /* End of Get16BitsHeaderData() function */

/*******************************************************************************
Name        : Get32BitsHeaderData
Description : Get 32bit word of header data
Parameters  : HAL Decode manager handle
Assumptions : Header FIFO is not empty, must be checked with
                IsHeaderDataFIFOEmpty()
Limitations :
Returns     :
*******************************************************************************/
static U32 Get32BitsHeaderData(const HALDEC_Handle_t HALDecodeHandle)
{
    U32 Tmp32;

    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    Tmp32 =                GetAByteFromES(HALDecodeHandle);
    Tmp32 = (Tmp32 << 8) | GetAByteFromES(HALDecodeHandle);
    Tmp32 = (Tmp32 << 8) | GetAByteFromES(HALDecodeHandle);
    Tmp32 = (Tmp32 << 8) | GetAByteFromES(HALDecodeHandle);

    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
    return(Tmp32);
} /* End of Get32BitsHeaderData() function */

/*******************************************************************************
Name        : Get16BitsStartCode
Description : Get 16 bit word of start code, says if start code is on MSB/LSB
Parameters  : HAL Decode manager handle, ..
Assumptions :
Limitations :
Returns     : U16 Value from the bit buffer
*******************************************************************************/
static U16 Get16BitsStartCode(const HALDEC_Handle_t  HALDecodeHandle,
                              BOOL * const StartCodeOnMSB_p)
{
    U16 SC_Val;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    *StartCodeOnMSB_p = FALSE;

    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);

#ifdef TRACE_UART
    if(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p == LCMPEG_CONTEXT_P->DecBuffer_p->LastSCEntry_p)
    {
        TraceBuffer(("Getting again the same startcode !\r\n"));
    }
    LCMPEG_CONTEXT_P->DecBuffer_p->LastSCEntry_p = LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p;
#endif /* TRACE_UART */

    /* This problematic case has been seen while doing reverse
       trickmodes. Better to have this check to avoid a crash */
    if(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p == NULL)
    {
        TraceBuffer(("Error : Next StartCode Entry is NULL !\r\n"));
        SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
        return(GREATEST_SYSTEM_START_CODE);
    }

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#ifdef ST_5100
    /* This chip dependency should be removed once addresses management is generalized */

    /* Get the sc value */
    SC_Val = GetSCVal(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p);
#else
    /* Get the sc value */
    SC_Val = GetSCVal( (STFDMA_SCEntry_t *)STAVMEM_DeviceToCPU(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p, &VirtualMapping) );
#endif

    /* Step our Data pointer */
    LCMPEG_CONTEXT_P->NextDataPointer_p ++;
    /* and loop if needed */
    if(  (LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
       &&(LCMPEG_CONTEXT_P->NextDataPointer_p > LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p))
    {
        LCMPEG_CONTEXT_P->NextDataPointer_p = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
    }
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
    return(SC_Val);
} /* End of Get16BitsStartCode() function */

/*******************************************************************************
Name        : ProcessSC
Description : Do the right treatment
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : TRUE : There is a time stamp available. FALSE : No time stamp
*******************************************************************************/
static void ProcessSC(const HALDEC_Handle_t  HALDecodeHandle)
{
    U16                                     SC_Val;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#ifdef ST_5100
    /* This chip dependency should be removed once addresses management is generalized */

    /* Get the sc value */
    SC_Val = GetSCVal(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p);
#else
    /* Get the sc value */
    SC_Val = GetSCVal( (STFDMA_SCEntry_t *)STAVMEM_DeviceToCPU(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p, &VirtualMapping) );
#endif

    /* Special Start Code : Slice +1 */
    /*-------------------------------*/
    if(LCMPEG_CONTEXT_P->EndOfSlicesSC) /* now it is "slice+1" */
    {
        LCMPEG_CONTEXT_P->EndOfSlicesSC = FALSE;
        GetStartCodeAddress(HALDecodeHandle, &(LCMPEG_CONTEXT_P->EndOfPictureAdd));
    }
    /* Special Start Code : Slice */
    /*----------------------------*/
    if(SC_Val == FIRST_SLICE_START_CODE)
    {
        GetStartCodeAddress(HALDecodeHandle, &(LCMPEG_CONTEXT_P->PictureAdd));
        LCMPEG_CONTEXT_P->EndOfSlicesSC = TRUE;
    } else if(SC_Val == PICTURE_START_CODE)
    {
    /* Special Start Code : Picture */
    /*------------------------------*/
        /* Try to find an associated PTS to this picture in the saved list of PTS */
        SearchForAssociatedPTS(HALDecodeHandle);
    }else if(SC_Val == USER_DATA_START_CODE)
    {
    /* Special Start Code : UserData */
    /*-------------------------------*/
        /* Next startcode will be detected automatically by common layer so skip it in HAL */
        LCMPEG_CONTEXT_P->SkipNextStartCode = TRUE;
    }
} /* End of ProcessSC() function */

/*******************************************************************************
Name        : GetPTS
Description : Gets current picture time stamp information
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : TRUE : There is a time stamp available. FALSE : No time stamp
*******************************************************************************/
static BOOL GetPTS(const HALDEC_Handle_t HALDecodeHandle,
                        U32 *  const PTS_p,
                        BOOL * const PTS33_p)
{
    /* Check if we have associated a PTS to current picture
     * (PTS are associated to pictures at parsing time)*/
    if (LCMPEG_CONTEXT_P->CurrentPicturePTSEntry.Address_p == NULL)
    {
        return(FALSE);
    }

    /* the previous test have not returned the fct :OK we can do */
    *PTS33_p = LCMPEG_CONTEXT_P->CurrentPicturePTSEntry.PTS33;
    *PTS_p   = LCMPEG_CONTEXT_P->CurrentPicturePTSEntry.PTS32;
    return(TRUE);
} /* Enf of GetPTS() function */

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
    UNUSED_PARAMETER(HALDecodeHandle);

    return(0);
} /* End of GetAbnormallyMissingInterruptStatus() function */

/*******************************************************************************
Name        : SearchForAssociatedPTS
Description : Searches for a PTS associated to the current picture
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void SearchForAssociatedPTS(const HALDEC_Handle_t HALDecodeHandle)
{
    void                                  * ES_Write_p = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Write_p;
    STFDMA_SCEntry_t                      * CPUNextSCEntry_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#ifdef ST_5100
    /* This chip dependency should be removed once addresses management is generalized */
    CPUNextSCEntry_p = LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p;
#else
    CPUNextSCEntry_p = STAVMEM_DeviceToCPU(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p, &VirtualMapping);
#endif

    /* Initialise Adrress to NULL to say no PTS associated to current picture */
    LCMPEG_CONTEXT_P->CurrentPicturePTSEntry.Address_p = NULL;
    /* Check if PTS address is before PICT address in ES */

    while(
           (LCMPEG_CONTEXT_P->PTS_SCList.Read_p != LCMPEG_CONTEXT_P->PTS_SCList.Write_p) && /* PTS List is empty */
           (
                (
                    /* 1rst case : PictAdd > PTSAdd.                              */
                    (((U32)GetSCAdd(CPUNextSCEntry_p)) >= ((U32)LCMPEG_CONTEXT_P->PTS_SCList.Read_p->Address_p + 3)) &&
                    /* The 2 pointers are on the same side of ES_Write and at least 3 bytes separate them.  */
                    (
                        /* PictAdd > PTSAdd => The 2 pointers must be on the same side of ES_Write */
                        (
                            (((U32)ES_Write_p) > ((U32)GetSCAdd(CPUNextSCEntry_p))) &&
                            (((U32)ES_Write_p) > ((U32)LCMPEG_CONTEXT_P->PTS_SCList.Read_p->Address_p))
                        ) ||
                        (
                            (((U32)ES_Write_p) < ((U32)GetSCAdd(CPUNextSCEntry_p))) &&
                            (((U32)ES_Write_p) < ((U32)LCMPEG_CONTEXT_P->PTS_SCList.Read_p->Address_p))
                        )
                    )
                ) ||
                (
                    /* 2nd case : PictAdd < PTSAdd.                               */
                    (((U32)GetSCAdd(CPUNextSCEntry_p)) < ((U32)LCMPEG_CONTEXT_P->PTS_SCList.Read_p->Address_p)) &&
                    /* The 2 pointers are not on the same side of ES_Write and at least 3 bytes separate them.  */
                    (((U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p
                    - (U32)LCMPEG_CONTEXT_P->PTS_SCList.Read_p->Address_p + 1
                    + (U32)GetSCAdd(CPUNextSCEntry_p)
                    - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p + 1 ) > 3 ) &&
                    /* PictAdd < PTSAdd => The 2 pointers must not be on the same side of ES_Write */
                    (((U32)ES_Write_p) > ((U32)GetSCAdd(CPUNextSCEntry_p))) &&
                    (((U32)ES_Write_p) < ((U32)LCMPEG_CONTEXT_P->PTS_SCList.Read_p->Address_p))
                )
            )
        )
    {
        /* Associate PTS to Picture */
        memcpy(&LCMPEG_CONTEXT_P->CurrentPicturePTSEntry, LCMPEG_CONTEXT_P->PTS_SCList.Read_p, sizeof(PTS_t));
        LCMPEG_CONTEXT_P->PTS_SCList.Read_p++;

        if (((U32)LCMPEG_CONTEXT_P->PTS_SCList.Read_p) >= ((U32)LCMPEG_CONTEXT_P->PTS_SCList.SCList + sizeof(PTS_t) * PTS_SCLIST_SIZE))
        {
            LCMPEG_CONTEXT_P->PTS_SCList.Read_p = (PTS_t*)LCMPEG_CONTEXT_P->PTS_SCList.SCList;
        }
    }
} /* End of SearchForAssociatedPTS() function */

/*******************************************************************************
Name        : SavePTSEntry
Description : Stores a new parsed PTSEntry in the PTS StartCode List
Parameters  : HAL Decode manager handle, PTS Entry Pointer in FDMA's StartCode list
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void SavePTSEntry(const HALDEC_Handle_t HALDecodeHandle, STFDMA_SCEntry_t* CPUPTSEntry_p)
{
    PTS_t* PTSLIST_WRITE_PLUS_ONE;

    if ((U32)GetSCAdd(CPUPTSEntry_p) != (U32)LCMPEG_CONTEXT_P->LastPTSStored.Address_p)
    {
        PTSLIST_WRITE_PLUS_ONE = PTSLIST_WRITE + 1;
        if(PTSLIST_WRITE_PLUS_ONE > PTSLIST_TOP)
        {
            PTSLIST_WRITE_PLUS_ONE = PTSLIST_BASE;
        }
        /* Check if we can find a place to store the new PTS Entry, otherwise we
         * overwrite the oldest one */
        if (PTSLIST_WRITE_PLUS_ONE == PTSLIST_READ)
        {
            TracePTS(("\r\n==OverwritePTS =="));
            /* First free the oldest one */
            /* Keep always an empty SCEntry because Read_p == Write_p means that the SCList is empty */
            PTSLIST_READ++;
            if (PTSLIST_READ >= PTSLIST_TOP)
            {
                PTSLIST_READ = PTSLIST_BASE;
            }
            /* Next overwrite it */
        }
        /* Store the PTS */
        PTSLIST_WRITE->PTS33     = GetPTS33(CPUPTSEntry_p);
        PTSLIST_WRITE->PTS32     = GetPTS32(CPUPTSEntry_p);
        PTSLIST_WRITE->Address_p = (void*)GetSCAdd(CPUPTSEntry_p);
        PTSLIST_WRITE++;
        TracePTS(("\r\n==Store     PTS== PTSAdd 0x%x at 0x%x", (U32)GetSCAdd(CPUPTSEntry_p), (U32)PTSLIST_WRITE));
        if (PTSLIST_WRITE >= PTSLIST_TOP)
        {
            PTSLIST_WRITE = PTSLIST_BASE;
        }
        LCMPEG_CONTEXT_P->LastPTSStored.PTS33      = GetPTS33(CPUPTSEntry_p);
        LCMPEG_CONTEXT_P->LastPTSStored.PTS32      = GetPTS32(CPUPTSEntry_p);
        LCMPEG_CONTEXT_P->LastPTSStored.Address_p  = (void*)GetSCAdd(CPUPTSEntry_p);
    }
#if defined TRACE_PTS
    else
    {
        TracePTS(("\r\n==Stored alread== PTSAdd 0x%x at 0x%x", (U32)GetSCAdd(CPUPTSEntry_p), (U32)LCMPEG_CONTEXT_P->LastPTSStored.Address_p));
    }
#endif /* TRACE_PTS */
} /* End of SavePTSEntry() function */

/*******************************************************************************
Name        : GetPictureDescriptors
Description : Get  Picture Descriptors : PictureMeanQP and PictureVarianceQP,
Parameters  : HAL decode handle, PictureMeanQP, PictureVarianceQP
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static ST_ErrorCode_t GetPictureDescriptors(const HALDEC_Handle_t HALDecodeHandle, STVID_PictureDescriptors_t * PictureDescriptors_p)
{
    LcmpegContext_t *       LcmpegContext_p;
    HALDEC_Properties_t *   Hal_Context_p;
    Hal_Context_p = (HALDEC_Properties_t *) HALDecodeHandle;
    LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
#ifdef ST_deblock
    if (LcmpegContext_p->DecodeParams.CurrentDeblockingState)
    {
        PictureDescriptors_p->PictureMeanQP = LcmpegContext_p->PictureMeanQP;
        PictureDescriptors_p->PictureVarianceQP = LcmpegContext_p->PictureVarianceQP;
    }
    else
    {
        PictureDescriptors_p->PictureMeanQP = 0;
        PictureDescriptors_p->PictureVarianceQP = 0;
    }
#else
    {
        PictureDescriptors_p->PictureMeanQP = 0;
        PictureDescriptors_p->PictureVarianceQP = 0;
    }
#endif /* no ST_deblock */
        return(ST_NO_ERROR);

} /* End of GetPictureDescriptors() function */

/*******************************************************************************
Name        : GetDecodeSkipConstraint
Description : We ask the HAL which is its possible action.
Parameters  : IN :HAL decode handle,
              OUT : possible Action.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void GetDecodeSkipConstraint(const HALDEC_Handle_t HALDecodeHandle,
        HALDEC_DecodeSkipConstraint_t * const HALDecodeSkipConstraint_p)
{
    UNUSED_PARAMETER(HALDecodeHandle);

    *HALDecodeSkipConstraint_p = HALDEC_DECODE_SKIP_CONSTRAINT_NONE;
} /* End of GetDecodeSkipConstraint() function */

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
    return(LCMPEG_CONTEXT_P->OutputCounter);
} /* End of GetBitBufferOutputCounter() function */

/*******************************************************************************
Name        : GetCDFifoAlignmentInBytes
Description :
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetCDFifoAlignmentInBytes(
        const HALDEC_Handle_t HALDecodeHandle, U32 * const CDFifoAlignment_p)
{
    UNUSED_PARAMETER(HALDecodeHandle);

    /**CDFifoAlignment_p = 1;*/
    /* In trickmode, the last startcode that should be found after parsing,
     * is a startcode verifying Buffer_end <SC_Add< Buffer_end_aligned + alignement
     * If alignement is 1, we can never reach the second condition, so patch it
     * to 128. (Used in trickmode, in backward mode) */
     *CDFifoAlignment_p = CD_FIFO_ALIGNEMENT;
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
    U32                 Level;
    void*               ES_Write_p;
    HALDEC_Properties_t *   Hal_Context_p;
    LcmpegContext_t *       LcmpegContext_p;
    BOOL                ComputeAgainTheBitBufferLevel = FALSE;

    /* Case : Circular bit-buffer */
    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
	ES_Write_p = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Write_p;
    if(LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
    {
        if(LCMPEG_CONTEXT_P->SWInterruptMask  == 0)
        {
            /* Driver is stopped */
            LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p = ES_Write_p;
        }
        else
        {
            /* Here we begin with a simplified version of the bitbuffer  level computation, not taking into
                account decoder read pointer. This is enough for bit buffer levels not nearly full.
                This is to avoid calling many times STOS_InterruptLock needed for a precise computation,
                as this function is called many times per VSYNC. */
            if(LCMPEG_CONTEXT_P->PictureAdd != NULL)
            {
                /* A new picture was found after launching the last decode */
                U32 FirstByteOfTheStartCodeAdd;

                /* Take into account the startcode size, to avoid rewrite of the startcode with a new injection */
                FirstByteOfTheStartCodeAdd = (U32)LCMPEG_CONTEXT_P->PictureAdd - 3;
                if(FirstByteOfTheStartCodeAdd < (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p)
                {
                    FirstByteOfTheStartCodeAdd     +=   (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p
                                                      - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p
                                                      + 1;
                }
                LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p = (void*)FirstByteOfTheStartCodeAdd;
            }
            else
            {
                /* No picture found since the last decode */
                LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p = (void*)LCMPEG_CONTEXT_P->NextDataPointer_p;
            }
#ifdef TRACE_UART
            if((LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p > LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p) ||
            (LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p < LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p) ||
            (LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p == NULL))
            {
                TraceBuffer(("ES_Read_p Error !\r\n"));
                TraceBuffer(("ES_Top =%08x\r\n", LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p));
                TraceBuffer(("ES_Read=%08x\r\n", LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p));
                TraceBuffer(("ES_Base=%08x\r\n", LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p));
            }
#endif /* TRACE_UART */
        }
        /* Compute the level */
        if(LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p  <= (U8 *)(ES_Write_p))
        {
            Level = (U32)ES_Write_p - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p;
        }
        else
        {
            Level = (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p
                  - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p + 1
                  + (U32)ES_Write_p
                  - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
        }
        /* Check for overflow */
        if(Level >= LCMPEG_CONTEXT_P->BitBufferThreshold)
        {
            /* Try to have a more precise value for the level, if the hardware decoder is
                hold by the current instance */
            STOS_InterruptLock();
            if(MultiDecodeController.HALDecodeHandleForNextIT == HALDecodeHandle)
            {
                LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p = (void*)HAL_ReadReg32(VID_VLDPTR);
                ComputeAgainTheBitBufferLevel = TRUE;
            }
            STOS_InterruptUnlock();
            /* Compute again the level */
            if(ComputeAgainTheBitBufferLevel)
            {
                if(LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p  <= (U8 *)(ES_Write_p))
                {
                    Level = (U32)ES_Write_p
                          - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p;
                }
                else
                {
                    Level = (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p
                          - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Read_p + 1
                          + (U32)ES_Write_p
                          - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
                }
            }
            /* Check again for the overflow */
            if(Level >= LCMPEG_CONTEXT_P->BitBufferThreshold)
            {
                /* Simulate an overflow interrupt */
                Hal_Context_p = (HALDEC_Properties_t *) HALDecodeHandle;
                LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
                LCMPEG_CONTEXT_P->SWStatus |= HALDEC_BIT_BUFFER_NEARLY_FULL_MASK;
                if((LCMPEG_CONTEXT_P->SWStatus & LCMPEG_CONTEXT_P->SWInterruptMask) != 0)
                {
                    STEVT_Notify(Hal_Context_p->EventsHandle,
                        LcmpegContext_p->InterruptEvtID,
                        STEVT_EVENT_DATA_TYPE_CAST NULL);
                }
            }
        }
    }
    else /* Case : linear bit_buffer */
    {
        Level = (U32)ES_Write_p
              - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
    }
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);

    return(Level);
} /* End of GetDecodeBitBufferLevel() function */

#ifdef STVID_HARDWARE_ERROR_EVENT
/*******************************************************************************
Name        : GetHardwareErrorStatus
Description :
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Harware error status.
*******************************************************************************/
static STVID_HardwareError_t GetHardwareErrorStatus(
        const HALDEC_Handle_t HALDecodeHandle)
{
    return(STVID_HARDWARE_NO_ERROR_DETECTED);
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
    U32 HWStatus, SWStatus, TranslatedStatus;
#ifdef TRACE_UART
    BOOL DecodingErrorOccured = FALSE;
    char ErrorMessage[256]="";
#endif /* TRACE_UART */

    /* !!! in multi-instance context : the status is get by the instance */
    /* that calls this fct and cleared in the hardware : so lost for all */
    /* other instances                                                   */
    TranslatedStatus = 0;

    if(INSIDE_TASK())
    {
        /* Lock interrupts to avoid that the interrupt status is read twice, from
           task context and from interrupt context, if an interrupt occurs while
           reading the interrupt status */
        STOS_InterruptLock();
    }

    HWStatus = LCMPEG_CONTEXT_P->HWStatus;
    SWStatus = LCMPEG_CONTEXT_P->SWStatus;
    LCMPEG_CONTEXT_P->HWStatus = 0;
    LCMPEG_CONTEXT_P->SWStatus = 0;

    if(INSIDE_TASK())
    {
        /* Lock interrupts to avoid that the interrupt status is read twice, from
           task context and from interrupt context, if an interrupt occurs while
           reading the interrupt status */
        STOS_InterruptUnlock();
    }

    /* Get the HW interrupts */
    if(HWStatus != 0) /* Function is called in hardware interruption context */
    {
        if ((HWStatus & (1 << VID_STA_DID)) != 0)
        {
            TracePerf((":%3d:END\r\n", time_now() / (ST_GetClocksPerSecond() / 1000)));
            TraceInterrupt(("%x DID ",HALDecodeHandle));
            TranslatedStatus |= HALDEC_DECODER_IDLE_MASK;
            TranslatedStatus |= HALDEC_PIPELINE_IDLE_MASK;
        }
        if ((HWStatus & (1 << VID_STA_DOE)) != 0)
        {
            TraceInterrupt(("%x DOE ", HALDecodeHandle));
            TranslatedStatus |= HALDEC_DECODING_OVERFLOW_MASK;
#ifdef TRACE_UART
            strcat(ErrorMessage, "Decoding overflow error !" );
            DecodingErrorOccured = TRUE;
#endif /* TRACE_UART */
        }
        if ((HWStatus & (1 << VID_STA_DUE)) != 0)
        {
            TraceInterrupt(("%x DUE ", HALDecodeHandle));
            TranslatedStatus |= HALDEC_DECODING_UNDERFLOW_MASK;
#ifdef TRACE_UART
            strcat(ErrorMessage, "Decoding underflow error !" );
            DecodingErrorOccured = TRUE;
#endif /* TRACE_UART */
        }
        if ((HWStatus & (1 << VID_STA_MSE)) != 0)
        {
            TraceInterrupt(("%x MSE ", HALDecodeHandle));
            TranslatedStatus |= HALDEC_DECODING_SYNTAX_ERROR_MASK;
#ifdef TRACE_UART
            strcat(ErrorMessage, "Decoding syntax error !" );
            DecodingErrorOccured = TRUE;
#endif /* TRACE_UART */
        }
        if ((HWStatus & (1 << VID_STA_DSE)) != 0)
        {
            TraceInterrupt(("%x DSE ", HALDecodeHandle));
            TranslatedStatus |= HALDEC_DECODING_SYNTAX_ERROR_MASK;
#ifdef TRACE_UART
            strcat(ErrorMessage, "Decoding syntax error !" );
            DecodingErrorOccured = TRUE;
#endif /* TRACE_UART */
        }
        if ((HWStatus & (1 << VID_STA_VLDRL)) != 0)
        {
            TraceInterrupt(("%x VLDRL ", HALDecodeHandle));
            TranslatedStatus |= 0;
#ifdef TRACE_UART
            strcat(ErrorMessage, "VLDRL reached error !" );
            DecodingErrorOccured = TRUE;
#endif /* TRACE_UART */
        }
        if ((HWStatus & (1 << VID_STA_R_OPC)) != 0)
        {
            TraceInterrupt(("%x ROPC ", HALDecodeHandle));
            TranslatedStatus |= 0;
#ifdef TRACE_UART
            strcat(ErrorMessage, "ROPC error !" );
            DecodingErrorOccured = TRUE;
#endif /* TRACE_UART */
        }
#ifdef TRACE_UART
        if(DecodingErrorOccured)
        {
            U32 ErrorAddress;
            ErrorAddress = (U32)HAL_ReadReg32(VID_VLDPTR);
            TraceBuffer(("%s@%08x\r\n", ErrorMessage, ErrorAddress));
        }
#endif /* TRACE_UART */
    }

    /* Get the SW interrupts */
    TranslatedStatus |= SWStatus;

#if defined(TRACE_INTERRUPT)
        if ((TranslatedStatus & HALDEC_DECODER_IDLE_MASK|HALDEC_PIPELINE_IDLE_MASK) != 0)
        {
            TraceInterrupt(("%x DID ",HALDecodeHandle));
        }
#endif

#if defined(TRACE_INTERRUPT) || defined(TRACE_SC)
        if((TranslatedStatus & HALDEC_START_CODE_HIT_MASK) != 0)
        {
            TraceBuffer(("%x SCH ", HALDecodeHandle));
        }
#endif
#ifdef TRACE_PERFORMANCES
        if((TranslatedStatus & HALDEC_START_CODE_HIT_MASK) != 0)
        {
            TracePerf((":%3d:SCH\r\n", time_now() / (ST_GetClocksPerSecond() / 1000)));
        }
#endif
    TraceInterrupt(("*\r\n"));

    return(TranslatedStatus);
} /* End of GetInterruptStatus() function */

/*******************************************************************************
Name        : GetLinearBitBufferTransferedDataSize
Description :
Parameters  : HAL decode handle
Assumptions : Level in bytes, with granularity of 256 bytes
Limitations : Returns     : 32bit size
*******************************************************************************/
static ST_ErrorCode_t GetLinearBitBufferTransferedDataSize(
                    const HALDEC_Handle_t HALDecodeHandle,
                    U32 * const LinearBitBufferTransferedDataSize_p)
{
    U32 Level;
    U8                      Pattern[]={0xAF, 0xAF, 0x00, 0x00, 0x01, 0xB6};
    U8                      Size = 6;

    /* Ensure no FDMA transfer is done while flushing the injecter */
    VIDINJ_EnterCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);

    /* Flush the input buffer content before getting the linear buffer level */
    VIDINJ_TransferFlush(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl, FALSE, Pattern, Size);
    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    Level = (U32)LCMPEG_CONTEXT_P->InjBuffer_p->ES_Write_p - (U32)LCMPEG_CONTEXT_P->InjBuffer_p->ES_Base_p;
    TracePerf((":%3d:EINJ\r\n", time_now() / (ST_GetClocksPerSecond() / 1000)));
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);

    /* Return the buffer level value */
    *LinearBitBufferTransferedDataSize_p = Level;
    TraceBuffer(("TransferedDataSize=%d\r\n", Level));
    return (ST_NO_ERROR);
} /* End of GetLinearBitBufferTransferedDataSize() function */

/*******************************************************************************
Name        : GetSCDAlignmentInBytes
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetSCDAlignmentInBytes(
                    const HALDEC_Handle_t HALDecodeHandle,
                    U32 * const SCDAlignment_p)
{
    UNUSED_PARAMETER(HALDecodeHandle);

    *SCDAlignment_p = 1;

    return (ST_NO_ERROR);
} /* End of GetSCDAlignmentInBytes() function */

/*******************************************************************************
Name        : GetStartCodeAddress
Description : Get a pointer on the address of the Start Code if available
Parameters  : Decoder registers address, pointer on buffer address
Assumptions :
Limitations :
Returns     : TRUE if StartCode Address available, FALSE if no
*******************************************************************************/
static ST_ErrorCode_t GetStartCodeAddress(
         const HALDEC_Handle_t HALDecodeHandle,  void ** const BufferAddress_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    if (BufferAddress_p == NULL)
    {
        /* Error cases */
        return(ST_ERROR_BAD_PARAMETER);
    }

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#ifdef ST_5100
    /* This chip dependency should be removed once addresses management is generalized */

    *BufferAddress_p = (void*)(GetSCAdd(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p));
#else
    *BufferAddress_p = (void*)(GetSCAdd( (STFDMA_SCEntry_t *)STAVMEM_DeviceToCPU(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p, &VirtualMapping) ));
#endif

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
    /* non supported */
    UNUSED_PARAMETER(HALDecodeHandle);

    return(0);
} /* End of GetStatus() function */

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
    U32     DataSize;
    void*   ES_Write_p = LCMPEG_CONTEXT_P->DecBuffer_p->ES_Write_p;
    U8 *    NextDataPointer_p = LCMPEG_CONTEXT_P->NextDataPointer_p;

    /* Don't simulate the CD-FIFO when in linear buffers management */
    if(LCMPEG_CONTEXT_P->BitBufferType != HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
    {
        return(FALSE);
    }

    if(NextDataPointer_p <= (U8 *)(ES_Write_p))
    {
        DataSize = (U32)ES_Write_p - (U32)NextDataPointer_p;
    }
    else
    {
        DataSize = (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p
                 - (U32)NextDataPointer_p + 1
                 + (U32)ES_Write_p
                 - (U32)LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
    }

     /* If there is less data than 4 bytes, after the current startcode,
     * let's say that there is no data in the data header */
    if(DataSize < 4)
    {
        /*TraceBuffer(("HeaderDataFifoEmpty\r\n"));*/
        return (TRUE);
    }
    else
    {
        return(FALSE);
    }
} /* End of IsHeaderDataFIFOEmpty() function */

/*******************************************************************************
Name        : SetFoundStartCode
Description : Used for the Pipe Mis-alignment detection.
Parameters  : HAL decode handle, StartCode found
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetFoundStartCode(const HALDEC_Handle_t HALDecodeHandle,
                const U8 StartCode, BOOL * const FirstPictureStartCodeFound_p)
{
    UNUSED_PARAMETER(HALDecodeHandle);
    UNUSED_PARAMETER(StartCode);

    *FirstPictureStartCodeFound_p = FALSE;
} /* End of SetFoundStartCode() function */

/*******************************************************************************
Name        : SetInterruptMask
Description : Set Interrupt Mask
Parameters  : HAL Decode manager handle, Mask to be written :
                HALDEC_MISALIGNMENT_START_CODE_DETECTOR_AHEAD_MASK  n/a
                HALDEC_MISALIGNMENT_PIPELINE_AHEAD_MASK             n/a
                HALDEC_VLD_READY_MASK                               n/a : smooth backward only
                HALDEC_START_CODE_DETECTOR_BIT_BUFFER_EMPTY_MASK    n/a : never used
                HALDEC_BITSTREAM_FIFO_FULL_MASK                     n/a : never used
                HALDEC_INCONSISTENCY_ERROR_MASK                     n/a : never used
                HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK               n/a : no information to provide it
                HALDEC_BIT_BUFFER_NEARLY_FULL_MASK                  ok : software managed
                HALDEC_DECODING_OVERFLOW_MASK                       ok : VID_STA_DOE
                HALDEC_DECODING_SYNTAX_ERROR_MASK                   ok : VID_STA_DSE
                HALDEC_OVERTIME_ERROR_MASK                          n/a: never used
                HALDEC_DECODING_UNDERFLOW_MASK                      ok : VID_STA_DUE
                HALDEC_DECODER_IDLE_MASK                            ok : sent at end of decode / pipe reset.
                                                                        soft reset / after a skip
                HALDEC_PIPELINE_IDLE_MASK                           ok : sent at end of decode / pipe reset.
                                                                        soft reset / after a skip
                HALDEC_PIPELINE_START_DECODING_MASK                 ok : software managed after each decode start
                HALDEC_HEADER_FIFO_EMPTY_MASK                       n/a: never used
                HALDEC_HEADER_FIFO_NEARLY_FULL_MASK                 n/a: never used
                HALDEC_START_CODE_HIT_MASK                          ok : always activated.

Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetInterruptMask(const HALDEC_Handle_t  HALDecodeHandle,
                             const U32              TranslatedMask)
{
    U32 Mask;

    Mask = 0;

    TraceBuffer(("InterruptMask=%d\r\n", TranslatedMask));

    /* Ensure no FDMA transfer is done while stopping the injecter */
    VIDINJ_EnterCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);

    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    if (TranslatedMask == 0) /* A stop was requested */
    {
        /* Stop injecter */
        VIDINJ_TransferStop(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    }

    if(TranslatedMask & HALDEC_DECODER_IDLE_MASK)
    {
        Mask |= (1 << VID_STA_DID);
    }
    if(TranslatedMask & HALDEC_DECODING_OVERFLOW_MASK)
    {
        Mask |= (1 << VID_STA_DOE);
    }
    if(TranslatedMask & HALDEC_DECODING_UNDERFLOW_MASK)
    {
        Mask |= (1 << VID_STA_DUE);
    }
    if(TranslatedMask & HALDEC_DECODING_SYNTAX_ERROR_MASK)
    {
        Mask |= (1 << VID_STA_DSE);
    }

    LCMPEG_CONTEXT_P->HWInterruptMask = Mask;
    LCMPEG_CONTEXT_P->SWInterruptMask = TranslatedMask;

    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
} /* End of SetInterruptMask() function */

/*******************************************************************************
Name        : SetSkipConfiguration
Description : Set configuration for skip. HAL needs to know which kind of
              picture is skipped. Because of hardware constraints with field
              pictures : same action for 2nd field than the 1st field.
              This function is called in the decode task context
Parameters  : HAL decode handle, Picture Structure information
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSkipConfiguration(const HALDEC_Handle_t HALDecodeHandle,
                const STVID_PictureStructure_t * const PictureStructure_p)
{
    UNUSED_PARAMETER(HALDecodeHandle);
    UNUSED_PARAMETER(PictureStructure_p);
    /*(TODO)*/
} /* End of SetSkipConfiguration() function */

/*******************************************************************************
Name        : SkipPicture
Description : Skip one picture in incoming data, this function is called in
              the decode task context
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SkipPicture(const HALDEC_Handle_t           HALDecodeHandle,
                        const BOOL                      OnlyOnNextVsync,
                        const HALDEC_SkipPictureMode_t  SkipMode)
{
    HALDEC_Properties_t  * Hal_Context_p;
    LcmpegContext_t *      LcmpegContext_p;

    UNUSED_PARAMETER(OnlyOnNextVsync);
    UNUSED_PARAMETER(SkipMode);

    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);
    /* reset flags */
    LCMPEG_CONTEXT_P->OverWriteEnabled     = FALSE;
    /*TraceBuffer(("SKIP \r\n"));*/
    /* Notify missing interrupts */
    Hal_Context_p   = (HALDEC_Properties_t *) HALDecodeHandle;
    LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
    /* Set interrupts */
    LCMPEG_CONTEXT_P->SWStatus |= HALDEC_DECODER_IDLE_MASK;
    LCMPEG_CONTEXT_P->SWStatus |= HALDEC_PIPELINE_IDLE_MASK;
    if((LCMPEG_CONTEXT_P->SWStatus & LCMPEG_CONTEXT_P->SWInterruptMask) != 0)
    {
        STEVT_Notify(Hal_Context_p->EventsHandle,
            LcmpegContext_p->InterruptEvtID,
            STEVT_EVENT_DATA_TYPE_CAST NULL);
    }
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);
} /* End of SkipPicture() function */

/*******************************************************************************
Name        : DisablePrimaryDecode
Description :
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisablePrimaryDecode(const HALDEC_Handle_t HALDecodeHandle)
{
    LCMPEG_CONTEXT_P->DecodeEnabled = FALSE;
} /* End of DisablePrimaryDecode() function */

/*******************************************************************************
Name        : EnablePrimaryDecode
Description : Enables Primary Decode
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnablePrimaryDecode(const HALDEC_Handle_t HALDecodeHandle)
{
    UNUSED_PARAMETER(HALDecodeHandle);
    LCMPEG_CONTEXT_P->DecodeEnabled = TRUE;
} /* End of EnablePrimaryDecode() function */

/*******************************************************************************
Name        : PrepareDecode
Description : Prepare decode of a picture
Parameters  : HAL decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PrepareDecode(const HALDEC_Handle_t HALDecodeHandle,
                void * const DecodeAddressFrom_p, const U16 pTemporalReference,
                BOOL * const WaitForVLD_p)
{
    UNUSED_PARAMETER(pTemporalReference);

    LCMPEG_CONTEXT_P->DecodeAdd = DecodeAddressFrom_p;

    /* No Need To wait decoder ready to start the picture decode */
    *WaitForVLD_p = FALSE;
} /* End of PrepareDecode() function */

/*******************************************************************************
Name        : SetDecodeConfiguration
Description : Fix parameters for next decode, this function is called in
              decode task context
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetDecodeConfiguration(
                         const HALDEC_Handle_t      HALDecodeHandle,
                         const StreamInfoForDecode_t * const StreamInfo_p)
{
    const PictureCodingExtension_t * PicCodExt_p    = &(StreamInfo_p->PictureCodingExtension);
    const Picture_t *                Pic_p          = &(StreamInfo_p->Picture);
#ifdef ST_deblock
    MPEG2Deblock_InitTransformerParam_t   TransformerInitParams
#endif /* ST_deblock */

/* OLGN - H264 */
#ifndef ST_OSWINCE
	#ifdef DEBUG
		M_TrackITS();
	#endif /* DEBUG */
#endif

/*#define TRACE_DEC_CONFIGURATION*/
#ifdef TRACE_DEC_CONFIGURATION
    TraceBuffer(("SetDec tempref=%d ",StreamInfo_p->Picture.temporal_reference));
#endif /* TRACE_DEC_CONFIGURATION */
    TracePictureType(("DecType", "%c%d",
		      Pic_p->picture_coding_type == 1 ? 'I' :
		      Pic_p->picture_coding_type == 2 ? 'P' :
		      Pic_p->picture_coding_type == 3 ? 'B' : '?',
		      StreamInfo_p->Picture.temporal_reference));

    /* Write width in macroblocks */
    LCMPEG_CONTEXT_P->DecodeParams.WidthInMacroBlock = ((U32)StreamInfo_p->HorizontalSize + 15) / 16;
    /* Write height in macroblocks */
    LCMPEG_CONTEXT_P->DecodeParams.HeightInMacroBlock = ((U32)StreamInfo_p->VerticalSize  + 15) / 16;
    /* Write total number of macroblocks */
    LCMPEG_CONTEXT_P->DecodeParams.TotalInMacroBlock = LCMPEG_CONTEXT_P->DecodeParams.HeightInMacroBlock * LCMPEG_CONTEXT_P->DecodeParams.WidthInMacroBlock;
    /* Same for MPEG1/MPEG2 : two least Significant bits of Coding Type */
    LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite = ((Pic_p->picture_coding_type & 0x03) << VID_PPR_PCT);
#ifdef TRACE_DEC_CONFIGURATION
    TraceBuffer(("W=%03d H=%03d T=%03d ",
        LCMPEG_CONTEXT_P->DecodeParams.WidthInMacroBlock,
        LCMPEG_CONTEXT_P->DecodeParams.HeightInMacroBlock,
        LCMPEG_CONTEXT_P->DecodeParams.TotalInMacroBlock));
    TraceBuffer(("type=%c ", Pic_p->picture_coding_type == 1 ? 'I' :
                             Pic_p->picture_coding_type == 2 ? 'P' :
                             Pic_p->picture_coding_type == 3 ? 'B' : '?'));
#endif
    /* Depending on MPEG1/MPEG2 */
    if (StreamInfo_p->MPEG1BitStream)
    {
#ifdef TRACE_DEC_CONFIGURATION
        TraceBuffer(("MPEG1 "));
        TraceBuffer(("back_f_code=%d ", ((Pic_p->backward_f_code) & 0x07)));
        TraceBuffer(("forw_f_code=%d ", ((Pic_p->forward_f_code) & 0x07)));
        TraceBuffer(("full_p_back=%d ", Pic_p->full_pel_backward_vector));
        TraceBuffer(("full_p_forw=%d ", Pic_p->full_pel_forward_vector));
#endif
        /* Setup for MPEG1 streams */
        LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (((Pic_p->backward_f_code) & 0x07) << VID_PPR_BFH);
        LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (((Pic_p->forward_f_code)  & 0x07) << VID_PPR_FFH);
        if (Pic_p->full_pel_backward_vector)
        {

            LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (1 << (VID_PPR_BFH+3));
        }
        if (Pic_p->full_pel_forward_vector)
        {
            LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (1 << (VID_PPR_FFH+3));
        }
    } /* end MPEG-1 */
    else
    {
#ifdef TRACE_DEC_CONFIGURATION
        TraceBuffer(("MPEG2 "));
        TraceBuffer(("intra_dc_prec=%d ", ((PicCodExt_p->intra_dc_precision) & 0x3)));
        TraceBuffer(("pict_struct=%d ", ((PicCodExt_p->picture_structure) & 0x3)));
        TraceBuffer(("top_f=%d ", PicCodExt_p->top_field_first));
        TraceBuffer(("frame_pred=%d ", PicCodExt_p->frame_pred_frame_dct));
        TraceBuffer(("concel_moti=%d ", PicCodExt_p->concealment_motion_vectors));
        TraceBuffer(("q_scal=%d ", PicCodExt_p->q_scale_type));
        TraceBuffer(("intra_vlc=%d ", PicCodExt_p->intra_vlc_format));
        TraceBuffer(("alternate_scan=%d ", PicCodExt_p->alternate_scan));
        TraceBuffer(("\r\n%03d %03d\r\n%03d %03d", (PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL]) & 0x0f,
                                                   (PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL]) & 0x0f,
                                                   (PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL]) & 0x0f,
                                                   (PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL]) & 0x0f));
#endif
        /* Setup for MPEG2 streams */
        LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (1  << VID_PPR_MP2);
        if (PicCodExt_p->picture_structure!= 0x3) /* 0x3 correpond to STVID_PICTURE_STRUCTURE_FRAME */
        {
            if (StreamInfo_p->ExpectingSecondField)
            {
                LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (1 << VID_PPR_FFN);
            }
            else
            {
                LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (0x3 << VID_PPR_FFN);
            }
        }
        if (PicCodExt_p->intra_dc_precision != 0x3) /* 0x3 is not allowed !*/
        {

            LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (PicCodExt_p->intra_dc_precision  & 0x3) << VID_PPR_DCP;
        }
        LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (PicCodExt_p->picture_structure & 0x3) << VID_PPR_PST;
        if (PicCodExt_p->top_field_first)
        {
            LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (1 << VID_PPR_TFF);
        }
        if (PicCodExt_p->frame_pred_frame_dct)
        {
            LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (1 << VID_PPR_FRM);
        }
        if (PicCodExt_p->concealment_motion_vectors)
        {
            LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (1 << VID_PPR_CMV);
        }
        if (PicCodExt_p->q_scale_type)
        {
            LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (1 << VID_PPR_QST);
        }
        if (PicCodExt_p->intra_vlc_format)
        {
            LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (1 << VID_PPR_IVF);
        }
        if (PicCodExt_p->alternate_scan)
        {
            LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= (U32) (1 << VID_PPR_AZZ);
        }
        LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= ((U32)(PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL]) & 0x0f) << VID_PPR_BFH;
        LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= ((U32)(PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL]) & 0x0f) << VID_PPR_FFH;
        LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= ((U32)(PicCodExt_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL]) & 0x0f) << VID_PPR_BFV;
        LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite |= ((U32)(PicCodExt_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL]) & 0x0f) << VID_PPR_FFV;
    } /* end MPEG-2 */
#ifdef ST_deblock
    if (LCMPEG_CONTEXT_P->DeblockingEnabled)
    {
        if ((StreamInfo_p->HorizontalSize > 720)||(StreamInfo_p->VerticalSize >576))/* HD stream, could not apply deblocking */
        {
            LCMPEG_CONTEXT_P->DecodeParams.CurrentDeblockingState = FALSE;
        }
        else
        {
            LCMPEG_CONTEXT_P->DecodeParams.CurrentDeblockingState = TRUE;
            TransformerInitParams.InputBufferBegin = (U32) LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p;
            TransformerInitParams.InputBufferEnd   = (U32) LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p;
            VID_DeblockingSetParams (&LCMPEG_CONTEXT_P->DeblockingHandle,(void *) StreamInfo_p,TransformerInitParams);

        }
    }
#endif  /* ST_deblock */

#ifdef TRACE_DEC_CONFIGURATION
    TraceBuffer(("\r\n"));
#endif
} /* End of SetDecodeConfiguration() function */

/*******************************************************************************
Name        : LoadIntraQuantMatrix
Description : Load quantization matrix for intra picture decoding, this function
              is called in decode task context
Parameters  : HAL Decode manager handle, pointer on matrix values
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LoadIntraQuantMatrix(const HALDEC_Handle_t   HALDecodeHandle,
                                 const QuantiserMatrix_t *Matrix_p,
                                 const QuantiserMatrix_t *ChromaMatrix_p)
{
    U32 i, Coef;

    UNUSED_PARAMETER(ChromaMatrix_p);

    if (Matrix_p != NULL)
    {
        for (i = 0; i < 64; i += 4)
        {
            Coef  = (U32) (*Matrix_p)[i    ];
            Coef |= (U32) (*Matrix_p)[i + 1] <<  8;
            Coef |= (U32) (*Matrix_p)[i + 2] << 16;
            Coef |= (U32) (*Matrix_p)[i + 3] << 24;
            LCMPEG_CONTEXT_P->DecodeParams.IntraCoefs[i/4] = Coef;
        }
    }
} /* End of LoadIntraQuantMatrix() function */

/*******************************************************************************
Name        : LoadNonIntraQuantMatrix
Description : Load quantization matrix for non-intra picture decoding, this
              function is called in decode task context
Parameters  : HAL Decode manager handle, pointer on matrix values
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LoadNonIntraQuantMatrix(const HALDEC_Handle_t   HALDecodeHandle,
                                    const QuantiserMatrix_t *Matrix_p,
                                    const QuantiserMatrix_t *ChromaMatrix_p)
{
    U32 i, Coef;

    UNUSED_PARAMETER(ChromaMatrix_p);

    if (Matrix_p != NULL)
    {
        for (i = 0; i < 64; i += 4)
        {
            Coef  = (U32) (*Matrix_p)[i    ];
            Coef |= (U32) (*Matrix_p)[i + 1] <<  8;
            Coef |= (U32) (*Matrix_p)[i + 2] << 16;
            Coef |= (U32) (*Matrix_p)[i + 3] << 24;
            LCMPEG_CONTEXT_P->DecodeParams.NonIntraCoefs[i/4] = Coef;
        }
    }
} /* End of LoadNonIntraQuantMatrix() function */

/*******************************************************************************
Name        : SetBackwardChromaFrameBuffer
Description : Set decoder backward reference chroma frame buffer, this function
              is called in decode task context
Parameters  : HAL Decode manager handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBackwardChromaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p)
{
    LCMPEG_CONTEXT_P->DecodeParams.BackwardChromaFrameBuffer = BufferAddress_p;
} /* End of SetBackwardChromaFrameBuffer() function */

/*******************************************************************************
Name        : SetBackwardLumaFrameBuffer
Description : Set decoder backward reference luma frame buffer, this function
              is called in decode task context
Parameters  : HAL Decode manager handle, buffer address
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetBackwardLumaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p)
{
    LCMPEG_CONTEXT_P->DecodeParams.BackwardLumaFrameBuffer = BufferAddress_p;
} /* End of SetBackwardLumaFrameBuffer() function */

/*******************************************************************************
Name        : SetDecodeChromaFrameBuffer
Description : Set decoder chroma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeChromaFrameBuffer(const HALDEC_Handle_t  HALDecodeHandle,
                                       void * const           BufferAddress_p)
{
    LCMPEG_CONTEXT_P->DecodeParams.DecodeChromaFrameBuffer = BufferAddress_p;
} /* End of SetDecodeChromaFrameBuffer() function */

/*******************************************************************************
Name        : SetDecodeLumaFrameBuffer
Description : Set decoder luma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDecodeLumaFrameBuffer(const HALDEC_Handle_t  HALDecodeHandle,
                                     void * const           BufferAddress_p)
{
    LCMPEG_CONTEXT_P->DecodeParams.DecodeLumaFrameBuffer = BufferAddress_p;
} /* End of SetDecodeLumaFrameBuffer() function */

/*******************************************************************************
Name        : SetForwardChromaFrameBuffer
Description : Set decoder forward reference chroma frame buffer, this function
              is called in decode task context
Parameters  : HAL Decode manager handle, buffer address
Assumptions : Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetForwardChromaFrameBuffer(HALDEC_Handle_t  HALDecodeHandle,
                                        void * const     BufferAddress_p)
{
    LCMPEG_CONTEXT_P->DecodeParams.ForwardChromaFrameBuffer = BufferAddress_p;
} /* End of SetForwardChromaFrameBuffer() function */

/*******************************************************************************
Name        : SetForwardLumaFrameBuffer
Description : Set decoder forward reference luma frame buffer, this function
              is called in decode task context
Parameters  : HAL Decode manager handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetForwardLumaFrameBuffer(HALDEC_Handle_t  HALDecodeHandle,
                                      void * const     BufferAddress_p)
{
    LCMPEG_CONTEXT_P->DecodeParams.ForwardLumaFrameBuffer = BufferAddress_p;
} /* End of SetForwardLumaFrameBuffer() function */

#ifdef ST_deblock
/*******************************************************************************
Name        : SetDeblockingMode
Description : enable/disable deblocking
Parameters  : HAL Decode manager handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDeblockingMode(const HALDEC_Handle_t  HALDecodeHandle,
                                     BOOL DeblockingEnabled)
{
    LCMPEG_CONTEXT_P->DeblockingEnabled = DeblockingEnabled;
    LCMPEG_CONTEXT_P->DecodeParams.CurrentDeblockingState = FALSE;
} /* End of SetDeblockingMode() function */

#ifdef VIDEO_DEBLOCK_DEBUG
/*******************************************************************************
Name        : SetDeblockingStrength
Description : change deblocking Strength
Parameters  : HAL Decode manager handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetDeblockingStrength(const HALDEC_Handle_t  HALDecodeHandle,
                                     int DeblockingStrength)
{
    LCMPEG_CONTEXT_P->DeblockingStrength = DeblockingStrength;
} /* End of SetDeblockingStrength() function */
#endif /* VIDEO_DEBLOCK_DEBUG  */
#endif /* ST_deblock */

/*******************************************************************************
Name        : StartDecodePicture
Description : Launch decode of a picture, this function is called
              decode task context
Parameters  : HAL Dec manager hl, flag telling whether the decode takes place
              on the same picture that is on display
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void StartDecodePicture(const HALDEC_Handle_t HALDecodeHandle,
                             const BOOL OnlyOnNextVsync,
                             const BOOL OverWrite,
                             const BOOL SecondaryOverWrite)
{
    UNUSED_PARAMETER(OnlyOnNextVsync);

    SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);

    if(LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
    {
        LCMPEG_CONTEXT_P->DecodeAdd         = LCMPEG_CONTEXT_P->PictureAdd;
        LCMPEG_CONTEXT_P->PictureAdd        = NULL;
    }
    /* Take into account the SecondaryOverWrite flag before starting decoding */
    LCMPEG_CONTEXT_P->OverWriteEnabled      = (OverWrite || SecondaryOverWrite);

    /* Notify an interrupt for the new decode */
    if (LCMPEG_CONTEXT_P->SWInterruptMask & HALDEC_PIPELINE_START_DECODING_MASK)
    {
        HALDEC_Properties_t  * Hal_Context_p;
        LcmpegContext_t *      LcmpegContext_p;

        TraceInterrupt(("%x PSD ", HALDecodeHandle));
        LCMPEG_CONTEXT_P->SWStatus |= HALDEC_PIPELINE_START_DECODING_MASK;
        Hal_Context_p = (HALDEC_Properties_t *) HALDecodeHandle;
        LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
        STEVT_Notify(Hal_Context_p->EventsHandle,
            LcmpegContext_p->InterruptEvtID,
            STEVT_EVENT_DATA_TYPE_CAST NULL);
    }
    /* Wait for the shared hardware decoder to be available */
    SEMAPHORE_WAIT(MultiDecodeController.HW_DecoderAccess_p);
    /* Start the decode */
    ExecuteDecodeNow(HALDecodeHandle, LCMPEG_CONTEXT_P->DecodeAdd);
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccess_p);

    /* Launch startcode detector */
    if(LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
    {
        SearchNextStartCode(HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);
    }
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
    STAVMEM_FreeBlockParams_t   FreeParams;
#ifdef WA_GNBvd63015
    VIDCOM_Task_t * const SharedAccessToMMETask_p = &(LCMPEG_CONTEXT_P->SharedAccessToMMETask);
#endif /* WA_GNBvd63015 */


#ifdef WA_GNBvd63015
    LCMPEG_CONTEXT_P->ProcessSharedVideoDeblockingInterruptIsRunning = FALSE;
    SEMAPHORE_SIGNAL(LCMPEG_CONTEXT_P->SharedAccessToMME_p);
    STOS_TaskDelay(10);
    STOS_TaskDelete(SharedAccessToMMETask_p->Task_p,
                                HAL_CONTEXT_P->CPUPartition_p,
                                SharedAccessToMMETask_p->TaskStack,
                                HAL_CONTEXT_P->CPUPartition_p);

    STOS_SemaphoreDelete(HAL_CONTEXT_P->CPUPartition_p,LCMPEG_CONTEXT_P->SharedAccessToMME_p);

#endif /* WA_GNBvd63015 */

    /* Ensure no FDMA transfer is done while stopping the injecter */
    VIDINJ_EnterCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    VIDINJ_TransferStop(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    /* Release FDMA transfers */
    VIDINJ_TransferReleaseInject(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    VIDINJ_LeaveCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);

    FreeParams.PartitionHandle = HAL_CONTEXT_P->AvmemPartition;

#ifdef ST_deblock
    VID_DeblockingTerm(LCMPEG_CONTEXT_P->DeblockingHandle);
#endif /* ST_deblock */

#if defined(LCMPEGX1_WA_GNBvd45748)
    STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->WAGNBvd45748BuffersHdl));
    LCMPEG_CONTEXT_P->RecoveryStream_p = NULL;
    LCMPEG_CONTEXT_P->RecoveryLuma_p = NULL;
    LCMPEG_CONTEXT_P->RecoveryChroma_p = NULL;
#endif /* defined(LCMPEGX1_WA_GNBvd45748) */
    STAVMEM_FreeBlock(&FreeParams,&(LCMPEG_CONTEXT_P->InputBufHdl));
    STAVMEM_FreeBlock(&FreeParams, &(LCMPEG_CONTEXT_P->SCListHdl));

    STOS_SemaphoreDelete(HAL_CONTEXT_P->CPUPartition_p,LCMPEG_CONTEXT_P->SharedAccess_p);

#ifdef ST_OSWINCE
    /* unmap decoder registers */
    WCE_VERIFY(HAL_CONTEXT_P->RegistersBaseAddress_p != NULL);
    UnmapPhysicalToVirtual(HAL_CONTEXT_P->RegistersBaseAddress_p);
#endif /* ST_OSWINCE */

    SEMAPHORE_WAIT(MultiDecodeController.HW_DecoderAccess_p);
    if (MultiDecodeController.NbInstalledIRQ > 0)
    {
        MultiDecodeController.NbInstalledIRQ --;
        if (MultiDecodeController.NbInstalledIRQ == 0)
        {
            /* Un-install interrupt handler after HAL has been terminated, i.e. after HW is fully de-activated.
            This is to be sure that no interrupt can occur after un-installing the interrupt handler, as recommended in OS20 doc. */
            STOS_InterruptLock();
    
            if (STOS_InterruptDisable(HAL_CONTEXT_P->InterruptNumber, HAL_CONTEXT_P->InterruptLevel) != STOS_SUCCESS)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem disabling interrupt !"));
            }
    
            if (STOS_InterruptUninstall(HAL_CONTEXT_P->InterruptNumber, HAL_CONTEXT_P->InterruptLevel, NULL) != STOS_SUCCESS)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem un-installing interrupt !"));
            }
    
#if !defined ST_OSWINCE
            /* Under WinCE, not possible to enable an uninstalled interrupt - not a share interrupt. */
            if (STOS_InterruptEnable(HAL_CONTEXT_P->InterruptNumber, HAL_CONTEXT_P->InterruptLevel) != STOS_SUCCESS)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem re-enbling interrupt !"));
            }
#endif
            STOS_InterruptUnlock();
        }
    }
    SEMAPHORE_SIGNAL(MultiDecodeController.HW_DecoderAccess_p);

   /* memory deallocation of the private datas */
   SAFE_MEMORY_DEALLOCATE(((HALDEC_Properties_t *) HALDecodeHandle)->PrivateData_p,
                          HAL_CONTEXT_P->CPUPartition_p,
                          sizeof(LcmpegContext_t));

} /* End of Term() function */

/* Private Functions -------------------------------------------------------- */

/*******************************************************************************
Name        : ExecuteDecodeNow
Description :
Parameters  :
Assumptions : !! Because of pending decodes list mechanism : this function cannot be
            called by several instances of hal decode at the same time !!
Limitations :
Returns     :
*******************************************************************************/
static void ExecuteDecodeNow(const HALDEC_Handle_t HALDecodeHandle, void* DecodeAdd)
{
    U32                 TaskInstruction;
    U32                 i;
#if defined(LCMPEGH1)
    U32                 RCMtoWrite = 0;
#endif /* LCMPEGH1 */
    void*               ES_Base_p;
    void*               ES_Top_p;
    U32                 FirstByteBeforeTheStartCodeAdd;

    /* set matrix*/
    for (i = 0; i < 64; i += 4)
    {
        HAL_WriteReg32(VID_QMWIp + i, LCMPEG_CONTEXT_P->DecodeParams.IntraCoefs[i/4]);
        HAL_WriteReg32(VID_QMWNIp+ i, LCMPEG_CONTEXT_P->DecodeParams.NonIntraCoefs[i/4]);
    }

    /* Set Bitbuffer limits */
    if(LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
    {
        ES_Base_p = LCMPEG_CONTEXT_P->ES_CircularBitBuffer.ES_Base_p;
        ES_Top_p  = LCMPEG_CONTEXT_P->ES_CircularBitBuffer.ES_Top_p;
        TracePerf((":%3d:STAC\r\n", time_now() / (ST_GetClocksPerSecond() / 1000)));
    }
    else
    {
        ES_Base_p = NULL;
        ES_Top_p  = NULL;
    }

    /* Set ES limits to the decoder */
    HAL_WriteReg32(VID_BBS, (U32)ES_Base_p);
    HAL_WriteReg32(VID_BBE, (U32)ES_Top_p + 1);

    /* set referencies */
    HAL_WriteReg32(VID_BFP,   LCMPEG_CONTEXT_P->DecodeParams.BackwardLumaFrameBuffer);
    HAL_WriteReg32(VID_BCHP,  LCMPEG_CONTEXT_P->DecodeParams.BackwardChromaFrameBuffer);
    HAL_WriteReg32(VID_FFP ,  LCMPEG_CONTEXT_P->DecodeParams.ForwardLumaFrameBuffer);
    HAL_WriteReg32(VID_FCHP,  LCMPEG_CONTEXT_P->DecodeParams.ForwardChromaFrameBuffer);
    /* set bufs in / out */
    HAL_WriteReg32(VID_RFP,   LCMPEG_CONTEXT_P->DecodeParams.DecodeLumaFrameBuffer);
    HAL_WriteReg32(VID_RCHP,  LCMPEG_CONTEXT_P->DecodeParams.DecodeChromaFrameBuffer);
    TraceFrameBuffer(("DecFB", "0x%x", LCMPEG_CONTEXT_P->DecodeParams.DecodeLumaFrameBuffer));

    /* set the decoding address
     * the DecodeAdd given is always the address of the first slice start code, so
     * we have to launch the decode before the beginning of the sequence 00 00 01 01 */
    FirstByteBeforeTheStartCodeAdd = (U32)DecodeAdd - 4;
    if(FirstByteBeforeTheStartCodeAdd < (U32)ES_Base_p)
    {
        FirstByteBeforeTheStartCodeAdd += (U32)ES_Top_p - (U32)ES_Base_p + 1;
    }

    HAL_WriteReg32(VID_VLDPTR, FirstByteBeforeTheStartCodeAdd);
#ifdef TRACE_UART
    if(DecodeAdd == NULL)
    {
        TraceBuffer(("DecodeAdd=NULL\r\n"));
    }
#endif

#ifdef TRACE_UART
{
    U8  SC[4];

    if( ((U32)FirstByteBeforeTheStartCodeAdd+1) > (U32)ES_Top_p)
    {
        SC[0] = STSYS_ReadRegMemUncached8(((U8*)((U32)FirstByteBeforeTheStartCodeAdd+1-((U32)ES_Top_p + 1 - (U32)ES_Base_p))));
    }
    else
    {
        SC[0] = STSYS_ReadRegMemUncached8(((U8*)((U32)FirstByteBeforeTheStartCodeAdd+1)));
    }

    if( ((U32)FirstByteBeforeTheStartCodeAdd+2) > (U32)ES_Top_p)
    {
        SC[1] = STSYS_ReadRegMemUncached8(((U8*)((U32)FirstByteBeforeTheStartCodeAdd+2-((U32)ES_Top_p + 1 - (U32)ES_Base_p))));
    }
    else
    {
        SC[1] = STSYS_ReadRegMemUncached8(((U8*)((U32)FirstByteBeforeTheStartCodeAdd+2)));
    }

    if( ((U32)FirstByteBeforeTheStartCodeAdd+3) > (U32)ES_Top_p)
    {
        SC[2] = STSYS_ReadRegMemUncached8(((U8*)((U32)FirstByteBeforeTheStartCodeAdd+3-((U32)ES_Top_p + 1 - (U32)ES_Base_p))));
    }
    else
    {
        SC[2] = STSYS_ReadRegMemUncached8(((U8*)((U32)FirstByteBeforeTheStartCodeAdd+3)));
    }

    if( ((U32)FirstByteBeforeTheStartCodeAdd+4) > (U32)ES_Top_p)
    {
        SC[3] = STSYS_ReadRegMemUncached8(((U8*)((U32)FirstByteBeforeTheStartCodeAdd+4-((U32)ES_Top_p + 1 - (U32)ES_Base_p))));
    }
    else
    {
        SC[3] = STSYS_ReadRegMemUncached8(((U8*)((U32)FirstByteBeforeTheStartCodeAdd+4)));
    }

    if((void*)FirstByteBeforeTheStartCodeAdd == NULL)
    {
        TraceBuffer(("FirstByteBeforeTheStartCodeAdd=NULL\r\n"));
    }
    if(!((SC[0]==0x0) && (SC[1]==0x0) && (SC[2]==0x1) && (SC[3]==0x1)))
    {
        TraceBuffer(("Dec@%08x address error ! Bytes are %02x %02x %02x %02x\r\n",
                    FirstByteBeforeTheStartCodeAdd,
                    SC[0],
                    SC[1],
                    SC[2],
                    SC[3]
       ));
    }
/*    else*/
/*    {*/
/*        TraceBuffer(("Dec@%08x\r\n", FirstByteBeforeTheStartCodeAdd));*/
/*    }*/
    if(LCMPEG_CONTEXT_P->LastDecodeAdd == DecodeAdd)
    {
        TraceBuffer(("Error ! Trying to decode again the same picture !\r\n"));
    }
    LCMPEG_CONTEXT_P->LastDecodeAdd = DecodeAdd;
}
#endif /* TRACE_UART */

    /* set params */
    HAL_WriteReg32(VID_PPR, LCMPEG_CONTEXT_P->DecodeParams.PPR_ToWrite);
    HAL_WriteReg32(VID_DFH, LCMPEG_CONTEXT_P->DecodeParams.HeightInMacroBlock);
    HAL_WriteReg32(VID_DFW, LCMPEG_CONTEXT_P->DecodeParams.WidthInMacroBlock);
    HAL_WriteReg32(VID_DFS, LCMPEG_CONTEXT_P->DecodeParams.TotalInMacroBlock);


#if defined(LCMPEGH1)
    /* Decimation management done in this hal but only for lcmpegh1     */

    /* Update secondary reconstruction frame pointer.   */
    if (LCMPEG_CONTEXT_P->SecondaryDecodeEnabled)
    {
        HAL_WriteReg32(VID_SRFP, (U32)LCMPEG_CONTEXT_P->DecodeParams.SecondaryDecodeLumaFrameBuffer);
        HAL_WriteReg32(VID_SRCHP, (U32)LCMPEG_CONTEXT_P->DecodeParams.SecondaryDecodeChromaFrameBuffer);
    }
    /* Also update the reconstruction mode.             */
    RCMtoWrite =
            (LCMPEG_CONTEXT_P->DecodeEnabled          ? 1 << VID_RCM_ENM : 0)|
            (LCMPEG_CONTEXT_P->SecondaryDecodeEnabled ? 1 << VID_RCM_ENS : 0)|
            (LCMPEG_CONTEXT_P->DecodeParams.SourceScanType == STVID_SCAN_TYPE_PROGRESSIVE ?
             1 << VID_RCM_PS : 0) ;

    RCMtoWrite |= (LCMPEG_CONTEXT_P->DecodeParams.HDecimation == STVID_DECIMATION_FACTOR_H2
                    ? (VID_RCM_DEC2 << VID_RCM_HDEC) :
                   LCMPEG_CONTEXT_P->DecodeParams.HDecimation == STVID_DECIMATION_FACTOR_H4
                    ? (VID_RCM_DEC4 << VID_RCM_HDEC) : 0);

    RCMtoWrite |= (LCMPEG_CONTEXT_P->DecodeParams.VDecimation == STVID_DECIMATION_FACTOR_V2
                    ? (VID_RCM_DEC2 << VID_RCM_VDEC) :
                   LCMPEG_CONTEXT_P->DecodeParams.VDecimation == STVID_DECIMATION_FACTOR_V4
                    ? (VID_RCM_DEC4 << VID_RCM_VDEC) : 0);

    HAL_WriteReg32(VID_RCM, RCMtoWrite);

#endif /* LCMPEGH1 */

#if defined(LCMPEGH1)
    if ((LCMPEG_CONTEXT_P->DecodeEnabled)||(LCMPEG_CONTEXT_P->SecondaryDecodeEnabled))
#else
    if (LCMPEG_CONTEXT_P->DecodeEnabled)
#endif /* LCMPEGH1 */
    {
       /* Set interrupt mask always before a new decode. To be sure that the
	* decoder mutex will be unlocked when the decode finishes, set always
	* the bitmask VID_STA_DID. */
       HAL_WriteReg32(VID_ITM, LCMPEG_CONTEXT_P->HWInterruptMask | (1 << VID_STA_DID));
       /* --- GO --- */
       TaskInstruction =  (( 1 << VID_TIS_RMM)
               |( 1 << VID_TIS_MVC)
                          |( 0 << VID_TIS_SBD)
                          |((LCMPEG_CONTEXT_P->OverWriteEnabled? 1:0) << VID_TIS_OVW));
       HAL_WriteReg32(VID_TIS, TaskInstruction);
       STOS_InterruptLock();
       MultiDecodeController.HALDecodeHandleForNextIT = HALDecodeHandle;
       HAL_WriteReg32(VID_EXE, 0x000000ff);
       STOS_InterruptUnlock();
       TraceDuration(("DecDur",
		      (LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR) ? "C" :
		      (((U32)DecodeAdd >= (U32)LCMPEG_CONTEXT_P->ES_LinearBitBufferB.ES_Base_p) &&
		       ((U32)DecodeAdd <= (U32)LCMPEG_CONTEXT_P->ES_LinearBitBufferB.ES_Top_p)) ? "B" :
		      (((U32)DecodeAdd >= (U32)LCMPEG_CONTEXT_P->ES_LinearBitBufferA.ES_Base_p) &&
		       ((U32)DecodeAdd <= (U32)LCMPEG_CONTEXT_P->ES_LinearBitBufferA.ES_Top_p)) ? "A" : "?"));
    }
#ifdef ST_deblock
    else
    {
      /* Release HW semaphore to avoid blocking on next decode (see GNBvd60624) */
      SEMAPHORE_SIGNAL(MultiDecodeController.HW_DecoderAccess_p);
    }
#endif /* ST_deblock */
} /* end of ExecuteDecodeNow() function */

/*******************************************************************************
Name        : InternalProcessInterruptStatus
Description : Process interrupt status for internal purposes.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void InternalProcessInterruptStatus(const HALDEC_Handle_t HALDecodeHandle, const U32 Status)
{
    /* Hardware decoder is now available: signal in case decode are pending */
    if( Status & (1 << VID_STA_DID) )
    {
#ifdef ST_deblock
        /* Avoid that MultiDecodeController.HALDecodeHandleForNextIT is changed
         by the interrupt context. Needed here because this function is not called
         from interrupt context when deblocking is used. */
        STOS_InterruptLock();
#endif
        /* Reset flags */
        LCMPEG_CONTEXT_P->OverWriteEnabled      = FALSE;
        MultiDecodeController.HALDecodeHandleForNextIT = NULL;
        /* Disable interrupts for current instance, they will be reinstalled by
            the next instance that will hold the hardware decoder */
        HAL_WriteReg32(VID_ITM, 0x0);
#ifdef ST_deblock
       STOS_InterruptUnlock();
#endif
        SEMAPHORE_SIGNAL(MultiDecodeController.HW_DecoderAccess_p);
        TraceDuration(("DecDur", " "));
    }
} /* End of InternalProcessInterruptStatus() function */

/*******************************************************************************
Name        : SharedVideoInterruptHandler
Description : Video interrupt handler : we are under interrupt context !!
Parameters  :
Assumptions :
Limitations :
Returns     :
Comment     : STOS_INTERRUPT_DECLARE is used in order to be free from
            : OS dependent interrupt function prototype
*******************************************************************************/
static STOS_INTERRUPT_DECLARE(SharedVideoInterruptHandler, Param)
{
    HALDEC_Properties_t  * Hal_Context_p;
    LcmpegContext_t *      LcmpegContext_p;
    HALDEC_Handle_t        HALDecodeHandle;
    U32                    HWStatus;
    U32                    InterruptMask;
#ifdef ST_deblock
    MPEG2Deblock_TransformParam_t DeblockTransfromParams;
#endif /* ST_deblock */

    UNUSED_PARAMETER(Param);

#ifdef TRACE_UART
    if(MultiDecodeController.HALDecodeHandleForNextIT == NULL)
    {
        HWStatus = STSYS_ReadRegDev32LE((void *) (MultiDecodeController.RegistersBaseAddress_p + VID_ITS));

        TraceBuffer(("No instance to receive current interrupt...%x", HWStatus));
        if (HWStatus & (1 << VID_STA_DID))
        {
            TraceBuffer(("DID "));
        }
        if (HWStatus & (1 << VID_STA_DOE))
        {
            TraceBuffer(("DOE "));
        }
        if (HWStatus & (1 << VID_STA_DUE))
        {
            TraceBuffer(("DUE "));
        }
        if (HWStatus & (1 << VID_STA_MSE))
        {
            TraceBuffer(("MSE "));
        }
        if (HWStatus & (1 << VID_STA_DSE))
        {
            TraceBuffer(("DSE "));
        }
        if (HWStatus & (1 << VID_STA_VLDRL))
        {
            TraceBuffer(("VLDRL "));
        }
        if (HWStatus & (1 << VID_STA_R_OPC))
        {
            TraceBuffer(("%x ROPC ", HALDecodeHandle));
        }
        TraceBuffer(("\r\n"));
        goto exit;
    }
#else
    if (MultiDecodeController.HALDecodeHandleForNextIT == NULL)
    {
        /* acknowledge any pending IT */
        HWStatus = STSYS_ReadRegDev32LE((void *) (MultiDecodeController.RegistersBaseAddress_p + VID_ITS));
        goto exit;
    }
#endif /* TRACE_UART */

    /* The event will be catched by the instance identified */
    HALDecodeHandle             = MultiDecodeController.HALDecodeHandleForNextIT;
    Hal_Context_p = (HALDEC_Properties_t *) HALDecodeHandle;

    /* Get the interrupt mask */
    InterruptMask = HAL_ReadReg32(VID_ITM);
    /* Prevent from loosing interrupts ! Disable all interrupts */
    HAL_WriteReg32(VID_ITM, 0x0);
    /* Get the interrupt */
    HWStatus = HAL_ReadReg32(VID_ITS);
    /* Set again the interrupt mask to handle all interrupts */
    HAL_WriteReg32(VID_ITM, InterruptMask);
#ifdef ST_deblock
            LCMPEG_CONTEXT_P->Deblock_HWStatus = HWStatus;
            LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;

        if((HWStatus& (1 << VID_STA_DID) )&&(LcmpegContext_p->DecodeParams.CurrentDeblockingState))
        {
            /* 1- Set MME transformer Params
                2- Send command to LX
                3- Wait until receive Lx job cmpleted*/
#ifdef VIDEO_DEBLOCK_DEBUG
            if (!LcmpegContext_p->DeblockingStrength)
            {
                DeblockTransfromParams.DeblockingEnable = 4;
                DeblockTransfromParams.PictureParam.FilterOffsetQP          = 15;
            }
            else
            {
                DeblockTransfromParams.DeblockingEnable = 3;
                DeblockTransfromParams.PictureParam.FilterOffsetQP          =  LcmpegContext_p->DeblockingStrength;
            }
#else  /* ifndef VIDEO_DEBLOCK_DEBUG */
             DeblockTransfromParams.DeblockingEnable = 3;
             DeblockTransfromParams.PictureParam.FilterOffsetQP          =  15;
#endif /* VIDEO_DEBLOCK_DEBUG */
            DeblockTransfromParams.PictureStartAddrCompressedBuffer_p        = (void *) LcmpegContext_p->DecodeAdd;
            DeblockTransfromParams.PictureStopAddrCompressedBuffer_p         = (void *) LcmpegContext_p->EndOfPictureAdd;
            DeblockTransfromParams.PictureBufferAddress.DecodedLuma_p        = (void *) LcmpegContext_p->DecodeParams.DecodeLumaFrameBuffer;
            DeblockTransfromParams.PictureBufferAddress.DecodedChroma_p      = (void *) LcmpegContext_p->DecodeParams.DecodeChromaFrameBuffer;
            DeblockTransfromParams.PictureBufferAddress.MainLuma_p           = (void *) LcmpegContext_p->DecodeParams.SecondaryDecodeLumaFrameBuffer;
            DeblockTransfromParams.PictureBufferAddress.MainChroma_p         = (void *) LcmpegContext_p->DecodeParams.SecondaryDecodeChromaFrameBuffer;
#ifdef WA_GNBvd63015
            LcmpegContext_p->DeblockTransfromParams = DeblockTransfromParams;
            SEMAPHORE_SIGNAL(LcmpegContext_p->SharedAccessToMME_p);
#else
            VID_DeblockingStart(&LcmpegContext_p->DeblockingHandle,DeblockTransfromParams);
#endif /* WA_GNBvd63015 */
        }
        else
        {
            LCMPEG_CONTEXT_P->HWStatus  |= HWStatus;
            /* Check if interrupt must be made available for upper level */
            if((LCMPEG_CONTEXT_P->HWStatus & LCMPEG_CONTEXT_P->HWInterruptMask) != 0)
            {
                LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
                STEVT_Notify(Hal_Context_p->EventsHandle,
                        LcmpegContext_p->InterruptEvtID,
                        STEVT_EVENT_DATA_TYPE_CAST NULL);
            }
            /* Process always internally the interrupt received */
            InternalProcessInterruptStatus(HALDecodeHandle, HWStatus);
        }
#else  /* not ST_deblock */
    {
        LCMPEG_CONTEXT_P->HWStatus  |= HWStatus;

        /* Check if interrupt must be made available for upper level */
        if((LCMPEG_CONTEXT_P->HWStatus & LCMPEG_CONTEXT_P->HWInterruptMask) != 0)
        {
            LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
            STEVT_Notify(Hal_Context_p->EventsHandle,
                    LcmpegContext_p->InterruptEvtID,
                    STEVT_EVENT_DATA_TYPE_CAST NULL);
        }

        /* Process always internally the interrupt received */
        InternalProcessInterruptStatus(HALDecodeHandle, HWStatus);
    }
#endif /* not ST_deblock */
exit:
    STOS_INTERRUPT_EXIT(STOS_SUCCESS);
} /* end of SharedVideoInterruptHandler() function */

/*******************************************************************************
Name        : DMATransferDoneFct
Description : Called from inject module when a slice is transfered
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DMATransferDoneFct(U32 UserIdent, void * ES_Write_p,
                               void * SCListWrite_p,
                               void * SCListLoop_p
#if defined(DVD_SECURED_CHIP)
                             , void * ESCopy_Write_p
#endif /* Secure chip */
)
{
    HALDEC_Properties_t  *                  Hal_Context_p;
    LcmpegContext_t *                       LcmpegContext_p;

    Hal_Context_p = (HALDEC_Properties_t *) UserIdent;
    LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;

    /* Update the write and loop pointers each time an FDMA transfer is done */
    SEMAPHORE_WAIT(LcmpegContext_p->SharedAccess_p);
    LcmpegContext_p->InjBuffer_p->ES_Write_p        = ES_Write_p;
    /* The FDMA should never be blocked because of a bitbuffer full. We authorize a rewrite, but this should never happen
    * if injection is from live because of VBV delay. Memory injection will be controlled from the bitbuffer free size
    * value returned to the higher levels (see GetDecodeBitbufferLevel) */
    if(LcmpegContext_p->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
    {
        VIDINJ_TransferSetESRead(Hal_Context_p->InjecterHandle, LcmpegContext_p->InjectHdl, (void*) (((U32)ES_Write_p)&ALIGNEMENT_FOR_TRANSFERS));
    }

#if defined(DVD_SECURED_CHIP)
    /* DG TO DO: For secure platforms, call VIDINJ_TransferSetESCopyRead() */
    UNUSED_PARAMETER(ESCopy_Write_p);
#endif /* DVD_SECURED_CHIP */

    if((U32)SCListWrite_p > (U32)LcmpegContext_p->InjBuffer_p->SCList_Write_p)
    {
        /* No loop in the startcode list */
        if((U32)LcmpegContext_p->InjBuffer_p->NextSCEntry_p <= (U32)LcmpegContext_p->InjBuffer_p->SCList_Write_p)
        {
            /* Case 1 : -----R---W
             *                   L */
            LcmpegContext_p->InjBuffer_p->SCList_Loop_p  = SCListWrite_p;
        }
/*        else*/
/*        {*/
            /* Case 2 : ---W----R--L */
            /* Noting to do, loop pointer is always the same */
/*        }*/
    }
    else if((U32)SCListWrite_p < (U32)LcmpegContext_p->InjBuffer_p->SCList_Write_p)
    {
        /* Case 3 : Loop in the startcode list
         *          -----R----W
         * =>       W----R------L */
        LcmpegContext_p->InjBuffer_p->SCList_Loop_p = SCListLoop_p;
    }
    LcmpegContext_p->InjBuffer_p->SCList_Loop_p  = SCListLoop_p;
    LcmpegContext_p->InjBuffer_p->SCList_Write_p = SCListWrite_p;

    /* If a startcode detection has already been launched, and new startcodes were found,
        notify immediately to the higher level a startcode hit interrupt */
    if((LcmpegContext_p->SCDetectSimuLaunched) && (IsThereANewStartCode((HALDEC_Handle_t)Hal_Context_p)))
    {
        LcmpegContext_p->SWStatus |= HALDEC_START_CODE_HIT_MASK;
        if((LcmpegContext_p->SWStatus & LcmpegContext_p->SWInterruptMask) != 0)
        {
            STEVT_Notify(Hal_Context_p->EventsHandle,
                LcmpegContext_p->InterruptEvtID,
                STEVT_EVENT_DATA_TYPE_CAST NULL);
        }
        LcmpegContext_p->SCDetectSimuLaunched = FALSE;
    }

    SEMAPHORE_SIGNAL(LcmpegContext_p->SharedAccess_p);
} /* end of DMATransferDoneFct() function */

/*******************************************************************************
Name        : PointOnNextSC
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void PointOnNextSC(LcmpegContext_t * LcmpegContext_p)
{
    STFDMA_SCEntry_t*   SCList_Write_p = LcmpegContext_p->DecBuffer_p->SCList_Write_p;
    STFDMA_SCEntry_t*   SCList_Loop_p  = LcmpegContext_p->DecBuffer_p->SCList_Loop_p;

#ifdef TRACE_UART
    if(LcmpegContext_p->DecBuffer_p->NextSCEntry_p == SCList_Write_p)
    {
        TraceBuffer(("Trying to point after the SCLIST_Write_p !\r\n"));
    }
#endif
    /* Step the sc pointer */
    if(LcmpegContext_p->DecBuffer_p->NextSCEntry_p == NULL)
    {
        LcmpegContext_p->DecBuffer_p->NextSCEntry_p = LcmpegContext_p->DecBuffer_p->SCList_Base_p;
    }
    else if(LcmpegContext_p->DecBuffer_p->NextSCEntry_p != SCList_Write_p)
    {
        LcmpegContext_p->DecBuffer_p->NextSCEntry_p++;
    }
    if(   (LcmpegContext_p->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
        &&(LcmpegContext_p->DecBuffer_p->NextSCEntry_p == SCList_Loop_p)
        &&(LcmpegContext_p->DecBuffer_p->NextSCEntry_p != SCList_Write_p))
    {
        /* sc list end: re-start from begining only for circular bitbuffer management */
        LcmpegContext_p->DecBuffer_p->NextSCEntry_p = LcmpegContext_p->DecBuffer_p->SCList_Base_p;
        LcmpegContext_p->DecBuffer_p->SCList_Loop_p = LcmpegContext_p->DecBuffer_p->SCList_Write_p;
    }
} /* end of PointOnNextSC() function */

/*******************************************************************************
Name        : GetAByteFromES
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U8 GetAByteFromES(const HALDEC_Handle_t HALDecodeHandle)
{
    U8 Val8;
    LcmpegContext_t * LcmpegContext_p = LCMPEG_CONTEXT_P;
#ifdef NATIVE_CORE
	U8 *CPU_NextDataPointer_p;
#endif

#ifdef TRACE_UART
    if(!(
          (LcmpegContext_p->NextDataPointer_p >= LcmpegContext_p->DecBuffer_p->ES_Base_p) &&
          (LcmpegContext_p->NextDataPointer_p <= LcmpegContext_p->DecBuffer_p->ES_Top_p)))
    {
        TraceBuffer(("Trying to read a byte out of the bitbuffer !\r\n"));
    }
#endif
    if(LcmpegContext_p->NextDataPointer_p == LcmpegContext_p->DecBuffer_p->ES_Write_p)
    {
        /* bit buffer empty : we should wait... (TODO)*/
        Val8 = 0;
        TraceBuffer(("Error reading 1 byte from ES\r\n"));
    }
    /* ok, current should be < write_ptr:  do the step ... */
#ifdef NATIVE_CORE
    CPU_NextDataPointer_p = STAVMEM_DeviceToCPU((LcmpegContext_p->NextDataPointer_p), &LcmpegContext_p->VirtualMapping);

    Val8 = STSYS_ReadRegMemUncached8((U8*)CPU_NextDataPointer_p);
#else
	Val8 = STSYS_ReadRegMemUncached8((U8*)LcmpegContext_p->NextDataPointer_p);
#endif

    LcmpegContext_p->NextDataPointer_p ++;
    if(  (LcmpegContext_p->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
       &&(LcmpegContext_p->NextDataPointer_p > LcmpegContext_p->DecBuffer_p->ES_Top_p))
    {
        /* Loop only if in circular bitbuffer management */
        LcmpegContext_p->NextDataPointer_p = LcmpegContext_p->DecBuffer_p->ES_Base_p;
    }

    return(Val8);
} /* end of GetAByteFromES() function */

/*******************************************************************************
Name        : DisableSecondaryDecode
Description : Unused for SD
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableSecondaryDecode(const HALDEC_Handle_t  HALDecodeHandle)
{
#if defined(LCMPEGH1)
    LCMPEG_CONTEXT_P->SecondaryDecodeEnabled                         = FALSE;
    LCMPEG_CONTEXT_P->DecodeParams.HDecimation                       = 0;
    LCMPEG_CONTEXT_P->DecodeParams.VDecimation                       = 0;
    LCMPEG_CONTEXT_P->DecodeParams.SourceScanType                    = 0;
    LCMPEG_CONTEXT_P->DecodeParams.SecondaryDecodeChromaFrameBuffer  = 0;
    LCMPEG_CONTEXT_P->DecodeParams.SecondaryDecodeLumaFrameBuffer    = 0;
#endif /* LCMPEGH1 */
} /* End of DisableSecondaryDecode() function */

/*******************************************************************************
Name        : EnableSecondaryDecode
Description : Feature used only for HD
Parameters  :
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
#if defined(LCMPEGH1)
#ifdef ST_deblock
    if (!LCMPEG_CONTEXT_P->DecodeParams.CurrentDeblockingState)
#endif /* ST_deblock */
    {
        LCMPEG_CONTEXT_P->SecondaryDecodeEnabled              = TRUE;
        LCMPEG_CONTEXT_P->DecodeParams.HDecimation            = H_Decimation;
        LCMPEG_CONTEXT_P->DecodeParams.VDecimation            = V_Decimation;
        LCMPEG_CONTEXT_P->DecodeParams.SourceScanType         = ScanType;
    }
#endif
} /* End of EnableSecondaryDecode() function */

/*******************************************************************************
Name        : DisableHDPIPDecode
Description : Unused for SD
Parameters  : HAL Decode manager handle, HDPIP parameters.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableHDPIPDecode(const HALDEC_Handle_t HALDecodeHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    UNUSED_PARAMETER(HALDecodeHandle);
    UNUSED_PARAMETER(HDPIPParams_p);
} /* End of DisableHDPIPDecode() function */

/*******************************************************************************
Name        : EnableHDPIPDecode
Description : Unused for SD
Parameters  : HAL Decode manager handle, HDPIP parameters.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnableHDPIPDecode(const HALDEC_Handle_t  HALDecodeHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p)
{
      UNUSED_PARAMETER(HALDecodeHandle);
      UNUSED_PARAMETER(HDPIPParams_p);
} /* End of EnableHDPIPDecode() function */

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
    /* unused in this hal 'standart def' */
    UNUSED_PARAMETER(HALDecodeHandle);
    UNUSED_PARAMETER(Compression);
} /* End of SetMainDecodeCompression() function */

/*******************************************************************************
Name        : SetSecondaryDecodeChromaFrameBuffer
Description : Set secondary decoder chroma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryDecodeChromaFrameBuffer(
                         const HALDEC_Handle_t  HALDecodeHandle,
                         void * const           BufferAddress_p)
{
    /* Used in this hal but only for lcmpegh1 cell */
#if defined(LCMPEGH1)
    LCMPEG_CONTEXT_P->DecodeParams.SecondaryDecodeChromaFrameBuffer   = BufferAddress_p;
#endif /* LCMPEGH1 */
} /* End of SetSecondaryDecodeChromaFrameBuffer() function */

/*******************************************************************************
Name        : SetSecondaryDecodeLumaFrameBuffer
Description : Set secondary decode luma frame buffer
Parameters  : HAL Decode manager handle, buffer address
Assumptions : Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetSecondaryDecodeLumaFrameBuffer(
                    const HALDEC_Handle_t  HALDecodeHandle,
                    void * const    BufferAddress_p)
{
    /* Used in this hal but only for lcmpegh1 cell */
#if defined(LCMPEGH1)
    LCMPEG_CONTEXT_P->DecodeParams.SecondaryDecodeLumaFrameBuffer   = BufferAddress_p;
#endif /* LCMPEGH1 */
} /* End of SetSecondaryDecodeLumaFrameBuffer() function */

/*******************************************************************************
Name        : WriteStartCode
Description : Writes a new startcode in the bitbuffer and adds it in the startcode list
Parameters  : HAL Decode manager handle, startcode value, startcode address
Assumptions : Limitations :
Returns     :
*******************************************************************************/
static void WriteStartCode(
                    const HALDEC_Handle_t  HALDecodeHandle,
                    const U32             SCVal,
                    const void * const    SCAdd_p)
{
    const U8*  Temp8;

    /* Write the startcode in the bitbuffer */
    Temp8 = (const U8*) SCAdd_p;
    STSYS_WriteRegMemUncached8(Temp8    , 0x0);
    STSYS_WriteRegMemUncached8(Temp8 + 1, 0x0);
    STSYS_WriteRegMemUncached8(Temp8 + 2, 0x1);
    STSYS_WriteRegMemUncached8(Temp8 + 3, SCVal);

    /* Ensure no FDMA transfer is done while injecting the startcode in the startcode list */
    VIDINJ_EnterCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    /* Add the startcode in the startcode list */
    VIDINJ_InjectStartCode(HAL_CONTEXT_P->InjecterHandle,
                           LCMPEG_CONTEXT_P->InjectHdl,
                           SCVal,
                           SCAdd_p);
    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
} /* End of WriteStartCode() function */

/*******************************************************************************
Name        : IsThereANewStartCode
Description : Checks if a startcode is available in the startcode list
Parameters  : HAL Decode manager handle, buffer address
Assumptions : Limitations :
Returns     : TRUE if a startcode is available
              FALSE if not
*******************************************************************************/
static BOOL IsThereANewStartCode(const HALDEC_Handle_t HALDecodeHandle)
{
    BOOL                                    SCFound = FALSE;
    STFDMA_SCEntry_t*                       CPUNextSCEntry_p;
    osclock_t                               Timeout, Initialtime ;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U8                                      Counter;

    /* Don't notify a startcode hit if the interruption is disabled */
    if((LCMPEG_CONTEXT_P->SWInterruptMask & HALDEC_START_CODE_HIT_MASK) == 0)
    {
        return(FALSE);
    }

    /* Loop SCEntry_p pointer if we reach the end of the startcode list */
    if(  (LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
       &&(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p  == LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Loop_p)
       &&(LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Write_p != LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Loop_p))
    {
        LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p = LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Base_p;
        LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Loop_p = LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Write_p;
    }
    /* If the startcode list is not empty, try to find a startcode within */
    if(   (LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Base_p != NULL)
        &&(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p != LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Write_p))
    {
        /* Arm timeout to avoid having an infinite startcodes search */
        Initialtime = time_now();
        Counter = 0;

        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

        /* search for an available SC in our list, but avoid to make infinite search */
        while(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p != LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Write_p)
        {
#ifdef ST_5100
            /* This chip dependency should be removed once addresses management is generalized */
            CPUNextSCEntry_p = LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p;
#else
            CPUNextSCEntry_p = STAVMEM_DeviceToCPU(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p, &VirtualMapping);
#endif
            if(!IsPTS(CPUNextSCEntry_p))
            {
                /* we have a sc !! */
                if(!(LCMPEG_CONTEXT_P->FirstSequenceReached))
                {
                    /* in this special case, we skip all until a sequence */
                    if(GetSCVal(CPUNextSCEntry_p) == SEQUENCE_HEADER_CODE)
                    {
                        /* OK , it is our first sequence sc */
                        SCFound = TRUE;
                        LCMPEG_CONTEXT_P->FirstSequenceReached = TRUE;
                        TraceSC(("SC_p (%08x) h%02x@%lu", CPUNextSCEntry_p, CPUNextSCEntry_p->SC.SCValue, CPUNextSCEntry_p->SC.Addr));
                        TraceSC((" First Sequence !!!\r\n"));
                    }
                    else if(GetSCVal(CPUNextSCEntry_p) == PICTURE_START_CODE)
                    {
                        /* Try to find an associated PTS to this picture in the saved list of PTS */
                        SearchForAssociatedPTS(HALDecodeHandle);
                    }
                    else
                    {
                        TraceSC(("SC_p (%08x) h%02x S\r\n", CPUNextSCEntry_p, CPUNextSCEntry_p->SC.SCValue, CPUNextSCEntry_p->SC.Addr));
                    }

                    /* Consume data in ES bit buffer */
                    LCMPEG_CONTEXT_P->NextDataPointer_p         = (void *)GetSCAdd(CPUNextSCEntry_p);
                }
                else
                {
                    U32     DiffPSC;

                    /* Output counter management : compute the number of bytes between the two startcodes
                     * and add the total to the output counter */
                    if ((U32) (LCMPEG_CONTEXT_P->DecBuffer_p->LastSCAdd_p) < (U32) GetSCAdd(CPUNextSCEntry_p))
                    {
                        DiffPSC = (U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(LCMPEG_CONTEXT_P->DecBuffer_p->LastSCAdd_p);
                    }
                    else
                    {
                        /* ES buffer (bit buffer) looped. */
                        DiffPSC = ((U32)(LCMPEG_CONTEXT_P->DecBuffer_p->ES_Top_p) -
                                   (U32)(LCMPEG_CONTEXT_P->DecBuffer_p->LastSCAdd_p)
                                 + (U32)(GetSCAdd(CPUNextSCEntry_p))
                                 - (U32)(LCMPEG_CONTEXT_P->DecBuffer_p->ES_Base_p) + 1);
                    }
                    LCMPEG_CONTEXT_P->DecBuffer_p->LastSCAdd_p   = (void*)GetSCAdd(CPUNextSCEntry_p);
                    LCMPEG_CONTEXT_P->OutputCounter              += DiffPSC;
                    LCMPEG_CONTEXT_P->NextDataPointer_p          = (void *)GetSCAdd(CPUNextSCEntry_p);

                    /* Don't generate a startcode interrupt if previous one is a userdata */
                    if (LCMPEG_CONTEXT_P->SkipNextStartCode)
                    {
                        LCMPEG_CONTEXT_P->SkipNextStartCode = FALSE;
                        ProcessSC(HALDecodeHandle);
                        TraceSC(("SC_p (%08x) h%02x@%lu S\r\n", CPUNextSCEntry_p, CPUNextSCEntry_p->SC.SCValue, CPUNextSCEntry_p->SC.Addr));
                    }
                    else
                    {
                        SCFound = TRUE;
                        ProcessSC(HALDecodeHandle);
                        TraceSC(("SC_p (%08x) h%02x@%lu\r\n", CPUNextSCEntry_p, CPUNextSCEntry_p->SC.SCValue, CPUNextSCEntry_p->SC.Addr));
                    }
                }
            }
            else
            {
                /* we have a PTS, so we have to store it in the PTS list */
                SavePTSEntry(HALDecodeHandle, CPUNextSCEntry_p);
                TraceSC(("PTS  (%08x) h%02x\r\n", CPUNextSCEntry_p, CPUNextSCEntry_p->SC.SCValue, CPUNextSCEntry_p->SC.Addr));
            }
            /* If we haven't found a SC, continue search on the next SCEntry. The NextSCEntry_p points on
             * either the current SCEntry from we where we take the startcode value, or on the next
             * SCEntry from we will begin the startcode search. */
            if(!(SCFound))
            {
                PointOnNextSC(LCMPEG_CONTEXT_P);
                if(LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p == LCMPEG_CONTEXT_P->DecBuffer_p->SCList_Write_p)
                {
                    /* Quit the while() loop as list has been fully parsed, and no result */
                    break;
                }
                /* To respect MAX_START_CODE_SEARCH_TIME, check it every 9 entries... (an example) */
                Counter++;
                if ((Counter % 9) == 0)
                {
                    Timeout = time_plus(Initialtime, MAX_START_CODE_SEARCH_TIME);
                    if (!STOS_time_after(Timeout, time_now()))
                    {
                        /* Quit the while() loop as time-out has gone */
                        break;
                    }
                }
            }
            else
            {
                /* Quit the while() loop as a SC was found */
                break;
            }
        } /* end loop */
    }
    /* Inform injecter about the new SCList read pointer, to manage the circular startcode list */
    if(LCMPEG_CONTEXT_P->BitBufferType == HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR)
    {
        VIDINJ_TransferSetSCListRead(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl, LCMPEG_CONTEXT_P->DecBuffer_p->NextSCEntry_p);
    }
    return(SCFound);
} /* End of IsThereANewStartCode() function */

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
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    /* Ensure no FDMA transfer is done while setting the FDMA node partition */
    VIDINJ_EnterCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);
    ErrorCode = VIDINJ_ReallocateFDMANodes(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl, SetupParams_p);
    VIDINJ_LeaveCriticalSection(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl);

    return(ErrorCode);
} /* End of Setup() function */

#ifdef ST_deblock
/*******************************************************************************
Name        : HALDEC_TransformerCallback_deblocking
Description : Transformer callback Receive events from Transformer,
              update decoder state, and notify upper layer.
Parameters  : Event, CallbackData, UserData
Assumptions : Limitations :
Returns     : None
*******************************************************************************/
static void HALDEC_TransformerCallback_deblocking(U32 Event, void *CallbackData, void * UserData)
{
    HALDEC_Properties_t  * Hal_Context_p;
    LcmpegContext_t *      LcmpegContext_p;
    HALDEC_Handle_t        HALDecodeHandle;
    U32                  HWStatus;
    MPEG2Deblock_CommandStatus_t * CommandStatus;
    U32 Lx_Lx_Time=0;

    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(CallbackData);

    CommandStatus = (MPEG2Deblock_CommandStatus_t *)  UserData;
    Lx_Lx_Time = CommandStatus->DecodeTimeInMicros + CommandStatus->FilterTimeInMicros;

    if (MultiDecodeController.HALDecodeHandleForNextIT != NULL)
    {
        /* Avoid that MultiDecodeController.HALDecodeHandleForNextIT is changed
         by the interrupt context. */
        STOS_InterruptLock();
        /* The event will be catched by the instance identified */
        HALDecodeHandle             = MultiDecodeController.HALDecodeHandleForNextIT  ;
        STOS_InterruptUnlock();

        Hal_Context_p = (HALDEC_Properties_t *) HALDecodeHandle;
        LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;
        LcmpegContext_p->PictureMeanQP = CommandStatus->PictureMeanQP;
        LcmpegContext_p->PictureVarianceQP = CommandStatus->PictureVarianceQP;

        LCMPEG_CONTEXT_P->HWStatus |= LCMPEG_CONTEXT_P->Deblock_HWStatus;
        HWStatus = LCMPEG_CONTEXT_P->HWStatus;
        /* Check if interrupt must be made available for upper level */
        if((LCMPEG_CONTEXT_P->HWStatus & LCMPEG_CONTEXT_P->HWInterruptMask) != 0)
        {
            LcmpegContext_p = (LcmpegContext_t*)Hal_Context_p->PrivateData_p;

            STEVT_Notify(Hal_Context_p->EventsHandle,
                    LcmpegContext_p->InterruptEvtID,
                    STEVT_EVENT_DATA_TYPE_CAST NULL);
        }

        /* Process always internally the interrupt received */
        InternalProcessInterruptStatus(HALDecodeHandle, HWStatus);
    }

}/* End of HALDEC_TransformerCallback_deblocking() function */

#endif /* ST_deblock */

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
    VIDINJ_GetStatus(HAL_CONTEXT_P->InjecterHandle, LCMPEG_CONTEXT_P->InjectHdl, Status_p);

} /* End of SetDecodeBitBuffer() function */
#endif /* STVID_DEBUG_GET_STATUS */





/* end of lcmpegx1.c */
