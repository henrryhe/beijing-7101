/*******************************************************************
               MPEG Video Codec Parser Task Module

            Copyright 2004 Scientific-Atlanta, Inc.

                  Subscriber Engineering
                  5030 Sugarloaf Parkway
                     P. O. Box 465447
                  Lawrenceville, GA 30042

                 Proprietary and Confidential
        Unauthorized distribution or copying is prohibited
                    All rights reserved

 No part of this computer software may be reprinted, reproduced or utilized
 in any form or by any electronic, mechanical, or other means, now known or
 hereafter invented, including photocopying and recording, or using any
 information storage and retrieval system, without permission in writing
 from Scientific Atlanta, Inc.

*******************************************************************

 File Name:    mpg2parsertsk.c

 See Also:     mpg2parser.h

 Project:      Zeus

 Compiler:     ST40

 Author:       John Bean

 Description:  Parser Task for MPEG Codec Parser

 Documents:

 Notes:

******************************************************************
 History:
 Rev Level    Date    Name    ECN#    Description
------------------------------------------------------------------
 1.00       10/04/04  JRB             First Release
******************************************************************/

/* ====== includes ====== */
#if !defined ST_OSLINUX
#include <stdio.h>
#endif

#include "sttbx.h"

#include "vid_com.h"

/* MPG2 Parser specific */
#include "mpg2parser.h"


/*#define TRACE_UART*/
#if defined TRACE_UART
#include "../../../../tests/src/trace.h"
#define TraceParse(x) TraceState x
#else
#define TraceParse(x)
#endif /* TRACE_UART */
/* ====== file info ====== */

/* ====== defines ====== */

/* Shortcuts to Start Code entries */

#ifdef DEBUG_STARTCODE
#define FORMAT_SPEC_OSCLOCK "ll"
#define OSCLOCK_T_MILLE     1000LL
#endif

/*#define COMPARE_OMEGA 1*/
#define DELTAMU_PARSER 1
#define MME_SIMULATOR 1

/*wait time for incrementing SC List Timeout : 2 fields display time so that it doesn't increment too quickly*/
#define MAX_WAIT_SCLIST    (2*STVID_MAX_VSYNC_DURATION)

/* ====== enums ======*/


/* ====== typedefs ======*/


/* ====== globals ======*/

/* osclock_t                               Time_start_PARS, time_end_PARS ;*/

/* ====== statics ======*/


#ifdef DEBUG_STARTCODE
static U8 * previousSC = 0;
static U8 * Loop[MAX_TRACES_SC];
static U8 * NexSC[MAX_TRACES_SC];
static U8 * Write[MAX_TRACES_SC];
#endif
#ifdef WA_SYNC_ON_PRIVIOUS_PTS
STVID_PTS_t CPTS, LastPTS;
#endif /* WA_SYNC_ON_PRIVIOUS_PTS */
/*#define STVID_VALID*/
/* ====== prototypes ======*/


void mpg2par_ParserTaskFunc(const PARSER_Handle_t ParserHandle);
void mpg2par_UpdateReferenceList(const PARSER_Handle_t ParserHandle);
void mpg2par_GetControllerCommand(const PARSER_Handle_t ParserHandle);
BOOL mpg2parser_HasPreviousReference(const PARSER_Handle_t ParserHandle);
void mpg2parser_ResetReferences(const PARSER_Handle_t ParserHandle);
void mpg2par_PointToNextSC(const PARSER_Handle_t ParserHandle);
void mpg2par_GetNextCompleteStartCode(const PARSER_Handle_t ParserHandle);
void mpg2par_ProcessCompleteStartCode(const PARSER_Handle_t ParserHandle);

static void mpg2par_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* CPUPTSEntry_p);
static void mpg2par_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle);

static void mpg2par_NewReferencePicture(const PARSER_Handle_t ParserHandle);
static void mpg2par_UpdateReferences(const PARSER_Handle_t ParserHandle);
void mpg2par_FillParsingJobResult(const PARSER_Handle_t ParserHandle);
#ifdef STVID_VALID
static void mpg2parserDumpParsingJobResults (PARSER_ParsingJobResults_t *ParsingJobResults_p, BOOL close);
/* Private interface for MME debug data dump*/
void DumpMMECommandInfo (void *TransformParam_p,
                         void *GlobalParam_p,
                         U32  DecodingOrderFrameID,
                         BOOL FirstPictureEver,
                         BOOL IsBitStreamMPEG1,
                         BOOL StreamTypeChange,
                         BOOL NewSeqHdr,
                         BOOL NewQuantMtx,
                         BOOL closeFiles);
#endif

/* PTS List access shortcuts */
#define PTSLIST_BASE     (PARSER_Data_p->PTS_SCList.SCList)
#define PTSLIST_READ     (PARSER_Data_p->PTS_SCList.Read_p)
#define PTSLIST_WRITE    (PARSER_Data_p->PTS_SCList.Write_p)
#define PTSLIST_TOP      (&PARSER_Data_p->PTS_SCList.SCList[PTS_SCLIST_SIZE])

/*******************************************************************
Function:    mpg2par_StartParserTask

Parameters:  ParserHandle

Returns:     ST_NO_ERROR if success
             ST_ERROR_ALREADY_INITIALIZED if task already running
             ST_ERROR_BAD_PARAMETER if problem of creation

Scope:       Private to parser

Purpose:     Starts the parser task

Behavior:

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t mpg2par_StartParserTask(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    VIDCOM_Task_t * const Task_p = &(PARSER_Data_p->ParserTask);
    char TaskName[25] = "STVID[?].MPEG2ParserTask";

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + PARSER_Properties_p->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;
        /* TODO: define PARSER own priority in vid_com.h */

    if (STOS_TaskCreate((void (*) (void*)) mpg2par_ParserTaskFunc,
                             (void *) ParserHandle,
                              PARSER_Properties_p->CPUPartition_p,
                              STVID_TASK_STACK_SIZE_DECODE,
                              &(Task_p->TaskStack),
                              PARSER_Properties_p->CPUPartition_p,
                              &(Task_p->Task_p),
                              &(Task_p->TaskDesc),
                              STVID_TASK_PRIORITY_PARSE,
                              TaskName,
                              task_flags_no_min_stack_size) != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Task_p->IsRunning  = TRUE;
    PARSER_Data_p->PreviousPictureIsAvailable= TRUE;
    return(ST_NO_ERROR);
} /* End of StartParserTask() function */

/*******************************************************************
Function:    mpg2par_StopParserTask

Parameters:  ParserHandle

Returns:     ST_NO_ERROR if success
             ST_ERROR_ALREADY_INITIALIZED if task not running

Scope:       Private to parser

Purpose:     Stops the parser task

Behavior:

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t mpg2par_StopParserTask(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
   PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
   MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
   VIDCOM_Task_t             * const Task_p  = &(PARSER_Data_p->ParserTask);
   task_t                    * TaskList_p    = Task_p->Task_p;

    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

#ifdef STVID_VALID
    mpg2parserDumpParsingJobResults (&PARSER_Data_p->ParserJobResults, TRUE);
#endif

    /* Signal semaphore to release task that may wait infinitely if stopped */
    STOS_SemaphoreSignal(PARSER_Data_p->ParserOrder);
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                      ((PARSER_Properties_t *) ParserHandle)->CPUPartition_p,
                      &Task_p->TaskStack,
                      ((PARSER_Properties_t *) ParserHandle)->CPUPartition_p);
    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopParserTask() function */

/*******************************************************************
Function:    mpg2par_GetControllerCommand

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Get the command from the Parser

Behavior:

Exceptions:  None

*******************************************************************/

void mpg2par_GetControllerCommand(const PARSER_Handle_t ParserHandle)
{
#if 0
    /* dereference ParserHandle to a local variable to ease debug */
    MPG2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPG2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STOS_SemaphoreWait(PARSER_Data_p->ControllerSharedAccess);
    if (PARSER_Data_p->ForTask.ControllerCommand)
	{
		PARSER_Data_p->ParserState = PARSER_STATE_PARSING;
		PARSER_Data_p->ForTask.ControllerCommand = FALSE;
	}
    STOS_SemaphoreSignal(PARSER_Data_p->ControllerSharedAccess);

	/* Force Parser to read next start code, if any */
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
#else

    /* dereference ParserHandle to a local variable to ease debug */
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    U8   ControllerCommand;

    /* Process all controller commands together... */
    while (vidcom_PopCommand(&PARSER_Data_p->CommandsBuffer, &ControllerCommand) == ST_NO_ERROR)
    {
       if (ControllerCommand == PARSER_COMMAND_START)
       {
           if (PARSER_Data_p->ParserState == PARSER_STATE_PARSING)
            {
                /* The parser is already parsing. Why did the controller sent a "Start" command again ?
                 * It should have wait until the Parser sends a new event "Job completed"
                 * Ignore the command!
                 */
            }
            else
            {
                /* The parser was waiting for a controller "Start" command to start processing the stream */
                /* Let's move to the "Parsing" State */
                PARSER_Data_p->ParserState = PARSER_STATE_PARSING;
            }
        }
        else if (ControllerCommand == PARSER_COMMAND_STOP)
        {
#ifdef STVID_VALID
          /*We don't do anything but close the Log files*/
         mpg2parserDumpParsingJobResults (&PARSER_Data_p->ParserJobResults, TRUE);
#endif
        }
    }
    /* The command queue has been flushed and the Parser State updated */
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
#endif
}

/*******************************************************************
Function:    mpg2par_NewReferencePicture

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Determines whether there is a startcode available to
             process, and whether it is complete.

Behavior:

Exceptions:  None

*******************************************************************/

static void mpg2par_NewReferencePicture(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t     * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    PARSER_ParsingJobResults_t  * ParsingJobResults_p = &PARSER_Data_p->ParserJobResults;
    VIDCOM_PictureGenericData_t * PictureGenericData_p = &ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData;
    MPEG2_PictureSpecificData_t * PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParsingJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    STVID_VideoParams_t         * VideoParams_p = (STVID_VideoParams_t *)&PictureGenericData_p->PictureInfos.VideoParams;


    /* Lock Access to Reference Pictures */
    STOS_SemaphoreWait(PARSER_Data_p->ReferencePicture.ReferencePictureSemaphore);

    /* Don't allow new reference if given picture is a 2nd field. The reference has already added at the 1st field. */
    /* Normally we have gone out with the previous test except if the decode of the 1st field has had an error.     */
    if ((VideoParams_p->PictureStructure != STVID_PICTURE_STRUCTURE_FRAME) &&
        (!PictureGenericData_p->IsFirstOfTwoFields))
    {
      /* UnLock Access to Reference Pictures */
      STOS_SemaphoreSignal(PARSER_Data_p->ReferencePicture.ReferencePictureSemaphore);
      /* Discard this reference */
      return;
    }


/*   If there was previously a reference picture, copy it to LastButOne*/
   if (PARSER_Data_p->ReferencePicture.LastRef.IsFree != TRUE)
   {
     /* References shift: add reference ! */
     PARSER_Data_p->ReferencePicture.LastButOneRef = PARSER_Data_p->ReferencePicture.LastRef;

     /* Previous reference present */
     PARSER_Data_p->ReferencePicture.MissingPrevious = FALSE;
   }
   else
   {
/*      Previous reference not present*/
      PARSER_Data_p->ReferencePicture.MissingPrevious = TRUE;
      PARSER_Data_p->ReferencePicture.LastButOneRef.IsFree = TRUE;
   }

/*    Store the information about this reference picture*/
   PARSER_Data_p->ReferencePicture.LastRef.IsFree = FALSE;
   PARSER_Data_p->ReferencePicture.LastRef.DecodingOrderFrameID =
          PictureGenericData_p->DecodingOrderFrameID;
   PARSER_Data_p->ReferencePicture.LastRef.TemporalReference = (S32)PictureSpecificData_p->temporal_reference;
   PARSER_Data_p->ReferencePicture.LastRef.ExtendedPresentationOrderPictureID =
          PictureGenericData_p->ExtendedPresentationOrderPictureID;


    /* UnLock Access to Reference Pictures */
    STOS_SemaphoreSignal(PARSER_Data_p->ReferencePicture.ReferencePictureSemaphore);
} /* end of mpg2par_NewReferencePicture() */


/*******************************************************************
Function:    mpg2parser_HasPreviousReference

Parameters:  ParserHandle

Returns:     TRUE - if current frame has a previous reference pic

Scope:       Private to parser

Purpose:     Determines whether there is a reference picture
             for the current picture.

Behavior:

Exceptions:  None

*******************************************************************/

BOOL mpg2parser_HasPreviousReference(const PARSER_Handle_t ParserHandle)
{
   PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
   MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

   return((PARSER_Data_p->ReferencePicture.MissingPrevious == TRUE) ? FALSE:TRUE);
}

/*******************************************************************
Function:    mpg2parser_ResetReferences

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Resets the reference picture structure

Behavior:

Exceptions:  None

*******************************************************************/

void mpg2parser_ResetReferences(const PARSER_Handle_t ParserHandle)
{
   PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
   MPG2ParserPrivateData_t     * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

   /* Lock Access to Reference Pictures */
   STOS_SemaphoreWait(PARSER_Data_p->ReferencePicture.ReferencePictureSemaphore);

   PARSER_Data_p->ReferencePicture.LastRef.IsFree = TRUE;
   PARSER_Data_p->ReferencePicture.LastButOneRef.IsFree = TRUE;
   PARSER_Data_p->ReferencePicture.MissingPrevious = TRUE;

   /* Lock Access to Reference Pictures */
   STOS_SemaphoreSignal(PARSER_Data_p->ReferencePicture.ReferencePictureSemaphore);
}




/*******************************************************************
Function:    mpg2par_UpdateReferences

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Builds the reference picture arrays in the Picture
             Generic Data structure from the reference information.

Behavior:

Exceptions:  None

*******************************************************************/

static void mpg2par_UpdateReferences(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t     * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    PARSER_ParsingJobResults_t  * ParsingJobResults_p = &PARSER_Data_p->ParserJobResults;
    VIDCOM_PictureGenericData_t * PictureGenericData_p = &ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData;
    STVID_VideoParams_t         * VideoParams_p = (STVID_VideoParams_t *)&PictureGenericData_p->PictureInfos.VideoParams;
    MPEG2_PictureSpecificData_t * PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParsingJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    VIDCOM_GlobalDecodingContextGenericData_t *GlobalDecodingContextGenericData_p = (VIDCOM_GlobalDecodingContextGenericData_t *)&ParsingJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *)ParsingJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    MPEG2_PictureDecodingContextSpecificData_t *PictureDecodingContextSpecificData_p = (MPEG2_PictureDecodingContextSpecificData_t *)ParsingJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p;

    MPG2ParserRefData_t         * ForwardRefData_p;
    MPG2ParserRefData_t         * BackwardRefData_p;
    MPG2ParserRefData_t           DummyRefData;
    U32                           counter;
    BOOL                          foundForward, foundBackward;

    for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; counter++)
    {
        PictureGenericData_p->ReferenceListP0[counter] = 0;
        PictureGenericData_p->ReferenceListB0[counter] = 0;
        PictureGenericData_p->ReferenceListB1[counter] = 0;
    }
    PictureGenericData_p->UsedSizeInReferenceListP0 = 0;
    PictureGenericData_p->UsedSizeInReferenceListB0 = 0;
    PictureGenericData_p->UsedSizeInReferenceListB1 = 0;


    GlobalDecodingContextGenericData_p->NumberOfReferenceFrames = 0;

    /*    Lock Access to Reference Pictures*/
    STOS_SemaphoreWait(PARSER_Data_p->ReferencePicture.ReferencePictureSemaphore);

    /*    Choose references*/
    switch (VideoParams_p->MPEGFrame)
    {
        case STVID_MPEG_FRAME_I:
        case STVID_MPEG_FRAME_P:
            /* For P picture: only forward prediction reference frames*/
            /* For I picture: referencs is useful for automatic error concealment decoder function*/
            DummyRefData.IsFree = TRUE;
            BackwardRefData_p = &DummyRefData;

            PictureGenericData_p->IsReference = TRUE;

            if ((VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) ||
                (PictureGenericData_p->IsFirstOfTwoFields))
            {
    /*             Frame or first field: take last reference*/
                ForwardRefData_p = &PARSER_Data_p->ReferencePicture.LastRef;
            }
            else
            {
    /*             Second field: take previous last reference -- don't take first field as reference ! */
                ForwardRefData_p = &PARSER_Data_p->ReferencePicture.LastButOneRef;
            }

    /*         TODO: Should reference frame be added for I Frame also?*/
            if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_P)
            {
                if (!ForwardRefData_p->IsFree)
                {
                    PictureGenericData_p->UsedSizeInReferenceListP0 = 1;
                    PictureGenericData_p->ReferenceListP0[0] = ForwardRefData_p->DecodingOrderFrameID;
                    PictureSpecificData_p->ForwardTemporalReferenceValue = ForwardRefData_p->TemporalReference;
                    PictureSpecificData_p->BackwardTemporalReferenceValue = -1;
                    GlobalDecodingContextGenericData_p->NumberOfReferenceFrames++;
                }
            }
            else
            {
                PictureSpecificData_p->ForwardTemporalReferenceValue  = -1;
                PictureSpecificData_p->BackwardTemporalReferenceValue = -1;
                GlobalDecodingContextGenericData_p->NumberOfReferenceFrames = 0;
            }

            PictureGenericData_p->IsDisplayBoundPictureIDValid = FALSE;

            if (! ForwardRefData_p->IsFree)
            {
    /*              If existing, the last reference picture to display has to be pushed to display as well as preceding pictures*/
                PictureGenericData_p->IsDisplayBoundPictureIDValid = FALSE;   /*PictureGenericData_p->IsDisplayBoundPictureIDValid = TRUE; MN*/
                PictureGenericData_p->DisplayBoundPictureID = ForwardRefData_p->ExtendedPresentationOrderPictureID;
            }
            break;

        case STVID_MPEG_FRAME_B:
        default:
/*          References for a B picture: backward and forward prediction*/
            BackwardRefData_p = &PARSER_Data_p->ReferencePicture.LastRef;
            ForwardRefData_p = &PARSER_Data_p->ReferencePicture.LastButOneRef;

            PictureGenericData_p->IsReference = FALSE;

            if (! ForwardRefData_p->IsFree)
            {
                PictureGenericData_p->UsedSizeInReferenceListB0 = 1;
                PictureGenericData_p->ReferenceListB0[0] = ForwardRefData_p->DecodingOrderFrameID;
                PictureSpecificData_p->ForwardTemporalReferenceValue = ForwardRefData_p->TemporalReference;
                GlobalDecodingContextGenericData_p->NumberOfReferenceFrames++;
            }

            if (! BackwardRefData_p->IsFree)
            {
                PictureGenericData_p->UsedSizeInReferenceListB1 = 1;
                PictureGenericData_p->ReferenceListB1[0] = BackwardRefData_p->DecodingOrderFrameID;
                PictureSpecificData_p->BackwardTemporalReferenceValue = BackwardRefData_p->TemporalReference;
                GlobalDecodingContextGenericData_p->NumberOfReferenceFrames++;
            }
            PictureGenericData_p->IsDisplayBoundPictureIDValid = FALSE; /*PictureGenericData_p->IsDisplayBoundPictureIDValid = TRUE; MN*/
            PictureGenericData_p->DisplayBoundPictureID = PictureGenericData_p->ExtendedPresentationOrderPictureID;
            break;
    }


    /*Manage the FullReferenceList array*/
    /*This is a list of all references for this frame.  Note that the list*/
    /*must be maintained such that a picture maintains the same index*/
    /*for its entire lifetime.*/
    /*TODO: Handle IsBrokenLinkIndexInReferenceFrame[] array*/

    /*Don't bother looking for nonexistent references*/
    foundForward  = ((ForwardRefData_p->IsFree) || (PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)) ? TRUE : FALSE;
    foundBackward = ((BackwardRefData_p->IsFree) || (PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)) ? TRUE : FALSE;


    if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_B)
    {
        if ((foundForward && foundBackward &&
             ((!GlobalDecodingContextSpecificData_p->HasGroupOfPictures) || (PictureDecodingContextSpecificData_p->closed_gop == 1))) ||
            ((foundForward || foundBackward) &&
             ((!GlobalDecodingContextSpecificData_p->HasGroupOfPictures) || (PictureDecodingContextSpecificData_p->closed_gop == 0))))
        {   /* Activate broken link flags when there is missing references or we have a missing GOP */
            for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; counter++)
            {
                PictureGenericData_p->IsBrokenLinkIndexInReferenceFrame[counter] = TRUE;
            }
            PictureDecodingContextSpecificData_p->IsBrokenLinkActive = TRUE;
        }
    }


    /* Loop through all entries to determine whether the references are already*/
    /* present in the array. Unmark all unused references.*/

    for (counter = 0; ((counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) /* &&
                        ((foundForward == FALSE) || (foundBackward == FALSE))*/); counter++)
    {
        if (PictureGenericData_p->IsValidIndexInReferenceFrame[counter] == TRUE)
        {
            if ((!foundForward) &&
                (ForwardRefData_p->DecodingOrderFrameID == PictureGenericData_p->FullReferenceFrameList[counter]))
            {
    /*            The forward reference is already in the List*/
                foundForward = TRUE;
            }
            else if ((!foundBackward) &&
                (BackwardRefData_p->DecodingOrderFrameID == PictureGenericData_p->FullReferenceFrameList[counter]))

            {
    /*             The backward reference is already in the List*/
                foundBackward = TRUE;
            }
            else
            {
    /*             This is a reference that is no longer needed*/
                PictureGenericData_p->IsValidIndexInReferenceFrame[counter] = FALSE;
            }
        }
    }


    /*If either of the references was not found, locate the first empty entries*/
    /*and use them*/

    for (counter = 0; ((counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) &&
                        ((foundForward == FALSE) || (foundBackward == FALSE))); counter ++)
    {
        if (PictureGenericData_p->IsValidIndexInReferenceFrame[counter] == FALSE)
        {
            if (!foundForward)
            {
                PictureGenericData_p->FullReferenceFrameList[counter] = ForwardRefData_p->DecodingOrderFrameID;
                PictureGenericData_p->IsValidIndexInReferenceFrame[counter] = TRUE;
                foundForward = TRUE;
                continue;
            }
            else if (!foundBackward)
            {
                PictureGenericData_p->FullReferenceFrameList[counter] = BackwardRefData_p->DecodingOrderFrameID;
                PictureGenericData_p->IsValidIndexInReferenceFrame[counter] = TRUE;
                foundBackward = TRUE;
            }
        }
    }

    /* Unlock Access to Reference Pictures */
    STOS_SemaphoreSignal(PARSER_Data_p->ReferencePicture.ReferencePictureSemaphore);

}

/*******************************************************************
Function:    mpg2par_FillParsingJobResult

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Fill ParsingJobResults_p Structure

Behavior:    Fill in ParsingJobResults_p. This function fills in all of the constant
             values in the ParsingJobResults_p structure (the ones that aren't actually
             parsed out of the stream), and values that need to be
             loaded post-parsinge, which include those which are
             based on whether a field/frame based value is
             overriding a sequence based value, and those related to the
             frame references. All other fields are loaded as they are
             parsed from the stream.

Exceptions:  None

*******************************************************************/
void mpg2par_FillParsingJobResult(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t     * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    PARSER_ParsingJobResults_t  * ParsingJobResults_p = &PARSER_Data_p->ParserJobResults;
    VIDCOM_PictureGenericData_t * PictureGenericData_p = &ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData;
    STGXOBJ_Bitmap_t            * Bitmap_p  = &PictureGenericData_p->PictureInfos.BitmapParams;
    STVID_VideoParams_t         * VideoParams_p  = &PictureGenericData_p->PictureInfos.VideoParams;
    MPEG2_PictureSpecificData_t * PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParsingJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    VIDCOM_GlobalDecodingContextGenericData_t *GlobalDecodingContextGenericData_p = &ParsingJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *)ParsingJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    STVID_SequenceInfo_t        *SequenceInfo_p = &GlobalDecodingContextGenericData_p->SequenceInfo;
    U32 PreviousExtendedTemporalReference = 0;
    U32 FrameRateForCalculation;
    U32 DispHorSize, DispVerSize, panandscan;
    S16 DifferenceBetweenTwoTemporalReferences;


/*    Parsing Job Status*/

    PARSER_Data_p->ParserJobResults.ParsingJobStatus.JobCompletionTime = STOS_time_now();


/*    Picture Generic Data*/


    PictureGenericData_p->RepeatFirstField = FALSE;
    PictureGenericData_p->RepeatProgressiveCounter = 0;

    if (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
    {
        if (SequenceInfo_p->ScanType != STVID_SCAN_TYPE_PROGRESSIVE)
        {
            if ((PictureSpecificData_p->progressive_frame) &&
                (PictureSpecificData_p->repeat_first_field))
            {
                PictureGenericData_p->RepeatFirstField = TRUE;
            }
        }
        else
        {
            if ((PictureSpecificData_p->repeat_first_field) &&
                (PictureSpecificData_p->progressive_frame) )
            {
                if (VideoParams_p->TopFieldFirst)
                {
                    /* Three identical progressive frames */
                    PictureGenericData_p->RepeatProgressiveCounter = 2;
                }
                else
                {
                    /* Two identical progressive frames */
                    PictureGenericData_p->RepeatProgressiveCounter = 1;
                }
            }
        }
    }


    if (GlobalDecodingContextSpecificData_p->HasSequenceDisplay)
    {
        DispHorSize = SequenceInfo_p->Width;
        DispVerSize = SequenceInfo_p->Height;
    }
    else
    {
        DispHorSize = 0;
        DispVerSize = 0;
    }

    for (panandscan = 0; panandscan < VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; panandscan++)
    {
        PictureGenericData_p->PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset = 0;
        PictureGenericData_p->PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset = 0;
        PictureGenericData_p->PanAndScanIn16thPixel[panandscan].DisplayHorizontalSize = DispHorSize * 16;
        PictureGenericData_p->PanAndScanIn16thPixel[panandscan].DisplayVerticalSize = DispVerSize * 16;
        PictureGenericData_p->PanAndScanIn16thPixel[panandscan].HasDisplaySizeRecommendation =
                        GlobalDecodingContextSpecificData_p->HasSequenceDisplay;
    }
    for (panandscan = 0; panandscan < PictureSpecificData_p->number_of_frame_centre_offsets; panandscan++)
    {
        PictureGenericData_p->PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset =
            PictureSpecificData_p->FrameCentreOffsets[panandscan].frame_centre_horizontal_offset;
        PictureGenericData_p->PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset =
            PictureSpecificData_p->FrameCentreOffsets[panandscan].frame_centre_vertical_offset;
    }
    PictureGenericData_p->NumberOfPanAndScan = PictureSpecificData_p->number_of_frame_centre_offsets;


    /*Set IsFirstOfTwoFields flag based on the current picture, and the previous*/
    /*state of IsFirstOfTwoFields*/

    if (PictureGenericData_p->IsFirstOfTwoFields == TRUE)
    {
        if (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)

        {
            /* Error - this is a frame based picture, and we're looking for a 2nd field*/
            PictureGenericData_p->PreviousPictureHasAMissingField = TRUE;
            PictureGenericData_p->MissingFieldPictureDecodingOrderFrameID = PictureGenericData_p->DecodingOrderFrameID;
            PictureGenericData_p->IsFirstOfTwoFields = FALSE;
        }
        else if (PictureSpecificData_p->temporal_reference != PictureSpecificData_p->PreviousTemporalReference)
        {
            /* Error - this is field based, but temporal ref doesn't match, so assume a first field*/
            PictureGenericData_p->PreviousPictureHasAMissingField = TRUE;
            PictureGenericData_p->MissingFieldPictureDecodingOrderFrameID = PictureGenericData_p->DecodingOrderFrameID;
            PictureGenericData_p->IsFirstOfTwoFields = TRUE;
        }
        else
        {
            /*No error - Last picture was first field*/
            PictureGenericData_p->PreviousPictureHasAMissingField = FALSE;
            PictureGenericData_p->IsFirstOfTwoFields = FALSE;
        }
    }
    else
    {
        /*If this is a field, and the temporal references don't match,*/
        /*we should expect the next picture to be a 2nd field. Otherwise,*/
        /*we're still looking for frames, or first fields.*/
        if ((GlobalDecodingContextSpecificData_p->IsBitStreamMPEG1 == FALSE) &&
            (VideoParams_p->PictureStructure != STVID_PICTURE_STRUCTURE_FRAME) &&
            ((PictureSpecificData_p->temporal_reference != PictureSpecificData_p->PreviousTemporalReference) ||
            (GlobalDecodingContextSpecificData_p->FirstPictureEver)))
        {
            /*This is a first field*/
            PictureGenericData_p->IsFirstOfTwoFields = TRUE;
            PictureGenericData_p->PreviousPictureHasAMissingField = FALSE;
        }
    }

    if ((VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_B) && (PictureSpecificData_p->PreviousMPEGFrame == STVID_MPEG_FRAME_B))
    {
        /* Compute temporal reference of this picture if it has no sense */
        DifferenceBetweenTwoTemporalReferences = PictureSpecificData_p->PreviousTemporalReference - PictureSpecificData_p->temporal_reference;
        DifferenceBetweenTwoTemporalReferences = (DifferenceBetweenTwoTemporalReferences > 0)? DifferenceBetweenTwoTemporalReferences:
            - DifferenceBetweenTwoTemporalReferences;
        /* ubnormal DifferenceBetweenTwoTemporalReferences means wrong temporal reference
            * expect at starting of a GOP when the prvious is IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE
            * compute the temporal reference based on the previous value */
        if ((DifferenceBetweenTwoTemporalReferences > 1) &&
            (PictureSpecificData_p->PreviousTemporalReference != IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE))
        {
            if ((VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) ||
                ((VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD) && (VideoParams_p->TopFieldFirst))||
                ((VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD) && !(VideoParams_p->TopFieldFirst)))
            {
                PictureSpecificData_p->temporal_reference = PictureSpecificData_p->PreviousTemporalReference +1;
            }
            else if (((VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD) && !(VideoParams_p->TopFieldFirst))||
                    ((VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD) && (VideoParams_p->TopFieldFirst)))
            {
                PictureSpecificData_p->temporal_reference = PictureSpecificData_p->PreviousTemporalReference;
            }
        }
    }

  /* Compute Extended Temporal Reference for this picture*/

  /* The Extended Temporal Reference is a constantly increasing ID, when looking at*/
  /* the pictures in DISPLAY*/

  /* This code tracks the ExtendedTemporalReference for the picture using*/
  /* these state variables:*/
  /*    GlobalDecodingContextSpecificData_p->ExtendedTemporalReference - calculated Ext Temporal Ref for current frame*/
  /*    GlobalDecodingContextSpecificData_p->TemporalReferenceOffset   - current offset, which is added to temporal reference*/
  /*                                       to calculate the Ext Temporal Ref*/
  /*    GlobalDecodingContextSpecificData_p->OneTemporalReferenceOverflowed - TRUE indicates that a reference frame with*/
  /*                                            a wrapped temporal reference has been*/
  /*                                            encountered. The Offset isn't updated until*/
  /*                                            the second picture with a wrapped temporal*/
  /*                                            reference is encountered.*/
  /*    GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference - max Ext Temporal Ref so far. Used*/
  /*                                                       for error recovery.*/



    /*Frame or first field picture*/

    if ((GlobalDecodingContextSpecificData_p->IsBitStreamMPEG1) ||
        (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) ||
        (PictureGenericData_p->IsFirstOfTwoFields))
    {
        /* Handle Initial Case for state variables*/
        if (GlobalDecodingContextSpecificData_p->FirstPictureEver)
        {
            GlobalDecodingContextSpecificData_p->ExtendedTemporalReference = PictureSpecificData_p->temporal_reference;
            PreviousExtendedTemporalReference = (U32)(-1);
            GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference = GlobalDecodingContextSpecificData_p->ExtendedTemporalReference;
        }
        else
        {
            /*Store current value, and calculate normal case value for Ext Temporal Reference*/
            PreviousExtendedTemporalReference = GlobalDecodingContextSpecificData_p->ExtendedTemporalReference;
            GlobalDecodingContextSpecificData_p->ExtendedTemporalReference  = GlobalDecodingContextSpecificData_p->TemporalReferenceOffset + PictureSpecificData_p->temporal_reference;
            GlobalDecodingContextSpecificData_p->PictureCounter++;
        }

        if(PictureSpecificData_p->temporal_reference == 0)
        {
            GlobalDecodingContextSpecificData_p->tempRefReachedZero = TRUE;
        }
        if (GlobalDecodingContextSpecificData_p->OneTemporalReferenceOverflowed)
        {
            if (VideoParams_p->MPEGFrame != PICTURE_CODING_TYPE_B)
            {
                GlobalDecodingContextSpecificData_p->GetSecondReferenceAfterOverflowing = TRUE;
            }
        }


        if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_B)
        {

         /*This is a B-frame. If the temporal reference has wrapped around, we should*/
         /*increment both the ext temporal reference value and the temporal reference*/
         /*offset by the size of the wraparound. Clear the Overflow flag, in case a*/
         /*ref picture already overflowed.*/

            if (((S32)(PreviousExtendedTemporalReference - GlobalDecodingContextSpecificData_p->ExtendedTemporalReference)) >= (TEMPORAL_REFERENCE_MODULO - 1))
            {
                GlobalDecodingContextSpecificData_p->ExtendedTemporalReference += TEMPORAL_REFERENCE_MODULO;
                GlobalDecodingContextSpecificData_p->TemporalReferenceOffset   += TEMPORAL_REFERENCE_MODULO;
                GlobalDecodingContextSpecificData_p->OneTemporalReferenceOverflowed = FALSE;
            }
        }
        else
        {
            /* I or P picture: reference picture */
            if (((S32)(PreviousExtendedTemporalReference - GlobalDecodingContextSpecificData_p->ExtendedTemporalReference)) >0)
            {
                GlobalDecodingContextSpecificData_p->ExtendedTemporalReference += TEMPORAL_REFERENCE_MODULO;
                GlobalDecodingContextSpecificData_p->OneTemporalReferenceOverflowed = TRUE;
                if (((PictureSpecificData_p->PreviousTemporalReference == (TEMPORAL_REFERENCE_MODULO - 1)) ||
                        (GlobalDecodingContextSpecificData_p->GetSecondReferenceAfterOverflowing)) &&
                            (GlobalDecodingContextSpecificData_p->tempRefReachedZero))
                {
                    GlobalDecodingContextSpecificData_p->TemporalReferenceOffset   += TEMPORAL_REFERENCE_MODULO;
                    GlobalDecodingContextSpecificData_p->tempRefReachedZero = FALSE;
                    GlobalDecodingContextSpecificData_p->OneTemporalReferenceOverflowed = FALSE;
                }
                GlobalDecodingContextSpecificData_p->GetSecondReferenceAfterOverflowing = FALSE;
            }

            if (GlobalDecodingContextSpecificData_p->ExtendedTemporalReference < GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference)
            {
                /*ExtendedTemporalReference of this reference picture is going back in the past. This should*/
                /*not happen. This could happen because temporal_reference was spoiled. This error case would*/
                /*lock the current architecture, as the new reference could be rejected by the display because*/
                /*too late, but still kept RESERVED as a reference. If the display was locked, it may never*/
                /*reach condition for unlock and the decoder may never find a buffer to decode next B, in 3 buffers mode.*/
                /*This case was experienced with spoiled streams and led to the following parsing :*/
                /*P754-B752-B753-P757-B750-P754-B752-B753-P757*/
                /*______________errors^^^^-^^^^*/

                GlobalDecodingContextSpecificData_p->TemporalReferenceOffset +=
                    (GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference - GlobalDecodingContextSpecificData_p->ExtendedTemporalReference - 1);

            /* The case above would now lead to the following parsing :*/
            /* P754-B752-B753-P757-B750-P758-B7xx-B7xx-P757*/
            /* ______________errors^^^^-^^^^*/

                GlobalDecodingContextSpecificData_p->ExtendedTemporalReference = GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference + 1;
            }

    /*         Manage greatest ExtendedTemporalReference of reference pictures*/
            if (GlobalDecodingContextSpecificData_p->ExtendedTemporalReference > GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference)
            {
                GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference = GlobalDecodingContextSpecificData_p->ExtendedTemporalReference;
            }
        }
    }
    else
    {
      /*This is the second field of a picture. It has the same temporal reference as the*/
      /*other field, so we need to increment the ext temporal reference. Also increment*/
      /*the temporal reference offset to propagate the increment to all following pictures*/
/* LD 28/08/06 : not needed --> Issue with FieldPS stream
      GlobalDecodingContextSpecificData_p->ExtendedTemporalReference++;
      GlobalDecodingContextSpecificData_p->TemporalReferenceOffset++;
*/
    }



/*   Compute Interpolated PTS, if required*/


    if (VideoParams_p->FrameRate != 0)
    {
        FrameRateForCalculation = VideoParams_p->FrameRate;
    }
    else
    {
        /* To avoid division by 0 */
        FrameRateForCalculation = 50000;
    }


    if (VideoParams_p->PTS.Interpolated)
    {
        if (GlobalDecodingContextSpecificData_p->FirstPictureEver)
        {
            /* Initialise first PTS to 0, or to a value in case of open GOP (first ExtendedTemporalReference not 0) */
            GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration = GlobalDecodingContextSpecificData_p->ExtendedTemporalReference *
                    ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation; /* Supposing 2 fields */
            VideoParams_p->PTS.PTS =  GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration;
            VideoParams_p->PTS.PTS33 = FALSE;
        }
        else
        {
/*          Store the previous PTS before it is overwritten*/
            U32 OldPTS = VideoParams_p->PTS.PTS;
            U32 NbFieldsDisplay;
            U32 TimeDisplayPreviousPicture;


/*          Calculate the number of fields the previous picture was displayed*/
            NbFieldsDisplay = 1;
            if (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
            {
                NbFieldsDisplay++;
            }
            if (PictureGenericData_p->RepeatFirstField == TRUE)
            {
                NbFieldsDisplay++;
            }

            NbFieldsDisplay *= (PictureGenericData_p->RepeatProgressiveCounter + 1);

/*          Calculate the length of time the previoud picture was displayed*/
            TimeDisplayPreviousPicture = ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) *
                                            NbFieldsDisplay / FrameRateForCalculation / 2;


/*          If this is a B following a B, or the second of a 2 field picture*/

            if ((PreviousExtendedTemporalReference == (GlobalDecodingContextSpecificData_p->ExtendedTemporalReference - 1)) ||
                (PreviousExtendedTemporalReference == GlobalDecodingContextSpecificData_p->ExtendedTemporalReference))
            {
/*             No reordering*/
                VideoParams_p->PTS.PTS += TimeDisplayPreviousPicture;

                if ((GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration != 0) &&
                    (PreviousExtendedTemporalReference == GlobalDecodingContextSpecificData_p->ExtendedTemporalReference))
                {
                    GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration += TimeDisplayPreviousPicture;
                }
            }

/*          If this is a B following a reference picture*/

            else if (PreviousExtendedTemporalReference > GlobalDecodingContextSpecificData_p->ExtendedTemporalReference)
            {
/*             To be re-ordered backwards*/
                VideoParams_p->PTS.PTS -= GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration;
                GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration = 0;
            }

/*          This is a reference picture*/

            else
            {
             /*To be re-ordered forwards*/
             /*GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration is what we suppose to be the*/
             /*duration of the display of the B between the references (not yet arrived so we can't guess !).*/
             /*We supppose they will not be repeated, and have the same picture structure as the current*/
             /*picture. This error will be substracted later on...*/
                if (PictureSpecificData_p->PreviousMPEGFrame == STVID_MPEG_FRAME_B)
                        /* Previous picture is not a reference */
                {

                    VideoParams_p->PTS.PTS += NbFieldsDisplay *
                            ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation / 2;
                    GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration =
                            (GlobalDecodingContextSpecificData_p->ExtendedTemporalReference - PreviousExtendedTemporalReference - 2) *
                            ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation;
                            /* Supposing 2 fields */
                }
                else
                {
                    GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration =
                            (GlobalDecodingContextSpecificData_p->ExtendedTemporalReference - PreviousExtendedTemporalReference - 1) *
                            ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation;
                            /* Supposing 2 fields */
                }

           /* if (Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE)*/
                         /* Current picture is a frame picture */
           /* {*/
                 /* Supposing 2 fields */
           /*      VIDDEC_Data_p->StartCodeProcessing.CalculatedPictureDisplayDuration =*/
           /*      VIDDEC_Data_p->StartCodeProcessing.CalculatedPictureDisplayDuration * 2;*/
           /* }*/
                VideoParams_p->PTS.PTS += TimeDisplayPreviousPicture + GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration;
            }
            if (VideoParams_p->PTS.PTS < OldPTS)
            {
/*                33th bit to toggle*/
                VideoParams_p->PTS.PTS33 = (!VideoParams_p->PTS.PTS33);
            }
        }
        VideoParams_p->PTS.IsValid = TRUE;
    } /* End of compute PTS interpolated */
    else
    {
        if (GlobalDecodingContextSpecificData_p->FirstPictureEver)
        {
         /* Initialise first PTS to 0, or to a value in case of open GOP (first ExtendedTemporalReference not 0) */
            GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration = GlobalDecodingContextSpecificData_p->ExtendedTemporalReference *
                    ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation; /* Supposing 2 fields */
        }
        else
        {
            if (PictureSpecificData_p->PreviousMPEGFrame == STVID_MPEG_FRAME_B)
                /* Previous picture is not a reference */
            {
                GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration =
                        (GlobalDecodingContextSpecificData_p->ExtendedTemporalReference - PreviousExtendedTemporalReference - 2) *
                        ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation;
                        /* Supposing 2 fields */
            }
            else
            {
                GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration =
                        (GlobalDecodingContextSpecificData_p->ExtendedTemporalReference - PreviousExtendedTemporalReference - 1) *
                        ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation;
                        /* Supposing 2 fields */
            }
/*         GlobalDecodingContextSpecificData_p->CalculatedPictureDisplayDuration = 0;*/
        }
    }

/*   Decoding Order Frame ID*/
    PictureGenericData_p->DecodingOrderFrameID = PARSER_Data_p->CurrentPictureDecodingOrderFrameID;

    /* Save picture information for next picture */
    if (VideoParams_p->MPEGFrame != STVID_MPEG_FRAME_B) /* reference */
    {
        PictureSpecificData_p->LastButOneReferenceTemporalReference = PictureSpecificData_p->LastReferenceTemporalReference;
        PictureSpecificData_p->LastReferenceTemporalReference = PictureSpecificData_p->temporal_reference;
    }
    PictureSpecificData_p->PreviousTemporalReference = PictureSpecificData_p->temporal_reference;

/*   Extended Presentation Order Picture ID*/
    PictureGenericData_p->ExtendedPresentationOrderPictureID.ID = GlobalDecodingContextSpecificData_p->ExtendedTemporalReference - GlobalDecodingContextSpecificData_p->TemporalReferenceOffset;
    PictureGenericData_p->ExtendedPresentationOrderPictureID.IDExtension = GlobalDecodingContextSpecificData_p->TemporalReferenceOffset;
    PictureGenericData_p->IsExtendedPresentationOrderIDValid = TRUE;
    mpg2par_UpdateReferences(ParserHandle);
    GlobalDecodingContextGenericData_p->MaxDecFrameBuffering                    = 2;    /* Number of reference frames (this number +2 gives the number of frame buffers needed for decode */

/*   If a Reference Picture, update the ReferencePicture structure*/
    if ((VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_I) || (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_P))
    {
        mpg2par_NewReferencePicture(ParserHandle);
    }

/*    Asynchronous Commands are all FALSE for MPEG streams*/
    PictureGenericData_p->AsynchronousCommands.FlushPresentationQueue = FALSE;
    PictureGenericData_p->AsynchronousCommands.FreezePresentationOnThisFrame = FALSE;
    PictureGenericData_p->AsynchronousCommands.ResumePresentationOnThisFrame = FALSE;

   /* Current parser does not handle Presentation asynchronous commands */
    PictureGenericData_p->AsynchronousCommands.FlushPresentationQueue = FALSE;
    PictureGenericData_p->AsynchronousCommands.FreezePresentationOnThisFrame = FALSE;
    PictureGenericData_p->AsynchronousCommands.ResumePresentationOnThisFrame = FALSE;

    PictureGenericData_p->DecodeStartTime = 0;
    PictureGenericData_p->IsDecodeStartTimeValid = FALSE;
    PictureGenericData_p->PresentationStartTime = 0;
    PictureGenericData_p->IsPresentationStartTimeValid = FALSE;

    if ((PARSER_Data_p->DiscontinuityDetected) &&
        (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData.ParsingError < VIDCOM_ERROR_LEVEL_BAD_DISPLAY))
    {
        PictureGenericData_p->ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
    }


/*   Initialize the Bitmap Parameters*/

    Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420;
    Bitmap_p->BitmapType = STGXOBJ_BITMAP_TYPE_MB;
    Bitmap_p->PreMultipliedColor = FALSE;
    Bitmap_p->Offset = 0;
    Bitmap_p->Data1_p = NULL;
    Bitmap_p->Size1 = 0;
    Bitmap_p->Data2_p = NULL;
    Bitmap_p->Size2 = 0;
    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
    Bitmap_p->BigNotLittle = FALSE;


   /* H.264 only. Not set by this parser, so I'm setting this to the value*/
   /* I see from the vidcodec parser.*/
    VideoParams_p->CompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
    VideoParams_p->DecimationFactors = STVID_DECIMATION_FACTOR_NONE; /*modified MN*/

    GlobalDecodingContextGenericData_p->FrameCropInPixel.LeftOffset = 0;
    GlobalDecodingContextGenericData_p->FrameCropInPixel.RightOffset = 0;
    GlobalDecodingContextGenericData_p->FrameCropInPixel.TopOffset = 0;
    GlobalDecodingContextGenericData_p->FrameCropInPixel.BottomOffset = 0;

   /* TODO_CL update error code returned */
    GlobalDecodingContextGenericData_p->ParsingError = VIDCOM_ERROR_LEVEL_NONE;

/*   Stream ID is passed in as a parameter to parser_start.*/
    SequenceInfo_p->StreamID = PARSER_Data_p->ParserGlobalVariable.StreamID;


   /*PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;*/
}

/*******************************************************************
Function:    mpg2par_PointToNextSC

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Points NextSCEntry to the next entry.

Behavior:    Takes into account the SC List Circular buffering.

Exceptions:  None

*******************************************************************/
void mpg2par_PointToNextSC(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
   PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
   MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

    PARSER_Data_p->SCBuffer.NextSCEntry ++;

#ifdef DEBUG_STARTCODE
   if (indSC >= MAX_TRACES_SC)
   {
      indSC = 0;
   }
   /*sprintf (&tracesSC[indSC][0], "NextSC=0x%08x SCloop=0x%08x SCWrite=0x%08x\n", PARSER_Data_p->SCBuffer.NextSCEntry, PARSER_Data_p->SCBuffer.SCList_Loop_p, PARSER_Data_p->SCBuffer.SCList_Write_p);*/
/*   Loop[indSC] = PARSER_Data_p->SCBuffer.SCList_Loop_p;
   NexSC[indSC] = PARSER_Data_p->SCBuffer.NextSCEntry;
   Write[indSC] = PARSER_Data_p->SCBuffer.SCList_Write_p;
   indSC++;
   nbSC++;
*/   if (PARSER_Data_p->SCBuffer.SCList_Loop_p > PARSER_Data_p->SCBuffer.SCList_Stop_p)
   {
/*       printf ("nbSC : %d   Loop 0x%08x > Stop 0x%08x\n", nbSC, PARSER_Data_p->SCBuffer.SCList_Loop_p, PARSER_Data_p->SCBuffer.SCList_Stop_p);*/
   }
#endif

   if((PARSER_Data_p->SCBuffer.NextSCEntry == PARSER_Data_p->SCBuffer.SCList_Loop_p) &&
      (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p))
   {
#ifdef STVID_PARSER_DO_NOT_LOOP_ON_STREAM
      STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PARSER : Reached end of Stream"));
      PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Write_p;
#else
        /* Reached top of SCList */
      PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Start_p;
/*        printf("PARSER : Reached end of Stream. Looping to start.\n"); */
#endif
   }
#ifdef ST_speed
    PARSER_Data_p->LastSCAdd_p   = (void*)(PARSER_Data_p->StartCode.StartAddress_p);
#endif /* ST_speed */
}



/*******************************************************************
Function:    mpg2par_GetCompleteStartCode

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Determines whether there is a startcode available to
             process, and whether it is complete.

Behavior:

Exceptions:  None

*******************************************************************/

void mpg2par_GetNextCompleteStartCode(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    PARSER_ParsingJobResults_t  * ParsingJobResults_p = &PARSER_Data_p->ParserJobResults;
    VIDCOM_PictureGenericData_t * PictureGenericData_p = &ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData;
    STVID_VideoParams_t         * VideoParams_p = (STVID_VideoParams_t *)&ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *)ParsingJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    STVID_TimeCode_t            *timecode_p = &ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.TimeCode;
    BOOL SCFound;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STFDMA_SCEntry_t          * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
    U32                       BitBufferLoop;
#ifdef STVID_TRICKMODE_BACKWARD
    STFDMA_SCEntry_t            * CheckSC;
    BOOL                          NotifyBitBufferFullyParsed;
#endif /*STVID_TRICKMODE_BACKWARD*/
    BOOL IsLastSC = FALSE;
    /* Test if a new start-code is available */

    STOS_SemaphoreWait(PARSER_Data_p->InjectSharedAccess);

    if (PARSER_Data_p->StartCode.IsPending == FALSE)
    {
        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

        if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p)
        {
            SCFound = FALSE;

            /* search for available SC in SC+PTS buffer : */
            while((PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p) && (SCFound == FALSE))
            {
                CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry,&VirtualMapping);

                if(!IsPTS(CPUNextSCEntry_p))
                {
                    SCFound = TRUE;
                    /* Get the Start Code itself */
                    PARSER_Data_p->StartCode.Value = GetSCVal(CPUNextSCEntry_p);
                    /* Set the Parser Read Pointer in ES Bit Buffer on the start address of the start code */
                    PARSER_Data_p->StartCode.StartAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
                    /* A valid current start code is available */
                    PARSER_Data_p->StartCode.IsPending = TRUE;
                    /* The parser must now look for the next start code to know if this one is fully in the bit buffer */
                    PARSER_Data_p->StartCode.IsInBitBuffer = FALSE;

#ifdef DEBUG_STARTCODE
                    if (previousSC != 0 && abs(PARSER_Data_p->StartCode.StartAddress_p - previousSC) > 100000)
                    {
                        if (indSC >= MAX_TRACES_SC)
                        {
                            indSC = 0;
                        }
                        sprintf (&tracesSC[indSC][0], "previous SC %d new SC %d diff %d\n", previousSC, PARSER_Data_p->StartCode.StartAddress_p,
                                abs(PARSER_Data_p->StartCode.StartAddress_p - previousSC));
                        indSC++;
                        /*FlushSCBuffer();*/
                    }
                    previousSC = PARSER_Data_p->StartCode.StartAddress_p;
#endif
                    if (PARSER_Data_p->StartCode.Value == PICTURE_START_CODE)
                    {
                        mpg2par_SearchForAssociatedPTS(ParserHandle);
#ifdef WA_SYNC_ON_PRIVIOUS_PTS
                        if (PARSER_Data_p->PTSAvailableForNextPicture)
                        {
                            /*ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsPTSValid = true;*/
                            CPTS = PARSER_Data_p->PTSForNextPicture;
                            PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
                            CPTS.IsValid = TRUE;
                            CPTS.Interpolated = FALSE;
                        }
                        else
                        {
                            /*ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsPTSValid = FALSE;*/ /* Set to TRUE as soon as a PTS is detected */
                            CPTS.IsValid = FALSE;
                            CPTS.Interpolated = TRUE;
                            CPTS.PTS = 0;
                            CPTS.PTS33 = FALSE;
                        }
#else
                        if (PARSER_Data_p->PTSAvailableForNextPicture)
                        {
                            /*ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsPTSValid = true;*/
                            VideoParams_p->PTS = PARSER_Data_p->PTSForNextPicture;
                            PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
                            VideoParams_p->PTS.IsValid = TRUE;
                            VideoParams_p->PTS.Interpolated = FALSE;
                        }
                        else
                        {
                            /*ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsPTSValid = FALSE;*/ /* Set to TRUE as soon as a PTS is detected */
                            VideoParams_p->PTS.IsValid = FALSE;
                            VideoParams_p->PTS.Interpolated = TRUE;
                            VideoParams_p->PTS.PTS = 0;
                            VideoParams_p->PTS.PTS33 = FALSE;
                        }
#endif
                    }

                    /* If this is not a picture data startcode, we need to check whether we can flush out a parsed picture.
                    * Otherwise, we have to wait for the startcode _after_ this one to arrive before the picture is flushed.*/

                    if ( ((PARSER_Data_p->StartCode.Value < FIRST_SLICE_START_CODE) ||
                          (PARSER_Data_p->StartCode.Value > GREATEST_SLICE_START_CODE))  &&
                          (PARSER_Data_p->SCParserState == ES_state_picture_data))
                    {
                        /* Store end of picture pointer */
                        if (PARSER_Data_p->StartCode.StartAddress_p-4<PARSER_Data_p->BitBuffer.ES_Start_p)
                        {
                            BitBufferLoop= PARSER_Data_p->StartCode.StartAddress_p-PARSER_Data_p->BitBuffer.ES_Start_p;
                            PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData.BitBufferPictureStopAddress = PARSER_Data_p->BitBuffer.ES_Stop_p-(3-BitBufferLoop);
                        }
                        else
                        {
                            PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData.BitBufferPictureStopAddress = PARSER_Data_p->StartCode.StartAddress_p - 4;
                        }

#ifdef WA_SYNC_ON_PRIVIOUS_PTS
                            VideoParams_p->PTS.IsValid = LastPTS.IsValid;
                            VideoParams_p->PTS.Interpolated = LastPTS.Interpolated;
                            VideoParams_p->PTS.PTS = LastPTS.PTS;
                            VideoParams_p->PTS.PTS33 = LastPTS.PTS33;
#endif /* WA_SYNC_ON_PRIVIOUS_PTS */

                        mpg2par_FillParsingJobResult(ParserHandle);
#ifdef STVID_VALID
                        mpg2parserDumpParsingJobResults (ParsingJobResults_p, FALSE);
#endif
                        /*                   Only send JOB COMPLETED event:
                        *  - if the frame is of the type that was requested.
                         * - if error recovery mode not FULL and the picture is reference */
#if defined(STVID_TRICKMODE_BACKWARD) && !defined(DVD_SECURED_CHIP)
                        if (PARSER_Data_p->Backward)
                        {
                            CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                            /* to test if next Sc exist: end of buffer */
                            if (((U32)((U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p))) > 0)
                            {
                                PARSER_Data_p->OutputCounter += (U32)((U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p));
                                PARSER_Data_p->ParserJobResults.DiffPSC = PARSER_Data_p->OutputCounter;
                            }

                        }
#endif /* #if defined(STVID_TRICKMODE_BACKWARD) && !defined(DVD_SECURED_CHIP) */

#ifdef STVID_TRICKMODE_BACKWARD
                        if ((((PARSER_Data_p->GetPictureParams.GetRandomAccessPoint &&
                               ((VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_I) ||
                                ((PARSER_Data_p->ParserGlobalVariable.ErrorRecoveryMode != PARSER_ERROR_RECOVERY_MODE1) &&
                                 (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_P)))) ||
                              ((PARSER_Data_p->SkipMode == STVID_DECODED_PICTURES_I) &&
                               (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_I)) ||
                              (!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint))) &&
                             (!PARSER_Data_p->Backward))
                            ||
                            (((VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_I)) &&
                             (PARSER_Data_p->Backward) &&
                             (!PARSER_Data_p->IsBitBufferFullyParsed)))
#else
                        if (!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint) ||
                            ((VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_I) ||
                             ((PARSER_Data_p->ParserGlobalVariable.ErrorRecoveryMode != PARSER_ERROR_RECOVERY_MODE1) &&
                              (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_P))))
#endif
                        {
                            PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;
                            if ((VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_I) ||
                                ((PARSER_Data_p->ParserGlobalVariable.ErrorRecoveryMode != PARSER_ERROR_RECOVERY_MODE1) &&
                                 (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_P)))
                            {
                                PARSER_Data_p->DiscontinuityDetected = FALSE;
                            }

                            STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->ParserJobResults));
/*                            time_end_PARS = STOS_time_now();*/
                        }
                        else
                        {
#ifdef STVID_TRICKMODE_BACKWARD
                            if (!PARSER_Data_p->Backward)
#endif
                            {
                            /* WARNING, at this stage only picture's start & stop addresses in the bit buffer are valid data, all remaingin of the picture's data might be unrelevant because picture parsing is not complete */
                                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData));
                            }
                        }

/*                   Reset the Per-picture flags*/

                        GlobalDecodingContextSpecificData_p->NewSeqHdr = FALSE;
                        GlobalDecodingContextSpecificData_p->NewQuantMxt = FALSE;
                        GlobalDecodingContextSpecificData_p->StreamTypeChange = FALSE;
                        GlobalDecodingContextSpecificData_p->HasSequenceDisplay = FALSE;
                        GlobalDecodingContextSpecificData_p->FirstPictureEver = FALSE;
                        VideoParams_p->PTS.Interpolated = TRUE;
                        PictureGenericData_p->ParsingError = VIDCOM_ERROR_LEVEL_NONE;

                   /*Increment the Frame based counters if this is a progressive picture,*/
                   /* or if this is the second field of a field based picture*/

                        if (((VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)&&(PARSER_Data_p->PreviousPictureIsAvailable == TRUE))||
                              (PictureGenericData_p->IsFirstOfTwoFields == FALSE))
                        {
                            /*Increment Decoding Frame ID*/
                            PARSER_Data_p->CurrentPictureDecodingOrderFrameID++;

                            /*Increment interpolated timecode*/
                            timecode_p->Interpolated = TRUE;
                            PARSER_Data_p->PreviousPictureIsAvailable = TRUE;
                            if (timecode_p->Interpolated)
                            {
                                if (GlobalDecodingContextSpecificData_p->FirstPictureEver)
                                {
                                    timecode_p->Frames = 0;
                                    timecode_p->Seconds = 0;
                                    timecode_p->Minutes = 0;
                                    timecode_p->Hours = 0;
                                }
                                else
                                {
                                    vidcom_IncrementTimeCode(timecode_p,
                                                            VideoParams_p->FrameRate,
                                                            GlobalDecodingContextSpecificData_p->HasGroupOfPictures );
                                }
                            } /* End compute time code */
                        }
                        else  if (PictureGenericData_p->IsFirstOfTwoFields == TRUE )
                        {
                            PARSER_Data_p->PreviousPictureIsAvailable = FALSE;

                        }
                        else
                        {
                            PARSER_Data_p->CurrentPictureDecodingOrderFrameID++;

                        /*Increment interpolated timecode*/
                        /*TODO: See decoder code for way to update timecode*/
                            timecode_p->Interpolated = TRUE;
                            PARSER_Data_p->PreviousPictureIsAvailable = TRUE;

                        }
                  }
#ifdef WA_SYNC_ON_PRIVIOUS_PTS
                LastPTS.IsValid = CPTS.IsValid;
                LastPTS.Interpolated = CPTS.Interpolated;
                LastPTS.PTS = CPTS.PTS;
                LastPTS.PTS33 = CPTS.PTS33;
#endif /* WA_SYNC_ON_PRIVIOUS_PTS */



#ifdef STVID_DEBUG_GET_STATISTICS
              /* I hate to reset this on every startcode, but I don't see*/
              /* another way to indicate that a startcode has been received*/
              /* since the last timeout occurred.*/
                    PARSER_Data_p->Statistics.SCListTimeout = 0;

#endif
                }
                else
                {
                    mpg2par_SavePTSEntry(ParserHandle, CPUNextSCEntry_p);
                }
                /*Make ScannedEntry_p point on the next entry in SC List*/
                mpg2par_PointToNextSC(ParserHandle);

#ifdef DEBUG_STARTCODE
                if (indSC >= MAX_TRACES_SC)
                {
                    indSC = 0;
                }
                nbSC++;
                if (nbSC == 10)
                {
                    DMATransferTime = STOS_time_now();
                    sprintf (&tracesSC[indSC][0], "%"FORMAT_SPEC_OSCLOCK"d;%d;%d;%d;%d;%d;\n",
                                        DMATransferTime * OSCLOCK_T_MILLE/ST_GetClocksPerSecond(),
                                        PARSER_Data_p->BitBuffer.ES_Write_p,
                                        PARSER_Data_p->BitBuffer.ES_DecoderRead_p,
                                        PARSER_Data_p->SCBuffer.SCList_Write_p,
                                        PARSER_Data_p->SCBuffer.NextSCEntry,
                                        PARSER_Data_p->SCBuffer.SCList_Loop_p);
                    indSC++;
                    nbSC = 0;
                }
#endif
                STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
            }
            if ((PARSER_Data_p->SCBuffer.NextSCEntry == PARSER_Data_p->SCBuffer.SCList_Write_p) &&
                (PARSER_Data_p->StartCode.Value == RESERVED_START_CODE_6) &&
                (SCFound == TRUE))
            {
                /* This is the last SC ONLY IF IT'S A DISCONTINUITY SC */
                IsLastSC = TRUE;
            }
        }
#ifdef STVID_DEBUG_GET_STATISTICS
        else
        {
         /*No startcodes waiting. In general, we'll just return, but we'll also */
         /*increment the counter each time the timeout rolls over.*/

            if ((PARSER_Data_p->Statistics.SCListTimeout != 0) &&
                (STOS_time_after(time_now(), PARSER_Data_p->Statistics.SCListTimeout) != 0))
            {
                PARSER_Data_p->Statistics.DecodePbHeaderFifoEmpty ++;
                PARSER_Data_p->Statistics.SCListTimeout = 0;
            }

            if (PARSER_Data_p->Statistics.SCListTimeout == 0)
            {
/*             We're not currently waiting for a timeout, so set up a new one*/
                PARSER_Data_p->Statistics.SCListTimeout = STOS_time_plus(time_now(), MAX_WAIT_SCLIST);
            }
        }
#endif /* STVID_DEBUG_GET_STATISTICS */
    }

#ifdef STVID_TRICKMODE_BACKWARD
        if ((PARSER_Data_p->Backward) && (!PARSER_Data_p->IsBitBufferFullyParsed))
        {
            NotifyBitBufferFullyParsed = FALSE;

            CheckSC = PARSER_Data_p->SCBuffer.NextSCEntry;
            if (CheckSC != PARSER_Data_p->SCBuffer.SCList_Write_p)
            {
                CheckSC++;
                if (CheckSC == PARSER_Data_p->SCBuffer.SCList_Write_p)
                    NotifyBitBufferFullyParsed = TRUE;
            }
            else
            {
                NotifyBitBufferFullyParsed = TRUE;
            }

            if (NotifyBitBufferFullyParsed)
            {
                PARSER_Data_p->IsBitBufferFullyParsed = TRUE;
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

                /* Bit Buffer is fully parsed: stop parsing ... */
                PARSER_Data_p->StopParsing = TRUE;
            }
        }
#endif

    /* Look if the Start Code is fully in bit buffer, now */
    if (PARSER_Data_p->StartCode.IsPending == TRUE)
    {
        if (IsLastSC)
        {
            /* This is the last start code in the list; so no need to check the next one. */
            PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
            PARSER_Data_p->StartCode.StopAddress_p = PARSER_Data_p->StartCode.StartAddress_p;

#ifdef STVID_DEBUG_GET_STATISTICS
            PARSER_Data_p->Statistics.DecodeStartCodeFound ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

            STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
        }
        else
        {
            STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

            if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p)
            {
                /* Saves the NextSCEntry before proceeding over the SC+PTS buffer */
                PARSER_Data_p->SCBuffer.SavedNextSCEntry_p = PARSER_Data_p->SCBuffer.NextSCEntry;
                SCFound = FALSE;
                /* search for available SC in SC+PTS buffer : */
                while((PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p) && (SCFound == FALSE))
                {
                    CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry,&VirtualMapping);
                    if(!IsPTS(CPUNextSCEntry_p))
                    {
                        /* There is a startcode present after the current one, which
                        * indicates that a complete startcode is present. */

                        SCFound = TRUE;
                        PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
                        PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
#ifdef STVID_DEBUG_GET_STATISTICS
                        PARSER_Data_p->Statistics.DecodeStartCodeFound ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

                        mpg2par_PointToNextSC(ParserHandle);
                        STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
                    }
                    else
                    {
                        mpg2par_SavePTSEntry(ParserHandle, CPUNextSCEntry_p);
                        mpg2par_PointToNextSC(ParserHandle);
                    }
                }
                /* Restores the NextSCEntry after proceeding over the SC+PTS buffer */
                PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SavedNextSCEntry_p;
            }
        }
    }

    /* inform inject module about the SC-list read progress */
    VIDINJ_TransferSetSCListRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, PARSER_Data_p->SCBuffer.NextSCEntry);

    /* If we are in ES_state_inactive state and the current start code isn't a SEQUENCE_HEADER_CODE,
     * ==> we have to inform inject module about the read progress
     * This is done to allow new data transfert when there is no SEQUENCE_HEADER_CODE start code in the SC-list */

    if ((PARSER_Data_p->SCParserState == ES_state_inactive) &&
        (PARSER_Data_p->StartCode.IsPending == TRUE) && (PARSER_Data_p->StartCode.IsInBitBuffer = TRUE) &&
        (PARSER_Data_p->StartCode.Value != SEQUENCE_HEADER_CODE))
    {
        /* Stores the decoder read pointer locally for further computation of the bit buffer level */
        PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->StartCode.StopAddress_p;

        /* Inform the FDMA on the read pointer of the parser in Bit Buffer */
        VIDINJ_TransferSetESRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, PARSER_Data_p->StartCode.StopAddress_p);
    }
    STOS_SemaphoreSignal(PARSER_Data_p->InjectSharedAccess);
}


/*******************************************************************
Function:    mpg2par_ProcessCompleteStartCode

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:

Behavior:

Exceptions:  None

*******************************************************************/
void mpg2par_ProcessCompleteStartCode (const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t     * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    PARSER_ParsingJobResults_t  * ParsingJobResults_p = &PARSER_Data_p->ParserJobResults;
    STVID_VideoParams_t         * VideoParams_p = (STVID_VideoParams_t *)&ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams;
    MPEG2_PictureSpecificData_t * PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParsingJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U8                          * StartAddress_p, *StopAddress_p;
    U32                           SCParserState  = PARSER_Data_p->SCParserState;
    U32                           NewSCParserState;
    U32                           BitBufferLoop;


    if ((PARSER_Data_p->StartCode.IsPending == TRUE) && (PARSER_Data_p->StartCode.IsInBitBuffer == TRUE))
    {
        /*
       Startcode ready to process
        */

        /* Remap sc pointers for use by the ES parser */
        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

        StartAddress_p = STAVMEM_DeviceToCPU(PARSER_Data_p->StartCode.StartAddress_p, &VirtualMapping);
        StopAddress_p  = STAVMEM_DeviceToCPU(PARSER_Data_p->StartCode.StopAddress_p, &VirtualMapping);

/*      printf ("Parsing Startcode:  Value 0x%04x  StartAddress 0x%08x  StopAddress 0x%08x\n", PARSER_Data_p->StartCode.Value, StartAddress_p, StopAddress_p); */

        /*
       Check for the start / end of picture data
        */

        if ((PARSER_Data_p->StartCode.Value >= FIRST_SLICE_START_CODE) &&
            (PARSER_Data_p->StartCode.Value <= GREATEST_SLICE_START_CODE))
        {
            /* Picture Data Startcode - is this the first picture data startcode ?? */
            if ((SCParserState == ES_state_picture_header) ||
                (SCParserState == ES_state_picture_coding_ext) ||
                (SCParserState == ES_state_ext2_user2))
            {
                /* Store start of picture pointer is the start of the 32 bit startcode */
                if(PARSER_Data_p->StartCode.StartAddress_p-3<PARSER_Data_p->BitBuffer.ES_Start_p)
                {
                    BitBufferLoop= PARSER_Data_p->StartCode.StartAddress_p-PARSER_Data_p->BitBuffer.ES_Start_p;
                    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData.BitBufferPictureStartAddress=PARSER_Data_p->BitBuffer.ES_Stop_p-(2-BitBufferLoop);
                }
                else
                {
                    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData.BitBufferPictureStartAddress = PARSER_Data_p->StartCode.StartAddress_p - 3;
                }
            }
        }

        /*
       Handle obviously incorrect startcodes by detecting and ignoring them
        */
        if (PARSER_Data_p->StartCode.Value >= SMALLEST_SYSTEM_START_CODE)
        {
#ifdef STVID_DEBUG_GET_STATISTICS
            PARSER_Data_p->Statistics.DecodePbStartCodeFoundInvalid ++;
            if ((PARSER_Data_p->StartCode.Value >= SMALLEST_VIDEO_PACKET_START_CODE) &&
                (PARSER_Data_p->StartCode.Value <= GREATEST_VIDEO_PACKET_START_CODE))
            {
                PARSER_Data_p->Statistics.DecodePbStartCodeFoundVideoPES ++;
            }
#endif
            PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.ParsingError = PARSER_ERROR_STREAM_SYNTAX;
        }
        else
        {
            /*
            Process the startcode
            */
            mpg2par_ProcessStartCode(PARSER_Data_p->StartCode.Value,
                                     StartAddress_p,
                                     StopAddress_p,
                                     ParserHandle);

            NewSCParserState = PARSER_Data_p->SCParserState;

            /* Nothing to do until the state machine transitions from the inactive state */
            if (NewSCParserState != ES_state_inactive)
            {
                /* Handle any userdata that may have been captured */
                if (PARSER_Data_p->UserData.Length > 0)
                {
/* TODO: what to do with BroadcastProfile                    PARSER_Data_p->UserData.BroadcastProfile = PARSER_Data_p->GetPictureParams.BroadcastProfile; */

                    if (PARSER_Data_p->UserData.PositionInStream == STVID_USER_DATA_AFTER_PICTURE)
                    {
                        PARSER_Data_p->UserData.PTS = VideoParams_p->PTS;
                        PARSER_Data_p->UserData.pTemporalReference = PictureSpecificData_p->temporal_reference;

                        if (PARSER_Data_p->UserData.BroadcastProfile == STVID_BROADCAST_DIRECTV)
                        {
                            /* Process the data as DirecTV specific data */
                            mpg2par_DirecTVUserData (ParserHandle);
                            PARSER_Data_p->UserData.Length = 0;
                        }
                    }
                    else
                    {
                        /* No PTS or Temporal Reference associated with non Picture user data */
                        PARSER_Data_p->UserData.PTS.PTS = 0;
                        PARSER_Data_p->UserData.PTS.PTS33 = FALSE;
                        PARSER_Data_p->UserData.PTS.Interpolated = TRUE;
                        PARSER_Data_p->UserData.pTemporalReference = 0;
                    }

                    /* Send any data that was not parsed by the DirecTV parsing */
                    if (PARSER_Data_p->UserData.Length > 0)
                    {
                        STEVT_Notify (PARSER_Properties_p->EventsHandle,
                                PARSER_Data_p->RegisteredEventsID[PARSER_USER_DATA_EVT_ID],
                                STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->UserData));
                        PARSER_Data_p->UserData.Length = 0;
                    }

#ifdef STVID_DEBUG_GET_STATISTICS
                    PARSER_Data_p->Statistics.DecodeUserDatafound ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                }

#ifdef STVID_DEBUG_GET_STATISTICS
                if (PARSER_Data_p->StartCode.Value == SEQUENCE_HEADER_CODE)
                {
                    PARSER_Data_p->Statistics.DecodeSequenceFound ++;
                }
#endif /* STVID_DEBUG_GET_STATISTICS */

            }
        }

        /* The start code has been analyzed: it is not pending anymore */
        PARSER_Data_p->StartCode.IsPending = FALSE;
    }
#ifdef STVID_TRICKMODE_BACKWARD
    else if ((PARSER_Data_p->Backward) && (PARSER_Data_p->StartCode.IsPending) && (!(PARSER_Data_p->StartCode.IsInBitBuffer)) && (!PARSER_Data_p->IsBitBufferFullyParsed))
	{
        PARSER_Data_p->IsBitBufferFullyParsed = TRUE;
        STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

        /* Bit Buffer is fully parsed: stop parsing ... */
        PARSER_Data_p->StopParsing = TRUE;
	}
#endif /* STVID_TRICKMODE_BACKWARD */
}


/*******************************************************************
Function:    mpg2par_ParserTaskFunc

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     The parser task

Behavior:

Exceptions:  None

*******************************************************************/
void mpg2par_ParserTaskFunc(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

    STOS_TaskEnter(NULL);
#ifdef BENCHMARK_WINCESTAPI
    P_ADDSEMAPHORE(PARSER_Data_p->ReferencePicture, "PARSER_Data_p->ReferencePicture");
#endif

   /* Big loop executed until ToBeDeleted is set --------------------------- */
    do
    {
        /* Semaphore has normal time-out to do polling of actions even if semaphore was not signaled. */

        while(!PARSER_Data_p->ParserTask.ToBeDeleted && (PARSER_Data_p->ParserState != PARSER_STATE_PARSING))
        {
            /* Waiting for a command */
            STOS_SemaphoreWait(PARSER_Data_p->ParserOrder);
            mpg2par_GetControllerCommand(ParserHandle);
        }

        if (!PARSER_Data_p->ParserTask.ToBeDeleted)
        {
            /* The parser has a command to process */
            while(!PARSER_Data_p->ParserTask.ToBeDeleted && (PARSER_Data_p->ParserState == PARSER_STATE_PARSING))
            {
                PARSER_Data_p->ForTask.MaxWaitOrderTime = STOS_time_plus(time_now(), MAX_WAIT_ORDER_TIME);
                STOS_SemaphoreWaitTimeOut(PARSER_Data_p->ParserSC, &(PARSER_Data_p->ForTask.MaxWaitOrderTime));               /*, &(PARSER_Data_p->ForTask.MaxWaitOrderTime)*/

                /* Flush the command queue, as the parser is able to process one command at a time */
                while (STOS_SemaphoreWaitTimeOut(PARSER_Data_p->ParserSC, TIMEOUT_IMMEDIATE) == 0)
                {
                    /* Nothing to do: just to decrement counter until 0, to avoid multiple looping on semaphore wait */;
                }

                if (!PARSER_Data_p->ParserTask.ToBeDeleted)
                {
                    /* Get Controller commands first */


                    if (PARSER_Data_p->ParserState == PARSER_STATE_PARSING)
                    {
#ifdef STVID_TRICKMODE_BACKWARD
                        if (((PARSER_Data_p->Backward) && (!PARSER_Data_p->StopParsing)) ||
                            (!PARSER_Data_p->Backward))
#endif /* STVID_TRICKMODE_BACKWARD */
                        {
                            /*Time_start_PARS =STOS_time_now();*/
                            /* Look for new Start Codes to process */
                            mpg2par_GetNextCompleteStartCode(ParserHandle);

                            /* Then, get Start code commands */
                            mpg2par_ProcessCompleteStartCode(ParserHandle);

                            /*time_end_PARS = STOS_time_now();*/
                        }
                    }
                }
            }
        }
    } while (!(PARSER_Data_p->ParserTask.ToBeDeleted));
    STOS_TaskExit(NULL);

} /* End of ParserTaskFunc() function */

/*---------------------------------------------------------------------
   Function:    mpg2par_SavePTSEntry

   Parameters:  ParserHandle PTS start code

   Returns:     None

   Scope:       Private to parser

   Purpose:     The parser task

   Behavior:

   Exceptions:  None

---------------------------------------------------------------------*/
static void mpg2par_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* CPUPTSEntry_p)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    U32 PTSSCAdd;
    PTS_t* PTSLIST_WRITE_PLUS_ONE;

    PTSSCAdd = GetSCAdd(CPUPTSEntry_p);
    PTSSCAdd = PTSSCAdd;

    if ((U32)PTSSCAdd != (U32)PARSER_Data_p->LastPTSStored.Address_p)
    {
        PTSLIST_WRITE_PLUS_ONE = PTSLIST_WRITE + 1;
        if(PTSLIST_WRITE_PLUS_ONE > PTSLIST_TOP)
        {
            PTSLIST_WRITE_PLUS_ONE = PTSLIST_BASE;
        }
        /* Check if we can find a place to store the new PTS Entry, otherwise we
         * overwrite the oldest one */
        if (PTSLIST_WRITE_PLUS_ONE == PTSLIST_READ)
        {
            /* First free the oldest one */
            /* Keep always an empty SCEntry because Read_p == Write_p means that the SCList is empty */
            PTSLIST_READ++;
            if (PTSLIST_READ >= PTSLIST_TOP)
            {
                PTSLIST_READ = PTSLIST_BASE;
            }
            /* Next overwrite it */
        }
        /* Store the PTS */
        PTSLIST_WRITE->PTS33     = GetPTS33(CPUPTSEntry_p);
        PTSLIST_WRITE->PTS32     = GetPTS32(CPUPTSEntry_p);
        PTSLIST_WRITE->Address_p = (void*)PTSSCAdd;
        PTSLIST_WRITE++;

        if (PTSLIST_WRITE >= PTSLIST_TOP)
        {
            PTSLIST_WRITE = PTSLIST_BASE;
        }
        PARSER_Data_p->LastPTSStored.PTS33      = GetPTS33(CPUPTSEntry_p);
        PARSER_Data_p->LastPTSStored.PTS32      = GetPTS32(CPUPTSEntry_p);
        PARSER_Data_p->LastPTSStored.Address_p  = (void*)PTSSCAdd;
    }
} /* End of mpg2par_SavePTSEntry() function */


/*---------------------------------------------------------------------
   Function:    mpg2par_SearchForAssociatedPTS

   Parameters:  ParserHandle PTS start code

   Returns:     None

   Scope:       Private to parser

   Purpose:     The parser task

   Behavior:

   Exceptions:  None

---------------------------------------------------------------------*/
static void mpg2par_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle)
{
    void* ES_Write_p;
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    U8 * PictureStartAddress_p;

    PictureStartAddress_p = PARSER_Data_p->StartCode.StartAddress_p;

    /* no PTS associated to current picture */
    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;

    /* Current position of write pointer in BitBuffer to check PTS & picture address side */
    ES_Write_p = PARSER_Data_p->BitBuffer.ES_Write_p;

    /* Check if PTS address is before PICT address in ES */
    while(
        (PARSER_Data_p->PTS_SCList.Read_p != PARSER_Data_p->PTS_SCList.Write_p) && /* PTS List is not empty */
        (
                (
                    /* 1rst case : PictAdd > PTSAdd.                              */
                    (((U32)PictureStartAddress_p) >= ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p + 3)) &&
                    /* The 2 pointers are on the same side of ES_Write and at least one byte separate them.  */
                    (
                        /* PictAdd > PTSAdd => The 2 pointers must be on the same side of ES_Write */
                        (
                            (((U32)ES_Write_p) > ((U32)PictureStartAddress_p)) &&
                            (((U32)ES_Write_p) > ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p))
                        ) ||
                        (
                            (((U32)ES_Write_p) < ((U32)PictureStartAddress_p)) &&
                            (((U32)ES_Write_p) < ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p))
                        )
                    )
                ) ||
                (
                    /* 2nd case : PictAdd < PTSAdd.                               */
                    (((U32)PictureStartAddress_p) < ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p)) &&
                    /* The 2 pointers are not on the same side of ES_Write and at least 1 byte separate them.  */
                    (((U32)PARSER_Data_p->BitBuffer.ES_Stop_p
                    - (U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p + 1
                    + (U32)PictureStartAddress_p
                    - (U32)PARSER_Data_p->BitBuffer.ES_Start_p + 1 ) > 1 ) &&
                    /* PictAdd < PTSAdd => The 2 pointers must not be on the same side of ES_Write */
                    (((U32)ES_Write_p) > ((U32)PictureStartAddress_p)) &&
                    (((U32)ES_Write_p) < ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p))
                )
            )
        )
    {
        /* Associate PTS to Picture */
        PARSER_Data_p->PTSAvailableForNextPicture = TRUE;
        PARSER_Data_p->PTSForNextPicture.PTS = PARSER_Data_p->PTS_SCList.Read_p->PTS32;
        PARSER_Data_p->PTSForNextPicture.PTS33 = PARSER_Data_p->PTS_SCList.Read_p->PTS33;
        PARSER_Data_p->PTSForNextPicture.Interpolated = FALSE;
        PARSER_Data_p->PTS_SCList.Read_p++;
        if (((U32)PARSER_Data_p->PTS_SCList.Read_p) >= ((U32)PARSER_Data_p->PTS_SCList.SCList + sizeof(PTS_t) * PTS_SCLIST_SIZE))
        {
            PARSER_Data_p->PTS_SCList.Read_p = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
        }
    }
} /* End of mpg2par_SearchForAssociatedPTS() function */




#ifdef STVID_VALID
static void mpg2parserDumpParsingJobResults (PARSER_ParsingJobResults_t *ParsingJobResults_p, BOOL close)
{
   static FILE *outFile = NULL;
   VIDCOM_ParsedPictureInformation_t *ParsedPictureInformation_p = ParsingJobResults_p->ParsedPictureInformation_p;
   PARSER_ParsingJobStatus_t         *ParsingJobStatus_p = &ParsingJobResults_p->ParsingJobStatus;
   VIDCOM_PictureGenericData_t       *PictureGenericData_p = &ParsedPictureInformation_p->PictureGenericData;
   VIDCOM_PictureDecodingContext_t   *PDC = ParsedPictureInformation_p->PictureDecodingContext_p;
   MPEG2_PictureSpecificData_t       *PictureSpecificData_p = (MPEG2_PictureSpecificData_t *)ParsedPictureInformation_p->PictureSpecificData_p;
   STVID_VideoParams_t               *VideoParams_p  = &PictureGenericData_p->PictureInfos.VideoParams;
   STGXOBJ_Bitmap_t                  *Bitmap_p  = &PictureGenericData_p->PictureInfos.BitmapParams;
   VIDCOM_GlobalDecodingContext_t    *GDC = PDC->GlobalDecodingContext_p;
   MPEG2_PictureDecodingContextSpecificData_t *PDCSD = (MPEG2_PictureDecodingContextSpecificData_t *)PDC->PictureDecodingContextSpecificData_p;
   MPEG2_GlobalDecodingContextSpecificData_t  *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *)GDC->GlobalDecodingContextSpecificData_p;
   VIDCOM_GlobalDecodingContextGenericData_t  *GlobalDecodingContextGenericData_p = &GDC->GlobalDecodingContextGenericData;
   STVID_SequenceInfo_t              *SI  = &GlobalDecodingContextGenericData_p->SequenceInfo;

   U32 panandscan, counter;

   if (close != FALSE)
   {
/*      Calling to close file*/
      if (outFile != NULL)
      {
/*         Close the MME Command files*/
         DumpMMECommandInfo (NULL, NULL, 0, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE);

         fclose (outFile);
         outFile = NULL;
      }
      return;
   }

   if (outFile == NULL)
   {
       osclock_t TOut_Now;
      char FileName[30];

      TOut_Now = STOS_time_now();

      sprintf(FileName,"../../PJR-%010u.txt", (U32)TOut_Now);

      outFile = fopen(FileName, "w" );
   }


   fprintf (outFile, "ParsingJobResults (%d)\n", PictureGenericData_p->DecodingOrderFrameID);

   fprintf (outFile, "  ParsingJobStatus\n");
   fprintf (outFile, "    HasErrors = %d\n", ParsingJobStatus_p->HasErrors);
   fprintf (outFile, "    ErrorCode = %d\n", ParsingJobStatus_p->ErrorCode);
#ifndef COMPARE_OMEGA
   fprintf (outFile, "    JobSubmissionTime = 0x%08x\n", ParsingJobStatus_p->JobSubmissionTime);
   fprintf (outFile, "    JobCompletionTime = 0x%08x\n", ParsingJobStatus_p->JobCompletionTime);
#endif

   fprintf (outFile, "  Parsed Picture Information\n");
   fprintf (outFile, "    PictureGenericData\n");
   fprintf (outFile, "      IsPTSValid %d\n", VideoParams_p->PTS.IsValid);
   fprintf (outFile, "      RepeatFirstField %d\n", PictureGenericData_p->RepeatFirstField);
   fprintf (outFile, "      RepeatProgressiveCounter %d\n", PictureGenericData_p->RepeatProgressiveCounter);

#ifndef COMPARE_OMEGA
   fprintf (outFile, "      NumberOfPanAndScan %d\n", PictureGenericData_p->NumberOfPanAndScan);
   for (panandscan = 0; panandscan < PictureGenericData_p->NumberOfPanAndScan; panandscan++)
   {
      fprintf (outFile, "       PanAndScanIn16thPixel[%d].FrameCentreHorizontalOffset  %d\n", panandscan, PictureGenericData_p->PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset);
      fprintf (outFile, "       PanAndScanIn16thPixel[%d].FrameCentreVerticaOffset     %d\n", panandscan, PictureGenericData_p->PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset);
      fprintf (outFile, "       PanAndScanIn16thPixel[%d].DisplayHorizontalSize        %d\n", panandscan, PictureGenericData_p->PanAndScanIn16thPixel[panandscan].DisplayHorizontalSize);
      fprintf (outFile, "       PanAndScanIn16thPixel[%d].DisplayVerticalSize          %d\n", panandscan, PictureGenericData_p->PanAndScanIn16thPixel[panandscan].DisplayVerticalSize);
      fprintf (outFile, "       PanAndScanIn16thPixel[%d].HasDisplaySizeRecommendation %d\n", panandscan, PictureGenericData_p->PanAndScanIn16thPixel[panandscan].HasDisplaySizeRecommendation);
   }
#endif

   fprintf (outFile, "      DecodingOrderFrameID %d\n", PictureGenericData_p->DecodingOrderFrameID);
   fprintf (outFile, "      ExtendedPresentationOrderPictureID: ID = %d    IDExtension = %d\n",
            PictureGenericData_p->ExtendedPresentationOrderPictureID.ID, PictureGenericData_p->ExtendedPresentationOrderPictureID.IDExtension);
   fprintf (outFile, "      IsFirstOfTwoFields %d\n", PictureGenericData_p->IsFirstOfTwoFields);
   fprintf (outFile, "      IsReference %d\n", PictureGenericData_p->IsReference);

   fprintf (outFile, "      FullReferenceFrameList:\n");

   for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; counter++)
   {
      if (PictureGenericData_p->IsValidIndexInReferenceFrame[counter])
      {
         fprintf (outFile, "      FullReferenceFrameList[%d] %d\n", counter, PictureGenericData_p->FullReferenceFrameList[counter]);
      }
   }

   fprintf (outFile, "      UsedSizeInReferenceListP0 %d\n", PictureGenericData_p->UsedSizeInReferenceListP0);
   for (counter = 0; counter < PictureGenericData_p->UsedSizeInReferenceListP0; counter++)
   {
      fprintf (outFile, "      ReferenceListP0[%d]  %d\n", counter, PictureGenericData_p->ReferenceListP0[counter]);
   }

   fprintf (outFile, "      UsedSizeInReferenceListB0 %d\n", PictureGenericData_p->UsedSizeInReferenceListB0);
   for (counter = 0; counter < PictureGenericData_p->UsedSizeInReferenceListB0; counter++)
   {
      fprintf (outFile, "      ReferenceListB0[%d]  %d\n", counter, PictureGenericData_p->ReferenceListB0[counter]);
   }

   fprintf (outFile, "      UsedSizeInReferenceListB1 %d\n", PictureGenericData_p->UsedSizeInReferenceListB1);
   for (counter = 0; counter < PictureGenericData_p->UsedSizeInReferenceListB1; counter++)
   {
      fprintf (outFile, "      ReferenceListB1[%d]  %d\n", counter, PictureGenericData_p->ReferenceListB1[counter]);
   }


#ifndef COMPARE_OMEGA
   fprintf (outFile, "      BitBufferPictureStartAddress 0x%08x\n", PictureGenericData_p->BitBufferPictureStartAddress);
   fprintf (outFile, "      BitBufferPictureStopAddress 0x%08x\n", PictureGenericData_p->BitBufferPictureStopAddress);
/*   fprintf (outFile, "      AsynchronousCommands %d\n", PictureGenericData_p->AsynchronousCommands);*/
   fprintf (outFile, "      DecodeStartTime %d    valid = %d\n", PictureGenericData_p->DecodeStartTime, PictureGenericData_p->IsDecodeStartTimeValid);
   fprintf (outFile, "      PresentationStartTime %d    valid = %d\n", PictureGenericData_p->PresentationStartTime, PictureGenericData_p->IsPresentationStartTimeValid);
#endif
   fprintf (outFile, "      ParsingError %d\n", PictureGenericData_p->ParsingError);
   fprintf (outFile, "      PictureInfos\n");
   fprintf (outFile, "        VideoParams\n");
   fprintf (outFile, "          FrameRate %d\n", VideoParams_p->FrameRate);
   fprintf (outFile, "          ScanType %s\n", (VideoParams_p->ScanType == 0) ? "STGXOBJ_PROGRESSIVE_SCAN" : "STGXOBJ_INTERLACED_SCAN");
   fprintf (outFile, "          MPEGFrame %s\n", (VideoParams_p->MPEGFrame == 1) ? "STVID_MPEG_FRAME_I" : (VideoParams_p->MPEGFrame == 2) ? "STVID_MPEG_FRAME_P" : "STVID_MPEG_FRAME_B");
   fprintf (outFile, "          PictureStructure %s\n", (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD) ? "STVID_PICTURE_STRUCTURE_TOP_FIELD": (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD) ? "STVID_PICTURE_STRUCTURE_BOTTOM_FIELD" : "STVID_PICTURE_STRUCTURE_FRAME");
   fprintf (outFile, "          TopFieldFirst %d\n", VideoParams_p->TopFieldFirst);
   fprintf (outFile, "          TimeCode %d hr  %d min  %d sec  %d frames  interpolated = %d\n", VideoParams_p->TimeCode.Hours, VideoParams_p->TimeCode.Minutes,
           VideoParams_p->TimeCode.Seconds, VideoParams_p->TimeCode.Frames, VideoParams_p->TimeCode.Interpolated );
   fprintf (outFile, "          PTS upper = %d   lower = %d   Interpolated = %d\n", VideoParams_p->PTS.PTS33, VideoParams_p->PTS.PTS, VideoParams_p->PTS.Interpolated);

#ifndef COMPARE_OMEGA
   fprintf (outFile, "          CompressionLevel %s\n", (VideoParams_p->CompressionLevel == STVID_COMPRESSION_LEVEL_NONE) ? "STVID_COMPRESSION_LEVEL_NONE" : (VideoParams_p->CompressionLevel == STVID_COMPRESSION_LEVEL_1) ? "STVID_COMPRESSION_LEVEL_1" : (VideoParams_p->CompressionLevel == STVID_COMPRESSION_LEVEL_2) ? "STVID_COMPRESSION_LEVEL_2": "Illegal");
#endif
   fprintf (outFile, "          DecimationFactors %s\n", (VideoParams_p->DecimationFactors == 1) ? "STVID_DECIMATION_FACTOR_H2":
                                               (VideoParams_p->DecimationFactors == 2) ? "STVID_DECIMATION_FACTOR_V2":
                                               (VideoParams_p->DecimationFactors == 4) ? "STVID_DECIMATION_FACTOR_H4":
                                               (VideoParams_p->DecimationFactors == 8) ? "STVID_DECIMATION_FACTOR_V4":
                                               (VideoParams_p->DecimationFactors == 3) ? "STVID_DECIMATION_FACTOR_2": "STVID_DECIMATION_FACTOR_4");
   ;
   fprintf (outFile, "        BitmapParams\n");
   fprintf (outFile, "          ColorType %d\n", Bitmap_p->ColorType);
   fprintf (outFile, "          BitmapType %d\n", Bitmap_p->BitmapType);
   fprintf (outFile, "          PreMultipliedColor %d\n", Bitmap_p->PreMultipliedColor);
#ifndef COMPARE_OMEGA
   fprintf (outFile, "          ColorSpaceConversion %d\n", Bitmap_p->ColorSpaceConversion);
#endif
   fprintf (outFile, "          Width %d\n", Bitmap_p->Width);
   fprintf (outFile, "          Height %d\n", Bitmap_p->Height);
   fprintf (outFile, "          Pitch %d\n", Bitmap_p->Pitch);
   fprintf (outFile, "          Offset %d\n", Bitmap_p->Offset);
#ifndef COMPARE_OMEGA
   fprintf (outFile, "          Data1_p 0x%08x\n", Bitmap_p->Data1_p);
   fprintf (outFile, "          Size1 %d\n", Bitmap_p->Size1);
   fprintf (outFile, "          Data2_p 0x%08x\n", Bitmap_p->Data2_p);
   fprintf (outFile, "          Size2 %d\n", Bitmap_p->Size2);
   fprintf (outFile, "          SubByteFormat %d\n", Bitmap_p->SubByteFormat);
   fprintf (outFile, "          BigNotLittle %d\n", Bitmap_p->BigNotLittle);
#endif

#ifdef DELTAMU_PARSER
   fprintf (outFile, "    PictureSpecificData\n");
   fprintf (outFile, "      temporal_reference %d\n", PictureSpecificData_p->temporal_reference);
   fprintf (outFile, "      vbv_delay %d\n", PictureSpecificData_p->vbv_delay);

   if (VideoParams_p->MPEGFrame != 1)
   {
      fprintf (outFile, "      full_pel_forward_vector %d\n", PictureSpecificData_p->full_pel_forward_vector);
      fprintf (outFile, "      forward_f_code %d\n", PictureSpecificData_p->forward_f_code);
   }

   if (VideoParams_p->MPEGFrame == 4)
   {
      fprintf (outFile, "      full_pel_backward_vector %d\n", PictureSpecificData_p->full_pel_backward_vector);
      fprintf (outFile, "      backward_f_code %d\n", PictureSpecificData_p->backward_f_code);
   }

   fprintf (outFile, "      temporal_reference %d\n", PictureSpecificData_p->temporal_reference);
   fprintf (outFile, "      f_code [0][0] %d  [0][1] %d  [1][0] %d  [1][1] %d\n", PictureSpecificData_p->f_code[0][0], PictureSpecificData_p->f_code[0][1], PictureSpecificData_p->f_code[1][0], PictureSpecificData_p->f_code[1][1]);
   fprintf (outFile, "      intra_dc_precision %d\n", PictureSpecificData_p->intra_dc_precision);
   fprintf (outFile, "      top_field_first %d\n", VideoParams_p->TopFieldFirst);
   fprintf (outFile, "      frame_pred_frame_dct %d\n", PictureSpecificData_p->frame_pred_frame_dct);
   fprintf (outFile, "      concealment_motion_vectors %d\n", PictureSpecificData_p->concealment_motion_vectors);
   fprintf (outFile, "      q_scale_type %d\n", PictureSpecificData_p->q_scale_type);
   fprintf (outFile, "      intra_vlc_format %d\n", PictureSpecificData_p->intra_vlc_format);
   fprintf (outFile, "      alternate_scan %d\n", PictureSpecificData_p->alternate_scan);
   fprintf (outFile, "      repeat_first_field %d\n", PictureSpecificData_p->repeat_first_field);
   fprintf (outFile, "      chroma_420_type %d\n", PictureSpecificData_p->chroma_420_type);
   fprintf (outFile, "      progressive_frame %d\n", PictureSpecificData_p->progressive_frame);
   fprintf (outFile, "      composite_display_flag %d\n", PictureSpecificData_p->composite_display_flag);
#else

/*    Old mpeg codec structures*/
   fprintf (outFile, "    PictureSpecificData\n", (U32)PictureSpecificData_p);
   fprintf (outFile, "      temporal_reference %d\n", PictureSpecificData_p->StreamInfoForDecode.Picture.temporal_reference);
   fprintf (outFile, "      vbv_delay %d\n", PictureSpecificData_p->StreamInfoForDecode.Picture.vbv_delay);

   if (VideoParams_p->MPEGFrame != 1)
   {
      fprintf (outFile, "      full_pel_forward_vector %d\n", PictureSpecificData_p->StreamInfoForDecode.Picture.full_pel_forward_vector);
      fprintf (outFile, "      forward_f_code %d\n", PictureSpecificData_p->StreamInfoForDecode.Picture.forward_f_code);
   }


   if (VideoParams_p->MPEGFrame == 4)
   {
      fprintf (outFile, "      full_pel_backward_vector %d\n", PictureSpecificData_p->StreamInfoForDecode.Picture.full_pel_backward_vector);
      fprintf (outFile, "      backward_f_code %d\n", PictureSpecificData_p->StreamInfoForDecode.Picture.backward_f_code);
   }

   fprintf (outFile, "      temporal_reference %d\n", PictureSpecificData_p->StreamInfoForDecode.Picture.temporal_reference);
   fprintf (outFile, "      f_code [0][0] %d  [0][1] %d  [1][0] %d  [1][1] %d\n",
            PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.f_code[0][0],
            PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.f_code[0][1],
            PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.f_code[1][0],
            PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.f_code[1][1]);
   fprintf (outFile, "      intra_dc_precision %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.intra_dc_precision);
/*   fprintf (outFile, "      picture_structure %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.picture_structure);*/
   fprintf (outFile, "      top_field_first %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.top_field_first);
   fprintf (outFile, "      frame_pred_frame_dct %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.frame_pred_frame_dct);
   fprintf (outFile, "      concealment_motion_vectors %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.frame_pred_frame_dct);
   fprintf (outFile, "      q_scale_type %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.q_scale_type);
   fprintf (outFile, "      intra_vlc_format %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.intra_vlc_format);
   fprintf (outFile, "      alternate_scan %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.alternate_scan);
   fprintf (outFile, "      repeat_first_field %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.repeat_first_field);
   fprintf (outFile, "      chroma_420_type %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.chroma_420_type);
   fprintf (outFile, "      progressive_frame %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.progressive_frame);
   fprintf (outFile, "      composite_display_flag %d\n", PictureSpecificData_p->StreamInfoForDecode.PictureCodingExtension.composite_display_flag);
#endif

#ifndef COMPARE_OMEGA
   fprintf (outFile, "      v_axis %d\n", PictureSpecificData_p->v_axis);
   fprintf (outFile, "      field_sequence %d\n", PictureSpecificData_p->field_sequence);
   fprintf (outFile, "      sub_carrier %d\n", PictureSpecificData_p->sub_carrier);
   fprintf (outFile, "      burst_amplitude %d\n", PictureSpecificData_p->burst_amplitude);
   fprintf (outFile, "      sub_carrier_phase %d\n", PictureSpecificData_p->sub_carrier_phase);
   fprintf (outFile, "      number_of_frame_centre_offsets %d\n", PictureSpecificData_p->number_of_frame_centre_offsets);
   for (counter = 0; counter < PictureSpecificData_p->number_of_frame_centre_offsets; counter++)
   {
      fprintf (outFile, "        FrameCenterOffsets[%d] %d\n", counter, PictureSpecificData_p->FrameCentreOffsets[counter]);
   }

   fprintf (outFile, "      LumaIntraQuantMatrix_p 0x%08x\n", (U32)PictureSpecificData_p->LumaIntraQuantMatrix_p);
/*
   if (PictureSpecificData_p->LumaIntraQuantMatrix_p != NULL)
   {
      for (counter = 0; counter < 64; counter++)
      {
         if ((counter % 8) == 0)
            fprintf (outFile, "\n      ");

         fprintf (outFile, "  %3d", *((U8 *)(PictureSpecificData_p->LumaIntraQuantMatrix_p) + counter));
      }
      fprintf (outFile, "\n");
   }
*/
   fprintf (outFile, "\n      LumaNonIntraQuantMatrix_p 0x%08x", (U32)PictureSpecificData_p->LumaNonIntraQuantMatrix_p);
/*
   if (PictureSpecificData_p->LumaNonIntraQuantMatrix_p != NULL)
   {
      for (counter = 0; counter < 64; counter++)
      {
         if ((counter % 8) == 0)
            fprintf (outFile, "\n      ");

         fprintf (outFile, "  %3d", *((U8 *)(PictureSpecificData_p->LumaNonIntraQuantMatrix_p) + counter));
      }
      fprintf (outFile, "\n");
   }
*/
      fprintf (outFile, "\n      ChromaIntraQuantMatrix_p 0x%08x\n", (U32)PictureSpecificData_p->ChromaIntraQuantMatrix_p);
/*   if (PictureSpecificData_p->ChromaIntraQuantMatrix_p != NULL)
   {
      for (counter = 0; counter < 64; counter++)
      {
         if ((counter % 8) == 0)
            fprintf (outFile, "\n      ");

         fprintf (outFile, "  %3d", *((U8 *)(PictureSpecificData_p->ChromaIntraQuantMatrix_p) + counter));
      }
      fprintf (outFile, "\n");
   }
*/
   fprintf (outFile, "\n      ChromaNonIntraQuantMatrix_p 0x%08x\n", (U32)PictureSpecificData_p->ChromaNonIntraQuantMatrix_p);
/*   if (PictureSpecificData_p->ChromaNonIntraQuantMatrix_p != NULL)
   {
      for (counter = 0; counter < 64; counter++)
      {
         if ((counter % 8) == 0)
            fprintf (outFile, "\n      ");

         fprintf (outFile, "  %3d", *((U8 *)(PictureSpecificData_p->ChromaNonIntraQuantMatrix_p) + counter));
      }
      fprintf (outFile, "\n");
   }
*/
#endif
#ifdef DELTAMU_PARSER
   fprintf (outFile, "      IntraQuantMatrixHasChanged %d\n", (U32)PictureSpecificData_p->IntraQuantMatrixHasChanged);
   fprintf (outFile, "      NonIntraQuantMatrixHasChanged %d\n", (U32)PictureSpecificData_p->NonIntraQuantMatrixHasChanged);
#ifndef COMPARE_OMEGA
   fprintf (outFile, "      BackwardTemporalReferenceValue %d\n", PictureSpecificData_p->BackwardTemporalReferenceValue);
   fprintf (outFile, "      ForwardTemporalReferenceValue %d\n", PictureSpecificData_p->ForwardTemporalReferenceValue);
#endif
#else
   fprintf (outFile, "      IntraQuantMatrixHasChanged %d\n", PictureSpecificData_p->StreamInfoForDecode.IntraQuantMatrixHasChanged);
   fprintf (outFile, "      NonIntraQuantMatrixHasChanged %d\n", PictureSpecificData_p->StreamInfoForDecode.NonIntraQuantMatrixHasChanged);
#endif

   fprintf (outFile, "    PictureDecodingContext_p\n", (U32)PDC);
#ifndef COMPARE_OMEGA
   fprintf (outFile, "      PictureDecodingContextSpecificData_p (0x%08x)\n", (U32)PDCSD);
   fprintf (outFile, "        closed_gop %d\n", PDCSD->closed_gop);
   fprintf (outFile, "        broken_link %d\n", PDCSD->broken_link);
#endif
   fprintf (outFile, "      SizeOfPictureDecodingContextSpecificDataInByte %d\n", PDC->SizeOfPictureDecodingContextSpecificDataInByte);

   fprintf (outFile, "      GlobalDecodingContext_p\n", PDC->GlobalDecodingContext_p);
   fprintf (outFile, "        GlobalDecodingContextGenericData\n");
   fprintf (outFile, "          ColourPrimaries %d\n", GlobalDecodingContextGenericData_p->ColourPrimaries);
   fprintf (outFile, "          TransferCharacteristics %d\n", GlobalDecodingContextGenericData_p->TransferCharacteristics);
   fprintf (outFile, "          MatrixCoefficients %d\n", GlobalDecodingContextGenericData_p->MatrixCoefficients);
   fprintf (outFile, "          FrameCropInPixel TopOffset %d  LeftOffset %d  BottomOffset %d  RightOffset %d\n",
           GlobalDecodingContextGenericData_p->FrameCropInPixel.TopOffset, GlobalDecodingContextGenericData_p->FrameCropInPixel.LeftOffset,
           GlobalDecodingContextGenericData_p->FrameCropInPixel.BottomOffset, GlobalDecodingContextGenericData_p->FrameCropInPixel.RightOffset);
   fprintf (outFile, "          NumberOfReferenceFrames %d\n", GlobalDecodingContextGenericData_p->NumberOfReferenceFrames);
   fprintf (outFile, "          SequenceInfo\n");
   fprintf (outFile, "            Height %d\n", SI->Height);
   fprintf (outFile, "            Width %d\n", SI->Width);
   fprintf (outFile, "            Aspect %s\n", (SI->Aspect == 1) ? "STVID_DISPLAY_ASPECT_RATIO_16TO9" :
                                      (SI->Aspect == 2) ? "STVID_DISPLAY_ASPECT_RATIO_4TO3" :
                                      (SI->Aspect == 4) ? "STVID_DISPLAY_ASPECT_RATIO_221TO1" : "STVID_DISPLAY_ASPECT_RATIO_SQUARE");
   fprintf (outFile, "            ScanType %2\n", (SI->ScanType == 1) ? "STVID_SCAN_TYPE_PROGRESSIVE" : "STVID_SCAN_TYPE_INTERLACED");
   fprintf (outFile, "            FrameRate %d\n", SI->FrameRate);
   fprintf (outFile, "            BitRate %d\n", SI->BitRate);
   fprintf (outFile, "            MPEGStandard %d\n", SI->MPEGStandard);
   fprintf (outFile, "            IsLowDelay %d\n", SI->IsLowDelay);
   fprintf (outFile, "            VBVBufferSize %d\n", SI->VBVBufferSize);
   fprintf (outFile, "            StreamID %d\n", SI->StreamID);
   fprintf (outFile, "            ProfileAndLevelIndication %d\n", SI->ProfileAndLevelIndication);
   fprintf (outFile, "            VideoFormat %d\n", SI->VideoFormat);
   fprintf (outFile, "            FrameRateExtensionN %d\n", SI->FrameRateExtensionN);
   fprintf (outFile, "            FrameRateExtensionD %d\n", SI->FrameRateExtensionD);

#ifndef COMPARE_OMEGA
   fprintf (outFile, "        GlobalDecodingContextSpecificData (0x%08x)\n", (U32)GlobalDecodingContextSpecificData_p);
   fprintf (outFile, "          vbv_buffer_size_value %d\n", GlobalDecodingContextSpecificData_p->vbv_buffer_size_value);
   fprintf (outFile, "          load_intra_quantizer_matrix %d\n", GlobalDecodingContextSpecificData_p->load_intra_quantizer_matrix);

   for (counter = 0; counter < MPEG2_Q_MATRIX_SIZE; counter++)
   {
      if ((counter % 8) == 0)
         fprintf (outFile, "\n      ");

      fprintf (outFile, "  %3d", GlobalDecodingContextSpecificData_p->intra_quantizer_matrix[counter]);
   }
   fprintf (outFile, "\n");

   fprintf (outFile, "          load_non_intra_quantizer_matrix %d\n", GlobalDecodingContextSpecificData_p->load_non_intra_quantizer_matrix);
   for (counter = 0; counter < MPEG2_Q_MATRIX_SIZE; counter++)
   {
      if ((counter % 8) == 0)
         fprintf (outFile, "\n      ");

      fprintf (outFile, "  %3d", GlobalDecodingContextSpecificData_p->non_intra_quantizer_matrix[counter]);
   }
   fprintf (outFile, "\n");

   fprintf (outFile, "          chroma_format %d\n", GlobalDecodingContextSpecificData_p->chroma_format);
   fprintf (outFile, "          matrix_coefficients %d\n", GlobalDecodingContextSpecificData_p->matrix_coefficients);
   fprintf (outFile, "          HasSequenceDisplay %d\n", GlobalDecodingContextSpecificData_p->HasSequenceDisplay);
#endif

   fprintf (outFile, "          display_horizontal_size %d\n", GlobalDecodingContextSpecificData_p->display_horizontal_size);
   fprintf (outFile, "          display_vertical_size %d\n", GlobalDecodingContextSpecificData_p->display_vertical_size);
#ifndef COMPARE_OMEGA
   fprintf (outFile, "          IsBitStreamMPEG1 %d\n", GlobalDecodingContextSpecificData_p->IsBitStreamMPEG1);
   fprintf (outFile, "          FirstPictureEver %d\n", GlobalDecodingContextSpecificData_p->FirstPictureEver);
   fprintf (outFile, "          ExtendedTemporalReference %d\n", GlobalDecodingContextSpecificData_p->ExtendedTemporalReference);
   fprintf (outFile, "          TemporalReferenceOffset %d\n", GlobalDecodingContextSpecificData_p->TemporalReferenceOffset);
   fprintf (outFile, "          PreviousGreatestExtendedTemporalReference %d\n", GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference);
   fprintf (outFile, "          PictureCounter %d\n", GlobalDecodingContextSpecificData_p->PictureCounter);
#endif

/*   fflush (outFile);*/
}
#endif

