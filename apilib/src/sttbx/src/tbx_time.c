/*******************************************************************************

File name   : tbx_time.c

Description : Source of Time features of toolbox API

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                             Name
----               ------------                             ----
10 Mar 99           Created                                  HG
Modifications : OS21 port
==============================
  Date        : 26/Mar/2004
  DDTS number : None
  Comments    : Modifications for porting over ST40/OS21.
                Modifications active in accordance to definition of
				ARCHITECTURE_ST20 or ARCHITECTURE_ST40.
  Name        : Angelos Hitas
*******************************************************************************/

 #if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sttbx.h"

#ifndef _LONG_LONG
#define _LONG_LONG   long long
#endif

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define WAIT_MICROSECONDS_NUMBER_OF_CYCLES
/*#define WAIT_MICROSECONDS_TIMER_ASM*/
/*#define WAIT_MICROSECONDS_TIMER_OS20*/

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/*
--------------------------------------------------------------------------------
Wait for n microseconds (independently of the CPU frequency)
   duration depends on cpu frequency
   5510 & 5512 : read a register to check frequency
   5508 & 5518 : frequency is 60.75Mhz (here we choose 60Mhz)
--------------------------------------------------------------------------------
*/
#ifdef ST_OS20
void STTBX_WaitMicroseconds(U32 Microseconds)
{
#ifdef WAIT_MICROSECONDS_TIMER_OS20
    U32 ExitingTime, RunningTime;
    /* Very accurate method with high priority timer.
       Problem: how to ensure the timer is started and what to do if not ???
                how to handle external changes of the value of the timer ??? */

    __optasm{
        ldc 0;                  /* 0 for ClockReg[0]: high priority timer */
        ldclock;                /* Load high priority timer */
        st RunningTime;
    }

    ExitingTime = time_plus(RunningTime, Microseconds);

    do
    {
        __optasm{
            ldc 0;                  /* 0 for ClockReg[0]: high priority timer */
            ldclock;                /* Load high priority timer */
            st RunningTime;
        }
    } while (time_after(ExitingTime, RunningTime) != 0);
#elif defined WAIT_MICROSECONDS_TIMER_ASM
    /* Very accurate method with high priority timer.
       Problem: how to ensure the timer is started and what to do if not ???
                how to handle external changes of the value of the timer ??? */

    __optasm{
        ldc 0;                  /* 0 for ClockReg[0]: high priority timer */
        ldclock;                /* Load high priority timer */
        ld Microseconds;        /* Load number of microseconds to wait for */
        sum;                    /* Calculate exiting time */
        stl 0;                  /* Store in local variable */

      loop:
        ldc 0;                  /* 0 for ClockReg[0]: high priority timer */
        ldclock;                /* Load high priority timer */
        ldl 0;                  /* Load exiting time */
        diff;                   /* Compare values */
        ldc 0;                  /* Test sign */
        gt;                     /* Compare result with 0 */
        cj loop;                /* Lopp while exit time not reached */
    }
#elif defined WAIT_MICROSECONDS_NUMBER_OF_CYCLES
    /* No very accurate method, but sure regarding timer problems.
       No ICache would triple the waiting time ! */
    __asm{
                /* Reserve room for 3 variables:
                    -MicrosecondLoopCounter: variable to count microseconds
                    -TenCyclesLoopCounter: variable to count groups of ten cycles
                    -TenMHzCPUFreq: number of groups of 10 cycles to reach 1 microsecond (constant dependant on CPU freq) */
                ajw -3;                                                 /* 2 cy */
                ld Microseconds;                                        /* 2 cy ?? */
                stl 0;      /* MicrosecondLoopCounter */                /* 1 cy */

#if defined(ST_5510) || defined(ST_5512)
                /* Here, load in TenMHzCPUFreq the value: (CPU frequency / 10 MHz)
                   example: CPU frequency = 50 MHz then TenMHzCPUFreq = 5
                   Caution: value should be at least 2, corresponding to 20 MHz.
                   This will enable to reach 1 microsecond precison whatever the CPU frequency:
                   (10 * TenMHzCPUFreq) cycles is exactly 1 microsecond. */
                /* Code below depends on STi5510 ! */
                ldc 0x20000500;     /* Address of clock frequency */    /* 1 cy */
                devlb;              /* Read clock frequency */          /* 3 cy */
                ldc 3;              /* Mask 2 LSB */                    /* 1 cy */
                and;                /* Mask */                          /* 1 cy */
                /* Here, value can be : 0-undefined   1-60MHz   2-40MHz  3-50MHz */
                adc -1;                                                 /* 1 cy */
                stl 2;              /* Save temporarly */               /* 1 cy */
                ldl 2;                                        /* 1 cy */
                cj setfreq60;                                           /* 1 or 5 cy */
                ldl 2;                                        /* 1 cy */
                adc -1;                                       /* 1 cy */
                cj setfreq40;                                 /* 1 or 5 cy */
                ldc 5;                                                  /* 1 cy */
                stl 2;      /* TenMHzCPUFreq 5 for 50 MHz */            /* 1 cy */
                j microsecond_loop;                           /* 5 cy */
            setfreq60:
#endif
                ldc 6;                                                  /* 1 cy */
                stl 2;      /* TenMHzCPUFreq 6 for 60 MHz */            /* 1 cy */
                adc 0;               /* dummy */              /* 1 cy */
                cj microsecond_loop; /* dummy */              /* 1 or 5 cy */
#if defined(ST_5510) || defined(ST_5512)
                j microsecond_loop;                           /* 5 cy */
            setfreq40:
                ldc 4;                                                  /* 1 cy */
                stl 2;      /* TenMHzCPUFreq 4 for 40 MHz */            /* 1 cy */
                j microsecond_loop;
                ldc 0; /*  dummy, never executed, but here to be able to have the jump above */
#endif
                /* Code above depends on STi5510 ! */

            microsecond_loop:
                ldl 0;      /* MicrosecondLoopCounter */                /* 1 cy */
                adc -1;     /* count 1 microsecond */                   /* 1 cy */
                stl 0;      /* MicrosecondLoopCounter */                /* 1 cy */
                ldl 2;      /* TenMHzCPUFreq */                         /* 1 cy */
                adc -2;     /* Substract an offset of 20 cycles */      /* 1 cy */
                stl 1;      /* TenCyclesLoopCounter */                  /* 1 cy */
                ten_cycles_loop:
                    ldl 1;      /* TenCyclesLoopCounter */              /* 1 cy */
                    adc -1;     /* count 10 cycles */                   /* 1 cy */
                    stl 1;      /* TenCyclesLoopCounter */              /* 1 cy */
                    ldl 1;      /* TenCyclesLoopCounter */              /* 1 cy */
                    cj end_ten_cycles;                                  /* 1 or 5 cy */
                j ten_cycles_loop;                                      /* 5 cy */
            end_ten_cycles:
                /* Between microsecond_loop: and end_ten_cycles:, the function waits for
                   (6 + ((TenMHzCPUFreq - 3) * 10) + 5) cy  =>  ((TenMHzCPUFreq - 2) * 10) + 1)  cy,
                   i.e. (1 microsecond - 19 cy) */
                ldl 0;      /* MicrosecondLoopCounter */                /* 1 cy */
                cj end_microseconds;                                    /* 1 or 5 cy */
         ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; /* 9 cy */
            j microsecond_loop;                                         /* 5 cy */
        end_microseconds:
                ajw 3;       /* 3 cy */
    }
#endif
}
#endif /* ST_OS20 */

#ifdef ST_OS21
#include "stcommon.h"
/*
 * Note: This implementation provides good accuracy for delays higher than
 * 20 usec.
 * For delays lower than 1ms, the implementation is too CPU intensive and may
 * stall the CPU.
 * For better accuracy and lower CPU overhead, DV_TIM can be used
 * alternatively.
 */
void STTBX_WaitMicroseconds(U32 Microseconds)
{
#ifdef ST_OSWINCE
    Sleep(Microseconds / 1000); /* the only correct way to wait ! */
#else
    clock_t Target;
    Target = time_plus(time_now(), (double long)(Microseconds) *
					   ST_GetClocksPerSecond() / 1000000);

	if(Microseconds > 1000)
	{
		/*
		 * This method provides a good precision for large delays.
		 */
		task_delay_until(Target);
	} /* if(delay larger than 5ms) */
	else
	{
		/*
		 * This is too CPU intensive. If current task has high priority it may
		 * stall the CPU. The correct way to do this is by using DV_TIM.
		 */
		while(time_after(Target, time_now()));
	}
#endif /* ST_OSWINCE */
}
#endif /* ST_OS21 */

#endif  /* #ifndef ST_OSLINUX */
/* End of tbx_time.c */


