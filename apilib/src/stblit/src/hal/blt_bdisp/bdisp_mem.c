/*******************************************************************************

File name    : bdisp_mem.c

Description :  write nodes in shared memory

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
01 Jun 2004        Created                                          HE
30 May 2006        Modified for WinCE platform					  WinCE Team-ST Noida
27 sep 2006        - added function UpdateNodeGlobalAlpha()
				   - added storing of BlendOp & PreMultipliedColor WinCE Team-ST Noida
                   - fixed OverlapOneSourceCoordinate()
				   - added SetNodeHandleSrc2WindowSize
				   - added SetNodeHandleSrc1DestWindowSize
				   - added implementation of stblit_UpdateFilters()
				   - added implementation of stblit_UpdateGlobalAlpha()
				   - All changes are under ST_OSWINCE option.
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stblit.h"
#include "bdisp_com.h"
#include "bdisp_mem.h"
#include "bdisp_fe.h"
#include "bdisp_flt.h"
#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#include "stavmem.h"
#endif
#include "stgxobj.h"
#include "stos.h"
#include "bdisp_pool.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif

#ifndef ST_ZEUS
#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include "stlayer.h"
#endif
#endif

#if defined(DVD_SECURED_CHIP)
#include "stmes.h"
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

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
__inline static ST_ErrorCode_t SelectHFilter (U32 Increment,U32* Offset_p);
__inline static ST_ErrorCode_t SelectVFilter (U32 Increment,U32* Offset_p);
__inline static ST_ErrorCode_t GetFilterMode (STGXOBJ_ColorType_t ColorType, U8 ClutEnable, U8 ClutMode, U8* FilterMode);
__inline static U8 OverlapOneSourceCoordinate(U32 SrcXStart, U32 SrcYStart,U32 SrcXStop,U32 SrcYStop,
                                              U32 DstXStart, U32 DstYStart,U32 DstXStop,U32 DstYStop);
__inline static ST_ErrorCode_t FormatColorType (STGXOBJ_ColorType_t TypeIn, U32* TypeOut_p,U8* AlphaRange_p,STGXOBJ_BitmapType_t Mode);
__inline static ST_ErrorCode_t InitNode(stblit_Node_t* Node_p, STBLIT_BlitContext_t*  BlitContext_p);
/*__inline static ST_ErrorCode_t SetNodeIT(stblit_Node_t* Node_p, stblit_Device_t* Device_p, STBLIT_BlitContext_t* BlitContext_p,stblit_CommonField_t* CommonField_p,BOOL LastNode);*/
__inline static ST_ErrorCode_t SetNodeSrc1Color(stblit_Node_t* Node_p,STGXOBJ_Color_t* Color_p,stblit_Device_t* Device_p,stblit_Source1Mode_t Mode,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc1Bitmap(stblit_Node_t* Node_p,STGXOBJ_Bitmap_t* Bitmap_p,STGXOBJ_Rectangle_t* Rectangle_p,U8 ScanConfig,stblit_Device_t* Device_p,stblit_Source1Mode_t Mode,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc1(stblit_Node_t* Node_p, STBLIT_Source_t* Src_p, U8 ScanConfig,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc2Color(stblit_Node_t* Node_p,STGXOBJ_Color_t* Color_p,STGXOBJ_Rectangle_t* DstRectangle_p,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeSrc2Bitmap(stblit_Node_t* Node_p,STGXOBJ_Bitmap_t* Bitmap_p,STGXOBJ_Rectangle_t* Rectangle_p,
                                                 U8 ScanConfig,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p,
                                                 U32 ChromaWidth,S32 ChromaPositionX,U32 ChromaHeight,S32 ChromaPositionY);
__inline static ST_ErrorCode_t SetNodeSrc2(stblit_Node_t* Node_p, STBLIT_Source_t* Src_p, STGXOBJ_Rectangle_t*  DstRectangle_p,
                                           U8 ScanConfig,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p,
                                           U32 ChromaWidth,S32 ChromaPositionX,U32 ChromaHeight,S32 ChromaPositionY);
__inline static ST_ErrorCode_t SetNodeTgt (stblit_Node_t* Node_p, STGXOBJ_Bitmap_t* Bitmap_p,STGXOBJ_Rectangle_t* Rectangle_p,U8 ScanConfig,stblit_Device_t* Device_p);
__inline static ST_ErrorCode_t SetNodeClipping (stblit_Node_t* Node_p, STGXOBJ_Rectangle_t* ClipRectangle_p,BOOL WriteInsideClipRectangle,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t UpdateNodeClipping (stblit_Node_t* Node_p, STGXOBJ_Rectangle_t* ClipRectangle_p,BOOL WriteInsideClipRectangle);
__inline static ST_ErrorCode_t SetNodeResize (  stblit_Node_t*          Node_p,
                                                U32                     SrcWidth,
                                                U32                     SrcHeight,
                                                U32                     DstWidth,
                                                U32                     DstHeight,
                                                stblit_Device_t*        Device_p,
                                                U8                      FilterMode,
                                                stblit_CommonField_t*   CommonField_p,
                                                U32                     HIncrement,
                                                U32                     HNbRepeat,
                                                U32                     HSRC_InitPhase,
                                                U32                     VIncrement,
                                                U32                     VNbRepeat,
                                                U32                     VSRC_InitPhase,
                                                BOOL                    SrcMb,
                                                U32                     Chroma_HSRC_InitPhase,
                                                U32                     ChromaHNbRepeat,
                                                U32                     ChromaHScalingFactor,
                                                U32                     Chroma_VSRC_InitPhase,
                                                U32                     ChromaVNbRepeat,
                                                U32                     ChromaVScalingFactor);
__inline static ST_ErrorCode_t SetNodeALUBlend (U8 GlobalAlpha,BOOL PreMultipliedColor,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeInputConverter (U8 InEnable,U8 InConfig,stblit_CommonField_t* CommonField_p, stblit_Node_t* Node_p);
__inline static ST_ErrorCode_t SetNodeOutputConverter (U8 OutEnable,U8 OutConfig, U8 InConfig, stblit_CommonField_t* CommonField_p, stblit_Node_t* Node_p);
__inline static ST_ErrorCode_t SetNodeColorCorrection (STBLIT_BlitContext_t* BlitContext_p,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t SetNodeColorExpansion  (STGXOBJ_Palette_t*  Palette_p,stblit_Device_t* Device_p,stblit_CommonField_t* CommonField_p);

#if defined (ST_7109) || defined (ST_7200) || defined (STBLIT_USE_COLOR_KEY_FOR_BDISP)
__inline static ST_ErrorCode_t SetNodeColorKey        (stblit_Node_t* Node_p,STBLIT_ColorKeyCopyMode_t  ColorKeyCopyMode,STGXOBJ_ColorKey_t* ColorKey_p,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t UpdateNodeColorKey     (stblit_Node_t* Node_p,STBLIT_ColorKeyCopyMode_t  ColorKeyCopyMode,STGXOBJ_ColorKey_t* ColorKey_p);
#endif /* (ST_7109) || defined (ST_7200) || (STBLIT_USE_COLOR_KEY_FOR_BDISP) */
#if defined(ST_7109) || defined(ST_7200)
__inline static ST_ErrorCode_t SetNodePlaneMask (stblit_Node_t* Node_p,U32 MaskWord,stblit_CommonField_t* CommonField_p);
__inline static ST_ErrorCode_t UpdateNodePlaneMask (stblit_Node_t* Node_p,U32 MaskWord);
#ifdef ST_OSWINCE
__inline static ST_ErrorCode_t UpdateNodeGlobalAlpha (stblit_Node_t* Node_p, U32 Value, U32 Mask);
#endif
__inline static ST_ErrorCode_t SetNodeALULogic (STBLIT_BlitContext_t* BlitContext_p,stblit_CommonField_t* CommonField_p);
__inline static U8 GetMatrixIndex(U8 EnableColorColorimetry, U8 EnableChromaFormat, U8 EnableColorDir);
__inline static ST_ErrorCode_t SetNodeRangeMode (stblit_Node_t* Node_p, STGXOBJ_Bitmap_t*  Bitmap_p, stblit_CommonField_t* CommonField_p);


#endif /* defined(ST_7109) || defined(ST_7200) */

__inline static ST_ErrorCode_t SetOperationIOConfiguration  (STBLIT_Source_t*      Src1_p,
                                                               STBLIT_Source_t*      Src2_p,
                                                               STBLIT_Destination_t* Dst_p,
                                                               stblit_OperationConfiguration_t* Config_p);
 __inline static ST_ErrorCode_t SetNodeFlicker (stblit_Node_t*          Node_p,
                                                stblit_CommonField_t*   CommonField_p,
                                                S32                     DstPositionY,
												U32                     DstBitmapHeight,
												S32*                    SrcPositionY_p,
												U32*                    SrcHeight_p);
/* Functions ---------------------------------------------------------------- */
#ifndef ST_ZEUS
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
#endif /* ST_ZEUS */


#if defined(ST_7109) || defined(ST_7200)

/*******************************************************************************
Name        : GetUpsizedValue
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static S32 GetUpsizedValue(S32 Coeff)
{
    S32 UpsizedValue, Sign;

    if ( ( Coeff > -256 ) && ( Coeff < 255 ) )
    {
        UpsizedValue = Coeff;
    }
    else
    {
        if ( Coeff > 0 )
        {
            Sign = 1;
        }
        else
        {
            Sign = -1;
        }

        UpsizedValue = ( Coeff + 512*Sign ) / 3;
    }

    return UpsizedValue;
}

/*******************************************************************************
Name        : GetMatrixIndex
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U8 GetMatrixIndex(U8 EnableColorColorimetry, U8 EnableChromaFormat, U8 EnableColorDir)
{
    U8 MatrixIndex;

    if ( EnableColorDir == 0 ) /* YcbCr to RGB */
    {
        if ( EnableColorColorimetry == 0 ) /* 601 */
        {
            if ( EnableChromaFormat == 0 )/* Unsigned */
                MatrixIndex = YCBCR_TO_RGB_601_UNSIGNED;
            else /* Signed */
                MatrixIndex = YCBCR_TO_RGB_601_SIGNED;
        }
        else /* 709 */
        {
            if ( EnableChromaFormat == 0 )/* Unsigned */
                MatrixIndex = YCBCR_TO_RGB_709_UNSIGNED;
            else /* Signed */
                MatrixIndex = YCBCR_TO_RGB_709_SIGNED;
        }
    }
    else  /* RGB to YcbCr */
    {
        if ( EnableColorColorimetry == 0 ) /* 601 */
        {
            if ( EnableChromaFormat == 0 )/* Unsigned */
                MatrixIndex = RGB_TO_YCBCR_601_UNSIGNED;
            else /* Signed */
                MatrixIndex = RGB_TO_YCBCR_601_SIGNED;
        }
        else /* 709 */
        {
            if ( EnableChromaFormat == 0 )/* Unsigned */
                MatrixIndex = RGB_TO_YCBCR_709_UNSIGNED;
            else /* Signed */
                MatrixIndex = RGB_TO_YCBCR_709_SIGNED;
        }
    }

    return MatrixIndex;
}
#endif /* defined(ST_7109) || defined(ST_7200) */


/*******************************************************************************
Name        : SetNodeIT
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialize
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeIT(stblit_Node_t*            Node_p,
                                          stblit_Device_t*          Device_p,
                                          STBLIT_BlitContext_t*     BlitContext_p,
                                          stblit_CommonField_t*     CommonField_p,
                                          BOOL                      LastNode)
 {
    ST_ErrorCode_t   Err = ST_NO_ERROR;

    UNUSED_PARAMETER(Device_p);

    if ( LastNode )
    {
        /* Enable notif flag */
        CommonField_p->INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);

        /* Initialize User tag */
        Node_p->UserTag_p = BlitContext_p->UserTag_p;

#ifdef STBLIT_DEBUG
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SetNodeIT(): Node_p->UserTag_p= 0x%x \n",  Node_p->UserTag_p ));
#endif
        /* Overwrite Support */
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_USR)), ((U32)((U32)(STBLIT_USR_VALUE) & STBLIT_USR_MASK) << STBLIT_USR_SHIFT));

        /* Update ITOpcode*/
        if ( BlitContext_p->NotifyBlitCompletion )
        {
            Node_p->ITOpcode |= STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
        }
        else
        {
            Node_p->ITOpcode &= (~STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK);
        }
    }
    else
    {
        /* Disable notif flag */
        CommonField_p->INS |= ((U32)(STBLIT_INS_DESABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
    }


/*    if ( (Device_p->SBlitNodeDataPool.NbElem - Device_p->SBlitNodeDataPool.NbFreeElem) % NB_NODES_TO_RELEASE == 0 )*/
/*    {*/
/*        CommonField_p->INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);*/
/*    }*/

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
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 16;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
            *AlphaRange_p   = 0;
            ((Mode == STGXOBJ_BITMAP_TYPE_MB) || (Mode == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP)) ?  (*TypeOut_p =  19) :  (*TypeOut_p =  18);
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 20;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
            *AlphaRange_p   = 0;
            ((Mode == STGXOBJ_BITMAP_TYPE_MB) || (Mode == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP)) ?  (*TypeOut_p =  19) :  (*TypeOut_p =  18);
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 20;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
        case STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888:
            *AlphaRange_p   = 0;
            *TypeOut_p      = 21;
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
        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
            *ValueOut_p =  (U32)(((ValueIn.UnsignedAYCbCr8888.Alpha & 0xFF) << 24) |
                                 ((ValueIn.UnsignedAYCbCr8888.Cr    & 0xFF) << 16) |
                                 ((ValueIn.UnsignedAYCbCr8888.Y     & 0xFF) << 8) |
                                  (ValueIn.UnsignedAYCbCr8888.Cb    & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888:
            *ValueOut_p =  (U32)(((ValueIn.SignedAYCbCr8888.Alpha & 0xFF) << 24) |
                                 ((ValueIn.SignedAYCbCr8888.Cr    & 0xFF) << 16) |
                                 ((ValueIn.SignedAYCbCr8888.Y     & 0xFF) << 8) |
                                  (ValueIn.SignedAYCbCr8888.Cb    & 0xFF));
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
        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
            *ValueOut_p =  (U32)(((ValueIn.UnsignedAYCbCr8888.Alpha & 0xFF) << 24) |
                                 ((ValueIn.UnsignedAYCbCr8888.Cr    & 0xFF) << 16) |
                                 ((ValueIn.UnsignedAYCbCr8888.Y     & 0xFF) << 8) |
                                  (ValueIn.UnsignedAYCbCr8888.Cb    & 0xFF));
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888:
            *ValueOut_p =  (U32)(((ValueIn.SignedAYCbCr8888.Alpha & 0xFF) << 24) |
                                 ((ValueIn.SignedAYCbCr8888.Cr    & 0xFF) << 16) |
                                 ((ValueIn.SignedAYCbCr8888.Y     & 0xFF) << 8) |
                                  (ValueIn.SignedAYCbCr8888.Cb    & 0xFF));
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
void stblit_WriteCommonFieldInNode(stblit_Device_t*        Device_p,
                                   stblit_Node_t*          Node_p,
                                   stblit_CommonField_t*   CommonField_p)
 {
#if defined (DVD_SECURED_CHIP)
    U8 MemoryStatus;
#endif

    UNUSED_PARAMETER(Device_p);

    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)), CommonField_p->INS);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_ACK)), CommonField_p->ACK);
    /*-----------------6/2/2004 12:02PM-----------------
     * The register become FCLT in 5100
     * --------------------------------------------------*/
    /*STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RZC)), CommonField_p->RZC);*/
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_F_RZC_CTL)), CommonField_p->F_RZC_CTL);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RZI)), CommonField_p->RZI);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CCO)), CommonField_p->CCO);

#if defined (DVD_SECURED_CHIP)
    {
        MemoryStatus = STMES_IsMemorySecure((void *)CommonField_p->CML, 0, 0);
        if(MemoryStatus == SECURE_REGION)
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CML)),(STBLIT_SECURE_ON | CommonField_p->CML));
        }
        else
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CML)), CommonField_p->CML);
        }
    }
#else
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CML)), CommonField_p->CML);
    }
#endif
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
#if defined (DVD_SECURED_CHIP)
    U8 MemoryStatus;
#endif

    /* Upate Next Node  of the first node*/
#ifndef PTV

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if ( PhysicJobBlitNodeBuffer_p )
    {
        Tmp = (U32) SecondNode_p - (U32) Device_p->JobBlitNodeBuffer_p + (U32) PhysicJobBlitNodeBuffer_p ;
#ifdef STBLIT_DEBUG
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "stblit_Connect2Nodes():Job...Old Address:0x%x ...New Address: 0x%x\n", SecondNode_p, Tmp ));
#endif
    }
    else
    if ( PhysicSingleBlitNodeBuffer_p )
    {
        Tmp = (U32) SecondNode_p - (U32) Device_p->SingleBlitNodeBuffer_p + (U32) PhysicSingleBlitNodeBuffer_p ;
#ifdef STBLIT_DEBUG
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "stblit_Connect2Nodes():Single...Old Address:0x%x ...New Address: 0x%x\n", SecondNode_p, Tmp ));
#endif
    }

#else
    Tmp = (U32)stblit_CpuToDevice((void*)SecondNode_p,&(Device_p->VirtualMapping));
#endif

#else /*!PTV*/
    Tmp = (U32)stblit_CpuToDevice((void*)SecondNode_p);
#endif

#if defined (DVD_SECURED_CHIP)
        MemoryStatus = STMES_IsMemorySecure((void *)Tmp, sizeof(stblit_Node_t), 0);
        if(MemoryStatus == SECURE_REGION)
        {
            STSYS_WriteRegMem32LE((void*)(&(FirstNode_p->HWNode.BLT_NIP)), (STBLIT_SECURE_ON | Tmp));
        }
        else
        {
            STSYS_WriteRegMem32LE((void*)(&(FirstNode_p->HWNode.BLT_NIP)), Tmp);
        }
#else
        STSYS_WriteRegMem32LE((void*)(&(FirstNode_p->HWNode.BLT_NIP)), Tmp);
#endif
    /* Update previous Node of the second node*/
    SecondNode_p->PreviousNode = (stblit_NodeHandle_t)FirstNode_p;

    return(Err);
 }

/*******************************************************************************
Name        : stblit_DisableNodeNotif
Description : Disable NodeNotif field in the node BLT_INS register
Parameters  :
Assumptions : All parameters checks done at upper level
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_DisableNodeNotif(stblit_Node_t*   Node_p)
 {
    ST_ErrorCode_t   Err = ST_NO_ERROR;
    U32              INS;

    INS = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS))); /* Read BLT_INS */
    INS = INS & 0x7FFFFFFF ; /* Set BLITCOMPIRQ bit to 0 */
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)), INS); /* Write updated BLT_INS */

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
    U32              CIC;

    /* Initialize Node common stuff*/
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    Node_p->SubscriberID        = BlitContext_p->EventSubscriberID;
#endif
    Node_p->JobHandle           = BlitContext_p->JobHandle;
    Node_p->PreviousNode        = STBLIT_NO_NODE_HANDLE;    /* Only one node operation */
    Node_p->IsContinuationNode  = FALSE;
    /*-----------------5/31/2004 3:19PM-----------------
     * PORT-5100 : BLT_CIC is a new register in 5100
     * --------------------------------------------------*/
    /* Initialize BLT_CIC */
    CIC = ((U32)( STBLIT_CIC_GROUPS_ENABLE & STBLIT_CIC_GROUPS_MASK) << STBLIT_CIC_GROUPS_SHIFT );

    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_CIC)),CIC);

    return(Err);
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

    UNUSED_PARAMETER(Device_p);

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
    S1TY |=  ((U32)(AlphaRange & STBLIT_S1TY_ALPHA_RANGE_MASK) << STBLIT_S1TY_ALPHA_RANGE_SHIFT);

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
#if defined (DVD_SECURED_CHIP)
    U8 MemoryStatus;
#endif

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);



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
    Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)(((U32)Bitmap_p->Data1_p) + Bitmap_p->Offset)));

#else
    Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif

#if defined (DVD_SECURED_CHIP)
    MemoryStatus = STMES_IsMemorySecure((void *)Tmp, Bitmap_p->Size1, 0);
    if(MemoryStatus == SECURE_REGION)
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1BA)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_S1BA_POINTER_MASK) << STBLIT_S1BA_POINTER_SHIFT)));
    }
    else
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1BA)),((U32)(Tmp & STBLIT_S1BA_POINTER_MASK) << STBLIT_S1BA_POINTER_SHIFT));
    }
#else
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1BA)),((U32)(Tmp & STBLIT_S1BA_POINTER_MASK) << STBLIT_S1BA_POINTER_SHIFT));
#endif
    /* Set Src1 Type */
    FormatColorType(Bitmap_p->ColorType,&ColorType,&AlphaRange,Bitmap_p->BitmapType); /* Prepare color Type HW representation*/

    S1TY = ((U32)(Bitmap_p->Pitch & STBLIT_S1TY_PITCH_MASK) << STBLIT_S1TY_PITCH_SHIFT);
    S1TY |=  ((U32)(ColorType & STBLIT_S1TY_COLOR_MASK) << STBLIT_S1TY_COLOR_SHIFT ); /* TBD */
    S1TY |=  ((U32)(ScanConfig & STBLIT_S1TY_SCAN_ORDER_MASK) << STBLIT_S1TY_SCAN_ORDER_SHIFT );
    S1TY |=  ((U32)(Bitmap_p->SubByteFormat & STBLIT_S1TY_SUB_BYTE_MASK) << STBLIT_S1TY_SUB_BYTE_SHIFT);
    /* GNBvd43933 "Color Conversion performed for blitter copy mode "STBLIT_CopyRectangl" fix */
    S1TY |=  ((U32)(1 & STBLIT_S1TY_RGB_EXPANSION_MASK) << STBLIT_S1TY_RGB_EXPANSION_SHIFT);
	S1TY |=  ((U32)(AlphaRange & STBLIT_S1TY_ALPHA_RANGE_MASK) << STBLIT_S1TY_ALPHA_RANGE_SHIFT);
#if defined (ST_7109) || defined (ST_7200)
    if (Bitmap_p->BigNotLittle == TRUE)
    {
        S1TY |=  ((U32)(1 & STBLIT_S1TY_BIG_NOT_LITTLE_MASK) << STBLIT_S1TY_BIG_NOT_LITTLE_SHIFT);
    }
#endif

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

    UNUSED_PARAMETER(Device_p);

    /* Src2 Instruction */
    CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_COLOR_FILL & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);

    /* Src2 Type (Default for RGB Expansion is : Missing LSBs are filled with 0) */
    FormatColorType(Color_p->Type,&ColorType,&AlphaRange,STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE) ; /* Prepare color Type HW representation*/
    S2TY = ((U32)(ColorType & STBLIT_S2TY_COLOR_MASK) << STBLIT_S2TY_COLOR_SHIFT );
    S2TY |= ((U32)(1 & STBLIT_S2TY_RGB_EXPANSION_MASK) << STBLIT_S2TY_RGB_EXPANSION_SHIFT);
    S2TY |=  ((U32)(AlphaRange & STBLIT_S2TY_ALPHA_RANGE_MASK) << STBLIT_S2TY_ALPHA_RANGE_SHIFT);

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
                                                 stblit_CommonField_t*  CommonField_p,
                                                 U32                    ChromaWidth,
                                                 S32                    ChromaPositionX,
                                                 U32                    ChromaHeight,
                                                 S32                    ChromaPositionY)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             ColorType = 0;
    U32             Tmp;
    U32             S2TY, S2SZ;
    S32             S2XY;
    U8              AlphaRange = 0;
#if defined (DVD_SECURED_CHIP)
    U8 MemoryStatus;
#endif

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* INS Instruction */
    CommonField_p->INS |= ((U32)(STBLIT_INS_SRC2_MODE_MEMORY & STBLIT_INS_SRC2_MODE_MASK) << STBLIT_INS_SRC2_MODE_SHIFT);
    if (( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB ) || ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP ))
    {
        CommonField_p->INS |= ((U32)(STBLIT_INS_SRC3_MODE_MEMORY & STBLIT_INS_SRC3_MODE_MASK) << STBLIT_INS_SRC3_MODE_SHIFT);
    }
    /* Src Base Address */
    if ( ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE ) || ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM ) )
    {
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
#else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
#if defined (DVD_SECURED_CHIP)
        MemoryStatus = STMES_IsMemorySecure((void *)Tmp, Bitmap_p->Size1, 0);
        if(MemoryStatus == SECURE_REGION)
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT)));
        }
        else
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));
        }
#else
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));
#endif
    }
    else /* ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB ) */
    {
        /* Chroma */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data2_p) + Bitmap_p->Offset);
#else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data2_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
#if defined (DVD_SECURED_CHIP)
        MemoryStatus = STMES_IsMemorySecure((void *)Tmp, Bitmap_p->Size2, 0);
        if(MemoryStatus == SECURE_REGION)
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT)));
        }
        else
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));
        }
#else
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));
#endif
        /* Luma */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
#else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
#if defined (DVD_SECURED_CHIP)
        MemoryStatus = STMES_IsMemorySecure((void *)Tmp, Bitmap_p->Size1, 0);
        if(MemoryStatus == SECURE_REGION)
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S3BA)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT )));
        }
        else
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S3BA)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));
        }
#else
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S3BA)),((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));
#endif
    }


    /* Src Type */
    FormatColorType(Bitmap_p->ColorType,&ColorType,&AlphaRange,Bitmap_p->BitmapType) ; /* Prepare color Type HW representation*/
#ifdef STBLIT_DEBUG
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "S2TY Bitmap_p->Pitch = 0x%x \n",  Bitmap_p->Pitch ));
#endif
    S2TY = ((U32)(Bitmap_p->Pitch & STBLIT_S2TY_PITCH_MASK) << STBLIT_S2TY_PITCH_SHIFT );
    S2TY |= ((U32)(ColorType & STBLIT_S2TY_COLOR_MASK) << STBLIT_S2TY_COLOR_SHIFT);
    S2TY |= ((U32)(ScanConfig & STBLIT_S2TY_SCAN_ORDER_MASK) << STBLIT_S2TY_SCAN_ORDER_SHIFT);
    S2TY |= ((U32)(Bitmap_p->SubByteFormat & STBLIT_S2TY_SUB_BYTE_MASK) << STBLIT_S2TY_SUB_BYTE_SHIFT);
    S2TY |=  ((U32)(AlphaRange & STBLIT_S2TY_ALPHA_RANGE_MASK) << STBLIT_S2TY_ALPHA_RANGE_SHIFT);
    /* GNBvd43933 "Color Conversion performed for blitter copy mode "STBLIT_CopyRectangl" fix */
    S2TY |= ((U32)(1 & STBLIT_S2TY_RGB_EXPANSION_MASK) << STBLIT_S2TY_RGB_EXPANSION_SHIFT);

#if defined (ST_7109) || defined (ST_7200)
    if (Bitmap_p->BigNotLittle == TRUE)
    {
        S2TY |=  ((U32)(1 & STBLIT_S2TY_BIG_NOT_LITTLE_MASK) << STBLIT_S2TY_BIG_NOT_LITTLE_SHIFT);
    }
#endif

    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)),S2TY);

    if (( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB ) || (( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP )))
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S3TY)),S2TY);
    }


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

    if (( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB ) || (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP))
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S3XY)),S2XY);

        S2XY = 0;
        S2XY |= ((U32)(ChromaPositionX & STBLIT_S3XY_X_MASK) << STBLIT_S3XY_X_SHIFT);
        S2XY |= ((U32)(ChromaPositionY & STBLIT_S3XY_Y_MASK) << STBLIT_S3XY_Y_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2XY)),S2XY);


    /* Src2 Size */
    S2SZ = ((U32)(Rectangle_p->Width & STBLIT_S2SZ_WIDTH_MASK) << STBLIT_S2SZ_WIDTH_SHIFT);
    S2SZ |= ((U32)(Rectangle_p->Height & STBLIT_S2SZ_HEIGHT_MASK) << STBLIT_S2SZ_HEIGHT_SHIFT);

    if (( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB ) || (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP))
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S3SZ)),S2SZ);

        S2SZ = 0;
        S2SZ |= ((U32)(ChromaWidth & STBLIT_S3SZ_WIDTH_MASK) << STBLIT_S3SZ_WIDTH_SHIFT);
        S2SZ |= ((U32)(ChromaHeight & STBLIT_S3SZ_HEIGHT_MASK) << STBLIT_S3SZ_HEIGHT_SHIFT);
    }
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2SZ)),S2SZ);

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
                                            stblit_CommonField_t*   CommonField_p,
                                            U32                     ChromaWidth,
                                            S32                     ChromaPositionX,
                                            U32                     ChromaHeight,
                                            S32                     ChromaPositionY)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

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
            SetNodeSrc2Bitmap(Node_p, Src_p->Data.Bitmap_p, &(Src_p->Rectangle), ScanConfig,Device_p, CommonField_p,
                              ChromaWidth, ChromaPositionX, ChromaHeight, ChromaPositionY);
        }
    }

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

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* Target Base Address */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
#else
    Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TBA)),((U32)(Tmp & STBLIT_TBA_POINTER_MASK) << STBLIT_TBA_POINTER_SHIFT));

    /* Target type */
    FormatColorType(Bitmap_p->ColorType,&ColorType,&AlphaRange,Bitmap_p->BitmapType) ; /* Prepare color Type HW representation*/
#ifdef STBLIT_DEBUG
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "TTY Bitmap_p->Pitch = 0x%x \n",  Bitmap_p->Pitch ));
#endif
    TTY = ((U32)(Bitmap_p->Pitch & STBLIT_TTY_PITCH_MASK) << STBLIT_TTY_PITCH_SHIFT );
    TTY |= ((U32)(ColorType & STBLIT_TTY_COLOR_MASK) << STBLIT_TTY_COLOR_SHIFT );
    TTY |= ((U32)(ScanConfig & STBLIT_TTY_SCAN_ORDER_MASK) << STBLIT_TTY_SCAN_ORDER_SHIFT );
    TTY |= ((U32)(Bitmap_p->SubByteFormat & STBLIT_TTY_SUB_BYTE_MASK) << STBLIT_TTY_SUB_BYTE_SHIFT);
    TTY |=  ((U32)(AlphaRange & STBLIT_TTY_ALPHA_RANGE_MASK) << STBLIT_TTY_ALPHA_RANGE_SHIFT);  /* PORT-5100 */
#if defined (ST_7109) || defined (ST_7200)
    /*-----------------5/31/2004 5:22PM-----------------
     * PORT-5100 : This field (TTY_BIG_NOT_LITTLE)
     * dispeare in 5100
     * --------------------------------------------------*/
    if (Bitmap_p->BigNotLittle == TRUE)
    {
        TTY |=  ((U32)(1 & STBLIT_TTY_BIG_NOT_LITTLE_MASK) << STBLIT_TTY_BIG_NOT_LITTLE_SHIFT);
    }
#endif
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
    /*-----------------5/31/2004 5:55PM-----------------
     * PORT-5100 : Target width and height are
     * defined in BLT_T_S1_SZ register in 5100.
     * --------------------------------------------------*/
    S1SZ = ((U32)(Rectangle_p->Width & STBLIT_T_S1_SZ_WIDTH_MASK) << STBLIT_T_S1_SZ_WIDTH_SHIFT);
    S1SZ |= ((U32)(Rectangle_p->Height & STBLIT_T_S1_SZ_HEIGHT_MASK) << STBLIT_T_S1_SZ_HEIGHT_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_T_S1_SZ)),S1SZ);

    return(Err);
 }


/*******************************************************************************
Name        : SelectHFilter
Description : Set the horizontal filtre offset
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SelectHFilter (U32 Increment,U32* Offset_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    if (Increment <= 921)
    {
        *Offset_p = 0;
    }
    else if (Increment <=1024)
    {
        *Offset_p = 1 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1126)
    {
        *Offset_p = 2 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1228)
    {
        *Offset_p = 3 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1331)
    {
        *Offset_p = 4 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1433)
    {
        *Offset_p = 5 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1536)
    {
        *Offset_p = 6 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=2048)
    {
        *Offset_p = 7 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=3072)
    {
        *Offset_p = 8 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=4096)
    {
        *Offset_p = 9 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=5120)
    {
        *Offset_p = 10 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }
    else /*  Increment <=6144 */
    {
        *Offset_p = 11 * STBLIT_HFILTER_COEFFICIENTS_SIZE;
    }

    return(Err);
 }

/*******************************************************************************
Name        : SelectVFilter
Description : Set the vertical filtre offset
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SelectVFilter (U32 Increment,U32* Offset_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    if (Increment <= 921)
    {
        *Offset_p = 0;
    }
    else if (Increment <=1024)
    {
        *Offset_p = 1 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1126)
    {
        *Offset_p = 2 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1228)
    {
        *Offset_p = 3 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1331)
    {
        *Offset_p = 4 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1433)
    {
        *Offset_p = 5 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=1536)
    {
        *Offset_p = 6 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=2048)
    {
        *Offset_p = 7 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=3072)
    {
        *Offset_p = 8 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=4096)
    {
        *Offset_p = 9 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else if (Increment <=5120)
    {
        *Offset_p = 10 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
    }
    else /*  Increment <=6144 */
    {
        *Offset_p = 11 * STBLIT_VFILTER_COEFFICIENTS_SIZE;
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
__inline static ST_ErrorCode_t GetFilterMode (STGXOBJ_ColorType_t ColorType, U8 ClutEnable, U8 ClutMode, U8* FilterMode)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    /* Look at color format when arriving in the HW 2D-resize block */
    if ((ClutEnable != 0) && (ClutMode == 0))
    {
        /* Color Expansion => Format is ARGB8888, i.e filter can be done on both alpha and color */
        *FilterMode = STBLIT_2DFILTER_ACTIV_BOTH;
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

#if defined (ST_7109) || defined (ST_7200)
/*******************************************************************************
Name        : SetNodeRangeMode
Description : As Range but without CommonField parameter
Parameters  :
Assumptions :  All parameters checks done at upper level. Node Not initialized !
               All parameters are unsigned !
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeRangeMode (stblit_Node_t*         Node_p,
                                                   STGXOBJ_Bitmap_t*       Bitmap_p,
                                                   stblit_CommonField_t* CommonField_p
                                                   )
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             VC1R;

    /* Instruction : Reset and Enable VC1 Range Mapping/Reduction engine */
    CommonField_p->INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_VC1R_MASK) << STBLIT_INS_ENABLE_VC1R_SHIFT)));
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_VC1R_MASK) << STBLIT_INS_ENABLE_VC1R_SHIFT);

    /* Enable Range Mapping/Reduction on Luma component*/
    VC1R = ((U32)(1 & STBLIT_VC1R_LUMA_MAP_MASK) << STBLIT_VC1R_LUMA_MAP_SHIFT);
    /*Luma coefficient to be used*/
    VC1R |= ((U32)((Bitmap_p->YUVScaling.ScalingFactorY - 2) & STBLIT_VC1R_LUMA_COEFF_MASK) << STBLIT_VC1R_LUMA_COEFF_SHIFT);
    /* Enable Range Mapping/Reduction on Chroma component*/
    VC1R |= ((U32)(1 & STBLIT_VC1R_CHROMA_MAP_MASK) << STBLIT_VC1R_CHROMA_MAP_SHIFT);
    /*Chroma coefficient to be used*/
    VC1R |= ((U32)((Bitmap_p->YUVScaling.ScalingFactorUV - 2) & STBLIT_VC1R_CHROMA_COEFF_MASK) << STBLIT_VC1R_CHROMA_COEFF_SHIFT);

    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VC1R)),VC1R);

    return(Err);
 }
#endif

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
                                                stblit_CommonField_t*   CommonField_p,
                                                U32                     HIncrement,
                                                U32                     HNbRepeat,
                                                U32                     HSRC_InitPhase,
                                                U32                     VIncrement,
                                                U32                     VNbRepeat,
                                                U32                     VSRC_InitPhase,
                                                BOOL                    SrcMb,
                                                U32                     Chroma_HSRC_InitPhase,
                                                U32                     ChromaHNbRepeat,
                                                U32                     ChromaHScalingFactor,
                                                U32                     Chroma_VSRC_InitPhase,
                                                U32                     ChromaVNbRepeat,
                                                U32                     ChromaVScalingFactor)

 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             HOffset;
    U32             VOffset;
    U32             Tmp;
    U32             RSF;
#if defined (DVD_SECURED_CHIP)
    U8 MemoryStatus;
#endif

    UNUSED_PARAMETER(SrcWidth);
    UNUSED_PARAMETER(SrcHeight);
    UNUSED_PARAMETER(DstWidth);
    UNUSED_PARAMETER(DstHeight);

    if ( (HIncrement != 1024) || (VIncrement != 1024) || (SrcMb) ) /* No resize if scaling factor = 1 and Raster*/
    {
        /* Instruction : Enable 2D Resize */
        CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_RESCALE_MASK) << STBLIT_INS_ENABLE_RESCALE_SHIFT);
        CommonField_p->RZI = 0;
        RSF = 0;

        if ( SrcMb )
        {
            U32             Y_RSF = 0;
            U32             Y_RZI = 0;

            /*
             * LUMA Configuration
             */

            /* Updating general horizontal registers  */
            if (HIncrement != 1024)
            {
                /* 2D Resize control */
                CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_LUMA_2DHRESIZER_ENABLE & STBLIT_RZC_LUMA_2DHRESIZER_MODE_MASK) << STBLIT_RZC_LUMA_2DHRESIZER_MODE_SHIFT);

                /* Initial subposition and Nb_Repeat*/
                Y_RZI |= ((U32)(HSRC_InitPhase & STBLIT_Y_RZI_INITIAL_HSRC_MASK) << STBLIT_Y_RZI_INITIAL_HSRC_SHIFT);
                Y_RZI |= ((U32)(HNbRepeat & STBLIT_Y_RZI_HNB_REPEAT_MASK) << STBLIT_Y_RZI_HNB_REPEAT_SHIFT);

                if ((FilterMode & STBLIT_2DFILTER_MODE_MASK) != STBLIT_2DFILTER_INACTIVE)
                {
                    /* FilterMode have been set by constants which are consistent with Horizantal constants */
                    CommonField_p->F_RZC_CTL |= ((U32)(FilterMode & STBLIT_RZC_LUMA_2DHFILTER_MODE_MASK) << STBLIT_RZC_LUMA_2DHFILTER_MODE_SHIFT);

                    /* Select Best default filter from filter buffer */
                    SelectHFilter(HIncrement, &HOffset);

                    /* Set horizontal filter */
                    #ifdef STBLIT_LINUX_FULL_USER_VERSION
                        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Device_p->HFilterBuffer_p) + HOffset);
                    #else
                        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->HFilterBuffer_p, &(Device_p->VirtualMapping)) + HOffset);
                    #endif

#if defined (DVD_SECURED_CHIP)
                MemoryStatus = STMES_IsMemorySecure((void *)Tmp, 0, 0);
                if(MemoryStatus == SECURE_REGION)
                {
                    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_HFP)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_Y_HFP_POINTER_MASK) << STBLIT_Y_HFP_POINTER_SHIFT )));
                }
                else
                {
                    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_HFP)),((U32)(Tmp & STBLIT_Y_HFP_POINTER_MASK) << STBLIT_Y_HFP_POINTER_SHIFT ));
                }
#else
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_HFP)),((U32)(Tmp & STBLIT_Y_HFP_POINTER_MASK) << STBLIT_Y_HFP_POINTER_SHIFT ));
#endif
                }
            }

            /* Updating general vertical registers  */
            if (VIncrement != 1024)
            {
                /* 2D Resize control */
                CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_LUMA_2DVRESIZER_ENABLE & STBLIT_RZC_LUMA_2DVRESIZER_MODE_MASK) << STBLIT_RZC_LUMA_2DVRESIZER_MODE_SHIFT);

                /* Initial subposition and Nb_Repeat*/
                Y_RZI |= ((U32)(VSRC_InitPhase & STBLIT_Y_RZI_INITIAL_VSRC_MASK) << STBLIT_Y_RZI_INITIAL_VSRC_SHIFT);
                Y_RZI |= ((U32)(VNbRepeat & STBLIT_Y_RZI_VNB_REPEAT_MASK) << STBLIT_Y_RZI_VNB_REPEAT_SHIFT);

                if ((FilterMode & STBLIT_2DFILTER_MODE_MASK) != STBLIT_2DFILTER_INACTIVE)
                {
                    /* FilterMode have been set by constants which are consistent with Vertical constants */
                    CommonField_p->F_RZC_CTL |= ((U32)(FilterMode & STBLIT_RZC_LUMA_2DVFILTER_MODE_MASK) << STBLIT_RZC_LUMA_2DVFILTER_MODE_SHIFT);

                    /* Select Best default filter from filter buffer */
                    SelectVFilter(VIncrement, &VOffset);

                    /* Set horizontal filter */
                    #ifdef STBLIT_LINUX_FULL_USER_VERSION
                        Tmp = (U32) Device_p->VFilterBuffer_p - (U32) Device_p->HFilterBuffer_p + PhysicWorkBuffer_p  + VOffset ;
                    #else
                        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->VFilterBuffer_p, &(Device_p->VirtualMapping)) + VOffset);
                    #endif

#if defined (DVD_SECURED_CHIP)
                    MemoryStatus = STMES_IsMemorySecure((void *)Tmp, 0, 0);
                    if(MemoryStatus == SECURE_REGION)
                    {
                        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_VFP)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_Y_VFP_POINTER_MASK) << STBLIT_Y_VFP_POINTER_SHIFT )));
                    }
                    else
                    {
                        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_VFP)),((U32)(Tmp & STBLIT_Y_VFP_POINTER_MASK) << STBLIT_Y_VFP_POINTER_SHIFT ));
                    }
#else
                    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_VFP)),((U32)(Tmp & STBLIT_Y_VFP_POINTER_MASK) << STBLIT_Y_VFP_POINTER_SHIFT ));
#endif
                }
            }

            /* Updating increment register */
            Y_RSF = 0;
            Y_RSF |= ((U32)(HIncrement & STBLIT_Y_RSF_INCREMENT_HSRC_MASK) << STBLIT_Y_RSF_INCREMENT_HSRC_SHIFT);
            Y_RSF |= ((U32)(VIncrement & STBLIT_Y_RSF_INCREMENT_VSRC_MASK) << STBLIT_Y_RSF_INCREMENT_VSRC_SHIFT);

            /* Write modified RSF and RZI value */
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_RSF)),Y_RSF);
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_RZI)),Y_RZI);

            /*
             * CHROMA Configuration
             */

            /* 2D Resize control */
            CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_CHROMA_2DHRESIZER_ENABLE_BOTH_CHANEL & STBLIT_RZC_CHROMA_2DHRESIZER_MODE_MASK) << STBLIT_RZC_CHROMA_2DHRESIZER_MODE_SHIFT);
            CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_CHROMA_2DVRESIZER_ENABLE_BOTH_CHANEL & STBLIT_RZC_CHROMA_2DVRESIZER_MODE_MASK) << STBLIT_RZC_CHROMA_2DVRESIZER_MODE_SHIFT);

            /* Initial subposition */
            CommonField_p->RZI |= ((U32)(Chroma_HSRC_InitPhase & STBLIT_RZI_INITIAL_HSRC_MASK) << STBLIT_RZI_INITIAL_HSRC_SHIFT);
            CommonField_p->RZI |= ((U32)(ChromaHNbRepeat & STBLIT_RZI_HNB_REPEAT_MASK) << STBLIT_RZI_HNB_REPEAT_SHIFT);
            CommonField_p->RZI |= ((U32)(Chroma_VSRC_InitPhase & STBLIT_RZI_INITIAL_VSRC_MASK) << STBLIT_RZI_INITIAL_VSRC_SHIFT);
            CommonField_p->RZI |= ((U32)(ChromaVNbRepeat & STBLIT_RZI_VNB_REPEAT_MASK) << STBLIT_RZI_VNB_REPEAT_SHIFT);

            /* Updating increment register */
            RSF = 0;
            RSF |= ((U32)(ChromaHScalingFactor & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
            RSF |= ((U32)(ChromaVScalingFactor & STBLIT_RSF_INCREMENT_VSRC_MASK) << STBLIT_RSF_INCREMENT_VSRC_SHIFT);
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)),RSF);

            /* Select Best default filter from filter buffer */
            SelectHFilter(ChromaHScalingFactor, &HOffset);
            SelectVFilter(ChromaVScalingFactor, &VOffset);

            /* Set horizontal filter */
            #ifdef STBLIT_LINUX_FULL_USER_VERSION
                Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Device_p->HFilterBuffer_p) + VOffset);
            #else
                Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->HFilterBuffer_p, &(Device_p->VirtualMapping)) + HOffset);
            #endif

#if defined (DVD_SECURED_CHIP)
            MemoryStatus = STMES_IsMemorySecure((void *)Tmp, 0, 0);
            if(MemoryStatus == SECURE_REGION)
            {
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_HFP)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT )));
            }
            else
            {
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_HFP)),((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT ));
            }
#else
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_HFP)),((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT ));
#endif
            /* Set vertical filter */
            #ifdef STBLIT_LINUX_FULL_USER_VERSION
                  Tmp = (U32) Device_p->VFilterBuffer_p - (U32) Device_p->HFilterBuffer_p + PhysicWorkBuffer_p  + VOffset ;
            #else
                Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->VFilterBuffer_p, &(Device_p->VirtualMapping)) + VOffset);
            #endif

#if defined (DVD_SECURED_CHIP)
            MemoryStatus = STMES_IsMemorySecure((void *)Tmp, 0, 0);
            if(MemoryStatus == SECURE_REGION)
            {
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VFP)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT )));
            }
            else
            {
                STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VFP)),((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT ));
            }
#else
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VFP)),((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT ));
#endif
        }
        else   /* Raster */
        {
            /* Updating general horizontal registers  */
            if (HIncrement != 1024)
            {
                /* 2D Resize control */
                CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_2DHRESIZER_ENABLE & STBLIT_RZC_2DHRESIZER_MODE_MASK) << STBLIT_RZC_2DHRESIZER_MODE_SHIFT);

                /* Initial subposition */
                CommonField_p->RZI |= ((U32)(HSRC_InitPhase & STBLIT_RZI_INITIAL_HSRC_MASK) << STBLIT_RZI_INITIAL_HSRC_SHIFT);
                CommonField_p->RZI |= ((U32)(HNbRepeat & STBLIT_RZI_HNB_REPEAT_MASK) << STBLIT_RZI_HNB_REPEAT_SHIFT);

                if ((FilterMode & STBLIT_2DFILTER_MODE_MASK) != STBLIT_2DFILTER_INACTIVE)
                {
                    /* FilterMode have been set by constants which are consistent with Horizantal constants */
                    CommonField_p->F_RZC_CTL |= ((U32)(FilterMode & STBLIT_RZC_2DHFILTER_MODE_MASK) << STBLIT_RZC_2DHFILTER_MODE_SHIFT);

                    /* Select Best default filter from filter buffer */
                    SelectHFilter(HIncrement, &HOffset);

                    /* Set horizontal filter */
                    #ifdef STBLIT_LINUX_FULL_USER_VERSION
                        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Device_p->HFilterBuffer_p) + HOffset);
                    #else
                        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->HFilterBuffer_p, &(Device_p->VirtualMapping)) + HOffset);
                    #endif

#if defined (DVD_SECURED_CHIP)
                    MemoryStatus = STMES_IsMemorySecure((void *)Tmp, 0, 0);
                    if(MemoryStatus == SECURE_REGION)
                    {
                        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_HFP)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT )));
                    }
                    else
                    {
                        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_HFP)),((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT ));
                    }
#else
                        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_HFP)),((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT ));
#endif
                }
            }

            /* Updating general vertical registers  */
            if (VIncrement != 1024)
            {
                /* 2D Resize control */
                CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_2DVRESIZER_ENABLE & STBLIT_RZC_2DVRESIZER_MODE_MASK) << STBLIT_RZC_2DVRESIZER_MODE_SHIFT);

                /* Initial subposition */
                CommonField_p->RZI |= ((U32)(VSRC_InitPhase & STBLIT_RZI_INITIAL_VSRC_MASK) << STBLIT_RZI_INITIAL_VSRC_SHIFT);
                CommonField_p->RZI |= ((U32)(VNbRepeat      & STBLIT_RZI_VNB_REPEAT_MASK) << STBLIT_RZI_VNB_REPEAT_SHIFT);

                if ((FilterMode & STBLIT_2DFILTER_MODE_MASK) != STBLIT_2DFILTER_INACTIVE)
                {
                    /* FilterMode have been set by constants which are consistent with Vertical constants */
                    CommonField_p->F_RZC_CTL |= ((U32)(FilterMode & STBLIT_RZC_2DVFILTER_MODE_MASK) << STBLIT_RZC_2DVFILTER_MODE_SHIFT);

                    /* Select Best default filter from filter buffer */
                    SelectVFilter(VIncrement, &VOffset);

                    /* Set Vertical filter */
                    #ifdef STBLIT_LINUX_FULL_USER_VERSION
                        Tmp = (U32) Device_p->VFilterBuffer_p - (U32) Device_p->HFilterBuffer_p + PhysicWorkBuffer_p  + VOffset ;
                    #else
                        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->VFilterBuffer_p, &(Device_p->VirtualMapping)) + VOffset);
                    #endif
#if defined (DVD_SECURED_CHIP)
                    MemoryStatus = STMES_IsMemorySecure((void *)Tmp, 0, 0);
                    if(MemoryStatus == SECURE_REGION)
                    {
                        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VFP)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT )));
                    }
                    else
                    {
                        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VFP)),((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT ));
                    }
#else
                    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VFP)),((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT ));
#endif
                }
            }

            /* Updating increment register */
            RSF = 0;
            RSF |= ((U32)(HIncrement & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
            RSF |= ((U32)(VIncrement & STBLIT_RSF_INCREMENT_VSRC_MASK) << STBLIT_RSF_INCREMENT_VSRC_SHIFT);

            /* Write modified RSF value */
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)),RSF);
        }
    }
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
                                                stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

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
                                                    stblit_CommonField_t*    CommonField_p,
                                                    stblit_Node_t*           Node_p)
 {
    ST_ErrorCode_t  Err  = ST_NO_ERROR;
#if defined (ST_7109) || defined (ST_7200)
    U8 EnableInputColorColorimetry, EnableInputChromaFormat, EnableInputColorDir, MatrixIndex;
    U32 IVMX = 0;
#endif /* defined (ST_7109) || defined (ST_7200) */

    /* Instruction : Enable Input color converter operator */
    CommonField_p->INS |= ((U32)(InEnable & STBLIT_INS_ENABLE_IN_CSC_MASK) << STBLIT_INS_ENABLE_IN_CSC_SHIFT);

#if defined (ST_7109) || defined (ST_7200)
    /* Get color conversion params */
    EnableInputColorColorimetry =  (U8)( InConfig & 1 );
    EnableInputChromaFormat = (U8)(( InConfig & 2 ) >> 1);
    EnableInputColorDir = (U8)(( InConfig & 4 ) >> 2);

    /* Get matrix index */
    MatrixIndex = GetMatrixIndex(EnableInputColorColorimetry, EnableInputChromaFormat, EnableInputColorDir);

    /* Set Coeff11, Coeff12, Coeff13 */
    IVMX = ( ( ( GetUpsizedValue(stblit_VersatileMatrix[MatrixIndex][2]) & STBLIT_VMX_COEFF3_MASK ) << STBLIT_VMX_COEFF3_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][1] & STBLIT_VMX_COEFF2_MASK ) << STBLIT_VMX_COEFF2_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][0] & STBLIT_VMX_COEFF1_MASK ) << STBLIT_VMX_COEFF1_SHIFT ) );
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_IVMX0)),IVMX);

    /* Set Coeff21, Coeff22, Coeff23 */
    IVMX = ( ( ( GetUpsizedValue(stblit_VersatileMatrix[MatrixIndex][5]) & STBLIT_VMX_COEFF3_MASK ) << STBLIT_VMX_COEFF3_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][4] & STBLIT_VMX_COEFF2_MASK ) << STBLIT_VMX_COEFF2_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][3] & STBLIT_VMX_COEFF1_MASK ) << STBLIT_VMX_COEFF1_SHIFT ) );
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_IVMX1)),IVMX);

    /* Set Coeff31, Coeff32, Coeff33 */
    IVMX = ( ( ( GetUpsizedValue(stblit_VersatileMatrix[MatrixIndex][8]) & STBLIT_VMX_COEFF3_MASK ) << STBLIT_VMX_COEFF3_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][7] & STBLIT_VMX_COEFF2_MASK ) << STBLIT_VMX_COEFF2_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][6] & STBLIT_VMX_COEFF1_MASK ) << STBLIT_VMX_COEFF1_SHIFT ) );
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_IVMX2)),IVMX);

    /* Set Offset1, Offset2, Offset3 */
    IVMX = ( ( ( stblit_VersatileMatrix[MatrixIndex][11] & STBLIT_VMX_OFFSET3_MASK ) << STBLIT_VMX_OFFSET3_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][10] & STBLIT_VMX_OFFSET2_MASK ) << STBLIT_VMX_OFFSET2_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][9] & STBLIT_VMX_OFFSET1_MASK ) << STBLIT_VMX_OFFSET1_SHIFT ) );
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_IVMX3)),IVMX);
#else
    /* Set operators */
    CommonField_p->CCO |= ((U32)(InConfig & STBLIT_CCO_IN_MASK) << STBLIT_CCO_IN_SHIFT);
#endif /* defined (ST_7109) || defined (ST_7200) */

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
  __inline static ST_ErrorCode_t SetNodeOutputConverter (U8                    OutEnable,
                                                         U8                    OutConfig,
                                                         U8                    InConfig,
                                                         stblit_CommonField_t* CommonField_p,
                                                         stblit_Node_t*        Node_p)
 {
    ST_ErrorCode_t  Err  = ST_NO_ERROR;
#if defined (ST_7109) || defined (ST_7200)
    U8 EnableOutColorColorimetry, EnableOutChromaFormat, EnableOutColorDir, MatrixIndex;
    U32 OVMX = 0;
#endif


    /* Instruction : Enable Output color converter operator */
    CommonField_p->INS |= ((U32)(OutEnable & STBLIT_INS_ENABLE_OUT_CSC_MASK) << STBLIT_INS_ENABLE_OUT_CSC_SHIFT);

#if defined (ST_7109) || defined (ST_7200)
    /* Get color conversion params */
    EnableOutColorColorimetry =  (U8)( OutConfig & 1 );
    EnableOutChromaFormat = (U8)(( OutConfig & 2 ) >> 1);
    EnableOutColorDir = (U8)(( InConfig & 4 ) >> 2);
    if ( EnableOutColorDir == 0 )
            EnableOutColorDir = 1;
    else
            EnableOutColorDir = 0;

    /* Get matrix index */
    MatrixIndex = GetMatrixIndex(EnableOutColorColorimetry, EnableOutChromaFormat, EnableOutColorDir);

    /* Set Coeff11, Coeff12, Coeff13 */
    OVMX = ( ( ( GetUpsizedValue(stblit_VersatileMatrix[MatrixIndex][2]) & STBLIT_VMX_COEFF3_MASK ) << STBLIT_VMX_COEFF3_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][1] & STBLIT_VMX_COEFF2_MASK ) << STBLIT_VMX_COEFF2_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][0] & STBLIT_VMX_COEFF1_MASK ) << STBLIT_VMX_COEFF1_SHIFT ) );
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_OVMX0)),OVMX);

    /* Set Coeff21, Coeff22, Coeff23 */
    OVMX = ( ( ( GetUpsizedValue(stblit_VersatileMatrix[MatrixIndex][5]) & STBLIT_VMX_COEFF3_MASK ) << STBLIT_VMX_COEFF3_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][4] & STBLIT_VMX_COEFF2_MASK ) << STBLIT_VMX_COEFF2_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][3] & STBLIT_VMX_COEFF1_MASK ) << STBLIT_VMX_COEFF1_SHIFT ) );
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_OVMX1)),OVMX);

    /* Set Coeff31, Coeff32, Coeff33 */
    OVMX = ( ( ( GetUpsizedValue(stblit_VersatileMatrix[MatrixIndex][8]) & STBLIT_VMX_COEFF3_MASK ) << STBLIT_VMX_COEFF3_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][7] & STBLIT_VMX_COEFF2_MASK ) << STBLIT_VMX_COEFF2_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][6] & STBLIT_VMX_COEFF1_MASK ) << STBLIT_VMX_COEFF1_SHIFT ) );
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_OVMX2)),OVMX);

    /* Set Offset1, Offset2, Offset3 */
    OVMX = ( ( ( stblit_VersatileMatrix[MatrixIndex][11] & STBLIT_VMX_OFFSET3_MASK ) << STBLIT_VMX_OFFSET3_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][10] & STBLIT_VMX_OFFSET2_MASK ) << STBLIT_VMX_OFFSET2_SHIFT ) |
             ( ( stblit_VersatileMatrix[MatrixIndex][9] & STBLIT_VMX_OFFSET1_MASK ) << STBLIT_VMX_OFFSET1_SHIFT ) );
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_OVMX3)),OVMX);
#else
    /* Set operators */
    CommonField_p->CCO |= ((U32)(OutConfig & STBLIT_CCO_OUT_MASK) << STBLIT_CCO_OUT_SHIFT);
#endif /* defined(ST_7109) || defined(ST_7200)*/

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
    Tmp =  (U32)stblit_CpuToDevice((void*)(BlitContext_p->ColorCorrectionTable_p->Data_p));
#else
    Tmp =  (U32)STAVMEM_VirtualToDevice((void*)(BlitContext_p->ColorCorrectionTable_p->Data_p),&(Device_p->VirtualMapping));
#endif

    CommonField_p->CML |= ((U32)(Tmp & STBLIT_CML_POINTER_MASK) << STBLIT_CML_POINTER_SHIFT);

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
    Tmp =  (U32)stblit_CpuToDevice((void*)(Palette_p->Data_p));
#else
    Tmp =  (U32)STAVMEM_VirtualToDevice((void*)(Palette_p->Data_p),&(Device_p->VirtualMapping));
#endif
    CommonField_p->CML |= ((U32)(Tmp & STBLIT_CML_POINTER_MASK) << STBLIT_CML_POINTER_SHIFT);

    return(Err);
 }

#if defined(ST_7109) || defined(ST_7200) || defined(STBLIT_USE_COLOR_KEY_FOR_BDISP)
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

    return(Err);
}
#endif /* (ST_7109) || defined(ST_7200) || (STBLIT_USE_COLOR_KEY_FOR_BDISP) */

#if defined(ST_7109) || defined(ST_7200)
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

    /* Instruction : Reset and Enable Plane mask */
    CommonField_p->INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_PLANE_MASK) << STBLIT_INS_ENABLE_PLANE_SHIFT)));
    CommonField_p->INS |= ((U32)(1 & STBLIT_INS_ENABLE_PLANE_MASK) << STBLIT_INS_ENABLE_PLANE_SHIFT);

    /* Set Plane mask value */
    STSYS_WriteRegMem32LE((void*)(&( Node_p->HWNode.BLT_PMK)),((U32)(MaskWord & STBLIT_PMK_VALUE_MASK) << STBLIT_PMK_VALUE_SHIFT));

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

    /* Instruction : Reset and Enable Plane mask */
    INS = (U32)STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)));
    INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_PLANE_MASK) << STBLIT_INS_ENABLE_PLANE_SHIFT)));
    INS |= ((U32)(1 & STBLIT_INS_ENABLE_PLANE_MASK) << STBLIT_INS_ENABLE_PLANE_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_INS)),INS);

    /* Set Plane mask value */
    STSYS_WriteRegMem32LE((void*)(&( Node_p->HWNode.BLT_PMK)),((U32)(MaskWord & STBLIT_PMK_VALUE_MASK) << STBLIT_PMK_VALUE_SHIFT));

    return(Err);
}

#ifdef ST_OSWINCE
    /*******************************************************************************
Name        : UpdateNodeGlobalAlpha
*******************************************************************************/
__inline static ST_ErrorCode_t UpdateNodeGlobalAlpha (stblit_Node_t* Node_p, U32 Value, U32 Mask)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;
	U32 ACK;

    // Get value of register
	ACK = (U32)STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_ACK)));

	// Change value of register
	ACK &= ~Mask;
	ACK |= Value;

	// Write it back
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_ACK)), ACK);

    return ST_NO_ERROR;
}

#endif

/*******************************************************************************
Name        : SetNodeALULogic
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeALULogic (STBLIT_BlitContext_t*    BlitContext_p,
                                                  stblit_CommonField_t*    CommonField_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    #ifdef STBLIT_BENCHMARK3
    t23 = STOS_time_now();
    #endif

    if (BlitContext_p->AluMode == STBLIT_ALU_NOOP)  /* Bypass Src1 */
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

    #ifdef STBLIT_BENCHMARK3
    t24 = STOS_time_now();
    #endif

    return(Err);
 }

#endif /* defined(ST_7109) || defined(ST_7200) */


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

    CanForegroundBeSet  = TRUE;
    CanBackgroundBeSet  = TRUE;
    CanDestinationBeSet = TRUE;
    Foreground_p        = Src2_p;
    Background_p        = Src1_p;
    Destination_p       = Dst_p;
    CanMaskBeSet        = FALSE;
    Config_p->MaskEnabled = FALSE;

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
Name        : SetNodeFlicker
Description :
Parameters  :
Assumptions : All parameters checks done at upper level. Node initialized
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t SetNodeFlicker (stblit_Node_t*          Node_p,
                                                stblit_CommonField_t*   CommonField_p,
                                                S32                     DstPositionY,
												U32                     DstBitmapHeight,
												S32*                    SrcPositionY_p,
												U32*                    SrcHeight_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
#if defined (ST_7109) || defined (ST_7200)
    U32             _FF0, _FF1, _FF2, _FF3;
#endif

    UNUSED_PARAMETER(Node_p);

    /* Instruction : Enable Flicker Filter */
	CommonField_p->INS |= ((U32)(STBLIT_INS_ENABLE_FLICKER_FILTER_VALUE & STBLIT_INS_ENABLE_FLICKER_FILTER_MASK) << STBLIT_INS_ENABLE_FLICKER_FILTER_SHIFT);


    if ((DstPositionY + (*SrcHeight_p)) == DstBitmapHeight)  /* Last line of the destination bitmap */
    {
		CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_FF_REPEAT_LL_ENABLE & STBLIT_RZC_FF_REPEAT_LL_MASK) << STBLIT_RZC_FF_REPEAT_LL_SHIFT);
    }
    else
    {
        (*SrcHeight_p) += 1;
    }

    if (DstPositionY == 0) /* First line of the destination bitmap */
    {
        CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_FF_REPEAT_FL_ENABLE & STBLIT_RZC_FF_REPEAT_FL_MASK) << STBLIT_RZC_FF_REPEAT_FL_SHIFT);
    }
    else     /* DstUpPosY > 0 */
    {
        (*SrcPositionY_p) -= 1;
        (*SrcHeight_p) += 1;
    }

#if defined (ST_7109) || defined (ST_7200)
    /* BLT_FF0 : Set value for Flicker filter  */
    _FF0 = ((U32)(32 & STBLIT_FF0_N_MINUS_1_COEFF_MASK) << STBLIT_FF0_N_MINUS_1_COEFF_SHIFT);
    _FF0 |= ((U32)(64 & STBLIT_FF0_N_COEFF_MASK) << STBLIT_FF0_N_COEFF_SHIFT);
    _FF0 |= ((U32)(32 & STBLIT_FF0_N_PLUS_1_COEFF_MASK) << STBLIT_FF0_N_PLUS_1_COEFF_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF0)),_FF0);

    /* BLT_FF1 : Set value for Flicker filter  */
    _FF1 = ((U32)(32 & STBLIT_FF1_N_MINUS_1_COEFF_MASK) << STBLIT_FF1_N_MINUS_1_COEFF_SHIFT);
    _FF1 |= ((U32)(64 & STBLIT_FF1_N_COEFF_MASK) << STBLIT_FF1_N_COEFF_SHIFT);
    _FF1 |= ((U32)(32 & STBLIT_FF1_N_PLUS_1_COEFF_MASK) << STBLIT_FF1_N_PLUS_1_COEFF_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF1)),_FF1);

    /* BLT_FF2 : Set value for Flicker filter  */
    _FF2 = ((U32)(32 & STBLIT_FF2_N_MINUS_1_COEFF_MASK) << STBLIT_FF2_N_MINUS_1_COEFF_SHIFT);
    _FF2 |= ((U32)(64 & STBLIT_FF2_N_COEFF_MASK) << STBLIT_FF2_N_COEFF_SHIFT);
    _FF2 |= ((U32)(32 & STBLIT_FF2_N_PLUS_1_COEFF_MASK) << STBLIT_FF2_N_PLUS_1_COEFF_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF2)),_FF2);

    /* BLT_FF3 : Set value for Flicker filter  */
    _FF3 = ((U32)(32 & STBLIT_FF3_N_MINUS_1_COEFF_MASK) << STBLIT_FF3_N_MINUS_1_COEFF_SHIFT);
    _FF3 |= ((U32)(64 & STBLIT_FF3_N_COEFF_MASK) << STBLIT_FF3_N_COEFF_SHIFT);
    _FF3 |= ((U32)(32 & STBLIT_FF3_N_PLUS_1_COEFF_MASK) << STBLIT_FF3_N_PLUS_1_COEFF_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_FF3)),_FF3);
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
 __inline static ST_ErrorCode_t SetNodeAntiFlutter(stblit_Node_t*           Node_p,
                                                   stblit_CommonField_t*    CommonField_p,
                                                   BOOL                     TopTile,
                                                   BOOL                     BottomTile)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    UNUSED_PARAMETER(Node_p);

    if(TopTile)
    {
        CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_ENABLE_AB_TOP_ENABLE & STBLIT_RZC_ENABLE_AB_TOP_MASK) << STBLIT_RZC_ENABLE_AB_TOP_SHIFT);
    }

    if(BottomTile)
    {
        CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_ENABLE_AB_BOTTOM_ENABLE & STBLIT_RZC_ENABLE_AB_BOTTOM_MASK) << STBLIT_RZC_ENABLE_AB_BOTTOM_SHIFT);
    }

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
 __inline static ST_ErrorCode_t SetNodeAntiAliasing (stblit_Node_t*            Node_p,
                                                          stblit_CommonField_t*     CommonField_p,
                                                          BOOL                      RightTile,
                                                          BOOL                      LeftTile)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    UNUSED_PARAMETER(Node_p);

    if(RightTile)
    {
        CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_ENABLE_AB_RIGHT_ENABLE & STBLIT_RZC_ENABLE_AB_RIGHT_MASK) << STBLIT_RZC_ENABLE_AB_RIGHT_SHIFT);
    }

    if(LeftTile)
    {
        CommonField_p->F_RZC_CTL |= ((U32)(STBLIT_RZC_ENABLE_AB_LEFT_ENABLE & STBLIT_RZC_ENABLE_AB_LEFT_MASK) << STBLIT_RZC_ENABLE_AB_LEFT_SHIFT);
    }

    return(Err);
 }


/*******************************************************************************
Name        : BlitOperationNodeSetup
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t BlitOperationNodeSetup( stblit_Node_t*                    Node_p,
                                                        STBLIT_BlitContext_t*             BlitContext_p,
                                                        STBLIT_Source_t*                  Src1_p,
                                                        STBLIT_Source_t*                  Src2_p,
                                                        STBLIT_Destination_t*             Dst_p,
                                                        stblit_Device_t*                  Device_p,
                                                        stblit_CommonField_t*             CommonField_p,
                                                        stblit_OperationConfiguration_t*  Config_p,
                                                        U8                                ScanConfig,
                                                        U8                                ClutMode,
                                                        U8                                InConfig,
                                                        U8                                OutConfig,
                                                        U8                                OutEnable,
                                                        U8                                InEnable,
                                                        U8                                ClutEnable,
                                                        U8                                FilterMode,
                                                        BOOL                              SrcMB,
                                                        U32                               HIncrement,
                                                        U32                               HNbRepeat,
                                                        U32                               HSRC_InitPhase,
                                                        U32                               VIncrement,
                                                        U32                               VNbRepeat,
                                                        U32                               VSRC_InitPhase,
                                                        U32                               Chroma_HSRC_InitPhase,
                                                        U32                               ChromaHNbRepeat,
                                                        U32                               ChromaHScalingFactor,
                                                        U32                               ChromaWidth,
                                                        S32                               ChromaPositionX,
                                                        U32                               Chroma_VSRC_InitPhase,
                                                        U32                               ChromaVNbRepeat,
                                                        U32                               ChromaVScalingFactor,
                                                        U32                               ChromaHeight,
                                                        S32                               ChromaPositionY,
                                                        BOOL                              LastNode,
                                                        U32                               DstBitmapHeight,
                                                        BOOL                              TopTile,
                                                        BOOL                              BottomTile,
                                                        BOOL                              RightTile,
                                                        BOOL                              LeftTile)
 {
     ST_ErrorCode_t   Err = ST_NO_ERROR;

    /* Init node */
    InitNode(Node_p, BlitContext_p);

	/* Set Flicker filter */
    if (BlitContext_p->EnableFlickerFilter == TRUE)
	{
		SetNodeFlicker (Node_p, CommonField_p, Dst_p->Rectangle.PositionY, DstBitmapHeight, &Src2_p->Rectangle.PositionY, &Src2_p->Rectangle.Height);
	}

    /* Set BorderAlpha for Anti-Flutter */
    if (Device_p->AntiFlutterOn == TRUE)
	{
        SetNodeAntiFlutter (Node_p, CommonField_p, TopTile, BottomTile);
	}

    /* Set BorderAlpha for Anti-Aliasing */
    if (Device_p->AntiAliasingOn == TRUE)
	{
        SetNodeAntiAliasing(Node_p, CommonField_p, RightTile, LeftTile);
	}

    /* set Src1 */
    SetNodeSrc1(Node_p, Src1_p, ScanConfig,Device_p, CommonField_p);

    /* Set Src2  */
    SetNodeSrc2(Node_p, Src2_p,&(Dst_p->Rectangle),ScanConfig,Device_p, CommonField_p,
                ChromaWidth, ChromaPositionX, ChromaHeight, ChromaPositionY);

    /* Set Target */
    SetNodeTgt(Node_p, Dst_p->Bitmap_p,&(Dst_p->Rectangle), ScanConfig,Device_p);

    /* Set Interrupts */
    SetNodeIT (Node_p, Device_p, BlitContext_p, CommonField_p, LastNode);

    /* Set Clipping */
    if (BlitContext_p->EnableClipRectangle == TRUE)
    {
        SetNodeClipping(Node_p,&(BlitContext_p->ClipRectangle), BlitContext_p->WriteInsideClipRectangle, CommonField_p);
    }

#if defined (ST_7109) || defined(ST_7200)
/* Set RangeMode */
    if ((Src1_p != NULL) && (Src1_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP))
    {
        SetNodeRangeMode(Node_p, Src1_p->Data.Bitmap_p, CommonField_p);
    }
    else if ((Src2_p != NULL) && (Src2_p->Data.Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP))
    {
        SetNodeRangeMode(Node_p, Src2_p->Data.Bitmap_p, CommonField_p);
    }

#endif /* defined(ST_7109) || defined(ST_7200) */

#if defined(ST_7109) || defined(ST_7200)
    /* Set Plane Mask */
    if (BlitContext_p->EnableMaskWord == TRUE)
    {
        SetNodePlaneMask(Node_p, BlitContext_p->MaskWord, CommonField_p);
    }
#endif /* defined(ST_7109) || defined(ST_7200) */

#if defined (ST_7109) || defined(ST_7200) || defined (STBLIT_USE_COLOR_KEY_FOR_BDISP)
    /* Set ColorKey */
    if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE)
    {
        SetNodeColorKey(Node_p, BlitContext_p->ColorKeyCopyMode, &(BlitContext_p->ColorKey),CommonField_p);
    }
#endif /* (ST_7109) || defined(ST_7200) || (STBLIT_USE_COLOR_KEY_FOR_BDISP) */

    if (Src2_p != NULL)
    {
        if (Src2_p->Type == STBLIT_SOURCE_TYPE_BITMAP)
        {
            /* Set Resize */
            /* Src1 and Dst have same size. Only resize possible between Src2 and Dst
            * Set Resize only if src2 is type bitmap : Src2 is always type color in color fill. It's not possible to have
            * src1 type color or destination type color and not src2*/
            GetFilterMode(Src2_p->Data.Bitmap_p->ColorType,ClutEnable,ClutMode,&FilterMode);

            SetNodeResize (Node_p, Src2_p->Rectangle.Width,Src2_p->Rectangle.Height,
                           Dst_p->Rectangle.Width,Dst_p->Rectangle.Height,Device_p,FilterMode, CommonField_p,
                           HIncrement,HNbRepeat,HSRC_InitPhase,VIncrement,VNbRepeat,VSRC_InitPhase,SrcMB,
                           Chroma_HSRC_InitPhase, ChromaHNbRepeat, ChromaHScalingFactor, Chroma_VSRC_InitPhase, ChromaVNbRepeat, ChromaVScalingFactor);
        }
    }

    /* Set Input converter */
    SetNodeInputConverter(InEnable, InConfig, CommonField_p, Node_p);

    /* Set Output converter */
    SetNodeOutputConverter(OutEnable, OutConfig, InConfig, CommonField_p, Node_p);


    if (ClutEnable) /* Expansion */
    {
        if (ClutMode == 0)
        {
            /* Set Color Expansion . In this case, Src2 is always not NULL (Src2 is NULL only if Table Single Src1 is used
             * and there is no CLUT expansion not reduction in this table) */
            SetNodeColorExpansion(Src2_p->Palette_p,Device_p, CommonField_p);
        }
    }
    else
    {
        if (BlitContext_p->EnableColorCorrection == TRUE)
        {
            /* Set Color correction */
            SetNodeColorCorrection(BlitContext_p,Device_p, CommonField_p);
        }
    }

    /* Set Blend */
#ifdef ST_OSWINCE
	Config_p->BlendOp = FALSE;
	Config_p->PreMultipliedColor = FALSE;
#endif
    if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)   /* In this case Both Src are not always not NULL! (cf upper level)*/
    {
#ifdef ST_OSWINCE
		Config_p->BlendOp = TRUE;
#endif
        if (Src2_p->Type == STBLIT_SOURCE_TYPE_BITMAP)
        {
            SetNodeALUBlend(BlitContext_p->GlobalAlpha ,Src2_p->Data.Bitmap_p->PreMultipliedColor, CommonField_p);
#ifdef ST_OSWINCE
			Config_p->PreMultipliedColor = Src2_p->Data.Bitmap_p->PreMultipliedColor;
#endif
        }
        else /* Type Color */
        {
            SetNodeALUBlend(BlitContext_p->GlobalAlpha ,FALSE, CommonField_p);
        }
        /* Update pre-multiplied info in destination if needed ...TBDone */
    }
#if defined(ST_7109) || defined(ST_7200)
    else if ((BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
            (BlitContext_p->EnableMaskWord == FALSE) &&
            (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
    {
        /*Bypass Src2*/
        if ( Src2_p!=NULL)
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }

        /*Bypass Src1*/
        if ( Src1_p!=NULL)
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC1 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_COPY) && (BlitContext_p->EnableMaskWord == TRUE))
    {
        /* ALU control */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_LOGICAL & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        CommonField_p->ACK |= ((U32)(BlitContext_p->AluMode & STBLIT_ACK_ALU_ROP_MODE_MASK) << STBLIT_ACK_ALU_ROP_MODE_SHIFT);
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_COPY) && (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST))
    {
        /* ALU control */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_LOGICAL & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        CommonField_p->ACK |= ((U32)(0x5 & STBLIT_ACK_ALU_ROP_MODE_MASK) << STBLIT_ACK_ALU_ROP_MODE_SHIFT);
    }
    else /* ALU operations */
    {
        SetNodeALULogic(BlitContext_p, CommonField_p);
    }
#else
#ifdef STBLIT_USE_COLOR_KEY_FOR_BDISP
    else if ((BlitContext_p->AluMode == STBLIT_ALU_COPY) &&
            (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
    {
        /*Bypass Src2*/
        if ( Src2_p!=NULL)
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
        /*Bypass Src1*/
        if ( Src1_p!=NULL)
        {
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC1 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_COPY) && (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST))
    {
        /* ALU control */
        CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_LOGICAL & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        CommonField_p->ACK |= ((U32)(0x5 & STBLIT_ACK_ALU_ROP_MODE_MASK) << STBLIT_ACK_ALU_ROP_MODE_SHIFT);
    }
#else /* !STBLIT_USE_COLOR_KEY_FOR_BDISP */
    else if (BlitContext_p->AluMode == STBLIT_ALU_COPY)
    {
        if ( Src2_p!=NULL)
        {
            /* Bypass Src2 */
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }

        if ( Src1_p!=NULL)
        {
            /* Bypass Src1 */
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC1 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
    }
#endif /* STBLIT_USE_COLOR_KEY_FOR_BDISP */
    else
    {
        return ST_ERROR_FEATURE_NOT_SUPPORTED ;
    }
#endif /* defined(ST_7109) || defined(ST_7200) */

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
        SetOperationIOConfiguration(Src1_p, Src2_p, Dst_p, Config_p);
    }

    return(Err);
 }


/*******************************************************************************
Name        : CopyOperationNodeSetup
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
 __inline static ST_ErrorCode_t CopyOperationNodeSetup(stblit_Node_t*                    Node_p,
                                          STBLIT_BlitContext_t*             BlitContext_p,
                                          STGXOBJ_Bitmap_t*                 SrcBitmap_p,
                                          STGXOBJ_Rectangle_t*              SrcRectangle_p,
                                          STGXOBJ_Bitmap_t*                 DstBitmap_p,
                                          S32                               DstPositionX,
                                          S32                               DstPositionY,
                                          stblit_Device_t*                  Device_p,
                                          stblit_CommonField_t*             CommonField_p,
                                          stblit_OperationConfiguration_t*  Config_p,
                                          U8                                ScanConfig,
                                          stblit_Source1Mode_t              OpMode,
                                          BOOL                              LastNode)
 {
     ST_ErrorCode_t   Err = ST_NO_ERROR;
        STBLIT_Source_t                   Src2;
        STBLIT_Destination_t              Dst;
        STGXOBJ_Rectangle_t               DstRectangle;

    /* Initialize Node */
    InitNode(Node_p, BlitContext_p);

    /* Initialize interruption stuff*/
    SetNodeIT (Node_p, Device_p, BlitContext_p, CommonField_p, LastNode);

    /* Set Src2  */
    SetNodeSrc2Bitmap(Node_p,SrcBitmap_p,SrcRectangle_p,ScanConfig,Device_p,CommonField_p,0,0,0,0);

    /* Set ALU mode bypass Src2 (copy)*/
    CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);

#if defined(ST_7109) || defined(ST_7200) || defined(STBLIT_USE_COLOR_KEY_FOR_BDISP)
    /* Set ColorKey */
    if (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE)
    {
        SetNodeColorKey(Node_p, BlitContext_p->ColorKeyCopyMode, &(BlitContext_p->ColorKey),CommonField_p);
    }
#endif /* STBLIT_USE_COLOR_KEY_FOR_BDISP */

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

    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->ConcatMode        = FALSE;
        Config_p->SrcMB             = FALSE;
        Config_p->IdenticalDstSrc1  = FALSE; /* Src1 is either ignored in Normal mode or foreground in case of Mask */
#ifdef ST_OSWINCE
		Config_p->BlendOp           = FALSE;
#endif
        Config_p->OpMode            = OpMode;
        Config_p->XYLMode           = STBLIT_JOB_BLIT_XYL_TYPE_NONE;
        Config_p->ScanConfig        = ScanConfig;

        Src2.Type                   = STBLIT_SOURCE_TYPE_BITMAP;
        Src2.Data.Bitmap_p          = SrcBitmap_p;
        Src2.Rectangle              = *SrcRectangle_p;
        Dst.Bitmap_p                = DstBitmap_p;
        Dst.Rectangle               = DstRectangle;

        SetOperationIOConfiguration(NULL, &Src2, &Dst, Config_p);
    }


     return(Err);
 }

/*******************************************************************************
Name        : FillOperationNodeSetup
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t FillOperationNodeSetup(stblit_Node_t*                    Node_p,
                                          STBLIT_BlitContext_t*             BlitContext_p,
                                          STGXOBJ_Bitmap_t*                 Bitmap_p,
                                          STGXOBJ_Rectangle_t*              Rectangle_p,
                                          STGXOBJ_Color_t*                  Color_p,
                                          stblit_Device_t*                  Device_p,
                                          stblit_CommonField_t*             CommonField_p,
                                          stblit_OperationConfiguration_t*  Config_p,
                                          U8                                ScanConfig,
                                          stblit_Source1Mode_t              OpMode,
                                          BOOL                              IdenticalDstSrc1,
                                          BOOL                              LastNode,
                                          BOOL                              TopTile,
                                          BOOL                              BottomTile,
                                          BOOL                              RightTile,
                                          BOOL                              LeftTile)
 {
     ST_ErrorCode_t   Err = ST_NO_ERROR;
     STBLIT_Source_t                   Src2;
     STBLIT_Destination_t              Dst;


    /* Initialize Node */
    InitNode(Node_p, BlitContext_p);

    /* Set BorderAlpha for Anti-Flutter */
    if (Device_p->AntiFlutterOn == TRUE)
	{
        SetNodeAntiFlutter (Node_p, CommonField_p, TopTile, BottomTile);
	}

    /* Set BorderAlpha for Anti-Aliasing */
    if (Device_p->AntiAliasingOn == TRUE)
	{
        SetNodeAntiAliasing (Node_p, CommonField_p, RightTile, LeftTile);
	}

    /* Initialize interruption stuff*/
    SetNodeIT (Node_p, Device_p ,BlitContext_p, CommonField_p, LastNode);

    if ((BlitContext_p->EnableColorCorrection == FALSE) &&
        (BlitContext_p->EnableFlickerFilter == FALSE) )
    {
        if (BlitContext_p->AluMode == STBLIT_ALU_COPY)
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


#if defined(ST_7109) || defined(ST_7200)
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

#else
        /* Set Src2 as a color if no bypass Src1*/
        SetNodeSrc2Color(Node_p,Color_p,Rectangle_p,Device_p,CommonField_p);
#endif /* defined(ST_7109) || defined(ST_7200) */

        /* Set ALU mode */

        if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)
        {
            SetNodeALUBlend(BlitContext_p->GlobalAlpha ,FALSE,CommonField_p);
        }
#if defined(ST_7109) || defined(ST_7200)
        else if (BlitContext_p->AluMode == STBLIT_ALU_COPY)
        {
            /* Set ALU mode bypass Src2 (copy)*/
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
        else
        {
            SetNodeALULogic(BlitContext_p,CommonField_p);
        }

#else
        else
        {
            /* Set ALU mode bypass Src2 (copy)*/
            CommonField_p->ACK |= ((U32)(STBLIT_ACK_ALU_MODE_BYPASS_SRC2 & STBLIT_ACK_ALU_MODE_MASK) << STBLIT_ACK_ALU_MODE_SHIFT);
        }
#endif /* defined(ST_7109) || defined(ST_7200) */
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
#ifdef ST_OSWINCE
		Config_p->BlendOp           = FALSE;
#endif
        Config_p->XYLMode           = STBLIT_JOB_BLIT_XYL_TYPE_NONE;
        Config_p->ScanConfig        = ScanConfig;

        Src2.Type                   = STBLIT_SOURCE_TYPE_COLOR;
        Src2.Data.Color_p           = Color_p;
        Dst.Bitmap_p                = Bitmap_p;
        Dst.Rectangle               = *Rectangle_p;

        SetOperationIOConfiguration(NULL, &Src2, &Dst, Config_p);
    }

     return(Err);
 }



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
#ifdef ST_OSWINCE
	// if Src and Dst completely separated => No Problem */
	if (   SrcXStop < DstXStart || DstXStop < SrcXStart
		|| SrcYStop < DstYStart || DstYStop < SrcYStart)
		return 0xF;
#endif
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
Name        : UpdateNodeSrc1Bitmap
Description : Src1 is a bitmap
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
               Concern always Data1_p  whatever mode (raster, MB)
               Only Data and Pitch updated
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
#if defined (DVD_SECURED_CHIP)
    U8 MemoryStatus;
#endif

    UNUSED_PARAMETER(Device_p);
    /* Reset to Zero Pitch field and update it */
    S1TY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1TY)));
    S1TY &=  ((U32)(~((U32)(STBLIT_S1TY_PITCH_MASK) << STBLIT_S1TY_PITCH_SHIFT)));
    S1TY |=  ((U32)(Bitmap_p->Pitch & STBLIT_S1TY_PITCH_MASK) << STBLIT_S1TY_PITCH_SHIFT);

    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1TY)),S1TY);

    /* Set new Base Address */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
#else
    Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
#if defined (DVD_SECURED_CHIP)
    MemoryStatus = STMES_IsMemorySecure((void *)Tmp, 0, 0);
    if(MemoryStatus == SECURE_REGION)
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1BA)),(STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_S1BA_POINTER_MASK) << STBLIT_S1BA_POINTER_SHIFT)));
    }
    else
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1BA)),((U32)(Tmp & STBLIT_S1BA_POINTER_MASK) << STBLIT_S1BA_POINTER_SHIFT ));
    }
#else
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1BA)),((U32)(Tmp & STBLIT_S1BA_POINTER_MASK) << STBLIT_S1BA_POINTER_SHIFT ));
#endif
    return(Err);
 }



/******************************************************************************
Name        : UpdateNodeSrc2Bitmap
Description : Src1 is a bitmap
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
               Only Data and Pitch updated
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
#if defined (DVD_SECURED_CHIP)
    U8 MemoryStatus;
#endif

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* Reset Pitch field and update it */
    S2TY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)));
    S2TY &= ((U32)(~((U32)(STBLIT_S2TY_PITCH_MASK) << STBLIT_S2TY_PITCH_SHIFT)));
    S2TY |= ((U32)(Bitmap_p->Pitch & STBLIT_S2TY_PITCH_MASK) << STBLIT_S2TY_PITCH_SHIFT);

    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2TY)),S2TY);
    if (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB)
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S3TY)),S2TY);
    }
    /* Set new Base Address*/
    if ( ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE ) || ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM ) )
    {
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data1_p) + Bitmap_p->Offset);
#else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
    }
    else /* (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB) */
    {
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32) ((U32)stblit_CpuToDevice((void*)Bitmap_p->Data2_p) + Bitmap_p->Offset);
#else
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Bitmap_p->Data2_p, &(Device_p->VirtualMapping)) + Bitmap_p->Offset);
#endif
    }
#if defined (DVD_SECURED_CHIP)
        MemoryStatus = STMES_IsMemorySecure((void *)Tmp, 0, 0);
        if(MemoryStatus == SECURE_REGION)
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)), (STBLIT_SECURE_ON | ((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT)));
        }
        else
        {
            STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)), ((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));
        }
#else
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2BA)), ((U32)(Tmp & STBLIT_S2BA_POINTER_MASK) << STBLIT_S2BA_POINTER_SHIFT ));
#endif
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
Name        : UpdateNodeTgtBitmap
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node Not initialized.
               Only Data and Pitch updated
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

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* Reset Pitch field and update it */
    TTY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_TTY)));
    TTY &= ((U32)(~((U32)(STBLIT_TTY_PITCH_MASK) << STBLIT_TTY_PITCH_SHIFT)));
    TTY |= ((U32)(Bitmap_p->Pitch & STBLIT_TTY_PITCH_MASK) << STBLIT_TTY_PITCH_SHIFT);

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
Name        : GetNodeHandleDstPositionXY
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t GetNodeHandleDstPositionXY (stblit_Node_t* Node_p,
                                                             S32*           PositionX_p,
                                                             S32*           PositionY_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             TXY;

    TXY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_TXY)));
    (*PositionX_p)=((TXY >> STBLIT_TXY_X_SHIFT) & STBLIT_TXY_X_MASK);
    (*PositionY_p)=((TXY >> STBLIT_TXY_Y_SHIFT) & STBLIT_TXY_Y_MASK);

    return(Err);
 }

 /*******************************************************************************
Name        : SetNodeHandleDstPositionXY
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeHandleDstPositionXY (stblit_Node_t* Node_p,
                                                             S32           PositionX,
                                                             S32           PositionY)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             TXY=0;

    TXY  = ((U32)((PositionX & STBLIT_TXY_X_MASK) << STBLIT_TXY_X_SHIFT));
    TXY |= ((U32)((PositionY & STBLIT_TXY_Y_MASK) << STBLIT_TXY_Y_SHIFT));
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_TXY)),TXY);

    return(Err);
 }

 /*******************************************************************************
Name        : GetNodeHandleSrc1PositionXY
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t GetNodeHandleSrc1PositionXY (stblit_Node_t* Node_p,
                                                             S32*           PositionX_p,
                                                             S32*           PositionY_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             S1XY;

    S1XY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1XY)));
    (*PositionX_p)=((S1XY >> STBLIT_S1XY_X_SHIFT) & STBLIT_S1XY_X_MASK);
    (*PositionY_p)=((S1XY >> STBLIT_S1XY_Y_SHIFT) & STBLIT_S1XY_Y_MASK);

    return(Err);
 }

 /*******************************************************************************
Name        : SetNodeHandleSrc1PositionXY
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeHandleSrc1PositionXY (stblit_Node_t* Node_p,
                                                             S32           PositionX,
                                                             S32           PositionY)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             S1XY=0;

    S1XY  = ((U32)((PositionX & STBLIT_S1XY_X_MASK) << STBLIT_S1XY_X_SHIFT));
    S1XY |= ((U32)((PositionY & STBLIT_S1XY_Y_MASK) << STBLIT_S1XY_Y_SHIFT));
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S1XY)),S1XY);

    return(Err);
 }

 /*******************************************************************************
Name        : GetNodeHandleSrc2PositionXY
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t GetNodeHandleSrc2PositionXY (stblit_Node_t* Node_p,
                                                             S32*           PositionX_p,
                                                             S32*           PositionY_p)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             S2XY;

    S2XY = STSYS_ReadRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2XY)));
    (*PositionX_p)=((S2XY >> STBLIT_S2XY_X_SHIFT) & STBLIT_S2XY_X_MASK);
    (*PositionY_p)=((S2XY >> STBLIT_S2XY_Y_SHIFT) & STBLIT_S2XY_Y_MASK);

    return(Err);
 }

 /*******************************************************************************
Name        : SetNodeHandleSrc2PositionXY
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeHandleSrc2PositionXY (stblit_Node_t* Node_p,
                                                             S32           PositionX,
                                                             S32           PositionY)
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             S2XY=0;

    S2XY  = ((U32)((PositionX & STBLIT_S2XY_X_MASK) << STBLIT_S2XY_X_SHIFT));
    S2XY |= ((U32)((PositionY & STBLIT_S2XY_Y_MASK) << STBLIT_S2XY_Y_SHIFT));
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2XY)),S2XY);

    return(Err);
 }

#ifdef ST_OSWINCE
/*******************************************************************************
Name        : SetNodeHandleSrc3PositionXY
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
#ifdef ST_OSLINUX
 static ST_ErrorCode_t SetNodeHandleSrc3PositionXY (stblit_Node_t* Node_p,
                                                    S32           PositionX,
                                                    S32           PositionY)

#else
  __inline static ST_ErrorCode_t SetNodeHandleSrc3PositionXY (stblit_Node_t* Node_p,
                                                             S32           PositionX,
                                                             S32           PositionY)
#endif
 {
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             S3XY=0;

    S3XY  = ((U32)((PositionX & STBLIT_S3XY_X_MASK) << STBLIT_S3XY_X_SHIFT));
    S3XY |= ((U32)((PositionY & STBLIT_S3XY_Y_MASK) << STBLIT_S3XY_Y_SHIFT));
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S3XY)),S3XY);

    return Err;
 }


 /*******************************************************************************
Name        : SetNodeHandleSrc2WindowSize
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeHandleSrc2WindowSize (stblit_Node_t* Node_p,
                                                            S32            Width,
                                                            S32            Height)
{
    U32 S2SZ = 0;

	S2SZ =  ((U32)(Width  & STBLIT_S2SZ_WIDTH_MASK)  << STBLIT_S2SZ_WIDTH_SHIFT);
    S2SZ |= ((U32)(Height & STBLIT_S2SZ_HEIGHT_MASK) << STBLIT_S2SZ_HEIGHT_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S2SZ)),S2SZ);

    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : SetNodeHandleSrc3WindowSize
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeHandleSrc3WindowSize (stblit_Node_t* Node_p,
                                                            S32            Width,
                                                            S32            Height)
{
    U32 S3SZ = 0;

	S3SZ =  ((U32)(Width  & STBLIT_S3SZ_WIDTH_MASK)  << STBLIT_S3SZ_WIDTH_SHIFT);
    S3SZ |= ((U32)(Height & STBLIT_S3SZ_HEIGHT_MASK) << STBLIT_S3SZ_HEIGHT_SHIFT);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_S3SZ)),S3SZ);

    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : SetNodeHandleSrc1DestWindowSize
Description :
Parameters  :
Assumptions :  All parameters checks done at upper level. Node not initialized
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetNodeHandleSrc1DestWindowSize (stblit_Node_t* Node_p,
                                                                S32            Width,
                                                                S32            Height)
{
	U32 S1SZ;

	S1SZ =  ((U32)(Width  & STBLIT_T_S1_SZ_WIDTH_MASK ) << STBLIT_T_S1_SZ_WIDTH_SHIFT )
		  | ((U32)(Height & STBLIT_T_S1_SZ_HEIGHT_MASK) << STBLIT_T_S1_SZ_HEIGHT_SHIFT);
	STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_T_S1_SZ)),S1SZ);

    return ST_NO_ERROR;
}

#endif

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

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        UpdateNodeTgtBitmap(Node_p,Bitmap_p,Device_p);

        NodeHandle =  Node_p->PreviousNode;
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

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        UpdateNodeSrc1Bitmap(Node_p,Bitmap_p,Device_p);

        NodeHandle =  Node_p->PreviousNode;
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
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;
    U32                 i;

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        UpdateNodeSrc2Bitmap(Node_p,Bitmap_p,Device_p);

        NodeHandle =  Node_p->PreviousNode;
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

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        UpdateNodeSrc2Color(Node_p,Color_p);

        NodeHandle =  Node_p->PreviousNode;
    }

    return(Err);
}

#ifdef ST_OSWINCE
/*******************************************************************************
Name        : stblit_UpdateDestinationRectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t stblit_UpdateDestinationRectangle(stblit_JobBlit_t*     JBlit_p,
                                                 STGXOBJ_Rectangle_t*  Rectangle_p)
{
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;

    NodeHandle = JBlit_p->FirstNodeHandle;
    Node_p = (stblit_Node_t*)NodeHandle;

    SetNodeHandleDstPositionXY(Node_p, Rectangle_p->PositionX, Rectangle_p->PositionY);
	SetNodeHandleSrc1DestWindowSize(Node_p, Rectangle_p->Width, Rectangle_p->Height);

    return ST_NO_ERROR;
}

#else
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
    S32 PositionX,PositionY,PositionX0,PositionY0;

    NodeHandle = JBlit_p->FirstNodeHandle;
    Node_p = (stblit_Node_t*)NodeHandle;
    GetNodeHandleDstPositionXY(Node_p,&PositionX0,&PositionY0);

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        GetNodeHandleDstPositionXY(Node_p,&PositionX,&PositionY);
        SetNodeHandleDstPositionXY(Node_p,(PositionX-PositionX0+Rectangle_p->PositionX),(PositionY-PositionY0+Rectangle_p->PositionY));

        NodeHandle =  Node_p->PreviousNode;
    }

    return(Err);
}
#endif /* ST_OSWINCE*/


#ifdef ST_OSWINCE
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
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;

    NodeHandle = JBlit_p->FirstNodeHandle;
    Node_p = (stblit_Node_t*)NodeHandle;

    SetNodeHandleSrc1PositionXY(Node_p, Rectangle_p->PositionX, Rectangle_p->PositionY);
    SetNodeHandleSrc1DestWindowSize(Node_p, Rectangle_p->Width, Rectangle_p->Height);

    return ST_NO_ERROR;
}

#else

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
    S32 PositionX,PositionY,PositionX0,PositionY0;

    NodeHandle = JBlit_p->FirstNodeHandle;
    Node_p = (stblit_Node_t*)NodeHandle;
    GetNodeHandleSrc1PositionXY(Node_p,&PositionX0,&PositionY0);

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        GetNodeHandleSrc1PositionXY(Node_p,&PositionX,&PositionY);
        SetNodeHandleSrc1PositionXY(Node_p,(PositionX-PositionX0+Rectangle_p->PositionX),(PositionY-PositionY0+Rectangle_p->PositionY));

        NodeHandle =  Node_p->PreviousNode;
    }

    return(Err);
}
#endif /* ST_OSWINCE */

#ifdef ST_OSWINCE
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
                                                BOOL isMB,
                                                STGXOBJ_Rectangle_t*  Rectangle_p,
                                                STGXOBJ_Rectangle_t*  RectangleChroma_p)
{
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;

    NodeHandle = JBlit_p->FirstNodeHandle;
    Node_p = (stblit_Node_t*)NodeHandle;

#ifdef ST_OSWINCE
    if (isMB)
    {
        // src2 is Chroma
        SetNodeHandleSrc2PositionXY(Node_p, RectangleChroma_p->PositionX, RectangleChroma_p->PositionY);
        SetNodeHandleSrc2WindowSize(Node_p, RectangleChroma_p->Width, RectangleChroma_p->Height);
        // src3 is Luma
        SetNodeHandleSrc3PositionXY(Node_p, Rectangle_p->PositionX, Rectangle_p->PositionY);
        SetNodeHandleSrc3WindowSize(Node_p, Rectangle_p->Width, Rectangle_p->Height);
    }
    else
#endif
    {
        SetNodeHandleSrc2PositionXY(Node_p, Rectangle_p->PositionX, Rectangle_p->PositionY);
        SetNodeHandleSrc2WindowSize(Node_p, Rectangle_p->Width, Rectangle_p->Height);
    }

    return ST_NO_ERROR;
}

#else
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
                                                BOOL isMB,
                                                STGXOBJ_Rectangle_t*  Rectangle_p,
                                                STGXOBJ_Rectangle_t*  RectangleChroma_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;
    U32                 i;
    S32 PositionX,PositionY,PositionX0,PositionY0;

	/* To remove comipation warnning */
    UNUSED_PARAMETER(isMB);
    UNUSED_PARAMETER(RectangleChroma_p);

    NodeHandle = JBlit_p->FirstNodeHandle;
    Node_p = (stblit_Node_t*)NodeHandle;
    GetNodeHandleSrc2PositionXY(Node_p,&PositionX0,&PositionY0);

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        GetNodeHandleSrc2PositionXY(Node_p,&PositionX,&PositionY);
        SetNodeHandleSrc2PositionXY(Node_p,(PositionX-PositionX0+Rectangle_p->PositionX),(PositionY-PositionY0+Rectangle_p->PositionY));

        NodeHandle = Node_p->PreviousNode;
    }

    return(Err);
}
#endif /* ST_OSWINCE */

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
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;
    U32                 i;

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        UpdateNodeClipping(Node_p,ClipRectangle_p,WriteInsideClipRectangle);

        NodeHandle =  Node_p->PreviousNode;
    }

    return(Err);
}



/*
 * Resize Routines
 */

/*******************************************************************************
Name        : SetS1DstTileWidth
Description : Calculate the destination tile width
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetS1DstTileWidth(U32  HScalingFactor,
                                                   U32  SrcWidth,
                                                   U32* S1DstTileWidth)
 {
    ST_ErrorCode_t          Err = ST_NO_ERROR;

    if (HScalingFactor==1024)
    {
        (*S1DstTileWidth)=SrcWidth;
    }
    else                            /* Resize */
    {
        (*S1DstTileWidth)=(((SrcWidth - HSRC_NB_LINE_REPEATED_LEFT_BORDER - HSRC_NB_REAL_LINE_LEFT_BORDER )
                            * 1024/HScalingFactor) + 1);
    }

    return(Err);
 }

/*******************************************************************************
Name        : SetChromaTileWidth
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetChromaTileWidth(U32 HScalingFactor,
                                                    U32  S1DstTileWidth,
                                                    U32* SrcWidth)
 {
    ST_ErrorCode_t          Err = ST_NO_ERROR;

    (*SrcWidth) = ( ( (S1DstTileWidth-1)* HScalingFactor/1024 ) + HSRC_NB_LINE_REPEATED_LEFT_BORDER + HSRC_NB_REAL_LINE_LEFT_BORDER );

    return(Err);
 }


/*******************************************************************************
Name        : SetS1DstTileHeight
Description : Calculate the destination tile Height
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetS1DstTileHeight(U32 VScalingFactor,
                                                    U32 SrcHeight,
                                                    U32* S1DstTileHeight)
 {
        ST_ErrorCode_t          Err = ST_NO_ERROR;

    if (VScalingFactor==1024)
    {
        (*S1DstTileHeight)=SrcHeight;
    }
    else                            /* Resize */
    {
        (*S1DstTileHeight)=(((SrcHeight - VSRC_NB_LINE_REPEATED_TOP_BORDER - VSRC_NB_REAL_LINE_TOP_BORDER )
                            * 1024/VScalingFactor) + 1);
    }

    return(Err);

 }

/*******************************************************************************
Name        : SetChromaTileHeight
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static ST_ErrorCode_t SetChromaTileHeight(U32 VScalingFactor,
                                                    U32  S1DstTileHeight,
                                                    U32* SrcHeight)
 {
    ST_ErrorCode_t          Err = ST_NO_ERROR;

    (*SrcHeight) = ( ( (S1DstTileHeight-1)* VScalingFactor/1024 ) + VSRC_NB_LINE_REPEATED_TOP_BORDER + VSRC_NB_REAL_LINE_TOP_BORDER );

    return(Err);
 }


/*******************************************************************************
Name        : GetHSRCInitPhase
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static U32  GetHSRCInitPhase(U32 HScalingFactor,
                                        U32 InitPhase,
                                        S32 TileDstPositionX,
                                        S32 DstPositionX)
 {
    return ( (((TileDstPositionX-DstPositionX)*HScalingFactor)+InitPhase) & 0x3FF );
 }


/*******************************************************************************
Name        : GetVSRCInitPhase
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static U32  GetVSRCInitPhase(U32 VScalingFactor,
                                        U32 InitPhase,
                                        S32 TileDstPositionY,
                                        S32 DstPositionY)
 {
    return ( (((TileDstPositionY-DstPositionY)*VScalingFactor)+InitPhase) & 0x3FF );
 }


/*******************************************************************************
Name        : GetTile_Src2PositionXLeft_And_HNbRepeat
Description : Calculate the destination tile width
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static void GetTile_Src2PositionXLeft_And_HNbRepeat(U32 HScalingFactor,
                                                               S32 DstPositionX,
                                                               S32 TileDstPositionX,
                                                               S32 SrcPositionX,
                                                               U32 InitPhase,
                                                               S32* TileSrc2PositionX_p,
                                                               U32* HNbRepeat_p)
 {
    if (HScalingFactor==1024)
    {
        (*TileSrc2PositionX_p) = (TileDstPositionX - DstPositionX) + SrcPositionX;
    }
    else
    {
        (*TileSrc2PositionX_p)= (((TileDstPositionX - DstPositionX )*HScalingFactor + InitPhase) / 1024)
                               + SrcPositionX - HSRC_NB_LINE_REPEATED_LEFT_BORDER;
        if ((*TileSrc2PositionX_p)<0)
        {
            (*HNbRepeat_p) = SrcPositionX - (*TileSrc2PositionX_p);
            (*TileSrc2PositionX_p) = SrcPositionX;
        }
        else
        {
            (*HNbRepeat_p) = 0;
        }
    }
 }



/*******************************************************************************
Name        : GetTile_Src2PositionYTop_And_VNbRepeat
Description : Calculate the destination tile width
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
__inline static void GetTile_Src2PositionYTop_And_VNbRepeat(U32 VScalingFactor,
                                                              S32 DstPositionY,
                                                              S32 TileDstPositionY,
                                                              S32 SrcPositionY,
                                                              U32 InitPhase,
                                                              S32 NbLigneCorrection,
                                                              S32* TileSrc2PositionY_p,
                                                              U32* VNbRepeat_p)
 {

    if (VScalingFactor==1024)
    {
        (*TileSrc2PositionY_p)=(TileDstPositionY-DstPositionY)+SrcPositionY;
    }
    else
    {
        (*TileSrc2PositionY_p)=(((TileDstPositionY-DstPositionY)*VScalingFactor + InitPhase)/1024)
                               + SrcPositionY - VSRC_NB_LINE_REPEATED_TOP_BORDER - NbLigneCorrection;
        if ((*TileSrc2PositionY_p)<0)
        {
            (*VNbRepeat_p)=SrcPositionY-(*TileSrc2PositionY_p);
            (*TileSrc2PositionY_p)=SrcPositionY;
        }
        else
        {
            (*VNbRepeat_p)=0;
        }
    }
 }

#ifdef ST_OSWINCE

/*******************************************************************************
Name        : stblit_ComputeFilters
Description : Compute the register values from the filter coefficients
Parameters  :
Assumptions : All parameters are checked at upper level :
              => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
void stblit_ComputeFilters(U32 HScalingFactor, U32 HSRC_InitPhase, U32 HNbRepeat, U32 VScalingFactor, U32 VSRC_InitPhase, U32 VNbRepeat,
                           U32* RZI, U32* RSF, U32* HOffset, U32* VOffset)
{
    *RZI = 0;

	*RZI |= ((U32)(HSRC_InitPhase & STBLIT_RZI_INITIAL_HSRC_MASK) << STBLIT_RZI_INITIAL_HSRC_SHIFT);
    *RZI |= ((U32)(HNbRepeat & STBLIT_RZI_HNB_REPEAT_MASK) << STBLIT_RZI_HNB_REPEAT_SHIFT);
    SelectHFilter(HScalingFactor, HOffset);

    *RZI |= ((U32)(VSRC_InitPhase & STBLIT_RZI_INITIAL_VSRC_MASK) << STBLIT_RZI_INITIAL_VSRC_SHIFT);
    *RZI |= ((U32)(VNbRepeat      & STBLIT_RZI_VNB_REPEAT_MASK) << STBLIT_RZI_VNB_REPEAT_SHIFT);
	SelectVFilter(VScalingFactor, VOffset);

    *RSF |= ((U32)(HScalingFactor & STBLIT_RSF_INCREMENT_HSRC_MASK) << STBLIT_RSF_INCREMENT_HSRC_SHIFT);
    *RSF |= ((U32)(VScalingFactor & STBLIT_RSF_INCREMENT_VSRC_MASK) << STBLIT_RSF_INCREMENT_VSRC_SHIFT);
}

 /*******************************************************************************
Name        : stblit_UpdateFilters
Description : Update the filter set-up in nodes for a given job Blit.
Parameters  :
Assumptions : All parameters are checked at upper level :
              => all pointers in parameters are not NULL !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_UpdateFilters(stblit_Device_t* Device_p, stblit_JobBlit_t* JBlit_p, STGXOBJ_Rectangle_t* DestRectangle_p,
                                    BOOL isMB, STGXOBJ_Rectangle_t* FgndRectangle_p, STGXOBJ_Rectangle_t* FgndRectangleChroma_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    stblit_Node_t*      Node_p;
    stblit_NodeHandle_t NodeHandle;
    U32                 HScalingFactor, HSRC_InitPhase, HNbRepeat, VScalingFactor, VSRC_InitPhase, VNbRepeat;
    U32                 ChromaHScalingFactor, Chroma_HSRC_InitPhase, ChromaHNbRepeat;
    U32                 ChromaVScalingFactor, Chroma_VSRC_InitPhase, ChromaVNbRepeat;
	U32                 RZI = 0, Y_RZI = 0;
	U32					RSF = 0, Y_RSF = 0;
	U32                 HOffset, Y_HOffset;
    U32                 VOffset, Y_VOffset;
	U32                 Tmp;

	// compute filters
	if (FgndRectangle_p->Width == 1 || DestRectangle_p->Width == 1)
		HScalingFactor =((FgndRectangle_p->Width * 1024) / DestRectangle_p->Width);
	else
		HScalingFactor =(((FgndRectangle_p->Width - 1) * 1024) / (DestRectangle_p->Width - 1));

	if (FgndRectangle_p->Height == 1 || DestRectangle_p->Height == 1)
		VScalingFactor =(FgndRectangle_p->Height * 1024) / DestRectangle_p->Height;
	else
		VScalingFactor =((FgndRectangle_p->Height - 1) * 1024) / (DestRectangle_p->Height - 1);

    if (isMB)
    {
        ChromaHScalingFactor = HScalingFactor / 2;
        ChromaVScalingFactor = VScalingFactor / 2;
        FgndRectangleChroma_p->PositionX = FgndRectangle_p->PositionX / 2;
        FgndRectangleChroma_p->PositionY = FgndRectangle_p->PositionY / 2;

        Chroma_HSRC_InitPhase = (FgndRectangle_p->PositionX % 2) * 512;
        Chroma_VSRC_InitPhase = 768 - (FgndRectangle_p->PositionY % 2) * 512;

        GetTile_Src2PositionXLeft_And_HNbRepeat(ChromaHScalingFactor, 0, 0,
                                                FgndRectangleChroma_p->PositionX, 0,
                                                &FgndRectangleChroma_p->PositionX, &ChromaHNbRepeat);
        FgndRectangleChroma_p->Width = (FgndRectangle_p->Width - 1) / 2 + HSRC_NB_LINE_REPEATED_LEFT_BORDER + HSRC_NB_REAL_LINE_LEFT_BORDER;

        GetTile_Src2PositionYTop_And_VNbRepeat(ChromaVScalingFactor, 0,0,
                                               FgndRectangleChroma_p->PositionY , Chroma_VSRC_InitPhase, 1,
                                               &FgndRectangleChroma_p->PositionY, &ChromaVNbRepeat);

        FgndRectangleChroma_p->Height = (FgndRectangle_p->Height - 1) / 2 + VSRC_NB_LINE_REPEATED_TOP_BORDER + VSRC_NB_REAL_LINE_TOP_BORDER;
    }

    HSRC_InitPhase = 0;
	Tmp = FgndRectangle_p->PositionX;
    GetTile_Src2PositionXLeft_And_HNbRepeat(HScalingFactor, 0, 0,
                                            FgndRectangle_p->PositionX, 0, &FgndRectangle_p->PositionX, &HNbRepeat);
	if (Tmp - FgndRectangle_p->PositionX != 0)
		FgndRectangle_p->Width += Tmp - FgndRectangle_p->PositionX;

    VSRC_InitPhase = 0;
	Tmp = FgndRectangle_p->PositionY;
	GetTile_Src2PositionYTop_And_VNbRepeat(VScalingFactor, 0, 0,
                                           FgndRectangle_p->PositionY, 0, 0, &FgndRectangle_p->PositionY, &VNbRepeat);
	if (Tmp - FgndRectangle_p->PositionY != 0)
		FgndRectangle_p->Height += Tmp - FgndRectangle_p->PositionY;

	// compute registers
    if (isMB)
    {
        // chroma filter
        stblit_ComputeFilters(ChromaHScalingFactor, Chroma_HSRC_InitPhase, ChromaHNbRepeat,
                              ChromaVScalingFactor, Chroma_VSRC_InitPhase, ChromaVNbRepeat,
                              &RZI, &RSF, &HOffset, &VOffset);
        // luma filter
        stblit_ComputeFilters(HScalingFactor, HSRC_InitPhase, HNbRepeat,
                              VScalingFactor, VSRC_InitPhase, VNbRepeat,
                              &Y_RZI, &Y_RSF, &Y_HOffset, &Y_VOffset);

    }
    else
    {
        // raster filter
        stblit_ComputeFilters(HScalingFactor, HSRC_InitPhase, HNbRepeat,
                              VScalingFactor, VSRC_InitPhase, VNbRepeat,
                              &RZI, &RSF, &HOffset, &VOffset);
    }

	// Update Registers
    NodeHandle = JBlit_p->FirstNodeHandle;
    Node_p = (stblit_Node_t*)NodeHandle;

    // chroma/raster registers
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RZI)), RZI);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_RSF)), RSF);

    Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->VFilterBuffer_p, &(Device_p->VirtualMapping)) + VOffset);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_VFP)),((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT ));
    Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->HFilterBuffer_p, &(Device_p->VirtualMapping)) + HOffset);
    STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_HFP)),((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT ));

    // luma registers
    if (isMB)
    {
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_RZI)), Y_RZI);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_RSF)), Y_RSF);

        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->VFilterBuffer_p, &(Device_p->VirtualMapping)) + Y_VOffset);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_VFP)),((U32)(Tmp & STBLIT_VFP_POINTER_MASK) << STBLIT_VFP_POINTER_SHIFT ));
        Tmp =  (U32)((U32)STAVMEM_VirtualToDevice((void*)Device_p->HFilterBuffer_p, &(Device_p->VirtualMapping)) + Y_HOffset);
        STSYS_WriteRegMem32LE((void*)(&(Node_p->HWNode.BLT_Y_HFP)),((U32)(Tmp & STBLIT_HFP_POINTER_MASK) << STBLIT_HFP_POINTER_SHIFT ));
    }
    return Err;
}
#endif

/*
 * Main fuctions
 */

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
ST_ErrorCode_t stblit_OnePassBlitOperation( stblit_Device_t*                 Device_p,
                                            STBLIT_Source_t*                 Src1_p,
                                            STBLIT_Source_t*                 Src2_p,
                                            STBLIT_Destination_t*            Dst_p,
                                            STBLIT_BlitContext_t*            BlitContext_p,
                                            U16                              Code,
                                            stblit_NodeHandle_t**            NodeHandleFirst_p,
                                            stblit_NodeHandle_t**            NodeHandleLast_p,
                                            U32*                             NumberNodes_p,
                                            stblit_OperationConfiguration_t* Config_p,
                                            BOOL                             SrcMB)
 {
    ST_ErrorCode_t          Err                 = ST_NO_ERROR;

    stblit_Node_t*          Node_p;
    stblit_Node_t*          Previous_Node_p = NULL ;
    stblit_Node_t*          Next_Node_p  = NULL ;
    stblit_Node_t*          Link_Node_p = NULL ;
    stblit_Node_t*          First_Node_p = NULL ;
    stblit_Node_t*          Last_Node_p = NULL ;

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
    U8                      FilterMode = 0;
    stblit_CommonField_t    CommonField;
    stblit_NodeHandle_t*    NodeHandle_p;
    U32                     HScalingFactor = 0,HSRC_InitPhase = 0,HNbRepeat = 0,Chroma_HSRC_InitPhase = 0,ChromaHNbRepeat = 0,ChromaHScalingFactor = 0,ChromaWidth = 0,
                            VScalingFactor,VSRC_InitPhase,VNbRepeat = 0,Chroma_VSRC_InitPhase = 0,ChromaVNbRepeat = 0,ChromaVScalingFactor = 0,ChromaHeight = 0;
    S32                     ChromaPositionX = 0,ChromaPositionY = 0,LumaXOffset,ChromaXOffset = 0,LumaYOffset,ChromaYOffset,Tmp;
    U32                     VLumaInitPhase = 0;                 /* Vertical Luma initial phase at the overlay top border */
    S32                     LumaNbLineRepeatCorrection;     /* Number of additional Luma lines to repeat at the overlay top border  */
    U32                     VChromaInitPhase = 0;               /* Vertical Chroma initial phase at the overlay top border */
    S32                     ChromaNbLineRepeatCorrection = 0;   /* Number of additional Chroma lines to repeat at the overlay top border  */
    S32                     ChromaSrcPosX = 0,ChromaSrcPosY = 0;

/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
    U32                     HorizIndexDisplay,VertIndexDisplay;
#endif
/*---------------------*/
    STBLIT_Source_t         TileSrc1,TileSrc2;
    STBLIT_Destination_t    TileDst;
    BOOL                    FirstHorizTile,LastHorizTile,FirstVertTile,LastVertTile;
    BOOL                    TopTile, BottomTile, RightTile, LeftTile;
    U32                     MaxSrcWidth;
    BOOL                    OverlapActivatedOnS1 = FALSE, OverlapActivatedOnS2 = FALSE;
	U32                     TempAddress;

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
                    if (Src1Code != 0xF)
                        OverlapActivatedOnS1 = TRUE;
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
                    if (Src2Code != 0xF)
                        OverlapActivatedOnS2 = TRUE;
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

    /* Decode U16 code :
     *  ________________________________________________________________________________
     * |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
     * | 1  |  1 |    |    |    |    | 1  | 1  # 1  | 1  | 1  | 1  # 1  | 1  | 1  |  1 |
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


    /*-----------------7/7/2004 8:59AM------------------
     *
     *  Analyzing data and spliting a blit operation
     *  in several sub-operations
     *
     * --------------------------------------------------*/
    (*NumberNodes_p)=0;

    /*-------- Calculating Horizontal And Vertical Scaling Factor --------*/
    if (Src2_p!=NULL)
    {
        if ( ( Src2_p->Rectangle.Width == 1 ) || ( Dst_p->Rectangle.Width == 1 ) )
        {
            HScalingFactor =( ( Src2_p->Rectangle.Width * 1024 ) /  Dst_p->Rectangle.Width );
        }
        else
        {
            HScalingFactor =( ( (Src2_p->Rectangle.Width-1) * 1024 ) /  (Dst_p->Rectangle.Width-1) );
        }

        if ( ( Src2_p->Rectangle.Height == 1 ) || ( Dst_p->Rectangle.Height == 1 ) )
        {
            VScalingFactor =( ( Src2_p->Rectangle.Height * 1024 ) / Dst_p->Rectangle.Height );
        }
        else
        {
            VScalingFactor =( ( (Src2_p->Rectangle.Height-1) * 1024 ) / (Dst_p->Rectangle.Height-1) );
        }
    }
    else
    {
        HScalingFactor=1024;
        VScalingFactor=1024;
    }

    if ( SrcMB )
    {
        ChromaHScalingFactor = HScalingFactor/2;
        ChromaVScalingFactor = VScalingFactor/2;
    }

    /*-------- Sources and destination global data Initialisation ---------*/
    if ( Src1_p!=NULL)
    {
        TileSrc1.Type=Src1_p->Type;
        TileSrc1.Data=Src1_p->Data;
        TileSrc1.Palette_p=Src1_p->Palette_p;
    }

    if ( Src2_p!=NULL)
    {
        TileSrc2.Type=Src2_p->Type;
        TileSrc2.Data=Src2_p->Data;
        TileSrc2.Palette_p=Src2_p->Palette_p;
    }

    TileDst.Bitmap_p=Dst_p->Bitmap_p;
    TileDst.Palette_p=Dst_p->Palette_p;


    /*-------- Horizantal Sources and destination Initialisation ---------*/
    if ( Src1_p!=NULL)
    {
        TileSrc1.Rectangle.PositionX=Src1_p->Rectangle.PositionX;
    }
    TileDst.Rectangle.PositionX=Dst_p->Rectangle.PositionX;

    /* Set MaxSrcWidth */
#if defined(ST_7109) || defined(ST_7200)
    if (( VScalingFactor == 1024 ) && ( !SrcMB ))
    {
        if (BlitContext_p->EnableFlickerFilter == TRUE)
        {
            MaxSrcWidth = STBLIT_MAX_SRC_WIDTH; /* No resize but AF*/
        }
        else
        {
            MaxSrcWidth = STBLIT_MAX_SIZE; /* No resize */
        }


    }
    else
    {
        MaxSrcWidth = STBLIT_MAX_SRC_WIDTH;  /* Resize */
    }
#else
    MaxSrcWidth = STBLIT_MAX_SRC_WIDTH;
#endif /*defined(ST_7109) || defined(ST_7200)*/


    FirstHorizTile = TRUE;
    LastHorizTile  = FALSE;
    LeftTile       = TRUE;
    RightTile      = FALSE;

    TileSrc2.Rectangle.Width = MaxSrcWidth;
    SetS1DstTileWidth(HScalingFactor,TileSrc2.Rectangle.Width,&TileDst.Rectangle.Width);
    TileSrc1.Rectangle.Width=TileDst.Rectangle.Width;

    if ( SrcMB )
    {
        ChromaSrcPosX   = Src2_p->Rectangle.PositionX / 2;
        ChromaSrcPosY   = Src2_p->Rectangle.PositionY / 2;  /* Always Frame indexing */

        /* Retrieve Horizontal Phase correction according to PanScan YOffset for Luma & chroma.
            Offsets have got a 1/1024 granularity */
        ChromaXOffset = ((Src2_p->Rectangle.PositionX % 2) * 512);
        LumaXOffset   = 0;

        /* Retrieve Vertical Phase correction according to PanScan YOffset for Luma & chroma.
               Offsets have got a 1/1024 granularity */
        ChromaYOffset = ((Src2_p->Rectangle.PositionY % 2) * 512);
        LumaYOffset   = 0;

        /* Horizontal NbRepeat correction  => Always 0 for Luma & Chroma  */

        /* Luma related */
        VLumaInitPhase               = LumaYOffset;
        LumaNbLineRepeatCorrection   = 0;

        /* Chroma related */
        Tmp                          = 768 + ChromaYOffset;
        VChromaInitPhase             = Tmp & 0x3FF;
        ChromaNbLineRepeatCorrection = 1 - (Tmp >> 10);
    }
    else
    {
        LumaXOffset   = 0;
        VLumaInitPhase = 0;
        LumaNbLineRepeatCorrection = 0;
   }


/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
    HorizIndexDisplay=0;
#endif
/*---------------------*/

    /*-------- Horizontal Scaling ---------*/
    do
    {
        /*-------- Upedating Src1 and Dst Width ---------*/
        if ( (TileDst.Rectangle.PositionX + TileDst.Rectangle.Width-1) >= (Dst_p->Rectangle.PositionX + Dst_p->Rectangle.Width-1) )
        {
            TileDst.Rectangle.Width=(Dst_p->Rectangle.Width+Dst_p->Rectangle.PositionX)-TileDst.Rectangle.PositionX;
            TileSrc1.Rectangle.Width=TileDst.Rectangle.Width;
            LastHorizTile = TRUE;
            RightTile     = TRUE;
        }

        /*-------- Calculating Initial_Phase, HNbRepeat and Src2PositionXLeft ---------*/
        HSRC_InitPhase = GetHSRCInitPhase(HScalingFactor,LumaXOffset,TileDst.Rectangle.PositionX,Dst_p->Rectangle.PositionX);
        if ( Src2_p!=NULL)
        {
            GetTile_Src2PositionXLeft_And_HNbRepeat(HScalingFactor,Dst_p->Rectangle.PositionX,TileDst.Rectangle.PositionX,
                                                    Src2_p->Rectangle.PositionX,LumaXOffset,&TileSrc2.Rectangle.PositionX,&HNbRepeat);
        }

        /*-------- Upedating Src2 and Dst Width ---------*/
        if ( LastHorizTile && (Src2_p!=NULL) )
        {
            TileSrc2.Rectangle.Width=(Src2_p->Rectangle.Width+Src2_p->Rectangle.PositionX)-TileSrc2.Rectangle.PositionX;
        }

        if ( SrcMB )
        {
            Chroma_HSRC_InitPhase = GetHSRCInitPhase(ChromaHScalingFactor,ChromaXOffset,TileDst.Rectangle.PositionX,Dst_p->Rectangle.PositionX);
            GetTile_Src2PositionXLeft_And_HNbRepeat(ChromaHScalingFactor,Dst_p->Rectangle.PositionX,TileDst.Rectangle.PositionX,
                                                    ChromaSrcPosX,ChromaXOffset,&ChromaPositionX,&ChromaHNbRepeat);
            SetChromaTileWidth(ChromaHScalingFactor,TileDst.Rectangle.Width,&ChromaWidth);
        }

        /*-------- Process particular cas where SRC_Witdh=1 ---------*/
        if (Src2_p!=NULL)
        {
            if (( Src2_p->Rectangle.Width == 1 ) )
            {
                TileSrc2.Rectangle.PositionX = Src2_p->Rectangle.PositionX;
                TileSrc2.Rectangle.Width = Src2_p->Rectangle.Width;
                TileDst.Rectangle.PositionX = Dst_p->Rectangle.PositionX;
                TileDst.Rectangle.Width = Dst_p->Rectangle.Width;
            }
        }
        /*-------- Vertical Sources and destination Initialisation ---------*/
        if ( Src1_p!=NULL)
        {
            TileSrc1.Rectangle.PositionY=Src1_p->Rectangle.PositionY;
        }
        TileDst.Rectangle.PositionY=Dst_p->Rectangle.PositionY;

        FirstVertTile = TRUE;
        LastVertTile  = FALSE;
        TopTile       = TRUE;
        BottomTile    = FALSE;
        TileSrc2.Rectangle.Height=STBLIT_MAX_SRC_HEIGHT;
        SetS1DstTileHeight(VScalingFactor,TileSrc2.Rectangle.Height,&TileDst.Rectangle.Height);
        TileSrc1.Rectangle.Height=TileDst.Rectangle.Height;
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
        VertIndexDisplay=0;
#endif
/*---------------------*/

        /*-------- Vertical Scaling ---------*/
        do
        {
            /*-------- Upedating Src1 and Dst Height ---------*/
            if ( (TileDst.Rectangle.PositionY+TileDst.Rectangle.Height-1)>=(Dst_p->Rectangle.PositionY+Dst_p->Rectangle.Height-1) )
            {
                TileDst.Rectangle.Height=(Dst_p->Rectangle.Height+Dst_p->Rectangle.PositionY)-TileDst.Rectangle.PositionY;
                TileSrc1.Rectangle.Height=TileDst.Rectangle.Height;
                LastVertTile = TRUE;
                BottomTile   = TRUE;
            }

            /*-------- Calculating Initial_Phase, HNbRepeat and Src2PositionXLeft ---------*/
            VSRC_InitPhase = GetVSRCInitPhase(VScalingFactor,VLumaInitPhase,TileDst.Rectangle.PositionY,Dst_p->Rectangle.PositionY);
            if ( Src2_p!=NULL)
            {
                GetTile_Src2PositionYTop_And_VNbRepeat(VScalingFactor,Dst_p->Rectangle.PositionY,TileDst.Rectangle.PositionY,
                                                        Src2_p->Rectangle.PositionY,VLumaInitPhase,LumaNbLineRepeatCorrection,&TileSrc2.Rectangle.PositionY,&VNbRepeat);
            }

            /*-------- Upedating Src2 and Dst Height ---------*/
            if ( LastVertTile && (Src2_p!=NULL) )
            {
                TileSrc2.Rectangle.Height=(Src2_p->Rectangle.Height+Src2_p->Rectangle.PositionY)-TileSrc2.Rectangle.PositionY;
            }

            if ( SrcMB )
            {
                Chroma_VSRC_InitPhase = GetVSRCInitPhase(ChromaVScalingFactor,VChromaInitPhase,TileDst.Rectangle.PositionY,Dst_p->Rectangle.PositionY);
                GetTile_Src2PositionYTop_And_VNbRepeat(ChromaVScalingFactor,Dst_p->Rectangle.PositionY,TileDst.Rectangle.PositionY,
                                                        ChromaSrcPosY,VChromaInitPhase,ChromaNbLineRepeatCorrection,&ChromaPositionY,&ChromaVNbRepeat);
                SetChromaTileHeight(ChromaVScalingFactor,TileDst.Rectangle.Height,&ChromaHeight);
            }

            /*-------- Process particular cas where SRC_Height=1 ---------*/
            if (Src2_p != NULL)
            {
                if ( Src2_p->Rectangle.Height == 1 )
                {
                    TileSrc2.Rectangle.PositionY = Src2_p->Rectangle.PositionY;
                    TileSrc2.Rectangle.Height = Src2_p->Rectangle.Height;
                    TileDst.Rectangle.PositionY = Dst_p->Rectangle.PositionY;
                    TileDst.Rectangle.Height = Dst_p->Rectangle.Height;
                }
            }

            /*---------------------- Creating node -----------------------*/
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
if ( (FirstHorizTile&&FirstVertTile) ||
     (LastHorizTile &&LastVertTile) ||
     ( (HorizIndexDisplay%2)==0 && (VertIndexDisplay%2)==0 ) ||
     ( (HorizIndexDisplay%2)!=0 && (VertIndexDisplay%2)!=0 ) )
{
#endif
/*---------------------*/

			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&NodeHandle_p;

			/* Get Node descriptor handle*/
            if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
            {
                Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) TempAddress);
                if (Err != ST_NO_ERROR)
                {
                    /* Release already allocated nodes */
                    if(Previous_Node_p)
                    {
                       stblit_ReleaseListNode(Device_p, (stblit_NodeHandle_t)Previous_Node_p);
                    }
                    return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
                }
            }
            else  /* Blit in Job */
            {
                Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
                if (Err != ST_NO_ERROR)
                {
                    /* Release already allocated nodes */
                    if(Previous_Node_p)
                    {
                       stblit_ReleaseListNode(Device_p, (stblit_NodeHandle_t)Previous_Node_p);
                    }

                    return(STBLIT_ERROR_MAX_JOB_NODE);
                }
            }

			Node_p  = (stblit_Node_t*)(NodeHandle_p);

            memset(&CommonField, 0, sizeof(stblit_CommonField_t));

            BlitOperationNodeSetup(Node_p, BlitContext_p, (Src1_p!=NULL ? &TileSrc1 : NULL), (Src2_p!=NULL ? &TileSrc2 : NULL), &TileDst,
                    Device_p, &CommonField, Config_p, ScanConfig, ClutMode, InConfig, OutConfig, OutEnable, InEnable, ClutEnable, FilterMode,SrcMB,
                    HScalingFactor,HNbRepeat,HSRC_InitPhase,VScalingFactor,VNbRepeat,VSRC_InitPhase,
                    Chroma_HSRC_InitPhase,ChromaHNbRepeat,ChromaHScalingFactor,ChromaWidth,ChromaPositionX,
                    Chroma_VSRC_InitPhase,ChromaVNbRepeat,ChromaVScalingFactor,ChromaHeight,ChromaPositionY, (LastHorizTile && LastVertTile), Dst_p->Rectangle.Height,
                    TopTile, BottomTile, RightTile, LeftTile);

            stblit_WriteCommonFieldInNode(Device_p, Node_p ,&CommonField);

            (*NumberNodes_p)++;

            if((OverlapActivatedOnS1 == TRUE) || (OverlapActivatedOnS2 == TRUE))
            {
                STGXOBJ_Rectangle_t  OverlapRectangle;

                if (OverlapActivatedOnS1 == TRUE)
                    OverlapRectangle = Src1_p->Rectangle;
                else
                    OverlapRectangle = Src2_p->Rectangle;

                /* ScanConfig gives the position of source compared with destination */
                switch(ScanConfig)
                {
                    /* Case 0:Down right, 2:Up right */
                    case 0:
                    case 2:
                        if (OverlapRectangle.PositionX < Dst_p->Rectangle.PositionY)
                        {
                            /* The first tile to be copied must be in the right-up side */
                            if ( FirstVertTile )
                            {
                                First_Node_p = Node_p;
                            }
                            else
                            {
                                stblit_Connect2Nodes(Device_p, Previous_Node_p, Node_p);
                            }
                            Previous_Node_p = Node_p;

                            /*first node*/
                            if ( LastHorizTile && FirstVertTile )
                            {
                                (*NodeHandleFirst_p)=(stblit_NodeHandle_t* )Node_p;
                            }

                            /*last node*/
                            if ( FirstHorizTile && LastVertTile)
                            {
                                (*NodeHandleLast_p)=(stblit_NodeHandle_t* )Node_p;
                                SetNodeIT (Node_p, Device_p ,BlitContext_p, &CommonField, TRUE);
                            }

                            if ( LastHorizTile && LastVertTile)
                            {
                                SetNodeIT (Node_p, Device_p ,BlitContext_p, &CommonField, FALSE);
                            }

                            if ( LastVertTile && !FirstHorizTile)
                            {
                                stblit_Connect2Nodes(Device_p, Node_p, Link_Node_p );
                                Link_Node_p = First_Node_p;
                            }

                            if ( FirstVertTile && FirstHorizTile)
                            {
                                Link_Node_p = First_Node_p;
                            }
                        }  /*end code*/
                        else
                        {
                            /* The first tile to be copied must be in the left-up side */
                            if ( FirstHorizTile && FirstVertTile )
                            {
                                (*NodeHandleFirst_p)=(stblit_NodeHandle_t* )Node_p;
                            }
                            else
                            {
                                stblit_Connect2Nodes(Device_p,Previous_Node_p,Node_p);
                            }

                            if ( LastHorizTile && LastVertTile )
                            {
                                (*NodeHandleLast_p)=(stblit_NodeHandle_t* )Node_p;
                            }
                            Previous_Node_p = Node_p;
                        }
                        break;

                    /* Case 1:Down left, 3:Down right */
                    case 1:
                    case 3:
                        if (OverlapRectangle.PositionX >= Dst_p->Rectangle.PositionX)
                        {
                            /* The first tile to be copied must be in the left-down side */
                            if ( FirstVertTile )
                            {
                                Last_Node_p = Node_p;
                            }
                            else
                            {
                                stblit_Connect2Nodes(Device_p, Node_p, Next_Node_p);
                            }
                            Next_Node_p = Node_p;

                            if ( FirstHorizTile && FirstVertTile)
                            {
                                SetNodeIT (Node_p, Device_p ,BlitContext_p, &CommonField, FALSE );
                            }

                            if ( LastHorizTile && FirstVertTile)
                            {
                                (*NodeHandleLast_p)=(stblit_NodeHandle_t* )Node_p;
                                SetNodeIT (Node_p, Device_p ,BlitContext_p, &CommonField, TRUE);
                            }

                            if ( FirstHorizTile && LastVertTile )
                            {
                                (*NodeHandleFirst_p)=(stblit_NodeHandle_t* )Node_p;
                            }

                            if ( LastHorizTile && LastVertTile)
                            {
                                SetNodeIT (Node_p, Device_p ,BlitContext_p, &CommonField, FALSE);
                            }

                            if ( LastVertTile && !FirstHorizTile)
                            {
                                stblit_Connect2Nodes(Device_p, Link_Node_p, Node_p);
                            }

                            if ( LastVertTile )
                            {
                                Link_Node_p = Last_Node_p;
                            }
                        }
                        else
                        {
                            /* The first tile to be copied must be in the right-down side */
                            if ( FirstHorizTile && FirstVertTile )
                            {
                                (*NodeHandleLast_p)=(stblit_NodeHandle_t* )Node_p;
                                SetNodeIT (Node_p, Device_p ,BlitContext_p, &CommonField, TRUE);
                            }
                            else
                            {
                                stblit_Connect2Nodes(Device_p, Node_p,Next_Node_p);
                            }

                            if ( LastHorizTile && LastVertTile )
                            {
                                (*NodeHandleFirst_p)=(stblit_NodeHandle_t* )Node_p;
                                SetNodeIT (Node_p, Device_p ,BlitContext_p, &CommonField, FALSE);
                            }

                            Next_Node_p = Node_p;
                        }
                        break;
                    default:
                        break;
                }
            }
            else /* No Overlap */
            {
                if ( FirstHorizTile && FirstVertTile )
                {
                    (*NodeHandleFirst_p)=(stblit_NodeHandle_t* )Node_p;
                }
                else
                {
                    stblit_Connect2Nodes(Device_p,Previous_Node_p,Node_p);
                }

                if ( LastHorizTile && LastVertTile )
                {
                    (*NodeHandleLast_p)=(stblit_NodeHandle_t* )Node_p;
                }
                Previous_Node_p = Node_p;
           }


/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
}
#endif
/*---------------------*/
            /*-------- Updating FirstVertTile ---------*/
            FirstVertTile=FALSE;

            /*-------- Updating TopTile ---------*/
            TopTile = FALSE;

            /*-------- Updating Src1 and Dst PositionY ---------*/
            if ( Src1_p!=NULL)
            {
                TileSrc1.Rectangle.PositionY+=TileSrc1.Rectangle.Height;
            }
            TileDst.Rectangle.PositionY+=TileDst.Rectangle.Height;

/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
            VertIndexDisplay++;
#endif
/*---------------------*/
        }
        while ( LastVertTile==FALSE);

        /*-------- Updating FirstHorizTile ---------*/
        FirstHorizTile=FALSE;

        /*-------- Updating LeftTile ---------*/
        LeftTile = FALSE;
        /*-------- Updating Src1 and Dst PositionX ---------*/
        if ( Src1_p!=NULL)
        {
            TileSrc1.Rectangle.PositionX+=TileSrc1.Rectangle.Width;
        }
        TileDst.Rectangle.PositionX+=TileDst.Rectangle.Width;


/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
        HorizIndexDisplay++;
#endif
/*---------------------*/
    }
    while ( LastHorizTile==FALSE);

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
                                            stblit_NodeHandle_t** NodeHandleFirst_p,
                                            stblit_NodeHandle_t** NodeHandleLast_p,
                                            U32*                  NumberNodes_p,
                                            stblit_OperationConfiguration_t*  Config_p)

{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    stblit_Node_t*          Previous_Node_p = NULL;
    stblit_CommonField_t    CommonField;
    stblit_NodeHandle_t*    NodeHandle_p;
    U8                      ScanConfig  = 0;  /* Vertical and Horizontal scan configuration. Down Right per default */
    U32                     SrcXStop;
    U32                     SrcYStop;
    U32                     DstXStop;
    U32                     DstYStop;
    U8                      OverlapCode;              /* Resulting overlap direction code both src */
    stblit_Source1Mode_t    OpMode = STBLIT_SOURCE1_MODE_NORMAL;
    STGXOBJ_Rectangle_t     TileSrcRectangle;
    S32                     TileDstPositionX = 0, TileDstPositionY = 0;
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
    U32                     HorizIndexDisplay,VertIndexDisplay;
#endif
/*---------------------*/
    BOOL                    FirstHorizTile,LastHorizTile,FirstVertTile,LastVertTile;
    U32                     MaxSrcWidth;
	U32                     TempAddress;

    TileSrcRectangle.PositionX = 0 ;
    TileSrcRectangle.PositionY = 0 ;

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

    /* Set MaxSrcWidth */
#if defined(ST_7109) || defined(ST_7200)
    MaxSrcWidth = STBLIT_MAX_SIZE; /* No resize */
#else
    MaxSrcWidth = STBLIT_MAX_SRC_WIDTH;
#endif


    /*-----------------7/7/2004 8:59AM------------------
     *
     *  Analyzing data and spliting a blit operation
     *  in several sub-operations
     *
     * --------------------------------------------------*/
    (*NumberNodes_p)=0;

    /* Taking in consideration blitter hardware limitation, the bitmap to be copied is splitted in a number of
     * tiles (height=STBLIT_MAX_SRC_HEIGHT, width=MaxSrcWidth). When the source and destination bitmaps copied
     * are overlapped, we must choose the right order of tiles copy. */

    /*-------- Horizantal Sources and destination Initialisation ---------*/
    FirstHorizTile = TRUE;
    LastHorizTile = FALSE;
    TileSrcRectangle.Width = MaxSrcWidth;

    /* ScanConfig gives the position of source compared with destination */
    switch(ScanConfig)
    {
        /* Case 0:Down right, 2:Up right */
        case 0:
        case 2:
            if (SrcRectangle_p->PositionX < DstPositionX)
            {
                /* The first tile to be copied must be in the right side */
                if ( SrcRectangle_p->Width >= MaxSrcWidth )
                {
                    TileDstPositionX = DstPositionX + SrcRectangle_p->Width - MaxSrcWidth;
                    TileSrcRectangle.PositionX = SrcRectangle_p->PositionX + SrcRectangle_p->Width - MaxSrcWidth;
                }
                else
                {
                    TileDstPositionX = DstPositionX;
                    TileSrcRectangle.Width = SrcRectangle_p->Width;
                    TileSrcRectangle.PositionX = SrcRectangle_p->PositionX;
                }
            }
            else
            {
                /* The first tile to be copied must be in the left side */
                TileDstPositionX = DstPositionX;
                TileSrcRectangle.PositionX = SrcRectangle_p->PositionX;
            }
            break;

        /* Case 1:Down left, 3:Down Left (right!!)  */
        case 1:
        case 3:
            if (SrcRectangle_p->PositionX >= DstPositionX)
            {
                /* The first tile to be copied must be in the left side */
                TileDstPositionX = DstPositionX;
                TileSrcRectangle.PositionX = SrcRectangle_p->PositionX;
            }
            else
            {
                /* The first tile to be copied must be in the right side */
                if (SrcRectangle_p->Width >= MaxSrcWidth)
                {
                    TileDstPositionX = DstPositionX + SrcRectangle_p->Width - MaxSrcWidth;
                    TileSrcRectangle.PositionX = SrcRectangle_p->PositionX + SrcRectangle_p->Width - MaxSrcWidth;
                }
                else
                {
                    TileDstPositionX = DstPositionX;
                    TileSrcRectangle.Width = SrcRectangle_p->Width;
                    TileSrcRectangle.PositionX = SrcRectangle_p->PositionX;
                }
            }
            break;
       default:
            break;
    }


/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
    HorizIndexDisplay=0;
#endif
/*---------------------*/


    /*-------- Horizontal Scaling ---------*/
    do
    {
        /*-------- Calculating Src Width ---------*/
        /* ScanConfig gives the position of source compared with destination */
        switch(ScanConfig)
        {
            /* Case 0:Down right, 2:Up right */
            case 0:
            case 2:
                if (SrcRectangle_p->PositionX < DstPositionX)
                {
                    if (TileSrcRectangle.PositionX == SrcRectangle_p->PositionX)
                    {
                        LastHorizTile = TRUE;
                    }
                }
                else
                {
                    if ( (TileSrcRectangle.PositionX + MaxSrcWidth-1) >= (SrcRectangle_p->PositionX + SrcRectangle_p->Width-1) )
                    {
                        TileSrcRectangle.Width = (SrcRectangle_p->Width+SrcRectangle_p->PositionX) - TileSrcRectangle.PositionX;
                        LastHorizTile = TRUE;
                    }
                }
                break;

            /* Case 1:Down left, 3:Down right */
            case 1:
            case 3:
                if (SrcRectangle_p->PositionX >= DstPositionX)
                {
                     if ( (TileSrcRectangle.PositionX + MaxSrcWidth-1) >= (SrcRectangle_p->PositionX + SrcRectangle_p->Width-1) )
                    {
                        TileSrcRectangle.Width = (SrcRectangle_p->Width + SrcRectangle_p->PositionX) - TileSrcRectangle.PositionX;
                        LastHorizTile = TRUE;
                    }
                }
                else
                {
                    if (TileSrcRectangle.PositionX == SrcRectangle_p->PositionX)
                    {
                        LastHorizTile = TRUE;
                    }
                }
                break;

        default:
                break;
        }


        /*-------- Vertical Sources and destination Initialisation ---------*/
        FirstVertTile = TRUE;
        LastVertTile = FALSE;
        TileSrcRectangle.Height = STBLIT_MAX_SRC_HEIGHT;

        /* ScanConfig gives the position of source compared with destination */
        switch(ScanConfig)
        {
            /* Case 0:Down right, 2:Up right */
            case 0:
            case 2:
                /* The first tile to be copied must be in the up side */
                TileDstPositionY = DstPositionY;
                TileSrcRectangle.PositionY = SrcRectangle_p->PositionY;
                break;

            /* Case 1:Down left, 3:Down right */
            case 1:
            case 3:
                /* The first tile to be copied must be in the down side */
                if (SrcRectangle_p->Height >= STBLIT_MAX_SRC_HEIGHT)
                {
                    TileDstPositionY = DstPositionY + SrcRectangle_p->Height - STBLIT_MAX_SRC_HEIGHT;
                    TileSrcRectangle.PositionY = SrcRectangle_p->PositionY + SrcRectangle_p->Height - STBLIT_MAX_SRC_HEIGHT;
                }
                else
                {
                    TileDstPositionY = DstPositionY;
                    TileSrcRectangle.Height = SrcRectangle_p->Height;
                    TileSrcRectangle.PositionY = SrcRectangle_p->PositionY;
                }
                break;

            default:
                break;
        }

/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
        VertIndexDisplay=0;
#endif
/*---------------------*/


        /*-------- Vertical Scaling ---------*/
        do
        {
            /*-------- Calculating Src Height ---------*/
            /* ScanConfig gives the position of source compared with destination */
            switch(ScanConfig)
            {
                /* Case 0:Down right, 2:Up right */
                case 0:
                case 2:
                    if ( (S32)(TileSrcRectangle.PositionY + STBLIT_MAX_SRC_HEIGHT-1) >= (S32)(SrcRectangle_p->PositionY + SrcRectangle_p->Height-1) )
                    {
                        TileSrcRectangle.Height = (SrcRectangle_p->Height + SrcRectangle_p->PositionY) - TileSrcRectangle.PositionY;
                        LastVertTile = TRUE;
                    }
                    break;

                /* Case 1:Down left, 3:Down right */
                case 1:
                case 3:
                    if (TileSrcRectangle.PositionY == SrcRectangle_p->PositionY)
                    {
                        LastVertTile = TRUE;
                    }
                    break;

            default:
                    break;
            }

            /*---------------------- Creating node -----------------------*/
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
if ( (FirstHorizTile&&FirstVertTile) ||
     (LastHorizTile &&LastVertTile) ||
     ( (HorizIndexDisplay%2)==0 && (VertIndexDisplay%2)==0 ) ||
     ( (HorizIndexDisplay%2)!=0 && (VertIndexDisplay%2)!=0 ) )
{
#endif
/*---------------------*/

			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&NodeHandle_p;

			/* Get Node descriptor handle*/
            if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
            {
                Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) TempAddress);
                if (Err != ST_NO_ERROR)
                {
                    /* Release already allocated nodes */
                    if(Previous_Node_p)
                    {
                       stblit_ReleaseListNode(Device_p, (stblit_NodeHandle_t)Previous_Node_p);
                    }
                    return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
                }
            }
            else  /* Blit in Job */
            {
                Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
                if (Err != ST_NO_ERROR)
                {
                    /* Release already allocated nodes */
                    if(Previous_Node_p)
                    {
                       stblit_ReleaseListNode(Device_p, (stblit_NodeHandle_t)Previous_Node_p);
                    }

                    return(STBLIT_ERROR_MAX_JOB_NODE);
                }
            }

            Node_p  = (stblit_Node_t*)(NodeHandle_p);

            memset(&CommonField, 0, sizeof(stblit_CommonField_t));

            CopyOperationNodeSetup(Node_p, BlitContext_p, SrcBitmap_p, &TileSrcRectangle, DstBitmap_p, TileDstPositionX,
                                   TileDstPositionY, Device_p, &CommonField, Config_p, ScanConfig, OpMode, ( LastHorizTile && LastVertTile ) );

            stblit_WriteCommonFieldInNode(Device_p, Node_p ,&CommonField);

            (*NumberNodes_p)++;

            if ( FirstHorizTile && FirstVertTile )
            {
                (*NodeHandleFirst_p) = (stblit_NodeHandle_t* )Node_p;
            }
            else
            {
                stblit_Connect2Nodes(Device_p, Previous_Node_p,Node_p);
            }

            if ( LastHorizTile && LastVertTile )
            {
                (*NodeHandleLast_p) = (stblit_NodeHandle_t* )Node_p;
            }

            Previous_Node_p = Node_p;
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
}
#endif
/*---------------------*/
            /*-------- Updating FirstVertTile ---------*/
            FirstVertTile=FALSE;

            /*-------- Updating Src and Dst PositionY ---------*/
            /* ScanConfig gives the position of source compared with destination */
            switch(ScanConfig)
            {
                /* Case 0:Down right, 2:Up right */
                case 0:
                case 2:
                    /* The tiles are copied from up to down */
                    TileSrcRectangle.PositionY += STBLIT_MAX_SRC_HEIGHT;
                    TileDstPositionY += STBLIT_MAX_SRC_HEIGHT;
                    break;

                /* Case 1:Down left, 3:Down right */
                case 1:
                case 3:
                    /* The tiles are copied from down to up */
                    if (TileSrcRectangle.PositionY - STBLIT_MAX_SRC_HEIGHT >= SrcRectangle_p->PositionY)
                    {
                        TileSrcRectangle.PositionY -= STBLIT_MAX_SRC_HEIGHT;
                        TileDstPositionY -= STBLIT_MAX_SRC_HEIGHT;
                    }
                    else
                    {
                        TileSrcRectangle.Height = TileSrcRectangle.PositionY - SrcRectangle_p->PositionY;
                        TileSrcRectangle.PositionY -= TileSrcRectangle.Height;
                        TileDstPositionY -= TileSrcRectangle.Height;
                    }
                    break;

                default:
                    break;
            }
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
            VertIndexDisplay++;
#endif
/*---------------------*/
        }
        while ( LastVertTile==FALSE);

        /*-------- Updating FirstHorizTile ---------*/
        FirstHorizTile=FALSE;

        /*-------- Updating Src1 and Dst PositionX ---------*/

        /* ScanConfig gives the position of source compared with destination */
        switch(ScanConfig)
        {
            /* Case 0:Down right, 2:Up right */
            case 0:
            case 2:
                if (SrcRectangle_p->PositionX < DstPositionX)
                {
                    /* The tiles are copied from right to left */
                    if ((S32)(TileSrcRectangle.PositionX - MaxSrcWidth) >= SrcRectangle_p->PositionX)
                    {
                        TileSrcRectangle.PositionX -= MaxSrcWidth;
                        TileDstPositionX -= MaxSrcWidth;
                    }
                    else
                    {
                        TileSrcRectangle.Width = TileSrcRectangle.PositionX - SrcRectangle_p->PositionX;
                        TileSrcRectangle.PositionX -= TileSrcRectangle.Width;
                        TileDstPositionX -= TileSrcRectangle.Width;
                    }
                }
                else
                {
                    /* The tiles are copied from left to right */
                    TileSrcRectangle.PositionX += MaxSrcWidth;
                    TileDstPositionX += MaxSrcWidth;
                }
                break;

            /* Case 1:Down left, 3:Down right */
            case 1:
            case 3:
                if (SrcRectangle_p->PositionX >= DstPositionX)
                {
                    /* The tiles are copied from left to right */
                    TileSrcRectangle.PositionX += MaxSrcWidth;
                    TileDstPositionX += MaxSrcWidth;
                }
                else
                {
                    /* The tiles are copied from right to left */
                    if ((S32)(TileSrcRectangle.PositionX - MaxSrcWidth) >= SrcRectangle_p->PositionX)
                    {
                        TileSrcRectangle.PositionX -= MaxSrcWidth;
                        TileDstPositionX -= MaxSrcWidth;
                    }
                    else
                    {
                        TileSrcRectangle.Width = TileSrcRectangle.PositionX - SrcRectangle_p->PositionX;
                        TileSrcRectangle.PositionX -= TileSrcRectangle.Width;
                        TileDstPositionX -= TileSrcRectangle.Width;
                    }
                }
                break;

            default:
                break;
        }
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
        HorizIndexDisplay++;
#endif
/*---------------------*/
    }
    while ( LastHorizTile==FALSE);

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
                                            STGXOBJ_Color_t*      Color_p,
                                            STBLIT_BlitContext_t* BlitContext_p,
                                            stblit_NodeHandle_t** NodeHandleFirst_p,
                                            stblit_NodeHandle_t** NodeHandleLast_p,
                                            U32*                  NumberNodes_p,
                                            stblit_OperationConfiguration_t*  Config_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    stblit_Node_t*          Node_p;
    stblit_Node_t*          Previous_Node_p = NULL;
    stblit_CommonField_t    CommonField;
    stblit_NodeHandle_t*    NodeHandle_p;
    U8                      ScanConfig  = 0;  /* Vertical and Horizontal scan configuration. Down Right per default */
    stblit_Source1Mode_t    OpMode = STBLIT_SOURCE1_MODE_NORMAL;
    BOOL                    IdenticalDstSrc1 = FALSE;
    STGXOBJ_Rectangle_t     TileDstRectangle;
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
    U32                     HorizIndexDisplay,VertIndexDisplay;
#endif
/*---------------------*/
    BOOL                    FirstHorizTile,LastHorizTile,FirstVertTile,LastVertTile;
    BOOL                    TopTile, BottomTile, RightTile, LeftTile;
    U32                     MaxSrcWidth;
	U32                     TempAddress;


    /* Set MaxSrcWidth */
#if defined(ST_7109) || defined(ST_7200)
    MaxSrcWidth = STBLIT_MAX_SIZE; /* No resize */
#else
    MaxSrcWidth = STBLIT_MAX_SRC_WIDTH;
#endif

    /*-----------------7/7/2004 8:59AM------------------
     *
     *  Analyzing data and spliting a blit operation
     *  in several sub-operations
     *
     * --------------------------------------------------*/
    (*NumberNodes_p)=0;

    /*-------- Horizantal Sources and destination Initialisation ---------*/
    FirstHorizTile = TRUE;
    LastHorizTile  = FALSE;
    LeftTile       = TRUE;
    RightTile      = FALSE;

    TileDstRectangle.PositionX=Rectangle_p->PositionX;
    TileDstRectangle.Width=MaxSrcWidth;
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
    HorizIndexDisplay=0;
#endif
/*---------------------*/

    /*-------- Horizontal Scaling ---------*/
    do
    {
        /*-------- Calculating Dst Width ---------*/
        if ( (TileDstRectangle.PositionX+MaxSrcWidth-1)>=(Rectangle_p->PositionX+Rectangle_p->Width-1) )
        {
            TileDstRectangle.Width=(Rectangle_p->Width+Rectangle_p->PositionX)-TileDstRectangle.PositionX;
            LastHorizTile = TRUE;
            RightTile     = TRUE;

        }

        /*-------- Vertical Sources and destination Initialisation ---------*/
        FirstVertTile = TRUE;
        LastVertTile  = FALSE;
        TopTile       = TRUE;
        BottomTile    = FALSE;

        TileDstRectangle.PositionY=Rectangle_p->PositionY;
        TileDstRectangle.Height=STBLIT_MAX_SRC_HEIGHT;
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
        VertIndexDisplay=0;
#endif
/*---------------------*/


        /*-------- Vertical Scaling ---------*/
        do
        {
            /*-------- Calculating Dst Height ---------*/
            if ((S32)(TileDstRectangle.PositionY+STBLIT_MAX_SRC_HEIGHT-1)>=(S32)(Rectangle_p->PositionY+Rectangle_p->Height-1))
            {
                TileDstRectangle.Height=(Rectangle_p->Height+Rectangle_p->PositionY)-TileDstRectangle.PositionY;
                LastVertTile     = TRUE;
                BottomTile       = TRUE;

            }

            /*---------------------- Creating node -----------------------*/

/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
if ( (FirstHorizTile&&FirstVertTile) ||
     (LastHorizTile &&LastVertTile) ||
     ( (HorizIndexDisplay%2)==0 && (VertIndexDisplay%2)==0 ) ||
     ( (HorizIndexDisplay%2)!=0 && (VertIndexDisplay%2)!=0 ) )
{
#endif
/*---------------------*/

			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&NodeHandle_p;

			/* Get Node descriptor handle*/
            if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single Blit */
            {
                Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) TempAddress);
                if (Err != ST_NO_ERROR)
                {
                    /* Release already allocated nodes */
                    if(Previous_Node_p)
                    {
                       stblit_ReleaseListNode(Device_p, (stblit_NodeHandle_t)Previous_Node_p);
                    }
                    return(STBLIT_ERROR_MAX_SINGLE_BLIT_NODE);
                }
            }
            else  /* Blit in Job */
            {
				Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
                if (Err != ST_NO_ERROR)
                {
                    /* Release already allocated nodes */
                    if(Previous_Node_p)
                    {
                       stblit_ReleaseListNode(Device_p, (stblit_NodeHandle_t)Previous_Node_p);
                    }

                    return(STBLIT_ERROR_MAX_JOB_NODE);
                }
            }

            Node_p  = (stblit_Node_t*)(NodeHandle_p);
            memset(&CommonField, 0, sizeof(stblit_CommonField_t));

            FillOperationNodeSetup(Node_p,BlitContext_p, Bitmap_p, &TileDstRectangle, Color_p, Device_p, &CommonField,
                                   Config_p, ScanConfig, OpMode, IdenticalDstSrc1, (LastHorizTile && LastVertTile),TopTile, BottomTile, RightTile, LeftTile);

            stblit_WriteCommonFieldInNode(Device_p, Node_p ,&CommonField);

            (*NumberNodes_p)++;

            if ( FirstHorizTile && FirstVertTile )
            {
                (*NodeHandleFirst_p)=(stblit_NodeHandle_t* )Node_p;
            }
            else
            {
                stblit_Connect2Nodes(Device_p, Previous_Node_p,Node_p);
            }

            if ( LastHorizTile && LastVertTile )
            {
                (*NodeHandleLast_p)=(stblit_NodeHandle_t* )Node_p;
            }

            Previous_Node_p = Node_p;
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
}
#endif
/*---------------------*/
            /*-------- Updating FirstVertTile ---------*/
            FirstVertTile=FALSE;

            /*-------- Updating TopTile ---------*/
            TopTile = FALSE;

            /*-------- Updating Dst PositionY ---------*/
            TileDstRectangle.PositionY+=STBLIT_MAX_SRC_HEIGHT;
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
            VertIndexDisplay++;
#endif
/*---------------------*/
        }
        while ( LastVertTile==FALSE);

        /*-------- Updating FirstHorizTile ---------*/
        FirstHorizTile=FALSE;

        /*-------- Updating LeftTile ---------*/
        LeftTile = FALSE;

        /*-------- Updating Src1 and Dst PositionX ---------*/
        TileDstRectangle.PositionX+=MaxSrcWidth;
/*---------------------*/
#ifdef STBLIT_SLICE_DEMO
        HorizIndexDisplay++;
#endif
/*---------------------*/
    }
    while ( LastHorizTile==FALSE);

#ifdef ST_OSLINUX
#ifdef DEBUG_BLIT
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_NIP = 0x%x-----------------\n", Node_p->HWNode.BLT_NIP ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_CIC = 0x%x-----------------\n", Node_p->HWNode.BLT_CIC ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_INS = 0x%x-----------------\n", Node_p->HWNode.BLT_INS ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_ACK = 0x%x-----------------\n", Node_p->HWNode.BLT_ACK ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_TBA = 0x%x-----------------\n", Node_p->HWNode.BLT_TBA ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_TTY = 0x%x-----------------\n", Node_p->HWNode.BLT_TTY ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_TXY = 0x%x-----------------\n", Node_p->HWNode.BLT_TXY ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_T_S1_SZ = 0x%x-----------------\n", Node_p->HWNode.BLT_T_S1_SZ ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S1CF = 0x%x-----------------\n", Node_p->HWNode.BLT_S1CF ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S2CF = 0x%x-----------------\n", Node_p->HWNode.BLT_S2CF ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S1BA = 0x%x-----------------\n", Node_p->HWNode.BLT_S1BA ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S1TY = 0x%x-----------------\n", Node_p->HWNode.BLT_S1TY ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S1XY = 0x%x-----------------\n", Node_p->HWNode.BLT_S1XY ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_STUFF_1 = 0x%x-----------------\n", Node_p->HWNode.BLT_STUFF_1 ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S2BA = 0x%x-----------------\n", Node_p->HWNode.BLT_S2BA ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S2TY = 0x%x-----------------\n", Node_p->HWNode.BLT_S2TY ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S2XY = 0x%x-----------------\n", Node_p->HWNode.BLT_S2XY ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S2SZ = 0x%x-----------------\n", Node_p->HWNode.BLT_S2SZ ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S3BA = 0x%x-----------------\n", Node_p->HWNode.BLT_S3BA ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S3TY = 0x%x-----------------\n", Node_p->HWNode.BLT_S3TY ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S3XY = 0x%x-----------------\n", Node_p->HWNode.BLT_S3XY ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_S3SZ = 0x%x-----------------\n", Node_p->HWNode.BLT_S3SZ ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_CWO = 0x%x-----------------\n", Node_p->HWNode.BLT_CWO ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_CWS = 0x%x-----------------\n", Node_p->HWNode.BLT_CWS ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_CCO = 0x%x-----------------\n", Node_p->HWNode.BLT_CCO ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_CML = 0x%x-----------------\n", Node_p->HWNode.BLT_CML ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_F_RZC_CTL = 0x%x-----------------\n", Node_p->HWNode.BLT_F_RZC_CTL ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_PMK = 0x%x-----------------\n", Node_p->HWNode.BLT_PMK ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_RSF = 0x%x-----------------\n", Node_p->HWNode.BLT_RSF ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_RZI = 0x%x-----------------\n", Node_p->HWNode.BLT_RZI ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_HFP = 0x%x-----------------\n", Node_p->HWNode.BLT_HFP ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_VFP = 0x%x-----------------\n", Node_p->HWNode.BLT_VFP ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_Y_RSF = 0x%x-----------------\n", Node_p->HWNode.BLT_Y_RSF ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_Y_RZI = 0x%x-----------------\n", Node_p->HWNode.BLT_Y_RZI ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_Y_HFP = 0x%x-----------------\n", Node_p->HWNode.BLT_Y_HFP ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_Y_VFP = 0x%x-----------------\n", Node_p->HWNode.BLT_Y_VFP ));


    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_FF0 = 0x%x-----------------\n", Node_p->HWNode.BLT_FF0 ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_FF1 = 0x%x-----------------\n", Node_p->HWNode.BLT_FF1 ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_FF2 = 0x%x-----------------\n", Node_p->HWNode.BLT_FF2 ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_FF3 = 0x%x-----------------\n", Node_p->HWNode.BLT_FF3 ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_KEY1 = 0x%x-----------------\n", Node_p->HWNode.BLT_KEY1 ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_KEY2 = 0x%x-----------------\n", Node_p->HWNode.BLT_KEY2 ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_XYL = 0x%x-----------------\n", Node_p->HWNode.BLT_XYL ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_XYP = 0x%x-----------------\n", Node_p->HWNode.BLT_XYP ));

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_SAR = 0x%x-----------------\n", Node_p->HWNode.BLT_SAR ));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "---------- BLT_USR = 0x%x-----------------\n", Node_p->HWNode.BLT_USR ));

#endif

#endif


    return(Err);
}

#if defined(ST_7109) || defined(ST_7200)
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

    /* Initialize interruption stuff*/
    SetNodeIT (Node_p, Device_p ,BlitContext_p, CommonField_p, TRUE);

    /* Set Src1  */
    SetNodeSrc1Bitmap(Node_p,SrcColorBitmap_p,SrcColorRectangle_p,ScanConfig,Device_p,STBLIT_SOURCE1_MODE_NORMAL,CommonField_p);

    /* Set Src2  */
    SetNodeSrc2Bitmap(Node_p,SrcAlphaBitmap_p,SrcAlphaRectangle_p,ScanConfig,Device_p,CommonField_p, 0, 0, 0, 0 );

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

    /* Set Info about operation configuration (only if Job)*/
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Config_p->ConcatMode             = TRUE;
        /* Other fields to be written????? */
    }


    return(Err);
}
#endif

#if defined(ST_7109) || defined(ST_7200)
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

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        /* Update mask word */
        UpdateNodePlaneMask(Node_p,MaskWord);

        NodeHandle =  Node_p->PreviousNode;
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

    NodeHandle = JBlit_p->LastNodeHandle;

    /* Update all nodes */
    for (i=0; i < JBlit_p->NbNodes ;i++ )
    {
        Node_p = (stblit_Node_t*)NodeHandle;

        /* Update Color key */
        UpdateNodeColorKey (Node_p,ColorKeyCopyMode,ColorKey_p);

        NodeHandle =  Node_p->PreviousNode;
    }
    return(Err);
 }

#ifdef ST_OSWINCE
 /*******************************************************************************
Name        : stblit_UpdateGlobalAlpha
Description : Update Global Alpha in node for a given job blend Blit.
Parameters  :
Assumptions : All parameters are checked at upper level
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t stblit_UpdateGlobalAlpha(stblit_JobBlit_t* JBlit_p, U8 GlobalAlpha)
{
	ST_ErrorCode_t          Err = ST_NO_ERROR;
	stblit_Node_t*          Node_p;
	stblit_NodeHandle_t     NodeHandle;
	U32                     i;
	U32                     Mask;
	stblit_CommonField_t    CommonField;

	// compute mask
	Mask =   STBLIT_ACK_ALU_GLOBAL_ALPHA_MASK << STBLIT_ACK_ALU_GLOBAL_ALPHA_SHIFT
		   |         STBLIT_ACK_ALU_MODE_MASK <<         STBLIT_ACK_ALU_MODE_SHIFT;

	// compute value (in ACK field)
	CommonField.ACK = 0;
	Err = SetNodeALUBlend(GlobalAlpha, JBlit_p->Cfg.PreMultipliedColor, &CommonField);
	if (Err != ST_NO_ERROR)
		return Err;

	NodeHandle = JBlit_p->LastNodeHandle;
	/* Update all nodes */
	for (i = 0; i < JBlit_p->NbNodes; i++)
	{
		Node_p = (stblit_Node_t*)NodeHandle;

		/* Update Global alpha */
		UpdateNodeGlobalAlpha(Node_p, CommonField.ACK, Mask);

		NodeHandle =  Node_p->PreviousNode;
	}
	return Err;
}

#endif
#endif /* 7109 */
/* End of bdisp_mem.c */
