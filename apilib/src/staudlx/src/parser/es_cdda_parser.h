/*******************************************************

 File Name: ES_CDDA_Parser.h

 Description: Header File for CDDA Parser

 Copyright: STMicroelectronics (c) 2005

*******************************************************/
#ifndef _CDDA_PARSER_H
#define _CDDA_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants ---------------------------------------- */
typedef enum
{
    ES_CDDA_COMPUTE_FRAME_LENGTH,
    ES_CDDA_FRAME,
    ES_CDDA_PARTIAL_FRAME

} ES_CDDA_ParserState_t;

/* Structures */

typedef struct
{
    U8   Nch;
    U8   WordSizeInBytes;
    U8   NewNch;
    U8   UpdatePcm;        /*Used as BOOL*/
    U8   FrameComplete;    /*Used as BOOL*/
    U8   First_Frame;      /*Used as BOOL*/
    U8   Get_Block;        /*Used as BOOL*/
    U8   Put_Block;        /*Used as BOOL*/
    U8   AppDataFlag;      /*Used as BOOL*/
    U16  NoBytesRemainingInFrame;
    U32  FrameSize;
    U32  Frequency;
    U32  SampleSize;
    U32  NewFrequency;
    U32  NewSampleSize;
    BOOL MixerEnabled;
    ES_CDDA_ParserState_t    CDDAParserState;
    ST_Partition_t           * AudioParserMemory_p;
    U8                       * Current_Write_Ptr1;
#if (defined PES_TO_ES_BY_FDMA)
    STFDMA_GenericNode_t     * node_p;
    STAud_Memory_Address_t   FDMAnodeAddress;
#endif
    STMEMBLK_Handle_t        OpBufHandle;
    MemoryBlock_t            * OpBlk_p;
    U32                      Blk_Out;
    STAud_LpcmStreamParams_t *AppData_p[2];
    STEVT_Handle_t           EvtHandle;
    STAUD_Object_t           ObjectID;
    STEVT_EventID_t          EventIDNewFrame;
    STEVT_EventID_t          EventIDFrequencyChange;
} ES_CDDA_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_CDDA_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_CDDA_Parse_Frame(ParserHandle_t const,void *, U32,U32 *,PTS_t *, Private_Info_t *);

ST_ErrorCode_t ES_CDDA_Parser_Deallocate(ParserHandle_t const );

ST_ErrorCode_t ES_CDDA_Parser_Term(ParserHandle_t *const handle, ST_Partition_t *CPUPartition_p);

ST_ErrorCode_t ES_CDDA_ParserGetParsingFunction(ParsingFunction_t * ParsingFucntion_p,
                                                    GetFreqFunction_t * GetFreqFunction_p,
                                                    GetInfoFunction_t * GetInfoFunction_p);

ST_ErrorCode_t ES_CDDA_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_CDDA_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_CDDA_Set_Pcm(ParserHandle_t const Handle,U32 Frequency, U32 SampleSize, U32 Nch);

ST_ErrorCode_t ES_CDDA_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);

ST_ErrorCode_t ES_CDDA_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_CDDA_Handle_EOF(ParserHandle_t  Handle);
#ifdef ST_51XX
void swap_input(U8 *pcmin,U8 *pcmout, U32 nbspl);
void Convert8bitTo16bit(U8 *pcmin,U8 *pcmout, U32 nbspl);
#endif
#ifdef __cplusplus
}
#endif



#endif /* _CDDA_PARSER_H */

