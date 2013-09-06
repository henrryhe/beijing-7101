/*****************************************************************************
* Copyright 2008 SMSC, All Rights Reserved
* FILE: task_manager.h
*
* PURPOSE: This file declares functions used for task management.
*
* BRIEF IMPLEMENTATION DESCRIPTION:
* The task manager as it is implemented here provides for basic priority based 
* function call scheduling. A task should not be confused with a thread. Some 
* environments use the term "task" to mean thread. Here the term "task" is the
* concept of a function, a function parameter, and a description of when to 
* call it.
* 
* This task manager is primarily designed to run in a single threaded 
* environment (SMSC_THREADING_ENABLED==0). In this case the task manager
* spins in a main loop(within TaskManager_Run) waiting for the right time to
* call the scheduled tasks. If we are running in a multithreaded environment
* (SMSC_THREADING_ENABLED!=0) then the main loop will wait on a semaphore 
* when there is no work to be done.
*
* A task is initialize with a given priority using the function
*    Task_Initialize
*
* You may schedule a task in three ways
* 1) TaskManager_ScheduleAsSoonAsPossible
*    Will schedule the task to run as soon as possible.
* 2) TaskManager_ScheduleByTimer
*    Will schedule the task to run after a specified number of milliseconds
* 3) TaskManager_ScheduleByInterrupt
*    Will schedule the task to run after a call to TaskManager_InterruptActivation
*    TaskManager_InterruptActivation is designed to be called from with in an
*    interrupt function. This scheduling method can also be used to allow
*    other threads to signal a task to run with in the task manager thread. 
*
* All task scheduling must be done in a single threaded environment. By that we
* mean there should never be multiple threads scheduling tasks simultaneously
* before the call to TaskManager_Run. And while TaskManager_Run is running, all
* scheduling must be done by the same thread that is running TaskManager_Run.
* That means scheduling is permitted during single threaded initialization,
* and with in the tasks called by the TaskManager_Run function. These rules are
* required because there is no multi thread protection used in the scheduling
* functions.
*
* All tasks should be implemented in a friendly manner to allow coexistance
* with other tasks. Since these tasks are not threads, their CPU time will 
* never be stolen from them. That means that if they have a lot of work to do
* they should be implemented to voluntarily give up the CPU and reschedule
* them selves to run again later. This allows TaskManager_Run to regain control
* and possibly call other tasks that have a higher priority.
* 
*****************************************************************************/
      
#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#if SMSC_ERROR_ENABLED
#define TASK_SIGNATURE	(0xF2ECDD94)
#endif

/*****************************************************************************
* FUNCTION TYPE: TASK_FUNCTION
* PURPOSE: This is the type of function called by the task manager
*****************************************************************************/
typedef void (*TASK_FUNCTION)(void * param);

typedef enum TASK_STATE_ {
/* TASK_STATE_IDLE indicates task is not currently scheduled in any way */
	TASK_STATE_IDLE=0,

/* TASK_STATE_WAITING_FOR_INTERRUPT means the task was scheduled 
	using TaskManager_ScheduleByInterruptActivation */
	TASK_STATE_WAITING_FOR_INTERRUPT,

/* TASK_STATE_WAITING_FOR_TIMER means the task is waiting for 
	a certain time before running. It means it was scheduled
	by TaskManager_ScheduleByTimer */
	TASK_STATE_WAITING_FOR_TIMER,

/* TASK_STATE_READY_TO_RUN means the task is ready to run */	
	TASK_STATE_READY_TO_RUN
} TASK_STATE, * PTASK_STATE;

/*****************************************************************************
* STRUCTURE: TASK
* DESCRIPTION: This structure is allocated by other modules and initialized with
*    Task_Initialize.
*****************************************************************************/
typedef struct TASK_ {
	DECLARE_SIGNATURE
	struct TASK_ * mNextTask;
	u8_t mPriority;/* 0=lowest priority*/

	/* mTaskState is set to one of the TASK_STATE_* properties */
	TASK_STATE mTaskState;
	
	/* mCallTime, specifies the time this task should be called */
	smsc_clock_t mCallTime;
	
	/* mTaskParameter, this is the parameter used when calling mTaskFunction */
	void * mTaskParameter;
	
	/* mTaskFunction, this is the function called when conditions permit */
	TASK_FUNCTION mTaskFunction;
} TASK, * PTASK;

/*****************************************************************************
* FUNCTION: Task_Initialize
* DESCRIPTION: 
*    Initializes a task structure. This must be called on a task before it 
*    can be scheduled.
* PARAMETERS:
*    task, pointer to the task structure to be initialized
*    priority, This decides the order with which task functions are called. 
*              When more than one task is ready to run, the tasks with higher
*              priority(higher value) are run first. The priority value
*              should never be set higher than TASK_MANAGER_HIGHEST_PRIORITY.
*    taskFunction, pointer to the function that will be called.
*    taskParameter, parameters that will be passed to the taskFunction
*****************************************************************************/
void Task_Initialize(
	PTASK task,u8_t priority,
	TASK_FUNCTION taskFunction,void * taskParameter);

/*****************************************************************************
* FUNCTION: Task_SetParameter
* DESCRIPTION:
*    Reassignes the parameter of the task. 
*****************************************************************************/
void Task_SetParameter(PTASK task, void * taskParameter);

/*****************************************************************************
* MACRO: Task_IsIdle
* DESCRIPTION:
*    Returns non-zero if the task is idle (not scheduled).
*    Returns zero if the task is not idle (is scheduled).
*****************************************************************************/
#define Task_IsIdle(task)	((task)->mTaskState==TASK_STATE_IDLE)

/*****************************************************************************
* FUNCTION: TaskManager_Initialize
* DESCRIPTION:
*    Initializes the task manager. This must be call before any other 
*    TaskManager_ functions are called.
*****************************************************************************/
void TaskManager_Initialize(void);

/*****************************************************************************
* FUNCTION: TaskManager_CancelTask:
* DESCRIPTION: Cancels a task that was previously scheduled
*    but not yet called. This function has the same single threaded
*    requirements as the scheduler functions. 
*****************************************************************************/
void TaskManager_CancelTask(PTASK task);

/*****************************************************************************
* FUNCTION: TaskManager_ScheduleAsSoonAsPossible
* DESCRIPTION:  
*    Schedules a task to be run as soon as possible
*****************************************************************************/
void TaskManager_ScheduleAsSoonAsPossible(PTASK task);

/*****************************************************************************
* FUNCTION: TaskManager_ScheduleByTimer
* DESCRIPTION:
*    Schedules a task to be run after a given number of milliSeconds
*****************************************************************************/
void TaskManager_ScheduleByTimer(PTASK task, u32_t milliSeconds);

/*****************************************************************************
* FUNCTION: TaskManager_ScheduleByInterruptActivation
* DESCRIPTION: 
*    Prepares a task for an interrupt activation where the
*    interrupt will call TaskManager_InterruptActivation
* NOTE: This function name is deprecated, use
*    TaskManager_ScheduleBySignalActivation instead
*****************************************************************************/
void TaskManager_ScheduleByInterruptActivation(PTASK task);
#define TaskManager_ScheduleBySignalActivation TaskManager_ScheduleByInterruptActivation

/*****************************************************************************
* FUNCTION: TaskManager_InterruptActivation
* DESCRIPTION: 
*    This function can be called from an interrupt sub routine
*    to activate a task that was previously scheduled with
*    TaskManager_ScheduleByInterruptActivation
* NOTE: This function name is deprecated, use
*    TaskManager_SignalActivation instead
*****************************************************************************/
void TaskManager_InterruptActivation(PTASK task);
#define TaskManager_SignalActivation TaskManager_InterruptActivation

/*****************************************************************************
* FUNCTION: TaskManager_Run
* DESCRIPTION:
*    Runs all scheduled tasks, will not exit until
*    TaskManager_StopRunnning is called 
*****************************************************************************/
void TaskManager_Run(void * arg);

/*****************************************************************************
* FUNCTION: TaskManager_StopRunning
* DESCRIPTION:
*    Signals the TaskManager_Run function to exit at its 
*    earliest convenience, can be called from any thread, any time.
*****************************************************************************/
void TaskManager_StopRunning(void);

#endif
