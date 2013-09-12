/*****************************************************************************
File Name   : cachereg.h

Description : CACHE registers for 5518

              This module contains definitions for configuring the system
              on-board instruction and data caches.

Copyright (C) 2000 STMicroelectronics

History     :

    16/02/01  First attempt based on STi5512.

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Cache control registers */

/* Cacheable region register offsets */
#define CAC_CacheControl0       0x000
#define CAC_CacheControl1       0x100
#define CAC_EMIBank0            0x300
#define CAC_EMIBank1            0x300
#define CAC_EMIBank2            0x300
#define CAC_EMIBank3            0x300
#define CAC_EMIBlock0           0x200

/* Global cache registers */
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
 *
 * IMPORTANT NOTE:  The 5518 cache control for REGION 1 allows
 * two modes of operation i.e., 64Kbit or 512Kbit block sizes.
 * Since the block definitions are overloaded into
 * CAC_CacheControl0 and CAC_CacheControl1 it is not possible
 * to support both simultaneously in the current lookup-table
 * framework.  64Kbit is the default option.
 */
#ifndef STBOOT_DCACHE_REGION0_512KBIT
#define CAC_REGION_0 \
    { 0xC0000000, 0xC000FFFF, CAC_CacheControl0, 0x01 }, \
    { 0xC0010000, 0xC001FFFF, CAC_CacheControl0, 0x02 }, \
    { 0xC0020000, 0xC002FFFF, CAC_CacheControl0, 0x04 }, \
    { 0xC0030000, 0xC003FFFF, CAC_CacheControl0, 0x08 }, \
    { 0xC0040000, 0xC004FFFF, CAC_CacheControl0, 0x10 }, \
    { 0xC0050000, 0xC005FFFF, CAC_CacheControl0, 0x20 }, \
    { 0xC0060000, 0xC006FFFF, CAC_CacheControl0, 0x40 }, \
    { 0xC0070000, 0xC007FFFF, CAC_CacheControl0, 0x80 }, \
    { 0xC0080000, 0xC008FFFF, CAC_CacheControl1, 0x01 }, \
    { 0xC0090000, 0xC009FFFF, CAC_CacheControl1, 0x02 }, \
    { 0xC00A0000, 0xC00AFFFF, CAC_CacheControl1, 0x04 }, \
    { 0xC00B0000, 0xC00BFFFF, CAC_CacheControl1, 0x08 }, \
    { 0xC00C0000, 0xC00CFFFF, CAC_CacheControl1, 0x10 }, \
    { 0xC00D0000, 0xC00DFFFF, CAC_CacheControl1, 0x20 }, \
    { 0xC00E0000, 0xC00EFFFF, CAC_CacheControl1, 0x40 }, \
    { 0xC00F0000, 0xC00FFFFF, CAC_CacheControl1, 0x80 },
#else
#define CAC_REGION_0 \
    { 0xC0000000, 0xC07FFFFF, CAC_CacheControl0, 0x100 }, \
    { 0xC0000000, 0xC007FFFF, CAC_CacheControl0, 0x01 }, \
    { 0xC0080000, 0xC00FFFFF, CAC_CacheControl0, 0x02 }, \
    { 0xC0100000, 0xC017FFFF, CAC_CacheControl0, 0x04 }, \
    { 0xC0180000, 0xC01FFFFF, CAC_CacheControl0, 0x08 }, \
    { 0xC0200000, 0xC027FFFF, CAC_CacheControl0, 0x10 }, \
    { 0xC0280000, 0xC02FFFFF, CAC_CacheControl0, 0x20 }, \
    { 0xC0300000, 0xC037FFFF, CAC_CacheControl0, 0x40 }, \
    { 0xC0380000, 0xC03FFFFF, CAC_CacheControl0, 0x80 }, \
    { 0xC0400000, 0xC047FFFF, CAC_CacheControl1, 0x01 }, \
    { 0xC0480000, 0xC04FFFFF, CAC_CacheControl1, 0x02 }, \
    { 0xC0500000, 0xC057FFFF, CAC_CacheControl1, 0x04 }, \
    { 0xC0580000, 0xC05FFFFF, CAC_CacheControl1, 0x08 }, \
    { 0xC0600000, 0xC067FFFF, CAC_CacheControl1, 0x10 }, \
    { 0xC0680000, 0xC06FFFFF, CAC_CacheControl1, 0x20 }, \
    { 0xC0700000, 0xC077FFFF, CAC_CacheControl1, 0x40 }, \
    { 0xC0780000, 0xC07FFFFF, CAC_CacheControl1, 0x80 },
#endif

#define DCACHE_LUT \
{ \
    /***************** REGION 0 (Internal SRAM) ****/ \
    /* Not cacheable on the 5518 */ \
    /***************** REGION 1 (Shared mem) *******/ \
    CAC_REGION_0 \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /***************** REGION 2 (Peripherals) ******/ \
    /* Not cacheable on the 5518 */ \
    /***************** REGION 3 (EMI) **************/ \
    { 0x40000000, 0x4FFFFFFF, CAC_EMIBank0, 0x01 }, \
    { 0x40080000, 0x4FFFFFFF, CAC_EMIBank0, 0x01 }, \
    { 0x50000000, 0x5FFFFFFF, CAC_EMIBank1, 0x02 }, \
    { 0x60000000, 0x6FFFFFFF, CAC_EMIBank2, 0x04 }, \
    { 0x70000000, 0x7FFFFFFF, CAC_EMIBank3, 0x08 }, \
    { 0x40000000, 0x4000FFFF, CAC_EMIBlock0, 0x01 }, \
    { 0x40010000, 0x4001FFFF, CAC_EMIBlock0, 0x02 }, \
    { 0x40020000, 0x4002FFFF, CAC_EMIBlock0, 0x04 }, \
    { 0x40030000, 0x4003FFFF, CAC_EMIBlock0, 0x08 }, \
    { 0x40040000, 0x4004FFFF, CAC_EMIBlock0, 0x10 }, \
    { 0x40050000, 0x4005FFFF, CAC_EMIBlock0, 0x20 }, \
    { 0x40060000, 0x4006FFFF, CAC_EMIBlock0, 0x40 }, \
    { 0x40070000, 0x4007FFFF, CAC_EMIBlock0, 0x80 }, \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /******************* EOT ***********************/ \
    { 0x00000000, 0x00000000, 0,               0x00 } \
}

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

/* End of cachereg.h */
