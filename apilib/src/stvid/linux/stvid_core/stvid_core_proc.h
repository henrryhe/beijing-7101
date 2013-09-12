/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2007.  All rights reserved.
 *
 *  Module      : stvid_core_proc.h
 *  Date        : 11th-07-2007
 *  Author      : Haithem ELLAFI
 *  Description : Header for proc filesystem
 *
 *****************************************************************************/


#ifndef STVID_CORE_PROC_H
#define STVID_CORE_PROC_H


#include "stvid.h"

int stvid_core_init_proc_fs(char *Name);
int stvid_core_cleanup_proc_fs(void);



#endif
