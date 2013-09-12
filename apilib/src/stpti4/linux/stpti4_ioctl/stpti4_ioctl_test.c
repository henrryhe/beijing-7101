/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_ioctl
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
 *  Description : This file contains code necessary for the test harness.
 *
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define STTBX_PRINT

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>            /* File operations (fops) defines */
#include <linux/ioport.h>        /* Memory/device locking macros   */

#include <linux/errno.h>         /* Defines standard error codes */

#include <asm/uaccess.h>         /* User space access routines */

#include <linux/sched.h>         /* User privilages (capabilities) */

#include "stpti4_ioctl_types.h"    /* Module specific types */

#include "stpti4_ioctl.h"          /* Defines ioctl numbers */


#include "stpti.h"	 /* A STAPI ioctl driver. */

#include "stpti4_ioctl_cfg.h"  /* PTI specific configurations. */

#include "pti_linux.h" /* Internal Linux event fuctions */

/*** PROTOTYPES **************************************************************/


#include "stpti4_ioctl_fops.h"


/* Imports */
extern PTI_Device_Info PTI_Devices[STPTI_CORE_COUNT];

/*** METHODS *****************************************************************/

/*=============================================================================

   stpti4_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'stpti4_ioctl.h'.

  ===========================================================================*/

void STPTI_TEST_ResetAllPti()
{
    /* Do nothing - This feature is no longer supported */
}


ST_ErrorCode_t STPTI_TEST_ForceLoader(int variant)
{
    int physical_pti;
    for(physical_pti=0;physical_pti<STPTI_CORE_COUNT;physical_pti++)
    {

        switch (variant)
        {

/* If the user specifies STPTI_MULTIPLE_LOADER_SUPPORT then they are able to switch between loaders at runtime.
   This is used by the STPTI test harness.  */
#if defined( STPTI_MULTIPLE_LOADER_SUPPORT )


#if defined( STPTI_DVB_SUPPORT )
            case 1:
#if defined( STPTI_ARCHITECTURE_PTI4 )
                PTI_Devices[physical_pti].TCLoader_p=STPTI_DVBTCLoader;
                PTI_Devices[physical_pti].LoaderName="STPTI_DVBTCLoader";
#else
#if defined( STPTI_ARCHITECTURE_PTI4_LITE )
                PTI_Devices[physical_pti].TCLoader_p=STPTI_DVBTCLoaderL;
                PTI_Devices[physical_pti].LoaderName="STPTI_DVBTCLoaderL";
#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1 )
                PTI_Devices[physical_pti].TCLoader_p=STPTI_DVBTCLoaderSL;
                PTI_Devices[physical_pti].LoaderName="STPTI_DVBTCLoaderSL";
#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2 ) && defined (STPTI_FRONTEND_HYBRID)
                PTI_Devices[physical_pti].TCLoader_p=STPTI_DVBTCLoaderSL2;
                PTI_Devices[physical_pti].LoaderName="STPTI_DVBTCLoaderSL2";
#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2 ) && defined (STPTI_FRONTEND_TSMERGER)
                PTI_Devices[physical_pti].TCLoader_p=STPTI_DVBTCLoaderSL3;
                PTI_Devices[physical_pti].LoaderName="STPTI_DVBTCLoaderSL3";
#endif
#endif
#endif
#endif
#endif
                STTBX_Print(("pticore: Forcing PTI%d loader to %s\n", physical_pti, PTI_Devices[physical_pti].LoaderName));
                break;
#endif

#if defined( STPTI_DVB_SUPPORT )
            case 2:
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
#endif


            /* leave cases 4 to 99 for major loader types such as above */
/*            case 4:*/



#if defined( STPTI_DVB_SUPPORT )
            case 6:
#if defined( STPTI_ARCHITECTURE_PTI4_LITE )
                PTI_Devices[physical_pti].TCLoader_p=STPTI_DVBTCLoaderLRL;
                PTI_Devices[physical_pti].LoaderName="STPTI_DVBTCLoaderLRL";
#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1 )
                PTI_Devices[physical_pti].TCLoader_p=STPTI_DVBTCLoaderSL;
                PTI_Devices[physical_pti].LoaderName="STPTI_DVBTCLoaderSL";
#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2 ) && defined (STPTI_FRONTEND_HYBRID)
                PTI_Devices[physical_pti].TCLoader_p=STPTI_DVBTCLoaderSL2;
                PTI_Devices[physical_pti].LoaderName="STPTI_DVBTCLoaderSL2";
#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2 ) && defined (STPTI_FRONTEND_TSMERGER)
                PTI_Devices[physical_pti].TCLoader_p=STPTI_DVBTCLoaderSL3;
                PTI_Devices[physical_pti].LoaderName="STPTI_DVBTCLoaderSL3";
#endif
#endif
#endif
#endif
                STTBX_Print(("pticore: Forcing PTI%d loader to %s\n", physical_pti, PTI_Devices[physical_pti].LoaderName));
                break;
#endif

            /* put special loaders here */
            case 100:

#endif  /* #if defined( STPTI_MULTIPLE_LOADER_SUPPORT ) */

            default:
                STTBX_Print(("pticore: Force loader %d not supported.\n", variant));
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);

        }
    }
    return(ST_NO_ERROR);
}
