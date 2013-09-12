/*****************************************************************************
File Name   : cachereg.h

Description : CACHE registers for ST20TP3

              This module contains definitions for configuring the system
              on-board instruction and data caches.

Copyright (C) 2000 STMicroelectronics

History     :

    24/05/00  Modified stucture from STi5510 - cache regions 1 and 2
              are not used on the TP3.

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Cache control registers */
#define CACHE_CONTROL_0         0x000 /* Not used */
#define CACHE_CONTROL_1         0x100 /* Not used */
#define CACHE_CONTROL_2         0x200
#define CACHE_CONTROL_3         0x300
#define DCACHE_NOT_SRAM         0x400
#define INVALIDATE_DCACHE       0x500
#define FLUSH_DCACHE            0x600
#define ENABLE_ICACHE           0x700
#define INVALIDATE_ICACHE       0x800
#define CACHE_STATUS            0x900
#define CACHE_CONTROL_LOCK      0xA00

/* The following lookup-table defines blocks of memory that can
 * be programmed to be cacheable.
 *
 * Each entry in the LUT defines a start address, end address,
 * cache control register address and a bitmask value to be
 * logically or'ed into the cache control register.
 *
 * A region is defined to be a set of contiguous cacheable areas.
 * An end of region LUT entry is marked by an entry of all '-1's.
 *
 * Note that where regions overlap it is the responsibility of
 * the LUT search algorithm to ensure that all the required
 * cache control bits are set accordingly.
 *
 * The end of the LUT is marked by an entry of all zeros.
 */
#define DCACHE_LUT \
{ \
    /***************** REGION 0 ********************/ \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /***************** REGION 1 ********************/ \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /***************** REGION 2 ********************/ \
    { 0x40000000, 0x4000FFFF, CACHE_CONTROL_2, 0x01 }, \
    { 0x40010000, 0x4001FFFF, CACHE_CONTROL_2, 0x02 }, \
    { 0x40020000, 0x4002FFFF, CACHE_CONTROL_2, 0x04 }, \
    { 0x40030000, 0x4003FFFF, CACHE_CONTROL_2, 0x08 }, \
    { 0x40040000, 0x4004FFFF, CACHE_CONTROL_2, 0x10 }, \
    { 0x40050000, 0x4005FFFF, CACHE_CONTROL_2, 0x20 }, \
    { 0x40060000, 0x4006FFFF, CACHE_CONTROL_2, 0x40 }, \
    { 0x40070000, 0x4007FFFF, CACHE_CONTROL_2, 0x80 }, \
    { 0x40080000, 0x4FFFFFFF, CACHE_CONTROL_3, 0x01 }, \
    /***************** REGION 3 ********************/ \
    { 0x40000000, 0x4FFFFFFF, CACHE_CONTROL_3, 0x01 }, \
    { 0x50000000, 0x5FFFFFFF, CACHE_CONTROL_3, 0x02 }, \
    { 0x60000000, 0x6FFFFFFF, CACHE_CONTROL_3, 0x04 }, \
    { 0x70000000, 0x7FFFFFFF, CACHE_CONTROL_3, 0x08 }, \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /******************* EOT ***********************/ \
    { 0x00000000, 0x00000000, 0,               0x00 } \
}

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

/* End of cachereg.h */
