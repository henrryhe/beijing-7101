/*******************************************************************************

File name   : clintmr.c

Description : STINTMR configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
16 Apr 2002        Created                                          HSdLM
02 Apr 2003        Add support for db573                            HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#ifdef USE_INTMR

#include "stdevice.h"

#include "stddefs.h"
#include "stintmr.h"
#include "clintmr.h"

/*#ifndef ST_OSLINUX*/
#include "sttbx.h"
/*#endif*/

#include "clevt.h"

#ifdef ST_GX1
#include <kbim/p_chIntr.h>     /* kbimST40_XXX symbols */
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#if defined (mb290)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7020
#define INTMR_BASE_ADDRESS            (STI7020_BASE_ADDRESS + ST7020_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST5514_EXTERNAL1_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         EXTERNAL_1_INTERRUPT_LEVEL

#elif defined (mb376) /* this means db573 7020 STEM board */

#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7020
#define INTMR_BASE_ADDRESS            (STI7020_BASE_ADDRESS + ST7020_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST5528_EXTERNAL2_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         3

#elif (defined (mb382) || defined (mb314)) /* this means db573 7020 STEM board */
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7020
#define INTMR_BASE_ADDRESS            (STI7020_BASE_ADDRESS + ST7020_CFG_OFFSET)
    #if defined(mb314)
    #define INTMR_INTERRUPT_NUMBER        ST5514_EXTERNAL0_INTERRUPT
    #else /* defined(mb382) */
    #define INTMR_INTERRUPT_NUMBER        ST5517_EXTERNAL0_INTERRUPT
    #endif
#define INTMR_INTERRUPT_LEVEL         EXTERNAL_0_INTERRUPT_LEVEL

#elif defined (ST_5528)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_5528
#define INTMR_BASE_ADDRESS            ST5528_VTG_BASE_ADDRESS
#define INTMR_INTERRUPT_NUMBER        ST5528_VTG_VSYNC_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         0

#elif defined (ST_7015)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7015
#define INTMR_BASE_ADDRESS            (STI7015_BASE_ADDRESS + ST7015_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST20TP3_EXTERNAL1_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         EXTERNAL_1_INTERRUPT_LEVEL

#elif defined (ST_7020)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7015
#define INTMR_BASE_ADDRESS            (STI7020_BASE_ADDRESS + ST7020_CFG_OFFSET)
#define INTMR_INTERRUPT_NUMBER        ST20TP3_EXTERNAL1_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         EXTERNAL_1_INTERRUPT_LEVEL

#elif defined (ST_GX1)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_GX1
#define INTMR_BASE_ADDRESS            (S32)(0xBE080000+0x28)      /* GX1_INTREQ08 */
#define INTMR_INTERRUPT_NUMBER        kbimST40_VideoSync
#define INTMR_INTERRUPT_LEVEL         0

#elif defined (ST_7100)
#define INTMR_DEVICE_TYPE             STINTMR_DEVICE_7100
#define INTMR_BASE_ADDRESS            ST7100_VTG_BASE_ADDRESS
#define INTMR_INTERRUPT_NUMBER        VTG_VSYNC_INTERRUPT
#define INTMR_INTERRUPT_LEVEL         0
#endif


/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

extern ST_Partition_t *SystemPartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : INTMR_Init
Description : Initialise STINTMR
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL INTMR_Init(void)
{
    BOOL RetOk = TRUE;
    ST_ErrorCode_t       ErrCode = ST_NO_ERROR;
    STINTMR_InitParams_t InitStIntMr;
#ifdef ST_GX1
    U32 i;
#endif /* #ifdef ST_GX1 */

    /* Initialise STINTMR */
    InitStIntMr.DeviceType          = INTMR_DEVICE_TYPE;
    InitStIntMr.Partition_p         = SystemPartition_p;
    InitStIntMr.BaseAddress_p       = (void *)INTMR_BASE_ADDRESS;
    InitStIntMr.InterruptNumbers[0] = INTMR_INTERRUPT_NUMBER;
    InitStIntMr.InterruptLevels[0]  = INTMR_INTERRUPT_LEVEL;
#if defined(ST_7015) || defined(ST_7020)
    /* The following lines are needed only for 7015 (backward compatibility reason) !! */
    InitStIntMr.InterruptNumber     = INTMR_INTERRUPT_NUMBER;
    InitStIntMr.InterruptLevel      = INTMR_INTERRUPT_LEVEL;
#endif /* ST_7015/20*/

    InitStIntMr.NumberOfInterrupts  = 1;
    InitStIntMr.NumberOfEvents      = 3;

    strcpy( InitStIntMr.EvtDeviceName, STEVT_DEVICE_NAME);

    if (STINTMR_Init(STINTMR_DEVICE_NAME, &InitStIntMr) != ST_NO_ERROR)
    {
        STTBX_Print(("STINTMR failed to initialise !\n"));
        RetOk = FALSE;
    }
    else
    {
        STTBX_Print(("STINTMR initialized, \trevision=%s", STINTMR_GetRevision() ));
        ErrCode = STINTMR_Enable(STINTMR_DEVICE_NAME);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print((", but failed to enable interrupts !\n"));
            RetOk = FALSE;
        }
        else
        {
            STTBX_Print((" and enabled.\n"));

            /* Setting default interrupt mask */
#if defined(ST_7015)
            /* Setting interrupt mask for VTG1  */
            ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, ST7015_VTG1_INT, 1<<ST7015_VTG1_INT );
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",ST7015_VTG1_INT,ErrCode));
            }
            /* Setting interrupt mask for VTG2  */
            ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, ST7015_VTG2_INT, 1<<ST7015_VTG2_INT );
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",ST7015_VTG2_INT,ErrCode));
            }
#elif defined(ST_7020)
            /* Setting interrupt mask for VTG1  */
            ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, ST7020_VTG1_INT, 1<<ST7020_VTG1_INT );
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",ST7020_VTG1_INT,ErrCode));
            }
            /* Setting interrupt mask for VTG2  */
            ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, ST7020_VTG2_INT, 1<<ST7020_VTG2_INT );
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",ST7020_VTG2_INT,ErrCode));
            }
#elif defined (ST_GX1)
            for(i=0;i<3;i++)
            {
                /* Setting interrupt mask for VTG=0, DVP1=1 & DVP2=2 */
                ErrCode = STINTMR_SetInterruptMask( STINTMR_DEVICE_NAME, i, (0x10000000)<<i);
                if(ErrCode != ST_NO_ERROR)
                {
                    STTBX_Print((" Cannot set interrupt mask %d ! error=0x%x.\n",i,ErrCode));
                }
            }
#endif /* ST_GX1 */
        }
    }

    return(RetOk);
} /* end INTMR_Init() */

/*******************************************************************************
Name        : INTMR_Term
Description : Terminate STINTMR
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void INTMR_Term(void)
{
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
    STINTMR_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    ErrorCode = STINTMR_Term(STINTMR_DEVICE_NAME, &TermParams);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("STINTMR_Term(%s): Warning !! Still open handle\n", STINTMR_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STINTMR_Term(STINTMR_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Print(("STINTMR_Term(%s): ok\n", STINTMR_DEVICE_NAME));
    }
    else
    {
        STTBX_Print(("STINTMR_Term(%s): error 0x%0x\n", STINTMR_DEVICE_NAME, ErrorCode));
    }
} /* end INTMR_Term() */

#endif /* USE_INTMR */
/* End of clintmr.c */
