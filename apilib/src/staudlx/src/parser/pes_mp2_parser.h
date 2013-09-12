/*******************************************************

 File Name: PES_MP2_Parser.h

 Description: Header File for PES MP2 Parser

 Copyright: STMicroelectronics (c) 2005

 Owner : ST ACC Noida

*******************************************************/
/*This file is to be used with ST Audio Driver */
/* Warning ==> Must not include Outside ST Audio Driver */

#ifndef _PES_MP2_PARSER_H
#define _PES_MP2_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"


#ifdef __cplusplus
extern "C" {
#endif



/* Constants ---------------------------------------- */

typedef enum
{
    PES_MP2_SYNC_0_00,
    PES_MP2_SYNC_1_00,
    PES_MP2_SYNC_2_01,
    PES_MP2_SYNC_3_STREAMID,
    PES_MP2_PACKET_LENGTH_1,
    PES_MP2_PACKET_LENGTH_2,
    PES_MP2_PACKET_RES1,
    PES_MP2_PACKET_RES2,
    PES_MP2_HEADER_LENGTH,
    PES_MP2_PTS_1,
    PES_MP2_PTS_2,
    PES_MP2_PTS_3,
    PES_MP2_PTS_4,
    PES_MP2_PTS_5,
    PES_MP2_SKIPPING_HEADER_EXT,
    PES_MP2_SKIP_EXT,
    PES_MP2_PACKET_EXT,
    PES_MP2_PARSE_AD_LENGTH,
    PES_MP2_SKIP_AD_TEXT_N_TAG,
    PES_MP2_PARSE_AD_FADE,
    PES_MP2_PARSE_AD_PAN,
    PES_MP2_SKIPPING_HEADER,
    PES_MP2_SKIPPING_PES,
    PES_SEARCH_PRIVATE_ID,
    PES_SEARCH_PRIVATE_DATA_0,
    PES_SEARCH_PRIVATE_DATA_1,
    PES_SEARCH_PRIVATE_DATA_2,
    PES_SEARCH_PRIVATE_DATA_3,
    PES_SEARCH_PRIVATE_DATA_4,
    PES_SEARCH_PRIVATE_DATA_5,
    PES_SEARCH_PRIVATE_DATA_6,
    PES_SEARCH_PRIVATE_DATA_7,
    PES_SEARCH_PRIVATE_DATA_8,
    PES_SEARCH_PRIVATE_DATA_9,
    PES_SKIPPING_PRIVATE_HEADER,
    PES_MP2_ES_STREAM
}PES_MP2_ParserState_t;


typedef struct
{
    PTS_t                 CurrentPTS;
    U8                    PES_MP2_PacketHeaderLength;
    U8                    PES_MP2_PackLength1;
    U8                    PES_MP2_PackLength2;
    U8                    RES2FLAG;
    U8                    PTS[5];
    U8                    StreamID;
    U16                   NoBytesToCopy;
    U16                   NoBytesCopied;
    U32                   PTS_DIVISION;
    STAUD_StreamContent_t StreamContent;
    STAUD_StreamType_t    StreamType;
    Private_Info_t        Private_Info;
    U32                   Skip;
    U32                   PES_MP2_PacketLength;
    S32                   Speed;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t      CLKRV_HandleForSynchro;
#endif
    U8                    AVSyncEnable;
    U32                   NumberOfBytesESBytesInCurrentPES;
    U32                   NumberOfBytesRemainingForCurrentPES;
    U32                   NumberBytesSkipInPESHeader;
    PES_MP2_ParserState_t ParserState;
    ST_Partition_t        * AudioParserMemory_p;

} PES_MP2_ParserData_t;

/* *******   Functions ------------------------------- */

ST_ErrorCode_t PES_MP2_Parser_Init(ComParserInit_t *Init_p, PESParser_Handle_t *);

ST_ErrorCode_t PES_MP2_Parse_Frame(PESParser_Handle_t const Handle, void *MemoryStart, U32 Size,
                                   U32 *No_Bytes_Used, ParsingFunction_t ES_ParsingFunction,
                                   ParserHandle_t ES_Parser_Handle);
ST_ErrorCode_t PES_MP2_Parser_SetStreamType(PESParser_Handle_t const Handle, STAUD_StreamType_t NewStream,
                                       STAUD_StreamContent_t StreamContents);
ST_ErrorCode_t PES_MP2_Parser_SetBroadcastProfile(PESParser_Handle_t const Handle,
                                                  STAUD_BroadcastProfile_t BroadcastProfile);
ST_ErrorCode_t PES_MP2_Parser_SetStreamID(PESParser_Handle_t const Handle, U32 NewStreamID);
ST_ErrorCode_t PES_MP2_Parser_Term(PESParser_Handle_t const Handle,ST_Partition_t   *CPUPartition_p);
ST_ErrorCode_t PES_MP2_SetSpeed(PESParser_Handle_t const Handle, S32 Speed);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t PES_MP2_SetClkSynchro(PESParser_Handle_t const Handle, STCLKRV_Handle_t Clksource);
#endif
ST_ErrorCode_t PES_MP2_AVSyncCmd(PESParser_Handle_t const Handle, BOOL AVSyncEnable);
ST_ErrorCode_t PES_MP2_Parser_Stop(PESParser_Handle_t const Handle);
ST_ErrorCode_t PES_MP2_Parser_Start(PESParser_Handle_t const Handle);

ST_ErrorCode_t PES_MP2_ParserGetParsingFuction( PESParsingFunction_t * PESParsingFucntion_p);
#ifdef __cplusplus
}
#endif



#endif /* _PES_PARSER_H */

