///
/// @file       STF\Interface\STFThread.h
///
/// @brief      Generic Threads
///
/// @par OWNER: STFTeam
///
/// @author     U. Sigmund
///
/// @par SCOPE:
///             EXTERNAL Header File
///
/// @date       2002-12-05 
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#ifndef STFTHREAD_H
#define STFTHREAD_H

#include "STF/Interface/Types/STFResult.h"
#include "STF/Interface/Types/STFString.h"
#include "STF/Interface/STFSynchronisation.h"

// This pragma is microsoft specific, it disables the 'this' used in base member initialier list warning on this platform
#ifdef _WIN32
#pragma warning(disable : 4355)
#endif

/// @name Thread Priority Constants
///
/// The following predefined thread priorities are guaranteed to be mapped
/// on unique thread priority levels of the underlying operating system (*).
/// When using any intermediate levels not mentioned here, be conscious
/// about the actual system-specific mapping!
/// 
/// (*): Current exception is the Win32 platform (to be changed)
//@{

/// Type used to indicate thread priority
typedef uint16 STFThreadPriority;

static const STFThreadPriority STFTP_IDLE				= 0;
static const STFThreadPriority STFTP_1					= 16;
static const STFThreadPriority STFTP_LOWEST			= 32;
static const STFThreadPriority STFTP_3					= 48;
static const STFThreadPriority STFTP_LOWER			= 64;
static const STFThreadPriority STFTP_5					= 80;
static const STFThreadPriority STFTP_BELOW_NORMAL	= 96;
static const STFThreadPriority STFTP_7					= 112;
static const STFThreadPriority STFTP_NORMAL			= 128;
static const STFThreadPriority STFTP_9					= 144;
static const STFThreadPriority STFTP_ABOVE_NORMAL	= 160;
static const STFThreadPriority STFTP_11				= 176;
static const STFThreadPriority STFTP_HIGHER			= 192;
static const STFThreadPriority STFTP_13				= 208;
static const STFThreadPriority STFTP_HIGHEST			= 224;
static const STFThreadPriority STFTP_15				= 240;
static const STFThreadPriority STFTP_CRITICAL		= 255;
//@}

///
/// @class STFBaseThread
///
class STFBaseThread
	{
	friend class OSSTFThread;
	protected:

		/// This value indicates that StopThread has been called.
		/// ThreadEntry routines should check the state of this variable to determine
		/// if they are to exit.
		volatile	bool terminate;																	

		STFBaseThread(void)
			{
			terminate = false;
			}

		virtual void ThreadEntry(void) = 0;
		virtual STFResult NotifyThreadTermination(void) = 0;
	};

#include "OSSTFThread.h"

///
/// @class STFThread
/// Abstraction class for OS-specific threads.
///
class STFThread : public STFBaseThread, public STFWaitable
	{
	friend class STFRootThread;
	friend STFResult InitializeSTFThreads(void);

	private:
		/// Default constructor
		/// This constructor is used to construct the root thread
		STFThread(void);

	protected:
		OSSTFThread		ost;
		STFSignal	*	event;

	public:
		/// @brief		Constructor specifying a name only
		/// @param		name		[in] STFString name identifying this thread
		///				
		/// @note		StartThread will fail if the thread is created using this
		///				constructor, but SetPriority and SetStackSize are not
		///				called before. The actual thread is created when StartThread()
		///				is called.
		STFThread(STFString name);


		/// @brief		Standard constructor
		/// @param		name			[in] STFString name identifying this thread
		/// @param		stackSize	[in] Stack size of the thread
		/// @param		priority		[in] Priority of the thread
		///
		///				Initialize new thread with given name, stack size and priority. The
		///				thread is created when StartThread() is called.
		STFThread(STFString name, uint32 stackSize, STFThreadPriority priority);

		virtual ~STFThread(void);

		/// @brief		Begin thread execution
		/// @retval		STFRES_OK: Successs
		/// @retval		STFRES_OPERATION_PROHIBITED: Priority and/or stack size have not yet 
		///													  configured;
		///
		///				This method will create a thread and begin execution.
		/// @note		This function will fail if stack size or priority have not been
		///				set.
		STFResult StartThread(void);

		/// @brief		Indicates to the running thread that it is to stop
		/// @retval		STFRES_OK: Success
		/// @retval		STFRES_OBJECT_NOT_FOUND: thread was not created
		///
		///				This method will invoke the NotifyThreadTermination function and
		///				also set the @a terminate member to true.  It is responsibility
		///				of the thread code to monitor this flag and quit when it is true.
		STFResult StopThread(void);

		/// @brief		Suspends a thread
		/// @retval		STFRES_OK: Success
		/// @retval		STFRES_OBJECT_NOT_FOUND: thread was not created
		///
		///				This function stops a thread execution where it is.  
		///				When thsi function is called, the suspend count of the the
		///				thread is incremented. Execution may be resumed by calling
		///				ResumeThread().
		STFResult SuspendThread(void);

		/// @brief		Resumes execution of a suspended thread
		/// @retval		STFRES_OK: Success
		/// @retval		STFRES_OBJECT_NOT_FOUND: thread was not yet created
		///
		///				This method will decrement the suspsend count
		///				of the thread.  If the suspend count is 0, execution of the 
		///				thread is resumed from the point where it was suspended.  
		STFResult ResumeThread(void);
		
		/// @brief		Immediately termintes the thread
		/// @retval		STFRES_OK: Success
		/// @retval		STFRES_OBJECT_NOT_FOUND: thread was not yet created
		///
		///				This function immediately stops execution of the thread
		///				and destroys the internal task.  The NotifyThreadTermination
		///				function is not invoked, but @a terminate is set to true.
		STFResult TerminateThread(void);

		/// @brief		Waits for a task to exit
		/// @retval		STFRES_OK: Success
		/// @retval		STFRES_OBJECT_NOT_FOUND: thread was not yet created
		///
		///				This method will wait until the thread exits.  When it has,
		///				the task will be terminated.
		STFResult Wait(void);

		/// @brief		Checks if a task is still running
		/// @retval		STFRES_OK: Thread is not running/ has exited
		/// @retval		STFRES_OBJECT_NOT_FOUND: thread was not yet created
		/// @retval		STFRES_TIMEOUT: Thread is still active
		///			
		///				This method waits with a 0ms timeout for the thread to exit.
		///				This provides a mechanism to determine if the task is still
		///				running.  If it is, the timeout will expire and this method
		///				will return STFRES_TIMEOUT.  Otherwise, the task has finished
		///				and this method will terminate the thread before returning STFRES_OK.
		STFResult WaitImmediate(void);

		/// @brief		Reconfigure thread's stack size
		/// @param		stackSize	[in] New stack size for the thread.
		/// @retval		STFRES_OK: Success
		///
		///				This method reconfigures the thread's stack size.
		///				This must only be called before StartThread or after StopThread.
		STFResult SetThreadStackSize(uint32 stackSize);

		/// @brief		Reconfigure thread's name
		/// @param		name			[in] STFString name of thread.
		/// @retval		STFRES_OK: Success
		///
		///				This method reconfigures the thread's name.
		///				This must only be called before StartThread or after StopThread.
		STFResult SetThreadName(STFString name);

		/// @brief		Reconfigure thread's priority
		/// @param		pri			[in] New priority for the thread.
		/// @retval		STFRES_OK: Success
		///
		///				This method reconfigures the priority of the thread.
		///				This must only be called before StartThread or after StopThread.
		STFResult SetThreadPriority(STFThreadPriority pri);

		/// @brief		Triggers the STFSignal event for this thread/class.
		/// @retval		STFRES_OK: Success
		///
		///				This method increments the signal object's count.  While this
		///				value is greater than zero, the signal is considered active.
		STFResult SetThreadSignal(void);

		/// @brief		Resets the thread's signal to an off state.
		/// @retval		STFRES_OK: Success
		///
		///				This method will set the signal object's count to 0.
		STFResult ResetThreadSignal(void);

		/// @brief		Waits for the thread's signal to be triggered
		/// @retval		STFRES_OK: Success
		///
		///				This method waits until the signal object's count
		///				is non-zero (i.e. the signal is triggered).  When this
		///				happens this function will decrement the signal's count 
		///				and return.			
		STFResult WaitThreadSignal(void);

		/// Retrieve the name of this thread
		STFString GetName(void) const;
	};

STFResult InitializeSTFThreads(void);

STFResult GetCurrentSTFThread(STFThread * & thread);


//-------------------------- INLINE --------------------------------

inline STFThread::STFThread(void)
:	ost(this) 
	{
	this->event = new STFSignal;
	}

inline STFThread::STFThread(STFString name) : ost(this, name)
	{
	this->event = new STFSignal;
	}

inline STFThread::STFThread(STFString name, uint32 stackSize, STFThreadPriority priority)
	: ost(this, name, stackSize, priority) 
	{
	this->event = new STFSignal;
	}

inline STFThread::~STFThread(void) 
	{
	delete event;
	}

inline STFResult STFThread::SetThreadStackSize(uint32 stackSize)
	{
	STFRES_RAISE(ost.SetThreadStackSize(stackSize));
	}

inline STFResult STFThread::SetThreadName(STFString name)
	{
	STFRES_RAISE(ost.SetThreadName(name));
	}

inline STFResult STFThread::StartThread(void)
	{
	STFRES_RAISE(ost.StartThread());
	}

inline STFResult STFThread::StopThread(void)
	{
	STFRES_RAISE(ost.StopThread());
	}
					             
inline STFResult STFThread::SuspendThread(void)
 	{
	STFRES_RAISE(ost.SuspendThread());
	}

inline STFResult STFThread::ResumeThread(void)
	{
	STFRES_RAISE(ost.ResumeThread());
	}
					             
inline STFResult STFThread::TerminateThread(void)
	{
	STFRES_RAISE(ost.TerminateThread());
	}
					             
inline STFResult STFThread::Wait(void)
	{
	STFRES_RAISE(ost.Wait());
	}

inline STFResult STFThread::WaitImmediate(void)
	{
	STFRES_RAISE(ost.WaitImmediate());
	}

					             
inline STFResult STFThread::SetThreadPriority(STFThreadPriority pri)
	{
	STFRES_RAISE(ost.SetThreadPriority(pri));
	}

					             
inline STFResult STFThread::SetThreadSignal(void)
	{
	event->SetSignal();

	STFRES_RAISE_OK;
	}

inline STFResult STFThread::ResetThreadSignal(void)
	{
	event->ResetSignal();

	STFRES_RAISE_OK;
	}

inline STFResult STFThread::WaitThreadSignal(void)
	{
	event->WaitSignal(); 

	STFRES_RAISE_OK;
	}

inline STFString STFThread::GetName(void) const
	{
	return ost.GetName();
	}


// Re-enable warning
#ifdef _WIN32
#pragma warning(default : 4355)
#endif

#endif //STFTHREAD_H
