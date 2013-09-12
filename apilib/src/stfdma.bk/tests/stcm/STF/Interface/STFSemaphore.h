///
/// @file STF\Interface\STFSemaphore.h
///
/// @brief OS-independent semaphore (and synchronisation)-methods 
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

#ifndef STFSEMAPHORE_H
#define STFSEMAPHORE_H

// Includes
#include "STF/Interface/Types/STFResult.h"
#include "STF/Interface/Types/STFBasicTypes.h"
#include "OSSTFSemaphore.h"


/// STFGlobalLock is a global method to enter a system global critical section
STFResult STFGlobalLock(void);

/// STFGlobalUnlock is a global method to leave a system global critical section
STFResult STFGlobalUnlock(void);

//! Atomic thread-safe operations on an unsigned 32bit integer
class STFInterlockedInt
	{
	protected:
		OSSTFInterlockedInt	osi;
	public:
		STFInterlockedInt(int32 init = 0);
		
		//! Assignment operator
		/*!
		\param val value to assign to variable
		*/
		int32 operator=(int32 val);
		
		int32 operator++(void); // prefix
		int32 operator++(int);  // postfix
		int32 operator--(void); // prefix
		int32 operator--(int);	// postfix
		int32 operator+=(int n);
		int32 operator-=(int n);

		//! Typecast to an inr32
		operator int32(void);

		int32 CompareExchange(int32 cmp, int32 val);

	};

//! Thread-safe and Interrupt-safe access to an integer variable
class STFInterlockedPointer
	{
	protected:
		OSSTFInterlockedPointer	osp;
	public:
		STFInterlockedPointer(pointer val = NULL);

		//! Assignment
		pointer operator=(pointer val);

		//! Compare and Exchane
		/*! This _atomic_ operation does the following: It compares the internally stored pointer with the cmp argument and
		    if they match the internal pointer is replaced by the val pointer.
		*/
		pointer CompareExchange(pointer cmp, pointer val);

		operator pointer(void);
	};

/*! All objects which you want to push on the STFInterlockedStack (see below) must be derived from STFInterlockedNode.
    No implementation is neccessary.
*/
class STFInterlockedNode
	{
	friend class STFInterlockedStack;
	protected:
		STFInterlockedNode	*	node;
	};

//! A thread-safe stack whose operations are atomic
/*!
    This stack provides the standard stack operarions but these are implemeted as atomic operations so you you use these
	 from different concurrent threads. All classes you want to used with this stack must be derived from STFInterlockedNode
*/
class STFInterlockedStack
	{
	protected:
		STFInterlockedPointer	top;
	public:
		STFInterlockedStack(void);

		void Push(STFInterlockedNode * node);
		STFInterlockedNode * Pop(void);
		STFInterlockedNode * Top(void);
		bool Empty(void);
		void Clear(void);
	};


//! Interface: All STF classes which can be signalled implement this interface
class STFSignalable
	{
	public:
		//! Set the signal
		virtual STFResult Set(void) = 0;

		//! Reset the signals state to unsignalled
		virtual STFResult Reset(void) = 0;
	};

//! Interface: All STF classes for which can be waited implement this interface
class STFWaitable
	{
	public:
		//! Wait for the signal or whatever you're waiting for.
		/*! The calling thread will be blocked until the wait condition is satisfied. There is NO timeout.
		*/
		virtual STFResult Wait(void) = 0;

		//! Unblocking wait function
		/*! This function only checks the wait condition. It will return immediately whether the wait condition is
		    satisfied or not. If the wait condition is satisfied it will return STFRES_OK, otherwise STFRES_TIMEOUT
		*/
		virtual STFResult WaitImmediate(void) = 0;
	};

//! Add-on interface for classes which implement STFWaitable
/*! The wait funciton defined in this interface provides an additional timeout which the user can define
*/

class STFTimeoutWaitable : public STFWaitable
	{
	public:
		//! Wait until the wait condition has been satisfied or a timeout occurs
		/*! The duration parameter is the relativ time from the point in time where this function
		    is called to the timeout.
		*/
		virtual STFResult WaitTimeout(const STFLoPrec32BitDuration & duration) = 0;
	};

/// @brief A standard semaphore which is upon creation initialized to 0
class STFSemaphore : public STFSignalable, public STFTimeoutWaitable
	{
	protected:
		OSSTFSemaphore osSemaphore;

	public:
		STFSemaphore (void);
		virtual ~STFSemaphore (void);

		//
		// STFSignalable interface implementation
		//

		/// @brief Set the counter of the semaphore to 0
		virtual STFResult Reset(void);

		/// @brief Increase the semaphore counter by one
		virtual STFResult Set(void);

		/// @brief: Same functionality as Set, provided for compatibilty reasons
		virtual STFResult Signal(void);

		//
		// STFTimeoutWaitable interface implementation
		//

		/// @brief Wait for semaphore
		/// This call blocks if the counter of the semaphore is 0 until this counter increases above 0. Upon entering
		/// the counter is decreased by 1.
		virtual STFResult Wait(void);

		/// @brief Wait for semaphore	(non-blocking)
		/// Same functionality as Wait(), however this call is non-blocking and returns an error if the semaphore cannot
		/// be aquired (the count is already zero)
		virtual STFResult WaitImmediate(void);

		/// @brief Wait for the semaphore
		/// The duration parameter is the relativ time from the point in time where this function
		/// is called to the timeout.
		virtual STFResult WaitTimeout(const STFLoPrec32BitDuration & duration);

	};


// *********************************************
// Inlines
// *********************************************

// Globals
inline STFResult STFGlobalLock(void)
   {STFRES_RAISE(OSSTFGlobalLock());}

inline STFResult STFGlobalUnlock(void)
   {STFRES_RAISE(OSSTFGlobalUnlock());}


// STFSemaphore

inline STFSemaphore::STFSemaphore (void) : osSemaphore() { }

inline STFSemaphore::~STFSemaphore (void) { }

inline STFResult STFSemaphore::Reset(void) { STFRES_RAISE(osSemaphore.Reset()); }

inline STFResult STFSemaphore::Signal(void) { STFRES_RAISE(osSemaphore.Signal()); }

inline STFResult STFSemaphore::Set(void) { STFRES_RAISE(osSemaphore.Signal()); }

inline STFResult STFSemaphore::Wait(void) { STFRES_RAISE(osSemaphore.Wait()); }

inline STFResult STFSemaphore::WaitImmediate(void) { STFRES_RAISE(osSemaphore.WaitImmediate()); }

inline STFResult STFSemaphore::WaitTimeout(const STFLoPrec32BitDuration & duration) { STFRES_RAISE(osSemaphore.WaitTimeout(duration)); }


// STFInterlockedInt
 
inline STFInterlockedInt::STFInterlockedInt(int32 init)
	: osi(init) {}

inline int32 STFInterlockedInt::operator=(int32 val)
	{return osi=val;}

inline int32 STFInterlockedInt::operator++(void)
	{return ++osi;}

inline int32 STFInterlockedInt::operator++(int)
	{return osi++;}

inline int32 STFInterlockedInt::operator--(void)
	{return --osi;}

inline int32 STFInterlockedInt::operator--(int)
	{return osi--;}

inline int32 STFInterlockedInt::operator+=(int n)
	{return osi+=n;}

inline int32 STFInterlockedInt::operator-=(int n)
	{return osi-=n;}

inline int32 STFInterlockedInt::CompareExchange(int32 cmp, int32 val)
	{return osi.CompareExchange(cmp,val);}

inline STFInterlockedInt::operator int32(void)
	{return osi;}

// STFInterlockedPointer

inline STFInterlockedPointer::STFInterlockedPointer(pointer val)
	: osp(val) {}

inline pointer STFInterlockedPointer::operator=(pointer val)
	{return osp = val;}

inline pointer STFInterlockedPointer::CompareExchange(pointer cmp, pointer val)
	{return osp.CompareExchange(cmp, val);}

inline STFInterlockedPointer::operator pointer(void)
	{return osp;}

// STFInterlockedStack
		
inline STFInterlockedStack::STFInterlockedStack(void)
	{}

inline void STFInterlockedStack::Push(STFInterlockedNode * node)
	{
	pointer	p	= top;
	pointer	n;

	do {
		n = p;
		node->node = (STFInterlockedNode *)n;
		p = top.CompareExchange(n, node);
		} while (n != p);
	}

inline STFInterlockedNode * STFInterlockedStack::Pop(void)
	{
	pointer	p	= top;
	pointer	n;

	if (p)
		{
		do {
			n = p;
			p = top.CompareExchange(p, ((STFInterlockedNode *)p)->node);
			} while (n && n != p);

		return (STFInterlockedNode *)n;
		}
	else
		return NULL;
	}

inline STFInterlockedNode * STFInterlockedStack::Top(void)
	{return (STFInterlockedNode *)(pointer)top;}

inline bool STFInterlockedStack::Empty(void)
	{return top == NULL;}

inline void STFInterlockedStack::Clear(void)
	{top = NULL;}


#endif // STFSYNCHRONISATION_H
