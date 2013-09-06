/* Copyright 2008 SMSC, All rights reserved
FILE: task_manager.c
*/

#include "smsc_environment.h"

#if SMSC_THREADING_ENABLED
#include "smsc_threading.h"
#endif

#include "task_manager.h"

SMSC_SEMAPHORE ptcpip_sema;/* add 20090112*/
char nettask_name[20],task_no =0;
void Task_Initialize(
	PTASK task, u8_t priority,
	TASK_FUNCTION taskFunction,void * taskParameter)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task!=NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(taskFunction!=NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(priority<=TASK_MANAGER_HIGHEST_PRIORITY);
	memset(task,0,sizeof(TASK));
	ASSIGN_SIGNATURE(task,TASK_SIGNATURE);
	task->mTaskState=TASK_STATE_IDLE;
	task->mPriority=priority;
	task->mTaskFunction=taskFunction;
	task->mTaskParameter=taskParameter;
	ptcpip_sema = smsc_semaphore_create( 1);/* add 20090112*/

#if 0
	sprintf(nettask_name,"task%d",task_no);
	task_no++;
	smsc_thread_create(taskFunction,taskParameter,1024 * 10,priority,(char *)nettask_name,0);
#endif
}
void Task_SetParameter(PTASK task, void * taskParameter)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task!=NULL);
	CHECK_SIGNATURE(task,TASK_SIGNATURE);
	task->mTaskParameter=taskParameter;
}

#if SMSC_THREADING_ENABLED
static SMSC_THREAD gTaskRunningThread=SMSC_THREAD_NULL;
#endif

/* mCheckInterruptListFlag: Interrupt routines will set this flag 
    by use of TaskManager_SignalActivation to signal 
	the task manager to check the tasks in the interrupt list */
static volatile u8_t gCheckInterruptListFlag=0;

/* gInterruptActivationList: This is the list of tasks that are waiting
   for interrupts to activate them */
static PTASK gInterruptActivationList=NULL;

/* gTimerActivationList: This is the list of tasks that are waiting
	for a certain time. The earliest time should be at the beginning of
	the list.*/
static PTASK gTimerActivationList=NULL;

/* gHighestPriority, indicates the priority of the highest priority
	scheduled task */
static u8_t gHighestPriority=0;

/* gTaskQueueHeads and gTaskQueueTails, maintains a queue for each priority level */
static PTASK gTaskQueueHeads[TASK_MANAGER_HIGHEST_PRIORITY+1];
static PTASK gTaskQueueTails[TASK_MANAGER_HIGHEST_PRIORITY+1];

static void TaskManager_PushTask(PTASK task)
{
	u8_t priority;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task!=NULL);
	CHECK_SIGNATURE(task,TASK_SIGNATURE);
	SMSC_ASSERT(task->mNextTask==NULL);	
	priority=task->mPriority;
	SMSC_ASSERT(priority<=TASK_MANAGER_HIGHEST_PRIORITY);
	if(gTaskQueueHeads[priority]==NULL) {
		SMSC_ASSERT(gTaskQueueTails[priority]==NULL);
		gTaskQueueHeads[priority]=task;
		gTaskQueueTails[priority]=task;
	} else {
		SMSC_ASSERT(gTaskQueueTails[priority]!=NULL);
		gTaskQueueTails[priority]->mNextTask=task;
		gTaskQueueTails[priority]=task;
	}
	if(priority>gHighestPriority) {
		gHighestPriority=priority;
	}
}

static PTASK TaskManager_PopTask(u8_t priority)
{
	PTASK result=NULL;
	SMSC_ASSERT(priority<=TASK_MANAGER_HIGHEST_PRIORITY);
	if(gTaskQueueHeads[priority]!=NULL) {
		SMSC_ASSERT(gTaskQueueTails[priority]!=NULL);
		result=gTaskQueueHeads[priority];
		gTaskQueueHeads[priority]=result->mNextTask;
		if(gTaskQueueHeads[priority]==NULL) {
			gTaskQueueTails[priority]=NULL;
		} else {
			result->mNextTask=NULL;
		}
	} else {
		SMSC_ASSERT(gTaskQueueTails[priority]==NULL);
	}
	return result;
}

static u8_t gStopRunningFlag=0;
#if SMSC_THREADING_ENABLED
static SMSC_SEMAPHORE gActivitySignal;
#endif

SMSC_ERROR_ONLY(static u8_t gTaskManagerInitialized=0;)

void TaskManager_Initialize(void)
{
	gCheckInterruptListFlag=0;
	gInterruptActivationList=NULL;
	gTimerActivationList=NULL;
	memset(gTaskQueueHeads,0,sizeof(PTASK)*(TASK_MANAGER_HIGHEST_PRIORITY+1));
	memset(gTaskQueueTails,0,sizeof(PTASK)*(TASK_MANAGER_HIGHEST_PRIORITY+1));
	SMSC_ERROR_ONLY(gTaskManagerInitialized=1;)
	#if SMSC_THREADING_ENABLED
	gActivitySignal = smsc_semaphore_create(0);
	gTaskRunningThread=SMSC_THREAD_NULL;
	#endif
}

void TaskManager_CancelTask(PTASK task)
{
	PTASK thisTask;
	PTASK lastTask;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task!=NULL);
	CHECK_SIGNATURE(task,TASK_SIGNATURE);
	
	/* Search Interrupt Activation List */
	thisTask=gInterruptActivationList;
	lastTask=NULL;
	while(thisTask!=NULL) {
		if(thisTask==task) {
			if(lastTask==NULL) {
				gInterruptActivationList=task->mNextTask;
			} else {
				lastTask->mNextTask=task->mNextTask;
			}
			task->mNextTask=NULL;
			task->mTaskState=TASK_STATE_IDLE;
			return;
		} else {
			lastTask=thisTask;
			thisTask=thisTask->mNextTask;
		}
	}
	
	/* Search Timer Activation List */
	thisTask=gTimerActivationList;
	lastTask=NULL;
	while(thisTask!=NULL) {
		if(thisTask==task) {
			if(lastTask==NULL) {
				gTimerActivationList=task->mNextTask;
			} else {
				lastTask->mNextTask=task->mNextTask;
			}
			task->mNextTask=NULL;
			task->mTaskState=TASK_STATE_IDLE;
			return;
		} else {
			lastTask=thisTask;
			thisTask=thisTask->mNextTask;
		}
	}
	
	/* Search Task Queue */
	thisTask=gTaskQueueHeads[task->mPriority];
	lastTask=NULL;
	while(thisTask!=NULL) {
		if(thisTask==task) {
			if(lastTask==NULL) {
				gTaskQueueHeads[task->mPriority]=task->mNextTask;
				if(gTaskQueueHeads[task->mPriority]==NULL) {
					gTaskQueueTails[task->mPriority]=NULL;
				}
			} else {
				if(task==gTaskQueueTails[task->mPriority]) {
					SMSC_ASSERT(task->mNextTask==NULL);
					gTaskQueueTails[task->mPriority]=lastTask;
				}
				lastTask->mNextTask=task->mNextTask;
			}
			task->mNextTask=NULL;
			task->mTaskState=TASK_STATE_IDLE;
			return;
		} else {
			lastTask=thisTask;
			thisTask=thisTask->mNextTask;
		}
	}
	SMSC_WARNING(1,("TaskManager_CancelTask: Could not find task to cancel"));
}

void TaskManager_ScheduleAsSoonAsPossible(PTASK task)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(gTaskManagerInitialized);
	
#if SMSC_THREADING_ENABLED
	/* This function should only be called by the task runner thread */
	SMSC_ASSERT(gTaskRunningThread?(gTaskRunningThread==smsc_thread_get_current()):1);
#endif

	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task!=NULL);
	CHECK_SIGNATURE(task,TASK_SIGNATURE);
	SMSC_ASSERT(task->mNextTask==NULL);
	SMSC_ASSERT(task->mTaskState==TASK_STATE_IDLE);
	SMSC_ASSERT(task->mPriority<=TASK_MANAGER_HIGHEST_PRIORITY);
	//SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task->mTaskFunction!=NULL);
	task->mTaskState=TASK_STATE_READY_TO_RUN;
	TaskManager_PushTask(task);
}

void TaskManager_ScheduleByTimer(PTASK task,u32_t milliSeconds)
{	
	SMSC_ASSERT(gTaskManagerInitialized);
#if SMSC_THREADING_ENABLED
	/* This function should only be called by the task runner thread */
	SMSC_ASSERT(gTaskRunningThread?(gTaskRunningThread==smsc_thread_get_current()):1);
#endif
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task!=NULL);
	CHECK_SIGNATURE(task,TASK_SIGNATURE);
	SMSC_ASSERT(task->mNextTask==NULL);
	SMSC_ASSERT(task->mTaskState==TASK_STATE_IDLE);
	SMSC_ASSERT(task->mPriority<=TASK_MANAGER_HIGHEST_PRIORITY);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task->mTaskFunction!=NULL);
	task->mTaskState=TASK_STATE_WAITING_FOR_TIMER;
	task->mCallTime=smsc_time_plus(smsc_time_now(),smsc_msec_to_tick(milliSeconds));
	{/* Add task to the timer activation list */
		PTASK lastTask=NULL;
		PTASK currentTask=gTimerActivationList;
		/* make sure the list order matches the call time order */
		while((currentTask!=NULL)&&(smsc_time_after(task->mCallTime,currentTask->mCallTime)))
		{
			lastTask=currentTask;
			currentTask=currentTask->mNextTask;
		}
		if(lastTask==NULL) {
			gTimerActivationList=task;
		} else {
			lastTask->mNextTask=task;
		}
		task->mNextTask=currentTask;
	}
}

void TaskManager_ScheduleByInterruptActivation(PTASK task)
{
	SMSC_ASSERT(gTaskManagerInitialized);
	
#if SMSC_THREADING_ENABLED
	/* This thread should only be called by the task runner thread */
	SMSC_ASSERT(gTaskRunningThread?(gTaskRunningThread==smsc_thread_get_current()):1);
#endif

	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task!=NULL);
	CHECK_SIGNATURE(task,TASK_SIGNATURE);
	SMSC_ASSERT(task->mNextTask==NULL);
	/*SMSC_ASSERT(task->mTaskState==TASK_STATE_IDLE);*/
	SMSC_ASSERT(task->mPriority<=TASK_MANAGER_HIGHEST_PRIORITY);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task->mTaskFunction!=NULL);
	/*task->mTaskState=TASK_STATE_WAITING_FOR_INTERRUPT;*/
	task->mNextTask=gInterruptActivationList;
	gInterruptActivationList=task;
}

void TaskManager_InterruptActivation(PTASK task)
{
	/*This function is written so it can be called in an
		interrupt context */
	SMSC_ASSERT(gTaskManagerInitialized);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(task!=NULL);
	CHECK_SIGNATURE(task,TASK_SIGNATURE);
	/*SMSC_ASSERT(task->mTaskState==TASK_STATE_WAITING_FOR_INTERRUPT);*/
	task->mTaskState=TASK_STATE_READY_TO_RUN;
	gCheckInterruptListFlag=1;
#if SMSC_THREADING_ENABLED
	smsc_semaphore_signal(gActivitySignal);
#endif
}

void TaskManager_StopRunning(void)
{
	gStopRunningFlag=1;
}

void TaskManager_Run(void *arg)
{
	int foundActivity=1;
	s8_t priorityIndex;
	smsc_clock_t currentTime;
	#if SMSC_THREADING_ENABLED
	smsc_clock_t nextTime;
	SMSC_ASSERT(gTaskRunningThread==SMSC_THREAD_NULL);
	gTaskRunningThread=smsc_thread_get_current();
	#endif
	
	while(gStopRunningFlag==0) {
	/*	printf("task ruan1\n");*/
		if(!foundActivity) {
				/*	printf("task ruan2\n");*/

			/* no activity was found in the last loop
			   There for there can be no activity unless
			   it is signaled by an interrupt, another 
			   thread or a timer. */
		#if 0/*SMSC_THREADING_ENABLED*/
			if(!smsc_time_after(currentTime,nextTime)) {
				u32_t milliSeconds=smsc_tick_to_msec(smsc_time_minus(nextTime,currentTime));
				/*SMSC_TRACE(1,("TaskManager_Run: Waiting %"U32_F" milliSeconds",milliSeconds));*/
				smsc_semaphore_wait_timeout(gActivitySignal,milliSeconds);
			} else {
				/*SMSC_TRACE(1,("TaskManager_Run: Waiting until activation"));*/
				SMSC_ASSERT(0);
				smsc_semaphore_wait(gActivitySignal);
			}
			/*SMSC_TRACE(1,("TaskManager_Run: Wake up"));*/
		#else
		smsc_semaphore_wait_timeout(gActivitySignal,50);
		#endif
		}
		foundActivity=0;
		if(gCheckInterruptListFlag) {
			PTASK task=gInterruptActivationList;
			PTASK lastTask=NULL;
			gCheckInterruptListFlag=0;
			while(task!=NULL) {
					/*	printf("task ruan2\n");*/
				//	do_report(0,"\ntask ruan2\n");

				if(task->mTaskState==TASK_STATE_READY_TO_RUN) {
					/* Remove this task from the waiting for 
					   interrupt list */
					SMSC_ASSERT(task->mPriority<=TASK_MANAGER_HIGHEST_PRIORITY);
					SMSC_ASSERT(task->mTaskFunction!=NULL);
					if(lastTask==NULL) {
						/*This is the first task on the list*/
						gInterruptActivationList=task->mNextTask;
						task->mNextTask=NULL;
						TaskManager_PushTask(task);
						task=gInterruptActivationList;
						continue;
					} else {
						/* This is not the first task on the list */
						lastTask->mNextTask=task->mNextTask;
						task->mNextTask=NULL;
						TaskManager_PushTask(task);
						task=lastTask->mNextTask;
						continue;
					}
				}
				lastTask=task;
				task=task->mNextTask;
			}
		}
		currentTime=smsc_time_now();
		while((gTimerActivationList!=NULL)&&
			(smsc_time_after(currentTime,gTimerActivationList->mCallTime)))
		{
			PTASK task=gTimerActivationList;
			gTimerActivationList=task->mNextTask;
			task->mNextTask=NULL;
			task->mTaskState=TASK_STATE_READY_TO_RUN;
			TaskManager_PushTask(task);
			//		do_report(0,"\ntask ruan3\n");/*printf("task ruan3\n");*/

		}
		#if SMSC_THREADING_ENABLED
		if(gTimerActivationList!=NULL) {
			nextTime=gTimerActivationList->mCallTime;
		} else {
			/* ensure that nextTime is before currentTime */
			nextTime=smsc_time_minus(currentTime,smsc_sec_to_tick(1));
		}
		#endif
		/* Find the first highest priority task and run it */
		for(priorityIndex=(s8_t)gHighestPriority;priorityIndex>=0;priorityIndex--)
		{
			PTASK task;
			gHighestPriority=(u8_t)priorityIndex;
			if((task=TaskManager_PopTask((u8_t)priorityIndex))!=NULL) 
			{
            
				/*printf("task ruan1 task(%x)\n",task->mTaskFunction);*/
			//do_report(0,"\n task ruan1 %d \n",priorityIndex);
				SMSC_ASSERT(task->mTaskState==TASK_STATE_READY_TO_RUN);
				foundActivity=1;
				task->mTaskState=TASK_STATE_IDLE;
				SMSC_ASSERT(task->mTaskFunction!=NULL);
				smsc_semaphore_wait(ptcpip_sema);/* add 20090112*/
				task->mTaskFunction(task->mTaskParameter);
				smsc_semaphore_signal(ptcpip_sema);	/* add 20090112*/		
				/* Since a task was found and called, there could be new
				  higher priority tasks scheduled so lets begin the 
				  main loop again */
				break;/* exit for loop */
			}
		}
	}
	
	#if SMSC_THREADING_ENABLED
	gTaskRunningThread=SMSC_THREAD_NULL;
	#endif
}
