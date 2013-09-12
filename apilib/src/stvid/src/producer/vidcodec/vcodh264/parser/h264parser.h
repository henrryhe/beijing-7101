/*!
 ************************************************************************
 * \file h264parser.h
 *
 * \brief H264 parser data structures and functions prototypes
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion */

#ifndef __H264PARSER_H
#define __H264PARSER_H

/* Glossary ----------------------------------------------------------------- */
/*
 * CFP: Complementary Field Pair
 * DPBRef: the array that stores the reference frames, CFP or non-paired fields
 * GDC: Global Decoding Context (a term to speak about SPS)
 * PDC: Picture Decoding Context (a term to speak about PPS)
 * POC: Picture Order Count
 * PPDO: Previous Picture in Decoding Order (some parameters need to be stored for POC computation
 */

/* Includes ----------------------------------------------------------------- */
#include "vcodh264.h"

#include "stfdma.h"
#include "inject.h"

#ifdef STVID_DEBUG_PARSER
#define STTBX_REPORT
#define STTBX_PRINT
#endif

#include "stos.h"
#include "sttbx.h"
#include "stsys.h"

#include "vid_ctcm.h"
/* Constants ------------------------------------------------------- */

/* Any time the driver needs a don't care value */
#define DONT_CARE 0

#define EMULATION_PREVENTION_BYTE 0x03
/*! the reverse emulation prevention byte (bit reverse) for detection on bit reversed streams */
#define ETYB_NOITNEVERP_NOITALUME 0xC0

/* A start code is the fixed pattern 0x00 00 01, length = 3 bytes */
#define START_CODE_LENGTH 3

/* for profile_idc */
#define PROFILE_BASELINE 66
#define PROFILE_MAIN     77
#define PROFILE_EXTENDED 88
#define PROFILE_HIGH     100
#define PROFILE_HIGH10   110
#define PROFILE_HIGH422  122
#define PROFILE_HIGH444  144

/* For Aspect Ratio */
#define ExtendedSAR 255

/* For Video Format */
#define UNSPECIFIED_VIDEO_FORMAT 5

/* For Colour Primaries */
#define UNSPECIFIED_COLOUR_PRIMARIES 2

/* For Transfer Characteristics */
#define UNSPECIFIED_TRANSFER_CHARACTERISTICS 2

/* For Matrix Coefficients */
/* Definition of matrix_coefficients values: same as mpeg2 */
#define MATRIX_COEFFICIENTS_ITU_R_BT_709        1
#define MATRIX_COEFFICIENTS_UNSPECIFIED         2
#define MATRIX_COEFFICIENTS_FCC                 4
#define MATRIX_COEFFICIENTS_ITU_R_BT_470_2_BG   5
#define MATRIX_COEFFICIENTS_SMPTE_170M          6
#define MATRIX_COEFFICIENTS_SMPTE_240M          7

/* For Primary pic type: table 7-2 */
#define PRIMARY_PIC_TYPE_I 0
#define PRIMARY_PIC_TYPE_IP 1
#define PRIMARY_PIC_TYPE_IPB 2
#define PRIMARY_PIC_TYPE_SI 3
#define PRIMARY_PIC_TYPE_SISP 4
#define PRIMARY_PIC_TYPE_ISI 5
#define PRIMARY_PIC_TYPE_ISIPSP 6
#define PRIMARY_PIC_TYPE_ISIPSPB 7
#define PRIMARY_PIC_TYPE_UNKNOWN 255

/* For SliceType: table 7-3 */
#define SLICE_TYPE_P 0
#define SLICE_TYPE_B 1
#define SLICE_TYPE_I 2
#define ALL_SLICE_TYPE_P (SLICE_TYPE_P + 5)
#define ALL_SLICE_TYPE_B (SLICE_TYPE_B + 5)
#define ALL_SLICE_TYPE_I (SLICE_TYPE_I + 5)

/* For pic_struct: table D-1 */
#define PICSTRUCT_FRAME             0
#define PICSTRUCT_TOP_FIELD         1
#define PICSTRUCT_BOTTOM_FIELD      2
#define PICSTRUCT_TOP_BOTTOM        3
#define PICSTRUCT_BOTTOM_TOP        4
#define PICSTRUCT_TOP_BOTTOM_TOP    5
#define PICSTRUCT_BOTTOM_TOP_BOTTOM 6
#define PICSTRUCT_FRAME_DOUBLING    7
#define PICSTRUCT_FRAME_TRIPLING    8

/* For MaxLongTermFrameIdx */
#define NO_LONG_TERM_FRAME_INDICES -1

/* The initial value of index when looking for a free frame buffer in DPBRef */
#define NO_FREE_FRAME_BUFFER_IN_DPBREF -1

/* for equation A.3.1 for MaxDpbSize: assumption is YUV 4:2:0 */
#define NB_BYTES_PER_MB (256 + 128)

/* For FDMA SC+PTS entries */
/* FDMA_SCPTS_MASK bits are set to 1 for the length of SC/PTS in word0 */
#define FDMA_SCPTS_MASK 0x3
/* right shift of next constant must be done to get the parameter from word0 */
#define FDMA_SCPTS_SHIFT 0x0

/* if the entry masked by FDMA_SCPTS_MASK + SHIFT is FDMA_SC_PATTERN, it is a start code */
#define FDMA_SC_PATTERN 0x00
/* if the entry masked by FDMA_SCPTS_MASK + SHIFT is FDMA_PTS_PATTERN, it is a start code */
#define FDMA_PTS_PATTERN 0x01

/* FDMA_NAL_REF_IDC_MASK bits are set to 1 for the length of nal_ref_idc */
#define FDMA_NAL_REF_IDC_MASK 0x3
/* right shift of next constant must be done to get the parameter from word2 */
#define FDMA_NAL_REF_IDC_SHIFT 0x5
/* FDMA_NAL_UNIT_TYPE_MASK bits are set to 1 for the length of nal_unit_type */
#define FDMA_NAL_UNIT_TYPE_MASK 0x1f
/* right shift of next constant must be done to get the parameter from word2 */
#define FDMA_NAL_UNIT_TYPE_SHIFT 0x0

#define SMALL_BUFF_BYTES 128

/* NAL unit types */
#define NAL_UNIT_TYPE_SLICE    1
#define NAL_UNIT_TYPE_DPA      2
#define NAL_UNIT_TYPE_DPB      3
#define NAL_UNIT_TYPE_DPC      4
#define NAL_UNIT_TYPE_IDR      5
#define NAL_UNIT_TYPE_SEI      6
#define NAL_UNIT_TYPE_SPS      7
#define NAL_UNIT_TYPE_PPS      8
#define NAL_UNIT_TYPE_AUD      9
#define NAL_UNIT_TYPE_EOSEQ    10
#define NAL_UNIT_TYPE_EOSTREAM 11
#define NAL_UNIT_TYPE_FILL     12
#define NAL_UNIT_TYPE_SPS_EXT   13
#define NAL_UNIT_TYPE_14       14
#define NAL_UNIT_TYPE_15       15
#define NAL_UNIT_TYPE_16       16
#define NAL_UNIT_TYPE_17       17
#define NAL_UNIT_TYPE_18       18
#define NAL_UNIT_TYPE_AUX_SLICE 19

/* NAL types are unspecified starting from 24. To get the NAL unit type from teh start code, we apply the mask: 0x1F. */
/* With start code B8 seen trough this mask gives 24 */
#define NAL_UNIT_DISCONTINUITY 24


/* Default values */
#if !defined(H264_DEFAULT_FRAME_RATE)
#define H264_DEFAULT_FRAME_RATE 25000 /* 25 frames/seconds */
#endif /* !defined(H264_DEFAULT_FRAME_RATE) */
#define H264_UNSPECIFIED_FRAME_RATE 1010 /* arbitrary key value used when stream has no VUI */
#define H264_DEFAULT_NON_RELEVANT   0 /* for non relevant parameters in the STVID API */
#define H264_DEFAULT_COLOR_TYPE STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420 /* TODO: find right value */
#define H264_DEFAULT_BITMAP_TYPE STGXOBJ_BITMAP_TYPE_MB /* TODO: find right value */
#define H264_DEFAULT_PREMULTIPLIED_COLOR FALSE /* TODO: find right value */
#define H264_DEFAULT_BITMAPPARAMS_OFFSET 0
#define H264_DEFAULT_SUBBYTEFORMAT STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB /* TODO: find right value */
#define H264_DEFAULT_BIGNOTLITTLE FALSE /* Little endian bitmap. TODO: find right value */
#define H264_DEFAULT_SCANTYPE STGXOBJ_INTERLACED_SCAN /* TODO: find right value */
#define H264_DEFAULT_TOPFIELDFIRST TRUE /* TODO: find right value */
#define H264_DEFAULT_REPEATFIRSTFIELD FALSE /* TODO: check default value */
#define H264_DEFAULT_REPEATPROGRESSIVECOUNTER 0 /* TODO: check default value */
#define H264_DEFAULT_PICSTRUCT PICSTRUCT_TOP_BOTTOM /* Consider stream as interlaced by default if there is no picture timing SEI */
#define H264_DEFAULT_CTTYPE 1 /* scan type interlaced by default */

/* Shortcuts to nal_ref_idc and nal_unit_type */
#define GetNalRefIdc(StartCode) (((StartCode) >> FDMA_NAL_REF_IDC_SHIFT) & FDMA_NAL_REF_IDC_MASK)
#define GetNalUnitType(StartCode) (((StartCode) >> FDMA_NAL_UNIT_TYPE_SHIFT) & FDMA_NAL_UNIT_TYPE_MASK)

enum
{
    PARSER_JOB_COMPLETED_EVT_ID,
    PARSER_STOP_EVT_ID,
    PARSER_USER_DATA_EVT_ID,
    PARSER_PICTURE_SKIPPED_EVT_ID,
    PARSER_FIND_DISCONTINUITY_EVT_ID,
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_BITBUFFER_FULLY_PARSED_EVT_ID,
#endif
    H264PARSER_NB_REGISTERED_EVENTS_IDS  /* Keep this one as the last one */
};

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

/* SC+PTS list size depends on the bit-buffer size (allocated at by the controller                               */
/* module at upper level)                                                                                        */
/* Assumption : 1 start code every 4 byte (absolute worst case: 3 bytes for SC, 1 byte for payload)              */
/*            : 1 entry = 4 bytes                                                                                */
/*            Hence: for stream @20Mbit/s, 1 second to store (start condition), gives 2.6 MByte for SCList Size! */
/* Alternative would be to store in advance the picture parameters as soon as they reach the Start Code list     */
/* TODO: arbitrate between the 2 solutions */
/* For the moment, the Start Code List Size is kept to a small value for debug */
#define SC_LIST_SIZE_IN_ENTRY /*2048*/ 11520
#define PTS_SCLIST_SIZE 10

/* max nb of NAL messages that can be pushed before they're parsed (Used for SEI messages and UserData) */
#define MAX_SEI_NUMBER 30
#define MAX_USER_DATA_NUMBER 10

/* PES Buffer Size (in bytes)                                                 */
/* (transfer periode) x (max bit rate) = 44Kb                                 */
/* 44Kb x 2 to bufferize enougth = 88Kb (here is defined 90Kb)                */
 #define PES_BUFFER_SIZE ((90*1024) & 0xffffff80)
/* size determined by experimental mean (!!) to allow */
/* injection from USB hard drive */
/* Definition of PES/NAL start code values */
#define SMALLEST_NAL_PACKET_START_CODE 0x00
#define GREATEST_NAL_PACKET_START_CODE 0x7F
#define SMALLEST_VIDEO_PACKET_START_CODE 0xE0
#define GREATEST_VIDEO_PACKET_START_CODE 0xEF
#define DUMMY_RANGE 0x00

/* Parser Stack size */
#define H264PARSER_TASK_STACK              1000

/* Max time waiting for an order: 5 times per field display (not less) */
#define MAX_WAIT_ORDER_TIME (STVID_MAX_VSYNC_DURATION / STVID_DECODE_TASK_FREQUENCY_BY_VSYNC)

/* Command buffers */
/* 10 commands sent by the controller is far enough, as no queue is allowed so far in the parser */
#define CONTROLLER_COMMANDS_BUFFER_SIZE 10

/* Shortcuts to Start Code entries */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
/* On STi7100 Entry->SC.SCValue is defined as a bitfield, so we can't get its address with the & operator */
#define GetSCVal(Entry)  (STSYS_ReadRegMemUncached8((U8*)((U32)(Entry) + 8)))
#else /* ST_7100 || ST_7109 || ST_7200 */
#define GetSCVal(Entry)  (STSYS_ReadRegMemUncached8((U8*)&((Entry)->SC.SCValue)))
#endif /* ST_7100 || ST_7109 || ST_7200 */

#if defined(DVD_SECURED_CHIP)
    #define GetSCAddForBitBuffer(Entry)     (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr)))
    #define GetSCAddForESCopyBuffer(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr2)))
#else
#define GetSCAdd(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr)))
#endif /* DVD_SECURED_CHIP */

#define GetPTS32(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->PTS.PTS0)))
#define GetPTS33(Entry)  ((STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->PTS.PTS1)) == 1) ? TRUE : FALSE)
/* The Entry->SC.Type field is a bitfield, so we can't get its address with the & operator */
#define IsPTS(Entry)     (((STSYS_ReadRegMemUncached32LE((U32*)(Entry)) & 3) != STFDMA_SC_ENTRY )? TRUE : FALSE)

/* Shortcuts to NAL unit types */
#define IsSliceStartCode(StartCode) (((GetNalUnitType(StartCode) == NAL_UNIT_TYPE_SLICE) || (GetNalUnitType(StartCode) == NAL_UNIT_TYPE_IDR)) ? TRUE : FALSE)

#define MIN_USER_DATA_SIZE 15

/*! The maximum number of field between 2 flushes of DPBRef (IDR or MMCO=5) picture
 * Computed here as: 30 frames/s, flush every 5 seconds */
#define MAX_PIC_BETWEEN_DPBREF_FLUSH (30 * 2 * 5)

/* SEI messages */
typedef enum {
    SEI_BUFFERING_PERIOD = 0,
    SEI_PIC_TIMING,
    SEI_PAN_SCAN_RECT,
    SEI_FILLER_PAYLOAD,
    SEI_USER_DATA_REGISTERED_ITU_T_T35,
    SEI_USER_DATA_UNREGISTERED,
    SEI_RECOVERY_POINT,
    SEI_DEC_REF_PIC_MARKING_REPETITION,
    SEI_SPARE_PIC,
    SEI_SCENE_INFO,
    SEI_SUB_SEQ_INFO,
    SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
    SEI_SUB_SEQ_CHARACTERISTICS,
    SEI_FULL_FRAME_FREEZE,
    SEI_FULL_FRAME_FREEZE_RELEASE,
    SEI_FULL_FRAME_SNAPSHOT,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
    SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
    SEI_FILM_GRAIN_CHARACTERISTICS,
    SEI_RESERVED_SEI_MESSAGE,
    SEI_MAX_ELEMENTS  /*!< number syntax elements */
} SEIType;

typedef enum
{
    RECOVERY_NONE, /*!< default */
    RECOVERY_PENDING, /*!< When the SEI message is seen, up to the computation of the frame_num for that point */
    RECOVERY_COMPUTED, /*!< When the slice related to the SEI message is parsed */
    RECOVERY_REACHED /*!< Active during the parsing of the recovery point slice only. Then reset to RECOVERY_NONE */
} RecoveryPointState_t;

/* Value to track h264par_GetUnsignedExpGolomb() failure */
#define UNSIGNED_EXPGOLOMB_FAILED   0xFFFFFFFF

/* Exported types ----------------------------------------------------------- */

/*! Byte stream structure */
typedef struct NALByteStream_s
{
    U8 *RBSPStartByte_p; /*!< pointer to beginning of byte stream */
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
#ifdef BYTE_SWAP_SUPPORT
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
#endif /*BYTE_SWAP_SUPPORT*/

} NALByteStream_t;

typedef struct SEIArray_s {
    NALByteStream_t NALByteStream[MAX_SEI_NUMBER];
    U32 Iterator;
} SEIArray_t;

typedef struct UserDataArray_s {
    NALByteStream_t NALByteStream[MAX_USER_DATA_NUMBER];
    U32 PayloadSize[MAX_USER_DATA_NUMBER];
    U32 PayloadType[MAX_USER_DATA_NUMBER];
    U32 Iterator;
} UserDataArray_t;

/* For POC computation */
/*! Previous Picture In Decoding Order data structure for POC computation. Is updated after any picture is parsed */
typedef struct PreviousPictureInDecodingOrder_s
{
    BOOL IsAvailable; /*!< FALSE for 1st picture (no previous picture available). Set to FALSE on setup. Set to TRUE once the 1st picture is parsed */
    STVID_PictureStructure_t PictureStructure; /*!< Frame, Top or bottom field information */
    BOOL HasMMCO5; /*!< TRUE if the picture contains a MMCO = 5 operation */
    BOOL IsFirstOfTwoFields; /*!< TRUE if picture is not the complementary field of the previous picture */
    U32  FrameNumOffset;
    U32  frame_num; /*!< stream element */
    BOOL IsIDR; /*!< TRUE if picture is an IDR */
    U32 IdrPicId;
    BOOL IsReference; /*!< TRUE if picture is a reference picture */
    U32 Width; /*!< the witdh in pixels */
    U32 Height; /*!< the height in pixels */
    U8 MaxDecFrameBuffering; /*!< max_dec_frame_buffering variables as in the standard */
} PreviousPictureInDecodingOrder_t;

/*! Previous Reference Picture In Decoding Order data structure for POC computation. Is updated after any reference picture is parsed */
typedef struct PreviousReferencePictureInDecodingOrder_s
{
    BOOL HasMMCO5; /*!< TRUE if the picture contains a MMCO = 5 operation */
    STVID_PictureStructure_t PictureStructure; /*!< Frame, Top or bottom field information */
    S32  TopFieldOrderCnt;
    S32  PicOrderCntMsb;
    U32  pic_order_cnt_lsb; /*!< stream element */
    U32  frame_num; /*!< used in pre marking detection stage to compute PrevRefFrameNum */

} PreviousReferencePictureInDecodingOrder_t;

/*! Current parsed picture data structure for POC computation. These parameters are not stored in the Codec API picture data structure */
typedef struct PictureLocalData_s
{
    U32  pic_order_cnt_lsb; /*!< stream element */
    S32  PicOrderCntMsb; /*! Used for PreviousReferencePictureInDecodingOrder */
    S32  delta_pic_order_cnt_bottom; /*!< stream element */
    S32  delta_pic_order_cnt[2]; /*!< stream element */
    S32  FrameNumOffset; /* copied in PreviousPictureInDecodingOrder once the picture is parsed */
    U32  frame_num; /*!< stream element */
    U32  PrevRefFrameNum; /*!< variable from the standard. This is not the same as
                           * PreviousReferencePictureInDecodingOrder.frame_num at least for IDR picture */
    BOOL HasMMCO5; /*!< TRUE if the picture contains a MMCO = 5 operation */
    U8 ActivePPS; /*!< the PPS index used for the current picture */
    U8 ActiveSPS; /*!< the SPS index used for the current picture */
    BOOL IsNonExisting; /*!< TRUE when the frame is a non-existing frame */
    U32 Width; /*!< the witdh in pixels */
    U32 Height; /*!< the height in pixels */
    U8 MaxDecFrameBuffering; /*!< max_dec_frame_buffering variables as in the standard */
    U32 IdrPicId;
    BOOL IsRandomAccessPoint;
    BOOL IncrementEPOID; /* Force ExtendedPresentationOrderPictureID increment if the entry point found is an IDR or a recovery point SEI message */
    BOOL IsValidPicture;
    BOOL HasSEIMessage;
    BOOL HasUserData;
#ifdef ST_XVP_ENABLE_FGT
    BOOL HasFGTMessage;
#endif  /* ST_XVP_ENABLE_FGT */
    U8 PicStruct; /*!< From Picture Timing SEI */
    BOOL HasCtType;
    U8 CtType; /*!< From Picture Timing SEI */
    NALByteStream_t SEIPositionInNAL;
    U8 PrimaryPicType; /*!< stream element */
} PictureLocalData_t;

/*! SPS data structure for POC computations. These parameters are not stored in the GDC */
typedef struct SPSLocalData_s
{
    BOOL IsAvailable; /*!< on setup: set to FALSE. Goes to TRUE on the first corresponding SPS encountered in the stream */
    U32 MaxPicOrderCntLsb; /*!< MaxPicOrderCntLsb = 2^( log2_max_pic_order_cnt_lsb_minus4 + 4 ) (7-2) */
    U32 MaxFrameNum; /*!< MaxFrameNum = 2( log2_max_frame_num_minus4 + 4 )      (7-1) */
    U8  num_ref_frames_in_pic_cnt_cycle; /*!< stream element */
    S32 offset_for_ref_frame[255]; /*!< stream element */
    S32 offset_for_non_ref_pic; /*!< stream element */
    S32 offset_for_top_to_bottom_field; /*!< stream element */
    BOOL gaps_in_frame_num_allowed_flag; /*!< stream element */
    BOOL pic_struct_present_flag; /*!< VUI stream element */
    U8   MaxDpbSize; /*!< at most = 16 for SD and 4 for HD */
    U32 MaxBR; /*!< see table A-1 */
    U32 MaxCPB; /*!< see table A-1 */
    U32 MaxWidth; /*!< see table A-5 */
    U32 MaxHeight; /*!< see table A-5 */
    U32 MaxPixel; /*!< see table A-5 */
    BOOL NalHrdBpPresentFlag; /*!< variable defined in the standard */
    BOOL VclHrdBpPresentFlag; /*!< variable defined in the standard */
    BOOL CpbDpbDelaysPresentFlag; /*!< variable defined in the standard */
    U8 InitialCpbRemovalDelayLength; /*!< initial_cpb_removal_delay_length VUI element */
    U8 CpbRemovalDelayLength; /*!< cpb_removal_delay_length VUI element */
    U8 DpbOutputDelayLength; /*!< dpb_output_delay_length VUI element */
    U8 TimeOffsetLength; /*!< time_offset_length VUI element */
    BOOL IsInitialCpbRemovalDelayValid;
    U32 InitialCpbRemovalDelay; /*!< From Buffering Period SEI */
} SPSLocalData_t;

/*! PPS data structure. These parameters are not stored in the GDC */
typedef struct PPSLocalData_s
{
    BOOL IsAvailable; /*!< on setup: set to FALSE. Goes to TRUE on the first corresponding PPS encountered in the stream */
    U8 seq_parameter_set_id; /*!< stream element: index to the corresponding SPS */
} PPSLocalData_t;

#ifdef STVID_PARSER_CHECK
/*! ListD is used to check TopFieldOrderCnt and BottomFieldOrderCnt values (8.2.1) */
typedef struct ListD_s
{
    S32  FieldOrderCnt; /*!< contains either TopFieldOrderCnt or BottomFieldOrderCnt according to the picture */
    U32  DecodingOrderFrameID; /*!< the FrameID for the picture being stored in this position in the array */
    BOOL IsTopField; /*!< TRUE when the field stored here is a top field */
    S32  PicOrderCnt; /*!< PresentationOrderPictureID */
} ListD_t;
#endif

/*! Recovery Point informations collected in this structure */
typedef struct RecoveryPoint_s
{
    RecoveryPointState_t RecoveryPointState;
    U32 RecoveryFrameCnt; /*!< stream element (SEI) */
    BOOL ExactMatchFlag; /*!< stream element (SEI) */
    BOOL BrokenLinkFlag; /*!< stream element (SEI) */
    U32 RecoveryFrameNum;
    BOOL RecoveryPointHasBeenHit; /*!< TRUE when reaching a RecoveryPoint. Reset to FALSE when the producer request a Random Access Point */
    BOOL CheckPOC; /*!< TRUE when reaching a Recovery Point. Set to FALSE when dealing with the sequence after the one containing the recovery point */
    BOOL CurrentPicturePOIDToBeSaved; /* TRUE if we reach a non IDR and not recovery point random access point, then RecoveryPointPresentationOrderPictureID will contain this picture order ID for further POC checks */
    VIDCOM_PictureID_t RecoveryPointPresentationOrderPictureID;
    U32 RecoveryPointDecodingOrderFrameID;
} RecoveryPoint_t;

/*! Global variables handled by the parser that do not fit in GDC, PDC or Picture */
typedef struct ParserGlobalVariable_s
{
    S8      MaxLongTermFrameIdx; /*!< a variable defined in the standard for MMCO operations */
    U32     CurrentPictureDecodingOrderFrameID; /*!< the DecodingOrderFrameID to use for the current picture */
    U8      CurrentPictureFrameIndexInDPBRef; /*!< the location where the current picture is stored in the DPBRef */
    U32     ExtendedPresentationOrderPictureID; /*!< to store the current value from one picture to another */
    #ifdef STVID_PARSER_CHECK
    U32     ListDNumberOfElements; /*!< used size for ListD */
    ListD_t ListD[MAX_PIC_BETWEEN_DPBREF_FLUSH]; /*!< to check TopFieldOrderCnt and BottomFieldOrderCnt from last IDR or MMCO=5 */
    ListD_t ListO[MAX_PIC_BETWEEN_DPBREF_FLUSH]; /*!< to check TopFieldOrderCnt and BottomFieldOrderCnt from last IDR or MMCO=5 */
    #endif
    U8      StreamID; /* copy from start parameters */
    STVID_BroadcastProfile_t Broadcast_profile;  /* copy from start parameters */
    RecoveryPoint_t RecoveryPoint;
    U8 ProfileIdc;
    PARSER_ErrorRecoveryMode_t     ErrorRecoveryMode; /* copy from GetPicture parameters */
    STVID_DecodedPictures_t DecodedPictures; /* I only , IP only, ALL pictures to be parsed & decoded */
} ParserGlobalVariable_t;

/*! Start Code + PTS buffer data structure */
typedef struct SCBuffer_s
{
    STAVMEM_BlockHandle_t   SCListHdl;
    STFDMA_SCEntry_t   *    SCList_Start_p;         /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    SCList_Stop_p;          /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    SCList_Loop_p;          /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    SCList_Write_p;         /* sc list  buffer write pointer  */
    STFDMA_SCEntry_t   *    LatchedSCList_Loop_p;   /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    LatchedSCList_Write_p;  /* sc list  buffer write pointer  */
    STFDMA_SCEntry_t   *    NextSCEntry;            /* pointer in SC List     */
    STFDMA_SCEntry_t   *    CurrentSCEntry;         /* pointer on current Start Code entry */
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
#ifdef STVID_TRICKMODE_BACKWARD
    U8 * ES_StoreStart_p;             /* ES buffer limit: Bit buffer stands from ES_Start_p inclusive */
    U8 * ES_StoreStop_p;              /* ES buffer limit: Bit buffer stands to ES_Stop_p inclusive    */
#endif /*STVID_TRICKMODE_BACKWARD*/
    U8 * ES_Write_p;             /* write pointer (of FDMA) in ES buffer   */
    U8 * ES_DecoderRead_p;       /* Read pointer (of decoder) in ES buffer */
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

#if defined(DVD_SECURED_CHIP)

/*! ES Copy Buffer data structure */
typedef struct ESCopyBuffer_s
{
    U8 * ESCopy_Start_p;             /* ES Copy buffer limit: buffer stands from ES_Start_p inclusive */
    U8 * ESCopy_Stop_p;              /* ES Copy buffer limit: buffer stands to ES_Stop_p inclusive    */
    U8 * ESCopy_Write_p;             /* Write pointer (of FDMA) in ES Copy buffer   */
} ESCopyBuffer_t;
#endif /* DVD_SECURED_CHIP */

/*! Bit Buffer data structure */
typedef struct PESBuffer_s
{
    STAVMEM_BlockHandle_t   PESBufferHdl;
    void *                  PESBufferBase_p;
    void *                  PESBufferTop_p;
    BOOL                    PESRangeEnabled; /*!< Enabled (true) when input is PES */
    U8                      PESSCRangeStart; /*!< the full 8 bit to use to configure the FDMA [7:5]=111 [4:0]=StreamID */
    U8                      PESSCRangeEnd;
    BOOL                    PESOneShotEnabled;
} PESBuffer_t;

/*! Injection structure */
typedef struct Inject_s
{
    VIDINJ_Handle_t InjecterHandle; /* Injecter instance handle */
    U32             InjectNum; /* Injection number */
} Inject_t;

/*! Context save for pre-marking process */
typedef struct PreMarkingContext_s
{
    U32 frame_num;
    BOOL IsIDR;
    STVID_PictureStructure_t PictureStructure;
    BOOL IsReference;
    S32 delta_pic_order_cnt[2];
} PreMarkingContext_t;

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
    BOOL IsNonExisting; /*!< Set to TRUE when the frame is issued from the gaps in frame_num pre marking process */
    BOOL IsBrokenLink;
} DPBFrameReference_t;

/*! Start Code Found */
typedef struct
{
    BOOL IsInBitBuffer; /* TRUE when the NAL is fully stored in the bit buffer */
    BOOL IsPending; /* TRUE when the start code is caught by the parser but is not yet processed */
    U8 Value;
    U8 * StartAddress_p;
    U8 * StopAddress_p;
    U32 SliceCount; /* Indicate the number of slices found in the previous picture */
#if defined(DVD_SECURED_CHIP)
    U8 * BitBufferStartAddress_p;   /* For secured platforms, StartAddress_p refers to ES Copy buffer */
    U8 * BitBufferStopAddress_p;    /* For secured platforms, StopAddress_p refers to ES Copy buffer  */
#endif /* DVD_SECURED_CHIP */
} StartCode_t;

/*! Parameters to manage the parser task */
typedef struct ForTask_s
{
    osclock_t  MaxWaitOrderTime; /* time to wait to get a new command */
    BOOL ControllerCommand;
} ForTask_t;

/*! Parser top level data structure that contains all the data structure above. This is to ease function calls */
typedef struct H264ParserPrivateData_s
{
    PictureLocalData_t                         PictureLocalData;
    VIDCOM_PictureGenericData_t                PictureGenericData;
    H264_PictureSpecificData_t                 PictureSpecificData;
    PreviousPictureInDecodingOrder_t           PreviousPictureInDecodingOrder;
    PreviousReferencePictureInDecodingOrder_t  PreviousReferencePictureInDecodingOrder;
    SPSLocalData_t                             SPSLocalData[MAX_SPS];
    SPSLocalData_t                             *ActiveSPSLocalData_p;
    VIDCOM_GlobalDecodingContextGenericData_t  GlobalDecodingContextGenericData[MAX_SPS];
    VIDCOM_GlobalDecodingContextGenericData_t  *ActiveGlobalDecodingContextGenericData_p;
    H264_GlobalDecodingContextSpecificData_t   GlobalDecodingContextSpecificData[MAX_SPS];
    H264_GlobalDecodingContextSpecificData_t   *ActiveGlobalDecodingContextSpecificData_p;
    PPSLocalData_t                             PPSLocalData[MAX_PPS];
    PPSLocalData_t                             *ActivePPSLocalData_p;
    H264_PictureDecodingContextSpecificData_t  PictureDecodingContextSpecificData[MAX_PPS];
    H264_PictureDecodingContextSpecificData_t  *ActivePictureDecodingContextSpecificData_p;
    ParserGlobalVariable_t                     ParserGlobalVariable;
#ifdef ST_XVP_ENABLE_FGT
    VIDCOM_FilmGrainSpecificData_t             FilmGrainSpecificData;
#endif /* ST_XVP_ENABLE_FGT */
    NALByteStream_t                            NALByteStream;
    NALByteStream_t                            SEIMessagePosition;
    SEIArray_t                                 SEIPositionArray;
    NALByteStream_t                            UserDataPosition;
    UserDataArray_t                            UserDataPositionArray;
    DPBFrameReference_t                        DPBFrameReference[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES];
    PreMarkingContext_t                        PreMarkingContext;
    STEVT_EventID_t                            RegisteredEventsID[H264PARSER_NB_REGISTERED_EVENTS_IDS];
    SCBuffer_t                                 SCBuffer;
    BitBuffer_t                                BitBuffer;
#if defined(DVD_SECURED_CHIP)
    ESCopyBuffer_t                             ESCopyBuffer;
#endif /* DVD_SECURED_CHIP */
    PESBuffer_t                                PESBuffer;
    Inject_t                                   Inject;
#ifdef STVID_DEBUG_GET_STATISTICS
    STVID_Statistics_t                         Statistics;
#endif
    semaphore_t *                              ParserOrder; /* Controller commands */
    semaphore_t *                              ParserSC; /* New Start Code to process */
    PARSER_GetPictureParams_t                  GetPictureParams;
    VIDCOM_Task_t                              ParserTask;
    ForTask_t                                  ForTask;
    ParserState_t                              ParserState;
    PARSER_ParsingJobResults_t                 ParserJobResults;
    STVID_UserData_t                           UserData;
    U32                                        UserDataSize;

    StartCode_t         StartCode; /*!< start code to process */
    BOOL                                       PTSAvailableForNextPicture; /* TRUE if new PTS found or interpolated */
    STVID_PTS_t                                PTSForNextPicture;
    PTS_t                                      LastPTSStored;
    PTS_SCList_t                               PTS_SCList;
    BOOL                                       DiscontinuityDetected;
#ifdef ST_speed
    U32                                        OutputCounter; /*Number of bytes between two start codes */
    U8 *                                       LastSCAdd_p;
    STVID_DecodedPictures_t                    SkipMode;
#ifdef STVID_TRICKMODE_BACKWARD
    BOOL                                       Backward;
    BOOL                                       StopParsing;
    BOOL                                       IsBitBufferFullyParsed;
#endif
#endif /*ST_speed */
    U8 *                                       LastSCAddBeforeSEI_p;
    BOOL                                        StopIsPending;
    /* GNBvd65692 : add the following DPBFrameReference_t arrays (~5KB each) in ParserData_p,
        instead of allocating as local variables, to save some space on the stack to avoid a possible stack corruption
        attn: There's no need to initialize these arrays at init time, as they are initialized just before being used in ref_pic_list.c */
    DPBFrameReference_t                        ShortTermFramesBelowPOC[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES];
    DPBFrameReference_t                        ShortTermFramesAbovePOC[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES];
    DPBFrameReference_t                        RefFrameList0ShortTerm[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES];
    DPBFrameReference_t                        RefFrameList1ShortTerm[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES];
    DPBFrameReference_t                        RefFrameList0LongTerm[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES];
    /* add PreviousSCWasNotValid variable for GNBvd63171  for LG (MPEG2/H264 switch): implement an algo in H264 parser to detect if MPEG2 stream is injected */
    BOOL                                       PreviousSCWasNotValid; 
} H264ParserPrivateData_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* macros around DPBRef */
#define DPBRefTopFieldIsRef(i) ( PARSER_Data_p->DPBFrameReference[i].TopFieldIsReference == TRUE )
#define DPBRefBottomFieldIsRef(i) ( PARSER_Data_p->DPBFrameReference[i].BottomFieldIsReference == TRUE )
#define DPBRefFrameIsUsed(i) ( (DPBRefTopFieldIsRef(i)) || (DPBRefBottomFieldIsRef(i)) )
#define DPBRefTopFieldIsShortTerm(i) ((DPBRefTopFieldIsRef(i)) && (PARSER_Data_p->DPBFrameReference[i].TopFieldIsLongTerm == FALSE))
#define DPBRefBottomFieldIsShortTerm(i) ( (DPBRefBottomFieldIsRef(i)) && (PARSER_Data_p->DPBFrameReference[i].BottomFieldIsLongTerm == FALSE))
#define DPBRefTopFieldIsLongTerm(i) ( (DPBRefTopFieldIsRef(i)) && (PARSER_Data_p->DPBFrameReference[i].TopFieldIsLongTerm == TRUE))
#define DPBRefBottomFieldIsLongTerm(i) ((DPBRefBottomFieldIsRef(i)) && (PARSER_Data_p->DPBFrameReference[i].BottomFieldIsLongTerm == TRUE))

#define DPBRefFrameHasShortTermField(i) (DPBRefTopFieldIsShortTerm(i) || DPBRefBottomFieldIsShortTerm(i))
#define DPBRefFrameHasLongTermField(i) (DPBRefTopFieldIsLongTerm(i) || DPBRefBottomFieldIsLongTerm(i))
#define DPBRefFrameIsShortTermFrame(i) (DPBRefTopFieldIsShortTerm(i) && DPBRefBottomFieldIsShortTerm(i))
#define DPBRefFrameIsLongTermFrame(i) (DPBRefTopFieldIsLongTerm(i) && DPBRefBottomFieldIsLongTerm(i))

/* macros around PictureStructure */
#define CurrentPictureIsFrame ( PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
#define CurrentPictureIsTopField (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
#define CurrentPictureIsBottomField (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
#define CurrentPictureIsNotFrame ( PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
#define CurrentPictureIsNotTopField (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_TOP_FIELD)
#define CurrentPictureIsNotBottomField (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)

#define PreviousPictureIsFrame ( PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
#define PreviousPictureIsTopField (PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
#define PreviousPictureIsBottomField (PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
#define PreviousPictureIsNotFrame ( PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
#define PreviousPictureIsNotTopField (PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure != STVID_PICTURE_STRUCTURE_TOP_FIELD)
#define PreviousPictureIsNotBottomField (PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure != STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)

/* macros for error management */
#ifdef STVID_DEBUG_PRODUCER
#define DumpError(Var, Message) {(Var) = TRUE; STTBX_Print((Message));}
#else
#define DumpError(Var, Message) {(Var) = TRUE; }
#endif

/* Exported Functions ------------------------------------------------------- */

/* In marking.c */
void h264par_AssignShortTermInFrameBuffer(U8 FrameIndex, const PARSER_Handle_t  ParserHandle);
void h264par_DecodingProcessForPictureNumbers(const PARSER_Handle_t  ParserHandle);
S32 h264par_ComputePicNumX(U32 DifferenceOfPicNumsMinus1, const PARSER_Handle_t  ParserHandle);
void h264par_DoMMCO1(U32 DifferenceOfPicNumsMinus1, const PARSER_Handle_t  ParserHandle);
void h264par_DoMMCO2(U32 LongTermPicNum, const PARSER_Handle_t  ParserHandle);
void h264par_DoMMCO3(U32 DifferenceOfPicNumsMinus1, U8 LongTermFrameIdx, const PARSER_Handle_t  ParserHandle);
void h264par_DoMMCO4(U8 MaxLongTermFrameIdxPlus1, const PARSER_Handle_t  ParserHandle);
void h264par_DoMMCO5(const PARSER_Handle_t  ParserHandle);
void h264par_DoMMCO6(U8 LongTermFrameIdx, const PARSER_Handle_t  ParserHandle);
S8   h264par_FindFreeFrameIndex(const PARSER_Handle_t  ParserHandle);
S8   h264par_FindComplementaryField(const PARSER_Handle_t  ParserHandle);
void h264par_MarkAllPicturesAsUnusedForReference(const PARSER_Handle_t  ParserHandle);
S8   h264par_MarkCurrentAsShortTermInFreeFrameBuffer(const PARSER_Handle_t  ParserHandle);
void h264par_MarkCurrentPictureAsLongTerm(U8 FrameIndex, U8 LongTermFrameIdx, const PARSER_Handle_t  ParserHandle);
void h264par_MarkCurrentPictureAsShortTerm(const PARSER_Handle_t ParserHandle);
void h264par_MarkFrameAsUnusedForReference(U8 FrameIndex, const PARSER_Handle_t  ParserHandle);
void h264par_MarkPictureBySliding(const PARSER_Handle_t  ParserHandle);
void h264par_MarkPictureWithLongTermFrameIdxAsUnusedForReference(U8 LongTermFrameIdx, S8 FrameToMark, const PARSER_Handle_t  ParserHandle);
void h264par_MarkPicNumXAsLongTerm(U8 FrameIndex, S32 PicNumX, U8 LongTermFrameIdx, const PARSER_Handle_t  ParserHandle);
void h264par_PerformPreMarking(const PARSER_Handle_t  ParserHandle);
void h264par_PerformSlidingWindowOnDPBRef (const PARSER_Handle_t  ParserHandle);
void h264par_PostProcessFrameNumOnMMCO5(const PARSER_Handle_t  ParserHandle);
void h264par_PostProcessPicOrderCntOnMMCO5 (const PARSER_Handle_t  ParserHandle);
void h264par_RestoreContextAfterPreMarking(const PARSER_Handle_t  ParserHandle);
void h264par_SaveContextBeforePreMarking(const PARSER_Handle_t  ParserHandle);
void h264par_ClipDPBAfterMMCO(const PARSER_Handle_t  ParserHandle);

/* In parse_pps.c */
void h264par_InitializePPS (U8 PicParameterSetId, const PARSER_Handle_t  ParserHandle);
void h264par_ParsePPS (const PARSER_Handle_t  ParserHandle);

/* In parse_slice.c */
#ifdef STVID_PARSER_CHECK
void h264par_BuildListO(const PARSER_Handle_t  ParserHandle);
void h264par_CheckListO(const PARSER_Handle_t ParserHandle);
void h264par_CheckListD(const PARSER_Handle_t  ParserHandle);
BOOL h264par_DiffPicOrderCntOutOfRange(U32 FirstElementLoop, U32 SecondElementLoop, const PARSER_Handle_t ParserHandle);
S32  h264par_FieldOrderCntListOCmpFunc(const void *elem1, const void *elem2);
void h264par_InsertCurrentPictureInListD(const PARSER_Handle_t  ParserHandle);
void h264par_SemanticsChecksPicOrderCntType(const PARSER_Handle_t  ParserHandle);
#endif
U32  h264par_ComputeDecodingOrderFrameID (const PARSER_Handle_t  ParserHandle);
void h264par_InitializeCurrentPicture(U8 NalRefIdc, U8 NalUnitType, U8 SeqParameterSetId, U8 PicParameterSetId, const PARSER_Handle_t  ParserHandle);
BOOL h264par_IsComplementaryFieldOfPreviousPicture(const PARSER_Handle_t  ParserHandle);
U8 * h264par_MoveBackInStream(U8 * Address, U8 RewindInByte, const PARSER_Handle_t  ParserHandle);
void h264par_ParseDecodedReferencePictureMarking(const PARSER_Handle_t  ParserHandle);
void h264par_ParsePredictionWeightTable(U8 SliceType, U8 NumRefIdxActiveL0ActiveMinus1, U8 NumRefIdxActiveL1ActiveMinus1, const PARSER_Handle_t  ParserHandle);
void h264par_ParseReferencePictureListReodering(U8 SliceType, const PARSER_Handle_t  ParserHandle);
void h264par_ParseRestOfSliceHeader(U8 SliceType, const PARSER_Handle_t  ParserHandle);
void h264par_GetSliceTypeFromSliceHeader (const PARSER_Handle_t  ParserHandle, U8 *SliceType);
void h264par_ParseSliceHeader(U8 NalRefIdc, U8 NalUnitType, StartCode_t * StartCode_p, const PARSER_Handle_t  ParserHandle, const U8 SliceType);
void h264par_UpdatePreviousPictureInfo(const PARSER_Handle_t  ParserHandle);

/* In parse_sps.c */
void h264par_ComputeMaxFromLevel(U8 LevelIdc, U8 PicWidthInMbsMinus1, U8 PicHeightInMapUnitsMinus1, U8 FrameMbsOnlyFlag, U8 SeqParameterSetId, const PARSER_Handle_t  ParserHandle);
void h264par_InitializeGlobalDecodingContext(U8 SeqParameterSedId, const PARSER_Handle_t  ParserHandle);
void h264par_ParseSPS(const PARSER_Handle_t  ParserHandle);
void h264par_ParseSPSExtension(const PARSER_Handle_t  ParserHandle);
void h264par_GetSampleAspectRatio(U8 AspectRatioIdc, U16 * SarWidth, U16 * SarHeight);
void h264par_ParseVUI (U8 SeqParameterSetId, const PARSER_Handle_t  ParserHandle);
void h264par_SetFlatScalingMatrix(H264_SpecificScalingMatrix_t *ScalingMatrix);
void h264par_SetScalingListDefault(H264_SpecificScalingMatrix_t *ScalingMatrix, U32 ScalingListNumber);
void h264par_SetScalingListFallBackA(H264_SpecificScalingMatrix_t *ScalingMatrix,U32 ScalingListNumber);
void h264par_SetScalingListFallBackB(H264_SpecificScalingMatrix_t *PPSScalingMatrix,const H264_SpecificScalingMatrix_t *SPSScalingMatrix,U32 ScalingListNumber);
void h264par_GetScalingList(const PARSER_Handle_t  ParserHandle, H264_SpecificScalingMatrix_t *ScalingMatrix,U32 ScalingListNumber);

/* In poc.c */
void h264par_CheckTopAndBottomFieldOrderCnt(const PARSER_Handle_t  ParserHandle);
void h264par_ComputePicOrderCnt (const PARSER_Handle_t  ParserHandle);
void h264par_ComputeTopAndBottomFieldOrderCntForPOC0 (const PARSER_Handle_t  ParserHandle);
void h264par_ComputeTopAndBottomFieldOrderCntForPOC1 (const PARSER_Handle_t  ParserHandle);
void h264par_ComputeTopAndBottomFieldOrderCntForPOC2 (const PARSER_Handle_t  ParserHandle);

/* In nal.c */
U8   h264par_GetBitFromNALUnit(const PARSER_Handle_t  ParserHandle);
U8   h264par_GetByteFromNALUnit(const PARSER_Handle_t  ParserHandle);
BOOL h264par_IsMoreRbspData(const PARSER_Handle_t  ParserHandle);
S32  h264par_GetSigned(U8 Len, const PARSER_Handle_t  ParserHandle);
S32  h264par_GetSignedExpGolomb(const PARSER_Handle_t  ParserHandle);
U32  h264par_GetUnsigned(U8 Len, const PARSER_Handle_t  ParserHandle);
U32  h264par_GetUnsignedExpGolomb(const PARSER_Handle_t  ParserHandle);
void h264par_MoveToNextByteFromNALUnit(const PARSER_Handle_t  ParserHandle);
void h264par_SavePositionInNAL(const PARSER_Handle_t ParserHandle, NALByteStream_t * const CurrentPositionInNAL);
void h264par_RestorePositionInNAL(const PARSER_Handle_t ParserHandle, const NALByteStream_t CurrentPositionInNAL);
void h264par_PushSEIListItem(const PARSER_Handle_t ParserHandle);
NALByteStream_t* h264par_GetSEIListItem(const PARSER_Handle_t ParserHandle);
void h264par_PushUserDataItem(U32 PayLoadSize, U32 PayLoadType, const PARSER_Handle_t ParserHandle);
NALByteStream_t* h264par_GetUserDataItem(U32 * PayLoadSize_p, U32 * PayLoadType_p, const PARSER_Handle_t ParserHandle);

/* In ref_pic_list.c */
void h264par_AlternateFieldsInList(U32 *                 UsedSizeInReferenceListX,
                                   U32 *                 ReferenceListX,
                                   BOOL *                IsReferenceTopField,
                                   U8                    NumberOfFrames,
                                   DPBFrameReference_t * RefFrameListX,
                                   BOOL (* h264par_IsCurrentParity) (DPBFrameReference_t RefFrame),
                                   BOOL (* h264par_IsAlternateParity) (DPBFrameReference_t RefFrame),
                                   const PARSER_Handle_t ParserHandle);
void h264par_BuildReferenceFieldsList(U32 *                 UsedSizeInReferenceListX,
                                      U32 *                 ReferenceListX,
                                      BOOL *                IsReferenceTopField,
                                      U8                    NumberOfShortTermFrames,
                                      U8                    NumberOfLongTermFrames,
                                      DPBFrameReference_t * RefFrameListXShortTerm,
                                      DPBFrameReference_t * RefFrameListXLongTerm,
                                      const PARSER_Handle_t ParserHandle);
void h264par_CheckIfB0AndB1ListAreEqual(const PARSER_Handle_t ParserHandle);
void h264par_InitFullList(const PARSER_Handle_t  ParserHandle);
void h264par_InitRefPicListForBSlicesInField(const PARSER_Handle_t  ParserHandle);
void h264par_InitRefPicListForBSlicesInFrame(const PARSER_Handle_t  ParserHandle);
void h264par_InitRefPicListForPSlicesInField(const PARSER_Handle_t ParserHandle);
void h264par_InitRefPicListForPSlicesInFrame(const PARSER_Handle_t ParserHandle);
BOOL h264par_IsBottomFieldLongRef(DPBFrameReference_t RefFrameLongTerm);
BOOL h264par_IsBottomFieldShortRef(DPBFrameReference_t RefFrameShortTerm);
BOOL h264par_IsTopFieldLongRef(DPBFrameReference_t RefFrameLongTerm);
BOOL h264par_IsTopFieldShortRef(DPBFrameReference_t RefFrameShortTerm);
S32  h264par_PicOrderCnt(S32 Elem1, S32 Elem2);
S32  h264par_PicOrderCntDPB(const DPBFrameReference_t * DPBFrameReference);
void h264par_ReferencePictureListsInitialisation(const PARSER_Handle_t  ParserHandle);
S32  h264par_SortAscendingLongTermFrameIdxFunction(const void *Elem1, const void *Elem2);
S32  h264par_SortAscendingPicOrderCntFunction(const void *Elem1, const void *Elem2);
S32  h264par_SortAscendingTopFieldLongTermPicNumFunction(const void *Elem1, const void *Elem2);
S32  h264par_SortDescendingFrameNumWrapFunction(const void *Elem1, const void *Elem2);
S32  h264par_SortDescendingPicNumFunction(const void *Elem1, const void *Elem2);
S32  h264par_SortDescendingPicOrderCntFunction(const void *Elem1, const void *Elem2);

/* In parse_sei.c */
void h264par_ParseBufferingPeriod(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseDataUnregistered(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseDecRefPicMarkingRepetitionPeriod(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseFillerPayLoad(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParsePicTiming(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParsePanScanRect(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseRecoveryPoint(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseUserDataRegisteredITUTT35(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseSparePic(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseSceneInfo(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseSubSeqInfo(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseSubSeqLayerCharacteristics(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseSubSeqCharacteristics(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseFullFrameFreeze(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseFullFrameFreezeRelease(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseFullFrameSnapshot(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseProgressiveRefinementSegmentStart(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseProgressiveRefinementSegmentEnd(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseMotionConstrainedSliceGroupSet(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseReservedSeiMessage(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_SEIPayLoad(U32 PayLoadType, U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseFilmGrainCharacteristics(const PARSER_Handle_t ParserHandle);
void h264par_SetDefaultSEI(const PARSER_Handle_t  ParserHandle);
void h264par_ResetPicturePanAndScanIn16thPixel(const PARSER_Handle_t  ParserHandle);
void h264par_IncrementInterpolatedTimeCode(const PARSER_Handle_t  ParserHandle);
void h264par_ParseAUD (const PARSER_Handle_t  ParserHandle);
void h264par_PrepareToParseSEI(const PARSER_Handle_t ParserHandle);
void h264par_MoveToNextSEIMessage(U32 PayLoadSize, const PARSER_Handle_t ParserHandle);
void h264par_ParseSEI (const PARSER_Handle_t  ParserHandle);

/* In h264parser.c. CODEC API functions are prototyped in parser.h  */
void           h264par_CheckEventToSend(const PARSER_Handle_t ParserHandle);
#if defined(DVD_SECURED_CHIP)
void           h264par_DMATransferDoneFct(U32 ParserHandle,void * ES_Write_p, void * SCListWrite_p, void * SCListLoop_p, void * ESCopy_Write_p);
#else
void           h264par_DMATransferDoneFct(U32 ParserHandle,void * ES_Write_p, void * SCListWrite_p, void * SCListLoop_p);
#endif /* DVD_SECURED_CHIP */
void           h264par_FillParsingJobResult(const PARSER_Handle_t ParserHandle);
void           h264par_GetControllerCommand(const PARSER_Handle_t ParserHandle);
void           h264par_GetStartCodeCommand(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t h264par_InitInjectionBuffers(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t *  const InitParams_p, const U32 SC_ListSize, const U32 PESBufferSize);
void           h264par_InitNALByteStream(StartCode_t * StartCode_p, const PARSER_Handle_t ParserHandle);
void           h264par_InitParserContext(const PARSER_Handle_t  ParserHandle);
void           h264par_InitSCBuffer(const PARSER_Handle_t ParserHandle);
void           h264par_IsNALInBitBuffer(const PARSER_Handle_t ParserHandle);
void           h264par_GetNextStartCodeToProcess(const PARSER_Handle_t ParserHandle);
void           h264par_IsThereStartCodeToProcess(const PARSER_Handle_t ParserHandle);
void           h264par_ParserTaskFunc(const PARSER_Handle_t ParserHandle);
void           h264par_PointOnNextSC(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t h264par_ProcessStartCode(StartCode_t * StartCode_p, const PARSER_Handle_t ParserHandle);
void           h264par_SetESRange(const PARSER_Handle_t  ParserHandle);
void           h264par_SetStreamType(const PARSER_Handle_t ParserHandle, const STVID_StreamType_t StreamType);
void           h264par_SetPESRange(const PARSER_Handle_t ParserHandle, const PARSER_StartParams_t * const StartParams_p);
ST_ErrorCode_t h264par_StartParserTask(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t h264par_StopParserTask(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t h264par_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle, void ** const BaseAddress_p, U32 * const Size_p);
ST_ErrorCode_t h264par_SetDataInputInterface(const PARSER_Handle_t ParserHandle,ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),void (*InformReadAddress)(void * const Handle, void * const Address),void * const Handle);
ST_ErrorCode_t h264par_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p);
void           h264par_FlushInjection(const PARSER_Handle_t ParserHandle);

#ifdef ST_speed
U32            h264par_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle);
#endif /*ST_speed*/

#ifdef STVID_TRICKMODE_BACKWARD
void           h264par_SetBitBuffer(const PARSER_Handle_t ParserHandle, void * const BufferAddressStart_p, const U32 BufferSize, const BitBufferType_t BufferType, const BOOL Stop);
void           h264par_WriteStartCode(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p);
#endif

ST_ErrorCode_t h264par_BitBufferInjectionInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t *  const BitBufferInjectionParams_p);
#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t h264par_SecureInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t *  const SecureInitParams_p);
#endif /* DVD_SECURED_CHIP */

#endif /* #ifdef __H264PARSER_H */
