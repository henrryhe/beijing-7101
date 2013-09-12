#include <stdio.h>
#include <stdlib.h>

#if !defined( ST_OSLINUX )
#include "stlite.h"
#endif

#include "evt_test.h"
#ifdef OS40
#include "exec/chTrap.h"
#include "exec/chExec.h"
#include "exec/f_chTrap.h"
#include "lap/chLap.h"
#include "exec/chIo.h"
#endif

#if defined( ST_OSLINUX )
#define GlobalInit() ST_NO_ERROR

ST_Partition_t * SystemPartition = (ST_Partition_t *)1;/* Not actually used for linux */

#endif

void os20_main(void *ptr)
{
    U32 Error;
    U32 TotalPassed = 0, TotalFailed = 0;
#if !defined( OS40 ) && !defined( ST_OSLINUX )
    U32 MemoryAvailableStart = 0;
    U32 MemoryAvailableEnd = 0;
    partition_status_t part_status;
#endif

    puts("\nStarting initialization process...");
    Error = GlobalInit();
    if (Error != ST_NO_ERROR)
    {
        puts("Initialization failed");
        return;
    }

#if defined(ST_OSLINUX)
    STEVT_CBAck(TRUE);/* Enabling CB acknowlegement */
#endif

#if !defined(OS40) && !defined( ST_OSLINUX )
    /* Shouldn't ever fail. */
    if (partition_status(system_partition, &part_status,
                         (partition_status_flags_t)0) == -1)
    {
        STEVT_Print(("FAILED to get partition status - corrupted?\n"));
    }
    else
    {
        MemoryAvailableStart = part_status.partition_status_free;
    }
#endif
#if !defined(STEVT_NO_PARAMETER_CHECK)
    STEVT_Print(("\nTComp_1_1: STEVT_Open interface test\n"));
    Error = TComp_1_1();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_1_1 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_1_1 FAILED\n"));
    }
    STEVT_Print(("\nTComp_1_2: STEVT_Close interface test\n"));
    Error = TComp_1_2();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_1_2 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_1_2 FAILED\n"));
    }

#if 0
    STEVT_Print(("\nTComp_1_3: STEVT_Subscriber  test\n"));
    Error = TComp_1_3();
    if (Error ==0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_1_3 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_1_3 FAILED\n"));
    }
#endif

    STEVT_Print(("\nTComp_2_1: STEVT_Register (without subscription) interface test\n"));
    Error = TComp_2_1();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_2_1 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_2_1 FAILED\n"));
    }
#endif

    STEVT_Print(("\nTComp_2_2: STEVT_Register (with subscription) interface test\n"));
    Error = TComp_2_2();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_2_2 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_2_2 FAILED\n"));
    }

#if !defined(STEVT_NO_PARAMETER_CHECK)
    STEVT_Print(("\nTComp_2_3: STEVT_Unregister interface test\n"));
    Error = TComp_2_3();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_2_3 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_2_3 FAILED\n"));
    }
    STEVT_Print(("\nTComp_3_1: STEVT_Subscribe (to registered events) interface test\n"));
    Error = TComp_3_1();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_3_1 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_3_1 FAILED\n"));
    }

    STEVT_Print(("\nTComp_3_2: STEVT_Subscribe (to unregistered events)interface test\n"));
    Error = TComp_3_2();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_3_2 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_3_2 FAILED\n"));
    }

    STEVT_Print(("\nTComp_3_3: STEVT_Unsubscribe interface test\n"));
    Error = TComp_3_3();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_3_3 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_3_3 FAILED\n"));
    }

    STEVT_Print(("\nTComp_4_1: STEVT_Notify interface test\n"));
    Error = TComp_4_1();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_4_1 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_4_1 FAILED\n"));
    }

#endif
    STEVT_Print(("\nTComp_4_2: STEVT_NotifySubscriber interface test\n"));
    Error = TComp_4_2();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_4_2 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_4_2 FAILED\n"));
    }

#if !defined(STEVT_NO_PARAMETER_CHECK)
    STEVT_Print(("\nTComp_5_1: STEVT_Init interface test\n"));
    Error = TComp_5_1();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_5_1 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_5_1 FAILED\n"));
    }

    STEVT_Print(("\nTComp_5_2: STEVT_Term interface test\n"));
    Error = TComp_5_2();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_5_2 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_5_2 FAILED\n"));
    }
#endif

    STEVT_Print(("\nTComp_6_1: STEVT_Notify/NotifySubscribers performance\n"));
    Error = TComp_6_1();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_6_1 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_6_1 FAILED\n"));
    }

    STEVT_Print(("\nTComp_7_1: Memory allocation test\n"));
    Error = TComp_7_1();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_7_1 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_7_1 FAILED\n"));
    }

    STEVT_Print(("\nTComp_8_1: Memory stress test\n"));
    Error = TComp_8_1();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TComp_8_1 %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TComp_8_1 FAILED\n"));
    }

#if !defined(STEVT_NO_PARAMETER_CHECK)
    STEVT_Print(("\nTESTEVT_MultiInstance: Multi instance drivers STEVT interface test\n"));
    Error = TESTEVT_MultiInstance();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TESTEVT_MultiInstance %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TESTEVT_MultiInstance FAILED\n"));
    }
#endif
    STEVT_Print(("\nTESTEVT_MultiName: Multi Name drivers STEVT interface test\n"));
    Error = TESTEVT_MultiName();
    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TESTEVT_MultiName %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TESTEVT_MultiName FAILED\n"));
    }
	
#if defined(ST_OSLINUX)
    /* If CB acknowledgement is enabled and because a single thread
       is used to make the callback then the lowest callback never returns and so also the Notify.
       By disabling CB acknowledgement the Notify is allowed to return immediately. */
       STEVT_CBAck(FALSE);/* Disabling CB acknowlegement */
#endif

    Error = TESTEVT_Threading();
    STEVT_Print(("\nTESTEVT_Threading: Multithreading tests\n"));

    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("TESTEVT_Threading %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("TESTEVT_Threading FAILED\n"));
    }

    STEVT_Print(("\nTESTEVT_ReEnter: Reenter Test\n"));
    Error = TESTEVT_ReEnter();

    if (Error == 0)
    {
        TotalPassed++;
    }
    else
    {
        STEVT_Print(("nTESTEVT_ReEnter %2d Test Cases failed\n",Error));
        TotalFailed++;
        STEVT_Print(("nTESTEVT_ReEnter FAILED\n"));
    }

#if defined(ST_OSLINUX)
    STEVT_CBAck(TRUE);/* Re-enabling CB acknowlegement */
#endif

#if !defined(OS40) && !defined( ST_OSLINUX )
    /* Shouldn't ever fail. */
    if (partition_status(system_partition, &part_status,
                         (partition_status_flags_t)0) == -1)
    {
        STEVT_Print(("FAILED to get partition status - corrupted?\n"));
    }
    else
    {
        MemoryAvailableEnd = part_status.partition_status_free;
        /* Temporary hack, until someone gets around to changing the
         * init stuff back to a linked list, a-la 1.[67].0
         */
        /* Changed EHDictNum and EHDictSize both to zero in the STEVT_Term */
        /* to fix the memory leak condition. We can avoid changing the init*/
        /*  stuff back to linked list implementation - Himanshu 15/11/2006 */
        if (MemoryAvailableEnd != MemoryAvailableStart)
        {
            STEVT_Print(("\nMemory leak test failed - leaked %i bytes (%i, %i)\n",
                        MemoryAvailableStart - MemoryAvailableEnd,
                        MemoryAvailableStart, MemoryAvailableEnd));
            TotalFailed++;
        }
        else
        {
            STEVT_Print(("\nMemory leak test passed, no unexpected leak\n"));
            TotalPassed++;
        }
    }
#endif


    STEVT_Print(("\n************************************************************\n"));
    STEVT_Print(("PASSED :%d\n", TotalPassed));
    STEVT_Print(("FAILED :%d", TotalFailed));
    STEVT_Print(("\n************************************************************\n"));


#if defined(ST_OSLINUX)

    STEVT_Print(("\n************************************************************\n"));
    STEVT_Print(("Linux Note:\n"));
    STEVT_Print(("The thread test(ListLock) has not been implemented.\n"));
    STEVT_Print(("\n************************************************************\n"));

#endif

}

int main(int argc, char *argv[])
{

#if defined(ARCHITECTURE_ST40) && !defined(ST_OS21) && !defined(ST_OSLINUX) && !defined(ST_OSWINCE)
    U32 loop, loop2 = 0;

    printf ("\nBOOT ...\n");
    OS20_main(argc, argv, os20_main);
    task_delay (100000);
    for (loop = 0;loop < 100000; loop++)
    {
        loop2 = loop2 + 2;
    }
#else
    os20_main(NULL);
#endif

    return 0;
}

