/*******************************************************************************

File name   : bdisp_fe.c

Description : front-end source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
01 Jun 2004        Created                                          HE
30 May 2006        Modified for WinCE platform					  WinCE Team-ST Noida
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <string.h>
#endif
#include "stddefs.h"
#include "stblit.h"
#include "stgxobj.h"
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "bdisp_init.h"
#include "bdisp_fe.h"
#include "bdisp_be.h"
#include "bdisp_com.h"
#include "bdisp_mem.h"
#include "bdisp_job.h"
#include "bdisp_pool.h"
#if !defined(ST_OS21) && !defined(ST_OSLINUX) && !defined(ST_OSWINCE)
	#include "ostime.h"
#endif /*End of ST_OS21 &&  ST_OSLINUX*/

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */
U32 STBLIT_FirstNodeAddress; /* Usefull when STBLIT_TEST_FRONTEND */


/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
void stblit_waitallblitscompleted( stblit_Device_t* Device_p );
/* Functions ---------------------------------------------------------------- */



/*******************************************************************************
Name        : stblit_GetIndex
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level.
Limitations :
Returns     :
*******************************************************************************/
 ST_ErrorCode_t stblit_GetIndex (STGXOBJ_ColorType_t TypeIn, U8* Index, STGXOBJ_ColorSpaceConversionMode_t Mode)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

     switch (TypeIn)
    {
        case STGXOBJ_COLOR_TYPE_RGB565:
            *Index =  0;
            break;
        case STGXOBJ_COLOR_TYPE_RGB888:
            *Index =  1;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB8565:
        case STGXOBJ_COLOR_TYPE_ARGB8565_255:
            *Index =  2;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            *Index =  3;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB1555:
            *Index =  4;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            *Index =  5;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT1:
            *Index =  6;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT2:
            *Index =  7;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            *Index =  8;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT8:
            *Index =  9;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT44:
            *Index =  10;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT88:
        case STGXOBJ_COLOR_TYPE_ACLUT88_255:
            *Index =  11;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
            (Mode == STGXOBJ_ITU_R_BT601) ? (*Index =  12) : (*Index =  18);
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
            (Mode == STGXOBJ_ITU_R_BT601) ? (*Index =  13) : (*Index =  19);
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
            (Mode == STGXOBJ_ITU_R_BT601) ? (*Index =  14) : (*Index =  20);
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
        case STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888:
            (Mode == STGXOBJ_ITU_R_BT601) ? (*Index =  15) : (*Index =  21);
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
            (Mode == STGXOBJ_ITU_R_BT601) ? (*Index =  16) : (*Index =  22);
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:
            (Mode == STGXOBJ_ITU_R_BT601) ? (*Index =  17) : (*Index =  23);
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            *Index =  24;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA8:
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
            *Index =  25;
            break;
        case STGXOBJ_COLOR_TYPE_BYTE:
            *Index =  26;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA4:
            *Index =  27;
            break;
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
 }

/*******************************************************************************
Name        : stblit_GetTmpIndex
Description : Get Tmp buffer index for conversion color checking
Parameters  :
Assumptions :  All parameters checks done at upper level.
               The Tmp bitmap is always using a 0:128 alpha range
               (0:255 range is never used internally by the blitter engine.
               It's always converted to 0:128 range at src1/Src2/Tgt level)
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_GetTmpIndex (U8 SrcIndex, U8* Index_p, STGXOBJ_ColorType_t* ColorType_p, U8* NbBitPerPixel_p)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    if ((SrcIndex == 0) ||
        (SrcIndex == 2))
    {
        *Index_p          = 2;
        *ColorType_p      = STGXOBJ_COLOR_TYPE_ARGB8565;
        *NbBitPerPixel_p  = 24;
    }
    else if ((SrcIndex == 1)  ||
             (SrcIndex == 3)  ||
             (SrcIndex == 4)  ||
             (SrcIndex == 5)  ||
             (SrcIndex == 12) ||
             (SrcIndex == 15) ||
             (SrcIndex == 18) ||
             (SrcIndex == 21))
    {
        *Index_p = 3;
        *ColorType_p = STGXOBJ_COLOR_TYPE_ARGB8888;
        *NbBitPerPixel_p  = 32;
    }
    else if ((SrcIndex == 9) ||  /* For Clut8(index 9), be carefull, the tmp bitmap will have 0 on its alpha channel on the
                                  * internal h/w bus */
             (SrcIndex == 11))
    {
        *Index_p = 11;
        *ColorType_p = STGXOBJ_COLOR_TYPE_ACLUT88 ;
        *NbBitPerPixel_p  = 16;
    }
    else
    {
        Err = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    return(Err);
 }

/*******************************************************************************
Name        : stblit_OneInputBlit
Description : 1 input / 1 output blit operation without mask
Parameters  :
Assumptions : All parameters not null
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_OneInputBlit(stblit_Device_t*        Device_p,
                                   STBLIT_Source_t*        Src_p,
                                   STBLIT_Destination_t*   Dst_p,
                                   STBLIT_BlitContext_t*   BlitContext_p)
 {
    ST_ErrorCode_t                      Err             = ST_NO_ERROR;
    STGXOBJ_ColorType_t                 DstColorType    = Dst_p->Bitmap_p->ColorType;
    STGXOBJ_ColorType_t                 SrcColorType;
    U8                                  SrcIndex ,DstIndex;
    U16                                 Code            = 0;
    STBLIT_Source_t                     Src1;
    STBLIT_Source_t                     Src2;
    STBLIT_Destination_t                Dest;
    STBLIT_Source_t*                    Src1_p     = &Src1;
    STBLIT_Source_t*                    Src2_p     = &Src2;
    U32                                 NumberNodes;
    stblit_NodeHandle_t                 *NodeFirstHandle_p, *NodeLastHandle_p;
    stblit_OperationConfiguration_t     OperationCfg;
    STGXOBJ_Rectangle_t                 SrcAutoClipRectangle, DstAutoClipRectangle;
    S32                                 DstPositionX, DstPositionY;
    U32                                 XOffset,YOffset;
    BOOL                                SrcMB           = FALSE;

    /* TOP/FIELD managment vars */
    U32                                 TopField_NumberNodes, BottomField_NumberNodes;
    stblit_NodeHandle_t                 *TopField_NodeFirstHandle_p, *TopField_NodeLastHandle_p,
                                        *BottomField_NodeFirstHandle_p, *BottomField_NodeLastHandle_p;



    /* Check Dst Color Type*/
    if ( ( Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB ) || ( Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_TOP_BOTTOM ) ||
         ( Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP ))
    {
        /* Hardware not supported */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

#if defined(ST_5105)
    /* Check if Src color type is (A)YCbCr and Dst color type is (A)RGB*/
    if ( ((Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444) ||
          (Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444) ||
          (Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422) ||
          (Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422) ||
          (Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888) ||
          (Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888)) &&
         ((Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_RGB888) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_RGB565) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB1555) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB4444) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888_255) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565_255)))
    {
        /* Hardware not supported */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif

    if (Src_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP)
    {
        /* Apply VC1 Data Range if src ScalingFactorY and ScalingFactorUV are different to YUV_NO_RESCALE */
        if ((Src_p->Data.Bitmap_p->YUVScaling.ScalingFactorY == YUV_NO_RESCALE) || (Src_p->Data.Bitmap_p->YUVScaling.ScalingFactorUV == YUV_NO_RESCALE))
        {
            return (ST_ERROR_BAD_PARAMETER);
        }
    }


    /* Check Dst rectangle size */
    if ((Dst_p->Rectangle.Width == 0)    ||
        (Dst_p->Rectangle.Height == 0))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    /* Some initializations*/
    Src2                  = *Src_p;
    Dest                  = *Dst_p;
    DstPositionX          = Dst_p->Rectangle.PositionX;
    DstPositionY          = Dst_p->Rectangle.PositionY;
    XOffset               = 0;
    YOffset               = 0;

    /* Auto clipping done when no resize */
    if (Src_p->Type == STBLIT_SOURCE_TYPE_BITMAP)
    {
        /* Check Src rectangle size */
        if ((Src_p->Rectangle.Width == 0)  ||
            (Src_p->Rectangle.Height == 0))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }

        /* Automatical clipping to Srcbitmap boundaries  */
        SrcAutoClipRectangle = Src_p->Rectangle;
        DstAutoClipRectangle = Dst_p->Rectangle;

        /* Perform Auto clip in case no resize */
        if ((Src_p->Rectangle.Height == Dst_p->Rectangle.Height) &&
            (Src_p->Rectangle.Width == Dst_p->Rectangle.Width))
        {
            /* Check if src rectangle outside completely Src bitmap or Dst Bitmap = > Nothing to copy */
            if ((Src_p->Rectangle.PositionX > (S32)(Src_p->Data.Bitmap_p->Width - 1))  ||
                (Src_p->Rectangle.PositionY > (S32)(Src_p->Data.Bitmap_p->Height - 1)) ||
                ((Src_p->Rectangle.PositionX + (S32)Src_p->Rectangle.Width - 1) < 0) ||
                ((Src_p->Rectangle.PositionY + (S32)Src_p->Rectangle.Height - 1) < 0) ||
                (Dst_p->Rectangle.PositionX > (S32)(Dst_p->Bitmap_p->Width - 1))  ||
                (Dst_p->Rectangle.PositionY > (S32)(Dst_p->Bitmap_p->Height - 1)) ||
                ((Dst_p->Rectangle.PositionX + (S32)Src_p->Rectangle.Width - 1) < 0) ||
                ((Dst_p->Rectangle.PositionY + (S32)Src_p->Rectangle.Height - 1) < 0))
            {
                if ((BlitContext_p->NotifyBlitCompletion == TRUE) && (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
                {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
                    /* Notify completion as required. */
                    STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
                }
                return(ST_NO_ERROR);
            }
            if (Src_p->Rectangle.PositionX < 0)
            {
                SrcAutoClipRectangle.Width      += Src_p->Rectangle.PositionX;
                DstAutoClipRectangle.PositionX  -= Src_p->Rectangle.PositionX;
                SrcAutoClipRectangle.PositionX   = 0;
                XOffset                          = -Src_p->Rectangle.PositionX;
            }
            if (Src_p->Rectangle.PositionY < 0)
            {
                SrcAutoClipRectangle.Height     += Src_p->Rectangle.PositionY;
                DstAutoClipRectangle.PositionY  -= Src_p->Rectangle.PositionY;
                SrcAutoClipRectangle.PositionY   = 0;
                YOffset                          = -Src_p->Rectangle.PositionY;
            }
            if ((SrcAutoClipRectangle.PositionX + SrcAutoClipRectangle.Width -1) > (Src_p->Data.Bitmap_p->Width - 1))
            {
                SrcAutoClipRectangle.Width = Src_p->Data.Bitmap_p->Width - SrcAutoClipRectangle.PositionX;
            }
            if ((SrcAutoClipRectangle.PositionY + SrcAutoClipRectangle.Height -1) > (Src_p->Data.Bitmap_p->Height - 1))
            {
                SrcAutoClipRectangle.Height = Src_p->Data.Bitmap_p->Height - SrcAutoClipRectangle.PositionY;
            }

            /* Check if Auto clip rectangle outside completely Dst Bitmap = > Nothing to copy */
            if ((DstAutoClipRectangle.PositionX > (S32)(Dst_p->Bitmap_p->Width - 1))  ||
                (DstAutoClipRectangle.PositionY > (S32)(Dst_p->Bitmap_p->Height - 1)) ||
                ((DstAutoClipRectangle.PositionX + (S32)SrcAutoClipRectangle.Width - 1) < 0) ||
                ((DstAutoClipRectangle.PositionY + (S32)SrcAutoClipRectangle.Height - 1) < 0))
            {
                if ((BlitContext_p->NotifyBlitCompletion == TRUE) && (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
                {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
                    /* Notify completion as required. */
                    STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
                }
                return(ST_NO_ERROR);
            }

            /* Automatical clipping to Dstbitmap boundaries  */
            if (Dst_p->Rectangle.PositionX < 0)
            {
                SrcAutoClipRectangle.Width     += DstAutoClipRectangle.PositionX;
                SrcAutoClipRectangle.PositionX -= DstAutoClipRectangle.PositionX;
                XOffset                        -= DstAutoClipRectangle.PositionX;
                DstAutoClipRectangle.PositionX = 0;
            }
            if (Dst_p->Rectangle.PositionY < 0)
            {
                SrcAutoClipRectangle.Height    += DstAutoClipRectangle.PositionY;
                SrcAutoClipRectangle.PositionY -= DstAutoClipRectangle.PositionY;
                YOffset                        -= DstAutoClipRectangle.PositionY;
                DstAutoClipRectangle.PositionY  = 0;
            }
            if ((DstAutoClipRectangle.PositionX + SrcAutoClipRectangle.Width -1) > (Dst_p->Bitmap_p->Width - 1))
            {
                SrcAutoClipRectangle.Width = Dst_p->Bitmap_p->Width - DstAutoClipRectangle.PositionX;
            }
            if((DstAutoClipRectangle.PositionY + SrcAutoClipRectangle.Height -1) > (Dst_p->Bitmap_p->Height - 1))
            {
                SrcAutoClipRectangle.Height = Dst_p->Bitmap_p->Height - DstAutoClipRectangle.PositionY;
            }

            /* Check if Auto clip rectangle outside completely Src Bitmap = > Nothing to copy */
            if ((SrcAutoClipRectangle.PositionX > (S32)(Src_p->Data.Bitmap_p->Width - 1))  ||
                (SrcAutoClipRectangle.PositionY > (S32)(Src_p->Data.Bitmap_p->Height - 1)) ||
                ((SrcAutoClipRectangle.PositionX + (S32)SrcAutoClipRectangle.Width - 1) < 0) ||
                ((SrcAutoClipRectangle.PositionY + (S32)SrcAutoClipRectangle.Height - 1) < 0))
            {
                if ((BlitContext_p->NotifyBlitCompletion == TRUE) && (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
                {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
                    /* Notify completion as required. */
                    STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
                }
                return(ST_NO_ERROR);
            }

            /* Update DstAutoClipRectangle size */
            DstAutoClipRectangle.Width  = SrcAutoClipRectangle.Width;
            DstAutoClipRectangle.Height = SrcAutoClipRectangle.Height;
        }
        else /* In case of resize, Auto clip rectangle calculation is not possible, ie follow h/w constraints */
        {
            /* Check Dst rectangle */
            if ((Dst_p->Rectangle.PositionX < 0)  ||
                (Dst_p->Rectangle.PositionY < 0))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (((Dst_p->Rectangle.PositionX + Dst_p->Rectangle.Width -1) > (Dst_p->Bitmap_p->Width - 1)) ||
                ((Dst_p->Rectangle.PositionY + Dst_p->Rectangle.Height -1) > (Dst_p->Bitmap_p->Height - 1)))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            /* Check Src rectangle */
            if ((Src_p->Rectangle.PositionX < 0) ||
                (Src_p->Rectangle.PositionY < 0))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            if (((Src_p->Rectangle.PositionX + Src_p->Rectangle.Width -1) > (Src_p->Data.Bitmap_p->Width - 1)) ||
                ((Src_p->Rectangle.PositionY + Src_p->Rectangle.Height -1) > (Src_p->Data.Bitmap_p->Height - 1)))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
        }
    }
    else /*  Src_p->Type == STBLIT_SOURCE_TYPE_COLOR */
    {
        /* Check if rectangle outside completely bitmap = > Nothing to fill */
        if ((Dst_p->Rectangle.PositionX > (S32)(Dst_p->Bitmap_p->Width - 1))  ||
            (Dst_p->Rectangle.PositionY > (S32)(Dst_p->Bitmap_p->Height - 1)) ||
            ((Dst_p->Rectangle.PositionX + (S32)Dst_p->Rectangle.Width - 1) < 0) ||
            ((Dst_p->Rectangle.PositionY + (S32)Dst_p->Rectangle.Height - 1) < 0))
        {
            if ((BlitContext_p->NotifyBlitCompletion == TRUE) && (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
            {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
                /* Notify completion as required. */
                STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
            }
            return(ST_NO_ERROR);
        }

        /* Automatical clipping to bitmap boundaries  */
        DstAutoClipRectangle = Dst_p->Rectangle;

        if (Dst_p->Rectangle.PositionX < 0)
        {
            DstAutoClipRectangle.Width     += Dst_p->Rectangle.PositionX;
            DstAutoClipRectangle.PositionX  = 0;
            XOffset                         = -Dst_p->Rectangle.PositionX;

        }
        if (Dst_p->Rectangle.PositionY < 0)
        {
            DstAutoClipRectangle.Height    += Dst_p->Rectangle.PositionY;
            DstAutoClipRectangle.PositionY  = 0;
            YOffset                         = -Dst_p->Rectangle.PositionY;

        }
        if ((DstAutoClipRectangle.PositionX + DstAutoClipRectangle.Width -1) > (Dst_p->Bitmap_p->Width - 1))
        {
            DstAutoClipRectangle.Width     = Dst_p->Bitmap_p->Width - DstAutoClipRectangle.PositionX;
        }
        if ((DstAutoClipRectangle.PositionY + DstAutoClipRectangle.Height -1) > (Dst_p->Bitmap_p->Height - 1))
        {
            DstAutoClipRectangle.Height = Dst_p->Bitmap_p->Height - DstAutoClipRectangle.PositionY;
        }

        SrcAutoClipRectangle=DstAutoClipRectangle;
    }

    /* At this stage the operation will be done by the h/W
     * However DstAutoClipRectangle, SrcAutoClipRectangle (in case Src=Bitmap) and MaskAutoClipRectangle (in case of bitmap
     * mask operation) must be used. All those rectangles fit entireley in their bitmaps */
    Src2.Rectangle  =  SrcAutoClipRectangle;
    Dest.Rectangle  =  DstAutoClipRectangle;

    /* Check Dst rectangle (h/w constraints)*/
    if ((DstAutoClipRectangle.Width > STBLIT_MAX_WIDTH)  || /* 12 bit hw constraint */
        (DstAutoClipRectangle.Height > STBLIT_MAX_HEIGHT))  /* 12 bit HW constraints */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check Src */
    if (Src_p->Type == STBLIT_SOURCE_TYPE_COLOR)   /* Src is a color */
    {
        SrcColorType = Src_p->Data.Color_p->Type;
        stblit_GetIndex(SrcColorType,&SrcIndex, STGXOBJ_ITU_R_BT601);  /* 601 valid only in ARGB Colors! TBDiscussed */
    }
    else   /* Src is a bitmap */
    {
        /* Check Src rectangle */
        if ((SrcAutoClipRectangle.Width > STBLIT_MAX_WIDTH)  || /* 12 bit hw constraint */
            (SrcAutoClipRectangle.Height > STBLIT_MAX_HEIGHT))  /* 12 bit HW constraints */
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }


        SrcColorType = Src_p->Data.Bitmap_p->ColorType;
        if ((Src_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB) || (Src_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP))
        {
            SrcMB = TRUE;
        }
        stblit_GetIndex(SrcColorType, &SrcIndex,Src_p->Data.Bitmap_p->ColorSpaceConversion);
    }

    /* Test if color conversion can be done in one blitter pass */
    stblit_GetIndex(DstColorType, &DstIndex,Dst_p->Bitmap_p->ColorSpaceConversion);

    /* Default values for Src1 : => Dst */
    Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
    Src1.Data.Bitmap_p  = Dst_p->Bitmap_p;
    Src1.Rectangle      = DstAutoClipRectangle;
    Src1.Palette_p      = Dst_p->Palette_p;

    if (SrcMB == TRUE)
    {
        Code = stblit_TableMB[SrcIndex][DstIndex];
        Src1_p = NULL;
    }
    else if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)
    {
        /* Blend mode */
        Code = stblit_TableBlend[SrcIndex][DstIndex];
    }
#if defined(ST_7109) || defined(ST_7200)
    else if ((BlitContext_p->AluMode == STBLIT_ALU_COPY) || (BlitContext_p->AluMode == STBLIT_ALU_COPY_INVERT))
    {
        if ((BlitContext_p->EnableMaskWord == FALSE)&&
            (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
        {
            /* Single Src2 */
            Code   = stblit_TableSingleSrc2[SrcIndex][DstIndex];
            Src1_p = NULL;
        }
        else if (BlitContext_p->EnableMaskWord == TRUE)
        {
                Code = stblit_TableROP[SrcIndex][DstIndex];
        }
        else if (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST)
        {
                Code = stblit_TableSingleSrc1[SrcIndex][DstIndex];
                Src1_p = Src2_p;
                Src2_p = NULL;
        }
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_NOOP)  ||
             (BlitContext_p->AluMode == STBLIT_ALU_INVERT))
    {
        /* Single Src1 */
        Code   = stblit_TableSingleSrc1[DstIndex][DstIndex];   /* Src1 == Dst  */
        Src2_p = NULL;
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_CLEAR)  ||
             (BlitContext_p->AluMode == STBLIT_ALU_SET))
    {
        /* Only Dst is checked */
        /* Code = TableSet[DstIndex]; */
        Code = 0;
    }
    else /* (All other ROP) Or (COPY/COPY_INVERT with mask)*/
    {
        /* ROP */
        Code = stblit_TableROP[SrcIndex][DstIndex];
    }

#else  /* defined(ST_7109) || defined(ST_7200) */

#ifdef STBLIT_USE_COLOR_KEY_FOR_BDISP
    else if ((BlitContext_p->AluMode == STBLIT_ALU_COPY)&&
             (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
    {
        /* Single Src2 */
        Code   = stblit_TableSingleSrc2[SrcIndex][DstIndex];
        Src1_p = NULL;
    }
    else if (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST)
    {
            Code = stblit_TableSingleSrc1[SrcIndex][DstIndex];
            Src1_p = Src2_p;
            Src2_p = NULL;
    }
#else /* !STBLIT_USE_COLOR_KEY_FOR_BDISP */
    else if ( BlitContext_p->AluMode == STBLIT_ALU_COPY )
    {
        /* Single Src2 */
        Code   = stblit_TableSingleSrc2[SrcIndex][DstIndex];
        Src1_p = NULL;
    }
#endif /* STBLIT_USE_COLOR_KEY_FOR_BDISP */

#endif  /* defined(ST_7109) || defined(ST_7200) */

    if (((Code >> 15) == 1) ||
        ((((Code >> 14) & 0x1) == 1) && (BlitContext_p->EnableColorCorrection == FALSE)))
    {
        /* Conversion implies multipass or not supported by hardware */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (Src2_p != NULL)
    {
        /* Affect Calculated AutoClip rectangle to Src_p and Dst_p */
        Src2_p->Rectangle = SrcAutoClipRectangle;
        Dst_p->Rectangle = DstAutoClipRectangle;

        if ((Src2_p->Type == STBLIT_SOURCE_TYPE_BITMAP) && ( Src2_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM ))
        {
            STGXOBJ_Bitmap_t                    TopField_Src2Bitmap, BottomField_Src2Bitmap, TopField_DstBitmap, BottomField_DstBitmap;
            STBLIT_Source_t                     TopField_Src1, BottomField_Src1, TopField_Src2, BottomField_Src2;
            STBLIT_Destination_t                TopField_Dst, BottomField_Dst;
            BOOL                                SrcFirstLineIsTop;

            /* Check if the first source line is top line */
            if ( Src2.Rectangle.PositionY % 2 == 0 )
            {
                SrcFirstLineIsTop = TRUE;
            }
            else
            {
                SrcFirstLineIsTop = FALSE;
            }

            /* Source2 Initialization */
            TopField_Src2Bitmap = (*Src2.Data.Bitmap_p);
            TopField_Src2Bitmap.Height = ( TopField_Src2Bitmap.Height + 1 ) >> 1;  /* Top lines represent only half of Height */

            BottomField_Src2Bitmap = (*Src2.Data.Bitmap_p);
            BottomField_Src2Bitmap.Data1_p = BottomField_Src2Bitmap.Data2_p;
            BottomField_Src2Bitmap.Height = BottomField_Src2Bitmap.Height  >> 1; /* Bottom lines represent only half of Height */

            TopField_Src2.Type                 = Src2_p->Type;
            TopField_Src2.Data.Bitmap_p        = &TopField_Src2Bitmap;
            TopField_Src2.Rectangle.PositionX  = Src2_p->Rectangle.PositionX;
            TopField_Src2.Rectangle.PositionY  = ( Src2_p->Rectangle.PositionY + 1 ) >> 1;  /* Top line Position Y in mode field = half( Top line Position Y in mode frame )*/
            TopField_Src2.Rectangle.Width      = Src2_p->Rectangle.Width;
            if ( SrcFirstLineIsTop )
            {
                TopField_Src2.Rectangle.Height     = ( Src2_p->Rectangle.Height + 1 ) >> 1; /* Top lines represent only half of Height */
            }
            else
            {
                TopField_Src2.Rectangle.Height     = Src2_p->Rectangle.Height >> 1; /* Top lines represent only half of Height */
            }

            BottomField_Src2.Type                 = Src2_p->Type;
            BottomField_Src2.Data.Bitmap_p        = &BottomField_Src2Bitmap;
            BottomField_Src2.Rectangle.PositionX  = Src2_p->Rectangle.PositionX;
            BottomField_Src2.Rectangle.PositionY  = Src2_p->Rectangle.PositionY >> 1; /* Bottom line Position Y in mode field = half( Bottom line Position Y in mode frame )*/
            BottomField_Src2.Rectangle.Width      = Src2_p->Rectangle.Width;
            if ( SrcFirstLineIsTop )
            {
                BottomField_Src2.Rectangle.Height     = Src2_p->Rectangle.Height  >> 1; /* Bottom lines represent only half of Height */
            }
            else
            {
                BottomField_Src2.Rectangle.Height     = ( Src2_p->Rectangle.Height + 1 ) >> 1; /* Bottom lines represent only half of Height */
            }

            /* Target Initialization */
            TopField_DstBitmap = (*Dst_p->Bitmap_p);
            BottomField_DstBitmap = (*Dst_p->Bitmap_p);

            if ( SrcFirstLineIsTop )
            {
                BottomField_DstBitmap.Data1_p = (void*)((U32)BottomField_DstBitmap.Data1_p + BottomField_DstBitmap.Pitch);
            }
            else
            {
                TopField_DstBitmap.Data1_p = (void*)((U32)TopField_DstBitmap.Data1_p + TopField_DstBitmap.Pitch);
            }

            TopField_DstBitmap.Height = ( TopField_DstBitmap.Height + 1 ) >> 1;
            TopField_DstBitmap.Pitch = TopField_DstBitmap.Pitch * 2;

            BottomField_DstBitmap.Height = BottomField_DstBitmap.Height  >> 1;
            BottomField_DstBitmap.Pitch = BottomField_DstBitmap.Pitch * 2;

            TopField_Dst.Bitmap_p        = &TopField_DstBitmap;
            TopField_Dst.Rectangle.PositionX  = Dst_p->Rectangle.PositionX;
            TopField_Dst.Rectangle.PositionY  = Dst_p->Rectangle.PositionY >> 1;
            TopField_Dst.Rectangle.Width      = Dst_p->Rectangle.Width;
            if ( SrcFirstLineIsTop )
            {
                TopField_Dst.Rectangle.Height     = ( Dst_p->Rectangle.Height + 1 ) >> 1;
            }
            else
            {
                TopField_Dst.Rectangle.Height     = Dst_p->Rectangle.Height  >> 1;
            }

            BottomField_Dst.Bitmap_p        = &BottomField_DstBitmap;
            BottomField_Dst.Rectangle.PositionX  = Dst_p->Rectangle.PositionX;
            BottomField_Dst.Rectangle.PositionY  = Dst_p->Rectangle.PositionY >> 1;
            BottomField_Dst.Rectangle.Width      = Dst_p->Rectangle.Width;
            if ( SrcFirstLineIsTop )
            {
                BottomField_Dst.Rectangle.Height     = Dst_p->Rectangle.Height >> 1;
            }
            else
            {
                BottomField_Dst.Rectangle.Height     = ( Dst_p->Rectangle.Height + 1 ) >> 1;
            }

            if ( SrcFirstLineIsTop )
            {
                /* Top field list node generation */
                if ( Src1_p == NULL )
                {
                    Err = stblit_OnePassBlitOperation(Device_p, NULL, &TopField_Src2, &TopField_Dst, BlitContext_p, Code,
                                                    &TopField_NodeFirstHandle_p,&TopField_NodeLastHandle_p,&TopField_NumberNodes,&OperationCfg,SrcMB);
                }
                else
                {
                    TopField_Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
                    TopField_Src1.Data.Bitmap_p  = TopField_Dst.Bitmap_p;
                    TopField_Src1.Rectangle      = TopField_Dst.Rectangle;
                    TopField_Src1.Palette_p      = TopField_Dst.Palette_p;

                    Err = stblit_OnePassBlitOperation(Device_p, &TopField_Src1, &TopField_Src2, &TopField_Dst, BlitContext_p, Code,
                                                    &TopField_NodeFirstHandle_p,&TopField_NodeLastHandle_p,&TopField_NumberNodes,&OperationCfg,SrcMB);
                }

                if ( BottomField_Dst.Rectangle.Height > 1 )
                {
                    /* Bottom field list node generation */
                    if ( Src1_p == NULL )
                    {
                        Err = stblit_OnePassBlitOperation(Device_p, NULL , &BottomField_Src2, &BottomField_Dst, BlitContext_p, Code,
                                                        &BottomField_NodeFirstHandle_p,&BottomField_NodeLastHandle_p,&BottomField_NumberNodes,&OperationCfg,SrcMB);
                    }
                    else
                    {
                        BottomField_Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
                        BottomField_Src1.Data.Bitmap_p  = BottomField_Dst.Bitmap_p;
                        BottomField_Src1.Rectangle      = BottomField_Dst.Rectangle;
                        BottomField_Src1.Palette_p      = BottomField_Dst.Palette_p;

                        Err = stblit_OnePassBlitOperation(Device_p, &BottomField_Src1, &BottomField_Src2, &BottomField_Dst, BlitContext_p, Code,
                                                        &BottomField_NodeFirstHandle_p,&BottomField_NodeLastHandle_p,&BottomField_NumberNodes,&OperationCfg,SrcMB);
                    }

                    /* Desable node notif of first list */
                    stblit_DisableNodeNotif( (stblit_Node_t*)TopField_NodeLastHandle_p );

                    /* Link of the two node list (Top+Bottom) */
                    stblit_Connect2Nodes( Device_p, ( (stblit_Node_t*)TopField_NodeLastHandle_p ), ( (stblit_Node_t*)BottomField_NodeFirstHandle_p ) );
                    NodeFirstHandle_p = TopField_NodeFirstHandle_p;
                    NodeLastHandle_p = BottomField_NodeLastHandle_p;
                    NumberNodes = TopField_NumberNodes + BottomField_NumberNodes;
                }
                else
                {
                    /* Only top list */
                    NodeFirstHandle_p = TopField_NodeFirstHandle_p;
                    NodeLastHandle_p = TopField_NodeLastHandle_p;
                    NumberNodes = TopField_NumberNodes;
                }
            }
            else /* !SrcFirstLineIsTop */
            {
                /* Bottom field list node generation */
                if ( Src1_p == NULL )
                {
                    Err = stblit_OnePassBlitOperation(Device_p, NULL, &BottomField_Src2, &BottomField_Dst, BlitContext_p, Code,
                                                    &BottomField_NodeFirstHandle_p,&BottomField_NodeLastHandle_p,&BottomField_NumberNodes,&OperationCfg,SrcMB);
                }
                else
                {
                    BottomField_Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
                    BottomField_Src1.Data.Bitmap_p  = BottomField_Dst.Bitmap_p;
                    BottomField_Src1.Rectangle      = BottomField_Dst.Rectangle;
                    BottomField_Src1.Palette_p      = BottomField_Dst.Palette_p;

                    Err = stblit_OnePassBlitOperation(Device_p, &BottomField_Src1, &BottomField_Src2, &BottomField_Dst, BlitContext_p, Code,
                                                    &BottomField_NodeFirstHandle_p,&BottomField_NodeLastHandle_p,&BottomField_NumberNodes,&OperationCfg,SrcMB);
                }

                if ( TopField_Dst.Rectangle.Height > 1 )
                {
                    /* Top field list node generation */
                    if ( Src1_p == NULL )
                    {
                        Err = stblit_OnePassBlitOperation(Device_p, NULL , &TopField_Src2, &TopField_Dst, BlitContext_p, Code,
                                                        &TopField_NodeFirstHandle_p,&TopField_NodeLastHandle_p,&TopField_NumberNodes,&OperationCfg,SrcMB);
                    }
                    else
                    {
                        TopField_Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
                        TopField_Src1.Data.Bitmap_p  = TopField_Dst.Bitmap_p;
                        TopField_Src1.Rectangle      = TopField_Dst.Rectangle;
                        TopField_Src1.Palette_p      = TopField_Dst.Palette_p;

                        Err = stblit_OnePassBlitOperation(Device_p, &TopField_Src1, &TopField_Src2, &TopField_Dst, BlitContext_p, Code,
                                                        &TopField_NodeFirstHandle_p,&TopField_NodeLastHandle_p,&TopField_NumberNodes,&OperationCfg,SrcMB);
                    }

                    /* Desable node notif of first list */
                    stblit_DisableNodeNotif( (stblit_Node_t*)BottomField_NodeLastHandle_p );

                    /* Link of the two node list (Top+Bottom) */
                    stblit_Connect2Nodes( Device_p, ( (stblit_Node_t*)BottomField_NodeLastHandle_p ), ( (stblit_Node_t*)TopField_NodeFirstHandle_p ) );
                    NodeFirstHandle_p = BottomField_NodeFirstHandle_p;
                    NodeLastHandle_p = TopField_NodeLastHandle_p;
                    NumberNodes = BottomField_NumberNodes + TopField_NumberNodes;
                }
                else
                {
                    /* Only top list */
                    NodeFirstHandle_p = BottomField_NodeFirstHandle_p;
                    NodeLastHandle_p = BottomField_NodeLastHandle_p;
                    NumberNodes = BottomField_NumberNodes;
                }
            }
        }
        else /* Src2_p->Data.Bitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM */
        {
            Err = stblit_OnePassBlitOperation(Device_p,Src1_p, Src2_p, Dst_p, BlitContext_p, Code,
                                            &NodeFirstHandle_p,&NodeLastHandle_p,&NumberNodes,&OperationCfg,SrcMB);
        }
    }
    else /*Src2_p == 0*/
    {
        Err = stblit_OnePassBlitOperation(Device_p,Src1_p, Src2_p , Dst_p, BlitContext_p, Code,
                                            &NodeFirstHandle_p,&NodeLastHandle_p,&NumberNodes,&OperationCfg,SrcMB);
    }

    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }

    /* Add Node list into Job if needed or submit to back-end single blit*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end.*/
    {
        Err = stblit_PostSubmitMessage(Device_p,NodeFirstHandle_p, NodeLastHandle_p, FALSE, FALSE, NumberNodes);
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,(stblit_NodeHandle_t)NodeFirstHandle_p, (stblit_NodeHandle_t)NodeLastHandle_p, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, (stblit_NodeHandle_t)NodeFirstHandle_p, (stblit_NodeHandle_t)NodeLastHandle_p, NumberNodes, &OperationCfg);
    }

    return(Err);
 }

/*******************************************************************************
Name        : stblit_TwoInputBlit
Description : 2 input / 1 output blit operation without mask
Parameters  :
Assumptions :  All parameters not null
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_TwoInputBlit(stblit_Device_t*        Device_p,
                                    STBLIT_Source_t*        Src1_p,
                                    STBLIT_Source_t*        Src2_p,
                                    STBLIT_Destination_t*   Dst_p,
                                    STBLIT_BlitContext_t*   BlitContext_p)
 {
    ST_ErrorCode_t          Err             = ST_NO_ERROR;
    STGXOBJ_ColorType_t     DstColorType    = Dst_p->Bitmap_p->ColorType;
    STGXOBJ_ColorType_t     Src1ColorType;
    STGXOBJ_ColorType_t     Src2ColorType;
    U8                      Src2Index, DstIndex;
    U16                     Code = 0;
    U32                     NumberNodes;
     stblit_NodeHandle_t     *NodeFirstHandle_p, *NodeLastHandle_p;
    stblit_OperationConfiguration_t     OperationCfg;

    /* TOP/FIELD managment vars */
    U32                                 TopField_NumberNodes, BottomField_NumberNodes;
    stblit_NodeHandle_t                 *TopField_NodeFirstHandle_p, *TopField_NodeLastHandle_p,
                                        *BottomField_NodeFirstHandle_p, *BottomField_NodeLastHandle_p;


    /* Check Dst color type */
    if ( ( Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB ) || ( Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_TOP_BOTTOM ) ||
          (Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP))
    {
        /* Hardware not supported */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    /* Check Dst rectangle  */
    if ((Dst_p->Rectangle.Width == 0)    ||
        (Dst_p->Rectangle.Height == 0)   ||
        (Dst_p->Rectangle.Width > STBLIT_MAX_WIDTH)  || /* 12 bit hw constraint */
        (Dst_p->Rectangle.Height > STBLIT_MAX_HEIGHT) || /* 12 bit HW constraints */
        (Dst_p->Rectangle.PositionX < 0) ||
        (Dst_p->Rectangle.PositionX < 0))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if (((Dst_p->Rectangle.PositionX + Dst_p->Rectangle.Width -1) > (Dst_p->Bitmap_p->Width - 1)) ||
        ((Dst_p->Rectangle.PositionY + Dst_p->Rectangle.Height -1) > (Dst_p->Bitmap_p->Height - 1)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

#if defined(ST_5105)
    /* Check if Src1 color type is (A)YCbCr and Dst color type is (A)RGB*/
    if ( ((Src1_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444) ||
          (Src1_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444) ||
          (Src1_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422) ||
          (Src1_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422) ||
          (Src1_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888) ||
          (Src1_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888)) &&
         ((Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_RGB888) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_RGB565) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB1555) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB4444) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888_255) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565_255)))
    {
        /* Hardware not supported */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check if Src1 color type is (A)YCbCr and Dst color type is (A)RGB*/
    if ( ((Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444) ||
          (Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444) ||
          (Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422) ||
          (Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422) ||
          (Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888) ||
          (Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888)) &&
         ((Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_RGB888) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_RGB565) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB1555) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB4444) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888_255) ||
          (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565_255)))
    {
        /* Hardware not supported */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

#endif


    if ((Src1_p->Type == STBLIT_SOURCE_TYPE_COLOR) && (Src2_p->Type == STBLIT_SOURCE_TYPE_COLOR))
    {
        /* Src1 color et Src2 color -> 2 pass */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    else if (Src1_p->Type == STBLIT_SOURCE_TYPE_COLOR)     /* Src2 bitmap and Src1 color */
    {
        /* Src1 (background) can not be a color if Src2 (foreground) is a bitmap */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    else if (Src2_p->Type == STBLIT_SOURCE_TYPE_COLOR)    /* Src2 color ant Src1 bitmap */
    {
         Src2_p->Rectangle = Dst_p->Rectangle;
        /* Check Src1 rectangle */
        if ((Src1_p->Rectangle.Width == 0)    ||
            (Src1_p->Rectangle.Height == 0)   ||
            (Src1_p->Rectangle.Width > STBLIT_MAX_WIDTH)  || /* 12 bit hw constraint */
            (Src1_p->Rectangle.Height > STBLIT_MAX_HEIGHT) || /* 12 bit HW constraints */
            (Src1_p->Rectangle.PositionX < 0) ||
            (Src1_p->Rectangle.PositionX < 0))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        if (((Src1_p->Rectangle.PositionX + Src1_p->Rectangle.Width -1) > (Src1_p->Data.Bitmap_p->Width - 1)) ||
            ((Src1_p->Rectangle.PositionY + Src1_p->Rectangle.Height -1) > (Src1_p->Data.Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }

        Src1ColorType = Src1_p->Data.Bitmap_p->ColorType;
        Src2ColorType = Src2_p->Data.Color_p->Type;
        stblit_GetIndex(Src2ColorType,&Src2Index, STGXOBJ_ITU_R_BT601); /* 601 valid only in ARGB Colors! TBDiscussed */


    }
    else   /* Src2 and Src1 are bitmap */
    {
        /* Check Src1 /Src2 rectangle */
        if ((Src1_p->Rectangle.Width == 0)                 ||
            (Src1_p->Rectangle.Height == 0)                ||
            (Src1_p->Rectangle.Width > STBLIT_MAX_WIDTH)   || /* 12 bit hw constraint */
            (Src1_p->Rectangle.Height > STBLIT_MAX_HEIGHT) || /* 12 bit HW constraints */
            (Src1_p->Rectangle.PositionX < 0)              ||
            (Src1_p->Rectangle.PositionX < 0)              ||
            (Src2_p->Rectangle.Width == 0)                 ||
            (Src2_p->Rectangle.Height == 0)                ||
            (Src2_p->Rectangle.Width > STBLIT_MAX_WIDTH)   || /* 12 bit hw constraint */
            (Src2_p->Rectangle.Height > STBLIT_MAX_HEIGHT) || /* 12 bit HW constraints */
            (Src2_p->Rectangle.PositionX < 0)              ||
            (Src2_p->Rectangle.PositionX < 0))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        if (((Src1_p->Rectangle.PositionX + Src1_p->Rectangle.Width -1) > (Src1_p->Data.Bitmap_p->Width - 1))   ||
            ((Src1_p->Rectangle.PositionY + Src1_p->Rectangle.Height -1) > (Src1_p->Data.Bitmap_p->Height - 1)) ||
            ((Src2_p->Rectangle.PositionX + Src2_p->Rectangle.Width -1) > (Src2_p->Data.Bitmap_p->Width - 1))   ||
            ((Src2_p->Rectangle.PositionY + Src2_p->Rectangle.Height -1) > (Src2_p->Data.Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }

        Src2ColorType = Src2_p->Data.Bitmap_p->ColorType;
        Src1ColorType = Src1_p->Data.Bitmap_p->ColorType;

        if ((Src1_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB) ||
            (Src2_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB) ||
            (Src1_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP) ||
            (Src2_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP))
        {
            /* If one of the source is MB, it means that it uses both hardware channels => Multipass if 2 blit inputs */
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }

        stblit_GetIndex(Src2ColorType, &Src2Index,Src2_p->Data.Bitmap_p->ColorSpaceConversion);
    }

    /* The driver only support Src1 Color Format = Dst Color format */
    if (Src1ColorType != DstColorType)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* The driver only support Src1 Area = Dst Area */
    /* if (stblit_PixelArea(&(Src1_p->Rectangle)) != stblit_PixelArea(&(Dst_p->Rectangle))) */
    if ((Src1_p->Rectangle.Width != Dst_p->Rectangle.Width) ||
        (Src1_p->Rectangle.Height != Dst_p->Rectangle.Height))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Test if color conversion can be done in one blitter pass */
    stblit_GetIndex(DstColorType, &DstIndex,Dst_p->Bitmap_p->ColorSpaceConversion );

    if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)
    {
        /* Blend mode */
         Code = stblit_TableBlend[Src2Index][DstIndex];
    }
#if defined(ST_7109) || defined (ST_7200)
    else if ((BlitContext_p->AluMode == STBLIT_ALU_COPY) || (BlitContext_p->AluMode == STBLIT_ALU_COPY_INVERT))
    {
        if ((BlitContext_p->EnableMaskWord == FALSE)&&
            (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
        {
            /* Single Src2 */
            Code   = stblit_TableSingleSrc2[Src2Index][DstIndex];
            Src1_p = NULL;
        }
        else if (BlitContext_p->EnableMaskWord == TRUE)
        {
                Code = stblit_TableROP[Src2Index][DstIndex];
        }
        else if (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST)
        {
                Code = stblit_TableSingleSrc1[Src2Index][DstIndex];
                Src1_p = Src2_p;
                Src2_p = NULL;
        }
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_NOOP)  ||
             (BlitContext_p->AluMode == STBLIT_ALU_INVERT))
    {
        /* Single Src1 */
        Code   = stblit_TableSingleSrc1[DstIndex][DstIndex];   /* Src1 == Dst  */
        Src2_p = NULL;
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_CLEAR)  ||
             (BlitContext_p->AluMode == STBLIT_ALU_SET))
    {
        /* Only Dst is checked */
        /* Code = TableSet[DstIndex]; */
        Code = 0;
    }
    else /* (All other ROP) Or (COPY/COPY_INVERT with mask)*/
    {
        /* ROP */
        Code = stblit_TableROP[Src2Index][DstIndex];
    }

#else /* defined (ST_7109) && defined (ST_7200) */
#ifdef STBLIT_USE_COLOR_KEY_FOR_BDISP
    else if ((BlitContext_p->AluMode == STBLIT_ALU_COPY)&&
             (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
    {
        /* Single Src2 */
        Code   = stblit_TableSingleSrc2[Src2Index][DstIndex];
        Src1_p = NULL;
    }
    else if (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST)
    {
        Code = stblit_TableSingleSrc1[Src2Index][DstIndex];
        Src1_p = Src2_p;
        Src2_p = NULL;
    }
#else /* !STBLIT_USE_COLOR_KEY_FOR_BDISP */
    else if ( BlitContext_p->AluMode == STBLIT_ALU_COPY )
    {
        /* Single Src2 */
        Code    = stblit_TableSingleSrc2[Src2Index][DstIndex];
        Src1_p  = NULL;
    }
#endif /* STBLIT_USE_COLOR_KEY_FOR_BDISP */
#endif /* defined (ST_7109) && defined (ST_7200) */

    if (((Code >> 15) == 1) ||
        ((((Code >> 14) & 0x1) == 1) && (BlitContext_p->EnableColorCorrection == FALSE)))
    {
        /* Conversion implies multipass or not supported by hardware */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (Src2_p != NULL)
    {
        if ( Src2_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM )
        {
            STGXOBJ_Bitmap_t                    TopField_Src2Bitmap, BottomField_Src2Bitmap, TopField_DstBitmap, BottomField_DstBitmap;
            STBLIT_Source_t                     TopField_Src1, BottomField_Src1, TopField_Src2, BottomField_Src2;
            STBLIT_Destination_t                TopField_Dst, BottomField_Dst;
            BOOL                                SrcFirstLineIsTop;

            /* Check if the first source line is top line */
            if ( Src2_p->Rectangle.PositionY % 2 == 0 )
            {
                SrcFirstLineIsTop = TRUE;
            }
            else
            {
                SrcFirstLineIsTop = FALSE;
            }

            /* Source2 Initialization */
            TopField_Src2Bitmap = (*Src2_p->Data.Bitmap_p);
            TopField_Src2Bitmap.Height = ( TopField_Src2Bitmap.Height + 1 ) >> 1;  /* Top lines represent only half of Height */

            BottomField_Src2Bitmap = (*Src2_p->Data.Bitmap_p);
            BottomField_Src2Bitmap.Data1_p = BottomField_Src2Bitmap.Data2_p;
            BottomField_Src2Bitmap.Height = BottomField_Src2Bitmap.Height  >> 1; /* Bottom lines represent only half of Height */

            TopField_Src2.Type                 = Src2_p->Type;
            TopField_Src2.Data.Bitmap_p        = &TopField_Src2Bitmap;
            TopField_Src2.Rectangle.PositionX  = Src2_p->Rectangle.PositionX;
            TopField_Src2.Rectangle.PositionY  = ( Src2_p->Rectangle.PositionY + 1 ) >> 1;  /* Top line Position Y in mode field = half( Top line Position Y in mode frame )*/
            TopField_Src2.Rectangle.Width      = Src2_p->Rectangle.Width;
            if ( SrcFirstLineIsTop )
            {
                TopField_Src2.Rectangle.Height     = ( Src2_p->Rectangle.Height + 1 ) >> 1; /* Top lines represent only half of Height */
            }
            else
            {
                TopField_Src2.Rectangle.Height     = Src2_p->Rectangle.Height >> 1; /* Top lines represent only half of Height */
            }

            BottomField_Src2.Type                 = Src2_p->Type;
            BottomField_Src2.Data.Bitmap_p        = &BottomField_Src2Bitmap;
            BottomField_Src2.Rectangle.PositionX  = Src2_p->Rectangle.PositionX;
            BottomField_Src2.Rectangle.PositionY  = Src2_p->Rectangle.PositionY >> 1; /* Bottom line Position Y in mode field = half( Bottom line Position Y in mode frame )*/
            BottomField_Src2.Rectangle.Width      = Src2_p->Rectangle.Width;
            if ( SrcFirstLineIsTop )
            {
                BottomField_Src2.Rectangle.Height     = Src2_p->Rectangle.Height  >> 1; /* Bottom lines represent only half of Height */
            }
            else
            {
                BottomField_Src2.Rectangle.Height     = ( Src2_p->Rectangle.Height + 1 ) >> 1; /* Bottom lines represent only half of Height */
            }

            /* Target Initialization */
            TopField_DstBitmap = (*Dst_p->Bitmap_p);
            BottomField_DstBitmap = (*Dst_p->Bitmap_p);

            if ( SrcFirstLineIsTop )
            {
                BottomField_DstBitmap.Data1_p = (void*)((U32)BottomField_DstBitmap.Data1_p + BottomField_DstBitmap.Pitch);
            }
            else
            {
                TopField_DstBitmap.Data1_p = (void*)((U32)TopField_DstBitmap.Data1_p + TopField_DstBitmap.Pitch);
            }

            TopField_DstBitmap.Height = ( TopField_DstBitmap.Height + 1 ) >> 1;
            TopField_DstBitmap.Pitch = TopField_DstBitmap.Pitch * 2;

            BottomField_DstBitmap.Height = BottomField_DstBitmap.Height  >> 1;
            BottomField_DstBitmap.Pitch = BottomField_DstBitmap.Pitch * 2;

            TopField_Dst.Bitmap_p        = &TopField_DstBitmap;
            TopField_Dst.Rectangle.PositionX  = Dst_p->Rectangle.PositionX;
            TopField_Dst.Rectangle.PositionY  = Dst_p->Rectangle.PositionY >> 1;
            TopField_Dst.Rectangle.Width      = Dst_p->Rectangle.Width;
            if ( SrcFirstLineIsTop )
            {
                TopField_Dst.Rectangle.Height     = ( Dst_p->Rectangle.Height + 1 ) >> 1;
            }
            else
            {
                TopField_Dst.Rectangle.Height     = Dst_p->Rectangle.Height  >> 1;
            }

            BottomField_Dst.Bitmap_p        = &BottomField_DstBitmap;
            BottomField_Dst.Rectangle.PositionX  = Dst_p->Rectangle.PositionX;
            BottomField_Dst.Rectangle.PositionY  = Dst_p->Rectangle.PositionY >> 1;
            BottomField_Dst.Rectangle.Width      = Dst_p->Rectangle.Width;
            if ( SrcFirstLineIsTop )
            {
                BottomField_Dst.Rectangle.Height     = Dst_p->Rectangle.Height >> 1;
            }
            else
            {
                BottomField_Dst.Rectangle.Height     = ( Dst_p->Rectangle.Height + 1 ) >> 1;
            }

            if ( SrcFirstLineIsTop )
			{
                /* Top field list node generation */
                TopField_Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
                TopField_Src1.Data.Bitmap_p  = TopField_Dst.Bitmap_p;
                TopField_Src1.Rectangle      = TopField_Dst.Rectangle;
                TopField_Src1.Palette_p      = TopField_Dst.Palette_p;

                Err = stblit_OnePassBlitOperation(Device_p, &TopField_Src1, &TopField_Src2, &TopField_Dst, BlitContext_p, Code,
                                                &TopField_NodeFirstHandle_p,&TopField_NodeLastHandle_p,&TopField_NumberNodes,&OperationCfg,FALSE);
                if ( BottomField_Dst.Rectangle.Height > 1 )
                {
                    /* Bottom field list node generation */
                    BottomField_Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
                    BottomField_Src1.Data.Bitmap_p  = BottomField_Dst.Bitmap_p;
                    BottomField_Src1.Rectangle      = BottomField_Dst.Rectangle;
                    BottomField_Src1.Palette_p      = BottomField_Dst.Palette_p;

                    Err = stblit_OnePassBlitOperation(Device_p, &BottomField_Src1, &BottomField_Src2, &BottomField_Dst, BlitContext_p, Code,
                                                    &BottomField_NodeFirstHandle_p,&BottomField_NodeLastHandle_p,&BottomField_NumberNodes,&OperationCfg,FALSE);

                    /* Desable node notif of first list */
                    stblit_DisableNodeNotif( (stblit_Node_t*)TopField_NodeLastHandle_p );

                    /* Link of the two node list (Top+Bottom) */
                    stblit_Connect2Nodes( Device_p, ( (stblit_Node_t*)TopField_NodeLastHandle_p ), ( (stblit_Node_t*)BottomField_NodeFirstHandle_p ) );
                    NodeFirstHandle_p = TopField_NodeFirstHandle_p;
                    NodeLastHandle_p = BottomField_NodeLastHandle_p;
                    NumberNodes = TopField_NumberNodes + BottomField_NumberNodes;
                }
                else
                {
                    /* Only top list */
                    NodeFirstHandle_p = TopField_NodeFirstHandle_p;
                    NodeLastHandle_p = TopField_NodeLastHandle_p;
                    NumberNodes = TopField_NumberNodes;
                }
            }
            else /* !SrcFirstLineIsTop */
            {
                /* Bottom field list node generation */
                BottomField_Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
                BottomField_Src1.Data.Bitmap_p  = BottomField_Dst.Bitmap_p;
                BottomField_Src1.Rectangle      = BottomField_Dst.Rectangle;
                BottomField_Src1.Palette_p      = BottomField_Dst.Palette_p;

                Err = stblit_OnePassBlitOperation(Device_p, &BottomField_Src1, &BottomField_Src2, &BottomField_Dst, BlitContext_p, Code,
                                                &BottomField_NodeFirstHandle_p,&BottomField_NodeLastHandle_p,&BottomField_NumberNodes,&OperationCfg,FALSE);

                if ( TopField_Dst.Rectangle.Height > 1 )
                {
                    /* Top field list node generation */
                    TopField_Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
                    TopField_Src1.Data.Bitmap_p  = TopField_Dst.Bitmap_p;
                    TopField_Src1.Rectangle      = TopField_Dst.Rectangle;
                    TopField_Src1.Palette_p      = TopField_Dst.Palette_p;

                    Err = stblit_OnePassBlitOperation(Device_p, &TopField_Src1, &TopField_Src2, &TopField_Dst, BlitContext_p, Code,
                                                    &TopField_NodeFirstHandle_p,&TopField_NodeLastHandle_p,&TopField_NumberNodes,&OperationCfg,FALSE);

                    /* Desable node notif of first list */
                    stblit_DisableNodeNotif( (stblit_Node_t*)BottomField_NodeLastHandle_p );

                    /* Link of the two node list (Top+Bottom) */
                    stblit_Connect2Nodes( Device_p, ( (stblit_Node_t*)BottomField_NodeLastHandle_p ), ( (stblit_Node_t*)TopField_NodeFirstHandle_p ) );
                    NodeFirstHandle_p = BottomField_NodeFirstHandle_p;
                    NodeLastHandle_p = TopField_NodeLastHandle_p;
                    NumberNodes = BottomField_NumberNodes + TopField_NumberNodes;
                }
                else
                {
                    /* Only top list */
                    NodeFirstHandle_p = BottomField_NodeFirstHandle_p;
                    NodeLastHandle_p = BottomField_NodeLastHandle_p;
                    NumberNodes = BottomField_NumberNodes;
                }
            }
        }
        else /* Src2_p->Data.Bitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM */
        {
            Err = stblit_OnePassBlitOperation(Device_p,Src1_p, Src2_p, Dst_p, BlitContext_p, Code,
                                            &NodeFirstHandle_p,&NodeLastHandle_p,&NumberNodes,&OperationCfg,FALSE);
        }
    }
    else
    {
        Err = stblit_OnePassBlitOperation(Device_p,Src1_p, Src2_p, Dst_p, BlitContext_p, Code,
                                        &NodeFirstHandle_p,&NodeLastHandle_p,&NumberNodes,&OperationCfg,FALSE);
    }


    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
        Err = stblit_PostSubmitMessage(Device_p,NodeFirstHandle_p, NodeLastHandle_p, FALSE, FALSE, NumberNodes);
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,(stblit_NodeHandle_t)NodeFirstHandle_p, (stblit_NodeHandle_t)NodeLastHandle_p, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, (stblit_NodeHandle_t)NodeFirstHandle_p, (stblit_NodeHandle_t)NodeLastHandle_p, NumberNodes, &OperationCfg);
    }

    return(Err);
 }


/*
--------------------------------------------------------------------------------
Blit API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Blit( STBLIT_Handle_t             Handle,
                            STBLIT_Source_t*            Src1_p,
                            STBLIT_Source_t*            Src2_p,
                            STBLIT_Destination_t*       Dst_p,
                            STBLIT_BlitContext_t*       BlitContext_p )
{
    ST_ErrorCode_t      Err         = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p;
    stblit_Job_t*       Job_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STBLIT_DEBUG_GET_STATISTICS
    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);

    /* Update statistics structure */
    Device_p->stblit_Statistics.Submissions++;
#ifndef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
    Device_p->GenerationStartTime = STOS_time_now();
#else /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    if(!Device_p->ExecutionRateCalculationStarted)
    {
        Device_p->ExecutionRateCalculationStarted = TRUE;
        Device_p->GenerationStartTime = STOS_time_now();
    }
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    Device_p->LatestBlitWidth = Src2_p->Rectangle.Width;
    Device_p->LatestBlitHeight = Src2_p->Rectangle.Height;

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */

    /* Check parameters */
    if ((BlitContext_p == NULL)                ||
        ((Src1_p == NULL) && (Src2_p == NULL)) ||
        (Dst_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check Blit context  */

#if !defined(ST_7109) && !defined(ST_7200) && !defined(STBLIT_USE_COLOR_KEY_FOR_BDISP)
    if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE)
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif /* !defined(ST_7109) && !defined(ST_7200)) && !defined(STBLIT_USE_COLOR_KEY_FOR_BDISP */

#if !defined(ST_7109) && !defined(ST_7200)
    if (BlitContext_p->EnableMaskWord == TRUE)
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if ( (BlitContext_p->AluMode != STBLIT_ALU_COPY) &&
         (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND) )
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif /*!defined(ST_7109) || !defined(ST_7200)*/

    if (BlitContext_p->EnableMaskBitmap == TRUE)
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {

        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }
    }

    /* Check that destination bitmap is not subbyte format (CLUT1/2/4) */
    if ( ( Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT1 ) ||
         ( Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT2 ) ||
         ( Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4 ) )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (Src2_p == NULL)   /* 1 source (src1) */
    {
        Err = stblit_OneInputBlit(Device_p, Src1_p, Dst_p, BlitContext_p);
    }
    else if (Src1_p == NULL)   /* 1 source (src2) */
    {
        Err = stblit_OneInputBlit(Device_p, Src2_p, Dst_p, BlitContext_p);
    }
    else   /* 2 sources  */
    {
        Err = stblit_TwoInputBlit(Device_p, Src1_p, Src2_p,  Dst_p, BlitContext_p);
    }

    return(Err);
}

/*
--------------------------------------------------------------------------------
Fill Rectangle API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_FillRectangle( STBLIT_Handle_t       Handle,
                                     STGXOBJ_Bitmap_t*        Bitmap_p,
                                     STGXOBJ_Rectangle_t*     Rectangle_p,
                                     STGXOBJ_Color_t*         Color_p,
                                     STBLIT_BlitContext_t* BlitContext_p )
{

    ST_ErrorCode_t                  Err         = ST_NO_ERROR;
    stblit_Unit_t*                  Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*                Device_p;
    stblit_Job_t*                   Job_p;
    stblit_OperationConfiguration_t OperationCfg;
    STGXOBJ_Rectangle_t             AutoClipRectangle;
    U32                             XOffset,YOffset;
    stblit_NodeHandle_t             *NodeFirstHandle_p, *NodeLastHandle_p;
    U32                             NumberNodes;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STBLIT_DEBUG_GET_STATISTICS
    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);

    /* Update statistics structure */
    Device_p->stblit_Statistics.Submissions++;
#ifndef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
    Device_p->GenerationStartTime = STOS_time_now();
#else /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    if(!Device_p->ExecutionRateCalculationStarted)
    {
        Device_p->ExecutionRateCalculationStarted = TRUE;
        Device_p->GenerationStartTime = STOS_time_now();
    }
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    Device_p->LatestBlitWidth = Rectangle_p->Width;
    Device_p->LatestBlitHeight = Rectangle_p->Height;

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */

    /* Check parameters */
    if ((BlitContext_p == NULL) ||
        (Bitmap_p == NULL)      ||
        (Color_p == NULL)       ||
        (Rectangle_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

#if !defined(ST_7109) && !defined(ST_7200) && !defined(STBLIT_USE_COLOR_KEY_FOR_BDISP)
    /* Check Blit context  */
    if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE)
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif /* (!defined(ST_7109) && !defined(ST_7200) && !defined(STBLIT_USE_COLOR_KEY_FOR_BDISP) */

    if ( (BlitContext_p->AluMode != STBLIT_ALU_COPY) &&
         (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND) )
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (BlitContext_p->EnableMaskWord == TRUE)
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (BlitContext_p->EnableMaskBitmap == TRUE)
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (BlitContext_p->EnableFlickerFilter == TRUE)
    {
        /* Feature not supported by Bdisp HW. */
     /*      return(ST_ERROR_FEATURE_NOT_SUPPORTED); yxl 2007-06-04 temp cancel*/

    }

    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {

        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }
    }

    /* Check that no color conversion */
    if ( Bitmap_p->ColorType != Color_p->Type )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check that destination bitmap is not subbyte format (CLUT1/2/4) */
    if ( ( Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT1 ) ||
         ( Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT2 ) ||
         ( Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4 ) )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check other supported blit context parameters */

    /* Check Dst rectangle size */
    if ((Rectangle_p->Width == 0)    ||
        (Rectangle_p->Height == 0))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check if rectangle outside completely bitmap = > Nothing to fill */
    if ((Rectangle_p->PositionX > (S32)(Bitmap_p->Width - 1))  ||
        (Rectangle_p->PositionY > (S32)(Bitmap_p->Height - 1)) ||
        ((Rectangle_p->PositionX + (S32)Rectangle_p->Width - 1) < 0) ||
        ((Rectangle_p->PositionY + (S32)Rectangle_p->Height - 1) < 0))
    {
        if ((BlitContext_p->NotifyBlitCompletion == TRUE) && (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
        {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
            /* Notify completion as required. */
            STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
        }
        return(ST_NO_ERROR);
    }

    /* Automatical clipping to bitmap boundaries  */
    AutoClipRectangle = *Rectangle_p;
    XOffset           = 0;
    YOffset           = 0;

    if (Rectangle_p->PositionX < 0)
    {
        AutoClipRectangle.Width     += Rectangle_p->PositionX;
        AutoClipRectangle.PositionX  = 0;
        XOffset                      = -Rectangle_p->PositionX;

    }
    if (Rectangle_p->PositionY < 0)
    {
        AutoClipRectangle.Height    += Rectangle_p->PositionY;
        AutoClipRectangle.PositionY  = 0;
        YOffset                      = -Rectangle_p->PositionY;

    }
    if ((AutoClipRectangle.PositionX + AutoClipRectangle.Width -1) > (Bitmap_p->Width - 1))
    {
        AutoClipRectangle.Width     = Bitmap_p->Width - AutoClipRectangle.PositionX;
    }
    if((AutoClipRectangle.PositionY + AutoClipRectangle.Height -1) > (Bitmap_p->Height - 1))
    {
        AutoClipRectangle.Height = Bitmap_p->Height - AutoClipRectangle.PositionY;
    }

    /* Check Dst rectangle max dimension  */
    if ((AutoClipRectangle.Width > STBLIT_MAX_WIDTH)  || /* 12 bit gamma hw constraint */
        (AutoClipRectangle.Height > STBLIT_MAX_HEIGHT))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Set Fill operation : All none supported features in the blit context are ignored */
    Err = stblit_OnePassFillOperation(Device_p, Bitmap_p, &AutoClipRectangle, Color_p, BlitContext_p,
                                      &NodeFirstHandle_p,&NodeLastHandle_p,&NumberNodes, &OperationCfg);

    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }


    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
        Err = stblit_PostSubmitMessage(Device_p,NodeFirstHandle_p, NodeLastHandle_p, FALSE, FALSE, NumberNodes);
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,(stblit_NodeHandle_t)NodeFirstHandle_p, (stblit_NodeHandle_t)NodeLastHandle_p, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, (stblit_NodeHandle_t)NodeFirstHandle_p, (stblit_NodeHandle_t)NodeLastHandle_p, NumberNodes, &OperationCfg);
    }


    return(Err);

}

/*
--------------------------------------------------------------------------------
Copy Rectangle API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_CopyRectangle( STBLIT_Handle_t       Handle,
                                     STGXOBJ_Bitmap_t*        SrcBitmap_p,
                                     STGXOBJ_Rectangle_t*     SrcRectangle_p,
                                     STGXOBJ_Bitmap_t*        DstBitmap_p,
                                     S32                   DstPositionX,
                                     S32                   DstPositionY,
                                     STBLIT_BlitContext_t* BlitContext_p )

{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*        Device_p;
    stblit_Job_t*           Job_p;
    stblit_OperationConfiguration_t   OperationCfg;
    STGXOBJ_Rectangle_t     AutoClipRectangle;
    stblit_NodeHandle_t     *NodeFirstHandle_p, *NodeLastHandle_p;
    U32                     NumberNodes;

    /* TOP/FIELD managment vars */
    U32                                 TopField_NumberNodes, BottomField_NumberNodes;
    stblit_NodeHandle_t                 *TopField_NodeFirstHandle_p, *TopField_NodeLastHandle_p,
                                        *BottomField_NodeFirstHandle_p, *BottomField_NodeLastHandle_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STBLIT_DEBUG_GET_STATISTICS
    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);

    /* Update statistics structure */
    Device_p->stblit_Statistics.Submissions++;
#ifndef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
    Device_p->GenerationStartTime = STOS_time_now();
#else /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    if(!Device_p->ExecutionRateCalculationStarted)
    {
        Device_p->ExecutionRateCalculationStarted = TRUE;
        Device_p->GenerationStartTime = STOS_time_now();
    }
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    Device_p->LatestBlitWidth = SrcRectangle_p->Width;
    Device_p->LatestBlitHeight = SrcRectangle_p->Height;

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */

    /* Check parameters */
    if ((BlitContext_p == NULL)  ||
        (SrcBitmap_p == NULL)    ||
        (SrcRectangle_p == NULL) ||
        (DstBitmap_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check Blit context  */
#if !defined(ST_7109) && !defined(ST_7200) && !defined(STBLIT_USE_COLOR_KEY_FOR_BDISP)
    if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE)
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif /* (!defined(ST_7109) || !defined(ST_7200)) && !defined(STBLIT_USE_COLOR_KEY_FOR_BDISP) */

    if ( (BlitContext_p->AluMode != STBLIT_ALU_COPY) &&
         (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND) )
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (BlitContext_p->EnableMaskWord == TRUE)
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (BlitContext_p->EnableMaskBitmap == TRUE)
    {
        /* Feature not supported by Bdisp HW. */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (BlitContext_p->EnableFlickerFilter == TRUE)
    {
        /* Feature not supported by Bdisp HW. */
  /*      return(ST_ERROR_FEATURE_NOT_SUPPORTED); yxl 2007-06-04 temp cancel*/
    }

    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {

        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }
    }

    /* Check Src rectangle size */
    if ((SrcRectangle_p->Width == 0)    ||
        (SrcRectangle_p->Height == 0))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check that no color conversion */
    if ( SrcBitmap_p->ColorType != DstBitmap_p->ColorType )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check that destination bitmap is not subbyte format (CLUT1/2/4) */
    if ( ( DstBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT1 ) ||
         ( DstBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT2 ) ||
         ( DstBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4 ) )
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    /* Check if src rectangle outside completely Src bitmap or Dst Bitmap = > Nothing to copy */
    if ((SrcRectangle_p->PositionX > (S32)(SrcBitmap_p->Width - 1))  ||
        (SrcRectangle_p->PositionY > (S32)(SrcBitmap_p->Height - 1)) ||
        ((SrcRectangle_p->PositionX + (S32)SrcRectangle_p->Width - 1) < 0) ||
        ((SrcRectangle_p->PositionY + (S32)SrcRectangle_p->Height - 1) < 0) ||
        (DstPositionX > (S32)(DstBitmap_p->Width - 1))  ||
        (DstPositionY > (S32)(DstBitmap_p->Height - 1)) ||
        ((DstPositionX + (S32)SrcRectangle_p->Width - 1) < 0) ||
        ((DstPositionY + (S32)SrcRectangle_p->Height - 1) < 0))
    {
        if ((BlitContext_p->NotifyBlitCompletion == TRUE) && (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
        {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
            /* Notify completion as required. */
            STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
        }
        return(ST_NO_ERROR);
    }

    /* Automatical clipping to Srcbitmap boundaries  */
    AutoClipRectangle = *SrcRectangle_p;

    if (SrcRectangle_p->PositionX < 0)
    {
        AutoClipRectangle.Width    += SrcRectangle_p->PositionX;
        DstPositionX               -= SrcRectangle_p->PositionX;
        AutoClipRectangle.PositionX = 0;
    }
    if (SrcRectangle_p->PositionY < 0)
    {
        AutoClipRectangle.Height   += SrcRectangle_p->PositionY;
        DstPositionY               -= SrcRectangle_p->PositionY;
        AutoClipRectangle.PositionY = 0;
    }
    if ((AutoClipRectangle.PositionX + AutoClipRectangle.Width -1) > (SrcBitmap_p->Width - 1))
    {
        AutoClipRectangle.Width = SrcBitmap_p->Width - AutoClipRectangle.PositionX;
    }
    if ((AutoClipRectangle.PositionY + AutoClipRectangle.Height -1) > (SrcBitmap_p->Height - 1))
    {
        AutoClipRectangle.Height = SrcBitmap_p->Height - AutoClipRectangle.PositionY;
    }

    /* Check if Auto clip rectangle outside completely Dst Bitmap = > Nothing to copy */
    if ((DstPositionX > (S32)(DstBitmap_p->Width - 1))  ||
        (DstPositionY > (S32)(DstBitmap_p->Height - 1)) ||
        ((DstPositionX + (S32)AutoClipRectangle.Width - 1) < 0) ||
        ((DstPositionY + (S32)AutoClipRectangle.Height - 1) < 0))
    {
        if ((BlitContext_p->NotifyBlitCompletion == TRUE) && (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
        {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
            /* Notify completion as required. */
            STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
        }
        return(ST_NO_ERROR);
    }

    /* Automatical clipping to Dstbitmap boundaries  */
    if (DstPositionX < 0)
    {
        AutoClipRectangle.Width     += DstPositionX;
        AutoClipRectangle.PositionX -= DstPositionX;
        DstPositionX                 = 0;
    }
    if (DstPositionY < 0)
    {
        AutoClipRectangle.Height    += DstPositionY;
        AutoClipRectangle.PositionY -= DstPositionY;
        DstPositionY                 = 0;
    }
    if ((DstPositionX + AutoClipRectangle.Width -1) > (DstBitmap_p->Width - 1))
    {
        AutoClipRectangle.Width = DstBitmap_p->Width - DstPositionX;
    }
    if((DstPositionY + AutoClipRectangle.Height -1) > (DstBitmap_p->Height - 1))
    {
        AutoClipRectangle.Height = DstBitmap_p->Height - DstPositionY;
    }

    /* Check if Auto clip rectangle outside completely Src Bitmap = > Nothing to copy */
    if ((AutoClipRectangle.PositionX > (S32)(SrcBitmap_p->Width - 1))  ||
        (AutoClipRectangle.PositionY > (S32)(SrcBitmap_p->Height - 1)) ||
        ((AutoClipRectangle.PositionX + (S32)AutoClipRectangle.Width - 1) < 0) ||
        ((AutoClipRectangle.PositionY + (S32)AutoClipRectangle.Height - 1) < 0))
    {
        if ((BlitContext_p->NotifyBlitCompletion == TRUE) && (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
        {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
            /* Notify completion as required. */
            STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
        }
        return(ST_NO_ERROR);
    }

    /* Check Src rectangle max dimensions (gamma/emulator contraints only )  */
    if ((SrcRectangle_p->Width > STBLIT_MAX_WIDTH)  || /* 12 bit gamma hw constraint */
        (SrcRectangle_p->Height > STBLIT_MAX_HEIGHT)) /* 12 bit gamma HW constraints */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if ( SrcBitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM )
    {
        STGXOBJ_Bitmap_t                    TopField_Src2Bitmap, BottomField_Src2Bitmap, TopField_DstBitmap, BottomField_DstBitmap;
        S32                                 TopField_DstPositionX, TopField_DstPositionY, BottomField_DstPositionX, BottomField_DstPositionY;
        STGXOBJ_Rectangle_t                 TopField_AutoClipRectangle, BottomField_AutoClipRectangle;
        BOOL                                SrcFirstLineIsTop;

        /* Check if the first source line is top line */
        if ( AutoClipRectangle.PositionY % 2 == 0 )
        {
            SrcFirstLineIsTop = TRUE;
        }
        else
        {
            SrcFirstLineIsTop = FALSE;
        }


        /* Source2 Initialization */
        TopField_Src2Bitmap = (*SrcBitmap_p);
        TopField_Src2Bitmap.Height = ( TopField_Src2Bitmap.Height + 1 ) >> 1;  /* Top lines represent only half of Height */

        BottomField_Src2Bitmap = (*SrcBitmap_p);
        BottomField_Src2Bitmap.Data1_p = BottomField_Src2Bitmap.Data2_p;
        BottomField_Src2Bitmap.Height = BottomField_Src2Bitmap.Height  >> 1; /* Bottom lines represent only half of Height */

        TopField_AutoClipRectangle.PositionX  = AutoClipRectangle.PositionX;
        TopField_AutoClipRectangle.PositionY  = ( AutoClipRectangle.PositionY + 1 ) >> 1;
        TopField_AutoClipRectangle.Width      = AutoClipRectangle.Width;
        if ( SrcFirstLineIsTop )
        {
            TopField_AutoClipRectangle.Height     = ( AutoClipRectangle.Height + 1 ) >> 1; /* Top lines represent only half of Height */
        }
        else
        {
            TopField_AutoClipRectangle.Height     = AutoClipRectangle.Height >> 1; /* Top lines represent only half of Height */
        }

        BottomField_AutoClipRectangle.PositionX  = AutoClipRectangle.PositionX;
        BottomField_AutoClipRectangle.PositionY  = AutoClipRectangle.PositionY >> 1;
        BottomField_AutoClipRectangle.Width      = AutoClipRectangle.Width;
        if ( SrcFirstLineIsTop )
        {
            BottomField_AutoClipRectangle.Height     = AutoClipRectangle.Height  >> 1; /* Bottom lines represent only half of Height */
        }
        else
        {
            BottomField_AutoClipRectangle.Height     = ( AutoClipRectangle.Height + 1 )  >> 1; /* Bottom lines represent only half of Height */
        }

        /* Target Initialization */
        TopField_DstBitmap = (*DstBitmap_p);
        BottomField_DstBitmap = (*DstBitmap_p);
        if ( SrcFirstLineIsTop )
        {
            BottomField_DstBitmap.Data1_p = (void*)((U32)BottomField_DstBitmap.Data1_p + BottomField_DstBitmap.Pitch);
        }
        else
        {
            TopField_DstBitmap.Data1_p = (void*)((U32)TopField_DstBitmap.Data1_p + TopField_DstBitmap.Pitch);
        }

        TopField_DstBitmap.Height = ( TopField_DstBitmap.Height + 1 ) >> 1;
        TopField_DstBitmap.Pitch = TopField_DstBitmap.Pitch * 2;
        BottomField_DstBitmap.Height = BottomField_DstBitmap.Height  >> 1;
        BottomField_DstBitmap.Pitch = BottomField_DstBitmap.Pitch * 2;

        TopField_DstPositionX  = DstPositionX;
        TopField_DstPositionY  = DstPositionY >> 1;

        BottomField_DstPositionX  = DstPositionX;
        BottomField_DstPositionY  = DstPositionY >> 1;

        /* Set Copy operation : All none supported features in the blit context are ignored (Mask, color key, filter, ALU Mode) */
        if ( SrcFirstLineIsTop )
        {
            /* Top field list node generation */
            Err = stblit_OnePassCopyOperation(Device_p, &TopField_Src2Bitmap, &TopField_AutoClipRectangle, &TopField_DstBitmap, TopField_DstPositionX, TopField_DstPositionY,
                                            BlitContext_p,&TopField_NodeFirstHandle_p,&TopField_NodeLastHandle_p,&TopField_NumberNodes, &OperationCfg);

            if ( BottomField_AutoClipRectangle.Height > 1 )
            {
                /* Bottom field list node generation */
                Err = stblit_OnePassCopyOperation(Device_p, &BottomField_Src2Bitmap, &BottomField_AutoClipRectangle, &BottomField_DstBitmap, BottomField_DstPositionX, BottomField_DstPositionY,
                                                BlitContext_p,&BottomField_NodeFirstHandle_p,&BottomField_NodeLastHandle_p,&BottomField_NumberNodes, &OperationCfg);

                /* Desable node notif of first list */
                stblit_DisableNodeNotif( (stblit_Node_t*)TopField_NodeLastHandle_p );

                /* Link of the two node list (Top+Bottom) */
                stblit_Connect2Nodes( Device_p, ( (stblit_Node_t*)TopField_NodeLastHandle_p ), ( (stblit_Node_t*)BottomField_NodeFirstHandle_p ) );
                NodeFirstHandle_p = TopField_NodeFirstHandle_p;
                NodeLastHandle_p = BottomField_NodeLastHandle_p;
                NumberNodes = TopField_NumberNodes + BottomField_NumberNodes;
            }
            else
            {
                /* Only top list */
                NodeFirstHandle_p = TopField_NodeFirstHandle_p;
                NodeLastHandle_p = TopField_NodeLastHandle_p;
                NumberNodes = TopField_NumberNodes;
            }
        }
        else /* !SrcFirstLineIsTop */
        {
            /* Bottom field list node generation */
            Err = stblit_OnePassCopyOperation(Device_p, &BottomField_Src2Bitmap, &BottomField_AutoClipRectangle, &BottomField_DstBitmap, BottomField_DstPositionX, BottomField_DstPositionY,
                                            BlitContext_p,&BottomField_NodeFirstHandle_p,&BottomField_NodeLastHandle_p,&BottomField_NumberNodes, &OperationCfg);

            if ( TopField_AutoClipRectangle.Height > 1 )
            {
                /* Top field list node generation */
                Err = stblit_OnePassCopyOperation(Device_p, &TopField_Src2Bitmap, &TopField_AutoClipRectangle, &TopField_DstBitmap, TopField_DstPositionX, TopField_DstPositionY,
                                                BlitContext_p,&TopField_NodeFirstHandle_p,&TopField_NodeLastHandle_p,&TopField_NumberNodes, &OperationCfg);

                /* Desable node notif of first list */
                stblit_DisableNodeNotif( (stblit_Node_t*)BottomField_NodeLastHandle_p );

                /* Link of the two node list (Top+Bottom) */
                stblit_Connect2Nodes( Device_p, ( (stblit_Node_t*)BottomField_NodeLastHandle_p ), ( (stblit_Node_t*)TopField_NodeFirstHandle_p ) );
                NodeFirstHandle_p = BottomField_NodeFirstHandle_p;
                NodeLastHandle_p = TopField_NodeLastHandle_p;
                NumberNodes = TopField_NumberNodes + BottomField_NumberNodes;
            }
            else
            {
                /* Only bottom list */
                NodeFirstHandle_p = BottomField_NodeFirstHandle_p;
                NodeLastHandle_p = BottomField_NodeLastHandle_p;
                NumberNodes = BottomField_NumberNodes;
            }
        }
    }
    else /*SrcBitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM */
    {
        /* Set Copy operation : All none supported features in the blit context are ignored (Mask, color key, filter, ALU Mode) */
        Err = stblit_OnePassCopyOperation(Device_p, SrcBitmap_p, &AutoClipRectangle, DstBitmap_p, DstPositionX, DstPositionY,
                                          BlitContext_p,&NodeFirstHandle_p,&NodeLastHandle_p,&NumberNodes, &OperationCfg);
    }


    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }


    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
        Err = stblit_PostSubmitMessage(Device_p,NodeFirstHandle_p, NodeLastHandle_p, FALSE, FALSE, NumberNodes);
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,(stblit_NodeHandle_t)NodeFirstHandle_p, (stblit_NodeHandle_t)NodeLastHandle_p, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, (stblit_NodeHandle_t)NodeFirstHandle_p, (stblit_NodeHandle_t)NodeLastHandle_p, NumberNodes, &OperationCfg);
    }

    return(Err);
}



/*
--------------------------------------------------------------------------------
Draw Horizontal line API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_DrawHLine( STBLIT_Handle_t       Handle,
                                 STGXOBJ_Bitmap_t*     Bitmap_p,
                                 S32                   PositionX,
                                 S32                   PositionY,
                                 U32                   Length,
                                 STGXOBJ_Color_t*      Color_p,
                                 STBLIT_BlitContext_t* BlitContext_p )

{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*        Device_p;
    stblit_Job_t*           Job_p;
    STBLIT_Source_t         Src2;
    STBLIT_Destination_t    Dst;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    /* Check parameters */
    if ((BlitContext_p == NULL) ||
        (Bitmap_p == NULL)      ||
        (Length == 0)           ||
        (Color_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check Blit context  */
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }
    }

    /* Check that no color conversion */

    /* Check other supported blit context parameters */

    /* Check line */
    if ((Length > STBLIT_MAX_WIDTH) || /* 12 bit HW constraints */
        (PositionX < 0)  ||
        (PositionY < 0))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if (((PositionX + Length -1) > (Bitmap_p->Width - 1)) ||
         (PositionY > ((S32)(Bitmap_p->Height - 1))))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* First implementation : rely on STBLIT_Blit() */

    Src2.Type            = STBLIT_SOURCE_TYPE_COLOR;
    Src2.Data.Color_p    = Color_p;

    Dst.Bitmap_p    = Bitmap_p;
    Dst.Rectangle.PositionX    = PositionX;
    Dst.Rectangle.PositionY    = PositionY;
    Dst.Rectangle.Width        = Length;
    Dst.Rectangle.Height       = 1;
    Dst.Palette_p   = NULL;

    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,BlitContext_p );

    return(Err);
}

/*
--------------------------------------------------------------------------------
Draw Vertical line API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_DrawVLine( STBLIT_Handle_t       Handle,
                                 STGXOBJ_Bitmap_t*     Bitmap_p,
                                 S32                   PositionX,
                                 S32                   PositionY,
                                 U32                   Length,
                                 STGXOBJ_Color_t*      Color_p,
                                 STBLIT_BlitContext_t* BlitContext_p )

{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*        Device_p;
    stblit_Job_t*           Job_p;
    STBLIT_Source_t         Src2;
    STBLIT_Destination_t    Dst;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    /* Check parameters */
    if ((BlitContext_p == NULL) ||
        (Bitmap_p == NULL)      ||
        (Length == 0)           ||
        (Color_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check Blit context  */
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {

        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }
    }

    /* Check that no color conversion */

    /* Check other supported blit context parameters */

    /* Check line */
    if ((Length > STBLIT_MAX_HEIGHT) || /* 12 bit HW constraints */
        (PositionX < 0)  ||
        (PositionY < 0))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if (((PositionY + Length -1) > (Bitmap_p->Height - 1)) ||
         (PositionX > ((S32)(Bitmap_p->Width - 1))))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }



    /* First implementation : rely on STBLIT_Blit() */

    Src2.Type            = STBLIT_SOURCE_TYPE_COLOR;
    Src2.Data.Color_p    = Color_p;

    Dst.Bitmap_p    = Bitmap_p;
    Dst.Rectangle.PositionX    = PositionX;
    Dst.Rectangle.PositionY    = PositionY;
    Dst.Rectangle.Width        = 1;
    Dst.Rectangle.Height       = Length;
    Dst.Palette_p   = NULL;

    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,BlitContext_p );

    return(Err);
}


/*
--------------------------------------------------------------------------------
Set Pixel API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SetPixel(  STBLIT_Handle_t       Handle,
                                 STGXOBJ_Bitmap_t*     Bitmap_p,
                                 S32                   PositionX,
                                 S32                   PositionY,
                                 STGXOBJ_Color_t*      Color_p,
                                 STBLIT_BlitContext_t* BlitContext_p )

{
    ST_ErrorCode_t          Err = ST_NO_ERROR;
    STBLIT_Source_t         Src2;
    STBLIT_Destination_t    Dst;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Job_t*           Job_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    /* Check parameters */
    if ((BlitContext_p == NULL) ||
        (Bitmap_p == NULL)      ||
        (Color_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }


    /* Check Blit context  */
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {

        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }
    }

    /* First implementation : rely on STBLIT_Blit() */

    Src2.Type            = STBLIT_SOURCE_TYPE_COLOR;
    Src2.Data.Color_p    = Color_p;

    Dst.Bitmap_p    = Bitmap_p;
    Dst.Rectangle.PositionX    = PositionX;
    Dst.Rectangle.PositionY    = PositionY;
    Dst.Rectangle.Width        = 1;
    Dst.Rectangle.Height       = 1;
    Dst.Palette_p   = NULL;

    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,BlitContext_p );

    return(Err);
}


/*
--------------------------------------------------------------------------------
Get Pixel API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetPixel(  STBLIT_Handle_t       Handle,
                                 STGXOBJ_Bitmap_t*     Bitmap_p,
                                 S32                   PositionX,
                                 S32                   PositionY,
                                 STGXOBJ_Color_t*      Color_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    U8*                     ColorAddress_p;
    U8                      Tmp1, Tmp2;
    U8                      BitShift;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    /* Check parameters */
    if ((Color_p == NULL) ||
        (Bitmap_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check Bitmap parameters */
    if (Bitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED); /* ?*/
    }

    Color_p->Type = Bitmap_p->ColorType;

    if ((Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB8888) ||
        (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB8888_255))
    {
        ColorAddress_p  = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch)  + (PositionX  *  4));
        Color_p->Value.ARGB8888.B     = *(U8*)(ColorAddress_p);
        Color_p->Value.ARGB8888.G     = *(U8*)((U32)ColorAddress_p + 1);
        Color_p->Value.ARGB8888.R     = *(U8*)((U32)ColorAddress_p + 2);
        Color_p->Value.ARGB8888.Alpha = *(U8*)((U32)ColorAddress_p + 3);
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_RGB888)
    {
        ColorAddress_p  = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch)  + (PositionX * 3));
        Color_p->Value.RGB888.B = *(U8*)(ColorAddress_p);
        Color_p->Value.RGB888.G = *(U8*)((U32)ColorAddress_p + 1);
        Color_p->Value.RGB888.R = *(U8*)((U32)ColorAddress_p + 2);
    }
    else if ((Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB8565) ||
             (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB8565_255))
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 3));
        Tmp1 = *(U8*)(ColorAddress_p);
        Tmp2 = *(U8*)((U32)ColorAddress_p + 1);
        Color_p->Value.ARGB8565.B     = Tmp1 & 0x1F;
        Color_p->Value.ARGB8565.G     = (((Tmp1 & 0xE0) >> 5) | ((Tmp2 & 0x7) << 3)) & 0x3F;
        Color_p->Value.ARGB8565.R     = (Tmp2 & 0xF8) >> 3;
        Color_p->Value.ARGB8565.Alpha = *(U8*)((U32)ColorAddress_p + 2);
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_RGB565)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 2));
        Tmp1 = *(U8*)(ColorAddress_p);
        Tmp2 = *(U8*)((U32)ColorAddress_p + 1);
        Color_p->Value.RGB565.B = Tmp1 & 0x1F;
        Color_p->Value.RGB565.G = (((Tmp1 & 0xE0) >> 5) | ((Tmp2 & 0x7) << 3)) & 0x3F;
        Color_p->Value.RGB565.R = (Tmp2 & 0xF8) >> 3;
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB1555)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 2));
        Tmp1 = *(U8*)(ColorAddress_p);
        Tmp2 = *(U8*)((U32)ColorAddress_p + 1);
        Color_p->Value.ARGB1555.B = Tmp1 & 0x1F;
        Color_p->Value.ARGB1555.G = (((Tmp1 & 0xE0) >> 5) | ((Tmp2 & 0x3) << 3)) & 0x1F;
        Color_p->Value.ARGB1555.R = (Tmp2 & 0x7C) >> 2;
        Color_p->Value.ARGB1555.Alpha = (Tmp2 & 0x80) >> 7;
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB4444)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 2));
        Tmp1 = *(U8*)(ColorAddress_p);
        Tmp2 = *(U8*)((U32)ColorAddress_p + 1);
        Color_p->Value.ARGB4444.B = Tmp1 & 0xF;
        Color_p->Value.ARGB4444.G = (Tmp1 & 0xF0) >> 4;
        Color_p->Value.ARGB4444.R = Tmp2 & 0xF;
        Color_p->Value.ARGB4444.Alpha = (Tmp2 & 0xF0) >> 4;
    }
    else if ((Color_p->Type == STGXOBJ_COLOR_TYPE_ACLUT88)  ||
             (Color_p->Type == STGXOBJ_COLOR_TYPE_ACLUT88_255))
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 2));
        Color_p->Value.ACLUT88.PaletteEntry = *(U8*)(ColorAddress_p);
        Color_p->Value.ACLUT88.Alpha        = *(U8*)((U32)ColorAddress_p + 1);

    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT8)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + PositionX);
        Color_p->Value.CLUT8 = *(U8*)(ColorAddress_p);
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ACLUT44)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + PositionX );
        Tmp1 = *(U8*)(ColorAddress_p);
        Color_p->Value.ACLUT44.PaletteEntry = Tmp1 & 0xF;
        Color_p->Value.ACLUT44.Alpha        = (Tmp1 & 0xF0) >> 4;
    }
    else if ((Color_p->Type == STGXOBJ_COLOR_TYPE_ALPHA8)  ||
             (Color_p->Type == STGXOBJ_COLOR_TYPE_ALPHA8_255))
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + PositionX );
        Color_p->Value.ALPHA8 = *(U8*)(ColorAddress_p);
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_BYTE)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + PositionX );
        Color_p->Value.Byte = *(U8*)(ColorAddress_p);
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 3));

        Color_p->Value.SignedYCbCr888_444.Cb = *(S8*)(ColorAddress_p);
        Color_p->Value.SignedYCbCr888_444.Y  = *(U8*)((U32)ColorAddress_p + 1);
        Color_p->Value.SignedYCbCr888_444.Cr = *(S8*)((U32)ColorAddress_p + 2);
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 4));

        Color_p->Value.SignedAYCbCr8888.Cb = *(S8*)(ColorAddress_p);
        Color_p->Value.SignedAYCbCr8888.Y  = *(U8*)((U32)ColorAddress_p    + 1);
        Color_p->Value.SignedAYCbCr8888.Cr = *(S8*)((U32)ColorAddress_p    + 2);
        Color_p->Value.SignedAYCbCr8888.Alpha = *(U8*)((U32)ColorAddress_p + 3);

    }

    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 3));

        Color_p->Value.UnsignedYCbCr888_444.Cb = *(U8*)(ColorAddress_p);
        Color_p->Value.UnsignedYCbCr888_444.Y  = *(U8*)((U32)ColorAddress_p + 1);
        Color_p->Value.UnsignedYCbCr888_444.Cr = *(U8*)((U32)ColorAddress_p + 2);
    }

    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 4));

        Color_p->Value.UnsignedAYCbCr8888.Cb = *(U8*)(ColorAddress_p);
        Color_p->Value.UnsignedAYCbCr8888.Y  = *(U8*)((U32)ColorAddress_p     + 1);
        Color_p->Value.UnsignedAYCbCr8888.Cr = *(U8*)((U32)ColorAddress_p     + 2);
        Color_p->Value.UnsignedAYCbCr8888.Alpha = *(U8*)((U32)ColorAddress_p  + 3);

    }

    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)
    {
        if ((PositionX % 2) == 0)  /* Even Position */
        {
            ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 2));
            Color_p->Value.SignedYCbCr888_422.Y  = *(U8*)((U32)ColorAddress_p + 1);
        }
        else  /* odd position */
        {
            ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + ((PositionX - 1) * 2));
            Color_p->Value.SignedYCbCr888_422.Y  = *(U8*)((U32)ColorAddress_p + 3);
        }

        Color_p->Value.SignedYCbCr888_422.Cb = *(S8*)(ColorAddress_p);
        Color_p->Value.SignedYCbCr888_422.Cr = *(S8*)((U32)ColorAddress_p + 2);

    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
    {
        if ((PositionX % 2) == 0)  /* Even Position */
        {
            ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 2));
            Color_p->Value.UnsignedYCbCr888_422.Y  = *(U8*)((U32)ColorAddress_p + 1);
        }
        else  /* odd position */
        {
            ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + ((PositionX - 1) * 2));
            Color_p->Value.UnsignedYCbCr888_422.Y  = *(U8*)((U32)ColorAddress_p + 3);
        }

        Color_p->Value.UnsignedYCbCr888_422.Cb = *(U8*)(ColorAddress_p);
        Color_p->Value.UnsignedYCbCr888_422.Cr = *(U8*)((U32)ColorAddress_p + 2);
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT4)
    {
        ColorAddress_p  = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset + (PositionY * Bitmap_p->Pitch)  + (PositionX / 2));
        Tmp1 = *(U8*)(ColorAddress_p);
        BitShift = (PositionX % 2) * 4 ;

        if (Bitmap_p->SubByteFormat == STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB)
        {
            Color_p->Value.CLUT4 = (Tmp1 >> BitShift) & 0xF;
        }
        else  /* STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB   */
        {
            Color_p->Value.CLUT4 = (Tmp1 >> (4 - BitShift)) & 0xF;
        }
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ALPHA4)
    {
        ColorAddress_p  = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset + (PositionY * Bitmap_p->Pitch)  + (PositionX / 2));
        Tmp1 = *(U8*)(ColorAddress_p);
        BitShift = (PositionX % 2) * 4 ;

        if (Bitmap_p->SubByteFormat == STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB)
        {
            Color_p->Value.ALPHA4 = (Tmp1 >> BitShift) & 0xF;
        }
        else  /* STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB   */
        {
            Color_p->Value.ALPHA4 = (Tmp1 >> (4 - BitShift)) & 0xF;
        }
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT2)
    {
        ColorAddress_p  = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  +  (PositionY * Bitmap_p->Pitch)  + (PositionX / 4));
        Tmp1 = *(U8*)(ColorAddress_p);
        BitShift = (PositionX % 4) * 2 ;

        if (Bitmap_p->SubByteFormat == STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB)
        {
            Color_p->Value.CLUT2 = (Tmp1 >> BitShift) & 0x3;
        }
        else  /* STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB   */
        {
            Color_p->Value.CLUT2 = (Tmp1 >> (6 - BitShift)) & 0x3;
        }
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT1)
    {
        ColorAddress_p  = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  +  (PositionY * Bitmap_p->Pitch)  + (PositionX / 8));
        Tmp1 = *(U8*)(ColorAddress_p);
        BitShift = PositionX % 8;

        if (Bitmap_p->SubByteFormat == STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB)
        {
            Color_p->Value.CLUT1 = (Tmp1 >> BitShift) & 0x1;
        }
        else  /* STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB   */
        {
            Color_p->Value.CLUT1 = (Tmp1 >> (7 - BitShift)) & 0x1;
        }
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ALPHA1)
    {
        ColorAddress_p  = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  +  (PositionY * Bitmap_p->Pitch)  + (PositionX / 8));
        Tmp1 = *(U8*)(ColorAddress_p);
        BitShift = PositionX % 8;

        if (Bitmap_p->SubByteFormat == STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB)
        {
            Color_p->Value.ALPHA1 = (Tmp1 >> BitShift) & 0x1;
        }
        else  /* STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB   */
        {
            Color_p->Value.ALPHA1 = (Tmp1 >> (7 - BitShift)) & 0x1;
        }
    }
    else
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }



    return(Err);
}


/*
--------------------------------------------------------------------------------
Draw XY API function
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STBLIT_DrawXYArray( STBLIT_Handle_t       Handle,
                                   STGXOBJ_Bitmap_t*     Bitmap_p,
                                   STBLIT_XY_t*          XYArray_p,
                                   U32                   PixelsNumber,
                                   STGXOBJ_Color_t*      Color_p,
                                   STBLIT_BlitContext_t* BlitContext_p )
{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(Bitmap_p);
    UNUSED_PARAMETER(XYArray_p);
    UNUSED_PARAMETER(PixelsNumber);
    UNUSED_PARAMETER(Color_p);
    UNUSED_PARAMETER(BlitContext_p);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*
--------------------------------------------------------------------------------
Draw XYC API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_DrawXYCArray( STBLIT_Handle_t       Handle,
                                    STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STBLIT_XYC_t*         XYArray_p,
                                    U32                   PixelsNumber,
                                    STGXOBJ_Color_t*      ColorArray_p,
                                    STBLIT_BlitContext_t* BlitContext_p )
{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(Bitmap_p);
    UNUSED_PARAMETER(XYArray_p);
    UNUSED_PARAMETER(PixelsNumber);
    UNUSED_PARAMETER(ColorArray_p);
    UNUSED_PARAMETER(BlitContext_p);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*
--------------------------------------------------------------------------------
Draw XYL API function
--------------------------------------------------------------------------------
*/
#if 0 /*yxl 2007-07-30 modify below section*/

ST_ErrorCode_t STBLIT_DrawXYLArray( STBLIT_Handle_t       Handle,
                                    STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STBLIT_XYL_t*         XYLArray_p,
                                    U32                   SegmentsNumber,
                                    STGXOBJ_Color_t*      Color_p,
                                    STBLIT_BlitContext_t* BlitContext_p )
{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(Bitmap_p);
    UNUSED_PARAMETER(XYLArray_p);
    UNUSED_PARAMETER(SegmentsNumber);
    UNUSED_PARAMETER(Color_p);
    UNUSED_PARAMETER(BlitContext_p);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

#else


/*******************************************************************************
Name        : FormatColorForMemory
Description : Generate a raw 32bit color value for writing to the frame buffer.
              For formats whose pixel boundaries align to 32 bits, the value
              will be replicated to 32bpp
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t FormatColorForMemory(STGXOBJ_Color_t* ColorIn_p, U32* ValueOut_p, U32 *Bpp_p)
{
    STGXOBJ_ColorType_t     TypeIn  = ColorIn_p->Type ;
    STGXOBJ_ColorValue_t    ValueIn = ColorIn_p->Value;

    switch (TypeIn)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            *ValueOut_p = (U32)(((ValueIn.ARGB8888.Alpha & 0xFF) << 24) |
                                ((ValueIn.ARGB8888.R & 0xFF) << 16)     |
                                ((ValueIn.ARGB8888.G & 0xFF) << 8) |
                                 (ValueIn.ARGB8888.B & 0xFF));
            *Bpp_p = 32;
            break;
        case STGXOBJ_COLOR_TYPE_RGB565:
            *ValueOut_p = (U32)(((ValueIn.RGB565.R & 0x1F) << 11) |
                                ((ValueIn.RGB565.G & 0x3F) << 5) |
                                 (ValueIn.RGB565.B & 0x1F));
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB1555:
            *ValueOut_p = (U32)(((ValueIn.ARGB1555.Alpha & 0x1) << 15) |
                                ((ValueIn.ARGB1555.R & 0x1F) << 10)     |
                                ((ValueIn.ARGB1555.G & 0x1F) << 5) |
                                 (ValueIn.ARGB1555.B & 0x1F));
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            *ValueOut_p = (U32)(((ValueIn.ARGB4444.Alpha & 0xF) << 12) |
                                ((ValueIn.ARGB4444.R & 0xF) << 8) |
                                ((ValueIn.ARGB4444.G & 0xF) << 4) |
                                 (ValueIn.ARGB4444.B & 0xF));
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT44:
            *ValueOut_p = (U32)(((ValueIn.ACLUT44.Alpha & 0xF) << 4) |
                                 (ValueIn.ACLUT44.PaletteEntry & 0xF));
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 8;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT8:
        case STGXOBJ_COLOR_TYPE_BYTE:
            *ValueOut_p =  (U32)(ValueIn.CLUT8 & 0xFF);
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 8;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT88:
        case STGXOBJ_COLOR_TYPE_ACLUT88_255:
            *ValueOut_p = (U32)(((ValueIn.ACLUT88.Alpha & 0xFF) << 8) |
                                 (ValueIn.ACLUT88.PaletteEntry & 0xFF));
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA8:
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
            *ValueOut_p =  (U32)(ValueIn.ALPHA8 & 0xFF);
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 8;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
            *ValueOut_p =  (U32)(((ValueIn.UnsignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 8) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 24) |
                                  (ValueIn.UnsignedYCbCr888_444.Cb & 0xFF));
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
            *ValueOut_p =  (U32)(((ValueIn.SignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.SignedYCbCr888_444.Y  & 0xFF) << 8) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 24) |
                                  (ValueIn.SignedYCbCr888_444.Cb & 0xFF));
            *Bpp_p = 16;
            break;
#if 0
/* We dont support these modes in the SW implementations yet! */
        case STGXOBJ_COLOR_TYPE_RGB888:
            *ValueOut_p = (U32)(((ValueIn.RGB888.R & 0xFF) << 16) |
                                ((ValueIn.RGB888.G & 0xFF) << 8) |
                                 (ValueIn.RGB888.B & 0xFF));
            *Bpp_p = 24;
            break;
       case STGXOBJ_COLOR_TYPE_ARGB8565:
       case STGXOBJ_COLOR_TYPE_ARGB8565_255:
            *ValueOut_p = (U32)(((ValueIn.ARGB8565.Alpha & 0xFF) << 16) |
                                ((ValueIn.ARGB8565.R & 0x1F) << 11) |
                                ((ValueIn.ARGB8565.G & 0x3F) << 5) |
                                 (ValueIn.ARGB8565.B & 0x1F));
            *Bpp_p = 24;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT1:
            *ValueOut_p =  (U32)(ValueIn.CLUT1 & 0x1);
            *ValueOut_p |= *ValueOut_p << 1;
            *ValueOut_p |= *ValueOut_p << 2;
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 1;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT2:
            *ValueOut_p =  (U32)(ValueIn.CLUT2 & 0x3);
            *ValueOut_p |= *ValueOut_p << 2;
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 2;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            *ValueOut_p =  (U32)(ValueIn.CLUT4 & 0xF);
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 4;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            *ValueOut_p =  (U32)(ValueIn.ALPHA1 & 0x1);
            *ValueOut_p |= *ValueOut_p << 1;
            *ValueOut_p |= *ValueOut_p << 2;
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 1;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA4:
            *ValueOut_p =  (U32)(ValueIn.ALPHA4 & 0xF);
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 4;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
            *ValueOut_p =  (U32)(((ValueIn.UnsignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 8) |
                                  (ValueIn.UnsignedYCbCr888_444.Cb & 0xFF));
            *Bpp_p = 24;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
            *ValueOut_p =  (U32)(((ValueIn.SignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.SignedYCbCr888_444.Y  & 0xFF) << 8) |
                                  (ValueIn.SignedYCbCr888_444.Cb & 0xFF));
            *Bpp_p = 24;
            break;
#endif
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
}

ST_ErrorCode_t stblit_sw_xyl( stblit_Device_t*      Device_p,
                              STGXOBJ_Bitmap_t*     Bitmap_p,
                              STBLIT_XYL_t*         XYL_p,
                              U32                   Entries,
                              STGXOBJ_Color_t*      Color_p,
                              STBLIT_BlitContext_t* BlitContext_p )
{

    /* Assumption : ClipLeft, ClipTop are inside the rectangle, ClipRight, ClipBottom are ouside the rectangle */
    S32 ClipLeft, ClipRight, ClipTop, ClipBottom;
    S32 X1, X2;
    U32 Bpp, RawColor, i;
    U8  *Line_p;
    U32 ByteCount;
    U32 Tmp;
	U32 Address;

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* We dont support colour conversion of any kind */
    if (Bitmap_p->ColorType != Color_p->Type)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if (FormatColorForMemory(Color_p, &RawColor, &Bpp) != ST_NO_ERROR)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    /* Set default Clip to Bitmap size */
    ClipLeft = 0;
    ClipTop = 0;
    ClipRight = Bitmap_p->Width;
    ClipBottom = Bitmap_p->Height;

    /* Clip to Cliprect */
    if (BlitContext_p->EnableClipRectangle)
    {
        if (!BlitContext_p->WriteInsideClipRectangle)
        {
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
        }

        if (ClipLeft < BlitContext_p->ClipRectangle.PositionX)
            ClipLeft = BlitContext_p->ClipRectangle.PositionX;

        if (ClipTop < BlitContext_p->ClipRectangle.PositionY)
            ClipTop = BlitContext_p->ClipRectangle.PositionY;

        if (ClipRight > (S32)(BlitContext_p->ClipRectangle.PositionX + BlitContext_p->ClipRectangle.Width))
            ClipRight = BlitContext_p->ClipRectangle.PositionX + BlitContext_p->ClipRectangle.Width;

        if (ClipBottom > (S32)(BlitContext_p->ClipRectangle.PositionY + BlitContext_p->ClipRectangle.Height))
            ClipBottom = BlitContext_p->ClipRectangle.PositionY + BlitContext_p->ClipRectangle.Height;
    }

    /* At this point, we have a valid cliprect */
    for (i = 0; i < Entries; i++, XYL_p++)
    {
        /* Check span against clip rect */
        if (XYL_p->PositionY < ClipTop || XYL_p->PositionY >= ClipBottom)
            continue;

        X1 = XYL_p->PositionX;
        X2 = XYL_p->PositionX + XYL_p->Length;

        if (X1 < ClipLeft)
            X1 = ClipLeft;

        if (X2 > ClipRight)
            X2 = ClipRight;

        if (X2 <= X1)
            continue;

        /* Now need to fill between X1 and X2 */

        /*  Convert Bitmap data virtual address to Cpu one */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32)stblit_DeviceToCpu((void*)(Bitmap_p->Data1_p));

#else
       Tmp =  (U32)STAVMEM_VirtualToCPU((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping));
  /*       Tmp =  (void*)(Bitmap_p->Data1_p);*/

#endif

        /* Work out the pointer to the first byte and the byte width to fill */
        Line_p = ((U8 *)Tmp) + Bitmap_p->Offset + XYL_p->PositionY * Bitmap_p->Pitch + (X1 << (Bpp >> 4));
        ByteCount = (X2 - X1) << (Bpp >> 4);

        if (Color_p->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 ||
            Color_p->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
        {
            /* Compressed YUV modes have 3 bytes for even pixels, one byte for odd! */
            Line_p += (X1 & 1);
            ByteCount += (X2 & 1) - (X1 & 1);
        }

        /* Fill initial unaligned left strip. We have to worry about alignment
           and width in this case. */
        if (((U32)Line_p) & 3)
        {
            U32 nBytes = (((U32)Line_p) & 3);
            U32 Color = RawColor;

            /* Work out number of bytes to fill */
            nBytes = 4 - nBytes;
            if (ByteCount < nBytes)
                nBytes = ByteCount;

            if (nBytes == 2)
            {
                Line_p[0] = (Color >> 16) & 0xFF;
                Line_p[1] = Color >> 24;
                ByteCount -= 2;
                Line_p += 2;
            }
            else if (nBytes == 3)
            {
                Line_p[0] = (Color >> 8) & 0xFF;
                Line_p[1] = (Color >> 16) & 0xFF;
                Line_p[2] = Color >> 24;
                ByteCount -= 3;
                Line_p += 3;
            }
            else
            {
                Line_p[0] = Color >> 24;
                ByteCount -= 1;
                Line_p += 1;
            }
        }

        if (ByteCount >= 4) {
			U32 Count;
            U32 *Word_p = 0;

			/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
			Address = (U32)Line_p;

			Count = ByteCount >> 5;
#if 0 /*yxl 2007-07-30 modify below */
            *Word_p = Address;
#else
			Word_p=(U32 *)Line_p;
#endif /*end yxl 2007-07-30 modify below */

            /* Fill in 32 bytes with each loop. */
            while (Count)
            {
                Word_p[0] = RawColor;
                Word_p[1] = RawColor;
                Word_p[2] = RawColor;
                Word_p[3] = RawColor;
                Word_p[4] = RawColor;
                Word_p[5] = RawColor;
                Word_p[6] = RawColor;
                Word_p[7] = RawColor;
                Word_p += 8;
                Count--;
            }

            Count = (ByteCount & 0x1c) >> 2;
            while (Count)
            {
                Word_p[0] = RawColor;
                Word_p++;
                Count--;
            }

            /* Setup Bytecount and Line_p for final strip.*/
            Line_p += ByteCount & 0xFFFFFFFC;
            ByteCount &= 3;
        }

        if (ByteCount)
        {
            if (ByteCount == 2)
            {
				/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
				Address = (U32)Line_p;

				*(U16 *)Address = RawColor & 0xFFFF;
            }
            else if (ByteCount == 3)
            {
				/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
				Address = (U32)Line_p;

                *(U16 *)Address = RawColor & 0xFFFF;
                Line_p[2] = (RawColor >> 16) & 0xFF;
            }
            else
            {
                Line_p[0] = RawColor & 0xFF;
            }
        }



    }


    if (BlitContext_p->NotifyBlitCompletion)
    {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
        /* Notify completion as required. */
        STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                               BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
    }

    return ST_NO_ERROR;
}


ST_ErrorCode_t STBLIT_DrawXYLArray( STBLIT_Handle_t       Handle,
                                    STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STBLIT_XYL_t*         XYLArray_p,
                                    U32                   SegmentsNumber,
                                    STGXOBJ_Color_t*      Color_p,
                                    STBLIT_BlitContext_t* BlitContext_p )
{
    ST_ErrorCode_t                      Err         = ST_NO_ERROR;
    stblit_Unit_t*                      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*                    Device_p;
    stblit_Job_t*                       Job_p;
    U8                                  NumberNodes;
    stblit_NodeHandle_t                 NodeFirstHandle, NodeLastHandle;
    stblit_OperationConfiguration_t     OperationCfg;
    stblit_DataPoolDesc_t*              CurrentNodeDataPool_p;
    stblit_Node_t*                      PreviousNode_p;
    U32                                 i;
/*    U8                      ScanConfig;*/
    U32                                 XOffset, YOffset;
    U32                                 TXY, S1XY, S1SZ;
    stblit_CommonField_t                CommonField;
    STBLIT_XYL_t*                       XYLArrayFromCpu_p;


    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    /* Check parameters */
    if ((BlitContext_p == NULL) ||
        (Bitmap_p == NULL)      ||
        (XYLArray_p == NULL)    ||
        (Color_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Return if no element */
    if (SegmentsNumber == 0)
    {
        if ((BlitContext_p->NotifyBlitCompletion == TRUE) && (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
        {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
            /* Notify completion as required. */
            STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
        }
        return(ST_NO_ERROR);
    }

    /* Perform operation directly in case of softaware device */
 /*   if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE) &&
        (BlitContext_p->EnableMaskBitmap == FALSE) &&
        (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE) &&
        (BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
        (BlitContext_p->EnableMaskWord == FALSE) &&
        (BlitContext_p->EnableColorCorrection == FALSE) &&
        (BlitContext_p->EnableFlickerFilter == FALSE) &&
        (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))*/
    {
        /* Attempt to fill the array with an optimised software renderer.*/
        Err = stblit_sw_xyl(Device_p, Bitmap_p, XYLArray_p, SegmentsNumber, Color_p, BlitContext_p);
        if (Err == ST_NO_ERROR)
        {
            return(Err);
        }
		
    }
	
    return(Err);
}
#endif /*end yxl 2007-07-30 modify below section*/

/*
--------------------------------------------------------------------------------
Draw XYLC API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_DrawXYLCArray( STBLIT_Handle_t       Handle,
                                     STGXOBJ_Bitmap_t*     Bitmap_p,
                                     STBLIT_XYL_t*         XYLArray_p,
                                     U32                   SegmentsNumber,
                                     STGXOBJ_Color_t*      ColorArray_p,
                                     STBLIT_BlitContext_t* BlitContext_p )
{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(Bitmap_p);
    UNUSED_PARAMETER(XYLArray_p);
    UNUSED_PARAMETER(SegmentsNumber);
    UNUSED_PARAMETER(ColorArray_p);
    UNUSED_PARAMETER(BlitContext_p);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}



/*
--------------------------------------------------------------------------------
Concatenation color format API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Concat( STBLIT_Handle_t       Handle,
                              STGXOBJ_Bitmap_t*     SrcAlphaBitmap_p,
                              STGXOBJ_Rectangle_t*  SrcAlphaRectangle_p,
                              STGXOBJ_Bitmap_t*     SrcColorBitmap_p,
                              STGXOBJ_Rectangle_t*  SrcColorRectangle_p,
                              STGXOBJ_Bitmap_t*     DstBitmap_p,
                              STGXOBJ_Rectangle_t*  DstRectangle_p,
                              STBLIT_BlitContext_t* BlitContext_p)
{
    ST_ErrorCode_t                      Err         = ST_NO_ERROR;
    stblit_Unit_t*                      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*                    Device_p;
    stblit_Job_t*                       Job_p;
    stblit_NodeHandle_t                 NodeHandle;
    stblit_OperationConfiguration_t     OperationCfg;
    stblit_CommonField_t                CommonField;


    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STBLIT_DEBUG_GET_STATISTICS
    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);

    /* Update statistics structure */
    Device_p->stblit_Statistics.Submissions++;
#ifndef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
    Device_p->GenerationStartTime = STOS_time_now();
#else /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    if(!Device_p->ExecutionRateCalculationStarted)
    {
        Device_p->ExecutionRateCalculationStarted = TRUE;
        Device_p->GenerationStartTime = STOS_time_now();
    }
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    Device_p->LatestBlitWidth = SrcColorRectangle_p->Width;
    Device_p->LatestBlitHeight = SrcColorRectangle_p->Height;

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */

    /* Check parameters */
    if ((SrcAlphaBitmap_p == NULL) ||
        (SrcColorBitmap_p == NULL)||
        (SrcAlphaRectangle_p == NULL)||
        (SrcColorRectangle_p == NULL)||
        (DstRectangle_p == NULL) ||
        (DstBitmap_p == NULL) ||
        (BlitContext_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check Blit context  */
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }
    }
    /* Check Alpha bitmap */
    if (SrcAlphaBitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if ((SrcAlphaBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ALPHA1) &&
        (SrcAlphaBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ALPHA4) &&
        (SrcAlphaBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ALPHA8))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check Color bitmap */
    if (SrcColorBitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if ((SrcColorBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_RGB888) &&
        (SrcColorBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_RGB565) &&
        (SrcColorBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8) &&
        (SrcColorBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT4))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check Src Alpha rectangle */
    if ((SrcAlphaRectangle_p->Width == 0)       ||
        (SrcAlphaRectangle_p->Height == 0)      ||
        (SrcAlphaRectangle_p->Width > STBLIT_MAX_WIDTH)    || /* 12 bit HW constraints */
        (SrcAlphaRectangle_p->Height > STBLIT_MAX_HEIGHT)   || /* 12 bit HW constraints */
        (SrcAlphaRectangle_p->PositionX < 0)    ||
        (SrcAlphaRectangle_p->PositionY < 0))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (((SrcAlphaRectangle_p->PositionX + SrcAlphaRectangle_p->Width -1) > (SrcAlphaBitmap_p->Width - 1))   ||
        ((SrcAlphaRectangle_p->PositionY + SrcAlphaRectangle_p->Height -1) > (SrcAlphaBitmap_p->Height - 1)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(SrcAlphaBitmap_p->ColorType, SrcAlphaRectangle_p->Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
    /* Check Src Color rectangle */
    if ((SrcColorRectangle_p->Width == 0)     ||
        (SrcColorRectangle_p->Height == 0)    ||
        (SrcColorRectangle_p->Width > STBLIT_MAX_WIDTH)  ||  /* 12 bit HW constraints */
        (SrcColorRectangle_p->Height > STBLIT_MAX_HEIGHT) ||  /* 12 bit HW constraints */
        (SrcColorRectangle_p->PositionX < 0)  ||
        (SrcColorRectangle_p->PositionY < 0))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
     if(((SrcColorRectangle_p->PositionX + SrcColorRectangle_p->Width -1) > (SrcColorBitmap_p->Width - 1))   ||
        ((SrcColorRectangle_p->PositionY + SrcColorRectangle_p->Height -1) > (SrcColorBitmap_p->Height - 1)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(SrcColorBitmap_p->ColorType, SrcColorRectangle_p->Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
    /* Check Dst Bitmap */
    if ((DstBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8888) &&
        (DstBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8888_255) &&
        (DstBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8565) &&
        (DstBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8565_255) &&
        (DstBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB1555) &&
        (DstBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB4444) &&
        (DstBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ACLUT88) &&
        (DstBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ACLUT88_255) &&
        (DstBitmap_p->ColorType != STGXOBJ_COLOR_TYPE_ACLUT44))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check Dst rectangle */
    if ((DstRectangle_p->Width == 0)     ||
        (DstRectangle_p->Height == 0)    ||
        (DstRectangle_p->Width > STBLIT_MAX_WIDTH)  || /* 12 bit HW constraints */
        (DstRectangle_p->Height > STBLIT_MAX_HEIGHT) || /* 12 bit HW constraints */
        (DstRectangle_p->PositionX < 0)  ||
        (DstRectangle_p->PositionY < 0))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if (((DstRectangle_p->PositionX + DstRectangle_p->Width -1) > (DstBitmap_p->Width - 1))   ||
        ((DstRectangle_p->PositionY + DstRectangle_p->Height -1) > (DstBitmap_p->Height - 1)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(DstBitmap_p->ColorType, DstRectangle_p->Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif

    /* No resize allowed */
    if ((SrcAlphaRectangle_p->Width != DstRectangle_p->Width) ||
        (SrcAlphaRectangle_p->Height != DstRectangle_p->Height) ||
        (SrcColorRectangle_p->Width != DstRectangle_p->Width) ||
        (SrcColorRectangle_p->Height != DstRectangle_p->Height))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Initialize CommonField value */
    memset(&CommonField, 0, sizeof(stblit_CommonField_t));

#if defined(ST_7109) || defined(ST_7200)
    /* Configure node for the concatenation */
    Err = stblit_OnePassConcatOperation(Device_p, SrcAlphaBitmap_p,SrcAlphaRectangle_p, SrcColorBitmap_p,
                                        SrcColorRectangle_p, DstBitmap_p, DstRectangle_p, BlitContext_p,
                                        &NodeHandle, &OperationCfg,&CommonField);
#endif

    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }
#ifdef WA_SOFT_RESET_EACH_NODE
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
    {
        ((stblit_Node_t*)NodeHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
        /* Set Blit completion IT on node */
        CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
    }
#endif

    /* Update CommonField in Node*/
    stblit_WriteCommonFieldInNode(Device_p,(stblit_Node_t*)NodeHandle ,&CommonField);


#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeHandle, (void*)NodeHandle, FALSE, FALSE, 0);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeHandle,NodeHandle,1);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeHandle, NodeHandle, 1, &OperationCfg);
    }

    return(Err);

}

/*
--------------------------------------------------------------------------------
Get Optimal Rectangle API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetOptimalRectangle( STBLIT_Handle_t      Handle,
                                           STGXOBJ_Bitmap_t*       Bitmap_p,
                                           STGXOBJ_Rectangle_t*    Rectangle_p)
{

    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(Bitmap_p);
    UNUSED_PARAMETER(Rectangle_p);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*
--------------------------------------------------------------------------------
Get Bitmap allocation parameter API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetBitmapAllocParams(STBLIT_Handle_t                 Handle,
                                           STGXOBJ_Bitmap_t*               Bitmap_p,
                                           STGXOBJ_BitmapAllocParams_t*    Params1_p,
                                           STGXOBJ_BitmapAllocParams_t*    Params2_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    U32                     ByteWidth;
    U32                     Remainder;
    U32                     MBRow, MBCol;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    if ((Bitmap_p == NULL)  ||
        (Params1_p == NULL) ||
        (Params2_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
        if ((Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888) ||
            (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888_255))
        {
            Params1_p->Pitch                        =  4 * Bitmap_p->Width;                   /* Pitch is multiple of 4 byte */
            Params1_p->AllocBlockParams.Size        = Params1_p->Pitch  * Bitmap_p->Height;
            Params1_p->AllocBlockParams.Alignment   = 4;                                      /* Alignement on a 32 bit word */
            Params1_p->Offset                       = 0;
        }
        else if ((Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_RGB888)              ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565)            ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565_255)        ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444) ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444))
        {
            ByteWidth  = 3 * Bitmap_p->Width;
            Remainder  = ByteWidth % 4;
            if (Remainder == 0)
            {
                Params1_p->Pitch  = ByteWidth;
            }
            else
            {
                Params1_p->Pitch  = ByteWidth - Remainder + 4;
            }
            Params1_p->AllocBlockParams.Size        = Params1_p->Pitch  * Bitmap_p->Height;
            Params1_p->AllocBlockParams.Alignment   = 4;
            Params1_p->Offset                       = 0;

        }
        else if ((Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422) ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422))
        {
            if ((Bitmap_p->Width % 2) == 0)
            {
                Params1_p->Pitch  = Bitmap_p->Width * 2 ;
            }
            else
            {
                Params1_p->Pitch  = (Bitmap_p->Width + 1) * 2;
            }
            Params1_p->AllocBlockParams.Size        = Params1_p->Pitch  * Bitmap_p->Height;
            Params1_p->AllocBlockParams.Alignment   = 4;
            Params1_p->Offset                       = 0;
        }
        else if ((Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_RGB565)   ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB1555) ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB4444) ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ACLUT88)  ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ACLUT88_255))
        {
            Params1_p->Pitch                        = Bitmap_p->Width * 2;
            Params1_p->AllocBlockParams.Size        = Params1_p->Pitch  * Bitmap_p->Height;
            Params1_p->AllocBlockParams.Alignment   = 2;
            Params1_p->Offset                       = 0;
        }
        else if ((Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ACLUT44)     ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA8)      ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA8_255)  ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_BYTE)        ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT8))
        {
            Params1_p->Pitch                        = Bitmap_p->Width;
            Params1_p->AllocBlockParams.Size        = Params1_p->Pitch  * Bitmap_p->Height;
            Params1_p->AllocBlockParams.Alignment   = 1;
            Params1_p->Offset                       = 0;
        }
        else if ((Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4) ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA4))
        {
            if ((Bitmap_p->Width % 2) == 0)
            {
                Params1_p->Pitch  = Bitmap_p->Width / 2;
            }
            else
            {
                Params1_p->Pitch  = (Bitmap_p->Width / 2) + 1;
            }
            Params1_p->AllocBlockParams.Size        = Params1_p->Pitch  * Bitmap_p->Height;
            Params1_p->AllocBlockParams.Alignment   = 1;
            Params1_p->Offset                       = 0;
        }
        else if (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT2)
        {
            if ((Bitmap_p->Width % 4) == 0)
            {
                Params1_p->Pitch  = Bitmap_p->Width / 4;
            }
            else
            {
                Params1_p->Pitch  = (Bitmap_p->Width / 4) + 1;
            }
            Params1_p->AllocBlockParams.Size        = Params1_p->Pitch  * Bitmap_p->Height;
            Params1_p->AllocBlockParams.Alignment   = 1;
            Params1_p->Offset                       = 0;
        }
        else if ((Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT1) ||
                 (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA1))
        {
            if ((Bitmap_p->Width % 8) == 0)
            {
                Params1_p->Pitch  = Bitmap_p->Width / 8;
            }
            else
            {
                Params1_p->Pitch  = (Bitmap_p->Width / 8) + 1;
            }
            Params1_p->AllocBlockParams.Size        = Params1_p->Pitch  * Bitmap_p->Height;
            Params1_p->AllocBlockParams.Alignment   = 1;
            Params1_p->Offset                       = 0;
        }
        else  /* Other color format */
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
    else if ((Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB) || (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP)) /* Note that a MB is a 16*16 Pixel area */
    {
        if ((Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)   &&
            (Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422) &&
            (Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420)   &&
            (Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        else
        {
            /* MB Horizontal */
            if ((Bitmap_p->Width % 16) == 0)
            {
                MBRow = Bitmap_p->Width / 16 ;
            }
            else
            {
                MBRow = (Bitmap_p->Width / 16) + 1;
            }

            /* MB Vertical (MBCol must be even) */
            if ((Bitmap_p->Height % 16) == 0)
            {
                MBCol = Bitmap_p->Height / 16 ;
            }
            else
            {
                MBCol = (Bitmap_p->Height / 16) + 1;
            }
            if ((MBCol % 2) != 0)
            {
                MBCol++;
            }

            /* Luma buffer related */
            Params1_p->AllocBlockParams.Size      = MBRow * MBCol * 256;
            Params1_p->AllocBlockParams.Alignment = 1;
            Params1_p->Pitch                      = MBRow * 16;
            Params1_p->Offset                       = 0;

            if ((Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)  ||
                (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422))
            {
                /* Chroma buffer related */
                *Params2_p = *Params1_p;

            }
            else  /*  420 Signed or Unsigned  */
            {
                if (((MBRow % 2) == 1)  && ((MBCol % 4) != 0))
                {
                    /* Chroma buffer related */
                    Params2_p->AllocBlockParams.Size      = MBRow * (MBCol + 1) * 128;
                    Params2_p->AllocBlockParams.Alignment = 1;
                    Params2_p->Pitch                      = Params1_p->Pitch;
                    Params2_p->Offset                     = 0;
                }
                else
                {
                    /* Chroma buffer related */
                    Params2_p->AllocBlockParams.Size      = MBRow * MBCol * 128;
                    Params2_p->AllocBlockParams.Alignment = 1;
                    Params2_p->Pitch                      = Params1_p->Pitch;
                    Params2_p->Offset                     = 0;
                }
            }
        }
    }
    else
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
}

/*
--------------------------------------------------------------------------------
Get Palette allocation parameter API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetPaletteAllocParams(STBLIT_Handle_t                 Handle,
                                            STGXOBJ_Palette_t*              Palette_p,
                                            STGXOBJ_PaletteAllocParams_t*   Params_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    if ((Palette_p == NULL)  ||
        (Params_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (Palette_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888)
    {
        Params_p->AllocBlockParams.Alignment = 16;
        Params_p->AllocBlockParams.Size = 4 * (1 << Palette_p->ColorDepth);
    }
    else
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);

}

/*
--------------------------------------------------------------------------------
Get Bitmap Header Size API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetBitmapHeaderSize (STBLIT_Handle_t      Handle,
                                           STGXOBJ_Bitmap_t*    Bitmap_p,
                                           U32*                 HeaderSize_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }


    if ((Bitmap_p == NULL)  ||
        (HeaderSize_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *HeaderSize_p = 0;

    return(Err);
}


/*
--------------------------------------------------------------------------------
Get FilterFlicker Mode  API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetFlickerFilterMode( STBLIT_Handle_t                  Handle,
                                            STBLIT_FlickerFilterMode_t*      FlickerFilterMode_p)
{


    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(FlickerFilterMode_p);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);

}

/*--------------------------------------------------------------------------------
Set FilterFlicker Mode  API function
----------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SetFlickerFilterMode( STBLIT_Handle_t                  Handle,
                                            STBLIT_FlickerFilterMode_t       FlickerFilterMode)

{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(FlickerFilterMode);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

#ifdef STBLIT_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : STBLIT_GetStatistics
Description :
Parameters  : IN  : STBLIT Handle.
              OUT : Data Interface Parameters.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STBLIT_GetStatistics(STBLIT_Handle_t  Handle, STBLIT_Statistics_t  * const Statistics_p)
{
    ST_ErrorCode_t    Err         = ST_NO_ERROR;
    stblit_Unit_t*    Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*  Device_p;
#ifdef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
    U32               Temp;
#endif

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Device_p = Unit_p->Device_p ;

    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);


    /* Get statistics structure */
#ifndef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
    Statistics_p->Submissions                 = Device_p->stblit_Statistics.Submissions;
    Statistics_p->BlitCompletionInterruptions = Device_p->stblit_Statistics.BlitCompletionInterruptions;
    Statistics_p->CorrectTimeStatisticsValues = Device_p->stblit_Statistics.CorrectTimeStatisticsValues;
    Statistics_p->MinGenerationTime           = Device_p->stblit_Statistics.MinGenerationTime;
    Statistics_p->AverageGenerationTime       = Device_p->stblit_Statistics.AverageGenerationTime;
    Statistics_p->MaxGenerationTime           = Device_p->stblit_Statistics.MaxGenerationTime;
    Statistics_p->LatestBlitGenerationTime    = Device_p->stblit_Statistics.LatestBlitGenerationTime;
    Statistics_p->MinExecutionTime            = Device_p->stblit_Statistics.MinExecutionTime;
    Statistics_p->AverageExecutionTime        = Device_p->stblit_Statistics.AverageExecutionTime;
    Statistics_p->MaxExecutionTime            = Device_p->stblit_Statistics.MaxExecutionTime;
    Statistics_p->LatestBlitExecutionTime     = Device_p->stblit_Statistics.LatestBlitExecutionTime;
    Statistics_p->MinProcessingTime           = Device_p->stblit_Statistics.MinProcessingTime;
    Statistics_p->AverageProcessingTime       = Device_p->stblit_Statistics.AverageProcessingTime;
    Statistics_p->MaxProcessingTime           = Device_p->stblit_Statistics.MaxProcessingTime;
    Statistics_p->LatestBlitProcessingTime    = Device_p->stblit_Statistics.LatestBlitProcessingTime;
    Statistics_p->MinExecutionRate            = Device_p->stblit_Statistics.MinExecutionRate;
    Statistics_p->AverageExecutionRate        = Device_p->stblit_Statistics.AverageExecutionRate;
    Statistics_p->MaxExecutionRate            = Device_p->stblit_Statistics.MaxExecutionRate;
    Statistics_p->LatestBlitExecutionRate     = Device_p->stblit_Statistics.LatestBlitExecutionRate;
#else /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    Statistics_p->Submissions                 = Device_p->stblit_Statistics.Submissions;
    Statistics_p->BlitCompletionInterruptions = Device_p->stblit_Statistics.BlitCompletionInterruptions;
    Statistics_p->CorrectTimeStatisticsValues = 0;
    Statistics_p->MinGenerationTime           = 0;
    Statistics_p->MaxGenerationTime           = 0;
    Statistics_p->LatestBlitGenerationTime    = 0;
    Statistics_p->MinExecutionTime            = 0;
    Statistics_p->AverageExecutionTime        = 0;
    Statistics_p->MaxExecutionTime            = 0;
    Statistics_p->LatestBlitExecutionTime     = 0;
    Statistics_p->MinProcessingTime           = 0;

    Statistics_p->MaxProcessingTime           = 0;
    Statistics_p->LatestBlitProcessingTime    = 0;
    Statistics_p->MinExecutionRate            = 0;
    Statistics_p->MaxExecutionRate            = 0;
    Statistics_p->LatestBlitExecutionRate     = 0;

    /* Calculate AverageGenerationTime */
    Temp = STOS_time_minus(Device_p->GenerationEndTime, Device_p->GenerationStartTime);
    Temp = stblit_ConvertFromTicsToUs(Temp);
    Statistics_p->AverageGenerationTime = Temp / Device_p->stblit_Statistics.Submissions;

    /* Calculate AverageProcessingTime */
    Temp = STOS_time_minus(Device_p->ExecutionEndTime, Device_p->GenerationStartTime);
    Temp = stblit_ConvertFromTicsToUs(Temp);
    Statistics_p->AverageProcessingTime = Temp / Device_p->stblit_Statistics.Submissions;

    /* Calculate AverageExecutionRate */
    Statistics_p->AverageExecutionRate = ((Device_p->LatestBlitWidth * Device_p->LatestBlitHeight * 1000) / Statistics_p->AverageProcessingTime);
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);

    return(Err);
} /* End of STBLIT_GetStatistics() function */

/*******************************************************************************
Name        : STBLIT_ResetStatistics
Description :
Parameters  : IN  : STBLIT Handle.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STBLIT_ResetStatistics(STBLIT_Handle_t  Handle)
{
    ST_ErrorCode_t    Err         = ST_NO_ERROR;
    stblit_Unit_t*    Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*  Device_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);


    /* Reset statistics structure */
    Device_p->TotalGenerationTime = 0;
    Device_p->TotalExecutionTime = 0;
    Device_p->TotalProcessingTime = 0;
    Device_p->TotalExecutionRate = 0;
#ifdef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
    Device_p->ExecutionRateCalculationStarted = FALSE;
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    Device_p->stblit_Statistics.Submissions = 0;
    Device_p->stblit_Statistics.BlitCompletionInterruptions = 0;
    Device_p->stblit_Statistics.CorrectTimeStatisticsValues = 0;
    Device_p->stblit_Statistics.MinGenerationTime = 1000000;
    Device_p->stblit_Statistics.AverageGenerationTime = 0;
    Device_p->stblit_Statistics.MaxGenerationTime = 0;
    Device_p->stblit_Statistics.LatestBlitGenerationTime = 0;
    Device_p->stblit_Statistics.MinExecutionTime = 1000000;
    Device_p->stblit_Statistics.AverageExecutionTime = 0;
    Device_p->stblit_Statistics.MaxExecutionTime = 0;
    Device_p->stblit_Statistics.LatestBlitExecutionTime = 0;
    Device_p->stblit_Statistics.MinProcessingTime = 1000000;
    Device_p->stblit_Statistics.AverageProcessingTime = 0;
    Device_p->stblit_Statistics.MaxProcessingTime = 0;
    Device_p->stblit_Statistics.LatestBlitProcessingTime = 0;
    Device_p->stblit_Statistics.MinExecutionRate = 1000000;
    Device_p->stblit_Statistics.AverageExecutionRate = 0;
    Device_p->stblit_Statistics.MaxExecutionRate = 0;
    Device_p->stblit_Statistics.LatestBlitExecutionRate = 0;

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);

    return(Err);
} /* End of STBLIT_ResetStatistics() function */

#endif /* STBLIT_DEBUG_GET_STATISTICS */


/*
--------------------------------------------------------------------------------
Sync blitter API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_WaitAllBlitsCompleted( STBLIT_Handle_t  Handle )
{
    stblit_Unit_t*      Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p;

    Device_p = Unit_p->Device_p ;
    stblit_waitallblitscompleted( Device_p );
    return (ST_NO_ERROR);

}

#ifdef STBLIT_USE_MEMORY_TRACE
/*******************************************************************************
Name        : STBLIT_MemoryTraceDump
Description : Dump the containing of the memory into dump file
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STBLIT_MemoryTraceDump()
{
    MemoryTrace_DumpMemory();
    return (ST_NO_ERROR);
}
#endif /* STBLIT_USE_MEMORY_TRACE */

/*******************************************************************************
Name        : STBLIT_DisableAntiFlutter
Description : disable the anti-Flutter feature.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t STBLIT_DisableAntiFlutter(STBLIT_Handle_t    Handle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*                    Device_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    Device_p = Unit_p->Device_p;
    STOS_SemaphoreWait(Device_p->AccessControl);
    Device_p->AntiFlutterOn = FALSE;
    STOS_SemaphoreSignal( Device_p->AccessControl );

    return(Err);
}

/*******************************************************************************
Name        : STBLIT_EnableAntiFlutter
Description : enable the anti-Flutter feature.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t STBLIT_EnableAntiFlutter(STBLIT_Handle_t    Handle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*                    Device_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    Device_p = Unit_p->Device_p;
    STOS_SemaphoreWait(Device_p->AccessControl);
    Device_p->AntiFlutterOn = TRUE;
    STOS_SemaphoreSignal( Device_p->AccessControl );

    return(Err);
}

/*******************************************************************************
Name        : STBLIT_DisableAntiAliasing
Description : disable the anti-Aliasing feature.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t STBLIT_DisableAntiAliasing(STBLIT_Handle_t    Handle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*                    Device_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    Device_p = Unit_p->Device_p;
    STOS_SemaphoreWait(Device_p->AccessControl);
    Device_p->AntiAliasingOn = FALSE;
    STOS_SemaphoreSignal( Device_p->AccessControl );

    return(Err);
}

/*******************************************************************************
Name        : STBLIT_EnableAntiAliasing
Description : enable the anti-Aliasing feature.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t STBLIT_EnableAntiAliasing(STBLIT_Handle_t    Handle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*                    Device_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    Device_p = Unit_p->Device_p;
    STOS_SemaphoreWait(Device_p->AccessControl);
    Device_p->AntiAliasingOn = TRUE;
    STOS_SemaphoreSignal( Device_p->AccessControl );

    return(Err);
}

/* End of bdisp_fe.c */
