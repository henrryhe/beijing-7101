/* Includes --------------------------------------------------------------- */
#if defined(ST_OS20) || defined(ST_OS21)
#include <string.h>                     /* C libs */
#include <stdio.h>
#endif

#ifdef ST_OSLINUX
#include "compat.h"
#else
#include "stlite.h"
#endif
#include "sttbx.h"
#include "stsys.h"

#include "stcommon.h"

#include "stsmart.h"                    /* STAPI includes */

#include "scregs.h"                     /* Local includes */
#include "scio.h"
#include "scdat.h"
#include "scdrv.h"
#include "sct1.h"

#include "crctable.h"                   /* For CRC tables. Eventually. */

#if (!defined(ST_OS21)) && (!defined(ST_OSLINUX))
#include "debug.h"
#endif

/* Prototypes-------------------------------------------------------------- */

/* Internal */
static ST_ErrorCode_t IO_T1ReadBlock(SMART_ControlBlock_t *Smart_p,
                                     SMART_T1Block_t *Block);
static ST_ErrorCode_t IO_T1WriteBlock(SMART_ControlBlock_t *Smart_p,
                                      SMART_T1Block_t *Block);

static BOOL SMART_ProcessSRequest(SMART_ControlBlock_t *Smart_p,
                                  SMART_T1Block_t *RxBlock_p);
static ST_ErrorCode_t SMART_T1Resync(SMART_ControlBlock_t *Smart_p,
                                     U8 SAD,
                                     U8 DAD);

static void GenerateCRC(U8 *Buffer_p, U32 Count, U16 *CRC_p);
static void GenerateLRC(U8 *Buffer_p, U32 Count, U8 *LRC_p);
static void CalculateEDC(SMART_ControlBlock_t *Smart_p,
                         SMART_T1Block_t *Block_p);
static SMART_BlockType_t SMART_GetBlockType(U8 PCB);
static void MakeU8FromBlock(SMART_T1Block_t *Block_p,
                            BOOL CRC, U8 *Buffer, U32 *Length);

/* Any test harness must supply these functions */
extern ST_ErrorCode_t GetBlock(U8 *Buffer, U32 *Length);
extern ST_ErrorCode_t PutBlock(U8 *Buffer, U32 Length);

#ifdef SMART_T1TEST
static ST_ErrorCode_t Sim_T1ReadBlock(SMART_ControlBlock_t *Smart_p,
                                      SMART_T1Block_t *Block);
static ST_ErrorCode_t Sim_T1WriteBlock(SMART_ControlBlock_t *Smart_p,
                                       SMART_T1Block_t *Block);
static ST_ErrorCode_t Debug_T1ReadBlock(SMART_ControlBlock_t *Smart_p,
                                        SMART_T1Block_t *Block);
static ST_ErrorCode_t Debug_T1WriteBlock(SMART_ControlBlock_t *Smart_p,
                                         SMART_T1Block_t *Block);

extern U8 UseSim;
#endif

/* Macros ----------------------------------------------------------------- */

#ifndef SMART_T1TEST
#define SMART_T1ReadBlock(Smart_p, Block)   IO_T1ReadBlock(Smart_p, Block)
#define SMART_T1WriteBlock(Smart_p, Block)  IO_T1WriteBlock(Smart_p, Block)
#else
#define SMART_T1ReadBlock(Smart_p, Block)   Debug_T1ReadBlock(Smart_p, Block)
#define SMART_T1WriteBlock(Smart_p, Block)  Debug_T1WriteBlock(Smart_p, Block)
#endif

/****************************************************************************/
/* Main functions                                                           */
/****************************************************************************/

/*****************************************************************************
SMART_T1_Transfer()
Description:
    This routine takes a command (data buffer), and deals with sending it to
    the card and reading any response. Adds and removes the appropriate
    protocol layers, checks for errors etc.
Parameters:
    Smart_p             pointer to smartcard control block
    Command_p           pointer to U8 buffer containing command/data
    NumberToWrite       size of the command buffer
    NumberWritten_p     pointer to U32 storing how many we wrote
    Response_p          pointer to U8 buffer to contain response from card
    NumberToRead        size of response buffer
    NumberRead_p        how many bytes were placed in the response buffer
    SAD                 source address
    DAD                 destination address (used for logical connections)
Return Value:
    ST_NO_ERROR
    STSMART_ERROR_BLOCK_RETRIES
    STSMART_ERROR_BLOCK_TIMEOUT
    STSMART_ERROR_CARD_ABORTED
    STSMART_ERROR_CARD_VPP_ERROR
    Plus values as from SMART_IO_Rx and SMART_IO_Tx
See Also:
    SMART_T0_Transfer
    STSMART_Transfer
*****************************************************************************/

ST_ErrorCode_t SMART_T1_Transfer(SMART_ControlBlock_t *Smart_p,
                                 U8 *Command_p,
                                 U32 NumberToWrite,
                                 U32 *NumberWritten_p,
                                 U8 *Response_p,
                                 U32 NumberToRead,
                                 U32 *NumberRead_p,
                                 U8 SAD,
                                 U8 DAD)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_BlockType_t ThisBlockType;    /* Type of current block */

    BOOL TransmitDone = FALSE;      /* Flags to control operations */
    BOOL ReceiveDone = FALSE;
    BOOL LastTxAcked = FALSE;
    BOOL DoReceive = FALSE;

    SMART_T1Block_t TxBlock, RxBlock;   /* Transmission and reception blocks */
    SMART_T1Block_t RxTxBlock;      /* used for transmitting blocks in receive
                                       routine, so last tx'ed block doesn't get
                                       overwritten.  */

    SMART_T1Details_t *T1Details_p; /* Shortcut for accessing details in Smart_p */
    U8 LRC;                         /* EDC values */
    U16 CRC;
    U8 *WritePointer_p;             /* Pointer into Command_p */
    /* Used for MakeU8FromBlock (EDC checking). */
    U8 BlockBuffer[MAXBLOCKLEN];
    /* Used to be safe when receiving blocks while transmitting */
    U8 TmpBuffer[MAXBLOCKLEN];
    U32 NumberLeft = NumberToWrite; /* Number left to write */

    /* Value in the struct might change between tx and r-block ack, meaning
     * we lose data. That's a Bad Thing. rxinfosize not used, so no need to
     * store it.
     */
    U16 TxMaxInfoStored = 0;

    /* Variable setup */
    T1Details_p = &Smart_p->T1Details;
    T1Details_p->BWTMultiplier = 1;
    WritePointer_p = Command_p;
    T1Details_p->TxCount = 0;
    T1Details_p->Aborting = FALSE;

    /* Work out the timeouts (once per transfer) before we begin */
    SMART_T1CalculateTimeouts(Smart_p, T1Details_p);

    /* Then disable the NACK signal (if supported on this ASC cell) */
    SMART_IO_NackDisable(Smart_p->IoHandle);
    /* Other than S(IFS), we shouldn't really receive anything in here */
    RxBlock.Buffer = TmpBuffer;

    /* Command -> response style, so we start off by dealing with
       sending the command */
    do {
        /* Build up the header - NAD, PCB, LEN, etc. */
        TxBlock.Header.NAD = SAD | (DAD << 4);
        TxBlock.Header.PCB = (T1Details_p->OurSequence << 6);

        /* See how long the remaining payload is, and therefore if we
         * have to chain */
        if (NumberLeft > T1Details_p->TxMaxInfoSize)
        {
            T1Details_p->State |= SMART_T1_CHAINING_US;
            TxBlock.Header.PCB |= I_CHAINING_BIT;
            TxBlock.Header.LEN = T1Details_p->TxMaxInfoSize;
        }
        else
        {
            T1Details_p->State &= ~SMART_T1_CHAINING_US;
            TxBlock.Header.LEN = NumberLeft;
        }

        /* Set up info section */
        TxBlock.Buffer = WritePointer_p;
        CalculateEDC(Smart_p, &TxBlock);

        /* Transmit block */
        error = SMART_T1WriteBlock(Smart_p, &TxBlock);
        TxMaxInfoStored = T1Details_p->TxMaxInfoSize;

        /* If chaining, wait for r-block */
        if ((T1Details_p->State & SMART_T1_CHAINING_US) && (error == ST_NO_ERROR))
        {
            DoReceive = TRUE;
        }
        else
        {
            DoReceive = FALSE;
            TransmitDone = TRUE;
        }

        while (DoReceive == TRUE)
        {
            BOOL Valid = FALSE;
            U32 Length = 0;

            /* Default to one pass through the receive section */
            DoReceive = FALSE;
            error = SMART_T1ReadBlock(Smart_p, &RxBlock);

            if (error == ST_NO_ERROR)
            {
                /* Calculate EDC */
                Valid = FALSE;
                if (Smart_p->RC == 1)
                {
                    MakeU8FromBlock(&RxBlock, TRUE, BlockBuffer, &Length);
                    GenerateCRC(BlockBuffer, Length - 2, &CRC);
                    if (CRC == RxBlock.EDC_u.CRC)
                        Valid = TRUE;
                }
                else
                {
                    MakeU8FromBlock(&RxBlock, FALSE, BlockBuffer, &Length);
                    GenerateLRC(BlockBuffer, Length - 1, &LRC);
                    if (LRC == RxBlock.EDC_u.LRC)
                        Valid = TRUE;
                }

                if (Valid == TRUE)
                {
                    /* See what we got back */
                    ThisBlockType = SMART_GetBlockType(RxBlock.Header.PCB);
                    if (ThisBlockType == T1_S_REQUEST)
                    {
                        TransmitDone = SMART_ProcessSRequest(Smart_p, &RxBlock);
                        /* Read another block, since processsreq. transmits a
                         * block
                         */
                        if (TransmitDone == FALSE)
                        {
                            error = SMART_T1ReadBlock(Smart_p, &RxBlock);
                            if (error == ST_NO_ERROR)
                            {
                                ThisBlockType = SMART_GetBlockType(RxBlock.Header.PCB);
                            }
                            else
                            {
                                ThisBlockType = T1_CORRUPT_BLOCK;
                            }
                        }
                    }
                }
                else
                {
                    ThisBlockType = T1_CORRUPT_BLOCK;
                }
            }
            else
            {
                ThisBlockType = T1_CORRUPT_BLOCK;
            }

            /* If r-block received and ack okay, flip our seq. number;
             */
            if ((ThisBlockType == T1_R_BLOCK) &&
                (((RxBlock.Header.PCB & 0x10) >> 4) != T1Details_p->OurSequence) &&
                ((RxBlock.Header.PCB & R_EDC_ERROR) == 0) &&
                ((RxBlock.Header.PCB & R_OTHER_ERROR) == 0)
               )
            {
                NumberLeft -= TxMaxInfoStored;
                T1Details_p->OurSequence = NextSeq(T1Details_p->OurSequence);
                WritePointer_p += TxMaxInfoStored;
                T1Details_p->TxCount = 0;
            }
            else
            {
                /* Note retransmission */
                T1Details_p->TxCount++;

                /* How many attempts have we made? */
                if (T1Details_p->TxCount >= 3)
                {
                    error = SMART_T1Resync(Smart_p, SAD, DAD);
                }
                else if (ThisBlockType == T1_CORRUPT_BLOCK)
                {
                    /* if corrupt, send an appropriate r-block */
                    RxTxBlock.Header.NAD = SAD | (DAD << 4);
                    if (T1Details_p->FirstBlock)
                        RxTxBlock.Header.PCB = 1;
                    else
                        RxTxBlock.Header.PCB = 1 | (T1Details_p->TheirSequence << 4);
                    RxTxBlock.Header.PCB |= R_BLOCK;
                    RxTxBlock.Header.LEN = 0;
                    RxTxBlock.Buffer = NULL;
                    CalculateEDC(Smart_p, &RxTxBlock);

                    /* Transmit */
                    error = SMART_T1WriteBlock(Smart_p, &RxTxBlock);
                    DoReceive = TRUE;
                }

            }
        }

    } while ((!TransmitDone) && (*NumberWritten_p < NumberToWrite) &&
             (error == ST_NO_ERROR));

    T1Details_p->FirstBlock = TRUE;
    do {
        U32 Length; /* Amount copied to U8 buffer */

        /* The only way this could occur is an error left by the transmit section
         * above. Putting a break here avoids a goto, to skip this section.
         */
        if (error != ST_NO_ERROR)
            break;
        /* Check whether the user was expecting to read anything */
        if ((NULL == Response_p) || (NULL == NumberRead_p))
            break;

        /* Set the buffer to be within the user's buffer */
        RxBlock.Buffer = &Response_p[*NumberRead_p];

        error = SMART_T1ReadBlock(Smart_p, &RxBlock);

        /* Calculate EDC */
        if (error == ST_NO_ERROR)
        {
            if (Smart_p->RC == 1)
            {
                MakeU8FromBlock(&RxBlock, TRUE, BlockBuffer, &Length);
                GenerateCRC(BlockBuffer, Length - 2, &CRC);
            }
            else
            {
                MakeU8FromBlock(&RxBlock, FALSE, BlockBuffer, &Length);
                GenerateLRC(BlockBuffer, Length - 1, &LRC);
            }
        }

        /* If EDC fails, transmit R-block requesting retransmission */
        if ((((Smart_p->RC & 0x01) == 1) && (CRC != RxBlock.EDC_u.CRC)) ||
            (((Smart_p->RC & 0x01) == 0) && (LRC != RxBlock.EDC_u.LRC)) ||
            (error != ST_NO_ERROR)
           )
        {
            /* Check how many times we've transmitted so far -
             * if it's less than 3, try it again, else start resync */
            if (T1Details_p->TxCount < 3)
            {
                /* Increase count, and retransmit */
                T1Details_p->TxCount++;

                /* Build r-block */
                RxTxBlock.Header.NAD = SAD | (DAD << 4);
                if ((error == STSMART_ERROR_BLOCK_TIMEOUT) && (T1Details_p->FirstBlock))
                    RxTxBlock.Header.PCB = 0;
                else
                    RxTxBlock.Header.PCB = (NextSeq(T1Details_p->TheirSequence) << 4);
                RxTxBlock.Header.PCB |= R_BLOCK;

                if (error == ST_NO_ERROR)
                    RxTxBlock.Header.PCB |= R_EDC_ERROR;
                else
                    RxTxBlock.Header.PCB |= R_OTHER_ERROR;

                RxTxBlock.Header.LEN = 0;
                RxTxBlock.Buffer = NULL;
                CalculateEDC(Smart_p, &RxTxBlock);

                /* On some errors, should retransmit the last block we sent */
                if ((error == STSMART_ERROR_BLOCK_TIMEOUT) ||
                    (error == STSMART_ERROR_OVERRUN) ||
                    (error == STSMART_ERROR_PARITY) ||
                    (error == STSMART_ERROR_FRAME))
                {
                    /* The interface should send out an R Block instead of an I Block
                       if there is a receiving error such as frame or over-run. */
                    error = SMART_T1WriteBlock(Smart_p, &TxBlock); /* R block */

                }
                else
                    /* Else request they retransmit theirs */
                    error = SMART_T1WriteBlock(Smart_p, &RxTxBlock);
            }
            else
            {
                /* Resync */
                error = SMART_T1Resync(Smart_p, SAD, DAD);
            }
        }
        else
        {
            /* See what kind of block this is */
            ThisBlockType = SMART_GetBlockType(RxBlock.Header.PCB);

            /* Process accordingly (expected types - s-request, i-block) */
            if ((ThisBlockType != T1_S_REQUEST) && (ThisBlockType != T1_I_BLOCK))
            {
                /* Retransmit */
                T1Details_p->TxCount++;

                /* How many times have we done this already? */
                if (T1Details_p->TxCount < 3)
                {
                    if (ThisBlockType == T1_R_BLOCK)
                    {
                        /* See if the last block we sent out is what they're
                         * requesting. */
                        if (T1Details_p->OurSequence != ((RxBlock.Header.PCB >> 4) & 1))
                        {
                            error = SMART_T1WriteBlock(Smart_p, &RxTxBlock);
                        }
                        else
                        {
                            if (LastTxAcked)
                                error = SMART_T1WriteBlock(Smart_p, &RxTxBlock);
                            else
                                error = SMART_T1WriteBlock(Smart_p, &TxBlock);
                        }
                    }
                    else
                    {
                        /* It's all gone horribly wrong. Panic. */
                        if (LastTxAcked)
                            error = SMART_T1WriteBlock(Smart_p, &RxTxBlock);
                        else
                            error = SMART_T1WriteBlock(Smart_p, &TxBlock);
                    }
                }
                else
                {
                    SMART_T1Resync(Smart_p, SAD, DAD);
                }
            }
            else
            {
                /* Valid block type, continue... */

                /* If s-request, process */
                if (ThisBlockType == T1_S_REQUEST)
                {
                    /* This function also transmits the response */
                    ReceiveDone = SMART_ProcessSRequest(Smart_p, &RxBlock);
                }
                else
                {
                    /* Check if this block has the expected seq. number */
                    if (((RxBlock.Header.PCB & I_SEQUENCE_BIT) >> 6) != T1Details_p->TheirSequence)
                    {
                        U32 Number = 0;

                        /* Seems valid ... copy data into the buffer,
                         * increase bytesread
                         */
                        if ((RxBlock.Header.LEN < (NumberToRead - *NumberRead_p)) ||
                            (NumberToRead == 0)
                           )
                            Number = RxBlock.Header.LEN;
                        else
                            Number = (NumberToRead - *NumberRead_p);

                        *NumberRead_p += Number;

                        T1Details_p->TxCount = 0;

                        /* Set their last received seq. number */
                        T1Details_p->TheirSequence =
                            ((RxBlock.Header.PCB & I_SEQUENCE_BIT) >> 6);

                        if (LastTxAcked == FALSE)
                        {
                            LastTxAcked = TRUE;
                            T1Details_p->OurSequence = NextSeq(T1Details_p->OurSequence);
                        }

                        if (T1Details_p->FirstBlock == TRUE)
                            T1Details_p->FirstBlock = FALSE;

                        /* Is this block chaining? */
                        if ((RxBlock.Header.PCB & I_CHAINING_BIT) != 0)
                        {
                            /* yes; send an r-block with their next
                             * expected no.  (block must have been
                             * received properly to get this far)
                             */
                            RxTxBlock.Header.PCB = R_BLOCK |
                                    (NextSeq(T1Details_p->TheirSequence) << 4);
                            RxTxBlock.Header.NAD = SAD | (DAD << 4);
                            RxTxBlock.Header.LEN = 0;
                            RxTxBlock.Buffer = NULL;
                            CalculateEDC(Smart_p, &RxTxBlock);

                            error = SMART_T1WriteBlock(Smart_p, &RxTxBlock);
                        }
                        else
                        {
                            /* Not chaining (now) */
                            ReceiveDone = TRUE;
                        }
                    }
                    else
                    {
                        /* Case - we received valid i-block with the
                         * same seq. number we got last; therefore they
                         * didn't get our block. Request retransmission
                         * of the missing block.
                         */
                        RxTxBlock.Header.NAD = SAD | (DAD << 4);
                        RxTxBlock.Header.PCB = R_BLOCK;
                        RxTxBlock.Header.PCB |= NextSeq(T1Details_p->TheirSequence);
                        RxTxBlock.Header.LEN = 0;
                        RxTxBlock.Buffer = NULL;
                        CalculateEDC(Smart_p, &RxTxBlock);
                        error = SMART_T1WriteBlock(Smart_p, &RxTxBlock);
                    }
                }
            }
        }

    } while ((!ReceiveDone) &&
             ((NumberToRead == 0) || (*NumberRead_p < NumberToRead)) &&
             (error == ST_NO_ERROR));

    Smart_p->Status_p->ErrorCode = error;
    return error;
}

/*****************************************************************************
SMART_ProcessSRequest()
Description:
    Takes care of acting on S-request blocks, and sending an appropriate
    S-response block.
Parameters:
    Smart_p             pointer to smartcard control block
    RxBlock_p           pointer to the S-block
Return Value:
    Whether to exit receive loop or not
See Also:
    SMART_T0_Transfer
*****************************************************************************/

static BOOL SMART_ProcessSRequest(SMART_ControlBlock_t *Smart_p,
                                  SMART_T1Block_t *RxBlock_p)
{
    SMART_T1Block_t TxBlock;
    SMART_T1Details_t *T1Details_p;
    BOOL ReceiveDone = FALSE;

    T1Details_p = &Smart_p->T1Details;

    if ((RxBlock_p->Header.PCB & S_VPP_ERROR_RESPONSE) == S_VPP_ERROR_RESPONSE)
    {
        Smart_p->Status_p->ErrorCode = STSMART_ERROR_CARD_VPP;
        ReceiveDone = TRUE;
    }
    else if ((RxBlock_p->Header.PCB & S_WTX_REQUEST) == S_WTX_REQUEST)
    {
        /* Multiply BWT by buffer[0], for this block only */
        T1Details_p->BWTMultiplier = RxBlock_p->Buffer[0];
        ReceiveDone = FALSE;
    }
    else if ((RxBlock_p->Header.PCB & S_ABORT_REQUEST) == S_ABORT_REQUEST)
    {
        /* Abort any pending IO operations with the card. */
        SMART_IO_Abort(Smart_p->IoHandle);
        Smart_p->Status_p->ErrorCode = STSMART_ERROR_CARD_ABORTED;
        ReceiveDone = TRUE;
    }
    else if ((RxBlock_p->Header.PCB & S_IFS_REQUEST) == S_IFS_REQUEST)
    {
        /* Change Txmaxinfosize */
        T1Details_p->TxMaxInfoSize = RxBlock_p->Buffer[0];
    }

    /* Those S-responses with an info field are supposed to match the
     * request. Which is handy.
     */
    TxBlock = *RxBlock_p;
    TxBlock.Header.PCB |= S_RESPONSE_BIT;
    CalculateEDC(Smart_p, &TxBlock);
    SMART_T1WriteBlock(Smart_p, &TxBlock);

    /* If we're acking an abort request, we need to read (and possibly discard)
     * the block the card sends in return.
     */
    if (Smart_p->Status_p->ErrorCode == STSMART_ERROR_CARD_ABORTED)
    {
        U8 BlockBuffer[MAXBLOCKLEN];
        SMART_T1Block_t RxBlock;
        BOOL Done = FALSE;
        ST_ErrorCode_t error = ST_NO_ERROR;

        T1Details_p->TxCount = 0;
        RxBlock.Buffer = &BlockBuffer[0];
        do
        {
            BOOL Sblock;

            /* Read the block tranferring right to send */
            error = SMART_T1ReadBlock(Smart_p, &RxBlock);

            /* Check type */
            Sblock = ((RxBlock.Header.PCB & 0x0c) == 0x0c)?TRUE:FALSE;

            /* S-response can be considered acknowledged (in this case) if the
             * card does *not* send another abort request.
             */
            if ((Sblock == FALSE) ||
                ((Sblock == TRUE) && ((RxBlock.Header.PCB & S_ABORT_REQUEST) == 0)))
            {
                Done = TRUE;
            }
            else
            {
                /* Else, retransmit 3 times, before doing resync */
                T1Details_p->TxCount++;
                if (T1Details_p->TxCount >= 3)
                {
                    SMART_T1Resync(Smart_p,
                                   RxBlock_p->Header.NAD & 0x7,
                                   (RxBlock_p->Header.NAD & 0x70) >> 4);
                    Done = TRUE;
                }
                else
                {
                    SMART_T1WriteBlock(Smart_p, &TxBlock);
                }
            }
        } while (Done == FALSE);
    }

    return ReceiveDone;
}

/* Switchers, for use when debugging - send calls to the appropriate
 * function. When SMART_T1Test is defined, calls to SMART_T1ReadBlock
 * and SMART_T1WriteBlock are sent through these two functions.
 */
#ifdef SMART_T1TEST
static ST_ErrorCode_t Debug_T1ReadBlock(SMART_ControlBlock_t *Smart_p,
                                        SMART_T1Block_t *Block_p)
{
    if (UseSim)
        return Sim_T1ReadBlock(Smart_p, Block_p);
    else
        return IO_T1ReadBlock(Smart_p, Block_p);
}

static ST_ErrorCode_t Debug_T1WriteBlock(SMART_ControlBlock_t *Smart_p,
                                         SMART_T1Block_t *Block_p)
{
    if (UseSim)
        return Sim_T1WriteBlock(Smart_p, Block_p);
    else
        return IO_T1WriteBlock(Smart_p, Block_p);
}
#endif

/*****************************************************************************
IO_T1ReadBlock()
Description:
    Reads a block from the card, using SMART_IO_Rx. First three header bytes
    are read individually, then the info field (if any), followed by the EDC
    field. Also raises/lowers Vpp according to card requests. Does no EDC
    or other validity checking, but does return timeout if CWT or BWT are
    exceeded.
Parameters:
    Smart_p             pointer to smartcard control block
    Block_p             pointer to block-structure to fill
Return Value:
    ST_NO_ERROR
    STSMART_ERROR_BLOCK_TIMEOUT
    Plus values as from SMART_IO_Rx
See Also:
    SMART_T0_Transfer
*****************************************************************************/
static ST_ErrorCode_t IO_T1ReadBlock(SMART_ControlBlock_t *Smart_p,
                                     SMART_T1Block_t *Block_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_T1Details_t *T1Details_p;
    U32 Read;
    U8 TmpNAD;

    T1Details_p = &Smart_p->T1Details;

    /* Wait for (and read) the header, starting with NAD
     * Timeout == T1Details_p->BWT, in this case only
     */
    error = SMART_IO_Rx(Smart_p->IoHandle, &Block_p->Header.NAD, 1, &Read,
                        T1Details_p->BWT * T1Details_p->BWTMultiplier,
                        NULL, NULL);

    /* Do we need to lower the Vpp? */
    if (T1Details_p->State & SMART_T1_VPPHIGHTILNAD)
    {
        SMART_DisableVpp(Smart_p);
        T1Details_p->State &= ~SMART_T1_VPPHIGHTILNAD;
    }

    if (error == ST_NO_ERROR)
    {
        /* Check if Vpp needs changing, and if so, for how long */
        TmpNAD = (Block_p->Header.NAD & 0x88);
        switch (TmpNAD)
        {
            case VPP_INVALID:   /* Shouldn't occur */
            case VPP_NOTHING:   /* Do nothing */
                                break;
            case VPP_TILPCB:    /* Vpp high till PCB */
                                SMART_EnableVpp(Smart_p);
                                break;
            case VPP_TILNAD:    /* Vpp high till next NAD */
                                SMART_EnableVpp(Smart_p);
                                T1Details_p->State |= SMART_T1_VPPHIGHTILNAD;
                                break;
        }

        /* Read PCB */
        if (error == ST_NO_ERROR)
        {
            error = SMART_IO_Rx(Smart_p->IoHandle, &Block_p->Header.PCB, 1,
                                &Read, T1Details_p->CWT, NULL, NULL);

            /* Time to turn Vpp off? */
            if (TmpNAD & VPP_TILPCB)
            {
                SMART_DisableVpp(Smart_p);
            }

        }

        /* Read LEN */
        if (error == ST_NO_ERROR)
        {
            error = SMART_IO_Rx(Smart_p->IoHandle, &Block_p->Header.LEN, 1,
                                &Read, T1Details_p->CWT, NULL, NULL);
        }

        /* Read any info field */
        if (error == ST_NO_ERROR)
        {
            error = SMART_IO_Rx(Smart_p->IoHandle, Block_p->Buffer,
                                Block_p->Header.LEN, &Read,
                                T1Details_p->CWT, NULL, NULL);
        }

        /* Read epilogue field */
        if (error == ST_NO_ERROR)
        {
            if (Smart_p->RC == 1)
            {
                error = SMART_IO_Rx(Smart_p->IoHandle,
                                    (U8 *)&Block_p->EDC_u.CRC, 2, &Read,
                                    T1Details_p->CWT, NULL, NULL);
            }
            else
            {
                error = SMART_IO_Rx(Smart_p->IoHandle,
                                    &Block_p->EDC_u.LRC, 1, &Read,
                                    T1Details_p->CWT, NULL, NULL);
            }
        }

    }
    else
    {
        error = STSMART_ERROR_BLOCK_TIMEOUT;
    }

    T1Details_p->LastReceiveTime = STOS_time_now();
    T1Details_p->NextTransmitTime = STOS_time_plus(STOS_time_now(),T1Details_p->BGT);

    /* Either way, any wait extension has now expired */
    T1Details_p->BWTMultiplier = 1;

    return error;
}

#ifdef SMART_T1TEST
static ST_ErrorCode_t Sim_T1ReadBlock(SMART_ControlBlock_t *Smart_p,
                                      SMART_T1Block_t *Block_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_T1Details_t *T1Details_p;
    U8 BlockBuffer[MAXBLOCKLEN];
    U32 Length;

    T1Details_p = &Smart_p->T1Details;

    /* Get the next buffer */
    error = GetBlock(BlockBuffer, &Length);

    /* Do the opposite of makeu8fromblock :) */
    Block_p->Header.NAD = BlockBuffer[0];
    Block_p->Header.PCB = BlockBuffer[1];
    Block_p->Header.LEN = BlockBuffer[2];

    memcpy(Block_p->Buffer, &BlockBuffer[3], BlockBuffer[2]);

    if (Smart_p->RC == 1)
        memcpy(&Block_p->EDC_u.CRC, &BlockBuffer[3 + Block_p->Header.LEN], 2);
    else
        Block_p->EDC_u.LRC = BlockBuffer[3 + Block_p->Header.LEN];

    /* Update, used for BGT checks in t1writeblock */
    T1Details_p->LastReceiveTime = STOS_time_now();
    T1Details_p->NextTransmitTime = STOS_time_plus(STOS_time_now(),
                                        T1Details_p->BGT);

    /* Any wait extension has now expired */
    T1Details_p->BWTMultiplier = 1;

    return error;
}
#endif

/*****************************************************************************
IO_T1WriteBlock()
Description:
    Writes a block to the card, using SMART_IO_Tx. Obeys block guard-time
    (minimum time between blocks). Transmits block en masse, by building a U8
    buffer from the block and transmitting that.
Parameters:
    Smart_p             pointer to smartcard control block
    Block_p             pointer to block-structure to send
Return Value:
    ST_NO_ERROR
    Plus values as from SMART_IO_Tx
See Also:
    SMART_T0_Transfer
*****************************************************************************/
static ST_ErrorCode_t IO_T1WriteBlock(SMART_ControlBlock_t *Smart_p,
                                      SMART_T1Block_t *Block_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_T1Details_t *T1Details_p;
    U32 Written, Length;
    U8 BlockBuffer[MAXBLOCKLEN];
    STOS_Clock_t t1;

    T1Details_p = &Smart_p->T1Details;
    MakeU8FromBlock(Block_p, (Smart_p->RC == 1)?TRUE:FALSE, BlockBuffer, &Length);

    /* Check that block-guard time has been observed */
#ifdef ST_OSLINUX
    if (STOS_time_after((unsigned long)STOS_time_now(), (unsigned long)T1Details_p->NextTransmitTime) == 0)
#else
    if (STOS_time_after(STOS_time_now(), T1Details_p->NextTransmitTime) == 0)
#endif
    {
        /* Work out how many ticks left */
        STOS_Clock_t delay = STOS_time_minus(T1Details_p->NextTransmitTime, STOS_time_now());

        /* And then idle them away... */
        STOS_TaskDelay(delay); /* DDTS 42465 */
    }
    /* The wait till CWT time before resending an I Block - DDTS 33326 */
    t1 = STOS_time_now();

    /* The wait till CWT time before resending an I Block - DDTS 33326 */
    t1 = STOS_time_now();

    error = SMART_IO_Tx( Smart_p->IoHandle, BlockBuffer, Length,
                        &Written, Smart_p->TxTimeout, NULL, NULL);

    if (error != ST_NO_ERROR)
    {
        STOS_TaskDelay( STOS_time_minus(Smart_p->TxTimeout,STOS_time_minus( STOS_time_now(),t1 ))) ;
    }

    return error;
}

#ifdef SMART_T1TEST
static ST_ErrorCode_t Sim_T1WriteBlock(SMART_ControlBlock_t *Smart_p,
                                       SMART_T1Block_t *Block_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_T1Details_t *T1Details_p;
    U8 BlockBuffer[MAXBLOCKLEN];
    U32 Length;

    T1Details_p = &Smart_p->T1Details;

    MakeU8FromBlock(Block_p, (Smart_p->RC == 1)?TRUE:FALSE, BlockBuffer, &Length);

    /* Check that block-guard time has been observed */
    if (STOS_time_after(STOS_time_now(), T1Details_p->NextTransmitTime) == 0)
    {
        /* Work out how many ticks left */
        STOS_Clock_t delay = STOS_time_minus(T1Details_p->NextTransmitTime, STOS_time_now());

        /* And then idle them away... */
        STOS_TaskDelay(delay);
    }

    error = PutBlock(BlockBuffer, Length);

    return error;
}
#endif

/*****************************************************************************
SMART_T1Resync()
Description:
    Carries out the resync. procedure with an ICC, sending and receiving
    S-request and S-response blocks. If resync completed, resets count and
    sequence numbers.
Parameters:
    Smart_p             pointer to smartcard control block
    SAD                 source address
    DAD                 destination address (used for logical connections)
Return Value:
    ST_NO_ERROR
    STSMART_ERROR_BLOCK_RETRIES
    Plus values as from SMART_T1WriteBlock, SMART_T1ReadBlock
See Also:
    SMART_T0_Transfer
*****************************************************************************/
static ST_ErrorCode_t SMART_T1Resync(SMART_ControlBlock_t *Smart_p,
                                     U8 SAD,
                                     U8 DAD)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_T1Details_t *T1Details_p = NULL;
    SMART_T1Block_t TxBlock, RxBlock;
    BOOL Valid;
    U8 LRC = 0, Count = 0;
    U16 CRC = 0;

    U8 BlockBuffer[MAXBLOCKLEN];
    U32 Length;

    T1Details_p = &Smart_p->T1Details;

    /* Send sync s.block */
    TxBlock.Header.PCB = S_REQUEST_BLOCK | S_RESYNCH_REQUEST;
    TxBlock.Header.NAD = SAD | (DAD << 4);
    TxBlock.Header.LEN = 0;
    TxBlock.Buffer = NULL;
    CalculateEDC(Smart_p, &TxBlock);

    /* Send until we get a valid response, or until we've tried 3 times.
     * No need to assign RxBlock a buffer, since resync-responses don't
     * have an info field.
     */
    Count = 0;
    do
    {
        error = SMART_T1WriteBlock(Smart_p, &TxBlock);

        error = SMART_T1ReadBlock(Smart_p, &RxBlock);

        /* Calculate EDC */
        Valid = FALSE;
        if (error == ST_NO_ERROR)
        {
            if (Smart_p->RC == 1)
            {
                MakeU8FromBlock(&RxBlock, TRUE, BlockBuffer, &Length);
                GenerateCRC(BlockBuffer, Length - 2, &CRC);
                if (CRC == RxBlock.EDC_u.CRC)
                    Valid = TRUE;
            }
            else
            {
                MakeU8FromBlock(&RxBlock, FALSE, BlockBuffer, &Length);
                GenerateLRC(BlockBuffer, Length - 1, &LRC);
                if (LRC == RxBlock.EDC_u.LRC)
                    Valid = TRUE;
            }
        }

        Count++;
    } while ((Count < 3) &&
             ((RxBlock.Header.PCB != S_RESYNCH_RESPONSE) || (Valid == FALSE))
            );

    if ((Valid == FALSE) || (RxBlock.Header.PCB != S_RESYNCH_RESPONSE))
    {
        /* stsmart_no_answer would maybe also fit, apart from possibility
         * card is transmitting but EDC incorrect
         */
        error = STSMART_ERROR_BLOCK_RETRIES;
    }

    /* Reset sequence numbers */
    T1Details_p->TheirSequence = 1;  /* So we're expecting 0 next block...   */
    T1Details_p->OurSequence = 0;    /* And sending 0 as our first S(N)      */
    T1Details_p->FirstBlock = TRUE;
    T1Details_p->TxCount = 0;

    return error;
}

/*****************************************************************************
SMART_T1CalculateTimeouts()
Description:
    Calculates the various timeouts required, in whatever unit they're used
    in.
Parameters:
    Smart_p             pointer to smartcard control block
    T1Details_p         pointer to a T1Details_t structure to store values in.
Return Value:
See Also:
    SMART_T0_Transfer
*****************************************************************************/
void SMART_T1CalculateTimeouts(SMART_ControlBlock_t *Smart_p,
                               SMART_T1Details_t *T1Details_p)
{
    /* Calculate CWT (ms)       */
    T1Details_p->CWT = ((((1 << Smart_p->CWInt) + 11) * 1000) /
                        Smart_p->SmartParams.BaudRate);

    /* Calculate BGT (ticks)    */
    T1Details_p->BGT = (22 * ST_GetClocksPerSecond()) /
                        Smart_p->SmartParams.BaudRate;

    /* Calculate BWT (ms)       */
    if (Smart_p->FInt == 0)
    {
        /* Internal clock */
        T1Details_p->BWT = ((1000 * (1 << Smart_p->BWInt)) / 10) +
                            ((1000 * 11) / Smart_p->SmartParams.BaudRate);
    }
    else
    {
        T1Details_p->BWT = ((1000 * (1 << Smart_p->BWInt) * 960 * 372) / Smart_p->Fs)
                           + ((1000 * 11) / Smart_p->SmartParams.BaudRate);
    }
}

/*****************************************************************************
SMART_T1_SendSBlock()
Description:
    Sends an S-block of the indicated type, with any associated data.
Parameters:
    Smart_p             pointer to smartcard control block
    Type                What type of S-block to send
    DataByte            Data (only valid for IFS adjustment)
    SAD                 Source address (logical connection)
    DAD                 Dest address (logical connection)
Return Value:
    ST_NO_ERROR,
    etc
See Also:
    STSMART_SetInfoFieldSize
    SMART_T1_Transfer
*****************************************************************************/
ST_ErrorCode_t SMART_T1_SendSBlock(SMART_ControlBlock_t *Smart_p,
                                   U32 Type, U8 DataByte,
                                   U8 SAD, U8 DAD)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    SMART_T1Block_t TxBlock, RxBlock;
    SMART_T1Details_t *T1Details_p = NULL;
    U8 BlockBuffer[MAXBLOCKLEN];

    T1Details_p = &Smart_p->T1Details;

    TxBlock.Header.NAD = ((DAD & 7) << 4) | (SAD & 7);
    /* S-block */
    TxBlock.Header.PCB = 0xc0;
    TxBlock.Header.PCB |= Type;

    if (Type == S_ABORT_REQUEST)
    {
        /* Write me. */
    }
    else if (Type == S_IFS_REQUEST)
    {
        TxBlock.Header.LEN = 1;
        TxBlock.Buffer = &DataByte;
        /* Note: we'll still accept blocks larger than this. */
        T1Details_p->RxMaxInfoSize = DataByte;
    }
    RxBlock.Buffer = BlockBuffer;

    CalculateEDC(Smart_p, &TxBlock);
    SMART_T1WriteBlock(Smart_p, &TxBlock);
    SMART_T1ReadBlock(Smart_p, &RxBlock);

    if (RxBlock.Header.PCB != (TxBlock.Header.PCB | S_RESPONSE_BIT))
    {
        error = STSMART_ERROR_UNKNOWN;
    }

    return error;
}

/****************************************************************************/
/* Support functions - simple                                               */
/****************************************************************************/

static SMART_BlockType_t SMART_GetBlockType(U8 PCB)
{
    SMART_BlockType_t ThisBlockType;

    /* See what we got back */
    switch (PCB & BLOCK_TYPE_BIT)
    {
        case S_REQUEST_BLOCK:   /* S-block */
                                ThisBlockType = T1_S_REQUEST;
                                break;
        case R_BLOCK:           /* R-block */
                                ThisBlockType = T1_R_BLOCK;
                                break;
        case I_BLOCK_1:         /* Two possible forms of i-block */
        case I_BLOCK_2:         ThisBlockType = T1_I_BLOCK;
                                break;
        default:                ThisBlockType = T1_CORRUPT_BLOCK;
                                break;
    }

    return ThisBlockType;
}

/* Calculate on the fly - this is slow, but will do for a first implementation.
 * Can look at a table driven version later if this is actually the right poly.
 */
static void GenerateCRC(U8 *Buffer_p, U32 Count, U16 *CRC_p)
{
    U32 i;

    /* Initialise the CRC */
    *CRC_p = 0;

    /* Then work through the buffer, updating the CRC accordingly as we go */
    for (i = 0; i < Count; i++)
    {
        *CRC_p ^= Buffer_p[i];
        *CRC_p ^= (U8)(*CRC_p & 0xff) >> 4;
        *CRC_p ^= (*CRC_p << 8) << 4;
        *CRC_p ^= ((*CRC_p & 0xff) << 4) << 1;
    }
}

static void GenerateLRC(U8 *Buffer_p, U32 Count, U8 *LRC_p)
{
    U32 i;

    *LRC_p = 0;
    for (i = 0; i < Count; i++)
    {
        *LRC_p = *LRC_p ^ Buffer_p[i];
    }
}

static void CalculateEDC(SMART_ControlBlock_t *Smart_p,
                         SMART_T1Block_t *Block_p)
{
    U8 BlockBuffer[MAXBLOCKLEN];
    U32 Length;

    /* Calculate EDC */
    if (Smart_p->RC == 1)
    {
        MakeU8FromBlock(Block_p, TRUE, BlockBuffer, &Length);
        GenerateCRC(BlockBuffer, Length - 2, &Block_p->EDC_u.CRC);
    }
    else
    {
        MakeU8FromBlock(Block_p, FALSE, BlockBuffer, &Length);
        GenerateLRC(BlockBuffer, Length - 1, &Block_p->EDC_u.LRC);
    }
}

static void MakeU8FromBlock(SMART_T1Block_t *Block_p,
                            BOOL CRC,
                            U8 *Buffer,
                            U32 *Length)
{
    Buffer[0] = Block_p->Header.NAD;
    Buffer[1] = Block_p->Header.PCB;
    Buffer[2] = Block_p->Header.LEN;

    if (Block_p->Buffer != NULL)
        memcpy(&Buffer[3], Block_p->Buffer, Buffer[2]);

    /* Might not be set yet, but copy it anyway */
    if (CRC)
    {
        /* MSB-first */
        Buffer[3 + Buffer[2]] = (U8)((Block_p->EDC_u.CRC & 0xff00) >> 8);
        Buffer[4 + Buffer[2]] = (U8)(Block_p->EDC_u.CRC & 0x00ff);
        *Length = 3 + Buffer[2] + 2;
    }
    else
    {
        Buffer[3 + Buffer[2]] = Block_p->EDC_u.LRC;
        *Length = 3 + Buffer[2] + 1;
    }

}
