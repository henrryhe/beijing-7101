/*******************************************************************************

File name   :   main.c

Description :   Main entry point of the programme.

COPYRIGHT (C) STMicroelectronics 2004.

   Date                     Modification                            Name
   ----                     ------------                            ----
 Nov 2003            Created                                         MB
 Jul 2004            STi7710 support modification                    GG

*******************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

/* Private preliminary definitions (internal use only) ---------------------- */
/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include "stddefs.h"
#include "testtool.h"
#include "startup.h"

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Global variables ------------------------------------------------ */

/* --- Prototype ------------------------------------------------------- */

/* --- Externals ------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : main
 *            Retreive driver's statistics
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    printf ("\n ------ main started ------ \n");
    STAPIGAT_Init();
    STAPIGAT_Term();
    printf ("\n ------  main ended  ------ \n");
    fflush (stdout);

/*    exit (0);*/
} /* End of main() function. */
