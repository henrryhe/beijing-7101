/*******************************************************************************
File Name   : board.c

Description : Init board depending of the PLATFORM environment variable
              include FPGA init
                      EPLD init
                      External clock generator init

COPYRIGHT (C) STMicroelectronics 2002

*******************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdlib.h>
#include <stdio.h>

#include "stdevice.h"           /* Device includes */
#include "stddefs.h"

#if defined(mb295) || defined(mb290) || defined(mb376)
#include "fpga.h"              /* Local includes */
#include "ics9161.h"           /* Local includes */
#endif

/* Private types/constants ------------------------------------------------ */

/* Private variables ------------------------------------------------------ */

/* Private function prototypes -------------------------------------------- */

/* Global variables ------------------------------------------------------- */

/* Function definitions --------------------------------------------------- */


/*-------------------------------------------------------------------------
 * Function : stboot_BoardInit
 *            Initialize board specified by PLATFORM environment variable
 * Input    : None
 * Output   : None
 * Return   : None
 * ----------------------------------------------------------------------*/
BOOL stboot_BoardInit(void)
{
    BOOL RetErr;

    RetErr = FALSE;

#if defined(mb295)
    /* Initialize the FPGA only for mb295 */
    RetErr = stboot_FpgaInit();
#endif

#if defined(mb295) || defined(mb290) || defined(mb376)
    if (RetErr == FALSE)
    {
        /* Initialize ICS9161 external clock generator */
        stboot_Ics9161Init(); /* No error Code returned */

        /* Initialize EPLD */
        RetErr = stboot_EpldInit();
    }
#endif

    return RetErr;

} /* stboot_BoardInit */

/* End of board.c */
