/*****************************************************************************

File name : be_7015.c

Description : Back end specific code of STBOOT

COPYRIGHT (C) STMicroelectronics 2001.

*****************************************************************************/

/* Private Definitions (internal use only) -------------------------------- */

#if defined(ST_7020)
#define DDR_CLK_FREQ            (U32)108E6
#else
#define DDR_CLK_FREQ            (U32)100E6
#endif
#define REF_CLK_FREQ            (U32)27E6

/* Includes --------------------------------------------------------------- */

#include "stdio.h"
#include "stddefs.h"

#include "stdevice.h"

#include "stsys.h"

#include "stboot.h"
#include "be_7015.h"
#include "clk_gen.h"
#include "bereg.h"

#include "cfg_reg.h"
#include "sdrm7015.h"


/* Private Types ----------------------------------------------------------- */


/* Private Variables (static)----------------------------------------------- */

#ifdef STBOOT_INSTALL_BACKEND_INTMGR

/* interrupts management */
static semaphore_t *InterruptSemaphoreArray[INT_NBR_OF_LINES];

static BOOL InBackendInterrupt = FALSE;
static volatile U32 BackendEnableCount = 0;

#endif /* STBOOT_INSTALL_BACKEND_INTMGR */

/* Private Macros ---------------------------------------------------------- */

#ifdef STTBX_PRINT
#define STBOOT_Printf(x) printf x
#else
#define STBOOT_Printf(x)
#endif

/* Private Function prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */

#if defined (ST_7020) && !defined(mb290) && !defined(mb295)

#include "sti2c.h"

#ifndef STBOOT_DB573_PCF8575_ADDRESS
/* The following address assumes the default SW3 setting 1=2=3=ON (low).
  This clashes with another PCF8575 on mb382 (with default option resistor
  settings), so you can override it with the above CFLAG (DDTS 21194) */
#define STBOOT_DB573_PCF8575_ADDRESS 0x40
#endif

/******************************************************************************
Name        : STBOOT_Init7020STEM
Description : Initialise 7020 on db573
Parameters  : Name of the STEM I2C device (SSC1 on mb314)
Assumptions :
Limitations :
Returns     : ST_ERROR_CODE, i.e. success
******************************************************************************/
ST_ErrorCode_t  STBOOT_Init7020STEM( const ST_DeviceName_t I2CDeviceName,
                                     U32 SDRAMFrequency )
{
    ST_ErrorCode_t      Err;
    STI2C_OpenParams_t  I2cOpenParams;
    STI2C_Handle_t      I2cHandle;
    U8                  Buffer[2];
    U32                 ActLen;

    /* We need to override a couple of points from the standard config file:
      5514 AUX_CLK (synth2) is used as the 7020 27 MHz clock (this should be
      fine despite what the datasheet says about a 20 MHz limit) and the EMI
      configuration needed is different */

    /* The clockgen has been left unlocked by the config file */

    /* SYNTH2_CONFIG0: NDIV=1, SDIV=3, MD=15, DISABLE=0, OP_EN=1,
      PLL_SEL=0, PLL_DIV=0 */
    STSYS_WriteRegDev32LE(0x20013140, 0x000009ED);

    /* SYNTH2_CONFIG1: PE = 0 */
    STSYS_WriteRegDev32LE(0x20013144, 0x00000000);


#if defined(mb314)
    /* On mb314, we use EMI bank 2 (STEM0, db573 J1=1-2) */

    /* EMI_CONFIGDATA0: DEVICETYPE=peripheral, PORTSIZE=32-bit,
      BEACTIVE=r&w, OEACTIVE=r, CSACTIVE=r&w, BUSRELEASETIME=0,
      DATADRIVEDELAY=0, LATCHPOINT=end of access,
      WAITPOLARITY=activehigh, WE_USE_OE_CONFIG=0*/
    STSYS_WriteRegDev32LE(0x20200180, 0x000006E9);

    /* EMI_CONFIGDATA1: Read timings: BEE2TIME=BEE1TIME=OEE2TIME=OEE1TIME=0,
      CSE2TIME=1, CSE1TIME=2, ACCESSTIME=6, CYCLENOTPHASE=0 */
    STSYS_WriteRegDev32LE(0x20200188, 0x06210000);

    /* EMI_CONFIGDATA2: Write timings: BEE2TIME=BEE1TIME=OEE2TIME=OEE1TIME=0,
      CSE2TIME=1, CSE1TIME=2, ACCESSTIME=6, CYCLENOTPHASE=0*/
    STSYS_WriteRegDev32LE(0x20200190, 0x06210000);

    /* EMI_CONFIGDATA3: not used in Peripheral mode*/
    STSYS_WriteRegDev32LE(0x20200198, 0x00000209);
#elif defined(mb382)
    /* Must use bank 2 (STEM CS1: db573 J1=2-3)
      because db573 requires address expansion */

    /* EMI_CONFIGDATA0: DEVICETYPE=peripheral, PORTSIZE=16-bit,
      BEACTIVE=OEACTIVE=CSACTIVE=r&w, BUSRELEASETIME=2, DATADRIVEDELAY=0,
      LATCHPOINT=1 clk before end of access, WAITPOLARITY=activehigh,
      WE_USE_OE_CONFIG=0*/
    STSYS_WriteRegDev32LE(0x20200180, 0x001017F1);

    /* EMI_CONFIGDATA1: Read timings: BEE2TIME=BEE1TIME=OEE2TIME=OEE1TIME=0,
      CSE2TIME=1, CSE1TIME=2, ACCESSTIME=7, CYCLENOTPHASE=0 */
    STSYS_WriteRegDev32LE(0x20200188, 0x07210000);

    /* EMI_CONFIGDATA2: Write timings: BEE2TIME=BEE1TIME=OEE2TIME=OEE1TIME=0,
      CSE2TIME=1, CSE1TIME=2, ACCESSTIME=7, CYCLENOTPHASE=0*/
    STSYS_WriteRegDev32LE(0x20200190, 0x07210000);

    /* EMI_CONFIGDATA3: not used in Peripheral mode*/
    STSYS_WriteRegDev32LE(0x20200198, 0x00000000);

    interrupt_lock(); /* protect read/modify/write against other processes */

    /* Enable EMI bank 2 address expansion in (CONFIG_CONTROL_E bit 21):
      this outputs BE3 on the CAS line, to clock the low and high words
      of each 32-bit access */
    STSYS_WriteRegDev32LE(0x20010028,
        STSYS_ReadRegDev32LE(0x20010028) | (1<<21));

    /* Turn off EMI bank 2 address bus shifting in EMI_GENCFG,
      because we are using address expansion instead */
    STSYS_WriteRegDev32LE(0x20200028,
        STSYS_ReadRegDev32LE(0x20200028) & ~(1<<8));

    interrupt_unlock();
    
#elif defined(mb376)
    /* Must use bank 4 (STEM CS1: db573 J1=2-3)
      because db573 requires address expansion */

    /* EMI_CONFIGDATA0: DEVICETYPE=peripheral, PORTSIZE=32-bit,
      BEACTIVE=OEACTIVE=CSACTIVE=r&w, BUSRELEASETIME=2, DATADRIVEDELAY=0,
      LATCHPOINT=1 clk before end of access, WAITPOLARITY=activehigh,
      WE_USE_OE_CONFIG=0*/
      
    STSYS_WriteRegDev32LE(0x1A100200, 0x001176e9);

    /* EMI_CONFIGDATA1: Read timings: BEE2TIME=BEE1TIME=OEE2TIME=OEE1TIME=0,
      CSE2TIME=1, CSE1TIME=2, ACCESSTIME=7, CYCLENOTPHASE=0 */
    /* STSYS_WriteRegDev32LE(0x1A100208, 0x04112121); */
    STSYS_WriteRegDev32LE(0x1A100208, 0x06313131);

    /* EMI_CONFIGDATA2: Write timings: BEE2TIME=BEE1TIME=OEE2TIME=OEE1TIME=0,
      CSE2TIME=1, CSE1TIME=2, ACCESSTIME=7, CYCLENOTPHASE=0*/
    /* STSYS_WriteRegDev32LE(0x1A100210, 0x07112211); */
    STSYS_WriteRegDev32LE(0x1A100210, 0x06141111);

    /* EMI_CONFIGDATA3: not used in Peripheral mode*/
    STSYS_WriteRegDev32LE(0x1A100218, 0x0000000a);

    interrupt_lock(); /* protect read/modify/write against other processes */

    /* Turn off EMI bank 2 address bus shifting in EMI_GENCFG,
      because we are using address expansion instead */
    STSYS_WriteRegDev32LE(0x1A100028, 0x00000118);

    interrupt_unlock();
    
#else
#error Unrecognised Motherboard for db573 EMI configuration
#endif

    /* The 7020 requires a valid 27 MHz clock as it comes out of reset. On
      db573, this clock comes from the AUX_CLK output of the 5514, and cannot
      be relied upon until the 5514 config has completed, well after the
      initial system reset. Thus, we must reset the 7020 explicitly, via the
      PCF8575 I2C PIO port. We also take the opportunity to enable the digital
      video interface and setup a standard Audio DAC configuration (I2S mode) */

    I2cOpenParams.I2cAddress        = STBOOT_DB573_PCF8575_ADDRESS;
    I2cOpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.BusAccessTimeOut  = 100000;

    Err = STI2C_Open (I2CDeviceName, &I2cOpenParams, &I2cHandle);
    if (Err != ST_NO_ERROR)
    {
        STBOOT_Printf(("STBOOT_Init7020STEM: Cannot open I2C device driver\n"));
        Err = STBOOT_ERROR_STI2C;
    }
    else
    {
        /* db573 connections:
         * 0[7]: system reset (active low)
         * 0[6:4]: CDTI, CSN, CCLK: control bus ignored when CSN=1
         * 0[3]: SMUTE (active high)
         * 0[2:0]: CKS[0:2]: all 0 for 256 Fs
         * 1[7]: Video PD168
         * 1[6:4]: CDTI, CSN, CCLK: control bus ignored when CSN=1
         * 1[3]: PnotS (we want 'Parallel' mode)
         * 1[2:0]: CKS[0:2]: all 0 for 256 Fs
         */
        Buffer[0] = 0x20; /* port 0 (reset active) */
        Buffer[1] = 0x24; /* port 1 (digital video disabled) */

        Err = STI2C_Write (I2cHandle, Buffer, 2, 1000, &ActLen);
        if (Err == ST_NO_ERROR)
        {
            /* the time taken to do an I2C Write
              should give the required reset duration of 1 us */

            Buffer[0] = 0xa0; /* port 0 (reset inactive) */
            Buffer[1] = 0xa4; /* enable digital video this time */
            Err = STI2C_Write (I2cHandle, Buffer, 2, 1000, &ActLen);
        }

        if (Err != ST_NO_ERROR)
        {
            STBOOT_Printf(("STBOOT_Init7020STEM: Error 0x%08X writing to the I2C device.\n", Err));
            Err = STBOOT_ERROR_STI2C;
        }

        if (STI2C_Close (I2cHandle) != ST_NO_ERROR)
        {
            STBOOT_Printf(("STBOOT_Init7020STEM: Failed to close the I2C driver.\n"));
            Err = STBOOT_ERROR_STI2C;
        }
    }


    if (Err == ST_NO_ERROR)
    {
        /* Now we can do the normal backend init */
        Err = stboot_BackendInit7015(SDRAMFrequency);
    }

    return Err;
}

#endif /* db573 */

/******************************************************************************
Name        : stboot_BackEndInit7015
Description : Initialisation of the back end drivers
Parameters  : U32 SDRAMFrequency
Assumptions :
Limitations :
Returns     : ST_ERROR_CODE, i.e. success
******************************************************************************/
ST_ErrorCode_t stboot_BackendInit7015(U32 SDRAMFrequency)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL           RetErr  = FALSE;

    /* Board initialisation */
    /* This init should not be located here, but due to */
    /* the stboot architecture, we can not separate it */
    /* from the stboot component. */
    RetErr = stboot_BoardInit();
    if (RetErr == TRUE)
    {
        ErrCode = STBOOT_ERROR_BACKEND_MISMATCH; /* We should use an error ! */
        return ErrCode;
    }

    /* Protect register read/re-write */
    interrupt_lock();

   
#if defined(ST_7020) && (defined(ST_5516) || defined(ST_5517))
    /* need to configure 7020 host interface in 16 bit mode */
    /* this means all writes must be 32 bits wide (split to 2*16-bit by EMI) */
    
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + EMPI_5516), 0x01);

#endif

#ifndef HARDWARE_EMULATOR
    /* Initialize Clock Gen */
    /* The returned error code is not the right one */ /* need to be deleted ??? */
    STBOOT_Printf(("InitBackend: Configuring Clock Gen... "));

    if ( stboot_InitClockGen(REF_CLK1, REF_CLK_FREQ, DDR_CLK_FREQ) )
    {
        ErrCode = STBOOT_ERROR_BACKEND_MISMATCH;
        return ErrCode;
    }

    STBOOT_Printf(("OK\n"));
#endif	/* not HARDWARE_EMULATOR */

    /* Display circuit clocks */
    stboot_DisplayClockGenStatus();

#if defined(mb295)
    /* Set the default page to access STi7015 DDR memory on first 8Mb */
    STSYS_WriteRegDev16LE((FPGA_BASE_ADDRESS + FPGA_7020_PAGE), 0);
#endif /* mb295 */

    /* Initialize SDRAM */
    if (stboot_InitSDRAM7015(SDRAMFrequency) == TRUE)
    {
        ErrCode = STBOOT_ERROR_SDRAM_INIT;
        return ErrCode;
    }

    /* Enable interfaces */
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + CFG_CCF),
                          CFG_CCF_ECDI | /* Enable Compressed Data interface, */
                          CFG_CCF_EVI    /* DMAs and video interface */
        );

    /* Configure Comp. Data I/F (common to audio and video) */
    /* Compressed data I/F enabled, No Subpicture Select */
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + CFG_CDCF),0);
    /* No Comp. data Grouping */
    STSYS_WriteRegDev32LE((void *) (CFG_BASE_ADDRESS + CFG_CDG),0);

    /* Enable S/PDIF, external audio I/F and audio outputs */
    STSYS_WriteRegDev32LE((CFG_BASE_ADDRESS + CFG_AUDIO_IO), CFG_AUDIO_IO_INIT);

    /* Configure Hardware Gamma Pipe Priorities with a default value */
#if defined (ST_7020)
    STSYS_WriteRegDev32LE((GAMMA_BASE_ADDRESS + CFG_GAM_PGAMMA), 0x277DDDDD);
#else
    STSYS_WriteRegDev32LE((GAMMA_BASE_ADDRESS + CFG_GAM_PGAMMA), 0x27DDDDDD);
#endif


    /* Release register read/re-write protection */
    interrupt_unlock();

#ifdef STBOOT_INSTALL_BACKEND_INTMGR

    /* Initialize interrupt management */
    /* Install the low level interrupt handler */
    STBOOT_Printf(("InitBackend: Installing interrupt handler... "));
    interrupt_lock();
    interrupt_install(BACKEND_INTERRUPT, BACKEND_INTERRUPT_LEVEL, (void(*)(void*))BackendInterruptHandler, NULL);
    interrupt_unlock();

#endif /* STBOOT_INSTALL_BACKEND_INTMGR */

    STBOOT_Printf(("OK\n"));
    return( ErrCode );
} /* stboot_BackendInit7015 */

#ifdef STBOOT_INSTALL_BACKEND_INTMGR

/* ---------------------------------------------------------------------------
 Interrupt routine: parse interrupt-line register bits and dispatch
 Param:     void
 Return:	void
 --------------------------------------------------------------------------- */

void BackendInterruptHandler(void)
{
    U32 int_line, i;

    /* lock interrupts */
    interrupt_lock();

    InBackendInterrupt = TRUE;

    /* get source of interruption */
    int_line = STSYS_ReadRegDev32LE(CFG_BASE_ADDRESS + EMPI_INT_LINE);

    /* test all the bits */
    for (i=0; i<INT_NBR_OF_LINES; i++, int_line >>= 1)
    {
        if ((int_line & 0x01) && (InterruptSemaphoreArray[i] != NULL))
        {
            /* signal corresponding task */
            semaphore_signal(InterruptSemaphoreArray[i]);
        }
    }

    InBackendInterrupt = TRUE;

    /* unlock interrupts */
    interrupt_unlock();
}


/* --------------------------------------------------------------------------
 Register an interrupt semaphore for given interrupt line
 Param:     Semaphore_p			semaphore on which a signal() should be done
 			InterruptLine		corresponding interrupt line (bit of INT_LINE register)
 Return:						void
 ---------------------------------------------------------------------------*/
void STBOOT_RegisterSemaphore(semaphore_t *Semaphore_p, U8 InterruptLine)
{
    InterruptSemaphoreArray[InterruptLine] = Semaphore_p;
}

/* ---------------------------------------------------------------------------
 Un-register the interrupt semaphore of the given interrupt line
 Param:     InterruptLine corresponding interrupt line (bit of INT_LINE
            register)
 Return:    void
  ---------------------------------------------------------------------------*/
void STBOOT_UnRegisterSemaphore(U8 InterruptLine)
{
    InterruptSemaphoreArray[InterruptLine] = NULL;
}

/* --------------------------------------------------------------------------
 Disable 7015/20 backend interrupts
 Note : works properly even in case of nesting calls
 Param: void
 Return:					void
   -------------------------------------------------------------------------*/
void STBOOT_DisableBackendIT(void)
{
    if (!InBackendInterrupt)
    {
        if (BackendEnableCount == 1)
    	{
            interrupt_disable(BACKEND_INTERRUPT_LEVEL);
    	}
        BackendEnableCount--;
    }
}

/* --------------------------------------------------------------------------
 Enable 7015/20 backend interrupts
 Note : works properly even in case of nesting calls
 Param: void
 Return:					void
 ---------------------------------------------------------------------------*/
void STBOOT_EnableBackendIT(void)
{
    if (!InBackendInterrupt)
    {
        if (BackendEnableCount == 0)
    	{
            interrupt_enable(BACKEND_INTERRUPT_LEVEL);
    	}
        BackendEnableCount++;
    }
}

#endif /* STBOOT_INSTALL_BACKEND_INTMGR */

/* End of be_7015.c */
