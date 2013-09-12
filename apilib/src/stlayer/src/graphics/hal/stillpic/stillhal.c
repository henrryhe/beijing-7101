/*******************************************************************************

File name   : stillhal.c

Description : It provides the STILL Hardware abstraction layer

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                 Name
----               ------------                                 ----
December 2000       Creation                                    Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "lay_stil.h"
#include "still_cm.h"
#include "still_vp.h"
#include "stillhal.h"

/* Constants ---------------------------------------------------------------- */

/* corrections (computed on 5516 / pal / ntsc) */
/* good to be checked on 5510, 5512, 5514, 5517 */
#ifdef HW_5517
#define STILL_PAL_HORIZONTAL_START  (-8)
#define STILL_PAL_VERTICAL_START    (-2)
#define STILL_NTSC_HORIZONTAL_START (-12)
#define STILL_NTSC_VERTICAL_START   (-2)
#else
#define STILL_PAL_HORIZONTAL_START  (-7)
#define STILL_PAL_VERTICAL_START    (-2)
#define STILL_NTSC_HORIZONTAL_START (-11)
#define STILL_NTSC_VERTICAL_START   (-2)
#endif
#define XOFFSET  stlayer_still_context.XOffset + stlayer_still_context.XStart

#define YOFFSET  stlayer_still_context.YOffset + stlayer_still_context.YStart

#define BitmapAdd(Add) ((U32)Add\
             - (U32)stlayer_still_context.VirtualMapping.VirtualBaseAddress_p)

/* Functions prototypes ----------------------------------------------------- */

static void ststill_GetConfiguration(U8 *DcfValue_p);
static void ststill_SetConfiguration(U8  DcfValue);

/* functions ---------------------------------------------------------------- */

void stlayer_StillHardUpdate(void)
{
    U8 DcfValue;
    U32 Value;
    U32 TopPointer = 0, BotPointer = 0,FineXInput;
    U32 XDO,XDS,YDO,YDS;
    U32 Alpha,Width,Lsr;

    if ((stlayer_still_context.OutputEnable == TRUE)
            &&(stlayer_still_context.ViewPort.Enabled == TRUE))
    {
        /* Mix Weight */
        Alpha = (stlayer_still_context.ViewPort.Alpha * 2);
        if (Alpha !=0)
        {
            Alpha -=1;
        }
        Alpha = 255 - Alpha;

        if (stlayer_still_context.ViewPort.Bitmap.BitmapType
                    == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM)
        {
            Value=(U32)BitmapAdd(stlayer_still_context.ViewPort.Bitmap.Data1_p);
            Value+=((stlayer_still_context.ViewPort.InputRectangle.PositionY/ 2)
                        *(stlayer_still_context.ViewPort.Bitmap.Pitch));
            Value += 2 /* nb bytes per pixel */
                    * (stlayer_still_context.ViewPort.InputRectangle.PositionX);
            if(stlayer_still_context.ViewPort.InputRectangle.PositionY % 2 == 0)
            {
                TopPointer = Value;
            }
            else
            {
                BotPointer =Value + stlayer_still_context.ViewPort.Bitmap.Pitch;
            }
            Value=(U32)BitmapAdd(stlayer_still_context.ViewPort.Bitmap.Data2_p);
            Value+=((stlayer_still_context.ViewPort.InputRectangle.PositionY/2)
                    *(stlayer_still_context.ViewPort.Bitmap.Pitch));
            Value += 2 /* nb bytes per pixel */
                    * (stlayer_still_context.ViewPort.InputRectangle.PositionX);
            if(stlayer_still_context.ViewPort.InputRectangle.PositionY % 2 == 0)
            {
                BotPointer = Value;
            }
            else
            {
                TopPointer = Value;
            }
            /* Set Width */
            Width = stlayer_still_context.ViewPort.Bitmap.Pitch / 16 / 2;
        }
        else
        {
            Value=(U32)BitmapAdd(stlayer_still_context.ViewPort.Bitmap.Data1_p);
            Value += (stlayer_still_context.ViewPort.Bitmap.Pitch)
                * (stlayer_still_context.ViewPort.InputRectangle.PositionY);
            Value += 2 /* nb bytes per pixel */
                * (stlayer_still_context.ViewPort.InputRectangle.PositionX);
            TopPointer = Value;
            BotPointer = Value + stlayer_still_context.ViewPort.Bitmap.Pitch;

            /* Set Width */
            Width = stlayer_still_context.ViewPort.Bitmap.Pitch / 16;
        }
        TopPointer = TopPointer - (TopPointer % 128);
        TopPointer = TopPointer / 32;
        TopPointer = (TopPointer & TDL_TEVENP24_MASK);

        BotPointer = BotPointer - (BotPointer % 128);
        BotPointer = BotPointer / 32;
        BotPointer = (BotPointer & TDL_TODDP24_MASK);

        /* Set PanScan Vector */
        FineXInput=stlayer_still_context.ViewPort.InputRectangle.PositionX % 64;
        FineXInput = FineXInput << TDL_SCN_PAN_POS;

        /* Set XDO */
        XDO = stlayer_still_context.ViewPort.OutputRectangle.PositionX +XOFFSET;
        switch(stlayer_still_context.FrameRate)
        {
            case 25000: /* PAL */
            case 50000: /* PAL */
                XDO += STILL_PAL_HORIZONTAL_START;
                break;
            default:
                XDO += STILL_NTSC_HORIZONTAL_START;
                break;
        }
        /* force even value to avoid 'blue effect' / inter-action with video */
        XDO &= 0xfffffffe;

        /* Set XDS */
        XDS = XDO + stlayer_still_context.ViewPort.OutputRectangle.Width -1;
        /* force even value to avoid a green line at the right of the bmp */
        XDS &= 0xfffffffe;

        /* Set YDO */
        YDO = stlayer_still_context.ViewPort.OutputRectangle.PositionY +YOFFSET;
        switch(stlayer_still_context.FrameRate)
        {
            case 25000: /*PAL*/
            case 50000: /*PAL*/
                YDO += STILL_PAL_VERTICAL_START;
                break;
            default:
                YDO += STILL_NTSC_VERTICAL_START;
                break;
        }
        YDO = YDO / 2;

        /* Set YDS */
        YDS = stlayer_still_context.ViewPort.OutputRectangle.PositionY +YOFFSET;
        YDS += stlayer_still_context.ViewPort.OutputRectangle.Height -1;
        YDS /= 2;

        /* Reading the TDL configuration register */
        ststill_GetConfiguration(&DcfValue);
        DcfValue = DcfValue | TDL_DCF_TEVD;

        /* Set HScale */
        if(stlayer_still_context.ViewPort.OutputRectangle.Width
                != stlayer_still_context.ViewPort.InputRectangle.Width)
        {
            Lsr = (stlayer_still_context.ViewPort.InputRectangle.Width - 4)* 256
                   / (stlayer_still_context.ViewPort.OutputRectangle.Width - 1);
            DcfValue = DcfValue & ( ~TDL_DCF_DSR );
        }
        else
        {
            Lsr = 0;
            /* Disable SRC */
            DcfValue = DcfValue | ( TDL_DCF_DSR );
        }

        /* Set VScaling */
        if(stlayer_still_context.ViewPort.OutputRectangle.Height
                == stlayer_still_context.ViewPort.InputRectangle.Height)
        {
              DcfValue = DcfValue & ( ~(TDL_DCF_UDS | TDL_DCF_UND) );
        }
        else if (2 * stlayer_still_context.ViewPort.OutputRectangle.Height
                == stlayer_still_context.ViewPort.InputRectangle.Height)
        {
              DcfValue = DcfValue | TDL_DCF_UDS;
              DcfValue = DcfValue & (~TDL_DCF_UND);
        }
        else if (stlayer_still_context.ViewPort.OutputRectangle.Height
                == 2 * stlayer_still_context.ViewPort.InputRectangle.Height)
        {
              DcfValue = DcfValue | (TDL_DCF_UDS | TDL_DCF_UND) ;
        }

        /* Writing the TDL register */
        ststill_SetConfiguration(DcfValue);
        ststill_WriteReg8(VID_MWS, Alpha);
        ststill_WriteReg24(TDL_TOP24, TopPointer);
        ststill_WriteReg24(TDL_TEP24, BotPointer);
        ststill_WriteReg8(TDL_TDW, Width);
        ststill_WriteReg16(TDL_XDO16,XDO);
        ststill_WriteReg16(TDL_XDS16, XDS);
        ststill_WriteReg16(TDL_YDO16, YDO);
        ststill_WriteReg16(TDL_YDS16, YDS);
        ststill_WriteReg16(TDL_SCN16, FineXInput );
        interrupt_lock();
        ststill_WriteReg8(TDL_LSR1, (U8)((Lsr >> 8) & TDL_LSR1_MASK));
        ststill_WriteReg8(TDL_LSR0, (U8)(Lsr & TDL_LSR0_MASK));
        interrupt_unlock();

        return;
    }
    else
    {
        stlayer_StillHardTerm();
        ststill_GetConfiguration(&DcfValue);
        DcfValue = DcfValue & ~TDL_DCF_TEVD;
        ststill_SetConfiguration(DcfValue);
    }
}


/* -------------------------------------------------------------------------- */
void stlayer_StillHardInit(void)
{
    U8  DcfValue;
    U32 Value;

    /* Reading the TDL configuration register */
    ststill_GetConfiguration(&DcfValue);
    /* Enable TDL display */
    DcfValue = (DcfValue | TDL_DCF_DSR) & (~(TDL_DCF_FMT));
    ststill_SetConfiguration(DcfValue);

    /* Offset to Unroll */
    Value = 511;
    Value &= TDL_SWT16_MASK;
    ststill_WriteReg16(TDL_SWT16,Value);

    /* Background = black =>  Initialization done by mixer */
/*    ststill_WriteReg8(BCK_Y, 16);*/
/*    ststill_WriteReg8(BCK_U, 128);*/
/*    ststill_WriteReg8(BCK_V, 128);*/


    /* Set Scan Vector */
    ststill_WriteReg16(TDL_SCN16, 0);
}

/* -------------------------------------------------------------------------- */
void stlayer_StillHardTerm(void)
{
    ststill_WriteReg8(VID_MWS, 0);
    ststill_WriteReg24(TDL_TEP24, 0);
    ststill_WriteReg24(TDL_TOP24, 0);
    ststill_WriteReg8(TDL_TDW, 0);
    ststill_WriteReg16(TDL_XDO16,0);
    ststill_WriteReg16(TDL_XDS16, 0);
    ststill_WriteReg16(TDL_YDO16, 0);
    ststill_WriteReg16(TDL_YDS16, 0);
}
/* -------------------------------------------------------------------------- */

static void ststill_GetConfiguration(U8 *DcfValue_p)
{
    *DcfValue_p = ststill_ReadReg8(TDL_DCF);
    *DcfValue_p &= TDL_DCF_MASK;
    return;
}

/* -------------------------------------------------------------------------- */

static void ststill_SetConfiguration(U8 DcfValue)
{
    DcfValue &= TDL_DCF_MASK;
    ststill_WriteReg8(TDL_DCF, DcfValue);
    return;
}


/* -------------------------------------------------------------------------- */
