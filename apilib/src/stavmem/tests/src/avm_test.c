/*******************************************************************************

File name : avm_test.c

Description : tests

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
17 jun 2000         Created                                          HG
 6 Oct 2000         Suppressed handles in mem access func
                    Suppressed Open/Close and introduce
                    partitions functions                             HG
13 Oct 2000         Added base addresses as init params              HG
10 Jan 2000         Added video, SDRAM base addresses and size,
                    and cached memory map in init params.
                    Moved away init related to mem access module.    HG
03 Jan 2001         Modified for mb275b board support                CL
24 Apr 2001         Add shared memory virtual addresses tests        HSdLM
11 Jun 2001         Add GPDMA method tests.                          HSdLM
06 Jul 2001         Add tests for stack metrics                      HSdLM
07 Aou 2001         Update for mb314 board.                          HSdLM
19 Sep 2001         Update for ST40GX1 / mb317b                      HSdLM
04 Apr 2002         Reorganize tests for speed metrics               HSdLM
20 Nov 2002         Add FDMA method tests.                           HSdLM
17 Apr 2003         Multi-partitions support                         HSdLM
29 Jan 2004         Get Memory Status                                Walid ATROUS
*******************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */

/*#define ANOTHER_TASK*/

#if defined(ST_OS21) || defined(ST_OSWINCE)
#define FORCE_32BITS_ALIGN
#endif

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef ST_OS20
#include <move2d.h>
#endif /* ST_OS20*/
#include "stdevice.h"
#include "stddefs.h"

#include "stsys.h"
#include "stcommon.h"

#include "stboot.h"
#include "clboot.h"

#include "testtool.h"
#include "sttbx.h"

#include "stavmem.h"
#include "api_cmd.h"
#include "startup.h"
#include "avm_test.h"
#include "clavmem.h"
#include "memory_map.h"
#include "avm_spd.h"
#include "avm_mac.h"

#ifdef STAVMEM_MEM_ACCESS_GPDMA
#include "clgpdma.h"
#endif

#ifdef STAVMEM_MEM_ACCESS_FDMA
#include "clfdma.h"
#endif

#ifdef DEBUG_CODE
#include "avm_init.h"
#include "avm_allo.h"
#endif

#if defined (DVD_SECURED_CHIP)
#include "stmes.h"
#endif

/* External global variables ------------------------------------------------ */

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#if (MAX_PARTITIONS == 1)
#define TESTS_MAX_PARTITIONS 2
#define SLICE_MEMORY_IN_PARTITIONS
#else
#define TESTS_MAX_PARTITIONS MAX_PARTITIONS
#endif

#define DRIVER_DEVICE_NAME        "STACKUSAGE"
#define STACK_SIZE                4096 /* STACKSIZE must oversize the estimated stack usage of the driver */

#ifdef DEBUG_CODE
#define SDRAM_BANK_SIZE      0x00200000
#define SDRAM_BANK_SIZE_9_TENS (SDRAM_BANK_SIZE * 9 / 10)
#define SDRAM_BANK_SIZE_8_TENS (SDRAM_BANK_SIZE * 8 / 10)
#define SDRAM_BANK_SIZE_7_TENS (SDRAM_BANK_SIZE * 7 / 10)
#define SDRAM_BANK_SIZE_6_TENS (SDRAM_BANK_SIZE * 6 / 10)
#define SDRAM_BANK_SIZE_5_TENS (SDRAM_BANK_SIZE * 5 / 10)
#define SDRAM_BANK_SIZE_4_TENS (SDRAM_BANK_SIZE * 4 / 10)
#define SDRAM_BANK_SIZE_3_TENS (SDRAM_BANK_SIZE * 3 / 10)
#define SDRAM_BANK_SIZE_2_TENS (SDRAM_BANK_SIZE * 2 / 10)
#define SDRAM_BANK_SIZE_1_TENS (SDRAM_BANK_SIZE * 1 / 10)
#endif /* #ifdef DEBUG_CODE */

#define RANDOM_TESTS_MAX_BLOCKS 50

/* Private Variables (static)------------------------------------------------ */

STAVMEM_InitParams_t GlobalInitAvmem;
STAVMEM_Capability_t GlobalCapableOf;
STAVMEM_CreatePartitionParams_t GlobalCreateParams;
U32 GlobalAskedFreeSize;
STAVMEM_MemoryRange_t GlobalForbidRange[2];
STAVMEM_BlockHandle_t GlobalMyBlock = STAVMEM_INVALID_BLOCK_HANDLE;
void *GlobalForbidBorder[1];
U8 GlobalZeroPattern = 0;
STAVMEM_AllocBlockParams_t GlobalAllocParams;
STAVMEM_ReAllocBlockParams_t GlobalReAllocParams;
STAVMEM_BlockParams_t GlobalMyBlockParams;
STAVMEM_FreeBlockParams_t GlobalFreeParams;
STAVMEM_DeletePartitionParams_t GlobalDeleteParams;
STAVMEM_TermParams_t GlobalTermAvmem;

static STAVMEM_SharedMemoryVirtualMapping_t SharedMemoryVirtualMappingGiven_s;
static STAVMEM_SharedMemoryVirtualMapping_t SharedMemoryVirtualMappingRetrieved_s;


#ifdef ST_WINCE_BLOCK_TEST
extern STAVMEM_MemoryRange_t PartitionMapTable[MAX_PARTITIONS+1];
extern STAVMEM_PartitionHandle_t AvmemPartitionHandle[MAX_PARTITIONS+1];
#define PartitionHandleTable AvmemPartitionHandle       /* Using Stapigat Partitions*/
#else /* ST_WINCE_BLOCK_TEST*/
static STAVMEM_MemoryRange_t     PartitionMapTable[TESTS_MAX_PARTITIONS +1];
static STAVMEM_PartitionHandle_t PartitionHandleTable[TESTS_MAX_PARTITIONS +1];
#endif /* ST_WINCE_BLOCK_TEST*/

/* Global Variables --------------------------------------------------------- */

STAVMEM_SharedMemoryVirtualMapping_t * VMap_p = &SharedMemoryVirtualMappingRetrieved_s;

U32 SDRamBaseAddr[] = {   SDRAM_CPU_BASE_ADDRESS
#if (MAX_PARTITIONS == 2)
                        , PARTITION1_START
#endif
#if (MAX_PARTITIONS == 3)
                        , PARTITION1_START
                        , PARTITION2_START
#endif
                      };

char * SDRamName[] = {   "SDRAM"
#if (MAX_PARTITIONS == 2)
                    , "PARTITION 1"
#endif
#if (MAX_PARTITIONS == 3)
                    , "PARTITION 1"
                    , "PARTITION 2"
#endif
                  };

/* Private Macros ----------------------------------------------------------- */

#ifdef FORCE_32BITS_ALIGN
#define GETBYTE1(v) (U8)((v)&0xFF)
#define GETBYTE2(v) (U8)(((v)&0xFF00)>>8)
#define GETBYTE3(v) (U8)(((v)&0xFF0000)>>16)
#define GETBYTE4(v) (U8)(((v)&0xFF000000)>>24)
#endif


#if defined (STAVMEM_NO_COPY_FILL)|| defined (DVD_SECURED_CHIP)
void Inv_memcpy(void *Src, void *Dst, U32 Size)
{
    memcpy(Dst, Src, Size);
}
#endif /*STAVMEM_NO_COPY_FILL*/

/*Trace Dynamic Data Size---------------------------------------------------- */

#ifdef TRACE_DYNAMIC_DATASIZE
    U32  DynamicDataSize ;
#endif
/* Private Function prototypes ---------------------------------------------- */

#if defined (DVD_SECURED_CHIP) && !defined(STLAYER_NO_STMES)

#define SYS_REGION_BASE_ADDR 0x05580200
#define REGION_SZ 0x800000

BOOL LAYER_STMESMode(U32 mode, char *Mode_p);
BOOL LAYER_STMESError(ST_ErrorCode_t Error, char *Error_p);
#endif


#ifdef ANOTHER_TASK
static void AnotherTask(void);
#endif
static BOOL AvmemHelp(STTST_Parse_t *Parse_p, char *ResultSymbol_p);

static ST_ErrorCode_t StandardInit(STAVMEM_InitParams_t * const Init_p);
static BOOL CreateStandardPartitions(void);
static void DisplayBlockInfo(STAVMEM_BlockHandle_t BlockHandle);
#ifdef DEBUG_CODE
static BOOL DisplayAllBlocksInfo(const BOOL NotOnlyErrors);
#endif
static U32 VerifyAllocBlock(STAVMEM_BlockHandle_t BlockHandle, STAVMEM_AllocBlockParams_t *AllocParams_p);
static U32 VerifyReAllocBlock(STAVMEM_BlockHandle_t BlockHandle, STAVMEM_ReAllocBlockParams_t *ReAllocParams_p);
static U32 AllocAndVerify(STAVMEM_AllocBlockParams_t *AllocBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p, const BOOL DisplayResults);
static U32 FreeAndVerify(STAVMEM_FreeBlockParams_t *FreeBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p, const BOOL DisplayResults);
static U32 ReAllocAndVerify(STAVMEM_ReAllocBlockParams_t *ReAllocBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p, const BOOL DisplayResults);

static BOOL Set_SdramN(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
#ifdef DRAM_TEST
static BOOL dram(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
#endif
static BOOL dump(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
static BOOL nodump(STTST_Parse_t *Parse_p, char *ResultSymbol_p);

static BOOL Set_DeviceType(STTST_Parse_t *Parse_p, char *ResultSymbol_p);

#ifdef ST_OS20
static void ReportError(int Error, char *FunctionName);
static void test_overhead(void *dummy);
static void test_typical(void *dummy);
static void metrics_Stack_Test(void);
#endif /* ST_OS20 */

static BOOL TTAVMEM_Init(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
static BOOL TTAVMEM_Term(STTST_Parse_t *Parse_p, char *ResultSymbol_p);

static BOOL Tests1D(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
static BOOL Test1DFill(const char *AreaName_p, void *DestAddr_p, const U32 PatternSize, const U32 Size);
static BOOL Test1DCopy(const char *AreaName_p, void *SourceAddr_p, void *DestAddr_p, const U32 Size);

static BOOL Tests2DCopy(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
static BOOL Test2DCopySamePitch(void *DeleteAddr_p, void *SourceAddr_p, void *DestAddr_p);
static BOOL Test2DCopyGreaterPitch(void *DeleteAddr_p, void *SourceAddr_p, void *DestAddr_p);
static BOOL Test2DCopySmallerPitch(void *DeleteAddr_p, void *SourceAddr_p, void *DestAddr_p);
/*static BOOL Test2DCopyPitchLessThanSize(void *DeleteAddr_p, void *SourceAddr_p, void *DestAddr_p);*/

static BOOL Tests2DFill(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
static BOOL Test2DFillNormal(void *DeleteAddr_p, void *DestAddr_p, U32 PatternSize);

static BOOL TestsBlocksAlloc(STTST_Parse_t *Parse_p ,char *ResultSymbol_p);

#ifdef DEBUG_CODE
static BOOL TestBlockAllocDebug(void);
#endif
static BOOL TestBlockAllocRand(void);

#ifdef STAVMEM_DEBUG_BLOCKS_INFO
static BOOL AvmemListBlocks(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
#endif /* #ifdef STAVMEM_DEBUG_BLOCKS_INFO */
static BOOL TestsPartitions(STTST_Parse_t *Parse_p ,char *ResultSymbol_p);
static BOOL TestMultipleCreateDelete(void);
static BOOL TestDeleteWithBlocks(void);

static BOOL STAVMEM_MacroInit (void);

/* Functions ---------------------------------------------------------------- */
/*-----------------1/29/2004 11:20AM----------------
 * Get Memory Status
 * --------------------------------------------------*/
#ifdef STAVMEM_DEBUG_MEMORY_STATUS
#ifdef ST_OSWINCE
  extern ST_ErrorCode_t STAVMEM_GetMemoryStatus(const ST_DeviceName_t DeviceName, char *buf, int *len );
#else
  extern ST_ErrorCode_t STAVMEM_GetMemoryStatus(const ST_DeviceName_t DeviceName, char *buf, int *len, U8 PartitionIndex );
#endif
#endif

/* To be found in stavmem.lib ! */
extern ST_ErrorCode_t STAVMEM_GetLargestFreeBlockSize(STAVMEM_PartitionHandle_t PartitionHandle,
                                                      STAVMEM_AllocMode_t BlockAllocMode, U32 *LargestFreeBlockSize_p);
#ifdef STAVMEM_DEBUG_BLOCKS_INFO
extern void STAVMEM_DisplayAllBlocksInfo(void);
#endif /* #ifdef STAVMEM_DEBUG_BLOCKS_INFO */

void Inv_memmove(void *Src, void *Dst, U32 Size)
{
    memmove(Dst, Src, Size);
}

/*#define FlushDCache  {                                              \*/
/*    STSYS_WriteRegDev8((void *) 0x4600, 1);                         \*/
/*    while ((STSYS_ReadRegDev8((void *) 0x4900) & 0x10) != 0) { ; }  \*/
/*}*/
#define FlushDCache {}


#if 0
void example (void)
{
/* Variables definition */
STAVMEM_PartitionHandle_t MyPartitionHandle;
ST_ErrorCode_t Err = ST_NO_ERROR;

/* Initialise AVMEM inside SDRAM */
GlobalInitAvmem.CPUPartition_p          = SystemPartition_p;
GlobalInitAvmem.NCachePartition_p       = NCachePartition_p;
GlobalInitAvmem.DeviceType              = STAVMEM_DEVICE_TYPE_GENERIC;
GlobalInitAvmem.MaxPartitions           = 1;
GlobalInitAvmem.MaxBlocks               = 10;
GlobalInitAvmem.MaxForbiddenRanges      = 2;
GlobalInitAvmem.MaxForbiddenBorders     = 2;
GlobalInitAvmem.MaxNumberOfMemoryMapRanges = (sizeof(Partition) / sizeof(STAVMEM_MemoryRange_t));
GlobalInitAvmem.BlockMoveDmaBaseAddr_p  = (void *) BM_BASE_ADDRESS;
GlobalInitAvmem.CacheBaseAddr_p         = (void *) CACHE_BASE_ADDRESS;
GlobalInitAvmem.VideoBaseAddr_p         = (void *) VIDEO_BASE_ADDRESS;
GlobalInitAvmem.SDRAMBaseAddr_p         = (void *) MPEG_SDRAM_BASE_ADDRESS;
GlobalInitAvmem.SDRAMSize               = MPEG_SDRAM_SIZE;
GlobalInitAvmem.NumberOfDCachedRanges   = (DCacheMapSize / sizeof(STBOOT_DCache_Area_t)) - 1;
GlobalInitAvmem.DCachedRanges_p         = (STAVMEM_MemoryRange_t *) DCacheMap_p;
strcpy(GlobalInitAvmem.GpdmaName, STGPDMA_DEVICE_NAME);
GlobalInitAvmem.GpdmaName[ST_MAX_DEVICE_NAME - 1] = '\0';
Err = STAVMEM_Init(STAVMEM_DEVICE_NAME, &GlobalInitAvmem);

/* Create a partition */
PartitionMapTable[0].StartAddr_p           = (void *) VIRTUAL_BASE_ADDRESS;
PartitionMapTable[0].StopAddr_p            = (void *) (VIRTUAL_BASE_ADDRESS + VIRTUAL_WINDOW_SIZE - 1);
GlobalCreateParams.NumberOfPartitionRanges = (sizeof(PartitionMapTable[0]) / sizeof(STAVMEM_MemoryRange_t));;
GlobalCreateParams.PartitionRanges_p       = &PartitionMapTable[0];
Err = STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME, &GlobalCreateParams, &MyPartitionHandle);

/* Define forbidden ranges and border for allocation:
-Don't allocate block on lowest 64Kbytes or on highest 64Kbytes
-Don't allocate block across the two SDRAM banks */
GlobalForbidRange[0].StartAddr_p = (void *) (SDRAM_CPU_BASE_ADDRESS);
GlobalForbidRange[0].StopAddr_p  = (void *) (SDRAM_CPU_BASE_ADDRESS + 0xffff);
GlobalForbidRange[1].StartAddr_p = (void *) (SDRAM_CPU_BASE_ADDRESS + 0x3f0000);
GlobalForbidRange[1].StopAddr_p  = (void *) (SDRAM_CPU_BASE_ADDRESS + 0x3fffff);
GlobalForbidBorder[0] = (void *) (SDRAM_CPU_BASE_ADDRESS + VIRTUAL_SIZE / 2);

/* Allocate a block */
GlobalAllocParams.PartitionHandle = MyPartitionHandle;
GlobalAllocParams.Size = 720 * 576;
GlobalAllocParams.Alignment = 1;
GlobalAllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
GlobalAllocParams.ForbiddenRangeArray_p = &GlobalForbidRange[0];
GlobalAllocParams.ForbiddenBorderArray_p = &GlobalForbidBorder[0];
GlobalAllocParams.NumberOfForbiddenRanges = 2;
GlobalAllocParams.NumberOfForbiddenBorders = 1;
Err = STAVMEM_AllocBlock(&GlobalAllocParams, &GlobalMyBlock);

/* Get parameters of the allocated block */
Err = STAVMEM_GetBlockParams(MyBlock, &MyBlockParams);

/* Fill the allocated block with zeros */
Err = STAVMEM_FillBlock1D((void *) &GlobalZeroPattern, 1, MyBlockParams.StartAddr_p, MyBlockParams.Size);

/* Re-allocate the block with a larger size but same constraints */
GlobalReAllocParams.PartitionHandle = MyPartitionHandle;
GlobalReAllocParams.Size = 720 * 576 * 2;
GlobalReAllocParams.ForbiddenRangeArray_p = &GlobalForbidRange[0];
GlobalReAllocParams.ForbiddenBorderArray_p = &GlobalForbidBorder[0];
GlobalReAllocParams.NumberOfForbiddenRanges = 2;
GlobalReAllocParams.NumberOfForbiddenBorders = 1;
GlobalReAllocParams.PreserveData = TRUE;
GlobalReAllocParams.PreserveDataFunction = (STAVMEM_PreserveDataFunction_t) STAVMEM_CopyBlock1D;
Err = STAVMEM_ReAllocBlock(&GlobalReAllocParams, &GlobalMyBlock);

/* Get parameters of the re-allocated block */
Err = STAVMEM_GetBlockParams(GlobalMyBlock, &GlobalMyBlockParams);

/* Copy a bitmap from CPU memory to the allocated block */
Err = STAVMEM_CopyBlock2D((void *) 0x40000000, 125, 125, 720, GlobalMyBlockParams.StartAddr_p, 720);

/* USE THE BLOCK HERE FOR OTHER OPERATIONS */

/* Free the block (not necessary with ForceDelete) */
GlobalFreeParams.PartitionHandle = MyPartitionHandle;
Err = STAVMEM_FreeBlock(&GlobalFreeParams, &GlobalMyBlock);

/* Delete partition (not necessary with ForceTerminate) */
GlobalDeleteParams.ForceDelete = TRUE;
Err = STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &GlobalDeleteParams, &MyPartitionHandle);

/* Terminate STAVMEM */
GlobalTermAvmem.ForceTerminate = TRUE;
Err = STAVMEM_Term(STAVMEM_DEVICE_NAME, &GlobalTermAvmem);
}
#endif /* #if 0 */



#ifdef ANOTHER_TASK
/*******************************************************************************
Name        : AnotherTask
Description : routine of a second task
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
void AnotherTask(void)
{
/*    char dummy[21];
    U32 err = 0;*/

    STTBX_Print(("ANOTHERTASK started.\n"));
/*    LedInit();
    LedOff();
    debuggetkey(dummy);
    LedOn();*/

    while (TRUE) {
/*        err = STTBX_InputStr(dummy, 10);
        STTBX_Print(("You entered '%s' in ANOTHERTASK, length is %d\n", dummy, err));*/
/*        STTBX_InputChar(dummy);
        STTBX_Print(("You entered '%c' in ANOTHERTASK\n", dummy[0]));*/
        memcpy((void *) 0xc03f0000, (void *) 0xc03f0001, 65534);
        STTBX_Print(("Copied...\n"));
    }
}
#endif /* #ifdef ANOTHER_TASK */



/*******************************************************************************
Name        : AvmemHelp
Description : Display help
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : FALSE (success)
*******************************************************************************/
static BOOL AvmemHelp(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr = FALSE;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    STTBX_Print(("\nNotes:\n"
                "AVMEM_Partitions and AVMEM_BlocksAlloc are stand alone tests. They don't require AVMEM_Init.\n"
                "Other tests require AVMEM_Init before running and AVMEM_Term after.\n\n"));

    return(RetErr);
}

/*******************************************************************************
Name        : Set_SdramN
Description : Sets working memory to SDRAM n
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL Set_SdramN(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr = FALSE;
    U8 SdramN;
    S32 Var;
    UNUSED_PARAMETER(ResultSymbol_p);

    RetErr = STTST_GetInteger( Parse_p, 0, &Var);
    if ( (RetErr) || (Var < 0) || (Var > MAX_PARTITIONS))
    {
        STTST_TagCurrentLine( Parse_p, "Expected SDRAM n [0 - Max partitions](default 0)" );
        return(TRUE);
    }
    SdramN = (U8)Var;

    STTST_AssignInteger("membase", SDRamBaseAddr[SdramN], FALSE);

    STTBX_Print(("Now working on memory area SDRAM%d, starting from address 0x%x\n", SdramN, SDRamBaseAddr[SdramN]));

    return(RetErr);
}

#ifdef DRAM_TEST
/*******************************************************************************
Name        : dram
Description : Sets working memory to DRAM
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL dram(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr = FALSE;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    STTST_AssignInteger("membase", DRAM_BASE_ADDRESS, FALSE);

    STTBX_Print(("Now working on memory area DRAM, starting from address 0x%x\n", DRAM_BASE_ADDRESS));

    return(RetErr);
}
#endif

/*******************************************************************************
Name        : dump
Description : Enable memory display when testing
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL dump(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr = FALSE;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);

    STTST_AssignInteger("memdump", 1, FALSE);

    STTBX_Print(("Now displaying memory when testing AVMEM\n"));

    return(RetErr);
}

/*******************************************************************************
Name        : nodump
Description : Disable memory display when testing
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL nodump(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr = FALSE;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    STTST_AssignInteger("memdump", 0, FALSE);

    STTBX_Print(("Now NOT displaying memory when testing AVMEM\n"));

    return(RetErr);
}

/*******************************************************************************
Name        : Set_DeviceType
Description : Sets DeviceType
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL Set_DeviceType(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr = FALSE;
    char* Type[] = {"GENERIC", "VIRTUAL"};
    char* Var_i;
    S32 Var;
    UNUSED_PARAMETER(ResultSymbol_p);


    RetErr = STTST_GetInteger( Parse_p, STAVMEM_DEVICE_TYPE_VIRTUAL, &Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( Parse_p, "Expected DeviceType" );
        return(TRUE);
    }

    STTST_AssignInteger("devicetype", (STAVMEM_DeviceType_t)Var, FALSE);
    Var_i = Type[(STAVMEM_DeviceType_t)Var];
    STTBX_Print(("Now working with DeviceType STAVMEM_DEVICE_TYPE_%s\n", Var_i));

    return(RetErr);
}

/*******************************************************************************/
/*                               METRICS                                       */
/*******************************************************************************/
#ifdef ST_OS20
static void ReportError(int Error, char *FunctionName)
{
    if ((Error) != ST_NO_ERROR)
    {
        printf( "ERROR: %s returned %d\n", FunctionName, Error );
    }
}

/*******************************************************************************
Name        : test_typical
Description : calculates the stack usage made by the driver for its typical
              conditions of use
Parameters  : None
Assumptions :
Limitations : Make sure to not define local variables within the function
              but use module static globals instead in order to not pollute the
              stack usage calculation.
Returns     : None
*******************************************************************************/
void test_typical(void *dummy)
{
    ST_ErrorCode_t Err;
    BOOL RetOk;

    RetOk = AVMEM_Init();
    ReportError(!RetOk, "STAVMEM_Init");
    Err = STAVMEM_GetCapability(STAVMEM_DEVICE_NAME, &GlobalCapableOf);
    ReportError(Err, "STAVMEM_GetCapability");

    Err = STAVMEM_GetFreeSize(STAVMEM_DEVICE_NAME, &GlobalAskedFreeSize);
    ReportError(Err, "STAVMEM_GetFreeSize");

    STAVMEM_GetSharedMemoryVirtualMapping(VMap_p);

    /* Define forbidden ranges and border for allocation:
    -Don't allocate block on lowest 64Kbytes or on highest 64Kbytes
    -Don't allocate block across the two SDRAM banks */
    GlobalForbidRange[0].StartAddr_p = (void *) (SDRAM_CPU_BASE_ADDRESS);
    GlobalForbidRange[0].StopAddr_p  = (void *) (SDRAM_CPU_BASE_ADDRESS + 0xffff);
    GlobalForbidRange[1].StartAddr_p = (void *) (SDRAM_CPU_BASE_ADDRESS + 0x3f0000);
    GlobalForbidRange[1].StopAddr_p  = (void *) (SDRAM_CPU_BASE_ADDRESS + 0x3fffff);
    GlobalForbidBorder[0] = (void *) (SDRAM_CPU_BASE_ADDRESS + VIRTUAL_SIZE / 2);

    /* Allocate a block */
    GlobalAllocParams.PartitionHandle = AvmemPartitionHandle[0];
    GlobalAllocParams.Size = 720 * 576;
    GlobalAllocParams.Alignment = 1;
    GlobalAllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    GlobalAllocParams.ForbiddenRangeArray_p = &GlobalForbidRange[0];
    GlobalAllocParams.ForbiddenBorderArray_p = &GlobalForbidBorder[0];
    GlobalAllocParams.NumberOfForbiddenRanges = 2;
    GlobalAllocParams.NumberOfForbiddenBorders = 1;
    Err = STAVMEM_AllocBlock(&GlobalAllocParams, &GlobalMyBlock);
    ReportError(Err, "STAVMEM_AllocBlock");

    /* Get parameters of the allocated block */
    Err = STAVMEM_GetBlockParams(GlobalMyBlock, &GlobalMyBlockParams);
    ReportError(Err, "STAVMEM_GetBlockParams");
#ifndef STAVMEM_NO_COPY_FILL
    /* Fill the allocated block with zeros */
    Err = STAVMEM_FillBlock1D((void *) &GlobalZeroPattern, 1, GlobalMyBlockParams.StartAddr_p, GlobalMyBlockParams.Size);
    ReportError(Err, "STAVMEM_FillBlock1D");
#endif  /*STAVMEM_NO_COPY_FILL*/
    /* Re-allocate the block with a larger size but same constraints */
    GlobalReAllocParams.PartitionHandle = AvmemPartitionHandle[0];
    GlobalReAllocParams.Size = 720 * 576 * 2;
    GlobalReAllocParams.ForbiddenRangeArray_p = &GlobalForbidRange[0];
    GlobalReAllocParams.ForbiddenBorderArray_p = &GlobalForbidBorder[0];
    GlobalReAllocParams.NumberOfForbiddenRanges = 2;
    GlobalReAllocParams.NumberOfForbiddenBorders = 1;
    GlobalReAllocParams.PreserveData = TRUE;
#ifndef STAVMEM_NO_COPY_FILL
    GlobalReAllocParams.PreserveDataFunction = (STAVMEM_PreserveDataFunction_t) STAVMEM_CopyBlock1D;
#else
    GlobalReAllocParams.PreserveDataFunction = Inv_memcpy;
#endif  /*STAVMEM_NO_COPY_FILL*/
    Err = STAVMEM_ReAllocBlock(&GlobalReAllocParams, &GlobalMyBlock);
    ReportError(Err, "STAVMEM_ReAllocBlock");

    /* Get parameters of the re-allocated block */
    Err = STAVMEM_GetBlockParams(GlobalMyBlock, &GlobalMyBlockParams);
    ReportError(Err, "STAVMEM_GetBlockParams");

#ifndef STAVMEM_NO_COPY_FILL
    /* Copy a bitmap from CPU memory to the allocated block */
    Err = STAVMEM_CopyBlock2D((void *) 0x40000000, 125, 125, 720, GlobalMyBlockParams.StartAddr_p, 720);
    ReportError(Err, "STAVMEM_CopyBlock2D");
#endif /*STAVMEM_NO_COPY_FILL*/
    /* Free the block (not necessary with ForceDelete) */
    GlobalFreeParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&GlobalFreeParams, &GlobalMyBlock);
    ReportError(Err, "STAVMEM_FreeBlock");

    AVMEM_Term();

} /* end test_typical() */

void test_overhead(void *dummy)
{
    ST_ErrorCode_t Err;
    Err = ST_NO_ERROR;

   ReportError(Err, "test_overhead");
}


/*******************************************************************************
Name        : metrics_Stack_Test
Description : launch tasks to calculate the stack usage made by the driver
              for an Init Term cycle and in its typical conditions of use
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void metrics_Stack_Test(void)
{
    task_t *task_p;
    task_status_t status;
    int overhead_stackused;
    /*char stack[STACK_SIZE];*/
    char *funcname[]= {
        "test_overhead",
        "test_typical",
        "NULL"
    };
    void (*func_table[])(void *) = {
        test_overhead,
        test_typical,
        NULL
    };
    void (*func)(void *);
    int i;

    overhead_stackused = 0;
    printf("*************************************\n");
    printf("* Stack usage calculation beginning *\n");
    printf("*************************************\n");
    for (i = 0; func_table[i] != NULL; i++)
    {
        ST_ErrorCode_t RetErr;

        func = func_table[i];

        /* Start the task */
        RetErr = STOS_TaskCreate(func,
                                 NULL,
                                 NULL,
                                 STACK_SIZE,
                                 NULL,
                                 NULL,
                                 &task_p,
                                 NULL,
                                 MAX_USER_PRIORITY,
                                 "stack_test",
                                 task_flags_no_min_stack_size);

        /* Wait for task to complete */
        STOS_TaskWait(&task_p, TIMEOUT_INFINITY);

        /* Dump stack usage */
        task_status(task_p, &status, 1);
        /* store overhead value */
        if (i==0)
        {
            printf("*-----------------------------------*\n");
            overhead_stackused = status.task_stack_used;
            printf("%s \t func=0x%08lx stack = %d bytes used\n", funcname[i], (long) func,
                    status.task_stack_used);
            printf("*-----------------------------------*\n");
        }
        else
        {
            printf("%s \t func=0x%08lx stack = %d bytes used (%d - %d overhead)\n", funcname[i], (long) func,
                    status.task_stack_used-overhead_stackused,status.task_stack_used,overhead_stackused);
        }
        /* Tidy up */
        STOS_TaskDelete(task_p,NULL,NULL,NULL);
    }
    printf("*************************************\n");
    printf("*    Stack usage calculation end    *\n");
    printf("*************************************\n");
} /* end metrics_Stack_Test() */
#endif /* ST_OS20 */


#if defined (DVD_SECURED_CHIP)
/*-----------------------------------------------------------------------------
 * Function : AVMEM_STMESError
 *
 * Input    : ST_ErrorCode_t, char *
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL AVMEM_STMESError(ST_ErrorCode_t Error, char *Error_p)
{
    BOOL RetErr = FALSE;

    if (Error != ST_NO_ERROR)
    {
        RetErr = TRUE;
    }
    RetErr = TRUE;
    switch ( Error )
    {
    case 0 :
        strcpy(Error_p,"ST_NO_ERROR");
        break;
    case STMES_ERROR_ALREADY_INITIALISED :
        strcpy(Error_p,"STMES_ERROR_ALREADY_INITIALISED");
        break;
    case STMES_ERROR_NOT_INITIALISED :
        strcpy(Error_p,"STMES_ERROR_NOT_INITIALISED");
        break;
    case STMES_ERROR_ADDR_RANGE :
        strcpy(Error_p,"STMES_ERROR_ADDR_RANGE");
        break;
    case STMES_ERROR_ADDR_ALIGN :
        strcpy(Error_p,"STMES_ERROR_ADDR_ALIGN");
        break;
    case STMES_ERROR_MES_INCORRECT :
        strcpy(Error_p,"STMES_ERROR_MES_INCORRECT");
        break;
    case STMES_ERROR_SID_INCORRECT :
        strcpy(Error_p,"STMES_ERROR_SID_INCORRECT");
        break;
    case STMES_ERROR_NO_REGIONS :
        strcpy(Error_p,"STMES_ERROR_NO_REGIONS");
        break;
    case STMES_ERROR_BUSY :
        strcpy(Error_p,"STMES_ERROR_BUSY");
        break;
    default:
        strcpy(Error_p,"Undefied Error Description");
        break;
    }

    return( API_EnableError ? RetErr : FALSE);
}


/*--------------------------------------------------------------
 * Function : AVMEM_STMESMode
 *
 * Input    : ST_ErrorCode_t, char *
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL AVMEM_STMESMode(U32 mode, char *Mode_p)
{
    BOOL RetErr = FALSE;

    RetErr = TRUE;
    switch ( mode )
        {
    case ACCESS_ALL :
        strcat(Mode_p," ACCESS_ALL ");
        break;
    case ACCESS_NO_STORE_INSECURE :
        strcat(Mode_p," ACCESS_NO_STORE_INSECURE ");
        break;
    case ACCESS_NO_SECURE :
        strcat(Mode_p," ACCESS_NO_SECURE ");
        break;
    case ACCESS_LOAD_SECURE_ONLY :
        strcat(Mode_p,"ACCESS_LOAD_SECURE_ONLY ");
        break;
    default:
        strcat(Mode_p," Undefied Access Mode ");
    break;
        }

    return( API_EnableError ? RetErr : FALSE);
}

/*****************************************************************************/

/*-----------------------------------------------------------------------------
 * Function : AVMEM_SetMESMode1
 *             CPU is configured to access to SECURE Memory Region
 *             and  LMI Sys is configured as Secured region
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int AVMEM_SetMESMode1 (parse_t *pars_p, char *result_sym_p)
{

    ST_ErrorCode_t RetErr;
    STMES_InitParams_t Init;
    unsigned int uCPU_Mode;
    unsigned int uMemoryStatus;
    char       Text[50];


    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);


    /* get the driver version this can be done before the driver is Initialised. */
    printf("STMES Driver Version = %s\n", STMES_GetVersion());
    /* initialise the driver leaving all interrutps alone*/
    memset(&Init,0,sizeof(STMES_InitParams_t));
    if((RetErr= STMES_Init(&Init)) != ST_NO_ERROR)
    {
        printf(" MES Init Failed  :\n");
        return(FALSE);
    }

    AVMEM_STMESError(RetErr,Text);

    printf(" STMES_Init = %s \n",Text);
    strcpy(Text,"\0");


     RetErr=STMES_SetCPUAccess(SID_SH4_CPU,ACCESS_ALL);

     AVMEM_STMESError(RetErr,Text);
     printf(" STMES_SetCPUAccess = %s \n",Text);
     strcpy(Text,"\0");

     uCPU_Mode= STMES_GetSourceMode( SID_SH4_CPU, SYS_MES );

     AVMEM_STMESMode(uCPU_Mode,Text);

    printf(" Mesmory Mode is %s \n",Text);
    strcpy(Text,"\0");


    uMemoryStatus = STMES_SetSecureRange((void *)SYS_REGION_BASE_ADDR,
          (void *)(SYS_REGION_BASE_ADDR + REGION_SZ - 1), 0);
    AVMEM_STMESMode(uMemoryStatus,Text);

    if( uMemoryStatus != ST_NO_ERROR )
    {
        printf(" Error for STMES_SetSecureRange(0x%08x,0x%08x,N/A) : %s\n",
        SYS_REGION_BASE_ADDR, (SYS_REGION_BASE_ADDR + REGION_SZ - 1),Text);
    }
    else
    {
        printf(" Secure Region Created : Start Addr= 0x%08x : End Addr= 0x%08x\n",
        SYS_REGION_BASE_ADDR, (SYS_REGION_BASE_ADDR + REGION_SZ - 1) );
    }

   strcpy(Text,"\0");

    return 0;
}

/*-----------------------------------------------------------------------------
 * Function : AVMEM_SetMESMode2
 *             CPU is configured to access to SECURE Memory Region
 *             and  LMI Sys is configured as Insecured region
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int AVMEM_SetMESMode2 (parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STMES_InitParams_t Init;
    unsigned int uCPU_Mode;
    char       Text[50];

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);


    /* get the driver version this can be done before the driver is Initialised. */
    printf("STMES Driver Version= %s\n", STMES_GetVersion());
    /* initialise the driver leaving all interrutps alone*/
    memset(&Init,0,sizeof(STMES_InitParams_t));
    if((RetErr= STMES_Init(&Init)) != ST_NO_ERROR)
    {
        printf(" MES Init Failed  :\n");
        return(FALSE);
    }

    AVMEM_STMESError(RetErr,Text);

    printf(" STMES_Init = %s \n",Text);
    strcpy(Text,"\0");

    RetErr=STMES_SetCPUAccess(SID_SH4_CPU,ACCESS_ALL);

     AVMEM_STMESError(RetErr,Text);
     printf(" STMES_SetCPUAccess = %s \n",Text);
     strcpy(Text,"\0");


    uCPU_Mode= STMES_GetSourceMode( SID_SH4_CPU, SYS_MES );

    AVMEM_STMESMode(uCPU_Mode,Text);

    printf(" Mesmory Mode is %s \n",Text);
    strcpy(Text,"\0");

    return (0);
}

/*-----------------------------------------------------------------------------
 * Function : AVMEM_SetMESMode3
 *             CPU is configured to NO access to SECURE Memory Region
 *             and  LMI Sys is configured as Secured region
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int AVMEM_SetMESMode3 (parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STMES_InitParams_t Init;
    unsigned int uCPU_Mode;
    unsigned int uMemoryStatus;
    char       Text[50];

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);


    /* get the driver version this can be done before the driver is Initialised. */
    printf("STMES Driver Version= %s\n", STMES_GetVersion());
    /* initialise the driver leaving all interrutps alone*/
    memset(&Init,0,sizeof(STMES_InitParams_t));
    if((RetErr= STMES_Init(&Init)) != ST_NO_ERROR)
    {
        printf(" MES Init Failed  :\n");
        return(FALSE);
    }

    AVMEM_STMESError(RetErr,Text);

    printf(" STMES_Init = %s \n",Text);
    strcpy(Text,"\0");


    uCPU_Mode= STMES_GetSourceMode( SID_SH4_CPU, SYS_MES );

    AVMEM_STMESMode(uCPU_Mode,Text);

    printf(" Mesmory Mode is %s \n",Text);
    strcpy(Text,"\0");


    uMemoryStatus = STMES_SetSecureRange((void *)SYS_REGION_BASE_ADDR,
          (void *)(SYS_REGION_BASE_ADDR + REGION_SZ - 1), 0);
    AVMEM_STMESMode(uMemoryStatus,Text);

    if( uMemoryStatus != ST_NO_ERROR )
    {
        printf(" Error for STMES_SetSecureRange(0x%08x,0x%08x,N/A) : %s\n",
        SYS_REGION_BASE_ADDR, (SYS_REGION_BASE_ADDR + REGION_SZ - 1),Text);
    }
    else
    {
        printf(" Secure Region Created : Start Addr= 0x%08x : End Addr= 0x%08x\n",
        SYS_REGION_BASE_ADDR, (SYS_REGION_BASE_ADDR + REGION_SZ - 1) );
    }

    strcpy(Text,"\0");

    return (0);
}

/*-----------------------------------------------------------------------------
 * Function : AVMEM_SetMESMode4
 *             CPU is configured to NO access to SECURE Memory Region
 *             and  LMI Sys is configured as Insecured region
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int AVMEM_SetMESMode4 (parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STMES_InitParams_t Init;
    unsigned int uCPU_Mode;
    char       Text[50];


    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);


    /* get the driver version this can be done before the driver is Initialised. */
    printf("STMES Driver Version= %s\n", STMES_GetVersion());
    /* initialise the driver leaving all interrutps alone*/
    memset(&Init,0,sizeof(STMES_InitParams_t));
    if((RetErr= STMES_Init(&Init)) != ST_NO_ERROR)
    {
        printf(" MES Init Failed  :\n");
        return(FALSE);
    }

    AVMEM_STMESError(RetErr,Text);

    printf(" STMES_Init = %s \n",Text);
    strcpy(Text,"\0");


    uCPU_Mode= STMES_GetSourceMode( SID_SH4_CPU, SYS_MES );

    AVMEM_STMESMode(uCPU_Mode,Text);

    printf(" Mesmory Mode is %s \n",Text);
    strcpy(Text,"\0");

 return (FALSE);

}

/*-----------------------------------------------------------------------------
 * Function : AVMEM_TermMES
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int AVMEM_TermMES (parse_t *pars_p, char *result_sym_p)
{
     ST_ErrorCode_t RetErr;
     char       Text[50];


    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);
    /* finally terminate the driver as finished with it */
    if((RetErr= STMES_Term()) != ST_NO_ERROR)
    {
    printf(" MES Term Failed  :\n");
    return(FALSE);
    }

    AVMEM_STMESError(RetErr,Text);

    printf(" STMES_Term = %s \n",Text);
    strcpy(Text,"\0");

    return (FALSE);
}
#endif /*DVD_SECURED_CHIP*/

/*******************************************************************************
Name        : TTAVMEM_Init
Description : Initialise AVMEM and open 2 partitions
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL TTAVMEM_Init(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    STAVMEM_InitParams_t InitAvmem;
    STAVMEM_Capability_t CapableOf;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    BOOL RetErr = FALSE;
#if 0
#if defined (DVD_SECURED_CHIP)
    STMES_InitParams_t Init;
#endif
#endif
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    /* Initialise AVMEM and open a handle */
    STTST_EvaluateInteger("devicetype", (S32*)&InitAvmem.DeviceType, 16);
    Err = StandardInit(&InitAvmem);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Error: STAVMEM cannot be initialised !\n"));
        RetErr = TRUE;
    }
    else
    {
        /* Try to init a second time: should fail ! */
        Err = STAVMEM_Init(STAVMEM_DEVICE_NAME, &InitAvmem);
        if (Err == ST_NO_ERROR)
        {
            STTBX_Print(("Error: STAVMEM should not have been initialised a second time !\n"));
            RetErr = TRUE;
        }
        else
        {
            STTBX_Print(("STAVMEM cannot be initialised a second time. (OK)\n"));

            /* Test capabilities first */
            Err = STAVMEM_GetCapability(STAVMEM_DEVICE_NAME, &CapableOf);
#ifdef ST_OS20
            ReportError(Err, "STAVMEM_GetCapability");
#endif
            if (!CapableOf.IsCopyFillCapable)
            {
                STTBX_Print(("!!!No Copy and Fill functions available with STAVMEM!!!!!\n"));
            }

            if (CapableOf.MaxPartition != TESTS_MAX_PARTITIONS)
            {
                STTBX_Print(("Error: STAVMEM_GetCapability returns %d max partition instead of %d !\n", CapableOf.MaxPartition, MAX_PARTITIONS));
            }
            else
            {
                STTBX_Print(("STAVMEM is capable of creating a maximum of %d partition%c\n", CapableOf.MaxPartition, (CapableOf.MaxPartition>1?'s':' ')));
            }
            /* Create partitions */
            RetErr = CreateStandardPartitions();

            if ((!RetErr) && (InitAvmem.DeviceType == STAVMEM_DEVICE_TYPE_VIRTUAL))
            {
                STAVMEM_GetSharedMemoryVirtualMapping(VMap_p);
            }
#if 0
/*#if defined (DVD_SECURED_CHIP) */
            memset(&Init,0,sizeof(STMES_InitParams_t));
            if((RetErr = STMES_Init(&Init)) != ST_NO_ERROR)
            {
                printf("MES Init Failed !! \n");
            }
            else
            {
                printf("MES Init Succeeded \n");
            }
#endif
        }
    }

    if (!RetErr)
    {
        STTBX_Print(("\n**** No error in tests of Init and partitions creation **** \n\n"));
    }
    else
    {
        STTBX_Print(("\n**** THERE ARE ERRORS IN TESTS OF INIT AND PARTITIONS CREATION **** \n\n\a"));
    }

    return(RetErr);
}


/*******************************************************************************
Name        : TTAVMEM_Term
Description : Close and terminate AVMEM
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL TTAVMEM_Term(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    STAVMEM_DeletePartitionParams_t DeleteParams;
    STAVMEM_TermParams_t TermAvmem;
    U32 i;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    BOOL RetErr = FALSE;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    /* Delete partition */
    DeleteParams.ForceDelete = TRUE;
    for (i=0; i < TESTS_MAX_PARTITIONS; i++)
    {

        Err = STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &PartitionHandleTable[i]);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error: Partition #%d cannot be deleted !\n", i));
            RetErr = TRUE;
        }
    }
    /* Terminate AVMEM */
    if (STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermAvmem) != ST_NO_ERROR)
    {
        STTBX_Print(("Error: STAVMEM cannot be terminated because it was not initialised !\n"));
        RetErr = TRUE;
    }
    else
    {
        if (STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermAvmem) == ST_NO_ERROR)
        {
            STTBX_Print(("Error: STAVMEM should not have been terminated a second time !\n"));
            RetErr = TRUE;
        }
        else
        {
            STTBX_Print(("STAVMEM cannot be terminated a second time. (OK)\n"));
        }
    }

    if (!RetErr)
    {
        STTBX_Print(("\n**** No error in tests of partition deletion and Term **** \n\n"));
    }
    else
    {
        STTBX_Print(("\n**** THERE ARE ERRORS IN TESTS OF PARTITION DELETION AND TERM **** \n\n\a"));
    }

    return(RetErr);
}



/*******************************************************************************
Name        : Tests1D
Description : Test 1D functions (STAVMEM_FillBlock1D and STAVMEM_CopyBlock1D)
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL Tests1D( STTST_Parse_t *Parse_p, char *ResultSymbol_p )
{
    BOOL RetErr = FALSE;
    S32 basemem;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    /* Get current memory region */
    STTST_EvaluateInteger("membase", &basemem, 16);

    /* Fill 1D and verify for different offsets */
    RetErr = ((Test1DFill("0x11", (void *) (basemem + 1), 1, 0x10))) || (RetErr);
    RetErr |= ((Test1DFill("0x11, 0x22", (void *) (basemem + 1), 2, 0x10))) || (RetErr);
    RetErr |= ((Test1DFill("0x11, 0x22, 0x33", (void *) (basemem + 1), 3, 0x10))) || (RetErr);
    RetErr |= ((Test1DFill("0x11, 0x22, 0x33, 0x44", (void *) (basemem + 1), 4, 0x10))) || (RetErr);
    RetErr |= ((Test1DFill("0x11, 0x22, 0x33, 0x44, 0x55", (void *) (basemem + 3), 5, 0x10))) || (RetErr);
    RetErr |= ((Test1DFill("0x11, 0x22, 0x33, 0x44", (void *) (basemem + 2), 4, 0x17))) || (RetErr);
    RetErr |= ((Test1DFill("0x11, 0x22, 0x33, 0x44, 0x55", (void *) (basemem + 2), 5, 0x17))) || (RetErr);
    RetErr |= ((Test1DFill("0x11, 0x22, 0x33, 0x44", (void *) (basemem + 3), 4, 0x11))) || (RetErr);
    RetErr |= ((Test1DFill("0x11, 0x22, 0x33, 0x44, 0x55", (void *) (basemem + 3), 5, 0x11))) || (RetErr);

    /* Copy 1D and verify for different offsets */
    RetErr |= ((Test1DCopy("+ 100 ", (void *) (basemem + 0), (void *) (basemem + 0x100), 0x20))) || (RetErr);
    RetErr |= ((Test1DCopy("+ 1 ", (void *) (basemem + 0), (void *) (basemem + 0x1), 0x20))) || (RetErr);
    RetErr |= ((Test1DCopy("- 1 ", (void *) (basemem + 1), (void *) (basemem + 0x0), 0x20))) || (RetErr);
    RetErr |= ((Test1DCopy("", (void *) (basemem + 0), (void *) (basemem + 0x0), 0x20))) || (RetErr);

    if (!RetErr)
    {
        STTBX_Print(("**** No error in tests 1D **** \n\n"));
    }
    else
    {
        STTBX_Print(("**** THERE ARE ERRORS IN TESTS 1D **** \n\n\a"));
    }

    return(RetErr);
}


/*******************************************************************************
Name        : Test1DFill
Description : test STAVMEM_FillBlock1D function
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL Test1DFill(const char *Comment_p, void *DestAddr_p, const U32 PatternSize, const U32 Size)
{
 #if defined (STAVMEM_NO_COPY_FILL)
   UNUSED_PARAMETER(Comment_p);
   STTBX_Print(("****  feature not supported !!!!!!   ****\n"));
   return(FALSE);
 #else
    U8 empty[1] = {0};
    U8 pattern[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
    U32 Err = 0, Tmp = 0, Pat = 0, PatSize = PatternSize;
    void * PhyDestAddr_p;

    if (PatternSize > 5)
    {
        PatSize = 5;
    }

    /* shared memory is CPU accessed, so if address is in virtual range, check it can be accessed from CPU */
    if ((ISVIR(DestAddr_p)) && !(ISINVW(DestAddr_p, Size)))
    {
        STTBX_Print(("****  ERROR(S) !!! **** data range overlaps virtual window frontier \n"));
        return(TRUE);
    }

    /* Clear working memory areas */
    STAVMEM_FillBlock1D(empty, 1, (void *) ((U32) DestAddr_p & (U32)~0xF), Size);

    /* Write a pattern inside source and display it */
    STTBX_Print(("Filling with %s:\n", Comment_p));
    STAVMEM_FillBlock1D(pattern, PatSize, DestAddr_p, Size);
    DumpMemory(DestAddr_p, DUMP_2_LINES);
    /* Verify byte by byte that destination is right */
    PhyDestAddr_p = IFV2CPU(DestAddr_p);
    Pat = 0;
    for (Tmp = 0; Tmp < Size; Tmp++)
    {
        if (*((U8 *) (((U8 *) PhyDestAddr_p) + Tmp)) != pattern[Pat % PatSize])
        {
            /* One error found ! */
            Err++;
        }
        Pat++;
    }

    /* Display error(s) status */
    if (Err == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR !!! ****  (%d errors)\n\n", Err));
        return(TRUE);
    }
#endif /*STAVMEM_NO_COPY_FILL */
}


/*******************************************************************************
Name        : Test1DCopy
Description : test STAVMEM_CopyBlock1D function
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL Test1DCopy(const char *Comment_p, void *SourceAddr_p, void *DestAddr_p, const U32 Size)
{
 #if defined (STAVMEM_NO_COPY_FILL)
   UNUSED_PARAMETER(Comment_p);
   STTBX_Print(("****  feature not supported !!!!!!   ****\n"));
   return(FALSE);
 #else
    U8 pattern[3] = {0x11, 0x22, 0x33};
    U8 empty[1] = {0};
    U32 Err = 0, Tmp = 0, Pat = 0, PatSize = 3;
    void * PhyDestAddr_p;

    /* shared memory is CPU accessed, so if address is in virtual range, check it can be accessed from CPU */
    if (   ((ISVIR(SourceAddr_p)) && !(ISINVW(SourceAddr_p,  Size)))
        || ((ISVIR(DestAddr_p)) && !(ISINVW(DestAddr_p, Size))))
    {
        STTBX_Print(("****  ERROR(S) !!! **** data range overlaps virtual window frontier \n"));
        return(TRUE);
    }

    /* Clear working memory areas */
    STAVMEM_FillBlock1D(empty, 1, SourceAddr_p, Size); /* HSdLM 24 April 2001: remove '2*Size' */
    STAVMEM_FillBlock1D(empty, 1, DestAddr_p, Size);   /* HSdLM 24 April 2001: remove '2*Size' */

    /* Write a pattern inside source and display it */
    STAVMEM_FillBlock1D(pattern, PatSize, SourceAddr_p, Size);
    DumpMemory(SourceAddr_p, DUMP_3_LINES);
    STTBX_Print(("... Now copy at source address %s:\n", Comment_p));

    /* Copy source to destination and display it */
    STAVMEM_CopyBlock1D(SourceAddr_p, DestAddr_p, Size);
    DumpMemory(DestAddr_p, DUMP_3_LINES);

    /* Verify byte by byte that destination is right */
    PhyDestAddr_p = IFV2CPU(DestAddr_p);
    Pat = 0;
    for (Tmp = 0; Tmp < Size; Tmp++)
    {
        if (*((U8 *) (((U8 *) PhyDestAddr_p) + Tmp)) != pattern[Pat])
        {
            /* One error found ! */
            Err++;
        }
        Pat++;
        if (Pat >= PatSize)
        {
            Pat = 0;
        }
    }

    /* Display error(s) status */
    if (Err == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR !!! ****  (%d errors)\n\n", Err));
        return(TRUE);
    }
#endif /*(STAVMEM_NO_COPY_FILL) */
}


/*******************************************************************************
Name        : Tests2DCopy
Description : Test STAVMEM_CopyBlock2D function
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL Tests2DCopy( STTST_Parse_t *Parse_p, char *ResultSymbol_p )
{
    BOOL RetErr = FALSE;
    S32 basemem;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    STTST_EvaluateInteger("membase", &basemem, 16);

    /* Test copy with no overlap of regions */
    RetErr = ((Test2DCopySamePitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 16 + 10 )))) || (RetErr);
    /* Test copy with overlap on top right */
    RetErr |= ((Test2DCopySamePitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 16 + 6 )))) || (RetErr);
    /* Test copy with overlap on top left */
    RetErr |= ((Test2DCopySamePitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 16 + 4 )))) || (RetErr);
    /* Test copy with overlap on bottom right */
    RetErr |= ((Test2DCopySamePitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 - 16 + 6 )))) || (RetErr);
    /* Test copy with overlap on bottom left */
    RetErr |= ((Test2DCopySamePitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 - 16 + 4 )))) || (RetErr);

    /* Test copy with destination pitch greater than source pitch */
    RetErr |= ((Test2DCopyGreaterPitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 0 )))) || (RetErr);
    RetErr |= ((Test2DCopyGreaterPitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 3 )))) || (RetErr);
    RetErr |= ((Test2DCopyGreaterPitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 5 )))) || (RetErr);

    /* Test copy with destination pitch smaller than source pitch */
    RetErr |= ((Test2DCopySmallerPitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 5 )))) || (RetErr);
    RetErr |= ((Test2DCopySmallerPitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 7 )))) || (RetErr);
    RetErr |= ((Test2DCopySmallerPitch((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 10 )))) || (RetErr);

    /* Test copy with pitches smaller than sizes */
/*    RetErr = ((Test2DCopyPitchLessThanSize((void *) (basemem), (void *) (basemem + 16 + 5), (void *) (basemem + 16 + 16 )))) || (RetErr);*/

    if (!RetErr)
    {
        STTBX_Print(("**** No error in tests Copy 2D **** \n\n"));
    }
    else
    {
        STTBX_Print(("**** THERE ARE ERRORS IN TESTS OF COPY 2D **** \n\n\a"));
    }

    return(RetErr);
}


/*******************************************************************************
Name        : Test2DCopySamePitch
Description : Test 2D copy with same Pitch for source and destination
Parameters  : address of memory to clear before, of source, of destination
Assumptions :
Limitations :
Returns     : FALSE if success, TRUE otherwise
*******************************************************************************/
static BOOL Test2DCopySamePitch(void *DeleteAddr_p, void *SourceAddr_p, void *DestAddr_p)
{
 #if defined (STAVMEM_NO_COPY_FILL)
   STTBX_Print(("****  feature not supported !!!!!!   ****\n"));
   return(FALSE);
 #else

    U8 pattern1[3] = {0x11, 0x22, 0x33};
    U8 pattern2[3] = {0x44, 0x66, 0x99};
    U8 empty[1] = {0};
    U32 Err = 0, Tmp = 0, PatSize = 3;
    void * PhyDestAddr_p;

    /* Clear working memory area */
    STAVMEM_FillBlock1D(empty, 1, ((U8 *) DeleteAddr_p), 0x60);

    /* Write a pattern inside source and display it */
    STAVMEM_FillBlock1D(pattern1, PatSize, SourceAddr_p, PatSize);
    STAVMEM_FillBlock1D(pattern2, PatSize, (void *) ((U32 *) SourceAddr_p + 4), PatSize);
    DumpMemory(((U8 *) SourceAddr_p) - 16, DUMP_5_LINES);
    STTBX_Print(("... Now copy with equal pitches :\n"));

    /* Copy source to destination and display it */
    STAVMEM_CopyBlock2D(SourceAddr_p, PatSize, 2, 16, DestAddr_p, 16);
    DumpMemory(((U8 *) SourceAddr_p) - 16, DUMP_5_LINES);

    /* Verify byte by byte that destination is right */
    PhyDestAddr_p = IFV2CPU(DestAddr_p);
    for (Tmp = 0; Tmp < PatSize; Tmp++)
    {
        if (*((U8 *) (((U8 *) PhyDestAddr_p) + Tmp)) != pattern1[Tmp])
        {
            /* One error found ! */
            Err++;
        }
        if (*((U8 *) (((U8 *) PhyDestAddr_p) + 16 + Tmp)) != pattern2[Tmp])
        {
            /* One error found ! */
            Err++;
        }
    }

    /* Display error(s) status */
    if (Err == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR !!! ****  (%d errors)\n\n", Err));
        return(TRUE);
    }
#endif /*(STAVMEM_NO_COPY_FILL)*/
}

/*******************************************************************************
Name        : Test2DCopyGreaterPitch
Description : Test 2D copy with Pitch of destination greater than Pitch of source
Parameters  :
Assumptions :
Limitations :
Returns     : FALSE if success, TRUE otherwise
*******************************************************************************/
static BOOL Test2DCopyGreaterPitch(void *DeleteAddr_p, void *SourceAddr_p, void *DestAddr_p)
{
#if defined (STAVMEM_NO_COPY_FILL)
   STTBX_Print(("****  feature not supported !!!!!!   ****\n"));
   return(FALSE);
 #else
    U8 pattern1[3] = {0x11, 0x22, 0x33};
    U8 pattern2[3] = {0x44, 0x55, 0x66};
    U8 pattern3[3] = {0x77, 0x88, 0x99};
    U8 empty[1] = {0};
    U32 Err = 0, Tmp = 0, PatSize = 3;
    U32 SrcPitch = 16, DestPitch = 18;
    void * PhyDestAddr_p;

    /* Clear working memory area */
    STAVMEM_FillBlock1D(empty, 1, ((U8 *) DeleteAddr_p), 0x60);

    /* Write a pattern inside source and display it */
    STAVMEM_FillBlock1D(pattern1, PatSize, SourceAddr_p, PatSize);
    STAVMEM_FillBlock1D(pattern2, PatSize, (void *) ((U32 *) SourceAddr_p + 4), PatSize);
    STAVMEM_FillBlock1D(pattern3, PatSize, (void *) ((U32 *) SourceAddr_p + 8), PatSize);
    DumpMemory(((U8 *) SourceAddr_p) - 16, DUMP_5_LINES);
    STTBX_Print(("... Now copy with pitch dest > pitch source (%d > %d)\n", DestPitch, SrcPitch));

    /* Copy source to destination and display it */
    STAVMEM_CopyBlock2D(SourceAddr_p, PatSize, 3, SrcPitch, (void *) ((U32 *) DestAddr_p +16), 3); /*copy below*/
    STAVMEM_CopyBlock2D(SourceAddr_p, PatSize, 3, SrcPitch, DestAddr_p, DestPitch);  /*copy overlap*/
    DumpMemory(((U8 *) SourceAddr_p) - 16, DUMP_5_LINES);

    /* Verify byte by byte that destination is right */
    PhyDestAddr_p = IFV2CPU(DestAddr_p);
    for (Tmp = 0; Tmp < PatSize; Tmp++)
    {
        if (*((U8 *) (((U8 *) PhyDestAddr_p) + Tmp)) != pattern1[Tmp])
        {
            /* One error found ! */
            Err++;
        }
        if (*((U8 *) (((U8 *) PhyDestAddr_p) + DestPitch + Tmp)) != pattern2[Tmp])
        {
            /* One error found ! */
            Err++;
        }
        if (*((U8 *) (((U8 *) PhyDestAddr_p) + DestPitch + DestPitch + Tmp)) != pattern3[Tmp])
        {
            /* One error found ! */
            Err++;
        }
    }

    /* Display error(s) status */
    if (Err == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR !!! ****  (%d errors)\n\n", Err));
        return(TRUE);
    }
#endif /* (STAVMEM_NO_COPY_FILL)*/
}



/*******************************************************************************
Name        : Test2DCopySmallerPitch
Description : Test 2D copy with Pitch of destination smaller than Pitch of source
Parameters  :
Assumptions :
Limitations :
Returns     : FALSE if success, TRUE otherwise
*******************************************************************************/
static BOOL Test2DCopySmallerPitch(void *DeleteAddr_p, void *SourceAddr_p, void *DestAddr_p)
{
 #if defined (STAVMEM_NO_COPY_FILL)
   STTBX_Print(("****  feature not supported !!!!!!   ****\n"));
   return(FALSE);
 #else

    U8 pattern1[3] = {0x11, 0x22, 0x33};
    U8 pattern2[3] = {0x44, 0x55, 0x66};
    U8 pattern3[3] = {0x77, 0x88, 0x99};
    U8 empty[1] = {0};
    U32 Err = 0, Tmp = 0, PatSize = 3;
    U32 SrcPitch = 16, DestPitch = 14;
    void * PhyDestAddr_p;

    /* Clear working memory area */
    STAVMEM_FillBlock1D(empty, 1, ((U8 *) DeleteAddr_p), 0x60);

    /* Write a pattern inside source and display it */
    STAVMEM_FillBlock1D(pattern1, PatSize, SourceAddr_p, 3);
    STAVMEM_FillBlock1D(pattern2, PatSize, (void *) ((U32 *) SourceAddr_p + 4), PatSize);
    STAVMEM_FillBlock1D(pattern3, PatSize, (void *) ((U32 *) SourceAddr_p + 8), PatSize);
    DumpMemory(((U8 *) SourceAddr_p) - 16, DUMP_5_LINES);
    STTBX_Print(("... Now copy with pitch source > pitch dest (%d > %d)\n", SrcPitch, DestPitch));

    /* Copy source to destination and display it */
    STAVMEM_CopyBlock2D(SourceAddr_p, PatSize, 3, SrcPitch, (void *) ((U32 *) DestAddr_p + 16), 3); /*copy below*/
    STAVMEM_CopyBlock2D(SourceAddr_p, PatSize, 3, SrcPitch, DestAddr_p, DestPitch);  /*copy overlap*/
    DumpMemory(((U8 *) SourceAddr_p) - 16, DUMP_5_LINES);

    /* Verify byte by byte that destination is right */
    PhyDestAddr_p = IFV2CPU(DestAddr_p);
    for (Tmp = 0; Tmp < PatSize; Tmp++)
    {
        if (*(((U8 *) PhyDestAddr_p) + Tmp) != pattern1[Tmp])
        {
            /* One error found ! */
            Err++;
        }
        if (*(((U8 *) PhyDestAddr_p) + DestPitch + Tmp) != pattern2[Tmp])
        {
            /* One error found ! */
            Err++;
        }
        if (*(((U8 *) PhyDestAddr_p) + DestPitch + DestPitch + Tmp) != pattern3[Tmp])
        {
            /* One error found ! */
            Err++;
        }
    }

    /* Display error(s) status */
    if (Err == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR !!! ****  (%d errors)\n\n", Err));
        return(TRUE);
    }
#endif /*(STAVMEM_NO_COPY_FILL)*/
}



#if 0
/*******************************************************************************
Name        : Test2DCopyPitchLessThanSize
Description : Test 2D copy with pitches smaller than size
Parameters  :
Assumptions :
Limitations :
Returns     : FALSE if success, TRUE otherwise
*******************************************************************************/
static BOOL Test2DCopyPitchLessThanSize(void *DeleteAddr_p, void *SourceAddr_p, void *DestAddr_p)
{
    U8 pattern1[9] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
    U8 empty[1] = {0};
    U32 Err = 0, Tmp = 0, PatSize = 9;
    void * PhyDestAddr_p;

    STAVMEM_FillBlock1D(empty, 1, ((U8 *) DeleteAddr_p), 0x60);

    STAVMEM_FillBlock1D(pattern1, PatSize, SourceAddr_p, PatSize);
    DumpMemory(((U8 *) SourceAddr_p) - 16, DUMP_4_LINES);
    STTBX_Print(("... Now copy with pitches smaller than size\n"));

    STAVMEM_CopyBlock2D(SourceAddr_p, 3, 3, 2, DestAddr_p, 1);  /*copy overlap*/
    DumpMemory(((U8 *) SourceAddr_p) - 16, DUMP_4_LINES);
    STTBX_Print(("(This case of pitches is not supported, width is automatically limited.)\n"));

    /* Verify byte by byte that destination is right */
    PhyDestAddr_p = IFV2CPU(DestAddr_p);
    for (Tmp = 0; Tmp < 3; Tmp++)
    {
        if (*(((U8 *) PhyDestAddr_p) + Tmp) != pattern1[2 * Tmp])
        {
            /* One error found ! */
            Err++;
        }
    }

    /* Display error(s) status */
    if (Err == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR !!! ****  (%d errors)\n\n", Err));
        return(TRUE);
    }
}
#endif /* #if 0 */

/*******************************************************************************
Name        : Tests2DFill
Description : Test STAVMEM_FillBlock2D function
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL Tests2DFill( STTST_Parse_t *Parse_p, char *ResultSymbol_p )
{
    BOOL RetErr = FALSE;
    S32 basemem;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    STTST_EvaluateInteger("membase", &basemem, 16);

    /* Test Fill with destination pitch greater than source pitch */
    RetErr  = ((Test2DFillNormal((void *) (basemem), (void *) (basemem + 16 + 1), 1))) || (RetErr);
    RetErr |= ((Test2DFillNormal((void *) (basemem), (void *) (basemem + 16 + 1), 2))) || (RetErr);
    RetErr |= ((Test2DFillNormal((void *) (basemem), (void *) (basemem + 16 + 1), 3))) || (RetErr);
    RetErr |= ((Test2DFillNormal((void *) (basemem), (void *) (basemem + 16 + 1), 4))) || (RetErr);
    RetErr |= ((Test2DFillNormal((void *) (basemem), (void *) (basemem + 16 + 1), 5))) || (RetErr);
    RetErr |= ((Test2DFillNormal((void *) (basemem), (void *) (basemem + 16 + 2), 4))) || (RetErr);
    RetErr |= ((Test2DFillNormal((void *) (basemem), (void *) (basemem + 16 + 2), 7))) || (RetErr);
    RetErr |= ((Test2DFillNormal((void *) (basemem), (void *) (basemem + 16 + 3), 4))) || (RetErr);
    RetErr |= ((Test2DFillNormal((void *) (basemem), (void *) (basemem + 16 + 3), 7))) || (RetErr);

    if (!RetErr)
    {
        STTBX_Print(("**** No error in tests Fill 2D **** \n\n"));
    }
    else
    {
        STTBX_Print(("**** THERE ARE ERRORS IN TESTS OF FILL 2D **** \n\n\a"));
    }

    return(RetErr);
}


/*******************************************************************************
Name        : Test2DFillNormal
Description : Test 2D copy with pitches smaller than size
Parameters  :
Assumptions :
Limitations :
Returns     : FALSE if success, TRUE otherwise
*******************************************************************************/
static BOOL Test2DFillNormal(void *DeleteAddr_p, void *DestAddr_p, U32 PatternSize)
{
 #if defined (STAVMEM_NO_COPY_FILL)
   STTBX_Print(("****  feature not supported !!!!!!   ****\n"));
   return(FALSE);
 #else
    U8 pattern1[9] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
    U8 empty[1] = {0};
    U32 Err = 0, Tmp, Tmp2, PatSize = PatternSize;
    U32 DestHeight, DestWidth;
    void * PhyDestAddr_p;

    if (PatSize > 9)
    {
        PatSize = 9;
    }

    STAVMEM_FillBlock1D(empty, 1, ((U8 *) DeleteAddr_p), 0x60);

    STTBX_Print(("Filling with "));
    for (Tmp = 0; Tmp < PatSize; Tmp++)
    {
        STTBX_Print(("%#x ", pattern1[Tmp]));
    }
    STTBX_Print(("...\n"));

    /* Fill 14 * 3 rectangle */
    DestWidth = 14;
    DestHeight = 3;
    STAVMEM_FillBlock2D(pattern1, PatSize, 1, 1, DestAddr_p, DestWidth, DestHeight, 0x10);
    PhyDestAddr_p = IFV2CPU(DestAddr_p);
    for (Tmp = 0; Tmp < DestHeight; Tmp++)
    {
        for (Tmp2 = 0; Tmp2 < DestWidth; Tmp2++)
        {
            if (*(((U8 *) PhyDestAddr_p) + (Tmp * 0x10) + Tmp2) != pattern1[Tmp2 % PatSize])
            {
                /* One error found ! */
                Err++;
            }
        }
    }
    DumpMemory((U32 *) DestAddr_p, DUMP_4_LINES);

    /* Display error(s) status */
    if (Err == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR !!! ****  (%d errors)\n\n", Err));
        return(TRUE);
    }
#endif /* (STAVMEM_NO_COPY_FILL)|| (DVD_SECURED_CHIP)*/
}



/*******************************************************************************
******************** ALLOCATION TESTS ******************************************
*******************************************************************************/

/*******************************************************************************
Name        : StandardInit
Description : Initialize with a set of parameters values
Parameters  : STAVMEM Init parameters
Assumptions : DeviceType can be filled yet
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
static ST_ErrorCode_t StandardInit(STAVMEM_InitParams_t * const Init_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    Init_p->MaxPartitions              = TESTS_MAX_PARTITIONS;
    Init_p->MaxBlocks                  = 100;
    Init_p->MaxForbiddenRanges         = 5;
    Init_p->MaxForbiddenBorders        = 3;
    Init_p->CPUPartition_p             = SystemPartition_p;
    Init_p->NCachePartition_p          = NCachePartition_p;
    Init_p->MaxNumberOfMemoryMapRanges = TESTS_MAX_PARTITIONS; /* because each partition has a single range here */
    Init_p->BlockMoveDmaBaseAddr_p     = (void *) BM_BASE_ADDRESS;
    Init_p->CacheBaseAddr_p            = (void *) CACHE_BASE_ADDRESS;
    Init_p->VideoBaseAddr_p            = (void *) VIDEO_BASE_ADDRESS;
    Init_p->SDRAMBaseAddr_p            = (void *) MPEG_SDRAM_BASE_ADDRESS;
    Init_p->SDRAMSize                  = MPEG_SDRAM_SIZE;
    if (Init_p->DeviceType == STAVMEM_DEVICE_TYPE_VIRTUAL)
    {
        SharedMemoryVirtualMappingGiven_s.PhysicalAddressSeenFromCPU_p      = (void *) PHYSICAL_ADDRESS_SEEN_FROM_CPU;
        SharedMemoryVirtualMappingGiven_s.PhysicalAddressSeenFromDevice_p   = (void *) PHYSICAL_ADDRESS_SEEN_FROM_DEVICE;
        SharedMemoryVirtualMappingGiven_s.PhysicalAddressSeenFromDevice2_p  = (void *) PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2;
        SharedMemoryVirtualMappingGiven_s.VirtualBaseAddress_p              = (void *)VIRTUAL_BASE_ADDRESS;
        SharedMemoryVirtualMappingGiven_s.VirtualSize                       = VIRTUAL_SIZE;
        SharedMemoryVirtualMappingGiven_s.VirtualWindowOffset               = 0;
        SharedMemoryVirtualMappingGiven_s.VirtualWindowSize                 = VIRTUAL_WINDOW_SIZE;

        Init_p->SharedMemoryVirtualMapping_p = &SharedMemoryVirtualMappingGiven_s;
    }
    Init_p->NumberOfDCachedRanges     = (DCacheMapSize / sizeof(STBOOT_DCache_Area_t)) - 1;
    Init_p->DCachedRanges_p           = (STAVMEM_MemoryRange_t *) DCacheMap_p;
#ifdef STAVMEM_MEM_ACCESS_GPDMA
    strcpy(Init_p->GpdmaName, STGPDMA_DEVICE_NAME);
    Init_p->GpdmaName[ST_MAX_DEVICE_NAME - 1] = '\0';
#endif
    ErrorCode = STAVMEM_Init(STAVMEM_DEVICE_NAME, Init_p);

    return(ErrorCode);
} /* end of StandardInit() */

/*******************************************************************************
Name        : CreateStandardPartitions
Description : Initialize partitions with a set of parameters values
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if ok.
*******************************************************************************/
static BOOL CreateStandardPartitions(void)
{
    BOOL RetErr = FALSE;
    ST_ErrorCode_t ErrorCode;
    STAVMEM_CreatePartitionParams_t CreateParams;
    U32 i, AskedTotalFreeSize, AskedPartitionFreeSize, ExpectedTotalFreeSize, ExpectedPartitionFreeSize[TESTS_MAX_PARTITIONS+1];
#ifdef SLICE_MEMORY_IN_PARTITIONS
    U32 StartOffset, StopOffset;
    StartOffset = VIRTUAL_WINDOW_SIZE / (8 *TESTS_MAX_PARTITIONS); /* arbitrary, in order to prevent partitions from being contiguous */
    StopOffset  = VIRTUAL_WINDOW_SIZE / (16*TESTS_MAX_PARTITIONS); /* idem */
    for (i=0; i < TESTS_MAX_PARTITIONS; i++)
    {
        PartitionMapTable[i].StartAddr_p = (void *) (VIRTUAL_BASE_ADDRESS + VIRTUAL_WINDOW_SIZE * i/TESTS_MAX_PARTITIONS + StartOffset);
        PartitionMapTable[i].StopAddr_p  = (void *) (VIRTUAL_BASE_ADDRESS + VIRTUAL_WINDOW_SIZE * (i+1)/TESTS_MAX_PARTITIONS - 1 - StopOffset);
    }
#else
    PartitionMapTable[0].StartAddr_p = (void *) (PARTITION0_START);
    PartitionMapTable[0].StopAddr_p  = (void *) (PARTITION0_STOP);
    PartitionMapTable[1].StartAddr_p = (void *) (PARTITION1_START);
    PartitionMapTable[1].StopAddr_p  = (void *) (PARTITION1_STOP);
#if ( MAX_PARTITIONS == 3)
    PartitionMapTable[2].StartAddr_p = (void *) (PARTITION2_START);
    PartitionMapTable[2].StopAddr_p  = (void *) (PARTITION2_STOP);
#endif
#endif
    for (i=0; i < TESTS_MAX_PARTITIONS; i++)
    {
        ExpectedPartitionFreeSize[i]     = (((U32) PartitionMapTable[i].StopAddr_p) + 1 - ((U32) PartitionMapTable[i].StartAddr_p));
    }
    /* Create TESTS_MAX_PARTITIONS partitions */
    ExpectedTotalFreeSize = 0;
    for (i=0; (i < TESTS_MAX_PARTITIONS) && !RetErr; i++)
    {
        CreateParams.NumberOfPartitionRanges = (sizeof(PartitionMapTable[i]) / sizeof(STAVMEM_MemoryRange_t));;
        CreateParams.PartitionRanges_p       = &PartitionMapTable[i];
        ErrorCode = STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME, &CreateParams, &PartitionHandleTable[i]);

        if (ErrorCode == ST_NO_ERROR)
        {
            STTBX_Print(("Partition # %d created\n", i));
            AvmemPartitionHandle[i] = PartitionHandleTable[i];
        }
        else
        {
            STTBX_Print(("Error: Partition #%d cannot be created !\n", i));
            return(TRUE);
        }

        ExpectedTotalFreeSize += ExpectedPartitionFreeSize[i];
        ErrorCode = STAVMEM_GetPartitionFreeSize(PartitionHandleTable[i], &AskedPartitionFreeSize);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Print(("Error: STAVMEM_GetPartitionFreeSize for partition #%d fails !\n", i));
            RetErr = TRUE;
        }
        else if (AskedPartitionFreeSize != ExpectedPartitionFreeSize[i])
        {
            STTBX_Print(("Error: Partition #%d size returned (%d) different from expected (%d)!\n",
                            i, AskedPartitionFreeSize, ExpectedPartitionFreeSize[i] ));
            RetErr = TRUE;
        }

    }



    if (!RetErr)
    {
        STAVMEM_GetFreeSize(STAVMEM_DEVICE_NAME, &AskedTotalFreeSize);
        if (AskedTotalFreeSize != ExpectedTotalFreeSize)
        {
            STTBX_Print(("Error: Total free size returned (%d) different from expected (%d)!\n",
                            AskedTotalFreeSize, ExpectedTotalFreeSize ));
            RetErr = TRUE;
        }
    }

    return(RetErr);
} /* end of CreateStandardPartitions() */


/*******************************************************************************
Name        : TestsBlocksAlloc
Description : Test memory allocation functions
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL TestsBlocksAlloc( STTST_Parse_t *Parse_p, char *ResultSymbol_p )
{
    BOOL RetErr = FALSE;
    ST_ErrorCode_t Err;
    STAVMEM_InitParams_t InitAvmem;
    S32 basemem;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);

    STTST_EvaluateInteger("membase", &basemem, 16);
    InitAvmem.DeviceType = STAVMEM_DEVICE_TYPE_VIRTUAL;

#ifdef ST_WINCE_BLOCK_TEST
	Err = StandardInit(&InitAvmem);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Warning: STAVMEM already initialized !\n"));
    }
    else
	{
		RetErr = CreateStandardPartitions();
		if (RetErr) return(TRUE);
	}
#else   /* ST_WINCE_BLOCK_TEST*/

	Err = StandardInit(&InitAvmem);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Error: STAVMEM cannot be initialized !\n"));
        return(TRUE);
    }
    RetErr = CreateStandardPartitions();
    if (RetErr) return(TRUE);
#endif  /* ST_WINCE_BLOCK_TEST*/

#ifdef DEBUG_CODE
    /* Debug tests: particular cases */
    RetErr = ((TestBlockAllocDebug())) || (RetErr);
#endif

    /* Test randomly with lots of allocations */
    RetErr = ((TestBlockAllocRand())) || (RetErr);

    if (!RetErr)
    {
        STTBX_Print(("**** No error in tests of blocks allocations **** \n\n"));
    }
    else
    {
        STTBX_Print(("**** THERE ARE ERRORS IN TESTS OF BLOCKS ALLOCATIONS **** \n\n\a"));
    }

    /* Call term tests to terminate STAVMEM */
    AVMEM_Term();

    return(RetErr);
}


/*******************************************************************************
Name        : DisplayBlockInfo
Description : display information of a block (all parameters)
Parameters  : Block handle
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void DisplayBlockInfo(STAVMEM_BlockHandle_t BlockHandle)
{
    char AllocModeStr[10];
    STAVMEM_BlockParams_t BlockParams;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (BlockHandle != STAVMEM_INVALID_BLOCK_HANDLE)
    {
        /* Get block parameters */
        Err = STAVMEM_GetBlockParams(BlockHandle, &BlockParams);

        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error while getting block parameters !\n"));
        }

        switch (BlockParams.AllocMode)
        {
            case STAVMEM_ALLOC_MODE_BOTTOM_TOP:
                strcpy(AllocModeStr, "BottomTop");
                break;

            case STAVMEM_ALLOC_MODE_TOP_BOTTOM:
                strcpy(AllocModeStr, "TopBottom");
                break;

            case STAVMEM_ALLOC_MODE_RESERVED:
                strcpy(AllocModeStr, "Reserved ");
                break;

            default:
                strcpy(AllocModeStr, "-Invalid-");
                break;
        }

        STTBX_Print(("Memory block in '%s' mode, %#x byte%c "
        "from %#010x to %#010x", AllocModeStr, BlockParams.Size, (BlockParams.Size>1)?'s':' ', ((U32) BlockParams.StartAddr_p), ((U32) BlockParams.StartAddr_p) + BlockParams.Size - 1));
        STTBX_Print((" (%d aligned)", BlockParams.Alignment));
        STTBX_Print(("\n"));
    }
    else
    {
        STTBX_Print(("Error: memory block NOT ALLOCATED !\n"));
    }
}


#ifdef DEBUG_CODE
/*******************************************************************************
Name        : DisplayAllBlockInfo
Description : display information of all blocks of the list of blocks of current partition
              and check consistency of list and free size
Parameters  : nothing
Assumptions :
Limitations :
Returns     : TRUE on success, FALSE otherwise
*******************************************************************************/
static BOOL DisplayAllBlocksInfo(const BOOL NotOnlyErrors)
{
    void *Addr_p;
    U32 i, Size;
    U32 CalculatedFreeSize = 0, AskedFreeSize;
    char AllocModeStr[10];
    MemBlockDesc_t *Cur_p;
    stavmem_Partition_t *Partition_p;
    BOOL RetErr = TRUE;

    for (i=0; i < TESTS_MAX_PARTITIONS; i++)
    {
        Partition_p = (stavmem_Partition_t *) PartitionHandleTable[i];
        if (Partition_p->Validity != AVMEM_VALID_PARTITION)
        {
            return(ST_ERROR_INVALID_HANDLE);
        }

        if (NotOnlyErrors)
        {
            STTBX_Print(("All memory blocks from bottom:\n"));
        }

        /* Display info about all blocks until the last one */
        Cur_p = Partition_p->VeryBottomBlock_p;
        while (Cur_p != NULL)
        {
            Size = Cur_p->Size;
            Addr_p = Cur_p->StartAddr_p;
            switch (Cur_p->AllocMode)
            {
                case STAVMEM_ALLOC_MODE_BOTTOM_TOP:
                    strcpy(AllocModeStr, "BottomTop");
                    break;

                case STAVMEM_ALLOC_MODE_TOP_BOTTOM:
                    strcpy(AllocModeStr, "TopBottom");
                    break;

                case STAVMEM_ALLOC_MODE_RESERVED:
                    strcpy(AllocModeStr, "RESERVED!");
                    break;

                case STAVMEM_ALLOC_MODE_FORBIDDEN:
                    strcpy(AllocModeStr, "FORBIDDEN");
                    break;

                default:
                    strcpy(AllocModeStr, "-Invalid-");
                    RetErr = FALSE;
                    break;
            }
            if ((RetErr) && (!Cur_p->IsUsed))
            {
                strcpy(AllocModeStr, "--FREE!--");
                CalculatedFreeSize += Size;
            }

            if (NotOnlyErrors)
            {
                STTBX_Print(("%#010x-%#010x : %s (%#x byte%c) \n", ((U32) Addr_p), ((U32) Addr_p) + Size - 1, AllocModeStr, Size, (Size>1)?'s':'.'));
            }

            /* If no error, get ready for next block, else abort */
            if ((Cur_p->BlockAbove_p != Cur_p) && ((Cur_p->BlockAbove_p == NULL) || (Cur_p->BlockAbove_p->BlockBelow_p == Cur_p)))
            {
                /* Block doesn't point to itself and links are consistent */
                /* Test if addresses are contiguous.
                Not always the case in the future maybe, but it is the case now... */
                if ((Cur_p->BlockAbove_p != NULL) && (((U32) Cur_p->BlockAbove_p->StartAddr_p) != (((U32) Cur_p->StartAddr_p) + Cur_p->Size)))
                {
                    /* Error case */
                    BlockAndBip();
                }
                Cur_p = Cur_p->BlockAbove_p;
            }
            else
            {
                Cur_p = NULL;
                STTBX_Print(("Error: memory block list is CORRUPTED !!!\n"));
                BlockAndBip();
                RetErr = FALSE;
            }
        }
    }

    /* Check total free size */
    STAVMEM_GetFreeSize(STAVMEM_DEVICE_NAME, &AskedFreeSize);
    if (AskedFreeSize == CalculatedFreeSize)
    {
        if (NotOnlyErrors)
        {
            STTBX_Print(("with %#x bytes free.\n", CalculatedFreeSize));
        }
    }
    else
    {
        STTBX_Print(("Error in free size: %#x / %#x bytes.\n", CalculatedFreeSize, AskedFreeSize));
        BlockAndBip();
        RetErr = FALSE;
    }


    return(RetErr);
}
#endif /* #ifdef DEBUG_CODE */


/*******************************************************************************
Name        : VerifyAllocBlock
Description : Verify that block matches the allocations parameters
Parameters  : block handle, allocation parameters
Assumptions : pointer is not NULL
Limitations :
Returns     : number of errors found
*******************************************************************************/
static U32 VerifyAllocBlock(STAVMEM_BlockHandle_t BlockHandle, STAVMEM_AllocBlockParams_t *AllocParams_p)
{
#ifdef DEBUG_CODE
    MemBlockDesc_t *Cur_p;
#endif
    STAVMEM_BlockParams_t BlockParams;
    U32 Errors = 0, Tmp = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (BlockHandle != STAVMEM_INVALID_BLOCK_HANDLE)
    {
        /* Get block parameters */
        Err = STAVMEM_GetBlockParams(BlockHandle, &BlockParams);

        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error while getting block parameters !\n"));
#ifdef DEBUG_CODE
            BlockAndBip();
#endif
            Errors++;
        }

#ifdef DEBUG_CODE
        Cur_p = (MemBlockDesc_t *) BlockHandle;

        /* Obvious errors of pointers */
        if ((Cur_p == Cur_p->BlockBelow_p) || (Cur_p == Cur_p->BlockAbove_p))
        {
            STTBX_Print(("Error: memory block points to itself !\n"));
            BlockAndBip();
            Errors++;
        }
        if ((Cur_p->BlockBelow_p != NULL) && (Cur_p->BlockBelow_p->BlockAbove_p != Cur_p))
        {
            STTBX_Print(("Error: memory block has bad links below !\n"));
            BlockAndBip();
            Errors++;
        }
        if ((Cur_p->BlockAbove_p != NULL) && (Cur_p->BlockAbove_p->BlockBelow_p != Cur_p))
        {
            STTBX_Print(("Error: memory block has bad links above !\n"));
            BlockAndBip();
            Errors++;
        }

        /* Error if sizes don't correspond */
        if (Cur_p->Size < BlockParams.Size)
        {
            STTBX_Print(("Error: memory block size parameters are corrupted !\n"));
            BlockAndBip();
            Errors++;
        }
#endif /* #ifdef DEBUG_CODE */

        /* Error if bad alignment */
        if ((AllocParams_p->Alignment != 1) && ((((U32) BlockParams.StartAddr_p) & (AllocParams_p->Alignment - 1)) != 0))
        {
            STTBX_Print(("Error: memory block badly aligned !\n"));
#ifdef DEBUG_CODE
            BlockAndBip();
#endif
            Errors++;
        }

        /* Error if bad alloc mode */
        if (BlockParams.AllocMode != AllocParams_p->AllocMode)
        {
            STTBX_Print(("Error: memory block has bad alloc mode !\n"));
#ifdef DEBUG_CODE
            BlockAndBip();
#endif
            Errors++;
        }

        /* Error if bad size (size can be greater because of alignment of the size) */
        if (BlockParams.Size != AllocParams_p->Size)
        {
            STTBX_Print(("Error: memory block size is wrong !\n"));
#ifdef DEBUG_CODE
            BlockAndBip();
#endif
            Errors++;
        }

        /* Error if forbidden zone entered */
        for (Tmp = 0; Tmp < AllocParams_p->NumberOfForbiddenRanges; Tmp++)
        {
            if ((((U32) BlockParams.StartAddr_p) <= ((U32) AllocParams_p->ForbiddenRangeArray_p[Tmp].StopAddr_p)) && (((U32) BlockParams.StartAddr_p) + BlockParams.Size - 1 >= ((U32) AllocParams_p->ForbiddenRangeArray_p[Tmp].StartAddr_p)))
            {
                /* Error: range overlaps */
                STTBX_Print(("Error: memory block is overlaping forbidden range !\n"));
#ifdef DEBUG_CODE
                BlockAndBip();
#endif
                Errors++;
            }
        }
        for (Tmp = 0; Tmp < AllocParams_p->NumberOfForbiddenBorders; Tmp++)
        {
            if ((((U32) AllocParams_p->ForbiddenBorderArray_p[Tmp]) > ((U32) BlockParams.StartAddr_p)) && (((U32) AllocParams_p->ForbiddenBorderArray_p[Tmp]) <= ((U32) BlockParams.StartAddr_p) + BlockParams.Size - 1))
            {
                STTBX_Print(("Error: memory block is crossing forbidden border !\n"));
#ifdef DEBUG_CODE
                BlockAndBip();
#endif
                Errors++;
            }
        }
    }
    else
    {
        STTBX_Print(("Error: memory block NOT ALLOCATED !\n"));
#ifdef DEBUG_CODE
        BlockAndBip();
#endif
        Errors++;
    }

    /* Display error(s) status */
    if (Errors != 0)
    {
        STTBX_Print(("%d errors in memory block allocation !\n", Errors));
    }

    return(Errors);
}



/*******************************************************************************
Name        : VerifyReAllocBlock
Description : Verify that block matches the re-allocations parameters
Parameters  : block handle, re-allocation parameters
Assumptions : pointer is not NULL
Limitations :
Returns     : number of errors found
*******************************************************************************/
static U32 VerifyReAllocBlock(STAVMEM_BlockHandle_t BlockHandle, STAVMEM_ReAllocBlockParams_t *ReAllocParams_p)
{
#ifdef DEBUG_CODE
    MemBlockDesc_t *Cur_p;
#endif
    U32 Errors = 0, Tmp;
    STAVMEM_BlockParams_t BlockParams;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (BlockHandle != STAVMEM_INVALID_BLOCK_HANDLE)
    {
        /* Get block parameters */
        Err = STAVMEM_GetBlockParams(BlockHandle, &BlockParams);

        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error while getting block parameters !\n"));
#ifdef DEBUG_CODE
            BlockAndBip();
#endif
            Errors++;
        }

#ifdef DEBUG_CODE
        Cur_p = (MemBlockDesc_t *) BlockHandle;

        /* Error if sizes don't correspond */
        if (Cur_p->Size < BlockParams.Size)
        {
            STTBX_Print(("Error: memory block size parameters are corrupted !\n"));
            BlockAndBip();
            Errors++;
        }
#endif

        /* Error if bad size (size can be greater because of alignment of the size) */
        if (BlockParams.Size != ReAllocParams_p->Size)
        {
            STTBX_Print(("Error: memory block size is wrong !\n"));
#ifdef DEBUG_CODE
            BlockAndBip();
#endif
            Errors++;
        }

        /* Error if forbidden zone entered */
        for (Tmp = 0; Tmp < ReAllocParams_p->NumberOfForbiddenRanges; Tmp++)
        {
            if ((((U32) BlockParams.StartAddr_p) <= ((U32) ReAllocParams_p->ForbiddenRangeArray_p[Tmp].StopAddr_p)) && (((U32) BlockParams.StartAddr_p) + BlockParams.Size - 1 >= ((U32) ReAllocParams_p->ForbiddenRangeArray_p[Tmp].StartAddr_p)))
            {
                /* Error: range overlaps */
                STTBX_Print(("Error: memory block is overlaping forbidden range !\n"));
#ifdef DEBUG_CODE
                BlockAndBip();
#endif
                Errors++;
            }
        }
        for (Tmp = 0; Tmp < ReAllocParams_p->NumberOfForbiddenBorders; Tmp++)
        {
            if ((((U32) ReAllocParams_p->ForbiddenBorderArray_p[Tmp]) > ((U32) BlockParams.StartAddr_p)) && (((U32) ReAllocParams_p->ForbiddenBorderArray_p[Tmp]) <= ((U32) BlockParams.StartAddr_p) + BlockParams.Size - 1))
            {
                STTBX_Print(("Error: memory block is crossing forbidden border !\n"));
#ifdef DEBUG_CODE
                BlockAndBip();
#endif
                Errors++;
            }
        }
    }
    else
    {
        STTBX_Print(("Error: memory block NOT RE-ALLOCATED !\n"));
#ifdef DEBUG_CODE
        BlockAndBip();
#endif
        Errors++;
    }

    /* Display error(s) status */
    if (Errors != 0)
    {
        STTBX_Print(("%d errors in memory block re-allocation !\n", Errors));
    }

    return(Errors);
}




/*******************************************************************************
Name        : AllocAndVerify
Description : allocate a block and verify it, as well as the list of blocks
Parameters  : allocation parameters, block handle, flag to display results or not
Assumptions :
Limitations :
Returns     : number of errors found
*******************************************************************************/
static U32 AllocAndVerify(STAVMEM_AllocBlockParams_t *AllocBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p, const BOOL DisplayResults)
{
    U32 Errors = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    UNUSED_PARAMETER(DisplayResults);

    Err = STAVMEM_AllocBlock(AllocBlockParams_p, BlockHandle_p);
    if (Err != ST_NO_ERROR)
    {
        switch (Err)
        {
            case STAVMEM_ERROR_PARTITION_FULL:
                STTBX_Print(("Allocation skipped because of lack of large enough memory.\n"));
                break;

            case STAVMEM_ERROR_MAX_BLOCKS:
                STTBX_Print(("STAVMEM_AllocBlock failed because no block is available !\n"));
#ifdef DEBUG_CODE
                BlockAndBip();
#endif
                Errors++;
                break;

            default :
                STTBX_Print(("STAVMEM_AllocBlock returned error code -%d- !.\n", Err));
#ifdef DEBUG_CODE
                BlockAndBip();
#endif
                Errors++;
                break;
        }
    }
    else
    {
        Errors += VerifyAllocBlock(*BlockHandle_p, AllocBlockParams_p);
        DisplayBlockInfo(*BlockHandle_p);
#ifdef DEBUG_CODE
        if (!DisplayAllBlocksInfo(DisplayResults)) Errors++;
#endif
    }

    return(Errors);
}


/*******************************************************************************
Name        : ReAllocAndVerify
Description : re-allocate a block and verify it, as well as the list of blocks
Parameters  : re-allocation parameters, block handle, flag to display results or not
Assumptions :
Limitations :
Returns     : number of errors found
*******************************************************************************/
static U32 ReAllocAndVerify(STAVMEM_ReAllocBlockParams_t *ReAllocBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p, const BOOL DisplayResults)
{
    U32 Errors = 0;
    U8 Probe;
    BOOL ShouldBePreserved; /* data can't be preserved if virtual outside window ! */
    void *ProbeAddr_p;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    UNUSED_PARAMETER(DisplayResults);

    if (ReAllocBlockParams_p->PreserveData)
    {
        STAVMEM_GetBlockAddress(*BlockHandle_p, &ProbeAddr_p);
        FlushDCache;
        Probe = *((U8 *) IFV2CPU(ProbeAddr_p));
        ShouldBePreserved = ISINVW(ProbeAddr_p,ReAllocBlockParams_p->Size);
    }
    Err = STAVMEM_ReAllocBlock(ReAllocBlockParams_p, BlockHandle_p);
    if (Err != ST_NO_ERROR)
    {
        switch (Err)
        {
            case STAVMEM_ERROR_PARTITION_FULL:
                STTBX_Print(("Re-allocation skipped because of lack of large enough memory.\n"));
                break;

            case STAVMEM_ERROR_BLOCK_IN_FORBIDDEN_ZONE:
                STTBX_Print(("Re-allocation skipped because block is in forbidden zone.\n"));
                break;

            case STAVMEM_ERROR_MAX_BLOCKS:
                STTBX_Print(("STAVMEM_ReAllocBlock failed because no block is available !\n"));
#ifdef DEBUG_CODE
                BlockAndBip();
#endif
                Errors++;
                break;

            default :
                STTBX_Print(("STAVMEM_ReAllocBlock returned error code -%d- !\n", Err));
#ifdef DEBUG_CODE
                BlockAndBip();
#endif
                Errors++;
                break;
        }
    }
    else
    {
        FlushDCache;
        STAVMEM_GetBlockAddress(*BlockHandle_p, &ProbeAddr_p);
        if (    (ReAllocBlockParams_p->PreserveData)
             && ShouldBePreserved
             && ISINVW(ProbeAddr_p,ReAllocBlockParams_p->Size)
             && (Probe != *(U8 *) IFV2CPU(ProbeAddr_p))
           )
        {
            STTBX_Print(("Re-allocations failed to preserve data (got %d for %d) !\n", *(U8 *) IFV2CPU(ProbeAddr_p), Probe));
#ifdef DEBUG_CODE
            BlockAndBip();
#endif
            Errors++;
        }
        Errors += VerifyReAllocBlock(*BlockHandle_p, ReAllocBlockParams_p);
        DisplayBlockInfo(*BlockHandle_p);
#ifdef DEBUG_CODE
        if (!DisplayAllBlocksInfo(DisplayResults)) Errors++;
#endif
    }

    return(Errors);
}

/*******************************************************************************
Name        : FreeAndVerify
Description : free a block and verify it, as well as the list of blocks
Parameters  : block handle, flag to display results or not
Assumptions :
Limitations :
Returns     : number of errors found
*******************************************************************************/
static U32 FreeAndVerify(STAVMEM_FreeBlockParams_t *FreeBlockParams_p, STAVMEM_BlockHandle_t *BlockHandle_p, const BOOL DisplayResults)
{
    U32 Errors = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    UNUSED_PARAMETER(DisplayResults);

    Err = STAVMEM_FreeBlock(FreeBlockParams_p, BlockHandle_p);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("STAVMEM_Free returned error code -%d- !.\n", Err));
        Errors++;
    }

#ifdef DEBUG_CODE
    if (!DisplayAllBlocksInfo(DisplayResults)) Errors++;
#endif

    return(Errors);
}


#ifdef DEBUG_CODE
/*******************************************************************************
Name        : TestBlockAllocDebug
Description : check particular cases of memory allocations for debug
Parameters  : nothing
Assumptions :
Limitations :
Returns     : FALSE if success, TRUE otherwise
*******************************************************************************/
static BOOL TestBlockAllocDebug(void)
{
    U32 Errors = 0;
    STAVMEM_BlockHandle_t Handle, H2, H3, KeepIt;
    STAVMEM_AllocBlockParams_t AllocParams;
    STAVMEM_ReAllocBlockParams_t ReAllocParams;
    STAVMEM_FreeBlockParams_t FreeParams;
    STAVMEM_MemoryRange_t ForbidRange[5];
    void *ForbidBorder[3];

    STTBX_Print(("\n**** TESTING A SEQUENCE OF ALLOC/REALLOC/FREE WITH A LOT OF CASES ****\n\n"));

    if (!DisplayAllBlocksInfo(TRUE)) Errors++;

    AllocParams.Alignment = 1;
    AllocParams.Size = SDRAM_BANK_SIZE - 1;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    ForbidRange[0].StartAddr_p = (void *) 0xc0001000;
    ForbidRange[0].StopAddr_p  = (void *) 0xc0001999;
    ForbidRange[1].StartAddr_p = (void *) 0xc0001000;
    ForbidRange[1].StopAddr_p  = (void *) 0xc0001999;
    ForbidRange[2].StartAddr_p = (void *) 0xc0020000;
    ForbidRange[2].StopAddr_p  = (void *) 0xc0020051;
    ForbidRange[3].StartAddr_p = (void *) 0xc003fffe;
    ForbidRange[3].StopAddr_p  = (void *) 0xc0040001;
    ForbidRange[4].StartAddr_p = (void *) 0x00050000;
    ForbidRange[4].StopAddr_p  = (void *) 0x00051000;
    ForbidBorder[0] = (void *) 0xc0200000;
    ForbidBorder[1] = (void *) 0xc0200001;
    ForbidBorder[2] = (void *) 0xc01fffff;
    AllocParams.NumberOfForbiddenRanges = 5;
    AllocParams.ForbiddenRangeArray_p = &ForbidRange[0];
    AllocParams.NumberOfForbiddenBorders = 3;
    AllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
    ReAllocParams.NumberOfForbiddenRanges = 5;
    ReAllocParams.ForbiddenRangeArray_p = &ForbidRange[0];
    ReAllocParams.NumberOfForbiddenBorders = 3;
    ReAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
    ReAllocParams.PreserveData = FALSE;
#if !(defined (STAVMEM_NO_COPY_FILL)|| defined (DVD_SECURED_CHIP))
    ReAllocParams.PreserveDataFunction = (STAVMEM_PreserveDataFunction_t) STAVMEM_CopyBlock1D;
#else
    ReAllocParams.PreserveDataFunction = Inv_memcpy;
#endif  /*STAVMEM_NO_COPY_FILL*/
    AllocParams.PartitionHandle = PartitionHandleTable[0];
    ReAllocParams.PartitionHandle = PartitionHandleTable[0];
    FreeParams.PartitionHandle = PartitionHandleTable[0];


    AllocAndVerify(&AllocParams, &Handle, TRUE);

    ReAllocParams.Size = SDRAM_BANK_SIZE - 3;
    ReAllocAndVerify(&ReAllocParams, &Handle, TRUE);

    AllocParams.Size = SDRAM_BANK_SIZE_2_TENS;
    AllocAndVerify(&AllocParams, &H2, TRUE);

    ReAllocParams.Size = SDRAM_BANK_SIZE_3_TENS;
    ReAllocAndVerify(&ReAllocParams, &H2, TRUE);

    ReAllocParams.Size = SDRAM_BANK_SIZE_3_TENS - 13;
    ReAllocAndVerify(&ReAllocParams, &H2, TRUE);

    AllocParams.Size = 2;
    AllocParams.Alignment = 2;
    AllocAndVerify(&AllocParams, &KeepIt, TRUE);

    ReAllocParams.Size = 0x14;
    ReAllocAndVerify(&ReAllocParams, &KeepIt, TRUE);


    FreeAndVerify(&FreeParams, &Handle, TRUE);

    FreeAndVerify(&FreeParams, &H2, TRUE);


    AllocParams.Alignment = 1;
    AllocParams.Size = SDRAM_BANK_SIZE_8_TENS;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocAndVerify(&AllocParams, &Handle, TRUE);

    AllocParams.Alignment = 30;
    AllocParams.Size = SDRAM_BANK_SIZE_1_TENS + 5;
    AllocAndVerify(&AllocParams, &H2, TRUE);

    AllocAndVerify(&AllocParams, &H3, TRUE);


    FreeAndVerify(&FreeParams, &Handle, TRUE);

    FreeAndVerify(&FreeParams, &H2, TRUE);

    FreeAndVerify(&FreeParams, &H3, TRUE);

    FreeAndVerify(&FreeParams, &KeepIt, TRUE);


    /* Display error(s) status */
    if (Errors == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR !!! ****  (%d errors)\n\n", Errors));
        return(TRUE);
    }
}
#endif /* #ifdef DEBUG_CODE */


/*******************************************************************************
Name        : TestBlockAllocRand
Description : test lot's of memory allocations/re-allocations/freeings randomly
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if success, TRUE otherwise
*******************************************************************************/
static BOOL TestBlockAllocRand()
{
    U32 Errors = 0;
    U32 TotalAllocated = 0;
    U32 AllocCounter, ReAllocCounter, FreeCounter, APISkipCounter;
    U32 PartitionBaseAddr, PartitionSize, Largest;
    STAVMEM_BlockHandle_t H[RANDOM_TESTS_MAX_BLOCKS];
    U8 PartitionIndex[RANDOM_TESTS_MAX_BLOCKS];
    STAVMEM_AllocBlockParams_t AllocParams;
    STAVMEM_ReAllocBlockParams_t ReAllocParams;
    STAVMEM_FreeBlockParams_t FreeParams;
    STAVMEM_MemoryRange_t ForbidRange[3];
    void *ForbidBorder[3];
    char dummychar = 0;
    U32 AlignmentsTab_p[8] = {2, 16, 64, 128, 256, 1024, 4096, 65536};
    U32 Action = 16; /* Start with displaying help and filling memory with random numbers*/
    U32 i = 0, j, offset, HIndex, CurrentPartitionIndex;
#ifdef ARCHITECTURE_ST200
    U32 k;
#endif
    AllocCounter = 0;
    ReAllocCounter = 0;
    FreeCounter = 0;
    APISkipCounter = 0;

    /* Set random starting point (ANSI) */
/*    srand(0); */
    srand((unsigned int)time_now());

    /* Set all blocks as not valid (available) */
    for (j = 0; j < RANDOM_TESTS_MAX_BLOCKS; j++)
    {
        H[j] = STAVMEM_INVALID_BLOCK_HANDLE;
    }

    /* Set forever unchanged parameters */
    AllocParams.ForbiddenRangeArray_p = &ForbidRange[0];
    AllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
    ReAllocParams.ForbiddenRangeArray_p = &ForbidRange[0];
    ReAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
    AllocParams.PartitionHandle = PartitionHandleTable[0];
    ReAllocParams.PartitionHandle = PartitionHandleTable[0];
    FreeParams.PartitionHandle = PartitionHandleTable[0];
   #if !(defined (STAVMEM_NO_COPY_FILL)|| defined (DVD_SECURED_CHIP))
       ReAllocParams.PreserveDataFunction = (STAVMEM_PreserveDataFunction_t) STAVMEM_CopyBlock1D;
/*    ReAllocParams.PreserveDataFunction = (STAVMEM_PreserveDataFunction_t) Inv_memmove;*/
   #else
    ReAllocParams.PreserveDataFunction = Inv_memcpy;
   #endif  /*STAVMEM_NO_COPY_FILL*/


#ifdef ARCHITECTURE_ST200
    for(k=0;k<20;k++)
    {
#else
    /* Actions loop: stop only at first error or when a key is pressed */
    while ((Errors == 0) && (dummychar == 0))
    {
        /* Test if a key was pressed on the keyboard */
        if ((Action != 16) && (STTBX_InputPollChar(&dummychar)))
        {
            /* A key was pressed: test this key */
            switch (dummychar)
            {
                case 'p' :
                case 'P' :
                    /* Test if tests are paused */
                    if (Action != 14)
                    {
                        /* Tests are not paused */
                        Action = 14;    /* Pause the test: won't choose any action */
                    }
                    else
                    {
                        /* Tests are paused */
                        Action = 0;     /* Resume the test: will choose an action */
                    }
                    dummychar = 0;  /* Don't stop the tests */
                    break;

#ifdef DEBUG_CODE
                case 'L' :
                case 'l' :
                    /* Display List of blocks */
                    DisplayAllBlocksInfo(TRUE);
                    dummychar = 0;  /* Don't stop the tests */
                    break;
#endif

                case 'a' :
                case 'A' :
                    Action = 1;     /* Force allocate */
                    dummychar = 0;  /* Don't stop the tests */
                    break;

                case 'r' :
                case 'R' :
                    Action = 2;     /* Force re-allocate */
                    dummychar = 0;  /* Don't stop the tests */
                    break;

                case 'f' :
                case 'F' :
                    Action = 3;     /* Force free */
                    dummychar = 0;  /* Don't stop the tests */
                    break;

                case 's' :
                case 'S' :
                    /* dummychar is not 0, so program will quit actions loop */
                    break;

                default :
                    Action = 15;    /* Display help */
                    dummychar = 0;  /* Don't stop the tests */
                    break;
            }
        }
#endif
        /* Choose an action, but not if tests are paused */
        if (Action == 0)
        {
            Action = GetRandomBetween1And(3);
        }

        /* Perform same action between 1 and 10 times... */
        for (i = GetRandomBetween1And(10); i >= 1; i--)
        {
            switch (Action)
            {
/******************************************************************************/
                case 1: /* Allocation */
                    STTBX_Print(("(%d) ", TotalAllocated));
                    CurrentPartitionIndex       = GetRandomBetween1And(TESTS_MAX_PARTITIONS)-1;
                    AllocParams.PartitionHandle = PartitionHandleTable[CurrentPartitionIndex];

                    /* Random or 1 alignment */
                    if (GetRandomBetween1And(2) == 1)
                    {
                        AllocParams.Alignment = 1;
                    }
                    else
                    {
                        AllocParams.Alignment = AlignmentsTab_p[GetRandomBetween1And(8) - 1];
                    }
                    /* TOP_BOTTOM or BOTTOM_TOP */
                    if (GetRandomBetween1And(2) == 1)
                    {
                        AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
                    }
                    else
                    {
                        AllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
                    }
                    /* Forbidden zones or not */
                    if (GetRandomBetween1And(2) == 1)
                    {
                        /* Random forbidden ranges */
                        PartitionBaseAddr = (U32)PartitionMapTable[CurrentPartitionIndex].StartAddr_p;
                        PartitionSize     = (U32)PartitionMapTable[CurrentPartitionIndex].StopAddr_p - PartitionBaseAddr + 1;

                        ForbidRange[0].StartAddr_p = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidRange[0].StopAddr_p  = (void *) (((U32) ForbidRange[0].StartAddr_p) + GetRandomBetween1And(PartitionSize / 4));
                        ForbidRange[1].StartAddr_p = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidRange[1].StopAddr_p  = (void *) (((U32) ForbidRange[1].StartAddr_p) + GetRandomBetween1And(PartitionSize / 128));
                        ForbidRange[2].StartAddr_p = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidRange[2].StopAddr_p  = (void *) (((U32) ForbidRange[2].StartAddr_p) + GetRandomBetween1And(PartitionSize / 4));
                        ForbidBorder[0] = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidBorder[1] = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidBorder[2] = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        AllocParams.NumberOfForbiddenRanges = 3;
                        AllocParams.NumberOfForbiddenBorders = 3;
                    }
                    else
                    {
                        AllocParams.NumberOfForbiddenRanges = 0;
                        AllocParams.NumberOfForbiddenBorders = 0;
                    }
                    /* Random size */
                    AllocParams.Size = GetRandomBetween1And(PartitionSize / 10);
                    STAVMEM_GetLargestFreeBlockSize(PartitionHandleTable[CurrentPartitionIndex], AllocParams.AllocMode, &Largest);
                    if (AllocParams.Size > Largest)
                    {
                        AllocParams.Size = Largest;
                        AllocParams.Alignment = 1; /* be sure to take everything ! */
                    }

                    /* Find an available block index */
                    j = 0;
                    offset = GetRandomBetween1And(RANDOM_TESTS_MAX_BLOCKS) - 1;
                    while ((H[(j + offset) % RANDOM_TESTS_MAX_BLOCKS] != STAVMEM_INVALID_BLOCK_HANDLE) && (j < RANDOM_TESTS_MAX_BLOCKS))
                    {
                        j++;
                    }
                    if (j >= RANDOM_TESTS_MAX_BLOCKS)
                    {
                        /* Could not find free index */
                        STTBX_Print(("Allocation skipped because no more block available in partition %d.\n", CurrentPartitionIndex));
                        APISkipCounter++;
                        /* Don't try any more for this time... */
                        i = 1;
                    }
                    else if (AllocParams.Size == 0)
                    {
                        /* Could not find free index */
                        STTBX_Print(("Allocation skipped because no more free memory in partition %d.\n", CurrentPartitionIndex));
                        APISkipCounter++;
                        /* Don't try any more for this time... */
                        i = 1;
                    }
                    else
                    {
                        /* Index to not valid block (available) found */
                        STTBX_Print(("Allocating %#x (max is %#x), %d aligned, in partition %d...\n",
                                    AllocParams.Size, Largest, AllocParams.Alignment, CurrentPartitionIndex));
                        HIndex = (j + offset) % RANDOM_TESTS_MAX_BLOCKS;
                        Errors += AllocAndVerify(&AllocParams, &H[HIndex], FALSE);
                        PartitionIndex[HIndex] = CurrentPartitionIndex;
                        if (H[HIndex] != STAVMEM_INVALID_BLOCK_HANDLE)
                        {
                            TotalAllocated++;
                            AllocCounter++;
                        }
                    }
                    break;

/******************************************************************************/
                case 2: /* ReAllocation */
                    STTBX_Print(("(%d) ", TotalAllocated));
                    /* Find a used block index */
                    j = 0;
                    offset = GetRandomBetween1And(RANDOM_TESTS_MAX_BLOCKS) - 1;
                    while ((H[(j + offset) % RANDOM_TESTS_MAX_BLOCKS] == STAVMEM_INVALID_BLOCK_HANDLE) && (j < RANDOM_TESTS_MAX_BLOCKS))
                    {
                        j++;
                    }
                    HIndex = (j + offset) % RANDOM_TESTS_MAX_BLOCKS;

                    /* Forbidden zones or not */
                    if (GetRandomBetween1And(2) == 1)
                    {
                        PartitionBaseAddr = (U32)PartitionMapTable[PartitionIndex[HIndex]].StartAddr_p;
                        PartitionSize     = (U32)PartitionMapTable[PartitionIndex[HIndex]].StopAddr_p - PartitionBaseAddr + 1;

                        /* Random forbidden zones: 2 with random size, 1 other with 0 size */
                        ForbidRange[0].StartAddr_p = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidRange[0].StopAddr_p  = (void *) (((U32) ForbidRange[0].StartAddr_p) + GetRandomBetween1And(PartitionSize / 4));
                        ForbidRange[1].StartAddr_p = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidRange[1].StopAddr_p  = (void *) (((U32) ForbidRange[1].StartAddr_p) + GetRandomBetween1And(PartitionSize / 128));
                        ForbidRange[2].StartAddr_p = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidRange[2].StopAddr_p  = (void *) (((U32) ForbidRange[2].StartAddr_p) + GetRandomBetween1And(PartitionSize / 4));
                        ForbidBorder[0] = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidBorder[1] = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ForbidBorder[2] = (void *) (PartitionBaseAddr + GetRandomBetween1And(PartitionSize));
                        ReAllocParams.NumberOfForbiddenRanges = 3;
                        ReAllocParams.NumberOfForbiddenBorders = 3;
                    }
                    else
                    {
                        ReAllocParams.NumberOfForbiddenRanges = 0;
                        ReAllocParams.NumberOfForbiddenBorders = 0;
                    }

                    /* Random size */
                    ReAllocParams.Size = GetRandomBetween1And(PartitionSize / 10);

                    if (j >= RANDOM_TESTS_MAX_BLOCKS)
                    {
                        /* Could not find free index */
                        STTBX_Print(("Re-allocation skipped because no block allocated.\n"));
                        APISkipCounter++;
                        /* Don't try any more for this time... */
                        i = 1;
                    }
                    else
                    {
                        /* Index to valid block found */
                        STTBX_Print(("Re-allocating  %#x ", ReAllocParams.Size));

                        /* Randomly Preserve data or not */
                        if (GetRandomBetween1And(2) == 1)
                        {
                            ReAllocParams.PreserveData = TRUE;
                            STTBX_Print(("(PreserveData) "));
                        }
                        else
                        {
                            ReAllocParams.PreserveData = FALSE;
                        }
                        ReAllocParams.PartitionHandle = PartitionHandleTable[PartitionIndex[HIndex]];
                        STTBX_Print(("in partition %d ...\n", PartitionIndex[HIndex]));
                        Errors += ReAllocAndVerify(&ReAllocParams, &H[HIndex], FALSE);
                        ReAllocCounter++;
                    }
                    break;

/******************************************************************************/
                case 3: /* Freeing */
                    STTBX_Print(("(%d) ", TotalAllocated));
                    /* Find a used block index */
                    j = 0;
                    offset = GetRandomBetween1And(RANDOM_TESTS_MAX_BLOCKS);
                    while ((H[(j + offset) % RANDOM_TESTS_MAX_BLOCKS] == STAVMEM_INVALID_BLOCK_HANDLE) && (j < RANDOM_TESTS_MAX_BLOCKS))
                    {
                        j++;
                    }
                    if (j < RANDOM_TESTS_MAX_BLOCKS)
                    {
                        /* Index to valid block found */

                        STTBX_Print(("Freeing "));
                        HIndex = (j + offset) % RANDOM_TESTS_MAX_BLOCKS;
                        FreeParams.PartitionHandle = PartitionHandleTable[PartitionIndex[HIndex]];
                        STTBX_Print(("in partition %d...\n", PartitionIndex[HIndex]));
                        Errors += FreeAndVerify(&FreeParams, &H[HIndex], FALSE);
                        H[HIndex] = STAVMEM_INVALID_BLOCK_HANDLE;
                        TotalAllocated--;
                        FreeCounter++;
                    }
                    else
                    {
                        /* Could not find free index */
                        STTBX_Print(("Free skipped because no block allocated.\n"));
                        APISkipCounter++;
                        /* Don't try any more for this time... */
                        i = 1;
                    }

                    break;

/******************************************************************************/
                case 14: /* Pause */
                    /* Do nothing */
                    i = 1; /* Don't do this many times... */
                    break;

/******************************************************************************/
                case 15: /* Display help */
                case 16: /* Display help and fill memory with random numbers */
                    STTBX_Print(("\n******* RANDOM TESTS OF ALLOC, REALLOC, FREE OF BLOCK *******\n"));
                    STTBX_Print(("*********  Press 'p' or 'P' to Pause/Resume the test  **********\n"));
#ifdef DEBUG_CODE
                    STTBX_Print(("*********  Press 'l' or 'L' to display List of blocks  ********\n"));
#endif
                    STTBX_Print(("*********  Press 'a' or 'A' to force Allocation  ***********\n"));
                    STTBX_Print(("*********  Press 'r' or 'R' to force Re-allocation  ********\n"));
                    STTBX_Print(("*********  Press 'f' or 'F' to force Freeing  **************\n"));
                    STTBX_Print(("********* Press 's' or 'S' to Stop the test  ************\n\n"));
                    i = 1; /* Don't do this many times... */
                    if (Action == 15)
                    {
                        break;
                    }

                    /* Fill memory with random numbers */
                    for (j=0; j < TESTS_MAX_PARTITIONS; j++)
                    {
                        FillMemoryWithRandom(PartitionMapTable[j].StartAddr_p,
                                             (U32)PartitionMapTable[j].StopAddr_p - (U32)PartitionMapTable[j].StartAddr_p + 1,
                                             TRUE);
                    }
                    break;

                default:
                    break;
            } /* End of action process */
        } /* End of 1 to 10 times same action */
        if (Action != 14)
        {
            /* Test blocks info */
#ifdef DEBUG_CODE
            DisplayAllBlocksInfo(FALSE);
#endif
            /* Allow randow action choose, but not if paused */
            Action = 0;
        }
    } /* End of actions loop */

#ifdef DEBUG_CODE
    /* End: display all blocks */
    DisplayAllBlocksInfo(TRUE);
#endif

    /* Free all blocks before exiting */
    for (j = 0; j < RANDOM_TESTS_MAX_BLOCKS; j++)
    {
        if (H[j] != STAVMEM_INVALID_BLOCK_HANDLE)
        {
            Errors += FreeAndVerify(&FreeParams, &H[j], FALSE);
        }
    }

    /* Display stats */
    STTBX_Print(("Statistics:\n # asked allocations   : %d\n # asked re-allocations: %d\n", AllocCounter, ReAllocCounter));
    STTBX_Print((" # asked freeing       : %d\n # API calls skipped   : %d\n", FreeCounter,  APISkipCounter));

    /* Display error(s) status */
    if (Errors == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        /* Ring ... */
        STTBX_Print(("\a"));
        STTBX_Print(("****  ERROR !!! ****  (%d errors)\n\n", Errors));
        return(TRUE);
    }
}

#ifdef STAVMEM_DEBUG_BLOCKS_INFO
/*******************************************************************************
Name        : AvmemListBlocks
Description : List Blocks and print information
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : FALSE (success)
*******************************************************************************/
static BOOL AvmemListBlocks(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    STAVMEM_DisplayAllBlocksInfo();
    return(FALSE);
}
#endif /* #ifdef STAVMEM_DEBUG_BLOCKS_INFO */

/*******************************************************************************
Name        : TestsPartitions
Description : Test memory partitions
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL TestsPartitions( STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    STAVMEM_InitParams_t InitAvmem;
    STAVMEM_CreatePartitionParams_t CreateParams;
    STAVMEM_AllocBlockParams_t AllocParams;
    STAVMEM_BlockHandle_t BlockHandle;
    STAVMEM_TermParams_t TermAvmem;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    BOOL RetErr = FALSE;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(ResultSymbol_p);


    /* Initialise AVMEM and open a handle */
    STTST_EvaluateInteger("devicetype", (S32*)&InitAvmem.DeviceType, 16);

    Err = StandardInit(&InitAvmem);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Error: STAVMEM cannot be initialised !\n"));
        RetErr = TRUE;
    }
    else
    {
        if (InitAvmem.DeviceType == STAVMEM_DEVICE_TYPE_VIRTUAL)
        {
            STAVMEM_GetSharedMemoryVirtualMapping(VMap_p);
        }

        /* Test partitions */
        RetErr = ((TestMultipleCreateDelete()) || (RetErr));
        RetErr = ((TestDeleteWithBlocks()) || (RetErr));

        /* Terminate AVMEM */
        Err = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermAvmem);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error: STAVMEM cannot be terminated because it was not initialised.\n"));
            RetErr = TRUE;
        }
    }

    /* Test terminate with existing partition */
    Err = STAVMEM_Init(STAVMEM_DEVICE_NAME, &InitAvmem);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Error: STAVMEM cannot be initialised !\n"));
        RetErr = TRUE;
    }
    else
    {
        STTBX_Print(("Testing STAVMEM termination with existing partitions:\n"));

        /* Create a partition */
        PartitionMapTable[0].StartAddr_p     = (void *) VIRTUAL_BASE_ADDRESS;
        PartitionMapTable[0].StopAddr_p      = (void *) (VIRTUAL_BASE_ADDRESS + VIRTUAL_WINDOW_SIZE - 1);
        CreateParams.NumberOfPartitionRanges = (sizeof(PartitionMapTable[0]) / sizeof(STAVMEM_MemoryRange_t));
        CreateParams.PartitionRanges_p       = &PartitionMapTable[0];
        Err = STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME, &CreateParams, &PartitionHandleTable[0]);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error: Partition cannot be created !\n"));
            RetErr = TRUE;
        }

        /* Try to terminate without ForceTerminate: should fail */
        TermAvmem.ForceTerminate = FALSE;
        Err = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermAvmem);
        if (Err != STAVMEM_ERROR_CREATED_PARTITION)
        {
            STTBX_Print(("Error: STAVMEM should fail to terminate because a partition still exists !\n"));
            RetErr = TRUE;
        }
        else
        {
            STTBX_Print(("STAVMEM failed to terminate because a partition exists. (OK)\n"));
        }

        /* ForceTerminate */
        TermAvmem.ForceTerminate = TRUE;
        Err = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermAvmem);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error: STAVMEM cannot terminate !\n"));
            RetErr = TRUE;
        }
        else
        {
            STTBX_Print(("STAVMEM now terminated with ForceTerminate. (OK)\n"));
        }

        /* Partition should be invalid now */
        Err = STAVMEM_Init(STAVMEM_DEVICE_NAME, &InitAvmem);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error: STAVMEM cannot be initialised !\n"));
            RetErr = TRUE;
        }
        AllocParams.Alignment = 1;
        AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocParams.NumberOfForbiddenRanges = 0;
        AllocParams.NumberOfForbiddenBorders = 0;
        AllocParams.Size = 1000;
        AllocParams.ForbiddenRangeArray_p = NULL;
        AllocParams.ForbiddenBorderArray_p = NULL;
        AllocParams.PartitionHandle = PartitionHandleTable[0];
        Err = STAVMEM_AllocBlock(&AllocParams, &BlockHandle);
        if (Err == STAVMEM_ERROR_INVALID_PARTITION_HANDLE)
        {
            STTBX_Print(("Partition is invalid after forced termination. (OK)\n"));
        }
        else
        {
            STTBX_Print(("Error: After forced termination, partition should be invalid !\n"));
            RetErr = TRUE;
        }
        Err = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermAvmem);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error: STAVMEM cannot terminate !\n"));
            RetErr = TRUE;
        }
    }
    STTBX_Print(("\n"));

    if (!RetErr)
    {
        STTBX_Print(("**** No error in tests of partitions **** \n\n"));
    }
    else
    {
        STTBX_Print(("**** THERE ARE ERRORS IN TESTS OF PARTITIONS **** \n\n\a"));
    }

    return(RetErr);
}


/*******************************************************************************
Name        : TestMultipleCreateDelete
Description : test multiple creation and deletion of partitions
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if success, TRUE if error
*******************************************************************************/
static BOOL TestMultipleCreateDelete(void)
{
    STAVMEM_CreatePartitionParams_t CreateParams;
    STAVMEM_DeletePartitionParams_t DeleteParams;
    STAVMEM_AllocBlockParams_t AllocParams;
    STAVMEM_BlockHandle_t BlockHandle;
    U32 AskedTotalFreeSize, AskedPartitionFreeSize, ExpectedTotalFreeSize, ExpectedPartitionFreeSize[TESTS_MAX_PARTITIONS+1];
    U32 i, StartOffset, StopOffset;
    U32 MaxPlusOne = TESTS_MAX_PARTITIONS+1;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    BOOL RetErr = FALSE;

    STTBX_Print(("Testing multiple creation/deletion of partitions:\n"));

    ExpectedTotalFreeSize = 0;
    StartOffset = VIRTUAL_WINDOW_SIZE / (8 *MaxPlusOne); /* arbitrary, in order to prevent partitions from being contiguous */
    StopOffset  = VIRTUAL_WINDOW_SIZE / (16*MaxPlusOne); /* idem */
    for (i=0; i < MaxPlusOne; i++)
    {
        PartitionMapTable[i].StartAddr_p = (void *) (VIRTUAL_BASE_ADDRESS + VIRTUAL_WINDOW_SIZE * i/MaxPlusOne + StartOffset);
        PartitionMapTable[i].StopAddr_p  = (void *) (VIRTUAL_BASE_ADDRESS + VIRTUAL_WINDOW_SIZE * (i+1)/MaxPlusOne - 1 - StopOffset);
        ExpectedPartitionFreeSize[i]     = (((U32) PartitionMapTable[i].StopAddr_p) + 1 - ((U32) PartitionMapTable[i].StartAddr_p));
    }
    /* Create TESTS_MAX_PARTITIONS partitions */
    for (i=0; (i < TESTS_MAX_PARTITIONS) && !RetErr; i++)
    {
        CreateParams.NumberOfPartitionRanges = (sizeof(PartitionMapTable[i]) / sizeof(STAVMEM_MemoryRange_t));;
        CreateParams.PartitionRanges_p       = &PartitionMapTable[i];
        Err = STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME, &CreateParams, &PartitionHandleTable[i]);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error: Partition #%d cannot be created !\n", i));
            return(TRUE);
        }
        ExpectedTotalFreeSize += ExpectedPartitionFreeSize[i];
        STAVMEM_GetPartitionFreeSize(PartitionHandleTable[i], &AskedPartitionFreeSize);
        if (AskedPartitionFreeSize != ExpectedPartitionFreeSize[i])
        {
            STTBX_Print(("Error: Partition #%d size returned (%d) different from expected (%d)!\n",
                            i, AskedPartitionFreeSize, ExpectedPartitionFreeSize[i] ));
            RetErr = TRUE;
        }
    }

    /* Try to create one more partition: should fail */
    CreateParams.NumberOfPartitionRanges = (sizeof(PartitionMapTable[TESTS_MAX_PARTITIONS]) / sizeof(STAVMEM_MemoryRange_t));;
    CreateParams.PartitionRanges_p       = &PartitionMapTable[TESTS_MAX_PARTITIONS];
    Err = STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME, &CreateParams, &PartitionHandleTable[TESTS_MAX_PARTITIONS]);
    if (Err != STAVMEM_ERROR_MAX_PARTITION)
    {
        STTBX_Print(("Error: partition #%d should not have been created !\n", TESTS_MAX_PARTITIONS));
        RetErr = TRUE;
    }
    else
    {
        STTBX_Print(("partition #%d (index from 0) cannot be created because max is %d. (OK)\n", TESTS_MAX_PARTITIONS, TESTS_MAX_PARTITIONS));
    }

    /* Test free sizes */
    STAVMEM_GetFreeSize(STAVMEM_DEVICE_NAME, &AskedTotalFreeSize);
    if (AskedTotalFreeSize != ExpectedTotalFreeSize)
    {
        STTBX_Print(("Error: Total size returned (%d) different from expected (%d)!\n",
                        AskedTotalFreeSize, ExpectedTotalFreeSize ));
        RetErr = TRUE;
    }
    else
    {
        STTBX_Print(("Total size is sum of partitions free size (%#x bytes). (OK)\n", AskedTotalFreeSize));
    }
    for (i=0; i < TESTS_MAX_PARTITIONS; i++)
    {
        STAVMEM_GetPartitionFreeSize(PartitionHandleTable[i], &AskedPartitionFreeSize);
        if (AskedPartitionFreeSize != ExpectedPartitionFreeSize[i])
        {
            STTBX_Print(("Error: Partition #%d size returned (%d) different from expected (%d)!\n",
                            i, AskedPartitionFreeSize, ExpectedPartitionFreeSize[i] ));
            RetErr = TRUE;
        }
    }

    /* Delete partitions */
    DeleteParams.ForceDelete = TRUE;
    for (i=0; i < TESTS_MAX_PARTITIONS; i++)
    {
        Err = STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &PartitionHandleTable[i]);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Error: Partition #%d cannot be deleted !\n", i));
            RetErr = TRUE;
        }
    }

    /* Try to delete a partition again: should fail */
    DeleteParams.ForceDelete = TRUE;
    Err = STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &PartitionHandleTable[0]);
    if (Err != STAVMEM_ERROR_INVALID_PARTITION_HANDLE)
    {
        STTBX_Print(("Error: Partition cannot be deleted !\n"));
        RetErr = TRUE;
    }
    else
    {
        STTBX_Print(("No partition to delete again. (OK)\n"));
    }

    /* Partition should be invalid now */
    AllocParams.Alignment = 1;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.Size = 1000;
    AllocParams.ForbiddenRangeArray_p = NULL;
    AllocParams.ForbiddenBorderArray_p = NULL;
    AllocParams.PartitionHandle = PartitionHandleTable[0];
    Err = STAVMEM_AllocBlock(&AllocParams, &BlockHandle);
    if (Err == STAVMEM_ERROR_INVALID_PARTITION_HANDLE)
    {
        STTBX_Print(("Partition is invalid after being deleted. (OK)\n"));
    }
    else
    {
        STTBX_Print(("Error: After forced termination, partition should be invalid !\n"));
        RetErr = TRUE;
    }


    STTBX_Print(("\n"));

    return(RetErr);
}



/*******************************************************************************
Name        : TestDeleteWithBlocks
Description : test multiple creation and deletion of partitions
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if success, TRUE if error
*******************************************************************************/
static BOOL TestDeleteWithBlocks(void)
{
    STAVMEM_CreatePartitionParams_t CreateParams;
    STAVMEM_DeletePartitionParams_t DeleteParams;
    STAVMEM_BlockHandle_t BlockHandle;
    STAVMEM_AllocBlockParams_t AllocParams;
    void * DummyAddr;
    U32 Errors = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    BOOL RetErr = FALSE;

    STTBX_Print(("Testing creation/deletion of partitions with existing blocks:\n"));

    /* Create a partition */
    PartitionMapTable[0].StartAddr_p     = (void *) VIRTUAL_BASE_ADDRESS;
    PartitionMapTable[0].StopAddr_p      = (void *) (VIRTUAL_BASE_ADDRESS + VIRTUAL_WINDOW_SIZE - 1);
    CreateParams.NumberOfPartitionRanges = (sizeof(PartitionMapTable[0]) / sizeof(STAVMEM_MemoryRange_t));;
    CreateParams.PartitionRanges_p       = &PartitionMapTable[0];
    Err = STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME, &CreateParams, &PartitionHandleTable[0]);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Error: Partition cannot be created !\n"));
        RetErr = TRUE;
    }

    /* Alloc block here */
    AllocParams.Alignment = 1;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.Size = 1000;
    AllocParams.ForbiddenRangeArray_p = NULL;
    AllocParams.ForbiddenBorderArray_p = NULL;
    AllocParams.PartitionHandle = PartitionHandleTable[0];
    STTBX_Print(("Allocating block: "));
    Err = STAVMEM_AllocBlock(&AllocParams, &BlockHandle);
    Errors = VerifyAllocBlock(BlockHandle, &AllocParams);
    if ((Err != ST_NO_ERROR) || (Errors != 0))
    {
        STTBX_Print(("problem encountered.\n"));
        RetErr = TRUE;
    }
    else
    {
        STTBX_Print(("done.\n"));
    }

    /* Try to delete partition without ForceDelete: should fail */
    DeleteParams.ForceDelete = FALSE;
    Err = STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &PartitionHandleTable[0]);
    if (Err != STAVMEM_ERROR_ALLOCATED_BLOCK)
    {
        STTBX_Print(("Error: Partition should not be able to be deleted !\n"));
        RetErr = TRUE;
    }
    else
    {
        STTBX_Print(("Partition cannot be deleted because block exists. (OK)\n"));
    }

    /* Delete partition */
    DeleteParams.ForceDelete = TRUE;
    Err = STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &PartitionHandleTable[0]);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Error: Partition cannot be deleted !\n"));
        RetErr = TRUE;
    }
    else
    {
        STTBX_Print(("Partition now deleted with ForceDelete. (OK)\n"));
    }

    /* Block should be invalid now */
    Err = STAVMEM_GetBlockAddress(BlockHandle, &DummyAddr);
    if (Err == STAVMEM_ERROR_INVALID_BLOCK_HANDLE)
    {
        STTBX_Print(("Block is invalid after partition force delete. (OK)\n"));
    }
    else
    {
        STTBX_Print(("Error: Aftter partition force delete, block should be invalid !\n"));
        RetErr = TRUE;
    }

    STTBX_Print(("\n"));

    return(RetErr);
}
#ifdef TRACE_DYNAMIC_DATASIZE
/*******************************************************************************
Name        : AVMEM_GetDynamicDataSize
Description : Get DynamicDatasize allocated for the driver
Parameters  : None
Returns     : None
*******************************************************************************/
static BOOL AVMEM_GetDynamicDataSize(STTST_Parse_t *pars_p, char *result_sym_p)
{


    printf("*******************************************\n");
    printf("* Dynamic data size calculation beginning *\n");
    printf("*******************************************\n");
    DynamicDataSize=0;
    AVMEM_Init();
    printf("** Size of Dynamic Data= %d bytes **\n", DynamicDataSize);
    AVMEM_Term();
    printf("*******************************************\n");
    printf("*   Dynamic data size calculation end     *\n");
    printf("*******************************************\n");

    return(FALSE );
}
#endif
#if defined (DVD_SECURED_CHIP)
/*******************************************************************************
Name        : AVMEM_SetSecRange
Description :
Parameters  : None
Returns     : None
*******************************************************************************/
static BOOL AVMEM_SetSecRange(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr = FALSE;
    S32 Var;
    void * StartAdd_p;
    void * StopAdd_p;

    unsigned int uMemoryStatus;

    UNUSED_PARAMETER(ResultSymbol_p);
    RetErr = STTST_GetInteger( Parse_p, PARTITION1_START, &Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( Parse_p, "Expected start address" );
        return(TRUE);
    }
    StartAdd_p = (void*)Var;

    RetErr = STTST_GetInteger( Parse_p, PARTITION1_STOP, &Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( Parse_p, "Expected stop address" );
        return(TRUE);
    }
    StopAdd_p = (void*)Var;


    uMemoryStatus = STMES_SetSecureRange((void *)StartAdd_p, (void *)(StopAdd_p - 1), 0);

    if( uMemoryStatus != ST_NO_ERROR )
    {
            printf("Error for STMES_SetSecureRange \n");
            return(TRUE);
    }
    else
    {
            printf("Region Created \n");
            return(FALSE);
    }

}
#endif

#ifdef ST_OSWINCE
/*******************************************************************************
Name        : WinceTest
Description : WinceTest functions (STAVMEM_FillBlock1D and STAVMEM_CopyBlock1D)
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL WinceTest( STTST_Parse_t *Parse_p, char *ResultSymbol_p )
{
    BOOL Err = FALSE;
    S32 basemem;
	char *DataAdd_p;
	char *CPU1_p, *CPU2_p;
  	char *Device1_p, *Device2_p;
    STAVMEM_AllocBlockParams_t AllocBlockParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    STAVMEM_BlockHandle_t BlockHandle;


	STAVMEM_BlockHandle_t MyBlock1 = STAVMEM_INVALID_BLOCK_HANDLE;
	STAVMEM_BlockHandle_t MyBlock2 = STAVMEM_INVALID_BLOCK_HANDLE;

	STTBX_Print(("WINCE AVMEM Test \n\n"));


	/* Block 1 Partition 0 ******************************************/

	/* Allocate a block in partition 0 */
    GlobalAllocParams.PartitionHandle = AvmemPartitionHandle[0];
    GlobalAllocParams.Size = 720 * 576;
    GlobalAllocParams.Alignment = 1;
    GlobalAllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    GlobalAllocParams.ForbiddenRangeArray_p = &GlobalForbidRange[0];
    GlobalAllocParams.ForbiddenBorderArray_p = &GlobalForbidBorder[0];
    GlobalAllocParams.NumberOfForbiddenRanges = 2;
    GlobalAllocParams.NumberOfForbiddenBorders = 1;
    Err = STAVMEM_AllocBlock(&GlobalAllocParams, &MyBlock1);
    STTBX_Print(("STAVMEM_AllocBlock 1"));
	if (Err) STTBX_Print(("Err1"));

    /* Get parameters of the allocated block */
    Err = STAVMEM_GetBlockParams(MyBlock1, &GlobalMyBlockParams);
    STTBX_Print(("STAVMEM_GetBlockParams 1"));
	if (Err) STTBX_Print(("Err2"));

    /* Fill the allocated block with zeros */
    Err = STAVMEM_FillBlock1D((void *) &GlobalZeroPattern, 1, GlobalMyBlockParams.StartAddr_p, GlobalMyBlockParams.Size);
    STTBX_Print(("STAVMEM_FillBlock1D 1"));
	if (Err) STTBX_Print(("Err3"));

	/* Block 2 Partition 1 ******************************************/

	/* Allocate a block in partition 1 */
    GlobalAllocParams.PartitionHandle = AvmemPartitionHandle[1];
    GlobalAllocParams.Size = 720 * 576;
    GlobalAllocParams.Alignment = 1;
    GlobalAllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    GlobalAllocParams.ForbiddenRangeArray_p = &GlobalForbidRange[0];
    GlobalAllocParams.ForbiddenBorderArray_p = &GlobalForbidBorder[0];
    GlobalAllocParams.NumberOfForbiddenRanges = 2;
    GlobalAllocParams.NumberOfForbiddenBorders = 1;
    Err = STAVMEM_AllocBlock(&GlobalAllocParams, &MyBlock2);
    STTBX_Print(("STAVMEM_AllocBlock 2"));
	if (Err) STTBX_Print(("Err4"));

    /* Get parameters of the allocated block */
    Err = STAVMEM_GetBlockParams(MyBlock2, &GlobalMyBlockParams);
    STTBX_Print(("STAVMEM_GetBlockParams 2"));
	if (Err) STTBX_Print(("Err5"));

    /* Fill the allocated block with zeros */
    Err = STAVMEM_FillBlock1D((void *) &GlobalZeroPattern, 1, GlobalMyBlockParams.StartAddr_p, GlobalMyBlockParams.Size);
    STTBX_Print(("STAVMEM_FillBlock1D 2"));
	if (Err) STTBX_Print(("Err6"));


     STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);    /* global*/

	/* Acces from Device ******************************************/

     STAVMEM_GetBlockAddress(MyBlock1, &DataAdd_p);
     Device1_p = STAVMEM_VirtualToDevice(DataAdd_p,&VirtualMapping);    /* block 1 seen from device*/
	 STTBX_Print(("Block1: Device %x Virtual %x\n", DataAdd_p, Device1_p));

     STAVMEM_GetBlockAddress(MyBlock2, &DataAdd_p);
     Device2_p = STAVMEM_VirtualToDevice2(DataAdd_p,&VirtualMapping);   /*block 2 seen from device*/

	 STTBX_Print(("Block2: Device %x Virtual %x\n", DataAdd_p, Device2_p));

	 STTBX_Print(("Access using DEVICE addresses..."));
/*   *Device1_p = *Device2_p;       // only possible with USER_MODE*/

	/* Acces from CPU ******************************************/

     STAVMEM_GetBlockAddress(MyBlock1, &DataAdd_p);
     CPU1_p = STAVMEM_VirtualToCPU(DataAdd_p,&VirtualMapping);  /* block 1 seen from CPU*/
	 STTBX_Print(("Block1: Cpu %x Virtual %x\n", DataAdd_p, CPU1_p));

     STAVMEM_GetBlockAddress(MyBlock2, &DataAdd_p);
     CPU2_p = STAVMEM_VirtualToCPU(DataAdd_p,&VirtualMapping);   /*block 2 seen from CPU*/
	 STTBX_Print(("Block2: Cpu %x Virtual %x\n", DataAdd_p, CPU2_p));

/*   STTBX_Print(("Access using CPU addresses..."));*/
     *CPU1_p = *CPU2_p;      /*always possible*/


    /* Free the block (not necessary with ForceDelete) */
    GlobalFreeParams.PartitionHandle = AvmemPartitionHandle[1];
    Err = STAVMEM_FreeBlock(&GlobalFreeParams, &MyBlock2);
    STTBX_Print(("STAVMEM_FreeBlock 2"));
	if (Err) STTBX_Print(("Err7"));

    /* Free the block (not necessary with ForceDelete) */
    GlobalFreeParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&GlobalFreeParams, &MyBlock1);
    STTBX_Print(("STAVMEM_FreeBlock 1"));
	if (Err) STTBX_Print(("Err8"));

    STTBX_Print(("End of WINCE test\n\n\a"));

    return(Err);
}

#endif /*ST_OSWINCE*/

/*******************************************************************************
Name        : STAVMEM_MacroInit
Description : Definition of testtool commands
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL STAVMEM_MacroInit (void)
{
    BOOL RetErr = FALSE;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr |= ((STTST_RegisterCommand("AVMEM_HELP", AvmemHelp, "Some help on STAVMEM test functions")));
    RetErr |= ((STTST_RegisterCommand("SDRAM_N", Set_SdramN, "Set working memory area 'membase' to SDRAM n")));
#ifdef DRAM_TEST
    RetErr |= ((STTST_RegisterCommand("DRAM",   dram,   "Set working memory area 'membase' to DRAM")));
#endif
    RetErr |= ((STTST_RegisterCommand("AVMEM_DUMP", dump, "Display memory dumping while testing AVMEM")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_NODUMP", nodump, "Do not display memory dumping while testing AVMEM")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_DeviceType", Set_DeviceType, "Set 'devicetype'")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_Init", TTAVMEM_Init, "Initialise AVMEM (with 'devicetype') and create partition.")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_Term", TTAVMEM_Term, "Delete partition and terminate AVMEM")));

    RetErr |= ((STTST_RegisterCommand("AVMEM_Partitions", TestsPartitions, "Test AVMEM memory partitions (with 'devicetype')")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_Access", TestsAccess, "<Interactive:0/1>Test AVMEM memory accesses (within ALL memory areas)")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_Speed", TestsSpeed, "<verify:0/1><loop:0/1><32 bytes align:0/1>Test AVMEM memory functions speed (within ALL memory areas)")));
#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
    RetErr |= ((STTST_RegisterCommand("AVMEM_1DBigData", Test1DBigData, "Test AVMEM 1D copy with big amount of data")));
#endif /* STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE */
#ifdef STAVMEM_DEBUG_BLOCKS_INFO
    RetErr |= ((STTST_RegisterCommand("AVMEM_LIST_BLOCKS", AvmemListBlocks, "List all AVMEM blocks")));
#endif /* #ifdef STAVMEM_DEBUG_BLOCKS_INFO */
    RetErr |= ((STTST_RegisterCommand("AVMEM_1D", Tests1D, "Test AVMEM 1D copy and fill (within working memory area 'membase')")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_2DCopy", Tests2DCopy, "Test AVMEM 2D copy (within working memory area 'membase')")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_2DFill", Tests2DFill, "Test AVMEM 2D fill (within working memory area 'membase')")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_BlocksAlloc", TestsBlocksAlloc, "Test AVMEM memory blocks allocations in SDRAM (with 'devicetype')")));
#if defined (DVD_SECURED_CHIP)
    #if !defined(STLAYER_NO_STMES)
    RetErr |= STTST_RegisterCommand( "AVMEM_SetMESMode1"  ,AVMEM_SetMESMode1,"Create partition  ");
    RetErr |= STTST_RegisterCommand( "AVMEM_SetMESMode2"  ,AVMEM_SetMESMode2,"Create partition  ");
    RetErr |= STTST_RegisterCommand( "AVMEM_SetMESMode3"  ,AVMEM_SetMESMode3,"Create partition  ");
    RetErr |= STTST_RegisterCommand( "AVMEM_SetMESMode4"  ,AVMEM_SetMESMode4,"Create partition  ");
    RetErr |= STTST_RegisterCommand( "AVMEM_TermMES"      ,AVMEM_TermMES,"Create partition  ");
    #else
    RetErr |= ((STTST_RegisterCommand("AVMEM_SSecReg", AVMEM_SetSecRange, "Set Secure range start and stop address ")));
    #endif
#endif
#ifdef ST_OSWINCE
    RetErr |= ((STTST_RegisterCommand("AVMEM_Wince" , WinceTest, "Test WINCE")));
#endif /* ST_OS20 */

#ifdef ST_OS20
    RetErr |= ((STTST_RegisterCommand("AVMEM_StackUse" , (BOOL (*)(STTST_Parse_t*, char*))metrics_Stack_Test, "Test AVMEM stack use")));
#endif /* ST_OS20 */
#ifdef TRACE_DYNAMIC_DATASIZE
    RetErr |= STTST_RegisterCommand("AVMEM_GetDynamicDataSize",AVMEM_GetDynamicDataSize,"Get Dynamic  Data Size allocated for the driver");
#endif
    STTST_AssignInteger("SDRAM_BASE_ADDRESS", (S32)SDRAM_CPU_BASE_ADDRESS, TRUE);
#if (MAX_PARTITIONS == 2)
    STTST_AssignInteger("PARTITION1_START", (S32)PARTITION1_START, TRUE);
#endif
#ifdef DRAM_TEST
    STTST_AssignInteger("DRAM_BASE_ADDRESS", (S32)DRAM_BASE_ADDRESS, TRUE);
#endif

    STTST_AssignInteger("devicetype", STAVMEM_DEVICE_TYPE_VIRTUAL, FALSE);
    STTST_AssignInteger("membase", (S32)SDRAM_CPU_BASE_ADDRESS, FALSE);
    STTST_AssignInteger("memdump", 0, FALSE);

#ifdef STAVMEM_DEBUG_MEMORY_STATUS
    RetErr |= AVMEM_Command();
#endif

    if ( !RetErr )
    {
        STTBX_Print(( "STAVMEM_MacroInit()     \t: ok           %s\n", STAVMEM_GetRevision()  ));
    }
    else
    {
        STTBX_Print(( "STAVMEM_MacroInit()     \t: failed !\n" ));
    }
    return(RetErr);
} /* end STAVMEM_MacroInit */


/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

/*******************************************************************************
Name        : DumpMemory
Description : Display memory content
Parameters  : first address of memory to display, Number of lines of memory to display
              (one line is 0x10 locations)
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
void DumpMemory(void *BeginAddr_p, const int NumberOfLines)
{
    int i,dumps;
    U32 address;
    char PrintBuffer[255];
    S32 dumpmem;
#ifdef FORCE_32BITS_ALIGN
    volatile long *pt;
#else
    char  *pt;
#endif

    STTST_EvaluateInteger("memdump", &dumpmem, 16);
    if (dumpmem == 0)
    {
        return;
    }
    address = ((U32) ((U32 *) (((U32)IFV2CPU(BeginAddr_p))&~(0x3)))) - (((U32) ((U32 *) ((U32)(IFV2CPU(BeginAddr_p))&~(0x3)))) & 0xF);
    dumps = 0;
    while (( dumps < NumberOfLines) && (dumps < MAX_DUMP_LINES) )
    {
#ifdef FORCE_32BITS_ALIGN
        pt = (volatile long *)(address & 0xFFFFFFFC);
        sprintf( PrintBuffer,
                "h%08lx: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x",
                address,
                GETBYTE1(*pt)    , GETBYTE2(*pt)    , GETBYTE3(*pt)    , GETBYTE4(*pt),
                GETBYTE1(*(pt+1)), GETBYTE2(*(pt+1)), GETBYTE3(*(pt+1)), GETBYTE4(*(pt+1)),
                GETBYTE1(*(pt+2)), GETBYTE2(*(pt+2)), GETBYTE3(*(pt+2)), GETBYTE4(*(pt+2)),
                GETBYTE1(*(pt+3)), GETBYTE2(*(pt+3)), GETBYTE3(*(pt+3)), GETBYTE4(*(pt+3)));
        STTBX_Print(("  %s", PrintBuffer));
        for (i=0;i<4;i++)
        {
            PrintBuffer[4*i]   = GETBYTE1(*(pt+i));
            PrintBuffer[4*i+1] = GETBYTE2(*(pt+i));
            PrintBuffer[4*i+2] = GETBYTE3(*(pt+i));
            PrintBuffer[4*i+3] = GETBYTE4(*(pt+i));
        }
        for (i=0;i<16;i++)
        {
            if (isprint(PrintBuffer[i]) == 0)
            {
                PrintBuffer[i] = '.';
            }
        }
#else
        pt = (char *)address;
        sprintf( PrintBuffer,
                "h%08x: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x",
                address,
                *pt     , *(pt+1) , *(pt+2) , *(pt+3),
                *(pt+4) , *(pt+5) , *(pt+6) , *(pt+7),
                *(pt+8) , *(pt+9) , *(pt+10), *(pt+11),
                *(pt+12), *(pt+13), *(pt+14), *(pt+15) );
        STTBX_Print(("  %s", PrintBuffer));
        for (i=0;i<16;i++)
        {
            PrintBuffer[i] = (U8)*(pt+i);
            if (isprint(PrintBuffer[i]) == 0)
            {
                PrintBuffer[i] = '.';
            }
        }
#endif

        PrintBuffer[16] = '\0';
        STTBX_Print(("  %s\n", PrintBuffer));
        dumps ++;
        address += 16;

    } /* end while (( dumps < NumberOfLines) && (dumps < MAX_DUMP_LINES) ) */
}

/*******************************************************************************
Name        : GetRandomBetween1And
Description : get a random number between 1 and Max
Parameters  : Max
Assumptions :
Limitations :
Returns     : a random number
*******************************************************************************/
U32 GetRandomBetween1And(const U32 Max)
{
    U32 mask = Max, test;

    /* Build smallest "0xffff style" mask not affecting Max (next 2^n value - 1) */
    if (Max == 0)
    {
        /* max 0 is not valid: return 1 */
        return(1);
    }
    else if ((Max & (Max - 1)) != 0)
    {
        /* Mask must be 2^n (where n = 0,1,2,3,etc), so round up max to next 2^n. */
        mask = 2;
        test = Max;
        while (test > 1)
        {
            mask <<= 1;
            test >>= 1;
        }
    }

    /* Find a value inside range */
    do
    {
        /* Get value masked with smallest possible mask */
        test = (((U32) rand()) & (mask - 1)) + 1;
    } while (test > Max);

    /* Got a value <= Max */
    return(test);
}

/*******************************************************************************
Name        : FillMemoryWithRandom
Description : Fill memory with random numbers
Parameters  : Verbose mode
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
void FillMemoryWithRandom(void *MemoryBaseAddr_p, U32 MemorySize, BOOL Verbose)
{
    U32 *FillPointer;
    U32 j, Largest;

    /* Fill memory with random numbers */
    FillPointer = (U32 *)((U32)MemoryBaseAddr_p &~(0x3)); /* manual alignment neaded for st40 ,
                                                           * this is not normal as it should be done by the toolset  */


    if (Verbose)
    {
        STTBX_Print(("Filling memory with random numbers from 0x%0x to 0x%0x ...\n", (U32)MemoryBaseAddr_p, (U32)MemoryBaseAddr_p + MemorySize ));
    }
    for (j = 0; j < MemorySize / 16; j++)
    {
        /* actually, never value 0...    */
        Largest = GetRandomBetween1And(0xffffffff);
        *(FillPointer + 4 * j) = Largest;
        *(FillPointer + 4 * j + 1) = Largest >> 1;  /* still random */
        *(FillPointer + 4 * j + 2) = Largest << 1;  /* still random */
        *(FillPointer + 4 * j + 3) = ~Largest;      /* still random */
    }
    if (Verbose)
    {
        STTBX_Print(("Finished.\n\n"));
    }
} /* FillMemoryWithRandom */


/*******************************************************************************
Name        : BlockAndBip
Description : Bip and block execution with a loop. Change value to exit the loop
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
void BlockAndBip(void)
{
    U32 val = 0;
    STTBX_Print(("\a\a"));

    while (val == 0)
    {
#ifdef ST_WINCE_BLOCK_TEST
			STTBX_InputChar((char *)&val);
#endif
		; /* change value in val to exit the loop ! */
    }
}



BOOL Test_CmdStart()
{
    BOOL RetOk;

    RetOk = !STAVMEM_MacroInit();

    return(RetOk);
}

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

#ifdef ANOTHER_TASK
    task_t * Task_p;

#endif /* #ifdef ANOTHER_TASK */
UNUSED_PARAMETER(ptr);

#ifdef ANOTHER_TASK
Task_p = STOS_TaskCreate((void (*)(void* )) AnotherTask, NULL, 1024, 5, "another task", task_flags_no_min_stack_size);
    if ( Task_p == NULL)
    {
        printf("Unable to create task structure or stack !\n");
        exit(EXIT_FAILURE);
    }
    task_priority_set(NULL, 5);
#endif /* #ifdef ANOTHER_TASK */

    STAPIGAT_Init();


#ifdef ANOTHER_TASK
    STOS_TaskWait(&Task_p, 1, TIMEOUT_INFINITY);
    STOS_TaskDelete(Task_p);
    STTBX_Print(("ANOTHERTASK deleted.\n"));
#endif

    STAPIGAT_Term();

} /* end os20_main */


/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

#if defined(ST_OS21) ||  !defined(ST_OSWINCE)
    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
    os20_main(NULL);
#else
    os20_main(NULL);
#endif

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
 }



/* End of avm_test.c */

