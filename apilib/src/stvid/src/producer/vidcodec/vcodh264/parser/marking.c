/*!
 *******************************************************************************
 * \file marking.c
 *
 * \brief H264 picture pre and post decoding marking functions
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) STMicroelectronics 2007
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#endif

/* H264 Parser specific */
#include "h264parser.h"
/* Functions ---------------------------------------------------------------- */

/******************************************************************************/
/*! \brief Removes a frame from the DPBRef
 *
 * The Virtual DPB state is modified. A frame buffer is freed
 * The removed picture is then considered as being "unused for reference"
 *
 * \param FrameIndex array index of the frame to remove
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_MarkFrameAsUnusedForReference(U8 FrameIndex, const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsReference = FALSE;
	PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsReference = FALSE;
    PARSER_Data_p->DPBFrameReference[FrameIndex].IsNonExisting = FALSE;
	/* all other members of DPBFrameReference[i] remain unchanged as they will be updated as soon
	 * as a new picture is pushed into the DPBRef
	 */
}

/******************************************************************************/
/*! \brief Assign a short term reference to a frame buffer in DPBRef
 *
 * A new frame buffer is used to store the picture
 * Depending on the frame/field, TOP and/or BOTTOM fields are updated
 *
 * \param FrameIndex the array index where to store the picture
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_AssignShortTermInFrameBuffer(U8 FrameIndex, const PARSER_Handle_t  ParserHandle)
{
   	/* dereference ParserHandle to a local variable to ease debug */
   	H264ParserPrivateData_t * PARSER_Data_p;

   	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if (DPBRefFrameIsUsed(FrameIndex))
	{
		PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.MarkInUsedFrameBufferError,
		"marking.c: (AssignShortTermInFrameBuffer) error: attempting to mark current picture in already used frame buffer\n");
    /* Stops treatment here to avoid bad data handling */
    return;
	}

    PARSER_Data_p->DPBFrameReference[FrameIndex].frame_num = PARSER_Data_p->PictureLocalData.frame_num;
    PARSER_Data_p->DPBFrameReference[FrameIndex].IsNonExisting = PARSER_Data_p->PictureLocalData.IsNonExisting;
	/* While BrokenLinkFlag is true (we are in between the SEI message that tagged a broken link and the recovery point
	 * tagged by the SEI message), all reference pictures are tagged as BrokenLink
	 */
	if (PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.BrokenLinkFlag)
	{
		PARSER_Data_p->DPBFrameReference[FrameIndex].IsBrokenLink = TRUE;
	}
	else
	{
		PARSER_Data_p->DPBFrameReference[FrameIndex].IsBrokenLink = FALSE;
	}


    if (CurrentPictureIsFrame)
    {
	  	PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsReference = TRUE;
	    PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsReference = TRUE;
    }
	else if (CurrentPictureIsTopField)
	{
        PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsReference = TRUE;
	}
    else /* Picture to mark is a bottom field */
    {
	    PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsReference = TRUE;
	}

	/* Clear both top and bottom long term flags as a new frame buffer is in use */
	PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsLongTerm = FALSE;
	PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsLongTerm = FALSE;

	/* FrameNumWrap, PicNum(s), LongTermPicNum(s) will be updated in the DecodingProcessForPictureNumbers() */
	/* For an existing picture, LongTermFrameIdx is set when a the picture is set to long term reference picture */
	/* For a non existing picture, the LongTermFrameIdx will never be used */

	/* DecodingOrderFrameID is computed after the marking process (we need to know whether the current picture has MMCO=5
	 * to do so. At that stage, the parser stores the FrameIndex where the current picture is stored, and the
	 * assignement of DecodingOrderFrameID will be done one DecodingOrderFrameID is computed
	 */
	/* The following assignement is also done when the picture is a non-existing frame. We don't care, as at the end of the
	 * marking, it will be done for the current picture. So the value at the end of the marking process is the right one
	 */
	PARSER_Data_p->ParserGlobalVariable.CurrentPictureFrameIndexInDPBRef = FrameIndex;
}

/******************************************************************************/
/*! \brief Performs the decoding process for picture numbers
 *
 * This function updates the FrameNumWrap, TopFieldPicNum, BottomFieldPicNum,
 * TopFieldLongTermPicNum and BottomFieldLongTermPicNum of any picture in the
 * DPBRef
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_DecodingProcessForPictureNumbers(const PARSER_Handle_t  ParserHandle)
{
	U8 i; /* loop index to sweep over the DPBRef */
 	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
	{
		/* Process only frame buffer in use in the DPBRef */
		if (DPBRefFrameIsUsed(i))
		{
			/* Decoding process for FrameNumWrap. Equation (8-28) */
			if (PARSER_Data_p->DPBFrameReference[i].frame_num > PARSER_Data_p->PictureLocalData.frame_num)
			{
                PARSER_Data_p->DPBFrameReference[i].FrameNumWrap = PARSER_Data_p->DPBFrameReference[i].frame_num - PARSER_Data_p->ActiveSPSLocalData_p->MaxFrameNum;
			}
			else
			{
				PARSER_Data_p->DPBFrameReference[i].FrameNumWrap = PARSER_Data_p->DPBFrameReference[i].frame_num;
			}

			/* Decoding process for TopFieldPicNum and BottomFieldPicNum */
			if (CurrentPictureIsFrame)
			{
				/* Note: we do not check whether the frame is short or long term, because the storage of PicNum and
				 * LongTermPicNum is at different location and so does not overlap
				 */
				/* For each short term frame, PicNum for both fields of the frame is equal to FrameNumWrap */
		 		/* equation (8-29) */
		 		PARSER_Data_p->DPBFrameReference[i].TopFieldPicNum = PARSER_Data_p->DPBFrameReference[i].FrameNumWrap;
		 		PARSER_Data_p->DPBFrameReference[i].BottomFieldPicNum = PARSER_Data_p->DPBFrameReference[i].FrameNumWrap;

				/* Note: we do not check whether the frame is short or long term, because the storage of PicNum and
				 * LongTermPicNum is at different location and so does not overlap
				 */
				/* For each long term frame, LongTermPicNum for both fields of the frame is equal to LongTermFrameIdx */
		 		/* equation (8-30) */
		 		PARSER_Data_p->DPBFrameReference[i].TopFieldLongTermPicNum = PARSER_Data_p->DPBFrameReference[i].LongTermFrameIdx;
		 		PARSER_Data_p->DPBFrameReference[i].BottomFieldLongTermPicNum = PARSER_Data_p->DPBFrameReference[i].LongTermFrameIdx;
			}
			else /* the current picture is a field */
			{
				/* Analyze all the fields of the frames stored in the DPBRef */
				/* If the current picture is a top field, the following applies */
				if (CurrentPictureIsTopField)
				{
					/* Set all short term top fields (same parity as current field) PicNum to 2 * FrameNumWrap + 1 */
					/* equation (8-31) */
					PARSER_Data_p->DPBFrameReference[i].TopFieldPicNum = (2 * PARSER_Data_p->DPBFrameReference[i].FrameNumWrap) + 1;

					/* Set all short term bottom fields (opposite parity as current field) PicNum to 2 * FrameNumWrap */
					/* equation (8-32) */
					PARSER_Data_p->DPBFrameReference[i].BottomFieldPicNum = 2 * PARSER_Data_p->DPBFrameReference[i].FrameNumWrap;

					/* Set all long term top fields (same parity as current field) LongTermPicNum to 2 * LongTermFrameIdx + 1 */
					/* equation (8-33) */
					PARSER_Data_p->DPBFrameReference[i].TopFieldLongTermPicNum = (2 * PARSER_Data_p->DPBFrameReference[i].LongTermFrameIdx) + 1;

					/* Set all long term bottom fields (opposite parity as current field) LongTermPicNum to 2 * LongTermFrameIdx */
					/* equation (8-34) */
					PARSER_Data_p->DPBFrameReference[i].BottomFieldLongTermPicNum = 2 * PARSER_Data_p->DPBFrameReference[i].LongTermFrameIdx;
				}
				else /* current field is a bottom field */
				{
					/* Set all short term top fields (opposite parity as current field) PicNum to 2 * FrameNumWrap */
					/* equation (8-31) */
					PARSER_Data_p->DPBFrameReference[i].TopFieldPicNum = 2 * PARSER_Data_p->DPBFrameReference[i].FrameNumWrap;

					/* Set all short term bottom fields (same parity as current field) PicNum to 2 * FrameNumWrap + 1*/
					/* equation (8-32) */
					PARSER_Data_p->DPBFrameReference[i].BottomFieldPicNum = (2 * PARSER_Data_p->DPBFrameReference[i].FrameNumWrap) + 1;

					/* Set all long term top fields (opposite parity as current field) LongTermPicNum to 2 * LongTermFrameIdx */
					/* equation (8-33) */
					PARSER_Data_p->DPBFrameReference[i].TopFieldLongTermPicNum = 2 * PARSER_Data_p->DPBFrameReference[i].LongTermFrameIdx;

					/* Set all long term bottom fields (same parity as current field) LongTermPicNum to 2 * LongTermFrameIdx + 1*/
					/* equation (8-34) */
					PARSER_Data_p->DPBFrameReference[i].BottomFieldLongTermPicNum = (2 * PARSER_Data_p->DPBFrameReference[i].LongTermFrameIdx) + 1;
				}
			} /* end of "if current picture is field" */
		} /* end of "if IsUsed" */
	} /* end of loop over the full DPBRef */
}

/******************************************************************************/
/*! \brief Mark the current picture as short term reference in DPBRef
 *
 * If the current picture is paired with an existing field in DPBRef, it is stored
 * within the same frame buffer
 * If the current picture is not paired with an existing field in DPBRef, it is
 * stored in a new frame buffer
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_MarkCurrentPictureAsShortTerm(const PARSER_Handle_t  ParserHandle)
{
    S8 FrameToMark;
    S8 FrameIndex;
 	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* First step: find if the current picture has a complementary field already stored in DPBRef */
	PARSER_Data_p->PictureLocalData.IsNonExisting = FALSE;
	FrameToMark = h264par_FindComplementaryField(ParserHandle);
	if (FrameToMark == -1)
	{
	    /* No complementary field has been found in DPBRef
		 * Thus the current picture is marked as short term in a free buffer, first
		 */
		FrameIndex = h264par_MarkCurrentAsShortTermInFreeFrameBuffer(ParserHandle);

        /* Check if a free frame buffer has been found */
        if (FrameIndex == -1)
        {
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
            DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.DPBRefFullError,
			"marking.c: (Do_MMCO6) error: no free buffer to store current picture in DPBRef\n");
			/* Stops treatment here to avoid bad data handling */
			return;
        }
    }
	else
    {
	    /* The current picture has a complementary field already stored in DPBRef.
	     * Mark current field as reference
	     */
		/* The frame buffer already contains some relevant information from the
		 * complementary field, such as:
		 * - frame_num
		 * - IsNonExisting
		 * Thus, no need to overwrite them.
		 * Just focus on specific members for this current field
		 */

		if (CurrentPictureIsTopField)
		{
			/* The current picture is the complementary field of a bottom field */
			PARSER_Data_p->DPBFrameReference[FrameToMark].TopFieldIsReference = TRUE;
		}
		else
		{
			/* No need to check for the current picture being a frame, because this picture
			 * has a complementary field in DPBRef and is not a top field.
			 * It shall then be a bottom field.
			 */
			PARSER_Data_p->DPBFrameReference[FrameToMark].BottomFieldIsReference = TRUE;
 		}
    }
}

/******************************************************************************/
/*! \brief Insert the current picture by applying the sliding window process
 *
 * if necessary, a frame buffer is freed before inserting the current picture
 * The current picture is marked as short-term reference
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_MarkPictureBySliding(const PARSER_Handle_t  ParserHandle)
{
	U8 NumShortTerm;
	U8 NumLongTerm;
	U8 FrameWithSmallestFrameNumWrap = VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES;  /* default value must be checked ... */
    S8 i; /* loop index to sweep over DPBRef */
    U8 MaxNumRefFrame1;

	struct MinimumFrameNumWrap_s
	{
		BOOL IsInitialised;
		S32 FrameNumWrap;
	} MinimumFrameNumWrap;

	S8 FreeDPBRefFrameBufferIndex; /* to detect which frame buffer is free while parsing the buffer */

	/* Dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* This is the second step of the sliding process: the incoming picture cannot be paired with an existing field.
	 * If the DPBRef is full, the parser removes the oldest short term frame buffer before inserting the new picture
	 */

	MinimumFrameNumWrap.IsInitialised = FALSE;
	MinimumFrameNumWrap.FrameNumWrap = 0;
	FreeDPBRefFrameBufferIndex = NO_FREE_FRAME_BUFFER_IN_DPBREF;

	/* Computes NumShortTerm and NumLongTerm in DPBRef */
	/* In the same loop, find out which frame has the smallest FrameNumWrap: this is for next step in the function */
	NumShortTerm = 0;
	NumLongTerm = 0;
    /* Do a descending order loop to get the first free frame index in the DPB */
	for  (i = VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES - 1; i >= 0 ; i--)
	{
		if (DPBRefFrameIsUsed(i))
		{
		    if (DPBRefTopFieldIsLongTerm(i) || DPBRefBottomFieldIsLongTerm(i))
			{
				/* the frame buffer contains at least a field that is marked as "used for long term" */
				NumLongTerm ++;
			}
		    if (DPBRefTopFieldIsShortTerm(i) || DPBRefBottomFieldIsShortTerm(i))
			{
				/* the frame buffer contains at least a field that is marked as "used for short term" */
				NumShortTerm ++;
				if (MinimumFrameNumWrap.IsInitialised == FALSE)
				{
					/* set first value for MinimumFrameNumWrap */
					MinimumFrameNumWrap.IsInitialised = TRUE;
					MinimumFrameNumWrap.FrameNumWrap = PARSER_Data_p->DPBFrameReference[i].FrameNumWrap;
					FrameWithSmallestFrameNumWrap = i;
				}
				else
				{
					if (PARSER_Data_p->DPBFrameReference[i].FrameNumWrap < MinimumFrameNumWrap.FrameNumWrap)
					{
					   MinimumFrameNumWrap.FrameNumWrap = PARSER_Data_p->DPBFrameReference[i].FrameNumWrap;
					   FrameWithSmallestFrameNumWrap = i;
					}
				}
			}
		}
		else /* this frame buffer is not in use */
		{
			FreeDPBRefFrameBufferIndex = i;
		}
	} /* end for */

    /* K012 corrigendum: check is performed with Max(num_ref_frames, 1) */
    MaxNumRefFrame1 = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->NumberOfReferenceFrames;
    if ( MaxNumRefFrame1 < 1)
    {
        MaxNumRefFrame1 = 1;
    }
    if (NumShortTerm + NumLongTerm == MaxNumRefFrame1)
	{
		if (NumShortTerm == 0)
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortTermToSlideError,
			"marking.c: (MarkPictureBySliding) error: no short term frame to free while sliding on DPB\n");
		}
        if (FrameWithSmallestFrameNumWrap != VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES)
        {
            /* Remove the frame with the minimum FrameNumWrap */
            h264par_MarkFrameAsUnusedForReference(FrameWithSmallestFrameNumWrap, ParserHandle);
            h264par_AssignShortTermInFrameBuffer(FrameWithSmallestFrameNumWrap, ParserHandle);
        }
	}
	else
	{
		/* a free buffer exists in the DPBRef: the current picture is stored there */
		if (FreeDPBRefFrameBufferIndex == NO_FREE_FRAME_BUFFER_IN_DPBREF)
		{
			/* There should be a free buffer, but despite the previous test, no free buffer has been found ! */
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.DPBRefFullError,
			"marking.c (MarkCurrentPictureBySliding) error: no free buffer to store incoming picture\n");
			/* Stops treatment here to avoid bad data handling */
			return;
		}
		h264par_AssignShortTermInFrameBuffer(FreeDPBRefFrameBufferIndex, ParserHandle);
	}
}

/******************************************************************************/
/*! \brief Finds the complementary field of the current picture (if any) in DPBRef.
 *
 *  - if the current picture is the second field of a CFP whose first field is
 *    a reference field (short or long) stored in the DPBRef, the function returns the
 *    FrameIndex in DPBRef where the 1st field is
 *  - This search occurs only on existing fields
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return FrameToMark the frame index in DPBRef where the first field is. If no field is found, the function returns -1
 */
/******************************************************************************/
S8 h264par_FindComplementaryField(const PARSER_Handle_t  ParserHandle)
{
	U8 i; /* loop index to sweep in DPBRef */
	BOOL EndOfLoop; /* TRUE as soon as the field is found */
	S8 FrameToMark;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	i = 0;
	EndOfLoop = FALSE;
	FrameToMark = -1;

	while ((i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) && (EndOfLoop == FALSE))
	{
		if (DPBRefFrameIsUsed(i))
		{
			/* If the two fields are already marked as reference, no need to check anything */
			if ((PARSER_Data_p->DPBFrameReference[i].TopFieldIsReference == FALSE) || (PARSER_Data_p->DPBFrameReference[i].BottomFieldIsReference == FALSE))
			{
				if (PARSER_Data_p->DPBFrameReference[i].frame_num == PARSER_Data_p->PictureLocalData.frame_num)
				{
					/* if frame_num of current picture is equal to frame_num of FrameBuffer in DPB */
					if (( DPBRefTopFieldIsRef(i) && (CurrentPictureIsBottomField))
			   		    ||
			   		    ( DPBRefBottomFieldIsRef(i) && (CurrentPictureIsTopField))
                       )
					{
						FrameToMark = i;
						EndOfLoop = TRUE;
					}
				}
			}
		}
		i++;
	}
	return (FrameToMark);
}
/******************************************************************************/
/*! \brief Performs the sliding window process on the DPB Ref.
 *
 *  - if the current picture is the second field of a CFP whose first field is
 *    a short-term reference field stored in the DPBRef, the second field is marked as
 *    short-term reference field and the pair is marked as "used for short term"
 *
 *  - Otherwise, the sliding window operates: if necessary, a frame buffer is freed.
 *    The current picture is then marked as short-term and stored in the DPBRef
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_PerformSlidingWindowOnDPBRef(const PARSER_Handle_t  ParserHandle)
{
	S8 FrameToMark; /* */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	/* First step: try to find if the complementary short-term reference field is already stored in
	 * DPBRef:
	 * Such a case occurs when the incoming field as opposite parity versus a
	 * previous field in decoding order, they share the same frame_num and
	 * the stored field has no complementary field already stored in the DPBRef
	 */
	FrameToMark = h264par_FindComplementaryField(ParserHandle);

	if (FrameToMark != -1)
	{
		/* The current picture has already its complementary field stored in DPBRef
		 * (at "FrameIndex"). Simply mark the current field as "used for short term reference"
		 */
		/* the i-th frame buffer contains now a complementary short-term reference field pair */
		if (PARSER_Data_p->DPBFrameReference[FrameToMark].TopFieldIsReference == TRUE)
		{
			/* The i-th frame buffer contained a top field. Let's assign the bottom field */
			PARSER_Data_p->DPBFrameReference[FrameToMark].BottomFieldIsReference = TRUE;
		}
		else
		{
			/* The i-th frame buffer contained a bottom field. Let's assign the top field */
			PARSER_Data_p->DPBFrameReference[FrameToMark].TopFieldIsReference = TRUE;
		}
		/* No need to update any other member of the incoming field as they are reset when the
		 * frame buffer is storing a new picture
		 */
	}
	else /* The current picture needs a new frame buffer in DPBRef */
	{
		h264par_MarkPictureBySliding(ParserHandle);
	}
}

/******************************************************************************/
/*! \brief Save the current picture context before applying the pre marking process
 *
 * As the same function calls are done whether the picture is existing or not
 * the goal of this function is to save any variable overridden during the
 * pre marking process to get them back after this process
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_SaveContextBeforePreMarking(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->PreMarkingContext.frame_num = PARSER_Data_p->PictureLocalData.frame_num;
    PARSER_Data_p->PreMarkingContext.IsIDR = PARSER_Data_p->PictureSpecificData.IsIDR;
    PARSER_Data_p->PreMarkingContext.PictureStructure = PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure;
    PARSER_Data_p->PreMarkingContext.IsReference = PARSER_Data_p->PictureGenericData.IsReference;
    PARSER_Data_p->PreMarkingContext.delta_pic_order_cnt[0] = PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[0];
    PARSER_Data_p->PreMarkingContext.delta_pic_order_cnt[1] = PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[1];

    /* Do not save the following because their parsing is done after this
     * function call:
     * - adaptive_ref_pic_marking_mode_flag
     */
}

/******************************************************************************/
/*! \brief Restore the current picture context after applying the pre marking process
 *
 * Reverse operation of "SaveContextBeforePreMarking
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_RestoreContextAfterPreMarking(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->PictureLocalData.frame_num = PARSER_Data_p->PreMarkingContext.frame_num;
    PARSER_Data_p->PictureSpecificData.IsIDR = PARSER_Data_p->PreMarkingContext.IsIDR;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = PARSER_Data_p->PreMarkingContext.PictureStructure;
    PARSER_Data_p->PictureGenericData.IsReference = PARSER_Data_p->PreMarkingContext.IsReference;
    PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[0] = PARSER_Data_p->PreMarkingContext.delta_pic_order_cnt[0];
    PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[1] = PARSER_Data_p->PreMarkingContext.delta_pic_order_cnt[1];
    PARSER_Data_p->PictureLocalData.IsNonExisting = FALSE;
}

/******************************************************************************/
/*! \brief Performs pre marking process
 *
 * This function performs the decoding process for gaps in frame_num.
 * The Virtual DPB is modified
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_PerformPreMarking(const PARSER_Handle_t  ParserHandle)
{
	U32 PrevRefFrameNum;
    U32 UnusedShortTermFrameNum;
    U32 CurrentPictureFrameNum;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    PrevRefFrameNum = PARSER_Data_p->PictureLocalData.PrevRefFrameNum;

    if (PARSER_Data_p->ActiveSPSLocalData_p->gaps_in_frame_num_allowed_flag == 0)
    {
        /* Error recovery, gap in frame num is not allowed, don't insert non existing reference pictures */
        if (PARSER_Data_p->PictureLocalData.IsRandomAccessPoint)
        {
            PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
        }
        else
        {
            PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
        }
        DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.UnintentionalLossOfPictureError,
        "marking.c: (PerformPreMarking) info: unintentional loss of picture\n");
        PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID ++;
        STTBX_Print(("marking.c: (PerformPreMarking) info: around DOFID = %d\n", PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID));

        if ((PARSER_Data_p->ParserGlobalVariable.ErrorRecoveryMode == STVID_ERROR_RECOVERY_HIGH) || (PARSER_Data_p->ParserGlobalVariable.ErrorRecoveryMode == STVID_ERROR_RECOVERY_FULL))
        {
            /* flush DBP ref */
            h264par_MarkAllPicturesAsUnusedForReference(ParserHandle);
        }
    }
    else
    {
        /* gap in frame num allowed, insert non existing reference pictures */
        /* Save the context from current slice for use after the pre-marking
        * Objective is to keep as much as possible the very same function calls
        * between non-existing and existing frames
        */
        h264par_SaveContextBeforePreMarking(ParserHandle);

        /* equation (7-10) */
        CurrentPictureFrameNum = PARSER_Data_p->PictureLocalData.frame_num;

        UnusedShortTermFrameNum = (PrevRefFrameNum + 1) % PARSER_Data_p->ActiveSPSLocalData_p->MaxFrameNum;
        /* The following parameters are constant over the non-existing frames */
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
        PARSER_Data_p->PictureSpecificData.IsIDR = FALSE;
        PARSER_Data_p->PictureGenericData.IsReference = TRUE;
        PARSER_Data_p->PictureLocalData.IsNonExisting = TRUE;
        PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[0] = 0;
        PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[1] = 0;
        PARSER_Data_p->PictureLocalData.HasMMCO5 = FALSE;

        while (UnusedShortTermFrameNum != CurrentPictureFrameNum)
        {
            /* Assign a new DecodingOrderFrameID for each non existing frame */
            PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID ++;

            /* Fill data structure to mark the current picture */
            PARSER_Data_p->PictureLocalData.frame_num = UnusedShortTermFrameNum;

            /* Update the Picture number in DPB before inserting the non-existing frame */
            h264par_DecodingProcessForPictureNumbers(ParserHandle);

            /* PicOrderCnt is not to be calculated when pic_order_cnt_type = 0 */
            if (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->pic_order_cnt_type != 0)
            {
                h264par_ComputePicOrderCnt(ParserHandle);
            }
            else
            {
                /* dummy value, as shall not be used by the reference building process */
                PARSER_Data_p->PictureSpecificData.PicOrderCntTop = DONT_CARE;
                PARSER_Data_p->PictureSpecificData.PicOrderCntBot = DONT_CARE;
            }
            /* Insert non existing frame in DPBRef */
            h264par_PerformSlidingWindowOnDPBRef(ParserHandle);
            PARSER_Data_p->DPBFrameReference[PARSER_Data_p->ParserGlobalVariable.CurrentPictureFrameIndexInDPBRef].DecodingOrderFrameID = PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID;
            PARSER_Data_p->DPBFrameReference[PARSER_Data_p->ParserGlobalVariable.CurrentPictureFrameIndexInDPBRef].TopFieldOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntTop;
            PARSER_Data_p->DPBFrameReference[PARSER_Data_p->ParserGlobalVariable.CurrentPictureFrameIndexInDPBRef].BottomFieldOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntBot;

            UnusedShortTermFrameNum = (UnusedShortTermFrameNum + 1) % PARSER_Data_p->ActiveSPSLocalData_p->MaxFrameNum;
            if ( UnusedShortTermFrameNum == 0 )
            {
                /* Frame number has wrapped. Since the presentation order ID is derived from
                * this, and this frame must be in the future of all frames that are still
                * hanging around in the producer (being pre-processed or decoded), we
                * need to advance the ExtensionID to keep the ordering correct in the
                * producer. */
                PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID ++;
            }
            /* Update previous picture in decoding order */
            h264par_UpdatePreviousPictureInfo(ParserHandle);

        }

        /* Restore context to go on the current existing slice */
        h264par_RestoreContextAfterPreMarking(ParserHandle);
    }
}


/******************************************************************************/
/*! \brief Computes PicNumX for MMCO=1 and MMCO=3 operations
 *
 * Computation is done with equation 8-40
 *
 * \param DifferenceOfPicNumsMinus1 the stream element
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return PicNumX the variable in the standard
 */
/******************************************************************************/
S32 h264par_ComputePicNumX(U32 DifferenceOfPicNumsMinus1, const PARSER_Handle_t  ParserHandle)
{
	S32 PicNumX; /* the variable in the standard used in MMCO = 1 */
	U32 CurrPicNum; /* the variable in the standard defined in the "frame_num" semantics */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if (CurrentPictureIsFrame)
	{
		/* same as to say if (field_pic_flag == 0) */
		CurrPicNum = PARSER_Data_p->PictureLocalData.frame_num;
	}
	else /* if current picture is a field */
	{
		CurrPicNum = (2 * PARSER_Data_p->PictureLocalData.frame_num) + 1;
	}

	/* equation (8-40) */
	PicNumX = CurrPicNum - (DifferenceOfPicNumsMinus1 + 1);

	return(PicNumX);
}

/******************************************************************************/
/*! \brief Apply Memory Management Control Operation = 1.
 *
 * A short term picture is marked as "unused for reference"
 *
 * \param DifferenceOfPicNumsMinus1 the stream element
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_DoMMCO1(U32 DifferenceOfPicNumsMinus1, const PARSER_Handle_t  ParserHandle)
{
	S32 PicNumX; /* the variable in the standard used in MMCO = 1 and MMCO=3 */
	S8 FrameIndex; /* index to sweep over the DPBRef */
	BOOL EndOfLoop; /* FALSE at init: becomes TRUE whenever the picture is found */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	PicNumX = h264par_ComputePicNumX(DifferenceOfPicNumsMinus1, ParserHandle);

	/* look for the frame whose PicNum (Top or Bottom Field) is equal to PicNumX */
	FrameIndex = 0;
	EndOfLoop = FALSE;
	while ((FrameIndex < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) && (EndOfLoop == FALSE))
	{
		if (DPBRefFrameHasShortTermField(FrameIndex))
		{
			if (CurrentPictureIsFrame)
			{
				if (PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldPicNum == PicNumX)
				{
					/* for a frame, the check is done on TopFieldPicNum only, as it is supposed to be the same for
					 * BottomFieldPicNum
					 */

					/* Mark the frame as "unused for reference" */
					h264par_MarkFrameAsUnusedForReference(FrameIndex, ParserHandle);
					EndOfLoop = TRUE;
				}
			}
			else
			{
				if (DPBRefTopFieldIsShortTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldPicNum == PicNumX))
				{
					/* Mark the top field as "unused for reference" */
					PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsReference = FALSE;
					EndOfLoop = TRUE;
				}
				else if (DPBRefBottomFieldIsShortTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldPicNum == PicNumX))
				{
					/* Mark the bottom field as "unused for reference" */
					PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsReference = FALSE;
					EndOfLoop = TRUE;
				}
			}
		}
		FrameIndex ++;
	}

    if (EndOfLoop == FALSE)
	{
		PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortTermToMarkAsUnusedError,
		"marking.c: (DoMMCO1) error: could not find short term reference to mark as unused for reference\n");
	}
}

/******************************************************************************/
/*! \brief Apply Memory Management Control Operation = 2.
 *
 * A long term picture is marked as "unused for reference"
 *
 * \param LongTermPicNum the stream element
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_DoMMCO2(U32 LongTermPicNum, const PARSER_Handle_t  ParserHandle)
{
	S8 FrameIndex; /* index to sweep over the DPBRef */
	BOOL EndOfLoop; /* FALSE at init: becomes TRUE whenever the picture is found */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	/* look for the picture (frame/field) whose LongTermPicNum (Top or Bottom Field) is equal to LongTermPicNum */
	FrameIndex = 0;
	EndOfLoop = FALSE;
	while ((FrameIndex < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) && (EndOfLoop == FALSE))
	{
		if (DPBRefFrameHasLongTermField(FrameIndex))
		{
			if (CurrentPictureIsFrame)
			{
				if (PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldLongTermPicNum == LongTermPicNum)
				{
					/* for a frame, the check is done on TopFieldLongTermPicNum only, as it is
                     * supposed to be the same for BottomFieldLongTermPicNum
					 */

					/* Mark the frame as "unused for reference" */
					h264par_MarkFrameAsUnusedForReference(FrameIndex, ParserHandle);
					EndOfLoop = TRUE;
				}
			}
			else
			{
				if (DPBRefTopFieldIsLongTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldLongTermPicNum == LongTermPicNum))
				{
					/* Mark the top field as "unused for reference" */
					PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsReference = FALSE;
					EndOfLoop = TRUE;
				}
				else if (DPBRefBottomFieldIsLongTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldLongTermPicNum == LongTermPicNum))
				{
					/* Mark the bottom field as "unused for reference" */
					PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsReference = FALSE;
					EndOfLoop = TRUE;
				}
			}
		}
        FrameIndex ++;
    }

    if (EndOfLoop == FALSE)
	{
   		PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoLongTermToMarkAsUnusedError,
		"marking.c: (DoMMCO2) error: could not find long term reference to mark as unused for reference\n");
	}
}

/******************************************************************************/
/*! \brief Mark the long term picture assigned to LongTermFrameIdx as "unused for reference"
 *
 * if the frame index of such a picture is equal to FrameToMark, then, the picture
 * marking is not updated by the function.
 * This function is the used for MMCO=3 and MMCO=6 operations
 *
 * \param LongTermFrameIdx the stream element
 * \param FrameToMark the frame index of the picture not to update
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_MarkPictureWithLongTermFrameIdxAsUnusedForReference(U8 LongTermFrameIdx, S8 FrameToMark, const PARSER_Handle_t  ParserHandle)
{
	U8 FrameIndex;
	BOOL EndOfLoop;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	FrameIndex = 0;
	EndOfLoop = FALSE;
	while ((FrameIndex < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) && (EndOfLoop == FALSE))
	{
		if ( (DPBRefTopFieldIsLongTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].LongTermFrameIdx == LongTermFrameIdx)) ||
			 (DPBRefBottomFieldIsLongTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].LongTermFrameIdx == LongTermFrameIdx))
		   )
		{
			if (FrameIndex != FrameToMark)
			{
				/* the Frame or field that has the same LongTermFrameIdx as the one to use for marking is not
				 * the one to mark: thus, set this frame or CFP as "unused for reference"
				 */
				/* When LongTermFrameIdx equal to long_term_frame_idx is already assigned to
                 * a long-term reference frame or a long-term complementary reference field
                 * pair, that frame or complementary field pair and both of its fields are
                 * marked as "unused for reference"
				 */
				h264par_MarkFrameAsUnusedForReference(FrameIndex, ParserHandle);
				EndOfLoop = TRUE;
			}
		}
		FrameIndex++;
	}
}

/******************************************************************************/
/*! \brief Apply Memory Management Control Operation = 3.
 *
 * A short term picture is marked as "used for long term reference"
 * Possibly, the long term picture that has the same LongTermFrameIdx is
 * marked as "unused for reference"
 *
 * \param DifferenceOfPicNumsMinus1 the stream element
 * \param LongTermFrameIdx the stream element
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_DoMMCO3(U32 DifferenceOfPicNumsMinus1, U8 LongTermFrameIdx, const PARSER_Handle_t  ParserHandle)
{
	S32 PicNumX; /* the variable in the standard used in MMCO = 1 and MMCO = 3 */
	S8 FrameIndex; /* index to sweep over the DPBRef */
	BOOL EndOfLoop; /* FALSE at init: becomes TRUE whenever the picture is found */
	S8 FrameToMark; /* the frame index to mark as long term */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	PicNumX = h264par_ComputePicNumX(DifferenceOfPicNumsMinus1, ParserHandle);

	/* First step: find out which FrameBuffer contains the picture to mark */
	FrameIndex = 0;
	EndOfLoop = FALSE;
	FrameToMark = -1;
	while ((FrameIndex < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) && (EndOfLoop == FALSE))
	{
		if ( (DPBRefTopFieldIsShortTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldPicNum == PicNumX)) ||
		     (DPBRefBottomFieldIsShortTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldPicNum == PicNumX))
		   )
		{
			FrameToMark = FrameIndex;
			if (PARSER_Data_p->DPBFrameReference[FrameIndex].IsNonExisting == TRUE)
			{
				PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
				DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.MarkNonExistingAsLongError,
				"marking.c: (DoMMCO3) error: attempting to mark a non existing frame as long term reference\n");
			}
			EndOfLoop = TRUE;
		}
		FrameIndex ++;
	}
	if (FrameToMark == -1)
	{
		PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortPictureToMarkAsLongError,
		"marking.c: (DoMMCO3) error: could not find short term picture to mark\n");
		/* Stops treatment here to avoid bad data handling */
		return;
	}

	/* Second step: unmark the frame or field that is already marked as long term
     * with LongTermFrameIdx and which is not the complementary field of the field to mark
	 */
	h264par_MarkPictureWithLongTermFrameIdxAsUnusedForReference(LongTermFrameIdx, FrameToMark, ParserHandle);

	/* Third step: mark the frame or field mentionned by PicNumX as "used for long term" and assign
	 * LongTermFrameIdx to that frame or field */
	h264par_MarkPicNumXAsLongTerm(FrameToMark, PicNumX, LongTermFrameIdx, ParserHandle);
}

/******************************************************************************/
/*! \brief Apply Memory Management Control Operation = 4.
 *
 *  The global variable MaxLongTermFrameIdx is updated
 *
 * \param MaxLongTermFrameIdxPlus1 the stream element
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_DoMMCO4(U8 MaxLongTermFrameIdxPlus1, const PARSER_Handle_t  ParserHandle)
{
	S8 FrameIndex; /* index to sweep over the DPBRef */
 	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	/* Mark long term pictures whose LongTermFrameIdx > MaxLongTermFrameIdxPlus1 -1
	 * as "unused for reference"
	 */

	FrameIndex = 0;
	while (FrameIndex < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES)
	{
	    if (DPBRefFrameHasLongTermField(FrameIndex))
	    {
		    if ((MaxLongTermFrameIdxPlus1 == 0) ||
                (PARSER_Data_p->DPBFrameReference[FrameIndex].LongTermFrameIdx > (U8)(MaxLongTermFrameIdxPlus1 - 1))
               ) /* casting to U8 because LongTermFrameIdx is U8 and MaxLongTermFrameIdxPlus1 is U8 */
		    {
		    	h264par_MarkFrameAsUnusedForReference(FrameIndex, ParserHandle);
		    }
	    }
		FrameIndex++;
	}

	if (MaxLongTermFrameIdxPlus1 == 0)
	{
		PARSER_Data_p->ParserGlobalVariable.MaxLongTermFrameIdx = NO_LONG_TERM_FRAME_INDICES;
	}
	else /* MaxLongTermFrameIdxPlus1 > 0 */
	{
		PARSER_Data_p->ParserGlobalVariable.MaxLongTermFrameIdx = MaxLongTermFrameIdxPlus1 - 1;
	}
}

/******************************************************************************/
/*! \brief Post process POC in picture with MMCO=5
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_PostProcessPicOrderCntOnMMCO5 (const PARSER_Handle_t  ParserHandle)
{
    S32 TempPicOrderCnt;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* Performs post decoding update of POC if the picture contains a MMCO=5 operation */

    /* When the current picture includes a memory management control operation equal to 5,
     * after the decoding of the current picture, tempPicOrderCnt is set equal to
     * PicOrderCnt( CurrPic ), TopFieldOrderCnt of the current picture (if any) is set equal
     * to TopFieldOrderCnt - tempPicOrderCnt, and BottomFieldOrderCnt of the current
     * picture (if any) is set equal to BottomFieldOrderCnt - tempPicOrderCnt.
     */
    TempPicOrderCnt = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCnt;

    /* update for top field pictures: TopFieldOrderCnt and POC */
	if (CurrentPictureIsTopField )
	{
    	PARSER_Data_p->PictureSpecificData.PicOrderCntTop = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop - TempPicOrderCnt;
	}

    /* update for bottom field pictures BottomFieldOrderCnt and POC */
	if (CurrentPictureIsBottomField)
    {
	    PARSER_Data_p->PictureSpecificData.PicOrderCntBot = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot - TempPicOrderCnt;
	}

    if (CurrentPictureIsFrame)
    {
	    PARSER_Data_p->PictureSpecificData.PicOrderCntTop = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop - TempPicOrderCnt;
	    PARSER_Data_p->PictureSpecificData.PicOrderCntBot = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot - TempPicOrderCnt;
	}
}

/******************************************************************************/
/*! \brief Post process frame_num in picture with MMCO=5
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_PostProcessFrameNumOnMMCO5(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* after the decoding of the current picture and the processing of the memory
     * management control operations, the picture shall be inferred to have had
     * frame_num equal to 0 for all subsequent use in the decoding process
     */

    /* Here, we force frame_num for current picture to 0. The picture will then be stored
     * in the DPBRef with a frame_num equal to 0
     */

    /* Do not need to do anything else as frame_num is used only in the parser */
    PARSER_Data_p->PictureLocalData.frame_num = 0;
}

/******************************************************************************/
/*! \brief Apply Memory Management Control Operation = 5.
 *
 * The Virtual DPB is modified
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_DoMMCO5(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Save MMCO=5 operation for current picture */
    PARSER_Data_p->PictureLocalData.HasMMCO5 = TRUE;

    if (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->pic_order_cnt_type == 0)
    {
	    /* Check on delta_pic_order_cnt_bottom once we know that current picture
	     * has a MMCO = 5 command. See semantics
	     */

        /* Check is performed only when pic_order_cnt_type is 0, meaning that
         * MaxPicOrderCntLsb is meaningful
         */
        /* Casting to S32 because MaxPicOrderCntLsb is U32 */
        if (PARSER_Data_p->PictureLocalData.delta_pic_order_cnt_bottom < (S32)(1 - (S32)PARSER_Data_p->ActiveSPSLocalData_p->MaxPicOrderCntLsb))
	    {
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
    		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.DeltaPicOrderCntBottomRangeError,
			"marking.c: (DoMMCO5) error: delta_pic_order_cnt_bottom out of range\n");
    	}
    }

	h264par_MarkAllPicturesAsUnusedForReference(ParserHandle);
	PARSER_Data_p->ParserGlobalVariable.MaxLongTermFrameIdx = NO_LONG_TERM_FRAME_INDICES;

    /* Perform post processing on the picture, once the marking has been done */
    h264par_PostProcessPicOrderCntOnMMCO5(ParserHandle);

    h264par_PostProcessFrameNumOnMMCO5(ParserHandle);

    /* A MMCO=5 picture starts a new sequence */
    PARSER_Data_p->PictureLocalData.IncrementEPOID = TRUE;
}

/******************************************************************************/
/*! \brief Apply Memory Management Control Operation = 6.
 *
 * The current picture is marked as long term
 * If any, the picture marked with the same LongTermFrameIdx as the current picture
 * is marked as "unused for reference"
 *
 * \param LongTermFrameIdx the stream element
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_DoMMCO6(U8 LongTermFrameIdx, const PARSER_Handle_t  ParserHandle)
{
	S8 FrameToMark;
	S8 FrameIndex;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	/* First step: find if the current picture has a complementary field already stored in DPBRef */
	PARSER_Data_p->PictureLocalData.IsNonExisting = FALSE;
	FrameToMark = h264par_FindComplementaryField(ParserHandle);

	/* Second step: unmark the frame or field that is already mark as long term with LongTermFrameIdx and which is
	 * not the complementary field of the field to mark
	 */
	h264par_MarkPictureWithLongTermFrameIdxAsUnusedForReference(LongTermFrameIdx, FrameToMark, ParserHandle);

	if (FrameToMark == -1)
	{
		/* No complementary field has been found in DPBRef
		 * Thus the current picture is marked as short term in a free buffer, first
		 */
		FrameIndex = h264par_MarkCurrentAsShortTermInFreeFrameBuffer(ParserHandle);

        /* Check if a free frame buffer has been found */
        if (FrameIndex == -1)
        {
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
            DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.DPBRefFullError,
			"marking.c: (Do_MMCO6) error: no free buffer to store current picture in DPBRef\n");
			/* Stops treatment here to avoid bad data handling */
			return;
        }
		/* Then mark it as long term picture */
		h264par_MarkCurrentPictureAsLongTerm(FrameIndex, LongTermFrameIdx, ParserHandle);
	}
	else
	{
		/* The current picture has a complementary field already stored in DPBRef.
		 * Mark current field as reference
		 */
		/* The frame buffer already contains some relevant information from the
		 * complementary field, such as:
		 * - frame_num
		 * - IsNonExisting
		 * Thus, no need to overwrite them.
		 * Just focus on specific members for this current field
		 */

		if (CurrentPictureIsTopField)
		{
			/* The current picture is the complementary field of a bottom field */
			PARSER_Data_p->DPBFrameReference[FrameToMark].TopFieldIsReference = TRUE;
 		   	PARSER_Data_p->DPBFrameReference[FrameToMark].TopFieldIsLongTerm = TRUE;
		}
		else
		{
			/* No need to check for the current picture being a frame, because this picture
			 * has a complementary field in DPBRef and is not a top field.
			 * It shall then be a bottom field.
			 */
			PARSER_Data_p->DPBFrameReference[FrameToMark].BottomFieldIsReference = TRUE;
		   	PARSER_Data_p->DPBFrameReference[FrameToMark].BottomFieldIsLongTerm = TRUE;
 		}

		/* The second field shall have the same LongTermFrameIdx as the first field
		 * This is not mentionned in the standard, but seems obvious as the CFP has
		 * only one LongTermFrameIdx
		 */
		if (LongTermFrameIdx != PARSER_Data_p->DPBFrameReference[FrameToMark].LongTermFrameIdx)
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.Marking2ndFieldWithDifferentLongTermFrameIdxError,
			"marking.c: (DoMMCO6) error: marking the second field as long term with a different LongTermFrameIdx as for the first field\n");
		}
	}
}


/******************************************************************************/
/*! \brief Mark the Top and/or Bottom Fields member of a given Frame Buffer as long term
 *
 * The picture that matches picnum = PicNumX (top and/or bottom fields) is marked as
 * long term
 *
 * \param FrameIndex the frame buffer to update
 * \param PicNumX the picture(s) having picnum = PicNumX are marked
 * \param LongTermFrameIdx the LongTermFrameIdx parameter to use for marking
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_MarkPicNumXAsLongTerm(U8 FrameIndex, S32 PicNumX, U8 LongTermFrameIdx, const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* In case the picture was already a Long Term Picture, check if LongTermFrameIdx is equal to
	 * LongTermFrameIdx already assigned
	 */
	if (DPBRefTopFieldIsLongTerm(FrameIndex) || DPBRefBottomFieldIsLongTerm(FrameIndex))
	{
		if (PARSER_Data_p->DPBFrameReference[FrameIndex].LongTermFrameIdx != LongTermFrameIdx)
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.Marking2ndFieldWithDifferentLongTermFrameIdxError,
			"marking.c: (MarkLongTermPicture) error: assigning a LongTermFrameIdx for second field different from first field\n");
		}
		/* Else, ok: no need to rewrite the same LongTermFrameIdx */
	}
	else
	{
		PARSER_Data_p->DPBFrameReference[FrameIndex].LongTermFrameIdx = LongTermFrameIdx;
	}

    /* Mark top field, if matches PicNumX */
    if (DPBRefTopFieldIsShortTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldPicNum == PicNumX))
    {
		PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsLongTerm = TRUE;
    }

    /* Mark bottom field, if matches PicNumX */
    if (DPBRefBottomFieldIsShortTerm(FrameIndex) && (PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldPicNum == PicNumX))
    {
		PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsLongTerm = TRUE;
    }
}

/******************************************************************************/
/*! \brief Mark the Top and/or Bottom Fields member of a given Frame Buffer as long term
 *
 * The current picture (top and/or bottom fields) is marked as long term
 *
 * \param FrameIndex the frame buffer to update
 * \param LongTermFrameIdx the LongTermFrameIdx parameter to use for marking
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_MarkCurrentPictureAsLongTerm(U8 FrameIndex, U8 LongTermFrameIdx, const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* In case the picture was already a Long Term Picture, check if LongTermFrameIdx is equal to
	 * LongTermFrameIdx already assigned
	 */
	if ( (PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsLongTerm == TRUE) ||
		 (PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsLongTerm == TRUE)
	   )
	{
		if (PARSER_Data_p->DPBFrameReference[FrameIndex].LongTermFrameIdx != LongTermFrameIdx)
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.Marking2ndFieldWithDifferentLongTermFrameIdxError,
			"marking.c: (MarkLongTermPicture) error: assigning a LongTermFrameIdx for second field different from first field\n");
		}
		/* Else, ok: no need to rewrite the same LongTermFrameIdx */
	}
	else
	{
		PARSER_Data_p->DPBFrameReference[FrameIndex].LongTermFrameIdx = LongTermFrameIdx;
	}

	/* Mark the corresponding fields or frame */
	if (CurrentPictureIsFrame)
	{
		/* Mark both fields */
		PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsLongTerm = TRUE;
		PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsLongTerm = TRUE;
	}
	else if (CurrentPictureIsTopField)
	{
		PARSER_Data_p->DPBFrameReference[FrameIndex].TopFieldIsLongTerm = TRUE;
	}
	else /* the current picture is a bottom field */
	{
		PARSER_Data_p->DPBFrameReference[FrameIndex].BottomFieldIsLongTerm = TRUE;
	}
}

/******************************************************************************/
/*! \brief Marks all the pictures in the DPBRef as unused for reference
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_MarkAllPicturesAsUnusedForReference(const PARSER_Handle_t  ParserHandle)
{
	U8 i; /* index loop to sweep over DPBRef */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES ; i++)
	{
		h264par_MarkFrameAsUnusedForReference(i, ParserHandle);
	}
}

/******************************************************************************/
/*! \brief Finds a free frame buffer in the DPBRef
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return FrameIndex the index of the free frame buffer in DPBRef (-1 if
 *         no free frame buffer)
 */
/******************************************************************************/
S8 h264par_FindFreeFrameIndex(const PARSER_Handle_t  ParserHandle)
{
	S8 FrameIndex; /* loop index to sweep over DPBRef */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	FrameIndex = 0;
	while ((FrameIndex < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) && DPBRefFrameIsUsed(FrameIndex))
	{
		FrameIndex++;
	}
	if (FrameIndex == VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES)
	{
		/* the end of buffer has been reached without finding any free buffer */
		FrameIndex = -1;
	}
	return (FrameIndex);
}

/******************************************************************************/
/*! \brief Mark the current picture as short term reference in a free buffer
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return FrameIndex set to -1 means no frame buffer is free
 */
/******************************************************************************/
S8 h264par_MarkCurrentAsShortTermInFreeFrameBuffer(const PARSER_Handle_t  ParserHandle)
{
	S8 FrameIndex;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	FrameIndex = h264par_FindFreeFrameIndex(ParserHandle);
	if (FrameIndex == -1)
	{
		PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.DPBRefFullError,
		"marking.c: (MarkCurrentAsShortTermInFreeFrameBuffer) error: no free buffer to store current picture as long term\n");
		/* Stops treatment here to avoid bad data handling */
        return(FrameIndex);
	}
	PARSER_Data_p->PictureLocalData.IsNonExisting = FALSE;
    h264par_AssignShortTermInFrameBuffer(FrameIndex, ParserHandle);

    return(FrameIndex);
}

/******************************************************************************/
/*! \brief Check the DPB fullness, if necessary, a frame buffer is freed
 * to keep number of active references fitting the stream's max reference number
 * This function must be called only within MMCO (sliding window is properly managed in non MMCO cases)
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ClipDPBAfterMMCO(const PARSER_Handle_t  ParserHandle)
{
	U8 NumShortTerm;
	U8 NumLongTerm;
	U8 FrameWithSmallestFrameNumWrap = VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES;  /* default value must be checked ... */
    S8 i; /* loop index to sweep over DPBRef */
    U8 MaxNumRefFrame1;

	struct MinimumFrameNumWrap_s
	{
		BOOL IsInitialised;
		S32 FrameNumWrap;
	} MinimumFrameNumWrap;

	/* Dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* This is the second step of the sliding process: the incoming picture cannot be paired with an existing field.
	 * If the DPBRef is full, the parser removes the oldest short term frame buffer before inserting the new picture
	 */

	MinimumFrameNumWrap.IsInitialised = FALSE;
	MinimumFrameNumWrap.FrameNumWrap = 0;

	/* Computes NumShortTerm and NumLongTerm in DPBRef */
	/* In the same loop, find out which frame has the smallest FrameNumWrap: this is for next step in the function */
	NumShortTerm = 0;
	NumLongTerm = 0;
    /* Do a descending order loop to get the first free frame index in the DPB */
	for  (i = VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES - 1; i >= 0 ; i--)
	{
		if (DPBRefFrameIsUsed(i))
		{
		    if (DPBRefTopFieldIsLongTerm(i) || DPBRefBottomFieldIsLongTerm(i))
			{
				/* the frame buffer contains at least a field that is marked as "used for long term" */
				NumLongTerm ++;
			}
		    if (DPBRefTopFieldIsShortTerm(i) || DPBRefBottomFieldIsShortTerm(i))
			{
				/* the frame buffer contains at least a field that is marked as "used for short term" */
				NumShortTerm ++;
				if (MinimumFrameNumWrap.IsInitialised == FALSE)
				{
					/* set first value for MinimumFrameNumWrap */
					MinimumFrameNumWrap.IsInitialised = TRUE;
					MinimumFrameNumWrap.FrameNumWrap = PARSER_Data_p->DPBFrameReference[i].FrameNumWrap;
					FrameWithSmallestFrameNumWrap = i;
				}
				else
				{
					if (PARSER_Data_p->DPBFrameReference[i].FrameNumWrap < MinimumFrameNumWrap.FrameNumWrap)
					{
					   MinimumFrameNumWrap.FrameNumWrap = PARSER_Data_p->DPBFrameReference[i].FrameNumWrap;
					   FrameWithSmallestFrameNumWrap = i;
					}
				}
			}
		}
	} /* end for */

    /* K012 corrigendum: check is performed with Max(num_ref_frames, 1) */
    MaxNumRefFrame1 = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->NumberOfReferenceFrames;
    if ( MaxNumRefFrame1 < 1)
    {
        MaxNumRefFrame1 = 1;
    }
    if (NumShortTerm + NumLongTerm > MaxNumRefFrame1)
	{
		if (NumShortTerm == 0)
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortTermToSlideError,
            "marking.c: (h264par_ClipDPBAfterMMCO) error: no short term frame to free while sliding on DPB\n");
		}
        if (FrameWithSmallestFrameNumWrap != VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES)
        {
            /* Remove the frame with the minimum FrameNumWrap */
            h264par_MarkFrameAsUnusedForReference(FrameWithSmallestFrameNumWrap, ParserHandle);
        }
	}
} /* end of h264par_ClipDPBAfterMMCO() */

