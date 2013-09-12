
/*****************************************************************************

File name   : main.c

Description : Main entry point

COPYRIGHT (C) 1999 STMicroelectronics

*****************************************************************************/

/* Includes --------------------------------------------------------------- */


#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stsys.h"
#include "stlite.h"
#include "stddefs.h"
#include "stdevice.h"	/* T20_INTERNAL_MEMORY_SIZE... */
#include "sttbx.h"
#include "testtool.h"
#include "dhry.h"

/* Definitions ------------------------------------------------------------ */

/* Private variables (static) --------------------------------------------- */

/* Typedefs --------------------------------------------------------------- */

/* Global ----------------------------------------------------------------- */
static  task_t *Dhryid;           /* for dhrystone task */
static  BOOL   Dhrystart = FALSE; /* */
BOOL Dhrystopped = FALSE;

char    Dhry_Msg[250];            /* text for trace */

/* Functions prototypes --------------------------------------------------- */

/* Functions -------------------------------------------------------------- */


static void DhryTask( void )
{

    Dhrystart = TRUE;
    dhrystone_main( TRUE, 100000 );
}

static BOOL Dhry_Start( STTST_Parse_t *pars_p, char *result_sym_p)
{
    void *Param = NULL;
/*    int  Priority;*/
    BOOL RetErr = FALSE;

/*    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&Priority);
    if ( (RetErr == TRUE) || (Priority < 0 || Priority > 15) )
    {
        STTST_TagCurrentLine( pars_p, "Priority 0-15 !");
        return(TRUE);
    }
*/
    if ( Dhrystart == TRUE )
    {
        STTST_Print(("Task 'Dhrystone' not deleted : run Dhry_Stop before\n"));
        return(TRUE);
    }

    Dhryid = task_create( (void (*)(void *))DhryTask, Param, 4096, /*Priority*/0, "Dhrystone Task", 0 );
    if ( Dhryid == NULL )
    {
        STTST_Print(("Unable to create dhrystone task\n"));
        RetErr = TRUE;
    }
    else
    {
        Dhrystopped = FALSE;
        STTST_Print(("dhrystone task running ...\n"));
    }

    return ( FALSE );
}


static BOOL Dhry_Stop( STTST_Parse_t *pars_p, char *result_sym_p)
{
    task_t *TaskList[1];
    BOOL RetErr = FALSE;

    if ( Dhryid != NULL)
    {
        TaskList[0] = Dhryid;
        Dhrystopped = TRUE;

        if ( task_wait( TaskList, 1, TIMEOUT_INFINITY ) == -1 )
        {
            STTBX_Print(("Error: Timeout task_wait\n"));
            RetErr = TRUE;
        }
        STTBX_Print(("dhrystone Task finished\n"));
        if ( task_delete( Dhryid ) != 0 )
        {
            STTBX_Print(("Error: Task Delete \n"));
            RetErr = TRUE;
        }
        Dhrystart = FALSE;
        Dhryid = NULL;
    }

    return ( RetErr );
}


static BOOL Dhrystone( STTST_Parse_t *pars_p, char *result_sym_p)
{
    int Runcount;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, 1000000, (S32*)&Runcount);
    if ( (RetErr == TRUE) )
    {
        STTST_TagCurrentLine( pars_p, "RunCount ??");
        return(TRUE);
    }

    dhrystone_main( FALSE, Runcount );

    return ( FALSE );
}

BOOL Dhrystone_RegisterCmd()
{
    BOOL RetErr;

    RetErr  = FALSE;
    RetErr |= STTST_RegisterCommand( "DHRYSTONE",  Dhrystone,  "Start one Dhrystone <Runcount> ");
    RetErr |= STTST_RegisterCommand( "DHRY_START", Dhry_Start, "Start Dhrystone loop ");
    RetErr |= STTST_RegisterCommand( "DHRY_STOP",  Dhry_Stop,  "Stop Dhrystone loop ");

    if ( !RetErr  )
    {
        sprintf(Dhry_Msg, "Dhrystone_RegCmd() \t: ok\n");
    }
    else
    {
        sprintf(Dhry_Msg, "Dhrystone_RegCmd() \t: failed !\n" );
    }
    STTST_Print((Dhry_Msg));

    return( RetErr ? FALSE : TRUE);
}
/* end of file */




/* EOF */
