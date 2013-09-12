/*******************************************************************************

File name   : cltsmux.c

Description : TSMUX configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
16 Jul 2002        Created                                           FQ
13 May 2003        Add db573 (7020 STEM) support                     CL
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>

#include "testcfg.h"
#ifdef USE_TSMUX
#include "stdevice.h"
#include "stddefs.h"
#include "stcommon.h"

#include "sttsmux.h"
#include "cltsmux.h"

#include "sttbx.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#if defined (ST_5514)
#define CLTSMUX_BASE_ADDRESS          ST5514_TSMUX_BASE_ADDRESS
#define CLTSMUX_DEVICE_TYPE           STTSMUX_DEVICE_ST5514;
#elif defined (ST_5517)
#define CLTSMUX_BASE_ADDRESS          ST5517_TSMUX_BASE_ADDRESS
#define CLTSMUX_DEVICE_TYPE           STTSMUX_DEVICE_ST5517;
#endif /* ST_5514 */

/* Private Variables (static)------------------------------------------------ */
static STTSMUX_Handle_t TSMUXHandle;

/* Global Variables --------------------------------------------------------- */

extern ST_Partition_t *DriverPartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


#if defined(mb290) || defined(mb314)
/*******************************************************************************
Name        : TSMUX_InitPTI_5514
Description : Initialize the TSMUX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL TSMUX_InitPTI_5514(void)
{
    BOOL                    RetOk;
    ST_ErrorCode_t          ErrCode;
    STTSMUX_InitParams_t    TSMUXInitParams;
    STTSMUX_OpenParams_t    TSMUXOpenParams;
    ST_ClockInfo_t          SysClockInfo;
    STTSMUX_Object_t        Source;
    STTSMUX_Object_t        Destination;
    STTSMUX_SWTSInterface_t Interface;

    RetOk=TRUE;
    ErrCode = ST_NO_ERROR;
    ST_GetClockInfo(&SysClockInfo);
    TSMUXInitParams.DeviceType = CLTSMUX_DEVICE_TYPE
    TSMUXInitParams.DriverPartition_p = DriverPartition_p;
    TSMUXInitParams.BaseAddress_p = (U32*)CLTSMUX_BASE_ADDRESS;
    TSMUXInitParams.ClockFrequency = SysClockInfo.STBus;
    TSMUXInitParams.MaxHandles = 1;

    /* Horrible patch to handle hidden MB314 register setting */
    /* This activate the STEM (board DB499) by default        */
#if defined(mb290)
    /* Setting : 0x1=enabled, 0x0=disabled                    */
    *((volatile unsigned char *) 0x7e0c0000) = 0x1;
#elif defined(mb314)
    /* Setting : 0x0=enabled, 0x1=disabled                    */
    *((volatile unsigned char *) 0x7e0c0000) = 0x0;
#else
#error Not defined board for injection
#endif /*mb290 */
    ErrCode = STTSMUX_Init(STTSMUX_DEVICE_NAME, &TSMUXInitParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STTSMUX_Init(): Failed. Err = 0x%x\n", ErrCode));
    }
    else
    {
        memset((void *)&TSMUXOpenParams, 0, sizeof(STTSMUX_OpenParams_t));
        ErrCode = STTSMUX_Open(STTSMUX_DEVICE_NAME, &TSMUXOpenParams, &TSMUXHandle);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STTSMUX_Open(): Failed. Err = 0x%x\n", ErrCode));
        }
        else
        {
            ErrCode = STTSMUX_SetTSMode(TSMUXHandle, STTSMUX_TSIN_0, STTSMUX_TSMODE_SERIAL);
            if (ErrCode == ST_NO_ERROR)
                ErrCode = STTSMUX_SetTSMode(TSMUXHandle, STTSMUX_TSIN_1, STTSMUX_TSMODE_PARALLEL);
            if (ErrCode == ST_NO_ERROR)
                ErrCode = STTSMUX_SetTSMode(TSMUXHandle, STTSMUX_TSIN_2, STTSMUX_TSMODE_PARALLEL);
            if (ErrCode == ST_NO_ERROR)
                ErrCode = STTSMUX_SetSyncMode(TSMUXHandle, STTSMUX_TSIN_0, STTSMUX_SYNCMODE_SYNCHRONOUS);
            if (ErrCode == ST_NO_ERROR)
                ErrCode = STTSMUX_SetSyncMode(TSMUXHandle, STTSMUX_TSIN_1, STTSMUX_SYNCMODE_SYNCHRONOUS);
            if (ErrCode == ST_NO_ERROR)
                ErrCode = STTSMUX_SetSyncMode(TSMUXHandle, STTSMUX_TSIN_2, STTSMUX_SYNCMODE_SYNCHRONOUS);

            if (ErrCode == ST_NO_ERROR)
                ErrCode = STTSMUX_GetSWTSInterface(TSMUXHandle, STTSMUX_SWTS_0, &Interface);
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("STTSMUX_SetSyncMode(): Failed. Error 0x%x\n", ErrCode));
            }
            if (ErrCode == ST_NO_ERROR)
            {
                ErrCode = STTSMUX_SetStreamRate(TSMUXHandle, STTSMUX_SWTS_0, 150 * 1000000 /* Interface.MaxStreamRate */);
                if (ErrCode != ST_NO_ERROR)
                {
                    STTBX_Print(("STTSMUX_SetStreamRate(): Failed. Error %d\n", ErrCode));
                }
            }

            /* TSIN 2 input goes to PTI_A, PTI_B & PTI_C */
            if (ErrCode == ST_NO_ERROR)
            {
                Source = STTSMUX_TSIN_2;
                for(Destination=STTSMUX_PTI_A; Destination<STTSMUX_PTI_C; Destination++)
                {
                    ErrCode = STTSMUX_Connect(TSMUXHandle, Source, Destination);
                    if( ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Print(("STTSMUX_Connect(%d,%d): Failed. Error 0x%x\n",ErrCode,
                                     Source, Destination));
                    }
                }
            }
        }
    }

    if(ErrCode != ST_NO_ERROR)
    {
        RetOk = FALSE;
        STTBX_Print(("TSMUX_Init() failed !! Error = %d", ErrCode));
    }
    else
    {
        STTBX_Print(("STTSMUX initialized,\trevision=%-21.21s\n", STTSMUX_GetRevision()));
    }
    return (RetOk);
} /* end of TSMUX_InitPTI_5514() */
#endif /* #if defined(mb290) || defined(mb314) */


#if defined (ST_7020) && defined(ST_5517)
/*******************************************************************************
Name        : TSMUX_InitPTI_5517_for_STEM7020
Description : Initialize the STTSMUX driver. Only necessary when using STEM
              board (db573) because of injection problem with db499 STEM card.
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL TSMUX_InitPTI_5517_for_STEM7020(void)
{
    ST_ErrorCode_t        ErrorCode;
    STTSMUX_InitParams_t  TSMUXInitParams;
    STTSMUX_OpenParams_t  TSMUXOpenParams;
    ST_ClockInfo_t        SysClockInfo;
    BOOL                  RetOk;

    RetOk = TRUE;
    TSMUXInitParams.DeviceType = CLTSMUX_DEVICE_TYPE;
    TSMUXInitParams.DriverPartition_p = DriverPartition_p;
    TSMUXInitParams.BaseAddress_p = (U32*)CLTSMUX_BASE_ADDRESS;
    ST_GetClockInfo(&SysClockInfo);
    TSMUXInitParams.ClockFrequency = SysClockInfo.STBus;
    TSMUXInitParams.MaxHandles = 1;
    ErrorCode = STTSMUX_Init(STTSMUX_DEVICE_NAME,&TSMUXInitParams);
    if(ErrorCode != ST_NO_ERROR)
    {
        RetOk = FALSE;
        STTBX_Print(("STTSMUX_Init(): Failed. Err = 0x%x\n", ErrorCode));
    }
    else
    {
        /* Open tsmux */
        ErrorCode = STTSMUX_Open(STTSMUX_DEVICE_NAME,&TSMUXOpenParams,&TSMUXHandle);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Print(("STTSMUX_Open(): Failed. Err = 0x%x\n", ErrorCode));
        }
        else
        {
            /* Setup TSIN1 for stream */
            ErrorCode = STTSMUX_SetTSMode(TSMUXHandle, STTSMUX_TSIN_1, STTSMUX_TSMODE_PARALLEL);
            if(ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STTSMUX_SetTSMode(): Failed. Error %x\n", ErrorCode));
            }
            else
            {
                /* Setup sync mode */
                ErrorCode = STTSMUX_SetSyncMode(TSMUXHandle, STTSMUX_TSIN_1, STTSMUX_SYNCMODE_SYNCHRONOUS);
                if(ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Print(("STTSMUX_SetSyncMode(): Failed. Error %x\n", ErrorCode));
                }
                else
                {
                    /* Connect TSIN to PTI */
                    ErrorCode = STTSMUX_Connect(TSMUXHandle,STTSMUX_TSIN_1, STTSMUX_PTI_A);
                    if(ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Print(("STTSMUX_Connect(%d,%d): Failed. Error 0x%x\n",ErrorCode,
                                     STTSMUX_TSIN_1, STTSMUX_PTI_A));
                    }
                }
            }
        }

        if(ErrorCode != ST_NO_ERROR)
        {
            RetOk = FALSE;
            STTBX_Print(("TSMUX_Init() failed !! Error = %d", ErrorCode));
        }
        else
        {
            STTBX_Print(("STTSMUX initialized using TSIN1 on PTI A,\trevision=%-21.21s\n", STTSMUX_GetRevision()));
        }
    }

    return (RetOk);
} /* End of TSMUX_InitPTI_5517_for_STEM7020() function. */
#endif /* #if defined (ST_7020) && defined(ST_5517) */


/*******************************************************************************
Name        : TSMUX_Init
Description : Initialise the STTSMUX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
BOOL TSMUX_Init(void)
{
    BOOL RetOk;
#if defined(mb290) || defined(mb314)
    RetOk = TSMUX_InitPTI_5514();
#elif defined (ST_7020) && defined(mb382)
    RetOk = TSMUX_InitPTI_5517_for_STEM7020();
#else
#error Not defined board for injection
    RetOk = FALSE;
#endif

    return (RetOk);
} /* end TSMUX_Init() */

/*******************************************************************************
Name        : TSMUX_Term
Description : Terminate the STTSMUX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void TSMUX_Term(void)
{
    ST_ErrorCode_t ErrCode;
    STTSMUX_TermParams_t TSMUXTermParams;

    ErrCode = STTSMUX_Close(TSMUXHandle);
    if( ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STTSMUX_Close(): Failed. Error 0x%x\n",ErrCode));
    }

    TSMUXTermParams.ForceTerminate = FALSE;
    ErrCode = STTSMUX_Term(STTSMUX_DEVICE_NAME, &TSMUXTermParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STTSMUX_Term(): Failed Error = 0x%x\n", ErrCode));
        TSMUXTermParams.ForceTerminate = TRUE;
        ErrCode = STTSMUX_Term(STTSMUX_DEVICE_NAME, &TSMUXTermParams);
    }

    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Print(("STTSMUX_Term(%s): ok\n", STTSMUX_DEVICE_NAME));
    }
    else
    {
        STTBX_Print(("STTSMUX_Term(%s): error=0x%0x !\n", STTSMUX_DEVICE_NAME, ErrCode));
    }

} /* end TSMUX_Term() */

#endif /* USE_TSMUX */

/* End of cltsmux.c */
