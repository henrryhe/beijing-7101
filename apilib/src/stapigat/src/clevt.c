/*******************************************************************************

File name   : clevt.c

Description : EVT configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
09 Apr 2002        Created                                           HSdLM
17 May 2002        Add EVTInitParams.MemorySizeFlag                  HSdLM
12 Jun 2002        Manage TermParams.ForceTerminate                  HSdLM
20 Jun 2002        Add MemoryPoolSize init parameter setting
 *                 and allow ...MaxNum choice.                       HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#if defined USE_EVT || defined USE_POLL
#include "stddefs.h"
#include "stevt.h"
#include "clevt.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#ifndef EVT_EVENT_MAX_NUM
#define EVT_EVENT_MAX_NUM 50
#endif
#ifndef EVT_CONNECT_MAX_NUM
#define EVT_CONNECT_MAX_NUM 50
#endif
#ifndef EVT_SUBSCR_MAX_NUM
#define EVT_SUBSCR_MAX_NUM 50
#endif

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

extern ST_Partition_t *DriverPartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : EVT_Init
Description : Initialize the EVT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL EVT_Init()
{
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;
    STEVT_InitParams_t EVTInitParams;
    BOOL               RetOk = TRUE;

    /* Initialize the Event Handle */
    EVTInitParams.EventMaxNum       = EVT_EVENT_MAX_NUM;
    EVTInitParams.ConnectMaxNum     = EVT_CONNECT_MAX_NUM;
    EVTInitParams.SubscrMaxNum      = EVT_SUBSCR_MAX_NUM;
    EVTInitParams.MemoryPartition   = DriverPartition_p;
    EVTInitParams.MemorySizeFlag    = STEVT_UNKNOWN_SIZE;
    EVTInitParams.MemoryPoolSize    = 0;

    ErrorCode = STEVT_Init(STEVT_DEVICE_NAME,&EVTInitParams);
    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STEVT initialized, \trevision=%s\n",STEVT_GetRevision());
    }
    else
    {
        printf("STEVT_Init() failed: error=0x%0x !\n", ErrorCode);
        RetOk = FALSE;
    }
    return(RetOk);
} /* end of EVT_Init() */

/*******************************************************************************
Name        : EVT_Term
Description : Initialise the STEVT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void EVT_Term(void)
{
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;
    STEVT_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    ErrorCode = STEVT_Term( STEVT_DEVICE_NAME, &TermParams);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
        printf("STEVT_Term(%s): Warning !! Still open handle\n", STEVT_DEVICE_NAME);
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STEVT_Term(STEVT_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STEVT_Term(%s): ok\n", STEVT_DEVICE_NAME);
    }
    else
    {
        printf("STEVT_Term(%s): error=0x%0x !\n", STEVT_DEVICE_NAME, ErrorCode);
    }
} /* end EVT_Term() */

#endif /* USE_EVT | USE_POLL */
/* End of clevt.c */
