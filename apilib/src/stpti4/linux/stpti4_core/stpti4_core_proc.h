/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_core_proc.h
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
 *  Description : Header for proc filesystem
 *
 *****************************************************************************/


#ifndef STPTI4_CORE_PROC_H
#define STPTI4_CORE_PROC_H

#include "stpti.h"
#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"
#include "memget.h"

int stpti4_core_init_proc_fs(void);
int stpti4_core_cleanup_proc_fs(void);

void AddPTIDeviceToProcFS( FullHandle_t DeviceHandle );
void RemovePTIDeviceFromProcFS( FullHandle_t DeviceHandle );

void AddPTIBufferToProcFS     ( FullHandle_t BufferHandle );
void RemovePTIBufferFromProcFS( FullHandle_t BufferHandle );

void AddPTISignalToProcFS( FullHandle_t SignalHandle );
void RemovePTISignalFromProcFS( FullHandle_t SignalHandle );
    
#endif /* STPTI4_CORE_PROC_H */

