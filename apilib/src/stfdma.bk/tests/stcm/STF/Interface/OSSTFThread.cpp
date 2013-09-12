///
/// @file STF\Source\OSAL\OS20_OS21\OSSTFThread.cpp
///
/// @brief Implementation for OS20 and OS21 specific Threads
///
/// @par OWNER: 
///             STF Team
///
/// @author     Rolland Veres
///
/// @par SCOPE:
///             INTERNAL Source File
///
/// @date       2002-12-05 
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///


#include "OSSTFThread.h"
#include "OSDefines.h"


// Set to see more debug messages (default of 0 only prints sever warnings)
#define DEBUG_THREAD_CONTROL (_DEBUG && 0)


void OSSTFThreadEntryCall(void * p)
	{
	((OSSTFThread *)p)->ThreadEntry();
	}

extern "C" 
	{
	void OSSTFThreadEntryCallC(void * p)
		{
		OSSTFThreadEntryCall(p);
		}
	}

void OSSTFThread::ThreadEntry(void)
	{
	// Store the 'this' pointer in the thread's data
	task_data_set(task, (void*)bthread);

	bthread->ThreadEntry();

   bthread->terminate = true;
	}

OSSTFThread::OSSTFThread(STFBaseThread * bthread, STFString name, uint32 stackSize, STFThreadPriority priority)
	{
	this->bthread = bthread;
	this->name = name;
	this->stackSize = max(stackSize, OSAL_MIN_STACK_SIZE);
#if _DEBUG
	if (stackSize < OSAL_MIN_STACK_SIZE)
		DP("*** WARNING! OSSTFThread::OSSTFThread: Thread %s specifies stack %d which is smaller than OSAL_MIN_STACK_SIZE(%d)!\n", 
		   (char*) name, stackSize,
			OSAL_MIN_STACK_SIZE);
#endif
	this->priority = priority;
	task = NULL;

	prioritySet		= true;
	stackSizeSet	= true;
	}

OSSTFThread::OSSTFThread(STFBaseThread * bthread, STFString name)
	{
	this->bthread = bthread;
	this->name = name;
	task = NULL;

	prioritySet		= false;
	stackSizeSet	= false;
	}

OSSTFThread::OSSTFThread(STFBaseThread * bthread)
	{
	this->bthread = bthread;
	this->name = "root thread";
	this->stackSize = OSAL_MIN_STACK_SIZE; //Dummy value. Thread already exists.
	this->priority = STFTP_NORMAL;
	task = task_id();

	// Store the thread pointer in the thread's data
	task_data_set(task, (void*)bthread);

	prioritySet		= true;
	stackSizeSet	= true;
	}

OSSTFThread::~OSSTFThread(void)
	{
	TerminateThread();
	}


STFResult OSSTFThread::SetThreadStackSize(uint32 stackSize)
	{
	this->stackSize = max(stackSize, OSAL_MIN_STACK_SIZE);
#if _DEBUG
	if (stackSize < OSAL_MIN_STACK_SIZE)
		DP("*** WARNING! OSSTFThread::SetThreadStackSize: Thread %s specifies stack %d which is smaller than OSAL_MIN_STACK_SIZE(%d)!\n", 
			(char*) name, stackSize, 
			OSAL_MIN_STACK_SIZE);
#endif
	stackSizeSet = true;
	STFRES_RAISE_OK;
	}


STFResult OSSTFThread::SetThreadName(STFString name)
	{
	this->name = name;
	STFRES_RAISE_OK;
	}

STFResult OSSTFThread::StartThread(void)
	{
	if (task && bthread->terminate)
		{
		STFRES_REASSERT(Wait());
		}

	if (!task)
		{
		// Do not allow starting the thread if there's no priority or stack size set.
		if (!prioritySet || !stackSizeSet)
			{
			DP("WARNING: Could not start thread because no stack size or priority set!\n");
			STFRES_RAISE(STFRES_OPERATION_PROHIBITED);
			}

		bthread->terminate = false;

		task = task_create(OSSTFThreadEntryCallC, (void*)this, stackSize, (priority * MAX_USER_PRIORITY) / STFTP_CRITICAL, name, (task_flags_t)0);

#if DEBUG_THREAD_CONTROL
		char threadName[100];
		this->name.Get(threadName, 100);
		DP("OSSTFThread::StartThread of %s with priority %d, OS priority %d, stack %d, OS task %08x\n", threadName, priority, (priority * MAX_USER_PRIORITY) / STFTP_CRITICAL, stackSize, task);
#endif		

		if (task == NULL)
			STFRES_RAISE(STFRES_NOT_ENOUGH_MEMORY);

		STFRES_RAISE_OK;
		}
	else
		STFRES_RAISE(STFRES_OBJECT_EXISTS);
	}

STFResult OSSTFThread::StopThread(void)
	{
	if (task)
		{
		bthread->terminate = true;

		STFRES_REASSERT(bthread->NotifyThreadTermination());

		STFRES_RAISE_OK;
		}
	else
		STFRES_RAISE(STFRES_OBJECT_NOT_FOUND);
	}

STFResult OSSTFThread::SuspendThread(void)
	{
	if (task)
		{
		task_suspend(task);

		STFRES_RAISE_OK;
		}
	else
		STFRES_RAISE(STFRES_OBJECT_NOT_FOUND);
	}

STFResult OSSTFThread::ResumeThread(void)
	{
	if (task)
		{
		task_resume(task);

		STFRES_RAISE_OK;
		}
	else
		STFRES_RAISE(STFRES_OBJECT_NOT_FOUND);
	}

STFResult OSSTFThread::TerminateThread(void)
	{
	if (task)
		{
		bthread->terminate = true;

		task_kill(task, 0, (task_kill_flags_t)0);
		task_delete(task);
		task = NULL;

		STFRES_RAISE_OK;
		}
	else
		STFRES_RAISE(STFRES_OBJECT_NOT_FOUND);
	}

STFResult OSSTFThread::Wait(void)
	{
	if (task)
		{
		task_wait(&task, 1, TIMEOUT_INFINITY);

		task_delete(task);
		task = NULL;

		STFRES_RAISE_OK;
		}
	else
		STFRES_RAISE(STFRES_OBJECT_NOT_FOUND);
	}

STFResult OSSTFThread::WaitImmediate(void)
	{
	if (task)
		{
		if (OS2X_FAILURE == task_wait(&task, 1, TIMEOUT_IMMEDIATE))
			STFRES_RAISE(STFRES_TIMEOUT);

		task_delete(task);
		task = NULL;

		STFRES_RAISE_OK;
		}
	else
		STFRES_RAISE(STFRES_OBJECT_NOT_FOUND);
	}



STFResult OSSTFThread::SetThreadPriority(STFThreadPriority pri)
	{
	prioritySet = true;

	if (task)
		{
		if (priority != pri)
			task_priority_set(task, (priority * MAX_USER_PRIORITY) / STFTP_CRITICAL);
		}

	priority = pri;

	STFRES_RAISE_OK;
	}

/// Retrieve the name of the thread
STFString OSSTFThread::GetName(void) const
	{
	return this->name;
	}

class STFRootThread : public STFThread
	{
	protected:
		void ThreadEntry(void) {}
		STFResult NotifyThreadTermination(void) {STFRES_RAISE_OK;}

	};

STFResult InitializeSTFThreads(void)
	{
	static bool initialized = false;

	if (!initialized)
		{
		initialized = true;

		new STFRootThread();
		}
	else
		{
		DP("Warning: InitializeSTFThreads() has been called more than once!!\n");
		assert(false);
		}

	STFRES_RAISE_OK;
	}

STFResult GetCurrentSTFThread(STFThread * & thread)
	{
	// Get the pointer to the current 
	thread = (STFThread *)task_data(task_id());

	STFRES_RAISE_OK;
	}
