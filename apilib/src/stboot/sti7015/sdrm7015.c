/*******************************************************************************
File Name   : sdrm7015.c

Description : SDRAM configuration according to a given frequency.

COPYRIGHT (C) STMicroelectronics 2001.

*******************************************************************************/

/* Includes --------------------------------------------------------------- */

#include "stdio.h"
#include "stlite.h"             /* STLite includes */

#include "stdevice.h"           /* Device includes */
#include "stddefs.h"

#include "stsys.h"              /* STAPI includes */
#include "stcommon.h"

#include "sdrm7015.h"           /* Local includes */
#include "cfg_reg.h"            /* Localy used registers */
#include "bereg.h"              /* Localy used registers */

/* Private types/constants ------------------------------------------------ */

/* Wait between two LMI requests */
#ifdef HARDWARE_EMULATOR
#define WAIT_BEFORE_NEXT_REQUEST        task_delay( ST_GetClocksPerSecond() / 10);
#else
#define WAIT_BEFORE_NEXT_REQUEST        task_delay( ST_GetClocksPerSecond() / 1000);
#endif

/* Wait between two LMI requests */
#ifdef HARDWARE_EMULATOR
#define WAIT_BEFORE_NEXT_LMI_WRITE      task_delay( ST_GetClocksPerSecond() / 1000);
#else
#define WAIT_BEFORE_NEXT_LMI_WRITE      task_delay( ST_GetClocksPerSecond() / 1000);
#endif

/* Wait 200us between end of reset and raising CKE */
#ifdef HARDWARE_EMULATOR
#define WAIT_BEFORE_RAISING_CKE         task_delay( ST_GetClocksPerSecond());
#else
#define WAIT_BEFORE_RAISING_CKE         task_delay( ST_GetClocksPerSecond() / 1000);
#endif

/* Wait after enabling LMI requests */
#ifdef HARDWARE_EMULATOR
#define WAIT_AFTER_ENABLE_LMI_REQUESTS  task_delay( ST_GetClocksPerSecond() / 100);
#else
#define WAIT_AFTER_ENABLE_LMI_REQUESTS  task_delay( ST_GetClocksPerSecond() / 100);
#endif

/* Wait after enabling refresh */
#ifdef HARDWARE_EMULATOR
#define WAIT_AFTER_ENABLE_REFRESH       task_delay( ST_GetClocksPerSecond() / 10);
#else
#define WAIT_AFTER_ENABLE_REFRESH       task_delay( ST_GetClocksPerSecond() / 1000);
#endif

/* LMI_CFG register value */

#if defined(ST_7015_FORCE_32MBIT)
#define LMI_CFG_REG 0x256               /* For cut 1.1 with bonding issue */
#else
#ifdef HARDWARE_EMULATOR
#define LMI_CFG_REG 0x21A
#else
#define LMI_CFG_REG 0x35A               /* For cuts without 64bit DRAM bug */
#endif
#endif


/* For SDRAM 7015-DDR test */
#ifdef mb295  /* mb295 board usually has 8 x 8Mb pages */
  #define IS_IN_SDRAM(addr)   ((U32)(addr) >= SDRAM_BASE_ADDRESS && (U32)(addr) < (SDRAM_BASE_ADDRESS + SDRAM_SIZE))
  #define PAGE_OF(addr)     (((U32)(addr) >> 23 ) & 0x7)
  #define OFFSET_OF(addr)   ((U32)(addr) & 0x7FFFFF)
  #define BUF_SIZE      0x4000    /* 16 Kb */
/*    #define SET_SDRAM_PAGE(p) STSYS_WriteRegMem16LE((FPGA_BASE_ADDRESS + FPGA_7020_PAGE), (p)); */
  #define SET_SDRAM_PAGE(p) /* Do not allow to switch page in mb295 for RefTree */
#else   /* mb290, etc */
  #define IS_IN_SDRAM(addr)   FALSE
  #define PAGE_OF(addr)       0
  #define OFFSET_OF(addr)     ((U32)(addr))
  #define BUF_SIZE            0x4000    /* 16 Kb */
  #define SET_SDRAM_PAGE(p)
#endif


/* Private macros ------------------------------------------------------ */

#ifdef STTBX_PRINT
#define STBOOT_Printf(x) printf x
#else
#define STBOOT_Printf(x)
#endif

/* On mb290, CPU access is too quick for STi7020 cut 1.0, fall into Cache
   bug each time. It still happens sometimes on cut 2.0 */
#if defined(mb290) && !defined(STBOOT_DISABLE_RAM_TESTS)
#define STBOOT_DISABLE_RAM_TESTS
#endif

/* Private variables ------------------------------------------------------ */

/* Private function prototypes -------------------------------------------- */

static BOOL stboot_ConfigureSDRAM(U32 Frequency);
#if !defined (STBOOT_DISABLE_RAM_TESTS)
static BOOL stboot_TestSDRAM(U32 start, U32 size);
#endif

/* Global variables ------------------------------------------------------- */

/* Function definitions --------------------------------------------------- */

/*******************************************************************************
Name        : stboot_InitSDRAM7015
Description : Init and Configure SDRAM
Parameters  : Frequency: SDRAM frequency
Assumptions :
Limitations :
Returns     : TRUE if Init has failed. FALSE if init was successful.
*****************************************************************************/

BOOL stboot_InitSDRAM7015(U32 Frequency)
{
    BOOL RetError;

    RetError = stboot_ConfigureSDRAM(Frequency);

#if !defined (STBOOT_DISABLE_RAM_TESTS)
    if (RetError == FALSE)
    {
        /* 0x60800000 is the window address of the 7015-DDR memory from CPU */
#ifdef HARDWARE_EMULATOR
        /* We test the first 1 kbytes.                                      */
        RetError = stboot_TestSDRAM(SDRAM_WINDOW_BASE_ADDRESS, 1*1024);
#else
        /* We test the first 32 kbytes.                                     */
        /* 256 bytes at start of memory seems to be corrupted !             */
        RetError = stboot_TestSDRAM(SDRAM_WINDOW_BASE_ADDRESS+0x100, 32*1024);
#endif
    }
#endif

    return (RetError);
} /* stboot_InitSDRAM7015 () */

/*****************************************************************************
Name        : stboot_ConfigureSDRAM
Description : Sequence init, refresh programmation, Phase locking
Parameters  : Frequency
Assumptions :
Limitations :
Returns     : FALSE if SDRAM test is OK, TRUE otherwise
*****************************************************************************/
#if defined(ST_7020)
static BOOL stboot_ConfigureSDRAM(U32 Frequency)
{
    BOOL RetError = FALSE;
    U32 lmi_cfg_reg;
#ifndef HARDWARE_EMULATOR
    BOOL DLL_locked = FALSE;

#ifdef STTBX_PRINT
    {
        U32 usrid, idcod;

        /* NOTE: These are backend CFG registers (i.e. 7015/20 NOT 5514) */
        usrid = STSYS_ReadRegDev32LE((void *) (CFG_BASE_ADDRESS + CFG_USRID));
        idcod = STSYS_ReadRegDev32LE((void *) (CFG_BASE_ADDRESS + CFG_IDCOD));

        printf("\nInitSDRAM: Chip Id Code: 0x%x", idcod);

        if (idcod == 0x1d40e041)
        {
            printf(" = STi7020");
        }

        printf("\nInitSDRAM: Chip User Id: Cut %d.%d", (usrid>>4)&0xf, usrid&0xf);
    }
#endif
#endif

#ifdef HARDWARE_EMULATOR
    STBOOT_Printf(("\nInitSDRAM: 32 Mbytes memory on Hardware Emulator"));
    lmi_cfg_reg = 0x21A;
#elif defined(mb290) || defined(mb295)
    STBOOT_Printf(("\nInitSDRAM: 4 x 16 bit, 64 Mbytes DDR-SDRAM"));
    lmi_cfg_reg = 0x35A;        /* 64 Mbytes, 4 x 16 bit DDR-SDRAM chips */
#else /* 7020 STEM board db573 */
    STBOOT_Printf(("\nInitSDRAM: 2 x 32 bit, 32 Mbytes DDR-SDRAM"));
    lmi_cfg_reg = 0x2a1a;       /* 32 Mbytes, 2 x 32 bit DDR-SDRAM chips */
#endif


    STBOOT_Printf(("\nInitSDRAM: Configuring EMPI... ")); /* (host interface) */

    /* NOTE: These are backend CFG registers (i.e. 7015/20 NOT 5514) */
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_WPRI),    0xE53);
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_RPRI),    0xD53);
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_DPRI),    0xFC);
#if 1
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_PTIM),    0x100);
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_WTIM),    0x80);
#else   /* FC: STi7020 cut 1.0 Bug GNBvd12831 resolved in cut 2.0 */
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_PTIM),    0x50);
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_WTIM),    0x0);
#endif
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_RCFG),    0x1);
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_DTIM),    0x0);
    STBOOT_Printf(("OK\n"));

    STBOOT_Printf(("InitSDRAM: Configuring VIDx_SRS... "));

    /* Soft reset the video pipelines */
    STSYS_WriteRegDev32LE((void *) (VID1_BASE_ADDRESS + VID_SRS),    0x1);
    STSYS_WriteRegDev32LE((void *) (VID2_BASE_ADDRESS + VID_SRS),    0x1);
    STSYS_WriteRegDev32LE((void *) (VID3_BASE_ADDRESS + VID_SRS),    0x1);
    STSYS_WriteRegDev32LE((void *) (VID4_BASE_ADDRESS + VID_SRS),    0x1);
    STSYS_WriteRegDev32LE((void *) (VID5_BASE_ADDRESS + VID_SRS),    0x1);
    STBOOT_Printf(("OK\n"));

    STBOOT_Printf(("InitSDRAM: Configuring LMI DLL..."));
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_CTRL), 0x43);

    {
        U32 Attempt = 0;
        while (DLL_locked == FALSE)
        {
	    WAIT_BEFORE_NEXT_LMI_WRITE /*task_delay(4)*/;
            if ( (STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS
                                                 + LMI_DLL_STATUS)) & 0x1) == 1)
            {
                DLL_locked = TRUE;
                STBOOT_Printf((" Locked\n"));
            }
            if(Attempt++ > 100)
            {
                break;
            }
        } /* while (DLL_locked == FALSE && try < 100) */
    }
    if ( (STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS
                                         + LMI_DLL_STATUS)) & 0x1) != 1)
    {
        STBOOT_Printf((" - FAILED TO LOCK\n"));
        return TRUE;
    }

    STBOOT_Printf(("InitSDRAM: Configuring LMI... "));
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_CFG), lmi_cfg_reg);
    WAIT_BEFORE_RAISING_CKE
    /*
     * LMI_IO_CFG: Drive of Clk:  16mA
     *             Drive of Cmd:  16mA
     *             Drive of Data: 16mA
     * changed from 8, 8, 2 to improve system stability
     * The TRC, TRP, TRRD and TRCD & MRS timings have been optimised for 7020 cut 2.0
     */
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_IO_CFG), 0x155555);
    WAIT_BEFORE_NEXT_LMI_WRITE
#if defined(mb290) || defined(mb295)
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRC), 8);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRCR), 0xa);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRP), 3);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TWRD), 4);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRAS), 6);
    WAIT_BEFORE_NEXT_LMI_WRITE
#else /* assume db573 7020 STEM board */
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRC), 5);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRCR), 8);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRP), 2);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TWRD), 7/*4*/);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRAS), 5);
    WAIT_BEFORE_NEXT_LMI_WRITE
#endif
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRRD), 2);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRCD), 3);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TDPLR), 1);
    WAIT_BEFORE_NEXT_LMI_WRITE
#if defined(mb290) || defined(mb295)
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TDPLW), 3);
    /* LMI_TDPLW = 3 provides optimal performance on boards with K4H281638D DDR
      chips. Older boards with K4H281638B should be upgraded to avoid a bug
      (DDTS 14151), though LMI_TDPLW = 6 can be used as a temporary workaround
      for write corruptions, blitter hangs, etc */
#else /* assume db573 7020 STEM board */
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TDPLW), 6);
#endif
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TBSTW), 3);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_LAF_DEPTH), 7);
    WAIT_BEFORE_NEXT_LMI_WRITE

    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_ENA), 1);
    WAIT_BEFORE_NEXT_LMI_WRITE

    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_CKE), 0);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_CKE), 1);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STBOOT_Printf(("OK\n"));

    STBOOT_Printf(("InitSDRAM: Configuring SDRAM... "));
#if defined(mb290) || defined(mb295)
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_EMRS), 0);
#else /* assume db573 7020 STEM board */
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_EMRS), 0x42 /*1*/);
#endif
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_EMRS), 0);
    WAIT_BEFORE_NEXT_REQUEST

    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_MRS), 0x100);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_MRS), 0);
    WAIT_BEFORE_NEXT_REQUEST

    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_PALL), 0);
    WAIT_BEFORE_NEXT_REQUEST

    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_CBR), 0);
    WAIT_BEFORE_NEXT_REQUEST
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_CBR), 0);
    WAIT_BEFORE_NEXT_REQUEST

#if defined(mb290) || defined(mb295)
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_MRS), 0x063 /*0x023*/);
#else /* assume db573 7020 STEM board */
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_MRS), 0x033 /*0x023*/);
#endif
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_MRS), 0);
    WAIT_BEFORE_NEXT_REQUEST
    STBOOT_Printf(("OK\n"));

    STBOOT_Printf(("InitSDRAM: Enable LMI requests... "));
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_CTRL), 1);
    WAIT_AFTER_ENABLE_LMI_REQUESTS
    STBOOT_Printf(("OK\n"));

    STBOOT_Printf(("InitSDRAM: Enable Refresh... "));
#if defined(mb290) || defined(mb295)
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REF_CFG), 0x10300);
#else /* assume db573 7020 STEM board */
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REF_CFG), 0x1034b);
#endif
    WAIT_AFTER_ENABLE_REFRESH
    STBOOT_Printf(("OK\n"));

  return(RetError);
} /* stboot_ConfigureSDRAM () */

#elif defined(ST_7015)

static BOOL stboot_ConfigureSDRAM(U32 Frequency)
{
    BOOL RetError = FALSE;
#ifndef HARDWARE_EMULATOR
    int init_value=0;
    int try = 0;
    BOOL DLL_locked = FALSE;

#ifdef STTBX_PRINT
    {
        int usrid;

        usrid = STSYS_ReadRegDev32LE((void *) (CFG_BASE_ADDRESS + CFG_USRID));
        printf("\nInitSDRAM: Chip User Id: Cut %d.%d", (usrid>>4)&0xf, usrid&0xf);
    }
#endif

#endif /* HARDWARE_EMULATOR */

/* LMI_CFG register value */
#ifdef HARDWARE_EMULATOR
    STBOOT_Printf(("\nInitSDRAM: 32 Mbytes memory on Hardware Emulator"));
#else
    STBOOT_Printf(("\nInitSDRAM: 4 x 16 bit, 64 Mbytes DDR-SDRAM"));
#endif


    STBOOT_Printf(("\nInitSDRAM: Configuring EMPI... "));
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_WPRI),    0xE53);
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_RPRI),    0xD53);
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_DPRI),    0xFC);
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_PTIM),    0x100);
    /* Set WTIM to 0x0 to avoid a cache coherence issue for STi7015 but     */
    /* introduce another cache problem !                                    */
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_WTIM),    0x80);
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_RCFG),    0x1);
    STBOOT_Printf(("OK\n"));

    STBOOT_Printf(("InitSDRAM: Configuring VIDx_SRS... "));
    STSYS_WriteRegDev32LE((void *) (VID1_BASE_ADDRESS + VID_SRS),    0x1);
    STSYS_WriteRegDev32LE((void *) (VID2_BASE_ADDRESS + VID_SRS),    0x1);
    STSYS_WriteRegDev32LE((void *) (VID3_BASE_ADDRESS + VID_SRS),    0x1);
    STSYS_WriteRegDev32LE((void *) (VID4_BASE_ADDRESS + VID_SRS),    0x1);
    STSYS_WriteRegDev32LE((void *) (VID5_BASE_ADDRESS + VID_SRS),    0x1);
    STBOOT_Printf(("OK\n"));

    STBOOT_Printf(("InitSDRAM: Configuring LMI... "));
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_ENA), 1);
    WAIT_BEFORE_NEXT_LMI_WRITE

    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_RST), 1);
    WAIT_BEFORE_NEXT_LMI_WRITE

    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_IO_CFG), 0x3f0000);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRC), 7);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRCR), 0xa);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRP), 2);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TWRD), 4);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRAS), 6);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRRD), 3);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRCD), 2);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TDPLR), 1);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TDPLW), 3);
    /* LMI_TDPLW = 3 provides optimal performance on boards with K4H281638D DDR
      chips. Older boards with K4H281638B should be upgraded to avoid a bug
      (DDTS 14151), though LMI_TDPLW = 6 can be used as a temporary workaround
      for write corruptions, blitter hangs, etc */
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TBSTW), 3);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_LAF_DEPTH), 7);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_EMRS), 0x0);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_MRS), 0x123);
    WAIT_BEFORE_NEXT_LMI_WRITE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_CFG), LMI_CFG_REG);
    WAIT_BEFORE_RAISING_CKE
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_CKE), 1);
    WAIT_BEFORE_NEXT_LMI_WRITE

    /* Start DLL */
#ifndef HARDWARE_EMULATOR
    for (init_value=0x65; init_value<0x70; init_value++)
    {
        for (try=0; try<100; try++)
        {
            STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_RST), 0);
            STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_CTRL), init_value);
            STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_RST), 1);
            STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_CTRL), 0x0);
            task_delay(4);
            if ( (STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_STATUS)) & 0x1) == 1)
            {
                DLL_locked = TRUE;
                break;
            }
        }
        if (DLL_locked == TRUE)
        {
            break;
        }
    }

    if (DLL_locked == FALSE)
    {
        STBOOT_Printf(("InitSDRAM: LMI... Failed\n"));
        STBOOT_Printf(("\tLMI: Read 0x%x in LMI_DLL_STATUS !\n",
                STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_STATUS))));
        RetError = TRUE;
    }
#endif  /* not HARDWARE_EMULATOR */

    if (STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRCR)) != 10)
    {
        STBOOT_Printf(("InitSDRAM: Configuring LMI... Failed\n");
        STBOOT_Printf(("\tLMI: Read 0x%x instead of 0xa in LMI_TRCR !\n",
                STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_TRCR))));
        RetError = TRUE;
    }
    else if (STSYS_ReadRegDev32LE((void *)(LMI_BASE_ADDRESS + LMI_CFG)) != LMI_CFG_REG)
    {
        STBOOT_Printf(("InitSDRAM: Configuring LMI... Failed\n"));
        STBOOT_Printf(("\tLMI: Read 0x%x instead of 0x%x in LMI_CFG !\n",
                STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_CFG)),LMI_CFG_REG));
        RetError = TRUE;
    }
#ifndef HARDWARE_EMULATOR
    else if ((STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_STATUS)) & 0x1) == 0x0)
    {
        STBOOT_Printf(("InitSDRAM: Configuring LMI... Failed\n"));
        STBOOT_Printf(("\tLMI: DLL not locked ! DLL_STATUS: 0x%x\n",
                STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_STATUS))));
        RetError = TRUE;
    }
    else
    {
        STBOOT_Printf(("InitSDRAM: Configuring LMI... Ok\n"));
        STBOOT_Printf(("\tLMI: DLL_STATUS: 0x%x, with init value = 0x%x, Tried %d times\n",
                STSYS_ReadRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_DLL_STATUS)), init_value, try));
    }
#else /* not HARDWARE_EMULATOR */
        else
        {
            STBOOT_Printf(("OK\n"));
        }
#endif /* not HARDWARE_EMULATOR */

    STBOOT_Printf(("InitSDRAM: Configuring SDRAM... "));
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_PALL), 0);
    WAIT_BEFORE_NEXT_REQUEST
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_EMRS), 0);
    WAIT_BEFORE_NEXT_REQUEST
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_MRS), 0);
    WAIT_BEFORE_NEXT_REQUEST
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_PALL), 0);
    WAIT_BEFORE_NEXT_REQUEST
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_CBR), 0);
    WAIT_BEFORE_NEXT_REQUEST
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_CBR), 0);
    WAIT_BEFORE_NEXT_REQUEST
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_MRS), 0x23);
    WAIT_BEFORE_NEXT_REQUEST
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REQ_MRS), 0);
    WAIT_BEFORE_NEXT_REQUEST
    STBOOT_Printf(("OK\n"));

    STBOOT_Printf(("InitSDRAM: Enable LMI requests... "));
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_CTRL), 1);
    WAIT_AFTER_ENABLE_LMI_REQUESTS
    STBOOT_Printf(("OK\n"));

    STBOOT_Printf(("InitSDRAM: Enable Refresh... "));

#ifndef HARDWARE_EMULATOR
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REF_CFG), 0x10618);
#else
    STSYS_WriteRegDev32LE((void *) (LMI_BASE_ADDRESS + LMI_REF_CFG), 0x10300);
#endif
    WAIT_AFTER_ENABLE_REFRESH
    STBOOT_Printf(("OK\n"));

    return(RetError);
} /* stboot_ConfigureSDRAM () */
#else
#error ERROR:invalid DVD_BACKEND defined
#endif

/*****************************************************************************
Name        : stboot_TestSDRAM
Description : Test SDRAM memory (DDR from 7015)
Parameters  : U32 start, U32 size
Assumptions :
Limitations :
Returns     : FALSE if SDRAM test is OK, TRUE otherwise
*****************************************************************************/

#if !defined (STBOOT_DISABLE_RAM_TESTS)
static BOOL stboot_TestSDRAM(U32 start, U32 size)
{
    U32                 stop;                                 /* memory stop address */
    volatile U32        *ptr;                                   /* current position in memory */
    volatile U32        *win_ptr;                               /* current position in memory inside the memory window */
    volatile U32        *ptr_start;                             /* start position in memory */
    volatile U32        *ptr_stop;                              /* stop position in memory */
    U8                  page;                                   /* Should be always 0 due to access memory through a window*/
    U32                 offset;
    S32                 count, n;
    BOOL                error = FALSE, err = FALSE;
    U32                 w, k;                                   /* counters */
    U32                 test, test1, test2;
    U32                 pattern[] = {0x00000000, 0xFFFF0000, 0xFF00FF00,
                                     0xF0F0F0F0, 0xCCCCCCCC, 0xAAAAAAAA};

    stop = start + size*4;
    ptr_start = (U32*)start;
    ptr_stop = ((U32*)stop) - 1;


#define MULTI_PAGE_ACCESS FALSE /* Code from valid, we do not allow window swap */
#if (MULTI_PAGE_ACCESS == TRUE)
    /* protect the sdram access by pages */
    if(IS_IN_SDRAM(start) && SdramMutex_p != NULL)
    {
        semaphore_wait(SdramMutex_p);
    }
#endif

    /* Addressing test 1st Loop */
    /* Filling RAM with the address */
#ifdef STTBX_PRINT
    printf("Memory test... -\b"); fflush(stdout);
#endif
    count = (int)size;
    ptr = ptr_start;
    while (count > 0)
    {
        if(IS_IN_SDRAM(ptr))
        {
            page = PAGE_OF(ptr);
            offset = OFFSET_OF(ptr);
            if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))                /* only one page */
            {
                n = count;
            }
            else                                                        /* several pages */
            {
                if (page == PAGE_OF(ptr_start))                         /* current page is first one */
                    n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                else if (page == PAGE_OF(ptr_stop))                     /* current page is last one */
                    n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                else                                                    /* current page is between first and last one */
                    n = SDRAM_WINDOW_SIZE >> 2;
            }
            SET_SDRAM_PAGE(page);
            win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
        }
        else    /* not in SDRAM */
        {
            n = count;
            win_ptr = ptr;
        }
        for (k =0; k < n; k++)
        {
            *win_ptr++ = (U32)ptr;
            ptr++;
        }
        count -= n;
    }

    /* Addressing test 2nd Loop */
    /* Read back RAM */
#ifdef STTBX_PRINT
    printf("\\\b"); fflush(stdout);
#endif
    count = (int)size;
    ptr = ptr_start;
    while (count > 0)
    {
        if(IS_IN_SDRAM(ptr))
        {
            page = PAGE_OF(ptr);
            offset = OFFSET_OF(ptr);
            if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))                /* only one page */
            {
                n = count;
            }
            else                                                        /* several pages */
            {
                if (page == PAGE_OF(ptr_start))                         /* current page is first one */
                    n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                else if (page == PAGE_OF(ptr_stop))                     /* current page is last one */
                    n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                else                                                    /* current page is between first and last one */
                    n = SDRAM_WINDOW_SIZE >> 2;
            }
            SET_SDRAM_PAGE(page);
            win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
        }
        else    /* not in SDRAM */
        {
            n = count;
            win_ptr = ptr;
        }
        for (k =0; k < n; k++)
        {
            test = *win_ptr;
            if (test != (U32)ptr)
            {
#ifdef STTBX_PRINT
                printf("\n**** Error: Read: %x instead of: %x, at address: %x",
                       test, (U32)win_ptr, (U32)win_ptr);
                if ((U32)win_ptr & 0x4)
                {
                    printf(" MSB word");
                }
                else
                {
                    printf(" LSB word");
                }
#endif
                error = err = TRUE;
            }
            win_ptr++;
            ptr++;
        }
        count -= n;
    }
    if (err)
    {
        err = FALSE;
#ifdef STTBX_PRINT
        printf("\n");
#endif
    }

    /* 00/FF test 1st Loop */
    /* Filling RAM with 0x00 at even positions, 0xFF at odd positions */
#ifdef STTBX_PRINT
    printf("|\b"); fflush(stdout);
#endif
    count = (int)size;
    ptr = ptr_start;
    while (count > 0)
    {
        if(IS_IN_SDRAM(ptr))
        {
            page = PAGE_OF(ptr);
            offset = OFFSET_OF(ptr);
            if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))                /* only one page */
            {
                n = count;
            }
            else                                                        /* several pages */
            {
                if (page == PAGE_OF(ptr_start))                         /* current page is first one */
                    n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                else if (page == PAGE_OF(ptr_stop))                     /* current page is last one */
                    n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                else                                                    /* current page is between first and last one */
                    n = SDRAM_WINDOW_SIZE >> 2;
            }
            SET_SDRAM_PAGE(page);
            win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
            ptr += n;
        }
        else    /* not in SDRAM */
        {
            n = count;
            win_ptr = ptr;
        }
        for (k =0; k < n; k+=2)
        {
            *win_ptr++ = 0x00000000;    /* keep both operations together ! */
            *win_ptr++ = 0xffffffff;    /* keep both operations together ! */
        }
        count -= n;
    }

    /* 00/FF test 2nd Loop */
    /* Read back RAM */
#ifdef STTBX_PRINT
    printf("/\b"); fflush(stdout);
#endif
    count = (int)size;
    ptr = ptr_start;
    while (count > 0)
    {
        if(IS_IN_SDRAM(ptr))
        {
            page = PAGE_OF(ptr);
            offset = OFFSET_OF(ptr);
            if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))                /* only one page */
            {
                n = count;
            }
            else                                                        /* several pages */
            {
                if (page == PAGE_OF(ptr_start))                         /* current page is first one */
                    n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                else if (page == PAGE_OF(ptr_stop))                     /* current page is last one */
                    n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                else                                                    /* current page is between first and last one */
                    n = SDRAM_WINDOW_SIZE >> 2;
            }
            SET_SDRAM_PAGE(page);
            win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
            ptr += n;
        }
        else    /* not in SDRAM */
        {
            n = count;
            win_ptr = ptr;
        }
        for (k =0; k < n; k+=2)
        {
            test1 = *win_ptr;           /* keep both operations together ! */
            test2 = *(win_ptr+1);       /* keep both operations together ! */
            if ((test1 != 0x00000000) || (test2 != 0xffffffff))
            {
#ifdef STTBX_PRINT
                printf("\n**** Error: Read: %x, %x instead of: %x, %x, at address: %x",
                       test1, test2, 0x00000000, 0xffffffff, (U32)win_ptr);
                if ((U32)win_ptr & 0x4)
                {
                    printf(" MSB word");
                }
                else
                {
                    printf(" LSB word");
                }
#endif
                error = err = TRUE;
            }
            win_ptr += 2;
        }
        count -= n;
    }
    if (err)
    {
        err = FALSE;
#ifdef STTBX_PRINT
        printf("\n");
#endif
    }

    /* FF/00 test 1st Loop */
    /* Filling RAM with 0xFF at even positions, 0x00 at odd positions */
#ifdef STTBX_PRINT
    printf("-\b"); fflush(stdout);
#endif
    count = (int)size;
    ptr = ptr_start;
    while (count > 0)
    {
        if(IS_IN_SDRAM(ptr))
        {
            page = PAGE_OF(ptr);
            offset = OFFSET_OF(ptr);
            if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))                /* only one page */
            {
                n = count;
            }
            else                                                        /* several pages */
            {
                if (page == PAGE_OF(ptr_start))                         /* current page is first one */
                    n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                else if (page == PAGE_OF(ptr_stop))                     /* current page is last one */
                    n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                else                                                    /* current page is between first and last one */
                    n = SDRAM_WINDOW_SIZE >> 2;
            }
            SET_SDRAM_PAGE(page);
            win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
            ptr += n;
        }
        else    /* not in SDRAM */
        {
            n = count;
            win_ptr = ptr;
        }
        for (k =0; k < n; k+=2)
        {
            *win_ptr++ = 0xffffffff;    /* keep both operations together ! */
            *win_ptr++ = 0x00000000;    /* keep both operations together ! */
        }
        count -= n;
    }

    /* FF/00 test 2nd Loop */
    /* Read back RAM */
#ifdef STTBX_PRINT
    printf("\\\b"); fflush(stdout);
#endif
    count = (int)size;
    ptr = ptr_start;
    while (count > 0)
    {
        if(IS_IN_SDRAM(ptr))
        {
            page = PAGE_OF(ptr);
            offset = OFFSET_OF(ptr);
            if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))                /* only one page */
            {
                n = count;
            }
            else                                                        /* several pages */
            {
                if (page == PAGE_OF(ptr_start))                         /* current page is first one */
                    n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                else if (page == PAGE_OF(ptr_stop))                     /* current page is last one */
                    n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                else                                                    /* current page is between first and last one */
                    n = SDRAM_WINDOW_SIZE >> 2;
            }
            SET_SDRAM_PAGE(page);
            win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
            ptr += n;
        }
        else    /* not in SDRAM */
        {
            n = count;
            win_ptr = ptr;
        }
        for (k =0; k < n; k+=2)
        {
            test1 = *win_ptr;           /* keep both operations together ! */
            test2 = *(win_ptr+1);       /* keep both operations together ! */
            if ((test1 != 0xffffffff) || (test2 != 0x00000000))
            {
#ifdef STTBX_PRINT
                printf("\n**** Error: Read: %x, %x instead of: %x, %x, at address: %x",
                       test1, test2, 0xffffffff, 0x00000000, (U32)win_ptr);
                if ((U32)win_ptr & 0x4)
                {
                    printf(" MSB word");
                }
                else
                {
                    printf(" LSB word");
                }
#endif
                error = err = TRUE;
            }
            win_ptr += 2;
        }
        count -= n;
    }
    if (err)
    {
        err = FALSE;
#ifdef STTBX_PRINT
        printf("\n");
#endif
    }

    /* Loop over each pattern */
    for(w = 0; w < (sizeof(pattern)/sizeof(U32)); w++)
    {
        /* 1st Marinescu Loop */
        /* Fill in RAM */
#ifdef STTBX_PRINT
        printf("|\b"); fflush(stdout);
#endif
        count = (int)size;
        ptr = ptr_start;
        while (count > 0)
        {
            if(IS_IN_SDRAM(ptr))
            {
                page = PAGE_OF(ptr);
                offset = OFFSET_OF(ptr);
                if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))    /* only one page */
                {
                    n = count;
                }
                else                                            /* several pages */
                {
                    if (page == PAGE_OF(ptr_start))       /* current page is first one */
                        n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                    else if (page == PAGE_OF(ptr_stop))   /* current page is last one */
                        n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                    else                                          /* current page is between first and last one */
                        n = SDRAM_WINDOW_SIZE >> 2;
                }
                SET_SDRAM_PAGE(page);
                win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
                ptr += n;
            }
            else  /* not in SDRAM */
            {
                n = count;
                win_ptr = ptr;
            }
            for (k =0; k < n; k++)
            {
                *win_ptr++ = pattern[w];
            }
            count -= n;
        }

        /* 2nd Marinescu Loop */
        /* Read back pattern, Write ~pattern, Write pattern, Write ~pattern */
#ifdef STTBX_PRINT
        printf("/\b"); fflush(stdout);
#endif
        count = (int)size;
        ptr = ptr_start;
        while (count > 0)
        {
            if(IS_IN_SDRAM(ptr))
            {
                page = PAGE_OF(ptr);
                offset = OFFSET_OF(ptr);
                if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))    /* only one page */
                {
                    n = count;
                }
                else                                            /* several pages */
                {
                    if (page == PAGE_OF(ptr_start))       /* current page is first one */
                        n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                    else if (page == PAGE_OF(ptr_stop))   /* current page is last one */
                        n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                    else                                          /* current page is between first and last one */
                        n = SDRAM_WINDOW_SIZE >> 2;
                }
                SET_SDRAM_PAGE(page);
                win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
                ptr += n;
            }
            else  /* not in SDRAM */
            {
                n = count;
                win_ptr = ptr;
            }
            for (k =0; k < n; k++)
            {
                test = *win_ptr;
                if (test != pattern[w])
                {
#ifdef STTBX_PRINT
                    printf("\n**** Error: Read: %x instead of: %x, at address: %x",
                           test, pattern[w], (U32)win_ptr);
                    if ((U32)win_ptr & 0x4)
                    {
                        printf(" MSB word");
                    }
                    else
                    {
                        printf(" LSB word");
                    }
#endif
                    error = err = TRUE;
                }
                *win_ptr = ~pattern[w];
                *win_ptr = pattern[w];
                *win_ptr++ = ~pattern[w];
            }
            count -= n;
        }
        if (err)
        {
            err = FALSE;
#ifdef STTBX_PRINT
            printf("\n");
#endif
        }

        /* 3rd Marinescu Loop */
        /* Read back ~pattern, Write pattern, (Wait) ,Read back pattern, Write ~pattern */
#ifdef STTBX_PRINT
        printf("-\b"); fflush(stdout);
#endif
        count = (int)size;
        ptr = ptr_start;
        while (count > 0)
        {
            if(IS_IN_SDRAM(ptr))
            {
                page = PAGE_OF(ptr);
                offset = OFFSET_OF(ptr);
                if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))    /* only one page */
                {
                    n = count;
                }
                else                                            /* several pages */
                {
                    if (page == PAGE_OF(ptr_start))             /* current page is first one */
                        n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                    else if (page == PAGE_OF(ptr_stop))         /* current page is last one */
                        n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                    else                                        /* current page is between first and last one */
                        n = SDRAM_WINDOW_SIZE >> 2;
                }
                SET_SDRAM_PAGE(page);
                win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
                ptr += n;
            }
            else  /* not in SDRAM */
            {
                n = count;
                win_ptr = ptr;
            }
            for (k =0; k < n; k++)
            {
                test = *win_ptr;
                if (test != (~pattern[w]))
                {
#ifdef STTBX_PRINT
                    printf("\n**** Error: Read: %x instead of: %x, at address: %x",
                           test, ~pattern[w], (U32)win_ptr);
                    if ((U32)win_ptr & 0x4)
                    {
                        printf(" MSB word");
                    }
                    else
                    {
                        printf(" LSB word");
                    }
#endif
                    error = err = TRUE;
                }
                *win_ptr = pattern[w];
                test = *win_ptr;
                if (test != pattern[w])
                {
#ifdef STTBX_PRINT
                    printf("\n**** Error: Read: %x instead of: %x, at address: %x",
                           test, pattern[w], (U32)win_ptr);
                    if ((U32)win_ptr & 0x4)
                    {
                        printf(" MSB word");
                    }
                    else
                    {
                        printf(" LSB word");
                    }
#endif
                    error = err = TRUE;
                }
                *win_ptr++ = ~pattern[w];
            }
            count -= n;
        }
        if (err)
        {
            err = FALSE;
#ifdef STTBX_PRINT
            printf("\n");
#endif
        }

        /* 4th Marinescu Loop */
        /* Read back ~pattern, Write pattern, Write ~pattern, Write pattern */
        /* Decrementing addresses */
#ifdef STTBX_PRINT
        printf("\\\b"); fflush(stdout);
#endif
        count = (int)size;
        ptr = ptr_stop;
        while (count > 0)
        {
            if(IS_IN_SDRAM(ptr))
            {
                page = PAGE_OF(ptr);
                offset = OFFSET_OF(ptr);
                if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))    /* only one page */
                {
                    n = count;
                }
                else                                            /* several pages */
                {
                    if (page == PAGE_OF(ptr_start))       /* current page is first one */
                        n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                    else if (page == PAGE_OF(ptr_stop))   /* current page is last one */
                        n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                    else                                          /* current page is between first and last one */
                        n = SDRAM_WINDOW_SIZE >> 2;
                }
                SET_SDRAM_PAGE(page);
                win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
                ptr -= n;
            }
            else  /* not in SDRAM */
            {
                n = count;
                win_ptr = ptr;
            }
            for (k =0; k < n; k++)
            {
                test = *win_ptr;
                if (test != (~pattern[w]))
                {
#ifdef STTBX_PRINT
                    printf("\n**** Error: Read: %x instead of: %x, at address: %x",
                           test, ~pattern[w], (U32)win_ptr);
                    if ((U32)win_ptr & 0x4)
                    {
                        printf(" MSB word");
                    }
                    else
                    {
                        printf(" LSB word");
                    }
#endif
                    error = err = TRUE;
                }
                *win_ptr = pattern[w];
                *win_ptr = ~pattern[w];
                *win_ptr-- = pattern[w];
            }
            count -= n;
        }
        if (err)
        {
            err = FALSE;
#ifdef STTBX_PRINT
            printf("\n");
#endif
        }

        /* 5th Marinescu Loop */
        /* Read back pattern, Write ~pattern, Read back ~pattern, Write pattern */
#ifdef STTBX_PRINT
        printf("|\b"); fflush(stdout);
#endif
        count = (int)size;
        ptr = ptr_stop;
        while (count > 0)
        {
            if(IS_IN_SDRAM(ptr))
            {
                page = PAGE_OF(ptr);
                offset = OFFSET_OF(ptr);
                if (PAGE_OF(ptr_stop) == PAGE_OF(ptr_start))    /* only one page */
                {
                    n = count;
                }
                else                                            /* several pages */
                {
                    if (page == PAGE_OF(ptr_start))       /* current page is first one */
                        n = (SDRAM_WINDOW_SIZE - OFFSET_OF(ptr_start)) >> 2;
                    else if (page == PAGE_OF(ptr_stop))   /* current page is last one */
                        n = (OFFSET_OF(ptr_stop) >> 2) + 1;
                    else                                          /* current page is between first and last one */
                        n = SDRAM_WINDOW_SIZE >> 2;
                }
                SET_SDRAM_PAGE(page);
                win_ptr = (U32*)(SDRAM_WINDOW_BASE_ADDRESS + offset);
                ptr -= n;
            }
            else  /* not in SDRAM */
            {
                n = count;
                win_ptr = ptr;
            }
            for (k =0; k < n; k++)
            {
                test = *win_ptr;
                if (test != pattern[w])
                {
#ifdef STTBX_PRINT
                    printf("\n**** Error: Read: %x instead of: %x, at address: %x",
                           test, pattern[w], (U32)win_ptr);
                    if ((U32)win_ptr & 0x4)
                    {
                        printf(" MSB word");
                    }
                    else
                    {
                        printf(" LSB word");
                    }
#endif
                    error = err = TRUE;
                }
                *win_ptr = ~pattern[w];
                test = *win_ptr;
                if (test != (~pattern[w]))
                {
#ifdef STTBX_PRINT
                    printf("\n**** Error: Read: %x instead of: %x, at address: %x",
                           test, ~pattern[w], (U32)win_ptr);
                    if ((U32)win_ptr & 0x4)
                    {
                        printf(" MSB word");
                    }
                    else
                    {
                        printf(" LSB word");
                    }
#endif
                    error = err = TRUE;
                }
                *win_ptr-- = pattern[w];
            }
            count -= n;
        }
        if (err)
        {
            err = FALSE;
#ifdef STTBX_PRINT
            printf("\n");
#endif
        }
    }

    if (error)
    {
        STBOOT_Printf(("FAILED\n"));
    }
    else
    {
        STBOOT_Printf(("OK\n"));
    }

#if (MULTI_PAGE_ACCESS == TRUE)
    /* release the sdram access by pages */
    if(IS_IN_SDRAM(start) && SdramMutex_p != NULL)
    {
        semaphore_signal(SdramMutex_p);
    }
#endif

    return error;

} /* stboot_TestSDRAM () */
#endif

/* End of sdram7015.c */
