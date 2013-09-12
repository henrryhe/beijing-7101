//
// FILE:      VDR/Source/Unit/IPhysicalUnit.h
// AUTHOR:    U. Sigmund, S. Herr
// COPYRIGHT: (C) 1995-2002 STMicroelectronics.  All Rights Reserved.
// CREATED:   26.11.2002
//
// PURPOSE:   Physical Unit interface
//
// SCOPE:     INTERNAL Header File


#ifndef IPHYSICALUNIT_H
#define IPHYSICALUNIT_H

#include "ITagUnit.h"
#include "VDR/Interface/Unit/IVDRUnit.h"

// Forward declarations
class IVirtualUnit;

///////////////////////////////////////////////////////////////////////////////
// Physical Unit Interface
///////////////////////////////////////////////////////////////////////////////

//! Internal "Physical Unit" Interface ID
static const VDRIID VDRIID_PHYSICAL_UNIT = 0x80000001;


//! Internal Physical Unit Interface
class IPhysicalUnit : virtual public IVDRPhysicalUnit,
							 virtual public ITagUnit
	{
	public:
		//! Take the activation mutex of this physical unit
		virtual STFResult LockActivationMutex(bool exclusive = true) = 0;

		//! Release the activation mutex of this physical unit
		virtual STFResult UnlockActivationMutex(void) = 0;

		//! Activate and lock the given virtual unit
		/*!
		Perform one or more steps of unit activation and locking for the given virtual unit.

		The extended flags (defined by \ref PrivateActivationFlags) specify, which subtasks
		of ActivateAndLock shall be performed by this call.

		*/
		virtual STFResult ActivateAndLock(IVirtualUnit * vunit, uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime) = 0;

		//! Unlock the given virtual unit
		/*!
		*/
		virtual STFResult Unlock(IVirtualUnit * vunit, const STFHiPrec64BitTime & systemTime) = 0;

		//! Passivate the given virtual unit
		/*!
		*/
		virtual STFResult Passivate(IVirtualUnit * vunit, const STFHiPrec64BitTime & systemTime) = 0;

		//! Unlock and Lock the given virtual unit
		/*!
		*/
		virtual STFResult UnlockAndLock(IVirtualUnit * vunit, uint32 flags, const STFHiPrec64BitTime & time, const STFHiPrec32BitDuration & duration, const STFHiPrec64BitTime & systemTime) = 0;

		//! Create a Virtual Unit from this Physical Unit
		/*!
			\param unit   Reference to pointer to newly create IVirtualUnit interface
			\param parent Parent Virtual Unit of new Virtual Unit
			\param root   Root Virtual Unit (top most Parent Virtual Unit) 
		*/
		virtual STFResult CreateVirtual(IVirtualUnit * & unit, IVirtualUnit * parent = NULL, IVirtualUnit * root = NULL) = 0;

		//
		// Unit Construction Functions
		//
		//! Creation function called during Allocation Phase of System Construction Process
		/*!
			\param createParams	Pointer to list of Unit Creation Parameters in the Global Board Configuration. 
										The parameters can be specified to set specific initialisation parameters 
										for the Physical Unit instance.
		*/
		virtual STFResult Create(long * createParams) = 0;

		//! Connection function called during Connection Phase of System Construction Process
		/*!
			\param localID Local ID of a Physical Unit which this Physical Unit is depending on. The local
								ID is the index of the referenced unit within the entry of this Physical Unit
								in the Global Board Configuration structure.
			\param source	Specifies the "source" Physical Unit that this Physical Unit is depending on
		*/
		virtual STFResult Connect(long localID, IPhysicalUnit * source) = 0;

		//! Initialization function called during Initialisation Phase of System Construction Process
		/*!
			\param depUnitsParams List containing all units this Physical Unit is depending on and their
					 initialization parameters.
		*/
		virtual STFResult Initialize(long * depUnitsParams) = 0;

		//!
		virtual STFResult BeginConfigure(IVirtualUnit * vUnit) = 0;

		//!
		virtual STFResult EndConfigure(IVirtualUnit * vUnit) = 0;

		//! Function can be called to determine current active unit
		/*!	
			Returns STFRES_TRUE if Virtual Unit is currently active, else STFRES_FALSE if Virtual Unit is
			not the currently active one
			\param vUnit Virtual Unit to be checked for active state. 
		*/
		virtual STFResult IsUnitCurrent(IVirtualUnit * vUnit) = 0; 

		//! Function to merge two tag list
		/*!
			\param inIDs			Pointer to the first list of supported Tag Ids from a unit 
			\param supportedIDs 	Pointer to the second list of supported Tag Ids from another unit
			\param outIDs			Pointer to the merged list of supported Tag ids formed from the two input Tag Ids list 
		*/
		virtual STFResult MergeTagTypeIDList(VDRTID * & inIDs,VDRTID * & supportedIDs,VDRTID * & outIDs)=0;
	};

#endif
