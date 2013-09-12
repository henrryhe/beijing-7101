/*! Time-stamp: <@(#)OS21WrapperTask.h   06/04/2005 - 14:32:43   William hennebois>
 *********************************************************************
 *  @file   : OS21WrapperTask.h
 *
 *  Project : STAPI Windows CE
 *
 *  Package : 
 *
 *  Company :  TeamLog for ST
 *
 *  Author  : William hennebois            Date: 06/04/2005
 *
 *  Purpose : Wrapper for OS21 Thread , this file should include 
 *  all structs defines and functions OS21 that should be re-implemented 
 *  on WinCE
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  06/04/2005 WH : First Revision
 *
 *********************************************************************
 */

#ifndef __OS21WRAPPERTASK__
#define  __OS21WRAPPERTASK__

////////////////////////////////////////////////////////
///////////////                 ////////////////////////
//////////////// Types		    ////////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////


/* To be adjusted */
#define MAX_USER_PRIORITY    255
#define MIN_USER_PRIORITY    0


int Priority_UCOS_Map( const char* taskname );


/* RT threads */
#define VIDEO_RT_THREADS

/* Task priorities 99 Highest - 0 Lowest */
#define STINTMR_THREAD_PRIORITY					15  /* OS20 - undefined */
#define STVID_THREAD_DISPLAY_PRIORITY         	12	/* OS20 - 12 */
#define STVID_THREAD_TRICKMODE_PRIORITY       	11	/* OS20 - 11 */
#define STVID_THREAD_DECODE_PRIORITY 			10	/* OS20 - 10 */
#define STVID_THREAD_INJECTERS_PRIORITY       	10	/* OS20 - 10 */
#define STVID_THREAD_ERROR_RECOVERY_PRIORITY  	9	/* OS20 - 9 */

#define STVID_INJECTION_THREAD_PRIORITY			5   /* OS20 - MIN */



// Some specifcs defines
#define task_flags_no_min_stack_size	  0
#define task_flags_high_priority_process  0 /* does not exist for ST40 */

void     task_delay_until (osclock_t ExitTime);



//! a flag used in task_create no longuer used
typedef int task_flags_t;       /* Unused */
/*
 * Task kill flags
 */
typedef unsigned int task_kill_flags_t;

#define task_kill_flags_no_exit_handler	(1)

typedef void *tdesc_t;


//! OS21 has 2 context of thread interrupt and task
//! No sens in the context of WINCE, but we keep them for compatibility

#define task_context_task       0
#define task_context_interrupt  1

// this is a redef of the OS21 func & struct
typedef struct
{
    void* task_stack_base; // Base address of the task stack
    int   task_stack_size; // Size of the task stack in bytes
    int   task_stack_used; // Amount of stack used by the task in bytes
    int   task_time;       // CPU time used by the task
    int   task_state;      // Running, terminated, or suspended
    int   task_exit_value; // Value set when task_exit() was called
} task_status_t; 
#define task_status(task, status, flags) {}

////////////////////////////////////////////////////////
///////////////                 ////////////////////////
//////////////// Struct		    ////////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////

//! the handle OS21 task_t becomes a struct with all information 
//! used for a windows CE thread, off course we use an opaque types
//! to avoid the include of windows.h in all STAPI module
//! See task_init,task_create,etc...

// Coould be define in stddefs.h, due to STOS compatibility.
#ifndef TASK_T
#define TASK_T
//#ifndef task_t
typedef struct	_wince_task 
{
	void			 *hHandle_opaque;				   // the thread handle (HANDLE)
	void			 (*pfnThread_opaque)(void* Param); // the Real STAPI Thread Function
	void			 *pThreadParameter;				   // the Real STAPI parameter pointer
	unsigned long     dwThreadId_opaque;			   // the thread ID, not used
	int				  dwOs21TaskPriority;			   // the task priority from 0 to 255
	int				  dwCeTaskPriority;				   // the task priority from 0 to 255
	char 			  tName[40];				       // task name
	void			  *dwdatap;						   // user data
	struct _wince_task *pThreadNext;				   // task next linkend
}task_t;
#endif /* TASK_T */

////////////////////////////////////////////////////////
///////////////                 ////////////////////////
//////////////// Task functions ////////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////

//! create a thread and sets the priority and CallBack function
//! I don't know why, the functions task_int replaces the function Task_Create
//! and do the same job ? so we use a define mapped on task_init to create the 
//! task_create for instance 
//! the priority passed to the fonction is the OS21 priority, the WINCE equivalent 
//! will be computed in the function

// int task_init(void (*pFunction)(void* Param), void* Param,void* Stack, size_t StackSize,task_t* Task, void * Tdesc,int Priority, const char* name,task_flags_t flags);

//! create a thread and sets the priority and CallBack function
//! I don't know why, the functions task_int replaces the function Task_Create
//! and do the same job ? so we use a define mapped on task_init to create the 
//! task_create for instance 
//! the priority passed to the fonction is the OS21 priority, the WINCE equivalent 
//! will be computed in the function

task_t * task_create(void (*pFunction)(void* Param), void* Param,size_t StackSize,int Priority, const char* name,task_flags_t flags);
// not used task_t *task_create_p( partition_t* TaskPartition ,void (*pFunction)(void* Param), void* Param,partition_t* StackPartition, size_t StackSize,int Priority, const char* name,task_flags_t flags);
// JLX: actually it is used in STVID
#define task_create_p(T_PART, FUNC, PARAM, S_PART, SIZE, PRIO, NAME, FLAGS) \
            task_create((FUNC), (PARAM), (SIZE), (PRIO), (NAME), (FLAGS))

//! stop the thread, an close the thread handle
//! 
int task_delete(task_t *task);

//! Wait the end of the thread, but don't destroy it
int task_wait(task_t **tasks, int taskcount, osclock_t *timeout);

//! Wait for n ticks, 64 ticks for 1 micro sec
void task_delay(osclock_t ticks);

//! Store User data
void *   task_data_set           (task_t * taskp, void * datap);
void *   task_data(task_t* task);


//! Always returns the same value
task_context_t task_context(task_t **task, int* level);


//TEAMLOG_PCL: a porter dans OS21WrapperTask 
int task_priority_set(task_t *task, int priority);
task_t* task_id(void);
//!This function prevents the kernel scheduler from preempting or timeslicing the
//!current task, although the task can still be interrupted by interrupt handlers.
//!This function should always be called as a pair with task_unlock(), so that it can
//!be used to create a critical region in which the task cannot be preempted by another
//!task. If the task deschedules the lock is terminated while the thread is not running.
//!When the task is rescheduled the lock is re-instated. Calls to task_lock() can be
//!nested, and the lock is not released until an equal number of calls to
//!task_unlock() have been made.

void task_lock(void);
void task_unlock(void);
void task_reschedule(void);
char *task_name(task_t * taskp);
int   task_priority(task_t * taskp);
task_t*task_list_next(task_t * taskp);
int   task_kill(task_t * taskp, int status, task_kill_flags_t flags);
int   task_resume(task_t * taskp);
int   task_suspend(task_t * taskp);
int	  _WCE_ConvertLevelPriorityFrom(task_t * taskp,int os21prioritylevel);
void  task_exit(int param);
void WINCE_DumpTasks();
char * _WCE_EnumStapiTaskName(int Index); //WHE  monitoring for dshow project
int    _WCE_SetCeTaskPriority(int Index, int WincePrority);//WHE monitoring for  dshow project
int	   _WCE_GetCeTaskPriority(int Index,HANDLE **pHandle);//WHE monitoring for  dshow project



#endif // 
