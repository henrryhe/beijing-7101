/*******************************************************************************

File name : avm_spd.c

Description : tests

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
04 Apr 2002         Created from avm_test.c                          HSdLM
*******************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */

/* If defined, will test speed of move2d_all() ST20 routine for comparison */
#ifdef ST_OS20
#define ST20_MOVE_2D
#endif

/* If defined, don't add offset when testing speed: copies in same areas may overlap !
Caution: this will not work with fill methods, because fill functions return error if overlapping */
/*#define TEST_SPEEDS_OVERLAPS*/

/* Define if STAVMEM read and write 8, 16, 24, 32 bits functions exist */
/*#define STAVMEM_READ_WRITE_MEM_FUNCTIONS*/

/* Define to test FLASH speed */
/*#define TEST_SPEED_FLASH*/

#if defined(mb275) || defined(mb275_64) || defined(mb295) /* || defined(mb391)*/
#define TEST_SPEED_DRAM
#endif /* #if defined(mb275) || defined(mb275_64) || defined(mb295) */

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
#include "sttbx.h"

#include "stavmem.h"
#include "startup.h"
#include "clavmem.h"
#include "memory_map.h"
#include "avm_test.h"
#include "avm_spd.h"

/* External global variables ------------------------------------------------ */

extern unsigned char *InternalBlock_p;
extern unsigned char *NCacheBlock_p;
extern unsigned char *ExternalBlock_p;

/* Private Types ------------------------------------------------------------ */

typedef enum SrcAndDestDump_e
{
    BEFORE,
    AFTER,
    FAILURE
} SrcAndDestDump_t;

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

static U32 TicksPerSecond;
static BOOL VerifyGoodCopy;
static BOOL SDRAMncLooping;
static BOOL Aligned32;
static U32 MemoryToOffset;
static U32 Width;
static U32 Height;
static U32 DestPitch;

/************************************************************/
/* Performance tests: change here locations of memory areas */
/************************************************************/
#ifdef TEST_SPEED_DRAM
/* Address of a range of at least 512Kbytes of not cached DRAM */
#define TEST_SPEED_DRAMnc   (((U32) (NCacheBlock_p) + 0x15) & ~0xF)
/* Address of a range of at least 512Kbytes of cached DRAM */
#define TEST_SPEED_DRAMc    (((U32) (ExternalBlock_p) + 0x15) & ~0xF)
#endif /* #ifdef TEST_SPEED_DRAM */

#ifdef TEST_SPEED_FLASH
#define TEST_SPEED_FLASH_ADR    FLASH_BANK_0_BASE_ADDRESS
#endif /*#ifdef TEST_SPEED_FLASH*/

#define TEST_SPEED_SRAM     (((U32) (InternalBlock_p) + 0x15) & (U32)~0xF)
/* Address of a range of at least 512Kbytes of not cached SDRAM */
#if defined (mb400)
#define TEST_SPEED_SDRAMnc  0xC1800000   /* Non cache base address required*/
#else
#define TEST_SPEED_SDRAMnc  (SDRAM_CPU_BASE_ADDRESS + 0x100000)
#endif


/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define Min(Val1, Val2) (((Val1) < (Val2))?(Val1):(Val2))
#define Max(Val1, Val2) (((Val1) > (Val2))?(Val1):(Val2))

/* Private Function prototypes ---------------------------------------------- */

static void ReportSpeedTest(U32 Speed, BOOL EndOfLine);
static BOOL TestAllAccessSpeeds(CopyMethod_t Method, const char *MethodString);
static U8 IsCopyMethod1or2D(CopyMethod_t Method);
static U32 SpeedCopyFromTo(U32 MemoryFrom, U32 MemoryTo, CopyMethod_t Method);

/* Functions ---------------------------------------------------------------- */

#ifdef STAVMEM_TEST_DIRECT_PICK
/* In order to test speed and access of particular methods, there is the need to have their functions available.
   For that, include them as it is done in acc_best.c, except that the methods HAVE TO BE AVAILABLE here,
   there is no check . */
#ifdef STAVMEM_MEM_ACCESS_GPDMA
#include "acc_gpd.h"
#define Copy1DNoOverlapGpdma(sa, da, s)               stavmem_GPDMA.Execute.Copy1DNoOverlap(sa, da, s)
#define Copy2DNoOverlapGpdma(sa, sw, sh, sp, da, dp)  stavmem_GPDMA.Execute.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
#endif
#ifdef STAVMEM_MEM_ACCESS_FDMA
#include "acc_fdma.h"
#define Copy1DNoOverlapFdma(sa, da, s)               stavmem_FDMA.Execute.Copy1DNoOverlap(sa, da, s)
#define Copy2DNoOverlapFdma(sa, sw, sh, sp, da, dp)  stavmem_FDMA.Execute.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
#endif
#ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
#include "acc_bmp.h"
#define Copy1DNoOverlapBlockMovePeripheral(sa, da, s) stavmem_BlockMovePeripheral.Execute.Copy1DNoOverlap(sa, da, s)
#endif
#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
#include "acc_2dbm.h"
#define Copy2DHandleOverlapMPEG2DBlockMove(sa, sw, sh, sp, da, dp)      stavmem_MPEG2DBlockMove.Execute.Copy2DHandleOverlap(sa, sw, sh, sp, da, dp)
#define CheckCopy2DHandleOverlapMPEG2DBlockMove(sa, sw, sh, sp, da, dp) stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy2DHandleOverlap(sa, sw, sh, sp, da, dp)
#define Copy2DNoOverlapMPEG2DBlockMove(sa, sw, sh, sp, da, dp)          stavmem_MPEG2DBlockMove.Execute.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
#define CheckCopy2DNoOverlapMPEG2DBlockMove(sa, sw, sh, sp, da, dp)     stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy2DNoOverlap(sa, sw, sh, sp, da, dp)
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
Name        : IsCopyMethod1or2D
Description : return if copy method is 1D or 2D
Parameters  :
Assumptions :
Limitations :
Returns     : 1: 1D copy, 2: 2D copy, 0: problem
*******************************************************************************/
static U8 IsCopyMethod1or2D(CopyMethod_t Method)
{
    U8 Dimension=0;

    switch (Method)
    {
#ifndef STAVMEM_NO_COPY_FILL
        case COPY_1D :
        case FILL_1D :
#endif  /*STAVMEM_NO_COPY_FILL*/
        case C_MEM_MOVE :
        case C_MEM_COPY :
#ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
        case BLK_MOVE_DMA :
#endif /* #ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL */
#ifdef STAVMEM_MEM_ACCESS_GPDMA
        case BLK_MOVE_GPDMA_1D :
#endif /* #ifdef STAVMEM_MEM_ACCESS_GPDMA */
#ifdef STAVMEM_MEM_ACCESS_FDMA
        case BLK_MOVE_FDMA_1D :
#endif /* #ifdef STAVMEM_MEM_ACCESS_FDMA */
#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
        case MPEG_1D_BLK_MOVE_1D :
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE */
            Dimension = 1;
            break;
#ifndef STAVMEM_NO_COPY_FILL
        case COPY_2D :
        case FILL_2D :
#endif /*STAVMEM_NO_COPY_FILL*/
#ifdef ST_OS20
        case MOVE_2D_SAME_PITCH :
        case MOVE_2D_DIF_PITCH :
#endif /* ST_OS20 */
#ifdef STAVMEM_MEM_ACCESS_GPDMA
        case BLK_MOVE_GPDMA_2D_SAME_PITCH :
        case BLK_MOVE_GPDMA_2D_DIF_PITCH :
#endif /* #ifdef STAVMEM_MEM_ACCESS_GPDMA */
#ifdef STAVMEM_MEM_ACCESS_FDMA
        case BLK_MOVE_FDMA_2D_SAME_PITCH :
        case BLK_MOVE_FDMA_2D_DIF_PITCH :
#endif /* #ifdef STAVMEM_MEM_ACCESS_FDMA */
#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
        case MPEG_2D_BLK_MOVE_SAME_PITCH :
        case MPEG_2D_BLK_MOVE_DIF_PITCH :
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE */
#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
        case MPEG_1D_BLK_MOVE_SAME_PITCH :
        case MPEG_1D_BLK_MOVE_DIF_PITCH :
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE */
            Dimension = 2;
            break;
        default :
            break;
    } /* switch */
    return(Dimension);

} /* IsCopyMethod1or2D() */


/*******************************************************************************
Name        : SpeedCopyFromTo
Description : test speed of memory access functions by calling them many times
Parameters  :
Assumptions :
Limitations :
Returns     : value in Kbytes/s (success), 0 (fail)
*******************************************************************************/
static U32 SpeedCopyFromTo(U32 MemoryFrom, U32 MemoryTo, CopyMethod_t Method)
{
    U32 FromAddr, ToAddr;
    U32 Size, count, PatternSize, DestFootPrintSize;
    clock_t Before;
    U32 DelayInTicks, ComputedSpeed;

    U32 PhyFrom,PhyTo, FOff, DOff;
    U32 SrcPitch;
    U32 i;
#ifndef STAVMEM_NO_COPY_FILL
    U32 srccount, destcount;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 Pat;
    U32 j;
#endif  /*STAVMEM_NO_COPY_FILL*/
    U8 Dimension;
    BOOL IsCopyOk = TRUE;

    Size        = Height * Width; /* must be aligned on 32 bytes: ok as Width is. Also used for 1D copy/fill */
    FOff        = Aligned32 ? 0 : 3;
    DOff        = Aligned32 ? 0 : 1;
    FromAddr    = MemoryFrom + FOff;
    ToAddr      = MemoryTo + MemoryToOffset + DOff;

    if (   ((ISVIR(FromAddr)) && !(ISINVW(FromAddr, Size)))
        || ((ISVIR(ToAddr))   && !(ISINVW(ToAddr, Size))))
    {
        STTBX_Print(("****  ERROR(S) !!! **** data range overlaps virtual window frontier \n"));
        return(0);
    }

    PhyFrom = (U32)IFV2CPU((void *)FromAddr);
    PhyTo   = (U32)IFV2CPU((void *)(ToAddr));

    PatternSize = Size/16 + 1;

    DestFootPrintSize = Width + (Height - 1) * DestPitch;

    /* fill source and dest memory so that copy checking can be done */
    /* warning: assumption SrcPitch is <= DestPitch (see FootPrintSizeMax calcul for non overlapping) */
    FillMemoryWithRandom((void *) PhyFrom, Width + DestPitch*(Height-1), FALSE); /* yes, DestPitch ! */
    FillMemoryWithRandom((void *) PhyTo  , Width + DestPitch*(Height-1), FALSE);

    /* fill beyond end of destination memory with identified values, to check later it has not been written to */
    Dimension = IsCopyMethod1or2D(Method);
    switch (Dimension)
    {
        case 1 :
            memset( ((U8 *)PhyTo) + Size, BEYOND_DEST_END_CHECK_VALUE, BEYOND_DEST_END_CHECK_SIZE);
            break;
        case 2 :
            memset( ((U8 *)PhyTo) + Width, BEYOND_DEST_END_CHECK_VALUE, Min(BEYOND_DEST_END_CHECK_SIZE, DestPitch));
            memset( ((U8 *)PhyTo) + DestFootPrintSize, BEYOND_DEST_END_CHECK_VALUE, BEYOND_DEST_END_CHECK_SIZE);
            break;
        case 0 :
        default :
            break;
    } /* switch (Dimension) */

    switch (Method)
    {
#ifndef STAVMEM_NO_COPY_FILL
        case COPY_1D :
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                ErrorCode = STAVMEM_CopyBlock1D((void *)FromAddr, (void *)ToAddr, Size);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Print(("STAVMEM_CopyBlock1D() returned error code 0x%0x\n",ErrorCode));
                    return(0);
                }
            }
            break;

        case COPY_2D :
            /* warning: SrcPitch must be <= 2*Width (see FootPrintSizeMax calcul for non overlapping) */
            SrcPitch = DestPitch;
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                ErrorCode = STAVMEM_CopyBlock2D((void *)FromAddr, Width, Height, SrcPitch, (void *)ToAddr, DestPitch);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Print(("STAVMEM_CopyBlock2D() returned error code 0x%0x\n",ErrorCode));
                    return(0);
                }
            }
            break;

        case FILL_1D :
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                ErrorCode = STAVMEM_FillBlock1D((void *)FromAddr, PatternSize,
                                          (void *)ToAddr, Size);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Print(("STAVMEM_FillBlock1D() returned error code 0x%0x\n",ErrorCode));
                    return(0);
                }
            }
            break;

        case FILL_2D :
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                ErrorCode = STAVMEM_FillBlock2D((void *)FromAddr, PatternSize, 1, 0,
                                          (void *)ToAddr, Width, Height, DestPitch);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Print(("STAVMEM_FillBlock2D() returned error code 0x%0x\n",ErrorCode));
                    return(0);
                }
            }
            break;
#endif /*STAVMEM_NO_COPY_FILL*/
        case C_MEM_MOVE :
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                memmove((void *)PhyTo, (void *)PhyFrom, Size);
            }
            break;

        case C_MEM_COPY :
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                memcpy((void *)PhyTo, (void *)PhyFrom, Size);
            }
            break;

#if defined (ST_OS20) && defined(PROCESSOR_C2)
/******************************************************************************/
/*- copy C MOVE 2D function --------------------------------------------------*/
/******************************************************************************/
        case MOVE_2D_SAME_PITCH : /* spec p182 */
        case MOVE_2D_DIF_PITCH : /* spec p182 */
            /* warning: SrcPitch must be <= 2*Width (see FootPrintSizeMax calcul for non overlapping) */
            SrcPitch  = (Method == MOVE_2D_SAME_PITCH) ? DestPitch : (Width+16);

            Before = time_now() ;
            for (count = 0; count < 10; count++)
            {
                 move2d_all (
                        (void *)PhyFrom,
                        (void *)PhyTo,
                        Width,
                        Height,
                        SrcPitch,
                        DestPitch );
            }
            break;
#endif /* ST_OS20 && PROCESSOR_C2*/

#ifdef STAVMEM_TEST_DIRECT_PICK
#ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
        case BLK_MOVE_DMA : /* spec p234 */
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                Copy1DNoOverlapBlockMovePeripheral((void *) FromAddr, (void *) ToAddr, Size);
            }
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL */

#ifdef STAVMEM_MEM_ACCESS_GPDMA
        case BLK_MOVE_GPDMA_1D:
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                Copy1DNoOverlapGpdma((void *) FromAddr, (void *) ToAddr, Size);
            }
            break;
        case BLK_MOVE_GPDMA_2D_SAME_PITCH :
        case BLK_MOVE_GPDMA_2D_DIF_PITCH :
            /* warning: SrcPitch must be <= 2*Width (see FootPrintSizeMax calcul for non overlapping) */
            SrcPitch  = (Method == BLK_MOVE_GPDMA_2D_SAME_PITCH) ? DestPitch : (Width+16);

            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                Copy2DNoOverlapGpdma((void *) FromAddr, Width, Height, SrcPitch, (void *) ToAddr, DestPitch );
            }
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_GPDMA */

/******************************************************************************/
/*- copy FDMA  ---------------------------------------------------------------*/
/******************************************************************************/
#ifdef STAVMEM_MEM_ACCESS_FDMA
        case BLK_MOVE_FDMA_1D:
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                Copy1DNoOverlapFdma((void *) FromAddr, (void *) ToAddr, Size);
            }
            break;
        case BLK_MOVE_FDMA_2D_SAME_PITCH:
        case BLK_MOVE_FDMA_2D_DIF_PITCH:
            /* warning: SrcPitch must be <= 2*Width (see FootPrintSizeMax calcul for non overlapping) */
            SrcPitch  = (Method == BLK_MOVE_FDMA_2D_SAME_PITCH) ? DestPitch : (Width+16);

            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                Copy2DNoOverlapFdma((void *) FromAddr, Width, Height, SrcPitch, (void *) ToAddr, DestPitch );
            }
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_FDMA */

/******************************************************************************/
/*- copy MPEG2DBLOCKMOVE -----------------------------------------------------*/
/******************************************************************************/
#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE

/*- MPEG2D BLOCK MOVE : copy 2D same pitch ---------------------------------------------------*/
        case MPEG_2D_BLK_MOVE_SAME_PITCH : /* spec p182 */
        case MPEG_2D_BLK_MOVE_DIF_PITCH : /* spec p182 */
            /* warning: SrcPitch must be <= 2*Width (see FootPrintSizeMax calcul for non overlapping) */
            SrcPitch  = (Method == MPEG_2D_BLK_MOVE_SAME_PITCH) ? DestPitch : (Width+16);
            /* check */
            if ( CheckCopy2DNoOverlapMPEG2DBlockMove(
                        (void *)FromAddr,
                        Width,
                        Height,
                        SrcPitch,
                        (void *)ToAddr,
                        DestPitch ) !=ST_NO_ERROR )
            {
                return(0);
            }
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                Copy2DNoOverlapMPEG2DBlockMove(
                        (void *)FromAddr,
                        Width,
                        Height,
                        SrcPitch,
                        (void *)ToAddr,
                        DestPitch );
            }
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE */


/******************************************************************************/
/*- copy MPEG2DBLOCKMOVE -----------------------------------------------------*/
/******************************************************************************/
#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE

/*- MPEG1D BLOCK MOVE : copy 1D --------------------------------------------------------------*/
        case MPEG_1D_BLK_MOVE_1D :
            /* check */
            if ( CheckCopy1DNoOverlapMPEG1DBlockMove((void *) FromAddr, (void *) ToAddr, Size) !=ST_NO_ERROR )
            {
                return(0);
            }
            Before = time_now() ;
            for (count = 0; count < 10; count++)
            {
                Copy1DNoOverlapMPEG1DBlockMove((void *) FromAddr, (void *) ToAddr, Size);
            }
        break;

/*- MPEG1D BLOCK MOVE : copy 2D  ---------------------------------------------------*/
        case MPEG_1D_BLK_MOVE_SAME_PITCH : /* spec p182 */
            /* warning: SrcPitch must be <= 2*Width (see FootPrintSizeMax calcul for non overlapping) */
            SrcPitch  = (Method == MPEG_1D_BLK_MOVE_SAME_PITCH) ? DestPitch : (Width+16);
            /* check */
            if ( CheckCopy2DNoOverlapMPEG1DBlockMove(
                        (void *)FromAddr,
                        Width,
                        Height,
                        SrcPitch,
                        (void *)ToAddr,
                        DestPitch ) !=ST_NO_ERROR )
            {
                return(0);
            }
            Before = time_now();
            for (count = 0; count < 10; count++)
            {
                Copy2DNoOverlapMPEG1DBlockMove(
                        (void *)FromAddr,
                        Width,
                        Height,
                        SrcPitch,
                        (void *)ToAddr,
                        DestPitch );
            }
            break;
#endif /* #ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE */

#endif /*#ifdef STAVMEM_TEST_DIRECT_PICK*/
        default :
            break;
    }
    DelayInTicks = (U32) time_minus(time_now(), Before);
    ComputedSpeed = (Size * 10) / 1024 * TicksPerSecond / DelayInTicks;

    if (VerifyGoodCopy)
    {
        STTBX_Print(("\n From:0x%08x  To:0x%08x : Speed=%6d KBytes/s   Checking: ... ",
                       PhyFrom, PhyTo, ComputedSpeed));
#ifndef STAVMEM_NO_COPY_FILL
        /* Test if copy has been done correctly */
        switch (Method)
        {

            case FILL_1D :
                Pat = 0;
                for (count = 0; (count < Size) && (count < 65535); count++)
                {
                    if (*((U8 *) (((U8 *)PhyTo) + count)) != *(((U8 *)PhyFrom) + Pat % PatternSize))
                    {
                        IsCopyOk = FALSE;
                        BlockAndBip();
                    }
                    Pat++;
                }
                break;
            case FILL_2D :
                for (i = 0; i < Height; i++)
                {
                    for (j = 0; j < Width; j++)
                    {
                        if (*((U8 *) (((U8 *)PhyTo) + (i * DestPitch) + j)) != *(((U8 *)PhyFrom) + j % PatternSize))
                        {
                            IsCopyOk = FALSE;
                            BlockAndBip();
                        }
                    }
                }
                break;
            default : /* all copy (not fill) methods */
                switch (Dimension)
                {
                    case 1 :
                        for (count = 0; (count < Size) && (count < 65535); count++)
                        {
                            if (*(((U8 *)PhyFrom) + count) != *(((U8 *)PhyTo) + count))
                            {
                                IsCopyOk = FALSE;
                                BlockAndBip();
                            }
                        }
                        break;
                    case 2 :
                        for (j=0 ; j<Height; j++)
                        {
                            for (i=0 ; i<Width; i++)
                            {
                                srccount = i + j*SrcPitch ;
                                destcount = i + j*DestPitch ;
                                if (*(((U8 *)PhyFrom) + srccount) != *(((U8 *)PhyTo) + destcount))
                                {
                                    IsCopyOk = FALSE;
                                    BlockAndBip();
                                }
                            }
                        }
                        break;
                    case 0 :
                    default :
                        break;
                } /* switch (Dimension) */
                break;
        } /* switch (Method) */
#endif /*STAVMEM_NO_COPY_FILL*/
        switch (Dimension)
        {
            case 1 :
                for (i=0 ; i<BEYOND_DEST_END_CHECK_SIZE; i++)
                {
                    if (*(((U8 *)PhyTo) + Size + i) != BEYOND_DEST_END_CHECK_VALUE)
                    {
                        IsCopyOk = FALSE;
                        BlockAndBip();
                    }
                }
                break;
            case 2 :
                for (i=0 ; i< Min(BEYOND_DEST_END_CHECK_SIZE, DestPitch); i++)
                {
                    if (*(((U8 *)PhyTo) + Width + i) != BEYOND_DEST_END_CHECK_VALUE)
                    {
                        IsCopyOk = FALSE;
                        BlockAndBip();
                    }
                }
                for (i=0 ; i<BEYOND_DEST_END_CHECK_SIZE; i++)
                {
                    if (*(((U8 *)PhyTo) + DestFootPrintSize + i) != BEYOND_DEST_END_CHECK_VALUE)
                    {
                        IsCopyOk = FALSE;
                        BlockAndBip();
                    }
                }
                break;
            case 0 :
            default :
                IsCopyOk = FALSE;
                BlockAndBip();
                break;
        } /* switch (Dimension) */
        STTBX_Print((IsCopyOk ? "Ok" : "*** Error ! ***\n"));
    } /* if (VerifyGoodCopy) */

    /* Return value in Kbytes/s */
    return(ComputedSpeed);
} /* SpeedCopyFromTo() */

/*******************************************************************************
Name        : ReportSpeedTest
Description : Print speed test result depending only if VerifyGoodCopy is FALSE
Parameters  :
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void ReportSpeedTest(U32 Speed, BOOL EndOfLine)
{
    UNUSED_PARAMETER(Speed);
    if (!VerifyGoodCopy)
    {
        if (EndOfLine)
        {
            STTBX_Print(("%8d\n", Speed));
        }
        else
        {
            STTBX_Print(("%8d ", Speed));
        }
    }
} /* ReportSpeedTest() */



/*******************************************************************************
Name        : TestCopyFillMethod
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : FALSE (success)
*******************************************************************************/
static BOOL TestCopyFillMethod(CopyMethod_t Method, const char *MethodString)
{
    BOOL RetErr = FALSE;

    if (SDRAMncLooping)
    {
        STTBX_Print(("\n%s: ", MethodString));
        SpeedCopyFromTo(TEST_SPEED_SDRAMnc, TEST_SPEED_SDRAMnc, Method);
#if (MAX_PARTITIONS == 2)
        SpeedCopyFromTo(PARTITION1_START, PARTITION1_START, Method);
#endif
#if (MAX_PARTITIONS == 3)
        SpeedCopyFromTo(PARTITION1_START, PARTITION1_START, Method);
        SpeedCopyFromTo(PARTITION2_START, PARTITION2_START, Method);
#endif

    }
    else
    {
        RetErr = TestAllAccessSpeeds(Method, MethodString);
    }
    return(RetErr);
} /* TestCopyFillMethod */

/*******************************************************************************
Name        : TestAllAccessSpeeds
Description : test speed of memory access functions by calling them many times
Parameters  :
Assumptions :
Limitations :
Returns     : FALSE (success)
*******************************************************************************/
static BOOL TestAllAccessSpeeds(CopyMethod_t Method, const char *MethodString)
{
    BOOL RetErr = FALSE;
    UNUSED_PARAMETER(MethodString);

    STTBX_Print(("\nTesting copy with %s\n", MethodString));

#ifdef TEST_SPEED_DRAM
#ifdef STAVMEM_TESTS_CACHE_OK
/* STTBX_Print(("From:   To:   DRAMnc    DRAMc     SRAM  SDRAMnc   SDRAMc\n"));*/
STTBX_Print(("From:   To:   DRAMnc   DRAMc    SDRAMnc   SDRAMc\n"));
#else
STTBX_Print(("From:   To:   DRAMnc   SDRAMnc  \n"));
#endif
#else
#ifdef STAVMEM_TESTS_CACHE_OK
/* STTBX_Print(("From:   To:   DRAMnc    DRAMc     SRAM  SDRAMnc   SDRAMc\n"));*/
STTBX_Print(("From:   To:   SDRAMnc   SDRAMc\n"));
#elif (MAX_PARTITIONS == 2)
STTBX_Print(("From:   To:   SDRAMnc   Parti1\n"));
#elif (MAX_PARTITIONS == 3)
STTBX_Print(("From:   To:   SDRAMnc   Parti1   Parti2\n"));
#else
STTBX_Print(("From:   To:   SDRAMnc\n"));
#endif
#endif /* #ifdef TEST_SPEED_DRAM */

#ifdef TEST_SPEED_DRAM
    /*- from DRAMnc ------------------------------*/
    STTBX_Print((" DRAMnc    "));
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMnc, TEST_SPEED_DRAMnc, Method),FALSE);
#ifdef STAVMEM_TESTS_CACHE_OK
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMnc, TEST_SPEED_DRAMc, Method),FALSE);
#endif
/*    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMnc, TEST_SPEED_SRAM, Method),FALSE);*/
      ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMnc, TEST_SPEED_SDRAMnc, Method),FALSE);
#ifdef STAVMEM_TESTS_CACHE_OK
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMnc, TEST_SPEED_SDRAMc, Method),TRUE);
#else
    STTBX_Print(("\n"));
#endif

   /*- from DRAMc -------------------------------*/
#ifdef STAVMEM_TESTS_CACHE_OK
    STTBX_Print((" DRAMc     "));
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMc, TEST_SPEED_DRAMnc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMc, TEST_SPEED_DRAMc, Method),FALSE);
#endif
/*    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMc, TEST_SPEED_SRAM, Method),FALSE);*/
#ifdef STAVMEM_TESTS_CACHE_OK
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMc, TEST_SPEED_SDRAMnc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_DRAMc, TEST_SPEED_SDRAMc, Method),TRUE);
#endif
#endif /* #ifdef TEST_SPEED_DRAM */

#ifdef TEST_SPEED_FLASH
    /*- from FLASH -------------------------------*/

    STTBX_Print((" FLASH     "));
#ifdef TEST_SPEED_DRAM
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_FLASH_ADR, TEST_SPEED_DRAMnc, Method),FALSE);
#ifdef STAVMEM_TESTS_CACHE_OK
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_FLASH_ADR, TEST_SPEED_DRAMc, Method),FALSE);
#endif
#endif /* #ifdef TEST_SPEED_DRAM */

/*   ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_FLASH_ADR, TEST_SPEED_SRAM, Method),FALSE);*/

    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_FLASH_ADR, TEST_SPEED_SDRAMnc, Method),FALSE);
#ifdef STAVMEM_TESTS_CACHE_OK
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_FLASH_ADR, TEST_SPEED_SDRAMc, Method),TRUE);
#else
    STTBX_Print(("\n"));
#endif
#endif /* #ifdef TEST_SPEED_FLASH */


    /*- from SRAM --------------------------------*/
/*    STTBX_Print((" SRAM      "));
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SRAM, TEST_SPEED_DRAMnc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SRAM, TEST_SPEED_DRAMc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SRAM, TEST_SPEED_SRAM, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SRAM, TEST_SPEED_SDRAMnc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SRAM, TEST_SPEED_SDRAMc, Method),TRUE);*/

    /*- from SDRAMnc -----------------------------*/
    STTBX_Print((" SDRAMnc   "));

#ifdef TEST_SPEED_DRAM
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMnc, TEST_SPEED_DRAMnc, Method),FALSE);
#ifdef STAVMEM_TESTS_CACHE_OK
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMnc, TEST_SPEED_DRAMc, Method),FALSE);
#endif
#endif /* #ifdef TEST_SPEED_DRAM */

/*    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMnc, TEST_SPEED_SRAM, Method),FALSE);*/
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMnc, TEST_SPEED_SDRAMnc, Method),FALSE);
#if (MAX_PARTITIONS == 2)
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMnc, PARTITION1_START, Method),FALSE);
#endif
#if (MAX_PARTITIONS == 3)
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMnc, PARTITION1_START, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMnc, PARTITION2_START, Method),FALSE);
#endif


#ifdef STAVMEM_TESTS_CACHE_OK
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMnc, TEST_SPEED_SDRAMc, Method),TRUE);
#else
    STTBX_Print(("\n"));
#endif

#if (MAX_PARTITIONS == 2)
    /*- from PARTITION 1 -----------------------------*/
    STTBX_Print((" Parti 1   "));
    ReportSpeedTest(SpeedCopyFromTo(PARTITION1_START, TEST_SPEED_SDRAMnc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(PARTITION1_START, PARTITION1_START, Method),TRUE);
#endif

#if (MAX_PARTITIONS == 3)
    /*- from PARTITION 1 -----------------------------*/
    STTBX_Print((" Parti 1   "));
    ReportSpeedTest(SpeedCopyFromTo(PARTITION1_START, TEST_SPEED_SDRAMnc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(PARTITION1_START, PARTITION1_START, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(PARTITION1_START, PARTITION2_START, Method),TRUE);

    /*- from PARTITION 2 -----------------------------*/
    STTBX_Print((" Parti 2   "));
    ReportSpeedTest(SpeedCopyFromTo(PARTITION2_START, TEST_SPEED_SDRAMnc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(PARTITION2_START, PARTITION1_START, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(PARTITION2_START, PARTITION2_START, Method),TRUE);
#endif


    /*- from SDRAMc ------------------------------*/
#ifdef STAVMEM_TESTS_CACHE_OK
    STTBX_Print((" SDRAMc    "));
#ifdef TEST_SPEED_DRAM
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMc, TEST_SPEED_DRAMnc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMc, TEST_SPEED_DRAMc, Method),FALSE);
#endif /* #ifdef TEST_SPEED_DRAM */
#endif
/*    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMc, TEST_SPEED_SRAM, Method),FALSE);*/
#ifdef  STAVMEM_TESTS_CACHE_OK
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMc, TEST_SPEED_SDRAMnc, Method),FALSE);
    ReportSpeedTest(SpeedCopyFromTo(TEST_SPEED_SDRAMc, TEST_SPEED_SDRAMc, Method),TRUE);
#endif

    return(RetErr);
} /* TestAllAccessSpeeds() */

/*******************************************************************************
Name        : TestAllMethods
Description : Test memory access functions speed in different memory regions
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL TestAllMethods()
{
    U32 WidthMin, WidthMax, HeightMax, FootPrintSizeMax;
    BOOL RetErr = FALSE;

    /* To avoid overlapping if TEST_SPEEDS_OVERLAPS not set, following rule must be respected:
     * MemoryFrom + Size < MemoryTo + MemoryToOffset
     * */
#ifdef TEST_SPEEDS_OVERLAPS
    MemoryToOffset    = 0;
    FootPrintSizeMax  = STAVMEM_512K;
#else
    MemoryToOffset    = STAVMEM_512K/2; /* must be aligned on 32 bytes */
    FootPrintSizeMax  = MemoryToOffset;
#endif
    WidthMin  = 512; /* arbitrary, but enough to be relevant for speed. must be aligned on 32 bytes. */

    /* Set random starting point (ANSI) */
    srand((unsigned int)time_now());

    /* FootPrintSize = Width + (Height-1)*Pitch
     * so with arbitrary Pitch = 2*Width max
     * FootPrintSizeMax = WidthMax + (Height-1)*2*WidthMax
     *
     * WidthMax > WidthMin implies HeightMax = (1+(FootPrintSizeMax/WidthMin))/2 */
    HeightMax = (1+(FootPrintSizeMax/WidthMin))/2;
    Height    = 1 + GetRandomBetween1And(HeightMax-1);                             /* must be >1 for 2D */

    WidthMax  = FootPrintSizeMax/(2*Height-1);
    Width     = (WidthMin-32) + 32 * GetRandomBetween1And((WidthMax-WidthMin)/32); /* must be aligned on 32 bytes. */
    DestPitch = 2*Width; /* warning: HeightMax and WidthMax are calculated with this assumption */

    STTBX_Print((" MemoryToOffset=0x%08x  Height=%6d  Width=%6d  Size=%6d  Aligned 32 bytes=%s (speed in KBytes/s): \n",
                   MemoryToOffset, Height, Width, Height*Width, Aligned32 ? "Yes" : "No"));

    /* Test speed from/to different areas */
#ifndef STAVMEM_NO_COPY_FILL
    RetErr = ((TestCopyFillMethod(COPY_1D, "Copy 1D\t\t\t\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(COPY_2D, "Copy 2D\t\t\t\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(FILL_1D, "Fill 1D\t\t\t\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(FILL_2D, "Fill 2D\t\t\t\t\t"))) || (RetErr);
#endif /*STAVMEM_NO_COPY_FILL*/
    RetErr = ((TestCopyFillMethod(C_MEM_MOVE, "MemMove\t\t\t\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(C_MEM_COPY, "MemCopy\t\t\t\t\t"))) || (RetErr);

#ifdef STAVMEM_TEST_DIRECT_PICK
#ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
    RetErr = ((TestCopyFillMethod(BLK_MOVE_DMA, "Block Move DMA\t\t\t"))) || (RetErr);
#endif
#ifdef STAVMEM_MEM_ACCESS_FDMA
    RetErr = ((TestCopyFillMethod(BLK_MOVE_FDMA_1D, "FDMA 1D\t\t\t\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(BLK_MOVE_FDMA_2D_SAME_PITCH, "FDMA 2D same Pitch\t\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(BLK_MOVE_FDMA_2D_DIF_PITCH, "FDMA 2D different Pitch\t\t\t"))) || (RetErr);
#endif
#ifdef STAVMEM_MEM_ACCESS_GPDMA
    RetErr = ((TestCopyFillMethod(BLK_MOVE_GPDMA_1D, "GPDMA 1D\t\t\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(BLK_MOVE_GPDMA_2D_SAME_PITCH, "GPDMA 2D same Pitch\t\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(BLK_MOVE_GPDMA_2D_DIF_PITCH, "GPDMA 2D different Pitch\t\t"))) || (RetErr);
#endif
#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
    RetErr = ((TestCopyFillMethod(MPEG_2D_BLK_MOVE_SAME_PITCH, "MPEG 2D Block Move same Pitch\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(MPEG_2D_BLK_MOVE_DIF_PITCH, "MPEG 2D Block Move different Pitch\t"))) || (RetErr);

#endif

#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
    RetErr = ((TestCopyFillMethod(MPEG_1D_BLK_MOVE_1D, "MPEG 1D Block Move 1D "))) || (RetErr);
    RetErr = ((TestCopyFillMethod(MPEG_1D_BLK_MOVE_SAME_PITCH, "MPEG 1D Block Move same Pitch\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(MPEG_1D_BLK_MOVE_DIF_PITCH, "MPEG 1D Block Move different Pitch\t"))) || (RetErr);
#endif
#endif /* #ifdef STAVMEM_TEST_DIRECT_PICK */
#ifdef ST20_MOVE_2D
    RetErr = ((TestCopyFillMethod(MOVE_2D_SAME_PITCH, "move 2D all same pitch\t\t\t"))) || (RetErr);
    RetErr = ((TestCopyFillMethod(MOVE_2D_DIF_PITCH, "move 2D all different pitch\t\t"))) || (RetErr);
#endif /* ST20_MOVE_2D */
    STTBX_Print(("\n"));
    return(RetErr);
}


/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

/*******************************************************************************
Name        : TestsSpeed
Description : Test memory access functions speed in different memory regions
Parameters  : testtool standard arguments
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
BOOL TestsSpeed(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr = FALSE;
    char dummychar = 0;
    S32 Var;
    U32 LoopIndex=0;
    UNUSED_PARAMETER(ResultSymbol_p);

    TicksPerSecond = ST_GetClocksPerSecond();

    RetErr = STTST_GetInteger( Parse_p, 0, &Var);
    if ( (RetErr) || (Var<0) || (Var>1))
    {
        STTST_TagCurrentLine( Parse_p, "Expected VerifyCopy [0 - 1](default 0)" );
        return(TRUE);
    }
    VerifyGoodCopy = (Var==1);
#ifdef TEST_SPEEDS_OVERLAPS
    if (VerifyGoodCopy)
    {
        STTBX_Print(("Speed tests verifying copy is only for NON overlapping, verifying turned off\n"));
        VerifyGoodCopy = FALSE;
    }
#else
    if (VerifyGoodCopy)
    {
        STTBX_Print(("Speed tests verifying copy (only for NON overlapping)\n"));
        /* Fill memory with random numbers */
        STTBX_Print(("  in SDRAM       : "));
        FillMemoryWithRandom((void *) PARTITION0_START, PARTITION0_STOP - PARTITION0_START + 1, TRUE);
#if (MAX_PARTITIONS == 2)
        STTBX_Print(("  in Partition 1 : "));
        FillMemoryWithRandom((void *) PARTITION1_START, PARTITION1_STOP - PARTITION1_START + 1, TRUE);
#endif
#if (MAX_PARTITIONS == 3)
        STTBX_Print(("  in Partition 1 : "));
        FillMemoryWithRandom((void *) PARTITION1_START, PARTITION1_STOP - PARTITION1_START + 1, TRUE);

        STTBX_Print(("  in Partition 2 : "));
        FillMemoryWithRandom((void *) PARTITION2_START, PARTITION2_STOP - PARTITION2_START + 1, TRUE);
#endif

    }
#endif /* #ifdef TEST_SPEEDS_OVERLAPS */
    else
    {
        STTBX_Print(("Speed tests NOT verifying copy, therefore copy may be not accurate.\n"));
    }

    RetErr = STTST_GetInteger( Parse_p, 0, &Var);
    if ( (RetErr) || (Var<0) || (Var>1))
    {
        STTST_TagCurrentLine( Parse_p, "Expected SDRAMnc looping [0 - 1](default 0)" );
        return(TRUE);
    }
    SDRAMncLooping = (Var==1);

    RetErr = STTST_GetInteger( Parse_p, 0, &Var);
    if ( (RetErr) || (Var<0) || (Var>1))
    {
        STTST_TagCurrentLine( Parse_p, "Expected 32 bytes align [0 - 1](default 0)" );
        return(TRUE);
    }
    Aligned32 = (Var==1);

    if (SDRAMncLooping)
    {
        STTBX_Print(("Testing copy in SDRAM non cached:\n"));
        while ((dummychar == 0) && !RetErr)
        {
            STTBX_Print(("\n\n##### LoopIndex = %04d ##### \n\n",LoopIndex));
            RetErr = TestAllMethods();
            /* Test if a key was pressed on the keyboard */
            if (STTBX_InputPollChar(&dummychar))
            {
                /* dummychar is not 0, so program will quit loop */
                STTBX_Print(("\nKey hit, end looping\n"));
            }
            LoopIndex++;
        }
    }
    else
    {
        RetErr = TestAllMethods();
    }
    return(RetErr);
}

/* End of avm_spd.c */



