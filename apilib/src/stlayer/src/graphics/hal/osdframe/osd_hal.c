/*******************************************************************************

File name   : osd_hal.c

Description : Hardware Abstraction Layer

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                 Name
----               ------------                                 ----
2000-12-20          Creation                                    Michel Bruant


*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "osd_hal.h"
#include "osd_vp.h"
#include "osd_cm.h"
#include "stsys.h"
#include <stdlib.h>
#include <string.h>

/* Macros ------------------------------------------------------------------- */

#define ADD         stlayer_osd_context.stosd_BaseAddress
#define XOFFSET     stlayer_osd_context.XOffset + stlayer_osd_context.XStart
#define YOFFSET     stlayer_osd_context.YOffset + stlayer_osd_context.YStart

#define DeviceAdd(Add) ((U32)Add \
      + (U32)stlayer_osd_context.VirtualMapping.PhysicalAddressSeenFromDevice_p\
      - (U32)stlayer_osd_context.VirtualMapping.PhysicalAddressSeenFromCPU_p)

#define BitmapAdd(Add) \
      STAVMEM_VirtualToDevice(Add,&stlayer_osd_context.VirtualMapping)

/* Constants ---------------------------------------------------------------- */
/* corrections (computed on 5516 / pal / ntsc) */
/* must be checked on 5514 and 5517 */
#if defined (HW_5516)
#define OSD_PAL_HORIZONTAL_START  (0)
#define OSD_PAL_VERTICAL_START    (0)
#define OSD_NTSC_HORIZONTAL_START (-4)
#define OSD_NTSC_VERTICAL_START   (0)
#elif defined (HW_5517)
#define OSD_PAL_HORIZONTAL_START  (-1)
#define OSD_PAL_VERTICAL_START    (0)
#define OSD_NTSC_HORIZONTAL_START (-5)
#define OSD_NTSC_VERTICAL_START   (0)
#else
#define OSD_PAL_HORIZONTAL_START  (0)
#define OSD_PAL_VERTICAL_START    (0)
#define OSD_NTSC_HORIZONTAL_START (-4)
#define OSD_NTSC_VERTICAL_START   (0)

#endif

/* Functions ---------------------------------------------------------------- */

static U32 ComputeMQEP(stosd_ViewportDesc *);
static ST_ErrorCode_t stosd_ResetActiveSignalMode(void);
static ST_ErrorCode_t stosd_SetActiveSignalDelay(U8 Delay);
ST_ErrorCode_t LAYEROSD_EnableViewPortFilter(void);
ST_ErrorCode_t LAYEROSD_DisableViewPortFilter(void);

/*******************************************************************************
Name        : stlayer_HardUpdateViewportList
Description : Write headers according to headers images
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_HardUpdateViewportList(BOOL VSyncIsTop)
{
    stosd_ViewportDesc *    CurrentViewport_p;
    osd_RegionHeader_t *    CurrentHeader_p;

    if (VSyncIsTop == TRUE) /* then update Bot list */
    {
        /* Write the start pointer */
        STSYS_WriteRegDev8((void *)(ADD + OSD_OBP),
                    (stlayer_osd_context.OBP_Image >> 8) & 0xFF);
        STSYS_WriteRegDev8((void *)(ADD + OSD_OBP),
                    stlayer_osd_context.OBP_Image & 0xFF);
        CurrentViewport_p = stlayer_osd_context.FirstViewPortLinked;

        /* Scan the viewports list and write the headers */
        while(CurrentViewport_p != 0)
        {
            CurrentHeader_p = CurrentViewport_p->Bot_header;
            STSYS_WriteRegMem32BE((void*)&CurrentHeader_p->word1,
                                 CurrentViewport_p->HardImage1);
            STSYS_WriteRegMem32BE((void*)&CurrentHeader_p->word2,
                                 CurrentViewport_p->HardImage2Bot);
            STSYS_WriteRegMem32BE((void*)&CurrentHeader_p->word3,
                                 CurrentViewport_p->HardImage3Bot);
            STSYS_WriteRegMem32BE((void*)&CurrentHeader_p->word4,
                                 CurrentViewport_p->HardImage4Bot);
            CurrentViewport_p = CurrentViewport_p->Next_p;
        }/* end while */
    }
    else /* Then update the Top list */
    {
        /* Write the start pointer */
        STSYS_WriteRegDev8((void *)(ADD + OSD_OTP),
                    (stlayer_osd_context.OTP_Image >> 8) & 0xFF);
        STSYS_WriteRegDev8((void *)(ADD + OSD_OTP),
                    stlayer_osd_context.OTP_Image & 0xFF);
        CurrentViewport_p = stlayer_osd_context.FirstViewPortLinked;

        /* Scan the viewports list and write the headers */
        while(CurrentViewport_p != 0)
        {
            CurrentHeader_p = CurrentViewport_p->Top_header;
            STSYS_WriteRegMem32BE((void*)&CurrentHeader_p->word1,
                                 CurrentViewport_p->HardImage1);
            STSYS_WriteRegMem32BE((void*)&CurrentHeader_p->word2,
                                 CurrentViewport_p->HardImage2Top);
            STSYS_WriteRegMem32BE((void*)&CurrentHeader_p->word3,
                                 CurrentViewport_p->HardImage3Top);
            STSYS_WriteRegMem32BE((void*)&CurrentHeader_p->word4,
                                 CurrentViewport_p->HardImage4Top);
            CurrentViewport_p = CurrentViewport_p->Next_p;
        }/* end while */
    }
}

/*******************************************************************************
Name        : stlayer_BuildHardImage
Description : Calculate the headers values (but don't write headers)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_BuildHardImage(void)
{
    stosd_ViewportDesc * CurrentViewport_p;
    U32 MQEP,S,Lenght,Pitch,YUpTop,YUpBot,YDownTop,YDownBot,XLeft,XRight;
    U32 NextTop,NextBot;
    U32 PixPerByte,NextLine,FirstPix;
    U32 TopPointer,BotPointer;
    U32 Alpha0,Alpha1,AA;
    ST_DeviceName_t Empty="\0\0";

    CurrentViewport_p = stlayer_osd_context.FirstViewPortLinked;
    /* Upadte OTP / OBP Registers */
    /*----------------------------*/
    if (CurrentViewport_p == 0)
    {
        /* Case : Empty list */
        stlayer_osd_context.OBP_Image
                    = (U32)DeviceAdd(stlayer_osd_context.GhostHeader_p) / 256;
        stlayer_osd_context.OTP_Image
                    = stlayer_osd_context.OBP_Image;
    }
    else
    {
        stlayer_osd_context.OBP_Image
                    = (U32)DeviceAdd(CurrentViewport_p->Bot_header) / 256;
        stlayer_osd_context.OTP_Image
                    = (U32)DeviceAdd(CurrentViewport_p->Top_header) / 256;
    }
    /* Write the headers */
    /*-------------------*/
    while(CurrentViewport_p != 0)
    {
        switch(CurrentViewport_p->Bitmap.ColorType)
        {
            case STGXOBJ_COLOR_TYPE_CLUT8:
                PixPerByte = 1;
                break;
            case STGXOBJ_COLOR_TYPE_CLUT4:
                PixPerByte = 2;
                break;
            case STGXOBJ_COLOR_TYPE_CLUT2:
                PixPerByte = 4;
                break;
            default: /* default is a rgb16 */
                PixPerByte = 1; /* instead of 1/2 */
                break;
        }
        NextLine = CurrentViewport_p->Bitmap.Pitch;
        FirstPix = (U32)BitmapAdd(CurrentViewport_p->Bitmap.Data1_p);
        FirstPix += (CurrentViewport_p->ClippedIn.PositionX / PixPerByte);
        if(stlayer_osd_context.EnableRGB16Mode) /* force 1pix=2bytes */
        {
            FirstPix += CurrentViewport_p->ClippedIn.PositionX;
        }
        FirstPix += CurrentViewport_p->ClippedIn.PositionY * NextLine;

        MQEP   = ComputeMQEP(CurrentViewport_p);
        S      = 1 - CurrentViewport_p->Recordable;
        Lenght = (CurrentViewport_p->ClippedIn.Width / PixPerByte);
        if(stlayer_osd_context.EnableRGB16Mode) /* force 1pix=2bytes */
        {
            Lenght += CurrentViewport_p->ClippedIn.Width;
        }

        Lenght = (Lenght + 8)           /* round */
                         / 8;           /* nb of 64bits words = 8 bytes */
        Pitch  = 2 * NextLine / 8;  /* bitmap prog ->interlaced display */
        if(CurrentViewport_p->ClippedOut.Height
                == CurrentViewport_p->ClippedIn.Height)
        {
            /* no resize */
            /* --------- */
            if((CurrentViewport_p->ClippedOut.PositionY + YOFFSET) % 2 == 0)
            {   /* even position */
                TopPointer = FirstPix;
                BotPointer = FirstPix + NextLine;
            }
            else
            {   /* odd position */
                BotPointer = FirstPix;
                TopPointer = FirstPix+ NextLine;
            }
        }
        else if(CurrentViewport_p->ClippedOut.Height
                == 2 * (CurrentViewport_p->ClippedIn.Height))
        {
            /* resize X 2 */
            /* ---------- */
            Pitch = Pitch / 2;
            TopPointer = FirstPix;
            BotPointer = FirstPix; /* each line is displayed twice */
        }
        else
        {
            /* resize X 0.5 */
            /* ------------ */
            Pitch = Pitch * 2;
            if((CurrentViewport_p->ClippedOut.PositionY + YOFFSET) % 2 == 0)
            {   /* even position */
                TopPointer = FirstPix;
                BotPointer = FirstPix + 2 * NextLine;
            }
            else
            {   /* odd position */
                BotPointer = FirstPix;
                TopPointer = FirstPix + 2 * NextLine;
            }
        }
        /* Set A0-A1 if alpha is one bit */
        if(CurrentViewport_p->Bitmap.ColorType == STGXOBJ_COLOR_TYPE_ARGB1555)
        {
            Alpha0 = CurrentViewport_p->Alpha.A0;
            Alpha0 = Alpha0 * 63 / 128; /* set in range */
            Alpha1 = CurrentViewport_p->Alpha.A1;
            Alpha1 = Alpha1 * 63 / 128; /* set in range */
        }
        else
        {
            Alpha0 = 0;
            Alpha1 = 0;
        }
        if(CurrentViewport_p->Bitmap.ColorType == STGXOBJ_COLOR_TYPE_RGB565)
        {
            AA=0;
        }
        else
        {
            AA=1;
        }
        CurrentViewport_p->HardImage1    = (MQEP                    << 25)
                                          |(S                       << 24)
                                          |(Lenght                  << 16)
                                          |(Alpha1                  << 10)
                                          |(Pitch                   << 0);

        CurrentViewport_p->HardImage2Top = (((U32)(1)) /*pcnotvid*/ << 31)
                                          |(AA                      << 30)
                                          |(Alpha0                  << 24)
                                          |((TopPointer / 8)        << 0) ;
        CurrentViewport_p->HardImage2Bot = (((U32)(1)) /*pcnotvid*/ << 31)
                                          |(AA                      << 30)
                                          |(Alpha0                  << 24)
                                          |((BotPointer / 8)        << 0) ;

        XLeft= CurrentViewport_p->ClippedOut.PositionX + XOFFSET;
        switch(stlayer_osd_context.FrameRate)
        {
            case 25000: /* PAL */
            case 50000: /* PAL */
            XLeft += OSD_PAL_HORIZONTAL_START;
            XRight= XLeft + CurrentViewport_p->ClippedOut.Width;
            if((CurrentViewport_p->ClippedOut.PositionY
                        + YOFFSET + OSD_PAL_VERTICAL_START) % 2 == 0)
            {  /* even position */
                YUpTop   = (CurrentViewport_p->ClippedOut.PositionY + YOFFSET
                    + OSD_PAL_VERTICAL_START )/2;
                YUpBot   = (CurrentViewport_p->ClippedOut.PositionY + YOFFSET
                    + OSD_PAL_VERTICAL_START )/2;
                YDownTop = YUpTop + (CurrentViewport_p->ClippedOut.Height / 2)
                              + (CurrentViewport_p->ClippedOut.Height % 2) - 1;
                YDownBot = YUpBot+(CurrentViewport_p->ClippedOut.Height/ 2) - 1;
            }
            else
            {   /* odd position */
                YUpTop   = (CurrentViewport_p->ClippedOut.PositionY + YOFFSET
                    + OSD_PAL_VERTICAL_START )/2 +1;
                YUpBot   = (CurrentViewport_p->ClippedOut.PositionY + YOFFSET
                    + OSD_PAL_VERTICAL_START )/2;
                YDownTop = YUpTop + (CurrentViewport_p->ClippedOut.Height/2) -1;
                YDownBot = YUpBot + (CurrentViewport_p->ClippedOut.Height / 2)
                              + (CurrentViewport_p->ClippedOut.Height % 2) - 1;
            }
            break;

            default:
            XLeft += OSD_NTSC_HORIZONTAL_START;
            XRight= XLeft + CurrentViewport_p->ClippedOut.Width;
            if((CurrentViewport_p->ClippedOut.PositionY
                        + YOFFSET + OSD_NTSC_VERTICAL_START) % 2 == 0)
            {  /* even position */
                YUpTop   = (CurrentViewport_p->ClippedOut.PositionY + YOFFSET
                    + OSD_NTSC_VERTICAL_START )/2;
                YUpBot   = (CurrentViewport_p->ClippedOut.PositionY + YOFFSET
                    + OSD_NTSC_VERTICAL_START )/2;
                YDownTop = YUpTop + (CurrentViewport_p->ClippedOut.Height / 2)
                              + (CurrentViewport_p->ClippedOut.Height % 2) - 1;
                YDownBot = YUpBot+(CurrentViewport_p->ClippedOut.Height/ 2) - 1;
            }
            else
            {   /* odd position */
                YUpTop   = (CurrentViewport_p->ClippedOut.PositionY + YOFFSET
                    + OSD_NTSC_VERTICAL_START )/2 +1;
                YUpBot   = (CurrentViewport_p->ClippedOut.PositionY + YOFFSET
                    + OSD_NTSC_VERTICAL_START )/2;
                YDownTop = YUpTop + (CurrentViewport_p->ClippedOut.Height/2) -1;
                YDownBot = YUpBot + (CurrentViewport_p->ClippedOut.Height / 2)
                              + (CurrentViewport_p->ClippedOut.Height % 2) - 1;
            }
            break;
        }
        CurrentViewport_p->HardImage3Top = (YUpTop       << 22)
                                         | (YDownTop     << 12)
                                         | (XLeft        << 02)
                                         | (XRight       >> 8);
        CurrentViewport_p->HardImage3Bot = (YUpBot       << 22)
                                         | (YDownBot     << 12)
                                         | (XLeft        << 02)
                                         | (XRight       >> 8);
        if (CurrentViewport_p->Next_p != 0)
        {
            NextBot = (U32)DeviceAdd(CurrentViewport_p->Next_p->Bot_header);
            NextTop = (U32)DeviceAdd(CurrentViewport_p->Next_p->Top_header);
        }
        else
        {
            NextTop = (U32)DeviceAdd(stlayer_osd_context.GhostHeader_p);
            NextBot = (U32)DeviceAdd(stlayer_osd_context.GhostHeader_p);
        }
        CurrentViewport_p->HardImage4Top = (XRight << 24) | (NextTop/ 8) ;
        CurrentViewport_p->HardImage4Bot = (XRight << 24) | (NextBot / 8) ;

        CurrentViewport_p = CurrentViewport_p->Next_p;
    }/* end while */

    /* If no synchro, direct write in headers */
    if (strcmp(stlayer_osd_context.VTGName, Empty) == 0)
    {
        stlayer_HardUpdateViewportList(TRUE);
        stlayer_HardUpdateViewportList(FALSE);
    }
    /* else : the update while be done on next VSync */
    stlayer_osd_context.UpdateBottom = TRUE;
    stlayer_osd_context.UpdateTop    = TRUE;
}

/*******************************************************************************
Name        : stlayer_HardEnableDisplay
Description : ena and set/reset the rgb16 mode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_HardEnableDisplay (void)
{
    U8 Cfg;

    Cfg = STSYS_ReadRegDev8((void *)(ADD + OSD_CFG));
    Cfg = Cfg | OSD_CFG_ENA;
    if(stlayer_osd_context.EnableRGB16Mode)
    {
        Cfg = Cfg | ( OSD_CFG_RGB16);  /* Set mode RGB16 */
        Cfg = Cfg & (~OSD_CFG_DBH);    /* Reset double header mode */
    }
    else
    {
        Cfg = Cfg | ( OSD_CFG_DBH);
        Cfg = Cfg & (~OSD_CFG_RGB16);
    }
    STSYS_WriteRegDev8((void *)(ADD + OSD_CFG),Cfg);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : stlayer_HardDisableDisplay
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_HardDisableDisplay (void)
{
    U8 Cfg;

    Cfg = STSYS_ReadRegDev8((void *)(ADD + OSD_CFG));
    STSYS_WriteRegDev8((void *)(ADD + OSD_CFG),Cfg & ~OSD_CFG_ENA);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYEROSD_EnableViewPortFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_EnableViewPortFilter(void)
{
    U8 Cfg;

    Cfg = STSYS_ReadRegDev8((void *)(ADD + OSD_CFG));
    Cfg = Cfg & (~OSD_CFG_FIL);                     /* Mode = antiflciker     */
    Cfg = Cfg | OSD_CFG_NOR;                        /* Filter ON              */
    STSYS_WriteRegDev8((void *)(ADD + OSD_CFG),Cfg);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYEROSD_DisableViewPortFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_DisableViewPortFilter(void)
{
    U8 Cfg;

    Cfg = STSYS_ReadRegDev8((void *)(ADD + OSD_CFG));
    Cfg = Cfg & (~OSD_CFG_FIL);                     /* Mode = antiflciker     */
    Cfg = Cfg & (~OSD_CFG_NOR);                     /* Filter OFF             */
    STSYS_WriteRegDev8((void *)(ADD + OSD_CFG),Cfg);
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stosd_ResetActiveSignalMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stosd_ResetActiveSignalMode(void)
{
  U8 Tmp;

  Tmp = STSYS_ReadRegDev8((void *)(ADD + OSD_ACT));
  STSYS_WriteRegDev8((void *)(ADD + OSD_ACT),Tmp & ~OSD_ACT_OAM);
  return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : stosd_SetActiveSignalDelay
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stosd_SetActiveSignalDelay(U8 Delay)
{
    U8 Tmp;
    U8 Tmp1;
    U8 Tmp2;

    Tmp = STSYS_ReadRegDev8((void *)(ADD + OSD_ACT));
    Tmp1 = Delay & OSD_ACT_OAD;
    Tmp2 = Tmp & ~OSD_ACT_OAD;
    STSYS_WriteRegDev8((void *)(ADD + OSD_ACT),Tmp1 | Tmp2);
    return (ST_NO_ERROR);
}
/*******************************************************************************
Name        : stlayer_HardInit
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_HardInit(void)
{
    U8 Tmp;

    interrupt_lock();

    /* set the boundary weight register to a arbitrary value */
    STSYS_WriteRegDev8((void *)(ADD + OSD_BDW),32);

    /* Set the double header mode within OSD_CFG register */
    Tmp = STSYS_ReadRegDev8((void *)(ADD + OSD_CFG));
    Tmp = Tmp |   OSD_CFG_LLG ;                     /* Set mem granularity    */
    Tmp = Tmp & (~OSD_CFG_FIL);                     /* Reset filters mode     */
    Tmp = Tmp & (~OSD_CFG_NOR);                     /* Reset filters          */
    Tmp = Tmp |   OSD_CFG_DBH ;                     /* Set double header mode */
    Tmp = Tmp |   OSD_CFG_REC ;                     /* Enable 4:2: output     */
    Tmp = Tmp |   OSD_CFG_POL ;                     /* Invert Enot0 sig       */
    STSYS_WriteRegDev8((void*)(ADD + OSD_CFG),Tmp);

    /* Set Video registers used by OSD */
    /* OSD_POL bit in VID_DCF used for OSD filtering */
    STSYS_WriteRegDev8((void *)(ADD + VID_DCF),
                    STSYS_ReadRegDev8((void *)(ADD + VID_DCF))| VID_DCF_OSDPOL);
    /* NOS bit in VID_OUT  */
    STSYS_WriteRegDev8((void *)(ADD + VID_OUT),
                    STSYS_ReadRegDev8((void *)(ADD + VID_OUT)) & ~VID_OUT_NOS);
    interrupt_unlock();

    stosd_ResetActiveSignalMode();
    stosd_SetActiveSignalDelay(0);
}

/*******************************************************************************
Name        : ComputeMQEP
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U32 ComputeMQEP(stosd_ViewportDesc * VP_p)
{
    U32     M = 0,Q,E = 0,P,MW,FMT;
    U32     Hresize;
    U32     MQEP;

    if (VP_p->ClippedOut.Width == 2 * VP_p->ClippedIn.Width)
    {
        Hresize = 1;
    }
    else
    {
        Hresize = 0;
    }
    P = 0; /* A Palette is behind the header */
    Q = Hresize;

    switch (VP_p->Bitmap.ColorType)
    {
        case STGXOBJ_COLOR_TYPE_ARGB4444:
        MW  = VP_p->Alpha.A0 * 15 / 128; /* set in range */
        FMT = 1;
        MQEP = (MW << 3) | (Q << 2) | (FMT <<0);
        break;

        case STGXOBJ_COLOR_TYPE_ARGB1555:
        MW  = VP_p->Alpha.A0 * 15 / 128; /* set in range */
        FMT = 2;
        MQEP = (MW << 3) | (Q << 2) | (FMT <<0);
        break;

        case STGXOBJ_COLOR_TYPE_RGB565:
        MW  = VP_p->Alpha.A0 * 15 / 128; /* set in range */
        FMT = 3;
        MQEP = (MW << 3) | (Q << 2) | (FMT <<0);
        break;

        default: /* Mode clut + YCbCr ; not RGB16 */
        switch(VP_p->Palette.ColorDepth)
        {
            case 2:
            E = 0;
            M = Hresize;
            break;
            case 4:
            E = 0;
            M = 1 - Hresize;
            break;
            case 8:
            E = 1;
            M = Hresize;
            break;
        }
        MQEP = (M << 3) | (Q << 2) | (E << 1) | (P << 0);
        break;
    }
    return(MQEP);
}
/******************************************************************************/

