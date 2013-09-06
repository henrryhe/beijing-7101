#ifdef    TRACE_CAINFO_ENABLE
#define  MGCLOCK_DRIVER_DEBUG
#endif

/******************************************************************************
*
* File : MgClockDrv.C
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
#if 1
#include    "MgClockDrv.h"
#else
#include "MgCardDrv.h"


#include "..\dbase\nvod.h"	/* for tdt time and data struct */

#include <time.h>
#endif

/*****************************************************************************
 *Interface  description
 *****************************************************************************/






/*******************************************************************************
 *Function name : MGDDIGetTime
 *           
 * 
 *Description: This function is used to recover the current date and time of the 
 *             STB returned in the data provided at the address given in the pTime
 *             parameter 
 * 
 *Prototype:
 *     TMGDDIStatus  MGDDIGetTime(time_t*   pTime)      
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
 *     MGDDIOk:     Execution corrent
 *     MGDDIError:  Interface execution error
 *
 *Comments:
 *
 *
 *
 *******************************************************************************/
 TMGDDIStatus  MGDDIGetTime(time_t*   pTime)
 { 
       TMGDDIStatus      StatusMgDdi = MGDDIError;

       if(pTime==NULL)
       {
             do_report(severity_info,"\n[MGDDIGetTime::] the parameter is wrong\n");     
             return StatusMgDdi;
	}

#if 0 /*FOT TEST ON 041122*/
      StatusMgDdi = MGDDIOK;
    
      *pTime = 20;  
#else

       StatusMgDdi = (TMGDDIStatus)CHCA_GetTimeInfo(pTime);
#endif

       return StatusMgDdi;

 } 



