/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : memory.c
Description : STOS memory specific tests

Note        :

Date          Modification                                    Initials
----          ------------                                    --------
Mar 2007      Creation                                        CD

************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include "memory.h"
#include "sttbx.h"


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Private Constants --- */

/* --- Global variables --- */
#define KILO                1024
#define MEGA                (1024*1024)

#define NB_ALLOCATIONS      7
static U32 Allocation_sizes[NB_ALLOCATIONS] =
                                 {  4,
                                    40,
                                    400,
                                    4*KILO,
                                    40*KILO,
                                    400*KILO,
                                    4*MEGA,
                                 };
/* --- Extern variables --- */

/* --- Private variables --- */

/* --- Extern functions prototypes --- */

/* --- Private Function prototypes --- */


/*-------------------------------------------------------------------------
 * Function : test_write_read
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static STOS_Error_t test_write_read(U32 * Test_ptr)
{
    *Test_ptr = 0;
    if (*Test_ptr != 0)
    {
        return STOS_FAILURE;
    }

    *Test_ptr = 0xAAAAAAAA;
    if (*Test_ptr != 0xAAAAAAAA)
    {
        return STOS_FAILURE;
    }

    *Test_ptr = 0x55555555;
    if (*Test_ptr != 0x55555555)
    {
        return STOS_FAILURE;
    }

    return STOS_SUCCESS;
}

/*-------------------------------------------------------------------------
 * Function : test_all_write_read
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static STOS_Error_t test_all_write_read(U32 * Test_ptr, U32 nbentries)
{
    STTBX_Print((" Testing read/write: "));

    if (test_write_read(Test_ptr) != STOS_SUCCESS)                      /* begin of allocated buffer */
        goto error;
    if (test_write_read(Test_ptr + nbentries/2) != STOS_SUCCESS)     /* middle of allocated buffer */
        goto error;
    if (test_write_read(Test_ptr + nbentries - 1) != STOS_SUCCESS)   /* end of allocated buffer */
        goto error;

    STTBX_Print(("OK\n"));

    return STOS_SUCCESS;


    /* Error cases */
error:
    STTBX_Print(("Error !!!\n"));

    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : test_clear
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static STOS_Error_t test_clear(U32 * Test_ptr, U32 nbentries)
{
    U32     i;

    for (i=0 ; i<nbentries ; i++)
    {
        if (Test_ptr[i])    /* data is not cleared */
            return STOS_FAILURE;
    }

    return STOS_SUCCESS;
}

/*-------------------------------------------------------------------------
 * Function : test_all_clear
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static STOS_Error_t test_all_clear(U32 * Test_ptr, U32 nbentries)
{
    STTBX_Print((" Testing clearing: "));
    if (nbentries*sizeof(U32) > KILO)
    {
        if (test_clear(Test_ptr, KILO/sizeof(U32)) != STOS_SUCCESS)     /* Check first block of 1K */
            goto error;

        if (nbentries*sizeof(U32) > 2*KILO)
        {
            if (test_clear(Test_ptr + nbentries/2, KILO/sizeof(U32)) != STOS_SUCCESS)     /* Check middle block of 1K */
                goto error;
        }

        if (test_clear(Test_ptr + nbentries - KILO/sizeof(U32) - 1, KILO/sizeof(U32)) != STOS_SUCCESS)     /* Check last block of 1K */
            goto error;
    }
    else
    {
        if (test_clear(Test_ptr, nbentries) != STOS_SUCCESS)
            goto error;
    }
    STTBX_Print(("OK\n"));

    return STOS_SUCCESS;

    /* Error cases */
error:
    STTBX_Print(("Error !!!\n"));

    return STOS_FAILURE;
}




/*#######################################################################
 *                             Exported functions
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : TestAllocate_write_read
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestAllocate_write_read(void)
{
    U32               * Test_ptr;
    U32                 NbPtrEntries;
    U32                 i;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    for (i=0 ; i < NB_ALLOCATIONS ; i++)
    {
        NbPtrEntries = Allocation_sizes[i]/sizeof(U32);

        STTBX_Print(("Allocating %d: ", Allocation_sizes[i]));

        Test_ptr = (U32 *)STOS_MemoryAllocate(NULL, NbPtrEntries*sizeof(U32));   /* Allocates sizes in byte */
        if (Test_ptr != NULL)
        {
            STTBX_Print(("OK\n"));

            /* Check effectiveness of allocation by writing dummy values */
            if (test_all_write_read(Test_ptr, NbPtrEntries) != STOS_SUCCESS)
                goto error;

            STOS_MemoryDeallocate(NULL, Test_ptr);
        }
        else
        {
            STTBX_Print(("Failed\n"));
        }
    }

    return STOS_SUCCESS;


    /* Error cases */
error:
    STTBX_Print(("Error !!!\n"));
    if (Test_ptr != NULL)
    {
        STOS_MemoryDeallocate(NULL, Test_ptr);
    }

    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : TestDeallocate_after_alloc
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestDeallocate_after_alloc(void)
{
    U32   * Test_ptr;
    U32     i;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* Check different allocation sizes */
    for (i=0 ; i<NB_ALLOCATIONS ; i++)
    {
        Test_ptr = (U32 *)STOS_MemoryAllocate(NULL, Allocation_sizes[i]);   /* Allocates sizes in byte */
        if (Test_ptr != NULL)
        {
            STOS_MemoryDeallocate(NULL, Test_ptr);
        }
    }

    STTBX_Print(("No way to check deallocation, please check by other means ...\n"));

    /* No way to check effectiveness of deallocation !!! */
    return STOS_SUCCESS;
}

/*-------------------------------------------------------------------------
 * Function : TestAllocateClear_clear_write_read
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestAllocateClear_clear_write_read(void)
{
    U32   * Test_ptr;
    U32     NbPtrEntries;
    U32     i;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    for (i=0 ; i < NB_ALLOCATIONS ; i++)
    {
        NbPtrEntries = Allocation_sizes[i]/sizeof(U32);

        STTBX_Print(("Allocating %d (%dx%d): ", Allocation_sizes[i], 1, Allocation_sizes[i]));
        Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, 1, Allocation_sizes[i]);   /* Allocates different sizes in bytes */
        if (Test_ptr != NULL)
        {
            STTBX_Print(("OK\n"));

            /* Check effectiveness of clearing */
            if (test_all_clear(Test_ptr, NbPtrEntries) != STOS_SUCCESS)
                goto error;

            /* Check effectiveness of allocation by writing dummy values */
            if (test_all_write_read(Test_ptr, NbPtrEntries) != STOS_SUCCESS)
                goto error;

            STOS_MemoryDeallocate(NULL, Test_ptr);
        }
        else
        {
            STTBX_Print(("Failed\n"));
        }

        if (Allocation_sizes[i] > KILO)
        {
            STTBX_Print(("Allocating %d (%dx%d): ", Allocation_sizes[i], KILO, Allocation_sizes[i]/KILO));
            Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, KILO, Allocation_sizes[i]/KILO);   /* Allocates different sizes in bytes */
            if (Test_ptr != NULL)
            {
                STTBX_Print(("OK\n"));

                /* Check effectiveness of clearing */
                if (test_all_clear(Test_ptr, NbPtrEntries) != STOS_SUCCESS)
                    goto error;

                /* Check effectiveness of allocation by writing dummy values */
                if (test_all_write_read(Test_ptr, NbPtrEntries) != STOS_SUCCESS)
                    goto error;

                STOS_MemoryDeallocate(NULL, Test_ptr);
            }
            else
            {
                STTBX_Print(("Failed\n"));
            }
        }

        if (Allocation_sizes[i] > MEGA)
        {
            STTBX_Print(("Allocating %d (%dx%d): ", Allocation_sizes[i], MEGA, Allocation_sizes[i]/MEGA));
            Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, MEGA, Allocation_sizes[i]/MEGA);   /* Allocates different sizes in bytes */
            if (Test_ptr != NULL)
            {
                STTBX_Print(("OK\n"));

                /* Check effectiveness of clearing */
                if (test_all_clear(Test_ptr, NbPtrEntries) != STOS_SUCCESS)
                    goto error;

                /* Check effectiveness of allocation by writing dummy values */
                if (test_all_write_read(Test_ptr, NbPtrEntries) != STOS_SUCCESS)
                    goto error;

                STOS_MemoryDeallocate(NULL, Test_ptr);
            }
            else
            {
                STTBX_Print(("Failed\n"));
            }
        }
    }

    return STOS_SUCCESS;


    /* Error cases */
error:
    STTBX_Print(("Error !!!\n"));
    if (Test_ptr != NULL)
    {
        STOS_MemoryDeallocate(NULL, Test_ptr);
    }

    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : TestDeallocate_after_allocclear
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestDeallocate_after_allocclear(void)
{
    U32   * Test_ptr;
    U32     NbPtrEntries;
    U32     i;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    for (i=0 ; i < NB_ALLOCATIONS ; i++)
    {
        NbPtrEntries = Allocation_sizes[i]/sizeof(U32);

        STTBX_Print(("Allocating %d (%dx%d): ", Allocation_sizes[i], 1, Allocation_sizes[i]));
        Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, 1, Allocation_sizes[i]);   /* Allocates different sizes in bytes */
        if (Test_ptr != NULL)
        {
            STTBX_Print(("OK\n"));
            STOS_MemoryDeallocate(NULL, Test_ptr);
        }
        else
        {
            STTBX_Print(("Failed\n"));
        }

        if (Allocation_sizes[i] > KILO)
        {
            STTBX_Print(("Allocating %d (%dx%d): ", Allocation_sizes[i], KILO, Allocation_sizes[i]/KILO));
            Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, KILO, Allocation_sizes[i]/KILO);   /* Allocates different sizes in bytes */
            if (Test_ptr != NULL)
            {
                STTBX_Print(("OK\n"));
                STOS_MemoryDeallocate(NULL, Test_ptr);
            }
            else
            {
                STTBX_Print(("Failed\n"));
            }
        }

        if (Allocation_sizes[i] > MEGA)
        {
            STTBX_Print(("Allocating %d (%dx%d): ", Allocation_sizes[i], MEGA, Allocation_sizes[i]/MEGA));
            Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, MEGA, Allocation_sizes[i]/MEGA);   /* Allocates different sizes in bytes */
            if (Test_ptr != NULL)
            {
                STTBX_Print(("OK\n"));
                STOS_MemoryDeallocate(NULL, Test_ptr);
            }
            else
            {
                STTBX_Print(("Failed\n"));
            }
        }
    }

    STTBX_Print(("No way to check deallocation, please check by other means ...\n"));

    /* No way to check effectiveness of deallocation !!! */
    return STOS_SUCCESS;
}

/*-------------------------------------------------------------------------
 * Function : TestReallocate_write_read
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestReallocate_write_read(void)
{
    U32               * Test_ptr;
    U32                 NbPtrEntries, NewNbPtrEntries;
    U32                 i;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* Tests ascending re-allocations */
    for (i=0 ; i < NB_ALLOCATIONS-1 ; i++)  /* all but one element */
    {
        NbPtrEntries = Allocation_sizes[i]/sizeof(U32);

        STTBX_Print(("Allocating %d: ", Allocation_sizes[i]));
        Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, 1, NbPtrEntries*sizeof(U32));   /* Allocates different sizes in bytes */
        if (Test_ptr != NULL)
        {
            STTBX_Print(("OK\n"));

            Test_ptr[0]                 = 0xDEADBEEF;
            Test_ptr[NbPtrEntries/2]    = 0xDEADBEEF;
            Test_ptr[NbPtrEntries - 1]  = 0xDEADBEEF;

            STTBX_Print((" Reallocating %d -> %d: ", Allocation_sizes[i], Allocation_sizes[i+1]));
            Test_ptr = (U32 *)STOS_MemoryReallocate(NULL, Test_ptr, Allocation_sizes[i+1], Allocation_sizes[i]);
            if (Test_ptr != NULL)
            {
                STTBX_Print(("OK\n"));

                NewNbPtrEntries = Allocation_sizes[i+1]/sizeof(U32);

                STTBX_Print((" Testing data integrity: "));
                if (   (Test_ptr[0] != 0xDEADBEEF)
                    || (Test_ptr[NbPtrEntries/2] != 0xDEADBEEF)
                    || (Test_ptr[NbPtrEntries - 1] != 0xDEADBEEF))
                {
                    goto error;
                }
                STTBX_Print(("OK\n"));

                /* Check effectiveness of allocation by writing dummy values */
                if (test_all_write_read(Test_ptr, NewNbPtrEntries) != STOS_SUCCESS)
                    goto error;

                STOS_MemoryDeallocate(NULL, Test_ptr);
            }
            else
            {
                STTBX_Print(("Failed\n"));
            }
        }
        else
        {
            STTBX_Print(("Failed\n"));
        }
    }

    /* Tests descending re-allocations */
    for (i=NB_ALLOCATIONS-1 ; i > 0 ; i--)      /* all but one element */
    {
        NbPtrEntries = Allocation_sizes[i]/sizeof(U32);

        STTBX_Print(("Allocating %d: ", Allocation_sizes[i]));
        Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, 1, NbPtrEntries*sizeof(U32));   /* Allocates different sizes in bytes */
        if (Test_ptr != NULL)
        {
            STTBX_Print(("OK\n"));

            Test_ptr[0]                 = 0xDEADBEEF;
            Test_ptr[NbPtrEntries/2]    = 0xDEADBEEF;
            Test_ptr[NbPtrEntries - 1]  = 0xDEADBEEF;

            STTBX_Print((" Reallocating %d -> %d: ", Allocation_sizes[i], Allocation_sizes[i-1]));
            Test_ptr = (U32 *)STOS_MemoryReallocate(NULL, Test_ptr, Allocation_sizes[i-1], Allocation_sizes[i]);
            if (Test_ptr != NULL)
            {
                STTBX_Print(("OK\n"));

                NewNbPtrEntries = Allocation_sizes[i-1]/sizeof(U32);

                STTBX_Print((" Testing data integrity: "));
                if (Test_ptr[0] != 0xDEADBEEF)
                    goto error;

                if (NbPtrEntries/2 < NewNbPtrEntries)
                {
                    if (Test_ptr[NbPtrEntries/2] != 0xDEADBEEF)
                        goto error;
                }
                STTBX_Print(("OK\n"));

                /* Check effectiveness of allocation by writing dummy values */
                if (test_all_write_read(Test_ptr, NewNbPtrEntries) != STOS_SUCCESS)
                    goto error;

                STOS_MemoryDeallocate(NULL, Test_ptr);
            }
            else
            {
                STTBX_Print(("Failed\n"));
            }
        }
        else
        {
            STTBX_Print(("Failed\n"));
        }
    }

    return STOS_SUCCESS;


    /* Error cases */
error:
    if (Test_ptr != NULL)
    {
        STOS_MemoryDeallocate(NULL, Test_ptr);
    }

    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : TestDeallocate_after_realloc
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestDeallocate_after_realloc(void)
{
    U32   * Test_ptr;
    U32     i;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* Tests ascending re-allocations */
    for (i=0 ; i < NB_ALLOCATIONS-1 ; i++)  /* all but one element */
    {
        STTBX_Print(("Allocating %d: ", Allocation_sizes[i]));
        Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, 1, Allocation_sizes[i]);   /* Allocates different sizes in bytes */
        if (Test_ptr != NULL)
        {
            STTBX_Print(("OK\n"));

            STTBX_Print((" Reallocating %d -> %d: ", Allocation_sizes[i], Allocation_sizes[i+1]));
            Test_ptr = (U32 *)STOS_MemoryReallocate(NULL, Test_ptr, Allocation_sizes[i+1], Allocation_sizes[i]);
            if (Test_ptr != NULL)
            {
                STTBX_Print(("OK\n"));

                STOS_MemoryDeallocate(NULL, Test_ptr);
            }
            else
            {
                STTBX_Print(("Failed\n"));
            }
        }
        else
        {
            STTBX_Print(("Failed\n"));
        }
    }

    /* Tests descending re-allocations */
    for (i=NB_ALLOCATIONS-1 ; i > 0 ; i--)      /* all but one element */
    {
        STTBX_Print(("Allocating %d: ", Allocation_sizes[i]));
        Test_ptr = (U32 *)STOS_MemoryAllocateClear(NULL, 1, Allocation_sizes[i]);   /* Allocates different sizes in bytes */
        if (Test_ptr != NULL)
        {
            STTBX_Print(("OK\n"));

            STTBX_Print((" Reallocating %d -> %d: ", Allocation_sizes[i], Allocation_sizes[i-1]));
            Test_ptr = (U32 *)STOS_MemoryReallocate(NULL, Test_ptr, Allocation_sizes[i-1], Allocation_sizes[i]);
            if (Test_ptr != NULL)
            {
                STTBX_Print(("OK\n"));

                STOS_MemoryDeallocate(NULL, Test_ptr);
            }
            else
            {
                STTBX_Print(("Failed\n"));
            }
        }
        else
        {
            STTBX_Print(("Failed\n"));
        }
    }

    STTBX_Print(("No way to check deallocation, please check by other means ...\n"));

    /* No way to check effectiveness of deallocation !!! */
    return STOS_SUCCESS;
}

/*-------------------------------------------------------------------------
 * Function : TestReallocate_block_NULL
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestReallocate_block_NULL(void)
{
    U32   * Test_ptr;
    U32     NbPtrEntries;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    NbPtrEntries = KILO/sizeof(U32);      /* Allocates 1K */

    STTBX_Print(("Allocating %d: ", NbPtrEntries*sizeof(U32)));
    Test_ptr = (U32 *)STOS_MemoryReallocate(NULL, NULL, NbPtrEntries*sizeof(U32), 0);
    if (Test_ptr != NULL)
    {
        STTBX_Print(("OK\n"));

        /* Check effectiveness of allocation by writing dummy values */
        if (test_all_write_read(Test_ptr, NbPtrEntries) != STOS_SUCCESS)
            goto error;

        STOS_MemoryDeallocate(NULL, Test_ptr);
    }
    else
    {
        goto error;
    }

    STTBX_Print(("Allocating %d: ", NbPtrEntries*sizeof(U32)));
    Test_ptr = (U32 *)STOS_MemoryReallocate(NULL, NULL, NbPtrEntries*sizeof(U32), MEGA);
    if (Test_ptr != NULL)
    {
        STTBX_Print(("OK\n"));

        /* Check effectiveness of allocation by writing dummy values */
        if (test_all_write_read(Test_ptr, NbPtrEntries) != STOS_SUCCESS)
            goto error;

        STOS_MemoryDeallocate(NULL, Test_ptr);
    }
    else
    {
        goto error;
    }

    return STOS_SUCCESS;


    /* Error cases */
error:
    STTBX_Print(("Error !!!\n"));
    if (Test_ptr != NULL)
    {
        STOS_MemoryDeallocate(NULL, Test_ptr);
    }

    return STOS_FAILURE;
}

/*-------------------------------------------------------------------------
 * Function : TestReallocate_size_zero
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestReallocate_size_zero(void)
{
    U32   * Test_ptr;
    U32     NbPtrEntries;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    NbPtrEntries = KILO/sizeof(U32);  /* Allocates 1K */

    STTBX_Print(("Allocating %d: ", NbPtrEntries*sizeof(U32)));
    Test_ptr = (U32 *)STOS_MemoryAllocate(NULL, NbPtrEntries*sizeof(U32));
    if (Test_ptr != NULL)
    {
        STTBX_Print(("OK\n"));

        Test_ptr = STOS_MemoryReallocate(NULL, Test_ptr, 0, NbPtrEntries*sizeof(U32));     /* No size means deallocation */
    }
    else
    {
        STTBX_Print(("Error !!!\n"));
    }

    STTBX_Print(("No way to check deallocation, please check by other means ...\n"));

    /* No way to check effectiveness of deallocation !!! */
    return STOS_SUCCESS;
}


/* end of memory.c */
