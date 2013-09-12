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
14/03/2006     Using STOS means this file is no longer required.     JCC

*****************************************************************************/
#include "stcommon.h"
#include "stos.h"
#include "stsys.h"

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


