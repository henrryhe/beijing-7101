
/******************************************************************************
*	COPYRIGHT (C) STMicroelectronics 1998
*
*   File name   : blt_surf.c
*   Description : surface orientated testing macros
*
*	Date          Modification                                    Initials
*	----          ------------                                    --------
*   12 Apr 2002   Creation                                        GS
*
******************************************************************************/

/*###########################################################################*/
/*############################## INCLUDES FILE ##############################*/
/*###########################################################################*/

#include <stdio.h>
#include <string.h>

#ifdef ST_OS21
    #include "os21debug.h"
    #include <sys/stat.h>
    #include <fcntl.h>
	#include "stlite.h"
#endif /*End of ST_OS21*/

#ifdef ST_OS20
	#include <debug.h>  /* used to read a file */
	#include "stlite.h"
#endif /*End of ST_OS20*/

#include "stddefs.h"
#include "stdevice.h"
#include "testtool.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "api_test.h"
#include "stblit.h"
#include "stcommon.h"
#include "stevt.h"
#include "stavmem.h"
#include "stsys.h"
#include "stgxobj.h"
#include "clevt.h"
#include "startup.h"
#include "blt_surf.h"
#include "api_cmd.h"

#if !defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
extern STBLIT_BlitContext_t ContextGen;

/*###########################################################################*/
/*############################### DEFINITION ################################*/
/*###########################################################################*/

/* --- Constants --- */
#define MAX_NAME_SIZE       16
#define MAX_FORMAT_SIZE     16
#define MAX_PATTERN_SIZE    32
#define MAX_TEST_NAME_SIZE  32

/* --- Prototypes --- */

/* --- Global variables --- */
static semaphore_t*  BlitCompleteSem;

/* --- Externals --- */
extern STAVMEM_PartitionHandle_t    AvmemPartitionHandle[];

/* --- Private functions prototypes---*/

/* --- Private types --- */
typedef struct _SURF_DESC {
    char                    Name[MAX_NAME_SIZE];
    STGXOBJ_ColorType_t     Type;
    U32                     Width;
    U32                     Height;
    U32                     Pitch;
    STAVMEM_BlockHandle_t   BlockHandle;
    struct _SURF_DESC       *pNext;

    STGXOBJ_Bitmap_t        Bitmap;
    STGXOBJ_Palette_t       Palette;

} SURF_DESC;

SURF_DESC *pFirstSurface = NULL;


typedef U32 (*PatternFn)(S32, S32, STGXOBJ_ColorType_t);


/* --- Functions --- */


static void BlitCompletionCallback(
    STEVT_CallReason_t Reason,
    STEVT_EventConstant_t Event,
    const void *EventData)
{
    STOS_SemaphoreSignal(BlitCompleteSem);
}

STEVT_Handle_t          EvtHandle;
STEVT_OpenParams_t      EvtOpenParams;
STEVT_SubscriberID_t    SubscrId;
#ifdef ST_OSLINUX
STEVT_SubscribeParams_t SubscrParams = { BlitCompletionCallback, NULL };
#else
STEVT_SubscribeParams_t SubscrParams = { BlitCompletionCallback };
#endif

void surf_open_evt()
{
    ST_ErrorCode_t          Error;

    /*** Open STEVT and subscribe to blit completion event ***/

    Error = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if(Error != ST_NO_ERROR)
    {
        STTBX_Print(("STEVT_Open() FAILED\n"));
        return;
    }
#if 1
    Error = STEVT_Subscribe(EvtHandle, STBLIT_BLIT_COMPLETED_EVT, &SubscrParams);
    if(Error != ST_NO_ERROR)
    {
        STTBX_Print(("STEVT_Subscribe() FAILED\n"));
        return;
    }

    /* currently you won't get a subscriber ID
    until you've made at least one subscription */
    Error = STEVT_GetSubscriberID(EvtHandle, &SubscrId);
    if(Error != ST_NO_ERROR)
    {
        STTBX_Print(("STEVT_GetSubscriberId() FAILED\n"));
        return;
    }
#endif

    BlitCompleteSem = STOS_SemaphoreCreateFifo(NULL, 0 );
}

void surf_close_evt()
{
    ST_ErrorCode_t          Error;

    Error = STEVT_Close(EvtHandle);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Error));
        return;
    }
}


void surf_print_error(char *Function, ST_ErrorCode_t ErrCode)
{

    STTBX_Print(("ERROR "));
    STTBX_Print((Function));
    STTBX_Print((": "));
    API_ErrorCount++;

	switch ( ErrCode )
	{
       case ST_ERROR_INVALID_HANDLE :
            STTBX_Print(("Handle Invalid\n"));
            break;
        case ST_ERROR_BAD_PARAMETER :
            STTBX_Print(("The specified parameters are invalid\n"));
            break;
        case STBLIT_ERROR_MAX_JOB_NODE :
            STTBX_Print(("The maximum number of job node has been reached\n"));
            break;
        case STBLIT_ERROR_MAX_SINGLE_BLIT_NODE :
            STTBX_Print(("The maximum number of single node has been reached\n"));
            break;
        case STBLIT_ERROR_INVALID_JOB_HANDLE :
            STTBX_Print(("Job Handle Invalid\n"));
            break;
        case STBLIT_ERROR_JOB_HANDLE_MISMATCH :
            STTBX_Print(("The valid job handle doesn't belong to the specified open handle context\n"));
            break;
        case STBLIT_ERROR_JOB_CLOSED :
            STTBX_Print(("It is no more possible to modify the job\n"));
            break;
        default:
            STTBX_Print(("unexpected error [%X]\n",ErrCode));
	        break;
    }
}


SURF_DESC *surf_find(char *Name)
{
    SURF_DESC *pSurf = pFirstSurface;

    while (pSurf)
    {
        if (strncmp(Name, pSurf->Name, MAX_NAME_SIZE) == 0)
            return pSurf;

        pSurf = pSurf->pNext;
    }

    /* found surface in list */
    return NULL;
}

typedef struct
{
    char                *Name;
    STGXOBJ_ColorType_t Type;
} SurfaceFormats_t;

SurfaceFormats_t Formats[] =
{
    { "ARGB8888",        STGXOBJ_COLOR_TYPE_ARGB8888              },
    { "RGB888",          STGXOBJ_COLOR_TYPE_RGB888                },
    { "ARGB8565",        STGXOBJ_COLOR_TYPE_ARGB8565              },
    { "RGB565",          STGXOBJ_COLOR_TYPE_RGB565                },
    { "ARGB1555",        STGXOBJ_COLOR_TYPE_ARGB1555              },
    { "ARGB4444",        STGXOBJ_COLOR_TYPE_ARGB4444              },
    { "ACLUT88",         STGXOBJ_COLOR_TYPE_ACLUT88               },
    { "S_YCBCR888_444",  STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444   },
    { "U_YCBCR888_444",  STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444 },
    { "S_YCBCR888_422",  STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422   },
    { "U_YCBCR888_422",  STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 },
    { "S_YCBCR888_420",  STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420   },
    { "U_YCBCR888_420",  STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420 },
    { "U_AYCBCR6888_444",STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444},
    { "CLUT8",           STGXOBJ_COLOR_TYPE_CLUT8                 },
    { "ACLUT44",         STGXOBJ_COLOR_TYPE_ACLUT44               },
    { "ALPHA8",          STGXOBJ_COLOR_TYPE_ALPHA8                },
    { "BYTE",            STGXOBJ_COLOR_TYPE_BYTE                  },
    { "CLUT4",           STGXOBJ_COLOR_TYPE_CLUT4                 },
    { "ALPHA4",          STGXOBJ_COLOR_TYPE_ALPHA4                },
    { "CLUT2",           STGXOBJ_COLOR_TYPE_CLUT2                 },
    { "CLUT1",           STGXOBJ_COLOR_TYPE_CLUT1                 },
    { "ALPHA1",          STGXOBJ_COLOR_TYPE_ALPHA1                },
    { "RGB565",          STGXOBJ_COLOR_TYPE_RGB565                },
};

BOOL surf_parse_format(char *Format, STGXOBJ_ColorType_t *Type)
{
    int i;

    for (i = 0; i < (sizeof(Formats) / sizeof(Formats[0])); i++)
    {
        if (strcmp(Format, Formats[i].Name) == 0)
        {
            *Type = Formats[i].Type;
            return FALSE;
        }
    }

    /* Failed to locate format */
    return TRUE;
}

U32 surf_get_type_bpp(STGXOBJ_ColorType_t Type)
{

    switch (Type)
    {
    case STGXOBJ_COLOR_TYPE_ARGB8888:
        return 32;

    case STGXOBJ_COLOR_TYPE_RGB888:
    case STGXOBJ_COLOR_TYPE_ARGB8565:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
        return 24;

    case STGXOBJ_COLOR_TYPE_RGB565:
    case STGXOBJ_COLOR_TYPE_ARGB1555:
    case STGXOBJ_COLOR_TYPE_ARGB4444:
    case STGXOBJ_COLOR_TYPE_ACLUT88:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
        return 16;

    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
        return 12;

    case STGXOBJ_COLOR_TYPE_CLUT8:
    case STGXOBJ_COLOR_TYPE_ACLUT44:
    case STGXOBJ_COLOR_TYPE_ALPHA8:
    case STGXOBJ_COLOR_TYPE_BYTE:
        return 8;

    case STGXOBJ_COLOR_TYPE_CLUT4:
    case STGXOBJ_COLOR_TYPE_ALPHA4:
        return 4;

    case STGXOBJ_COLOR_TYPE_CLUT2:
        return 2;

    case STGXOBJ_COLOR_TYPE_CLUT1:
    case STGXOBJ_COLOR_TYPE_ALPHA1:
        return 1;

    default:
        STTBX_Print(("Invalid Colour Format Type\n"));
        API_ErrorCount++;
        return 32;

    }
    return 32;
}


U32 surf_ARGB888_to_Type(U32 ARGB, STGXOBJ_ColorType_t Type)
{

    switch (Type)
    {
    case STGXOBJ_COLOR_TYPE_ARGB8888:
        return ARGB;

    case STGXOBJ_COLOR_TYPE_RGB888:
        return ARGB & 0xFFFFFF;

    case STGXOBJ_COLOR_TYPE_ARGB8565:
        return (((ARGB & 0xFFF80000) >> 8) |
                ((ARGB & 0x0000FC00) >> 5) |
                ((ARGB & 0x000000F8) >> 3));

    case STGXOBJ_COLOR_TYPE_RGB565:
        return (((ARGB & 0x00F80000) >> 8) |
                ((ARGB & 0x0000FC00) >> 5) |
                ((ARGB & 0x000000F8) >> 3));

    case STGXOBJ_COLOR_TYPE_ARGB1555:
        return (((ARGB & 0x80000000) >> 16) |
                ((ARGB & 0x00F80000) >> 9) |
                ((ARGB & 0x0000F800) >> 6) |
                ((ARGB & 0x000000F8) >> 3));

    case STGXOBJ_COLOR_TYPE_ARGB4444:
        return (((ARGB & 0xF0000000) >> 16) |
                ((ARGB & 0x00F00000) >> 12) |
                ((ARGB & 0x0000F000) >> 8) |
                ((ARGB & 0x000000F0) >> 4));

    case STGXOBJ_COLOR_TYPE_ALPHA8:
        return ARGB >> 24;

    case STGXOBJ_COLOR_TYPE_ALPHA4:
        return ARGB >> 28;

    case STGXOBJ_COLOR_TYPE_ALPHA1:
        return ARGB >> 31;

    case STGXOBJ_COLOR_TYPE_ACLUT88:
        return  ((ARGB & 0xFF000000) >> 16) |
                ((ARGB & 0xFF0000) >> 16) |
                ((ARGB & 0x00FF00) >> 8) |
                (ARGB & 0x0000FF);

    case STGXOBJ_COLOR_TYPE_CLUT8:
    case STGXOBJ_COLOR_TYPE_BYTE:
        return  ((ARGB & 0xFF0000) >> 16) |
                ((ARGB & 0x00FF00) >> 8) |
                (ARGB & 0x0000FF);

    case STGXOBJ_COLOR_TYPE_ACLUT44:
        return  ((ARGB & 0xF0000000) >> 24) |
                ((ARGB & 0xF00000) >> 20) |
                ((ARGB & 0x00F000) >> 12) |
                ((ARGB & 0x0000F0) >> 4);

    case STGXOBJ_COLOR_TYPE_CLUT4:
        return  ((ARGB & 0xF00000) >> 20) |
                ((ARGB & 0x00F000) >> 12) |
                ((ARGB & 0x0000F0) >> 4);

    case STGXOBJ_COLOR_TYPE_CLUT2:
        return  ((ARGB & 0xC00000) >> 22) |
                ((ARGB & 0x00C000) >> 14) |
                ((ARGB & 0x0000C0) >> 6);

    case STGXOBJ_COLOR_TYPE_CLUT1:
        return  ((ARGB & 0x800000) >> 23) |
                ((ARGB & 0x008000) >> 15) |
                ((ARGB & 0x000080) >> 7);

    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
        {

            U32 Y, Cb, Cr;
            U32 R = (ARGB >> 16) & 0xFF;
            U32 G = (ARGB >> 8) & 0xFF;
            U32 B = ARGB & 0xFF;

            /* Some rough calculations for the appropriate colour */
			Cb  = (U8)(-0.14*R - 0.29*G + 0.43*B);
		    Y   = (U8)(0.29*R + 0.59*G + 0.14*B);
			Cr  = (U8)(0.36*R - 0.29*G - 0.07*B);

            return (((Y & 0xFF) << 24)  |
                    ((Cr & 0xFF) << 16) |
                    ((Y & 0xFF) << 8)   |
                    (Cb & 0xFF));
        }

    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
        {

            U32 Y, Cb, Cr;
            U32 R = (ARGB >> 16) & 0xFF;
            U32 G = (ARGB >> 8) & 0xFF;
            U32 B = ARGB & 0xFF;

            /* Some rough calculations for the appropriate colour */
			Cb  = (U8)(128.0 - 0.14*R - 0.29*G + 0.43*B);
		    Y   = (U8)(0.29*R + 0.59*G + 0.14*B);
			Cr  = (U8)(128.0 + 0.36*R - 0.29*G - 0.07*B);

            return (((Y & 0xFF) << 24)  |
                    ((Cr & 0xFF) << 16) |
                    ((Y & 0xFF) << 8)   |
                    (Cb & 0xFF));
        }

    case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
        {

            U32 Y, Cb, Cr;
            U32 R = (ARGB >> 16) & 0xFF;
            U32 G = (ARGB >> 8) & 0xFF;
            U32 B = ARGB & 0xFF;

            /* Some rough calculations for the appropriate colour */
			Cb  = (U8)(128.0 - 0.14*R - 0.29*G + 0.43*B);
		    Y   = (U8)(0.29*R + 0.59*G + 0.14*B);
			Cr  = (U8)(128.0 + 0.36*R - 0.29*G - 0.07*B);

            return ((ARGB & 0xff000000) |
                    ((Y & 0xFF) << 24)  |
                    ((Cr & 0xFF) << 16) |
                    ((Y & 0xFF) << 8)   |
                    (Cb & 0xFF));
        }

    default:
        STTBX_Print(("Invalid ARGB to Colour Type\n"));
        API_ErrorCount++;
        return 0;
    }
    return 32;
}


void surf_generate_gxobj_colour(STGXOBJ_Color_t *Colour, U32 ColourARGB)
{
    switch (Colour->Type)
    {
    case STGXOBJ_COLOR_TYPE_ARGB8888:
        Colour->Value.ARGB8888.Alpha = ColourARGB >> 24;
        Colour->Value.ARGB8888.R = (ColourARGB >> 16) & 255;
        Colour->Value.ARGB8888.G = (ColourARGB >> 8) & 255;
        Colour->Value.ARGB8888.B = ColourARGB & 255;
        break;

    case STGXOBJ_COLOR_TYPE_RGB888:
        Colour->Value.RGB888.R = (ColourARGB >> 16) & 255;
        Colour->Value.RGB888.G = (ColourARGB >> 8) & 255;
        Colour->Value.RGB888.B = ColourARGB & 255;
        break;

    case STGXOBJ_COLOR_TYPE_ARGB8565:
        Colour->Value.ARGB8565.Alpha = ColourARGB >> 24;
        Colour->Value.ARGB8565.R = (ColourARGB >> 19) & 31;
        Colour->Value.ARGB8565.G = (ColourARGB >> 10) & 63;
        Colour->Value.ARGB8565.B = (ColourARGB >> 3) & 31;
        break;

    case STGXOBJ_COLOR_TYPE_RGB565:
        Colour->Value.RGB565.R = (ColourARGB >> 19) & 31;
        Colour->Value.RGB565.G = (ColourARGB >> 10) & 63;
        Colour->Value.RGB565.B = (ColourARGB >> 3) & 31;
        break;

    case STGXOBJ_COLOR_TYPE_ARGB1555:
        Colour->Value.ARGB1555.Alpha = ColourARGB >> 31;
        Colour->Value.ARGB1555.R = (ColourARGB >> 19) & 31;
        Colour->Value.ARGB1555.G = (ColourARGB >> 11) & 31;
        Colour->Value.ARGB1555.B = (ColourARGB >> 3) & 31;
        break;

    case STGXOBJ_COLOR_TYPE_ARGB4444:
        Colour->Value.ARGB4444.Alpha = ColourARGB >> 28;
        Colour->Value.ARGB4444.R = (ColourARGB >> 20) & 15;
        Colour->Value.ARGB4444.G = (ColourARGB >> 12) & 15;
        Colour->Value.ARGB4444.B = (ColourARGB >> 4) & 15;
        break;

    case STGXOBJ_COLOR_TYPE_ALPHA8:
        Colour->Value.ALPHA8 = ColourARGB >> 24;
        break;

    case STGXOBJ_COLOR_TYPE_ALPHA4:
        Colour->Value.ALPHA4 = ColourARGB >> 28;
        break;

    case STGXOBJ_COLOR_TYPE_ALPHA1:
        Colour->Value.ALPHA1 = ColourARGB >> 31;
        break;

    case STGXOBJ_COLOR_TYPE_CLUT8:
        Colour->Value.CLUT8 = (ColourARGB | (ColourARGB >> 8) | (ColourARGB >> 16)) & 0xFF;
        break;

    case STGXOBJ_COLOR_TYPE_BYTE:
        Colour->Value.Byte = (ColourARGB | (ColourARGB >> 8) | (ColourARGB >> 16)) & 0xFF;
        break;

    case STGXOBJ_COLOR_TYPE_ACLUT88:
        Colour->Value.ACLUT88.Alpha = ColourARGB >> 24;
        Colour->Value.ACLUT88.PaletteEntry = (ColourARGB | (ColourARGB >> 8) | (ColourARGB >> 16)) & 0xFF;
        break;

    case STGXOBJ_COLOR_TYPE_ACLUT44:
        Colour->Value.ACLUT44.Alpha = ColourARGB >> 28;
        Colour->Value.ACLUT44.PaletteEntry = ((ColourARGB >> 4) | (ColourARGB >> 12) | (ColourARGB >> 20)) & 0xF;
        break;

    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:
    case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
        {
            U32 R = (ColourARGB >> 16) & 0xFF;
            U32 G = (ColourARGB >> 8) & 0xFF;
            U32 B = ColourARGB & 0xFF;

            /* Some rough calculations for the appropriate colour */
			Colour->Value.SignedYCbCr888_444.Cb   = (S8)(-0.14*R - 0.29*G + 0.43*B);
		    Colour->Value.SignedYCbCr888_444.Y    = (U8)(0.29*R + 0.59*G + 0.14*B);
			Colour->Value.SignedYCbCr888_444.Cr   = (S8)(+0.36*R - 0.29*G - 0.07*B);
        }
        break;
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
        {
            U32 R = (ColourARGB >> 16) & 0xFF;
            U32 G = (ColourARGB >> 8) & 0xFF;
            U32 B = ColourARGB & 0xFF;

            /* Some rough calculations for the appropriate colour */
			Colour->Value.UnsignedYCbCr888_444.Cb   = (U8)(128.0 - 0.14*R - 0.29*G + 0.43*B);
		    Colour->Value.UnsignedYCbCr888_444.Y    = (U8)(0.29*R + 0.59*G + 0.14*B);
			Colour->Value.UnsignedYCbCr888_444.Cr   = (U8)(128.0 + 0.36*R - 0.29*G - 0.07*B);
        }
        break;

    case STGXOBJ_COLOR_TYPE_CLUT4:
    case STGXOBJ_COLOR_TYPE_CLUT2:
    case STGXOBJ_COLOR_TYPE_CLUT1:
    case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
    default:
        STTBX_Print(("Invalid ARGB to GXOBJ Colour Type\n"));
        API_ErrorCount++;
    }
}


BOOL surf_string_colour(char *ColourString, U32 *pColour)
{
    if (strcmp(ColourString, "Red") == 0)
    {
        *pColour = 0xFF0000;
        return TRUE;
    }

    if (strcmp(ColourString, "Green") == 0)
    {
        *pColour = 0x00FF00;
        return TRUE;
    }

    if (strcmp(ColourString, "Blue") == 0)
    {
        *pColour = 0x0000FF;
        return TRUE;
    }

    if (strcmp(ColourString, "Black") == 0)
    {
        *pColour = 0x000000;
        return TRUE;
    }

    if (strcmp(ColourString, "White") == 0)
    {
        *pColour = 0xFFFFFF;
        return TRUE;
    }

    return FALSE;

}

U32 surf_checker_pattern(S32 X, S32 Y, STGXOBJ_ColorType_t Type)
{
    /* Use some statics to implement a destination format cache */
    static STGXOBJ_ColorType_t CachedType = STGXOBJ_COLOR_TYPE_ARGB8888;
    static U32 Blue = 0x0000FF;
    static U32 White = 0xFFFFFF;

    if (CachedType != Type)
    {
        Blue = surf_ARGB888_to_Type(0x0000FF, Type);
        White = surf_ARGB888_to_Type(0xFFFFFF, Type);

        CachedType = Type;
    }

    /* 9x9 white/blue squares. 9x9 chosen because any sw bugs are likely to
       involve powers of 2 .... */
    if (((U32)X/9 + (U32)Y/9) & 1)
        return Blue;
    else
        return White;
}

U32 surf_diagonal_pattern(S32 X, S32 Y, STGXOBJ_ColorType_t Type)
{
    /* Use some statics to implement a destination format cache */
    static STGXOBJ_ColorType_t CachedType = STGXOBJ_COLOR_TYPE_ARGB8888;
    static U32 Blue = 0x0000FF;
    static U32 White = 0xFFFFFF;

    if (CachedType != Type)
    {
        Blue = surf_ARGB888_to_Type(0x0000FF, Type);
        White = surf_ARGB888_to_Type(0xFFFFFF, Type);

        CachedType = Type;
    }

    /* 13 pixel wide white/blue diagonal lines. */
    if ((((U32)(X + Y))/13) & 1)
        return Blue;
    else
        return White;
}

U32 surf_gradient_pattern(S32 X, S32 Y, STGXOBJ_ColorType_t Type)
{

    switch (surf_get_type_bpp(Type))
    {
    case 8:
        return (X + Y) & 0xFF;
    case 16:
        return ((X & 0x1F) << 11) | ((Y & 0x3F) << 5) | (Y & 0x1F);
    default:
        return ((X & 0xFF) << 16) | ((Y & 0xFF) << 8) | (Y & 0xFF);
    }
}

PatternFn surf_string_pattern(char *Pattern)
{
    if (strcmp(Pattern, "Checker") == 0)
    {
        return surf_checker_pattern;
    }

    if (strcmp(Pattern, "Diagonal") == 0)
    {
        return surf_diagonal_pattern;
    }

    if (strcmp(Pattern, "Gradient") == 0)
    {
        return surf_gradient_pattern;
    }

    return NULL;

}


void surf_fill_pattern(SURF_DESC *pSurf, S32 X, S32 Y, U32 Width, U32 Height, char *Pattern)
{
    U32 Col;
    U8 *pLine;
    U32 i, j;

    STAVMEM_GetBlockAddress(pSurf->BlockHandle, (void **)&pLine);

    pLine += Y * pSurf->Pitch;

    if (surf_string_colour(Pattern, &Col)) {
        Col = surf_ARGB888_to_Type(Col, pSurf->Type);

        switch(surf_get_type_bpp(pSurf->Type))
        {
        case 8:
            {
                pLine += X;
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        *pData++ = Col;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 16:
            if (pSurf->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 ||
                pSurf->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
            {
                pLine += X * 2 + (X & 1);
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        if ((X + i) & 1)
                            *pData++ = Col >> 24;
                        else
                        {
                            *pData++ = Col & 0xFF;
                            *pData++ = (Col >> 8) & 0xFF;
                            *pData++ = (Col >> 16) & 0xFF;
                        }
                    }
                    pLine += pSurf->Pitch;
                }
            }
            else
            {
                pLine += X * 2;
                for (j = 0; j < Height; j++)
                {
                    U16 *pData = (U16 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        *pData++ = Col;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 24:
            {
                pLine += X * 3;
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        *pData++ = Col & 0xFF;
                        *pData++ = (Col & 0xFF00) >> 8;
                        *pData++ = (Col & 0xFF0000) >> 16;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 32:
            {
                pLine += X * 4;
                for (j = 0; j < Height; j++)
                {
                    U32 *pData = (U32 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        *pData++ = Col;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;

        default:
            STTBX_Print(("Unimplemented Fill depth\n"));
        }
        return;
    }
    else
    {
        PatternFn PatFn_p;

        PatFn_p = surf_string_pattern(Pattern);

        if (!PatFn_p)
        {
            STTBX_Print(("Invalid Pattern\n"));
            return;
        }

        switch(surf_get_type_bpp(pSurf->Type))
        {
        case 8:
            {
                pLine += X;
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        *pData++ = (U8)(PatFn_p(i+X, j+Y, pSurf->Type));
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 16:
            if (pSurf->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 ||
                pSurf->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
            {
                pLine += X * 2 + (X & 1);
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        Col = PatFn_p(i+X, j+Y, pSurf->Type) >> 16;
                        if ((X + i) & 1)
                            *pData++ = Col >> 24;
                        else
                        {
                            *pData++ = Col & 0xFF;
                            *pData++ = (Col >> 8) & 0xFF;
                            *pData++ = (Col >> 16) & 0xFF;
                        }
                    }
                    pLine += pSurf->Pitch;
                }
            }
            else
            {
                pLine += X * 2;
                for (j = 0; j < Height; j++)
                {
                    U16 *pData = (U16 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        *pData++ = (U16)(PatFn_p(i+X, j+Y, pSurf->Type));
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 24:
            {
                pLine += X * 3;
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        Col = PatFn_p(i+X, j+Y, pSurf->Type);
                        *pData++ = Col & 0xFF;
                        *pData++ = (Col & 0xFF00) >> 8;
                        *pData++ = (Col & 0xFF0000) >> 16;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 32:
            {
                pLine += X * 4;
                for (j = 0; j < Height; j++)
                {
                    U32 *pData = (U32 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        *pData++ = (U32)(PatFn_p(i+X, j+Y, pSurf->Type));
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;


        default:
            STTBX_Print(("Unimplemented Fill depth\n"));
        }
    }
}

BOOL surf_check_pattern(SURF_DESC *pSurf, S32 X, S32 Y, U32 Width, U32 Height, char *Pattern, S32 OffsetX, S32 OffsetY)
{
    U32 Col;
    U8 *pLine;
    U32 i, j;

    STAVMEM_GetBlockAddress(pSurf->BlockHandle, (void **)&pLine);

    pLine += Y * pSurf->Pitch;

    /* Check for a plain colour */
    if (surf_string_colour(Pattern, &Col))
    {
        Col = surf_ARGB888_to_Type(Col, pSurf->Type);

        switch(surf_get_type_bpp(pSurf->Type))
        {
        case 8:
            {
                pLine += X;
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        if (*pData != Col)
                        {
                            STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, *pData, Col));
                            API_ErrorCount++;
                            return FALSE;
                        }
                        pData++;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;

        case 16:
            if (pSurf->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 ||
                pSurf->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
            {
                pLine += X * 2 + (X & 1);
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        if ((X + i) & 1)
                        {
                            if (pData[0] != Col >> 24)
                            {
                                STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, pData[0], Col>>24));
                                API_ErrorCount++;
                                return FALSE;
                            }
                            pData++;
                        }
                        else
                        {
                            if ((pData[0] != (Col & 0xFF)) ||
                                (pData[1] != ((Col >> 8) & 0xFF)) ||
                                (pData[2] != ((Col >> 16) & 0xFF)))
                            {
                                STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x%x%x Expecting 0x%x\n", X + i, Y + j, pData[2], pData[1], pData[0], Col & 0xFFFFFF));
                                API_ErrorCount++;
                                return FALSE;
                            }
                            pData += 3;
                        }
                    }
                    pLine += pSurf->Pitch;
                }
            }
            else
            {
                pLine += X * 2;
                for (j = 0; j < Height; j++)
                {
                    U16 *pData = (U16 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        if (*pData != Col)
                        {
                            STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, *pData, Col));
                            API_ErrorCount++;
                            return FALSE;
                        }
                        pData++;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 24:
            {
                pLine += X * 3;
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        if (pData[0] != (Col & 0xFF) ||
                            pData[1] != ((Col & 0xFF00) >> 8) ||
                            pData[2] != ((Col & 0xFF0000) >> 16))
                        {
                            STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, *pData, Col));
                            API_ErrorCount++;
                            return FALSE;
                        }
                        pData += 3;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 32:
            {
                pLine += X * 4;
                for (j = 0; j < Height; j++)
                {
                    U32 *pData = (U32 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        if (*pData != Col)
                        {
                            STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, *pData, Col));
                            API_ErrorCount++;
                            return FALSE;
                        }
                        pData++;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;

        default:
            STTBX_Print(("Unimplemented Fill depth\n"));
            API_ErrorCount++;
            return FALSE;
        }

    }
    else
    {
        PatternFn PatFn_p;

        PatFn_p = surf_string_pattern(Pattern);

        if (!PatFn_p)
        {
            STTBX_Print(("Invalid Pattern\n"));
            API_ErrorCount++;
            return FALSE;
        }

        switch(surf_get_type_bpp(pSurf->Type))
        {
        case 8:
            {
                pLine += X;
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        Col = PatFn_p(i+X-OffsetX, j+Y-OffsetY, pSurf->Type);
                        if (*pData != Col)
                        {
                            STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, *pData, Col));
                            API_ErrorCount++;
                            return FALSE;
                        }
                        pData++;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;

        case 16:
            if (pSurf->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 ||
                pSurf->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
            {
                pLine += X * 2 + (X & 1);
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        Col = PatFn_p(i+X, j+Y, pSurf->Type) >> 16;
                        if ((X + i) & 1)
                        {
                            if (pData[0] != Col >> 24)
                            {
                                STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, pData[0], Col>>24));
                                API_ErrorCount++;
                                return FALSE;
                            }
                            pData++;
                        }
                        else
                        {
                            if ((pData[0] != Col & 0xFF) &&
                                (pData[1] != (Col >> 8) & 0xFF) &&
                                (pData[2] != (Col >> 16) & 0xFF))
                            {
                                STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x%x%x Expecting 0x%x\n", X + i, Y + j, pData[2], pData[1], pData[0], Col>>24));
                                API_ErrorCount++;
                                return FALSE;
                            }
                            pData += 3;
                        }
                    }
                    pLine += pSurf->Pitch;
                }

            }
            else
            {
                pLine += X * 2;
                for (j = 0; j < Height; j++)
                {
                    U16 *pData = (U16 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        Col = PatFn_p(i+X-OffsetX, j+Y-OffsetY, pSurf->Type);
                        if (*pData != Col)
                        {
                            STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, *pData, Col));
                            API_ErrorCount++;
                            return FALSE;
                        }
                        pData++;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 24:
            {
                pLine += X * 3;
                for (j = 0; j < Height; j++)
                {
                    U8 *pData = (U8 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        Col = PatFn_p(i+X-OffsetX, j+Y-OffsetY, pSurf->Type);
                        if (pData[0] != (Col & 0xFF) ||
                            pData[1] != ((Col & 0xFF00) >> 8) ||
                            pData[2] != ((Col & 0xFF0000) >> 16))
                        {
                            STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, pData[0] + (pData[1] << 8) + (pData[2] << 16), Col));
                            API_ErrorCount++;
                            return FALSE;
                        }
                        pData += 3;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;
        case 32:
            {
                pLine += X * 4;
                for (j = 0; j < Height; j++)
                {
                    U32 *pData = (U32 *)pLine;
                    for (i = 0; i < Width; i++)
                    {
                        Col = PatFn_p(i+X-OffsetX, j+Y-OffsetY, pSurf->Type);
                        if (*pData != Col)
                        {
                            STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y + j, *pData, Col));
                            API_ErrorCount++;
                            return FALSE;
                        }
                        pData++;
                    }
                    pLine += pSurf->Pitch;
                }
            }
            break;


        default:
            STTBX_Print(("Unimplemented Fill depth\n"));
            API_ErrorCount++;
            return FALSE;
        }
    }

    return FALSE;
}

BOOL surf_check_span(SURF_DESC *pSurf, S32 X, S32 Y, U32 Width, U32 Col)
{
    U8 *pLine;
    U32 i;

    Col = surf_ARGB888_to_Type(Col, pSurf->Type);

    STAVMEM_GetBlockAddress(pSurf->BlockHandle, (void **)&pLine);

    pLine += Y * pSurf->Pitch;

    switch(surf_get_type_bpp(pSurf->Type))
    {
    case 8:
        {
            U8 *pData = ((U8 *)pLine) + X;
            for (i = 0; i < Width; i++)
            {
                if (*pData != Col)
                {
                    STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y, *pData, Col));
                    API_ErrorCount++;
                    return FALSE;
                }
                pData++;
            }
        }
        break;

    case 16:
        if (pSurf->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 ||
            pSurf->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
        {
            U8 *pData = ((U8 *)pLine) + X * 2 + (X & 1);
            for (i = 0; i < Width; i++)
            {
                if ((X + i) & 1)
                {
                    if (pData[0] != Col >> 24)
                    {
                        STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y, pData[0], Col>>24));
                        API_ErrorCount++;
                        return FALSE;
                    }
                    pData++;
                }
                else
                {
                    if ((pData[0] != (Col & 0xFF)) ||
                        (pData[1] != ((Col >> 8) & 0xFF)) ||
                        (pData[2] != ((Col >> 16) & 0xFF)))
                    {
                        STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x%x%x Expecting 0x%x\n", X + i, Y, pData[2], pData[1], pData[0], Col & 0xFFFFFF));
                        API_ErrorCount++;
                        return FALSE;
                    }
                    pData += 3;
                }
            }
        }
        else
        {
            U16 *pData = ((U16 *)pLine) + X;
            for (i = 0; i < Width; i++)
            {
                if (*pData != Col)
                {
                    STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y, *pData, Col));
                    API_ErrorCount++;
                    return FALSE;
                }
                pData++;
            }
        }
        break;
    case 24:
        {
            U8 *pData = ((U8 *)pLine) + X * 3;
            for (i = 0; i < Width; i++)
            {
                if (pData[0] != (Col & 0xFF) ||
                    pData[1] != ((Col & 0xFF00) >> 8) ||
                    pData[2] != ((Col & 0xFF0000) >> 16))
                {
                    STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y, *pData, Col));
                    API_ErrorCount++;
                    return FALSE;
                }
                pData += 3;
            }
        }
        break;
    case 32:
        {
            U32 *pData = ((U32 *)pLine) + X;
            for (i = 0; i < Width; i++)
            {
                if (*pData != Col)
                {
                    STTBX_Print(("Unexpected colour at (%d, %d) Found 0x%x Expecting 0x%x\n", X + i, Y, *pData, Col));
                    API_ErrorCount++;
                    return FALSE;
                }
                pData++;
            }
        }
        break;

    default:
        STTBX_Print(("Unimplemented Fill depth\n"));
        API_ErrorCount++;
        return FALSE;
    }

    return FALSE;
}

BOOL surf_string_shape_xyl(char *ShapeString, U32 X, U32 Y, STBLIT_XYL_t** XYL_pp, U32 *Entries, U32 *Pixels)
{
    STBLIT_XYL_t* XYL_p;
    U32 i;

    if (strcmp(ShapeString, "Tri") == 0)
    {
        /* A simple filled triangle 60 high, base 120 */
        XYL_p = (STBLIT_XYL_t*) malloc(sizeof(STBLIT_XYL_t)*60);
        *XYL_pp = XYL_p;

        *Pixels = 0;
        for (i = 0; i < 60; i++)
        {
            XYL_p->PositionX = X + 120 - (i * 2);
            XYL_p->PositionY = Y + i;
            XYL_p->Length = i * 4;
            *Pixels += XYL_p->Length;

            XYL_p++;
        }

        *Entries = XYL_p - *XYL_pp;
        return TRUE;
    }

    if (strcmp(ShapeString, "Box") == 0)
    {
        /* A 5 pixel wide outline box 250x60 size*/
        XYL_p = (STBLIT_XYL_t*) malloc(sizeof(STBLIT_XYL_t)*120);
        *XYL_pp = XYL_p;

        for (i = 0; i < 5; i++)
        {
            XYL_p->PositionX = X;
            XYL_p->PositionY = Y + i;
            XYL_p->Length = 250;
            XYL_p++;
        }
        for (i = 5; i < 55; i++)
        {
            XYL_p->PositionX = X;
            XYL_p->PositionY = Y + i;
            XYL_p->Length = 5;
            XYL_p++;

            XYL_p->PositionX = X + 245;
            XYL_p->PositionY = Y + i;
            XYL_p->Length = 5;
            XYL_p++;
        }
        for (i = 55; i < 60; i++)
        {
            XYL_p->PositionX = X;
            XYL_p->PositionY = Y + i;
            XYL_p->Length = 250;
            XYL_p++;
        }

        *Entries = XYL_p - *XYL_pp;
        *Pixels = 250*10 + 10*50;
        return TRUE;
    }
    if (strcmp(ShapeString, "Diamond") == 0)
    {
        /* A single pixel outline diamond - fits in a 60x60 square*/
        XYL_p = (STBLIT_XYL_t*) malloc(sizeof(STBLIT_XYL_t)*120);
        *XYL_pp = XYL_p;

        for (i = 0; i < 30; i++)
        {
            XYL_p->PositionX = X + 30 - i;
            XYL_p->PositionY = Y + i;
            XYL_p->Length = 1;
            XYL_p++;

            XYL_p->PositionX = X + 31 + i;
            XYL_p->PositionY = Y + i;
            XYL_p->Length = 1;
            XYL_p++;

            XYL_p->PositionX = X + 30 - i;
            XYL_p->PositionY = Y + 59 - i;
            XYL_p->Length = 1;
            XYL_p++;

            XYL_p->PositionX = X + 31 + i;
            XYL_p->PositionY = Y + 59 - i;
            XYL_p->Length = 1;
            XYL_p++;
        }

        *Entries = XYL_p - *XYL_pp;
        *Pixels = 120;
        return TRUE;
    }

    return FALSE;
}

BOOL surf_check_shape(parse_t *pars_p, char *result_sym_p)
{

    BOOL                RetErr;

    char                Name[MAX_NAME_SIZE];
    char                ShapeStr[MAX_NAME_SIZE];
    char                BgStr[MAX_NAME_SIZE];
    char                FgStr[MAX_NAME_SIZE];

    S32                 X, Y, P1, P2, j;
    U32                 FgCol, BgCol;

    SURF_DESC           *pSurf;

    enum
    {
        Tri,
        Box,
        Diamond
    } Shape;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", Name, sizeof(Name));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", ShapeStr, sizeof(ShapeStr));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &X);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &Y);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", FgStr, sizeof(FgStr));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", BgStr, sizeof(BgStr));
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pSurf = surf_find(Name);
    if (pSurf == NULL)
    {
        STTBX_Print(("Surface not found\n"));
        goto InvalidUsage;
    }

    if (!surf_string_colour(FgStr, &FgCol)) {
        STTBX_Print(("Invalid Foreground colour\n"));
        goto InvalidUsage;
    }

    if (!surf_string_colour(BgStr, &BgCol)) {
        STTBX_Print(("Invalid Background colour\n"));
        goto InvalidUsage;
    }

    if (strcmp(ShapeStr, "Tri") == 0) {
        Shape = Tri;
    }
    else if (strcmp(ShapeStr, "Box") == 0) {
        Shape = Box;
    }
    else if (strcmp(ShapeStr, "Diamond") == 0) {
        Shape = Diamond;
    }

    for (j = 0; j < pSurf->Height; j++) {
        if (j < Y || j >= Y + 60)
        {
            surf_check_span(pSurf, 0, j, pSurf->Width, BgCol);
        }
        else
        {
            switch (Shape)
            {
            case Tri:
                {
                    P1 = X + 120 - ((j-Y) * 2);
                    P2 = X + 120 + ((j-Y) * 2);

                    surf_check_span(pSurf, 0, j, P1, BgCol);
                    surf_check_span(pSurf, P1, j, P2 - P1, FgCol);
                    surf_check_span(pSurf, P2, j, pSurf->Width - P2, BgCol);
                }
                break;
            case Box:
                {
                    if ((j - Y) < 5 || (j - Y) >= 55)
                    {
                        /* Full span */
                        surf_check_span(pSurf, 0, j, X, BgCol);
                        surf_check_span(pSurf, X, j, 250, FgCol);
                        surf_check_span(pSurf, X+250, j, pSurf->Width - (X+250), BgCol);
                    }
                    else
                    {
                        /* 2 5 pixel spans */
                        surf_check_span(pSurf, 0, j, X, BgCol);
                        surf_check_span(pSurf, X, j, 5, FgCol);
                        surf_check_span(pSurf, X+5, j, 240, BgCol);
                        surf_check_span(pSurf, X+245, j, 5, FgCol);
                        surf_check_span(pSurf, X+250, j, pSurf->Width - (X+250), BgCol);
                    }
                }
                break;
            case Diamond:
                {

                    U32 Line = j - Y;
                    if (Line >= 30) Line = 59 - Line;

                    P1 = X + 30 - Line;
                    P2 = X + 30 + Line;

                    /* 2 single pixels on each line. */
                    surf_check_span(pSurf, 0, j, P1, BgCol);
                    surf_check_span(pSurf, P1, j, 1, FgCol);
                    surf_check_span(pSurf, P1+1, j, P2 - (P1+1), BgCol);
                    surf_check_span(pSurf, P2, j, 1, FgCol);
                    surf_check_span(pSurf, P2+1, j, pSurf->Width - (P2+1), BgCol);

                }
                break;
            }
        }
    }
    return FALSE;

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_CHECK_SHAPE <Surface> <Shape> <X> <Y> <Fg Color> <Bg Color>\n"));
    return TRUE;

}



/*-----------------------------------------------------------------------------
 * Function : surf_create;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL surf_create (parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr;

    char    Name[MAX_NAME_SIZE];
    char    Format[MAX_FORMAT_SIZE];
    char    Pattern[MAX_PATTERN_SIZE];

    S32     Width;
    S32     Height;
    S32     Pitch;
    STGXOBJ_ColorType_t Type;
    STAVMEM_BlockHandle_t       BlockHandle;

    SURF_DESC   *pSurf;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", Name, sizeof(Name));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", Format, sizeof(Format));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &Width);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &Height);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "Black", Pattern, sizeof(Pattern));
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pSurf = surf_find(Name);
    if (pSurf)
    {
        STTBX_Print(("Surface already exists\n"));
        return FALSE;
    }

    if (surf_parse_format(Format, &Type))
    {
        STTBX_Print(("Invalid Surface Format\n"));
        goto InvalidUsage;
    }

    /* Work out byte pitch, align to DWORD. */
    Pitch = (Width * (surf_get_type_bpp(Type) >> 3) + 3) & ~3;

    /* Allocate some memory */
    {
        STAVMEM_AllocBlockParams_t  AllocParams;
        AllocParams.PartitionHandle = AvmemPartitionHandle[0];
        AllocParams.Size            = Pitch * Height;
        AllocParams.Alignment       = 4;
        AllocParams.AllocMode       = STAVMEM_ALLOC_MODE_TOP_BOTTOM;

        AllocParams.NumberOfForbiddenRanges = 0;
        AllocParams.ForbiddenRangeArray_p = NULL;
        AllocParams.NumberOfForbiddenBorders = 0;
        AllocParams.ForbiddenBorderArray_p = NULL;

        if (STAVMEM_AllocBlock(&AllocParams, &BlockHandle) != ST_NO_ERROR)
        {
            STTBX_Print(("Unable to allocate memory\n"));
            goto InvalidUsage;
        }
    }

    pSurf = (SURF_DESC *)malloc(sizeof(SURF_DESC));

    strncpy(pSurf->Name, Name, MAX_NAME_SIZE);
    pSurf->Type = Type;
    pSurf->Width = Width;
    pSurf->Height = Height;
    pSurf->Pitch = Pitch;
    pSurf->BlockHandle = BlockHandle;

    /* Add surface into list */
    pSurf->pNext = pFirstSurface;
    pFirstSurface = pSurf;

    /* Fill out GXOBJ fields */
    pSurf->Bitmap.ColorType             = Type;
    pSurf->Bitmap.BitmapType            = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    pSurf->Bitmap.PreMultipliedColor    = FALSE;
    pSurf->Bitmap.ColorSpaceConversion  = STGXOBJ_ITU_R_BT601;
    pSurf->Bitmap.AspectRatio           = STGXOBJ_ASPECT_RATIO_SQUARE;
    pSurf->Bitmap.Width                 = Width;
    pSurf->Bitmap.Height                = Height;
    pSurf->Bitmap.Pitch                 = Pitch;
    pSurf->Bitmap.Offset                = 0;
    pSurf->Bitmap.Size1                 = 0;
    pSurf->Bitmap.SubByteFormat         = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
    STAVMEM_GetBlockAddress(pSurf->BlockHandle, (void **)&pSurf->Bitmap.Data1_p);

    surf_fill_pattern(pSurf, 0, 0, Width, Height, Pattern);

    return FALSE;

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_CREATE <Name> <Format> <Width> <Height> <Pattern>\n"));
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Function : surf_delete;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL surf_delete (parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr;

    char        Name[MAX_NAME_SIZE];
    SURF_DESC   *pSurf;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", Name, sizeof(Name));
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pSurf = surf_find(Name);
    if (!pSurf)
    {
        STTBX_Print(("Surface doesnt exist\n"));
        return TRUE;
    }

    /* Unlink from surface list */
    if (pFirstSurface == pSurf) {
        pFirstSurface = pSurf->pNext;
    }
    else {
        SURF_DESC *pPrevSurf = pFirstSurface;
        while (pPrevSurf)
        {
            if (pPrevSurf->pNext == pSurf)
            {
                pPrevSurf->pNext = pSurf->pNext;
                break;
            }
        }
    }

    /* Free up allocated bitmap memory */
    {
        STAVMEM_FreeBlockParams_t FreeParams;

        FreeParams.PartitionHandle = AvmemPartitionHandle[0];
        STAVMEM_FreeBlock(&FreeParams, &pSurf->BlockHandle);
    }

    /* Free structure */
    free(pSurf);
    return FALSE;

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_DELETE <Name>\n"));
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Function : surf_fill;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL surf_fill (parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr;

    char    Name[MAX_NAME_SIZE];
    char    Pattern[MAX_PATTERN_SIZE];

    S32     X, Y;
    S32     W, H;

    SURF_DESC   *pSurf;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", Name, sizeof(Name));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &X);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &Y);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &W);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &H);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", Pattern, sizeof(Pattern));
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pSurf = surf_find(Name);
    if (pSurf == NULL)
    {
        STTBX_Print(("Surface not found\n"));
        goto InvalidUsage;
    }

    surf_fill_pattern(pSurf, X, Y, W, H, Pattern);
    return FALSE;

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_FILL_PATTERN <Name> <X> <Y> <Width> <Height> <Pattern>\n"));
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Function : surf_check_inside;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL surf_check_inside (parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr;

    char    Name[MAX_NAME_SIZE];
    char    Pattern[MAX_PATTERN_SIZE];

    S32     X, Y;
    S32     W, H;
    S32     OX, OY;

    SURF_DESC   *pSurf;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", Name, sizeof(Name));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &X);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &Y);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &W);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &H);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", Pattern, sizeof(Pattern));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &OX);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &OY);
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pSurf = surf_find(Name);
    if (pSurf == NULL)
    {
        STTBX_Print(("Surface not found\n"));
        goto InvalidUsage;
    }

    return surf_check_pattern(pSurf, X, Y, W, H, Pattern, OX, OY);

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_CHECK_INSIDE <Name> <X> <Y> <Width> <Height> <Pattern> <PatOriginX> <PatOriginY>\n"));
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Function : surf_check_outside;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL surf_check_outside (parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr;

    char    Name[MAX_NAME_SIZE];
    char    Pattern[MAX_PATTERN_SIZE];

    S32     X, Y;
    S32     W, H;
    S32     OX, OY;

    SURF_DESC   *pSurf;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", Name, sizeof(Name));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &X);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &Y);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &W);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &H);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", Pattern, sizeof(Pattern));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &OX);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &OY);
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pSurf = surf_find(Name);
    if (pSurf == NULL)
    {
        STTBX_Print(("Surface not found\n"));
        goto InvalidUsage;
    }

    /* Check the 4 regions outside the rectangle */
    RetErr  = surf_check_pattern(pSurf, 0, 0, X, pSurf->Height, Pattern, OX, OY);
    RetErr |= surf_check_pattern(pSurf, X + W, 0, pSurf->Width - X - W, pSurf->Height, Pattern, OX, OY);
    RetErr |= surf_check_pattern(pSurf, X, 0, W, Y, Pattern, OX, OY);
    RetErr |= surf_check_pattern(pSurf, X, Y + H, W, pSurf->Height - Y - H, Pattern, OX, OY);

    return RetErr;

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_CHECK_OUTSIDE <Name> <X> <Y> <Width> <Height> <Pattern> <PatOriginX> <PatOriginY>\n"));
    return TRUE;
}


/*-----------------------------------------------------------------------------
 * Function : STBLIT_FillRectangle;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL surf_STBLIT_FillRect (parse_t *pars_p, char *result_sym_p)
{

    BOOL                RetErr;

    char                Name[MAX_NAME_SIZE];
    char                ColourString[MAX_NAME_SIZE];

    S32                 X, Y;
    S32                 W, H;
    U32                 ColourRGB;

    ST_ErrorCode_t      ErrCode;
    SURF_DESC           *pSurf;

    S32                 Lvar;
    STBLIT_Handle_t     Handle;

    STGXOBJ_Color_t     Colour;
    STGXOBJ_Rectangle_t Rect;

    /* Get the Device Handle */
    RetErr = STTST_GetInteger( pars_p, -1, &Lvar);
    if ( RetErr == TRUE )
    {
            tag_current_line( pars_p, "expected Handle" );
    }
    Handle = (STBLIT_Handle_t)Lvar;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", Name, sizeof(Name));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &X);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &Y);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &W);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &H);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", ColourString, sizeof(ColourString));
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pSurf = surf_find(Name);
    if (pSurf == NULL)
    {
        STTBX_Print(("Surface not found\n"));
        goto InvalidUsage;
    }

    if (!surf_string_colour(ColourString, &ColourRGB)) {
        STTBX_Print(("Invalid solid colour\n"));
        goto InvalidUsage;
    }

    Colour.Type = pSurf->Type;
    surf_generate_gxobj_colour(&Colour, ColourRGB);

    Rect.PositionX  = X;
    Rect.PositionY  = Y;
    Rect.Width      = W;
    Rect.Height     = H;

    ErrCode = STBLIT_FillRectangle( Handle, &(pSurf->Bitmap), &Rect, &Colour, &ContextGen ) ;
    if (ErrCode != ST_NO_ERROR)
    {
        surf_print_error("STBLIT_FillRectangle", ErrCode);
    }

    return FALSE;

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_STBLIT_FILLRECT <Handle> <Surface> <X> <Y> <Width> <Height> <Color>\n"));
    return TRUE;

}


/*-----------------------------------------------------------------------------
 * Function : surf_STBLIT_CopyRect;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL surf_STBLIT_CopyRect (parse_t *pars_p, char *result_sym_p)
{

    BOOL                RetErr;

    char                DstName[MAX_NAME_SIZE], SrcName[MAX_NAME_SIZE];
    char                ColourString[MAX_NAME_SIZE];

    S32                 DstX, DstY, SrcX, SrcY;
    S32                 W, H;

    ST_ErrorCode_t      ErrCode;
    SURF_DESC           *pDstSurf, *pSrcSurf;

    S32                 Lvar;
    STBLIT_Handle_t     Handle;
    STGXOBJ_Rectangle_t Rect;

    /* Get the Device Handle */
    RetErr = STTST_GetInteger( pars_p, -1, &Lvar);
    if ( RetErr == TRUE )
    {
            tag_current_line( pars_p, "expected Handle" );
    }
    Handle = (STBLIT_Handle_t)Lvar;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", DstName, sizeof(DstName));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &DstX);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &DstY);
    if (RetErr) goto InvalidUsage;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", SrcName, sizeof(SrcName));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &SrcX);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &SrcY);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &W);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &H);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", ColourString, sizeof(ColourString));
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pDstSurf = surf_find(DstName);
    if (pDstSurf == NULL)
    {
        STTBX_Print(("Surface not found\n"));
        goto InvalidUsage;
    }

    /* Check to see if suface name is in use */
    pSrcSurf = surf_find(SrcName);
    if (pSrcSurf == NULL)
    {
        STTBX_Print(("Surface not found\n"));
        goto InvalidUsage;
    }

    Rect.PositionX  = SrcX;
    Rect.PositionY  = SrcY;
    Rect.Width      = W;
    Rect.Height     = H;

    ErrCode = STBLIT_CopyRectangle( Handle, &(pSrcSurf->Bitmap), &Rect, &(pDstSurf->Bitmap), DstX, DstY, &ContextGen) ;
    if (ErrCode != ST_NO_ERROR)
    {
        surf_print_error("STBLIT_CopyRectangle", ErrCode);
    }

    return FALSE;

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_STBLIT_COPYRECT <Handle> <Surface> <X> <Y> <Width> <Height> <Color>\n"));
    return TRUE;

}



void surf_benchmark_fillrect_align(STBLIT_Handle_t Handle, SURF_DESC *pDstSurf, U32 W, U32 H, U32 Iterations)
{
    STGXOBJ_Color_t     Colour;
    STGXOBJ_Rectangle_t Rect;
    ST_ErrorCode_t      ErrCode;

    U32 i, j;

    STOS_Clock_t        Start, End;
    int                 PPS;


    if (pDstSurf->Width < W || pDstSurf->Height < H)
    {
        STTBX_Print(("surf_benchmark_fillrect_align: Needs larger destination surface\n"));
    }

    /* 8 passes */
    Iterations = Iterations/8;

    Colour.Type = pDstSurf->Type;
    surf_generate_gxobj_colour(&Colour, 0);

    Rect.PositionX  = 0;
    Rect.PositionY  = 0;
    Rect.Width      = W;
    Rect.Height     = H;

    STTBX_Print(("FillRectAlign %d iterations %dx%d\n", Iterations, W, H));
    STTBX_Print(("      0      1      2      3      4      5      6      7\n", Iterations));

    for (j = 0; j < 8; j++) {
        Start = STOS_time_now();
        for (i = 0; i < Iterations; i++) {
            Rect.PositionX = 0;
            Rect.Width     = W+j;

            Rect.PositionY += 5;
            if (Rect.PositionY + W > pDstSurf->Height)
                Rect.PositionY = 0;

            /* Some kind of colour change */
            *(int*)&(Colour.Value) += 0x87654321;

            if ( i == Iterations-1) {
                ContextGen.NotifyBlitCompletion     = TRUE;
                ContextGen.EventSubscriberID        = SubscrId;

            }

            ErrCode = STBLIT_FillRectangle( Handle, &(pDstSurf->Bitmap), &Rect, &Colour, &ContextGen ) ;
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("Iteration %d\n", i));
                surf_print_error("STBLIT_FillRectangle", ErrCode);
                break;
            }
        }

        ContextGen.NotifyBlitCompletion = FALSE;
        STOS_SemaphoreWait(BlitCompleteSem);

        End = STOS_time_now();

        PPS = (int)(((double)W * (double)H * (double)i * 100) / ((double)time_minus(End, Start) * 64));

        STTBX_Print(("%4d.%02d", PPS / 100, PPS % 100));

    }
    STTBX_Print(("\n\n"));
}

void surf_benchmark_fillrect(STBLIT_Handle_t Handle, SURF_DESC *pDstSurf, U32 W, U32 H, U32 Iterations)
{
    STGXOBJ_Color_t     Colour;
    STGXOBJ_Rectangle_t Rect;
    ST_ErrorCode_t      ErrCode;

    U32                 i;
    STOS_Clock_t        Start, End;
    int                 PPS;

    if (pDstSurf->Width < W || pDstSurf->Height < H)
    {
        STTBX_Print(("surf_benchmark_fillrect: Needs larger destination surface\n"));
    }


    Colour.Type = pDstSurf->Type;
    surf_generate_gxobj_colour(&Colour, 0);

    Rect.PositionX  = 0;
    Rect.PositionY  = 0;
    Rect.Width      = W;
    Rect.Height     = H;

    Start = STOS_time_now();

    for (i = 0; i < Iterations; i++) {
        Rect.PositionX++;
        if (Rect.PositionX + W > pDstSurf->Width)
            Rect.PositionX = 0;

        Rect.PositionY += 5;
        if (Rect.PositionY + W > pDstSurf->Height)
            Rect.PositionY = 0;

        /* Some kind of colour change */
        *(int*)&(Colour.Value) += 0x87654321;

        if ( i == Iterations-1) {
            ContextGen.NotifyBlitCompletion     = TRUE;
            ContextGen.EventSubscriberID        = SubscrId;

        }

        ErrCode = STBLIT_FillRectangle( Handle, &(pDstSurf->Bitmap), &Rect, &Colour, &ContextGen ) ;
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("Iteration %d\n", i));
            surf_print_error("STBLIT_FillRectangle", ErrCode);
            break;
        }
    }

    ContextGen.NotifyBlitCompletion = FALSE;
    STOS_SemaphoreWait(BlitCompleteSem);

    End = STOS_time_now();

    PPS = (int)(((double)W * (double)H * (double)i * 1000) / ((double)time_minus(End, Start) * 64));

    STTBX_Print(("FillRect %d iterations %4dx%4d %d.%03d MP/S (%d ticks)\n", i, W, H, PPS / 1000, PPS % 1000, time_minus(End, Start)));
}

void surf_benchmark_copyrect(STBLIT_Handle_t Handle, SURF_DESC *pDstSurf, SURF_DESC *pSrcSurf, U32 W, U32 H, U32 Iterations)
{
    STGXOBJ_Color_t     Colour;
    STGXOBJ_Rectangle_t Rect;
    ST_ErrorCode_t      ErrCode;

    U32                 i, DestX, DestY;
    STOS_Clock_t        Start, End;
    int                 PPS;

    /* The excess size required is used later in the test to vary positions */
    if (pDstSurf->Width < W+64 || pDstSurf->Height < H+32)
    {
        STTBX_Print(("surf_benchmark_copyrect: Needs larger destination surface\n"));
    }

    if (pSrcSurf->Width < W+64 || pSrcSurf->Height < H+32)
    {
        STTBX_Print(("surf_benchmark_copyrect: Needs larger source surface\n"));
    }


    Colour.Type = pDstSurf->Type;
    surf_generate_gxobj_colour(&Colour, 0);

    Rect.PositionX  = 0;
    Rect.PositionY  = 0;
    Rect.Width      = W;
    Rect.Height     = H;

    Start = STOS_time_now();

    for (i = 0; i < Iterations; i++) {
        Rect.PositionX++;
        if (Rect.PositionX + W > pDstSurf->Width)
            Rect.PositionX = 0;

        Rect.PositionY += 5;
        if (Rect.PositionY + W > pDstSurf->Height)
            Rect.PositionY = 0;

        /* Some varying destination positions - just use a few bits of
           the source position */
        DestX = (Rect.PositionX >> 3) & 63;
        DestY = (Rect.PositionY >> 3) & 31;

        /* Some kind of colour change */
        *(int*)&(Colour.Value) += 0x87654321;

        if ( i == Iterations-1) {
            ContextGen.NotifyBlitCompletion     = TRUE;
            ContextGen.EventSubscriberID        = SubscrId;

        }

        ErrCode = STBLIT_CopyRectangle( Handle, &(pSrcSurf->Bitmap), &Rect, &(pDstSurf->Bitmap), DestX, DestY, &ContextGen ) ;
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("Iteration %d\n", i));
            surf_print_error("STBLIT_FillRectangle", ErrCode);
            break;
        }
    }

    ContextGen.NotifyBlitCompletion = FALSE;
    STOS_SemaphoreWait(BlitCompleteSem);

    End = STOS_time_now();

    PPS = (int)(((double)W * (double)H * (double)i * 1000) / ((double)time_minus(End, Start) * 64));

    STTBX_Print(("CopyRect%s %d iterations %4dx%4d %d.%03d MP/S (%d ticks)\n",
        pSrcSurf == pDstSurf ? "Overlap" : "",
        i, W, H, PPS / 1000, PPS % 1000, time_minus(End, Start)));
}

void surf_benchmark_copyrect_align(STBLIT_Handle_t Handle, SURF_DESC *pDstSurf, SURF_DESC *pSrcSurf, U32 W, U32 H, U32 Iterations)
{
    STGXOBJ_Color_t     Colour;
    STGXOBJ_Rectangle_t Rect;
    ST_ErrorCode_t      ErrCode;

    U32                 i, j, k, DestX, DestY;
    STOS_Clock_t        Start, End;
    int                 PPS;

    /* The excess size required is used later in the test to vary positions */
    if (pDstSurf->Width < W+64 || pDstSurf->Height < H+32)
    {
        STTBX_Print(("surf_benchmark_copyrect: Needs larger destination surface\n"));
    }

    if (pSrcSurf->Width < W+64 || pSrcSurf->Height < H+32)
    {
        STTBX_Print(("surf_benchmark_copyrect: Needs larger source surface\n"));
    }


    Colour.Type = pDstSurf->Type;
    surf_generate_gxobj_colour(&Colour, 0);

    STTBX_Print(("CopyRect Align %d Iterations  %dx%d\n", Iterations, W, H));
    STTBX_Print(("            -4     -3     -2     -1     -0     -1     -2     -3     -4\n"));
    for (k = 0; k < 2; k++)
    {

        STTBX_Print(("%7s", pSrcSurf == pDstSurf ? "Overlap" : "Normal"));

        /* First pass is non overlapped copies */
        for (j = 0; j < 9; j++)
        {
            Rect.PositionX  = 16;
            Rect.PositionY  = 0;
            Rect.Width      = W;
            Rect.Height     = H;

            DestX = 12 + j;
            DestY = 0;

            if (pSrcSurf == pDstSurf && Rect.PositionX == DestX)
            {
                /* Dont do the overlaped 0 pixel blit (a noop!) */
                STTBX_Print(("       "));
                continue;
            }


            Start = STOS_time_now();

            for (i = 0; i < Iterations; i++) {
                /* Some kind of colour change */
                *(int*)&(Colour.Value) += 0x87654321;

                if ( i == Iterations-1) {
                    ContextGen.NotifyBlitCompletion     = TRUE;
                    ContextGen.EventSubscriberID        = SubscrId;

                }

                ErrCode = STBLIT_CopyRectangle( Handle, &(pSrcSurf->Bitmap), &Rect, &(pDstSurf->Bitmap), DestX, DestY, &ContextGen ) ;
                if (ErrCode != ST_NO_ERROR)
                {
                    STTBX_Print(("Iteration %d\n", i));
                    surf_print_error("STBLIT_FillRectangle", ErrCode);
                    break;
                }
            }

            ContextGen.NotifyBlitCompletion = FALSE;
            STOS_SemaphoreWait(BlitCompleteSem);

            End = STOS_time_now();

            PPS = (int)(((double)W * (double)H * (double)i * 100) / ((double)time_minus(End, Start) * 64));

            STTBX_Print(("%4d.%02d", PPS / 100, PPS % 100));
        }
        STTBX_Print(("\n"));

        /* Second pass is overlapped copies */
        pSrcSurf = pDstSurf;
    }
    STTBX_Print(("\n"));
}

void surf_benchmark_xyl(STBLIT_Handle_t Handle, SURF_DESC *pDstSurf, char *Shape, U32 Iterations)
{
    STGXOBJ_Color_t     Colour;
    ST_ErrorCode_t      ErrCode;

    U32                 i;
    STOS_Clock_t        Start, End;
    int                 PPS;

    STBLIT_XYL_t*       XYL_p;
    U32                 Entries, Pixels;

    Colour.Type = pDstSurf->Type;
    surf_generate_gxobj_colour(&Colour, 0);

    surf_string_shape_xyl(Shape, 0, 0, &XYL_p, &Entries, &Pixels);

    Start = STOS_time_now();

    for (i = 0; i < Iterations; i++) {
        /* Some kind of colour change */
        *(int*)&(Colour.Value) += 0x87654321;

        if ( i == Iterations-1) {
            ContextGen.NotifyBlitCompletion     = TRUE;
            ContextGen.EventSubscriberID        = SubscrId;

        }

        ErrCode = STBLIT_DrawXYLArray( Handle, &(pDstSurf->Bitmap), XYL_p, Entries, &Colour, &ContextGen ) ;
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("Iteration %d\n", i));
            surf_print_error("STBLIT_DrawXYLArray", ErrCode);
            break;
        }
    }

    ContextGen.NotifyBlitCompletion = FALSE;
    STOS_SemaphoreWait(BlitCompleteSem);

    End = STOS_time_now();

    PPS = (int)(((double)Pixels * (double)i * 1000) / ((double)time_minus(End, Start) * 64));

    STTBX_Print(("XYL %d iterations %s %d.%03d MP/S (%d ticks)\n", i, Shape, PPS / 1000, PPS % 1000, time_minus(End, Start)));

    free(XYL_p);
}

/*-----------------------------------------------------------------------------
 * Function : surf_benchmark;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL surf_benchmark (parse_t *pars_p, char *result_sym_p)
{

    BOOL                RetErr;
    BOOL                All = FALSE;

    char                SrcName[MAX_NAME_SIZE];
    char                DstName[MAX_NAME_SIZE];
    char                TestName[MAX_TEST_NAME_SIZE];

    S32                 Iterations;

    SURF_DESC           *pSrcSurf, *pDstSurf;

    S32                 Lvar;
    STBLIT_Handle_t     Handle;

    surf_open_evt();

    /* Get the Device Handle */
    RetErr = STTST_GetInteger( pars_p, -1, &Lvar);
    if ( RetErr == TRUE )
    {
            tag_current_line( pars_p, "expected Handle" );
    }
    Handle = (STBLIT_Handle_t)Lvar;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "dst", DstName, sizeof(DstName));
    if (RetErr) goto InvalidUsage;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, DstName, SrcName, sizeof(SrcName));
    if (RetErr) goto InvalidUsage;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "all", TestName, sizeof(TestName));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 100, &Iterations);
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pDstSurf = surf_find(DstName);
    if (pDstSurf == NULL)
    {
        STTBX_Print(("Dest Surface not found\n"));
        goto InvalidUsage;
    }

    /* Check to see if suface name is in use */
    pSrcSurf = surf_find(SrcName);
    if (pSrcSurf == NULL)
    {
        STTBX_Print(("Source Surface not found\n"));
        goto InvalidUsage;
    }

    if (strcmp(TestName, "all") == 0)
        All = TRUE;

    if (All || strncmp(TestName, "fillrect", 8) == 0)
    {
        if (!All && TestName[8] == ':')
        {
            S32 W, H;
            sscanf(TestName + 9, "%dx%d", &W, &H);
            if (W < 0 || H < 0 || W > pDstSurf->Width || H > pDstSurf->Height)
            {
                STTBX_Print(("Invalid Width and Height specified\n"));
                goto InvalidUsage;
            }
            surf_benchmark_fillrect(Handle, pDstSurf, W,      H,      Iterations);
        }
        else
        {
            surf_benchmark_fillrect(Handle, pDstSurf, 1,      1,      Iterations);
            surf_benchmark_fillrect(Handle, pDstSurf, 16,     16,     Iterations);
            surf_benchmark_fillrect(Handle, pDstSurf, 64,     64,     Iterations);
            surf_benchmark_fillrect(Handle, pDstSurf, 128,    128,    Iterations);
            surf_benchmark_fillrect(Handle, pDstSurf, 256,    256,    Iterations);

            surf_benchmark_fillrect_align(Handle, pDstSurf,   256,    256,    Iterations);
        }
    }

    if (All || strncmp(TestName, "copyrect", 8) == 0)
    {
        if (!All && TestName[8] == ':')
        {
            S32 W, H;
            sscanf(TestName + 9, "%dx%d", &W, &H);
            if (W < 0 || H < 0 || W > pDstSurf->Width || H > pDstSurf->Height)
            {
                STTBX_Print(("Invalid Width and Height specified\n"));
                goto InvalidUsage;
            }
            surf_benchmark_copyrect(Handle, pDstSurf, pSrcSurf, W,      H,      Iterations);
        }
        else
        {
            surf_benchmark_copyrect(Handle, pDstSurf, pSrcSurf, 1,      1,      Iterations);
            surf_benchmark_copyrect(Handle, pDstSurf, pSrcSurf, 16,     16,     Iterations);
            surf_benchmark_copyrect(Handle, pDstSurf, pSrcSurf, 64,     64,     Iterations);
            surf_benchmark_copyrect(Handle, pDstSurf, pSrcSurf, 128,    128,    Iterations);
            surf_benchmark_copyrect(Handle, pDstSurf, pSrcSurf, 256,    256,    Iterations);

            surf_benchmark_copyrect(Handle, pDstSurf, pDstSurf, 128,    128,    Iterations);
            surf_benchmark_copyrect(Handle, pDstSurf, pDstSurf, 256,    256,    Iterations);

            surf_benchmark_copyrect_align(Handle, pDstSurf, pSrcSurf, 256,    256,    Iterations);
        }
    }

    if (All || strncmp(TestName, "xyl", 3) == 0)
    {
        surf_benchmark_xyl(Handle, pDstSurf, "Tri", Iterations);
        surf_benchmark_xyl(Handle, pDstSurf, "Box", Iterations);
        surf_benchmark_xyl(Handle, pDstSurf, "Diamond", Iterations);
    }

    surf_close_evt();

    return FALSE;

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_BENCHMARK <Handle> <Dest Surf> <Source Surf> <Test> <Iterations>\n"));
    return TRUE;

}


BOOL surf_STBLIT_XYL (parse_t *pars_p, char *result_sym_p)
{

    BOOL                RetErr;

    char                Name[MAX_NAME_SIZE];
    char                ColourString[MAX_NAME_SIZE];
    char                ShapeString[MAX_NAME_SIZE];

    S32                 X, Y;
    U32                 ColourRGB, Entries, Pixels;

    ST_ErrorCode_t      ErrCode;
    SURF_DESC           *pSurf;

    S32                 Lvar;
    STBLIT_Handle_t     Handle;

    STGXOBJ_Color_t     Colour;

    STBLIT_XYL_t*       XYL_p;

    /* Get the Device Handle */
    RetErr = STTST_GetInteger( pars_p, -1, &Lvar);
    if ( RetErr == TRUE )
    {
            tag_current_line( pars_p, "expected Handle" );
    }
    Handle = (STBLIT_Handle_t)Lvar;

    /* Get the parameters */
    RetErr = STTST_GetString( pars_p, "", Name, sizeof(Name));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", ShapeString, sizeof(ShapeString));
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &X);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetInteger( pars_p, 0, &Y);
    if (RetErr) goto InvalidUsage;

    RetErr = STTST_GetString( pars_p, "", ColourString, sizeof(ColourString));
    if (RetErr) goto InvalidUsage;

    /* Check to see if suface name is in use */
    pSurf = surf_find(Name);
    if (pSurf == NULL)
    {
        STTBX_Print(("Surface not found\n"));
        goto InvalidUsage;
    }

    if (!surf_string_shape_xyl(ShapeString, X, Y, &XYL_p, &Entries, &Pixels)) {
        STTBX_Print(("Invalid shape colour\n"));
        goto InvalidUsage;
    }

    if (!surf_string_colour(ColourString, &ColourRGB)) {
        STTBX_Print(("Invalid solid colour\n"));
        goto InvalidUsage;
    }

    Colour.Type = pSurf->Type;
    surf_generate_gxobj_colour(&Colour, ColourRGB);


    ErrCode = STBLIT_DrawXYLArray( Handle, &(pSurf->Bitmap), XYL_p, Entries, &Colour, &ContextGen ) ;
    if (ErrCode != ST_NO_ERROR)
    {
        surf_print_error("STBLIT_DrawXYLArray", ErrCode);
    }

    /* Cleanup */
    free(XYL_p);

    return FALSE;

InvalidUsage:
    STTBX_Print(("\nUsage:\n"));
    STTBX_Print(("    SURF_STBLIT_XYL <Handle> <Surface> <Shape> <X> <Y> <Color>\n"));
    return TRUE;

}
#endif



