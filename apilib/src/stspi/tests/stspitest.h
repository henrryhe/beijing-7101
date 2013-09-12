/*****************************************************************************
File Name   : stspitest.h

Description : STSPI test harness header file.

Copyright (C) 2004 STMicroelectronics

Reference   :

ST API Definition "SPI Driver API" DVD-API-331 Revision 1.1
*****************************************************************************/
#ifndef __STSPITEST_H
#define __STSPITEST_H


#ifdef __cplusplus
extern "C" {
#endif

#include "sttbx.h"

/* FLASH Specific commands */

#define FLASH_READ_STATUS       0x05
#define FLASH_WRITE_STATUS      0x01
#define FLASH_PAGE_PROGRAM      0x02
#define FLASH_DATA_READ         0x03
#define FLASH_FAST_READ         0x0B
#define FLASH_SECTOR_ERASE      0xD8
#define FLASH_BULK_ERASE        0xC7
#define FLASH_WRITE_ENABLE      0x06
#define FLASH_WRITE_DISABLE     0x04

#if !defined(STSPI_NO_TBX)
#define STSPI_Print(x)          STTBX_Print(x);
#else
#define STSPI_Print(x)          printf x;
#endif

/*****************************************************************************
Global data
*****************************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STSPITEST_H */
