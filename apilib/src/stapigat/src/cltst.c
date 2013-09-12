/*******************************************************************************

File name   : cltst.c

Description : Testtool configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
09 Apr 2002        Created                                          HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "stddefs.h"
#include "testtool.h"
#include "testcfg.h"

#if defined(USE_TESTTOOL) || defined(TESTTOOL_TESTS)
/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

extern ST_Partition_t *DriverPartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
BOOL TST_Init(const char * DefaultScript);
void TST_Term(void);
/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : TST_Init
Description : Initialise the Testtool driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL TST_Init(const char * DefaultScript)
{
    BOOL RetErr = FALSE;
    STTST_InitParams_t InitParams;

    InitParams.CPUPartition_p = DriverPartition_p;
#ifndef TESTTOOL_MAX_SYMBOLS
#define TESTTOOL_MAX_SYMBOLS 1000
#endif
    InitParams.NbMaxOfSymbols = TESTTOOL_MAX_SYMBOLS;

#ifndef TESTTOOL_INPUT_FILE_NAME
#define TESTTOOL_INPUT_FILE_NAME ""
#endif

    if (DefaultScript != NULL)
    {
        strcpy( InitParams.InputFileName, DefaultScript);
    }
    else
    {
        strcpy( InitParams.InputFileName, TESTTOOL_INPUT_FILE_NAME);
    }
    RetErr = STTST_Init(&InitParams);
    if (!RetErr )
    {
        printf("STTST initialized, \trevision=%s\n",STTST_GetRevision());
    }
    else
    {
        printf("STTST_Init() failed !\n");
    }
    return(!RetErr);
} /* end of TST_Init() */

/*******************************************************************************
Name        : TST_Term
Description : Terminate Testtool driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void TST_Term(void)
{
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;

    ErrCode = STTST_Term();
    if (ErrCode == ST_NO_ERROR)
    {
        printf("STTST_Term(): ok\n");
    }
    else
    {
        printf("STTST_Term(): error !\n");
    }
} /* end TST_Term() */

#endif /* USE_TESTTOOL or TESTTOOL_TESTS*/
/* End of cltst.c */
