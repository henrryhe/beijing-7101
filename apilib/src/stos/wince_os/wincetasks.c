/*****************************************************************************
File name   :  OS21tasks.c

Description :  Operating system independence file.

COPYRIGHT (C) STMicroelectronics 2007.

Date               Modification                                          Name
----               ------------                                          ----
08/06/2007         Created                                               WA
*****************************************************************************/

/* --- Includes -------------------------------------------------------- */


#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "stsys.h"
#include "sttbx.h"
#include "stos.h"


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Global variables ------------------------------------------------ */

/* --- Prototype ------------------------------------------------------- */

/* --- Externals ------------------------------------------------------- */

/* Tasks related functions */
/*******************************************************************************
Name        : STOS_TaskCreate
Description : Common call to OS20 and OS21 for task creation
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_NO_MEMORY
*******************************************************************************/
ST_ErrorCode_t  STOS_TaskCreate   (void (*Function)(void* Param),
                                        void* Param,
                                        partition_t* StackPartition,
                                        size_t StackSize,
                                        void** Stack,
                                        partition_t* TaskPartition,
                                        task_t** Task,
                                        tdesc_t* Tdesc,
                                        int Priority,
                                        const char* Name,
                                        task_flags_t Flags )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    /* To avoid warnings */
    UNUSED_PARAMETER(Stack);
    UNUSED_PARAMETER(Tdesc);

    if (TaskPartition == NULL)
    {
        *Task = task_create(Function, Param, StackSize, Priority, Name, Flags);
    }
    else
    {
        *Task = task_create_p(TaskPartition, Function, Param, StackPartition, StackSize,
                          Priority, Name, Flags | task_flags_no_min_stack_size);
    }

    if (*Task == NULL)
    {
        STTBX_Print(("ERROR: Couldn't create task %s\n", Name ));
        Error = ST_ERROR_NO_MEMORY;
    }

    return Error;
}

/*******************************************************************************
Name        : STOS_TaskDelete
Description : Common call to OS20 and OS21 for task deletion
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR or ST_ERROR_TIMEOUT
*******************************************************************************/
ST_ErrorCode_t  STOS_TaskDelete ( task_t* Task,
                                  partition_t* TaskPartition,
                                  void* Stack,
                                  partition_t* StackPartition )
{
    /* To avoid warnings */
    UNUSED_PARAMETER(TaskPartition);
    UNUSED_PARAMETER(Stack);
    UNUSED_PARAMETER(StackPartition);

    if( task_delete(Task) == STOS_SUCCESS)
    {
        STTBX_Print(("Task Deleted Successfully\n"));
        return ST_NO_ERROR;
    }
    else
    {
        STTBX_Print(("ERROR: Couldn't delete Task - it may not have terminated...\n"));
        return ST_ERROR_TIMEOUT;
    }
}

/*******************************************************************************
Name        : STOS_TaskWait
Description : Common call to OS20 and OS21 for task deletion wait
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR or ST_ERROR_TIMEOUT
*******************************************************************************/
ST_ErrorCode_t  STOS_TaskWait ( task_t** Task, const STOS_Clock_t * TimeOutValue_p )
{
    ST_ErrorCode_t   ret;
    ret = task_wait(Task, 1, (STOS_Clock_t *)TimeOutValue_p);

    return ret;
}

/*******************************************************************************
Name        : STOS_TaskEnter
Description : Common call to OS20, OS21 and LINUX for task entering
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t  STOS_TaskEnter ( void * Param )
{
    /* To avoid warnings */
    UNUSED_PARAMETER(Param);
    /* Nothing to do */
    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : STOS_TaskExit
Description : Common call to OS20, OS21 and LINUX for task exiting
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t  STOS_TaskExit ( void * Param )
{
    /* To avoid warnings */
    UNUSED_PARAMETER(Param);
    /* Nothing to do */
    return ST_NO_ERROR;
}

/* End of wincetasks.c */


