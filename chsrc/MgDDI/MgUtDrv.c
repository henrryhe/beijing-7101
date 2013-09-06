#ifdef    TRACE_CAINFO_ENABLE
#define  MGUTILITY_DRIVER_DEBUG
#endif

/******************************************************************************
*
* File : MgUtilityDrv.C
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
#include "MgUtDrv.h"


#else

#include "MgCardDrv.h"
 
 #include <stdio.h>               /* for print                               */
#include <stdarg.h>              /* for argv argc                           */

#include "stlite.h"                /* OS20 for ever..                         */
#include "report.h"
#include "sttbx.h"	
#endif


/*****************************************************************************
 *Interface description
 *****************************************************************************/


/*******************************************************************************
 *Function name : MGDDIGetTickTime
 *           
 *
 *Description: This function recovers the number of CPU or timer cycles that have
 *             elapsed since the STB was powered up.This value progresses in cycles,
 *             i.e.once the maximum value has been reached,it restarts from 0
 *
 *Prototype:
 *     udword  MGDDIGetTickTime()       
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
 *       Absolute time value from power-up,expressed in CPU or timer cycles.
 *
 *
 *Comments:
 *           
 *******************************************************************************/
 udword  MGDDIGetTickTime(void) 
 {
       CHCA_TICKTIME          iCurTime;
 
       iCurTime = CHCA_GetCurrentClock();

       do_report(severity_info,"\n [MGDDIGetTickTime]  the number[%d] of CPU",iCurTime);

        return  (udword)iCurTime;

 }	  

/*******************************************************************************
 *Function name: MGDDITraceString
 *           
 *
 *Description: This function displays a character string on a debugging peripheral
 *             (if any).The character string ends with a null byte and consists of
 *             a maximum of 255 characters.
 *
 *
 *Prototype:
 *    void   MGDDITraceString(ubyte* pStr)      
 *           
 *
 *
 *input:
 *    pStr:     Character string to be displayed.       
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
 void   MGDDITraceString(ubyte* pStr)
 {
 #if 0
       char String_p[256] = "";

      	va_list  Argument;

	/* Translate arguments into a character buffer */
	va_start(Argument, pStr);
	vsprintf(String_p, (char *)pStr, Argument);
	va_end(Argument);

	do_report(0,String_p);
#else

       CHCA_Report(pStr);
#endif
	  
 }
/*******************************************************************************
 *Function name:MGDDITraceSetAsync
 *           
 *
 *Description: This function switches trace mode from asunchronous to sunchronous
 *             depending on the value of bAsync flag.If FALSE,trace mode is 
 *             synchronous.Otherwise it is asynchronous. 
 *
 *
 *Prototype:
 *      void  MGDDITraceSetAsync(boolean bAsync)       
 *           
 *
 *
 *input:
 *      bAsync:       Indicates if the trace mode is asynchronous.     
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
 void  MGDDITraceSetAsync(boolean bAsync)
 {
      do_report(severity_info,"\n MGDDITraceSetAsync \n");
 }	  

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


