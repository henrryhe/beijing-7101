/*! Time-stamp: <@(#)OS21WrapperMessageClock.h   07/04/2005 - 14:18:36   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperMessageClock.h
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
#ifndef __OS21WrapperMessageClock__
#define __OS21WrapperMessageClock__

#ifndef CLOCK_T
#define CLOCK_T
typedef __int64      clock_t;
#endif
#ifndef OSCLOCK_T
#define OSCLOCK_T
typedef clock_t      osclock_t;
#endif

/* OCLOCK definition */
#define FORMAT_SPEC_OSCLOCK ""
#define OSCLOCK_T_MILLE     1000
#define OSCLOCK_T_MILLION   1000000

#ifndef WINCE_USE_INLINE
/* OCLOCK definition */

osclock_t      time_now (void);
int            time_after (osclock_t Time1, osclock_t Time2);
osclock_t      time_minus (osclock_t Time1, osclock_t Time2);
osclock_t      time_plus (osclock_t Time1, osclock_t Time2);
osclock_t      time_ticks_per_sec();

#else

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
__forceinline osclock_t  time_now (void)
{
	volatile __int64 * timer  = (__int64 *)(0xB9230000 + 0x50);
	return (osclock_t)(*timer);

}

#define time_ticks_per_sec() 		  (osclock_t)(90000)
#define  time_after(Time1, Time2)   ((Time1 - Time2) > 0)
#define  time_minus(Time1, Time2) ( Time1 - Time2)
#define  time_plus(Time1, Time2)    ( Time1 + Time2)

#endif



#endif
