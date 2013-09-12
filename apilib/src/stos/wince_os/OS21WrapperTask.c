/*! Time-stamp: <@(#)OS21WrapperTask.c   06/04/2005 - 14:32:18   William hennebois>
 *********************************************************************
 *  @file   : OS21WrapperTask.c
 *
 *  Project : STAPI Windows CE
 *
 *  Package : 
 *
 *  Company :  TeamLog for ST
 *
 *  Author  : William hennebois            Date: 06/04/2005
 *
 *  Purpose : This module implements all Wrapper OS21 to Windows CE
 *			  Thread
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  05/04/2005 WHE : First Revision
 * V 0.11  06/01/2005 JLX : Somme bugs correction 
 * V 0.12  06/03/2005 WHE : Somme bugs correction
 *
 *********************************************************************
 */

#include <OS21WrapperTask.h>
#include <kfuncs.h>

#ifdef TRACE_THREAD_NAME
  #include "SetThreadTag.h"
#endif

//! Global handler for Task_Lock
static CRITICAL_SECTION gCriticalHandler;
static task_t *gListThread=0;
static task_t dummyMainTask;
static int iCeHIGHestTaskPriority=1;
static int iCeLOWestTaskPriority =255;




__inline int  MAPPRIORITYFROMCETOOS21(int cepriority)			
{
	int dce =   iCeHIGHestTaskPriority -iCeLOWestTaskPriority;
	int d21 =   MIN_USER_PRIORITY -MAX_USER_PRIORITY;
	return MAX_USER_PRIORITY - (((cepriority-iCeHIGHestTaskPriority )*d21)/dce);
}

__inline int  MAPPRIORITYFROMOS21TOCE(int os21priority)		
{
	int dce =   iCeHIGHestTaskPriority -iCeLOWestTaskPriority;
	int d21 =   MIN_USER_PRIORITY -MAX_USER_PRIORITY;
	return iCeHIGHestTaskPriority  + ((MAX_USER_PRIORITY -os21priority)*dce)/d21;
}


/*!-------------------------------------------------------------------------------------
 * Nexted list Add
 *
 * @param *pThread : 
 *
 * @return static  : 
 */
static void _AddThreadList(task_t *pThread)
{
    pThread->pThreadNext = gListThread;
    gListThread = pThread;
}


/*!-------------------------------------------------------------------------------------
 * Nexted list Remove ( see task_list_next ) 
 *
 * @param *pThread : 
 *
 * @return static  : 
 */
static void _RemoveThreadList(task_t *pThread)
{
    task_t* parent = NULL;

    if (gListThread == pThread) // remove head
        gListThread = pThread->pThreadNext;
    else // find parent and remove it
    {
        for (parent = gListThread; parent != NULL; parent = parent->pThreadNext)
            if (parent->pThreadNext == pThread)
            {
                parent->pThreadNext = pThread->pThreadNext;
                break;
            }
        WCE_ASSERT(parent != NULL); // should have found it
    }
}


/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param none
 *
 * @return static task_t  : 
 */
static task_t * _GetCurrentThread()
{
    task_t* parent = NULL;
	unsigned long   id = GetCurrentThreadId();
    for (parent = gListThread; parent != NULL; parent = parent->pThreadNext)
	{
		if ((unsigned long)parent->dwThreadId_opaque == id)
		{
			break;
		}
	}
	return parent;
}



/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param cePriority : 
 *
 * @return static task_t  : 
 */
static task_t * FindTaskPriority(unsigned int cePriority)
{
    task_t* parent = NULL;
    for (parent = gListThread; parent != NULL; parent = parent->pThreadNext)
	{
		if (parent->dwCeTaskPriority == cePriority) 
		{
			return parent;
		}
	}
	return NULL;
}

/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param index : 
 *
 * @return static task_t  : 
 */
static task_t * FindTaskByIndex( int index)
{
    task_t* parent = gListThread;
    for (; index &&  parent != NULL; parent = parent->pThreadNext) index--;
	return parent;
}


static task_t * FindTaskByName( char *pName)
{
    task_t* parent = gListThread;
    for (; parent != NULL; parent = parent->pThreadNext) 
	{
		if(strcmp(pName,parent->tName)==0) break;
	}
	return parent;
}



// -------------------------------------------------------------------------------------
//
//
//				Thread functions  WinCE Implementation
//
//
//
//
//
/****************************************************/
/*                	TASKS                          */
/****************************************************/

/*!-------------------------------------------------------------------------------------
 * Convert the lvl of priority from OS21 to WinCE
 * On OS21 the priority goes from MAX_USER_PRIORITY == 0 to MIN_USER_PRIORITY 255
 * On Windows The Time Critical is 0 and the lowest is 255
 * to convert the prority level we assume that the priority is 255 - pos21
 * @param os21prioritylevel : 0  to 255
 *
 * @return int  : MIN_USER_PRIORITY to MAX_USER_PRIORITY WinCE
 */
int _WCE_ConvertLevelPriorityFrom(task_t *ptask,int os21prioritylevel)
{
	int cePriority ;

#ifdef WINCE_INTERRUPT_MAP
//  print("OS21 Task Priority: %d", os21prioritylevel);  
	cePriority = OS21toWinCEPriorityMap(task_name(ptask));
//  print("WinCE Task Priority: %d", cePriority);
	if (cePriority == -1)	//Task not found
		cePriority = MAPPRIORITYFROMOS21TOCE(os21prioritylevel);
#else 
	cePriority = MAPPRIORITYFROMOS21TOCE(os21prioritylevel);
#endif

//  printf("WinCE Task priority : %s[%x] - OS21=%d  WINCE=%d\n", task_name(ptask), ptask->hHandle_opaque, os21prioritylevel, cePriority);
  
	// Assume that the priority level are same as CE 
	WCE_MSG(MDL_MSG,"_WCE_ConvertLevelPriorityFrom : Change Priority to (%d) ",cePriority);
	if(cePriority < iCeHIGHestTaskPriority || cePriority > iCeLOWestTaskPriority)
	{
		WCE_MSG(MDL_WARNING,"_WCE_ConvertLevelPriorityFrom : priority out of scope  (%d), should be between min=%d and  max=%d",cePriority,iCeHIGHestTaskPriority,iCeLOWestTaskPriority);
	}
	return cePriority;
}



char * _WCE_EnumStapiTaskName(int Index)
{
	task_t *pTask=0;

	pTask =  FindTaskByIndex(Index);
	if(pTask == 0) return NULL;
	return pTask->tName;;
}

int _WCE_SetCeTaskPriority(int index, int WincePriority)
{
	task_t *pTask=0;
	pTask =  FindTaskByIndex(index);
	if(pTask == 0) return FALSE;
	pTask->dwCeTaskPriority = WincePriority;
	pTask->dwOs21TaskPriority = MAPPRIORITYFROMCETOOS21(WincePriority);
	WCE_VERIFY(CeSetThreadPriority(pTask->hHandle_opaque,pTask->dwCeTaskPriority) != 0);
	return TRUE;
}



int _WCE_GetCeTaskPriority(int index,HANDLE **pHandle)
{

	task_t *pTask=0;
	pTask =  FindTaskByIndex(index);
	if(pTask == 0) return -1;
	if(pHandle)
	{
		*pHandle = pTask->hHandle_opaque;
	}
	return (int)pTask->dwCeTaskPriority;
}





/*!-------------------------------------------------------------------------------------
 * Because the prototype of Thread function of WinCE are diffrents, we need to use  
 * a function callback,  
 *
 * it is juste a debug implmentation, we need to manage the re-entrance
 * @param Function :  Callback fn
 * @param Stack :	  not used the system allocate the stack in the stack pool
 * @param StackSize : 
 * @param Task :	  task handle	
 * @param Tdesc : 
 * @param Priority :  priority of the thread (should be a OS21 priority)
 * @param name :	  name 
 * @param flags :	  ????
 *
 * @return int  : 
 */
static int task_init(void (*Function)(void* Param), void* Param,void* Stack, size_t StackSize, task_t* Task, void * Tdesc,int Priority, const char* name,task_flags_t flags)
{
	task_t tmp;
	tmp = *Task;

	_AddThreadList(Task);

	// reset handle
	memset(Task,0,sizeof(Task));
	// Save real parameters
	Task->pfnThread_opaque = Function;
	Task->pThreadParameter = Param;
	if(name)	strncpy(Task->tName,name,sizeof(Task->tName));
	else		strncpy(Task->tName,"noname",sizeof(Task->tName));

	
	
	// create the thread
	Task->hHandle_opaque = (void *)CreateThread(NULL,StackSize,(LPTHREAD_START_ROUTINE)Function,(LPVOID)Param,STACK_SIZE_PARAM_IS_A_RESERVATION,(LPDWORD)&Task->dwThreadId_opaque);
	if(Task->hHandle_opaque == NULL) 
	{
		*Task = tmp;
		return 0;
	}
	
	#ifdef TRACE_THREAD_NAME
	  SetThreadTag((HANDLE)Task->hHandle_opaque, (DWORD)&Task->tName);
	#endif
	
	P_ADDTHREAD(Task->dwThreadId_opaque, GetCurrentThreadId(), name);	
	Task->dwOs21TaskPriority = Priority;
	Task->dwCeTaskPriority   = _WCE_ConvertLevelPriorityFrom(Task,Priority);
	WCE_VERIFY(CeSetThreadPriority(Task->hHandle_opaque,Task->dwCeTaskPriority) != 0);

#ifdef WINCE_PERF_OPTIMIZE
	WCE_VERIFY(CeSetThreadQuantum( Task->hHandle_opaque,  10)!= 0);
#endif
	return 1;
}
 


/*!-------------------------------------------------------------------------------------
 *  See task_init
 *
 * @param Param) : 
 * @param Param : 
 * @param StackSize : 
 * @param Priority : 
 * @param name : 
 * @param flags : 
 *
 * @return task_t*  : 
 */
task_t* task_create(void (*pFunction)(void* Param), void* Param,size_t StackSize,int Priority, const char* name,task_flags_t flags)
{
	task_t* pTask = 0;
	WCE_MSG(MDL_OSCALL,"-- task_create %s",name?name:"noname");


	pTask =  memory_allocate(NULL,sizeof(task_t));
	WCE_ASSERT(pTask);
	// init to NULL
	memset(pTask,0,sizeof(task_t));
	if(!task_init(pFunction,Param,NULL,StackSize,pTask,NULL,Priority,name,flags)) return 0;


	return pTask;

}
/*
//! With partition memory
task_t *task_create_p( partition_t* taskPartition ,void (*pFunction)(void* Param), void* Param,partition_t* StackPartition, size_t StackSize,int Priority, const char* name,task_flags_t flags)
{
	return task_create(pFunction,Param,StackSize,Priority,name,flags);
}
*/


/*!-------------------------------------------------------------------------------------
 * This function allows a task to be deleted. The task must have terminated (by
 * returning from its entry point function) before this can be called. Attempting to
 * delete a task which has not yet terminated fails.
 *
 * @param *task : the handle
 *
 * @return int  : true or false if fail
 */
int task_delete(task_t *task)
{
	WCE_ASSERT(task);
	WCE_MSG(MDL_OSCALL,"-- task_delete(%x)",task);
	
	// Wait a maximum of 1 sec for task ending
	if(WaitForSingleObject(task->hHandle_opaque,1000) != WAIT_OBJECT_0)
	{
		WCE_ERROR("task_delete without ending the task")
	}

    /* No equivalent (needed) for pthreads. */
	if(!CloseHandle(task->hHandle_opaque)) return 0;
	_RemoveThreadList(task);

	memory_deallocate(NULL,task);

    return OS21_SUCCESS;
}


/*!-------------------------------------------------------------------------------------
 * Waiting for the end of the task 
 *
 * @param **tasks : list of tasks
 * @param taskcount : number of tasks
 * @param *timeout_p : in OS21 ticks, or TIMEOUT_IMMEDIATE/TIMEOUT_INFINITE
 *
 * @return int  : 
 */
int task_wait(task_t **tasks, int taskcount, osclock_t *timeout_p)
{
    task_t *task = NULL;
    DWORD  iRet;
    DWORD  dwMilliseconds;
    int    taskindex;
    HANDLE *listOfTasks = NULL;

 	WCE_MSG(MDL_OSCALL,"-- task_wait(...)");

    WCE_VERIFY(taskcount >= 1);
    WCE_VERIFY(tasks != NULL);

    listOfTasks = (HANDLE*) malloc(taskcount * sizeof(HANDLE));
    WCE_VERIFY(listOfTasks != NULL);

    if (timeout_p == TIMEOUT_IMMEDIATE)
        dwMilliseconds = 0;
    else if (timeout_p == TIMEOUT_INFINITY)
        dwMilliseconds = INFINITE;
    else
    {
        WCE_VERIFY(timeout_p != NULL);
        dwMilliseconds = (DWORD)_WCE_Tick2Millisecond(*timeout_p);
    }

    for (taskindex = 0; taskindex < taskcount; taskindex++)
    {
        WCE_VERIFY(tasks[taskindex] != NULL);
        WCE_VERIFY(tasks[taskindex]->hHandle_opaque != NULL);
        listOfTasks[taskindex] = tasks[taskindex]->hHandle_opaque;
    }
    
    P_ADDSYSTEMCALL(P_TASK_WAIT,0);

	iRet = WaitForMultipleObjects(taskcount, listOfTasks, FALSE, dwMilliseconds);
    WCE_VERIFY(iRet != WAIT_FAILED);
    free(listOfTasks);

    if (iRet != WAIT_FAILED && iRet != WAIT_TIMEOUT)
    {
        WCE_VERIFY(iRet >= WAIT_OBJECT_0 && iRet < WAIT_OBJECT_0 + taskcount);
        return iRet - WAIT_OBJECT_0; // return terminated task index 
    }
    
    return OS21_FAILURE; // time-out or actual failure
}


/*!-------------------------------------------------------------------------------------
 * Delay the calling task until the specified time. If this_time is before the current
   time, then this function returns immediately. this_time is specified in ticks, which
   is an implementation dependent quantity, see Chapter 10: Real-time clocks on
   page 231.
 *
 * @param ExitTime : 
 *
 * @return void task_delay_until  : 
 */

void task_delay_until (osclock_t ExitTime) 
{
	osclock_t timenow = time_now();
  	osclock_t timetowaitticks;
   DWORD     timetowaitmilli;
 	WCE_MSG(MDL_OSCALL,"-- task_delay_until(%I64d)",ExitTime);

	/* Apply delay only if current time has not run over ExitTime */
	if(time_after( ExitTime, timenow))
   {
	   /*  Compute the number of ticks to wait */
		timetowaitticks = time_minus(ExitTime, timenow);

      /* Convert ticks to milliseconds */
		timetowaitmilli = (DWORD)_WCE_Tick2Millisecond(timetowaitticks);
		
      /* Sleep only if non zero */
		if(timetowaitmilli)
      {
         P_ADDSYSTEMCALL(P_TASK_DELAY,0);
			Sleep(timetowaitmilli);
      }
   }
}


/*!-------------------------------------------------------------------------------------
 * Sleep a Task for a delay of n ticks :  64 ticks == 1 Micro seconds
 *
 * @param ticks : Number of ticks 1 microsec = 64 ticks
 *
 * @return int  : 
 */

void task_delay(osclock_t ticks)
{
 	WCE_MSG(MDL_OSCALL,"-- task_delay(%d)",ticks);


	// The Sleep function suspends the execution of the current thread for a specified interval.
	// sleep time in milliseconds
	P_ADDSYSTEMCALL(P_TASK_DELAY,0);
	Sleep((DWORD)_WCE_Tick2Millisecond(ticks)+1);  //+1 to avoid Sleep(0)
}


/*!-------------------------------------------------------------------------------------
 *  Strange function always used like this : task_context(NULL,NULL) and return 
	 #define task_context_task       0
	 #define task_context_interrupt  1

 *
 * @param **task : NULL
 * @param level : 
 *
 * @return task_context_t  : 
 */
task_context_t task_context(task_t **task, int* level)
{
 	WCE_MSG(MDL_OSCALL,"-- task_context(...)");

   // JLX: in the WinCE porting, we are always in task context (ISRs do not call STAPI "drivers")
   return task_context_task; 
}


void _WCE_TaskInitilize()
{

#ifdef TRACE_THREAD_NAME
	ResetThreadName();
#endif

	InitializeCriticalSection(&gCriticalHandler);
	// note in Win CE ID == Handle
	dummyMainTask.dwThreadId_opaque  = GetCurrentThreadId();
	dummyMainTask.hHandle_opaque     = (void *)GetCurrentThreadId();
	dummyMainTask.dwCeTaskPriority   = CeGetThreadPriority(dummyMainTask.hHandle_opaque );
	dummyMainTask.dwOs21TaskPriority = 4 ; //MAPPRIORITYFROMCETOOS21(dummyMainTask.dwCeTaskPriority);
	_AddThreadList(&dummyMainTask);
}


void _WCE_TaskTerminate()
{
	DeleteCriticalSection(&gCriticalHandler);

}



/*!-------------------------------------------------------------------------------------
 * For the wrapper we try to emulate this function by WINCE Enter and Leave CriticalSection 
 *
 * @param void : 
 *
 * @return void  : 
 */
void task_lock(void)
{
	// Now the current thread is locked
	// we should call task_unlock to after this state 
	// 
 	WCE_MSG(MDL_OSCALL,"-- task_lock()");

	EnterCriticalSection(&gCriticalHandler);
}


/*!-------------------------------------------------------------------------------------
 * For the wrapper we try to emulate this function by WINCE Enter and Leave CriticalSection 
 *
 * @param void : 
 *
 * @return void  : 
 */
void task_unlock(void)
{
 	WCE_MSG(MDL_OSCALL,"-- task_unlock()");
	LeaveCriticalSection(&gCriticalHandler);
	// we can now re call the current thread

}



/*!-------------------------------------------------------------------------------------
 * task_priority_set() sets the priority of the task specified by task, or of the
 * currently active task if task is NULL. If this results in the current task priority
 * falling below that of another task which is ready to run, or a ready task now has a
 * priority higher than the current task, then tasks are rescheduled.
 * If the specified task owns a priority mutex then priority inversion logic is also run.
 * If the task owns a priority mutex for which a higher priority task is waiting, and the
 * call attempted to lower the task. priority, then the lowering of the priority is
 * deferred until the mutex is released.
 * If the specified task is waiting for a priority mutex or semaphore then its position in
 * the queue of waiting tasks is re-calculated. If the call attempted to raise the
 * specified task priority, and it was queuing for a priority mutex, then priority
 * inversion logic is also run, and may result in the temporary priority boosting of the
 * mutex current owning task
 *
 * @param *task : 
 * @param priority : 
 *
 * @return int  : 
 */
int task_priority_set(task_t *task, int priority)
{
	int oldPriority;
	WCE_ASSERT(task);
	oldPriority = task->dwOs21TaskPriority;
	task->dwOs21TaskPriority = priority;
	task->dwCeTaskPriority   = _WCE_ConvertLevelPriorityFrom(task,priority);
 	WCE_MSG(MDL_OSCALL,"-- task_priority_set(%x,%d)",task,priority);
	WCE_VERIFY(CeSetThreadPriority(task->hHandle_opaque,task->dwCeTaskPriority) != 0);
	return oldPriority;
}


/*!-------------------------------------------------------------------------------------
 *  Returns a pointer to the task structure of the currently active task.
 *  Warning, I assume that this task_t will never be used with task_delete
 * @param void : 
 *
 * @return task_t*  : 
 */


task_t* task_id(void)
{

	task_t*taskp = _GetCurrentThread();
	WCE_ASSERT(taskp);
 	WCE_MSG(MDL_OSCALL,"-- task_id()");
	return taskp;
}



/*!-------------------------------------------------------------------------------------
	This function reschedules the current task, moving it to the back of the current
	priority scheduling list, and selecting the new task from the front of the list. If the
	scheduling list was empty before this call, then it has no effect, otherwise it
	performs a timeslice at the current priority.
	If task_reschedule() is called while a task_lock() is in effect, it does not
	cause a reschedule.
 *
 * @param void : 
 *
 * @return void   : 
 */
void task_reschedule(void)
{
 	WCE_MSG(MDL_OSCALL,"-- task_reschedule()");

	Sleep(0);
}


/*!-------------------------------------------------------------------------------------
 This function returns the name of the specified task, or if task is NULL, the current
 task. The task name is set when the task is created.
 *
 * @param taskp : 
 *
 * @return char  : 
 */
char *task_name(task_t * taskp)
{
 	WCE_MSG(MDL_OSCALL,"-- task_name()");

	WCE_ASSERT(taskp);
	return taskp->tName;

}




/*!-------------------------------------------------------------------------------------
	task_priority() retrieves the OS21 priority of the task specified by task or the
	priority of the currently active task if task is NULL.
	If the specified task is currently subject to a temporary priority boost by the priority
	inversion logic, then the nominal priority is returned, not the boosted priority. *
 * @param taskp : 
 *
 * @return int   : 
 */
int   task_priority(task_t * taskp)
{
 	WCE_MSG(MDL_OSCALL,"-- task_priority(...)");
	WCE_ASSERT(taskp);
	return taskp->dwOs21TaskPriority;
}



/*!-------------------------------------------------------------------------------------
 * This function returns the task descriptor of the next task on OS21s internal list of
tasks. Passing a NULL parameter returns the first task on the list. This enables the
caller to enumerate all OS21 tasks on the system.
The caller should bracket calls to this function with task_lock() and
task_unlock() to ensure that a consistent list of tasks are returned.
 *
 * @param taskp : 
 *
 * @return task_t   : 
 */
task_t  *task_list_next(task_t * taskp)
{
 	WCE_MSG(MDL_OSCALL,"-- task_list_next(...)");

	if(taskp == 0) return gListThread;

	return taskp->pThreadNext;
}


/*!-------------------------------------------------------------------------------------
	task_kill() kills the task specified by task, causing it to stop running, and call
	its exit handler. If task is NULL then the current task is killed. If the task was
	waiting on any objects when it is killed, it is removed from the list of tasks waiting
	for that object before the exit handler is called.
	status is the exit status for the task. Thus task_kill() can be viewed as a way
	of forcing the task to call task_exit(status).
	Normally flags should have the value 0. However, by specifying the value
	task_kill_flags_no_exit_handler, it is possible to prevent the task calling its
	exit handler, and so it terminates immediately, never running again.
	A task can temporarily make itself immune to being killed by calling
	task_immortal(), see Section 4.10: Killing a task on page 72 for more details.
	When a task which has made itself immortal is killed, task_kill() returns
	immediately, but the killed task does not die until it makes itself mortal again.
	Note: task_kill() may return before the task has died. A task_kill() should
	normally be followed by a task_wait() to be sure that the task has made itself
	mortal again, and completed its exit handler. If the task is mortal, its exit handlers
	will be called from the killing task context; not the context of the task being killed.
	task_kill() cannot be called from an interrupt handler.
 *
 * @param taskp : 
 * @param status : 
 * @param flags : 
 *
 * @return int   : 
 */
int   task_kill(task_t * taskp, int status, task_kill_flags_t flags)
{
	WCE_ASSERT(taskp);
	WCE_ASSERT(flags == 0);
	WCE_VERIFY(TerminateThread(taskp->hHandle_opaque,status));
 	WCE_MSG(MDL_OSCALL,"-- task_kill(...)");

	
	return OS21_SUCCESS;
}



/*!-------------------------------------------------------------------------------------
	This function resumes the specified task. The task must previously have been
	suspended, either by calling task_suspend(), or created by specifying a flag of
	task_flags_suspended to task_create() or task_create_p().
	If the task is suspended multiple times, by more than one call to task_suspend(),
	then an equal number of calls to task_resume() are required before the task
	starts to execute again.
	If the task was waiting for an object (for example, waiting on a semaphore) when it
	was suspended, then that event must also occur before the task starts executing.
	When a task is resumed it starts executing the next time it is the highest priority
	task, and so may preempt the task calling task_resume().
  *
 * @param taskp : 
 *
 * @return int   : 
 */
int   task_resume(task_t * taskp)
{
	WCE_ASSERT(taskp);
	WCE_VERIFY(ResumeThread(taskp->hHandle_opaque));
 	WCE_MSG(MDL_OSCALL,"-- task_resume(...)");

	return OS21_SUCCESS;

}


/*!-------------------------------------------------------------------------------------
	This function suspends the specified task. If task is NULL then this suspends the
	current task. task_suspend() stops the task from executing immediately, until it
	is resumed using task_resume().
 *
 * @param taskp : 
 *
 * @return int   : 
 */
int   task_suspend(task_t * taskp)
{
	WCE_ASSERT(taskp);
	WCE_VERIFY(SuspendThread(taskp->hHandle_opaque));
	WCE_MSG(MDL_OSCALL,"-- task_suspend()");

	return OS21_SUCCESS;

}




/*!-------------------------------------------------------------------------------------
 * task_data_set() sets the task-data pointer of the task specified by task, or of
 * the currently active task if task is NULL. See Section 4.13: Task data on page 74.
 *
 * @param taskp : 
 * @param datap : 
 *
 * @return void *  : 
 */
void *   task_data_set (task_t * taskp, void * datap)
{
	void *pRrev;
	if(taskp ==0) 
	{
		taskp = _GetCurrentThread();
	}
	WCE_ASSERT(taskp);

	pRrev = taskp->dwdatap ;
	taskp->dwdatap =datap;
	WCE_MSG(MDL_OSCALL,"-- task_data_set(%0x,%0x)",taskp,datap);

	return pRrev;
}



/*!-------------------------------------------------------------------------------------
 * task_data() retrieves the task-data pointer of the task specified by task, or the
 *  currently active task if task is NULL. See Section 4.13: Task data on page 74.
 *
 * @param task : 
 *
 * @return void*  : 
 */
void* task_data(task_t* taskp)
{
	WCE_MSG(MDL_OSCALL,"-- task_data(%0x,...)",taskp);

	if(taskp ==0) 
	{
		taskp = _GetCurrentThread();
	}
	WCE_ASSERT(taskp);
	return taskp->dwdatap;
}



/*!-------------------------------------------------------------------------------------
This causes the current task to terminate, after having called the onexit handler. It
has the same effect as the task returning from its entry point function.
 */
void  task_exit(int param)
{
	task_t* taskp = _GetCurrentThread();
	WCE_ASSERT(taskp);
	TerminateThread(taskp->hHandle_opaque,param);
}


void WINCE_DumpTasks()
{
	int cptThread = 0;
    task_t* parent = NULL;
    for (parent = gListThread; parent != NULL; parent = parent->pThreadNext)
	{
			_WCE_Printf("%03d: %s CePrio = %03d, OS21Prio %03d\n",cptThread,parent->tName,parent->dwCeTaskPriority,parent->dwOs21TaskPriority);		
			cptThread++;

	}

}

