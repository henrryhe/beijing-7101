/************************************************************************
COPYRIGHT (C) STMicroelectronics 1999

source file name : clocktst.c

Test harness for the CPU clock functions in stclock.c

************************************************************************/

/* USE THESE #defines TO BUILD DIFFERENT 
FUNCTIONALITY INTO THE TESTS PROGRAM */

#include <stdio.h>
#include "stsys.h"

#include "stddefs.h"
#include "stcommon.h"
#include "stlite.h"
#include "stdevice.h"

#ifndef DISABLE_TOOLBOX
#include "sttbx.h"
#endif

/* Interrupt control structure */

#ifdef ARCHITECTURE_ST20
typedef struct
{
    char                        *stack_base;
    size_t                      stack_size;
    interrupt_flags_t           flags;
} local_interrupt_control_t;
#endif

#ifdef DISABLE_TOOLBOX
#define Printf(x)       printf x
#else
#define Printf(x)       STTBX_Print(x)
#endif

extern U32 TotalPassed;
extern U32 TotalFailed;

char Clocks[24][15] = { "C2",
                        "STBus",
                        "CommsBlock",
                        "HDDI",
                        "VideoMem",
                        "Video2",
                        "Video3",
                        "AudioDSP",
                        "EMI",
                        "MPX",
                        "Flash",
                        "SDRAM",
                        "PCM",
                        "SmartCard",
                        "Aux",
                        "HPTimer",
                        "LPTimer",
                        "ST40",
                        "ST40Per",
                        "UsbIrda",
                        "PWM",
                        "PCI",
                        "VideoPipe",
                        "ST200"
                      };
                     
/* ST_GetClocksPerSecond reference against which calls
  from multiple threads are compared */ 
U32 ClocksPerSec = 0;

U32 LowPriorityTicks = 0;
U32 HighPriorityTicks = 0;

static void TestPassed(void) {
    Printf(("PASSED\n"));
    TotalPassed++;
}

static void TestFailed(const char *string) {
    Printf(("FAILED: %s\n", string));
    TotalFailed++;
}

/* discrepency allowed between invocations of ST_GetClocksPerSecond() */
#define CLOCK_DISCREP 1

void vClockThread( void* Param )
{
    int i;
    U32 LocalClocksPerSec;
    
    for( i = 0; i < 100; ++i) /* about 3.5 secs */
    {
        LocalClocksPerSec = ST_GetClocksPerSecond();
        
        if((LocalClocksPerSec > ClocksPerSec) ?
            (LocalClocksPerSec - ClocksPerSec > CLOCK_DISCREP) :
            (ClocksPerSec - LocalClocksPerSec > CLOCK_DISCREP))
        {
            Printf(("vClockThread: ST_GetClocksPerSecond returned %u,"
                    " expected %u\n", LocalClocksPerSec, ClocksPerSec));
                         
            *(BOOL*)Param = TRUE; /* return failure */
            break;
        }
    }
}


/* Routines for GetClocksPersSec test from interrupt and task */
#ifdef ARCHITECTURE_ST20
static void GetClocksPerSecLowPriority(void* NotUsed)
{
   LowPriorityTicks  = ST_GetClocksPerSecond();
}

static void GetClocksPerSecHighPriority(void* NotUsed)
{
   HighPriorityTicks = ST_GetClocksPerSecond();
}
#endif

void vClockTest( void )
{
    ST_ClockInfo_t ClockInfo;
    U32 *Info_p, i;
    
    Printf(("**************************************************\n"));
    Printf(("TEST: Clock functionality...\n"));
    Printf(("**************************************************\n"));

    /* Test code for CPU and clock functions */
    Printf(("Clock speed %d\n", ST_GetClockSpeed() ));
    Printf(("Clocks per second   %6u\n", ST_GetClocksPerSecond() ));
    Printf(("Clocks per sec high %6u\n", ST_GetClocksPerSecondHigh() ));
    Printf(("Clocks per sec low  %6u\n", ST_GetClocksPerSecondLow() ));

    if ((ST_GetClockSpeed() == 0) || 
        ((ClocksPerSec = ST_GetClocksPerSecond()) == 0) ||
        (ST_GetClocksPerSecondHigh() == 0) ||
        (ST_GetClocksPerSecondLow() == 0))
    {
        TestFailed("One or more clock values was zero");
    }
    else
    {
        TestPassed();
    }

    ST_GetClockInfo (&ClockInfo);
    Info_p = (U32 *)&ClockInfo;

    for (i=0; i<24; i++)
    {
        Printf(("\n%12s clock: %9u Hz",Clocks[i],*Info_p));
        Info_p++;
    }
    Printf(("\n"));


#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
    {
        /*int i;*/
        U32 PWMtimer=0;
        float Time;

#define PWM_CONTROL         0x50
#define PWM_COUNT           0x60
#define PWM_CAPTURECOUNT    0x64
#define TimerHigh(Value) __optasm{ldc 0; ldclock; st Value;}

        /*for(i=0; i<20; i++)*/
        {
            STSYS_WriteRegDev32LE ((PWM_BASE_ADDRESS + PWM_CONTROL),      0x00000000);
            STSYS_WriteRegDev32LE ((PWM_BASE_ADDRESS + PWM_CAPTURECOUNT), 0x00000000);
            STSYS_WriteRegDev32LE ((PWM_BASE_ADDRESS + PWM_CONTROL),      0x00000600);

            ST_GetClocksPerSecond();
        
            PWMtimer = STSYS_ReadRegDev32LE (PWM_BASE_ADDRESS + PWM_CAPTURECOUNT);
        
            Time = ((float)PWMtimer)/((float)ST_GetClockSpeed());
            Printf(("ST_GetClocksPerSecond() took %d ticks = %fs\n", PWMtimer, Time));
        }
    }
#endif /*#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)*/

#if defined(ST_OS21) || defined(PROCESSOR_C1) || defined(ST_OSWINCE)
    {
    	U32 Time_BeforeCall, Time_AfterCall, NoOfTicks, TickTime;
    	Time_BeforeCall = time_now();
    	NoOfTicks = ST_GetClocksPerSecond();
    	Time_AfterCall = time_now();
    	TickTime = ((Time_AfterCall-Time_BeforeCall) * 1000000) / NoOfTicks;
    	Printf(("ST_GetClocksPerSecond() took approx %d ticks = %d microseconds (rounded off->integer division)\n", (Time_AfterCall-Time_BeforeCall), TickTime));
    }
#endif /*#if defined(ST_OS21) || defined(PROCESSOR_C1) || defined(ST_OSWINCE)*/
    
#if defined(ARCHITECTURE_ST20) /*test not useful for os21, since os21 API used to get value. OS21 will take care of multiple threads*/
    {
        /* launch a number of tasks simultaneously calling ST_GetClocksPerSecond,
          and confirm this doesn't affect the result they receive (DDTS 18116).
          Runnable on all platforms, but really testing 5514/16/17 MeasureTimers */
        
        #define CLOCK_THREADS 8 /* number of threads to launch */
        task_t* Threads[CLOCK_THREADS];
        char ThreadName[] = "ClockThread0";
        U32 ThreadFailed = FALSE; /* set to TRUE by failing vClockThread */
        BOOL ManageFailed = FALSE; /* set to TRUE if task management fails */
        
        Printf(("Launching ST_GetClocksPerSecond() thread-safety test:\n"));
        
        for( i = 0; i < CLOCK_THREADS; ++i )
        {
            ThreadName[sizeof(ThreadName)-2] = '0' + i;
            Threads[i] = task_create(vClockThread, &ThreadFailed, 4*1024, 10, ThreadName, 0);
            if(Threads[i] == NULL)
            {
                Printf(("Failed creating ClockThread %i\n", i));
                ManageFailed = TRUE;
                break;
            }
        }
        
        /* the threads are at lower priority than us,
          so they all start together when we wait: */

        for( i = 0; i < CLOCK_THREADS; ++i )
        {
            if (0 != task_wait(&Threads[i], 1, TIMEOUT_INFINITY))
            {
                Printf(("Error waiting for ClockThread %i\n", i));
               ManageFailed = TRUE;
            }
            
            if (0 != task_delete(Threads[i]))
            {
                Printf(("Error deleting ClockThread %i\n", i));
               ManageFailed = TRUE;
            }
        }
        
        if(ManageFailed)
        {
            TestFailed("Unable to run ClockThread test\n");
        }
        else if(ThreadFailed)
        {
            TestFailed("At least one thread received bad ST_GetClocksPerSecond\n");
        }
        else
        {
            TestPassed();
        }


    }
#endif /*#if defined(ARCHITECTURE_ST20)*/


#ifdef ARCHITECTURE_ST20 /*Test not useful on OS21 since OS21 API is used to get the ticks*/
    /* Test GetClocksPerSecond() from low and high priority interrupts */
    {
        /* Test hijacks level 1 and 2 interrupts. (Level 0 continuously interrupts on 5518)
         * Level 1 interrupt configured as LOW priorty, expect LP ticks returned.
         * Level 2 interrupt configured as HIGH priority, expect HP ticks returned.
         */
        U32 IntId = 0;
        BOOL Failed = FALSE;

        /* For interrupt remapping */
        static char interrupt_level_1_stack[1024];
        static char interrupt_level_2_stack[1024];
        static local_interrupt_control_t interrupt_control[] =
        {
            { interrupt_level_1_stack, 1024, interrupt_flags_low_priority },
            { interrupt_level_2_stack, 1024, interrupt_flags_high_priority }
        };

        /* For task usage */
        task_t GetClocksPerSecTask;
        tdesc_t GetClocksPerSecTaskDescriptor;
        char GetClocksPerSecTaskStack[1024];
        task_t *Task_p;


        Printf(("\nTest ST_GetClocksPerSecond() from low and high priority interrupts and tasks:\n"));

        /* From interrupt context...*/

        /* Setup and use two interrupts of different priority */
        for (i=0;i!=2; i++)
        {
            IntId = i+1; /* Use interrupts 1 and 2. Offset value of array index i */
           
            /* Remapping level 0 and level 1 interrupts */
            if (interrupt_init( IntId,
                                interrupt_control[i].stack_base,
                                interrupt_control[i].stack_size,
                                interrupt_trigger_mode_high_level,
                                interrupt_control[i].flags) )
            {
                TestFailed("interrupt_init");
                Failed = TRUE;
            }

            /* Install interrupt handlers
             * Low on Int 0 on level 0, High on int 1 on level 1.
             */
            if (!Failed)
            {                
                if (i==0) 
                {
                    if (interrupt_install (IntId, IntId, GetClocksPerSecLowPriority, NULL ))
                    {
                        TestFailed("interrupt_install");                    
                        Failed = TRUE;
                    }
                }
                else /* assumes only 2 interrupts */
                {
                    if (interrupt_install (IntId, IntId, GetClocksPerSecHighPriority, NULL ))
                    {
                        TestFailed("interrupt_install");
                        Failed = TRUE;                    
                    }                    
                }
            }

            /* enable interrupt */
            if ((!Failed) && (interrupt_enable(IntId)))
            {
                TestFailed("interrupt_enable");
                Failed = TRUE;                
            }

            /* Fire the interrupt, cause the handler to execute */
            if ((!Failed) && (interrupt_raise_number(IntId)))
            {                
                TestFailed("interrupt_raise");
                Failed = TRUE;                
            }

            /* Job done, clean up.... */
            if ((!Failed) && (interrupt_disable(IntId)))
            {
                TestFailed("interrupt_disable");
                Failed = TRUE;                
            }
            
            if ((!Failed) && (interrupt_uninstall(IntId, IntId)))
            {
                TestFailed("interrupt_disable");
                Failed = TRUE;                
            }  
        }

        /* Expect the correct amount of ticks in the global vars */
        Printf(("Interrupt LowPriorityTicks = %d\n",LowPriorityTicks));
        Printf(("Interrupt HighPriorityTicks = %d\n",HighPriorityTicks));

        if ((LowPriorityTicks != ST_GetClocksPerSecondLow()) ||
            (HighPriorityTicks != ST_GetClocksPerSecondHigh()))
        {
            TestFailed("Number of ticks from interrupt context not as expected");
            Failed = TRUE;
        }

        /* From task context....*/

        /* Clear out globals */
        LowPriorityTicks = 0;
        HighPriorityTicks = 0;

        /* Low priority task */
        task_init( (void(*)(void *))GetClocksPerSecLowPriority, 
                   NULL,
                   GetClocksPerSecTaskStack,
                   1024,
                   &GetClocksPerSecTask,
                   &GetClocksPerSecTaskDescriptor,
                   MAX_USER_PRIORITY, 
                   "LowPriority", 
                   0);

        /* Wait for completion  then kill */
        Task_p = &GetClocksPerSecTask;
        task_wait(&Task_p, 1, TIMEOUT_INFINITY);
        task_delete(Task_p);

        /* Now for high priority */
        task_init( (void(*)(void *))GetClocksPerSecHighPriority, 
                   NULL,
                   GetClocksPerSecTaskStack,
                   1024,
                   &GetClocksPerSecTask,
                   &GetClocksPerSecTaskDescriptor,
                   MAX_USER_PRIORITY, 
                   "HighPriority", 
                   task_flags_high_priority_process);

        /* Wait for completion  then kill */
        Task_p = &GetClocksPerSecTask;
        task_wait(&Task_p, 1, TIMEOUT_INFINITY);
        task_delete(Task_p);        

        /* Expect the correct amount of ticks in the global vars */
        Printf(("Task LowPriorityTicks = %d\n",LowPriorityTicks));
        Printf(("Task HighPriorityTicks = %d\n",HighPriorityTicks));

        if ((LowPriorityTicks != ST_GetClocksPerSecondLow()) ||
            (HighPriorityTicks != ST_GetClocksPerSecondHigh()))
        {
            TestFailed("Number of ticks from interrupt context not as expected");
            Failed = TRUE;
        }

        /* All testing ok? */
        if (!Failed)
        {
            TestPassed();
        }
                   
    }
#endif /*#ifdef ARCHITECTURE_ST20*/


/* Test ability to reconfigure clock speed for *8* devices */
#if defined(ST_5508) || defined(ST_5518) || defined(ST_5580)
    {
        clock_t t;
        U8 i;
        U32 F1[] = { 81000000, 60750000, 48600000, 27000000, 0 };
        U32 F2;
        ST_ErrorCode_t ErrorCode;

        for (i = 0; F1[i] != 0; i++)
        {
            Printf(("Setting clock speed to %d...\n", F1[i]));
            ErrorCode = ST_SetClockSpeed(F1[i]);
            if(ErrorCode != ST_NO_ERROR)
            {
                char Msg[80];
                sprintf(Msg, "ST_SetClockSpeed(%u) returned 0x%x",
                        F1[i], ErrorCode);
                TestFailed(Msg);
            }
            else
            {
                Printf(("Checking clock speed...\n"));
                F2 = ST_GetClockSpeed();
                Printf(("F = %d, TicksL = %d, TickH = %d\n",
                        F2,
                        ST_GetClocksPerSecondLow(),
                        ST_GetClocksPerSecondHigh()));

                /* Check frequency match */
                if (F2 == F1[i])
                {
                    TestPassed();
                }
                else
                {
                    TestFailed("Frequencies do not match");
                }
            }
            
            t = time_now();
            task_delay(ST_GetClocksPerSecond()*5);
            Printf(("Ticks elapsed = %d\n", time_minus(time_now(), t)));
        }
    }
#endif
    
   
}

/* EOF */
