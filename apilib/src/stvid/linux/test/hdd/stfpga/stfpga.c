/******************************************************************************

File name   : stfpga.c

Description : Programmation of fpga on MB282 evaluation board

COPYRIGHT (C) STMicroelectronics 1999-2000.

Date               Modification                                     Name
----               ------------                                     ----
  2 Nov 2000        Created                                          FR
 23 Jun 2000        STAPI Format                                     AN
******************************************************************************/

/*
******************************************************************************
Includes
******************************************************************************
*/
#include <task.h>
#include "stddefs.h"
#include "stfpga.h"
#include "sttbx.h"

/*
******************************************************************************
Private Macros
******************************************************************************
*/

/*
******************************************************************************
Private Constants
******************************************************************************
*/
#define FPGA_OUTREGADDR      *(volatile U16*) 0x70600000
#define FPGA_DCLKADDR        *(volatile U16*) 0x70400000
#define FPGA_NAME            *(volatile U16*) 0x70A00002
#define FPGA_REVISION        *(volatile U16*) 0x70A00000

#define STFPGA_PROGRAM_ERROR  0x01
#define FPGA_NOTCONFIG        0x01
#define FPGA_CONFDONE         0x02
#define FPGA_NOTSTATUS        0x04

#define MAX_DELAY             15625

/*
******************************************************************************
Private Types
******************************************************************************
*/


/*
******************************************************************************
Private variables (static)
******************************************************************************
*/

/*
******************************************************************************
Global variables
******************************************************************************
*/

/*
******************************************************************************
Private Functions (static)
******************************************************************************
*/

/*
******************************************************************************
Public Functions (not static)
******************************************************************************
*/
/*
------------------------------------------------------------------------------
*/
/*******************************************************************************
Name        : STFPGA_Init
Description : Initialise FPGA
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              STFPGA_PROGRAM_ERROR otherwise
*******************************************************************************/
ST_ErrorCode_t  STFPGA_Init (void)
{
    U32             TimeOut, Loop, Loop1;
    static const char FpgaArray[] =
    {

#ifdef mb282b
#include "fpga1_5512.ttf"        
#else
#include "fpga1.ttf"
#endif
        ,0, 0};

    /* Reset notCONFIG to set FPGA into "wait to be programmed" mode      */
    FPGA_OUTREGADDR = FPGA_NOTCONFIG;   /* CONFIG = "1" */
    task_delay (1000);
    FPGA_OUTREGADDR = 0;                /* CONFIG = "0" */
    task_delay (1000);
    FPGA_OUTREGADDR = FPGA_NOTCONFIG;   /* CONFIG = "1" */
    task_delay (1000);

    /* Waiting for notSTATUS to go high - ready to be programmed */
    TimeOut = 0;
    while (((FPGA_OUTREGADDR & FPGA_NOTSTATUS) == 0) && (TimeOut < MAX_DELAY))
    {
        task_delay (1);
        TimeOut++;
    }

    if (TimeOut == MAX_DELAY)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Program error !"));
        return (STFPGA_PROGRAM_ERROR);
    }

    /* now program the FPGA bit per bit */
    for (Loop = 0; Loop < sizeof (FpgaArray); Loop++)
    {
        for (Loop1 = 0; Loop1 < 8; Loop1++)
        {
            FPGA_DCLKADDR = (FpgaArray[Loop] >> Loop1) & 0x01;
        }
    }

    /* Waiting for CONFDONE to go high - means the program is complete    */
    TimeOut = 0;

    while (((FPGA_OUTREGADDR & FPGA_CONFDONE) == 0) && (TimeOut < MAX_DELAY))
    {
        FPGA_DCLKADDR = 0x0;
        TimeOut++;
    }

    if (TimeOut == MAX_DELAY)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Program error !"));
        return (STFPGA_PROGRAM_ERROR);
    }

    /* Clock another 10 times - gets the device into a working state      */
    for (Loop = 0; Loop < 10; Loop++)
    {
        FPGA_DCLKADDR = 0x0;
    }

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Init FPGA successful. FPGA Name : "
            "%d FPGA Rev : %d", FPGA_NAME, FPGA_REVISION));

    return (ST_NO_ERROR);
} /* end of STFPGA_Init() */

/* ========================================================================
   Name:        STFPGA_GetRevision
   Description: Returns the name and revision Id of the FPGA

   ======================================================================== */
ST_ErrorCode_t STFPGA_GetRevision ( U16* Name, U16* VersionId )
{
    *Name      = FPGA_NAME;
    *VersionId = FPGA_REVISION;
    return ( ST_NO_ERROR );
}

/* ------------------------------- End of file ---------------------------- */

