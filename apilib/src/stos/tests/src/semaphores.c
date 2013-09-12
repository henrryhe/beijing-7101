/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : semaphores.c
Description : STOS semaphores specific tests

Note        :

Date          Modification                                    Initials
----          ------------                                    --------
May 2007      Creation                                        CD

************************************************************************/
/*#define TRACE_SEM_WAIT_SIGNAL */

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include "semaphores.h"
#include "sttbx.h"


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */
#define DEFAULT_TASK_STACK_SIZE     (16*1024)

#define SEMA_COUNT_MAX	            100000
#define SEMA_WAIT_SIGNAL_MAX        50

#ifdef TRACE_SEM_WAIT_SIGNAL
#define Semaphore_Debug_Info(x)     STTBX_Print(x)
#else
#define Semaphore_Debug_Info(x)
#endif


/* --- Private Constants --- */

/* --- Private prototypes --- */
typedef struct {
    semaphore_t	*sem1;
    semaphore_t	*sem2;
} sema_data_t;

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
    sema_data_t         Sema;
    semaphore_t	      * SemTestEnded_p;
} Task_Param_t;

/* --- Global variables --- */

/* --- Extern variables --- */

/* --- Private variables --- */

/* --- Extern functions prototypes --- */

/* --- Private Function prototypes --- */

static void wait_signal_task(Task_Param_t * Task_Param_p)
{
    int     count, rc;

    STOS_TaskEnter(NULL);

    Task_Param_p->IsRunning  = TRUE;

    for (count = 0; count < SEMA_WAIT_SIGNAL_MAX; count++)
    {
        rc = STOS_SemaphoreWait(Task_Param_p->Sema.sem2);
        Semaphore_Debug_Info(("%s received\n", Task_Param_p->Name));

        STOS_SemaphoreSignal(Task_Param_p->Sema.sem1);
        Semaphore_Debug_Info(("%s signaled\n", Task_Param_p->Name));
    }

    STOS_SemaphoreSignal(Task_Param_p->SemTestEnded_p);

    STOS_TaskExit(NULL);
}

static void performances_producer_task(Task_Param_t * Task_Param_p)
{
    int     count, rc;

    STOS_TaskEnter(NULL);

    Task_Param_p->IsRunning  = TRUE;

    for (count = 0; count < SEMA_COUNT_MAX; count++)
    {
        STOS_SemaphoreSignal(Task_Param_p->Sema.sem1);
        rc = STOS_SemaphoreWait(Task_Param_p->Sema.sem2);
    }

    STOS_SemaphoreSignal(Task_Param_p->SemTestEnded_p);

    STOS_TaskExit(NULL);
}

static int performances_consumer(sema_data_t *sema_data)
{
    clock_t	    starttime = STOS_time_now();
    clock_t     timedelta;
    int	        count, rc;

    for (count = 0; count < SEMA_COUNT_MAX; count++)
    {
        rc = STOS_SemaphoreWait(sema_data->sem1);
        STOS_SemaphoreSignal(sema_data->sem2);
    }

    timedelta = STOS_time_minus(STOS_time_now(), starttime);

#if !defined(ST_OS20)
    if ((U32)timedelta > 0)
    {
        STTBX_Print(("%s woke up %d times, %ld ticks, avg=%ld times/ticks\n",
        __FUNCTION__, count, timedelta, count/timedelta));
    }
#endif
    return(0);
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

    if (Task_p->SemTestEnded_p != NULL)
    {
        STOS_SemaphoreDelete(Task_p->CPUPartition_p, Task_p->SemTestEnded_p);
    }

    STTBX_Print(("Task: %s deleted ...\n", Task_p->Name));

    return(STOS_SUCCESS);
} /* End of delete_task() function */




/*#######################################################################
 *                             Exported functions
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : TestSemaphores_performances
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestSemaphores_performances(void)
{
    Task_Param_t    Task_Param;
    STOS_Error_t    Error = STOS_SUCCESS;
    sema_data_t     sema_data;
    int             rc;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    sema_data.sem1 = NULL;
    sema_data.sem2 = NULL;

    /* Creates first semaphore */
    sema_data.sem1 = STOS_SemaphoreCreateFifo(NULL,0);
    if (sema_data.sem1 == NULL)
    {
        STTBX_Print(("Cannot create semaphore 1\n"));
        goto error;
    }

    /* Creates second semaphore */
    sema_data.sem2 = STOS_SemaphoreCreateFifo(NULL,0);
    if (sema_data.sem2 == NULL)
    {
        STTBX_Print(("Cannot create semaphore 2\n"));
        goto error;
    }

    /* Creates task */
    Task_Param.Semaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Task_Param.ToBeDeleted = FALSE;
    Task_Param.CPUPartition_p = NULL;
    strcpy(Task_Param.Name, "STOSTEST[0]");
    Task_Param.Sema.sem1 = sema_data.sem1;
    Task_Param.Sema.sem2 = sema_data.sem2;
    Task_Param.SemTestEnded_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Error = STOS_TaskCreate((void (*) (void*)) performances_producer_task, (void *) &Task_Param,
                    Task_Param.CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param.TaskStack,
                    Task_Param.CPUPartition_p ,
                    &(Task_Param.Task_p),
                    &(Task_Param.TaskDesc),
                    12,
                    Task_Param.Name,
                    task_flags_no_min_stack_size);
    if (Error != STOS_SUCCESS)
    {
        goto error;
    }

    /* Check performances */
    rc = performances_consumer(&sema_data);

    /* Wait for producer task end */
    STOS_SemaphoreWait(Task_Param.SemTestEnded_p);

    delete_task(&Task_Param);
    rc = STOS_SemaphoreDelete(NULL, sema_data.sem1);
    rc = STOS_SemaphoreDelete(NULL, sema_data.sem2);

    return STOS_SUCCESS;


    /* Error cases */
error:
    STTBX_Print(("Error !!!\n"));

    if (sema_data.sem1 != NULL)
    {
        STOS_SemaphoreDelete(NULL, sema_data.sem1);
        sema_data.sem1 = NULL;
    }

    if (sema_data.sem2 != NULL)
    {
        STOS_SemaphoreDelete(NULL, sema_data.sem2);
        sema_data.sem2 = NULL;
    }

    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : TestSemaphores_wait_signal_priorities
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestSemaphores_wait_signal_priorities(void)
{
    Task_Param_t    Task_Param[2];
    STOS_Error_t    Error = STOS_SUCCESS;
    sema_data_t     sema_data;
    int             rc;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    sema_data.sem1 = NULL;
    sema_data.sem2 = NULL;

    /* Creates first semaphore */
    sema_data.sem1 = STOS_SemaphoreCreateFifo(NULL,0);
    if (sema_data.sem1 == NULL)
    {
        STTBX_Print(("Cannot create semaphore 1\n"));
        goto error;
    }

    /* Creates second semaphore */
    sema_data.sem2 = STOS_SemaphoreCreateFifo(NULL,0);
    if (sema_data.sem2 == NULL)
    {
        STTBX_Print(("Cannot create semaphore 2\n"));
        goto error;
    }

    /* First test: signal/wait semaphores at same priority level */
    STTBX_Print(("First test: signal/wait semaphores at same priority level\n"));
    Task_Param[0].Semaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Task_Param[0].ToBeDeleted = FALSE;
    Task_Param[0].CPUPartition_p = NULL;
    strcpy(Task_Param[0].Name, "STOSTEST[0]");
    Task_Param[0].Sema.sem1 = sema_data.sem1;
    Task_Param[0].Sema.sem2 = sema_data.sem2;
    Task_Param[0].SemTestEnded_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Error = STOS_TaskCreate((void (*) (void*)) wait_signal_task, (void *) &Task_Param[0],
                    Task_Param[0].CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param[0].TaskStack,
                    Task_Param[0].CPUPartition_p ,
                    &(Task_Param[0].Task_p),
                    &(Task_Param[0].TaskDesc),
                    12,
                    Task_Param[0].Name,
                    task_flags_no_min_stack_size);
    if (Error != STOS_SUCCESS)
    {
        goto error;
    }

    Task_Param[1].Semaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Task_Param[1].ToBeDeleted = FALSE;
    Task_Param[1].CPUPartition_p = NULL;
    strcpy(Task_Param[1].Name, "STOSTEST[1]");
    Task_Param[1].Sema.sem1 = sema_data.sem2;   /* Inverts semaphores */
    Task_Param[1].Sema.sem2 = sema_data.sem1;   /* Inverts semaphores */
    Task_Param[1].SemTestEnded_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Error = STOS_TaskCreate((void (*) (void*)) wait_signal_task, (void *) &Task_Param[1],
                    Task_Param[1].CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param[1].TaskStack,
                    Task_Param[1].CPUPartition_p ,
                    &(Task_Param[1].Task_p),
                    &(Task_Param[1].TaskDesc),
                    12,
                    Task_Param[1].Name,
                    task_flags_no_min_stack_size);
    if (Error != STOS_SUCCESS)
    {
        goto error;
    }

    /* Launch test */
    STOS_SemaphoreSignal(Task_Param[0].Sema.sem2);

    if (STOS_SemaphoreWait(Task_Param[0].SemTestEnded_p) == 0)
    {
        STTBX_Print(("Task 0 ended\n"));
    }
    if (STOS_SemaphoreWait(Task_Param[1].SemTestEnded_p) == 0)
    {
        STTBX_Print(("Task 1 ended\n"));
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    STTBX_Print(("\n"));



    /* Second test: signal/wait semaphores with task1 priority > task2 priority */
    STTBX_Print(("Second test: signal/wait semaphores with task1 priority > task2 peiority\n"));
    Task_Param[0].Semaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Task_Param[0].ToBeDeleted = FALSE;
    Task_Param[0].CPUPartition_p = NULL;
    strcpy(Task_Param[0].Name, "STOSTEST[0]");
    Task_Param[0].Sema.sem1 = sema_data.sem1;
    Task_Param[0].Sema.sem2 = sema_data.sem2;
    Task_Param[0].SemTestEnded_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Error = STOS_TaskCreate((void (*) (void*)) wait_signal_task, (void *) &Task_Param[0],
                    Task_Param[0].CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param[0].TaskStack,
                    Task_Param[0].CPUPartition_p ,
                    &(Task_Param[0].Task_p),
                    &(Task_Param[0].TaskDesc),
                    15,
                    Task_Param[0].Name,
                    task_flags_no_min_stack_size);
    if (Error != STOS_SUCCESS)
    {
        goto error;
    }

    Task_Param[1].Semaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Task_Param[1].ToBeDeleted = FALSE;
    Task_Param[1].CPUPartition_p = NULL;
    strcpy(Task_Param[1].Name, "STOSTEST[1]");
    Task_Param[1].Sema.sem1 = sema_data.sem2;   /* Inverts semaphores */
    Task_Param[1].Sema.sem2 = sema_data.sem1;   /* Inverts semaphores */
    Task_Param[1].SemTestEnded_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Error = STOS_TaskCreate((void (*) (void*)) wait_signal_task, (void *) &Task_Param[1],
                    Task_Param[1].CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param[1].TaskStack,
                    Task_Param[1].CPUPartition_p ,
                    &(Task_Param[1].Task_p),
                    &(Task_Param[1].TaskDesc),
                    12,
                    Task_Param[1].Name,
                    task_flags_no_min_stack_size);
    if (Error != STOS_SUCCESS)
    {
        goto error;
    }

    /* Launch test */
    STOS_SemaphoreSignal(Task_Param[0].Sema.sem2);

    if (STOS_SemaphoreWait(Task_Param[0].SemTestEnded_p) == 0)
    {
        STTBX_Print(("Task 0 ended\n"));
    }
    if (STOS_SemaphoreWait(Task_Param[1].SemTestEnded_p) == 0)
    {
        STTBX_Print(("Task 1 ended\n"));
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    STTBX_Print(("\n"));


    /* Third test: signal/wait semaphores with task1 priority < task2 priority */
    STTBX_Print(("Third test: signal/wait semaphores with task1 priority < task2 priority\n"));
    Task_Param[0].Semaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Task_Param[0].ToBeDeleted = FALSE;
    Task_Param[0].CPUPartition_p = NULL;
    strcpy(Task_Param[0].Name, "STOSTEST[0]");
    Task_Param[0].Sema.sem1 = sema_data.sem1;
    Task_Param[0].Sema.sem2 = sema_data.sem2;
    Task_Param[0].SemTestEnded_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Error = STOS_TaskCreate((void (*) (void*)) wait_signal_task, (void *) &Task_Param[0],
                    Task_Param[0].CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param[0].TaskStack,
                    Task_Param[0].CPUPartition_p ,
                    &(Task_Param[0].Task_p),
                    &(Task_Param[0].TaskDesc),
                    12,
                    Task_Param[0].Name,
                    task_flags_no_min_stack_size);
    if (Error != STOS_SUCCESS)
    {
        goto error;
    }

    Task_Param[1].Semaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Task_Param[1].ToBeDeleted = FALSE;
    Task_Param[1].CPUPartition_p = NULL;
    strcpy(Task_Param[1].Name, "STOSTEST[1]");
    Task_Param[1].Sema.sem1 = sema_data.sem2;   /* Inverts semaphores */
    Task_Param[1].Sema.sem2 = sema_data.sem1;   /* Inverts semaphores */
    Task_Param[1].SemTestEnded_p = STOS_SemaphoreCreateFifo(NULL, 0);
    Error = STOS_TaskCreate((void (*) (void*)) wait_signal_task, (void *) &Task_Param[1],
                    Task_Param[1].CPUPartition_p ,
                    DEFAULT_TASK_STACK_SIZE,
                    &Task_Param[1].TaskStack,
                    Task_Param[1].CPUPartition_p ,
                    &(Task_Param[1].Task_p),
                    &(Task_Param[1].TaskDesc),
                    15,
                    Task_Param[1].Name,
                    task_flags_no_min_stack_size);
    if (Error != STOS_SUCCESS)
    {
        goto error;
    }

    /* Launch test */
    STOS_SemaphoreSignal(Task_Param[0].Sema.sem2);

    if (STOS_SemaphoreWait(Task_Param[0].SemTestEnded_p) == 0)
    {
        STTBX_Print(("Task 0 ended\n"));
    }
    if (STOS_SemaphoreWait(Task_Param[1].SemTestEnded_p) == 0)
    {
        STTBX_Print(("Task 1 ended\n"));
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    STTBX_Print(("\n"));


    rc = STOS_SemaphoreDelete(NULL, sema_data.sem1);
    rc = STOS_SemaphoreDelete(NULL, sema_data.sem2);

    return STOS_SUCCESS;


    /* Error cases */
error:
    STTBX_Print(("Error !!!\n"));

    if (sema_data.sem1 != NULL)
    {
        STOS_SemaphoreDelete(NULL, sema_data.sem1);
        sema_data.sem1 = NULL;
    }

    if (sema_data.sem2 != NULL)
    {
        STOS_SemaphoreDelete(NULL, sema_data.sem2);
        sema_data.sem2 = NULL;
    }

    return STOS_FAILURE;
}

/* end of semaphores.c */
