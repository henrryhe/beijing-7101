/*******************************************************************************

File name   : vid_mult.c

Description : Video multi-decode source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
30 Jul 2001        Created                                           HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "vid_dec.h"
#include "sttbx.h"

/* Private Types ------------------------------------------------------------ */

/* Type for internal structures for multi-decode */
typedef struct {
    ST_Partition_t *    CPUPartition_p;
    STVID_DeviceType_t  DeviceType;
    void *              DeviceBaseAddress_p;
    HALDEC_MultiDecodeHandle_t HALMultiDecodeHandle;
    semaphore_t *       DataProtectionSemaphore_p;
    U8                  NumberInstancesConnected; /* limited by VIDDEC_MAX_MULTI_DECODE_CONNECT */
    void *              MultiDecodeReadyOrders_p; /* size of is VIDDEC_MAX_MULTI_DECODE_CONNECT, to store pending orders that could not be processed straight becuase a decode was on-going */
    U8                  UsedNumberMultiDecodeReadyOrders; /* Number of orders used in MultiDecodeReadyOrders_p */
    BOOL                IsOneDecodeOnGoing;
    U32                 ValidityCheck;
} VIDDEC_MultiDecodeData_t;

/* Parameters required to decode a picture */
typedef struct
{
    BOOL                                IsUsed;                 /* Set TRUE when a decode ready arrives, set FALSE when the decode is executed and finished. */
    BOOL                                IsOnGoing;              /* TRUE while decode is on-going, FALSE otherwise. */
    VIDDEC_MultiDecodeReadyParams_t     MultiDecodeReadyParams;
} MultiDecodeReadyData_t;


/* Private Constants -------------------------------------------------------- */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define VIDDEC_MULTI_DECODE_VALID_HANDLE 0x0155155a


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static MultiDecodeReadyData_t * FindMostUrgentDecode(const MultiDecodeReadyData_t * MultiDecodeReadyData_p);

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : VIDDEC_MultiDecodeConnect
Description : Connect to a multi-instance software controller: to be done by each
              decoder using it, gives the handle for the decoder's HAL calls
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_INVALID_HANDLE if invalid handle
*******************************************************************************/
ST_ErrorCode_t VIDDEC_MultiDecodeConnect(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle)
{

    if ((((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle) == NULL) ||
        (((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle)->ValidityCheck != VIDDEC_MULTI_DECODE_VALID_HANDLE))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Lock access to multi-instance software controler data */
    semaphore_wait(((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle)->DataProtectionSemaphore_p);

    /* Connect */
    ((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle)->NumberInstancesConnected ++;

    /* Unlock access to multi-instance software controler data */
    semaphore_signal(((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle)->DataProtectionSemaphore_p);

    return(ST_NO_ERROR);
} /* End of VIDDEC_MultiDecodeConnect() function */


/*******************************************************************************
Name        : VIDDEC_MultiDecodeDisconnect
Description : Disconnect from a multi-instance software controller: to be done by
              each decoder using it when finished using it.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_INVALID_HANDLE if invalid handle
              ST_ERROR_NO_MEMORY if no connected decode
*******************************************************************************/
ST_ErrorCode_t VIDDEC_MultiDecodeDisconnect(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle)
{
    if ((((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle) == NULL) ||
        (((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle)->ValidityCheck != VIDDEC_MULTI_DECODE_VALID_HANDLE))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check that there are connections */
    if (((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle)->NumberInstancesConnected == 0)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Lock access to multi-instance software controler data */
    semaphore_wait(((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle)->DataProtectionSemaphore_p);

    /* Disconnect */
    ((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle)->NumberInstancesConnected --;

    /* Unlock access to multi-instance software controler data */
    semaphore_signal(((VIDDEC_MultiDecodeData_t *) MultiDecodeHandle)->DataProtectionSemaphore_p);

    return(ST_NO_ERROR);
} /* End of VIDDEC_MultiDecodeDisconnect() function */


/*******************************************************************************
Name        : VIDDEC_MultiDecodeFinished
Description : Inform multi-decode that a decode is finished
Parameters  : handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_INVALID_HANDLE if invalid handle
              ST_ERROR_BAD_PARAMETER if bad parameter (internal)
*******************************************************************************/
ST_ErrorCode_t VIDDEC_MultiDecodeFinished(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle)
{
    VIDDEC_MultiDecodeData_t * VIDDEC_MultiDecodeData_p = (VIDDEC_MultiDecodeData_t *) MultiDecodeHandle;
    MultiDecodeReadyData_t * NextDecodeFound_p = NULL;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 k;

    if ((VIDDEC_MultiDecodeData_p == NULL) || (VIDDEC_MultiDecodeData_p->ValidityCheck != VIDDEC_MULTI_DECODE_VALID_HANDLE))
    {
        /* Error case */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Lock access to multi-instance software controler data */
    semaphore_wait(VIDDEC_MultiDecodeData_p->DataProtectionSemaphore_p);

    /* Check decode on-going and set it unused */
    for (k = 0; k < VIDDEC_MAX_MULTI_DECODE_CONNECT; k ++)
    {
        /* Look for on-going decode */
        if ((((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p)[k].IsUsed) &&
            (((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p)[k].IsOnGoing))
        {
            ((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p)[k].IsUsed = FALSE;
            VIDDEC_MultiDecodeData_p->UsedNumberMultiDecodeReadyOrders --;
            /* Found: should quit 'for' loop, but continue to be sure no IsOnGoing flag is still TRUE */
/*            break;*/
        }
    }

    /* No decode is on-going now */
    VIDDEC_MultiDecodeData_p->IsOneDecodeOnGoing = FALSE;

    /* If there are other decodes to execute: find the good one */
    if (VIDDEC_MultiDecodeData_p->UsedNumberMultiDecodeReadyOrders > 1)
    {
        /* More than one decode pending: find most urgent decode to execute */
        NextDecodeFound_p = FindMostUrgentDecode(((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p));
    } /* end there are decodes to execute */
    else if (VIDDEC_MultiDecodeData_p->UsedNumberMultiDecodeReadyOrders == 1)
    {
        /* Only one decode pending: find it and execute it. */
        k = 0;
        do
        {
            if (((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p)[k].IsUsed)
            {
                /* Found order */
                NextDecodeFound_p = ((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p) + k;
            } /* end IsUsed */
            k ++;
        } while ((NextDecodeFound_p == NULL) && (k < VIDDEC_MAX_MULTI_DECODE_CONNECT));
    }

    if (NextDecodeFound_p != NULL)
    {
        /* Found next decode to excute: execute it now ! */
        VIDDEC_MultiDecodeData_p->IsOneDecodeOnGoing = TRUE;
        NextDecodeFound_p->IsOnGoing = TRUE;
        (NextDecodeFound_p->MultiDecodeReadyParams.ExecuteFunction) (NextDecodeFound_p->MultiDecodeReadyParams.ExecuteFunctionParam);
    } /* end execute decode */

    if ((NextDecodeFound_p == NULL) &&
        (VIDDEC_MultiDecodeData_p->UsedNumberMultiDecodeReadyOrders > 0))
    {
        /* Error case that should not happen */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    } /* end execute decode */

    /* Unlock access to multi-instance software controler data */
    semaphore_signal(VIDDEC_MultiDecodeData_p->DataProtectionSemaphore_p);

    return(ErrorCode);

} /* End of VIDDEC_MultiDecodeFinished() function */


/*******************************************************************************
Name        : VIDDEC_MultiDecodeInit
Description : Initialise multi-instance software controller : to be done once for all decoders using it
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_MultiDecodeInit(const VIDDEC_MultiDecodeInitParams_t * const InitParams_p, VIDDEC_MultiDecodeHandle_t * const MultiDecodeHandle_p)
{
    VIDDEC_MultiDecodeData_t * VIDDEC_MultiDecodeData_p;
    HALDEC_MultiDecodeInitParams_t HALInitParams;
    U32 k;
    ST_ErrorCode_t ErrorCode;

    if ((MultiDecodeHandle_p == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate a HALDEC structure */
    SAFE_MEMORY_ALLOCATE(VIDDEC_MultiDecodeData_p, VIDDEC_MultiDecodeData_t *, InitParams_p->CPUPartition_p,
                         sizeof(VIDDEC_MultiDecodeData_t));
    if (VIDDEC_MultiDecodeData_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocate table of decode ready orders */
    SAFE_MEMORY_ALLOCATE(VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p, MultiDecodeReadyData_t *,
                         InitParams_p->CPUPartition_p, (VIDDEC_MAX_MULTI_DECODE_CONNECT - 1) * sizeof(MultiDecodeReadyData_t));
    if (VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p == NULL)
    {
        /* Error: allocation failed, deallocate previous allocations */
        SAFE_MEMORY_DEALLOCATE(VIDDEC_MultiDecodeData_p, InitParams_p->CPUPartition_p, sizeof(VIDDEC_MultiDecodeData_t));
        return(ST_ERROR_NO_MEMORY);
    }
    /* Initialise decode ready orders structures */
    for (k = 0; k < VIDDEC_MAX_MULTI_DECODE_CONNECT; k ++)
    {
        /* Initialise orders as not used */
        ((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p)[k].IsUsed = FALSE;
    }

    /* Remember partition */
    VIDDEC_MultiDecodeData_p->CPUPartition_p = InitParams_p->CPUPartition_p;

    /* Allocation succeeded: make handle point on structure */
    *MultiDecodeHandle_p = (VIDDEC_MultiDecodeHandle_t *) VIDDEC_MultiDecodeData_p;
    VIDDEC_MultiDecodeData_p->ValidityCheck = VIDDEC_MULTI_DECODE_VALID_HANDLE;

    /* Store parameters */
    VIDDEC_MultiDecodeData_p->DeviceType = InitParams_p->DeviceType;

    /* Initialize parameters */
    VIDDEC_MultiDecodeData_p->NumberInstancesConnected = 0;
    VIDDEC_MultiDecodeData_p->UsedNumberMultiDecodeReadyOrders = 0;
    VIDDEC_MultiDecodeData_p->IsOneDecodeOnGoing = FALSE;

    VIDDEC_MultiDecodeData_p->DataProtectionSemaphore_p = semaphore_create_priority(1);

    /* Init HAL */
    HALInitParams.CPUPartition_p    = InitParams_p->CPUPartition_p;
    HALInitParams.DeviceType        = InitParams_p->DeviceType;
    ErrorCode = HALDEC_MultiDecodeInit(&HALInitParams, &(VIDDEC_MultiDecodeData_p->HALMultiDecodeHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: HAL init failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not initialize HAL multi-decode !"));
        VIDDEC_MultiDecodeTerm(*MultiDecodeHandle_p);
        return(ErrorCode);
    }

    return(ErrorCode);
} /* End of VIDDEC_MultiDecodeInit() function */


/*******************************************************************************
Name        : VIDDEC_MultiDecodeReady
Description : A decode is ready for multi-decode HAL
Parameters  : Handle, decode params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_INVALID_HANDLE if invalid handle
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if could not find a place for the order
*******************************************************************************/
ST_ErrorCode_t VIDDEC_MultiDecodeReady(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle, const VIDDEC_MultiDecodeReadyParams_t * const MultiDecodeReadyParams_p)
{
    VIDDEC_MultiDecodeData_t * VIDDEC_MultiDecodeData_p = (VIDDEC_MultiDecodeData_t *) MultiDecodeHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 k;

    if ((VIDDEC_MultiDecodeData_p == NULL) || (VIDDEC_MultiDecodeData_p->ValidityCheck != VIDDEC_MULTI_DECODE_VALID_HANDLE))
    {
        /* Error case */
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (MultiDecodeReadyParams_p == NULL)
    {
        /* Error case */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Lock access to multi-instance software controler data */
    semaphore_wait(VIDDEC_MultiDecodeData_p->DataProtectionSemaphore_p);

    /* Check if one decode is not already on-going */
    if (!(VIDDEC_MultiDecodeData_p->IsOneDecodeOnGoing))
    {
        /* No decode on-going, execute right now */
        VIDDEC_MultiDecodeData_p->IsOneDecodeOnGoing = TRUE;
        (MultiDecodeReadyParams_p->ExecuteFunction) (MultiDecodeReadyParams_p->ExecuteFunctionParam);
    }
    else
    {
        /* One decode is already on-going: save order to be processed later */
        ErrorCode = ST_ERROR_NO_MEMORY;
        k = 0;
        do /* for all the table of decode orders */
        {
            /* Look for free decode */
            if (!((((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p)[k].IsUsed)))
            {
                ((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p)[k].IsOnGoing = FALSE;
                ((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p)[k].MultiDecodeReadyParams = *MultiDecodeReadyParams_p;
                ((MultiDecodeReadyData_t *) VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p)[k].IsUsed = TRUE;
                VIDDEC_MultiDecodeData_p->UsedNumberMultiDecodeReadyOrders ++;
                /* Found: quit 'do while' loop */
                ErrorCode = ST_NO_ERROR;
            }
            k ++;
        } while((k < VIDDEC_MAX_MULTI_DECODE_CONNECT) && (ErrorCode != ST_NO_ERROR));
    } /* end one decode on-going */

    /* Unlock access to multi-instance software controler data */
    semaphore_signal(VIDDEC_MultiDecodeData_p->DataProtectionSemaphore_p);

    /* Should never happen if MAX_NUMBER_OF_MULTI_DECODE is well choosen according
    to STVID decoder instances, and if one instance launch only one decodde at a time. (normal) */
    return(ErrorCode);

} /* End of VIDDEC_MultiDecodeReady() function */


/*******************************************************************************
Name        : VIDDEC_MultiDecodeTerm
Description : Terminate multi-instance software controller : to be done once for all decoders using it
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if memory allocation fails
              STVID_ERROR_EVENT_REGISTRATION if problem registering events
*******************************************************************************/
ST_ErrorCode_t VIDDEC_MultiDecodeTerm(const VIDDEC_MultiDecodeHandle_t MultiDecodeHandle)
{
    VIDDEC_MultiDecodeData_t * VIDDEC_MultiDecodeData_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    VIDDEC_MultiDecodeData_p = (VIDDEC_MultiDecodeData_t *) MultiDecodeHandle;

    if ((VIDDEC_MultiDecodeData_p == NULL) || (VIDDEC_MultiDecodeData_p->ValidityCheck != VIDDEC_MULTI_DECODE_VALID_HANDLE))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Terminate HAL */
    HALDEC_MultiDecodeTerm(VIDDEC_MultiDecodeData_p->HALMultiDecodeHandle);

    semaphore_delete(VIDDEC_MultiDecodeData_p->DataProtectionSemaphore_p);

    /* De-validate structure */
    VIDDEC_MultiDecodeData_p->ValidityCheck = 0; /* not VIDDEC_MULTI_DECODE_VALID_HANDLE */

    /* De-allocate structures */
    SAFE_MEMORY_DEALLOCATE(VIDDEC_MultiDecodeData_p->MultiDecodeReadyOrders_p, VIDDEC_MultiDecodeData_p->CPUPartition_p,
                           (VIDDEC_MAX_MULTI_DECODE_CONNECT - 1) * sizeof(MultiDecodeReadyData_t));

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDDEC_MultiDecodeData_p, VIDDEC_MultiDecodeData_p->CPUPartition_p, sizeof(VIDDEC_MultiDecodeData_t));

    return(ErrorCode);
} /* End of VIDDEC_MultiDecodeTerm() function */


/* Private functions -------------------------------------------------------- */



/*******************************************************************************
Name        : FindMostUrgentDecode
Description : Find next decode to execute according to priority rules
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static MultiDecodeReadyData_t * FindMostUrgentDecode(const MultiDecodeReadyData_t * MultiDecodeReadyData_p)
{
    U8 k = 0;
    BOOL Found = FALSE;
    U8 MaxPoints = 0, CurrentPoints;
    const MultiDecodeReadyData_t * DecodeFound_p = NULL;

    do
    {
        if (MultiDecodeReadyData_p[k].IsUsed)
        {
            /* Attribute points of priority */
            CurrentPoints = MultiDecodeReadyData_p->MultiDecodeReadyParams.DecoderNumber;
            if (MultiDecodeReadyData_p->MultiDecodeReadyParams.DecodeScanType == STGXOBJ_INTERLACED_SCAN)
            {
                CurrentPoints |= 0x10;
            }
            if (MultiDecodeReadyData_p->MultiDecodeReadyParams.DecodePictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
            {
                CurrentPoints |= 0x20;
            }

            /* Take the decode having most points of priority */
            if (CurrentPoints > MaxPoints)
            {
                MaxPoints = CurrentPoints;
                DecodeFound_p = MultiDecodeReadyData_p + k;
            }
        } /* end IsUsed */
        k ++;
    } while ((!Found) && (k < VIDDEC_MAX_MULTI_DECODE_CONNECT));

    /* Could not find decode to execute: should never happen unless data is corrupted:
     if UsedNumberMultiDecodeReadyOrders was != 0, there must be a decode to execute ! */

    return((MultiDecodeReadyData_t *) DecodeFound_p);

} /* End of FindMostUrgentDecode() function */


/* End of vid_mult.c */
