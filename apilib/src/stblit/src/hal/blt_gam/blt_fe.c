/*******************************************************************************

File name   : blt_fe.c

Description : front-end source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
20 Jun 2000        Created                                          TM
29 May 2006        Modified for WinCE platform						WinCE Team-ST Noida
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <string.h>
#endif
#include "stddefs.h"
#include "blt_init.h"
#include "blt_fe.h"
#include "blt_com.h"
#include "stblit.h"
#include "stgxobj.h"
#include "blt_ctl.h"
#include "blt_mem.h"
#include "blt_job.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "blt_pool.h"
#if !defined(ST_OS21) && !defined(ST_OSLINUX) && !defined(ST_OSWINCE)
	#include "ostime.h"
#endif /*End of ST_OS21 &&  ST_OSLINUX*/
#include "blt_sw.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */
#ifdef STBLIT_TEST_FRONTEND
    U32 STBLIT_FirstNodeAddress; /* Usefull when STBLIT_TEST_FRONTEND */
#endif

#ifdef STBLIT_BENCHMARK3
    STOS_Clock_t t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19, t20;
    STOS_Clock_t t21, t22, t23, t24, t25, t26, t27, t28, t29, t30, t31, t32, t33, t34, t35, t36;
#endif

#ifdef STBLIT_BENCHMARK2
    STOS_Clock_t t1, t2;
#endif

#ifdef STBLIT_BENCHMARK4
    STOS_Clock_t t1, t2;
#endif

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */
void stblit_waitallblitscompleted( stblit_Device_t* Device_p );

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
            (Mode == STGXOBJ_ITU_R_BT601) ? (*Index =  12) : (*Index =  18);
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
            (Mode == STGXOBJ_ITU_R_BT601) ? (*Index =  13) : (*Index =  19);
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
            (Mode == STGXOBJ_ITU_R_BT601) ? (*Index =  14) : (*Index =  20);
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
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
    U8                                  SrcIndex ,DstIndex, TmpIndex;
    U16                                 Code            = 0;
    U16                                 CodeMask        = 0;
    BOOL                                SrcMB           = FALSE;
    STBLIT_Source_t                     Src1;
    STBLIT_Source_t                     Src2;
    STBLIT_Destination_t                Dest;
    STBLIT_Source_t*                    Src1_p     = &Src1;
    STBLIT_Source_t*                    Src2_p     = &Src2;
    STBLIT_Destination_t*               Dest_p     = &Dest;
    STBLIT_Source_t*                    MaskSrc1_p = Src1_p;
    STBLIT_Source_t*                    MaskSrc2_p = Src2_p;
    STBLIT_Source_t                     Mask;
    STBLIT_Source_t                     SrcTmp;
    STBLIT_Destination_t                DstTmp;
    STGXOBJ_Palette_t                   MaskPalette;
    U8                                  NumberNodes;
    stblit_NodeHandle_t                 NodeFirstHandle, NodeLastHandle;
    STGXOBJ_Bitmap_t                    WorkBitmap;
    STGXOBJ_ColorType_t                 TmpColorType;
    U8                                  TmpNbBitPerPixel;
    U32                                 TmpBitWidth;
    stblit_OperationConfiguration_t     OperationCfg;
    stblit_CommonField_t                FirstNodeCommonField;
    stblit_CommonField_t                LastNodeCommonField;
    STGXOBJ_Rectangle_t                 SrcAutoClipRectangle, DstAutoClipRectangle;
    S32                                 DstPositionX, DstPositionY;
    STGXOBJ_Rectangle_t                 MaskAutoClipRectangle;
    U32                                 XOffset,YOffset;
#ifdef WA_FILL_OPERATION
    BOOL                                EnableIT = FALSE;
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
    BOOL                                IsVResizeOrFF = FALSE;
#endif

    /* Check Dst Color Type*/
    if (Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB)
    {
        /* Hardware not supported */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
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
    MaskAutoClipRectangle = BlitContext_p->MaskRectangle;
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

            /* Update Mask Rectangle according to AutoClip rectangle */
            if (BlitContext_p->EnableMaskBitmap == TRUE)
            {
                MaskAutoClipRectangle.PositionX  += XOffset;
                MaskAutoClipRectangle.PositionY  += YOffset;
                MaskAutoClipRectangle.Width       = SrcAutoClipRectangle.Width;
                MaskAutoClipRectangle.Height      = SrcAutoClipRectangle.Height;
            }

            /* Update DstAutoClipRectangle size */
            DstAutoClipRectangle.Width  = SrcAutoClipRectangle.Width;
            DstAutoClipRectangle.Height = SrcAutoClipRectangle.Height;

            /* Perform copy acceleration */
            if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE) &&
                (BlitContext_p->EnableMaskBitmap == FALSE) &&
                (BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
                (BlitContext_p->EnableMaskWord == FALSE) &&
                (BlitContext_p->EnableColorCorrection == FALSE) &&
                (BlitContext_p->EnableFlickerFilter == FALSE) &&
                (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
            {

                /* Attempt to fill the array with an optimised software renderer.*/
                Err = stblit_sw_copyrect(Device_p, Src_p->Data.Bitmap_p, &SrcAutoClipRectangle, Dst_p->Bitmap_p,
                                         DstAutoClipRectangle.PositionX, DstAutoClipRectangle.PositionY, BlitContext_p);
                if (Err == ST_NO_ERROR)
                {
                    return(Err);
                }
            }

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

        /* Update Mask Rectangle according to AutoClip rectangle */
        if (BlitContext_p->EnableMaskBitmap == TRUE)
        {
            MaskAutoClipRectangle.PositionX  += XOffset;
            MaskAutoClipRectangle.PositionY  += YOffset;
            MaskAutoClipRectangle.Width       = DstAutoClipRectangle.Width;
            MaskAutoClipRectangle.Height      = DstAutoClipRectangle.Height;
        }

        /* Perform operation directly in case of softaware device */
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE) &&
            (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE) &&
            (BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
            (BlitContext_p->EnableMaskWord == FALSE) &&
            (BlitContext_p->EnableColorCorrection == FALSE) &&
            (BlitContext_p->EnableFlickerFilter == FALSE) &&
            (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
        {
            /* Attempt to fill the rectangle with an optimised software renderer.*/
            Err = stblit_sw_fillrect(Device_p, Dst_p->Bitmap_p, &DstAutoClipRectangle,&MaskAutoClipRectangle, Src_p->Data.Color_p,
                                     BlitContext_p);
            if (Err == ST_NO_ERROR)
            {
                return (Err);
            }
        }
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
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(Dst_p->Bitmap_p->ColorType, DstAutoClipRectangle.Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif

    /* Check Src */
    if (Src_p->Type == STBLIT_SOURCE_TYPE_COLOR)   /* Src is a color */
    {
        SrcColorType = Src_p->Data.Color_p->Type;
        stblit_GetIndex(SrcColorType,&SrcIndex, STGXOBJ_ITU_R_BT601);  /* 601 valid only in ARGB Colors! TBDiscussed */
#ifdef WA_GNBvd09730
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))  &&
        ((SrcColorType == STGXOBJ_COLOR_TYPE_CLUT1)  ||
         (SrcColorType == STGXOBJ_COLOR_TYPE_CLUT2)  ||
         (SrcColorType == STGXOBJ_COLOR_TYPE_CLUT4)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
#ifdef WA_FILL_OPERATION
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
        {
            /* Interruption in case of color fill*/
            EnableIT = TRUE;
        }
#endif
#ifdef WA_GNBvd06658
        /* This WA may be useless since it is meaningless to have flicker in Fill color operation !*/
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            (BlitContext_p->EnableFlickerFilter == TRUE) &&
            (BlitContext_p->EnableClipRectangle == TRUE)&&
            (DstAutoClipRectangle.Width >= 128 ))      /* Src is the same size as Dst in case of fill color */
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        /* This WA may be useless since it is meaningless to have flicker in Fill color operation !*/
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||

             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            (BlitContext_p->EnableFlickerFilter == TRUE) &&
            (DstAutoClipRectangle.Width >= 128 ))    /* Src is the same size as Dst in case of fill color */
        {
            IsVResizeOrFF = TRUE;
        }
#endif
    }
    else   /* Src is a bitmap */
    {
#ifdef WA_HORIZONTAL_POSITION_ODD
      if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
          ((SrcAutoClipRectangle.PositionX % 2) == 0) &&
          ((Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    ||
           (Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)  ||
           (Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420)    ||
           (Src_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420)) &&
          ((SrcAutoClipRectangle.Height != DstAutoClipRectangle.Height) ||
           (BlitContext_p->EnableFlickerFilter == TRUE)) &&
          (SrcAutoClipRectangle.Width >= 128 ))
      {
        (SrcAutoClipRectangle.PositionX)++;
      }
#endif
        /* Check Src rectangle */
        if ((SrcAutoClipRectangle.Width > STBLIT_MAX_WIDTH)  || /* 12 bit hw constraint */
            (SrcAutoClipRectangle.Height > STBLIT_MAX_HEIGHT))  /* 12 bit HW constraints */
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
        Err = stblit_WA_CheckWidthByteLimitation(Src_p->Data.Bitmap_p->ColorType, SrcAutoClipRectangle.Width);
        if (Err != ST_NO_ERROR)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
#ifdef WA_GNBvd06658
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            ((SrcAutoClipRectangle.Height != DstAutoClipRectangle.Height) ||
             (BlitContext_p->EnableFlickerFilter == TRUE)) &&
            (BlitContext_p->EnableClipRectangle == TRUE)&&
            (SrcAutoClipRectangle.Width >= 128 ))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
          ((SrcAutoClipRectangle.Height != DstAutoClipRectangle.Height) ||
           (BlitContext_p->EnableFlickerFilter == TRUE)) &&
          (SrcAutoClipRectangle.Width >= 128 ))
        {
            IsVResizeOrFF = TRUE;
        }
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET
      if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
          ((SrcAutoClipRectangle.Height != DstAutoClipRectangle.Height) ||
           (BlitContext_p->EnableFlickerFilter == TRUE)) &&
          (SrcAutoClipRectangle.Width != DstAutoClipRectangle.Width) &&
          (SrcAutoClipRectangle.Width >= 128 ) &&
          ((Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422) ||
           (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)))
      {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
      }
#endif
#ifdef WA_GNBvd13838
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
        ((SrcAutoClipRectangle.Height != DstAutoClipRectangle.Height) ||
         (BlitContext_p->EnableFlickerFilter == TRUE)) &&
        (SrcAutoClipRectangle.Width != DstAutoClipRectangle.Width) &&
        (SrcAutoClipRectangle.Width >= 128 ) &&
        (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT1))
      {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
      }
#endif


        SrcColorType = Src_p->Data.Bitmap_p->ColorType;
        if (Src_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB)
        {
            SrcMB = TRUE;
#ifdef WA_GNBvd08376
           if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1) &&
               ((Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT1) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT2) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4)))
           {
               return(ST_ERROR_FEATURE_NOT_SUPPORTED);
           }

#endif
#ifdef WA_GNBvd15280
           if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1) ||
                (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
                (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
                (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
                (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)) &&
               ((Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB1555) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB4444) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ACLUT88)  ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ACLUT44) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8888_255) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565_255) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ACLUT88_255) ||
                (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA8_255)))
                /* ALPHA1, ALPHA4, ALPHA8 not supported for dst format when src is MB (see conversion table in design doc) */
           {
               return(ST_ERROR_FEATURE_NOT_SUPPORTED);
           }
#endif


            if ((BlitContext_p->EnableMaskBitmap == TRUE) ||
                ((BlitContext_p->AluMode != STBLIT_ALU_COPY) &&
                 (BlitContext_p->AluMode != STBLIT_ALU_COPY_INVERT) &&
                 (BlitContext_p->AluMode != STBLIT_ALU_SET) &&
                 (BlitContext_p->AluMode != STBLIT_ALU_CLEAR)))
            {
                /* Feature not supported by HW in MB mode. */
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
        }
        stblit_GetIndex(SrcColorType, &SrcIndex,Src_p->Data.Bitmap_p->ColorSpaceConversion);
    }
    /* Test if color conversion can be done in one blitter pass */
    stblit_GetIndex(DstColorType, &DstIndex,Dst_p->Bitmap_p->ColorSpaceConversion);


    if (SrcMB == TRUE)
    {
        Code = stblit_TableMB[SrcIndex][DstIndex];

        /* Set Src MB
         * Src1 pipe : luma (Src_p Data1_p)
         * Src2 pipe : chroma (Src_p Data2_p)*/
        Src1_p = Src2_p;

       /* Overlap has to be disabled : Only in raster mode
        * ALU set to bypass Src2 */
    }
    else
    {

        /* Default values for Src1 : => Dst */
        Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
        Src1.Data.Bitmap_p  = Dst_p->Bitmap_p;
        Src1.Rectangle      = DstAutoClipRectangle;
        Src1.Palette_p      = Dst_p->Palette_p;

        if (BlitContext_p->EnableMaskBitmap == TRUE)
        {
            /* If Err = ST_NO_ERROR, CodeMask is always supported. No need to check the last bit */
            if (stblit_GetTmpIndex(SrcIndex,&TmpIndex,&TmpColorType, &TmpNbBitPerPixel) != ST_NO_ERROR)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            CodeMask = stblit_TableSingleSrc1[SrcIndex][TmpIndex];
            /* Code Mask does not have any CLUT operation enabled because TableSingleSrc1 does not set anything on Src2 h/w flow.
             * => CodeMask 2nd bit is always 0 (Clut disable).
             * However in case of Alpha4 mask, a color expansion has to be done in Src2 h/w flow in order to make it Alpha8, i.e a
             * CLUT expansion operation has to be enabled.
             * => We just set Clut expansion code in CodeMask  */
            if (BlitContext_p->MaskBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA4)
            {
                CodeMask |= 0x2;
            }
            SrcIndex = TmpIndex;
        }

        if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)
        {
            /* Blend mode */
            Code = stblit_TableBlend[SrcIndex][DstIndex];
        }
        else if (((BlitContext_p->AluMode == STBLIT_ALU_COPY) || (BlitContext_p->AluMode == STBLIT_ALU_COPY_INVERT)) &&
                 (BlitContext_p->EnableMaskBitmap == FALSE) &&
                 (BlitContext_p->EnableMaskWord == FALSE)&&
              (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
        {
            /* Single Src2 */
            Code   = stblit_TableSingleSrc2[SrcIndex][DstIndex];
            Src1_p = NULL;
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
    }

    if (((Code >> 15) == 1) ||
        ((((Code >> 14) & 0x1) == 1) && (BlitContext_p->EnableColorCorrection == FALSE)))
    {
        /* Conversion implies multipass or not supported by hardware */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Initialize CommonField values */
    memset(&FirstNodeCommonField, 0, sizeof(stblit_CommonField_t));
    memset(&LastNodeCommonField, 0, sizeof(stblit_CommonField_t));

    if (BlitContext_p->EnableMaskBitmap == FALSE)    /* If MB always in this case */
    {
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if ((IsVResizeOrFF == TRUE)  &&
            (Src1_p != NULL))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
        Err = stblit_OnePassBlitOperation(Device_p, Src1_p, Src2_p, Dest_p, BlitContext_p, Code,
                                      STBLIT_NO_MASK, &NodeFirstHandle, SrcMB,&OperationCfg, &FirstNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }
#ifdef WA_FILL_OPERATION
        if (EnableIT == TRUE)
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif
#ifdef WA_SOFT_RESET_EACH_NODE
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif
#ifdef  WA_GNBvd15279
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)) &&
             (SrcMB == TRUE))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        NodeLastHandle  = NodeFirstHandle;
        NumberNodes     = 1;

        /* Update CommonField in node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&FirstNodeCommonField);
    }
    else   /* 2 passes for mask */
    {
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1   /* Concern Second node */
        if (IsVResizeOrFF == TRUE)       /* => Src1 is always non NULL in Mask mode */
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
        /* Set MaskPalette : Palette used in case of Alpha4 to Alpha8 conversion */
        MaskPalette.ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
        MaskPalette.Data_p      = (void*) Device_p->Alpha4To8Palette_p;
/*        MaskPalette.PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;*/
/*        MaskPalette.ColorDepth  = ;    */

        /* Set Mask Source */
        Mask.Type          = STBLIT_SOURCE_TYPE_BITMAP;
        Mask.Data.Bitmap_p = BlitContext_p->MaskBitmap_p;
        Mask.Rectangle     = MaskAutoClipRectangle;
        Mask.Palette_p     = &MaskPalette;

        /* Set Work Bitmap fields : TBD (Pitch, Alignment)!!!!*/
        WorkBitmap.ColorType            = TmpColorType;
        WorkBitmap.BitmapType           = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        WorkBitmap.PreMultipliedColor   = FALSE ;                                 /* TBD (Src2 params?)*/
        WorkBitmap.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;                    /* TBD (Src2 params?)*/
        WorkBitmap.AspectRatio          = STGXOBJ_ASPECT_RATIO_SQUARE;            /* TBD (Src2 params?)*/
        /* Use Mask dimensions */
        WorkBitmap.Width                = MaskAutoClipRectangle.Width;
        WorkBitmap.Height               = MaskAutoClipRectangle.Height;
        TmpBitWidth                     = WorkBitmap.Width * TmpNbBitPerPixel;
        if ((TmpBitWidth % 8) != 0)
        {
            WorkBitmap.Pitch            = (TmpBitWidth / 8) + 1;
        }
        else
        {
            WorkBitmap.Pitch            = TmpBitWidth / 8;
        }
        WorkBitmap.Data1_p              = BlitContext_p->WorkBuffer_p;
        WorkBitmap.Offset               = 0;

        /* Assumption :
        * The Mask Size must be = foreground Src2 Size.
        * The destination of the first pass has got the same size as them.
        * The driver uses the Mask Rectangle because in some case Src2 is a color
        * and has no rectangle !*/
        DstTmp.Bitmap_p             = &WorkBitmap;
        DstTmp.Rectangle.Width      = MaskAutoClipRectangle.Width;
        DstTmp.Rectangle.Height     = MaskAutoClipRectangle.Height;
        DstTmp.Rectangle.PositionX  = 0;
        DstTmp.Rectangle.PositionY  = 0;
        DstTmp.Palette_p            = MaskSrc2_p->Palette_p;

        Err = stblit_OnePassBlitOperation(Device_p, MaskSrc2_p, &Mask, &DstTmp, BlitContext_p, CodeMask,
                                      STBLIT_MASK_FIRST_PASS, &NodeFirstHandle, FALSE,&OperationCfg, &FirstNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }

        SrcTmp.Type          = STBLIT_SOURCE_TYPE_BITMAP;
        SrcTmp.Data.Bitmap_p = DstTmp.Bitmap_p;
        SrcTmp.Rectangle     = DstTmp.Rectangle;
        SrcTmp.Palette_p     = DstTmp.Palette_p;

        Err = stblit_OnePassBlitOperation(Device_p, MaskSrc1_p, &SrcTmp, Dest_p, BlitContext_p, Code,
                                      STBLIT_MASK_SECOND_PASS, &NodeLastHandle, FALSE,&OperationCfg, &LastNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Release First node which has been sucessfully created previously */
            STOS_SemaphoreWait(Device_p->AccessControl);
            stblit_ReleaseNode(Device_p,NodeFirstHandle);
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }
        NumberNodes = 2;

        /* Link Two nodes together */
        stblit_Connect2Nodes(Device_p, (stblit_Node_t*)NodeFirstHandle,(stblit_Node_t*)NodeLastHandle);

        /* Remove Blit completion interruption for the first node :
        * In this case both nodes are considered as atomic. Never interruption between
         * HAS TO BE DONE BEFORE WORKAROUNDS IT !!!
         * At this stage, the only possible IT is for blit notification
         * Note that there is no lock IT, unlock IT, submission IT, workaround IT nor job notification IT  , yet at this stage !!*/
        ((stblit_Node_t*)NodeFirstHandle)->ITOpcode &= (U32)~STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
        FirstNodeCommonField.INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED) << STBLIT_INS_ENABLE_IRQ_SHIFT)));

#ifdef WA_FILL_OPERATION   /* Concern first node IT */
        if (EnableIT == TRUE)
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT (only for first pass mask)*/
            FirstNodeCommonField.INS  |=  ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif
#ifdef WA_SOFT_RESET_EACH_NODE
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT on first node */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);

            ((stblit_Node_t*)NodeLastHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT on last node */
            LastNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif
        /* Update CommonField in node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&FirstNodeCommonField);
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeLastHandle ,&LastNodeCommonField);
    }


#ifdef STBLIT_TEST_FRONTEND
    /* Simulator dump */
    /* NodeDumpFile((U32*)NodeFirstHandle , sizeof(stblit_Node_t), "DumpFirstNode");
    NodeDumpFile((U32*)NodeLastHandle , sizeof(stblit_Node_t), "DumpLastNode");  */
    STBLIT_FirstNodeAddress = (U32)NodeFirstHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end.*/
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeFirstHandle, (void*)NodeLastHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeFirstHandle,NodeLastHandle, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeFirstHandle, NodeLastHandle, NumberNodes, &OperationCfg);
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
    U8                      Src2Index, DstIndex, TmpIndex;
    U16                     Code     = 0;
    U16                     CodeMask = 0;
    STBLIT_Source_t*        MaskSrc1_p      = Src1_p;
    STBLIT_Source_t*        MaskSrc2_p      = Src2_p;
    STBLIT_Source_t         Mask;
    STBLIT_Source_t         SrcTmp;
    STBLIT_Destination_t    DstTmp;
    STGXOBJ_Palette_t       MaskPalette;
    U8                      NumberNodes;
    stblit_NodeHandle_t     NodeFirstHandle, NodeLastHandle;
    STGXOBJ_Bitmap_t        WorkBitmap;
    STGXOBJ_ColorType_t     TmpColorType;
    U8                      TmpNbBitPerPixel;
    U32                     TmpBitWidth;
    stblit_OperationConfiguration_t     OperationCfg;
    stblit_CommonField_t                FirstNodeCommonField;
    stblit_CommonField_t                LastNodeCommonField;
#ifdef WA_FILL_OPERATION
    BOOL                    EnableIT = FALSE;
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
    BOOL                    IsVResizeOrFF = FALSE;
#endif
    /* Check Dst color type */
    if (Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB)
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
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(Dst_p->Bitmap_p->ColorType, Dst_p->Rectangle.Width);
    if (Err != ST_NO_ERROR)
    {
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
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
        Err = stblit_WA_CheckWidthByteLimitation(Src1_p->Data.Bitmap_p->ColorType, Src1_p->Rectangle.Width);
        if (Err != ST_NO_ERROR)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
        Src1ColorType = Src1_p->Data.Bitmap_p->ColorType;
        Src2ColorType = Src2_p->Data.Color_p->Type;
        stblit_GetIndex(Src2ColorType,&Src2Index, STGXOBJ_ITU_R_BT601); /* 601 valid only in ARGB Colors! TBDiscussed */

#ifdef WA_GNBvd09730
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))  &&
        ((Src2ColorType == STGXOBJ_COLOR_TYPE_CLUT1)  ||
         (Src2ColorType == STGXOBJ_COLOR_TYPE_CLUT2)  ||
         (Src2ColorType == STGXOBJ_COLOR_TYPE_CLUT4)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif

#ifdef WA_FILL_OPERATION
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710)  ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
        {
            /* Interruption in case of color fill*/
            EnableIT = TRUE;
        }
#endif
#ifdef WA_GNBvd06658
        /* This WA may be useless since it is meaningless to have flicker in Fill color operation !*/
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            (BlitContext_p->EnableFlickerFilter == TRUE) &&
            (BlitContext_p->EnableClipRectangle == TRUE)&&
            (Dst_p->Rectangle.Width >= 128 ))      /* Src is the same size as Dst in case of fill color */
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        /* This WA may be useless since it is meaningless to have flicker in Fill color operation !*/
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            (BlitContext_p->EnableFlickerFilter == TRUE) &&
            (Dst_p->Rectangle.Width >= 128 ))        /* Src is the same size as Dst in case of fill color */
        {
            IsVResizeOrFF = TRUE;
        }
#endif
    }
    else   /* Src2 and Src1 are bitmap */
    {
#ifdef WA_HORIZONTAL_POSITION_ODD
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            ((Src2_p->Rectangle.PositionX % 2) == 0) &&
            ((Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    ||
             (Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)  ||
             (Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420)    ||
             (Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420)) &&
            ((Src2_p->Rectangle.Height != Dst_p->Rectangle.Height) ||
             (BlitContext_p->EnableFlickerFilter == TRUE)) &&
            (Src2_p->Rectangle.Width >= 128 ))
        {
            (Src2_p->Rectangle.PositionX)++;
        }
#endif
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
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
        Err = stblit_WA_CheckWidthByteLimitation(Src1_p->Data.Bitmap_p->ColorType, Src1_p->Rectangle.Width);
        if (Err != ST_NO_ERROR)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        Err = stblit_WA_CheckWidthByteLimitation(Src2_p->Data.Bitmap_p->ColorType, Src2_p->Rectangle.Width);
        if (Err != ST_NO_ERROR)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif

        Src2ColorType = Src2_p->Data.Bitmap_p->ColorType;
        Src1ColorType = Src1_p->Data.Bitmap_p->ColorType;

        if ((Src1_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB)  ||
            (Src2_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB))
        {
            /* If one of the source is MB, it means that it uses both hardware channels => Multipass if 2 blit inputs */
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }

#ifdef WA_GNBvd06658
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            ((Src2_p->Rectangle.Height != Dst_p->Rectangle.Height) ||
             (BlitContext_p->EnableFlickerFilter == TRUE)) &&
            (BlitContext_p->EnableClipRectangle == TRUE)&&
            (Src2_p->Rectangle.Width >= 128 ))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif

#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
          ((Src2_p->Rectangle.Height != Dst_p->Rectangle.Height) ||
           (BlitContext_p->EnableFlickerFilter == TRUE)) &&
          (Src2_p->Rectangle.Width >= 128 ))
        {
            IsVResizeOrFF = TRUE;
        }
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            ((Src2_p->Rectangle.Height != Dst_p->Rectangle.Height) ||
             (BlitContext_p->EnableFlickerFilter == TRUE)) &&
            (Src2_p->Rectangle.Width != Dst_p->Rectangle.Width) &&
            (Src2_p->Rectangle.Width >= 128 ) &&
            ((Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422) ||
             (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
#ifdef WA_GNBvd13838
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
        ((Src2_p->Rectangle.Height != Dst_p->Rectangle.Height) ||
         (BlitContext_p->EnableFlickerFilter == TRUE)) &&
        (Src2_p->Rectangle.Width != Dst_p->Rectangle.Width) &&
        (Src2_p->Rectangle.Width >= 128 ) &&
        (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT1))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }

#endif

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

    if (BlitContext_p->EnableMaskBitmap == TRUE)
    {
        /* If Err = ST_NO_ERROR, CodeMask is always supported. No need to check the last bit */
        if (stblit_GetTmpIndex(Src2Index,&TmpIndex,&TmpColorType, &TmpNbBitPerPixel) != ST_NO_ERROR)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        CodeMask = stblit_TableSingleSrc1[Src2Index][TmpIndex]; /* Src2 foreground placed on Src1 HW pipe */
        /* Code Mask does not have any CLUT operation enabled because TableSingleSrc1 does not set anything on Src2 h/w flow.
         * => CodeMask 2nd bit is always 0 (Clut disable).
         * However in case of Alpha4 mask, a color expansion has to be done in Src2 h/w flow in order to make it Alpha8, i.e a
         * CLUT expansion operation has to be enabled.
         * => We just set Clut expansion code in CodeMask  */
        if (BlitContext_p->MaskBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA4)
        {
            CodeMask |= 0x2;
        }
        Src2Index = TmpIndex;
    }

    if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)
    {
        /* Blend mode */
         Code = stblit_TableBlend[Src2Index][DstIndex];
    }
    else if (((BlitContext_p->AluMode == STBLIT_ALU_COPY)  ||
              (BlitContext_p->AluMode == STBLIT_ALU_COPY_INVERT)) &&
              (BlitContext_p->EnableMaskBitmap == FALSE) &&
              (BlitContext_p->EnableMaskWord == FALSE) &&
              (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
    {
        /* Single Src2 */
        Code    = stblit_TableSingleSrc2[Src2Index][DstIndex];
        Src1_p  = NULL;
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_NOOP)  ||
             (BlitContext_p->AluMode == STBLIT_ALU_INVERT))
    {
        /* Single Src1 */
        Code    = stblit_TableSingleSrc1[DstIndex][DstIndex];  /* Src1 Color format == Dst Color Format
                                                         * because same color format constraint for the moment! */
        Src2_p  = NULL;
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
    if (((Code >> 15) == 1) ||
        ((((Code >> 14) & 0x1) == 1) && (BlitContext_p->EnableColorCorrection == FALSE)))
    {
        /* Conversion implies multipass or not supported by hardware */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Initialize CommonField values */
    memset(&FirstNodeCommonField, 0, sizeof(stblit_CommonField_t));
    memset(&LastNodeCommonField, 0, sizeof(stblit_CommonField_t));


    if (BlitContext_p->EnableMaskBitmap == FALSE)
    {
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if ((IsVResizeOrFF == TRUE)  &&
            (Src1_p != NULL))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
        Err = stblit_OnePassBlitOperation(Device_p,Src1_p, Src2_p, Dst_p, BlitContext_p, Code,
                                      STBLIT_NO_MASK, &NodeFirstHandle, FALSE,&OperationCfg,&FirstNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }
#ifdef WA_FILL_OPERATION
        if (EnableIT == TRUE)
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif
#ifdef WA_SOFT_RESET_EACH_NODE
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif
        NodeLastHandle  = NodeFirstHandle;
        NumberNodes     = 1;

        /* Update CommonField in node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&FirstNodeCommonField);
    }
    else
    {
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1  /* Concern Second node */
        if (IsVResizeOrFF == TRUE)     /* => Src1 is always non NULL in Mask mode */
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif

        /* Set MaskPalette : Palette used in case of Alpha4 to Alpha8 conversion */
        MaskPalette.ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
        MaskPalette.Data_p      = (void*) Device_p->Alpha4To8Palette_p;
/*        MaskPalette.PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;*/
/*        MaskPalette.ColorDepth  = ;    */

        /* Set Mask source */
        Mask.Type          = STBLIT_SOURCE_TYPE_BITMAP;
        Mask.Data.Bitmap_p = BlitContext_p->MaskBitmap_p;
        Mask.Rectangle     = BlitContext_p->MaskRectangle;
        Mask.Palette_p     = &MaskPalette;

        /* Set Work Bitmap fields : TBD (Pitch, Alignment)!!!!*/
        WorkBitmap.ColorType            = TmpColorType;
        WorkBitmap.BitmapType           = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        WorkBitmap.PreMultipliedColor   = FALSE ;                                 /* TBD (Src2 params?)*/
        WorkBitmap.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;                    /* TBD (Src2 params?)*/
        WorkBitmap.AspectRatio          = STGXOBJ_ASPECT_RATIO_SQUARE;            /* TBD (Src2 params?)*/
        /* Use Mask dimensions */
        WorkBitmap.Width                = BlitContext_p->MaskRectangle.Width;
        WorkBitmap.Height               = BlitContext_p->MaskRectangle.Height;
        TmpBitWidth                     = WorkBitmap.Width * TmpNbBitPerPixel;
        if ((TmpBitWidth % 8) != 0)
        {
            WorkBitmap.Pitch            = (TmpBitWidth / 8) + 1;
        }
        else
        {
            WorkBitmap.Pitch            = TmpBitWidth / 8;
        }
        WorkBitmap.Data1_p               = BlitContext_p->WorkBuffer_p;
        WorkBitmap.Offset               = 0;

        /* Assumption :
        * The Mask Size must be = foreground Src2 Size.
        * The destination of the first pass has got the same size as them.
        * The driver uses the Mask Rectangle because in some case Src2 is a color
        * and has no rectangle !*/
        DstTmp.Bitmap_p             = &WorkBitmap;
        DstTmp.Rectangle.Width      = BlitContext_p->MaskRectangle.Width;
        DstTmp.Rectangle.Height     = BlitContext_p->MaskRectangle.Height;
        DstTmp.Rectangle.PositionX  = 0;
        DstTmp.Rectangle.PositionY  = 0;
        DstTmp.Palette_p            = MaskSrc2_p->Palette_p;

        Err = stblit_OnePassBlitOperation(Device_p, MaskSrc2_p, &Mask, &DstTmp, BlitContext_p, CodeMask,
                                      STBLIT_MASK_FIRST_PASS, &NodeFirstHandle, FALSE,&OperationCfg,&FirstNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }

        SrcTmp.Type          = STBLIT_SOURCE_TYPE_BITMAP;
        SrcTmp.Data.Bitmap_p = DstTmp.Bitmap_p;
        SrcTmp.Rectangle     = DstTmp.Rectangle;
        SrcTmp.Palette_p     = DstTmp.Palette_p;

        Err = stblit_OnePassBlitOperation(Device_p, MaskSrc1_p, &SrcTmp, Dst_p, BlitContext_p, Code,
                                      STBLIT_MASK_SECOND_PASS, &NodeLastHandle, FALSE,&OperationCfg,&LastNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Release First node which has been sucessfully created previously */
            STOS_SemaphoreWait(Device_p->AccessControl);
            stblit_ReleaseNode(Device_p, NodeFirstHandle);
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }
        NumberNodes = 2;

        /* Link Two nodes together */
        stblit_Connect2Nodes(Device_p, (stblit_Node_t*)NodeFirstHandle,(stblit_Node_t*)NodeLastHandle);

        /* Remove Blit completion interruption for the first node :
         * In this case both nodes are considered as atomic. Never interruption between
         * HAS TO BE DONE BEFORE WORKAROUNDS IT !!!
         * At this stage, the only possible IT is for blit notification
         * Note that there is no lock IT, unlock IT, submission IT, workaround IT nor job notification IT  , yet at this stage !! */
        ((stblit_Node_t*)NodeFirstHandle)->ITOpcode &= (U32)~STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
        FirstNodeCommonField.INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED) << STBLIT_INS_ENABLE_IRQ_SHIFT)));

#ifdef WA_FILL_OPERATION  /* Concern First node IT */
        if (EnableIT == TRUE)
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT (only for first pass mask)*/
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif
#ifdef WA_SOFT_RESET_EACH_NODE
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT on first node */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);

            ((stblit_Node_t*)NodeLastHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT on last node */
            LastNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif
        /* Update CommonField in node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&FirstNodeCommonField);
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeLastHandle ,&LastNodeCommonField);
    }

#ifdef STBLIT_TEST_FRONTEND
    /* Simulator dump */
    /* NodeDumpFile((U32*)NodeFirstHandle , sizeof(stblit_Node_t), "DumpFirstNode");
    NodeDumpFile((U32*)NodeLastHandle , sizeof(stblit_Node_t), "DumpLastNode"); */
    STBLIT_FirstNodeAddress = (U32)NodeFirstHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeFirstHandle, (void*)NodeLastHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeFirstHandle,NodeLastHandle, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeFirstHandle, NodeLastHandle, NumberNodes, &OperationCfg);
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

    #ifdef STBLIT_BENCHMARK4
    t1 = STOS_time_now();
    #endif

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
    if ((BlitContext_p == NULL)                ||
        ((Src1_p == NULL) && (Src2_p == NULL)) ||
        (Dst_p == NULL))
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
    /* Check Mask Size if any (Mask Size = Foreground src)          TBDone */

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

    #ifdef STBLIT_BENCHMARK4
    t2 = STOS_time_now();
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time for Blit = %d",t2 - t1));
    #endif

    #ifdef STBLIT_BENCHMARK3
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to init node = %d",t2 - t1));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set interrupt = %d",t4 - t3));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Src1 = %d",t6 - t5));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Src2 = %d",t8 - t7));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set target = %d",t10 - t9));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set clipping = %d",t12 - t11));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Color Key = %d",t14 - t13));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Plane mask = %d",t16 - t15));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Trigger = %d",t18 - t17));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Resize = %d",t20 - t19));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set ALU Blend = %d",t22 - t21));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set ALU Logic = %d",t24 - t23));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Flicker = %d",t26 - t25));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Input node converter = %d",t28 - t27));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Output node converter = %d",t30 - t29));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Color Correction = %d",t32 - t31));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Color Expansion = %d",t34 - t33));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to set Plane Reduction = %d",t36 - t35));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "sizeof node = %d",sizeof(stblit_Node_t)));
    #endif


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
    stblit_NodeHandle_t             NodeHandle;
    stblit_OperationConfiguration_t OperationCfg;
    stblit_CommonField_t            CommonField;
    STGXOBJ_Rectangle_t             AutoClipRectangle;
    STGXOBJ_Rectangle_t             MaskAutoClipRectangle;
    U32                             XOffset,YOffset;

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
        (Color_p == NULL)       ||
        (Rectangle_p == NULL))
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

    /* Update Mask Rectangle according to AutoClip rectangle */
    if (BlitContext_p->EnableMaskBitmap == TRUE)
    {
        MaskAutoClipRectangle             = BlitContext_p->MaskRectangle;
        MaskAutoClipRectangle.PositionX  += XOffset;
        MaskAutoClipRectangle.PositionY  += YOffset;
        MaskAutoClipRectangle.Width       = AutoClipRectangle.Width;
        MaskAutoClipRectangle.Height      = AutoClipRectangle.Height;
    }

    /* Perform operation directly in case of softaware device */
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE) &&
        (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE) &&
        (BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
        (BlitContext_p->EnableMaskWord == FALSE) &&
        (BlitContext_p->EnableColorCorrection == FALSE) &&
        (BlitContext_p->EnableFlickerFilter == FALSE) &&
        (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
    {
        /* Attempt to fill the rectangle with an optimised software renderer.*/
        Err = stblit_sw_fillrect(Device_p, Bitmap_p, &AutoClipRectangle,&MaskAutoClipRectangle, Color_p, BlitContext_p);
        if (Err == ST_NO_ERROR)
            return (Err);
    }
    /* Check Dst rectangle max dimension  */
    if ((AutoClipRectangle.Width > STBLIT_MAX_WIDTH)  || /* 12 bit gamma hw constraint */
        (AutoClipRectangle.Height > STBLIT_MAX_HEIGHT))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#ifdef WA_GNBvd09730
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))  &&
        ((Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT1)  ||
         (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT2)  ||
         (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT4)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(Bitmap_p->ColorType, AutoClipRectangle.Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
    /* Initialize CommonField value */
    memset(&CommonField, 0, sizeof(stblit_CommonField_t));
    /* Set Fill operation : All none supported features in the blit context are ignored */
    Err = stblit_OnePassFillOperation(Device_p, Bitmap_p, &AutoClipRectangle, &MaskAutoClipRectangle,Color_p, BlitContext_p,&NodeHandle, &OperationCfg,&CommonField);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }
#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
    {
        ((stblit_Node_t*)NodeHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
        /* Set Blit completion IT */
        CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
    }
#endif
    /* Update CommonField in node*/
    stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeHandle ,&CommonField);

#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeHandle;
#endif
    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeHandle, (void*)NodeHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeHandle,NodeHandle, 1);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeHandle, NodeHandle,1, &OperationCfg);
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
    stblit_NodeHandle_t     NodeHandle;
    stblit_OperationConfiguration_t   OperationCfg;
    stblit_CommonField_t    CommonField;
    STGXOBJ_Rectangle_t     AutoClipRectangle;

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
    if ((BlitContext_p == NULL)  ||
        (SrcBitmap_p == NULL)    ||
        (SrcRectangle_p == NULL) ||
        (DstBitmap_p == NULL))
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

    /* Check Src rectangle size */
    if ((SrcRectangle_p->Width == 0)    ||
        (SrcRectangle_p->Height == 0))
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

    /* Perform operation directly in case of softaware device */
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE) &&
        (BlitContext_p->EnableMaskBitmap == FALSE) &&
        (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE) &&
        (BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
        (BlitContext_p->EnableMaskWord == FALSE) &&
        (BlitContext_p->EnableColorCorrection == FALSE) &&
        (BlitContext_p->EnableFlickerFilter == FALSE) &&
        (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
    {
        /* Attempt to fill the array with an optimised software renderer.*/
        Err = stblit_sw_copyrect(Device_p, SrcBitmap_p, &AutoClipRectangle, DstBitmap_p, DstPositionX, DstPositionY, BlitContext_p);
        if (Err == ST_NO_ERROR)
        {
            return(Err);
        }
    }

    /* Check Src rectangle max dimensions (gamma/emulator contraints only )  */
    if ((SrcRectangle_p->Width > STBLIT_MAX_WIDTH)  || /* 12 bit gamma hw constraint */
        (SrcRectangle_p->Height > STBLIT_MAX_HEIGHT)) /* 12 bit gamma HW constraints */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(SrcBitmap_p->ColorType, AutoClipRectangle.Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
    /* Initialize CommonField value */
    memset(&CommonField, 0, sizeof(stblit_CommonField_t));

    /* Set Copy operation : All none supported features in the blit context are ignored (Mask, color key, filter, ALU Mode) */
    Err = stblit_OnePassCopyOperation(Device_p, SrcBitmap_p, &AutoClipRectangle, DstBitmap_p, DstPositionX, DstPositionY,
                                      BlitContext_p,&NodeHandle, &OperationCfg, &CommonField);

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

    /* Update CommonField in node*/
    stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeHandle ,&CommonField);


#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeHandle, (void*)NodeHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeHandle,NodeHandle, 1);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeHandle, NodeHandle, 1, &OperationCfg);
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
    stblit_NodeHandle_t     NodeHandle;
    stblit_OperationConfiguration_t   OperationCfg;
    stblit_CommonField_t    CommonField;

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
#ifdef WA_GNBvd09730
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))  &&
        ((Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT1)  ||
         (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT2)  ||
         (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT4)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(Bitmap_p->ColorType, Length);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif

    /* Initialize CommonField value */
    memset(&CommonField, 0, sizeof(stblit_CommonField_t));

    /* Set Draw operation : All none supported features in the blit context are ignored */
    Err = stblit_OnePassDrawLineOperation(Device_p, Bitmap_p, PositionX, PositionY, Length, TRUE, Color_p,
                                          BlitContext_p, &NodeHandle, &OperationCfg,&CommonField);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }
#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
    {
        ((stblit_Node_t*)NodeHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
        /* Set Blit completion IT */
        CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
    }
#endif

    /* Update CommonField in node*/
    stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeHandle ,&CommonField);

#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeHandle, (void*)NodeHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeHandle,NodeHandle, 1);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeHandle, NodeHandle,1, &OperationCfg);
    }

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
    stblit_NodeHandle_t     NodeHandle;
    stblit_OperationConfiguration_t   OperationCfg;
    stblit_CommonField_t    CommonField;

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

#ifdef WA_GNBvd09730
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))  &&
        ((Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT1)  ||
         (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT2)  ||
         (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT4)))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif

    /* Initialize CommonField value */
    memset(&CommonField, 0, sizeof(stblit_CommonField_t));

    /* Set Draw operation : All none supported features in the blit context are ignored */
    Err = stblit_OnePassDrawLineOperation(Device_p, Bitmap_p, PositionX, PositionY, Length, FALSE, Color_p,
                                          BlitContext_p, &NodeHandle, &OperationCfg, &CommonField);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }
#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
    {
        ((stblit_Node_t*)NodeHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
        /* Set Blit completion IT */
        CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
    }
#endif

    /* Update CommonField in node*/
    stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeHandle ,&CommonField);

#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeHandle, (void*)NodeHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeHandle,NodeHandle, 1);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeHandle, NodeHandle, 1, &OperationCfg);
    }

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
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444)
    {
        ColorAddress_p = (U8*) ((U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset  + (PositionY * Bitmap_p->Pitch) + (PositionX * 3));

        Color_p->Value.UnsignedYCbCr888_444.Cb = *(U8*)(ColorAddress_p);
        Color_p->Value.UnsignedYCbCr888_444.Y  = *(U8*)((U32)ColorAddress_p + 1);
        Color_p->Value.UnsignedYCbCr888_444.Cr = *(U8*)((U32)ColorAddress_p + 2);
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
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*        Device_p;
    stblit_Job_t*           Job_p;
    U8                      NumberNodes;
    stblit_NodeHandle_t     NodeFirstHandle, NodeLastHandle;
    stblit_OperationConfiguration_t   OperationCfg;
    stblit_DataPoolDesc_t*  CurrentNodeDataPool_p;
    stblit_Node_t*          PreviousNode_p;
    U32                     i;
/*    U8                      ScanConfig;*/
    U32                     XOffset, YOffset;
    U32                     TXY, S1XY;
    stblit_CommonField_t    CommonField;
    STBLIT_XY_t*            XYArrayFromCpu_p;

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
        (XYArray_p == NULL)     ||
        (Color_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Get the address seen from Cpu of XYLArray */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    XYArrayFromCpu_p = (STBLIT_XY_t*)stblit_DeviceToCpu((void*)(XYArray_p));
#else
    XYArrayFromCpu_p = (STBLIT_XY_t*)STAVMEM_VirtualToCPU((void*)(XYArray_p),&(Device_p->VirtualMapping));
#endif

    /* return if no element */
    if (PixelsNumber == 0)
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

    /* Start protection for pool access*/
    STOS_SemaphoreWait(Device_p->AccessControl);

    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)
    {
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            /* Check there is enough nodes in pool to set the full array operations */
            if (PixelsNumber > Device_p->SBlitNodeDataPool.NbFreeElem)
            {
                /* Stop protection */
                STOS_SemaphoreSignal(Device_p->AccessControl);

                return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);  /* is it the correct error to return ? */
            }
        }
        CurrentNodeDataPool_p = &(Device_p->SBlitNodeDataPool);
    }
    else  /* Job operation */
    {
        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }

        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            /* Check there is enough nodes in pool to set the full array operations */
            if (PixelsNumber > Device_p->JBlitNodeDataPool.NbFreeElem)
            {
                /* Stop protection */
                STOS_SemaphoreSignal(Device_p->AccessControl);

                return(STBLIT_ERROR_MAX_JOB_NODE);  /* is it the correct error to return ? */
            }
        }
        CurrentNodeDataPool_p = &(Device_p->JBlitNodeDataPool);
    }

    /* Check that no color conversion */

    /* Check other supported blit context parameters */

    /* Initialize CommonField value */
    memset(&CommonField, 0, sizeof(stblit_CommonField_t));

    /* Set operation : All none supported features in the blit context are ignored. */
    if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
    {
    /* This implementation consists of an emulation of the XYL engine since it is not present on 7015 device but on 7020/ST40GX1
     * One way to emulate it, is to have a blit node per pixel to set. This solution implies really bad performances since number
     * of nodes could be high but it allows the user to take advantage of HW blitter features.
     * This solution is the one implemented !
     * Note that with a full software emulation (no use of blitter), the blit context could not be as rich. In fact it could
     * be but with a significant sw effort...TBD ..... */

        /* At this moment no Mask support (first implementation)-> ScanConfig =0 -> XOffset =Y Offset = 0 */

        /* First Node setting according to context analysis and stuff... */
        Err = stblit_OnePassDrawHLineOperation(Device_p, Bitmap_p,XYArrayFromCpu_p->PositionX, XYArrayFromCpu_p->PositionY,1,Color_p, BlitContext_p,
                                         &NodeFirstHandle, &OperationCfg,&CommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }

#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /* Extract the Scan Direction for destination because of overlap Mask/Dst: This direction is the same for Src1 (if enabled) */
/*        ScanConfig =(U8)((Node_p->HWNode.BLT_TTY  >> STBLIT_TTY_SCAN_ORDER_SHIFT) & STBLIT_TTY_SCAN_ORDER_MASK);*/
          XOffset = 0;
          YOffset = 0;
/*        if (((ScanConfig & 0x2) >> 1) == 0)*/
/*        {*/
/*            YOffset = 0;*/
/*        }*/
/*        else*/
/*        {*/
/*            YOffset = Rectangle_p->Height - 1;*/
/*        }*/
/*        if ((ScanConfig & 0x1) == 0)*/
/*        {*/
/*            XOffset = 0;*/
/*        }*/
/*        else*/
/*        {*/
/*            XOffset = Rectangle_p->Width - 1;*/
/*        }*/


        /* Update CommonField in Node first handle node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&CommonField);

        /* Other nodes ('til PixelsNumber) have got exactly the same parameters as the first one except their X and Y coordinates
         * A copy of the first node is done and X, Y coordinates are updated : No need of further Context Analysis */

        PreviousNode_p  = (stblit_Node_t*)NodeFirstHandle;

        for(i = 1; i < PixelsNumber; i++)
        {
			U32 TempAddress;

            /* Increment XYArray_p */
            XYArrayFromCpu_p++;

			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&Handle;

            /* Get Node */
            stblit_GetElement(CurrentNodeDataPool_p, (void**) TempAddress);

            /* Copy Node */
            *((stblit_Node_t*)Handle) =  *((stblit_Node_t*)NodeFirstHandle);

            /* Connect to Previous node */
            stblit_Connect2Nodes(Device_p,PreviousNode_p,(stblit_Node_t*)Handle);

            /* Update Dst and Src1 X and Y coordinates.
            * In Case Src1 is not enabled (bypass Src2), Src1 X,Y updates have no effect */
            TXY = ((U32)((XYArrayFromCpu_p->PositionX + XOffset)& STBLIT_TXY_X_MASK) << STBLIT_TXY_X_SHIFT);
            TXY |= ((U32)((XYArrayFromCpu_p->PositionY + YOffset)& STBLIT_TXY_Y_MASK) << STBLIT_TXY_Y_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_TXY)),TXY);

            S1XY = ((U32)((XYArrayFromCpu_p->PositionX + XOffset)& STBLIT_S1XY_X_MASK) << STBLIT_S1XY_X_SHIFT);
            S1XY |= ((U32)((XYArrayFromCpu_p->PositionY + YOffset)& STBLIT_S1XY_Y_MASK) << STBLIT_S1XY_Y_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S1XY)),S1XY);

            /* Update NIP */
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_NIP)),0);

            /* Update Previous */
            PreviousNode_p = (stblit_Node_t*)Handle;

        }

        /* Update interruption for last node if completion needed (PreviousNode is the last node at this stage!)*/
        if (BlitContext_p->NotifyBlitCompletion == TRUE)
        {
            PreviousNode_p->ITOpcode |= STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
            /* Update CommonField  */
            stblit_WriteCommonFieldInNode(PreviousNode_p ,&CommonField);
        }

        NodeLastHandle  = (stblit_NodeHandle_t)PreviousNode_p;
        NumberNodes     = PixelsNumber;
    }
    else  /* GAMMA with XYL device or emulation */
    {
        Err = stblit_OnePassDrawArrayOperation(Device_p, Bitmap_p, STBLIT_ARRAY_TYPE_XY,(void*)XYArray_p,(STGXOBJ_Color_t*)Color_p,
                                               PixelsNumber, BlitContext_p,&NodeFirstHandle, &OperationCfg,&CommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }

#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /* Update CommonField in Node first handle node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&CommonField);


        NodeLastHandle = NodeFirstHandle;
        NumberNodes = 1;
    }

    /* Stop protection */
    STOS_SemaphoreSignal(Device_p->AccessControl);

#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeFirstHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeFirstHandle, (void*)NodeLastHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeFirstHandle,NodeLastHandle, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeFirstHandle, NodeLastHandle, NumberNodes, &OperationCfg);
    }

    return(Err);

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
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*        Device_p;
    stblit_Job_t*           Job_p;
    U8                      NumberNodes;
    stblit_NodeHandle_t     NodeFirstHandle, NodeLastHandle;
    U32                     i;
    U32                     Color;
    stblit_OperationConfiguration_t   OperationCfg;
    stblit_DataPoolDesc_t*  CurrentNodeDataPool_p;
    stblit_Node_t*          PreviousNode_p;
/*    U8                      ScanConfig;*/
    U32                     XOffset, YOffset;
    U32                     TXY, S1XY;
    stblit_CommonField_t    CommonField;
    STBLIT_XYC_t*           XYArrayFromCpu_p;

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
        (XYArray_p == NULL)     ||
        (ColorArray_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Get the address seen from Cpu of XYLArray */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    XYArrayFromCpu_p = (STBLIT_XYC_t*)stblit_DeviceToCpu((void*)(XYArray_p));
#else
    XYArrayFromCpu_p = (STBLIT_XYC_t*)STAVMEM_VirtualToCPU((void*)(XYArray_p),&(Device_p->VirtualMapping));
#endif

    /* Return if no element */
    if (PixelsNumber == 0)
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

    /* Start protection for pool access*/
    STOS_SemaphoreWait(Device_p->AccessControl);

    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)
    {
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            /* Check there is enough nodes in pool to set the full array operations */
            if (PixelsNumber > Device_p->SBlitNodeDataPool.NbFreeElem)
            {
                /* Stop protection */
                STOS_SemaphoreSignal(Device_p->AccessControl);

                return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);  /* is it the correct error to return ? */
            }
        }
        CurrentNodeDataPool_p = &(Device_p->SBlitNodeDataPool);
    }
    else  /* Job operation */
    {
        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }

        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            /* Check there is enough nodes in pool to set the full array operations */
            if (PixelsNumber > Device_p->JBlitNodeDataPool.NbFreeElem)
            {
                /* Stop protection */
                STOS_SemaphoreSignal(Device_p->AccessControl);

                return(STBLIT_ERROR_MAX_JOB_NODE);  /* is it the correct error to return ? */
            }
        }
        CurrentNodeDataPool_p = &(Device_p->JBlitNodeDataPool);
    }

    /* Check that no color conversion */

    /* Check other supported blit context parameters */

    /* Initialize CommonField value */
    memset(&CommonField, 0, sizeof(stblit_CommonField_t));

    /* Set operation : All none supported features in the blit context are ignored. */
    if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
    {
    /* This implementation consists of an emulation of the XYL engine since it is not present on 7015 device but on 7020/ST40GX1
     * One way to emulate it, is to have a blit node per pixel to set. This solution implies really bad performances since number
     * of nodes could be high but it allows the user to take advantage of HW blitter features.
     * This solution is the one implemented !
     * Note that with a full software emulation (no use of blitter), the blit context could not be as rich. In fact it could
     * be but with a significant sw effort...TBD ..... */

        /* At this moment no Mask support (first implementation)-> ScanConfig =0 -> XOffset =Y Offset = 0 */

        /* First Node setting according to context analysis and stuff... */
        Err = stblit_OnePassDrawHLineOperation(Device_p, Bitmap_p,XYArrayFromCpu_p->PositionX, XYArrayFromCpu_p->PositionY,1,ColorArray_p, BlitContext_p,
                                         &NodeFirstHandle, &OperationCfg,&CommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }

#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /*  Extract the Scan Direction for destination because of overlap Mask/Dst: This direction is the same for Src1 (if enabled) */
/*        ScanConfig =(U8)((Node_p->HWNode.BLT_TTY  >> STBLIT_TTY_SCAN_ORDER_SHIFT) & STBLIT_TTY_SCAN_ORDER_MASK);*/
        XOffset = 0;
        YOffset = 0;
/*        if (((ScanConfig & 0x2) >> 1) == 0)*/
/*        {*/
/*            YOffset = 0;*/
/*        }*/
/*        else*/
/*        {*/
/*            YOffset = Rectangle_p->Height - 1;*/
/*        }*/
/*        if ((ScanConfig & 0x1) == 0)*/
/*        {*/
/*            XOffset = 0;*/
/*        }*/
/*        else*/
/*        {*/
/*            XOffset = Rectangle_p->Width - 1;*/
/*        }*/

        /* Update CommonField in Node first handle node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&CommonField);

        /* Other nodes ('til PixelsNumber) have got exactly the same parameters as the first one except their X and Y coordinates
         * A copy of the first node is done and X, Y coordinates are updated : No need of further Context Analysis */
        PreviousNode_p = (stblit_Node_t*)NodeFirstHandle;

        for(i = 1; i < PixelsNumber; i++)
        {
			U32 TempAddress;

			/* Increment XYArray_p*/
            XYArrayFromCpu_p++;

			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&Handle;

			/* Get Node */
            stblit_GetElement(CurrentNodeDataPool_p, (void**) TempAddress);

            /* Copy Node */
            *((stblit_Node_t*)Handle) =  *((stblit_Node_t*)NodeFirstHandle);

            /* Connect to Previous node */
            stblit_Connect2Nodes(Device_p,PreviousNode_p,(stblit_Node_t*)Handle);

            /* Update Dst and Src1 X and Y coordinates.
            * In Case Src1 is not enabled (bypass Src2), Src1 X,Y updates have no effect */
            TXY = ((U32)((XYArrayFromCpu_p->PositionX + XOffset)& STBLIT_TXY_X_MASK) << STBLIT_TXY_X_SHIFT);
            TXY |= ((U32)((XYArrayFromCpu_p->PositionY + YOffset)& STBLIT_TXY_Y_MASK) << STBLIT_TXY_Y_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_TXY)),TXY);

            S1XY = ((U32)((XYArrayFromCpu_p->PositionX + XOffset)& STBLIT_S1XY_X_MASK) << STBLIT_S1XY_X_SHIFT);
            S1XY |= ((U32)((XYArrayFromCpu_p->PositionY + YOffset)& STBLIT_S1XY_Y_MASK) << STBLIT_S1XY_Y_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S1XY)),S1XY);

            /* Update NIP */
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_NIP)),0);

            /* No XYL engine, i.e one color fill per node
             * Update Color Fill in Src2 (its type is unchanged, no need to update it)
             * The color must be set according to input to the internal src formatter */
            stblit_FormatColorForInputFormatter((STGXOBJ_Color_t*)(ColorArray_p + i),&Color) ;
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S2CF)),((U32)(Color & STBLIT_S2CF_COLOR_MASK)
                                                             << STBLIT_S2CF_COLOR_SHIFT));
            /* Update Previous */
            PreviousNode_p = (stblit_Node_t*)Handle;
        }

        /* Update interruption for last node if completion needed (PreviousNode is the last node at this stage!)*/
        if (BlitContext_p->NotifyBlitCompletion == TRUE)
        {
            PreviousNode_p->ITOpcode |= STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
            /* Update CommonField  */
            stblit_WriteCommonFieldInNode(PreviousNode_p ,&CommonField);

        }

        NodeLastHandle  = (stblit_NodeHandle_t)PreviousNode_p;
        NumberNodes     = PixelsNumber;
    }
    else
    {
        /* Analyse color Array and copy colors in Reserved1 fields from XYArray */
        for (i = 0; i < PixelsNumber; i++)
        {
            /* color setting for XYL engine
             * The color must be set according to output of the internal src formatter
             * This is the contrary of normal fill operation!!!! */
            stblit_FormatColorForOutputFormatter((STGXOBJ_Color_t*)(ColorArray_p + i),&Color) ;
            ((STBLIT_XYC_t*)(XYArrayFromCpu_p + i))->Reserved2 = Color;
        }

        Err = stblit_OnePassDrawArrayOperation(Device_p, Bitmap_p, STBLIT_ARRAY_TYPE_XYC,(void*)XYArray_p,(STGXOBJ_Color_t*)NULL,
                                           PixelsNumber, BlitContext_p,&NodeFirstHandle, &OperationCfg,&CommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }
#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /* Update CommonField in Node first handle node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&CommonField);

        NodeLastHandle = NodeFirstHandle;
        NumberNodes = 1;
    }

    /* Stop protection */
    STOS_SemaphoreSignal(Device_p->AccessControl);

#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeFirstHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeFirstHandle, (void*)NodeLastHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeFirstHandle,NodeLastHandle, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeFirstHandle, NodeLastHandle, NumberNodes, &OperationCfg);
    }

    return(Err);

}

/*
--------------------------------------------------------------------------------
Draw XYL API function
--------------------------------------------------------------------------------
*/
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
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE) &&
        (BlitContext_p->EnableMaskBitmap == FALSE) &&
        (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE) &&
        (BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
        (BlitContext_p->EnableMaskWord == FALSE) &&
        (BlitContext_p->EnableColorCorrection == FALSE) &&
        (BlitContext_p->EnableFlickerFilter == FALSE) &&
        (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE))
    {
        /* Attempt to fill the array with an optimised software renderer.*/
        Err = stblit_sw_xyl(Device_p, Bitmap_p, XYLArray_p, SegmentsNumber, Color_p, BlitContext_p);
        if (Err == ST_NO_ERROR)
        {
            return(Err);
        }
    }

    /* Get the address seen from Cpu of XYLArray */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    XYLArrayFromCpu_p = (STBLIT_XYL_t*)stblit_DeviceToCpu((void*)(XYLArray_p));
#else
    XYLArrayFromCpu_p = (STBLIT_XYL_t*)STAVMEM_VirtualToCPU((void*)(XYLArray_p),&(Device_p->VirtualMapping));
#endif

    /* Start protection for pool access control*/
    STOS_SemaphoreWait(Device_p->AccessControl);

    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)
    {
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            /* Check there is enough nodes in pool to set the full array operations */
            if (SegmentsNumber > Device_p->SBlitNodeDataPool.NbFreeElem)
            {
                /* Stop protection */
                STOS_SemaphoreSignal(Device_p->AccessControl);

                return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);  /* is it the correct error to return ? */
            }
        }
        CurrentNodeDataPool_p = &(Device_p->SBlitNodeDataPool);
    }
    else  /* Job operation */
    {
        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }

        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            /* Check there is enough nodes in pool to set the full array operations */
            if (SegmentsNumber > Device_p->JBlitNodeDataPool.NbFreeElem)
            {
                /* Stop protection */
                STOS_SemaphoreSignal(Device_p->AccessControl);

                return(STBLIT_ERROR_MAX_JOB_NODE);  /* is it the correct error to return ? */
            }
        }
        CurrentNodeDataPool_p = &(Device_p->JBlitNodeDataPool);
    }

    /* Check that no color conversion */

    /* Check other supported blit context parameters */

    /* Initialize CommonField value */
    memset(&CommonField, 0, sizeof(stblit_CommonField_t));

    /* Set operation : All none supported features in the blit context are ignored. */
    if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
    {
    /* This implementation consists of an emulation of the XYL engine since it is not present on 7015 device but on 7020/ST40GX1
     * One way to emulate it, is to have a blit node per pixel to set. This solution implies really bad performances since number
     * of nodes could be high but it allows the user to take advantage of HW blitter features.
     * This solution is the one implemented !
     * Note that with a full software emulation (no use of blitter), the blit context could not be as rich. In fact it could
     * be but with a significant sw effort...TBD ..... */

        /* At this moment no Mask support (first implementation)-> ScanConfig =0 -> XOffset =Y Offset = 0 */

        /* First Node setting according to context analysis and stuff... */
        Err = stblit_OnePassDrawHLineOperation(Device_p, Bitmap_p,XYLArrayFromCpu_p->PositionX, XYLArrayFromCpu_p->PositionY,XYLArrayFromCpu_p->Length,Color_p,
                                               BlitContext_p, &NodeFirstHandle, &OperationCfg, &CommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }

#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /*  Extract the Scan Direction for destination because of overlap Mask/Dst: This direction is the same for Src1 (if enabled) */
/*        ScanConfig =(U8)((Node_p->HWNode.BLT_TTY  >> STBLIT_TTY_SCAN_ORDER_SHIFT) & STBLIT_TTY_SCAN_ORDER_MASK);*/
        XOffset = 0;
        YOffset = 0;
/*        if (((ScanConfig & 0x2) >> 1) == 0)*/
/*        {*/
/*            YOffset = 0;*/
/*        }*/
/*        else*/
/*        {*/
/*            YOffset = Rectangle_p->Height - 1;*/
/*        }*/
/*        if ((ScanConfig & 0x1) == 0)*/
/*        {*/
/*            XOffset = 0;*/
/*        }*/
/*        else*/
/*        {*/
/*            XOffset = Rectangle_p->Width - 1;*/
/*        }*/

        /* Update CommonField in Node first handle node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&CommonField);

        /* Other nodes ('til PixelsNumber) have got exactly the same parameters as the first one except their X and Y coordinates
         * A copy of the first node is done and X, Y coordinates are updated : No need of further Context Analysis */
        PreviousNode_p = (stblit_Node_t*)NodeFirstHandle;

        for(i = 1; i < SegmentsNumber; i++)
        {
			U32 TempAddress;

            /* Increment XYLArrayFromCpu_p */
            XYLArrayFromCpu_p++;

			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&Handle;

			/* Get Node */
            stblit_GetElement(CurrentNodeDataPool_p, (void**) TempAddress);

            /* Copy Node */
            *((stblit_Node_t*)Handle) =  *((stblit_Node_t*)NodeFirstHandle);

            /* Connect to Previous node */
            stblit_Connect2Nodes(Device_p,PreviousNode_p,(stblit_Node_t*)Handle);

            /* Update Dst and Src1 X and Y coordinates.
            * In Case Src1 is not enabled (bypass Src2), Src1 X,Y updates have no effect */
            TXY = ((U32)((XYLArrayFromCpu_p->PositionX + XOffset)& STBLIT_TXY_X_MASK) << STBLIT_TXY_X_SHIFT);
            TXY |= ((U32)((XYLArrayFromCpu_p->PositionY + YOffset)& STBLIT_TXY_Y_MASK) << STBLIT_TXY_Y_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_TXY)),TXY);

            S1XY = ((U32)((XYLArrayFromCpu_p->PositionX + XOffset)& STBLIT_S1XY_X_MASK) << STBLIT_S1XY_X_SHIFT);
            S1XY |= ((U32)((XYLArrayFromCpu_p->PositionY + YOffset)& STBLIT_S1XY_Y_MASK) << STBLIT_S1XY_Y_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S1XY)),S1XY);

            /* Update Dst and Src1 Size */
            S1SZ = ((U32)(XYLArrayFromCpu_p->Length & STBLIT_S1SZ_WIDTH_MASK) << STBLIT_S1SZ_WIDTH_SHIFT);
            S1SZ |= ((U32)(1 & STBLIT_S1SZ_HEIGHT_MASK) << STBLIT_S1SZ_HEIGHT_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S1SZ)),S1SZ);

            /* Update Src2 size = Dst/Src1 size (Size can not be ignored : HW constraint) */
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S2SZ)),S1SZ);

            /* Update NIP */
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_NIP)),0);

            /* Update Previous */
            PreviousNode_p = (stblit_Node_t*)Handle;
        }

        /* Update interruption for last node if completion needed (PreviousNode is the last node at this stage!)*/
        if (BlitContext_p->NotifyBlitCompletion == TRUE)
        {
            PreviousNode_p->ITOpcode |= STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
            /* Update CommonField  */
            stblit_WriteCommonFieldInNode(PreviousNode_p ,&CommonField);
        }

        NodeLastHandle  = (stblit_NodeHandle_t)PreviousNode_p;
        NumberNodes     = SegmentsNumber;
    }
    else
    {
        Err = stblit_OnePassDrawArrayOperation(Device_p, Bitmap_p, STBLIT_ARRAY_TYPE_XYL,(void*)XYLArray_p,(STGXOBJ_Color_t*)Color_p,
                                           SegmentsNumber, BlitContext_p,&NodeFirstHandle, &OperationCfg,&CommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }

#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
        if  ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /* Update CommonField in Node first handle node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&CommonField);


        NodeLastHandle = NodeFirstHandle;
        NumberNodes = 1;
    }

    /* Stop protection */
    STOS_SemaphoreSignal(Device_p->AccessControl);

#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeFirstHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeFirstHandle, (void*)NodeLastHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeFirstHandle,NodeLastHandle, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeFirstHandle, NodeLastHandle, NumberNodes, &OperationCfg);
    }

    return(Err);

}

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
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*        Device_p;
    stblit_Job_t*           Job_p;
    U8                      NumberNodes;
    stblit_NodeHandle_t     NodeFirstHandle, NodeLastHandle;
    U32                     i;
    U32                     Color;
    stblit_OperationConfiguration_t   OperationCfg;
    stblit_DataPoolDesc_t*  CurrentNodeDataPool_p;
    stblit_Node_t*          PreviousNode_p;
/*    U8                      ScanConfig;*/
    U32                     XOffset, YOffset;
    U32                     TXY, S1XY, S1SZ;
    stblit_CommonField_t    CommonField ;
    STBLIT_XYL_t*           XYLArrayFromCpu_p;

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
        (XYLArray_p == NULL)     ||
        (ColorArray_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Get the address seen from Cpu of XYLArray */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    XYLArrayFromCpu_p = (STBLIT_XYL_t*)stblit_DeviceToCpu((void*)(XYLArray_p));
#else
    XYLArrayFromCpu_p = (STBLIT_XYL_t*)STAVMEM_VirtualToCPU((void*)(XYLArray_p),&(Device_p->VirtualMapping));
#endif

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

    /* Start protection for loop access control*/
    STOS_SemaphoreWait(Device_p->AccessControl);

    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)
    {
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            /* Check there is enough nodes in pool to set the full array operations */
            if (SegmentsNumber > Device_p->SBlitNodeDataPool.NbFreeElem)
            {
                /* Stop protection */
                STOS_SemaphoreSignal(Device_p->AccessControl);

                return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);  /* is it the correct error to return ? */
            }
        }
        CurrentNodeDataPool_p = &(Device_p->SBlitNodeDataPool);
    }
    else  /* Job operation */
    {
        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }

        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            /* Check there is enough nodes in pool to set the full array operations */
            if (SegmentsNumber > Device_p->JBlitNodeDataPool.NbFreeElem)
            {
                /* Stop protection */
                STOS_SemaphoreSignal(Device_p->AccessControl);

                return(STBLIT_ERROR_MAX_JOB_NODE);  /* is it the correct error to return ? */
            }
        }
        CurrentNodeDataPool_p = &(Device_p->JBlitNodeDataPool);
    }

    /* Check that no color conversion */

    /* Check other supported blit context parameters */

    /* Initialize CommonField value */
    memset(&CommonField, 0, sizeof(stblit_CommonField_t));

    /* Set operation : All none supported features in the blit context are ignored. */
    if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
    {
    /* This implementation consists of an emulation of the XYL engine since it is not present on 7015 device but on 7020/ST40GX1
     * One way to emulate it, is to have a blit node per pixel to set. This solution implies really bad performances since number
     * of nodes could be high but it allows the user to take advantage of HW blitter features.
     * This solution is the one implemented !
     * Note that with a full software emulation (no use of blitter), the blit context could not be as rich. In fact it could
     * be but with a significant sw effort...TBD ..... */

        /* At this moment no Mask support (first implementation)-> ScanConfig =0 -> XOffset =Y Offset = 0 */

        /* First Node setting according to context analysis and stuff... */
        Err = stblit_OnePassDrawHLineOperation(Device_p, Bitmap_p,XYLArrayFromCpu_p->PositionX, XYLArrayFromCpu_p->PositionY,XYLArrayFromCpu_p->Length,
                                        ColorArray_p, BlitContext_p,&NodeFirstHandle, &OperationCfg, &CommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }

#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
        if (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015)
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /*  Extract the Scan Direction for destination because of overlap Mask/Dst: This direction is the same for Src1 (if enabled) */
/*        ScanConfig =(U8)((Node_p->HWNode.BLT_TTY  >> STBLIT_TTY_SCAN_ORDER_SHIFT) & STBLIT_TTY_SCAN_ORDER_MASK);*/
        XOffset = 0;
        YOffset = 0;
/*        if (((ScanConfig & 0x2) >> 1) == 0)*/
/*        {*/
/*            YOffset = 0;*/
/*        }*/
/*        else*/
/*        {*/
/*            YOffset = Rectangle_p->Height - 1;*/
/*        }*/
/*        if ((ScanConfig & 0x1) == 0)*/
/*        {*/
/*            XOffset = 0;*/
/*        }*/
/*        else*/
/*        {*/
/*            XOffset = Rectangle_p->Width - 1;*/
/*        }*/

        /* Update CommonField in Node first handle node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&CommonField);

        /* Other nodes ('til PixelsNumber) have got exactly the same parameters as the first one except their X and Y coordinates
         * A copy of the first node is done and X, Y coordinates are updated : No need of further Context Analysis */
        PreviousNode_p = (stblit_Node_t*)NodeFirstHandle;

        for(i = 1; i < SegmentsNumber; i++)
        {
			U32 TempAddress;

			/* Increment XYLArray_p */
            XYLArrayFromCpu_p++;

			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&Handle;

            /* Get Node */
            stblit_GetElement(CurrentNodeDataPool_p, (void**) TempAddress);

            /* Copy Node */
            *((stblit_Node_t*)Handle) =  *((stblit_Node_t*)NodeFirstHandle);

            /* Connect to Previous node */
            stblit_Connect2Nodes(Device_p,PreviousNode_p,(stblit_Node_t*)Handle);

            /* Update Dst and Src1 X and Y coordinates.
             * In Case Src1 is not enabled (bypass Src2), Src1 X,Y updates have no effect */
            TXY = ((U32)((XYLArrayFromCpu_p->PositionX + XOffset)& STBLIT_TXY_X_MASK) << STBLIT_TXY_X_SHIFT);
            TXY |=((U32)((XYLArrayFromCpu_p->PositionY + YOffset)& STBLIT_TXY_Y_MASK) << STBLIT_TXY_Y_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_TXY)),TXY);

            S1XY = ((U32)((XYLArrayFromCpu_p->PositionX + XOffset)& STBLIT_S1XY_X_MASK) << STBLIT_S1XY_X_SHIFT);
            S1XY |= ((U32)((XYLArrayFromCpu_p->PositionY + YOffset)& STBLIT_S1XY_Y_MASK) << STBLIT_S1XY_Y_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S1XY)),S1XY);

            /* No XYL engine, i.e one color fill per node
             * Update Color Fill in Src2 (its type is unchanged, no need to update it)
             * The color must be set according to input to the internal src formatter */
            stblit_FormatColorForInputFormatter((STGXOBJ_Color_t*)(ColorArray_p + i),&Color) ;
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S2CF )),((U32)(Color & STBLIT_S2CF_COLOR_MASK)
                                                                << STBLIT_S2CF_COLOR_SHIFT));

            /* Update Dst and Src1 Size */
            S1SZ = ((U32)(XYLArrayFromCpu_p->Length & STBLIT_S1SZ_WIDTH_MASK) << STBLIT_S1SZ_WIDTH_SHIFT);
            S1SZ |= ((U32)(1 & STBLIT_S1SZ_HEIGHT_MASK) << STBLIT_S1SZ_HEIGHT_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S1SZ)),S1SZ);

            /* Update Src2 size = Dst/Src1 size (Size can not be ignored : HW constraint) */
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_S2SZ)),S1SZ);

            /* Update NIP */
            STSYS_WriteRegMem32LE((void*)(&(((stblit_Node_t*)Handle)->HWNode.BLT_NIP)),0);

            /* Update Previous */
            PreviousNode_p = (stblit_Node_t*)Handle;
        }

        /* Update interruption for last node if completion needed (PreviousNode is the last node at this stage!)*/
        if (BlitContext_p->NotifyBlitCompletion == TRUE)
        {
            PreviousNode_p->ITOpcode |= STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
            /* Update CommonField  */
            stblit_WriteCommonFieldInNode(PreviousNode_p ,&CommonField);
        }

        NodeLastHandle  = (stblit_NodeHandle_t)PreviousNode_p;
        NumberNodes     = SegmentsNumber;
    }
    else
    {
        /* Analyse color Array and copy colors in Reserved field from XYLArray */
        for (i = 0; i < SegmentsNumber; i++)
        {
            /* color stting for XYL engine
             * The color must be set according to output of the internal src formatter
             * This is the contrary of norma fill operation!!!! */
             stblit_FormatColorForOutputFormatter((STGXOBJ_Color_t*)(ColorArray_p + i),&Color) ;
            ((STBLIT_XYL_t*)(XYLArrayFromCpu_p + i))->Reserved = Color;
        }

        Err = stblit_OnePassDrawArrayOperation(Device_p, Bitmap_p, STBLIT_ARRAY_TYPE_XYLC,(void*)XYLArray_p,(STGXOBJ_Color_t*)NULL,
                                           SegmentsNumber, BlitContext_p,&NodeFirstHandle, &OperationCfg,&CommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Stop protection */
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }

#if defined (WA_FILL_OPERATION) || defined (WA_SOFT_RESET_EACH_NODE)
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            CommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /* Update CommonField in Node first handle node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&CommonField);

        NodeLastHandle = NodeFirstHandle;
        NumberNodes = 1;
    }

    /* Stop protection */
    STOS_SemaphoreSignal(Device_p->AccessControl);

#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeFirstHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeFirstHandle, (void*)NodeLastHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeFirstHandle,NodeLastHandle, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeFirstHandle, NodeLastHandle, NumberNodes, &OperationCfg);
    }

    return(Err);

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

    /* Configure node for the concatenation */
    Err = stblit_OnePassConcatOperation(Device_p, SrcAlphaBitmap_p,SrcAlphaRectangle_p, SrcColorBitmap_p,
                                        SrcColorRectangle_p, DstBitmap_p, DstRectangle_p, BlitContext_p,
                                        &NodeHandle, &OperationCfg,&CommonField);
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
    stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeHandle ,&CommonField);


#ifdef STBLIT_TEST_FRONTEND
    STBLIT_FirstNodeAddress = (U32)NodeHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeHandle, (void*)NodeHandle, FALSE, FALSE);
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
	/* To remove comipation warnning */
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
    else if (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB)  /* Note that a MB is a 16*16 Pixel area */
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

    Device_p = Unit_p->Device_p ;

    STOS_SemaphoreWait(Device_p->AccessControl);

    (*FlickerFilterMode_p)=Device_p->FlickerFilterMode;

    STOS_SemaphoreSignal(Device_p->AccessControl);

    return(Err);
}

/*--------------------------------------------------------------------------------
Set FilterFlicker Mode  API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SetFlickerFilterMode( STBLIT_Handle_t                  Handle,
                                            STBLIT_FlickerFilterMode_t       FlickerFilterMode)

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


    Device_p = Unit_p->Device_p ;

    STOS_SemaphoreWait(Device_p->AccessControl);

    Device_p->FlickerFilterMode=FlickerFilterMode;

    STOS_SemaphoreSignal(Device_p->AccessControl);

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




/* End of blt_fe.c */
