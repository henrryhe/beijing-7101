/* ----------------------------------------------------------------------------
File Name: sysdbase.c

Description: Simple access to driver and instance databases.

Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 19-June-2001
version: 3.1.0
 author: GJP
comment: write for multi-instance.
    
Reference:
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
 
/* C libs */
#include <string.h>                     
#endif
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* header for this file */



/* private variables ------------------------------------------------------- */

/* database of driver types */
static STTUNER_DriverDbase_t    STTUNER_DrvDb;


/* database of instances, indexed by value: [0..STTUNER_MAX_HANDLES-1] */
static STTUNER_InstanceDbase_t  STTUNER_DrvInst[STTUNER_MAX_HANDLES];


/* ----------------------------------------------------------------------------
Name: STTUNER_GetDrvDb()

Description:
    
Parameters:
    
Return Value:
    
See Also:

---------------------------------------------------------------------------- */
STTUNER_DriverDbase_t *STTUNER_GetDrvDb(void)
{
    return (&STTUNER_DrvDb);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_GetDrvInst()

Description:
    
Parameters:
    
Return Value:
    
See Also:

---------------------------------------------------------------------------- */
STTUNER_InstanceDbase_t *STTUNER_GetDrvInst(void)
{
    return (STTUNER_DrvInst);
}



/* End of sysdbase.c */
