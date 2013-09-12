// FILE:			IVDRUnit.h
//
// AUTHOR:		Ulrich Sigmund
// COPYRIGHT:	(c) 2002 STMicroelectronics.  All Rights Reserved.
// CREATED:		04.12.2002
//
// PURPOSE:		Definition of public Device, Physical and Virtual Unit Interfaces
//
// SCOPE:		PUBLIC Header File

/*! \file
	 \brief Definition of public Device, Physical and Virtual Unit Interfaces
*/

#ifndef IVDRUNIT_H
#define IVDRUNIT_H

#include "IVDRGlobalUnitIDs.h"
#include "IVDRTagUnit.h"
#include "STF/Interface/Types/STFTime.h"

/** \defgroup ActivationFlags Public flags for ActivateAndLock and UnlockAndLock */
/*@{*/

//! The mask for all priorities
/*!
This flag can be used to mask the priority value of the flag set passed
to the ActivateAndLock or UnlockAndLock methods.
*/
#define VDRUALF_PRIORITY_MASK					0x0000000f

//! Priority for idle tasks
/*!
The requesting unit just wants to perform some idle processing, this
would be the case for e.g. a background picture.  An idle unit can wait
in the background to do some processing when a unit becomes completely
unused.  It will then gain the physical unit, perform some idle work
and unlock again.	 A request with an idle priority will not be
satisfied unless there is no request with a higher priority.
*/
#define VDRUALF_IDLE_PRIORITY					0x00000000

//! Priority for background tasks
/*!
The requesting thread wants to perform some background processing, e.g.
render some user interface changes etc. that are not constrained by
realtime issues.  A background level request will not be satisfied in
time, but might be given priority over a streaming request, if they fit
into the timeout sequence.  A background request should therefore contain
a duration value.
*/
#define VDRUALF_BACKGROUND_PRIORITY			0x00000001

//! Priority for streaming tasks
/*!
The requesting thread performs streaming and needs the unit in time,
otherwise it will have a disruption in the presentation.  Streaming requests
will have priority over background requests if the times collide.  The
timeout value represents the last safe time when the activation needs to
happen.
*/
#define VDRUALF_STREAMING_PRIORITY			0x00000002

//! Priority for foreground tasks
/*!
The requesting thread performs a user operation that should complete ASAP.
This might block streaming requests from being served in time.  A typical
case would be an abort operation that needs to come in before the streaming
operations.
*/
#define VDRUALF_FOREGROUND_PRIORITY			0x00000003

//! Priority for realtime tasks
/*!
A request at realtime priority will be satisfied as soon as the unit becomes
unlocked.  
*/
#define VDRUALF_REALTIME_PRIORITY			0x00000004

//! Flag signaling that the time parameter is valid
/*!
A request at background *may*, and a request at streaming or foreground priority 
*must* be accompanied by a time value.  This time value states a worst case time, 
when the request should be satisfied.
*/
#define VDRUALF_TIME_VALID						0x00000010

//! Flag signaling that the duration parameter is valid
/*!
A request at background should, a request at streaming may be accompanied
by a duration value.  This duration describes the expected duration of the lock
period.  A background request will only be satisfied if its duration fits
into the gap to the next streaming request timeout.
*/
#define VDRUALF_DURATION_VALID				0x00000020

//! Flag signaling that the call should wait for the activate to complete
/*!
If an ActivateAndLock can not be satistfied or if a unit is lost during a
call of UnlockAndLock, the call will return with GNR_OBJECT_IN_USE.  If the
wait flag is specified, the call will wait for the units to become free and
then retry the activation and lock.
*/
#define VDRUALF_WAIT								0x00000040
/*@}*/



//! Device Unit Public Interface ID
static const VDRIID VDRIID_VDR_DEVICE_UNIT = 0x00000008;


//! Device Unit Public Interface
class IVDRDeviceUnit : virtual public IVDRTagUnit
	{
	public:
		//! Get Global Unit ID
		/*! This is actually the Global Unit ID of the underlying Physical Unit,
			 which is sufficiently identifying the Virtual Unit counterpart.
		*/
		virtual VDRUID GetUnitID(void) = 0;
	};



//! Physical Unit Public Interface ID
static const VDRIID VDRIID_VDR_PHYSICAL_UNIT = 0x00000009;


//! Physical Unit Public Interface
class IVDRPhysicalUnit : virtual public IVDRDeviceUnit
	{
	public:
		// Pretty empty... for now.
	};



//! Virtual Unit Public Interface ID
static const VDRIID VDRIID_VDR_VIRTUAL_UNIT = 0x0000000a;


//! Virtual Unit Public Interface
class IVDRVirtualUnit : virtual public IVDRDeviceUnit
	{
	public:
		//! Activate and lock this unit
		/*!
		In order to do usefull work with a unit, it needs to be active and locked.  This call
		will (if it succeeds) activate this virtual unit (and all of its subunits) and increase
		the lock count.  If the unit is already active, only the lock count is incremented.
		If the activation fails, this call will return a GNR_OBJECT_IN_USE error.  If the VDRUALF_WAIT
		flag is set in the flags parameter, it will wait for the units to become available.

		\param flags Any of the \ref ActivationFlags
		\param time The time, when the request should be completed
		\param duration The expected duration for the lock
		*/
		virtual STFResult ActivateAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration) = 0;

		//! Unlock this unit
		/*!
		This call decrements the lock counter.  If the lock counter is zero, the unit can be 
		preempted by another unit with a higher priority.  This preemption will not happen during
		the Unlock call, but can happen immediately after it.  If a unit is unlocked, it may
		be lost anytime, so no usefull work can be done with it.  The unlocked state is merely used
		to prevent unneccessary preemption in the case of no other virtual unit interfering with
		this one.  
		*/
		virtual STFResult Unlock(void) = 0;

		//! Unlock and lock this unit
		/*!
		This call decrements the lock counter of this and all subunits.  It then immediately
		attempts to lock the units again.  If no unit with a higher priority is waiting, this
		lock will succeed.  Otherwise the unit will be lost, and the call will return with a
		GNR_OBJECT_IN_USE error.  If the VDRUALF_WAIT flag is set in the flags parameter, it
		will wait for the units to become available again.

		An application that holds a unit for a long or even infinite time, should Unlock and Lock
		the unit from time to time to enable other applications to preempt it.  

		The main difference between a call to UnlockAndLock and a call sequence of Unlock and
		ActivateAndLock is, that in the first case, the units will not be lost if a lower
		priority request is pending.

		\param flags Any of the \ref ActivationFlags
		\param time The time, when the request should be completed
		\param duration The expected duration for the lock
		*/
		virtual STFResult UnlockAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration) = 0;

		//! Passivate this unit
		/*!
		When a unit is no longer needed, it should be passivated.  Passivation also clears
		all locks pending from this unit.  A passive unit will not become active unless
		ActivateAndLock is called again.  Its state is comparable to the state after unit
		creation.

		When the unit is needed again, it can be reactivated by ActivateAndLock.
		*/
		virtual STFResult Passivate(void) = 0;
	};

#endif	// #ifndef IVDRTAGUNITS_H
