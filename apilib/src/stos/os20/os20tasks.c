/*****************************************************************************
File name   :  os20tasks.c

Description :  Operating system independence file.

COPYRIGHT (C) STMicroelectronics 2007.

Date               Modification                                          Name
----               ------------                                          ----
08/06/2007         Created                                               WA
*****************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "stsys.h"
#include "sttbx.h"
#include "stos.h"



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
    UNUSED_PARAMETER(Flags);

    *Stack = (void *) memory_allocate(StackPartition, StackSize);
    if(*Stack == NULL)
    {
        STTBX_Print(("ERROR: Couldn't Allocate memory for task %s's stack\n", Name));
        Error = ST_ERROR_NO_MEMORY;
    }
    else
    {
        *Task = (void *) memory_allocate(TaskPartition, sizeof(task_t));
        if(*Task == NULL)
        {
            STTBX_Print(("ERROR: Couldn't Allocate memory for task %s\n", Name));

            /* deallocate memory for the stack*/
            memory_deallocate((void *) StackPartition, *Stack);
            Error = ST_ERROR_NO_MEMORY;
        }
    }

    if(Error == ST_NO_ERROR)
    {
        Error = task_init( Function, Param, *Stack, StackSize, *Task, Tdesc, Priority, Name, 0);

        if (Error != STOS_SUCCESS)
        {
            STTBX_Print(("ERROR: Couldn't create task %s\n", Name));
            Error = ST_ERROR_NO_MEMORY;
        }
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
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(task_delete(Task) != 0)
    {
        STTBX_Print(("ERROR: Couldn't delete Task - it may not have terminated...\n"));
        Error = ST_ERROR_TIMEOUT;
    }
    else
    {
        /* Deallocate the task memory */
        memory_deallocate((void *)TaskPartition, Task);

        /* Deallocate the stack memory */
        memory_deallocate((void *)StackPartition, Stack);
    }
    return Error;
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

