/********************************************************************************
 *   Filename       : es_aac_parser.h                                           *
 *   Start          : 19.01.2006                                                *
 *   By             : Rahul Bansal                                          *
 *   Contact        : rahul.bansal@st.com                                   *
 *   Description    : The file contains the parser for ADIF, MP4 file and RAW AAC *
 *                    and its APIs that will be exported to the Modules.        *
 ********************************************************************************
*/

#ifndef _AAC_PARSER_H
#define _AAC_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants ---------------------------------------- */



typedef enum
{
    ES_AAC_FRAME

} ES_AAC_ParserState_t;



/* Structures */


typedef struct
{
    U8                   FrameComplete;
    U8                   * Current_Write_Ptr1;
    U16                  NoBytesCopied;
    U32                  FrameSize;
    U32                  Frequency;
    ES_AAC_ParserState_t AACParserState;
    ST_Partition_t       * AudioParserMemory_p;
    STMEMBLK_Handle_t    OpBufHandle;
    MemoryBlock_t        * OpBlk_p;
    U32                  Blk_Out;
    U32                  Skip;
    U32                  Pause;
    STEVT_Handle_t       EvtHandle;
    STAUD_Object_t       ObjectID;
    STEVT_EventID_t      EventIDNewFrame;
    STEVT_EventID_t      EventIDFrequencyChange;
} ES_AAC_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_AAC_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_AAC_Parse_Frame(ParserHandle_t const,void *, U32,U32 *,PTS_t *, Private_Info_t *);


ST_ErrorCode_t ES_AAC_Parser_Deallocate(ParserHandle_t const );

ST_ErrorCode_t ES_AAC_Parser_Term(ParserHandle_t *const handle, ST_Partition_t  *CPUPartition_p);

ST_ErrorCode_t ES_AAC_ParserGetParsingFunction( ParsingFunction_t * ParsingFunction_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p);
ST_ErrorCode_t ES_AAC_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_AAC_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_AAC_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);
ST_ErrorCode_t ES_AAC_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_AAC_Handle_EOF(ParserHandle_t  Handle);

ST_ErrorCode_t  ES_AAC_GetSynchroUnit(ParserHandle_t  Handle,STAUD_SynchroUnit_t *SynchroUnit_p);

#ifdef __cplusplus
}
#endif



#endif /* _AAC_PARSER_H */


