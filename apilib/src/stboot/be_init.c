/*******************************************************************************

File name : be_init.c

Description : Back end specific code of STBOOT

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                     Name
----               ------------                     ----
26 July 99         Created                           HG
22 Sept 99         Remove SDRAM PLL config
                   Remove Audio external AC3 config  HG
15 Jan 2000        5510/12/05/08/18 management       FP
*******************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */

/* override makefile forcing of 70xx addresses; this file needs the frontend
  ones instead (INTERRUPT_...) */
#undef REMOVE_GENERIC_ADDRESSES

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stdevice.h"
#include "stsys.h"
#include "stboot.h"
#include "be_init.h"

#ifdef ST_OS20
#include "bereg.h"
#endif


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* 4629 register addresses*/
#ifdef ARCHITECTURE_ST20
#define STI4629_PLL_CFG        (STI4629_BASE_ADDRESS + 0x220)
#define STI4629_PLL_MDIV       (STI4629_BASE_ADDRESS + 0x221)
#define STI4629_PLL_NDIV       (STI4629_BASE_ADDRESS + 0x222)
#define STI4629_PLL_PDIV       (STI4629_BASE_ADDRESS + 0x223)
#define STI4629_PLL_SETUP_LOW  (STI4629_BASE_ADDRESS + 0x224)
#define STI4629_PLL_SETUP_HIGH (STI4629_BASE_ADDRESS + 0x225)
#define STI4629_PLL_NRST       (STI4629_BASE_ADDRESS + 0x226)
#endif

#ifdef ARCHITECTURE_ST40
#define STI4629_PLL_CFG        (STI4629_ST40_BASE_ADDRESS + 0x220) /*0xA2800220*/
#define STI4629_PLL_MDIV       (STI4629_ST40_BASE_ADDRESS + 0x221) /*0xA2800221*/
#define STI4629_PLL_NDIV       (STI4629_ST40_BASE_ADDRESS + 0x222) /*0xA2800222*/
#define STI4629_PLL_PDIV       (STI4629_ST40_BASE_ADDRESS + 0x223) /*0xA2800223*/
#define STI4629_PLL_SETUP_LOW  (STI4629_ST40_BASE_ADDRESS + 0x224) /*0xA2800224*/
#define STI4629_PLL_SETUP_HIGH (STI4629_ST40_BASE_ADDRESS + 0x225) /*0xA2800225*/
#define STI4629_PLL_NRST       (STI4629_ST40_BASE_ADDRESS + 0x226) /*0xA2800226*/
#endif

/* Private Variables (static)------------------------------------------------ */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

#if defined (ST_4629)
/*******************************************************************************
Name        : stboot_4629Init
Description : Initialisation of the 4629 PLL
Parameters  : 
Assumptions :
Limitations :
Returns     : ST_ERROR_CODE, i.e. success
*******************************************************************************/
ST_ErrorCode_t stboot_4629Init(void)
{
    /*BOOT_DebugPrintf(("stboot_4629Init called"));*/
    
    /* Turn the PLL off */
    STSYS_WriteRegDev8((void*)STI4629_PLL_CFG, 0x02);
    
    /* Write the setup params */
    /* M, N and P set to give 133Mhz which is divided by 2 to give 66Mhz for the MMDSP */
    /* M and N values are taken from 5528 data sheet for 266Mhz with P set to divide by 2*/
    STSYS_WriteRegDev8((void*)STI4629_PLL_MDIV, 0x1A);
    STSYS_WriteRegDev8((void*)STI4629_PLL_NDIV, 0x84);
    STSYS_WriteRegDev8((void*)STI4629_PLL_PDIV, 0x01);
    STSYS_WriteRegDev8((void*)STI4629_PLL_SETUP_LOW, 0xC8);
    STSYS_WriteRegDev8((void*)STI4629_PLL_SETUP_HIGH, 0x01);
    
    /* Turn the PLL on */
    STSYS_WriteRegDev8((void*)STI4629_PLL_CFG, 0x00);
    
    /* Enable the feedback loop */
    /*STSYS_WriteRegDev8((void*)STI4629_PLL_CFG, 0x01);*/

    /* Bring PLL out of reset*/
    STSYS_WriteRegDev8((void*)STI4629_PLL_NRST, 0x01);
  
    return(ST_NO_ERROR);
}
#endif

#ifndef mb295
/*******************************************************************************
Name        : stboot_BackEndInit
Description : Initialisation of the back end drivers
Parameters  : BackendType (not used)
Assumptions :
Limitations :
Returns     : ST_ERROR_CODE, i.e. success
*******************************************************************************/
ST_ErrorCode_t stboot_BackendInit(STBOOT_Backend_t *BackendType)
{
#if !defined (ST_5528) && !defined (ST_5100) && !defined (ST_5101) && !defined (ST_7710) && !defined(ST_5105) && !defined(ST_5700) && !defined(ST_7100) && !defined(ST_5301) && !defined(ST_8010) && !defined(ST_7109) && !defined(ST_5188) && !defined(ST_5525) && !defined(ST_5107) && !defined(ST_7200) /* new video/graphics system doesn't need these pokes */
    U8 TempReg;
    
  #if !defined (ST_5514) && !defined (ST_5516) && !defined(ST_5517)
    /* MPEG config register (done in cfg file for the excluded chips) */
    STSYS_WriteRegDev8((void *)MPEG_CONTROL_0, MPEG_CTRL0_RESET_VALUE);
    STSYS_WriteRegDev8((void *)MPEG_CONTROL_1, MPEG_CTRL1_REG_1_EXTRA);
  #endif

  #if defined(ST_5512) || defined(ST_5514) || defined(ST_5516) || defined(ST_5517) /* not 5510/08/18 */
    /* Manage the CPU priority in the VID_DIS register */
    STSYS_WriteRegDev8((void *) (VIDEO_BASE_ADDRESS + VID_DIS), VID_DIS_HLH);
  #endif

    /* Protect register read/re-write */
    interrupt_lock();

    /* Select planes order */
    TempReg = STSYS_ReadRegDev8((void *)(VIDEO_BASE_ADDRESS + VID_OUT));
    STSYS_WriteRegDev8((void *)(VIDEO_BASE_ADDRESS + VID_OUT),
                       (TempReg & (~VID_OUT_SPO) | VID_OUT_LAY));

    /* Clear CDR */
    TempReg = STSYS_ReadRegDev8((void *)(VIDEO_BASE_ADDRESS  + CFG_GCF));
    STSYS_WriteRegDev8((void *)(VIDEO_BASE_ADDRESS + CFG_GCF),
                       (TempReg & (~CFG_GCF_VID_INPUT)));

    /* Release register read/re-write protection */
    interrupt_unlock();
#endif /* 5528 5100 5101 7710 5105 5700 7100 5301 8010 7109 5188 5525 5107 7200 */

#if defined(ST_4629) && defined(ST_OS20)
    stboot_4629Init();
#endif
    return(ST_NO_ERROR);
}

#endif /* mb295 */
/* End of be_init.c */
