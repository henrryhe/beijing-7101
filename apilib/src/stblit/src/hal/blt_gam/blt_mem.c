/*******************************************************************************

File name    : blt_mem.c

Description :  write nodes in shared memory

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
16 Jun 2000        Created                                           TM
29 May 2006        Modified for WinCE platform						WinCE Team-ST Noida
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stblit.h"
#include "blt_com.h"
#include "blt_mem.h"
#include "blt_fe.h"
#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#include "stavmem.h"
#endif
#include "stgxobj.h"
#include "blt_pool.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Constants used by GetFilterMode functions
 * Their values are the same one as the horizontal/vertical one */
#define STBLIT_2DFILTER_MODE_MASK            3
#define STBLIT_2DFILTER_INACTIVE             0
#define STBLIT_2DFILTER_ACTIV_COLOR          1
#define STBLIT_2DFILTER_ACTIV_ALPHA          2
#define STBLIT_2DFILTER_ACTIV_BOTH           3

#ifdef ST_OSLINUX
#ifdef MODULE

#define PrintTrace(x)       STTBX_Print(x)

#else   /* MODULE */

#define PrintTrace(x)		printf x

#endif  /* MODULE */
#endif

/* Private Variables (static)------------------------------------------------ */

/* Table which gives the configuration of Vertical and Horizontal scan directions
 * according to an Overlap Code
 *
 *   Overlap   | Description  | Configuration
 *   Code      |              |   Hardware
 *  -----------------------------------------
 *    0000     | NotSupported | 0x80
 *    0001     | Down Right   | 0
 *    0010     | Down Left    | 1
 *    0011     | Down Right   | 0
 *    0100     | Up Right     | 2
 *    0101     | Down Right   | 0
 *    0110     | Up Right     | 2
 *    0111     | Down Right   | 0
 *    1000     | Up Left      | 3
 *    1001     | Down Right   | 0
 *    1010     | Up Left      | 3
 *    1011     | Down Right   | 0
 *    1100     | Up Left      | 3
 *    1101     | Down Right   | 0
 *    1110     | Up Left      | 3
 *    1111     | Down Right   | 0
 *
 * In most of the case there are several possibilities : Only one is chosen
 *
 * */
static const U8 ScanDirectionTable[16]  =
{
    0x80, 0, 1, 0, 2, 0, 2, 0, 3 ,0 ,3 ,0 ,3 ,0 ,3 ,0
};


/* Global Variables --------------------------------------------------------- */
#ifdef STBLIT_BENCHMARK3
extern STOS_Clock_t t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15;
extern STOS_Clock_t t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28 ;
extern STOS_Clock_t t29, t30, t31, t32, t33, t34, t35, t36;
#endif

#ifdef STBLIT_BENCHMARK2
extern STOS_Clock_t t1, t2;
#endif

/* Private Macros ----------------------------------------------------------- */
#define AbsMinus(a, b) ((((U32) a) > ((U32) b)) ? (((U32) a) - ((U32) b)) : (((U32) b) - ((U32) a)))
#define MemoryInterlaced(FirstAddr1, LastAddr1, FirstAddr2, LastAddr2) ((((U32) (FirstAddr1)) <= ((U32) (LastAddr2))) && (((U32) (LastAddr1)) >= ((U32) (FirstAddr2))))
#define MemoryRangeWithinRange(FirstAddr1, LastAddr1, FirstAddr2, LastAddr2) ((((U32) (FirstAddr1)) >= ((U32) (FirstAddr2))) && (((U32) (LastAddr1)) <= ((U32) (LastAddr2))))
#define MemoryIdentical(FirstAddr1, LastAddr1, FirstAddr2, LastAddr2) ((((U32) (FirstAddr1)) == ((U32) (FirstAddr2))) && (((U32) (LastAddr1)) == ((U32) (LastAddr2))))
/*
#define Spatial2DOverlap(FirstAddr1, FirstAddr2, Width12, Height12, Pitch12) ( \
 (MemoryInterlaced(((U32) FirstAddr1), ((U32) FirstAddr1) + (Width12) - 1 + ((Pitch12) * ((Height12) - 1)), ((U32) FirstAddr2), ((U32) FirstAddr2) + (Width12) - 1 + ((Pitch12) * ((Height12) - 1)))) && \
 ((((AbsMinus((FirstAddr1), (FirstAddr2)) + (Pitch12)) % (Pitch12)) < (Width12)) || \
  (((AbsMinus((FirstAddr1), (FirstAddr2)) + (Pitch12)) % (Pitch12)) > ((Pitch12) - (Width12)))) \
)
#define UpLeft (FirstAddr1, FirstAddr2, Width, Pitch) ( \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) % (Pitch)) < (Width)) && \
  ((FirstAddr1) < (FirstAddr2)) && \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) / (Pitch)) != 0) \
 )
#define DownRight (FirstAddr1, FirstAddr2, Width, Pitch) ( \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) % (Pitch)) < (Width)) && \
  ((FirstAddr1) > (FirstAddr2)) && \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) / (Pitch)) != 0) \
 )
#define UpRight (FirstAddr1, FirstAddr2, Width, Pitch) ( \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) % (Pitch)) < ((Pitch) - (Width))) && \
  ((FirstAddr1) < (FirstAddr2))\
 )
#define DownLeft (FirstAddr1, FirstAddr2, Width, Pitch) ( \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) % (Pitch)) < ((Pitch) - (Width))) && \
  ((FirstAddr1) > (FirstAddr2))\
 )
#define ImmediateLeft (FirstAddr1, FirstAddr2, Width, Pitch) ( \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) % (Pitch)) < (Width)) && \
  ((FirstAddr1) < (FirstAddr2)) && \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) / (Pitch)) == 0) \
 )
#define ImmediateRight (FirstAddr1, FirstAddr2, Width, Pitch) ( \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) % (Pitch)) < (Width)) && \
  ((FirstAddr1) > (FirstAddr2)) && \
  (((AbsMinus((FirstAddr1), (FirstAddr2))) / (Pitch)) == 0) \
 )

          */

/* Private Function prototypes ---------------------------------------------- */
__inline static ST_ErrorCode_t SelectFilter (U32 Increment,U32* Offset_p);
__inline static ST_ErrorCode_t GetFilterMode (STGXOBJ_ColorType_t ColorType, U8 ClutEnable, U8 ClutMode, BOOL LogicalMaskSecondPass, U8* FilterMode);
/*__inline static U8 OverlapOneSourceAddress(U32 SrcStart, U32 SrcStop, U32 DstStart, U32 DstStop, U32 Pitch, U32 SrcWidth, U32 DstWidth);*/
__inline static U8 OverlapOneSourceCoordinate(U32 SrcXStart, U32 SrcYStart,U32 SrcXStop,U32 SrcYStop,
                                              U32 DstXStart, U32 DstYStart,U32 DstXStop,U32 DstYStop);
__inline static ST_ErrorCode_t FormatColorType (STGXOBJ_ColorType_t TypeIn, U32* TypeOut_p,U8* AlphaRange_p,STGXOBJ_BitmapType_t Mode);
__inline static ST_ErrorCode_t InitNode(stblit_Node_t* Node_p, STBLIT_BlitContext_t*  BlitContext_p);
__inline static ST_ErrorCode_t SetNodeIT(stblit_Node_t* Node_p, STBLIT_BlitContext_t* BlitContext_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc1Color(stblit_Node_t* Node_p,STGXOBJ_Color_t* Color_p,stblit_Device_t* Device_p,stblit_Source1Mode_t Mode,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc1Bitmap(stblit_Node_t* Node_p,STGXOBJ_Bitmap_t* Bitmap_p,STGXOBJ_Rectangle_t* Rectangle_p,U8 ScanConfig,stblit_Device_t* Device_p,stblit_Source1Mode_t Mode,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc1(stblit_Node_t* Node_p, STBLIT_Source_t* Src_p, U8 ScanConfig,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc1XYL(stblit_Node_t* Node_p,STGXOBJ_Bitmap_t* Bitmap_p,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc2Color(stblit_Node_t* Node_p,STGXOBJ_Color_t* Color_p,STGXOBJ_Rectangle_t* DstRectangle_p,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc2Bitmap(stblit_Node_t* Node_p,STGXOBJ_Bitmap_t* Bitmap_p,STGXOBJ_Rectangle_t* Rectangle_p,U8 ScanConfig,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc2(stblit_Node_t* Node_p, STBLIT_Source_t* Src_p, STGXOBJ_Rectangle_t*  DstRectangle_p, U8 ScanConfig,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeTgt (stblit_Node_t* Node_p, STGXOBJ_Bitmap_t* Bitmap_p,STGXOBJ_Rectangle_t* Rectangle_p,U8 ScanConfig,stblit_Device_t* Device_p);
__inline static ST_ErrorCode_t SetNodeTgtXYL (stblit_Node_t* Node_p,STGXOBJ_Bitmap_t* Bitmap_p,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeClipping (stblit_Node_t* Node_p, STGXOBJ_Rectangle_t* ClipRectangle_p,BOOL WriteInsideClipRectangle,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t UpdateNodeClipping (stblit_Node_t* Node_p, STGXOBJ_Rectangle_t* ClipRectangle_p,BOOL WriteInsideClipRectangle);
__inline static ST_ErrorCode_t SetNodeColorKey (stblit_Node_t* Node_p,STBLIT_ColorKeyCopyMode_t  ColorKeyCopyMode,STGXOBJ_ColorKey_t* ColorKey_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t UpdateNodeColorKey (stblit_Node_t* Node_p,STBLIT_ColorKeyCopyMode_t  ColorKeyCopyMode,STGXOBJ_ColorKey_t* ColorKey_p);
__inline static ST_ErrorCode_t SetNodePlaneMask (stblit_Node_t* Node_p,U32 MaskWord,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t UpdateNodePlaneMask (stblit_Node_t* Node_p,U32 MaskWord);
__inline static ST_ErrorCode_t SetNodeTrigger (stblit_Node_t* Node_p, STBLIT_BlitContext_t*   BlitContext_p,stblit_CommonField_t* CommonField_p);
#if defined (WA_GNBvd10454)
__inline static ST_ErrorCode_t SetNodeResize (stblit_Node_t* Node_p, U32 SrcWidth, U32 SrcHeight, U32 DstWidth, U32 DstHeight,stblit_Device_t*  Device_p, U8 FilterMode,stblit_CommonField_t* CommonField_p, BOOL* HResizeOn, BOOL* VResizeOn);
__inline static ST_ErrorCode_t SetNodeSliceResize (stblit_Node_t* Node_p,stblit_Device_t* Device_p,U8 FilterMode,STBLIT_SliceRectangle_t* SliceRectangle_p,stblit_CommonField_t* CommonField_p, BOOL* HResizeOn, BOOL* VResizeOn);
#else
__inline static ST_ErrorCode_t SetNodeResize (stblit_Node_t* Node_p, U32 SrcWidth, U32 SrcHeight, U32 DstWidth, U32 DstHeight,stblit_Device_t*  Device_p, U8 FilterMode,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSliceResize (stblit_Node_t* Node_p,stblit_Device_t* Device_p,U8 FilterMode,STBLIT_SliceRectangle_t* SliceRectangle_p,stblit_CommonField_t* CommonField_p);
#endif
__inline static ST_ErrorCode_t SetNodeALUBlend (U8 GlobalAlpha,BOOL PreMultipliedColor,stblit_MaskMode_t MaskMode,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeALUBlendXYL (U8 GlobalAlpha,BOOL  PreMultipliedColor,BOOL  MaskModeEnable,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeALULogic (stblit_MaskMode_t MaskMode,STBLIT_BlitContext_t* BlitContext_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeALULogicXYL (BOOL MaskModeEnable,STBLIT_BlitContext_t* BlitContext_p, stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeFlicker (stblit_Node_t* Node_p, STGXOBJ_ColorType_t ColorType,STBLIT_FlickerFilterMode_t  FlickerFilter,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeInputConverter (U8 InEnable,U8 InConfig,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeOutputConverter (U8 OutEnable,U8 OutConfig,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeColorCorrection (STBLIT_BlitContext_t* BlitContext_p,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeColorExpansion  (STGXOBJ_Palette_t*  Palette_p,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeColorReduction  (STGXOBJ_Palette_t*  Palette_p,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetMaskPalette(STGXOBJ_Color_t* Color_p, STBLIT_BlitContext_t* BlitContext_p, STGXOBJ_Palette_t* MaskPalette_p,stblit_Device_t*  Device_p);

/* Functions ---------------------------------------------------------------- */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
/*******************************************************************************
Name        : stblit_CpuToDevice
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 stblit_CpuToDevice(void* Address)
{
	U32* TempAddress;

	TempAddress = STLAYER_UserToKernel((U32)Address);

	return  ((U32)((U32)TempAddress & MASK_STBUS_FROM_ST40));
}
#endif /* STBLIT_LINUX_FULL_USER_VERSION */


#ifdef WA_WIDTH_8176_BYTES_LIMITATION
/*******************************************************************************
Name        : stblit_WA_CheckWidthByteLimitation
Description : Workaround function for checking if pixel width > 8176 bytes
Parameters  :
Assumptions : Pixel Width <= STBLIT_MAX_WIDTH
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_WA_CheckWidthByteLimitation(STGXOBJ_ColorType_t ColorType, U32 Width)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    switch (ColorType)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            if ((Width * 4) > 8176)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            break;
        case STGXOBJ_COLOR_TYPE_RGB888:
        case STGXOBJ_COLOR_TYPE_ARGB8565:
        case STGXOBJ_COLOR_TYPE_ARGB8565_255:
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
            if ((Width * 3) > 8176)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            break;
        case STGXOBJ_COLOR_TYPE_ARGB1555:
        case STGXOBJ_COLOR_TYPE_ARGB4444:
        case STGXOBJ_COLOR_TYPE_ACLUT88:
        case STGXOBJ_COLOR_TYPE_ACLUT88_255:
        case STGXOBJ_COLOR_TYPE_RGB565:
            if ((Width * 2) > 8176)
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
            if ((Width % 2) == 0)
            {
                if ((Width * 2) > 8176)
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            else
            {
                if ((((Width - 1) * 2) + 3) > 8176)
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:

            /* TBD */

            break;

        case STGXOBJ_COLOR_TYPE_CLUT1:
        case STGXOBJ_COLOR_TYPE_CLUT2:
        case STGXOBJ_COLOR_TYPE_CLUT4:
        case STGXOBJ_COLOR_TYPE_CLUT8:
        case STGXOBJ_COLOR_TYPE_ACLUT44:
        case STGXOBJ_COLOR_TYPE_ALPHA1:
        case STGXOBJ_COLOR_TYPE_ALPHA4:
        case STGXOBJ_COLOR_TYPE_ALPHA8:
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
        case STGXOBJ_COLOR_TYPE_BYTE:

            /* In case of Byte and subbytes format, no problem because
             * Width in byte always < 8176  (STBLIT_MAX_WIDTH < 8176) */
            break;

        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
}
#endif

/*******************************************************************************
Name        : SelectFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SelectFilter (U32 Increment,U32* Offset_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    if (Increment <= 921)
    {
        *Offset_p = 0;
    }
    else if (Increment <=1024)
    {
        *Offset_p = 1 * 40;
    }
    else if (Increment <=1126)
    {
        *Offset_p = 2 * 40;
    }
    else if (Increment <=1228)
    {
        *Offset_p = 3 * 40;
    }
    else if (Increment <=1331)
    {
        *Offset_p = 4 * 40;
    }
    else if (Increment <=1433)
    {
        *Offset_p = 5 * 40;
    }
    else if (Increment <=1536)
    {
        *Offset_p = 6 * 40;
    }
    else if (Increment <=2048)
    {
        *Offset_p = 7 * 40;
    }
    else if (Increment <=3072)
    {
        *Offset_p = 8 * 40;
    }
    else if (Increment <=4096)
    {
        *Offset_p = 9 * 40;
    }
    else if (Increment <=5120)
    {
        *Offset_p = 10 * 40;
    }
    else /*  Increment <=6144 */
    {
        *Offset_p = 11 * 40;
    }

    return(Err);
 }
/*******************************************************************************
Name        : GetFilterMode
Description : get the filter mode to configure
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t GetFilterMode (STGXOBJ_ColorType_t ColorType, U8 ClutEnable, U8 ClutMode,
                                              BOOL LogicalMaskSecondPass, U8* FilterMode)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    /* Look at color format when arriving in the HW 2D-resize block */
    if ((ClutEnable != 0) && (ClutMode == 0))
    {
        /* Color Expansion => Format is ARGB8888, i.e filter can be done on both alpha and color */
        *FilterMode = STBLIT_2DFILTER_ACTIV_BOTH;
    }
    else if ((ClutEnable != 0) && (ClutMode == 2))
    {
        /* Set Color reduction => Format is ACLUT, i.e filter can only be perform on Alpha */
        *FilterMode = STBLIT_2DFILTER_ACTIV_ALPHA;
    }
    else if ((ColorType == STGXOBJ_COLOR_TYPE_ALPHA1)  ||
             (ColorType == STGXOBJ_COLOR_TYPE_CLUT8)   ||
             (ColorType == STGXOBJ_COLOR_TYPE_CLUT4)   ||
             (ColorType == STGXOBJ_COLOR_TYPE_CLUT2)   ||
             (ColorType == STGXOBJ_COLOR_TYPE_CLUT1))
    {
        /* Filter inactive*/
        *FilterMode = STBLIT_2DFILTER_INACTIVE;
    }
    else if ((ColorType == STGXOBJ_COLOR_TYPE_ACLUT88) ||
             (ColorType == STGXOBJ_COLOR_TYPE_ACLUT88_255) ||
             (ColorType == STGXOBJ_COLOR_TYPE_ACLUT44))
    {
        /* Filter can only be perform on Alpha */
        *FilterMode = STBLIT_2DFILTER_ACTIV_ALPHA;
    }
    else if (ColorType == STGXOBJ_COLOR_TYPE_ARGB1555)
    {
        /* Filter has to be disabled on alpha */
        *FilterMode = STBLIT_2DFILTER_ACTIV_COLOR;
    }
    else
    {
        *FilterMode = STBLIT_2DFILTER_ACTIV_BOTH;
    }

    /* Take mask into account : Only when Second pass logic ! : Use special Alpha value 255 -> no filter on alpha allowed! */
    if (LogicalMaskSecondPass == TRUE)
    {
        *FilterMode = (*FilterMode) & ~STBLIT_2DFILTER_ACTIV_ALPHA;
    }

    return(Err);
 }




/*******************************************************************************
Name        : FormatColorType
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level.ex: Only 422 and 420 can be
                                                             Macroblock or Raster
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t FormatColorType (STGXOBJ_ColorType_t TypeIn, U32* TypeOut_p,
                                                U8* AlphaRange_p, STGXOBJ_BitmapType_t Mode)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

     switch (TypeIn)
    {
        case STGXOBJ_COLOR_TYPE_RGB565:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 0;
            break;
        case STGXOBJ_COLOR_TYPE_RGB888:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 1;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB8565:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 4;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB8565_255:
            *AlphaRange_p   = 1;
            *TypeOut_p      = 4;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB8888:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 5;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            *AlphaRange_p   = 1;
            *TypeOut_p      = 5;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB1555:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 6;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 7;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT1:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 8;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT2:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 9;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA4:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 10;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 10;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT8:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 11;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT44:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 12;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT88:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 13;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT88_255:
            *AlphaRange_p   = 1;
            *TypeOut_p      = 13;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 16;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
            *AlphaRange_p   = 0;
            (Mode == STGXOBJ_BITMAP_TYPE_MB) ?  (*TypeOut_p =  19) :  (*TypeOut_p =  18);
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 20;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 16;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
            *AlphaRange_p   = 0;
            (Mode == STGXOBJ_BITMAP_TYPE_MB) ?  (*TypeOut_p =  19) :  (*TypeOut_p =  18);
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 20;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 24;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA8:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 25;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
            *AlphaRange_p   = 1;
            *TypeOut_p      = 25;
            break;
        case STGXOBJ_COLOR_TYPE_BYTE:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 31;
            break;
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
 }

/*******************************************************************************
Name        : stblit_FormatColorForInputFormatter
Description : Format color according to the input to the internal h/W src formatters
Parameters  :
Assumptions :  All parameters checks done at upper level.
Limitations :  Only use for fill rectangle (off cource no MB support , i.e no 420 format)
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_FormatColorForInputFormatter (STGXOBJ_Color_t* ColorIn_p, U32* ValueOut_p)
 {
    ST_ErrorCode_t      Err     = ST_NO_ERROR;
    STGXOBJ_ColorType_t    TypeIn  = ColorIn_p->Type ;
    STGXOBJ_ColorValue_t   ValueIn = ColorIn_p->Value;

    *ValueOut_p = 0;

     switch (TypeIn)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            *ValueOut_p = (U32)(((ValueIn.ARGB8888.Alpha & 0xFF) << 24) |
                           ((ValueIn.ARGB8888.R & 0xFF) << 16)     |
                           ((ValueIn.ARGB8888.G & 0xFF) << 8) |
                            (ValueIn.ARGB8888.B & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_RGB888:
            *ValueOut_p = (U32)(((ValueIn.RGB888.R & 0xFF) << 16) |
                          ((ValueIn.RGB888.G & 0xFF) << 8) |
                           (ValueIn.RGB888.B & 0xFF));
            break;
       case STGXOBJ_COLOR_TYPE_ARGB8565:
       case STGXOBJ_COLOR_TYPE_ARGB8565_255:
            *ValueOut_p = (U32)(((ValueIn.ARGB8565.Alpha & 0xFF) << 16) |
                           ((ValueIn.ARGB8565.R & 0x1F) << 11) |
                           ((ValueIn.ARGB8565.G & 0x3F) << 5) |
                            (ValueIn.ARGB8565.B & 0x1F));
            break;
        case STGXOBJ_COLOR_TYPE_RGB565:
            *ValueOut_p = (U32)(((ValueIn.RGB565.R & 0x1F) << 11) |
                                ((ValueIn.RGB565.G & 0x3F) << 5) |
                                 (ValueIn.RGB565.B & 0x1F));
            break;
        case STGXOBJ_COLOR_TYPE_ARGB1555:
            *ValueOut_p = (U32)(((ValueIn.ARGB1555.Alpha & 0x1) << 15) |
                           ((ValueIn.ARGB1555.R & 0x1F) << 10)     |
                           ((ValueIn.ARGB1555.G & 0x1F) << 5) |
                            (ValueIn.ARGB1555.B & 0x1F));
            break;
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            *ValueOut_p = (U32)(((ValueIn.ARGB4444.Alpha & 0xF) << 12) |
                           ((ValueIn.ARGB4444.R & 0xF) << 8)     |
                           ((ValueIn.ARGB4444.G & 0xF) << 4) |
                            (ValueIn.ARGB4444.B & 0xF));
            break;
        case STGXOBJ_COLOR_TYPE_CLUT1:
            *ValueOut_p =  (U32)(ValueIn.CLUT1 & 0x1);
            break;
        case STGXOBJ_COLOR_TYPE_CLUT2:
            *ValueOut_p =  (U32)(ValueIn.CLUT2 & 0x3);
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            *ValueOut_p =  (U32)(ValueIn.CLUT4 & 0xF);
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT44:
            *ValueOut_p = (U32)(((ValueIn.ACLUT44.Alpha & 0xF) << 4) |
                                (ValueIn.ACLUT44.PaletteEntry & 0xF));
            break;
        case STGXOBJ_COLOR_TYPE_CLUT8:
            *ValueOut_p =  (U32)(ValueIn.CLUT8 & 0xFF);
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT88:
        case STGXOBJ_COLOR_TYPE_ACLUT88_255:
            *ValueOut_p = (U32)(((ValueIn.ACLUT88.Alpha & 0xFF) << 8) |
                                (ValueIn.ACLUT88.PaletteEntry & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            *ValueOut_p =  (U32)(ValueIn.ALPHA1 & 0x1);
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA4:
            *ValueOut_p =  (U32)(ValueIn.ALPHA4 & 0xF);
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA8:
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
            *ValueOut_p =  (U32)(ValueIn.ALPHA8 & 0xFF);
            break;
        case STGXOBJ_COLOR_TYPE_BYTE:
            *ValueOut_p =  (U32)(ValueIn.Byte & 0xFF);
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
            *ValueOut_p =  (U32)(((ValueIn.UnsignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 8) |
                                  (ValueIn.UnsignedYCbCr888_444.Cb & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
            *ValueOut_p =  (U32)(((ValueIn.SignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.SignedYCbCr888_444.Y  & 0xFF) << 8) |
                                  (ValueIn.SignedYCbCr888_444.Cb & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
            *ValueOut_p =  (U32)(((ValueIn.UnsignedYCbCr888_422.Y  & 0xFF) << 24) |  /* Y redondancy for direct fill/copy */
                                 ((ValueIn.UnsignedYCbCr888_422.Cr & 0xFF) << 16) |
                                 ((ValueIn.UnsignedYCbCr888_422.Y  & 0xFF) << 8) |
                                  (ValueIn.UnsignedYCbCr888_422.Cb & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
            *ValueOut_p =  (U32)(((ValueIn.SignedYCbCr888_422.Y  & 0xFF) << 24) | /* Y redondancy for direct fill/copy */
                                 ((ValueIn.SignedYCbCr888_422.Cr & 0xFF) << 16) |
                                 ((ValueIn.SignedYCbCr888_422.Y  & 0xFF) << 8) |
                                  (ValueIn.SignedYCbCr888_422.Cb & 0xFF));
            break;

        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
 }

/*******************************************************************************
Name        : stblit_FormatColorForOutputFormatter
Description : Format color according to the output of the internal h/W src formatters
Parameters  :
Assumptions :  All parameters checks done at upper level.
               When RGB expansion, MSB filled with 0 mode only!
               Internal format is only having 0:128 alpha range.
               => 0:255 alpha range must be converted to 0:128 ((Alpha + 1) >> 1)
Limitations :  Only implemented for 4:4:4 formats
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_FormatColorForOutputFormatter (STGXOBJ_Color_t* ColorIn_p, U32* ValueOut_p)
 {
    ST_ErrorCode_t          Err     = ST_NO_ERROR;
    STGXOBJ_ColorType_t     TypeIn  = ColorIn_p->Type ;
    STGXOBJ_ColorValue_t    ValueIn = ColorIn_p->Value;
    U32                     Alpha   = 0;

    *ValueOut_p = 0;

     switch (TypeIn)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
            *ValueOut_p = (U32)(((ValueIn.ARGB8888.Alpha & 0xFF) << 24) |
                           ((ValueIn.ARGB8888.R & 0xFF) << 16)     |
                           ((ValueIn.ARGB8888.G & 0xFF) << 8) |
                            (ValueIn.ARGB8888.B & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            *ValueOut_p = (U32)(((((ValueIn.ARGB8888.Alpha + 1) >> 1) & 0xFF) << 24) |
                           ((ValueIn.ARGB8888.R & 0xFF) << 16)     |
                           ((ValueIn.ARGB8888.G & 0xFF) << 8) |
                            (ValueIn.ARGB8888.B & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_RGB888:
            *ValueOut_p = (U32)(((ValueIn.RGB888.R & 0xFF) << 16) |
                          ((ValueIn.RGB888.G & 0xFF) << 8) |
                           (ValueIn.RGB888.B & 0xFF));
            break;
       case STGXOBJ_COLOR_TYPE_ARGB8565:
            *ValueOut_p = (U32)(((ValueIn.ARGB8565.Alpha & 0xFF) << 24) |
                           ((ValueIn.ARGB8565.R & 0x1F) << 19) |
                           ((ValueIn.ARGB8565.G & 0x3F) << 10) |
                           ((ValueIn.ARGB8565.B & 0x1F) << 3 ));
            break;
       case STGXOBJ_COLOR_TYPE_ARGB8565_255:
            *ValueOut_p = (U32)(((((ValueIn.ARGB8565.Alpha + 1) >> 1) & 0xFF) << 24) |
                           ((ValueIn.ARGB8565.R & 0x1F) << 19) |
                           ((ValueIn.ARGB8565.G & 0x3F) << 10) |
                           ((ValueIn.ARGB8565.B & 0x1F) << 3 ));
            break;
        case STGXOBJ_COLOR_TYPE_RGB565:
            *ValueOut_p = (U32)((((U32)128) << 24) |
                          ((ValueIn.RGB565.R & 0x1F) << 19) |
                          ((ValueIn.RGB565.G & 0x3F) << 10) |
                          ((ValueIn.RGB565.B & 0x1F) << 3));
            break;
        case STGXOBJ_COLOR_TYPE_ARGB1555:
            *ValueOut_p = (U32)(((ValueIn.ARGB1555.Alpha & 0x1) << 31) |
                                ((ValueIn.ARGB1555.R & 0x1F) << 19)     |
                                ((ValueIn.ARGB1555.G & 0x1F) << 11) |
                                ((ValueIn.ARGB1555.B & 0x1F) << 3));
            break;
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            if ((ValueIn.ARGB4444.Alpha & 0xF) == 0xF)
            {
                Alpha  =  128;
            }
            else  if ((ValueIn.ARGB4444.Alpha & 0xF) == 0)
            {
                Alpha = 0;
            }
            else
            {
                Alpha  =  (U8)(((ValueIn.ARGB4444.Alpha & 0xF) << 3) | 0x4);
            }
            *ValueOut_p = (U32)(((Alpha & 0xFF) << 24) |
                          ((ValueIn.ARGB4444.R & 0xF) << 20) |
                          ((ValueIn.ARGB4444.G & 0xF) << 12) |
                          ((ValueIn.ARGB4444.B & 0xF) << 4  ));
            break;
        case STGXOBJ_COLOR_TYPE_CLUT1:
            *ValueOut_p =  (U32)(ValueIn.CLUT1 & 0x1);
            break;
        case STGXOBJ_COLOR_TYPE_CLUT2:
            *ValueOut_p =  (U32)(ValueIn.CLUT2 & 0x3);
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            *ValueOut_p =  (U32)(ValueIn.CLUT4 & 0xF);
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT44:
            if ((ValueIn.ACLUT44.Alpha & 0xF) == 0xF)
            {
                Alpha  =  128;
            }
            else  if ((ValueIn.ACLUT44.Alpha & 0xF) == 0)
            {
                Alpha = 0;
            }
            else
            {
                Alpha  =  (U8)(((ValueIn.ACLUT44.Alpha & 0xF) << 3) | 0x4);
            }
            *ValueOut_p = (U32)(((Alpha & 0xFF) << 24) |
                                 (ValueIn.ACLUT44.PaletteEntry & 0xF));
            break;
        case STGXOBJ_COLOR_TYPE_CLUT8:
            *ValueOut_p =  (U32)(ValueIn.CLUT8 & 0xFF);
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT88:
            *ValueOut_p = (U32)(((ValueIn.ACLUT88.Alpha & 0xFF) << 24) |
                                 (ValueIn.ACLUT88.PaletteEntry & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT88_255:
            *ValueOut_p = (U32)(((((ValueIn.ACLUT88.Alpha + 1) >> 1) & 0xFF) << 24) |
                                 (ValueIn.ACLUT88.PaletteEntry & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            *ValueOut_p =  (U32)((ValueIn.ALPHA1 & 0x1) << 31);
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA4:
            *ValueOut_p =  (U32)(ValueIn.ALPHA4 & 0xF);
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA8:
            *ValueOut_p =  (U32)(ValueIn.ALPHA8 & 0xFF);
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
            *ValueOut_p =  (U32)(((ValueIn.ALPHA8 + 1) >> 1) & 0xFF);
            break;
        case STGXOBJ_COLOR_TYPE_BYTE:
            *ValueOut_p =  (U32)(ValueIn.Byte & 0xFF);
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
            *ValueOut_p =  (U32)((((U32)128) << 24) |
                                 ((ValueIn.UnsignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 8) |
                                  (ValueIn.UnsignedYCbCr888_444.Cb & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
            *ValueOut_p =  (U32)((((U32)128) << 24) |
                                 ((ValueIn.SignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.SignedYCbCr888_444.Y  & 0xFF) << 8) |
                                  (ValueIn.SignedYCbCr888_444.Cb & 0xFF));
            break;
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
 }


/*******************************************************************************
Name        : stblit_WriteCommonFieldInNode
Description : Fill Node with its common field values
Parameters  :
Assumptions : All parameters checks done at upper level
Limitations :
Returns     :
*******************************************************************************/
void stblit_WriteCommonFieldInNode(stblit_Node_t*          Node_p,
                                   stblit_CommonField_t*   CommonField_p)
 {
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)), CommonField_p->INS);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_ACK)), CommonField_p->ACK);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RZC)), CommonField_p->RZC);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CCO)), CommonField_p->CCO);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CML)), CommonField_p->CML);
 }


/*******************************************************************************
Name        : stblit_Connect2Nodes
Description : Simple connection between 2 nodes.
Parameters  :
Assumptions : All parameters checks done at upper level
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_Connect2Nodes(stblit_Device_t* Device_p,
                                    stblit_Node_t*   FirstNode_p,
                                    stblit_Node_t*   SecondNode_p)
 {
    ST_ErrorCode_t   Err = ST_NO_ERROR;
    U32              Tmp = 0;

    /* Upate Next Node  of the first node*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if ( PhysicJobBlitNodeBuffer_p )
    {
        Tmp = (U32) SecondNode_p - (U32) Device_p->JobBlitNodeBuffer_p + (U32) PhysicJobBlitNodeBuffer_p ;
    }
    else
    if ( (U32) SecondNode_p > (U32) Device_p->SingleBlitNodeBuffer_p )
    {
        Tmp = (U32) SecondNode_p - (U32) Device_p->SingleBlitNodeBuffer_p + (U32) PhysicSingleBlitNodeBuffer_p ;
    }
#else
    Tmp = (U32)stblit_CpuToDevice((void*)SecondNode_p,&(Device_p->VirtualMapping));
#endif

    STSYS_WriteRegMem32LE((void*)(&(FirstNode_p->HWNode.BLT_NIP)), Tmp);

    /* Update previous Node of the second node*/
    SecondNode_p->PreviousNode = (stblit_NodeHandle_t)FirstNode_p;

    return(Err);
 }

/*******************************************************************************
Name        : InitNode
Description : HWNode is reset first and then some general stuff is set
Parameters  :
Assumptions :  All parameters checks done at upper level
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t InitNode(stblit_Node_t*          Node_p,
                                          STBLIT_BlitContext_t*   BlitContext_p)
 {

    ST_ErrorCode_t   Err = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t1 = STOS_time_now();
    #endif


    /* Initialize Node common stuff*/
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    Node_p->SubscriberID     = BlitContext_p->EventSubscriberID;
#endif
    Node_p->JobHandle        = BlitContext_p->JobHandle;
    Node_p->PreviousNode     = STBLIT_NO_NODE_HANDLE;    /* Only one node operation */

    /* Initialize User tag */
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_USR)),((U32)((U32)(BlitContext_p->UserTag_p) & STBLIT_USR_MASK) << STBLIT_USR_SHIFT ));

    #ifdef STBLIT_BENCHMARK3
    t2 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeIT
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeIT(stblit_Node_t*            Node_p,
                                          STBLIT_BlitContext_t*     BlitContext_p,
                                          stblit_CommonField_t*     CommonField_p)
 {
    ST_ErrorCode_t   Err = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t3 = STOS_time_now();
    #endif

    /* Initialize interruption stuff*/
    CommonField_p->INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_SUSPENDED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);

    if (BlitContext_p->NotifyBlitCompletion == TRUE)
    {
        Node_p->ITOpcode |= STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
        CommonField_p->INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
    }

    #ifdef STBLIT_BENCHMARK3
    t4 = STOS_time_now();
    #endif

    return(Err);
 }


/*******************************************************************************
Name        : OverlapOneSourceAddress
Description : Look if there is memory overlap between src and dst.
              Method : Look at Src/Dst byte addresses
Parameters  :
Assumptions : Pitch identical between src and dst
              All parameters are in bytes!
              Litter Endian
Limitations :
Returns     : Code giving the scan direction
*******************************************************************************/
/* __inline static U8 OverlapOneSourceAddress(U32 SrcStart, U32 SrcStop, U32 DstStart, U32 DstStop, U32 Pitch, U32 SrcWidth, U32 DstWidth)*/
/*{*/
/*    U8  Code = 0xF;*/
/*    U32 Gap;*/
/*    U32 GapRemainder;*/
    /* U8 code :
     * _________________________________________
     * |    |    |    |    |    |    |    |    |
     * | 0  | 0  | 0  | 0  | 1  | 1  | 1  |  1 |
     * |____|____|____|____|____|____|____|____|
     *                        |    |    |    |
     *                        |    |    |    |
     *                        |    |    |    |
     *                       \/   \/   \/   \/
     *                      Up   Up    Down   Down
     *                     Left  Right Left  Right
     */

    /* if Src = Dst => No Problem */
/*    if (MemoryIdentical(SrcStart,SrcStop, DstStart, DstStop))*/
/*    {*/
/*        Code = 0xF;  *//* Code = 00001111 : All directions supported */
/*        return(Code);*/
/*    }*/
    /* Not supported when one of the memory range overlap completely the other */
/*    if ((MemoryRangeWithinRange(SrcStart, SrcStop, DstStart, DstStop)) ||*/
/*        (MemoryRangeWithinRange(DstStart, DstStop, SrcStart, SrcStop)))*/
/*    {*/
/*        Code = 0;  *//* Not Supported */
/*        return(Code);*/
/*    }*/

/*    if (MemoryInterlaced(SrcStart, SrcStop, DstStart, DstStop))*/
/*    {*/
/*        GapRemainder =  (AbsMinus(SrcStart,DstStart)) % Pitch;*/
/*        Gap = (AbsMinus(SrcStart,DstStart)) / Pitch;*/

/*        if (SrcStart < DstStart)*/
/*        {*/
/*            if (GapRemainder > (Pitch - DstWidth))*/
/*            {*/
                /* Up Right Src according to Dst */
/*                Code = 0xC; */ /* Copy Direction UpLeft or UpRight */
/*            }*/
/*            else if (GapRemainder == 0 )*/
/*            {*/
                /* Immediate Up Src according to Dst */
/*                Code = 0xC; *//* Copy Direction UpLeft or UpRight */
/*            }*/
/*            else if (GapRemainder < SrcWidth)*/
/*            {*/
/*                if (Gap == 0)*/
/*                {*/
                    /* Immediate left Src according to Dst */
/*                    Code =0xA; */ /* Copy Direction UpLeft or DownLeft */
/*                }*/
/*                else*/
/*                {*/
                    /* Up Left Src according to Dst */
/*                    Code = 0xC; */ /* Copy Direction UpLeft or UpRight */
/*                }*/
/*            }*/
/*        }*/
/*        else*/
/*        {*/
/*            if (GapRemainder > (Pitch - SrcWidth))*/
/*            {*/
                /*  Down Left Src according to Dst */
/*                Code = 0x3; */ /* Copy Direction DownLeft or DownRight */
/*            }*/
/*            else if (GapRemainder == 0 )*/
/*            {*/
                /* Immediate Down Src according to Dst */
/*                Code = 0x3; */ /* Copy Direction DownLeft or DownRight */
/*            }*/
/*            else if (GapRemainder < DstWidth)*/
/*            {*/
/*                if (Gap == 0)*/
/*                {*/
                    /* Immediate Right Src according to Dst */
/*                    Code =0x5; */ /* Copy Direction UpRight or DownRight */
/*                }*/
/*                else*/
/*                {*/
                    /* Down Right Src according to Dst */
/*                    Code = 0x3;*/ /* Copy Direction DownLeft or DownRight */
/*                }*/
/*            }*/
/*        }*/
/*    }*/

/*    return(Code);*/
/*}*/

/*******************************************************************************
Name        : OverlapOneSourceCoordinate
Description : Look if there is overlap between src and dst
              Method : Look at Src/Dst coordinnates
Parameters  :
Assumptions : Same referential for Dst and Src coordiantes (ie from same bitmap)
Limitations :
Returns     : Code giving the scan direction
*******************************************************************************/
 __inline static U8 OverlapOneSourceCoordinate(U32 SrcXStart, U32 SrcYStart,U32 SrcXStop,U32 SrcYStop,
                                               U32 DstXStart, U32 DstYStart,U32 DstXStop,U32 DstYStop)
{
    U8  Code = 0xF;
    /* U8 code :
     * _________________________________________
     * |    |    |    |    |    |    |    |    |
     * | 0  | 0  | 0  | 0  | 1  | 1  | 1  |  1 |
     * |____|____|____|____|____|____|____|____|
     *                        |    |    |    |
     *                        |    |    |    |
     *                        |    |    |    |
     *                       \/   \/   \/   \/
     *                      Up   Up    Down   Down
     *                     Left  Right Left  Right
     */

    /* if Src = Dst => No Problem */
    if ((SrcXStart == DstXStart) &&
        (SrcYStart == DstYStart) &&
        (SrcXStop == DstXStop)   &&
        (SrcYStop == DstYStop))
    {
        Code = 0xF;  /* Code = 00001111 : All directions supported */
        return(Code);
    }
    /* Not supported when one of the range overlap completely the other */
    if (((SrcXStart <= DstXStart) && (SrcYStart <= DstYStart) && (SrcXStop  >= DstXStop) && (SrcYStop  >= DstYStop))    ||
        ((SrcXStart >= DstXStart) && (SrcYStart >= DstYStart) && (SrcXStop  <= DstXStop) && (SrcYStop  <= DstYStop))    ||
        ((SrcYStart < DstYStart) && (SrcYStop >  DstYStop) && (((SrcXStart < DstXStop) && (SrcXStart > DstXStart)) ||
                                                               ((DstXStart < SrcXStop) && (DstXStart > SrcXStart))))    ||
        ((DstYStart < SrcYStart) && (DstYStop >  SrcYStop) && (((SrcXStart < DstXStop) && (SrcXStart > DstXStart)) ||
                                                               ((DstXStart < SrcXStop) && (DstXStart > SrcXStart)))))
    {
        Code = 0;  /* Not Supported */
        return(Code);
    }



    if ((SrcYStart < DstYStart) && (SrcYStop < DstYStop))
    {
        /* Up left, Up Right or Immediate Up Src according to  Dst */
        Code = 0xC; /* Copy Direction UpLeft or UpRight */
    }
    else if ((DstYStart < SrcYStart) && (DstYStop < SrcYStop))
    {
        /* Down left, Down Right or Immediate Down Src according to Dst */
        Code = 0x3; /* Copy Direction DownLeft or DownRight */
    }
    else if ((SrcYStart == DstYStart) && (SrcXStart < DstXStart) && (SrcXStop < DstXStop) )
    {
        /* Immediate Left Src according to Dst */
        Code =0xA;  /* Copy Direction UpLeft or DownLeft */
    }
    else if ((SrcYStart == DstYStart) && (DstXStart < SrcXStart) && (DstXStop < SrcXStop))
    {
        /* Immediate Right Src according to Dst */
        Code =0x5;  /* Copy Direction UpRight or DownRight */
    }

    return(Code);
}


/******************************************************************************
Name        : SetNodeSrc1Color
Description : Src1 is a color
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeSrc1Color(stblit_Node_t*          Node_p,
                                                  STGXOBJ_Color_t*        Color_p,
                                                  stblit_Device_t*        Device_p,
                                                  stblit_Source1Mode_t    Mode,
                                                  stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Color;
    U32             ColorType = 0;
    U32             S1TY;
    U8              AlphaRange = 0;

    /* Src1 Instruction */
    if (Mode == STBLIT_SOURCE1_MODE_NORMAL)
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_COLOR_FILL & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);
    }
    else  /* STBLIT_SOURCE1_MODE_DIRECT_FILL */
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_DIRECT_FILL & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);
    }

    /* Set Src1 Type (Default for RGB Expansion is : Missing LSBs are filled with 0)*/
    FormatColorType(Color_p->Type,&ColorType,&AlphaRange,STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE) ;  /* Prepare color Type HW representation*/
    S1TY = ((U32)(ColorType & STBLIT_S1TY_COLOR_MASK) << STBLIT_S1TY_COLOR_SHIFT);
    S1TY |= ((U32)(1 & STBLIT_S1TY_RGB_EXPANSION_MASK) << STBLIT_S1TY_RGB_EXPANSION_SHIFT);
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)  ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        S1TY |=  ((U32)(AlphaRange & STBLIT_S1TY_ALPHA_RANGE_MASK) << STBLIT_S1TY_ALPHA_RANGE_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1TY)),S1TY);

    /*Src1 Color Fill */
    stblit_FormatColorForInputFormatter(Color_p,&Color) ; /* Prepare color HW representation*/
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1CF)),((U32)(Color & STBLIT_S1CF_COLOR_MASK) << STBLIT_S1CF_COLOR_SHIFT));

    /* Src1 Size */
    /* The size is the same as the target. Setting done in SetNodeTgt().*/

    return(Err);
 }

/******************************************************************************
Name        : SetNodeSrc1Bitmap
Description : Src1 is a bitmap
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
               Concern always Data1_p  whatever mode (raster, MB)
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeSrc1Bitmap(stblit_Node_t*          Node_p,
                                                   STGXOBJ_Bitmap_t*       Bitmap_p,
                                                   STGXOBJ_Rectangle_t*    Rectangle_p,
                                                   U8                      ScanConfig,
                                                   stblit_Device_t*        Device_p,
                                                   stblit_Source1Mode_t    Mode,
                                                   stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             ColorType = 0 ;
    U32             Tmp;
    U32             S1TY, S1XY;
    U8              AlphaRange = 0;

    /* Src1 Instruction */
    if (Mode == STBLIT_SOURCE1_MODE_NORMAL)
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_MEMORY & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);
    }
    else  /* STBLIT_SOURCE1_MODE_DIRECT_COPY */
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_DIRECT_COPY & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);
    }

    /* Src1 Base Address */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
#else
    Tmp =  (U32) ((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1BA)),((U32)(Tmp & STBLIT_S1BA_POINTER_MASK) << STBLIT_S1BA_POINTER_SHIFT));

    /* Set Src1 Type (Default for RGB Expansion is : Missing LSBs are filled with 0)*/
    FormatColorType(Bitmap_p->ColorType,&ColorType,&AlphaRange,Bitmap_p->BitmapType); /* Prepare color Type HW representation*/
    S1TY = ((U32)(Bitmap_p->Pitch & STBLIT_S1TY_PITCH_MASK) << STBLIT_S1TY_PITCH_SHIFT);
    S1TY |=  ((U32)(ColorType & STBLIT_S1TY_COLOR_MASK) << STBLIT_S1TY_COLOR_SHIFT ); /* TBD */
    S1TY |=  ((U32)(ScanConfig & STBLIT_S1TY_SCAN_ORDER_MASK) << STBLIT_S1TY_SCAN_ORDER_SHIFT );
    S1TY |=  ((U32)(Bitmap_p->SubByteFormat & STBLIT_S1TY_SUB_BYTE_MASK) << STBLIT_S1TY_SUB_BYTE_SHIFT);
    S1TY |=  ((U32)(1 & STBLIT_S1TY_RGB_EXPANSION_MASK) << STBLIT_S1TY_RGB_EXPANSION_SHIFT);
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        S1TY |=  ((U32)(AlphaRange & STBLIT_S1TY_ALPHA_RANGE_MASK) << STBLIT_S1TY_ALPHA_RANGE_SHIFT);
        if (Bitmap_p->BigNotLittle == TRUE)
        {
            S1TY |=  ((U32)(1 & STBLIT_S1TY_BIG_NOT_LITTLE_MASK) << STBLIT_S1TY_BIG_NOT_LITTLE_SHIFT);
        }
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1TY)),S1TY);

    /* Src1 XY location. It depends on the programmed horizontal and vertical scan direction */
    if (((ScanConfig & 0x2) >> 1) == 0)
    {
        S1XY = ((U32)(Rectangle_p->PositionY & STBLIT_S1XY_Y_MASK) << STBLIT_S1XY_Y_SHIFT);
    }
    else
    {
        S1XY = ((U32)((Rectangle_p->PositionY + Rectangle_p->Height - 1) & STBLIT_S1XY_Y_MASK) << STBLIT_S1XY_Y_SHIFT);
    }

    if ((ScanConfig & 0x1) == 0)
    {
        S1XY |= ((U32)(Rectangle_p->PositionX & STBLIT_S1XY_X_MASK) << STBLIT_S1XY_X_SHIFT);
    }
    else
    {
        S1XY |= ((U32)((Rectangle_p->PositionX + Rectangle_p->Width - 1) & STBLIT_S1XY_X_MASK) << STBLIT_S1XY_X_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1XY)),S1XY);

    /* Src1 Size */
    /* The size is the same as the target. Setting done in SetNodeTgt().*/


    return(Err);
 }

/******************************************************************************
Name        : SetNodeSrc1XYL
Description : Src1 is a bitmap
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
               Concern always Data1_p  whatever mode (raster, MB)
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeSrc1XYL(stblit_Node_t*          Node_p,
                                                STGXOBJ_Bitmap_t*       Bitmap_p,
                                                stblit_Device_t*        Device_p,
                                                stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             ColorType = 0;
    U32             Tmp;
    U32             S1TY;
    U8              AlphaRange = 0;

    /* Src1 Instruction */
    CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_MEMORY & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);

    /* Src1 Base Address */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
#else
    Tmp =  (U32) ((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1BA)),((U32)(Tmp & STBLIT_S1BA_POINTER_MASK) << STBLIT_S1BA_POINTER_SHIFT));

    /* Set Src1 Type (Default for RGB Expansion is : Missing LSBs are filled with 0)*/
    FormatColorType(Bitmap_p->ColorType,&ColorType,&AlphaRange,Bitmap_p->BitmapType); /* Prepare color Type HW representation*/
    S1TY = ((U32)(Bitmap_p->Pitch & STBLIT_S1TY_PITCH_MASK) << STBLIT_S1TY_PITCH_SHIFT);
    S1TY |=  ((U32)(ColorType & STBLIT_S1TY_COLOR_MASK) << STBLIT_S1TY_COLOR_SHIFT ); /* TBD */
    S1TY |=  ((U32)(Bitmap_p->SubByteFormat & STBLIT_S1TY_SUB_BYTE_MASK) << STBLIT_S1TY_SUB_BYTE_SHIFT);
    S1TY |=  ((U32)(1 & STBLIT_S1TY_RGB_EXPANSION_MASK) << STBLIT_S1TY_RGB_EXPANSION_SHIFT);
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        S1TY |=  ((U32)(AlphaRange & STBLIT_S1TY_ALPHA_RANGE_MASK) << STBLIT_S1TY_ALPHA_RANGE_SHIFT);
        if (Bitmap_p->BigNotLittle == TRUE)
        {
            S1TY |=  ((U32)(1 & STBLIT_S1TY_BIG_NOT_LITTLE_MASK) << STBLIT_S1TY_BIG_NOT_LITTLE_SHIFT);
        }
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1TY)),S1TY);

    return(Err);
 }




/******************************************************************************
Name        : SetNodeSrc1
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
Limitations :  No direct Copy nor Direct Fill in this function, only Supported separetely by SetNodeSrc1Color
               and SetNodeSrc1Bitmap
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeSrc1(stblit_Node_t*          Node_p,
                                             STBLIT_Source_t*        Src_p,
                                             U8                      ScanConfig,
                                             stblit_Device_t*        Device_p,
                                             stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t5 = STOS_time_now();
    #endif

    if (Src_p == NULL)   /* Src1 disabled */
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_DISABLED & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);

    }
    else
    {
        if (Src_p->Type == STBLIT_SOURCE_TYPE_COLOR)  /* Fill rectangle */
        {
            SetNodeSrc1Color(Node_p,Src_p->Data.Color_p,Device_p,STBLIT_SOURCE1_MODE_NORMAL,CommonField_p);
        }
        else  /* Copy Rectangle */
        {
            SetNodeSrc1Bitmap(Node_p,Src_p->Data.Bitmap_p,&(Src_p->Rectangle),ScanConfig, Device_p, STBLIT_SOURCE1_MODE_NORMAL,CommonField_p);
        }

    }
    #ifdef STBLIT_BENCHMARK3
    t6 = STOS_time_now();
    #endif

    return(Err);
 }


/*******************************************************************************
Name        : SetNodeSrc2Color
Description : Src2 is a color
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeSrc2Color(stblit_Node_t*          Node_p,
                                                 STGXOBJ_Color_t*        Color_p,
                                                 STGXOBJ_Rectangle_t*    DstRectangle_p,
                                                 stblit_Device_t*        Device_p,
                                                 stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Color;
    U32             ColorType = 0;
    U32             S2TY, S2SZ;
    U8              AlphaRange = 0;

    /* Src2 Instruction */
    CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_COLOR_FILL & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);

    /* Src2 Type (Default for RGB Expansion is : Missing LSBs are filled with 0) */
    FormatColorType(Color_p->Type,&ColorType,&AlphaRange,STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE) ; /* Prepare color Type HW representation*/
    S2TY = ((U32)(ColorType & STBLIT_S2TY_COLOR_MASK) << STBLIT_S2TY_COLOR_SHIFT );
    S2TY |= ((U32)(1 & STBLIT_S2TY_RGB_EXPANSION_MASK) << STBLIT_S2TY_RGB_EXPANSION_SHIFT);
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        S2TY |=  ((U32)(AlphaRange & STBLIT_S2TY_ALPHA_RANGE_MASK) << STBLIT_S2TY_ALPHA_RANGE_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)),S2TY);

    /*Src2 Color Fill */
    stblit_FormatColorForInputFormatter(Color_p,&Color) ; /* Prepare color HW representation */
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2CF)),((U32)(Color & STBLIT_S2CF_COLOR_MASK) << STBLIT_S2CF_COLOR_SHIFT));

    /* Set Src2 Size = Dst size (size can not be ignored : HW constraint) */
    S2SZ = ((U32)(DstRectangle_p->Width & STBLIT_S2SZ_WIDTH_MASK) << STBLIT_S2SZ_WIDTH_SHIFT);
    S2SZ |= ((U32)(DstRectangle_p->Height & STBLIT_S2SZ_HEIGHT_MASK) << STBLIT_S2SZ_HEIGHT_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2SZ)),S2SZ);

    return(Err);
 }


/*******************************************************************************
Name        : SetNodeSrc2Bitmap
Description : Src2 is a bitmap.
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
               Concerns Data1_p, or Data2_p if MBlock
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeSrc2Bitmap(stblit_Node_t*         Node_p,
                                                 STGXOBJ_Bitmap_t*      Bitmap_p,
                                                 STGXOBJ_Rectangle_t*   Rectangle_p,
                                                 U8                     ScanConfig,
                                                 stblit_Device_t*       Device_p,
                                                 stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             ColorType = 0;
    U32             Tmp;
    U32             S2TY, S2XY, S2SZ;
    U8              AlphaRange = 0;

    /* Src2 Instruction */
    CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_MEMORY & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);

    /* Src2 Base Address */
    if (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
    #ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
    #else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
    #endif
    }
    else  /* STGXOBJ_BITMAP_TYPE_MB */
    {
    #ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data2_p) + Bitmap_p->Offset);
    #else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data2_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
    #endif
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));


    /* Src2 Type :
     * + Default for RGB Expansion is : Missing LSBs are filled with 0)
     * + Chroma line is enabled by default : Make sure that it does not affect non 420MB modes ! TB verified */
    FormatColorType(Bitmap_p->ColorType,&ColorType,&AlphaRange,Bitmap_p->BitmapType) ; /* Prepare color Type HW representation*/
    S2TY = ((U32)(Bitmap_p->Pitch & STBLIT_S2TY_PITCH_MASK) << STBLIT_S2TY_PITCH_SHIFT );
    S2TY |= ((U32)(ColorType & STBLIT_S2TY_COLOR_MASK) << STBLIT_S2TY_COLOR_SHIFT);
    S2TY |= ((U32)(ScanConfig & STBLIT_S2TY_SCAN_ORDER_MASK) << STBLIT_S2TY_SCAN_ORDER_SHIFT);
    S2TY |= ((U32)(Bitmap_p->SubByteFormat & STBLIT_S2TY_SUB_BYTE_MASK) << STBLIT_S2TY_SUB_BYTE_SHIFT);
    S2TY |= ((U32)(1 & STBLIT_S2TY_RGB_EXPANSION_MASK) << STBLIT_S2TY_RGB_EXPANSION_SHIFT);
    S2TY |= ((U32)(1 & STBLIT_S2TY_CHROMA_LINE_REPEAT_MASK) << STBLIT_S2TY_CHROMA_LINE_REPEAT_SHIFT);
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        S2TY |=  ((U32)(AlphaRange & STBLIT_S2TY_ALPHA_RANGE_MASK) << STBLIT_S2TY_ALPHA_RANGE_SHIFT);
        if (Bitmap_p->BigNotLittle == TRUE)
        {
            S2TY |=  ((U32)(1 & STBLIT_S2TY_BIG_NOT_LITTLE_MASK) << STBLIT_S2TY_BIG_NOT_LITTLE_SHIFT);
        }
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)),S2TY);

    /* Src2 XY location. It depends on the programmed horizontal and vertical scan direction */
    if (((ScanConfig & 0x2) >> 1) == 0)
    {
        S2XY = ((U32)(Rectangle_p->PositionY & STBLIT_S2XY_Y_MASK) << STBLIT_S2XY_Y_SHIFT);
    }
    else
    {
        S2XY = ((U32)((Rectangle_p->PositionY + Rectangle_p->Height - 1) & STBLIT_S2XY_Y_MASK) << STBLIT_S2XY_Y_SHIFT);
    }

    if ((ScanConfig & 0x1) == 0)
    {
        S2XY |= ((U32)(Rectangle_p->PositionX & STBLIT_S2XY_X_MASK) << STBLIT_S2XY_X_SHIFT);
    }
    else
    {
        S2XY |= ((U32)((Rectangle_p->PositionX + Rectangle_p->Width - 1) & STBLIT_S2XY_X_MASK) << STBLIT_S2XY_X_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2XY)),S2XY);


    /* Src2 Size */
    S2SZ = ((U32)(Rectangle_p->Width & STBLIT_S2SZ_WIDTH_MASK) << STBLIT_S2SZ_WIDTH_SHIFT);
    S2SZ |= ((U32)(Rectangle_p->Height & STBLIT_S2SZ_HEIGHT_MASK) << STBLIT_S2SZ_HEIGHT_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2SZ)),S2SZ);

    return(Err);
 }

/******************************************************************************
Name        : SetNodeSrc2MaskXYL
Description : Set Clipmask Src2 in XYL mode
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
               Only TopLeft->BottomRight scan  supported
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeSrc2MaskXYL(stblit_Node_t*          Node_p,
                                                    STGXOBJ_Bitmap_t*       MaskBitmap_p,
                                                    STGXOBJ_Rectangle_t*    MaskRectangle_p,
                                                    stblit_Device_t*        Device_p,
                                                    stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             ColorType = 0;
    U32             Tmp;
    U32             S2TY, S2XY;
    U8              AlphaRange = 0;

    /* Src2 Instruction */
    CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_MEMORY & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);

    /* Src2 Base Address */
    #ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)MaskBitmap_p->Data1_p) + MaskBitmap_p->Offset);
    #else
        Tmp =  (U32) ((U32)STAVMEM_VirtualToDevice((void*)MaskBitmap_p->Data1_p, &(Device_p->VirtualMapping)) + MaskBitmap_p->Offset);
    #endif
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT));

    /* Src2 Type :
     * + Default for RGB Expansion is : Missing LSBs are filled with 0) */
    FormatColorType(MaskBitmap_p->ColorType,&ColorType,&AlphaRange,MaskBitmap_p->BitmapType) ; /* Prepare color Type HW representation*/
    S2TY = ((U32)(MaskBitmap_p->Pitch & STBLIT_S2TY_PITCH_MASK) << STBLIT_S2TY_PITCH_SHIFT );
    S2TY |= ((U32)(ColorType & STBLIT_S2TY_COLOR_MASK) << STBLIT_S2TY_COLOR_SHIFT);
    S2TY |= ((U32)(MaskBitmap_p->SubByteFormat & STBLIT_S2TY_SUB_BYTE_MASK) << STBLIT_S2TY_SUB_BYTE_SHIFT);
    S2TY |= ((U32)(1 & STBLIT_S2TY_RGB_EXPANSION_MASK) << STBLIT_S2TY_RGB_EXPANSION_SHIFT);
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        S2TY |=  ((U32)(AlphaRange & STBLIT_S2TY_ALPHA_RANGE_MASK) << STBLIT_S2TY_ALPHA_RANGE_SHIFT);
        if (MaskBitmap_p->BigNotLittle == TRUE)
        {
            S2TY |=  ((U32)(1 & STBLIT_S2TY_BIG_NOT_LITTLE_MASK) << STBLIT_S2TY_BIG_NOT_LITTLE_SHIFT);
        }
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)),S2TY);


    /* Src2 XY location (TopLeft->BottomRight scan)*/
    S2XY =  ((U32)(MaskRectangle_p->PositionY & STBLIT_S2XY_Y_MASK) << STBLIT_S2XY_Y_SHIFT);
    S2XY |= ((U32)(MaskRectangle_p->PositionX & STBLIT_S2XY_X_MASK) << STBLIT_S2XY_X_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2XY)),S2XY);





    return(Err);
 }




 /*******************************************************************************
Name        : SetNodeSrc2
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeSrc2(stblit_Node_t*          Node_p,
                                            STBLIT_Source_t*        Src_p,
                                            STGXOBJ_Rectangle_t*    DstRectangle_p,
                                            U8                      ScanConfig,
                                            stblit_Device_t*        Device_p,
                                            stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t7 = STOS_time_now();
    #endif

    if (Src_p == NULL)   /* Src disabled */
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_DISABLED & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);
    }
    else
    {
        if (Src_p->Type == STBLIT_SOURCE_TYPE_COLOR)  /* Fill rectangle */
        {
            SetNodeSrc2Color(Node_p,Src_p->Data.Color_p,DstRectangle_p,Device_p, CommonField_p);
        }
        else  /* Copy Rectangle */
        {
            SetNodeSrc2Bitmap(Node_p, Src_p->Data.Bitmap_p, &(Src_p->Rectangle), ScanConfig,Device_p, CommonField_p);
        }
    }
    #ifdef STBLIT_BENCHMARK3
    t8 = STOS_time_now();
    #endif

    return(Err);
 }


/*******************************************************************************
Name        : SetNodeTgt
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeTgt (stblit_Node_t*          Node_p,
                                             STGXOBJ_Bitmap_t*       Bitmap_p,
                                             STGXOBJ_Rectangle_t*    Rectangle_p,
                                             U8                      ScanConfig,
                                             stblit_Device_t*        Device_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             ColorType = 0;
    U32             Tmp;
    U32             TTY, TXY, S1SZ;
    U8              AlphaRange = 0;

    #ifdef STBLIT_BENCHMARK3
    t9 = STOS_time_now();
    #endif

#ifdef ST_OSLINUX
#ifdef BLIT_DEBUG
    PrintTrace(("Address Bitmap =0x%x, Bitmap Offset=0x%x \n", (U32)Bitmap_p->Data1_p, (U32)Bitmap_p->Offset ));
#endif
#endif
    /* Target Base Address */
    #ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
    #else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
    #endif
#ifdef ST_OSLINUX
#ifdef BLIT_DEBUG
    PrintTrace(("New Address Bitmap =0x%x\n", (U32)Tmp ));
#endif
#endif
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TBA)),((U32)(Tmp & STBLIT_TBA_POINTER_MASK) << STBLIT_TBA_POINTER_SHIFT));

    /* Target type */
    FormatColorType(Bitmap_p->ColorType,&ColorType,&AlphaRange,Bitmap_p->BitmapType) ; /* Prepare color Type HW representation*/
    TTY = ((U32)(Bitmap_p->Pitch & STBLIT_TTY_PITCH_MASK) << STBLIT_TTY_PITCH_SHIFT );
    TTY |= ((U32)(ColorType & STBLIT_TTY_COLOR_MASK) << STBLIT_TTY_COLOR_SHIFT );
    TTY |= ((U32)(ScanConfig & STBLIT_TTY_SCAN_ORDER_MASK) << STBLIT_TTY_SCAN_ORDER_SHIFT );
    TTY |= ((U32)(Bitmap_p->SubByteFormat & STBLIT_TTY_SUB_BYTE_MASK) << STBLIT_TTY_SUB_BYTE_SHIFT);
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        TTY |=  ((U32)(AlphaRange & STBLIT_TTY_ALPHA_RANGE_MASK) << STBLIT_TTY_ALPHA_RANGE_SHIFT);
        if (Bitmap_p->BigNotLittle == TRUE)
        {
            TTY |=  ((U32)(1 & STBLIT_TTY_BIG_NOT_LITTLE_MASK) << STBLIT_TTY_BIG_NOT_LITTLE_SHIFT);
        }
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TTY)),TTY);

    /* Target XY location. It depends on the programmed horizontal and vertical scan direction */
    if (((ScanConfig & 0x2) >> 1) == 0)
    {
        TXY = ((U32)(Rectangle_p->PositionY & STBLIT_TXY_Y_MASK) << STBLIT_TXY_Y_SHIFT);
    }
    else
    {
        TXY = ((U32)((Rectangle_p->PositionY + Rectangle_p->Height - 1) & STBLIT_TXY_Y_MASK) << STBLIT_TXY_Y_SHIFT);
    }

    if ((ScanConfig & 0x1) == 0)
    {
        TXY |= ((U32)(Rectangle_p->PositionX & STBLIT_TXY_X_MASK) << STBLIT_TXY_X_SHIFT);
    }
    else
    {
        TXY |= ((U32)((Rectangle_p->PositionX + Rectangle_p->Width - 1) & STBLIT_TXY_X_MASK) << STBLIT_TXY_X_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TXY)),TXY);

    /* Target and Src1 size */
    S1SZ = ((U32)(Rectangle_p->Width & STBLIT_S1SZ_WIDTH_MASK) << STBLIT_S1SZ_WIDTH_SHIFT);
    S1SZ |= ((U32)(Rectangle_p->Height & STBLIT_S1SZ_HEIGHT_MASK) << STBLIT_S1SZ_HEIGHT_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1SZ)),S1SZ);

    #ifdef STBLIT_BENCHMARK3
    t10 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeTgtXYL
Description : Configure target when XYL mode
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeTgtXYL (stblit_Node_t*          Node_p,
                                                STGXOBJ_Bitmap_t*       Bitmap_p,
                                                stblit_Device_t*        Device_p,
                                                stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             ColorType = 0;
    U32             Tmp;
    U32             TTY;
    U8              AlphaRange = 0;

	/* To remove comipation warnning */
	UNUSED_PARAMETER(CommonField_p);

    /* Target Base Address */
    #ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
    #else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
    #endif
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TBA)),((U32)(Tmp & STBLIT_TBA_POINTER_MASK) << STBLIT_TBA_POINTER_SHIFT));

    /* Target type */
    FormatColorType(Bitmap_p->ColorType,&ColorType,&AlphaRange,Bitmap_p->BitmapType) ; /* Prepare color Type HW representation*/
    TTY = ((U32)(Bitmap_p->Pitch & STBLIT_TTY_PITCH_MASK) << STBLIT_TTY_PITCH_SHIFT );
    TTY |= ((U32)(ColorType & STBLIT_TTY_COLOR_MASK) << STBLIT_TTY_COLOR_SHIFT );
    TTY |= ((U32)(Bitmap_p->SubByteFormat & STBLIT_TTY_SUB_BYTE_MASK) << STBLIT_TTY_SUB_BYTE_SHIFT);
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        TTY |=  ((U32)(AlphaRange & STBLIT_TTY_ALPHA_RANGE_MASK) << STBLIT_TTY_ALPHA_RANGE_SHIFT);
        if (Bitmap_p->BigNotLittle == TRUE)
        {
            TTY |=  ((U32)(1 & STBLIT_TTY_BIG_NOT_LITTLE_MASK) << STBLIT_TTY_BIG_NOT_LITTLE_SHIFT);
        }
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TTY)),TTY);

    return(Err);
 }



/******************************************************************************
Name        : UpdateNodeSrc1Bitmap
Description : Src1 is a bitmap
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
               Concern always Data1_p  whatever mode (raster, MB)
               Only Data and Pitch updated and also Endianness (if 7020 device)
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t UpdateNodeSrc1Bitmap(stblit_Node_t*          Node_p,
                                                      STGXOBJ_Bitmap_t*       Bitmap_p,
                                                      stblit_Device_t*        Device_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Tmp;
    U32             S1TY;

    /* Reset to Zero Pitch field and update it */
    S1TY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1TY)));
    S1TY &=  ((U32)(~((U32)(STBLIT_S1TY_PITCH_MASK) << STBLIT_S1TY_PITCH_SHIFT)));
    S1TY |=  ((U32)(Bitmap_p->Pitch & STBLIT_S1TY_PITCH_MASK) << STBLIT_S1TY_PITCH_SHIFT);

    /* Reset Endianness info and update it */
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        S1TY &=  ((U32)(~((U32)(STBLIT_S1TY_BIG_NOT_LITTLE_MASK) << STBLIT_S1TY_BIG_NOT_LITTLE_SHIFT)));
        if (Bitmap_p->BigNotLittle == TRUE)
        {
            S1TY |=  ((U32)(1 & STBLIT_S1TY_BIG_NOT_LITTLE_MASK) << STBLIT_S1TY_BIG_NOT_LITTLE_SHIFT);
        }
    }

    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1TY)),S1TY);

    /* Set new Base Address */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
#else
    Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1BA)),((U32)(Tmp & STBLIT_S1BA_POINTER_MASK) << STBLIT_S1BA_POINTER_SHIFT ));

    return(Err);
 }


/******************************************************************************
Name        : UpdateNodeSrc1Color
Description : Src1 is a color
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized.
               Only color value updated
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t UpdateNodeSrc1Color(stblit_Node_t*          Node_p,
                                                     STGXOBJ_Color_t*        Color_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Color;

    /*Src1 Color Fill */
    stblit_FormatColorForInputFormatter(Color_p,&Color) ; /* Prepare color HW representation*/
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1CF)),((U32)(Color & STBLIT_S1CF_COLOR_MASK) << STBLIT_S1CF_COLOR_SHIFT));

    return(Err);
 }

 /******************************************************************************
Name        : UpdateNodeSrc1Rectangle
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t UpdateNodeSrc1Rectangle(stblit_Node_t*          Node_p,
                                                         STGXOBJ_Rectangle_t*    Rectangle_p,
                                                         U8                      ScanConfig,
                                                         BOOL                    SrcMB)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             S1TY, S1XY, S1SZ;

    /* Reset Scan order field and update */
    S1TY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1TY)));
    S1TY &= ((U32)(~((U32)(STBLIT_S1TY_SCAN_ORDER_MASK) << STBLIT_S1TY_SCAN_ORDER_SHIFT)));
    S1TY |= ((U32)(ScanConfig & STBLIT_S1TY_SCAN_ORDER_MASK) << STBLIT_S1TY_SCAN_ORDER_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1TY)),S1TY);

    /* Src1 XY location. It depends on the programmed horizontal and vertical scan direction */
    if (((ScanConfig & 0x2) >> 1) == 0)
    {
        S1XY = ((U32)(Rectangle_p->PositionY & STBLIT_S1XY_Y_MASK) << STBLIT_S1XY_Y_SHIFT);
    }
    else
    {
        S1XY = ((U32)((Rectangle_p->PositionY + Rectangle_p->Height - 1) & STBLIT_S1XY_Y_MASK) << STBLIT_S1XY_Y_SHIFT);
    }

    if ((ScanConfig & 0x1) == 0)
    {
        S1XY |= ((U32)(Rectangle_p->PositionX & STBLIT_S1XY_X_MASK) << STBLIT_S1XY_X_SHIFT);
    }
    else
    {
        S1XY |= ((U32)((Rectangle_p->PositionX + Rectangle_p->Width - 1) & STBLIT_S1XY_X_MASK) << STBLIT_S1XY_X_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1XY)),S1XY);

    /* In case of Raster mode (normal, direct copy, direct fill), src1 size is the same as the target.
     * In case of MB mode, there is no size for src1 to program. The size of the MB bitmap is programmed in Src2.
     * Note that the size programmed in S1SZ correspond to Target. */
    if (SrcMB == FALSE)
    {
        S1SZ = ((U32)(Rectangle_p->Width & STBLIT_S1SZ_WIDTH_MASK) << STBLIT_S1SZ_WIDTH_SHIFT);
        S1SZ |= ((U32)(Rectangle_p->Height & STBLIT_S1SZ_HEIGHT_MASK) << STBLIT_S1SZ_HEIGHT_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1SZ)),S1SZ);
    }


    return(Err);
 }


/******************************************************************************
Name        : UpdateNodeSrc2Bitmap
Description : Src1 is a bitmap
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
               Only Data and Pitch updated and also Endianness (if 7020 device)
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t UpdateNodeSrc2Bitmap(stblit_Node_t*          Node_p,
                                                      STGXOBJ_Bitmap_t*       Bitmap_p,
                                                      stblit_Device_t*        Device_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Tmp;
    U32             S2TY;

    /* Reset Pitch field and update it */
    S2TY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)));
    S2TY &= ((U32)(~((U32)(STBLIT_S2TY_PITCH_MASK) << STBLIT_S2TY_PITCH_SHIFT)));
    S2TY |= ((U32)(Bitmap_p->Pitch & STBLIT_S2TY_PITCH_MASK) << STBLIT_S2TY_PITCH_SHIFT);

    /* Reset Endianness info and update it */
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        S2TY &=  ((U32)(~((U32)(STBLIT_S2TY_BIG_NOT_LITTLE_MASK) << STBLIT_S2TY_BIG_NOT_LITTLE_SHIFT)));
        if (Bitmap_p->BigNotLittle == TRUE)
        {
            S2TY |=  ((U32)(1 & STBLIT_S2TY_BIG_NOT_LITTLE_MASK) << STBLIT_S2TY_BIG_NOT_LITTLE_SHIFT);
        }
    }

    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)),S2TY);

    /* Set new Base Address*/
    if (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
    {
    #ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
    #else

        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
    #endif
    }
    else  /* STGXOBJ_BITMAP_TYPE_MB */
    {
    #ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data2_p) + Bitmap_p->Offset);
    #else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data2_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
    #endif
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));

    return(Err);
 }

/*******************************************************************************
Name        : UpdateNodeSrc2Color
Description : Src2 is a color
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
               Only color value updated!
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t UpdateNodeSrc2Color(stblit_Node_t*         Node_p,
                                                    STGXOBJ_Color_t*        Color_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Color;

    /*Src2 Color Fill */
    stblit_FormatColorForInputFormatter(Color_p,&Color) ; /* Prepare color HW representation*/
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2CF)),((U32)(Color & STBLIT_S2CF_COLOR_MASK) << STBLIT_S2CF_COLOR_SHIFT));

    return(Err);
 }

/*******************************************************************************
Name        : UpdateNodeSrc2Rectangle
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t UpdateNodeSrc2Rectangle(stblit_Node_t*         Node_p,
                                                       STGXOBJ_Rectangle_t*   Rectangle_p,
                                                       U8                     ScanConfig)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             S2TY, S2XY, S2SZ;

    /* Reset Scan order field and update */
    S2TY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)));
    S2TY &= ((U32)(~((U32)(STBLIT_S2TY_SCAN_ORDER_MASK) << STBLIT_S2TY_SCAN_ORDER_SHIFT)));
    S2TY |= ((U32)(ScanConfig & STBLIT_S2TY_SCAN_ORDER_MASK) << STBLIT_S2TY_SCAN_ORDER_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)),S2TY);

    /* Src2 XY location. It depends on the programmed horizontal and vertical scan direction */
    if (((ScanConfig & 0x2) >> 1) == 0)
    {
        S2XY = ((U32)(Rectangle_p->PositionY & STBLIT_S2XY_Y_MASK) << STBLIT_S2XY_Y_SHIFT);
    }
    else
    {
        S2XY = ((U32)((Rectangle_p->PositionY + Rectangle_p->Height - 1) & STBLIT_S2XY_Y_MASK) << STBLIT_S2XY_Y_SHIFT);
    }

    if ((ScanConfig & 0x1) == 0)
    {
        S2XY |= ((U32)(Rectangle_p->PositionX & STBLIT_S2XY_X_MASK) << STBLIT_S2XY_X_SHIFT);
    }
    else
    {
        S2XY |= ((U32)((Rectangle_p->PositionX + Rectangle_p->Width - 1) & STBLIT_S2XY_X_MASK) << STBLIT_S2XY_X_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2XY)),S2XY);


    /* Src2 Size */
    S2SZ = ((U32)(Rectangle_p->Width & STBLIT_S2SZ_WIDTH_MASK) << STBLIT_S2SZ_WIDTH_SHIFT);
    S2SZ |= ((U32)(Rectangle_p->Height & STBLIT_S2SZ_HEIGHT_MASK) << STBLIT_S2SZ_HEIGHT_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2SZ)),S2SZ);

    return(Err);
 }


/*******************************************************************************
Name        : UpdateNodeTgtBitmap
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node Not initialized.
               Only Data and Pitch updated and also endianness (if 7020 device)
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t UpdateNodeTgtBitmap (stblit_Node_t*          Node_p,
                                                     STGXOBJ_Bitmap_t*       Bitmap_p,
                                                     stblit_Device_t*        Device_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Tmp;
    U32             TTY;


    /* Reset Pitch field and update it */
    TTY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_TTY)));
    TTY &= ((U32)(~((U32)(STBLIT_TTY_PITCH_MASK) << STBLIT_TTY_PITCH_SHIFT)));
    TTY |= ((U32)(Bitmap_p->Pitch & STBLIT_TTY_PITCH_MASK) << STBLIT_TTY_PITCH_SHIFT);

    /* Reset Endianness info and update it */
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528))
    {
        TTY &=  ((U32)(~((U32)(STBLIT_TTY_BIG_NOT_LITTLE_MASK) << STBLIT_TTY_BIG_NOT_LITTLE_SHIFT)));
        if (Bitmap_p->BigNotLittle == TRUE)
        {
            TTY |=  ((U32)(1 & STBLIT_TTY_BIG_NOT_LITTLE_MASK) << STBLIT_TTY_BIG_NOT_LITTLE_SHIFT);
        }
    }

    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TTY)),TTY);

    /* Set new Base Address */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
#else
    Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TBA)),((U32)(Tmp & STBLIT_TBA_POINTER_MASK) << STBLIT_TBA_POINTER_SHIFT ));


    return(Err);
 }

 /*******************************************************************************
Name        : UpdateNodeTgtRectangle
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t UpdateNodeTgtRectangle (stblit_Node_t*          Node_p,
                                                         STGXOBJ_Rectangle_t*    Rectangle_p,
                                                         U8                      ScanConfig)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             TTY, TXY, S1SZ;

    /* Reset Scan order field and update */
    TTY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_TTY)));
    TTY &= ((U32)(~((U32)(STBLIT_TTY_SCAN_ORDER_MASK) << STBLIT_TTY_SCAN_ORDER_SHIFT)));
    TTY |= ((U32)(ScanConfig & STBLIT_TTY_SCAN_ORDER_MASK) << STBLIT_TTY_SCAN_ORDER_SHIFT );
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TTY)),TTY);

    /* Target XY location. It depends on the programmed horizontal and vertical scan direction */
    if (((ScanConfig & 0x2) >> 1) == 0)
    {
        TXY = ((U32)(Rectangle_p->PositionY & STBLIT_TXY_Y_MASK) << STBLIT_TXY_Y_SHIFT);
    }
    else
    {
        TXY = ((U32)((Rectangle_p->PositionY + Rectangle_p->Height - 1) & STBLIT_TXY_Y_MASK) << STBLIT_TXY_Y_SHIFT);
    }

    if ((ScanConfig & 0x1) == 0)
    {
        TXY |= ((U32)(Rectangle_p->PositionX & STBLIT_TXY_X_MASK) << STBLIT_TXY_X_SHIFT);
    }
    else
    {
        TXY |= ((U32)((Rectangle_p->PositionX + Rectangle_p->Width - 1) & STBLIT_TXY_X_MASK) << STBLIT_TXY_X_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TXY)),TXY);

    /* Target and Src1 size */
    S1SZ = ((U32)(Rectangle_p->Width & STBLIT_S1SZ_WIDTH_MASK) << STBLIT_S1SZ_WIDTH_SHIFT);
    S1SZ |= ((U32)(Rectangle_p->Height & STBLIT_S1SZ_HEIGHT_MASK) << STBLIT_S1SZ_HEIGHT_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1SZ)),S1SZ);

    return(Err);
 }




/*******************************************************************************
Name        : SetNodeClipping
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node Not initialized !
               All parameters are unsigned !
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeClipping (stblit_Node_t*         Node_p,
                                                  STGXOBJ_Rectangle_t*   ClipRectangle_p,
                                                  BOOL                   WriteInsideClipRectangle,
                                                  stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             CWO, CWS;

    /* Instruction : Reset and Enable clipping rect */
    CommonField_p->INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_RECT_CLIPPING_MASK) << STBLIT_INS_ENABLE_RECT_CLIPPING_SHIFT)));
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_RECT_CLIPPING_MASK) << STBLIT_INS_ENABLE_RECT_CLIPPING_SHIFT);

    /* Clipping window start / If Forbidden to write inside the clipping window */
    CWO = ((U32)(ClipRectangle_p->PositionX & STBLIT_CWO_XDO_MASK) << STBLIT_CWO_XDO_SHIFT);
    CWO |= ((U32)(ClipRectangle_p->PositionY & STBLIT_CWO_YDO_MASK) << STBLIT_CWO_YDO_SHIFT);
    if (WriteInsideClipRectangle == FALSE)
    {
        CWO |= ((U32)(1 & STBLIT_CWO_INT_NOT_EXT_MASK) << STBLIT_CWO_INT_NOT_EXT_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CWO)),CWO);

    /* Clipping Window Stop */
    CWS = ((U32)((ClipRectangle_p->PositionX + ClipRectangle_p->Width - 1) & STBLIT_CWS_XDS_MASK) << STBLIT_CWS_XDS_SHIFT);
    CWS |= ((U32)((ClipRectangle_p->PositionY + ClipRectangle_p->Height - 1) & STBLIT_CWS_YDS_MASK) << STBLIT_CWS_YDS_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CWS)),CWS);

    return(Err);
 }

 /*******************************************************************************
Name        : UpdateNodeClipping
Description : As SetClipping but without CommonField parameter
Parameters  :
Assumptions :  All parameters checks done at upper level. Node Not initialized !
               All parameters are unsigned !
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t UpdateNodeClipping (stblit_Node_t*         Node_p,
                                                  STGXOBJ_Rectangle_t*      ClipRectangle_p,
                                                  BOOL                      WriteInsideClipRectangle)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             CWO, CWS, INS;

    /* Instruction : Reset and Enable clipping rect */
    INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)));
    INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_RECT_CLIPPING_MASK) << STBLIT_INS_ENABLE_RECT_CLIPPING_SHIFT)));
    INS |= ((U32)(1 & STBLIT_INS_ENABLE_RECT_CLIPPING_MASK) << STBLIT_INS_ENABLE_RECT_CLIPPING_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)),INS);

    /* Clipping window start / If Forbidden to write inside the clipping window */
    CWO = ((U32)(ClipRectangle_p->PositionX & STBLIT_CWO_XDO_MASK) << STBLIT_CWO_XDO_SHIFT);
    CWO |= ((U32)(ClipRectangle_p->PositionY & STBLIT_CWO_YDO_MASK) << STBLIT_CWO_YDO_SHIFT);
    if (WriteInsideClipRectangle == FALSE)
    {
        CWO |= ((U32)(1 & STBLIT_CWO_INT_NOT_EXT_MASK) << STBLIT_CWO_INT_NOT_EXT_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CWO)),CWO);

    /* Clipping Window Stop */
    CWS = ((U32)((ClipRectangle_p->PositionX + ClipRectangle_p->Width - 1) & STBLIT_CWS_XDS_MASK) << STBLIT_CWS_XDS_SHIFT);
    CWS |= ((U32)((ClipRectangle_p->PositionY + ClipRectangle_p->Height - 1) & STBLIT_CWS_YDS_MASK) << STBLIT_CWS_YDS_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CWS)),CWS);

    return(Err);
 }


/*******************************************************************************
Name        : SetNodeColorKey
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node Not initialized
               There is color key : ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE
               ColorKey_p points to an allocated Color key
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeColorKey (stblit_Node_t*              Node_p,
                                                 STBLIT_ColorKeyCopyMode_t   ColorKeyCopyMode,
                                                 STGXOBJ_ColorKey_t*         ColorKey_p,
                                                 stblit_CommonField_t*       CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             KEY1, KEY2;

    #ifdef STBLIT_BENCHMARK3
    t13 = STOS_time_now();
    #endif

    /* Reset concerned fields of registers wich are not fully overwritten in this function
     * (Note that KEY1 and KEY2 registers are not concerned since they are fully overwritten)*/
    CommonField_p->INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_COLOR_KEY_MASK) << STBLIT_INS_ENABLE_COLOR_KEY_SHIFT)));
    CommonField_p->ACK &= ((U32)(~((U32)(STBLIT_ACK_COLOR_KEY_IN_SELECT_MASK) << STBLIT_ACK_COLOR_KEY_IN_SELECT_SHIFT)));
    CommonField_p->ACK &= ((U32)(~((U32)(STBLIT_ACK_COLOR_KEY_CFG_MASK) << STBLIT_ACK_COLOR_KEY_CFG_SHIFT)));

    if (ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST)
    {
        /* Instruction : Enable color Key */
        CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_COLOR_KEY_MASK) << STBLIT_INS_ENABLE_COLOR_KEY_SHIFT);
        /* Color key input selection */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IN_SELECT_DST & STBLIT_ACK_COLOR_KEY_IN_SELECT_MASK) << STBLIT_ACK_COLOR_KEY_IN_SELECT_SHIFT);
    }
    else if (ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC)
    {
        /* Instruction : Enable color Key */
        CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_COLOR_KEY_MASK) << STBLIT_INS_ENABLE_COLOR_KEY_SHIFT);
        /* Color key input selection (always before CLIUT ? )*/
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IN_SELECT_SRC_BEFORE & STBLIT_ACK_COLOR_KEY_IN_SELECT_MASK) << STBLIT_ACK_COLOR_KEY_IN_SELECT_SHIFT);
    }

    if (ColorKey_p->Type == STGXOBJ_COLOR_KEY_TYPE_CLUT8)
    {
        /* Color Key control */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);

        if ((ColorKey_p->Value.CLUT8.PaletteEntryEnable == TRUE) &&
            (ColorKey_p->Value.CLUT8.PaletteEntryOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.CLUT8.PaletteEntryEnable == TRUE) &&
                 (ColorKey_p->Value.CLUT8.PaletteEntryOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),((U32)(ColorKey_p->Value.CLUT8.PaletteEntryMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT));
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),((U32)(ColorKey_p->Value.CLUT8.PaletteEntryMax & STBLIT_KEY2_BLUE_MASK) << STBLIT_KEY2_BLUE_SHIFT));

    }
    else if (ColorKey_p->Type == STGXOBJ_COLOR_KEY_TYPE_CLUT1)
    {
        /* Color Key control */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);

        if ((ColorKey_p->Value.CLUT1.PaletteEntryEnable == TRUE) &&
            (ColorKey_p->Value.CLUT1.PaletteEntryOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.CLUT1.PaletteEntryEnable == TRUE) &&
                 (ColorKey_p->Value.CLUT1.PaletteEntryOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),((U32)(ColorKey_p->Value.CLUT1.PaletteEntryMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT));
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),((U32)(ColorKey_p->Value.CLUT1.PaletteEntryMax & STBLIT_KEY2_BLUE_MASK) << STBLIT_KEY2_BLUE_SHIFT));
    }
    else if (ColorKey_p->Type == STGXOBJ_COLOR_KEY_TYPE_RGB565)
    {
        if ((ColorKey_p->Value.RGB565.REnable == TRUE) &&
            (ColorKey_p->Value.RGB565.ROut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else if ((ColorKey_p->Value.RGB565.REnable == TRUE) &&
                 (ColorKey_p->Value.RGB565.ROut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else    /* Red ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }

        if ((ColorKey_p->Value.RGB565.GEnable == TRUE) &&
            (ColorKey_p->Value.RGB565.GOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else if ((ColorKey_p->Value.RGB565.GEnable == TRUE) &&
                 (ColorKey_p->Value.RGB565.GOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else    /* Green ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }

        if ((ColorKey_p->Value.RGB565.BEnable == TRUE) &&
            (ColorKey_p->Value.RGB565.BOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.RGB565.BEnable == TRUE) &&
                 (ColorKey_p->Value.RGB565.BOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value : Color key value has to follow the internal bus format */
        KEY1 = (((ColorKey_p->Value.RGB565.RMin & 0x1F) << 19) |
                ((ColorKey_p->Value.RGB565.GMin & 0x3F) << 10) |
                ((ColorKey_p->Value.RGB565.BMin & 0x1F) << 3));
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),KEY1);

        KEY2 = (((ColorKey_p->Value.RGB565.RMax & 0x1F) << 19) |
                ((ColorKey_p->Value.RGB565.GMax & 0x3F) << 10) |
                ((ColorKey_p->Value.RGB565.BMax & 0x1F) << 3));
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),KEY2);
    }
    else if (ColorKey_p->Type == STGXOBJ_COLOR_KEY_TYPE_RGB888)
    {

        if ((ColorKey_p->Value.RGB888.REnable == TRUE) &&
            (ColorKey_p->Value.RGB888.ROut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else if ((ColorKey_p->Value.RGB888.REnable == TRUE) &&
                 (ColorKey_p->Value.RGB888.ROut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else    /* Red ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }

        if ((ColorKey_p->Value.RGB888.GEnable == TRUE) &&
            (ColorKey_p->Value.RGB888.GOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else if ((ColorKey_p->Value.RGB888.GEnable == TRUE) &&
                 (ColorKey_p->Value.RGB888.GOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else    /* Green ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }

        if ((ColorKey_p->Value.RGB888.BEnable == TRUE) &&
            (ColorKey_p->Value.RGB888.BOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.RGB888.BEnable == TRUE) &&
                 (ColorKey_p->Value.RGB888.BOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        KEY1 = ((U32)(ColorKey_p->Value.RGB888.RMin & STBLIT_KEY1_RED_MASK) << STBLIT_KEY1_RED_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.RGB888.GMin & STBLIT_KEY1_GREEN_MASK) << STBLIT_KEY1_GREEN_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.RGB888.BMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),KEY1);

        KEY2 = ((U32)(ColorKey_p->Value.RGB888.RMax & STBLIT_KEY2_RED_MASK) << STBLIT_KEY2_RED_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.RGB888.GMax & STBLIT_KEY2_GREEN_MASK) << STBLIT_KEY2_GREEN_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.RGB888.BMax & STBLIT_KEY2_BLUE_MASK) << STBLIT_KEY2_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),KEY2);
    }
    else if (ColorKey_p->Type == STGXOBJ_COLOR_KEY_TYPE_YCbCr888_UNSIGNED)
    {
        if ((ColorKey_p->Value.UnsignedYCbCr888.CrEnable == TRUE) &&
            (ColorKey_p->Value.UnsignedYCbCr888.CrOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else if ((ColorKey_p->Value.UnsignedYCbCr888.CrEnable == TRUE) &&
                 (ColorKey_p->Value.UnsignedYCbCr888.CrOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else    /* Red ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }

        if ((ColorKey_p->Value.UnsignedYCbCr888.YEnable == TRUE) &&
            (ColorKey_p->Value.UnsignedYCbCr888.YOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else if ((ColorKey_p->Value.UnsignedYCbCr888.YEnable == TRUE) &&
                 (ColorKey_p->Value.UnsignedYCbCr888.YOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else    /* Green ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }

        if ((ColorKey_p->Value.UnsignedYCbCr888.CbEnable == TRUE) &&
            (ColorKey_p->Value.UnsignedYCbCr888.CbOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.UnsignedYCbCr888.CbEnable == TRUE) &&
                 (ColorKey_p->Value.UnsignedYCbCr888.CbOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        KEY1 = ((U32)(ColorKey_p->Value.UnsignedYCbCr888.CrMin & STBLIT_KEY1_RED_MASK) << STBLIT_KEY1_RED_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.UnsignedYCbCr888.YMin & STBLIT_KEY1_GREEN_MASK) << STBLIT_KEY1_GREEN_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.UnsignedYCbCr888.CbMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),KEY1);

        KEY2 = ((U32)(ColorKey_p->Value.UnsignedYCbCr888.CrMax & STBLIT_KEY2_RED_MASK) << STBLIT_KEY2_RED_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.UnsignedYCbCr888.YMax & STBLIT_KEY2_GREEN_MASK) << STBLIT_KEY2_GREEN_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.UnsignedYCbCr888.CbMax & STBLIT_KEY2_BLUE_MASK)  << STBLIT_KEY2_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),KEY2);
    }
    else    /* STGXOBJ_COLOR_KEY_TYPE_YCbCr888_SIGNED  */
    {
         if ((ColorKey_p->Value.SignedYCbCr888.CrEnable == TRUE) &&
            (ColorKey_p->Value.SignedYCbCr888.CrOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else if ((ColorKey_p->Value.SignedYCbCr888.CrEnable == TRUE) &&
                 (ColorKey_p->Value.SignedYCbCr888.CrOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else    /* Red ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }

        if ((ColorKey_p->Value.SignedYCbCr888.YEnable == TRUE) &&
            (ColorKey_p->Value.SignedYCbCr888.YOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else if ((ColorKey_p->Value.SignedYCbCr888.YEnable == TRUE) &&
                 (ColorKey_p->Value.SignedYCbCr888.YOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else    /* Green ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }

        if ((ColorKey_p->Value.SignedYCbCr888.CbEnable == TRUE) &&
            (ColorKey_p->Value.SignedYCbCr888.CbOut == FALSE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.SignedYCbCr888.CbEnable == TRUE) &&
                 (ColorKey_p->Value.SignedYCbCr888.CbOut == TRUE))
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        KEY1 = ((U32)(ColorKey_p->Value.SignedYCbCr888.CrMin & STBLIT_KEY1_RED_MASK) << STBLIT_KEY1_RED_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.SignedYCbCr888.YMin & STBLIT_KEY1_GREEN_MASK) << STBLIT_KEY1_GREEN_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.SignedYCbCr888.CbMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),KEY1);

        KEY2 = ((U32)(ColorKey_p->Value.SignedYCbCr888.CrMax & STBLIT_KEY2_RED_MASK) << STBLIT_KEY2_RED_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.SignedYCbCr888.YMax & STBLIT_KEY2_GREEN_MASK) << STBLIT_KEY2_GREEN_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.SignedYCbCr888.CbMax & STBLIT_KEY2_BLUE_MASK) << STBLIT_KEY2_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),KEY2);
    }

    #ifdef STBLIT_BENCHMARK3
    t14 = STOS_time_now();
    #endif

    return(Err);
 }
 /*******************************************************************************
Name        : UpdateNodeColorKey
Description : As Set ColorKey but without CommonField
Parameters  :
Assumptions :  All parameters checks done at upper level. Node Not initialized
               There is color key : ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE
               ColorKey_p points to an allocated Color key
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t UpdateNodeColorKey (stblit_Node_t*              Node_p,
                                                    STBLIT_ColorKeyCopyMode_t   ColorKeyCopyMode,
                                                    STGXOBJ_ColorKey_t*         ColorKey_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             KEY1, KEY2, INS, ACK;

    #ifdef STBLIT_BENCHMARK3
    t13 = STOS_time_now();
    #endif

    /* Reset concerned fields of registers wich are not fully overwritten in this function
     * (Note that KEY1 and KEY2 registers are not concerned since they are fully overwritten)*/
    INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)));
    INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_COLOR_KEY_MASK) << STBLIT_INS_ENABLE_COLOR_KEY_SHIFT)));

    ACK = (U32)STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_ACK)));
    ACK &= ((U32)(~((U32)(STBLIT_ACK_COLOR_KEY_IN_SELECT_MASK) << STBLIT_ACK_COLOR_KEY_IN_SELECT_SHIFT)));
    ACK &= ((U32)(~((U32)(STBLIT_ACK_COLOR_KEY_CFG_MASK) << STBLIT_ACK_COLOR_KEY_CFG_SHIFT)));

    if (ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST)
    {
        /* Instruction : Enable color Key */
        INS |= ((U32)(1 & STBLIT_INS_ENABLE_COLOR_KEY_MASK) << STBLIT_INS_ENABLE_COLOR_KEY_SHIFT);
        /* Color key input selection */
        ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IN_SELECT_DST & STBLIT_ACK_COLOR_KEY_IN_SELECT_MASK) << STBLIT_ACK_COLOR_KEY_IN_SELECT_SHIFT);
    }
    else if (ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC)
    {
        /* Instruction : Enable color Key */
        INS |= ((U32)(1 & STBLIT_INS_ENABLE_COLOR_KEY_MASK) << STBLIT_INS_ENABLE_COLOR_KEY_SHIFT);
        /* Color key input selection (always before CLIUT ? )*/
        ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IN_SELECT_SRC_BEFORE & STBLIT_ACK_COLOR_KEY_IN_SELECT_MASK) << STBLIT_ACK_COLOR_KEY_IN_SELECT_SHIFT);
    }

    if (ColorKey_p->Type == STGXOBJ_COLOR_KEY_TYPE_CLUT8)
    {
        /* Color Key control */
        ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);

        if ((ColorKey_p->Value.CLUT8.PaletteEntryEnable == TRUE) &&
            (ColorKey_p->Value.CLUT8.PaletteEntryOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.CLUT8.PaletteEntryEnable == TRUE) &&
                 (ColorKey_p->Value.CLUT8.PaletteEntryOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),((U32)(ColorKey_p->Value.CLUT8.PaletteEntryMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT));
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),((U32)(ColorKey_p->Value.CLUT8.PaletteEntryMax & STBLIT_KEY2_BLUE_MASK) << STBLIT_KEY2_BLUE_SHIFT));

    }
    else if (ColorKey_p->Type == STGXOBJ_COLOR_KEY_TYPE_CLUT1)
    {
        /* Color Key control */
        ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);

        if ((ColorKey_p->Value.CLUT1.PaletteEntryEnable == TRUE) &&
            (ColorKey_p->Value.CLUT1.PaletteEntryOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.CLUT1.PaletteEntryEnable == TRUE) &&
                 (ColorKey_p->Value.CLUT1.PaletteEntryOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),((U32)(ColorKey_p->Value.CLUT1.PaletteEntryMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT));
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),((U32)(ColorKey_p->Value.CLUT1.PaletteEntryMax & STBLIT_KEY2_BLUE_MASK) << STBLIT_KEY2_BLUE_SHIFT));
    }

    else if (ColorKey_p->Type == STGXOBJ_COLOR_KEY_TYPE_RGB888)
    {

        if ((ColorKey_p->Value.RGB888.REnable == TRUE) &&
            (ColorKey_p->Value.RGB888.ROut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else if ((ColorKey_p->Value.RGB888.REnable == TRUE) &&
                 (ColorKey_p->Value.RGB888.ROut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else    /* Red ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }

        if ((ColorKey_p->Value.RGB888.GEnable == TRUE) &&
            (ColorKey_p->Value.RGB888.GOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else if ((ColorKey_p->Value.RGB888.GEnable == TRUE) &&
                 (ColorKey_p->Value.RGB888.GOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else    /* Green ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }

        if ((ColorKey_p->Value.RGB888.BEnable == TRUE) &&
            (ColorKey_p->Value.RGB888.BOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.RGB888.BEnable == TRUE) &&
                 (ColorKey_p->Value.RGB888.BOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        KEY1 = ((U32)(ColorKey_p->Value.RGB888.RMin & STBLIT_KEY1_RED_MASK) << STBLIT_KEY1_RED_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.RGB888.GMin & STBLIT_KEY1_GREEN_MASK) << STBLIT_KEY1_GREEN_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.RGB888.BMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),KEY1);

        KEY2 = ((U32)(ColorKey_p->Value.RGB888.RMax & STBLIT_KEY2_RED_MASK) << STBLIT_KEY2_RED_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.RGB888.GMax & STBLIT_KEY2_GREEN_MASK) << STBLIT_KEY2_GREEN_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.RGB888.BMax & STBLIT_KEY2_BLUE_MASK) << STBLIT_KEY2_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),KEY2);
    }
    else if (ColorKey_p->Type == STGXOBJ_COLOR_KEY_TYPE_YCbCr888_UNSIGNED)
    {
        if ((ColorKey_p->Value.UnsignedYCbCr888.CrEnable == TRUE) &&
            (ColorKey_p->Value.UnsignedYCbCr888.CrOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else if ((ColorKey_p->Value.UnsignedYCbCr888.CrEnable == TRUE) &&
                 (ColorKey_p->Value.UnsignedYCbCr888.CrOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else    /* Red ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }

        if ((ColorKey_p->Value.UnsignedYCbCr888.YEnable == TRUE) &&
            (ColorKey_p->Value.UnsignedYCbCr888.YOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else if ((ColorKey_p->Value.UnsignedYCbCr888.YEnable == TRUE) &&
                 (ColorKey_p->Value.UnsignedYCbCr888.YOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else    /* Green ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }

        if ((ColorKey_p->Value.UnsignedYCbCr888.CbEnable == TRUE) &&
            (ColorKey_p->Value.UnsignedYCbCr888.CbOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.UnsignedYCbCr888.CbEnable == TRUE) &&
                 (ColorKey_p->Value.UnsignedYCbCr888.CbOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        KEY1 = ((U32)(ColorKey_p->Value.UnsignedYCbCr888.CrMin & STBLIT_KEY1_RED_MASK) << STBLIT_KEY1_RED_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.UnsignedYCbCr888.YMin & STBLIT_KEY1_GREEN_MASK) << STBLIT_KEY1_GREEN_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.UnsignedYCbCr888.CbMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),KEY1);

        KEY2 = ((U32)(ColorKey_p->Value.UnsignedYCbCr888.CrMax & STBLIT_KEY2_RED_MASK) << STBLIT_KEY2_RED_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.UnsignedYCbCr888.YMax & STBLIT_KEY2_GREEN_MASK) << STBLIT_KEY2_GREEN_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.UnsignedYCbCr888.CbMax & STBLIT_KEY2_BLUE_MASK)  << STBLIT_KEY2_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),KEY2);
    }
    else    /* STGXOBJ_COLOR_KEY_TYPE_YCbCr888_SIGNED  */
    {
         if ((ColorKey_p->Value.SignedYCbCr888.CrEnable == TRUE) &&
            (ColorKey_p->Value.SignedYCbCr888.CrOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else if ((ColorKey_p->Value.SignedYCbCr888.CrEnable == TRUE) &&
                 (ColorKey_p->Value.SignedYCbCr888.CrOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }
        else    /* Red ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_RED_MASK) << STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT);
        }

        if ((ColorKey_p->Value.SignedYCbCr888.YEnable == TRUE) &&
            (ColorKey_p->Value.SignedYCbCr888.YOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else if ((ColorKey_p->Value.SignedYCbCr888.YEnable == TRUE) &&
                 (ColorKey_p->Value.SignedYCbCr888.YOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }
        else    /* Green ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK) << STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT);
        }

        if ((ColorKey_p->Value.SignedYCbCr888.CbEnable == TRUE) &&
            (ColorKey_p->Value.SignedYCbCr888.CbOut == FALSE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_INSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else if ((ColorKey_p->Value.SignedYCbCr888.CbEnable == TRUE) &&
                 (ColorKey_p->Value.SignedYCbCr888.CbOut == TRUE))
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_OUTSIDE & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }
        else    /* Blue ignored */
        {
            ACK |= ((U32)(STBLIT_ACK_COLOR_KEY_IGNORED & STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK) << STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT);
        }

        /* Set Color key value */
        KEY1 = ((U32)(ColorKey_p->Value.SignedYCbCr888.CrMin & STBLIT_KEY1_RED_MASK) << STBLIT_KEY1_RED_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.SignedYCbCr888.YMin & STBLIT_KEY1_GREEN_MASK) << STBLIT_KEY1_GREEN_SHIFT);
        KEY1 |= ((U32)(ColorKey_p->Value.SignedYCbCr888.CbMin & STBLIT_KEY1_BLUE_MASK) << STBLIT_KEY1_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY1)),KEY1);

        KEY2 = ((U32)(ColorKey_p->Value.SignedYCbCr888.CrMax & STBLIT_KEY2_RED_MASK) << STBLIT_KEY2_RED_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.SignedYCbCr888.YMax & STBLIT_KEY2_GREEN_MASK) << STBLIT_KEY2_GREEN_SHIFT);
        KEY2 |= ((U32)(ColorKey_p->Value.SignedYCbCr888.CbMax & STBLIT_KEY2_BLUE_MASK) << STBLIT_KEY2_BLUE_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_KEY2)),KEY2);
    }

    /* Write INS and ACK value */
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)),INS);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_ACK)),ACK);

    #ifdef STBLIT_BENCHMARK3
    t14 = STOS_time_now();
    #endif

    return(Err);
 }


/*******************************************************************************
Name        : SetNodePlaneMask
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized !
               All parameters are unsigned !
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodePlaneMask (stblit_Node_t*          Node_p,
                                                   U32                     MaskWord,
                                                   stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t15 = STOS_time_now();
    #endif

    /* Instruction : Reset and Enable Plane mask */
    CommonField_p->INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_PLANE_MASK) << STBLIT_INS_ENABLE_PLANE_SHIFT)));
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_PLANE_MASK) << STBLIT_INS_ENABLE_PLANE_SHIFT);

    /* Set Plane mask value */
    STSYS_WriteRegMem32LE((void*)(&( Node_p->HWNode.BLT_PMK)),((U32)(MaskWord & STBLIT_PMK_VALUE_MASK) << STBLIT_PMK_VALUE_SHIFT));

    #ifdef STBLIT_BENCHMARK3
    t16 = STOS_time_now();
    #endif

    return(Err);
 }

 /*******************************************************************************
Name        : UpdateNodePlaneMask
Description : As set Plane mask but without CommonField
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized !
               All parameters are unsigned !
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t UpdateNodePlaneMask (stblit_Node_t*          Node_p,
                                                      U32                     MaskWord)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             INS;

    #ifdef STBLIT_BENCHMARK3
    t15 = STOS_time_now();
    #endif

    /* Instruction : Reset and Enable Plane mask */
    INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)));
    INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_PLANE_MASK) << STBLIT_INS_ENABLE_PLANE_SHIFT)));
    INS |= ((U32)(1 & STBLIT_INS_ENABLE_PLANE_MASK) << STBLIT_INS_ENABLE_PLANE_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)),INS);

    /* Set Plane mask value */
    STSYS_WriteRegMem32LE((void*)(&( Node_p->HWNode.BLT_PMK)),((U32)(MaskWord & STBLIT_PMK_VALUE_MASK) << STBLIT_PMK_VALUE_SHIFT));

    #ifdef STBLIT_BENCHMARK3
    t16 = STOS_time_now();
    #endif

    return(Err);
 }


/*******************************************************************************
Name        : SetNodeTrigger
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized
               All parameters are unsigned !
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeTrigger (stblit_Node_t*          Node_p,
                                                STBLIT_BlitContext_t*   BlitContext_p,
                                                stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    U32                 RST;

    #ifdef STBLIT_BENCHMARK3
    t17 = STOS_time_now();
    #endif

    /* Instruction : Select VTG */
    CommonField_p->INS |= ((U32)(BlitContext_p->Trigger.VTGId & STBLIT_INS_TRIGGER_CONTROL_MASK) << STBLIT_INS_TRIGGER_CONTROL_SHIFT);

    /* Instruction : Select trigger conditions */
    if ((BlitContext_p->Trigger.Type == STBLIT_TRIGGER_TYPE_TOP_LINE) || (BlitContext_p->Trigger.Type == STBLIT_TRIGGER_TYPE_PROGRESSIVE_LINE))
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_TRIGGER_RASTER_TOP & STBLIT_INS_TRIGGER_CONTROL_MASK) << STBLIT_INS_TRIGGER_CONTROL_SHIFT);
    }
    else if (BlitContext_p->Trigger.Type == STBLIT_TRIGGER_TYPE_BOTTOM_LINE)
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_TRIGGER_RASTER_BOT & STBLIT_INS_TRIGGER_CONTROL_MASK) << STBLIT_INS_TRIGGER_CONTROL_SHIFT);
    }
    else if ((BlitContext_p->Trigger.Type == STBLIT_TRIGGER_TYPE_TOP_VSYNC) || (BlitContext_p->Trigger.Type == STBLIT_TRIGGER_TYPE_PROGRESSIVE_VSYNC))
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_TRIGGER_VSYNC_TOP & STBLIT_INS_TRIGGER_CONTROL_MASK) << STBLIT_INS_TRIGGER_CONTROL_SHIFT);
    }
    else if (BlitContext_p->Trigger.Type == STBLIT_TRIGGER_TYPE_BOTTOM_VSYNC)
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_TRIGGER_VSYNC_BOT & STBLIT_INS_TRIGGER_CONTROL_MASK) << STBLIT_INS_TRIGGER_CONTROL_SHIFT);
    }
    else if ((BlitContext_p->Trigger.Type == STBLIT_TRIGGER_TYPE_TOP_CAPTURE) || (BlitContext_p->Trigger.Type == STBLIT_TRIGGER_TYPE_PROGRESSIVE_CAPTURE))
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_TRIGGER_CAPTURE_TOP & STBLIT_INS_TRIGGER_CONTROL_MASK) << STBLIT_INS_TRIGGER_CONTROL_SHIFT);
    }
    else /*  BlitContext_p->Trigger.Type == STBLIT_TRIGGER_TYPE_BOTTOM_CAPTURE */
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_TRIGGER_CAPTURE_BOT & STBLIT_INS_TRIGGER_CONTROL_MASK) << STBLIT_INS_TRIGGER_CONTROL_SHIFT);
    }


    /* Raster scan trigger */
    RST = ((U32)(BlitContext_p->Trigger.ScanLine & STBLIT_RST_VIDEO_LINE_MASK) << STBLIT_RST_VIDEO_LINE_SHIFT);
    RST |= ((U32)(BlitContext_p->Trigger.VTGId & STBLIT_RST_VTG_MASK) << STBLIT_RST_VTG_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RST)),RST);

    #ifdef STBLIT_BENCHMARK3
    t18 = STOS_time_now();
    #endif

    return(Err);
 }

 /*******************************************************************************
Name        : SetNodeResize
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized.
               No slice management.
Limitations :  MB? TBD...
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeResize (stblit_Node_t*          Node_p,
                                                U32                     SrcWidth,
                                                U32                     SrcHeight,
                                                U32                     DstWidth,
                                                U32                     DstHeight,
                                                stblit_Device_t*        Device_p,
                                                U8                      FilterMode,
                                                stblit_CommonField_t*   CommonField_p
                                              #if defined (WA_GNBvd10454)
                                                , BOOL* HResizeOn,
                                                  BOOL* VResizeOn
                                              #endif
                                                 )
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             HIncrement;
    U32             VIncrement;
    U32             HSubposition  = 0;
    U32             VSubposition  = 0;
    U32             HOffset;
    U32             VOffset;
    U32             Tmp;
    U32             RSF = 0;

    #ifdef STBLIT_BENCHMARK3
    t19 = STOS_time_now();
    #endif

    /* scaling factor calculation*/
    if (DstWidth > 1)
    {
        HIncrement = (U32)(((SrcWidth - 1) * 1024) / (DstWidth - 1));
    }
    else
    {
		HIncrement = 0;
    }
    if (DstHeight > 1)
    {
        VIncrement = (U32)(((SrcHeight - 1) * 1024) / (DstHeight - 1));
    }
    else
    {
        VIncrement = 0;
    }

    if ((HIncrement == 1024) && (VIncrement == 1024))
    {
        /* No resize if scaling factor = 1 */
        #ifdef STBLIT_BENCHMARK3
        t20 = STOS_time_now();
        #endif

        return(Err);
    }

    /* Rajouter err si resize au dela de 6.10 */

    /* Instruction : Enable 2D Resize */
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_RESCALE_MASK) << STBLIT_INS_ENABLE_RESCALE_SHIFT);

    if (HIncrement != 1024)
    {
#if defined (WA_GNBvd10454)
        *HResizeOn = TRUE;
#endif
        /* 2D Resize control */
        CommonField_p->RZC |= ((U32)(STBLIT_RZC_2DHRESIZER_ENABLE & STBLIT_RZC_2DHRESIZER_MODE_MASK) << STBLIT_RZC_2DHRESIZER_MODE_SHIFT);

        /* Initial subposition */
        CommonField_p->RZC |= ((U32)(HSubposition & STBLIT_RZC_INITIAL_SUBPOS_HSRC_MASK) << STBLIT_RZC_INITIAL_SUBPOS_HSRC_SHIFT);

        if ((FilterMode & STBLIT_2DFILTER_MODE_MASK) != STBLIT_2DFILTER_INACTIVE)
        {
            CommonField_p->RZC |= ((U32)(FilterMode & STBLIT_RZC_2DHFILTER_MODE_MASK) << STBLIT_RZC_2DHFILTER_MODE_SHIFT);

            /* Select Best default filter from filter buffer */
            SelectFilter(HIncrement, &HOffset);

            /* Set horizontal filter */
        #ifdef STBLIT_LINUX_FULL_USER_VERSION
            Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Device_p->FilterBuffer_p) + HOffset);
        #else
            Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->FilterBuffer_p,&(Device_p->VirtualMapping)) + HOffset);
        #endif
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_HFP)),((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT ));
        }

        /* Scaling factor */
        RSF = ((U32)(HIncrement & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
    }


    if (VIncrement != 1024)
    {
#if defined (WA_GNBvd10454)
        *VResizeOn = TRUE;
#endif
        /* 2D Resize control */
        CommonField_p->RZC |= ((U32)(STBLIT_RZC_2DVRESIZER_ENABLE & STBLIT_RZC_2DVRESIZER_MODE_MASK) << STBLIT_RZC_2DVRESIZER_MODE_SHIFT);

        /* Initial subposition */
        CommonField_p->RZC |= ((U32)(VSubposition & STBLIT_RZC_INITIAL_SUBPOS_VSRC_MASK) << STBLIT_RZC_INITIAL_SUBPOS_VSRC_SHIFT);

        if ((FilterMode & STBLIT_2DFILTER_MODE_MASK) != STBLIT_2DFILTER_INACTIVE)
        {
            /* FilterMode have been set by constants which are consistent with Vertical constants */
            CommonField_p->RZC |= ((U32)(FilterMode & STBLIT_RZC_2DVFILTER_MODE_MASK) << STBLIT_RZC_2DVFILTER_MODE_SHIFT);

            /* Select Best default filter from filter buffer */
            SelectFilter(VIncrement, &VOffset);

            /* Set Vertical filter */
            #ifdef STBLIT_LINUX_FULL_USER_VERSION
                Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Device_p->FilterBuffer_p) + VOffset);
            #else
                Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->FilterBuffer_p,&(Device_p->VirtualMapping)) + VOffset);
            #endif
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VFP)),((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT ));

        }

        /* Scaling factor */
        RSF |= ((U32)(VIncrement & STBLIT_RSF_INCREMENT_VSRC_MASK) << STBLIT_RSF_INCREMENT_VSRC_SHIFT);

    }
    /* Write modified RSF value */
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)),RSF);

    #ifdef STBLIT_BENCHMARK3
    t20 = STOS_time_now();
    #endif

    return(Err);
 }

 /*******************************************************************************
Name        : SetNodeSliceResize
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized.
               slice management.
Limitations :  MB? TBD...
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeSliceResize (stblit_Node_t*           Node_p,
                                                    stblit_Device_t*         Device_p,
                                                    U8                       FilterMode,
                                                    STBLIT_SliceRectangle_t* SliceRectangle_p,
                                                    stblit_CommonField_t*    CommonField_p
                                                  #if defined (WA_GNBvd10454)
                                                    , BOOL* HResizeOn,
                                                      BOOL* VResizeOn
                                                  #endif
                                                      )
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             HOffset;
    U32             VOffset;
    /* Reserved1 = Increment Horizontal
     * Reserved3 = Increment Vertical
     * Reserved2 = Subposition Horizontal
     * Reserved4 = Subposition Vertical */
    U32             HIncrement    = SliceRectangle_p->Reserved1;
    U32             VIncrement    = SliceRectangle_p->Reserved3;
    U32             HSubposition  = (SliceRectangle_p->Reserved2) >> 7 ;  /* 10 bits precision to 3 bit precision (HW related)*/
    U32             VSubposition  = (SliceRectangle_p->Reserved4) >> 7;   /* 10 bits precision to 3 bit precision (HW related)*/
    U32             Tmp;
    U32             RSF = 0;

    /* Rajouter err si resize au dela de 6.10 */


    if ((HIncrement == 1024) && (VIncrement == 1024))
    {
        /* No resize if scaling factor = 1 */
        return(Err);
    }

    /* Instruction : Enable 2D Resize */
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_RESCALE_MASK) << STBLIT_INS_ENABLE_RESCALE_SHIFT);

    if (HIncrement != 1024)
    {

#if defined (WA_GNBvd10454)
        *HResizeOn = TRUE;
#endif
        /* 2D Resize control */
        CommonField_p->RZC |= ((U32)(STBLIT_RZC_2DHRESIZER_ENABLE & STBLIT_RZC_2DHRESIZER_MODE_MASK) << STBLIT_RZC_2DHRESIZER_MODE_SHIFT);

        /* Initial subposition */
        CommonField_p->RZC |= ((U32)(HSubposition & STBLIT_RZC_INITIAL_SUBPOS_HSRC_MASK) << STBLIT_RZC_INITIAL_SUBPOS_HSRC_SHIFT);

        if ((FilterMode & STBLIT_2DFILTER_MODE_MASK) != STBLIT_2DFILTER_INACTIVE)
        {
            /* FilterMode have been set by constants which are consistent with horizontal constants */
            CommonField_p->RZC |= ((U32)(FilterMode & STBLIT_RZC_2DHFILTER_MODE_MASK) << STBLIT_RZC_2DHFILTER_MODE_SHIFT);

            /* Select Best default filter from filter buffer */
            SelectFilter(HIncrement, &HOffset);

            /* Set horizontal filter */
        #ifdef STBLIT_LINUX_FULL_USER_VERSION
            Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Device_p->FilterBuffer_p) + HOffset);
        #else
            Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->FilterBuffer_p,&(Device_p->VirtualMapping)) + HOffset);
        #endif
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_HFP)),((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT ));

        }

        /* Scaling factor */
        RSF = ((U32)(HIncrement & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
    }

    if (VIncrement != 1024)
    {
#if defined (WA_GNBvd10454)
        *VResizeOn = TRUE;
#endif
        /* 2D Resize control */
        CommonField_p->RZC |= ((U32)(STBLIT_RZC_2DVRESIZER_ENABLE & STBLIT_RZC_2DVRESIZER_MODE_MASK) << STBLIT_RZC_2DVRESIZER_MODE_SHIFT);

        /* Initial subposition */
        CommonField_p->RZC |= ((U32)(VSubposition & STBLIT_RZC_INITIAL_SUBPOS_VSRC_MASK) << STBLIT_RZC_INITIAL_SUBPOS_VSRC_SHIFT);

        if ((FilterMode & STBLIT_2DFILTER_MODE_MASK) != STBLIT_2DFILTER_INACTIVE)
        {
            /* FilterMode have been set by constants which are consistent with Vertical constants */
            CommonField_p->RZC |= ((U32)(FilterMode & STBLIT_RZC_2DVFILTER_MODE_MASK) << STBLIT_RZC_2DVFILTER_MODE_SHIFT);

            /* Select Best default filter from filter buffer */
            SelectFilter(VIncrement, &VOffset);

            /* Set Vertical filter */
        #ifdef STBLIT_LINUX_FULL_USER_VERSION
            Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Device_p->FilterBuffer_p) + VOffset);
        #else
            Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->FilterBuffer_p,&(Device_p->VirtualMapping)) + VOffset);
        #endif
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VFP)),((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT ));
        }

        /* Scaling factor */
        RSF |= ((U32)(VIncrement & STBLIT_RSF_INCREMENT_VSRC_MASK) << STBLIT_RSF_INCREMENT_VSRC_SHIFT);
    }

    /* Write modified RSF value */
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)),RSF);

    return(Err);
 }


/*******************************************************************************
Name        : SetNodeALUBlend
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeALUBlend (U8                       GlobalAlpha,
                                                  BOOL                     PreMultipliedColor,
                                                  stblit_MaskMode_t        MaskMode,
                                                  stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t21 = STOS_time_now();
    #endif

    if ((MaskMode == STBLIT_NO_MASK) || (MaskMode == STBLIT_MASK_SECOND_PASS))
    {
        /* ALU control */
        CommonField_p->ACK |= ((U32)(GlobalAlpha & STBLIT_ACK_ALU_GLOBAL_ALPHA_MASK) <<  STBLIT_ACK_ALU_GLOBAL_ALPHA_SHIFT);

        if (PreMultipliedColor == FALSE)
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BLEND_UNWEIGHT & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
        else  /* Src weighted */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BLEND_WEIGHT & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
    }
    else  /* MaskMode == STBLIT_MASK_FIRST_PASS */
    {
        /* ALU control */
        CommonField_p->ACK |= ((U32)(128 & STBLIT_ACK_ALU_GLOBAL_ALPHA_MASK) <<  STBLIT_ACK_ALU_GLOBAL_ALPHA_SHIFT);
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_MASK_BLEND & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);

    }

    #ifdef STBLIT_BENCHMARK3
    t22 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeALUBlendXYL
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeALUBlendXYL (U8                    GlobalAlpha,
                                                   BOOL                  PreMultipliedColor,
                                                   BOOL                  MaskModeEnable,
                                                   stblit_CommonField_t* CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;


    /* ALU control */
    CommonField_p->ACK |= ((U32)(GlobalAlpha & STBLIT_ACK_ALU_GLOBAL_ALPHA_MASK) <<  STBLIT_ACK_ALU_GLOBAL_ALPHA_SHIFT);

    if (MaskModeEnable == FALSE)
    {
        if (PreMultipliedColor == FALSE)
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BLEND_UNWEIGHT & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
        else  /* Src weighted */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BLEND_WEIGHT & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
    }
    else  /* MaskMode enable*/
    {
        if (PreMultipliedColor == FALSE)
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_MASK_XYL_BLEND_UNWEIGHT & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
        else  /* Src weighted */
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_MASK_XYL_BLEND_WEIGHT & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }

    }

    return(Err);
 }



/*******************************************************************************
Name        : SetNodeALULogic
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeALULogic (stblit_MaskMode_t        MaskMode,
                                                  STBLIT_BlitContext_t*    BlitContext_p,
                                                  stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t23 = STOS_time_now();
    #endif

    if (MaskMode == STBLIT_NO_MASK)
    {
        if ((BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
            (BlitContext_p->EnableMaskWord == FALSE) &&
            (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))   /* Bypass Src2 */
        {
            /* ALU control */
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
        else if (BlitContext_p->AluMode == STBLIT_ALU_NOOP)  /* Bypass Src1 */
        {
            /* ALU control */
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC1 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
        else   /* ROP */
        {
            /* ALU control */
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_LOGICAL & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
            CommonField_p->ACK |= ((U32)(BlitContext_p->AluMode & STBLIT_ACK_ALU_ROP_MODE_MASK) << STBLIT_ACK_ALU_ROP_MODE_SHIFT);
        }
    }
    else if (MaskMode == STBLIT_MASK_FIRST_PASS)
    {
        /* ALU control */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_MASK_LOGICAL_FIRST & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
    }
    else /* MaskMode == STBLIT_MASK_SECOND_PASS */
    {
        /* ALU control */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_MASK_LOGICAL_SECOND & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        CommonField_p->ACK |= ((U32)(BlitContext_p->AluMode & STBLIT_ACK_ALU_ROP_MODE_MASK) << STBLIT_ACK_ALU_ROP_MODE_SHIFT);
    }

    #ifdef STBLIT_BENCHMARK3
    t24 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeALULogicXYL
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeALULogicXYL (BOOL                     MaskModeEnable,
                                                   STBLIT_BlitContext_t*    BlitContext_p,
                                                   stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    if (MaskModeEnable == FALSE)
    {
        if ((BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
            (BlitContext_p->EnableMaskWord == FALSE) &&
            (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))   /* Bypass Src2 */
        {
            /* ALU control */
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
        else if (BlitContext_p->AluMode == STBLIT_ALU_NOOP)  /* Bypass Src1 */
        {
            /* ALU control */
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC1 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
        else   /* ROP */
        {
            /* ALU control */
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_LOGICAL & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
            CommonField_p->ACK |= ((U32)(BlitContext_p->AluMode & STBLIT_ACK_ALU_ROP_MODE_MASK) << STBLIT_ACK_ALU_ROP_MODE_SHIFT);
        }
    }
    else /*  Mask enable */
    {
        /* ALU control */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_MASK_XYL_LOGICAL & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        CommonField_p->ACK |= ((U32)(BlitContext_p->AluMode & STBLIT_ACK_ALU_ROP_MODE_MASK) << STBLIT_ACK_ALU_ROP_MODE_SHIFT);
    }

    return(Err);
 }


/*******************************************************************************
Name        : SetNodeFlicker
Description :
Parameters  :
Assumptions : All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeFlicker (stblit_Node_t*              Node_p,
                                                STGXOBJ_ColorType_t         ColorType,
                                                STBLIT_FlickerFilterMode_t  FlickerFilter,
                                                stblit_CommonField_t*       CommonField_p)
 {
    U32             _FF0, _FF1, _FF2, _FF3;
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t25 = STOS_time_now();
    #endif

    /* Instruction : Enable Flicker filter */
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_FLICKER_FILTER_MASK) << STBLIT_INS_ENABLE_FLICKER_FILTER_SHIFT);

    if ((ColorType == STGXOBJ_COLOR_TYPE_RGB565) ||
        (ColorType == STGXOBJ_COLOR_TYPE_RGB888) ||
        (ColorType == STGXOBJ_COLOR_TYPE_ARGB8565) ||
        (ColorType == STGXOBJ_COLOR_TYPE_ARGB8565_255) ||
        (ColorType == STGXOBJ_COLOR_TYPE_ARGB8888) ||
        (ColorType == STGXOBJ_COLOR_TYPE_ARGB8888_255) ||
        (ColorType == STGXOBJ_COLOR_TYPE_ARGB1555) ||
        (ColorType == STGXOBJ_COLOR_TYPE_ARGB4444))
    {
        /* RZC : Set Flicker filter mode (If color == RGB => Adaptatif */
        CommonField_p->RZC |= ((U32)(STBLIT_RZC_FFILTER_MODE_ADAPTATIVE & STBLIT_RZC_FFILTER_MODE_MASK ) << STBLIT_RZC_FFILTER_MODE_SHIFT);

         /* Check FilterFlicker Type */

        if (FlickerFilter == STBLIT_FLICKER_FILTER_MODE_ADAPTIVE )

        {
        /* BLT_FF0 : Set Default value for Flicker filter  */
        _FF0 = ((U32)(0 & STBLIT_FF0_N_MINUS_1_COEFF_MASK) << STBLIT_FF0_N_MINUS_1_COEFF_SHIFT);
        _FF0 |= ((U32)(128 & STBLIT_FF0_N_COEFF_MASK) << STBLIT_FF0_N_COEFF_SHIFT);
        _FF0 |= ((U32)(0 & STBLIT_FF0_N_PLUS_1_COEFF_MASK) << STBLIT_FF0_N_PLUS_1_COEFF_SHIFT);
        _FF0 |= ((U32)(4 & STBLIT_FF0_THRESHOLD_MASK) << STBLIT_FF0_THRESHOLD_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF0)),_FF0);

        /* BLT_FF1 : Set Default value for Flicker filter  */
        _FF1 = ((U32)(16 & STBLIT_FF1_N_MINUS_1_COEFF_MASK) << STBLIT_FF1_N_MINUS_1_COEFF_SHIFT);
        _FF1 |= ((U32)(96 & STBLIT_FF1_N_COEFF_MASK) << STBLIT_FF1_N_COEFF_SHIFT);
        _FF1 |= ((U32)(16 & STBLIT_FF1_N_PLUS_1_COEFF_MASK) << STBLIT_FF1_N_PLUS_1_COEFF_SHIFT);
        _FF1 |= ((U32)(6 & STBLIT_FF1_THRESHOLD_MASK) << STBLIT_FF1_THRESHOLD_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF1)),_FF1);

        /* BLT_FF2 : Set Default value for Flicker filter  */
        _FF2 = ((U32)(24 & STBLIT_FF2_N_MINUS_1_COEFF_MASK) << STBLIT_FF2_N_MINUS_1_COEFF_SHIFT);
        _FF2 |= ((U32)(80 & STBLIT_FF2_N_COEFF_MASK) << STBLIT_FF2_N_COEFF_SHIFT);
        _FF2 |= ((U32)(24 & STBLIT_FF2_N_PLUS_1_COEFF_MASK) << STBLIT_FF2_N_PLUS_1_COEFF_SHIFT);
        _FF2 |= ((U32)(8 & STBLIT_FF2_THRESHOLD_MASK) << STBLIT_FF2_THRESHOLD_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF2)),_FF2);

        /* BLT_FF3 : Set Default value for Flicker filter  */
        _FF3 = ((U32)(32 & STBLIT_FF3_N_MINUS_1_COEFF_MASK) << STBLIT_FF3_N_MINUS_1_COEFF_SHIFT);
        _FF3 |= ((U32)(64 & STBLIT_FF3_N_COEFF_MASK) << STBLIT_FF3_N_COEFF_SHIFT);
        _FF3 |= ((U32)(32 & STBLIT_FF3_N_PLUS_1_COEFF_MASK) << STBLIT_FF3_N_PLUS_1_COEFF_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF3)),_FF3);

        }

        if (FlickerFilter== STBLIT_FLICKER_FILTER_MODE_SIMPLE )

        {

        /* BLT_FF0 : Set Mode2 value for Flicker filter  */
        _FF0 = ((U32)(32 & STBLIT_FF0_N_MINUS_1_COEFF_MASK) << STBLIT_FF0_N_MINUS_1_COEFF_SHIFT);
        _FF0 |= ((U32)(64 & STBLIT_FF0_N_COEFF_MASK) << STBLIT_FF0_N_COEFF_SHIFT);
        _FF0 |= ((U32)(32 & STBLIT_FF0_N_PLUS_1_COEFF_MASK) << STBLIT_FF0_N_PLUS_1_COEFF_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF0)),_FF0);

        /* BLT_FF1 : Set Mode2 value for Flicker filter  */
        _FF1 = ((U32)(32 & STBLIT_FF1_N_MINUS_1_COEFF_MASK) << STBLIT_FF1_N_MINUS_1_COEFF_SHIFT);
        _FF1 |= ((U32)(64 & STBLIT_FF1_N_COEFF_MASK) << STBLIT_FF1_N_COEFF_SHIFT);
        _FF1 |= ((U32)(32 & STBLIT_FF1_N_PLUS_1_COEFF_MASK) << STBLIT_FF1_N_PLUS_1_COEFF_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF1)),_FF1);

        /* BLT_FF2 : Set Mode2 value for Flicker filter  */
        _FF2 = ((U32)(32 & STBLIT_FF2_N_MINUS_1_COEFF_MASK) << STBLIT_FF2_N_MINUS_1_COEFF_SHIFT);
        _FF2 |= ((U32)(64 & STBLIT_FF2_N_COEFF_MASK) << STBLIT_FF2_N_COEFF_SHIFT);
        _FF2 |= ((U32)(32 & STBLIT_FF2_N_PLUS_1_COEFF_MASK) << STBLIT_FF2_N_PLUS_1_COEFF_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF2)),_FF2);

        /* BLT_FF3 : Set Mode2 value for Flicker filter  */
        _FF3 = ((U32)(32 & STBLIT_FF3_N_MINUS_1_COEFF_MASK) << STBLIT_FF3_N_MINUS_1_COEFF_SHIFT);
        _FF3 |= ((U32)(64 & STBLIT_FF3_N_COEFF_MASK) << STBLIT_FF3_N_COEFF_SHIFT);
        _FF3 |= ((U32)(32 & STBLIT_FF3_N_PLUS_1_COEFF_MASK) << STBLIT_FF3_N_PLUS_1_COEFF_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF3)),_FF3);

         }

    }
    else
    {
        /* RZC : Set Flicker filter mode (If color != RGB => ForceO */
        CommonField_p->RZC |= ((U32)(STBLIT_RZC_FFILTER_MODE_FORCE0 & STBLIT_RZC_FFILTER_MODE_MASK ) << STBLIT_RZC_FFILTER_MODE_SHIFT);

        /* BLT_FF0 : Set Default value for Flicker filter  TBDefine*/
        _FF0 = ((U32)(16 & STBLIT_FF0_N_MINUS_1_COEFF_MASK) << STBLIT_FF0_N_MINUS_1_COEFF_SHIFT);
        _FF0 |= ((U32)(96 & STBLIT_FF0_N_COEFF_MASK) << STBLIT_FF0_N_COEFF_SHIFT);
        _FF0 |= ((U32)(16 & STBLIT_FF0_N_PLUS_1_COEFF_MASK) << STBLIT_FF0_N_PLUS_1_COEFF_SHIFT);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF0)),_FF0);
    }
    #ifdef STBLIT_BENCHMARK3
    t26 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeInputConverter
Description :
Parameters  :
Assumptions : All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeInputConverter (U8                       InEnable,
                                                        U8                       InConfig,
                                                        stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err  = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t27 = STOS_time_now();
    #endif

    /* Instruction : Enable Input color converter operator */
    CommonField_p->INS |= ((U32)(InEnable & STBLIT_INS_ENABLE_IN_CSC_MASK) << STBLIT_INS_ENABLE_IN_CSC_SHIFT);

    /* Set operators */
    CommonField_p->CCO |= ((U32)(InConfig & STBLIT_CCO_IN_MASK) << STBLIT_CCO_IN_SHIFT);

    #ifdef STBLIT_BENCHMARK3
    t28 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeOutputConverter
Description :
Parameters  :
Assumptions : All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeOutputConverter (U8                OutEnable,
                                                         U8                OutConfig,
                                                         stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err  = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t29 = STOS_time_now();
    #endif

    /* Instruction : Enable Output color converter operator */
    CommonField_p->INS |= ((U32)(OutEnable & STBLIT_INS_ENABLE_OUT_CSC_MASK) << STBLIT_INS_ENABLE_OUT_CSC_SHIFT);

    /* Set operators */
    CommonField_p->CCO |= ((U32)(OutConfig & STBLIT_CCO_OUT_MASK) << STBLIT_CCO_OUT_SHIFT);

    #ifdef STBLIT_BENCHMARK3
    t30 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeColorCorrection
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized
               Clut must be aligned on 128 bit words
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeColorCorrection (STBLIT_BlitContext_t*    BlitContext_p,
                                                        stblit_Device_t*         Device_p,
                                                        stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Tmp;

    #ifdef STBLIT_BENCHMARK3
    t31 = STOS_time_now();
    #endif

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* Instruction : Enable CLUT-Based operator */
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_CLUT_MASK) << STBLIT_INS_ENABLE_CLUT_SHIFT);

    /* Set Color Correction mode */
    CommonField_p->CCO |= ((U32)(STBLIT_CCO_CLUT_OPMODE_CORRECTION & STBLIT_CCO_CLUT_OPMODE_MASK) << STBLIT_CCO_CLUT_OPMODE_SHIFT);

    /* Enable CLUT Update */
    CommonField_p->CCO |= ((U32)(1 & STBLIT_CCO_ENABLE_CLUT_UPDATE_MASK ) << STBLIT_CCO_ENABLE_CLUT_UPDATE_SHIFT);

    /* Set CLUT */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) stblit_CpuToDevice((void*)(BlitContext_p->ColorCorrectionTable_p->Data_p));
#else
    Tmp =  (U32)STAVMEM_VirtualToDevice((void*)(BlitContext_p->ColorCorrectionTable_p->Data_p),&(Device_p->VirtualMapping));
#endif
    CommonField_p->CML |= ((U32)(Tmp & STBLIT_CML_POINTER_MASK) << STBLIT_CML_POINTER_SHIFT);

    #ifdef STBLIT_BENCHMARK3
    t32 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeColorExpansion
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized
               Clut must be aligned on 128 bit words
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeColorExpansion  (STGXOBJ_Palette_t*  Palette_p,
                                                        stblit_Device_t* Device_p,
                                                        stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Tmp;

    #ifdef STBLIT_BENCHMARK3
    t33 = STOS_time_now();
    #endif

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* Instruction : Enable CLUT-Based operator */
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_CLUT_MASK) << STBLIT_INS_ENABLE_CLUT_SHIFT);

    /* Set Color Expansion mode */
    CommonField_p->CCO |= ((U32)(STBLIT_CCO_CLUT_OPMODE_EXPANSION & STBLIT_CCO_CLUT_OPMODE_MASK) << STBLIT_CCO_CLUT_OPMODE_SHIFT);

    /* Enable CLUT Update */
    CommonField_p->CCO |= ((U32)(1 & STBLIT_CCO_ENABLE_CLUT_UPDATE_MASK ) << STBLIT_CCO_ENABLE_CLUT_UPDATE_SHIFT);

    /* Update Alpha whitin the CLUT when ACLUTXX  color types. In that case the alpha value within the CLUT must be equal to its index.
     * In that case aIndexRIndexGIndexB -> aRGB.  */

    /* Set CLUT */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) stblit_CpuToDevice((void*)Palette_p->Data_p);
#else
    Tmp =  (U32)STAVMEM_VirtualToDevice((void*)(Palette_p->Data_p),&(Device_p->VirtualMapping));
#endif
    CommonField_p->CML |= ((U32)(Tmp & STBLIT_CML_POINTER_MASK) << STBLIT_CML_POINTER_SHIFT);

    #ifdef STBLIT_BENCHMARK3
    t34 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeColorReduction
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized
               Clut must be aligned on 128 bit words
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetNodeColorReduction  (STGXOBJ_Palette_t*      Palette_p,
                                                         stblit_Device_t*        Device_p,
                                                        stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             Tmp;

    #ifdef STBLIT_BENCHMARK3
    t35 = STOS_time_now();
    #endif

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);


    /* Instruction : Enable CLUT-Based operator */
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_CLUT_MASK) << STBLIT_INS_ENABLE_CLUT_SHIFT);

    /* Set Color Reduction mode */
    CommonField_p->CCO |= ((U32)(STBLIT_CCO_CLUT_OPMODE_REDUCTION & STBLIT_CCO_CLUT_OPMODE_MASK) << STBLIT_CCO_CLUT_OPMODE_SHIFT);

    /* Set Mode adaptatif */
    CommonField_p->CCO |= ((U32)(STBLIT_CCO_ERROR_DIFF_WEIGHT_ADAPT & STBLIT_CCO_ERROR_DIFF_WEIGHT_MASK) << STBLIT_CCO_ERROR_DIFF_WEIGHT_SHIFT);

    /* Enable CLUT Update */
    CommonField_p->CCO |= ((U32)(1 & STBLIT_CCO_ENABLE_CLUT_UPDATE_MASK ) << STBLIT_CCO_ENABLE_CLUT_UPDATE_SHIFT);

    /* Set CLUT */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) stblit_CpuToDevice((void*)Palette_p->Data_p);
#else
    Tmp =  (U32)STAVMEM_VirtualToDevice((void*)(Palette_p->Data_p),&(Device_p->VirtualMapping));
#endif
    CommonField_p->CML |= ((U32)(Tmp & STBLIT_CML_POINTER_MASK) << STBLIT_CML_POINTER_SHIFT);

    #ifdef STBLIT_BENCHMARK3
    t36 = STOS_time_now();
    #endif

    return(Err);
 }

/*******************************************************************************
Name        : SetNodeAntiFlutter
Description :
Parameters  :
Assumptions : All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeAntiFlutter (stblit_Node_t*          Node_p,
                                                    stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    UNUSED_PARAMETER(Node_p);

    CommonField_p->RZC |= ((U32)(1 & STBLIT_RZC_ENABLE_ALPHA_H_BORDER_MASK) << STBLIT_RZC_ENABLE_ALPHA_H_BORDER_SHIFT);

    return(Err);
 }


/*******************************************************************************
Name        : SetNodeAntiAliasing
Description :
Parameters  :
Assumptions : All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeAntiAliasing (stblit_Node_t*          Node_p,
                                                     stblit_CommonField_t*   CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    UNUSED_PARAMETER(Node_p);

    CommonField_p->RZC |= ((U32)(1 & STBLIT_RZC_ENABLE_ALPHA_V_BORDER_MASK) << STBLIT_RZC_ENABLE_ALPHA_V_BORDER_SHIFT);

    return(Err);
 }


/*******************************************************************************
Name        : SetOperationConfiguration
Description : Record operation configuration for further use by Job Blit
Parameters  :
Assumptions :  All parameters checks done at upper level.
               Src1_p and Src2_p can only be NULL in case of STBLIT_NO_MASK
               In Mask support, there are always non NULL!
Limitations :
Returns     :
*******************************************************************************/
  __inline static ST_ErrorCode_t SetOperationIOConfiguration  (STBLIT_Source_t*      Src1_p,
                                                               STBLIT_Source_t*      Src2_p,
                                                               STBLIT_Destination_t* Dst_p,
                                                               stblit_MaskMode_t     MaskMode,
                                                               stblit_OperationConfiguration_t* Config_p)
 {
    ST_ErrorCode_t          Err                  = ST_NO_ERROR;
    BOOL                    CanForegroundBeSet   = FALSE;
    BOOL                    CanBackgroundBeSet   = FALSE;
    BOOL                    CanDestinationBeSet  = FALSE;
    BOOL                    CanMaskBeSet         = FALSE;
    STBLIT_Source_t*        Mask_p               = NULL;
    STBLIT_Source_t*        Foreground_p         = NULL;
    STBLIT_Source_t*        Background_p         = NULL;
    STBLIT_Destination_t*   Destination_p        = NULL;

    if (MaskMode == STBLIT_NO_MASK) /* One pass operation without mask */
    {
        CanForegroundBeSet  = TRUE;
        CanBackgroundBeSet  = TRUE;
        CanDestinationBeSet = TRUE;
        Foreground_p        = Src2_p;
        Background_p        = Src1_p;
        Destination_p       = Dst_p;
        CanMaskBeSet        = FALSE;
        Config_p->MaskEnabled = FALSE;
    }
    else if (MaskMode == STBLIT_MASK_FIRST_PASS)
    {
        CanForegroundBeSet  = TRUE;
        CanBackgroundBeSet  = FALSE;
        CanDestinationBeSet = FALSE;
        Foreground_p        = Src1_p;
        CanMaskBeSet        = TRUE;
        Mask_p                 = Src2_p;
        Config_p->MaskEnabled  = TRUE;
    }
    else  /* STBLIT__MASK_SECOND_PASS */
    {
        CanForegroundBeSet  = FALSE;
        CanBackgroundBeSet  = TRUE;
        CanDestinationBeSet = TRUE;
        Background_p        = Src1_p;
        Destination_p       = Dst_p;
        CanMaskBeSet        = FALSE;
        Config_p->MaskEnabled = TRUE;
    }

    if (CanForegroundBeSet == TRUE)
    {
        if (Foreground_p == NULL)
        {
            Config_p->Foreground.Type = STBLIT_INFO_TYPE_NONE;
        }
        else if (Foreground_p->Type == STBLIT_SOURCE_TYPE_COLOR)
        {
            Config_p->Foreground.Type = STBLIT_INFO_TYPE_COLOR;
            Config_p->Foreground.Info.Color.ColorType = Foreground_p->Data.Color_p->Type;
        }
        else /* STBLIT_SOURCE_TYPE_BITMAP */
        {
            Config_p->Foreground.Type = STBLIT_INFO_TYPE_BITMAP;
            Config_p->Foreground.Info.Bitmap.ColorType    = Foreground_p->Data.Bitmap_p->ColorType;
            Config_p->Foreground.Info.Bitmap.BitmapType   = Foreground_p->Data.Bitmap_p->BitmapType;
            if ((Config_p->Foreground.Info.Bitmap.ColorType == STGXOBJ_COLOR_TYPE_CLUT4) ||
                (Config_p->Foreground.Info.Bitmap.ColorType == STGXOBJ_COLOR_TYPE_CLUT2) ||
                (Config_p->Foreground.Info.Bitmap.ColorType == STGXOBJ_COLOR_TYPE_CLUT1) ||
                (Config_p->Foreground.Info.Bitmap.ColorType == STGXOBJ_COLOR_TYPE_ALPHA1))
            {
                Config_p->Foreground.Info.Bitmap.IsSubByteFormat    = TRUE;
                Config_p->Foreground.Info.Bitmap.SubByteFormat      = Foreground_p->Data.Bitmap_p->SubByteFormat;
            }
            else
            {
                Config_p->Foreground.Info.Bitmap.IsSubByteFormat    = FALSE;
            }
            Config_p->Foreground.Info.Bitmap.BitmapWidth        = Foreground_p->Data.Bitmap_p->Width;
            Config_p->Foreground.Info.Bitmap.BitmapHeight       = Foreground_p->Data.Bitmap_p->Height;
            Config_p->Foreground.Info.Bitmap.Rectangle          = Foreground_p->Rectangle;
        }
    }
    if (CanBackgroundBeSet == TRUE)
    {
        if (Background_p == NULL)
        {
            Config_p->Background.Type = STBLIT_INFO_TYPE_NONE;
        }
        else if (Background_p->Type == STBLIT_SOURCE_TYPE_COLOR)
        {
            Config_p->Background.Type = STBLIT_INFO_TYPE_COLOR;
            Config_p->Background.Info.Color.ColorType = Background_p->Data.Color_p->Type;
        }
        else /* STBLIT_SOURCE_TYPE_BITMAP */
        {
            Config_p->Background.Type = STBLIT_INFO_TYPE_BITMAP;
            Config_p->Background.Info.Bitmap.ColorType  = Background_p->Data.Bitmap_p->ColorType;
            Config_p->Background.Info.Bitmap.BitmapType = Background_p->Data.Bitmap_p->BitmapType;
            if ((Config_p->Background.Info.Bitmap.ColorType == STGXOBJ_COLOR_TYPE_CLUT4) ||
                (Config_p->Background.Info.Bitmap.ColorType == STGXOBJ_COLOR_TYPE_CLUT2) ||
                (Config_p->Background.Info.Bitmap.ColorType == STGXOBJ_COLOR_TYPE_CLUT1) ||
                (Config_p->Background.Info.Bitmap.ColorType == STGXOBJ_COLOR_TYPE_ALPHA1))
            {
                Config_p->Background.Info.Bitmap.IsSubByteFormat  = TRUE;
                Config_p->Background.Info.Bitmap.SubByteFormat    = Background_p->Data.Bitmap_p->SubByteFormat;
            }
            else
            {
                Config_p->Background.Info.Bitmap.IsSubByteFormat  = FALSE;
            }
            Config_p->Background.Info.Bitmap.BitmapWidth        = Background_p->Data.Bitmap_p->Width;
            Config_p->Background.Info.Bitmap.BitmapHeight       = Background_p->Data.Bitmap_p->Height;
            Config_p->Background.Info.Bitmap.Rectangle          = Background_p->Rectangle;
        }
    }

    if (CanDestinationBeSet == TRUE) /* Destination_p is never NULL in that case */
    {
        Config_p->Destination.ColorType          = Destination_p->Bitmap_p->ColorType;
        Config_p->Destination.BitmapType         = Destination_p->Bitmap_p->BitmapType;
        if ((Config_p->Destination.ColorType == STGXOBJ_COLOR_TYPE_CLUT4) ||
            (Config_p->Destination.ColorType == STGXOBJ_COLOR_TYPE_CLUT2) ||
            (Config_p->Destination.ColorType == STGXOBJ_COLOR_TYPE_CLUT1) ||
            (Config_p->Destination.ColorType == STGXOBJ_COLOR_TYPE_ALPHA1))
        {
            Config_p->Destination.IsSubByteFormat    = TRUE;
            Config_p->Destination.SubByteFormat      = Destination_p->Bitmap_p->SubByteFormat;
        }
        else
        {
            Config_p->Destination.IsSubByteFormat    = FALSE;
        }
        Config_p->Destination.BitmapWidth        = Destination_p->Bitmap_p->Width;
        Config_p->Destination.BitmapHeight       = Destination_p->Bitmap_p->Height;
        Config_p->Destination.Rectangle          = Destination_p->Rectangle;
    }

    if (CanMaskBeSet == TRUE)  /* Mask_p never NULL in that case*/
    {
        Config_p->MaskInfo.ColorType          = Mask_p->Data.Bitmap_p->ColorType;
        Config_p->MaskInfo.BitmapType         = Mask_p->Data.Bitmap_p->BitmapType;
        if (Config_p->MaskInfo.ColorType == STGXOBJ_COLOR_TYPE_ALPHA1)
        {
            Config_p->MaskInfo.IsSubByteFormat    = TRUE;
            Config_p->MaskInfo.SubByteFormat      = Mask_p->Data.Bitmap_p->SubByteFormat;
        }
        else
        {
            Config_p->MaskInfo.IsSubByteFormat    = FALSE;
        }
        Config_p->MaskInfo.BitmapWidth        = Mask_p->Data.Bitmap_p->Width;
        Config_p->MaskInfo.BitmapHeight       = Mask_p->Data.Bitmap_p->Height;
        Config_p->MaskInfo.Rectangle          = Mask_p->Rectangle;
    }


    return(Err);
 }


/*******************************************************************************
Name        : stblit_OnePassBlitOperation
Description : Set one node parameters
Parameters  :
Assumptions :  All parameters checks done at upper level :
               + Device_p is valid pointer never NULL
               + Dst_p is valid pointer never NULL
               + BlitContext_p is valid pointer never NULL
               + CommonField_p is valid pointer never NULL
               + Config_p is valid pointer never NULL
               + NodeHandle_p is valid pointer never NULL

               Src1_p and Src2_p may be NULL !!!!
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_OnePassBlitOperation(stblit_Device_t*        Device_p,
                                        STBLIT_Source_t*        Src1_p,
                                        STBLIT_Source_t*        Src2_p,
                                        STBLIT_Destination_t*   Dst_p,
                                        STBLIT_BlitContext_t*   BlitContext_p,
                                        U16                     Code,
                                        stblit_MaskMode_t       MaskMode,
                                        stblit_NodeHandle_t*    NodeHandle_p,
                                        BOOL                    SrcMB,
                                        stblit_OperationConfiguration_t*  Config_p,
                                        stblit_CommonField_t*             CommonField_p)
 {
    ST_ErrorCode_t          Err                 = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    U8                      ClutMode            = 0;  /* CLUT mode */
    U8                      InConfig            = 0;  /* 3 bits : Config  Input Converter
                                                              bit0 =>  Input Color Converter Colorimetry
                                                              bit1 =>  Input Color Converter Chroma format
                                                              bit2 =>  Input Color converter direction */
    U8                      OutConfig           = 0;  /* 2 bits : Config  Output Converter
                                                              bit0 =>  Output Color Converter Colorimetry
                                                              bit1 =>  Output Color Converter Chroma format*/
    U8                      OutEnable           = 0;  /* Enable Output converter */
    U8                      InEnable            = 0;  /* Enable Input converter */
    U8                      ClutEnable          = 0;  /* Enable CLUT mode */
    U8                      ScanConfig          = 0;  /* Vertical and Horizontal scan configuration. Down Right per default */
    U8                      OverlapCode;              /* Resulting overlap direction code both src */
    U8                      Src1Code            = 0xF;  /* Overlap direction code Src1 */
    U8                      Src2Code            = 0xF;  /* Overlap direction code Src2 */
    U32                     DstXStop;
    U32                     DstYStop;
    U32                     Src1XStop;
    U32                     Src1YStop;
    U32                     Src2XStop;
    U32                     Src2YStop;
    U8                      FilterMode;
#if defined (WA_GNBvd10454)
    BOOL                    HResizeOn = FALSE;
    BOOL                    VResizeOn = FALSE;
    U32                     RSF       = 0;
#endif

#ifdef WA_GNBvd49348
    U32                     RSF = 0;
#endif



    #ifdef STBLIT_BENCHMARK2
    t1 = STOS_time_now();
    #endif

    if (SrcMB == FALSE)
    {
        /* Analyse overlap Src1/Dst and Src2/Dst => Find common direction
         * If any, Src1, Src2 and Dst are from the same bitmap */

        /* Destination settings */
        DstXStop  = Dst_p->Rectangle.PositionX + (U32)(Dst_p->Rectangle.Width) - 1 ;
        DstYStop  = Dst_p->Rectangle.PositionY + (U32)(Dst_p->Rectangle.Height) - 1 ;

        /* Overlap Src1 / Dst */
        if (Src1_p != NULL)
        {
            /* Src1 settings */
            if (Src1_p->Type == STBLIT_SOURCE_TYPE_BITMAP)
            {
                if (Src1_p->Data.Bitmap_p == Dst_p->Bitmap_p)
                {
                    Src1XStop  = Src1_p->Rectangle.PositionX + (U32)(Src1_p->Rectangle.Width) - 1 ;
                    Src1YStop  = Src1_p->Rectangle.PositionY + (U32)(Src1_p->Rectangle.Height) - 1 ;
                    Src1Code   = OverlapOneSourceCoordinate(Src1_p->Rectangle.PositionX,Src1_p->Rectangle.PositionY,Src1XStop,Src1YStop,
                                                    Dst_p->Rectangle.PositionX,Dst_p->Rectangle.PositionY,DstXStop,DstYStop);
                }
            }
        }
        /* Overlap Src2 / Dst */
        if (Src2_p != NULL)
        {
            /* Src1 settings */
            if (Src2_p->Type == STBLIT_SOURCE_TYPE_BITMAP)
            {
                if (Src2_p->Data.Bitmap_p == Dst_p->Bitmap_p)
                {

                    Src2XStop  = Src2_p->Rectangle.PositionX + (U32)(Src2_p->Rectangle.Width) - 1 ;
                    Src2YStop  = Src2_p->Rectangle.PositionY + (U32)(Src2_p->Rectangle.Height) - 1 ;
                    Src2Code   = OverlapOneSourceCoordinate(Src2_p->Rectangle.PositionX,Src2_p->Rectangle.PositionY,Src2XStop,Src2YStop,
                                                       Dst_p->Rectangle.PositionX,Dst_p->Rectangle.PositionY,DstXStop,DstYStop);
                }
            }
        }

        OverlapCode = (Src1Code & Src2Code) & 0xF;
        if (OverlapCode != 0)
        {
            ScanConfig = ScanDirectionTable[OverlapCode];
        }
        else /* Code = 0 => Overlap Not Supported */
        {
            return(STBLIT_ERROR_OVERLAP_NOT_SUPPORTED);
        }
    }

    /* Get Node descriptor handle*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
        }
    }
    else  /* Blit in Job */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_JOB_NODE);
        }
    }

    Node_p  = (stblit_Node_t*)(*NodeHandle_p);

    /* Decode U16 code :
     *  ________________________________________________________________________________
     * |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
     * | 1  |  1 |    |    |    |    | 1  | 1  | 1  | 1  | 1  | 1  | 1  | 1  | 1  |  1 |
     * |____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|
     *   |    |                         |    |    |    |    |    |    |    |    |    |
     *   |____|                         |____|    |    |    |    |    |    |    |    |
     *   |                              |         |    |    |    |    |    |    |    |
     *  \/                             \/        \/    |   \/    |   \/   \/   \/   \/
     * Conversion                    CLUT      Out     | In RGB  |  In    Ena  Ena  Ena
     * Supported :                   Mode      Signed  |  To     |Matrix  Out  CLUT  In
     * 1X : Not supported                             \/  YCbCr \/  709
     * 00 : Ok                                       Out        In
     * 01 : Ok if color correction enabled          Matrix 709  Signed
     *
     *
     * */

    ClutMode    = (U8)((Code >> 8) & 0x3);
    InConfig    = (U8)((Code >> 3) & 0x7);
    OutConfig   = (U8)((Code >> 6) & 0x3);
    OutEnable   = (U8)((Code >> 2) & 0x1);
    ClutEnable  = (U8)((Code >> 1) & 0x1);
    InEnable    = (U8)(Code  & 0x1);

#ifdef WA_GNBvd13604
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528)  ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
    {
        if ((ClutEnable == 1) && (ClutMode == 0)) /* Color expansion */
        {
            if (Src2_p->Type == STBLIT_SOURCE_TYPE_COLOR)
            {
                if (Src2_p->Data.Color_p->Type == STGXOBJ_COLOR_TYPE_ACLUT44)
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
            else
            {
                if (Src2_p->Data.Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ACLUT44)
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
            }
        }
    }
#endif

    /* Init node */
    InitNode(Node_p, BlitContext_p);

    /* Set BorderAlpha for Anti-Flutter */
    if (Device_p->AntiFlutterOn == TRUE)
	{
        SetNodeAntiFlutter (Node_p, CommonField_p);
	}

    /* Set BorderAlpha for Anti-Aliasing */
    if (Device_p->AntiAliasingOn == TRUE)
	{
        SetNodeAntiAliasing (Node_p, CommonField_p);
	}

    /* set Src1 */
    SetNodeSrc1(Node_p, Src1_p, ScanConfig,Device_p, CommonField_p);

    /* Set Src2  */
    SetNodeSrc2(Node_p, Src2_p,&(Dst_p->Rectangle),ScanConfig,Device_p, CommonField_p);

    /* Set Target */
    SetNodeTgt(Node_p, Dst_p->Bitmap_p,&(Dst_p->Rectangle), ScanConfig,Device_p);

    /* Set Interrupts */
    SetNodeIT (Node_p,BlitContext_p, CommonField_p);

    if (MaskMode != STBLIT_MASK_FIRST_PASS)
    {
        /* Set Clipping */
        if (BlitContext_p->EnableClipRectangle == TRUE)
        {
            SetNodeClipping(Node_p,&(BlitContext_p->ClipRectangle), BlitContext_p->WriteInsideClipRectangle, CommonField_p);
        }

        /* Set Plane Mask */
        if (BlitContext_p->EnableMaskWord == TRUE)
        {
            SetNodePlaneMask(Node_p, BlitContext_p->MaskWord, CommonField_p);
        }

        /* Set ColorKey */
        if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE)
        {
            SetNodeColorKey(Node_p, BlitContext_p->ColorKeyCopyMode, &(BlitContext_p->ColorKey),CommonField_p);
        }
    }

    /* Set Trigger */
    if (BlitContext_p->Trigger.EnableTrigger == TRUE)
    {
        SetNodeTrigger(Node_p, BlitContext_p, CommonField_p);
    }

    /* If Mask first pass, there must be no resize since Mask size = Foreground size. On top
     * of that there is no reason to filter on first pass. Flicker will be done on 2nd pass if any*/
    if ((Src2_p != NULL) && (MaskMode != STBLIT_MASK_FIRST_PASS))
    {

        if (Src2_p->Type == STBLIT_SOURCE_TYPE_BITMAP)
        {
            /* Set Flicker filter only if Src2 is a bitmap */
            if (BlitContext_p->EnableFlickerFilter == TRUE)
            {
                SetNodeFlicker(Node_p,Src2_p->Data.Bitmap_p->ColorType,Device_p->FlickerFilterMode, CommonField_p);
            }

            /* Set Resize */
            /* Src1 and Dst have same size. Only resize possible between Src2 and Dst
            * Set Resize only if src2 is type bitmap : Src2 is always type color in color fill. It's not possible to have
            * src1 type color or destination type color and not src2*/
            if ( BlitContext_p->EnableResizeFilter == FALSE )
            {
                FilterMode = STBLIT_2DFILTER_INACTIVE;
            }
            else
            {
                if ((MaskMode == STBLIT_MASK_SECOND_PASS) && (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND))
                {
                    GetFilterMode(Src2_p->Data.Bitmap_p->ColorType,ClutEnable,ClutMode,TRUE,&FilterMode);
                }
                else
                {
                    GetFilterMode(Src2_p->Data.Bitmap_p->ColorType,ClutEnable,ClutMode,FALSE,&FilterMode);
                }

            }

            SetNodeResize (Node_p, Src2_p->Rectangle.Width,Src2_p->Rectangle.Height,
                        Dst_p->Rectangle.Width,Dst_p->Rectangle.Height,Device_p,FilterMode, CommonField_p
                        #if defined (WA_GNBvd10454)
                        , &HResizeOn, &VResizeOn
                        #endif
                        );


#if defined (WA_GNBvd10454)
            if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1) &&
                (((BlitContext_p->EnableFlickerFilter == TRUE) || (VResizeOn == TRUE)) &&
                  (HResizeOn == FALSE)))
            {
                /* Horizontal 2D resize control */
                CommonField_p->RZC |= ((U32)(STBLIT_RZC_2DHRESIZER_ENABLE & STBLIT_RZC_2DHRESIZER_MODE_MASK) << STBLIT_RZC_2DHRESIZER_MODE_SHIFT);

                /* Horizontal initial subposition */
                CommonField_p->RZC |= ((U32)(0 & STBLIT_RZC_INITIAL_SUBPOS_HSRC_MASK) << STBLIT_RZC_INITIAL_SUBPOS_HSRC_SHIFT);

                /* Set horizontal scaling factor to 1024 (x 1) */
                RSF = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)));
                RSF &= ((U32)(~((U32)(STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT)));
                RSF |= ((U32)(1024 & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)),RSF);
            }
#endif

#if defined (WA_GNBvd49348)
            if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) && (BlitContext_p->EnableFlickerFilter == TRUE))
            {
                /* Instruction : Enable 2D Resize */
                CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_RESCALE_MASK) << STBLIT_INS_ENABLE_RESCALE_SHIFT);

                /* Horizontal 2D resize control */
                CommonField_p->RZC |= ((U32)(STBLIT_RZC_2DHRESIZER_ENABLE & STBLIT_RZC_2DHRESIZER_MODE_MASK) << STBLIT_RZC_2DHRESIZER_MODE_SHIFT);

                /* Horizontal initial subposition */
                CommonField_p->RZC |= ((U32)(0 & STBLIT_RZC_INITIAL_SUBPOS_HSRC_MASK) << STBLIT_RZC_INITIAL_SUBPOS_HSRC_SHIFT);

                /* Set horizontal scaling factor to 1024 (x 1) */
                RSF = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)));
                RSF &= ((U32)(~((U32)(STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT)));
                RSF |= ((U32)(1024 & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)),RSF);
            }
#endif


        }
    }

    /* Set Input converter */
    SetNodeInputConverter(InEnable, InConfig, CommonField_p);

    /* Set Output converter */
    SetNodeOutputConverter(OutEnable,OutConfig, CommonField_p);

    if (ClutEnable) /* Expansion or Reduction */
    {
        if (ClutMode == 0)
        {
            /* Set Color Expansion . In this case, Src2 is always not NULL (Src2 is NULL only if Table Single Src1 is used
             * and there is no CLUT expansion not reduction in this table) */
            SetNodeColorExpansion(Src2_p->Palette_p,Device_p, CommonField_p);
        }
        else if (ClutMode == 2)
        {
            /* Set Color reduction */
            SetNodeColorReduction(Dst_p->Palette_p,Device_p, CommonField_p);
        }
    }
    else if (BlitContext_p->EnableColorCorrection == TRUE)
    {
        /* Condition of correction support TBD */

        /* Set Color correction */
        SetNodeColorCorrection(BlitContext_p,Device_p, CommonField_p);
    }

    /* Set ALU */
    if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)   /* In this case Both Src are not always not NULL! (cf upper level)*/
    {
        if (Src2_p->Type == STBLIT_SOURCE_TYPE_BITMAP)
        {
            SetNodeALUBlend(BlitContext_p->GlobalAlpha ,Src2_p->Data.Bitmap_p->PreMultipliedColor, MaskMode, CommonField_p);
        }
        else /* Type Color */
        {
            SetNodeALUBlend(BlitContext_p->GlobalAlpha ,FALSE, MaskMode, CommonField_p);
        }
        /* Update pre-multiplied info in destination if needed ...TBDone */
    }
    else if (SrcMB == TRUE)  /* MB mode */
    {
        /* Bypass Src2 for mode MB */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
    }
    else /* Raster */
    {
        SetNodeALULogic(MaskMode, BlitContext_p, CommonField_p);
    }

    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->ConcatMode        = FALSE;
        Config_p->SrcMB             = SrcMB;
        if (Src1_p != NULL)
        {
            if ((Src1_p->Type == STBLIT_SOURCE_TYPE_BITMAP) && (Src1_p->Data.Bitmap_p == Dst_p->Bitmap_p))
            {
                Config_p->IdenticalDstSrc1  = TRUE;
            }
            else
            {
                Config_p->IdenticalDstSrc1  = FALSE;
            }
        }
        else
        {
            Config_p->IdenticalDstSrc1  = FALSE;
        }
        Config_p->OpMode            = STBLIT_SOURCE1_MODE_NORMAL;
        Config_p->XYLMode           = STBLIT_JOB_BLIT_XYL_TYPE_NONE;
        Config_p->ScanConfig        = ScanConfig;
        SetOperationIOConfiguration(Src1_p, Src2_p, Dst_p, MaskMode, Config_p);
    }

    #ifdef STBLIT_BENCHMARK2
    t2 = STOS_time_now();
    #endif

    #ifdef STBLIT_BENCHMARK2
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to Process One pass Blit = %d",t2 - t1));
    #endif

    return(Err);
}

/*******************************************************************************
Name        : stblit_OnePassBlitSliceRectangleOperation
Description : Set one node parameters
Parameters  :
Assumptions :  All parameters checks done at upper level :
               + Device_p is valid pointer never NULL
               + Dst_p is valid pointer never NULL
               + BlitContext_p is valid pointer never NULL
               + CommonField_p is valid pointer never NULL
               + Config_p is valid pointer never NULL
               + SliceRectangle_p is valid pointer never NULL
               + NodeHandle_p is valid pointer never NULL

               Src1_p and Src2_p may be NULL !!!!
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_OnePassBlitSliceRectangleOperation(stblit_Device_t*   Device_p,
                                                        STBLIT_SliceData_t*   Src1_p,
                                                        STBLIT_SliceData_t*   Src2_p,
                                                        STBLIT_SliceData_t*   Dst_p,
                                                        STBLIT_SliceRectangle_t*  SliceRectangle_p,
                                                        STBLIT_BlitContext_t*   BlitContext_p,
                                                        U16                     Code,
                                                        stblit_MaskMode_t       MaskMode,
                                                        stblit_NodeHandle_t*    NodeHandle_p,
                                                        stblit_OperationConfiguration_t*  Config_p,
                                                        stblit_CommonField_t*             CommonField_p)
 {
    ST_ErrorCode_t          Err                 = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    U8                      ClutMode            = 0;  /* CLUT mode */
    U8                      InConfig            = 0;  /* 3 bits : Config  Input Converter
                                                              bit0 =>  Input Color Converter Colorimetry
                                                              bit1 =>  Input Color Converter Chroma format
                                                              bit2 =>  Input Color converter direction */
    U8                      OutConfig           = 0;  /* 2 bits : Config  Output Converter
                                                              bit0 =>  Output Color Converter Colorimetry
                                                              bit1 =>  Output Color Converter Chroma format*/
    U8                      OutEnable           = 0;  /* Enable Output converter */
    U8                      InEnable            = 0;  /* Enable Input converter */
    U8                      ClutEnable          = 0;  /* Enable CLUT mode */
    U8                      ScanConfig          = 0;  /* Vertical and Horizontal scan configuration. Down Right per default */
    U8                      OverlapCode;              /* Resulting overlap direction code both src */
    U8                      Src1Code            = 0xF;  /* Overlap direction code Src1 */
    U8                      Src2Code            = 0xF;  /* Overlap direction code Src2 */
    U32                     DstXStop;
    U32                     DstYStop;
    U32                     Src1XStop;
    U32                     Src1YStop;
    U32                     Src2XStop;
    U32                     Src2YStop;
    U8                      FilterMode;
    STGXOBJ_Rectangle_t     DstRectangle;
    STGXOBJ_Rectangle_t     Src1Rectangle;
    STGXOBJ_Rectangle_t     Src2Rectangle;
    STBLIT_Source_t         Src1, Src2;
    STBLIT_Source_t*        Source1_p       = NULL; /* Default value */
    STBLIT_Source_t*        Source2_p       = NULL; /* Default value */
    STBLIT_Destination_t    Dst;
#if defined (WA_GNBvd10454)
    BOOL                    HResizeOn = FALSE;
    BOOL                    VResizeOn = FALSE;
    U32                     RSF       = 0;
#endif

#ifdef WA_GNBvd49348
    U32                     RSF = 0;
#endif


    /* Set Default rectangle for Src1, Src1 and Dst */

    /* Destination settings */
    Src2Rectangle = SliceRectangle_p->SrcRectangle;
    if (MaskMode != STBLIT_MASK_FIRST_PASS)
    {
        DstRectangle = SliceRectangle_p->DstRectangle;
        Src1Rectangle = SliceRectangle_p->DstRectangle;
    }
    else /* First pass mask, Dst and Src1 rectangle have the same slice rectangle */
    {
        DstRectangle = SliceRectangle_p->SrcRectangle;
        Src1Rectangle = SliceRectangle_p->SrcRectangle;
    }

    /* Dst related */
    if (Dst_p->UseSliceRectanglePosition == FALSE)
    {
        DstRectangle.PositionX = 0;
        DstRectangle.PositionY = 0;
    }

    DstXStop  = DstRectangle.PositionX + (U32)(DstRectangle.Width) - 1 ;
    DstYStop  = DstRectangle.PositionY + (U32)(DstRectangle.Height) - 1 ;

    /* Source1 related*/
    if (Src1_p != NULL)
    {
        /* Settings */
        if (Src1_p->UseSliceRectanglePosition == FALSE)
        {
            Src1Rectangle.PositionX = 0;
            Src1Rectangle.PositionY = 0;
        }

        /* Overlap Src1/Dst*/
        if (Src1_p->Bitmap_p == Dst_p->Bitmap_p)
        {
            Src1XStop  = Src1Rectangle.PositionX + (U32)(Src1Rectangle.Width) - 1 ;
            Src1YStop  = Src1Rectangle.PositionY + (U32)(Src1Rectangle.Height) - 1 ;
            Src1Code   = OverlapOneSourceCoordinate(Src1Rectangle.PositionX,Src1Rectangle.PositionY, Src1XStop,Src1YStop,
                                                    DstRectangle.PositionX, DstRectangle.PositionY, DstXStop,DstYStop);
        }
    }

    /* Source2 related*/
    if (Src2_p != NULL)
    {
        /* Settings */
        if (Src2_p->UseSliceRectanglePosition == FALSE)
        {
            Src2Rectangle.PositionX = 0;
            Src2Rectangle.PositionY = 0;
        }

        /* Overlap Src2/Dst*/
        if (Src2_p->Bitmap_p == Dst_p->Bitmap_p)
        {
            Src2XStop  = Src2Rectangle.PositionX + (U32)(Src2Rectangle.Width) - 1 ;
            Src2YStop  = Src2Rectangle.PositionY + (U32)(Src2Rectangle.Height) - 1 ;
            Src2Code   = OverlapOneSourceCoordinate(Src2Rectangle.PositionX,Src2Rectangle.PositionY,Src2XStop,Src2YStop,
                                                    DstRectangle.PositionX, DstRectangle.PositionY,DstXStop,DstYStop);
        }
    }

    OverlapCode = (Src1Code & Src2Code) & 0xF;
    if (OverlapCode != 0)
    {
        ScanConfig = ScanDirectionTable[OverlapCode];
    }
    else /* Code = 0 => Overlap Not Supported */
    {
        return(STBLIT_ERROR_OVERLAP_NOT_SUPPORTED);
    }


    /* Get Node descriptor handle*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
        }
    }
    else  /* Blit in Job */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_JOB_NODE);
        }
    }

    Node_p  = (stblit_Node_t*)(*NodeHandle_p);

    /* Decode U16 code :
     *  ________________________________________________________________________________
     * |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
     * | 1  |  1 |    |    |    |    | 1  | 1  | 1  | 1  | 1  | 1  | 1  | 1  | 1  |  1 |
     * |____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|
     *   |    |                         |    |    |    |    |    |    |    |    |    |
     *   |____|                         |____|    |    |    |    |    |    |    |    |
     *   |                              |         |    |    |    |    |    |    |    |
     *  \/                             \/        \/    |   \/    |   \/   \/   \/   \/
     * Conversion                    CLUT      Out     | In RGB  |  In    Ena  Ena  Ena
     * Supported :                   Mode      Signed  |  To     |Matrix  Out  CLUT  In
     * 1X : Not supported                             \/  YCbCr \/  709
     * 00 : Ok                                       Out        In
     * 01 : Ok if color correction enabled          Matrix 709  Signed
     *
     *
     * */

    ClutMode    = (U8)((Code >> 8) & 0x3);
    InConfig    = (U8)((Code >> 3) & 0x7);
    OutConfig   = (U8)((Code >> 6) & 0x3);
    OutEnable   = (U8)((Code >> 2) & 0x1);
    ClutEnable  = (U8)((Code >> 1) & 0x1);
    InEnable    = (U8)(Code  & 0x1);

#ifdef WA_GNBvd13604
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020)  ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528)  ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
    {
        if ((ClutEnable == 1) && (ClutMode == 0) && (Src2_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ACLUT44))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
#endif

    /* Init node */
    InitNode(Node_p, BlitContext_p);

    /* Set Target */
    SetNodeTgt(Node_p, Dst_p->Bitmap_p,&(DstRectangle), ScanConfig,Device_p);

    /* Set Src1 (same Rectangle as Dst)*/
    if (Src1_p == NULL)   /* Src1 disabled */
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_DISABLED & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);
    }
    else
    {
        SetNodeSrc1Bitmap(Node_p, Src1_p->Bitmap_p, &(Src1Rectangle),ScanConfig,Device_p, STBLIT_SOURCE1_MODE_NORMAL,CommonField_p );
    }

    /* Set Src2  */
    if (Src2_p == NULL)   /* Src2 disabled */
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_DISABLED & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);
    }
    else
    {
        SetNodeSrc2Bitmap(Node_p, Src2_p->Bitmap_p,&(Src2Rectangle),ScanConfig,Device_p, CommonField_p);
    }

    /* Set Interrupts */
    SetNodeIT (Node_p,BlitContext_p, CommonField_p);

    if (MaskMode != STBLIT_MASK_FIRST_PASS)
    {
        /* Set Clipping */
        if (BlitContext_p->EnableClipRectangle == TRUE)
        {
            SetNodeClipping(Node_p,&(SliceRectangle_p->ClipRectangle), TRUE, CommonField_p);
        }

        /* Set Plane Mask */
        if (BlitContext_p->EnableMaskWord == TRUE)
        {
            SetNodePlaneMask(Node_p, BlitContext_p->MaskWord, CommonField_p);
        }

        /* Set ColorKey */
        if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE)
        {
            SetNodeColorKey(Node_p, BlitContext_p->ColorKeyCopyMode, &(BlitContext_p->ColorKey), CommonField_p);
        }
    }

    /* Set Trigger */
    if (BlitContext_p->Trigger.EnableTrigger == TRUE)
    {
        SetNodeTrigger(Node_p, BlitContext_p, CommonField_p);
    }

    /* If Mask first pass, there must be no resize since Mask size = Foreground size. On top
     * of that there is no reason to filter on first pass. Flicker will be done on 2nd pass if any*/
    if ((Src2_p != NULL) && (MaskMode != STBLIT_MASK_FIRST_PASS))
    {
        /* Set Flicker filter only if Src2 is a bitmap */
        if (BlitContext_p->EnableFlickerFilter == TRUE)
        {
            SetNodeFlicker(Node_p,Src2_p->Bitmap_p->ColorType,Device_p->FlickerFilterMode, CommonField_p);
        }

        /* Set Resize */
        /* Src1 and Dst have same size. Only resize possible between Src2 and Dst  */
        if ((MaskMode == STBLIT_MASK_SECOND_PASS) && (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND))
        {
            GetFilterMode(Src2_p->Bitmap_p->ColorType,ClutEnable,ClutMode,TRUE,&FilterMode);
        }
        else
        {
            GetFilterMode(Src2_p->Bitmap_p->ColorType,ClutEnable,ClutMode,FALSE,&FilterMode);
        }

        SetNodeSliceResize(Node_p,Device_p,FilterMode,SliceRectangle_p, CommonField_p
                          #if defined (WA_GNBvd10454)
                           , &HResizeOn, &VResizeOn
                          #endif
                           );

#if defined (WA_GNBvd10454)
            if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1) &&
                (((BlitContext_p->EnableFlickerFilter == TRUE) || (VResizeOn == TRUE)) &&
                  (HResizeOn == FALSE)))
            {
                /* Horizontal 2D resize control */
                CommonField_p->RZC |= ((U32)(STBLIT_RZC_2DHRESIZER_ENABLE & STBLIT_RZC_2DHRESIZER_MODE_MASK) << STBLIT_RZC_2DHRESIZER_MODE_SHIFT);

                /* Horizontal initial subposition */
                CommonField_p->RZC |= ((U32)(0 & STBLIT_RZC_INITIAL_SUBPOS_HSRC_MASK) << STBLIT_RZC_INITIAL_SUBPOS_HSRC_SHIFT);

                /* Set horizontal scaling factor to 1024 (x 1) */
                RSF = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)));
                RSF &= ((U32)(~((U32)(STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT)));
                RSF |= ((U32)(1024 & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)),RSF);
            }
#endif

#if defined (WA_GNBvd49348)
            if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) && (BlitContext_p->EnableFlickerFilter == TRUE))
            {
                /* Instruction : Enable 2D Resize */
                CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_RESCALE_MASK) << STBLIT_INS_ENABLE_RESCALE_SHIFT);

                /* Horizontal 2D resize control */
                CommonField_p->RZC |= ((U32)(STBLIT_RZC_2DHRESIZER_ENABLE & STBLIT_RZC_2DHRESIZER_MODE_MASK) << STBLIT_RZC_2DHRESIZER_MODE_SHIFT);

                /* Horizontal initial subposition */
                CommonField_p->RZC |= ((U32)(0 & STBLIT_RZC_INITIAL_SUBPOS_HSRC_MASK) << STBLIT_RZC_INITIAL_SUBPOS_HSRC_SHIFT);

                /* Set horizontal scaling factor to 1024 (x 1) */
                RSF = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)));
                RSF &= ((U32)(~((U32)(STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT)));
                RSF |= ((U32)(1024 & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)),RSF);
            }
#endif

    }

    /* Set Input converter */
    SetNodeInputConverter(InEnable, InConfig, CommonField_p);

    /* Set Output converter */
    SetNodeOutputConverter(OutEnable,OutConfig, CommonField_p);

    if (ClutEnable) /* Expansion or Reduction */
    {
        if (ClutMode == 0)
        {
            /* Set Color Expansion. In this case, Src2 is always not NULL (Src2 is NULL only if Table Single Src1 is used
             * and there is no CLUT expansion not reduction in this table) */
            SetNodeColorExpansion(Src2_p->Palette_p,Device_p, CommonField_p);
        }
        else if (ClutMode == 2)
        {
            /* Set Color reduction */
            SetNodeColorReduction(Dst_p->Palette_p,Device_p, CommonField_p);
        }
    }
    else if (BlitContext_p->EnableColorCorrection == TRUE)
    {
        /* Condition of correction support TBD */

        /* Set Color correction */
        SetNodeColorCorrection(BlitContext_p,Device_p, CommonField_p);
    }

    /* Set ALU */
    if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)  /* Both Src never NULL in this case (cf upper level)*/
    {
        SetNodeALUBlend(BlitContext_p->GlobalAlpha ,Src2_p->Bitmap_p->PreMultipliedColor, MaskMode, CommonField_p);
        /* Update pre-multiplied info in destination if needed ...TBDone */
    }
    else
    {
        SetNodeALULogic(MaskMode, BlitContext_p, CommonField_p);
    }

    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->ConcatMode        = FALSE;
        Config_p->SrcMB             = FALSE;
        if (Src1_p != NULL)
        {
            if (Src1_p->Bitmap_p == Dst_p->Bitmap_p)
            {
                Config_p->IdenticalDstSrc1  = TRUE;
            }
            else
            {
                Config_p->IdenticalDstSrc1  = FALSE;
            }

            Src1.Type           = STBLIT_SOURCE_TYPE_BITMAP;
            Src1.Data.Bitmap_p  = Src1_p->Bitmap_p;
            Src1.Rectangle      = Src1Rectangle;
            Src1.Palette_p      = Src1_p->Palette_p;
            Source1_p           = &Src1;
        }
        else
        {
            Config_p->IdenticalDstSrc1  = FALSE;
        }
        Config_p->OpMode       = STBLIT_SOURCE1_MODE_NORMAL;
        Config_p->XYLMode      = STBLIT_JOB_BLIT_XYL_TYPE_NONE;
        Config_p->ScanConfig   = ScanConfig;

        Dst.Bitmap_p    = Dst_p->Bitmap_p;
        Dst.Rectangle   = DstRectangle;
        Dst.Palette_p   = Dst_p->Palette_p;

        if (Src2_p != NULL)
        {
            Src2.Type           = STBLIT_SOURCE_TYPE_BITMAP;
            Src2.Data.Bitmap_p  = Src2_p->Bitmap_p;
            Src2.Rectangle      = Src2Rectangle;
            Src2.Palette_p      = Src2_p->Palette_p;
            Source2_p           = &Src2;
        }

        SetOperationIOConfiguration(Source1_p, Source2_p, &Dst, STBLIT_NO_MASK, Config_p);
    }



    return(Err);
}


 /*******************************************************************************
Name        : stblit_OnePassCopyOperation
Description : Set one node parameters in particular case of CopyRectangle
Parameters  :
Assumptions : No Color conversions
              All checks done at upper level :
              =>  All pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_OnePassCopyOperation( stblit_Device_t*      Device_p,
                                            STGXOBJ_Bitmap_t*     SrcBitmap_p,
                                            STGXOBJ_Rectangle_t*  SrcRectangle_p,
                                            STGXOBJ_Bitmap_t*     DstBitmap_p,
                                            S32                   DstPositionX,
                                            S32                   DstPositionY,
                                            STBLIT_BlitContext_t* BlitContext_p,
                                            stblit_NodeHandle_t*  NodeHandle_p,
                                            stblit_OperationConfiguration_t*  Config_p,
                                            stblit_CommonField_t*             CommonField_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    U8                      ScanConfig  = 0;  /* Vertical and Horizontal scan configuration. Down Right per default */
    U8                      OverlapCode;  /* Overlap direction code Src/Dst */
    STGXOBJ_Rectangle_t     DstRectangle;
    U32                     DstXStop;
    U32                     DstYStop;
    U32                     SrcXStop;
    U32                     SrcYStop;
    STBLIT_Source_t         Src2;
    STBLIT_Destination_t    Dst;
    stblit_Source1Mode_t    OpMode = STBLIT_SOURCE1_MODE_NORMAL;

    #ifdef STBLIT_BENCHMARK2
    t1 = STOS_time_now();
    #endif

    /* Analyse overlap Src/Dst Only when Dst = Src */
    if (SrcBitmap_p == DstBitmap_p)   /* First implementation TBD */
    {
        DstXStop    = DstPositionX + (U32)(SrcRectangle_p->Width) - 1 ;
        DstYStop    = DstPositionY + (U32)(SrcRectangle_p->Height) - 1 ;
        SrcXStop    = SrcRectangle_p->PositionX + (U32)(SrcRectangle_p->Width) - 1 ;
        SrcYStop    = SrcRectangle_p->PositionY + (U32)(SrcRectangle_p->Height) - 1 ;
        OverlapCode = OverlapOneSourceCoordinate(SrcRectangle_p->PositionX,SrcRectangle_p->PositionY,SrcXStop,SrcYStop,
                                                 DstPositionX,DstPositionY,DstXStop,DstYStop);
        if (OverlapCode != 0)
        {
            ScanConfig = ScanDirectionTable[OverlapCode];
        }
        else /* Code = 0 => Overlap Not Supported */
        {
            return(STBLIT_ERROR_OVERLAP_NOT_SUPPORTED);
        }
    }

    /* Get Node descriptor handle*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
        }
    }
    else  /* Blit in Job */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_JOB_NODE);
        }
    }

    Node_p  = (stblit_Node_t*)(*NodeHandle_p);

    /* Initialize Node */
    InitNode(Node_p, BlitContext_p);

    /* Initialize interruption stuff*/
    SetNodeIT (Node_p,BlitContext_p, CommonField_p);

    /* Look if Direct Copy mode can be enabled:
     * On 70XX, no clipping is allowed   with DC
     * On 55XX, clipping is allowed with DC (but only inside clipping) */
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE))
    {
        ScanConfig = 0; /* Direct Copy will be done by STAVMEM which manages already overlaps! */
        /* In that case it's possible to set DC */
        OpMode = STBLIT_SOURCE1_MODE_DIRECT_COPY;
        SetNodeSrc1Bitmap(Node_p,SrcBitmap_p,SrcRectangle_p,ScanConfig, Device_p,STBLIT_SOURCE1_MODE_DIRECT_COPY,CommonField_p);
    }
#ifndef WA_DIRECT_COPY_FILL_OPERATION
    else if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
              (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
              (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
              (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
              (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
              (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
             (BlitContext_p->EnableClipRectangle == FALSE))
    {
        /* In that case it's possible to set DC */
        OpMode = STBLIT_SOURCE1_MODE_DIRECT_COPY;
        SetNodeSrc1Bitmap(Node_p,SrcBitmap_p,SrcRectangle_p,ScanConfig, Device_p,STBLIT_SOURCE1_MODE_DIRECT_COPY,CommonField_p);
    }
#endif
    else /* Normal Mode */
    {
        /* Sr1 is disabled */
        /* CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_DISABLED & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);*/

        /* Set Src2  */
        SetNodeSrc2Bitmap(Node_p,SrcBitmap_p,SrcRectangle_p,ScanConfig,Device_p,CommonField_p);

        /* Set ALU mode bypass Src2 (copy)*/
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
    }

    /* Set Clipping */
    if (BlitContext_p->EnableClipRectangle == TRUE)
    {
        SetNodeClipping(Node_p,&(BlitContext_p->ClipRectangle), BlitContext_p->WriteInsideClipRectangle,CommonField_p);
    }

    /* Set Target */
    DstRectangle.PositionX = DstPositionX;
    DstRectangle.PositionY = DstPositionY;
    DstRectangle.Width     = SrcRectangle_p->Width;
    DstRectangle.Height    = SrcRectangle_p->Height;
    SetNodeTgt(Node_p,DstBitmap_p,&DstRectangle, ScanConfig,Device_p);


    /* Set Trigger */
    if (BlitContext_p->Trigger.EnableTrigger == TRUE)
    {
        SetNodeTrigger(Node_p, BlitContext_p,CommonField_p);
    }

    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->ConcatMode        = FALSE;
        Config_p->SrcMB             = FALSE;
        Config_p->IdenticalDstSrc1  = FALSE; /* Src1 is either ignored in Normal mode or foreground in case of Mask */
        Config_p->OpMode            = OpMode;
        Config_p->XYLMode           = STBLIT_JOB_BLIT_XYL_TYPE_NONE;
        Config_p->ScanConfig        = ScanConfig;

        Src2.Type                   = STBLIT_SOURCE_TYPE_BITMAP;
        Src2.Data.Bitmap_p          = SrcBitmap_p;
        Src2.Rectangle              = *SrcRectangle_p;
        Dst.Bitmap_p                = DstBitmap_p;
        Dst.Rectangle               = DstRectangle;

        SetOperationIOConfiguration(NULL, &Src2, &Dst, STBLIT_NO_MASK, Config_p);
    }


    #ifdef STBLIT_BENCHMARK2
    t2 = STOS_time_now();
    #endif

    #ifdef STBLIT_BENCHMARK2
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to Process One pass CopyRect = %d",t2 - t1));
    #endif

    return(Err);
}

/*******************************************************************************
Name        : SetMaskPalette
Description : Set Palette in order to perform color expansion on Mask data.
              Create ARGB8888 palette if mask mode (Use to avoid the 2 HW passes
              needed by generic mask mode):
              Mask is considered as CLUT
              Mask is placed on Src2 HW pipe and Color expansion is performed.

               BLEND MODE ALPHA8 MASK  : 129 palette entries are set
               ======================

               If fill color is argb, each ARGB8888 Palette entry is defined as :
               A = (PaletteIndex * a / 128)  (unit = 1/128)
               R = r
               G = g
               B = b

               For YCbCr fill color (ycbcr) in that case :
               A = PaletteIndex
               R = cr
               G = y
               B = cb

               For ACLUT, no blend possible !

               BLEND MODE ALPHA4 MASK  : 16 palette entries are set
               ======================

               If fill color is argb, each ARGB8888 Palette entry is defined as :
               A = (ConvertAlpha4To8[PaletteIndex] * a / 128)  (unit = 1/128)
               R = r
               G = g
               B = b

               For YCbCr fill color (ycbcr) in that case :
               A = ConvertAlpha4To8[PaletteIndex]
               R = cr
               G = y
               B = cb

               For ACLUT, no blend possible !

               BLEND MODE ALPHA1 MASK : Only Palette entries 0 and 1 are set !
               =======================

               If fill color is argb, each ARGB8888 Palette entry is defined as :
               A = 0 (PaletteIndex 0) or = a (PaletteIndex 1)
               R = r
               G = g
               B = b

               For YCbCr fill color (ycbcr) in that case :
               A = 0 (PaletteIndex 0) or = 128 (PaletteIndex 1)
               R = cr
               G = y
               B = cb

               For ACLUT, no blend possible !

               LOGIC MODE ALPHA1 MASK : Only Palette entries 0 and 1 are set !
               =======================

               If fill color is argb, each ARGB8888 Palette entry is defined as :
               A = 255 (PaletteIndex 0) or = a (PaletteIndex 1)
               R = r
               G = g
               B = b

               For YCbCr fill color (ycbcr) in that case :
               A = 255 (PaletteIndex 0) or = 128 (PaletteIndex 1)
               R = cr
               G = y
               B = cb

               For AClut (aI) fill color :
               A = 255 (PaletteIndex 0) or = a (PaletteIndex 1)
               R = 0
               G = 0
               B = i



              For None 32bit formats, Palette entries are filled the same way
              as Source2 formatter !
Parameters  :
Assumptions :  All checks must be done at upper level !  (pointers supposed to be not NULL)
              There is a mask and its format is either Alpha1, Alpha4 or Alpha8
              MaskPalette ColorType = STGXOBJ_COLOR_TYPE_ARGB8888
              MaskPalette data aligned on 128 bit word
              Not every color types  supported!
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetMaskPalette(STGXOBJ_Color_t*      Color_p,
                                              STBLIT_BlitContext_t* BlitContext_p,
                                              STGXOBJ_Palette_t*    MaskPalette_p,
                                              stblit_Device_t*       Device_p)
{
    ST_ErrorCode_t  Err             = ST_NO_ERROR;
    U32             ColorData       = 0; /* Only 24 bit relevant*/
    U32             Alpha           = 0;
    U8*             Address_p;
    int             i;
    U32             ConvertAlpha4To8[16]   = {0, 12, 20, 28, 36, 44, 52, 60, 68, 76, 84, 92, 100, 108, 116, 128};

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* Alpha4 to Alpha8 conversion is done the same way the h/w blitter does :
          => 0 A4 100  apart for special values 0 (0) and 15 (128)

       Alpha4   Alpha8(binary)   Alpha8

        0     0 0000 000 =>      0
        1     0 0001 100 =>      12
        2     0 0010 100 =>      20
        3     0 0011 100 =>      28
        4     0 0100 100 =>      36
        5     0 0101 100 =>      44
        6     0 0110 100 =>      52
        7     0 0111 100 =>      60
        8     0 1000 100 =>      68
        9     0 1001 100 =>      76
        10    0 1010 100 =>      84
        11    0 1011 100 =>      92
        12    0 1100 100 =>      100
        13    0 1101 100 =>      108
        14    1 1110 100 =>      116
        15    1 0000 000 =>      128 */

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Address_p = (U8*)stblit_DeviceToCpu((void*)MaskPalette_p->Data_p);
#else
    Address_p = (U8*)STAVMEM_VirtualToCPU((void*)MaskPalette_p->Data_p, &(Device_p->VirtualMapping));
#endif

    /* Set Color Data + Alpha: cf Source 2 formatter (for non 8 bit fields, MSB are set to O)
     * Alpha follow the internal representation, ie it is always 0:128 range!  Other range must be converted! */
    if (Color_p->Type == STGXOBJ_COLOR_TYPE_RGB888)
    {
        ColorData = (U32)((Color_p->Value.RGB888.R << 16) |
                          (Color_p->Value.RGB888.G << 8)  |
                          (Color_p->Value.RGB888.B)) ;
        Alpha     = 128;
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB8888)
    {
        ColorData = (U32)((Color_p->Value.ARGB8888.R << 16) |
                          (Color_p->Value.ARGB8888.G << 8)  |
                          (Color_p->Value.ARGB8888.B)) ;
        Alpha     =  Color_p->Value.ARGB8888.Alpha;
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB8888_255)
    {
        ColorData = (U32)((Color_p->Value.ARGB8888.R << 16) |
                          (Color_p->Value.ARGB8888.G << 8)  |
                          (Color_p->Value.ARGB8888.B)) ;
        Alpha     =  (Color_p->Value.ARGB8888.Alpha + 1) >> 1; /* To get 0:128 range */
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_RGB565)
    {
        ColorData = (U32)(((Color_p->Value.RGB565.R & 0x1F) << 19) |
                          ((Color_p->Value.RGB565.G & 0x3F) << 10) |
                          ((Color_p->Value.RGB565.B & 0x1F) << 3));
        Alpha     = 128;
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB8565)
    {
        ColorData = (U32)(((Color_p->Value.ARGB8565.R & 0x1F)  << 19) |
                          ((Color_p->Value.ARGB8565.G & 0x3F)  << 10) |
                          ((Color_p->Value.ARGB8565.B & 0x1F)  << 3));
        Alpha     =  Color_p->Value.ARGB8565.Alpha;
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB8565_255)
    {
        ColorData = (U32)(((Color_p->Value.ARGB8565.R & 0x1F)  << 19) |
                          ((Color_p->Value.ARGB8565.G & 0x3F)  << 10) |
                          ((Color_p->Value.ARGB8565.B & 0x1F)  << 3));
        Alpha     =  (Color_p->Value.ARGB8565.Alpha + 1) >> 1;    /* To get 0:128 range */
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB1555)
    {
        ColorData = (U32)(((Color_p->Value.ARGB1555.R & 0x1F)  << 19) |
                          ((Color_p->Value.ARGB1555.G & 0x1F)  << 11) |
                          ((Color_p->Value.ARGB1555.B & 0x1F)  << 3));
        Alpha     =  (U8)((Color_p->Value.ARGB1555.Alpha & 0x1) << 7);
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ARGB4444)
    {
        ColorData = (U32)(((Color_p->Value.ARGB4444.R & 0xF)  << 20) |
                          ((Color_p->Value.ARGB4444.G & 0xF)  << 12) |
                          ((Color_p->Value.ARGB4444.B & 0xF)  << 4));

        if (Color_p->Value.ARGB4444.Alpha == 0xF)
        {
            Alpha  =  128;
        }
        else  if (Color_p->Value.ARGB4444.Alpha == 0)
        {
            Alpha = 0;
        }
        else
        {
            Alpha  =  (U8)(((Color_p->Value.ARGB4444.Alpha & 0xF) << 3) | 0x4);
        }
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444)
    {
        ColorData = (U32)((Color_p->Value.SignedYCbCr888_444.Cr << 16) |
                          (Color_p->Value.SignedYCbCr888_444.Y  << 8)  |
                          (Color_p->Value.SignedYCbCr888_444.Cb)) ;
        Alpha     = 128;
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444)
    {
        ColorData = (U32)((Color_p->Value.UnsignedYCbCr888_444.Cr << 16) |
                          (Color_p->Value.UnsignedYCbCr888_444.Y  << 8)  |
                          (Color_p->Value.UnsignedYCbCr888_444.Cb)) ;
        Alpha     = 128;
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT8)
    {
        ColorData = (U32)(Color_p->Value.CLUT8);
        Alpha     = 128;  /* Don't care */

    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ACLUT88)
    {
        ColorData = (U32)(Color_p->Value.ACLUT88.PaletteEntry);
        Alpha     = Color_p->Value.ACLUT88.Alpha;
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ACLUT88_255)
    {
        ColorData = (U32)(Color_p->Value.ACLUT88.PaletteEntry);
        Alpha     = (Color_p->Value.ACLUT88.Alpha + 1) >> 1;      /* To get 0:128 range */
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT4)
    {
        ColorData = (U32)(Color_p->Value.CLUT4 & 0xF);
        Alpha     = 128;  /* Don't care */
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_ACLUT44)
    {
        ColorData = (U32)(Color_p->Value.ACLUT44.PaletteEntry & 0xF);
        if (Color_p->Value.ACLUT44.Alpha == 0xF)
        {
            Alpha  =  128;
        }
        else  if (Color_p->Value.ACLUT44.Alpha == 0)
        {
            Alpha = 0;
        }
        else
        {
            Alpha  =  (U8)(((Color_p->Value.ACLUT44.Alpha & 0xF) << 3) | 0x4);
        }
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT2)
    {
        ColorData = (U32)(Color_p->Value.CLUT2 & 0x3);
        Alpha     = 128;  /* Don't care */
    }
    else if (Color_p->Type == STGXOBJ_COLOR_TYPE_CLUT1)
    {
        ColorData = (U32)(Color_p->Value.CLUT1 & 0x1);
        Alpha     = 128;  /* Don't care */
    }

    /* Set Palette entries */
    if(BlitContext_p->MaskBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA1)
    {
            if (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND)  /* Logic Operation */
            {
                /* Put 255 in alpha pipe (cf Logic Mask Mode 2nd pass)*/
                STSYS_WriteRegMem32LE((void*)Address_p,(U32)(ColorData | (((U32)0xFF) << 24)));

            }
            else  /* Blend operation */
            {
                /* Alpha is 0 (Transparent) */
                STSYS_WriteRegMem32LE((void*)Address_p,(U32)ColorData);
            }
             Address_p              += 4;
             STSYS_WriteRegMem32LE((void*)Address_p,(U32) (ColorData | (Alpha << 24)));
    }
    else if (BlitContext_p->MaskBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA4)
    {
        for (i=0;i<16;i++)
        {
            STSYS_WriteRegMem32LE((void*)Address_p,(U32) (ColorData | ((((Alpha  *  ConvertAlpha4To8[i]) / 128) & 0xFF) << 24)));
            Address_p           += 4;

        }
    }
    else  /* STGXOBJ_COLOR_TYPE_ALPHA8 */
    {
        for (i=0;i<129;i++) /* A8 -> range 0 to 128 => 129 possibilities. Don't care about range 129 to 255! */
        {
            STSYS_WriteRegMem32LE((void*)Address_p,(U32) (ColorData | ((((Alpha  *  i) / 128) & 0xFF) << 24)));
            Address_p           += 4;
        }
    }

    return(Err);
}

/*******************************************************************************
Name        : stblit_OnePassFillOperation
Description : Set one node parameters in particular case of FillRectangle
Parameters  :
Assumptions : No Color conversions
              First implementation : If Mask, Mask rectangle = Dst Rectangle. No resize!
                                              Mask Pitch = Bitmap Pitch (For overlap checking)
              Mask is either Alpha1 or Alpha8
              All parameters checked at upper level :
              => All pointers in parameters are not NULL
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_OnePassFillOperation( stblit_Device_t*      Device_p,
                                            STGXOBJ_Bitmap_t*     Bitmap_p,
                                            STGXOBJ_Rectangle_t*  Rectangle_p,
                                            STGXOBJ_Rectangle_t*  MaskRectangle_p,
                                            STGXOBJ_Color_t*      Color_p,
                                            STBLIT_BlitContext_t* BlitContext_p,
                                            stblit_NodeHandle_t*  NodeHandle_p,
                                            stblit_OperationConfiguration_t*  Config_p,
                                            stblit_CommonField_t*             CommonField_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    STGXOBJ_Palette_t       MaskPalette;
    stblit_MaskMode_t       MaskMode     = STBLIT_NO_MASK;
    STGXOBJ_ColorType_t     OriginalMaskColorType;
    U32                     MaskXStop;
    U32                     MaskYStop;
    U32                     DstXStop;
    U32                     DstYStop;
    U8                      ScanConfig  = 0;  /* Vertical and Horizontal scan configuration. Down Right per default */
    U8                      OverlapCode;  /* Overlap direction code Mask/Bitmap */
    stblit_Source1Mode_t    OpMode = STBLIT_SOURCE1_MODE_NORMAL;
    STBLIT_Source_t         Src2;
    STBLIT_Destination_t    Dst;
    BOOL                    IdenticalDstSrc1 = FALSE;
#if defined (WA_GNBvd10454)
    BOOL                    HResizeOn = FALSE;
    BOOL                    VResizeOn = FALSE;
    U32                     RSF       = 0;
#endif

    #ifdef STBLIT_BENCHMARK2
    t1 = STOS_time_now();
    #endif

    /* Analyse overlap Src/Dst Only when Mask and Mask = Dst ... Make it sence??? */
    if ((BlitContext_p->EnableMaskBitmap == TRUE) && (BlitContext_p->MaskBitmap_p == Bitmap_p))  /* First implementation TBD */
    {
        DstXStop    = Rectangle_p->PositionX + (U32)(Rectangle_p->Width) - 1 ;
        DstYStop    = Rectangle_p->PositionY + (U32)(Rectangle_p->Height) - 1 ;
        MaskXStop   = MaskRectangle_p->PositionX + (U32)(MaskRectangle_p->Width) - 1 ;
        MaskYStop   = MaskRectangle_p->PositionY + (U32)(MaskRectangle_p->Height) - 1 ;
        OverlapCode = OverlapOneSourceCoordinate(MaskRectangle_p->PositionX,MaskRectangle_p->PositionY,
                                                 MaskXStop,MaskYStop,Rectangle_p->PositionX,Rectangle_p->PositionY,
                                                 DstXStop,DstYStop);
        if (OverlapCode != 0)
        {
            ScanConfig = ScanDirectionTable[OverlapCode];
        }
        else /* Code = 0 => Overlap Not Supported */
        {
            return(STBLIT_ERROR_OVERLAP_NOT_SUPPORTED);
        }
    }



    /* Get Node descriptor handle*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
        }
    }
    else  /* Blit in Job */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_JOB_NODE);
        }
    }

    Node_p  = (stblit_Node_t*)(*NodeHandle_p);

    /* Initialize Node */
    InitNode(Node_p, BlitContext_p);

    /* Initialize interruption stuff*/
    SetNodeIT (Node_p,BlitContext_p, CommonField_p);

    if ((BlitContext_p->EnableMaskBitmap == FALSE) &&
        (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE) &&
        (BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
        (BlitContext_p->EnableMaskWord == FALSE) &&
        (BlitContext_p->EnableColorCorrection == FALSE) &&
        (BlitContext_p->EnableFlickerFilter == FALSE) &&
        ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE)
#ifndef WA_DIRECT_COPY_FILL_OPERATION
        || (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            (BlitContext_p->EnableClipRectangle == FALSE))
#endif
        ))
    {
        /* Whatever platform, since there is not any mask, scanConfig always 0 !*/
        /* In that case it's possible to set DF */
        OpMode = STBLIT_SOURCE1_MODE_DIRECT_FILL;
        SetNodeSrc1Color(Node_p,Color_p,Device_p,STBLIT_SOURCE1_MODE_DIRECT_FILL, CommonField_p);
    }
    else /* Normal Mode */
    {
        if (BlitContext_p->EnableMaskBitmap == FALSE)
        {

            if (((BlitContext_p->AluMode == STBLIT_ALU_COPY) || (BlitContext_p->AluMode == STBLIT_ALU_COPY_INVERT)) &&
                (BlitContext_p->EnableMaskWord == FALSE) &&
                (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
            {
                /* Bypass Src2 : Disable Src1 */
                CommonField_p->INS |=((U32)(STBLIT_INS_SRC1_MODE_DISABLED & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);
            }
            else
            {
                /* Set Src1 = Dst if no bypass Src2 */
                SetNodeSrc1Bitmap(Node_p,Bitmap_p,Rectangle_p,ScanConfig,Device_p,STBLIT_SOURCE1_MODE_NORMAL, CommonField_p);
                IdenticalDstSrc1 = TRUE;
            }


            if ((BlitContext_p->AluMode == STBLIT_ALU_NOOP)  ||
                (BlitContext_p->AluMode == STBLIT_ALU_INVERT))
            {
                /* Bypass Src1 : Disable Src2 */
                CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_DISABLED & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);
            }
            else
            {
                /* Set Src2 as a color if no bypass Src1*/
                SetNodeSrc2Color(Node_p,Color_p,Rectangle_p,Device_p,CommonField_p);
            }
        }
        else  /* Mask bitmap enabled */
        {
            U8                      FilterMode;

            /* Set Src1 = Dst */
            SetNodeSrc1Bitmap(Node_p,Bitmap_p,Rectangle_p,ScanConfig, Device_p,STBLIT_SOURCE1_MODE_NORMAL,CommonField_p);
            IdenticalDstSrc1 = TRUE;

            /* Set Src2 as Mask Bitmap :
            * Assumption Mask rectangle = Dst rectangle!
            * Mask ColorType has to be changed from ALPHA to CLUT */
            OriginalMaskColorType = BlitContext_p->MaskBitmap_p->ColorType;
            if (OriginalMaskColorType == STGXOBJ_COLOR_TYPE_ALPHA1)
            {
                BlitContext_p->MaskBitmap_p->ColorType = STGXOBJ_COLOR_TYPE_CLUT1;
            }
            else if (OriginalMaskColorType == STGXOBJ_COLOR_TYPE_ALPHA4)
            {
                BlitContext_p->MaskBitmap_p->ColorType = STGXOBJ_COLOR_TYPE_CLUT4;
            }
            else /* Mask is Alpha8 */
            {
                BlitContext_p->MaskBitmap_p->ColorType = STGXOBJ_COLOR_TYPE_CLUT8;
            }
            SetNodeSrc2Bitmap(Node_p, BlitContext_p->MaskBitmap_p,MaskRectangle_p,ScanConfig,Device_p,CommonField_p);
            /* Set back mask color type */
            BlitContext_p->MaskBitmap_p->ColorType = OriginalMaskColorType;

            /* Set MaskPalette */
            MaskPalette.ColorType       = STGXOBJ_COLOR_TYPE_ARGB8888;
            MaskPalette.Data_p   = (void*)((U32)(BlitContext_p->WorkBuffer_p) & 0xFFFFFFF0); /* Make sure palette aligned
                                                                                              * on 128 bit words */
            /* MaskPalette.PaletteType  = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
            MaskPalette.ColorDepth      = ;    */
            SetMaskPalette(Color_p,BlitContext_p,&MaskPalette, Device_p);

            /* Set Color Expansion : (No need to put WA_GNBvd13604 because never ACLUT44 for SRC2 ) */
            SetNodeColorExpansion(&MaskPalette,Device_p,CommonField_p);

            /* Set MaskMode */
            MaskMode = STBLIT_MASK_SECOND_PASS;


            FilterMode = STBLIT_2DFILTER_INACTIVE;


            /* Set Resize if any between Mask rectangle and Dst Rectangle. The color format of data arriving in the resize hw block are
            * ARGB8888 because of the palette expansion done just before. Make sure the resize is done without any filter on Alpha
            * pipe in case of logic mask (because of special value 255).  */
            if (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND)     /* Disable filter on alpha */
            {
                if ( BlitContext_p->EnableResizeFilter != FALSE )
                {
                    FilterMode = STBLIT_2DFILTER_ACTIV_COLOR;
                }
                SetNodeResize (Node_p, MaskRectangle_p->Width,MaskRectangle_p->Height,
                            Rectangle_p->Width,Rectangle_p->Height,Device_p, STBLIT_2DFILTER_ACTIV_COLOR,CommonField_p
                            #if defined (WA_GNBvd10454)
                            , &HResizeOn, &VResizeOn
                            #endif
                                );
            }
            else /* Blend */
            {
                if ( BlitContext_p->EnableResizeFilter != FALSE )
                {
                    FilterMode = STBLIT_2DFILTER_ACTIV_BOTH;
                }

                SetNodeResize (Node_p, MaskRectangle_p->Width,MaskRectangle_p->Height,
                              Rectangle_p->Width,Rectangle_p->Height,Device_p, STBLIT_2DFILTER_ACTIV_BOTH,CommonField_p
                            #if defined (WA_GNBvd10454)
                              , &HResizeOn, &VResizeOn
                            #endif
                            );
            }

#if defined (WA_GNBvd10454)
            if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1) &&
                (((BlitContext_p->EnableFlickerFilter == TRUE) || (VResizeOn == TRUE)) &&
                  (HResizeOn == FALSE)))
            {
                /* Horizontal 2D resize control */
                CommonField_p->RZC |= ((U32)(STBLIT_RZC_2DHRESIZER_ENABLE & STBLIT_RZC_2DHRESIZER_MODE_MASK) << STBLIT_RZC_2DHRESIZER_MODE_SHIFT);

                /* Horizontal initial subposition */
                CommonField_p->RZC |= ((U32)(0 & STBLIT_RZC_INITIAL_SUBPOS_HSRC_MASK) << STBLIT_RZC_INITIAL_SUBPOS_HSRC_SHIFT);

                /* Set horizontal scaling factor to 1024 (x 1) */
                RSF = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)));
                RSF &= ((U32)(~((U32)(STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT)));
                RSF |= ((U32)(1024 & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)),RSF);
            }
#endif
        }

        if (BlitContext_p->EnableMaskWord == TRUE)
        {
            SetNodePlaneMask(Node_p, BlitContext_p->MaskWord,CommonField_p);
        }


        /* Set Trigger */
        if (BlitContext_p->Trigger.EnableTrigger == TRUE)
        {
            SetNodeTrigger(Node_p, BlitContext_p,CommonField_p);
        }

        /* Set Color Key */
        if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE)
        {
            SetNodeColorKey(Node_p, BlitContext_p->ColorKeyCopyMode, &(BlitContext_p->ColorKey),CommonField_p);
        }

        /* Set ALU mode */
        if (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND)
        {
            SetNodeALULogic(MaskMode, BlitContext_p,CommonField_p);
        }
        else
        {
            SetNodeALUBlend(BlitContext_p->GlobalAlpha ,FALSE,MaskMode,CommonField_p);
        }

    }

    /* Set Clipping */
    if (BlitContext_p->EnableClipRectangle == TRUE)
    {
        SetNodeClipping(Node_p,&(BlitContext_p->ClipRectangle), BlitContext_p->WriteInsideClipRectangle,CommonField_p);
    }

    /* Set Target */
    SetNodeTgt(Node_p,Bitmap_p,Rectangle_p, ScanConfig, Device_p);

    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->ConcatMode        = FALSE;
        Config_p->SrcMB             = FALSE;
        Config_p->IdenticalDstSrc1  = IdenticalDstSrc1;
        Config_p->OpMode            = OpMode;
        Config_p->XYLMode           = STBLIT_JOB_BLIT_XYL_TYPE_NONE;
        Config_p->ScanConfig        = ScanConfig;

        Src2.Type                   = STBLIT_SOURCE_TYPE_COLOR;
        Src2.Data.Color_p           = Color_p;
        Dst.Bitmap_p                = Bitmap_p;
        Dst.Rectangle               = *Rectangle_p;

        SetOperationIOConfiguration(NULL, &Src2, &Dst, MaskMode, Config_p);
    }

    #ifdef STBLIT_BENCHMARK2
    t2 = STOS_time_now();
    #endif

    #ifdef STBLIT_BENCHMARK2
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to Process One pass FillRect = %d",t2 - t1));
    #endif

    return(Err);
}

/*******************************************************************************
Name        : stblit_OnePassDrawLineOperation
Description : Set one node parameters in particular case of Draw Line
              Vertical or Horizontal
Parameters  :
Assumptions : No Color conversions
              No mask bitmap
              No mask word
              ALU mode Copy only
              All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_OnePassDrawLineOperation( stblit_Device_t*      Device_p,
                                                STGXOBJ_Bitmap_t*     Bitmap_p,
                                                S32                   PositionX,
                                                S32                   PositionY,
                                                U32                   Length,
                                                BOOL                  HorNotVer,
                                                STGXOBJ_Color_t*      Color_p,
                                                STBLIT_BlitContext_t* BlitContext_p,
                                                stblit_NodeHandle_t*  NodeHandle_p,
                                                stblit_OperationConfiguration_t*  Config_p,
                                                stblit_CommonField_t*             CommonField_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    STGXOBJ_Rectangle_t     DstRectangle;
    STBLIT_Source_t         Src2;
    STBLIT_Destination_t    Dst;

    #ifdef STBLIT_BENCHMARK2
    t1 = STOS_time_now();
    #endif


    /* Get Node descriptor handle*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
        }
    }
    else  /* Blit in Job */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_JOB_NODE);
        }
    }

    Node_p  = (stblit_Node_t*)(*NodeHandle_p);

    /* Initialize Node */
    InitNode(Node_p, BlitContext_p);

    /* Initialize interruption stuff*/
    SetNodeIT (Node_p,BlitContext_p,CommonField_p);

    /* Mode Copy : Bypass Src2 -> Sr1 is disabled */
    /* CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_DISABLED & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);*/

    /* Set Destiantion rectangle  */
    DstRectangle.PositionX = PositionX;
    DstRectangle.PositionY = PositionY;
    if (HorNotVer == TRUE)  /* Draw Horizontal line */
    {
        DstRectangle.Width     = Length;
        DstRectangle.Height    = 1;
    }
    else /* Draw Vertical line */
    {
        DstRectangle.Width     = 1;
        DstRectangle.Height    = Length;
    }

    /* Set Src2  */
    SetNodeSrc2Color(Node_p,Color_p,&DstRectangle,Device_p,CommonField_p);

    /* Set Target */
    SetNodeTgt(Node_p,Bitmap_p,&DstRectangle, 0,Device_p);

    /* Set Clipping */
    if (BlitContext_p->EnableClipRectangle == TRUE)
    {
        SetNodeClipping(Node_p,&(BlitContext_p->ClipRectangle), BlitContext_p->WriteInsideClipRectangle,CommonField_p);
    }


    /* Set Trigger */
    if (BlitContext_p->Trigger.EnableTrigger == TRUE)
    {
        SetNodeTrigger(Node_p, BlitContext_p,CommonField_p);
    }

    /* Set ALU mode bypass Src2 (copy)*/
    CommonField_p->ACK |=((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);

    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->ConcatMode        = FALSE;
        Config_p->SrcMB             = FALSE;
        Config_p->IdenticalDstSrc1  = FALSE; /* Src1 is always disabled */
        Config_p->OpMode            = STBLIT_SOURCE1_MODE_NORMAL;
        Config_p->XYLMode           = STBLIT_JOB_BLIT_XYL_TYPE_NONE;
        Config_p->ScanConfig        = 0;

        Src2.Type                   = STBLIT_SOURCE_TYPE_COLOR;
        Src2.Data.Color_p           = Color_p;
        Dst.Bitmap_p                = Bitmap_p;
        Dst.Rectangle               = DstRectangle;

        SetOperationIOConfiguration(NULL, &Src2, &Dst, STBLIT_NO_MASK, Config_p);
    }


    #ifdef STBLIT_BENCHMARK2
    t2 = STOS_time_now();
    #endif

    #ifdef STBLIT_BENCHMARK2
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "time to Process One pass DrawLine = %d",t2 - t1));
    #endif

    return(Err);
}


/*******************************************************************************
Name        : stblit_OnePassDrawHLineOperation
Description : Set one node parameters according to the context.
              This Function is USED BY DrawArray API functions when no XYL engine
                               =======
              Its context analysis is richer than stblit_OnePassDrawLineOperation().
Parameters  :
Assumptions : No Color conversions
              No Mask support (First implementation, Node is STBLIT_NODE_TYPE_STANDALONE)
              The Access control is not performed inthe function but at upper level
              All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_OnePassDrawHLineOperation( stblit_Device_t*      Device_p,
                                                 STGXOBJ_Bitmap_t*     Bitmap_p,
                                                 S32                   PositionX,
                                                 S32                   PositionY,
                                                 U32                   Length,
                                                 STGXOBJ_Color_t*      Color_p,
                                                 STBLIT_BlitContext_t* BlitContext_p,
                                                 stblit_NodeHandle_t*  NodeHandle_p,
                                                 stblit_OperationConfiguration_t*  Config_p,
                                                 stblit_CommonField_t*             CommonField_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    STGXOBJ_Rectangle_t     DstRectangle;
    STBLIT_Source_t         Src2;
    STBLIT_Destination_t    Dst;
    BOOL                    IdenticalDstSrc1 = FALSE;

    /* Get Node descriptor handle*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
    {
        Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) NodeHandle_p);
        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
        }
    }
    else  /* Blit in Job */
    {
        Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) NodeHandle_p);
        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_JOB_NODE);
        }
    }

    Node_p  = (stblit_Node_t*)(*NodeHandle_p);

    /* Initialize Node */
    InitNode(Node_p, BlitContext_p);

    /* Initialize interruption stuff concerning only suspension (Completion added only for last node of array !)*/
    CommonField_p->INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_SUSPENDED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);

    /* Set Dst Rectangle */
    DstRectangle.PositionX = PositionX;
    DstRectangle.PositionY = PositionY;
    DstRectangle.Width     = Length;
    DstRectangle.Height    = 1;


    if (((BlitContext_p->AluMode == STBLIT_ALU_COPY) || (BlitContext_p->AluMode == STBLIT_ALU_COPY_INVERT))&&
        (BlitContext_p->EnableMaskWord == FALSE)&&
        (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
    {
        /* Bypass Src2 : Disable Src1 */
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_DISABLED & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);
    }
    else
    {
        /* Set Src1 = Dst if no bypass Src2 */
        SetNodeSrc1Bitmap(Node_p,Bitmap_p,&DstRectangle,0,Device_p,STBLIT_SOURCE1_MODE_NORMAL, CommonField_p);
        IdenticalDstSrc1 = TRUE;
    }


    if ((BlitContext_p->AluMode == STBLIT_ALU_NOOP)  ||
        (BlitContext_p->AluMode == STBLIT_ALU_INVERT))
    {
        /* Bypass Src1 : Disable Src2 */
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_DISABLED & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);
    }
    else
    {
        /* Set Src2 as a color if no bypass Src1*/
        SetNodeSrc2Color(Node_p,Color_p,&DstRectangle, Device_p,CommonField_p);
    }


    /* Set Target */
    SetNodeTgt(Node_p,Bitmap_p,&DstRectangle,0,Device_p);

    if (BlitContext_p->EnableMaskWord == TRUE)
    {
        SetNodePlaneMask(Node_p, BlitContext_p->MaskWord, CommonField_p);
    }

    /* Set Trigger */
    if (BlitContext_p->Trigger.EnableTrigger == TRUE)
    {
        SetNodeTrigger(Node_p, BlitContext_p, CommonField_p);
    }

    /* Set Clipping */
    if (BlitContext_p->EnableClipRectangle == TRUE)
    {
        SetNodeClipping(Node_p,&(BlitContext_p->ClipRectangle), BlitContext_p->WriteInsideClipRectangle, CommonField_p);
    }


    /* Set Color Key */
    if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE)
    {
        SetNodeColorKey(Node_p, BlitContext_p->ColorKeyCopyMode, &(BlitContext_p->ColorKey), CommonField_p);
    }

    /* Set ALU mode */
    if (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND)
    {
        SetNodeALULogic(STBLIT_NO_MASK, BlitContext_p, CommonField_p);
    }
    else
    {
        SetNodeALUBlend(BlitContext_p->GlobalAlpha ,FALSE,STBLIT_NO_MASK, CommonField_p);
    }

    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->ConcatMode        = FALSE;
        Config_p->SrcMB             = FALSE;
        Config_p->IdenticalDstSrc1  = IdenticalDstSrc1;
        Config_p->OpMode            = STBLIT_SOURCE1_MODE_NORMAL;
        Config_p->XYLMode           = STBLIT_JOB_BLIT_XYL_TYPE_EMULATED;
        Config_p->ScanConfig        = 0;

        Src2.Type                   = STBLIT_SOURCE_TYPE_COLOR;
        Src2.Data.Color_p           = Color_p;
        Dst.Bitmap_p                = Bitmap_p;
        Dst.Rectangle               = DstRectangle;

        SetOperationIOConfiguration(NULL, &Src2, &Dst, STBLIT_NO_MASK, Config_p);
    }



    return(Err);
}

/*******************************************************************************
Name        : stblit_OnePassDrawArrayOperation
Description : Set node parameters according to the context.
              This Function is USED BY DrawArray API functions when XYL engine
                               =======
              Its context analysis is richer than stblit_OnePassDrawLineOperation().
Parameters  :
Assumptions : No Color conversions
              The Access control is not performed inthe function but at upper level
              At least 1 array element to draw
              When Mask operation , no memory overlap supported between Dst/Mask
              All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_OnePassDrawArrayOperation( stblit_Device_t*      Device_p,
                                                 STGXOBJ_Bitmap_t*     Bitmap_p,
                                                 stblit_ArrayType_t    ArrayType,
                                                 void*                 Array_p,
                                                 STGXOBJ_Color_t*      Color_p,
                                                 U32                   NbElem,
                                                 STBLIT_BlitContext_t* BlitContext_p,
                                                 stblit_NodeHandle_t*  NodeHandle_p,
                                                 stblit_OperationConfiguration_t*  Config_p,
                                                 stblit_CommonField_t*             CommonField_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    U32                     XYL,Tmp;
    stblit_Node_t*          Node_p;
    BOOL                    IdenticalDstSrc1 = FALSE;
    U32                     Color;
    BOOL                    IsMaskModeEnable = FALSE;

    /* Get Node descriptor handle*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
    {
        Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) NodeHandle_p);
        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
        }
    }
    else  /* Blit in Job */
    {
        Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) NodeHandle_p);
        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_JOB_NODE);
        }
    }

    Node_p  = (stblit_Node_t*)(*NodeHandle_p);

    /* Initialize Node */
    InitNode(Node_p, BlitContext_p);


    /* In case of soft emulation, some acceleration are possible in some cases.
     * Use Reserved Field in Node_p to pass the info (0 no optimization (default), 1 optimized) */
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE)) &&
        (BlitContext_p->EnableMaskBitmap == FALSE) &&
        (BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
        (BlitContext_p->EnableMaskWord == FALSE) &&
        (BlitContext_p->EnableColorCorrection == FALSE) &&
        (BlitContext_p->EnableFlickerFilter == FALSE) &&
        ((BlitContext_p->EnableClipRectangle == FALSE) ||
         ((BlitContext_p->EnableClipRectangle == TRUE) && (BlitContext_p->WriteInsideClipRectangle == TRUE))))
    {
        Node_p->Reserved = 1;
    }

    /* Initialize interruption stuff*/
    SetNodeIT (Node_p,BlitContext_p,CommonField_p);

    /* Set XYL mode On */
    CommonField_p->INS |= ((U32)(STBLIT_INS_XYL_MODE_ENABLE  & STBLIT_INS_XYL_MODE_MASK) << STBLIT_INS_XYL_MODE_SHIFT);

    /* Set XYL Control mode */
    XYL  = ((U32)(STBLIT_XYL_MODE_STANDARD & STBLIT_XYL_MODE_MASK) << STBLIT_XYL_MODE_SHIFT);
    XYL |= ((U32)(ArrayType & STBLIT_XYL_BUFFER_FORMAT_MASK) << STBLIT_XYL_BUFFER_FORMAT_SHIFT);

#if defined (WA_GNBvd12051)
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020))
    {
        NbElem--;
    }
#endif
    XYL |= ((U32)(NbElem & STBLIT_XYL_NB_SUBINST_MASK) << STBLIT_XYL_NB_SUBINST_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_XYL)),XYL);

    /* Set XYP SubInstruction ptr */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) stblit_CpuToDevice((void*)Array_p);
#else
    Tmp = (U32)STAVMEM_VirtualToDevice((void*)Array_p, &(Device_p->VirtualMapping));
#endif
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_XYP)),((U32)(Tmp & STBLIT_XYP_SUBINST_PTR_MASK) << STBLIT_XYP_SUBINST_PTR_SHIFT));

    /* Set Target */
    SetNodeTgtXYL (Node_p,Bitmap_p, Device_p, CommonField_p);

    /* Set Src1 */
    if (((BlitContext_p->AluMode == STBLIT_ALU_COPY) || (BlitContext_p->AluMode == STBLIT_ALU_COPY_INVERT))&&
        (BlitContext_p->EnableMaskWord == FALSE)&&
        (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST) &&
        (BlitContext_p->EnableMaskBitmap == FALSE))
    {
        /* Bypass Src2 : Disable Src1 */
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC1_MODE_DISABLED & STBLIT_INS_SRC1_MODE_MASK) << STBLIT_INS_SRC1_MODE_SHIFT);
    }
    else
    {
        /* Set Src1 = Dst if no bypass Src2 */
        SetNodeSrc1XYL(Node_p,Bitmap_p,Device_p,CommonField_p);
        IdenticalDstSrc1 = TRUE;
    }

    /* Set Color if any */
    if (Color_p != NULL)
    {
        /* Src2 color setting for XYL
         * The color must be set according to output of the internal src formatter
         * This is the contrary of normal fill operation!!!! */
        stblit_FormatColorForOutputFormatter(Color_p,&Color) ;
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2CF)),((U32)(Color & STBLIT_S2CF_COLOR_MASK) << STBLIT_S2CF_COLOR_SHIFT));
    }

    if (BlitContext_p->EnableMaskBitmap == TRUE)
    {
        /* Set Src2 parameters for mask operation */
        SetNodeSrc2MaskXYL(Node_p,BlitContext_p->MaskBitmap_p,&(BlitContext_p->MaskRectangle),Device_p,CommonField_p);
        IsMaskModeEnable = TRUE;
    }
    else
    {
        /* At least one Src pipe (Src1 or Src2) must be enable to start the operation. In case of Src1 disabled, Src2 has to
         * be enable even if there is not any mask operation!
        * At present, Src2 is enabled in all cases */
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_MEMORY & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);
    }


    /* Set Mask word */
    if (BlitContext_p->EnableMaskWord == TRUE)
    {
        SetNodePlaneMask(Node_p, BlitContext_p->MaskWord, CommonField_p);
    }

    /* Set Trigger */
    if (BlitContext_p->Trigger.EnableTrigger == TRUE)
    {
        SetNodeTrigger(Node_p, BlitContext_p, CommonField_p);
    }

    /* Set Clipping */
    if (BlitContext_p->EnableClipRectangle == TRUE)
    {
        SetNodeClipping(Node_p,&(BlitContext_p->ClipRectangle), BlitContext_p->WriteInsideClipRectangle, CommonField_p);
    }

    /* Set Color Key (Only Destination color key can be use )*/
    if (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST)
    {
        SetNodeColorKey (Node_p, BlitContext_p->ColorKeyCopyMode, &(BlitContext_p->ColorKey), CommonField_p);
    }

    /* Set ALU mode */
    if (BlitContext_p->AluMode != STBLIT_ALU_ALPHA_BLEND)
    {
        SetNodeALULogicXYL(IsMaskModeEnable, BlitContext_p, CommonField_p);
    }
    else
    {
        SetNodeALUBlendXYL(BlitContext_p->GlobalAlpha ,FALSE,IsMaskModeEnable, CommonField_p);
    }


    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->XYLMode = STBLIT_JOB_BLIT_XYL_TYPE_DEVICE;
        /* Other fields to be written ??? */
    }


    return(Err);
}



/*******************************************************************************
Name        : stblit_OnePassConcatOperation
Description : Set one node parameters in particular case of Concatenation
Parameters  :
Assumptions : No Color conversions
              All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_OnePassConcatOperation( stblit_Device_t*      Device_p,
                                              STGXOBJ_Bitmap_t*     SrcAlphaBitmap_p,
                                              STGXOBJ_Rectangle_t*  SrcAlphaRectangle_p,
                                              STGXOBJ_Bitmap_t*     SrcColorBitmap_p,
                                              STGXOBJ_Rectangle_t*  SrcColorRectangle_p,
                                              STGXOBJ_Bitmap_t*     DstBitmap_p,
                                              STGXOBJ_Rectangle_t*  DstRectangle_p,
                                              STBLIT_BlitContext_t* BlitContext_p,
                                              stblit_NodeHandle_t*  NodeHandle_p,
                                              stblit_OperationConfiguration_t*  Config_p,
                                              stblit_CommonField_t*             CommonField_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    U8                      ScanConfig  = 0;  /* Vertical and Horizontal scan configuration. Down Right per default */
    U8                      OverlapCode;              /* Resulting overlap direction code both src */
    U8                      AlphaOverlapCode            = 0xF;  /* Overlap direction code Alpha bitmap */
    U8                      ColorOverlapCode            = 0xF;  /* Overlap direction code Color Bitmap */
    U32                     DstXStop;
    U32                     DstYStop;
    U32                     SrcXStop;
    U32                     SrcYStop;
    STGXOBJ_Palette_t       SrcAlphaPalette;

    /* Analyse overlap Src/Dst Only when Dst = Src */
    if (SrcAlphaBitmap_p == DstBitmap_p)
    {
        DstXStop    = (U32)(DstRectangle_p->PositionX) + (U32)(DstRectangle_p->Width) - 1 ;
        DstYStop    = (U32)(DstRectangle_p->PositionY) + (U32)(DstRectangle_p->Height) - 1 ;
        SrcXStop    = SrcAlphaRectangle_p->PositionX + (U32)(SrcAlphaRectangle_p->Width) - 1 ;
        SrcYStop    = SrcAlphaRectangle_p->PositionY + (U32)(SrcAlphaRectangle_p->Height) - 1 ;
        AlphaOverlapCode = OverlapOneSourceCoordinate(SrcAlphaRectangle_p->PositionX,SrcAlphaRectangle_p->PositionY,SrcXStop,SrcYStop,
                                                 DstRectangle_p->PositionX,DstRectangle_p->PositionY,DstXStop,DstYStop);


    }
    if (SrcColorBitmap_p == DstBitmap_p)
    {
        DstXStop    = (U32)(DstRectangle_p->PositionX) + (U32)(DstRectangle_p->Width) - 1 ;
        DstYStop    = (U32)(DstRectangle_p->PositionY) + (U32)(DstRectangle_p->Height) - 1 ;
        SrcXStop    = SrcColorRectangle_p->PositionX + (U32)(SrcColorRectangle_p->Width) - 1 ;
        SrcYStop    = SrcColorRectangle_p->PositionY + (U32)(SrcColorRectangle_p->Height) - 1 ;
        ColorOverlapCode = OverlapOneSourceCoordinate(SrcColorRectangle_p->PositionX,SrcColorRectangle_p->PositionY,SrcXStop,SrcYStop,
                                                 DstRectangle_p->PositionX,DstRectangle_p->PositionY,DstXStop,DstYStop);


    }

    OverlapCode = (ColorOverlapCode & AlphaOverlapCode) & 0xF;
    if (OverlapCode != 0)
    {
        ScanConfig = ScanDirectionTable[OverlapCode];
    }
    else /* Code = 0 => Overlap Not Supported */
    {
        return(STBLIT_ERROR_OVERLAP_NOT_SUPPORTED);
    }



    /* Get Node descriptor handle*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
        }
    }
    else  /* Blit in Job */
    {
        STOS_SemaphoreWait(Device_p->AccessControl);
        Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) NodeHandle_p);
        STOS_SemaphoreSignal(Device_p->AccessControl);

        if (Err != ST_NO_ERROR)
        {
            return(STBLIT_ERROR_MAX_JOB_NODE);
        }
    }

    Node_p  = (stblit_Node_t*)(*NodeHandle_p);

    /* Initialize Node */
    InitNode(Node_p, BlitContext_p);

    /* Initialize interruption stuff*/
    SetNodeIT (Node_p,BlitContext_p,CommonField_p);

    /* Set Src1  */
    SetNodeSrc1Bitmap(Node_p,SrcColorBitmap_p,SrcColorRectangle_p,ScanConfig,Device_p,STBLIT_SOURCE1_MODE_NORMAL,CommonField_p);

    /* Set Src2  */
    SetNodeSrc2Bitmap(Node_p,SrcAlphaBitmap_p,SrcAlphaRectangle_p,ScanConfig,Device_p,CommonField_p);

    /* Set Color expansion if SrcAlphaBitmap is ALPHA4 format. Use Alpha4 To Alpha8 conversion palette */
    if (SrcAlphaBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA4)
    {
        /* Set SrcAlphaPalette */
        SrcAlphaPalette.ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
        SrcAlphaPalette.Data_p      = (void*)Device_p->Alpha4To8Palette_p;
/*        SrcAlphaPalette.PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;*/
/*        SrcAlphaPalette.ColorDepth  = ;*/

        /* No need to put WA_GNBvd13604 because SRC2 is never ACLUT44 */
        SetNodeColorExpansion(&SrcAlphaPalette,Device_p, CommonField_p);
    }

    /* Set ALU mode Mask Blen first pass wiht no global alpha */
    CommonField_p->ACK |= ((U32)(128 & STBLIT_ACK_ALU_GLOBAL_ALPHA_MASK) <<  STBLIT_ACK_ALU_GLOBAL_ALPHA_SHIFT);
    CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_MASK_BLEND & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);

    /* Set Target */
    SetNodeTgt(Node_p,DstBitmap_p,DstRectangle_p, ScanConfig,Device_p);

    /* Set Trigger */
    if (BlitContext_p->Trigger.EnableTrigger == TRUE)
    {
        SetNodeTrigger(Node_p, BlitContext_p,CommonField_p);
    }

    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->ConcatMode             = TRUE;
        /* Other fields to be written????? */
    }


    return(Err);
}





/*******************************************************************************
Name        : stblit_UpdateDestinationBitmap
Description : Update Destination Bitmap in node for a given job Blit
Parameters  :
Assumptions : All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateDestinationBitmap(stblit_Device_t*       Device_p,
                                              stblit_JobBlit_t*      JBlit_p,
                                              STGXOBJ_Bitmap_t*      Bitmap_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;
    U32                 i;


    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* In case of single node(Normal or Single pass mask), Last and first node is the same .
         * In case multi nodes (2 passes mask), the node to update is the last one for destination.
         * => Fetch/update always last node! */
        UpdateNodeTgtBitmap((stblit_Node_t*)(JBlit_p->LastNodeHandle),Bitmap_p,Device_p);

        /* Update Src1 if Dst = Src */
        if (JBlit_p->Cfg.IdenticalDstSrc1 == TRUE)
        {
            UpdateNodeSrc1Bitmap((stblit_Node_t*)(JBlit_p->LastNodeHandle),Bitmap_p,Device_p);
        }
    }
    else if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_EMULATED)
    {
        /* Assumption is No Mask
         * Several Nodes to update*/

        NodeHandle = JBlit_p->LastNodeHandle;

        /* Update all nodes */
        for (i=0; i < JBlit_p->NbNodes ;i++ )
        {
            Node_p = (stblit_Node_t*)NodeHandle;

            /* Update Target */
            UpdateNodeTgtBitmap(Node_p,Bitmap_p,Device_p);

            /* Update Src1  if Dst = Src1 */
            if (JBlit_p->Cfg.IdenticalDstSrc1 == TRUE)
            {
                UpdateNodeSrc1Bitmap(Node_p,Bitmap_p,Device_p);
            }

           NodeHandle =  Node_p->PreviousNode;
        }
    }
    else /* STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Err);
}

/*******************************************************************************
Name        :  stblit_UpdateBackgroundBitmap
Description :  Update background bitmap in node for a given job Blit
Parameters  :
Assumptions :  All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateBackgroundBitmap(stblit_Device_t*      Device_p,
                                              stblit_JobBlit_t*     JBlit_p,
                                              STGXOBJ_Bitmap_t*     Bitmap_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;
    U32                 i;

    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* Update node :
         * In case of single node (Normal or Single pass mask), Last and first node are the same.
         * In case of multi nodes (2 passes mask ), the node to update is the last one for background.
         * => Fetch/update always last node! */

        UpdateNodeSrc1Bitmap((stblit_Node_t*)(JBlit_p->LastNodeHandle),Bitmap_p,Device_p);

        /* Update Dst if Dst = Src1 */
        if (JBlit_p->Cfg.IdenticalDstSrc1 == TRUE)
        {
            UpdateNodeTgtBitmap((stblit_Node_t*)(JBlit_p->LastNodeHandle),Bitmap_p,Device_p);
        }

    }
    else if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_EMULATED)
    {
        /* Assumption is No Mask
         * Several Nodes to update */

        NodeHandle = JBlit_p->LastNodeHandle;

        /* Update all nodes */
        for (i=0; i < JBlit_p->NbNodes ;i++ )
        {
            Node_p = (stblit_Node_t*)NodeHandle;

            /* Update background */
            UpdateNodeSrc1Bitmap(Node_p,Bitmap_p,Device_p);

            /* Update Dst  if Dst = Src1 */
            if (JBlit_p->Cfg.IdenticalDstSrc1 == TRUE)
            {
                UpdateNodeTgtBitmap(Node_p,Bitmap_p,Device_p);
            }

           NodeHandle =  Node_p->PreviousNode;
        }
    }
    else  /*  STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Err);
}

/*******************************************************************************
Name        :  stblit_UpdateForegroundBitmap
Description :  Update foreground bitmap in node for a given job Blit
Parameters  :
Assumptions : All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateForegroundBitmap(stblit_Device_t*       Device_p,
                                              stblit_JobBlit_t*      JBlit_p,
                                              STGXOBJ_Bitmap_t*      Bitmap_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;


    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* Update node :
         * In case of single node (Normal Mode).(Note that single pass Mask is only in case of foreground color), Last and first node
         * are the same.
         * In case of multi nodes (2 passes mask), the node to update is the first one for foreground.
         * => Fetch/update always first node! */

        if ((JBlit_p->Cfg.OpMode == STBLIT_SOURCE1_MODE_DIRECT_COPY) || (JBlit_p->Cfg.MaskEnabled == TRUE))
        {
            /* Always raster in direct copy mode.
             * Always raster in normal mode when mask */
            UpdateNodeSrc1Bitmap((stblit_Node_t*)(JBlit_p->FirstNodeHandle),Bitmap_p,Device_p);
        }
        else if (JBlit_p->Cfg.OpMode ==  STBLIT_SOURCE1_MODE_NORMAL)   /* And there is no mask */
        {
            /* Raster or MB */
            UpdateNodeSrc2Bitmap((stblit_Node_t*)(JBlit_p->FirstNodeHandle),Bitmap_p,Device_p);
            if (JBlit_p->Cfg.SrcMB == TRUE)
            {
                UpdateNodeSrc1Bitmap((stblit_Node_t*)(JBlit_p->FirstNodeHandle),Bitmap_p,Device_p);
            }
        }
        else
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
    }
    else /* STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Err);
}

/*******************************************************************************
Name        :  stblit_UpdateForegroundColor
Description :  Update foreground color in node for a given job Blit
Parameters  :
Assumptions :  All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateForegroundColor(stblit_JobBlit_t*     JBlit_p,
                                            STGXOBJ_Color_t*      Color_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;
    U32                 i;

    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
         /* In case of single pass mask with color, the Mask palette have to be updated */
        if ((JBlit_p->Cfg.MaskEnabled == TRUE) && (JBlit_p->NbNodes == 1))
        {
            /*  UpdateMaskPalette() */ ;
        }
        else
        {
            /* 2 cases :
            *      + Single pass No Mask
            *      + Two pass Mask
            * In case of multi passes, the node to update is the first one for foreground.
            * => Fetch/update always first node! */
            if ((JBlit_p->Cfg.OpMode == STBLIT_SOURCE1_MODE_DIRECT_FILL) || (JBlit_p->Cfg.MaskEnabled == TRUE))
            {
                UpdateNodeSrc1Color((stblit_Node_t*)(JBlit_p->FirstNodeHandle),Color_p);
            }
            else if  (JBlit_p->Cfg.OpMode == STBLIT_SOURCE1_MODE_NORMAL)  /* And there is no Mask */
            {
                UpdateNodeSrc2Color((stblit_Node_t*)(JBlit_p->FirstNodeHandle),Color_p);
            }
            else
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
        }
    }
    else if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_EMULATED)
    {
      /* Several Nodes to update.
       * Same color in every nodes => if previous config was XYLC or XYC (variable color),it becomes  XYL or XY (constant color).
       * No Mask support */
        NodeHandle = JBlit_p->LastNodeHandle;

        /* Update all nodes */
        for (i=0; i < JBlit_p->NbNodes ;i++ )
        {
            Node_p = (stblit_Node_t*)NodeHandle;

            /* Update Src2 */
            UpdateNodeSrc2Color(Node_p,Color_p);

            NodeHandle =  Node_p->PreviousNode;
        }
    }
    else /* STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Err);
}

/*******************************************************************************
Name        : stblit_UpdateMaskBitmap
Description : Update Mask bitmap in node for a given job Blit
Parameters  :
Assumptions : All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateMaskBitmap(stblit_Device_t*      Device_p,
                                       stblit_JobBlit_t*     JBlit_p,
                                       STGXOBJ_Bitmap_t*     Bitmap_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* Whatever the mode of mask (One pass or Two passes), update the Src2 pipe */
        UpdateNodeSrc2Bitmap((stblit_Node_t*)(JBlit_p->FirstNodeHandle),Bitmap_p,Device_p);
    }
    else  /* First implementation , Mask only supported when no XYL */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
}

/*******************************************************************************
Name        : stblit_UpdateDestinationRectangle
Description : Update Destination rectangle in node for a given job Blit
Parameters  :
Assumptions : All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
              No Overlap check.
              Rectangle size does not changed!
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateDestinationRectangle(stblit_JobBlit_t*     JBlit_p,
                                                 STGXOBJ_Rectangle_t*  Rectangle_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;
    U32                 i;

    /* No Check of Overlap and ScanConfig */

    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* In case of single node(Normal or Single pass mask), Last and first node is the same .
         * In case multi nodes (2 passes mask), the node to update is the last one for destination.
         * => Fetch/update always last node! */
        UpdateNodeTgtRectangle((stblit_Node_t*)(JBlit_p->LastNodeHandle),Rectangle_p,JBlit_p->Cfg.ScanConfig);

        /* Update Src1 if Dst = Src */
        if (JBlit_p->Cfg.IdenticalDstSrc1 == TRUE)
        {
            UpdateNodeSrc1Rectangle((stblit_Node_t*)(JBlit_p->LastNodeHandle),Rectangle_p,JBlit_p->Cfg.ScanConfig, FALSE);
        }
    }
    else if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_EMULATED)
    {
        /* Assumption is No Mask
         * Several Nodes to update*/

        NodeHandle = JBlit_p->LastNodeHandle;

        /* Update all nodes */
        for (i=0; i < JBlit_p->NbNodes ;i++ )
        {
            Node_p = (stblit_Node_t*)NodeHandle;

            /* Update Target */
            UpdateNodeTgtRectangle(Node_p,Rectangle_p,JBlit_p->Cfg.ScanConfig);

            /* Update Src1  if Dst = Src1 */
            if (JBlit_p->Cfg.IdenticalDstSrc1 == TRUE)
            {
                UpdateNodeSrc1Rectangle(Node_p,Rectangle_p,JBlit_p->Cfg.ScanConfig, FALSE);
            }

           NodeHandle =  Node_p->PreviousNode;
        }
    }
    else /* STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Err);
}


/*******************************************************************************
Name        :  stblit_UpdateBackgroundRectangle
Description :  Update background rectangle in node for a given job Blit
Parameters  :
Assumptions :  All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
               No Overlap check.
               Rectangle size does not changed!
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateBackgroundRectangle(stblit_JobBlit_t*     JBlit_p,
                                                STGXOBJ_Rectangle_t*  Rectangle_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;
    U32                 i;


    /* No Check of Overlap and ScanConfig */

    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* Update node :
         * In case of single node (Normal or Single pass mask), Last and first node are the same.
         * In case of multi nodes (2 passes mask ), the node to update is the last one for background.
         * => Fetch/update always last node! */

        UpdateNodeSrc1Rectangle((stblit_Node_t*)(JBlit_p->LastNodeHandle),Rectangle_p,JBlit_p->Cfg.ScanConfig, FALSE);

        /* Update Dst if Dst = Src1 */
        if (JBlit_p->Cfg.IdenticalDstSrc1 == TRUE)
        {
            UpdateNodeTgtRectangle((stblit_Node_t*)(JBlit_p->LastNodeHandle),Rectangle_p,JBlit_p->Cfg.ScanConfig);
        }

    }
    else if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_EMULATED)
    {
        /* Assumption is No Mask
         * Several Nodes to update */

        NodeHandle = JBlit_p->LastNodeHandle;

        /* Update all nodes */
        for (i=0; i < JBlit_p->NbNodes ;i++ )
        {
            Node_p = (stblit_Node_t*)NodeHandle;

            /* Update background */
            UpdateNodeSrc1Rectangle(Node_p,Rectangle_p,JBlit_p->Cfg.ScanConfig, FALSE);

            /* Update Dst  if Dst = Src1 */
            if (JBlit_p->Cfg.IdenticalDstSrc1 == TRUE)
            {
                UpdateNodeTgtRectangle(Node_p,Rectangle_p,JBlit_p->Cfg.ScanConfig);
            }

           NodeHandle =  Node_p->PreviousNode;
        }
    }
    else  /*  STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Err);
}


/*******************************************************************************
Name        : stblit_UpdateForegroundRectangle
Description : Update Foreground rectangle positions in node for a given job Blit.
              Size is not updated (resize not recalculated)
Parameters  :
Assumptions : All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
              No Overlap check.
              Rectangle size does not changed!
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateForegroundRectangle(stblit_JobBlit_t*     JBlit_p,
                                                STGXOBJ_Rectangle_t*  Rectangle_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* No Check of Overlap and ScanConfig */

    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* Update node :
         * In case of single node (Normal Mode).(Note that single pass Mask is only in case of foreground color), Last and first node
         * are the same.
         * In case of multi nodes (2 passes mask), the node to update is the first one for foreground.
         * => Fetch/update always first node! */
        if ((JBlit_p->Cfg.OpMode == STBLIT_SOURCE1_MODE_DIRECT_COPY) ||
            (JBlit_p->Cfg.OpMode == STBLIT_SOURCE1_MODE_DIRECT_FILL ) ||
            (JBlit_p->Cfg.MaskEnabled == TRUE))
        {
            /* Always raster  */
            UpdateNodeSrc1Rectangle((stblit_Node_t*)(JBlit_p->FirstNodeHandle),Rectangle_p,JBlit_p->Cfg.ScanConfig, FALSE);
        }
        else /*  JBlit_p->Cfg.OpMode ==  STBLIT_SOURCE1_MODE_NORMAL */
        {
            /* Raster or MB */
            UpdateNodeSrc2Rectangle((stblit_Node_t*)(JBlit_p->FirstNodeHandle),Rectangle_p,JBlit_p->Cfg.ScanConfig);
            if (JBlit_p->Cfg.SrcMB == TRUE)
            {
                UpdateNodeSrc1Rectangle((stblit_Node_t*)(JBlit_p->FirstNodeHandle),Rectangle_p,JBlit_p->Cfg.ScanConfig, TRUE);
            }
        }

    }
    else /* STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Err);
}

/*******************************************************************************
Name        : stblit_UpdateClipRectangle
Description : Update Clip rectangle in node for a given job Blit.
Parameters  :
Assumptions : All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateClipRectangle(stblit_JobBlit_t*     JBlit_p,
                                          BOOL                  WriteInsideClipRectangle,
                                          STGXOBJ_Rectangle_t*  ClipRectangle_p)
{
    ST_ErrorCode_t          Err = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    stblit_NodeHandle_t     NodeHandle;
    U32                     i;


    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* In case of single node (Mode Normal or Single pass mask), Last and first node is the same .
         * In case multi nodes (2 passes mask), the node to update is the last one for destination.
         * => Fetch/update always last node! */
        UpdateNodeClipping((stblit_Node_t*)(JBlit_p->LastNodeHandle),ClipRectangle_p,WriteInsideClipRectangle);

    }
    else if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_EMULATED)
    {
        /* Assumption is No Mask
         * Several Nodes to update*/

        NodeHandle = JBlit_p->LastNodeHandle;

        /* Update all nodes */
        for (i=0; i < JBlit_p->NbNodes ;i++ )
        {
            Node_p = (stblit_Node_t*)NodeHandle;

            /* Update Clip rectangle */
            UpdateNodeClipping(Node_p,ClipRectangle_p,WriteInsideClipRectangle);


           NodeHandle =  Node_p->PreviousNode;
        }
    }
    else /* STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
}

/*******************************************************************************
Name        : stblit_UpdateMaskWord
Description : Update Mask word in node for a given job Blit.
Parameters  :
Assumptions : All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/

 ST_ErrorCode_t stblit_UpdateMaskWord(stblit_JobBlit_t*      JBlit_p,
                                     U32                    MaskWord)
 {
    ST_ErrorCode_t          Err = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    stblit_NodeHandle_t     NodeHandle;
    U32                     i;

    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* In case of single node (Mode Normal or Single pass mask), Last and first node is the same .
         * In case multi nodes (2 passes mask), the node to update is the last one for destination.
         * => Fetch/update always last node! */
        UpdateNodePlaneMask((stblit_Node_t*)(JBlit_p->LastNodeHandle),MaskWord);
    }
    else if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_EMULATED)
    {
        /* Assumption is No Mask
         * Several Nodes to update*/

        NodeHandle = JBlit_p->LastNodeHandle;

        /* Update all nodes */
        for (i=0; i < JBlit_p->NbNodes ;i++ )
        {
            Node_p = (stblit_Node_t*)NodeHandle;

            /* Update mask word */
            UpdateNodePlaneMask(Node_p,MaskWord);

           NodeHandle =  Node_p->PreviousNode;
        }
    }
    else /* STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
 }

/*******************************************************************************
Name        : stblit_UpdateColorKey
Description : Update Color Key in node for a given job Blit.
Parameters  :
Assumptions : All parameters are checked at upper level :
               => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/

 ST_ErrorCode_t stblit_UpdateColorKey(stblit_JobBlit_t*         JBlit_p,
                                     STBLIT_ColorKeyCopyMode_t ColorKeyCopyMode,
                                     STGXOBJ_ColorKey_t*       ColorKey_p)
 {
    ST_ErrorCode_t          Err = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    stblit_NodeHandle_t     NodeHandle;
    U32                     i;

    if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_NONE)
    {
        /* In case of single node (Mode Normal or Single pass mask), Last and first node is the same .
         * In case multi nodes (2 passes mask), the node to update is the last one for destination.
         * => Fetch/update always last node! */
        UpdateNodeColorKey ((stblit_Node_t*)(JBlit_p->LastNodeHandle),ColorKeyCopyMode,ColorKey_p);
    }
    else if (JBlit_p->Cfg.XYLMode == STBLIT_JOB_BLIT_XYL_TYPE_EMULATED)
    {
        /* Assumption is No Mask
         * Several Nodes to update*/

        NodeHandle = JBlit_p->LastNodeHandle;

        /* Update all nodes */
        for (i=0; i < JBlit_p->NbNodes ;i++ )
        {
            Node_p = (stblit_Node_t*)NodeHandle;

            /* Update Color key */
            UpdateNodeColorKey (Node_p,ColorKeyCopyMode,ColorKey_p);

           NodeHandle =  Node_p->PreviousNode;
        }
    }
    else /* STBLIT_JOB_BLIT_XYL_TYPE_DEVICE */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
 }


/* End of blt_mem.c */
