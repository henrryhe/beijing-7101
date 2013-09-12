/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : mutex.c
Description : STOS Mutex specific tests
Note        :
Date          Modification                                    Initials
----          ------------                                    --------
May 2007      Creation                                        WA
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include "stcommon.h"
#include "stos_mutex.h"
#include "tasks.h"
#include "sttbx.h"



/* --- Constants (default values) -------------------------------------- */

/* --- Private Constants --- */
#define MUTEX_MEMORY_TEST_LOOP          10000000
#define DEFAULT_TASK_STACK_SIZE         (16*1024)

#define LONG_TERM_TESTS_NBSECONDS       60 /* Waits 60s */

/* --- Extern structure prototypes --- */

/* --- Private prototypes --- */

typedef struct {
    mutex_t *mutex1;
    mutex_t *mutex2;
} mutex_data_t;

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
    mutex_t           * mutex_p;
    mutex_data_t        mutex;
} Task_Param_t;

/* --- Global variables --- */
static Task_Param_t    Task_Param[3];
int             count;
int             lock_test_running  = 0;

int             fifo_default_error = 0;
int             stress_default_error = 0;
int             lock_default_error = 0;
int             delete_default_error =0;

int             count_fifo_error = 0;
int             count_stress_error = 0;
int             count_lock_error = 0;
int             count_delete_error = 0;

int             count_task1 = 0, count_task2 = 0, count_task3 = 0;
semaphore_t   * lock_sem1_p = NULL;
semaphore_t   * fifo_sem_p = NULL;

/* --- Extern variables --- */

/* --- Private variables --- */

/* --- Extern functions prototypes --- */

/* --- Private function prototypes --- */

static void mutex_fifo_task(Task_Param_t * Task_Param_p)
{
    STOS_TaskEnter(NULL);
    Task_Param_p->IsRunning = TRUE;

    STOS_MutexLock(Task_Param_p->mutex_p);
    switch(Task_Param_p->Name[14])
    {
            case '0' :
                if (count == 0)
                {
                    count = 1;
                }
                else
                {
                    STTBX_Print(("Mutex FIFO Test : Task 1 Error  !!!\n"));
                    count_fifo_error ++;
                }
            break;
            case '1' :
                if (count == 1)
                {
                    count = 2;
                }
                else
                {
                    STTBX_Print(("Mutex FIFO Test : Task 2 Error  !!!\n"));
                    count_fifo_error ++;
                }
            break;
            case '2' :
                if (count == 2)
                {
                    STOS_SemaphoreSignal(fifo_sem_p);
                }
                else
                {
                    STTBX_Print(("Mutex FIFO Test : Task 3 Error  !!!\n"));
                    count_fifo_error ++;
                }
            break;
            default:
                STTBX_Print(("Mutex FIFO TEST Error !!!\n"));
                fifo_default_error ++;
            break;
    }
    STOS_MutexRelease(Task_Param_p->mutex_p);
    STTBX_Print(("Exiting: %s ...\n", Task_Param_p->Name));
    STOS_TaskExit(NULL);
}

static void mutex_stress_task(Task_Param_t * Task_Param_p)
{
    int count_stress = 0;

    STOS_TaskEnter(NULL);
    Task_Param_p->IsRunning  = TRUE;
    do
    { /* While not ToBeDeleted */
            STOS_MutexLock(Task_Param_p->mutex_p);
            if(count_stress == 1)
            {
                STTBX_Print(("STRESS TEST : Critical Section reached  !!!\n"));
                count_stress_error ++;
            }
            count_stress = 1;
            STOS_TaskDelayUs(100000);  /*waits 100 ms*/
            switch(Task_Param_p->Name[14])
            {
                    case '0' :
                        count_task1 ++;
                    break;
                    case '1' :
                        count_task2 ++;
                    break;
                    case '2' :
                        count_task3 ++;
                    break;
                    default:
                        STTBX_Print(("Mutex STRESS TEST Error !!!\n"));
                        stress_default_error ++;
                    break;
            }
            count_stress = 0;
            STOS_MutexRelease(Task_Param_p->mutex_p);

            STOS_TaskDelayUs(200000); /* give the hand to the other tasks to be executed */
    }while (!(Task_Param_p->ToBeDeleted));

    STTBX_Print(("Exiting: %s ...\n", Task_Param_p->Name));
    STOS_TaskExit(NULL);
}

static void mutex_lock_task(Task_Param_t * Task_Param_p)
{
    STOS_TaskEnter(NULL);
    Task_Param_p->IsRunning  = TRUE;

    switch(Task_Param_p->Name[14])
    {
            case '0' :
#if !defined DO_NOT_SUPPORT_RECURSIVE_MUTEX
                /* Linux kernel space does not support recursive mutexes */
                STOS_MutexLock(Task_Param_p->mutex_p);
                STOS_MutexLock(Task_Param_p->mutex_p);
                STOS_MutexLock(Task_Param_p->mutex_p);
                STOS_MutexLock(Task_Param_p->mutex_p);
#endif
                STOS_MutexLock(Task_Param_p->mutex_p);
                if(lock_test_running != 1)
                {
#if !defined DO_NOT_SUPPORT_RECURSIVE_MUTEX
                    /* Linux kernel space does not support recursive mutexes */
                    STOS_MutexRelease(Task_Param_p->mutex_p);
                    STOS_MutexRelease(Task_Param_p->mutex_p);
                    STOS_MutexRelease(Task_Param_p->mutex_p);
                    STOS_MutexRelease(Task_Param_p->mutex_p);
#endif
                    STOS_MutexRelease(Task_Param_p->mutex_p);
                }
                else
                {
                    STTBX_Print(("Lock Test : Critical Section reached after consecutive lock of same mutex !!!\n"));
                    count_lock_error ++;
                }
            break;
            case '1' :
                STOS_MutexLock(Task_Param_p->mutex_p);
                if(lock_test_running != 1)
                {
                    STOS_MutexRelease(Task_Param_p->mutex_p);
                    STOS_SemaphoreSignal(lock_sem1_p);
                }
                else
                {
                    STTBX_Print(("Lock Test : Critical Section reached, high priority task lock the mutex  !!!\n"));
                    count_lock_error ++;
                }
            break;
            default:
                STTBX_Print(("Mutex LOCK Task Error !!!\n"));
                lock_default_error ++;
            break;
    }
    STTBX_Print(("Exiting: %s ...\n", Task_Param_p->Name));
    STOS_TaskExit(NULL);
}

static void mutex_delete_task(Task_Param_t * Task_Param_p)
{
    int count_delete = 0;

    STOS_TaskEnter(NULL);
    Task_Param_p->IsRunning  = TRUE;

    switch(Task_Param_p->Name[14])
    {
            case '0' :
            case '1' :
                do
                { /* While not ToBeDeleted */
                        STOS_MutexLock(Task_Param_p->mutex_p);
                        if(count_delete == 1)
                        {
                            STTBX_Print(("STRESS TEST : Critical Section reached  !!!\n"));
                            count_delete_error ++;
                        }
                        count_delete = 1;
                        STOS_TaskDelayUs(100000);   /* waits 100 ms */
                        count_delete = 0;
                        STOS_MutexRelease(Task_Param_p->mutex_p);
                        STOS_TaskDelayUs(200000);   /* waits 200 ms to give the hand to the other tasks to be executed */

                }while (!(Task_Param_p->ToBeDeleted));
            break;
            case '2' :
                do
                {
                    Task_Param_p->mutex_p = STOS_MutexCreateFifo();
                    STOS_TaskDelayUs(1000000);  /* waits 1s */
                    STOS_MutexDelete(Task_Param_p->mutex_p);
                }while (!(Task_Param_p->ToBeDeleted));
            break;
            default:
                STTBX_Print(("Mutex LOCK Task Error !!!\n"));
                delete_default_error ++;
            break;
    }
    STTBX_Print(("Exiting: %s ...\n", Task_Param_p->Name));
    STOS_TaskExit(NULL);
}

static STOS_Error_t create_mutex_fifo_task(Task_Param_t * Task_Param_p, U32 priority, char * Name)
{
    STOS_Error_t    Error;
    Task_Param_p->ToBeDeleted = FALSE;
    Task_Param_p->CPUPartition_p = NULL;
    strcpy(Task_Param_p->Name, Name);
    Task_Param_p->Semaphore_p = NULL;
    Task_Param_p->TaskStack   = NULL;
    Error = STOS_TaskCreate((void (*) (void*)) mutex_fifo_task, (void *) Task_Param_p,
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

static STOS_Error_t create_mutex_stress_task(Task_Param_t * Task_Param_p, U32 priority, char * Name)
{
    STOS_Error_t    Error;
    Task_Param_p->ToBeDeleted = FALSE;
    Task_Param_p->CPUPartition_p = NULL;
    strcpy(Task_Param_p->Name, Name);
    Task_Param_p->Semaphore_p = NULL;
    Error = STOS_TaskCreate((void (*) (void*)) mutex_stress_task, (void *) Task_Param_p,
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

static STOS_Error_t create_mutex_lock_task(Task_Param_t * Task_Param_p, U32 priority, char * Name)
{
    STOS_Error_t    Error;
    Task_Param_p->ToBeDeleted = FALSE;
    Task_Param_p->CPUPartition_p = NULL;
    strcpy(Task_Param_p->Name, Name);
    Task_Param_p->Semaphore_p = NULL;
    Error = STOS_TaskCreate((void (*) (void*)) mutex_lock_task, (void *) Task_Param_p,
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

static STOS_Error_t create_mutex_delete_task(Task_Param_t * Task_Param_p, U32 priority, char * Name)
{
    STOS_Error_t    Error;
    Task_Param_p->ToBeDeleted = FALSE;
    Task_Param_p->CPUPartition_p = NULL;
    strcpy(Task_Param_p->Name, Name);
    Task_Param_p->Semaphore_p = NULL;
    Error = STOS_TaskCreate((void (*) (void*)) mutex_delete_task, (void *) Task_Param_p,
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
    STOS_TaskWait(&(Task_p->Task_p), TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                      Task_p->CPUPartition_p,
                      Task_p->TaskStack,
                      Task_p->CPUPartition_p );
    Task_p->IsRunning  = FALSE;
    STTBX_Print(("Task: %s deleted ...\n", Task_p->Name));
    return(STOS_SUCCESS);
} /* End of delete_task() function */

/*  funtion which return the max between 3 integers*/
int MaxValue (int A, int B, int C)
{
    int MaxAB = A > B ? A : B;
    return (MaxAB > C ? MaxAB : C);
}

STOS_Error_t check_count_rates(int c1, int c2, int c3)
{
    STOS_Error_t    Error = STOS_SUCCESS;
    int             Rate1 = 0, Rate2 = 0, Rate3 = 0;

    STTBX_Print(("count_task1 value is : %d!!!\n", c1));
    STTBX_Print(("count_task2 value is : %d!!!\n", c2));
    STTBX_Print(("count_task3 value is : %d!!!\n", c3));

    if((c1 != 0) && (c2 != 0) && (c3 != 0))
    {
        if (c1 == MaxValue(c1, c2, c3))
        {
            Rate1 = 100;
            Rate2 = (c2*100)/c1;
            Rate3 = (c3*100)/c1;
        }
        else if (c2 == MaxValue(c1, c2, c3))
        {
            Rate1 = (c1*100)/c2;
            Rate2 = 100;
            Rate3 = (c3*100)/c2;
        }
        else /* count3 is the max of count1, count2, count3 */
        {
            Rate1 = (c1*100)/c3;
            Rate2 = (c2*100)/c3;
            Rate3 = 100;
        }

        STTBX_Print(("The rate of taking Mutex by Task 1 is : %d \n", Rate1 ));
        STTBX_Print(("The rate of taking Mutex by Task 2 is : %d \n", Rate2 ));
        STTBX_Print(("The rate of taking Mutex by Task 3 is : %d \n", Rate3 ));
        if((Rate1 < 80) || (Rate2 < 80) || (Rate3 < 80))
        {
            STTBX_Print(("Mutex distribution between tasks is not equitable \n"));
            Error =  STOS_FAILURE;
        }
    }
    else
    {
        STTBX_Print(("c is NULL !!! \n"));
        Error = STOS_FAILURE;
    }

    return (Error);
}



/*#######################################################################
 *                             Exported functions
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : TestMutex_mutex_fifo
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestMutex_mutex_fifo(void)
{
    mutex_t       * int_mutex_p = NULL;
    int             ret_sem  = 0;
    clock_t         Max_waiting_time;


#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* pass test with the same priority */
    STTBX_Print(("TEST 1 : Tasks with the same priorities... \n"));

    /* mutex count held initialization */
    count = 0;
    count_fifo_error = 0;

    /* Semaphore Creation */
    fifo_sem_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);
    if(fifo_sem_p == NULL)
    {
        STTBX_Print(("Semaphore creation failed !!! \n"));
        goto error;
    }

    /* Mutex creation */
    int_mutex_p = STOS_MutexCreateFifo();

    Task_Param[0].mutex_p = int_mutex_p;
    Task_Param[1].mutex_p = int_mutex_p;
    Task_Param[2].mutex_p = int_mutex_p;

    STOS_MutexLock(int_mutex_p);
        /* creation of 3 tasks each one request the mutex*/
    STTBX_Print(("Tasks Creation ... \n"));

    create_mutex_fifo_task(&Task_Param[0], 10, (char *)"STOSMUTEXTEST[0]");
    create_mutex_fifo_task(&Task_Param[1], 10, (char *)"STOSMUTEXTEST[1]");
    create_mutex_fifo_task(&Task_Param[2], 10, (char *)"STOSMUTEXTEST[2]");

    if(fifo_default_error != 0)
    {
        STTBX_Print(("Error : task not created  !!! \n"));
        goto error;
    }

    /* delay to make sure of the exection of these tasks in the case of the high priority of the main task */
    STOS_TaskDelayUs(100000);    /* waits 100ms */

    STOS_MutexRelease(int_mutex_p);

    /* to let tasks releases be executed */
    STOS_TaskDelayUs(10000);   /* waits 10 ms to give the hand to the other tasks to be executed */

    /*semaphore should wait 2s to make sure of the safety of the process */
    Max_waiting_time  = time_plus(time_now() , 3*ST_GetClocksPerSecond());
    ret_sem = STOS_SemaphoreWaitTimeOut(fifo_sem_p, &Max_waiting_time);
    if(ret_sem != 0)
    {
        STTBX_Print(("Task 3 not finished !!! \n"));
        goto error;
    }

    if (count_fifo_error)
    {
        STTBX_Print(("Count error !!! \n"));
        goto error;
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    STOS_MutexDelete(int_mutex_p);
    STTBX_Print(("\n"));

    /* Repeat the same execution with different priorities of tasks : increasing order */
    STTBX_Print(("TEST 2 : Tasks have different priorities : increasing order ... \n"));

    /* mutex count held initialization */
    count = 0;
    count_fifo_error = 0;

    /* Mutex creation */
    int_mutex_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex_p;
    Task_Param[1].mutex_p = int_mutex_p;
    Task_Param[2].mutex_p = int_mutex_p;

    STOS_MutexLock(int_mutex_p);
        /* creation of 3 tasks each one request the mutex*/
    STTBX_Print(("Tasks Creation ... \n"));

    create_mutex_fifo_task(&Task_Param[0], 10, (char *)"STOSMUTEXTEST[0]");
        /*To make sure that the first lock is executed*/
    STOS_TaskDelayUs(10000);    /* waits 10ms */
    create_mutex_fifo_task(&Task_Param[1], 20, (char *)"STOSMUTEXTEST[1]");
        /*To make sure that the first lock is executed*/
    STOS_TaskDelayUs(10000);    /* waits 10ms */
    create_mutex_fifo_task(&Task_Param[2], 30, (char *)"STOSMUTEXTEST[2]");

    if(fifo_default_error != 0)
    {
        STTBX_Print(("Error : task not created  !!! \n"));
        goto error;
    }

    /* delay to make sure of the exection of these tasks in the case of the high priority of the main task */
    STOS_TaskDelayUs(100000);    /* waits 100ms */

    STOS_MutexRelease(int_mutex_p);

    /* to let tasks releases be executed */
    STOS_TaskDelayUs(10000);   /* waits 10 ms to give the hand to the other tasks to be executed*/

    /*semaphore should wait 2s to make sure of the safety of the process */
    Max_waiting_time  = time_plus(time_now() , 3*ST_GetClocksPerSecond());
    ret_sem = STOS_SemaphoreWaitTimeOut(fifo_sem_p, &Max_waiting_time);
    if(ret_sem != 0)
    {
        STTBX_Print(("Task 3 not finished !!! \n"));
        goto error;
    }

    if (count_fifo_error)
    {
        STTBX_Print(("Count error !!! \n"));
        goto error;
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    STOS_MutexDelete(int_mutex_p);
    STTBX_Print(("\n"));

    /* Repeat the same execution with different priorities of tasks : decreasing order */
    STTBX_Print(("TEST 3 : Tasks have different priorities : decreasing order ... \n"));

    /* mutex count held initialization */
    count = 0;
    count_fifo_error = 0;

    /* Mutex creation */
    int_mutex_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex_p;
    Task_Param[1].mutex_p = int_mutex_p;
    Task_Param[2].mutex_p = int_mutex_p;

    STOS_MutexLock(int_mutex_p);
        /* creation of 3 tasks each one request the mutex*/
    STTBX_Print(("Tasks Creation ... \n"));

    create_mutex_fifo_task(&Task_Param[0], 30, (char *)"STOSMUTEXTEST[0]");
    create_mutex_fifo_task(&Task_Param[1], 20, (char *)"STOSMUTEXTEST[1]");
    create_mutex_fifo_task(&Task_Param[2], 10, (char *)"STOSMUTEXTEST[2]");

    if(fifo_default_error != 0)
    {
        STTBX_Print(("Error : task not created  !!! \n"));
        goto error;
    }

    /* delay to make sure of the exection of these tasks in the case of the high priority of the main task */
    STOS_TaskDelayUs(100000);    /* waits 100ms */

    STOS_MutexRelease(int_mutex_p);

    /* to let tasks releases be executed */

    STOS_TaskDelayUs(10000);   /* waits 10 ms to give the hand to the other tasks to be executed */

    /*semaphore should wait 2s to make sure of the safety of the process */
    Max_waiting_time  = time_plus(time_now() , 3*ST_GetClocksPerSecond());
    ret_sem = STOS_SemaphoreWaitTimeOut(fifo_sem_p, &Max_waiting_time);
    if(ret_sem != 0)
    {
        STTBX_Print(("Task 3 not finished !!! \n"));
        goto error;
    }

    if (count_fifo_error)
    {
        STTBX_Print(("Count error !!! \n"));
        goto error;
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    if(fifo_sem_p != NULL)
    {
        STOS_SemaphoreDelete(NULL, fifo_sem_p);
    }
    STOS_MutexDelete(int_mutex_p);
    STTBX_Print(("\n"));



    return STOS_SUCCESS;

    /* Error cases */
error:

    STTBX_Print(("Error !!!\n"));
    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    if(fifo_sem_p != NULL)
    {
        STOS_SemaphoreDelete(NULL, fifo_sem_p);
    }
    STOS_MutexDelete(int_mutex_p);
    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : TestMutex_mutex_stress
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestMutex_mutex_stress(void)
{
    mutex_t       * int_mutex_p = NULL;
    U16             i;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* pass test with the same priority */
    STTBX_Print(("TEST 1 : Tasks with the same priorities... \n"));

    /* mutex count held initialization */
    count_task1 = 0;
    count_task2 = 0;
    count_task3 = 0;

    /* Mutex creation */
    int_mutex_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex_p;
    Task_Param[1].mutex_p = int_mutex_p;
    Task_Param[2].mutex_p = int_mutex_p;

    /* creation of 3 tasks each one request the mutex*/
    STTBX_Print(("Tasks Creation ... \n"));

    create_mutex_stress_task(&Task_Param[0], 10, (char *)"STOSMUTEXTEST[0]");
    create_mutex_stress_task(&Task_Param[1], 10, (char *)"STOSMUTEXTEST[1]");
    create_mutex_stress_task(&Task_Param[2], 10, (char *)"STOSMUTEXTEST[2]");

   if((stress_default_error != 0)||(count_stress_error != 0))
    {
        STTBX_Print(("No Task to be executed !!! \n"));
        goto error;
    }

    for(i=0; i<LONG_TERM_TESTS_NBSECONDS; i++)
    {
        STOS_TaskDelayUs(1000000);         /* waits 1s  */
    }

    if(check_count_rates(count_task1, count_task2, count_task3) != STOS_SUCCESS)
    {
        goto error;
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    if(int_mutex_p != NULL)
    {
        STOS_MutexDelete(int_mutex_p);
    }
    STTBX_Print(("\n"));

    /* Repeat the same execution with different priorities of tasks : increasing order */
    STTBX_Print(("TEST 2 : Tasks have different priorities : increasing order ... \n"));

    /* mutex count held initialization */
    count_task1 = 0;
    count_task2 = 0;
    count_task3 = 0;

    /* Mutex creation */
    int_mutex_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex_p;
    Task_Param[1].mutex_p = int_mutex_p;
    Task_Param[2].mutex_p = int_mutex_p;

    /* creation of 3 tasks each one request the mutex*/
    STTBX_Print(("Tasks Creation ... \n"));

    create_mutex_stress_task(&Task_Param[0], 10, (char *)"STOSMUTEXTEST[0]");
    create_mutex_stress_task(&Task_Param[1], 20, (char *)"STOSMUTEXTEST[1]");
    create_mutex_stress_task(&Task_Param[2], 30, (char *)"STOSMUTEXTEST[2]");

   if((stress_default_error != 0)||(count_stress_error != 0))
    {
        STTBX_Print(("No Task to be executed !!! \n"));
        goto error;
    }

    for(i=0; i<LONG_TERM_TESTS_NBSECONDS; i++)
    {
        STOS_TaskDelayUs(1000000);         /* waits 1s  */
    }

    if(check_count_rates(count_task1, count_task2, count_task3) != STOS_SUCCESS)
    {
        goto error;
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);

    if(int_mutex_p != NULL)
    {
        STOS_MutexDelete(int_mutex_p);
    }
    STTBX_Print(("\n"));


    /* Repeat the same execution with different priorities of tasks : decreasing order */
    STTBX_Print(("TEST 3 : Tasks have different priorities : decreasing order ... \n"));

    /* mutex count held initialization */
    count_task1 = 0;
    count_task2 = 0;
    count_task3 = 0;

    /* Mutex creation */
    int_mutex_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex_p;
    Task_Param[1].mutex_p = int_mutex_p;
    Task_Param[2].mutex_p = int_mutex_p;

    /* creation of 3 tasks each one request the mutex*/
    STTBX_Print(("Tasks Creation ... \n"));

    create_mutex_stress_task(&Task_Param[0], 30, (char *)"STOSMUTEXTEST[0]");
    create_mutex_stress_task(&Task_Param[1], 20, (char *)"STOSMUTEXTEST[1]");
    create_mutex_stress_task(&Task_Param[2], 10, (char *)"STOSMUTEXTEST[2]");

   if((stress_default_error != 0)||(count_stress_error != 0))
    {
        STTBX_Print(("No Task to be executed !!! \n"));
        goto error;
    }

    /* task should take at least more than 1h */
    for(i=0; i<LONG_TERM_TESTS_NBSECONDS; i++)
    {
        STOS_TaskDelayUs(1000000);         /* waits 1s  */
    }

    if(check_count_rates(count_task1, count_task2, count_task3) != STOS_SUCCESS)
    {
        goto error;
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    if(int_mutex_p != NULL)
    {
        STOS_MutexDelete(int_mutex_p);
    }
    STTBX_Print(("\n"));


    return STOS_SUCCESS;

    /* Error cases */
error:
    STTBX_Print(("Error !!!\n"));
    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    if(int_mutex_p != NULL)
    {
        STOS_MutexDelete(int_mutex_p);
    }
    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : TestMutex_mutex_delete
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestMutex_mutex_delete(void)
{
    mutex_t       * int_mutex1_p = NULL;
    U16             i;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* pass test with the same priority */
    STTBX_Print(("TEST 1 : Tasks with the same priorities... \n"));

    /* Mutexes Creations */
    int_mutex1_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex1_p;
    Task_Param[1].mutex_p = int_mutex1_p;

    count_delete_error = 0;

    /* Create task   */
    STTBX_Print(("Task creation ... \n"));

    create_mutex_delete_task(&Task_Param[0], 10, (char *)"STOSMUTEXTEST[0]");
    create_mutex_delete_task(&Task_Param[1], 10, (char *)"STOSMUTEXTEST[1]");
    create_mutex_delete_task(&Task_Param[2], 10, (char *)"STOSMUTEXTEST[2]");

    if(delete_default_error != 0)
    {
        STTBX_Print(("No Task to be executed !!! \n"));
        goto error;
    }

    /* task should take the necessary time verify the test */
    for(i=0; i<LONG_TERM_TESTS_NBSECONDS; i++)
    {
        STOS_TaskDelayUs(1000000);         /* waits 1s  */
    }

    if(count_delete_error != 0)
    {
        goto error;
    }

    /* End the function of the task here... */
    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    if(int_mutex1_p != NULL)
    {
        STOS_MutexDelete(int_mutex1_p);
    }
    STTBX_Print(("\n"));



    /* Repeat the same execution with different priorities of tasks : increasing order */
    STTBX_Print(("TEST 2 : Tasks have different priorities : increasing order ... \n"));

    /* Mutexes Creations */
    int_mutex1_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex1_p;
    Task_Param[1].mutex_p = int_mutex1_p;

    count_delete_error = 0;

    /* Create task   */
    STTBX_Print(("Task creation ... \n"));

    create_mutex_delete_task(&Task_Param[0], 10, (char *)"STOSMUTEXTEST[0]");
    create_mutex_delete_task(&Task_Param[1], 20, (char *)"STOSMUTEXTEST[1]");
    create_mutex_delete_task(&Task_Param[2], 30, (char *)"STOSMUTEXTEST[2]");

    if(delete_default_error != 0)
    {
        STTBX_Print(("No Task to be executed !!! \n"));
        goto error;
    }

    /* task should take the necessary time verify the test */
    for(i=0; i<LONG_TERM_TESTS_NBSECONDS; i++)
    {
        STOS_TaskDelayUs(1000000);         /* waits 1s  */
    }

    if(count_delete_error != 0)
    {
        goto error;
    }

    /* End the function of the task here... */
    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    if(int_mutex1_p != NULL)
    {
        STOS_MutexDelete(int_mutex1_p);
    }
    STTBX_Print(("\n"));




    /* Repeat the same execution with different priorities of tasks : decreasing order */
    STTBX_Print(("TEST 3 : Tasks have different priorities : decreasing order ... \n"));

    /* Mutexes Creations */
    int_mutex1_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex1_p;
    Task_Param[1].mutex_p = int_mutex1_p;

    count_delete_error = 0;

    /* Create task   */
    STTBX_Print(("Task creation ... \n"));

    create_mutex_delete_task(&Task_Param[0], 30, (char *)"STOSMUTEXTEST[0]");
    create_mutex_delete_task(&Task_Param[1], 20, (char *)"STOSMUTEXTEST[1]");
    create_mutex_delete_task(&Task_Param[2], 10, (char *)"STOSMUTEXTEST[2]");

    if(delete_default_error != 0)
    {
        STTBX_Print(("No Task to be executed !!! \n"));
        goto error;
    }

    /* task should take the necessary time verify the test */
    for(i=0; i<LONG_TERM_TESTS_NBSECONDS; i++)
    {
        STOS_TaskDelayUs(1000000);         /* waits 1s  */
    }

    if(count_delete_error != 0)
    {
        goto error;
    }

    /* End the function of the task here... */
    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    if(int_mutex1_p != NULL)
    {
        STOS_MutexDelete(int_mutex1_p);
    }
    STTBX_Print(("\n"));


    return STOS_SUCCESS;

    /* Error cases */
error:

    STTBX_Print(("Error !!!\n"));
    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    delete_task(&Task_Param[2]);
    if(int_mutex1_p != NULL)
    {
        STOS_MutexDelete(int_mutex1_p);
    }
    if(Task_Param[2].mutex_p != NULL)
    {
        STOS_MutexDelete(Task_Param[2].mutex_p);
    }
    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : TestMutex_mutex_lock
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestMutex_mutex_lock(void)
{
    mutex_t       * int_mutex_p = NULL;
    int             ret_sem1 = 0;
    clock_t         Max_waiting_time1;

    /* pass test with the same priority */
    STTBX_Print(("TEST 1 : Tasks with the same priorities... \n"));

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    lock_test_running = 1;
    count_lock_error = 0;

    /* Semaphores Creation */
    lock_sem1_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);
    if(lock_sem1_p == NULL)
    {
        STTBX_Print(("Sempahore creation failed !!! \n"));
        goto error;
    }
    /* Mutex creation */
    int_mutex_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex_p;
    Task_Param[1].mutex_p = int_mutex_p;

    /* creation of 2 tasks with different priorities, each one request the mutex*/
    STOS_MutexLock(int_mutex_p);

    create_mutex_lock_task(&Task_Param[0], 10, (char *)"STOSMUTEXTEST[0]");
    create_mutex_lock_task(&Task_Param[1], 10, (char *)"STOSMUTEXTEST[1]");


    STOS_TaskDelayUs(10000);    /* waits 10ms */
    lock_test_running = 0;

    /* to finish the task 1 and task2 */
    STOS_MutexRelease(int_mutex_p);

    STOS_TaskSchedule(); 

    /*semaphore should wait 2s to make sure of the safety of the process */
    Max_waiting_time1 = time_plus(time_now() , 2*ST_GetClocksPerSecond());
    ret_sem1 = STOS_SemaphoreWaitTimeOut(lock_sem1_p, &Max_waiting_time1);
    if(ret_sem1 != 0)
    {
        STTBX_Print(("Task 2 not finished !!! \n"));
        goto error;
    }

    if((lock_default_error != 0)||(lock_test_running == 1))
    {
        STTBX_Print(("No Task to be executed !!! \n"));
        goto error;
    }

    if(count_lock_error != 0)
    {
        STTBX_Print(("Errors during execution !!! \n"));
        goto error;
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    STOS_MutexDelete(int_mutex_p);
    STTBX_Print(("\n"));

    /* Repeat the same execution with different priorities of tasks : increasing order */
    STTBX_Print(("TEST 2 : Tasks have different priorities : increasing order ... \n"));

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    lock_test_running = 1;
    count_lock_error = 0;

    /* Mutex creation */
    int_mutex_p = STOS_MutexCreateFifo();
    Task_Param[0].mutex_p = int_mutex_p;
    Task_Param[1].mutex_p = int_mutex_p;

    /* creation of 2 tasks with different priorities, each one request the mutex*/
    STOS_MutexLock(int_mutex_p);

    create_mutex_lock_task(&Task_Param[0], 10, (char *)"STOSMUTEXTEST[0]");
    create_mutex_lock_task(&Task_Param[1], 30, (char *)"STOSMUTEXTEST[1]");


    STOS_TaskDelayUs(10000);    /* waits 10ms */
    lock_test_running = 0;

    /* to finish the task 1 and task2 */
    STOS_MutexRelease(int_mutex_p);

    STOS_TaskSchedule(); 


    /*semaphore should wait 2s to make sure of the safety of the process */
    Max_waiting_time1 = time_plus(time_now() , 2*ST_GetClocksPerSecond());
    ret_sem1 = STOS_SemaphoreWaitTimeOut(lock_sem1_p, &Max_waiting_time1);

    if(ret_sem1 != 0)
    {
        STTBX_Print(("Task 2 not finished !!! \n"));
        goto error;
    }

    if((lock_default_error != 0)||(lock_test_running == 1))
    {
        STTBX_Print(("No Task to be executed !!! \n"));
        goto error;
    }

    if(count_lock_error != 0)
    {
        STTBX_Print(("Errors during execution !!! \n"));
        goto error;
    }

    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    if(lock_sem1_p != NULL)
    {
        STOS_SemaphoreDelete(NULL, lock_sem1_p);
    }
    STOS_MutexDelete(int_mutex_p);
    STTBX_Print(("\n"));
    return STOS_SUCCESS;

    /* Error cases */
error:
    STTBX_Print(("Error !!!\n"));
    delete_task(&Task_Param[0]);
    delete_task(&Task_Param[1]);
    if(lock_sem1_p != NULL)
    {
        STOS_SemaphoreDelete(NULL, lock_sem1_p);
    }
    STOS_MutexDelete(int_mutex_p);
    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : TestMutex_mutex_memory
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestMutex_mutex_memory(void)
{
    mutex_t *       mutex_p;
    U32             i;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* Mutexes Create/delete in order to verify the memory leak which can be caused by these several calls */
    for (i=0; i<MUTEX_MEMORY_TEST_LOOP; i++)
    {
        mutex_p = STOS_MutexCreateFifo();
        STOS_MutexLock(mutex_p) ;
        STOS_MutexRelease(mutex_p) ;
        STOS_MutexDelete(mutex_p) ;
    }
    STTBX_Print(("\n"));
    return STOS_SUCCESS;
}

/* end of mutex.c */




