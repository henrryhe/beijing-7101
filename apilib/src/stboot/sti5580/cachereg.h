/*****************************************************************************
File Name   : cachereg.h

Description : CACHE registers for 5510

              This module contains definitions for configuring the system
              on-board instruction and data caches.

Copyright (C) 1999 STMicroelectronics

History     :

    15/08/99  First attempt.

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Cache control registers */
#define CACHE_CONTROL_0         0x000
#define CACHE_CONTROL_1         0x100
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
    { 0xC0000000, 0xC000FFFF, CACHE_CONTROL_0, 0x01 }, \
    { 0xC0010000, 0xC001FFFF, CACHE_CONTROL_0, 0x02 }, \
    { 0xC0020000, 0xC002FFFF, CACHE_CONTROL_0, 0x04 }, \
    { 0xC0030000, 0xC003FFFF, CACHE_CONTROL_0, 0x08 }, \
    { 0xC0040000, 0xC004FFFF, CACHE_CONTROL_0, 0x10 }, \
    { 0xC0050000, 0xC005FFFF, CACHE_CONTROL_0, 0x20 }, \
    { 0xC0060000, 0xC006FFFF, CACHE_CONTROL_0, 0x40 }, \
    { 0xC0070000, 0xC007FFFF, CACHE_CONTROL_0, 0x80 }, \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /***************** REGION 1 ********************/ \
    { 0xC0200000, 0xC020FFFF, CACHE_CONTROL_1, 0x01 }, \
    { 0xC0210000, 0xC021FFFF, CACHE_CONTROL_1, 0x02 }, \
    { 0xC0220000, 0xC022FFFF, CACHE_CONTROL_1, 0x04 }, \
    { 0xC0230000, 0xC023FFFF, CACHE_CONTROL_1, 0x08 }, \
    { 0xC0240000, 0xC024FFFF, CACHE_CONTROL_1, 0x10 }, \
    { 0xC0250000, 0xC025FFFF, CACHE_CONTROL_1, 0x20 }, \
    { 0xC0260000, 0xC026FFFF, CACHE_CONTROL_1, 0x40 }, \
    { 0xC0270000, 0xC027FFFF, CACHE_CONTROL_1, 0x80 }, \
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
