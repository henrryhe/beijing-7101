/*****************************************************************************
File name   :  OS21semaphore.c

Description :  Operating system independence file.

COPYRIGHT (C) STMicroelectronics 2007.

Date               Modification                                          Name
----               ------------                                          ----
08/06/2007         Created                                               WA
*****************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "sttbx.h"
#include "stos.h"
#include "os21utils.h"


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Global variables ------------------------------------------------ */

/* --- Prototype ------------------------------------------------------- */

/* --- Externals ------------------------------------------------------- */

/*******************************************************************************
Name        : STOS_SemaphoreCreateFifo
Description : OS-independent implementation of semaphore_create_fifo().
              Allocates memory for semaphore structure and initializes semaphore.
              The STOS_SemaphoreCreateFifo() function call is supposed to be independent of the OS used but needs
              a partition_p argument. Depending on the OS used, a different partition must be used.
Parameters  : Partition where to create semaphore, initial value of the semaphore
              If Partition_p = NULL:
                 - For OS20, the STOS function recognizes that it needs to use system_partition.
                 - For OS21, the semaphore_create() function is used instead of semaphore_create_p().
                 - For Linux, the Partition_p parameter is not used.
Assumptions :
Limitations :
Returns     : Address of initialized semaphore or NULL if not enough memory
*******************************************************************************/
semaphore_t * STOS_SemaphoreCreateFifo(partition_t * Partition_p, const int InitialValue)
{
    if(Partition_p == NULL)
    {
        return(semaphore_create_fifo(InitialValue));
    }
    else
    {
        return(semaphore_create_fifo_p(Partition_p, InitialValue));
    }
}

/*******************************************************************************
Name        : STOS_SemaphoreCreateFifoTimeOut
Description : OS-independent implementation of semaphore_create_fifo_timeout()
              Allocates memory for semaphore structure and initializes semaphore
              The STOS_SemaphoreCreateFifo() function call is supposed to be independent of the OS used but needs
              a partition_p argument. Depending on the OS used, a different partition must be used.
Parameters  : Partition where to create semaphore, initial value of the semaphore
              If Partition_p = NULL:
                 - the STOS function recognizes that it needs to use system_partition for OS20.
                 - For OS21, the semaphore_create() function is used instead of semaphore_create_p().
Assumptions :
Limitations :
Returns     : Address of initialized semaphore or NULL if not enough memory
*******************************************************************************/
semaphore_t * STOS_SemaphoreCreateFifoTimeOut(partition_t * Partition_p, const int InitialValue)
{
    /* OS21 does not have semaphore_create_fifo_timeout() function so calls to
       STOS_SemaphoreCreateFifo() and STOS_SemaphoreCreateFifoTimeOut() are equivalent */
    if(Partition_p == NULL)
    {
        return(semaphore_create_fifo(InitialValue));
    }
    else
    {
        return(semaphore_create_fifo_p(Partition_p, InitialValue));
    }
}

/*******************************************************************************
Name        : STOS_SemaphoreCreatePriority
Description : OS-independent implementation of semaphore_create_priority().
              Allocates memory for semaphore structure and initializes semaphore.
              The STOS_SemaphoreCreatePriority() function call is supposed to be independent of the OS used but needs
              a partition_p argument. Depending on the OS used, a different partition must be used.
Parameters  : Partition where to create semaphore, initial value of the semaphore
              If Partition_p = NULL:
                 - the STOS function recognizes that it needs to use system_partition for OS20.
                 - For OS21, the semaphore_create() function is used instead of semaphore_create_p().
Assumptions :
Limitations :
Returns     : Address of initialized semaphore or NULL if not enough memory
*******************************************************************************/
semaphore_t * STOS_SemaphoreCreatePriority(partition_t * Partition_p, const int InitialValue)
{

    if(Partition_p == NULL)
    {
        return(semaphore_create_priority(InitialValue));
    }
    else
    {
        return(semaphore_create_priority_p(Partition_p, InitialValue));
    }
}

/*******************************************************************************
Name        : STOS_SemaphoreCreatePriorityTimeOut
Description : OS-independent implementation of semaphore_create_priority_timeout().
              Allocates memory for semaphore structure and initializes semaphore.
              The STOS_SemaphoreCreatePriority() function call is supposed to be independent of the OS used but needs
              a partition_p argument. Depending on the OS used, a different partition must be used.
Parameters  : Partition where to create semaphore, initial value of the semaphore
              If Partition_p = NULL:
                 - the STOS function recognizes that it needs to use system_partition for OS20.
                 - For OS21, the semaphore_create() function is used instead of semaphore_create_p().
Assumptions :
Limitations :
Returns     : Address of initialized semaphore or NULL if not enough memory
*******************************************************************************/
semaphore_t * STOS_SemaphoreCreatePriorityTimeOut(partition_t * Partition_p, const int InitialValue)
{
    /* OS21 does not have semaphore_create_priority_timeout() function so calls to
       STOS_SemaphoreCreatePriority() and STOS_SemaphoreCreatePriorityTimeOut() are equivalent */
    if(Partition_p == NULL)
    {
        return(semaphore_create_priority(InitialValue));
    }
    else
    {
        return(semaphore_create_priority_p(Partition_p, InitialValue));
    }
}

/*******************************************************************************
Name        : STOS_SemaphoreDelete
Description : OS-independent implementation of semaphore_delete
Parameters  : Partition of the semaphore to delete, pointer on semaphore
              Partition is the same as used for STOS_SemaphoreCreate.
              If Partition_p = NULL, then we use the predefined system_partition for OS20.
              For OS21, this parameter is unused.
Assumptions : A call to STOS_SemaphoreCreate was made to create the semaphore to be deleted
Limitations : For OS20, don't use after call to semaphore_create because semaphore_delete()
              already deallocates memory in that case. Only use after STOS_SemaphoreCreate.
Returns     : STOS_SUCCESS or STOS_FAILURE
*******************************************************************************/
int STOS_SemaphoreDelete(partition_t * Partition_p, semaphore_t * Semaphore_p)
{
    assert(Semaphore_p != NULL);
    /* To avoid warnings */
    UNUSED_PARAMETER(Partition_p);

    return(semaphore_delete(Semaphore_p));
}

/*******************************************************************************
Name        : STOS_SemaphoreSignal
Description : OS-independent implementation of semaphore_signal
Parameters  : semaphore to signal to
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void STOS_SemaphoreSignal(semaphore_t * Semaphore_p)
{
    assert(Semaphore_p != NULL);

    semaphore_signal(Semaphore_p);
}

/*******************************************************************************
Name        : STOS_SemaphoreWait
Description : OS-independent implementation of semaphore_wait
Parameters  : semaphore to wait for
Assumptions :
Limitations :
Returns     : Always returns STOS_SUCCESS
*******************************************************************************/
int STOS_SemaphoreWait(semaphore_t * Semaphore_p)
{
    assert(Semaphore_p != NULL);

    semaphore_wait(Semaphore_p);

    return(STOS_SUCCESS);
}

/*******************************************************************************
Name        : STOS_SemaphoreWaitTimeOut
Description : OS-independent implementation of semaphore_wait_timeout
Parameters  : semaphore to wait for, time out value
Assumptions :
Limitations :
Returns     : STOS_SUCCESS if success, STOS_FAILURE if timeout occurred
*******************************************************************************/
int STOS_SemaphoreWaitTimeOut(semaphore_t * Semaphore_p, const STOS_Clock_t * TimeOutValue_p)
{
    osclock_t * TimeOut_p;

    assert(Semaphore_p != NULL);

    TimeOut_p = CAST_CONST_PARAM_TO_DATA_TYPE(osclock_t *, TimeOutValue_p);

    return(semaphore_wait_timeout(Semaphore_p, TimeOut_p));
}

/* End of os21semaphore.c */

