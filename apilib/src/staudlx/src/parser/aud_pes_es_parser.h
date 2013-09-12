/******************************************************************************/
/*                                                                            */
/* File name   : aud_pes_es_parser.h                                          */
/*                                                                            */
/* Description : PES+ES parser file                                           */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/* History     :                                                              */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 15/09/05           Created                      K.MAITI                    */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/


#ifndef _AUD_PES_ES_PARSER
#define _AUD_PES_ES_PARSER

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
#include "stclkrv.h"
#endif
#include "staud.h"
#include "aud_drv.h"
#include "acc_multicom_toolbox.h"

#ifdef STAUD_INSTALL_HEAAC
#include "parse_heaac.h"
#endif
#ifdef ST_51XX
#include "aud_utils.h"
#endif
#include "blockmanager.h"

/*DVDV+DVDA+PACKMP1/MP2*/
#if defined MSPP_PARSER
#include "mspp_parser.h"
#else
#include "com_parser.h"
#endif
#include "aud_evt.h"
#ifdef MSPP_PARSER
    #define PES_TO_ES_BY_FDMA
#else
    //#define PES_TO_ES_BY_FDMA
#endif
#if ((defined STAUD_INSTALL_AIFF) || (defined STAUD_INSTALL_WAVE))
#define MAXBUFFSIZE  (CDDA_NB_SAMPLES*2*2*3)    // Maximum Size of the buffer samples
#endif

#define PES_ES_MAX_NUMBER       3 /*Can be increased*/

/* Parser task stack size */
#if defined(ST_51XX)
#define PES_ES_PARSER_TASK_STACK_SIZE   (1024*2)
#else
#define PES_ES_PARSER_TASK_STACK_SIZE   (1024*16)
#endif
/* Parser task priority */
#ifndef PES_ES_PARSER_TASK_PRIORITY
#define PES_ES_PARSER_TASK_PRIORITY     6
#endif

#define PES_ES_NUM_IP_BUFFERS (1 << 3)
#define PES_ES_NUM_IP_MASK      (~((U32)(-PES_ES_NUM_IP_BUFFERS)))
/* Memory Pool Handle */
typedef U32 STPESES_Handle_t;

typedef enum
{
    TIMER_INIT = 0,
    TIMER_TRANSFER_START,
    TIMER_TRANSFER_PAUSE,
    TIMER_TRANSFER_RESUME,
    TIMER_STOP,
    TIMER_TERMINATE
}ParserState_t;

typedef enum
{
    PARSER_STATE_IDLE = 0,
    PARSER_STATE_GET_INPUT_BUFFER,
    PARSER_STATE_POLL_INPUT_BUFFER,
    PARSER_STATE_CHECK_UNDERFLOW,
    PARSER_STATE_PARSE_INPUT,
    PARSER_STATE_FREE_INPUT_BUFFER,
    PARSER_STATE_UPDATE_READ_PONTER
}ParserInputState_t;

typedef enum
{
    BUFFER_FREE = 0,
    BUFFER_FIILED,
    BUFFER_SENT
}BufferState_t;
typedef struct Timer_s
{
    ParserState_t State;
    STOS_Mutex_t *Lock;
}Timer_t;

#ifndef ST_51XX
typedef union StartupState_u
{

#ifdef ST_OSWINCE
    int dummy; /* WinCE doesn't take in empty unions*/
#endif
}StartupState_t;
#endif
typedef ST_ErrorCode_t (* Parse_FrameFunc_t)
(
    STPESES_Handle_t    Handle
);

typedef ST_ErrorCode_t (* StartUpSyncFunc_t)
(
    STPESES_Handle_t    Handle
);

typedef ST_ErrorCode_t (* Parse_StopFunc_t)
(
    STPESES_Handle_t    Handle
);

typedef ST_ErrorCode_t (* Parse_StartFunc_t)
(
    STPESES_Handle_t    Handle
);

typedef ST_ErrorCode_t (* PESES_BytesConsumed_t)
(
    U32 , U32
);
#ifndef ST_51XX

typedef union AudioSpecificParams_u
{

#ifdef STAUD_INSTALL_HEAAC
    HEAACParserLocalParams_t HEAACParams;
#endif
#ifdef ST_OSWINCE
    int dummy; // Compilation Wince don't appreciate empty union in case of STAUD_INSTALL_MPEG & STAUD_INSTALL_AC3 not defined
#endif
}AudioSpecificParams_t;
#endif
typedef struct PESBlock_s
{
    U32     StartPointer;
    U32     ValidSize;
}PESBlock_t;

typedef struct PESESInitParams_s
{
    /*PES Buffer Params*/
    U32                          PESbufferSize;
    STMEMBLK_Handle_t            IpBufHandle;
    STMEMBLK_Handle_t            OpBufHandle;
    STAVMEM_PartitionHandle_t    AVMEMPartition;
    STAVMEM_PartitionHandle_t    AudioBufferPartition;
    ST_Partition_t               *CPUPartition_p;
    ST_DeviceName_t              EvtHandlerName;
    ST_DeviceName_t              EvtSkipName;
    ST_DeviceName_t              Name;
    ST_DeviceName_t              EvtGeneratorName;
    U32                          EvtSkipId;
    STAUD_Object_t               ObjectID;
    BOOL                         EnableMSPPSupport;
    BOOL                         MixerEnabled;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t             CLKRV_Handle;
    STCLKRV_Handle_t             CLKRV_HandleForSynchro;
#endif
    U32                          SamplingFrequency;

}PESESInitParams_t;

/* Output buffersand their params */
typedef struct LinerBufferParam_s
{
    /* Structure to Hold received Memory Block */
    MemoryBlock_t   *OpBlk_p;       /*  Linear output Buffer */
    BOOL            Valid;

}LinerBufferParam_t;

/* input blocks list */
typedef struct PESESInBlocklist_s
{
    MemoryBlock_t   *InputBlock_p;  /* Input Block received */
    BufferState_t   BufferSatus;    /* Marks if this buffer still holds some valid data */
    U32             BytesSent;/* Marks the total bytes consumed from start of this input Block */
    U32             BytesFreed;
}PESESInBlocklist_t;

typedef struct PESES_ParserControl_s
{
    ST_DeviceName_t           Name;
    STPESES_Handle_t          Handle;
    //BOOL                      TerminateTask;
    STAVMEM_PartitionHandle_t AVMEMPartition;
    STAVMEM_PartitionHandle_t AudioBufferPartition;
    STAVMEM_BlockHandle_t     PESBufferHandle;
    ST_Partition_t            *CPUPartition_p;

    /* Buffer manager Handle */
    STMEMBLK_Handle_t         IpBufHandle;
    STMEMBLK_Handle_t         OpBufHandle;
#ifndef STAUD_REMOVE_EMBX
    EMBX_TRANSPORT            tp;
#endif
#ifndef ST_51XX

    AudioSpecificParams_t     AudioParams;
#endif
    U32                       Blk_in;
    U32                       Blk_in_Next;
    U32                       Blk_out;

    LinerBufferParam_t        LinearQueue[NUM_NODES_PARSER];

    /*States and params for specific audio type*/

    STAUD_StreamInfo_t        StreamInfo;
    STAUD_BroadcastProfile_t  BroadcastProfile;

    U32                       ESValidBytes;   /*ES valid bytes*/
    U32                       PTS33Bit;
    U32                       PTS;                /*PTS*/
    U32                       PTDTSFlags;     /*PTSDTS Flag*/
    U8                        PTSValidFlag;   /*PTS Valid flag, used as BOOL*/
    U8                        Flags1;
    U8                        Flags2;
    U8                        *src;               /*pointer to write the parsed liniarized frame*/

    /*Parser function*/
    Parse_FrameFunc_t         Parse_Frame_fp;
    Parse_StopFunc_t          Parse_StopFunc_fp;
    Parse_StartFunc_t         Parse_StartFunc_fp;

    Timer_t                   PESESCurrentState;  /*Current state of Parser*/
    Timer_t                   PESESNextState;     /*Next state of Parser*/
    ParserInputState_t        PESESInputState;
    U32                       SkipDurationInMs;
    U32                       PTIBufferUnderflowCount;

    /*PES Buffer Params*/
    U32                       *PESbufferAddress_p;

    U8                     StreamID;
    /*Read/write points to manage the PES buffer*/
    U8                     * PTIBasePointer_p;
    U8                     * PTITopPointer_p;
    U8                     * PreviousPTIWritePointer_p;
    U8                     * CurrentPTIWritePointer_p;
    U8                     * PTIReadPointer_p;
    U8                     * PreviousPTIReadPointer_p;
    int                    PointerDifference;
    U32                    PESFilled;
    U32                    sentForParsing;
    U32                    BytesUsed;
    U32                    BitBufferSize;
    U32                    BitBufferThreshold;
    HALAUD_PTIInterface_t  PTIDMAReadWritePointer ;

    /*Ticks per millisec in ticks*/
    U32                       NumberOfTicksPerSecond;
    U32                       NumberOfTicksPermSecond;
    /*BroadCast Profile*/
    STAUD_BroadcastProfile_t  BroadCastProfile;
    PESBlock_t                PESBlock;
    S32                       Speed;
    /* Parser Task */
    task_t                    *PESESTask;

    /* Parameters for generating the event */
    U8                        LowDataLevel;
    U8                        EnableMSPPSupport; /*Used as BOOL*/
    U8                        EOFFlag; /*Used as BOOL*/
    U8                        AVSyncEnable; /*Used as BOOL*/
#if defined (ST_OS20)
    U8                        *PESESTaskstack;
    tdesc_t                   PESESTaskDesc;
#endif
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t          CLKRV_Handle;
    STCLKRV_Handle_t          CLKRV_HandleForSynchro;
#endif
    U32                       SamplingFrequency;
    STAUD_StreamType_t        StreamType;
    /*fdma channel for memcpy*/
#if ((defined STAUD_INSTALL_AIFF) || (defined STAUD_INSTALL_WAVE) || (defined PES_TO_ES_BY_FDMA))
    STFDMA_ChannelId_t              PESToESChannelID;
    STFDMA_GenericNode_t            *NodePESToES_p;
    STAVMEM_BlockHandle_t           PESToESBufferHandle;
    STFDMA_TransferGenericParams_t  PESTOESTransferParams;
    STFDMA_TransferId_t             PESTOESTransferID;
    STFDMA_Block_t                  FDMABlock;              /* FDMA Silicon used */
#endif
    STOS_Mutex_t                    *Lock_p;
    semaphore_t                     *SemWait_p;
    semaphore_t                     *SemComplete_p;
    STEVT_Handle_t                  EvtHandle;
    STEVT_DeviceSubscribeParams_t   EVT_SubcribeParams;
    ST_DeviceName_t                 EvtHandlerName;
    ST_DeviceName_t                 EvtSkipName;
    ST_DeviceName_t                 EvtGeneratorName;
    STAUD_Object_t                  ObjectID;
    U32                             EvtSkipId;
#if defined MSPP_PARSER
    MsppControlBlock_t              *Mspp_Cntrl_p;
    /* Structure to Hold received Memory Block */
#else
    //DVDV+DVDA+PACKMP1/MP2
    Com_ParserData_t                *ComParser_p;
#endif
    MemoryBlock_t                   *CurrentIPBlock_p;  /* Input Block received */
    PESESInBlocklist_t              IPBlkFrmProducer[PES_ES_NUM_IP_BUFFERS];
    U32                             IPQueueTail;
    U32                             IPQueueFront;
    U32                             IPQueueSent;
    // Events
    STEVT_EventID_t EventIDNewFrame;
    STEVT_EventID_t EventIDLowDataLevel;
    STEVT_EventID_t EventIDDataUnderFlow;
    STEVT_EventID_t EventIDFrequencyChange;
    STEVT_EventID_t EventIDPrepared;
    STEVT_EventID_t EventIDNewRouting;
    STEVT_EventID_t EventIDDataOverFlow;
    U8              LowLevelDataEvent;

#if ((defined STAUD_INSTALL_AIFF) || (defined STAUD_INSTALL_WAVE))
    U8 *                    DummyBufferBaseAddress_p;
    U32                     DummyBufferSize;
    STAVMEM_BlockHandle_t   DummyBufferHandle;
#endif

    struct PESES_ParserControl_s *Next_p;
}PESES_ParserControl_t;


ST_ErrorCode_t PESES_Init(ST_DeviceName_t  Name, PESESInitParams_t *InitParams_p, STPESES_Handle_t *Handle);
ST_ErrorCode_t PESESSetAudioType(STPESES_Handle_t Handle,STAUD_StreamParams_t * StreamParams_p);
ST_ErrorCode_t PESES_SetBroadcastProfile(STPESES_Handle_t Handle, STAUD_BroadcastProfile_t BroadcastProfile);
ST_ErrorCode_t PESES_GetBroadcastProfile(STPESES_Handle_t Handle, STAUD_BroadcastProfile_t* BroadcastProfile_p);

void PESES_Parse_Entry(void *Params_p);

ParserState_t PES_ES_ParseState(STPESES_Handle_t Handle);
ST_ErrorCode_t SendPES_ES_ParseCmd(STPESES_Handle_t Handle, ParserState_t State);
ST_ErrorCode_t PES_ES_ParseDeallocate(STPESES_Handle_t Handle,U32 i);
ST_ErrorCode_t PTS_PCROffsetInms(STPESES_Handle_t Handle,U32 CurrentInputPTS);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t PESES_SetClkSynchro(STPESES_Handle_t Handle,STCLKRV_Handle_t Clksource);
#endif
ST_ErrorCode_t PESES_AVSyncCmd(STPESES_Handle_t Handle,BOOL AVSyncEnable);
ST_ErrorCode_t PESES_SubscribeEvents(PESESInitParams_t *InitParams_p,PESES_ParserControl_t *ControlBlock);
ST_ErrorCode_t PESES_RegisterEvents(ST_DeviceName_t EventHandlerName, ST_DeviceName_t AudioDevice,PESES_ParserControl_t *ControlBlock);
ST_ErrorCode_t PESES_UnSubscribeEvents(PESES_ParserControl_t *ControlBlock_p);
void PESES_Handle_AvSync_Evt(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);
ST_ErrorCode_t PESES_GetInputCapability(STPESES_Handle_t Handle, STAUD_IPCapability_t * Capability_p);

ST_ErrorCode_t PESES_GetStreamInfo(STPESES_Handle_t Handle,STAUD_StreamInfo_t * StreamInfo_p);
ST_ErrorCode_t PESES_GetSamplingFreq(STPESES_Handle_t Handle,U32 *SamplingFrequency_p);
ST_ErrorCode_t PESES_SetStreamID(STPESES_Handle_t Handle,U32 StreamID);
ST_ErrorCode_t PESES_GetInputBufferParams(STPESES_Handle_t Handle,STAUD_BufferParams_t* DataParams_p);
ST_ErrorCode_t PESES_SetDataInputInterface(STPESES_Handle_t Handle,
                                                         GetWriteAddress_t  GetWriteAddress_p,
                                                         InformReadAddress_t InformReadAddress_p, void * const Handle_p);
ST_ErrorCode_t PESES_SetBitBufferLevel(STPESES_Handle_t Handle, U32 Level);
ST_ErrorCode_t PESES_SetPCMParams(STPESES_Handle_t Handle, U32 Frequency, U32 SampleSize, U32 Nch);
ST_ErrorCode_t PESES_EnableEOFHandling(STPESES_Handle_t Handle);
ST_ErrorCode_t PESES_SetSpeed(STPESES_Handle_t Handle, S32 Speed);
ST_ErrorCode_t PESES_GetSpeed(STPESES_Handle_t Handle, S32 *Speed_p);
ST_ErrorCode_t PESES_SetWMAStreamID(STPESES_Handle_t Handle, U8 WMAStreamNumber);
ST_ErrorCode_t PESES_GetBitBufferFreeSize(STPESES_Handle_t Handle, U32 * Size_p);
ST_ErrorCode_t PESES_Term(STPESES_Handle_t Handle);
//#if defined MSPP_PARSER
ST_ErrorCode_t PESES_BytesConsumed(U32 ControlBlockhandle, U32 BytesConsumed);
//#endif
ST_ErrorCode_t PESES_GetSynchroUnit(STPESES_Handle_t Handle,STAUD_SynchroUnit_t *SynchroUnit_p);
ST_ErrorCode_t PESES_SkipSynchro(STPESES_Handle_t Handle, U32 Delay);
ST_ErrorCode_t PESES_PauseSynchro(STPESES_Handle_t Handle, U32 Delay);
#endif /*_AUD_PES_ES_PARSER*/

