/*******************************************************************************

File name   : desc_mng.c

Description : functions Descriptor List Manager

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant

    Descriptor List Manager
    -----------------------
    stlayer_InsertVPDescriptor
    stlayer_ExtractVPDescriptor
    stlayer_CheckVPDescriptor
    stlayer_GetViewPortHandle
    stlayer_FreeViewPortHandle
    stlayer_FillViewPort

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#endif
#include "hard_mng.h"
#include "frontend.h"
#include "desc_mng.h"

/* Private functions prototypes ----------------------------------------------*/

ST_ErrorCode_t stlayer_CheckForInsertion(
  stlayer_ViewPortDescriptor_t* VPList,
  stlayer_ViewPortDescriptor_t * InsertedVP
);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : stlayer_InsertVPDescriptor
Description : Insert an initialized VP in its layer
Parameters  :
Assumptions :
Limitations : !!! The VP must be checked before inserted.
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_InsertVPDescriptor
                                (stlayer_ViewPortDescriptor_t * NewVPDesc)
{
    stlayer_XLayerContext_t *      XLayerContext;
    stlayer_ViewPortDescriptor_t * CurrentVP;
    BOOL                           Insertion;

    /* Find the concerned Layer */
    XLayerContext = &(stlayer_context.XContext[NewVPDesc->LayerHandle]);

    /* Case : Empty list */
    if(XLayerContext->NumberLinkedViewPorts == 0)
    {
        XLayerContext->LinkedViewPorts        = NewVPDesc;
        NewVPDesc->Next_p                     = NewVPDesc;
        NewVPDesc->Prev_p                     = NewVPDesc;
    }
    else /* Scan the list */
    {
        CurrentVP = XLayerContext->LinkedViewPorts;
        Insertion = FALSE;
        do
        {

        if (XLayerContext->LayerType==STLAYER_GAMMA_GDPVBI)
           {
                if (  (S32) (CurrentVP->ClippedOut.PositionY
                    + CurrentVP->ClippedOut.Height - 1)
                    < (S32)(NewVPDesc->ClippedOut.PositionY))
                {
                    CurrentVP = CurrentVP->Next_p;
                }
                else
                {
                    Insertion = TRUE;
                }
           }
           else
           {
                if (  CurrentVP->ClippedOut.PositionY
                    + CurrentVP->ClippedOut.Height - 1
                    < (U32)(NewVPDesc->ClippedOut.PositionY))
                {
                    CurrentVP = CurrentVP->Next_p;
                }
                else
                {
                    Insertion = TRUE;
                }
            }
        } while ((Insertion == FALSE)
                && (CurrentVP != XLayerContext->LinkedViewPorts));
        /* Insertion */
        NewVPDesc->Next_p         = CurrentVP;
        NewVPDesc->Prev_p         = CurrentVP->Prev_p;
        CurrentVP->Prev_p->Next_p = NewVPDesc;
        CurrentVP->Prev_p         = NewVPDesc;
        /* Case : Beginning of the list */
        if ((CurrentVP == XLayerContext->LinkedViewPorts)
            && (Insertion == TRUE))
        {
            XLayerContext->LinkedViewPorts = NewVPDesc;
        }
    }
    XLayerContext->NumberLinkedViewPorts  ++;
    return(DESC_MANAGER_OK);
}

/*******************************************************************************
Name        : stlayer_ExtractVPDescriptor
Description : Extract a VP from its layer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_ExtractVPDescriptor
                                    (stlayer_ViewPortDescriptor_t * OldVPDesc)
{
    stlayer_XLayerContext_t *      XLayerContext;

    /* Find the concerned Layer */
    XLayerContext = &(stlayer_context.XContext[OldVPDesc->LayerHandle]);

    /* Case : Empty List */
    if(XLayerContext->NumberLinkedViewPorts == 1)
    {
        XLayerContext->LinkedViewPorts = 0;
    }
    /* Case : Beginning of the list */
    else if (XLayerContext->LinkedViewPorts == OldVPDesc)
    {
        XLayerContext->LinkedViewPorts = OldVPDesc->Next_p;
    }
    /* Extraction */
    OldVPDesc->Next_p->Prev_p = OldVPDesc->Prev_p;
    OldVPDesc->Prev_p->Next_p = OldVPDesc->Next_p;
    XLayerContext->NumberLinkedViewPorts  --;
    return(DESC_MANAGER_OK);
}

/*******************************************************************************
Name        :  stlayer_CheckVPDescriptor
Description :  Check a VP before insertion.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_CheckVPDescriptor(stlayer_ViewPortDescriptor_t * NewVP)
{
    stlayer_XLayerContext_t *      XLayerContext;
    ST_ErrorCode_t                 Check;

    /* Find the concerned Layer */
    XLayerContext = &(stlayer_context.XContext[NewVP->LayerHandle]);

    /* Case : Empty List */
    if (XLayerContext->NumberLinkedViewPorts == 0)
    {
        Check = DESC_MANAGER_OK;
    }
    else
    {
        /* Check an insertion */
        Check = stlayer_CheckForInsertion(XLayerContext->LinkedViewPorts,
                                              NewVP);
    }
    return(Check);
}
/*******************************************************************************
Name        : stlayer_FillViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_FillViewPort(STLAYER_Handle_t LayerHandle,
                    STLAYER_ViewPortParams_t * Params,
                    stlayer_ViewPortHandle_t  VPHandleCasted)
{
    stlayer_ViewPortDescriptor_t * PtViewPort;

    if((VPHandleCasted == 0) || (Params == 0))
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if(Params->Source_p == 0)
    {
        layergfx_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    PtViewPort = (stlayer_ViewPortDescriptor_t *)VPHandleCasted;
    PtViewPort->LayerHandle     = LayerHandle;
    PtViewPort->Enabled         = FALSE;
    STOS_memcpy(&PtViewPort->Bitmap, Params->Source_p->Data.BitMap_p, sizeof(STGXOBJ_Bitmap_t));
    if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR)
    {
        STOS_memcpy(&PtViewPort->Palette, Params->Source_p->Palette_p, sizeof(STGXOBJ_Palette_t));
    }
    if ((stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_GDP)  && (Params->Source_p->Palette_p != 0))
    {
        STOS_memcpy(&PtViewPort->Palette, Params->Source_p->Palette_p, sizeof(STGXOBJ_Palette_t));
    }

    STOS_memcpy(&PtViewPort->InputRect, &Params->InputRectangle, sizeof(STGXOBJ_Rectangle_t));
    STOS_memcpy(&PtViewPort->OutputRect, &Params->OutputRectangle, sizeof(STGXOBJ_Rectangle_t));

    PtViewPort->ColorKeyEnabled                    = FALSE;
    PtViewPort->ColorKeyValue.Type              = STGXOBJ_COLOR_KEY_TYPE_RGB888;
    PtViewPort->ColorKeyValue.Value.RGB888.RMin    = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.RMax    = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.ROut    = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.REnable = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.GMin    = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.GMax    = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.GOut    = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.GEnable = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.BMin    = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.BMax    = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.BOut    = 0;
    PtViewPort->ColorKeyValue.Value.RGB888.BEnable = 0;
    PtViewPort->BorderAlphaOn                      = FALSE;
    PtViewPort->Gain.BlackLevel                    = 0;
    PtViewPort->Gain.GainLevel                     = 128;
    PtViewPort->Alpha.A0                           = 128;
    PtViewPort->Alpha.A1                           = 128;
    PtViewPort->AssociatedToMainLayer              = 0;
    PtViewPort->FilterPointer                      = 0;
    PtViewPort->HideOnMix2                         = FALSE;
    PtViewPort->FFilterEnabled                     = FALSE;
    PtViewPort->FFilterMode                        = STLAYER_FLICKER_FILTER_MODE_SIMPLE;
    PtViewPort->ColorFillEnabled                   = FALSE;
    PtViewPort->ColorFillValue.R                   = 100;
    PtViewPort->ColorFillValue.G                   = 100;
    PtViewPort->ColorFillValue.B                   = 0;
    PtViewPort->ColorFillValue.Alpha               = 0x80;


    return(DESC_MANAGER_OK);
}

/*******************************************************************************
Name        : stlayer_GetViewPortHandle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_GetViewPortHandle(
  stlayer_ViewPortHandle_t*   PtViewPortHandle,
  STLAYER_Handle_t            LayerHandle)
{
  ST_ErrorCode_t Error;

  Error =stlayer_GetElement(&stlayer_context.XContext[LayerHandle].DataPoolDesc,
                            (Handle_t*)PtViewPortHandle);
  return(Error);
}

/*******************************************************************************
Name        : FreeViewPortHandle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_FreeViewPortHandle(
        stlayer_ViewPortHandle_t ViewPortHandle,
        STLAYER_Handle_t            LayerHandle)
{
    ST_ErrorCode_t Error;
    Error = stlayer_ReleaseElement(&stlayer_context.XContext[LayerHandle].
                                    DataPoolDesc,(Handle_t)ViewPortHandle);
    return(Error);
}
/*******************************************************************************
Name        : CheckForInsertion
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_CheckForInsertion(stlayer_ViewPortDescriptor_t * VPList,
                      stlayer_ViewPortDescriptor_t * InsertedVP)
{
    ST_ErrorCode_t                  Check;
    stlayer_ViewPortDescriptor_t *  CurrentVP;
    BOOL                            Insertion;
    stlayer_XLayerContext_t *      XLayerContext;
    Check = DESC_MANAGER_OK;
    CurrentVP = VPList;
    Insertion = FALSE;


    XLayerContext = &(stlayer_context.XContext[InsertedVP->LayerHandle]);

      if (XLayerContext->LayerType==STLAYER_GAMMA_GDPVBI)
      {
        /* Scan the list */
        do
        {
            if (  (S32)(CurrentVP->ClippedOut.PositionY
                + CurrentVP->ClippedOut.Height - 1)
                < (S32)(InsertedVP->ClippedOut.PositionY))
            {
                CurrentVP = CurrentVP->Next_p;
            }
            else
            {
                Insertion = TRUE;
            }
        } while ((Insertion == FALSE) && (CurrentVP != VPList));

        if (Insertion == FALSE)/* last position */
        {
        return(DESC_MANAGER_OK);
        }
        if (CurrentVP == VPList) /* first postion */
        {
            if ((S32)(InsertedVP->ClippedOut.PositionY + InsertedVP->ClippedOut.Height -1)
                < (S32)(CurrentVP->ClippedOut.PositionY))
            {
                return(DESC_MANAGER_OK);
            }
        }
        else /* middle position */
        {
            if((InsertedVP->ClippedOut.PositionY + InsertedVP->ClippedOut.Height -1
                < (U32)(CurrentVP->ClippedOut.PositionY))
                &&
                ((U32)(InsertedVP->ClippedOut.PositionY) >
                CurrentVP->Prev_p->ClippedOut.PositionY
                + CurrentVP->Prev_p->ClippedOut.Height - 1 ))
            {
                return(DESC_MANAGER_OK);
            }
        }

      }
    else
     {
        /* Scan the list */
        do
        {
            if (  CurrentVP->ClippedOut.PositionY
                + CurrentVP->ClippedOut.Height - 1
                < (U32)(InsertedVP->ClippedOut.PositionY))
            {
                CurrentVP = CurrentVP->Next_p;
            }
            else
            {
                Insertion = TRUE;
            }
        } while ((Insertion == FALSE) && (CurrentVP != VPList));

        if (Insertion == FALSE)/* last position */
        {
        return(DESC_MANAGER_OK);
        }
        if (CurrentVP == VPList) /* first postion */
        {
            if (InsertedVP->ClippedOut.PositionY + InsertedVP->ClippedOut.Height -1
                < (U32)(CurrentVP->ClippedOut.PositionY))
            {
                return(DESC_MANAGER_OK);
            }
        }
        else /* middle position */
        {
            if((InsertedVP->ClippedOut.PositionY + InsertedVP->ClippedOut.Height -1
                < (U32)(CurrentVP->ClippedOut.PositionY))
                &&
                ((U32)(InsertedVP->ClippedOut.PositionY) >
                CurrentVP->Prev_p->ClippedOut.PositionY
                + CurrentVP->Prev_p->ClippedOut.Height - 1 ))
            {
                return(DESC_MANAGER_OK);
            }
        }
    }
    /* other cases */
    return(DESC_MANAGER_IMPOSSIBLE_INSERTION);
}
/*******************************************************************************
Name        : stlayer_TestMove
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : DESC_MANAGER_IMPOSSIBLE_RESIZE_MOVE
              DESC_MANAGER_OK
              DESC_MANAGER_MOVE_INVERSION_LIST
*******************************************************************************/
ST_ErrorCode_t stlayer_TestMove(STLAYER_ViewPortHandle_t VPHandle,
                        STGXOBJ_Rectangle_t * ClippedOut)
{
    stlayer_XLayerContext_t *       XLayerContext;
    U32                             OldListRank,NewListRank;
    stlayer_ViewPortDescriptor_t *  CurrentVP;
    ST_ErrorCode_t                  Error;
    STGXOBJ_Rectangle_t             SaveClippedOut;

    XLayerContext = &(stlayer_context.XContext
            [((stlayer_ViewPortDescriptor_t*)VPHandle)->LayerHandle]);

    /* Calculate the rank of the VP in its list */
    CurrentVP = XLayerContext->LinkedViewPorts;
    OldListRank  = 1;
    while (CurrentVP != (stlayer_ViewPortDescriptor_t*)VPHandle)
    {
        CurrentVP = CurrentVP->Next_p;
        OldListRank ++;
    }

    /* save rectangle before changes */
    STOS_memcpy(&SaveClippedOut, &((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, sizeof(STGXOBJ_Rectangle_t));
    /* Test New Rectangle */
    stlayer_ExtractVPDescriptor((stlayer_ViewPortDescriptor_t*)VPHandle);
    STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, ClippedOut, sizeof(STGXOBJ_Rectangle_t));
    Error = stlayer_CheckVPDescriptor((stlayer_ViewPortDescriptor_t*)VPHandle);
    if (Error)
    {
        /* restore previous position and quit */
        STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &SaveClippedOut, sizeof(STGXOBJ_Rectangle_t));
        stlayer_InsertVPDescriptor((stlayer_ViewPortDescriptor_t*)VPHandle);
        return(DESC_MANAGER_IMPOSSIBLE_RESIZE_MOVE);
    }
    else
    {
        /* insert in new position : Move OK with or without inversion */
        stlayer_InsertVPDescriptor((stlayer_ViewPortDescriptor_t*)VPHandle);
    }

    /* Calculate the rank of the VP in its list */
    CurrentVP = XLayerContext->LinkedViewPorts;
    NewListRank  = 1;
    while (CurrentVP != (stlayer_ViewPortDescriptor_t*)VPHandle)
    {
        CurrentVP = CurrentVP->Next_p;
        NewListRank ++;
    }
    if (NewListRank == OldListRank)
    {
        /* Move OK without any inversion */
        return(DESC_MANAGER_OK);
    }
    else
    {
        /* Move OK with inversion : restore the previous config */
        stlayer_ExtractVPDescriptor((stlayer_ViewPortDescriptor_t*)VPHandle);
        STOS_memcpy(&((stlayer_ViewPortDescriptor_t*)VPHandle)->ClippedOut, &SaveClippedOut, sizeof(STGXOBJ_Rectangle_t));
        stlayer_InsertVPDescriptor((stlayer_ViewPortDescriptor_t*)VPHandle);
        return(DESC_MANAGER_MOVE_INVERSION_LIST);
    }
}
/******************************************************************************/
