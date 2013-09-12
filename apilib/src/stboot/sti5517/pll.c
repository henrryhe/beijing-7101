/*******************************************************************************
File Name   : pll.c

Description : Pll initialization.

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                             Name
----               ------------                             ----
09 July 01         Created                                  AdlF

*******************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "stlite.h"             /* STLite includes */
#include "stdevice.h"           /* Device includes */
#include "stddefs.h"
#include "stcommon.h"           /* Clock information */
#include "stsys.h"              /* STAPI includes */
#include "bereg.h"         		/* Localy used registers */

/* Private types/constants ------------------------------------------------ */

#if defined(CPUCLOCK81)
#error CPUCLOCK81 feature not supported
#endif

#define CKG_CLOCK_SEL2          0x204
#define CKG_PLLFSDIV1           0x04
#define CKG_PLLFSDIV0_low       0x00
#define CKG_PLLFSDIV0_high      0x01
#define CLOCK_IN_27             27000000
#define MPX_CLOCK_IN            27000000

#define PLL_CLOCK4              0x50
#define PLL_CLOCK5              0x60
#define PLL_CLOCK6              0x70

#define DIVIDER_MODE 0x0f8


/* Private variables ------------------------------------------------------ */
/*static U16 DividerTable[23][6] = {
{8,0x0,0x000c,0x03,1,1},
{9,0x0,0x019c,0x08,0,1},
{10,0x0,0x001c,0x04,0,0},
{11,0x0,0x071c,0x0A,0,1},
{12,0x0,0x0038,0x05,1,1},
{13,0x0,0x1c78,0x0C,0,1},
{14,0x0,0x0078,0x06,0,0},
{15,0x0,0x7878,0x0E,0,1},
{16,0x0,0x00f0,0x07,1,1},
{17,0x1,0xe1f0,0x10,0,1},
{18,0x0,0x01f0,0x08,0,0},
{19,0x7,0xc1f0,0x12,0,1},
{20,0x0,0x03e0,0x09,1,1},
{22,0x0,0x07e0,0x0A,0,0},
{24,0x0,0x0fc0,0x0B,1,1},
{26,0x0,0x1fc0,0x0C,0,0},
{28,0x0,0x3f80,0x0D,1,1},
{30,0x0,0x7f80,0x0E,0,0},
{32,0x0,0xff00,0x0F,1,1},
{34,0x1,0xff00,0x10,0,0},
{36,0x3,0xfe00,0x11,1,1},
{38,0x7,0xfe00,0x12,0,0},
{40,0xf,0xfc00,0x13,1,1}};*/

static U16 DividerTable[14][4] = {
{4,0x0002,0,0x0071},
{6,0x0006,0,0x0012},
{8,0xcccc,0,0x0075},
{10,0x739c,0,0x0014},
{12,0x0e38,0,0x0071},
{14,0x3c78,0,0x0013},
{16,0xf0f0,0,0x0075},
{18,0xe1f0,0x0003,0x0017},
{20,0x83e0,0x000f,0x0079},
{22,0x07e0,0,0x0010},
{24,0x0fc0,0,0x0071},
{30,0x7f80,0,0x0014},
{32,0x00ff,0,0x0077},
{40,0x03ff,0,0x079}};


/* Private function prototypes -------------------------------------------- */
void stboot_ConfigurePLL(U32 Frequency);

/* Function definitions --------------------------------------------------- */
/*******************************************************************************
Name        : SetupDividers
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetupDividers(U32 ClockOffset, U32 Div)
{
    U32 i;
    
    /* Find the register settings in the table */
    for (i = 0;i < 14; i++)
    {
        if (DividerTable[i][0] == Div)
        {
            /* We found the value !!! */
            break;
        }
        if (DividerTable[i][0] > Div)
        {
            /* The value is not in the table. We take an aproximation */
            break;
        }
    }
    
    /* If the loop has finished without any 'break', the value wanted is bigger
       than any in the table. We take the biggest one. */
    if (i == 14)
        i = 13;
    
    *((U8 *)CKG_BASE_ADDRESS + ClockOffset) = DividerTable[i][1];
    *((U8 *)CKG_BASE_ADDRESS + ClockOffset + 4) = DividerTable[i][2];
    *((U8 *)CKG_BASE_ADDRESS + ClockOffset + 8) = DividerTable[i][3];
}



/*******************************************************************************
Name        : stboot_ConfigurePLL
Description : 5514 : Configure the PLL, the clock generators
Parameters  : Frequency
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stboot_ConfigurePLL(U32 Frequency)
{
    U8 N,M,P;
    U32 PLLFreq;
    U8 ClockSelect2;
    U32 Div;
    U32 ClkIn;
    
    /* We don't actually configure the PLL. We read the frequency of the PLL and 
       we setup the dividers so that we get the frequency we want */
    /* Get the input clock */
    ClockSelect2 = (*((U16 *)(CKG_BASE_ADDRESS + CKG_CLOCK_SEL2)));    
    if ((ClockSelect2 & 0x01) == 0x01)
    {
        /* PLL81FS input clock is mpxclock */
        ClkIn = MPX_CLOCK_IN;
    }
    else
    {
        /* PLL81FS input clock is clockin27 */
        ClkIn = CLOCK_IN_27;
    }

    /* Read the PLL frequency */
    M = *((U8 *)CKG_BASE_ADDRESS + CKG_PLLFSDIV0_low);
    N = *((U8 *)CKG_BASE_ADDRESS + CKG_PLLFSDIV0_high);
    P = (*((U8 *)CKG_BASE_ADDRESS + CKG_PLLFSDIV1)) & 0x07;
    PLLFreq = ((2*((U32)N)*(ClkIn/1000))/(((U32)M) << P))*1000;

    /* The divider we want is : */
    Div = PLLFreq / (Frequency*1000/2);  /* This is actually the divider x 2 */

    /* Start programming registers */
    /* Move to x1 mode */
    *((U8 *)CKG_BASE_ADDRESS + DIVIDER_MODE) = 0x00;
    
    /* Program the three dividers. pll_clock[4] should be the frequency of
       the SDRAM, pll_clock[5] should be half of pll_clock[4] and 
       pll_clock[6] should be half of pll_clock[5] */

    SetupDividers(PLL_CLOCK4, Div);
    Div = Div * 2;
    SetupDividers(PLL_CLOCK5, Div);
    Div = Div * 2;
    SetupDividers(PLL_CLOCK6, Div);
    
    
    /* Move to programmable mode. This updates the clocks */
    *((U8 *)CKG_BASE_ADDRESS + DIVIDER_MODE) = 0x02;    
}
/* End of pll.c */
