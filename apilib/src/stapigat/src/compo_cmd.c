/*****************************************************************************

File name   : compo_cmd.c

Description : COMPO macros

COPYRIGHT (C) STMicroelectronics 2004.

Date                     Modification                  Name
----                     ------------                  ----
20 februay 2005          Created                       HE
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "compo_cmd.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : COMPO_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL COMPO_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t          Err = ST_NO_ERROR;

    if (Err == ST_NO_ERROR)
    {
        STTBX_Print (("STCOMPO_Init() : OK\n"));
    }
    else
    {
        STTBX_Print (("STCOMPO_Init() : %d\n",Err));
        return (TRUE);
    }

    return Err;
}

/*-------------------------------------------------------------------------
 * Function : COMPO_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL COMPO_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t          Err = ST_NO_ERROR;

    if (Err == ST_NO_ERROR)
    {
        STTBX_Print (("STCOMPO_Term() : OK\n"));
    }
    else
    {
        STTBX_Print (("STCOMPO_Term() : %d\n",Err));
        return (TRUE);
    }

    return(Err);

}


/*-------------------------------------------------------------------------
 * Function : COMPO_GetRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL COMPO_GetRevision( parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;

    RevisionNumber = STCOMPO_GetRevision( );
    STTBX_Print(("STCOMPO_GetRevision() : revision=%s\n", RevisionNumber ));
    return ( FALSE );
}



/*-------------------------------------------------------------------------
 * Function : COMPO_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : FALSE if error, TRUE if success
 * ----------------------------------------------------------------------*/
static BOOL COMPO_InitCommand(void)
{
    BOOL RetErr = FALSE;

    RetErr  = STTST_RegisterCommand( "COMPO_Init", COMPO_Init, "COMPO Init");
    RetErr |= STTST_RegisterCommand( "COMPO_Term", COMPO_Term, "COMPO Term");
    RetErr |= STTST_RegisterCommand( "COMPO_GetRev", COMPO_GetRevision, "COMPO Get Revision");

    return (!RetErr);
}

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL COMPO_RegisterCmd(void)
{
    BOOL RetOk;

    RetOk = COMPO_InitCommand();
    if ( RetOk )
    {
        STTBX_Print(( "COMPO_RegisterCmd()    \t: ok           %s\n", STCOMPO_GetRevision()));
    }
    else
    {
        STTBX_Print(( "COMPO_RegisterCmd()     \t: failed !\n" ));
    }
    return(RetOk);
}
