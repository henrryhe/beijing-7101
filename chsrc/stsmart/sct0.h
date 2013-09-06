/*****************************************************************************
File Name   : sct0.h

Description : Definitions specific to the T=0 protocol.

Copyright (C) 2000 STMicroelectronics

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __SCT0_H
#define __SCT0_H

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* T=0 defines */
#define T0_CMD_LENGTH           5
#define T0_CLA_OFFSET           0
#define T0_INS_OFFSET           1
#define T0_P1_OFFSET            2
#define T0_P2_OFFSET            3
#define T0_P3_OFFSET            4
#define T0_RETRIES_DEFAULT      3
#define TIME_DELAY             10000

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t SMART_T0_CheckTransfer(U8 *Command_p);

ST_ErrorCode_t SMART_T0_Transfer(IN SMART_ControlBlock_t *Smart_p,
                                 IN U8 *WriteBuffer_p,
                                 IN U32 NumberToWrite,
                                 IN U8 *ReadBuffer_p,
                                 IN U32 NumberToRead,
                                 OUT U32 *NumberWritten_p,
                                 OUT U32 *NumberRead_p
                                );

#endif /* __SCT0_H */

/* End of sct0.h */
