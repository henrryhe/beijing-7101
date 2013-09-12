/*****************************************************************************
File Name   : cachereg.h

Description : CACHE registers for 5107

              This module contains definitions for configuring the system
              on-board instruction and data caches.

Copyright (C) 2006 STMicroelectronics

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Cache control registers */

/* Cacheable region register offsets */
/* Note: In 5107, the Dcache is on a lower address than the Icache. So here the 0x1000 offset is
   added to the Icache register offsets instead of the dcache addresses , differently from the other
   devices (5514,5516,5517,5528,5100,5101)*/
#define ICAC_Region0Enable       (0x1000 + 0x020)
#define ICAC_Region1BlockEnable  (0x1000 + 0x028)
#define ICAC_Region1TopEnable    (0x1000 + 0x02C)
#define ICAC_Region2Enable       (0x1000 + 0x030)
#define ICAC_Region3BlockEnable  (0x1000 + 0x038)
#define ICAC_Region3BankEnable   (0x1000 + 0x03C)

#define DCAC_Region0Enable       0x020
#define DCAC_Region1BlockEnable  0x028
#define DCAC_Region1TopEnable    0x02C
#define DCAC_Region2Enable       0x030
#define DCAC_Region3BlockEnable  0x038
#define DCAC_Region3BankEnable   0x03C

/* Global cache registers */
#define ENABLE_ICACHE           (0x1000 + 0x000)
#define INVALIDATE_ICACHE       (0x1000 + 0x010)

#define ENABLE_DCACHE           0x000
#define INVALIDATE_DCACHE       0x010
#define FLUSH_DCACHE            0x014
#define CACHE_STATUS            0x018
#define DCACHE_NOT_SRAM         ENABLE_DCACHE

#define ICACHE_CONTROL_LOCK     (0x1000 + 0x004)
#define DCACHE_CONTROL_LOCK     0x004


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
 */
#define DCAC_REGION_1 \
    { 0xC0000000, 0xC007FFFF, DCAC_Region1BlockEnable, 0x0001 }, \
    { 0xC0080000, 0xC00FFFFF, DCAC_Region1BlockEnable, 0x0002 }, \
    { 0xC0100000, 0xC017FFFF, DCAC_Region1BlockEnable, 0x0004 }, \
    { 0xC0180000, 0xC01FFFFF, DCAC_Region1BlockEnable, 0x0008 }, \
    { 0xC0200000, 0xC027FFFF, DCAC_Region1BlockEnable, 0x0010 }, \
    { 0xC0280000, 0xC02FFFFF, DCAC_Region1BlockEnable, 0x0020 }, \
    { 0xC0300000, 0xC037FFFF, DCAC_Region1BlockEnable, 0x0040 }, \
    { 0xC0380000, 0xC03FFFFF, DCAC_Region1BlockEnable, 0x0080 }, \
    { 0xC0400000, 0xC047FFFF, DCAC_Region1BlockEnable, 0x0100 }, \
    { 0xC0480000, 0xC04FFFFF, DCAC_Region1BlockEnable, 0x0200 }, \
    { 0xC0500000, 0xC057FFFF, DCAC_Region1BlockEnable, 0x0400 }, \
    { 0xC0580000, 0xC05FFFFF, DCAC_Region1BlockEnable, 0x0800 }, \
    { 0xC0600000, 0xC067FFFF, DCAC_Region1BlockEnable, 0x1000 }, \
    { 0xC0680000, 0xC06FFFFF, DCAC_Region1BlockEnable, 0x2000 }, \
    { 0xC0700000, 0xC077FFFF, DCAC_Region1BlockEnable, 0x4000 }, \
    { 0xC0780000, 0xC07FFFFF, DCAC_Region1BlockEnable, 0x8000 }, \
    { 0xC0800000, 0xFFFFFFFF, DCAC_Region1TopEnable,   0x0001 },

#define DCAC_REGION_3 \
    { 0x40000000, 0x4007FFFF, DCAC_Region3BlockEnable, 0x0001 }, \
    { 0x40080000, 0x400FFFFF, DCAC_Region3BlockEnable, 0x0002 }, \
    { 0x40100000, 0x4017FFFF, DCAC_Region3BlockEnable, 0x0004 }, \
    { 0x40180000, 0x401FFFFF, DCAC_Region3BlockEnable, 0x0008 }, \
    { 0x40200000, 0x4027FFFF, DCAC_Region3BlockEnable, 0x0010 }, \
    { 0x40280000, 0x402FFFFF, DCAC_Region3BlockEnable, 0x0020 }, \
    { 0x40300000, 0x4037FFFF, DCAC_Region3BlockEnable, 0x0040 }, \
    { 0x40380000, 0x403FFFFF, DCAC_Region3BlockEnable, 0x0080 }, \
    { 0x40400000, 0x4047FFFF, DCAC_Region3BlockEnable, 0x0100 }, \
    { 0x40480000, 0x404FFFFF, DCAC_Region3BlockEnable, 0x0200 }, \
    { 0x40500000, 0x4057FFFF, DCAC_Region3BlockEnable, 0x0400 }, \
    { 0x40580000, 0x405FFFFF, DCAC_Region3BlockEnable, 0x0800 }, \
    { 0x40600000, 0x4067FFFF, DCAC_Region3BlockEnable, 0x1000 }, \
    { 0x40680000, 0x406FFFFF, DCAC_Region3BlockEnable, 0x2000 }, \
    { 0x40700000, 0x4077FFFF, DCAC_Region3BlockEnable, 0x4000 }, \
    { 0x40780000, 0x407FFFFF, DCAC_Region3BlockEnable, 0x8000 }, \
    { 0x40800000, 0x4FFFFFFF, DCAC_Region3BankEnable,  0x0001 }, \
    { 0x50000000, 0x5FFFFFFF, DCAC_Region3BankEnable,  0x0002 }, \
    { 0x60000000, 0x6FFFFFFF, DCAC_Region3BankEnable,  0x0004 }, \
    { 0x70000000, 0x7FFFFFFF, DCAC_Region3BankEnable,  0x0008 },


#define DCACHE_LUT \
{ \
    /***************** REGION 0 (Internal SRAM) ****/ \
    /* Not cacheable on the 5107 */ \
    /***************** REGION 1 (Shared mem) *******/ \
    DCAC_REGION_1 \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /***************** REGION 2 (Peripherals) ******/ \
    /* Not cacheable on the 5107 */ \
    /***************** REGION 3 (EMI) **************/ \
    DCAC_REGION_3 \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /******************* EOT ***********************/ \
    { 0x00000000, 0x00000000, 0,               0x00 } \
}

#define ICAC_REGION_1 \
    { 0xC0000000, 0xC007FFFF, ICAC_Region1BlockEnable, 0x0001 }, \
    { 0xC0080000, 0xC00FFFFF, ICAC_Region1BlockEnable, 0x0002 }, \
    { 0xC0100000, 0xC017FFFF, ICAC_Region1BlockEnable, 0x0004 }, \
    { 0xC0180000, 0xC01FFFFF, ICAC_Region1BlockEnable, 0x0008 }, \
    { 0xC0200000, 0xC027FFFF, ICAC_Region1BlockEnable, 0x0010 }, \
    { 0xC0280000, 0xC02FFFFF, ICAC_Region1BlockEnable, 0x0020 }, \
    { 0xC0300000, 0xC037FFFF, ICAC_Region1BlockEnable, 0x0040 }, \
    { 0xC0380000, 0xC03FFFFF, ICAC_Region1BlockEnable, 0x0080 }, \
    { 0xC0400000, 0xC047FFFF, ICAC_Region1BlockEnable, 0x0100 }, \
    { 0xC0480000, 0xC04FFFFF, ICAC_Region1BlockEnable, 0x0200 }, \
    { 0xC0500000, 0xC057FFFF, ICAC_Region1BlockEnable, 0x0400 }, \
    { 0xC0580000, 0xC05FFFFF, ICAC_Region1BlockEnable, 0x0800 }, \
    { 0xC0600000, 0xC067FFFF, ICAC_Region1BlockEnable, 0x1000 }, \
    { 0xC0680000, 0xC06FFFFF, ICAC_Region1BlockEnable, 0x2000 }, \
    { 0xC0700000, 0xC077FFFF, ICAC_Region1BlockEnable, 0x4000 }, \
    { 0xC0780000, 0xC07FFFFF, ICAC_Region1BlockEnable, 0x8000 }, \
    { 0xC0800000, 0xFFFFFFFF, ICAC_Region1TopEnable,   0x0001 },

#define ICAC_REGION_3 \
    { 0x40000000, 0x4007FFFF, ICAC_Region3BlockEnable, 0x0001 }, \
    { 0x40080000, 0x400FFFFF, ICAC_Region3BlockEnable, 0x0002 }, \
    { 0x40100000, 0x4017FFFF, ICAC_Region3BlockEnable, 0x0004 }, \
    { 0x40180000, 0x401FFFFF, ICAC_Region3BlockEnable, 0x0008 }, \
    { 0x40200000, 0x4027FFFF, ICAC_Region3BlockEnable, 0x0010 }, \
    { 0x40280000, 0x402FFFFF, ICAC_Region3BlockEnable, 0x0020 }, \
    { 0x40300000, 0x4037FFFF, ICAC_Region3BlockEnable, 0x0040 }, \
    { 0x40380000, 0x403FFFFF, ICAC_Region3BlockEnable, 0x0080 }, \
    { 0x40400000, 0x4047FFFF, ICAC_Region3BlockEnable, 0x0100 }, \
    { 0x40480000, 0x404FFFFF, ICAC_Region3BlockEnable, 0x0200 }, \
    { 0x40500000, 0x4057FFFF, ICAC_Region3BlockEnable, 0x0400 }, \
    { 0x40580000, 0x405FFFFF, ICAC_Region3BlockEnable, 0x0800 }, \
    { 0x40600000, 0x4067FFFF, ICAC_Region3BlockEnable, 0x1000 }, \
    { 0x40680000, 0x406FFFFF, ICAC_Region3BlockEnable, 0x2000 }, \
    { 0x40700000, 0x4077FFFF, ICAC_Region3BlockEnable, 0x4000 }, \
    { 0x40780000, 0x407FFFFF, ICAC_Region3BlockEnable, 0x8000 }, \
    { 0x40800000, 0x4FFFFFFF, ICAC_Region3BankEnable,  0x0001 }, \
    { 0x50000000, 0x5FFFFFFF, ICAC_Region3BankEnable,  0x0002 }, \
    { 0x60000000, 0x6FFFFFFF, ICAC_Region3BankEnable,  0x0004 }, \
    { 0x70000000, 0x7FFFFFFF, ICAC_Region3BankEnable,  0x0008 },


#define ICACHE_LUT \
{ \
    /***************** REGION 0 (Internal SRAM) ****/ \
    /* Not cacheable on the 5107 */ \
    /***************** REGION 1 (Shared mem) *******/ \
    ICAC_REGION_1 \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /***************** REGION 2 (Peripherals) ******/ \
    /* Not cacheable on the 5107 */ \
    /***************** REGION 3 (EMI) **************/ \
    ICAC_REGION_3 \
    { ((U32)-1),  ((U32)-1),  ((U32)-1),   ((U8)-1) }, \
    /******************* EOT ***********************/ \
    { 0x00000000, 0x00000000, 0,               0x00 } \
}


/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

/* End of cachereg.h */
