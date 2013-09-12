/*******************************************************************************

File name   : halv_mult.c

Description : Multi-decode HAL video decode source file

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
30 Jul 2001        Created                                           HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"

#include "halv_mult.h"

#include "sttbx.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define HALDEC_MULTI_DECODE_VALID_HANDLE 0x01551555


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/* Exported functions ------------------------------------------------------- */


/*******************************************************************************
Name        : HALDEC_MultiDecodeInit
Description : Init multi-decode HAL
Parameters  : Init params, pointer to handle to return
Assumptions : Pointers are not NULL
Limitations :
Returns     : in params: HAL decode handle
*******************************************************************************/
ST_ErrorCode_t HALDEC_MultiDecodeInit(const HALDEC_MultiDecodeInitParams_t * const InitParams_p, HALDEC_MultiDecodeHandle_t * const MultiDecodeHandle_p)
{
    HALDEC_MultiDecodeProperties_t * HALDEC_MultiDecodeData_p;

    /* Allocate a HALDEC structure */
    SAFE_MEMORY_ALLOCATE(HALDEC_MultiDecodeData_p, HALDEC_MultiDecodeProperties_t *, InitParams_p->CPUPartition_p,
                         sizeof(HALDEC_MultiDecodeProperties_t));
    if (HALDEC_MultiDecodeData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *MultiDecodeHandle_p = (HALDEC_MultiDecodeHandle_t *) HALDEC_MultiDecodeData_p;
    HALDEC_MultiDecodeData_p->ValidityCheck = HALDEC_MULTI_DECODE_VALID_HANDLE;

    /* Store parameters */
    HALDEC_MultiDecodeData_p->CPUPartition_p    = InitParams_p->CPUPartition_p;
    HALDEC_MultiDecodeData_p->DeviceType        = InitParams_p->DeviceType;

    return(ST_NO_ERROR);
} /* End of HALDEC_MultiDecodeInit() function */


/*******************************************************************************
Name        : HALDEC_MultiDecodeTerm
Description : Terminate multi-decode HAL
Parameters  : HAL multi-decode handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HALDEC_MultiDecodeTerm(const HALDEC_MultiDecodeHandle_t MultiDecodeHandle)
{
    HALDEC_MultiDecodeProperties_t * HALDEC_MultiDecodeData_p = (HALDEC_MultiDecodeProperties_t *) MultiDecodeHandle;

    if ((HALDEC_MultiDecodeData_p == NULL) || (HALDEC_MultiDecodeData_p->ValidityCheck != HALDEC_MULTI_DECODE_VALID_HANDLE))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* De-validate structure */
    HALDEC_MultiDecodeData_p->ValidityCheck = 0; /* not HALDEC_MULTI_DECODE_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(HALDEC_MultiDecodeData_p, HALDEC_MultiDecodeData_p->CPUPartition_p,
                           sizeof(HALDEC_MultiDecodeProperties_t));

    return(ST_NO_ERROR);
} /* End of HALDEC_MultiDecodeTerm() function */




/* Private functions -------------------------------------------------------- */




/* End of halv_mult.c */
