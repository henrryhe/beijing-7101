/*!
 ************************************************************************
 * \file vc1parser.h
 *
 * \brief VC1 parser data structures and functions prototypes
 *
 * \author
 *  \n
 * CMG/STB \n
 * Copyright (C) STMicroelectronics 2007
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion */

#ifndef __VC1PARSER_H
#define __VC1PARSER_H


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
#include "stos.h"

#include "sttbx.h"
#include "stsys.h"

#include "vcodvc1.h"

#include "stfdma.h"
#include "inject.h"

/*
#ifdef STVID_DEBUG_PRODUCER
#define STTBX_REPORT
#define STTBX_PRINT
#endif
*/
#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
#define VC1_MP_SC_SIGNATURE_SIZE  6 /* Default Implementation Signature = 0 0 1 SC 'S' 'T' 'M' 'I' 'C' SC */
/* For signature 'S' 'T' 'M' 'I' 'C' is added and SC is repeated */
#define VC1_MP_SC_PES_HEADER_LENGTH 16 /* PES Header size with the start code sequence : 00 00 01 E1 etc... */
#endif

#ifdef STVID_PARSER_DUMP
#define TRACE_UART
#include "trace.h"
#endif /*STVID_PARSER_DUMP */



#include "vid_ctcm.h"


/* Constants ------------------------------------------------------- */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
#define TRACE_VC1PAR_COLOR (TRC_ATTR_BACKGND_BLACK | TRC_ATTR_FORGND_CYAN | TRC_ATTR_BRIGHT)

#define VC1PAR_TRACE_BLOCKID   (0x3115)    /* 0x3100 | STVID_DRIVER_ID */
#define VC1PAR_TRACE_BLOCKNAME "VC1PARSE"

enum
{
    VC1PAR_TRACE_GLOBAL = 0,
    VC1PAR_TRACE_INIT_TERM,
    VC1PAR_TRACE_EXTRACTBDU,
    VC1PAR_TRACE_PARSEBDU,
    VC1PAR_TRACE_PICTURE_INFO_COMPILE
};

#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

/*/#define VC1_DEBUG_PARSER 1 */

/* VC1_PARSER_PRINT  is used to print messages that might be useful
   for debugging the parser, but otherwise should not be used.
*/
#ifdef VC1_DEBUG_PARSER
#define VC1_PARSER_PRINT(x) STTBX_Print(x)
#else
#define VC1_PARSER_PRINT(x)
#endif /* VC1_DEBUG_PARSER*/


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
 time a reference picture occurs.
*/
#define VC1_TEST_DISPLAY_ORDER_ID 1

/*#ifdef VC1_DEBUG_PARSER
#define VC1_DEBUG_PRINT printf
#else
#define VC1_DEBUG_PRINT
#endif*/

#define EMULATION_PREVENTION_BYTE 0x03
/*! the reverse emulation prevention byte (bit reverse) for detection on bit reversed streams */
#define ETYB_NOITNEVERP_NOITALUME 0xC0

#ifdef VC1_TEST_DISPLAY_ORDER_ID
/* no reason for this number. */
#define MAX_B_FRAMES  20
#endif



/* @@@ This is temporarily placed here for compilation  purposes until the definition is included in the FDMA */
/*#define STFDMA_DEVICE_VC1_RANGE_0 4 */


/* A start code is the fixed pattern 0x00 00 01, length = 3 bytes */
#define VC1_START_CODE_PREFIX_LENGTH 3

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
#define UNSPECIFIED_COLOR_PRIMARIES 2

/* For Transfer Characteristics */
#define UNSPECIFIED_TRANSFER_CHARACTERISTICS 2

/* For Matrix Coefficients */
/* Definition of matrix_coefficients values: same as mpeg2 */
#define MATRIX_COEFFICIENTS_ITU_R_BT_709        1
#define MATRIX_COEFFICIENTS_UNSPECIFIED         2
#define MATRIX_COEFFICIENTS_ITU_R_BT_1700       6

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
#define CHROMA_FORMAT_FACTOR (3/2)

/* For FDMA SC+PTS entries */
/* FDMA_SCPTS_MASK bits are set to 1 for the length of SC/PTS in word0 */
#define FDMA_SCPTS_MASK 0x3
/* right shift of next constant must be done to get the parameter from word0 */
#define FDMA_SCPTS_SHIFT 0x0

/* if the entry masked by FDMA_SCPTS_MASK + SHIFT is FDMA_SC_PATTERN, it is a start code */
#define FDMA_SC_PATTERN 0x00
/* if the entry masked by FDMA_SCPTS_MASK + SHIFT is FDMA_PTS_PATTERN, it is a start code */
#define FDMA_PTS_PATTERN 0x01

/* FDMA_BDU_REF_IDC_MASK bits are set to 1 for the length of bdu_ref_idc */
#define FDMA_BDU_REF_IDC_MASK 0x3
/* right shift of next constant must be done to get the parameter from word2 */
#define FDMA_BDU_REF_IDC_SHIFT 0x5
/* FDMA_BDU_TYPE_MASK bits are set to 1 for the length of bdu_unit_type */
#define FDMA_BDU_TYPE_MASK 0x1f
/* right shift of next constant must be done to get the parameter from word2 */
#define FDMA_BDU_TYPE_SHIFT 0x0

/* BDU unit types */
#define BDU_TYPE_SEQ_END        0x0A
#define BDU_TYPE_SLICE          0x0B
#define BDU_TYPE_FIELD          0x0C
#define BDU_TYPE_FRAME          0x0D
#define BDU_TYPE_EPH            0x0E
#define BDU_TYPE_SH             0x0F
#define BDU_TYPE_SLICE_UD       0x1B
#define BDU_TYPE_FIELD_UD       0x1C
#define BDU_TYPE_FRAME_UD       0x1D
#define BDU_TYPE_EP_UD          0x1E
#define BDU_TYPE_SEQ_UD         0x1F
#define BDU_TYPE_SH_MPSP        0x1F /* !!!BC: warning this value is temporary (equal to BDU_TYPE_SEQ_UD which is not used in VC1 SP/MP) and will be updated once FDMA FW stop filtering VC1 reserved SC */
/* ST specific for Main and Simple profile with STVID_USE_VC1_MP_SC_SIGNATURE flag */

/* This is for indicating a
sequence header read in from
a file.  This is a "forbidden"
SC in the VC1 spec so that
it shouldn't ever conflict
with a real value. */
#define BDU_TYPE_SEQ_STRUCT_ABC                  0x80

/* This indicates the frame layer data structure
   from a file. (see Annex L.3) along plus the
  "KEY" element when the KEY element was not
   set in the header, BDU_TYPE_FRAME_LAYER_DATA_STRUCT_NO_KEY
   is received.  when the KEY element was set in
   the header, BDU_TYPE_FRAME_LAYER_DATA_STRUCT_KEY
   is received.*/
#define BDU_TYPE_FRAME_LAYER_DATA_STRUCT_NO_KEY  0x81
#define BDU_TYPE_FRAME_LAYER_DATA_STRUCT_KEY	 0x82

#define BDU_TYPE_DISCONTINUITY      0xB4 /* ST internal definition for specific usage */

#define VC1_FRAMERATEEXP_STEP 3125  /* 1/100000 Frames per second */

#define VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT  8192

#define VC1_FRAME_LAYER_DATA_STRUCT_SIZE_BYTES 8
#define VC1_PTS_TICKS_PER_SECOND               90000
#define VC1_PTS_TICKS_PER_MILLISECOND          (VC1_PTS_TICKS_PER_SECOND / 1000)


/* Default values */
#define VC1_DEFAULT_FRAME_RATE 30000 /* 30 frame/s */
#define VC1_DEFAULT_BIT_RATE (10000000/400);/* Advanced Profile Level 1  @@@  Need to determine best default */
#define VC1_DEFAULT_VBVMAX 20480000;/*  Advanced Profile Level 1 @@@  Need to determine best default */
#define VC1_DEFAULT_NON_RELEVANT   0 /* for non relevant parameters in the STVID API */
#define VC1_DEFAULT_COLOR_TYPE STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420 /* TODO: find right value */
#define VC1_DEFAULT_BITMAP_TYPE STGXOBJ_BITMAP_TYPE_MB /* TODO: find right value */
#define VC1_DEFAULT_PREMULTIPLIED_COLOR FALSE /* TODO: find right value */
#define VC1_DEFAULT_BITMAPPARAMS_OFFSET 0
#define VC1_DEFAULT_SUBBYTEFORMAT STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB /* TODO: find right value */
#define VC1_DEFAULT_BIGNOTLITTLE FALSE /* Little endian bitmap. TODO: find right value */
#define VC1_DEFAULT_SCANTYPE STGXOBJ_INTERLACED_SCAN /* TODO: find right value */
#define VC1_DEFAULT_TOPFIELDFIRST TRUE /* TODO: find right value */
#define VC1_DEFAULT_REPEATFIRSTFIELD FALSE /* TODO: check default value */
#define VC1_DEFAULT_REPEATPROGRESSIVECOUNTER 0 /* TODO: check default value */
#define VC1_DEFAULT_LEVEL  (0) /* Default to Level zero */
#define VC1_DEFAULT_WIDTH  720   /* @@@ TODO: Find best value */
#define VC1_DEFAULT_HEIGHT 480   /* @@@ TODO: Find best value */
#define VC1_DEFAULT_COLOR_TYPE STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420


/* Field Sizes */

#define VC1_FIELD_PIC_FCM_FIELD_SIZE  2
#define VC1_FPTYPE_FIELD_SIZE         3
#define VC1_TFCNTR_FIELD_SIZE         8
#define VC1_TFF_FIELD_SIZE            1
#define VC1_RFF_FIELD_SIZE            1
#define VC1_RPTFRM_FIELD_SIZE         2
#define VC1_PS_PRESENT_FIELD_SIZE     1
#define VC1_PS_HOFFSET_FIELD_SIZE    18
#define VC1_PS_VOFFSET_FIELD_SIZE    18
#define VC1_PS_WIDTH_FIELD_SIZE      14
#define VC1_PS_HEIGHT_FIELD_SIZE     14
#define VC1_PAN_N_SCAN_SIZE          ((VC1_PS_HOFFSET_FIELD_SIZE) + (VC1_PS_VOFFSET_FIELD_SIZE) + (VC1_PS_WIDTH_FIELD_SIZE) + (VC1_PS_HEIGHT_FIELD_SIZE))
#define VC1_RNDCTRL_FIELD_SIZE        1
#define VC1_UVSAMP_FIELD_SIZE         1


typedef enum
{
	PARSER_JOB_COMPLETED_EVT_ID,
	PARSER_STOP_EVT_ID,
    PARSER_PICTURE_SKIPPED_EVT_ID,
    PARSER_FIND_DISCONTINUITY_EVT_ID,
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_BITBUFFER_FULLY_PARSED_EVT_ID,
#endif
	VC1PARSER_NB_REGISTERED_EVENTS_IDS  /* Keep this one as the last one */
}VC1ParserEventIDs;

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
	VC1_PROGRESSIVE,
	VC1_INVALID_FCM,
	VC1_FRAME_INTERLACE,
	VC1_FIELD_INTERLACE,
	VC1_NUM_FRAME_CODING_MODES
} FrameCodingMode_t;

typedef enum
{
	VC1_P_PICTURE_TYPE    = 0,
	VC1_B_PICTURE_TYPE    = 2,
	VC1_I_PICTURE_TYPE    = 6,
	VC1_BI_PICTURE_TYPE   = 14,
	VC1_SKIP_PICTURE_TYPE = 15,
	VC1_NUM_PICTURE_TYPES = 16

}VC1_PictureType_t;

typedef enum
{
	VC1_REFDIST_0 =  0x00,
	VC1_REFDIST_1 =  0x01,
	VC1_REFDIST_2 =  0x02,
	VC1_REFDIST_3 =  0x06,
	VC1_REFDIST_4 =  0x0E,
	VC1_REFDIST_5 =  0x1E,
	VC1_REFDIST_6 =  0x3E,
	VC1_REFDIST_7 =  0x7E,
	VC1_REFDIST_8 =  0xFE,
	VC1_REFDIST_9 =  0x1FE,
	VC1_REFDIST_10 = 0x3FE,
	VC1_REFDIST_11 = 0x7FE,
	VC1_REFDIST_12 = 0xFFE,
	VC1_REFDIST_13 = 0x1FFE,
	VC1_REFDIST_14 = 0x3FFE,
	VC1_REFDIST_15 = 0x7FFE,
	VC1_REFDIST_16 = 0xFFFE

} VC1_Refdist_Interpret_t;

typedef enum
{
	VC1_I_I_FIELD_PICTURE_TYPE,
	VC1_I_P_FIELD_PICTURE_TYPE,
	VC1_P_I_FIELD_PICTURE_TYPE,
	VC1_P_P_FIELD_PICTURE_TYPE,
	VC1_B_B_FIELD_PICTURE_TYPE,
	VC1_B_BI_FIELD_PICTURE_TYPE,
	VC1_BI_B_FIELD_PICTURE_TYPE,
	VC1_BI_BI_FIELD_PICTURE_TYPE,
	VC1_NUM_FIELD_PICTURE_TYPES

}VC1_FieldPictureType_t;

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
#ifdef STVID_VALID
    #define PES_BUFFER_SIZE ((1*1024*1024) & 0xffffff80)
#else /* STVID_VALID */
/* (transfer periode) x (max bit rate) = 44Kb                                 */
/* 44Kb x 2 to bufferize enougth = 88Kb (here is defined 90Kb)                */
 #define PES_BUFFER_SIZE ((90*1024) & 0xffffff80)
/* size determined by experimental mean to allow */
/* injection from USB hard drive */
#endif /* STVID_VALID */



/* Definition of PES/BDU start code values */
#define SMALLEST_BDU_PACKET_START_CODE 0x0A
#define GREATEST_BDU_PACKET_START_CODE 0x1F
#define SMALLEST_VIDEO_PACKET_START_CODE 0xE0

#define GREATEST_VIDEO_PACKET_START_CODE 0xFD /* This is the stream ID used for VC1 */
#define DUMMY_RANGE 0x00

/* Parser Stack size */
#define VC1PARSER_TASK_STACK              1000

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
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_DELTAMU_HE) || defined(ST_ZEUS)
/* On STi7100 Entry->SC.SCValue is defined as a bitfield, so we can't get its address with the & operator */
#define GetSCVal(Entry)  (STSYS_ReadRegMem8((U8*)(Entry) + 8))
#else
#define GetSCVal(Entry)  (STSYS_ReadRegMem8((U8*)&((Entry)->SC.SCValue)))
#endif /*  defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_DELTAMU_HE) */
#if defined(DVD_SECURED_CHIP)
    #define GetSCAddForBitBuffer(Entry)     (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr)))
    #define GetSCAddForESCopyBuffer(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr2)))
#else
    #define GetSCAdd(Entry,Handle)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr))) /* handle kept for compatibility with inline version of GetSCAdd */
#endif /* DVD_SECURED_CHIP */


#define MIN_USER_DATA_SIZE 15

/*! The maximum number of field between 2 flushes of DPBRef (IDR or MMCO=5) picture
 * Computed here as: 30 frames/s, flush every 5 seconds */
#define MAX_PIC_BETWEEN_DPBREF_FLUSH (30 * 2 * 5)


/* Exported types ----------------------------------------------------------- */

/*! Byte stream structure */
typedef struct BDUByteStream_s
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
	BOOL StartCodeEmulationPreventionBytesUsed; /* TRUE if Start Code Emulation Prevention has been used in the
												   data being processed. If so the Emulation Prevention bytes
	                                               must be removed.  FALSE if there are no Start Code Emulation
	                                               Prevention bytes existing in the stream to be removed*/

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

} BDUByteStream_t;


/*! Current parsed picture data structure. These parameters are not stored in the Codec API picture data structure */
typedef struct PictureLocalData_s
{
	S32  FrameNumOffset; /* copied in PreviousPictureInDecodingOrder once the picture is parsed */
	U32  frame_num; /*!< stream element */
	U32  PrevRefFrameNum; /*!< variable from the standard. This is not the same as
						   * PreviousReferencePictureInDecodingOrder.frame_num at least for IDR picture */
    BOOL IsExisting; /*!< FALSE when the frame is a non-existing frame */
	U32 Width; /*!< the witdh in pixels */
	U32 Height; /*!< the height in pixels */
	BOOL IsRandomAccessPoint;
	BOOL IsValidPicture;
	U8 PrimaryPicType; /*!< stream element */

	U8  NumRef;
	U8  RefField;

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

/* Used to keep track of Intensity
   Compensation that has been applied
   to one interlace frame or
   two interlace fields */
typedef struct
{
	BOOL IntCompAvailable; /* TRUE if there are valid intensity compensation values
	                          in this structure, FALSE otherwise.  This prevents
	                          the continuous checking or resetting of all of these
	                          values, assuming that intensity compensation is fairly rare. */
	U32 Top_1;/* Pass 1 of Intensity Comp parameters for the Top field*/
	U32 Top_2;/* Pass 2 of Intensity Comp parameters for the Top field*/
	U32 Bot_1;/* Pass 1 of Intensity Comp parameters for the Bottom field*/
	U32 Bot_2;/* Pass 2 of Intensity Comp parameters for the Bottom field*/
}IntensityCompensation_t;


/*! Recovery Point informations collected in this structure */
#if 0
typedef struct RecoveryPoint_s
{
	RecoveryPointState_t RecoveryPointState;
	U8 RecoveryFrameCnt; /*!< stream element (SEI) */
	BOOL ExactMatchFlag; /*!< stream element (SEI) */
	BOOL BrokenLinkFlag; /*!< stream element (SEI) */
	U8 RecoveryFrameNum;
	BOOL RecoveryPointHasBeenHit; /*!< TRUE when reaching a RecoveryPoint. Reset to FALSE when the producer request a Random Access Point */
	BOOL CheckPOC; /*!< TRUE when reaching a Recovery Point. Set to FALSE when dealing with the sequence after the one containing the recovery point */
	VIDCOM_PictureID_t RecoveryPointPresentationOrderPictureID;
} RecoveryPoint_t;
#endif /* #if 0 */

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
	U8							     NextReferencePictureIndex;
    PARSER_ErrorRecoveryMode_t       ErrorRecoveryMode; /* copy from GetPicture parameters */
	STGXOBJ_ScanType_t               ScanTypePreviousRefPicture;
	U32                              IntensityCompPrevRefPicture;
	BOOL							 ForwardRangeredfrmFlagBFrames; /* The RangeredfrmFlag of the forward reference picture for B Frames  */
	BOOL							 ForwardRangeredfrmFlagPFrames; /* The RangeredfrmFlag of the forward reference picture for P Frames  */
	U8								 RespicBBackwardRefPicture;/* The RESPIC for the Backward Reference Picture of the next B frame */
	U8								 RespicBForwardRefPicture; /* The RESPIC for the Forward Reference Picture of the next B frame */
	U8								 RndCtrlRecentRefPicture;/* The RndCtrl from the most recent reference piture. Used for skip pictures
	                                                              and for simple and main profile*/
	VC9_PictureType_t				 SecondFieldPictureTypeSpecific;
	STVID_MPEGFrame_t				 SecondFieldPictureTypeGeneric;

	/* This is an array in which each element corresponds to each element in FullReferenceFrameList
	   if this is TRUE, then the reference picture is a field pair.  If this is false, then the
	   reference picture is a frame. Without this flag, there is no way to know whether the
	   previous reference pictures were frames or field pairs. */
	BOOL							 FieldPairFlag[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];


	U16								 RefDistPreviousRefPic; /* Stream element */
	IntensityCompensation_t          IntComp[2]; /* Intensity Compensation storage for the 2 reference frames.
	                                                The index into FullReferenceFrameList will be the same index used for
	                                                this array. */

	BOOL                             FrameRatePresent; /* TRUE if the Frame Rate has been found in the stream or it has
	                                                      already been calculated using PTSs, FALSE otherwise. */

	U32                              PreviousTimestamp;/* Saved off timestamp, used for computing the frame rate. */
	U32								 MaxWidth; /* The maximum possible width  of a frame (taken from sequence header)*/
	U32								 MaxHeight;/* The maximum possible height of a frame (taken from sequence header)*/

	VC9_PictureSyntax_t				 ForwardReferencePictureSyntax;  /* Picture Syntax for the forward reference picture  */
	VC9_PictureSyntax_t				 BackwardReferencePictureSyntax; /* Picture Syntax for the backward reference picture */


} StreamState_t;

/*! Start Code + PTS buffer data structure */
typedef struct SCBuffer_s
{
	STAVMEM_BlockHandle_t   SCListHdl;
    STFDMA_SCEntry_t   *    SCList_Start_p;         /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    SCList_Stop_p;          /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    SCList_Loop_p;          /* sc list  buffer limit  */
    STFDMA_SCEntry_t   *    SCList_Write_p;         /* sc list  buffer write pointer  */
    STFDMA_SCEntry_t   *    NextSCEntry;            /* pointer in SC List     */
    STFDMA_SCEntry_t   *    CurrentSCEntry_p;       /* pointer on current Start Code entry */
	STFDMA_SCEntry_t   *    SavedNextSCEntry_p;     /* saves the NextSCEntry before looking for next start code */
#ifdef STVID_VALID
	U32                     InjectFileFrameNum;     /* This is the frame number within an injection file */
#endif /* STVID_VALID */

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
#ifdef STVID_VALID
	U8 * ES_LastFrameEndAddrGivenToDecoder_p;
#endif
#ifdef STVID_TRICKMODE_BACKWARD
    U8 * ES_StoreStart_p;             /* ES buffer limit: Bit buffer stands from ES_Start_p inclusive */
    U8 * ES_StoreStop_p;              /* ES buffer limit: Bit buffer stands to ES_Stop_p inclusive    */
#endif /*STVID_TRICKMODE_BACKWARD*/
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
	U8 						PESSCRangeEnd;
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
	BOOL IsInBitBuffer; /* TRUE when the BDU is fully stored in the bit buffer */
	BOOL IsPending; /* TRUE when the start code is caught by the parser but is not yet processed */
	U8 Value;
	U8 * StartAddress_p;
	U8 * StopAddress_p;

	/* inside of a picture, there could be "subBDUs" such as slices, or frame, field, or slice user data
	   these variables keep track of the "subBDUs"*/
	BOOL SubBDUPending;
	U8 SubBDUType;
    U8* SubBDUStartAddress;
    U8* FirstSubBDUStartAddress;/* if there are not any slices, then this is the end address of the picture*/
	BOOL SliceFound;/*if a Slice is Found then the end address of the picture is the start
					   address of the next field, frame, sequence header of entry-point header,
					   if a Slice is not found, and there is a SubBDU start Address, then the
					   end address of the picture is the start address of the SubBDU.*/
#if defined(DVD_SECURED_CHIP)
	U8 * StartAddressInBitBuffer_p;   /* For secured platforms, StartAddress_p refers to ES Copy buffer */
	U8 * StopAddressInBitBuffer_p;    /* For secured platforms, StopAddress_p refers to ES Copy buffer  */
#endif /* defined(DVD_SECURED_CHIP) */
} StartCode_t;

/*! Parameters to manage the parser task */
typedef struct ForTask_s
{
    osclock_t  MaxWaitOrderTime; /* time to wait to get a new command */
	BOOL ControllerCommand;
} ForTask_t;


/*! Parser top level data structure that contains all the data structure above. This is to ease function calls */
typedef struct VC1ParserPrivateData_s
{
	VIDCOM_PictureGenericData_t                PictureGenericData;
	VC1_PictureSpecificData_t                  PictureSpecificData;

	VIDCOM_GlobalDecodingContextGenericData_t  GlobalDecodingContextGenericData;
	VC1_GlobalDecodingContextSpecificData_t    GlobalDecodingContextSpecificData;

	VC1_PictureDecodingContextSpecificData_t   PictureDecodingContextSpecificData;


	PrivateGlobalContextData_t                 PrivateGlobalContextData;
	PrivatePictureContextData_t                PrivatePictureContextData;
	PictureLocalData_t                         PictureLocalData;
	StreamState_t							   StreamState;
	STEVT_EventID_t                            RegisteredEventsID[VC1PARSER_NB_REGISTERED_EVENTS_IDS];
	SCBuffer_t                                 SCBuffer;
	BitBuffer_t                                BitBuffer;
#if defined(DVD_SECURED_CHIP)
	ESCopyBuffer_t                             ESCopyBuffer;
#endif /* DVD_SECURED_CHIP */
	PESBuffer_t								   PESBuffer;
	Inject_t								   Inject;
	BOOL									   IntensityCompAvailablePictSpecific; /* This avoids the continuous clearing
	                                                                                  out of the Intensity Compensation data
	                                                                                  The Intensity Compensation data
	                                                                                  coming in is a rare occurance.*/
	BOOL									   ExpectingFieldBDU;/* Received a first Field and expecting the
															        matching second field */
	BOOL									   NextPicRandomAccessPoint;  /* True if the next picture in the stream is a random access point*/
	BOOL									   SeqEndAfterThisBDU;
	BOOL									   AllReferencesAvailableForFrame;
	U32										   NumberOfFramesInFile;/* The number of frames in the VC1 file */

	semaphore_t *                              InjectSharedAccess;
	semaphore_t *                              ParserOrder; /* Controller commands */
	semaphore_t *                              ParserSC; /* New Start Code to process */
	semaphore_t *                              ControllerSharedAccess; /* access between controller and parser */
	PARSER_GetPictureParams_t                  GetPictureParams;
	VIDCOM_Task_t                              ParserTask;
	ForTask_t								   ForTask;
	ParserState_t 							   ParserState;
	PARSER_ParsingJobResults_t				   ParserJobResults;
	STVID_UserData_t						   UserData;
	U32                                        UserDataSize;
	StartCode_t 							   StartCode; /*!< start code to process */
    BOOL                                       PTSAvailableForNextPicture; /* TRUE if new PTS found or interpolated */
    STVID_PTS_t                                PTSForNextPicture;
	BDUByteStream_t                            BDUByteStream;
    PTS_t                                      LastPTSStored;
    PTS_SCList_t                               PTS_SCList;
    BOOL                                       DiscontinuityDetected;
#ifdef STVID_DEBUG_GET_STATISTICS
	STVID_Statistics_t                         Statistics;
#endif
#ifdef ST_speed
    U32                                        OutputCounter; /*Number of bytes between two start codes */
    U8 *                                       LastSCAdd_p;
    STVID_DecodedPictures_t                         SkipMode;
#endif /*ST_speed*/
#ifdef STVID_TRICKMODE_BACKWARD
    BOOL                                       Backward;
	BOOL									   StopParsing;
    BOOL                                       IsBitBufferFullyParsed;
#endif
} VC1ParserPrivateData_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#if 0 /* to be done. */
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
#endif /* 0   -- to be done */

/* macros for error management */
#ifdef STVID_DEBUG_PRODUCER
#define DumpError(Var, Message) {(Var) = TRUE; STTBX_Print((Message));}
#else
#define DumpError(Var, Message) {(Var) = TRUE; }
#endif

/* Exported Functions ------------------------------------------------------- */

/* In parse_bdu.c */

void vc1par_ParseSequenceHeader (const PARSER_Handle_t  ParserHandle);
void vc1par_ParseSpMpPesSeqHdr(const PARSER_Handle_t ParserHandle);
void vc1par_ParseSeqStructABC(const PARSER_Handle_t ParserHandle);
void vc1par_ParseEntryPointHeader(const PARSER_Handle_t ParserHandle);
void vc1par_ParseAPPicture(U8 *StartCodeValueAddress,
						 U8 *NextBDUStartCodeValueAddress,
						 const PARSER_Handle_t ParserHandle);

void vc1par_ParseAPTypePPicturePartTwo(U32 Fcm, U8* IntCompField, U32* IntComp1, U32* IntComp2, U32* IntComp1Mod, U32* IntComp2Mod,
                                BOOL BottomFieldFlag, const PARSER_Handle_t ParserHandle);
void vc1par_HandleAPPictureTypeI(const PARSER_Handle_t ParserHandle, U32 OldestRefPicIndex);
void vc1par_HandleAPPictureTypeP(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 OldestRefPicIndex,
								 U8 IntCompField, U32 IntComp1, U32 IntComp2, U32 IntComp1Mod, U32 IntComp2Mod, U32 Fcm);
void vc1par_HandleAPPictureTypeB(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 OldestRefPicIndex);
void vc1par_HandleAPPictureTypeBI(const PARSER_Handle_t ParserHandle);
void vc1par_HandleAPPictureTypeSkip(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 OldestRefPicIndex);
void vc1par_ParseAPFrameLayerDataStruct(const PARSER_Handle_t ParserHandle);
void vc1par_ParseSPMPFrame(U8 *StartCodeValueAddress,
						   U8 *NextBDUStartCodeValueAddress,
						   const PARSER_Handle_t ParserHandle);
void vc1par_ParseField(U8 *StartCodeValueAddress,
					   U8 *NextBDUStartCodeValueAddress,
					   const PARSER_Handle_t ParserHandle);
U32 vc1par_readBFraction(const PARSER_Handle_t ParserHandle,
						 U32 *NumeratorBFraction,
						 U32 *DenominatorBFraction,
						 BOOL MainProfileFlag,
						 BOOL *BIPicture);
U8 * vc1par_MoveBackInStream(U8 * Address, U8 RewindInByte, const PARSER_Handle_t  ParserHandle);
U8 * vc1par_MoveForwardInStream(U8 * Address, U8 Increment, const PARSER_Handle_t  ParserHandle);

/* In bdu.c */
void vc1par_FindUsableStartCodeBDU(const PARSER_Handle_t ParserHandle);
void vc1par_FindBDUEndAddress(const PARSER_Handle_t ParserHandle);
void vc1par_MoveToNextByteFromBDU(const PARSER_Handle_t  ParserHandle);
U8   vc1par_GetBitFromBDUUnit(const PARSER_Handle_t  ParserHandle);
U8   vc1par_GetByteFromBDUUnit(const PARSER_Handle_t  ParserHandle);
BOOL vc1par_IsMoreRbspData(const PARSER_Handle_t  ParserHandle);
S32  vc1par_GetSigned(U8 Len, const PARSER_Handle_t  ParserHandle);
S32  vc1par_GetSignedExpGolomb(const PARSER_Handle_t  ParserHandle);
U32  vc1par_GetUnsigned(U8 Len, const PARSER_Handle_t  ParserHandle);
U32  vc1par_GetVariableLengthCode(U32 *VLCTable, U32 maxlength, const PARSER_Handle_t  ParserHandle);
void vc1par_EnableDetectionOfStartCodeEmulationPreventionBytes(const PARSER_Handle_t  ParserHandle);
void vc1par_DisableDetectionOfStartCodeEmulationPreventionBytes(const PARSER_Handle_t  ParserHandle);
void vc1par_SwitchFromLittleEndianToBigEndianParsing(const PARSER_Handle_t  ParserHandle);
void vc1par_SwitchFromBigEndianToLittleEndianParsing(const PARSER_Handle_t  ParserHandle);

/* In VC1parser.c. CODEC API functions are prototyped in parser.h  */
void           vc1par_CheckEventToSend(const PARSER_Handle_t ParserHandle);
#if defined(DVD_SECURED_CHIP)
void           vc1par_DMATransferDoneFct(U32 ParserHandle,void * ES_Write_p, void * SCListWrite_p, void * SCListLoop_p, void * ESCopy_Write_p);
#else
void           vc1par_DMATransferDoneFct(U32 ParserHandle,void * ES_Write_p, void * SCListWrite_p, void * SCListLoop_p);
#endif /* DVD_SECURED_CHIP */
void           vc1par_PictureInfoCompile(const PARSER_Handle_t ParserHandle);
void           vc1par_GetControllerCommand(const PARSER_Handle_t ParserHandle);
void           vc1par_GetStartCodeCommand(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t vc1par_InitInjectionBuffers(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t *  const InitParams_p, const U32 SC_ListSize, const U32 PESBufferSize);
void           vc1par_InitBDUByteStream(U8 * BDUStartAddress, U8 * BDUStopAddress, const PARSER_Handle_t ParserHandle);
void           vc1par_InitParserContext(const PARSER_Handle_t  ParserHandle);
void           vc1par_InitSCBuffer(const PARSER_Handle_t ParserHandle);
void           vc1par_IsThereStartCodeToProcess(const PARSER_Handle_t ParserHandle);
void           vc1par_ParserTaskFunc(const PARSER_Handle_t ParserHandle);
void           vc1par_PointOnNextSC(const PARSER_Handle_t ParserHandle);
void		   vc1par_SkipSliceInfoInSCBuffer(U32 SliceElementSize, const PARSER_Handle_t ParserHandle);
#if defined(DVD_SECURED_CHIP)
void           vc1par_ProcessBDU(U8 StartCode, U8 * BDUStartAddress, U8 * BDUStopAddress, U8 * BDUStartAddressInBitBuffer, U8 * BDUStopAddressInBitBuffer, const PARSER_Handle_t ParserHandle);
#else /* defined(DVD_SECURED_CHIP) */
void           vc1par_ProcessBDU(U8 StartCode, U8 * BDUStartAddress, U8 * BDUStopAddress, const PARSER_Handle_t ParserHandle);
#endif /* defined(DVD_SECURED_CHIP) */
void           vc1par_SetESRange(const PARSER_Handle_t  ParserHandle);
void           vc1par_SetStreamType(const PARSER_Handle_t ParserHandle, const STVID_StreamType_t StreamType);
void           vc1par_SetPESRange(const PARSER_Handle_t ParserHandle, const PARSER_StartParams_t * const StartParams_p);
ST_ErrorCode_t vc1par_StartParserTask(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t vc1par_StopParserTask(const PARSER_Handle_t ParserHandle);
void		   vc1par_InitIntComp(const PARSER_Handle_t ParserHandle);
void		   vc1par_InitErrorFlags(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t vc1par_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p);
void vc1par_FlushInjection(const PARSER_Handle_t ParserHandle);
#ifdef ST_speed
#ifdef STVID_TRICKMODE_BACKWARD
void vc1par_SetBitBuffer(const PARSER_Handle_t ParserHandle, void * const BufferAddressStart_p, const U32 BufferSize, const BitBufferType_t BufferType, const BOOL Stop);
void vc1par_WriteStartCode(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p);
#endif /* STVID_TRICKMODE_BACKWARD */
#endif /* ST_speed */

ST_ErrorCode_t vc1par_BitBufferInjectionInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t * const BitBufferInjectionParams_p);
#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t vc1par_SecureInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t * const SecureInitParams_p);
#endif  /* DVD_SECURED_CHIP */

#endif /* #ifdef __VC1PARSER_H */
