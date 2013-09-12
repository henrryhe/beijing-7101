/*******************************************************************************

File name   : avsparser.h

Description : AVS parser data structures and functions prototypes

COPYRIGHT (C) STMicroelectronics 2008

Date               Modification                                     Name
----               ------------                                     ----
29 January 2008    First STAPI version                               PLE
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __AVSPARSER_H
#define __AVSPARSER_H


/* Glossary ----------------------------------------------------------------- */
/*
 * CFP: Complementary Field Pair
 * DPBRef: the array that stores the reference frames, CFP or non-paired fields
 * GDC: Global Decoding Context (a term to speak about SH)
 * PDC: Picture Decoding Context (a term to speak about EPH)
 * POC: Picture Order Count
 * PPDO: Previous Picture in Decoding Order (some parameters need to be stored for POC computation
 */

/* Includes ----------------------------------------------------------------- */
#include "vcodavs.h"

#include "stfdma.h"
#include "inject.h"

#include "stos.h"

#include "sttbx.h"
#include "stsys.h"

#include "vid_ctcm.h"


/* Constants ------------------------------------------------------- */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
#define TRACE_AVSPAR_COLOR (TRC_ATTR_BACKGND_BLACK | TRC_ATTR_FORGND_CYAN | TRC_ATTR_BRIGHT)

#define AVSPAR_TRACE_BLOCKID   (0x5115)
#define AVSPAR_TRACE_BLOCKNAME "AVSPARSER"

enum
{
    AVSPAR_TRACE_GLOBAL = 0,
    AVSPAR_TRACE_INIT_TERM,
    AVSPAR_TRACE_PARSESEQ,
    AVSPAR_TRACE_PARSEVO,
    AVSPAR_TRACE_PARSEVOL,
    AVSPAR_TRACE_PARSEVOP,
    AVSPAR_TRACE_PARSEGOV,
    AVSPAR_TRACE_PARSESVH,
    AVSPAR_TRACE_PARSEVST,
    AVSPAR_TRACE_PARSEVPH,
    AVSPAR_TRACE_PICTURE_PARAMS,
    AVSPAR_TRACE_PICTURE_INFO_COMPILE
};

#endif /* defined(TRACE_UART) && defined(VALID_TOOLS)  */



/* AVS_PARSER_PRINT  is used to print messages that might be useful
   for debugging the parser, but otherwise should not be used. */
#ifdef AVS_DEBUG_PARSER
#define AVS_PARSER_PRINT STTBX_Print
#else
#define AVS_PARSER_PRINT
#endif /* not AVS_DEBUG_PARSER */


/* This is for testing the parser without having a
 changed interface for display order ID.
 Since Display order ID cannot be known
 by the parser when a reference frame is parsed,
 it should wait until the next reference picture
 before this information is known.  But the
 parser interface has not yet been changed for this,
 so, a temporary method to assume that the number
 of B frames will not be more than a certain value
 and add that value to the Display order ID every
 time a reference picture occurs. */
#define AVS_TEST_DISPLAY_ORDER_ID 1

#define EMULATION_PREVENTION_BYTE 0x03
/*! the reverse emulation prevention byte (bit reverse) for detection on bit reversed streams */
#define ETYB_NOITNEVERP_NOITALUME 0xC0

#ifdef AVS_TEST_DISPLAY_ORDER_ID
/* no reason for this number. */
#define MAX_B_FRAMES  20
#endif



/* @@@ This is temporarily placed here for compilation  purposes until the definition is included in the FDMA */
#define STFDMA_DEVICE_AVS_RANGE_0 32

/* A start code is the fixed pattern 0x00 00 01, length = 3 bytes */
#define AVS_START_CODE_PREFIX_LENGTH 3

/* For Video Format */
#define UNSPECIFIED_VIDEO_FORMAT 5

/* For Colour Primaries */
#define UNSPECIFIED_COLOUR_PRIMARIES 2

/* For Transfer Characteristics */
#define UNSPECIFIED_TRANSFER_CHARACTERISTICS 2

#define PRIMARY_PIC_TYPE_UNKNOWN 255

/* For Matrix Coefficients */
/* Definition of matrix_coefficients values: same as mpeg2 */
#define MATRIX_COEFFICIENTS_ITU_R_BT_709        1
#define MATRIX_COEFFICIENTS_UNSPECIFIED         2
#define MATRIX_COEFFICIENTS_FCC                 4
#define MATRIX_COEFFICIENTS_ITU_R_BT_470_2_BG   5
#define MATRIX_COEFFICIENTS_SMPTE_170M          6
#define MATRIX_COEFFICIENTS_SMPTE_240M          7

/* For FDMA SC+PTS entries */
/* FDMA_SCPTS_MASK bits are set to 1 for the length of SC/PTS in word0 */
#define FDMA_SCPTS_MASK 0x3
/* right shift of next constant must be done to get the parameter from word0 */
#define FDMA_SCPTS_SHIFT 0x0

/* if the entry masked by FDMA_SCPTS_MASK + SHIFT is FDMA_SC_PATTERN, it is a start code */
#define FDMA_SC_PATTERN 0x00
/* if the entry masked by FDMA_SCPTS_MASK + SHIFT is FDMA_PTS_PATTERN, it is a start code */
#define FDMA_PTS_PATTERN 0x01

/* Shortcuts to VBS start code type */
#define FDMA_UNIT_TYPE_MASK 0xFF
#define FDMA_UNIT_TYPE_SHIFT 0x0
#define GetUnitType(StartCode) ((StartCode >> FDMA_UNIT_TYPE_SHIFT) & FDMA_UNIT_TYPE_MASK)

/* AVS Start Codes*/

#define SLICE_START_CODE_MIN                0x00
#define SLICE_OBJECT_START_CODE_MAX         0xAF

#define VIDEO_SEQUENCE_START_CODE           0xB0
#define VIDEO_SEQUENCE_END_CODE             0xB1
#define USER_DATA_START_CODE                0xB2
#define I_PICTURE_START_CODE                0xB3
#define EXTENSION_START_CODE                0xB5
#define PB_PICTURE_START_CODE               0xB6
#define VIDEO_EDIT_START_CODE               0xB7



#define EXTENDED_PAR            0x0F    /* extended pixel aspect ratio*/

/* IMG IDs */
#define I_IMG                   0
#define P_IMG                   1
#define B_IMG                   2

#define RESYNC_MARKER           1

/* Default values */
#define AVS_DEFAULT_NON_RELEVANT            0   /* for non relevant parameters in the STVID API */
#define AVS_DEFAULT_FRAME_RATE              25000   /* 25 frame/s */
#define AVS_DEFAULT_BIT_RATE                    (8000*1024) /* Bitrate for Advanced Simple Profile/Level 5 */
#define AVS_DEFAULT_VBVMAX                     2105344  /* Maximum VBV Buffer Size in bytes for Advanced Simple Profile/Level 5  */
#define AVS_DEFAULT_PROFILE_AND_LEVEL       0xF5    /* Default to Advanced Simple Profile/Level 5 */
#define AVS_DEFAULT_COLOR_TYPE              STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420 /* TODO: find right value */
#define AVS_DEFAULT_BITMAP_TYPE             STGXOBJ_BITMAP_TYPE_MB /* TODO: find right value */
#define AVS_DEFAULT_PREMULTIPLIED_COLOR FALSE /* TODO: find right value */
#define AVS_DEFAULT_SUBBYTEFORMAT           STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB /* TODO: find right value */
#define AVS_DEFAULT_SCANTYPE                    STGXOBJ_INTERLACED_SCAN /* TODO: find right value */
#define AVS_DEFAULT_BITMAPPARAMS_OFFSET 0
#define AVS_DEFAULT_BIGNOTLITTLE                FALSE /* Little endian bitmap. TODO: find right value */

typedef enum
{
    PARSER_JOB_COMPLETED_EVT_ID,
    PARSER_STOP_EVT_ID,
        PARSER_PICTURE_SKIPPED_EVT_ID,
    AVSPARSER_NB_REGISTERED_EVENTS_IDS  /* Keep this one as the last one */
}AVSParserEventIDs;

/*! Parser state */
typedef enum
{
    PARSER_STATE_IDLE,
    PARSER_STATE_READY_TO_PARSE,
    PARSER_STATE_PARSING
} ParserState_t;

/*! Parser commands */
typedef enum
{
    PARSER_COMMAND_START,
    PARSER_COMMAND_START_CODE_TO_PROCESS
} ParserCommands_t;

typedef enum
{
    AVS_I_I_FIELD_PICTURE_TYPE,
    AVS_I_P_FIELD_PICTURE_TYPE,
    AVS_P_I_FIELD_PICTURE_TYPE,
    AVS_P_P_FIELD_PICTURE_TYPE,
    AVS_B_B_FIELD_PICTURE_TYPE,
    AVS_NUM_FIELD_PICTURE_TYPES
}AVS_FieldPictureType_t;

/* SC+PTS list size depends on the bit-buffer size (allocated at by the controller                               */
/* module at upper level)                                                                                        */
/* Assumption : 1 start code every 4 byte (absolute worst case: 3 bytes for SC, 1 byte for payload)              */
/*            : 1 entry = 4 bytes                                                                                */
/*            Hence: for stream @20Mbit/s, 1 second to store (start condition), gives 2.6 MByte for SCList Size! */
/* Alternative would be to store in advance the picture parameters as soon as they reach the Start Code list     */
/* TODO: arbitrate between the 2 solutions */
/* For the moment, the Start Code List Size is kept to a small value for debug */
#define SC_LIST_SIZE_IN_ENTRY /*2048*/ 32768
#define PTS_SCLIST_SIZE 10

/* PES Buffer Size (in bytes)                                                 */
/* (transfer periode) x (max bit rate) = 44Kb                                 */
/* 44Kb x 2 to bufferize enougth = 88Kb (here is defined 90Kb)                */
#define PES_BUFFER_SIZE ((90*1024) & 0xffffff80)   /* explicit the constraint */

/* Definition of PES/VBS start code values */
#define SMALLEST_VBS_PACKET_START_CODE 0x00
#define GREATEST_VBS_PACKET_START_CODE 0xFF
#define SMALLEST_VIDEO_PACKET_START_CODE 0xE0
#define GREATEST_VIDEO_PACKET_START_CODE 0xEF
#define DUMMY_RANGE 0x00

/* Parser Stack size */
#define AVSPARSER_TASK_STACK              1000

/* Max time waiting for an order: 5 times per field display (not less) */
#define MAX_WAIT_ORDER_TIME (STVID_MAX_VSYNC_DURATION / STVID_DECODE_TASK_FREQUENCY_BY_VSYNC)

/* Command buffers */
/* 10 commands sent by the controller is far enough, as no queue is allowed so far in the parser */
#define CONTROLLER_COMMANDS_BUFFER_SIZE 10

/* Shortcuts to Start Code entries */
#define GetPTS32(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->PTS.PTS0)))
#define GetPTS33(Entry)  ((STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->PTS.PTS1)) == 1) ? TRUE : FALSE)
/* The Entry->SC.Type field is a bitfield, so we can't get its address with the & operator */
#define IsPTS(Entry)     (((STSYS_ReadRegMemUncached32LE((U32*)(Entry)) & 3) != STFDMA_SC_ENTRY )? TRUE : FALSE)

#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
/* On STi7100 Entry->SC.SCValue is defined as a bitfield, so we can't get its address with the & operator */
#define GetSCVal(Entry)  (STSYS_ReadRegMemUncached8((U8*)((U32)(Entry) + 8)))
#else
#define GetSCVal(Entry)  (Entry->SC.SCValue)
#endif /* defined(ST_7100) || defined(ST_7109) */

#define GetSCAdd(Entry)  (U32)(Entry->SC.Addr)

#define MIN_USER_DATA_SIZE 15

/*! The maximum number of field between 2 flushes of DPBRef (IDR or MMCO=5) picture
 * Computed here as: 30 frames/s, flush every 5 seconds */
#define MAX_PIC_BETWEEN_DPBREF_FLUSH (30 * 2 * 5)

typedef struct AVSVideoSequenceHeader_s
{
    U8 ProfileID;
    U8 LevelID;
    U8 ProgressiveSequence;
    U8 ChromaFormat;
    U8 SmaplePrecision;
    U8 AspectRatioInfo;
    U8 FrameRateCode;
    U32 BitRate;
    U8 LowDelay;
    U32 VBVBufferSize;
    /*Sequence display extension*/
    U8 VideoFormat;
    U8 SampleRange;
    U8 ColorDescription;
    U8 ColorPrimaries;
    U8 TransferCharacteristics;
    U8 MatrixCoefficients;
    U16 DisplayHorizontalSize;
    U16 DisplayVerticalSize;
} AVSVideoSequenceHeader_t;

typedef struct AVSPictureHeader_s
{
    U16 VbvDelay;
    U8 PictureCodingType;
    U8 PictureDistance;
    U32 TimeCode;
    U16 VbvCheckTimes;
    U8 ProgressiveFrame;
    U8 PictureStructure;
    U8 AdvancedPredModeDisable;
    U8 TopFieldFirst;
    U8 RepeatFirstField;
    U8 FixedPictureQP;
    U8 PictureQP;
    U8 PictureReferenceFlag;
    U8 NoForwardReferenceFlag;
    U8 SkipModeFlag;
    U8 LoopFilterDisable;
    U8 LoopFilterParameterFlag;
    S32 AlphaCOffset;
    S32 BetaOffset;
    U16 PicWidth;
    U16 PicHeight;
} AVSPictureHeader_t;




/* Exported types ----------------------------------------------------------- */

/*! Byte stream structure */
typedef struct VBSByteStream_s
{
    U8 *StartByte_p; /*!< pointer to beginning of byte stream */
    U32 Len; /*!< length in bytes \n
              * a stream made of: 0x00 0x01 0x02 0x03 has a Len=4
              */
    S8 BitOffset; /*!< bit offset inside a byte \n
                   * BitOffset: 7= MSBit of byte, 0= LSBit of byte \n
                   * Init value: 7
                   */
    U32 ByteOffset; /*!< byte offset inside the stream: follows 32-bit word structure (in Word swapping: Offset = 3,2,1,0,7,6...*/
    U32 ByteCounter; /*!< 0 = beginning of stream. Is incremented on each new byte read. Used to detect end of stream */
    BOOL EndOfStreamFlag; /*!< TRUE when the end of stream is reached */
    U8 LastByte; /*!< the previous byte read from the byte stream: this is to ease EMULATION PREVENTION detection */
    U8 LastButOneByte; /*!< the byte before the previous byte: this is to ease EMULATION PREVENTION detection */
    BOOL ByteSwapFlag; /*!< if FALSE: bits are read from MSB to LSB . If TRUE: bits are read from LSB to MSB */
    BOOL WordSwapFlag; /*!< if FALSE: bytes are read from LSByte to MSByte (Little Endian) \n
                        * If TRUE: bytes are read from MSByte to LSByte
                        */

    /* About byte and word swapping */
    /* Assume a stream (Byte Stream Annex B) made of the following pattern in decoding order (left to right): */
    /*       0000 0001 0010 0011 0100 0101 0110 0111  */
    /* ie :  0    1    2    3    4    5    6    7     */

    /* if the system stores it like that: */
    /*           32:24     23:16     15:8     7:0       ByteSwapFlag   WordSwapFlag */
    /* A[0] =     67        45        23      01          False           False     */
    /* A[0] =     01        23        45      67          False           True      */
    /* A[0] =     E6        A2        C4      80          True            False     */
    /* A[0] =     80        C4        A2      E6          True            True      */

} ByteStream_t;


/*! Current parsed picture data structure. These parameters are not stored in the Codec API picture data structure */
typedef struct PictureLocalData_s
{
    S32 FrameNumOffset; /* copied in PreviousPictureInDecodingOrder once the picture is parsed */
    U32 frame_num;          /*!< stream element */
    U32 PrevRefFrameNum;    /*!< variable from the standard. This is not the same as
                                        * PreviousReferencePictureInDecodingOrder.frame_num at least for IDR picture */
   BOOL IsExisting;         /*!< FALSE when the frame is a non-existing frame */
    U32 Width;              /*!< the witdh in pixels */
    U32 Height;             /*!< the height in pixels */
    BOOL    IsRandomAccessPoint;
    BOOL    IsValidPicture;
    U8      PrimaryPicType; /*!< stream element */
    U8      NumRef;
    U8      RefField;

} PictureLocalData_t;

/*! SH data structure for POC computations. These parameters are not stored in the GDC */
typedef struct PrivateGlobalContextData_s
{
    BOOL GlobalContextValid;
    U8 Profile;
    U8 Level;
    U16 MaxWidth;
    U16 MaxHeight;
    /*BOOL PulldownFlag;
    BOOL InterlaceFlag;*/
    BOOL HRDParamFlag;


} PrivateGlobalContextData_t;

/*! EPH data structure. These parameters are not stored in the GDC */
typedef struct PrivatePictureContextData_s
{
    BOOL PictureContextValid;
    BOOL BrokenLinkFlag;
    BOOL ClosedEntryFlag;

} PrivatePictureContextData_t;

/*! Global variables handled by the parser that do not fit in GDC, PDC or Picture */
typedef struct StreamState_s
{
    VIDCOM_PictureID_t               BackwardRefPresentationOrderPictureID;
    VIDCOM_PictureID_t               CurrentPresentationOrderPictureID;
    BOOL                             WaitingRefPic;  /* True if there is a Reference picture that has been parsed, but the
                                                        presentation order is not known. */
    U8                               CurrentTfcntr; /* read from the stream */
    U8                               BackwardRefTfcntr;
    U8                               StreamID; /* copy from start parameters */
    /* VIDCOM_PanAndScanIn16thPixel_t PanAndScanIn16thPixel[VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN]; */
    /* U8                             NumberOfPanAndScan; */
    /* When the next picture comes in, It will be placed at this index in the FullReferenceFrameList */
    U8                                          NextReferencePictureIndex;
    PARSER_ErrorRecoveryMode_t       ErrorRecoveryMode; /* copy from GetPicture parameters */
    STGXOBJ_ScanType_t               ScanTypePreviousRefPicture;
    BOOL                             ForwardRangeredfrmFlagBFrames; /* The RangeredfrmFlag of the forward reference picture for B Frames  */
    BOOL                             ForwardRangeredfrmFlagPFrames; /* The RangeredfrmFlag of the forward reference picture for P Frames  */
    U8                               RespicBBackwardRefPicture;/* The RESPIC for the Backward Reference Picture of the next B frame */
    U8                               RespicBForwardRefPicture; /* The RESPIC for the Forward Reference Picture of the next B frame */
    U8                               RndCtrlRecentRefPicture;/* The RndCtrl from the most recent reference piture. Used for skip pictures and for simple and main profile*/
    STVID_MPEGFrame_t                SecondFieldPictureTypeGeneric;

    U16                              RefDistPreviousRefPic; /* Stream element */

} StreamState_t;

/*! Start Code + PTS buffer data structure */
typedef struct SCBuffer_s
{
    STAVMEM_BlockHandle_t   SCListHdl;
   STFDMA_SCEntry_t   *    SCList_Start_p;         /* sc list  buffer limit  */
   STFDMA_SCEntry_t   *    SCList_Stop_p;          /* sc list  buffer limit  */
   STFDMA_SCEntry_t   *    SCList_Loop_p;          /* sc list  buffer limit  */
   STFDMA_SCEntry_t   *    SCList_Write_p;         /* sc list  buffer write pointer  */
   STFDMA_SCEntry_t   *    NextSCEntry;            /* pointer in SC List    */
    STFDMA_SCEntry_t   *    SavedNextSCEntry_p;     /* saves the NextSCEntry before looking for next start code */
} SCBuffer_t;

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
/*! Bit Buffer data structure */
typedef struct BitBuffer_s
{
   U8 * ES_Start_p;             /* ES buffer limit: Bit buffer stands from ES_Start_p inclusive */
   U8 * ES_Stop_p;              /* ES buffer limit: Bit buffer stands to ES_Stop_p inclusive    */
   U8 * ES_Write_p;             /* write pointer (of FDMA) in ES buffer   */
    U8 * ES_ParserRead_p;        /* Read pointer (of parser) in ES buffer */
    U8 * ES_DecoderRead_p;       /* Read pointer (of decoder) in ES buffer */
#ifdef STVID_TRICKMODE_BACKWARD
    U8 * ES_StoreStart_p;             /* ES buffer limit: Bit buffer stands from ES_Start_p inclusive */
    U8 * ES_StoreStop_p;              /* ES buffer limit: Bit buffer stands to ES_Stop_p inclusive    */
#endif /* STVID_TRICKMODE_BACKWARD */
    /* To program the FDMA */
    U8   ES0RangeEnabled;
    U8   ES0RangeStart;
    U8   ES0RangeEnd;
    BOOL ES0OneShotEnabled;
    U8   ES1RangeEnabled;
    U8   ES1RangeStart;
    U8   ES1RangeEnd;
    BOOL ES1OneShotEnabled;
} BitBuffer_t;

/*! Bit Buffer data structure */
typedef struct PESBuffer_s
{
   STAVMEM_BlockHandle_t   PESBufferHdl;
   void *                  PESBufferBase_p;
   void *                  PESBufferTop_p;
    BOOL                    PESRangeEnabled; /*!< Enabled (true) when input is PES */
    U8                      PESSCRangeStart; /*!< the full 8 bit to use to configure the FDMA [7:5]=111 [4:0]=StreamID */
    U8                          PESSCRangeEnd;
    BOOL                    PESOneShotEnabled;
} PESBuffer_t;

/*! Injection structure */
typedef struct Inject_s
{
   VIDINJ_Handle_t InjecterHandle; /* Injecter instance handle */
    U32             InjectNum; /* Injection number */
} Inject_t;


/*! Parameters needed to store reference frames in the DPBRef */
typedef struct DPBFrameReference_s
{
    BOOL TopFieldIsReference; /*!< TRUE when the top field (or the frame) is short term or long term reference. */
    BOOL BottomFieldIsReference; /*!< TRUE when the bottom field (or the frame) is short term or long term reference. */
    /* for a frame: both TopFieldIsReference and BottomFieldIsReference are set and reset at the same time */
    /* when both TopFieldIsReference and BottomFieldIsReference are FALSE, the Frame Buffer is free */
    U32 DecodingOrderFrameID; /*!< the FrameID for the picture being stored in this position in the array */
   U32 frame_num; /*!< the stream element read when parsing: same for both fields of a frame */
    S32 FrameNumWrap; /*!< variable defined in standard */
   S32 TopFieldPicNum; /*!< the PicNum for the top field of the frame stored in this position */
   S32 BottomFieldPicNum; /*!< the PicNum for the bottom field of frame */
   U8  LongTermFrameIdx; /*!< variable defined in standard */
   U32 TopFieldLongTermPicNum; /*!< the LongTermPicNum for the top field of the frame stored in this position */
   U32 BottomFieldLongTermPicNum; /*!< the LongTermPicNum for the bottom field of the frame stored in this position */
    BOOL TopFieldIsLongTerm; /*!< TRUE if top field is a long term reference picture */
    BOOL BottomFieldIsLongTerm; /*!< TRUE if bottom field is a long term reference picture */
    /* For a frame: both TopFieldIsLongTerm and BottomFieldIsLongTerm are set and reset at the same time */
   S32 TopFieldOrderCnt; /*!< variable defined in standard */
   S32 BottomFieldOrderCnt; /*!< variable defined in standard */
   BOOL IsExisting; /*!< Set to FALSE when the frame is issued from the gaps in frame_num pre marking process */
    BOOL IsBrokenLink;
} DPBFrameReference_t;

/*! Start Code Found */
typedef struct
{
    BOOL IsInBitBuffer; /* TRUE when the VOP is fully stored in the bit buffer */
    BOOL IsPending; /* TRUE when the start code is caught by the parser but is not yet processed */
    U8 Value;
    U8 * StartAddress_p;
    U8 * StopAddress_p;
    BOOL SliceFound;/*if a Slice is Found then the end address of the picture is the start
                       address of the next field, frame, sequence header of entry-point header,
                       if a Slice is not found, and there is a SubBDU start Address, then the
                       end address of the picture is the start address of the SubBDU.*/
       U8 * SliceStartAddress_p;
} StartCode_t;

/*! Parameters to manage the parser task */
typedef struct ForTask_s
{
    osclock_t  MaxWaitOrderTime; /* time to wait to get a new command */
    BOOL ControllerCommand;
} ForTask_t;


/*! Parser top level data structure that contains all the data structure above. This is to ease function calls */
typedef struct AVSParserPrivateData_s
{
    VIDCOM_PictureGenericData_t                     PictureGenericData;
    AVS_PictureSpecificData_t                       PictureSpecificData;

    VIDCOM_GlobalDecodingContextGenericData_t       GlobalDecodingContextGenericData;
    AVS_GlobalDecodingContextSpecificData_t         GlobalDecodingContextSpecificData;

    AVS_PictureDecodingContextSpecificData_t        PictureDecodingContextSpecificData;


    PrivateGlobalContextData_t                      PrivateGlobalContextData;
    PrivatePictureContextData_t                     PrivatePictureContextData;
    PictureLocalData_t                              PictureLocalData;
    StreamState_t                                   StreamState;
    STEVT_EventID_t                                 RegisteredEventsID[AVSPARSER_NB_REGISTERED_EVENTS_IDS];
    SCBuffer_t                                      SCBuffer;
    BitBuffer_t                                     BitBuffer;
    PESBuffer_t                                     PESBuffer;
    Inject_t                                        Inject;
    BOOL                                            IntensityCompAvailablePictSpecific; /* This avoids the continuous clearing
                                                                                                            out of the Intensity Compensation data
                                                                                                            The Intensity Compensation data
                                                                                                            coming in is a rare occurance.*/
    BOOL                                             ExpectingFieldBDU;  /* Received a first Field and expecting the
                                                                                        matching second field */
    BOOL                                             NextPicRandomAccessPoint;  /* True if the next picture in the stream is a random access point*/
    BOOL                                             SeqEndAfterThisSC;
    BOOL                                             AllReferencesAvailableForFrame;
    U32                                              NumberOfFramesInFile;   /* The number of frames in the AVS file */

    semaphore_t *                                    InjectSharedAccess;
    semaphore_t *                                    ParserOrder; /* Controller commands */
    semaphore_t *                                    ParserSC; /* New Start Code to process */
    semaphore_t *                                    ControllerSharedAccess; /* access between controller and parser */
    PARSER_GetPictureParams_t                        GetPictureParams;
    VIDCOM_Task_t                                    ParserTask;
    ForTask_t                                        ForTask;
    ParserState_t                                    ParserState;
    PARSER_ParsingJobResults_t                       ParserJobResults;
    STVID_UserData_t                                 UserData;
    StartCode_t                                      StartCode;  /*!< start code to process */
   BOOL                                              PTSAvailableForNextPicture; /* TRUE if new PTS found or interpolated */
   STVID_PTS_t                                       PTSForNextPicture;
    PTS_t                                            LastPTSStored;
    PTS_SCList_t                                     PTS_SCList;
    ByteStream_t                                     ByteStream;
    AVSVideoSequenceHeader_t                         VideoSequenceHeader;    /* vide sequence parameters */
    AVSPictureHeader_t                               VideoPictureHeader; /* VOP parameters */
    U8 *                                             NonVOPStartCodeAddress; /* Start code address of non VOPs in bitbuffer */
    BOOL                                             NonVOPStartCodeFound;
    BOOL                                             UserDataAllowed;    /* This variable indicates where we can allow user data to be taken into account */

#ifdef STVID_DEBUG_GET_STATISTICS
    STVID_Statistics_t                               Statistics;
#endif
#ifdef ST_speed
    U32                                              OutputCounter; /*Number of bytes between two start codes */
    U8 *                                             LastSCAdd_p;
    STVID_DecodedPictures_t                          SkipMode;
#endif /*ST_speed*/
#ifdef STVID_TRICKMODE_BACKWARD
    BOOL                                             Backward;
    BOOL                                             BackwardStop;
    BOOL                                             StopParsing;
    BOOL                                             BackwardCheckWrite;
#endif /* STVID_TRICKMODE_BACKWARD */
} AVSParserPrivateData_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* macros for error management */
#ifdef STVID_DEBUG_PRODUCER
#define DumpError(Var, Message) {(Var) = TRUE; STTBX_Print((Message));}
#else
#define DumpError(Var, Message) {(Var) = TRUE; }
#endif

/* Exported Functions ------------------------------------------------------- */

/* In parser.c */

void avspar_InitParserVariables (const PARSER_Handle_t  ParserHandle);
void avspar_ParseVideoSequenceHeader (const PARSER_Handle_t  ParserHandle);
void avspar_ParseIntraPictureHeader(U8 *StartCodeValueAddress, U8 *NextStartCodeValueAddress,const PARSER_Handle_t  ParserHandle);
void avspar_ParseInterPictureHeader(U8 *StartCodeValueAddress, U8 *NextStartCodeValueAddress,const PARSER_Handle_t  ParserHandle);
void avspar_ParseUserDataHeader (const PARSER_Handle_t  ParserHandle);
void avspar_ParseMacroblock (const PARSER_Handle_t  ParserHandle);
void avspar_NextStartCode(const PARSER_Handle_t  ParserHandle);
void avspar_ByteAlign(const PARSER_Handle_t  ParserHandle);
U32 avspar_ByteAligned(const PARSER_Handle_t  ParserHandle, U32 nBits);
U32 avspar_NextBitsByteAligned(const PARSER_Handle_t  ParserHandle, U32 nBits);
U32 avspar_NextBitsResyncMarker(const PARSER_Handle_t  ParserHandle);
void avspar_UpdatePictureParams(const PARSER_Handle_t  ParserHandle);
void avspar_HandlePictureTypeI(const PARSER_Handle_t ParserHandle, U32 PrevPrevRefPicIndex);
void avspar_HandlePictureTypeP(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 PrevPrevRefPicIndex);
void avspar_HandlePictureTypeB(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 PrevPrevRefPicIndex);
void avspar_ParseVideoEditCode (const PARSER_Handle_t  ParserHandle);

/* In vbs.c */
U8   avspar_GetBitFromByteStream(const PARSER_Handle_t  ParserHandle);
U8   avspar_GetByteFromByteStream(const PARSER_Handle_t  ParserHandle);
BOOL avspar_IsMoreRbspData(const PARSER_Handle_t  ParserHandle);
S32  avspar_GetSigned(U8 Len, const PARSER_Handle_t  ParserHandle);
U32  avspar_GetUnsigned(U8 Len, const PARSER_Handle_t  ParserHandle);
U32  avspar_ReadUnsigned(U8 Len, const PARSER_Handle_t  ParserHandle);
void avspar_MoveToNextByteInByteStream(const PARSER_Handle_t  ParserHandle);
U32  avspar_GetVariableLengthCode(U32 *SymbolInfo, const PARSER_Handle_t  ParserHandle);
S32 se_v (const PARSER_Handle_t  ParserHandle);
U32 ue_v (const PARSER_Handle_t  ParserHandle);
void avspar_SwitchFromLittleEndianToBigEndianParsing(const PARSER_Handle_t  ParserHandle);
void avspar_SwitchFromBigEndianToLittleEndianParsing(const PARSER_Handle_t  ParserHandle);

/* In avsparser.c. CODEC API functions are prototyped in parser.h  */
void           avspar_CheckEventToSend(const PARSER_Handle_t ParserHandle);
void           avspar_DMATransferDoneFct(U32 ParserHandle,void * ES_Write_p, void * SCListWrite_p, void * SCListLoop_p);
void           avspar_PictureInfoCompile(const PARSER_Handle_t ParserHandle);
void           avspar_GetControllerCommand(const PARSER_Handle_t ParserHandle);
void           avspar_GetStartCodeCommand(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t avspar_InitInjectionBuffers(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t *  const InitParams_p, const U32 SC_ListSize, const U32 PESBufferSize);
void           avspar_InitByteStream(U8 * StartAddress, U8 * StopAddress, const PARSER_Handle_t ParserHandle);
void           avspar_InitParserContext(const PARSER_Handle_t  ParserHandle);
void           avspar_InitSCBuffer(const PARSER_Handle_t ParserHandle);
void           avspar_IsThereStartCodeToProcess(const PARSER_Handle_t ParserHandle);
void           avspar_GetNextStartCodeToProcess(const PARSER_Handle_t ParserHandle);
void           avspar_IsVOPInBitBuffer(const PARSER_Handle_t ParserHandle);
void           avspar_ParserTaskFunc(const PARSER_Handle_t ParserHandle);
void           avspar_PointOnNextSC(const PARSER_Handle_t ParserHandle);
void           avspar_ProcessStartCode(U8 StartCode, U8 * StartAddress, U8 * StopAddress, const PARSER_Handle_t ParserHandle);
void           avspar_SetESRange(const PARSER_Handle_t  ParserHandle);
void           avspar_SetStreamType(const PARSER_Handle_t ParserHandle, const STVID_StreamType_t StreamType);
void           avspar_SetPESRange(const PARSER_Handle_t ParserHandle, const PARSER_StartParams_t * const StartParams_p);
ST_ErrorCode_t avspar_StartParserTask(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t avspar_StopParserTask(const PARSER_Handle_t ParserHandle);
void avspar_FlushInjection(const PARSER_Handle_t ParserHandle);

#ifdef STVID_TRICKMODE_BACKWARD
void avspar_SetBitBuffer(const PARSER_Handle_t ParserHandle, void * const BufferAddressStart_p, const U32 BufferSize, const BitBufferType_t BufferType, const BOOL Stop);
void avspar_WriteStartCode(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p);
#endif /* STVID_TRICKMODE_BACKWARD */

ST_ErrorCode_t avspar_BitBufferInjectionInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t *  const BitBufferInjectionParams_p);


#endif /* #ifdef __AVSPARSER_H */

