/*******************************************************************************
File Name   : pll.c

Description : Pll initialization.

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                             Name
----               ------------                             ----
03 Feb 99          Created                                  FP
29/01/01           Modified for audio mmdsp  

*******************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "stlite.h"             /* STLite includes */
#include "stdevice.h"           /* Device includes */
#include "stddefs.h"
#include "stsys.h"              /* STAPI includes */
#include "bereg.h"         	/* Localy used registers */

/* Private types/constants ------------------------------------------------ */

#if defined(CPUCLOCK81)
#error CPUCLOCK81 feature not supported
#endif

/* Private variables ------------------------------------------------------ */

/* Private function prototypes -------------------------------------------- */
void stboot_ConfigurePLL(U32 Frequency);

/* Function definitions --------------------------------------------------- */

/*******************************************************************************
Name        : stboot_ConfigurePLL
Description : 5580 : Configure the PLL, the clock generators, the frequency
              synthesizer and a control register
Parameters  : Frequency
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stboot_ConfigurePLL(U32 Frequency)
{
	Frequency = Frequency;	/* to avoid warning message */
/*---------------------------------------------------
 * PLL initialization
 * --------------------------------------------------*/
    /* By default, after a hard reset, the PLL is initialised to 243MHz */
    /* => It is not useful to initialise the PLL register */

/*---------------------------------------------------
 * Clock generator initialization
 * --------------------------------------------------*/
    /* By default, after a hard reset, the clock generator configuration
     * register is well initialised :
     * - the pixel clock pin is an input
     * - the PWM2 pin is configured as a PWM2 pin output ( not as the alternate function VSYNC )
     * - the PWM0 pin is configured as a PWM0 pin output ( not as the alternate function HSYNC )
     * - Normal clock for the MMDSP
     * => It is not usefull to initialise the clock generator configuration register */
	/*STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_CFG), 0x38 );*/

    /* By default, after a hard reset, the ST20/FEI/LINK clock generator is initialised to 60.75 Mhz */
    /* => It is not usefull to initialise the ST20/FEI/LINK clock controller register */

    /* By default, after a hard reset, the denc clock generator is initialised
     * to 27 Mhz
     */
    /* => It is not usefull to initialise the denc clock controller register */

    /* Initialize the memory clock generator to 121.5MHz */
    /* Fmemory = Fpll / (2*NUM) ; Fmemory = 121.5MHz, Fpll=243MHz => NUM = 1*/
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_DIV_MCK), (0x01 << 4) );
    /* Memory clock generator not bypassed, output not divided by 2, divider enabled */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_CNT_MCK), (CKG_XXX_ENA | CKG_XXX_NOTBYP));

    /* Audio PCM clock generator not bypassed, output not divided by 2,
     * divider enabled
     */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_CNT_AUD),
                       (CKG_XXX_ENA | CKG_XXX_NOTBYP));
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_SFREQPCM_CNT), 0xe);
    
    /* Initialize the karaoke clock generator to 121.5MHz */

    /* Divide Karaoke clock by 2 to give 121.5 MHz */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_KARA2), 1 );
    /* Enable karaoke clock */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_KARA1), 0xA0 );
    
    /*---------------------------------------------------
     * Frequency synthesizer initialization
     * --------------------------------------------------*/

    /* By default, after a hard reset, the frequency synthesizer is managed
     * by the MMDSP....
     */

    /* => It is not usefull to initialise the frequency synthesizer registers */
    /* WARNING : A WORKAROUND IS NEEDED DUE TO A HARDWARE BUG: 
     * While this warning will exist, the two lines of comment above should
     * not be taken into account.
     * Due to a bug, the MMDSP can not managed automatically the frequency
     * synthesizer so we have to initialize the frequency synthesizer to be
     * controlled by the ST20.
     */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_CNT_IDDQPAD), 0);

    /* Setup audio cell frequency synthesizer to 60MHz
       These values are taken directly from the datasheet.
     */

    /* Disable clock */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_SFREQAUD_CNT), 1);

    /* Configure clock */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_SFREQAUD_SDIV), 0x01);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_SFREQAUD_MD),   0x1C);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_SFREQAUD_PE0),  0x99);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_SFREQAUD_PE1),  0x19);

    /* Enable clock */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_SFREQAUD_CNT), 0);
    

    /* Chip configuration */
    STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + CFG_CCF),
                       CFG_CCF_EOU | /* Enable Ovf/Udf errors        */
                       CFG_CCF_PBO | /* Prevent Bitbuffer Overflow   */
                       CFG_CCF_EC3 | /* Enable Clock 3 = clock1/4    */
                       CFG_CCF_EC2 | /* Enable Clock 2 = clock1/2    */
                       CFG_CCF_ECK | /* Enable Clock 1 = SDRAM clock */
                       CFG_CCF_EDI | /* Enable Display Interface     */
                       CFG_CCF_EVI   /* Enable Video Interface       */
                      );
}

/* End of pll.c */
