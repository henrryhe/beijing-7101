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
#include "stcommon.h"           /* Clock information */
#include "stsys.h"              /* STAPI includes */
#include "bereg.h"         		/* Localy used registers */

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
Description : 5510 : Configure the PLL, the clock generators
Parameters  : Frequency
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stboot_ConfigurePLL(U32 Frequency)
{
    /* 5510, 5512 */
    /* Protect access to CKG_PLL and CKG_PCM which are also used by STAUD */
    interrupt_lock();

    /* Stop the PLL, and wait 1ms to be sure frequency has fallen down to zero */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PLL), CKG_PLL_PDM);

    /* Release protection of register access to CKG_PLL and CKG_PCM which are also used by STAUD */
    interrupt_unlock();

    /* task_delay must be outside the critical region interrupt_lock/interrupt_unlock */
    task_delay( ST_GetClocksPerSecond() / 1000);

    /* Select I/O clock: PCMCLK is generated internally */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_CFG),
                       CKG_CFG_PCM_INT | CKG_CFG_MEM_INT | CKG_CFG_AUX_INT);

    /* Set PLL parameters: external 27 MHz PIXCLK, N=2, PLL factor depending on Frequency.
    This action starts the PLL (from power down mode to active mode).
    Then wait for 10ms */
    /*    (0x50 | ((((Frequency * 4) / 27) - 7) & 0xF))*/
    /* Added by HG: + 2 for a better rounding of the value in hex */

    /* Protect access to CKG_PLL and CKG_PCM which are also used by STAUD */
    interrupt_lock();

    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PLL), (0x50 | (((((Frequency * 4) + 2) / 27000) - 7) & 0xF)));

    /* Release protection of register access to CKG_PLL and CKG_PCM which are also used by STAUD */
    interrupt_unlock();

    /* task_delay must be outside the critical region interrupt_lock/interrupt_unlock */
    task_delay( ST_GetClocksPerSecond() / 100);

    /* Set memory clock divider */
    /* Writing to highest address (last one) latches the new configuration */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_MCK3), 0x00);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_MCK2), 0x00);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_MCK1), 0x0F);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_MCK0), 0xFE);

    /* Initialize AUXclk with a default value : fvco/6 (why not ?) */
    /* Writing to highest address (last one) latches the new configuration */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_AUX3), 0x23);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_AUX2), 0x00);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_AUX1), 0x0F);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_AUX0), 0xFE);

    /* Protect access to CKG_PLL and CKG_PCM which are also used by STAUD */
    interrupt_lock();

    /* Initialize PCM clock with a dummy value (just to have clock running) */
    /* Writing to highest address (last one) latches the new configuration */
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PCM3), 0x32);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PCM2), 0x00);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PCM1), 0x0F);
    STSYS_WriteRegDev8((void *) (CKG_BASE_ADDRESS + CKG_PCM0), 0xFE);

    /* Release protection of register access to CKG_PLL and CKG_PCM which are also used by STAUD */
    interrupt_unlock();

    /* Enable all clocks. Note the new policy where CFG_CCF.PBO
     (Prevent Bitbuffer Overflow on CD FIFOs) is NOT set, because it
     causes problems with video PTS association. Injection from memory
     must be modified (or PBO turned on) to take care not to overflow
     the bit buffers */
    STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + CFG_CCF),
                       CFG_CCF_EAI | /* Enable Audio Interface       */
                       CFG_CCF_EOU | /* Enable Ovf/Udf errors        */
                       CFG_CCF_EC3 | /* Enable Clock 3 = clock1/4    */
                       CFG_CCF_EC2 | /* Enable Clock 2 = clock1/2    */
                       CFG_CCF_ECK | /* Enable Clock 1 = SDRAM clock */
                       CFG_CCF_EDI | /* Enable Display Interface     */
                       CFG_CCF_EVI   /* Enable Video Interface       */
                      );

    /* HG: ??? Video and Audio soft reset (see AN879) */

    /* Wait 1ms for PLL to stabilise */
    task_delay( ST_GetClocksPerSecond() / 1000);
}

/* End of pll.c */
