/*******************************************************

 File Name: ES_LPCMA_Parser.h

 Description: Header File for LPCMA Parser

 Copyright: STMicroelectronics (c) 2005

*******************************************************/
#ifndef _LPCMA_PARSER_H
#define _LPCMA_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif



/* Constants ---------------------------------------- */



typedef enum
{
    ES_LPCMA_COMPUTE_FRAME_LENGTH,
    ES_LPCMA_FRAME,
    ES_LPCMA_PARTIAL_FRAME

} ES_LPCMA_ParserState_t;



/* Structures */


typedef struct
{
    U8                     FrameComplete;/*Used as BOOL*/
    U8                     First_Frame;  /*Used as BOOL*/
    U8                     Get_Block;    /*Used as BOOL*/
    U8                     Put_Block;    /*Used as BOOL*/
    U8                     AppDataFlag;  /*Used as BOOL*/
    U16                    NoBytesRemainingInFrame;
    U32                    FrameSize;
    U32                    Frequency;
    ES_LPCMA_ParserState_t LPCMAParserState;
    ST_Partition_t         * AudioParserMemory_p;
    U8                     * Current_Write_Ptr1;
    STMEMBLK_Handle_t      OpBufHandle;
    MemoryBlock_t          * OpBlk_p;
    STAud_LpcmStreamParams_t * AppData_p[2];
    STEVT_Handle_t           EvtHandle;
    STAUD_Object_t           ObjectID;
    STEVT_EventID_t          EventIDNewFrame;
    STEVT_EventID_t          EventIDFrequencyChange;
} ES_LPCMA_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_LPCMA_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_LPCMA_Parse_Frame(ParserHandle_t const,void *, U32,U32 *,PTS_t *, Private_Info_t *);

ST_ErrorCode_t ES_LPCMA_Parser_Deallocate(ParserHandle_t const );

ST_ErrorCode_t ES_LPCMA_Parser_Term(ParserHandle_t *const handle, ST_Partition_t    *CPUPartition_p);

ST_ErrorCode_t ES_LPCMA_ParserGetParsingFunction(ParsingFunction_t   * ParsingFucntion_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p);
ST_ErrorCode_t ES_LPCMA_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_LPCMA_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_LPCMA_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);

ST_ErrorCode_t ES_LPCMA_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_LPCMA_Handle_EOF(ParserHandle_t  Handle);

#ifdef __cplusplus
}
#endif



#endif /* _LPCMA_PARSER_H */

