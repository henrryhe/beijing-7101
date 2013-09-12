//
// FILE:			VDR/Source/Unit/VirtualUnit.h
//
// AUTHOR:		U. Sigmund, S. Herr
// OWNER:		VDR Architecture Team
// COPYRIGHT:	(C) 1995-2002 STMicroelectronics.  All Rights Reserved.
// CREATED:		06.12.2002
//
// PURPOSE:		Generic Virtual Unit
//
// SCOPE:		INTERNAL Header File

#ifndef VIRTUALUNITS_H
#define VIRTUALUNITS_H

#include "IVirtualUnit.h"
#include "IPhysicalUnit.h"
#include "VDR/Source/Base/VDRBase.h"
#include "STF/Interface/STFSynchronisation.h"


//! Implements the generic bits of a Virtual Unit
class	VirtualUnit : virtual public VDRBase,
						  virtual public IVirtualUnit
	{
	protected:
		IPhysicalUnit		*	physical;			//! The related Physical Unit
		STFArmedSignal		*	activationGrantSignal;
		IVirtualUnit		*	parent;				//! Direct Parent Unit
		IVirtualUnit		*	root;					//! Root Unit (anchor unit)
		STFInterlockedInt		configureCounter;	//! Counts nested Tag configuration calls

	public:
		VirtualUnit(IPhysicalUnit * physical);
		virtual ~VirtualUnit(void);

		//
		// Partial IVDRBase implementation
		//
		virtual STFResult QueryInterface(VDRIID iid, void * & ifp);

		//
		// IVDRDeviceUnit implementation
		//
		virtual VDRUID GetUnitID(void);

		//
		// IVDRVirtualUnit implementation
		//
		virtual STFResult ActivateAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration);
		virtual STFResult Unlock(void);
		virtual STFResult UnlockAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration);
		virtual STFResult Passivate(void);

		//
		// IVirtualUnit implementation
		//
		//! Connect unit to its parent and root units
		virtual STFResult Connect(IVirtualUnit * parent, IVirtualUnit * root);

		//! Initialise unit after creation and connection
		/*!
			This currently does nothing - but it should be overridden
			if a class derived from VirtualUnit needs special initialisation.
		*/
		virtual STFResult Initialize(void);

		virtual STFResult LockActivationMutexes(bool exclusive = true);
		virtual STFResult UnlockActivationMutexes(void);

		virtual STFResult InternalActivateAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime);
		virtual STFResult InternalUnlock(const STFHiPrec64BitTime & systemTime);
		virtual STFResult InternalPassivate(const STFHiPrec64BitTime & systemTime);
		virtual STFResult InternalUnlockAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime);

		virtual STFResult NotifyActivationRegistered(void);
		virtual STFResult NotifyActivationRetry(void);
		virtual STFResult WaitUnlockedForActivation(void);
		virtual STFResult PreemptUnit(uint32 flags);
		virtual STFResult BuildPhysicalUnitSequence(IPhysicalUnitSequence * sequence);
		virtual STFResult IsUnitCurrent(void);

		//
		// IVDRTagUnit implementation
		//
		virtual STFResult BeginConfigure(void);
		virtual STFResult ConfigureTags(TAG * tags);
		virtual STFResult Configure(const TAGList & tagList);
		virtual STFResult CompleteConfigure(void);

		//
		// Partial ITagUnit implementation
		//
		virtual STFResult GetTagIDs(VDRTID * & ids);
		virtual STFResult InternalBeginConfigure(void);
		virtual STFResult InternalAbortConfigure(void);
		virtual STFResult InternalCompleteConfigure(void);
		virtual STFResult InternalConfigureTags(TAG * tags);
		virtual STFResult InternalUpdate(void);
		virtual STFResult InternalEndConfigure(void);
	};


#endif
