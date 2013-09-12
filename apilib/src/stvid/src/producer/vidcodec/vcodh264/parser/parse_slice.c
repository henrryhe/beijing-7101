/*!
 *******************************************************************************
 * \file parse_slice.c
 *
 * \brief H264 Slice header parsing and filling of data structures
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) STMicroelectronics 2006
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

/* H264 Parser specific */
#include "h264parser.h"
#ifdef PARSER_DEBUG_PATCH_ALLOWED
static BOOL PatchFrameRateEnabled = FALSE;
static U32  PatchFrameRateValue = 60000;
static BOOL PatchScanTypeEnabled  = FALSE;
static STGXOBJ_ScanType_t PatchScanTypeValue  = STGXOBJ_PROGRESSIVE_SCAN;
#endif /* PARSER_DEBUG_PATCH_ALLOWED */

#ifdef ST_XVP_ENABLE_FGT
static U32 LastIDR_IdrPicId;
#endif

/* Don't infers the picture is progressive if both field share the same instant.
 * This lead to consider interlaced stream as progressive with some streams encoded by Tandberg
 * The issue is known by Tandberg and fixed but we have no way to check the origin of the stream
 * So frame pictures without SEI:pic_struct will be always considered as INTERLACED
*/
#undef MARK_PICTURE_AS_PROGRESSIVE_IF_BOTH_FIELDS_SHARE_SAME_INSTANT

/* Functions ---------------------------------------------------------------- */
static void h264par_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle);

/******************************************************************************/
/*! \brief Update the "PreviousPicture" data structure with current picture
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_UpdatePreviousPictureInfo(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure = PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure;
    PARSER_Data_p->PreviousPictureInDecodingOrder.HasMMCO5 = PARSER_Data_p->PictureLocalData.HasMMCO5;
    PARSER_Data_p->PreviousPictureInDecodingOrder.FrameNumOffset = PARSER_Data_p->PictureLocalData.FrameNumOffset;
	/* fram_num is always the frame_num after decoding (if last picture had mmco=5, frame_num is already set to 0 */
    PARSER_Data_p->PreviousPictureInDecodingOrder.frame_num = PARSER_Data_p->PictureLocalData.frame_num;
	PARSER_Data_p->PreviousPictureInDecodingOrder.IsIDR = PARSER_Data_p->PictureSpecificData.IsIDR;
    PARSER_Data_p->PreviousPictureInDecodingOrder.IsFirstOfTwoFields = PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields;
    PARSER_Data_p->PreviousPictureInDecodingOrder.IsReference = PARSER_Data_p->PictureGenericData.IsReference;
	PARSER_Data_p->PreviousPictureInDecodingOrder.Width = PARSER_Data_p->PictureLocalData.Width;
	PARSER_Data_p->PreviousPictureInDecodingOrder.Height = PARSER_Data_p->PictureLocalData.Height;
	PARSER_Data_p->PreviousPictureInDecodingOrder.MaxDecFrameBuffering = PARSER_Data_p->PictureLocalData.MaxDecFrameBuffering;
	PARSER_Data_p->PreviousPictureInDecodingOrder.IdrPicId = PARSER_Data_p->PictureLocalData.IdrPicId;

	/* If the previous picture was a reference picture, then, update also the PreviousReferencePictureInDecodingOrder structure */
	if (PARSER_Data_p->PictureGenericData.IsReference)
	{
       	PARSER_Data_p->PreviousReferencePictureInDecodingOrder.HasMMCO5 = PARSER_Data_p->PictureLocalData.HasMMCO5;
        PARSER_Data_p->PreviousReferencePictureInDecodingOrder.PictureStructure = PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure;
        /* Take TopFieldOrderCnt (is the reference to take for next picture) */
        PARSER_Data_p->PreviousReferencePictureInDecodingOrder.TopFieldOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntTop;
	    PARSER_Data_p->PreviousReferencePictureInDecodingOrder.PicOrderCntMsb = PARSER_Data_p->PictureLocalData.PicOrderCntMsb;
		PARSER_Data_p->PreviousReferencePictureInDecodingOrder.pic_order_cnt_lsb = PARSER_Data_p->PictureLocalData.pic_order_cnt_lsb;
		PARSER_Data_p->PreviousReferencePictureInDecodingOrder.frame_num = PARSER_Data_p->PictureLocalData.frame_num;
	}
	else
	{
		/* the current picture is not a reference picture, thus we do not update the
		 * "PreviousReferencePictureInDecodingOrder" data structure
		 */
	}
}

#ifdef STVID_PARSER_CHECK
/******************************************************************************/
/*! \brief Performs semantics checks on pic_order_cnt_type
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_SemanticsChecksPicOrderCntType(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    if (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->pic_order_cnt_type == 2)
	{
		/* pic_order_cnt_type shall not be equal to 2 in a coded video sequence that contains any of the following: */

		/* -	an access unit containing a non-reference frame followed immediately by an access unit containing
		 *      a non-reference picture
		 */
		if ((PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE) &&
			(PARSER_Data_p->PreviousPictureInDecodingOrder.IsReference == FALSE) &&
			(PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) &&
			(PARSER_Data_p->PictureGenericData.IsReference == TRUE)
		   )
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.PicOrderCntType2SemanticsError,
			"parse_slice.c: (SemanticsChecksPicOrderCntType) error: semantics violation on pic_order_cnt_type = 2\n");
		}

		/* -	two access units each containing a field with the two fields together forming a complementary
		 *      non-reference field pair followed immediately by an access unit containing a non-reference picture
		 */
		if ((PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE) &&
			(PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME) &&
			(PARSER_Data_p->PreviousPictureInDecodingOrder.IsFirstOfTwoFields == FALSE) &&
			(PARSER_Data_p->PreviousPictureInDecodingOrder.IsReference == FALSE) &&
			(PARSER_Data_p->PictureGenericData.IsReference == TRUE)
		   )
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.PicOrderCntType2SemanticsError,
			"parse_slice.c: (SemanticsChecksPicOrderCntType) error: semantics violation on pic_order_cnt_type = 2\n");
		}

		/* -	an access unit containing a non-reference field followed immediately by an access unit containing
		 *      another non-reference picture that does not form a complementary non-reference field pair with the
		 *      first of the two access units
		 */
		if ((PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE) &&
			(PARSER_Data_p->PreviousPictureInDecodingOrder.IsReference == FALSE) &&
			(PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME) &&
			(PARSER_Data_p->PictureGenericData.IsReference == FALSE) &&
			(PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields == TRUE)
		   )
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.PicOrderCntType2SemanticsError,
			"parse_slice.c: (SemanticsChecksPicOrderCntType) error: semantics violation on pic_order_cnt_type = 2\n");
		}
	}
}
#endif /* STVID_PARSER_CHECK */

/******************************************************************************/
/*! \brief Insert the current picture in ListD
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
#ifdef STVID_PARSER_CHECK
void h264par_InsertCurrentPictureInListD(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
	/* If the ListDNumberOfElements exceeds the ListD array limit, the
	 * current picture is not inserted in the ListD.
	 */

	/* Includes current picture in List D */
	if (CurrentPictureIsFrame)
	{
		if (PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements < MAX_PIC_BETWEEN_DPBREF_FLUSH - 1) /* need 2 free elements to store a frame */
		{
			/* Fill Top field information in ListD */
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].FieldOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntTop;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].DecodingOrderFrameID = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].IsTopField = TRUE;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].PicOrderCnt = h264par_PicOrderCnt(PARSER_Data_p->PictureSpecificData.PicOrderCntTop, PARSER_Data_p->PictureSpecificData.PicOrderCntBot);
			PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements++;

			/* Fill Bottom field information in ListD */
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].FieldOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntBot;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].DecodingOrderFrameID = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].IsTopField = FALSE;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].PicOrderCnt = h264par_PicOrderCnt(PARSER_Data_p->PictureSpecificData.PicOrderCntTop, PARSER_Data_p->PictureSpecificData.PicOrderCntBot);
			PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements++;
		}
		else
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
    		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.ListDOverFlowError,
					"parse_slice.c: (InsertCurrentPictureInListD) ListD overflow\n");
		}
	}
	else if (CurrentPictureIsTopField)
	{
		if (PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements < MAX_PIC_BETWEEN_DPBREF_FLUSH) /* need 1 free element to store a field */
		{
			/* Fill Top field information in ListD */
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].FieldOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntTop;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].DecodingOrderFrameID = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].IsTopField = TRUE;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].PicOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntTop;
			PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements++;
		}
		else
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
    		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.ListDOverFlowError,
					"parse_slice.c: (InsertCurrentPictureInListD) ListD overflow\n");
		}
	}
	else /* current picture is a bottom field */
	{
		if (PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements < MAX_PIC_BETWEEN_DPBREF_FLUSH) /* need 1 free element to store a field */
		{
			/* Fill Bottom field information in ListD */
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].FieldOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntBot;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].DecodingOrderFrameID = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].IsTopField = FALSE;
			PARSER_Data_p->ParserGlobalVariable.ListD[PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements].PicOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntBot;
			PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements++;
		}
		else
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
    		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.ListDOverFlowError,
					"parse_slice.c: (InsertCurrentPictureInListD) ListD overflow\n");
		}
	}
}
#endif /* STVID_PARSER_CHECK */

#ifdef STVID_PARSER_CHECK
/******************************************************************************/
/*! \brief Comparison function used by qsort to perform an ascending sort
 *         on the ListO elements
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
S32 h264par_FieldOrderCntListOCmpFunc(const void *elem1, const void *elem2)
{
    if ( (*(ListD_t *)elem1).FieldOrderCnt > (*(ListD_t *)elem2).FieldOrderCnt)
    {
        return 1;
    }
    else if ( (*(ListD_t *)elem1).FieldOrderCnt < (*(ListD_t *)elem2).FieldOrderCnt)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}
#endif /* STVID_PARSER_CHECK */

#ifdef STVID_PARSER_CHECK
/******************************************************************************/
/*! \brief Build ListO from ListD. This is to check FieldOrderCnt serie
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_BuildListO(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
	/* the list variable listO [...] contains the elements of listD sorted in ascending order */
	/* Note: the sort is done on the FieldOrderCnt member of ListD data structure */
	/*       the other members are then used to perform the checks */

	/* Copy ListD into ListO */
    (void *)memcpy((void *)PARSER_Data_p->ParserGlobalVariable.ListO, (const void *)PARSER_Data_p->ParserGlobalVariable.ListD, sizeof(PARSER_Data_p->ParserGlobalVariable.ListD));

	/* Sort ListO by ascending FieldOrderCnt */
	qsort((void *)PARSER_Data_p->ParserGlobalVariable.ListD, PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements, sizeof(PARSER_Data_p->ParserGlobalVariable.ListD[0]), h264par_FieldOrderCntListOCmpFunc);
}
#endif /* STVID_PARSER_CHECK */

#ifdef STVID_PARSER_CHECK
/******************************************************************************/
/*! \brief Check that 2 PicOrderCnt do not differ by more than 2^15
 *
 * See standard 8.2.1
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
BOOL h264par_DiffPicOrderCntOutOfRange(U32 FirstElementLoop, U32 SecondElementLoop, const PARSER_Handle_t ParserHandle)
{
	S32 DiffPicOrderCnt;
	BOOL OutOfRange;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	OutOfRange = FALSE;
	DiffPicOrderCnt = PARSER_Data_p->ParserGlobalVariable.ListO[SecondElementLoop].PicOrderCnt -
					  PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].PicOrderCnt;

	if ((DiffPicOrderCnt > 32767) || (DiffPicOrderCnt < -32768)) /* range: -2^15 to 2^15 -1 */
	{
		OutOfRange = TRUE;
	}

	return (OutOfRange);
}
#endif /* STVID_PARSER_CHECK */

#ifdef STVID_PARSER_CHECK
/******************************************************************************/
/*! \brief Perform checks on ListO (see standard: 8.2.1)
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_CheckListO(const PARSER_Handle_t ParserHandle)
{
	U32 FirstElementLoop;
	U32 SecondElementLoop;
	U32 FrameIDUnderTest;
	U32 FirstElementPosition;
	U32 SecondElementPosition;
	BOOL SecondElementFound;
	S32 FieldOrderCntUnderTest;
	BOOL ParityUnderTest;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* listO shall not contain any of the following: */

    /* -	a pair of TopFieldOrderCnt and BottomFieldOrderCnt for a frame or complementary field pair that are not at
	 *      consecutive positions in listO.
	 */
	FirstElementLoop = 0;
	while (FirstElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements)
	{
		/* For all elements of ListO, look for the element having the same DecodingOrderFrameID and check whether they
		 * are consecutive
		 */
		SecondElementFound = FALSE;
		FrameIDUnderTest = PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].DecodingOrderFrameID;
		SecondElementLoop = FirstElementLoop + 1;
		while ((SecondElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements) && (!SecondElementFound))
		{
			if (PARSER_Data_p->ParserGlobalVariable.ListO[SecondElementLoop].DecodingOrderFrameID == FrameIDUnderTest)
			{
				FirstElementPosition = FirstElementLoop;
				SecondElementPosition = SecondElementLoop;
				SecondElementFound = TRUE;
			}

			SecondElementLoop ++;
		}
		if (SecondElementFound)
		{
			/* Are FirstElementPosition and SecondElementPosition consecutive ? */
			if ((FirstElementPosition != SecondElementPosition -1) &&
				(FirstElementPosition != SecondElementPosition +1)
			   )
			{
				PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
				DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.FieldOrderCntNotConsecutiveInListOError,
						"parse_slice.c: (CheckListO) Top and Bottom Field Order Cnt for Frame/CFP not consecutive in List0\n");
			}
		}
		FirstElementLoop ++;
	}

    /* -	a TopFieldOrderCnt that has a value equal to another TopFieldOrderCnt. */
	FirstElementLoop = 0;
	while (FirstElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements)
	{
		if (PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].IsTopField == TRUE)
		{
			/* For all Top Fields elements of ListO, look for the element having the same TopFieldOrderCnt */
			FieldOrderCntUnderTest = PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].FieldOrderCnt;

			SecondElementLoop = FirstElementLoop + 1;
			SecondElementFound = FALSE;
			while ((SecondElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements) && (!SecondElementFound))
			{
				if ((PARSER_Data_p->ParserGlobalVariable.ListO[SecondElementLoop].IsTopField == TRUE) &&
					(PARSER_Data_p->ParserGlobalVariable.ListO[SecondElementLoop].FieldOrderCnt == FieldOrderCntUnderTest)
				   )
				{
					SecondElementFound = TRUE;
				}

			    SecondElementLoop ++;
			}
			if (SecondElementFound)
			{
				PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
				DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.DuplicateTopFieldOrderCntInListOError,
						"parse_slice.c: (CheckListO) Duplicate entries with same TopFieldOrderCnt found in List0\n");
			}
		}
		FirstElementLoop ++;
	}
    /* -	a BottomFieldOrderCnt that has a value equal to another BottomFieldOrderCnt. */
	FirstElementLoop = 0;
	while (FirstElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements)
	{
		if (PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].IsTopField == FALSE)
		{
			/* For all Bottom Fields elements of ListO, look for the element having the same BottomFieldOrderCnt */
			FieldOrderCntUnderTest = PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].FieldOrderCnt;

			SecondElementLoop = FirstElementLoop + 1;
			SecondElementFound = FALSE;
			while ((SecondElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements) && (!SecondElementFound))
			{
				if ((PARSER_Data_p->ParserGlobalVariable.ListO[SecondElementLoop].IsTopField == FALSE) &&
					(PARSER_Data_p->ParserGlobalVariable.ListO[SecondElementLoop].FieldOrderCnt == FieldOrderCntUnderTest)
				   )
				{
					SecondElementFound = TRUE;
				}

			    SecondElementLoop ++;
			}
			if (SecondElementFound)
			{
				PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
				DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.DuplicateBottomFieldOrderCntInListOError,
						"parse_slice.c: (CheckListO) Duplicate entries with same BotFieldOrderCnt found in List0\n");
			}
		}
		FirstElementLoop ++;
	}

    /* -	a BottomFieldOrderCnt that has a value equal to a TopFieldOrderCnt unless the BottomFieldOrderCnt and
	 *      TopFieldOrderCnt belong to the same coded frame or complementary field pair.
	 */
	FirstElementLoop = 0;
	while (FirstElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements)
	{
		if (PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].IsTopField == FALSE)
		{
			/* For all field elements of ListO, look for the alternate parity field element having the same FieldOrderCnt */
			FieldOrderCntUnderTest = PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].FieldOrderCnt;

	        ParityUnderTest = PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].IsTopField;
			SecondElementLoop = FirstElementLoop + 1;
			SecondElementFound = FALSE;
			while ((SecondElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements) && (!SecondElementFound))
			{
				if (PARSER_Data_p->ParserGlobalVariable.ListO[SecondElementLoop].IsTopField != ParityUnderTest)
				{
					if ((PARSER_Data_p->ParserGlobalVariable.ListO[FirstElementLoop].DecodingOrderFrameID !=
							PARSER_Data_p->ParserGlobalVariable.ListO[SecondElementLoop].DecodingOrderFrameID) &&
						(PARSER_Data_p->ParserGlobalVariable.ListO[SecondElementLoop].FieldOrderCnt == FieldOrderCntUnderTest)
					   )
					{
						SecondElementFound = TRUE;
					}
				}
			    SecondElementLoop ++;
			}
			if (SecondElementFound)
			{
				PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
				DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.NonPairedFieldsShareSameFieldOrderCntInListOError,
						"parse_slice.c: (CheckListO) error: Non-paired Bottom and Top Field share the same FieldOrder in List0\n");
			}
		}
		FirstElementLoop ++;
	}

	/* DiffPicOrderCnt( picA, picB ) = PicOrderCnt( picA ) - PicOrderCnt( picB )	(8-2) */
    /* The bitstream shall contain data that results in values of DiffPicOrderCnt( picA, picB ) used in the decoding
	 * process that are in the range of -2^15 to 2^15 - 1, inclusive
	 */
	FirstElementLoop = 0;
	while (FirstElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements)
	{
		SecondElementLoop = FirstElementLoop + 1;
		SecondElementFound = FALSE;
		while ((SecondElementLoop < PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements) && (!SecondElementFound))
		{
			if (h264par_DiffPicOrderCntOutOfRange(FirstElementLoop, SecondElementLoop, ParserHandle))
			{
				SecondElementFound = TRUE;
			}
			SecondElementLoop ++;
		}
		if (SecondElementFound)
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.DifferenceBetweenPicOrderCntExceedsAllowedRangeError,
						"parse_slice.c: (CheckListO) error: Difference between PicOrderCnt exceeds range in List0\n");
		}
		FirstElementLoop ++;
	}


}
#endif /* STVID_PARSER_CHECK */

#ifdef STVID_PARSER_CHECK
/******************************************************************************/
/*! \brief Performs checks on TopFieldOrderCnt and BottomFieldOrderCnt
 *
 * This check is described in 8.2.1 and involves the building of
 * 2 lists listD and List0
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_CheckListD(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
	/* When the current picture is not an IDR picture, the following applies.
     * -	Consider the list variable listD containing as elements the TopFieldOrderCnt and BottomFieldOrderCnt values
	 *      associated with the list of pictures including all of the following
     *      -	the first picture in the list is the previous picture of any of the following types
     *          -	an IDR picture
                -	a picture containing a memory_management_control_operation equal to 5
                -	all other pictures that follow in decoding order after the first picture in the list and precede
	 *              through the current picture which is also included in listD prior to the invoking of the decoded reference
	 *              picture marking process.
	 */

    if (PARSER_Data_p->PictureSpecificData.IsIDR == FALSE)
	{
		h264par_InsertCurrentPictureInListD(ParserHandle);

		/* Build List0 from ListD */
		h264par_BuildListO(ParserHandle);

		/* Perform the checks on List0 */
		h264par_CheckListO(ParserHandle);

		/* Reset the ListD if current picture has MMCO=5 */
		if (PARSER_Data_p->PictureLocalData.HasMMCO5 == TRUE)
		{
			PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements = 0; /* erase ListD content */
			/* Insert current picture again */
			h264par_InsertCurrentPictureInListD(ParserHandle);
		}
	}
	else
	{
		/* No check to perform. Simply flush the ListD */
		PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements = 0; /* erase ListD content */
		/* Insert current picture */
		h264par_InsertCurrentPictureInListD(ParserHandle);
	}
}
#endif /* STVID_PARSER_CHECK */

/******************************************************************************/
/*! \brief Move back by n byte in the stream
 *
 * \param Address position from where to rewind
 * \param RewindInByte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void * BackAddress
 */
/******************************************************************************/
U8 * h264par_MoveBackInStream(U8 * Address, U8 RewindInByte, const PARSER_Handle_t  ParserHandle)
{
    U8 * BackAddress;
    /* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    BackAddress = Address - RewindInByte;

    /* Adjust with bit buffer boundaries */
    if (BackAddress < PARSER_Data_p->BitBuffer.ES_Start_p)
    {
        /* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
        BackAddress = PARSER_Data_p->BitBuffer.ES_Stop_p
                    - PARSER_Data_p->BitBuffer.ES_Start_p
                    + BackAddress
                    + 1;
    }
    return (BackAddress);
}

/******************************************************************************/
/*! \brief Initializes the current picture structure
 *
 * This is to fill the current picture data structure with default parameters
 *
 * \param NalRefIdc the nal_ref_idc for the NAL containing this slice
 * \param NalUnitType the nal_unit_type for the NAL containing this slice
 * \param SeqParameterSetId the seq_parameter_set_id for this slice
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_InitializeCurrentPicture(U8 NalRefIdc, U8 NalUnitType, U8 SeqParameterSetId, U8 PicParameterSetId, const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
    NALByteStream_t CurrentPositionInNAL;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Once the Active SPS and PPS have been found for this picture, copy the information to the
	 * ActiveSPSLocalData, ActiveGlobalDecodingContextGenericData, ActiveGlobalDecodingContextSpecificData
	 * ActivePPSLocalData and ActiveGlobalPictureDecodingContextSpecificData
	 * Reason for this: the Active"SPS/PPS" information shall not be modified by SPS and/or PPS
	 * being inserted within the same access unit. So that, when issuing the event "picture parsed", the active
	 * "SPS/PPS" are valid.
	 */
     PARSER_Data_p->ActiveSPSLocalData_p                       = &PARSER_Data_p->SPSLocalData[SeqParameterSetId];
     PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p   = &PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId];
     PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p  = &PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId];
     PARSER_Data_p->ActivePPSLocalData_p                       = &PARSER_Data_p->PPSLocalData[PicParameterSetId];
     PARSER_Data_p->ActivePictureDecodingContextSpecificData_p = &PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId];

	/* First, fills in the parameters for the previous picture in decoding order with the ones used for current picture */
	if (PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable)
	{
        h264par_UpdatePreviousPictureInfo(ParserHandle);
    } else
	{
		/* First time the procedure is called from setup */
		/* Do not update parameters from the current picture! */
	}

    /* If Previous picture PanAndScanRepetitionPeriod was 0, then reset the pan ans scan settings */
    if (PARSER_Data_p->PictureGenericData.PanScanRectRepetitionPeriod == 0)
    {
        h264par_ResetPicturePanAndScanIn16thPixel(ParserHandle);
        PARSER_Data_p->PictureGenericData.PanScanRectRepetitionPeriod = 1;
    }
    /* This is the start code of the 1st picture of a sequence */
    /* Reset pan and scan */
    if (PARSER_Data_p->PictureLocalData.IsRandomAccessPoint)
    {
       h264par_ResetPicturePanAndScanIn16thPixel(ParserHandle);
    }

    PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
	PARSER_Data_p->PictureGenericData.IsDecodeStartTimeValid = FALSE;
	PARSER_Data_p->PictureGenericData.IsPresentationStartTimeValid = FALSE;


	/* Update reference parameters according to the NAL unit */

	if (NalRefIdc != 0)
	{
		/* this slice is a slice of a reference picture */
		PARSER_Data_p->PictureGenericData.IsReference = TRUE;
	}
	else
	{
		PARSER_Data_p->PictureGenericData.IsReference = FALSE;
	}

    if (NalUnitType == NAL_UNIT_TYPE_IDR)
    {
        if (NalRefIdc == 0)
		{
			/* nal_ref_idc shall not be zero for a IDR slice */
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NalRefIdcNullForIDRError,
			"test_main.c: (ProcessSCEntry) error: nal_ref_idc = 0 for IDR picture\n");
		}
		PARSER_Data_p->PictureSpecificData.IsIDR = TRUE;
    }
    else
    {
        PARSER_Data_p->PictureSpecificData.IsIDR = FALSE;
    }

	/* When field_pic_flag is not present it shall be inferred to be equal to 0 */
	/* When bottom_field_flag is not present for the current slice,
	 *       it shall be inferred to be equal to 0.
	 *
	 * Therefore, the default for the picture is FRAME
	 */
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;

	/* When delta_pic_order_cnt_bottom is not present in the bitstream for the current slice,
	 * it shall be inferred to be equal to 0
	 */
	PARSER_Data_p->PictureLocalData.delta_pic_order_cnt_bottom = 0;

	/* When delta_pic_order_cnt[0] is not present in the bitstream for the current slice,
	 * it shall be inferred to be equal to 0
	 */
	PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[0] = 0;

	/* When delta_pic_order_cnt[1] is not present in the bitstream for the current slice,
	 * it shall be inferred to be equal to 0
	 */
	PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[1] = 0;

	/* no_output_of_prior_pics_flag set to FALSE by default as it is an asynchronous command
	 * that may be used by the controller even if the picture is not an IDR
	 */
	PARSER_Data_p->PictureGenericData.AsynchronousCommands.FlushPresentationQueue = FALSE;

    /* Full freeze frame set to FALSE by default
     TODO : extract them from the stream
	 */
    PARSER_Data_p->PictureGenericData.AsynchronousCommands.FreezePresentationOnThisFrame = FALSE;
    PARSER_Data_p->PictureGenericData.AsynchronousCommands.ResumePresentationOnThisFrame = FALSE;

    /* Local parameters handled by the parser*/
    /* This switch is set to TRUE only if the picture has MMCO=5 operation
     */
    PARSER_Data_p->PictureLocalData.HasMMCO5 = FALSE;

    /* ExtendedPresentationOrderPictureID is growing only on IDR and recovery points */
    PARSER_Data_p->PictureLocalData.IncrementEPOID = FALSE;

    /* Random access point pictures are IDR, recovery points and I (all slices) pictures */
	PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = FALSE;

	/* Initialize Error variables that are modified by the slice parsing process to false */
	PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_NONE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NalRefIdcNullForIDRError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NotTheFirstSliceError  = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NoPPSAvailableError  = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NoSPSAvailableError  = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.UnintentionalLossOfPictureError  = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.DeltaPicOrderCntBottomRangeError  = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.PicOrderCntType2SemanticsError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.LongTermReferenceFlagNotNullError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.MaxLongTermFrameIdxPlus1RangeError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.DuplicateMMCO4Error = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.MMCO1OrMMCO3WithMMCO5Error = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.DuplicateMMCO5Error = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.IdrPicIdViolationError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NumRefIdxActiveMinus1RangeError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NumRefIdxActiveOverrrideFlagNullError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.SameParityOnComplementaryFieldsError = FALSE;

	PARSER_Data_p->PictureSpecificData.PictureError.POCError.IDRWithPOCNotNullError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.POCError.PicOrderCntTypeRangeError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.POCError.ListDOverFlowError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.POCError.FieldOrderCntNotConsecutiveInListOError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.POCError.DuplicateTopFieldOrderCntInListOError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.POCError.DuplicateBottomFieldOrderCntInListOError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.POCError.NonPairedFieldsShareSameFieldOrderCntInListOError = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.POCError.DifferenceBetweenPicOrderCntExceedsAllowedRangeError = FALSE;

    PARSER_Data_p->PictureSpecificData.PictureError.SEIError.ReservedPictStructSEIMessageError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.SEIError.TruncatedNAL = FALSE;

    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.MarkInUsedFrameBufferError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.DPBRefFullError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortTermToSlideError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortTermToMarkAsUnusedError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoLongTermToMarkAsUnusedError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.MarkNonExistingAsLongError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortPictureToMarkAsLongError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.Marking2ndFieldWithDifferentLongTermFrameIdxError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoLongTermAssignmentAllowedError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.LongTermFrameIdxOutOfRangeError = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.TruncatedNAL = FALSE;

	/* And sets various parameters for the current picture */
    PARSER_Data_p->PictureLocalData.Width = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Width;
    PARSER_Data_p->PictureLocalData.Height = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Height;
    PARSER_Data_p->PictureLocalData.MaxDecFrameBuffering = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->MaxDecFrameBuffering;

	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorType = H264_DEFAULT_COLOR_TYPE;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BitmapType = H264_DEFAULT_BITMAP_TYPE;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.PreMultipliedColor = H264_DEFAULT_PREMULTIPLIED_COLOR;

    /* Since YUV scaling is not used in H264, just set the scaling factor to No Rescale (JSG) */
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorY =	YUV_NO_RESCALE;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorUV = YUV_NO_RESCALE;

    switch (PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->MatrixCoefficients)
    {
        case MATRIX_COEFFICIENTS_FCC : /* Very close to ITU_R_BT601 matrixes */
			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
            break;

        case MATRIX_COEFFICIENTS_ITU_R_BT_470_2_BG : /* Same matrixes as ITU_R_BT601 */
            /* Not worth it, should take 601 ! */
            PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT470_2_BG;
            break;

        case MATRIX_COEFFICIENTS_SMPTE_170M : /* Same matrixes as ITU_R_BT601 */
            /* Not worth it, should take 601 ! */
            PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_170M;
            break;

        case MATRIX_COEFFICIENTS_SMPTE_240M : /* Slightly differ from ITU_R_BT709 */
            PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_240M;
            break;

        case MATRIX_COEFFICIENTS_UNSPECIFIED :
            /* TODO: implement ColorSpaceConversionMode choice according to DirecTV & DVB recommendations for H264 broadcasting */
            /* For now set BT709 by default for HD and BT601 for SD*/
            if(PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width > 720)
            {
                PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT709;
            }
            else
            {
                PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
            }
            break;

        case MATRIX_COEFFICIENTS_ITU_R_BT_709 :
        default : /* 709 seems to be the most suitable for the default case */
            PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT709;
            break;
    }

    switch (PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Aspect)
    {
        case STVID_DISPLAY_ASPECT_RATIO_16TO9 :
            PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
            break;

        case STVID_DISPLAY_ASPECT_RATIO_SQUARE :
            PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
            break;

        case STVID_DISPLAY_ASPECT_RATIO_221TO1 :
            PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_221TO1;
            break;

/*        case STVID_DISPLAY_ASPECT_RATIO_4TO3 :*/
        default :
            PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
            break;
    }

    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Width;
    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Height;

    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Pitch = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Width; /* TODO: check right value */
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Offset = H264_DEFAULT_BITMAPPARAMS_OFFSET;

	/* TODO? Data1_p, Size1, Data2_p, Size2 seem unknown at parser level. Is that true ??? */

	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.SubByteFormat = H264_DEFAULT_SUBBYTEFORMAT;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BigNotLittle = H264_DEFAULT_BIGNOTLITTLE;

    if (PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.FrameRate == H264_UNSPECIFIED_FRAME_RATE)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = H264_DEFAULT_FRAME_RATE;
    }
    else
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.FrameRate;
    }

#ifdef ST_XVP_ENABLE_FGT
    if (PARSER_Data_p->FilmGrainSpecificData.film_grain_characteristics_repetition_period == 0)
    h264par_ResetFgtParams(ParserHandle);
#endif /* ST_XVP_ENABLE_FGT */

    /* moved the PTS association before parsing the SEI, needed for Closed Caption retrieval */
    h264par_SearchForAssociatedPTS(ParserHandle);
	if (PARSER_Data_p->PTSAvailableForNextPicture)
	{
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS = PARSER_Data_p->PTSForNextPicture;
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = TRUE;
		PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
	}
    else
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE; /* Set to TRUE as soon as a PTS is detected */
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS = 0;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33 = FALSE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated = TRUE; /* default zero PTS value must be considered as interpolated for the producer */
    }

	/* The following setup is performed only when no SEI message is delivered alongwith the picture */
	/* h264par_SetDefaultSEI was called later in h264par_ParseSEI, it should be called before any parsing,
	     whether there is or not SEI messages available */
    h264par_SetDefaultSEI(ParserHandle);
	if (PARSER_Data_p->PictureLocalData.HasSEIMessage == TRUE)
	{
        /* parse the SEI related to the current picture */
        h264par_SavePositionInNAL(ParserHandle, &CurrentPositionInNAL);
        /* run through the saved SEI position list */
        while ((h264par_GetSEIListItem(ParserHandle)) != NULL)
        {
            h264par_ParseSEI(ParserHandle);
        }
        /* now we got all the SEI messages, we can reset HasSEIMessage flag */
        PARSER_Data_p->PictureLocalData.HasSEIMessage = FALSE;
        h264par_RestorePositionInNAL(ParserHandle, CurrentPositionInNAL);
    }

	/* TODO: what to do with CompressionLevel ?? */

	PARSER_Data_p->PictureGenericData.IsDisplayBoundPictureIDValid = FALSE; /* TODO: management of DisplayBoundPicture */

}

/******************************************************************************/
/*! \brief Parses slice header reference picture reordering section.
 *
 * The full section is discarded to reach the Picture Marking section
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param SliceType the slice_type stream element
 * \return void
 */
/******************************************************************************/
void h264par_ParseReferencePictureListReodering(U8 SliceType, const PARSER_Handle_t  ParserHandle)
{
	/* All of this parsing is thrown away: we just need to parse it
     * to reach the Ref Pic Marking section! Pity... */

	U8 RefPicListReorderingFlagL; /* to store both ..L0 and ..L1 */
	U8 ReorderingOfPicNumsIdc;
	U16 AbsDiffPicNumMinus1;
	U32 LongTermPicNum;

	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if ((SliceType != SLICE_TYPE_I) && (SliceType != ALL_SLICE_TYPE_I)) /* no SI slice */
    {
        RefPicListReorderingFlagL = h264par_GetUnsigned(1, ParserHandle);
        if (RefPicListReorderingFlagL == 1)
        {
            do
			{
				ReorderingOfPicNumsIdc = h264par_GetUnsignedExpGolomb(ParserHandle);
                if (ReorderingOfPicNumsIdc > 3)
                {
                    /* error recovery */
                    ReorderingOfPicNumsIdc = 3;
                    PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
                    DumpError(PARSER_Data_p->PictureSpecificData.PictureError.TruncatedNAL,
                    "parse_slice.c: (ParseReferencePictureListReodering) error: Truncated NAL\n");
                }
				if ((ReorderingOfPicNumsIdc == 0) || (ReorderingOfPicNumsIdc == 1))
				{
					AbsDiffPicNumMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
				}
				else if (ReorderingOfPicNumsIdc == 2)
				{
					LongTermPicNum = h264par_GetUnsignedExpGolomb(ParserHandle);
				}
			} while (ReorderingOfPicNumsIdc != 3);
		}
	}

	if ((SliceType == SLICE_TYPE_B) || (SliceType == ALL_SLICE_TYPE_B))
	{
		RefPicListReorderingFlagL = h264par_GetUnsigned(1, ParserHandle);
		if (RefPicListReorderingFlagL == 1)
		{
			do
			{
				ReorderingOfPicNumsIdc = h264par_GetUnsignedExpGolomb(ParserHandle);
                if (ReorderingOfPicNumsIdc > 3)
                {
                    /* error recovery */
                    ReorderingOfPicNumsIdc = 3;
                    PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
                    DumpError(PARSER_Data_p->PictureSpecificData.PictureError.TruncatedNAL,
                    "parse_slice.c: (ParseReferencePictureListReodering) error: Truncated NAL\n");
                }
				if ((ReorderingOfPicNumsIdc == 0) || (ReorderingOfPicNumsIdc == 1))
				{
					AbsDiffPicNumMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
				}
				else if (ReorderingOfPicNumsIdc == 2)
				{
					LongTermPicNum = h264par_GetUnsignedExpGolomb(ParserHandle);
				}
			} while (ReorderingOfPicNumsIdc != 3);
		}
	}
}

/******************************************************************************/
/*! \brief Parses slice header prediction weight section.
 *
 * The full section is discarded to reach the Picture Marking section
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param SliceType the slice_type stream element
 * \param NumRefIdxActiveL0ActiveMinus1 the PPS or Slice header stream element
 * \param NumRefIdxActiveL1ActiveMinus1 the PPS or Slice header stream element
 * \return void
 */
/******************************************************************************/
void h264par_ParsePredictionWeightTable(U8 SliceType, U8 NumRefIdxActiveL0ActiveMinus1, U8 NumRefIdxActiveL1ActiveMinus1, const PARSER_Handle_t  ParserHandle)
{
	/* All of this parsing is thrown away: we just need to parse it
	 * to reach the Ref Pic Marking section! Pity...
	 */
	U8 LumaLog2WeightDenom;
	U8 ChromaLog2WeightDenom;
	U8 LumaWeightLFlag; /* for both ..L0.. and ..L1.. */
	S8 LumaWeightLi; /* for both ..L0.. and ..L1.. */
	S8 LumaOffsetLi; /* for both ..L0.. and ..L1.. */
	U8 ChromaWeightLFlag; /* for both ..L0.. and ..L1.. */
	S8 ChromaWeightLij; /* for both ..L0.. and ..L1.. */
	S8 ChromaOffsetLij; /* for both ..L0.. and ..L1.. */

	U8 i; /* loop index as in standard */
	U8 j; /* loop index as in standard */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	LumaLog2WeightDenom = h264par_GetUnsignedExpGolomb(ParserHandle);
  if (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->chroma_format_idc)
  {
	   ChromaLog2WeightDenom = h264par_GetUnsignedExpGolomb(ParserHandle);
	}
	for (i = 0; i <= NumRefIdxActiveL0ActiveMinus1; i++)
	{
		LumaWeightLFlag = h264par_GetUnsigned(1, ParserHandle);
		if (LumaWeightLFlag == 1)
		{
			LumaWeightLi = h264par_GetSignedExpGolomb(ParserHandle);
			LumaOffsetLi = h264par_GetSignedExpGolomb(ParserHandle);
		}
    if (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->chroma_format_idc)
    {
		  ChromaWeightLFlag = h264par_GetUnsigned(1, ParserHandle);
		  if (ChromaWeightLFlag == 1)
		  {
			  for (j = 0; j < 2 ; j++)
			  {
			  	ChromaWeightLij = h264par_GetSignedExpGolomb(ParserHandle);
			  	ChromaOffsetLij = h264par_GetSignedExpGolomb(ParserHandle);
		  	}
	  	}
	  }
	}

	if ((SliceType == SLICE_TYPE_B) || (SliceType == ALL_SLICE_TYPE_B))
	{
		for (i = 0; i <= NumRefIdxActiveL1ActiveMinus1; i++)
		{
			LumaWeightLFlag = h264par_GetUnsigned(1, ParserHandle);
			if (LumaWeightLFlag == 1)
			{
				LumaWeightLi = h264par_GetSignedExpGolomb(ParserHandle);
				LumaOffsetLi = h264par_GetSignedExpGolomb(ParserHandle);
			}
      if (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->chroma_format_idc)
      {
		  	ChromaWeightLFlag = h264par_GetUnsigned(1, ParserHandle);
			  if (ChromaWeightLFlag == 1)
			  {
			  	for (j = 0; j < 2 ; j++)
			  	{
			  		ChromaWeightLij = h264par_GetSignedExpGolomb(ParserHandle);
			  		ChromaOffsetLij = h264par_GetSignedExpGolomb(ParserHandle);
			  	}
			  }
			}
		}
	}
}

/******************************************************************************/
/*! \brief Detect wether the current field is the complementary field of the previous picture
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return DecodingOrderFrameID
 */
/******************************************************************************/
BOOL h264par_IsComplementaryFieldOfPreviousPicture(const PARSER_Handle_t  ParserHandle)
{
    BOOL IsComplementaryFieldOfPreviousPicture;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    IsComplementaryFieldOfPreviousPicture = FALSE;

  if (CurrentPictureIsFrame)
  {
	  PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields = FALSE;
  }
  else
  {
    if (PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE)
    {
        /* 1st test: is the current non reference field the 2nd field of a
         * complementary non-reference field pair */
        if ((PARSER_Data_p->PreviousPictureInDecodingOrder.IsReference == FALSE) &&
            (PARSER_Data_p->PictureGenericData.IsReference == FALSE)
           )
        {
            /* Condition for that field to be the 2nd field of a complementary
             * non-reference field pair is:
             * - current picture is of opposite parity versus the previous picture
             * - the previous picture is not already paired
             */
            if (((CurrentPictureIsBottomField) &&
                 (PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
                ) ||
                ((CurrentPictureIsTopField) &&
                 (PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
                )
               )
            {
                if (PARSER_Data_p->PreviousPictureInDecodingOrder.IsFirstOfTwoFields == TRUE)
                {
                    /* This field is the 2nd of a complementary non-reference field pair */
                    IsComplementaryFieldOfPreviousPicture = TRUE;
                }
            }
        }
        /* 2nd test: is the current reference field the 2nd field of a
         * complementary reference field pair */
        else if ((PARSER_Data_p->PreviousPictureInDecodingOrder.IsReference == TRUE) &&
                 (PARSER_Data_p->PictureGenericData.IsReference == TRUE)
                )
        {
            /* Condition for that field to be the 2nd field of a complementary
             * reference field pair is:
             * - current picture and previous picture are coded as fields
             * - current field and previous field share the same frame_num
             *   (implicit: they have opposite parity, see semantics of frame_num)
             * - current field is not an IDR and does not include a MMCO=5 operation
             */

            /* Implementation note: the very first condition (current picture is field) is already tested at that stage */
            if (PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
            {
                if (PARSER_Data_p->PictureLocalData.frame_num == PARSER_Data_p->PreviousPictureInDecodingOrder.frame_num)
                {
                    if ((PARSER_Data_p->PictureSpecificData.IsIDR == FALSE) &&
                        (PARSER_Data_p->PictureLocalData.HasMMCO5 == FALSE)
                    )
                    {
                        /* Check anyway if both fields have opposite parity. If not, return an error */
                        if (((CurrentPictureIsBottomField) &&
                            (PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
                            ) ||
                            ((CurrentPictureIsTopField) &&
                            (PARSER_Data_p->PreviousPictureInDecodingOrder.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
                            )
                        )
                        {
                            /* H264 norm says:
                             * - current field and previous field share the same frame_num (implicit: they have opposite parity, see semantics of frame_num)
                             * but if the application injects partial stream for trickmode purpose we may have several CFP with the same frame_num
                             * in order to handle such cases IsComplementaryFieldOfPreviousPicture must be returned false for non 2nd fields to make DecodingOrderID increase properly.
                             */
                            if (PARSER_Data_p->PreviousPictureInDecodingOrder.IsFirstOfTwoFields == TRUE)
                            {
                                /* This field is the 2nd of a complementary non-reference field pair */
                                IsComplementaryFieldOfPreviousPicture = TRUE;
                            }
                        }
                        else
                        {
                            /* both fields share the same frame_num are not IDR or MMCO=5 but have the same parity.
                            * The stream is not H264 compliant */
                            PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
                            DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.SameParityOnComplementaryFieldsError,
                            "parse_slice.c: (h264par_IsComplementaryFieldOfPreviousPicture) error:  consecutive paired fields share same parity\n");

                        }
                    }
                }
            }
        }
        /* else, if one only between the current and the previous picture is reference,
         * they cannot be part of a CFP */
    }

        /* Current picture is a field */
        if (IsComplementaryFieldOfPreviousPicture == TRUE)
        {
			PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields = FALSE;
        }
        else
        {
            /* this picture is a field that is not the complementary field of the
             * previous picture. Then, it is the first field of a CFP
             */
			PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields = TRUE;
        }
    }
    return (IsComplementaryFieldOfPreviousPicture);
}

/******************************************************************************/
/*! \brief Computes DecodingOrderFrameID.
 *
 * DecodingOrderFrameID is a unique identifier for frames
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return DecodingOrderFrameID
 */
/******************************************************************************/
U32 h264par_ComputeDecodingOrderFrameID(const PARSER_Handle_t  ParserHandle)
{
    BOOL IsComplementaryFieldOfPreviousPicture;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* First step: check if current picture is the 2nd field of a CFP.
     * (If yes, then it will share the DecodingOrderFrameID with the first field
     */

    IsComplementaryFieldOfPreviousPicture = h264par_IsComplementaryFieldOfPreviousPicture(ParserHandle);

    if (IsComplementaryFieldOfPreviousPicture == FALSE)
    {
        PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID ++;
    }
    else
    {
        /* Else, the current picture is the second field of a CFP, then: re-use the
         * very same DecodingOrderFrameID than for the first field
         */
    }

    return(PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID);
}

/******************************************************************************/
/*! \brief Parses slice header RefPicMarking section.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseDecodedReferencePictureMarking(const PARSER_Handle_t  ParserHandle)
{
	U8 NoOutputOfPriorPicsFlag;
	U8 LongTermReferenceFlag;
	U8 AdaptiveRefPicMarkingModeFlag;
	U8 MemoryManagementControlOperation;
    U32 DifferenceOfPicNumsMinus1 = 0;     /* unspecified range: put minimum, default value must be checked ... */
    U32 LongTermPicNum = 0;                /* unspecified range: put minimum, default value must be checked ... */
    U8 LongTermFrameIdx = 0;               /* default value must be checked ... */
    U8 MaxLongTermFrameIdxPlus1 = 0;       /* default value must be checked ... */
	S8 FrameIndex;
	BOOL HasMMCO6; /* set to true if current picture includes a MMCO=6 */
	/* The following variables are set for semantics checks */
	BOOL HasMMCO1; /* set to true if current picture includes a MMCO=1 */
	BOOL HasMMCO2; /* set to true if current picture includes a MMCO=2 */
	BOOL HasMMCO3; /* set to true if current picture includes a MMCO=3 */
	BOOL HasMMCO4; /* set to true if current picture includes a MMCO=4 */
	BOOL HasMMCO5; /* set to true if current picture includes a MMCO=5 */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	if (PARSER_Data_p->PictureSpecificData.IsIDR) /* nal_unit_type == 5 */
	{
		NoOutputOfPriorPicsFlag = h264par_GetUnsigned(1, ParserHandle);

		/* When the IDR picture is not the first IDR picture in the bitstream and the value of PicWidthInMbs,
		 * FrameHeightInMbs, or max_dec_frame_buffering derived from the active sequence parameter set is different
		 * from the value of PicWidthInMbs, FrameHeightInMbs, or max_dec_frame_buffering derived from the sequence
		 * parameter set active for the preceding sequence, no_output_of_prior_pics_flag equal to 1 may be inferred
		 * by the decoder, regardless of the actual value of no_output_of_prior_pics_flag
		 */
		if ((PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE) &&
		    ((PARSER_Data_p->PreviousPictureInDecodingOrder.Width != PARSER_Data_p->PictureLocalData.Width) ||
			 (PARSER_Data_p->PreviousPictureInDecodingOrder.Height != PARSER_Data_p->PictureLocalData.Height) ||
			 (PARSER_Data_p->PreviousPictureInDecodingOrder.MaxDecFrameBuffering != PARSER_Data_p->PictureLocalData.MaxDecFrameBuffering)
		    )
		   )
		{
			NoOutputOfPriorPicsFlag = 1;
		}
#ifdef STVID_VALID
        /* In validation, we force the FlushPresentationQueue because we want to check all pictures */
		PARSER_Data_p->PictureGenericData.AsynchronousCommands.FlushPresentationQueue = FALSE;
#else
        PARSER_Data_p->PictureGenericData.AsynchronousCommands.FlushPresentationQueue = ((NoOutputOfPriorPicsFlag == 1) ? TRUE : FALSE);
#endif
		LongTermReferenceFlag = h264par_GetUnsigned(1, ParserHandle);

		/* When num_ref_frames is equal to 0, long_term_reference_flag shall be equal to 0 */
        if ((PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->NumberOfReferenceFrames == 0) &&
			(LongTermReferenceFlag == 1)
		   )
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.LongTermReferenceFlagNotNullError,
			"parse_slice.c: (ParseDecodedReferencePictureMarking) error: long_term_reference_flag shall be 0 when num_ref_frames = 0\n");
		}

		/* First, mark the IDR as short term picture */
		/* All the frame buffers in DPBRef should be free at that stage */
		FrameIndex = h264par_FindFreeFrameIndex(ParserHandle);
		if (FrameIndex == -1)
		{
            PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_RESET;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.DPBRefFullError,
			"parse_slice.c: (ParseDecodedReferencePictureMarking) error no free buffer to store incoming IDR\n");
			/* Stops treatment here to avoid bad data handling */
			return;
		}
        else
        {
            PARSER_Data_p->PictureLocalData.IsNonExisting = FALSE;
            h264par_AssignShortTermInFrameBuffer(FrameIndex, ParserHandle);

            if (LongTermReferenceFlag == 0)
            {
                /* long_term_reference_flag equal to 0 specifies that the MaxLongTermFrameIdx
                * variable is set equal to "no long-term frame indices"
                */
                PARSER_Data_p->ParserGlobalVariable.MaxLongTermFrameIdx = NO_LONG_TERM_FRAME_INDICES;
            }
            else /* LongTermReferenceFlag = 1 */
            {
                /* long_term_reference_flag equal to 1 specifies that the MaxLongTermFrameIdx
                * variable is set equal to 0
                */
                PARSER_Data_p->ParserGlobalVariable.MaxLongTermFrameIdx = 0;

                /* Mark current picture as long term */
                /* IDR picture marked with LongTermFrameIdx = 0
                */
                h264par_MarkCurrentPictureAsLongTerm(FrameIndex, 0, ParserHandle);
            }
            /* Update Picture numbers after reference insertion in DPB */
            h264par_DecodingProcessForPictureNumbers(ParserHandle);
        }
	}
	else /* current picture is not an IDR picture */
	{
		AdaptiveRefPicMarkingModeFlag = h264par_GetUnsigned(1, ParserHandle);

		/* Semantics checks on AdaptiveRefPicMarkingModeFlag versus the number of frames in Virtual DPB
		 * is done in the sliding window process, after computing the number of long term frames in DPB
		 */

		if (AdaptiveRefPicMarkingModeFlag == 0)
		{
			/* Fill data structure to mark the current picture */
			PARSER_Data_p->PictureLocalData.IsNonExisting = FALSE;
		  h264par_PerformSlidingWindowOnDPBRef(ParserHandle);
  		/* Update Picture numbers after reference insertion in DPB */
      h264par_DecodingProcessForPictureNumbers(ParserHandle);
		}
		else /* AdaptiveRefPicMarkingModeFlag =1 */
		{
            /* if current picture is not marked with MMCO=6, it will be
             * marked as short term at the end of the process
             */
            HasMMCO6 = FALSE;

			/* Initialize variables for MMCO semantics checks */
			HasMMCO1 = FALSE;
			HasMMCO2 = FALSE;
			HasMMCO3 = FALSE;
			HasMMCO4 = FALSE;
			HasMMCO5 = FALSE;

			do
			{
				MemoryManagementControlOperation = h264par_GetUnsignedExpGolomb(ParserHandle);
                if (PARSER_Data_p->NALByteStream.EndOfStreamFlag)
                {
                    MemoryManagementControlOperation = 0;
                    PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
                    DumpError(PARSER_Data_p->PictureSpecificData.PictureError.TruncatedNAL,
                    "parse_slice.c: (ParseDecodedReferencePictureMarking) Truncated NAL \n");
                }

				if ((MemoryManagementControlOperation == 1) ||
					(MemoryManagementControlOperation == 3)
				   )
				{
					DifferenceOfPicNumsMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
				}
				if (MemoryManagementControlOperation == 2)
				{
					LongTermPicNum = h264par_GetUnsignedExpGolomb(ParserHandle);
				}
				if ((MemoryManagementControlOperation == 3) ||
					(MemoryManagementControlOperation == 6)
				   )
				{
					LongTermFrameIdx = h264par_GetUnsignedExpGolomb(ParserHandle);

					/* If the variable MaxLongTermFrameIdx is equal to
					 * "no long-term frame indices", long_term_frame_idx shall not be present
					 */
					if (PARSER_Data_p->ParserGlobalVariable.MaxLongTermFrameIdx == NO_LONG_TERM_FRAME_INDICES)
					{
						PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
						DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoLongTermAssignmentAllowedError,
						"parse_slice.c: (ParseDecodedReferencePictureMarking) long_term_frame_idx present while MaxLongTermFrameIdx = no_long_term_frame_indices\n");
					}
					else
					{
						/* Otherwise, the value of long_term_frame_idx shall be in the range
						 * of 0 to MaxLongTermFrameIdx, inclusive
						 */
						if (LongTermFrameIdx > PARSER_Data_p->ParserGlobalVariable.MaxLongTermFrameIdx)
						{
							PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
							DumpError(PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.LongTermFrameIdxOutOfRangeError,
							"parse_slice.c: (ParseDecodedReferencePictureMarking) long_term_frame_idx out of range\n");
							/* Clip it */
							LongTermFrameIdx = PARSER_Data_p->ParserGlobalVariable.MaxLongTermFrameIdx;
						}
					}
				}
				if (MemoryManagementControlOperation == 4)
				{
					MaxLongTermFrameIdxPlus1 = h264par_GetUnsignedExpGolomb(ParserHandle);

					/* max_long_term_frame_idx_plus1 shall be in the range of 0 to num_ref_frames, inclusive */
                    if (MaxLongTermFrameIdxPlus1 > PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->NumberOfReferenceFrames)
					{
						DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.MaxLongTermFrameIdxPlus1RangeError,
						"parse_slice.c: (ParseDecodedReferencePictureMarking) error: max_long_term_frame_idx_plus1 out of range\n");
						/* Clip it */
                        MaxLongTermFrameIdxPlus1 = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->NumberOfReferenceFrames;
					}
				}

				/* Semantics checks on MMCO */
				/* memory_management_control_operation shall not be equal to 1 in a slice header unless the specified
				 * short-term picture is currently marked as "used for reference" and has not been assigned to a long-term
				 * frame index and is not assigned to a long-term frame index in the same decoded reference picture marking
				 * syntax structure:
				 * -> this is done while processing the marking. Any of the above case ends up with an error while managing
				 *    the DPBRef
				 */

				/* memory_management_control_operation shall not be equal to 2 in a slice header unless the specified
				 * long-term picture number refers to a frame or field that is currently marked as "used for reference":
				 * -> this is done while processing the marking. The above case ends up with an error while managing
				 *    the DPBRef
				 */

				/* memory_management_control_operation shall not be equal to 3 in a slice header unless the specified
				 * short-term reference picture is currently marked as "used for reference" and has not previously been
				 * assigned a long-term frame index and is not assigned to any other long-term frame index within the
				 * same decoded reference picture marking syntax structure:
				 * -> this is done while processing the marking. The above case ends up with an error while managing
				 *    the DPBRef
				 */

				/* Actions to undertake on MMCO */
				switch (MemoryManagementControlOperation)
				{
					case 1:
						/* No more than one memory_management_control_operation shall be present in a slice header
						 * that specifies the same action to be taken
						 * -> if it happens, the parser will detect an error while managing the MMCO command
						 *    (no picture to unmark)
						 */

						h264par_DoMMCO1(DifferenceOfPicNumsMinus1, ParserHandle);
						HasMMCO1 = TRUE;
						break;
					case 2:
						/* No more than one memory_management_control_operation shall be present in a slice header
						 * that specifies the same action to be taken
						 * -> if it happens, the parser will detect an error while managing the MMCO command
						 *    (no picture to unmark)
						 */
						h264par_DoMMCO2(LongTermPicNum, ParserHandle);
						break;
					case 3:
						/* No more than one memory_management_control_operation shall be present in a slice header
						 * that specifies the same action to be taken
						 * -> if it happens, the parser will detect an error while managing the MMCO command
						 *    (no short term picture to mark)
						 */
						/* TODO: MMCO3: When decoding a field and a memory_management_control_operation command equal to 3
						 * assigns a long-term frame index to a field that is part of a short-term reference frame or a
						 * short-term complementary reference field pair, another memory_management_control_operation command
						 * to assign the same long-term frame index to the other field of the same frame or complementary
						 * reference field pair shall be present in the same decoded reference picture marking syntax
						 * structure
						 */
						h264par_DoMMCO3(DifferenceOfPicNumsMinus1, LongTermFrameIdx, ParserHandle);
						HasMMCO3 = TRUE;
						break;
					case 4:
						/* semantics checks: Not more than one memory_management_control_operation equal to 4 shall be
						 * present in a slice header
						 */
						if (HasMMCO4 == TRUE)
						{
							PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
							DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.DuplicateMMCO4Error,
							"parse_slice.c: (ParseDecodedReferencePictureMarking) error: more than one MMCO=4 operation found in the same slice header\n");
						}
						h264par_DoMMCO4(MaxLongTermFrameIdxPlus1, ParserHandle);
						HasMMCO4 = TRUE;
						break;
					case 5:
						if ((HasMMCO1 == TRUE) || (HasMMCO3 == TRUE))
						{
							/* memory_management_control_operation shall not be equal to 5 in a slice header unless no
							 * memory_management_control_operation in the range of 1 to 3 is present in the same decoded
							 * reference picture marking syntax structure.
							 * Note: if MMCO1 or MMCO3 comes after the MMCO5 operation, the parser will raise an error
							 *       (no picture to unmark) as the DPBRef will be emptied
							 */
							PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
							DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.MMCO1OrMMCO3WithMMCO5Error,
							"parse_slice.c: (ParseDecodedReferencePictureMarking) error: MMCO1 or MMCO3 found with MMCO5\n");
						}
						if (HasMMCO5 == TRUE)
						{
							/* No more than one memory_management_control_operation shall be present in a slice header
							 * that specifies the same action to be taken
							 */
							DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.DuplicateMMCO5Error,
							"parse_slice.c (ParseDecodedReferencePictureMarking) error: more than one MMCO5 command in same slice header\n");
						}
						h264par_DoMMCO5(ParserHandle);
						HasMMCO5 = TRUE;
						break;
					case 6:
  						/* TODO: MMCO6: No more than one memory_management_control_operation shall be present in a slice header
  						 * that specifies the same action to be taken
  						 */
						/* TODO: MMCO6: When the first field (in decoding order) of a complementary reference field pair
						 * includes a long_term_reference_flag equal to 1 or a memory_management_control_operation command
						 * equal to 6, the decoded reference picture marking syntax structure for the other field of the
						 * complementary reference field pair shall contain a memory_management_control_operation command
						 * equal to 6 that assigns the same long-term frame index to the other field
						 */
						h264par_DoMMCO6(LongTermFrameIdx, ParserHandle);
						HasMMCO6 = TRUE;
						break;
				}
    		    /* Update Picture numbers after reference insertion in DPB */
                h264par_DecodingProcessForPictureNumbers(ParserHandle);
			} while (MemoryManagementControlOperation != 0);

			if (HasMMCO6 == FALSE)
			{
		  		/* the current picture has not been marked by the sliding process (because AdaptiveRefPicMarkingModeFlag = 1)
				 * and it has not been marked as long term picture by any MMCO=6
				 * Thus, the current picture is marked as short term
				 */

                 h264par_MarkCurrentPictureAsShortTerm(ParserHandle);
                 /* Update Picture numbers after reference insertion in DPB */
                 h264par_DecodingProcessForPictureNumbers(ParserHandle);
			}
            h264par_ClipDPBAfterMMCO(ParserHandle);
		} /* AdaptiveRefPicMarkingModeFlag =1 */
	}

	/* After marking the current decoded reference picture, the total number of frames with at least one field
	 * marked as "used for reference", plus the number of complementary field pairs with at least one field marked
	 * as "used for reference", plus the number of non-paired fields marked as "used for reference" shall not be greater
	 * than Max(num_ref_frames, 1) (K012)
	 */
}
/******************************************************************************/
/*! \brief Get slice type.
 *
 *
 * \param NalRefIdc the nal_ref_idc for the NAL containing this slice
 * \param NalUnitType the nal_unit_type for the NAL containing this slice
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_GetSliceTypeFromSliceHeader (const PARSER_Handle_t  ParserHandle, U8 *SliceType)
{
	 /* dereference ParserHandle to a local variable to ease debug */
	 H264ParserPrivateData_t * PARSER_Data_p;
    U8 FirstMbInSlice;

	/* dereference ParserHandle to a local variable to ease debug */

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	FirstMbInSlice = h264par_GetUnsignedExpGolomb(ParserHandle);

	/* Check this slice header is the first of a picture */
	if (FirstMbInSlice != 0)
	{
		PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
		DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NotTheFirstSliceError,
		"parse_slice.c: (ParseSliceHeader) info: not parsing the first slice header of a picture\n");
	}
	else
	{
		/* A new picture is being parsed. */
	}
    *SliceType = h264par_GetUnsignedExpGolomb(ParserHandle);
}

/******************************************************************************/
/*! \brief Parses slice header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param NalRefIdc the nal_ref_idc for the NAL containing this slice
 * \param NalUnitType the nal_unit_type for the NAL containing this slice
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseSliceHeader(U8 NalRefIdc, U8 NalUnitType, StartCode_t * StartCode_p, const PARSER_Handle_t  ParserHandle, const U8 SliceType)
{
    U16 PicParameterSetId; /* the PPS pointed by the picture */
    U8 SeqParameterSetId; /* the SPS pointed by the PPS */
    U8 * NALStartAddress, * NALStopAddress;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;
    NALByteStream_t CurrentPositionInNAL;
    U32 PayLoadSize, PayLoadType;
    VIDCOM_ErrorLevel_t PreviousParsingErrorValue;


#if defined(DVD_SECURED_CHIP) && defined (ST_7109)
    /* Here we want to use real bitbuffer address */
    NALStartAddress = StartCode_p->BitBufferStartAddress_p;
    NALStopAddress  = StartCode_p->BitBufferStopAddress_p;
#else
    NALStartAddress = StartCode_p->StartAddress_p;
    NALStopAddress  = StartCode_p->StopAddress_p;
#endif /* DVD_SECURED_CHIP && 7109 */

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PicParameterSetId = h264par_GetUnsignedExpGolomb(ParserHandle);

	/* Default: this picture is not valid (it may miss SPS and/or PPS) */
	PARSER_Data_p->PictureLocalData.IsValidPicture = FALSE;

	/* Update position in bit buffer */
    /* The picture start address used through the CODEC API is the position of the beginning of the start code */
    PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress = (void *) h264par_MoveBackInStream(NALStartAddress, START_CODE_LENGTH, ParserHandle);
	/* The stop address is the byte preceeding the next start code */
    PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  = (void *) h264par_MoveBackInStream(NALStopAddress, START_CODE_LENGTH + 1, ParserHandle);

	/* Check if the active PPS is available */
    if ((PicParameterSetId > (MAX_PPS-1)) || (PARSER_Data_p->PPSLocalData[PicParameterSetId].IsAvailable == FALSE))
	{
       PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
       DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NoPPSAvailableError,
       "parse_slice.c: (ParseSliceHeader) warning: no PPS available for this picture\n");
       /* We are missing the PPS for this picture. Thus it cannot be a valid picture
        * No need to parse the slice header */
	}
	else
	{
        /* Set the active PPS for this picture */
        PARSER_Data_p->PictureLocalData.ActivePPS = PicParameterSetId;

		/* Retrieve the right SPS data structure from PicParameterSetId */
		SeqParameterSetId = PARSER_Data_p->PPSLocalData[PicParameterSetId].seq_parameter_set_id;

		/* Check if the active SPS is available */
        if ((SeqParameterSetId > (MAX_SPS-1)) ||(PARSER_Data_p->SPSLocalData[SeqParameterSetId].IsAvailable == FALSE))
		{
            PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
              DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NoSPSAvailableError,
              "parse_slice.c: (ParseSliceHeader) warning: no SPS available for this picture\n");
		      /* We are missing the SPS for this picture. Thus it cannot be a valid picture
    		   * No need to parse the slice header */
		}
		else
		{
            /* Set the active SPS for this picture */
            PARSER_Data_p->PictureLocalData.ActiveSPS = SeqParameterSetId;
            h264par_InitializeCurrentPicture(NalRefIdc, NalUnitType, SeqParameterSetId, PicParameterSetId, ParserHandle);
            h264par_ParseRestOfSliceHeader(SliceType, ParserHandle);

            PreviousParsingErrorValue = PARSER_Data_p->PictureGenericData.ParsingError;

            if (PARSER_Data_p->PictureLocalData.HasUserData == TRUE)
            {
                /* save position in NAL */
                h264par_SavePositionInNAL(ParserHandle, &CurrentPositionInNAL);
                /* for each UserDataPosition saved, parse the userdata */
                while ((h264par_GetUserDataItem(&PayLoadSize, &PayLoadType, ParserHandle)) != NULL)
                {
                    if (PayLoadType == SEI_USER_DATA_REGISTERED_ITU_T_T35)
                    {
                        h264par_ParseUserDataRegisteredITUTT35(PayLoadSize, ParserHandle);
                    }
                    else
                    {
                        h264par_ParseDataUnregistered(PayLoadSize, ParserHandle);
                    }
                }
                /* don't mark a picture as bad if there was a problem while reading the user data.
                   Just restore ParsingError to its previous value */
                PARSER_Data_p->PictureGenericData.ParsingError = PreviousParsingErrorValue;
                /* now we parsed, and eventually signaled all the UserData messages, we can reset HasUserData flag */
                PARSER_Data_p->PictureLocalData.HasUserData = FALSE;
                /* restore position in NAL */
                h264par_RestorePositionInNAL(ParserHandle, CurrentPositionInNAL);
            }



#ifdef ST_XVP_ENABLE_FGT
            if(PARSER_Data_p->FilmGrainSpecificData.IsFilmGrainEnabled == TRUE)
            {
                if(PARSER_Data_p->PictureSpecificData.IsIDR)
                {
                    PARSER_Data_p->FilmGrainSpecificData.picture_offset =
                    (PARSER_Data_p->PictureLocalData.IdrPicId << 5) + PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID;
                }
                else
                {
                    if(SliceType == ALL_SLICE_TYPE_I)
                    {
                        PARSER_Data_p->FilmGrainSpecificData.picture_offset = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID;
                    }
                    else
                    {
                        PARSER_Data_p->FilmGrainSpecificData.picture_offset =
                        (LastIDR_IdrPicId << 5) + PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID;
                    }
                }
            }
#endif /* ST_XVP_ENABLE_FGT */

        }
	}

	 /* make sure SEIList is empty */
    while ((h264par_GetSEIListItem(ParserHandle)) != NULL)
    {
        /* Intentionnally void statement */
    }
    /* Set no SEI as a default for next picture */
    PARSER_Data_p->PictureLocalData.HasSEIMessage = FALSE;

	/* No AUD for next picture */
    PARSER_Data_p->PictureLocalData.PrimaryPicType = PRIMARY_PIC_TYPE_UNKNOWN;
} /* End of h264par_ParseSliceHeader() */

/******************************************************************************/
/*! \brief Parses the second part of slice header and fills data structures.
 *
 * This part of slice header is parsed only when both SPS and PPS are available
 * for this picture.
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseRestOfSliceHeader(U8 SliceType, const PARSER_Handle_t  ParserHandle)
{
    U8 FieldPicFlag = 0; /* local variable to get the stream element field_pic_flag, default value must be checked ... */
	U8 BottomFieldFlag; /* local variable to get the stream element bottom_field_flag */
	U16 IdrPicId;
	U8 DirectSpatialMvPredFlag;
	U8 NumRefIdxActiveOverrrideFlag;
	U8 NumRefIdxActiveL0ActiveMinus1; /* used for parsing prediction weight table */
	U8 NumRefIdxActiveL1ActiveMinus1; /* used for parsing prediction weight table */
	U32 PrevRefFrameNum; /* variable from the standard */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
    U32 DeltaTPhiDivisor;
    U32 ClocksPerSecond;
    U8 SeqParameterSetId;
    U32 TimeFactor;
    STOS_Clock_t CpbRemovalDelay;
    U16 PictureDisplayHeightAfterCropping;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* This picture is valid (it has SPS and PPS available) */
	PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;

	/* frame_num is a parameter coded on log2_max_frame_num_minus4 + 4 bits */
    /* log2_max_frame_num_minus4 is U32 (for MME API needs). A cast is then necessary here */
    PARSER_Data_p->PictureLocalData.frame_num = h264par_GetUnsigned((U8)(PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->log2_max_frame_num_minus4 + 4), ParserHandle);

	if (PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointState == RECOVERY_PENDING)
	{
		/* RecoveryFrameNum is the frame_num of the recovery point */
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryFrameNum = PARSER_Data_p->PictureLocalData.frame_num + PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryFrameCnt;
        if (PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryFrameNum >= PARSER_Data_p->ActiveSPSLocalData_p->MaxFrameNum)
		{
            PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryFrameNum -= PARSER_Data_p->ActiveSPSLocalData_p->MaxFrameNum;
		}
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointState = RECOVERY_COMPUTED;
	}

	if (PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointState == RECOVERY_COMPUTED)
	{
		/* A recovery point is active: let's check whether the current picture is the expected recovery point */
		if ((PARSER_Data_p->PictureLocalData.frame_num == PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryFrameNum) &&
			PARSER_Data_p->PictureGenericData.IsReference)
		{
			/* This picture is a recovery point: starting from here, next pictures in presentation order are correct
			 * or approximately correct
			 */
			PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointState = RECOVERY_REACHED;

			/* Next reference pictures will no longer be tagged as Broken Link */
			PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.BrokenLinkFlag = FALSE;
		}
	}

	/* This picture is a random access point if either of the following is true:
	 * - the current picture is an IDR
	 * - the current picture is marked as Recovery Access Point by a SEI message
     * - the current picture contains only I slices
	 */
	if ((PARSER_Data_p->PictureSpecificData.IsIDR == TRUE) ||
        (PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointState == RECOVERY_REACHED))
	{
		PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = TRUE;
        PARSER_Data_p->PictureLocalData.IncrementEPOID = TRUE;
        if (!PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointHasBeenHit &&
			 PARSER_Data_p->PictureSpecificData.IsIDR == FALSE)
		{
			/* If a recovery point has already been hit, no need to perform the presentation order as the video is
			 * already synchronized
			 */
			/* Same happens if the recovery point is an IDR, no need to check for potentially corrupted pictures, as an IDR is
			 * a closed GOP */
			PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CheckPOC = TRUE;
		}
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointHasBeenHit = TRUE;
	}
    else
    if ((SliceType == ALL_SLICE_TYPE_I) || /* nonIDR picture with I only slices are random access points */
         ((PARSER_Data_p->PictureLocalData.PrimaryPicType == PRIMARY_PIC_TYPE_I) || (PARSER_Data_p->PictureLocalData.PrimaryPicType == PRIMARY_PIC_TYPE_ISI)))
	{
		PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = TRUE;
        if (!PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointHasBeenHit)
        {
			PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CheckPOC = TRUE;
            PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CurrentPicturePOIDToBeSaved = TRUE;
        }
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointHasBeenHit = TRUE;
#ifndef ST_OSWINCE  /* Optimization problem. */
        if (PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "++++++++++++++ RAP requested and found on non IDR or non Recovery point ++++++++++++++"));
        }
#endif
	}

	if (PARSER_Data_p->PictureLocalData.IsRandomAccessPoint)
	{
	 	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
	}
	else if (PARSER_Data_p->PictureGenericData.IsReference)
	{
	 	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;
	}
	else
	{
	 	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
	}

      /* This is the start code of the 1st picture of a sequence */
      /* If the cpbRemovalDelay is available, compute the Decode Start Time */
    if (PARSER_Data_p->PictureLocalData.IsRandomAccessPoint)
    {
      SeqParameterSetId = PARSER_Data_p->PictureLocalData.ActiveSPS;
      if (PARSER_Data_p->SPSLocalData[SeqParameterSetId].IsInitialCpbRemovalDelayValid == TRUE)
      {
        ClocksPerSecond = ST_GetClocksPerSecond();
        if (ClocksPerSecond != 0)
        {
            /* Find the appropriate way to compute CpbRemovalDelay with the best possible accuracy */
            /* theoric equation is CpbRemovalDelay (in ticks) = InitialCpbRemovalDelay (in 90 KHz clock units) * TicksBySeconds / 90000 (for 90 Khz clock to seconds conversion)*/
            /* Split computation to avoid a too big accuraccy loss on CpbRemovalDelay result as we compute with integer division */
            /* in Wince, STOS_Clock_t is a U64, could TimeFactor be higher than 0xffffffff? */
            /* ==> Remove the test on TimeFactor>1000 and force WinCE to the first option (as it was implicitely done with this test) */
            TimeFactor = ClocksPerSecond / 90;
#ifdef ST_OSWINCE
            CpbRemovalDelay = (STOS_Clock_t)((PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelay / 1000) * TimeFactor);
#else
            if (PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelay >= ((U32)0xffffffff / TimeFactor))
            {
                CpbRemovalDelay = (STOS_Clock_t)((PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelay / 1000) * TimeFactor);
            }
            else
            {
                CpbRemovalDelay = (STOS_Clock_t)((PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelay * TimeFactor) / 1000);
            }
#endif  /* #ifdef ST_OSWINCE */
            PARSER_Data_p->PictureGenericData.DecodeStartTime = time_plus(time_now(), CpbRemovalDelay);
            PARSER_Data_p->PictureGenericData.IsDecodeStartTimeValid = TRUE;
        }
      }
    }


	/* PrevRefFrameNum computation from frame_num */
    if (PARSER_Data_p->PictureSpecificData.IsIDR)
	{
		PrevRefFrameNum = 0;
	}
	else
	{
		PrevRefFrameNum = PARSER_Data_p->PreviousReferencePictureInDecodingOrder.frame_num;
	}

	/* Store for future use (in pre marking process) */
	PARSER_Data_p->PictureLocalData.PrevRefFrameNum = PrevRefFrameNum;

    FieldPicFlag = 0;

    if (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->frame_mbs_only_flag == 0)
	{
		FieldPicFlag = h264par_GetUnsigned(1, ParserHandle);

		if (FieldPicFlag == 1)
		{
			BottomFieldFlag = h264par_GetUnsigned(1, ParserHandle);
			if (BottomFieldFlag == 1)
			{
				PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_BOTTOM_FIELD;
			}
			else
			{
				PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_TOP_FIELD;
			}
		}
	}

    /* TABLE E-6 DeltaTPhiDivisor*/
    /* Divide SPS frame rate by 2 if picture is frame */
    DeltaTPhiDivisor = 1;

    if (PARSER_Data_p->ActiveSPSLocalData_p->pic_struct_present_flag == 0)
    {
        if (FieldPicFlag == 1)
        {
            DeltaTPhiDivisor = 1;
        }
        else
        {
            DeltaTPhiDivisor = 2;
        }
    }
    else
    {
        if ((PARSER_Data_p->PictureLocalData.PicStruct == PICSTRUCT_TOP_FIELD) || (PARSER_Data_p->PictureLocalData.PicStruct == PICSTRUCT_BOTTOM_FIELD) ||  (FieldPicFlag == 1))
        {
            DeltaTPhiDivisor = 1;
        }
        else
        {
            DeltaTPhiDivisor = 2;
        }
    }

    if (PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.FrameRate == H264_UNSPECIFIED_FRAME_RATE)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = H264_DEFAULT_FRAME_RATE;
    }
    else
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.FrameRate / DeltaTPhiDivisor;
    }

#ifdef PARSER_DEBUG_PATCH_ALLOWED
    if (PatchFrameRateEnabled)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = PatchFrameRateValue;
    }
#endif /* PARSER_DEBUG_PATCH_ALLOWED */
#ifdef ST_crc
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ForceFieldCRC =
     PARSER_Data_p->GlobalDecodingContextSpecificData[PARSER_Data_p->PictureLocalData.ActiveSPS].mb_adaptive_frame_field_flag;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.IsMonochrome =
     (PARSER_Data_p->GlobalDecodingContextSpecificData[PARSER_Data_p->PictureLocalData.ActiveSPS].chroma_format_idc == 0);
#endif /* ST_crc */
    /* Pre Marking stage (if any) */
	if (PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable)
	{
		if ((PARSER_Data_p->PictureLocalData.frame_num != PrevRefFrameNum) &&
            (PARSER_Data_p->PictureLocalData.frame_num != ((PrevRefFrameNum + 1) % PARSER_Data_p->ActiveSPSLocalData_p->MaxFrameNum))
	   	)
		{
	   		h264par_PerformPreMarking(ParserHandle); /* decoding process for gaps in frame_num applies */
		}
	}
	/* Updates FrameNumWrap, PicNum and LongTermPicNum for all the pictures in the DPBRef */
    /* This process is to be done AFTER the pre-marking process, if any
     * Rationale: the pre marking process modifies the picture numbers by adding
     * reference frames in the DPB
     */
    h264par_DecodingProcessForPictureNumbers(ParserHandle);

    if (PARSER_Data_p->PictureSpecificData.IsIDR) /* nal_unit_type == 5 */
	{
		/* IdrPicId is read but not used in this release of the software */
		IdrPicId = h264par_GetUnsignedExpGolomb(ParserHandle);
		PARSER_Data_p->PictureLocalData.IdrPicId = IdrPicId;

#ifdef ST_XVP_ENABLE_FGT
		LastIDR_IdrPicId = IdrPicId;
#endif /* ST_XVP_ENABLE_FGT */

		if ((PARSER_Data_p->PictureSpecificData.IsIDR == TRUE) &&
			(PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE) &&
			(PARSER_Data_p->PreviousPictureInDecodingOrder.IsIDR == TRUE) &&
			(PARSER_Data_p->PictureLocalData.IdrPicId == PARSER_Data_p->PreviousPictureInDecodingOrder.IdrPicId)
		   )
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
#ifndef STVID_VALID
            /* In validation, we do not want to get bothered by this error that may occurs when looping on the stream */
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.IdrPicIdViolationError,
			"parse_slice.c: (ParseRestOfSliceHeader) error: two consecutive IDR have same idr_pic_id\n");
#endif
		}
	}

    if (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->pic_order_cnt_type == 0)
	{
        /* log2_max_pic_order_cnt_lsb_minus4 is U32 (for MME API needs). A cast is then necessary here */
        PARSER_Data_p->PictureLocalData.pic_order_cnt_lsb = h264par_GetUnsigned((U8)(PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->log2_max_pic_order_cnt_lsb_minus4 + 4), ParserHandle);

        if ((PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->pic_order_present_flag == 1) &&
			(CurrentPictureIsFrame)
		   )
		{
			PARSER_Data_p->PictureLocalData.delta_pic_order_cnt_bottom = h264par_GetSignedExpGolomb(ParserHandle);
			/* Check on delta_pic_order_cnt_bottom can be done only after parsing the post marking */
		}
	}

    if ((PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->pic_order_cnt_type == 1) &&
        (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->delta_pic_order_always_zero_flag == 0)
	   )
	{
		PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[0] = h264par_GetSignedExpGolomb(ParserHandle);

        if ((PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->pic_order_present_flag == 1) &&
			(CurrentPictureIsFrame)
		   )
		{
			PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[1] = h264par_GetSignedExpGolomb(ParserHandle);
		}
	}

    /* At that stage, we've got all the parameters the PreDecodingPicOrderCntTop and PreDecodingPicOrderCntBot
     */
    h264par_ComputePicOrderCnt(ParserHandle);

    /* Initialize the reference picture lists */
	/* This process is done AFTER the PicOrderCnt computation.
	 * Rationale: the initialisation for B slices in frame is using the current picture POC */
	h264par_ReferencePictureListsInitialisation(ParserHandle);

	/* In main profile, redundant_pic_cnt_present_flag is always 0, so
	 * redundant_pic_cnt is never present
	 */

	/* The following blocks until the call to ParseDecodedReferencePictureMarking
	 * are just discarded. We need to parse them to reach the picture marking.
	 * The parsing cannot be accelerated due to the variable length structure of the stream
	 */

	if ((SliceType == SLICE_TYPE_B) || (SliceType == ALL_SLICE_TYPE_B))
	{
		/* direct_spatial_mv_pred_flag is discarded in this release */
		DirectSpatialMvPredFlag = h264par_GetUnsigned(1, ParserHandle);
	}

	/* Load default values from PPS for the following 2 parameters that may be overrriden
	 * here.
     * These parameters are used to parse the prediction weight table
	 */
    NumRefIdxActiveL0ActiveMinus1 = PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->num_ref_idx_l0_active_minus1;
    NumRefIdxActiveL1ActiveMinus1 = PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->num_ref_idx_l1_active_minus1;

	if ((SliceType == SLICE_TYPE_B) || (SliceType == ALL_SLICE_TYPE_B) ||
		(SliceType == SLICE_TYPE_P) || (SliceType == ALL_SLICE_TYPE_P)
	   )
	{
		NumRefIdxActiveOverrrideFlag = h264par_GetUnsigned(1, ParserHandle);

		if (NumRefIdxActiveOverrrideFlag == 1)
		{
			NumRefIdxActiveL0ActiveMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
			/* Semantics check */
			if (((FieldPicFlag == 0) && (NumRefIdxActiveL0ActiveMinus1 > 15)) ||
				((FieldPicFlag == 1) && (NumRefIdxActiveL0ActiveMinus1 > 31))
			   )
			{
				PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
				DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NumRefIdxActiveMinus1RangeError,
				"parse_slice.c: (ParseRestOfSliceHeader) error: num_ref_idx_active_l0_active_minus1 out of range\n");
                NumRefIdxActiveL0ActiveMinus1 = 1;
			}

			if ((SliceType == SLICE_TYPE_B) || (SliceType == ALL_SLICE_TYPE_B))
			{
				NumRefIdxActiveL1ActiveMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
				/* Semantics check */
				if (((FieldPicFlag == 0) && (NumRefIdxActiveL1ActiveMinus1 > 15)) ||
					((FieldPicFlag == 1) && (NumRefIdxActiveL1ActiveMinus1 > 31))
				   )
				{
					PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
					DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NumRefIdxActiveMinus1RangeError,
					"parse_slice.c: (ParseRestOfSliceHeader) error: num_ref_idx_active_l1_active_minus1 out of range\n");
                    NumRefIdxActiveL1ActiveMinus1 = 1;
				}
			}
		}
		else
		{
			/* semantics checks: can be performed on the first slice only.
			 * Thus, should be complemented by the slice engine (originally DeltaPhi)
			 */
			if (FieldPicFlag == 0)
			{
                if (PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->num_ref_idx_l0_active_minus1 > 15)
				{
					DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NumRefIdxActiveOverrrideFlagNullError,
					"parse_slice.c: (ParseRestOfSliceHeader) error: expected num_ref_idx_active_overrride_flag = 1, got 0\n");
				}
				if ((SliceType == SLICE_TYPE_B) || (SliceType == ALL_SLICE_TYPE_B))
				{
                    if (PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->num_ref_idx_l1_active_minus1 > 15)
					{
						DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NumRefIdxActiveOverrrideFlagNullError,
						"parse_slice.c: (ParseRestOfSliceHeader) error: expected num_ref_idx_active_overrride_flag = 1, got 0\n");
					}
				}
			}
		}
	}

	h264par_ParseReferencePictureListReodering(SliceType, ParserHandle);

    if (((PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->weighted_pred_flag == 1) &&
		 ((SliceType == SLICE_TYPE_P) || (SliceType == ALL_SLICE_TYPE_P))
		) ||
        ((PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->weighted_bipred_idc == 1) &&
		 ((SliceType == SLICE_TYPE_B) || (SliceType == ALL_SLICE_TYPE_B))
		)
	   )
	{
		h264par_ParsePredictionWeightTable(SliceType, NumRefIdxActiveL0ActiveMinus1, NumRefIdxActiveL1ActiveMinus1, ParserHandle);
	}

	if (PARSER_Data_p->PictureGenericData.IsReference)
	{
		h264par_ParseDecodedReferencePictureMarking(ParserHandle);
    }

    /* At that stage, DecodingOrderFrameID can be computed.
	 * Note: the parser needs to know whether the current picture has MMCO=5 to fill the
	 * DecodingOrderFrameID. This is why the computation is done AFTER picture marking.
     */
    PARSER_Data_p->PictureGenericData.DecodingOrderFrameID = h264par_ComputeDecodingOrderFrameID(ParserHandle);

	/* If the current picture is reference, update DPBRef for:
     * - DecodingOrderFrameID
     * - PicOrderCntTop and PicOrderCntBot
	 */
	if (PARSER_Data_p->PictureGenericData.IsReference)
	{
		/* It may erase a previously computed DecodingOrderFrameID, but as it is the same, we don't care */
		PARSER_Data_p->DPBFrameReference[PARSER_Data_p->ParserGlobalVariable.CurrentPictureFrameIndexInDPBRef].DecodingOrderFrameID = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
		PARSER_Data_p->DPBFrameReference[PARSER_Data_p->ParserGlobalVariable.CurrentPictureFrameIndexInDPBRef].TopFieldOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntTop;
		PARSER_Data_p->DPBFrameReference[PARSER_Data_p->ParserGlobalVariable.CurrentPictureFrameIndexInDPBRef].BottomFieldOrderCnt = PARSER_Data_p->PictureSpecificData.PicOrderCntBot;
	}
	/* When DecodingOrderFrameID is computed, some parameters telling whether this picture is the complementary
	 * field of the previous picture are set. Then the parser can perform some checks on pic_order_cnt_type
	 * (see semantics of this element)
	 */
    #ifdef STVID_PARSER_CHECK
	h264par_SemanticsChecksPicOrderCntType(ParserHandle);
    #endif

    /* A first picture has been parsed, thus the previous picture in decoding order will be set for next picture to parse */
	PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable = TRUE;

	/* Check POC from previous pictures up to last IDR or MMCO=5 (See 8.2.1) */
    #ifdef STVID_PARSER_CHECK
    h264par_CheckListD(ParserHandle);
    #endif

	/* Check wether end of NAL has been reached while parsing */
	if (PARSER_Data_p->NALByteStream.EndOfStreamFlag == TRUE)
	{
		PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
        PARSER_Data_p->PictureSpecificData.PictureError.TruncatedNAL = TRUE;
 	}

	/* ExtendedPresentationOrderPictureID computation */
	/* this number is set to 0 on the setup */
	/* and is incremented with each IDR or
     * on a MMCO=5 picture or
	 * on a recovery point and
     * is never reset */

    if (PARSER_Data_p->PictureLocalData.IncrementEPOID)
	{
        PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID ++;
	}

	/* Fill in generic data structure */
    if (CurrentPictureIsFrame)
    {
	   PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID =
        h264par_PicOrderCnt(PARSER_Data_p->PictureSpecificData.PicOrderCntTop,
                            PARSER_Data_p->PictureSpecificData.PicOrderCntBot);
    }
    else if (CurrentPictureIsTopField)
    {
	   PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID = PARSER_Data_p->PictureSpecificData.PicOrderCntTop;
    }
    else
    {
	   PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID = PARSER_Data_p->PictureSpecificData.PicOrderCntBot;
    }
	PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension = PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID;

    /* Now we hawe PicOrderCnt, compute the end of validity of the current Pan And Scan settings, if any */
    if (PARSER_Data_p->PictureGenericData.PanScanRectRepetitionPeriod > 1)
    {
        PARSER_Data_p->PictureGenericData.PanScanRectPersistence = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCnt  + PARSER_Data_p->PictureGenericData.PanScanRectRepetitionPeriod;
        PARSER_Data_p->PictureGenericData.PanScanRectRepetitionPeriod = 1;
    }
    /* Check current POC against the previously computed one */
    if (PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCnt > PARSER_Data_p->PictureGenericData.PanScanRectPersistence)
    {
        h264par_ResetPicturePanAndScanIn16thPixel(ParserHandle);
    }

    /* We started on non IDR and no recovery point, set RecoveryPointPresentationOrderPictureID to the current picture (new entry point) to check POC orders on next pictures */
    if (PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CurrentPicturePOIDToBeSaved)
    {
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointPresentationOrderPictureID.ID = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID;
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointPresentationOrderPictureID.IDExtension = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension;
        PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointDecodingOrderFrameID = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
        PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CurrentPicturePOIDToBeSaved = FALSE;
    }

	/* If the current picture is a recovery point, store its POC for further comparison with next pictures:
	 * When reaching a Recovery Point, pictures to be displayed before this recovery point shall be discarded */
	if (PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointState == RECOVERY_REACHED)
	{
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointPresentationOrderPictureID.ID = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID;
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointPresentationOrderPictureID.IDExtension = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension;
        PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointDecodingOrderFrameID = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
		/* Next picture is not assumed to be again a recovery point */
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointState = RECOVERY_NONE;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Recovery Point reached : entry point == %c(%d-%d/%d)",
                (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD?'T':
                ((PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD?'B':'F'))),
                PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointDecodingOrderFrameID,
                PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointPresentationOrderPictureID.IDExtension,
                PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointPresentationOrderPictureID.ID));
	}

    if ((PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CheckPOC) && (!PARSER_Data_p->PictureLocalData.IsRandomAccessPoint) &&
            (PARSER_Data_p->PictureGenericData.DecodingOrderFrameID != PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointDecodingOrderFrameID)) /* 2nd field of a recovery point must be considered as OK for display, else PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CheckPOC will be reset too early ! */
    {
        if (vidcom_ComparePictureID(&PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID, &PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointPresentationOrderPictureID) < 0)
        {
            /* Signal to the controller that this picture may have visual artifacts when displayed */
                PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ParseRestOfSliceHeader: picture has a POC prior to the entry point POC"));
        }
        else
        {
            PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CheckPOC = FALSE;
        }
    }

    /* Compute picture scan type */
    /*- DVB states that 1280x720 streams are progressive only */
    /*- When playing 1920x1080 or SD, DVB says pictures may be progressive or interlace. */
    /*  - A coded field is always displayed as interlace. */
    /*  - A coded frame is displayed according to the first condition encountered below */
    /*    - When SEI:pic_struct is present: */
    /*      - When SEI:ct_type is present: */
    /*        - 2 fields of a frame having SEI:ct_type = 0 or 2 are to displayed as progressive frame */
    /*        - 2 fields of a frame having SEI:ct_type = 1 are to be displayed as interlaced frame */
    /*      - When SEI:ct_type is not present: */
    /*        - SEI:pic_struct = 0,7,8 indicate progressive display */
    /*        - SEI:pic_struct = (other values) indicate interlace display */
    /*    - When SEI:pic_struct is not present: */
    /*      - When for a coded frame, TopFieldOrderCnt = BottomFieldOrderCnt, this frame is displayed as a progressive frame */
    /*      - When for a coded frame, TopFieldOrderCnt != BottomFieldOrderCnt, this frame is displayed as a interlaced frame */
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;

    PictureDisplayHeightAfterCropping = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Height;

    /* Compute cropped height of the picture */
    PictureDisplayHeightAfterCropping = PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Height
                                        - PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->FrameCropInPixel.TopOffset
                                        - PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->FrameCropInPixel.BottomOffset;

    if (PictureDisplayHeightAfterCropping == 720)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = STGXOBJ_PROGRESSIVE_SCAN;
    }
    else
    if (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
    {
        if (PARSER_Data_p->ActiveSPSLocalData_p->pic_struct_present_flag)
        {
            if (PARSER_Data_p->PictureLocalData.HasCtType)
            {
                /* 2 fields of a frame having SEI:ct_type = 0 or 2 are to displayed as progressive frame */
                switch (PARSER_Data_p->PictureLocalData.CtType)
                {
                    case 0 :
                    case 2 :
                        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = STGXOBJ_PROGRESSIVE_SCAN;
                        break;
                    default :
                        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;
                        break;
                }
            }
            else
            {
                /* SEI:pic_struct = 0,7,8 indicate progressive display */
                switch (PARSER_Data_p->PictureLocalData.PicStruct)
                {
                    case PICSTRUCT_FRAME :
                    case PICSTRUCT_FRAME_DOUBLING :
                    case PICSTRUCT_FRAME_TRIPLING :
                        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = STGXOBJ_PROGRESSIVE_SCAN;
                        break;
                    default :
                        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;
                        break;
                }
            }
        }
#ifdef MARK_PICTURE_AS_PROGRESSIVE_IF_BOTH_FIELDS_SHARE_SAME_INSTANT
        else
        {
            /* When for a coded frame, TopFieldOrderCnt = BottomFieldOrderCnt, this frame is displayed as a progressive frame */
            if (PARSER_Data_p->PictureSpecificData.PicOrderCntTop == PARSER_Data_p->PictureSpecificData.PicOrderCntBot)
            {
                PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = STGXOBJ_PROGRESSIVE_SCAN;
            }
        }
#endif /* MARK_PICTURE_AS_PROGRESSIVE_IF_BOTH_FIELDS_SHARE_SAME_INSTANT */
    }

#ifdef PARSER_DEBUG_PATCH_ALLOWED
    if (PatchScanTypeEnabled)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = PatchScanTypeValue;
    }
#endif /* PARSER_DEBUG_PATCH_ALLOWED */

    /* In some cases due to not fully compliant streams support (satrt on I picture, all I slice type picture)
     * second field can be tagged as a random access point.
     * If second field doesn't mark it as an entry point to avoid dangling field on start or re-start in the producer */
    if ((PARSER_Data_p->PictureLocalData.IsRandomAccessPoint) &&
        (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME) &&
        (!PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields))
    {
        PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = FALSE;
    }

    if (PARSER_Data_p->PictureLocalData.IsRandomAccessPoint)
    {
        PARSER_Data_p->DiscontinuityDetected = FALSE;
    }

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSlice(ParserHandle);
#endif

} /* end of h264par_ParseRestOfSliceHeader */

/*******************************************************************************
Name        : SearchForAssociatedPTS
Description : Searches for a PTS associated to the passed picture
Parameters  : Parser handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void h264par_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle)
{
    void* ES_Write_p;
	H264ParserPrivateData_t * PARSER_Data_p;
    U8 * PictureStartAddress_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    PictureStartAddress_p = PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress;

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
                    (((U32)PictureStartAddress_p) >= ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p + 1)) &&
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
        PARSER_Data_p->PTSForNextPicture.IsValid = TRUE;
        PARSER_Data_p->PTS_SCList.Read_p++;
        if (((U32)PARSER_Data_p->PTS_SCList.Read_p) >= ((U32)PARSER_Data_p->PTS_SCList.SCList + sizeof(PTS_t) * PTS_SCLIST_SIZE))
        {
            PARSER_Data_p->PTS_SCList.Read_p = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
        }
    }
} /* End of h264par_SearchForAssociatedPTS() function */

