///
/// @file STF\Interface\OSAL\OS20\OSSTFSemaphore.h
///
/// @brief OS-dependent semaphore (and synchronisation)-methods 
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


// Includes
#include <assert.h>
#include "STF/Interface/Types/STFResult.h"
#include "STF/Interface/Types/STFTime.h"


// OS specific headers
#include <semaphor.h>
#include <ostime.h>
#include <interrup.h>

//Globals
STFResult OSSTFGlobalLock(void);
STFResult OSSTFGlobalUnlock(void);

class OSSTFSemaphore
	{
	protected:
		semaphore_t * sema;
	public:
		OSSTFSemaphore (void);
		virtual ~OSSTFSemaphore (void);

		STFResult Reset(void);
		STFResult Signal(void);
		STFResult Wait(void);
		STFResult WaitImmediate(void);
		STFResult WaitTimeout(const STFLoPrec32BitDuration & duration);
	};

//! Thread-safe and Interrupt-safe access to an integer variable
class OSSTFInterlockedInt
	{
	protected:
		volatile int32		value;

	public:
		OSSTFInterlockedInt(int32 init = 0)
			: value(init) {}

		int32 operator=(int32 val)
			{
			interrupt_lock();
			value = val;
			interrupt_unlock();
			return val;
			}
		
		int32 operator++(void)
			{
			interrupt_lock();
			int32 result = ++value;
			interrupt_unlock();
			return result;
			}

		int32 operator++(int)
			{
			interrupt_lock();
			int32 result = value++;
			interrupt_unlock();
			return result;
			}

		int32 operator--(void)
			{
			interrupt_lock();
			int32 result = --value;
			interrupt_unlock();
			return result;
			}

		int32 operator--(int)
			{
			interrupt_lock();
			int32 result = value--;
			interrupt_unlock();
			return result;
			}

		int32 operator+=(int add)	
			{
			interrupt_lock();
			int32 result = (value += add);
			interrupt_unlock();
			return result;
			}

		int32 operator-=(int sub)
 			{
			interrupt_lock();	
			int32 result = (value -= sub);
			interrupt_unlock();
			return result;
			}

		int32 CompareExchange(int32 cmp, int32 val)
			{
			int32 p;

			// Compare value to cmp. If they match, replace value by val and return old value
			interrupt_lock();

			p = value;
			if (value == cmp)
				value = val;

			interrupt_unlock();

			return p;
			}

		operator int32(void)
			{
			return value;
			}
		};

class OSSTFInterlockedPointer
	{
	protected:
		volatile pointer					ptr;
	public:
		OSSTFInterlockedPointer(pointer val = NULL)
			: ptr(val) {}

		pointer operator=(pointer val)
			{
			interrupt_lock();

			ptr = val;

			interrupt_unlock();

			return val;
			}

		pointer CompareExchange(pointer cmp, pointer val)
			{
			pointer p;

			// Compare ptr to cmd. If they match, replace ptr by val and return old ptr value
			interrupt_lock();

			p = ptr;
			if (ptr == cmp)
				ptr = val;

			interrupt_unlock();

			return p;
			}

		operator pointer(void)
			{
			return ptr;
			}
	};
