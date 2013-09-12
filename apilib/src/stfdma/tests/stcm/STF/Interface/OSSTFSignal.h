///
/// @file STF\Interface\OSAL\OS20\OSSTFSignal.h
///
/// @brief OS20-dependent signal methods
///
/// @par OWNER: 
/// STF-Team
/// @author Stephan Bergmann & Christian Johner   
///
/// @par SCOPE:
///	EXTERNAL Header File
///
/// @date       2003-12-11 create file (copy content from original STFSynchronisation.h)
/// @date       2004-04-05 [Sriram Rao, Carson Zirkle] Replaced message queue based implementation of OSSTFSignal with a semaphore based one.
/// @date       2004-04-06 [Sriram Rao] Fixed bug in OSSTFTimeoutSignal::WaitTimeout and changed implementation of OSSTFArmedSignal. Changed comments.
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///



#ifndef OSSTFSIGNAL_H
#define OSSTFSIGNAL_H

// Includes
#include <message.h>
#include <ostime.h>
#include <time.h>

#include "STF/Interface/STFMutex.h"
#include "STF/Interface/Types/STFTime.h"
#include "STF/Interface/STFDebug.h"
#include "OSSTFTimer.h"


///
/// @class OSSTFSignal
///
/// @brief OS20-dependent signal
///
/// A thread can set a signal or wait until an other thread has set a signal.
/// There is always maximum one signal set even if diffrent threads have set the signal
/// multiple times. 

class OSSTFSignal
	{
	protected:
		semaphore_t			*	signal;		///< semaphore being used as a signal

	public:
		OSSTFSignal(void)
			{
			// create a timeout semaphore with initial count 0
			signal = semaphore_create_fifo_timeout(0);
			assert(signal != NULL);
			}

		~OSSTFSignal(void)
			{
			semaphore_delete(signal);
			}

		/// This method should be called to set a signal. It always sets the semaphore, so the semaphore count can become
		/// greater than one. The task that first waits or is waiting on this semaphore makes sure the semaphore count is reset to 0.
		STFResult SetSignal(void)
			{
			semaphore_signal(signal);
			STFRES_RAISE_OK;
			}

		/// Resets the signal by forcing the semaphore count to 0.
		STFResult ResetSignal(void)
			{
			signal->semaphore_count = 0;
			STFRES_RAISE_OK;
			}

		/// WaitSignal() blocks the calling thread until a signal is set or if there was a signal already set it 'takes' it.
		/// It also forces the semaphore_count to 0 to unset the signal.
		STFResult WaitSignal(void)
			{
			semaphore_wait(signal);
			signal->semaphore_count = 0;
			STFRES_RAISE_OK;
			}

		/// WaitImmediateSignal() does do a check if there was signal set. If it is not set, STFRES_TIMEOUT is returned. If
		/// the signal was set, it takes it and sets semaphore_count to 0 to unset the signal.
		STFResult WaitImmediateSignal(void)
			{
			if (semaphore_wait_timeout(signal, TIMEOUT_IMMEDIATE) != 0)
				STFRES_RAISE(STFRES_TIMEOUT);

			signal->semaphore_count = 0;
			STFRES_RAISE_OK;
			}
	};


///
/// @class OSSTFTimeoutSignal
///
/// @brief OS20-dependent signal with timeout capability

class OSSTFTimeoutSignal : public OSSTFSignal
	{
	public:
		/// WaitTimeoutSignal() waits on the semaphore with a timeout. If it is not set, STFRES_TIMEOUT is returned. If
		/// the signal was set, it takes it and sets semaphore_count to 0 to unset the signal.
		STFResult WaitTimeoutSignal(STFHiPrec32BitDuration duration)	
			{
			clock_t time;

#if DEBUG_CHECK_SYSTEM_TIMER_SPEED
			//BREAKPOINT;  // WILL CAUSE st20cc to crashs as of 23-Feb-04
			int counter = 0;
			clock_t timeStart = time_now();
			uint32 timeStartMS = timeStart / OS20_HighResTicksPerMillisec;

			while (1)
				{
				counter++;
				clock_t timeNext = time_plus(timeStart, (OS20_HighResTicksPerMillisec * 1000) * counter);
				task_delay_until(timeNext);
				clock_t timeNow = time_now();

				uint32 timeDeltaMS = (timeNow / OS20_HighResTicksPerMillisec) - timeStartMS;

				DP("cnt %d ts %d tn %d dt %d\n",
					counter, timeStart, timeNow, timeDeltaMS);
				}
#endif

			time = time_plus(time_now(), OS20_HighResTicksPerMillisec * duration.Get32BitDuration(STFTU_MILLISECS));
			if (semaphore_wait_timeout(signal, &time) != 0)
				STFRES_RAISE(STFRES_TIMEOUT);

			signal->semaphore_count = 0;
			STFRES_RAISE_OK;
			}
	};


///
/// @class OSSTFArmedSignal
///
/// @brief OS20-dependent signal with a counter
///
/// A thread can set a signal or wait until an other thread has set a signal. The signal must be set a certain number of times before
/// it gets armed, i.e., a wait on it will succeed in getting the signal.
///
/// Most functions work like in the OSSTFSignal class, but there is one additional method that allows to set a signal multiple times. 

class OSSTFArmedSignal
	{
	protected:
		OSSTFSignal				event;
		OSSTFInterlockedInt	counter;

	public:
		OSSTFArmedSignal(void)
			{
			counter = 0;
			}

		~OSSTFArmedSignal(void)
			{
			}

		/// The additional number of times a signal must be set for it to become Armed.
		STFResult ArmSignal(int n)
			{
			counter += n;
			STFRES_RAISE_OK;
			}

		STFResult SetSignal(int n)
			{
			if ((counter - n) <= 0)
				{
				counter = 0;
				event.SetSignal();
				}
			STFRES_RAISE_OK;
			}

		STFResult ResetSignal(void)
			{
			event.ResetSignal();
			STFRES_RAISE_OK;
			}

		STFResult WaitSignal(void)
			{
			event.WaitSignal();
			STFRES_RAISE_OK;
			}

		STFResult WaitImmediateSignal(void)
			{
			STFRES_RAISE(event.WaitImmediateSignal());
			}
	};


#endif // OSSTFSIGNAL_H
