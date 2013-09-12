/*******************************************************************************

File name : acc_2dbm.c

Description : AVMEM memory access file for MPEG 2D block move

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
 8 Dec 1999         Created                                          HG
01 Jun 2001         Add stavmem_ before non API exported symbols     HSdLM
02 Dec 2002         Fix GNBvd13243 "Overlapped copy fails to copy    HSdLM
 *                  all lines"
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"
#include "stsys.h"
#include "sttbx.h"

#include "stavmem.h"
#include "avm_acc.h"
#include "acc_2dbm.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* MPEG 2D block move is supposing the SDRAM is at this address.
This cannot be configured by registers, this is set within the device. */
#define STAVMEM_MPEG2DBMBuildInSDRAMBase ((U32) stavmem_MemAccessDevice.SDRAMBaseAddr_p)

#define STAVMEM_SDRAMSize (stavmem_MemAccessDevice.SDRAMSize)

/* Definitions for MPEG 2D block move */
#define STAVMEM_STVID_BASE_ADDRESS  ((U32) stavmem_MemAccessDevice.VideoBaseAddr_p)
#define VID_STA2                    0x64
#define VID_STA_BMI                 0x20
#define USD_BMC0                    0x9E
#define USD_BMC1                    0x9F
#define USD_BMC1_D_A                0x40
#define USD_BMC1_NIR                0x20
#define USD_BMC1_CPAT               0x10
#define USD_BMC1_RST                0x08
#define USD_BMC1_VSE                0x04
#define USD_BMC1_VSO                0x02
#define USD_BMC1_EXE                0x01
#define USD_BMH10                   0xA2
#define USD_BMH10_MASK             0x3FF
#define USD_BMW0                    0xA1
#define USD_BMW1                    0xA0
#define USD_BMW16                   0xA0
#define USD_BRP21                   0x88  /* BRP, BWP and BSK registers are 21 bits on 5514/16/17, 20 on 5512, 19 on 5510 */
#define USD_BSK21                   0xA4  /* however 21 bits are always used */
#define USD_BSK21_MASK          0x1FFFFF  /* fill reserved bit(s) on 5510 & 5512 */
#define USD_BWP21                   0x8C
#define USD_PAT64                   0xA7


/* Private Variables (static)------------------------------------------------ */

static semaphore_t *stavmem_LockMPEG2DBlockMove_p; /* MPEG 2D Block move locking semaphore */
static BOOL MPEG2DBlockMoveInitialised = FALSE;
static BOOL GlobIncrement;      /* Needed because read of MPEG 2D registers fails ! */
static U8 GlobCfg;              /* Needed because read of MPEG 2D registers fails ! */


/* Private Macros ----------------------------------------------------------- */

/* Access to registers is big endian */
#define STAVMEM_MPEG2DBMWriteReg32Invert(AddrFromBase, Val) STSYS_WriteRegDev32LE(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))), (U32) (Val))
#define STAVMEM_MPEG2DBMWriteReg32(AddrFromBase, Val) STSYS_WriteRegDev32BE(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))), (U32) (Val))
#define STAVMEM_MPEG2DBMWriteReg24(AddrFromBase, Val) STSYS_WriteRegDev24BE(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))), (U32) ((Val) & 0xFFFFFF))
#define STAVMEM_MPEG2DBMWriteReg16(AddrFromBase, Val) STSYS_WriteRegDev16BE(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))), (U16) (Val))
#define STAVMEM_MPEG2DBMWriteReg8(AddrFromBase, Val)  STSYS_WriteRegDev8(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))), (U8) (Val))
#define STAVMEM_MPEG2DBMReadReg8(AddrFromBase)        STSYS_ReadRegDev8(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))))

/* Check that block has finished in video interrupt status register */
#define MPEG2DBM_IsIdle ((STSYS_ReadRegDev8((void *) (STAVMEM_STVID_BASE_ADDRESS + VID_STA2)) & VID_STA_BMI) != 0)

/* Setup of the MPEG 2D block move */
#define MPEG2DBM_RESET_COPY_SRC_ADDR_UP   (0x00)
#define MPEG2DBM_RESET_COPY_SRC_ADDR_DOWN (USD_BMC1_D_A)
#define MPEG2DBM_RESET_COPY_SRC_PATTERN   (USD_BMC1_NIR)
#define MPEG2DBM_RESET_COPY_PAT_PATTERN   (USD_BMC1_CPAT)
#define MPEG2DBM_Reset(USD_BMC1_ResetConfig) {                              \
    STAVMEM_MPEG2DBMWriteReg8(USD_BMC1, USD_BMC1_RST);                      \
    STTBX_WaitMicroseconds(1);                                              \
    STAVMEM_MPEG2DBMWriteReg8(USD_BMC1, (USD_BMC1_ResetConfig) & ~USD_BMC1_RST); \
    GlobCfg = (USD_BMC1_ResetConfig) & 0xF0;                                \
    if (((USD_BMC1_ResetConfig) & USD_BMC1_D_A) == 0)                       \
    {                                                                       \
        GlobIncrement = TRUE;                                               \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        GlobIncrement = FALSE;                                              \
    }                                                                       \
    }

#define MPEG2DBM_SetPitchSkipPositive(WidthInWords64bit, PitchInWords64bit) { \
    U32 BlockSkip32;                                                        \
    /*if ((STAVMEM_MPEG2DBMReadReg8(USD_BMC1) & USD_BMC1_D_A) == 0)   */ \
    if (GlobIncrement)                                                      \
    {                                                                       \
        /* Incrementing addresses */                                        \
        BlockSkip32 = (PitchInWords64bit) - (WidthInWords64bit) + 1;        \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        /* Decrementing addresses */                                        \
        BlockSkip32 = (PitchInWords64bit) + (WidthInWords64bit) - 1;        \
    }                                                                       \
    STAVMEM_MPEG2DBMWriteReg24(USD_BSK21, BlockSkip32 & USD_BSK21_MASK);    \
    }

#define MPEG2DBM_SetPitchSkipNegative(WidthInWords64bit, PitchInWords64bit) { \
    U32 BlockSkip32;                                                        \
    /*if ((STAVMEM_MPEG2DBMReadReg8(USD_BMC1) & USD_BMC1_D_A) == 0) */ \
    if (GlobIncrement)                                                      \
    {                                                                       \
        /* Incrementing addresses */                                        \
        BlockSkip32 = -(PitchInWords64bit) - (WidthInWords64bit) + 1;       \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        /* Decrementing addresses */                                        \
        BlockSkip32 = -(PitchInWords64bit) + (WidthInWords64bit) - 1;       \
    }                                                                       \
    STAVMEM_MPEG2DBMWriteReg24(USD_BSK21, BlockSkip32 & USD_BSK21_MASK);    \
    }

#define MPEG2DBM_SetHeight(Lines) {                                         \
    STAVMEM_MPEG2DBMWriteReg16(USD_BMH10, (Lines) & USD_BMH10_MASK);        \
    }

#define MPEG2DBM_SetWidth(Words64bit) {                                     \
    STAVMEM_MPEG2DBMWriteReg8(USD_BMW1, (Words64bit) >> 8);                 \
    STAVMEM_MPEG2DBMWriteReg8(USD_BMW0, (Words64bit)     );                 \
    }

/* Caution: set width and height must have been called before ! */
#define MPEG2DBM_SetSrc(SrcAddrAsOffsetFromSDRAMBaseAddr) {                 \
    STAVMEM_MPEG2DBMWriteReg24(USD_BRP21, ((U32) (SrcAddrAsOffsetFromSDRAMBaseAddr) >> 3)); \
    }

/* Caution: set width and height must have been called before ! */
#define MPEG2DBM_SetDest(DestAddrAsOffsetFromSDRAMBaseAddr) {               \
    STAVMEM_MPEG2DBMWriteReg24(USD_BWP21, ((U32) (DestAddrAsOffsetFromSDRAMBaseAddr) >> 3)); \
    }

#define MPEG2DBM_SetPattern(Pattern32MSB, Pattern32LSB) {                   \
    /* Caution: pattern must be set with endianness inverted ! (Why ?) */   \
    STAVMEM_MPEG2DBMWriteReg8(USD_PAT64 + 7, (Pattern32MSB) >> 24);         \
    STAVMEM_MPEG2DBMWriteReg8(USD_PAT64 + 6, (Pattern32MSB) >> 16);         \
    STAVMEM_MPEG2DBMWriteReg8(USD_PAT64 + 5, (Pattern32MSB) >>  8);         \
    STAVMEM_MPEG2DBMWriteReg8(USD_PAT64 + 4, (Pattern32MSB)      );         \
    STAVMEM_MPEG2DBMWriteReg8(USD_PAT64 + 3, (Pattern32LSB) >> 24);         \
    STAVMEM_MPEG2DBMWriteReg8(USD_PAT64 + 2, (Pattern32LSB) >> 16);         \
    STAVMEM_MPEG2DBMWriteReg8(USD_PAT64 + 1, (Pattern32LSB) >>  8);         \
    STAVMEM_MPEG2DBMWriteReg8(USD_PAT64 + 0, (Pattern32LSB)      );         \
    }

#define MPEG2DBM_START_AFTER_NEXT_VSYNC      (USD_BMC1_VSE | USD_BMC1_VSO)
#define MPEG2DBM_START_AFTER_NEXT_EVEN_VSYNC (USD_BMC1_VSE)
#define MPEG2DBM_START_AFTER_NEXT_ODD_VSYNC  (USD_BMC1_VSO)
#define MPEG2DBM_START_NOW                   (USD_BMC1_EXE)
#define MPEG2DBM_Start(USD_BMC1_StartConfig, LineNumberAfterWhichDisplayToStart) { \
    U8 Tmp8;                                                                \
    STAVMEM_MPEG2DBMWriteReg8(USD_BMC0, (LineNumberAfterWhichDisplayToStart)); \
/*    Tmp8 = STAVMEM_MPEG2DBMReadReg8(USD_BMC1) & ~0x07; */ \
    Tmp8 = GlobCfg;                                                         \
    STAVMEM_MPEG2DBMWriteReg8(USD_BMC1, (Tmp8 | ((USD_BMC1_StartConfig) & 0x07))); \
    }



/* Private Function prototypes ---------------------------------------------- */

static void InitMPEG2DBlockMove(void);
static void Copy1DNoOverlapMPEG2DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy1DHandleOverlapMPEG2DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy2DNoOverlapMPEG2DBlockMove(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                           void * const DestAddr_p, const U32 DestPitch);
static void Copy2DHandleOverlapMPEG2DBlockMove(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                               void * const DestAddr_p, const U32 DestPitch);
static void Fill1DMPEG2DBlockMove(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize);
static void Fill2DMPEG2DBlockMove(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                  void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);
static ST_ErrorCode_t CheckCopy1DMPEG2DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static ST_ErrorCode_t CheckCopy2DMPEG2DBlockMove(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                                 void * const DestAddr_p, const U32 DestPitch);
static ST_ErrorCode_t CheckFill1DMPEG2DBlockMove(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize);
static ST_ErrorCode_t CheckFill2DMPEG2DBlockMove(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                            void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);
static ST_ErrorCode_t CheckIfMPEG2DBlockMoveCanDoIt(void * const SrcAddr_p, void * const DestAddr_p, const U32 Width, const U32 Height, const U32 Pitch);



stavmem_MethodOperations_t stavmem_MPEG2DBlockMove = {
    InitMPEG2DBlockMove,                            /* Private function */
    /* All functions supported by MPEG 2D block move */
    {
        Copy1DNoOverlapMPEG2DBlockMove,             /* Private function */
        Copy1DHandleOverlapMPEG2DBlockMove,         /* Private function */
        Copy2DNoOverlapMPEG2DBlockMove,             /* Private function */
        Copy2DHandleOverlapMPEG2DBlockMove,         /* Private function */
        Fill1DMPEG2DBlockMove,                      /* Private function */
        Fill2DMPEG2DBlockMove                       /* Private function */
    },
    /* Need to check: MPEG 2D block move has strong constraints (being in SDRAM, alignments) */
    {
        CheckCopy1DMPEG2DBlockMove,                 /* Private function */
        CheckCopy1DMPEG2DBlockMove,                 /* Private function */
        CheckCopy2DMPEG2DBlockMove,                 /* Private function */
        CheckCopy2DMPEG2DBlockMove,                 /* Private function */
        CheckFill1DMPEG2DBlockMove,                 /* Private function */
        CheckFill2DMPEG2DBlockMove                  /* Private function */
    }
};



/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : InitMPEG2DBlockMove
Description : Init semaphore used for memory accesses with MPEG 2D block move
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitMPEG2DBlockMove(void)
{
    static BOOL MPEG2DBlockMoveSemaphoreInitialised = FALSE;

    /* Protect semaphores initialisation */
    STOS_TaskLock();

    if (!MPEG2DBlockMoveSemaphoreInitialised)
    {
        stavmem_LockMPEG2DBlockMove_p = STOS_SemaphoreCreateFifo(NULL,1);   /* Init MPEG 2D Block move DMA locking semaphore */
        MPEG2DBlockMoveSemaphoreInitialised = TRUE;
    }

    /* Un-protect semaphores initialisation */
    STOS_TaskUnlock();

    /* Caution, deletion will never occur ! */
    /* Delete MPEG 2D Block move locking semaphore */
/*    semaphore_delete(stavmem_LockMPEG2DBlockMove_p);*/

    MPEG2DBlockMoveInitialised = TRUE;
} /* InitMPEG2DBlockMove() */



/*******************************************************************************
Name        : CheckIfMPEG2DBlockMoveCanDoIt
Description : Tells whether MPEG 2D block move is applicable for the copy/fill
Parameters  : source, destination, width, height, pitch
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if it is applicable, ST_ERROR_FEATURE_NOT_SUPPORTED otherwise
*******************************************************************************/
static ST_ErrorCode_t CheckIfMPEG2DBlockMoveCanDoIt(void * const SrcAddr_p, void * const DestAddr_p, const U32 Width, const U32 Height, const U32 Pitch)
{
    /* MPEG 2D block move only valid within SDRAM, if pitch is multiple of 64bit,
    if source and destination addresses can be aligned on 64bit the same way. */
    if ((((U32) SrcAddr_p)  < ((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase)) || /* source must be within SDRAM */
        ((((U32) SrcAddr_p)  + (Pitch * (Height - 1)) + Width - 1) > (((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase) + STAVMEM_SDRAMSize - 1)) ||
        (((U32) DestAddr_p) < ((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase)) || /* destination must be within SDRAM */
        ((((U32) DestAddr_p) + (Pitch * (Height - 1)) + Width - 1) > (((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase) + STAVMEM_SDRAMSize - 1)) ||
        (((Height > 1) && (Pitch & 0x7) != 0)) ||     /* pitch must be 64 bit aligned (if relevant) */
        ((((U32) SrcAddr_p) & 0x7) != (((U32) DestAddr_p) & 0x7)) /* source and destination must have the same 64 bit alignment */
        )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
} /* CheckIfMPEG2DBlockMoveCanDoIt() */



/*******************************************************************************
Name        : Copy1DNoOverlapMPEG2DBlockMove
Description : Perform 1D copy with no care of overlaps, with the MPEG 2D Block move peripheral
Parameters  : source, destination, size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DNoOverlapMPEG2DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    Copy2DNoOverlapMPEG2DBlockMove(SrcAddr_p, Size, 1, Size, DestAddr_p, Size);
}


/*******************************************************************************
Name        : Copy1DHandleOverlapMPEG2DBlockMove
Description : Perform 1D copy with care of overlaps, with the MPEG 2D Block move peripheral
Parameters  : source, destination, size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DHandleOverlapMPEG2DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    Copy2DHandleOverlapMPEG2DBlockMove(SrcAddr_p, Size, 1, Size, DestAddr_p, Size);
}


/*******************************************************************************
Name        : Copy2DNoOverlapMPEG2DBlockMove
Description : Perform 2D copy with no care of overlaps, with the MPEG 2D Block move peripheral
Parameters  : source, width, height, source pitch, destination, dest pitch
Assumptions :
Limitations : If pitches are different, MPEG 2D block move cannot do it in 2D. Anyway, we can do it line by line.
Returns     : Nothing
*******************************************************************************/
static void Copy2DNoOverlapMPEG2DBlockMove(void * const SrcAddr_p, const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                                     void * const DestAddr_p, const U32 DestPitch)
{
    U32 DecHeight = SrcHeight;
    U32 AlignedWidth = SrcWidth, DecWidth;
    U32 ToAddr   = (U32) DestAddr_p, BaseToAddr;
    U32 FromAddr = (U32) SrcAddr_p,  BaseFromAddr;
    U32 i,j;
    U8 NonAlignedBytesLeft = 0, NonAlignedBytesRight = 0;

    /* If first time, initialise semaphore */
/*    if (!MPEG2DBlockMoveInitialised)
    {
        InitMPEG2DBlockMove();
    }*/

    /* If pitches are different, MPEG 2D block move cannot do it in 2D. Anyway, we can do it line by line... */
    if (SrcPitch != DestPitch)
    {
        for (i = 0; i < SrcHeight; i++)
        {
            /* Copy 1D: one line */
            Copy2DNoOverlapMPEG2DBlockMove((void *)((U32) SrcAddr_p + (i * SrcPitch)), SrcWidth, 1, SrcWidth, (void *)((U32) DestAddr_p + (i * DestPitch)), SrcWidth);
        }
        return;
    }

    /* Don't care about overlaps: always take the same copy direction ! */

    if (SrcWidth < 8)
    {
        /* Will copy all manually for small width */
        NonAlignedBytesLeft = SrcWidth;
    }
    else
    {
        /* If Addresses not aligned on 64bit, a column at the left of the 2D area will have
        to be copied to align the address of the rest to copy */
        while ((FromAddr & 0x7) != 0)
        {
            NonAlignedBytesLeft++;
            FromAddr++;
            ToAddr++;
            /* Decrement AlignedWidth but don't become negative ! */
            if (AlignedWidth != 0)
            {
                AlignedWidth--;
            }
        }

        /* Now we know the address aligned on 64bit: deduce address for MPEG 2D block move */
        BaseFromAddr = FromAddr - ((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase);
        BaseToAddr   = ToAddr   - ((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase);

        /* If AlignedWidth not aligned on 64bit, a column at the right of the 2D area will have
        to be copied to align the width of the rest to copy */
        while ((AlignedWidth & 0x7) != 0)
        {
            NonAlignedBytesRight++;
            AlignedWidth--;
        }

        /* Wait for the MPEG 2D Block move locking semaphore to be free: use block only below this limit ! */
        STOS_SemaphoreWait(stavmem_LockMPEG2DBlockMove_p);

        /* Here ToAddr, FromAddr, AlignedWidth and SrcPitch must be 64bit aligned (&0x7==0) */

        MPEG2DBM_Reset(MPEG2DBM_RESET_COPY_SRC_ADDR_UP);

        /* Setup skip */
        MPEG2DBM_SetPitchSkipPositive(AlignedWidth / 8, SrcPitch / 8);

        /* If size is greater than max of block move DMA, cut in blocks of this size  */
        while (DecHeight >= 1023)
        {
            MPEG2DBM_SetHeight(1023);

            DecWidth = AlignedWidth;
            while (DecWidth >= (65535 * 8))
            {
                MPEG2DBM_SetWidth(65535);

                MPEG2DBM_SetSrc(BaseFromAddr + AlignedWidth - DecWidth);
                MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }

                DecWidth = DecWidth - (65535 * 8);
            }
            if (DecWidth > 0)
            {
                MPEG2DBM_SetWidth(DecWidth / 8);

                MPEG2DBM_SetSrc(BaseFromAddr + AlignedWidth - DecWidth);
                MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }
            }

            /* 1023 lines less to be copied... */
            DecHeight = DecHeight - 1023;

            /* Shift addresses in height */
            BaseFromAddr = BaseFromAddr + (1023 * SrcPitch);
            BaseToAddr   = BaseToAddr   + (1023 * SrcPitch);
        }

        /* After copy of blocks of max size of block move DMA, copy the remaining bytes */
        if (DecHeight > 0)
        {
            MPEG2DBM_SetHeight(DecHeight);

            DecWidth = AlignedWidth;
            while (DecWidth >= (65535 * 8))
            {
                MPEG2DBM_SetWidth(65535);

                MPEG2DBM_SetSrc(BaseFromAddr + AlignedWidth - DecWidth);
                MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }

                DecWidth -= 65535 * 8;
            }
            if (DecWidth > 0)
            {
                MPEG2DBM_SetWidth(DecWidth / 8);

                MPEG2DBM_SetSrc(BaseFromAddr + AlignedWidth - DecWidth);
                MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }
            }
        }

        /* Free the MPEG 2D Block move locking semaphore: use block only above this limit ! */
        STOS_SemaphoreSignal(stavmem_LockMPEG2DBlockMove_p);
    }

    /* Copy remaining left bytes if there are some */
    if (NonAlignedBytesLeft != 0)
    {
        j = 0;
        do
        {
            for (i = 0; i < SrcHeight; i++)
            {
                *(((U8 *) DestAddr_p) + (i * SrcPitch) + j) = *(((U8 *) SrcAddr_p) + (i * SrcPitch) + j);
            }
            j++;
        }
        while (j < NonAlignedBytesLeft);
        NonAlignedBytesLeft = 0;
    }
    /* Copy remaining right bytes if there are some */
    if (NonAlignedBytesRight != 0)
    {
        j = 0;
        do
        {
            for (i = 0; i < SrcHeight; i++)
            {
                *(((U8 *) DestAddr_p) + (i * SrcPitch) + SrcWidth - j - 1) = *(((U8 *) SrcAddr_p) + (i * SrcPitch) + SrcWidth - j - 1);
            }
            j++;
        }
        while (j < NonAlignedBytesRight);
        NonAlignedBytesRight = 0;
    }
} /* Copy2DNoOverlapMPEG2DBlockMove() */


/*******************************************************************************
Name        : Copy2DHandleOverlapMPEG2DBlockMove
Description : Perform 2D copy with care of overlaps, with the MPEG 2D Block move peripheral
Parameters  : source, width, height, source pitch, destination, dest pitch
Assumptions :
Limitations : If pitches are different, MPEG 2D block move cannot do it in 2D. Anyway, we can do it line by line
Returns     : Nothing
*******************************************************************************/
static void Copy2DHandleOverlapMPEG2DBlockMove(void * const SrcAddr_p, const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                               void * const DestAddr_p, const U32 DestPitch)
{
    U32 DecHeight = SrcHeight;
    U32 AlignedWidth = SrcWidth, DecWidth;
    U32 ToAddr   = (U32) DestAddr_p, BaseToAddr;
    U32 FromAddr = (U32) SrcAddr_p,  BaseFromAddr;
    U32 i,j;
    /* The model choosen here to describe geomatrical situations refers to the memory as follows:
      Origin (0,0)
      = top-left       --->  addresses up (X up)
          |
          | lines up
          |  (Y up)
          V                                        */
    BOOL LinesUp, AddressesUp;  /* Direction of copy to be taken to handle all overlaps */
    U8 NonAlignedBytesLeft = 0, NonAlignedBytesRight = 0;

    /* If first time, initialise semaphore */
/*    if (!MPEG2DBlockMoveInitialised)
    {
        InitMPEG2DBlockMove();
    }*/

    /* Determine direction of copy: in Y first, in X then.
       In Y it is the relative position of the source and destination addresses.
       In X: let's call diff the absolute difference in X between source and destination.
       Considering the 4 possible cases of spatial situations, the particular value
       Z = (Abs(SrcAddr - DestAddr) % SrcPitch) can only be diff or (SrcPitch - diff),
       depending on the relative spatial situation:
        - Z is diff when source is at top-left or bottom-right of destination
        - Z is (SrcPitch - diff) when source is at top-right or bottom-left of destination
       As the condition diff <= (SrcPitch - SrcWidth) is necessary (geometrically), then we deduce:
        - if (Z < SrcWidth), Z can only be diff
        - if (Z > (SrcPitch - SrcWidth)), Z can only be (SrcPitch - diff)
        - if SrcWidth <= Z <= (SrcPitch - SrcWidth), Z can be diff or (SrcPitch - diff), but it doesn't
          care because there is no spatial overlap of the source and destination in that case. */
    if (((U32) SrcAddr_p) < ((U32) DestAddr_p))
    {
        /* In Y: source is above destination, or maybe on the same line */
        LinesUp = FALSE;
        if (((((U32) DestAddr_p) - ((U32) SrcAddr_p) + SrcPitch) % SrcPitch) < SrcWidth)
        {
            /* Source at the left */
            AddressesUp = FALSE;
        }
        else
        {
            /* Source at the right */
            AddressesUp = TRUE;
        }
    }
    else
    {
        /* In Y: source is below destination, or maybe on the same line */
        LinesUp = TRUE;
        if (((((U32) SrcAddr_p) - ((U32) DestAddr_p) + SrcPitch) % SrcPitch) < SrcWidth)
        {
            /* Source at the right */
            AddressesUp = TRUE;
        }
        else
        {
            /* Source at the left */
            AddressesUp = FALSE;
        }
    }

    /* If pitches are different, MPEG 2D block move cannot do it in 2D. Anyway, we can do it line by line... */
    if (SrcPitch != DestPitch)
    {
        if (LinesUp)
        {
            for (i = 0; i < SrcHeight; i++)
            {
                /* Copy 1D: one line */
                Copy2DHandleOverlapMPEG2DBlockMove((void *)((U32) SrcAddr_p + (i * SrcPitch)), SrcWidth, 1, SrcWidth, (void *)((U32) DestAddr_p + (i * DestPitch)), SrcWidth);
            }
        }
        else
        {
            for (i = SrcHeight; i > 0; i--)
            {
                /* Copy 1D: one line */
                Copy2DHandleOverlapMPEG2DBlockMove((void *)((U32) SrcAddr_p + ((i - 1) * SrcPitch)), SrcWidth, 1, SrcWidth, (void *)((U32) DestAddr_p + ((i - 1) * DestPitch)), SrcWidth);
            }
        }
        return;
    }

    if (SrcWidth < 8)
    {
        /* Will copy all manually for small width */
        NonAlignedBytesLeft = SrcWidth;
    }
    else
    {
        /* If Addresses not aligned on 64bit, a column at the left of the 2D area will have
        to be copied to align the address of the rest to copy */
        while ((FromAddr & 0x7) != 0)
        {
            NonAlignedBytesLeft++;
            FromAddr++;
            ToAddr++;
            /* Decrement AlignedWidth but don't become negative ! */
            if (AlignedWidth != 0)
            {
                AlignedWidth--;
            }
        }

        /* Now we know the address aligned on 64bit: deduce address for MPEG 2D block move */
        BaseFromAddr = FromAddr - ((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase);
        BaseToAddr   = ToAddr   - ((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase);

        /* If AlignedWidth not aligned on 64bit, a column at the right of the 2D area will have
        to be copied to align the width of the rest to copy */
        while ((AlignedWidth & 0x7) != 0)
        {
            NonAlignedBytesRight++;
            AlignedWidth--;
        }

        /* Wait for the MPEG 2D Block move locking semaphore to be free: use block only below this limit ! */
        STOS_SemaphoreWait(stavmem_LockMPEG2DBlockMove_p);

        /* Here ToAddr, FromAddr, AlignedWidth and SrcPitch must be 64bit aligned (&0x7==0) */

        /* Copy up/down depending on relative position in X of source and destination */
        if (AddressesUp)
        {
            /* Source at the right */
            MPEG2DBM_Reset(MPEG2DBM_RESET_COPY_SRC_ADDR_UP);

            /* Copy left bytes before they disappear with the copy */
            if (NonAlignedBytesLeft != 0)
            {
                j = 0;
                do
                {
                    if (LinesUp)
                    {
                        for (i = 0; i < SrcHeight; i++)
                        {
                            *(((U8 *) DestAddr_p) + (i * SrcPitch) + j) = *(((U8 *) SrcAddr_p) + (i * SrcPitch) + j);
                        }
                    }
                    else
                    {
                        for (i = SrcHeight; i > 0; i--)
                        {
                            *(((U8 *) DestAddr_p) + ((i - 1) * SrcPitch) + j) = *(((U8 *) SrcAddr_p) + ((i - 1) * SrcPitch) + j);
                        }
                    }
                    j++;
                }
                while (j < NonAlignedBytesLeft);
                NonAlignedBytesLeft = 0;
            }
        }
        else
        {
            /* Source at the left */
            MPEG2DBM_Reset(MPEG2DBM_RESET_COPY_SRC_ADDR_DOWN);

            /* Addresses go down: shift addresses in width */
            BaseFromAddr = BaseFromAddr + AlignedWidth - 1;
            BaseToAddr   = BaseToAddr   + AlignedWidth - 1;

            /* Copy right bytes before they disappear with the copy */
            if (NonAlignedBytesRight != 0)
            {
                j = 0;
                do
                {
                    if (LinesUp)
                    {
                        for (i = 0; i < SrcHeight; i++)
                        {
                            *(((U8 *) DestAddr_p) + (i * SrcPitch) + SrcWidth - j - 1) = *(((U8 *) SrcAddr_p) + (i * SrcPitch) + SrcWidth - j - 1);
                        }
                    }
                    else
                    {
                        for (i = SrcHeight; i > 0; i--)
                        {
                            *(((U8 *) DestAddr_p) + ((i - 1) * SrcPitch) + SrcWidth - j - 1) = *(((U8 *) SrcAddr_p) + ((i - 1) * SrcPitch) + SrcWidth - j - 1);
                        }
                    }
                    j++;
                }
                while (j < NonAlignedBytesRight);
                NonAlignedBytesRight = 0;
            }
        }

        /* Setup skip and shift addresses in height if negative */
        if (LinesUp)
        {
            MPEG2DBM_SetPitchSkipPositive(AlignedWidth / 8, SrcPitch / 8);
        }
        else
        {
            MPEG2DBM_SetPitchSkipNegative(AlignedWidth / 8, SrcPitch / 8);
            BaseFromAddr = BaseFromAddr + (SrcPitch * (SrcHeight - 1));
            BaseToAddr   = BaseToAddr   + (SrcPitch * (SrcHeight - 1));
        }

        /* If size is greater than max of block move DMA, cut in blocks of this size  */
        while (DecHeight >= 1023)
        {
            MPEG2DBM_SetHeight(1023);

            DecWidth = AlignedWidth;
            while (DecWidth >= (65535 * 8))
            {
                MPEG2DBM_SetWidth(65535);

                if (AddressesUp)
                {
                    MPEG2DBM_SetSrc(BaseFromAddr + AlignedWidth - DecWidth);
                    MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);
                }
                else
                {
                    MPEG2DBM_SetSrc(BaseFromAddr - AlignedWidth + DecWidth);
                    MPEG2DBM_SetDest(BaseToAddr - AlignedWidth + DecWidth);
                }

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }

                DecWidth = DecWidth - (65535 * 8);
            }
            if (DecWidth > 0)
            {
                MPEG2DBM_SetWidth(DecWidth / 8);

                if (AddressesUp)
                {
                    MPEG2DBM_SetSrc(BaseFromAddr + AlignedWidth - DecWidth);
                    MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);
                }
                else
                {
                    MPEG2DBM_SetSrc(BaseFromAddr - AlignedWidth + DecWidth);
                    MPEG2DBM_SetDest(BaseToAddr - AlignedWidth + DecWidth);
                }

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }
            }

            /* 1023 lines less to be copied... */
            DecHeight = DecHeight - 1023;

            /* Shift addresses in height */
            if (LinesUp)
            {
                BaseFromAddr = BaseFromAddr + (1023 * SrcPitch);
                BaseToAddr   = BaseToAddr   + (1023 * SrcPitch);
            }
            else
            {
                BaseFromAddr = BaseFromAddr - (1023 * SrcPitch);
                BaseToAddr   = BaseToAddr   - (1023 * SrcPitch);
            }
        }

        /* After copy of blocks of max size of block move DMA, copy the remaining bytes */
        if (DecHeight > 0)
        {
            MPEG2DBM_SetHeight(DecHeight);

            DecWidth = AlignedWidth;
            while (DecWidth >= (65535 * 8))
            {
                MPEG2DBM_SetWidth(65535);

                if (AddressesUp)
                {
                    MPEG2DBM_SetSrc(BaseFromAddr + AlignedWidth - DecWidth);
                    MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);
                }
                else
                {
                    MPEG2DBM_SetSrc(BaseFromAddr - AlignedWidth + DecWidth);
                    MPEG2DBM_SetDest(BaseToAddr - AlignedWidth + DecWidth);
                }

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }

                DecWidth -= 65535 * 8;
            }
            if (DecWidth > 0)
            {
                MPEG2DBM_SetWidth(DecWidth / 8);

                if (AddressesUp)
                {
                    MPEG2DBM_SetSrc(BaseFromAddr + AlignedWidth - DecWidth);
                    MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);
                }
                else
                {
                    MPEG2DBM_SetSrc(BaseFromAddr - AlignedWidth + DecWidth);
                    MPEG2DBM_SetDest(BaseToAddr - AlignedWidth + DecWidth);
                }

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }
            }
        }

        /* Free the MPEG 2D Block move locking semaphore: use block only above this limit ! */
        STOS_SemaphoreSignal(stavmem_LockMPEG2DBlockMove_p);
    }

    /* Copy remaining left bytes if there are some */
    if (NonAlignedBytesLeft != 0)
    {
        j = 0;
        do
        {
            if (LinesUp)
            {
                for (i = 0; i < SrcHeight; i++)
                {
                    *(((U8 *) DestAddr_p) + (i * SrcPitch) + j) = *(((U8 *) SrcAddr_p) + (i * SrcPitch) + j);
                }
            }
            else
            {
                for (i = SrcHeight; i > 0; i--)
                {
                    *(((U8 *) DestAddr_p) + ((i - 1) * SrcPitch) + j) = *(((U8 *) SrcAddr_p) + ((i - 1) * SrcPitch) + j);
                }
            }
            j++;
        }
        while (j < NonAlignedBytesLeft);
        NonAlignedBytesLeft = 0;
    }
    /* Copy remaining right bytes if there are some */
    if (NonAlignedBytesRight != 0)
    {
        j = 0;
        do
        {
            if (LinesUp)
            {
                for (i = 0; i < SrcHeight; i++)
                {
                    *(((U8 *) DestAddr_p) + (i * SrcPitch) + SrcWidth - j - 1) = *(((U8 *) SrcAddr_p) + (i * SrcPitch) + SrcWidth - j - 1);
                }
            }
            else
            {
                for (i = SrcHeight; i > 0; i--)
                {
                    *(((U8 *) DestAddr_p) + ((i - 1) * SrcPitch) + SrcWidth - j - 1) = *(((U8 *) SrcAddr_p) + ((i - 1) * SrcPitch) + SrcWidth - j - 1);
                }
            }
            j++;
        }
        while (j < NonAlignedBytesRight);
        NonAlignedBytesRight = 0;
    }
} /* Copy2DHandleOverlapMPEG2DBlockMove() */



/*******************************************************************************
Name        : Fill1DMPEG2DBlockMove
Description : Perform 1D fill with the MPEG 2D Block move peripheral
Parameters  : pattern, pattern size, destination, destination size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Fill1DMPEG2DBlockMove(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize)
{
    Fill2DMPEG2DBlockMove(Pattern_p, PatternSize, 1, PatternSize, DestAddr_p, DestSize, 1, DestSize);
}



/*******************************************************************************
Name        : Fill2DMPEG2DBlockMove
Description : Perform 2D fill with the MPEG 2D Block move peripheral
Parameters  : pattern, pattern characteristics, destination, and destination  characteristics
Assumptions :
Limitations : As pattern supported is one dimensional and can be copied in 8 bytes,
              PatternHeight and PatternPitch are not considered, and with max PatternWidth of 8
Returns     : Nothing
*******************************************************************************/
static void Fill2DMPEG2DBlockMove(void * const Pattern_p, const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                  void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch)
{
    U32 DecHeight = DestHeight;
    U32 AlignedWidth = DestWidth, DecWidth;
    U32 ToAddr   = (U32) DestAddr_p, BaseToAddr;
    U32 FromAddr = (U32) Pattern_p,  BaseFromAddr;
    U32 i,j;
    U8 NonAlignedBytesLeft = 0, NonAlignedBytesRight = 0;
    U8 Pattern64[8];
    U32 NewMSB;
    U32 NewLSB;
    /* To elliminate warning tests */
    U32 Unused;
    Unused = PatternHeight;
    Unused = PatternPitch;

    /* If first time, initialise semaphore */
/*    if (!MPEG2DBlockMoveInitialised)
    {
        InitMPEG2DBlockMove();
    }*/

    /* Store values in order to copy from pattern register, and for copy of left and right non aligned areas */
    i = 0;
    do
    {
        j = 0;
        do
        {
            Pattern64[i] = *(((U8 *) Pattern_p) + j);
            j++;
            i++;
        } while (j < PatternWidth);
    } while (i < 8);

    if (DestWidth < 8)
    {
        /* Will copy all manually for small width */
        NonAlignedBytesLeft = DestWidth;
    }
    else
    {
        /* If Addresses not aligned on 64bit, a column at the left of the 2D area will have
        to be filled to align the address of the rest to copy */
        while ((ToAddr & 0x7) != 0)
        {
            NonAlignedBytesLeft++;
            ToAddr++;
            /* Decrement AlignedWidth but don't become negative ! */
            if (AlignedWidth != 0)
            {
                AlignedWidth--;
            }
        }

        /* Now we know the address aligned on 64bit: deduce address for MPEG 2D block move */
        BaseFromAddr = FromAddr - ((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase);
        BaseToAddr   = ToAddr   - ((U32) STAVMEM_MPEG2DBMBuildInSDRAMBase);

        /* If AlignedWidth not aligned on 64bit, copy column at the right of the 2D area to align the width of the rest to copy */
        while ((AlignedWidth & 0x7) != 0)
        {
            NonAlignedBytesRight++;
            AlignedWidth--;
        }

        /* Wait for the MPEG 2D Block move locking semaphore to be free: use block only below this limit ! */
        STOS_SemaphoreWait(stavmem_LockMPEG2DBlockMove_p);

        /* Here ToAddr, FromAddr, AlignedWidth and DestPitch must be 64bit aligned (&0x7==0) */
        /* Setup pattern */
        MPEG2DBM_Reset(MPEG2DBM_RESET_COPY_PAT_PATTERN);
    /*    if (NonAlignedBytesLeft <= 3)
        {
            NewMSB = (Pattern64[1] >> (8 * NonAlignedBytesLeft));
            NewLSB = (Pattern64[0] >> (8 * NonAlignedBytesLeft));
            if (NonAlignedBytesLeft != 0)
            {
                NewMSB += (U32) (Pattern64[0] << (32 - 8 * NonAlignedBytesLeft));
                NewLSB += (U32) (Pattern64[1] << (32 - 8 * NonAlignedBytesLeft));
            }
        }
        else
        {
            NewMSB = (Pattern64[0] >> (8 * (NonAlignedBytesLeft - 4)));
            NewLSB = (Pattern64[1] >> (8 * (NonAlignedBytesLeft - 4)));
            if (NonAlignedBytesLeft != 4)
            {
                NewMSB += (U32) (Pattern64[1] << (32 - 8 * (NonAlignedBytesLeft - 4)));
                NewLSB += (U32) (Pattern64[0] << (32 - 8 * (NonAlignedBytesLeft - 4)));
            }
        }*/
        NewLSB = 0;
        NewMSB = 0;
        for (i = 0; i <= 3; i++)
        {
            NewLSB += (Pattern64[(NonAlignedBytesLeft + i    ) % 8]) << (8 * i);
            NewMSB += (Pattern64[(NonAlignedBytesLeft + i + 4) % 8]) << (8 * i);
        }
        MPEG2DBM_SetPattern(NewMSB, NewLSB);

        /* No constraint on direction of copy... */
        MPEG2DBM_SetPitchSkipPositive(AlignedWidth / 8, DestPitch / 8);

        /* If size is greater than max of block move DMA, cut in blocks of this size  */
        while (DecHeight >= 1023)
        {
            MPEG2DBM_SetHeight(1023);

            DecWidth = AlignedWidth;
            while (DecWidth >= (65535 * 8))
            {
                MPEG2DBM_SetWidth(65535);

                MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }

                DecWidth -= 65535 * 8;
            }
            if (DecWidth > 0)
            {
                MPEG2DBM_SetWidth(DecWidth / 8);

                MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }
            }

            DecHeight -= 1023;
            FromAddr += 1023 * DestPitch;
            ToAddr += 1023 * DestPitch;
        }

        /* After copy of blocks of max size of block move DMA, copy the remaining bytes */
        if (DecHeight > 0)
        {
            MPEG2DBM_SetHeight(DecHeight);

            DecWidth = AlignedWidth;
            while (DecWidth >= (65535 * 8))
            {
                MPEG2DBM_SetWidth(65535);

                MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }

                DecWidth -= 65535 * 8;
            }
            if (DecWidth > 0)
            {
                MPEG2DBM_SetWidth(DecWidth / 8);

                MPEG2DBM_SetDest(BaseToAddr + AlignedWidth - DecWidth);

                MPEG2DBM_Start(MPEG2DBM_START_NOW, 0);

                while(!(MPEG2DBM_IsIdle))
                {
                    STOS_TaskDelay(1); /* Wait it is finished */
                }
            }
        }

        /* Free the MPEG 2D Block move locking semaphore: use block only above this limit ! */
        STOS_SemaphoreSignal(stavmem_LockMPEG2DBlockMove_p);
    }

    /* Copy remaining left bytes if there are some */
    if (NonAlignedBytesLeft != 0)
    {
        j = 0;
        do
        {
            for (i = 0; i < DestHeight; i++)
            {
                *(((U8 *) DestAddr_p) + (i * DestPitch) + j) = *(((U8 *) Pattern64) + j);
            }
            j++;
        }
        while (j < NonAlignedBytesLeft);
    }
    /* Copy remaining right bytes if there are some */
    if (NonAlignedBytesRight != 0)
    {
        j = 0;
        do
        {
            for (i = 0; i < DestHeight; i++)
            {
                /* Offset of NonAlignedBytesLeft is because of the pattern shitfing ! */
                *(((U8 *) DestAddr_p) + (i * DestPitch) + DestWidth - j - 1) = *(((U8 *) Pattern64) + ((NonAlignedBytesRight - 1 - j + NonAlignedBytesLeft) % 8));
            }
            j++;
        }
        while (j < NonAlignedBytesRight);
    }
} /* Fill2DMPEG2DBlockMove() */




/*******************************************************************************
Name        : CheckCopy1DMPEG2DBlockMove
Description : Check if MPEG 2D block move can perform the copy
Parameters  : same as real copy
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if can make it, ST_ERROR_FEATURE_NOT_SUPPORTED if cannot make it
*******************************************************************************/
static ST_ErrorCode_t CheckCopy1DMPEG2DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    if (CheckIfMPEG2DBlockMoveCanDoIt(SrcAddr_p, DestAddr_p, Size, 1, Size) != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : CheckCopy2DMPEG2DBlockMove
Description : Check if MPEG 2D block move can perform the copy
Parameters  : same as real copy
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if can make it, ST_ERROR_FEATURE_NOT_SUPPORTED if cannot make it
*******************************************************************************/
static ST_ErrorCode_t CheckCopy2DMPEG2DBlockMove(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                                 void * const DestAddr_p, const U32 DestPitch)
{
    if (SrcPitch != DestPitch) /* If we can't do it because of pitches different, we do it anyway line by line... */
    {
        if (SrcHeight > 1)
        {
            if  (((SrcPitch) & 0x7) != ((DestPitch) & 0x7))
            {
                /* We cannot do it line by line correclty if pitches don't have the same alignement ! */
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
        }
    }

    if (CheckIfMPEG2DBlockMoveCanDoIt(SrcAddr_p, DestAddr_p, SrcWidth, SrcHeight, SrcPitch) != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
} /* CheckCopy2DMPEG2DBlockMove() */

/*******************************************************************************
Name        : CheckFill1DMPEG2DBlockMove
Description : Check if MPEG 2D block move can perform the fill
Parameters  : same as real fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if can make it, ST_ERROR_FEATURE_NOT_SUPPORTED if cannot make it
*******************************************************************************/
static ST_ErrorCode_t CheckFill1DMPEG2DBlockMove(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize)
{
    /* To elliminate warning tests */
	void * Unused;
    Unused = Pattern_p;
    /* Pattern should be with a size that can be copied into 8 bytes */
    if (((PatternSize != 8) && (PatternSize != 4) && (PatternSize > 2)) ||
        (CheckIfMPEG2DBlockMoveCanDoIt(DestAddr_p, DestAddr_p, DestSize, 1, 8) != ST_NO_ERROR))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
} /* CheckFill1DMPEG2DBlockMove() */

/*******************************************************************************
Name        : CheckFill2DMPEG2DBlockMove
Description : Check if MPEG 2D block move can perform the fill
Parameters  : same as real fill (but PatternPitch doesn't care as pattern must be 1D)
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if can make it, ST_ERROR_FEATURE_NOT_SUPPORTED if cannot make it
*******************************************************************************/
static ST_ErrorCode_t CheckFill2DMPEG2DBlockMove(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                       void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch)
{
    /* To elliminate warning tests */
	void * Unused;
    Unused = Pattern_p;
    /* Pattern should be 1D and with a size that can be copied into 8 bytes */
    if ((PatternHeight != 1) ||
        ((PatternWidth != 8) && (PatternWidth != 4) && (PatternWidth > 2)) ||
        (PatternPitch == (U32) NULL) ||
        (CheckIfMPEG2DBlockMoveCanDoIt(DestAddr_p, DestAddr_p, DestWidth, DestHeight, DestPitch) != ST_NO_ERROR))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
} /* CheckFill2DMPEG2DBlockMove() */

/* End of acc_2dbm.c */
