/******************************************************************************
*	COPYRIGHT (C) STMicroelectronics 1998
*
*   File name   : BLT_test.c
*   Description : Macros used to test STBlit_xxx functions
*                 of the Layer Driver
*	Note        : All functions return TRUE if error for CLILIB compatibility
*
*	Date          Modification                                    Initials
*	----          ------------                                    --------
*   27 Jul 2000   Creation                                        VV
	30 May 2006   Modified for WinCE platform					  WinCE Team-ST Noida
*
******************************************************************************/

/*###########################################################################*/
/*############################## INCLUDES FILE ##############################*/
/*###########################################################################*/

#include <stdio.h>
#include <string.h>
#if defined(ST_OS21) || defined(ST_OSWINCE)
    #include "os21debug.h"
    #include <sys/stat.h>
#ifndef ST_OSWINCE
    #include <fcntl.h>
#endif /* ST_OSWINCE */
	#include "stlite.h"
#endif /*End of ST_OS21*/
#ifdef ST_OS20
	#include <debug.h>  /* used to read a file */
	#include "stlite.h"
#endif /*End of ST_OS20*/

#include "stddefs.h"
#include "stdevice.h"
#ifndef STBLIT_EMULATOR
#include "clintmr.h"
#endif
#include "stsys.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "testtool.h"
#include "api_test.h"
#include "startup.h"
#include "clevt.h"
#include "stblit.h"
#include "config.h"
#include "api_cmd.h"

/*###########################################################################*/
/*############################### DEFINITION ################################*/
/*###########################################################################*/

/* --- Constants --- */

/* --- Prototypes --- */

/* --- Global variables --- */

/* --- Externals --- */






/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL Test_CmdStart()
{
    BOOL RetErr;

#if defined(ST200_OS21)
     __asm__ volatile ("pswclr %0" : : "r" (0x4));
#endif

    return(RetErr ? FALSE : TRUE);

} /* end BLITTER_CmdStart */


/*###########################################################################*/

/*#########################################################################
 *                                 MAIN
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : os20_main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void os20_main(void *ptr)
{
    STAPIGAT_Init();
    STAPIGAT_Term();
} /* end os20_main */


/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
#ifndef PTV
int main(int argc, char *argv[])
#else /* !PTV */
int stapi_blt_main(void)
#endif /* !PTV */
{
#if (defined(ST_OS21)  || defined(ST_OSLINUX)) && !defined(ST_OSWINCE)
    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
    os20_main(NULL);
#endif
#if defined(ST_OSWINCE)
	SetFopenBasePath("/dvdgr-prj-stblit/tests/src/objs/ST40");
    printf ("\nBOOT ...\n");
    os20_main(NULL);
#endif
#ifdef ST_OS20
    os20_main(NULL);
#endif

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}

/* End of blt_test.c */

