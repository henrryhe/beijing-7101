/*****************************************************************************
File Name   : sct0.c

Description : Protocol type T=0 routines.

Copyright (C) 2006 STMicroelectronics

History     :

    30/05/00    All stuart communications have been moved to the scio.c
                module.  This simplifies all smartcard communications
                routines and avoids much code duplication in the driver.

Reference   :

*****************************************************************************/

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
#include "sct0.h"

/* Private types/constants ------------------------------------------------ */

/* Private variables ------------------------------------------------------ */

/* Private macros --------------------------------------------------------- */

/* Private Prototypes  ---------------------------------------------------- */

ST_ErrorCode_t SMART_T0_ProcessProcedureBytes(SMART_ControlBlock_t *Smart_p,
                                              IN U8 INS,
                                              IN U32 WriteSize,
                                              IN U32 ReadSize,
                                              OUT U8 *Buf_p,
                                              OUT U8 *Size_p,
                                              OUT U32 *NextWriteSize_p,
                                              OUT U32 *NextReadSize_p
                                             );

/* Exported Functions  ---------------------------------------------------- */

ST_ErrorCode_t SMART_T0_CheckTransfer(U8 *Command_p)
{
    ST_ErrorCode_t error;

    /* Check the CLA and INS bytes are valid */
    if (Command_p[T0_CLA_OFFSET] != 0xFF)
    {
        if ((Command_p[T0_INS_OFFSET] & 0xF0) != 0x60 &&
            (Command_p[T0_INS_OFFSET] & 0xF0) != 0x90)
        {
            /* This is a valid command */
            error = ST_NO_ERROR;
        }
        else
        {
            /* INS is invalid */
            error = STSMART_ERROR_INVALID_CODE;
        }
    }
    else
    {
        /* CLA is invalid */
        error = STSMART_ERROR_INVALID_CLASS;
    }

    return error;
}

ST_ErrorCode_t SMART_T0_Transfer(IN SMART_ControlBlock_t *Smart_p,
                                 IN U8 *WriteBuffer_p,
                                 IN U32 NumberToWrite,
                                 IN U8 *ReadBuffer_p,
                                 IN U32 NumberToRead,
                                 OUT U32 *NumberWritten_p,
                                 OUT U32 *NumberRead_p
                                )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 NumberWritten, NextWriteSize, NextReadSize, TotalRead;

    /* Calculate number of bytes expected to be received */
    if (NumberToWrite == T0_CMD_LENGTH)
    {
        /* Extract number to read from P3 */
        NumberToRead = WriteBuffer_p[T0_P3_OFFSET];
    }
    else
    {
        /* Data is being sent, so no data response will be permitted */
        NumberToRead = 0;
    }

    /* Flush FIFOs first */
    SMART_IO_Flush(Smart_p->IoHandle);

    /* Make sure the NACK signal is enabled */
    SMART_IO_NackEnable(Smart_p->IoHandle);

    /* Set status INS byte */
    Smart_p->Status_p->StatusBlock.T0.Ins = WriteBuffer_p[T0_INS_OFFSET];

    /* Initially we want to write the whole command; this then gets set by
     * the procedure bytes after the first write.
     */
    NextWriteSize = T0_CMD_LENGTH;

    /* Should be updated by procedure bytes */
    NextReadSize = 0;

    /* Transmit data under the control of procedure bytes */
    while (NextWriteSize > 0 && Error == ST_NO_ERROR)
    {
        /* Transmit command */
        Error = SMART_IO_Tx(Smart_p->IoHandle,
                            WriteBuffer_p,
                            NextWriteSize,
                            &NumberWritten,
                            Smart_p->TxTimeout,
                            NULL,
                            NULL
                           );
#if defined(STSMART_DEBUG)
        if (Error != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                        "%s: got error 0x%08x sending command:\n",
                         Smart_p->DeviceName,(U32)Error));
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                        "%s: tried to write %i bytes, wrote %i\n",
                        Smart_p->DeviceName,NextWriteSize, NumberWritten));
        }
#endif

        /* Update pointers */
        WriteBuffer_p += NumberWritten;
        NumberToWrite -= NumberWritten;
        *NumberWritten_p += NumberWritten;

        if (Error == ST_NO_ERROR)
        {
            /* Read procedure bytes */
            Error = SMART_T0_ProcessProcedureBytes(
                        Smart_p,
                        Smart_p->Status_p->StatusBlock.T0.Ins,
                        NumberToWrite,
                        NumberToRead,
                        Smart_p->Status_p->StatusBlock.T0.PB,
                        &Smart_p->Status_p->StatusBlock.T0.Size,
                        &NextWriteSize,
                        &NextReadSize
                        );
#if defined(STSMART_DEBUG)
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Received error 0x%x dealing with PTS bytes\n",
                            Error));
            }
#endif

            STOS_TaskDelay( ST_GetClocksPerSecond() / TIME_DELAY );
        }
    }

    TotalRead = 0;
    while (Error == ST_NO_ERROR && NextReadSize > 0 && NumberToRead > 0)
    {
        /* Read data response */
#if defined(STSMART_DEBUG)
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                     "Reading %i bytes\n", NextReadSize));
#endif
        Error = SMART_IO_Rx(Smart_p->IoHandle,
                            &ReadBuffer_p[TotalRead],
                            NextReadSize,
                            NumberRead_p,
                            Smart_p->RxTimeout,
                            NULL,
                            NULL
                           );
#if defined(STSMART_DEBUG)
        if (Error != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                        "%s: got error 0x%08x reading response:\n",
                         Smart_p->DeviceName));
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                        "%s: tried to read %i bytes, received %i\n",
                         Smart_p->DeviceName,NextReadSize, *NumberRead_p));
        }
#endif

        /* Handle procedure bytes */
        if (Error == ST_NO_ERROR)
        {
            NumberToRead -= *NumberRead_p;
            TotalRead += *NumberRead_p;
            NextReadSize = NumberToRead;
            Error = SMART_T0_ProcessProcedureBytes(
                Smart_p,
                Smart_p->Status_p->StatusBlock.T0.Ins,
                0,
                NumberToRead,
                Smart_p->Status_p->StatusBlock.T0.PB,
                &Smart_p->Status_p->StatusBlock.T0.Size,
                &NextWriteSize,
                &NextReadSize
                );
#if defined(STSMART_DEBUG)
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Received error 0x%x dealing with PTS bytes\n",
                            Error));
            }
#endif
        }
    }
    *NumberRead_p = TotalRead;

    return Error;
} /* SMART_T0_Transfer() */

/* Private Functions  ---------------------------------------------------- */

ST_ErrorCode_t SMART_T0_ProcessProcedureBytes(SMART_ControlBlock_t *Smart_p,
                                              IN U8 INS,
                                              IN U32 WriteSize,
                                              IN U32 ReadSize,
                                              OUT U8 *Buf_p,
                                              OUT U8 *Size_p,
                                              OUT U32 *NextWriteSize_p,
                                              OUT U32 *NextReadSize_p
                                             )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ST_ErrorCode_t CallError = ST_NO_ERROR;
    BOOL MoreProcedureBytes;
    U8 P, i;
    U32 Sz, NextWriteSize;

    /* Assume no more data to be sent by IFD */
    NextWriteSize = 0;

    /* Assume that the next read is all available bytes */
    *NextReadSize_p = ReadSize;

    i = 0;                              /* Procedure byte count */
    do                                  /* Process each procedure byte */
    {
        /* Assume no more procedure bytes to come */
        MoreProcedureBytes = FALSE;

#if defined(STSMART_DEBUG)
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Reading Response byte: Calling SMART_IO_Rx "));
#endif
        /* Try to read an available procedure byte */
        CallError = SMART_IO_Rx(Smart_p->IoHandle,
                                &P,
                                1,
                                &Sz,
                                Smart_p->RxTimeout,
                                NULL,
                                NULL
                               );

#if defined(STSMART_DEBUG)

        {
            char buf[255];
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Procedure byte: "));
            sprintf(buf, "[0x%02x]", P);
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, buf));
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Error= 0x%x",CallError)); /*TBR*/
        }
#endif

        /* Check procedure byte */
        if (CallError == ST_NO_ERROR)       /* Do we have a procedure byte? */
        {
            /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                         "Read procedure byte %i - 0x%02x", i, P));*/

            /* Set the procedure byte in the status structure */
            Buf_p[i] = P;

            if (i == 0)                 /* First procedure byte? */
            {
                /* Check for ACK byte */
                if ((INS ^ P) == 0x00) /* ACK == INS */
                {
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                  "ACK=INS: P[%d] = 0x%02x\n", i, P));
#endif
                    /* Vpp should be set idle - all bytes can go */
                    /* Internal Smart Card VPP not used */
                    if(Smart_p->CmdVppHandle != (STPIO_Handle_t)(NULL))
                    {
                        SMART_DisableVpp(Smart_p);
                        Smart_p->SmartParams.VppActive = FALSE;
                    }

                    if (WriteSize > 0)
                    {
                        /* Send all remaining bytes */
                        NextWriteSize = WriteSize;
                    }
                    else if (ReadSize == 0)
                    {
                        /* Await further procedure bytes */
                        MoreProcedureBytes = TRUE;
                        i = 0;
                        continue;
                    }
                }
                else if ((INS ^ P) == 0xFF) /* ACK == ~INS */
                {
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                  "ACK=~INS: P[%d] = 0x%02x\n", i, P));
#endif
                    /* Vpp should be set idle - one byte can go */
                    if(Smart_p->CmdVppHandle != (STPIO_Handle_t)NULL)
                    {
                        SMART_DisableVpp(Smart_p);
                        Smart_p->SmartParams.VppActive = FALSE;
                    }


                    if (WriteSize > 0)
                    {
                        /* Send the next byte only */
                        NextWriteSize = 1;
                    }
                    else if (ReadSize == 0)
                    {
                        /* No more bytes available - await further bytes */
                        MoreProcedureBytes = TRUE;
                        i = 0;
                        continue;
                    }
                    else
                    {
                        *NextReadSize_p = 1;
                    }
                }
                else if ((INS ^ P) == 0x01) /* ACK == INS+1 */
                {
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                  "ACK=INS+1: P[%d] = 0x%02x\n", i, P));
#endif
                    /* Vpp should be set active - all bytes can go */
                    if(Smart_p->CmdVppHandle != (STPIO_Handle_t)NULL)
                    {
                        SMART_EnableVpp(Smart_p);
                        Smart_p->SmartParams.VppActive = TRUE;
                    }


                    if (WriteSize > 0)
                    {
                        /* Send all remaining bytes */
                        NextWriteSize = WriteSize;
                    }
                    else if (ReadSize == 0)
                    {
                        /* No more bytes available - await further bytes */
                        MoreProcedureBytes = TRUE;
                        i = 0;
                        continue;
                    }
                }
                else if ((INS ^ P) == 0xFE) /* ACK == ~INS+1 */
                {
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                  "ACK=~INS+1: P[%d] = 0x%02x\n", i, P));
#endif
                    /* Vpp should be set active - next byte can go   */
                    if(Smart_p->CmdVppHandle != (STPIO_Handle_t)NULL)
                    {
                        SMART_EnableVpp(Smart_p);
                        Smart_p->SmartParams.VppActive = TRUE;
                    }


                    if (WriteSize > 0)
                    {
                        /* Send the next byte only */
                        NextWriteSize = 1;
                    }
                    else if (ReadSize == 0)
                    {
                        /* No more bytes available - await further bytes */
                        MoreProcedureBytes = TRUE;
                        i = 0;
                        continue;
                    }
                    else
                    {
                        *NextReadSize_p = 1;
                    }
                }
                else if ((P & 0xF0) == 0x60) /* SW1 or NULL */
                {
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Got SW1 0x%02x\n", P));
#endif
                    /* Get next procedure byte too */
                    MoreProcedureBytes = TRUE;

                    /* 0x60 is a 'null' byte, which indicates "please
                     * wait". Therefore we shouldn't modify the bytes
                     * expected when we get that. The others are errors.
                     */
                    if (P != 0x60)
                    {
                        *NextReadSize_p = 0;
                    }

                    switch (P)
                    {
                        case 0x60:
                            i = 0;      /* New sequence of bytes to come */
                            continue;
                        case 0x6E:      /* SW1 byte */
                            Error = STSMART_ERROR_INVALID_CLASS;
                            break;
                        case 0x6D:      /* SW1 byte */
                            Error = STSMART_ERROR_INVALID_CODE;
                            break;
                        case 0x68:      /* SW1 byte */
                            Error = STSMART_ERROR_INCORRECT_REFERENCE;
                            break;
                        case 0x67:      /* SW1 byte */
                            Error = STSMART_ERROR_INCORRECT_LENGTH;
                            break;
                        default:
                        case 0x6F:      /* SW1 byte */
                            Error = STSMART_ERROR_UNKNOWN;
                            break;
                    }
                }
                else if ((P & 0xF0) == 0x90) /* SW1 byte */
                {
#if defined(STSMART_DEBUG)
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Got SW1 byte 0x9x\n"));
#endif
                    /* Next procedure byte is the final one */
                    MoreProcedureBytes = TRUE;
                    *NextReadSize_p = 0;
                }
                else
                {
                    /* Unrecognised status byte */
                    Error = STSMART_ERROR_INVALID_STATUS_BYTE;
                }
            }
            else
            {
                /* This was SW2, so we should set the error status now */
            }
        }
        else
        {
            /* No answer from the card */
            Error = STSMART_ERROR_NO_ANSWER;
            break;
        }

        i++;                        /* Next procedure byte */
    } while (MoreProcedureBytes);

    /* Set number of procedure bytes received */
    *Size_p = i;

    /* Set number of bytes to write in next IO transfer */
    *NextWriteSize_p = NextWriteSize;

    /* Make sure that any errors from any functions we called
     * get passed back up.
     */
    if (Error == ST_NO_ERROR)
    {
        Error = CallError;
    }

    /* Return error code */
    return Error;
} /* SMART_T0_ProcessProcedureBytes() */

/* End of sct0.c */
