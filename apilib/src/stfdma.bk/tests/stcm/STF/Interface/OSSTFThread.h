///
/// @file STF\Interface\OSAL\OS20_OS21\OSSTFThread.h
///
/// @brief Header for OS20 and OS21 specific threads
///
/// @par OWNER: 
///             STF Team
///
/// @author     Rolland Veres
///
/// @par SCOPE:
///             INTERNAL Header File
///
/// @date       2002-12-09 
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#include "STF/Interface/STFThread.h"

#ifndef OSSTFTHREADS_H
#define OSSTFTHREADS_H

#include "STF/Interface/Types/STFBasicTypes.h"
#include "STF/Interface/Types/STFResult.h"
#include "STF/Interface/Types/STFString.h"

#include "OSDefines.h"

///
/// @class OSSTFThread

class OSSTFThread
	{
	protected:
		task_t					*	task;
		STFBaseThread			*	bthread;
		
		bool							prioritySet, stackSizeSet;
		
		STFString					name;
		uint32						stackSize;
		STFThreadPriority			priority;

		friend void OSSTFThreadEntryCall(void * p);

		void ThreadEntry(void);

	public:
		OSSTFThread(void)
			{
			prioritySet		= false;
			stackSizeSet	= false;
			}

		OSSTFThread(STFBaseThread * bthread, STFString name, uint32 stackSize, STFThreadPriority priority);
		OSSTFThread(STFBaseThread * bthread, STFString name);
		OSSTFThread(STFBaseThread * bthread);

		~OSSTFThread(void);

		STFResult SetThreadStackSize(uint32 stackSize);
		STFResult SetThreadName(STFString name);

		STFResult StartThread(void);
		STFResult StopThread(void);

		STFResult SuspendThread(void);
		STFResult ResumeThread(void);
		
		STFResult TerminateThread(void);

		STFResult Wait(void);
		STFResult WaitImmediate(void);

		STFResult SetThreadPriority(STFThreadPriority pri);

		STFResult SetThreadSignal(void);
		STFResult ResetThreadSignal(void);
		STFResult WaitThreadSignal(void);

		/// Retrieve the name of the thread
		STFString GetName(void) const;
	};

#endif //OSSTFTHREAD_H
