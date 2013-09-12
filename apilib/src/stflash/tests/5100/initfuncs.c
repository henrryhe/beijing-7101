/****************************************************************************

File Name   : initfuncs.c

Description : Initialization functions, used incase of Boot from Flash.

Copyright (C) 2004, ST Microelectronics

 ****************************************************************************/
 /* Includes ----------------------------------------------------------- */

#include <initfuncs.h>
#include "stsys.h"
#include "reg.h"

/* Private Types ------------------------------------------------------ */

/* Private Constants -------------------------------------------------- */

/* Private Variables -------------------------------------------------- */

/* Private Macros ----------------------------------------------------- */
#define PLL_A_FREQUENCY     532286
#define PLL_B_FREQUENCY     405000

#define PLL_A           0
#define PLL_B           1
#define INPUT_CLOCK_FREQUENCY   27000 /* Input clock frequency in KHz */

#define CPU_DIV                 2
#define LMI_DIV                 4
#define BLITTER_DIV             3
#define FDMA_DIV                2
#define LS_INTERCONNECT_DIV     5
#define VIDEO_DIV               6
#define HS_INTERCONNECT_DIV     4
#define FLASH_DIV               5 /* FLASH source is the same as LS_INTERCONECT */

#define CPU_SOURCE              PLL_B
#define LMI_SOURCE              PLL_A
#define BLITTER_SOURCE          PLL_A
#define FDMA_SOURCE             PLL_A
#define LS_INTERCONNECT_SOURCE  PLL_A
#define VIDEO_SOURCE            PLL_A
#define HS_INTERCONNECT_SOURCE  PLL_A

#define CPU_INDEX               0
#define LMI_INDEX               1
#define BLITTER_INDEX           2
#define LS_INTERCONNECT_INDEX   3
#define FDMA_INDEX              4
#define VIDEO_INDEX             5
#define HS_INTERCONNECT_INDEX   6
#define FLASH_INDEX             7

#define HALF_VALUE      1
#define WHOLE_VALUE     0


/* Private Function prototypes ---------------------------------------- */

/* Functions ---------------------------------------------------------- */

static void Simudelay(int);
#pragma ST_nolink(Simudelay)
static void sti5100_init_clocks(void);
#pragma ST_nolink(sti5100_init_clocks)
static void sti5100_InitPLLFrequency(U32 PLLFrequency, U32 PLLSource);
#pragma ST_nolink(sti5100_InitPLLFrequency)
static void sti5100_SetupDividers(void);
#pragma ST_nolink(sti5100_SetupDividers)
static void sti5100_SetupFrequencySynthesizer(void);
#pragma ST_nolink(sti5100_SetupFrequencySynthesizer)
static void sti5100_SetPLLClockDivider(U32 clock, U32 div, U32 half, U32 source, S32 phase);
#pragma ST_nolink(sti5100_SetPLLClockDivider)
static void STi51xx_InitLMI_Cas20_MB390(void);
#pragma ST_nolink(STi51xx_InitLMI_Cas20_MB390)
static void sti5100_InterconnectPriority( void );
#pragma ST_nolink(sti5100_InterconnectPriority)
static void sti5100_set_default_config(void);
#pragma ST_nolink(sti5100_set_default_config)
static void MB390_ConfigureFmi(void);
#pragma ST_nolink(MB390_ConfigureFmi)
static void MB390_ConfigureSysConf(void);
#pragma ST_nolink(MB390_ConfigureSysConf)


/* Functions -------------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : PostPokeLoopCallback
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
void PostPokeLoopCallback(void)
{
}

/*-------------------------------------------------------------------------
 * Function : PrePokeLoopCallback
 * Input    : None
 * Output   :
 * Return   : None
 * Comment  : Function automatically called after the poke loop
 * ----------------------------------------------------------------------*/
void PrePokeLoopCallback(void)
{
    sti5100_set_default_config ();

    /* Set up the PLL's */
    sti5100_init_clocks ();

    /*Set Interconnect Priority*/
    sti5100_InterconnectPriority ();

    STi51xx_InitLMI_Cas20_MB390 ();

    /* Configure FMI */
    MB390_ConfigureFmi ();

    /* General Configuration */
    MB390_ConfigureSysConf ();

    /* EPLD configurations for DB499 */
    #if defined(ENABLE_EPLD_DB499_SERIAL)
        STSYS_WriteRegDev32LE(0x41800000,0x000B000B);
    #elif defined(ENABLE_EPLD_DB499_PARALLEL)
        STSYS_WriteRegDev32LE(0x41800000,0x00080008);
    #endif
}

/*-------------------------------------------------------------------------
 * Function : Simudelay
 * Input    : delay
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void Simudelay(int delay)
{
    volatile int j,dummy;

    for ( j = 0 ; j < delay ; j ++ )
       dummy = STSYS_ReadRegDev32LE(0x007C);
}

/*-------------------------------------------------------------------------
 * Function : sti5100_init_clocks
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void sti5100_init_clocks (void)
{
    volatile U32    Source,
                    LockA,
                    LockB,
                    count = 0,
                    config_c,
                    x,y;

    /* Starting clock generator setup */
    /* Unlock clockgen registers */
    STSYS_WriteRegDev32LE(STI5100_REGISTER_LOCK,0xF0);
    STSYS_WriteRegDev32LE(STI5100_REGISTER_LOCK,0x0F);

    /* Now Transistion to X1 mode */
    STSYS_WriteRegDev32LE(STI5100_MODE_CONTROL,0x01);
    Simudelay (1000);

    /* Select the PLL source */
    Source = (HS_INTERCONNECT_SOURCE << 8) | (VIDEO_SOURCE << 7) | (FDMA_SOURCE << 6) | (LMI_SOURCE << 5);
    Source = (Source) | (HS_INTERCONNECT_SOURCE << 4) | (BLITTER_SOURCE << 3) | (LMI_SOURCE << 2);
    Source = (Source) | (CPU_SOURCE << 1);

    STSYS_WriteRegDev32LE(STI5100_CLOCK_SELECT_CFG,Source);

    /* Setup PLL_A and PLL_B */
    sti5100_InitPLLFrequency(PLL_A_FREQUENCY,PLL_A);
    sti5100_InitPLLFrequency(PLL_B_FREQUENCY,PLL_B);

    LockA = 0;
    LockB = 0;
    count = 0;

    if (count == 0)
    {
        while (count < 20 && ((LockA == 0) || (LockB == 0)))
        {
            x = STSYS_ReadRegDev32LE (STI5100_PLLA_CONFIG1);
            y = STSYS_ReadRegDev32LE(STI5100_PLLB_CONFIG1);
            if ((x & 0x4000) == 0x4000) LockA = 1; /* Check for PLL_A Lock */
            if ((y & 0x4000) == 0x4000) LockB = 1; /* Check for PLL_A Lock */
            count++;
        }
        if (LockA == 0)
        {
            /* write PLL_A failed to LOCK! */
            return;
        }
        if (LockB == 0)
        {
            /* write PLL_B failed to LOCK! */
            return;
        }
    }

    sti5100_SetupDividers ();
    sti5100_SetupFrequencySynthesizer ();

    Simudelay (10000);

    /*
    ** Set the correct divider ratio for the PCM clock to the MMDSP
    ** See DDTS GNBvd28890
    */

    config_c = STSYS_ReadRegDev32LE(STI5100_CONFIG_CONTROL_C);
    config_c = (config_c) | (0x00004000);

    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_C,config_c);

    /* Now Transistion to programmed mode */
    STSYS_WriteRegDev32LE(STI5100_MODE_CONTROL,0x02);
    Simudelay (1000);

    /* Lock clockgen registers */
    STSYS_WriteRegDev32LE(STI5100_REGISTER_LOCK,0x100);

}

/*-------------------------------------------------------------------------
 * Function : sti5100_InitPLLFrequency
 * Input    : PLLFrequency, PLLSource
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void sti5100_InitPLLFrequency ( U32 PLLFrequency, U32 PLLSource )
{
    volatile U32    Pfactor,
                    Divider,
                    Reminder,
                    StoredReminder,
                    Index,
                    IndexMax,
                    tmp,
                    Nfactor,
                    Mfactor;

    if ((PLLFrequency < 10000) || (PLLFrequency > 650000))
    {
        /* -->  Invalid PLL Frequency : (PLLFrequency) Mhz */
        return;
    }

    if ((10000 <= PLLFrequency) && (PLLFrequency <=300000))
    {
        /*
        ** Pfactor = 1, Fvco = 2 * Fpll
        ** in this case Fpll = (Nfactor/Mfactor) * Fclockin (Fclockin is 27 MHz)
        ** so Nfactor = (Fpll * Mfactor) / Fclockin
        */
        Pfactor = 1;
        Divider = 1;
    }

    if ((300000 <= PLLFrequency) && (PLLFrequency <=650000))
    {
        /*
        ** Pfactor = 0, Fvco = Fpll
        ** in this case Fpll = (2*Nfactor/Mfactor) * Fclockin (Fclockin is 27 MHz)
        ** so Nfactor = (Fpll * Mfactor) / (2*Fclockin)
        */
        Pfactor = 0;
        Divider = 2;
    }

    StoredReminder = 100000;
    IndexMax = 27;

    for (Index=5 ; Index<=IndexMax ; Index++)
    {
        Reminder = (PLLFrequency*Index) % (INPUT_CLOCK_FREQUENCY*Divider);
        tmp = (PLLFrequency*Index) / (INPUT_CLOCK_FREQUENCY*Divider);

        if ((Reminder == 0) && (tmp < 126))
        {
            Mfactor = Index;
            Nfactor = tmp;
            Index = 100;
        }

        if (Reminder > Index)
        {
            if ((Reminder - Index < StoredReminder) && (tmp < 126) && (Reminder != 0))
            {
                StoredReminder = Reminder;
                Mfactor = Index;
                Nfactor = tmp;
            }
        }
        else
        {
            if ((Index - Reminder < StoredReminder) && (tmp < 126) && (Reminder != 0))
            {
                StoredReminder = Reminder;
                Mfactor = Index;
                Nfactor = tmp;
            }
        }
    }

    PLLFrequency = (Divider*Nfactor*INPUT_CLOCK_FREQUENCY)/Mfactor;

    if (PLLSource == PLL_A)
    {
        STSYS_WriteRegDev32LE(STI5100_PLLA_CONFIG0,(((Nfactor-1)<<8) | (Mfactor-1)));
        STSYS_WriteRegDev32LE(STI5100_PLLA_CONFIG1,(0x4838 | Pfactor));
    }

    if (PLLSource == PLL_B)
    {
        STSYS_WriteRegDev32LE(STI5100_PLLB_CONFIG0,(((Nfactor-1)<<8) | (Mfactor-1)));
        STSYS_WriteRegDev32LE(STI5100_PLLB_CONFIG1,(0x4838 | Pfactor));
    }
    Simudelay (50);
}

/*-------------------------------------------------------------------------
 * Function : sti5100_SetupDividers
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void sti5100_SetupDividers(void)
{
    /* Setup PLL dividers for STi5100 chip */

    sti5100_SetPLLClockDivider(CPU_INDEX,               CPU_DIV,               WHOLE_VALUE,   CPU_SOURCE,               0);
    sti5100_SetPLLClockDivider(LMI_INDEX,               LMI_DIV,               WHOLE_VALUE,   LMI_SOURCE,               0);
    sti5100_SetPLLClockDivider(BLITTER_INDEX,           BLITTER_DIV,           WHOLE_VALUE,   BLITTER_SOURCE,           0);
    sti5100_SetPLLClockDivider(LS_INTERCONNECT_INDEX,   LS_INTERCONNECT_DIV,   WHOLE_VALUE,   LS_INTERCONNECT_SOURCE,   0);
    sti5100_SetPLLClockDivider(FDMA_INDEX,              FDMA_DIV,              WHOLE_VALUE,   FDMA_SOURCE,              0);
    sti5100_SetPLLClockDivider(VIDEO_INDEX,             VIDEO_DIV,             WHOLE_VALUE,   VIDEO_SOURCE,             0);
    sti5100_SetPLLClockDivider(HS_INTERCONNECT_INDEX,   HS_INTERCONNECT_DIV,   WHOLE_VALUE,   HS_INTERCONNECT_SOURCE,   0);
    sti5100_SetPLLClockDivider(FLASH_INDEX,             FLASH_DIV,             WHOLE_VALUE,   LS_INTERCONNECT_SOURCE,   0);
}

/*-------------------------------------------------------------------------
 * Function : sti5100_SetupFrequencySynthesizer
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void sti5100_SetupFrequencySynthesizer(void)
{
    volatile U32    ctrl_d,
                    MD,SDIV,PE;

    /* Setup PLL Frequency Synthesizers for STi5100 chip */
    STSYS_WriteRegDev32LE(STI5100_FSA_SETUP,0x4);
    STSYS_WriteRegDev32LE(STI5100_FSB_SETUP,0x4);

    STSYS_WriteRegDev32LE(STI5100_DCO_MODE_CFG,0x01);

    MD   = 0x1f;
    SDIV = 0x2;
    PE   = 0x0000;

    /* Pixel clock = 27 MHz */
    STSYS_WriteRegDev32LE(STI5100_PIX_CLK_SETUP0,(0xE00 | (SDIV)<<6 | (MD)));
    STSYS_WriteRegDev32LE(STI5100_PIX_CLK_SETUP1,(PE));

    /* PCM clock = 24.576 MHz */
    STSYS_WriteRegDev32LE(STI5100_PCM_CLK_SETUP0,0x0ef1);
    STSYS_WriteRegDev32LE(STI5100_PCM_CLK_SETUP1,0x3600);

    /* SPDIF clock = 27 MHz */
    STSYS_WriteRegDev32LE(STI5100_SPDIF_CLK_SETUP0,0x0EBF);
    STSYS_WriteRegDev32LE(STI5100_SPDIF_CLK_SETUP1,0x0000);

    /* Smart Card clock = 27 MHz */
    STSYS_WriteRegDev32LE(STI5100_SC_CLK_SETUP0,0x0EBF);
    STSYS_WriteRegDev32LE(STI5100_SC_CLK_SETUP1,0x0000);

    /* DAA clock = 32.4 MHz */
    STSYS_WriteRegDev32LE(STI5100_DAA_CLK_SETUP0,0x0EBA);
    STSYS_WriteRegDev32LE(STI5100_DAA_CLK_SETUP1,0x0000);

    /* USB clock = 48 MHz */
    STSYS_WriteRegDev32LE(STI5100_USB_CLK_SETUP0,0x0EB1);
    STSYS_WriteRegDev32LE(STI5100_USB_CLK_SETUP1,0x0000);

    /* AUX clock = 27 MHz */
    STSYS_WriteRegDev32LE(STI5100_AUX_CLK_SETUP0,0x0EBF);
    STSYS_WriteRegDev32LE(STI5100_AUX_CLK_SETUP1,0x0000);

    /* AUDIO clock = 160 MHz */
    ctrl_d = STSYS_ReadRegDev32LE(STI5100_CONFIG_CONTROL_D);
    ctrl_d = (ctrl_d) & ~(1<<26);
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_D,ctrl_d);

    STSYS_WriteRegDev32LE(STI5100_AUDIO_CLK_SETUP1,0x3333);
    STSYS_WriteRegDev32LE(STI5100_AUDIO_CLK_SETUP0,0x0E35);

    STSYS_WriteRegDev32LE(STI5100_DCO_MODE_CFG,0x00);
}

/*-------------------------------------------------------------------------
 * Function : sti5100_SetPLLClockDivider
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void sti5100_SetPLLClockDivider( U32 clock, U32 div, U32 half, U32 source, S32 phase )
{
    volatile U32    PLL,
                    config_0,
                    config_1,
                    config_2,
                    PLL_MHz_x10,
                    div_x10,
                    depth,
                    pattern,
                    bpat,
                    temptoppat,
                    newpat,
                    offset;

    /*
     * If phase is not passed in Function Call it must be Zero
     * phase=0; Handled in Fn Call itself
     */
    if (source == PLL_A)
    {
        PLL = PLL_A_FREQUENCY*1000;
    }
    else
    {
        PLL = PLL_B_FREQUENCY*1000;
    }

    config_0 = 0;
    config_1 = 0;
    config_2 = 0;

    switch ( div )
    {
        case 2:
            if (half==WHOLE_VALUE) /* divide by 2 */
            {
                config_0 = 0x0AAA;
                config_2 = 0x0071;
            }
            else if (half==HALF_VALUE) /* divide by 2.5 */
            {
                config_0 = 0x5AD6;
                config_2 = 0x0054;
            }
            break;

        case 3:
            if (half==WHOLE_VALUE)    /* divide by 3 */
            {
                config_0 = 0x0DB6;
                config_2 = 0x0011;
            }
            else if (half==HALF_VALUE) /* divide by 3.5 */
            {
                config_0 = 0x366C;
                config_2 = 0x0053;
            }
            break;

        case 4:
            if (half==WHOLE_VALUE)    /* divide by 4 */
            {
                config_0 = 0xCCCC;
                config_2 = 0x0075;
            }
            else if (half==HALF_VALUE)  /* divide by 4.5 */
            {
                config_0 = 0x399C;
                config_1 = 0x0003;
                config_2 = 0x0057;
            }
            break;

        case 5:
            if (half==WHOLE_VALUE)  /* divide by 5 */
            {
                config_0 = 0x739c;
                config_2 = 0x0014;
            }
            else if (half==HALF_VALUE)  /* divide by 5.5 */
            {
                config_0 = 0x071C;
                config_2 = 0x0050;
            }
            break;

        case 6:
            if (half==WHOLE_VALUE)  /* divide by 6 */
            {
                config_0 = 0x0E38;
                config_2 = 0x0071;
            }
            else if (half==HALF_VALUE)  /* divide by 6.5 */
            {
                config_0 = 0x1C78;
                config_2 = 0x0052;
            }
            break;

        case 7:
            if (half==WHOLE_VALUE)  /* divide by 7 */
            {
                config_0 = 0x3C78;
                config_2 = 0x0013;
            }
            else if (half==HALF_VALUE)  /* divide by 7.5 */
            {
                config_0 = 0x7878;
                config_2 = 0x0054;
            }
            break;

        case 8:
            if (half==WHOLE_VALUE)  /* divide by 8 */
            {
                config_0 = 0xF0F0;
                config_2 = 0x0075;
            }
            else if (half==HALF_VALUE)  /* divide by 8.5 */
            {
                config_0 = 0xE1F0;
                config_1 = 0x0001;
                config_2 = 0x0056;
            }
            break;

        case 9:
            if (half==WHOLE_VALUE) /* divide by 9 */
            {
                config_0 = 0xE1F0;
                config_1 = 0x0003;
                config_2 = 0x0017;
            }
            else if (half==HALF_VALUE)    /* divide by 9.5 */
            {
                config_0 = 0xC1F0;
                config_1 = 0x0007;
                config_2 = 0x0058;
            }
            break;

        case 10:
            if (half==WHOLE_VALUE) /* divide by 10 */
            {
                config_0 = 0x83e0;
                config_1 = 0x000f;
                config_2 = 0x0079;
            }
            break;

        case 11:
            if (half==WHOLE_VALUE)  /* divide by 11 */
            {
                config_0 = 0x07e0;
                config_2 = 0x0010;
            }
            break;

        case 12:
            if (half==WHOLE_VALUE)  /* divide by 12 */
            {
                config_0 = 0x0fc0;
                config_2 = 0x0071;
            }
            break;

        case 13:
            if (half==WHOLE_VALUE)  /* divide by 13 */
            {
                config_0 = 0x1FC0;
                config_2 = 0x0012;
            }
            break;

        case 14:
            if (half==WHOLE_VALUE) /* divide by 14 */
            {
                config_0 = 0x3F80;
                config_2 = 0x0073;
            }
            break;

        case 15:
            if (half==WHOLE_VALUE)  /* divide by 15 */
            {
                config_0 = 0x7F80;
                config_2 = 0x0014;
            }
            break;

        case 16:
            if (half==WHOLE_VALUE)  /* divide by 16 */
            {
                config_0 = 0xFF00;
                config_1 = 0x0000;
                config_2 = 0x0015;
            }
            break;

        case 17:
            if (half==WHOLE_VALUE)  /* divide by 17 */
            {
                config_0 = 0xFF00;
                config_1 = 0x0001;
                config_2 = 0x0016;
            }
            break;

        case 18:
            if (half==WHOLE_VALUE)  /* divide by 18 */
            {
                config_0 = 0xFE00;
                config_1 = 0x0003;
                config_2 = 0x0077;
            }
            break;

        case 19:
            if (half==WHOLE_VALUE)  /* divide by 19 */
            {
                config_0 = 0xFE00;
                config_1 = 0x0007;
                config_2 = 0x0018;
            }
            break;

        case 20:
            if (half==WHOLE_VALUE)  /* divide by 20 */
            {
                config_0 = 0xFC00;
                config_1 = 0x000F;
                config_2 = 0x0019;
            }
            break;
    }

    if (config_0 == 0)
    {
        /* Divider value (div) not supported for module (names[clock]) */
        return;
    }

    /* Re-adjust PLL and div to allow for half value division using integers. */
    PLL_MHz_x10 = (PLL / 1000000) * 10;

    if (half==(WHOLE_VALUE))
    {
        div_x10 = div * 10;  /* whole value x10 */
    }
    else
    {
        div_x10 = (div * 10) + 5;  /* half value x10 */
    }

    if (phase > 0)
    {
        /* Calculate the depth of the pattern */
        depth = (config_2 & 0x0f) + 11;

        /* 32-bit field pattern */
        pattern = config_0 | (config_1 << 16);

        /* Shifted Pattern */
        bpat = pattern >> phase;

        /* Bits of the pattern needed to be rapped */
        temptoppat = pattern & ((1 << phase)-1);

        newpat = bpat | (temptoppat << (depth - phase));

        config_0 = newpat & 0xffff;
        config_1 = newpat >> 16;
    }
    else if (phase < 0)
    {
        phase = 0-phase;

        /* Calculate the depth of the pattern */
        depth = (config_2 & 0x0f) + 11;

        /* 32-bit field pattern */
        pattern = config_0 | (config_1 << 16);

        /* Shifted Pattern */
        bpat = (pattern << phase) & ((1 << depth)-1);

        /* Bits of the pattern needed to be rapped */
        temptoppat = pattern >> (depth - phase);

        newpat = bpat | temptoppat;
        config_0=newpat & 0xffff;
        config_1=newpat >> 16;
    }

    offset = (clock * 0x10);

    STSYS_WriteRegDev32LE(STI5100_CPU_CLKDIV_CONFIG0+offset, config_0);
    STSYS_WriteRegDev32LE(STI5100_CPU_CLKDIV_CONFIG1+offset, config_1);
    STSYS_WriteRegDev32LE(STI5100_CPU_CLKDIV_CONFIG2+offset, config_2);
}

/*-------------------------------------------------------------------------
 * Function : STi51xx_InitLMI_Cas20_MB390
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void STi51xx_InitLMI_Cas20_MB390(void)
{
    volatile U32    data;

    /* "\n Configuring LMI for 16-bit data CAS 2.0 @ 133MHz " */

    /* Set LMI_COC_UPPER Register, bits [63:32]   (LMI Pad logic) */
    STSYS_WriteRegDev32LE(STI5100_LMI_COC_UPPER,0x000C6750);
    /* Even Though its a 64 bit register the upper 32 bits are reserved and therefore no need to fill*/

    /* Set LMI_COC_LOWER Register, bits [31:0]  (LMI Pad logic)
    ** Bits [19:18] Compensation mode DDR
    ** Bits [17:16] Pad strenght 		 (0x0:5pF, 0x1:15pF, 0x2:25pF, Ox3:35pF)
    ** Bits [15:14] output Impedance (0x0:25Ohm, 0x1:40Ohm, 0x2:55Ohm, Ox3:70Ohm)
    ** Bit  [13]    DLL preset reset value enable
    */
    STSYS_WriteRegDev32LE(STI5100_LMI_COC_LOWER,0x00002000);
    data = STSYS_ReadRegDev32LE(STI5100_LMI_COC_LOWER);
    data = (data | 0x0<<16 | 0x0<<14);
    STSYS_WriteRegDev32LE(STI5100_LMI_COC_LOWER,data);

    /* SDRAM Mode Register
    ** Set Refresh Interval, Enable Refresh, 16-bit bus,
    ** Grouping Disabled, DDR-SDRAM, Enable.
    ** Bits[27:16]: Refresh Interval = 7.8 microseconds (8K/64ms)
    ** @  50MHz =  390 clk cycles -> 0x186
    ** @  75MHz =  585 clk cycles -> 0x249
    ** @ 100MHz =  780 clk cycles -> 0x30C
    ** @ 125MHz =  975 clk cycles -> 0x3CF
    ** @ 133MHz = 1040 clk cycles -> 0x410  <--
    ** @ 166MHz = 1300 clk cycles -> 0x514
    */
    STSYS_WriteRegDev32LE(STI5100_LMI_MIM,0x04100203);

    /* SDRAM Timing Register
    ** For 133MHz (7.5ns) operation:
    **  3 clks RAS_precharge, Trp;
    **  3 clks RAS_to_CAS_delay, Trcd-r;
    **  8 clks RAS cycle time, Trc;
    **  6 clks RAS Active time, Tras;
    **  2 clks RAS_to_RAS_Active_delay, Trrd;
    **  2 clks Last write to PRE/PALL period SDRAM, Twr;
    **  2 clks CAS Latency;
    ** 10 clks Auto Refresh RAS cycle time, Trfc;
    **  Enable Write to Read interruption;
    **  1 clk  Write to Read interruption, Twtr;
    **  3 clks RAS_to_CAS_delay, Trcd-w;
    ** (200/16)=3 clks Exit self-refresh to next command, Tsxsr;
    */
    STSYS_WriteRegDev32LE(STI5100_LMI_STR,0x35085225);

    /* SDRAM Row Attribute 0 & 1 Registers
    ** UBA = 32MB + Base Adr, Quad-bank, Shape 13x9,
    ** Bank Remapping Disabled
    **
    **  LMI base address 	0xC0000000
    **  Memory size 32MB 	0x02000000
    **  Row UBA value    	0xC200
    */
    STSYS_WriteRegDev32LE(STI5100_LMI_SDRA0,0xC2001900);

    /*  We just have one Row connected to cs0, so we must program UBA0 = UBA1,
    **  following LMI specification
    */
    STSYS_WriteRegDev32LE(STI5100_LMI_SDRA1,0xC2001900);

    /*---------------------------------------------------------------------------
    ** Initialisation Sequence for LMI & DDR-SDRAM Device
    **---------------------------------------------------------------------------
    ** 200 microseconds to settle clocks
    */
    Simudelay (100);

    /* SDRAM Control Register */
    /* Clock enable */
    STSYS_WriteRegDev32LE(STI5100_LMI_SCR,0x3);

    /* NOP enable */
    STSYS_WriteRegDev32LE(STI5100_LMI_SCR,0x1);

    /* Precharge all banks */
    STSYS_WriteRegDev32LE(STI5100_LMI_SCR,0x2);

    /* LMI_SDRAM_ROW_MODE0 & LMI_SDRAM_ROW_MODE1 Registers */
    /* EMRS Row 0 & 1: Weak Drive : Enable DLL */
    STSYS_WriteRegDev32LE(STI5100_LMI_SDMR0,0x0402);
    STSYS_WriteRegDev32LE(STI5100_LMI_SDMR1,0x0402);
    Simudelay (100);

    /* MRS Row 0 & 1 : Reset DLL - /CAS = 2.0, Mode Sequential, Burst Length 8 */
    STSYS_WriteRegDev32LE(STI5100_LMI_SDMR0,0x0123);
    STSYS_WriteRegDev32LE(STI5100_LMI_SDMR1,0x0123);

    /* 200 clock cycles required to lock DLL */
    Simudelay (100);

    /* Precharge all banks */
    STSYS_WriteRegDev32LE(STI5100_LMI_SCR,0x2);

    /* CBR enable (auto-refresh) */
    STSYS_WriteRegDev32LE(STI5100_LMI_SCR,0x4);
    STSYS_WriteRegDev32LE(STI5100_LMI_SCR,0x4);
    STSYS_WriteRegDev32LE(STI5100_LMI_SCR,0x4);

    /* LMI_SDRAM_ROW_MODE0 & 1 Registers */
    /* MRS Row 0 & 1 : Normal - /CAS = 2.0, Mode Sequential, Burst Length 8 */
    STSYS_WriteRegDev32LE(STI5100_LMI_SDMR0,0x0023);
    STSYS_WriteRegDev32LE(STI5100_LMI_SDMR1,0x0023);

    /* Normal SDRAM operation, No burst Refresh after standby */
    STSYS_WriteRegDev32LE(STI5100_LMI_SCR,0x0);
}

/*-------------------------------------------------------------------------
* Function : MB390_ConfigureFmi
* Input    : None
* Output   :
* Return   : None
* ----------------------------------------------------------------------*/
static void MB390_ConfigureFmi(void)
{
    /* Ensure all FMI control registers are unlocked */
    /* at reset the state of these regs is 'undefined' */

    STSYS_WriteRegDev32LE(STI5100_FMI_LOCK,0x00000000);
    STSYS_WriteRegDev32LE(STI5100_FMI_STATUS_LOCK,0x00000000);

    /* Number of FMI Banks : Enable all banks */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK_ENABLE,0x00000004);

    /* FMI Bank base addresses
    ** NOTE: bits [0,7] define bottom address bits [22,29] of bank
    ** Bank 0 - 16MBytes Atapi Configured as 16-bit peripheral
    */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK0_BASE,0x00000003); /* 0x40000000 - 0x40FFFFFF */

    /* Bank 1 - 32MBytes Stem0/DVBCI/EPLD Configured as 16-bit peripheral */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK1_BASE,0x0000000B); /* 0x41000000 - 0x42FFFFFF */

    /* Bank 2 - 32MBytes Stem1 Configured as 16-bit peripheral */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK2_BASE,0x00000013); /* 0x43000000 - 0x44FFFFFF */

    /* Bank 3 - 8MBytes ST M58LW064D Flash */
    /* Note only the top 8Mbytes is populated from 0x7F800000 */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK3_BASE,0x000000FF); /* 0x45000000 - 0x7FFFFFFF */

    /*------------------------------------------------------------------------------
    ## Program bank functions
    ##------------------------------------------------------------------------------

    ##------------------------------------------------------------------------------
    ## Bank 0 - 16MBytes Atapi Configured as 16-bit peripheral
    ##------------------------------------------------------------------------------
    ## Parameters: -weuseoeconfig 0 -waitpolarity 0 -latchpoint 16 -datadrivedelay 31
    ##             -busreleasetime 0 -csactive 3 -oeactive 3 -beactive 0 -portsize 16
    ##             -devicetype 1

    ##             -cyclenotphaseread 1 -accesstimeread 62 -cse1timeread 0
    ##             -cse2timeread 2 -oee1timeread 8 -oee2timeread 15 -bee1timeread 2
    ##             -bee2timeread 1

    ##             -cyclenotphasewrite 1 -accesstimewrite 62 -cse1timewrite 0
    ##             -cse2timewrite 2 -oee1timewrite 8 -oee2timewrite 15 -bee1timewrite 2
    ##             -bee2timewrite 2

    ##             -strobeonfalling 0 -burstsize 2 -datalatency 2 -dataholddelay 2
    ##             -burstmode 0
    */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK0_DATA0,0x010f8791);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK0_DATA1,0xbe028f21);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK0_DATA2,0xbe028f21);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK0_DATA3,0x0000000a);

    /*------------------------------------------------------------------------------
    ## Bank 1 - 32MBytes Stem0/DVBCI/EPLD Configured as 16-bit peripheral
    ##------------------------------------------------------------------------------
    ## Parameters: -weuseoeconfig 0 -waitpolarity 0 -latchpoint 1 -datadrivedelay 0
    ##             -busreleasetime 2 -csactive 3 -oeactive 1 -beactive 2 -portsize 16
    ##             -devicetype 1

    ##             -cyclenotphaseread 1 -accesstimeread 1d -cse1timeread 2
    ##             -cse2timeread 0 -oee1timeread 0 -oee2timeread 0 -bee1timeread 0
    ##             -bee2timeread 0

    ##             -cyclenotphasewrite 1 -accesstimewrite 1d -cse1timewrite 2
    ##             -cse2timewrite 2 -oee1timewrite 0 -oee2timewrite 0 -bee1timewrite 0
    ##             -bee2timewrite 0

    ##             -strobeonfalling 0 -burstsize 0 -datalatency 0 -dataholddelay 0
    ##             -burstmode 0
    */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK1_DATA0,0x001016D1);  /*BE not active during rd */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK1_DATA1,0x9d200000);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK1_DATA2,0x9d220000);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK1_DATA3,0x00000000);

    /*------------------------------------------------------------------------------
    ## Bank 2 - 32MBytes Stem1 Configured as 16-bit peripheral
    ##------------------------------------------------------------------------------
    ## Parameters: -weuseoeconfig 0 -waitpolarity 0 -latchpoint 1 -datadrivedelay 0
    ##             -busreleasetime 2 -csactive 3 -oeactive 1 -beactive 2 -portsize 16
    ##             -devicetype 1

    ##             -cyclenotphaseread 1 -accesstimeread 1d -cse1timeread 2
    ##             -cse2timeread 0 -oee1timeread 0 -oee2timeread 0 -bee1timeread 0
    ##             -bee2timeread 0

    ##             -cyclenotphasewrite 1 -accesstimewrite 1d -cse1timewrite 2
    ##             -cse2timewrite 2 -oee1timewrite 0 -oee2timewrite 0 -bee1timewrite 0
    ##             -bee2timewrite 0

    ##             -strobeonfalling 0 -burstsize 0 -datalatency 0 -dataholddelay 0
    ##             -burstmode 0
    */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK2_DATA0,0x001016D1);  /* BE not active during rd */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK2_DATA1,0x9d200000);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK2_DATA2,0x9d220000);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK2_DATA3,0x00000000);

    /* FMI Bank 3 8MBytes ST M58LW064D Flash - ONLY 8MBytes FLASH ON BOARD THIS MAY BE WRONG!
    ##------------------------------------------------------------------------------
    ## Bank 3 - 8MBytes ST M58LW064D Flash
    ##------------------------------------------------------------------------------
    ## Parameters: -weuseoeconfig 0 -waitpolarity 0 -latchpoint 1 -datadrivedelay 0
    ##             -busreleasetime 2 -csactive 3 -oeactive 1 -beactive 2 -portsize 16
    ##             -devicetype 1

    ##             -cyclenotphaseread 1 -accesstimeread 1d -cse1timeread 2
    ##             -cse2timeread 0 -oee1timeread 0 -oee2timeread 0 -bee1timeread 0
    ##             -bee2timeread 0

    ##             -cyclenotphasewrite 1 -accesstimewrite 1d -cse1timewrite 2
    ##             -cse2timewrite 2 -oee1timewrite 0 -oee2timewrite 0 -bee1timewrite 0
    ##             -bee2timewrite 0

    ##             -strobeonfalling 0 -burstsize 0 -datalatency 0 -dataholddelay 0
    ##             -burstmode 0
    */
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK3_DATA0,0x001016D1);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK3_DATA1,0x9d200000);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK3_DATA2,0x9d220000);
    STSYS_WriteRegDev32LE(STI5100_FMI_BANK3_DATA3,0x00000000);

    /*  ------- Program Other FMI Registers --------
    ** sdram refresh bank 5
    ** flash runs @ 1/3 bus clk
    ** sdram runs @ bus clk
    */
    STSYS_WriteRegDev32LE(STI5100_FMI_GEN_CFG,0x00000000);
    STSYS_WriteRegDev32LE(STI5100_FMI_FLASH_CLK_SEL,0x00000002);
    STSYS_WriteRegDev32LE(STI5100_FMI_CLK_ENABLE,0x00000001);
    /* Reset flash Banks */
    /*
    STSYS_WriteRegMem32LE(0x41400000,0x0);
    STSYS_WriteRegMem32LE(0x41400000,0x22222222);
    */
}

/*-------------------------------------------------------------------------
 * Function : sti5100_InterconnectPriority
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void sti5100_InterconnectPriority(void)
{
    STSYS_WriteRegDev32LE(STI5100_NHS2_INIT_0_PRIORITY,0x0000000F);
    STSYS_WriteRegDev32LE(STI5100_NHS2_INIT_1_PRIORITY,0x0000000F);

    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_0_PRIORITY,0x00000006);
    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_1_PRIORITY,0x00000006);
    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_2_PRIORITY,0x00000006);
    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_3_PRIORITY,0x00000006);
    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_4_PRIORITY,0x00000007);

    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_0_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_1_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_2_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_3_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHS3_INIT_4_LIMIT,0x00000001);

    STSYS_WriteRegDev32LE(STI5100_NHS4_INIT_0_PRIORITY,0x0000000F);
    STSYS_WriteRegDev32LE(STI5100_NHS4_INIT_1_PRIORITY,0x00000006);
    STSYS_WriteRegDev32LE(STI5100_NHS4_INIT_2_PRIORITY,0x00000005);
    STSYS_WriteRegDev32LE(STI5100_NHS4_INIT_3_PRIORITY,0x00000007);

    STSYS_WriteRegDev32LE(STI5100_NHS4_INIT_0_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHS4_INIT_1_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHS4_INIT_2_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHS4_INIT_3_LIMIT,0x00000001);

    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_0_PRIORITY,0x00000008);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_1_PRIORITY,0x00000002);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_2_PRIORITY,0x00000005);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_3_PRIORITY,0x00000003);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_4_PRIORITY,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_5_PRIORITY,0x00000004);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_6_PRIORITY,0x00000007);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_7_PRIORITY,0x00000008);

    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_0_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_1_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_2_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_3_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_4_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_5_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_6_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD1_INIT_7_LIMIT,0x00000001);

    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_0_PRIORITY,0x00000007);
    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_1_PRIORITY,0x00000006);
    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_2_PRIORITY,0x00000005);
    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_3_PRIORITY,0x00000004);
    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_4_PRIORITY,0x00000002);

    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_0_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_1_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_2_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_3_LIMIT,0x00000001);
    STSYS_WriteRegDev32LE(STI5100_NHD2_INIT_4_LIMIT,0x00000001);

    STSYS_WriteRegDev32LE(STI5100_CPU_FRAME,0x64);
    STSYS_WriteRegDev32LE(STI5100_CPU_LIMIT,0x1E);
}

/*-------------------------------------------------------------------------
* Function : MB390_ConfigureSysConf
* Input    : None
* Output   :
* Return   : None
* ----------------------------------------------------------------------*/
static void MB390_ConfigureSysConf(void)
{
    volatile U32    ctrl_d;

    /* Take lmiss+lmpl out from reset */
    /* poke -d (STI5100_system_config11) 0x000000C0 */

    /* Program COC register Pad strength / Impedance */
    /* poke -d (STI5100_LMI_COC) 0x00000000 */

    /* Program DLL register */
    /* poke -d (STI5100_system_config13) 0x00000000 */

    /* Power up video DACs */
    ctrl_d = STSYS_ReadRegDev32LE(STI5100_CONFIG_CONTROL_D);
    ctrl_d = (ctrl_d) | 0x18000;
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_D,ctrl_d);
}

/*-------------------------------------------------------------------------
* Function : sti5100_set_default_config
* Input    : None
* Output   :
* Return   : None
* ----------------------------------------------------------------------*/
static void sti5100_set_default_config(void)
{
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_A,0x00000000);
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_B,0x00000000);
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_C,0x00000000);
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_D,0x00000000);
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_E,0x00000000);
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_F,0x00000000);
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_G,0x00000000);
    STSYS_WriteRegDev32LE(STI5100_CONFIG_CONTROL_H,0x00000000);

    /* ADSC ADAC cfg */
    STSYS_WriteRegDev32LE(0x20500800,0x00000000);

    /* PTI top level config */
    STSYS_WriteRegDev32LE(0x20E00058,0x00000000);
}

/* EOF --------------------------------------------------------------------- */
