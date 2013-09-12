/*!
 *******************************************************************************
 * \file parsebdu.c
 *
 * \brief vc1 bdu parsing and filling of data structures
 *
 * \author
 * Jeremy Gasper \n
 * CMG/STB \n
 * Copyright (C) 2005 STMicroelectronics
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined (ST_OSLINUX)
#include <stdio.h>
#include <stdlib.h>
#endif

/* vc1 Parser specific */
#include "vc1parser.h"

#define MAXIMUM_U8  0xFF
#define MAXIMUM_U32 0xFFFFFFFF
#define MAXIMUM_S32 0x7FFFFFFF

/* functions ----------------------------------------------------------------- */
static void vc1par_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle);
static void vc1par_ResetPicturePanAndScanIn16thPixel(const PARSER_Handle_t  ParserHandle);
void vc1par_ParseFrameLayerDataStruct(BOOL *SkipPicture, const PARSER_Handle_t ParserHandle);
void vc1par_ParseFrameLayerDataStruct_NotAligned(BOOL *SkipPicture, const PARSER_Handle_t ParserHandle);

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
static int vc1par_PESHeaderExist(U8* NextBDUStartCodeValueAddress_p, const PARSER_Handle_t  ParserHandle);
#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */
/******************************************************************************/
/*! \brief Parses a SH and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void vc1par_ParseSequenceHeader (const PARSER_Handle_t  ParserHandle)
{
	U32 Temp, i, FrameRateNR, FrameRateDR, FrameRate;

	U32 FrameRateNRArray[9] = {0,24*1000,25*1000,30*1000,50*1000,60*1000,48*1000,72*1000,0};

	U32 DispHorizSize;
	U32 DispVertSize;
	U32 SarWidth;
	U32 SarHeight;
	BOOL SampleAspectRatioFlag = FALSE;
	U32 DisplayAspectRatio;

	/* These arrays allow easy access to either the width or the
	   height of the sample aspect ratio with 4/3 being the default.*/
	U32 SarWidthArray[16]  = {4,1,12,10,16,40,24,20,32,80,18,15,64,160,4,4};
	U32 SarHeightArray[16] = {3,1,11,11,11,33,11,11,11,33,11,11,33,99,3,3};

	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* debugging only */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"vc1par_SeqHeader  "));
#endif

	if(PARSER_Data_p->ParserJobResults.ParsingJobStatus.HasErrors == TRUE)
	{
		vc1par_InitErrorFlags(ParserHandle);
	}

	/* assume that if we got this far, the profile is Advanced.  another function
	   is called for main and simple profiles.
	 */

	PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_ADVANCE;

	if((PARSER_Data_p->PrivateGlobalContextData.Level = vc1par_GetUnsigned(3, ParserHandle)) > 4)
	{
		PARSER_Data_p->PrivateGlobalContextData.Level = 4;
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.LevelOutOfRange = TRUE;
	}

	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication =
		  ((VC9_PROFILE_ADVANCE << 8) |                     /* bits 8 - 15 : Profile */
		    PARSER_Data_p->PrivateGlobalContextData.Level);  /* bits 0 - 7  : Level   */

	/* read in Color format, but don't use it.  This is specified in
	   STVID_ColorType_t, but not used.
     */
    Temp=vc1par_GetUnsigned(2, ParserHandle);

	if(Temp != 1)
	{
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ColorDiffFormatOutOfRange = TRUE;
	}

	PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedFrameRateForPostProcessing = vc1par_GetUnsigned(3, ParserHandle);

	PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing = vc1par_GetUnsigned(5, ParserHandle);

	PARSER_Data_p->GlobalDecodingContextSpecificData.PostProcessingFlag = vc1par_GetUnsigned(1, ParserHandle);

	Temp=vc1par_GetUnsigned(12, ParserHandle);

	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width = (Temp * 2)+2;
	PARSER_Data_p->StreamState.MaxWidth = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width;

	Temp=vc1par_GetUnsigned(12, ParserHandle);

	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height = (Temp * 2)+2;
	PARSER_Data_p->StreamState.MaxHeight = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height;

	PARSER_Data_p->GlobalDecodingContextSpecificData.PulldownFlag = vc1par_GetUnsigned(1, ParserHandle);

	PARSER_Data_p->GlobalDecodingContextSpecificData.InterlaceFlag = vc1par_GetUnsigned(1, ParserHandle);

	if(PARSER_Data_p->GlobalDecodingContextSpecificData.InterlaceFlag == 0)
	{
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType         = STGXOBJ_PROGRESSIVE_SCAN;
		PARSER_Data_p->PictureSpecificData.PictureSyntax = VC9_PICTURE_SYNTAX_PROGRESSIVE;
		PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax  = VC9_PICTURE_SYNTAX_PROGRESSIVE;
		PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax = VC9_PICTURE_SYNTAX_PROGRESSIVE;
	}

	PARSER_Data_p->GlobalDecodingContextSpecificData.TfcntrFlag = vc1par_GetUnsigned(1, ParserHandle);
	PARSER_Data_p->GlobalDecodingContextSpecificData.FinterpFlag = vc1par_GetUnsigned(1, ParserHandle);

	Temp=vc1par_GetUnsigned(1, ParserHandle);

	if(Temp != 1)
	{
		STTBX_Print(("Error Reserved bit in sequence header should be 1, but is set to zero.\n"));
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ReservedBitOutOfRange = TRUE;
	}

	PARSER_Data_p->GlobalDecodingContextSpecificData.ProgressiveSegmentedFrame = vc1par_GetUnsigned(1, ParserHandle);

	/* read in Display Extension flag. */
	Temp=vc1par_GetUnsigned(1, ParserHandle);

#ifdef VC1_DEBUG_PARSER
	VC1_PARSER_PRINT(("Level = %d\n",PARSER_Data_p->PrivateGlobalContextData.Level));
	if((PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing == 0) && (PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing == 31))
	{
		VC1_PARSER_PRINT(("Undefined Frame Rate PostProc Indicator\n"));
		VC1_PARSER_PRINT(("Undefined Bit   Rate PostProc Indicator\n"));
	}
	else if((PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing == 0) && (PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing == 30))
	{
		VC1_PARSER_PRINT(("QuantizedFrameRateForPostProcessing = Approx 2 frames per second\n"));
		VC1_PARSER_PRINT(("QuantizedBitRateForPostProcessing = Approx 1952 kbps or more\n"));
	}
	else if((PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing == 1) && (PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing == 31))
	{
		VC1_PARSER_PRINT(("QuantizedFrameRateForPostProcessing = Approx 6 frames per second\n"));
		VC1_PARSER_PRINT(("QuantizedBitRateForPostProcessing = Approx 2016 kbps or more\n"));
	}
	else
	{
		if(PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing == 7)
		{
			VC1_PARSER_PRINT(("QuantizedFrameRateForPostProcessing = Approx 30 frames per second or more\n"));
		}
		else
		{
			VC1_PARSER_PRINT(("QuantizedFrameRateForPostProcessing = Approx %d frames per second", (PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing*4)+2));
		}

		if(PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing == 31)
		{
			VC1_PARSER_PRINT(("QuantizedBitRateForPostProcessing = Approx 2016 kbps or more\n"));
		}
		else
		{
			VC1_PARSER_PRINT(("QuantizedBitRateForPostProcessing = Approx %d kbps", (PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing*64)+32));
		}

	}/* end else */
	VC1_PARSER_PRINT(("PostProcessingFlag = %u\n",PARSER_Data_p->GlobalDecodingContextSpecificData.PostProcessingFlag));
	VC1_PARSER_PRINT(("Max Width = %d\n",PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width));
	VC1_PARSER_PRINT(("Max Height = %u\n",PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height));
	VC1_PARSER_PRINT(("PulldownFlag = %d\n",PARSER_Data_p->GlobalDecodingContextSpecificData.PulldownFlag));
	VC1_PARSER_PRINT(("InterlaceFlag = %d\n",PARSER_Data_p->GlobalDecodingContextSpecificData.InterlaceFlag));
	VC1_PARSER_PRINT(("TfcntrFlag = %d\n",PARSER_Data_p->GlobalDecodingContextSpecificData.TfcntrFlag));
	VC1_PARSER_PRINT(("FinterpFlag = %d\n",PARSER_Data_p->GlobalDecodingContextSpecificData.FinterpFlag));
	VC1_PARSER_PRINT(("%s", (PARSER_Data_p->GlobalDecodingContextSpecificData.ProgressiveSegmentedFrame == 1) ?
					"Progressive Segmented Frame\n":"Not Progressive Segmented Frame\n"));
	VC1_PARSER_PRINT(("%s", (Temp == 1) ? "Display Extensions Present\n":"Display Extensions Not Present\n"));

#endif /* VC1_DEBUG_PARSER */


    /*      When the colour_primaries syntax element is not present,
     * the value of colour_primaries shall be inferred to be equal to 2
     * (the chromaticity is unspecified or is determined by the application).
     */
    PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries = UNSPECIFIED_COLOR_PRIMARIES;

    /* When the transfer_characteristics syntax element is not present,
     * the value of transfer_characteristics shall be inferred to be equal to 2
     * (the transfer characteristics are unspecified or are determined by the application)
     */
    PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics = UNSPECIFIED_TRANSFER_CHARACTERISTICS;

    /* When the matrix_coefficients syntax element is not present,
     * the value of matrix_coefficients shall be inferred to be equal to 2
     */
    PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients = MATRIX_COEFFICIENTS_UNSPECIFIED;

	/* default to no frame rate present */
	PARSER_Data_p->StreamState.FrameRatePresent = FALSE;

	if(Temp==1)
	{

		DispHorizSize = vc1par_GetUnsigned(14, ParserHandle) + 1;
		DispVertSize  = vc1par_GetUnsigned(14, ParserHandle) + 1;

		SampleAspectRatioFlag = vc1par_GetUnsigned(1, ParserHandle);

		if(SampleAspectRatioFlag == 1)
		{
			/* Sample Aspect ratio */
			Temp = vc1par_GetUnsigned(4, ParserHandle);

			if(Temp == 15)
			{
				SarWidth = vc1par_GetUnsigned(8, ParserHandle);
				SarHeight = vc1par_GetUnsigned(8, ParserHandle);
			}
			else
			{
				SarWidth  = SarWidthArray[Temp];
				SarHeight = SarHeightArray[Temp];

				if((Temp == 0) || (Temp == 14))
				{
					PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.SampleAspectRatioOutOfRange = TRUE;
				}
			}

			/* DisplayAspectRatio is set to 9 times the aspect ratio, so that a further integer comparison
			* gives the correspondance with STVID types
			*/
			DisplayAspectRatio = (9* SarWidth  * DispHorizSize) / (SarHeight * DispVertSize);

			/* 1:1 (square) is provided when DisplayAspectRatio is around 9 ([7;10]) */
			if ((DisplayAspectRatio >= 7) && (DisplayAspectRatio <= 10))
			{
				PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_SQUARE;
				PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
			}
			/* 4:3 is provided when DisplayAspectRatio is around 12 (]10;14[) */
			else if ((DisplayAspectRatio > 10) && (DisplayAspectRatio < 14))
			{
				PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
				PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
			}
			/* 16:9 is provided when DisplayAspectRatio is around 16 (+-20% = [14;18[) */
			else if ((DisplayAspectRatio >= 14) && (DisplayAspectRatio < 18))
			{
				PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
				PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
			}
			/* 2.21:1 is provided when DisplayAspectRatio is around 20 (+-20% = [18;22[) */
			else if ((DisplayAspectRatio >= 18) && (DisplayAspectRatio < 22))
			{
				PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_221TO1;
				PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_221TO1;
			}
			else
			{
                /* DisplayAspectRatio is not around (+-20%) of any known value.
			       An error is raised and the default aspect ratio is selected */
				PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
				PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.SampleAspectRatioOutOfRange = TRUE;
			}
		}/* end if(SampleAspectRatioFlag == 1) */

		/* FRAMERATEFLAG */
		Temp = vc1par_GetUnsigned(1, ParserHandle);

		if(Temp == 1)
		{
#ifdef VC1_DEBUG_PARSER
			VC1_PARSER_PRINT(("Frame Rate Info Present\n"));
#endif /* VC1_DEBUG_PARSER */

			PARSER_Data_p->StreamState.FrameRatePresent = TRUE;

			Temp=vc1par_GetUnsigned(1, ParserHandle);

			if(Temp == 0)
			{
				/*FRAMERATENR*/
				Temp = vc1par_GetUnsigned(8, ParserHandle);
				/* $frame_rate_nr = {"Forbidden","24*1000","25*1000","30*1000","50*1000","60*1000","48*1000","72*1000", "Reserved", ...); */

				if((Temp != 0) && (Temp <= 7))
				{
					FrameRateNR = FrameRateNRArray[Temp];
				}
				else
				{
					/* @@@ ILLEGAL Frame Rate Numerator */
					STTBX_Print(("ERROR: ILLEGAL Frame Rate Numerator %d\n", Temp));
					FrameRateNR = 30;

				}

				/*FRAMERATEDR*/
				Temp = vc1par_GetUnsigned(4, ParserHandle);

				/* frame rate denominator = {Forbidden,1000,1001, Reserved, ...}; */
				if(Temp == 1)
				{
					FrameRateDR = 1000;
				}
				else if(Temp == 2)
				{
					FrameRateDR = 1001;
				}
				else
				{
					/* @@@ ILLEGAL Frame Rate Denominator */
					STTBX_Print(("ERROR: ILLEGAL Frame Rate Denominator %d\n", Temp));
					FrameRateDR = 1;
				}

				if((FrameRateNR != 0) && (FrameRateDR != 1))
				{
					/* The frame rate is given as the number of frames per second multiplied by 1000 */
					FrameRate = (FrameRateNR * 1000)/FrameRateDR;
					PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = FrameRate;
					PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = FrameRate;
					PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRateExtensionN = FrameRateNR;
					PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRateExtensionD = FrameRateDR;
				}
			}
			else
			{
				/* FRAMERATEEXP */
				Temp = vc1par_GetUnsigned(16, ParserHandle);

				/* every incement of FRAMERATEEXP is an increment of .03125 frames per second.
				   Our Frame rate is specified in terms of 1/1000 frame per second.  So
				   convert from the given format to the required format.  @@@ Note that we lose
				   some precision here !!! */
				FrameRate = ((Temp + 1) * VC1_FRAMERATEEXP_STEP) / 100;
				PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = FrameRate;
				PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = FrameRate;
			}
		}
		else
		{
#ifdef VC1_DEBUG_PARSER
			VC1_PARSER_PRINT(("Frame Rate Info Not Present\n"));
#endif /* VC1_DEBUG_PARSER */
		}

		/* COLORFORMATFLAG */
		Temp = vc1par_GetUnsigned(1, ParserHandle);

		if(Temp == 1)
		{
#ifdef VC1_DEBUG_PARSER
			VC1_PARSER_PRINT(("Color Format present\n"));
#endif /* VC1_DEBUG_PARSER */

			/*$color_primaries = {"Forbidden","ITU-R BT.709-5","Unspecified color primaries",
			 *	"Reserved", "ITU-R BT.1700 Part B -625 PAL",
			 *	"SMPTE C Primaries frm ITU-R BT.1700 PartB-525 PAL", Reserved, ...);
             */
			Temp = vc1par_GetUnsigned(8, ParserHandle);

			if((Temp == 0) || (Temp == 3) || (Temp == 4) || (Temp > 6))
			{
				/* Leave Color primaries at the default value. */
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ColorPrimariesOutOfRange = TRUE;

			}
			else
			{
				PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries = Temp;
			}

			/*$transfer_characteristics = {"Forbidden","ITU-R BT.709-5","Unspecified Transfer Characteristics",
				"Reserved", "ITU-R BT.407-6, System M/NTSC",
				"ITU-R BT. 1700 Part B and Part C", "ITU-R BT. 1700 Part A", "Reserved",
				"ITU-R BT.1361(Conventional Color Space)", "Reserved",...);*/

			Temp = vc1par_GetUnsigned(8, ParserHandle);

			if((Temp == 0) || (Temp == 3) || (Temp == 7) || (Temp > 8))
			{
				/* Leave Transfer Characteristics at the default value. */
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.TransferCharOutOfRange = TRUE;

			}
			else
			{
				PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics = Temp;
			}

			/* $matrix_coefficients = {"Forbidden","ITU-R BT.790-2","Unspecified Matrix",
			 *	"Reserved","Reserved","Reserved", "ITU-R BT.601-5","Reserved",...);
             */
			Temp = vc1par_GetUnsigned(8, ParserHandle);

			if((Temp == 1) || (Temp == 2) || (Temp == 6))
			{
				PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients = Temp;
			}
			else
			{
				/* Leave Matrix Coefficients at the default value. */
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MatrixCoefOutOfRange = TRUE;
			}
		}/* end if(Temp == 1) */
		else
		{
#ifdef VC1_DEBUG_PARSER
			VC1_PARSER_PRINT(("Color Format not present\n"));
#endif /* VC1_DEBUG_PARSER */
		}
		/* Always set the Colorspace conversion even if color format not present */
    	switch (PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients)
    	{
    		case MATRIX_COEFFICIENTS_ITU_R_BT_1700 :
    			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_170M;
    			break;

    		case MATRIX_COEFFICIENTS_ITU_R_BT_709 :
    			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT709;
    		break;

    		case MATRIX_COEFFICIENTS_UNSPECIFIED :
    		default :
    			/* According to spec SMPTE 421M, default value for Matrix Coefficient is ITU-R BT. 601-5*/
    			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
    			break;
    	}/* end switch (PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients) */

	}/* Display extensions present */
	else
	{
	    /* If no Display horizontal and vertical sizes are given, use the
		   height and width to determine the aspect ratio (see Annex I.4.1)*/
		DispHorizSize = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width;
		DispVertSize  = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height;

	}/* end Display extensions present -- else*/

/* modif TD */
    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Pitch  = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width;
/* End modif TD : BUT PAN and Scan structure should be updated */

	/* Now that the display width and height are known, reset the
	   Pan and Scan to default values based on the display width and height */
	vc1par_ResetPicturePanAndScanIn16thPixel(ParserHandle);

    /* No Aspect Ratio given.  Use default.*/
	if(SampleAspectRatioFlag == FALSE)
	{
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
	}

	if((PARSER_Data_p->PrivateGlobalContextData.HRDParamFlag=vc1par_GetUnsigned(1, ParserHandle))==1)
	{

		PARSER_Data_p->GlobalDecodingContextSpecificData.NumLeakyBuckets = vc1par_GetUnsigned(5, ParserHandle);

		PARSER_Data_p->GlobalDecodingContextSpecificData.BitRateExponent = vc1par_GetUnsigned(4, ParserHandle);

		PARSER_Data_p->GlobalDecodingContextSpecificData.BufferSizeExponent = vc1par_GetUnsigned(4, ParserHandle);

#ifdef VC1_DEBUG_PARSER
		VC1_PARSER_PRINT(("HRD Parameters Present: HRD_NUM_LEAKY_BUCKETS = %u, BIT_RATE_EXPONENT = %u, BUFFER_SIZE_EXPONENT = %u\n",
					 PARSER_Data_p->GlobalDecodingContextSpecificData.NumLeakyBuckets,
					 PARSER_Data_p->GlobalDecodingContextSpecificData.BitRateExponent,
					 PARSER_Data_p->GlobalDecodingContextSpecificData.BufferSizeExponent));
#endif /*VC1_DEBUG_PARSER  */

		/* process each of the HRD Leaky bucket parameters */
		for(i=0; i < PARSER_Data_p->GlobalDecodingContextSpecificData.NumLeakyBuckets; i++)
		{
			PARSER_Data_p->GlobalDecodingContextSpecificData.HrdBufferParams[i].HrdRate = vc1par_GetUnsigned(16, ParserHandle);
			/* $HRD_arr[1] = ($HRD_arr[0] + 1) * (pow(2, ($sl_arr[BITRATEEXP]+6))); */

			PARSER_Data_p->GlobalDecodingContextSpecificData.HrdBufferParams[i].HrdBuffer = vc1par_GetUnsigned(16, ParserHandle);
			/* $HRD_arr[3] = ($HRD_arr[2] + 1) * (pow(2, ($sl_arr[BUFFRATEEXP]+4))); */

#ifdef VC1_DEBUG_PARSER
			VC1_PARSER_PRINT(("HRD BUCKET %d: HRD_RATE = %u, HRD_BUFFER = %u\n", i,
					 PARSER_Data_p->GlobalDecodingContextSpecificData.HrdBufferParams[i].HrdRate,
					 PARSER_Data_p->GlobalDecodingContextSpecificData.HrdBufferParams[i].HrdBuffer));
#endif /* VC1_DEBUG_PARSER */
		}

	}
	else
	{
#ifdef VC1_DEBUG_PARSER
		VC1_PARSER_PRINT(("No HRD Parameters Present\n"));
#endif /* VC1_DEBUG_PARSER */
		PARSER_Data_p->GlobalDecodingContextSpecificData.NumLeakyBuckets = 0;

	}

	PARSER_Data_p->PrivateGlobalContextData.GlobalContextValid = 1;

}/* end vc1par_ParseSequenceHeader () */


/******************************************************************************/
/*! \brief Parses a simple/main profile sequence header
*   inserted at the beginning of a PES packet and fills data structures.
*
* GlobalDecodingContext and PictureDecodingContext is updated with stream info
*
* \param ParserHandle Parser Handle (variables managed by the parser)
* \return void
*/
/******************************************************************************/
void vc1par_ParseSpMpPesSeqHdr(const PARSER_Handle_t ParserHandle)
{
	U32 Temp, Temp1;
	BOOL ProfileError = FALSE;
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"vc1par_ParseSpMpPesSeqHdr  "));
#endif

#ifndef STUB_INJECT
/*All Header is Big Endian */

		/* Read the Width */
		/* first 2 bit already read for profile */
		Temp1=vc1par_GetUnsigned(14, ParserHandle);

		if(Temp1 > VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT)
		{
			STTBX_Print(("ERROR: Width must be less than %d, Instead it is %u.  Setting to Max value\n",VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT, Temp1));
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedWidthOutOfRange = TRUE;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedWidthOutOfRange = TRUE;
			Temp1 = VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT;
		}
		/* Width must be an even value*/
		else if((Temp1 & 1) != 0)
		{
			STTBX_Print(("ERROR: Width must be even, Instead it is %u.  Rounding Value Down\n",Temp1));
			/* round value down to the nearest 2 */
			Temp1 = (Temp1/2) * 2;
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedWidthOutOfRange = TRUE;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedWidthOutOfRange = TRUE;
		}
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width = Temp1;
		PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth       = Temp1;
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width  = Temp1;
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Pitch  = Temp1;

        /* Read the Height */
		Temp=vc1par_GetUnsigned(16, ParserHandle);

		if(Temp > VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT)
		{
			STTBX_Print(("ERROR: Height must be less than or eqaul to %d, Instead it is %u. Setting to Max value\n",VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT, Temp));
			Temp = VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT;
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedHeightOutOfRange = TRUE;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedHeightOutOfRange = TRUE;
		}
		/* Height must be an even value*/
		else if((Temp & 1) != 0)
		{
			STTBX_Print(("ERROR: Height must be even, Instead it is %u.  Rounding Value Down\n",Temp));
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedHeightOutOfRange = TRUE;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedHeightOutOfRange = TRUE;
			/* round value down to the nearest 2 */
			Temp = (Temp/2) * 2;
		}

		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height = Temp;
		PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight       = Temp;
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height  = Temp;

              /* Try to figure out the correct ratio with widht and height */
              if(  (PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width != 0) &&
                    ((10000*PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height) / PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width) == (10000*9/16) )
              {
        		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
        		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
              }
              else
              {
        		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
        		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
              }

            /*      When the colour_primaries syntax element is not present,
             * the value of colour_primaries shall be inferred to be equal to 2
             * (the chromaticity is unspecified or is determined by the application).
             */
            PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries = UNSPECIFIED_COLOR_PRIMARIES;

            /* When the transfer_characteristics syntax element is not present,
             * the value of transfer_characteristics shall be inferred to be equal to 2
             * (the transfer characteristics are unspecified or are determined by the application)
             */
            PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics = UNSPECIFIED_TRANSFER_CHARACTERISTICS;

            /* When the matrix_coefficients syntax element is not present,
             * the value of matrix_coefficients shall be inferred to be equal to 2
             */
            PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients = MATRIX_COEFFICIENTS_UNSPECIFIED;

            /* According to spec SMPTE 421M, default value for Matrix Coefficient is ITU-R BT. 601-5*/
               PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;


		/* Now that the display width and height are known, reset the
		   Pan and Scan to default values based on the display width and height */
		vc1par_ResetPicturePanAndScanIn16thPixel(ParserHandle);

	/* READ STRUCT_C*/

	/* Struct C is big endian */

	/* Profile is 4 bits (see Annex J.1)
	      0  :  Simple
	      4  :  Main
	      12 :  Advanced  */
	Temp = vc1par_GetUnsigned(4, ParserHandle);

	switch(Temp)
	{
		case 0:
			PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_SIMPLE;
			/* Simple and Main Profile streams have always progressive frames*/
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType         = STGXOBJ_PROGRESSIVE_SCAN;
			PARSER_Data_p->PictureSpecificData.PictureSyntax                            = VC9_PICTURE_SYNTAX_PROGRESSIVE;
			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax			= VC9_PICTURE_SYNTAX_PROGRESSIVE;
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax			= VC9_PICTURE_SYNTAX_PROGRESSIVE;
			break;
		case 4:
			PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_MAIN;
			/* Simple and Main Profile streams have always progressive frames*/
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType         = STGXOBJ_PROGRESSIVE_SCAN;
			PARSER_Data_p->PictureSpecificData.PictureSyntax                            = VC9_PICTURE_SYNTAX_PROGRESSIVE;
			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax			= VC9_PICTURE_SYNTAX_PROGRESSIVE;
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax			= VC9_PICTURE_SYNTAX_PROGRESSIVE;
			break;
		case 12:
			PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_ADVANCE;
			break;
		default:
			STTBX_Print(("ERROR: Illegal Profile Specified %d\n", Temp));
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ProfileOutOfRange = TRUE;
			ProfileError = TRUE;

			/* If the next 28 bits are all zero, then the profile is probably Advanced*/
			if((Temp = vc1par_GetUnsigned(28, ParserHandle)) == 0)
			{
				PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_ADVANCE;
			}
			else /* otherwise, the profile should be either simple or main profile*/
			{
				/* See Annex J.1 */

				PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedFrameRateForPostProcessing = Temp >> 25;          /* 3 bits */
				PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing   = (Temp >> 20) & 0x1F; /* 5 bits */
				PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag                     = (Temp >> 19) & 1;	/* 1 bit */
				PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag                        = (Temp >> 17) & 1;	/* 1 bit */
				PARSER_Data_p->PictureDecodingContextSpecificData.Fastuvmc							 = (Temp >> 15) & 1;	/* 1 bit */
				PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag					 = (Temp >> 14) & 1;	/* 1 bit */
				PARSER_Data_p->PictureDecodingContextSpecificData.Dquant							 = (Temp >> 13) & 3;	/* 2 bits */
				PARSER_Data_p->PictureDecodingContextSpecificData.VstransformFlag					 = (Temp >> 11) & 1;	/* 1 bit */
				PARSER_Data_p->PictureDecodingContextSpecificData.OverlapFlag						 = (Temp >>  9) & 1;	/* 1 bit */
				PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag						 = (Temp >>  8) & 1;	/* 1 bit */
				PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag						 = (Temp >>  7) & 1;	/* 1 bit */
				PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes							 = (Temp >>  6) & 7;	/* 3 bits */
				PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer							 = (Temp >>  3) & 3;	/* 2 bits */
				PARSER_Data_p->GlobalDecodingContextSpecificData.FinterpFlag						 = (Temp >>  1) & 1;	/* 1 bit */

				if(PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag == 0)
				{
					PARSER_Data_p->PictureSpecificData.HalfWidthFlag = FALSE;
					PARSER_Data_p->PictureSpecificData.HalfHeightFlag = FALSE;
				}

				/* Since these values must always be zero for simple profile, if any of them
				   are one, assume Main profile, but if all of them are zero, assume simple
				   profile.  Of course, main profile could have all of these zero too, but
				   this is the only chance of getting a good value.*/
				if((PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag == 1)||
				   (PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag == 1)||
				   (PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag == 1))
				{
					PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_MAIN;
				}
				else
				{
					PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_SIMPLE;
				}
			}
			break;
	}/* end switch(Temp) */

	if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE)
	{
			/* TODO */
	}
	else
	{

		/* If there was a profile error, then this metadata was already read in, so don't read it in again.*/
		if(ProfileError == FALSE)
		{
			/* See Annex J.1 */
			PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedFrameRateForPostProcessing = vc1par_GetUnsigned(3, ParserHandle);

			PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing = vc1par_GetUnsigned(5, ParserHandle);

			PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE) &&
				(PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag == 1))
			{
				STTBX_Print(("ERROR: Simple profile streams must not have loop filtering enabled\n"));
				PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag = 0;
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.LoopFilterFlagSPOutOfRange = TRUE;
			}

			if((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 1)
			{
				STTBX_Print(("ERROR: Reserved3 set to one when it must be zero\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES3OutOfRange = TRUE;
			}

			/* MULTIRES */
			PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag = vc1par_GetUnsigned(1, ParserHandle);

			if(PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag == 0)
			{
				PARSER_Data_p->PictureSpecificData.HalfWidthFlag = FALSE;
				PARSER_Data_p->PictureSpecificData.HalfHeightFlag = FALSE;
			}

			if((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 0)
			{
				STTBX_Print(("ERROR: Reserved4 set to zero when it must be one\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES4OutOfRange = TRUE;
			}

			/* FASTUVMCFLAG */
			PARSER_Data_p->PictureDecodingContextSpecificData.Fastuvmc = vc1par_GetUnsigned(1, ParserHandle);

			/* ExtendedMVFlag */
			PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag = vc1par_GetUnsigned(1, ParserHandle);

			/* DQUANT */
			PARSER_Data_p->PictureDecodingContextSpecificData.Dquant = vc1par_GetUnsigned(2, ParserHandle);

			/* VSTRANSFORMFLAG */
			PARSER_Data_p->PictureDecodingContextSpecificData.VstransformFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 1)
			{
				STTBX_Print(("ERROR: Reserved5 set to one when it must be zero\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES5OutOfRange = TRUE;
			}

			/* OVERLAPFLAG */
			PARSER_Data_p->PictureDecodingContextSpecificData.OverlapFlag = vc1par_GetUnsigned(1, ParserHandle);

			/* SYNCMARKER */
			PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE) &&
				(PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag == 1))
			{
				STTBX_Print(("ERROR: Simple profile streams must not use SYNC MARKERS\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag = 0;
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.SyncMarkerFlagSPOutOfRange = TRUE;
			}

			/* RANGERED */
			PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE) &&
				(PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag == 1))
			{
				STTBX_Print(("ERROR: Simple profile streams must not use Range Reduction\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag = 0;
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RangeredFlagSPOutOfRange = TRUE;
			}

			PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes = vc1par_GetUnsigned(3, ParserHandle);
			PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer = vc1par_GetUnsigned(2, ParserHandle);
			PARSER_Data_p->GlobalDecodingContextSpecificData.FinterpFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 0)
			{
				STTBX_Print(("ERROR: Reserved6 set to zero when it must be one\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES6OutOfRange = TRUE;
			}
		}/* end if(ProfileError == FALSE)*/
	}

		/* Finished reading STRUCT C*/

		/* no STRUCT_B  set to default */

		PARSER_Data_p->PrivateGlobalContextData.Level = 2;

		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication =
		  ((PARSER_Data_p->PrivateGlobalContextData.Profile << 8) |
		    PARSER_Data_p->PrivateGlobalContextData.Level);

		PARSER_Data_p->GlobalDecodingContextSpecificData.ABSHrdBuffer =  0x1388;
		PARSER_Data_p->GlobalDecodingContextSpecificData.ABSHrdRate   =  0x1B7740;

		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = 24000;
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = 24000;

		PARSER_Data_p->StreamState.FrameRatePresent = TRUE;

		PARSER_Data_p->GlobalDecodingContextSpecificData.AebrFlag    = TRUE;
#endif /* ifndef STUB_INJECT*/

		PARSER_Data_p->PrivateGlobalContextData.GlobalContextValid = 1;
		PARSER_Data_p->PrivatePictureContextData.PictureContextValid = 1;

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
		PARSER_Data_p->GlobalDecodingContextSpecificData.AebrFlag    = FALSE;  /* No emulation if we use the signature*/
#else
		PARSER_Data_p->GlobalDecodingContextSpecificData.AebrFlag    = TRUE;
#endif


}/* end vc1par_ParseSpMpPesSeqHdr() */


/******************************************************************************/
/*! \brief Parses a simple/main profile file format sequence header (struct a, b, and c)
*   included in a VC1 file
*
* GlobalDecodingContext and PictureDecodingContext is updated with stream info
*
* \param ParserHandle Parser Handle (variables managed by the parser)
* \return void
*/
/******************************************************************************/
void vc1par_ParseSeqStructABC(const PARSER_Handle_t ParserHandle)
{
	U32 Temp, Temp1, i;
	BOOL ProfileError = FALSE;

	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"vc1par_ParseSeqStructABC  "));
#endif

	if(PARSER_Data_p->ParserJobResults.ParsingJobStatus.HasErrors == TRUE)
	{
		vc1par_InitErrorFlags(ParserHandle);
	}

	/* No Start Code Emulation Prevention bytes in the Struct ABC header. */
	vc1par_DisableDetectionOfStartCodeEmulationPreventionBytes(ParserHandle);

	if((Temp = vc1par_GetUnsigned(8, ParserHandle)) != 0xC5)
	{
		STTBX_Print(("ERROR: Expected 0xC5 at the beginning of the Seqence Layer Data Structure, Received %02x\n", Temp));
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.NoC5inRCVFormat = TRUE;
	}

	PARSER_Data_p->NumberOfFramesInFile = vc1par_GetUnsigned(24, ParserHandle);

	if((Temp = vc1par_GetUnsigned(32, ParserHandle)) != 0x00000004)
	{
		STTBX_Print(("ERROR: Expected 0x0000 0004 at the second word of the Seqence Layer Data Structure, Received %08x\n", Temp));
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.No00000004inRCVFormat = TRUE;
	}

	/* Struct C is big endian */
	vc1par_SwitchFromLittleEndianToBigEndianParsing(ParserHandle);

	/* Profile is 4 bits (see Annex J.1)
	      0  :  Simple
	      4  :  Main
	      12 :  Advanced  */
	Temp = vc1par_GetUnsigned(4, ParserHandle);

	switch(Temp)
	{
		case 0:
			PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_SIMPLE;
			/* Simple and Main Profile streams have always progressive frames*/
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType         = STGXOBJ_PROGRESSIVE_SCAN;
			PARSER_Data_p->PictureSpecificData.PictureSyntax                            = VC9_PICTURE_SYNTAX_PROGRESSIVE;
			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax			= VC9_PICTURE_SYNTAX_PROGRESSIVE;
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax			= VC9_PICTURE_SYNTAX_PROGRESSIVE;
			break;
		case 4:
			PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_MAIN;
			/* Simple and Main Profile streams have always progressive frames*/
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType         = STGXOBJ_PROGRESSIVE_SCAN;
			PARSER_Data_p->PictureSpecificData.PictureSyntax                            = VC9_PICTURE_SYNTAX_PROGRESSIVE;
			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax			= VC9_PICTURE_SYNTAX_PROGRESSIVE;
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax			= VC9_PICTURE_SYNTAX_PROGRESSIVE;
			break;
		case 12:
			PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_ADVANCE;
			break;
		default:
			STTBX_Print(("ERROR: Illegal Profile Specified %d\n", Temp));
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ProfileOutOfRange = TRUE;
			ProfileError = TRUE;

			/* If the next 28 bits are all zero, then the profile is probably Advanced*/
			if((Temp = vc1par_GetUnsigned(28, ParserHandle)) == 0)
			{
				PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_ADVANCE;
			}
			else /* otherwise, the profile should be either simple or main profile*/
			{
				/* See Annex J.1 */

				PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedFrameRateForPostProcessing = Temp >> 25;          /* 3 bits */
				PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing   = (Temp >> 20) & 0x1F; /* 5 bits */
				PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag                     = (Temp >> 19) & 1;	/* 1 bit */
				PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag                        = (Temp >> 17) & 1;	/* 1 bit */
				PARSER_Data_p->PictureDecodingContextSpecificData.Fastuvmc							 = (Temp >> 15) & 1;	/* 1 bit */
				PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag					 = (Temp >> 14) & 1;	/* 1 bit */
				PARSER_Data_p->PictureDecodingContextSpecificData.Dquant							 = (Temp >> 13) & 3;	/* 2 bits */
				PARSER_Data_p->PictureDecodingContextSpecificData.VstransformFlag					 = (Temp >> 11) & 1;	/* 1 bit */
				PARSER_Data_p->PictureDecodingContextSpecificData.OverlapFlag						 = (Temp >>  9) & 1;	/* 1 bit */
				PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag						 = (Temp >>  8) & 1;	/* 1 bit */
				PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag						 = (Temp >>  7) & 1;	/* 1 bit */
				PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes							 = (Temp >>  6) & 7;	/* 3 bits */
				PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer							 = (Temp >>  3) & 3;	/* 2 bits */
				PARSER_Data_p->GlobalDecodingContextSpecificData.FinterpFlag						 = (Temp >>  1) & 1;	/* 1 bit */

				if(PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag == 0)
				{
					PARSER_Data_p->PictureSpecificData.HalfWidthFlag = FALSE;
					PARSER_Data_p->PictureSpecificData.HalfHeightFlag = FALSE;
				}

				/* Since these values must always be zero for simple profile, if any of them
				   are one, assume Main profile, but if all of them are zero, assume simple
				   profile.  Of course, main profile could have all of these zero too, but
				   this is the only chance of getting a good value.*/
				if((PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag == 1)||
				   (PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag == 1)||
				   (PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag == 1))
				{
					PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_MAIN;
				}
				else
				{
					PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_SIMPLE;
				}
			}
			break;
	}/* end switch(Temp) */

	if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE)
	{
		/* If there was a profile error, then these 28 bits were already read in*/
		if(ProfileError == FALSE)
		{
			if((Temp = vc1par_GetUnsigned(28, ParserHandle)) != 0)
			{
				STTBX_Print(("ERROR: Reserved7 for Advanced Profile STRUCT_SEQUENCE_HEADER_C should be zero, but is equal to %d\n", Temp));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES7OutOfRange = TRUE;
			}
		}/* end if if(ProfileError == FALSE)*/

		/* The rest of Struct ABC is Little Endian. */
		vc1par_SwitchFromBigEndianToLittleEndianParsing(ParserHandle);

		/* skip STRUCT_A*/
		for(i = 0; i < 2; i++)
		{
			Temp = vc1par_GetUnsigned(32, ParserHandle);
		}
	}
	else
	{

		/* If there was a profile error, then this metadata was already read in, so don't read it in again.*/
		if(ProfileError == FALSE)
		{
			/* See Annex J.1 */
			PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedFrameRateForPostProcessing = vc1par_GetUnsigned(3, ParserHandle);

			PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing = vc1par_GetUnsigned(5, ParserHandle);

			PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE) &&
				(PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag == 1))
			{
				STTBX_Print(("ERROR: Simple profile streams must not have loop filtering enabled\n"));
				PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag = 0;
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.LoopFilterFlagSPOutOfRange = TRUE;
			}

			if((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 1)
			{
				STTBX_Print(("ERROR: Reserved3 set to one when it must be zero\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES3OutOfRange = TRUE;
			}

			/* MULTIRES */
			PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag = vc1par_GetUnsigned(1, ParserHandle);

			if(PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag == 0)
			{
				PARSER_Data_p->PictureSpecificData.HalfWidthFlag = FALSE;
				PARSER_Data_p->PictureSpecificData.HalfHeightFlag = FALSE;
			}

			if((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 0)
			{
				STTBX_Print(("ERROR: Reserved4 set to zero when it must be one\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES4OutOfRange = TRUE;
			}

			/* FASTUVMCFLAG */
			PARSER_Data_p->PictureDecodingContextSpecificData.Fastuvmc = vc1par_GetUnsigned(1, ParserHandle);

			/* ExtendedMVFlag */
			PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag = vc1par_GetUnsigned(1, ParserHandle);

			/* DQUANT */
			PARSER_Data_p->PictureDecodingContextSpecificData.Dquant = vc1par_GetUnsigned(2, ParserHandle);

			/* VSTRANSFORMFLAG */
			PARSER_Data_p->PictureDecodingContextSpecificData.VstransformFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 1)
			{
				STTBX_Print(("ERROR: Reserved5 set to one when it must be zero\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES5OutOfRange = TRUE;
			}

			/* OVERLAPFLAG */
			PARSER_Data_p->PictureDecodingContextSpecificData.OverlapFlag = vc1par_GetUnsigned(1, ParserHandle);

			/* SYNCMARKER */
			PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE) &&
				(PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag == 1))
			{
				STTBX_Print(("ERROR: Simple profile streams must not use SYNC MARKERS\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag = 0;
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.SyncMarkerFlagSPOutOfRange = TRUE;
			}

			/* RANGERED */
			PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE) &&
				(PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag == 1))
			{
				STTBX_Print(("ERROR: Simple profile streams must not use Range Reduction\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag = 0;
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RangeredFlagSPOutOfRange = TRUE;
			}

			PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes = vc1par_GetUnsigned(3, ParserHandle);
			PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer = vc1par_GetUnsigned(2, ParserHandle);
			PARSER_Data_p->GlobalDecodingContextSpecificData.FinterpFlag = vc1par_GetUnsigned(1, ParserHandle);

			if((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 0)
			{
				STTBX_Print(("ERROR: Reserved6 set to zero when it must be one\n"));
				PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES6OutOfRange = TRUE;
			}
		}/* end if(ProfileError == FALSE)*/

		/* Finished reading STRUCT C, Now read STRUCT A */

		/* The rest of Struct ABC is Little Endian. */
		vc1par_SwitchFromBigEndianToLittleEndianParsing(ParserHandle);

        /* Read the Height */
		Temp=vc1par_GetUnsigned(32, ParserHandle);

		if(Temp > VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT)
		{
			STTBX_Print(("ERROR: Height must be less than or eqaul to %d, Instead it is %u. Setting to Max value\n",VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT, Temp));
			Temp = VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT;
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedHeightOutOfRange = TRUE;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedHeightOutOfRange = TRUE;
		}
		/* Height must be an even value*/
		else if((Temp & 1) != 0)
		{
			STTBX_Print(("ERROR: Height must be even, Instead it is %u.  Rounding Value Down\n",Temp));
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedHeightOutOfRange = TRUE;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedHeightOutOfRange = TRUE;
			/* round value down to the nearest 2 */
			Temp = (Temp/2) * 2;
		}

		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height = Temp;
		PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight       = Temp;
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height  = Temp;

		/* Read the Width */
		Temp1=vc1par_GetUnsigned(32, ParserHandle);

		if(Temp1 > VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT)
		{
			STTBX_Print(("ERROR: Width must be less than %d, Instead it is %u.  Setting to Max value\n",VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT, Temp1));
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedWidthOutOfRange = TRUE;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedWidthOutOfRange = TRUE;
			Temp1 = VC1_SPAP_MAX_WIDTH_AND_MAX_HEIGHT;
		}
		/* Width must be an even value*/
		else if((Temp1 & 1) != 0)
		{
			STTBX_Print(("ERROR: Width must be even, Instead it is %u.  Rounding Value Down\n",Temp1));
			/* round value down to the nearest 2 */
			Temp1 = (Temp1/2) * 2;
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedWidthOutOfRange = TRUE;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedWidthOutOfRange = TRUE;
		}
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width = Temp1;
		PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth       = Temp1;
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width  = Temp1;
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Pitch  = Temp1;

		/* Since no aspect ratio is given for the RCV format, use the default aspect ratio.*/
              /* Try to figure out the correct ratio with widht and height */
              if(  (PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width != 0) &&
                    ((10000*PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height) / PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width) == (10000*9/16) )
              {
        		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
        		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;

              }
              else
              {
        		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
        		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;

              }


		/* Now that the display width and height are known, reset the
		   Pan and Scan to default values based on the display width and height */
		vc1par_ResetPicturePanAndScanIn16thPixel(ParserHandle);


#ifdef VC1_DEBUG_PARSER

		if(PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedFrameRateForPostProcessing == 7)
		{
			VC1_PARSER_PRINT(("Quantized frame rate is around 30 frames/second or more\n"));
		}
		else
		{
			VC1_PARSER_PRINT(("Quantized frame rate is around %d frames/second\n",
				(2 + PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedFrameRateForPostProcessing *4)));
		}

		if(PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing == 31)
		{
			VC1_PARSER_PRINT(("Quantized bit rate is around 2016 kbps or more \n"));
		}
		else
		{
			VC1_PARSER_PRINT(("Quantized bit rate is around %d kbps\n",
				32 + PARSER_Data_p->GlobalDecodingContextSpecificData.QuantizedBitRateForPostProcessing * 64));
		}

		VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag == 1) ? "Loop Filtering Enabled":"Loop Filtering Disabled"));

		VC1_PARSER_PRINT(("Multires flag = %d\n", PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag));

		VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.Fastuvmc == 1) ? "Fast UV Motion Compensation Flag Enabled":"Fast UV Motion Compensation Disabled"));

		VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag == 1) ? "Extended Motion Vectors Enabled":"Extended Motion Vectors Disabled"));

		if(PARSER_Data_p->PictureDecodingContextSpecificData.Dquant == 0)
		{
			VC1_PARSER_PRINT(("One Quant Step Size Per Frame\n"));
		}
		else if(PARSER_Data_p->PictureDecodingContextSpecificData.Dquant < 3)
		{
			VC1_PARSER_PRINT(("Varying Quant Step Size Per Frame\n"));
		}
		else
		{
			VC1_PARSER_PRINT(("Warning, Reserved DQuant Value Used\n"));
		}

		VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.VstransformFlag == 1) ?
			"Variable-Sized Transform Coding Enabled":"Variable-Sized Transform Coding Disabled"));

		VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.OverlapFlag == 1) ?
			"Overlap Transforms May Be Used":"Overlap Transforms Not Used"));

		VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->GlobalDecodingContextSpecificData.SyncmarkerFlag == 1) ?
			"SYNC MARKERS May Be Used":"SYNC MARKERS Not Used"));

		VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag == 1) ?
			"Range Reduction Used":"Range Reduction Not Used"));

		VC1_PARSER_PRINT(("%s, MAXBFRAMES = %d\n", (PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes == 0) ?
			"No B Frames in the stream":"B frames may be present in the stream", PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes));

		switch (PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer)
		{
		case 0:
			VC1_PARSER_PRINT(("Quantizer implicitly specified at frame level\n"));
			break;
		case 1:
			VC1_PARSER_PRINT(("Quantizer explicitly specified at frame level\n"));
			break;
		case 2:
			VC1_PARSER_PRINT(("Nonuniform quantizer used for all frames\n"));
			break;
		case 3:
			VC1_PARSER_PRINT(("Uniform quantizer used for all frames\n"));
			break;
		}

		VC1_PARSER_PRINT(("FinterpFlag = %d\n",PARSER_Data_p->GlobalDecodingContextSpecificData.FinterpFlag));
		VC1_PARSER_PRINT(("Height = %u\n",PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight));
		VC1_PARSER_PRINT(("Width = %d\n",PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth));

#endif /* VC1_DEBUG_PARSER */

	}/* end if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE) -- else */


	if((Temp = vc1par_GetUnsigned(32, ParserHandle)) != 0x0000000C)
	{
		STTBX_Print(("ERROR: Expected 0x0000 000C After Struct A, Received %08x\n", Temp));
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.No0000000CinRCVFormat = TRUE;
	}

    /* Read STRUCT B */

	if((PARSER_Data_p->PrivateGlobalContextData.Level = vc1par_GetUnsigned(3, ParserHandle)) > 4)
	{
		/* @@@ERROR print level error, and act on it. */
		STTBX_Print(("ERROR:  Level is too high, it should be less than 4, but it equals %d\n", PARSER_Data_p->PrivateGlobalContextData.Level));

		if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE)
		{
			PARSER_Data_p->PrivateGlobalContextData.Level = 2;
		}
		else
		{
			PARSER_Data_p->PrivateGlobalContextData.Level = 4;
		}
	}/* if((PARSER_Data_p->PrivateGlobalContextData.Level = vc1par_GetUnsigned(3, ParserHandle)) > 4) */

	/* values of 0, 2, and 4  are valid for Main profile, but
	   only 0 and 2 are valid for Simple profile. */
	else if((PARSER_Data_p->PrivateGlobalContextData.Profile != VC9_PROFILE_ADVANCE) &&
		    (PARSER_Data_p->PrivateGlobalContextData.Level % 2 != 0))
	{
		STTBX_Print(("ERROR:  Illegal Level indicated: %d\n", PARSER_Data_p->PrivateGlobalContextData.Level));

		/* round the Level down from 3 to 2 or from 1 to 0. */
		PARSER_Data_p->PrivateGlobalContextData.Level = (PARSER_Data_p->PrivateGlobalContextData.Level/2) * 2;

		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.LevelOutOfRange = TRUE;
	}
	else if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE) &&
		    (PARSER_Data_p->PrivateGlobalContextData.Level == 4))
	{
		/* If there was a profile error, it is now probable that we guessed the profile to
		   be SIMPLE profile, but it was really main profile because the level 4 would be
		   valid for main profile, but not for simple profile. */
		if(ProfileError == TRUE)
		{
			PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_MAIN;
		}
		else
		{
			STTBX_Print(("ERROR:  Illegal Level indicated: %d\n", PARSER_Data_p->PrivateGlobalContextData.Level));
			PARSER_Data_p->PrivateGlobalContextData.Level = 2;
			PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.LevelOutOfRange = TRUE;
		}
	}/* else if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE)...*/

	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication =
		  ((PARSER_Data_p->PrivateGlobalContextData.Profile << 8) |  /* bits 8 - 15 : Profile */
		    PARSER_Data_p->PrivateGlobalContextData.Level);			 /* bits 0 - 7  : Level   */

	/* @@@ CBR -- see J.2*/
 	Temp = vc1par_GetUnsigned(1, ParserHandle);

	VC1_PARSER_PRINT(("CBR Flag = %d\n",Temp));


	if((Temp = vc1par_GetUnsigned(4, ParserHandle)) != 0)
	{
		STTBX_Print(("ERROR: RES1 should be 0, but is %d\n",Temp));
		/*PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES1OutOfRange = TRUE; */
	}

	Temp = vc1par_GetUnsigned(24, ParserHandle);
	Temp1 = vc1par_GetUnsigned(32, ParserHandle);

	/* For advanced profile, these variables should both be zero */
	if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE) &&
	   ((Temp != 0) || (Temp1 != 0)))
	{
		STTBX_Print(("ERROR: RES2 should be 0, but is %06x %08x\n", Temp, Temp1));
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES2OutOfRange = TRUE;
	}
	else
	{
		/* These variables will only be used by the producer/decoder in
		   Simple and Main Profile. */
		PARSER_Data_p->GlobalDecodingContextSpecificData.ABSHrdBuffer =  Temp;
		PARSER_Data_p->GlobalDecodingContextSpecificData.ABSHrdRate   =  Temp1;

		/* HRD Buffer and HRD rate should not both be zero for Simple and Main profile
		   If this is the case and we had an error detecting the profile, its now probable
		   that the profile really was advanced profile. and that we have just read RES2
		   (rather than ABSHrdBuffer and ABSHrdRate. So change the
		   profile to advanced profile.*/
		if((ProfileError == TRUE) && (Temp == 0) && (Temp1 == 0))
		{
			PARSER_Data_p->PrivateGlobalContextData.Profile = VC9_PROFILE_ADVANCE;
		}
	}/* if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE) && ... -- else */

	/* FRAMERATE */
	Temp = vc1par_GetUnsigned(32, ParserHandle);

	if((Temp * 1000) < 0xFFFFFFFF)
	{
		/* framerate is specified in terms of frames per millisecond */
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = Temp * 1000;
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate   = Temp * 1000;
	}

	/* If the frame rate is high enough such that multiplying it by 1000 overflows an integer,
	   then just use a default frame rate and calculate the framerate using the timestamps.
	   This also includes 0xffffffff which is means unspecified
	   frame rate */
	else /* unspecified frame rate, so use frame rate of 30 frames per second */
		 /* @@@ do we need a better default frame rate */
	{
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = VC1_DEFAULT_FRAME_RATE;
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = VC1_DEFAULT_FRAME_RATE;
		PARSER_Data_p->StreamState.FrameRatePresent = FALSE;
	}
	/* Detection of Start Code Emulation Prevention bytes will not be
	   needed for the rest of the Simple and Main Profile stream, but
	   it will be needed for the BDUs in the rest of the Advanced profile stream. */
	if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE)
	{
		vc1par_EnableDetectionOfStartCodeEmulationPreventionBytes(ParserHandle);
	}
	else
	{
		/* We have now received all of the Global and Picture Context Data
		   for Simple and Main profile.   For Advanced profile, this comes
		   later in the stream.  */
		PARSER_Data_p->PrivateGlobalContextData.GlobalContextValid   = 1;
		PARSER_Data_p->PrivatePictureContextData.PictureContextValid = 1;
		/* No Anti-Emulation Byte removal for Simple main profile streams coming from a file*/
		PARSER_Data_p->GlobalDecodingContextSpecificData.AebrFlag    = FALSE;
	}

#ifdef VC1_DEBUG_PARSER
	VC1_PARSER_PRINT(("Level = %d\n",PARSER_Data_p->PrivateGlobalContextData.Level));
	VC1_PARSER_PRINT((	"Profile and Level = %08x\n",PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication));
#endif /* VC1_DEBUG_PARSER */

}/* end vc1par_ParseSeqStructABC() */


/******************************************************************************/
/*! \brief Parses entry point header (Advanced profile only)
*
* PictureDecodingContext is updated with stream info
*
* \param ParserHandle Parser Handle (variables managed by the parser)
* \return void
*/
/******************************************************************************/
void vc1par_ParseEntryPointHeader(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	U32 Temp;
	U32 i;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
		TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"vc1par_ParseEntryPointHeader\n"));
#endif

	if(PARSER_Data_p->ParserJobResults.ParsingJobStatus.HasErrors == TRUE)
	{
		PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.BrokenLinkOutOfRange  = FALSE;
		PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedWidthOutOfRange  = FALSE;
		PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedHeightOutOfRange = FALSE;
	}

	/* BROKENLINKFLAG */
	PARSER_Data_p->PrivatePictureContextData.BrokenLinkFlag= vc1par_GetUnsigned(1, ParserHandle);

/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"%s\n", (PARSER_Data_p->PrivatePictureContextData.BrokenLinkFlag == 1) ? "Broken Link":"No Broken Link"));
#endif */

	/* CLOSEDENTRYFLAG */
	PARSER_Data_p->PrivatePictureContextData.ClosedEntryFlag = vc1par_GetUnsigned(1, ParserHandle);
/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"%s\n", (PARSER_Data_p->PrivatePictureContextData.ClosedEntryFlag == 1) ? "Closed Entry Point":"Open Entry Point"));
#endif*/

	/* If Closed Entry point, we do not need any of our reference frames any more, so
	 * mark them as no longer valid.  Also mark the 3 reference lists as empty.
	 */
	if(PARSER_Data_p->PrivatePictureContextData.ClosedEntryFlag == 1)
	{
#if 0
		for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; i++)
		{
			PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i] = FALSE;
			PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = FALSE;
		}
		/* The first reference picture will be placed at index zero in the
		 * FullReferenceFrameList.
		 */
		PARSER_Data_p->StreamState.NextReferencePictureIndex = 0;

		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
#endif /* 0 */
		/* When closed entry flag is one, broken link flag must be zero.*/
		if(PARSER_Data_p->PrivatePictureContextData.BrokenLinkFlag == 1)
		{
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.BrokenLinkOutOfRange = TRUE;
			PARSER_Data_p->PrivatePictureContextData.BrokenLinkFlag = 0;
		}

	}/* end if(PARSER_Data_p->PrivatePictureContextData.ClosedEntryFlag == 1) */
	/* If Open entry point and Broken Link, that means there are missing
	 * pictures in the stream before the entry point header.  In this case,
	 * all current reference pictures must be marked with the broken link
	 * flag.
	 */
	else if(PARSER_Data_p->PrivatePictureContextData.BrokenLinkFlag == 1)
	{
		for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; i++)
		{
			/* mark all valid reference pictures with the broken link flag. */
			if(PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i] == TRUE)
				PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = TRUE;
		}
	}

	/* PANSCANFLAG */
	PARSER_Data_p->PictureDecodingContextSpecificData.PanScanFlag = vc1par_GetUnsigned(1, ParserHandle);

	/* REFDISTFLAG */
	PARSER_Data_p->PictureDecodingContextSpecificData.RefDistFlag = vc1par_GetUnsigned(1, ParserHandle);

	/* LOOPFILTERFLAG */
	PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag = vc1par_GetUnsigned(1, ParserHandle);

	/* FASTUVMCFLAG */
	PARSER_Data_p->PictureDecodingContextSpecificData.Fastuvmc = vc1par_GetUnsigned(1, ParserHandle);

	/* ExtendedMVFlag */
	PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag = vc1par_GetUnsigned(1, ParserHandle);

	/* DQUANT */
	PARSER_Data_p->PictureDecodingContextSpecificData.Dquant = vc1par_GetUnsigned(2, ParserHandle);

	if(PARSER_Data_p->PictureDecodingContextSpecificData.Dquant >= 3)
	{
       STTBX_Print(("Warning, Reserved DQuant Value Used (%d)\n", PARSER_Data_p->PictureDecodingContextSpecificData.Dquant));
	}

	/* VSTRANSFORMFLAG */
	PARSER_Data_p->PictureDecodingContextSpecificData.VstransformFlag = vc1par_GetUnsigned(1, ParserHandle);

	/* OVERLAPFLAG */
	PARSER_Data_p->PictureDecodingContextSpecificData.OverlapFlag = vc1par_GetUnsigned(1, ParserHandle);

	/* QUANTIZER */
	PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer = vc1par_GetUnsigned(2, ParserHandle);


	if(PARSER_Data_p->PrivateGlobalContextData.HRDParamFlag == 1)
	{
		/* process each of the HRD fullness
		 * array elements
		 */
		for(i=0; i < PARSER_Data_p->GlobalDecodingContextSpecificData.NumLeakyBuckets; i++)
		{
			/* $HRDfullness -- should this be fully calculated here? */
			PARSER_Data_p->PictureDecodingContextSpecificData.HRDFullness[i] = vc1par_GetUnsigned(8, ParserHandle);
			/* PARSER_Data_p->PictureDecodingContextSpecificData.HRDFullness[i] = (Temp+1)*$HRDbuffsize_arr[i]/256; */
#ifdef VC1_DEBUG_PARSER
			VC1_PARSER_PRINT((" HRD FULLNESS %d = %u\n",i,PARSER_Data_p->PictureDecodingContextSpecificData.HRDFullness[i]));
#endif /* VC1_DEBUG_PARSER */

		}
	}

#ifdef VC1_DEBUG_PARSER
	VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.PanScanFlag == 1) ? "Pan/Scan info exists in picture header":"Pan/Scan info not in picture header"));
	VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.RefDistFlag == 1) ? "Reference Frame Distance Exists":"No Reference Frame Distance"));
	VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.LoopFilterFlag == 1) ? "Loop Filtering Enabled":"Loop Filtering Disabled"));
	VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.Fastuvmc == 1) ? "Fast UV Motion Compensation Flag Enabled":"Fast UV Motion Compensation Disabled"));
	VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag == 1) ? "Extended Motion Vectors Enabled":"Extended Motion Vectors Disabled"));

	if(PARSER_Data_p->PictureDecodingContextSpecificData.Dquant == 0)
	{
       VC1_PARSER_PRINT(("One Quant Step Size Per Frame\n"));
	}
	else if(PARSER_Data_p->PictureDecodingContextSpecificData.Dquant < 3)
	{
       VC1_PARSER_PRINT(("Varying Quant Step Size Per Frame\n"));
    }

	VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.VstransformFlag == 1) ?
					"Variable-Sized Transform Coding Enabled":"Variable-Sized Transform Coding Disabled"));

	VC1_PARSER_PRINT(("%s\n", (PARSER_Data_p->PictureDecodingContextSpecificData.OverlapFlag == 1) ?
						"Overlap Transforms May Be Used":"Overlap Transforms Not Used"));

	switch (PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer)
	{
		case 0:
			VC1_PARSER_PRINT(("Quantizer implicitly specified at frame level\n"));
			break;
		case 1:
			VC1_PARSER_PRINT(("Quantizer explicitly specified at frame level\n"));
			break;
		case 2:
			VC1_PARSER_PRINT(("Nonuniform quantizer used for all frames\n"));
			break;
		case 3:
			VC1_PARSER_PRINT(("Uniform quantizer used for all frames\n"));
			break;
	}

#endif /* VC1_DEBUG_PARSER */

	/* CODEDSIZEFLAG */
	Temp = vc1par_GetUnsigned(1, ParserHandle);

	if(Temp==1)
	{
		/* CODEDWIDTH */
		Temp = vc1par_GetUnsigned(12, ParserHandle);
		PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth = (Temp*2)+2;

		if(PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth >
		   PARSER_Data_p->StreamState.MaxWidth)
		{
			STTBX_Print(("ERROR: CODED WIDTH %d is greater than MAX CODED WIDTH %d -- correction: setting equal to MAX CODED WIDTH.\n",
				PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth, PARSER_Data_p->StreamState.MaxWidth));
			PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth = PARSER_Data_p->StreamState.MaxWidth;
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width = PARSER_Data_p->StreamState.MaxWidth;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedWidthOutOfRange = TRUE;
		}
		else
		{
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width = PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth;
		}

		/* CODEDHEIGHT  */
		Temp = vc1par_GetUnsigned(12, ParserHandle);
		PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight = (Temp*2)+2;

		if(PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight >
		   PARSER_Data_p->StreamState.MaxHeight)
		{
			STTBX_Print(("ERROR: CODED HEIGHT %d is greater than MAX CODED HEIGHT %d -- correction: setting equal to MAX CODED HEIGHT.\n",
				PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight, PARSER_Data_p->StreamState.MaxHeight));
			PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight = PARSER_Data_p->StreamState.MaxHeight;
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height = PARSER_Data_p->StreamState.MaxHeight;
			PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedHeightOutOfRange = TRUE;
		}
		else
		{
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height = PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight;
		}

#ifdef VC1_DEBUG_PARSER
		VC1_PARSER_PRINT(("CODED WIDTH & HEIGHT present\n"));
		VC1_PARSER_PRINT(("Coded Witdh = %d\n", PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth));
		VC1_PARSER_PRINT(("Coded Height = %d\n", PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight));
#endif /* VC1_DEBUG_PARSER */

	}
	else
	{
#ifdef VC1_DEBUG_PARSER
		VC1_PARSER_PRINT(("CODED WIDTH & HEIGHT not present, setting them to maximum value.\n"));
#endif /* VC1_DEBUG_PARSER */
		PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth = PARSER_Data_p->StreamState.MaxWidth;
		PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight = PARSER_Data_p->StreamState.MaxHeight;
	}

	if(PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag == 1)
	{
		PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedDMVFlag = vc1par_GetUnsigned(1, ParserHandle);

	}/* end if(...ExtendedMVFlag] == 1) */
	else
	{
		PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedDMVFlag = 0;
	}

	/* RANGEMAPYFLAG */
	PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPYFlag = vc1par_GetUnsigned(1, ParserHandle);

	if(PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPYFlag == 1)
	{
		PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPY = vc1par_GetUnsigned(3, ParserHandle);

		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorY =
			YUV_RANGE_MAP_0 + PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPY;
	}
	else
	{
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorY = YUV_NO_RESCALE;
	}

	/* RANGEMAPUVFLAG */
	PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPUVFlag = vc1par_GetUnsigned(1, ParserHandle);

	if(PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPUVFlag == 1)
	{
		PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPUV = vc1par_GetUnsigned(3, ParserHandle);

		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorUV =
			YUV_RANGE_MAP_0 + PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPUV;
	}
	else
	{
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorUV = YUV_NO_RESCALE;
	}


#ifdef VC1_DEBUG_PARSER

	if(PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag == 1)
	{
		VC1_PARSER_PRINT(("%s",PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedDMVFlag == 1 ?
			"ext diff MV range signaled in P,B pict layers\n":
		    "extended differential MV range not signaled\n"));

	}/* end if(...ExtendedMVFlag] == 1) */
	else
	{
		VC1_PARSER_PRINT(("EXTENDED DMV flag not present\n"));
	}

	if(PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPYFlag == 1)
	{
		VC1_PARSER_PRINT(("RANGE_MAPY present\n"));
		VC1_PARSER_PRINT(("luma comps scaled using RANGE_MAPY %d\n", PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPY));
	}
	else
	{
		VC1_PARSER_PRINT(("RANGE_MAPY not present\n"));
	}

	if(PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPUVFlag == 1)
	{
		VC1_PARSER_PRINT(("RANGE_MAPUV present\n"));
		VC1_PARSER_PRINT(("color-diff comps scaled using RANGE_MAPUV %d\n", PARSER_Data_p->PictureDecodingContextSpecificData.Range_MAPUV));
	}
	else
	{
		VC1_PARSER_PRINT(("RANGE_MAPUV not present\n"));
	}
#endif

	PARSER_Data_p->PrivatePictureContextData.PictureContextValid = TRUE;
	PARSER_Data_p->NextPicRandomAccessPoint = TRUE;
}/* end vc1par_ParseEntryPointHeader() */

/*******************************************************************************/
/*! \brief Parses a Advanced Profile frame header
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
* \return void
*/
/******************************************************************************/
void vc1par_ParseAPPicture(U8 *StartCodeValueAddress, U8 *NextBDUStartCodeValueAddress, const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	U32 Temp, i;
	U32 NumPanScanWindows = 0;
	U32 NumFramesToRepeat = 0;
	U32 RepeatFirstField = 0;
	U32 TopFieldFirst = 1;
	U32 PSHoffset,PSVoffset,PSWidth,PSHeight;

	/* For FCM interpretation. */
	U32 FcmVlcTable[VC1_NUM_FRAME_CODING_MODES] = {0,2,0,0};
#ifdef VC1_DEBUG_PARSER
	U8  FcmDescription[VC1_NUM_FRAME_CODING_MODES][20] = {"Progressive","Invalid","Frame-Interlace","Field-Interlace"};
#endif
	U32 Fcm = VC1_PROGRESSIVE;

	/* For Picture / Frame Picture Type interpretation. */
	U32 PTypeVlcTable[VC1_NUM_PICTURE_TYPES] = {0,2,0,6,0,0,0,14,0,0,0,0,0,0,0,0};
#if defined(TRACE_UART) && defined(VALID_TOOLS)
	U8  PTypeDescription[VC1_NUM_PICTURE_TYPES][15]={"P Frame","Invalid","B Frame","Invalid","Invalid",
						   "Invalid","I Frame","Invalid","Invalid","Invalid",
						   "Invalid","Invalid","Invalid","Invalid","BI Frame","Skipped Frame"};
    U8 FPTypeDescription[VC1_NUM_FIELD_PICTURE_TYPES][5] ={"I/I","I/P","P/I","P/P","B/B","B/BI","BI/B","BI/BI"};
#endif
	U32  PType;
	U8   PSPresent;



	/* REFDIST Interpretation */
/*	U32	RefDistVlcTable[] = {2,4,0,0,0,6,0,8,0,10,0,12,0,14,0,16,0,18,0,20,0,22,0,24,0,26,0,27,0,28,0,30,0,0}; */


	/* TRUE if this is a reference picture, false otherwise; */
	BOOL RefPic = FALSE;
	U8 UVSamp = 1;

	/* used for display order calculataions */
	U32 PicDiff;
	U32 RefPicIndex0, RefPicIndex1;

	/* Intensity Compensation information */
	U8   IntCompField;
	U32  IntComp1;
	U32  IntComp2;
	U32  IntComp1Mod;
	U32  IntComp2Mod;
	U32  FramePicLayerSize = 0;

	BOOL BFractPresent = FALSE;


#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"vc1par_APParsePicture  "));
#endif

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Update position in bit buffer */
    /* The picture start address is the byte after the Start Code Value */
    PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress = (void *) vc1par_MoveForwardInStream(StartCodeValueAddress, 1, ParserHandle);
	/* The stop address points to the Start Code Value Byte of the next start code */
	PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  = NextBDUStartCodeValueAddress;
    /* Set to FALSE by default */
	PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = FALSE;

	vc1par_SearchForAssociatedPTS(ParserHandle);

    if (PARSER_Data_p->PTSAvailableForNextPicture)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = TRUE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS = PARSER_Data_p->PTSForNextPicture;
        PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
    }
    else
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE; /* Set to TRUE as soon as a PTS is detected */
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS = 0;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33 = FALSE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated = TRUE; /* default zero PTS value must be considered as interpolated for the producer */
    }


	if (PARSER_Data_p->GlobalDecodingContextSpecificData.InterlaceFlag == 1)
	{
		PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[2] = FALSE;

		Fcm = vc1par_GetVariableLengthCode(FcmVlcTable, 2, ParserHandle);

		if((Fcm == VC1_PROGRESSIVE) || (Fcm == VC1_FRAME_INTERLACE))
		{
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;

			if(Fcm == VC1_PROGRESSIVE)
			{
				PARSER_Data_p->PictureSpecificData.PictureSyntax = VC9_PICTURE_SYNTAX_PROGRESSIVE;
			}
			else
			{
				PARSER_Data_p->PictureSpecificData.PictureSyntax = VC9_PICTURE_SYNTAX_INTERLACED_FRAME;
			}
		}
		else if(Fcm == VC1_FIELD_INTERLACE)
		{
			PARSER_Data_p->PictureSpecificData.PictureSyntax = VC9_PICTURE_SYNTAX_INTERLACED_FIELD;
		}
		else if(Fcm == VC1_INVALID_FCM)
		{
			STTBX_Print(("ERROR: Invalid FCM (%d) found (problem getting variable length code)\n", Fcm));
		}

		PARSER_Data_p->PictureSpecificData.SecondFieldFlag = 0;

	}/* end if (PARSER_Data_p->GlobalDecodingContextSpecificData.InterlaceFlag==1)*/

	/* If not Field Interlace picture, get the picture type as a VLC */
	if(Fcm != VC1_FIELD_INTERLACE)
	{
		PType = vc1par_GetVariableLengthCode(PTypeVlcTable,4,ParserHandle);
#if defined(TRACE_UART) && defined(VALID_TOOLS)
		TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"PType %s (%d)  ",PTypeDescription[PType], PType));
#endif
		switch(PType)
		{
			case VC1_P_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_P;

				RefPic = TRUE;
				break;

			case VC1_B_PICTURE_TYPE:

				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_B;
				RefPic = FALSE;
				BFractPresent = TRUE;
				break;

			case VC1_I_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_I;

				RefPic = TRUE;
				break;

			case VC1_BI_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_BI;

				RefPic = FALSE;
				break;

			case VC1_SKIP_PICTURE_TYPE:

				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_SKIP;

				RefPic = TRUE;
				break;
			default:
				STTBX_Print(("ERROR: Picture Type %d is not defined", PType));

				break;

		}/* end switch(PType) */
	} /* end if(Fcm != VC1_FIELD_INTERLACE) */
	else /* otherwise, get the field picture type */
	{
		PARSER_Data_p->ExpectingFieldBDU = TRUE;
		PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields = TRUE;
		FramePicLayerSize = VC1_FIELD_PIC_FCM_FIELD_SIZE + VC1_FPTYPE_FIELD_SIZE;

		/* FPTYPE */
		PType = vc1par_GetUnsigned(VC1_FPTYPE_FIELD_SIZE, ParserHandle);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
		TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"FP Type %s (%d)  ",FPTypeDescription[PType], PType));
#endif

		switch(PType)
		{
			case VC1_I_I_FIELD_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_I;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific = VC9_PICTURE_TYPE_I;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeGeneric  = STVID_MPEG_FRAME_I;
				RefPic = TRUE;
				break;
			case VC1_I_P_FIELD_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_I;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific = VC9_PICTURE_TYPE_P;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeGeneric  = STVID_MPEG_FRAME_P;
				RefPic = TRUE;
				break;
			case VC1_P_I_FIELD_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_P;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific = VC9_PICTURE_TYPE_I;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeGeneric  = STVID_MPEG_FRAME_I;
				RefPic = TRUE;
				break;
			case VC1_P_P_FIELD_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_P;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific = VC9_PICTURE_TYPE_P;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeGeneric  = STVID_MPEG_FRAME_P;
				RefPic = TRUE;
				break;
			case VC1_B_B_FIELD_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_B;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific = VC9_PICTURE_TYPE_B;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeGeneric  = STVID_MPEG_FRAME_B;
                RefPic = FALSE; /* The first field in the field-pair is reference for the second field in the field-pair */
				BFractPresent = TRUE;
				break;
			case VC1_B_BI_FIELD_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_B;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific = VC9_PICTURE_TYPE_BI;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeGeneric  = STVID_MPEG_FRAME_B;
				RefPic = FALSE;/* Since BI is the second field, The second field does not depend on the first field,
				                  therefore this picture is not used as reference */
				BFractPresent = TRUE;
				break;
			case VC1_BI_B_FIELD_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_BI;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific = VC9_PICTURE_TYPE_B;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeGeneric  = STVID_MPEG_FRAME_B;
                RefPic = FALSE; /* The first field in the field-pair is reference for the second field in the field-pair */
				BFractPresent = TRUE;
				break;
			case VC1_BI_BI_FIELD_PICTURE_TYPE:
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_BI;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific = VC9_PICTURE_TYPE_BI;
				PARSER_Data_p->StreamState.SecondFieldPictureTypeGeneric  = STVID_MPEG_FRAME_B;
				RefPic = FALSE;/* Since BI is the second field, The second field does not depend on the first field,
				                  therefore this picture is not used as reference */
				BFractPresent = TRUE;
				break;
			default:
				STTBX_Print(("ERROR: Frame Picture Type %d is not defined", PType));

				break;

		} /* end switch(PType) */

		/* indicate that we expect the next BDU to be a field. */
		PARSER_Data_p->ExpectingFieldBDU = TRUE;
	}/* end else field picture type */

	PARSER_Data_p->PictureGenericData.IsReference = RefPic;


    /* skipped pictures do not have a tfcntr */
	if ((PARSER_Data_p->GlobalDecodingContextSpecificData.TfcntrFlag==1) &&
		(PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_SKIP))
	{
		/* TFCNTR */
		Temp = vc1par_GetUnsigned(VC1_TFCNTR_FIELD_SIZE, ParserHandle);
		FramePicLayerSize += VC1_TFCNTR_FIELD_SIZE;

#if defined(TRACE_UART) && defined(VALID_TOOLS)
		/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"TFCNTR %d  ",Temp)); */
#endif
		if(RefPic)
		{
			/* if the BackwardRefPresentationOrderPictureID is greater than CurrentPresentationOrderPictureID,
			   then things are normal and CurrentPresentationOrderPictureID gets set to
			   BackwardRefPresentationOrderPictureID; otherwise, there has been a missing reference picture,
			   so, just increment CurrentPresentationOrderPictureID by one.   This should account for the missing
			   reference picture. */
			if((PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID >
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID) ||
			   (PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension >
			    PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension) ||
				((PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension == 0) &&
				 (PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension == MAXIMUM_U32)))
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID = PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID;
				PARSER_Data_p->StreamState.CurrentTfcntr = PARSER_Data_p->StreamState.BackwardRefTfcntr;
			}
			else
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID++;
				PARSER_Data_p->StreamState.CurrentTfcntr++;
			}

			/* handle possible rollover of Tfcntr */
			if(Temp <= PARSER_Data_p->StreamState.BackwardRefTfcntr)
			{
				PicDiff = (MAXIMUM_U8 + 1 - PARSER_Data_p->StreamState.BackwardRefTfcntr) + Temp;
			}
			else
			{
				PicDiff = Temp - PARSER_Data_p->StreamState.BackwardRefTfcntr;
			}

			PARSER_Data_p->StreamState.BackwardRefTfcntr = Temp;

			/* handle possible rollover of BackwardRefPresentationOrderPictureID */
			if( (S32)(MAXIMUM_S32) -  PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID < (S32)PicDiff)
			{
				if(PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension == MAXIMUM_U32)
				{
					PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension = 0;
				}
				else
				{
					PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension++;
				}

				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID = PicDiff -
					((MAXIMUM_U32 - PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID) + 1);
			}
			else
			{
				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID += PicDiff;
			}

			PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID =
				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID;
			PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
		}/*  end if(RefPic)*/
		else /* Not a reference picture */
		{
			/* handle possible rollover of Tfcntr */
			if(Temp <= PARSER_Data_p->StreamState.CurrentTfcntr)
			{
				PicDiff = (MAXIMUM_U8 + 1 - PARSER_Data_p->StreamState.CurrentTfcntr) + Temp;
			}
			else
			{
				PicDiff = Temp - PARSER_Data_p->StreamState.CurrentTfcntr;
			}

			PARSER_Data_p->StreamState.CurrentTfcntr = Temp;

			/* handle possible rollover of CurrentPresentationOrderPictureID */
			if( (S32)(MAXIMUM_S32) -  PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID < (S32)PicDiff)
			{
				if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension == MAXIMUM_U32)
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension = 0;
				}
				else
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension++;
				}

				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID = PicDiff -
					((MAXIMUM_U32 - PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID) + 1);
			}
			else
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID += PicDiff;
			}
			PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID =
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID;
			PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
		}/* Not a reference frame */
	}/* end if ((PARSER_Data_p->GlobalDecodingContextSpecificData.TfcntrFlag==1) && (PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_SKIP)) */
	else if ((PARSER_Data_p->GlobalDecodingContextSpecificData.TfcntrFlag==1) &&
			 (PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_SKIP))
	{
		PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = FALSE;
	}
	else /* (TfcntrFlag == 0) */
	{

#ifndef VC1_TEST_DISPLAY_ORDER_ID

	/* This is probably something like the way Presentation Order Picture ID
	   will be calculated.  Since the presentation order picture ID is
	   not known for a reference picture at parse-time, the value of
	   the previous presentation order picture ID will be passed back
	   to the producer.  But if there was a reference picture
	   waiting on its presentation order, the CurrentPresentationOrderPictureID
	   is incremented when a new reference picture comes in to account
	   for the waiting reference picture.  CurrentPresentationOrderPictureID
	   also gets incremented by one for every non-reference picture.
	   For the non-reference picture, this value will actually be passed
	   back to the producer.
	   The following illustrates what should happen:

		 PType				  I   P   B   B   B   P   B   P   B   B   P
		 Returned Pres ID	  0   0   2   3   4   4   6   6   8   9   9
		 Stored Pres ID      0   1   2   3   4   5   6   7   8   9   10

		   Note that the producer can store off the previous value of presentation ID
		   and assume that if the current presentation ID matches the previous, then
		   the Presentation ID is not known.  Actually a new flag should be added,
		but at this point, I don't know what the flag would be.   */


		if((RefPic) && (!PARSER_Data_p->StreamState.WaitingRefPic))
		{
			PARSER_Data_p->StreamState.WaitingRefPic = TRUE;
		}
		else
		{
			/* take care of rollover */
			if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID == MAXIMUM_U32)
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID = 0;

				if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension == MAXIMUM_U32)
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension = 0;
				}
				else
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension++;
				}
			}
			else
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID++;
			}

			if(!RefPic)
			{
				PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID =
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID;
			    PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
			}
		} /* end if((RefPic) && (!PARSER_Data_p->StreamState.WaitingRefPic)) -- else */

#else /*  VC1_TEST_DISPLAY_ORDER_ID */


		/* This is a presentation order implementation done only for
		 * initial testing before a interface change is made.
		 * basically, whenever a reference picture comes in,
		 * a constant is added to presentation order ID to
		 * account for the Maximum B frames.
		 *
		 *  BackwardRefPresentationOrderPictureID is the current reference
		 *    picture presentation order ID.
		 *  CurrentPresentationOrderPictureID is the current non-reference picture
		 *    presentation order ID.  This gets incremented by one whenever a
		 *    new non-reference picture comes in.  Whenever a reference picture
		 *    comes in,  CurrentPresentationOrderPictureID gets set to the
		 *    previous reference picture presentation order picture ID.
		 *
		 *  So the presentation order picture ID should look like the following:
		 *   (assuming MAX_B_FRAMES is 20)
		 *
		 *  Type:      I   P   B   B   B    P    B    B    P    B    B    P
		 *  pres ID:   20  40  21  22  23   60   41   42   80   61   62   100
		 *
		 */
		if(RefPic)
		{
			PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID     =
				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID;

			/* take care of rollover */
			if(PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID > (S32)(MAXIMUM_S32 - MAX_B_FRAMES))
			{
				if(PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension == MAXIMUM_U32)
				{
					PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension = 0;
				}
				else
				{
					PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension++;
				}

				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID = (MAX_B_FRAMES - (MAXIMUM_U32 -
					PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID));
			}
			else
			{
				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID += MAX_B_FRAMES;
			}
			PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID =
				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID;
			PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
		}
		else /* not a reference picture */
		{
			/* take care of rollover */
			if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID == (S32)MAXIMUM_S32)
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID = 0;

				if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension == MAXIMUM_U32)
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension = 0;
				}
				else
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension++;
				}
			}
			else
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID++;
			}
			PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID =
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID;
			PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
		}
#endif /* end VC1_TEST_DISPLAY_ORDER_ID */
	} /* end if ((PARSER_Data_p->GlobalDecodingContextSpecificData.TfcntrFlag==1) && (PType != VC1_SKIP_PICTURE_TYPE)) -- else */

	if (PARSER_Data_p->GlobalDecodingContextSpecificData.PulldownFlag == 1)
	{
		if ((PARSER_Data_p->GlobalDecodingContextSpecificData.InterlaceFlag == 0) || (PARSER_Data_p->GlobalDecodingContextSpecificData.ProgressiveSegmentedFrame==1))
		{
			/* RPTFRM  */
			NumFramesToRepeat = vc1par_GetUnsigned(VC1_RPTFRM_FIELD_SIZE, ParserHandle);
			FramePicLayerSize += VC1_RPTFRM_FIELD_SIZE;
		}
		else
		{
			TopFieldFirst	 = vc1par_GetUnsigned(VC1_TFF_FIELD_SIZE, ParserHandle);
			RepeatFirstField = vc1par_GetUnsigned(VC1_RFF_FIELD_SIZE, ParserHandle);
			FramePicLayerSize += (VC1_TFF_FIELD_SIZE + VC1_RFF_FIELD_SIZE);
		}
	}/* end if (PARSER_Data_p->GlobalDecodingContextSpecificData.PulldownFlag==1) */
	else/* PARSER_Data_p->GlobalDecodingContextSpecificData.PulldownFlag != 1 */
	{
		/* default is top field first */
		TopFieldFirst      = 1;
	}

	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TopFieldFirst;
	PARSER_Data_p->PictureSpecificData.TffFlag = TopFieldFirst;

	/* determine whether this is a top or a bottom field. */
	if(Fcm == VC1_FIELD_INTERLACE)
	{
		if(TopFieldFirst == 1)
		{
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_TOP_FIELD;
		}
		else
		{
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_BOTTOM_FIELD;
		}
	}

	PARSER_Data_p->PictureGenericData.RepeatFirstField = (RepeatFirstField == 1) ? TRUE : FALSE;
	PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = NumFramesToRepeat;


	if (PARSER_Data_p->PictureDecodingContextSpecificData.PanScanFlag == 1)
	{
		/* PSPRESENT */
		PSPresent = vc1par_GetUnsigned(VC1_PS_PRESENT_FIELD_SIZE, ParserHandle);
		FramePicLayerSize += VC1_PS_PRESENT_FIELD_SIZE;

		/* logic taken from section 8.9.1 of the VC1 spec
		 * except I am assuming that if RFF and RPTFRM are not
		 * defined, they are set to zero.
		 */
		if ((PARSER_Data_p->GlobalDecodingContextSpecificData.InterlaceFlag == 1) ||
			(PARSER_Data_p->GlobalDecodingContextSpecificData.ProgressiveSegmentedFrame == 0))
		{
			NumPanScanWindows = 2 + RepeatFirstField;
		}
		else
		{
			NumPanScanWindows = 1 + NumFramesToRepeat;
		}

		if(PSPresent==1)
		{
			for(i = 0; i<NumPanScanWindows; i++)
			{
				/* PSHOFFSET */
				PSHoffset = vc1par_GetUnsigned(VC1_PS_HOFFSET_FIELD_SIZE, ParserHandle);
				/* PSVOFFSET */
				PSVoffset = vc1par_GetUnsigned(VC1_PS_VOFFSET_FIELD_SIZE, ParserHandle);
				/* PSWIDTH */
				PSWidth = vc1par_GetUnsigned(VC1_PS_WIDTH_FIELD_SIZE, ParserHandle);
				/* PSHEIGHT */
				PSHeight = vc1par_GetUnsigned(VC1_PS_HEIGHT_FIELD_SIZE, ParserHandle);

				/* In VC1, the pan and scan Horizontal offset is an offset from the left of the frame to the
				   left border of the pan and scan window; and the pan and scan vertical offset is an offset from
				   the top of the frame to the top border of the pan and scan window.  But the producer expects an
				   offset from the center of the picture with the center being 0, 0.
				   So, first find the relation between the center point of the original display window and the
				   center point of the pan and scan window assuming that the pan scan window were placed in the upper
				   left corner of the picture. Then add the Horizontal offset to the X coordinate and the vertical offset
				   to the Y coordinate to get the center point of the pan and scan window shifted to its desired spot.*/
				PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreHorizontalOffset =
					(PSWidth/2) - (PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width/2) + PSHoffset;
				PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreVerticalOffset =
					(PSHeight/2) - (PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height/2) + PSVoffset;

				PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayHorizontalSize = PSWidth;
				PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayVerticalSize = PSHeight;
				PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation = TRUE;

#ifdef VC1_DEBUG_PARSER
				VC1_PARSER_PRINT(("PSHOFFSET = %d\n",PSHoffset));
				VC1_PARSER_PRINT(("PSVOFFSET = %d\n",PSVoffset));
				VC1_PARSER_PRINT(("PSWIDTH = %d\n",PSWidth));
				VC1_PARSER_PRINT(("PSHEIGHT = %d\n",PSHeight));
#endif /* VC1_DEBUG_PARSER */
				FramePicLayerSize += VC1_PAN_N_SCAN_SIZE;

			}/* end for(i = 0; i<NumPanScanWindows; i++) */

			/* set all unused array elements' HasDisplaySizeRecommendation to FALSE */
			for(i = NumPanScanWindows; i < VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; i++)
			{
				PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation = FALSE;
			}
		}/* end if(PSPresent==1) */
		else
		{

			/* If there are no Pan and Scan windows present, then set HasDisplaySizeRecommendation to FALSE
			for every window (NumPanScanWindows = 0).
			@@@ the assumption is that the producer will be able to determine pan scan window information
			from the previous picture in display order.
			If there are pan and scan windows given for this frame, set all
			unused array elements' HasDisplaySizeRecommendation to FALSE */
			for(i = 0; i < VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; i++)
			{
				PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation = FALSE;
			}
		}/* end if(PSPresent==1) -- else */


		/* Here, the logic is, even if the Pan and Scan info is not present in the stream for this picture,
		   the Pan and Scan info for the previous picture in display order are used. For this case,
		   NumberOfPanAndScan is set to the number of pan and scan windows, but HasDisplaySizeRecommendation
		   is set to FALSE in all of the array elements.
		   @@@ Is this the correct way to communicate this case to the Producer?  */
		PARSER_Data_p->PictureGenericData.NumberOfPanAndScan = NumPanScanWindows;

	}/* end if PARSER_Data_p->PictureDecodingContextSpecificData.PanScanFlag ==1) */

    /* The skipped frame syntax ends here. */
	if(PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_SKIP)
	{
		/* RNDCTRL  */
		PARSER_Data_p->PictureSpecificData.RndCtrl = vc1par_GetUnsigned(VC1_RNDCTRL_FIELD_SIZE, ParserHandle);
		FramePicLayerSize += VC1_RNDCTRL_FIELD_SIZE;
		if(PARSER_Data_p->GlobalDecodingContextSpecificData.InterlaceFlag == 1)
		{
			/* UVSAMP */
			UVSamp= vc1par_GetUnsigned(VC1_UVSAMP_FIELD_SIZE, ParserHandle);
			FramePicLayerSize += VC1_UVSAMP_FIELD_SIZE;

			if(UVSamp == 1)
			{
				PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = STGXOBJ_PROGRESSIVE_SCAN;
			}
			else
			{
				PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;
			}

		}/* end if(PARSER_Data_p->GlobalDecodingContextSpecificData.InterlaceFlag == 1) */

		/* Reference, non-B/BI picture.*/
		if((PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_B) &&
		  (PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_BI))
		{
			PARSER_Data_p->StreamState.ScanTypePreviousRefPicture =
				PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType;
			PARSER_Data_p->StreamState.RndCtrlRecentRefPicture =
				PARSER_Data_p->PictureSpecificData.RndCtrl;

			/* Assume here that for field-pairs, this information will stay the same and will not
			   need to be changed for the second field.*/
			PARSER_Data_p->StreamState.ForwardReferencePictureSyntax  = PARSER_Data_p->StreamState.BackwardReferencePictureSyntax;
			PARSER_Data_p->StreamState.BackwardReferencePictureSyntax = PARSER_Data_p->PictureSpecificData.PictureSyntax;

			/* For reference pictures there is not Backward Reference Picture Syntax, so always set don't care values to zero. */
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax = 0;

			if(PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_I)
			{
				PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax = 0;
			}
			else
			{
				PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax = PARSER_Data_p->StreamState.ForwardReferencePictureSyntax;
			}
		}/* if(PARSER_Data_p->PictureGenericData.IsReference == TRUE) */
		else if(PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_B)/* non-reference B picture */
		{
			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax  = PARSER_Data_p->StreamState.ForwardReferencePictureSyntax;
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax = PARSER_Data_p->StreamState.BackwardReferencePictureSyntax;
		}
		else /* non-reference BI picture -- since it has no reference pictures, set the fwd and bkwd ref pict syntax to zero. */
		{
			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax  = 0;
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax = 0;
		}/* end if((PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_B) &&  -- else */

		/* only I/I, I/P, P/I, and P/P can have a refdist element */
		if((Fcm == VC1_FIELD_INTERLACE) &&
		   (PARSER_Data_p->PictureDecodingContextSpecificData.RefDistFlag == 1) &&
		   (PType <= VC1_P_P_FIELD_PICTURE_TYPE ))
		{
		    /* For refdist, read in the first two bits, if they are not equal to 11b (3),
			   then you have your refdist value, otherwise, keep reading in one bit at a
			   time until a zero bit is reached. Once the zero bits have been reached, the
			   total number of bits read in (including the first 2 bits) is the refdist
			   value.  */
			Temp = vc1par_GetUnsigned(2, ParserHandle);

			FramePicLayerSize += 2;

			if(Temp != 3)
			{
				PARSER_Data_p->StreamState.RefDistPreviousRefPic = Temp;
			}
			else
			{
				i = 0;

				do
				{
					Temp = vc1par_GetUnsigned(1, ParserHandle);
					i++;
				}while((Temp == 1) && (i < 14));/* The value of refdist can not go over 14 + 2*/

				PARSER_Data_p->StreamState.RefDistPreviousRefPic = 2 + i;
				/* "i" more bits have been read in for refdist */
				FramePicLayerSize += i;

				if(Temp == 1)
				{
					STTBX_Print(("ERROR: Undefined Ref Dist value (greater than 16 -- see 9.1.1.43)\nerror correction, setting Refdist to 16\n"));
					PARSER_Data_p->PictureSpecificData.PictureError.RefDistOutOfRange = TRUE;
				}
			}/* end if(Temp != 3) */

			PARSER_Data_p->PictureSpecificData.RefDist =
							PARSER_Data_p->StreamState.RefDistPreviousRefPic;

		}/* end if((Fcm == VC1_FIELD_INTERLACE)...*/
		else
		{
			PARSER_Data_p->PictureSpecificData.RefDist = 0;
		}

		if((Fcm == VC1_PROGRESSIVE) && (PARSER_Data_p->GlobalDecodingContextSpecificData.FinterpFlag == 1))
		{
			/* skip INTERPFRM */
			Temp = vc1par_GetUnsigned(1, ParserHandle);
		}

		/* BFRACTION comes later for frame interlace B pictures */
		if(BFractPresent == TRUE)
		{
			/* for frame interlace B pictures, for some reason, these fields
			come before the BFRACTION rather than after the B fraction. */
			if(Fcm == VC1_FRAME_INTERLACE)
			{

				/* skip PQINDEX see 7.1.1.6 (identical for progressive and interlace) */
				Temp = vc1par_GetUnsigned(5, ParserHandle);
				VC1_PARSER_PRINT(("PQINDEX = %d\n",Temp));

				/* skip HALFQP if necessary*/
				if(Temp <= 8)
				{
					Temp = vc1par_GetUnsigned(1, ParserHandle);

					VC1_PARSER_PRINT(("HALFQP = %d\n",Temp));
				}

				/* skip PQUANTIZER if necessary -- see 7.1.1.8 */
				if(PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer == 1)
				{
					Temp = vc1par_GetUnsigned(1, ParserHandle);
					VC1_PARSER_PRINT(("PQUANTIZER = %d\n",Temp));
				}

				/* skip POSTPROC if necessary -- see 7.1.1.27*/
				if(PARSER_Data_p->GlobalDecodingContextSpecificData.PostProcessingFlag == 1)
				{
					Temp = vc1par_GetUnsigned(1, ParserHandle);
					VC1_PARSER_PRINT(("POSTPROC = %d\n",Temp));
				}
			}

			/* read in BFRACTION, the size of BFraction is returned. and added
			   to the Frame Picture Layer Size */
			FramePicLayerSize += vc1par_readBFraction(ParserHandle,
				                                      & (PARSER_Data_p->PictureSpecificData.NumeratorBFraction),
													  & (PARSER_Data_p->PictureSpecificData.DenominatorBFraction),
													  FALSE, (BOOL*)&Temp);

		}/* end if(BFractPresent == TRUE) */
		else
		{
			PARSER_Data_p->PictureSpecificData.NumeratorBFraction   = 0;
			PARSER_Data_p->PictureSpecificData.DenominatorBFraction = 0;
		}

		/* The rest of the fields only need to be parsed
		   for P pictures */
		if(PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_P)
		{
			vc1par_ParseAPTypePPicturePartTwo(Fcm, &IntCompField, &IntComp1, &IntComp2,
											  &IntComp1Mod, &IntComp2Mod, FALSE, ParserHandle);

		}

	}/* end if(PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_SKIP) */

	/* We only care about FramePictureLayerSize for field pictures.*/
	if(Fcm == VC1_FIELD_INTERLACE)
	{
		PARSER_Data_p->PictureSpecificData.FramePictureLayerSize = FramePicLayerSize;
	}
	else
	{
		PARSER_Data_p->PictureSpecificData.FramePictureLayerSize = 0;
	}


	/* Clear out previous Intensity Compensation values */
	vc1par_InitIntComp(ParserHandle);

	/* Determine the reference picture lists (P0, B0, B1)*/
	/* Forward Reference frame/field (for B fields or frames) */
	RefPicIndex0 = PARSER_Data_p->StreamState.NextReferencePictureIndex;
	/* Backward Reference frame (for B fields or frames) and Reference frame (for P pictures) */
	RefPicIndex1 = PARSER_Data_p->StreamState.NextReferencePictureIndex ^ 1;

	switch(PARSER_Data_p->PictureSpecificData.PictureType)
	{
		case VC9_PICTURE_TYPE_I:
			vc1par_HandleAPPictureTypeI(ParserHandle, RefPicIndex0);
			break;

		case VC9_PICTURE_TYPE_P:
			vc1par_HandleAPPictureTypeP(ParserHandle, RefPicIndex1, RefPicIndex0,
				IntCompField, IntComp1, IntComp2, IntComp1Mod, IntComp2Mod, Fcm);
			break;

		case VC9_PICTURE_TYPE_SKIP:
			vc1par_HandleAPPictureTypeSkip(ParserHandle, RefPicIndex1, RefPicIndex0);
			break;

		case VC9_PICTURE_TYPE_B:
			vc1par_HandleAPPictureTypeB(ParserHandle, RefPicIndex1, RefPicIndex0);
			break;

		case VC9_PICTURE_TYPE_BI:
			vc1par_HandleAPPictureTypeBI(ParserHandle);
			break;

		default:
			break;
	}/* end switch(PARSER_Data_p->PictureSpecificData.PictureType) */


#ifdef VC1_DEBUG_PARSER
	if (PARSER_Data_p->PictureDecodingContextSpecificData.PanScanFlag == 1)
	{
		if(PSPresent ==1)
		{
			VC1_PARSER_PRINT(("Pan and Scan Window Info Exists\n"));
			VC1_PARSER_PRINT(("Num Pan and Scan Windows = %d\n", NumPanScanWindows));
		}
		else
		{
			VC1_PARSER_PRINT(("No Pan and Scan Information for this picture, use previous pan scan info\n"));
		}
	}
	else
	{
		VC1_PARSER_PRINT(("Pan/Scan Not used\n"));
	}

	VC1_PARSER_PRINT(("repeat frame %d times\n",NumFramesToRepeat));
	VC1_PARSER_PRINT(("%s\n", (TopFieldFirst==1) ? "Top Field First":"Bottom Field First"));
	VC1_PARSER_PRINT(("%s\n", (RepeatFirstField==1) ?"First Field can be repeated":"First Field Repetition Not Necessary"));
	VC1_PARSER_PRINT(("FCM is %s (%d)\n",FcmDescription[Fcm], Fcm));
	VC1_PARSER_PRINT(("Rounding done using RND =%d\n",PARSER_Data_p->PictureSpecificData.RndCtrl));
	VC1_PARSER_PRINT(("%s\n", (UVSamp == 1) ? "Progressive subsampling of the color-difference is used":
										 "Interlace   subsampling of the color-difference is used"));
	VC1_PARSER_PRINT(("REFDIST = %d\n", PARSER_Data_p->StreamState.RefDistPreviousRefPic));
#endif /* VC1_DEBUG_PARSER*/

	PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;

}/* end vc1par_ParseAPPicture() */


/*******************************************************************************/
/*! \Reads in the BFRACTION field and sets NumeratorBFraction
*      and DenominatorBFraction accordingly.
*
* Picture info is updated
*
* \param NumeratorBFraction - The address of the BFraction Numerator
*        DenominatorBFraction - The address of the BFraction Denominator
*        MainProfileFlag - TRUE if this is a Main profile picture, FALSE otherwise
*        BIPictureFlag   - address of a flag which is set to TRUE if MainProfileFlag
*                          and the BFRACTION indicates that the picture
*                          is a BI picture.
*
* \return the number of bits read for BFRACTION
*/
/******************************************************************************/
U32 vc1par_readBFraction(const PARSER_Handle_t ParserHandle,
						 U32 *NumeratorBFraction,
						  U32 *DenominatorBFraction,
						  BOOL MainProfileFlag,
						  BOOL *BIPictureFlag)
{
	U32 Temp;
	U32 BFractSize = 3;
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);



	*BIPictureFlag = FALSE;


	/* We need to read in BFRACTION here.
	 *  First, read in 3 bits.  if the first 3 bits are 111b (7),
	 * then there are another 4  bits to read in. See 7.1.1.14  B Picture Fraction
	 */
	Temp = vc1par_GetUnsigned(3, ParserHandle);

	switch (Temp)
	{
		case 0:
			/*  1/2 */
			*NumeratorBFraction   = 1;
			*DenominatorBFraction	= 2;
			break;
		case 1:
			/*  1/3 */
			*NumeratorBFraction   = 1;
			*DenominatorBFraction	= 3;
			break;
		case 2:
			/*  2/3 */
			*NumeratorBFraction   = 2;
			*DenominatorBFraction	= 3;
			break;
		case 3:
			/*  1/4 */
			*NumeratorBFraction   = 1;
			*DenominatorBFraction	= 4;
			break;
		case 4:
			/*  3/4 */
			*NumeratorBFraction   = 3;
			*DenominatorBFraction	= 4;
			break;
		case 5:
			/*  1/5 */
			*NumeratorBFraction   = 1;
			*DenominatorBFraction	= 5;
			break;
		case 6:
			/*  2/5 */
			*NumeratorBFraction   = 2;
			*DenominatorBFraction	= 5;
			break;
		case 7:
			Temp = vc1par_GetUnsigned(4, ParserHandle);
			BFractSize += 4;
			switch(Temp)
			{
				case 0:
					/*  3/5 */
					*NumeratorBFraction   = 3;
					*DenominatorBFraction	= 5;
					break;
				case 1:
					/*  4/5 */
					*NumeratorBFraction   = 4;
					*DenominatorBFraction	= 5;
					break;
				case 2:
					/*  1/6 */
					*NumeratorBFraction   = 1;
					*DenominatorBFraction	= 6;
					break;
				case 3:
					/*  5/6 */
					*NumeratorBFraction   = 5;
					*DenominatorBFraction	= 6;
					break;
				case 4:
					/*  1/7 */
					*NumeratorBFraction   = 1;
					*DenominatorBFraction	= 7;
					break;
				case 5:
					/*  2/7 */
					*NumeratorBFraction   = 2;
					*DenominatorBFraction	= 7;
					break;
				case 6:
					/*  3/7 */
					*NumeratorBFraction   = 3;
					*DenominatorBFraction	= 7;
					break;
				case 7:
					/*  4/7 */
					*NumeratorBFraction   = 4;
					*DenominatorBFraction	= 7;
					break;
				case 8:
					/*  5/7 */
					*NumeratorBFraction   = 5;
					*DenominatorBFraction	= 7;
					break;
				case 9:
					/*  6/7 */
					*NumeratorBFraction   = 6;
					*DenominatorBFraction	= 7;
					break;
				case 10:
					/*  1/8 */
					*NumeratorBFraction   = 1;
					*DenominatorBFraction	= 8;
					break;
				case 11:
					/*  3/8 */
					*NumeratorBFraction   = 3;
					*DenominatorBFraction	= 8;
					break;
				case 12:
					/*  5/8 */
					*NumeratorBFraction   = 5;
					*DenominatorBFraction	= 8;
					break;
				case 13:
					/*  7/8 */
					*NumeratorBFraction   = 7;
					*DenominatorBFraction	= 8;
					break;
				case 14:
					/*  7/8 */
					*NumeratorBFraction   = 7;
					*DenominatorBFraction = 8;
					/* STTBX_Print(("ERROR: Reseved BFRACTION value 0x7%x\nerror correction, setting BFRACTION to 7/8\n", Temp)); */
					PARSER_Data_p->PictureSpecificData.PictureError.BFractionOutOfRange = TRUE;

					break;
				default:
					/*  7/8 */

					if(MainProfileFlag == TRUE)
					{
						*NumeratorBFraction   = 0;
						*DenominatorBFraction = 0;
						*BIPictureFlag = TRUE;
					}
					else
					{
						*NumeratorBFraction   = 7;
						*DenominatorBFraction = 8;
						/* STTBX_Print(("ERROR: Reseved BFRACTION value 0x7%x\nerror correction, setting BFRACTION to 7/8\n", Temp)); */
						PARSER_Data_p->PictureSpecificData.PictureError.BFractionOutOfRange = TRUE;
					}
					break;
			}/* end of switch(Temp) -- second occurance */
			break;/* end of case 7 */
		default:
			break;
	}/* end of switch(Temp) */

	return(BFractSize);

}/* end void vc1par_readBFraction()  */

/*******************************************************************************/
/*! \Does processing related to obtaining an picture of type I.
*  This assumes that the frame  header has already been parsed.
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
*        PrevRefPicIndex - The index in the full reference picture list
*                          to the most recent previous reference picture
*        OldestRefPicIndex - The index in the full reference picture list
*                          to the oldest reference picture
*
* \return void
*/
/******************************************************************************/
void vc1par_HandleAPPictureTypeI(const PARSER_Handle_t ParserHandle, U32 OldestRefPicIndex)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
/*	PARSER_Data_p->StreamState.RefDistPreviousRefPic = 0; */
	PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
	PARSER_Data_p->PictureSpecificData.BackwardRefDist = 0;
	/* The oldest reference frame is no longer used because this current frame will
	   take its place. */
	if(PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].IntCompAvailable == TRUE)
	{
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Top_1 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Top_2 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Bot_1 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Bot_2 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].IntCompAvailable = FALSE;
	}
}/* end void vc1par_HandleAPPictureTypeI() */


/*******************************************************************************/
/*! \Does processing related to obtaining an picture of type P.  This assumes that the frame
*    header has already been parsed.
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
*        PrevRefPicIndex - The index in the full reference picture list
*                          to the most recent previous reference picture
*        OldestRefPicIndex - The index in the full reference picture list
*                          to the oldest reference picture
*
* \return void
*/
/******************************************************************************/
void vc1par_HandleAPPictureTypeP(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 OldestRefPicIndex,
								 U8 IntCompField, U32 IntComp1, U32 IntComp2, U32 IntComp1Mod, U32 IntComp2Mod,U32 Fcm)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;

	/* First check to make sure that the reference frame is available. */
	if(PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[PrevRefPicIndex] == FALSE)
	{
		STTBX_Print(("ERROR: Not all references available for P picture %u\n",
			PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));
		PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
	}/* end if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[PrevRefPicIndex] == FALSE) ...*/
	else
	{
		PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
		PARSER_Data_p->PictureGenericData.ReferenceListP0[0] =
			PARSER_Data_p->PictureGenericData.FullReferenceFrameList[PrevRefPicIndex];

			/* Refering to both fields of a reference field-pair or just to a frame
			@@@Should a frame which is referring to a field pair have 2 references in
		its P0 list or just one reference? For now assume two */
		if((Fcm != VC1_FIELD_INTERLACE) || (PARSER_Data_p->PictureLocalData.NumRef == 1))
		{
			if(PARSER_Data_p->StreamState.FieldPairFlag[PrevRefPicIndex] == TRUE)
			{
				PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[0]	= PARSER_Data_p->PictureSpecificData.TffFlag;
				PARSER_Data_p->PictureGenericData.ReferenceListP0[1] =
					PARSER_Data_p->PictureGenericData.FullReferenceFrameList[PrevRefPicIndex];
				PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[1]	= !PARSER_Data_p->PictureSpecificData.TffFlag;
				PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 2;
			}
			else
			{
				PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[0]	= TRUE;
				PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 1;
			}

		}/* end if(PARSER_Data_p->PictureLocalData.NumRef == 1) */
		/* Refering to one of two fields of a reference field-pair*/
		else
		{
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 1;

			if(PARSER_Data_p->PictureLocalData.RefField == 1)
			{
				PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[0]	= PARSER_Data_p->PictureSpecificData.TffFlag;
			}
			else
			{
				PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[0]	= !PARSER_Data_p->PictureSpecificData.TffFlag;
			}

		}/* end if(PARSER_Data_p->PictureLocalData.NumRef == 1) -- else */
	}/* end if(PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[OldestRefPicIndex] == FALSE) -- else */

	PARSER_Data_p->PictureSpecificData.BackwardRefDist = 0;


	/* Handle Intensity Compensation For P Top Fields or Frames Here */

	/* The oldest reference frame is no longer used because this current frame will
	   take its place. */
	if(PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].IntCompAvailable == TRUE)
	{
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Top_1 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Top_2 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Bot_1 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Bot_2 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].IntCompAvailable = FALSE;
	}

	switch(IntCompField)
	{
		case 0:/* Intensity Compensation available for top and bottom reference fields */

			   /* If there is not already intensity compensation available for
			   the top field of the reference field-pair, set the first
			   intensity compensation information to the extracted values, otherwise
			set the second intensity comp. info. to the extracted values.*/
			if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1 == VC9_NO_INTENSITY_COMP)
			{
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1 = IntComp1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   = IntComp1Mod;
			}
			else
			{
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_2 = IntComp1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   =
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_2   = IntComp1Mod;
			}

			/* If there is not already intensity compensation available for
			the bottom field of the reference field-pair, set the first
			intensity compensation information to the extracted values, otherwise
			set the second intensity comp. info. to the extracted values.*/
			if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1 == VC9_NO_INTENSITY_COMP)
			{
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1 = IntComp2;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   = IntComp2Mod;
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].IntCompAvailable = TRUE;
			}
			else
			{
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_2 = IntComp2;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   =
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_2   = IntComp2Mod;
			}

			PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].IntCompAvailable = TRUE;
			PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
			break;

		case 1:/* Intensity Compensation available for just the bottom reference fields */

			   /* If there is not already intensity compensation available for
			   the bottom field of the reference field-pair, set the first
			   intensity compensation information to the extracted values, otherwise
			set the second intensity comp. info. to the extracted values.*/
			if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1 == VC9_NO_INTENSITY_COMP)
			{
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1 = IntComp1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   = IntComp1Mod;
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].IntCompAvailable = TRUE;
			}
			else
			{
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_2 = IntComp1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   =
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_2   = IntComp1Mod;
			}

			PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;

			/* Add by TD for field pb when top field P has bot intensity but no top intensity, should take reference top intensity */
			if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1 != VC9_NO_INTENSITY_COMP)
			{
				PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   =
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1;
			}

			break;

		case 2:/* Intensity Compensation available for just the top reference fields
			or for the reference frame */

			/* If there is not already intensity compensation available for
			the top field of the reference field-pair or for the reference frame, set the first
			intensity compensation information to the extracted values, otherwise
			set the second intensity comp. info. to the extracted values.*/
			if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1 == VC9_NO_INTENSITY_COMP)
			{
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1 = IntComp1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   = IntComp1Mod;
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].IntCompAvailable = TRUE;
			}
			else
			{
				PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_2 = IntComp1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   =
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_2   = IntComp1Mod;
			}

			/* Add by TD for field pb when bot field P has top intensity but no bot intensity, should take reference bot intensity */
			if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1 != VC9_NO_INTENSITY_COMP)
			{
				PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   =
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1;
			}

			if(Fcm != VC1_FIELD_INTERLACE)
			{

				/* For frames, the top and bottom field are set to the same value.*/
				if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1 == VC9_NO_INTENSITY_COMP)
				{
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1 = IntComp1;
					PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   = IntComp1Mod;
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].IntCompAvailable = TRUE;
				}
				else
				{
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_2 = IntComp1;
					PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   =
						PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1;
					PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_2   = IntComp1Mod;
				}

			}

			PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;

			break;
		case 3:/* No Intensity Compensation available with this field/frame */

			   /* If there was Intensity Compensation already stored for this
			   field-pair/frame, there could be only one set of numbers (there
			   could not be second pass), so copy over whatever values are in
			Top_1 and Bot_1.  */
			if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].IntCompAvailable)
			{
				PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   =
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1;
				PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   =
					PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1;
				PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
			}
			break;
		default:
			break;
	}/* end switch(IntCompField) */

}/* end void vc1par_HandleAPPictureTypeP() */


/*******************************************************************************/
/*! \Does processing related to obtaining an picture of type B.  This assumes that the frame
*    header has already been parsed.
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
*        PrevRefPicIndex - The index in the full reference picture list
*                          to the most recent previous reference picture
*        OldestRefPicIndex - The index in the full reference picture list
*                          to the oldest reference picture
*
* \return void
*/
/******************************************************************************/
void vc1par_HandleAPPictureTypeB(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 OldestRefPicIndex)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
	if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[OldestRefPicIndex] == FALSE) ||
	   (PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[PrevRefPicIndex] == FALSE))
	{
		STTBX_Print(("ERROR: Not all references available for B Picture %u\n",
			PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));
		PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
	}/* end if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[OldestRefPicIndex] == FALSE) ...*/
	else
	{
		PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;

		PARSER_Data_p->PictureGenericData.ReferenceListB0[0] =
			PARSER_Data_p->PictureGenericData.FullReferenceFrameList[OldestRefPicIndex];

		PARSER_Data_p->PictureGenericData.ReferenceListB1[0] =
			PARSER_Data_p->PictureGenericData.FullReferenceFrameList[PrevRefPicIndex];

		/* If the frame referred to is actually a field pair, then include another entry
		in B0 to account for the second field.  But  this second field will always
		have the Decoding order frame ID of the first entry in the list.*/
		if(PARSER_Data_p->StreamState.FieldPairFlag[OldestRefPicIndex] == TRUE)
		{
			PARSER_Data_p->PictureGenericData.ReferenceListB0[1] =
				PARSER_Data_p->PictureGenericData.ReferenceListB0[0];
			PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[1] = !PARSER_Data_p->PictureSpecificData.TffFlag;
			PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[0]	= PARSER_Data_p->PictureSpecificData.TffFlag;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 2;
		}
		else/* This is the case where we are referring to a frame rather than a field.*/
		{
			PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[0]	= TRUE;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 1;
		}

		/* If the frame referred to is actually a field pair, then include another entry
		in B1 to account for the second field.  But  this second field will always
		have the Decoding order frame ID of the first entry in the list.*/
		if(PARSER_Data_p->StreamState.FieldPairFlag[PrevRefPicIndex] == TRUE)
		{
			PARSER_Data_p->PictureGenericData.ReferenceListB1[1] =
				PARSER_Data_p->PictureGenericData.ReferenceListB1[0];
			PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[1] = !PARSER_Data_p->PictureSpecificData.TffFlag;
			PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[0]	= PARSER_Data_p->PictureSpecificData.TffFlag;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 2;
		}
		else/* This is the case where we are referring to a frame rather than a field.*/
		{
			PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[0]	= TRUE;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 1;
		}

	}/* end if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[OldestRefPicIndex] == FALSE) ... -- else*/

	PARSER_Data_p->PictureSpecificData.BackwardRefDist = PARSER_Data_p->StreamState.RefDistPreviousRefPic;

	/* Copy Intensity Compensation Info from the Forward reference frame if available */
	if(PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].IntCompAvailable == TRUE)
	{
		PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1 = PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Top_1;
		PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_2 = PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Top_2;
		PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1 = PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Bot_1;
		PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_2 = PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Bot_2;
		PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;

	}/* end if(PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].IntCompAvailable == TRUE) */

	/* Copy Intensity Compensation Info from the Backward reference frame if available*/
	if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].IntCompAvailable == TRUE)
	{
		PARSER_Data_p->PictureSpecificData.IntComp.BackTop_1 = PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1;
		PARSER_Data_p->PictureSpecificData.IntComp.BackTop_2 = PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_2;
		PARSER_Data_p->PictureSpecificData.IntComp.BackBot_1 = PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1;
		PARSER_Data_p->PictureSpecificData.IntComp.BackBot_2 = PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_2;
		PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
	}/* end if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].IntCompAvailable == TRUE) */

}/* end void vc1par_HandleAPPictureTypeB() */


/*******************************************************************************/
/*! \Does processing related to obtaining an picture of type BI.  This assumes that the frame
*    header has already been parsed.
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
*        PrevRefPicIndex - The index in the full reference picture list
*                          to the most recent previous reference picture
*        OldestRefPicIndex - The index in the full reference picture list
*                          to the oldest reference picture
*
* \return void
*/
/******************************************************************************/
void vc1par_HandleAPPictureTypeBI(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
	PARSER_Data_p->AllReferencesAvailableForFrame						 = TRUE;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1		     = 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0			 = 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0			 = 0;
	PARSER_Data_p->PictureSpecificData.BackwardRefDist				     = 0;
}/* end void vc1par_HandleAPPictureTypeBI() */


/*******************************************************************************/
/*! \Does processing related to obtaining an picture of type Skip.  This assumes that the frame
*    header has already been parsed.
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
*        PrevRefPicIndex - The index in the full reference picture list
*                          to the most recent previous reference picture
*        OldestRefPicIndex - The index in the full reference picture list
*                          to the oldest reference picture
*
* \return void
*/
/******************************************************************************/
void vc1par_HandleAPPictureTypeSkip(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 OldestRefPicIndex)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;

	/* use the scan type of the previous reference picture */
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType =
					PARSER_Data_p->StreamState.ScanTypePreviousRefPicture;

	PARSER_Data_p->PictureSpecificData.RndCtrl =
					PARSER_Data_p->StreamState.RndCtrlRecentRefPicture;

	/* Use the RefDist of the previous reference picture.
	   Should this be set to zero instead because this is a frame
	   and Ref dist only occurs for a field-pair.  @@@ Is it possible
	   for a skip frame to actually duplicate a field pair?  */
	PARSER_Data_p->PictureSpecificData.RefDist = PARSER_Data_p->StreamState.RefDistPreviousRefPic;

	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;


	PARSER_Data_p->StreamState.ForwardReferencePictureSyntax  = PARSER_Data_p->StreamState.BackwardReferencePictureSyntax;
	PARSER_Data_p->StreamState.BackwardReferencePictureSyntax = PARSER_Data_p->PictureSpecificData.PictureSyntax;

	PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax  = PARSER_Data_p->StreamState.ForwardReferencePictureSyntax;
	PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax = 0;

	/* First check to make sure that the reference frame is available. */
	if(PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[PrevRefPicIndex] == FALSE)
	{
		STTBX_Print(("ERROR: Not all references available for SKIP picture %u\n",
			PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));
		PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
	}/* end if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[PrevRefPicIndex] == FALSE) ...*/
	else
	{
		PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
		PARSER_Data_p->PictureGenericData.ReferenceListP0[0] =
			PARSER_Data_p->PictureGenericData.FullReferenceFrameList[PrevRefPicIndex];

		if(PARSER_Data_p->StreamState.FieldPairFlag[PrevRefPicIndex] == TRUE)
		{
			PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[0]	= PARSER_Data_p->PictureSpecificData.TffFlag;
			PARSER_Data_p->PictureGenericData.ReferenceListP0[1] =
				PARSER_Data_p->PictureGenericData.FullReferenceFrameList[PrevRefPicIndex];
			PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[1]	= !PARSER_Data_p->PictureSpecificData.TffFlag;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 2;
		}
		else
		{
			PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[0]	= TRUE;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 1;
		}

	}/* end if(PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[OldestRefPicIndex] == FALSE) -- else */

	PARSER_Data_p->PictureSpecificData.BackwardRefDist = 0;

	/* The oldest reference frame is no longer used because this current frame will
	   take its place. */
	if(PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].IntCompAvailable == TRUE)
	{
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Top_1 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Top_2 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Bot_1 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].Bot_2 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntComp[OldestRefPicIndex].IntCompAvailable = FALSE;
	}


	/* If there was Intensity Compensation already stored for this
	   field-pair/frame, there could be only one set of numbers (there
	   could not be second pass), so copy over whatever values are in
	   Top_1 and Bot_1.  */
	if(PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].IntCompAvailable)
	{
		PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   =
			PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Top_1;
		PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   =
			PARSER_Data_p->StreamState.IntComp[PrevRefPicIndex].Bot_1;
		PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
	}
}/* end void vc1par_HandleAPPictureTypeSkip() */

/*******************************************************************************/
/*! \Parses a Type P field header or
* the second half of a Type P frame picture starting with PQINDEX
* this gets the intensity compensation information and,
* for fields, NUMREF and REFFIELD
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
* \      Fcm - Frame coding mode, as defined in FrameCodingMode_t
* \      IntCompField - 0:  Intensity Compensation information available for both fields (IntComp1 = top, IntComp2 = bottom),
* \                     1:  Intensity Compensation information available for just the bottom field (IntComp1),
* \                     2:  Intensity Compensation information available for just the top field (IntComp1),
* \                     3:  No Intensity Compensation information available.
* \return void
*/
/******************************************************************************/
void vc1par_ParseAPTypePPicturePartTwo(U32 Fcm,
									   U8* IntCompField,
									   U32* IntComp1,
									   U32* IntComp2,
									   U32* IntComp1Mod,
									   U32* IntComp2Mod,
									   BOOL BottomFieldFlag,
									   const PARSER_Handle_t ParserHandle)
{
		/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	U32 Temp;
	BOOL IntensityCompensationPresent = FALSE;

	U8 NumDigits = 0; /* the number of digits of MVMODE read in. */

	/* U32 LumShiftLumScale = VC9_NO_INTENSITY_COMP; */
	/* U32 LumShiftLumScale2 = VC9_NO_INTENSITY_COMP; */

	/* For MVRANGE and DMVRANGE VLC interpretation. */
	/* valid values are 0, 10, 110, 111. */
	U32 MVandDMVRangeTable[] = {0,2,0,4,0,0};

	/* For MVMODE VLC interpretation. */
	/* valid values are 1, 01, 001, 0000, 0001 */
  /*  U32 MVModeTbable[] = {2,0,4,0,6,0,0,0};*/

	/* For MVMODE2 VLC interpretation. */
	/* valid values are 1, 01, 001, 000 */
	U32 MVMode2Table[] = {2,0,4,0,0,0};

	*IntCompField = 2; /* Default to Top field only so that, for frames, only one set of
	                     Intensity Compensation variables are read in.
	                     Also, If we defaulted to 3, then it would communicate that
	                     Intensity Compensation was not present for a frame.*/

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* skip PQINDEX see 7.1.1.6 (identical for progressive and interlace) */
	Temp = vc1par_GetUnsigned(5, ParserHandle);
	VC1_PARSER_PRINT(("PQINDEX = %d\n",Temp));

	/* skip HALFQP if necessary*/
	if(Temp <= 8)
	{
		Temp = vc1par_GetUnsigned(1, ParserHandle);

		VC1_PARSER_PRINT(("HALFQP = %d\n",Temp));
	}

	/* skip PQUANTIZER if necessary -- see 7.1.1.8 */
	if(PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer == 1)
	{
		Temp = vc1par_GetUnsigned(1, ParserHandle);
		VC1_PARSER_PRINT(("PQUANTIZER = %d\n",Temp));
	}

	/* skip POSTPROC if necessary -- see 7.1.1.27*/
	if(PARSER_Data_p->GlobalDecodingContextSpecificData.PostProcessingFlag == 1)
	{
		Temp = vc1par_GetUnsigned(2, ParserHandle);
		VC1_PARSER_PRINT(("POSTPROC = %d\n",Temp));
	}

	/* Read in NUMREF and REFFIELD for field interlace P frames only
	   see 9.1.1.44, 9.1.1.45*/
	if(Fcm == VC1_FIELD_INTERLACE)
	{
		PARSER_Data_p->PictureLocalData.NumRef = vc1par_GetUnsigned(1, ParserHandle);
		VC1_PARSER_PRINT(("NUMREF = %d\n",PARSER_Data_p->PictureLocalData.NumRef));

		if(PARSER_Data_p->PictureLocalData.NumRef == 0)
		{
			PARSER_Data_p->PictureLocalData.RefField = vc1par_GetUnsigned(1, ParserHandle);
			VC1_PARSER_PRINT(("REFFIELD = %d\n",PARSER_Data_p->PictureLocalData.RefField));
		}
	}

	/* skip MVRANGE if necessary -- see 7.1.1.9 and 9.1.1.26*/
	if(PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag == 1)
	{
		/* MVRANGE has a maximum of 3 bits. */
		Temp = vc1par_GetVariableLengthCode(MVandDMVRangeTable, 3, ParserHandle);
		VC1_PARSER_PRINT(("MVRANGE = %d\n",Temp));
	}

	/* skip DMVRANGE if necessary -- see 9.1.1.27, not there for
	   progressive frames */
	if((Fcm != VC1_PROGRESSIVE) &&
	   (PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedDMVFlag == 1))
	{
		/* DMVRANGE has a maximum of 3 bits. */
		Temp = vc1par_GetVariableLengthCode(MVandDMVRangeTable, 3, ParserHandle);
		VC1_PARSER_PRINT(("DMVRANGE = %d\n",Temp));
	}

	if(Fcm != VC1_FRAME_INTERLACE)
	{
		/* Read in MVMODE VLC, valid values are 1, 01, 001, 0000, 0001
		   Note that vc1par_GetVariableLengthCode cannot be used here because
		   it will return the value of 1 for 4 out of the 5 values,
		   so the solution is to read a bit until a one is received or
		   until 4 bits have been read.  The number of bits (NumDigits) read in
		   will determine the value of the VLC as follows:
			NumDigits   VLC value
			    0           1
				1           01
				2           001
				3           0001 (specifying Intensity compensation, the only value we care about)
				4           0000

		*/
		while((NumDigits < 4) && ((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 0))
		{
			NumDigits++;
		}

		if(NumDigits == 3)
		{
			IntensityCompensationPresent = TRUE;
		}

		if(IntensityCompensationPresent)
		{
			/* since we don't care about the value of MVMODE2, we can use
			 * vc1par_GetVariableLengthCode
			 */
			Temp = vc1par_GetVariableLengthCode(MVMode2Table, 3, ParserHandle);

			/* read in INTCOMPFIELD. valid values are 1, 00, 01 */
			if(Fcm == VC1_FIELD_INTERLACE)
			{
				*IntCompField = 0;
				while((*IntCompField < 2) && ((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 0))
				{
					(*IntCompField)++;
				}

				/* with this method, IntCompField means the following: */
				/*  0 -> 1 (both fields),  1 -> 01 (bottom field), and  2 ->00 (top field) */
			}
		}
	}
	else
	{
		/* Skip 4MVSWITCH */
		Temp = vc1par_GetUnsigned(1, ParserHandle);

		if((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 1)
		{
			IntensityCompensationPresent = TRUE;
		}
	}

	*IntComp1 = VC9_NO_INTENSITY_COMP;
	*IntComp2 = VC9_NO_INTENSITY_COMP;
	*IntComp1Mod = VC9_NO_INTENSITY_COMP;
	*IntComp2Mod = VC9_NO_INTENSITY_COMP;

	if(IntensityCompensationPresent)
	{
	    /* LUMSCALE */
		*IntComp1 = vc1par_GetUnsigned(6, ParserHandle);

		/* LUMSHIFT */
		*IntComp1 |= (vc1par_GetUnsigned(6, ParserHandle) << 16);

		*IntComp1Mod = *IntComp1;

		VC1_PARSER_PRINT(("Intensity Compensation Present, LumShiftLumScale = %8X\n", *IntComp1));

		/* There are intensity compensation information for 2 fields. */
		if(*IntCompField == 0)
		{
			/* LUMSCALE2 */
			*IntComp2 = vc1par_GetUnsigned(6, ParserHandle);

			/* LUMSHIFT2 */
			*IntComp2 |= (vc1par_GetUnsigned(6, ParserHandle) << 16);
			VC1_PARSER_PRINT(("Intensity Compensation Present (Second/Bottom Field), LumShiftLumScale = %8X\n", *IntComp2));

			*IntComp2Mod = *IntComp2;

		}

		/* Only one of the two reference frames are used */
		if((Fcm == VC1_FIELD_INTERLACE) && (PARSER_Data_p->PictureLocalData.NumRef == 0))
		{
			/* If the current field is a top field and it only refers to the nearest field, then it only refers to the
			   bottom field before it, and not the top field.
			   Similarly, if the current field is a bottom field and it only refers to the second most temporarily closest
			   field, then it also only refers to the bottom field before it and not the top field.  So, If there is intensity
			   compensation information for the top field, then indicate that there is no intensity compensation info for the top field. */
			if(((BottomFieldFlag == FALSE)  && (PARSER_Data_p->PictureLocalData.RefField == 0)) ||
			   ((BottomFieldFlag == TRUE)   && (PARSER_Data_p->PictureLocalData.RefField == 1)))
			{
				/* If IntCompField is one, then IntComp1Mod will refer to the bottom reference field and there is no
				   IntComp2Mod.  For all other cases, IntComp1Mod will refer to the top reference field.*/
				if(*IntCompField != 1)
				{
					*IntComp1Mod = VC9_NO_INTENSITY_COMP;
				}
			}
			/* If the current field is a top field and it only refers to the second most temporarily closest field, then it only refers to the
			   top field before it, and not the bottom field.
			   Similarly, if the current field is a bottom field and it only refers to the temporarily closest
			   field, then it also only refers to the top field before it and not the bottom field.  So, If there is intensity
			   compensation information for the bottom field, then indicate that there is no intensity compensation info for the bottom field. */
			else if(((BottomFieldFlag == FALSE)  && (PARSER_Data_p->PictureLocalData.RefField == 1)) ||
					((BottomFieldFlag == TRUE)   && (PARSER_Data_p->PictureLocalData.RefField == 0)))
			{
				/* If *IntCompField is 0, then IntComp1Mod refers to the top field and IntComp2Mod refers to the bottom field */
				if(*IntCompField == 0)
				{
					*IntComp2Mod = VC9_NO_INTENSITY_COMP;
				}

				/* If IntCompField is one, then IntComp1Mod will refer to the bottom reference field*/
				else if(*IntCompField == 1)
				{
					*IntComp1Mod = VC9_NO_INTENSITY_COMP;
				}
			}
		}/* end if(PARSER_Data_p->PictureLocalData.NumRef == 0)*/
	}
	else
	{
		*IntCompField = 3; /* Invented IntCompField to indicate there is not intensity compensation available. */
	}

}/* end vc1par_ParseAPPicturePartTwo()*/

/*******************************************************************************/
/*! \brief Parses Frame Layer Data Structure
*
*   Assumption: detection of SC Emulation Prevention
*               bytes has already been disabled before this point.
*
* \param ParserHandle Parser Handle (variables managed by the parser)
* \return void
*/
/******************************************************************************/
void vc1par_ParseFrameLayerDataStruct(BOOL *SkipPicture, const PARSER_Handle_t ParserHandle)
{
	U32 Temp, Timediff = 0;
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* KEY -- already read in, so ignored this time */
	Temp = vc1par_GetUnsigned(1, ParserHandle);

	/* RES */
	if((Temp = vc1par_GetUnsigned(7, ParserHandle)) != 0)
	{
		STTBX_Print(("ERROR:  RES = %u,  should be 0\n", Temp));
		PARSER_Data_p->PictureSpecificData.PictureError.RESOutOfRange = TRUE;
	}

	/* FRAMESIZE */
	if((Temp = vc1par_GetUnsigned(24, ParserHandle)) < 2)
	{
		*SkipPicture = TRUE;
	}
	else
	{
		*SkipPicture = FALSE;
	}


	/* TIMESTAMP -- use in place of PTS */
	Temp = vc1par_GetUnsigned(32, ParserHandle);

	if(Temp < (0xFFFFFFFF/VC1_PTS_TICKS_PER_MILLISECOND))
	{
		PARSER_Data_p->PTSForNextPicture.PTS = Temp * VC1_PTS_TICKS_PER_MILLISECOND;
		PARSER_Data_p->PTSForNextPicture.PTS33 = 0;
	}
	else
	{
		/* @@@ assumption here is that TIMESTAMP never rolls over.  That would be a video 49 days long which
		       would be a very very large file.  */
		/* PTS has rolled over, but Timestamp has not. 0xFFFFFFFF/VC1_PTS_TICKS_PER_MILLISECOND is the
		   number of timestamps ticks it takes for the PTS to rollover. Using the modulo gives the number
		   of timestamp ticks after the PTS rollover.  Then multiplying by VC1_PTS_TICKS_PER_MILLISECOND
		   converts the timestamp tick to PTS ticks.  It is done this way to avoid overflowing a 32 bit integer.*/
		PARSER_Data_p->PTSForNextPicture.PTS = (Temp % (0xFFFFFFFF/VC1_PTS_TICKS_PER_MILLISECOND)) * VC1_PTS_TICKS_PER_MILLISECOND;

		/* Determine the number of times the 32-bit PTS has rolled over.  Then modulo 2 gives the PTS33 bit.*/
		PARSER_Data_p->PTSForNextPicture.PTS33 = (Temp / (0xFFFFFFFF/VC1_PTS_TICKS_PER_MILLISECOND)) % 2;
	}

	/* If no framerate has been given, use 2 Timestamps to determine the
	   framerate. */
	if(PARSER_Data_p->StreamState.FrameRatePresent == FALSE)
	{
		/* Only one Timestamp has occurred, so wait for the next timestamp */
		if(PARSER_Data_p->StreamState.PreviousTimestamp == 0xFFFFFFFF)
		{
			PARSER_Data_p->StreamState.PreviousTimestamp = Temp;
		}
		else
		{
			/* Calculate the difference between the current timestamp and the previous timestamp */
			if(Temp > PARSER_Data_p->StreamState.PreviousTimestamp)
			{
				Timediff = Temp - PARSER_Data_p->StreamState.PreviousTimestamp;
			}
			else if(Temp < PARSER_Data_p->StreamState.PreviousTimestamp)
			{
				Timediff = 0xFFFFFFFF - PARSER_Data_p->StreamState.PreviousTimestamp + 1 + Temp;
			}

            /* ensure that the frame rate is greater than than one frame per second. If it
			   is less than one frame per second, wait for the next Timestamp before computing
			   the frame rate. */
			if(Timediff <= 1000)
			{
				/* Timestamp ticks are in milliseconds. the number of milliseconds per second (1000)
				   divided by the number of milliseconds per frame gives the number of frames per second. */
				PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = (1000 / Timediff);
				PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = (1000 / Timediff);
				PARSER_Data_p->StreamState.FrameRatePresent = TRUE;
			}
			else
			{
				PARSER_Data_p->StreamState.PreviousTimestamp = Temp;
			}

		}/* end if(PARSER_Data_p->StreamState.PreviousTimestamp == 0xFFFFFFFF) -- else*/
	}/* if(PARSER_Data_p->StreamState.FrameRatePresent == FALSE) -- else */

	PARSER_Data_p->StreamState.PreviousTimestamp = Temp;
	/* Associate PTS to Picture */
	PARSER_Data_p->PTSAvailableForNextPicture = TRUE;
	PARSER_Data_p->PTSForNextPicture.Interpolated = FALSE;

}/* end vc1par_ParseFrameLayerDataStruct() */


/* Parse Advanced Profile Frame */
void vc1par_ParseAPFrameLayerDataStruct(const PARSER_Handle_t ParserHandle)
{
	BOOL SkipPicture;

#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"vc1par_ParseAPFrameLayerDataStruct  "));
#endif

	/* No Start Code Emulation Prevention bytes in the Frame Layer Data Structure. */
	vc1par_DisableDetectionOfStartCodeEmulationPreventionBytes(ParserHandle);

	/* @@@ How do I handle the skip picture here???*/
	vc1par_ParseFrameLayerDataStruct(&SkipPicture, ParserHandle);

    /* For Advanced Profile, Emulation Prevention can occur after the
	   Frame Layer Data Structure.  */
	vc1par_EnableDetectionOfStartCodeEmulationPreventionBytes(ParserHandle);
}/* end vc1par_ParseAPFrameLayerDataStruct() */


/*******************************************************************************/
/*! \brief Parses a Simple/Main Profile frame header
*
* Picture info is updated.
*   Assumption: detection of SC Emulation Prevention
*               bytes has already been disabled before this point.
*
* \param ParserHandle Parser Handle (variables managed by the parser)
* \return void
*/
/******************************************************************************/
void vc1par_ParseSPMPFrame(U8 *StartCodeValueAddress,
						   U8 *NextBDUStartCodeValueAddress,
						   const PARSER_Handle_t ParserHandle)
{
	VC1ParserPrivateData_t * PARSER_Data_p;

	BOOL SkipPicture;
	U32 Temp;
	U32 NumDigits = 0;
	BOOL CurRangeredfrmFlag = FALSE;
	BOOL RefPic = FALSE;
    U8 MoveBackLength;

	/* For MVMODE2 VLC interpretation. */
	/* valid values are 1, 01, 001, 000 */
	U32 MVMode2Table[] = {2,0,4,0,0,0};
	U32 MVandDMVRangeTable[] = {0,2,0,4,0,0};
	U32 LumShiftLumScale = VC9_NO_INTENSITY_COMP;

#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"vc1par_ParseSPMPFrame  "));
#endif

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Set to FALSE by default */
	PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = FALSE;

#ifdef STUB_INJECT
	vc1par_ParseFrameLayerDataStruct(&SkipPicture, ParserHandle);
    PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress =
		(void *) vc1par_MoveForwardInStream(StartCodeValueAddress, VC1_FRAME_LAYER_DATA_STRUCT_SIZE_BYTES, ParserHandle);
	/* The stop address is the byte preceeding the frame layer data structure */
	PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  =
		(void *) vc1par_MoveBackInStream(NextBDUStartCodeValueAddress, 1, ParserHandle);

#else
	vc1par_SearchForAssociatedPTS(ParserHandle);

    if (PARSER_Data_p->PTSAvailableForNextPicture)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = TRUE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS = PARSER_Data_p->PTSForNextPicture;
        PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
    }
    else
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE; /* Set to TRUE as soon as a PTS is detected */
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS = 0;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33 = FALSE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated = TRUE; /* default zero PTS value must be considered as interpolated for the producer */
    }

/* temporary flagged in WINCE environment to be confirmed */
#ifdef ST_OSWINCE
	vc1par_ParseFrameLayerDataStruct_NotAligned(&SkipPicture, ParserHandle);
#else
	vc1par_ParseFrameLayerDataStruct(&SkipPicture, ParserHandle);
#endif

	/* Update position in bit buffer */
    /* The picture start address used through the CODEC API is normally the position of the beginning of the start code;
	however, in this case, there are no start codes, so the start address is the END of the  Frame Layer data structure. */
    PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress =
		(void *) vc1par_MoveForwardInStream(StartCodeValueAddress, VC1_FRAME_LAYER_DATA_STRUCT_SIZE_BYTES+1, ParserHandle);

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
    /* Check if there is a PES header before the beginning of next BDU */
    MoveBackLength = 0;
    if (vc1par_PESHeaderExist(NextBDUStartCodeValueAddress, ParserHandle) == true)
        MoveBackLength += VC1_MP_SC_PES_HEADER_LENGTH;

	/* The stop address is the byte preceding the frame layer data structure */
	/* When we use the Start Code + signature, we should move back to */
	/* the beginning  of the start code, so we must add VC1_MP_SC_SIGNATURE_SIZE to point on 0 0 1 SC */
	PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  =
		(void *) vc1par_MoveBackInStream(NextBDUStartCodeValueAddress, 3 + VC1_MP_SC_SIGNATURE_SIZE + MoveBackLength, ParserHandle);

#else
		/* The stop address is the byte preceding the frame layer data structure */
	PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  =
		(void *) vc1par_MoveBackInStream(NextBDUStartCodeValueAddress, 3, ParserHandle);
#endif

#endif  /* STUB_INJECT */

	if(SkipPicture == TRUE)
	{
#if defined(TRACE_UART) && defined(VALID_TOOLS)
		TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"Skip Picture  "));
#endif
		/* Skip Picture type acts like a P picture, so it
		   is always a reference picture type*/
		PARSER_Data_p->PictureGenericData.IsReference = TRUE;
		RefPic = TRUE;

		/* if there are no frames already in Reference List P0, then
		skip the frame because it doesn't have the frame that it refers to. */
		if(PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 > 0)
		{
			PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
		}
		else
		{
			PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
		}

		PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_SKIP;
		PARSER_Data_p->PictureGenericData.PictureInfos.
			VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;

		/* There is no intensity compensation to apply to the forward reference frame */
		PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1 = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->StreamState.IntensityCompPrevRefPicture = VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1 = VC9_NO_INTENSITY_COMP;

		/* Leave RespicBBackwardRefPicture the same */
		PARSER_Data_p->StreamState.RespicBForwardRefPicture  = PARSER_Data_p->StreamState.RespicBBackwardRefPicture;
		PARSER_Data_p->PictureSpecificData.HalfWidthFlag = PARSER_Data_p->StreamState.RespicBBackwardRefPicture & 1;
		PARSER_Data_p->PictureSpecificData.HalfHeightFlag = (PARSER_Data_p->StreamState.RespicBBackwardRefPicture & 2) >> 1;


		/* ForwardRangeredfrmFlag is the RangeredfrmFlag which we
		   saved off from the previous (forward) reference frame */
		PARSER_Data_p->PictureSpecificData.ForwardRangeredfrmFlag =
			PARSER_Data_p->StreamState.ForwardRangeredfrmFlagPFrames;

		/* Since this is a skip frame, it is identical to the previous
		   reference frame, so the stored ForwardRangeredfrmFlag information
		   for future P frames stays the same while the RangeredfrmFlag
		   value for the previous reference frame becomes the forward value
		   for future B pictures */
		PARSER_Data_p->StreamState.ForwardRangeredfrmFlagBFrames = PARSER_Data_p->StreamState.ForwardRangeredfrmFlagPFrames;

		/* NOTE that Rndctrl for Skip frames stays the same as previous
		   reference frames in decode order */

	}
	else /* not a SKIP frame */
	{

#ifdef STUB_INJECT
		/* We read in the frame layer data structure which was little
		   endian.  Now, the rest is big endian.  So change modes*/
		vc1par_SwitchFromLittleEndianToBigEndianParsing(ParserHandle);
#endif

		if(PARSER_Data_p->GlobalDecodingContextSpecificData.FinterpFlag == TRUE)
		{
			/* INTERPFRM */
			Temp = vc1par_GetUnsigned(1, ParserHandle);
			VC1_PARSER_PRINT(("INTERPFRM = %d\n", Temp));
		}

		/* FRMCNT */
		Temp = vc1par_GetUnsigned(2, ParserHandle);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
		/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"FRMCNT %d  ", Temp)); */
#endif

		if(PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag == TRUE)
		{
			CurRangeredfrmFlag = vc1par_GetUnsigned(1, ParserHandle);

			if(CurRangeredfrmFlag == 1)
			{
				PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorY =	YUV_HALF_VALUE;
				PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorUV = YUV_HALF_VALUE;
			}
			else
			{
				PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorY =	YUV_NO_RESCALE;
				PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorUV = YUV_NO_RESCALE;
			}
		}/* end if(PARSER_Data_p->GlobalDecodingContextSpecificData.RangeredFlag == TRUE) */

		/* No B frames */
		if(PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes == 0)
		{
			if(vc1par_GetUnsigned(1, ParserHandle) == 0) /* I Frame */
			{
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_I;
				PARSER_Data_p->PictureGenericData.PictureInfos.
								  VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
				PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;

#if defined(TRACE_UART) && defined(VALID_TOOLS)
				TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"PType is I  "));
#endif
				/* rndctrl, for simple and main profile is always one for I and BI frames (see 8.3.7) */
				PARSER_Data_p->StreamState.RndCtrlRecentRefPicture = 1;

			}
			else /* P frame */
			{

#if defined(TRACE_UART) && defined(VALID_TOOLS)
				TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"PType is P  "));
#endif
				PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_P;
				PARSER_Data_p->PictureGenericData.PictureInfos.
								  VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;
				/* ForwardRangeredfrmFlag is the RangeredfrmFlag which we
				saved off from the previous (forward) reference frame */
				PARSER_Data_p->PictureSpecificData.ForwardRangeredfrmFlag =
					PARSER_Data_p->StreamState.ForwardRangeredfrmFlagPFrames;

				/* if there are no frames already in Reference List P0, then
				   skip the frame because it doesn't have all of its reference frames
				   available.  */
				if(PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 > 0)
				{
					PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
				}
				else
				{
					PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
				}

				/* rndctrl, for simple and main profile toggles between 1 and 0 for P frames (see 8.3.7).
				   Use exclusive-or with one to toggle bit 0. */
				PARSER_Data_p->StreamState.RndCtrlRecentRefPicture ^= 1;
			}
		}
		else
		{
			/* Read in PTYPE VLC, valid values are 1, 01, 00
			Note that vc1par_GetVariableLengthCode cannot be used here because
			it will return the value of 1 for 2 out of the 3 values,
			so the solution is to read a bit until a one is received or
			until 2 bits have been read.  The number of bits (NumDigits) read in
			will determine the value of the VLC as follows:
			NumDigits   VLC value  VLC Meaning
			0           1             P
			1           01            I
			2           00            B or BI  */

			Temp = 0;

			while((NumDigits < 2) && ((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 0))
			{
				NumDigits++;
			}

			switch(NumDigits)
			{
				case 0:

#if defined(TRACE_UART) && defined(VALID_TOOLS)
					TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"PType is P  "));
#endif
					PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_P;
					PARSER_Data_p->PictureGenericData.PictureInfos.
								  VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;

					/* ForwardRangeredfrmFlag is the RangeredfrmFlag which we
					   saved off from the previous (forward) reference frame */
					PARSER_Data_p->PictureSpecificData.ForwardRangeredfrmFlag =
						PARSER_Data_p->StreamState.ForwardRangeredfrmFlagPFrames;

					/* if there are no frames already in Reference List P0, then
					skip the frame because it doesn't have all of its reference frames
					available.  */
					if(PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 > 0)
					{
						PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
					}
					else
					{
						PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
					}

					/* rndctrl, for simple and main profile toggles between 1 and 0 for P frames (see 8.3.7).
					Use exclusive-or with one to toggle bit 0. */
					PARSER_Data_p->StreamState.RndCtrlRecentRefPicture ^= 1;
					PARSER_Data_p->PictureSpecificData.NumeratorBFraction   = 0;
					PARSER_Data_p->PictureSpecificData.DenominatorBFraction = 0;
					break;
				case 1:

#if defined(TRACE_UART) && defined(VALID_TOOLS)
					TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"PType is I  "));
#endif
					PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_I;
					PARSER_Data_p->PictureGenericData.PictureInfos.
								  VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
					/* ForwardRangeredfrmFlag is always  0 for an I frame */
					PARSER_Data_p->PictureSpecificData.ForwardRangeredfrmFlag = 0;
					PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;

					/* rndctrl, for simple and main profile is always one for I and BI frames (see 8.3.7) */
					PARSER_Data_p->StreamState.RndCtrlRecentRefPicture = 1;
					PARSER_Data_p->PictureSpecificData.NumeratorBFraction   = 0;
					PARSER_Data_p->PictureSpecificData.DenominatorBFraction = 0;

					break;
				case 2:
					PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
					PARSER_Data_p->PictureGenericData.IsReference = FALSE;


					/* read in BFRACTION.  If BFRACTION is 1111111, then the frame
					   is a BI frame. Indicate TRUE for the Main profile so that the
					   function will set Temp to TRUE if BFRACTION is 1111111*/
					(void)vc1par_readBFraction(ParserHandle, & (PARSER_Data_p->PictureSpecificData.NumeratorBFraction),
										 & (PARSER_Data_p->PictureSpecificData.DenominatorBFraction),
								         TRUE, (BOOL*)&Temp);
                    /* BI frame */
					if(Temp == TRUE)
					{

#if defined(TRACE_UART) && defined(VALID_TOOLS)
						TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"PType is BI  "));
#endif

						/* rndctrl, for simple and main profile is always one for I and BI frames (see 8.3.7) */
						PARSER_Data_p->StreamState.RndCtrlRecentRefPicture = 1;
						PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_BI;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1 = VC9_NO_INTENSITY_COMP;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1 = VC9_NO_INTENSITY_COMP;
						/* ForwardRangeredfrmFlag is always  0 for a BI frame */
						PARSER_Data_p->PictureSpecificData.ForwardRangeredfrmFlag = 0;
						PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;

					}

					/* B frame */
					else
					{
#if defined(TRACE_UART) && defined(VALID_TOOLS)
						TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"PType is B  "));
#endif

						/* ForwardRangeredfrmFlag is the RangeredfrmFlag which we
							saved off from the forward reference frame (not the
						    previous reference frame, but the one before that */
						PARSER_Data_p->PictureSpecificData.ForwardRangeredfrmFlag =
							PARSER_Data_p->StreamState.ForwardRangeredfrmFlagBFrames;
						PARSER_Data_p->PictureSpecificData.PictureType = VC9_PICTURE_TYPE_B;

						PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1 =
							PARSER_Data_p->StreamState.IntensityCompPrevRefPicture;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1 =
							PARSER_Data_p->StreamState.IntensityCompPrevRefPicture;

					 /* There must be at least one forward reference frame and one backward
						reference frame for this frame to be valid; otherwise,
						skip the frame because the frames that it refers to do not exist. */
						if((PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 > 0) &&
							(PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 > 0))
						{
							PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
						}
						else
						{
							PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
						}

						/* NOTE that Rndctrl for B frames stays the same as previous
						   reference frames in decode order */
					}/* end if((vc1par_GetUnsigned(3, ParserHandle) == 0x7) -- else  */

					/* If RESPIC exists, set the HalfWidthFlag and HalfHeightFlag according
					   to the RESPIC of the forward reference frame. */
					if(PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag == 1)
					{
						PARSER_Data_p->PictureSpecificData.HalfWidthFlag = PARSER_Data_p->StreamState.RespicBForwardRefPicture & 1;
						PARSER_Data_p->PictureSpecificData.HalfHeightFlag = (PARSER_Data_p->StreamState.RespicBForwardRefPicture & 2) >> 1;
					}

					break;
			}

		}/* end if(PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes == 0) -- else */

		PARSER_Data_p->PictureSpecificData.RndCtrl = PARSER_Data_p->StreamState.RndCtrlRecentRefPicture;

		if((PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_BI) ||
		   (PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_I))
		{
			/* @@@ BF */
			Temp = vc1par_GetUnsigned(7, ParserHandle);
		}

		/* For P and I pictures, we may need to to read in RESPIC and,
		   for P pictures only, Intensity compensation info*/
		if((PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_I)||
		   (PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_P))
		{
			PARSER_Data_p->PictureGenericData.IsReference = TRUE;
			RefPic = TRUE;

			/* Since this is a reference frame, change the stored ForwardRangeredfrmFlag information.
			   The RangeredfrmFlag value for the current frame becomes the forward value for future P pictures
			   while the RangeredfrmFlag value for the previous reference frame becomes the forward value for future B pictures*/
			PARSER_Data_p->StreamState.ForwardRangeredfrmFlagBFrames = PARSER_Data_p->StreamState.ForwardRangeredfrmFlagPFrames;
			PARSER_Data_p->StreamState.ForwardRangeredfrmFlagPFrames = CurRangeredfrmFlag;

			/*  For I pictures, If Multires flag is not set, then there will
			    be no RESPIC, so no need to parse any more in the picture header
			    For P pictures, If Multires flag is not set and the profile is
			      Simple Profile, there is no RESPIC or intensity compenation
			      info, so no need to parse any more in the picture header.*/
			if((PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag == 1) ||
				((PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_P) &&
				(PARSER_Data_p->PrivateGlobalContextData.Profile != VC9_PROFILE_SIMPLE)))
			{
				/* skip PQINDEX see 7.1.1.6 (identical for progressive and interlace) */
				Temp = vc1par_GetUnsigned(5, ParserHandle);
				VC1_PARSER_PRINT(("PQINDEX = %d\n",Temp));

				/* skip HALFQP if necessary*/
				if(Temp <= 8)
				{
					Temp = vc1par_GetUnsigned(1, ParserHandle);

					VC1_PARSER_PRINT(("HALFQP = %d\n",Temp));
				}

				/* skip PQUANTIZER if necessary -- see 7.1.1.8 */
				if(PARSER_Data_p->PictureDecodingContextSpecificData.Quantizer == 1)
				{
					Temp = vc1par_GetUnsigned(1, ParserHandle);
					VC1_PARSER_PRINT(("PQUANTIZER = %d\n",Temp));
				}

				/* skip POSTPROC if necessary -- see 7.1.1.27*/
				if(PARSER_Data_p->GlobalDecodingContextSpecificData.PostProcessingFlag == 1)
				{
					Temp = vc1par_GetUnsigned(1, ParserHandle);
					VC1_PARSER_PRINT(("POSTPROC = %d\n",Temp));
				}

				/* skip MVRANGE if necessary -- see 7.1.1.9 and 9.1.1.26*/
				if(PARSER_Data_p->PictureDecodingContextSpecificData.ExtendedMVFlag == 1)
				{
					/* MVRANGE has a maximum of 3 bits. */
					Temp = vc1par_GetVariableLengthCode(MVandDMVRangeTable, 3, ParserHandle);
					VC1_PARSER_PRINT(("MVRANGE = %d\n",Temp));
				}

				if(PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag == 1)
				{
					Temp = vc1par_GetUnsigned(2, ParserHandle);
					VC1_PARSER_PRINT(("RESPIC = %d\n",Temp));
					PARSER_Data_p->PictureSpecificData.HalfWidthFlag = Temp & 1;
					PARSER_Data_p->PictureSpecificData.HalfHeightFlag = (Temp & 2) >> 1;

					/* If multiresolution is used, then the coded frame size might be
					   less than the display frame size.  If the HalfWidthFlag is set,
					   then the coded width is one half of the displayed width.  The
					   display will know to scale the picture when it sees this situation.
					   Also, if the HalfHeightFlag is set, then the coded height is one half
					   of the displayed height.  No need to make these changes for B frames
					   because B frames automatically take the same value as the previous P frame. */
					if(PARSER_Data_p->PictureSpecificData.HalfWidthFlag == 1)
					{
						PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width =
							(PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth/2);
					}
					else
					{
						PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width =
							PARSER_Data_p->PictureDecodingContextSpecificData.CodedWidth;
					}

					if(PARSER_Data_p->PictureSpecificData.HalfHeightFlag == 1)
					{
						PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height =
							(PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight/2);
					}
					else
					{
						PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height =
							PARSER_Data_p->PictureDecodingContextSpecificData.CodedHeight;
					}


					/* When the next B frame comes along, it needs to use RESPIC for
					   its forward reference frame.  this means that we need to keep track
					   of RESPIC for the last 2 reference frames. */
					PARSER_Data_p->StreamState.RespicBForwardRefPicture  = PARSER_Data_p->StreamState.RespicBBackwardRefPicture;
					PARSER_Data_p->StreamState.RespicBBackwardRefPicture = Temp;
				}

				/* If a Main profile P picture, look for Intensity Compensation information */
				if((PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_P) &&
					(PARSER_Data_p->PrivateGlobalContextData.Profile != VC9_PROFILE_SIMPLE))
				{
				/* Read in MVMODE VLC, valid values are 1, 01, 001, 0000, 0001
				   Note that vc1par_GetVariableLengthCode cannot be used here because
				   it will return the value of 1 for 4 out of the 5 values,
				   so the solution is to read a bit until a one is received or
				   until 4 bits have been read.  The number of bits (NumDigits) read in
				   will determine the value of the VLC as follows:
				   NumDigits   VLC value
				    0           1
				    1           01
				    2           001
				    3           0001 (specifying Intensity compensation, the only value we care about)
				    4           0000 	*/
					while((NumDigits < 4) && ((Temp = vc1par_GetUnsigned(1, ParserHandle)) == 0))
					{
						NumDigits++;
					}

					/* If intensity compensation is present, read it in. */
					if(NumDigits == 3)
					{
						/* since we don't care about the value of MVMODE2, we can use
						 * vc1par_GetVariableLengthCode
						 */
						Temp = vc1par_GetVariableLengthCode(MVMode2Table, 3, ParserHandle);

						/* LUMSCALE */
						LumShiftLumScale = vc1par_GetUnsigned(6, ParserHandle);

						/* LUMSHIFT */
						LumShiftLumScale |= (vc1par_GetUnsigned(6, ParserHandle) << 16);

						VC1_PARSER_PRINT(("Intensity Compensation Present, LumShiftLumScale = %8X\n", LumShiftLumScale));
					}
					else
					{
						VC1_PARSER_PRINT(("Intensity Compensation Not Present\n"));
					}

				}/* end if((PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_P) ... */
			}/* end if((PARSER_Data_p->GlobalDecodingContextSpecificData.MultiresFlag == 1) ...*/

			/* set Intensity Compensation info for reference pictures */
			PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   = LumShiftLumScale;
			PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   = LumShiftLumScale;
			PARSER_Data_p->StreamState.IntensityCompPrevRefPicture = LumShiftLumScale;

		}/* end if((PARSER_Data_p->PictureSpecificData.PictureType == VC9_PICTURE_TYPE_I)... */

	}/* end if(SkipPicture == TRUE) -- else */

	/* No B frames, so there is no reordering, so presentation order
	   just increments by one each time.  */
	if(PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes == 0)
	{
		/* take care of rollover */
		if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID == (S32)MAXIMUM_S32)
		{
			PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID = 0;

			if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension == MAXIMUM_U32)
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension = 0;
			}
			else
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension++;
			}
		}
		else
		{
			PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID++;
		}

		PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID =
			PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID;
		PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
	}
	else /* B frames exist */
	{
#ifndef VC1_TEST_DISPLAY_ORDER_ID

	/* This is probably something like the way Presentation Order Picture ID
	   will be calculated.  Since the presentation order picture ID is
	   not known for a reference picture at parse-time, the value of
	   the previous presentation order picture ID will be passed back
	   to the producer.  But if there was a reference picture
	   waiting on its presentation order, the CurrentPresentationOrderPictureID
	   is incremented when a new reference picture comes in to account
	   for the waiting reference picture.  CurrentPresentationOrderPictureID
	   also gets incremented by one for every non-reference picture.
	   For the non-reference picture, this value will actually be passed
	   back to the producer.
	   The following illustrates what should happen:

		 PType				  I   P   B   B   B   P   B   P   B   B   P
		 Returned Pres ID	  0   0   2   3   4   4   6   6   8   9   9
		 Stored Pres ID       0   1   2   3   4   5   6   7   8   9   10

		   Note that the producer can store off the previous value of presentation ID
		   and assume that if the current presentation ID matches the previous, then
		   the Presentation ID is not known.  Actually a new flag should be added that
		   indicates that the Presentation ID is not known,
		   but at this point, I don't know what the flag would be.   */


		if((RefPic) && (!PARSER_Data_p->StreamState.WaitingRefPic))
		{
			PARSER_Data_p->StreamState.WaitingRefPic = TRUE;
		}
		else
		{
			/* take care of rollover */
			if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID == MAXIMUM_U32)
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID = 0;

				if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension == MAXIMUM_U32)
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension = 0;
				}
				else
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension++;
				}
			}
			else
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID++;
			}

			if(!RefPic)
			{
				PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID =
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID;
			    PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
			}
		}/* end if((RefPic) && (!PARSER_Data_p->StreamState.WaitingRefPic)) -- else */

#else/*  VC1_TEST_DISPLAY_ORDER_ID */


		/* This is a presentation order implementation done only for
		 * initial testing before a interface change is made.
		 * basically, whenever a reference picture comes in,
		 * a constant is added to presentation order ID to
		 * account for the Maximum B frames.
		 *
		 *  BackwardRefPresentationOrderPictureID is the current reference
		 *    picture presentation order ID.
		 *  CurrentPresentationOrderPictureID is the current non-reference picture
		 *    presentation order ID.  This gets incremented by one whenever a
		 *    new non-reference picture comes in.  Whenever a reference picture
		 *    comes in,  CurrentPresentationOrderPictureID gets set to the
		 *    previous reference picture presentation order picture ID.
		 *
		 *  So the presentation order picture ID should look like the following:
		 *   (assuming MAX_B_FRAMES is 20)
		 *
		 *  Type:      I   P   B   B   B    P    B    B    P    B    B    P
		 *  pres ID:   20  40  21  22  23   60   41   42   80   61   62   100
		 */

		if(RefPic)
		{
			PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID     =
				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID;

			/* take care of rollover */
			if(PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID > (S32)(MAXIMUM_S32 - MAX_B_FRAMES))
			{
				if(PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension == MAXIMUM_U32)
				{
					PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension = 0;
				}
				else
				{
					PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension++;
				}

				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID = (MAX_B_FRAMES - (MAXIMUM_U32 -
					PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID));
			}
			else
			{
				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID += MAX_B_FRAMES;
			}
			PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID =
				PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID;
			PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
		}
		else/* not a reference picture */
		{
			/* take care of rollover */
			if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID == (S32)MAXIMUM_S32)
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID = 0;

				if(PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension == MAXIMUM_U32)
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension = 0;
				}
				else
				{
					PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension++;
				}
			}
			else
			{
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID++;
			}
			PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID =
				PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID;
			PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
		}
#endif /* end VC1_TEST_DISPLAY_ORDER_ID */
	}/* end if(PARSER_Data_p->GlobalDecodingContextSpecificData.Maxbframes == 0) -- else */


	PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;
}/* end vc1par_ParseSPMPFrame */



/*******************************************************************************/
/*! \brief Parses a field header
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
* \return void
*/
/******************************************************************************/
void vc1par_ParseField(U8 *StartCodeValueAddress,
					   U8 *NextBDUStartCodeValueAddress,
					   const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;
	U32  RefPicIndex0,RefPicIndex1;
	U32  i = 0;
	U8   IntCompField;
	U32  IntComp1;
	U32  IntComp2;
	U32  IntComp1Mod;
	U32  IntComp2Mod;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_PARSEBDU,"vc1par_ParseField\n"));
#endif

		/* Update position in bit buffer */
    /* The picture start address is the byte after the Start Code Value */
    PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress = (void *) vc1par_MoveForwardInStream(StartCodeValueAddress, 1, ParserHandle);
	/* The stop address points to the Start Code Value Byte of the next start code */
	PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  = NextBDUStartCodeValueAddress;

	PARSER_Data_p->PictureSpecificData.PictureType = PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific;
	PARSER_Data_p->PictureGenericData.PictureInfos.
					VideoParams.MPEGFrame = PARSER_Data_p->StreamState.SecondFieldPictureTypeGeneric;

	PARSER_Data_p->PictureSpecificData.SecondFieldFlag = 1;
	PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields = FALSE;
	PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[2] = FALSE;
	/* If the top field was not first then this, the second field must be the top field; Otherwise,
	   the top field was first and this is the bottom field.  */
	if(PARSER_Data_p->PictureSpecificData.TffFlag == FALSE)
	{
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_TOP_FIELD;
	}
	else
	{
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_BOTTOM_FIELD;
	}

	if(PARSER_Data_p->StreamState.SecondFieldPictureTypeSpecific == VC9_PICTURE_TYPE_P)
	{
		vc1par_ParseAPTypePPicturePartTwo(VC1_FIELD_INTERLACE, &IntCompField,
			                              &IntComp1, &IntComp2, &IntComp1Mod,
										  &IntComp2Mod, TRUE, ParserHandle);
	}

	PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;

	vc1par_InitIntComp(ParserHandle);

	switch(PARSER_Data_p->PictureSpecificData.PictureType)
	{
		case VC9_PICTURE_TYPE_I:
			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax  = 0;
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax = 0;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
			PARSER_Data_p->PictureSpecificData.BackwardRefDist = 0;
			break;

		case VC9_PICTURE_TYPE_P:
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;

			RefPicIndex0 = PARSER_Data_p->StreamState.NextReferencePictureIndex; /* Index to forward reference frame */
			RefPicIndex1 = PARSER_Data_p->StreamState.NextReferencePictureIndex^1; /* Index to current frame within
																					the Full Ref Frame List */

			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax = PARSER_Data_p->StreamState.ForwardReferencePictureSyntax;
			i = 0;

			PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;

			/* If the current field refers back to the previous reference frame or field-pair, then
			   set the first element in list P0 to either the second field in the previous reference field-pair
			   or to the whole previous reference frame.  */
			if((PARSER_Data_p->PictureLocalData.NumRef == 1) || (PARSER_Data_p->PictureLocalData.RefField == 1))
			{
				if(PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[RefPicIndex0] == FALSE)
				{
					STTBX_Print(("WARNING Not all references available for second field of field-pair %u\nError Correction: Only refer to"
						         "the first field of the field-pair and not to the previous field pair\n",
								 PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));

					if(PARSER_Data_p->PictureLocalData.RefField == 1)
					{
						PARSER_Data_p->PictureGenericData.ReferenceListP0[i] = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;

						/* If the top field is first, then we are refering to the top field, if top field is false,
						then we are referring to the bottom field.*/
						PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[i]	= PARSER_Data_p->PictureSpecificData.TffFlag;

						i++;
					}
				}
				else
				{
					PARSER_Data_p->PictureGenericData.ReferenceListP0[i] =
						PARSER_Data_p->PictureGenericData.FullReferenceFrameList[RefPicIndex0];

					/* refer to the second field in the field-pair/frame */
					PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[i]	= (!PARSER_Data_p->PictureSpecificData.TffFlag);

					i++;
				}
			}/* end if((PARSER_Data_p->PictureLocalData.NumRef == 1) || (PARSER_Data_p->PictureLocalData.RefField == 0)) */

			/* If the current field refers back to the first field in its own field-pair, then
			   no need to put the current picture in the reference picture list.  The Decoder will
			   already have a pointer to the current picture and will know from the picture syntax whether it is used
			   for reference or not.  */
#if 0
			/* If the current field refers back to the first field in its own field-pair, then
			   set the next element in list P0 to its own DecodingOrderFrameID and indic.  */
			if((PARSER_Data_p->PictureLocalData.NumRef == 1) || (PARSER_Data_p->PictureLocalData.RefField == 0)) /* NumRef == 0 */
			{
				PARSER_Data_p->PictureGenericData.ReferenceListP0[i] = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;

				/* If the top field is first, then we are refering to the top field, if top field is false,
				   then we are referring to the bottom field.*/
				PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[i]	= PARSER_Data_p->PictureSpecificData.TffFlag;

				i++;
			}/* end if((PARSER_Data_p->PictureLocalData.NumRef == 1) || (PARSER_Data_p->PictureLocalData.RefField == 1)) */
#endif
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = i;
			PARSER_Data_p->PictureSpecificData.BackwardRefDist = 0;

			/* Handle Intensity Compensation For P Second Fields Here */

			/* The current field-pair has no previous intensity compensation information
			   applied to it yet, So, if there were valid previous values, clear them out
			   before filling in new values. It is possible there are residual values
			   from the reference frame that was in this slot recently. */
			if(PARSER_Data_p->StreamState.IntComp[RefPicIndex1].IntCompAvailable == TRUE)
			{
				PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Top_1 = VC9_NO_INTENSITY_COMP;
				PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Top_2 = VC9_NO_INTENSITY_COMP;
				PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Bot_1 = VC9_NO_INTENSITY_COMP;
				PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Bot_2 = VC9_NO_INTENSITY_COMP;
				PARSER_Data_p->StreamState.IntComp[RefPicIndex1].IntCompAvailable = FALSE;
			}

			switch(IntCompField)
			{
				case 0:/* Intensity Compensation available for top and bottom reference fields */

					if(PARSER_Data_p->PictureSpecificData.TffFlag)
					{
						/* Since the current field is a bottom field, IntComp1 refers
						   to the corresponding top field in the same field-pair and
						   IntComp2 refers to the bottom field of the forward reference
						   field-pair.  It's possible that there could already be Intensity
						   compensation information for this bottom field (provided
						   in the top field of the current field pair)*/
						PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Top_1 = IntComp1;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   = IntComp1Mod;

						if(PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_1 == VC9_NO_INTENSITY_COMP)
						{
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_1 = IntComp2;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   = IntComp2Mod;
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].IntCompAvailable = TRUE;
						}
						else
						{
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_2 = IntComp2;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   =
									   PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_2   = IntComp2Mod;
						}
					}/* end if(PARSER_Data_p->PictureSpecificData.TffFlag) */
					else
					{
						/* Since the current field is a top field, IntComp2 refers
						   to the corresponding bottom field in the same field-pair and
						   IntComp1 refers to the top field of the forward reference
						   field-pair.  It's possible that there could already be Intensity
						   compensation information for this top field (provided
						   in the bottom field of the current field pair)*/
						PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Bot_1 = IntComp2;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   = IntComp2Mod;

						if(PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_1 == VC9_NO_INTENSITY_COMP)
						{
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_1 = IntComp1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   = IntComp1Mod;
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].IntCompAvailable = TRUE;
						}
						else
						{
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_2 = IntComp1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   =
								PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_2   = IntComp1Mod;
						}

					}/* end if(PARSER_Data_p->PictureSpecificData.TffFlag) -- else*/

					PARSER_Data_p->StreamState.IntComp[RefPicIndex1].IntCompAvailable = TRUE;
			                PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
					break;

				case 1:/* Intensity Compensation available for just the bottom reference fields */

					if(PARSER_Data_p->PictureSpecificData.TffFlag)
					{
						/* Since the current field is a bottom field,
						   IntComp1 refers to the bottom field of the forward reference
						   field-pair.  It's possible that there could already be Intensity
						   compensation information for this bottom field (provided
						   in the top field of the current field pair).  In this case,
						   there would not be any pre-existing Intensity Compensation info
						   for the top field of this field pair. */

						if(PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_1 == VC9_NO_INTENSITY_COMP)
						{
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_1 = IntComp1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   = IntComp1Mod;
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].IntCompAvailable = TRUE;
						}
						else
						{
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_2 = IntComp1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   =
									   PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_2   = IntComp1Mod;
						}
					}/* end if(PARSER_Data_p->PictureSpecificData.TffFlag) */
					else
					{
						/* Since the current field is a top field, IntComp1 refers
						   to the corresponding bottom field in the same field-pair */
						PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Bot_1 = IntComp1;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   = IntComp1Mod;
						PARSER_Data_p->StreamState.IntComp[RefPicIndex1].IntCompAvailable = TRUE;

						/* There may already be Intensity Compensation information
						   saved off for the top field of the forward reference frame.
			               If so, use it. */
						PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   =
								PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_1;

					}/* end if(PARSER_Data_p->PictureSpecificData.TffFlag) -- else*/
			                PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;

					break;
				case 2:/* Intensity Compensation available for just the top reference fields */

					if(PARSER_Data_p->PictureSpecificData.TffFlag)
					{
						/* Since the current field is a bottom field, IntComp1 refers
						   to the corresponding top field in the same field-pair */
						PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Top_1			  = IntComp1;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1			  = IntComp1Mod;
						PARSER_Data_p->StreamState.IntComp[RefPicIndex1].IntCompAvailable = TRUE;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1 =
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_1;

					}/* end if(PARSER_Data_p->PictureSpecificData.TffFlag) */
					else
					{
						/* Since the current field is a top field,
						   IntComp1 refers to the top field of the forward reference
						   field-pair.  It's possible that there could already be Intensity
						   compensation information for this top field (provided
						   in the bottom field of the current field pair)*/
						if(PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_1 == VC9_NO_INTENSITY_COMP)
						{
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_1 = IntComp1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   = IntComp1Mod;
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].IntCompAvailable = TRUE;
						}
						else
						{
							PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_2 = IntComp1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   =
								PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_1;
							PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_2   = IntComp1Mod;
						}

					}/* end if(PARSER_Data_p->PictureSpecificData.TffFlag) -- else*/
			                PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;

					break;
				case 3:/* No Intensity Compensation available with this field */

					/* If this field refers back to the previous field pair, there could be
					   some intensity comp info to include. */
					if((PARSER_Data_p->PictureLocalData.NumRef == 1) ||
					   (PARSER_Data_p->PictureLocalData.RefField == 1))
					{
					    /* this is a bottom field, the top field of this field-pair may have had intensity compensation
					       info related to the bottom field of the previous reference frame.  include this
						   info */
						if(PARSER_Data_p->PictureSpecificData.TffFlag)
						{
							PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1   =
								PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_1;
			                PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
						}
						/* this is a top field, the bottom field of this field-pair may have had intensity compensation
						info related to the top field of the previous reference frame.  include this
						info */
						else
						{
							PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1   =
								PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_1;
			                PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
						}
					}

					break;
				default:
					break;
			}/* end switch(IntCompField) */

			break;

		case VC9_PICTURE_TYPE_B:

			RefPicIndex0 = PARSER_Data_p->StreamState.NextReferencePictureIndex;    /* Index to forward Reference frame */
			RefPicIndex1 = PARSER_Data_p->StreamState.NextReferencePictureIndex ^ 1;/* Index to backward Reference frame */

			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax  = PARSER_Data_p->StreamState.ForwardReferencePictureSyntax;
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax = PARSER_Data_p->StreamState.BackwardReferencePictureSyntax;

			if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[RefPicIndex0] == FALSE) ||
			   (PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[RefPicIndex1] == FALSE))
			{
				STTBX_Print(("ERROR: Not all references available for second field of field-pair %u\n",
					PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));
				PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
				PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
				PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
			}/* end if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[RefPicIndex0] == FALSE) ...*/
			else
			{
				PARSER_Data_p->PictureGenericData.ReferenceListB0[0] =
					PARSER_Data_p->PictureGenericData.FullReferenceFrameList[RefPicIndex0];

				/* refer to the second field in the field-pair/frame */
				PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[0]	= !PARSER_Data_p->PictureSpecificData.TffFlag;
				PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 1;

				/* the decoder does not want the current picture in the reference picture list because it already
				   has the pointer to the current picture and it knows from the picture syntax that the first picture is used as reference. */
#if 0
				/* The second field in the Reference list will always be the first field of the
				   field-pair */
				PARSER_Data_p->PictureGenericData.ReferenceListB0[1]		= PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
				PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[1]	= PARSER_Data_p->PictureSpecificData.TffFlag;
				PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 2;
#endif /*0 */

				/* This is a rare case in which a B picture can be used as a reference (the first B field in a field-pair can be
				   a reference for the second B field).  In this case, add the DecodingOrderFrameID to
				   the full reference frame list. It will always be in position number 2 since, otherwise
				   there will only be 2 reference frames/field-pairs in the list */
				PARSER_Data_p->PictureGenericData.FullReferenceFrameList[2] = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
                /* Note CL: ugly workaround to avoid non existing reference issues detected in producer
                 * This should be done in the proper way later, i.e. mark the first B field as IsReference to make it handled properly by the producer
                 * ugly fix for now: *_Don't_* mark the previous B field as reference from the producer point of view
                 * Caution, this may lead to visual default which can't be hidden by the error recovery in producer in case of badly decoded first B field */
                /*original line was :
                 * PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[2] = TRUE;*/
                PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[2] = FALSE;

				PARSER_Data_p->PictureGenericData.ReferenceListB1[0] =
					PARSER_Data_p->PictureGenericData.FullReferenceFrameList[RefPicIndex1];

				/* If the frame referred to is actually a field pair, then include another entry
				   in B1 to account for the second field.  But  this second field will always
				   have the Decoding order frame ID of the first entry in the list.*/
				if(PARSER_Data_p->StreamState.FieldPairFlag[RefPicIndex1] == TRUE)
				{
					PARSER_Data_p->PictureGenericData.ReferenceListB1[1] =
						PARSER_Data_p->PictureGenericData.ReferenceListB1[0];
					PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[0]	= PARSER_Data_p->PictureSpecificData.TffFlag;
					PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[1] = !PARSER_Data_p->PictureSpecificData.TffFlag;
					PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 2;
				}
				else/* This is the case where we are referring to a frame rather than a field.*/
				{
					PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[0]	= TRUE;
					PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 1;
				}

				/* Copy Intensity Compensation Info from the Forward reference frame */
				if(PARSER_Data_p->StreamState.IntComp[RefPicIndex0].IntCompAvailable == TRUE)
				{
					if(PARSER_Data_p->PictureSpecificData.TffFlag)
					{
						/* The current field is a bottom field, so its forward bottom intensity compensation info
						   comes from the the bottom field of the forward reference field-pair or frame.
						   The top forward reference field has no intensity compensation because
						   intensity comp info comes in P fields only, but the bottom forward reference field may
						   have intensity comp info provided by the backward reference frame. */
						PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1 = PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_1;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_2 = PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Bot_2;
					}
					else
					{
						/* The current field is a top field, so its forward top intensity compensation info
						   comes from the the top field of the forward reference field-pair or frame.
						   The bottom forward reference field has no intensity compensation because
						   intensity comp info comes in P fields only, but the top forward reference field may
						   have intensity comp info provided by the backward reference frame. */
						PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1 = PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_1;
						PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_2 = PARSER_Data_p->StreamState.IntComp[RefPicIndex0].Top_2;
					}
			                PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
				}/* end if(PARSER_Data_p->StreamState.IntComp[RefPicIndex0].IntCompAvailable == TRUE) */

				/* Copy Intensity Compensation Info from the Backward reference frame */
				if(PARSER_Data_p->StreamState.IntComp[RefPicIndex1].IntCompAvailable == TRUE)
				{
					PARSER_Data_p->PictureSpecificData.IntComp.BackTop_1 = PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Top_1;
					PARSER_Data_p->PictureSpecificData.IntComp.BackTop_2 = PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Top_2;
					PARSER_Data_p->PictureSpecificData.IntComp.BackBot_1 = PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Bot_1;
					PARSER_Data_p->PictureSpecificData.IntComp.BackBot_2 = PARSER_Data_p->StreamState.IntComp[RefPicIndex1].Bot_2;
			                PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
				}/* end if(PARSER_Data_p->StreamState.IntComp[RefPicIndex0].IntCompAvailable == TRUE) */


			}/* end if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[RefPicIndex0] == FALSE) ... -- else*/

			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
			PARSER_Data_p->PictureSpecificData.BackwardRefDist = PARSER_Data_p->StreamState.RefDistPreviousRefPic;
			break;
		case VC9_PICTURE_TYPE_BI:
			PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax  = 0;
			PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax = 0;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
			PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
			PARSER_Data_p->PictureSpecificData.BackwardRefDist = 0;
			break;
		default:
			break;
	}/* end switch(PARSER_Data_p->PictureSpecificData.PictureType) */

	PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;

}/* end vc1par_ParseField() */


/******************************************************************************/
/*! \brief Move back by n byte in the stream
 *
 * \param Address position from where to rewind
 * \param RewindInByte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void * BackAddress
 */
/******************************************************************************/
U8 * vc1par_MoveBackInStream(U8 * Address, U8 RewindInByte, const PARSER_Handle_t  ParserHandle)
{
    U8 * BackAddress;
    /* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
}/* end vc1par_MoveBackInStream() */


/******************************************************************************/
/*! \brief Move forward by n byte in the stream
 *
 * \param Address position from where to start
 * \param Number of bytes to move forward
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void * BackAddress
 */
/******************************************************************************/
U8 * vc1par_MoveForwardInStream(U8 * Address, U8 Increment, const PARSER_Handle_t  ParserHandle)
{
    U8 * ForwardAddress;
    /* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* Adjust with bit buffer boundaries */
    if (Address > PARSER_Data_p->BitBuffer.ES_Stop_p - Increment)
    {
        /* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
        ForwardAddress = PARSER_Data_p->BitBuffer.ES_Start_p +
			             (Increment - (PARSER_Data_p->BitBuffer.ES_Stop_p -
						  Address) - 1);
    }
	else
	{
		ForwardAddress = Address + Increment;
	}
    return (ForwardAddress);
}/* end vc1par_MoveBackInStream() */


static void vc1par_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle)
{
    void* ES_Write_p;
	VC1ParserPrivateData_t * PARSER_Data_p;
    U8 * PictureStartAddress_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    PictureStartAddress_p = PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress;

    /* no PTS associated to current picture */
    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;

    /* Current position of write pointer in BitBuffer to check PTS & picture address side */
    ES_Write_p = PARSER_Data_p->BitBuffer.ES_Write_p;

    /* Check if PTS address is before PICT address in ES */
    while(
           (PARSER_Data_p->PTS_SCList.Read_p != PARSER_Data_p->PTS_SCList.Write_p) && /* PTS List is empty */
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
        PARSER_Data_p->PTS_SCList.Read_p++;
        if (((U32)PARSER_Data_p->PTS_SCList.Read_p) >= ((U32)PARSER_Data_p->PTS_SCList.SCList + sizeof(PTS_t) * PTS_SCLIST_SIZE))
        {
            PARSER_Data_p->PTS_SCList.Read_p = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
        }
    }
} /* End of vc1par_SearchForAssociatedPTS() function */


/******************************************************************************/
/*! \brief Reset the Pan And Scan parameters
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
static void vc1par_ResetPicturePanAndScanIn16thPixel(const PARSER_Handle_t  ParserHandle)
{
	VC1ParserPrivateData_t * PARSER_Data_p;
	U8 i;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	for(i = 0; i < VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; i++) {
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreHorizontalOffset = 0;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreVerticalOffset = 0;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayHorizontalSize =
			 16 * PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayVerticalSize =
			16 * PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation = FALSE;
	}
	PARSER_Data_p->PictureGenericData.NumberOfPanAndScan = 0;
}/* end vc1par_ResetPicturePanAndScanIn16thPixel*/


/*******************************************************************************/
/*! \brief Parses Frame Layer Data Structure
*
*   Assumption: detection of SC Emulation Prevention
*               bytes has already been disabled before this point.
*
* No way to handle a little endian header if not align on a word boundary
* So handle this specificly
*/
/******************************************************************************/
void vc1par_ParseFrameLayerDataStruct_NotAligned(BOOL *SkipPicture, const PARSER_Handle_t ParserHandle)
{
	U32 BETemp, Temp, Timediff;
	VC1ParserPrivateData_t * PARSER_Data_p;
	U8 Key, Res;
	U32 FrameSize;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	/* Get 32 bit in BigEndian and then read it in the right order*/

	BETemp = vc1par_GetUnsigned(32, ParserHandle);

  /* KEY -- already read in, so ignored this time */
	Key = (BETemp&0x00000080)>>7;
	Res = (BETemp&0x0000007F);
	FrameSize = ((BETemp&0xFF000000)>>24)|((BETemp&0x00FF0000)>>8)|((BETemp&0x0000FF00)<<8);

	/* RES */
	if(Res != 0)
	{
        STTBX_Print(("ERROR:  RES = %u,  should be 0\n", Res));
		PARSER_Data_p->PictureSpecificData.PictureError.RESOutOfRange = TRUE;
	}

	/* FRAMESIZE */
	if(FrameSize < 2)
	{
		*SkipPicture = TRUE;
	}
	else
	{
		*SkipPicture = FALSE;
	}


	/* TIMESTAMP -- use in place of PTS */
	Temp = vc1par_GetUnsigned(32, ParserHandle);

	/* big to little */
	Temp = ((Temp&0xFF000000)>> 24) | ((Temp&0x00FF0000)>> 8 ) | ((Temp&0x0000FF00)<< 8) | ((Temp&0x000000FF)<< 24);

	if(Temp < (0xFFFFFFFF/VC1_PTS_TICKS_PER_MILLISECOND))
	{
		PARSER_Data_p->PTSForNextPicture.PTS = Temp * VC1_PTS_TICKS_PER_MILLISECOND;
		PARSER_Data_p->PTSForNextPicture.PTS33 = 0;
	}
	else
	{
		/* @@@ assumption here is that TIMESTAMP never rolls over.  That would be a video 49 days long which
		       would be a very very large file.  */
		/* PTS has rolled over, but Timestamp has not. 0xFFFFFFFF/VC1_PTS_TICKS_PER_MILLISECOND is the
		   number of timestamps ticks it takes for the PTS to rollover. Using the modulo gives the number
		   of timestamp ticks after the PTS rollover.  Then multiplying by VC1_PTS_TICKS_PER_MILLISECOND
		   converts the timestamp tick to PTS ticks.  It is done this way to avoid overflowing a 32 bit integer.*/
		PARSER_Data_p->PTSForNextPicture.PTS = (Temp % (0xFFFFFFFF/VC1_PTS_TICKS_PER_MILLISECOND)) * VC1_PTS_TICKS_PER_MILLISECOND;

		/* Determine the number of times the 32-bit PTS has rolled over.  Then modulo 2 gives the PTS33 bit.*/
		PARSER_Data_p->PTSForNextPicture.PTS33 = (Temp / (0xFFFFFFFF/VC1_PTS_TICKS_PER_MILLISECOND)) % 2;
	}

	/* If no framerate has been given, use 2 Timestamps to determine the
	   framerate. */
	if(PARSER_Data_p->StreamState.FrameRatePresent == FALSE)
	{
		/* Only one Timestamp has occurred, so wait for the next timestamp */
		if(PARSER_Data_p->StreamState.PreviousTimestamp == 0xFFFFFFFF)
		{
			PARSER_Data_p->StreamState.PreviousTimestamp = Temp;
		}
		else
		{
			/* Calculate the difference between the current timestamp and the previous timestamp */
			if(Temp >= PARSER_Data_p->StreamState.PreviousTimestamp)
			{
				Timediff = Temp - PARSER_Data_p->StreamState.PreviousTimestamp;
			}
			else
			{
				Timediff = 0xFFFFFFFF - PARSER_Data_p->StreamState.PreviousTimestamp + 1 + Temp;
			}

            /* ensure that the frame rate is greater than than one frame per second. If it
			   is less than one frame per second, wait for the next Timestamp before computing
			   the frame rate. */
			if((Timediff <= 1000) && (Timediff != 0))
			{
				/* Timestamp ticks are in milliseconds. the number of milliseconds per second (1000)
				   divided by the number of milliseconds per frame gives the number of frames per second. */
				PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = (1000 / Timediff);
				PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = (1000 / Timediff);
				PARSER_Data_p->StreamState.FrameRatePresent = TRUE;
			}
			else
			{
				PARSER_Data_p->StreamState.PreviousTimestamp = Temp;
			}

		}/* end if(PARSER_Data_p->StreamState.PreviousTimestamp == 0xFFFFFFFF) -- else*/
	}/* if(PARSER_Data_p->StreamState.FrameRatePresent == FALSE) -- else */

	PARSER_Data_p->StreamState.PreviousTimestamp = Temp;
	/* Associate PTS to Picture */
	PARSER_Data_p->PTSAvailableForNextPicture = TRUE;
	PARSER_Data_p->PTSForNextPicture.Interpolated = FALSE;

}/* end vc1par_ParseFrameLayerDataStruct_NotAligned() */

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
/*******************************************************************************/
/*
 *
 check the PES header presence
 and return TRUE, if the sequence header start code is found
 *
 */
/*******************************************************************************/
static int vc1par_PESHeaderExist(U8* NextBDUStartCodeValueAddress_p, const PARSER_Handle_t  ParserHandle)
{
    U8 * SHAddress_p;
    U8 * StartAddress_p;
    U8 * StopAddress_p;
    
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
	
    if(NextBDUStartCodeValueAddress_p == NULL)   return FALSE;
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    StartAddress_p = STAVMEM_DeviceToCPU((void *)(PARSER_Data_p->BitBuffer.ES_Start_p),&VirtualMapping);
    StopAddress_p = STAVMEM_DeviceToCPU((void *)(PARSER_Data_p->BitBuffer.ES_Stop_p),&VirtualMapping);
	
	SHAddress_p  = (U8 *) vc1par_MoveBackInStream(NextBDUStartCodeValueAddress_p, 3 + VC1_MP_SC_SIGNATURE_SIZE + VC1_MP_SC_PES_HEADER_LENGTH, ParserHandle);
		
    SHAddress_p = STAVMEM_DeviceToCPU((void *)(SHAddress_p),&VirtualMapping);

	if(*SHAddress_p++ != 0x00) return FALSE;
	
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

    if(*(SHAddress_p++) != 0x00) return FALSE;
    
    /* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;
        
	if(*(SHAddress_p++) != 0x01) return FALSE;
	
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;
    
	if((*(SHAddress_p) != 0xE0) && (*(SHAddress_p) != 0xE1)) return FALSE;
	return TRUE;
}
#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */
