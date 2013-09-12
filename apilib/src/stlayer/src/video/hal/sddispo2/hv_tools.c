/*******************************************************************************

File name   : hv_tools.c

Description : Layer Video HAL (Hardware Abstraction Layer) for sddispo2 chips access to hardware source file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
05 Aug 2003        Created                                          AN

Content     :
            - stlayer_GetIncrement().
            - stlayer_GetPositionDefinition().
            - stlayer_GetMemorySize().
            - stlayer_WriteHorizontalCoefficientsFilterToMemory().
            - stlayer_WriteVerticalCoefficientsFilterToMemory().
            - stlayer_WriteHorizontalNullCoefficientsFilterToMemory().
            - stlayer_WriteVerticalNullCoefficientsFilterToMemory().
            - stlayer_UpdateDisplayRegisters().
            - stlayer_GetDecimationFactorFromDecimation.

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* This file is a Copy / Paste of the file coming from the Validation :       */
/* \dvdgr-prj-valigamma\soft\hal\gam_hal_vo_util.c                            */
/* The functions calculate initial phase (ph0) of polyphase filters, first    */
/* line/pixel repeat (fylr_t(b), fypr, fclr_t(b), fcpr)                       */
/*                                                                            */
/*  Inputs:                                                                   */
/*  -------                                                                   */
/*   pos_x                  horizontal position of the source window -        */
/* ->  PositionX            integer part in pixel units (from 0 to            */
/*                          horizontal_size-1)                                */
/*                                                                            */
/*   pos_y                  vertical position (in the frame) of the source    */
/* ->  PositionY            window - integer part in line units (from 0 to    */
/*                          vertical_size-1)                                  */
/*                                                                            */
/*   sub_x                  sub-pixel position of the source window (from 0   */
/* -> SubPositionX          to 31) position_x = pos_x + sub_x/32 in pixel unit*/
/*                                                                            */
/*   sub_y                  sub-line position of the source window (from 0 to */
/* -> SubPositionY          31) position_y = pos_y + sub_y/32 in line unit    */
/*                          (frame based)                                     */
/*                                                                            */
/*   nrep_pix               0 - first pixel/line is systematicaly repeated    */
/*                                                                            */
/*   fr_notfi               0 - field (interlaced) scan,                      */
/*                          1 - frame (progressive) scan (Target)             */
/* -> IsOutputScantypeProgressive                                              */
/*                                                                            */
/*   f422_not420            0 - 4:2:0 chroma format,                          */
/* -> IsFormat422Not420     1 - 4:2:2 chroma format                           */
/*                                                                            */
/*   one_nottwo_buf         0 - 2 input buffers (luma and chroma),            */
/*                          1 - one input buffer (luma and chroma in the same */
/*                              buffer)                                       */
/* -> IsFormatRasterNotMacroblock                                             */
/*                                                                            */
/*   t_notb_first           0 - bottom field on the first line of the viewport*/
/* -> IsFirstLineTopNotBot  1 - top field on the first line of the viewport   */
/*                                                                            */
/*   sp_ni                  0 - Source is considered as interlaced,           */
/*                          1 - Source considered as progressive              */
/* -> IsInputScantypeProgressive                                              */
/*                                                                            */
/*   Incr                   Value of increment (zoom factor)                  */
/* -> Increment                                                               */
/*                                                                            */
/*   sp_ni_used             Can be different from sp_in only when zoom out    */
/*                          is bigger than 4 and the output is interlaced     */
/*                          with progressive input                            */
/*                                                                            */
/*  Outputs:                                                                  */
/*  -------                                                                   */
/*   PosMem                 Position in the memory of the first line and      */
/* -> MemoryPosition        pixel to be  accessed                             */
/*                                                                            */
/*   FRep                   How many times first pixel/line is to be repeated */
/* -> FirstPixelRepetition  to have nice borders                              */
/*                                                                            */
/*   Ph0                    Initial phase to be put in polyphase filter       */
/* -> Phase                                                                   */

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <string.h>
#endif

#include "stddefs.h"
#include "hv_tools.h"
#include "hv_filt8.h"

#ifdef ST_OSLINUX
    #include "stavmem.h"
#endif

/* Enable this to get the possibility to force a given VHSRC filter */
/*#define FORCE_VHSRC_FILTER */

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/*#define TRACE_UART*/

/* Select the Traces you want */
#define TrMain                 TraceOff
#define TrDebug                TraceOff         /* Additional trace for debug */

#define TrWarning              TraceOn
/* TrError is now defined in layer_trace.h */

/* layer_trace.h" should be included AFTER the definition of the traces wanted */
#include "layer_trace.h"

/* Private Constants -------------------------------------------------------- */

/* Default automatic parameters settings.                                     */

const STLAYER_TintParameters_t  DefaultTintSettings[5] =
    {       /*  STLAYER_VIDEO_CHROMA_TINT               */
            /* Chroma Tint default parameters.          */
    {00},                                   /* Disable  */
    {-50},                                  /* AutoMode1*/
    {+25},                                  /* AutoMode2*/
    {+75},                                  /* AutoMode3*/
    {00}                                    /* Manual  */
    };
const STLAYER_SatParameters_t DefaultSatSettings[5] =
    {       /*  STLAYER_VIDEO_CHROMA_SAT                */
            /* Chroma Saturation default parameters.    */
    {00},                                   /* Disable  */
    {-50},                                  /* AutoMode1*/
    {+25},                                  /* AutoMode2*/
    {+75},                                  /* AutoMode3*/
    {00}                                    /* Manual  */
    };
const STLAYER_BrightnessContrastParameters_t DefaultBCSettings[5] =
    {       /*  STLAYER_VIDEO_LUMA_BC                   */
            /* Chroma Saturation default parameters.    */
    {00,50},                                   /* Disable  */
    {-50,5},                                  /* AutoMode1*/
    {+25,40},                                  /* AutoMode2*/
    {+75,70},                                  /* AutoMode3*/
    {00,50}                                    /* Manual  */
    };

/* Private Variables FOR DEBUG ONLY ----------------------------------------- */
#ifdef FORCE_VHSRC_FILTER
    BOOL                ManualFilterSelection = FALSE;
    HALLAYER_Filter_t   HorizontalLumaFilter = FILTER_B;
    HALLAYER_Filter_t   HorizontalChromaFilter = FILTER_B;
    HALLAYER_Filter_t   VerticalLumaFilter = FILTER_B;
    HALLAYER_Filter_t   VerticalChromaFilter = FILTER_B;
#endif

/* Private Types ------------------------------------------------------------ */

#if defined(DVD_SECURED_CHIP)
/* Structure for storing each Luma/chroma filter pair */
typedef struct stlayer_filter_s
{
    const void * Luma;
    const void * Chroma;
} stlayer_LumaChroma_CoefficientsPair_t;

/* Structure for controling the loaded filters */
typedef struct stlayer_filter_info_s
{
    BOOL         FiltersLoaded;
    /* Horizontal coefficients filters addresses */
    const void * HorizontalCoefficientsA_Address;
    const void * HorizontalCoefficientsB_ZoomX2_Address;
    const void * HorizontalCoefficientsC_Address;
    const void * HorizontalCoefficientsD_Address;
    const void * HorizontalCoefficientsE_Address;
    const void * HorizontalCoefficientsF_Address;
    /* Vertical coefficients filters addresses */
    const void * VerticalCoefficientsA_Address;
    const void * VerticalCoefficientsB_ZoomX2_Address;
    const void * VerticalCoefficientsC_Address;
    const void * VerticalCoefficientsD_Address;
    const void * VerticalCoefficientsE_Address;
    const void * VerticalCoefficientsF_Address;
} stlayer_filter_info_t;
#endif /* defined(DVD_SECURED_CHIP) */

/* Private Macros ----------------------------------------------------------- */

#if defined(DVD_SECURED_CHIP)
#define NEXT_ALIGNED_ADDRESS(addr, align) (void*)(((U32)addr+align-1)&(~(align-1)))
#endif /* defined(DVD_SECURED_CHIP) */

/* Private Variables (static)------------------------------------------------ */

#if defined(DVD_SECURED_CHIP)
static stlayer_filter_info_t CoefficientsFiltersInfo =
{
    FALSE, /* FiltersLoaded */
    NULL,  /* HorizontalCoefficientsA_Address */
    NULL,  /* HorizontalCoefficientsB_ZoomX2_Address */
    NULL,  /* HorizontalCoefficientsC_Address */
    NULL,  /* HorizontalCoefficientsD_Address */
    NULL,  /* HorizontalCoefficientsE_Address */
    NULL,  /* HorizontalCoefficientsF_Address */
    NULL,  /* VerticalCoefficientsA_Address */
    NULL,  /* VerticalCoefficientsB_ZoomX2_Address */
    NULL,  /* VerticalCoefficientsC_Address */
    NULL,  /* VerticalCoefficientsD_Address */
    NULL,  /* VerticalCoefficientsE_Address */
    NULL   /* VerticalCoefficientsF_Address */
};
#endif /* defined(DVD_SECURED_CHIP) */

/* Functions ---------------------------------------------------------------- */

/* private ERROR management functions --------------------------------------- */

/* private functions -------------------------------------------------------- */

static void GetHorizontalSettings(S32 PosIn, S32 PosOut, U32 * Phase_p, U32 * LineRepeat_p);
static void GetVerticalSettings(S32 PosIn, S32 PosOut, U32 * Phase_p, U32 * LineRepeat_p);

#if defined(HW_7100) || defined(HW_7109)
/* PSI feature is only supported by the STi7100 and the STi7109  */

static ST_ErrorCode_t SetTintMode (const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);
static ST_ErrorCode_t SetSaturationMode (const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);
static ST_ErrorCode_t SetBCMode (const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);
#endif /* defined(HW_7100) || defined(HW_7109) */

/*******************************************************************************
Name        : GetHorizontalSettings
Description : Function called by GetPositionDefinition in the polyphase filter
              calculation.
              This function calculates the initial phase of polyphase filters
              and the first pixel repeat of the Horizontal filter.
Parameters  :
Assumptions : PosIn and PosOut should be expressed on a grid of 8192 steps.
Limitations :
Returns     : nothing
*******************************************************************************/
static void GetHorizontalSettings(S32 PosIn, S32 PosOut, U32 * Phase_p, U32 * LineRepeat_p)
{
    S32     PositionDelta;
    S32     Repeat, Phase;

    PositionDelta = (PosOut - PosIn);

    if (PositionDelta < 0)
    {
        Repeat = 4;
    }
    else
    {
        if (PositionDelta < 8192)
        {
            Repeat = 3;
        }
        else
        {
            if (PositionDelta < 16384)
            {
                Repeat = 2;
            }
            else
            {
                if (PositionDelta < 20480)
                {
                    Repeat = 1;
                }
                else
                {
                    Repeat = 0;
                }
            }
        }
    }

    Phase = PositionDelta - (3-Repeat) * 8192;
    /* Phase should then be in range [0..8191] */

    if (Phase < 0)
    {
        /*TrError(("\r\nError! Output pixel is out of accessible horizontal range! (%d)", Phase));*/
        Phase = 0;
    }

    if (Phase >= 8192)
    {
        /*TrError(("\r\nError! Output pixel is out of accessible horizontal range! (%d)", Phase));*/
        Phase = 8191;
    }

    /* Return the Repeat and Phase values */
    *LineRepeat_p = Repeat;
    *Phase_p = Phase;
}

/*******************************************************************************
Name        : GetVerticalSettings
Description : Function called by GetPositionDefinition in the polyphase filter
              calculation.
              This function calculates the initial phase of polyphase filters
              and the first line repeat of the Vertical filter.
Parameters  :
Assumptions : PosIn and PosOut should be expressed on a grid of 8192 steps.
Limitations :
Returns     : nothing
*******************************************************************************/
static void GetVerticalSettings(S32 PosIn, S32 PosOut, U32 * Phase_p, U32 * LineRepeat_p)
{
    S32     PositionDelta;
    S32     Repeat, Phase;

    PositionDelta = (PosOut - PosIn);

    /* For (-4096 < PositionDelta < 4096) we should in fact use Repeat=3 but this value is not supported by the Hw */
    if (PositionDelta < 4096)
    {
        Repeat = 2;
    }
    else
    {
        if (PositionDelta < 12288)
        {
            Repeat = 1;
        }
        else
        {
            Repeat = 0;
        }
    }

    Phase = PositionDelta + 4096 - (2-Repeat) * 8192;
    /* Phase should then be in range [0..8191] */

    if (Phase < 0)
    {
        /*TrError(("\r\nError! Output pixel is out of accessible range! (%d)", Phase));*/
        Phase = 0;
    }

    if (Phase >= 8192)
    {
        /*TrError(("\r\nError! Output pixel is out of accessible range! (%d)", Phase));*/
        Phase = 8191;
    }

    /* Return the Repeat and Phase values */
    *LineRepeat_p = Repeat;
    *Phase_p = Phase;
}

/* Global functions --------------------------------------------------------- */


/*******************************************************************************
Name        : stlayer_GetIncrement
Description : Calculate and return the increments.
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void stlayer_GetIncrement(
        STGXOBJ_Rectangle_t * InputRectangle_p,
        STGXOBJ_Rectangle_t * OutputRectangle_p,
        BOOL IsFormat422Not420,
        BOOL IsInputScantypeProgressive,
        BOOL IsOutputScantypeProgressive,
        HALLAYER_Increment_t *Increment_p)
{
#if (defined WA_GNBvd16935)
    U32 OriginalIncrement = 0;
#endif

    /* Vertical increment */
    /* Luma */
    /* Test output height to avoid div by 0 */
    if (OutputRectangle_p->Height != 0)
    {
        if ( (IsOutputScantypeProgressive && IsInputScantypeProgressive) || (!IsOutputScantypeProgressive && !IsInputScantypeProgressive))
        {
            /* Both source and target interlaced or both progressive */
            Increment_p->LumaVertical = (32 * (InputRectangle_p->Height << 8)) / OutputRectangle_p->Height;
        }
        else
        if (!IsOutputScantypeProgressive && IsInputScantypeProgressive)
        {
            /* Target interlaced, Source progressive */
            Increment_p->LumaVertical = (32 * (InputRectangle_p->Height << 9)) / OutputRectangle_p->Height;
        }
        else
        {
            /* Target progressive, Source interlaced */
            Increment_p->LumaVertical = 32 * (InputRectangle_p->Height << 7) / OutputRectangle_p->Height;
        }
    }
    else
    {
        Increment_p->LumaVertical = LUMA_VERTICAL_MAX_ZOOM_OUT + 1; /* OutputRectangle height is zero in this case*/
    }

#if (defined WA_GNBvd16935)
    OriginalIncrement = Increment_p->LumaVertical;
    /* NB: Original patch used by validation team, but commenting the following line    */
    /*     gives a better output result.                                                */
    /*     Increment_p->LumaVertical = (Increment_p->LumaVertical + (1<<8));*/
    Increment_p->LumaVertical = Increment_p->LumaVertical & 0xFFFFFE00;
    /* Check any change...   */
    if (OriginalIncrement < Increment_p->LumaVertical)
    {
        /* Adjust output window according to hardware constraints.  */
        OutputRectangle_p->Height = (InputRectangle_p->Height * NO_ZOOM_FACTOR) / Increment_p->LumaVertical;
    }
#endif

    /* Chroma */
    if (IsFormat422Not420)
    {
        Increment_p->ChromaVertical = Increment_p->LumaVertical;
    }
    else
    {
        Increment_p->ChromaVertical = Increment_p->LumaVertical / 2;
    }

    /* Horizontal increment */
    /* Test output width to avoid div by 0 */
    if (OutputRectangle_p->Width != 0)
    {
        Increment_p->LumaHorizontal = (32 * (InputRectangle_p->Width << 8)) / OutputRectangle_p->Width;
    }
    else
    {
        Increment_p->LumaHorizontal = LUMA_HORIZONTAL_MAX_ZOOM_OUT + 1;
    }
    if (Increment_p->LumaHorizontal > LUMA_HORIZONTAL_MAX_ZOOM_OUT)
    {
        /* Directly force the output window to size 0.   */
        OutputRectangle_p->Width = 0;
    }
    else
    {
#if (defined WA_GNBvd16935)
        OriginalIncrement = Increment_p->LumaHorizontal;
    /* NB: Original patch used by validation team, but commenting the following line    */
    /*     gives a better output result.                                                */
    /*     Increment_p->LumaHorizontal = (Increment_p->LumaHorizontal + (1<<8));        */
        Increment_p->LumaHorizontal = Increment_p->LumaHorizontal & 0xFFFFFE00;

        if (OriginalIncrement < Increment_p->LumaHorizontal)
        {
            /* Adjust output window according to hardware constraints.  */
            OutputRectangle_p->Width = (InputRectangle_p->Width * NO_ZOOM_FACTOR) / Increment_p->LumaHorizontal;
        }
#endif
        Increment_p->ChromaHorizontal = Increment_p->LumaHorizontal / 2;
    }
} /* End of stlayer_GetIncrement() function. */


/*******************************************************************************
Name        : stlayer_GetPositionDefinition
Description : Calculate and return the polyphase filter parameters.
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void stlayer_GetPositionDefinition(
        STGXOBJ_Rectangle_t *           InputRectangle_p,
        HALLAYER_Increment_t *          Increment_p,
        BOOL                            IsInputScantypeProgressive,
        BOOL                            IsOutputScantypeProgressive,
        BOOL                            IsFormat422Not420,
        BOOL                            IsFormatRasterNotMacroblock,
        BOOL                            IsFirstLineTopNotBot,
        BOOL                            IsLmuActivated,
        BOOL                            IsPseudo422FromTopField,
        U32                             DecimXOffset,
        U32                             DecimYOffset,
        STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation,
        STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation,
        BOOL                            advanced_decimation,
        HALLAYER_PositionDefinition_t * PositionDefinition_p)
{
    HALLAYER_Position_t                 Position;
    HALLAYER_Phase_t                    Phase;
    HALLAYER_FirstPixelRepetition_t     FirstPixelRepetition;
    HALLAYER_MemoryPosition_t           MemoryPosition;
    /* Horizontal variables */
    S32    HDinY, HDinC;                /* Horizontal distance between 2 consecutive Luma/Chroma samples */
    S32    HPosInY, HPosInC;            /* Horizontal Luma/Chroma input positions */
    S32    HPosOutY, HPosOutC;          /* Horizontal Luma/Chroma output positions */
    /* Vertical variables */
    S32    VDinY, VDinC;                /* Vertical distance between 2 consecutive Luma/Chroma samples */
    S32    VPosInYTop, VPosInYBot;      /* Vertical Luma Position in Input Top/Bottom Field */
    S32    VPosInCTop, VPosInCBot;      /* Vertical Chroma Position in Input Top/Bottom Field */
    S32    VPosOutYTop, VPosOutYBot;    /* Vertical Luma Position in Output Top/Bottom Field */
    S32    VPosOutCTop, VPosOutCBot;    /* Vertical Chroma Position in Output Top/Bottom Field */
    UNUSED_PARAMETER(IsFormatRasterNotMacroblock);
    UNUSED_PARAMETER(IsLmuActivated);
    UNUSED_PARAMETER(IsPseudo422FromTopField);

    /* Kind of Init. Take care that both PositionX and PositionY are 16th of pixel !!!  */
    /* so they have to be transformed before use.                                       */
    /* Decimation has already been taken into account in ComputeAndApplyThePolyphaseFilters() */
    Position.XStart = InputRectangle_p->PositionX / 16;
    Position.YStart = InputRectangle_p->PositionY / 16;

    Position.XOffset = DecimXOffset;
    Position.YOffset = DecimYOffset;

    /* ************************  Horizontal calculations: ***************************** */

    /*** Input Position ***/
    if (IsFormat422Not420)
    {
        /* 4:2:2 */
        HPosInY = 0;
        HDinY = 8192;
        HPosInC = 0;
        HDinC = 8192;
    }
    else
    {
        /* 4:2:0 */
        HPosInY = 0;
        HDinY = 8192;
        HPosInC = 0;
        HDinC = 16384;
    }

    /*** Input Position modification due to DECIMATION ***/
    switch (CurrentPictureHorizontalDecimation)
    {
        case STLAYER_DECIMATION_FACTOR_NONE:
        case STLAYER_DECIMATION_MPEG4_P2:
        case STLAYER_DECIMATION_AVS:
            /* No decimation: Positions are unchanged */
            break;

        case STLAYER_DECIMATION_FACTOR_2:
            if (!advanced_decimation)
            {
                /* Normal H2 decimation: */
                HPosInY = HPosInY + HDinY / 2;
                HPosInC = HPosInC + HDinC / 2;
            }
            else
            {
                /* Advanced H2 decimation: */
                HPosInY = HPosInY + (7 * HDinY) / 4;
                HPosInC = HPosInC + (7 * HDinC) / 4;
            }
            break;

        case STLAYER_DECIMATION_FACTOR_4:
            if (!advanced_decimation)
            {
                /* Normal H4 decimation: */
                HPosInY = HPosInY + 3 * HDinY / 8;
                HPosInC = HPosInC + 3 * HDinC / 8;
            }
            else
            {
                /* Advanced H4 decimation: */
                HPosInY = HPosInY + (7 * HDinY) / 8;
                HPosInC = HPosInC + (7 * HDinC) / 8;
            }
            break;
    }

    /*** Normalization of Input Positions (in steps of 1/8192) ***/
    HPosInY = (HPosInY * 8192) / HDinY;
    HPosInC = (HPosInC * 8192) / HDinC;

    /*** Selection of referential origin ***/
    /* HPosInY should have the position '0' because this is on this pixel that we're going to align the output */
    HPosInC = HPosInC - HPosInY;
    HPosInY = 0;

    /*** Output Position (4:2:2 Output) ***/
    HPosOutY = 0;
    HPosOutC = 0;

    /* Compute the Phase and PixelRepeat values */
    GetHorizontalSettings(HPosInY, HPosOutY, &(Phase.LumaHorizontal), &(FirstPixelRepetition.LumaProgressive) );
    GetHorizontalSettings(HPosInC, HPosOutC, &(Phase.ChromaHorizontal), &(FirstPixelRepetition.ChromaProgressive) );

    /* ************************  Vertical calculations: ***************************** */

    /*** Input Position ***/
    if (IsInputScantypeProgressive)
    {
        /* Progressive Input */
#if 0
        if  (IsLmuActivated)
        {
            /* Pseudo 4:2:2 Progressive */
            VDinY      = 16384;
            VDinC      = 16384;
            /* Progressive frame coming from a Top Field */
            VPosInYTop = 0;
            VPosInCTop = 4096;
            /* Progressive frame coming from a Bottom Field */
            VPosInYBot = 8192;
            VPosInCBot = 4096;
        }
        else
#endif
        {
            if (IsFormat422Not420)
            {
                /* 4:2:2 Progressive Input */
                VDinY      = 8192;
                VDinC      = 8192;
                VPosInYTop = 0;
                VPosInCTop = 0;
                /* No Bottom field so we copy the same values than for the Top field */
                VPosInYBot = VPosInYTop;
                VPosInCBot = VPosInCTop;
            }
            else
            {
                /* 4:2:0 Progressive Input */
                VDinY      = 8192;
                VDinC      = 16384;
                VPosInYTop = 0;
                VPosInCTop = 4096;
                /* No Bottom field so we copy the same values than for the Top field */
                VPosInYBot = VPosInYTop;
                VPosInCBot = VPosInCTop;
            }
        }
    }
    else
    {
        /* Interlaced Input */
        if (IsFormat422Not420)
        {
            /* 4:2:2 Interlaced Input */
            VDinY      = 16384;
            VDinC      = 16384;
            VPosInYTop = 0;
            VPosInCTop = 0;
            VPosInYBot = 8192;
            VPosInCBot = 8192;
        }
        else
        {
            /* 4:2:0 Interlaced Input */
            VDinY      = 16384;
            VDinC      = 32768;
            VPosInYTop = 0;
            VPosInCTop = 4096;
            VPosInYBot = 8192;
            VPosInCBot = 20480;
        }
    }

    /*** Input Position modification due to DECIMATION ***/
    switch (CurrentPictureVerticalDecimation)
    {
        case STLAYER_DECIMATION_FACTOR_NONE:
        case STLAYER_DECIMATION_MPEG4_P2:
        case STLAYER_DECIMATION_AVS:
            /* No decimation: Positions are unchanged */
            break;

        case STLAYER_DECIMATION_FACTOR_2:
            /* V2 decimation: */
            VPosInYTop = VPosInYTop + VDinY / 4;
            VPosInYBot = VPosInYBot + VDinY / 4;
            VPosInCTop = VPosInCTop + VDinC / 4;
            VPosInCBot = VPosInCBot + VDinC / 4;
            break;

        case STLAYER_DECIMATION_FACTOR_4:
            /* V4 decimation: */
            VPosInYTop = VPosInYTop + (3 * VDinY) / 8;
            VPosInYBot = VPosInYBot + (3 * VDinY) / 8;
            VPosInCTop = VPosInCTop + (3 * VDinC) / 8;
            VPosInCBot = VPosInCBot + (3 * VDinC) / 8;
            break;
    }

    /*** Normalization of Input Positions (in steps of 1/8192) ***/
    VPosInYTop = (VPosInYTop * 8192) / VDinY;
    VPosInYBot = (VPosInYBot * 8192) / VDinY;
    VPosInCTop = (VPosInCTop * 8192) / VDinC;
    VPosInCBot = (VPosInCBot * 8192) / VDinC;

    /*** Selection of referential origin ***/
    /* VPosInYTop should have the position '0' because this is on this pixel that we're going to align the output */
    VPosInYBot = VPosInYBot - VPosInYTop;
    VPosInCTop = VPosInCTop - VPosInYTop;
    VPosInCBot = VPosInCBot - VPosInYTop;
    VPosInYTop = 0;

    /*** Output Position ***/
    if (IsOutputScantypeProgressive)
    {
        /* 4:2:2 Progressive Output */
        VPosOutYTop = 0;
        VPosOutCTop = 0;
        /* No Bottom field so we copy the same values than for the Top field */
        VPosOutYBot = VPosOutYTop;
        VPosOutCBot = VPosOutCTop;
    }
    else
    {
        /* 4:2:2 Interlaced Output */
        VPosOutYTop = 0;
        VPosOutCTop = 0;
        VPosOutYBot = Increment_p->LumaVertical / 2;
        VPosOutCBot = Increment_p->ChromaVertical / 2;
    }

    /* Compute the Phase and PixelRepeat values */
    if (IsFirstLineTopNotBot)
    {
        /* Usual case: No Field Inversion */
        GetVerticalSettings(VPosInYTop, VPosOutYTop, &(Phase.LumaVerticalTop), &(FirstPixelRepetition.LumaTop) );
        GetVerticalSettings(VPosInCTop, VPosOutCTop, &(Phase.ChromaVerticalTop), &(FirstPixelRepetition.ChromaTop) );
        GetVerticalSettings(VPosInYBot, VPosOutYBot, &(Phase.LumaVerticalBottom), &(FirstPixelRepetition.LumaBottom) );
        GetVerticalSettings(VPosInCBot, VPosOutCBot, &(Phase.ChromaVerticalBottom), &(FirstPixelRepetition.ChromaBottom) );
    }
    else
    {
        /* Field Inversion compensation */
        GetVerticalSettings(VPosInYTop, VPosOutYBot, &(Phase.LumaVerticalTop), &(FirstPixelRepetition.LumaTop) );
        GetVerticalSettings(VPosInCTop, VPosOutCBot, &(Phase.ChromaVerticalTop), &(FirstPixelRepetition.ChromaTop) );
        GetVerticalSettings(VPosInYBot, VPosOutYTop, &(Phase.LumaVerticalBottom), &(FirstPixelRepetition.LumaBottom) );
        GetVerticalSettings(VPosInCBot, VPosOutCTop, &(Phase.ChromaVerticalBottom), &(FirstPixelRepetition.ChromaBottom) );
    }

    MemoryPosition.X1 = 0;
    MemoryPosition.X2 = 0;
    MemoryPosition.Y1Top = 0;
    MemoryPosition.Y1Bottom = 0;
    MemoryPosition.Y2Top = 0;
    MemoryPosition.Y2Bottom = 0;

    /*** Here is the result: ***/
    PositionDefinition_p->FirstPixelRepetition  = FirstPixelRepetition;
    PositionDefinition_p->MemoryPosition        = MemoryPosition;
    PositionDefinition_p->Phase                 = Phase;

    TrDebug(("FPixL:T=%d B=%d  FPixC:T=%d B=%d\r\n",FirstPixelRepetition.LumaTop,FirstPixelRepetition.LumaBottom,FirstPixelRepetition.ChromaTop,FirstPixelRepetition.ChromaBottom));
    TrDebug(("MemPosY1:T=%d B=%d  MemPosY2:T=%d B=%d\r\n",MemoryPosition.Y1Top,MemoryPosition.Y1Bottom,MemoryPosition.Y2Top,MemoryPosition.Y2Bottom));
    TrDebug(("PL:T=%d B=%d  PC:T=%d B=%d\r\n",Phase.LumaVerticalTop,Phase.LumaVerticalBottom,Phase.ChromaVerticalTop,Phase.ChromaVerticalBottom));

} /* End of stlayer_GetPositionDefinition() function. */


/*******************************************************************************
Name        : stlayer_GetMemorySize
Description : Calculate and return the memory size used.
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void stlayer_GetMemorySize(
        STLAYER_Handle_t                    LayerHandle,
        STGXOBJ_Rectangle_t *               InputRectangle_p,
        STGXOBJ_Rectangle_t *               OutputRectangle_p,
        HALLAYER_Increment_t *              Increment_p,
        HALLAYER_Phase_t *                  Phase_p,
        HALLAYER_MemoryPosition_t *         MemoryPosition_p,
        HALLAYER_FirstPixelRepetition_t *   FirstPixelRepetition_p,
        BOOL                                IsInputScantypeProgressive,
        BOOL                                IsOutputScantypeProgressive,
        BOOL                                IsFirstLineTopNotBot,
        BOOL                                IsFormat422Not420,
        BOOL                                IsFormatRasterNotMacroblock,
        HALLAYER_MemorySize_t *             MemorySize_p)
{
    S32 TempS32A;
    S32 TempS32A2;
    S32 TempS32B;
    S32 TempS32B2;
#if (defined WA_GNBvd39528)
    S32 WA_x, WA_w;
#endif /* WA_GNBvd39528 */
    U32 TargetHeightTop;
    U32 TargetHeightBottom;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    U32 SourcePictureWidth;
    U32 SourcePictureHeight;
    BOOL IsPixelRepeatEnable = FALSE;

    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    SourcePictureWidth = InputRectangle_p->Width;
    SourcePictureHeight = InputRectangle_p->Height;

    /* Horizontal calculations: */
    /* Luma */
    TempS32A = OutputRectangle_p->Width * Increment_p->LumaHorizontal + Phase_p->LumaHorizontal;
    TempS32A = 8 + (TempS32A >> 13) - FirstPixelRepetition_p->LumaProgressive - 3 * (1 - (IsPixelRepeatEnable ? 1 : 0));

    if (TempS32A < ((S32)SourcePictureWidth - (S32)(MemoryPosition_p->X1)))
    {
        MemorySize_p->Width1 = TempS32A;
    }
    else
    {
        MemorySize_p->Width1 = SourcePictureWidth - MemoryPosition_p->X1;
    }

#if (defined WA_GNBvd39528)
    /* Workaround luma for GNBvd39528 : Pan/Scan issue on display resize */
    WA_x = (MemoryPosition_p->X1)&0x7;
    WA_w = (MemorySize_p->Width1)&0x7;
    if ((WA_x!=0)
        &&(WA_w!=0)
        &&(((WA_x<4)&&(WA_w>=5))||(WA_x>4))
        &&((WA_x+WA_w)<=8))
    {
        MemorySize_p->Width1 = ((MemorySize_p->Width1)+7)&0xfffffff8;
    }
#endif /* WA_GNBvd39528 */

    /* Chroma */
    if (IsFormatRasterNotMacroblock)
    {
        if (2*(MemoryPosition_p->X1 >> 1) == MemoryPosition_p->X1)
        {
            MemorySize_p->Width2 = (MemorySize_p->Width1 + 1) / 2;
        }
        else
        {
            MemorySize_p->Width2 = MemorySize_p->Width1 / 2;
        }
    }
    else
    {
        TempS32A = OutputRectangle_p->Width * Increment_p->ChromaHorizontal + Phase_p->ChromaHorizontal;
        TempS32A = 8 + (TempS32A >> 13) - FirstPixelRepetition_p->ChromaProgressive - 3 * (1 - (IsPixelRepeatEnable ? 1 : 0));

        TempS32B = SourcePictureWidth + (IsFormat422Not420 ? 1 : 0);
        TempS32B = (TempS32B >> 1) - MemoryPosition_p->X2;

        if (TempS32A < TempS32B)
        {
            MemorySize_p->Width2 = TempS32A;
        }
        else
        {
            MemorySize_p->Width2 = TempS32B;
        }
    }

#if (defined WA_GNBvd39528)
    /* Workaround chroma for GNBvd39528 : Pan/Scan issue on display resize */
    WA_x = (MemoryPosition_p->X2)&0x3;
    WA_w = (MemorySize_p->Width2)&0x3;
    if ((WA_x!=0)
    &&(WA_w!=0)
    &&((WA_x+WA_w)>=2)
    &&((WA_x+WA_w)<=4))
    {
        MemorySize_p->Width2 = ((MemorySize_p->Width2)+3)&0xfffffffc;
    }
#endif /* WA_GNBvd39528 */

    /* Vertical calculations: */
    TargetHeightTop = (OutputRectangle_p->Height + (IsFirstLineTopNotBot ? 1 : 0));
    TargetHeightTop = (TargetHeightTop >> 1);

    TargetHeightBottom = (OutputRectangle_p->Height + 1 - (IsFirstLineTopNotBot ? 1 : 0));
    TargetHeightBottom = (TargetHeightBottom >> 1);

    /* Luma */
    if (IsOutputScantypeProgressive && IsInputScantypeProgressive)
    {
        /* Progressive source and target */
        TempS32A = OutputRectangle_p->Height * Increment_p->LumaVertical + Phase_p->LumaVerticalTop;
        TempS32A = 5 + (TempS32A >> 13) - FirstPixelRepetition_p->LumaTop - 2 * (1 - (IsPixelRepeatEnable ? 1 : 0));

        TempS32B = SourcePictureHeight - MemoryPosition_p->Y1Top;
        if (TempS32A < TempS32B)
        {
            MemorySize_p->Height1Top = TempS32A;
        }
        else
        {
            MemorySize_p->Height1Top = TempS32B;
        }
        MemorySize_p->Height1Bottom = MemorySize_p->Height1Top;
    }
    else if (IsOutputScantypeProgressive && !IsInputScantypeProgressive)
    {
        /* Progressive target and interlaced source */
        TempS32A = OutputRectangle_p->Height * Increment_p->LumaVertical + Phase_p->LumaVerticalTop;
        TempS32A = 5 + (TempS32A >> 13) - FirstPixelRepetition_p->LumaTop - 2 * (1 - (IsPixelRepeatEnable ? 1 : 0));
        TempS32B = (SourcePictureHeight >> 1) - MemoryPosition_p->Y1Top;
        if (TempS32A < TempS32B)
        {
            MemorySize_p->Height1Top = TempS32A;
        }
        else
        {
            MemorySize_p->Height1Top = TempS32B;
        }
        TempS32A = OutputRectangle_p->Height * Increment_p->LumaVertical + Phase_p->LumaVerticalBottom;
        TempS32A = 5 + (TempS32A >> 13) - FirstPixelRepetition_p->LumaBottom - 2 * (1 - (IsPixelRepeatEnable ? 1 : 0));
        TempS32B = (SourcePictureHeight - (MemoryPosition_p->Y1Bottom << 1) + 1) >> 1;
        if (TempS32A < TempS32B)
        {
            MemorySize_p->Height1Bottom = TempS32A;
        }
        else
        {
            MemorySize_p->Height1Bottom = TempS32B;
        }
    }
    else    /* Interlaced target */
    {
        /* Top Field */
        TempS32A = TargetHeightTop * Increment_p->LumaVertical + Phase_p->LumaVerticalTop;
        TempS32A = 5 + (TempS32A >> 13) - FirstPixelRepetition_p->LumaTop - 2 * ((!IsPixelRepeatEnable) ? 1 : 0);
        TempS32B = (SourcePictureHeight + ((!IsInputScantypeProgressive) ? 1 : 0));
        TempS32B = (TempS32B >> (U32) (!IsInputScantypeProgressive)) - MemoryPosition_p->Y1Top;
        if (TempS32A < TempS32B)
        {
            MemorySize_p->Height1Top = TempS32A;
        }
        else
        {
            MemorySize_p->Height1Top = TempS32B;
        }

        /* Bottom Field */
        TempS32A = TargetHeightBottom * Increment_p->LumaVertical + Phase_p->LumaVerticalBottom;
        TempS32A = 5 + (TempS32A >> 13) - FirstPixelRepetition_p->LumaBottom - 2 * ((!IsPixelRepeatEnable) ? 1 : 0);
        TempS32B = (SourcePictureHeight >> (U32) ((!IsInputScantypeProgressive) ? 1 : 0)) - MemoryPosition_p->Y1Bottom;
        if (TempS32A < TempS32B)
        {
            MemorySize_p->Height1Bottom = TempS32A;
        }
        else
        {
            MemorySize_p->Height1Bottom = TempS32B;
        }
    }

    if (IsOutputScantypeProgressive && IsInputScantypeProgressive)
    {
        /* Progressive source and target */
        TempS32A = OutputRectangle_p->Height * Increment_p->ChromaVertical + Phase_p->ChromaVerticalTop;
        TempS32A = 5 + (TempS32A >> 13) - FirstPixelRepetition_p->ChromaTop - 2 * ((!IsPixelRepeatEnable) ? 1 : 0);
        TempS32B = SourcePictureHeight / (2 - (IsFormat422Not420) ? 1 : 0) - MemoryPosition_p->Y2Top;
        if (TempS32A < TempS32B)
        {
            MemorySize_p->Height2Top = TempS32A;
        }
        else
        {
            MemorySize_p->Height2Top = TempS32B;
        }
        MemorySize_p->Height2Bottom = MemorySize_p->Height2Top;
    }
    else if (IsOutputScantypeProgressive && !IsInputScantypeProgressive)
    {
        /* Interlaced source, Progressive target */
        TempS32A = OutputRectangle_p->Height * Increment_p->ChromaVertical + Phase_p->ChromaVerticalTop;
        TempS32A = 5 + (TempS32A >> 13) - FirstPixelRepetition_p->ChromaTop - 2 * ((!IsPixelRepeatEnable) ? 1 : 0);
        TempS32B = (SourcePictureHeight / (2 - (IsFormat422Not420) ? 1 : 0) >> 1) - MemoryPosition_p->Y2Top;
        if (TempS32A < TempS32B)
        {
            MemorySize_p->Height2Top = TempS32A;
        }
        else
        {
            MemorySize_p->Height2Top = TempS32B;
        }
        TempS32A = OutputRectangle_p->Height * Increment_p->ChromaVertical + Phase_p->ChromaVerticalBottom;
        TempS32A = 5 + (TempS32A >> 13) - FirstPixelRepetition_p->ChromaBottom - 2 * ((!IsPixelRepeatEnable) ? 1 : 0);
        TempS32B = (SourcePictureHeight / (2 - (IsFormat422Not420) ? 1 : 0) - (MemoryPosition_p->Y2Bottom << 1) + 1) >> 1;
        if (TempS32A < TempS32B)
        {
            MemorySize_p->Height2Bottom = TempS32A;
        }
        else
        {
            MemorySize_p->Height2Bottom = TempS32B;
        }
    }
    else    /* Interlaced target */
    {
        TempS32A  = TargetHeightTop * Increment_p->ChromaVertical + Phase_p->ChromaVerticalTop;
        TempS32A  = 5 + (TempS32A >> 13) - FirstPixelRepetition_p->ChromaTop - 2 * ((!IsPixelRepeatEnable) ? 1 : 0);
        TempS32A2 = TargetHeightBottom * Increment_p->ChromaVertical + Phase_p->ChromaVerticalBottom;
        TempS32A2 = 5 + (TempS32A2 >> 13) - FirstPixelRepetition_p->ChromaBottom - 2 * ((!IsPixelRepeatEnable) ? 1 : 0);

        if (IsFormat422Not420)
        {
            /* Format 422 or Interlaced Source */
            TempS32B  = SourcePictureHeight + (((!IsInputScantypeProgressive) ? 1 : 0));
            TempS32B  = (TempS32B >> (U32) ((!IsInputScantypeProgressive) ? 1 : 0)) - MemoryPosition_p->Y2Top;
            TempS32B2 = SourcePictureHeight;
            TempS32B2 = (TempS32B2 >> (U32) ((!IsInputScantypeProgressive) ? 1 : 0)) - MemoryPosition_p->Y2Bottom;
            if (TempS32A < TempS32B)
            {
                MemorySize_p->Height2Top = TempS32A;
            }
            else
            {
                MemorySize_p->Height2Top = TempS32B;
            }
            if (TempS32A2 < TempS32B2)
            {
                MemorySize_p->Height2Bottom = TempS32A2;
            }
            else
            {
                MemorySize_p->Height2Bottom = TempS32B2;
            }
        }
        else if (!IsInputScantypeProgressive)
        {
            /* Format 420 and Interlaced source */
            TempS32B  = (SourcePictureHeight >> 1) + 1;
            TempS32B  = (TempS32B >> 1) - MemoryPosition_p->Y2Top;
            TempS32B2 = SourcePictureHeight >> 1;
            TempS32B2 = (TempS32B2 >> 1) - MemoryPosition_p->Y2Bottom;
            if (TempS32A < TempS32B)
            {
                MemorySize_p->Height2Top = TempS32A;
            }
            else
            {
                MemorySize_p->Height2Top = TempS32B;
            }
            if (TempS32A2 < TempS32B2)
            {
                MemorySize_p->Height2Bottom = TempS32A2;
            }
            else
            {
                MemorySize_p->Height2Bottom = TempS32B2;
            }
        }
        else
        {
            /* Format 420 and Progressive source */
            TempS32B  = SourcePictureHeight / 2 - MemoryPosition_p->Y2Top;
            TempS32B2 = SourcePictureHeight / 2 - MemoryPosition_p->Y2Bottom;
            if (TempS32A < TempS32B)
            {
                MemorySize_p->Height2Top = TempS32A;
            }
            else
            {
                MemorySize_p->Height2Top = TempS32B;
            }
            if (TempS32A2 < TempS32B2)
            {
                MemorySize_p->Height2Bottom = TempS32A2;
            }
            else
            {
                MemorySize_p->Height2Bottom = TempS32B2;
            }
        }
    }
}

/*******************************************************************************
Name        : stlayer_SelectHorizontalLumaFilter
Description : Select the luma filter coefficients for the Horizontal ratio according
              to the increment and phase.
Parameters  : - Increment : calculated increment
              - Phase : calculated phase
Assumptions :
Limitations :
Returns     : Loaded coefficients
*******************************************************************************/
HALLAYER_Filter_t stlayer_SelectHorizontalLumaFilter(
            U32 Increment, U32 Phase)
{
    HALLAYER_Filter_t LoadedFilter;

    if ((Increment < 0x2000) || ((Increment == 0x2000) && (Phase != 0x0)))
    {
        /* HF (zoom > 1 => zoom in) */
        LoadedFilter = FILTER_A;
    }
    else if ((Increment == 0x2000) && (Phase == 0))
    {
        /* HF (zoom = 1) */
        LoadedFilter =  FILTER_B;
    }
    else if ((Increment > 0x2000) && (Increment <= 0x2800))
    {
        /* HF (0.80 < zoom out < 1) */
        LoadedFilter = FILTER_Y_C;
    }
    else if ((Increment > 0x2800) && (Increment <= 0x3555))
    {
        /* HF (0.60 < zoom out < 0.80) */
        LoadedFilter = FILTER_Y_D;
    }
    else if ((Increment > 0x3555) && (Increment <= 0x4000))
    {
        /* HF (0.50 < zoom out < 0.60) */
        LoadedFilter = FILTER_Y_E;
    }
    else if ((Increment > 0x4000) && (Increment <= 0x8000))
    {
        /* HF (0.25 < zoom out < 0.50) */
        LoadedFilter = FILTER_Y_F;
    }
    else
    {
        LoadedFilter = FILTER_NO_SET;
    }

#ifdef FORCE_VHSRC_FILTER
    if (ManualFilterSelection)
    {
        /* Force the Loaded Filter */
        LoadedFilter = HorizontalLumaFilter;
    }
    else
    {
        /* Param chosen automatically by the driver. Simply save it */
        HorizontalLumaFilter = LoadedFilter;
    }
#endif

    return(LoadedFilter);
}

/*******************************************************************************
Name        : stlayer_SelectHorizontalChromaFilter
Description : Select the chroma filter coefficients for the Horizontal ratio according
              to the increment and phase.
Parameters  : - Increment : calculated increment
              - Phase : calculated phase
Assumptions :
Limitations :
Returns     : Loaded coefficients
*******************************************************************************/
HALLAYER_Filter_t stlayer_SelectHorizontalChromaFilter(
            U32 Increment, U32 Phase)
{
    HALLAYER_Filter_t LoadedFilter;

    if ( (Increment == 0x1000) && (Phase == 0x0) )
    {
        /* HF (picture zoom = 1 <=> zoom Chroma = 2) */
        LoadedFilter = FILTER_H_CHRX2;
    }
    else if ((Increment < 0x2000) || ((Increment == 0x2000) && (Phase != 0x0)))
    {
        /* HF (zoom > 1 => zoom in) */
        LoadedFilter = FILTER_A;
    }
    else if ((Increment == 0x2000) && (Phase == 0))
    {
        /* HF (zoom = 1) */
        LoadedFilter =  FILTER_B;
    }
    else if ((Increment > 0x2000) && (Increment <= 0x2800))
    {
        /* HF (0.80 < zoom out < 1) */
        LoadedFilter = FILTER_C_C;
    }
    else if ((Increment > 0x2800) && (Increment <= 0x3555))
    {
        /* HF (0.60 < zoom out < 0.80) */
        LoadedFilter = FILTER_C_D;
    }
    else if ((Increment > 0x3555) && (Increment <= 0x4000))
    {
        /* HF (0.50 < zoom out < 0.60) */
        LoadedFilter = FILTER_C_E;
    }
    else if ((Increment > 0x4000) && (Increment <= 0x8000))
    {
        /* HF (0.25 < zoom out < 0.50) */
        LoadedFilter = FILTER_C_F;
    }
    else
    {
        LoadedFilter = FILTER_NO_SET;
    }

#ifdef FORCE_VHSRC_FILTER
    if (ManualFilterSelection)
    {
        /* Force the Loaded Filter */
        LoadedFilter = HorizontalChromaFilter;
    }
    else
    {
        /* Param chosen automatically by the driver. Simply save it */
        HorizontalChromaFilter = LoadedFilter;
    }
#endif

    return(LoadedFilter);
}

/*******************************************************************************
Name        : stlayer_SelectVerticalLumaFilter
Description : Select the luma filter coefficients for the Vertical ratio according to
              the increment and phase.
Parameters  : - Increment : calculated increment
              - Phase : calculated phase
Assumptions :
Limitations :
Returns     : Loaded coefficients
*******************************************************************************/
HALLAYER_Filter_t stlayer_SelectVerticalLumaFilter(
            U32 Increment, U32 PhaseTop, U32 PhaseBot)
{
    HALLAYER_Filter_t LoadedFilter;

    if ( (Increment < 0x2000) ||
         ( (Increment == 0x2000) && ( (PhaseTop != 0x1000) || (PhaseBot != 0x1000) ) ) )
    {
        /* VF (zoom > 1 => zoom in) */
        LoadedFilter = FILTER_A;
    }
    else if ( (Increment == 0x2000) && (PhaseTop == 0x1000) && (PhaseBot == 0x1000) )
    {
        /* VF (zoom = 1) */
        LoadedFilter = FILTER_B;
    }
    else if (((Increment > 0x2000) && Increment <= 0x2800))
    {
        /* VF (0.80 < zoom out  < 1 ) */
        LoadedFilter = FILTER_Y_C;
    }
    else if ((Increment > 0x2800) && (Increment <= 0x3555))
    {
        /* VF (0.60 < zoom out  < 0.80 ) */
        LoadedFilter = FILTER_Y_D;
    }
    else if ((Increment > 0x3555) && (Increment <= 0x4000))
    {
        /* VF (0.50 < zoom out  < 0.60 ) */
        LoadedFilter = FILTER_Y_E;
    }
    else if((Increment > 0x4000) && (Increment <= 0x8000))
    {
        /* VF (0.25 < zoom out  < 0.50 ) */
        LoadedFilter = FILTER_Y_F;
    }
    else
    {
        LoadedFilter = FILTER_NO_SET;
    }

#ifdef FORCE_VHSRC_FILTER
    if (ManualFilterSelection)
    {
        /* Force the Loaded Filter */
        LoadedFilter = VerticalLumaFilter;
    }
    else
    {
        /* Param chosen automatically by the driver. Simply save it */
        VerticalLumaFilter = LoadedFilter;
    }
#endif

    return(LoadedFilter);
}

/*******************************************************************************
Name        : stlayer_SelectVerticalChromaFilter
Description : Select the chroma filter coefficients for the Vertical ratio according to
              the increment and phase.
Parameters  : - Increment : calculated increment
              - Phase : calculated phase
Assumptions :
Limitations :
Returns     : Loaded coefficients
*******************************************************************************/
HALLAYER_Filter_t stlayer_SelectVerticalChromaFilter(
            U32 Increment, U32 PhaseTop, U32 PhaseBot)
{
    HALLAYER_Filter_t LoadedFilter;

    if (Increment == 0x1000)
    {
        /* VF (picture zoom = 1 <=> zoom Chroma = 2) */
        LoadedFilter = FILTER_V_CHRX2;
    }
    else if ( (Increment < 0x2000) ||
              ( (Increment == 0x2000) && ( (PhaseTop != 0x1000) || (PhaseBot != 0x1000) ) ) )
    {
        /* VF (zoom > 1 => zoom in) */
        LoadedFilter = FILTER_A;
    }
    else if ( (Increment == 0x2000)&& (PhaseTop == 0x1000) && (PhaseBot == 0x1000) )
    {
        /* VF (zoom = 1) */
        LoadedFilter = FILTER_B;
    }
    else if (((Increment > 0x2000) && Increment <= 0x2800))
    {
        /* VF (0.80 < zoom out  < 1 ) */
        LoadedFilter = FILTER_C_C;
    }
    else if ((Increment > 0x2800) && (Increment <= 0x3555))
    {
        /* VF (0.60 < zoom out  < 0.80 ) */
        LoadedFilter = FILTER_C_D;
    }
    else if ((Increment > 0x3555) && (Increment <= 0x4000))
    {
        /* VF (0.50 < zoom out  < 0.60 ) */
        LoadedFilter = FILTER_C_E;
    }
    else if((Increment > 0x4000) && (Increment <= 0x8000))
    {
        /* VF (0.25 < zoom out  < 0.50 ) */
        LoadedFilter = FILTER_C_F;
    }
    else
    {
        LoadedFilter = FILTER_NO_SET;
    }

#ifdef FORCE_VHSRC_FILTER
    if (ManualFilterSelection)
    {
        /* Force the Loaded Filter */
        LoadedFilter = VerticalChromaFilter;
    }
    else
    {
        /* Param chosen automatically by the driver. Simply save it */
        VerticalChromaFilter = LoadedFilter;
    }
#endif

    return(LoadedFilter);
}

/*******************************************************************************
Name        : stlayer_WriteHorizontalFilterCoefficientsToMemory
Description : Copy filter coefficients to a given memory address.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_WriteHorizontalFilterCoefficientsToMemory(
            U32 * Memory_p, HALLAYER_Filter_t Filter)
{

    switch(Filter)
    {
        case FILTER_A:
            memcpy(Memory_p, HorizontalCoefficientsA, HFC_NB_COEFS*4);
            break;
        case FILTER_B:
            memcpy(Memory_p, HorizontalCoefficientsB, HFC_NB_COEFS*4);
            break;
        case FILTER_Y_C:
            memcpy(Memory_p, HorizontalCoefficientsLumaC, HFC_NB_COEFS*4);
            break;
        case FILTER_C_C:
            memcpy(Memory_p, HorizontalCoefficientsChromaC, HFC_NB_COEFS*4);
            break;
        case FILTER_Y_D:
            memcpy(Memory_p, HorizontalCoefficientsLumaD, HFC_NB_COEFS*4);
            break;
        case FILTER_C_D:
            memcpy(Memory_p, HorizontalCoefficientsChromaD, HFC_NB_COEFS*4);
            break;
        case FILTER_Y_E:
            memcpy(Memory_p, HorizontalCoefficientsLumaE, HFC_NB_COEFS*4);
            break;
        case FILTER_C_E:
            memcpy(Memory_p, HorizontalCoefficientsChromaE, HFC_NB_COEFS*4);
            break;
        case FILTER_Y_F:
            memcpy(Memory_p, HorizontalCoefficientsLumaF, HFC_NB_COEFS*4);
            break;
        case FILTER_C_F:
            memcpy(Memory_p, HorizontalCoefficientsChromaF, HFC_NB_COEFS*4);
            break;
        case FILTER_H_CHRX2:
            memcpy(Memory_p, HorizontalCoefficientsChromaZoomX2, HFC_NB_COEFS*4);
            break;

        default:
        case FILTER_NO_SET:
        case FILTER_NULL:
        case FILTER_V_CHRX2:
            /* Error case: We load coeffs for not filtering */
            TrError(("\r\nError! Invalid horizontal filter!"));
            memcpy(Memory_p, HorizontalCoefficientsB, HFC_NB_COEFS*4);
            break;
    }

    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : stlayer_WriteVerticalFilterCoefficientsToMemory
Description : Copy filter coefficients to a given memory address.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_WriteVerticalFilterCoefficientsToMemory(
            U32 * Memory_p, HALLAYER_Filter_t Filter)
{

    switch(Filter)
    {
        case FILTER_A:
            memcpy(Memory_p, VerticalCoefficientsA, VFC_NB_COEFS*4);
            break;
        case FILTER_B:
            memcpy(Memory_p, VerticalCoefficientsB, VFC_NB_COEFS*4);
            break;
        case FILTER_Y_C:
            memcpy(Memory_p, VerticalCoefficientsLumaC, VFC_NB_COEFS*4);
            break;
        case FILTER_C_C:
            memcpy(Memory_p, VerticalCoefficientsChromaC, VFC_NB_COEFS*4);
            break;
        case FILTER_Y_D:
            memcpy(Memory_p, VerticalCoefficientsLumaD, VFC_NB_COEFS*4);
            break;
        case FILTER_C_D:
            memcpy(Memory_p, VerticalCoefficientsChromaD, VFC_NB_COEFS*4);
            break;
        case FILTER_Y_E:
            memcpy(Memory_p, VerticalCoefficientsLumaE, VFC_NB_COEFS*4);
            break;
        case FILTER_C_E:
            memcpy(Memory_p, VerticalCoefficientsChromaE, VFC_NB_COEFS*4);
            break;
        case FILTER_Y_F:
            memcpy(Memory_p, VerticalCoefficientsLumaF, VFC_NB_COEFS*4);
            break;
        case FILTER_C_F:
            memcpy(Memory_p, VerticalCoefficientsChromaF, VFC_NB_COEFS*4);
            break;
        case FILTER_V_CHRX2:
            memcpy(Memory_p, VerticalCoefficientsChromaZoomX2, VFC_NB_COEFS*4);
            break;

        default:
        case FILTER_NO_SET:
        case FILTER_NULL:
        case FILTER_H_CHRX2:
            /* Error case: We load coeffs for not filtering */
            TrError(("\r\nError! Invalid Vertical filter!"));
            memcpy(Memory_p, VerticalCoefficientsB, VFC_NB_COEFS*4);
            break;
    }

    return ST_NO_ERROR;
}


#if defined(HAL_VIDEO_LAYER_FILTER_DISABLE) || defined (STLAYER_USE_CRC)
/*******************************************************************************
Name        : stlayer_WriteVerticalNullCoefficientsFilterToMemory
Description : Update the filter coefficients for no filtering
Parameters  : - MemoryPointer : memory address
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
HALLAYER_Filter_t stlayer_WriteVerticalNullCoefficientsFilterToMemory(U32* MemoryPointer)
{
    U8 index;

    /* Vertical */
    for (index = 0 ; index < (VFC_NB_COEFS-1) ; index++)
    {
        MemoryPointer[index] = 0;
     }
    MemoryPointer[VFC_NB_COEFS-1] = 0x40000000;

    return(FILTER_NULL);
}

/*******************************************************************************
Name        : stlayer_WriteHorizontalNullCoefficientsFilterToMemory
Description : Update the filter coefficients for no filtering
Parameters  : - MemoryPointer : memory address
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
HALLAYER_Filter_t stlayer_WriteHorizontalNullCoefficientsFilterToMemory(U32* MemoryPointer)
{
    U8 index;

    /* Horizontal */
    for (index = 0 ; index < (HFC_NB_COEFS-1) ; index++)
    {
        MemoryPointer[index] = 0;
    }

    MemoryPointer[HFC_NB_COEFS-1] = 0x00800000;

    return(FILTER_NULL);
}
#endif /* HAL_VIDEO_LAYER_FILTER_DISABLE || STLAYER_USE_CRC */

#if defined(DVD_SECURED_CHIP)
/*******************************************************************************
Name        : stlayer_LoadCoefficientsFilters
Description : Write all coef filters at the requested address and stores
              address of each filters
Parameters  : - DestinationBaseAddress : base address where to write filters
              - DestinationSize        : size of destination buffer (for checkings)
              - Alignment              : Alignent of filter (e.g 128 or 256)
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/

ST_ErrorCode_t stlayer_LoadCoefficientsFilters(U32* DestinationBaseAddress,
                                               U32  DestinationSize,
                                               U32  Alignment)
{
    stlayer_LumaChroma_CoefficientsPair_t HorizontalFiltersList[] =
                {
                    /* Horizontal Luma/Chroma coefficients A */
                    { HorizontalCoefficientsA, HorizontalCoefficientsA },
                    /* Horizontal Luma/Chroma coefficients B / Zoomx2 */
                    { HorizontalCoefficientsB, HorizontalCoefficientsChromaZoomX2 },
                    /* Horizontal Luma/Chroma coefficients C */
                    { HorizontalCoefficientsLumaC, HorizontalCoefficientsChromaC },
                    /* Horizontal Luma/Chroma coefficients D */
                    { HorizontalCoefficientsLumaD, HorizontalCoefficientsChromaD },
                    /* Horizontal Luma/Chroma coefficients E */
                    { HorizontalCoefficientsLumaE, HorizontalCoefficientsChromaE },
                    /* Horizontal Luma/Chroma coefficients F */
                    { HorizontalCoefficientsLumaF, HorizontalCoefficientsChromaF }
                };
    stlayer_LumaChroma_CoefficientsPair_t VerticalFiltersList[] =
                {
                    /* Vertical Luma/Chroma coefficients A */
                    { VerticalCoefficientsA, VerticalCoefficientsA },
                    /* Vertical Luma/Chroma coefficients B / Zoomx2 */
                    { VerticalCoefficientsB, VerticalCoefficientsChromaZoomX2 },
                    /* Vertical Luma/Chroma coefficients C */
                    { VerticalCoefficientsLumaC, VerticalCoefficientsChromaC },
                    /* Vertical Luma/Chroma coefficients D */
                    { VerticalCoefficientsLumaD, VerticalCoefficientsChromaD },
                    /* Vertical Luma/Chroma coefficients E */
                    { VerticalCoefficientsLumaE, VerticalCoefficientsChromaE },
                    /* Vertical Luma/Chroma coefficients F */
                    { VerticalCoefficientsLumaF, VerticalCoefficientsChromaF }
                };
    U32  NbHorizontalFilters = sizeof(HorizontalFiltersList)/sizeof(HorizontalFiltersList[0]);
    U32  NbVerticalFilters = sizeof(VerticalFiltersList)/sizeof(VerticalFiltersList[0]);
    U32* DestinationEndAddress  = (void*)((U32)DestinationBaseAddress+DestinationSize);
    U32* CurrentFilterAddress = DestinationBaseAddress;
    U32  HorizontalLumaFilterSize = HFC_NB_COEFS*4;
    U32  HorizontalChromaFilterSize = HFC_NB_COEFS*4;
    U32  VerticalLumaFilterSize = VFC_NB_COEFS*4;
    U32  VerticalChromaFilterSize = VFC_NB_COEFS*4;
    U32  FilterIndex = 0;

    /* Check parameters */
    if(DestinationBaseAddress == NULL)
    {
        /* Bad parameter */
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Check if filters have already been loaded */
    if(CoefficientsFiltersInfo.FiltersLoaded == TRUE)
    {
        /* Coefficients filters already loaded */
        /* Filters can be loaded only once otherwise filters addresses would be lost */
        return ST_ERROR_BAD_PARAMETER;
    }

    /* ********************* Load Horizontal Coefficients ********************** */
    for( FilterIndex = 0; FilterIndex < NbHorizontalFilters; FilterIndex++)
    {
         /* Chech if there is enough space in destination buffer */
         if(((U32)CurrentFilterAddress + HorizontalLumaFilterSize + HorizontalChromaFilterSize) > (U32)DestinationEndAddress)
         {
             /* Insufficient space in destination buffer */
             return ST_ERROR_BAD_PARAMETER;
         }

         /* Copy Horizontal Luma filter coefficients */
         memcpy( CurrentFilterAddress, HorizontalFiltersList[FilterIndex].Luma, HorizontalLumaFilterSize);
         /* Copy Horizontal Chroma filter coefficients */
         memcpy( (U32*)((U32)CurrentFilterAddress + HorizontalLumaFilterSize), HorizontalFiltersList[FilterIndex].Chroma, HorizontalChromaFilterSize);

         /* Set the appropriate filter pointer */
         if(HorizontalFiltersList[FilterIndex].Luma == HorizontalCoefficientsA)
         {
             CoefficientsFiltersInfo.HorizontalCoefficientsA_Address = CurrentFilterAddress;
         }
         else if(HorizontalFiltersList[FilterIndex].Luma == HorizontalCoefficientsB)
         {
             CoefficientsFiltersInfo.HorizontalCoefficientsB_ZoomX2_Address = CurrentFilterAddress;
         }
         else if(HorizontalFiltersList[FilterIndex].Luma == HorizontalCoefficientsLumaC)
         {
             CoefficientsFiltersInfo.HorizontalCoefficientsC_Address = CurrentFilterAddress;
         }
         else if(HorizontalFiltersList[FilterIndex].Luma == HorizontalCoefficientsLumaD)
         {
             CoefficientsFiltersInfo.HorizontalCoefficientsD_Address = CurrentFilterAddress;
         }
         else if(HorizontalFiltersList[FilterIndex].Luma == HorizontalCoefficientsLumaE)
         {
             CoefficientsFiltersInfo.HorizontalCoefficientsE_Address = CurrentFilterAddress;
         }
         else /* HorizontalCoefficientsLumaF */
         {
             CoefficientsFiltersInfo.HorizontalCoefficientsF_Address = CurrentFilterAddress;
         }

         /* Compute the aligned address for the next filter */
         CurrentFilterAddress = (U32*)((U32)CurrentFilterAddress + HorizontalLumaFilterSize + HorizontalChromaFilterSize);
         CurrentFilterAddress = NEXT_ALIGNED_ADDRESS( CurrentFilterAddress, Alignment);

    } /* end for on horizontal coef filters */

    /* ********************* Load Vertical Coefficients ********************** */
    for( FilterIndex = 0; FilterIndex < NbVerticalFilters; FilterIndex++)
    {
         /* Chech if there is enough space in destination buffer */
         if(((U32)CurrentFilterAddress + VerticalLumaFilterSize + VerticalChromaFilterSize) > (U32)DestinationEndAddress)
         {
             /* Insufficient space in destination buffer */
             return ST_ERROR_BAD_PARAMETER;
         }

         /* Copy Vertical Luma filter coefficients */
         memcpy( CurrentFilterAddress, VerticalFiltersList[FilterIndex].Luma, VerticalLumaFilterSize);
         /* Copy Vertical Chroma filter coefficients */
         memcpy( (U32*)((U32)CurrentFilterAddress + VerticalLumaFilterSize), VerticalFiltersList[FilterIndex].Chroma, VerticalChromaFilterSize);

         /* Set the appropriate filter pointer */
         if(VerticalFiltersList[FilterIndex].Luma == VerticalCoefficientsA)
         {
             CoefficientsFiltersInfo.VerticalCoefficientsA_Address = CurrentFilterAddress;
         }
         else if(VerticalFiltersList[FilterIndex].Luma == VerticalCoefficientsB)
         {
             CoefficientsFiltersInfo.VerticalCoefficientsB_ZoomX2_Address = CurrentFilterAddress;
         }
         else if(VerticalFiltersList[FilterIndex].Luma == VerticalCoefficientsLumaC)
         {
             CoefficientsFiltersInfo.VerticalCoefficientsC_Address = CurrentFilterAddress;
         }
         else if(VerticalFiltersList[FilterIndex].Luma == VerticalCoefficientsLumaD)
         {
             CoefficientsFiltersInfo.VerticalCoefficientsD_Address = CurrentFilterAddress;
         }
         else if(VerticalFiltersList[FilterIndex].Luma == VerticalCoefficientsLumaE)
         {
             CoefficientsFiltersInfo.VerticalCoefficientsE_Address = CurrentFilterAddress;
         }
         else /* VerticalCoefficientsLumaF */
         {
             CoefficientsFiltersInfo.VerticalCoefficientsF_Address = CurrentFilterAddress;
         }

         /* Compute the aligned address for the next filter */
         CurrentFilterAddress = (U32*)((U32)CurrentFilterAddress + VerticalLumaFilterSize + VerticalChromaFilterSize);
         CurrentFilterAddress = NEXT_ALIGNED_ADDRESS( CurrentFilterAddress, Alignment);

    } /* end for on Vertical coef filters */

    /* All cofficients filters successfully loaded */
    CoefficientsFiltersInfo.FiltersLoaded = TRUE;
    return ST_NO_ERROR;

} /* End of stlayer_LoadCoefficientsFilters() function */

/*******************************************************************************
Name        : stlayer_GetHorizontalCoefficientsFilterAddress
Description : Get the Horizontal filter coefficients for the ratio according
              to the increment and phase.
Parameters  : - FilterAddress : Address of corresponding luma/chroma filter
              - LumaIncrement : calculated luma increment
              - LumaPhase : calculated luma phase
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
ST_ErrorCode_t stlayer_GetHorizontalCoefficientsFilterAddress(
                                       const U32**        FilterAddress,
                                       HALLAYER_Filter_t* LoadedLumaFilter,
                                       HALLAYER_Filter_t* LoadedChromaFilter,
                                       U32                LumaIncrement,
                                       U32                LumaPhase)
{
    if(CoefficientsFiltersInfo.FiltersLoaded == FALSE)
    {
        *LoadedLumaFilter   = FILTER_NO_SET;
        *LoadedChromaFilter = FILTER_NO_SET;
    }
    else if ((LumaIncrement == 0x2000) && (LumaPhase == 0x0))
    {
        /* HF (picture zoom = 1 <=> zoom Chroma = 2) */
        *FilterAddress      = CoefficientsFiltersInfo.HorizontalCoefficientsB_ZoomX2_Address;
        *LoadedLumaFilter   = FILTER_B;
        *LoadedChromaFilter = FILTER_H_CHRX2;
    }
    else if ((LumaIncrement < 0x2000) || ((LumaIncrement == 0x2000) && (LumaPhase != 0x0)))
    {
        /* HF (zoom > 1 => zoom in) */
        *FilterAddress      = CoefficientsFiltersInfo.HorizontalCoefficientsA_Address;
        *LoadedLumaFilter   = FILTER_A;
        *LoadedChromaFilter = FILTER_A;
    }
    else if ((LumaIncrement > 0x2000) && (LumaIncrement <= 0x2800))
    {
        /* HF (0.80 < zoom out < 1) */
        *FilterAddress      = CoefficientsFiltersInfo.HorizontalCoefficientsC_Address;
        *LoadedLumaFilter   = FILTER_Y_C;
        *LoadedChromaFilter = FILTER_C_C;
    }
    else if ((LumaIncrement > 0x2800) && (LumaIncrement <= 0x3555))
    {
        /* HF (0.60 < zoom out < 0.80) */
        *FilterAddress      = CoefficientsFiltersInfo.HorizontalCoefficientsD_Address;
        *LoadedLumaFilter   = FILTER_Y_D;
        *LoadedChromaFilter = FILTER_C_D;
    }
    else if ((LumaIncrement > 0x3555) && (LumaIncrement <= 0x4000))
    {
        /* HF (0.50 < zoom out < 0.60) */
        *FilterAddress      = CoefficientsFiltersInfo.HorizontalCoefficientsE_Address;
        *LoadedLumaFilter   = FILTER_Y_E;
        *LoadedChromaFilter = FILTER_C_E;
    }
    else if ((LumaIncrement > 0x4000) && (LumaIncrement <= 0x8000))
    {
        /* HF (0.25 < zoom out < 0.50) */
        *FilterAddress      = CoefficientsFiltersInfo.HorizontalCoefficientsF_Address;
        *LoadedLumaFilter   = FILTER_Y_F;
        *LoadedChromaFilter = FILTER_C_F;
    }
    else
    {
        *LoadedLumaFilter   = FILTER_NO_SET;
        *LoadedChromaFilter = FILTER_NO_SET;
    }

    return(ST_NO_ERROR);
}
/* end of stlayer_GetHorizontalCoefficientsFilterAddress() */

/*******************************************************************************
Name        : stlayer_GetVerticalCoefficientsFilterAddress
Description : Get the Vertical filter coefficients for the ratio according
              to the increment and phase.
Parameters  : - FilterAddress : Address of corresponding luma/chroma filter
              - LumaIncrement : calculated luma increment
              - LumaPhase : calculated luma phase
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
ST_ErrorCode_t stlayer_GetVerticalCoefficientsFilterAddress(
                                       const U32**        FilterAddress,
                                       HALLAYER_Filter_t* LoadedLumaFilter,
                                       HALLAYER_Filter_t* LoadedChromaFilter,
                                       U32                LumaIncrement,
                                       U32                LumaPhase)
{
    if(CoefficientsFiltersInfo.FiltersLoaded == FALSE)
    {
        *LoadedLumaFilter   = FILTER_NO_SET;
        *LoadedChromaFilter = FILTER_NO_SET;
    }
    else if ((LumaIncrement == 0x2000) && (LumaPhase == 0x1000))
    {
        /* HF (picture zoom = 1 <=> zoom Chroma = 2) */
        *FilterAddress      = CoefficientsFiltersInfo.VerticalCoefficientsB_ZoomX2_Address;
        *LoadedLumaFilter   = FILTER_B;
        *LoadedChromaFilter = FILTER_V_CHRX2;
    }
    else if ((LumaIncrement < 0x2000) || ((LumaIncrement == 0x2000) && (LumaPhase != 0x1000)))
    {
        /* HF (zoom > 1 => zoom in) */
        *FilterAddress      = CoefficientsFiltersInfo.VerticalCoefficientsA_Address;
        *LoadedLumaFilter   = FILTER_A;
        *LoadedChromaFilter = FILTER_A;
    }
    else if ((LumaIncrement > 0x2000) && (LumaIncrement <= 0x2800))
    {
        /* HF (0.80 < zoom out < 1) */
        *FilterAddress      = CoefficientsFiltersInfo.VerticalCoefficientsC_Address;
        *LoadedLumaFilter   = FILTER_Y_C;
        *LoadedChromaFilter = FILTER_C_C;
    }
    else if ((LumaIncrement > 0x2800) && (LumaIncrement <= 0x3555))
    {
        /* HF (0.60 < zoom out < 0.80) */
        *FilterAddress      = CoefficientsFiltersInfo.VerticalCoefficientsD_Address;
        *LoadedLumaFilter   = FILTER_Y_D;
        *LoadedChromaFilter = FILTER_C_D;
    }
    else if ((LumaIncrement > 0x3555) && (LumaIncrement <= 0x4000))
    {
        /* HF (0.50 < zoom out < 0.60) */
        *FilterAddress      = CoefficientsFiltersInfo.VerticalCoefficientsE_Address;
        *LoadedLumaFilter   = FILTER_Y_E;
        *LoadedChromaFilter = FILTER_C_E;
    }
    else if ((LumaIncrement > 0x4000) && (LumaIncrement <= 0x8000))
    {
        /* HF (0.25 < zoom out < 0.50) */
        *FilterAddress      = CoefficientsFiltersInfo.VerticalCoefficientsF_Address;
        *LoadedLumaFilter   = FILTER_Y_F;
        *LoadedChromaFilter = FILTER_C_F;
    }
    else
    {
        *LoadedLumaFilter   = FILTER_NO_SET;
        *LoadedChromaFilter = FILTER_NO_SET;
    }

    return(ST_NO_ERROR);
}
/* end of stlayer_GetVerticalCoefficientsFilterAddress() */

#endif /* defined(DVD_SECURED_CHIP) */

/*******************************************************************************
Name        : stlayer_UpdateDisplayRegisters
Description : Simply write the DISPLAY registers adressed by STLAYER.
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void stlayer_UpdateDisplayRegisters(STLAYER_Handle_t LayerHandle,
        HALLAYER_RegisterMap_t * DisplayRegister_p,
        U8                       EnaHFilterUpdate,
        U8                       EnaVFilterUpdate)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;

    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;

    /* Set periodicly the registers      */
    HAL_WriteDisp32(DISP_CTL        , DisplayRegister_p->RegDISP_CTL);

    if (EnaHFilterUpdate)
    {
        HAL_WriteDisp32(DISP_HFP        , DisplayRegister_p->RegDISP_HFP);
    }
    else
    {
        /* RegDISP_HFP is unchanged */
    }

    if (EnaVFilterUpdate)
    {
        HAL_WriteDisp32(DISP_VFP        , DisplayRegister_p->RegDISP_VFP);
    }
    else
    {
        /* RegDISP_VFP is unchanged */
    }

    HAL_WriteDisp32(DISP_PKZ        , DisplayRegister_p->RegDISP_PKZ );

    HAL_WriteDisp32(DISP_NLZZD_Y    , DisplayRegister_p->RegDISP_NLZZD_Y);
    HAL_WriteDisp32(DISP_NLZZD_C    , DisplayRegister_p->RegDISP_NLZZD_C);
    HAL_WriteDisp32(DISP_PDELTA     , DisplayRegister_p->RegDISP_PDELTA);

    HAL_WriteDisp32(DISP_LUMA_HSRC  , DisplayRegister_p->RegDISP_LUMA_HSRC);
    HAL_WriteDisp32(DISP_LUMA_VSRC  , DisplayRegister_p->RegDISP_LUMA_VSRC);
    HAL_WriteDisp32(DISP_CHR_HSRC   , DisplayRegister_p->RegDISP_CHR_HSRC);
    HAL_WriteDisp32(DISP_CHR_VSRC   , DisplayRegister_p->RegDISP_CHR_VSRC);
    HAL_WriteDisp32(DISP_TARGET_SIZE, DisplayRegister_p->RegDISP_TARGET_SIZE);
    HAL_WriteDisp32(DISP_MA_CTL     , DisplayRegister_p->RegDISP_MA_CTL);
    HAL_WriteDisp32(DISP_LUMA_SIZE  , DisplayRegister_p->RegDISP_LUMA_SIZE);
    HAL_WriteDisp32(DISP_CHR_SIZE   , DisplayRegister_p->RegDISP_CHR_SIZE);

    TrDebug(("XY:%08x\r\n",DisplayRegister_p->RegDISP_LUMA_XY));

} /* End of stlayer_UpdateDisplayRegisters() function. */

#if defined(HW_7100) || defined(HW_7109)
/* PSI feature is only supported by the STi7100 and the STi7109  */

/*******************************************************************************
Name        : InitFilter
Description : Initialize the filtering access device.
Parameters  : Layerhandle
Assumptions : LayerHandle is valid.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t InitFilter (const STLAYER_Handle_t LayerHandle)
{
    U32                             Count;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    STLAYER_VideoFilteringParameters_t  FilterParams;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   =
            (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Initialize default Array parameters.     */
    for (Count = 0; Count < STLAYER_MAX_VIDEO_FILTER_POSITION; Count ++)
    {
        LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[Count].VideoFiltering
            = Count;
        LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[Count].VideoFilteringControl
            = STLAYER_DISABLE;
    }

    /* Initialize overall activity mask for this video layer.   */
    LayerCommonData_p->PSI_Parameters.ActivityMask = 0;

    FilterParams.BCParameters = DefaultBCSettings[STLAYER_DISABLE];

    SetBCMode(LayerHandle, &FilterParams);
    memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_BC].VideoFilteringParameters.BCParameters),
           &(DefaultBCSettings[STLAYER_DISABLE]),
           sizeof(STLAYER_BrightnessContrastParameters_t));

    FilterParams.SatParameters = DefaultSatSettings[STLAYER_DISABLE];
    SetSaturationMode(LayerHandle, &FilterParams);
    memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_CHROMA_SAT].VideoFilteringParameters.SatParameters),
           &(DefaultSatSettings[STLAYER_DISABLE]),
           sizeof(STLAYER_SatParameters_t));

    FilterParams.TintParameters = DefaultTintSettings[STLAYER_DISABLE];
    SetTintMode(LayerHandle, &FilterParams);
    memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_CHROMA_TINT].VideoFilteringParameters.TintParameters),
           &(DefaultTintSettings[STLAYER_DISABLE]),
           sizeof(STLAYER_TintParameters_t));

        /* No error to report. */
    return(ErrorCode);

}

/*******************************************************************************
Name        : GetViewportPsiParameters
Description : Get all required PSI parameters.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t GetViewportPsiParameters(const STLAYER_Handle_t LayerHandle,
        STLAYER_PSI_t * const ViewportPSI_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    if (ViewportPSI_p->VideoFiltering >= STLAYER_MAX_VIDEO_FILTER_POSITION)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if ( (ViewportPSI_p->VideoFiltering != STLAYER_VIDEO_CHROMA_TINT) &&
         (ViewportPSI_p->VideoFiltering != STLAYER_VIDEO_CHROMA_SAT) &&
         (ViewportPSI_p->VideoFiltering != STLAYER_VIDEO_LUMA_BC))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    *ViewportPSI_p =LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[ViewportPSI_p->VideoFiltering];

    return(ST_NO_ERROR);

} /* End of GetViewportPsiParameters() function */

/*******************************************************************************
Name        : SetViewportPsiParameters
Description : Set all required PSI parameters (if enable) and store its
              parameters.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t SetViewportPsiParameters(const STLAYER_Handle_t LayerHandle,
        const STLAYER_PSI_t * const ViewportPSI_p)
{
    HALLAYER_LayerProperties_t *            LayerProperties_p;
    HALLAYER_LayerCommonData_t *            LayerCommonData_p;
    STLAYER_VideoFilteringParameters_t     FilterParamsToApply;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    ST_ErrorCode_t(*FilterFct_p)(const STLAYER_Handle_t FilterFct_LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);


    /* init Layer and viewport properties.  */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Test input parameters.               */
    if ( (ViewportPSI_p->VideoFiltering >= STLAYER_MAX_VIDEO_FILTER_POSITION) ||
         (ViewportPSI_p->VideoFilteringControl > STLAYER_ENABLE_MANUAL) )
    {
        /* Bad parameters. Return immediately. */
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (ViewportPSI_p->VideoFiltering)
    {
        case STLAYER_VIDEO_CHROMA_TINT :
            FilterFct_p = SetTintMode;
            FilterParamsToApply.TintParameters = DefaultTintSettings[ViewportPSI_p->VideoFilteringControl];

            break;
        case STLAYER_VIDEO_CHROMA_SAT :
            FilterFct_p = SetSaturationMode;
            FilterParamsToApply.SatParameters = DefaultSatSettings[ViewportPSI_p->VideoFilteringControl];

            break;
        case STLAYER_VIDEO_LUMA_BC :
            FilterFct_p = SetBCMode;
            FilterParamsToApply.BCParameters = DefaultBCSettings[ViewportPSI_p->VideoFilteringControl];

            break;

        case STLAYER_VIDEO_CHROMA_AUTOFLESH :
        case STLAYER_VIDEO_CHROMA_GREENBOOST :
        case STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT :
        case STLAYER_VIDEO_LUMA_PEAKING :
        case STLAYER_VIDEO_LUMA_DCI :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
        default :
            /* Unknown case. Return corresponding error.    */
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    if (ViewportPSI_p->VideoFilteringControl == STLAYER_ENABLE_MANUAL)
    {
        FilterParamsToApply = ViewportPSI_p->VideoFilteringParameters;
    }

    ErrorCode = FilterFct_p (LayerHandle, &FilterParamsToApply);

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Don't know which field was filled in the video filtering parameters union, so copy all */
        memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[ViewportPSI_p->VideoFiltering].VideoFilteringParameters.AutoFleshParameters),
               &FilterParamsToApply,
               sizeof(STLAYER_VideoFilteringParameters_t));

        LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[ViewportPSI_p->VideoFiltering].VideoFilteringControl
                = ViewportPSI_p->VideoFilteringControl;
    }
    else
    {
        /* Wrong operation. Restore previous parameters.*/
        FilterParamsToApply = LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[ViewportPSI_p->VideoFiltering].VideoFilteringParameters;
        FilterFct_p (LayerHandle, &FilterParamsToApply);
    }

    /* Return current error code. */
    return(ErrorCode);

} /* End of SetViewportPsiParameters() function */


/*******************************************************************************
Name        : SetTintMode
Description : Set the Peaking (Horizontal and Vertical) and SinX/X compensation
              parameters.
Parameters  : - Layerhandle,
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetTintMode (const STLAYER_Handle_t LayerHandle,
        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    U32 Temp32,TempT32;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (FilterParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Temp32 = HAL_Read32((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_CTL);

    /* - DIS_AF_TINT : Tint correction control value with ->            */
    /*                 0=OFF. -32=min and +31=MAX (signed with 6 bits)  */

    if (FilterParams_p->TintParameters.TintRotationControl != 0)
    {
        if (FilterParams_p->TintParameters.TintRotationControl > 0)
        {
            /* ((percentage of filter activity) * (Main filter activity)) / 100 */
            TempT32 = ((FilterParams_p->TintParameters.TintRotationControl * (GAM_VIDn_TINT_MASK >> 3)) / 100);
            /* Shift to get the value to be written */
#ifdef WA_SHIFT_TINT
            TempT32 <<= 2;
#endif
        }
        else
        {
            /* ((percentage of filter activity) * (Main filter activity)) / 100 */
            TempT32 = (((-1 * FilterParams_p->TintParameters.TintRotationControl) * (GAM_VIDn_TINT_MASK >> 3)) / 100);
            /* Negative TintRotationControl so add 1 as MSB */
            TempT32 += 0x20;
            /* Shift to get the value to be written */
#ifdef WA_SHIFT_TINT
            TempT32 <<= 2;
#endif
        }
        /* Tint filter enable.                            */
        HAL_WriteGam32(GAM_VIDn_TINT,TempT32 & GAM_VIDn_TINT_MASK);
        /* Enable overall Tint filter.                   */
        HAL_WriteGam32(GAM_VIDn_CTL, (Temp32 | GAM_VIDn_CTL_PSI_TINT) & GAM_VIDn_CTL_MASK);
    }
    else
    {
        /* Tint filter disable.                           */
        HAL_WriteGam32(GAM_VIDn_TINT, GAM_VIDn_TINT_RST);
        /* Disable overall Tint filter if necessary. */
        if ((LayerCommonData_p->PSI_Parameters.ActivityMask & (1<<STLAYER_VIDEO_CHROMA_TINT)) == 0)
        {
            /* Tint is not activated. Disable Tint filter.        */
            HAL_WriteGam32(GAM_VIDn_CTL, (Temp32 & !GAM_VIDn_CTL_PSI_TINT) & GAM_VIDn_CTL_MASK);
        }
    }

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetTintMode() function */

/*******************************************************************************
Name        : SetSaturationMode
Description : Set the Saturation boost value. If Saturation is disabled, default
              value will be loaded.
Parameters  : - Layerhandle,
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetSaturationMode (const STLAYER_Handle_t LayerHandle,
        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    U32 Temp32,TempS32;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (FilterParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Temp32 = HAL_Read32((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_CTL);

    if (FilterParams_p->SatParameters.SaturationGainControl != 0)
    {
        FilterParams_p->SatParameters.SaturationGainControl += 100;
        /* ((percentage of filter activity) * (Main filter activity)) / 100 */
        TempS32 = ((FilterParams_p->SatParameters.SaturationGainControl * (GAM_VIDn_SAT_MASK >> 3)) / 100);
        /* Shift to get the value to be written */
        TempS32 <<= 2;
        /* Saturation filter enable.                            */
        HAL_WriteGam32(GAM_VIDn_SAT, TempS32 & GAM_VIDn_SAT_MASK);
        /* Enable overall Saturation filter.               */
        HAL_WriteGam32(GAM_VIDn_CTL, (Temp32 | GAM_VIDn_CTL_PSI_SAT) & GAM_VIDn_CTL_MASK);
        FilterParams_p->SatParameters.SaturationGainControl -= 100;
    }
    else
    {
        /* Saturation filter disable.                           */
        HAL_WriteGam32(GAM_VIDn_SAT, GAM_VIDn_SAT_RST);
        /* Disable overall Saturation filter if necessary. */
        if ((LayerCommonData_p->PSI_Parameters.ActivityMask & (1<<STLAYER_VIDEO_CHROMA_SAT)) == 0)
        {
            /* Saturation is not activated. Disable Saturation filter.        */
            HAL_WriteGam32(GAM_VIDn_CTL, (Temp32 & !GAM_VIDn_CTL_PSI_SAT) & GAM_VIDn_CTL_MASK);
        }
    }

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetSaturationMode() function */

/*******************************************************************************
Name        : SetBCMode
Description : Performs modifications on luminance dynamic range and intensity. If contrast and brightness are disabled, default
              value will be loaded.
Parameters  : - Layerhandle,
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetBCMode (const STLAYER_Handle_t LayerHandle,
        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    U32 Temp32,TempB32,TempC32;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (FilterParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Temp32 = HAL_Read32((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_CTL);

    if (FilterParams_p->BCParameters.BrightnessGainControl != 0)
    {
        if (FilterParams_p->BCParameters.BrightnessGainControl > 0)
        {
            /* ((percentage of filter activity) * (Main filter activity)) / 100 */
            TempB32 = ((FilterParams_p->BCParameters.BrightnessGainControl * (GAM_VIDn_B_MASK >> 1)) / 100);
        }
        else
        {
            /* ((percentage of filter activity) * (Main filter activity)) / 100 */
            TempB32 = (((100 + FilterParams_p->BCParameters.BrightnessGainControl) * (GAM_VIDn_B_MASK >> 1)) / 100);
            /* Negative BrightnessGainControl so add 1 as MSB */
            TempB32 += 0x80;
        }

        if (FilterParams_p->BCParameters.ContrastGainControl != 50)
        {
            /* ((percentage of filter activity) * (Main filter activity)) / 100 */
            TempC32 = ((FilterParams_p->BCParameters.ContrastGainControl * (GAM_VIDn_C_MASK >> 8)) / 100);
            /* Shift to get the value to be written */
            TempC32 <<= 8;
            /* contrast filter enable.                            */
            HAL_WriteGam32(GAM_VIDn_BC, (TempC32 & GAM_VIDn_C_MASK)|(TempB32 & GAM_VIDn_B_MASK ));
        }
        else
        {
            /* brightness filter enable.                            */
            HAL_WriteGam32(GAM_VIDn_BC, (TempB32 & GAM_VIDn_B_MASK )|GAM_VIDn_BC_RST);
        }
        /* Enable overall contrast and brightness filter.               */
        HAL_WriteGam32(GAM_VIDn_CTL, (Temp32 | GAM_VIDn_CTL_PSI_BC) & GAM_VIDn_CTL_MASK);
    }
    else
    {
        if (FilterParams_p->BCParameters.ContrastGainControl != 50)
        {
            /* ((percentage of filter activity) * (Main filter activity)) / 100 */
            TempC32 = ((FilterParams_p->BCParameters.ContrastGainControl * (GAM_VIDn_C_MASK >> 8)) / 100);
            /* Shift to get the value to be written */
            TempC32 <<= 8;
            /* contrast filter enable.                            */
            HAL_WriteGam32(GAM_VIDn_BC, (TempC32 & GAM_VIDn_C_MASK));
            /* Enable overall contrast and brightness filter.               */
            HAL_WriteGam32(GAM_VIDn_CTL, (Temp32 | GAM_VIDn_CTL_PSI_BC) & GAM_VIDn_CTL_MASK);
        }
        else
        {
            /* contrast and brightness filter disable.                           */
            HAL_WriteGam32(GAM_VIDn_BC, GAM_VIDn_BC_RST);
            /* Disable overall contrast and brightness filter if necessary. */
            if ((LayerCommonData_p->PSI_Parameters.ActivityMask & (1<<STLAYER_VIDEO_LUMA_BC)) == 0)
            {
                /* Disable contrast and brightness filter.        */
                HAL_WriteGam32(GAM_VIDn_CTL, (Temp32 & !GAM_VIDn_CTL_PSI_BC) & GAM_VIDn_CTL_MASK);
            }
        }
    }

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetBCMode() function */
#endif /* defined(HW_7100) || defined(HW_7109) */

/* End of hv_tools.c */
