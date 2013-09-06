#ifndef __SCT1_H
#define __SCT1_H

/* Includes ******************************************************/

#include "stlite.h"
#include "stsmart.h"

/* Defines *******************************************************/

/* Generally useful things */
#define MAXBLOCKLEN             3 + 254 + 2

/* Our internal state markers */
#define SMART_T1_OURTX          0x0001
#define SMART_T1_CHAINING_US    0x0002
#define SMART_T1_CHAINING_THEM  0x0004
#define SMART_T1_TXWAITING      0x0008
#define SMART_T1_SRESPEXPECT    0x0010
#define SMART_T1_VPPHIGHTILNAD  0x0020
#define SMART_T1_GOTNAK         0x0040
#define SMART_T1_WTX            0x0080

/* Block types */
#define BLOCK_TYPE_BIT          0xC0
#define S_REQUEST_BLOCK         0xC0
#define R_BLOCK                 0x80
#define I_BLOCK_1               0x00
#define I_BLOCK_2               0x40

/* In S-block */
#define S_RESYNCH_REQUEST       0x00
#define S_IFS_REQUEST           0x01
#define S_ABORT_REQUEST         0x02
#define S_WTX_REQUEST           0x03
#define S_VPP_ERROR_RESPONSE    0x24
#define S_RESPONSE_BIT          0x20
#define S_RESYNCH_RESPONSE      0xE0

/* I-block */
#define I_CHAINING_BIT          0x20
#define I_SEQUENCE_BIT          0x40

/* R-block */
#define R_EDC_ERROR             0x01
#define R_OTHER_ERROR           0x02

/* NAD byte */
#define VPP_NOTHING             0x00
#define VPP_TILNAD              0x08
#define VPP_TILPCB              0x80
#define VPP_INVALID             0x88

/* Type definitions **********************************************/

typedef enum {
    T1_R_BLOCK = 0,
    T1_S_REQUEST,
    T1_S_RESPONSE,
    T1_I_BLOCK,
    T1_CORRUPT_BLOCK
} SMART_BlockType_t;

typedef struct {
    U8 NAD;
    U8 PCB;
    U8 LEN;
} SMART_T1Header_t;

typedef struct {
    SMART_T1Header_t Header;
    U8 *Buffer;
    union {
        U8 LRC;
        U16 CRC;
    } EDC_u;
} SMART_T1Block_t;

/* Macros ********************************************************/

#define NextSeq(i)      (i == 1)?0:1

/* Prototypes ****************************************************/

ST_ErrorCode_t SMART_T1_Transfer(SMART_ControlBlock_t *Smart_p,
                                 U8 *Command_p,
                                 U32 NumberToWrite,
                                 U32 *NumberWritten_p,
                                 U8 *Response_p,
                                 U32 NumberToRead,
                                 U32 *NumberRead_p,
                                 U8 SAD,
                                 U8 DAD);

/* DataByte only used for IFSD adjustment */
ST_ErrorCode_t SMART_T1_SendSBlock(SMART_ControlBlock_t *Smart_p,
                                   U32 Type, U8 DataByte,
                                   U8 SAD, U8 DAD);

void SMART_T1CalculateTimeouts(SMART_ControlBlock_t *Smart_p,
                               SMART_T1Details_t *T1Details_p);
#endif /* #define __SCT1_H */
