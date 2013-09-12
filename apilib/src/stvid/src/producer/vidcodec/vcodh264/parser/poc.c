/*!
 *******************************************************************************
 * \file poc.c
 *
 * \brief H264 POC computation
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
#endif

/* H264 Parser specific */
#include "h264parser.h"
/* Functions ---------------------------------------------------------------- */

/******************************************************************************/
/*! \brief Computes TopFieldOrderCnt and BottomFieldOrderCnt when pic_order_cnt_type = 0
 *
 * This is the 1st stage computation for POC.
 * The whole process for this function is described in H264 Standard (I050) 8.2.1.1
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ComputeTopAndBottomFieldOrderCntForPOC0 (const PARSER_Handle_t  ParserHandle)
{
	S32 PrevPicOrderCntMsb; /* in standard: prevPicOrderCntMsb */
	U32 PrevPicOrderCntLsb; /* in standard: prevPicOrderCntLsb */

	S32 PicOrderCntMsb;
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	/* Computation of PrevPicOrderCntMsb and PrevPicOrderCntLsb */

	/* if current picture is an IDR, PrevPicOrderCntMsb and PrevPicOrderCntLsb are set to 0 */
	if (PARSER_Data_p->PictureSpecificData.IsIDR == TRUE)
    {
		PrevPicOrderCntMsb = 0;
		PrevPicOrderCntLsb = 0;
	}
	/* 	Otherwise, if the current picture is not an IDR picture
	 *  and the previous decoded picture in decoding order included a
	 *	memory_management_control_operation equal to 5
	 *	and the previous coded picture in decoding order is not a bottom field,
	 *	prevPicOrderCntMsb is set equal to 0
	 *	and prevPicOrderCntLsb is set equal to the value of TopFieldOrderCnt
	 *	for the previous picture.
     */
	/* About the test on the availability of the previous picture:
	 * - decoding always starts on a reference picture (IDR or RecoveryPoint)
	 * Thus, when a previous picture is available, a previous reference picture is also available
	 */
	else if ((PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE) &&
			 (PARSER_Data_p->PreviousReferencePictureInDecodingOrder.HasMMCO5 == TRUE) &&
			 (PARSER_Data_p->PreviousReferencePictureInDecodingOrder.PictureStructure != STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
			)
	{
		PrevPicOrderCntMsb = 0;
		/* Warning: standard specifies PrevPicOrderCntLsb as unsigned and TopFieldOrderCnt as signed. Thus a casting
		 * is necessary */
		PrevPicOrderCntLsb = (U32)PARSER_Data_p->PreviousReferencePictureInDecodingOrder.TopFieldOrderCnt;

    }
	/* Otherwise, if the current picture is not an IDR picture
	 * and the previous decoded picture in decoding order included a
	 * memory_management_control_operation equal to 5
	 * and the previous coded picture in decoding order is a bottom field,
	 * prevPicOrderCntMsb is set equal to 0 and prevPicOrderCntLsb is set equal to 0.
     */
	else if ((PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE) &&
			 (PARSER_Data_p->PreviousReferencePictureInDecodingOrder.HasMMCO5 == TRUE) &&
			 (PARSER_Data_p->PreviousReferencePictureInDecodingOrder.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
			)
	{
		PrevPicOrderCntMsb = 0;
		PrevPicOrderCntLsb = 0;

    }
    /* Otherwise, prevPicOrderCntMsb is set equal to PicOrderCntMsb of
	 * the previous reference picture in decoding order
	 * and prevPicOrderCntLsb is set equal to the value of pic_order_cnt_lsb
	 * of the previous reference picture in decoding order
     */
    else if (PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE)
	{
		PrevPicOrderCntMsb = PARSER_Data_p->PreviousReferencePictureInDecodingOrder.PicOrderCntMsb;
		PrevPicOrderCntLsb = PARSER_Data_p->PreviousReferencePictureInDecodingOrder.pic_order_cnt_lsb;
	}
	else
	{
		/* No previous picture available! But, we need to start decoding anyway!
		 * As this stands out of the specifications, the arbitrary choice made here is to start as if this
		 * point is an IDR
		 */
		PrevPicOrderCntMsb = 0;
		PrevPicOrderCntLsb = 0;
	}

	/* End of computation of PrevPicOrderCntMsb and PrevPicOrderCntLsb */

	/* Start computation of PicOrderCntMsb of the (current) parsed picture. Equation (8-3) */
	if ((PARSER_Data_p->PictureLocalData.pic_order_cnt_lsb < PrevPicOrderCntLsb) &&
        ((PrevPicOrderCntLsb - PARSER_Data_p->PictureLocalData.pic_order_cnt_lsb) >= ( PARSER_Data_p->ActiveSPSLocalData_p->MaxPicOrderCntLsb / 2))
	   )
	{
        PicOrderCntMsb = PrevPicOrderCntMsb + PARSER_Data_p->ActiveSPSLocalData_p->MaxPicOrderCntLsb;
	}
	else if ((PARSER_Data_p->PictureLocalData.pic_order_cnt_lsb > PrevPicOrderCntLsb) &&
             ((PARSER_Data_p->PictureLocalData.pic_order_cnt_lsb - PrevPicOrderCntLsb) > ( PARSER_Data_p->ActiveSPSLocalData_p->MaxPicOrderCntLsb / 2))
			)
	{
        PicOrderCntMsb = PrevPicOrderCntMsb - PARSER_Data_p->ActiveSPSLocalData_p->MaxPicOrderCntLsb;
	}
	else
	{
		PicOrderCntMsb = PrevPicOrderCntMsb;
	}
	/* End computation of  PicOrderCntMsb of the (current) parsed picture */

   /* Store PicOrderCntMsb for PreviousReferencePictureInDecodingOrder */
	PARSER_Data_p->PictureLocalData.PicOrderCntMsb = PicOrderCntMsb;

	/* Start computation of TopFieldOrderCnt and BottomFieldOrderCnt */

	/* Equation (8-4) */
	if ((CurrentPictureIsFrame) ||
		(CurrentPictureIsTopField)
	   )
	{
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop = PicOrderCntMsb + PARSER_Data_p->PictureLocalData.pic_order_cnt_lsb;
	}

	/* Equation (8-5) */
	if (CurrentPictureIsFrame)
	{
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop + PARSER_Data_p->PictureLocalData.delta_pic_order_cnt_bottom;
	}
	else if (CurrentPictureIsNotTopField)
	{
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot = PicOrderCntMsb + PARSER_Data_p->PictureLocalData.pic_order_cnt_lsb;
	}
	/* End computation of TopFieldOrderCnt and BottomFieldOrderCnt */
}

/******************************************************************************/
/*! \brief Computes TopFieldOrderCnt and BottomFieldOrderCnt when pic_order_cnt_type = 1
 *
 * This is the 1st stage computation for POC.
 * The whole process for this function is described in H264 Standard (I050) 8.2.1.2
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ComputeTopAndBottomFieldOrderCntForPOC1 (const PARSER_Handle_t  ParserHandle)
{
    U32 PrevFrameNum = 0; /* in standard: prevFrameNum, default value must be checked ... */
    U32 PrevFrameNumOffset = 0; /* in standard: prevFrameNumOffset, default value must be checked ... */
	U32 AbsFrameNum; /* in standard: absFrameNum */
    U32 PicOrderCntCycleCnt = 0; /* in standard: picOrderCntCycleCnt, default value must be checked ... */
    U32 FrameNumInPicOrderCntCycle = 0; /* in standard: frameNumInPicOrderCntCycle, default value must be checked ... */
	S32 ExpectedDeltaPerPicOrderCntCycle; /* in standard: expectedDeltaPerPicOrderCntCycle */
	U8 i; /* loop counter as mentionned in the standard for equation 8-9 and 8-10 */
	S32 ExpectedPicOrderCnt; /* in standard: expectedPicOrderCnt */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if (PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable)
	{
		PrevFrameNum = PARSER_Data_p->PreviousPictureInDecodingOrder.frame_num;

		/* When the current picture is not an IDR picture,
	 	* the variable prevFrameNumOffset is derived as follows
	 	*/
		if (PARSER_Data_p->PictureSpecificData.IsIDR == FALSE)
		{
			/* If the previous picture in decoding order included
			 * a memory_management_control_operation equal to 5,
			 * prevFrameNumOffset is set equal to 0
			 */
			if (PARSER_Data_p->PreviousPictureInDecodingOrder.HasMMCO5 == TRUE)
			{
		   		PrevFrameNumOffset = 0;
			}
			/* Otherwise prevFrameNumOffset is set equal to the value of FrameNumOffset
			 * of the previous picture
		 	*/
			else
			{
				PrevFrameNumOffset = PARSER_Data_p->PreviousPictureInDecodingOrder.FrameNumOffset;
			}
		}
	} /* if Previous Picture is available */

	/* equation (8-6) */
	/* Added to spec: if the previous picture is not available, it means the parser is dealing with a recovery point.
	 * The assumption is made that the behaviour is the same as for an IDR
	 */
	if ((PARSER_Data_p->PictureSpecificData.IsIDR == TRUE) /* nal_unit_type == 5 */ ||
		(PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == FALSE)
	   )
	{
		PARSER_Data_p->PictureLocalData.FrameNumOffset = 0;
	}
	else if (PrevFrameNum > PARSER_Data_p->PictureLocalData.frame_num)
	{
        PARSER_Data_p->PictureLocalData.FrameNumOffset = PrevFrameNumOffset + PARSER_Data_p->ActiveSPSLocalData_p->MaxFrameNum;
	}
	else
	{
		PARSER_Data_p->PictureLocalData.FrameNumOffset = PrevFrameNumOffset;
	}

	/* equation (8-7) */

    if (PARSER_Data_p->ActiveSPSLocalData_p->num_ref_frames_in_pic_cnt_cycle != 0)
	{
		AbsFrameNum = PARSER_Data_p->PictureLocalData.FrameNumOffset + PARSER_Data_p->PictureLocalData.frame_num;
	}
	else
	{
		AbsFrameNum = 0;
	}
	if (/* nal_ref_idc == 0 */(PARSER_Data_p->PictureGenericData.IsReference == FALSE) &&
		(AbsFrameNum > 0)
	   )
	{
		AbsFrameNum--;
	}

	/* equation (8-8) */

	if (AbsFrameNum > 0)
	{
        PicOrderCntCycleCnt = (AbsFrameNum - 1) / PARSER_Data_p->ActiveSPSLocalData_p->num_ref_frames_in_pic_cnt_cycle;
        FrameNumInPicOrderCntCycle = (AbsFrameNum - 1) % PARSER_Data_p->ActiveSPSLocalData_p->num_ref_frames_in_pic_cnt_cycle;
	}
	/* equation (8-9) */

	ExpectedDeltaPerPicOrderCntCycle = 0;
    for (i = 0; i < PARSER_Data_p->ActiveSPSLocalData_p->num_ref_frames_in_pic_cnt_cycle; i++)
	{
        ExpectedDeltaPerPicOrderCntCycle += PARSER_Data_p->ActiveSPSLocalData_p->offset_for_ref_frame[i];
	}

	/* equation (8-10) */

	if (AbsFrameNum > 0)
	{
		ExpectedPicOrderCnt = PicOrderCntCycleCnt * ExpectedDeltaPerPicOrderCntCycle;
		for (i = 0; i <= FrameNumInPicOrderCntCycle; i++)
		{
            ExpectedPicOrderCnt += PARSER_Data_p->ActiveSPSLocalData_p->offset_for_ref_frame[i];
		}
	}
	else
	{
		ExpectedPicOrderCnt = 0;
	}
	if (PARSER_Data_p->PictureGenericData.IsReference == FALSE) /* nal_ref_idc == 0 */
	{
        ExpectedPicOrderCnt += PARSER_Data_p->ActiveSPSLocalData_p->offset_for_non_ref_pic;
	}

	/* equation (8-11) */

	if (CurrentPictureIsFrame)
	{
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop = ExpectedPicOrderCnt + PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[0];
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop +
                                                              PARSER_Data_p->ActiveSPSLocalData_p->offset_for_top_to_bottom_field +
															  PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[1];
	}
	else if (CurrentPictureIsTopField)
	{
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop = ExpectedPicOrderCnt + PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[0];
	}
	else
	{
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot = ExpectedPicOrderCnt +
                                                              PARSER_Data_p->ActiveSPSLocalData_p->offset_for_top_to_bottom_field +
															  PARSER_Data_p->PictureLocalData.delta_pic_order_cnt[0];
	}
}

/******************************************************************************/
/*! \brief Computes TopFieldOrderCnt and BottomFieldOrderCnt when pic_order_cnt_type = 2
 *
 * This is the 1st stage computation for POC.
 * The whole process for this function is described in H264 Standard (I050) 8.2.1.3
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ComputeTopAndBottomFieldOrderCntForPOC2 (const PARSER_Handle_t  ParserHandle)
{
    U32 PrevFrameNum = 0; /* in standard: prevFrameNum, default value must be checked ... */
    U32 PrevFrameNumOffset = 0; /* in standard: prevFrameNumOffset, default value must be checked ... */
	U32 TempPicOrderCnt; /* in standard: tempPicOrderCnt */
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if (PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable)
	{
		PrevFrameNum = PARSER_Data_p->PreviousPictureInDecodingOrder.frame_num;

		/* When the current picture is not an IDR picture,
	 	* the variable prevFrameNumOffset is derived as follows
	 	*/
		if (PARSER_Data_p->PictureSpecificData.IsIDR == FALSE)
		{
			/* If the previous picture in decoding order included
		 	* a memory_management_control_operation equal to 5,
		 	* prevFrameNumOffset is set equal to 0
		 	*/
			if (PARSER_Data_p->PreviousPictureInDecodingOrder.HasMMCO5 == TRUE)
			{
				PrevFrameNumOffset = 0;
			}
			/* Otherwise prevFrameNumOffset is set equal to the value of FrameNumOffset
			 * of the previous picture
			 */
			else
			{
				PrevFrameNumOffset = PARSER_Data_p->PreviousPictureInDecodingOrder.FrameNumOffset;
			}
		}
	} /* if previous picture is available */

	/* equation (8-12) */

	/* Added to spec: if the previous picture is not available, it means the parser is dealing with a recovery point.
	 * The assumption is made that the behaviour is the same as for an IDR
	 */
	if ((PARSER_Data_p->PictureSpecificData.IsIDR == TRUE) /* nal_unit_type == 5 */ ||
		(PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == FALSE)
	   )
	{
		PARSER_Data_p->PictureLocalData.FrameNumOffset = 0;
	}
	else if (PrevFrameNum > PARSER_Data_p->PictureLocalData.frame_num)
	{
        PARSER_Data_p->PictureLocalData.FrameNumOffset = PrevFrameNumOffset + PARSER_Data_p->ActiveSPSLocalData_p->MaxFrameNum;
	}
	else
	{
		PARSER_Data_p->PictureLocalData.FrameNumOffset = PrevFrameNumOffset;
	}

	/* equation (8-13) */

	/* Added to spec: if the previous picture is not available, it means the parser is dealing with a recovery point.
	 * The assumption is made that the behaviour is the same as for an IDR
	 */
	if ((PARSER_Data_p->PictureSpecificData.IsIDR == TRUE) /* nal_unit_type == 5 */ ||
		(PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == FALSE)
	   )
	{
		TempPicOrderCnt = 0;
	}
	else if (PARSER_Data_p->PictureGenericData.IsReference == FALSE) /* nal_ref_idc == 0 */
	{
		TempPicOrderCnt = 2 * (PARSER_Data_p->PictureLocalData.FrameNumOffset + PARSER_Data_p->PictureLocalData.frame_num) - 1;
	}
	else
	{
		TempPicOrderCnt = 2 * (PARSER_Data_p->PictureLocalData.FrameNumOffset + PARSER_Data_p->PictureLocalData.frame_num);
	}

	/* equation (8-14) */

	if (CurrentPictureIsFrame)
	{
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop = TempPicOrderCnt;
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot = TempPicOrderCnt;
	}
	else if (CurrentPictureIsNotTopField)
	{
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot = TempPicOrderCnt;
	}
	else
	{
		PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop = TempPicOrderCnt;
	}

}

/******************************************************************************/
/*! \brief Checks the TopFieldOrderCnt and BottomFieldOrderCnt value versus IDR
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_CheckTopAndBottomFieldOrderCnt(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* The bitstream shall not contain data that results in
	 * Min( TopFieldOrderCnt, BottomFieldOrderCnt ) not equal to 0 for a coded IDR frame
	 */

	if ((PARSER_Data_p->PictureSpecificData.IsIDR == TRUE) &&
		(CurrentPictureIsFrame)
	   )
	{
		if ((PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop != 0) &&
			(PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot != 0)
		   )
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.IDRWithPOCNotNullError,
			"poc.c (CheckTopAndBottomFieldOrderCnt): IDR frame has min(TopFieldOrderCnt, BottomFieldOrderCnt) != 0");
		}
	}

	/* TopFieldOrderCnt not equal to 0 for a coded IDR top field */
	if ((PARSER_Data_p->PictureSpecificData.IsIDR == TRUE) &&
		(CurrentPictureIsNotFrame) &&
		(CurrentPictureIsTopField)
	   )
	{
		if (PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop != 0)
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.IDRWithPOCNotNullError,
			"poc.c (CheckTopAndBottomFieldOrderCnt): IDR top field has TopFieldOrderCnt != 0");
		}
	}

	/* BottomFieldOrderCnt not equal to 0 for a coded IDR bottom field */
	if ((PARSER_Data_p->PictureSpecificData.IsIDR == TRUE) &&
		(CurrentPictureIsNotFrame) &&
		(CurrentPictureIsNotTopField)
	   )
	{
		if (PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot != 0)
		{
			PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
			DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.IDRWithPOCNotNullError,
			"poc.c (CheckTopAndBottomFieldOrderCnt): IDR bottom field has BottomFieldOrderCnt != 0");
		}
	}

	/* The bitstream shall not contain data that results in values of TopFieldOrderCnt, BottomFieldOrderCnt,
	 * PicOrderCntMsb, or FrameNumOffset used in the decoding process as specified in subclauses 8.2.1.1 to 8.2.1.3
	 * that exceed the range of values from -231 to 231-1, inclusive.
	 * -> As PicOrderCntTop, PicOrderCntBot, PresentationOrderPictureID and FrameNumOffset all are declared as S32
	 *    they are by construction in the range specified above. No check is then performed
	 */
}

/******************************************************************************/
/*! \brief Computes PicOrderCnt, PicOrderCntTop, PicOrderCntBot
 *
 * This function computes the Picture Order Cnt
 * It computes standard specific values (for Top and Bottom field) linked to POC
 * The whole process for this function is described in H264 Standard (I050) 8.2.1
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ComputePicOrderCnt(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	/* TopFieldOrderCnt and BottomFieldOrderCnt are derived by invoking
	 * one of the decoding processes for picture order count type 0, 1, and 2
	 * in subclauses 8.2.1.1, 8.2.1.2, and 8.2.1.3, respectively
	 */

    switch (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->pic_order_cnt_type)
	{
		case 0: h264par_ComputeTopAndBottomFieldOrderCntForPOC0(ParserHandle);
				break;
		case 1: h264par_ComputeTopAndBottomFieldOrderCntForPOC1(ParserHandle);
				break;
		case 2: h264par_ComputeTopAndBottomFieldOrderCntForPOC2(ParserHandle);
				break;
		default:
				PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
				DumpError(PARSER_Data_p->PictureSpecificData.PictureError.POCError.PicOrderCntTypeRangeError,
				 "pic_cnt_order_type out of range in poc.c:ComputePictureOrderCnt\n");
	}

	/* Start of checks done on TopFieldOrderCont and BottomFieldOrderCnt */
	h264par_CheckTopAndBottomFieldOrderCnt(ParserHandle);

	/* Equation (8-1) */
	if (CurrentPictureIsFrame)
	{
        PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCnt = h264par_PicOrderCnt(PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop, PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot);
	}
	else if (CurrentPictureIsTopField)
	{
        PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCnt = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop;
	}
	else /* if bottom field */
	{
        PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCnt = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot;
	}

    /* Default values: overridden if MMCO=5 picture */
    PARSER_Data_p->PictureSpecificData.PicOrderCntTop = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntTop;
    PARSER_Data_p->PictureSpecificData.PicOrderCntBot = PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCntBot;
}
