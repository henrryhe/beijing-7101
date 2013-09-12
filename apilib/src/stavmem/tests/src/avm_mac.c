/*******************************************************************************

File name : avm_mac.c

Description : tests on avmem memory accesses

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
04 Dec 2002         Created from avm_spd.c                          HSdLM
*******************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */

#if defined(mb275) || defined(mb275_64) || defined(mb295) /* || defined(mb391)*/
#define TEST_ACCESS_DRAM
#endif /* #if defined(mb275) || defined(mb275_64) || defined(mb295) */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "stdevice.h"
#include "stddefs.h"

#include "stsys.h"
#include "stcommon.h"
#include "sttbx.h"

#include "stavmem.h"
#include "startup.h"
#include "clavmem.h"
#include "memory_map.h"
#include "avm_test.h"
#include "avm_mac.h"

/* External global variables ------------------------------------------------ */
extern unsigned char    *InternalBlock_p;
extern unsigned char    *ExternalBlock_p;

/*static unsigned char *InternalBlock_p = InternalBlock;*/
/*static unsigned char *ExternalBlock_p = ExternalBlock;*/
/* Private Types ------------------------------------------------------------ */

typedef enum SrcAndDestDump_e
{
    BEFORE,
    AFTER,
    FAILURE
} SrcAndDestDump_t;

typedef enum CopyFillChoice_e
{
    NOTHING,
    COPY,
    FILL
} CopyFillChoice_t;

typedef enum AccessSystem_e
{
    AS_BEST_1D,
    AS_BEST_2D,
    AS_BLK_MOVE_DMA,
    AS_FDMA,
    AS_GPDMA,
    AS_MPEG_1D,
    AS_MPEG_2D,
    AS_NUMBER /* keep it last */
} AccessSystem_t;

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

static const char ASystemName[AS_NUMBER][20] = { /* must keep AccessSystem_t order !!! */
    "BEST_1D",
    "BEST_2D",
    "BLK_MOVE_DMA",
    "FDMA",
    "GPDMA",
    "MPEG_1D",
    "MPEG_2D"
};

/************************************************************/
/* Performance tests: change here locations of memory areas */
/************************************************************/
#ifdef TEST_ACCESS_DRAM
/* Address of a range of at least 512Kbytes of cached DRAM */
#define TEST_ACCESS_DRAMc    (((U32) (ExternalBlock_p) + 0x15) & ~0xF)
#endif /* #ifdef TEST_ACCESS_DRAM */

#define TEST_ACCESS_SRAM     (((U32) (InternalBlock_p) + 0x15) & (U32)~0xF)

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define Min(Val1, Val2) (((Val1) < (Val2))?(Val1):(Val2))
#define Max(Val1, Val2) (((Val1) > (Val2))?(Val1):(Val2))
#define RangesOverlap(FirstAddr1, LastAddr1, FirstAddr2, LastAddr2) ((((U32) (FirstAddr1)) <= ((U32) (LastAddr2))) && (((U32) (LastAddr1)) >= ((U32) (FirstAddr2))))

/* Private Function prototypes ---------------------------------------------- */

static BOOL TestAccessAlignment(const char* AreaName_p, void *BaseAddr_p, const U32 OffsetWrite, const U32 OffsetRead);
static BOOL TestAccessEndianness(const char* AreaName_p, void *BaseAddr_p);
static BOOL TestAccessCache(U8 * CachedAddr_p);

static BOOL Check1DCopy(U32 SrcPhyAddr, U32 SrcWidth, U32 DestPhyAddr, AccessSystem_t ASystem);
static BOOL Check2DCopy(U32 SrcPhyAddr,  U32 SrcWidth,  U32 SrcHeight,  U32 SrcPitch,
                        U32 DestPhyAddr, U32 DestPitch, AccessSystem_t ASystem);
static BOOL Check1DFill(U32 PatPhyAddr, U32 PatWidth, U32 DestPhyAddr, U32 DestWidth, AccessSystem_t ASystem);
static BOOL Check2DFill(U32 PatPhyAddr,  U32 PatWidth,  U32 PatHeight,  U32 PatPitch,
                        U32 DestPhyAddr, U32 DestWidth, U32 DestHeight, U32 DestPitch, AccessSystem_t ASystem);

static void AccessSystemCopy(U32 SrcPhyAddr, U32 SrcWidth, U32 SrcHeight, U32 SrcPitch, U32 DestPhyAddr, U32 DestPitch, AccessSystem_t ASystem);
static void AccessSystemFill(U32 PatPhyAddr, U32 PatWidth, U32 PatHeight, U32 PatPitch, U32 DestPhyAddr, U32 DestWidth, U32 DestHeight, U32 DestPitch, AccessSystem_t ASystem);
static void DumpSourceAnDest(U32 SrcPhyAddr, U32 SrcOffset, U32 DestPhyAddr, U32 DestOffset, SrcAndDestDump_t DumpType);

#if defined (STAVMEM_TEST_DIRECT_PICK) && (defined(STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE) || defined(STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE))
static BOOL TestAccessSDRAM(U8 * SourceAddr_p, U8 * BaseAddr_p);
#endif

/* Functions ---------------------------------------------------------------- */

#ifdef STAVMEM_TEST_DIRECT_PICK
/* In order to test speed and access of particular methods, there is the need to have their functions available.
   For that, include them as it is done in acc_best.c, except that the methods HAVE TO BE AVAILABLE here,
   there is no check . */
#ifdef STAVMEM_MEM_ACCESS_GPDMA
#include "acc_gpd.h"
#define Copy1DNoOverlapGpdma(sa, da, s)               stavmem_GPDMA.Execute.Copy1DNoOverlap(sa, da, s)
#define Copy2DNoOverlapGpdma(sa, sw, sh, sp, da, dp)  stavmem_GPDMA.Execute.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
#define Fill1DGpdma(pa, ps, da, ds)                   stavmem_GPDMA.Execute.Fill1D(pa, ps, da, ds)
#define Fill2DGpdma(pa, pw, ph, pp, da, dw, dh, dp)   stavmem_GPDMA.Execute.Fill2D(pa, pw, ph, pp, da, dw, dh, dp)
#endif
#ifdef STAVMEM_MEM_ACCESS_FDMA
#include "acc_fdma.h"
#define Copy1DNoOverlapFdma(sa, da, s)               stavmem_FDMA.Execute.Copy1DNoOverlap(sa, da, s)
#define Copy2DNoOverlapFdma(sa, sw, sh, sp, da, dp)  stavmem_FDMA.Execute.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
#define Fill1DFdma(pa, ps, da, ds)                   stavmem_FDMA.Execute.Fill1D(pa, ps, da, ds)
#define Fill2DFdma(pa, pw, ph, pp, da, dw, dh, dp)   stavmem_FDMA.Execute.Fill2D(pa, pw, ph, pp, da, dw, dh, dp)
#endif
#ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
#include "acc_bmp.h"
#define Copy1DNoOverlapBlockMovePeripheral(sa, da, s)              stavmem_BlockMovePeripheral.Execute.Copy1DNoOverlap(sa, da, s)
#define Fill1DBlockMovePeripheral(pa, ps, da, ds)                  stavmem_BlockMovePeripheral.Execute.Fill1D(pa, ps, da, ds)
#define Fill2DBlockMovePeripheral(pa, pw, ph, pp, da, dw, dh, dp)  stavmem_BlockMovePeripheral.Execute.Fill2D(pa, pw, ph, pp, da, dw, dh, dp)
#endif
#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
#include "acc_2dbm.h"
#define Copy1DHandleOverlapMPEG2DBlockMove(sa, da, s)                   stavmem_MPEG2DBlockMove.Execute.Copy1DHandleOverlap(sa, da, s)
#define CheckCopy1DHandleOverlapMPEG2DBlockMove(sa, da, s)              stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy1DHandleOverlap(sa, da, s)
#define Copy1DNoOverlapMPEG2DBlockMove(sa, da, s)                       stavmem_MPEG2DBlockMove.Execute.Copy1DNoOverlap(sa, da, s)
#define CheckCopy1DNoOverlapMPEG2DBlockMove(sa, da, s)                  stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy1DNoOverlap(sa, da, s)
#define Copy2DHandleOverlapMPEG2DBlockMove(sa, sw, sh, sp, da, dp)      stavmem_MPEG2DBlockMove.Execute.Copy2DHandleOverlap(sa, sw, sh, sp, da, dp)
#define CheckCopy2DHandleOverlapMPEG2DBlockMove(sa, sw, sh, sp, da, dp) stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy2DHandleOverlap(sa, sw, sh, sp, da, dp)
#define Copy2DNoOverlapMPEG2DBlockMove(sa, sw, sh, sp, da, dp)          stavmem_MPEG2DBlockMove.Execute.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
#define CheckCopy2DNoOverlapMPEG2DBlockMove(sa, sw, sh, sp, da, dp)     stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
#define Fill1DMPEG2DBlockMove(pa, ps, da, ds)                           stavmem_MPEG2DBlockMove.Execute.Fill1D(pa, ps, da, ds)
#define CheckFill1DMPEG2DBlockMove(pa, ps, da, ds)                      stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Fill1D(pa, ps, da, ds)
#define Fill2DMPEG2DBlockMove(pa, pw, ph, pp, da, dw, dh, dp)           stavmem_MPEG2DBlockMove.Execute.Fill2D(pa, pw, ph, pp, da, dw, dh, dp)
#define CheckFill2DMPEG2DBlockMove(pa, pw, ph, pp, da, dw, dh, dp)      stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Fill2D(pa, pw, ph, pp, da, dw, dh, dp)
#endif
#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
#include "acc_1dbm.h"
#define Copy1DNoOverlapMPEG1DBlockMove(sa, sw, sh)                  stavmem_MPEG1DBlockMove.Execute.Copy1DNoOverlap(sa, sw, sh)
#define CheckCopy1DNoOverlapMPEG1DBlockMove(sa, sw, sh)             stavmem_MPEG1DBlockMove.CheckIfCanMakeIt.Copy1DNoOverlap(sa, sw, sh)
#define Copy2DNoOverlapMPEG1DBlockMove(sa, sw, sh, sp, da, dp)      stavmem_MPEG1DBlockMove.Execute.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
#define CheckCopy2DNoOverlapMPEG1DBlockMove(sa, sw, sh, sp, da, dp) stavmem_MPEG1DBlockMove.CheckIfCanMakeIt.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
#endif
#endif /* #ifdef STAVMEM_TEST_DIRECT_PICK */

/*#define FlushDCache  {                                              \*/
/*    STSYS_WriteRegDev8((void *) 0x4600, 1);                         \*/
/*    while ((STSYS_ReadRegDev8((void *) 0x4900) & 0x10) != 0) { ; }  \*/
/*}*/
#define FlushDCache {}

/*******************************************************************************
Name        : TestAccessEndianness
Description : test endianness of the memory access at a given base address
Parameters  : AreaName (for display) and pointer to memory address
Assumptions : Pointer is not NULL
Limitations :
Returns     : FALSE (success)
*******************************************************************************/
static BOOL TestAccessEndianness(const char* AreaName_p, void *BaseAddr_p)
{
    U32 *WriteAddress32_p;
    U8 Value8;
    char Endianness_p[21];
    UNUSED_PARAMETER(AreaName_p);

    WriteAddress32_p = (U32 *)((U32)(IFV2CPU(BaseAddr_p))&~(0x3)); /* manual alignment neaded for st40 ,
                                                                    * this is not normal as it should be done by the toolset  */

    /* Write a value on 32 bits */
    STTBX_Print(("   write the value 0x44332211 in area %s at address=0x%x\n",
            AreaName_p, BaseAddr_p));
    *WriteAddress32_p = 0x44332211;

    /* Read the value on 8 bits */
    Value8 = *((U8 *) WriteAddress32_p);
    STTBX_Print(("   value read at 8bit address=0x%x is 0x%#x\n", WriteAddress32_p, Value8));
    if (Value8 == 0x11)
    {
        /* Got LSB: little endianness */
        strcpy(Endianness_p, "little endian, test ok");
    }
    else if (Value8 == 0x44)
    {
        /* Got MSB: big endianness */
        strcpy(Endianness_p, "big endian, test failed !");
    }
    else
    {
        /* Should not happen, but... */
        strcpy(Endianness_p, "error in detection !");
    }

    STTBX_Print(("   result : %s \n", Endianness_p));
    STTBX_Print(("\n"));

    return(FALSE);
}


/*******************************************************************************
Name        : TestAccessAlignment
Description : test alignement of the copy into memory at a given base address
Parameters  : Area name (for display), pointer to memory address, and offsets for write/read
Assumptions : Pointers are not NULL
Limitations :
Returns     : FALSE (success)
*******************************************************************************/
static BOOL TestAccessAlignment(const char* AreaName_p, void *BaseAddr_p, const U32 OffsetWrite, const U32 OffsetRead)
{
    U8 *WriteAddr_p;
    void *ReadAddr_p;
    U32 ReadValue32 = 0;
#ifdef STAVMEM_READ_WRITE_MEM_FUNCTIONS
    U32 ReadValueAvmem = 0;
#endif
    U16 ReadValue16 = 0;
    U8 ReadValue8 = 0;
    U8 WriteValue8;
    int i = 0;
#ifndef STAVMEM_NO_COPY_FILL
    U8 empty[1] = {0};
    UNUSED_PARAMETER(AreaName_p);

    /* Fill 16 bytes with 0 */
    STTBX_Print(("   fill area %s with 0 at address=0x%x \n", AreaName_p, BaseAddr_p));

    STAVMEM_FillBlock1D(empty, 1, BaseAddr_p, 0x10);
#endif /*STAVMEM_NO_COPY_FILL*/


    /* Write into memory */
    WriteAddr_p = (U8 *) IFV2CPU(( ((U8 *) (BaseAddr_p)) + OffsetWrite));
    STTBX_Print(("   write values 0x11 0x22 0x33 ... 0x88 at 8bit address=0x%x\n", WriteAddr_p ));
    WriteValue8 = 0x11;
    for (i = 0; i < 8; i++) {
        *(WriteAddr_p++) = WriteValue8;     /*Write 11-22-33-44-55-66-77-88*/
        WriteValue8 += 0x11;
    }

    /* Display what's in memory */
    DumpMemory(((U8 *) BaseAddr_p), DUMP_1_LINE);
    /* Read in memory with different pointers */
    ReadAddr_p = (U8 *) IFV2CPU(( ((U8 *) (BaseAddr_p)) + OffsetRead));
    ReadValue32 = *((U32 *) ((U32)ReadAddr_p & (~0x3)));       /*Read 32 bit value*/
                                                               /* manual alignment neaded for st40 ,
                                                               * this is not normal as it should be done by the toolset  */

    ReadValue16 = *((U16 *) ((U32)ReadAddr_p & (~0x1)));       /*Read 16 bit value*/
                                                               /* manual alignment neaded for st40 ,
                                                                * this is not normal as it should be done by the toolset  */

    ReadValue8 = *((U8 *) ReadAddr_p);       /*Read 8 bit value*/

#ifdef STAVMEM_READ_WRITE_MEM_FUNCTIONS
    ReadValueAvmem = STAVMEM_ReadMem32(ReadAddr_p);    /*Read 32 bit value*/
#endif

    /* Display reading results */
    STTBX_Print(("   read byte %d from address=0x%x\n", OffsetRead, BaseAddr_p ));
    STTBX_Print(("   *(U32 *) address gives %#x\n", ReadValue32));
    STTBX_Print(("   *(U16 *) address gives %#x\n", ReadValue16));
    STTBX_Print(("    *(U8 *) address gives %#x\n", ReadValue8));
    if (ReadValue32==0x44332211 && ReadValue16==0x4433 && ReadValue8==0x44)
    {
        STTBX_Print(("   result : ok\n"));
    }
    else
    {
        STTBX_Print(("   result : unexpected read value !\n"));
    }

#ifdef STAVMEM_READ_WRITE_MEM_FUNCTIONS
    STTBX_Print((" STAVMEM_ReadMem32 gives %#x\n", ReadValueAvmem));
#endif
    STTBX_Print(("\n"));

    return(FALSE);
}



static U32 AccessCacheDisplayLine(U8 DeviceValue, U8 PointerValue, U8 ExpectedValue)
{
    U32 Err = 0;

    STTBX_Print(("0x%02x", DeviceValue));
    if (DeviceValue != ExpectedValue)
    {
        STTBX_Print((" (Error CC)"));
        Err++;
    }
    else
    {
        STTBX_Print(("           "));
    }
    STTBX_Print(("    0x%02x", PointerValue));
    if (PointerValue != ExpectedValue)
    {
        STTBX_Print((" (Error)\n"));
        Err++;
    }
    else
    {
        STTBX_Print(("\n"));
    }

    return(Err);
}


/*******************************************************************************
Name        : TestAccessCache
Description : test cache accesses
Parameters  :
Assumptions : Pointers are not NULL
Limitations :
Returns     : FALSE (success)
*******************************************************************************/
static BOOL TestAccessCache(U8 * CachedAddr_p)
{

    U8 Old8;
    U8 Rdev8, Rptr8;
    U32 Err = 0;
    U8 * PhyCachedAddr_p;
#ifndef STAVMEM_NO_COPY_FILL
    U8 empty[1] = {0};

    STAVMEM_FillBlock1D(empty, 1, CachedAddr_p, 64);
#endif /*STAVMEM_NO_COPY_FILL*/

    STTBX_Print(("Testing cache accesses at cached address 0x%x:\n", CachedAddr_p));
    STTBX_Print(("                              Reading            Reading\n"));
    STTBX_Print(("                              device             normal\n"));
    STTBX_Print(("Writing:                      access:            access:\n"));

    /* Testing direct access */
    PhyCachedAddr_p = (U8 *)IFV2CPU(CachedAddr_p);

    Old8 = 0x11;
    FlushDCache;
    STSYS_WriteRegDev8(PhyCachedAddr_p, Old8);

    Rdev8 = STSYS_ReadRegDev8(PhyCachedAddr_p);
    Rptr8 = * ((U8 *) PhyCachedAddr_p);
    STTBX_Print(("0x%02x device (bypass cache)     ", Old8));
    Err += AccessCacheDisplayLine(Rdev8, Rptr8, Old8);

    /* Testing pointers access */
    Old8 = 0x22;
    *PhyCachedAddr_p = Old8;

    Rptr8 = * ((U8 *) PhyCachedAddr_p);
    FlushDCache;
    Rdev8 = STSYS_ReadRegDev8(PhyCachedAddr_p);
    STTBX_Print(("0x%02x normal (through cache)    ", Old8));
    Err += AccessCacheDisplayLine(Rdev8, Rptr8, Old8);

    /* Testing concurrent access with device accesses */
    Old8 = 0x33;
    FlushDCache;
    STSYS_WriteRegDev8(PhyCachedAddr_p, Old8);

    Rdev8 = STSYS_ReadRegDev8(PhyCachedAddr_p);
    Rptr8 = * ((U8 *) PhyCachedAddr_p);
    STTBX_Print(("0x%02x device (bypass cache)     ", Old8));
    Err += AccessCacheDisplayLine(Rdev8, Rptr8, Old8);

    /* Testing concurrent access with device accesses */
    Old8 = 0x44;
    *PhyCachedAddr_p = Old8;

    Rptr8 = * ((U8 *) PhyCachedAddr_p);
    FlushDCache;
    Rdev8 = STSYS_ReadRegDev8(PhyCachedAddr_p);
    STTBX_Print(("0x%02x normal (through cache)    ", Old8));
    Err += AccessCacheDisplayLine(Rdev8, Rptr8, Old8);

#ifdef STAVMEM_TEST_DIRECT_PICK
#ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
    /* Testing concurrent access with block move peripheral */
    Old8 = 0x55;
    FlushDCache;
    STSYS_WriteRegDev8((void *) ((U8 *)PhyCachedAddr_p + 16), Old8);
    Copy1DNoOverlapBlockMovePeripheral((void *) ((U8 *)CachedAddr_p + 16), CachedAddr_p, 11);

    Rdev8 = STSYS_ReadRegDev8(PhyCachedAddr_p);
    Rptr8 = * ((U8 *) PhyCachedAddr_p);
    STTBX_Print(("0x%02x with BM peripheral        ", Old8));
    Err += AccessCacheDisplayLine(Rdev8, Rptr8, Old8);
#endif

#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
    /* Testing concurrent access with MPEG 2D block move */
    Old8 = 0x66;
    FlushDCache;
    STSYS_WriteRegDev8((void *) ((U8 *)PhyCachedAddr_p + 8), Old8);
    Copy2DHandleOverlapMPEG2DBlockMove((void *) ((U8 *)CachedAddr_p + 8), 8, 1, 8, CachedAddr_p, 8);

    Rdev8 = STSYS_ReadRegDev8(PhyCachedAddr_p);
    Rptr8 = * ((U8 *) PhyCachedAddr_p);
    STTBX_Print(("0x%02x with MPEG 2D Block Move   ", Old8));
    if ((((U32) PhyCachedAddr_p) >= TEST_SPEED_SDRAMc) && (((U32) PhyCachedAddr_p) <= (TEST_SPEED_SDRAMc + STAVMEM_512K - 1)))
    {
        Err += AccessCacheDisplayLine(Rdev8, Rptr8, Old8);
    }
    else
    {
        STTBX_Print(("***** Only supported in SDRAM *****\n"));
    }
#endif
#endif /* #ifdef STAVMEM_TEST_DIRECT_PICK */

    /* Display error(s) status */
    if (Err == 0)
    {
        STTBX_Print(("**** SUCCESSFUL ****\n\n"));
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR !!! ****  (%d errors of cache coherence)\n\n", Err));
        return(TRUE);
    }
}

#ifdef STAVMEM_TEST_DIRECT_PICK
#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
/*******************************************************************************
Name        : Test1DBigData
Description : Test memory copy (test GNBvd05617 resolution)
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL Test1DBigData( STTST_Parse_t *Parse_p, char *ResultSymbol_p )
{
    U8 * SrcBeginAddr_p;
    U8 * SrcEndAddr_p;
    U32  Size;

    U8 * DestBeginAddr_p;
    U8 * DestEndAddr_p;

    BOOL CopyPassed = TRUE;
    static BOOL init = FALSE;

    STTBX_Print(("---------------------------------------------------\n"));
    STTBX_Print(("Starting test of Copy 1D with data amount > 65536*8\n"));

    /* in SDRAM */
    SrcBeginAddr_p = (U8 *)(0xC0000003);
    SrcEndAddr_p = (U8 *)(0xC01FF17D);

    /* Size is set so whole data can be contained inside virtual window */
    Size = (U32)(SrcEndAddr_p) - (U32)(SrcBeginAddr_p);

    DestBeginAddr_p =  (U8 *)(0xC0200003);
    DestEndAddr_p = (U8 * )((U32)(DestBeginAddr_p) + Size);

    memset(V2CPU(SrcBeginAddr_p), 0x11, Size);
    *((U8 *)V2CPU(SrcBeginAddr_p) - 1) = 0x22;
    *((U8 *)V2CPU(SrcEndAddr_p)) = 0x22;

    memset(V2CPU(DestBeginAddr_p), 0xEE, Size);
    *((U8 *)V2CPU(DestBeginAddr_p) - 1) = 0xFF;
    *((U8 *)V2CPU(DestEndAddr_p)) = 0xFF;

    if (!init)
    {
        stavmem_MPEG1DBlockMove.Init();
        init = TRUE;
    }
    Copy1DNoOverlapMPEG1DBlockMove((void *)(SrcBeginAddr_p), (void *)(DestBeginAddr_p), Size);

    STTBX_Print(("%d bytes data buffer copied. Checking copy results...\n", Size));

    if (*((U8 *)V2CPU(DestBeginAddr_p) - 1) != 0xFF)
    {
        CopyPassed = FALSE;
    }

    if (*((U8 *)V2CPU(DestEndAddr_p)) != 0xFF)
    {
        CopyPassed = FALSE;
    }

    CopyPassed = memcmp(V2CPU(SrcBeginAddr_p), V2CPU(DestBeginAddr_p), Size)==0;

    if (!CopyPassed)
    {
        STTBX_Print(("**** Test FAILED !!! ****\n"));
    }
    else
    {
        STTBX_Print(("---- No error in tests of big data amount 1D copy ---- \n"));
    }
    STTBX_Print(("---------------------------------------------------\n"));

    return(!CopyPassed);
}
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE */

#if (defined(STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE) || defined(STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE))
/*******************************************************************************
Name        : TestAccessSDRAM
Description : test cache accesses
Parameters  :
Assumptions : Pointers are not NULL
Limitations :
Returns     : FALSE (success), TRUE (error)
*******************************************************************************/
static BOOL TestAccessSDRAM(U8 * SourceAddr_p, U8 * BaseAddr_p)
{
    U32 i, j;
#ifndef STAVMEM_NO_COPY_FILL
    U8 empty[1] = {0};
    U8 pattern[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
#endif  /*STAVMEM_NO_COPY_FILL*/
    U32 Err = 0;
    U8 * PhySourceAddr_p;
    U8 * PhyBaseAddr_p;

    /* shared memory is CPU accessed, so if address is in virtual range, check it can be accessed from CPU */
    if (   ((ISVIR(SourceAddr_p)) && !(ISINVW(SourceAddr_p,  64)))
        || ((ISVIR(BaseAddr_p)) && !(ISINVW(BaseAddr_p, 64))))
    {
        STTBX_Print(("****  ERROR(S) !!! **** data range overlaps virtual window frontier \n"));
        return(TRUE);
    }
#ifndef STAVMEM_NO_COPY_FILL
    /* Fill source with known pattern */
    STAVMEM_FillBlock1D(pattern, 5, SourceAddr_p, 64);

    /* Fill dest with 0 */
    STAVMEM_FillBlock1D(empty, 1, BaseAddr_p, 64);
#endif  /*STAVMEM_NO_COPY_FILL*/

    STTBX_Print(("Testing access to SDRAM with MPEG block move: 0x%x to 0x%x - ", SourceAddr_p, BaseAddr_p));
    DumpSourceAnDest((U32)SourceAddr_p, 0, (U32)BaseAddr_p, 0, BEFORE);

#if defined STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
    Copy2DNoOverlapMPEG2DBlockMove(SourceAddr_p, 12, 3, 20, BaseAddr_p, 20);
#elif defined STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
    Copy2DNoOverlapMPEG1DBlockMove(SourceAddr_p, 12, 3, 20, BaseAddr_p, 20);
#endif

    /* Verify good copy */
    PhySourceAddr_p = (U8 *)IFV2CPU(SourceAddr_p);
    PhyBaseAddr_p   = (U8 *)IFV2CPU(BaseAddr_p);
    for (i = 0; i < 12; i++)
    {
        for (j = 0; j < 3; j++)
        {
            if (PhySourceAddr_p[i + (j * 20)] != PhyBaseAddr_p[i + (j * 20)])
            {
                /* Error case: bad copy */
                Err++;
            }
        }
    }

    /* Display error(s) status */
    if (Err == 0)
    {
        STTBX_Print(("OK.\n"));
        DumpSourceAnDest((U32)SourceAddr_p, 0, (U32)BaseAddr_p, 0, AFTER);
        return(FALSE);
    }
    else
    {
        STTBX_Print(("****  ERROR(S) !!! **** (%d)\n", Err));
        DumpSourceAnDest((U32)SourceAddr_p, 0, (U32)BaseAddr_p, 0, FAILURE);
        return(TRUE);
    }
}
#endif /* #if (defined(STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE) || defined(STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE)) */
#endif /* #ifdef STAVMEM_TEST_DIRECT_PICK */

/*******************************************************************************
Name        : DumpSourceAnDest
Description : Dump memory on source and destination,
Parameters  : Source and destination locations
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void DumpSourceAnDest(U32 SrcPhyAddr, U32 SrcOffset, U32 DestPhyAddr, U32 DestOffset, SrcAndDestDump_t DumpType)
{
    char DumpTypeText[20];
    S32 Dumpmem;

    STTST_EvaluateInteger("memdump", &Dumpmem, 16);

    if (Dumpmem == 0)
    {
        return;
    }

    switch (DumpType)
    {
        case BEFORE:
            STTBX_Print(("\nDump start of memories before copy\n"));
            strcpy(DumpTypeText, "before copy");
            break;
        case AFTER:
            strcpy(DumpTypeText, "after copy");
            break;
        case FAILURE:
            strcpy(DumpTypeText, "where failed");
            break;
        default :
            strcpy(DumpTypeText, "????????????");
            break;
    } /* switch (DumpType) */

    STTBX_Print(("\nSource location %s: Addr: 0x%09x Offset %d\n", DumpTypeText, SrcPhyAddr, SrcOffset));
    DumpMemory((((U8 *)SrcPhyAddr) + SrcOffset), DUMP_6_LINES);

    STTBX_Print(("\n"));

    STTBX_Print(("Destination location %s: Addr: 0x%09x Offset %d\n", DumpTypeText, DestPhyAddr, DestOffset));
    DumpMemory((((U8 *)DestPhyAddr) + DestOffset), DUMP_6_LINES);

} /* DumpSourceAnDest() */

/*******************************************************************************
Name        : Check1DCopy
Description : Check 1D copy has succeed
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE: ok. FALSE: control failed.
*******************************************************************************/
static BOOL Check1DCopy(U32 SrcPhyAddr, U32 SrcWidth, U32 DestPhyAddr, AccessSystem_t ASystem)
{
    U32 i;
    BOOL IsCopyOk = TRUE;
    UNUSED_PARAMETER(ASystem);


    for (i = 0; (i < SrcWidth) && IsCopyOk; i++)
    {
        if (*(((U8 *)SrcPhyAddr) + i) != *(((U8 *)DestPhyAddr) + i))
        {
            IsCopyOk = FALSE;
        }
    }
    for (i=0 ; (i< BEYOND_DEST_END_CHECK_SIZE) && IsCopyOk; i++)
    {
        if (*(((U8 *)DestPhyAddr) + SrcWidth + i) != BEYOND_DEST_END_CHECK_VALUE)
        {
            IsCopyOk = FALSE;
            i += SrcWidth; /* for dump */
        }
    }
    if (IsCopyOk)
    {
        STTBX_Print(("--> %s 1D copy checked ok\n", ASystemName[ASystem]));
        DumpSourceAnDest(SrcPhyAddr, 0, DestPhyAddr, 0, AFTER);
    }
    else
    {
        STTBX_Print(("\n **** %s 1D copy failed !!! ****\n", ASystemName[ASystem]));
        DumpSourceAnDest(SrcPhyAddr, i, DestPhyAddr, i, FAILURE);
    }
    return(IsCopyOk);
} /* Check1DCopy() */

/*******************************************************************************
Name        : Check2DCopy
Description : Check 2D copy has succeed
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE: ok. FALSE: control failed.
*******************************************************************************/
static BOOL Check2DCopy(U32 SrcPhyAddr,  U32 SrcWidth,  U32 SrcHeight,  U32 SrcPitch,
                        U32 DestPhyAddr, U32 DestPitch, AccessSystem_t ASystem)
{
    U32 i, j;
    U32 srccount, destcount, SrcFootPrintSize, DestFootPrintSize;
    BOOL IsCopyOk = TRUE;
    BOOL IsRangesOverlap;
    UNUSED_PARAMETER(ASystem);

    SrcFootPrintSize  = SrcWidth + (SrcHeight - 1) * SrcPitch;
    DestFootPrintSize = SrcWidth + (SrcHeight - 1) * DestPitch;

    IsRangesOverlap = RangesOverlap(SrcPhyAddr, ((U8 *) SrcPhyAddr) + SrcFootPrintSize - 1, DestPhyAddr, ((U8 *) DestPhyAddr) + SrcFootPrintSize - 1);

    for (j=0 ; (j<SrcHeight) && IsCopyOk; j++)
    {
        for (i=0 ; (i<SrcWidth) && IsCopyOk; i++)
        {
            srccount = i + j*SrcPitch ;
            destcount = i + j*DestPitch ;
            /* if no overlap, do check source and destination are equals */
            if (!IsRangesOverlap)
            {
                if (*(((U8 *)SrcPhyAddr) + srccount) != *(((U8 *)DestPhyAddr) + destcount))
                {
                    IsCopyOk = FALSE;
                }
            }
            else /* if overlap, check destination equals what has been written to source before */
            {
                if (*(((U8 *)DestPhyAddr) + destcount) != (U8)(srccount % 0x100))
                {
                    IsCopyOk = FALSE;
                }
            } /* if (!IsRangesOverlap) */
        } /* for (i=0 ; */
    } /* for (j=0 ;  */

    for (i=0 ; (i< Min(BEYOND_DEST_END_CHECK_SIZE, DestPitch - SrcWidth)) && IsCopyOk; i++)
    {
        if (*(((U8 *)DestPhyAddr) + SrcWidth + i) != BEYOND_DEST_END_CHECK_VALUE)
        {
            IsCopyOk = FALSE;
            destcount = SrcWidth + i; /* for dump */
        }
    }
    for (i=0 ; (i<BEYOND_DEST_END_CHECK_SIZE) && IsCopyOk; i++)
    {
        if (*(((U8 *)DestPhyAddr) + DestFootPrintSize + i) != BEYOND_DEST_END_CHECK_VALUE)
        {
            IsCopyOk = FALSE;
            destcount = DestFootPrintSize + i;
        }
    }

    if (IsCopyOk)
    {
        STTBX_Print(("--> %s 2D copy checked ok\n", ASystemName[ASystem]));
        DumpSourceAnDest(SrcPhyAddr, 0, DestPhyAddr, 0, AFTER);
    }
    else
    {
        STTBX_Print(("\n **** %s 2D copy failed !!! ****\n", ASystemName[ASystem]));
        DumpSourceAnDest(SrcPhyAddr, srccount, DestPhyAddr, destcount, FAILURE);
    }
    return(IsCopyOk);
} /* Check2DCopy() */


/*******************************************************************************
Name        : SetSourceControlData
Description : Write known values to source location so that copy result can be checked later
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetSourceControlData(U32 SrcPhyAddr,  U32 SrcWidth,  U32 SrcHeight, U32 SrcPitch)
{
    U32 i, j, srccount;

    for (j=0 ; j<SrcHeight; j++)
    {
        for (i=0 ; i<SrcWidth; i++)
        {
            srccount = i + j*SrcPitch ;
            *(((U8 *)SrcPhyAddr) + srccount) = (U8)(srccount % 0x100);
        }
    }
} /* SetSourceControlData() */


/*******************************************************************************
Name        : AccessSystemCopy
Description : Test memory accesses by copy method
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void AccessSystemCopy(U32 SrcPhyAddr,  U32 SrcWidth,  U32 SrcHeight, U32 SrcPitch,
                             U32 DestPhyAddr, U32 DestPitch, AccessSystem_t ASystem)
{
    U32 Size;
#ifndef STAVMEM_NO_COPY_FILL
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#endif  /*STAVMEM_NO_COPY_FILL*/
    STTBX_Print(("Request: Copy From=0x%09x To=0x%09x SrcWidth=%d SrcHeight=%d SrcPitch=%d DestPitch=%d Method= %s\n",
                          SrcPhyAddr, DestPhyAddr, SrcWidth, SrcHeight, SrcPitch, DestPitch, ASystemName[ASystem]));

    Size = SrcWidth + SrcPitch*(SrcHeight-1);
    /* fill source and dest memory so that copy checking can be done */
    FillMemoryWithRandom((void *) SrcPhyAddr, 2*Size, FALSE);
    FillMemoryWithRandom((void *) DestPhyAddr, 2*Size, FALSE);

    SetSourceControlData(SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch);

    /* fill beyond end of destination memory with identified values, to check later it has not been written to */
    memset( ((U8 *)DestPhyAddr) + SrcWidth,
             BEYOND_DEST_END_CHECK_VALUE,
             BEYOND_DEST_END_CHECK_SIZE); /* if 2D may be overwritten by next row, it's ok */

    memset( ((U8 *)DestPhyAddr) + SrcWidth + (SrcHeight - 1) * DestPitch,
            BEYOND_DEST_END_CHECK_VALUE,
            BEYOND_DEST_END_CHECK_SIZE);

    DumpSourceAnDest(SrcPhyAddr, 0, DestPhyAddr, 0, BEFORE);

    switch (ASystem)
    {
#ifndef STAVMEM_NO_COPY_FILL
        case AS_BEST_1D :
            ErrorCode = STAVMEM_CopyBlock1D((void *)SrcPhyAddr, (void *)DestPhyAddr, SrcWidth);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STAVMEM_CopyBlock1D() returned error code 0x%0x\n",ErrorCode));
                break;
            }
            Check1DCopy(SrcPhyAddr, SrcWidth, DestPhyAddr, ASystem);
            break;
        case AS_BEST_2D :
            ErrorCode = STAVMEM_CopyBlock2D((void *)SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch, (void *)DestPhyAddr, DestPitch);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STAVMEM_CopyBlock2D() returned error code 0x%0x\n",ErrorCode));
                break;
            }
            Check2DCopy(SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch, DestPhyAddr, DestPitch, ASystem);
            break;
#endif /*STAVMEM_NO_COPY_FILL*/
#ifdef STAVMEM_TEST_DIRECT_PICK
#ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
        case AS_BLK_MOVE_DMA :
            Copy1DNoOverlapBlockMovePeripheral((void *)SrcPhyAddr, (void *)DestPhyAddr, SrcWidth);
            Check1DCopy(SrcPhyAddr, SrcWidth, DestPhyAddr, ASystem);
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL */

#ifdef STAVMEM_MEM_ACCESS_FDMA
        case AS_FDMA :
            Copy1DNoOverlapFdma((void *)SrcPhyAddr, (void *)DestPhyAddr, SrcWidth);
            Check1DCopy(SrcPhyAddr, SrcWidth, DestPhyAddr, ASystem);
            Copy2DNoOverlapFdma((void *)SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch, (void *)DestPhyAddr, DestPitch);
            Check2DCopy(SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch, DestPhyAddr, DestPitch, ASystem);
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_FDMA */

#ifdef STAVMEM_MEM_ACCESS_GPDMA
        case AS_GPDMA :
            Copy1DNoOverlapGpdma((void *)SrcPhyAddr, (void *)DestPhyAddr, SrcWidth);
            Check1DCopy(SrcPhyAddr, SrcWidth, DestPhyAddr, ASystem);
            Copy2DNoOverlapGpdma((void *)SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch, (void *)DestPhyAddr, DestPitch);
            Check2DCopy(SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch, DestPhyAddr, DestPitch, ASystem);
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_GPDMA */

#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
        case AS_MPEG_1D :
            if ( CheckCopy1DNoOverlapMPEG1DBlockMove((void *)SrcPhyAddr, (void *) DestPhyAddr, SrcWidth) !=ST_NO_ERROR )
            {
                STTBX_Print(("MPEG 1D copy 1D cannot make it\n"));
                break;
            }
            Copy1DNoOverlapMPEG1DBlockMove((void *)SrcPhyAddr, (void *) DestPhyAddr, SrcWidth);
            Check1DCopy(SrcPhyAddr, SrcWidth, DestPhyAddr, ASystem);
            if ( CheckCopy2DNoOverlapMPEG1DBlockMove(
                        (void *)SrcPhyAddr,
                        SrcWidth,
                        SrcHeight,
                        SrcPitch,
                        (void *)DestPhyAddr,
                        DestPitch ) !=ST_NO_ERROR )
            {
                STTBX_Print(("MPEG 1D copy 2D cannot make it\n"));
                break;
            }
            Copy2DNoOverlapMPEG1DBlockMove(
                        (void *)SrcPhyAddr,
                        SrcWidth,
                        SrcHeight,
                        SrcPitch,
                        (void *)DestPhyAddr,
                        DestPitch );
            Check2DCopy(SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch, DestPhyAddr, DestPitch, ASystem);
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE */

#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
        case AS_MPEG_2D :
            /* 1D no overlap */
            if ( CheckCopy1DNoOverlapMPEG2DBlockMove((void *)SrcPhyAddr, (void *)DestPhyAddr, SrcWidth) !=ST_NO_ERROR )
            {
                STTBX_Print(("MPEG 2D Copy 1D No Overlap cannot make it\n"));
                break;
            }
            STTBX_Print((" MPEG 2D No Overlap\n"));
            Copy1DNoOverlapMPEG2DBlockMove((void *)SrcPhyAddr, (void *)DestPhyAddr, SrcWidth);
            Check1DCopy(SrcPhyAddr, SrcWidth, DestPhyAddr, ASystem);

            /* 1D handle overlap */
            /* as checking for overlap rely on data set on source, re-initialize it */
            SetSourceControlData(SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch);
            if ( CheckCopy1DHandleOverlapMPEG2DBlockMove((void *)SrcPhyAddr, (void *)DestPhyAddr, SrcWidth) !=ST_NO_ERROR )
            {
                STTBX_Print(("MPEG 2D Copy 1D Handle Overlap cannot make it\n"));
                break;
            }
            STTBX_Print((" MPEG 2D Handle Overlap\n"));
            Copy1DHandleOverlapMPEG2DBlockMove((void *)SrcPhyAddr, (void *)DestPhyAddr, SrcWidth);
            Check1DCopy(SrcPhyAddr, SrcWidth, DestPhyAddr, ASystem);

            /* 2D no overlap */
            if ( CheckCopy2DNoOverlapMPEG2DBlockMove(
                        (void *)SrcPhyAddr,
                        SrcWidth,
                        SrcHeight,
                        SrcPitch,
                        (void *)DestPhyAddr,
                        DestPitch ) !=ST_NO_ERROR )
            {
                STTBX_Print(("MPEG 2D Copy 2D No Overlap cannot make it\n"));
                break;
            }
            STTBX_Print((" MPEG 2D No Overlap\n"));
            Copy2DNoOverlapMPEG2DBlockMove(
                    (void *)SrcPhyAddr,
                    SrcWidth,
                    SrcHeight,
                    SrcPitch,
                    (void *)DestPhyAddr,
                    DestPitch );
            Check2DCopy(SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch, DestPhyAddr, DestPitch, ASystem);

            /* 2D handle overlap */
            /* as checking for overlap rely on data set on source, re-initialize it */
            SetSourceControlData(SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch);
            if ( CheckCopy2DHandleOverlapMPEG2DBlockMove(
                        (void *)SrcPhyAddr,
                        SrcWidth,
                        SrcHeight,
                        SrcPitch,
                        (void *)DestPhyAddr,
                        DestPitch ) !=ST_NO_ERROR )
            {
                STTBX_Print(("MPEG 2D Copy 2D Handle Overlap cannot make it\n"));
                break;
            }
            STTBX_Print((" MPEG 2D Handle Overlap\n"));
            Copy2DHandleOverlapMPEG2DBlockMove(
                    (void *)SrcPhyAddr,
                    SrcWidth,
                    SrcHeight,
                    SrcPitch,
                    (void *)DestPhyAddr,
                    DestPitch );
            Check2DCopy(SrcPhyAddr, SrcWidth, SrcHeight, SrcPitch, DestPhyAddr, DestPitch, ASystem);
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE */
#endif /* #ifdef STAVMEM_TEST_DIRECT_PICK */
        default :
            STTBX_Print(("Method not available\n"));
            break;
    } /* first switch (ASystem) */
} /* AccessSystemCopy() */


/*******************************************************************************
Name        : Check1DFill
Description : Check 1D fill has succeed
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE: ok. FALSE: control failed.
*******************************************************************************/
static BOOL Check1DFill(U32 PatPhyAddr, U32 PatWidth, U32 DestPhyAddr, U32 DestWidth, AccessSystem_t ASystem)
{
    U32 i, Pat;
    BOOL IsCopyOk = TRUE;
    UNUSED_PARAMETER(ASystem);

    Pat = 0;
    for (i = 0; (i < DestWidth) && IsCopyOk; i++)
    {
        if (*((U8 *) (((U8 *)DestPhyAddr) + i )) != *(((U8 *)PatPhyAddr) + Pat % PatWidth))
        {
            IsCopyOk = FALSE;
        }
        Pat++;
    }
    for (i=0 ; (i< BEYOND_DEST_END_CHECK_SIZE) && IsCopyOk; i++)
    {
        if (*(((U8 *)DestPhyAddr) + DestWidth + i) != BEYOND_DEST_END_CHECK_VALUE)
        {
            IsCopyOk = FALSE;
            i += DestWidth; /* for dump */
        }
    }
    if (IsCopyOk)
    {
        STTBX_Print(("--> %s 1D fill checked ok\n", ASystemName[ASystem]));
        DumpSourceAnDest(PatPhyAddr, 0, DestPhyAddr, 0, AFTER);
    }
    else
    {
        STTBX_Print(("\n **** %s 1D fill failed !!! ****\n", ASystemName[ASystem]));
        DumpSourceAnDest(PatPhyAddr, (Pat-1) % PatWidth, DestPhyAddr, i, FAILURE);
    }
    return(IsCopyOk);
} /* Check1DFill() */

/*******************************************************************************
Name        : Check2DFill
Description : Check 2D fill has succeed
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE: ok. FALSE: control failed.
*******************************************************************************/
static BOOL Check2DFill(U32 PatPhyAddr,  U32 PatWidth,  U32 PatHeight,  U32 PatPitch,
                        U32 DestPhyAddr, U32 DestWidth, U32 DestHeight, U32 DestPitch, AccessSystem_t ASystem)
{
    U32 i, j;
    U32 srccount, destcount, DestFootPrintSize;
    BOOL IsCopyOk = TRUE;
    UNUSED_PARAMETER(ASystem);

    DestFootPrintSize = DestWidth + (DestHeight - 1) * DestPitch;

    for (j = 0; (j < DestHeight) && IsCopyOk; j++)
    {
        for (i = 0; (i < DestWidth) && IsCopyOk; i++)
        {
            srccount = (i % PatWidth) + (j % PatHeight)* PatPitch;
            destcount = i + (j * DestPitch);

            if ( *((U8 *) (((U8 *)DestPhyAddr) + destcount )) != *(((U8 *)PatPhyAddr) + srccount))
            {
                IsCopyOk = FALSE;
            }
        }
        for (i=0 ; (i< Min(BEYOND_DEST_END_CHECK_SIZE, DestPitch - DestWidth)) && IsCopyOk; i++)
        {
            if (*(((U8 *)DestPhyAddr) + DestWidth + i) != BEYOND_DEST_END_CHECK_VALUE)
            {
                IsCopyOk = FALSE;
                destcount = DestWidth + i; /* for dump */
            }
        }
        for (i=0 ; (i<BEYOND_DEST_END_CHECK_SIZE) && IsCopyOk; i++)
        {
            if (*(((U8 *)DestPhyAddr) + DestFootPrintSize + i) != BEYOND_DEST_END_CHECK_VALUE)
            {
                IsCopyOk = FALSE;
                destcount = DestFootPrintSize + i; /* for dump */
            }
        }
    }
    if (IsCopyOk)
    {
        STTBX_Print(("--> %s 2D fill checked ok\n", ASystemName[ASystem]));
        DumpSourceAnDest(PatPhyAddr, 0, DestPhyAddr, 0, AFTER);
    }
    else
    {
        STTBX_Print(("\n **** %s 2D fill failed !!! ****\n", ASystemName[ASystem]));
        DumpSourceAnDest(PatPhyAddr, srccount, DestPhyAddr, destcount, FAILURE);
    }
    return(IsCopyOk);
} /* Check2DFill() */


/*******************************************************************************
Name        : AccessSystemFill
Description : Test memory accesses by copy method
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void AccessSystemFill(U32 PatPhyAddr,  U32 PatWidth,  U32 PatHeight,  U32 PatPitch,
                             U32 DestPhyAddr, U32 DestWidth, U32 DestHeight, U32 DestPitch, AccessSystem_t ASystem)
{
    U32 PatSize, DestSize;
#ifndef STAVMEM_NO_COPY_FILL
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#endif  /*STAVMEM_NO_COPY_FILL*/
    STTBX_Print(("Request: Fill with: 0x%09x, To: 0x%09x, PatWidth= %d, PatHeight= %d, PatPitch= %d, DestWidth= %d, DestHeight= %d, DestPitch= %d, Method= %s\n",
                          PatPhyAddr, DestPhyAddr, PatWidth, PatHeight, PatPitch, DestWidth, DestHeight, DestPitch, ASystemName[ASystem]));

    PatSize  = PatWidth +  PatPitch* (PatHeight-1);
    DestSize = DestWidth + DestPitch*(DestHeight-1);
    /* fill dest memory so that copy checking can be done */
    FillMemoryWithRandom((void *) DestPhyAddr, 2*DestSize, FALSE);

    SetSourceControlData(PatPhyAddr, PatWidth, PatHeight, PatPitch);

    /* fill beyond end of destination memory with identified values, to check later it has not been written to */
    memset( ((U8 *)DestPhyAddr) + DestWidth,
             BEYOND_DEST_END_CHECK_VALUE,
             BEYOND_DEST_END_CHECK_SIZE); /* if 2D may be overwritten by next row, it's ok */

    memset( ((U8 *)DestPhyAddr) + DestWidth + (DestHeight - 1) * DestPitch,
            BEYOND_DEST_END_CHECK_VALUE,
            BEYOND_DEST_END_CHECK_SIZE);

    DumpSourceAnDest(PatPhyAddr, 0, DestPhyAddr, 0, BEFORE);

    switch (ASystem)
    {
#ifndef STAVMEM_NO_COPY_FILL
        case AS_BEST_1D :
            ErrorCode = STAVMEM_FillBlock1D((void *)PatPhyAddr, PatWidth, (void *)DestPhyAddr, DestWidth);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STAVMEM_FillBlock1D() returned error code 0x%0x\n",ErrorCode));
                break;
            }
            Check1DFill(PatPhyAddr, PatWidth, DestPhyAddr, DestWidth, ASystem);
            break;

        case AS_BEST_2D :
            ErrorCode = STAVMEM_FillBlock2D((void *)PatPhyAddr,  PatWidth,  PatHeight,  PatPitch,
                                            (void *)DestPhyAddr, DestWidth, DestHeight, DestPitch);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STAVMEM_FillBlock2D() returned error code 0x%0x\n",ErrorCode));
                break;
            }
            Check2DFill(PatPhyAddr, PatWidth, PatHeight, PatPitch, DestPhyAddr, DestWidth, DestHeight, DestPitch, ASystem );
            break;
#endif /*STAVMEM_NO_COPY_FILL*/
#ifdef STAVMEM_TEST_DIRECT_PICK
#ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
        case AS_BLK_MOVE_DMA :
            Fill1DBlockMovePeripheral((void *)PatPhyAddr, PatWidth, (void *)DestPhyAddr, DestWidth);
            Check1DFill(PatPhyAddr, PatWidth, DestPhyAddr, DestWidth, ASystem);
            Fill2DBlockMovePeripheral((void *)PatPhyAddr,  PatWidth,  PatHeight,  PatPitch,
                                      (void *)DestPhyAddr, DestWidth, DestHeight, DestPitch);
            Check2DFill(PatPhyAddr, PatWidth, PatHeight, PatPitch, DestPhyAddr, DestWidth, DestHeight, DestPitch, ASystem );
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL */

#ifdef STAVMEM_MEM_ACCESS_FDMA
        case AS_FDMA :
            Fill1DFdma((void *)PatPhyAddr, PatWidth, (void *)DestPhyAddr, DestWidth);
            Check1DFill(PatPhyAddr, PatWidth, DestPhyAddr, DestWidth, ASystem);
            Fill2DFdma((void *)PatPhyAddr, PatWidth, PatHeight, PatPitch, (void *)DestPhyAddr, DestWidth, DestHeight, DestPitch);
            Check2DFill(PatPhyAddr, PatWidth, PatHeight, PatPitch, DestPhyAddr, DestWidth, DestHeight, DestPitch, ASystem );
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_FDMA */

#ifdef STAVMEM_MEM_ACCESS_GPDMA
        case AS_GPDMA :
            Fill1DGpdma((void *)PatPhyAddr, PatWidth, (void *)DestPhyAddr, DestWidth);
            Check1DFill(PatPhyAddr, PatWidth, DestPhyAddr, DestWidth, ASystem);
            Fill2DGpdma((void *)PatPhyAddr, PatWidth, PatHeight, PatPitch, (void *)DestPhyAddr, DestWidth, DestHeight, DestPitch);
            Check2DFill(PatPhyAddr, PatWidth, PatHeight, PatPitch, DestPhyAddr, DestWidth, DestHeight, DestPitch, ASystem);
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_GPDMA */

        /*  AS_MPEG_1D not supporting fill */

#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
        case AS_MPEG_2D :
            if ( CheckFill1DMPEG2DBlockMove((void *)PatPhyAddr, PatWidth, (void *)DestPhyAddr, DestWidth ) !=ST_NO_ERROR )
            {
                STTBX_Print(("MPEG 2D Fill 1D cannot make it\n"));
                break;
            }
            Fill1DMPEG2DBlockMove((void *)PatPhyAddr, PatWidth, (void *)DestPhyAddr, DestWidth );
            Check1DFill(PatPhyAddr, PatWidth, DestPhyAddr, DestWidth, ASystem);

            if ( CheckFill2DMPEG2DBlockMove(
                        (void *)PatPhyAddr,
                        PatWidth,
                        PatHeight,
                        PatPitch,
                        (void *)DestPhyAddr,
                        DestWidth,
                        DestHeight,
                        DestPitch ) !=ST_NO_ERROR )
            {
                STTBX_Print(("MPEG 2D Fill 2D cannot make it\n"));
                break;
            }
            Fill2DMPEG2DBlockMove(
                    (void *)PatPhyAddr,
                    PatWidth,
                    PatHeight,
                    PatPitch,
                    (void *)DestPhyAddr,
                    DestWidth,
                    DestHeight,
                    DestPitch );
            Check2DFill(PatPhyAddr, PatWidth, PatHeight, PatPitch, DestPhyAddr, DestWidth, DestHeight, DestPitch, ASystem);
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE */
#endif /* #ifdef STAVMEM_TEST_DIRECT_PICK */
        default :
            STTBX_Print(("Method not available\n"));
            break;
    } /* switch (ASystem) */

} /* AccessSystemFill() */


/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/


/*******************************************************************************
Name        : TestsAccess
Description : Test memory accesses
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
BOOL TestsAccess(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr = FALSE;
    S32 Var;
    BOOL Interactive;
    CopyFillChoice_t CopyFillChoice;
    U32 SrcPatPhyAddr, DestPhyAddr, SrcPatWidth, SrcPatHeight, SrcPatPitch, DestWidth, DestHeight, DestPitch, i;
    AccessSystem_t ASystem;
    char HelpText[255];
    UNUSED_PARAMETER(ResultSymbol_p);

    RetErr = STTST_GetInteger( Parse_p, 0, &Var);
    if ( (RetErr) || (Var<0) || (Var>1))
    {
        STTST_TagCurrentLine( Parse_p, "Expected Interactive [0 - 1](default 0)\n" );
        return(TRUE);
    }
    Interactive = (Var == 1);

    if (Interactive)
    {
        RetErr = STTST_GetInteger( Parse_p, 0, &Var);
        if (RetErr || (Var<=0) || (Var>2))
        {
            STTBX_Print(("Parameters: interactive - Copy(%d)/Fill(%d)\n", COPY, FILL));
            STTBX_Print(("  if Copy:   SrcPhyAddr - SrcWidth - SrcHeight - SrcPitch - DestPhyAddr - DestPitch - method\n"));
            STTBX_Print(("  if Fill:   PatPhyAddr - PatWidth - PatHeight - PatPitch - DestPhyAddr - DestWidth - DestHeight - DestPitch - method\n"));
            sprintf(HelpText,"method : \n");
            for (i=0; i < AS_NUMBER ; i++)
            {
                sprintf(HelpText,"%s       : %d:%s\n", HelpText, i, ASystemName[i]);
            }
            STTBX_Print(("%s",HelpText));
            return(TRUE);
        }
        CopyFillChoice = (U8)Var;

        RetErr = STTST_GetInteger( Parse_p, 1, &Var);
        if (RetErr || (Var==0))
        {
            STTST_TagCurrentLine( Parse_p, (CopyFillChoice==COPY) ? "Expected SrcPhyAddr\n" : "Expected PatPhyAddr\n" );
            return(TRUE);
        }
        SrcPatPhyAddr = (U32)Var;

        RetErr = STTST_GetInteger( Parse_p, 1, &Var);
        if (RetErr || (Var==0))
        {
            STTST_TagCurrentLine( Parse_p, (CopyFillChoice==COPY) ? "Expected SrcWidth\n" : "Expected PatWidth\n" );
            return(TRUE);
        }
        SrcPatWidth = (U32)Var;

        RetErr = STTST_GetInteger( Parse_p, 1, &Var);
        if (RetErr || (Var==0))
        {
            STTST_TagCurrentLine( Parse_p, (CopyFillChoice==COPY) ? "Expected SrcHeight\n" : "Expected PatHeight\n" );
            return(TRUE);
        }
        SrcPatHeight = (U32)Var;

        RetErr = STTST_GetInteger( Parse_p, 1, &Var);
        if (RetErr || (Var==0))
        {
            STTST_TagCurrentLine( Parse_p, (CopyFillChoice==COPY) ? "Expected SrcPitch\n" : "Expected PatPitch\n" );
            return(TRUE);
        }
        SrcPatPitch = (U32)Var;

        RetErr = STTST_GetInteger( Parse_p, 1, &Var);
        if (RetErr || (Var==0))
        {
            STTST_TagCurrentLine( Parse_p, "Expected DestPhyAddr\n" );
            return(TRUE);
        }
        DestPhyAddr = (U32)Var;

        if (CopyFillChoice==FILL)
        {
            RetErr = STTST_GetInteger( Parse_p, 1, &Var);
            if (RetErr || (Var==0))
            {
                STTST_TagCurrentLine( Parse_p, "Expected DestWidth\n");
                return(TRUE);
            }
            DestWidth = (U32)Var;

            RetErr = STTST_GetInteger( Parse_p, 1, &Var);
            if (RetErr || (Var==0))
            {
                STTST_TagCurrentLine( Parse_p, "Expected DestHeight\n");
                return(TRUE);
            }
            DestHeight = (U32)Var;
        } /* if (CopyFillChoice==FILL) */

        RetErr = STTST_GetInteger( Parse_p, 1, &Var);
        if (RetErr || (Var==0))
        {
            STTST_TagCurrentLine( Parse_p, "Expected DestPitch\n" );
            return(TRUE);
        }
        DestPitch = (U32)Var;

        RetErr = STTST_GetInteger( Parse_p, 0, &Var);
        if (RetErr)
        {
            STTST_TagCurrentLine( Parse_p, "Expected Method\n" );
            return(TRUE);
        }
        ASystem = (AccessSystem_t)Var;

        STTBX_Print(("Test of memory accesses by copy method...\n"));
        if (CopyFillChoice==COPY)
        {
            AccessSystemCopy(SrcPatPhyAddr, SrcPatWidth, SrcPatHeight, SrcPatPitch, DestPhyAddr, DestPitch, ASystem);
        }
        else
        {
            AccessSystemFill(SrcPatPhyAddr, SrcPatWidth, SrcPatHeight, SrcPatPitch, DestPhyAddr, DestWidth, DestHeight, DestPitch, ASystem);
        }
    }
    else
    {
        /* Test memory alignement in the different memory regions */
        STTBX_Print(("Test of memory alignement for each available partition...\n"));
        for (i=0; i < MAX_PARTITIONS; i++)
        {
            RetErr = ((TestAccessAlignment(SDRamName[i], (void *) SDRamBaseAddr[i], OFFSET_4, OFFSET_7))) || (RetErr);
        }
#ifdef DRAM_TEST
        RetErr = ((TestAccessAlignment("DRAM", (void *) DRAM_BASE_ADDRESS, OFFSET_4, OFFSET_7))) || (RetErr);
#endif

        /* Test memory endianness in the different memory regions */
        STTBX_Print(("Test of the endianness in different regions...\n"));
        for (i=0; i < MAX_PARTITIONS; i++)
        {
            RetErr = ((TestAccessEndianness(SDRamName[i], (void *) SDRamBaseAddr[i]))) || (RetErr);
        }
#if !defined(ST_OS21) && !defined(ST_OSWINCE)
        /* note : ST40 has internal memory cache, but has no internal memory for data storage */
        RetErr = ((TestAccessEndianness("SRAM", (void *) TEST_ACCESS_SRAM))) || (RetErr);
#endif
#ifdef DRAM_TEST
        RetErr = ((TestAccessEndianness("DRAM", (void *) DRAM_BASE_ADDRESS))) || (RetErr);
#endif

        /* Test memory cache accesses */
        STTBX_Print(("Test of memory cache accesses...\n"));
        RetErr = (TestAccessCache((U8 *) TEST_SPEED_SDRAMc)) || (RetErr);
#ifdef TEST_ACCESS_DRAM
        RetErr = (TestAccessCache((U8 *) TEST_ACCESS_DRAMc)) || (RetErr);
#endif /* #ifdef TEST_ACCESS_DRAM */

#if defined (STAVMEM_TEST_DIRECT_PICK) && (defined(STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE) || defined(STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE))
        /* Test access to SDRAM with MPEG 1D/2D block move */
        /* Caution: respect alignment of src and dest, as copy is performed by the MPEG block move ! */
        RetErr = ((TestAccessSDRAM((U8 *) (MPEG_SDRAM_BASE_ADDRESS + 1), (U8 *) (MPEG_SDRAM_BASE_ADDRESS + 0x101)))) || (RetErr);
        RetErr = ((TestAccessSDRAM((U8 *) (MPEG_SDRAM_BASE_ADDRESS + 3), (U8 *) (MPEG_SDRAM_BASE_ADDRESS + 0x103)))) || (RetErr);
#endif /* #if defined (STAVMEM_TEST_DIRECT_PICK) && (defined(STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE) || defined(STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE)) */

        STTBX_Print(("Test of memory accesses by copy method...\n"));
#ifdef ST_OS20
        SrcPatPhyAddr = MPEG_SDRAM_BASE_ADDRESS;
#endif
#if defined(ST_OS21) || defined(ST_OSWINCE)
 #if defined (mb376)
        SrcPatPhyAddr = AVMEM_ST40_BASE_ADDRESS; /* wrong SDRAM_BASE_ADDRESS value in mb376.h */
 #elif defined (mb411)
        SrcPatPhyAddr =0xb0000000; /*SDRAM_BASE_ADDRESS;*/
 #elif defined (mb519)
        SrcPatPhyAddr = AVMEM0_ADDRESS/*0xB8000000*/;
 #endif
#if defined (ST_5301)
        SrcPatPhyAddr = ST5301_AVMEM_BASE_ADDRESS;
#elif defined (ST_5525)
        SrcPatPhyAddr = AVMEM_BASE_ADDRESS;
#endif
#endif
        SrcPatWidth   = 32;
        SrcPatHeight  = 1;
        SrcPatPitch   = 1;
        /*DestPhyAddr   = MPEG_SDRAM_BASE_ADDRESS + 0x100000;*/
        DestPhyAddr   = SrcPatPhyAddr + 0x100000;
        DestWidth     = 1024;
        DestHeight    = 16;
        DestPitch     = 1040;
#if defined(DVD_SECURED_CHIP)
        AccessSystemCopy(SrcPatPhyAddr, SrcPatWidth, SrcPatHeight, SrcPatPitch, DestPhyAddr, DestPitch, AS_FDMA);
        AccessSystemFill(SrcPatPhyAddr, SrcPatWidth, SrcPatHeight, SrcPatPitch, DestPhyAddr, DestWidth, DestHeight, DestPitch, AS_FDMA);
#else

        for (ASystem = AS_BEST_1D; ASystem < AS_NUMBER; ASystem++)
        {
            AccessSystemCopy(SrcPatPhyAddr, SrcPatWidth, SrcPatHeight, SrcPatPitch, DestPhyAddr, DestPitch, ASystem);
        }
        for (ASystem = AS_BEST_1D; ASystem < AS_NUMBER; ASystem ++)
        {
            AccessSystemFill(SrcPatPhyAddr, SrcPatWidth, SrcPatHeight, SrcPatPitch, DestPhyAddr, DestWidth, DestHeight, DestPitch, ASystem);
        }



        /* GNBvd13397 "STAVMEM_CopyBlock2D fails at end of memory" */
        /* original @ are 0xc07ff800 to 0xc07ff820, they are translated to 1ff800 to 1ff820 for UNIFIED_MEMORY support */
        STTBX_Print(("\nCheck: GNBvd13397 'STAVMEM_CopyBlock2D fails at end of memory'\n"));
        AccessSystemCopy(SrcPatPhyAddr + 0x1ff800, 10, 1, 0x800, SrcPatPhyAddr + 0x1ff820, 0x800, AS_BEST_2D);

#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
        /* GNBvd10712 "Horizontal artefacts when Fill Block using 2DBM method "*/
        /* GNBvd14259 "STAVMEM_FillBlock1D writes to wrong address & with vertical bars"*/
        /* STAVMEM_TEST_DIRECT_PICK must be defined */
        STTBX_Print(("\nCheck: GNBvd10712 'Horizontal artefacts when Fill Block using 2DBM method'\n"));
        STTBX_Print(("Check: GNBvd14259 'STAVMEM_FillBlock1D writes to wrong address & with vertical bars'\n"));
        AccessSystemFill(SrcPatPhyAddr, 1, 1, 1, SrcPatPhyAddr + 0x100000, 1024, 16, 1040, AS_MPEG_2D);
        AccessSystemFill(SrcPatPhyAddr, 8, 1, 8, SrcPatPhyAddr + 0x100000, 1024, 16, 1024, AS_MPEG_2D);

        /* GNBvd13243 "Overlapped copy fails to copy all lines" */
        /* STAVMEM_TEST_DIRECT_PICK must be defined */
        /* original @ are 0xc07f0002 to 0xc07f080a, they are translated to 1f0002 to 1f080a for UNIFIED_MEMORY support */
        STTBX_Print(("\nCheck: GNBvd13243 'Overlapped copy fails to copy all lines'\n"));
        AccessSystemCopy(SrcPatPhyAddr + 0x1f0002, 30, 6, 1024, SrcPatPhyAddr + 0x1f080a, 1024, AS_MPEG_2D);
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE */

#ifdef STAVMEM_MEM_ACCESS_GPDMA
        /* GNBvd15894 "Still Layer bug - picture not properly displayed" */
        /* STAVMEM_TEST_DIRECT_PICK must be defined */
        STTBX_Print(("\nCheck: GNBvd15894 'Still Layer bug - picture not properly displayed'\n"));
        AccessSystemCopy(SrcPatPhyAddr + 1, 30, 7, 35, SrcPatPhyAddr + 0x100003, 63, AS_GPDMA);
#endif /* #ifdef STAVMEM_MEM_ACCESS_GPDMA */

#endif /* DVD_SECURED_CHIP */
    } /* else of if (Interactive) */

    return(RetErr);
} /* TestsAccess() */

/* End of avm_mac.c */

