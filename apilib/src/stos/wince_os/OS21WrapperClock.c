/*! Time-stamp: <@(#)OS21WrapperClock.c   07/04/2005 - 14:21:46   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperClock.c
 *
 *  Project : STAPI Windows CE
 *
 *  Package : 
 *
 *  Company :  TeamLog for ST
 *
 *  Author  : William Hennebois            Date: 07/04/2005
 *
 *  Purpose : Implementation of methods to manage Clock and Time and System Ticks
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  07/04/2005 WH : First Revision
 *
 *********************************************************************
 */


#include "OS21WrapperClock.h"

#ifndef WINCE_USE_INLINE
static	LARGE_INTEGER giFreq = {0}; // store once the nb ticks by sec for optim

/*!-------------------------------------------------------------------------------------
 * time_ticks_per_sec() returns the number of clock ticks per second.
 *
 * @param none
 *
 * @return int time_ticks_per_sec  : 
 */
osclock_t time_ticks_per_sec ()
{
	WCE_MSG(MDL_OSCALL,"-- Calltime_ticks_per_sec");

	// Call once Win32 
	if(giFreq.QuadPart ==0)
	{
		WCE_VERIFY(QueryPerformanceFrequency(&giFreq));
	}
	return (osclock_t)giFreq.QuadPart;
}




/*!-------------------------------------------------------------------------------------
 * time_now() returns the number of ticks since the system started running. The
 *  exact time at which counting starts is implementation specific, but it is no later
 * than the call to kernel_start().
 * The units of ticks is an implementation dependent quantity, but it is in the range of
 * 1 to 10 microseconds.
 *
 * @param void : 
 *
 * @return osclock_t   : 
 */
osclock_t  time_now (void)
{
	LARGE_INTEGER timer;
	WCE_MSG(MDL_OSCALL,"-- time_now()");

	WCE_VERIFY(QueryPerformanceCounter(&timer));
	return (osclock_t)timer.QuadPart;

}


/*!-------------------------------------------------------------------------------------
 * Returns 1 if time1 is after time2, otherwise 0.
 *
 * @param Time1 : 
 * @param Time2 : 
 *
 * @return int time_after  : 
 */
int time_after (osclock_t Time1, osclock_t Time2)
{
	WCE_MSG(MDL_OSCALL,"-- time_after()");

   return ((Time1 - Time2) > 0);
}


/*!-------------------------------------------------------------------------------------
 * Returns the result of subtracting time2 from time1.
 *
 * @param Time1 : 
 * @param Time2 : 
 *
 * @return osclock_t time_minus  : 
 */
osclock_t time_minus (osclock_t Time1, osclock_t Time2)
{
	WCE_MSG(MDL_OSCALL,"-- time_minus()");
	return ( Time1 - Time2);

}


/*!-------------------------------------------------------------------------------------
 * time_plus() adds one clock value to another.
 *
 * @param Time1 : 
 * @param Time2 : 
 *
 * @return osclock_t time_plus  : 
 */
osclock_t time_plus (osclock_t Time1, osclock_t Time2)
{
	WCE_MSG(MDL_OSCALL,"-- time_plus()");

    return Time1 + Time2;

}

#endif /* WINCE_USE_INLINE */
