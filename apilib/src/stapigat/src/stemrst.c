/*******************************************************************************

File name   : stemrst.c

Description : STEM reset configuration source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
02 Apr 2003        Created                                          HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#ifdef USE_STEM_RESET

#include "stdevice.h"

#include "stddefs.h"

#include "stboot.h"
#include "sttbx.h"

#include "cli2c.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* this is the condition set in stboot.h: */
#if !(defined (ST_7020) && !defined(mb290) && !defined(mb295)) /* db573 7020 STEM */
#error ERROR: no STEM reset known on this platform !
#endif

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : StemReset_Init
Description : Initialise the STEM reset
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL StemReset_Init(void)
{
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;
    BOOL               RetOk = TRUE;

    STTBX_Print(( "STEM reset: " ));

    /* STBOOT has to use I2C to reset the 7020,
        and sets up the DAC mode we want (I2S) for audio */

    ErrorCode = STBOOT_Init7020STEM(STI2C_FRONT_DEVICE_NAME, DB573_SDRAM_FREQUENCY);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("STBOOT_Init7020STEM failed with error 0x%08X !\n", ErrorCode));
        RetOk = FALSE;
    }
    if (RetOk)
    {
        STTBX_Print(( "initialized"));
    }
    STTBX_Print(("\n"));
    return(RetOk);
} /* end StemReset_Init() */

#endif /* USE_STEM_RESET */
/* End of stemrst.c */
