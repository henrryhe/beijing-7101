/*****************************************************************************

File name   :  statareg.h

Description :  local registers and defines for ATA interface

COPYRIGHT (C) ST-Microelectronics 1999-2000.

Date               Modification                 Name
----               ------------                 ----
 02/12/00          Created                      FR

*****************************************************************************/
/* --- prevents recursive inclusion --------------------------------------- */
#ifndef __STATAREG_H
#define __STATAREG_H

/* --- allows C compiling with C++ compiler ------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif
/* --- includes ----------------------------------------------------------- */
#include "stddefs.h"
#include "stdevice.h"
#include "stsys.h"

/* --- specific defines ( hardware specific ) ----------------------------- */
#ifdef mb282b
#if 1
#define     BASE_ADDRESS                                    0x70B00000
#define     ATA_HRD_RST     *(volatile U8*)                 0x70A0000C
#define     ATA_HRD_RST_ASSERT                              0x0
#define     ATA_HRD_RST_DEASSERT                            0x1
#else
#define     BASE_ADDRESS                                    0x60000000
#define     ATA_HRD_RST     *(volatile U16*)                0x60100000
#define     ATA_HRD_RST_ASSERT                              0x0
#define     ATA_HRD_RST_DEASSERT                            0x1
#endif

#define     aCS1                                            0x00040000
#define     nCS1                                            0x00000000
#define     aCS0                                            0x00080000
#define     nCS0                                            0x00000000
#define     aDA2                                            0x00020000
#define     nDA2                                            0x00000000
#define     aDA1                                            0x00010000
#define     nDA1                                            0x00000000
#define     aDA0                                            0x00008000
#define     nDA0                                            0x00000000

/* end of #ifdef mb282b */
#elif defined (mb275) || defined(mb275_64)
#define     BASE_ADDRESS                                    0x50000000

#define     aCS1                                            0x00100000
#define     nCS1                                            0x00000000
#define     aCS0                                            0x00200000
#define     nCS0                                            0x00000000
#define     aDA2                                            0x00080000
#define     nDA2                                            0x00000000
#define     aDA1                                            0x00040000
#define     nDA1                                            0x00000000
#define     aDA0                                            0x00020000
#define     nDA0                                            0x00000000

/* end of #if defined(mb275) || defined(mb275_64) */
#elif defined (mb314)
#define     BASE_ADDRESS                                    0x20017000

#define     ATA_HRD_RST             *(STSYS_DU32*) (BASE_ADDRESS | 0x84 )
#define     ATA_HRD_RST_ASSERT      0x1
#define     ATA_HRD_RST_DEASSERT    0x0

#define     ATA_REG_HIGHIMPEDANCE   *(STSYS_DU32*) (BASE_ADDRESS | 0    )
#define     ATA_REG_ALTSTAT         *(STSYS_DU32*) (BASE_ADDRESS | 0x38 )
#define     ATA_REG_CONTROL         *(STSYS_DU32*) (BASE_ADDRESS | 0x38 )
/* command blocks except packet and service ------------------------------- */
#define     ATA_REG_DATA            *(STSYS_DU32*) (BASE_ADDRESS | 0x40 )
#define     ATA_REG_ERROR           *(STSYS_DU32*) (BASE_ADDRESS | 0x44 )
#define     ATA_REG_FEATURE         *(STSYS_DU32*) (BASE_ADDRESS | 0x44 )
#define     ATA_REG_SECCOUNT        *(STSYS_DU32*) (BASE_ADDRESS | 0x48 )
#define     ATA_REG_SECNUM          *(STSYS_DU32*) (BASE_ADDRESS | 0x4C )
#define     ATA_REG_CYLLOW          *(STSYS_DU32*) (BASE_ADDRESS | 0x50 )
#define     ATA_REG_CYLHIGH         *(STSYS_DU32*) (BASE_ADDRESS | 0x54 )
#define     ATA_REG_DEVHEAD         *(STSYS_DU32*) (BASE_ADDRESS | 0x58 )
#define     ATA_REG_STATUS          *(STSYS_DU32*) (BASE_ADDRESS | 0x5C )
#define     ATA_REG_COMMAND         *(STSYS_DU32*) (BASE_ADDRESS | 0x5C )
/* specific fields for command block for packet and service --------------- */
#define     ATA_REG_INTREASON       *(STSYS_DU32*) (BASE_ADDRESS | 0x48 )
#define     ATA_REG_BYTECOUNTLOW    *(STSYS_DU32*) (BASE_ADDRESS | 0x50 )
#define     ATA_REG_BYTECOUNTHIGH   *(STSYS_DU32*) (BASE_ADDRESS | 0x54 )
#define     ATA_REG_DEVSELECT       *(STSYS_DU32*) (BASE_ADDRESS | 0x58 )

#define     HDD_DMA_C               *(STSYS_DU32*) (BASE_ADDRESS | 0xB0 )
#define     HDD_DMA_SA              *(STSYS_DU32*) (BASE_ADDRESS | 0xB4 )
#define     HDD_DMA_WC              *(STSYS_DU32*) (BASE_ADDRESS | 0xB8 )
#define     HDD_DMA_SI              *(STSYS_DU32*) (BASE_ADDRESS | 0xBC )
#define     HDD_DMA_ITS             *(STSYS_DU32*) (BASE_ADDRESS | 0xE0 )
#define     HDD_DMA_STA             *(STSYS_DU32*) (BASE_ADDRESS | 0xE4 )
#define     HDD_DMA_ITM             *(STSYS_DU32*) (BASE_ADDRESS | 0xE8 )

/* --- HDDI_DMA_C register ( hardware specific ) ----------------------------------- */
#define     HDD_DMA_C_STARTBIT         0x00000001
#define     HDD_DMA_C_STOPBIT          0x00000002
#define     HDD_DMA_C_DMAENABLE        0x00000004
#define     HDD_DMA_C_NOTINCADDRESS    0x00000008
#define     HDD_DMA_C_READNOTWRITE     0x00000010
/* --- HDDI_DMA_SI register ( hardware specific ) ---------------------------------- */
#define     HDD_DMA_SI_16BYTES         0x00000000
#define     HDD_DMA_SI_32BYTES         0x00000001
#define     HDD_DMA_SI_64BYTES         0x00000002
#define     HDD_DMA_SI_UNDEFINED       0x00000003

#else
/* Dummy denitions to compile */
#define     BASE_ADDRESS                                    0x50000000

#define     aCS1    0
#define     nCS1    0
#define     aCS0    0
#define     nCS0    0
#define     aDA2    0
#define     nDA2    0
#define     aDA1    0
#define     nDA1    0
#define     aDA0    0
#define     nDA0    0

#endif

#if !defined (mb314)
#ifdef USE_BLOCK_MOVE_DMA
#define BMDMA_SOURCE    *(STSYS_DU32*) (BM_BASE_ADDRESS       )
#define BMDMA_DEST      *(STSYS_DU32*) (BM_BASE_ADDRESS + 0x04)
#define BMDMA_COUNT     *(STSYS_DU32*) (BM_BASE_ADDRESS + 0x08)
#define BMDMA_INTEN     *(STSYS_DU32*) (BM_BASE_ADDRESS + 0x0C)
#define BMDMA_STATUS    *(STSYS_DU32*) (BM_BASE_ADDRESS + 0x10)
#define BMDMA_INTACK    *(STSYS_DU32*) (BM_BASE_ADDRESS + 0x14)
#define BMDMA_ABORT     *(STSYS_DU32*) (BM_BASE_ADDRESS + 0x18)
#endif /* USE_BLOCK_MOVE_DMA */

/* control blocks except packet and service ------------------------------- */
#define     ATA_REG_HIGHIMPEDANCE   *(volatile U8*) (BASE_ADDRESS | nCS1 | nCS0 | nDA2 | nDA1 | nDA0)
#define     ATA_REG_ALTSTAT         *(volatile U8*) (BASE_ADDRESS | aCS1 | nCS0 | aDA2 | aDA1 | nDA0)
#define     ATA_REG_CONTROL         *(volatile U8*) (BASE_ADDRESS | aCS1 | nCS0 | aDA2 | aDA1 | nDA0)

/* command blocks except packet and service ------------------------------- */
#define     ATA_REG_DATA                            (BASE_ADDRESS | nCS1 | aCS0 | nDA2 | nDA1 | nDA0)
#define     ATA_REG_ERROR           *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | nDA2 | nDA1 | aDA0)
#define     ATA_REG_FEATURE         *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | nDA2 | nDA1 | aDA0)
#define     ATA_REG_SECCOUNT        *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | nDA2 | aDA1 | nDA0)
#define     ATA_REG_SECNUM          *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | nDA2 | aDA1 | aDA0)
#define     ATA_REG_CYLLOW          *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | aDA2 | nDA1 | nDA0)
#define     ATA_REG_CYLHIGH         *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | aDA2 | nDA1 | aDA0)
#define     ATA_REG_DEVHEAD         *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | aDA2 | aDA1 | nDA0)
#define     ATA_REG_STATUS          *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | aDA2 | aDA1 | aDA0)
#define     ATA_REG_COMMAND         *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | aDA2 | aDA1 | aDA0)

/* specific fields for command block for packet and service --------------- */
#define     ATA_REG_INTREASON       *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | nDA2 | aDA1 | nDA0)
#define     ATA_REG_BYTECOUNTLOW    *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | aDA2 | nDA1 | nDA0)
#define     ATA_REG_BYTECOUNTHIGH   *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | aDA2 | nDA1 | aDA0)
#define     ATA_REG_DEVSELECT       *(volatile U8*) (BASE_ADDRESS | nCS1 | aCS0 | aDA2 | aDA1 | nDA0)
#endif /* !mb314 */

/* standard defines ------------------------------------------------------- */

/* --- command values ----------------------------------------------------- */
#define     COMMAND_NOP                                         0x00
#define     COMMAND_CFA_REQUEST_EXTENDED_ERROR_CODE             0x03
#define     COMMAND_DEVICE_RESET                                0x08
#define     COMMAND_READ_SECTOR_0                               0x20
#define     COMMAND_READ_SECTOR_1                               0x21
#define     COMMAND_WRITE_SECTOR_0                              0x30
#define     COMMAND_WRITE_SECTOR_1                              0x31
#define     COMMAND_CFA_WRITE_SECTORS_WITHOUT_ERASE_            0x38
#define     COMMAND_READ_VERIFY_SECTOR_0                        0x40
#define     COMMAND_READ_VERIFY_SECTOR_1                        0x41
#define     COMMAND_SEEK                                        0x70
#define     COMMAND_CFA_TRANSLATE_SECTOR                        0x87
#define     COMMAND_EXECUTE_DEVICE_DIAGNOSTIC                   0x90
#define     COMMAND_INITIALIZE_DEVICE_PARAMETERS                0x91
#define     COMMAND_DOWNLOAD_MICROCODE                          0x92
#define     COMMAND_PACKET                                      0xA0
#define     COMMAND_IDENTIFY_PACKET_DEVICE                      0xA1
#define     COMMAND_SERVICE                                     0xA2
#define     COMMAND_SMART                                       0xB0
#define     COMMAND_CFA_ERASE_SECTORS                           0xC0
#define     COMMAND_READ_MULTIPLE                               0xC4
#define     COMMAND_WRITE_MULTIPLE                              0xC5
#define     COMMAND_SET_MULTIPLE_MODE                           0xC6
#define     COMMAND_READ_DMA_QUEUED                             0xC7
#define     COMMAND_READ_DMA_0                                  0xC8
#define     COMMAND_READ_DMA_1                                  0xC9
#define     COMMAND_WRITE_DMA_0                                 0xCA
#define     COMMAND_WRITE_DMA_1                                 0xCB
#define     COMMAND_WRITE_DMA_QUEUED                            0xCC
#define     COMMAND_CFA_WRITE_MULTIPLE_WITHOUT_ERASE            0xCD
#define     COMMAND_GET_MEDIA_STATUS                            0xDA
#define     COMMAND_MEDIA_LOCK                                  0xDE
#define     COMMAND_MEDIA_UNLOCK                                0xDF
#define     COMMAND_STANDBY_IMMEDIATE                           0xE0
#define     COMMAND_IDLE_IMMEDIATE                              0xE1
#define     COMMAND_STANDBY                                     0xE2
#define     COMMAND_IDLE                                        0xE3
#define     COMMAND_READ_BUFFER                                 0xE4
#define     COMMAND_CHECK_POWER_MODE                            0xE5
#define     COMMAND_SLEEP                                       0xE6
#define     COMMAND_FLUSH_CACHE                                 0xE7
#define     COMMAND_WRITE_BUFFER                                0xE8
#define     COMMAND_IDENTIFY_DEVICE                             0xEC
#define     COMMAND_MEDIA_EJECT                                 0xED
#define     COMMAND_SET_FEATURES                                0xEF
#define     COMMAND_SECURITY_SET_PASSWORD                       0xF1
#define     COMMAND_SECURITY_UNLOCK                             0xF2
#define     COMMAND_SECURITY_ERASE_PREPARE                      0xF3
#define     COMMAND_SECURITY_ERASE_UNIT                         0xF4
#define     COMMAND_SECURITY_FREEZE_LOCK                        0xF5
#define     COMMAND_SECURITY_DISABLE_PASSWORD                   0xF6
#define     COMMAND_READ_NATIVE_MAX_ADDRESS                     0xF8
#define     COMMAND_SET_MAX_ADDRESS                             0xF9

/* --- ATA bits ----------------------------------------------------------- */
#define     ATA_REG_STATUS_ERR                                  0x01
#define     ATA_REG_STATUS_COR                                  0x04
#define     ATA_REG_STATUS_DRQ                                  0x08
#define     ATA_REG_STATUS_SKC                                  0x10
#define     ATA_REG_STATUS_DF                                   0x20
#define     ATA_REG_STATUS_RDY                                  0x40
#define     ATA_REG_STATUS_BSY                                  0x80
#define     ATA_REG_STATUS_ABRT                                 0x04
#define     ATA_REG_STATUS_IDNF                                 0x10
#define     DEVICE_0_PASSED_1_PASSED                            0x01
#define     DEVICE_0_PASSED_1_FAILED                            0x81

/* --- sets of feature, in the set_features command ----------------------- */
#define     FEATURE_ENABLE_WRITE_CACHE                          0x02
#define     FEATURE_SET_TRANSFER_MODE                           0x03
#define     FEATURE_ENABLE_ADVANCED_POWER_MANAGEMENT            0x05
#define     FEATURE_DISABLE_MEDIA_STATUS_NOTIFICATION           0x31
#define     FEATURE_DISABLE_READ LOOK_AHEAD_FEATURE             0x55
#define     FEATURE_ENABLE_RELEASE_INTERRUPT                    0x5D
#define     FEATURE_ENABLE_SERVICE_INTERRUPT                    0x5E
#define     FEATURE_DISABLE_REVERTING_TO_POWER_ON_DEFAULTS      0x66
#define     FEATURE_DISABLE_WRITE_CACHE                         0x82
#define     FEATURE_DISABLE_ADVANCED_POWER_MANAGEMENT           0x85
#define     FEATURE_ENABLE_MEDIA_STATUS_NOTIFICATION            0x95
#define     FEATURE_ENABLE_READ_LOOK_AHEAD_FEATURE              0xAA
#define     FEATURE_ENABLE_REVERTING_TO_POWER_ON_DEFAULTS       0xCC
#define     FEATURE_DISABLE_RELEASE_INTERRUPT                   0xDD
#define     FEATURE_DISABLE_SERVICE_INTERRUPT                   0xDE

/* --- set feature values ------------------------------------------------- */
#define     PIO_MODE_2                                          0x0A
#define     PIO_MODE_3                                          0x0B
#define     PIO_MODE_4                                          0x0C
#define     PIO_MODE_4_NO_IORDY                                 0x04

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STATAREG_H */




