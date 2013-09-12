/*******************************************************************************

File name   : osd_list.c

Description :

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                 Name
----               ------------                                 ----
2000-12-20          Creation                                    Michel Bruant


*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stlayer.h"
#include "osd_list.h"
#include "osd_vp.h"
#include "osd_cm.h"
#include <stdlib.h>
#include "layerosd.h"

/* Constants ---------------------------------------------------------------- */

#define INSERT_TRY_NEXT     0
#define INSERT_TRY_CURR     1
#define INSERT_TRY_END      2

/*******************************************************************************
Name        : stlayer_osd_InsertViewport
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_osd_InsertViewport(STLAYER_ViewPortHandle_t Handle)
{
    U32                     Insertion;
    stosd_ViewportDesc *    CurrentVP;
    stosd_ViewportDesc *    NewVP;

    NewVP = (stosd_ViewportDesc *)Handle;

    /* Case : Empty list */
    if (stlayer_osd_context.NumViewPortLinked == 0)
    {
        stlayer_osd_context.FirstViewPortLinked = NewVP;
        NewVP->Next_p = NULL;
        NewVP->Prev_p = NULL;
        stlayer_osd_context.NumViewPortLinked = 1;
        return(ST_NO_ERROR);
    }
    else /* Scan the list */
    {
        CurrentVP = stlayer_osd_context.FirstViewPortLinked;
        Insertion = INSERT_TRY_NEXT;
        do
        {
            if ( ((S32)(CurrentVP->OutputRectangle.PositionY
                 + CurrentVP->OutputRectangle.Height - 1))
                < NewVP->OutputRectangle.PositionY)
            {
                if (CurrentVP->Next_p != 0)
                {
                    CurrentVP = CurrentVP->Next_p;
                }
                else /* reach end of the list */
                {
                    Insertion = INSERT_TRY_END;
                }
            }
            else
            {
                Insertion = INSERT_TRY_CURR;
            }
        } while (Insertion == INSERT_TRY_NEXT);
        if ((((S32)(NewVP->OutputRectangle.PositionY + NewVP->OutputRectangle.Height -1))
              < CurrentVP->OutputRectangle.PositionY)
                ||(Insertion == INSERT_TRY_END))
        {
            /* insertion */
            /* Case : End of the list */
            if (Insertion == INSERT_TRY_END)
            {
                NewVP->Next_p                       = 0;
                NewVP->Prev_p                       = CurrentVP;
                CurrentVP->Next_p                   = NewVP;
            }
            /* Case : Beginning of the list */
            else if (CurrentVP == stlayer_osd_context.FirstViewPortLinked)
            {
                stlayer_osd_context.FirstViewPortLinked    = NewVP;
                NewVP->Next_p                       = CurrentVP;
                NewVP->Prev_p                       = 0;
                CurrentVP->Prev_p                   = NewVP;

            }
            /* Case : Middle of the list */
            else
            {
                NewVP->Next_p                       = CurrentVP;
                NewVP->Prev_p                       = CurrentVP->Prev_p;
                CurrentVP->Prev_p->Next_p           = NewVP;
                CurrentVP->Prev_p                   = NewVP;
            }
            stlayer_osd_context.NumViewPortLinked ++;
            return(ST_NO_ERROR);
        }
        else
        {
            layerosd_Defense(STLAYER_ERROR_OVERLAP_VIEWPORT);
            return(STLAYER_ERROR_OVERLAP_VIEWPORT);
        }
    }
}

/*******************************************************************************
Name        : stlayer_osd_ExtractViewport
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_osd_ExtractViewport(STLAYER_ViewPortHandle_t Handle)
{
    stosd_ViewportDesc *    NewVP;

    NewVP = (stosd_ViewportDesc *)Handle;
    if (NewVP->Next_p != 0)
    {
        NewVP->Next_p->Prev_p = NewVP->Prev_p;
    }

    if(NewVP->Prev_p != 0)
    {
        NewVP->Prev_p->Next_p = NewVP->Next_p;
    }
    else
    {
        stlayer_osd_context.FirstViewPortLinked = NewVP->Next_p;
    }
    stlayer_osd_context.NumViewPortLinked --;
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : stlayer_osd_IndexNewViewport
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 stlayer_osd_IndexNewViewport(void)
{
    BOOL    Found;
    U32     Index;

    Found = FALSE;
    Index = 0;
    do
    {
        if (stlayer_osd_context.OSD_ViewPort[Index].Open == FALSE)
        {
            Found = TRUE;
        }
        else
        {
            Index ++;
        }
    }while((Found == FALSE)&&(Index <= stlayer_osd_context.MaxViewPorts));

    if (Found == TRUE)
    {
        return(Index);
    }
    else
    {
        return(NO_NEW_VP);
    }
}
/******************************************************************************/
