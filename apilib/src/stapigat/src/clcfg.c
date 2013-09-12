/*******************************************************************************

File name   : clcfg.c

Description : CFG configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
22 May 2002        Created                                          HSdLM
11 Oct 2002        Add support for STi5517                          HSdLM
14 May 2003        Add support for STi5528                          HSdLM
16 Sep 2004        Add support for ST40/OS21                        MH
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#ifdef ST_OS20
#include <debug.h>
#endif /* ST_OS20 */
#ifdef ST_OS21
#include <os21debug.h>
#endif /* ST_OS21 */

#include "testcfg.h"


#ifdef USE_STCFG

#define USE_AS_FRONTEND

#include "genadd.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stcfg.h"
#include "clcfg.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#ifdef ST_5514
#define CLCFG_DEVICE_TYPE   STCFG_DEVICE_5514
#define CLCFG_BASE_ADDRESS  ST5514_CFG_BASE_ADDRESS
#define CLCFG_PTI_FIRST     STCFG_PTI_A
#define CLCFG_PTI_LAST      STCFG_PTI_C
#define PCR_CONNECT
#elif defined (ST_5528)
#define CLCFG_DEVICE_TYPE   STCFG_DEVICE_5528
#define CLCFG_BASE_ADDRESS  ST5528_CFG_BASE_ADDRESS
#define CLCFG_PTI_FIRST     STCFG_PTI_A
#define CLCFG_PTI_LAST      STCFG_PTI_C
#elif defined (ST_5516)
#define CLCFG_DEVICE_TYPE   STCFG_DEVICE_5516
#define CLCFG_BASE_ADDRESS  ST5516_CFG_BASE_ADDRESS
#define CLCFG_PTI_FIRST     STCFG_PTI_A
#define CLCFG_PTI_LAST      STCFG_PTI_A
#elif defined (ST_5517)
#define CLCFG_DEVICE_TYPE   STCFG_DEVICE_5517
#define CLCFG_BASE_ADDRESS  ST5517_CFG_BASE_ADDRESS
#define CLCFG_PTI_FIRST     STCFG_PTI_A
#define CLCFG_PTI_LAST      STCFG_PTI_A
#else
#error STCFG is only available for STi5514 or STi5528 or STi5516 or STi5517.
#endif

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

extern ST_Partition_t *DriverPartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : CFG_Init
Description : Initialise the CFG driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL CFG_Init(void)
{
    STCFG_InitParams_t  CfgInit;
    STCFG_Handle_t      CFGHandle;
    U8                  Index;
    char                Msg[150];
    BOOL                RetOk = TRUE;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    CfgInit.DeviceType    = CLCFG_DEVICE_TYPE;
    CfgInit.Partition_p   = DriverPartition_p;
    CfgInit.BaseAddress_p = (U32*)CLCFG_BASE_ADDRESS;
    if (CfgInit.BaseAddress_p == NULL)
    {
       debugmessage("***ERROR: Invalid device base address OR simulator memory allocation error\n\n");
       return(FALSE);
    }

    ErrorCode = STCFG_Init("cfg", &CfgInit);
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STCFG_Open("cfg", NULL, &CFGHandle);
        if (ErrorCode == ST_NO_ERROR)
        {
#if defined (ST_5528) && defined (ST_7020)
            /* Enable Exteranl interrupt (STi5528 and STEM 7020) */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                              STCFG_EXTINT_0, STCFG_EXTINT_INPUT);
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                              STCFG_EXTINT_1, STCFG_EXTINT_INPUT);
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                              STCFG_EXTINT_2, STCFG_EXTINT_INPUT);
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                              STCFG_EXTINT_3, STCFG_EXTINT_INPUT);

                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to enable Video DAC in STCFG \n");
                }
            }
#else /* else of (ST_5528) && defined (ST_7020) */

            /* CDREQ configuration for PTIs A, B and C */
            for (Index = CLCFG_PTI_FIRST; Index <= CLCFG_PTI_LAST; Index++)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_PTI_CDREQ_CONNECT, Index, STCFG_PTI_CDIN_0, STCFG_CDREQ_VIDEO_INT);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to set PTI CDREQ connect in STCFG \n" );
                    break;
                }
#ifdef PCR_CONNECT
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_PTI_PCR_CONNECT, Index, STCFG_PCR_27MHZ_A);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to set PTI PCR connect in STCFG \n");
                    break;
                }
#endif /* PCR_CONNECT */
            }
            /* Enable video DACs */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_VIDEO_ENABLE, STCFG_VIDEO_DAC_ENABLE);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to enable Video DAC in STCFG \n");
                }
            }

            /* Power down audio DACs */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_AUDIO_ENABLE, STCFG_AUDIO_DAC_POWER_OFF);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to power down audio DACs in STCFG \n");
                }
            }

            /* Enable PIO4<4> as IrDA Data not IRDA Control */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STCFG_CallCmd(CFGHandle, STCFG_CMD_PIO_ENABLE, STCFG_PIO4_BIT4_IRB_NOT_IRDA);
                if (ErrorCode != ST_NO_ERROR)
                {
                    debugmessage(" Unable to enable PIO4<4> as IrDA Data not IRDA Control in STCFG \n");
                }
            }
#endif /* not defined (ST_5528) && defined (ST_7020) */

            ErrorCode = STCFG_Close(CFGHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                debugmessage(" Unable to close opened STCFG handle \n");
            }

        }
        else
        {
           debugmessage(" Unable to open STCFG \n");
        }
    }
    else
    {
        debugmessage(" Unable to initialize STCFG \n");
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        sprintf(Msg,"STCFG initialized,\trevision=%-21.21s\n", STCFG_GetRevision());
        RetOk = TRUE;
    }
    else
    {
        sprintf(Msg,"Error: STCFG_Init() failed ! Error 0x%0x\n", ErrorCode);
        RetOk = FALSE;
    }
    debugmessage(Msg);

    return(RetOk);
} /* end CFG_Init() */

#endif /* USE_STCFG */

/* End of clcfg.c */
