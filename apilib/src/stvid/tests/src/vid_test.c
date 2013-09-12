/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

File name   : vid_test.c
Description : VID specific test macros

Note        : All functions return TRUE if error for CLILIB compatibility

Date          Modification                                    Initials
----          ------------                                    --------
23 May 2002   Creation                                        AN
18 Jan 2005   Os21 porting                                    MH

************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "startup.h"
#include "testtool.h"


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Private Constants --- */

/* --- Global variables --- */

/* --- Extern variables --- */

/* --- Private variables --- */

/* --- Extern functions prototypes --- */
BOOL VID_RegisterCmd2 (void);
BOOL VID_Inj_RegisterCmd (void);


/* --- Private Function prototypes --- */

/*-------------------------------------------------------------------------
 * Function : Test_CmdStart
 *            Definition of test register command
 * Input    :
 * Output   :
 * Return   : FALSE if error, TRUE if success
 * ----------------------------------------------------------------------*/
BOOL Test_CmdStart()
{
    BOOL RetOk;
    char VID_Msg[64]; /* text for trace */

    RetOk = VID_RegisterCmd2();

    if ( RetOk )
    {
       RetOk = VID_Inj_RegisterCmd();
    }

#ifdef STVID_USE_BLIT
    if ( RetOk )
    {
         RetOk = VID_BlitCmd();
    }
#endif
#ifdef STI7710_CUT2x
STTST_AssignString ("CHIP_CUT", "STI7710_CUT2x", TRUE);
#else
STTST_AssignString ("CHIP_CUT", "", TRUE);
#endif
    /* Print result */
    if ( RetOk )
    {
        sprintf( VID_Msg, "VID_TestCommand() \t: ok\n");
    }
    else
    {
        sprintf( VID_Msg, "VID_TestCommand() \t: failed !\n" );
    }
    STTST_Print(( VID_Msg ));

    return(RetOk);
}

/*#########################################################################
 *                                 MAIN
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : os20_main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void os20_main(void *ptr, const char * TTDefaultScript)
{
#ifdef STAPIGAT_USE_NEW_INIT_API
    STAPIGAT_Init(TTDefaultScript);
#else
    STAPIGAT_Init();
#endif
    STAPIGAT_Term();

} /* end os20_main */


/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
#ifdef NATIVE_CORE
void main(int argc, char * argv[])
#else
int main(int argc, char * argv[])
#endif
{

#ifdef ST_OS21
	#ifdef ST_OSWINCE
		SetFopenBasePath("/dvdgr-prj-stvid/tests/src/objs/ST40");
	#endif

    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
#endif
#if defined(ST_7200)
    if (argc == 2)
    {
        os20_main(NULL, argv[1]);
    }
    else
#endif
    {
        os20_main(NULL, NULL);
    }
    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}

/* end of vid_test.c */
