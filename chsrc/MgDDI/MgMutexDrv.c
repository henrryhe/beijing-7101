#ifdef    TRACE_CAINFO_ENABLE
#define  MGMUTEX_DRIVER_DEBUG
#endif
/******************************************************************************
*
* File : MgMutexDrv.C
*
* Description : SoftCell driver
*
*
* NOTES :
*
*
* Author :
*
* Status : 
*
* History : 0.0   2004-06-08  Start coding
*           
*
* Copyright: Changhong 2004 (c)
*
*****************************************************************************/
/*****************************************************************************
 *BASIC DEFINITIONS
 *****************************************************************************/
#include "MgMutexDrv.h"



/*******************************************************************************
 *Function name: MGDDIMutexCreate
 *           
 * 
 *Description: This function creates a mutex for the current task,It returns a mutex
 *             resource handle that identificates the mutex resource in all mutex
 *             related interfaces.  
 * 
 *Prototype:
 *     TMGDDIMutexHandle MGDDIMutexCreate()
 *       
 *           
 *
 *
 *input:
 *           
 *           
 *
 *output:
 *           
 *           
 *           
 *Return code
 *    Mutex resource handle,if not null 
 *    NULL,if a problem has occurred during processing.
 *         - no mutex handle available.
 *
 *Comments:
 *    The default state of the newly created mutex resource is unlocked.Its lock
 *    count is null. 
 *
 *******************************************************************************/
 TMGDDIMutexHandle MGDDIMutexCreate(void)
 {
        TMGDDIMutexHandle              DdiMutexHandle;
        DdiMutexHandle =(TMGDDIMutexHandle)CHCA_MutexCreate();
	 return DdiMutexHandle;
 } 

/*******************************************************************************
 *Function name : MGDDIMutexDestroy
 *           
 * 
 *Description: This function destroys the mutex resource related to the hMutex 
 *             handle
 *
 *             If the mutex resources is locked and task are blocked by a lock
 *             request on this resource,the blocked tasks must be awakened.
 *
 *Prototype:
 *      TMGDDIStatus  MGDDIMutexDestroy(TMGDDIMutexHandle  hMutex)     
 *            
 *
 *
 *input:
 *      hMutex:     Mutex resource handle     
 *           
 *
 *output:
 *           
 *           
 *           
 *Return code
 *     MGDDIOk:         Execution correct.
 *     MGDDIBadParam:   Mutex resource handle unknown 
 *     MGDDIError:      Interface execution error
 *
 *Comments:
 *
 *
 *
 *******************************************************************************/
 TMGDDIStatus  MGDDIMutexDestroy(TMGDDIMutexHandle  hMutex)
 {
        TMGDDIStatus                 StatusMgDdi = MGDDIOK; 
        StatusMgDdi = (TMGDDIStatus)CHCA_MutexDestroy((TCHCAMutexHandle)hMutex);
 	  return (StatusMgDdi);
 }

/*******************************************************************************
 *Function name : MGDDIMutexLock
 *           
 * 
 *Description: This function request the locking of the mutex resource related the
 *             hMutex handle.
 *
 *             If the mutex resource is unlocked,it is now locked by the current
 *             task having requested it.its lock count is set to 1 and MGDDIOk  
 *             code is returned.
 *
 *             If the current task requesting the mutex resource has already locked
 *             the mutex resource,it is not blocked.The lock count of the mutex
 *             resource is increased and MGDDIOk code is returned
 *
 *             If the mutex resource has already been locked by another task,the 
 *             current task requesting the mutex locking is blocked in the interface
 *             call.when the mutex resource is released,the task is awakened by  
 *             returning from the MGDDIMutexLock function with MGDDIOk return code.
 *             At that time,the mutex resource is locked again by the awakened task
 *             and it lock count is set to 1. 
 *
 *Prototype:
 *       TMGDDIStatus  MGDDIMutexLock(TMGDDIMutexHandle  hMutex)    
 *           
 *
 *
 *input:
 *      hMutex:    Mutex resource handle      
 *           
 *
 *output:
 *           
 *           
 *           
 *Return code
 *      MGDDIOk:         Mutex resource locked
 *      MGDDIBadParam:   Mutex resource handle unknown
 *      MGDDIError:      Interface execution error
 *
 *Comments:
 *
 *
 *
 *******************************************************************************/
 TMGDDIStatus  MGDDIMutexLock(TMGDDIMutexHandle  hMutex)
 {
       TMGDDIStatus                 StatusMgDdi=MGDDIOK; 
       StatusMgDdi = (TMGDDIStatus)CHCA_MutexLock((TCHCAMutexHandle)hMutex);
	 return (StatusMgDdi);
 } 


/*******************************************************************************
 *Function name: MGDDIMutexUnlock
 *           
 * 
 *Description: This function decreases the lock count of the mutex resource related
 *             to the hMutex handle.If this count reaches 0,the mutex resource is 
 *             unlocked.If the lock count is already null at function call,lock count  
 *             remains null,nothing else applies and MGDDIOk code is returned.
 *  
 *             When a mutex resource is unlocked and if tasks were blocked by a lock  
 *             request on this resource,the first task blocked that requests the mutex
 *             resource is awakened.The mutex resource is locked again but by the 
 *             awakened task and its lock count is set to 1.   
 *   
 *   
 *Prototype:
 *      TMGDDIStatus   MGDDIMutexUnlock(TMGDDIMutexhandle   hMutex)     
 *           
 *
 *
 *input:
 *      hMutex:   Mutex resource handle     
 *           
 *
 *output:
 *           
 *           
 *           
 *Return code
 *      MGDDIOk:         Mutex resource locked
 *      MGDDIBadParam:   Mutex resource handle unknown
 *      MGDDIError:      Interface execution error    
 *
 *
 *Comments:
 *
 *
 *
 *******************************************************************************/
 TMGDDIStatus   MGDDIMutexUnlock( TMGDDIMutexHandle   hMutex)
 {                                                               
        TMGDDIStatus                 StatusMgDdi = MGDDIOK; 
        StatusMgDdi = (TMGDDIStatus)CHCA_MutexUnlock((TCHCAMutexHandle)hMutex);
  
	 return (StatusMgDdi);

 }



