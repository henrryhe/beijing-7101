/*******************************************************

 File Name: ES_AC3_Parser.h

 Description: Header File for AC3 Parser

 Copyright: STMicroelectronics (c) 2005

*******************************************************/
#ifndef _AC3_PARSER_H
#define _AC3_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif



/* Constants ---------------------------------------- */



typedef enum
{
	ES_AC3_SYNC_0,
	ES_AC3_SYNC_1,
	ES_AC3_HDR_0,
	ES_AC3_HDR_1,
	ES_AC3_HDR_2,
	ES_AC3_HDR_3,
	ES_AC3_CHECK_HDR,
	ES_AC3_FSCOD,
	ES_AC3_FRAME

} ES_AC3_ParserState_t;



/* Structures */


typedef struct
{
	U32 FrameSize;
	U32 Frequency;
	U16 NoBytesCopied;
	U16 NoBytesRemainingInFrame;
	U8 HDR[4];
	U8 Bit_Rate_Code;
	U8 Bit_Stream_Mode;
	U8 Audio_Coding_Mode;
	ES_AC3_ParserState_t AC3ParserState;
	ST_Partition_t * AudioParserMemory_p;
	BOOL FrameComplete;
	U32 Odd_Byte;
	BOOL First_Frame;

	BOOL Get_Block;
	BOOL Put_Block;

	U8 * Current_Write_Ptr1;
	STMEMBLK_Handle_t	OpBufHandle;
	MemoryBlock_t	*OpBlk_p;
	U32 Blk_Out;
	U32 Skip;
	U32 Pause;
	BOOL Reuse;

	STEVT_Handle_t  EvtHandle;
	STAUD_Object_t ObjectID;
	STEVT_EventID_t EventIDNewFrame;
	STEVT_EventID_t EventIDFrequencyChange;
	STEVT_EventID_t EventIDNewRouting;
	U32 BitRate;

	U32  StartFramePTS;
	U8   StartFramePTS33;
	BOOL StartFramePTSValid;
} ES_AC3_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_AC3_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_AC3_Parse_Frame(ParserHandle_t const,void *, U32,U32 *,PTS_t *, Private_Info_t *);

ST_ErrorCode_t ES_AC3_Parser_Deallocate(ParserHandle_t const );

ST_ErrorCode_t ES_AC3_Parser_Term(ParserHandle_t *const handle, ST_Partition_t	*CPUPartition_p);

ST_ErrorCode_t ES_AC3_ParserGetParsingFunction(	ParsingFunction_t * ParsingFunction_p,
												GetFreqFunction_t * GetFreqFunction_p,
												GetInfoFunction_t * GetInfoFunction_p);
ST_ErrorCode_t ES_AC3_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_AC3_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_AC3_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);
ST_ErrorCode_t ES_AC3_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_AC3_Handle_EOF(ParserHandle_t  Handle);
ST_ErrorCode_t ES_AC3_GetSynchroUnit(ParserHandle_t  Handle,STAUD_SynchroUnit_t *SynchroUnit_p);
U32 				ES_AC3_GetRemainTime(ParserHandle_t  Handle);
#ifdef __cplusplus
}
#endif



#endif /* _AC3_PARSER_H */

