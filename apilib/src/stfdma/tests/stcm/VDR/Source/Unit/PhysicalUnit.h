#ifndef PHYSICALUNIT_H
#define PHYSICALUNIT_H

///
/// @file       VDR/Source/Unit/PhysicalUnit.h
///
/// @brief      Generic Physical Units
///
/// @author     U. Sigmund, S. Herr
///
/// @par OWNER: VDR Architecture Team
///
/// @par SCOPE: INTERNAL Header File
///
/// @date       2002-12-06
///
/// &copy; 2003 STMicroelectronics. All Rights Reserved.
///

#include "IVirtualUnit.h"
#include "IPhysicalUnit.h"
#include "VDR/Source/Base/VDRBase.h"
#include "STF/Interface/STFSynchronisation.h"


///////////////////////////////////////////////////////////////////////////////
// Physical Unit
///////////////////////////////////////////////////////////////////////////////

//! Generic standard Physical Unit implementation
class PhysicalUnit : public VDRBase,
							virtual public IPhysicalUnit
	{
	protected:
		STFSharedMutex		activationMutex;
		VDRUID				unitID;				//! Global Unit ID
		STFInterlockedInt	configureCounter;	//! Counts nested Tag configuration calls
		uint32				changeSet;			//! Indicates which group of properties was affected by Tag configurations

		//
		// Commodities
		//

		/// Get the actual number of creation parameters
		uint32 GetNumberOfParameters(long * createParams);

		/// Get the nth creation parameter
		STFResult GetDWordParameter(long * createParams, uint32 index, uint32 & param);
		STFResult GetStringParameter(long * createParams, uint32 index, char * & param);
		STFResult GetPointerParameter(long * createParams, uint32 index, void * & param);

	public:
		PhysicalUnit(VDRUID unitID)
			{
			this->unitID = unitID;
			configureCounter = 0;
			changeSet = 0;
			}

		//
		// Partial IVDRBase implementation
		//
		virtual STFResult QueryInterface(VDRIID iid, void * & ifp);

		//
		// IVDRDeviceUnit implementation
		//
		virtual VDRUID GetUnitID(void) {return unitID;}

		//
		// Part of IPhysicalUnit implementation
		//
		virtual STFResult LockActivationMutex(bool exclusive = true)
			{
			return activationMutex.Enter(exclusive);
			}

		virtual STFResult UnlockActivationMutex(void)
			{
			return activationMutex.Leave();
			}

		virtual STFResult BeginConfigure(IVirtualUnit * vUnit);
		virtual STFResult EndConfigure(IVirtualUnit * vUnit);

		virtual STFResult IsUnitCurrent(IVirtualUnit * vUnit); 
		virtual STFResult MergeTagTypeIDList(VDRTID * & inIDs,VDRTID * & supportedIDs,VDRTID * & outIDs);

		//
		// IVDRTagUnit implementation
		//
		virtual STFResult BeginConfigure(void);
		virtual STFResult ConfigureTags(TAG * tags);
		virtual STFResult Configure(const TAGList & tagList);
		virtual STFResult CompleteConfigure(void);

		//
		// ITagUnit implementation (partial)
		//
		virtual STFResult InternalBeginConfigure(void);
		virtual STFResult InternalAbortConfigure(void);
		virtual STFResult InternalCompleteConfigure(void);

		// The following 3 are default implementations. They should be overridden if
		// the unit supports tags!
		virtual STFResult GetTagIDs(VDRTID * & ids);
		virtual STFResult InternalConfigureTags(TAG * tags);
		virtual STFResult	InternalUpdate(void);
	};



///////////////////////////////////////////////////////////////////////////////
// Registered Virtual Unit
///////////////////////////////////////////////////////////////////////////////

struct RegisteredVirtualUnit
	{
	IVirtualUnit			*	unit;
	uint32						flags, age;
	STFHiPrec64BitTime		time;
	STFHiPrec32BitDuration	duration;

	RegisteredVirtualUnit(void)
		{
		}

	RegisteredVirtualUnit(IVirtualUnit * unit_, uint32 flags_, uint32 age_, const STFHiPrec64BitTime & time_, const STFHiPrec32BitDuration & duration_)
		: unit(unit_), flags(flags_), age(age_), time(time_), duration(duration_)
		{
		}

	bool operator < (const RegisteredVirtualUnit & runit) const
		{
		uint32	pri, rpri;
		
		pri  = flags & VDRUALF_PRIORITY_MASK;
		rpri = runit.flags & VDRUALF_PRIORITY_MASK;

		if (pri == rpri)
			{
			if (flags & VDRUALF_TIME_VALID)
				{
				if (runit.flags & VDRUALF_TIME_VALID)
					{
					if (time == runit.time)
						return age < runit.age;
					else
						return time < runit.time;
					}
				else
					return true;
				}
			else if (runit.flags & VDRUALF_TIME_VALID)
				return false;
			else
				return age < runit.age;
			}
		else
			return pri > rpri;
		}
	};

///////////////////////////////////////////////////////////////////////////////
// Exclusive Physical Unit
///////////////////////////////////////////////////////////////////////////////

//! Further specialisation of Physical Unit, which only allows exclusive use by one Virtual Unit
class ExclusivePhysicalUnit : public PhysicalUnit
	{
	protected:
		IVirtualUnit				*	currentUnit;
		RegisteredVirtualUnit	*	registeredUnits;
		int								numRegisteredUnits, maxRegisteredUnits;
		int								lockCount;	
		uint32							ageCount;
 		uint32							preemptionPhase;	//! State of a preemption process

		STFResult CancelRegistration(IVirtualUnit * vunit, RegisteredVirtualUnit & runit);

		STFResult AddRegistration(const RegisteredVirtualUnit & runit);

		STFResult SignalTopRegistration(void);

	public:
		ExclusivePhysicalUnit(VDRUID unitID);
		virtual ~ExclusivePhysicalUnit(void);

		//
		// Part of IPhysicalUnit implementation
		//
		virtual STFResult ActivateAndLock(IVirtualUnit * vunit, uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime);
		virtual STFResult Unlock(IVirtualUnit * vunit, const STFHiPrec64BitTime & systemTime);
		virtual STFResult Passivate(IVirtualUnit * vunit, const STFHiPrec64BitTime & systemTime);
		virtual STFResult UnlockAndLock(IVirtualUnit * vunit, uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime);

		virtual STFResult BeginConfigure(IVirtualUnit * vUnit);
		virtual STFResult EndConfigure(IVirtualUnit * vUnit);

		virtual STFResult IsUnitCurrent(IVirtualUnit * vUnit); 
	};

///////////////////////////////////////////////////////////////////////////////
// Shared Physical Unit
///////////////////////////////////////////////////////////////////////////////

class SharedPhysicalUnit : public PhysicalUnit
	{
	protected:
		struct CurrentVirtualUnitShare
			{
			IVirtualUnit	*	unit;
			int					lockCount;
			uint32				preemptionPhase;	//! State of a preemption process
			bool					active;
			} * currentUnits;
		int	numCurrentUnits, maxCurrentUnits;


	public:
		SharedPhysicalUnit(VDRUID unitID);
		virtual ~SharedPhysicalUnit(void);

		//
		// Part of IPhysicalUnit implementation
		//
		virtual STFResult ActivateAndLock(IVirtualUnit * vunit, uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime);
		virtual STFResult Unlock(IVirtualUnit * vunit, const STFHiPrec64BitTime & systemTime);
		virtual STFResult Passivate(IVirtualUnit * vunit, const STFHiPrec64BitTime & systemTime);
		virtual STFResult UnlockAndLock(IVirtualUnit * vunit, uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime);

		virtual STFResult BeginConfigure(IVirtualUnit * vUnit);
		virtual STFResult EndConfigure(IVirtualUnit * vUnit);

		virtual STFResult IsUnitCurrent(IVirtualUnit * vUnit); 
	};

#endif
