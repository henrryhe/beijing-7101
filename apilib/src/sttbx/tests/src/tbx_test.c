/*******************************************************************************

File name : tbx_test.c

Description : tests of the toolbox

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                 Name
----               ------------                                 ----
 9 Aug 1999         Created                                      HG
11 Aug 1999         Implement UART input/output                  HG
12 Jan 2000         Added new stboot init param MemorySize       HG
16 Nov 2001         Added new test to check fix of DDTS          HS
 *                  GNBvd09866
30 Jan 2002         Add STTBX_NO_UART compilation flag.          HS
 *                  Move dependencies configuration outside.
 *                  Add redirection and filtering tests.
10 Dec 2002         Add count of tests run                       HS
14 Sep 2004         Add support for OS21                         FQ+MH
*******************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */

#define ANOTHER_TASK

#ifndef STTBX_NO_UART
/*#define TEST_UART_DRIVER*/
#endif /* #ifndef STTBX_NO_UART  */

/*#define TEST_CLOCK_SELECT_PIN*/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

#ifdef ST_OS20
#include "debug.h"
#endif /* ST_OS20 */
#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include "os21debug.h"
#endif /* ST_OS21 */


#include "stdevice.h"
#include "stddefs.h"

#include "startup.h"

#include "stcommon.h"
#ifndef STTBX_NO_UART
#include "cluart.h"
#endif /* #ifndef STTBX_NO_UART  */
#include "sttbx.h"
#include "cltbx.h"
#include "metrics.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define STTBX_NEW_DEVICE_NAME "TBX_NEW_NAME"

/* Private Variables (static)------------------------------------------------ */

#ifdef ANOTHER_TASK
static volatile BOOL AnotherTaskRunning = FALSE;
static volatile BOOL AnotherTaskForStressRunning = FALSE;
#endif

static U32 TicksFor100ms, TicksFor200ms, TicksFor500ms, TicksFor1000ms;

#ifndef STTBX_NO_UART
static const char *OutputTypeName[] = {
    "STTBX_OUTPUT_ALL",
    "STTBX_OUTPUT_PRINT",
    "STTBX_OUTPUT_REPORT",
    "STTBX_OUTPUT_REPORT_FATAL",
    "STTBX_OUTPUT_REPORT_ERROR",
    "STTBX_OUTPUT_REPORT_WARNING",
    "STTBX_OUTPUT_REPORT_ENTER_LEAVE_FN",
    "STTBX_OUTPUT_REPORT_INFO",
    "STTBX_OUTPUT_REPORT_USER1",
    "STTBX_OUTPUT_REPORT_USER2"
    };
#endif /* #ifndef STTBX_NO_UART  */

/* Global Variables --------------------------------------------------------- */

U16 LineCount;
S16 TotalErrCount;
U32 TotalTestsCount;

/* Private Macros ----------------------------------------------------------- */

/*#define STTBX_WaitMicroseconds(Microseconds)  __asm                 \
    {                                                               \
                ajw -3;                                             \
                ld Microseconds;                                    \
                stl 0;                                              \
                ldc 0x20000500;                                     \
                devlb;                                              \
                ldc 3;                                              \
                and;                                                \
                adc -1;                                             \
                stl 2;                                              \
                cj setfreq60__LINE__;                                       \
                ldl 2;                                              \
                adc -1;                                             \
                cj setfreq40;                                       \
                ldc 5;                                              \
                stl 2;                                              \
                j microsecond_loop;                                 \
            setfreq60__LINE__:                                              \
                ldc 6;                                              \
                stl 2;                                              \
                ldl 2;                                              \
                adc 0;                                              \
                cj microsecond_loop;                                \
                j microsecond_loop;                                 \
            setfreq40:                                              \
                ldc 4;                                              \
                stl 2;                                              \
                j microsecond_loop;                                 \
                ldc 0;                                              \
            microsecond_loop:                                       \
                ldl 0;                                              \
                adc -1;                                             \
                stl 0;                                              \
                ldl 2;                                              \
                adc -1;                                             \
                stl 1;                                              \
                ten_cycles_loop:                                    \
                    ldl 1;                                          \
                    adc -1;                                         \
                    stl 1;                                          \
                    ldl 1;                                          \
                    cj end_ten_cycles;                              \
                j ten_cycles_loop;                                  \
            end_ten_cycles:                                         \
                ldl 0;                                              \
                cj end_microseconds;                                \
            j microsecond_loop;                                     \
        end_microseconds:                                           \
                ajw 3;                                              \
    }*/


/* Private Function prototypes ---------------------------------------------- */

#ifdef ANOTHER_TASK
static void AnotherTask(void);
static void AnotherTaskForStress(void);
static void AnotherTaskForStress2(void);
static void RecursivePrint(const U8 FromNb, const U8 ToNb);
#endif

#if defined(ST_5510) || defined(ST_5512)
#ifdef TEST_CLOCK_SELECT_PIN
static void SpeedTest(void);
#endif
#endif

static void TestWaitMicroseconds(void);
static void PrintTest1(void);
static void PrintTest2(void);
static void PrintTest3(void);
static void PrintTest4(void);
static void WaitTest(void);
static void ReportTest(void);
static void InputCharTest(void);
static void InputPollCharTest(void);
static void InputStringTest(void);
static void InitAgainTest(void);
static void Multitask1Test(void);
static void Multitask2Test(void);
static void IO_Tests(void);

#ifndef STTBX_NO_UART
static void RedirectionTest(void);
static void FilterTests(void);
#endif /* #ifndef STTBX_NO_UART */


/* Functions ---------------------------------------------------------------- */

#ifdef ANOTHER_TASK
/*******************************************************************************
Name        : AnotherTask
Description : routine of a second task
              call some STTBX_xxxx() functions from this task
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void AnotherTask(void)
{
    char InputBuffer[128];
    U32 NbOfChar;

    STTBX_Print(("ANOTHERTASK> task started\n"));

    /* note : input on UART can't be done from a high priority task  */
    /*        because semaphore usage is forbidden in such a context */

    while (AnotherTaskRunning)
    {
        if (task_context(NULL,NULL) == task_context_task)
        {
            STTBX_Print(("\nANOTHERTASK> please enter a string and Return: "));
            NbOfChar = STTBX_InputStr(InputBuffer, sizeof(InputBuffer));
            STTBX_Print(("ANOTHERTASK> you entered '%s', length is %d.\n", InputBuffer, NbOfChar));
            task_delay(10 * TicksFor1000ms);
        }
        else
        {
            STTBX_Print(("\nANOTHERTASK> Urgent message from high priority task \n"));
            task_delay(5 * ST_GetClocksPerSecond()); /* high priority: TicksFor1000ms not valid ! */
        }
    }
    STTBX_Print(("ANOTHERTASK> task terminated\n"));
/*#if 0
    while (AnotherTaskRunning)
    {
        STTBX_Print(("ANOTHERTASK> I am here ... ;-)\n"));
        task_delay (5 * TicksFor1000ms);
    }
#endif*/ /* #if 0 */
}

/*******************************************************************************
Name        : RecursivePrint
Description : calling STTBX_Print() function
Parameters  : FromNb , ToNb
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void RecursivePrint(const U8 FromNb, const U8 ToNb)
{
    if (FromNb > ToNb)
    {
        STTBX_Print(("- %d", FromNb));
        RecursivePrint(FromNb-1, ToNb);
    }
}

/*******************************************************************************
Name        : AnotherTaskForStress
Description : call some STTBX_xxxx() print functions from this task
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void AnotherTaskForStress(void)
{
    STTBX_Print(("AnotherTaskForStress> task started\n"));

    while (AnotherTaskForStressRunning)
    {
        STTBX_Print(("\n>>>AnotherTaskForStress> Printed string from AnotherTaskForStress<<<"));
        RecursivePrint(50,40);
        task_delay(TicksFor100ms/10);
    }
    STTBX_Print(("\nAnotherTaskForStress> task terminated\n"));
}

/*******************************************************************************
Name        : AnotherTaskForStress2
Description : call some STTBX_xxxx() print functions from this task
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void AnotherTaskForStress2(void)
{
    STTBX_Print(("AnotherTaskForStress2> task started\n"));

    while (AnotherTaskForStressRunning)
    {
        STTBX_Print(("\n--- message given by parallel thread---\n"));
        RecursivePrint(20,10);
        task_delay(TicksFor1000ms/99);
    }
    STTBX_Print(("\nAnotherTaskForStress2> task terminated\n"));
}

#endif /* #ifdef ANOTHER_TASK */


/*******************************************************************************
Name        : TestWaitMicroseconds
Description : routine to test WaitMicroseconds() function (in high priority process)
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void TestWaitMicroseconds(void)
{
#if defined(ST_OS21) || defined(ST_OSWINCE)
    osclock_t Before, After, NbTicks;
#endif
#ifdef ST_OS20
    clock_t Before, After, NbTicks;
#endif
    U32 delayus;
    U32 ClockFrequency;
    U32 DecClockFrequency;
    U32 GetClocksPerSecond;

    ClockFrequency = ST_GetClockSpeed() / 1000000;
    DecClockFrequency = ST_GetClockSpeed() % 1000000;
    STTBX_Print(("CPU frequency = %d.%d MHz\n", ClockFrequency, DecClockFrequency));

    GetClocksPerSecond = ST_GetClocksPerSecond();
    STTBX_Print(("Ticks per seconds =  %d\n", GetClocksPerSecond));

    Before = time_now();
    STTBX_WaitMicroseconds(1);
    After = time_now();
    NbTicks = time_minus(After, Before);
    /* Add error due to CPU freq */
    delayus = (U32)NbTicks + ((U32)NbTicks * (1000000 - GetClocksPerSecond) / GetClocksPerSecond);
    STTBX_Print(("WaitMicroseconds(     1) took %d microseconds or %d ticks \n", delayus, NbTicks));


    Before = time_now();
    STTBX_WaitMicroseconds(10);
    After = time_now();
    NbTicks = time_minus(After, Before);
    /* Add error due to CPU freq */
    delayus = (U32)NbTicks + ((U32)NbTicks * (1000000 - GetClocksPerSecond) / GetClocksPerSecond);
    STTBX_Print(("WaitMicroseconds(    10) took %d microseconds or %d ticks\n", delayus, NbTicks));

    Before = time_now();
    STTBX_WaitMicroseconds(100);
    After = time_now();
    /* In high priority, one tick is exaclty one microsecond, so no need to format value */
    NbTicks = time_minus(After, Before);
    /* Add error due to CPU freq */
    delayus = (U32)NbTicks + ((U32)NbTicks * (1000000 - GetClocksPerSecond) / GetClocksPerSecond);
    STTBX_Print(("WaitMicroseconds(   100) took %d microseconds or %d ticks\n", delayus, NbTicks));

    Before = time_now();
    STTBX_WaitMicroseconds(1000);
    After = time_now();
    /* In high priority, one tick is exaclty one microsecond, so no need to format value */
    NbTicks = time_minus(After, Before);
    /* Add error due to CPU freq */
    delayus = (U32)NbTicks + ((U32)NbTicks * (1000000 - GetClocksPerSecond) / GetClocksPerSecond);
    STTBX_Print(("WaitMicroseconds(  1000) took %d microseconds or %d ticks\n", delayus, NbTicks));

    Before = time_now();
    STTBX_WaitMicroseconds(10000);
    After = time_now();
    /* In high priority, one tick is exaclty one microsecond, so no need to format value */
    NbTicks = time_minus(After, Before);
    /* Add error due to CPU freq */
    delayus = (U32)NbTicks + ((U32)NbTicks * (1000000 - GetClocksPerSecond) / GetClocksPerSecond);
    STTBX_Print(("WaitMicroseconds( 10000) took %d microseconds or %d ticks\n", delayus, NbTicks));

    Before = time_now();
    STTBX_WaitMicroseconds(100000);
    After = time_now();
    /* In high priority, one tick is exaclty one microsecond, so no need to format value */
    NbTicks = time_minus(After, Before);
    /* Add error due to CPU freq */
    delayus = (U32)NbTicks + ((U32)NbTicks * (1000000 - GetClocksPerSecond) / GetClocksPerSecond);
    STTBX_Print(("WaitMicroseconds(100000) took %d microseconds or %d ticks\n", delayus, NbTicks));

    STTBX_Print(("                              |\\_/ |\n"));
    STTBX_Print(("       normal waiting time __/  |   \\__ extra time caused by the call\n"));
    STTBX_Print(("                                |        (few microseconds)\n"));
    STTBX_Print(("                  extra time caused by the loop\n"));
    STTBX_Print(("                     (few nanoseconds * n)\n"));
}

/*******************************************************************************
Name        : PrintTest1
Description : test of STTBX_Print() function
Parameters  : none
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void PrintTest1(void)
{
    U16 Count;
    S32 Nb1, Nb2;

    /* --- Do some kinds of normal print --- */

    TotalTestsCount++;

    STTBX_Print(("\nSTTBX_Print() TESTS : \n\n"));
    for (Count=0 ; Count < 200 ; Count++)
    {
        STTBX_Print((""));
    }
    STTBX_Print(("\n"));
    STTBX_Print(("line 3: lines 1 and 2 above are empty\n"));
    STTBX_Print(("line 4 is this single string\n"));
    STTBX_Print(("line 5 is this another single string\n"));
    STTBX_Print(("line 6 is ended by 26 print of a letter: "));
    Count = 0;
    while (Count < 26)
    {
        STTBX_Print(("%c",('A'+Count) ));
        Count++;
    }

    Nb1 = -1;
    Nb2 = 999999;
    STTBX_Print(("\nline 7 is ended by the print of 2 numbers whose values are -1 and 999999: "));
    STTBX_Print(("%d %d \n",Nb1, Nb2));
    LineCount += 7;

    /* rmk: no question here because input is no tested yet */
    STTBX_Print(("### STTBX_Print() test is ok if lines 1 to 7 are displayed\n\n"));

} /* end of PrintTest1() */

/*******************************************************************************
Name        : PrintTest2
Description : test of STTBX_Print() function with buffer enable
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void PrintTest2(void)
{
    char Msg[1024];
    STTBX_BuffersConfigParams_t ConfParam;
    char InputCarac;

    TotalTestsCount++;

    /* --- Decide to print through a buffer --- */

    STTBX_Print(("\nSTTBX_Print() with buffer TESTS : \n"));

    STTBX_Print(("\nNow put slowly next output data in a buffer...\n"));

    ConfParam.Enable = TRUE ;
    ConfParam.KeepOldestWhenFull = TRUE ;
    STTBX_SetBuffersConfig(&ConfParam);

    /* --- Fill the circular buffer to verify if it is well managed --- */

    STTBX_Print(("Test output buffer management...\n"));

    LineCount = 1;
    while (LineCount <= 20)
    {
        /* string with 500 readable characters + \n + \0 */
        sprintf(Msg, "line %02d is a very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long bufferised line filled with 502 characters\n",LineCount);
        STTBX_Print((Msg));

        task_delay(TicksFor500ms);
        LineCount++;
    }

    STTBX_Print(("\nAre all the lines 1 to 20 fully displayed, in the right order (Y/N) ? \n"));
    STTBX_InputChar(&InputCarac);
    if ( InputCarac == 'y' || InputCarac == 'Y' )
    {
        STTBX_Print(("### STTBX_Print(): buffer management successful\n\n"));
    }
    else
    {
        STTBX_Print(("### STTBX_Print(): buffer management failure !\n\n"));
        TotalErrCount++;
    }

} /* end of PrintTest2() */

/*******************************************************************************
Name        : PrintTest3
Description : test of STTBX_Print() function with buffer disable
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void PrintTest3(void)
{
    char Msg[1024];
    STTBX_BuffersConfigParams_t ConfParam;
    char InputCarac;

    TotalTestsCount++;

    /* --- Decide to print without buffer --- */

    STTBX_Print(("\nNow send next output data directly to output...\n"));

    ConfParam.Enable = FALSE ;
    STTBX_SetBuffersConfig(&ConfParam);

    task_delay(TicksFor500ms); /* give some time to be sure that output buffer is empty */

    while (LineCount <= 29)
    {
        sprintf(Msg, "line %02d is a very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long line of 502 characters\n",LineCount);
        STTBX_Print((Msg));
        task_delay(TicksFor500ms); /* run slowly to avoid lost messages */
        LineCount++;
    }

    STTBX_Print(("\nAre all the lines 21 to 29 fully displayed, in the right order (Y/N) ? \n"));
    STTBX_InputChar(&InputCarac);
    if ( InputCarac == 'y' || InputCarac == 'Y' )
    {
        STTBX_Print(("### STTBX_Print(): output mode change successful\n\n"));
    }
    else
    {
        STTBX_Print(("### STTBX_Print(): output mode change failure !\n\n"));
        TotalErrCount++;
    }

} /* end of PrintTest3() */

/*******************************************************************************
Name        : PrintTest4
Description : test of STTBX_Print() function with buffer enable
              test oldest & newest modes
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void PrintTest4(void)
{
    char Msg[1024];
    STTBX_BuffersConfigParams_t ConfParam;
    char InputCarac;

    TotalTestsCount++;

    /* --- Decide to print through a buffer --- */

    STTBX_Print(("Now put again next output data in a buffer...\n"));

    ConfParam.Enable = TRUE ;
    ConfParam.KeepOldestWhenFull = TRUE ;
    STTBX_SetBuffersConfig(&ConfParam);

    task_delay(TicksFor200ms);

    /* --- Fill quiclky the buffer to verify if the user is informed of lost messages --- */

    sprintf(Msg, "LINE %02d IS A VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY VERY LONG BUFFERISED LINE FILLED WITH 502 CHARACTERS\n",LineCount);

    STTBX_Print(("Now test buffer overflow (keep oldest messages)...\n"));
    task_delay(TicksFor200ms);
    while (LineCount <= 39)
    {
        Msg[5] = '0'+(LineCount/10);
        Msg[6] = '0'+(LineCount%10);
        STTBX_Print((Msg));
        LineCount++;
    }

    task_delay(TicksFor1000ms);
    STTBX_Print(("\nAre the LINES 30 to 30+x fully (except last) displayed (Y/N) ? \n"));
    STTBX_InputChar(&InputCarac);
    if ( InputCarac == 'y' || InputCarac == 'Y' )
    {
        STTBX_Print(("### STTBX_Print(): output oldest message successful\n\n"));
    }
    else
    {
        STTBX_Print(("### STTBX_Print(): output oldest message failure !\n\n"));
        TotalErrCount++;
    }

    TotalTestsCount++;

    /* --- Fill quiclky the buffer to verify if the user is informed of lost messages --- */

    sprintf(Msg, "line %02d is a very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long bufferised line filled with 502 characters\n",LineCount);

    STTBX_Print(("Now test buffer overflow (keep newest messages)...\n"));
    task_delay(TicksFor500ms);
    ConfParam.KeepOldestWhenFull = FALSE ;
    STTBX_SetBuffersConfig(&ConfParam);

    task_delay(TicksFor200ms);
    while (LineCount <= 55)
    {
        Msg[5] = '0'+(LineCount/10);
        Msg[6] = '0'+(LineCount%10);
        STTBX_Print((Msg));
        LineCount++;
    }

    task_delay(TicksFor1000ms);
    STTBX_Print(("\nAre the lines 41+x to 55 fully (except first) displayed (Y/N) ? \n"));
    STTBX_InputChar(&InputCarac);
    if ( InputCarac == 'y' || InputCarac == 'Y' )
    {
        STTBX_Print(("### STTBX_Print(): output newest message successful\n\n"));
    }
    else
    {
        STTBX_Print(("### STTBX_Print(): output newest message failure !\n\n"));
        TotalErrCount++;
    }

    /* --- Print a string bigger than 512 bytes --- */

    /* can't be done because of buffer overflow */

    STTBX_Print(("line %d is the end of print tests\n\n",LineCount));
    task_delay(TicksFor500ms);

} /* end of PrintTest4() */

#if defined(ST_5510) || defined(ST_5512)
#ifdef TEST_CLOCK_SELECT_PIN
/*******************************************************************************
Name        : SpeedTest
Description : test of cpu speed
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SpeedTest(void)
{
    STTBX_Print(("### Test of CPU speed...\n"));

    /* Verify that state of register giving CPU frequency is changing depending on pins state.
       However, the effective CPU frequency doesn't change depending on pins state, except at reset ! */
    while (TRUE)
    {
#pragma ST_device(pu8SpeedSelect)

        volatile U8 *pu8SpeedSelect;
        pu8SpeedSelect = (volatile U8 *) (0x20000500);
        debugmessage("%d\n", *pu8SpeedSelect);
    }
} /* end of SpeedTest() */
#endif /* TEST_CLOCK_SELECT_PIN */
#endif /* #if defined(ST_5510) || defined(ST_5512) */

/*******************************************************************************
Name        : WaitTest
Description : test of STTBX_WaitMicroseconds() function
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void WaitTest(void)
{
    task_t * Task_p;
    char InputCarac;
    U16  TestError;

    TotalTestsCount++;
    TestError = 1;
    STTBX_Print(("\nSTTBX_WaitMicroseconds() TESTS : "));

    /* Test WaitMicroseconds() with high priority task to measure time precisely */
    /* Remark: why, but Output on UART scratches when in high priority task if the STTBX_Print() are not bufferized !!! */
    STTBX_Print(("\n\n"));
#if defined(ST_OS21) || defined(ST_OSWINCE)
    Task_p = task_create((void (*)(void* )) TestWaitMicroseconds, NULL, 16*1024, MIN_USER_PRIORITY, "Test WaitMicroseconds", task_flags_high_priority_process);
#endif
#ifdef ST_OS20
    Task_p = task_create((void (*)(void* )) TestWaitMicroseconds, NULL, 1024, 15, "Test WaitMicroseconds", task_flags_high_priority_process);
#endif
    if ( Task_p == NULL)
    {
        STTBX_Print(("Unable to initialise task !\n"));
    }
    else
    {
        task_wait(&Task_p, 1, TIMEOUT_INFINITY);
        task_delete(Task_p);

        STTBX_Print(("\nAre the real durations near from expected delays (Y/N) ? \n"));
        STTBX_InputChar(&InputCarac);
        if ( InputCarac == 'y' || InputCarac == 'Y' )
        {
            TestError = 0;
        }
    }
    if ( TestError ==0 )
    {
        STTBX_Print(("### STTBX_WaitMicroseconds(): test successful\n\n"));
    }
    else
    {
        STTBX_Print(("### STTBX_WaitMicroseconds(): test failure !\n\n"));
        TotalErrCount++;
    }

} /* end of WaitTest() */

/*******************************************************************************
Name        : ReportTest
Description : test of STTBX_Report() function
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ReportTest(void)
{
    char InputCarac;

    TotalTestsCount++;

    STTBX_Print(("\nSTTBX_Report() TESTS : \n"));
    STTBX_Print(("\n"));
    STTBX_Print((" filename  , line     xxxx> .................................................\n"));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "This report is aligned with previous printed line"));
    STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN, "Hello, this line is an enter_leave_function report"));
    STTBX_Report((111, "Hello, this line is an undefined level report"));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, ""));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "\0"));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "The 2 previous info lines contain no text message"));

    STTBX_Print(("\nAre the report lines are well formatted (Y/N) ? \n"));
    STTBX_InputChar(&InputCarac);
    if ( InputCarac == 'y' || InputCarac == 'Y' )
    {
        STTBX_Print(("### STTBX_Report(): test successful\n\n"));
    }
    else
    {
        STTBX_Print(("### STTBX_Report(): test failure !\n\n"));
        TotalErrCount++;
    }

} /* end of ReportTest() */

/*******************************************************************************
Name        : InputCharTest
Description : test of STTBX_InputChar() function
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void InputCharTest(void)
{
    char InputCarac;
    S16 ErrCount;

    TotalTestsCount++;
    ErrCount = 0;

    STTBX_Print(("\n\nSTTBX_InputChar() TESTS : "));

    STTBX_Print(("\n\nPlease press on <spacebar> key\n"));
    STTBX_InputChar(&InputCarac);
    if ( InputCarac == ' ' )
    {
        STTBX_Print(("You entered '%c': ok\n",InputCarac));
    }
    else
    {
        if ( isprint(InputCarac))
        {
            STTBX_Print(("You entered '%c': wrong !\n",InputCarac));
        }
        else
        {
            STTBX_Print(("You entered a non-printable character: wrong !\n"));
        }
        ErrCount++;
    }

    TotalTestsCount++;

    STTBX_Print(("Please press on <a> key (lower case)\n"));
    STTBX_InputChar(&InputCarac);
    if ( InputCarac == 'a' )
    {
        STTBX_Print(("You entered '%c': ok\n",InputCarac));
    }
    else
    {
        if ( isprint(InputCarac))
        {
            STTBX_Print(("You entered '%c': wrong !\n",InputCarac));
        }
        else
        {
            STTBX_Print(("You entered a non-printable character: wrong !\n"));
        }
        ErrCount++;
    }

    TotalTestsCount++;

    STTBX_Print(("Please press on <A> key (uppercase)\n"));
    STTBX_InputChar(&InputCarac);
    if ( InputCarac == 'A' )
    {
        STTBX_Print(("You entered '%c': ok\n",InputCarac));
    }
    else
    {
        if ( isprint(InputCarac))
        {
            STTBX_Print(("You entered '%c': wrong !\n",InputCarac));
        }
        else
        {
            STTBX_Print(("You entered a non-printable character: wrong !\n"));
        }
        ErrCount++;
    }
    if ( ErrCount == 0)
    {
        STTBX_Print(("### STTBX_InputChar(): test successful\n"));
    }
    else
    {
        TotalErrCount += ErrCount;
        STTBX_Print(("### STTBX_InputChar(): test failure !\n"));
    }

} /* end of InputCharTest() */

/*******************************************************************************
Name        : InputPollCharTest
Description : test of STTBX_InputPollChar() functions
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void InputPollCharTest(void)
{
    char InputCarac;
    S16 ErrCount;
    U32 Count, PollCount;
    BOOL KeyHit;

    TotalTestsCount++;
    ErrCount = 0;

    STTBX_Print(("\nSTTBX_InputPollChar() TESTS : \n"));

    STTBX_Print(("\nPlease press on <x> or <X> key (listening for 5 seconds)\n"));
    PollCount = 0;
    for (Count = 0; Count < 50; Count++)
    {
        KeyHit = STTBX_InputPollChar(&InputCarac);
        if ( KeyHit )
        {
            PollCount ++;
            if ( InputCarac == 'x' || InputCarac == 'X' )
            {
                STTBX_Print(("You entered '%c': ok\n",InputCarac));
            }
            else
            {
                if ( isprint(InputCarac))
                {
                    STTBX_Print(("You entered '%c': wrong !\n",InputCarac));
                }
                else
                {
                    STTBX_Print(("You entered a non-printable character: wrong !\n"));
                }
                ErrCount++;
            }
        }
        task_delay(TicksFor100ms);
    } /* end for */

    if ( PollCount == 1 && (( InputCarac == 'x' || InputCarac == 'X' )) )
    {
        STTBX_Print(("### STTBX_InputPollChar(): test successful\n"));
    }
    else
    {
        if (PollCount == 0)
        {
            STTBX_Print(("You're not quick enough !!!\n"));;
        }
        TotalErrCount += ErrCount;
        STTBX_Print(("### STTBX_InputPollChar(): test failure !\n"));
    }
} /* end of InputPollCharTest() */

/*******************************************************************************
Name        : InputStringTest
Description : test of STTBX_InputStr() functions
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void InputStringTest(void)
{
    char InputString[255];
    S16 ErrCount;

    TotalTestsCount++;
    ErrCount = 0;

    STTBX_Print(("\nSTTBX_InputString() TESTS : \n"));

    STTBX_Print(("\nEnter the string 'abcdefgh' (lower case) and press <return>\n"));
    STTBX_InputStr(InputString, sizeof(InputString));
    if ( strcmp(InputString,"abcdefgh")==0 )
    {
        STTBX_Print(("You entered '%s': ok\n",InputString));
    }
    else
    {
        STTBX_Print(("You entered '%s': wrong !\n",InputString));
        ErrCount++;
    }

    TotalTestsCount++;
    STTBX_Print(("Enter the string '123489', press <backspace> <backspace> <5> and <return>\n"));
    STTBX_InputStr(InputString, sizeof(InputString));
    if ( strcmp(InputString,"12345")==0 )
    {
        STTBX_Print(("You entered '%s': ok\n",InputString));
    }
    else
    {
        STTBX_Print(("You entered '%s': wrong !\n",InputString));
        ErrCount++;
    }

    TotalTestsCount++;
    STTBX_Print(("Enter a very long string until you hear a beep and press <return>\n"));
    STTBX_InputStr(InputString, sizeof(InputString));
    STTBX_Print(("You entered '%s' \n",InputString));
    if ( strlen(InputString) == sizeof(InputString)-1 )
    {
        STTBX_Print(("String length = %d: ok\n",strlen(InputString)));
    }
    else
    {
        STTBX_Print(("String length = %d: failure (%d expected)!\n",strlen(InputString),sizeof(InputString)-1));
        ErrCount++;
    }

    if ( ErrCount == 0)
    {
        STTBX_Print(("### STTBX_InputStr(): test successful\n"));
    }
    else
    {
        TotalErrCount += ErrCount;
        STTBX_Print(("### STTBX_InputStr(): test failure !\n"));
    }

} /* end of InputStringTest() */

/*******************************************************************************
Name        : InitAgainTest
Description : test another initialization
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void InitAgainTest(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STTBX_InitParams_t InitParams;
    STTBX_BuffersConfigParams_t BuffParams;
    S16 ErrCount;

    InitParams.SupportedDevices    = STTBX_DEVICE_DCU;
    InitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    InitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
    BuffParams.Enable = FALSE;
    InitParams.CPUPartition_p = DriverPartition_p;

    TotalTestsCount++;
    ErrCount = 0;

    STTBX_Print(("\nSTTBX_Init() TESTS : \n"));
    STTBX_Print(("\nTry to initialize Toolbox again with same name...\n"));
    ErrorCode = STTBX_Init(STTBX_DEVICE_NAME, &InitParams);

    if (ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
    {
        STTBX_Print(("new STTBX_Init(%s): already initialized error as expected\n", STTBX_DEVICE_NAME));
    }
    else
    {
        STTBX_Print(("new STTBX_Init(%s): error=0x%0x not expected!\n", STTBX_DEVICE_NAME, ErrorCode));
        ErrCount ++;
    }

    TotalTestsCount++;
    STTBX_Print(("\nTry to initialize Toolbox again with new name...\n"));
    ErrorCode = STTBX_Init(STTBX_NEW_DEVICE_NAME, &InitParams);

    if (ErrorCode == ST_ERROR_NO_MEMORY)
    {
        STTBX_Print(("new STTBX_Init(%s): no memory error as expected\n", STTBX_NEW_DEVICE_NAME));
    }
    else
    {
        STTBX_Print(("new STTBX_Init(%s): error=0x%0x not expected!\n", STTBX_NEW_DEVICE_NAME, ErrorCode));
        ErrCount ++;
    }

    if (ErrCount == 0)
    {
        STTBX_Print(("STTBX cannot be initialised twice\n"));
        STTBX_Print(("### STTBX_Init(): test successful \n"));
    }
    else
    {
        TotalErrCount += ErrCount;
        STTBX_Print(("STTBX can be initialised twice\n"));
        STTBX_Print(("### STTBX_Init(): test failure ! \n"));
    }

} /* end of InitAgainTest() */

/*******************************************************************************
Name        : Multitask1Test
Description : test of simultaneous call to STTBX_Xxxx() functions
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void Multitask1Test(void)
{
#ifdef ANOTHER_TASK
    task_t * Task_p;
    task_t * Task2_p;
    char *Stack;
    char *Stack2;
    U32 Count;
    char InputBuffer[50];
    U32 NbOfChar;

    TotalTestsCount++;

    STTBX_Print(("### Multitask1Test() start\n\n"));
    STTBX_Print(("Now test simultaneous call to STTBX from main and another task ...\n\n"));

    Stack = (char *)memory_allocate(DriverPartition_p, 1024*sizeof(char));
    if (Stack == NULL)
    {
        STTBX_Print (("\nUnable to allocate the stack !!!"));
        return;
    }
    Stack2 = (char *)memory_allocate(DriverPartition_p, 1024*sizeof(char));
    if (Stack2 == NULL)
    {
        STTBX_Print (("\nUnable to allocate the stack !!!"));
        return;
    }

    AnotherTaskRunning = TRUE;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    Task_p = task_create((void (*)(void* )) AnotherTask, NULL, 16*1024, MIN_USER_PRIORITY, "another task", task_flags_no_min_stack_size);
#endif
#ifdef ST_OS20
    Task_p = task_create((void (*)(void* )) AnotherTask, NULL, 1024, 5, "another task", 0);
#endif
    if ( Task_p == NULL)
    {
        STTBX_Print(("Unable to create task structure or stack !\n"));
        return;
    }

    /* Do some input and output */

    Count = 1;
    while (Count <= 4)
    {
        STTBX_Print(("[%d/4] hello task1, please enter a string and Return: ", Count));
        if (Count!=4)
        {
          NbOfChar = STTBX_InputStr(InputBuffer, sizeof(InputBuffer));
          STTBX_Print(("[%d/4] hello task1, you entered '%s' (length=%d)\n", Count, InputBuffer, NbOfChar));
        }
        Count++;
    }

    AnotherTaskRunning = FALSE;
    STTBX_Print(("\n thanks task1\n"));

    task_wait(&Task_p, 1, TIMEOUT_INFINITY);
    task_delete(Task_p);

    task_delay(TicksFor1000ms);

    STTBX_Print(("\nNow stress STTBX prints several tasks ...\n"));

    AnotherTaskForStressRunning = TRUE;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    Task_p = task_create((void (*)(void* )) AnotherTaskForStress, NULL, 16*1024, 5, "another task for stress", task_flags_no_min_stack_size);
#endif
#ifdef ST_OS20
    Task_p = task_create((void (*)(void* )) AnotherTaskForStress, NULL, 1024, 5, "another task for stress", 0);
#endif
    if ( Task_p == NULL)
    {
        STTBX_Print((" ERROR !!! Unable to create task structure or stack !\n"));
    }
#if defined(ST_OS21) || defined(ST_OSWINCE)
    Task2_p = task_create((void (*)(void* )) AnotherTaskForStress2, NULL, 16*1024, 5, "another task for stress 2", task_flags_no_min_stack_size);
#endif
#ifdef ST_OS20
    Task2_p = task_create((void (*)(void* )) AnotherTaskForStress2, NULL, 1024, 5, "another task for stress 2", task_flags_no_min_stack_size);
#endif
    if ( Task2_p == NULL)
    {
        STTBX_Print((" ERROR !!! Unable to create task2 structure or stack2 !\n"));
    }

    task_delay(10*TicksFor1000ms);

    /* Terminate */

    AnotherTaskForStressRunning = FALSE;
    task_wait(&Task_p, 1, TIMEOUT_INFINITY);
    task_delete(Task_p);
    task_wait(&Task2_p, 1, TIMEOUT_INFINITY);
    task_delete(Task2_p);
    memory_deallocate (DriverPartition_p, Stack);
    memory_deallocate (DriverPartition_p, Stack2);
    STTBX_Print(("### Multitask1Test() finished\n"));

    task_delay(TicksFor1000ms); /* to allow task cleaning */

#endif
} /* end of Multitask1Test() */

/*******************************************************************************
Name        : Multitask2Test
Description : test of simultaneous call to STTBX_Xxxx() functions
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void Multitask2Test(void)
{
#ifdef ANOTHER_TASK
    task_t * Task_p;
    char *Stack;
    U32 Count;
    char InputBuffer[50];
    U32 NbOfChar;

    TotalTestsCount++;

    STTBX_Print(("### Multitask2Test() start\n\n"));
    STTBX_Print(("Now test simultaneous call to STTBX from main and another high priority task ...\n\n"));

    Stack = (char *)memory_allocate(DriverPartition_p, 1024*sizeof(char));
    if (Stack == NULL)
    {
        STTBX_Print (("\nUnable to allocate the stack !!!"));
        return;
    }

    AnotherTaskRunning = TRUE;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    Task_p = task_create((void (*)(void* )) AnotherTask, NULL, 16*1024, 5, "another task", task_flags_high_priority_process);
#endif
#ifdef ST_OS20
    Task_p = task_create((void (*)(void* )) AnotherTask, NULL, 1024, 5, "another task", 0);
#endif
    if ( Task_p == NULL)
    {
        STTBX_Print(("Unable to create task structure or stack !\n"));
        return;
    }

    /* Do some input and output */

    Count = 1;
    while (Count <= 4)
    {
        task_delay(TicksFor100ms);
        STTBX_Print(("[%d/4] hello task2, please enter a string and Return: ", Count));
        NbOfChar = STTBX_InputStr(InputBuffer, sizeof(InputBuffer));
        STTBX_Print(("[%d/4] hello task2, you entered '%s' (length=%d)\n", Count, InputBuffer, NbOfChar));
        Count++;
    }

    /* Terminate */

    AnotherTaskRunning = FALSE;
    STTBX_Print(("thanks task2\n"));

    task_wait(&Task_p, 1, TIMEOUT_INFINITY);
    task_delete(Task_p);
    memory_deallocate (DriverPartition_p, Stack);
    STTBX_Print(("### Multitask2Test() finished\n"));

    task_delay(TicksFor1000ms);

#endif

} /* end of Multitask2Test() */

/*******************************************************************************
Name        : IO_Tests
Description : input / output tests
Parameters  : none
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void IO_Tests(void)
{
    char InputCarac;

    LineCount = 1;

    PrintTest1();
    InputCharTest();
    InputPollCharTest();
    InputStringTest();
    ReportTest();

    STTBX_Print(("\n\nPlease press any key to continue...\n"));
    STTBX_InputChar(&InputCarac);

    PrintTest2();
    PrintTest3();
    PrintTest4();

} /* end of IO_Tests() */

#ifndef STTBX_NO_UART
/*******************************************************************************
Name        : RedirectionTest
Description : redirection tests
Parameters  : none
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void RedirectionTest(void)
{
    STTBX_BuffersConfigParams_t ConfParam;
    char InputCarac;

    TotalTestsCount++;

    STTBX_Print(("### RedirectionTest: start\n\n"));

    STTBX_Print(("\n\nNow on the point of redirecting to UART, please press any key when ready...\n"));
    STTBX_InputChar(&InputCarac);
    STTBX_SetRedirection(STTBX_DEVICE_DCU, STTBX_DEVICE_UART);
    STTBX_Print(("First message sent to UART\n"));

    IO_Tests();

    TotalTestsCount++;
    STTBX_Print(("\n\nNow on the point of going back to DCU, please press any key when ready...\n"));
    STTBX_InputChar(&InputCarac);
    STTBX_SetRedirection(STTBX_DEVICE_DCU, STTBX_DEVICE_DCU);

    ConfParam.Enable = FALSE;
    STTBX_SetBuffersConfig(&ConfParam);

    STTBX_Print(("\n*** First message back to DCU ***\n"));

    STTBX_Print(("\nHas everything gone well (Y/N) ? \n"));
    STTBX_InputChar(&InputCarac);

    if ( InputCarac == 'y' || InputCarac == 'Y' )
    {
        STTBX_Print(("### RedirectionTest: test successful\n\n"));
    }
    else
    {
        STTBX_Print(("### RedirectionTest: test failure !\n\n"));
        TotalErrCount++;
    }
} /* end of RedirectionTest() */


/*******************************************************************************
Name        : FilterTests
Description : filtering tests
Parameters  : none
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void FilterTests(void)
{
    STTBX_OutputFilter_t OutputFilter;
    STTBX_OutputType_t   OutputType;
    STTBX_InputFilter_t  InputFilter;
    char InputCarac;
    char InputString[20];
    S16 ErrCount = 0;

    TotalTestsCount++;

    OutputFilter.ModuleID = STTBX_MODULE_ID;
    InputFilter.ModuleID  = STTBX_MODULE_ID;

    STTBX_Print(("### FilterTests: start\n\n"));
    STTBX_Print(("Next outputs will be filtered from DCU to UART\n"));

    for (OutputType = 0; OutputType < STTBX_NB_OF_OUTPUTS; OutputType ++)
    {
        STTBX_Print(("\nOn the point of filtering output of type: %s\n", OutputTypeName[OutputType]));
        OutputFilter.OutputType = OutputType;
        STTBX_SetOutputFilter(&OutputFilter, STTBX_DEVICE_UART);
        STTBX_Print(("Check this line from STTBX_Print is output to the right device\n"));

        STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "Check this line from STTBX_Report is output to the right device\n"));
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Check this line from STTBX_Report is output to the right device\n"));
        STTBX_Report((STTBX_REPORT_LEVEL_WARNING, "Check this line from STTBX_Report is output to the right device\n"));
        STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN, "Check this line from STTBX_Report is output to the right device\n"));
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Check this line from STTBX_Report is output to the right device\n"));
        STTBX_Report((STTBX_REPORT_LEVEL_USER1, "Check this line from STTBX_Report is output to the right device\n"));
        STTBX_Report((STTBX_REPORT_LEVEL_USER2, "Check this line from STTBX_Report is output to the right device\n"));

        /* reset filtering */
        OutputFilter.OutputType = STTBX_OUTPUT_ALL;
        STTBX_SetOutputFilter(&OutputFilter, STTBX_DEVICE_DCU);

        STTBX_Print(("\nIs each line output to the right device (Y/N) ? \n"));
        STTBX_InputChar(&InputCarac);

        if ( (InputCarac != 'y') && (InputCarac != 'Y') )
        {
            ErrCount ++;
        }
    } /* end for (OutputType ... */

    TotalTestsCount++;

    STTBX_Print(("\nNow all outputs are sent to DCU\n"));
    STTBX_Print(("\nNext input will be filtered from DCU to UART\n"));

    InputFilter.InputType = STTBX_INPUT_ALL;
    STTBX_SetInputFilter(&InputFilter, STTBX_DEVICE_UART);

    STTBX_Print(("\nEnter the string 'from DCU' from your DCU console and press <return>\n"));
    STTBX_Print(("\nEnter the string 'from UART' from your UART console and press <return>\n"));
    STTBX_InputStr(InputString, sizeof(InputString));

    /* reset input filtering */
    STTBX_SetInputFilter(&InputFilter, STTBX_DEVICE_DCU);

    if ( strcmp(InputString,"from UART")==0 )
    {
        STTBX_Print(("You entered '%s': ok\n",InputString));
    }
    else
    {
        STTBX_Print(("You entered '%s': wrong !\n",InputString));
        ErrCount++;
    }

    TotalTestsCount++;
    STTBX_Print(("\nNow all inputs are got from DCU\n\n"));

    if ( ErrCount == 0)
    {
        STTBX_Print(("### FilterTests: test successful\n"));
    }
    else
    {
        STTBX_Print(("### FilterTests: test failure !\n"));
        TotalErrCount += ErrCount;
    }

    /* empty DCU console input buffer */
    STTBX_InputStr(InputString, sizeof(InputString));

} /* end of FilterTests() */
#endif /* #ifndef STTBX_NO_UART */


/*#########################################################################
 *                                 MAIN
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : os20_main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void os20_main(void *ptr)
{
    BOOL RetOk = TRUE;

    STAPIGAT_Init();

#ifdef TEST_UART_DRIVER
    Test_Uart_Driver_fn();
#endif

    /* --- Starting basic tests -------------------------------------------- */

    printf ("\nStack usage ... \n");
    metrics_Stack_Test();

    printf ("\nStarting basic tests ... \n");
    TotalErrCount = 0;
    TotalTestsCount = 0;

    TicksFor1000ms = ST_GetClocksPerSecond();
    TicksFor100ms = TicksFor1000ms / 10;
    TicksFor200ms = TicksFor1000ms / 5;
    TicksFor500ms = TicksFor1000ms / 2;

    /* Initialise toolbox */
    RetOk = TBX_Init();
    if (!RetOk) return ;

    InitAgainTest();
    IO_Tests();

#if defined(ST_5510) || defined(ST_5512)
#ifdef TEST_CLOCK_SELECT_PIN
    SpeedTest();
#endif
#endif

    /* Caution: Output device should not be UART when testing WaitMicrosecond. */
    WaitTest();

#ifndef STTBX_NO_UART
    RedirectionTest();
    FilterTests();
#endif /* #ifndef STTBX_NO_UART  */

    Multitask1Test();
    Multitask2Test();

    STTBX_Print(("\n### Toolbox tests: %d run, ", TotalTestsCount));
    if ( TotalErrCount == 0 )
    {
        STTBX_Print(("successful\n"));
    }
    else
    {
        STTBX_Print(("failure ! (%d errors)\n", TotalErrCount));
    }
    task_delay(TicksFor1000ms); /* to allow remaining output */

    /* --- Termination ----------------------------------------------------- */

    TBX_Term();

    STAPIGAT_Term();

} /* end os20_main */

#endif  /* #ifndef ST_OSLINUX */

/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
#if defined(ST_OS21) && !defined(ST_OSWINCE)
    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
    os20_main(NULL);
#endif
#if defined(ST_OSWINCE)
    printf ("\nBOOT ...\n");
    os20_main(NULL);
#endif
#ifdef ST_OS20
    os20_main(NULL);
#endif

    printf ("\n --- End of main --- \n");
#if !defined ST_OSLINUX
    fflush (stdout);
#endif

    exit (0);
}

/* End of tbx_test.c */


