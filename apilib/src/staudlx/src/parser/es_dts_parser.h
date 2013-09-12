/*******************************************************

 File Name: ES_DTS_Parser.h

 Description: Header File for DTS Parser

 Copyright: STMicroelectronics (c) 2005

*******************************************************/
#ifndef _DTS_PARSER_H
#define _DTS_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DTS_SYNCWORD_CORE_24        0xfe80007f
#define DTS_SYNCWORD_CORE_16        0x7ffe8001
#define DTS_SYNCWORD_CORE_14        0x1fffe800
#define DTS_SYNCWORD_CORE_24M       0xfe7f0180
#define DTS_SYNCWORD_CORE_16M       0xfe7f0180
#define DTS_SYNCWORD_CORE_14M       0xff1f00e8
#define DTS_SYNCWORD_CORE_24EXT     0x3f
#define DTS_SYNCWORD_CORE_16EXT     0x3f
#define DTS_SYNCWORD_CORE_14EXT     0x07f

#define DTS_SYNCWORD_AUX        0x9A1105A0

#define DTS_SYNCWORD_XCH        0x5a5a5a5a
#define DTS_SYNCWORD_XXCH       0x47004a03
#define DTS_SYNCWORD_X96K       0x1d95f262
#define DTS_SYNCWORD_XBR        0x655e315e
#define DTS_SYNCWORD_LBR        0x0a801921

#define DTS_SYNCWORD_SUBSTREAM      0x64582025
#define DTS_SYNCWORD_SUBSTREAM_M    0x58642520

#define DTS_SYNCWORD_SUBSTREAM_CORE 0x02b09261
#define DTS_SYNCWORD_SUBSTREAM_CORE_M   0xb0026192

#define DTS_SYNCWORD_IEC61937       0xf8724e1f
#define DTS_SYNCWORD_IEC61937_Intel 0x72f81f4e

/* Constants ---------------------------------------- */



typedef enum
{
    ES_DTS_SYNC_0,
#ifdef PARSE_LOSS_LESS_HEADER
    ES_DTS_LOSS_LESS_HEADER_1,
#endif
    ES_DTS_AUPR_HDR,
    ES_DTS_AUPR_HDR1,
    ES_DTS_NEXT_TBD,
    ES_DTS_NEXT_TBD_1,
    ES_DTS_NEXT_TBD_2,
    ES_DTS_CHECK_SYNC_CASE2,
    ES_DTS_NEXT1_TBD,
    ES_DTS_NEXT2_TBD_CASE1_SYNC_LOCK,
    ES_DTS_IN_SYNC_CASE1,
    ES_DTS_WRITE_IN_FRAME_BUFFER,
    ES_DTS_PARTIAL_FRAME_WRITE_IN_FRAME_BUFFER,
    ES_DTS_PUT_OUTPUT,
    ES_DTS_FILL_TEMP_BUFF
} ES_DTS_ParserState_t;

typedef enum
{
    DTS_BITSTREAM_LOSSY_16      = 0x00000001,
    DTS_BITSTREAM_LOSSY_14      = 0x00000002,
    DTS_BITSTREAM_LOSSY_24      = 0x00000004,
    DTS_BITSTREAM_LOSSLESS_24   = 0x00000008,
    DTS_BITSTREAM_SUBSTREAM_32  = 0x00000010,
    DTS_BITSTREAM_HDMI_64       = 0x00000020
} DTSBitStreamE;

/* Structures */
typedef struct
{
    U8 ReadPos;
    U8 Size;
    U8 WritePos;
    U8 Buff[16];
}TempBuff_t;


typedef struct
{
    U8                   SubStreamIndex;
    U8                   SubStreamIndex_last;
    U8                   nByteSkip;
    U8                   FirstByteEncSamples;
    U8                   FrameComplete;    /*Used as BOOL*/
    U8                   First_Frame;    /*Used as BOOL*/
    U8                   Get_Block;      /*Used as BOOL*/
    U8                   Put_Block;      /*Used as BOOL*/
    U8                   InSync;         /*Used as BOOL*/
    U8                   CheckNextSubStreamSync;  /*Used as BOOL*/
    U8                   SubStream;              /*Used as BOOL*/
    U8                   AUPR_HDR;               /*Used as BOOL*/
    U16                  NoBytesCopied;
    U16                  NoBytesRemainingInFrame;
    U16                  DelayLossLess;
    U32                  FrameSize;
    U32                  Frequency;
    U32                  Odd_Byte;
    U32                  CoreBytes;
    U32                  SyncWord;
    U32                  SubStreamSize;
    U32                  nSubStreamSize;
    U32                  Last4ByteEncSamples;
    ES_DTS_ParserState_t DTSParserState;
    ES_DTS_ParserState_t DTSParserState_Current;
    ST_Partition_t       * AudioParserMemory_p;
    U8                   * Current_Write_Ptr1;
    STMEMBLK_Handle_t    OpBufHandle;
    MemoryBlock_t        * OpBlk_p;
    TempBuff_t           TempBuff;
    DTSBitStreamE        BitStreamType;
#ifndef ST_51XX
    STAud_DtsStreamParams_t *AppData_p;
#endif
    STEVT_Handle_t       EvtHandle;
    STAUD_Object_t       ObjectID;
    STEVT_EventID_t      EventIDNewFrame;
    STEVT_EventID_t      EventIDFrequencyChange;
} ES_DTS_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_DTS_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_DTS_Parse_Frame(ParserHandle_t const,void *, U32,U32 *,PTS_t *, Private_Info_t *);


ST_ErrorCode_t ES_DTS_Parser_Deallocate(ParserHandle_t const );

ST_ErrorCode_t ES_DTS_Parser_Term(ParserHandle_t *const handle, ST_Partition_t  *CPUPartition_p);

ST_ErrorCode_t ES_DTS_ParserGetParsingFunction(ParsingFunction_t  * ParsingFucntion_p,
                                                    GetFreqFunction_t * GetFreqFunction_p,
                                                    GetInfoFunction_t * GetInfoFunction_p);

ST_ErrorCode_t ES_DTS_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_DTS_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_DTS_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);
ST_ErrorCode_t ES_DTS_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_DTS_Handle_EOF(ParserHandle_t  Handle);

BOOL        DTSCheckSyncWord(ES_DTS_ParserLocalParams_t * DTSParserLocalParams);
U32             DTS_READ_NEXT_WORD32(ES_DTS_ParserLocalParams_t * DTSParserLocalParams);
U32             DTS_READ_NEXT_WORD32_64(ES_DTS_ParserLocalParams_t * DTSParserLocalParams);
U32             DTS_READ_NEXT_WORD64_96(ES_DTS_ParserLocalParams_t * DTSParserLocalParams);
U32             DTS_READ_NEXT_WORD96_128(ES_DTS_ParserLocalParams_t * DTSParserLocalParams);
BOOL        ReadSubStreamHeader(ES_DTS_ParserLocalParams_t * DTSParserLocalParams);




#ifdef __cplusplus
}
#endif



#endif /* _DTS_PARSER_H */

