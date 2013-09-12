/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_ioctl_cfg.h
 *  Date        : 19-05-2005
 *  Author      : STIEGLITZP
 *  Description : Definitions necessary for configuration
 *
 *****************************************************************************/

#include "stpti.h"
#include "stdevice.h"
#include "pti4.h"
#include "pti_loc.h"
#include "pti_hndl.h"
#include "tchal.h"

typedef struct{
    const char *Name;
    u32 Address;        /* Physical Address of the PTI device */
    u32 Range;          /* The PTI register space length */
    u32 Interrupt;
    ST_ErrorCode_t               (*TCLoader_p) (STPTI_DevicePtr_t, void *);
    const char *LoaderName;
    STPTI_Device_t Architecture;
}PTI_Device_Info;

