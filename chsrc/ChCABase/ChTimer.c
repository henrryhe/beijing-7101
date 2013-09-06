#ifdef  TRACE_CAINFO_ENABLE
#define  CHTIMER_DRIVER_DEBUG
#endif

/******************************************************************************
*
* File : ChTimer.C
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
* opyright: Changhong 2004 (c)
*
*****************************************************************************/
#include   "ChTimer.h"


/*timer*/
semaphore_t                           *pSemCHCATimerAccess;
CHCA_Timer_t                         TimerInstance[CHCA_MAX_NO_TIMER];

static  task_t                       *ptidCHCATimerProcessTaskId;/* 060118 xjp change from *ptidCHCATimerProcessTaskId to ptidCHCATimerProcessTaskId  for adopt task_init()*/
const  CHCA_INT                      CHCA_TIMER_PROCESS_PRIORITY                         =  5;  /*20060622 change from 7 to 5*/
const  CHCA_INT                      CHCA_TIMER_PROCESS_WORKSPACE                     = (1024*15);
#if 1/*060117 xjp add for task_create() change into task_init()*/
	static void* g_CHCATimerProcessStack;
	static tdesc_t g_CHCATimerProcessTaskDesc;
#endif


extern    CHCA_BOOL              TimerDataProcessing; /*add this on 050521*/
extern     CHCA_BOOL              CardContenUpdate; /*add this on 050521*/


/*extern CHCA_USHORT              CatTimerHandle;delete this on 050311*/
/*******************************************************************************
 *Function name : CHCA_GetCurrentDate
 *           
 * 
 *Description: This function is used to recover the current date and time of the 
 *             STB returned in the data provided at the address given in the pTime
 *             parameter 
 * 
 *Prototype:
 *     void CHCA_GetCurrentDate(CHCA_TDTDATE *MyDate)
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *           
 *Return code
 *
 *Comments:
 *
 *
*******************************************************************************/ 
void CHCA_GetCurrentDate(CHCA_TDTDATE *MyDate)
{	
#ifdef CH_IPANEL_MGCA

int year;
int month;
int date;
int hour;
int min;
int sec; 
extern void ipanel_cam_get_utc_time(int *year, int *month, int *date, int *hour, int *min, int *sec);
ipanel_cam_get_utc_time(&year,&month,&date,&hour,&min,&sec);
MyDate->sYear = year;
MyDate->ucMonth = month;
MyDate->ucDay = date;


#endif
}


/*******************************************************************************
 *Function name : CHCA_GetTimeInfo
 *           
 * 
 *Description: This function is used to recover the current date and time of the 
 *             STB returned in the data provided at the address given in the pTime
 *             parameter 
 * 
 *Prototype:
 *     CHCA_DDIStatus  CHCA_GetTimeInfo(time_t*   pTime)      
 *           
 *
 *
 *input:
 *     pTime:    Address of time data.      
 *           
 *
 *output:
 *           
 *           
 *           
 *Return code
 *     CHCADDIOk:     Execution corrent
 *     CHCADDIError:  Interface execution error
 *
 *Comments:
 *
 *
 *
*******************************************************************************/ 
void CHCA_GetCurrentTime(CHCA_TDTTIME *MyTime)
{
#ifdef CH_IPANEL_MGCA
 int year;
int month;
int date;
int hour;
int min;
int sec; 
extern void ipanel_cam_get_utc_time(int *year, int *month, int *date, int *hour, int *min, int *sec);
ipanel_cam_get_utc_time(&year,&month,&date,&hour,&min,&sec);

MyTime->ucHours = hour;
MyTime->ucMinutes = min;
MyTime->ucSeconds = date;
#else
        GetCurrentTime((TDTTIME *)MyTime);
#endif
}




/*******************************************************************************
 *Function name : CHCA_GetTimeInfo
 *           
 * 
 *Description: This function is used to recover the current date and time of the 
 *             STB returned in the data provided at the address given in the pTime
 *             parameter 
 * 
 *Prototype:
 *     CHCA_DDIStatus  CHCA_GetTimeInfo(time_t*   pTime)      
 *           
 *
 *
 *input:
 *     pTime:    Address of time data.      
 *           
 *
 *output:
 *           
 *           
 *           
 *Return code
 *     CHCADDIOk:     Execution corrent
 *     CHCADDIError:  Interface execution error
 *
 *Comments:
 *
 *
 *
*******************************************************************************/ 
#ifndef  NAGRA_PRODUCE_VERSION /*DEVELOPMENT VERSION*/
CHCA_DDIStatus  CHCA_GetTimeInfo(time_t*   pTime)
{ 
	 struct tm                    usr_set_tm;
	 time_t                         uCount;
	 CHCA_UINT                 i;

	 CHCA_UINT                aiCheckYear [ 12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	 /*static  time_t                tt=0;   */

	 CHCA_TDTTIME           MgCurrent_Time;
	 CHCA_TDTDATE          MgCurrent_Date;

	 TMGDDIStatus             StatusMgDdi = CHCADDIOK;

        if(pTime==NULL)
        {
               StatusMgDdi = CHCADDIError;
		 return StatusMgDdi;   
	 }

#if 0
        CHCA_GetCurrentDate(&MgCurrent_Date); 
        CHCA_GetCurrentTime(&MgCurrent_Time);
#endif		


#if   0
        if(MgCurrent_Date.sYear==0)
        {
                usr_set_tm.tm_year = 0;
	 }
	 else	
	 {
	     usr_set_tm.tm_year = MgCurrent_Date.sYear-1900;
	 }
	 
	 usr_set_tm.tm_mon = MgCurrent_Date.ucMonth;
	 usr_set_tm.tm_mday = MgCurrent_Date.ucDay;
	 
	 if(MgCurrent_Date.ucWday==7)
	 {
	        usr_set_tm.tm_wday = 0;
	 }
	 else
	 {
               usr_set_tm.tm_wday = MgCurrent_Date.ucWday;
	 }
#else
      	 usr_set_tm.tm_year = 2005-1900;
	 usr_set_tm.tm_mon = 01;
	 usr_set_tm.tm_mday = 01;
	 usr_set_tm.tm_wday= 14;
#endif


#if 0
	 usr_set_tm.tm_hour = MgCurrent_Time.ucHours;
	 usr_set_tm.tm_min = MgCurrent_Time.ucMinutes;
	 usr_set_tm.tm_sec = MgCurrent_Time.ucSeconds;
#else
	 usr_set_tm.tm_hour = 8;
	 usr_set_tm.tm_min = 0;
	 usr_set_tm.tm_sec = 0;
#endif	 

        usr_set_tm.tm_yday = 0;	


	/***計算潤年*/
	
	if ((( MgCurrent_Date.sYear % 4 == 0) && ( MgCurrent_Date.sYear % 100 != 0)) || ( MgCurrent_Date.sYear % 400 == 0)) 
		aiCheckYear[1] = 29;
	
	if(usr_set_tm.tm_mon>0 && usr_set_tm.tm_mon<12)
	{
              for(i=0;i<usr_set_tm.tm_mon-1;i++)
              {
                     usr_set_tm.tm_yday = usr_set_tm.tm_yday + aiCheckYear[i];
	       }
	}

	usr_set_tm.tm_yday = usr_set_tm.tm_yday + MgCurrent_Date.ucDay;
  
	usr_set_tm.tm_isdst = 0;

	uCount = mktime(&usr_set_tm);

#if   0
            *pTime = tt;
             tt= tt+6;
#else
	 
	 *pTime = uCount;

#endif

#ifdef   CHTIMER_DRIVER_DEBUG
	 do_report(severity_info,"\n[CHCA_GetTimeInfo==>] pTime[%d] StatusMgDdi[%d]\n",uCount,StatusMgDdi);
#endif

        return StatusMgDdi;

 } 

#else
CHCA_DDIStatus  CHCA_GetTimeInfo(time_t*   pTime)
{ 
	 struct tm                    usr_set_tm;
	 time_t                         uCount;
	 CHCA_UINT                 i;

	 CHCA_UINT                aiCheckYear [ 12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	 /*static  time_t                tt=0;   */

	 CHCA_TDTTIME           MgCurrent_Time;
	 CHCA_TDTDATE          MgCurrent_Date;

	 TMGDDIStatus             StatusMgDdi = CHCADDIOK;

        if(pTime==NULL)
        {
               StatusMgDdi = CHCADDIError;
		 return StatusMgDdi;   
	 }

        CHCA_GetCurrentDate(&MgCurrent_Date); 
        CHCA_GetCurrentTime(&MgCurrent_Time);


#if   1
        if(MgCurrent_Date.sYear==0)
        {
                usr_set_tm.tm_year = 0;
	 }
	 else	
	 {
	     usr_set_tm.tm_year = MgCurrent_Date.sYear-1900;
	 }
	 
	 usr_set_tm.tm_mon = MgCurrent_Date.ucMonth;
	 usr_set_tm.tm_mday = MgCurrent_Date.ucDay;
	 
	 if(MgCurrent_Date.ucWday==7)
	 {
	        usr_set_tm.tm_wday = 0;
	 }
	 else
	 {
               usr_set_tm.tm_wday = MgCurrent_Date.ucWday;
	 }
#else
      	 usr_set_tm.tm_year = 2004-1900;
	 usr_set_tm.tm_mon = 7;
	 usr_set_tm.tm_mday = 27;
#endif

	 usr_set_tm.tm_hour = MgCurrent_Time.ucHours;
	 usr_set_tm.tm_min = MgCurrent_Time.ucMinutes;
	 usr_set_tm.tm_sec = MgCurrent_Time.ucSeconds;

        usr_set_tm.tm_yday = 0;	


	/***計算潤年*/
	
	if ((( MgCurrent_Date.sYear % 4 == 0) && ( MgCurrent_Date.sYear % 100 != 0)) || ( MgCurrent_Date.sYear % 400 == 0)) 
		aiCheckYear[1] = 29;
	
	if(usr_set_tm.tm_mon>0 && usr_set_tm.tm_mon<12)
	{
              for(i=0;i<usr_set_tm.tm_mon-1;i++)
              {
                     usr_set_tm.tm_yday = usr_set_tm.tm_yday + aiCheckYear[i];
	       }
	}

	usr_set_tm.tm_yday = usr_set_tm.tm_yday + MgCurrent_Date.ucDay;
  
	usr_set_tm.tm_isdst = 0;

	uCount = mktime(&usr_set_tm);

#if   0
            *pTime = tt;
             tt= tt+6;
#else
	 
	 *pTime = uCount;

#endif

#ifdef   CHTIMER_DRIVER_DEBUG
	 do_report(severity_info,"\n[CHCA_GetTimeInfo==>] pTime[%d] StatusMgDdi[%d]\n",uCount,StatusMgDdi);
#endif

        return StatusMgDdi;

 } 
#endif


/*******************************************************************************
 *Function name: CHCA_GetCurrentClock
 *           
 *
 *Description:  get the clock of the system 
 *             
 *             
 *
 *Prototype:
 *        CHCA_TICKTIME CHCA_GetCurrentClock(void)
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
 *********************************************************************************/
CHCA_TICKTIME CHCA_GetCurrentClock(void)
{
        clock_t      ticktime;

	 ticktime = time_now();

	 return (CHCA_TICKTIME)ticktime;

}

/*******************************************************************************
 *Function name: CHCA_GetDiffClock
 *           
 *
 *Description:  get the different between two clock
 *             
 *             
 *
 *Prototype:
 *        CHCA_TICKTIME CHCA_GetDiffClock(CHCA_TICKTIME  TimeEnd, CHCA_TICKTIME  TimeStart)
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
 *********************************************************************************/
CHCA_TICKTIME CHCA_GetDiffClock(CHCA_TICKTIME  TimeEnd, CHCA_TICKTIME  TimeStart)
{
        clock_t      diff_time;

	 diff_time = time_minus((clock_t)TimeEnd, (clock_t)TimeStart); 

	 return (CHCA_TICKTIME)diff_time;

}


/*******************************************************************************
 *Function name: CHCA_GetDiffClock
 *           
 *
 *Description:  get the different between two clock
 *             
 *             
 *
 *Prototype:
 *        CHCA_TICKTIME CHCA_GetDiffClock(CHCA_TICKTIME  TimeStart)
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
 *********************************************************************************/
CHCA_TICKTIME CHCA_ClockADD(CHCA_ULONG  DelayTimeMs)
{
        clock_t      delay_time;

	 delay_time = time_plus(time_now(),(clock_t)(CHCA_GET_TICKPERMS*DelayTimeMs));

	 return delay_time;

}




/*******************************************************************************
 *Function name: CHCA_TimerPause
 *           
 *
 *Description:  pause the timer
 *             
 *             
 *
 *Prototype:
 *        CHCA_BOOL CHCA_TimerPause(ChCA_Timer_Handle_t   timerhandle)
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
 *********************************************************************************/
CHCA_BOOL CHCA_TimerPause(ChCA_Timer_Handle_t   timerhandle)
{
        CHCA_BOOL             bErrCode = false;

	 CHCA_UINT              TimerIndex;

	 if((timerhandle==0) || (timerhandle>=CHCA_MAX_NO_TIMER))
	 {
	         bErrCode = true;
#ifdef      CHTIMER_DRIVER_DEBUG
                do_report(severity_info,"\n[CHCA_TimerPause==>]Timer handle is wrong\n");
#endif
                return bErrCode;
	 }

	 TimerIndex = timerhandle - 1;

	 semaphore_wait(pSemCHCATimerAccess);
	 if(TimerInstance[TimerIndex].UsedStatus == CHCA_TIMER_RUNNING)
	     TimerInstance[TimerIndex].UsedStatus = CHCA_TIMER_PAUSED	;
	 semaphore_signal(pSemCHCATimerAccess);

	 return bErrCode;
		
}


/*******************************************************************************
 *Function name: CHCA_TimerResume
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype: CHCA_BOOL  CHCA_TimerResume(ChCA_Timer_Handle_t  timerhandle)
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
CHCA_BOOL CHCA_TimerResume(ChCA_Timer_Handle_t  timerhandle)
{
        CHCA_BOOL             bErrCode = false;

        CHCA_UINT              TimerIndex;
	
	 if((timerhandle==0)||(timerhandle>=CHCA_MAX_NO_TIMER))
	 {
	         bErrCode = true;
#ifdef      CHTIMER_DRIVER_DEBUG
                do_report(severity_info,"\n[CHCA_TimerPause==>]Timer handle is wrong\n");
#endif
                return bErrCode;
	 }

	 TimerIndex = timerhandle - 1;

	 semaphore_wait(pSemCHCATimerAccess);
	 if(TimerInstance[TimerIndex].UsedStatus == CHCA_TIMER_PAUSED)
	     TimerInstance[TimerIndex].UsedStatus = CHCA_TIMER_RUNNING;
	 semaphore_signal(pSemCHCATimerAccess);

	 return bErrCode;

}


/*******************************************************************************
 *Function name: CHCA_SetTimerOpen
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
 *      ChCA_Timer_Handle_t  CHCA_SetTimerOpen(CHCA_ULONG   DelayTime,CHCA_CALLBACK_FN  hCallback)
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
ChCA_Timer_Handle_t  CHCA_SetTimerOpen(CHCA_ULONG   DelayTime,CHCA_CALLBACK_FN  hCallback)
{
      CHCA_UINT                            TimerIndex,i;
      CHCA_ULONG                         Temp;
	  

      if((DelayTime==0) || (hCallback==NULL))
      {
            do_report(severity_info,"\n[CHCA_SetTimerOpen==>] bad parameter\n");
	     return /*NULL*/0;		
      }

#if  0 /*just for test*/
      TimerInstance[TimerIndex].hTimerHandle = (ChCA_Timer_Handle_t)1;

      return(TimerInstance[TimerIndex].hTimerHandle); 
#endif	  

      semaphore_wait(pSemCHCATimerAccess);
      for(TimerIndex=0;TimerIndex<CHCA_MAX_NO_TIMER;TimerIndex++)
      {
            if(TimerInstance[TimerIndex].CallbackRoutine==hCallback)
            {
#ifndef       NAGRA_PRODUCE_VERSION     
                   if((TimerInstance[TimerIndex].UsedStatus==CHCA_TIMER_RUNNING) || (TimerInstance[TimerIndex].UsedStatus==CHCA_TIMER_PAUSED) )
#else
                   if((TimerInstance[TimerIndex].UsedStatus==CHCA_TIMER_RUNNING)/* || (TimerInstance[TimerIndex].UsedStatus==CHCA_TIMER_PAUSED)*/ )
#endif
                   {
#ifdef   CHTIMER_DRIVER_DEBUG
      		           do_report(severity_info,"\n[CHCA_SetTimerOpen==>] The timer already started,not need to reset the timer\n");
#endif
                         break;
		     }
		     else
		     {
		           do_report(severity_info,"\n[CHCA_SetTimerOpen==>] The timer CHCA_TIMER_IDLE\n");
		     }
	     }
      }

      if(TimerIndex<CHCA_MAX_NO_TIMER)
      {
              /* do_report(severity_info,"\n[CHCA_SetTimerOpen==>]reset a old timer[%d] successfully[%d][%d]\n",TimerIndex,TimerInstance[TimerIndex].TimeDelay,hCallback);*/

#ifndef     NAGRA_PRODUCE_VERSION    
               /*do_report(severity_info,"\n[CHCA_SetTimerOpen==>]reset a old timer[%d] successfully[%d][%d]\n",TimerIndex,TimerInstance[TimerIndex].TimeDelay,hCallback);*/
               TimerInstance[TimerIndex].StartTime = CHCA_GetCurrentClock();
               TimerInstance[TimerIndex].UsedStatus=CHCA_TIMER_RUNNING;
#endif		 
               semaphore_signal(pSemCHCATimerAccess);
	        return(TimerInstance[TimerIndex].hTimerHandle); 		
      }
	  
      for(TimerIndex=0;TimerIndex<CHCA_MAX_NO_TIMER;TimerIndex++)
      {
             if(TimerInstance[TimerIndex].UsedStatus==CHCA_TIMER_IDLE)
		  break;	 	
      }

      if(TimerIndex>=CHCA_MAX_NO_TIMER) 
      {
             do_report(severity_info,"\n[CHCA_SetTimerOpen::] NO Timer Resource\n");
             semaphore_signal(pSemCHCATimerAccess);
             return /*NULL*/0;
      }		   

      /*CatTimerHandle = TimerIndex;delete this on 050311*/
	

#if 1
#ifndef    NAGRA_PRODUCE_VERSION 	

     Temp= 5000 * 0.001; /*add this just for test*/
#else
     Temp= DelayTime * 0.001;
#endif

#else
/*20070115 add 为了加快授权处理速度，强制刷新速度小于5秒*/
if(DelayTime>5000)
{
      do_report(0,"DelayTime=%d\n",DelayTime);
	DelayTime=5000;
}
     Temp= DelayTime * 0.001;


#endif


      TimerInstance[TimerIndex].TimeDelay  =  DelayTime;
      TimerInstance[TimerIndex].CallbackRoutine = hCallback; 
      /*do_report(severity_info,"\n[CHCA_SetTimerOpen==>]Start a new timer TimeDelay [%d] \n",Temp);*/

      TimerInstance[TimerIndex].TimeOut =  (CHCA_ULONG)ST_GetClocksPerSecond()*Temp;
      TimerInstance[TimerIndex].StartTime = CHCA_GetCurrentClock();
      	  
      TimerInstance[TimerIndex].UsedStatus = CHCA_TIMER_RUNNING;
      TimerInstance[TimerIndex].hTimerHandle = TimerIndex+1;
 	  
#ifdef   CHTIMER_DRIVER_DEBUG
      do_report(severity_info,"\n[CHCA_SetTimerOpen==>]Start a new timer[%d] successfully[%d][%x] Timeout[%d]\n",TimerIndex,TimerInstance[TimerIndex].TimeDelay,hCallback,TimerInstance[TimerIndex].TimeOut);
#endif
	  
      semaphore_signal(pSemCHCATimerAccess);
	  
      return(TimerInstance[TimerIndex].hTimerHandle); 		
      			 
}

/*******************************************************************************
 *Function name: CHCA_RestartTimer
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
 *      CHCA_BOOL  CHCA_RestartTimer(void)
 *
 *
 *input:
 *     
 *
 *output:
 *           
 *
 *
 *Return code:
 *    
 *
 *Comments:
 *    
 *           
 *******************************************************************************/
CHCA_BOOL  CHCA_RestartTimer(void)
{
      CHCA_BOOL           bReturnCode = false;
      CHCA_UINT            iTimerIndex;	  
 
      semaphore_wait(pSemCHCATimerAccess);	
      for(iTimerIndex=0;iTimerIndex<CHCA_MAX_NO_TIMER;iTimerIndex++)	  
      {
            if(TimerInstance[iTimerIndex].UsedStatus == CHCA_TIMER_RUNNING)	  
            {
                 TimerInstance[iTimerIndex].StartTime = CHCA_GetCurrentClock();
            }
      } 	  
      semaphore_signal(pSemCHCATimerAccess);
	  
      return bReturnCode; 		
}

/*******************************************************************************
 *Function name: CHCA_SetTimerStop
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
 *      CHCA_BOOL  CHCA_SetTimerStop(CHCA_UINT   iTimerIndex)
 *
 *
 *input:
 *     
 *
 *output:
 *           
 *
 *
 *Return code:
 *    
 *
 *Comments:
 *    
 *           
 *******************************************************************************/
CHCA_BOOL  CHCA_SetTimerStop(CHCA_UINT   iTimerIndex)
{
      CHCA_BOOL           bReturnCode = false;
 
      if(iTimerIndex>=CHCA_MAX_NO_TIMER) 
      {
             bReturnCode = true;
             return bReturnCode;
      }		

      semaphore_wait(pSemCHCATimerAccess);	  
      if(TimerInstance[iTimerIndex].UsedStatus == CHCA_TIMER_RUNNING)	  
      {
           TimerInstance[iTimerIndex].TimeDelay  =  0;
           TimerInstance[iTimerIndex].CallbackRoutine = NULL; 	
           TimerInstance[iTimerIndex].TimeOut =  0;
           TimerInstance[iTimerIndex].StartTime = 0;
           TimerInstance[iTimerIndex].UsedStatus = CHCA_TIMER_IDLE;
           TimerInstance[iTimerIndex].hTimerHandle =/*NULL*/0;
      }
      semaphore_signal(pSemCHCATimerAccess);
	  
      return bReturnCode; 		
}



/*******************************************************************************
 *Function name: CHCA_SetTimerCancel
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
 *      CHCA_DDIStatus  CHCA_SetTimerCancel(ChCA_Timer_Handle_t   hTHandle)
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
CHCA_DDIStatus  CHCA_SetTimerCancel(ChCA_Timer_Handle_t   hTHandle)
 {
      CHCA_DDIStatus                      StatusMgDdi= CHCADDIOK;
      CHCA_UINT                              TimerIndex;

      TimerIndex = hTHandle - 1;

      if((hTHandle==/*NULL*/0)||(TimerIndex>=CHCA_MAX_NO_TIMER)) 
      {
             StatusMgDdi = CHCADDIBadParam;
             do_report(severity_info,"\n[MGDDIDelayCancel::] Handle unknown\n");
             return StatusMgDdi;
      }		

      semaphore_wait(pSemCHCATimerAccess);	  
      if(TimerInstance[TimerIndex].UsedStatus != CHCA_TIMER_RUNNING)	  
      {
           TimerInstance[TimerIndex].TimeDelay  =  0;
           TimerInstance[TimerIndex].CallbackRoutine = NULL; 	
           TimerInstance[TimerIndex].TimeOut =  0;
           TimerInstance[TimerIndex].StartTime = 0;
           TimerInstance[TimerIndex].UsedStatus = CHCA_TIMER_IDLE;
           TimerInstance[TimerIndex].hTimerHandle =/*NULL*/0;
      }
      semaphore_signal(pSemCHCATimerAccess);
	  
      return StatusMgDdi; 		
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
static void  CHCA_TimerProcess(void *pvParam)
{
        CHCA_UCHAR               TimerIndex;
        CHCA_TICKTIME           EndTime;
	 CHCA_TICKTIME           diff_time;	
        while(true)
        {
		    for(TimerIndex = 0; TimerIndex < CHCA_MAX_NO_TIMER; TimerIndex++)
		    {
		          semaphore_wait(pSemCHCATimerAccess); 
                         if(TimerInstance[TimerIndex].UsedStatus==CHCA_TIMER_RUNNING)
			    {
			          EndTime = CHCA_GetCurrentClock();
                              /* semaphore_wait(pSemCHCATimerAccess); 20070711 move to up*/
				   diff_time =CHCA_GetDiffClock(EndTime, TimerInstance[TimerIndex].StartTime);
				   /*do_report(severity_info,"\ndiff_time[%d] TimerInstance[TimerIndex].TimeOut[%d]\n",diff_time,TimerInstance[TimerIndex].TimeOut);*/
				   if(diff_time >= TimerInstance[TimerIndex].TimeOut)
                               {
                                      /*if(TimerIndex==2)
                                      {
                                           do_report(severity_info,"\n[chca_timerprocess==>]StartTime[%d] Index[%d] Timeout[%d]EndTime[%d] Diff_time[%d]\n",
                                                                           TimerInstance[TimerIndex].StartTime,
                                                                           TimerIndex,
                                                                           TimerInstance[TimerIndex].TimeOut,
                                                                           EndTime,
                                                                           diff_time);
                                       }	*/				  
#ifndef                          NAGRA_PRODUCE_VERSION  
         
                                      /*pause the timer*/
                                      TimerInstance[TimerIndex].UsedStatus = CHCA_TIMER_PAUSED;
                                      /*TimerInstance[TimerIndex].TimeDelay  =  0;*/
                                      /*TimerInstance[TimerIndex].TimeOut =  0;*/
                                      /*TimerInstance[TimerIndex].StartTime = 0; */
#else
                                      TimerInstance[TimerIndex].StartTime = CHCA_GetCurrentClock();  
#endif
                                      CHCA_SendTimerInfo2CAT((CHCA_USHORT)TimerIndex,CHCADDIEvSrcDmxFltrTimeout); 
                                     do_report(severity_info,"\n[CHCA_TimerProcess==>] Time[%d] timeout\n",TimerIndex); /**/
 				   }
				  /* semaphore_signal(pSemCHCATimerAccess);*/
			     }
			   semaphore_signal(pSemCHCATimerAccess);
			    			 
		    }

		    /*CHCA_WAIT_N_MS(1);  wait for 1ms*/

		    task_delay(ST_GetClocksPerSecond()*0.5); /*from 1ms to 200ms on 050319*/	
        }
} 



/*******************************************************************************
 *Function name: CHCA_TimerOperation
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype: CHCA_BOOL  CHCA_TimerOperation(CHCA_USHORT    iTimer)
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
CHCA_BOOL  CHCA_TimerOperation(CHCA_USHORT    iTimer)
{
        CHCA_BOOL                                       bReturnCode=false;
        ChCA_Timer_Handle_t                         hTimer;
        CHCA_UCHAR                                     EventCode;
        CHCA_ULONG                                     Delay;	 

        CHCA_CALLBACK_FN                          CallbackFunTemp=NULL;

        if(iTimer>=CHCA_MAX_NO_TIMER)
        {
              do_report(severity_info,"\n[CHCA_TimerOperation] the timer handle is wrong\n");
		bReturnCode = true; 	  
	       return bReturnCode;		   
        }

        semaphore_wait(pSemCHCATimerAccess);
  	 EventCode = MGDDIEvDelay;		
        hTimer = TimerInstance[iTimer].hTimerHandle; 
	 Delay = TimerInstance[iTimer].TimeDelay;	

	 CallbackFunTemp = TimerInstance[iTimer].CallbackRoutine;
        semaphore_signal(pSemCHCATimerAccess);

#ifdef   CHTIMER_DRIVER_DEBUG
        do_report(severity_info,"\n[CHCA_TimerOperation==>] hTimer[%d] TimerIndex[%d] Delay[%d] Callback[%4x]\n",hTimer,iTimer,Delay,CallbackFunTemp);
#endif 


        if(CallbackFunTemp!=NULL)
        {
     #ifdef CH_IPANEL_MGCA
/*{
#include "chcard.h"
extern SCOperationInfo_t                                                  SCOperationInfo[];
extern CHCA_UINT                                                            MgCardIndex ;
extern boolean gb_ForceUpdateSmartMirror;

        TMGAPIStatus                        iAPIStatus; 
        TMGAPICardHandle                hApiCardHandle=NULL;

        hApiCardHandle = SCOperationInfo[MgCardIndex].iApiCardHandle;       
	//  semaphore_wait(pSemMgApiAccess); 
	
        if(hApiCardHandle != NULL)
	 {
	   gb_ForceUpdateSmartMirror=true;
           iAPIStatus = MGAPICardUpdate(hApiCardHandle);
	      do_report(0,"\n[EMM UPdate==>] MGAPICardUpdate\n");		 
        }	


}*/
#endif
   
               semaphore_wait(pSemMgApiAccess);
		 TimerDataProcessing=true;/*add this on 050521*/
	        CallbackFunTemp((HANDLE)hTimer,(byte)EventCode,NULL,(dword)Delay);
	        CHCA_CaDataOperation();/*add this on 050311*/
		 TimerDataProcessing=false;/*add this on 050521*/

#ifdef     CHTIMER_DRIVER_DEBUG		 
               if(CardContenUpdate)	
               {
                     do_report(severity_info,"\n[CHCA_TimerOperation==>]timer update the mirror image of the card\n");
               }	
#endif			   
		 
		 CHMG_CaContentUpdate();	
		 semaphore_signal(pSemMgApiAccess);	
        }	   
		
	 return bReturnCode;

}



/*******************************************************************************
 *Function name: CHCA_TimerInit
 *           
 * 
 *Description:  init the timer module
 *         
 *   
 *Prototype:
 *     
 *            CHCA_BOOL CHCA_TimerInit(void)
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
 *        true: fail to init the timer module
 *
 *Comments:
 *
 *******************************************************************************/
 CHCA_BOOL CHCA_TimerInit(void)
{
        CHCA_BOOL          bErrCode = false;
        CHCA_USHORT      TimerIndex;		
#if 1/*20070721 move to here*/
        pSemCHCATimerAccess=semaphore_create_fifo(1);

        /*init the timer structure*/
        for(TimerIndex=0;TimerIndex<CHCA_MAX_NO_TIMER;TimerIndex++)
        {
             TimerInstance[TimerIndex].CallbackRoutine = NULL; 	
             TimerInstance[TimerIndex].UsedStatus = CHCA_TIMER_IDLE;
             TimerInstance[TimerIndex].TimeDelay  =  0;
             TimerInstance[TimerIndex].TimeOut =  0;
             TimerInstance[TimerIndex].StartTime = 0;
	 }
#endif		

        /* Create and start the timer task */

#if 1/*060117 xjp change for STi5105 SRAM is small*/
        if ( ( ptidCHCATimerProcessTaskId = Task_Create ( CHCA_TimerProcess,
                                                                                      NULL,
                                                                                      CHCA_TIMER_PROCESS_WORKSPACE,
                                                                                      CHCA_TIMER_PROCESS_PRIORITY,
                                                                                      "chca_timerprocess",
                                                                                      0 ) ) == NULL )
   	 {
              do_report ( severity_error, "CHCA_TimerCreate=> Unable to create MgTimerProcess\n" );
              bErrCode = true;			  
		return(bErrCode);		
	 }
#else
	{		
		int i_Error=-1;


		g_CHCATimerProcessStack = memory_allocate( SystemPartition, CHCA_TIMER_PROCESS_WORKSPACE );
		if( NULL == g_CHCATimerProcessStack )
		{
			do_report ( severity_error, "ChCardInit=> Unable to create CHCA_TimerProcess for no memory\n" );
			return  true;		
		}
		i_Error = task_init( (void(*)(void *))CHCA_TimerProcess, 
										NULL, 
										g_CHCATimerProcessStack, 
										CHCA_TIMER_PROCESS_WORKSPACE, 
										&ptidCHCATimerProcessTaskId, 
										&g_CHCATimerProcessTaskDesc, 
										CHCA_TIMER_PROCESS_PRIORITY, 
										"CHCA_TimerProcess", 
										0 );

		if( -1 == i_Error )

		{
			do_report ( severity_error, "ChCardInit=> Unable to create CHCA_TimerProcess for task init\n" );
			return  true;	
		}


	}
#endif

#if 0/*20070721 move to up*/
        pSemCHCATimerAccess=semaphore_create_fifo(1);

        /*init the timer structure*/
        for(TimerIndex=0;TimerIndex<CHCA_MAX_NO_TIMER;TimerIndex++)
        {
             TimerInstance[TimerIndex].CallbackRoutine = NULL; 	
             TimerInstance[TimerIndex].UsedStatus = CHCA_TIMER_IDLE;
             TimerInstance[TimerIndex].TimeDelay  =  0;
             TimerInstance[TimerIndex].TimeOut =  0;
             TimerInstance[TimerIndex].StartTime = 0;
	 }
#endif		
    return(bErrCode);	
		
} /* MgTimerCreate() */





