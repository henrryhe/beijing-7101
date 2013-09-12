/*******************************************************

 File Name: ES_WMAPES_Parser.h

 Description: Header File for WMAPES Parser

 Copyright: STMicroelectronics (c) 2005

*******************************************************/
#ifndef _WMAPES_PARSER_H
#define _WMAPES_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif



/* Constants ---------------------------------------- */



typedef enum
{
    ES_WMAPES_PRIVATE_DATA_HEADER_A,
    ES_WMAPES_PRIVATE_DATA_HEADER_S,
    ES_WMAPES_PRIVATE_DATA_HEADER_F,
    ES_WMAPES_PRIVATE_DATA_HEADER_LENGTH,
    ES_WMAPES_PARSE_ASF_HEADER_nVersion_0,
    ES_WMAPES_PARSE_ASF_HEADER_nVersion_1,
    ES_WMAPES_PARSE_ASF_HEADER_wFormatTag_0,
    ES_WMAPES_PARSE_ASF_HEADER_wFormatTag_1,
    ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_0,
    ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_1,
    ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_2,
    ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_3,
    ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_0,
    ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_1,
    ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_2,
    ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_3,
    ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_0,
    ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_1,
    ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_2,
    ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_3,
    ES_WMAPES_PARSE_ASF_HEADER_nChannels_0,
    ES_WMAPES_PARSE_ASF_HEADER_nChannels_1,
    ES_WMAPES_PARSE_ASF_HEADER_nEncodeOpt_0,
    ES_WMAPES_PARSE_ASF_HEADER_nEncodeOpt_1,
    ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_0,
    ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_1,
    ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_2,
    ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_3,
    ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_0,
    ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_1,
    ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_2,
    ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_3,
    ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_0,
    ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_1,
    ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_2,
    ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_3,
    ES_WMAPES_PARSE_ASF_HEADER_wValidBitsPerSample_0,
    ES_WMAPES_PARSE_ASF_HEADER_wValidBitsPerSample_1,
    ES_WMAPES_PARSE_ASF_HEADER_wStreamId_0,
    ES_WMAPES_PARSE_ASF_HEADER_wStreamId_1,
    ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_0,
    ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_1,
    ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_2,
    ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_3,
    ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_0,
    ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_1,
    ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_2,
    ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_3,
    ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_0,
    ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_1,
    ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_2,
    ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_3,
    ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_0,
    ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_1,
    ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_2,
    ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_3,
    ES_WMAPES_PARSE_ASF_HEADER_SET_IN_DATA_BLOCK,
    ES_WMAPES_DELIVER_PAYLOAD_DATA,
    ES_WMAPES_PARTIAL_FRAME

} ES_WMAPES_ParserState_t;



/* Structures */

typedef struct
{
    U8                      FrameComplete;
    U8                      First_Frame;
    U8                      Get_Block;     /*Use as BOOL*/
    U8                      Put_Block;     /*Use as BOOL*/
    U8                      * Current_Write_Ptr1;
    STMEMBLK_Handle_t       OpBufHandle;
    MemoryBlock_t           * OpBlk_p;
    U32                     FrameSize;
    U32                     Frequency;
    U16                     NoBytesRemainingInFrame;
    U16                     nVersion;
    U16                     wFormatTag;
    U32                     nSamplesPerSec;
    U32                     nAvgBytesPerSec;
    U32                     nBlockAlign;
    U16                     nChannels;
    U16                     nEncodeOpt;
    U32                     nSamplesPerBlock;
    U32                     dwChannelMask;
    U32                     nBitsPerSample;
    U16                     wValidBitsPerSample;
    U16                     wStreamId;
    U32                     iPeakAmplitudeRef;
    U32                     iRmsAmplitudeRef;
    U32                     iPeakAmplitudeTarget;
    U32                     iRmsAmplitudeTarget;
    U16                     local_nVersion;
    U16                     local_wFormatTag;
    U32                     local_nSamplesPerSec;
    U32                     local_nAvgBytesPerSec;
    U32                     local_nBlockAlign;
    U16                     local_nChannels;
    U16                     local_nEncodeOpt;
    U32                     local_nSamplesPerBlock;
    U32                     local_dwChannelMask;
    U32                     local_nBitsPerSample;
    U16                     local_wValidBitsPerSample;
    U16                     local_wStreamId;
    U32                     local_iPeakAmplitudeRef;
    U32                     local_iRmsAmplitudeRef;
    U32                     local_iPeakAmplitudeTarget;
    U32                     local_iRmsAmplitudeTarget;
    ES_WMAPES_ParserState_t WMAPESParserState;
    ST_Partition_t          * AudioParserMemory_p;
    STEVT_Handle_t          EvtHandle;
    STAUD_Object_t          ObjectID;
    STEVT_EventID_t         EventIDNewFrame;
} ES_WMAPES_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_WMAPES_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_WMAPES_Parse_Frame(ParserHandle_t const,void *, U32,U32 *,PTS_t *, Private_Info_t *);

ST_ErrorCode_t ES_WMAPES_Parser_Deallocate(ParserHandle_t const );

ST_ErrorCode_t ES_WMAPES_Parser_Term(ParserHandle_t *const handle, ST_Partition_t   *CPUPartition_p);

ST_ErrorCode_t ES_WMAPES_ParserGetParsingFunction(  ParsingFunction_t   * ParsingFucntion_p,
                                                    GetFreqFunction_t * GetFreqFunction_p,
                                                    GetInfoFunction_t * GetInfoFunction_p);

ST_ErrorCode_t ES_WMAPES_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_WMAPES_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_WMAPES_Set_Pcm(ParserHandle_t const Handle,U32 Frequency, U32 SampleSize, U32 Nch);

ST_ErrorCode_t ES_WMAPES_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);

ST_ErrorCode_t ES_WMAPES_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_WMAPES_Handle_EOF(ParserHandle_t  Handle);
#ifdef __cplusplus
}
#endif



#endif /* _WMAPES_PARSER_H */

