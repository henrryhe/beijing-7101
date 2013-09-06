#define  CHREPORT_DRIVER_DEBUG
/******************************************************************************
*
* File : ChReport.C
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
#include   "ChReport.h"



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
 void   CHCA_Report(CHCA_UCHAR * pStr,...)
 {
       char String_p[512] = "";

      	va_list  Argument;

	if(pStr!=NULL)
	{
              /* Translate arguments into a character buffer */
	       va_start(Argument, pStr);
	       vsprintf(String_p, (char *)pStr, Argument);
	       va_end(Argument);

	       do_report(0,String_p);
	}	
	  
 }



