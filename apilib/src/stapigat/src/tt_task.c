/*****************************************************************************

File name   : tt_task.c

Description : Testtool TASK Commands

COPYRIGHT (C) STMicroelectronics 2004

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include <stdio.h>

#ifdef ST_OS20
#include <debug.h>
#endif /* ST_OS20 */

#ifdef ST_OS21
#include <os21debug.h>
#endif /* ST_OS21 */

#ifndef ST_OSLINUX
#include "stlite.h"
#include "testcfg.h"
#endif

#ifdef USE_PROCESS_TOOL
#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "testtool.h"

/***#include "app_data.h"***/
/***#include "tt_api.h"***/

extern BOOL  API_EnableError;
extern int   API_ErrorCount;

#define TaskTrace     STTBX_Print

#ifdef ST_OS20
/* DCU Toolset 1.7.x to 1.8.0 */
#if ( __ICC_VERSION_NUMBER__  >= 50702 ) && ( __ICC_VERSION_NUMBER__  < 60100 )
typedef enum
{
  task_status_flags_stack_used = 1
} task_status_flags_t;
#elif ( __ICC_VERSION_NUMBER__  < 50702 )   /* DCU Toolset 1.6.x  */
 #error STOP: Toolsets 1.6.x and below not supported for this functionality
#endif
#endif /* ST_OS20 */

#ifdef ST_OS21
#define task_list_head()    task_list_next(NULL)
#endif /* ST_OS21 */

#define TASKLIST_LEN 50
#define BUFFER_LEN 128
#define TASK_NAMELEN 33



/* Internal functions ----------------------------------------------------- */

static task_t *findtaskinlist(void *psudo_pid);
static task_t *findtasknumberinlist(U32 number);
static BOOL   TTTASK_ps( STTST_Parse_t *pars_p, char *Result );
static BOOL   TTTASK_nice( STTST_Parse_t *pars_p, char *Result );
static BOOL   TTTASK_niceNumber( STTST_Parse_t *pars_p, char *Result );
static BOOL   TTTASK_kill( STTST_Parse_t *pars_p, char *Result );
static BOOL   TTTASK_suspend( STTST_Parse_t *pars_p, char *Result );
static BOOL   TTTASK_resume( STTST_Parse_t *pars_p, char *Result );
static BOOL   TTTASK_list( STTST_Parse_t *pars_p, char *Result );

BOOL TTTask_RegisterCmd( void );

/* Functions -------------------------------------------------------------- */

/* ------------------------------------------------------------------------ */

static task_t  *findtaskinlist(void *psudo_pid)
{
    task_t*       task;

    for (task = task_list_head(); task != NULL; task = task_list_next(task))
    {
        if (psudo_pid == (void *)task)
        {
            return(task);
        }
    }
    return(task);
}

static task_t  *findtasknumberinlist(U32 number)
{
    task_t*       task;
    U32           index;

    for (task = task_list_head(), index = 0; task != NULL; task = task_list_next(task), index ++)
    {
        if (number == index)
        {
            /* Number found, return task structure pointer               */
            return(task);
        }
    }
    return(NULL);
}


/*-------------------------------------------------------------------------
 * Function : TT_Task_ps
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTASK_ps( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    int           pri, count;
    task_t*       task;
    char          *state_p;

    char          buffer[TASKLIST_LEN][BUFFER_LEN];
#ifdef ST_OS20
    char          *susstate_p;
    char          *state[] = {"created", "active", "terming", "term'd"};
    char          *susstate[] = {"active", "suspending", "suspended"};
    char          *unknown = "???";
#endif /* ST_OS20 */
#ifdef ST_OS21
    char          *state[] = {"active", "termated", "suspended"};

#endif /* ST_OS21 */

    BOOL           RetErr;
    task_status_t  status;
    BOOL ListOverflowed = FALSE;


    RetErr = FALSE;
    UNUSED_PARAMETER(pars_p);
#ifdef ST_OS20
    TaskTrace(("n  %-*s pri stack       size/used    state    imm# susp# susp.state\n",
                  TASK_NAMELEN-1, "task"));
    TaskTrace(("== %-*s === ======== =============== ======== ==== ===== ==========\n",
                  TASK_NAMELEN-1, "=========================" ));
#endif /* ST_OS20 */
#ifdef ST_OS21
    TaskTrace(("n  %-*s pri stack       size/used    state    \n", TASK_NAMELEN-1, "task"));
    TaskTrace(("== %-*s === ======== =============== ======== \n", TASK_NAMELEN-1, "=========================" ));
#endif /* ST_OS21 */

    for (task = task_list_head(), count = 0;
         task != NULL;
         task = task_list_next(task), count++ )
    {
        /* Warning to check stack used could take a while                  */
         task_status(task, &status, task_status_flags_stack_used);
#ifdef ST_OS20
        if (( task->task_state >= task_state_created ) &&
            ( task->task_state <= task_state_terminated ))
            state_p = state[task->task_state];
        else
            state_p = unknown;

        if (( task->task_tdesc->tdesc_susstate >= tdesc_susstate_active ) &&
            ( task->task_tdesc->tdesc_susstate <= tdesc_susstate_suspended ))
            susstate_p = susstate[task->task_tdesc->tdesc_susstate];
        else
            susstate_p = unknown;
#endif /* ST_OS20 */
#ifdef ST_OS21
        state_p = state[status.task_state];
#endif /* ST_OS21 */

        if ( count < TASKLIST_LEN )
        {
#ifdef ST_OS20
            sprintf( buffer[count],
                     "%2d %-*s %3d %8p %7d/%7d %-8s %4d %5d %-10s\th%8p\n",
                     count,
                     TASK_NAMELEN-1, task_name(task),
                     task_priority(task),
                     (void*)task->task_stack_base,
                     task->task_stack_size,
                     status.task_stack_used,
                     state_p,
                     task->task_immortal,
                     task->task_suscount,
                     susstate_p,
                     (void*)task );
#endif /* ST_OS20 */
#ifdef ST_OS21
            sprintf( buffer[count],
                     "%2d %-*s %3d %8p %7d/%7d %-8s\th%8p\n",
                     count,
                     TASK_NAMELEN-1, task_name(task),
                     task_priority(task),
                     (void*)status.task_stack_base,
                     status.task_stack_size,
                     status.task_stack_used,
                     state_p,
                     (void*)task );
#endif /* ST_OS21 */
        }
        else
        {
            ListOverflowed = TRUE;
            break;
        }
    } /* for(task) */

    for ( pri = 0; (pri < count) && (pri < TASKLIST_LEN); pri ++ )
    {
        TaskTrace((buffer[pri]));
    }
    if (ListOverflowed)
    {
        TaskTrace(("== WARNING == This task list should not be exhaustive more than %d tasks have been found !!!", TASKLIST_LEN ));
    }

    STTST_AssignInteger(Result, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* end TASK_PS */

/*-------------------------------------------------------------------------
 * Function : TTTASK_nice
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTASK_nice( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;

    S16     sel;
    S32     Var1, Var2;
    task_t *task;
    const char *usage = "USAGE: TASK_NICE hxxxxxxxx n\n where xxxxxxxx is the task psudo-pid (use TASK_PS) and n is the new priority\n";
    BOOL    RetErr;

    /* --------------------------------- */

    RetErr = FALSE;

    STTST_GetTokenCount(pars_p, &sel);

    if (sel != 3)
    {
        TaskTrace(( " 2 arguments required:\n%s", usage ));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    if ( STTST_GetInteger( pars_p, -1, &Var1 ) == TRUE )
    {
        TaskTrace((" Could not get arguments.\n%s", usage));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    if ( STTST_GetInteger( pars_p, -1, &Var2 ) == TRUE )
    {
        TaskTrace((" Could not get arguments.\n%s", usage));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    /* --------------------------------- */

    task = findtaskinlist( (void *)Var1 );

    if (task == NULL)
    {
        ErrCode = ST_ERROR_INVALID_HANDLE;
        TaskTrace((" task h%08x not found.\n", Var1));
        API_ErrorCount++;
        RetErr = TRUE;
    }

    sel = task_priority_set(task, Var2);

    /* --------------------------------- */

    STTST_AssignInteger(Result, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* TTTASK_nice */


/*-------------------------------------------------------------------------
 * Function : TTTASK_niceNumber
 *            Change Task priority with task number of the TASK_ps list instead
 *            of pseudo id.
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTASK_niceNumber( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;

    S16     sel;
    S32     Var1, Var2;
    task_t *task;
    const char *usage = "USAGE: TASK_NICENUM hxxxxxxxx n\n where xxxxxxxx is the task number (use TASK_PS) and n is the new priority\n";
    BOOL    RetErr;

    /* --------------------------------- */

    RetErr = FALSE;

    STTST_GetTokenCount(pars_p, &sel);

    if (sel != 3)
    {
        TaskTrace(( " 2 arguments required:\n%s", usage ));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    if ( STTST_GetInteger( pars_p, -1, &Var1 ) == TRUE )
    {
        TaskTrace((" Could not get arguments.\n%s", usage));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    if ( STTST_GetInteger( pars_p, -1, &Var2 ) == TRUE )
    {
        TaskTrace((" Could not get arguments.\n%s", usage));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    /* --------------------------------- */

    task = findtasknumberinlist( (U32)Var1 );

    if (task == NULL)
    {
        ErrCode = ST_ERROR_INVALID_HANDLE;
        TaskTrace((" task %d not found.\n", Var1));
        API_ErrorCount++;
        RetErr = TRUE;
    }

    sel = task_priority_set(task, Var2);

    /* --------------------------------- */

    STTST_AssignInteger(Result, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* TTTASK_niceNumber */


/*-------------------------------------------------------------------------
 * Function : TTTASK_kill
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTASK_kill( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;

    S16               sel;
    S32               Var1, Var2;
    task_t           *task;
    int               tstatus;
    task_kill_flags_t tflags;
    const char       *usage = "USAGE: TASK_KILL hxxxxxxxx n\n where xxxxxxxx is the task psudo-pid (use TASK_PS) and (optionally)\n n is 9 to bypass calling the task handler.\n";
    BOOL              RetErr;

    /* --------------------------------- */
    RetErr = FALSE;

    STTST_GetTokenCount(pars_p, &sel);

    if ((sel != 2) && (sel != 3))
    {
        TaskTrace(( " 2 arguments required:\n%s", usage ));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    if ( STTST_GetInteger( pars_p, -1, &Var1 ) == TRUE )
    {
        TaskTrace((" Could not get arguments.\n%s", usage));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    if ( STTST_GetInteger( pars_p, -1, &Var2 ) == TRUE )
    {
        TaskTrace((" Could not get arguments.\n%s", usage));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    /* --------------------------------- */

    task = findtaskinlist( (void *)Var1 );

    if (task == NULL)
    {
        ErrCode = ST_ERROR_INVALID_HANDLE;
        TaskTrace((" task h%08x not found.\n", Var1));
        API_ErrorCount++;
        RetErr = TRUE;
    }

    /* --------------------------------- */

    tstatus = 0; /* set task exit status */

    if (Var2 == 9)
    {
        tflags = task_kill_flags_no_exit_handler;
    }
    else
    {
        tflags = 0;
    }

    sel = task_kill(task, tstatus, tflags);

    if (sel == -1)
    {
        ErrCode = ST_ERROR_INVALID_HANDLE;
        TaskTrace((" task h%08x cannot be killed.\n", Var1));
        API_ErrorCount++;
        RetErr = TRUE;
    }
    else
    {
        TaskTrace((" +h%08x killed ok.\n", Var1));
    }


    /* --------------------------------- */

    STTST_AssignInteger(Result, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* TTTASK_kill */

/*-------------------------------------------------------------------------
 * Function : TTTASK_suspend
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTASK_suspend( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;

    S16     sel;
    S32     Var1;
    task_t *task;
    const char *usage = "USAGE: TASK_SUSPEND hxxxxxxxx n, where xxxxxxxx is the task psudo-pid.\n";
    BOOL    RetErr;

    /* --------------------------------- */

    RetErr = FALSE;

    STTST_GetTokenCount(pars_p, &sel);

    if (sel != 2)
    {
        TaskTrace(( " 2 arguments required:\n%s", usage ));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    if ( STTST_GetInteger( pars_p, -1, &Var1 ) == TRUE )
    {
        TaskTrace((" Could not get arguments.\n%s", usage));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    /* --------------------------------- */

    task = findtaskinlist( (void *)Var1 );

    if (task == NULL)
    {
        ErrCode = ST_ERROR_INVALID_HANDLE;
        TaskTrace((" task h%08x not found.\n", Var1));
        API_ErrorCount++;
        RetErr = TRUE;
    }

    /* --------------------------------- */

    sel = task_suspend(task);

    if (sel == -1)
    {
        ErrCode = ST_ERROR_INVALID_HANDLE;
        TaskTrace((" task h%08x could not be suspended.\n", Var1));
        API_ErrorCount++;
        RetErr = TRUE;
    }
    else
    {
        TaskTrace((" +h%08x suspended ok.\n", Var1));
    }

    /* --------------------------------- */

    STTST_AssignInteger(Result, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* TTTASK_suspend */

/*-------------------------------------------------------------------------
 * Function : TTTASK_resume
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTASK_resume( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;

    S16     sel;
    S32     Var1;
    task_t *task;
    BOOL    RetErr;

    const char *usage = "USAGE: TASK_RESUME hxxxxxxxx n, where xxxxxxxx is the task psudo-pid.\n";

    /* --------------------------------- */

    RetErr = FALSE;

    STTST_GetTokenCount(pars_p, &sel);

    if (sel != 2)
    {
        TaskTrace(( " 2 arguments required:\n%s", usage ));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    if ( STTST_GetInteger( pars_p, -1, &Var1 ) == TRUE )
    {
        TaskTrace((" Could not get arguments.\n%s", usage));
        ErrCode = ST_ERROR_INVALID_HANDLE;
        API_ErrorCount++;
        RetErr = TRUE;
    }

    /* --------------------------------- */

    task = findtaskinlist( (void *)Var1 );

    if (task == NULL)
    {
        ErrCode = ST_ERROR_INVALID_HANDLE;
        TaskTrace((" task h%08x not found.\n", Var1));
        API_ErrorCount++;
        RetErr = TRUE;
    }

    /* --------------------------------- */

    sel = task_resume(task);

    if (sel == -1)
    {
        ErrCode = ST_ERROR_INVALID_HANDLE;
        TaskTrace((" task h%08x could not be resumed.\n", Var1));
        API_ErrorCount++;
        RetErr = TRUE;
    }
    else
    {
        TaskTrace((" +h%08x resumed ok.\n", Var1));
    }

    /* --------------------------------- */

    STTST_AssignInteger(Result, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* TTTASK_resume */

/*-------------------------------------------------------------------------
 * Function : TT_Task_list
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTASK_list( STTST_Parse_t *pars_p, char *Result )
{
    int           count;
    task_t*       task;
    char          name[TASK_NAMELEN];

    count = 0;
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(Result);
    for (task = task_list_head(); task != NULL; task = task_list_next(task))
    {
        strcpy( name, task_name(task));

        TaskTrace(("%2d %-*s \n", count, TASK_NAMELEN-1, name ));
        count++;

    }   /* for(task) */

    return (API_EnableError);
}

/*-------------------------------------------------------------------------
 * Function : TTTask_RegisterCmd
 * Input    :
 * Output   :
 * Return   : TRUE if ok, FALSE if failed
 * ----------------------------------------------------------------------*/
BOOL TTTask_RegisterCmd( void )
{
    BOOL RetErr = FALSE;

    RetErr |= STTST_RegisterCommand("TASK_KILL",    TTTASK_kill,    "kill task <hxxxx.xxxx> n, if n=9 causes task exit handler not to be called");
    RetErr |= STTST_RegisterCommand("TASK_LIST",    TTTASK_list,    "list the running tasks");
    RetErr |= STTST_RegisterCommand("TASK_NICE",    TTTASK_nice,    "reset task <hxxxx.xxxx> priority to n");
    RetErr |= STTST_RegisterCommand("TASK_NICENUM", TTTASK_niceNumber, "reset task <Number in list> priority to n");
    RetErr |= STTST_RegisterCommand("TASK_PS",      TTTASK_ps,      "show the processing task states");
    RetErr |= STTST_RegisterCommand("TASK_RESUME",  TTTASK_resume,  "resume task <hxxxx.xxxx>");
    RetErr |= STTST_RegisterCommand("TASK_SUSPEND", TTTASK_suspend, "suspend task <hxxxx.xxxx>");

    if (RetErr)
    {
        STTST_Print(( "TTTask_RegisterCmd() \t: failed !\n"));
    }
    else
    {
        STTST_Print(( "TTTask_RegisterCmd() \t: ok\n" ));
    }
    return(!RetErr);
} /* end of TTTask_RegisterCmd() */

#endif /* USE_PROCESS_TOOL */

/* EOF tt_task.c */
