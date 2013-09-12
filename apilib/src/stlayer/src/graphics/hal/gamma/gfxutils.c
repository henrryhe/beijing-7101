/*******************************************************************************

File name   : gfxutils.c

Description : gfxutil source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                  Name
----               ------------                                  ----
2000-06-19         Created                                       Adriano Melis
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#ifndef ST_OSLINUX
    #include <string.h>
#endif

#include "stddefs.h"
#include "layergfx.h"
#include "frontend.h"

#include "stos.h"

/* Macros ------------------------------------------------------------------- */
#if defined (ST_7200)
#define Macro_TestSupportedFormat(ColorType)                            \
    {                                                                   \
        switch(stlayer_context.XContext[LayerHandle].LayerType)         \
        {                                                               \
            case STLAYER_GAMMA_CURSOR:                                  \
                if (ColorType != STGXOBJ_COLOR_TYPE_CLUT8)              \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
                                                                        \
            case STLAYER_GAMMA_GDP:                                     \
            case STLAYER_GAMMA_GDPVBI:                                     \
            case STLAYER_GAMMA_FILTER:                                  \
                if ((ColorType != STGXOBJ_COLOR_TYPE_RGB565)            \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_RGB888)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8565)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8565_255)       \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8888)          \
		    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8888_255)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB1555)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB4444)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_CLUT8)             \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444)    \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444)  \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888)    \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 )) \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
                                                                        \
            case STLAYER_GAMMA_BKL:                                     \
                if ((ColorType != STGXOBJ_COLOR_TYPE_RGB565)            \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB1555)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB4444)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 )) \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
                                                                        \
            default:                                                    \
                return (ST_ERROR_FEATURE_NOT_SUPPORTED);                \
                break;                                                  \
        }                                                               \
    }
#else /* !7200 */

#if defined (ST_7109) /* Add of CLUT8 support for GDP layer */
#define Macro_TestSupportedFormat(ColorType)                            \
    {                                                                   \
        switch(stlayer_context.XContext[LayerHandle].LayerType)         \
        {                                                               \
            case STLAYER_GAMMA_CURSOR:                                  \
                if (ColorType != STGXOBJ_COLOR_TYPE_CLUT8)              \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
                                                                        \
            case STLAYER_GAMMA_GDP:                                     \
            case STLAYER_GAMMA_GDPVBI:                                     \
            case STLAYER_GAMMA_FILTER:                                  \
                if ((ColorType != STGXOBJ_COLOR_TYPE_RGB565)            \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_RGB888)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8565)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8565_255)       \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8888)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8888_255)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB1555)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB4444)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_CLUT8)              \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444)    \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444)  \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888)    \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 )) \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);                      \
                }                                                       \
                break;                                                  \
                                                                        \
            case STLAYER_GAMMA_BKL:                                     \
                if ((ColorType != STGXOBJ_COLOR_TYPE_RGB565)            \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB1555)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB4444)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 )) \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
                                                                        \
            case STLAYER_GAMMA_ALPHA:                                   \
                if ((ColorType != STGXOBJ_COLOR_TYPE_ALPHA1)            \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_ALPHA8)           \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_ALPHA8_255))      \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
            default:                                                    \
                return (ST_ERROR_FEATURE_NOT_SUPPORTED);                \
                break;                                                  \
        }                                                               \
    }
#else /* !7109 */
#define Macro_TestSupportedFormat(ColorType)                            \
    {                                                                   \
        switch(stlayer_context.XContext[LayerHandle].LayerType)         \
        {                                                               \
            case STLAYER_GAMMA_CURSOR:                                  \
                if (ColorType != STGXOBJ_COLOR_TYPE_CLUT8)              \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
                                                                        \
            case STLAYER_GAMMA_GDP:                                     \
            case STLAYER_GAMMA_GDPVBI:                                     \
            case STLAYER_GAMMA_FILTER:                                  \
                if ((ColorType != STGXOBJ_COLOR_TYPE_RGB565)            \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_RGB888)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8565)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8565_255)       \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8888)          \
		    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB8888_255)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB1555)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB4444)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444)    \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444)  \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888)    \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 )) \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
                                                                        \
            case STLAYER_GAMMA_BKL:                                     \
                if ((ColorType != STGXOBJ_COLOR_TYPE_RGB565)            \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB1555)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_ARGB4444)          \
                    &&(ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 )) \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
                                                                        \
            case STLAYER_GAMMA_ALPHA:                                   \
                if ((ColorType != STGXOBJ_COLOR_TYPE_ALPHA1)            \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_ALPHA8)            \
                    &&(ColorType !=STGXOBJ_COLOR_TYPE_ALPHA8_255))       \
                {                                                       \
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);             \
                }                                                       \
                break;                                                  \
            default:                                                    \
                return (ST_ERROR_FEATURE_NOT_SUPPORTED);                \
                break;                                                  \
        }                                                               \
    }
#endif  /* 7109 */
#endif  /* 7200 */
/* Functions ---------------------------------------------------------------- */
/*
--------------------------------------------------------------------------------
  LAYERGFX_GetVTGName()
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERGFX_GetVTGName(STLAYER_Handle_t LayerHandle,
                ST_DeviceName_t * const VTGName_p)
{

    if(stlayer_context.XContext[LayerHandle].Initialised == FALSE)
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        strncpy(*VTGName_p,stlayer_context.XContext[LayerHandle].VTGName, sizeof(ST_DeviceName_t));
        return(ST_NO_ERROR);
    }
}
/*
--------------------------------------------------------------------------------
  LAYERGFX_GetBitmapAllocParams()
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERGFX_GetBitmapAllocParams(STLAYER_Handle_t      LayerHandle,
                                            STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params1_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params2_p)
{
    Params1_p->AllocBlockParams.Alignment = 0;
    Params1_p->AllocBlockParams.Size      = 0;
    Params1_p->Pitch                      = 0;
    Params1_p->Offset                     = 0;
    Params2_p->AllocBlockParams.Alignment = 0;
    Params2_p->AllocBlockParams.Size      = 0;
    Params2_p->Pitch                      = 0;
    Params2_p->Offset                     = 0;


    Macro_TestSupportedFormat(Bitmap_p->ColorType);
    /* The macro returns an error or continue */

    /* Offset is 0 because we dont write any header in the bitmap data */
    /* allocation for a progressive bitmap */
    if(Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
        /* 2nd alloc ununsed */
        switch(Bitmap_p->ColorType)
        {
            /* 4-Byte format */
            case STGXOBJ_COLOR_TYPE_ARGB8888:
            case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            case STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888:
            case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
                Params1_p->AllocBlockParams.Alignment = 4;
                Params1_p->Pitch = 4 * Bitmap_p->Width;
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                    Bitmap_p->Height;
            break;
            /* 3-Byte formats */
            case STGXOBJ_COLOR_TYPE_ARGB8565:              /* fall through */
            case STGXOBJ_COLOR_TYPE_ARGB8565_255:              /* fall through */
            case STGXOBJ_COLOR_TYPE_RGB888:                /* fall through */
            case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:   /* fall through */
            case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
                Params1_p->AllocBlockParams.Alignment = 4;
                Params1_p->Pitch = (Bitmap_p->Width * 3);
                if (Params1_p->Pitch %4 != 0)
                {
                    Params1_p->Pitch += (4 - Bitmap_p->Width % 4);
                }
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                    Bitmap_p->Height;
            break;
            /* 2-Byte formats */
            case STGXOBJ_COLOR_TYPE_RGB565:   /* fall through */
            case STGXOBJ_COLOR_TYPE_ARGB1555: /* fall through */
            case STGXOBJ_COLOR_TYPE_ARGB4444: /* fall through */
            case STGXOBJ_COLOR_TYPE_ACLUT88:
                Params1_p->AllocBlockParams.Alignment = 2;
                Params1_p->Pitch = 2 * Bitmap_p->Width;
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                Bitmap_p->Height;
            break;
            /* 1-Byte formats */
            case STGXOBJ_COLOR_TYPE_CLUT8:   /* fall through */
            case STGXOBJ_COLOR_TYPE_ACLUT44: /* fall through */
            case STGXOBJ_COLOR_TYPE_ALPHA8:
            case STGXOBJ_COLOR_TYPE_ALPHA8_255:
                Params1_p->AllocBlockParams.Alignment = 1;
                Params1_p->Pitch = Bitmap_p->Width;
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                Bitmap_p->Height;
            break;
            /* 1/2-Byte format */
            case STGXOBJ_COLOR_TYPE_CLUT4:
                Params1_p->AllocBlockParams.Alignment = 1;
                Params1_p->Pitch = (Bitmap_p->Width/2) +
                        ((Bitmap_p->Width & 0x1) ? 1 : 0);
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                Bitmap_p->Height;
            break;
            /* 1/4-Byte format */
            case STGXOBJ_COLOR_TYPE_CLUT2:
                Params1_p->AllocBlockParams.Alignment = 1;
                Params1_p->Pitch = (Bitmap_p->Width/4)
                            + ((Bitmap_p->Width%4) ? 1 : 0);
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                    Bitmap_p->Height;
            break;
            /* 1/8-Byte formats */
            case STGXOBJ_COLOR_TYPE_CLUT1: /* fall through */
            case STGXOBJ_COLOR_TYPE_ALPHA1:
                Params1_p->AllocBlockParams.Alignment = 1;
                Params1_p->Pitch = (Bitmap_p->Width/8)
                            + ((Bitmap_p->Width%8) ? 1 : 0);
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                 Bitmap_p->Height;
            break;
            case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422: /* fall through */
            case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
                Params1_p->AllocBlockParams.Alignment = 4;
                Params1_p->Pitch = (Bitmap_p->Width*2) +
                          ((Bitmap_p->Width & 0x1) ? 2 : 0);
                Params1_p->AllocBlockParams.Size = Params1_p->Pitch *
                                                 Bitmap_p->Height;
            break;

            default:
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
        } /* end switch */

    } /* endif bitmap is progressive */
    else /* if bitmap is top/bottom or macro block */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
}


/*
--------------------------------------------------------------------------------
  LAYERGFX_GetBitmapHeaderSize()
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERGFX_GetBitmapHeaderSize(STLAYER_Handle_t  LayerHandle,
                                           STGXOBJ_Bitmap_t* Bitmap_p,
                                           U32 *             HeaderSize_p)
{
    * HeaderSize_p = 0;

    Macro_TestSupportedFormat(Bitmap_p->ColorType);
    /* The macro returns an error or continue */

    /* No HeaderSize in this version */
    * HeaderSize_p = 0;
    return(ST_NO_ERROR);
}

/*
--------------------------------------------------------------------------------
  LAYERGFX_GetPaletteAllocParams()
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t LAYERGFX_GetPaletteAllocParams(STLAYER_Handle_t  LayerHandle,
                                        STGXOBJ_Palette_t*     Palette_p,
                                        STGXOBJ_PaletteAllocParams_t* Params_p)
{
    if((stlayer_context.XContext[LayerHandle].LayerType != STLAYER_GAMMA_CURSOR)&&(stlayer_context.XContext[LayerHandle].LayerType != STLAYER_GAMMA_GDP))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if(Palette_p == NULL)
    {
       return(ST_ERROR_BAD_PARAMETER);
    }


    /* the color format of a palette can be only one of these three */
    if(Palette_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB8888  &&
        Palette_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB4444 )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    switch(Palette_p->ColorType)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888: /* fall through */
            Params_p->AllocBlockParams.Alignment = 16;
            Params_p->AllocBlockParams.Size = 4 * (1 << Palette_p->ColorDepth);
        break;
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            Params_p->AllocBlockParams.Alignment = 16;
            Params_p->AllocBlockParams.Size = 2 * (1 << Palette_p->ColorDepth);
        break;
        default:
        break;
    }
    return(ST_NO_ERROR);
}


