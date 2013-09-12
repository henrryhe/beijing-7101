/*******************************************************************************

File name : acc_1dbm.c

Description : AVMEM memory access file for MPEG 1D block move

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
 3 mar 2000         Created                                          PLe
01 Jun 2001         Add stavmem_ before non API exported symbols     HSdLM
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"
#include "stsys.h"
#include "sttbx.h"

#include "stavmem.h"
#include "avm_acc.h"
#include "acc_1dbm.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* MPEG 1D block move is supposing the SDRAM is at this address.
This cannot be configured by registers, this is set within the device. */
#define STAVMEM_MPEG1DBMBuildInSDRAMBase         ((U32) stavmem_MemAccessDevice.SDRAMBaseAddr_p)
#define STAVMEM_SDRAMSize                        (stavmem_MemAccessDevice.SDRAMSize)
#define STAVMEM_MPEG1DBMBuildInSDRAMBase_End     (((U32) STAVMEM_MPEG1DBMBuildInSDRAMBase) + STAVMEM_SDRAMSize - 1)


/* Definitions for MPEG 1D block move */
#define STAVMEM_STVID_BASE_ADDRESS  ((U32) stavmem_MemAccessDevice.VideoBaseAddr_p)
#define USD_BMS                    0x09
#define USD_BWP                    0x8C
#define USD_BRP                    0x88
#define USD_BSK                    0xA4
#define VID_STA2                   0x64
#define VID_STA_BMI                0x20


/* Private Macros ----------------------------------------------------------- */

/* Access to registers is big endian */
#define STAVMEM_MPEG1DBMWriteReg24(AddrFromBase, Val)       STSYS_WriteRegDev24BE(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))), (U32) ((Val) & 0xFFFFFF))
#define STAVMEM_MPEG1DBMWriteReg16(AddrFromBase, Val)       STSYS_WriteRegDev16BE(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))), (U16) (Val))
#define STAVMEM_MPEG1DBMReadReg16(AddrFromBase)             STSYS_ReadRegDev16BE(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))))
#define STAVMEM_MPEG1DBMWriteReg8(AddrFromBase, Val)        STSYS_WriteRegDev8(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))), (U8) (Val))
#define STAVMEM_MPEG1DBMReadReg8(AddrFromBase)              STSYS_ReadRegDev8(((void *) (STAVMEM_STVID_BASE_ADDRESS + (AddrFromBase))))


/* Private Variables (static)------------------------------------------------ */
static semaphore_t *stavmem_LockMPEG1DBlockMove_p;   /* Block move DMA locking semaphore */
static BOOL MPEG1DBlockMoveInitialised = FALSE;


/* Private Function prototypes ---------------------------------------------- */
static void InitMPEG1DBlockMove(void);
static void Copy1DNoOverlapMPEG1DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy2DNoOverlapMPEG1DBlockMove(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                           void * const DestAddr_p, const U32 DestPitch);
static ST_ErrorCode_t CheckCopy1DMPEG1DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static ST_ErrorCode_t CheckCopy2DMPEG1DBlockMove(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                                 void * const DestAddr_p, const U32 DestPitch);

/* Exported structure --------------------------------------------------------*/
stavmem_MethodOperations_t stavmem_MPEG1DBlockMove = {
    InitMPEG1DBlockMove,                            /* Private function */
    /* All functions supported by MPEG 1D block move */
    {
        Copy1DNoOverlapMPEG1DBlockMove,           /* Private function */
        NULL,                                     /* Not Supported */
        Copy2DNoOverlapMPEG1DBlockMove,           /* Not Supported */
        NULL,                                     /* Not Supported */
        NULL,                                     /* Not Supported */
        NULL                                      /* Not Supported */
    },
    /* check */
    {
        CheckCopy1DMPEG1DBlockMove,                 /* Private function */
        NULL,
        CheckCopy2DMPEG1DBlockMove,                 /* Private function */
        NULL,
        NULL,
        NULL
    }
};


/*******************************************************************************
Name        : InitMPEG1DBlockMove
Description : Init semaphore used for memory accesses with MPEG 1D block move
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitMPEG1DBlockMove(void)
{
    if (!MPEG1DBlockMoveInitialised)
    {
        /* Protect semaphores initialisation */
        STOS_TaskLock();

        /* init semaphore  */
        stavmem_LockMPEG1DBlockMove_p = STOS_SemaphoreCreateFifo(NULL,1);
        MPEG1DBlockMoveInitialised = TRUE;

        /* Un-protect semaphores initialisation */
        STOS_TaskUnlock();

        /* MPEG1DBLockMove initialised */
        MPEG1DBlockMoveInitialised = TRUE;
    }
}


/*******************************************************************************
Name        : CheckCopy1DMPEG1DBlockMove
Description : Tells whether MPEG 1D block move is applicable for the copy/fill
Parameters  : source, destination, width, height, pitch
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if it is applicable, ST_ERROR_FEATURE_NOT_SUPPORTED otherwise
*******************************************************************************/
static ST_ErrorCode_t CheckCopy1DMPEG1DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    /* MPEG 1D block move only valid within SDRAM */
    if (
       (((U32)SrcAddr_p)  < ((U32)STAVMEM_MPEG1DBMBuildInSDRAMBase)) || /* source must be within SDRAM */
       ((((U32)SrcAddr_p)+Size) >  (STAVMEM_MPEG1DBMBuildInSDRAMBase_End)) ||
       (((U32)DestAddr_p) < ((U32)STAVMEM_MPEG1DBMBuildInSDRAMBase)) || /* destination must be within SDRAM */
       ((((U32)DestAddr_p)+Size) >  (STAVMEM_MPEG1DBMBuildInSDRAMBase_End))
       )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (((U8)((U32)(SrcAddr_p) & 0x7 )) != ((U8)((U32)(DestAddr_p) & 0x7 )))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : CheckCopy2DMPEG1DBlockMove
Description : Check if MPEG 2D block move can perform the copy
Parameters  : same as real copy
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if can make it, ST_ERROR_FEATURE_NOT_SUPPORTED if cannot make it
*******************************************************************************/

static ST_ErrorCode_t CheckCopy2DMPEG1DBlockMove(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight,
                                                 const U32 SrcPitch, void * const DestAddr_p, const U32 DestPitch)
{
    U32 farest_Src_pix = (U32)((SrcPitch * (SrcHeight - 1)) + SrcWidth - 1) ;
    U32 farest_Dest_pix = (U32)((DestPitch * (SrcHeight - 1)) + SrcWidth - 1) ;

    /* MPEG 1D block move only valid within SDRAM */
    if (
       (((U32)SrcAddr_p)  < ((U32)STAVMEM_MPEG1DBMBuildInSDRAMBase)) || /* source must be within SDRAM */
       ((((U32)SrcAddr_p)  + farest_Src_pix) > (STAVMEM_MPEG1DBMBuildInSDRAMBase_End)) ||
       (((U32)DestAddr_p) < ((U32) STAVMEM_MPEG1DBMBuildInSDRAMBase)) || /* destination must be within SDRAM */
       ((((U32)DestAddr_p) + farest_Dest_pix) > (STAVMEM_MPEG1DBMBuildInSDRAMBase_End))
       )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (((U32)(SrcAddr_p) & 0x7) != ((U32)(DestAddr_p) & 0x7))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if ((SrcHeight > 1) && (SrcPitch != DestPitch))
    {
        if  (((SrcPitch) & 0x7) != ((DestPitch) & 0x7 ))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }

    return(ST_NO_ERROR);
}



/*******************************************************************************
Name        : Copy1DNoOverlapMPEG1DBlockMove
Description : Perform 1D copy with no care of overlaps, with the STi5508 MPEG 1D Block move peripheral
Parameters  : source, destination, size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DNoOverlapMPEG1DBlockMove(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    U32 aligned_64_size, Size_in_bytes = Size ;
    U32 FromAddr ;
    U32 ToAddr ;
    U32 nb_bytes_first ;
    U32 nb_bytes_end ;
    U32 i;

    /* calculates first bytes to copy by hand */
    nb_bytes_first = ( 8 - (((U32)(SrcAddr_p)) & 0x7)) % 8 ;

    /* if size > nb of first bytes to copy, copy by hand and return */
    if (nb_bytes_first > Size)
    {
        nb_bytes_first = Size_in_bytes ;
        for (i = 0 ; i < nb_bytes_first ; i++)
        {
            *((U8 *)(DestAddr_p) + i) = *((U8 *)(SrcAddr_p) + i) ;
        }
        return ;
    }

    /* copy first bytes to copy by hand */
    if (nb_bytes_first)
    {
        FromAddr = (U32)(SrcAddr_p) + nb_bytes_first ;
        ToAddr =  (U32)(DestAddr_p) + nb_bytes_first ;
        Size_in_bytes = Size_in_bytes - nb_bytes_first ;  /* remove the first bytes size from the whole size */
        for (i = 0 ; i < nb_bytes_first ; i++)
        {
            *((U8 *)(DestAddr_p) + i) = *((U8 *)(SrcAddr_p) + i) ;
        }
    }
    else
    {
        FromAddr = (U32)(SrcAddr_p) ;
        ToAddr =  (U32)(DestAddr_p) ;
    }

    /* calculates nb of last bytes to copy by hand */
    nb_bytes_end = (FromAddr + Size_in_bytes) & 0x7;

    /* last bytes to copy by hand */
    if (nb_bytes_end)
    {
        Size_in_bytes = Size_in_bytes - nb_bytes_end;   /* remove the last bytes size from the whole size */
        for (i=0 ; i<nb_bytes_end ; i++ )
        {
            *((U8 *)(ToAddr + Size_in_bytes + i)) = *((U8 *)(FromAddr + Size_in_bytes + i)) ;
        }
        /* if Size_in_bytes == 0 all the bytes have been copied */
        if (Size_in_bytes == 0)
        {
            return ;
        }
    }

    /* copy with MPEG1DBlockmove */
    aligned_64_size = (Size_in_bytes >> 3);

    /* wait for the semaphore */
    STOS_SemaphoreWait(stavmem_LockMPEG1DBlockMove_p);

    /* BSK not used : 0 */
    STAVMEM_MPEG1DBMWriteReg24(USD_BSK, 0) ;

    while (aligned_64_size > 0xFFFF)
    {
        /* write size : USD_BMS */
        STAVMEM_MPEG1DBMWriteReg8(USD_BMS, 0xFF) ;
        STAVMEM_MPEG1DBMWriteReg8(USD_BMS, 0xFF) ;

        /* write destination : USD_BWP */
        STAVMEM_MPEG1DBMWriteReg24(USD_BWP , ((U32)(ToAddr) >> 3));

        /* write source : USD_BRP */
        STAVMEM_MPEG1DBMWriteReg24(USD_BRP , ((U32)(FromAddr) >> 3));

        /* wait for the end of the transfert : block move in progress while VID_STA BMI is set */
        while (((STAVMEM_MPEG1DBMReadReg8(VID_STA2)) & VID_STA_BMI) == 0)
        {
            STOS_TaskDelay(1);
        }

        ToAddr = ToAddr + (U32)((0x0000FFFF) << 3);
        FromAddr = FromAddr + (U32)((0x0000FFFF) << 3);

        aligned_64_size -= 0xFFFF;
    }

    if (aligned_64_size != 0)
    {
        /* write size : USD_BMS */
        STAVMEM_MPEG1DBMWriteReg8(USD_BMS, (U8)((aligned_64_size & 0x0000FF00) >> 8)) ;
        STAVMEM_MPEG1DBMWriteReg8(USD_BMS, (U8)(aligned_64_size & 0xFF)) ;

        /* write destination : USD_BWP */
        STAVMEM_MPEG1DBMWriteReg24(USD_BWP , ((U32)(ToAddr) >> 3));

        /* write source : USD_BRP */
        STAVMEM_MPEG1DBMWriteReg24(USD_BRP , ((U32)(FromAddr) >> 3));

        /* wait for the end of the transfert : block move in progress while VID_STA BMI is set */
        while (((STAVMEM_MPEG1DBMReadReg8(VID_STA2)) & VID_STA_BMI) == 0)
        {
            STOS_TaskDelay(1);
        }

    }

    /* free stavmem_MPEG1DBlockMove */
    STOS_SemaphoreSignal(stavmem_LockMPEG1DBlockMove_p);
}

/*******************************************************************************
Name        : Copy2DNoOverlapMPEG1DBlockMove
Description : Perform 2D copy with no care of overlaps, with the STi5508 MPEG 1D Block move peripheral
Parameters  : source, destination, size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DNoOverlapMPEG1DBlockMove(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                           void * const DestAddr_p, const U32 DestPitch)
{
    U32 y;
    for (y = 0; y < SrcHeight; y++)
    {
        Copy1DNoOverlapMPEG1DBlockMove((void *)(((U8 *) SrcAddr_p) + (y * SrcPitch)), (void *)(((U8 *) DestAddr_p) + (y * DestPitch)), SrcWidth);
    }
}

/* End of acc_1dbm.c */
