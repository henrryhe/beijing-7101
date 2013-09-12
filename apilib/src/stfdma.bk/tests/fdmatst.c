/*****************************************************************************
File Name   : fdmatst.c

Description : STFDMA development test harness

Copyright (C) 2002 STMicroelectronics

History     :

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

/* Only set the following for full data on each node created */
/*#define DISPLAY_NODE        1
#define STFDMA_TEST_VERBOSE 1*/

#define STTBX_PRINT 1   /*Always required on */

#if defined (ST_OSLINUX)
#include <linux/stddef.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <asm/semaphore.h>
#else
#include <string.h>                     /* C libs */
#include <stdio.h>
#include "sttbx.h"
#include "stcommon.h"
#endif

#include "stdevice.h"
#include "stddefs.h"
#if !defined (ST_OSLINUX)
#include "stboot.h"
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
#include "stpio.h"
#endif

#if defined (ST_5188)
/* For FEI tests */
#include "stsys.h"
#endif

#include "stfdma.h"                     /* fdma api header file */

#include "leak.h"                       /* Test case headers */
#include "soak.h"
#include "func.h"
#include "state.h"
#include "param.h"

#if !defined (ST_OSLINUX)
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
#include "audiopaced.h"
#endif
#endif

#ifdef STCM_TEST
#include "stcm/cpptest.h"
#endif

#if defined (ST_5301) || defined (ST_5525)
#include <machine/bsp.h>
#endif

#include "fdmatst.h"

#ifdef STFDMA_SIM
#include "fdmasim.h"
#endif

#if defined (ST_5525)
#define DISABLE_DCACHE 1
#endif
/*#define DISABLE_DCACHE 1
#define DISABLE_ICACHE 1*/

#if defined (ST_5105) || defined (ST_5107)
#define SET_REG(Offset, Value)   ( *(volatile U32*)(Offset) = ((U32)Value) )
#endif

#ifndef FDMA_BASE_ADDRESS
#if defined (ST_7100) || defined (ST_5528) || defined (ST_7109)
#define FDMA_BASE_ADDRESS 0xB9220000
#endif
#endif

/* Private Constants ------------------------------------------------------ */

/* Test count. Should only be adjusted when test added/removed */
#ifdef SOAK
#define NUMBER_TESTS 2
#elif !defined(ST_5517)
#define NUMBER_TESTS (61 + 6)
#else
#define NUMBER_TESTS 61
#endif

/* Utility defines */
#define K (1024)
#define MEG (1024 * K)

/* Test pattern */
#define DEFAULT_U32_TEST_PATTERN   0xDCAFC0FE

/* PIO defines for Pace request signal generation. 5517 values */
 #if defined (ST_5105) || defined (ST_5107)
#define PIO_REQ_SEND        PIO_BIT_6
#define PIO_REQ_RECEIVE_1   PIO_BIT_4
#define PIO_REQ_RECEIVE_2   PIO_BIT_5
#define SEND_BIT_IDX        6
#define RECEIVE_BIT_IDX     4
#else
#define PIO_REQ_SEND        PIO_BIT_4
#define PIO_REQ_RECEIVE_1   PIO_BIT_6
#define PIO_REQ_RECEIVE_2   PIO_BIT_5
#define SEND_BIT_IDX        4
#if defined (ST_5517)
#define RECEIVE_BIT_IDX     5
#else
#define RECEIVE_BIT_IDX     6
#endif
#endif

/* Memory structure defines...
 * NOTE: Changing these values will effect the performance of mixed dimension transfers if
 * multiples are not used. Also, fdmatst_CheckData() has built in, sometimes assumed
 * knowledge of the values used below.
 */

/* Basic word size used as building block for all other structures.
 * All other structures are multiples of this value.
 */
#if defined(ST_5517)
#define DEFAULT_TRANSFER_WORDSIZE   (8)
#else
#define DEFAULT_TRANSFER_WORDSIZE   (1)
#endif

/* Number of lines of data in 1d and 2d structures.
 * In a 2d memory structure each line is seperated by a gap of unused bytes.
 * In a 1d struture the data is contiguous.
 */
#if defined (ST_7100)  || defined (ST_7109) || defined (ST_5525) /* Arif */
#define DEFAULT_NUMBER_LINES         (5001)
#else
#define DEFAULT_NUMBER_LINES         (1001)
#endif

/* Defines for 0D memory structure.
 * For paced transfers the opcode size for request lines used MUST match the 0D wordsize.
 * For non-paced, the source or destination must be a multiple of the 0d wordsize.
 */

#define DEFAULT_0D_WORDSIZE         (DEFAULT_TRANSFER_WORDSIZE)

#define DEFAULT_REQ_WORSIZE         (DEFAULT_TRANSFER_WORDSIZE)

#define DEFAULT_0D_TRANSFER_COUNT    (32)

/* Defines for 1d memory structures.
 * The memory size must be mutliple of 0d wordsize for 0d to 1d to work properly.
 */

#define DEFAULT_1D_MEMSIZE           (DEFAULT_TRANSFER_WORDSIZE * DEFAULT_NUMBER_LINES)

#define DEFAULT_0D_TO_1D_TRANSFER_COUNT    (DEFAULT_1D_MEMSIZE)

/* Defines for 2d data structure definition.
 * Memory size must be multiple of word size used to construct the 0d and 1d memory structures
 * that are used as sources or destinations for correct operation.
 */
#define DEFAULT_2D_UNUSED       (4)  /* 4byte gap between useful data */

#define DEFAULT_2D_LINE_LENGTH  (DEFAULT_TRANSFER_WORDSIZE)

#define DEFAULT_STRIDE          (DEFAULT_2D_LINE_LENGTH + DEFAULT_2D_UNUSED)

#define DEFAULT_2D_MEMSIZE      (DEFAULT_STRIDE * DEFAULT_NUMBER_LINES)

#define DEFAULT_0D_TO_2D_TRANSFER_COUNT    (DEFAULT_2D_MEMSIZE - ( DEFAULT_NUMBER_LINES * DEFAULT_2D_UNUSED))

/* Declarations for memory partitions */

/* Sizes of partitions */
#if defined (ST_OS20)
#define INTERNAL_PARTITION_SIZE         (ST20_INTERNAL_MEMORY_SIZE-1400)
#define SYSTEM_PARTITION_SIZE           0x180000
#if defined (ST_5517)
#define NCACHE_PARTITION_SIZE           0x70000
#else
#define NCACHE_PARTITION_SIZE           0x300000
#endif
#elif defined (ST_OS21) && !defined (ST_OSWINCE)
/*#define SYSTEM_PARTITION_SIZE           0x180000
#define NCACHE_PARTITION_SIZE           0x200000*/
#define SYSTEM_PARTITION_SIZE           0x100000
#define NCACHE_PARTITION_SIZE           0x1500000
#elif defined(ST_OSWINCE)
#define SYSTEM_PARTITION_SIZE           0x100000
#define NCACHE_PARTITION_SIZE           0x600000
#define SHARED_NCACHE_BASE				0xA7A00000
#endif

#if defined ST_5100
#define SWTS_REGISTER_ADDRESS (ST5100_TSMERGE2_BASE_ADDRESS + (0x0280/4))
#elif defined ST_7100
#define SWTS_REGISTER_ADDRESS (ST7100_TSMERGE2_BASE_ADDRESS + (0x0280/4))
#elif defined ST_7109
#define SWTS_REGISTER_ADDRESS (ST7109_TSMERGE2_BASE_ADDRESS + (0x0280/4))
#elif defined ST_7710
#define SWTS_REGISTER_ADDRESS (ST7710_TSMERGE2_BASE_ADDRESS + (0x0280/4))
#elif defined ST_5528
#define SWTS_REGISTER_ADDRESS (ST5528_TSMERGE_BASE_ADDRESS + (0x0280/4))
#else
#define SWTS_REGISTER_ADDRESS (NULL)
#endif

/* Global Variables ------------------------------------------------------- */

/*rsisr_t Rsisr;*/
U32 mailbox_interrupt_received;

/* For STSYS_ST200UncachedOffset */
#if defined(ST200_OS21)
/* Variable needed by the STSYS driver to remember the offset to add to
 * a physical address to have a CPU uncached address to the memory. This
 * is more efficient than calling mmap_translate_uncached every time we
 * make an uncached access */
S32 STSYS_ST200UncachedOffset;
#endif /* defined(ST200_OS21) */

/* Private Variables ------------------------------------------------------ */

#if defined (ST_OS20)
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static U8               system_block [SYSTEM_PARTITION_SIZE];
static U8               ncache_block [NCACHE_PARTITION_SIZE];

partition_t             *internal_partition;
partition_t             *system_partition;
ST_Partition_t          *ncache_partition;

partition_t             the_internal_partition;
#pragma ST_section      (internal_block, "internal_section_noinit")

partition_t             the_system_partition;
#pragma ST_section      (system_block,   "system_section_noinit")

partition_t             the_ncache_partition;
#pragma ST_section      (ncache_block,   "ncache_section")

/* This is to avoid a linker warning */
#if defined(ST_5528)|| defined(ST_5100) || defined(ST_7710) ||defined(ST_5105) || defined (ST_5188) ||\
defined(ST_7100) || defined(ST_5700) || defined(ST_7109) || defined (ST_5525) || defined (ST_5107)

static U8               data_section[1];
#pragma ST_section      ( data_section,   "data_section")
#endif

static unsigned char    internal_block_init[1];
#pragma ST_section      ( internal_block_init, "internal_section")

static unsigned char    system_block_init[1];
#pragma ST_section      ( system_block_init, "system_section")

#elif defined (ST_OS21)
static U8               system_block [SYSTEM_PARTITION_SIZE];
partition_t             *system_partition;

static U8               ncache_block [NCACHE_PARTITION_SIZE];
ST_Partition_t          *ncache_partition;

#elif defined (ST_OSLINUX)
partition_t             *system_partition = NULL;
#endif

ST_ErrorCode_t          retVal;
#if !defined (ST_OSLINUX)
#pragma ST_section      ( retVal,   "ncache_section")
#endif

#if defined (ST_5517)
/* SMI configuration for SMi to SMI test */
#define ACTUAL_SMI_BASE             0xC0000000
#define SMI_PARTITION_SIZE  (3 * MEG)
partition_t             the_smi_partition;
ST_Partition_t          *smi_partition = &the_smi_partition;
#endif

/* Harness revision value */
static const ST_Revision_t stfdmatst_Rev = "2.2.0";

/* Success tracking varaibles */
U32 TestCasesExpected = NUMBER_TESTS;

typedef struct
{
    U32  Run;
    U32  Passed;
    U32  Failed;
}fdmatst_TestResult_t;

fdmatst_TestResult_t TestCaseResults = {0};
fdmatst_TestResult_t TestCasesTotal = {0};

BOOL TestCaseOk = TRUE;

/* For tracking aligned node allocation/deallocation. Nodes are allocated on
 * 32 byte boundaries so their deallocate pointer must be stored to enable
 * deallocation.
 */
#define MAX_NUM_NODES 20

typedef struct
{
    STFDMA_GenericNode_t  *DeallocateNode_p; /* Deallocate pointer */
    STFDMA_GenericNode_t  *Node_p;           /* Node start pointer */
#if defined (ST_OSLINUX)
    dma_addr_t             physical_address;
#endif
}fdmatst_ActiveNodes_t;

static fdmatst_ActiveNodes_t  ActiveNodes[MAX_NUM_NODES];

static U32      ActiveNodeCount = 0;

/* For tracking src and destintion buffer usage. Since they can aligned
 * or unaligned on 32byte boundaries the address stored in the Src_p and
 * Dest_p of the Node maybe not be suitable for deallocation of the buffer
 * There is no count of buffers used since it directly related to ActiveNodeCount.
 */
typedef struct
{
    U32        *DeallocateSrc_p;   /* Deallocate pointer for src buffer */
    U32        *DeallocateDest_p;  /* Deallocate pointer for dest buffer */
    U32         SrcBuffSize;
    U32         DestBuffSize;
#if defined (ST_OSLINUX)
    dma_addr_t  src_physical_address;
    dma_addr_t  dest_physical_address;
    U32        *Src_p;
    U32        *Dest_p;
#endif
}fdmatst_ActiveBuffers_t;

static fdmatst_ActiveBuffers_t  ActiveBuffers[MAX_NUM_NODES];

/* Local device access pointer */
static U32* BaseAddress_p;

/* For pace signal testing */
typedef struct
{
    U32 *Src_p;
    U32 *Dest_p;
    U32 *Next_p;
    BOOL Src0D;
    BOOL Dest0D;
    U32 NumberBytes;
    U32 BytesPerSec;
}PaceRequestGen_t;

static semaphore_t* pActiveNodeTableSemap;
#if !defined (STFDMA_NO_PACED_TESTS)
static semaphore_t* pWakeupRequestGenerator;
static semaphore_t* pRequestGeneratorAlive;
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
#if defined (ST_OSLINUX)
static int   PaceRequestGeneratorTask(void * TaskConfig);
#else
static void  PaceRequestGeneratorTask(void * TaskConfig);
#endif
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
#if defined (ST_OSLINUX)
static struct task_struct* pPaceReqTaskId;
#else
static task_t*  pPaceReqTaskId;
#endif
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
static BOOL RequestGeneratorSuicide = FALSE;
static BOOL RequestGeneratorStop = TRUE;
#endif

#if defined (ARCHITECTURE_ST40)
static void fdmatst_CovertNodeAddressDataToCPU(STFDMA_Node_t* pFirstNode);
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
static U8      *PaceReqTaskStack_p = NULL;
static tdesc_t *PaceReqTaskDesc = NULL;
#endif

#if defined (ST_OS20)
#define TASK_STACK_SIZE     1024
#elif defined (ST_OS21)
#define TASK_STACK_SIZE     (OS21_DEF_MIN_STACK_SIZE + 1024)
#else
#define TASK_STACK_SIZE     1024
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
static  STPIO_Handle_t  g_STPIOHandle;
#endif
#if defined (ST_OSLINUX)
static char*            g_NodePoolName = "FDMANodePool";
struct dma_pool* g_NodePool;
#endif

#if defined (ST_5301) || defined (ST_5525)
/****************************************************************************/
/**
 * @brief Turns off all caching of LMI
 *
 * Turns off caching of LMI from 0xc000_0000 to 0xC200_0000
 *
 ****************************************************************************/
void DCacheOff(void)
{
#if defined (ST_5525)
          bsp_mmu_memory_unmap( (void *)(0x80000000), 0x02000000);
#else
          bsp_mmu_memory_unmap( (void *)(0xC0000000), 0x02000000);
#endif

#if defined (ST_5525)
        bsp_mmu_memory_map((void *)(0x80000000),
                        0x02000000,
                        PROT_USER_EXECUTE|
                        PROT_USER_WRITE|
                        PROT_USER_READ|
                        PROT_SUPERVISOR_EXECUTE|
                        PROT_SUPERVISOR_WRITE|
                        PROT_SUPERVISOR_READ,
                        MAP_SPARE_RESOURCES|
                        MAP_FIXED|
                        MAP_OVERRIDE,
                        (void *)(0x80000000));

#else
          bsp_mmu_memory_map((void *)(0xc0000000),
                           0x02000000,
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES,
                           (void *)(0xc0000000));

#endif
}
#endif

/* Functions -------------------------------------------------------------- */
/****************************************************************************
Name         : fdmatst_AllocSrcBuffer
Description  : Allocate the source buffer
Parameters   : None
****************************************************************************/
void fdmatst_AllocSrcBuffer(void)
{
#if defined (ST_OSLINUX)
    ActiveBuffers[ActiveNodeCount].DeallocateSrc_p = (U32 *)dma_alloc_coherent(
        NULL,
        ActiveBuffers[ActiveNodeCount].SrcBuffSize,
        &(ActiveBuffers[ActiveNodeCount].src_physical_address),
        GFP_KERNEL);

     VERBOSE_PRINT_DATA(" DeallocateSrc_p      : 0x%08x\n", (U32)ActiveBuffers[ActiveNodeCount].DeallocateSrc_p);
     VERBOSE_PRINT_DATA(" src_physical_address : 0x%08x\n", ActiveBuffers[ActiveNodeCount].src_physical_address);
     VERBOSE_PRINT_DATA(" SrcBuffSize          : %d\n", ActiveBuffers[ActiveNodeCount].SrcBuffSize);

     memset(ActiveBuffers[ActiveNodeCount].DeallocateSrc_p, 0 , ActiveBuffers[ActiveNodeCount].SrcBuffSize);
#else
    ActiveBuffers[ActiveNodeCount].DeallocateSrc_p = (U32 *) STOS_MemoryAllocateClear(
        fdmatst_GetNCachePartition(),
        1,
        ActiveBuffers[ActiveNodeCount].SrcBuffSize);
#endif
}

/****************************************************************************
Name         : fdmatst_AllocDestBuffer
Description  : Allocate the destination buffer
Parameters   : None
****************************************************************************/
void fdmatst_AllocDestBuffer(void)
{
#if defined (ST_OSLINUX)
    ActiveBuffers[ActiveNodeCount].DeallocateDest_p = (U32 *)dma_alloc_coherent(
        NULL,
        ActiveBuffers[ActiveNodeCount].DestBuffSize,
        &(ActiveBuffers[ActiveNodeCount].dest_physical_address),
        GFP_KERNEL);

     VERBOSE_PRINT_DATA(" DeallocateDest_p      : 0x%08x\n", (U32)ActiveBuffers[ActiveNodeCount].DeallocateDest_p);
     VERBOSE_PRINT_DATA(" dest_physical_address : 0x%08x\n", ActiveBuffers[ActiveNodeCount].dest_physical_address);
     VERBOSE_PRINT_DATA(" DestBuffSize          : %d\n", ActiveBuffers[ActiveNodeCount].DestBuffSize);

     memset(ActiveBuffers[ActiveNodeCount].DeallocateDest_p, 0 , ActiveBuffers[ActiveNodeCount].DestBuffSize);

#else
    ActiveBuffers[ActiveNodeCount].DeallocateDest_p = (U32 *) STOS_MemoryAllocateClear(
        fdmatst_GetNCachePartition(),
        1,
        ActiveBuffers[ActiveNodeCount].DestBuffSize);
#endif
}

/****************************************************************************
Name         : fdmatst_AllocNode
Description  : Allocate a node
Parameters   : None
****************************************************************************/
STFDMA_GenericNode_t* fdmatst_AllocNode(void)
{
    STFDMA_GenericNode_t* Node_p = NULL;

#if defined (ST_OSLINUX)
    ActiveNodes[ActiveNodeCount].DeallocateNode_p =
        (STFDMA_GenericNode_t *)dma_pool_alloc(g_NodePool,
        GFP_KERNEL,
        &(ActiveNodes[ActiveNodeCount].physical_address));

    VERBOSE_PRINT_DATA(" DeallocateNode_p : 0x%x\n", (U32)ActiveNodes[ActiveNodeCount].DeallocateNode_p);
    VERBOSE_PRINT_DATA(" physical_address : 0x%x\n", ActiveNodes[ActiveNodeCount].physical_address);

#else
    /* Nodes must be 32 byte aligned... */

    /* Create node+31bytes and store pointer for deallocation later */
    ActiveNodes[ActiveNodeCount].DeallocateNode_p = (STFDMA_GenericNode_t *)
        (STOS_MemoryAllocateClear(fdmatst_GetNCachePartition(),1, (sizeof(STFDMA_GenericNode_t)+31)));
#endif

    if (ActiveNodes[ActiveNodeCount].DeallocateNode_p == NULL)
    {
        VERBOSE_PRINT("*** ERROR: Node creation: Not enough memory\n");
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
    }
    else
    {
#if defined (ST_OSLINUX)
        Node_p = (STFDMA_GenericNode_t*)ActiveNodes[ActiveNodeCount].DeallocateNode_p;
#else
    /* Ensure the node pointer is aligned on a 32byte boundary */
        Node_p = (STFDMA_GenericNode_t*) (((U32)ActiveNodes[ActiveNodeCount].DeallocateNode_p + 31) & ~31);
#endif

#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
        cache_purge_data(ActiveNodes[ActiveNodeCount].DeallocateNode_p, (sizeof(STFDMA_GenericNode_t)+31));
        Node_p = (STFDMA_GenericNode_t*)ADDRESS_TO_P2(Node_p);
#endif
    }

    ActiveNodes[ActiveNodeCount].Node_p = (STFDMA_GenericNode_t *)Node_p;

    return Node_p;
}

/****************************************************************************
Name         : fdmatst_DeallocNode
Description  : Allocate a node
Parameters   : None
****************************************************************************/
void fdmatst_DeallocNode(void)
{
#if defined (ST_OSLINUX)
    dma_pool_free(g_NodePool,
        ActiveNodes[ActiveNodeCount].DeallocateNode_p,
        ActiveNodes[ActiveNodeCount].physical_address);
#else
    STOS_MemoryDeallocate(fdmatst_GetNCachePartition(), ActiveNodes[ActiveNodeCount].DeallocateNode_p);
#endif
}

/****************************************************************************
Name         : fdmatst_CreateNode
Description  : Creates a node and associated buffers according the params
               given.
Parameters   : SrcDim : source buffere dimension. 0 = 0D, 1= 1D, 2 = 2D
               SrcYDir : increment direction of src buffer. 0 = inc, 1 = dec
               SrcAligned: True indicates source data structre is 32bytes aligned
               DestDim : destination buffer dimension.
               DestYDir : increment direction of dest buffer.
               DestAligned: True indicates source data structre is 32bytes aligned
               NCN : Node complete notify flag
               NCP : Node complete paused flag
               PaceSignal: Pace signal to use
               FirstNode_p : Ptr to direct to created node

               - A 1D arrangement is a linear array of DEFAULT_MEMSIZE.
               - A 2D arrangement is usefull data bytes, and  gap bytes
                 repeated for DEFAULT_MEMSIZE number of bytes.

               A paced signal node relies on the configuration in the fdmareq.h
               driver source file for word size definition. This function assumes
               4 byte wide locations will be used for source and destination with
               a fixed number of transfers occurring.
               When pacing to/from a 0D location the memsize is a fixed amout
               governed by the 0D location size (4bytes) and the number of bytes
               to write is fixed by #define.


Return Value : 0 : ok.
               ST_NO_MEMORY : no space
****************************************************************************/
ST_ErrorCode_t fdmatst_CreateNode(U8 SrcDim, U8 SrcYDir, BOOL SrcAligned,
                                  U8 DestDim, U8  DestYDir, BOOL DestAligned,
                                  BOOL NodeCompleteNotify, BOOL NodeCompletePause,
                                  U32 PaceSignal, STFDMA_Node_t **FirstNode_p)

{
    STFDMA_Node_t       *Node_p;
    U32                 i = 0;
    U32                 TestValue = 0;
    U32                 SrcMemSize = 0;
    U32                 DestMemSize = 0;
    U32                 *Src_p;
    U32                 *Dest_p;

    /* create memory for node */
    VERBOSE_PRINT("Creating memory for node\n");

    STOS_SemaphoreWait(pActiveNodeTableSemap);

    if (ActiveNodeCount < MAX_NUM_NODES)
    {
        Node_p = (STFDMA_Node_t*)fdmatst_AllocNode();

        if (Node_p == NULL)
        {
            return ST_ERROR_NO_MEMORY;
        }
    }
    else
    {
        VERBOSE_PRINT("*** ERROR: Node creation: Not enough memory in deallocation pointer table\n");
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

    /* Define memory size to use dependant on dimensions */
    if (SrcDim == FDMATST_DIM_0D)
    {
        /* Memsize is single 0 location */
        SrcMemSize = DEFAULT_0D_WORDSIZE;
    }
    else  if (SrcDim == FDMATST_DIM_1D)
    {
        SrcMemSize = DEFAULT_1D_MEMSIZE;
    }
    else
    {
        SrcMemSize = DEFAULT_2D_MEMSIZE;
    }

    if (DestDim == FDMATST_DIM_0D)
    {
        /* Memsize is single 0 location */
        DestMemSize = DEFAULT_0D_WORDSIZE;
    }
    else  if (DestDim == FDMATST_DIM_1D)
    {
        DestMemSize = DEFAULT_1D_MEMSIZE;
    }
    else
    {
        DestMemSize = DEFAULT_2D_MEMSIZE;
    }

    if(PaceSignal != STFDMA_REQUEST_SIGNAL_NONE)
    {
       /* Provision for the two ..MultipleDreq.. tests*/
#if defined (ST_7109)
    	if(PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS1)
#elif defined (ST_5105) || defined (ST_5188) || defined (ST_5107) || defined (ST_7200)
        if(PaceSignal == STFDMA_REQUEST_SIGNAL_SSC1_TX)
#else
    	if(PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS)
#endif
    	{
    		SrcMemSize = 1024*1024;
    	}
    }

    /* Create source and destination memory spaces */
    VERBOSE_PRINT("Creating memory source and destination\n");

    if (DestAligned == FALSE)
    {
        ActiveBuffers[ActiveNodeCount].DestBuffSize = DestMemSize + 35;
    }
    else
    {
        ActiveBuffers[ActiveNodeCount].DestBuffSize = DestMemSize + 31;
    }

    fdmatst_AllocDestBuffer();

    if (ActiveBuffers[ActiveNodeCount].DeallocateDest_p == NULL)
    {
        VERBOSE_PRINT("*** ERROR: Destination mem creation: Not enough memory\n");
        fdmatst_DeallocNode();
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
    cache_purge_data(ActiveBuffers[ActiveNodeCount].DeallocateDest_p, ActiveBuffers[ActiveNodeCount].DestBuffSize);
#endif

    if (DestAligned == FALSE)
    {
        /* Adjust pointer point to a NON 32byte aligned address */
        Dest_p = (U32 *) ((((U32)ActiveBuffers[ActiveNodeCount].DeallocateDest_p + 31) & ~31) + 4);
    }
    else
    {
        /* Adjust pointer to point to a 32byte aligned address */
        Dest_p = (U32 *) (((U32)ActiveBuffers[ActiveNodeCount].DeallocateDest_p + 31) & ~31);
    }

    if (SrcAligned == FALSE)
    {
        ActiveBuffers[ActiveNodeCount].SrcBuffSize = SrcMemSize + 35;
    }
    else
    {
        ActiveBuffers[ActiveNodeCount].SrcBuffSize = SrcMemSize + 31;
    }

    /* Random memory allocation */

    fdmatst_AllocSrcBuffer();

    if (ActiveBuffers[ActiveNodeCount].DeallocateSrc_p == NULL)
    {
        VERBOSE_PRINT("*** ERROR: Source mem creation: Not enough memory\n");
        fdmatst_DeallocNode();
#if defined (ST_OSLINUX)
        dma_free_coherent(NULL,
            ActiveBuffers[ActiveNodeCount].DestBuffSize,
            ActiveBuffers[ActiveNodeCount].DeallocateDest_p,
            ActiveBuffers[ActiveNodeCount].dest_physical_address);
#else
        STOS_MemoryDeallocate(fdmatst_GetNCachePartition(), ActiveBuffers[ActiveNodeCount].DeallocateDest_p);
#endif
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
    cache_purge_data(ActiveBuffers[ActiveNodeCount].DeallocateSrc_p, ActiveBuffers[ActiveNodeCount].SrcBuffSize);
#endif

    if (SrcAligned == FALSE)
    {
        /* Adjust pointer to point to an NON 32byte aligned address */
        Src_p = (U32 *) ((((U32)ActiveBuffers[ActiveNodeCount].DeallocateSrc_p + 31) & ~31) + 4);
    }
    else
    {
        /* Adjust pointer to point to a 32byte aligned address */
        Src_p = (U32 *) (((U32)ActiveBuffers[ActiveNodeCount].DeallocateSrc_p + 31) & ~31);
    }

#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
        Src_p = (U32*)ADDRESS_TO_P2(Src_p);
        Dest_p = (U32*)ADDRESS_TO_P2(Dest_p);
#endif

#if defined (ST_OSLINUX)
    ActiveBuffers[ActiveNodeCount].Src_p = Src_p;
    ActiveBuffers[ActiveNodeCount].Dest_p = Dest_p;
#endif
    /* Update private count of active nodes */
    ActiveNodeCount++;

    /* Node table update complete. ok to release semap */
    STOS_SemaphoreSignal(pActiveNodeTableSemap);

    /* Generic node config */
    Node_p->Next_p                         = NULL;

    if((Node_p->NodeControl.PaceSignal = PaceSignal) != STFDMA_REQUEST_SIGNAL_NONE)
    {
        /* Provision for the two ..MultipleDreq.. tests*/
#if defined(ST_7109)
    	if(Node_p->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS1)
#elif defined (ST_5105) || defined (ST_5188) || defined (ST_5107) || defined (ST_7200)
        if(PaceSignal == STFDMA_REQUEST_SIGNAL_SSC1_TX)
#else
        if(Node_p->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS)
#endif
    	{
#if defined (ST_5105)
            Dest_p = (U32 *)ST5105_SSC1_BASE_ADDRESS;
#elif defined (ST_5107)
            Dest_p = (U32 *)ST5107_SSC1_BASE_ADDRESS;
#elif defined (ST_5188)
            Dest_p = (U32 *)ST5188_SSC1_BASE_ADDRESS;
#elif defined (ST_7200)
            Dest_p = (U32 *)ST7200_SSC1_BASE_ADDRESS;
#else
    		Dest_p = (U32 *)SWTS_REGISTER_ADDRESS;
#endif
    	}
    }

    Node_p->NodeControl.NodeCompleteNotify = NodeCompleteNotify;
    Node_p->NodeControl.NodeCompletePause  = NodeCompletePause;
#if defined (ST_7109)
    Node_p->NodeControl.Reserved           = 0;
    Node_p->NodeControl.Secure             = 0;
    Node_p->NodeControl.Reserved1          = 0;
#else
    Node_p->NodeControl.Reserved           = 0;
#endif

    /* Node details config for possible combinations of transfer dimenstions... */

    /* Source is 0D.... */
    if (SrcDim == FDMATST_DIM_0D)
    {
        /* Fill source memory space to known values */
        TestValue = 0xf1;
        for (i=0; (i != DEFAULT_0D_WORDSIZE); i++)
        {
            ((U8*)Src_p)[i] = (U8)TestValue++;
        }

        Node_p->SourceStride = 0;
        Node_p->NodeControl.SourceDirection = STFDMA_DIRECTION_STATIC;

        /* Node config depending on destination memory type... */

        /* Destination is 0D...*/
        if (DestDim == FDMATST_DIM_0D)
        {
            Node_p->DestinationStride = 0;
            Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;
            Node_p->NumberBytes = DEFAULT_0D_TRANSFER_COUNT;
            Node_p->Length = 0;
        }
        /* Destination is 1D...*/
        else if (DestDim == FDMATST_DIM_1D)
        {

            Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
            Node_p->NumberBytes = DEFAULT_0D_TO_1D_TRANSFER_COUNT;
            Node_p->Length = DEFAULT_0D_WORDSIZE;
            Node_p->DestinationStride = Node_p->Length;
        }
        /* Destination is 2D.... */
        else /*  assume FDMATST_DIM_2D */
        {
            /* NOTE: Requires 0D source structure to be same size as DEFAULT_2D_LINE_LENGTH */
            Node_p->NumberBytes = DEFAULT_0D_TO_2D_TRANSFER_COUNT;
            Node_p->Length = DEFAULT_0D_WORDSIZE;
            Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
            if (DestYDir == FDMATST_Y_INC)
            {
                Node_p->DestinationStride = DEFAULT_STRIDE;
            }
            else
            {
                Node_p->DestinationStride = -DEFAULT_STRIDE;

                /* Need to offset destination start address to first address in last line prior to DMA start */
                Dest_p = (U32*) (((U8 *)Dest_p + DEFAULT_2D_MEMSIZE) - DEFAULT_STRIDE);
            }
        }
    }
    /* Source is 1D....*/
    else if (SrcDim == FDMATST_DIM_1D)
    {
        /* Fill all source memory space to known values */
        TestValue = 0xe1;
        for (i=0; (i < SrcMemSize); i++)
        {
            ((U8*)Src_p)[i] = TestValue++;
        }
        /* 1d common node config */
        Node_p->NodeControl.SourceDirection = STFDMA_DIRECTION_INCREMENTING;
        Node_p->NumberBytes = SrcMemSize;

        /* Node config dependant on dest memory structure.... */
        /* Destination is 0D...*/
        if (DestDim == FDMATST_DIM_0D)
        {
            Node_p->Length = DEFAULT_0D_WORDSIZE;
            Node_p->SourceStride = 0;
            Node_p->DestinationStride = 0;
            Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;
        }
        /* Desintation is 1D...*/
        else if  (DestDim == FDMATST_DIM_1D)
        {
            /* Fill dest with non-0 */
            TestValue = 0X01;
            for (i=0; (i < DestMemSize); i++)
            {
                ((U8*)Dest_p)[i] = TestValue++;
            }
            Node_p->Length = Node_p->NumberBytes;
            Node_p->SourceStride = 0;
            Node_p->DestinationStride = 0;
            Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
        }
        /* Desintation is 2D..*/
        else /*  assume FDMATST_DIM_2D */
        {
            Node_p->Length = DEFAULT_2D_LINE_LENGTH;
            Node_p->SourceStride = Node_p->Length;
            Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
            if (DestYDir == FDMATST_Y_INC)
            {
                Node_p->DestinationStride = DEFAULT_STRIDE;
            }
            else
            {
                Node_p->DestinationStride = -DEFAULT_STRIDE;

                /* Need to offset destination start address to first address in last line prior to DMA start */
                Dest_p = (U32*) (((U8 *)Dest_p + DEFAULT_2D_MEMSIZE) - DEFAULT_STRIDE);
            }
        }
    }
    /* Source is 2D....*/
    else /* assume SrcDim == FDMATST_DIM_2D */
    {
        /* Fill the source memory space to known values.
           Set Usefull data to test value and leave gaps at 0.
         */
        TestValue = 1;
        for (i=0; (i < DEFAULT_2D_MEMSIZE); i += 1)
        {
            ((U8*)Src_p)[i] = TestValue++;
        }

        /* Common node config...*/
        /* Direction of source stride */
        if (SrcYDir == FDMATST_Y_INC)
        {
            Node_p->SourceStride = DEFAULT_STRIDE;
        }
        else
        {
            Node_p->SourceStride = -DEFAULT_STRIDE;

            /* Need to offset source start address to first address in last line prior to DMA start */
            Src_p = (U32*) (((U8*)Src_p + DEFAULT_2D_MEMSIZE) - DEFAULT_STRIDE);
        }
        Node_p->NodeControl.SourceDirection = STFDMA_DIRECTION_INCREMENTING;
        Node_p->Length = DEFAULT_2D_LINE_LENGTH;

        /* SrcMemSize comprises usefull and unused data. NumberBytes is useful data only.
         * To find NumberBytes, remove the amount of unused data from the overall source memory
         * size to leave the amount of useful data.
         */
        Node_p->NumberBytes = (SrcMemSize - ((SrcMemSize / DEFAULT_STRIDE) * DEFAULT_2D_UNUSED));


        /* Destination is 0d....*/
        if (DestDim == FDMATST_DIM_0D)
        {
            /* NOTE: Requires destination 0d location to be same size as 2d line length */
            Node_p->DestinationStride = 0;
            Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;

        }
        /* Destination is 1D...*/
        else if (DestDim == FDMATST_DIM_1D)
        {
            Node_p->DestinationStride = Node_p->Length;
            Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
        }
        /* Destination is 2D...*/
        else /*  assume FDMATST_DIM_2D */
        {
            Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
            if (DestYDir == FDMATST_Y_INC)
            {
                Node_p->DestinationStride = DEFAULT_STRIDE;
            }
            else
            {
                Node_p->DestinationStride = -DEFAULT_STRIDE;

                /* Need to offset destination start address to first address in last line prior to DMA start */
                Dest_p = (U32*) (((U8 *)Dest_p + DEFAULT_2D_MEMSIZE) - DEFAULT_STRIDE);
            }
        }
    }

#if defined (ST_OSLINUX)
    ActiveBuffers[ActiveNodeCount - 1].Src_p = Src_p;
    ActiveBuffers[ActiveNodeCount - 1].Dest_p = Dest_p;
#endif

    /* Setup node source and destination ptrs  */
    Node_p->DestinationAddress_p = Dest_p;
    Node_p->SourceAddress_p = Src_p;

    VERBOSE_PRINT_DATA(" Node: 0x%x\n",(U32)Node_p);
#ifdef DISPLAY_NODE
    VERBOSE_PRINT_DATA(" Control.PaceSignal         : 0x%x\n",Node_p->NodeControl.PaceSignal);
    VERBOSE_PRINT_DATA(" Control.SrcDir             : 0x%x\n",Node_p->NodeControl.SourceDirection);
    VERBOSE_PRINT_DATA(" Control.DestDir            : 0x%x\n",Node_p->NodeControl.DestinationDirection);
    VERBOSE_PRINT_DATA(" Control.Reserved           : 0x%x\n",Node_p->NodeControl.Reserved);
    VERBOSE_PRINT_DATA(" Control.NodeCompletePause  : 0x%x\n",Node_p->NodeControl.NodeCompletePause);
    VERBOSE_PRINT_DATA(" Control.NodeCompleteNotify : 0x%x\n",Node_p->NodeControl.NodeCompleteNotify);
    VERBOSE_PRINT_DATA(" NumberBytes                : %d\n",  Node_p->NumberBytes);
    VERBOSE_PRINT_DATA(" SrcAddress_p               : 0x%x\n",(U32)Node_p->SourceAddress_p);
    VERBOSE_PRINT_DATA(" DestAddress_p              : 0x%x\n",(U32)Node_p->DestinationAddress_p);
    VERBOSE_PRINT_DATA(" Length                     : %d\n",  Node_p->Length);
    VERBOSE_PRINT_DATA(" SrcStride                  : %d\n",  Node_p->SourceStride);
    VERBOSE_PRINT_DATA(" DestStride                 : %d\n",  Node_p->DestinationStride);
#endif

    /* return node */
    *FirstNode_p = Node_p;

    return ST_NO_ERROR;
}

#if defined (ST_5188)
/****************************************************************************
Name         : fdmatst_CreateFEINode
Description  : Creates a node and associated buffers for an FEC transfer according
               the params given.
Parameters   : SrcDim : source buffere dimension. 0 = 0D, 1= 1D, 2 = 2D
               SrcYDir : increment direction of src buffer. 0 = inc, 1 = dec
               SrcAligned: True indicates source data structre is 32bytes aligned
               DestDim : destination buffer dimension.
               DestYDir : increment direction of dest buffer.
               DestAligned: True indicates source data structre is 32bytes aligned
               NCN : Node complete notify flag
               NCP : Node complete paused flag
               FirstNode_p : Ptr to direct to created node
Return Value : 0 : ok.
               ST_NO_MEMORY : no space
****************************************************************************/
ST_ErrorCode_t fdmatst_CreateFEINode(U8 SrcDim,                 U8 SrcYDir,             BOOL SrcAligned,
                                     U8 DestDim,                U8  DestYDir,           BOOL DestAligned,
                                     BOOL NodeCompleteNotify,   BOOL NodeCompletePause, U32 NumPackets,
                                     STFDMA_GenericNode_t **FirstNode_p)
{
    STFDMA_GenericNode_t   *Node_p;
    U32                     SrcMemSize = 0;
    U32                     DestMemSize = 0;
    U32                    *Src_p;
    U32                    *Dest_p;

    /* create memory for node */
    VERBOSE_PRINT("Creating memory for node\n");

    STOS_SemaphoreWait(pActiveNodeTableSemap);

    if (ActiveNodeCount < MAX_NUM_NODES)
    {
        Node_p = (STFDMA_GenericNode_t*)fdmatst_AllocNode();

        if (Node_p == NULL)
        {
            return ST_ERROR_NO_MEMORY;
        }
    }
    else
    {
        VERBOSE_PRINT("*** ERROR: Node creation: Not enough memory in deallocation pointer table\n");
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

    /* Define memory size to use dependending upon dimensions */
    if (SrcDim == FDMATST_DIM_0D)
    {
        /* Memsize is single 0 location */
        SrcMemSize = DEFAULT_0D_WORDSIZE;
    }
    else  if (SrcDim == FDMATST_DIM_1D)
    {
        SrcMemSize = DEFAULT_1D_MEMSIZE;
    }
    else
    {
        SrcMemSize = DEFAULT_2D_MEMSIZE;
    }

    if (DestDim == FDMATST_DIM_0D)
    {
        /* Memsize is single 0 location */
        DestMemSize = DEFAULT_0D_WORDSIZE;
    }
    else  if (DestDim == FDMATST_DIM_1D)
    {
        DestMemSize = DEFAULT_1D_MEMSIZE;
    }
    else
    {
        DestMemSize = DEFAULT_2D_MEMSIZE;
    }

    DestMemSize = 192*(NumPackets+1);

    /* Create source and destination memory spaces */
    VERBOSE_PRINT("Creating memory source and destination\n");

    if (DestAligned == FALSE)
    {
        ActiveBuffers[ActiveNodeCount].DestBuffSize = DestMemSize + 127;
    }
    else
    {
        ActiveBuffers[ActiveNodeCount].DestBuffSize = DestMemSize + 127;
    }

    fdmatst_AllocDestBuffer();

    if (ActiveBuffers[ActiveNodeCount].DeallocateDest_p == NULL)
    {
        VERBOSE_PRINT("*** ERROR: Destination mem creation: Not enough memory\n");
        fdmatst_DeallocNode();
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
    cache_purge_data(ActiveBuffers[ActiveNodeCount].DeallocateDest_p, ActiveBuffers[ActiveNodeCount].DestBuffSize);
#endif

    if (DestAligned == FALSE)
    {
        /* Adjust pointer to point to a NON 32byte aligned address */
        Dest_p = (U32 *) ((((U32)ActiveBuffers[ActiveNodeCount].DeallocateDest_p + 127) & ~127 ));
    }
    else
    {
        /* Adjust pointer to point to a 32byte aligned address */
        Dest_p = (U32 *) (((U32)ActiveBuffers[ActiveNodeCount].DeallocateDest_p + 127) & ~127);
    }

    /* Src_p = FEI_OUT_DATA */
    Src_p = (U32 *) (FEI_OUT_DATA);

    /* Random memory allocation */

#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
    cache_purge_data(ActiveBuffers[ActiveNodeCount].DeallocateSrc_p, ActiveBuffers[ActiveNodeCount].SrcBuffSize);
#endif

    /* Update private count of active nodes */
    ActiveNodeCount++;

    /* Node table update complete. ok to release semap */
    STOS_SemaphoreSignal(pActiveNodeTableSemap);

    /* FEI node details */
    Node_p->FEINode.Next_p                          = NULL;
    Node_p->FEINode.Extended                        = STFDMA_EXTENDED_NODE;
    Node_p->FEINode.Type                            = STFDMA_EXT_NODE_FEI;
    Node_p->FEINode.DreqStart                       = STFDMA_REQUEST_SIGNAL_FEI_DREQSTART;
    Node_p->FEINode.DreqData                        = STFDMA_REQUEST_SIGNAL_FEI_DREQ;
    Node_p->FEINode.FirstNodeOfTransfer             = 1;
    Node_p->FEINode.Pad1                            = 0;
    Node_p->FEINode.NodeCompleteNotify              = NodeCompleteNotify;
    Node_p->FEINode.NodeCompletePause               = NodeCompletePause;
    Node_p->FEINode.NumberBytes                     = 192*NumPackets;
    Node_p->FEINode.DestinationAddress_p            = Dest_p;
    Node_p->FEINode.SourceAddress_p                 = Src_p;
    Node_p->FEINode.TVOCounterAddress_p             = (U32 *) FEI_TVO_COUNTER_ADDRESS;

    memset ((char*)Dest_p, 0x55, 192*(NumPackets+1));
#ifdef DISPLAY_NODE
    VERBOSE_PRINT_DATA(" Node                   : 0x%x\n", Node_p);
    VERBOSE_PRINT_DATA(" Next_p                 : 0x%x\n", Node_p->FEINode.Next_p);
    VERBOSE_PRINT_DATA(" Extended               : 0x%x\n", Node_p->FEINode.Extended);
    VERBOSE_PRINT_DATA(" Type                   : 0x%x\n", Node_p->FEINode.Type);
    VERBOSE_PRINT_DATA(" DreqStart              : 0x%x\n", Node_p->FEINode.DreqStart);
    VERBOSE_PRINT_DATA(" DreqData               : 0x%x\n", Node_p->FEINode.DreqData);
    VERBOSE_PRINT_DATA(" FirstNodeOfTransfer    : 0x%x\n", Node_p->FEINode.FirstNodeOfTransfer);
    VERBOSE_PRINT_DATA(" Pad1                   : 0x%x\n", Node_p->FEINode.Pad1);
    VERBOSE_PRINT_DATA(" NodeCompleteNotify     : 0x%x\n", Node_p->FEINode.NodeCompleteNotify);
    VERBOSE_PRINT_DATA(" NodeCompletePause      : 0x%x\n", Node_p->FEINode.NodeCompletePause);
    VERBOSE_PRINT_DATA(" NumberBytes            : 0x%x\n", Node_p->FEINode.NumberBytes);
    VERBOSE_PRINT_DATA(" DestinationAddress_p   : 0x%x\n", Node_p->FEINode.DestinationAddress_p);
    VERBOSE_PRINT_DATA(" SourceAddress_p        : 0x%x\n", Node_p->FEINode.SourceAddress_p);
    VERBOSE_PRINT_DATA(" TVOCounterAddress_p    : 0x%x\n", Node_p->FEINode.TVOCounterAddress_p);
#endif

    /* return node */
    *FirstNode_p = Node_p;

    return ST_NO_ERROR;
}
#endif

/* Functions -------------------------------------------------------------- */
/****************************************************************************
Name         : fdmatst_CreatePESNode
Description  : Creates a node and associated buffers according the params
               given. Appends to the start of the Node list.
Parameters   : FirstNode_p : Ptr to direct to created node
               SrcBuff_p   : Source data
               Context     : Transfer Context.
               Size        : Size (in bytes of source data);

Return Value : 0 : ok.
               ST_NO_MEMORY : no space
****************************************************************************/
ST_ErrorCode_t fdmatst_CreatePESNode(U8 *SrcBuff_p, U32 Size,
                                     STFDMA_ContextId_t Context,
                                     STFDMA_GenericNode_t **FirstNode_p)
{
    STFDMA_GenericNode_t *Node_p;
    U32                  *Src_p;

    /* create memory for node */
    VERBOSE_PRINT("Creating memory for node\n");

    STOS_SemaphoreWait(pActiveNodeTableSemap);
    if (ActiveNodeCount < MAX_NUM_NODES)
    {
        Node_p = fdmatst_AllocNode();

        if (Node_p == NULL)
        {
            return ST_ERROR_NO_MEMORY;
        }
    }
    else
    {
        VERBOSE_PRINT("*** ERROR: Node creation: Not enough memory in deallocation pointer table\n");
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

    /* 128 byte aligned memory allocation */

    /* Create memory and store dellocate pointer */
    ActiveBuffers[ActiveNodeCount].SrcBuffSize = Size + 127;

    fdmatst_AllocSrcBuffer();

    if (ActiveBuffers[ActiveNodeCount].DeallocateSrc_p == NULL)
    {
        VERBOSE_PRINT("*** ERROR: Source mem creation: Not enough memory\n");
        STOS_MemoryDeallocate(fdmatst_GetNCachePartition(), ActiveNodes[ActiveNodeCount].DeallocateNode_p);
        fdmatst_DeallocNode();
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
    cache_purge_data(ActiveBuffers[ActiveNodeCount].DeallocateSrc_p, (Size + 127));
#endif

    ActiveBuffers[ActiveNodeCount].DeallocateDest_p = NULL;

    /* Adjust pointer to point to a 32byte aligned address */
    Src_p = (U32 *) (((U32)ActiveBuffers[ActiveNodeCount].DeallocateSrc_p + 127) & ~127);

#if defined (ARCHITECTURE_ST40)
#if defined (ST_OS21)
    Src_p = (U32*)ADDRESS_TO_P2(Src_p);
#else
    ActiveBuffers[ActiveNodeCount].Src_p = Src_p;
#endif
#endif

    memcpy(Src_p, SrcBuff_p, Size&~127);

    /* Update private count of active nodes */
    ActiveNodeCount++;

    /* Node table update complete. ok to release semap */
    STOS_SemaphoreSignal(pActiveNodeTableSemap);

    /* Generic node config */
    Node_p->ContextNode.Next_p   = (STFDMA_ContextNode_t*)*FirstNode_p;
    Node_p->ContextNode.Extended = STFDMA_EXTENDED_NODE;
    Node_p->ContextNode.Type     = STFDMA_EXT_NODE_PES;
    Node_p->ContextNode.Context  = Context;
#if defined (ST_7109)
    Node_p->ContextNode.Pad1     = 0;
    Node_p->ContextNode.Secure   = 0;
#else
    Node_p->ContextNode.Pad1     = 0;
#endif
    Node_p->ContextNode.Tag      = 0;
    Node_p->ContextNode.Pad2     = 0;
    Node_p->ContextNode.NodeCompletePause  = FALSE;
    Node_p->ContextNode.NodeCompleteNotify = FALSE;
    Node_p->ContextNode.SourceAddress_p    = Src_p;
    Node_p->ContextNode.NumberBytes        = Size&~127;

    VERBOSE_PRINT_DATA(" Node: 0x%x\n",(U32)Node_p);
#ifdef DISPLAY_NODE
    VERBOSE_PRINT_DATA(" Next_p                     : 0x%02x\n",(U32)Node_p->Gen.Next_p);
    VERBOSE_PRINT_DATA(" Extended                   : 0x%02x\n",Node_p->Gen.Extended);
    VERBOSE_PRINT_DATA(" Type                       : 0x%02x\n",Node_p->Gen.Type);
    VERBOSE_PRINT_DATA(" NumberBytes                : %d\n",    Node_p->Gen.NumberBytes);
    VERBOSE_PRINT_DATA(" SrcAddress_p               : 0x%08x\n",(U32)Node_p->Gen.SourceAddress_p);
    VERBOSE_PRINT_DATA(" Context                    : 0x%02x\n",Node_p->ContextNode.Context );
    VERBOSE_PRINT_DATA(" Tag                        : 0x%02x\n",Node_p->ContextNode.Tag);
    VERBOSE_PRINT_DATA(" NodeCompletePause          : 0x%02x\n",Node_p->ContextNode.NodeCompletePause);
    VERBOSE_PRINT_DATA(" NodeCompleteNotify         : 0x%02x\n",Node_p->ContextNode.NodeCompleteNotify);
#endif

    /* return node */
    *FirstNode_p = Node_p;

    return ST_NO_ERROR;
}

#if !defined (STFDMA_NO_PACED_TESTS)
/* Functions -------------------------------------------------------------- */
/****************************************************************************
Name         : fdmatst_CreateSPDINode
Description  : Creates a node and associated buffers according the params
               given. Appends to the start of the Node list.
Parameters   : FirstNode_p : Ptr to direct to created node
               SrcBuff_p   : Source data
               SrcSize     : Size (in bytes of source data);

Return Value : 0 : ok.
               ST_NO_MEMORY : no space
****************************************************************************/
ST_ErrorCode_t fdmatst_CreateSPDIFNode(U8  *SrcBuff_p,  U32 SrcSize,
                                       U8 **DestBuff_p, BOOL End, BOOL Valid,
                                       STFDMA_GenericNode_t **FirstNode_p)

{
    STFDMA_GenericNode_t *Node_p;
    U8                   *Src_p;

    /* create memory for node */
    VERBOSE_PRINT("Creating memory for node\n");

    STOS_SemaphoreWait(pActiveNodeTableSemap);
    if (ActiveNodeCount < MAX_NUM_NODES)
    {
        Node_p = fdmatst_AllocNode();

        if (Node_p == NULL)
        {
            return ST_ERROR_NO_MEMORY;
        }
    }
    else
    {
        VERBOSE_PRINT("*** ERROR: Node creation: Not enough memory in deallocation pointer table\n");
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

    /* 128 byte aligned memory allocation */

    ActiveBuffers[ActiveNodeCount].SrcBuffSize = SrcSize + 16;

    /* Create memory and store dellocate pointer */
    fdmatst_AllocSrcBuffer();

    if (ActiveBuffers[ActiveNodeCount].DeallocateSrc_p == NULL)
    {
        VERBOSE_PRINT("*** ERROR: Source mem creation: Not enough memory\n");
#if defined (ST_OSLINUX)
        dma_pool_free(g_NodePool,
            ActiveNodes[ActiveNodeCount].DeallocateNode_p,
            ActiveNodes[ActiveNodeCount].physical_address);
#else
        STOS_MemoryDeallocate(fdmatst_GetNCachePartition(), ActiveNodes[ActiveNodeCount].DeallocateNode_p);
#endif
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

    ActiveBuffers[ActiveNodeCount].DestBuffSize = (SPDIF_BURST_SIZE * 2 * sizeof(U32)) + (4 * 2 * sizeof(U32));
    fdmatst_AllocDestBuffer();

    if (ActiveBuffers[ActiveNodeCount].DeallocateDest_p == NULL)
    {
        VERBOSE_PRINT("*** ERROR: Source mem creation: Not enough memory\n");
        fdmatst_DeallocNode();
#if defined (ST_OSLINUX)
        dma_free_coherent(NULL,
            ActiveBuffers[ActiveNodeCount].SrcBuffSize,
            ActiveBuffers[ActiveNodeCount].DeallocateSrc_p,
            ActiveBuffers[ActiveNodeCount].src_physical_address);
#else
        STOS_MemoryDeallocate(fdmatst_GetNCachePartition(), ActiveBuffers[ActiveNodeCount].DeallocateSrc_p);
#endif
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

    Src_p = ((U8*)ActiveBuffers[ActiveNodeCount].DeallocateSrc_p)+1;
#if defined (ARCHITECTURE_ST40) &&  defined (ST_OS21)
    cache_purge_data(ActiveBuffers[ActiveNodeCount].DeallocateSrc_p, ActiveBuffers[ActiveNodeCount].SrcBuffSize);
    cache_purge_data(ActiveBuffers[ActiveNodeCount].DeallocateDest_p, ActiveBuffers[ActiveNodeCount].DestBuffSize);
    Src_p = (U8*)ADDRESS_TO_P2(Src_p);
#endif

    /* Set up the buffers and return the destination buffer */
    memcpy(Src_p, SrcBuff_p, SrcSize);

    *DestBuff_p = (U8*)ActiveBuffers[ActiveNodeCount].DeallocateDest_p;
#if defined (ARCHITECTURE_ST40) &&  defined (ST_OS21)
    *DestBuff_p = (U8*)ADDRESS_TO_P2(*DestBuff_p);
#endif
    memset((U8*)*DestBuff_p, 0, ActiveBuffers[ActiveNodeCount].DestBuffSize);

    /* Update private count of active nodes */
    ActiveNodeCount++;

    /* Node table update complete. ok to release semap */
    STOS_SemaphoreSignal(pActiveNodeTableSemap);

    /* Generic node config */
    Node_p->SPDIFNode.Next_p   = (STFDMA_SPDIFNode_t*)*FirstNode_p;
    Node_p->SPDIFNode.Extended = STFDMA_EXTENDED_NODE;
    Node_p->SPDIFNode.Type     = STFDMA_EXT_NODE_SPDIF;
    Node_p->SPDIFNode.DReq     = FDMA_TEST_ACTIVE_DREQ;  /* Use pace signal driven from software */
    Node_p->SPDIFNode.Pad      = 0;
    Node_p->SPDIFNode.BurstEnd = (End)?(1):(0);
    Node_p->SPDIFNode.Valid    = (Valid)?(1):(0);
    Node_p->SPDIFNode.NodeCompletePause    = FALSE;
    Node_p->SPDIFNode.NodeCompleteNotify   = FALSE;
    Node_p->SPDIFNode.NumberBytes          = SrcSize;
    Node_p->SPDIFNode.SourceAddress_p      = Src_p;
    Node_p->SPDIFNode.DestinationAddress_p = *DestBuff_p;

#if defined (ST_7100) || defined (ST_7109)
    Node_p->SPDIFNode.Data.CompressedMode.PreambleA   = 0XA123;
    Node_p->SPDIFNode.Data.CompressedMode.PreambleB   = 0XB123;
    Node_p->SPDIFNode.Data.CompressedMode.PreambleC   = 0XC123;
    Node_p->SPDIFNode.Data.CompressedMode.PreambleD   = 0XD123;
    Node_p->SPDIFNode.Data.CompressedMode.BurstPeriod = SPDIF_BURST_SIZE;
#else
    Node_p->SPDIFNode.PreambleA   = 0XA123;
    Node_p->SPDIFNode.PreambleB   = 0XB123;
    Node_p->SPDIFNode.PreambleC   = 0XC123;
    Node_p->SPDIFNode.PreambleD   = 0XD123;
    Node_p->SPDIFNode.BurstPeriod = SPDIF_BURST_SIZE;
#endif

    Node_p->SPDIFNode.Channel0.Status_0   = 0X01234567;
#if defined (ST_7100) || defined (ST_7109)
    Node_p->SPDIFNode.Channel0.Status.CompressedMode.Status_1   = 0X8;
    Node_p->SPDIFNode.Channel0.Status.CompressedMode.UserStatus = 0X1;
    Node_p->SPDIFNode.Channel0.Status.CompressedMode.Valid      = 0X1;
    Node_p->SPDIFNode.Channel0.Status.CompressedMode.Pad        = 0;
#else
    Node_p->SPDIFNode.Channel0.Status_1   = 0X8;
    Node_p->SPDIFNode.Channel0.UserStatus = 0X1;
    Node_p->SPDIFNode.Channel0.Valid      = 0X1;
    Node_p->SPDIFNode.Channel0.Pad        = 0;
#endif

    Node_p->SPDIFNode.Channel1.Status_0   = 0X9ABCDEF0;
#if defined (ST_7100) || defined (ST_7109)
    Node_p->SPDIFNode.Channel1.Status.CompressedMode.Status_1   = 0X1;
    Node_p->SPDIFNode.Channel1.Status.CompressedMode.UserStatus = 0X0;
    Node_p->SPDIFNode.Channel1.Status.CompressedMode.Valid      = 0;
    Node_p->SPDIFNode.Channel1.Status.CompressedMode.Pad        = 0;
#else
    Node_p->SPDIFNode.Channel1.Status_1   = 0X1;
    Node_p->SPDIFNode.Channel1.UserStatus = 0X0;
    Node_p->SPDIFNode.Channel1.Valid      = 0;
    Node_p->SPDIFNode.Channel1.Pad        = 0;
#endif

    VERBOSE_PRINT_DATA(" Node: 0x%x\n",Node_p);
#ifdef DISPLAY_NODE
    VERBOSE_PRINT_DATA(" Next_p                     : 0x%08x\n",Node_p->SPDIFNode.Next_p);
    VERBOSE_PRINT_DATA(" Extended                   : 0x%02x\n",Node_p->SPDIFNode.Extended);
    VERBOSE_PRINT_DATA(" Type                       : 0x%02x\n",Node_p->SPDIFNode.Type);
    VERBOSE_PRINT_DATA(" DReq                       : %d\n",    Node_p->SPDIFNode.DReq);
    VERBOSE_PRINT_DATA(" Valid Node                 : %d\n",    Node_p->SPDIFNode.Valid);
    VERBOSE_PRINT_DATA(" Burst End                  : %d\n",    Node_p->SPDIFNode.BurstEnd);
    VERBOSE_PRINT_DATA(" NumberBytes                : %d\n",    Node_p->SPDIFNode.NumberBytes);
    VERBOSE_PRINT_DATA(" SrcAddress_p               : 0x%08x\n",Node_p->SPDIFNode.SourceAddress_p);
    VERBOSE_PRINT_DATA(" DestAddress_p              : 0x%08x\n",Node_p->SPDIFNode.DestinationAddress_p);
    VERBOSE_PRINT_DATA(" NodeCompletePause          : 0x%02x\n",Node_p->SPDIFNode.NodeCompletePause);
    VERBOSE_PRINT_DATA(" NodeCompleteNotify         : 0x%02x\n",Node_p->SPDIFNode.NodeCompleteNotify);
#endif

    /* return node */
    *FirstNode_p = Node_p;

    return ST_NO_ERROR;
}
#endif

/* Functions -------------------------------------------------------------- */
/****************************************************************************
Name         : fdmatst_CreateESBuffer
Description  : Creates a Buffer in non-cached memory.
Parameters   : Buffer_p : Ptr to buffer
               Size     : Size (in bytes of source data);

Return Value : 0 : ok.
               ST_NO_MEMORY : no space
****************************************************************************/
ST_ErrorCode_t fdmatst_CreateESBuffer(U32 Size,
                                      U8  **Buffer_p)
{
    /* create memory for node */
    VERBOSE_PRINT("Creating memory for buffer\n");

    /* 32 byte aligned memory allocation */

    /* Create memory and store dellocate pointer */
    ActiveBuffers[ActiveNodeCount].DestBuffSize = Size + 128;
#if defined (ST_OSLINUX)
    ActiveBuffers[ActiveNodeCount].DeallocateDest_p = (U32 *)dma_alloc_coherent(
        NULL,
        ActiveBuffers[ActiveNodeCount].DestBuffSize,
        &(ActiveBuffers[ActiveNodeCount].dest_physical_address),
        GFP_KERNEL);

     VERBOSE_PRINT_DATA(" DeallocateDest_p      : 0x%08x\n", (U32)ActiveBuffers[ActiveNodeCount].DeallocateDest_p);
     VERBOSE_PRINT_DATA(" dest_physical_address : 0x%08x\n", ActiveBuffers[ActiveNodeCount].dest_physical_address);
     VERBOSE_PRINT_DATA(" DestBuffSize          : %d\n", ActiveBuffers[ActiveNodeCount].DestBuffSize);

     memset(ActiveBuffers[ActiveNodeCount].DeallocateDest_p, 0 , ActiveBuffers[ActiveNodeCount].DestBuffSize);
#else
    ActiveBuffers[ActiveNodeCount].DeallocateDest_p = (U32 *) STOS_MemoryAllocateClear(
        fdmatst_GetNCachePartition(),
        1,
        ActiveBuffers[ActiveNodeCount].DestBuffSize);
#endif

    if (ActiveBuffers[ActiveNodeCount].DeallocateDest_p == NULL)
    {
        VERBOSE_PRINT("*** ERROR: Dest mem creation: Not enough memory\n");
        STOS_SemaphoreSignal(pActiveNodeTableSemap);
        return ST_ERROR_NO_MEMORY;
    }

#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
    cache_purge_data(ActiveBuffers[ActiveNodeCount].DeallocateDest_p, (Size + 128));
#endif

    ActiveNodes  [ActiveNodeCount].DeallocateNode_p = NULL;
    ActiveBuffers[ActiveNodeCount].DeallocateSrc_p  = NULL;

    /* Adjust pointer to point to a 128byte aligned address */
    *Buffer_p = (U8 *) (((U32)ActiveBuffers[ActiveNodeCount].DeallocateDest_p + 128) & ~127);

#if defined (ARCHITECTURE_ST40)
#if defined (ST_OS21)
    *Buffer_p = (U8 *)ADDRESS_TO_P2(*Buffer_p);
#else
    ActiveBuffers[ActiveNodeCount].Dest_p = (U32*)*Buffer_p;
#endif
#endif

    /* Update private count of active nodes */
    ActiveNodeCount++;

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : fmdatst_SetupCircularTransfer
Description  : Sets up a two node circular transfer where each node
               is 1D to 1D for non paced, or 0D to 1D to paced.
Parameters   : NodeCompleteNotify : TRUE for notify
               NodeCompletePause : TRUE for pause
               SrcAligned : TRUE for aligned structure
               DestAligned : TRUE for aligned structure
               Paced : TRUE for a paced transfer
               FirstNext_p : Return Ptr to Ptr for node storage.
Return Value : Ptr to first node in list (ping)
               Error code : 0 ok
****************************************************************************/
ST_ErrorCode_t fdmatst_SetupCircularTransfer(BOOL NodeCompleteNotify,
                                             BOOL NodeCompletePause,
                                             BOOL SrcAligned,
                                             BOOL DestAligned,
                                             BOOL Paced,
                                             STFDMA_Node_t **FirstNode_p)
{
    STFDMA_Node_t       *PingNode_p;
    STFDMA_Node_t       *PongNode_p;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    if (Paced == FALSE)
    {
        /* Setup non-paced node transfer for 1D only */

        ErrorCode = fdmatst_CreateNode(FDMATST_DIM_1D, FDMATST_Y_INC, SrcAligned,
                                       FDMATST_DIM_1D, FDMATST_Y_INC, DestAligned,
                                       NodeCompleteNotify, NodeCompletePause,
                                       STFDMA_REQUEST_SIGNAL_NONE,
                                       &PingNode_p);

        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        if (ErrorCode != ST_NO_ERROR)
        {
            return ErrorCode;
        }

        ErrorCode = fdmatst_CreateNode(FDMATST_DIM_1D, FDMATST_Y_INC, SrcAligned,
                                       FDMATST_DIM_1D, FDMATST_Y_INC, DestAligned,
                                       NodeCompleteNotify, NodeCompletePause,
                                       STFDMA_REQUEST_SIGNAL_NONE,
                                       &PongNode_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
    }
    else
    {
        /* Paced usage restricted 0D to 1D using PIO 2 bit 5 usage only */

        ErrorCode = fdmatst_CreateNode(FDMATST_DIM_0D, FDMATST_Y_STATIC, SrcAligned,
                                       FDMATST_DIM_1D, FDMATST_Y_INC, DestAligned,
                                       NodeCompleteNotify, NodeCompletePause,
                                       FDMA_TEST_ACTIVE_DREQ,
                                       &PingNode_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        if (ErrorCode != ST_NO_ERROR)
        {
            return ErrorCode;
        }

        ErrorCode = fdmatst_CreateNode(FDMATST_DIM_0D, FDMATST_Y_STATIC, SrcAligned,
                                       FDMATST_DIM_1D, FDMATST_Y_INC, DestAligned,
                                       NodeCompleteNotify, NodeCompletePause,
                                       FDMA_TEST_ACTIVE_DREQ,
                                       &PongNode_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
    }


    if (ErrorCode != ST_NO_ERROR)
    {
        /* Pong node creation failed, dealloc ping */
        fdmatst_DeallocateNodes();
    }
    else
    {
        /* make self circular reference */
        PingNode_p->Next_p = PongNode_p;
        PongNode_p->Next_p = PingNode_p;
        VERBOSE_PRINT_DATA(" Ping node   : 0x%x\n",(U32)PingNode_p);
        VERBOSE_PRINT_DATA(" Ping Next_p : 0x%x\n",(U32)PingNode_p->Next_p);
        VERBOSE_PRINT_DATA(" Pong node   : 0x%x\n",(U32)PongNode_p);
        VERBOSE_PRINT_DATA(" Pong Next_p : 0x%x\n",(U32)PongNode_p->Next_p);

        /* return first node */
        *FirstNode_p = PingNode_p;
    }

    return ErrorCode;
}

#if !defined (STFDMA_NO_PACED_TESTS)
/****************************************************************************
Name         : PaceRequestGeneratorTask
Description  : Task for generating pace signal requests.
               Used to support pace signal testing without using additional
               components.
               Repeatedly toggles a port which is connected to an input port
               configured as FDMA external request signal.
               NOTE: REQUIRES A WIRE JUMPER CONNECTING PIO2 BIT4 TO PIO2 BIT5.
                Also, Many task config parameters not yet used. Could be used
                for data transfer check in 0D to 0D transfers.
Parameters   : Configuration parameters, see struc def
Return Value : None.
****************************************************************************/
#if defined (ST_OSLINUX)
static int   PaceRequestGeneratorTask(void * TaskConfig)
#else
static void PaceRequestGeneratorTask(void * TaskConfig)
#endif
{
    STOS_TaskEnter(TaskConfig);

    /* Notify task alive */
    STOS_SemaphoreSignal(pRequestGeneratorAlive);

    /* Toggle port at required pace until all bytecount reached */
    while(1)
    {
        /* Task control....*/
        if (RequestGeneratorStop == TRUE)
        {
            VERBOSE_PRINT("REQGEN: Waiting\n");
            RequestGeneratorStop = FALSE;
            STOS_SemaphoreWait(pWakeupRequestGenerator);

            VERBOSE_PRINT("REQGEN: Wakeup\n");
            if (RequestGeneratorSuicide == TRUE)
            {
                /* Exit command recieved */
                VERBOSE_PRINT("REQGEN: Suicide\n");
                RequestGeneratorSuicide = FALSE;
                break;
            }
        }

        /* Set output bit, so raise request signal */
#if defined (ST_5105) || defined (ST_5107)
        SET_REG(0x45600000, 0x01010101);
#else
        STPIO_Set(g_STPIOHandle, PIO_REQ_SEND);
#endif
        STOS_TaskDelay(ST_GetClocksPerSecond() / 100);

        /* Drop request line */
#if defined (ST_5105) || defined (ST_5107)
        SET_REG(0x45600000, 0x10101010);
#else
        STPIO_Clear(g_STPIOHandle, PIO_REQ_SEND);
#endif
        STOS_TaskDelay(ST_GetClocksPerSecond() / 100);
    }

    VERBOSE_PRINT("REQGEN: Exit\n");

    STOS_TaskExit(TaskConfig);
#if defined (ST_OSLINUX)
    return ST_NO_ERROR;
#endif
}
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
/****************************************************************************
Name         : fdmatst_InitPaceRequestGenerator
Description  : Creates request generator task.
Parameters   : *Node_p: Node pointer
               BytesPerSec: Number of bytes per sec to send.
                            MIN is assumed to be 4bytes persec.
                            MAX is 15000bytes * 4 persec
Return Value : 0: All ok.
             : !0 : error
****************************************************************************/
ST_ErrorCode_t fdmatst_InitPaceRequestGenerator(STFDMA_Node_t *Node_p, U32 BytesPerSec)
{
    PaceRequestGen_t    TaskConfig;
    STPIO_InitParams_t  STPIOInitParams;
    STPIO_OpenParams_t  STPIOOpenParams;
    ST_ErrorCode_t      ErrorCode;
    BOOL                SetupFailed     = FALSE;

    VERBOSE_PRINT("Allocating PaceRequestGenerator resources\n");

    pWakeupRequestGenerator = STOS_SemaphoreCreateFifo(system_partition, 0);
    pRequestGeneratorAlive = STOS_SemaphoreCreateFifo(system_partition, 0);

    RequestGeneratorSuicide = FALSE;
    RequestGeneratorStop = TRUE;

    /* Load task config parameters */
    TaskConfig.Src_p = Node_p->SourceAddress_p;
    TaskConfig.Dest_p = Node_p->DestinationAddress_p;
    TaskConfig.Next_p = (U32 *)Node_p->Next_p;
    TaskConfig.NumberBytes = Node_p->NumberBytes;

    /* Sending 4bytes at a time and using STOS_TaskDelay for timing, so need to ensure limits are not exceeded */
    if (BytesPerSec <  DEFAULT_REQ_WORSIZE)
    {
        TaskConfig.BytesPerSec = DEFAULT_REQ_WORSIZE;
    }

    if (BytesPerSec > (fdmatst_GetClocksPerSec() * DEFAULT_REQ_WORSIZE))
    {
        TaskConfig.BytesPerSec = fdmatst_GetClocksPerSec();
    }
    else
    {
        TaskConfig.BytesPerSec = BytesPerSec;
    }

    /* Configure 0D controls for source */
    if (Node_p->NodeControl.SourceDirection == STFDMA_DIRECTION_STATIC)
    {
        TaskConfig.Src0D = TRUE;
    }
    else
    {
        TaskConfig.Src0D = FALSE;
    }

    /* Configure 0D controls for destination */
    if (Node_p->NodeControl.DestinationDirection == STFDMA_DIRECTION_STATIC)
    {
        TaskConfig.Dest0D = TRUE;
    }
    else
    {
        TaskConfig.Dest0D = FALSE;
    }

    /* Init and open PIO connections */
    VERBOSE_PRINT("REQGEN: Setup pio\n");
    STPIOInitParams.BaseAddress = (U32*) PIO_2_BASE_ADDRESS;
    STPIOInitParams.InterruptNumber = PIO_2_INTERRUPT;
    STPIOInitParams.InterruptLevel = PIO_2_INTERRUPT_LEVEL;
    STPIOInitParams.DriverPartition = fdmatst_GetSystemPartition();
    ErrorCode = STPIO_Init("PIO2", &STPIOInitParams);

    if (ErrorCode != ST_NO_ERROR)
    {
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        SetupFailed = TRUE;
    }

    /*
    Configure one bit for output to wiggle when desired (bit4), and one bit for alternate
    function (bit5) which in external request signal to FDMA.
    */
    STPIOOpenParams.ReservedBits = (PIO_REQ_SEND |
#if defined (ST_5517)
                                    PIO_REQ_RECEIVE_2);
#else
                                    PIO_REQ_RECEIVE_1);
#endif
    /* Output port under task control */
    STPIOOpenParams.BitConfigure[SEND_BIT_IDX] = STPIO_BIT_OUTPUT;

    /* Request signal input. No Input setting, use bidirectional */
    STPIOOpenParams.BitConfigure[RECEIVE_BIT_IDX] = STPIO_BIT_INPUT;

    STPIOOpenParams.IntHandler = NULL;
    ErrorCode = STPIO_Open("PIO2", &STPIOOpenParams, &g_STPIOHandle);

    if (ErrorCode != ST_NO_ERROR)
    {
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        SetupFailed = TRUE;
    }

    /* Ensure request lines are low to start with */
    ErrorCode = STPIO_Clear(g_STPIOHandle, PIO_REQ_SEND);
    ErrorCode = STPIO_Clear(g_STPIOHandle, PIO_REQ_RECEIVE_1);
    ErrorCode = STPIO_Clear(g_STPIOHandle, PIO_REQ_RECEIVE_2);

    if (ErrorCode != ST_NO_ERROR)
    {
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        SetupFailed = TRUE;
    }

    /* Only perform core task if setup went ok...*/
    if (SetupFailed == FALSE)
    {
        /* initialise task */
        STOS_TaskCreate((void(*)(void *))PaceRequestGeneratorTask,
                        (void *)&TaskConfig,
                        system_partition,
                        (size_t)TASK_STACK_SIZE,
                        (void **)&PaceReqTaskStack_p,
                        system_partition,
                        &pPaceReqTaskId,
                        PaceReqTaskDesc,
                        MAX_USER_PRIORITY,
                        "fdmatst_PaceReqGenTask",
                        (task_flags_t)0);

        VERBOSE_PRINT("Wait for task to initialise\n");
        STOS_SemaphoreWait(pRequestGeneratorAlive);
    }
    return (ST_NO_ERROR);
}

#endif


#if !defined (STFDMA_NO_PACED_TESTS)
/****************************************************************************
Name         : fdmatst_StartPaceRequestGenerator
Description  : Start the request generator task running.
Parameters   :
Return Value :
****************************************************************************/
U32 fdmatst_StartPaceRequestGenerator(void)
{
    /* Wakeup task by semap signal */
    VERBOSE_PRINT("Wakeup PaceReqGen task\n");
    STOS_SemaphoreSignal(pWakeupRequestGenerator);

    /* Let the generator settle */
    STOS_TaskDelay(fdmatst_GetClocksPerSec()/10);
    return 0;
}
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
/****************************************************************************
Name         : fdmatst_StopPaceRequestGenerator
Description  : Stops the request generator task running.
Parameters   :
Return Value :
****************************************************************************/
U32 fdmatst_StopPaceRequestGenerator(void)
{
    /* Wakeup task by semap signal */
    VERBOSE_PRINT("Stop PaceReqGen task\n");
    RequestGeneratorStop = TRUE;
    return 0;
}
#endif

#if !defined (STFDMA_NO_PACED_TESTS)
/****************************************************************************
Name         : fdmatst_TermPaceRequestGenerator
Description  : Wakesup the task and sets the suicide flag.
Parameters   : None
Return Value : 0: Term ok.
****************************************************************************/
U32 fdmatst_TermPaceRequestGenerator(void)
{
    STPIO_TermParams_t  STPIOTermParams;

    VERBOSE_PRINT("Terminating PaceReqGen task\n");
    /* Stop task, Set suicide flag and wake up task */
    RequestGeneratorStop = TRUE;
    RequestGeneratorSuicide = TRUE;
    STOS_SemaphoreSignal(pWakeupRequestGenerator);

    STOS_TaskWait(&pPaceReqTaskId, (STOS_Clock_t *)TIMEOUT_INFINITY);
    STOS_TaskDelete(pPaceReqTaskId, system_partition, (void *)PaceReqTaskStack_p, system_partition);

    /* close and term pio connection */
    VERBOSE_PRINT("REQGEN: Terming PIO\n");
    STPIO_Close(g_STPIOHandle);
    STPIOTermParams.ForceTerminate = FALSE;
    STPIO_Term("PIO2", &STPIOTermParams);

    /* Kill task, del task resources */

    VERBOSE_PRINT("Deleting task resources\n");

    STOS_SemaphoreDelete(system_partition, pWakeupRequestGenerator);
    STOS_SemaphoreDelete(system_partition, pRequestGeneratorAlive);

    return 0;
}
#endif

/****************************************************************************
Name         : fmdatst_DeallocateNodes
Description  : Deallocate all currently active nodes and src/dest buffers
Parameters   :
Return Value : U32 : Error indicator
****************************************************************************/
U32 fdmatst_DeallocateNodes(void)
{
    U32 i=0;

   /* Traverse list until no nodes left */
   STOS_SemaphoreWait(pActiveNodeTableSemap);
   for (i=0; i<ActiveNodeCount; i++)
   {
       VERBOSE_PRINT_DATA("Deallocate node %d \n",i);

       /* Remove buffers */
       if (NULL != ActiveBuffers[i].DeallocateSrc_p)
       {
#if defined (ST_OSLINUX)
           dma_free_coherent(NULL,
           ActiveBuffers[i].SrcBuffSize,
           ActiveBuffers[i].DeallocateSrc_p,
           ActiveBuffers[i].src_physical_address);
           ActiveBuffers[i].DeallocateSrc_p = NULL;
           ActiveBuffers[i].src_physical_address = 0;
           ActiveBuffers[i].Src_p = NULL;
#else
           STOS_MemoryDeallocate(fdmatst_GetNCachePartition(), ActiveBuffers[i].DeallocateSrc_p);
#endif
       }

       if (NULL != ActiveBuffers[i].DeallocateDest_p)
       {
#if defined (ST_OSLINUX)
           dma_free_coherent(NULL,
           ActiveBuffers[i].DestBuffSize,
           ActiveBuffers[i].DeallocateDest_p,
           ActiveBuffers[i].dest_physical_address);
           ActiveBuffers[i].DeallocateDest_p = NULL;
           ActiveBuffers[i].dest_physical_address = 0;
           ActiveBuffers[i].Dest_p = NULL;
#else
           STOS_MemoryDeallocate(fdmatst_GetNCachePartition(), ActiveBuffers[i].DeallocateDest_p);
#endif
       }

       /* Remove current node using deallocate pointer from array */
       if (NULL != ActiveNodes[i].DeallocateNode_p)
       {
#if defined (ST_OSLINUX)
           dma_pool_free(g_NodePool,
           ActiveNodes[i].DeallocateNode_p,
           ActiveNodes[i].physical_address);
           ActiveNodes[i].DeallocateNode_p = NULL;
           ActiveNodes[i].physical_address = 0;
#else
           STOS_MemoryDeallocate(fdmatst_GetNCachePartition(),  ActiveNodes[i].DeallocateNode_p);
#endif
       }
   }
   ActiveNodeCount = 0;
   STOS_SemaphoreSignal(pActiveNodeTableSemap);

   return 0;
}

/****************************************************************************
Name         : DumpBuffers
Description  : Dumps src and dest buffers for visual inspection.
Parameters   : Scr_p: Src buffer pointer
               Dst_p: Destination buffer pointer
               Idx: Current index position
               DumpNum: Number of 32bit word to dump
Return Value : None
****************************************************************************/
void DumpBuffers(U32 *Src_p, U32 *Dst_p, U32 CurrentIdx, U32 BufferEndIdx, U32 DumpNum)
{
    U32 DumpOffset = 0;
    U32 DumpEnd = 0;

    STTBX_Print(("Data differs from offset %d: \n",CurrentIdx));

    /* Ensure stop point is at buffer end or DumpNum number of bytes away */
    if ((CurrentIdx + DumpNum) <= BufferEndIdx)
    {
        DumpEnd = (CurrentIdx + DumpNum);
    }
    else
    {
        DumpEnd = BufferEndIdx;
    }

    /* Dump portion of buffers for visual inspection */
    for (DumpOffset = 0; DumpOffset < DumpEnd; DumpOffset++)     /* junk */
    {
        STTBX_Print(("Src_p: 0x%08x == 0x%08x,   Dst_p: 0x%08x == 0x%08x\n",(U32)&Src_p[DumpOffset],Src_p[DumpOffset],
                     (U32)&Dst_p[DumpOffset], Dst_p[DumpOffset]));
    }
}

/****************************************************************************
Name         : fdmatst_CheckData
Description  : Traverses list of nodes comparing source data to destination
               data buffer to determine if transfer was successful.
               NOTE: This function has built in knowledge of each test case
               to check and the structure of the buffers used in those transfers.
Parameters   : Ptr to first node in list of nodes
               TestCaseId[4] - Test case Id string
Return Value : 0   - No differences found
               > 0 - A bitfield representing the node whose source and destination
                     buffers differed.
****************************************************************************/
U32 fdmatst_CheckData(STFDMA_Node_t **FirstNode_p, char TestCaseId[5])
{
    STFDMA_Node_t *CurrentNode_p = NULL;
    U8  *SrcBuff_p;
    U8  *DstBuff_p;
    U32 NodeCount = 0;
    U32 ErrorLog = 0;
    BOOL DataWrong;
    U32 i=0;
    U32 j=0;
    U32 k=0;

#if defined (ARCHITECTURE_ST40)
#if defined (ST_OS21)
    CurrentNode_p = (STFDMA_Node_t*)ADDRESS_TO_P2(*FirstNode_p);
#else
    CurrentNode_p = *FirstNode_p;
#endif
    fdmatst_CovertNodeAddressDataToCPU(CurrentNode_p);
#else
    CurrentNode_p = *FirstNode_p;
#endif

    /* Perform data checks for as long as there are node to check..*/
    while (CurrentNode_p != NULL)
    {

        SrcBuff_p = CurrentNode_p->SourceAddress_p;
        DstBuff_p = CurrentNode_p->DestinationAddress_p;

        DataWrong = FALSE;

        /* Perform data comparisons.... */

        /* 1D to 1D tranfer....*/
        if (((strcmp(&TestCaseId[0], "F103")) == 0) ||
            ((strcmp(&TestCaseId[0], "F109")) == 0) ||
            ((strcmp(&TestCaseId[0], "F204")) == 0) ||
            ((strcmp(&TestCaseId[0], "Sk02")) == 0))
        {
            /* Compare buffers */
            for (i=0; i!=(CurrentNode_p->NumberBytes); i++)
            {
                /* Check contents of source 1D is in destination 1D */
                if (SrcBuff_p[i] != DstBuff_p[i])
                {
                    DataWrong = TRUE;
                    DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (CurrentNode_p->NumberBytes / 4), 100);
                    break;
                }
            }
        }    /* 2D to 2D src and dest dec y.... */
        else if ((strcmp(&TestCaseId[0], "F201")) == 0)
        {

            /* Since src and dest are decrementing y, the buffer pointers refer to the first byte
             * in the last line of the array.
             * Need to adjust them to point to the begining of the buffer (the opposite
             * operation is performed by CreateNode() function).
             * Need to calculate the end of the arrays, then move back to the beginning.
             */
            SrcBuff_p = ((SrcBuff_p + DEFAULT_STRIDE) - DEFAULT_2D_MEMSIZE);
            DstBuff_p = ((DstBuff_p + DEFAULT_STRIDE) - DEFAULT_2D_MEMSIZE);

            i=0;
            while (i < DEFAULT_2D_MEMSIZE)
            {
                /* Check the length */
                for (j = 0; (j < DEFAULT_2D_LINE_LENGTH); j++, i++)
                {
                    if (DstBuff_p[i] != SrcBuff_p[i])
                    {
                        DataWrong = TRUE;
                        DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (DEFAULT_2D_MEMSIZE / 4), 100);

                        /* Quit out */
                        j = DEFAULT_2D_LINE_LENGTH;
                        i = DEFAULT_2D_MEMSIZE;
                    }
                }

                /* Check the gap is untouched */
                if (i < DEFAULT_2D_MEMSIZE)
                {
                    for (j = 0; (j < (DEFAULT_STRIDE - DEFAULT_2D_LINE_LENGTH)); j++, i++)
                    {
                        if (DstBuff_p[i] != 0)
                        {
                            DataWrong = TRUE;
                            DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (DEFAULT_2D_MEMSIZE / 4), 100);

                            /* Quit out */
                            j = (DEFAULT_STRIDE - DEFAULT_2D_LINE_LENGTH);
                            i = DEFAULT_2D_MEMSIZE;
                        }
                    }
                }
            }
        }   /* 2DdecY to 2DincY transfer */
        else if ((strcmp(&TestCaseId[0], "F202")) == 0)
        {
            /* The src is decrementing y
             * The destination is incrementing y
             * We increment through the destination buffer
             * but decrement through the source buffer.
             */
            i = 0;
            k = 0;
            while (i < DEFAULT_2D_MEMSIZE)
            {
                /* Check the length */
                for (j = 0; (j < DEFAULT_2D_LINE_LENGTH); j++, i++)
                {
                    if (DstBuff_p[i] != SrcBuff_p[j+k])
                    {
                        STTBX_Print(("Error at %d %d\n", i, j+k));
                        DataWrong = TRUE;
                        DumpBuffers((U32*)SrcBuff_p-DEFAULT_2D_MEMSIZE+DEFAULT_2D_LINE_LENGTH, (U32*)DstBuff_p, i/4, (DEFAULT_2D_MEMSIZE / 4), 100);

                        /* Quit out */
                        j = DEFAULT_2D_LINE_LENGTH;
                        i = DEFAULT_2D_MEMSIZE;
                    }
                }

                /* Check the gap is untouched */
                if (i < DEFAULT_2D_MEMSIZE)
                {
                    k -= DEFAULT_STRIDE;
                    for (j = 0; (j < (DEFAULT_2D_UNUSED)); j++, i++)
                    {
                        if (DstBuff_p[i] != 0)
                        {
                            STTBX_Print(("Error at %d\n", i));
                            DataWrong = TRUE;
                            DumpBuffers((U32*)SrcBuff_p-DEFAULT_2D_MEMSIZE+DEFAULT_2D_LINE_LENGTH, (U32*)DstBuff_p, i/4, (DEFAULT_2D_MEMSIZE / 4), 100);

                            /* Quit out */
                            j = (DEFAULT_STRIDE - DEFAULT_2D_LINE_LENGTH);
                            i = DEFAULT_2D_MEMSIZE;
                        }
                    }
                }
            }

        }    /* 2D to 2D src and dest inc y.... */
        else if (((strcmp(&TestCaseId[0], "F209")) == 0) ||
                 ((strcmp(&TestCaseId[0], "F210")) == 0) ||
                 ((strcmp(&TestCaseId[0], "F203")) == 0) ||
                 ((strcmp(&TestCaseId[0], "Sk01")) == 0))
        {
            i=0;
            while (i < DEFAULT_2D_MEMSIZE)
            {
                /* Check the length */
                for (j = 0; (j < DEFAULT_2D_LINE_LENGTH); j++, i++)
                {
                    if (DstBuff_p[i] != SrcBuff_p[i])
                    {
                        DataWrong = TRUE;
                        DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (DEFAULT_2D_MEMSIZE / 4), 100);

                        /* Quit out */
                        j = DEFAULT_2D_LINE_LENGTH;
                        i = DEFAULT_2D_MEMSIZE;
                    }
                }

                /* Check the gap is untouched */
                if (i < DEFAULT_2D_MEMSIZE)
                {
                    for (j = 0; (j < (DEFAULT_STRIDE - DEFAULT_2D_LINE_LENGTH)); j++, i++)
                    {
                        if (DstBuff_p[i] != 0)
                        {
                            DataWrong = TRUE;
                            DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (DEFAULT_2D_MEMSIZE / 4), 100);

                            /* Quit out */
                            j = (DEFAULT_STRIDE - DEFAULT_2D_LINE_LENGTH);
                            i = DEFAULT_2D_MEMSIZE;
                        }
                    }
                }
            }

        }  /* 1D to 2Dincy transfer */
        else if ((strcmp(&TestCaseId[0], "F205")) == 0)
        {
            U32 Idx2D = 0; /* 2D index. Asssumes gap is 4bytes */

            /* Compare the buffers..
             * Main index is i for the 1D structure.
             * Seperate index is used for 2d buffer comparison since they will inc at different rates.
             */
            for (i=0; (i != CurrentNode_p->NumberBytes); i++)
            {
                /* Got a difference between source and destination buffers? */
                if (SrcBuff_p[i] != DstBuff_p[Idx2D])
                {
                    /* If we are not at a gap in the 2D structure, its value is not as expected, and the value
                     * of the location after the gap is not the same the current src, there is an error. */
                    if ((i != 0) && ((i % DEFAULT_2D_LINE_LENGTH) == 0) &&
                        ((DstBuff_p[Idx2D] != 0) ||
                         (SrcBuff_p[i] != DstBuff_p[Idx2D+4])))
                    {
                        DataWrong = TRUE;
                        DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (CurrentNode_p->NumberBytes / 4), 100);
                        break;
                    }
                }

                /* If we are at a gap in the 2d structure...*/
                if ((i != 0) && ((i % DEFAULT_2D_LINE_LENGTH) == 0))
                {
                    /* Advance 2d index an additional time to pass gap */
                    Idx2D += 4;
                }

                /* Advance position in 2d structure */
                Idx2D++;
            }

        }    /* 2D to 1D transfer */
        else if ((strcmp(&TestCaseId[0], "F206")) == 0)
        {
            /* Compare the buffers..
             * Main index is i for the 1D structure.
             * Seperate index (k) is used for 2d buffer comparison since they will inc at different rates.
             */
            i=0;
            k=0;
            while (i < CurrentNode_p->NumberBytes)
            {
                /* Check the length */
                for (j = 0; (j < DEFAULT_2D_LINE_LENGTH); j++, i++)
                {
                    if (DstBuff_p[i] != SrcBuff_p[k+i])
                    {
                        DataWrong = TRUE;
                        DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (DEFAULT_2D_MEMSIZE / 4), 100);

                        /* Quit out */
                        j = DEFAULT_2D_LINE_LENGTH;
                        i = DEFAULT_2D_MEMSIZE;
                    }
                }

                k += DEFAULT_STRIDE - DEFAULT_2D_LINE_LENGTH;
            }

        }  /* 0D to 1D transfer */
        else if ((strcmp(&TestCaseId[0], "F207")) == 0)
        {
            for (i=0; (i != CurrentNode_p->NumberBytes); i++)
            {
                /* Check source location(s) is duplicated though 1D destination buffer */
                if (SrcBuff_p[(i % DEFAULT_0D_WORDSIZE)] != DstBuff_p[i])
                {
                    DataWrong = TRUE;
                    DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (CurrentNode_p->NumberBytes / 4), 100);
                    break;
                }
            }

        } /* 0D to 2D transfer */
        else if ((strcmp(&TestCaseId[0], "F208")) == 0)
        {
            U32 Idx2D = 0;

            for (i=0; (i < CurrentNode_p->NumberBytes); i++)
            {
                /* Check the length */
                for (j = 0; (j < DEFAULT_0D_WORDSIZE); j++)
                {
                    if (DstBuff_p[Idx2D++] != SrcBuff_p[j])
                    {
                        DataWrong = TRUE;
                        DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (CurrentNode_p->NumberBytes / 4), 100);

                        /* Quit out */
                        j = DEFAULT_STRIDE - DEFAULT_0D_WORDSIZE;
                        i = CurrentNode_p->NumberBytes;
                    }
                }
                i += DEFAULT_0D_WORDSIZE;

                /* Check the gap is untouched */
                if (i < CurrentNode_p->NumberBytes)
                {
                    for (j = 0; (j < (DEFAULT_STRIDE - DEFAULT_0D_WORDSIZE)); j++)
                    {
                        if (DstBuff_p[Idx2D++] != 0)
                        {
                            DataWrong = TRUE;
                            DumpBuffers((U32*)SrcBuff_p, (U32*)DstBuff_p, i/4, (CurrentNode_p->NumberBytes / 4), 100);

                            /* Quit out */
                            j = DEFAULT_STRIDE - DEFAULT_0D_WORDSIZE;
                            i = CurrentNode_p->NumberBytes;
                        }
                    }
                }
            }
        }
        else  /* Combination not supported */
        {
            VERBOSE_PRINT("***ERROR: Src and Dest buffer dimension not supported in data check\n");
        }

        /* If comparison failed, log node number */
        if (DataWrong == TRUE)
        {
            ErrorLog |= (1 << NodeCount);
        }

        /* Move to next node in list */
        CurrentNode_p = CurrentNode_p->Next_p;

        NodeCount++;

        /* Check for circular list list */
        if (CurrentNode_p == *FirstNode_p)
        {
            CurrentNode_p = NULL;
        }
    }

    return ErrorLog;
}

#if !defined (ST_OSLINUX)
/****************************************************************************
Name         : fdmatst_DisplayPartitionStatus
Description  : Displays the contents of the memory partition status
Parameters   : Ptr to a memory partition status type
Return Value :
****************************************************************************/
void fdmatst_DisplayPartitionStatus(partition_status_t *Partition)
{

    VERBOSE_PRINT(" Status: ");
    switch (Partition->partition_status_state)
    {
    case partition_status_state_valid :
        VERBOSE_PRINT("  VALID\n");
        break;
    case partition_status_state_invalid :
        VERBOSE_PRINT("  INVALID\n");
        break;
    default :
        VERBOSE_PRINT("  UNKNOWN\n");
        break;
    }

    VERBOSE_PRINT(" Type: ");
    switch (Partition->partition_status_type)
    {
    case partition_status_type_simple :
        VERBOSE_PRINT("    Simple\n");
        break;
    case partition_status_type_fixed :
        VERBOSE_PRINT("    Fixed\n");
        break;
    case partition_status_type_heap :
        VERBOSE_PRINT("    Heap\n");
        break;
    default :
        VERBOSE_PRINT("    UNKNOWN\n");
        break;
    }

    VERBOSE_PRINT_DATA(" Size:     %d\n",Partition->partition_status_size);
    VERBOSE_PRINT_DATA(" Free:     %d\n",Partition->partition_status_free);
    VERBOSE_PRINT_DATA(" Largest:  %d\n",Partition->partition_status_free_largest);
    VERBOSE_PRINT_DATA(" Used:     %d\n",Partition->partition_status_used);
}
#endif

/****************************************************************************
Name            : fdmatst_ResetSuccessState
Description     : Load given value to success control
Parameters      :
****************************************************************************/
void fdmatst_ResetSuccessState(void)
{
    TestCaseOk = TRUE;
}

/****************************************************************************
Name            : fdmatst_SetSuccessState
Description     : Load given value to success control
Parameters      :
****************************************************************************/
void fdmatst_SetSuccessState(BOOL Value)
{
    if (TestCaseOk != FALSE)
    {
        TestCaseOk = Value;
    }
}

/****************************************************************************
Name            : fdmatst_IsTestSuccessful
Description     : Returns current condition of test case
Parameters      :
****************************************************************************/
BOOL fdmatst_IsTestSuccessful(void)
{
    return TestCaseOk;
}

/****************************************************************************
Name            : fdmatst_GetBaseAddress
Description     : Returns device base address
Parameters      :
****************************************************************************/
U32* fdmatst_GetBaseAddress(void)
{
    return BaseAddress_p;
}

/****************************************************************************
Name            : fdmatst_GetClocksPerSec
Description     : Returns device clock ticks per second
Parameters      :
****************************************************************************/
U32 fdmatst_GetClocksPerSec(void)
{
#ifdef STFDMA_EMU
    return (15625);
#else
    return (ST_GetClocksPerSecond());
#endif
}

/****************************************************************************
Name            : fdmatst_TestCaseSummarise
Description     : Summarise the test case execution
Parameters      : Ptr to string
****************************************************************************/
void fmdatst_TestCaseSummarise(void)
{
    TestCaseResults.Run++;

    if (fdmatst_IsTestSuccessful() == TRUE)
    {
        STTBX_Print((">>> PASSED\n"));
        TestCaseResults.Passed++;
    }
    else
    {
        STTBX_Print(("*** FAILED!! ***\n"));
        TestCaseResults.Failed++;
    }

    /* Clear success state for next test */
    fdmatst_ResetSuccessState();

    STTBX_Print(("--------------------------------------------------------\n"));
}

/****************************************************************************
Name         : fdmatst_ErrorReport()
Description  : Outputs the appropriate error message string.
Parameters   :
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.
Return Value : none
See Also     : ST_ErrorCode_t
****************************************************************************/
void fdmatst_ErrorReport(char *Func, ST_ErrorCode_t ErrorGiven, ST_ErrorCode_t ExpectedErr)
{
    switch( ErrorGiven )
    {
        case ST_NO_ERROR:
            VERBOSE_PRINT("-> ST_NO_ERROR\n");
            break;

        case ST_ERROR_ALREADY_INITIALIZED:
            VERBOSE_PRINT("-> ST_ERROR_ALREADY_INITIALIZED\n" );
            break;

        case ST_ERROR_UNKNOWN_DEVICE:
            VERBOSE_PRINT("-> ST_ERROR_UNKNOWN_DEVICE\n" );
            break;

        case ST_ERROR_INVALID_HANDLE:
            VERBOSE_PRINT("-> ST_ERROR_INVALID_HANDLE\n" );
            break;

        case ST_ERROR_OPEN_HANDLE:
            VERBOSE_PRINT("-> ST_ERROR_OPEN_HANDLE\n" );
            break;

        case ST_ERROR_BAD_PARAMETER:
            VERBOSE_PRINT("-> ST_ERROR_BAD_PARAMETER\n" );
            break;

        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            VERBOSE_PRINT("-> ST_ERROR_FEATURE_NOT_SUPPORTED\n" );
            break;

        case ST_ERROR_NO_MEMORY:
            VERBOSE_PRINT("-> ST_ERROR_NO_MEMORY\n" );
            break;

        case ST_ERROR_NO_FREE_HANDLES:
            VERBOSE_PRINT("-> ST_ERROR_NO_FREE_HANDLES\n" );
            break;

        case ST_ERROR_TIMEOUT:
            VERBOSE_PRINT("-> ST_ERROR_TIMEOUT\n" );
            break;

        case ST_ERROR_INTERRUPT_INSTALL:
            VERBOSE_PRINT("-> ST_ERROR_INTERRUPT_INSTALL\n" );
            break;

        /* FDMA error codes */
        case STFDMA_ERROR_NOT_INITIALIZED:
            VERBOSE_PRINT("-> STFDMA_ERROR_NOT_INITIALIZED\n");
            break;

        case STFDMA_ERROR_DEVICE_NOT_SUPPORTED:
            VERBOSE_PRINT("-> STFDMA_ERROR_DEVICE_NOT_SUPPORTED\n");
            break;

        case STFDMA_ERROR_NO_CALLBACK_TASK:
            VERBOSE_PRINT("-> STFDMA_ERROR_NO_CALLBACK_TASK\n");
            break;

        case STFDMA_ERROR_BLOCKING_TIMEOUT:
            VERBOSE_PRINT("-> STFDMA_ERROR_BLOCKING_TIMEOUT\n");
            break;

        case STFDMA_ERROR_CHANNEL_BUSY:
            VERBOSE_PRINT("-> STFDMA_ERROR_CHANNEL_BUSY\n");
            break;

        case STFDMA_ERROR_NO_FREE_CHANNELS:
            VERBOSE_PRINT("-> STFDMA_ERROR_NO_FREE_CHANNELS\n");
            break;

        case STFDMA_ERROR_ALL_CHANNELS_LOCKED:
            VERBOSE_PRINT("-> STFDMA_ERROR_ALL_CHANNELS_LOCKED\n");
            break;

        case STFDMA_ERROR_CHANNEL_NOT_LOCKED:
            VERBOSE_PRINT("-> STFDMA_ERROR_CHANNEL_NOT_LOCKED\n");
            break;

        case STFDMA_ERROR_UNKNOWN_CHANNEL_ID:
            VERBOSE_PRINT("-> STFDMA_ERROR_UNKNOWN_CHANNEL_ID\n");
            break;

        case STFDMA_ERROR_INVALID_TRANSFER_ID:
            VERBOSE_PRINT("-> STFDMA_ERROR_INVALID_TRANSFER_ID\n");
            break;

        case STFDMA_ERROR_TRANSFER_ABORTED:
            VERBOSE_PRINT("-> STFDMA_ERROR_TRANSFER_ABORTED\n");
            break;

        case STFDMA_ERROR_TRANSFER_IN_PROGRESS:
            VERBOSE_PRINT("-> STFDMA_ERROR_TRANSFER_IN_PROGRESS \n");
            break;

        case STFDMA_ERROR_INVALID_BUFFER:
            VERBOSE_PRINT("-> STFDMA_ERROR_INVALID_BUFFER \n");
            break;

        case STFDMA_ERROR_INVALID_CHANNEL:
            VERBOSE_PRINT("-> STFDMA_ERROR_INVALID_CHANNEL \n");
            break;

        case STFDMA_ERROR_INVALID_CONTEXT_ID:
            VERBOSE_PRINT("-> STFDMA_ERROR_INVALID_CONTEXT_ID \n");
            break;

        case STFDMA_ERROR_INVALID_SC_RANGE:
            VERBOSE_PRINT("-> STFDMA_ERROR_INVALID_SC_RANGE \n");
            break;

        case STFDMA_ERROR_NO_FREE_CONTEXTS:
            VERBOSE_PRINT("-> STFDMA_ERROR_NO_FREE_CONTEXTS \n");
            break;

        case STFDMA_ERROR_REQUEST_SIGNAL_BUSY:
            VERBOSE_PRINT("-> STFDMA_ERROR_REQUEST_SIGNAL_BUSY\n");
            break;

            /* others */
        default:
            VERBOSE_PRINT( "*** Unrecognised return code ***\n" );
    }

    /*
    if ErrorGiven does not match ExpectedErr, update
    ErrorStore only if no previous error was noted.
    */
    if( ErrorGiven != ExpectedErr )
    {
        STTBX_Print(("*** ERROR : Unexpected value returned (%s): Expected 0x%x, got: 0x%x  ***\n",
                      Func, ExpectedErr, ErrorGiven));
        fdmatst_SetSuccessState(FALSE);
    }
}

/****************************************************************************
Name            : fdmatst_GetSystemPartition
Description     : Returns the system partition
Parameters      :
****************************************************************************/
partition_t *fdmatst_GetSystemPartition(void)
{
#if defined (ST_OSLINUX) /* TODO */
    return (partition_t *)0x80000020;
#else
    return system_partition;
#endif
}

/****************************************************************************
Name            : fdmatst_GetInternalPartition
Description     : Returns the driver partition
Parameters      :
****************************************************************************/
partition_t *fdmatst_GetInternalPartition(void)
{
#if defined (ST_OSLINUX) /* TODO */
    return (partition_t *)0x80000020;
#elif defined (ST_OS21)
    return system_partition;
#else
    return internal_partition;
#endif
}

/****************************************************************************
Name            : fdmatst_GetNCachePartition
Description     : Returns the NCache partition
Parameters      :
****************************************************************************/
partition_t *fdmatst_GetNCachePartition(void)
{
#if defined (ST_OSLINUX) /* TODO */
    return (partition_t *)0x80000020;
#elif defined (ST_OS21)
    return ncache_partition;
#else
    return ncache_partition;
#endif
}

#if !defined (ST_OSLINUX)
/****************************************************************************
Name            : fdmatst_GetSMIPartition
Description     : Returns the SMI partition
Parameters      :
****************************************************************************/
partition_t *fdmatst_GetSMIPartition(void)
{
#if defined (ST_5517)
    return smi_partition;
#else
    return NULL;
#endif
}

/****************************************************************************
Name         : TBX_Init
Description  : STTBX initialisation funciton
Parameters   :
Return Value : FALSE if all ok.
See Also     :
****************************************************************************/
BOOL TBX_Init(void)
{
    ST_ErrorCode_t error    = ST_NO_ERROR;

    STTBX_InitParams_t TBXInitParams;

    /* Initialize the toolbox */
    TBXInitParams.SupportedDevices = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.CPUPartition_p = system_partition;
    error = STTBX_Init( "STTBX:0", &TBXInitParams );

    return error ? TRUE : FALSE;
}

/****************************************************************************
Name         : BOOT_Init
Description  : STBOOT initialisation funciton
Parameters   :
Return Value : FALSE if all ok.
See Also     :
****************************************************************************/
BOOL BOOT_Init(void)
{
    ST_ErrorCode_t error;
    STBOOT_InitParams_t     stboot_InitParams;

#if !defined (ST_OS21)
#if !defined (DISABLE_DCACHE)
    STBOOT_DCache_Area_t Cache[] =    { { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS }, /* assumed ok */
                                            { NULL, NULL }    };
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    STBOOT_DCache_Area_t Cache[] =    { { (U32 *)0x40200000, (U32 *)0x407FFFFF }, /* ok */
                                            { NULL, NULL }    };
#elif defined(ST_5301)
    STBOOT_DCache_Area_t Cache[] =    { { (U32 *)0xC0000000, (U32 *)0xC1FFFFFF }, /* ok */
                                            { NULL, NULL }    };
#elif defined(ST_8010)
    STBOOT_DCache_Area_t Cache[] =    { { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
                                            { NULL, NULL }    };
#elif defined(ST_5525)
    STBOOT_DCache_Area_t Cache[] =    { { (U32 *)0x80000000, (U32 *)0x813fffff }, /* 20 M ok */
                                            { NULL, NULL }    };
#else
    STBOOT_DCache_Area_t Cache[] = NULL;
#endif
#endif

#ifndef DISABLE_ICACHE
    stboot_InitParams.ICacheEnabled = TRUE;
#else
    stboot_InitParams.ICacheEnabled = FALSE;
#endif

#if defined (ST_OS21) || defined (ST_OSWINCE)
#ifndef DISABLE_DCACHE
    stboot_InitParams.DCacheEnabled = TRUE;
#else
    stboot_InitParams.DCacheEnabled = FALSE;
#endif
#endif

    stboot_InitParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    stboot_InitParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
#if defined (ST_OS21) || defined (ST_OSWINCE)
    stboot_InitParams.TimeslicingEnabled = TRUE;
#else
    stboot_InitParams.DCacheMap = Cache;
    stboot_InitParams.CacheBaseAddress          = (U32 *)CACHE_BASE_ADDRESS;
    stboot_InitParams.MemorySize                = SDRAM_SIZE;
    stboot_InitParams.SDRAMFrequency            = SDRAM_FREQUENCY;
#endif

#if defined (ST_5301) || defined (ST_5525)
    stboot_InitParams.DCacheMap = NULL;
#endif

    /* Initialize the boot driver */
    error = STBOOT_Init( "STBOOT:0", &stboot_InitParams );

    return ((error) ? (TRUE) : (FALSE));
}
#endif

/*--- private functions ---*/

/****************************************************************************
Name         : UpdateCounts
Description  : Stores
Parameters   :
Return Value :
****************************************************************************/
void UpdateCounts(void)
{
    TestCasesTotal.Run += TestCaseResults.Run;
    TestCasesTotal.Passed += TestCaseResults.Passed;
    TestCasesTotal.Failed += TestCaseResults.Failed;
    TestCaseResults.Run = 0;
    TestCaseResults.Passed = 0;
    TestCaseResults.Failed = 0;

    /* Clear out test case count */
    fdmatst_ResetSuccessState();

}

/****************************************************************************
Name         : main
Description  : Entry point to test harness. Calls test layer for test execution
Parameters   :
Return Value : 0
****************************************************************************/
#if defined (ST_OSLINUX)
int fdmatst_main(void)
#else
int main(void)
#endif
{
    BOOL InitOk = TRUE;

/* For STSYS_ST200UncachedOffset */
#if defined(ST200_OS21)
    STSYS_ST200UncachedOffset = (S32)mmap_translate_uncached(&STSYS_ST200UncachedOffset) - ((S32)(&STSYS_ST200UncachedOffset));
#endif /* ST200_OS21 */

#if defined (ST_OS20)
    partition_init_heap (&the_internal_partition, internal_block, sizeof(internal_block));
    internal_partition = &the_internal_partition;

    partition_init_heap (&the_system_partition, system_block, sizeof(system_block));
    system_partition = &the_system_partition;

    partition_init_heap (&the_ncache_partition, ncache_block, sizeof(ncache_block));
    ncache_partition = &the_ncache_partition;

/* Avoid compiler warnings */
    internal_block_init[0] = 0;
    system_block_init[0]   = 0;

#if defined(ST_5528)|| defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined (ST_5188) ||\
defined(ST_7100) || defined(ST_5700) || defined(ST_7109) || defined (ST_5525) || defined (ST_5107)
    data_section[0] = 0;
#endif

#elif defined (ST_OS21) && !defined (ST_OSWINCE)
    system_partition = partition_create_heap((void*)system_block, SYSTEM_PARTITION_SIZE);
    ncache_partition = partition_create_heap((void*)ncache_block, NCACHE_PARTITION_SIZE);

#elif defined (ST_OSWINCE)
    system_partition = partition_create_heap((void*)system_block, SYSTEM_PARTITION_SIZE);
    ncache_partition = partition_create_heap((void*)SHARED_NCACHE_BASE, NCACHE_PARTITION_SIZE);

#elif defined (ST_OSLINUX)
    /*
    From fdmatst_GetSystemPartition(). Even NULL can be passed as none of the linux APIs
    require the partition pointer.
    */
    system_partition = (partition_t *)0x80000020;
#endif

#if defined(ST_5517)
    partition_init_heap (&the_smi_partition, (void *)ACTUAL_SMI_BASE, SMI_PARTITION_SIZE);
#endif

#if defined (ST_5301)  || defined (ST_5525)
    DCacheOff();
#endif

#if defined (ST_OSLINUX)
    g_NodePool = dma_pool_create(g_NodePoolName,
        NULL,
        sizeof(STFDMA_GenericNode_t),
        32,
        PAGE_SIZE);
    if (NULL == g_NodePool)
    {
        printk(KERN_ALERT "Failed to create node pool\n");
        InitOk = FALSE;
    }
#else
    /* Initialize all drivers */
    if (BOOT_Init())
    {
        printf("Problem initializing STBoot!!!\n");
        InitOk = FALSE;
    }
    else if (TBX_Init())
    {
        printf("Problem initializing STTBX!!!\n");
        InitOk = FALSE;
    }
#endif

    pActiveNodeTableSemap = STOS_SemaphoreCreateFifo(system_partition, 1);

#if !defined (STFDMA_NO_PACED_TESTS)
#if defined (ST_OS20)
    PaceReqTaskDesc = (tdesc_t *)STOS_MemoryAllocate(system_partition, sizeof(tdesc_t));
#endif
#endif

    STTBX_Print(("********************************************************\n"));
    STTBX_Print(("                STFDMA Test Harness\n"));
    STTBX_Print(("Driver Revision: %s\n", STFDMA_GetRevision()));
    STTBX_Print(("Test Harness Revision: %s\n", stfdmatst_Rev));

#ifdef DISABLE_DCACHE
    STTBX_Print(("DCache DISABLED\n"));
#else
    STTBX_Print(("DCache ENABLED\n"));
#endif

#ifdef DISABLE_ICACHE
    STTBX_Print(("ICache DISABLED\n"));
#else
    STTBX_Print(("ICache ENABLED\n"));
#endif

    STTBX_Print(("********************************************************\n\n"));

    /* Setup base address (or simulator, if required) */

#ifdef STFDMA_SIM
    /* Allocate block of memory the size of the FDMA block....32K */
    BaseAddress_p = (U32 *) STOS_MemoryAllocateClear(fdmatst_GetSystemPartition(),1,32*K);
    if (BaseAddress_p == NULL)
    {
        STTBX_Print(("**** ERR0R: No memory for FDMA sim\n"));
        InitOk = FALSE;
    }

    FDMASIM_Init(BaseAddress_p, FDMA_INTERRUPT_NUMBER,fdmatst_GetSystemPartition());
#else
    BaseAddress_p = (U32 *)FDMA_BASE_ADDRESS;
#endif

    /* Now start the tests */
    if (InitOk == TRUE)
    {
#ifdef STCM_TEST
        RunCPPTest();
        UpdateCounts();
#else
#ifdef SOAK
        soak_RunSoakTest();
        UpdateCounts();
#else
        state_RunStateTest();
        UpdateCounts();

        func_RunFuncTest();
        UpdateCounts();

        param_RunParamTest();
        UpdateCounts();

#if !defined (ST_OSLINUX)
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
        audiopaced_RunAudioPacedTest();
        UpdateCounts();
#endif
#endif

#if !defined (ST_OSLINUX)
        leak_RunLeakTest();
        UpdateCounts();
#endif

#endif
#endif
    }

    STTBX_Print(("========================================================\n"));
    STTBX_Print(("Test Execution Summary:\n"));
    STTBX_Print(("   Run:        %d\n", TestCasesTotal.Run));
    STTBX_Print(("   Passed:     %d\n", TestCasesTotal.Passed));
    STTBX_Print(("   Failed:     %d\n\n",TestCasesTotal.Failed));
    if ((TestCasesTotal.Run != 0) && (TestCasesTotal.Failed == 0))
    {
        STTBX_Print(("** PASSED **\n"));
    }
    else
    {
        STTBX_Print(("!! FAILED !!\n"));
    }
    STTBX_Print(("========================================================\n"));

#ifdef STFDMA_SIM
    STOS_MemoryDeallocate (fdmatst_GetNCachePartition(),BaseAddress_p);
#endif

#if defined (ST_OSLINUX)
    if (g_NodePool)
        dma_pool_destroy(g_NodePool);
#endif

    STOS_SemaphoreDelete(system_partition, pActiveNodeTableSemap);

#if !defined (STFDMA_NO_PACED_TESTS)
#if defined (ST_OS20)
    STOS_MemoryDeallocate(system_partition, PaceReqTaskDesc);
#endif
#endif

    return 0;
}

#if defined (ARCHITECTURE_ST40)
void fdmatst_CovertNodeAddressDataToPeripheral(STFDMA_Node_t* pFirstNode)
{
    STFDMA_Node_t* pNextNode;
    STFDMA_Node_t* pCurrentNode;

    pCurrentNode = pFirstNode;

    do
    {
        pNextNode = pCurrentNode->Next_p;
        if (NULL != pCurrentNode->Next_p)
        {
            pCurrentNode->Next_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(pCurrentNode->Next_p);
        }
        pCurrentNode->SourceAddress_p = TRANSLATE_BUFFER_ADDRESS_TO_PHYS(pCurrentNode->SourceAddress_p);
        pCurrentNode->DestinationAddress_p = TRANSLATE_BUFFER_ADDRESS_TO_PHYS(pCurrentNode->DestinationAddress_p);

        VERBOSE_PRINT_DATA("fdmatst_CovertNodeAddressDataToPeripheral - Node                       : 0x%08X\n", (U32)pCurrentNode);
        VERBOSE_PRINT_DATA("fdmatst_CovertNodeAddressDataToPeripheral - Node->Next_p               : 0x%08X\n", (U32)pCurrentNode->Next_p);
        VERBOSE_PRINT_DATA("fdmatst_CovertNodeAddressDataToPeripheral - Node->SourceAddress_p      : 0x%08X\n", (U32)pCurrentNode->SourceAddress_p);
        VERBOSE_PRINT_DATA("fdmatst_CovertNodeAddressDataToPeripheral - Node->DestinationAddress_p : 0x%08X\n", (U32)pCurrentNode->DestinationAddress_p);
        pCurrentNode = pNextNode;
    }
    while ((pCurrentNode != NULL) && (pCurrentNode != pFirstNode));
}

static void fdmatst_CovertNodeAddressDataToCPU(STFDMA_Node_t* pFirstNode)
{
    STFDMA_Node_t* pNextNode;
    STFDMA_Node_t* pCurrentNode;

    pCurrentNode = pFirstNode;

    do
    {
        pNextNode = pCurrentNode->Next_p;
        if (NULL != pCurrentNode->Next_p)
        {
            pCurrentNode->Next_p = (STFDMA_Node_t*)TRANSLATE_NODE_ADDRESS_TO_VIRT(pCurrentNode->Next_p);
        }
        pCurrentNode->SourceAddress_p = (void*)TRANSLATE_BUFFER_ADDRESS_TO_VIRT(pCurrentNode->SourceAddress_p);
        pCurrentNode->DestinationAddress_p = (void*)TRANSLATE_BUFFER_ADDRESS_TO_VIRT(pCurrentNode->DestinationAddress_p);

        VERBOSE_PRINT_DATA("fdmatst_CovertNodeAddressDataToCPU - Node                       : 0x%08X\n", (U32)pCurrentNode);
        VERBOSE_PRINT_DATA("fdmatst_CovertNodeAddressDataToCPU - Node->Next_p               : 0x%08X\n", (U32)pCurrentNode->Next_p);
        VERBOSE_PRINT_DATA("fdmatst_CovertNodeAddressDataToCPU - Node->SourceAddress_p      : 0x%08X\n", (U32)pCurrentNode->SourceAddress_p);
        VERBOSE_PRINT_DATA("fdmatst_CovertNodeAddressDataToCPU - Node->DestinationAddress_p : 0x%08X\n", (U32)pCurrentNode->DestinationAddress_p);

        if (pNextNode != NULL)
        {
            pCurrentNode = (STFDMA_Node_t*)TRANSLATE_NODE_ADDRESS_TO_VIRT(pNextNode);
        }
    }
    while ((pNextNode != NULL) && (pCurrentNode != pFirstNode));
}
#endif

#if defined (ST_OSLINUX)
dma_addr_t fdmatst_GetNodePhysicalAddressFromVirtual(STFDMA_GenericNode_t* pNode)
{
    U32 i = 0;

   /* Traverse list looking for the correct node */

   VERBOSE_PRINT_DATA("fdmatst_GetNodePhysicalAddressFromVirtual - Node virtual address: 0x%08X\n", (U32)pNode);
   STOS_SemaphoreWait(pActiveNodeTableSemap);

   for (i=0; i<MAX_NUM_NODES; i++)
   {
       /* Remove current node using deallocate pointer from array */
       if (((STFDMA_GenericNode_t*)pNode) == (ActiveNodes[i].DeallocateNode_p))
       {
           VERBOSE_PRINT_DATA("fdmatst_GetNodePhysicalAddressFromVirtual - Node physical address : 0x%08X\n", ActiveNodes[i].physical_address);
           STOS_SemaphoreSignal(pActiveNodeTableSemap);
           return ActiveNodes[i].physical_address;
       }
   }

   STOS_SemaphoreSignal(pActiveNodeTableSemap);

   VERBOSE_PRINT("fdmatst_GetNodePhysicalAddressFromVirtual - failed\n");
   return (dma_addr_t)NULL;
}

dma_addr_t fdmatst_GetBufferPhysicalAddressFromVirtual(void* buffer_addr)
{
    U32 i = 0;

    /* Traverse list looking for the correct node */

    VERBOSE_PRINT_DATA("fdmatst_GetBufferPhysicalAddressFromVirtual - Buffer virtual address : 0x%08X\n", (U32)buffer_addr);
    STOS_SemaphoreWait(pActiveNodeTableSemap);

    for (i=0; i<MAX_NUM_NODES; i++)
    {
        if ((U32*)buffer_addr == (ActiveBuffers[i].Src_p))
        {
            VERBOSE_PRINT_DATA("fdmatst_GetBufferPhysicalAddressFromVirtual - Buffer physical address : 0x%08X\n",
                ActiveBuffers[i].src_physical_address + ((U32)ActiveBuffers[i].Src_p - (U32)ActiveBuffers[i].DeallocateSrc_p));
            STOS_SemaphoreSignal(pActiveNodeTableSemap);
            return ActiveBuffers[i].src_physical_address + ((U32)ActiveBuffers[i].Src_p - (U32)ActiveBuffers[i].DeallocateSrc_p);
        }

        if ((U32*)buffer_addr == (ActiveBuffers[i].Dest_p))
        {
            VERBOSE_PRINT_DATA("fdmatst_GetBufferPhysicalAddressFromVirtual - Buffer physical address : 0x%08X\n",
                ActiveBuffers[i].dest_physical_address + ((U32)ActiveBuffers[i].Dest_p - (U32)ActiveBuffers[i].DeallocateDest_p));
            STOS_SemaphoreSignal(pActiveNodeTableSemap);
            return ActiveBuffers[i].dest_physical_address + ((U32)ActiveBuffers[i].Dest_p - (U32)ActiveBuffers[i].DeallocateDest_p);
        }
    }

    STOS_SemaphoreSignal(pActiveNodeTableSemap);

    VERBOSE_PRINT("fdmatst_GetBufferPhysicalAddressFromVirtual - failed\n");
    return (dma_addr_t)NULL;
}

STFDMA_GenericNode_t* fdmatst_GetNodeVirtualAddressFromPhysical(dma_addr_t pNode)
{
    U32 i = 0;

   /* Traverse list looking for the correct node */

   VERBOSE_PRINT_DATA("fdmatst_GetNodeVirtualAddressFromPhysical - Node physical address: 0x%08X\n", pNode);
   STOS_SemaphoreWait(pActiveNodeTableSemap);

   for (i=0; i<MAX_NUM_NODES; i++)
   {
       /* Remove current node using deallocate pointer from array */
       if (pNode == (ActiveNodes[i].physical_address))
       {
           VERBOSE_PRINT_DATA("fdmatst_GetNodeVirtualAddressFromPhysical - Node virtual address : 0x%08X\n", (U32)ActiveNodes[i].DeallocateNode_p);
           STOS_SemaphoreSignal(pActiveNodeTableSemap);
           return ActiveNodes[i].DeallocateNode_p;
       }
   }

   STOS_SemaphoreSignal(pActiveNodeTableSemap);


   VERBOSE_PRINT("fdmatst_GetNodeVirtualAddressFromPhysical - failed\n");
   return (STFDMA_GenericNode_t*)NULL;
}

void* fdmatst_GetBufferVirtualAddressFromPhysical(dma_addr_t buffer_addr)
{
    U32 i = 0;

   /* Traverse list looking for the correct node */

   VERBOSE_PRINT_DATA("fdmatst_GetBufferVirtualAddressFromPhysical - Buffer physical address : 0x%08X\n", buffer_addr);
   STOS_SemaphoreWait(pActiveNodeTableSemap);

   for (i=0; i<MAX_NUM_NODES; i++)
   {
       if (buffer_addr == (ActiveBuffers[i].src_physical_address + ((U32)ActiveBuffers[i].Src_p - (U32)ActiveBuffers[i].DeallocateSrc_p)))
       {
           VERBOSE_PRINT_DATA("fdmatst_GetBufferVirtualAddressFromPhysical - Buffer virtual address : 0x%08X\n", (U32)ActiveBuffers[i].Src_p);
           STOS_SemaphoreSignal(pActiveNodeTableSemap);
           return ActiveBuffers[i].Src_p;
       }

       if (buffer_addr == (ActiveBuffers[i].dest_physical_address + ((U32)ActiveBuffers[i].Dest_p - (U32)ActiveBuffers[i].DeallocateDest_p)))
       {
           VERBOSE_PRINT_DATA("fdmatst_GetBufferVirtualAddressFromPhysical - Buffer virtual address : 0x%08X\n", (U32)ActiveBuffers[i].Dest_p);
           STOS_SemaphoreSignal(pActiveNodeTableSemap);
           return ActiveBuffers[i].Dest_p;
       }
   }

   STOS_SemaphoreSignal(pActiveNodeTableSemap);

   VERBOSE_PRINT("fdmatst_GetBufferVirtualAddressFromPhysical - failed\n");
   return NULL;
}
#endif

#if defined (ST_5188)
/* FEI functions */

void STFEI_Init()
{
	/* config FEI  in rs mode */

	/* configuration Register
	Bit 0 = 1 => use protocol
	Bit 1 = 1 => use ext_sync_pol == active high
	Bit 2 = 1 => use dvalid_pol == active high
	Bit 3 = 1 => use evalid_pol == active high
	Bit 4 = 0 => sample edge
	Bit 5 = 0 => endianess
	Bit [31:6] = n.a => reserved */

/*    U32 i = 10;*/
	mailbox_interrupt_received = 0;

/*	rsisr.n_intrs = 0;
	for (i = 0; i < RS_INTRS; i++)
	{
   		rsisr.n_status[i] = 0;
   		rsisr.time_intrs[i] = 0;
	}*/

	STSYS_WriteRegDev32LE (FEI_SOFT_RST, 0x01);

	STSYS_WriteRegDev32LE (FEI_CONFIG, 0x5E);
	STSYS_WriteRegDev32LE (FEI_SECTOR_SIZE, 0xBC);

/*    STSYS_WriteRegDev32LE (FEI_SOFT_RST, 0x01);*/

/*    STTBX_Print(("\rFEI_Setup=ST_NO_ERROR\n"));*/

    return;
}

/*------------------------------------------------------------------------------
 Name    : STFEI_Start
 Purpose : Remove FEI Soft Reset
 In      : -
 Out     : - Integer
 Note    :
------------------------------------------------------------------------------*/
void STFEI_Start ()
{
/*	  STSYS_WriteRegDev32LE (FEI_SOFT_RST, 0x01);*/
/*	  TraceBuffer(("\rFEI_Start=ST_NO_ERROR"));*/
}
#endif

/* eof */
