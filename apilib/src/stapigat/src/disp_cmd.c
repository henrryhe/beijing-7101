/*****************************************************************************

File name   : disp_cmd.c

Description : DISP macros

COPYRIGHT (C) STMicroelectronics 2004.

Date                     Modification                  Name
----                     ------------                  ----
20 februay 2005          Created                       HE
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "disp_cmd.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : DISP_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DISP_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t          Err = ST_NO_ERROR;

    if (Err == ST_NO_ERROR)
    {
        STTBX_Print (("STDISP_Init() : OK\n"));
    }
    else
    {
        STTBX_Print (("STDISP_Init() : %d\n",Err));
        return (TRUE);
    }

    return Err;
}

/*-------------------------------------------------------------------------
 * Function : DISP_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DISP_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t          Err = ST_NO_ERROR;

    if (Err == ST_NO_ERROR)
    {
        STTBX_Print (("STDISP_Term() : OK\n"));
    }
    else
    {
        STTBX_Print (("STDISP_Term() : %d\n",Err));
        return (TRUE);
    }

    return(Err);

}

/*-------------------------------------------------------------------------
 * Function : DISP_GetRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DISP_GetRevision( parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;

    RevisionNumber = STDISP_GetRevision( );
    STTBX_Print(("STDISP_GetRevision() : revision=%s\n", RevisionNumber ));
    return ( FALSE );
}


/*-------------------------------------------------------------------------
 * Function : DISP_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : FALSE if error, TRUE if success
 * ----------------------------------------------------------------------*/
static BOOL DISP_InitCommand(void)
{
    BOOL RetErr = FALSE;

    RetErr  = STTST_RegisterCommand( "DISP_Init", DISP_Init, "DISP Init");
    RetErr |= STTST_RegisterCommand( "DISP_Term", DISP_Term, "DISP Term");
    RetErr |= STTST_RegisterCommand( "DISP_GetRev", DISP_GetRevision, "DISP Get Revision");

    return (!RetErr);
}

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL DISP_RegisterCmd(void)
{
    BOOL RetOk;

    RetOk = DISP_InitCommand();
    if ( RetOk )
    {
        STTBX_Print(( "DISP_RegisterCmd()     \t: ok           %s\n", STDISP_GetRevision()));
    }
    else
    {
        STTBX_Print(( "DISP_RegisterCmd()     \t: failed !\n" ));
    }
    return(RetOk);
}
