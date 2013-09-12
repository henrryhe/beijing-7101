///
/// @file STF\Interface\STFMutex.h
///
/// @brief OS-independent mutex-methods 
///
/// @par OWNER: 
/// STF-Team
/// @author CJ & SB    
///
/// @par SCOPE:
///	INTERNAL Header File
///
/// @date       2003-12-11 changed header
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#ifndef STFMUTEX_H
#define STFMUTEX_H

#include "STF/Interface/STFSemaphore.h"	//this file is what is left of the old synchronisation.h

#include "STF/Interface/Types/STFResult.h"
#include "STF/Interface/Types/STFBasicTypes.h"
#include "OSSTFMutex.h"

//! Synchronisation object
/*! Only one thread at a time can be inside the mutex. The same thread can enter multiple times but must leave the mutex the same
    numer of times. Can not be used to synchronize over process boundaries under Win32.*/
class STFMutex
	{
	protected:
		OSSTFMutex	osm;
	public:
		STFMutex(void);

		//! Enter the mutex.
		/*! If another thread is already inside the mutex this call blocks the calling thread until the other
		    thread has called Leave()
		*/
		STFResult Enter(void);

		//! Leave the mutex
		/*! By calling this member a thread relinquishes ownership of the mutex and the new thread that is waiting is unblocked
		    and can enter the mutex.
		*/
		STFResult Leave(void);
	};


class STFAutoMutex
	{
	protected:
		STFMutex	*	mutex;
	public:
		STFAutoMutex(STFMutex	*	mutex)
			{
			this->mutex = mutex;
			mutex->Enter();
			}

		~STFAutoMutex(void)
			{
			mutex->Leave();
			}
	};

//! A mutex which allows multiple threads to be inside in non-exclusive mode, but only one in exclusive mode

class STFSharedMutex
	{
	protected:
		OSSTFSharedMutex	osm;
	public:
		STFSharedMutex(void);
		
		//! Enter the mutex
		/*! If a thread wants to Enter the mutex in non-exclusive mode it is permitted if no exclusive thread is inside.
		    If a thread wants to Enter in exclusive mode it is only permitted if no thread is currently inside the mutex.
		*/
		STFResult Enter(bool exclusive = true);

		//! Leave the mutex
		STFResult Leave(void);
	};



// *********************************************
// Inlines
// *********************************************


// STFMutex
inline STFMutex::STFMutex(void)
	: osm() {}

inline STFResult STFMutex::Enter()
	{STFRES_RAISE(osm.Enter());}

inline STFResult STFMutex::Leave(void)
	{STFRES_RAISE(osm.Leave());}

// STFSharedMutex
inline STFSharedMutex::STFSharedMutex(void)
	: osm() {}

inline STFResult STFSharedMutex::Enter(bool exclusive)
	{ STFRES_RAISE(osm.Enter(exclusive));}

inline STFResult STFSharedMutex::Leave(void)
	{STFRES_RAISE(osm.Leave());}

#endif //STFMUTEX_H
