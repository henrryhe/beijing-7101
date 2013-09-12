///
/// @file       Dvd_driver/STF/Interface\STFTimer.h
///
/// @brief 
///
/// @par OWNER: 
/// @author     Ulrich Mohr
///
/// @par SCOPE:
///
/// @date       2004-04-22 
///
/// &copy; 2004 STMicroelectronics Design and Applications. All Rights Reserved.
///

#ifndef STFTIMER_H
#define STFTIMER_H

#include "STF/Interface/Types/STFTime.h"
#include "STF/Interface/Types/STFMessage.h"
#include "OSSTFTimer.h"

//! Interface of a timer service
/*!
Timers can be used to retrieve their current value, to wait an amount of time
or to schedule an event, that is later sent to a message sink.  
*/
class STFTimer	: public STFThread
	{	
	private:
		OSSTFTimer ost;
		
		STFTimeoutSignal	signal;
		STFMutex				mutex;
		uint32 count;

	public:
		STFTimer(void);

		virtual ~STFTimer();

		
		//
		// Event scheduling
		//

		struct PendingEvents
			{
			STFHiPrec64BitTime   	time;
			STFMessageSink		*	   sink;
			STFMessage				   message;
			uint32					   id;
			bool                    recurrent;
			STFHiPrec64BitDuration  dueCycleDuration;
			} pendingEvents[16];


		void ThreadEntry(void);

		STFResult NotifyThreadTermination(void);
		
		
		//! Get current high precision time
		/*!
		Retrieves the current high precision timer.  This time stamp is valid
		for 30 years, but only if the system is up.  This call may have a significantly
		lower performance than the low precision time function.
		\param time Returns the current high precision time
		*/
		void GetTime(STFHiPrec64BitTime & time) { ost.GetTime(time); }

		//! Wait an amount of time
		/*!
		Wait for an amount of time to complete.  This function blocks the calling thread
		until a given amount of time has elapsed.  It is not interrupt safe.  It is not
		guaranteed that the calling thread will be resumed immediately after the duration
		has elapse, due to scheduler details.  For interrupt safe waiting, use WaitIdleTime.
		\param duration The duration to wait
		*/
		STFResult WaitDuration(const STFLoPrec32BitDuration & duration) { STFRES_RAISE(ost.WaitDuration(duration)); }

		//! Wait for a point in time
		/*!
		Wait for a point of time to be reached.  This function blocks the calling thread
		until a given point in time is reached.  It is not interrupt safe.  It is not
		guaranteed that the calling thread will be resumed immediately after the duration
		has elapse, due to scheduler details.  For interrupt safe waiting, use WaitIdleTime.
		\param time The point in time to wait for
		*/
		STFResult WaitTime(const STFHiPrec64BitTime & time)			
			{
			STFHiPrec64BitTime	ct;

			GetTime(ct);

			STFRES_RAISE(WaitDuration(time - ct));
			}

		//! Keep the processor busy for a certain duration
		/*!
		Keeps the cpu spinning for a given amount of time.  This call does not block the
		calling thread, it just spins in a loop until the number of microseconds has
		elapsed.  This function is interrupt safe.  Do not use this function to wait for
		longer amounts of time, because it will keep the cpu busy.  If you need to wait
		for a longer amount, use WaitDuration.
		\param micros The number of microseconds to idle
		*/
		STFResult WaitIdleTime(uint32 micros) { STFRES_RAISE(ost.WaitIdleTime(micros));}

		
		STFResult ScheduleEventRelative(const STFHiPrec64BitDuration & offsetToCurrentTime, 
		                                STFMessageSink * sink, 
		                                const STFMessage & message, 
		                                uint32 & eventId)
			{
			STFHiPrec64BitTime currentSystemTime;
			GetTime(currentSystemTime);
			STFRES_RAISE(ScheduleEvent(currentSystemTime + offsetToCurrentTime, sink, message, eventId));
			}
		
		//! Schedule an event
		/*!
		Schedule an event, that is sent to a message sink after a point of time has been
		reached.  The event can be canceled using CancelEvent with the timer id returned
		by this call.	If no slot is free, this function may return with an error.
		\param time Target time for the message
		\param sink Message sink that receives the message
		\param message The message id of the message
		\param timer An id describing the scheduled event
		*/
		STFResult ScheduleEvent(const STFHiPrec64BitTime & time, STFMessageSink * sink, const STFMessage & message, uint32 & timer);

		STFResult ScheduleRecurrentEvent(const STFHiPrec64BitTime & time, const STFHiPrec64BitDuration & dueClycleDuration, STFMessageSink * sink, const STFMessage & message, uint32 & timer);
		
		//! Cancel an event
		/*!
		Cancel an event by its event id.
		*/
		STFResult CancelEvent(uint32 timer);
	};

//! The main system timer
extern STFTimer	*	SystemTimer;

//! Initialization call
/*!
Initializes timer services.  This call has to be made, before any timer specific
functions are called or classes created.
*/
STFResult InitializeSTFTimer(void);


//
// inlines
//



//! Timeout service class.
/*!
	Use the "STFSystemTickTimeout" to implement efficient timeouts without knowing the exact
	timer precision.
	The class will only guarantee that a timeout is not shorter than the desired amount of
	milliseconds. It may be a bit larger. There are two mutual exclusive ways to use this:

		timeout.SetMilliseconds (10);
		while (condition  &&  ! timeout.IsExpired ())
			updateCondition();

	Or:

		timeout.StartTimer();
		while (condition  &&  ! timeout.MillisecondsAreExpired (10))
			updateCondition();
*/

class STFSystemTickTimeout
	{
	private:
		STFHiPrec64BitTime time;

	public:
		void SetMilliseconds (uint32 milliSeconds)
			{
			SystemTimer->GetTime (time);
			time += STFLoPrec32BitDuration (milliSeconds, STFTU_MILLISECS);
			}

		bool IsExpired (void)
			{
			STFHiPrec64BitTime now;
			SystemTimer->GetTime (now);
			return now >= time;
			}

	public:
		void StartTimer (void)
			{
			SystemTimer->GetTime (time);
			}

		bool MillisecondsAreExpired (uint32 milliSeconds)
			{
			STFHiPrec64BitTime now;
			STFLoPrec32BitDuration duration;

			SystemTimer->GetTime (now);
			duration = now - time;
			return (uint32)(duration.Get32BitDuration(STFTU_MILLISECS)) >= milliSeconds;
			}
	};



#endif
