/*******************************************************************************

File name   : trancoder.c

Description : Video driver transcoder init source file

COPYRIGHT (C) STMicroelectronics 2006.

Date               Modification                                     Name
----               ------------                                     ----
18 Aug 2006        Created from producer module                       OD
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>

#include "stddefs.h"

#include "transcoder.h"

#ifdef ST_multicodec
#include "vid_tran.h"
#endif /* ST_multicodec */

#include "sttbx.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/* Exported functions ------------------------------------------------------- */

/*******************************************************************************
Name        : VIDTRAN_Init
Description : Init trancode module
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDTRAN_Init(const VIDTRAN_InitParams_t * const InitParams_p, VIDTRAN_Handle_t * const TranscoderHandle_p)
{
    VIDTRAN_Properties_t * VIDTRAN_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Allocate a  data structure */
    VIDTRAN_Data_p = (VIDTRAN_Properties_t *) memory_allocate(InitParams_p->CPUPartition_p, sizeof(VIDTRAN_Properties_t));
    if (VIDTRAN_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *TranscoderHandle_p = (VIDTRAN_Handle_t *) VIDTRAN_Data_p;

    /* Store parameters */
    VIDTRAN_Data_p->CPUPartition_p         = InitParams_p->CPUPartition_p;

    VIDTRAN_Data_p->FunctionsTable_p = &VIDTRAN_Functions;

    /* Initialise transcode controller */
    ErrorCode = (VIDTRAN_Data_p->FunctionsTable_p->Init)(InitParams_p, &(((VIDTRAN_Properties_t *)(*TranscoderHandle_p))->InternalTranscoderHandle));

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: allocation failed, undo initialisations done */
        VIDTRAN_Term(*TranscoderHandle_p);
    }

    return(ErrorCode);
} /* End of VIDTRAN_Init() function */

/*******************************************************************************
Name        : VIDTRAN_Term
Description : Terminate transcoder module 
Parameters  :
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDTRAN_Term(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDTRAN_Properties_t * VIDTRAN_Data_p;

    /* Find structure */
    VIDTRAN_Data_p = (VIDTRAN_Properties_t *) TranscoderHandle;

    /* Terminate transcoder. */
    (VIDTRAN_Data_p->FunctionsTable_p->Term)(VIDTRAN_Data_p->InternalTranscoderHandle);

    /* De-allocate last: data inside cannot be used after ! */
    memory_deallocate(VIDTRAN_Data_p->CPUPartition_p, (void *) VIDTRAN_Data_p);

    return(ST_NO_ERROR);
} /* End of VIDTRAN_Term() function */

/* Private functions -------------------------------------------------------- */

/* End of transcode.c */
