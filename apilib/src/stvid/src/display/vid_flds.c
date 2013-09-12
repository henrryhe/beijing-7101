/*******************************************************************************

File name   : vid_flds.c

Description : Video display source file. Fields counters algorithms.

   viddisp_DecrementFieldCounterBackward
   viddisp_DecrementFieldCounterForward

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
 02 Sep 2002        Created                                   Michel Bruant
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "vid_disp.h"
#include "fields.h"

/* Traces : ----------------------------------------------------------------- */

#ifdef TRACE_FIELDS /* define or uncomment in ./makefile to set traces */
  #ifdef TRACE_FIELDS_VERBOSE
    #define TraceVerbose(x) TraceBuffer(x)
  #else
    #define TraceVerbose(x)
  #endif
  #include "vid_trc.h"
#else
  #define TraceInfo(x)
  #define TraceBuffer(x)
  #define TraceVerbose(x)
#endif

/*******************************************************************************
Name        : viddisp_DecrementFieldCounterBackward
Description : Decrement field counter depending on display, and give info
                for display of next VSync:
                    - NextFieldOnDisplay_p is VIDDISP_FIELD_DISPLAY_NONE
                    if display of the picture is finished on next VSync
                    - else NextFieldOnDisplay_p indicates the next field
                    of the picture that should be displayed next
Parameters  :
Assumptions : Not all counters of the picture are 0 !
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if can't decrement
*******************************************************************************/
ST_ErrorCode_t viddisp_DecrementFieldCounterBackward(
                            VIDDISP_Picture_t * const CurrentPicture_p,
                            VIDDISP_FieldDisplay_t * const NextFieldOnDisplay_p,
                            U32 Index)
{
    STVID_PictureInfos_t *  CurrentPictureInfos_p;
    BOOL                    RepeatProgressive = FALSE;
    BOOL                    ReDecrement = FALSE;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    U32                     SaveTopFieldCounter, SaveBottomFieldCounter;

    /* Shortcut */
    CurrentPictureInfos_p = &(CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos);

    /* Suppose display won't change */
    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_SAME;

    SaveTopFieldCounter = CurrentPicture_p->Counters[Index].TopFieldCounter;
    SaveBottomFieldCounter = CurrentPicture_p->Counters[Index].BottomFieldCounter;

    /* How counters must be decremented: (R=repeat T=top B=bottom)
        - progressive: R-B-T, repeated RepeatProgressiveCounter times
        - interlaced: R-B-T if TopFieldFirst
                      R-T-B if !TopFieldFirst */
    if (CurrentPictureInfos_p->VideoParams.ScanType
            == STGXOBJ_PROGRESSIVE_SCAN)
    {
        /* Playing backward progressive */
        if (CurrentPicture_p->Counters[Index].RepeatFieldCounter != 0)
        {
            /* Repeat field was first, decrement repeat counter */
            CurrentPicture_p->Counters[Index].RepeatFieldCounter--;
            if (CurrentPicture_p->Counters[Index].RepeatFieldCounter == 0)
            {
                /* Last repeat field displayed: display top or bottom now */
                /* (the opposite to TopFieldFirst) */
                if ((CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
                && ((CurrentPictureInfos_p->VideoParams.TopFieldFirst)
                ||  (CurrentPicture_p->Counters[Index].TopFieldCounter == 0)) )
                {
                    /* Display bottom next */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_BOTTOM;
                }
                else if ((CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
                 && (!(CurrentPictureInfos_p->VideoParams.TopFieldFirst)
                  ||(CurrentPicture_p->Counters[Index].BottomFieldCounter == 0)))
                {
                    /* Display top next */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_TOP;
                }
                else
                {
                    /* All field counters are 0: display again if required */
                    /* or launch display of next picture */
                    if (CurrentPicture_p->Counters[Index].RepeatProgressiveCounter != 0)
                    {
                        RepeatProgressive = TRUE;
                    }
                    else
                    {
                        /* Display is finished: put next picte to the screen */
                        *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                    }
                }
            }
        }
        else if ((CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
             && ((CurrentPictureInfos_p->VideoParams.TopFieldFirst)
             ||  (CurrentPicture_p->Counters[Index].TopFieldCounter == 0)))
        {
            /* Top first or top counter is 0, decrement bottom counter */
            CurrentPicture_p->Counters[Index].BottomFieldCounter--;
            if (CurrentPicture_p->Counters[Index].BottomFieldCounter == 0)
            {
                /* Last bottom field displayed: display top now, or next */
                /* picture if no top */
                if (CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
                {
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_TOP;
                }
                else
                {
                    /* All field counters are 0: display again if required */
                    /* or launch display of next picture */
                    if (CurrentPicture_p->Counters[Index].
                                                RepeatProgressiveCounter != 0)
                    {
                        RepeatProgressive = TRUE;
                    }
                    else
                    {
                        /* Display is finished: put next pict to the screen */
                        *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                    }
                }
            }
        }
        else if ((CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
            && ((!(CurrentPictureInfos_p->VideoParams.TopFieldFirst))
              ||  (CurrentPicture_p->Counters[Index].BottomFieldCounter == 0)))
        {
            /* Bottom first or bottom counter is 0, decrement top counter */
            CurrentPicture_p->Counters[Index].TopFieldCounter--;
            if (CurrentPicture_p->Counters[Index].TopFieldCounter == 0)
            {
                /* Last top field displayed: display bottom now, or repeat, */
                /* or next picture if no bottom nor repeat */
                if (CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
                {
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_BOTTOM;
                }
                else
                {
                    /* All field counters are 0: display again if required */
                    /* or launch display of next picture */
                    if (CurrentPicture_p->Counters[Index].RepeatProgressiveCounter != 0)
                    {
                        RepeatProgressive = TRUE;
                    }
                    else
                    {
                        /* is finished: put next pict to the screen */
                        *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                    }
                }
            }
        }
        else if (CurrentPicture_p->Counters[Index].RepeatProgressiveCounter != 0)
        {
            /* Should not happen: field counter should not be 0 with */
            /* RepeatProgressiveCounter not 0 ! */
            RepeatProgressive = TRUE;
            ReDecrement = TRUE;
        }
        else
        {
            /* Debug: Should not happen: case 0,0 treated before by pop of */
            /* the frame */
            TraceBuffer(("Can't have all counters 0 !\r\n"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        /* Playing backward interlaced */
        if (CurrentPicture_p->Counters[Index].RepeatFieldCounter != 0)
        {
            /* Repeat field was first, decrement repeat counter */
            CurrentPicture_p->Counters[Index].RepeatFieldCounter--;
            if (CurrentPicture_p->Counters[Index].RepeatFieldCounter == 0)
            {
                /* Last repeat field displayed: display top or bottom now */
                /* (the opposite to TopFieldFirst) */
                if ((CurrentPictureInfos_p->VideoParams.TopFieldFirst)
                  &&(CurrentPicture_p->Counters[Index].BottomFieldCounter != 0))
                {
                    /* Display bottom (second) next */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_BOTTOM;
                }
                else if ((
                    !(CurrentPictureInfos_p->VideoParams.TopFieldFirst))
                  && (CurrentPicture_p->Counters[Index].TopFieldCounter != 0))
                {
                    /* Display top (second) next */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_TOP;
                }
                else
                {
                    if (CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
                    {
                        /* Should display top (first) next */
                        *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_TOP;
                    }
                    else if (CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
                    {
                        *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_BOTTOM;
                    }
                    else
                    {
                        /* All field counters are 0: launch display of next */
                        /* picture */
                        *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                    }
                }
            }
        }
        else if ((CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
              &&((CurrentPictureInfos_p->VideoParams.TopFieldFirst)
              || (CurrentPicture_p->Counters[Index].TopFieldCounter == 0)))
        {
            /* Top first or top counter is 0, decrement bottom counter */
            CurrentPicture_p->Counters[Index].BottomFieldCounter--;
            if (CurrentPicture_p->Counters[Index].BottomFieldCounter == 0)
            {
                /* Last bottom field displayed: display top now, or next */
                /* picture if no top */
                if (CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
                {
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_TOP;
                }
                else
                {
                    /* All field counters are 0: launch display of next pict */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                }
            }
        }
        else if ((CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
              && ((!(CurrentPictureInfos_p->VideoParams.TopFieldFirst))
              || (CurrentPicture_p->Counters[Index].BottomFieldCounter == 0)) )
        {
            /* Bottom first or bottom counter is 0, decrement top counter */
            CurrentPicture_p->Counters[Index].TopFieldCounter--;
            if (CurrentPicture_p->Counters[Index].TopFieldCounter == 0)
            {
                /* Last top field displayed: display bottom now, or repeat, */
                /* or next picture if no bottom nor repeat */
                if (CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
                {
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_BOTTOM;
                }
                else
                {
                    /* All field counters are 0: launch display of next pict */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                }
            }
        }
        else
        {
            /* Debug: Should not happen: case 0,0 treated before by pop of */
            /* the frame */
            TraceBuffer(("Can't have all counters 0 !\r\n"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    if (RepeatProgressive)
    {
        CurrentPicture_p->Counters[Index].RepeatProgressiveCounter --;
        /** Restore the values saved in  SaveTopFieldCounter and  SaveBottomFieldCounter   **/
        CurrentPicture_p->Counters[Index].TopFieldCounter    = SaveTopFieldCounter;
        CurrentPicture_p->Counters[Index].BottomFieldCounter = SaveBottomFieldCounter;

        if (!(CurrentPictureInfos_p->VideoParams.TopFieldFirst))
        {
            *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_TOP;
        }
        else
        {
            *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_BOTTOM;
        }
        if (ReDecrement)
        {
            ErrorCode = viddisp_DecrementFieldCounterBackward(CurrentPicture_p,
                                                        NextFieldOnDisplay_p,
                                                        Index);
        }
    }

    return(ErrorCode);
} /* End of viddisp_DecrementFieldCounterBackward() function */

/*******************************************************************************
Name        : viddisp_DecrementFieldCounterForward
Description : Decrement field counter depending on display, and give info
                    for display of next VSync:
                    - NextFieldOnDisplay_p is VIDDISP_FIELD_DISPLAY_NONE
                    if display of the picture is finished on next VSync
                    - else NextFieldOnDisplay_p indicates the next field
                    of the picture that should be displayed next
Parameters  :
Assumptions : Not all counters of the picture are 0 !
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if can't decrement
*******************************************************************************/
ST_ErrorCode_t viddisp_DecrementFieldCounterForward(
                            VIDDISP_Picture_t * const CurrentPicture_p,
                            VIDDISP_FieldDisplay_t * const NextFieldOnDisplay_p,
                            U32 Index)
{
    STVID_PictureInfos_t *  CurrentPictureInfos_p;
    BOOL                    RepeatProgressive   = FALSE;
    BOOL                    ReDecrement         = FALSE;
    ST_ErrorCode_t          ErrorCode           = ST_NO_ERROR;
    U32                     SaveTopFieldCounter, SaveBottomFieldCounter;

    /* Shortcut */
    CurrentPictureInfos_p = &(CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos);

    /* Suppose display won't change */
    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_SAME;

    SaveTopFieldCounter = CurrentPicture_p->Counters[Index].TopFieldCounter;
    SaveBottomFieldCounter = CurrentPicture_p->Counters[Index].BottomFieldCounter;

    /* How counters must be decremented: (R=repeat T=top B=bottom)
        - progressive: T-B-R, repeated RepeatProgressiveCounter times
        - interlaced: T-B-R if TopFieldFirst
                      B-T-R if !TopFieldFirst */
    if (CurrentPictureInfos_p->VideoParams.ScanType
            == STGXOBJ_PROGRESSIVE_SCAN)
    {
        /* Playing forward progressive */
        if ((CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
          &&((CurrentPictureInfos_p->VideoParams.TopFieldFirst)
          ||(CurrentPicture_p->Counters[Index].BottomFieldCounter == 0)) )
        {
            /* Top first or bottom counter is 0, decrement top counter */
            CurrentPicture_p->Counters[Index].TopFieldCounter--;
            TraceVerbose(("t~"));
            if (CurrentPicture_p->Counters[Index].TopFieldCounter == 0)
            {
                /* Last top field displayed: display bottom now, or repeat, */
                /* or next picture if no bottom nor repeat */
                if (CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
                {
                    /* OK to display bottom */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_BOTTOM;
                }
                else if (CurrentPicture_p->Counters[Index].RepeatFieldCounter!=0)
                {
                    /* No bottom field but OK to display repeat field */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_REPEAT;
                }
                else
                {
                    /* All field counters are 0: display again if required */
                    /* or launch display of next picture */
                    if (CurrentPicture_p->Counters[Index].
                                                RepeatProgressiveCounter != 0)
                    {
                        RepeatProgressive = TRUE;
                    }
                    else
                    {
                        /* Display is finished: put next pict to the screen */
                        *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                    }
                }
            } /* End TopFieldCounter == 0 */
        }
        else if ((CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
               && ((!(CurrentPictureInfos_p->VideoParams.TopFieldFirst))
               || (CurrentPicture_p->Counters[Index].TopFieldCounter == 0)) )
        {
            /* Bottom first or top counter is 0, decrement bottom counter */
            CurrentPicture_p->Counters[Index].BottomFieldCounter--;
            TraceVerbose(("b~"));
            if (CurrentPicture_p->Counters[Index].BottomFieldCounter == 0)
            {
                /* Last bottom field displayed: display top now, or next */
                /* picture if no top */
                if (CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
                {
                    /* OK to display top field */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_TOP;
                }
                else if (CurrentPicture_p->Counters[Index].RepeatFieldCounter!=0)
                {
                    /* No top field but OK to display repeat field */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_REPEAT;
                }
                else
                {
                    /* All field counters are 0: display again if required */
                    /* or launch display of next picture */
                    if (CurrentPicture_p->Counters[Index].RepeatProgressiveCounter != 0)
                    {
                        RepeatProgressive = TRUE;
                    }
                    else
                    {
                        /* Display is finished: put next pict to the screen */
                        *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                    }
                }
            } /* End BottomFieldCounter == 0 */
        }
        else if (CurrentPicture_p->Counters[Index].RepeatFieldCounter != 0)
        {
            /* Top and bottom counters are 0, decrement repeat counter */
            CurrentPicture_p->Counters[Index].RepeatFieldCounter--;
            TraceVerbose(("r~"));
            if (CurrentPicture_p->Counters[Index].RepeatFieldCounter == 0)
            {
                /* All field counters are 0: display again if required or */
                /* launch display of next picture */
                if (CurrentPicture_p->Counters[Index].RepeatProgressiveCounter != 0)
                {
                    RepeatProgressive = TRUE;
                }
                else
                {
                    /* Display is finished: put next picture to the screen */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                }
            }
        }
        else if (CurrentPicture_p->Counters[Index].RepeatProgressiveCounter != 0)
        {
            /* Should not happen: field counter should not be 0 with */
            /* RepeatProgressiveCounter not 0 ! */
            TraceInfo(("ERROR: picture counters inconsistence\r\n"));
            RepeatProgressive = TRUE;
            ReDecrement = TRUE;
        }
        else
        {
            /* Should not happen: case 0,0 treated before by pop of the frame */
            TraceBuffer(("Can't have all counters 0 !\r\n"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        /* Playing forward interlaced */
        if ((CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
         &&((CurrentPictureInfos_p->VideoParams.TopFieldFirst)
         || (CurrentPicture_p->Counters[Index].BottomFieldCounter == 0)))
        {
            /* Top first or bottom counter is 0, decrement top counter */
            CurrentPicture_p->Counters[Index].TopFieldCounter--;
            TraceVerbose(("t~"));
            if (CurrentPicture_p->Counters[Index].TopFieldCounter == 0)
            {
                /* Last top field displayed: display bottom now, or repeat, */
                /* or next picture if no bottom nor repeat */
                if (CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
                {
                    /* OK to display bottom */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_BOTTOM;
                }
                else if (CurrentPicture_p->Counters[Index].RepeatFieldCounter != 0)
                {
                    /* No bottom field but OK to display repeat field */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_REPEAT;
                }
                else
                {
                    /* All field counters are 0: launch display of next pict */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                }
            }
        }
        else if ((CurrentPicture_p->Counters[Index].BottomFieldCounter != 0)
              &&((!(CurrentPictureInfos_p->VideoParams.TopFieldFirst))
              ||   (CurrentPicture_p->Counters[Index].TopFieldCounter == 0)))
        {
            /* Bottom first or top counter is 0, decrement bottom counter */
            CurrentPicture_p->Counters[Index].BottomFieldCounter--;
            TraceVerbose(("b~"));
            if (CurrentPicture_p->Counters[Index].BottomFieldCounter == 0)
            {
                /* Last bottom field displayed: display top now, or next */
                /* picture if no top */
                if (CurrentPicture_p->Counters[Index].TopFieldCounter != 0)
                {
                    /* OK to display top field */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_TOP;
                }
                else if (CurrentPicture_p->Counters[Index].RepeatFieldCounter!=0)
                {
                    /* No top field but OK to display repeat field */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_REPEAT;
                }
                else
                {
                    /* All field counters are 0: launch display of next pict */
                    *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
                }
            }
        }
        else if (CurrentPicture_p->Counters[Index].RepeatFieldCounter != 0)
        {
            /* Top and bottom counters are 0, decrement repeat counter */
            CurrentPicture_p->Counters[Index].RepeatFieldCounter--;
            TraceVerbose(("r~"));
            if (CurrentPicture_p->Counters[Index].RepeatFieldCounter == 0)
            {
                /* All field counters are 0: launch display of next picture */
                *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_NONE;
            }
        }
        else
        {
            /* Should not happen: case 0,0 treated before by pop of the frame */
            TraceBuffer(("Can't have all counters 0 !\r\n"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    } /* End playing forward interlaced */

    if (RepeatProgressive)
    {
        CurrentPicture_p->Counters[Index].RepeatProgressiveCounter --;
        /** Restore the values saved in  SaveTopFieldCounter and  SaveBottomFieldCounter   **/
        CurrentPicture_p->Counters[Index].TopFieldCounter    = SaveTopFieldCounter;
        CurrentPicture_p->Counters[Index].BottomFieldCounter = SaveBottomFieldCounter;

        if (CurrentPictureInfos_p->VideoParams.TopFieldFirst)
        {
            *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_TOP;
        }
        else
        {
            *NextFieldOnDisplay_p = VIDDISP_FIELD_DISPLAY_BOTTOM;
        }
        if (ReDecrement)
        {
            ErrorCode = viddisp_DecrementFieldCounterForward(CurrentPicture_p,
                                                    NextFieldOnDisplay_p,
                                                    Index);
        }
    }
    return(ErrorCode);
} /* End of viddisp_DecrementFieldCounterForward() function */

/* end of vid_flds.c */
