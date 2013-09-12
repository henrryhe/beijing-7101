
#ifndef PARSER_COMMON_H_
#define PARSER_COMMON_H_

#include "staudlx.h"
#include "blockmanager.h"
#ifndef ST_51XX
#include "aud_mmeaudiostreamdecoder.h"
#else
#include "aud_utils.h"
#include "aud_defines.h"
#endif
#include "stos.h"
#include "staud.h"


typedef U32 ParserHandle_t;
typedef U32 PESParser_Handle_t;
typedef U32 ComParser_Handle_t;

//#define MAX_NUM_PARSER 11
//#define MAX_NUM_PES_PARSER 4

#define LPCM_DEFAULT_CHANNEL_ASSIGNMENT 0xFF
#define PRIVATE_DATA_SIZE 15
enum eES_Parser
{
    ES_MPEG,
    ES_AC3,
    ES_LPCMV,
    ES_DTS,
    ES_LPCMA,
    ES_MLP,
    ES_CDDA,
    ES_WMA,
    ES_WMA_PES,
    ES_AAC,
    ES_WAVE,
    ES_AIFF,
    /*Do Not Add anything beyond this line*/
    MAX_NUM_PARSER
};

enum ePES_Parser
{
    PES_PACKMP1,
    PES_MP2,
    PES_DVDV,
    PES_DVDA,
    /*Do Not Add anything beyond this line*/
    MAX_NUM_PES_PARSER
};

typedef struct
{
    U32 PTS33Bit;
    U32 PTS;
    U32 PTSDTSFlags;
    U8 PTSValidFlag;
}PTS_t;

typedef struct
{
    U8  Private_Data[PRIVATE_DATA_SIZE];
    U8  NbFrameHeaders;
    U16 FirstAccessUnitPtr;
    U32 Private_LPCM_Data[2];
}Private_Info_t;

typedef int (*ParsingFunction_t)(ParserHandle_t const,void *,U32,U32*,PTS_t*,Private_Info_t*);
typedef int (*GetFreqFunction_t)(ParserHandle_t const,U32*);
typedef int (*GetInfoFunction_t)(ParserHandle_t const,STAUD_StreamInfo_t *);
typedef int (*PESParsingFunction_t)(PESParser_Handle_t const,void *,U32,U32*,ParsingFunction_t,ParserHandle_t);
typedef int (*ESStartFunction_t)(ParserHandle_t const);
typedef int (*ESStopFunction_t)(ParserHandle_t const);
typedef int (*PESStartFunction_t)(PESParser_Handle_t const);
typedef int (*PESStopFunction_t)(PESParser_Handle_t const);
typedef int (*ESEOFFunction_t)(PESParser_Handle_t const);
typedef struct
{
    U8                        AVSyncEnable;
    ST_Partition_t            *Memory_Partition;
    STAVMEM_PartitionHandle_t AudioBufferPartition;
    STAVMEM_PartitionHandle_t AVMEMPartition;
#if defined (PES_TO_ES_BY_FDMA) || defined (STAUD_INSTALL_WAVE) || defined(STAUD_INSTALL_AIFF)
    STFDMA_GenericNode_t      *NodePESToES_p;
#endif
    ComParser_Handle_t        *Handle;
    STMEMBLK_Handle_t         OpBufHandle;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t          CLKRV_HandleForSynchro;
#endif
    STEVT_Handle_t            EvtHandle;
    STAUD_Object_t            ObjectID;
    U8 *                      DummyBufferBaseAddress;
    STEVT_EventID_t           EventIDNewFrame;
    STEVT_EventID_t           EventIDFrequencyChange;
    STEVT_EventID_t           EventIDNewRouting;
    BOOL                      MixerEnabled;

}ComParserInit_t;


typedef struct
{
    PESParser_Handle_t   PESParserHandle[MAX_NUM_PES_PARSER];
    PESParsingFunction_t PESParseFunction[MAX_NUM_PES_PARSER];

    ParserHandle_t       ESParserHandle[MAX_NUM_PARSER];
    ParsingFunction_t    ESParseFunction[MAX_NUM_PARSER];
    GetFreqFunction_t    ESGetFreqFunction[MAX_NUM_PARSER];
    GetInfoFunction_t    ESGetInfoFunction[MAX_NUM_PARSER];

    ESStartFunction_t    ESStartFunction;
    ESStopFunction_t     ESStopFunction;
    PESStartFunction_t   PESStartFunction;
    PESStopFunction_t    PESStopFunction;
    ESEOFFunction_t      ESEOFFunction;
    PESParsingFunction_t CurPESParseFunction;
    U16                  CurESParser;
    U16                  CurPESParser;

    STAUD_StreamContent_t StreamContent;
    STAUD_StreamType_t    StreamType;

} Com_ParserData_t;

ST_ErrorCode_t Com_Parser_Init(ComParserInit_t *Init_p, ComParser_Handle_t *Handle);
ST_ErrorCode_t Com_Parser_SetStreamType(ComParser_Handle_t  Handle,STAUD_StreamType_t StreamType,   STAUD_StreamContent_t StreamContents);
ST_ErrorCode_t Com_Parser_SetBroadcastProfile(ComParser_Handle_t  Handle,STAUD_BroadcastProfile_t BroadcastProfile);
ST_ErrorCode_t Com_Parser_SetStreamID(ComParser_Handle_t  Handle, U32 StreamID);
ST_ErrorCode_t Com_Parser_SetPCMParams(ComParser_Handle_t  Handle,U32 Frequency, U32 SampleSize, U32 Nch);
ST_ErrorCode_t Com_Parser_GetInfo(ComParser_Handle_t  Handle,STAUD_StreamInfo_t * StreamInfo_p);
ST_ErrorCode_t Com_Parser_GetFreq(ComParser_Handle_t  Handle,U32 *SamplingFrequency_p);
ST_ErrorCode_t Com_Parser_HandleEOF(ComParser_Handle_t  Handle);
ST_ErrorCode_t Com_Parser_SetWMAStreamID(ComParser_Handle_t  Handle, U8 WMAStreamNumber);
ST_ErrorCode_t Com_Parser_SetSpeed(ComParser_Handle_t  Handle,S32 Speed);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t Com_Parser_SetClkSynchro(ComParser_Handle_t  Handle,STCLKRV_Handle_t Clksource);
#endif // STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t Com_Parser_AVSyncCmd(ComParser_Handle_t  Handle,BOOL AVSyncEnable);

ST_ErrorCode_t Com_Parser_GetSynchroUnit(ComParser_Handle_t Handle,STAUD_SynchroUnit_t *SynchroUnit_p);
ST_ErrorCode_t Com_Parser_SkipSynchro(ComParser_Handle_t Handle, U32 Delay);
ST_ErrorCode_t Com_Parser_PauseSynchro(ComParser_Handle_t Handle, U32 Delay);
U32 Com_Parser_GetRemainTime(ComParser_Handle_t Handle);

#endif //PARSER_COMMON_H_

