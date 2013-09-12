///
/// @file STF\Source\OSAL\OS20\OSSTFSemaphore.h
///
/// @brief OS-dependent timer 
///
/// @par OWNER: 
/// STF-Team
/// @author Christian Johner   
///
/// @par SCOPE:
///	INTERNAL Header File
///
/// @date       2003-11-25 changed header
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#include "STF/Interface/STFSynchronisation.h"
#include "OSSTFTimer.h"
#include <ostime.h>


uint32	OS20_SystemTime_Overflow_Counter;
uint32	OS20_Last_SystemTime;
uint32	OS20_HighResTicksPerMillisec;

uint32 ClockMultiplier_SystemTo108;
uint32 ClockMultiplier_108ToSystem; 


//! Helper method to calculate two floats for the 108MHZ clock emulation.
/*! This function calculates the multipliers XXX_108ToSystem which represents 
	 an 14.18 decimal and XXX_SystemTo108 wich is an 18.14 decimal. 
	 If the clock is slower than 0.5 kHz the function will fail.
*/

void Calculate108MhzMultiplier(STFInt64	SystemTicksPerSecond)
	{
	ASSERT(SystemTicksPerSecond > 500);
	STFInt64 emulated_108Mhz_clock = 108000000;
	STFInt64 temp,ticks;
	ticks = SystemTicksPerSecond;
	temp = 0;
	emulated_108Mhz_clock <<= 14;

	temp = emulated_108Mhz_clock / SystemTicksPerSecond;
	ASSERT(temp.Upper() == 0);

	ClockMultiplier_SystemTo108 = temp.Lower();
	
	ticks <<= 18;
	temp = ticks / 108000000;
	ClockMultiplier_108ToSystem = temp.Lower();
	ASSERT(temp.Upper() == 0);
	}

//! This function initializes the timer system and must be called before any timer operation is performed
/*! This function returns STFRES_TIMEOUT when the timer is not accurate enough or an error occured during
    initialization ('out of time' would be a better error value ;) )
*/

STFResult InitializeOSSTFTimer(void)
	{
	clock_t	frequency;
	
	static bool		initialized;

	if (!initialized)
		{
		/// overflow_counter and last_time are needed for 64bit timer support on OS20
		OS20_SystemTime_Overflow_Counter = 0;
		OS20_Last_SystemTime = (uint32)time_now();

		initialized = true;

		//very temp Feb-16-04 please remove and replace with something NOT 5700 specific
//		#define CLOCKS_PER_SEC (99 * 1000000) //very temp //600 MHZ
		#define CLOCKS_PER_SEC ((99 * 1000000)/3)*4 //very temp //800MHZ
		frequency = CLOCKS_PER_SEC;

		OS20_HighResTicksPerMillisec = frequency / 1000;
		
		STFInt64 systemFrequency = STFInt64(CLOCKS_PER_SEC); // has to be changed when define above is removed !!!!

		Calculate108MhzMultiplier(systemFrequency);		
		}

	STFRES_RAISE_OK;
	}
