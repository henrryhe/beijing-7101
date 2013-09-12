/*****************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

File name   :  osx.h
Description :  Operating system independence file.

               The purpose of this file is to abstract away differences
               between OSes.  This facility is now offered by STOS and so
               this file is now odds and ends not covered by STOS.


Date           Modification                                          Name
----           ------------                                          ----
13/09/2004     Created                                               JW
11/03/2005     Added support for tasks with static stacks.           JW
02/08/2006     Moved to STOS.  Leaving just a few odds and ends.     JCC

*****************************************************************************/

#ifndef _OSX_H_
#define _OSX_H_

#include "stos.h"
#include "stcommon.h"
#include "stdevice.h"


/*** convenience ************************************************************************************************/
/* Left here for convenience as their removal will require blanket changes */

#define CLOCK                                   STOS_Clock_t
#define TICKS_PER_SECOND                        ST_GetClocksPerSecond()

/* A useful macro defined for linux (in compat.h) but not for os2x */
#if !defined(ST_OSLINUX)
#include "stsys.h"
#define STSYS_SetRegMask32LE(address, mask)     STSYS_WriteRegDev32LE(address, STSYS_ReadRegDev32LE(address) | (mask) )
#define STSYS_ClearRegMask32LE(address, mask)   STSYS_WriteRegDev32LE(address, STSYS_ReadRegDev32LE(address) & ~(mask) )
#endif /* #endif !defined(ST_OSLINUX) */

/*** Task stack size allocation  ********************************************/

#ifndef STPTI_TASK_STACK_SIZE
  #if defined( ST_OS20 )
    #define STPTI_TASK_STACK_SIZE 4096
  #elif defined( ST_OS21 )
    #define STPTI_TASK_STACK_SIZE OS21_DEF_MIN_STACK_SIZE
  #elif defined ( ST_OSLINUX )
    #define STPTI_TASK_STACK_SIZE 16384
  #elif defined(ST_UCOS_SYSTEM)
    #define STPTI_TASK_STACK_SIZE 16384
  #else
    #error ST_OS20,ST_OS21,ST_OSLINUX not found.
  #endif
#endif /* #ifndef STPTI_TASK_STACK_SIZE */


#define CAROUSEL_TASK_STACK   (STPTI_TASK_STACK_SIZE)
#define EVENT_Q_STACK         (STPTI_TASK_STACK_SIZE)
#define   INT_Q_STACK         (STPTI_TASK_STACK_SIZE)
#define INDEX_Q_STACK         (STPTI_TASK_STACK_SIZE)

/*** address translation *****************************************************************************************/

#if defined( ST_OSLINUX )
/* System uses MMU */
#define TRANSLATE_PHYS_ADD(_add_)   	    	    	    	    (_add_)
#define TRANSLATE_LOG_ADD(_add_, _region_)   	    	    	    (_add_)
#define NONCACHED_TRANSLATION(_add_)	    	    	    	    (_add_)

#elif defined( ST_OS20 )
#if defined( ARCHITECTURE_ST20 )
/* System has direct mapped memory structure (no MMU) */
#define TRANSLATE_PHYS_ADD(_add_)   	    	    	    	    (_add_)
#define TRANSLATE_LOG_ADD(_add_, _region_)   	    	    	    (_add_)
#define NONCACHED_TRANSLATION(_add_)	    	    	    	    (_add_)
#endif

#elif defined( ST_OS21 ) || defined( ST_UCOS_SYSTEM )

#if defined( ARCHITECTURE_ST40 )
/* System setup to have a multiply mapped memory structure cached & uncached (but not via MMU) */
#define TRANSLATE_PHYS_ADD(_add_)   	    	    	    	    ST40_PHYS_ADDR((_add_))
#define TRANSLATE_LOG_ADD(_add_, _region_)  	    	    	    (((U32)(_add_) | ((U32) (_region_) & ~ST40_PHYS_MASK )))
#define NONCACHED_TRANSLATION(_add_)	    	    	    	    ST40_NOCACHE_NOTRANSLATE((_add_))

#elif defined( ARCHITECTURE_ST200 )
/* System has been setup to have a direct mapped memory structure (not via MMU) */
#define TRANSLATE_PHYS_ADD(_add_)   	    	    	    	    (_add_)
#define TRANSLATE_LOG_ADD(_add_, _region_)   	    	    	    (_add_)
#define NONCACHED_TRANSLATION(_add_)	    	    	    	    (_add_)

#endif

#endif



/*** Critical Section Macros ***********************************************************************************/
/* There are a few cases in the driver where we need to prevent task switching and require critical timing.    */

#define STPTI_CRITICAL_SECTION_BEGIN     STOS_InterruptLock();
#define STPTI_CRITICAL_SECTION_END       STOS_InterruptUnlock()


/*** Interrupt handling Macros **********************************************************************************/
#if defined( ST_OS20 )

#define STPTI_SYSTEM_IRQ_ENABLE interrupt_enable_global( );
#elif defined(ST_OSLINUX)

#define  STPTI_SYSTEM_IRQ_ENABLE local_irq_enable( );

#else    /* Equivalent for OS21 is not need or unsed. */

#define  STPTI_SYSTEM_IRQ_ENABLE ;

#endif


/*** assembly nop delays **************************************************************************************/
/* Very short delays, often used for critical sections */

#if defined( ARCHITECTURE_ST200 )
#define __ASM_NOP                   __asm__ __volatile__ ("nop")

#elif defined( ARCHITECTURE_ST40 )
#define __ASM_NOP                   __asm__ __volatile__ ("nop")

#elif defined( ARCHITECTURE_ST20 )
#define __ASM_NOP                   __asm{ldc 0;}
#endif

#define __ASM_12_NOPS               __ASM_NOP;__ASM_NOP;__ASM_NOP;__ASM_NOP;__ASM_NOP;__ASM_NOP; \
                                    __ASM_NOP;__ASM_NOP;__ASM_NOP;__ASM_NOP;__ASM_NOP;__ASM_NOP




/*** semaphore debug ******************************************************************************************/

/* Whilst unusual to check the Semaphore count, it is good debugging facility, for our memory lock semaphore. */

#if defined( ST_OS20 )
#define SEMAPHORE_P_COUNT(_sem_)                                _sem_->semaphore_count

#elif defined( ST_OS21 )
#define SEMAPHORE_P_COUNT(_sem_)                                semaphore_value((semaphore_t *)_sem_)

#elif defined( ST_OSLINUX )
/* #define SEMAPHORE_P_COUNT(_sem_)                              _sem_->Count
The Grenoble implementation of semaphores increments the count on wait rather than decrementing it as OS20 and OS21 expects.
Hence all assetions that check for lock==0 fail. This avoids the problem.  It would be nice to fix, but time, etc...
*/
#define SEMAPHORE_P_COUNT(_sem_)                                (0)

#elif  defined(ST_UCOS_SYSTEM)
#define SEMAPHORE_P_COUNT(_sem_)                                (0)

#endif

void *stpti_MemoryAlign(void* Memory_p, U32 Alignment);

/* ST20 Compiler does not support inline declarations */
void stpti_AtomicSetBit( void* Address_p, U32 BitNumber );
void stpti_AtomicClearBit( void* Address_p, U32 BitNumber );

#endif /* #ifndef _OSX_H_ */
