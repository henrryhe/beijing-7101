

void MyLeaveCriticalSection(CRITICAL_SECTION *p);
void MyEnterCriticalSection(CRITICAL_SECTION *p);
#ifdef CR_LOCKIT



#define _IMASK = 0x000000F0
#define _MASK_ALL  15

/*
Import Assembler From OS21 kernel code, this code set the IMASK bits in the SR SH4 control register
The goal is to mask all interrupt during a short time.
The function accepts the level interrupt mask to set and return the default one.
input param come in r4, return param in r0 !! Hoping compilation rules always the same !! 
*/

extern unsigned int inline_lockSave;
__forceinline int sh4_intr_mask(int level)
{
    int result;
// the prolog of this function put r4 as level arg
 __asm (
     "stc     sr, r0 ; load status register, save r0 for futur use\n"
     "mov     r0, r2 ; r2 = status register \n"
    "mov  #60, r1 ; // load r1 with interrupt flag\n"
    "shll2   r1 \n"
    "and     r1, r0 ; //jsilva save sr bits for lockSave\n"
    "shll2   r4\n"
    "shll2   r4\n"
    "and     r1, r4 ; //jsilva save ensure we change only the 4 intr bits in level \n" 
    "not   r1, r1 \n"
    "and     r1, r2\n"
    "or      r4, r2\n"
    "ldc     r2, sr\n"
    "shlr2   r0 \n"
    "shlr2   r0 \n"
    "mov r0,@r5",
    level  , &result);

    return result;


// r0 is the return value
 
}

#endif	

__forceinline __int64 __WCE_Tick2Millisecond(__int64 ticks)
{
	__int64 millisec ;
	millisec = (__int64)(((double)ticks * 1000) / time_ticks_per_sec()+ 0.5);
	return millisec;
}


/*!-------------------------------------------------------------------------------------
 * Lock the mutex ( wait until the mutex is released)
 *
 * @param *psem : 
 *
 * @return int  : T_ETIMEDOUT
 */
__forceinline int _WCE_Semaphore_Lock(semaphore_t *psem)
{
	int iRet;

	if (psem->eSynchObjType==CRIT_SECT)
	{
#ifndef CR_LOCKIT
		MyEnterCriticalSection((CRITICAL_SECTION*)psem->hHandle_opaque);
#else

		 inline_lockSave = sh4_intr_mask ((int) _MASK_ALL);
#endif
	//	return T_SIGNALED;
	}
	else
	{
		iRet = WaitForSingleObject(psem->hHandle_opaque,INFINITE);
		if(iRet == WAIT_ABANDONED) return T_ABANDONED;
	}
	return T_SIGNALED;
}






/*!-------------------------------------------------------------------------------------
 * Lock semaphore with time out
 *
 * @param *psem : 
 *
 * @return int  : 
 */
__forceinline int _WCE_Semaphore_TimedLock(semaphore_t *psem, DWORD timeout )
{
	int iRet=1;
	iRet = WaitForSingleObject(psem->hHandle_opaque,timeout);
	if(iRet == WAIT_TIMEOUT) 	return T_ETIMEDOUT;
	if(iRet == WAIT_ABANDONED) return T_ABANDONED;
	return T_SIGNALED;
}


 


/*!-------------------------------------------------------------------------------------
 * Unlock sur mutex ( Signaled)
 *
 * @param *psem : 
 *
 * @return int  : 1 if ok
 */
__forceinline int _WCE_Semaphore_Unlock(semaphore_t *psem)
{
	
	if (psem->eSynchObjType==CRIT_SECT)
	{
#ifndef CR_LOCKIT	
		MyLeaveCriticalSection((CRITICAL_SECTION*)psem->hHandle_opaque);
#else
	 sh4_intr_mask ((int) inline_lockSave);
	
#endif		
	}
	if (psem->eSynchObjType==SEMAPHORE)
	{
		ReleaseSemaphore(psem->hHandle_opaque,1,NULL);		
	}
	if (psem->eSynchObjType==WCE_EVENT)
	{
		SetEvent(psem->hHandle_opaque);
	}
	return 1;
}




/*
 * semaphore_wait
 */
__forceinline int _semaphore_wait(semaphore_t *sem)
{
    return (_WCE_Semaphore_Lock(sem)? OS21_SUCCESS : OS21_FAILURE);
}

/*!-------------------------------------------------------------------------------------
 * 
 * semaphore_wait_timeout() performs a wait operation on the specified
 * semaphore (sem). If the time specified by the timeout is reached before a signal
 * operation is performed on the semaphore, then semaphore_wait_timeout()
 * returns the value OS21_FAILURE indicating that a timeout occurred, and the
 * semaphore count is unchanged. If the semaphore is signalled before the timeout is
 * reached, then semaphore_wait_timeout() returns OS21_SUCCESS.
 * Note: Timeout is an absolute not a relative value, so if a relative timeout is required this
 * needs to be made explicit, as shown in the example below.
 * The timeout value may be specified in ticks, which is an implementation
 * dependent quantity. Two special time values may also be specified for timeout.
 * TIMEOUT_IMMEDIATE causes the semaphore to be polled, that is, the function
 * always returns immediately. This must be the value used if
 * semaphore_wait_timeout() is called from an interrupt service routine. If the
 * semaphore count is greater than zero, then it is successfully decremented, and the
 * function returns OS21_SUCCESS, otherwise the function returns a value of
 * OS21_FAILURE. A timeout of TIMEOUT_INFINITY behaves exactly as
 * semaphore_wait().
 *
 * @param *sem : 
 * @param timeout_p : 
 *
 * @return int  : 
 */
__forceinline int _semaphore_wait_timeout(semaphore_t *sem, osclock_t * timeout_p)
{
    int    ret = -1; /* Timeout by default */
    DWORD dwTicks;


    if (timeout_p == TIMEOUT_IMMEDIATE)
        dwTicks = 0;
    else if (timeout_p == TIMEOUT_INFINITY)
        dwTicks = INFINITE;
    else
    {
        osclock_t now = time_now();

        if (time_after(*timeout_p, now)) //timeout not yet reached
           dwTicks = (DWORD)__WCE_Tick2Millisecond(time_minus(*timeout_p, now));
        else
		   return OS21_FAILURE;
    }

    ret = _WCE_Semaphore_TimedLock(sem, dwTicks);

    return (ret == T_SIGNALED ? OS21_SUCCESS : OS21_FAILURE);
}


/*!-------------------------------------------------------------------------------------
 * Signal the semaphore 
 *
 * @param *sem : 
 *
 * @return int  : 
 */
__forceinline int _semaphore_signal(semaphore_t *sem)
{

     return  _WCE_Semaphore_Unlock(sem) ? OS21_SUCCESS : OS21_FAILURE;
}



/*******************************************************************************
Name        : STOS_SemaphoreSignal
Description : OS-independent implementation of semaphore_signal
Parameters  : semaphore to signal to
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
__forceinline void STOS_SemaphoreSignal(semaphore_t * Semaphore_p)
{
    _semaphore_signal(Semaphore_p);
}

/*******************************************************************************
Name        : STOS_SemaphoreWait
Description : OS-independent implementation of semaphore_wait
Parameters  : semaphore to wait for
Assumptions :
Limitations :
Returns     : Always returns STOS_SUCCESS
*******************************************************************************/
__forceinline int STOS_SemaphoreWait(semaphore_t * Semaphore_p)
{

    _semaphore_wait(Semaphore_p);
    return(0);
}

/*******************************************************************************
Name        : STOS_SemaphoreWaitTimeOut
Description : OS-independent implementation of semaphore_wait_timeout
Parameters  : semaphore to wait for, time out value
Assumptions :
Limitations :
Returns     : STOS_SUCCESS if success, STOS_FAILURE if timeout occurred
*******************************************************************************/
__forceinline  int STOS_SemaphoreWaitTimeOut(semaphore_t * Semaphore_p, osclock_t * TimeOutValue_p)
{
    return(_semaphore_wait_timeout(Semaphore_p, (osclock_t*) TimeOutValue_p));
}

