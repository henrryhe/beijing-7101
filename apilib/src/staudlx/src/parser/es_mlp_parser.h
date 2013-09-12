/*******************************************************

 File Name: ES_MLP_Parser.h

 Description: Header File for MLP Parser

 Copyright: STMicroelectronics (c) 2005

*******************************************************/
#ifndef _MLP_PARSER_H
#define _MLP_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants ---------------------------------------- */
typedef enum
{
    ES_MLP_START_FRAME,
    ES_MLP_BYTE_1,
    ES_MLP_BYTE_2,
    ES_MLP_BYTE_3,
    ES_MLP_BYTE_4,
    ES_MLP_BYTE_5,
    ES_MLP_BYTE_6,
    ES_MLP_BYTE_7,
    ES_MLP_BYTE_8,
    ES_MLP_CHECK_SYNC,
    ES_MLP_MAJOR_SYNC_INFO_0,
    ES_MLP_MAJOR_SYNC_INFO_1,
    ES_MLP_MAJOR_SYNC_INFO_2,
    ES_MLP_MAJOR_SYNC_INFO_3,
    ES_MLP_MAJOR_SYNC_INFO,
    ES_MLP_WRITE_SYNC,
    ES_MLP_FRAME,
    ES_MLP_PARTIAL_FRAME

} ES_MLP_ParserState_t;



/* Structures */


typedef struct
{
    U8                   Data[12];
    U8                   SerialAccessUnits;
    U8                   NbAccessUnits;
    U8                   Check_Major_Sync;  /*Used as BOOL*/
    U8                   Get_Block;         /*Used as BOOL*/
    U8                   Put_Block;         /*Used as BOOL*/
    U8                   FrameComplete;     /*Used as BOOL*/
    U8                   First_Frame;       /*Used as BOOL*/
    U16                  NoBytesCopied;
    U16                  NoBytesRemainingInFrame;
    U32                  FrameSize;
    U32                  Frequency;
    U32                  Odd_Byte;
    U32                  Access_Unit_Size;
    U32                  Blk_Out;
    U8                   * Current_Write_Ptr1;
    ES_MLP_ParserState_t MLPParserState;
    ST_Partition_t       * AudioParserMemory_p;
    STMEMBLK_Handle_t    OpBufHandle;
    MemoryBlock_t        * OpBlk_p;
    STEVT_Handle_t       EvtHandle;
    STAUD_Object_t       ObjectID;
    STEVT_EventID_t      EventIDNewFrame;
    STEVT_EventID_t      EventIDFrequencyChange;
} ES_MLP_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_MLP_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_MLP_Parse_Frame(ParserHandle_t const,void *, U32,U32 *,PTS_t *, Private_Info_t *);

ST_ErrorCode_t ES_MLP_Parser_Deallocate(ParserHandle_t const );

ST_ErrorCode_t ES_MLP_Parser_Term(ParserHandle_t *const handle, ST_Partition_t  *CPUPartition_p);

ST_ErrorCode_t ES_MLP_ParserGetParsingFunction( ParsingFunction_t   * ParsingFunc_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p);

ST_ErrorCode_t ES_MLP_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_MLP_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_MLP_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);

ST_ErrorCode_t ES_MLP_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_MLP_Handle_EOF(ParserHandle_t  Handle);

#ifdef __cplusplus
}
#endif



#endif /* _MLP_PARSER_H */

