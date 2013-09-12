/**********************************************************************

File Name   : stflashd.h

Description : Hardware Flash Device Definitions for
              ST M28F411 & ST M29W800T devices

Copyright (C) 2005 STMicroelectronics

References  :

$ClearCase (VOB: stflash)

API.PDF "Flash Memory API" Reference DVD-API-004 Revision 1.3

***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STFLASHD_H
#define __STFLASHD_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes --------------------------------------------------------- */
/* Exported Types --------------------------------------------------- */

/* Exported Constants ----------------------------------------------- */

#define VPP_HIGH      0xFFFFFFFF
#define VPP_LOW       0
#define VPP_DELAY     2           /* two slow ticks: 64 - 128us */

/* Note that the following are HARDWARE DEPENDENT values,
   correct for the STMicroelectronics  M29W800T Flash devices,
   but not necessarily valid for other manufacturers'.   */
#define M29W800T_DEV_ID_ACCESS   0x00000001
#define M29W800T_MANUF_ID_ACCESS 0x00000000

#define M29W800T_CODED_ADDR1     0x00000555
#define M29W800T_CODED_ADDR2     0x000002AA
#define M29W800T_CODED_DATA1     0x00AA00AA
#define M29W800T_CODED_DATA2     0x00550055

#define M29W800T_AUTO_SELECT     0x00900090
#define M29W800T_BLOCK_ERASE     0x00800080
#define M29W800T_ERASE_CONFIRM   0x00300030
#define M29W800T_ERASE_CHIP      0x00100010
#define M29W800T_PROGRAM         0x00A000A0

#define M28W320_BLKLOCK          0x00600060

#define M29W800T_DQ2_MASK        0x00040004    /* DQ2 on both devices */
#define M29W800T_DQ3_MASK        0x00080008    /* DQ3 on both devices */
#define M29W800T_DQ5_MASK        0x00200020    /* DQ5 on both devices */
#define M29W800T_DQ6_MASK        0x00800080    /* DQ6 on both devices */
#define M29W800T_DQ7_MASK        0x00800080    /* DQ7 on both devices */
#define M29W800T_DQ256_MASK      0x00640064    /* DQ2|DQ5|DQ6 (both devices) */
#define M29W800T_DQ26_MASK       0x00440044    /* DQ2|DQ6 (both devices) */

#define M29W800T_HIGH_DQ5_MASK   0x00200000    /* DQ5 on (d0..d15) device */
#define M29W800T_LOW_DQ5_MASK    0x00000020    /* DQ5 on (d16..d31) device */

#define M29W800T_HIGH_DQ26_MASK  0x00440000    /* DQ2|DQ6 (d0..d15) device */
#define M29W800T_LOW_DQ26_MASK   0x00000044    /* DQ2|DQ6 (d16..d31) device */

#define M29W800T_BLOCK_UNPROTECTION 0x00200020
/* HARDWARE DEPENDANT values for M29DW640D -- DDTS 32164 */
#define M29DW640D_DEV_ID_ACCESS   0x00000001
#define M29DW640D_MANUF_ID_ACCESS 0x00000000

#define M29DW640D_CODED_ADDR1     0x00000AAA
#define M29DW640D_CODED_ADDR2     0x00000555
#define M29DW640D_CODED_DATA1     0x00AA00AA
#define M29DW640D_CODED_DATA2     0x00550055

#define M29DW640D_AUTO_SELECT     0x00900090
#define M29DW640D_BLOCK_ERASE     0x00800080
#define M29DW640D_ERASE_CONFIRM   0x00300030
#define M29DW640D_PROGRAM         0x00A000A0

#define M28W320_BLKLOCK          0x00600060

/* Note that the following are HARDWARE DEPENDENT values,
   correct for the STMicroelectronics  M28F410/1 Flash devices,
   but not necessarily valid for other manufacturers'.   */


#define M28F411_DEV_ID_ACCESS    0x00000004  /* A0 line set (A2 in S/W, A1-0 are CS) */
#define M28F411_MANUF_ID_ACCESS  0x00000000  /* A0 line unset (A2 in S/W, A1-0 are CS) */
#define M58LW032_DEV_ID_ACCESS   0x00000002  /* A0 line set (A2 in S/W, A1-0 are CS) */
#define M58LW032_MANUF_ID_ACCESS 0x00000000  /* A0 line unset (A2 in S/W, A1-0 are CS) */

#if defined(ST_5528) || defined(ST_5100)
#define ERASE_TIMEOUT (90 * ST_GetClocksPerSecond() ) /* 90 second timeout for 5528*/
#else
#define ERASE_TIMEOUT (15 * ST_GetClocksPerSecond() ) /* 15 second timeout otherwise */
#endif

#ifdef ARCHITECTURE_ST40
#define WRITE_TIMEOUT ST_GetClocksPerSecond()
#else
#define WRITE_TIMEOUT (ST_GetClocksPerSecond()/50)     /* ~20 ms of slow ticks */
#endif

#define WRITE_LOOP_TIMEOUT (ST_GetClocksPerSecond()/5000) /* 200us wait in 1 loop */

#define SETTLING_TIME 1000000
#define FLASH_WRITE_BUFFER_SIZE 16

/* DDTS OTP */
#define M58LW064_READ_OTP        0x0080
#define M58LW064_WRITE_OTP       0x0030

/* Read Memory Array Command Codes */
#define M28_READ_MA08     0xFF
#define M28_READ_MA16     0x00FF
#define M28_READ_MA32     0x00FF00FF

#define M29_READ_MA08     0xF0
#define M29_READ_MA16     0x00F0
#define M29_READ_MA32     0x00F000F0

/* Read Status Register Command Codes */
#define READ_SR08     0x70
#define READ_SR16     0x0070
#define READ_SR32     0x00700070

/* Read Electronic Signature Command Codes */
#define READ_ES08     0x90
#define READ_ES16     0x0090
#define READ_ES32     0x00900090

/* Erase (Block) Set-Up Command Code */
#define ERASE_SU08    0x20
#define ERASE_SU16    0x0020
#define ERASE_SU32    0x00200020

/* Erase (Block) Confirm/Resume Command Code */
#define ERASE_CR08    0xD0
#define ERASE_CR16    0x00D0
#define ERASE_CR32    0x00D000D0

/* Program Set-Up Command Codes */ /* NOT valid for M58LW064 (Use Write to Buffer) */
#define PROG_SU08     0x40
#define PROG_SU16     0x0040
#define PROG_SU32     0x00400040

/* Program Set-Up Command Codes for QUADRUPLE WORD write*/
#define PROG_FAST_SU08     0x56
#define PROG_FAST_SU16     0x0056
#define PROG_FAST_SU32     0x00560056

/* Program Confirm/Resume Command Code */
#define PROG_CR08       0xD0
#define PROG_CR16       0x00D0
#define PROG_CR32       0x00D000D0

/* Clear Status Register Command Codes */
#define CLEAR_SR08    0x50
#define CLEAR_SR16    0x0050
#define CLEAR_SR32    0x00500050

/* STATUS REGISTER BITS */

/* Status Register Completion Status bit value */
#define COM_STAT08    0x80
#define COM_STAT16    0x0080
#define COM_STAT32    0x00800080

/* Status Register Erase Status bit value */
#define ERA_STAT08    0x20
#define ERA_STAT16    0x0020
#define ERA_STAT32    0x00200020

/* Status Register Program Status bit value */
#define PRG_STAT08    0x10
#define PRG_STAT16    0x0010
#define PRG_STAT32    0x00100010

/* Status Register Vpp Low Status bit value */
#define VPP_STAT08    0x08
#define VPP_STAT16    0x0008
#define VPP_STAT32    0x00080008

/* Status Register Block Protection bit value */
#define BPE_STAT08    0x10
#define BPE_STAT16    0x0010
#define BPE_STAT32    0x00100010

#define BUE_STAT08    0x20
#define BUE_STAT16    0x0020
#define BUE_STAT32    0x00200020

/* Lock/unlock register - used for M28W320 so far only*/
#define BLK_LOCK_16   0x0001
#define BLK_LOCK_32   0x00010001
#define BLK_UNLOCK_16 0x00D0
#define BLK_UNLOCK_32 0x00D000D0



#define STATUS_MASK            0x00FE00FE
#define PROTECT_STATUS_BIT     0x00010001


/* M58LW064 constants and commands. Not supported at the moment */
/* Write to Buffer Command Code */
#define PROG_BUFF16     0x00E8
#define PROG_BUFF32     0x00E800E8

#define M58LW064_PROGRAM_ERROR_MASK     0x00100010
#define M58LW064_ERASE_ERROR_MASK       0x00200020
#define M58LW064_P_E_END_MASK           0x00800080
#define M58LW064_BLKLOCK_32             0x00600060
#define M58LW064_BLKLOCK_16             0x0060

/* Serial flash defines */
#define CHIP_SELECT_LOW    0x00000000
#define CHIP_SELECT_HIGH   0x00010001
#define READ_STATUS_CMD    0x05
#define WRITE_ENABLE_CMD   0x06
#define PAGE_PROGRAM_CMD   0x02
#define SECTOR_ERASE_CMD   0xD8
#define WRITE_DISABLE_CMD  0x04
#define WRITE_STATUS_CMD   0x01
#define BLOCK_UNLOCK_CMD   0x01
#define CFI_QUERY_COMMAND  0x98
#define BLOCK_LOCK_CMD     0x01
#define DATA_READ_CMD      0x03
#define REACHED_TIMEOUT    100000
#define PAGE_SIZE          256
#define PAGE_MASK          0x000000FF
#define PAGE_WRITE_TIMEOUT        ST_GetClocksPerSecond()/500  /* 2msec max for a page write */
#define MEMORY_READ_TIMEOUT       ST_GetClocksPerSecond()      /* 1sec max for entire memory read */

/* SPI config register */
#define SPI_BASE_ADDRESS   0x40000000 /* 5188 */

#define SPI_CLOCK_DIV_REG           (SPI_BASE_ADDRESS + 0x10)
#define SPI_MODE_SELECT_REG         (SPI_BASE_ADDRESS + 0x18)
#define SPI_CONFIG_DATA_REG         (SPI_BASE_ADDRESS + 0x20)
#define SPI_DYNAMICMODE_CHANGE_REG  (SPI_BASE_ADDRESS + 0x28)

/* Exported Variables ----------------------------------------------- */

/* Exported Macros -------------------------------------------------- */

/* Exported Functions ----------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif   /* #ifndef __STFLASHD_H */

/* End of stflashd.h */



