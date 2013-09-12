///
/// @file STF\Interface\STFSignal.h
///
/// @brief OS-independent signal-methods 
///
/// @par OWNER: 
/// STF-Team
/// @author Christian Johner & Stephan Bergmann    
///
/// @par SCOPE:
///	INTERNAL Header File
///
/// @date       2003-12-11 changed header
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#ifndef STFSIGNAL_H
#define STFSIGNAL_H

// Includes
#include "STF/Interface/STFSemaphore.h"	//this file is what is left of the synchronisation.h

#include "STF/Interface/Types/STFResult.h"
#include "STF/Interface/Types/STFBasicTypes.h"
#include "OSSTFSignal.h"

class STFSignal : virtual public STFWaitable, virtual public STFSignalable
	{
	protected:
		OSSTFSignal		oss;
	public:
		STFSignal(void);
		virtual ~STFSignal() {}

		//
		// Direct calls (functionally identical to the interface group below)
		//
		STFResult SetSignal(void);
		STFResult ResetSignal(void);
		STFResult WaitSignal(void);
		STFResult WaitImmediateSignal(void);

		//
		// Interface calls (for descriptions see STFSignallable and STFWaitable interfaces above)
		//
		STFResult Set(void);
		STFResult Reset(void);
		STFResult Wait(void);
		STFResult WaitImmediate(void);
	};

//! Identical to STFSignal, but implements additionally the STFTimeoutWaitable interface
class STFTimeoutSignal : virtual public STFTimeoutWaitable, virtual public STFSignalable
	{
	protected:
		OSSTFTimeoutSignal		oss;
	public:
		STFTimeoutSignal(void);
		virtual ~STFTimeoutSignal() {}


		//
		// Direct calls
		//
		STFResult SetSignal(void);
		STFResult ResetSignal(void);
		STFResult WaitSignal(void);
		STFResult WaitImmediateSignal(void);
		STFResult WaitTimeoutSignal(const STFLoPrec32BitDuration & duration);

		//
		// Interface calls
		//
		virtual STFResult Set(void);
		virtual STFResult Reset(void);
		virtual STFResult Wait(void);
		virtual STFResult WaitImmediate(void);
		virtual STFResult WaitTimeout(const STFLoPrec32BitDuration & duration);
	};

//! The STFArmedSignal is similar to STFSignal but it has to be signalled multiple (user-defined) time before the wait condition is satisfied
class STFArmedSignal : virtual public STFWaitable, virtual public STFSignalable
	{
	protected:
		OSSTFArmedSignal		oss;
	public:
		// The counter is initializes to 0
		STFArmedSignal(void);
		virtual ~STFArmedSignal() {}

		//
		// Direct calls
		//

		//! Set the number of times this signal has to be Set() before the wait condition is satisfied
		/*! This call is additive, that means if you first call ArmSignal(5) and then ArmSignal(2) have have
		    to call SetSignal() seven times (or setSignal(7) ) until the wait condition is satisfied.
			 Initially this counter is 0.
		*/
		STFResult ArmSignal(uint16 n = 1);

		//! Set the signal. Calling this function with a parameter n > 1 is sematically identical to calling it n times without parameter.
		/*! This function effectively decreases the internal counter.
		*/
		STFResult SetSignal(uint16 n = 1);

		//! Resets the signal if it is in signalled state and reset the counter to 0
		STFResult ResetSignal(void);
		STFResult WaitSignal(void);
		STFResult WaitImmediateSignal(void);

		//
		// Interface calls
		//
		virtual STFResult Set(void);
		virtual STFResult Reset(void);
		virtual STFResult Wait(void);
		virtual STFResult WaitImmediate(void);
	};

// *********************************************
// Inlines
// *********************************************


inline STFSignal::STFSignal(void)
	{}
// Direct calls

inline STFResult STFSignal::SetSignal(void)
	{STFRES_RAISE(oss.SetSignal());}

inline STFResult STFSignal::ResetSignal(void)
	{STFRES_RAISE(oss.ResetSignal());}

inline STFResult STFSignal::WaitSignal(void)
	{STFRES_RAISE(oss.WaitSignal());}

inline STFResult STFSignal::WaitImmediateSignal(void)
	{STFRES_RAISE(oss.WaitImmediateSignal());}

// Interface calls

inline STFResult STFSignal::Set(void)
	{STFRES_RAISE(oss.SetSignal());}

inline STFResult STFSignal::Reset(void)
	{STFRES_RAISE(oss.ResetSignal());}

inline STFResult STFSignal::Wait(void)
	{STFRES_RAISE(oss.WaitSignal());}

inline STFResult STFSignal::WaitImmediate(void)
	{STFRES_RAISE(oss.WaitImmediateSignal());}

// STFTimeoutSignal

inline STFTimeoutSignal::STFTimeoutSignal(void)
	{}

inline STFResult STFTimeoutSignal::SetSignal(void)
	{STFRES_RAISE(oss.SetSignal());}

inline STFResult STFTimeoutSignal::ResetSignal(void)
	{STFRES_RAISE(oss.ResetSignal());}

inline STFResult STFTimeoutSignal::WaitSignal(void)
	{STFRES_RAISE(oss.WaitSignal());}

inline STFResult STFTimeoutSignal::WaitImmediateSignal(void)
	{STFRES_RAISE(oss.WaitImmediateSignal());}

inline STFResult STFTimeoutSignal::WaitTimeoutSignal(const STFLoPrec32BitDuration & duration)
	{
	if (duration.Get32BitDuration(STFTU_LOWSYSTEM) > 0) STFRES_RAISE(oss.WaitTimeoutSignal(duration));
	STFRES_RAISE_OK;
	}

inline STFResult STFTimeoutSignal::Set(void)
	{STFRES_RAISE(oss.SetSignal());}

inline STFResult STFTimeoutSignal::Reset(void)
	{STFRES_RAISE(oss.ResetSignal());}

inline STFResult STFTimeoutSignal::Wait(void)
	{STFRES_RAISE(oss.WaitSignal());}

inline STFResult STFTimeoutSignal::WaitImmediate(void)
	{STFRES_RAISE(oss.WaitImmediateSignal());}

inline STFResult STFTimeoutSignal::WaitTimeout(const STFLoPrec32BitDuration & duration)
	{STFRES_RAISE(oss.WaitTimeoutSignal(duration));}

// STFArmedSignal

inline STFArmedSignal::STFArmedSignal(void)
	{}

inline STFResult STFArmedSignal::ArmSignal(uint16 n)
	{STFRES_RAISE(oss.ArmSignal(n));}

inline STFResult STFArmedSignal::SetSignal(uint16 n)
	{STFRES_RAISE(oss.SetSignal(n));}

inline STFResult STFArmedSignal::ResetSignal(void)
	{STFRES_RAISE(oss.ResetSignal());}

inline STFResult STFArmedSignal::WaitSignal(void)
	{STFRES_RAISE(oss.WaitSignal());}

inline STFResult STFArmedSignal::WaitImmediateSignal(void)
	{STFRES_RAISE(oss.WaitImmediateSignal());}


// Implementation of virtual member functions
inline STFResult STFArmedSignal::Set(void)
	{
	STFRES_RAISE(oss.SetSignal(1));
	}

inline STFResult STFArmedSignal::Reset(void)
	{
	STFRES_RAISE(oss.ResetSignal());
	}

inline STFResult STFArmedSignal::Wait(void)
	{
	STFRES_RAISE(oss.WaitSignal());
	}

inline STFResult STFArmedSignal::WaitImmediate(void)
	{
	STFRES_RAISE(oss.WaitImmediateSignal());
	}



#endif //STFSIGNAL_H
