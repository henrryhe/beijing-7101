/*****************************************************************************

File name   : Report.C

Description : contains the report functions

COPYRIGHT (C) ST-Microelectronics 1998.


Date          Modification                                    Initials
----          ------------                                    --------
5-oct-98      Ported to 5510                                  FR
*****************************************************************************/
/* Includes --------------------------------------------------------------- */
#include <stdio.h>               /* for print                               */
#include <stdarg.h>              /* for argv argc                           */

#include "stlite.h"                /* OS20 for ever..                         */
#include "report.h"

#ifndef ENABLE_STAPI_ENVIRONMENT
#define ENABLE_STAPI_ENVIRONMENT
#include "sttbx.h"		 /* ensures the compatibility with STTBX 
							All messages printed out with do_report() are directed
							to the OutputDevice_t defined in STTBX_Init()*/
#endif

#define inrange(v,a,b)  (((v)>=(a)) && ((v)<=(b)))

/* Local variables -------------------------------------------------------- */
static int 	        severity_restriction_lower = 0x00000000;
static int 	        severity_restriction_upper = 0x7fffffff;
static char	        report_buffer[256*5];
/*static semaphore_t	report_lock;*/

boolean EnablePrint=true;

/* ========================================================================
   Initialization of level of severities and queue
=========================================================================== */
boolean report_init( void )
{
    severity_restriction_lower	= 0;
    severity_restriction_upper	= 0x7fffffff;
   /* semaphore_init_priority( &report_lock, 1 );*/
    return false;
}

/* ========================================================================
   report : called from everywhere
=========================================================================== */
void do_report( report_severity_t   report_severity,
	     char 		*format, ... )
{
    va_list      list;    
   if(EnablePrint==true)
   	{
    va_start(list, format);
    vsprintf(report_buffer, format, list);
    va_end(list);
    
	#ifdef ENABLE_STAPI_ENVIRONMENT
	STTBX_Print(("%s",report_buffer));
	#else
	printf( report_buffer );
	#endif
}
}
void CH_SetReport(boolean Enable)
{
EnablePrint=Enable;
}
//20061127 change #undef STTBX_Print
//20061127 change  #define STTBX_Print sttbx_Print

