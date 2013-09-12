/******************************************************************************

    File Name   : ThreadTests.c

    Description : Top-level file to STEVT threading tests

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "ThreadTests.h"
#if !defined(ST_OSLINUX)
#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif
#endif
#include "evt_test.h"

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */

/******************************************************************************
Function Name : TESTEVT_Threading
  Description : Wrapper for STEVT threading tests
   Parameters :
******************************************************************************/
U32 TESTEVT_Threading ( void )
{
    ST_ErrorCode_t Error;
    U32 ErrorNum = 0;

    Error = TESTEVT_ListLock();
    if (Error == ST_NO_ERROR)
    {
        STEVT_Print(("ListLock test passed\n"));
    }
    else
    {
        STEVT_Print(("ListLock test FAILED\n"));
        ++ErrorNum;
    }
    
    Error = TESTEVT_ConcNotify();
    if (Error == ST_NO_ERROR)
    {
        STEVT_Print(("ConcNotify test passed\n"));
    }
    else
    {
        STEVT_Print(("ConcNotify test FAILED\n"));
        ++ErrorNum;
    }
    
    return (ErrorNum);
}
