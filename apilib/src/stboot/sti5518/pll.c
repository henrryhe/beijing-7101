/*******************************************************************************
File Name   : pll.c

Description : Pll initialization.

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                             Name
----               ------------                             ----
03 Feb 99          Created                                  FP

*******************************************************************************/

/* Includes --------------------------------------------------------------- */

#include "stlite.h"             /* STLite includes */
#include "stdevice.h"           /* Device includes */
#include "stddefs.h"
#include "stsys.h"              /* STAPI includes */
#include "bereg.h"              /* Localy used registers */
#include "stcommon.h"               /* Localy used registers */

/* Private types/constants ------------------------------------------------ */

/* Private variables ------------------------------------------------------ */

/* Private function prototypes -------------------------------------------- */
void stboot_ConfigurePLL(U32 Frequency);

/* Function definitions --------------------------------------------------- */

/*******************************************************************************
Name        : stboot_ConfigurePLL
Description : 5508 : Configure the PLL, the clock generators, the frequency
synthesizer and a control register
Parameters  : Frequency
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stboot_ConfigurePLL(U32 Frequency)
{
    U8 Reg;

    Frequency = Frequency;  /* to avoid warning message */

    /* ---------------------------------------------- */
    /* PLL initialization                             */
    /* ---------------------------------------------- */

    /* The PLL is initialised to 486 MHz */

    /* First write Pfactor = 2 */
    Reg = STSYS_ReadRegDev8( (void *)(CKG_BASE_ADDRESS + CKG_PLL1) );
    Reg = (Reg & 0x1F) | (2 << 5) ;
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PLL1), Reg );

    /* Mfactor = 14 = 0x0E */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PLL3), 0x0E );

    /* Nfactor = 126 = 0x7E */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PLL2), 0x7E );

    /* Important workaround : after a write to CKG_PREDIVPLL and CKG_FBKDIVPLL,
    the content of  with the values 0xa0 and 0x20 respectively,
    so ST20 clock = Fpll / 4 . Before modifying P value, we must bypass the ST20 clock (=27 Mhz)
    because with P=0, we can have Fpll > 400 Mhz, so ST20 clock > 100 MHz during a short time */

    /* Wait 5 milliseconds that CKG_CCST20 and CKG_DIVST20 are re-initialised */
    task_delay((clock_t)(ST_GetClocksPerSecond()*0.005));

    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_ST20_FEI_LINK1),
                       CKG_XXX_ENA);

    /* Then write Pfactor = 0 */
    Reg = STSYS_ReadRegDev8( (void *)(CKG_BASE_ADDRESS + CKG_PLL1) );
    Reg = Reg & 0x1F;
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PLL1), Reg );

    /* ---------------------------------------------- */
    /* Initialize the CPU clock generator             */
    /* ---------------------------------------------- */

#if defined(CPUCLOCK81)

    /* Initialize the CPU clock generator to 81Mhz */
    /* FCPU = Fpll / (2*NUM) ; FCPU = 81MHz, Fpll=486MHz => NUM = 3 */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_ST20_FEI_LINK0),
                       (3 << 4));

    /* CPU clock generator not bypassed, output not divided by 2, divider
     * enabled.
     */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_ST20_FEI_LINK1),
                       (CKG_XXX_ENA | CKG_XXX_NOTBYP) );
#else

    /* Initialize the CPU clock generator to 60.75Mhz */

    /* FCPU = Fpll / (2*NUM) ; FCPU = 60.75MHz, Fpll=483MHz => NUM = 4 */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_ST20_FEI_LINK0),
                       (4 << 4));
    /* CPU clock generator not bypassed, output not divided by 2, divider enabled */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_ST20_FEI_LINK1),
                       (CKG_XXX_ENA | CKG_XXX_NOTBYP) );
#endif

    /* ------------------------------------------------- */
    /* Initialize the denc clock generator to 27MHz      */
    /* ------------------------------------------------- */

    /* By default, after a hard reset, the denc clock generator is initialised to 27 Mhz */
    /* => It is not usefull to initialise the denc clock controller register */

    /* ------------------------------------------------- */
    /* Initialize the memory clock generator to 121.5MHz */
    /* ------------------------------------------------- */

    /* Fmemory = Fpll / (2*NUM) ; Fmemory = 121.5MHz, Fpll=486MHz => NUM = 2*/
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_MCK0), (2 << 4) );
    /* Memory clock generator not bypassed, output not divided by 2, divider enabled */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_MCK1), (CKG_XXX_ENA | CKG_XXX_NOTBYP));

    /* ------------------------------------------------ */
    /* Initialize the audio clock generator to 60.75MHz */
    /* ------------------------------------------------ */

    /* Faudio = Fpll / (2*NUM) ; Faudio = 60.75MHz, Fpll=486MHz but Fpll is divided by two
     * before the audio cell (243) so we just need to divide this last by 4 => NUM = 2*/
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_AUD0), (2 << 4) );
    /* Audio clock generator not bypassed, output not divided by 2, divider enabled */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_AUD1), (CKG_XXX_ENA | CKG_XXX_NOTBYP));

    /* ---------------------------------------------- */
    /* Frequency synthesizer initialization           */
    /* ---------------------------------------------- */

    /* By default, after a hard reset, the frequency synthesizer is managed by the MMDSP */
    /* => It is not usefull to initialise the frequency synthesizer registers */
    /*                                                                                 */
    /* WARNING : A WORKAROUND IS NEEDED DUE TO A HARDWARE BUG                          */
    /*                                                                                 */
    /* While this warning will exist, the two lines of comment above shoud not         */
    /* be taken into account                                                           */
    /* Due to a bug, the MMDSP can not managed automatically the frequency synthesizer */
    /* so we have to initialize the frequency synthesizer to be controlled by the ST20 */
    STSYS_WriteRegDev8((void *) (FSY_BASE_ADDRESS + FSY_SELREG),
                       NOT_MMDSP_CONTROL);

    /* ---------------------------------------------- */
    /* Control Register initialization                */
    /* ---------------------------------------------- */
    /* Chip configuration. Note the new policy where CFG_CCF.PBO
     (Prevent Bitbuffer Overflow on CD FIFOs) is NOT set, because it
     causes problems with video PTS association. Injection from memory
     must be modified (or PBO turned on) to take care not to overflow
     the bit buffers */
    STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + CFG_CCF),
                       CFG_CCF_EOU | /* Enable Ovf/Udf errors        */
                       CFG_CCF_EC3 | /* Enable Clock 3 = clock1/4    */
                       CFG_CCF_EC2 | /* Enable Clock 2 = clock1/2    */
                       CFG_CCF_ECK | /* Enable Clock 1 = SDRAM clock */
                       CFG_CCF_EDI | /* Enable Display Interface     */
                       CFG_CCF_EVI   /* Enable Video Interface       */
                      );
}

/* End of pll.c */
