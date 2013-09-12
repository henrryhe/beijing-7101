//
// FILE:			VDR/Source/Unit/VirtualUnit.cpp
//
// AUTHOR:		U. Sigmund, S. Herr
// OWNER:		VDR Architecture Team
// COPYRIGHT:	(C) 1995-2002 STMicroelectronics.  All Rights Reserved.
// CREATED:		06.12.2002
//
// PURPOSE:		Generic Virtual Unit Implementation
//
// SCOPE:		INTERNAL Implementation File

#include "VirtualUnit.h"
#include "STF/Interface/STFTimer.h"
#include "STF/Interface/STFDebug.h"

//
// IVirtualUnit implementation
//

STFResult VirtualUnit::LockActivationMutexes(bool exclusive)
	{
	return physical->LockActivationMutex(exclusive);
	}


STFResult VirtualUnit::UnlockActivationMutexes(void)
	{
	return physical->UnlockActivationMutex();
	}


STFResult VirtualUnit::InternalActivateAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime)
	{
	return physical->ActivateAndLock(this, flags, time, duration, systemTime);
	}


STFResult VirtualUnit::InternalUnlock(const STFHiPrec64BitTime & systemTime)
	{
	return physical->Unlock(this, systemTime);
	}


STFResult VirtualUnit::InternalPassivate(const STFHiPrec64BitTime & systemTime)
	{
	return physical->Passivate(this, systemTime);
	}


STFResult VirtualUnit::InternalUnlockAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime)
	{
	return physical->UnlockAndLock(this, flags, time, duration, systemTime);
	}


STFResult VirtualUnit::NotifyActivationRegistered(void)
	{
	if (root)
		STFRES_RAISE(root->NotifyActivationRegistered());
	else
		STFRES_RAISE(activationGrantSignal->ArmSignal());
	}


STFResult VirtualUnit::NotifyActivationRetry(void)
	{
	if (root)
		STFRES_RAISE(root->NotifyActivationRetry());
	else
		STFRES_RAISE(activationGrantSignal->SetSignal());
	}


STFResult VirtualUnit::WaitUnlockedForActivation(void)
	{
	STFResult err;

	if (root)
		err = root->WaitUnlockedForActivation();
	else
		{
		if (!STFRES_IS_ERROR(err = UnlockActivationMutexes()))
			{
			if (!STFRES_IS_ERROR(err = activationGrantSignal->Wait()))
				{
				err = LockActivationMutexes();
				}
			}
		}

	STFRES_RAISE(err);
	}


STFResult VirtualUnit::PreemptUnit(uint32 flags)
	{
	STFRES_RAISE_OK;
	}


STFResult VirtualUnit::BuildPhysicalUnitSequence(IPhysicalUnitSequence * sequence)
	{
	return sequence->InsertUnit(physical);
	}


STFResult VirtualUnit::IsUnitCurrent(void)
	{
	return physical->IsUnitCurrent(this);
	}



//
// VirtualUnit constructor and destructor.
//

VirtualUnit::VirtualUnit(IPhysicalUnit * physical)
	{
	this->physical = physical;
	this->parent = NULL;
	this->root = NULL;
	this->activationGrantSignal = NULL;

	configureCounter = 0;

	physical->AddRef();
	}


VirtualUnit::~VirtualUnit(void)
	{
	assert(!(physical->IsUnitCurrent(this)));

	physical->Release();
	delete activationGrantSignal;
	}



//
// Partial IVDRBase implementation
//

STFResult VirtualUnit::QueryInterface(VDRIID iid, void *& ifp)
	{
	VDRQI_BEGIN
		VDRQI_IMPLEMENT(VDRIID_VIRTUAL_UNIT,		IVirtualUnit);
		VDRQI_IMPLEMENT(VDRIID_VDR_VIRTUAL_UNIT,	IVDRVirtualUnit);
		VDRQI_IMPLEMENT(VDRIID_VDR_DEVICE_UNIT,	IVDRDeviceUnit);
		VDRQI_IMPLEMENT(VDRIID_VDR_TAG_UNIT,		IVDRTagUnit);
	VDRQI_END(VDRBase);

	STFRES_RAISE_OK;
	}


//
// IVirtualUnit implementation, second part
//

STFResult VirtualUnit::Connect(IVirtualUnit * parent, IVirtualUnit * root)
	{
	//
	// Uptree links shall not be refcounted to prevent circular
	// reference counts.
	//
	this->parent = parent;
	this->root = root;

	if (!root)
		{
		// If we have no root then we are the topmost instance and must create the signal.
		this->activationGrantSignal = new STFArmedSignal();
		assert (this->activationGrantSignal != NULL);
		STFRES_ASSERT(this->activationGrantSignal != NULL, STFRES_NOT_ENOUGH_MEMORY);
		}

	STFRES_RAISE_OK;
	}


STFResult VirtualUnit::Initialize(void)
	{
	STFRES_RAISE_OK;
	}



//
// IVDRDeviceUnit implementation
//

VDRUID VirtualUnit::GetUnitID(void)
	{
	return physical->GetUnitID();
	}



//
// IVDRVirtualUnit implementation
//

STFResult VirtualUnit::ActivateAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration)
	{
	STFResult				err = STFRES_OK;
	STFHiPrec64BitTime	systemTime;

	//
	// Get this single activation mutex
	//
	if (!STFRES_IS_ERROR(err = LockActivationMutexes()))
		{
		//
		// Extend flags with preemption information, we want to do a full check and
		// activation, including recovery in the case of a failure.
		//
		flags |= VDRUALF_PREEMPT_DIRECT | VDRUALF_PREEMPT_RECOVER;

		//
		// If we are to wait, we set the preemption register flag as well
		//
		if (flags & VDRUALF_WAIT)
			flags |= VDRUALF_PREEMPT_REGISTER;

		//
		// Attempt initial activation
		//
		SystemTimer->GetTime(systemTime);
		err = InternalActivateAndLock(flags, time, duration, systemTime);

		//
		// While the activation is deferred
		//
		while (err == STFRES_OPERATION_PENDING)
			{
			// 
			// Unlock the activation mutex, wait for granted signal, and take the mutex again
			//
			if (!STFRES_IS_ERROR(err = WaitUnlockedForActivation()))
				{
				//
				// Retry the activation until failure or success
				//
				SystemTimer->GetTime(systemTime);
				err = InternalActivateAndLock(flags, time, duration, systemTime);
				}
			}

		//
		// Unlock the activation mutex
		//
		UnlockActivationMutexes();
		}

	STFRES_RAISE(err);
	}


STFResult VirtualUnit::Unlock(void)
	{
	STFResult				err = STFRES_OK;
	STFHiPrec64BitTime	systemTime;

	//
	// Get this single activation mutex
	//
	if (!STFRES_IS_ERROR(err = LockActivationMutexes()))
		{
		SystemTimer->GetTime(systemTime);
		err = InternalUnlock(systemTime);

		//
		// Unlock the activation mutex
		//
		UnlockActivationMutexes();
		}

	STFRES_RAISE(err);
	}


STFResult VirtualUnit::UnlockAndLock(uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration)
	{
	STFResult				err = STFRES_OK;
	STFHiPrec64BitTime	systemTime;

	//
	// Get this single activation mutex
	//
	if (!STFRES_IS_ERROR(err = LockActivationMutexes()))
		{
		//
		// Extend flags with preemption information, we want to do a full check and
		// activation, including recovery in the case of a failure.
		//
		flags |= VDRUALF_PREEMPT_DIRECT | VDRUALF_PREEMPT_RECOVER | VDRUALF_PREEMPT_UNLOCKLOCK;

		//
		// If we are to wait, we set the preemption register flag as well
		//
		if (flags & VDRUALF_WAIT)
			flags |= VDRUALF_PREEMPT_REGISTER;

		//
		// Attempt initial activation
		//
		SystemTimer->GetTime(systemTime);
		err = InternalUnlockAndLock(flags, time, duration, systemTime);

		//
		// While the activation is deferred, which means we lost the lock...
		//
		while (err == STFRES_OPERATION_PENDING)
			{
			// 
			// Unlock the activation mutex, wait for granted signal, and take the mutex again
			//
			if (!STFRES_IS_ERROR(err = WaitUnlockedForActivation()))
				{
				//
				// Retry the activation until failure or success
				//
				SystemTimer->GetTime(systemTime);
				err = InternalActivateAndLock(flags, time, duration, systemTime);
				}
			}

		//
		// Unlock the activation mutex
		//
		UnlockActivationMutexes();
		}

	STFRES_RAISE(err);
	}


STFResult VirtualUnit::Passivate(void)
	{
	STFResult				err = STFRES_OK;
	STFHiPrec64BitTime	systemTime;

	//
	// Get this single activation mutex
	//
	if (!STFRES_IS_ERROR(err = LockActivationMutexes()))
		{
		SystemTimer->GetTime(systemTime);
		err = InternalPassivate(systemTime);

		//
		// Unlock the activation mutex
		//
		UnlockActivationMutexes();
		}

	STFRES_RAISE(err);
	}



//
// IVDRTagUnit implementation
//

STFResult VirtualUnit::BeginConfigure(void)
	{
	STFRES_RAISE(InternalBeginConfigure());
	}


STFResult VirtualUnit::ConfigureTags(TAG * tags)
	{
	STFResult res;
	STFResult tres;

	STFRES_REASSERT(InternalBeginConfigure());
	res = InternalConfigureTags(tags);
	tres = InternalCompleteConfigure();

	// Return result with higher severity
	res = STFRES_HIGHER_SEVERITY(res, tres);

	STFRES_RAISE(res);
	}


STFResult VirtualUnit::Configure(const TAGList & tagList)
	{
	STFRES_RAISE(ConfigureTags((TAG *)(tagList.Tags())));
	}


STFResult VirtualUnit::CompleteConfigure(void)
	{
	STFRES_RAISE(InternalCompleteConfigure());
	}



//
// Partial ITagUnit implementation
//

STFResult VirtualUnit::GetTagIDs(VDRTID * & ids)
	{
	STFRES_RAISE(physical->GetTagIDs(ids));
	}


STFResult VirtualUnit::InternalBeginConfigure(void)
	{
	configureCounter++;

	if (configureCounter == 1)
		{
		STFRES_REASSERT(physical->BeginConfigure(this));
		}

	STFRES_RAISE_OK;
	}


STFResult VirtualUnit::InternalAbortConfigure(void)
	{
	configureCounter--;
	STFRES_RAISE_OK;
	}


STFResult VirtualUnit::InternalConfigureTags(TAG * tags)
	{
	// something generic possible here?
	STFRES_RAISE_OK;
	}


STFResult VirtualUnit::InternalCompleteConfigure(void)
	{
	configureCounter--;

	if (configureCounter == 0)
		{
		STFRES_REASSERT(InternalEndConfigure());
		STFRES_REASSERT(physical->EndConfigure(this));

		}

	STFRES_RAISE_OK;
	}


STFResult VirtualUnit::InternalUpdate(void)
	{
	DP("WARNING: Internal Update called but not overridden in unit %08x.\n", GetUnitID());
	DP("         If you see this message then you have unhandled tags!\n");

	STFRES_RAISE_OK;
	}

 STFResult VirtualUnit::InternalEndConfigure(void)
	{
	STFRES_RAISE_OK;
	}

