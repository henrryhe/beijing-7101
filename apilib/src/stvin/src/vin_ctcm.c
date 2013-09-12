/*******************************************************************************

File name   : vin_ctcm.c

Description : Common things commands source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
21 Mar 2000        Created                                           HG
26 Jul 2001        Duplicated/Extract from STVID                     JA
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include "stdio.h"
#endif /*ST_OSLINUX*/

#include "stddefs.h"
#include "vin_ctcm.h"
#include "vin_drv.h"

/* Private Types ------------------------------------------------------------ */

typedef const struct
{
    U32 SizeOfParams;
} ControllerCommand_t;


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private function prototypes ---------------------------------------------- */

static void PushChar(CommandsBuffer_t * const Buffer_p, const U8 Char);
static void PushWord(CommandsBuffer32_t * const Buffer_p, const U32 Word);
static U8 PopChar(CommandsBuffer_t * const Buffer_p);
static U32 PopWord(CommandsBuffer32_t * const Buffer_p);


/* Exported functions ------------------------------------------------------- */

/*******************************************************************************
Name        : vincom_PopCommand
Description : Pop a command from a circular buffer of commands
Parameters  : Buffer, pointer to data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't pop if empty
Returns     : ST_NO_ERROR if success,
              ST_ERROR_NO_MEMORY if buffer empty,
              ST_ERROR_BAD_PARAMETER if bad params
*******************************************************************************/
ST_ErrorCode_t vincom_PopCommand(CommandsBuffer_t * const Buffer_p, U8 * const Data_p)
{
    if (Buffer_p->UsedSize == 0)
    {
        /* Can't pop command: buffer empty */
        return(ST_ERROR_NO_MEMORY);
    }

    if (Data_p == NULL)
    {
        /* No param pointer provided: cannot return data ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Write command */
    *Data_p = PopChar(Buffer_p);

    return(ST_NO_ERROR);
} /* End of vincom_PopCommand() function */


/*******************************************************************************
Name        : vincom_PopCommand32
Description : Pop a command from a circular buffer of commands
Parameters  : Buffer, pointer to data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't pop if empty
Returns     : ST_NO_ERROR if success,
              ST_ERROR_NO_MEMORY if buffer empty,
              ST_ERROR_BAD_PARAMETER if bad params
*******************************************************************************/
ST_ErrorCode_t vincom_PopCommand32(CommandsBuffer32_t * const Buffer_p, U32 * const Data_p)
{
    if (Buffer_p->UsedSize == 0)
    {
        /* Can't pop command: buffer empty */
        return(ST_ERROR_NO_MEMORY);
    }

    if (Data_p == NULL)
    {
        /* No param pointer provided: cannot return data ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Write command */
    *Data_p = PopWord(Buffer_p);

    return(ST_NO_ERROR);
} /* End of vincom_PopCommand32() function */


/*******************************************************************************
Name        : vincom_PushCommand
Description : Push a command into a circular buffer of commands
Parameters  : Buffer, data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't push if full
Returns     : Nothing
*******************************************************************************/
void vincom_PushCommand(CommandsBuffer_t * const Buffer_p, const U8 Data)
{
    if (Buffer_p->UsedSize + 1 > Buffer_p->TotalSize)
    {
        /* Can't push command: buffer full */
        return;
    }

    PushChar(Buffer_p, Data);
} /* End of vincom_PushCommand() function */


/*******************************************************************************
Name        : vincom_PushCommand32
Description : Push a command into a circular buffer of commands
Parameters  : Buffer, data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't push if full
Returns     : Nothing
*******************************************************************************/
void vincom_PushCommand32(CommandsBuffer32_t * const Buffer_p, const U32 Data)
{
    if (Buffer_p->UsedSize + 1 > Buffer_p->TotalSize)
    {
        /* Can't push command: buffer full */
        return;
    }

    PushWord(Buffer_p, Data);
} /* End of vincom_PushCommand32() function */




/* Private functions -------------------------------------------------------- */


/*******************************************************************************
Name        : PopChar
Description : Pop a character from the circular buffer
Parameters  : pointer to buffer
Assumptions : Buffer is not empty
Limitations :
Returns     : Character
*******************************************************************************/
static U8 PopChar(CommandsBuffer_t * const Buffer_p)
{
    U8 Pop;

    /* Protect access to BeginPointer_p and UsedSize */
    STOS_InterruptLock();

    /* Read character */
    Pop = *(Buffer_p->BeginPointer_p);

    if ((Buffer_p->BeginPointer_p) >= (Buffer_p->Data_p + Buffer_p->TotalSize - 1))
    {
        Buffer_p->BeginPointer_p = Buffer_p->Data_p;
    }
    else
    {
        (Buffer_p->BeginPointer_p)++;
    }

    /* Update size */
    (Buffer_p->UsedSize)--;

    /* Un-protect access to BeginPointer_p and UsedSize */
    STOS_InterruptUnlock();

    /* Return character */
    return(Pop);
} /* End of PopChar() function */


/*******************************************************************************
Name        : PopWord
Description : Pop a U32 word from the circular buffer
Parameters  : pointer to buffer
Assumptions : Buffer is not empty
Limitations :
Returns     : Character
*******************************************************************************/
static U32 PopWord(CommandsBuffer32_t * const Buffer_p)
{
    U32 Pop;

    /* Protect access to BeginPointer_p and UsedSize */
    STOS_InterruptLock();

    /* Read character */
    Pop = *(Buffer_p->BeginPointer_p);

    if ((Buffer_p->BeginPointer_p) >= (Buffer_p->Data_p + Buffer_p->TotalSize - 1))
    {
        Buffer_p->BeginPointer_p = Buffer_p->Data_p;
    }
    else
    {
        (Buffer_p->BeginPointer_p)++;
    }

    /* Update size */
    (Buffer_p->UsedSize)--;

    /* Un-protect access to BeginPointer_p and UsedSize */
    STOS_InterruptUnlock();

    /* Return character */
    return(Pop);
} /* End of PopWord() function */


/*******************************************************************************
Name        : PushChar
Description : Push a character into the circular buffer
Parameters  : pointer to buffer, character
Assumptions : buffer is not full
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PushChar(CommandsBuffer_t * const Buffer_p, const U8 Char)
{
    U8 * EndPointer_p;

    /* Protect access to BeginPointer_p and UsedSize */
    STOS_InterruptLock();

    EndPointer_p = Buffer_p->BeginPointer_p + Buffer_p->UsedSize;

    /* Update size */
    (Buffer_p->UsedSize)++;

    if (EndPointer_p >= (Buffer_p->Data_p + Buffer_p->TotalSize))
    {
        EndPointer_p -= Buffer_p->TotalSize;
    }

    /* Write character */
    *EndPointer_p = Char;

    /* Monitor max used size in commands buffer */
    if (Buffer_p->UsedSize > Buffer_p->MaxUsedSize)
    {
        Buffer_p->MaxUsedSize = Buffer_p->UsedSize;
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    STOS_InterruptUnlock();

} /* End of PushChar() function */


/*******************************************************************************
Name        : PushWord
Description : Push a U32 word into the circular buffer
Parameters  : pointer to buffer, U32 word
Assumptions : buffer is not full
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PushWord(CommandsBuffer32_t * const Buffer_p, const U32 Word)
{
    U32 * EndPointer_p;

    /* Protect access to BeginPointer_p and UsedSize */
    STOS_InterruptLock();

    EndPointer_p = Buffer_p->BeginPointer_p + Buffer_p->UsedSize;

    /* Update size */
    (Buffer_p->UsedSize)++;

    if (EndPointer_p >= (Buffer_p->Data_p + Buffer_p->TotalSize))
    {
        EndPointer_p -= Buffer_p->TotalSize;
    }

    /* Write character */
    *EndPointer_p = Word;

    /* Monitor max used size in commands buffer */
    if (Buffer_p->UsedSize > Buffer_p->MaxUsedSize)
    {
        Buffer_p->MaxUsedSize = Buffer_p->UsedSize;
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    STOS_InterruptUnlock();

} /* End of PushWord() function */


/* End of vin_ctcm.c */
