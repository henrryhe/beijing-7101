/**
 * Copyright (c) 2003-2007 STMicroelectronics Limited. All rights reserved.
 *
 * Low-level Cryptocore driver to send and receive the basic hardware level
 * Cryptocore commands to all different versions of the Cryptocore:
 *   * Cryptocore 1 and 1.5:
 *       - Stop, Restart, Restart in Linked-List mode
 *   * Cryptocore 2:
 *       - Add region, remove region, add a linked list of regions
 *   * All Cryptocores:
 *       - Pause
 *
 * To enable, ensure one of these is defined:
 *   CRYPTOCORE1
 *   CRYPTOCORE1_5
 *   CRYPTOCORE2
 * and CRYPTOCORE_BASE is defined to be the base address of the Cryptocore.
 *
 * CT_START_ADDR should be defined to the address at which the config table
 * start address is stored (BOOT_GAP_LO if boot vector is below gap, or
 * BOOT_GAP_HI - 4 if the boot vector is above the gap).  It does not have to be
 * if one of the defaults which can be found below is appropriate.
 *
 * MAX_PAUSE_IN_CYCLE can also be defined if it is not the default value
 * (96 for Cryptocore 1 or 1.5, 32 for Cryptocore 2)
 *
 * By default the code will be put in a section called PLACE_WITH_BOOTSTRAP on
 * the ST20.  To prevent this, define the symbol NO_PLACE_WITH_BOOTSTRAP.
 *
 * Note about caches: 
 * Reads and writes are potentially via cached memory, so cache purging code is
 * included using OS20/OS21 for functions which need it.  Where calls are made
 * to this code from bootstraps, caches will be purged/flushed on return (if
 * caches are on and not write-thru) for those functions which have a CachesOn
 * version.
 **/

#include <stdlib.h>
#include <string.h>


/*zxg20080605 add definition for STi7101*/
#define CRYPTOCORE_BASE 0xb9200000
#define CRYPTOCORE2
#define BASE(BYTE_OFFSET)               ((unsigned int*)&(((unsigned char*)CRYPTOCORE_BASE)[BYTE_OFFSET]))


/*zxg20080605 end*/



#ifdef CRYPTOCORE1


#elif defined(CRYPTOCORE1_5)

#elif defined(CRYPTOCORE2)

/* Bitmasks for Cryptocore commands/status */
#define     MAILBOX_ADD_REG_REQ         0x00010000
#define     MAILBOX_ADD_REG_ACK         0x00000001
#define     MAILBOX_REM_REG_REQ         0x00020000
#define     MAILBOX_REM_REG_ACK         0x00000002
#define     MAILBOX_PAUSE_REQ           0x00040000
#define     MAILBOX_PAUSE_ACK           0x00000004
#define     MAILBOX_SIGDMA_REQ          0x01000000
#define     MAILBOX_SIGDMA_ACK          0x00000100
#define     MAILBOX_SIGDMA_ERR          0x00000200
#define     CC_FAILURE_MASK             0x00000FFF

/* Offsets in bytes from Cryptocore base address */
#define     SIGCHK_REGS                 0x4004
#define     SIGDMA_REGS                 0x4014
#define     CHECK_MAILBOX_STATUS        0x5E2C
#define     CHECK_MAILBOX_SET           0x5E30
#define     CHECK_MAILBOX_CLEAR         0x5E34
#define     CHECK_SIGCHK_STATUS         0x5E3C
#define     CHECK_FAILURE_SOURCE        0x5E40

#else
#error No Cryptocore version defined - define one of: CRYPTOCORE1, CRYPTOCORE1_5 or CRYPTOCORE2
#endif /* Different Cryptocore versions */







#ifdef CRYPTOCORE2

/*****************************************************************************
cryptRemoveRegionNumber()
Description:
    Removes a single signed block from what is being checked by the Cryptocore.
    Takes a region number as argument.
    Returns non-zero on failure, 0 for success.
*****************************************************************************/

__inline int cryptRemoveRegionNumber(int regionNumber)
{
 
 unsigned int i_fail = 0;
#if (CRYPTOCORE_BASE != 0)
  /* Only the block/region number register needs to be set for a remove region
     request.
   */
  *BASE(SIGCHK_REGS + 12) = regionNumber;
  
  /* Send the remove region request */
  *BASE(CHECK_MAILBOX_SET) = MAILBOX_REM_REG_REQ;
  
  /* Wait for acknowledge */
  i_fail = 0;
  while (((*BASE(CHECK_MAILBOX_STATUS) & MAILBOX_REM_REG_ACK) != MAILBOX_REM_REG_ACK &&
         (*BASE(CHECK_FAILURE_SOURCE) & CC_FAILURE_MASK) == 0) && i_fail < 5)
  {
   i_fail++;
  task_delay( ST_GetClocksPerSecondLow() / 10 );
  }

  if(i_fail >= 5)
  {
  do_report(0,"Check1 TO\n");
  }
#if 1
  i_fail = *BASE(CHECK_FAILURE_SOURCE);
  if (i_fail & CC_FAILURE_MASK)
    return -1;
#endif  
  /* Clear acknowledge */
  *BASE(CHECK_MAILBOX_CLEAR) = MAILBOX_REM_REG_ACK;
#endif /* (CRYPTOCORE_BASE != 0) */
  return 0;
}


#endif /* CRYPTOCORE2 */
