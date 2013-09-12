/*!
 ************************************************************************
 * \file mpg2parser.h
 *
 * \brief MPG2 parser data structures and functions prototypes
 *
 * \author
 * Cyril Dailly based on h264parser.h file
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion */

#ifndef __MPG2PARSER_H__
#define __MPG2PARSER_H__


/* Includes ----------------------------------------------------------------- */
#include "vcodmpg2.h"

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

#include "vidcodec.h"
#include "mpg2getbits.h"


/* Constants ------------------------------------------------------- */
/*#define DEBUG_STARTCODE 1*/

#ifdef DEBUG_STARTCODE
#define MAX_TRACES_SC 10000
#define TRACES_FILE_NAME  "mpg2_dumpsc.txt"
osclock_t DMATransferTime;
extern void FlushSCBuffer();
#endif


/*Configurable Parser Task Parameters
For now, use the ones from STVID
*/
#define MPEG_PARSER_TASK_PRIORITY     STVID_TASK_PRIORITY_DECODE
#define MPEG_PARSER_TASK_STACK_SIZE   STVID_TASK_STACK_SIZE_DECODE

#define MIN_USER_DATA_SIZE 15

#define CONTROLLER_COMMANDS_BUFFER_SIZE 10

/* SC+PTS list size depends on the bit-buffer size (allocated at by the controller                               */
/* module at upper level)                                                                                        */
/* Assumption : 1 start code every 4 byte (absolute worst case: 3 bytes for SC, 1 byte for payload)              */
/*            : 1 entry = 4 bytes                                                                                */
/*            Hence: for stream @20Mbit/s, 1 second to store (start condition), gives 2.6 MByte for SCList Size! */
/* Alternative would be to store in advance the picture parameters as soon as they reach the Start Code list     */
/* TODO: arbitrate between the 2 solutions */
/* For the moment, the Start Code List Size is kept to a small value for debug */
/* TODO: Need final value for SC List Size -- increased to fit a Sarnoff test in buffer*/
/*#define SC_LIST_SIZE_IN_ENTRY 2048*/
#define SC_LIST_SIZE_IN_ENTRY  16384
#define PTS_SCLIST_SIZE 10

/* Impossible value for temporal reference (defined on 10 bits). Needed at init. */
#define IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE 2000
/* maximum difference between too succesive temporal differences */
#define MAX_DIFF_BETWEEN_TEMPORAL_REFERENCES 18
/* Maximum frames per GOP: 18 (NTSC) / 15 (PAL), i.e. 0.6 seconds */


#ifdef STVID_VALID
    #define PES_BUFFER_SIZE ((1024*1024) & 0xffffff80)
#else /* STVID_VALID */
/* (transfer periode) x (max bit rate) = 44Kb                                 */
/* 44Kb x 2 to bufferize enougth = 88Kb (here is defined 90Kb)                */
 #define PES_BUFFER_SIZE ((90*1024) & 0xffffff80)
#endif /* not STVID_VALID */

/* Max time waiting for an order: 5 times per field display (not less) */
#define MAX_WAIT_ORDER_TIME (STVID_MAX_VSYNC_DURATION / STVID_DECODE_TASK_FREQUENCY_BY_VSYNC)

/* Shortcuts to Start Code entries */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_ZEUS) || defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
/* On STi7100 Entry->SC.SCValue is defined as a bitfield, so we can't get its address with the & operator */
#define GetSCVal(Entry)  (STSYS_ReadRegMemUncached8((U8*)((U32)(Entry) + 8)))
#else
#define GetSCVal(Entry)  (STSYS_ReadRegMemUncached8((U8*)&((Entry)->SC.SCValue)))
#endif /* not (defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_ZEUS) || defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)) */

#define GetSCAdd(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr)))
#define GetPTS32(Entry)  (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->PTS.PTS0)))
#define GetPTS33(Entry)  ((STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->PTS.PTS1)) == 1) ? TRUE : FALSE)
/* The Entry->SC.Type field is a bitfield, so we can't get its address with the & operator */
#define IsPTS(Entry)     (((STSYS_ReadRegMemUncached32LE((U32*)(Entry)) & 3) != STFDMA_SC_ENTRY )? TRUE : FALSE)

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
    PARSER_NB_REGISTERED_EVENTS_IDS
};


/* Parser state */
typedef enum ParserState_e
{
	PARSER_STATE_IDLE,
	PARSER_STATE_READY_TO_PARSE,
	PARSER_STATE_PARSING
} ParserState_t;

/* Parser commands */
typedef enum ParserCommands_e
{
	PARSER_COMMAND_START,
	PARSER_COMMAND_START_CODE_TO_PROCESS,
   PARSER_COMMAND_STOP
} ParserCommands_t;


/*Startcode Parser States*/

enum ES_parse_states
{
   ES_state_no_transition,           /*Indicates that no transition should be made*/
   ES_state_inactive,                /*Looking for sequence header*/
   ES_state_sequence_header,         /*Looking for sequence header extension*/
   ES_state_sequence_extension,
   ES_state_ext0_user0,
   ES_state_gop_header,
   ES_state_user1,
   ES_state_picture_header,
   ES_state_picture_coding_ext,
   ES_state_ext2_user2,
   ES_state_picture_data,
   ES_state_sequence_end
};

#ifdef STVID_DEBUG_GET_STATISTICS
typedef struct ParserStats_s
{
   U32                                 DecodeStartCodeFound;                /*++ each completed startcode*/
   U32                                 DecodeSequenceFound;                 /*++ each sequence hdr*/
   U32                                 DecodeUserDatafound;                 /*++ each user data packet*/
   U32                                 DecodeInterruptBitBufferEmpty;       /*++ empty ES buffer interrupt*/
   U32                                 DecodeInterruptBitBufferFull;        /*++ full ES buffer interrupt*/
   U32                                 DecodePbStartCodeFoundInvalid;       /*++ invalid startcode*/
   U32                                 DecodePbStartCodeFoundVideoPES;      /*++ PES startcode encountered*/
   U32                                 DecodePbInterruptQueueOverflowError; /*++ Overflow in interrupt command queue*/
   U32                                 DecodePbHeaderFifoEmpty;             /*++ timeout waiting for startcode*/
   U32                                 SCListTimeout;                       /*Timeout for managing SC timeout*/
} ParserStats_t;
#endif


/* Exported types ----------------------------------------------------------- */


/*Startcode Parser Errors*/
enum ES_parse_error
{
   ES_error_unexpected_startcode  = 0x0001, /*Unexpected startcode received */
   ES_error_aspect_ratio          = 0x0002, /*Illegal aspect ratio specified in seq hdr*/
   ES_error_invalid_frame_type    = 0x0004, /*Invalid Frame type in picture header*/
   ES_error_frame_rate            = 0x0008, /*Invalid Frame Rate in Seq Hdr*/
   ES_error_marker_bit            = 0x0010, /*Marker bit not set*/
   ES_error_constrained_parm_flag = 0x0020, /*Constrained Parameters Flag not 0*/
   ES_error_unexpected_chroma_mtx = 0x0040, /*Unexpected chroma matrix in quant mtx ext*/
   ES_error_missing_2nd_field     = 0x0080, /*This pic is not a 2nd field as expected*/
   ES_error_unexpected_profile    = 0x0100 /*Invalid Profile in Seq Hdr*/
};


/*Startcode Parser State Structure*/

typedef struct
{
    U32                           parse_state;         /*current parsing state*/
    U32                           next_state;          /*next parsing state*/
    gb_Handle_t                   gbHandle;            /*Handle for bitwise access to eS buffer*/
    U32                           error_mask;          /*Bitfield for errors*/
    PARSER_ParsingJobResults_t    *ParserJobResults_p;   /*Parsing Job Results*/
    STVID_UserData_t              *UserData;           /*User data storage allocated at Init*/
    U32                           UserDataSize;        /*Size of User Data Buffer*/
    U32                           error_count;
    U32                           parse_error;
} tESState;

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

/*! Bit Buffer data structure */
typedef struct PESBuffer_s
{
    STAVMEM_BlockHandle_t   PESBufferHdl;
    void *                  PESBufferBase_p;
    void *                  PESBufferTop_p;
} PESBuffer_t;

/*! Injection structure */
typedef struct Inject_s
{
    VIDINJ_Handle_t InjecterHandle; /* Injecter instance handle */
    U32             InjectNum;      /* Injection number */
} Inject_t;

/*! Global variables handled by the parser that do not fit in GDC, PDC or Picture */
typedef struct ParserGlobalVariable_s
{
	U32     CurrentPictureDecodingOrderFrameID; /*!< the DecodingOrderFrameID to use for the current picture */
    U32     ExtendedPresentationOrderPictureID; /*!< to store the current value from one picture to another */
	U8      StreamID; /* copy from setup parameters */
    PARSER_ErrorRecoveryMode_t     ErrorRecoveryMode; /* copy from GetPicture parameters */
} ParserGlobalVariable_t;

/*! Start Code Found */
typedef struct
{
	BOOL IsInBitBuffer; /* TRUE when the NAL is fully stored in the bit buffer */
	BOOL IsPending; /* TRUE when the start code is caught by the parser but is not yet processed */
	U8 Value;
	U8 * StartAddress_p;
	U8 * StopAddress_p;
	U32 SliceCount; /* Indicate the number of slices found in the previous picture */
} StartCode_t;

/*! Parameters to manage the parser task */
typedef struct ForTask_s
{
    osclock_t  MaxWaitOrderTime; /* time to wait to get a new command */
} ForTask_t;

typedef struct
{
   BOOL                 IsFree;
   U32                  DecodingOrderFrameID;
   S32                  TemporalReference;
   VIDCOM_PictureID_t   ExtendedPresentationOrderPictureID;
} MPG2ParserRefData_t;

typedef struct
{
   BOOL                 MissingPrevious;
   MPG2ParserRefData_t  LastRef;
   MPG2ParserRefData_t  LastButOneRef;
   semaphore_t          *ReferencePictureSemaphore;

} MPG2ParserReference_t;

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

typedef struct MPG2ParserPrivateData_s
{
    ST_Partition_t      *CPUPartition_p;
    STEVT_EventID_t     RegisteredEventsID[PARSER_NB_REGISTERED_EVENTS_IDS];
    ParserState_t       ParserState;
    PARSER_StartParams_t StartParams_p;
    PARSER_ParsingJobResults_t ParserJobResults;
    /* Local tables for specific data used for specifi pointers within ParserJobResults.ParsedPictureInformation_p structure */
    U8 PictureSpecificData[VIDCOM_MAX_PICTURE_SPECIFIC_DATA_SIZE];
    U8 PictureDecodingContextSpecificData[VIDCOM_MAX_PICTURE_DECODING_CONTEXT_SPECIFIC_DATA_SIZE];
    U8 GlobalDecodingContextSpecificData[VIDCOM_MAX_GLOBAL_DECODING_CONTEXT_SPECIFIC_DATA_SIZE];
	semaphore_t *                              InjectSharedAccess;
	semaphore_t *                              ParserOrder;
	semaphore_t *                              ParserSC;
	SCBuffer_t                                 SCBuffer;
	BitBuffer_t                                BitBuffer;
	PESBuffer_t								   PESBuffer;
	Inject_t								      Inject;
	STVID_UserData_t						   UserData;
    U32                                 UserDataSize;
	StartCode_t 	                     StartCode;
	ParserGlobalVariable_t              ParserGlobalVariable;
	PARSER_GetPictureParams_t                GetPictureParams;
	VIDCOM_Task_t                       ParserTask;
	ForTask_t								   ForTask;
    U32                                 SCParserState;
    gb_Handle_t                         gbHandle;
    CommandsBuffer_t                    CommandsBuffer;
    U8                                  CommandsData[CONTROLLER_COMMANDS_BUFFER_SIZE];
    U32                                 CurrentPictureDecodingOrderFrameID;
    MPG2ParserReference_t               ReferencePicture;
#ifdef STVID_DEBUG_GET_STATISTICS
    ParserStats_t                       Statistics;
#endif
#ifdef ST_speed
    U32                                        OutputCounter; /*Number of bytes between two start codes */
    U8 *                                       LastSCAdd_p;
    STVID_DecodedPictures_t                    SkipMode;
#ifdef STVID_TRICKMODE_BACKWARD
    BOOL                                       Backward;
	BOOL									   StopParsing;
    BOOL                                       IsBitBufferFullyParsed;
#endif
#endif /*ST_speed*/

    BOOL                                   PreviousPictureIsAvailable;

    BOOL                                       PTSAvailableForNextPicture; /* TRUE if new PTS found or interpolated */
    STVID_PTS_t                                PTSForNextPicture;
    PTS_t                                      LastPTSStored;
    PTS_SCList_t                               PTS_SCList;

    BOOL                                DiscontinuityDetected;

} MPG2ParserPrivateData_t;



/* Exported Variables ------------------------------------------------------- */

#ifdef DEBUG_STARTCODE
char tracesSC[MAX_TRACES_SC][80];
U8 * SCListAddress[MAX_TRACES_SC * 10];
U32 indSC, indAdr, nbSC, nbSCW;
STFDMA_SCEntry_t * SClist_p, * Previous_SCListWrite_p;
#endif

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t mpg2par_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p);
ST_ErrorCode_t mpg2par_Stop(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t mpg2par_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p);
ST_ErrorCode_t mpg2par_GetPicture(const PARSER_Handle_t ParserHandle, const PARSER_GetPictureParams_t * const GetPictureParams_p);
ST_ErrorCode_t mpg2par_Term(const PARSER_Handle_t ParserHandle);
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t mpg2par_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * Statistics_p);
ST_ErrorCode_t mpg2par_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p);
#endif
#ifdef STVID_DEBUG_GET_STATUS
static ST_ErrorCode_t mpg2par_GetStatus(const PARSER_Handle_t ParserHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
ST_ErrorCode_t mpg2par_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p);
ST_ErrorCode_t mpg2par_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p);

ST_ErrorCode_t mpg2par_ProcessStartCode(U8 StartCode, U8 * StartAddress_p, U8 * StopAddress_p, const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t mpg2par_InitStartCode(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t mpg2par_StartParserTask(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t mpg2par_StopParserTask(const PARSER_Handle_t ParserHandle);

ST_ErrorCode_t  mpg2par_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle,
                                                void ** const BaseAddress_p,
                                                U32 * const Size_p);
ST_ErrorCode_t  mpg2par_SetDataInputInterface(const PARSER_Handle_t ParserHandle,
                                ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
                                void (*InformReadAddress)(void * const Handle, void * const Address),
                                void * const Handle);

void mpg2par_DirecTVUserData(const PARSER_Handle_t ParserHandle);

ST_ErrorCode_t mpg2par_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p);
void mpg2par_FlushInjection(const PARSER_Handle_t ParserHandle);
#ifdef ST_speed
U32            mpg2par_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle);
#ifdef STVID_TRICKMODE_BACKWARD
void           mpg2par_SetBitBuffer(const PARSER_Handle_t ParserHandle, void * const BufferAddressStart_p, const U32 BufferSize, const BitBufferType_t BufferType, const BOOL Stop);
void           mpg2par_WriteStartCode(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p);
#endif /* STVID_TRICKMODE_BACKWARD */
#endif /*ST_speed*/

ST_ErrorCode_t mpg2par_BitBufferInjectionInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t * const SecureInitParams_p);
#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t mpg2par_SecureInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t * const SecureInitParams_p);
#endif /* DVD_SECURED_CHIP */

#endif /* #ifdef __MPG2PARSER_H__ */
