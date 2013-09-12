/*******************************************************

 File Name: PES_PACKMP1_Parser.h

 Description: Header File for PES PACKMP1 Parser

 Copyright: STMicroelectronics (c) 2005

 Owner : ST ACC Noida

*******************************************************/
/*This file is to be used with ST Audio Driver */
/* Warning ==> Must not include Outside ST Audio Driver */

#ifndef _PES_PACKMP1_PARSER_H
#define _PES_PACKMP1_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif



/* Constants ---------------------------------------- */

typedef enum
{
    PES_PACKMP1_SYNC_0_00,
    PES_PACKMP1_SYNC_1_00,
    PES_PACKMP1_SYNC_2_01,
    PES_PACKMP1_SYNC_3_STREAMID,
    PES_PACKMP1_PACKET_LENGTH_1,
    PES_PACKMP1_PACKET_LENGTH_2,
    PES_PACKMP1_STUFFING_BYTE,
    PES_PACKMP1_STD_BUFFER_0,
    PES_PACKMP1_STD_BUFFER_1,
    PES_PACKMP1_PTS,
    PES_PACKMP1_PTS_1,
    PES_PACKMP1_PTS_2,
    PES_PACKMP1_PTS_3,
    PES_PACKMP1_PTS_4,
    PES_PACKMP1_PTS_5,
    PES_PACKMP1_SKIPPING_HEADER,
    PES_PACKMP1_SKIPPING_PES,
    PES_PACKMP1_ES_STREAM
}PES_PACKMP1_ParserState_t;


typedef struct
{
    U8                    PES_PACKMP1_PacketHeaderLength;
    U8                    PES_PACKMP1_PackLength1;
    U8                    PES_PACKMP1_PackLength2;
    U8                    PTS[5];
    U8                    AVSyncEnable;
    U8                    StreamID;
    U16                   NoBytesToCopy;
    U16                   NoBytesCopied;
    PTS_t                 CurrentPTS;
    Private_Info_t        Private_Info;
    STAUD_StreamContent_t StreamContent;
    STAUD_StreamType_t    StreamType;
    U32                   PES_PACKMP1_PacketLength;
    U32                   NumberOfBytesESBytesInCurrentPES;
    U32                   NumberOfBytesRemainingForCurrentPES;
    U32                   NumberBytesSkipInPESHeader;
    U32                   PTS_DIVISION;
    S32                   PesHeaderPos;
    S32                   Speed;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_Handle_t      CLKRV_HandleForSynchro;
#endif
    PES_PACKMP1_ParserState_t ParserState;
    ST_Partition_t            * AudioParserMemory_p;

} PES_PACKMP1_ParserData_t;

/* *******   Functions ------------------------------- */

ST_ErrorCode_t PES_PACKMP1_Parser_Init(ComParserInit_t *Init_p, PESParser_Handle_t *);

ST_ErrorCode_t PES_PACKMP1_Parse_Frame(PESParser_Handle_t const Handle, void *MemoryStart,
                                       U32 Size,U32 *No_Bytes_Used, ParsingFunction_t ES_ParsingFunction,
                                       ParserHandle_t ES_Parser_Handle);
ST_ErrorCode_t PES_PACKMP1_Parser_SetStreamType(PESParser_Handle_t const Handle,STAUD_StreamType_t  NewStream,
                                       STAUD_StreamContent_t StreamContents);
ST_ErrorCode_t PES_PACKMP1_Parser_SetBroadcastProfile(PESParser_Handle_t const Handle,
                                       STAUD_BroadcastProfile_t BroadcastProfile);
ST_ErrorCode_t PES_PACKMP1_Parser_SetStreamID(PESParser_Handle_t const Handle, U32 NewStreamID);
ST_ErrorCode_t PES_PACKMP1_Parser_Term(PESParser_Handle_t const Handle,ST_Partition_t   *CPUPartition_p);
ST_ErrorCode_t PES_PACKMP1_SetSpeed(PESParser_Handle_t const Handle, S32 Speed);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t PES_PACKMP1_SetClkSynchro(PESParser_Handle_t const Handle, STCLKRV_Handle_t Clksource);
#endif
ST_ErrorCode_t PES_PACKMP1_AVSyncCmd(PESParser_Handle_t const Handle, BOOL AVSyncEnable);
ST_ErrorCode_t PES_PACKMP1_Parser_Stop(PESParser_Handle_t const Handle);
ST_ErrorCode_t PES_PACKMP1_Parser_Start(PESParser_Handle_t const Handle);


ST_ErrorCode_t PES_PACKMP1_ParserGetParsingFuction( PESParsingFunction_t * PESParsingFucntion_p);

#ifdef __cplusplus
}
#endif



#endif /* _PES_PARSER_H */

