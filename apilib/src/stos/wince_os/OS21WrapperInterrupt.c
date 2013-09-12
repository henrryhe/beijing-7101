/*
 *********************************************************************
 *  @file   : OS21WrapperInterrupt.c
 *
 *
 *  Purpose : This module implements the WinCE wrapping functions for
 *            the OS21 Interrupt library.
 *
 * 7/6/2005 remplace U32 by unsigned int for STCM audio compatibility
 *********************************************************************
 */

#include <Nkintr.h>  // SYSINTR_* constants
#include <DBGAPI.h>  // otherwise pkfuncs.h does not compile !!
#include <pkfuncs.h> // Interrupt* functions
#include <kfuncs.h>  //  For SetEvent

#ifdef TRACE_THREAD_NAME
  #include "SetThreadTag.h"
#endif

#include <OS21WrapperInterrupt.h>
#define MAX_HANDLER 5

// Thread priority for the ISTs
// It is set to very high since we are masking interrupts until the ISTs are un-blocked
#ifdef WINCE_INTERRUPT_MAP
#define IST_PRIORITY 10  // was 97 for OS21 highest priority for device drivers 
#else
#define IST_PRIORITY 97
#endif

static CRITICAL_SECTION sCRInterruptLockUnlock;
static DWORD dwInterruptLock_Priority = -1;
static DWORD dwInterruptLock_ThreadId = -1; 
static DWORD dwCountSection = 0;

// Private data passed to each IST
typedef struct {
	interrupt_handler_t userHandler[MAX_HANDLER]; // user-provided handler function
	void *              userParam[MAX_HANDLER];   // user-provided data
	BOOL                bStopNow;    // commands the IST to stop
	HANDLE              hSyncEvent;	 // synchronization event
	HANDLE              hIST;        // handle to the IST
	DWORD               dwSysintr;   // logical interrupt number
	DWORD               dwIRQintr;   // hardware interrupt id
	DWORD				dwPriority;
	DWORD				dwNbHandler; // Nb Handleur for this see shared interrupt
	char				tName[60];
} IST_DATA;


// List of ISTs
static IST_DATA gISTList[SH4202T_IRQ_MAXIMUM + 1];

static int _lockCount = 0;
static int _lockSave  = 0;
#ifdef _USE_NAMED_INTERRUPTS
// WHE:
// In order to trace and modify dynamicaly the IST, we keep the name ( in tName) of the file that, init the interrupt
// Can be removed by changing #ifdef _USE_NAMED_INTERRUPTS
// mainly used in the debug toolbox of DSHOW project
// change nothing to the behavour of API

#undef interrupt_install_WinCE
#undef interrupt_install_shared_WinCE

int _Named_interrupt_install_shared_WinCE(unsigned int    interruptId,
					  interrupt_handler_t userHandler,
					  void *              userParam,
					  char *			  pComment
					 )
{
	IST_DATA* myIST; // data for the current IST
	char *pfname = strchr(pComment,0);
	int ret = interrupt_install_shared_WinCE(interruptId,userHandler,userParam);
	myIST = gISTList + interruptId;

	if(pfname == 0) return ret;
	while(pfname != pComment)
	{
		if(*(pfname-1) == '\\' || *(pfname-1) == '/') break;
		pfname--;
	}
	strncpy(myIST->tName,pfname,sizeof(myIST->tName));
	return ret;	

}

int _Named_interrupt_install_WinCE(unsigned int                 interruptId,
					  interrupt_handler_t userHandler,
					  void *              userParam,
					  char *			  pComment
					 )
{
	IST_DATA* myIST; // data for the current IST
	char *pfname = strchr(pComment,0);
	int ret = interrupt_install_WinCE(interruptId,userHandler,userParam);
	myIST = gISTList + interruptId;
	if(pfname == 0) return ret;
	while(pfname != pComment)
	{
		if(*(pfname-1) == '\\' || *(pfname-1) == '/') break;
		pfname--;
	}
	strncpy(myIST->tName,pfname,sizeof(myIST->tName));
	return ret;	

}
#endif

void _WCE_InterruptInitialize()
{
	dwInterruptLock_Priority = -1;
	dwInterruptLock_ThreadId = -1;
	dwCountSection = 0;
	InitializeCriticalSection(&sCRInterruptLockUnlock);
}

void _WCE_InterruptTerminate()
{
	DeleteCriticalSection(&sCRInterruptLockUnlock);
}

char * _WCE_EnumStapiISTName(int Index)
{
	IST_DATA *pTask;
	if(Index >= SH4202T_IRQ_MAXIMUM + 1) return NULL;
	pTask =  &gISTList[Index];
	if(pTask->dwNbHandler == 0) return NULL;
	return pTask->tName;
}

int _WCE_SetCeISTPriority(int Index, int WincePriority)
{
	IST_DATA *pTask;
	if(Index >= SH4202T_IRQ_MAXIMUM + 1) return FALSE;
	pTask =  &gISTList[Index];
	
	WCE_VERIFY(CeSetThreadPriority(pTask->hIST,pTask->dwPriority) != 0);
	return TRUE;
}



int _WCE_GetCeISTPriority(int Index)
{

	IST_DATA *pTask;
	if(Index >= SH4202T_IRQ_MAXIMUM + 1) return -1;
	pTask =  &gISTList[Index];
	return (int)pTask->dwPriority;
}


/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param state : 
 *
 * @return static void  : 
 */
static void EnableStapiInterrupts(int state)
{
	

//	return;
	
	int interruptId= 0;
	for(interruptId= 0; interruptId < SH4202T_IRQ_MAXIMUM ; interruptId++)
	{
		IST_DATA *myIST = gISTList + interruptId;

		if(myIST->dwNbHandler != 0) 
		{
			if(state == false) interrupt_disable_WinCE(interruptId);
			else			   interrupt_enable_WinCE(interruptId);
		}
	}

}



// -------------------------------------------------------------------------------------
//
//
//				Private functions
//
//


static int FindSharedHandler(IST_DATA *myIST,interrupt_handler_t handler,void * param)
{
	int cptHandler;
	for(cptHandler = 0; cptHandler < myIST->dwNbHandler ; cptHandler++)
	{
		if(myIST->userHandler[cptHandler] == handler && myIST->userParam[cptHandler] == param) return cptHandler;
	}
	return -1;
}


// Function: ISTEntryPoint
// Calls user handler & InterruptDone()
int ThreadProc(LPVOID lpParameter)
{
	IST_DATA* self = (IST_DATA*) lpParameter; // retrieves our context
	int	nhandler;
  	DWORD dwOldPermissions;
  	
	dwOldPermissions=GetCurrentPermissions();  // Save procedure's permissions
	SetProcPermissions(0xFFFFFFFF);  // Enable access to all processes (for timing info)
	
	while (!self->bStopNow)
	{
        WCE_MSG(MDL_OSCALL, "ThreadProc(): Waiting on event for IRQ %d / SYSINTR %d", self->dwIRQintr, self->dwSysintr);
        // wait on event
        P_ADDSYSTEMCALL(P_WAIT_OBJECT,0);
		if (WaitForSingleObject(self->hSyncEvent, INFINITE) != WAIT_OBJECT_0)
		{
			WCE_ERROR("WaitForSingleObject(ISTsync) failed!");
			return -1;
		}
		
		if (self->bStopNow) // spurious event raised only to kill thread
			break;

        WCE_MSG(MDL_OSCALL, "ThreadProc(): event! for SYSINTR %d", self->dwSysintr);

		for(nhandler = 0 ; nhandler < self->dwNbHandler ; nhandler++)
		{
			// calls user handler
			(*(self->userHandler[nhandler]))(self->userParam[nhandler]);
		}

		// ACK interrupt
        WCE_MSG(MDL_OSCALL, "ThreadProc(): >InterruptDone for SYSINTR %d", self->dwSysintr);
		InterruptDone(self->dwSysintr);
	}

	SetProcPermissions(dwOldPermissions);  // Disable access to all processes (for timing info)

	// end of thread
	return 0;
}


// Function: GetNewSysintr
// Ask the OAL (via the kernel) for a new Sysintr
unsigned long  GetNewSysintr(unsigned int interruptId)
{
    DWORD newSysintr = SYSINTR_NOP;
    DWORD sizeOut = 0;

    if (!KernelIoControl( IOCTL_HAL_REQUEST_SYSINTR,            // IO control code
                          &interruptId, sizeof(interruptId),    // IN : the IRQ number
                          &newSysintr, sizeof(newSysintr),      // OUT: the new sysintr
                          &sizeOut
                        ))
    {
        WCE_ERROR("GetNewSysintr: KernelIoControl failed !");
    }
    WCE_VERIFY(sizeOut == sizeof(newSysintr)); // sanity check
    WCE_VERIFY(newSysintr >= 0 && newSysintr < SYSINTR_MAXIMUM); // just to be sure

    return newSysintr;
}

void ReleaseSysintr(DWORD sysintr)
{
    DWORD sizeOut = 0;

    if (!KernelIoControl( IOCTL_HAL_RELEASE_SYSINTR,    // IO control code
                          &sysintr, sizeof(sysintr),    // IN : sysintr to release
                          NULL, 0,                      // OUT: not used
                          &sizeOut                      // not used
                        ))
    {
        WCE_ERROR("GetNewSysintr: KernelIoControl failed !");
    }
}





// -------------------------------------------------------------------------------------
//
//
//				Interrupt functions  WinCE Implementation
//
//

// Function: interrupt_install_WinCE
// a) Asks the kernel for a new SYSINTR_*
// b) Sets up an event/IST plumbing.
// c) Connects the plumbing with InterruptInitialize()
// Note: interrupt_install_WinCE() does not enable the interrupt.
// Returns: OS21_SUCCESS for success, OS21_FAILURE for failure.
int interrupt_install_WinCE(unsigned int                 interruptId, // this is an IRQ number
                            interrupt_handler_t userHandler,
                            void *              userParam,
                            char *             pName
                           )
{
	
	DWORD  dwSysintr = SYSINTR_NOP; // the logical interrupt ID
	IST_DATA* myIST; // data for the current IST
    DWORD threadId;
    char name[100];

	WCE_MSG(MDL_OSCALL,"-- interrupt_install_WinCE(IRQ=%d)", interruptId);

    // retrieve a SYSINTR for this IRQ
    dwSysintr = GetNewSysintr(interruptId);
    if (dwSysintr == SYSINTR_NOP)
        return OS21_FAILURE;
    WCE_MSG(MDL_OSCALL,"interrupt_install_WinCE: new sysintr=%d", dwSysintr);

    // get the IST context
    WCE_VERIFY(interruptId >= 0 && interruptId <= SH4202T_IRQ_MAXIMUM); // sanity check
	myIST = gISTList + interruptId;

	// Creates the Event for our IST
	myIST->hSyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	WCE_ASSERT(myIST->hSyncEvent != NULL);
	if (myIST->hSyncEvent == NULL)
		return OS21_FAILURE;

	// Creates the IST
	myIST->bStopNow = FALSE;
	myIST->userHandler[0] = userHandler;
	myIST->userParam[0]  = userParam;
	myIST->dwSysintr = dwSysintr;
    myIST->dwIRQintr = interruptId;
	myIST->dwPriority = IST_PRIORITY;
	strcpy(myIST->tName,"STAPI IST ");
	if(pName)
	{
		strncat(myIST->tName,pName,20);
	}
	myIST->dwNbHandler = 1;
	myIST->hIST = CreateThread(NULL,         // Security
							   0,            // No Stack Size
							   ThreadProc,   // Entry point
							   (void*)myIST, // Entry point parameters
							   CREATE_SUSPENDED, // Create Suspended
							   NULL              // don't need Thread Id
							  );
	WCE_ASSERT(myIST->hIST != NULL);
	
	#ifdef TRACE_THREAD_NAME
	  SetThreadTag((HANDLE)myIST->hIST, (DWORD)&myIST->tName);
	#endif
	
	if (myIST->hIST == NULL)
	{
		CloseHandle(myIST->hSyncEvent);
		return OS21_FAILURE;
	}

    sprintf(name, "IntHandler(%d)", interruptId);
    P_ADDTHREAD(myIST->hIST, GetCurrentThreadId(), name);

	// Set priority
	if(!CeSetThreadPriority(myIST->hIST, myIST->dwPriority))
	{
		WCE_ASSERT(0);
		TerminateThread(myIST->hIST, 0);
		CloseHandle(myIST->hSyncEvent);
		CloseHandle(myIST->hIST);
		return OS21_FAILURE;
	}

//  printf("WinCE IST priority : %s[%x] - WINCE=%d\n", "-", myIST->hIST, myIST->dwPriority);

	// Initialize the interrupt
	InterruptMask(dwSysintr, TRUE); // Make sure interrupt is disabled
	if (!InterruptInitialize(dwSysintr, myIST->hSyncEvent, NULL,0)) 
	{
		WCE_ASSERT(0);
		TerminateThread(myIST->hIST, 0);
		CloseHandle(myIST->hSyncEvent);
		CloseHandle(myIST->hIST);
		return OS21_FAILURE;
	}
	WCE_MSG(MDL_OSCALL, "interrupt_install_WinCE(): InterruptInitialize OK", interruptId);
	InterruptMask(dwSysintr, TRUE); // Make sure interrupt is still disabled

	// Get the thread started
	if (ResumeThread(myIST->hIST) == 0xFFFFFFFF)
	{
		WCE_ASSERT(0);
		TerminateThread(myIST->hIST, 0);
  		CloseHandle(myIST->hSyncEvent);
		CloseHandle(myIST->hIST);
		return OS21_FAILURE;
	}

	return OS21_SUCCESS;
	// note that interrupt is disabled until interrupt_enable_number() is called
}

int interrupt_install_WinCE_2(
							unsigned int        interruptId, // this is an IRQ number
                            DWORD *             sysIntr, 
							HANDLE              hSyncEvent,
							int                 stackSize,
                            interrupt_handler_t userHandler,
                            void *              userParam,
							char *              taskName
                           )
{
	
	DWORD  dwSysintr = SYSINTR_NOP; // the logical interrupt ID
	semaphore_t *sem = (semaphore_t *)hSyncEvent;
	IST_DATA* myIST; // data for the current IST
    DWORD threadId;
    char name[100];

	WCE_MSG(MDL_OSCALL,"-- interrupt_install_WinCE(IRQ=%d)", interruptId);

    // retrieve a SYSINTR for this IRQ
    dwSysintr = GetNewSysintr(interruptId);
    if (dwSysintr == SYSINTR_NOP)
        return OS21_FAILURE;
    WCE_MSG(MDL_OSCALL,"interrupt_install_WinCE: new sysintr=%d", dwSysintr);

	CloseHandle(sem->hHandle_opaque);

	sem->hHandle_opaque = CreateEvent(NULL, FALSE, FALSE, NULL);
	sem->eSynchObjType  = WCE_EVENT;

    // get the IST context
    WCE_VERIFY(interruptId >= 0 && interruptId <= SH4202T_IRQ_MAXIMUM); // sanity check
	myIST = gISTList + interruptId;

	// Creates the Event for our IST
	myIST->hSyncEvent = sem->hHandle_opaque;

	WCE_ASSERT(myIST->hSyncEvent != NULL);
	if (myIST->hSyncEvent == NULL)
		return OS21_FAILURE;

	// Creates the IST
	myIST->bStopNow = FALSE;
	myIST->userHandler[0] = userHandler;
	myIST->userParam[0]  = userParam;
	myIST->dwSysintr = dwSysintr;
    myIST->dwIRQintr = interruptId;
	myIST->dwPriority = IST_PRIORITY;
	strcpy(myIST->tName, taskName);
	myIST->dwNbHandler = 1;

    /* Create the InterruptProcess task */
	myIST->hIST = CreateThread(NULL,         // Security
							   stackSize,    // Stack Size
							   userHandler,  // Entry point
							   (void*)userParam, // Entry point parameters
							   CREATE_SUSPENDED, // Create Suspended
							   NULL              // don't need Thread Id
							  );
	WCE_ASSERT(myIST->hIST != NULL);

    sprintf(name, "%s(%d)", taskName, interruptId);	
	#ifdef TRACE_THREAD_NAME
	  SetThreadTag((HANDLE)myIST->hIST, name);
	#endif
	
	if (myIST->hIST == NULL)
	{
		CloseHandle(myIST->hSyncEvent);
		return OS21_FAILURE;
	}

    P_ADDTHREAD(myIST->hIST, GetCurrentThreadId(), name);

	// Set priority
	if(!CeSetThreadPriority(myIST->hIST, myIST->dwPriority))
	{
		WCE_ASSERT(0);
		TerminateThread(myIST->hIST, 0);
		CloseHandle(myIST->hSyncEvent);
		CloseHandle(myIST->hIST);
		return OS21_FAILURE;
	}

//  printf("WinCE IST priority : %s[%x] - WINCE=%d\n", "-", myIST->hIST, myIST->dwPriority);

	// Initialize the interrupt
	InterruptMask(dwSysintr, TRUE); // Make sure interrupt is disabled
	if (!InterruptInitialize(dwSysintr, myIST->hSyncEvent, NULL,0)) 
	{
		WCE_ASSERT(0);
		TerminateThread(myIST->hIST, 0);
		CloseHandle(myIST->hSyncEvent);
		CloseHandle(myIST->hIST);
		return OS21_FAILURE;
	}
	WCE_MSG(MDL_OSCALL, "interrupt_install_WinCE(): InterruptInitialize OK", interruptId);
	InterruptMask(dwSysintr, TRUE); // Make sure interrupt is still disabled

	// Get the thread started
	if (ResumeThread(myIST->hIST) == 0xFFFFFFFF)
	{
		WCE_ASSERT(0);
		TerminateThread(myIST->hIST, 0);
  		CloseHandle(myIST->hSyncEvent);
		CloseHandle(myIST->hIST);
		return OS21_FAILURE;
	}

	*sysIntr = dwSysintr; 

	return OS21_SUCCESS;
	// note that interrupt is disabled until interrupt_enable_number() is called
}

// interrupt_uninstall_WinCE
// a) tells kernel to disable interrupt with InterruptDisable()
// b) terminates the IST
// c) closes the handles (event, IST)
int interrupt_uninstall_WinCE(unsigned int interruptId)
{
	IST_DATA* myIST;  // data for the current IST
	WCE_MSG(MDL_OSCALL,"-- interrupt_uninstall_WinCE(%d)",interruptId);

	if(interruptId == 0)
	{
		// found in vid_dec line 107, call  interrupt_uninstall but not initialized
        return OS21_FAILURE;

	}
    // get the IST context
    WCE_VERIFY(interruptId >= 0 && interruptId <= SH4202T_IRQ_MAXIMUM); // sanity check
	myIST = gISTList + interruptId;
	if (interruptId != myIST->dwIRQintr)
    {
        // Found during VTG unit tests: sometimes interrupt_uninstall() is called twice or more
		WCE_TRACE("interrupt_uninstall() called twice or gISTList corrupted");
        return OS21_FAILURE;
    }

	myIST->bStopNow = TRUE; // commands termination of the thread

	// disable interrupt in WinCE kernel
	InterruptDisable(myIST->dwSysintr);
	// Note: some (but not all) WinCE docs says that this also signal the event.
	//       so the SetEvent below might be useless.

	// awake the thread so it can terminates
	if (!SetEvent(myIST->hSyncEvent)) 
	{
		WCE_ASSERT(0);
		return OS21_FAILURE;
	}
	
	// join terminated thread to me
	if (WaitForSingleObject(myIST->hIST, 1000) != WAIT_OBJECT_0)
	{
		WCE_ERROR("WaitForSingleObject(IST) failed!");
		return -1;
	}

	// close handles
	CloseHandle(myIST->hIST);
	CloseHandle(myIST->hSyncEvent);

    // release sysintr
    ReleaseSysintr(myIST->dwSysintr);
    // mark node as invalid
	myIST->dwNbHandler = 0;
    myIST->dwIRQintr = SH4202T_IRQ_MAXIMUM + 1;

	return OS21_SUCCESS;
}


// interrupt_enable_WinCE
int interrupt_enable_WinCE(unsigned int interruptId)
{
    IST_DATA* myIST;  // data for the current IST
	WCE_MSG(MDL_OSCALL,"-- interrupt_enable_WinCE(%d)",interruptId);

    // get the IST context
    WCE_VERIFY(interruptId >= 0 && interruptId <= SH4202T_IRQ_MAXIMUM); // sanity check
	myIST = gISTList + interruptId;
	if (interruptId != myIST->dwIRQintr)
    {
		WCE_ERROR("gISTList corrupted!");
        return OS21_FAILURE;
    }

    WCE_MSG(MDL_OSCALL, "interrupt_enable_WinCE(IRQ=%d): calling InterruptMask(SYSINTR=%d,FALSE)",
            interruptId, myIST->dwSysintr);
	InterruptMask(myIST->dwSysintr, FALSE);
	return OS21_SUCCESS;
}

// interrupt_raise_WinCE
int interrupt_raise_WinCE(unsigned int interruptId)
{
	WCE_NOT_IMPLEMENTED();
	return OS21_FAILURE;
}

// interrupt_disable_WinCE
int interrupt_disable_WinCE(unsigned int interruptId)
{
    IST_DATA* myIST;  // data for the current IST
	WCE_MSG(MDL_OSCALL,"-- interrupt_disable_WinCE(%d)",interruptId);

    // get the IST context
    WCE_VERIFY(interruptId >= 0 && interruptId <= SH4202T_IRQ_MAXIMUM); // sanity check
	myIST = gISTList + interruptId;
	if (interruptId != myIST->dwIRQintr)
    {
        // Sometimes "disable" is called after "uninstall" (e.g. STCLKRV_Term).
        // Since we manually disable before uninstalling, it is OK.
		WCE_MSG(MDL_MSG,"gISTList corrupted for IRQ %d: usually <disable> called after <uninstall>", interruptId);
        return OS21_SUCCESS;
    }

    WCE_MSG(MDL_OSCALL, "interrupt_disable_WinCE(IRQ=%d): calling InterruptMask(SYSINTR=%d,TRUE)",
            interruptId, myIST->dwSysintr);
	InterruptMask(myIST->dwSysintr, TRUE);
	return OS21_SUCCESS;
}

#define IMASK = 0x000000F0
#define MASK_ALL  15

/*
Import Assembler From OS21 kernel code, this code set the IMASK bits in the SR SH4 control register
The goal is to mask all interrupt during a short time.
The function accepts the level interrupt mask to set and return the default one.
input param come in r4, return param in r0 !! Hoping compilation rules always the same !! 
*/

#pragma off

static int asm_sh4_intr_mask(int level)
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

#pragma on

// interrupt_lock_WinCE(void)
void interrupt_lock_WinCE()
{
 if (_lockCount < 0)
		printf ("Big Problem Lock !\n");
//  P_ADDSYSTEMCALL(P_IT_LOCK,0);

  if (_lockCount == 0)
  {
//	 P_ADDSYSTEMCALL(P_IT_LOCK,0);
    _lockSave = asm_sh4_intr_mask ((int) MASK_ALL);
  }
  else
  {
//	P_ADDSYSTEMCALL(P_IT_LOCK_RE,0);
  }
  _lockCount++;
} 


// interrupt_unlock_WinCE(void)
void interrupt_unlock_WinCE(void)
{
//  WCE_ASSERT (_lockCount > 0);
  if (_lockCount <= 0)
		printf ("Big Problem Unlock !\n");
  _lockCount--;
  if (_lockCount == 0)
  {
	int result;

    result = asm_sh4_intr_mask (_lockSave);
//	P_ADDSYSTEMCALL(P_IT_UNLOCK,0);
  }
  else
  {
//	  P_ADDSYSTEMCALL(P_IT_UNLOCK_RE,0);
  }

}

// interrupt_mask_all
int interrupt_mask_all_WinCE(void)
{
	
	return OS21_SUCCESS;
	
	WCE_MSG(MDL_OSCALL,"-- interrupt_mask_all()");

    // WCE_MSG(MDL_OSCALL, "interrupt_mask_all_WinCE(): calling INTERRUPTS_OFF");
	// INTERRUPTS_OFF();
//    WCE_MSG(MDL_OSCALL, "interrupt_mask_all_WinCE(): doing nothing");
	EnableStapiInterrupts(false);

	return IST_PRIORITY; // not used in WinCE
}


// interrupt_unmask
int interrupt_unmask_all_WinCE()
{
	return OS21_SUCCESS; 
	
	WCE_MSG(MDL_OSCALL,"-- interrupt_unmask_all_WinCE");
	// "priority" not used in WinCE
    // WCE_MSG(MDL_OSCALL, "interrupt_unmask_WinCE(): calling INTERRUPTS_ON");
	// INTERRUPTS_ON();
	EnableStapiInterrupts(true);
	return OS21_SUCCESS; // not used in WinCE

}




/*!-------------------------------------------------------------------------------------
 * Attempts to poll the specified interrupt. Some interrupts cannot be polled and in
 * this case the poll fails. For successful calls the result of the poll is placed in the
 * location pointed to by value.
 *
 * @param ip : 
 * @param value : 
 *
 * @return int   : 
 */
int   interrupt_poll_WinCE (unsigned int interruptId, int * value)
{
    WCE_MSG(MDL_OSCALL, "interrupt_poll_WinCE(%d,%d)",interruptId,value);
	WCE_NOT_IMPLEMENTED();
	return OS21_SUCCESS;
}

/*!-------------------------------------------------------------------------------------
 * Attempts to clear the specified interrupt. Many interrupts are automatically
 * cleared when the hardware stops asserting them. Some interrupt controllers
 * however latch the interrupt, these have to be cleared otherwise the interrupt
 * remains asserted, causing the processor to take an unwanted interrupt.
 *
 * @param interruptId : 
 *
 * @return int  : 
 */
int interrupt_clear_WinCE(unsigned int interruptId)
{
    WCE_MSG(MDL_OSCALL, "interrupt_clear_WinCE(%d)",interruptId);

//	WCE_NOT_IMPLEMENTED();
	return OS21_SUCCESS;
}


/*!-------------------------------------------------------------------------------------
 * Uninstall an interrupt handler where the interrupt source is shared.
 Results
	Returns OS21_SUCCESS for success, OS21_FAILURE for failure.
	Errors
	OS21_FAILURE if the interrupt source ip is invalid, if the combination of handler
	and param are not currently registered for the interrupt source, or if the interrupt
	has been marked as nonshareable.
	Context
	Callable from task or interrupt handler.
 *
 * @param ip : 
 * @param handler : 
 * @param param : 
 *
 * @return int interrupt_uninstall_shared  : 
 */
int interrupt_uninstall_shared_Wince (unsigned int  interruptId,interrupt_handler_t handler,void * param)
{
    IST_DATA* myIST;  // data for the current IST
	int indexHandler;
	int	cptHandler;
	WCE_MSG(MDL_OSCALL, "interrupt_uninstall_shared_WInce(%d)",interruptId);
    // Get the IST context
    WCE_VERIFY(interruptId >= 0 && interruptId <= SH4202T_IRQ_MAXIMUM); // sanity check

	//	Make the handler
		
	myIST = gISTList + interruptId;
	if(myIST->dwNbHandler == 0) 
	{
		// not sharable, only one instance 
		return OS21_FAILURE;
	}
	indexHandler =  FindSharedHandler(myIST,handler,param);

	if(myIST->dwNbHandler == 1)
  {
	WCE_ASSERT(indexHandler == 0); // must be 0
    myIST->dwNbHandler  = 0;
    return interrupt_uninstall_WinCE(interruptId);
  }


	if(indexHandler == -1) 
	{
		// never the first one ( original)
		// and not found
		 return OS21_FAILURE;
	}

	// Enter in a frozen situation
	InterruptMask(myIST->dwSysintr, TRUE); // Make sure interrupt is disable
	WCE_VERIFY(SuspendThread(myIST->hIST) != 0xFFFFFFFF);
	task_lock();

	for(cptHandler = indexHandler ; cptHandler < MAX_HANDLER-1 ; cptHandler++)
	{
		myIST->userHandler[cptHandler] = myIST->userHandler[cptHandler+1];
		myIST->userParam[cptHandler] = myIST->userParam[cptHandler+1];
	}
	// remove teh count 
	myIST->dwNbHandler--;

	task_unlock();
	WCE_VERIFY(ResumeThread(myIST->hIST) != 0xFFFFFFFF);
	InterruptMask(myIST->dwSysintr, FALSE); // Make sure interrupt is enable
	return OS21_SUCCESS;
}



/*
 OS21 provides a mechanism for more than one interrupt handler to share an
interrupt. Handlers are chained by means of interrupt_install_shared()
which adds an interrupt handler to the chain of handlers for a given interrupt. This
increases interrupt latency, but may be required where different hardware
interrupts are routed to the same interrupt vector


This installs the specified user interrupt handler for the interrupt source described
by ip. The handler function is called with its the single parameter set to param.
The user handler returns OS21_SUCCESS if it handled the interrupt, otherwise it
returns OS21_FAILURE. This call allows a multiple interrupt handler to be
registered for the given source, therefore allowing interrupt vector sharing. When
an interrupt from source ip is detected by OS21, it calls each handler that has been
registered, until one returns OS21_SUCCESS. If no handler accepts the interrupt
OS21 will panic.
Once any handlers have been registered with the call, any call to
interrupt_install() for this ip will fail, since it is now set for shared use.
 */

int interrupt_install_shared_WinCE(unsigned int                 interruptId, // this is an IRQ number
                            interrupt_handler_t userHandler,
                            void *              userParam,
                            char *   pName
                           )
{
	IST_DATA* myIST; // data for the current IST
	myIST = gISTList + interruptId;
	if(myIST->dwNbHandler == 0) 
	{
		return interrupt_install_WinCE(interruptId,userHandler,userParam,pName);
	}
	else
	{
	    WCE_MSG(MDL_OSCALL, "interrupt_install_shared_WinCE(%d)",interruptId);
		WCE_ASSERT(myIST->dwNbHandler < MAX_HANDLER);
		
		InterruptMask(myIST->dwSysintr, TRUE); // Make sure interrupt is disable
		WCE_VERIFY(SuspendThread(myIST->hIST) != 0xFFFFFFFF);
		task_lock();
		myIST->userHandler[myIST->dwNbHandler] = userHandler;
		myIST->userParam[myIST->dwNbHandler]  = userParam;
		myIST->dwNbHandler++;
		task_unlock();
			
		WCE_VERIFY(ResumeThread(myIST->hIST) != 0xFFFFFFFF);
		InterruptMask(myIST->dwSysintr, FALSE); // Make sure interrupt is enable


	}
	return OS21_SUCCESS;
}


/*!-------------------------------------------------------------------------------------
 * This function is used to query the priority of an interrupt. The current priority of
 * the given interrupt is written to the location pointed to by priority.
 *
 * @param interruptId : 
 * @param priority : 
 *
 * @return int   : 
 */
int   interrupt_priority_WinCE (unsigned int interruptId,int * priority)
{
	
	IST_DATA* myIST; // data for the current IST

    WCE_MSG(MDL_OSCALL, "interrupt_priority_WinCE(%d,%d)",interruptId,priority);

	myIST = gISTList + interruptId;
	WCE_ASSERT(priority);
	*priority = myIST->dwPriority;
	WCE_MSG(MDL_WARNING,"To Be verified by JLX");
	return OS21_SUCCESS;
}

