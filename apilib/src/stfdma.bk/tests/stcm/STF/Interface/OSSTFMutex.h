///
/// @file STF\Interface\OSAL\OS20_21\OSSTFMutex.h
///
/// @brief OS-dependent mutex-methods 
///
/// @par OWNER: 
/// STF-Team
/// @author Christian Johner & Stephan Bergmann    
///
/// @par SCOPE:
///	INTERNAL Header File
///
/// @date       2003-12-11 copy content (from original STFSynchronisation.h)
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#ifndef OSSTFMutex_H
#define OSSTFMutex_H

// Classes
class OSSTFMutex
	{
	private:
		semaphore_t*	sema;
		semaphore_t*	lock;
		task_t	*		ownerTask;
		int				count;
		bool				shared;
	public:
		OSSTFMutex(void)
			{
			sema = semaphore_create_fifo (1);
			ownerTask = NULL;
			count = 0;
			}

		OSSTFMutex(const char* name)
			{
			sema = semaphore_create_fifo (1);
			ownerTask = NULL;
			count = 0;
			}

		~OSSTFMutex(void)
			{
			semaphore_delete(sema);
			}

		STFResult Enter(void)
			{
			// Get the Id of the current task
			task_t *id = task_id();

			// Check whether we already own the mutex
			if (id == ownerTask)
				{
				// We already own the semaphore. Proceed.
				count++;
				}
			else
				{
				// Have to wait for ownership.
				semaphore_wait(sema);
				count = 1;
				ownerTask = id;
				}
			STFRES_RAISE_OK;
			}

		STFResult Leave(void)
			{
			// Check if Leave() is called although no one owns the mutex
			assert(count > 0);

			if (--count == 0)
				{
				ownerTask = NULL;
				semaphore_signal (sema);
				}
			STFRES_RAISE_OK;
			}
	};		

typedef OSSTFMutex OSSTFLocalMutex;

class OSSTFSharedMutex
	{
	protected:
		semaphore_t*	lock;
		semaphore_t*	wlock;
		task_t*			owner;
		bool				shared;
		int32				count;

	public:
		OSSTFSharedMutex(void)
			{
			owner = NULL;
			shared = false;
			count = 0;
			// Init semaphore
			lock = semaphore_create_priority(1);
			wlock = semaphore_create_priority(1);
			}

		~OSSTFSharedMutex(void)
			{
			semaphore_delete(lock);
			semaphore_delete(wlock);
			}

		STFResult Enter(bool exclusive)
			{
			task_t* me = ::task_id();

			if (me == owner)
				{
				++count;
				}
			else if (exclusive)
				{
				semaphore_wait(wlock);
				count = 1;
				owner = me;
				}
			else
				{
				semaphore_wait(lock);

				if (!shared)
					{
					semaphore_wait(wlock);
					count = 1;
					shared = true;
					}
				else
					++count;

				semaphore_signal(lock);
				}

			STFRES_RAISE_OK;
			}

		STFResult Leave(void)
			{
			if (shared)
				{
				semaphore_wait(lock);
				if (--count == 0)
					{
					shared = false;
					semaphore_signal(wlock);
					}
				semaphore_signal(lock);
				}
			else
				{
				if (--count == 0)
					{
					owner = NULL;
					semaphore_signal(wlock);
					}
				}

			STFRES_RAISE_OK;
			}
	};

#endif	// OSSTFMUTEX_H
