#define  CHMUTEX_DRIVER_DEBUG
/******************************************************************************
*
* File : ChMutex.C
*
* Description : 
*
*
* NOTES :
*
*
* Author : 
*
* Status :
*
* History : 0.0    2004-6-26   Start coding
*           
* copyright: Changhong 2004 (c)
*
*****************************************************************************/
#include   "ChMutex.h"


TCHCAMutex              MutexInstance[CHCA_MUTEX_MAX_NUM];

semaphore_t              *pSemCHCAMutexAccess=NULL;        


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
 TCHCAMutexHandle   CHCA_MutexCreate(void)
 {
        /*semaphore_t                  *psemMgDDIMutex = NULL;*/
        CHCA_UINT                       MutexIndex;

        semaphore_wait(pSemCHCAMutexAccess);
	 for(MutexIndex=0;MutexIndex<CHCA_MUTEX_MAX_NUM;MutexIndex++)
	 {
              if(!MutexInstance[MutexIndex].InUsed)
              {
                      break;
		}
	 }

	 if(MutexIndex>=CHCA_MUTEX_MAX_NUM)
	 {
	        semaphore_signal(pSemCHCAMutexAccess);
               return NULL;
	 }
	 	
        MutexInstance[MutexIndex].InUsed = true;
        MutexInstance[MutexIndex].Locked = false;
     
	 MutexInstance[MutexIndex].psemMutex = semaphore_create_fifo ( 1 );	

	 if(MutexInstance[MutexIndex].psemMutex==NULL)
	 {
               do_report(severity_info,"\n[CHCA_MutexCreate::] can not open the semaphore\n");
		 semaphore_signal(pSemCHCAMutexAccess);	   
		 return NULL;	   
	 }
        semaphore_signal(pSemCHCAMutexAccess);

	 return (MutexIndex+1);
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
 CHCA_DDIStatus  CHCA_MutexDestroy(TCHCAMutexHandle  hMutex)
 {
        CHCA_DDIStatus                 StatusMgDdi = MGDDIOK; 
	 CHCA_UINT                         MutexIndex;

 	 MutexIndex = hMutex - 1;
 	 semaphore_wait(pSemCHCAMutexAccess);
	 if((hMutex==NULL)||(MutexIndex>=CHCA_MUTEX_MAX_NUM)||\
	    (MutexInstance[MutexIndex].psemMutex==NULL)||(!MutexInstance[MutexIndex].InUsed))
        {
              do_report(severity_info,"\n Mutex Destroy fail for MGDDIBadParam\n");	 	
              StatusMgDdi = MGDDIBadParam;
              semaphore_signal(pSemCHCAMutexAccess);
	       return (StatusMgDdi); 
	 }	

	 if(MutexInstance[MutexIndex].psemMutex==NULL)
	 {
              StatusMgDdi = MGDDIBadParam;
              semaphore_signal(pSemCHCAMutexAccess);
	       return (StatusMgDdi); 
	 }

	 if(!MutexInstance[MutexIndex].InUsed)
	 {
              semaphore_signal(pSemCHCAMutexAccess);
	       return (StatusMgDdi); 
	 }

	 MutexInstance[MutexIndex].InUsed = false;
	 semaphore_delete(MutexInstance[MutexIndex].psemMutex);

	 MutexInstance[MutexIndex].psemMutex = NULL;
        semaphore_signal(pSemCHCAMutexAccess);

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
 CHCA_DDIStatus  CHCA_MutexLock(TCHCAMutexHandle  hMutex)
 {
        CHCA_DDIStatus                 StatusMgDdi = MGDDIOK; 
	 CHCA_UINT                         MutexIndex;
	 semaphore_t*                      psemMutexTemp = NULL;

 	 MutexIndex = hMutex - 1;
        semaphore_wait(pSemCHCAMutexAccess);
	 if((hMutex==NULL)||(MutexIndex>=CHCA_MUTEX_MAX_NUM)||\
	    (MutexInstance[MutexIndex].psemMutex==NULL)||(!MutexInstance[MutexIndex].InUsed))
        {
             StatusMgDdi = MGDDIBadParam;
             semaphore_signal(pSemCHCAMutexAccess);
	      return (StatusMgDdi); 
	 }	

        if(MutexInstance[MutexIndex].psemMutex==NULL) 
        {
             StatusMgDdi = MGDDIBadParam;
             semaphore_signal(pSemCHCAMutexAccess);
	      return (StatusMgDdi); 
	 }

        if(!MutexInstance[MutexIndex].InUsed)
        {
             StatusMgDdi = MGDDIBadParam;
             semaphore_signal(pSemCHCAMutexAccess);
	      return (StatusMgDdi); 
	 }

        if(MutexInstance[MutexIndex].Locked)   
        {
              semaphore_signal(pSemCHCAMutexAccess);
              return StatusMgDdi;
	 }

	 MutexInstance[MutexIndex].Locked = true;

	 psemMutexTemp = MutexInstance[MutexIndex].psemMutex;

        semaphore_signal(pSemCHCAMutexAccess);


	 semaphore_wait(psemMutexTemp);
	 
	 
	 return (StatusMgDdi);
	  	
 } 


/*******************************************************************************
 *Function name: CHCA_MutexUnlock
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
 *      CHCA_DDIStatus   CHCA_MutexUnlock( TCHCAMutexHandle   hMutex)
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
 *      CHCADDIOk:         Mutex resource locked
 *      CHCADDIBadParam:   Mutex resource handle unknown
 *      CHCADDIError:      Interface execution error    
 *
 *
 *Comments:
 *
 *
 *
 *******************************************************************************/
 CHCA_DDIStatus   CHCA_MutexUnlock( TCHCAMutexHandle   hMutex)
 {                                                               
        CHCA_DDIStatus                 StatusMgDdi = MGDDIOK; 
     	 semaphore_t*                      psemMutexTemp=NULL;
        CHCA_UINT                         MutexIndex;
	
	 MutexIndex = hMutex - 1;
        semaphore_wait(pSemCHCAMutexAccess);
        if((hMutex==NULL)||(MutexIndex>=CHCA_MUTEX_MAX_NUM)||\
	    (MutexInstance[MutexIndex].psemMutex==NULL)||(!MutexInstance[MutexIndex].InUsed))
        {
             do_report(severity_info,"\n[CHCA_MutexLock::] Mutex resource handle unknown\n");	 	
             StatusMgDdi = MGDDIBadParam;
             semaphore_signal(pSemCHCAMutexAccess);
	      return (StatusMgDdi); 
	 }

	 
        MutexInstance[MutexIndex].Locked = false;

        psemMutexTemp = MutexInstance[MutexIndex].psemMutex;		
        semaphore_signal(pSemCHCAMutexAccess);
      	
        semaphore_signal(psemMutexTemp);
    
	 return (StatusMgDdi);

 }


/*******************************************************************************
 *Function name: CHCA_MutexInit
 *           
 * 
 *Description:  init the mutex module
 *         
 *   
 *Prototype:
 *     
 *           CHCA_BOOL   CHCA_MutexInit( void)
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
 *        false: init successfully
 *        true: fail to init the mutex module
 *
 *Comments:
 *
 *******************************************************************************/
CHCA_BOOL   CHCA_MutexInit( void)
 {                                                               
        CHCA_BOOL                        bErrCode=false; 
        CHCA_UINT                         iMutexIndex;

        pSemCHCAMutexAccess = semaphore_create_fifo(1);  

        if(pSemCHCAMutexAccess==NULL)
        {
                 bErrCode = true;
		   do_report(severity_info,"\nfail to chca mutex Init\n");		 
	 }

	 for(iMutexIndex=0;iMutexIndex<CHCA_MUTEX_MAX_NUM;iMutexIndex++)	
	 {
                MutexInstance[iMutexIndex].Locked = false;
		  MutexInstance[iMutexIndex].InUsed = false;
		  MutexInstance[iMutexIndex].psemMutex = NULL;
		  MutexInstance[iMutexIndex].MutexHandle = 0;
	 }
	 
        return  bErrCode; 
 }


