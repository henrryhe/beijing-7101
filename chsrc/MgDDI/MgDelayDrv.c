#ifdef    TRACE_CAINFO_ENABLE
#define  MGDELAY_DRIVER_DEBUG
#endif

/******************************************************************************
*
* File : MgDelayDrv.C
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
 *INCLUDE 
 *****************************************************************************/
#include "MgDelayDrv.h"


/*****************************************************************************
 *VARIABLE  DEFINITIONS
 *****************************************************************************/




/*******************************************************************************
 *Function name: MGDDIDelayStart
 *           
 *
 *Description:This function programs a Timer peripheral and start it.The handle
 *            sent by the function enables the Timer peripheral reserved to be  
 *            identified.The Timer peripheral is programmed with a timeout specified  
 *            in milliseconds by Delay parameter.When this timeout has elapsed,the
 *            callback function hCallback is called through the MGDDIEvDelay event
 *
 *Prototype:
 *     TMGDDITimerHandle  MGDDIDelayStart(udword  Delay,CALLBACK  hCallback)      
 *           
 *
 *
 *input:
 *     Delay:        Timeout to be programmed,in milliseconds.      
 *     hCallback:    Callback function address.    
 *
 *output:
 *           
 *
 *
 *Return code:
 *     Timer peripheral resource handle,if not null
 *     NULL,if a program has occurred during processing.
 *         -  timeout impossible,
 *         -  resource not available, 
 *         -  null callback function address 
 *
 *Comments:
 *           
 *******************************************************************************/
/*modify this on 040901*/
#if 1
TMGDDITimerHandle  MGDDIDelayStart(udword  Delay,CALLBACK  hCallback)
{
      TMGDDITimerHandle           hDdiTimerHandle;

      if((Delay==0) || (hCallback==NULL))
      {
#ifdef  MGDELAY_DRIVER_DEBUG
            do_report(severity_info,"\n[MGDDIDelayStart==>] bad parameter\n");
#endif
	     return NULL;		
      }

      hDdiTimerHandle = (TMGDDITimerHandle)CHCA_SetTimerOpen(Delay,hCallback);

#ifdef  MGDELAY_DRIVER_DEBUG
      do_report(severity_info,"\n[MGDDIDelayStart==>] Start the timer[%d]\n",hDdiTimerHandle);
#endif
	  
      return(hDdiTimerHandle); 		
}
#else /*modify this on 040826*/
TMGDDITimerHandle  MGDDIDelayStart(udword  Delay,CALLBACK  hCallback)
{
      TMGDDITimerHandle      hDdiTimerHandle;
      int                                 TimerIndex;

     /*return 0;*/

      semaphore_wait(&SemTimerAccess);
      for(TimerIndex=0;TimerIndex<MGMAX_NO_TIMER;TimerIndex++)
      {
            if(MgTimer_p[TimerIndex].CallbackRoutine==hCallback)
            {
                   if(MgTimer_p[TimerIndex].UsedStatus==MGTIMER_RUNNING)
                   {
                         break;
		     }
		     else
		     {
		           hDdiTimerHandle = MgTimer_p[TimerIndex].hDdiTimerHandle;
                         semaphore_signal(&SemTimerAccess);
	                  return(hDdiTimerHandle); 
		     }
	     }
      }

      if(TimerIndex<MGMAX_NO_TIMER)
      {
            /*restart the timer*/
	     do_report(severity_info,"\n[MGDDIDelayStart::] Restart the Timer[%d][%d]\n",MgTimer_p[TimerIndex].Delay,hCallback);		
            /*MgTimer_p[TimerIndex].StartTime = time_now();*/
           /* MgTimer_p[TimerIndex].Delay  =  Delay+10000;*/
	     MgTimer_p[TimerIndex].StartTime = time_now();		
            hDdiTimerHandle = MgTimer_p[TimerIndex].hDdiTimerHandle;
            semaphore_signal(&SemTimerAccess);
	   
	     return(hDdiTimerHandle); 		
      }

      for(TimerIndex=0;TimerIndex<MGMAX_NO_TIMER;TimerIndex++)
      {
             if(MgTimer_p[TimerIndex].UsedStatus==MGTIMER_IDLE)
		  break;	 	
      }

      if(TimerIndex>=MGMAX_NO_TIMER) 
      {
             do_report(severity_info,"\n[MGDDIDelayStart::] NO Timer Resource\n");
             semaphore_signal(&SemTimerAccess);
             return 0;
      }		   

      if( Delay<=0 )
      {
            do_report(severity_info,"\n[MGDDIDelayStart::] Dealy is equal to 0\n");
	     semaphore_signal(&SemTimerAccess);		
            return 0;
      }
	  
      MgTimer_p[TimerIndex].Delay  =  Delay-10000;
      MgTimer_p[TimerIndex].CallbackRoutine = hCallback; 	

      MgTimer_p[TimerIndex].TimeOut =  ST_GetClocksPerSecond()*MgTimer_p[TimerIndex].Delay*0.001;
      MgTimer_p[TimerIndex].StartTime = time_now();
      	  
      interrupt_lock();	  
      MgTimer_p[TimerIndex].UsedStatus = MGTIMER_RUNNING;
      interrupt_unlock();

      do_report(severity_info,"\n[MGDDIDelayStart::]Start a new timer[%d][%d]\n",MgTimer_p[TimerIndex].Delay,hCallback);
	  
      hDdiTimerHandle = (TMGDDITimerHandle)(TimerIndex+1);
      MgTimer_p[TimerIndex].hDdiTimerHandle = hDdiTimerHandle; 	   

      semaphore_signal(&SemTimerAccess);
      return(hDdiTimerHandle); 		
      			 
}
#endif

/*******************************************************************************
 *Function name: MGDDIDelayCancel
 *           
 *
 *Description: This function is used to cancel programming of the Timer perpheral
 *             related to the hTimer handle.This handle will have been given by
 *             the MGDDIDelayStart function      
 *
 *Prototype:
 *     TMGDDIStatus  MGDDIDelayCancel(TMGDDITimerHandle   hTimer)      
 *           
 *
 *
 *input:
 *     hTimer:      Timer  peripheral resource handle      
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *    MGDDIOk:        Timer peripheral program cancelled.
 *    MGDDIBadParam:  Handle unknown.
 *    MGDDIError:     Interface execution error
 *
 *Comments:
 *    Any pending event related to the deleted timer program must be canceled           
 *           
 *******************************************************************************/
 TMGDDIStatus  MGDDIDelayCancel(TMGDDITimerHandle   hTimer)
 {
      TMGDDIStatus                    StatusMgDdi= MGDDIOK;
	  
      if(hTimer==NULL) 
      {
             StatusMgDdi = MGDDIBadParam;
#ifdef  MGDELAY_DRIVER_DEBUG			 
             do_report(severity_info,"\n[MGDDIDelayCancel==>] Handle unknown\n");
#endif
             return StatusMgDdi;
      }		

      StatusMgDdi = (TMGDDIStatus)CHCA_SetTimerCancel((ChCA_Timer_Handle_t)hTimer); 
	  
      return StatusMgDdi; 		
 }  


#if 0
/*******************************************************************************
 *Function name: 
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
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
 *Return code:
 *    
 *    
 *    
 *
 *Comments:
 *    
 *           
 *******************************************************************************/
void   ChMgTimerPause(void)
{
        int    TimerIndex;
		
        for(TimerIndex=0;TimerIndex<MGMAX_NO_TIMER;TimerIndex++)
        {
                if(MgTimer_p[TimerIndex].UsedStatus==MGTIMER_RUNNING)
                {
                       semaphore_wait(&SemTimerAccess);
  			  MgTimer_p[TimerIndex].UsedStatus = MGTIMER_PAUSED;		   
		         semaphore_signal(&SemTimerAccess);
		  }
	 }
		
}




/*******************************************************************************
 *Function name: 
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
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
 *Return code:
 *    
 *    
 *    
 *
 *Comments:
 *    
 *           
 *******************************************************************************/
void   ChMgTimerResume(void)
{
        int    TimerIndex; 

        for(TimerIndex=0;TimerIndex<MGMAX_NO_TIMER;TimerIndex++)
        {
                if(MgTimer_p[TimerIndex].UsedStatus==MGTIMER_PAUSED)
                {
                       semaphore_wait(&SemTimerAccess);
			  MgTimer_p[TimerIndex].UsedStatus = MGTIMER_RUNNING;		   
		         semaphore_signal(&SemTimerAccess);
		  }
	 }

}


/*****************************************************************************
stuart_TimerTask()
Description:
    This routine is passed to task_create() when creating a timer task that
    provides the timer/callback functionality.
    The timer task may be in two states:

    TIMER_IDLE: the timer is blocked on the guard semaphore.
    TIMER_RUNNING: the timer is blocked on the timeout semaphore.

    NOTE: A timer may only be deleted when in TIMER_IDLE.

Parameters:
    param,  pointer to task function's parameters - this should be a pointer
            to a Timer_t object.

Return Value:
    Nothing.
See Also:
    stuart_TimerCreate()
    stuart_TimerDelete()
*****************************************************************************/
/*modify this on 040826*/
extern BOOLEAN SendTimerInfo2MGKernel( filter  iFilterId,TMGDDIEventCode  EvCode);

static void  MgTimerProcess(void *pvParam)
{
        int    TimerIndex;

        clock_t                        EndTime;
	 unsigned long int          diff_time;	

        HANDLE                       hTimer;
        byte                            EventCode;
        udword                        Delay;	 

        EventCode = MGDDIEvDelay;	 		
      
		
        for (;;)
        {
		    for(TimerIndex = 0; TimerIndex < MGMAX_NO_TIMER; TimerIndex ++)
		    {
                         /*check the timer*/
			    if(MgTimer_p[TimerIndex].UsedStatus==MGTIMER_RUNNING)
			    {
			          EndTime = time_now();
			          diff_time = time_minus(EndTime, MgTimer_p[TimerIndex].StartTime);
				   /*do_report(severity_info,"\ndiff_time[%d] totaltime[%d]\n",diff_time,MgTimer_p[TimerIndex].TimeOut);	  */
                               if(diff_time >= MgTimer_p[TimerIndex].TimeOut)
                               {
#if 0                               
                                        do_report(severity_info,"\nTimer[%d] time out\n",TimerIndex);
                                        hTimer = (HANDLE)MgTimer_p[TimerIndex].hDdiTimerHandle;
				            Delay = MgTimer_p[TimerIndex].Delay;	 

					     semaphore_wait(pSemMgApiAccess); 							  	
					     if(MgTimer_p[TimerIndex].CallbackRoutine!=NULL)
					     {
					           MgTimer_p[TimerIndex].CallbackRoutine(hTimer,EventCode,NULL,Delay);
						    do_report(severity_info,"\nTimer[%d]CallbackRoutine dealy[%d] \n",TimerIndex,Delay);	   
					     }
					     semaphore_signal(pSemMgApiAccess); 			   

					     semaphore_wait(&SemTimerAccess);			   
					     MgTimer_p[TimerIndex].StartTime = time_now();	
					     semaphore_signal(&SemTimerAccess);	
#else
                                        SendTimerInfo2MGKernel(TimerIndex,MGDDIEvSrcDmxFltrTimeout); 
                                        MgTimer_p[TimerIndex].StartTime = time_now();
#endif
 				   }
							   
		 	    }
		    }

		    task_delay(ST_GetClocksPerSecond()*0.001);		  
        }
} 


void  ChTimerFunction(int   iTimer)
{
        HANDLE                       hTimer;
        byte                            EventCode;
        udword                        Delay;	 

        semaphore_wait(&SemTimerAccess);
        if(MgTimer_p[iTimer].TimerRestart)		
	 { 
	       EventCode = MGDDIEvDelay;		
              hTimer = (HANDLE)MgTimer_p[iTimer].hDdiTimerHandle;
	       Delay = MgTimer_p[iTimer].Delay;	 

	       MgTimer_p[iTimer].TimerRestart = false; 		

              if(MgTimer_p[iTimer].CallbackRoutine!=NULL)
             {
	             MgTimer_p[iTimer].CallbackRoutine(hTimer,EventCode,NULL,Delay);
	             do_report(severity_info,"\nTimer[%d]CallbackRoutine dealy[%d] \n",iTimer,Delay);	   
	       }
         }	   
         semaphore_signal(&SemTimerAccess);
	  return;

}



/*****************************************************************************
stuart_TimerCreate()
Description:
    This routine creates two semaphores and a task, required for operating
    the timer mechanism, as the basis for initializing the user's
    timer structure.
    Once initialized, the user may use their timer with TimerWait,
    TimerSignal.  TimerDelete is used to free up resources tied
    with the timer.
Parameters:
    Timer_p,        pointer to a timer structure to initialize.
    TaskPriority,   OS-specific priority for timer task.
Return Value:
    FALSE,                  timer successfully created.
    TRUE,                   error creating the timer task.
See Also:
    stuart_TimerDelete()
*****************************************************************************/
BOOL MgTimerCreate(void)
{
        BOOL          error = FALSE;
        int               TimerIndex;		


#if 0	
        /* Create the timer idle semaphore required to control the timer task */
        semaphore_init_fifo(&MgTimer_p.GuardSemaphore, 0);

        /* Create the timeout semaphore */
        semaphore_init_fifo_timeout(&MgTimer_p.TimeoutSemaphore, 0);
#endif

       semaphore_init_fifo(&SemTimerAccess,1);

        /* Create and start the timer task */
        if ( ( ptidtimerprocessTaskId = Task_Create ( MgTimerProcess,
                                                                               NULL,
                                                                               MGTIMER_PROCESS_WORKSPACE,
                                                                               MGTIMER_PROCESS_PRIORITY,
                                                                               "MgTimerProcess",
                                                                               0 ) ) == NULL )
   	{
              do_report ( severity_error, "MgTimerCreate=> Unable to create MgTimerProcess\n" );
              error = TRUE;			  
		return(error);		
	}

#if 1
       /*init the timer structure*/
       for(TimerIndex=0;TimerIndex<MGMAX_NO_TIMER;TimerIndex++)
       {
             MgTimer_p[TimerIndex].CallbackRoutine = NULL; 	
             MgTimer_p[TimerIndex].UsedStatus = MGTIMER_IDLE;
             MgTimer_p[TimerIndex].Delay  =  0;
             MgTimer_p[TimerIndex].TimeOut =  0;
             MgTimer_p[TimerIndex].StartTime = 0;
	}
#endif      
       return(error);		
} /* MgTimerCreate() */

#endif

/*******************************************************************************
 *Function name
 *           
 *
 *Description:
 *           
 *
 *Prototype:
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
 *Return code:
 *
 *
 *
 *Comments:
 *           
 *******************************************************************************/


