/*******************************************************************************

File name   : cltbx.c

Description : TBX configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
05 Fev 2002        Created                                           HSdLM
*******************************************************************************/
#if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "cltbx.h"
#ifndef STTBX_NO_UART
#include "cluart.h"
#endif /* #ifndef STTBX_NO_UART  */

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

extern ST_Partition_t *DriverPartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : TBX_Init
Description : Initialise the STTBX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL TBX_Init(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STTBX_InitParams_t InitParams;
    STTBX_BuffersConfigParams_t BuffParams;
    BOOL RetOk = TRUE;

#ifndef STTBX_NO_UART
    InitParams.SupportedDevices = STTBX_DEVICE_DCU | STTBX_DEVICE_UART;
    strcpy(InitParams.UartDeviceName, STUART_DEVICE_NAME);
#else
    InitParams.SupportedDevices = STTBX_DEVICE_DCU;
#endif /* #ifndef STTBX_NO_UART */

    InitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    InitParams.DefaultInputDevice = STTBX_DEVICE_DCU;
    BuffParams.Enable = FALSE;

    InitParams.CPUPartition_p = DriverPartition_p;

    ErrorCode = STTBX_Init(STTBX_DEVICE_NAME, &InitParams);
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Print(("STTBX initialized,\trevision=%s\n",STTBX_GetRevision()));
        BuffParams.KeepOldestWhenFull = TRUE;
        STTBX_SetBuffersConfig (&BuffParams);
#ifdef USE_UART_AS_STDIO
        STTBX_SetRedirection(STTBX_DEVICE_DCU, STTBX_DEVICE_UART);
#endif /* REDIRECT_TO_UART */
    }
    else
    {
        printf("STTBX_Init(%s) error=0x%0x !\n", STTBX_DEVICE_NAME, ErrorCode);
        RetOk = FALSE;
    }
    return(RetOk);
} /* End of TBX_Init() */


/*******************************************************************************
Name        : TBX_Term
Description : Initialise the STTBX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void TBX_Term(void)
{
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;
    STTBX_TermParams_t TermParams;

    ErrCode = STTBX_Term( STTBX_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_NO_ERROR)
    {
        printf("STTBX_Term(%s): ok\n", STTBX_DEVICE_NAME);
    }
    else
    {
        printf("STTBX_Term(%s): error=0x%0x !\n", STTBX_DEVICE_NAME, ErrCode);
    }
} /* end TBX_Term() */
#endif  /* #ifndef ST_OSLINUX */

/* End of cltbx.c */
