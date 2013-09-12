/*******************************************************************************

File name   : tbx_in.c

Description : Source of Input feature of toolbox API

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
10 Mar 1999         Created                                          HG
23 Apr 1999         Added InputPollChar
                    Implemented inputs with Pollkey                  HG
07 Jul 1999         Modify characters use in InputStr:
                    -add end string on '\n'
                    -add backspace on 127
                    -add tab to space conversion                     HG
11 Aug 1999         Change Semaphores structure (now one semaphore
                    for each I/O device). Implement UART inputs      HG
 8 Sep 1999         Exit debug functions if debugger is not
                    connected (DEBUG_NOT_CONNECTED)                  HG
19 Oct 1999         Change Semaphores structure (now two semaphores
                    for each I/O device: one In and one Out)         HG
11 Jan 2000         Added outputs semaphores lock in InputStr (echo)
                    Use task_delay() of about 0.1s instead of 64us   HG
28 Jan 2002         DDTS GNBvd10702 "Use ST_GetClocksPerSecond       HS
 *                  instead of Hardcoded value"
 *                  Add STTBX_NO_UART compilation flag.
*******************************************************************************/

 #if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Includes ----------------------------------------------------------------- */

#ifdef NATIVE_CORE
#ifdef __WIN32/* case: native Windows host */
#include <conio.h>
#else /* case: native Linux host */
#include <fcntl.h>
#endif
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "stddefs.h"
#ifdef ST_OS20
#include <debug.h>
#endif /* ST_OS20 */
#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include <os21debug.h>
#endif /* ST_OS21 && !ST_OSWINCE*/

#undef STTBX_INPUT
#include "sttbx.h"

#include "tbx_init.h"
#include "tbx_out.h"
#include "tbx_filt.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#ifdef ST_OS21
#define TASK_DESCHEDULING_TIME    (osclock_t)(sttbx_ClocksPerSecond / 10) /* 0.1s */
#endif
#ifdef ST_OS20
#define TASK_DESCHEDULING_TIME    (sttbx_ClocksPerSecond / 10) /* 0.1s */
#endif

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */
semaphore_t *sttbx_LockDCUIn_p;    /* DCU input is independant from output */

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
#ifdef ST_OS21
long int debugpollkey(long int *LongKey_p);
#endif
BOOL sttbx_InputPollChar(const STTBX_ModuleID_t ModuleID, char * const Value_p);
void sttbx_InputChar(const STTBX_ModuleID_t ModuleID, char * const Value_p);
U16 sttbx_InputStr(const STTBX_ModuleID_t ModuleID, char * const Buffer_p, const U16 BufferSize);

/* Functions ---------------------------------------------------------------- */

#ifdef ST_OS21
/*
--------------------------------------------------------------------------------
Wrapper function : Get the hitten key
--------------------------------------------------------------------------------
*/
long int debugpollkey(long int *LongKey_p)
{
    int Keycode = 0;
    int Status;

     /* Must use console input as stdin does not allow single keypress
       to be detected.
     */
#ifdef NATIVE_CORE
#ifdef __WIN32/* case: native Windows host */
    if(kbhit())
    {
        *(int*)LongKey_p = getche();
        Status = 1;
        *LongKey_p &= 0x000000FF;
    }
    else
    {
		*LongKey_p = 0;
        Status = 0;
    }

#else /* case: native Linux host */
    int fct_ctrl;
    fct_ctrl=fcntl(0,F_GETFL,0);
    fcntl(0,F_SETFL,fct_ctrl|O_NONBLOCK);


    *LongKey_p = getchar();
    if(*LongKey_p != EOF)
    {
        Status = 1;
        *LongKey_p &= 0x000000FF;
    }
    else
    {
		*LongKey_p = 0;
        Status = 0;
    }

    fcntl(0,F_SETFL,fct_ctrl);

#endif
#else
#ifdef CONFIG_POSIX
    /* Use Posix function which is available only with debugger sh4gdb */
    _SH_posix_PollKey(&Keycode);
    if (Keycode == 0)
	{
		*LongKey_p = 0;
        Status = 0;
	}
	else
	{
        *LongKey_p = 0xff & (long int)Keycode;
        Status = 1;
	}

#else/*ST200*/
#ifndef ST_OSWINCE
    /* Use ANSI function, available with sh4run and sh4gdb                  */
    /* Caution : Return key must be hit to end the input !                  */
    /*           and the 2 next scanf will return 0x0D 0x0A !!! (July 2004) */
    *LongKey_p = getchar();
    /*scanf("%c", (char *)LongKey_p); */
    *LongKey_p &= 0x000000FF;
     Status = 1;
#else /* !ST_OSWINCE */

    *LongKey_p = Wce_Console_Getchar(FALSE); /* not blocking */
    if (*LongKey_p != 0)
        Status = 1;
#endif /* ST_OSWINCE */
#endif /* CONFIG_POSIX */
#endif /* NATIVE_CORE */
	return (Status);
}
#endif /* ST_OS21 */


/*
--------------------------------------------------------------------------------
Checks if the selected input has a character available.
-gets the character and return TRUE if there is one
-returns FALSE if there isn't
--------------------------------------------------------------------------------
*/
BOOL sttbx_InputPollChar(const STTBX_ModuleID_t ModuleID, char * const Value_p)
{
    long int LongKey;
    char CharKey;
    long int ErrorCode = 0;
#ifndef STTBX_NO_UART
    U32 UARTNumberRead;
    ST_ErrorCode_t Err = ST_NO_ERROR;
#endif /* #ifndef STTBX_NO_UART  */
    BOOL GotSomething = FALSE;
    STTBX_Device_t Device;

    /* Select current input device */
    Device = sttbx_GlobalCurrentInputDevice;
    sttbx_ApplyInputFiltersOnInputDevice(ModuleID, &Device);
    sttbx_ApplyRedirectionsOnDevice(&Device);

    /* Exit if no input device is selected */
    if (Device == STTBX_DEVICE_NULL)
    {
        return(FALSE);
    }

    /* Input from respective device */
    switch(Device)
    {
        case STTBX_DEVICE_DCU :
            SEM_WAIT(sttbx_LockDCUIn_p);
            /* The while is to ensure the good execution of debugpollkey in case
               other debug functions are used somewhere else at the same time.
               Try debugpollkey until success... (debugpollkey returns 1 if a key
               was available, 0 if no key was available, other values are errors.) */
            /* However, stop if the debugger is not connected. */
            do
            {
                ErrorCode = debugpollkey(&LongKey);
#ifdef ARCHITECTURE_ST200
                if ((ErrorCode == 1) && ( LongKey == 0x0A))
                {
                    *((char *)&LongKey) = '\0';
                    ErrorCode = 0;
                }
#endif
                /* Test on '\0' is a workaround for a debugpollkey bug with DCU toolset 1.6.2 */
                if ((ErrorCode == 1) && (((char *) LongKey) == '\0'))
                {
                    ErrorCode = 0;
                }
                if (ErrorCode == 1)
                {
                    /* One character was available: cast it */
                    CharKey = (char) LongKey;
                    /* Store the character */
                    *Value_p = CharKey;
                    /* Return code when a character was available */
                    GotSomething = TRUE;
                }
                else if ((ErrorCode != 0) && (ErrorCode != DEBUG_NOT_CONNECTED))
                {
                    /* Error: wait and let time for other tasks */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
            } while ((ErrorCode != 1) && (ErrorCode != 0) && (ErrorCode != DEBUG_NOT_CONNECTED));
            SEM_SIGNAL(sttbx_LockDCUIn_p);
            break;

#ifndef STTBX_NO_UART
        case STTBX_DEVICE_UART :
            SEM_WAIT(sttbx_LockUartIn_p);
            do
            {
                /* Use time-out as short as possible (1ms) to 'poll' for the character */
                Err = STUART_Read(sttbx_GlobalUartHandle, (U8 *) &CharKey, 1, &UARTNumberRead, 1);
                if ((Err == ST_NO_ERROR) || (Err == ST_ERROR_TIMEOUT))
                {
                    if (UARTNumberRead == 1)
                    {
                        /* One character was available: store it */
                        *Value_p = CharKey;
                        /* Return code when a character was available */
                        GotSomething = TRUE;
                    }
                }
                else if (Err != ST_ERROR_INVALID_HANDLE)
                {
                    /* Error: wait and let time for other tasks */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
                /* Exit if ST_NO_ERROR: success with character input
                   OR ST_ERROR_TIMEOUT: success without character input
                   OR ST_ERROR_INVALID_HANDLE: UART not initialised: just pass out... */
            } while ((Err != ST_NO_ERROR) && (Err != ST_ERROR_TIMEOUT) && (Err != ST_ERROR_INVALID_HANDLE));
            SEM_SIGNAL(sttbx_LockUartIn_p);
            break;

        case STTBX_DEVICE_UART2 :
            SEM_WAIT(sttbx_LockUart2In_p);
            do
            {
                /* Use time-out as short as possible (1ms) to 'poll' for the character */
                Err = STUART_Read(sttbx_GlobalUart2Handle, (U8 *) &CharKey, 1, &UARTNumberRead, 1);
                if ((Err == ST_NO_ERROR) || (Err == ST_ERROR_TIMEOUT))
                {
                    if (UARTNumberRead == 1)
                    {
                        /* One character was available: store it */
                        *Value_p = CharKey;
                        /* Return code when a character was available */
                        GotSomething = TRUE;
                    }
                }
                else if (Err != ST_ERROR_INVALID_HANDLE)
                {
                    /* Error: wait and let time for other tasks */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
                /* Exit if ST_NO_ERROR: success with character input
                   OR ST_ERROR_TIMEOUT: success without character input
                   OR ST_ERROR_INVALID_HANDLE: UART not initialised: just pass out... */
            } while ((Err != ST_NO_ERROR) && (Err != ST_ERROR_TIMEOUT) && (Err != ST_ERROR_INVALID_HANDLE));
            SEM_SIGNAL(sttbx_LockUart2In_p);
            break;
#endif /* #ifndef STTBX_NO_UART  */

#if 0 /* Not supported yet */
        case STTBX_DEVICE_I1284 :
            SEM_WAIT(sttbx_LockI1284_p);
            /* Not supported yet */
            SEM_SIGNAL(sttbx_LockI1284_p);
            break;
#endif /* Not supported yet */

        default :
            break; /* Unknown input device */
    }

    return(GotSomething);
} /* sttbx_InputPollChar( */

/*
--------------------------------------------------------------------------------
Gets a character from the selected input
--------------------------------------------------------------------------------
*/
void sttbx_InputChar(const STTBX_ModuleID_t ModuleID, char * const Value_p)
{
    long int LongKey;
    char CharKey;
#ifndef STTBX_NO_UART
    U32 UARTNumberRead;
    ST_ErrorCode_t Err = ST_NO_ERROR;
#endif /* #ifndef STTBX_NO_UART  */
    long int ErrorCode = 0;
    STTBX_Device_t Device;

    /* Select current input device */
    Device = sttbx_GlobalCurrentInputDevice;
    sttbx_ApplyInputFiltersOnInputDevice(ModuleID, &Device);
    sttbx_ApplyRedirectionsOnDevice(&Device);
    /* Exit if no input device is selected */
    if (Device == STTBX_DEVICE_NULL)
    {
        return;
    }

    /* Input from respective device */
    switch(Device)
    {
        case STTBX_DEVICE_DCU :
            SEM_WAIT(sttbx_LockDCUIn_p);
            /* The while is to ensure the good execution of debugpollkey in case
               other debug functions are used somewhere else at the same time.
               Try debugpollkey until success... (debugpollkey returns 1 if a key
               was available, 0 if no key was available, other values are errors.) */
            /* However, stop if the debugger is not connected. */
#ifndef ST_OSWINCE
            do
            {
                ErrorCode = debugpollkey(&LongKey);
                /* Test on '\0' is a workaround for a debugpollkey bug with DCU toolset 1.6.2 */
                if ((ErrorCode == 1) && (((char *) LongKey) == '\0'))
                {
                    ErrorCode = 0;

                }
                if (ErrorCode == 1)
                {
                    /* One character was available: cast it */
                    CharKey = (char) LongKey;
                    /* One character was available: store the character */
                    *Value_p = CharKey;

                }
                /** (FQ-ddts 2745:wait cnx)
                 else if (ErrorCode == DEBUG_NOT_CONNECTED)
                { **/
                    /* No character is available but none will ever come ! : store empty character */
                    /** *Value_p = '\0';
                } **/
                else
                {
                    /* No character available or error: wait and let time for other tasks */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
            } while (ErrorCode != 1);
            /** } while ((ErrorCode != 1) && (ErrorCode != DEBUG_NOT_CONNECTED)); **/
#else /* ! ST_OSWINCE */
            *Value_p = Wce_Console_Getchar(TRUE); /* blocking */
#endif
            SEM_SIGNAL(sttbx_LockDCUIn_p);
            break;

#ifndef STTBX_NO_UART
        case STTBX_DEVICE_UART :
            SEM_WAIT(sttbx_LockUartIn_p);
            do
            {
                /* Use time-out as short as possible (1ms) to 'poll' for the character */
                Err = STUART_Read(sttbx_GlobalUartHandle, (U8 *) &CharKey, 1, &UARTNumberRead, 1);
                if ((Err == ST_NO_ERROR) && (UARTNumberRead == 1))
                {
                    /* One character was available: store it */
                    *Value_p = CharKey;
                }
                else if (Err != ST_ERROR_INVALID_HANDLE)
                {
                    /* No character available or error: wait and let time for other tasks */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
                /* Exit if ST_NO_ERROR and read 1 character: success with character input
                   OR ST_ERROR_INVALID_HANDLE: UART not initialised: just pass out... */
            } while (!((Err == ST_NO_ERROR) && (UARTNumberRead == 1)) && (Err != ST_ERROR_INVALID_HANDLE));
            SEM_SIGNAL(sttbx_LockUartIn_p);
            break;

        case STTBX_DEVICE_UART2 :
            SEM_WAIT(sttbx_LockUart2In_p);
            do
            {
                /* Use time-out as short as possible (1ms) to 'poll' for the character */
                Err = STUART_Read(sttbx_GlobalUart2Handle, (U8 *) &CharKey, 1, &UARTNumberRead, 1);
                if ((Err == ST_NO_ERROR) && (UARTNumberRead == 1))
                {
                    /* One character was available: store it */
                    *Value_p = CharKey;
                }
                else if (Err != ST_ERROR_INVALID_HANDLE)
                {
                    /* No character available or error: wait and let time for other tasks */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
                /* Exit if ST_NO_ERROR and read 1 character: success with character input
                   OR ST_ERROR_INVALID_HANDLE: UART not initialised: just pass out... */
            } while (!((Err == ST_NO_ERROR) && (UARTNumberRead == 1)) && (Err != ST_ERROR_INVALID_HANDLE));
            SEM_SIGNAL(sttbx_LockUart2In_p);
            break;
#endif /* #ifndef STTBX_NO_UART  */

#if 0 /* Not supported yet */
        case STTBX_DEVICE_I1284 :
            SEM_WAIT(sttbx_LockI1284_p);
            /* Not supported yet */
            SEM_SIGNAL(sttbx_LockI1284_p);
            break;
#endif /* Not supported yet */

        default :
            break; /* Unknown input device */
    }
} /* end of sttbx_InputChar() */


/*
--------------------------------------------------------------------------------
Gets a string of characters from the selected input
Returns size of the input string (excluding the '\0' closing character)
--------------------------------------------------------------------------------
*/
U16 sttbx_InputStr(const STTBX_ModuleID_t ModuleID, char * const Buffer_p, const U16 BufferSize)
{
    long int LongKey;
    char CharKey[2] = {'h', '\0'};  /* '\0' used to be able to use functions which need strings */
    U16 InputSize = 0;
#ifndef STTBX_NO_UART
    U32 UARTNumberRead;
    ST_ErrorCode_t Err = ST_NO_ERROR;
#endif /* #ifndef STTBX_NO_UART  */
    long int ErrorCode = 0;
    STTBX_Device_t Device;
    char string[10];
    /* Determine input device */
    Device = sttbx_GlobalCurrentInputDevice;
    sttbx_ApplyInputFiltersOnInputDevice(ModuleID, &Device);
    sttbx_ApplyRedirectionsOnDevice(&Device);

    /* Exit if no input device is selected */
    if (Device == STTBX_DEVICE_NULL)
    {
        return 0;
    }

    /* Input from respective device */
    switch(Device)
    {
        case STTBX_DEVICE_DCU :
            SEM_WAIT(sttbx_LockDCUIn_p);

            /* The while is to ensure the good execution of debuggets in case
               other debug functions are used somewhere else at the same time.
               Try debugpollkey until success... (debugpollkey returns 1 if a key
               was available, 0 if no key was available, other values are errors.) */
            /* However, stop if the debugger is not connected. */
            do
            {
                ErrorCode = debugpollkey(&LongKey);
                /* Test on '\0' is a workaround for a debugpollkey bug with DCU toolset 1.6.2 */
                if ((ErrorCode == 1) && (((char *) LongKey) == '\0'))
                {
                    ErrorCode = 0;
                    continue;
                }
                if (ErrorCode == 1)
                {
                    /* One character was available: cast it */
                    CharKey[0] = (char) LongKey;
                    if ((CharKey[0] == '\b') || (CharKey[0] == 127))
                    {
                        /* Backspace character (\b on PC, 127 on UNIX): only applicable if Buffer_p is not empty */
                        if (InputSize > 0)
                        {
                            /* Backspace applicable: forget last entered character */
                            InputSize--;
                          /* Echo new input: delete previous character */

#ifndef CONFIG_POSIX
                            strcpy(string,"\b \b");

#else
                            strcpy(string," \b");
#endif
                            sttbx_Output(STTBX_DEVICE_DCU,string);
                        }
                        else
                        {
                            /* Backspace not applicable: echo an alert character */
#ifndef CONFIG_POSIX
                            strcpy(string,"\a");
#else
                            strcpy(string,"\b\a");
#endif
                            sttbx_Output(STTBX_DEVICE_DCU,string);
                        }
                    }
                    else if ((CharKey[0] == '\r') || (CharKey[0] == '\n'))
                    {
                        /* End of entry character (\r on PC, \n on UNIX): close the string */
                        *(Buffer_p + InputSize) = '\0';
                        /* Echo new line character */
                        strcpy(string,"\r\n");
                        sttbx_Output(STTBX_DEVICE_DCU,string);
                        /*sttbx_OutputDCUNoSemaphore("\r\n");*/
                        /* The "do while" should end now */
                    }
                    else
                    {
                        /* Not end of entry nor backspace: process character */
                        if (InputSize < (BufferSize - 1))
                        {
                            /* There is still room in the character buffer: take character */
                            *(Buffer_p + InputSize) = CharKey[0]; /* Store character in the string */
                            InputSize++;
                            /* Change tabulation into space for echo (this spares multiple space management) */
                            if (CharKey[0] == '\t')
                            {
                                CharKey[0] = ' ';
                            }
#ifndef CONFIG_POSIX
                            /* Echo new input */
                            sttbx_Output(STTBX_DEVICE_DCU,&CharKey[0]);
                            /*sttbx_OutputDCUNoSemaphore(&CharKey[0]);*/
#endif
                        }
                        else
                        {
#ifndef CONFIG_POSIX
                            /* No room in the character buffer: echo an alert character */
                            strcpy(string,"\a");
#else
                            /* Echo new input: delete last echoed character and do alert */
                            strcpy(string,"\b\a");
#endif
                            sttbx_Output(STTBX_DEVICE_DCU,string);
                        }
                    }
                }
                /** (FQ-ddts 2745:wait cnx)
                else if (ErrorCode == DEBUG_NOT_CONNECTED)
                { **/
                    /* No characted will ever come as the debugger is not connected: close the string */
                    /** *(Buffer_p + InputSize) = '\0'; **/
                    /* Echo nothing */
/*                    sttbx_OutputDCUNoSemaphore("\r\n");*/
                    /* The "do while" should end now */
                /** } **/
                else
                {
                    /* No character available or error: wait and let time for other tasks */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
            } while ((CharKey[0] != '\r') && (CharKey[0] != '\n'));
            /** } while ((CharKey[0] != '\r') && (CharKey[0] != '\n') && (ErrorCode != DEBUG_NOT_CONNECTED)); **/
            /* In any case, finish only when carriage return is given, or if not connected */
            SEM_SIGNAL(sttbx_LockDCUIn_p);
            break;

#ifndef STTBX_NO_UART
        case STTBX_DEVICE_UART :
            SEM_WAIT(sttbx_LockUartIn_p);
            do
            {
                /* Use time-out as short as possible (1ms) to 'poll' for the character */
                Err = STUART_Read(sttbx_GlobalUartHandle, (U8 *) &CharKey[0], 1, &UARTNumberRead, 1);
                if (Err == ST_ERROR_INVALID_HANDLE)
                {
                    /* Exit if ST_ERROR_INVALID_HANDLE: UART not initialised: just pass out... */
                    CharKey[0] = '\n'; /* Do as if carriage return was entered. */
                }
                if (((Err == ST_NO_ERROR) && (UARTNumberRead == 1)) || (Err == ST_ERROR_INVALID_HANDLE))
                {
                    /* One character was available: deal with it */
                    if ((CharKey[0] == '\b') || (CharKey[0] == 127))
                    {
                        /* Backspace character (\b on PC, 127 on UNIX): only applicable if Buffer_p is not empty */
                        if (InputSize > 0)
                        {
                            /* Backspace applicable: forget last entered character */
                            InputSize--;
                            /* Echo new input: delete previous character */
                            strcpy(string,"\b \b");
                            sttbx_Output(STTBX_DEVICE_UART,string);
                            /*sttbx_OutputUartNoSemaphore("\b \b");*/
                        }
                        else
                        {
                            /* Backspace not applicable: echo an alert character */
                            strcpy(string,"\a");
                            sttbx_Output(STTBX_DEVICE_UART,string);
                            /*sttbx_OutputUartNoSemaphore("\a");*/
                        }
                    }
                    else if ((CharKey[0] == '\r') || (CharKey[0] == '\n'))
                    {
                        /* End of entry character (\r on PC, \n on UNIX): close the string */
                        *(Buffer_p + InputSize) = '\0';
                        /* Echo new line character */
                        strcpy(string,"\r\n");
                        sttbx_Output(STTBX_DEVICE_UART,string);
                        /*sttbx_OutputUartNoSemaphore("\r\n");*/
                        /* The "do while" should end now */
                    }
                    else
                    {
                        /* Not end of entry nor backspace: process character */
                        if (InputSize < (BufferSize - 1))
                        {
                            /* There is still room in the character buffer: take character */
                            *(Buffer_p + InputSize) = CharKey[0]; /* Store character in the string */
                            InputSize++;
                            /* Change tabulation into space for echo (this spares multiple space management) */
                            if (CharKey[0] == '\t')
                            {
                                CharKey[0] = ' ';
                            }
                            /* Echo new input */
                            sttbx_Output(STTBX_DEVICE_UART,&CharKey[0]);
                            /*sttbx_OutputUartNoSemaphore(&CharKey[0]);*/
                        }
                        else
                        {
                            /* No room in the character buffer: echo an alert character */
                            strcpy(string,"\a");
                            sttbx_Output(STTBX_DEVICE_UART,string);
                            /*sttbx_OutputUartNoSemaphore("\a");*/
                        }
                    }
                }
                else
                {
                    /* No character available or error: wait and let time for other tasks */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
            } while ((CharKey[0] != '\r') && (CharKey[0] != '\n')); /* In any case, finish only when carriage return is given */
            SEM_SIGNAL(sttbx_LockUartIn_p);
            break;

        case STTBX_DEVICE_UART2 :
            SEM_WAIT(sttbx_LockUart2In_p);
            do
            {
                /* Use time-out as short as possible (1ms) to 'poll' for the character */
                Err = STUART_Read(sttbx_GlobalUart2Handle, (U8 *) &CharKey[0], 1, &UARTNumberRead, 1);
                if (Err == ST_ERROR_INVALID_HANDLE)
                {
                    /* Exit if ST_ERROR_INVALID_HANDLE: UART not initialised: just pass out... */
                    CharKey[0] = '\n'; /* Do as if carrige return was entered. */
                }
                if (((Err == ST_NO_ERROR) && (UARTNumberRead == 1)) || (Err == ST_ERROR_INVALID_HANDLE))
                {
                    /* One character was available: deal with it */
                    if ((CharKey[0] == '\b') || (CharKey[0] == 127))
                    {
                        /* Backspace character (\b on PC, 127 on UNIX): only applicable if Buffer_p is not empty */
                        if (InputSize > 0)
                        {
                            /* Backspace applicable: forget last entered character */
                            InputSize--;
                            /* Echo new input: delete previous character */
                            strcpy(string,"\b \b");
                            sttbx_Output(STTBX_DEVICE_UART2,string);
                            /*sttbx_OutputUart2NoSemaphore("\b \b");*/
                        }
                        else
                        {
                            /* Backspace not applicable: echo an alert character */
                            strcpy(string,"\a");
                            sttbx_Output(STTBX_DEVICE_UART2,string);
                            /*sttbx_OutputUart2NoSemaphore("\a");*/
                        }
                    }
                    else if ((CharKey[0] == '\r') || (CharKey[0] == '\n'))
                    {
                        /* End of entry character (\r on PC, \n on UNIX): close the string */
                        *(Buffer_p + InputSize) = '\0';
                        /* Echo new line character */
                        strcpy(string,"\r\n");
                        sttbx_Output(STTBX_DEVICE_UART2,string);
                        /*sttbx_OutputUart2NoSemaphore("\r\n");*/
                        /* The "do while" should end now */
                    }
                    else
                    {
                        /* Not end of entry nor backspace: process character */
                        if (InputSize < (BufferSize - 1))
                        {
                            /* There is still room in the character buffer: take character */
                            *(Buffer_p + InputSize) = CharKey[0]; /* Store character in the string */
                            InputSize++;
                            /* Change tabulation into space for echo (this spares multiple space management) */
                            if (CharKey[0] == '\t')
                            {
                                CharKey[0] = ' ';
                            }
                            /* Echo new input */
                            sttbx_Output(STTBX_DEVICE_UART2,&CharKey[0]);
                            /*sttbx_OutputUart2NoSemaphore(&CharKey[0]);*/
                        }
                        else
                        {
                            /* No room in the character buffer: echo an alert character */
                            strcpy(string,"\a");
                            sttbx_Output(STTBX_DEVICE_UART2,string);
                            /*sttbx_OutputUart2NoSemaphore("\a");*/
                        }
                    }
                }
                else
                {
                    /* No character available or error: wait and let time for other tasks */
                    task_delay(TASK_DESCHEDULING_TIME);
                }
            } while ((CharKey[0] != '\r') && (CharKey[0] != '\n')); /* In any case, finish only when carriage return is given */
            SEM_SIGNAL(sttbx_LockUart2In_p);
            break;
#endif /* #ifndef STTBX_NO_UART  */

#if 0 /* Not supported yet */
        case STTBX_DEVICE_I1284 :
            SEM_WAIT(sttbx_LockI1284_p);
            /* Not supported yet */
            SEM_SIGNAL(sttbx_LockI1284_p);
            break;
#endif /* Not supported yet */

        default :
            break; /* Unknown input device */
    }
    return (InputSize);
} /* end of sttbx_InputStr() */
#endif  /* #ifndef ST_OSLINUX */
/* End of tbx_in.c */



