/*******************************************************************************
File Name   : sdram.c

Description : SDRAM configuration according to a given frequency.

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                             Name
----               ------------                             ----
15 Sept 99          Created                                  HG
20 Sept 99          Modified PLL setup for audio interface
                    to correct definitions                   ??
15 Oct 99           Suppressed external dependance for
                    registers definitions                    HG
15 Jan 2000         5510/12/05/08/18 management              FP
13 July 2001        5514 support                             AdlF
22 Feb  2002        5516 support                             SJR

Reference   :
    STi5505 MPEG/System Clocks and SDRAM Initialization, DVD-DOC-079 1.1
    STi3520A PLL Application Note, AN879/0996
*******************************************************************************/

/* Includes --------------------------------------------------------------- */

#include "stlite.h"             /* STLite includes */

#include "stdevice.h"           /* Device includes */
#include "stddefs.h"

#include "stsys.h"              /* STAPI includes */
#include "stcommon.h"	        /* Clock information */

#include "stboot.h"             /* Device_t type */
#include "sdram.h"              /* Local includes */
#include "bereg.h"              /* Localy used registers */
#include "macro.h"              /* Localy used registers */

/* Private types/constants ------------------------------------------------ */

#define TRYS_SDRAM_INIT         5

/* Private variables ------------------------------------------------------ */

/* Private function prototypes -------------------------------------------- */
static BOOL MpegSearchPhase(void);
static BOOL ConfigureSDRAM(U32 Frequency, STBOOT_DramMemorySize_t MemorySize);
static BOOL PLLAlreadyInitialized(void);

/* external function prototypes -------------------------------------------- */
extern void stboot_ConfigurePLL(U32 Frequency);

/* Function definitions --------------------------------------------------- */

/*******************************************************************************
Name        : stboot_InitSDRAM
Description : Init and Configure SDRAM
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if Init has failed. FALSE if init was successful.
*****************************************************************************/

BOOL stboot_InitSDRAM(U32 Frequency, STBOOT_DramMemorySize_t MemorySize)
{
    BOOL RetError = FALSE;
    U32 i = 0;

    if ( !PLLAlreadyInitialized() )
    {
#if !defined (ST_5510)
        /* Unlock the CFG_DRC, CFG_MFC, CFG_CCF registers  */
        STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + VID_LCK), VID_LCK_DIS);
#endif

        /* Try to init SDRAM a maximum of 5 times */
        do
        {
            stboot_ConfigurePLL(Frequency);
            RetError = ConfigureSDRAM(Frequency, MemorySize);
            i++;
        } while ((RetError) && (i < TRYS_SDRAM_INIT));
 

#if !defined (ST_5510)
        /* Lock the CFG_DRC, CFG_MFC, CFG_CCF registers  */
        STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + VID_LCK), VID_LCK_ENA);
#endif
    }

    return (RetError);
}


/*******************************************************************************
Name        : PLLAlreadyInitialized
Description : Check if the PLL has already been set by the CFG file.
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if already initialized. FALSE if not.
*******************************************************************************/

static BOOL PLLAlreadyInitialized(void)
{
    U8 tmp;
    BOOL RetError;

    /* Test if the PLL has already has been programmed */
    RetError = FALSE;

    tmp = STSYS_ReadRegDev8( (void *)(VIDEO_BASE_ADDRESS + CFG_DRC) );

    if ( (tmp & 0x0f) != 0x00 )
    {
        RetError = TRUE;
    }

    return (RetError);
}

/*******************************************************************************
Name        : MpegSearchPhase
Description : Search MPEG SDRAM clock correct phase
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if finds a phase that works, TRUE otherwise
*******************************************************************************/

static BOOL MpegSearchPhase(void)
{
    U8 DramConfigRegister;
    U8 Phase;
    volatile U32 *SdramAddress_p = (U32 *) SDRAM_BASE_ADDRESS;
    BOOL RetError = TRUE;

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    return FALSE;
#endif

    DramConfigRegister = STSYS_ReadRegDev8( (void *)(VIDEO_BASE_ADDRESS + CFG_DRC) );

    /* The following procedure must be followed:
    - write a 32-bit word to SDRAM
    - read back the word from SDRAM
    - if not read back correctly, increment the value of phase until the read is correct.*/
    for (Phase = 0 ; Phase < 4; Phase++)
    {
        /* Set phase */
        DramConfigRegister = (DramConfigRegister & 0xF3) | (Phase << 2);
        STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + CFG_DRC), DramConfigRegister);
        /* Write a word into SDRAM */
        *SdramAddress_p = 0x12345678;
        /* Read the byte from SDRAM */
        if (*SdramAddress_p == 0x12345678)
        {
            /* Success: Phase was found ! */
            RetError = FALSE;
            break;
        }
    }

    return(RetError);
}

/*****************************************************************************
Name        : ConfigureSDRAM
Description : Sequence init, refresh programmation, Phase locking
Parameters  : Frequency
Assumptions :
Limitations :
Returns     : FALSE if SDRAM test is OK, TRUE otherwise
*****************************************************************************/

static BOOL ConfigureSDRAM(U32 Frequency, STBOOT_DramMemorySize_t MemorySize)
{
    BOOL RetError;
    U8 RefreshTime;

    /* Reset refresh time */
    STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + CFG_MCF), 0);

    /* Reset SDRAM first ! */
    STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + CFG_DRC), 0);

    /* Prepare SDR */
    STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + CFG_DRC), CFG_DRC_SDR);

    /* Action of writing MRS launches the initialisation sequence */
    STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + CFG_DRC), CFG_DRC_SDR | CFG_DRC_MRS);

    /* Now enable the requests to the LMC */
    STSYS_WriteRegDev8 ((void *) (VIDEO_BASE_ADDRESS + CFG_DRC),
                        CFG_DRC_SDR | CFG_DRC_MRS | CFG_DRC_ERQ );

#if !defined (ST_5510) /* SDRAM > 32MBit not supported on STi5510 */
    if (MemorySize == STBOOT_DRAM_MEMORY_SIZE_64_MBIT)
    {
        /* Shared SDRAM is 64Mbits */
        STSYS_WriteRegDev8 ((void *) (VIDEO_BASE_ADDRESS + CFG_DRC),
                            CFG_DRC_SDR | CFG_DRC_MRS | CFG_DRC_ERQ | CFG_DRC_M64);
    }
    /* NOTE: With STi5514 STi5516 SMI can be up to 128Mbits in size */
    /* Currently this routine is not used to configure 5514/16 SMI
       so no extra configuration of the CFG_DRC is required */
#endif

    /* Memory refresh */
    REFRESH(RefreshTime,Frequency);
    STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + CFG_MCF), RefreshTime);

    /* Wait for at least 2 refresh cycle (RefreshTime is in units of 32 SDRAM clock periods) */
    task_delay( ST_GetClocksPerSecond() / 20);

    RetError = MpegSearchPhase();

    return(RetError);
}

/* End of sdram.c */
