//
// FILE:			VDR/Source/Unit/IVirtualUnit.h
//
// AUTHOR:		U. Sigmund, S. Herr
// OWNER:		VDR Architecture Team
// COPYRIGHT:	(C) 1995-2002 STMicroelectronics.  All Rights Reserved.
// CREATED:		26.11.2002
//
// PURPOSE:		Internal Virtual Unit interface
//
// SCOPE:		INTERNAL Header File


#ifndef IVIRTUALUNIT_H
#define IVIRTUALUNIT_H

#include "ITagUnit.h"
#include "VDR/Interface/Unit/IVDRUnit.h"

// Forward declarations
class IPhysicalUnit;


///////////////////////////////////////////////////////////////////////////////
// Activation Flags to contol ActivateAndLock and UnlockAndLock functions
///////////////////////////////////////////////////////////////////////////////


/** \defgroup PrivateActivationFlags Private flags for ActivateAndLock and UnlockAndLock */
/*@{*/

//! All flags used in internal preemption calls
#define VDRUALF_PREEMPT_MASK					0x7fff0000

//! Preemption not in progress
#define VDRUALF_PREEMPT_NONE					0x00000000

//! Check if an activation and lock can be performed
/*!
This flag is valid in a call to ActivateAndLock of physical units.  It checks, if
the given virtual unit can actually be activated.  If this is not the case, the call
will fail with GNR_OBJECT_IN_USE.  This flag can be combined with the VDRUALF_PREEMPT_REGISTER
flag.  If the register flag is present, the call will not fail, but register the
virtual unit for later activation.
*/
#define VDRUALF_PREEMPT_CHECK					0x00010000

//! Stop the previous unit
/*!
The first phase of preemption is to stop the current virtual unit.  This flag is
used in ActivateAndLock of physical units, but also in the PreemptUnit
call of virtual units.
*/
#define VDRUALF_PREEMPT_STOP_PREVIOUS		0x00020000

//! Change the parameters for the new unit
/*!
The second phase of preemption is to change the parameters in the physical unit
to the values present in the newly active virtual unit.  This flag is used
in ActivateAndLock of physical units, but also in the PreemptUnit call
of virtual units.
*/
#define VDRUALF_PREEMPT_CHANGE				0x00040000

//! Start the new unit
/*!
The third phase of preemption is to start processing with the newly active
virtual unit.  This flag is used in ActivateAndLock of physical units, but also
in the PreemptUnit call of virtual units.
*/
#define VDRUALF_PREEMPT_START_NEW			0x00080000

//! Recover by restarting the previous unit
/*!
This is the final recover phase if preemption fails.  It is supposed to restart
the previously active virtual unit.  It is used in ActivateAndLock of physical
units, but also in the PreemptUnit call of virtual units.
*/
#define VDRUALF_PREEMPT_RESTART_PREVIOUS	0x00100000

//! Recover by restoring the previous values
/*!
This recovery phase corresponds to the change phase during normal preemption.
It is supposed to restore the values from the previous virtual unit into the
physical unit.  It is used in ActivateAndLock of phyiscal units, but also
in the PreemptUnit call of virtual units.
*/
#define VDRUALF_PREEMPT_RESTORE				0x00200000

//! Recover by stopping the new unit
/*!
This recovery phase corresponds to the start new phase during normal preemption.
It is supposed to stop the new unit from working.  This would normaly not be
the case for a single virtual unit activation, but if more than one unit is
part of the activation, a later one might fail, causing the whole preemption
to be undone.  This flag is used in ActivateAndLock of physical units, but
also in the PreemptUnit call of virtual units.
*/
#define VDRUALF_PREEMPT_STOP_NEW				0x00400000

//! The request should be registered if it fails
/*!
This flag should be combined either with VDRUALF_PREEMPT_CHECK or with
VDRUALF_PREEMPT_UNLOCK.  In the case that the operation would fail due to a
busy physical unit, it will instead register the virtual unit for notification
and return with GNR_OPERATION_DEFERRED.  This can be used to implement
waiting on a physical unit.  

For each registration, a call to NotifyActivationRegistered
of the physical unit will be done.  If the registration succeeds or is canceled
a call to NotifyActivationRetry will be done.  This way, a virtual unit can
wait for multiple physical units to become available.
*/
#define VDRUALF_PREEMPT_REGISTER				0x00800000

//! The activation is complete, the lock can be done
#define VDRUALF_PREEMPT_COMPLETE				0x01000000

//! The activation failed
#define VDRUALF_PREEMPT_FAILED				0x02000000

//! The unit is being passivated
#define VDRUALF_PREEMPT_PASSIVATED			0x04000000

//! Unlock the unit
#define VDRUALF_PREEMPT_UNLOCK				0x08000000

//! Lock the unit again
#define VDRUALF_PREEMPT_LOCK					0x10000000

//! The virtual unit is registered for activation
#define VDRUALF_PREEMPT_REGISTERED			0x80000000

//! Combination of all recover flags
#define VDRUALF_PREEMPT_RECOVER				(VDRUALF_PREEMPT_STOP_NEW | VDRUALF_PREEMPT_RESTORE | VDRUALF_PREEMPT_RESTART_PREVIOUS | VDRUALF_PREEMPT_FAILED )

//! Combination of all activation flags
#define VDRUALF_PREEMPT_DIRECT				(VDRUALF_PREEMPT_CHECK | VDRUALF_PREEMPT_STOP_PREVIOUS | VDRUALF_PREEMPT_CHANGE | VDRUALF_PREEMPT_START_NEW  | VDRUALF_PREEMPT_COMPLETE )

//! Combination of all unlock and lock flags
#define VDRUALF_PREEMPT_UNLOCKLOCK			(VDRUALF_PREEMPT_UNLOCK | VDRUALF_PREEMPT_LOCK)

/*@}*/



///////////////////////////////////////////////////////////////////////////////
// Physical Unit Sequence Interface
///////////////////////////////////////////////////////////////////////////////

//! Internal Physical Unit Sequence interface
class IPhysicalUnitSequence
	{
	public:
		virtual ~IPhysicalUnitSequence(void) {};

		virtual STFResult InsertUnit(IPhysicalUnit * unit) = 0;
		virtual STFResult LockActivationMutexes(bool exclusive) = 0;
		virtual STFResult UnlockActivationMutexes(void) = 0;
	};


///////////////////////////////////////////////////////////////////////////////
// Virtual Unit Interface
///////////////////////////////////////////////////////////////////////////////

//! Internal Virtual Unit Interface ID
static const VDRIID VDRIID_VIRTUAL_UNIT = 0x80000000;


//! Internal Virtual Unit Interface
class IVirtualUnit : virtual public IVDRVirtualUnit,
							virtual public ITagUnit

	{
	public:
		//! Connects the Virtual Unit with its parent and the root unit
		/*!
			Part of the Unit Set creation process: the new virtual unit
			must know its parent and unit so that it can notify them
			about activation events.
			If this unit is a parent unit, \param parent set to NULL.
			If this unit is a root unit, \param root is set to NULL.
			(The unit can be parent and root at the same time - then
			 it is the anchor unit of the Virtual Unit Set (typically
			 a Virtual Board Unit)).
		*/
		virtual STFResult Connect(IVirtualUnit * parent, IVirtualUnit * root) = 0;

		//! Initialise the Virtual Unit after its creation
		/*!
			This function is overridden to do specific initialisations after
			it has been created and connected.
		*/
		virtual STFResult Initialize(void) = 0;

		virtual STFResult LockActivationMutexes(bool exclusive = true) = 0;
		virtual STFResult UnlockActivationMutexes(void) = 0;

		//
		// The system time parameter is used to find potential background priority
		// activation requests, that might be satisfied in time.
		//
		virtual STFResult InternalActivateAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime) = 0;
		virtual STFResult InternalUnlock(const STFHiPrec64BitTime & systemTime) = 0;
		virtual STFResult InternalPassivate(const STFHiPrec64BitTime & systemTime) = 0;
		virtual STFResult InternalUnlockAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime) = 0;

		virtual STFResult NotifyActivationRegistered(void) = 0;
		virtual STFResult NotifyActivationRetry(void) = 0;
		virtual STFResult WaitUnlockedForActivation(void) = 0;

		virtual STFResult PreemptUnit(uint32 flags) = 0;
		virtual STFResult BuildPhysicalUnitSequence(IPhysicalUnitSequence * sequence) = 0;

		//! Use this function to determine current active unit (see also IPhysicalUnit.h)
		virtual STFResult IsUnitCurrent(void) = 0; 
	};


#endif	// #ifndef IVIRTUALUNIT_H
