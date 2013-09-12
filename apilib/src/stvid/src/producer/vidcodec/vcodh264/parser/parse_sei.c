/*!
 *******************************************************************************
 * \file parse_sei.c
 *
 * \brief H264 SEI parsing and filling of data structures
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


/* H264 Parser specific */
#include "h264parser.h"

/*#define SEI_PARSING_VERBOSE*/

/* Functions ---------------------------------------------------------------- */
/******************************************************************************/
/*! \brief Increment an interpolated time code
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_IncrementInterpolatedTimeCode(const PARSER_Handle_t  ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;
    U32 MaxFrames;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    /* MaxFrames is the number of frames per second, depending on FrameRate.
     It is done +60 before dividing by 1000 in order to round 29970 or 59940 values */
    MaxFrames = (H264_DEFAULT_FRAME_RATE + 60) /1000;

	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Frames ++; /* TODO: fake value, we assume one more frame each picture! */
    while (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Frames >= MaxFrames)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Frames -= MaxFrames;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Seconds ++;
    }
    while (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Seconds >= 60)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Seconds -= 60;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Minutes ++;
    }
    while (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Minutes >= 60)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Minutes -= 60;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Hours ++;
    }
    while (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Hours >= 24)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Hours -= 24;
    }
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Interpolated = TRUE;

}

/******************************************************************************/
/*! \brief Reset the Pan And Scan parameters
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ResetPicturePanAndScanIn16thPixel(const PARSER_Handle_t  ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;
	U8 i;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	for(i = 0; i < VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; i++) {
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreHorizontalOffset = 0;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreVerticalOffset = 0;
        PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayHorizontalSize = 16 * PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Width;
        PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayVerticalSize = 16 * PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p->SequenceInfo.Height;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation = FALSE;
	}
    PARSER_Data_p->PictureGenericData.NumberOfPanAndScan = 0;
    PARSER_Data_p->PictureGenericData.PanScanRectRepetitionPeriod = 1;
    PARSER_Data_p->PictureGenericData.PanScanRectPersistence = 0;
}

/******************************************************************************/
/*! \brief Sets default parameters for the picture
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_SetDefaultSEI(const PARSER_Handle_t  ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* ScanType is found in ct_type SEI message */
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType = H264_DEFAULT_SCANTYPE;

	/* TopFieldFirst is found in pic_struct SEI message */
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = H264_DEFAULT_TOPFIELDFIRST;

	h264par_IncrementInterpolatedTimeCode(ParserHandle);

	/* RepeatFirstField is found in pic_struct SEI message */
	PARSER_Data_p->PictureGenericData.RepeatFirstField = H264_DEFAULT_REPEATFIRSTFIELD;

	/* TODO: where is RepeatProgressiveCounter found ?? */
	PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = H264_DEFAULT_REPEATPROGRESSIVECOUNTER;

    PARSER_Data_p->PictureLocalData.PicStruct = H264_DEFAULT_PICSTRUCT;

    PARSER_Data_p->PictureLocalData.HasCtType = FALSE;
    PARSER_Data_p->PictureLocalData.CtType = H264_DEFAULT_CTTYPE;

#ifdef ST_XVP_ENABLE_FGT
    PARSER_Data_p->FilmGrainSpecificData.IsFilmGrainEnabled = FALSE;
#endif
}

#ifdef ST_XVP_ENABLE_FGT
/******************************************************************************/
/*! \brief Reset film grain parameters
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ResetFgtParams(const PARSER_Handle_t  ParserHandle)
{
	H264ParserPrivateData_t    *PARSER_Data_p;
	U32                        BackupFgtMessageId;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    BackupFgtMessageId = PARSER_Data_p->FilmGrainSpecificData.FgtMessageId;
    /*clear all parameters*/
    memset(&PARSER_Data_p->FilmGrainSpecificData, 0, sizeof(PARSER_Data_p->FilmGrainSpecificData));
    PARSER_Data_p->FilmGrainSpecificData.IsFilmGrainEnabled = FALSE;
    PARSER_Data_p->FilmGrainSpecificData.FgtMessageId       = BackupFgtMessageId;
}
#endif /* ST_XVP_ENABLE_FGT */

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseBufferingPeriod(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;
    U8  SeqParameterSetId;
    U32 SchedSelIdx;
    /* TODO : keep the appropriate InitialCpbRemovalDelay & InitialCpbRemovalDelayOffset from all possible SchedSelIdx values */
    U8  InitialCpbRemovalDelayLength;
    U32 InitialCpbRemovalDelay;
    U32 InitialCpbRemovalDelayTemp;
    U32 InitialCpbRemovalDelayOffset;
    UNUSED_PARAMETER(PayLoadSize);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Buffering Period");
#endif
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    SeqParameterSetId = h264par_GetUnsignedExpGolomb(ParserHandle);
    if ((SeqParameterSetId > (MAX_SPS-1)) ||(PARSER_Data_p->SPSLocalData[SeqParameterSetId].IsAvailable == FALSE))
    {
        PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
        DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NoSPSAvailableError,
        "parse_slice.c: (ParseSliceHeader) warning: no SPS available for this picture\n");
    /* We are missing the SPS for this picture. Thus it cannot be a valid picture
        * No need to parse the slice header
        */
        return;
    }
#ifdef SEI_PARSING_VERBOSE
        STTBX_Print(("SEI Buffering Period SeqParameterSetId = %d", SeqParameterSetId));
#endif /* SEI_PARSING_VERBOSE */

    InitialCpbRemovalDelay = 0;
    InitialCpbRemovalDelayTemp = 0;
    InitialCpbRemovalDelayOffset = 0;


    /* Get the InitialCpbRemovalDelay from nal_hrd */
    if (PARSER_Data_p->SPSLocalData[SeqParameterSetId].NalHrdBpPresentFlag)
    {
        InitialCpbRemovalDelayLength = PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelayLength;
        for (SchedSelIdx = 0; SchedSelIdx <= PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_cpb_cnt_minus1; SchedSelIdx++)
        {
            /* TODO: check if OK: we keep smallest value for now */
            InitialCpbRemovalDelayTemp = h264par_GetUnsigned(InitialCpbRemovalDelayLength, ParserHandle);
            InitialCpbRemovalDelayOffset = h264par_GetUnsigned(InitialCpbRemovalDelayLength, ParserHandle);
            if ((InitialCpbRemovalDelayTemp != 0) && (InitialCpbRemovalDelayTemp < InitialCpbRemovalDelay))
            {
                InitialCpbRemovalDelay = InitialCpbRemovalDelayTemp;
            }
        }
#ifdef SEI_PARSING_VERBOSE
            STTBX_Print(("NalHrdBpPresentFlag nal_cpb_cnt_minus1=%d InitialCpbRemovalDelay=%d InitialCpbRemovalDelayOffset=%d",
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_cpb_cnt_minus1,
                InitialCpbRemovalDelay, InitialCpbRemovalDelayOffset));
#endif /* SEI_PARSING_VERBOSE */

        PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelay= InitialCpbRemovalDelay;
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].IsInitialCpbRemovalDelayValid= TRUE;

    }


    if (PARSER_Data_p->SPSLocalData[SeqParameterSetId].VclHrdBpPresentFlag)
    {
        InitialCpbRemovalDelayLength = PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelayLength;
        for (SchedSelIdx = 0; SchedSelIdx <= PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_cpb_cnt_minus1; SchedSelIdx++)
        {
            /* TODO: check if OK: we keep smallest value for now */
            InitialCpbRemovalDelayTemp = h264par_GetUnsigned(InitialCpbRemovalDelayLength, ParserHandle);
            InitialCpbRemovalDelayOffset = h264par_GetUnsigned(InitialCpbRemovalDelayLength, ParserHandle);
            if ((InitialCpbRemovalDelayTemp != 0) || (InitialCpbRemovalDelayTemp < InitialCpbRemovalDelay))
            {
                InitialCpbRemovalDelay = InitialCpbRemovalDelayTemp;
            }
        }

#ifdef SEI_PARSING_VERBOSE
            STTBX_Print(("VclHrdBpPresentFlag vcl_cpb_cnt_minus1=%d InitialCpbRemovalDelay=%d InitialCpbRemovalDelayOffset=%d",
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_cpb_cnt_minus1,
                InitialCpbRemovalDelay, InitialCpbRemovalDelayOffset));
#endif /* SEI_PARSING_VERBOSE */

        /* if we only have vcl hrd dat, use it... (TODO: check if *1.2 is needed ?) */
        if ( !PARSER_Data_p->SPSLocalData[SeqParameterSetId].NalHrdBpPresentFlag)
        {
            PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelay = (InitialCpbRemovalDelay * 12)/10;
            PARSER_Data_p->SPSLocalData[SeqParameterSetId].IsInitialCpbRemovalDelayValid = TRUE;
        }
    }
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParsePicTiming(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;
    U32 CpbRemovalDelay = 0;
    U32 DpbOutputDelay = 0;
    U32 PicStruct = 0;
    U32 NumClockTS = 0;
    U32 ClockTimestampFlag[3]; /* TODO : what to do when we have more than one Clock timestamp ?
                                * vid_com picture structure has one decode time sub structure only by picture */
    U32 CtType = 4;
    U32 nuit_field_based_flag = 0;
    U32 counting_type = 0;
    U32 full_timestamp_flag = 0;
    U32 discontinuity_flag = 0;
    U32 cnt_dropped_flag = 0;
    U32 n_frames = 0;
    U32 seconds_value = 0;
    U32 minutes_value = 0;
    U32 hours_value = 0;
    U32 seconds_flag = 0;
    U32 minutes_flag = 0;
    U32 hours_flag = 0;
    S32 time_offset = 0;
    U8 loopcounter;
    U32 TimeFactor;
    UNUSED_PARAMETER(PayLoadSize);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Picture Timing");
#endif

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if (PARSER_Data_p->ActiveSPSLocalData_p->CpbDpbDelaysPresentFlag)
    {
        CpbRemovalDelay = h264par_GetUnsigned(PARSER_Data_p->ActiveSPSLocalData_p->CpbRemovalDelayLength, ParserHandle);
        DpbOutputDelay = h264par_GetUnsigned(PARSER_Data_p->ActiveSPSLocalData_p->DpbOutputDelayLength, ParserHandle);
#ifdef SEI_PARSING_VERBOSE
        STTBX_Print(("\n\rCpbRemovalDelay=%d DpbOutputDelay=%d", CpbRemovalDelay, DpbOutputDelay));
#endif /* SEI_PARSING_VERBOSE */


        /* Tr (removal time of access unit from cpb) is not used currently, so for the reordering queue, the sole DpbOutputDelayTime should be sufficient. */
        /* ==> we don'tcompute the real presentationStartTime here! */

        /* Split computation to avoid a too big accuraccy loss on DpbOutputDelay result as we compute with integer division */
        TimeFactor = ST_GetClocksPerSecond() / 90;
        if (DpbOutputDelay >= ((U32)0xffffffff / TimeFactor))
        {
            PARSER_Data_p->PictureGenericData.PresentationStartTime = (STOS_Clock_t)((DpbOutputDelay / 1000)* TimeFactor);
        }
        else
        {
            PARSER_Data_p->PictureGenericData.PresentationStartTime = (STOS_Clock_t)((DpbOutputDelay * TimeFactor) / 1000);
        }
        PARSER_Data_p->PictureGenericData.IsPresentationStartTimeValid = TRUE;
    }

    if (PARSER_Data_p->ActiveSPSLocalData_p->pic_struct_present_flag)
    {
        PicStruct = h264par_GetUnsigned(4, ParserHandle);
        PARSER_Data_p->PictureLocalData.PicStruct = PicStruct;
        switch (PicStruct)
        {
            case PICSTRUCT_FRAME: /* frame   field_pic_flag shall be 0   NumClockTS =1*/
            case PICSTRUCT_TOP_FIELD: /* top field   field_pic_flag shall be 1, bottom_field_flag shall be 0 NumClockTS =1*/
                PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TRUE;
                PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
                PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = 0;
                NumClockTS = 1;
                break;
            case PICSTRUCT_BOTTOM_FIELD: /* bottom field    field_pic_flag shall be 1, bottom_field_flag shall be 1 NumClockTS =1*/
                PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = FALSE;
                PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
                PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = 0;
                NumClockTS = 1;
                break;
            case PICSTRUCT_TOP_BOTTOM: /* top field, bottom field, in that order  field_pic_flag shall be 0   NumClockTS =2*/
                PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TRUE;
                PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
                PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = 0;
                NumClockTS = 2;
                break;
            case PICSTRUCT_BOTTOM_TOP: /* bottom field, top field, in that order  field_pic_flag shall be 0   NumClockTS =2*/
                PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = FALSE;
                PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
                PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = 0;
                NumClockTS = 2;
                break;
            case PICSTRUCT_TOP_BOTTOM_TOP: /* top field, bottom field, top field repeated, in that order  field_pic_flag shall be 0   NumClockTS =3*/
                PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TRUE;
                PARSER_Data_p->PictureGenericData.RepeatFirstField = TRUE;
                PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = 0;
                NumClockTS = 3;
                break;
            case PICSTRUCT_BOTTOM_TOP_BOTTOM: /* bottom field, top field, bottom field repeated, in that order   field_pic_flag shall be 0   NumClockTS =3*/
                PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = FALSE;
                PARSER_Data_p->PictureGenericData.RepeatFirstField = TRUE;
                PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = 0;
                NumClockTS = 3;
                break;
            case PICSTRUCT_FRAME_DOUBLING: /* frame doubling  field_pic_flag shall be 0fixed_frame_rate_flag shall be 1   NumClockTS =2 */
                PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TRUE;
                PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
                PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = 1;
                NumClockTS = 2;
                break;
            case PICSTRUCT_FRAME_TRIPLING: /* frame tripling  field_pic_flag shall be 0fixed_frame_rate_flag shall be 1   NumClockTS =3 */
                PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TRUE;
                PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
                PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter = 2;
                NumClockTS = 3;
                break;
            default:
                /* Found a reserved value */
                NumClockTS = 1;
                PARSER_Data_p->PictureLocalData.PicStruct = H264_DEFAULT_PICSTRUCT;
                DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SEIError.ReservedPictStructSEIMessageError,
                "parse_sei.c: (SEIPayLoad) error: Reserved pic_struct value encountered in SEI message\n");
                break;
        }
#ifdef SEI_PARSING_VERBOSE
            /* just for debug logging ... */
            if (!PARSER_Data_p->ActiveSPSLocalData_p->CpbDpbDelaysPresentFlag)
            {
                STTBX_Print(("\n\r"));
            }
            STTBX_Print((" PicStruct=%d NumClockTS=%d TFF=%d RFF=%d RPC=%d ", PicStruct, NumClockTS,
                    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst,
                    PARSER_Data_p->PictureGenericData.RepeatFirstField,
                    PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter));
#endif /* SEI_PARSING_VERBOSE */
        for (loopcounter = 0; loopcounter < NumClockTS ; loopcounter++)
        {
            ClockTimestampFlag[loopcounter] = h264par_GetUnsigned(1, ParserHandle);
            if (ClockTimestampFlag[loopcounter] == 1)
            {
                CtType = h264par_GetUnsigned(2, ParserHandle);
                PARSER_Data_p->PictureLocalData.HasCtType = TRUE;
                PARSER_Data_p->PictureLocalData.CtType = CtType;

                /* TODO, check scan type is consistent between subsequent ClockTimestampFlag[i] for the same picture */
                nuit_field_based_flag = h264par_GetUnsigned(1, ParserHandle);
                counting_type         = h264par_GetUnsigned(5, ParserHandle);
                full_timestamp_flag   = h264par_GetUnsigned(1, ParserHandle);
                discontinuity_flag    = h264par_GetUnsigned(1, ParserHandle);
                cnt_dropped_flag      = h264par_GetUnsigned(1, ParserHandle);
                n_frames              = h264par_GetUnsigned(8, ParserHandle);

                if( full_timestamp_flag == 1)
                {
                    seconds_value = h264par_GetUnsigned(6, ParserHandle); /* 0..59 */
                    minutes_value = h264par_GetUnsigned(6, ParserHandle); /* 0..59 */
                    hours_value   = h264par_GetUnsigned(5, ParserHandle); /* 0..23 */
                }
                else
                {
                    /* if seconds_flag if <> 1 the previous seconds_value in decoding order shall be used as sS to compute clockTimestamp */
                    /* if minutes_flag if <> 1 the previous minutes_value in decoding order shall be used as mM to compute clockTimestamp */
                    /* if hours_flag if <> 1 the previous hours_value in decoding order shall be used as hH to compute clockTimestamp */
                    seconds_flag = h264par_GetUnsigned(1, ParserHandle);
                    if( seconds_flag == 1 )
                    {
                        seconds_value = h264par_GetUnsigned(6, ParserHandle); /* range 0..59 */
                        minutes_flag = h264par_GetUnsigned(1, ParserHandle);
                        if( minutes_flag == 1 )
                        {
                            minutes_value = h264par_GetUnsigned(6, ParserHandle); /* 0..59 */
                            hours_flag = h264par_GetUnsigned(1, ParserHandle);
                            if( hours_flag == 1 )
                            {
                                hours_value = h264par_GetUnsigned(5, ParserHandle); /* 0..23 */
                            }
                        }
                    }
                }
                if (PARSER_Data_p->ActiveSPSLocalData_p->TimeOffsetLength > 0)
                {
                    time_offset = h264par_GetSigned(PARSER_Data_p->ActiveSPSLocalData_p->TimeOffsetLength, ParserHandle);
                }
                else
                {
                    time_offset = 0;
                }
                /* TODO : implement clockTimestamp computation and check (clockTimestamp must be increasing) */
            }
        }
    }

}


/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParsePanScanRect(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    H264ParserPrivateData_t * PARSER_Data_p;
    U32 pan_scan_rect_id;                                     /* useless ? */
    U32 pan_scan_rect_cancel_flag;                     /* cancel the previous P&S values */
    U32 pan_scan_cnt_minus1;                              /* number of P&S rectangle present */
    S32 pan_scan_rect_left_offset;                       /* offset relative to the luma sampling grid, in 1/16th pixel */
    S32 pan_scan_rect_right_offset;                     /* same */
    S32 pan_scan_rect_top_offset;                       /* same */
    S32 pan_scan_rect_bottom_offset;                 /* same */
    U32 i;
    UNUSED_PARAMETER(PayLoadSize);

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Pan Scan");
#endif

    pan_scan_rect_id = h264par_GetUnsignedExpGolomb(ParserHandle);
    pan_scan_rect_cancel_flag = h264par_GetUnsigned(1, ParserHandle);
    if (pan_scan_rect_cancel_flag)
    {
        /* this cancels all previous PanScan SEI messages in output order */
        for(i=0; i<VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; i++)
        {
            PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation = FALSE;
        }
    }
    else
    {
        PARSER_Data_p->PictureGenericData.NumberOfPanAndScan = 0;
        if ((pan_scan_cnt_minus1 = h264par_GetUnsignedExpGolomb(ParserHandle)) != UNSIGNED_EXPGOLOMB_FAILED)
        {
            PARSER_Data_p->PictureGenericData.NumberOfPanAndScan = pan_scan_cnt_minus1+1;
        }

        for( i=0; (i<PARSER_Data_p->PictureGenericData.NumberOfPanAndScan) && (i<VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN) ; i++)
        {
            pan_scan_rect_left_offset = h264par_GetSignedExpGolomb(ParserHandle);
            pan_scan_rect_right_offset = h264par_GetSignedExpGolomb(ParserHandle);
            pan_scan_rect_top_offset = h264par_GetSignedExpGolomb(ParserHandle);
            pan_scan_rect_bottom_offset= h264par_GetSignedExpGolomb(ParserHandle);


            /* In h264, the pan and scan offsets are defined from the top-left corner of the frame to the
				   top-left corner of the pan and scan window; but the producer expects an
				   offset from the center of the picture with the center being 0, 0.*/

            PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreHorizontalOffset =
                pan_scan_rect_left_offset  +                                                                           /* PanScan H offset */
                ((pan_scan_rect_right_offset - pan_scan_rect_left_offset)/2) -                          /* PanScan width/2 */
                (PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width*8);      /* display center in 16th pixel (*16/2) */

            PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreVerticalOffset =
                pan_scan_rect_top_offset+                                                                                 /* PanScan V offset */
                ((pan_scan_rect_bottom_offset - pan_scan_rect_top_offset)/2) -                          /* PanScan height/2 */
                (PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height*8);        /* display center in 16th pixel (*16/2) */

            PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayHorizontalSize =
                (pan_scan_rect_right_offset - pan_scan_rect_left_offset)/2;

            PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayVerticalSize =
                (pan_scan_rect_bottom_offset - pan_scan_rect_top_offset)/2;

            PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation = TRUE;
        }

        /* Set all other unused elements of PanAndScanIn16thPixel to FALSE */
        for( i=PARSER_Data_p->PictureGenericData.NumberOfPanAndScan; i<VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN ; i++)
        {
            PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation = FALSE;
        }

        if ((PARSER_Data_p->PictureGenericData.PanScanRectRepetitionPeriod = h264par_GetUnsignedExpGolomb(ParserHandle)) == UNSIGNED_EXPGOLOMB_FAILED)
        {
            PARSER_Data_p->PictureGenericData.PanScanRectRepetitionPeriod = 0;
        }
        PARSER_Data_p->PictureGenericData.PanScanRectPersistence = 0;

    }

}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseFillerPayLoad(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Filler Payload");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseUserDataRegisteredITUTT35(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t * PARSER_Properties_p = (PARSER_Properties_t *)ParserHandle;
    H264ParserPrivateData_t * PARSER_Data_p = (H264ParserPrivateData_t *)(PARSER_Properties_p->PrivateData_p);
    U32 LoopIndex;
    U8 * UDBuffer;


#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("User Data Registered");
#endif

    /* check if we can safely read the 2 first bytes */
    if (PayLoadSize < 3)
        return;         /* no, the UserData is empty.. */

    UDBuffer = (U8*)PARSER_Data_p->UserData.Buff_p;

    /* This is a Registered Data */
    /* => the fields itu_t_t35_xx are valid */
    PARSER_Data_p->UserData.IsRegistered = TRUE;

    /* read itu_t_35_country_code and itu_t_t35_country_code_extension_byte if any */
    LoopIndex = 0;
    PARSER_Data_p->UserData.itu_t_t35_country_code = (U8) h264par_GetUnsigned(8, ParserHandle);
    if ( PARSER_Data_p->UserData.itu_t_t35_country_code != 0xFF)
    {
          PARSER_Data_p->UserData.itu_t_t35_country_code_extension_byte = 0;
          LoopIndex = 1;
    }
    else
    {
          PARSER_Data_p->UserData.itu_t_t35_country_code_extension_byte = (U8) h264par_GetUnsigned(8, ParserHandle);
          LoopIndex = 2;
    }

    /* read the itu_t_t35_provider_code */
    PARSER_Data_p->UserData.itu_t_t35_provider_code = (U16)h264par_GetUnsigned(16, ParserHandle);
    LoopIndex++;
    LoopIndex++;

    /*Copy all the UserDataRegistered_ITU_T_35 payload into UserData->Buff_p*/
    while ( (LoopIndex < PayLoadSize) && (LoopIndex < PARSER_Data_p->UserDataSize) && (!PARSER_Data_p->NALByteStream.EndOfStreamFlag) )
    {
        *UDBuffer = (U8) h264par_GetUnsigned(8, ParserHandle);
        UDBuffer++;
        LoopIndex++;
    }

    /* If there was a problem while reading the user data, set the buffer overflow field of the userdata */
    if ((PayLoadSize > PARSER_Data_p->UserDataSize) ||
         PARSER_Data_p->NALByteStream.EndOfStreamFlag)
    {
        PARSER_Data_p->UserData.BufferOverflow = TRUE;
    }
    else
    {
        PARSER_Data_p->UserData.BufferOverflow = FALSE;
    }

    PARSER_Data_p->UserData.Length = LoopIndex;
    PARSER_Data_p->UserData.BroadcastProfile = PARSER_Data_p->ParserGlobalVariable.Broadcast_profile;
    PARSER_Data_p->UserData.PositionInStream = STVID_USER_DATA_AFTER_PICTURE;
    STOS_memcpy (&PARSER_Data_p->UserData.PTS, &PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS, sizeof(STVID_PTS_t));
    PARSER_Data_p->UserData.pTemporalReference = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID;



    if (PARSER_Data_p->UserData.Length > 0)
    {
      STEVT_Notify (  PARSER_Properties_p->EventsHandle,
                                PARSER_Data_p->RegisteredEventsID[PARSER_USER_DATA_EVT_ID],
                                STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->UserData));
    }
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseDataUnregistered(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t * PARSER_Properties_p = (PARSER_Properties_t *)ParserHandle;
    H264ParserPrivateData_t * PARSER_Data_p = (H264ParserPrivateData_t *)(PARSER_Properties_p->PrivateData_p);
    U32 LoopIndex;
    U8 * UDBuffer;

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("User Data Unregistered");
#endif

    UDBuffer = (U8*)PARSER_Data_p->UserData.Buff_p;

    /* fill the uuid field */
    LoopIndex=0;

    /* Copy all the UserDataUnRegistered payload into UserData->Buff_p
    including the uuid code (16 bytes) */
    while ( (LoopIndex < PayLoadSize) && (LoopIndex < PARSER_Data_p->UserDataSize) && (!PARSER_Data_p->NALByteStream.EndOfStreamFlag) )
    {
        *UDBuffer = (U8) h264par_GetUnsigned(8, ParserHandle);
        UDBuffer++;
        LoopIndex++;
    }

    /* If there was a problem while reading the user data, set the buffer overflow field of the userdata */
    if ((PayLoadSize > PARSER_Data_p->UserDataSize) ||
         PARSER_Data_p->NALByteStream.EndOfStreamFlag )
    {
        PARSER_Data_p->UserData.BufferOverflow = TRUE;
    }
    else
    {
        PARSER_Data_p->UserData.BufferOverflow = FALSE;
    }
    PARSER_Data_p->UserData.IsRegistered = FALSE;
    PARSER_Data_p->UserData.Length = LoopIndex;
    PARSER_Data_p->UserData.BroadcastProfile = PARSER_Data_p->ParserGlobalVariable.Broadcast_profile;
    PARSER_Data_p->UserData.PositionInStream = STVID_USER_DATA_AFTER_PICTURE;
    STOS_memcpy (&PARSER_Data_p->UserData.PTS, &PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS, sizeof(STVID_PTS_t));
    PARSER_Data_p->UserData.pTemporalReference = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID;

    PARSER_Data_p->UserData.itu_t_t35_country_code = 0;
    PARSER_Data_p->UserData.itu_t_t35_country_code_extension_byte = 0;
    PARSER_Data_p->UserData.itu_t_t35_provider_code = 0;

    if (PARSER_Data_p->UserData.Length > 0)
    {
      STEVT_Notify (  PARSER_Properties_p->EventsHandle,
                                PARSER_Data_p->RegisteredEventsID[PARSER_USER_DATA_EVT_ID],
                                STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->UserData));
    }
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseRecoveryPoint(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;
	U8 ChangingSliceGroupIdc;
    UNUSED_PARAMETER(PayLoadSize);

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Recovery Point");
#endif

	if (!PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointHasBeenHit)
	{
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointState = RECOVERY_PENDING;
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryFrameCnt = h264par_GetUnsignedExpGolomb(ParserHandle);
		/* TODO: management of ExactMatchFlag */
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.ExactMatchFlag = h264par_GetUnsigned(1, ParserHandle);

		/* if set by the SEI message, BrokenLinkFlag stays active until the recovery point is reached.
	 	* Between the SEI message that sets BrokenLinkFlag and the recovery point, all reference pictures are
	 	* tagged as BrokenLink
	 	*/
		PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.BrokenLinkFlag = h264par_GetUnsigned(1, ParserHandle);

		ChangingSliceGroupIdc = h264par_GetUnsigned(2, ParserHandle);
		if (ChangingSliceGroupIdc != 0)
		{
			/* TODO: ERROR in SEI: not allowed in main profile */
		}
	}
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseDecRefPicMarkingRepetitionPeriod(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Dec Ref Pic Marking Repetition Period");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseSparePic(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Spare Picture");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseSceneInfo(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Scene Info");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseSubSeqInfo(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Sub Seq info");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseSubSeqLayerCharacteristics(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Sub Seq Layer Characteristics");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseSubSeqCharacteristics(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Sub Seq Characteristics");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseFullFrameFreeze(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Full Frame Freeze");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseFullFrameFreezeRelease(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Full Frame Freeze Release");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseFullFrameSnapshot(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Full Frame Snapshot");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseProgressiveRefinementSegmentStart(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Progressive Refinement Segment Start");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseProgressiveRefinementSegmentEnd(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Progressive Refinement Segment Stop");
#endif

}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseMotionConstrainedSliceGroupSet(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Motion Constrained Slice Group Set");
#endif
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseFilmGrainCharacteristics(const PARSER_Handle_t ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;
#ifdef ST_XVP_ENABLE_FGT
    U32         filmgrain_cancel;
    U32         separate_colour_description_present;
    U32         comp_model[3];
    int         c,i,j;
    U32         model_id;
#endif /* ST_XVP_ENABLE_FGT */

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Film Grain Characteristics");
#endif
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#ifdef ST_XVP_ENABLE_FGT
    filmgrain_cancel = h264par_GetUnsigned(1, ParserHandle);

    PARSER_Data_p->FilmGrainSpecificData.FgtMessageId++;
    if (filmgrain_cancel == 0)
    {
        PARSER_Data_p->FilmGrainSpecificData.IsFilmGrainEnabled = TRUE;

        model_id = h264par_GetUnsigned(2, ParserHandle);
        switch (model_id)
        {
        case 0: /* Identifies the film grain simulation mode as frequency filtering. */
            separate_colour_description_present = h264par_GetUnsigned(1, ParserHandle);
            if (separate_colour_description_present == 0)
            {
                PARSER_Data_p->FilmGrainSpecificData.blending_mode_id = h264par_GetUnsigned(2, ParserHandle);
                /* TBD : blending mode id shall be equal to 0 which corresponds to the additive blendings mode */
                PARSER_Data_p->FilmGrainSpecificData.log2_scale_factor = h264par_GetUnsigned(4, ParserHandle);
                comp_model[0] = h264par_GetUnsigned(1, ParserHandle);
                comp_model[1] = h264par_GetUnsigned(1, ParserHandle);
                comp_model[2] = h264par_GetUnsigned(1, ParserHandle);
                for (c = 0; c < 3; c++)
                {
                    if (comp_model[c] == 1)
                    {
                        PARSER_Data_p->FilmGrainSpecificData.comp_model_present_flag[c] = TRUE;
                        PARSER_Data_p->FilmGrainSpecificData.num_intensity_intervals_minus1[c] = h264par_GetUnsigned(8, ParserHandle);
                        PARSER_Data_p->FilmGrainSpecificData.num_model_values_minus1[c] = h264par_GetUnsigned(3, ParserHandle);

                        if (    (PARSER_Data_p->FilmGrainSpecificData.num_intensity_intervals_minus1[c] > 256) ||
                                (PARSER_Data_p->FilmGrainSpecificData.num_model_values_minus1[c] > 3))
                        {
                            /* prevent bad parsing values */
                            PARSER_Data_p->FilmGrainSpecificData.IsFilmGrainEnabled = FALSE;
                            break;
                        }

                        for (   i = 0;
                                i <= PARSER_Data_p->FilmGrainSpecificData.num_intensity_intervals_minus1[c];
                                i++)
                        {
                            PARSER_Data_p->FilmGrainSpecificData.intensity_interval_lower_bound[c][i] = h264par_GetUnsigned(8, ParserHandle);
                            PARSER_Data_p->FilmGrainSpecificData.intensity_interval_upper_bound[c][i] = h264par_GetUnsigned(8, ParserHandle);
                            for (   j = 0;
                                    j <= PARSER_Data_p->FilmGrainSpecificData.num_model_values_minus1[c];
                                    j++)
                            {
                                PARSER_Data_p->FilmGrainSpecificData.comp_model_value[c][i][j] = h264par_GetSignedExpGolomb(ParserHandle);
                            }
                        }
                    }
                    else
                    {
                        PARSER_Data_p->FilmGrainSpecificData.comp_model_present_flag[c] = FALSE;
                    }
                }
                PARSER_Data_p->FilmGrainSpecificData.film_grain_characteristics_repetition_period = h264par_GetUnsignedExpGolomb(ParserHandle);
            }
            break;

        case 1: /* auto regression film grain simulation : feature not supported */
            break;

        default: /* reserved value for model Id */
            break;
        }
    }
    else
    {
        h264par_ResetFgtParams(ParserHandle);
    }

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpFgtCharacterestics(ParserHandle);
#endif
#endif /* ST_XVP_ENABLE_FGT */
}

/******************************************************************************/
/*! \brief Parses The eponym SEI message.
 *
 *
 * \param PayLoadSize SEI message Size in Byte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseReservedSeiMessage(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(PayLoadSize);
    UNUSED_PARAMETER(ParserHandle);
#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIContentHeader("Reserved SEI Message");
#endif

}

/******************************************************************************/
/*! \brief Parses a SEI message and forward to each SEI extraction functions.
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_SEIPayLoad(U32 PayLoadType, U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	switch (PayLoadType)
	{
		case SEI_BUFFERING_PERIOD:
			h264par_ParseBufferingPeriod(PayLoadSize, ParserHandle);
			break;
  		case SEI_PIC_TIMING:
			h264par_ParsePicTiming(PayLoadSize, ParserHandle);
			break;
  		case SEI_PAN_SCAN_RECT:
			h264par_ParsePanScanRect(PayLoadSize, ParserHandle);
			break;
  		case SEI_FILLER_PAYLOAD:
			h264par_ParseFillerPayLoad(PayLoadSize, ParserHandle);
			break;
  		case SEI_USER_DATA_REGISTERED_ITU_T_T35:
            h264par_PushUserDataItem(PayLoadSize, PayLoadType, ParserHandle);
			break;
  		case SEI_USER_DATA_UNREGISTERED:
            h264par_PushUserDataItem(PayLoadSize, PayLoadType, ParserHandle);
			break;
  		case SEI_RECOVERY_POINT:
			h264par_ParseRecoveryPoint(PayLoadSize, ParserHandle);
			break;
  		case SEI_DEC_REF_PIC_MARKING_REPETITION:
			h264par_ParseDecRefPicMarkingRepetitionPeriod(PayLoadSize, ParserHandle);
			break;
  		case SEI_SPARE_PIC:
			h264par_ParseSparePic(PayLoadSize, ParserHandle);
			break;
  		case SEI_SCENE_INFO:
			h264par_ParseSceneInfo(PayLoadSize, ParserHandle);
			break;
  		case SEI_SUB_SEQ_INFO:
			h264par_ParseSubSeqInfo(PayLoadSize, ParserHandle);
			break;
  		case SEI_SUB_SEQ_LAYER_CHARACTERISTICS:
			h264par_ParseSubSeqLayerCharacteristics(PayLoadSize, ParserHandle);
			break;
  		case SEI_SUB_SEQ_CHARACTERISTICS:
			h264par_ParseSubSeqCharacteristics(PayLoadSize, ParserHandle);
			break;
  		case SEI_FULL_FRAME_FREEZE:
			h264par_ParseFullFrameFreeze(PayLoadSize, ParserHandle);
			break;
  		case SEI_FULL_FRAME_FREEZE_RELEASE:
			h264par_ParseFullFrameFreezeRelease(PayLoadSize, ParserHandle);
			break;
  		case SEI_FULL_FRAME_SNAPSHOT:
			h264par_ParseFullFrameSnapshot(PayLoadSize, ParserHandle);
			break;
  		case SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START:
			h264par_ParseProgressiveRefinementSegmentStart(PayLoadSize, ParserHandle);
			break;
  		case SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END:
			h264par_ParseProgressiveRefinementSegmentEnd(PayLoadSize, ParserHandle);
			break;
  		case SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET:
			h264par_ParseMotionConstrainedSliceGroupSet(PayLoadSize, ParserHandle);
			break;
  		case SEI_FILM_GRAIN_CHARACTERISTICS:
			h264par_ParseFilmGrainCharacteristics(ParserHandle);
			break;
  		case SEI_RESERVED_SEI_MESSAGE:
		default:
			h264par_ParseReservedSeiMessage(PayLoadSize, ParserHandle);
			break;
	}
}

/******************************************************************************/
/*! \brief Move to the next SEI message by applying successive read bytes
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_MoveToNextSEIMessage(U32 PayLoadSize, const PARSER_Handle_t ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
	U32 LoopIndex;

	/* Restart from the beginning of NAL */
    h264par_RestorePositionInNAL(ParserHandle, PARSER_Data_p->SEIMessagePosition);

	/* At that stage, we shall be byte aligned */

	/* Then, move by PayLoadSize byte forward */
	for (LoopIndex = 0; LoopIndex < PayLoadSize; LoopIndex++)
	{
		h264par_GetUnsigned(8, ParserHandle);
	}
}

/******************************************************************************/
/*! \brief Parses a SEI header.
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseSEI(const PARSER_Handle_t ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;
	U32 PayLoadType;
	U8 LastPayLoadTypeByte;
	U32 PayLoadSize;
	U8 LastPayLoadSizeByte;
	U8 NextByte;
	BOOL IsEndOfSEIMessage;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	IsEndOfSEIMessage = FALSE;

	/* h264par_SetDefaultSEI(ParserHandle); */

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIPayLoadStart(ParserHandle);
#endif

	do
	{
		PayLoadType = 0;
        while (((LastPayLoadTypeByte = h264par_GetUnsigned(8, ParserHandle)) == 0xff) && (!PARSER_Data_p->NALByteStream.EndOfStreamFlag))
		{
			PayLoadType += 255;
		}

		PayLoadType += LastPayLoadTypeByte;

		PayLoadSize = 0;
        while (((LastPayLoadSizeByte = h264par_GetUnsigned(8, ParserHandle)) == 0xff) && (!PARSER_Data_p->NALByteStream.EndOfStreamFlag))
		{
			PayLoadSize += 255;
		}
		PayLoadSize += LastPayLoadSizeByte;

        if (!PARSER_Data_p->NALByteStream.EndOfStreamFlag)
        {
            h264par_SavePositionInNAL(ParserHandle, &(PARSER_Data_p->SEIMessagePosition));
            h264par_SEIPayLoad(PayLoadType, PayLoadSize, ParserHandle);
            h264par_MoveToNextSEIMessage(PayLoadSize, ParserHandle);
            h264par_SavePositionInNAL(ParserHandle, &(PARSER_Data_p->SEIMessagePosition));
            NextByte = h264par_GetUnsigned(8, ParserHandle);
            h264par_RestorePositionInNAL(ParserHandle, PARSER_Data_p->SEIMessagePosition);
            if (NextByte == 0x80) /* Stop bit + stuffing */
            {
                IsEndOfSEIMessage = TRUE;
            }
        }
        if (PARSER_Data_p->NALByteStream.EndOfStreamFlag)
        {
			IsEndOfSEIMessage = TRUE;
            PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->PictureSpecificData.PictureError.SEIError.TruncatedNAL,
            "parse_sei.c: (SEIPayLoad) error: Truncated NAL\n");
        }

  	} while( !IsEndOfSEIMessage);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSEIPayLoadStop();
#endif

}

/******************************************************************************/
/*! \brief Parses a AUD and fills data structures.
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseAUD (const PARSER_Handle_t  ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->PictureLocalData.PrimaryPicType = h264par_GetUnsigned(3, ParserHandle);

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpAUD(ParserHandle);
#endif

}

