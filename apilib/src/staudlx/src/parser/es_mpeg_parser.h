/*******************************************************

 File Name: ES_MPEG_Parser.h

 Description: Header File for MPEG Parser

 Copyright: STMicroelectronics (c) 2005

*******************************************************/
#ifndef _MPEG_PARSER_H
#define _MPEG_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"
#ifdef PES_TO_ES_BY_FDMA
#include "aud_utils.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

/* Constants ---------------------------------------- */

typedef enum
{
    MPAUDLAYER_AAC,
    MPAUDLAYER_3,
    MPAUDLAYER_2,
    MPAUDLAYER_1
}MPEGAudioLayer;

typedef enum
{
    ES_MPEG_SYNC_0,
    ES_MPEG_SYNC_1,
    ES_MPEG_PARSE_HDR2_SYNC_FRAME,
    ES_MPEG_PARSE_HDR2,
    ES_MPEG_PARSE_HDR3,
    ES_MPEG_PARSE_CHECKHEADER,
    ES_MPEG_AAC_HDR5,
    ES_MPEG_SET_VALID_FRAME,
    ES_MPEG_FRAME,
    ES_MPEG_PARTIAL_FRAME,
    ES_MPEG_EXT_SYNC_0,
    ES_MPEG_EXT_SYNC_1,
    ES_MPEG_EXT_HDR_1,
    ES_MPEG_EXT_HDR_2,
    ES_MPEG_EXT_HDR_3,
    ES_MPEG_EXT_FRAME,
    ES_MPEG_PARTIAL_EXT_FRAME,
    ES_MPEG_FULL_FRAME,
    ES_MPEG_CHECK_PADDING,
    ES_MPEG_CHECK_PADDING_1,
    ES_MPEG_CHECK_PADDING_2,
    ES_MPEG_CHECK_PADDING_3,
    ES_MPEG_CHECK_HEADER_0,
    ES_MPEG_CHECK_HEADER_1,
    ES_MPEG_CHECK_HEADER_2,
    ES_MPEG_CHECK_HEADER


} ES_MPEG_ParserState_t;



/* Structures */


typedef struct
{
    U8                     HDR[6];
    U8                     TMP[8];
    U8                     PRE_HDR[4];
    U8                     PADD[4];
    U8                     PADDING_CHECK_BYTE;
    U8                     Layer;
    U8                     Current_Layer;
    U8                     Mode;
    U8                     Emphasis;
    U8                     PADDING_BYTE;
    U8                     StartFramePTS33;
    U8                     StartFramePTSValid;      /*use as BOOL*/
    U8                     Reuse;                   /*use as BOOL*/
    U8                     Get_Block;               /*use as BOOL*/
    U8                     Put_Block;               /*use as BOOL*/
    U8                     FrameComplete;           /*use as BOOL*/
    U8                     First_Frame;             /*use as BOOL*/
    BOOL                   PADDING;                 /*use as BOOL*/
    BOOL                   PADDING_CHECK_BYTE_8;    /*use as BOOL*/
    BOOL                   PADDING_CHECK;           /*use as BOOL*/
    BOOL                   ExtFrame;                /*use as BOOL*/
    BOOL                   NIBBLE_ALIGNED;          /*use as BOOL*/
    BOOL                   PADDING_BIT;             /*use as BOOL*/
    BOOL                   AAC;                     /*use as BOOL*/
    BOOL                   SYNC;                    /*use as BOOL*/
    U16                    ExtFrameSize;
    U16                    NoBytesCopied;
    U16                    NoBytesRemainingInFrame;
    U32                    FrameSize;
    U32                    Frequency;
    U32                    Current_Frequency;
    U32                    Skip;
    U32                    Pause;
    U32                    StartFramePTS;
    U32                    BitRate;
    U32                    Blk_Out;
    U32                    Odd_Byte;
    U8                     * Current_Write_Ptr1;
    ES_MPEG_ParserState_t MPEGParserState;
    ST_Partition_t         * AudioParserMemory_p;
    STMEMBLK_Handle_t      OpBufHandle;
    MemoryBlock_t          * OpBlk_p;
    STEVT_Handle_t         EvtHandle;
    STAUD_Object_t         ObjectID;
    STEVT_EventID_t        EventIDNewFrame;
    STEVT_EventID_t        EventIDFrequencyChange;
    STEVT_EventID_t        EventIDNewRouting;
    STAUD_StreamContent_t  StreamContent;
#ifdef PES_TO_ES_BY_FDMA
    STFDMA_GenericNode_t   * NodePESToES_p;
#endif
} ES_MPEG_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_MPEG_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_MPEG_Parse_Frame(ParserHandle_t const,void *, U32,U32 *,PTS_t *, Private_Info_t *);

ST_ErrorCode_t ES_MPEG_Parser_Deallocate(ParserHandle_t const );

ST_ErrorCode_t ES_MPEG_Parser_Term(ParserHandle_t *const handle, ST_Partition_t *CPUPartition_p);

ST_ErrorCode_t ES_MPEG_ParserGetParsingFunction( ParsingFunction_t   * ParsingFucntion_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p);

ST_ErrorCode_t ES_MPEG_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_MPEG_Parser_Start(ParserHandle_t const Handle);


ST_ErrorCode_t ES_MPEG_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);

ST_ErrorCode_t ES_MPEG_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_MPEG_Handle_EOF(ParserHandle_t  Handle);

ST_ErrorCode_t ES_MPEG_GetSynchroUnit(ParserHandle_t  Handle,STAUD_SynchroUnit_t *SynchroUnit_p);
ST_ErrorCode_t ES_MPEG_SetStreamContent(ParserHandle_t  Handle,STAUD_StreamContent_t StreamContent);
U32                 ES_MPEG_GetRemainTime(ParserHandle_t  Handle);
#ifdef __cplusplus
}
#endif



#endif /* _MPEG_PARSER_H */

