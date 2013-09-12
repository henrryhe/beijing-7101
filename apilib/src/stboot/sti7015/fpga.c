/*******************************************************************************
File Name   : fpga.c

Description : FPGA configuration from TFF file
              + Some EPLD initialization

COPYRIGHT (C) STMicroelectronics 2001

*******************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "stdio.h"

#include "stlite.h"             /* STLite includes */
#include "stdevice.h"           /* Device includes */
#include "stddefs.h"

#include "stsys.h"              /* STAPI includes */
#include "stcommon.h"

#include "fpga.h"              /* Local includes */

/* Private types/constants ------------------------------------------------ */

#if defined(ST_7020)
#define IDCOD                   0x1d40e041
#else /* default 7015 */
#define IDCOD                   0x1d404041
#endif

#define NRSSFPGA_PROGADDR       0x70408000
#define NRSSFPGA_CTRLADDR       0x7040c000

#define FPGA_IDENTADDR          0x7043C000
#define FPGA_TESTADDR           0x7043C004

#define FPGAnotCONFIG           0x01
#define FPGA_CONFDONE           0x02
#define FPGAnotSTATUS           0x04

#define FPGA_ENABLETEST TRUE

/* Private variables ------------------------------------------------------ */

/* Private function prototypes -------------------------------------------- */
#if defined(mb295)
static void delay(int timedelay);
#endif

/* Global variables ------------------------------------------------------- */

/* Function definitions --------------------------------------------------- */

/************************************************************************
 * Simple delay routine.
 ************************************************************************/
#if defined(mb295)
static void delay(int timedelay)
{
    volatile int i, a = 0;

    for (i = 0; i<(timedelay*100); i++) {a=a+1;}
}
#endif

/************************************************************************
 * This routine is a self contained routine to PROGRAM the EPF10K50 device
 * on the MB295.
 * Return : TRUE if error, FALSE if no.
 ************************************************************************/
#if defined(mb295)
BOOL stboot_FpgaInit()
{
    BOOL RetErr;
    /* Create array with FPGA program information */
    static const char nrssfpga[] = {
#ifdef STBOOT_SUPPORT_SH4_OVERDRIVE
    #include "fpga_sh4.ttf"   /* Output file from Maxplus2 FPGA compiler */
#else
#ifdef HARDWARE_EMULATOR
    #include "fpga_he.ttf"    /* Output file from Maxplus2 FPGA compiler */
#else
    #include "fpga.ttf"       /* Output file from Maxplus2 FPGA compiler */
#endif
#endif
    ,0,0 };

    /* Define pointers to FPGA registers */
    volatile unsigned short *fpgaprog  = (volatile unsigned short *)(NRSSFPGA_PROGADDR);
    volatile unsigned short *fpgactrl  = (volatile unsigned short *)(NRSSFPGA_CTRLADDR);
    int i, j;

    RetErr = FALSE;

#ifdef STTBX_PRINT
    printf(" *************************************\n");
    printf(" *****      MB295-B Board        *****\n");
#if defined(ST_7020)
    printf(" ***** WITH    STi7020 IC        *****\n");
#else
    printf(" ***** WITH    STi7015 IC        *****\n");
#endif
#ifdef STBOOT_SUPPORT_SH4_OVERDRIVE
    printf(" ***** WITH    SH4 Overdrive     *****\n");
#else
    printf(" ***** WITHOUT SH4 Overdrive     *****\n");
#ifdef HARDWARE_EMULATOR
    printf(" ***** WITH Hardware Emulator    *****\n");
#else
    printf(" ***** WITHOUT Hardware Emulator *****\n");
#endif
#endif
    printf(" *************************************\n");

    printf("STBOOT_FpgaInit: Starting FPGA programming...\n");
#endif
    /* Reset notCONFIG to set NRSSFPGA into "wait to be programmed" mode      */

    /* CONFIG = "1" */
    STSYS_WriteRegDev16LE(fpgactrl, FPGAnotCONFIG);
    delay(1000);

    /* CONFIG = "0" */
    STSYS_WriteRegDev16LE(fpgactrl, 0);
    delay(1000);

    /* CONFIG = "1" */
    STSYS_WriteRegDev16LE(fpgactrl, FPGAnotCONFIG);
    delay(1000);

    /* Waiting for notSTATUS to go high - ready to be programmed */
    while ( (STSYS_ReadRegDev16LE(fpgactrl) & FPGAnotSTATUS)==0)
    {
#ifdef STTBX_PRINT
        printf(".\n");
#endif
    }

    /* Copying data to NRSSFPGA */
    for (i = 0; i < sizeof(nrssfpga); i++)
    {
        for (j=0; j<8; j++)
        {
            /* Copy array to FPGA - bit at a time */
            STSYS_WriteRegDev16LE(fpgaprog, ((nrssfpga[i] >> j) & 0x01));
        }
    }

    /* Waiting for CONFDONE to go high - means the program is complete    */
    while ( (STSYS_ReadRegDev16LE(fpgactrl) & FPGA_CONFDONE)==0)
    {
        STSYS_WriteRegDev16LE(fpgaprog, 0);
    }

    /* Clock another 10 times - gets the device into a working state */
    for (i=0; i<10; i++)
    {
        STSYS_WriteRegDev16LE(fpgaprog, 0);
    }

    /* Now should be ready to use */
#ifdef STTBX_PRINT
    printf("STBOOT_FpgaInit: FPGA programming completed.\n");
#endif

    return RetErr;
} /*STBOOT_FpgaInit () */
#endif

/************************************************************************
 * Put all resets high in the EPLD.
 * Return : TRUE if error, FALSE if no.
 ************************************************************************/
BOOL stboot_EpldInit()
{
#if defined(mb295)
#if ((defined STTBX_PRINT) || (FPGA_ENABLETEST == TRUE))
    volatile unsigned short *fpgaident = (volatile unsigned short *)(FPGA_IDENTADDR);
#endif
    volatile unsigned short *fpgamode  = (volatile unsigned short *)(0x7043C010);
    BOOL RetErr;

    RetErr = FALSE;

    interrupt_lock();   /* Prevent any glitch on IRQ lines */

#ifdef STTBX_PRINT
    printf("stboot_EpldInit: Raising EPLD resets");
#endif
    STSYS_WriteRegDev16LE(0x70414000, 0x1);  /* SAA7114 reset */
    STSYS_WriteRegDev16LE(0x7041C000, 0x1);  /* Audio reset */
    STSYS_WriteRegDev16LE(0x70420000, 0x1);  /* Tuner reset */
    STSYS_WriteRegDev16LE(0x70424000, 0x1);  /* ATAPI reset */
#ifdef STTBX_PRINT
    printf(" OK\n");

    printf("stboot_EpldInit: Resetting FPGA... ");
#endif
    STSYS_WriteRegDev16LE(0x70428000, 0x1);    /* FPGA reset low */
#ifdef STTBX_PRINT
    printf("OK\n");

    printf("stboot_EpldInit: Enabling MemWait... ");
#endif
    STSYS_WriteRegDev16LE(0x70424000, 0x3);  /* ATAPI reset + MemWaitEn */
#ifdef STTBX_PRINT
    printf("OK\n");

    /* Read the FPGA Ident register */
    printf("stboot_EpldInit: FPGA Identity: %x\n", *fpgaident);
#endif

#ifdef STBOOT_SUPPORT_SH4_OVERDRIVE
    STSYS_WriteRegDev16LE(0x70430000, 0x1);     /* DMA used */
#else
    STSYS_WriteRegDev16LE(0x70430000, 0x0);     /* DMA not used */
#endif

    /* Write default values */
#ifdef HARDWARE_EMULATOR
    *fpgamode = 0x00;   /* SH4 not used, CD clock out disable */
#else
#ifdef STBOOT_SUPPORT_SH4_OVERDRIVE
    *fpgamode = 0x09;   /* SH4 used, CD clock out enable */
#else
    *fpgamode = 0x08;   /* SH4 not used, CD clock out disable */
#endif
#endif /* HARDWARE_EMULATOR */
#ifdef STTBX_PRINT
  if (*fpgamode != 0x4321)
  {
      printf("stboot_EpldInit: FPGA Mode: %x (SH4%sUsed, CD clock output %s)\n",
             *fpgamode,
             (*fpgamode & 0x1) ? " " : " not ",
             (*fpgamode & 0x8) ? "enabled" : "disabled");
  }
  else
  {
      printf("STBOOT_EpldInit: Simplified FPGA");
  }
#endif
    STSYS_WriteRegDev16LE(0x70400000, 0x10); /* Clock control: enable CD clk only */

    /* ICS9161 enables:
     * 0x10: enable CD clk
     * 0x20: enable USRCLK1, USRCLK2
     * 0x40: enable USRCLK3, PIXCLK, RCLK2, PCMCLK
     */
#if defined(ST_7020)
    STSYS_WriteRegDev16LE(0x70400000, 0x50);    /* Clock control: enable CD clk, USRCLK2, PIXCLK */
#elif defined(ST_7015)
    STSYS_WriteRegDev16LE(0x70400000, 0x10);    /* Clock control: enable CD clk only */
#else
#error ERROR:invalid DVD_BACKEND defined
#endif

    /* Wait for CD_CLOCK before setting HardReset high */
    task_delay(ST_GetClocksPerSecond()/2);

    STSYS_WriteRegDev16LE(0x70418000, 0x1);  /* STi7020 reset */
    interrupt_unlock();

#if (FPGA_ENABLETEST == TRUE)
    /* Check if FPGA has been well programed */
    if (STSYS_ReadRegDev16LE(fpgaident) != 0x4321)
    {
#ifdef STTBX_PRINT
        printf("stboot_EpldInit: Bad FPGA, Identity check failed !\n");
#endif
        RetErr = TRUE;
    }
#endif 

#if (FPGA_ENABLETEST == TRUE)
    if (RetErr == FALSE)
    {
        /* Check if FPGA allow a correct access to 7015/20 */
        U32 Tmp;

        /* This register does not exist should return 0x1B4 */
        if ((Tmp = STSYS_ReadRegDev32LE(0x60002298)) != 0x1B4)
        {
#ifdef STTBX_PRINT
            printf("STBOOT_FpgaInit: Failed to access to 70xx register read 0x%08x expected 0x%08x.\n", Tmp, 0x1B4);
#endif
            /* If here, fpga found a register where there is not !!!!      */
            RetErr = TRUE;
        }

        /* This register is the IDCOD of the 7015/20, should be constant      */
        if ((Tmp = STSYS_ReadRegDev32LE(0x600001f8)) != IDCOD)
        {
#ifdef STTBX_PRINT
            printf("STBOOT_FpgaInit: Failed to access to 70xx register read 0x%08x expected 0x%08x.\n",
                   Tmp, IDCOD);
#endif
            RetErr = TRUE;
        }
    }
#endif /* (FPGA_ENABLETEST == TRUE) */

    return RetErr;

    /* -------- */

#elif defined(mb290) /* end defined(mb295), now defined(mb295) */

#define ResetReg0setADDR          0x60300000
#define ResetReg0clrADDR          0x60400000
#define ResetReg1setADDR          0x60600000
#define ResetReg1clrADDR          0x60700000
#define cHDMuxRegaddr             0x60D00000
#define DviInPowerDownADDR        0x60F00000

    interrupt_lock();	/* Prevent any glitch on IRQ lines */

#ifdef STTBX_PRINT
    printf("InitEPLD: Raising EPLD resets");
#endif
    STSYS_WriteRegDev8(ResetReg0setADDR, 0x40);     /* SAA7114 enable */
#ifdef STTBX_PRINT
    printf(".");
#endif
    STSYS_WriteRegDev8(ResetReg0setADDR, 0x30);     /* AudioDACs and AudioMux enable */
#ifdef STTBX_PRINT
    printf(".");
#endif
    STSYS_WriteRegDev8(ResetReg0setADDR, 0x04);     /* SIL168 enable */
#ifdef STTBX_PRINT
    printf(".");
#endif
    STSYS_WriteRegDev8(ResetReg1clrADDR, 0x04);     /* SIL159 enable */
#ifdef STTBX_PRINT
    printf(".");
#endif
    STSYS_WriteRegDev8(DviInPowerDownADDR, 0x01);   /* SIL159 powerdown */
#ifdef STTBX_PRINT
    printf(".");
#endif
    STSYS_WriteRegDev8(ResetReg0setADDR, 0x01);     /* GS9020 enable */
#ifdef STTBX_PRINT
    printf(".");
#endif
    /* Ensure that the HDIN input mux is active and that the HDIN clock (HDCKI)
      is driven during reset. Without this, the output of the HD mux is
      tri-stated, which can cause problems during 7020 reset */

    STSYS_WriteRegDev8(ResetReg0setADDR, 0x08);     /* HDin Mux enable */
    STSYS_WriteRegDev8(cHDMuxRegaddr   , 0x03);     /* HDin Mux select input */
#ifdef STTBX_PRINT
    printf(".");
#endif
    STSYS_WriteRegDev8(ResetReg1setADDR, 0x0A);     /* IO_STEM and STEM reset high */
#ifdef STTBX_PRINT
    printf(".");
#endif
    STSYS_WriteRegDev8(ResetReg1setADDR, 0x90);     /* SPDIF and TUNER reset high */
#ifdef STTBX_PRINT
    printf(".");
#endif
    STSYS_WriteRegDev8(ResetReg0setADDR, 0x80);     /* STi7015/20 Reset */
    STSYS_WriteRegDev8(ResetReg0clrADDR, 0x80);
    STSYS_WriteRegDev8(ResetReg0clrADDR, 0x80);     /* hold reset low for at least 2 microseconds */
    STSYS_WriteRegDev8(ResetReg0clrADDR, 0x80);     /* (each write is about 800 ns) */
    STSYS_WriteRegDev8(ResetReg0setADDR, 0x80);
#ifdef STTBX_PRINT
    printf(" Ok\n");
#endif

    interrupt_unlock();

    return FALSE;

    /* -------- */


#elif defined(mb376) /* end defined(mb290), now defined(mb376) */

    interrupt_lock();	/* Prevent any glitch on IRQ lines */

#ifdef STTBX_PRINT
    printf(" Enabling interrupts through FPGA ...");
#endif

    /* MASK 0 Line 5 from STEM int 0 only */
    STSYS_WriteRegDev8((void *)0x420c0000, 0x20);
    /* MASK 1 Nothing */
    STSYS_WriteRegDev8((void *)0x42100000, 0x00);


#ifdef STTBX_PRINT
    printf(" Ok\n");
#endif

    interrupt_unlock();
    
    return FALSE;
    
#else
#error ERROR: Invalid DVD_PLATFORM defined
#endif
} /* stboot_EpldInit () */

/* End of fpga.c */
