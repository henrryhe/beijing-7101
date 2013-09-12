///
/// @file       STF/Interface/OSAL/OS21/OSSTFTimer.h
///
/// @brief      STFTimer implementation for OS21
///
/// @par OWNER: STCM STF Group
/// @author     Ulrich Mohr
///
/// @par SCOPE: Private implementation file
///
/// @date       2004-04-23 
///
/// &copy; 2004 STMicroelectronics Design and Applications. All Rights Reserved.
///


// Includes
#ifndef OSSTFTIMER_H
#define OSSTFTIMER_H

#include "STF/Interface/Types/STFBasicTypes.h"
#include "STF/Interface/Types/STFResult.h"
#include "STF/Interface/Types/STFInt64.h"
#include "STF/Interface/Types/STFTime.h"
#include "STF/Interface/STFMutex.h"
#include <ostime.h>

extern uint32	OS20_SystemTime_Overflow_Counter;
extern uint32	OS20_Last_SystemTime;
extern uint32  OS20_HighResTicksPerMillisec;


class OSSTFTimer
	{
	protected:
		STFMutex				mutex;
		
		/// overflow_counter and last_time are needed for 64bit timer support on OS20					
	public:		
		virtual ~OSSTFTimer() {};

		void GetTime(STFHiPrec64BitTime & restime)
			{
			interrupt_lock();
			clock_t time = time_now();
			
			if (OS20_Last_SystemTime > time)	
				OS20_SystemTime_Overflow_Counter++;
			
			OS20_Last_SystemTime = (uint32)time;
			interrupt_unlock();

			restime = STFHiPrec64BitTime(STFInt64(time, OS20_SystemTime_Overflow_Counter), STFTU_HIGHSYSTEM);
			}

		 
		STFResult WaitDuration(const STFLoPrec32BitDuration & duration) 
			{
			STFInt64	timeToWait = duration.Get64BitDuration(STFTU_HIGHSYSTEM);
			clock_t	delay;
			delay = 0xffff;	//maximum amount of time
			
			if (duration < STFLoPrec32BitDuration(0,STFTU_HIGHSYSTEM)) STFRES_RAISE(STFRES_INVALID_PARAMETERS);
			
			while (timeToWait.Upper() > 0)
				{
				timeToWait =- delay;
				task_delay(delay);
				}
			delay = timeToWait.Lower();
			task_delay(delay);

			STFRES_RAISE_OK;
			}		

		STFResult WaitIdleTime(uint32 micros)
			{
			STFRES_RAISE(STFRES_UNIMPLEMENTED);
			}		
	};




STFResult InitializeOSSTFTimer(void);

#endif //OSSTFTIMER_H
