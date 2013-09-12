/*!
 *******************************************************************************
 * \file mpg2toh264source.c
 *
 * \brief Transcodec MPEG2 to H264 source decoder functions
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "mpg2toh264source.h"

#if !defined ST_OSLINUX
#include "sttbx.h"
#endif

/* Static functions prototypes ---------------------------------------------- */

/* Constants ---------------------------------------------------------------- */
/* Source decoder */
const SOURCE_FunctionsTable_t SOURCE_MPG2Functions =
{
    mpg2toh264Source_Init,
    mpg2toh264Source_Start,
    mpg2toh264Source_UpdateAndGetRefList,
    mpg2toh264Source_IsPictureToBeSkipped,
    mpg2toh264Source_Term
};

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : mpg2toh264Source_Init
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Source_Init(const SOURCE_Handle_t SourceHandle, const SOURCE_InitParams_t * const InitParams_p)
{
    mpg2toh264source_PrivateData_t *    SOURCE_Data_p;
    SOURCE_Properties_t *               SOURCE_Properties_p;
    ST_ErrorCode_t                      ErrorCode;
    
    ErrorCode = ST_NO_ERROR;
    SOURCE_Properties_p = (SOURCE_Properties_t *) SourceHandle;

    if ((SourceHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate SOURCE PrivateData structure */
    SOURCE_Properties_p->PrivateData_p = (mpg2toh264source_PrivateData_t *) memory_allocate(InitParams_p->CPUPartition_p, sizeof(mpg2toh264source_PrivateData_t));
    SOURCE_Data_p = (mpg2toh264source_PrivateData_t *) SOURCE_Properties_p->PrivateData_p;

    if (SOURCE_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    else
    {
        memset(SOURCE_Data_p, 0xA5, sizeof(mpg2toh264source_PrivateData_t)); /* fill in data with 0xA5 by default */
    }

    /* Init private data */
    SOURCE_Data_p->IsFirstPicture = TRUE;
    SOURCE_Data_p->PreviousPictureDecodingOrderFrameID = 0;
    SOURCE_Data_p->BPictureCounter = 0;
    SOURCE_Data_p->BPictureBetweenP = 0;
    SOURCE_Data_p->ToggleBSkip = FALSE;
    SOURCE_Data_p->IsLastPictureSkipped = FALSE;

    return(ErrorCode);
}

/*******************************************************************************
Name        : mpg2toh264Source_Start
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Source_Start(const SOURCE_Handle_t SourceHandle, const SOURCE_StartParams_t * const StartParams_p)
{
    mpg2toh264source_PrivateData_t  *SOURCE_Data_p;
    SOURCE_Properties_t             *SOURCE_Properties_p;
    ST_ErrorCode_t                  ErrorCode;

    UNUSED_PARAMETER(StartParams_p);

    ErrorCode = ST_NO_ERROR;
    SOURCE_Properties_p = (SOURCE_Properties_t *) SourceHandle;
    SOURCE_Data_p = (mpg2toh264source_PrivateData_t *) SOURCE_Properties_p->PrivateData_p;

    /* Re-Init private data */
    SOURCE_Data_p->IsFirstPicture = TRUE;
    SOURCE_Data_p->PreviousPictureDecodingOrderFrameID = 0;
    SOURCE_Data_p->BPictureCounter = 0;
    SOURCE_Data_p->BPictureBetweenP = 0;
    SOURCE_Data_p->ToggleBSkip = FALSE;
    SOURCE_Data_p->IsLastPictureSkipped = FALSE;

    return(ErrorCode);

}

/*******************************************************************************
Name        : mpg2toh264Source_Term
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Source_Term(const SOURCE_Handle_t SourceHandle)
{
    mpg2toh264source_PrivateData_t * SOURCE_Data_p;
    SOURCE_Properties_t * SOURCE_Properties_p;
    ST_ErrorCode_t ErrorCode;

    ErrorCode = ST_NO_ERROR;
    SOURCE_Properties_p = (SOURCE_Properties_t *) SourceHandle;
    SOURCE_Data_p = (mpg2toh264source_PrivateData_t *) SOURCE_Properties_p->PrivateData_p;

    /* Deallocate private data */
    if (SOURCE_Data_p != NULL)
    {
        memory_deallocate(SOURCE_Properties_p->CPUPartition_p, (void *) SOURCE_Data_p);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : mpg2toh264Source_UpdateAndGetRefList
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Source_UpdateAndGetRefList(const SOURCE_Handle_t SourceHandle, SOURCE_UpdateAndGetRefListParams_t * const SOURCE_UpdateAndGetRefListParams_p)
{
    mpg2toh264source_PrivateData_t  *SOURCE_Data_p;
    SOURCE_Properties_t             *SOURCE_Properties_p;
    ST_ErrorCode_t                  ErrorCode;

    UNUSED_PARAMETER(SOURCE_UpdateAndGetRefListParams_p);

    ErrorCode = ST_NO_ERROR;
    SOURCE_Properties_p = (SOURCE_Properties_t *) SourceHandle;
    SOURCE_Data_p = (mpg2toh264source_PrivateData_t *) SOURCE_Properties_p->PrivateData_p;

    return(ErrorCode);

}

/*******************************************************************************
Name        : mpg2toh264Source_IsPictureToBeSkipped
Description : To indicate whether the current picture shall be skipped
Parameters  : SourceHandle
              CurrentPictureDecodingOrderFrameID (for field transcoding)
              MPEGFrame (current picture)
              SourceFrameRate
              TargetFrameRate
Assumptions :
Limitations :
Returns     : BOOL: TRUE is current picture is to be skipped
*******************************************************************************/
BOOL mpg2toh264Source_IsPictureToBeSkipped(const SOURCE_Handle_t SourceHandle, 
                                           const U32 CurrentPictureDOFID, 
                                           const STVID_MPEGFrame_t MPEGFrame, 
                                           const U32 SourceFrameRate, 
                                           const U32 TargetFrameRate)
{
    mpg2toh264source_PrivateData_t * SOURCE_Data_p;
    SOURCE_Properties_t * SOURCE_Properties_p;
    ST_ErrorCode_t ErrorCode;
    SkipBMethod_t SkipBMethod;
    BOOL SkipThisPicture;
    
    ErrorCode = ST_NO_ERROR;
    SOURCE_Properties_p = (SOURCE_Properties_t *) SourceHandle;
    SOURCE_Data_p = (mpg2toh264source_PrivateData_t *) SOURCE_Properties_p->PrivateData_p;

    SkipThisPicture = FALSE;
    
    /* Field management */
    if ((SOURCE_Data_p->IsFirstPicture == FALSE) && (CurrentPictureDOFID == SOURCE_Data_p->PreviousPictureDecodingOrderFrameID))
    {
        /* Current Picture is the complementary field of previous one */
        /* Apply the same pattern as the previous one */
        return (SOURCE_Data_p->IsLastPictureSkipped);
        /* And do not modify the Skip-B state machine */
    }

    SOURCE_Data_p->IsFirstPicture = FALSE;

    /* Keep track for next picture */
    SOURCE_Data_p->PreviousPictureDecodingOrderFrameID = CurrentPictureDOFID;
        
    if (MPEGFrame != STVID_MPEG_FRAME_B)
    {
        /* Store gop structure: follows potential gop evolution */
        SOURCE_Data_p->BPictureBetweenP = SOURCE_Data_p->BPictureCounter;
        /* Reset B Counter */
        SOURCE_Data_p->BPictureCounter = 0;
        /* Cannot skip Reference pictures */
        SkipThisPicture = FALSE;
        SOURCE_Data_p->IsLastPictureSkipped = SkipThisPicture;
        return (SkipThisPicture);
    }

    /* Information: target frame versus source frame rate
     * as a function of number of B to skip
     *
     * Source Frame Rate: 30Hz
     *              Skip 1 B   Skip 2 B   Skip 3B
     * IBPB stream: 15         15        15
     * IBBPBB       20         10        10
     * IBBBPBBB     22.5       15        7.5
     *
     * Source Frame Rate 25Hz
     *              Skip 1 B   Skip 2 B   Skip 3B
     * IBPB stream: 12.5       12.5       12.5
     * IBBPBB       16.5       8.3         8.3
     * IBBBPBBB     18.5       12.5        6.3
     */

    /* Assumption is that the incoming stream will never have more than 3 consecutive B between P */    

    if (SourceFrameRate >= 29000)
    {
         if (TargetFrameRate > 25000)
         {
            /* Do not skip any B */
             SkipBMethod = SKIP_0_B;             
         }
         else if (TargetFrameRate > 15000)
         {
             /* Skip 1 B: It will result in target stream to be ~15Hz to 22.5 Hz */            
             SkipBMethod = SKIP_1_B; 
         }
         else if (TargetFrameRate > 10000)
         {
             /* Skip 2 B: It will result in target stream to be ~10Hz to 15 Hz */                        
             SkipBMethod = SKIP_2_B; 
         }
         else
         {
             /* Skip 3 B: It will result in target stream to be ~7.5Hz to 15 Hz */                        
             SkipBMethod = SKIP_3_B; 
         }
    }
    else
    {
         if (TargetFrameRate > 20000)
         {
            /* Do not skip any B */
             SkipBMethod = SKIP_0_B;                      
         }
         else if (TargetFrameRate > 12500)
         {
             /* Skip 1 B: It will result in target stream to be ~12.5Hz to 18.5 Hz */            
             SkipBMethod = SKIP_1_B; 
         }
         else if (TargetFrameRate > 10000)
         {
             /* Skip 2 B: It will result in target stream to be ~8.3Hz to 12.5 Hz */                        
             SkipBMethod = SKIP_2_B; 
         }
         else
         {
             /* Skip 3 B: It will result in target stream to be ~6.3Hz to 12.5 Hz */                        
             SkipBMethod = SKIP_3_B; 
         }
    }

    if (SkipBMethod == SKIP_0_B)
    {
        SkipThisPicture = FALSE;
        SOURCE_Data_p->IsLastPictureSkipped = SkipThisPicture;
        return (SkipThisPicture);
    }
    
    SOURCE_Data_p->BPictureCounter++;
    if ((SOURCE_Data_p->BPictureCounter == 1) &&
        (SkipBMethod == SKIP_1_B) &&
        (SOURCE_Data_p->BPictureBetweenP == 2)
       )
    {
        /* Toggle: to avoid repetitive skip pattern:
           select either the first or the second to skip */
        if (SOURCE_Data_p->ToggleBSkip)
        {
            SkipThisPicture = TRUE;
        }
        else
        {
            SkipThisPicture = FALSE;        
        }
        SOURCE_Data_p->ToggleBSkip = !SOURCE_Data_p->ToggleBSkip;
    }
    else if ((SOURCE_Data_p->BPictureCounter == 1) &&
             (SkipBMethod == SKIP_1_B) &&
             (SOURCE_Data_p->BPictureBetweenP == 3)
            )
    {
        /* Don't skip the first picture of 3 consecutive if only 1 B is to be skipped */
        SkipThisPicture = FALSE;
    }
    else if ((SOURCE_Data_p->BPictureCounter == 2) &&
             (SkipBMethod == SKIP_1_B) &&
             (SOURCE_Data_p->BPictureBetweenP == 2)
            )
    {
        /* Toggle: to avoid repetitive skip pattern:
           select either the first or the second to skip */
        if (SOURCE_Data_p->ToggleBSkip)
        {
            SkipThisPicture = TRUE;
        }
        else
        {
            SkipThisPicture = FALSE;        
        }
        SOURCE_Data_p->ToggleBSkip = !SOURCE_Data_p->ToggleBSkip;
    }
    else if ((SOURCE_Data_p->BPictureCounter == 2) &&
             (SkipBMethod == SKIP_2_B) &&
             (SOURCE_Data_p->BPictureBetweenP == 3)
            )
    {
        /* Don't skip the second B when 2-B to skip in 3 consecutive B */
        SkipThisPicture = FALSE;
    }
    else if ((SOURCE_Data_p->BPictureCounter == 3) &&
             (SkipBMethod == SKIP_1_B) &&
             (SOURCE_Data_p->BPictureBetweenP == 3)
            )
    {
        /* Don't skip the last picture of 3 consecutive if only 1 B is to be skipped */
        SkipThisPicture = FALSE;
    }
    else
    {
        /* In all other cases, skip the B */
        SkipThisPicture = TRUE;        
    }
    
#if 0
    if (SOURCE_Data_p->BPictureCounter == 1)
    {
        SkipThisPicture = TRUE;
    }
    else if (SOURCE_Data_p->BPictureCounter == 2)
    {
        if (SkipBMethod != SKIP_1_B)
        {
            SkipThisPicture = TRUE;        
        }
    }
    else if (SOURCE_Data_p->BPictureCounter == 3)
    {
        if (SkipBMethod == SKIP_3_B)
        {
            SkipThisPicture = TRUE;        
        }
    }
#endif

    SOURCE_Data_p->IsLastPictureSkipped = SkipThisPicture;
    return (SkipThisPicture);
}

/* End of mpg2toh264source.c */
