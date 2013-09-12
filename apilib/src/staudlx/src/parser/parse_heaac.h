/*******************************************************

 File Name: parse_mpeg.h

 Description: Header File for HEAAC Parser

 Copyright: STMicroelectronics (c) 2005

*******************************************************/
#ifndef _PARSE_HEAAC_H
#define _PARSE_HEAAC_H

/* Includes ----------------------------------------- */
#include "staud.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Constants ---------------------------------------- */



typedef enum
{
    DETECT_LAOS_SYNC0,
    DETECT_LAOS_SYNC1,
    PARSE_LAOS_MUX_LENGTH,
    LINEARIZE_LAOS_MUX_DATA,
    DELIVER_LAOS_MUX_DATA
}LAOSParserState_t;

typedef enum
{
    HEAACPES_SYNC_0_00,
    HEAACPES_SYNC_1_00,
    HEAACPES_SYNC_2_01,
    HEAACPES_SYNC_3_STREAMID,
    HEAACPES_PACKET_LENGTH_1,
    HEAACPES_PACKET_LENGTH_2,
    HEAACPES_PACKET_RES1,
    HEAACPES_PACKET_RES2,
    HEAACPES_PACKET_LENGTH,
    HEAACPES_PTS_1,
    HEAACPES_PTS_2,
    HEAACPES_PTS_3,
    HEAACPES_PTS_4,
    HEAACPES_PTS_5,
    HEAACPES_FIRST_VOBU,
    HEAACPES_SKIPPING_HEADER,
    HEAACPES_SKIPPING_PES,
    HEAACPES_ES_STREAM
}HEAACParserStatePES_t;

/* Structures */



typedef struct
{
    U8  HDR[6];
    U8 PESPacketHeaderLength;
    U8 PESPackLength1;
    U8 PESPackLength2;
    U8 PTS[5];
    U8  FrameDuration;
    U16 NoBytesToCopy;
    U16 NoBytesCopied;
    BOOL FirstTransfer;
    BOOL CurretTransfer;
    BOOL CurrentBlockInUse;
    U32 PESPacketLength;
    U32 NumberOfBytesESBytesInCurrentPES;
    U32 LAOSFrameSize;
    U32 Frequency;
    U32 NumberOfBytesRemainingForCurrentPES;
    U32 NumberBytesSkipInPESHeader;
    LAOSParserState_t HEAACParserState;
    HEAACParserStatePES_t ParserState;
} HEAACParserLocalParams_t;

/* *******   Functions ------------------------------- */


#ifdef __cplusplus
}
#endif



#endif /* _PARSE_MPEG_H */

