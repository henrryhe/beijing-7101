/******************************************************************************

File name   : net_rtp.h

Description : RTP specific header

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/
#ifndef __NET_RTP_H
#define __NET_RTP_H

/* Includes ---------------------------------------------------------------- */


#include "stddefs.h"
#include "stnet.h"
#include "net.h"
#include "net_udp.h"


/* Private Types ----------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef  LITTLE_ENDIAN_CPU
typedef struct STNETi_RTP_Header_s
{
/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
    U32                         Version         : 2;
    U32                         Padding         : 1;
    U32                         Extension       : 1;
    U32                         CSRCCount       : 4;
    U32                         Marker          : 1;
    U32                         PayloadType     : 7;
    U32                         SequenceNumber  : 16;
    U32                         TimeStamp;
    U32                         SSRC;
} STNETi_RTP_Header_t;



#else

typedef struct STNETi_RTP_Header_s
{
/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
    /* BYTE 3 & 4 */
    U32                         SequenceNumber  : 16;
    /* BYTE 2 */
    U32                         PayloadType     : 7;
    U32                         Marker          : 1;
    /* BYTE 1 */
    U32                         CSRCCount       : 4;
    U32                         Extension       : 1;
    U32                         Padding         : 1;
    U32                         Version         : 2;

    U32                         TimeStamp;
    U32                         SSRC;
} STNETi_RTP_Header_t;

#endif

typedef struct STNETi_RTP_Packet_s
{
  STNETi_RTP_Header_t          *Header;
  U8                           *Data;
  U32                           CRC;
} STNETi_RTP_Packet_t;

/* Private Constants ------------------------------------------------------- */


#define STNETi_RTP_DEFAULT_PORT         5000
#define STNETi_RTP_HEADER_MIN_SIZE      (3 * sizeof(U32))
#define STNETi_RTP_HEADER_MAX_SIZE      ((3 + 15) * sizeof(U32))
#define STNETi_CRC_SIZE                 4
#define STNETi_RTP_PACKET_MAX_SIZE      (STNETi_RTP_HEADER_MAX_SIZE + STNETi_TS_PAYLOAD_SIZE_IN_UDP + STNETi_CRC_SIZE)

#define STNETi_RTP_PAYLOAD_TYPE_TS      33

#define STNETi_RTP_SEQ_MOD		(1<<16)	/* Sequence number is 16 bits */


/* These provide access to the RTP header in network endianness,
 * independent of the host CPU's endianness.
 */
#define RTP_GET_VERSION(ptr)      ((*((unsigned char*)(ptr)) & 0xc0) >> 6)
#define RTP_GET_PADDING(ptr)      ((*((unsigned char*)(ptr)) & 0x20) >> 5)
#define RTP_GET_EXT(ptr)          ((*((unsigned char*)(ptr)) & 0x10) >> 4)
#define RTP_GET_CSRC_CNT(ptr)     (*((unsigned char*)(ptr)) & 0x0f)
#define RTP_GET_MARKER(ptr)       ((*((unsigned char*)(ptr) + 1) & 0x80) >> 7)
#define RTP_GET_PT(ptr)           (*((unsigned char*)(ptr) + 1) & 0x7f)
#define RTP_GET_SEQ(ptr)          ntohs(*(unsigned short*)((unsigned char*)(ptr) + 2))
#define RTP_GET_TIME(ptr)         ntohl(*(unsigned long*)((unsigned char*)(ptr) + 4))
#define RTP_GET_SSRC(ptr)         ntohl(*(unsigned long*)((unsigned char*)(ptr) + 8))
/* RTP Header Extension handling.
 * This needs a pointer to the extension header.
 */
#define RTP_GET_EXT_SIZE(ptr)	ntohs(*(unsigned short*)((unsigned char*)(ptr) + 2))

/* Number of padding octets */
#define RTP_GET_PADDING_SIZE(ptr, size)	ntohs( ((unsigned char*)(ptr))[size-1] )

#ifdef __cplusplus
}
#endif

#endif
/* EOF --------------------------------------------------------------------- */

