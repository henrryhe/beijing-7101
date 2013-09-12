/************************************************************************
File Name   :   ics9161.c

Description :   ICS9161A external clock driver

COPYRIGHT (C) STMicroelectronics 2002

Date               Modification                                     Name
----               ------------                                     ----
06-Oct-00          Created                                           FC
30 Apr 2002        Port to STVTG                                     HSdLM
04 Jul 2002        Fix DDTS GNBvd14076 "ICS9161A configuration       HSdLM
 *                 disable Compress Data Clock"
22 Jul 2002        Fix computing error (U32 overflow)                PC/HSdLM
************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <stsys.h>

#include "ics9161.h"

/* Private Constants -------------------------------------------------------- */

#define SCALING_FACTOR 2  /* Scaling factor to avoid U32 overflow during ICS9161
                            setup computing 2 is enough for input frequencies
                            up to 30MHz */

/* Default clock frequencies */
#define DEFAULT_USRCLK1  27000000
#define DEFAULT_USRCLK2 108000000
#define DEFAULT_USRCLK3  27000000
#define DEFAULT_PIXCLK   74250000
#define DEFAULT_RCLK2    27000000
#define DEFAULT_PCMCLK   27000000
#define DEFAULT_CDCLK    80000000
/* EPLD registers for ICS programmation */
#if defined(mb295) || defined(mb295b) || defined(mb295c)
  #define ICS_CTRL          0x70400000
  #define ICS_DATA          0x70404000

  #define ICS_CTRL_CLKOE1   0x10
  #define ICS_CTRL_CLKOE2   0x20
  #define ICS_CTRL_CLKOE3   0x40
  #define ICS_CTRL_CLKCS    0x80
#elif defined(mb290) || defined(mb290c)
  #define ICS_CTRL          0x61000000
  #define ICS_DATA          0x61100000

  #define ICS_CTRL_CLKOE1   0x01
  #define ICS_CTRL_CLKCS    0x02
#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif

/* ICS Address locations */
#define ICS_REG0              0
#define ICS_REG1              1
#define ICS_REG2              2
#define ICS_MREG              3
#define ICS_PWRDWN            4
#define ICS_CNTLREG           6

/* Private Types ------------------------------------------------------------ */

typedef struct ICS9161_ClkFreq_s
{
    U32 UsrClk1;
    U32 UsrClk2;
    U32 UsrClk3;
    U32 PixClk;
    U32 RClk2;
    U32 PcmClk;
    U32 CdClk;
} ICS_ClkFreq_t;

/* Private variables (static) ----------------------------------------------- */

#if defined(mb295) || defined(mb295b) || defined(mb295c)
#define    ICS_FREF 14318180
#elif defined(mb290) || defined(mb290c)
#define    ICS_FREF 27000000
#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif
static ICS_ClkFreq_t ICS_ClkFreq;

static BOOL ICS_ClkFreqFirstInitDone = FALSE;

/* Private Macros ----------------------------------------------------------- */

#define STSYS_WriteIcsReg8(reg, value) STSYS_WriteRegDev8((reg), (value))

/* Global variables --------------------------------------------------------- */

/* Prototypes --------------------------------------------------------------- */
static void Unlock( void );
static void Startbit( void );
static void Progbit(U8 bit0, U8 bit1, U8 bit2, U8 bit3);
static void Stopbit( void );
static void ConfigPLL(S32 pll[4], S32 addr[4]);
static void ICS9161_InitializeExternalClocks(void);
static U32 ComputeICSSetup(U32 Frequency);
static U32 ComputeActualFrequency(U32 RegValue);

/* Private Functions (static) ----------------------------------------------- */

/*----------------------------------------------------------------------------
 Name   : Unlock()
 Purpose: Private function to produce unlock sequence for ICS9161s
 In     :
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
static void Unlock()
{
    S32 i;

    STSYS_WriteIcsReg8(ICS_DATA, 0x0F);
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1);

    for(i=0; i<5; i++)
    {
        STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS | ICS_CTRL_CLKOE1);  /* Rising edge of CS */
        STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1);                   /* Falling edge of CS */
    }
}

/*----------------------------------------------------------------------------
 Name   : Startbit
 Purpose: Private function to produce startbit sequence for ICS9161A
 In     :
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
static void Startbit()
{
    STSYS_WriteIcsReg8(ICS_DATA, 0x0);                                /* Start bit has data = 0 */
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS | ICS_CTRL_CLKOE1);   /* Rising edge of CS */
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1);                    /* Falling edge of CS */
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS | ICS_CTRL_CLKOE1);   /* Rising edge of CS */
}


/*----------------------------------------------------------------------------
 Name   : Progbit
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
static void Progbit(U8 bit0, U8 bit1, U8 bit2, U8 bit3)
{
    U8 workdata;

    workdata = (U8)((bit0 | (bit1<<1) | (bit2<<2) | (bit3<<3)) & 0xF );

    STSYS_WriteIcsReg8(ICS_DATA, ~workdata);                          /* Complement on falling */
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1);                    /* Falling edge of CS */
    STSYS_WriteIcsReg8(ICS_DATA, workdata);                           /* True data on rising */
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS | ICS_CTRL_CLKOE1);   /* Rising edge of CS */
}


/*----------------------------------------------------------------------------
 Name   : Stopbit
 Purpose: Private function to produce stop sequence for ICS9161s
 In     :
 Out    :
 Errors :
 Note   :
------------------------------------------------------------------------------
*/
static void Stopbit()
{
    STSYS_WriteIcsReg8(ICS_DATA, 0x0F);
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1);                    /* Falling edge of CS */
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS | ICS_CTRL_CLKOE1);   /* Rising edge of CS */
}

/*******************************************************************************
Name        : ConfigPLL
Description : Configure the 4 PLL at the same time
Parameters  :
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void ConfigPLL(S32 pll[4], S32 addr[4])
{
    S32 i;

    /* Initialise ClkControl bus into known state */
    STSYS_WriteIcsReg8(ICS_DATA, 0x0F);
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1);

    /* Start programming sequence */
    Unlock();
    Startbit();
    for (i=0; i<21; i++) /* Output data bits - LSB first */
    {
        Progbit( ((pll[0]>>i)&1), ((pll[1]>>i)&1), ((pll[2]>>i)&1), ((pll[3]>>i)&1));
    }
    for (i=0; i<3; i++) /* Output address bits - LSB first */
    {
        Progbit( ((addr[0]>>i)&1), ((addr[1]>>i)&1), ((addr[2]>>i)&1), ((addr[3]>>i)&1));
    }
    Stopbit();
} /* end of ConfigPLL() */

/*******************************************************************************
Name        : ICS9161_InitializeExternalClocks
Description : initialize the ICS1961 External clocks.
Parameters  : none
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void ICS9161_InitializeExternalClocks(void)
{
#if defined(mb295) || defined(mb295b) || defined(mb295c)
                        /* N.C.        USRCLK2             PIXCLK          PCMCLK  */
    S32 vcodata[4];
    S32 vcoaddr[4]  = {ICS_REG2,      ICS_REG2,           ICS_REG2,       ICS_REG2};

                       /* CD Clk       USRCLK1             USRCLK3         RCLK2     */
    S32 mregdata[4];
    S32 mregaddr[4] = {ICS_MREG,      ICS_MREG,           ICS_MREG,       ICS_MREG};

    vcodata[0] = ComputeICSSetup(27000000);
    vcodata[1] = ICS_ClkFreq.UsrClk2;
    vcodata[2] = ICS_ClkFreq.PixClk;
    vcodata[3] = ICS_ClkFreq.PcmClk;

    mregdata[0] = ICS_ClkFreq.CdClk;
    mregdata[1] = ICS_ClkFreq.UsrClk1;
    mregdata[2] = ICS_ClkFreq.UsrClk3;
    mregdata[3] = ICS_ClkFreq.RClk2;

    STOS_InterruptLock();
    ConfigPLL(vcodata, vcoaddr);
    ConfigPLL(mregdata, mregaddr);

    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1   /* Rising edge of CS */
                               | ICS_CTRL_CLKOE2
                               | ICS_CTRL_CLKOE3
                             | ICS_CTRL_CLKCS);
    STSYS_WriteIcsReg8(ICS_DATA, 0x0F);
    STOS_InterruptUnLock();

#elif defined(mb290) || defined(mb290c)
                                  /* PIXCLK               N.C                 N.C                 N.C     */
    S32 vcodata[4];
    S32 vcoaddr[4]  = {ICS_REG2,      ICS_REG2,           ICS_REG2,       ICS_REG2};

                                  /* USRCLK2          N.C                 N.C                 N.C     */
    S32 mregdata[4];
    S32 mregaddr[4] = {ICS_MREG,      ICS_MREG,           ICS_MREG,       ICS_MREG};

    vcodata[0] = ICS_ClkFreq.PixClk;
    vcodata[1] = ComputeICSSetup(27000000);
    vcodata[2] = ComputeICSSetup(27000000);
    vcodata[3] = ComputeICSSetup(27000000);
    mregdata[0] = ICS_ClkFreq.UsrClk2;
    mregdata[1] = ComputeICSSetup(27000000);
    mregdata[2] = ComputeICSSetup(27000000);
    mregdata[3] = ComputeICSSetup(27000000);

    STOS_InterruptLock();
    ConfigPLL(vcodata, vcoaddr);
    ConfigPLL(mregdata, mregaddr);

    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1   /* Rising edge of CS */
                               | ICS_CTRL_CLKCS);
    STSYS_WriteIcsReg8(ICS_DATA, 0x0F);
    STOS_InterruptUnLock();

#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif
} /* end of ICS9161_InitializeExternalClocks() */

/*******************************************************************************
Name        : ComputeICSSetup
Description :
Parameters  :
Assumptions :
Limitations : Max Frequency : 120000000
Returns     : RegValue
*******************************************************************************/
static U32 ComputeICSSetup(U32 Frequency)
{
    U32 Fvco, DivOut, R, I, M, N, RealFvco, FvcoError, CurrM, CurrN, CurrRealFvco, CurrFvcoError, RegValue;
    U32 ScaledInputFrequency;
    U32 ScaledFrequency;

    ScaledInputFrequency = ICS_FREF/SCALING_FACTOR;
    ScaledFrequency      = Frequency/SCALING_FACTOR;

    DivOut = 1;
    R = 0;
    M = 2; /* otherwise might be used uninitialized, this is a not-impact value */
    N = 3; /* otherwise might be used uninitialized, this is a not-impact value */
    Fvco = Frequency * DivOut;
    while ((Fvco < 50000000) && (DivOut <= 128))
    {
        DivOut *= 2;
        R++;
        Fvco = Frequency*DivOut;
    }
    if(R == 8)
    {
        return(0);
    }

    if(Fvco <= 51000000)
        I = 0;
    else if(Fvco <= 53200000)
        I = 1;
    else if(Fvco <= 58500000)
        I = 2;
    else if(Fvco <= 60700000)
        I = 3;
    else if(Fvco <= 64400000)
        I = 4;
    else if(Fvco <= 66800000)
        I = 5;
    else if(Fvco <= 73500000)
        I = 6;
    else if(Fvco <= 75600000)
        I = 7;
    else if(Fvco <= 80900000)
        I = 8;
    else if(Fvco <= 83200000)
        I = 9;
    else if(Fvco <= 91500000)
        I = 10;
    else if(Fvco <= 100000000)
        I = 11;
    else
        I = 12;

    FvcoError = 0xFFFFFFFF;
    for(CurrM=3 ; CurrM<=129 ; CurrM++)
    {
        CurrN = ((Fvco/SCALING_FACTOR) * CurrM)/(2 * ScaledInputFrequency);

        if((CurrN >= 4) && (CurrN <= 130))
        {
            CurrRealFvco = ((2 * ScaledInputFrequency * CurrN) / CurrM)*SCALING_FACTOR;
            if(CurrRealFvco < Fvco)
                CurrFvcoError = Fvco - CurrRealFvco;
            else
                CurrFvcoError = CurrRealFvco - Fvco;
            if(CurrFvcoError < FvcoError)
            {
                M = CurrM;
                N = CurrN;
                RealFvco = CurrRealFvco;
                FvcoError = CurrFvcoError;
            }
        }

        CurrN++;

        if((CurrN >= 4) && (CurrN <= 130))
        {
            CurrRealFvco = ((2 * ScaledInputFrequency * CurrN) / CurrM)*SCALING_FACTOR;
            if(CurrRealFvco < Fvco)
                CurrFvcoError = Fvco - CurrRealFvco;
            else
                CurrFvcoError = CurrRealFvco - Fvco;
            if(CurrFvcoError < FvcoError)
            {
                M = CurrM;
                N = CurrN;
                RealFvco = CurrRealFvco;
                FvcoError = CurrFvcoError;
            }
        }
    }
    for(CurrN=4 ; CurrN<=130 ; CurrN++)
    {
        CurrM = (2 * ScaledInputFrequency * CurrN)/ (Fvco/SCALING_FACTOR);

        if((CurrM >= 3) && (CurrN <= 129))
        {
            CurrRealFvco = ((2 * ScaledInputFrequency * CurrN) / CurrM)*SCALING_FACTOR;
            if(CurrRealFvco < Fvco)
                CurrFvcoError = Fvco - CurrRealFvco;
            else
                CurrFvcoError = CurrRealFvco - Fvco;
            if(CurrFvcoError < FvcoError)
            {
                M = CurrM;
                N = CurrN;
                RealFvco = CurrRealFvco;
                FvcoError = CurrFvcoError;
            }
        }

        CurrM++;

        if((CurrM >= 3) && (CurrN <= 129))
        {
            CurrRealFvco = ((2 * ScaledInputFrequency * CurrN) / CurrM)*SCALING_FACTOR;
            if(CurrRealFvco < Fvco)
                CurrFvcoError = Fvco - CurrRealFvco;
            else
                CurrFvcoError = CurrRealFvco - Fvco;
            if(CurrFvcoError < FvcoError)
            {
                M = CurrM;
                N = CurrN;
                RealFvco = CurrRealFvco;
                FvcoError = CurrFvcoError;
            }
        }
    }
    RegValue = (M - 2) |
               (R << 7) |
               ((N - 3) << 10) |
               (I << 17);
    return(RegValue);
} /* end of ComputeICSSetup() */

/*******************************************************************************
Name        : ComputeActualFrequency
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : frequency
*******************************************************************************/
static U32 ComputeActualFrequency(U32 RegValue)
{
    U32 M, N, R, OutDiv, Fout;
    U32 ScaledInputFrequency;
    U32 ScaledFout;

    ScaledInputFrequency = ICS_FREF/SCALING_FACTOR;

    M = (RegValue & 0x7F) + 2;
    R = (RegValue >> 7) & 0x7;
    N = ((RegValue >> 10) & 0x7F) + 3;

    OutDiv     = 1 << R;
    ScaledFout = (2*N*ScaledInputFrequency)/(M*OutDiv);
    Fout       = ScaledFout*SCALING_FACTOR;
    return(Fout);
} /* end of ComputeActualFrequency() */

/*******************************************************************************
Name        : stvtg_ICS9161SetPixClkFrequency
Description : Set external ICS9161 pixel clock frequency
Parameters  :
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void stvtg_ICS9161SetPixClkFrequency(U32 Frequency)
{
    U32 ActualFrequency;

    if (!ICS_ClkFreqFirstInitDone)
    {
#if defined(mb295) || defined(mb295b) || defined(mb295c)
        ICS_ClkFreq.UsrClk1     = ComputeICSSetup(DEFAULT_USRCLK1);
        ICS_ClkFreq.UsrClk2     = ComputeICSSetup(DEFAULT_USRCLK2);
        ICS_ClkFreq.UsrClk3     = ComputeICSSetup(DEFAULT_USRCLK3);
        ICS_ClkFreq.RClk2       = ComputeICSSetup(DEFAULT_RCLK2);
        ICS_ClkFreq.PcmClk      = ComputeICSSetup(DEFAULT_PCMCLK);
        ICS_ClkFreq.CdClk       = ComputeICSSetup(DEFAULT_CDCLK);
#elif defined(mb290) || defined(mb290c)
        ICS_ClkFreq.UsrClk2     = ComputeICSSetup(DEFAULT_USRCLK2);
#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif
        ICS_ClkFreqFirstInitDone = TRUE;
    }
    ICS_ClkFreq.PixClk = ComputeICSSetup(Frequency);
    ActualFrequency = ComputeActualFrequency((U32)ICS_ClkFreq.PixClk);
    ICS9161_InitializeExternalClocks();
} /* end of stvtg_ICS9161SetPixClkFrequency() */


/* ----------------------------- End of file ------------------------------ */
