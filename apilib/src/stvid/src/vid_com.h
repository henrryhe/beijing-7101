/*******************************************************************************

File name   : vid_com.h

Description : Video common definitions header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
13 Jul 2000        Created                                           HG
17 Jul 2001        Task priorities in #ifndef to be set from outside HG
 8 Oct 2001        No 2 task priorities equal to avoid time slicing  HG
26 Jan 2004        Added generic standard independant objects        HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_COMON_DEFINITIONS_H
#define __VIDEO_COMON_DEFINITIONS_H

/* Notes -------------------------------------------------------------------- */

/* NOTE_1 : #ifdef with this note NOTE_1 have the following comments.
   Added to save memory for old chips compilation : when VIDDEC only is compiled.
   Should be removed if one of the parameters inside are to be used outside VIDPROD, i.e. in generic code.
   Actually this is not the case, as the files api\vid_buf.c, decode\vid_dec.c,
   trickmod\vid_trick.c fill those fields for default values but nodoby use them except VIDPROD. */


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stos.h"

#include "stcommon.h"

#include "stvid.h"

#ifdef STVID_STVAPI_ARCHITECTURE
#include "stgvobj.h"
#endif /* STVID_STVAPI_ARCHITECTURE */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ------------------------------------------------------- */

#define STVID_IRQ_NAME          "stvid"
#define STVID_MAPPED_WIDTH      0x1000

/* Video working with 3 buffers need to have  : */
/*  No matter, because changes have been performed in decode not to depend on task priority.    */
/* In many cases, Pictures are inserted in display queue when the decode is launched, but not   */
/* at the end of it. So, thers is no more critical timing to manage : previous task priority    */
/* order applied again : Display > TrickMode > Decode.                                          */

/* decode task priority */
#ifndef STVID_TASK_PRIORITY_DECODE

#ifdef ST_OSLINUX
#define STVID_TASK_PRIORITY_DECODE          STVID_THREAD_DECODE_PRIORITY 
#else
#ifndef STVID_STVAPI_ARCHITECTURE
#define STVID_TASK_PRIORITY_DECODE          3
#else /* STVID_STVAPI_ARCHITECTURE */
#define STVID_TASK_PRIORITY_DECODE          MAX_DRIVER_PRIORITY
#endif /* not STVID_STVAPI_ARCHITECTURE */
#endif /* LINUX */
#endif /* not STVID_TASK_PRIORITY_DECODE */

/* parse task priority */
#ifndef STVID_TASK_PRIORITY_PARSE
#define STVID_TASK_PRIORITY_PARSE           STVID_TASK_PRIORITY_DECODE
#endif /* not STVID_TASK_PRIORITY_PARSE */

/* display task priority */
#ifndef STVID_TASK_PRIORITY_DISPLAY
#ifdef ST_OSLINUX
#define STVID_TASK_PRIORITY_DISPLAY         STVID_THREAD_DISPLAY_PRIORITY
#else
#define STVID_TASK_PRIORITY_DISPLAY         5
#endif /* LINUX */
#endif /* not STVID_TASK_PRIORITY_DISPLAY */

/* error recovery task priority */
#ifndef STVID_TASK_PRIORITY_ERROR_RECOVERY
#ifdef ST_OSLINUX
#define STVID_TASK_PRIORITY_ERROR_RECOVERY  STVID_THREAD_ERROR_RECOVERY_PRIORITY
#else
#ifndef STVID_STVAPI_ARCHITECTURE
#define STVID_TASK_PRIORITY_ERROR_RECOVERY  2
#else /* STVID_STVAPI_ARCHITECTURE */
#define STVID_TASK_PRIORITY_ERROR_RECOVERY  MED_DRIVER_PRIORITY
#endif /* not STVID_STVAPI_ARCHITECTURE */
#endif /* LINUX */
#endif /* not STVID_TASK_PRIORITY_ERROR_RECOVERY */

/* trick mode task priority */
#ifndef STVID_TASK_PRIORITY_SPEED
#ifdef ST_OSLINUX
#define STVID_TASK_PRIORITY_SPEED           STVID_THREAD_SPEED_PRIORITY
#else
#define STVID_TASK_PRIORITY_SPEED           4
#endif /* LINUX */
#endif /* not STVID_TASK_PRIORITY_SPEED */

#ifndef STVID_TASK_PRIORITY_TRICKMODE
#ifdef ST_OSLINUX
#define STVID_TASK_PRIORITY_TRICKMODE       STVID_THREAD_TRICKMODE_PRIORITY
#else
#define STVID_TASK_PRIORITY_TRICKMODE       4
#endif /* LINUX */
#endif /* not STVID_TASK_PRIORITY_TRICKMODE */

/* inject task priority */
#ifndef STVID_TASK_PRIORITY_INJECTERS
#ifdef ST_OSLINUX
#define STVID_TASK_PRIORITY_INJECTERS       STVID_THREAD_INJECT_ES_PRIORITY
#else
#define STVID_TASK_PRIORITY_INJECTERS       2
#endif /* LINUX */
#endif /* not STVID_TASK_PRIORITY_INJECTERS */

#if defined ST_OS21 || defined ST_OSLINUX
    #ifndef STVID_TASK_STACK_SIZE_DECODE
    #define STVID_TASK_STACK_SIZE_DECODE            16384
    #endif
    #ifndef STVID_TASK_STACK_SIZE_DISPLAY
    #define STVID_TASK_STACK_SIZE_DISPLAY           16384
    #endif
    #ifndef STVID_TASK_STACK_SIZE_ERROR_RECOVERY
    #define STVID_TASK_STACK_SIZE_ERROR_RECOVERY    16384
    #endif
    #ifndef STVID_TASK_STACK_SIZE_TRICKMODE
    #define STVID_TASK_STACK_SIZE_TRICKMODE         16384
    #endif
    #ifndef STVID_TASK_STACK_SIZE_SPEED
    #define STVID_TASK_STACK_SIZE_SPEED             16384
    #endif
    #ifndef STVID_TASK_STACK_SIZE_INJECTERS
    #define STVID_TASK_STACK_SIZE_INJECTERS         16384
    #endif
#endif /* ST_OS21 */
#ifdef ST_OS20
    #ifndef STVID_TASK_STACK_SIZE_DECODE
    #define STVID_TASK_STACK_SIZE_DECODE            8192
    #endif
    #ifndef STVID_TASK_STACK_SIZE_DISPLAY
    #define STVID_TASK_STACK_SIZE_DISPLAY           8192
    #endif
    #ifndef STVID_TASK_STACK_SIZE_ERROR_RECOVERY
    #define STVID_TASK_STACK_SIZE_ERROR_RECOVERY    4096
    #endif
    #ifndef STVID_TASK_STACK_SIZE_TRICKMODE
    #define STVID_TASK_STACK_SIZE_TRICKMODE         4096
    #endif
    #ifndef STVID_TASK_STACK_SIZE_SPEED
    #define STVID_TASK_STACK_SIZE_SPEED             4096
    #endif
    #ifndef STVID_TASK_STACK_SIZE_INJECTERS
    #define STVID_TASK_STACK_SIZE_INJECTERS         4096
    #endif
#endif /* ST_OS20 */

/* Duration of one second field in 90kHz units, i.e. PTS/PCR clock unit */
#define STVID_MPEG_CLOCKS_ONE_SECOND_DURATION 90000

/* conversion Factor to compute a duration in units of 90 Khz a bit rate in uniots of 400 bits/s and */
/* a size in bytes units : duration = size * 8 * 90000 / ( bit rate * 400 ).  duration = size * 1800 / bit rate */
#define STVID_DURATION_COMPUTE_FACTOR         1800

/* BitBufferFreeSize / BitRateValue must be multiplicated by ( 90000 * 8 ) / 400 ( thus multiplicated by 1800) */

/* Average VSync duration between 50Hz and 60Hz, for the case no VTG is connected so VTGFrameRate is 0, but division by 0 should be avoided. */
#define STVID_DEFAULT_VSYNC_DURATION_MICROSECOND_WHEN_NO_VTG  18333

/* Limited values for VTG FrameRate, to avoid too bad behaviour in case of bad params given to the driver. Limits between 1Hz and 1000Hz */
#define STVID_MAX_VALID_VTG_FRAME_RATE      1000000
#define STVID_DEFAULT_VALID_VTG_FRAME_RATE  50000
#define STVID_MIN_VALID_VTG_FRAME_RATE      1000

/* Define max VSync duration for video */
#ifdef HARDWARE_EMULATOR_FREQ_RATIO
#define STVID_MAX_VSYNC_DURATION ((ST_GetClocksPerSecond() / (STVID_DEFAULT_VALID_VTG_FRAME_RATE / 1000))*HARDWARE_EMULATOR_FREQ_RATIO)
#else
#define STVID_MAX_VSYNC_DURATION (ST_GetClocksPerSecond() / (STVID_DEFAULT_VALID_VTG_FRAME_RATE / 1000))
#endif

#define STVID_DECODE_TASK_FREQUENCY_BY_VSYNC 5

#define VIDCOM_PAN_AND_SCAN_INDEX_FIRST             0
#define VIDCOM_PAN_AND_SCAN_INDEX_SECOND            1
#define VIDCOM_PAN_AND_SCAN_INDEX_REPEAT            2
#define VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN           3

/* The values below should be the max value over all codec's of the producer,
so that the generic type can be sure that there won't be more data that that. */
#if defined(ST_multicodec) /* See NOTE_1 at the top of the file */
/* Max for codecs : H264, MPEG-2 */
#define VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES     16
#else /* ST_multicodec */
/* Standard MPEG-2 */
#define VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES     2
#endif /* not ST_multicodec */

#define VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS     (2 * VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES)

/* The 3 values below should define the MAX among all codecs compiled */
#if defined(ST_multicodec)
/* Max for codecs : H264, MPEG-2 */
#define VIDCOM_MAX_GLOBAL_DECODING_CONTEXT_SPECIFIC_DATA_SIZE   (1940)
#define VIDCOM_MAX_PICTURE_DECODING_CONTEXT_SPECIFIC_DATA_SIZE  (1020)
#define VIDCOM_MAX_PICTURE_SPECIFIC_DATA_SIZE                   (1100)
#else /* ST_multicodec */
/* Max when no codec at all */
#define VIDCOM_MAX_GLOBAL_DECODING_CONTEXT_SPECIFIC_DATA_SIZE   (4) /* Not needed, but set 4 just to be not 0... */
#define VIDCOM_MAX_PICTURE_DECODING_CONTEXT_SPECIFIC_DATA_SIZE  (4) /* Not needed, but set 4 just to be not 0... */
#define VIDCOM_MAX_PICTURE_SPECIFIC_DATA_SIZE                   (4) /* Not needed, but set 4 just to be not 0... */
#endif /* not ST_multicodec */

/* The max bitrate values for MPEG */
#define VIDCOM_MAX_BITRATE_MPEG_SD  37500  /* 15 Mbits/s */
#define VIDCOM_MAX_BITRATE_MPEG_HD  200000 /* 80 Mbits/s */

/* The min framerate value for MPEG */
#define VIDCOM_MIN_FRAME_RATE_MPEG  25 /* 25 pict/s */

/* The default hardware MPEG decoder speed */
#define VIDCOM_DECODER_SPEED_MPEG 3 /* 3 times normal speed */

/* List of Module ID for Take/Release */
typedef enum VIDCOM_MODULE_ID_e
{
    VIDCOM_APPLICATION_MODULE_BASE, /* 0 */
    VIDCOM_VIDORDQUEUE_MODULE_BASE, /* 1 */
    VIDCOM_VIDDECO_MODULE_BASE,     /* 2 */
    VIDCOM_VIDDEC_REF_MODULE_BASE,  /* 3 */
    VIDCOM_VIDBUFF_MODULE_BASE,     /* 4 */
    VIDCOM_VIDDEC_MODULE_BASE,      /* 5 */
    VIDCOM_VIDDISP_MODULE_BASE,     /* 6 */
    VIDCOM_VIDSPEED_MODULE_BASE,    /* 7 */
    VIDCOM_VIDTRICK_MODULE_BASE,    /* 8 */
    VIDCOM_VIDPROD_MODULE_BASE,     /* 9 */
    VIDCOM_VIDQUEUE_MODULE_BASE,    /* 10 */
    VIDCOM_DEI_MODULE_BASE,         /* 11 */
    VIDCOM_XCODE_MODULE_BASE,       /* 12 */
    VIDCOM_MAX_MODULE_NBR           /* Should be the last one!*/
} VIDCOM_MODULE_ID_t;

typedef enum VIDCOM_ErrorLevel_e
{
    VIDCOM_ERROR_LEVEL_NONE        = 0,    /* No error */
    VIDCOM_ERROR_LEVEL_INFORMATION = 1,    /* Minor error, for information only */
    VIDCOM_ERROR_LEVEL_BAD_DISPLAY = 2,    /* Small error visible only at display level */
    VIDCOM_ERROR_LEVEL_CRASH       = 4,    /* Important error with consequence on the hardware engine */
    VIDCOM_ERROR_LEVEL_RESET       = 8     /* Fatal error requiring a reset of the hardware engine */
} VIDCOM_ErrorLevel_t;

typedef enum VIDBUFF_AllocationMode_e
{
    VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY,
    VIDBUFF_ALLOCATION_MODE_CPU_PARTITION
} VIDBUFF_AllocationMode_t;

typedef enum VIDBUFF_FrameBufferType_e
{
    VIDBUFF_UNKNOWN_FRAME_BUFFER_TYPE,
    VIDBUFF_PRIMARY_FRAME_BUFFER,
    VIDBUFF_SECONDARY_FRAME_BUFFER,
    VIDBUFF_EXTRA_PRIMARY_FRAME_BUFFER,
    VIDBUFF_EXTRA_SECONDARY_FRAME_BUFFER
} VIDBUFF_FrameBufferType_t;

/* Defines which reconstruction mode is available for each frame buffer (primary or secondary) */
typedef enum VIDBUFF_AvailableReconstructionMode_e
{                                       /*                                            NORMAL    COMPRESSED  DECIMATED   */
    VIDBUFF_NONE_RECONSTRUCTION,        /*                                              N           N           N       */
    VIDBUFF_MAIN_RECONSTRUCTION,        /* Only primary frame buffers are available     Y           Y           N       */
    VIDBUFF_SECONDARY_RECONSTRUCTION,    /* Only secondary frame buffers are available   N           N           Y       */
    VIDBUFF_BOTH_RECONSTRUCTION          /* Primary and secondary frame buffers are available Y       Y           Y      */
} VIDBUFF_AvailableReconstructionMode_t;

typedef enum VIDBUFF_BufferType_e
{
    VIDBUFF_BUFFER_SIMPLE,                  /* Size is unique, in bytes. To be used for "one-space" buffers like bit buffers. */
    VIDBUFF_BUFFER_FRAME_12BITS_PER_PIXEL,  /* 12 bits per pixel : e.g. - YCbCr420 (1 byte for luma and 0.5 byte for chroma). */
    VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL   /* 16 bits per pixel : e.g. - YCbCr422 (1 byte for luma and 1 byte for chroma).   */
} VIDBUFF_BufferType_t;                     /*                          - YCbCr420 HD-PIP.                                    */

typedef enum VIDBUFF_DisplayStatus_e
{
    VIDBUFF_NOT_ON_DISPLAY,
    VIDBUFF_ON_DISPLAY,
    VIDBUFF_LAST_FIELD_ON_DISPLAY
} VIDBUFF_DisplayStatus_t;

typedef enum VIDBUFF_PictureDecodingStatus_e
{
    VIDBUFF_PICTURE_BUFFER_EMPTY,
    VIDBUFF_PICTURE_BUFFER_PARSED,
    VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT,
    VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER,
    VIDBUFF_PICTURE_BUFFER_DECODING,
    VIDBUFF_PICTURE_BUFFER_DECODED,
    VIDBUFF_PICTURE_BUFFER_TO_BE_FREED
} VIDBUFF_PictureDecodingStatus_t;

/* Exported Types ----------------------------------------------------------- */

typedef struct VIDCOM_Interrupt_s {
    U32  Mask;          /* Always contains the value of the interrupt mask, avoids reading it everytime */
    U32  Status;
    BOOL InstallHandler; /* FALSE when using interrupt events, TRUE when installing handler inside video */
    U32  Level;     /* Valid if InstallHandler is TRUE */
    U32  Number;    /* Valid if InstallHandler is TRUE */
    STEVT_EventID_t EventID; /* Valid if InstallHandler is TRUE */
} VIDCOM_Interrupt_t;

typedef struct VIDCOM_Task_s {
    task_t * Task_p;
    tdesc_t   TaskDesc;
    void *    TaskStack;
    BOOL    IsRunning;          /* TRUE if task is running, FALSE otherwise */
    BOOL    ToBeDeleted;        /* Set TRUE to ask task to end in order to delete it */
} VIDCOM_Task_t;

/* Memory profile used internaly in the driver */
typedef struct VIDCOM_InternalProfile_s
{
    /* all exported fields */
    STVID_MemoryProfile_t      ApiProfile;
    /* added field : */
     /* = nb frame store is set at init and may be patched througth api after */
     /* the init                      */
    U32                        NbMainFrameStore;
    U32                        NbSecondaryFrameStore;
    STVID_DecimationFactor_t   ApplicableDecimationFactor;
} VIDCOM_InternalProfile_t;

/* Generic standard independant objects */

/** This structure is related to the internal structure FrameCropInPixel defined in stvid.h ,
 * so any modification done here also should be made  in stvid.h **/
typedef struct VIDCOM_FrameCropInPixel_s
{
    U32     LeftOffset;
    U32     RightOffset;
    U32     TopOffset;
    U32     BottomOffset;
} VIDCOM_FrameCropInPixel_t;

typedef struct VIDCOM_GlobalDecodingContextGenericData_s
{
    STVID_SequenceInfo_t                SequenceInfo;
    /* Notes about particular fields of SequenceInfo:
    SequenceInfo.ScanType : Progressive means progressive, Interlaced means that it is determined picture per picture (Progressive/Interlaced)
    BitRate: in H264, max(Nal_BitRate)
    VBVBufferSize : in H264, BitRate * max(nal_initial_cpb_removal_delay) * UnitConvertionRatio (to be checked) */
    U8          ColourPrimaries;
    U8          TransferCharacteristics;
    U8          MatrixCoefficients;
    VIDCOM_FrameCropInPixel_t   FrameCropInPixel;
    U8                          NumberOfReferenceFrames;
    U8                          MaxDecFrameBuffering;   /* Number of frame buffer needed for decode and reordering */

    /* Error Level information */
    VIDCOM_ErrorLevel_t ParsingError;
} VIDCOM_GlobalDecodingContextGenericData_t;

typedef struct VIDCOM_GlobalDecodingContext_s
{
    VIDCOM_GlobalDecodingContextGenericData_t   GlobalDecodingContextGenericData;
    U32                                         SizeOfGlobalDecodingContextSpecificDataInByte;
    void *                                      GlobalDecodingContextSpecificData_p;
} VIDCOM_GlobalDecodingContext_t;

typedef struct VIDCOM_PictureDecodingContext_s
{
    VIDCOM_GlobalDecodingContext_t *    GlobalDecodingContext_p;
    U32                                 SizeOfPictureDecodingContextSpecificDataInByte;
    void *                              PictureDecodingContextSpecificData_p;
} VIDCOM_PictureDecodingContext_t;

typedef struct VIDCOM_PictureID_s
{
    S32     ID;             /* LSB for PictureID */
    U32     IDExtension;    /* MSB for PictureID, incremented at each "reset" of ID */
} VIDCOM_PictureID_t;

/** This structure is related to the internal structure PanAndScanIn16thPixel defined in stvid.h ,
 * so any modification done here  also should be made  in stvid.h **/
typedef struct VIDCOM_PanAndScanIn16thPixel_s
{
    S32     FrameCentreHorizontalOffset;
    S32     FrameCentreVerticalOffset;
    U32     DisplayHorizontalSize;
    U32     DisplayVerticalSize;
    BOOL    HasDisplaySizeRecommendation;
} VIDCOM_PanAndScanIn16thPixel_t;

typedef struct VIDCOM_PictureGenericData_s
{
    STVID_PictureInfos_t                PictureInfos;
    BOOL                                RepeatFirstField;
    U8                                  RepeatProgressiveCounter;
    VIDCOM_PanAndScanIn16thPixel_t      PanAndScanIn16thPixel[VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN];
    U8                                                     NumberOfPanAndScan;
    U32                                                   PanScanRectRepetitionPeriod;
    S32                                                   PanScanRectPersistence;
    U32     DecodingOrderFrameID;         /* Same for two associated fields */
    VIDCOM_PictureID_t                  ExtendedPresentationOrderPictureID; /* Continuously increasing in presentation order, */
                                                                            /* even over sequences Different for each picture, even fields */
    BOOL                                IsExtendedPresentationOrderIDValid; /* This flag is set by the parser if it can assign presentation ID for the picture */
    BOOL                                IsDisplayBoundPictureIDValid; /* When TRUE, the producer can present pictures until the DisplayBoundPictureID below */
    VIDCOM_PictureID_t                  DisplayBoundPictureID;

    BOOL                                IsFirstOfTwoFields;
    BOOL                                PreviousPictureHasAMissingField;
    U32                                 MissingFieldPictureDecodingOrderFrameID;
    BOOL    IsReference; /* needed by controler */

#if defined(ST_multicodec) /* See NOTE_1 at the top of the file */
    U32     FullReferenceFrameList[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES]; /* List off all reference frames. */
                                      /* Contains DecodingOrderFrameID in fixed
                                       * index position all along their life */
    BOOL    IsValidIndexInReferenceFrame[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES]; /* TRUE if corresponding index in
                                           * FullReferenceFrameList is allocated */
    BOOL    IsBrokenLinkIndexInReferenceFrame[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES]; /* TRUE if corresponding index in
            * FullReferenceFrameList is not the real reference for this picture according to the stream context */
    U32     UsedSizeInReferenceListP0;                                 /* Number of valid entries in ReferenceListP0 */
    U32     ReferenceListP0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS]; /* Contains DecodingOrderFrameID's */
    U32     UsedSizeInReferenceListB0;                                 /* Number of valid entries in ReferenceListB0 */
    U32     ReferenceListB0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS]; /* Contains DecodingOrderFrameID's */
    U32     UsedSizeInReferenceListB1;                                 /* Number of valid entries in ReferenceListB1 */
    U32     ReferenceListB1[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS]; /* Contains DecodingOrderFrameID's */
    void *  BitBufferPictureStartAddress;
    void *  BitBufferPictureStopAddress;
    struct
    {
        BOOL    FlushPresentationQueue;
        BOOL    FreezePresentationOnThisFrame;
        BOOL    ResumePresentationOnThisFrame;
    } AsynchronousCommands;
#endif /* ST_multicodec */

    STOS_Clock_t  DecodeStartTime;
    BOOL  IsDecodeStartTimeValid;
    STOS_Clock_t  PresentationStartTime;
    BOOL  IsPresentationStartTimeValid;

    U32 NbFieldsDisplay;
    U32 DiffPSC;

    /* Error information */
    VIDCOM_ErrorLevel_t ParsingError;

} VIDCOM_PictureGenericData_t;

#ifdef ST_XVP_ENABLE_FGT
typedef struct VIDCOM_FilmGrainSpecificData_s
{
  U32         FgtMessageId;
  BOOL        IsFilmGrainEnabled;
  U8          blending_mode_id;
  U8          log2_scale_factor;
  BOOL        comp_model_present_flag[3];
  U8          num_intensity_intervals_minus1[3];
  U8          intensity_interval_lower_bound[3][256];
  U8          intensity_interval_upper_bound[3][256];
  U8          num_model_values_minus1[3];
  U8          comp_model_value[3][256][3];
  U32         film_grain_characteristics_repetition_period;
  S32         picture_offset;
}VIDCOM_FilmGrainSpecificData_t;
#endif /* ST_XVP_ENABLE_FGT */

typedef struct VIDCOM_ParsedPictureInformation_s
{
    VIDCOM_PictureDecodingContext_t *   PictureDecodingContext_p;
    VIDCOM_PictureGenericData_t PictureGenericData;
    U32                                 SizeOfPictureSpecificDataInByte;
    void *                              PictureSpecificData_p;
#ifdef ST_XVP_ENABLE_FLEXVP
    VIDCOM_FilmGrainSpecificData_t      PictureFilmGrainSpecificData;
#endif /* ST_XVP_ENABLE_FLEXVP */
} VIDCOM_ParsedPictureInformation_t;


/* Parameters required to know everything about a buffer allocation (frame buffer or bit buffer) */
typedef struct VIDBUFF_BufferAllocationParams_s
{
    STAVMEM_PartitionHandle_t PartitionHandle;  /* Partition in which this buffer has been allocated */
    VIDBUFF_BufferType_t BufferType;    /* Buffer type */
    void *  Address_p;  /* Allocated address of the total of the buffer */
    U32     TotalSize;  /* Total size of the buffer in bytes. */
    void *  Address2_p; /* Used for frame buffers: address of chroma data inside buffer */
    U32     Size2;      /* Used for frame buffers: size of the chroma in bytes. Luma size is exactly (Size - Size2 - Size3) */
#ifdef ST_XVP_ENABLE_FGT
    void *  FGTLumaAccumBuffer_p;       /* Used for Film grain accum buffers: address of luma data*/
    U32     SizeFGTLumaAccumBuffer;     /* Used for Film grain accum buffers: size of the luma data in bytes.*/
    void *  FGTChromaAccumBuffer_p;     /* Used for Film grain accum buffers: address of chroma data*/
    U32     SizeFGTChromaAccumBuffer;   /* Used for Film grain accum buffers: size of the chroma data in bytes.*/
#endif /* ST_XVP_ENABLE_FGT */
    VIDBUFF_AllocationMode_t AllocationMode;
    STAVMEM_BlockHandle_t AvmemBlockHandle; /* Used if AllocationMode is SHARED_MEMORY */
} VIDBUFF_BufferAllocationParams_t;

typedef struct VIDBUFF_FrameBuffer_s
{
    VIDBUFF_BufferAllocationParams_t Allocation;
    BOOL    AllocationIsDone;                   /* TRUE if buffer is allocated, FALSE otherwise */
    BOOL    ToBeKilledAsSoonAsPossible;         /* TRUE when the frame has a wrong memory profile, and has to be killed
                                                 * (MemoryDeallocated, and removed from AllocatedFrameBuffersLoop_p) when
                                                 * it's removed from display Queue */
    STVID_CompressionLevel_t CompressionLevel;  /* Level of compression the primary frame buffer (main use). */
                                                /* Only available with omega2 cells. */
    STVID_DecimationFactor_t DecimationFactor;  /* Decimation factor used for the secondary frame buffers. */
                                                /* Only available with omega2 cells. */
    BOOL                        AdvancedHDecimation;   /* TRUE : When Advanced Decimation is used .*/
    VIDBUFF_AvailableReconstructionMode_t AvailableReconstructionMode;   /* This field indicate which reconstruction mode is available for frame buffer. */
    VIDBUFF_FrameBufferType_t       FrameBufferType; /* Variable indicating if this frame buffer belongs to the Primary, Secondary or Extra Frame Buffer Pool */
    struct VIDBUFF_FrameBuffer_s *   NextAllocated_p;   /* Pointer to a loop of allocated frames (all pointing to another and the last pointing back to the first) */
    struct VIDBUFF_PictureBuffer_s * FrameOrFirstFieldPicture_p; /* Pointer to a picture when the frame is used: either a frame picture or the first field picture */
    struct VIDBUFF_PictureBuffer_s * NothingOrSecondFieldPicture_p; /* NULL if FrameOrFirstField_p points to a frame picture or to NULL, pointer to the second field picture otherwise */
    BOOL HasDecodeToBeDoneWhileDisplaying; /* TRUE if the decode to come in this frame buffer has to be synchronized on the display of the previous decoded picture in this frame buffer. */
} VIDBUFF_FrameBuffer_t;

typedef struct VIDBUFF_AdditionnalData_s
{
    STAVMEM_PartitionHandle_t AvmemPartitionHandleForAdditionnalData;
    STAVMEM_BlockHandle_t AvmemBlockHandle;
    void *  Address_p;      /* Allocated address of the buffer */
    U32     Size;           /* Size of the buffer in bytes. */
    U8      FieldCounter;   /* To handle field pair attached to a same AdditionnalData buffer, last field (FieldCounter == 0) actually frees the buffer */
} VIDBUFF_AdditionnalData_t;

/* Take Release counter specific to one module */
typedef struct VIDBUFF_ModuleCounter_s
{
	U32     TakeReleaseCounter;
} VIDBUFF_ModuleCounter_t;

typedef struct VIDBUFF_PictureBuffer_s
{
    VIDCOM_ParsedPictureInformation_t   ParsedPictureInformation;
    U8                                  PictureSpecificData[VIDCOM_MAX_PICTURE_SPECIFIC_DATA_SIZE];
    VIDCOM_PictureDecodingContext_t     PictureDecodingContext;
    U8                                  PictureDecodingContextSpecificData[VIDCOM_MAX_PICTURE_DECODING_CONTEXT_SPECIFIC_DATA_SIZE];
    VIDCOM_GlobalDecodingContext_t      GlobalDecodingContext;
    U8                                  GlobalDecodingContextSpecificData[VIDCOM_MAX_GLOBAL_DECODING_CONTEXT_SPECIFIC_DATA_SIZE];
#if defined(ST_multicodec) /* See NOTE_1 at the top of the file */
    BOOL                                IsReferenceForDecoding;
    BOOL                                IsNeededForPresentation;
    BOOL                                IsPushed;
#endif /* ST_multicodec */
    VIDBUFF_FrameBuffer_t *             FrameBuffer_p;          /* Frame buffer the picture is stored into */
    VIDBUFF_AdditionnalData_t           PPB;                    /* Additionnal data buffer (aka Picture Parameters Buffer) */

    struct
    {
        BOOL                            IsInUse;                /* Boolean indicating if this PictureBuffer is in use */
        VIDBUFF_DisplayStatus_t         DisplayStatus;
        U32                             PictureLockCounter;     /* Global Picture Lock Counter. Indicates if the Picture Buffer is available or not. */
        BOOL                            IsInDisplayQueue;
        BOOL                            IsLinkToFrameBufferToBeRemoved;
        VIDBUFF_ModuleCounter_t         Module[VIDCOM_MAX_MODULE_NBR];
        semaphore_t *                   LockCounterSemaphore_p;   /* Semaphore protecting access to the Picture Lock counters */
    } Buffers;

#if defined(ST_multicodec) /* See NOTE_1 at the top of the file */
    /* One table, which make a matrix for conversion from DecodeOrderFrameID to Picture pointers.
    Usage: find the index in PictureGenericData.FullReferenceFrameList, then take picture pointer in ReferencesFrameBufferPointer. */
    VIDBUFF_FrameBuffer_t *     ReferencesFrameBufferPointer[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES];
    /* Worst error information found among all reference pictures */
    /* ReferenceDecodingError will give helpfull information about the current picture quality for the producer and display module error recovery */
    VIDCOM_ErrorLevel_t ReferenceDecodingError; /* Set by the producer when it updates the references before to launch the decode */
#endif /* ST_multicodec */

    struct
    {
        VIDBUFF_PictureDecodingStatus_t  PictureDecodingStatus;
        U32                             CommandId;
        /* processing time information for decode of this picture */
        STOS_Clock_t                       StartTime;
        STOS_Clock_t                       ConfirmEVTTime;
        STOS_Clock_t                       BufferSeachStartTime;
        STOS_Clock_t                       ConfirmedTime;
        STOS_Clock_t                       DecodeStartTime;
        STOS_Clock_t                       JobCompletedTime;
#ifdef STVID_VALID_MEASURE_TIMING
        U32                             LXDecodeTimeInMicros;
#ifdef STVID_VALID_MULTICOM_PROFILING
        STOS_Clock_t                    HostSideLXStartTime;
        STOS_Clock_t                    HostSideLXCompletedTime;
        U32                             LXSideLXDecodeTime;
#endif /* STVID_VALID_MULTICOM_PROFILING */
#endif /* STVID_VALID_MEASURE_TIMING */
        /* Error information about the decode of this picture */
        VIDCOM_ErrorLevel_t DecodingError;
    } Decode;
    struct VIDBUFF_PictureBuffer_s * AssociatedDecimatedPicture_p; /* pointer to decimated picture / non decimated or null */
#if defined ST_smoothbackward || defined STVID_TRICKMODE_BACKWARD
    void * SmoothBackwardPictureInfo_p;
#endif /* ST_smoothbackward */

    U32 PictureNumber;
} VIDBUFF_PictureBuffer_t;


/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_COMON_DEFINITIONS_H */

/* End of vid_com.h */
