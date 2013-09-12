/*******************************************************************************

File name   : osd_list.c

Description : Linked list manager. 

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
#include "osd_hal.h"
#include <stdlib.h>
#include "layerosd.h"

/* Constants ---------------------------------------------------------------- */

#define INSERT_TRY_NEXT     0
#define INSERT_TRY_CURR     1
#define INSERT_TRY_END      2

/*******************************************************************************
Name        : stlayer_InsertViewport
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_InsertViewport(STLAYER_ViewPortHandle_t Handle)
{
    U32                     Insertion;
    stosd_ViewportDesc *    CurrentVP;
    stosd_ViewportDesc *    NewVP;

    NewVP = (stosd_ViewportDesc *)Handle;

    /* Case : Empty list */
    if (stlayer_osd_context.NumViewPortEnabled == 0)
    {
        stlayer_osd_context.FirstViewPortEnabled = NewVP;
        NewVP->Next_p = NULL; 
        NewVP->Prev_p = NULL;
        stlayer_osd_context.NumViewPortEnabled ++;
        return(ST_NO_ERROR);
    }
    else /* Scan the list */
    {
        CurrentVP = stlayer_osd_context.FirstViewPortEnabled;
        Insertion = INSERT_TRY_NEXT;
        do
        {
            if (( CurrentVP->PositionY
                 + CurrentVP->Height + 2) /* two lines between VP */
                < NewVP->PositionY)
            {
                if (CurrentVP->Next_p != NULL)
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
        if ((NewVP->PositionY + NewVP->Height + 2 /* two lines between VP */
              < CurrentVP->PositionY) 
                ||(Insertion == INSERT_TRY_END))
        {
            /* insertion */
            /* Case : End of the list */
            if (Insertion == INSERT_TRY_END)
            {
                NewVP->Next_p                       = NULL;
                NewVP->Prev_p                       = CurrentVP;
                CurrentVP->Next_p                   = NewVP;
            }
            /* Case : Beginning of the list */
            else if (CurrentVP == stlayer_osd_context.FirstViewPortEnabled)
            {
                NewVP->Next_p                       = CurrentVP;
                NewVP->Prev_p                       = NULL;
                CurrentVP->Prev_p                   = NewVP;
                stlayer_osd_context.FirstViewPortEnabled    = NewVP;
            }
            /* Case : Middle of the list */
            else
            {    
                NewVP->Next_p                       = CurrentVP;
                NewVP->Prev_p                       = CurrentVP->Prev_p;
                CurrentVP->Prev_p->Next_p           = NewVP;
                CurrentVP->Prev_p                   = NewVP;
            }
            stlayer_osd_context.NumViewPortEnabled ++;
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
Name        : stlayer_ExtractViewport
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_ExtractViewport(STLAYER_ViewPortHandle_t Handle)
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
        stlayer_osd_context.FirstViewPortEnabled = NewVP->Next_p;
    }
    stlayer_osd_context.NumViewPortEnabled --;
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stlayer_IndexNewViewport
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 stlayer_IndexNewViewport(void)
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
    }while((Found == FALSE)&&(Index < stlayer_osd_context.MaxViewPorts));

    if (Found == TRUE)
    {
        return(Index);
    }
    else
    {
        return(NO_NEW_VP);
    }
}
/*******************************************************************************
Name        : stlayer_TestMove
Description : 
Parameters  :
Assumptions :
Limitations : 
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_TestMove(STLAYER_ViewPortHandle_t VPHandle,
                        STGXOBJ_Rectangle_t * OutputRectangle)
{
    U32                             OldListRank,NewListRank;
    stosd_ViewportDesc *  CurrentVP;
    ST_ErrorCode_t                  Error;
    STGXOBJ_Rectangle_t             SaveOutput;

    /* Calculate the rank of the VP in its list */
    CurrentVP = stlayer_osd_context.FirstViewPortEnabled;
    OldListRank  = 1;
    while (CurrentVP != (stosd_ViewportDesc*)VPHandle)
    {
        CurrentVP = CurrentVP->Next_p;
        OldListRank ++;
    }

    /* save rectangle before changes */
    SaveOutput.Height       = ((stosd_ViewportDesc*)VPHandle)->Height;
    SaveOutput.PositionY    = ((stosd_ViewportDesc*)VPHandle)->PositionY;

    /* Test New Rectangle */
    stlayer_ExtractViewport(VPHandle);
    ((stosd_ViewportDesc*)VPHandle)->Height     = OutputRectangle->Height;
    ((stosd_ViewportDesc*)VPHandle)->PositionY  = OutputRectangle->PositionY;
    Error = stlayer_InsertViewport(VPHandle);
    if (Error)
    {
        ((stosd_ViewportDesc*)VPHandle)->Height     = SaveOutput.Height;
        ((stosd_ViewportDesc*)VPHandle)->PositionY  = SaveOutput.PositionY;
        stlayer_InsertViewport(VPHandle);
        return(OSD_IMPOSSIBLE_RESIZE_MOVE);
    }
    
    /* Calculate the rank of the VP in its list */
    CurrentVP = stlayer_osd_context.FirstViewPortEnabled;
    NewListRank  = 1;
    while (CurrentVP != (stosd_ViewportDesc*)VPHandle)
    {
        CurrentVP = CurrentVP->Next_p;
        NewListRank ++;
    }  
    if (NewListRank == OldListRank)
    {
        /* Move OK without any inversion */
        return(OSD_OK);
    }
    else
    {
        /* Move OK with inversion */
        return(OSD_MOVE_INVERSION_LIST);
    }
}  
/******************************************************************************/
