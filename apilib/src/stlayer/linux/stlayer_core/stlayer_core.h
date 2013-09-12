/*****************************************************************************
 *
 *  Module      : stlayer_core
 *  Date        : 13-11-2005
 *  Author      : AYARITAR
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef STLAYER_CORE_H
#define STLAYER_CORE_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */
#include "stdevice.h"

#if defined(ST_5528) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#define LAYER_COMPOSITOR_MAPPING        COMPOSITOR_BASE_ADDRESS
#define DISP0BaseAddress                DISP0_BASE_ADDRESS
#define DISP1BaseAddress                DISP1_BASE_ADDRESS
#endif

#if defined(ST_7200)
#define DISP2BaseAddress                DISP2_BASE_ADDRESS
#endif

#if defined(ST_5528) || defined(ST_7100) || defined(ST_7109)
#define DISP2BaseAddress                0   /* DISP2BaseAddress set to 0 as DISP2 is not existing on these chips. */
#endif

typedef struct STLAYERMod_Param_s
{
    unsigned long LayerBaseAddress;	/* Vtg Register base address to map */
    unsigned long LayerAddressWidth;  /* Vtg address range */
	unsigned long LayerMappingIndex; /* Display vs compositor */
} STLAYERMod_Param_t;

#define STLAYER_MAPPED_WIDTH            0x1000
/*** IOCtl defines ***/

#define STLAYER_CORE_TYPE   0XFF     /* Unique module id - See 'ioctl-number.txt' */

#define STLAYER_CORE_CMD0   _IO  (STLAYER_CORE_TYPE, 0)         /* icoctl() - no argument */
#define STLAYER_CORE_CMD1   _IOR (STLAYER_CORE_TYPE, 1, int)    /* icoctl() - read an int argument */
#define STLAYER_CORE_CMD2   _IOW (STLAYER_CORE_TYPE, 2, long)   /* icoctl() - write a long argument */
#define STLAYER_CORE_CMD3   _IOWR(STLAYER_CORE_TYPE, 3, double) /* icoctl() - read/write a double argument */



#endif
