/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : tasks.c
Description : STOS tasks/threads specific tests

Note        :

Date          Modification                                    Initials
----          ------------                                    --------
Mar 2007      Creation                                        CD

************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include "stcommon.h"
#include "tasks.h"
#include "sttbx.h"


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Private Constants --- */
#define DEFAULT_TASK_STACK_SIZE     (16*1024)

/* --- Extern structure prototypes --- */

typedef struct
{
    semaphore_t       * Semaphore_p;
    ST_Partition_t    * CPUPartition_p;
    task_t            * Task_p;
    tdesc_t             TaskDesc;
    void              * TaskStack;
    BOOL                IsRunning;          /* TRUE if task is running, FALSE otherwise */
    BOOL                ToBeDeleted;        /* Set TRUE to ask task to end in order to delete it */
    char                Name[20];


    /* Utilities variables */
    U32                 Counter;
    U32                 OtherTaskPriority;
    semaphore_t       * SynchSem_p;
} Task_Param_t;

/* --- Private structure prototypes --- */

/* --- Global variables --- */

static Task_Param_t     Task_Param[2];
static semaphore_t    * SynchSem_p = NULL;

/* --- Extern variables --- */

/* --- Private variables --- */

/* --- Extern functions prototypes --- */

/* --- Private function prototypes --- */

static void simple_task(Task_Param_t * Task_Param_p)
{
    STOS_TaskEnter(NULL);

    Task_Param_p->IsRunning  = TRUE;

    STTBX_Print(("Task created: %s\n", Task_Param_p->Name));

    do
    { /* While not ToBeDeleted */
        STOS_SemaphoreWait(Task_Param_p->Semaphore_p);

        Task_Param_p->Counter++;
        if ((Task_Param_p->Counter == 2) && (Task_Param_p->SynchSem_p != NULL))
        {
            STOS_SemaphoreSignal(Task_Param_p->SynchSem_p);
        }

        STTBX_Print(("%s awaken (%d)\n", Task_Param_p->Name, Task_Param_p->Counter));

    } while (!(Task_Param_p->ToBeDeleted));

    STTBX_Print(("Exiting: %s ...\n", Task_Param_p->Name));
    STOS_TaskExit(NULL);
}

static STOS_Error_t create_simple_task(Task_Param_t * Task_Param_p, U32 priority, char * Name)
{
    STOS_Error_t    Error;

    Task_Param_p->ToBeDeleted = FALSE;
    Task_Param_p->CPUPartition_p = NULL;
    Task_Param_p->Counter = 0;
    Task_Param_p->OtherTaskPriority = 0;    /* Unused here */
    strcpy(Task_Param_p->Name, Name);
    Task_Param_p->Semaphore_p = STOS_SemaphoreCreateFifo(Task_Param_p->CPUPartition_p, 0);
    Error = STOS_TaskCreate((void (*) (void*)) simple_task, (void *) Task_Param_p,
                    Task_Param_p->CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param_p->TaskStack,
                    Task_Param_p->CPUPartition_p ,
                    &(Task_Param_p->Task_p),
                    &(Task_Param_p->TaskDesc),
                    priority,
                    Task_Param_p->Name,
                    task_flags_no_min_stack_size);

    return Error;
}

static STOS_Error_t delete_task(Task_Param_t * Task_p)
{
    if (! Task_p->IsRunning)
    {
        return(STOS_SUCCESS);
    }

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    /* And awake the task.                  */
    STOS_SemaphoreSignal(Task_p->Semaphore_p);

    STOS_TaskWait(&(Task_p->Task_p), TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                      Task_p->CPUPartition_p,
                      Task_p->TaskStack,
                      Task_p->CPUPartition_p );

    Task_p->IsRunning  = FALSE;
    STOS_SemaphoreDelete(Task_p->CPUPartition_p, Task_p->Semaphore_p);

    Task_p->SynchSem_p = NULL;

    STTBX_Print(("Task: %s deleted ...\n", Task_p->Name));

    return(STOS_SUCCESS);
} /* End of StopErrorRecoveryTask() function */


static void task_creating_deleting_other_task(Task_Param_t * Task_Param_p)
{
    STOS_TaskEnter(NULL);

    Task_Param_p->IsRunning  = TRUE;

    STTBX_Print(("Task created: %s\n", Task_Param_p->Name));

    do
    { /* While not ToBeDeleted */

        STOS_SemaphoreWait(Task_Param_p->Semaphore_p);
        Task_Param_p->Counter++;
        STTBX_Print(("%s awaken (%d)\n", Task_Param_p->Name, Task_Param_p->Counter));

        if (! Task_Param_p->ToBeDeleted)
        {
            create_simple_task(&Task_Param[1], Task_Param_p->OtherTaskPriority, (char *)"STOSTEST[1]");
            STOS_SemaphoreSignal(Task_Param[1].Semaphore_p);
            STOS_SemaphoreSignal(Task_Param[1].Semaphore_p);
            delete_task(&Task_Param[1]);
        }

    } while (!(Task_Param_p->ToBeDeleted));

    STTBX_Print(("Exiting: %s ...\n", Task_Param_p->Name));
    STOS_TaskExit(NULL);
}



/*#######################################################################
 *                             Exported functions
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : TestTasks_tasks_end
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestTasks_tasks_end(void)
{
    Task_Param_t  * Task_Param_p;
    STOS_Error_t    Error;
    clock_t         Max_waiting_time;
    int             ret_sem  = 0;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* Creates synchronisation semaphore */
    SynchSem_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);
    if(SynchSem_p == NULL)
    {
        STTBX_Print(("Semaphore creation failed !!! \n"));
        goto error;
    }

    Task_Param[0].SynchSem_p = SynchSem_p;

    /* 1st test: create and delete 2 tasks */
    STTBX_Print(("First test\n"));
    create_simple_task(&Task_Param[0], 10, (char *)"STOSTEST[0]");

    /* Check tasks are running */
    STOS_SemaphoreSignal(Task_Param[0].Semaphore_p);
    STOS_TaskSchedule();
    STOS_SemaphoreSignal(Task_Param[0].Semaphore_p);
    STOS_TaskSchedule();

    /*semaphore should wait 3s to make sure of the safety of the process */
    Max_waiting_time  = STOS_time_plus(STOS_time_now() , 3*ST_GetClocksPerSecond());
    ret_sem = STOS_SemaphoreWaitTimeOut(Task_Param[0].SynchSem_p, &Max_waiting_time);
    if(ret_sem != 0)
    {
        STTBX_Print(("Checking task awakening: failed !!!\n  Counter[0]=%d\n", Task_Param[0].Counter));
        goto error;
    }
    else
    {
        STTBX_Print(("Checking task awakening: OK\n"));
    }

    delete_task(&Task_Param[0]);
    STTBX_Print(("\n"));



    /* 2nd test: create 1 task creating another task with higher priority then deleting it */
    STTBX_Print(("Second test\n"));
    Task_Param_p = &Task_Param[0];
    Task_Param[0].Semaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Task_Param[0].ToBeDeleted = FALSE;
    Task_Param[0].CPUPartition_p = NULL;
    Task_Param[0].Counter = 0;
    Task_Param[0].SynchSem_p = NULL;
    Task_Param[0].OtherTaskPriority = 15;
    strcpy(Task_Param[0].Name, "STOSTEST[0]");
    Error = STOS_TaskCreate((void (*) (void*)) task_creating_deleting_other_task, (void *) &Task_Param[0],
                    Task_Param[0].CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param[0].TaskStack,
                    Task_Param[0].CPUPartition_p ,
                    &(Task_Param[0].Task_p),
                    &(Task_Param[0].TaskDesc),
                    12,
                    Task_Param[0].Name,
                    task_flags_no_min_stack_size);

    STOS_SemaphoreSignal(Task_Param[0].Semaphore_p);
    STOS_TaskDelayUs(10000);    /* waits 10ms */

    delete_task(&Task_Param[0]);
    STTBX_Print(("\n"));



    /* 3rd test: create 1 task creating another task with lower priority then deleting it */
    STTBX_Print(("Third test\n"));
    Task_Param_p = &Task_Param[0];
    Task_Param[0].Semaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Task_Param[0].ToBeDeleted = FALSE;
    Task_Param[0].CPUPartition_p = NULL;
    Task_Param[0].Counter = 0;
    Task_Param[0].SynchSem_p = NULL;
    Task_Param[0].OtherTaskPriority = 10;
    strcpy(Task_Param[0].Name, "STOSTEST[0]");
    Error = STOS_TaskCreate((void (*) (void*)) task_creating_deleting_other_task, (void *) &Task_Param[0],
                    Task_Param[0].CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param[0].TaskStack,
                    Task_Param[0].CPUPartition_p ,
                    &(Task_Param[0].Task_p),
                    &(Task_Param[0].TaskDesc),
                    12,
                    Task_Param[0].Name,
                    task_flags_no_min_stack_size);

    STOS_SemaphoreSignal(Task_Param[0].Semaphore_p);
    STOS_TaskDelayUs(10000);    /* waits 10ms */

    delete_task(&Task_Param[0]);
    if(SynchSem_p != NULL)
    {
        STOS_SemaphoreDelete(NULL, SynchSem_p);
    }
    STTBX_Print(("\n"));

    return STOS_SUCCESS;


    /* Error cases */
error:
    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    if(SynchSem_p != NULL)
    {
        STOS_SemaphoreDelete(NULL, SynchSem_p);
    }

    return STOS_FAILURE;
}



/* end of tasks.c */
