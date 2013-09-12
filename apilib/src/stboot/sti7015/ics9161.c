/************************************************************************
File Name   : 	ics9161.c

Description : 	ICS9161A external clock driver

COPYRIGHT (C) STMicroelectronics 2001

************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "stsys.h"

#include "ics9161.h"

/* Private Constants -------------------------------------------------------- */

/* Default clock frequencies */
#define DEFAULT_USRCLK1 ICS_27000;
#define DEFAULT_USRCLK2 ICS_100000;
#define DEFAULT_USRCLK3 ICS_27000;
#define DEFAULT_PIXCLK  ICS_74250;
#define DEFAULT_RCLK2   ICS_27000;
#define DEFAULT_PCMCLK  ICS_27000;
#define DEFAULT_CDCLK   ICS_80000;

/* EPLD registers for ICS programmation */
#if defined(mb295)
  #define ICS_CTRL          0x70400000
  #define ICS_DATA          0x70404000

  #define ICS_CTRL_CLKOE1   0x10
  #define ICS_CTRL_CLKOE2   0x20
  #define ICS_CTRL_CLKOE3   0x40
  #define ICS_CTRL_CLKCS    0x80
#elif defined(mb290) || defined(mb376)
  #define ICS_CTRL          0x61000000
  #define ICS_DATA          0x61100000

  #define ICS_CTRL_CLKOE1   0x01
  #define ICS_CTRL_CLKCS    0x02
#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif

/* ICS Address locations */
#define ICS_REG0	0
#define ICS_REG1	1
#define ICS_REG2	2
#define ICS_MREG	3
#define ICS_PWRDWN	4
#define ICS_CNTLREG	6

/* Private Types ------------------------------------------------------------ */

/* Private variables (static) ----------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Global variables --------------------------------------------------------- */

/* Prototypes --------------------------------------------------------------- */
static void unlock( void );
static void startbit( void );
static void progbit(U8 bit0, U8 bit1, U8 bit2, U8 bit3);
static void stopbit( void );
static void configPLL(S32 pll[4], S32 addr[4]);

/* Private Functions (static) ----------------------------------------------- */

/*----------------------------------------------------------------------------
 Name   : unlock()
 Purpose: Private function to produce unlock sequence for ICS9161s
 In     :
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
static void unlock()
{
    S32 i;

    STSYS_WriteRegDev8(ICS_DATA, 0x0F);
    STSYS_WriteRegDev8(ICS_CTRL, 0x0);

    for(i=0; i<5; i++)
    {
    	STSYS_WriteRegDev8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
    	STSYS_WriteRegDev8(ICS_CTRL, 0x0);	      /* Falling edge of CS */
    }
}


/*----------------------------------------------------------------------------
 Name   : startbit
 Purpose: Private function to produce startbit sequence for ICS9161A
 In     :
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
static void startbit()
{
    STSYS_WriteRegDev8(ICS_DATA, 0x0);  	  /* Start bit has data = 0 */
    STSYS_WriteRegDev8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
    STSYS_WriteRegDev8(ICS_CTRL, 0x0);  	  /* Falling edge of CS */
    STSYS_WriteRegDev8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
}


/*----------------------------------------------------------------------------
 Name   : progbit
 Purpose: Program data bit into ICS1961 generator. Packages two bits into nibble
          to program both ICS1961 generators at the same time.
 In     : U8 bit0 : bit for first ICS1961
          U8 bit1 : bit for second ICS1961
          U8 bit2 : bit for third ICS1961
          U8 bit3 : bit for fourth ICS1961
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
static void progbit(U8 bit0, U8 bit1, U8 bit2, U8 bit3)
{
    U8 workdata;

    workdata = (U8)((bit0 | (bit1<<1) | (bit2<<2) | (bit3<<3)) & 0xF );

    STSYS_WriteRegDev8(ICS_DATA, ~workdata);	  /* Complement on falling */
    STSYS_WriteRegDev8(ICS_CTRL, 0x0);  	  /* Falling edge of CS */
    STSYS_WriteRegDev8(ICS_DATA, workdata);	  /* True data on rising */
    STSYS_WriteRegDev8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
}


/*----------------------------------------------------------------------------
 Name   : stopbit
 Purpose: Private function to produce stop sequence for ICS9161s
 In     :
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
static void stopbit()
{
    STSYS_WriteRegDev8(ICS_DATA, 0x0F);
    STSYS_WriteRegDev8(ICS_CTRL, 0x0);  	  /* Falling edge of CS */
    STSYS_WriteRegDev8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
}

/*----------------------------------------------------------------------------
 Name   : configPLL
 Purpose: Configure the 4 PLL at the same time
 In     :
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
static void configPLL(S32 pll[4], S32 addr[4])
{
    S32 i;

    /* Initialise ClkControl bus into known state */
    STSYS_WriteRegDev8(ICS_DATA, 0x0F);
    STSYS_WriteRegDev8(ICS_CTRL, 0x0);

    /* Start programming sequence */
    unlock();
    startbit();
    for (i=0; i<21; i++) /* Output data bits - LSB first */
    {
        progbit( ((pll[0]>>i)&1), ((pll[1]>>i)&1), ((pll[2]>>i)&1), ((pll[3]>>i)&1));
    }
    for (i=0; i<3; i++) /* Output address bits - LSB first */
    {
        progbit( ((addr[0]>>i)&1), ((addr[1]>>i)&1), ((addr[2]>>i)&1), ((addr[3]>>i)&1));
    }
    stopbit();
}

/* Public Functions (not static) -------------------------------------------- */

/*----------------------------------------------------------------------------
 Name   : Ics9161InitializeExternalClocks
 Purpose: To initialize the ICS1961 External clocks.
 In     :
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
static void Ics9161InitializeExternalClocks(ICS_ClkFreq_t ICS_ClkFreq)
{
#if defined(mb295)
			/* N.C.		USRCLK2		PIXCLK		PCMCLK 	*/
    S32 vcodata[4]  = {ICS_27000,	ICS_27000,	ICS_27000,	ICS_27000}; /* Default value overwritten */
    S32 vcoaddr[4]  = {ICS_REG2,	ICS_REG2,	ICS_REG2,	ICS_REG2};

			/* CD Clk	USRCLK1		USRCLK3		RCLK2     */
    S32 mregdata[4] = {ICS_27000,	ICS_27000,	ICS_27000,	ICS_27000}; /* Default value overwritten */
    S32 mregaddr[4] = {ICS_MREG,	ICS_MREG,	ICS_MREG,	ICS_MREG};

    vcodata[1] = ICS_ClkFreq.UsrClk2;
    vcodata[2] = ICS_ClkFreq.PixClk;
    vcodata[3] = ICS_ClkFreq.PcmClk;

    mregdata[0] = ICS_ClkFreq.CdClk;
    mregdata[1] = ICS_ClkFreq.UsrClk1;
    mregdata[2] = ICS_ClkFreq.UsrClk3;
    mregdata[3] = ICS_ClkFreq.RClk2;

    configPLL(vcodata, vcoaddr);
    configPLL(mregdata, mregaddr);

    STSYS_WriteRegDev8(ICS_CTRL, ICS_CTRL_CLKOE1   /* Rising edge of CS */
    			       | ICS_CTRL_CLKOE2
    			       | ICS_CTRL_CLKOE3
    			       | ICS_CTRL_CLKCS);
    STSYS_WriteRegDev8(ICS_DATA, 0x0F);

#elif defined(mb290) || defined(mb376)
    /* PIXCLK                           N.C                         N.C                             N.C  */
    S32 vcodata[4]  = {ICS_74250,       ICS_27000,              ICS_27000,              ICS_27000};
    S32 vcoaddr[4]  = {ICS_REG2,        ICS_REG2,               ICS_REG2,               ICS_REG2};

    /* USRCLK2                  N.C                         N.C                             N.C         */
    S32 mregdata[4] = {ICS_100000,      ICS_27000,              ICS_27000,              ICS_27000};
    S32 mregaddr[4] = {ICS_MREG,        ICS_MREG,               ICS_MREG,               ICS_MREG};

    vcodata[0] = ICS_ClkFreq.PixClk;
    mregdata[0] = ICS_ClkFreq.UsrClk2;

    configPLL(vcodata, vcoaddr);
    configPLL(mregdata, mregaddr);

    /* For 7020 cut 2.0, the 74.25 MHz needed for HD output pixel clock can be
      generated internally. To avoid a clash on the PIXCKI pin, we disable this
      external clock generator by not setting ICS_CTRL_CLKOE1. It's also not
      needed for 7015 on mb290 */

    STSYS_WriteRegDev8(ICS_CTRL, ICS_CTRL_CLKCS);   /* Rising edge of CS, but not enabled */
    STSYS_WriteRegDev8(ICS_DATA, 0x0F);

#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif

}

/*----------------------------------------------------------------------------
 Name   : stboot_Ics9161Init
 Purpose: Initialize the ICS1961.
 In     :
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
void stboot_Ics9161Init(void)
{
    ICS_ClkFreq_t ICS_ClkFreq;

#ifdef STTBX_PRINT
    printf("stboot_Ics9161Init: Configuring External Clock Gen... ");
#endif

#if defined(mb295)
    ICS_ClkFreq.UsrClk1 = DEFAULT_USRCLK1;
    ICS_ClkFreq.UsrClk2 = DEFAULT_USRCLK2;
    ICS_ClkFreq.UsrClk3 = DEFAULT_USRCLK3;
    ICS_ClkFreq.PixClk  = DEFAULT_PIXCLK;
    ICS_ClkFreq.RClk2	= DEFAULT_RCLK2;
    ICS_ClkFreq.PcmClk  = DEFAULT_PCMCLK;
    ICS_ClkFreq.CdClk	= DEFAULT_CDCLK;
#elif defined(mb290) || defined(mb376)
    ICS_ClkFreq.UsrClk2 = DEFAULT_USRCLK2;
    ICS_ClkFreq.PixClk  = DEFAULT_PIXCLK;
#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif
    Ics9161InitializeExternalClocks(ICS_ClkFreq);

#ifdef STTBX_PRINT
    printf("OK\n");
#endif
}


/* ----------------------------- End of file ------------------------------ */
