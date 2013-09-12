/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

File name   :  osx.c
Description :  Operating system independence file.

               The purpose of this file is to centralise the differences
               between builds for OS20 and OS21 to the greatest extent
               possible by wrapping OS dependant calls in preprocessor
               macros and, where necessary, functions.

Date           Modification                                          Name
----           ------------                                          ----
14/09/2004     Created                                               JW
11/03/2005     Added support for tasks with static stacks.           JW
06/04/2007     Added cache invalidation/flush functions.             JCC

*****************************************************************************/
#include "stcommon.h"
#include "stos.h"
#include "stsys.h"
#if defined(STAPIREF_COMPAT)
#else
/* STFAE - Do this inclusion to have access to env variables STPTI_SUPPRESS_CACHING (OS20) */
#include "stpti.h"
#endif /* STAPIREF_COMPAT */
#include "osx.h"

#if !defined(ST_OSLINUX) 
#include <string.h>
#endif


/******************************************************************************
Function Name : stpti_MemoryAlign
  Description : Align addresses to a addressing boundry.
   Parameters : Alignment - alignment boundry size (1 for byte, 2 for U16, 
                            4 for U32)
******************************************************************************/
void *stpti_MemoryAlign(void* Memory_p, U32 Alignment)
{
    return (void *) ((U32)((U32)Memory_p+Alignment-1) & ~(Alignment-1) );
}


/******************************************************************************
Function Name : stpti_AtomicSetBit
  Description : An atomic operation to set a bit in a register/memory location.
   Parameters : Address_p - the address to perform the atomic operation on.
                BitNumber - the bit to set (LSB is 0)
******************************************************************************/
void stpti_AtomicSetBit( void* Address_p, U32 BitNumber )
{
    #if defined( ST_OSLINUX )

        /* Linux supports this as an atomic operation */
        set_bit( BitNumber, Address_p );

    #elif defined( ST_OS20 )  || defined( ST_OS21 )

        /* Under os2x we have to explicitly lock interrupts */
        STOS_InterruptLock();
        STSYS_WriteRegDev32LE( Address_p, (1 << BitNumber) | STSYS_ReadRegDev32LE( Address_p ));
        STOS_InterruptUnlock();

    #else
        #error Unsupported operating system.
    #endif
}


/******************************************************************************
Function Name : stpti_AtomicClearBit
  Description : An atomic operation to clear a bit in a register/memory location.
   Parameters : Address_p - the address to perform the atomic operation on.
                BitNumber - the bit to clear (LSB is 0)
******************************************************************************/
void stpti_AtomicClearBit( void* Address_p, U32 BitNumber )
{
    #if defined( ST_OSLINUX )

        /* Linux supports this as an atomic operation */
        clear_bit( BitNumber, Address_p );

    #elif defined( ST_OS20 )  || defined( ST_OS21 )

        /* Under os2x we have to explicitly lock interrupts */
        STOS_InterruptLock();
        STSYS_WriteRegDev32LE( Address_p, ~(1 << BitNumber) & STSYS_ReadRegDev32LE( Address_p ));
        STOS_InterruptUnlock();

    #else
        #error Unsupported operating system.
    #endif
}


/******************************************************************************
Function Name : stpti_InvalidateRegionCircular
  Description : Invalidate a region taking into accound buffer wrapping.
   Parameters : Base_p - Base virtual address
                Top_p  - Top virtual address
                Read_p - Read (starting) virtual address
                Size   - number of bytes to invalidate from read address
******************************************************************************/
void stpti_InvalidateRegionCircular(U32 Base_p, U32 Top_p, U32 Read_p, U32 Size)
{
    if ( (Read_p+Size) > Top_p)
    {
        stpti_InvalidateRegion((void*) Read_p, Top_p-Read_p);
        stpti_InvalidateRegion((void*) Base_p, Size-(Top_p-Read_p));
    }
    else
    {
        stpti_InvalidateRegion((void*) Read_p, Size);
    }
}


/******************************************************************************
Function Name : stpti_FlushRegionCircular
  Description : Flush a region taking into accound buffer wrapping.
   Parameters : Base_p - Base virtual address
                Top_p  - Top virtual address
                Read_p - Read (starting) virtual address
                Size   - number of bytes to flush from read address
******************************************************************************/
void stpti_FlushRegionCircular(U32 Base_p, U32 Top_p, U32 Read_p, U32 Size)
{
    /* If Read_p is outside the buffer then place inside the buffer */
    Read_p = ((Read_p-Base_p) % (Top_p-Base_p) ) + Base_p;
    if ( (Read_p+Size) > Top_p)
    {
        stpti_FlushRegion((void*) Read_p, Top_p-Read_p);
        stpti_FlushRegion((void*) Base_p, Size-(Top_p-Read_p));
    }
    else
    {
        stpti_FlushRegion((void*) Read_p, Size);
    }
}



/* STFAE - Workaround for 7100 Cut 1.0 to 3.0 */
#if defined(STPTI_710x_EARLY_CUT_WORKAROUND)

/******************************************************************************
Function Name : stpti_710xEarlyCutWorkaround
  Description : Check on 710x if you have cut requiring the tc instruction ram
                workaround, and perform it if necessary.
   Parameters : CodeSize - TC Loader's TC Code Size
******************************************************************************/
void stpti_710xEarlyCutWorkaround(U32 CodeStart, U32 CodeSize)
{
    static U32 TCIRam[9*1024];
    U32 ShiftValue  = 0x200;        /* 512 bytes */
    
    /* Workout the Chip cut and call stpti_TCIRAMMove_710x() if WA needed */
    U32 ChipVersion = ST_GetCutRevision();
    
#if defined(ST_7100)
    if (ChipVersion<=0xC0)   /* STFAE - Move the buffer only for cut to 3.0 included */
#elif defined(ST_7109)
    if (ChipVersion<0xB0)    /* STFAE - Move the buffer only for cut to 1.0 to 2.0 excluded */
#else
    if (FALSE)
#endif
    {
        U32 i;
        U32 *TCIAddr = (U32*)CodeStart;
        U32 Offset = 0;
        U32 JumpValue = 0xc0000000 | (ShiftValue << 6);
        U32 offset = 0;
#if defined(ST_7109)
        if (CodeSize + ShiftValue >= 13*1024)                   /* 7109 is 13K */
#else
        if (CodeSize + ShiftValue >= 9*1024)                    /* 7100 is 9K */
#endif
        {
            /* Code is too large to move */
            return;
        }
        memset(TCIRam,0,9*1024);
        for (i = 0; i < CodeSize/4; i++)                        /* copy into TCIRam array */
        {      
            TCIRam[i] = TCIAddr[i];
        }
        for (i = 0; i < CodeSize/4; i++)                        /* update TCIRam array with new offsets */
        {
            U32 dest = 0;
            dest = (TCIRam[i] >> 23) & 0x7;
            dest |= ((TCIRam[i] & 0x4) << 1);
            if (dest == 0xe)
            {
                U32 Instr = 0;
                Instr = TCIRam[i];
                Offset = Instr & 0x007FFFC0;
                Instr &= ~0x007FFFC0;
                Offset >>= 6;
                Offset += ShiftValue+offset*4;
                Offset <<= 6;
                Instr = Instr | (Offset & 0x7FFFC0); 
                TCIRam[i+offset] = Instr;
            }
            if (((U32)(TCIRam[i]) & 0x80000000) != 0)
            {
                U32 Instr = 0;
                Instr = TCIRam[i];
                if ((Instr >> 26) != 0x38)
                { 
                    Offset = Instr & 0x007FFFC0;
                    Instr &= ~0x007FFFC0;
                    Offset >>= 6;
                    Offset += ShiftValue+offset*4;
                    Offset <<= 6;
                    Instr = Instr | (Offset & 0x7FFFC0); 
                }        
                TCIRam[i+offset] = Instr;
            }
        }
        for (i=0;i<ShiftValue/4;i++)                        /* Fill first "ShiftValue" bytes with Jump instr. */
        {
            TCIAddr[i] = JumpValue;
        }
        for (i = 0; i < (CodeSize/4)+offset; i++)           /* Put in rest of instructions. */
        {
            TCIAddr[i + ShiftValue/4] = TCIRam[i];
        }
    }
}
/* STFAE - Workaround for 7100 Cut 1.0 to 3.0 */
#endif

