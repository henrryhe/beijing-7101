/*****************************************************************************

File name   : osd_hal.c

Description : OSD Hardware Layer

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                 Name
----               ------------                                 ----
2001-03-13          Creation                                Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "osd_hal.h"
#include "stsys.h"
#include <stdlib.h>

/* Macros ------------------------------------------------------------------- */

#define XOFFSET (stlayer_osd_context.XOffset + stlayer_osd_context.XStart)
#define YOFFSET (stlayer_osd_context.YOffset + stlayer_osd_context.YStart)
#define DeviceAdd(Add) ((U32)Add \
      + (U32)stlayer_osd_context.VirtualMapping.PhysicalAddressSeenFromDevice_p\
      - (U32)stlayer_osd_context.VirtualMapping.PhysicalAddressSeenFromCPU_p)

/* Functions ---------------------------------------------------------------- */

static osd_RegionHeader_t * OsdAllocHeader(STAVMEM_BlockHandle_t * Handle);
static void OsdSetNext (osd_RegionHeader_t * header_p,
                        osd_RegionHeader_t * next_p);
ST_ErrorCode_t LAYEROSD_EnableViewPortFilter(void);
ST_ErrorCode_t LAYEROSD_DisableViewPortFilter(void);

/* Variables ---------------------------------------------------------------- */

static STAVMEM_BlockHandle_t GhostHandle,TopHandle,BotHandle;

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
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
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
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*******************************************************************************
Name        : stlayer_HardEnable
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_HardEnable(stosd_ViewportDesc * ViewPort_p, BOOL VSyncIsTop)
{
    osd_RegionHeader_t *    Header_p;
    osd_RegionHeader_t *    Next_p;

    if (VSyncIsTop == FALSE) /* -------------------------------- Top List */
    {
        /* Header to affect */
        if(ViewPort_p->Prev_p == NULL)
        {
            Header_p = stlayer_osd_context.FirstDummyHeader_p;
        }
        else if((ViewPort_p->Prev_p->PositionY + YOFFSET +
              ViewPort_p->Prev_p->Height) % 2 == 0)
        {
            Header_p = (osd_RegionHeader_t *)
                (ViewPort_p->Prev_p->BitmapPrevLastHeader_p);
        }
        else
        {
            Header_p = (osd_RegionHeader_t *)
                (ViewPort_p->Prev_p->BitmapLastHeader_p);
        }
        /* header to link (next) */
        if((ViewPort_p->PositionY + YOFFSET) %2 == 0)
        {
            Next_p  = (osd_RegionHeader_t *)
                (ViewPort_p->PaletteFirstHeader_p);
        }
        else
        {
            Next_p  = (osd_RegionHeader_t *)
                (ViewPort_p->PaletteSecondHeader_p);
        }
    }
    else /* ---------------------------------------------------- Bot List */
    {
        /* Header to affect */
        if(ViewPort_p->Prev_p == NULL)
        {
            Header_p = stlayer_osd_context.SecondDummyHeader_p;
        }
        else if((ViewPort_p->Prev_p->PositionY + YOFFSET +
              ViewPort_p->Prev_p->Height) % 2 == 0)
        {
            Header_p = (osd_RegionHeader_t *)
                (ViewPort_p->Prev_p->BitmapLastHeader_p);
        }
        else
        {
            Header_p = (osd_RegionHeader_t *)
                (ViewPort_p->Prev_p->BitmapPrevLastHeader_p);
        }
        if((ViewPort_p->PositionY + YOFFSET) %2 == 0)
        {
            Next_p  = (osd_RegionHeader_t *)
                (ViewPort_p->PaletteSecondHeader_p);
        }
        /* header to link (next) */
        else
        {
            Next_p  = (osd_RegionHeader_t *)
                (ViewPort_p->PaletteFirstHeader_p);
        }
    }
    /* Do the link */
    OsdSetNext(Header_p,Next_p);
}
/*******************************************************************************
Name        : stlayer_HardDisable
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_HardDisable(void)
{
    /* Link First Dummy -> Ghost */
    /* ------------------------- */
    OsdSetNext(stlayer_osd_context.FirstDummyHeader_p,
            stlayer_osd_context.GhostHeader_p);

    /* Link Second Dummy -> Ghost */
    /* -------------------------- */
    OsdSetNext(stlayer_osd_context.SecondDummyHeader_p,
            stlayer_osd_context.GhostHeader_p);
}
/*******************************************************************************
Name        : stlayer_HardUpdateViewportList
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_HardUpdateViewportList(stosd_ViewportDesc * ViewPort_p,
        BOOL VSyncIsTop)
{
    U32                     RegionType,NextHard;
    U32                     word1,word2,Record,CountLine,LastLine;
    U32                     XStart,XStop,YCurrent;
    osd_RegionHeader_t *    Header_p;
    osd_RegionHeader_t *    Next_p;


    /* Link the palette to the bitmap */
    /* ------------------------------ */
    if (VSyncIsTop == FALSE) /* -------------------------- Top List */
    {
        if((ViewPort_p->PositionY + YOFFSET) % 2 == 0)  /* Even pos */
        {
            Header_p = ViewPort_p->PaletteFirstHeader_p;
            Next_p = ViewPort_p->BitmapFirstHeader_p;
            YCurrent = ((ViewPort_p->PositionY + YOFFSET) / 2);
        }
        else                                             /* Odd pos */
        {
            Header_p = ViewPort_p->PaletteSecondHeader_p;
            Next_p = ViewPort_p->BitmapSecondHeader_p;
            YCurrent = ((ViewPort_p->PositionY + YOFFSET) / 2 + 1);
        }
    }
    else /* ---------------------------------------------- Bot List */
    {
        if((ViewPort_p->PositionY + YOFFSET) % 2 == 0)  /* Even pos */
        {
            Header_p = ViewPort_p->PaletteSecondHeader_p;
            Next_p = ViewPort_p->BitmapSecondHeader_p;
            YCurrent = ((ViewPort_p->PositionY + YOFFSET) / 2);
        }
        else                                             /* Odd pos */
        {
            Header_p = ViewPort_p->PaletteFirstHeader_p;
            Next_p = ViewPort_p->BitmapFirstHeader_p;
            YCurrent = ((ViewPort_p->PositionY + YOFFSET) / 2);
        }
    }
    /* Update the palette header */
    /* ------------------------- */
    XStart   = XOFFSET;
    XStop    = XOFFSET+10;
    YCurrent -=1; /* palette position (dummy) */
    switch(ViewPort_p->Palette.ColorDepth)
    {
        case 8:
        RegionType = OSD_PALETTE_256;
        break;
        case 4:
        RegionType = OSD_PALETTE_16;
        break;
        case 2:
        RegionType = OSD_PALETTE_4;
        break;
    }
    Record     = 1 - (ViewPort_p->Recordable);
#ifdef HW_5510
    NextHard   = ((U32)DeviceAdd(Next_p)) / 8;
#else /* Other chip : double granularity */
    NextHard   = ((U32)DeviceAdd(Next_p)) / 16;
#endif
    word1  = (RegionType                << 26)
            |(Record                    << 25)
            |(YCurrent                  << 16)
            |(0                         << 12)
            |(((NextHard >> 4) & 0x07)  << 9)
            |(YCurrent                  << 0);
    word2  = (((NextHard >> 7) & 0x3F)  << 26)
            |(XStart                    << 16)
            |(((NextHard >> 13) & 0x3F) << 10)
            |(XStop                     << 0);
    STSYS_WriteRegMem32BE((void*)(Header_p),word1);
    STSYS_WriteRegMem32BE((void*)((U8*)Header_p + sizeof(U32)),word2);

    /* Update the bitmap region list */
    /* ----------------------------- */
    XStart     = ViewPort_p->PositionX + XOFFSET;
    XStop      = XStart + ViewPort_p->Width -1;
    YCurrent   += 1; /* viewport position */
    RegionType = OSD_NO_PALETTE;
    CountLine  = 0;
    LastLine = ViewPort_p->Height / 2;
    if (ViewPort_p->Height %2 != 0)
    {
        if ((VSyncIsTop == FALSE) &&((ViewPort_p->PositionY + YOFFSET) %2 == 0))
        {
            LastLine +=1;
        }
        if ((VSyncIsTop == TRUE) &&((ViewPort_p->PositionY + YOFFSET) %2 == 1))
        {
            LastLine +=1;
        }
    }
    do
    {
        Header_p  = Next_p;
        Next_p  = (osd_RegionHeader_t *)((U8*)Header_p + 2 * ViewPort_p->Pitch);
#ifdef HW_5510
        NextHard   = ((U32)DeviceAdd(Next_p)) / 8;
#else /* Other Chip : double granularity */ 
        NextHard   = ((U32)DeviceAdd(Next_p)) / 16;
#endif
        word1  = (RegionType                << 26)
                |(Record                    << 25)
                |(YCurrent                  << 16)
                |(0                         << 12)
                |(((NextHard >> 4) & 0x07)  << 9)
                |(YCurrent                  << 0);
        word2  = (((NextHard >> 7) & 0x3F)  << 26)
                |(XStart                    << 16)
                |(((NextHard >> 13) & 0x3F) << 10)
                |(XStop                     << 0);
        STSYS_WriteRegMem32BE((void*)(Header_p),word1);
        STSYS_WriteRegMem32BE((void*)((U8*)Header_p + sizeof(U32)),word2);
        /* re-iterration */
        YCurrent  += 1;
        CountLine += 1;
    } while( CountLine < LastLine );

    /* Update End of the list */
    /* ---------------------- */
    if (ViewPort_p->Next_p == NULL)
    {
        /* link the last header to the ghost header */
        Next_p = stlayer_osd_context.GhostHeader_p;
    }
    else
    {
        /* last header -> next = next viewport palette header */
        if (VSyncIsTop == FALSE) /* -------------------------------- Top List */
        {
            if ((ViewPort_p->Next_p->PositionY 
                 + ViewPort_p->Next_p->Height + YOFFSET) %2 == 0)
            {
                Next_p = ViewPort_p->Next_p->PaletteFirstHeader_p;
            }
            else
            {
                Next_p = ViewPort_p->Next_p->PaletteSecondHeader_p;
            }
        }
        else /* ---------------------------------------------------- Bot List */
        {
            if ((ViewPort_p->Next_p->PositionY 
                 + ViewPort_p->Next_p->Height + YOFFSET) %2 == 0)
            {
                Next_p = ViewPort_p->Next_p->PaletteSecondHeader_p;
            }
            else
            {
                Next_p = ViewPort_p->Next_p->PaletteFirstHeader_p;
            }
        }
    }
    /* Link to next */
    OsdSetNext(Header_p,Next_p);
}

/*******************************************************************************
Name        : stlayer_EnableDisplay
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_EnableDisplay (void)
{
    U8 Cfg;

    Cfg = STSYS_ReadRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                    + OSD_CFG));
    STSYS_WriteRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                    + OSD_CFG),Cfg | OSD_CFG_ENA);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : stlayer_DisableDisplay
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_DisableDisplay (void)
{
    U8 Cfg;

    Cfg = STSYS_ReadRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                    + OSD_CFG));
    STSYS_WriteRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                    + OSD_CFG),Cfg & ~OSD_CFG_ENA);
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

    Tmp = STSYS_ReadRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                            + OSD_ACT));
    STSYS_WriteRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                            + OSD_ACT), Tmp & ~OSD_ACT_OAM);
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

    Tmp = STSYS_ReadRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress
                                         + OSD_ACT));
    Tmp1 = Delay & OSD_ACT_OAD;
    Tmp2 = Tmp & ~OSD_ACT_OAD;
    STSYS_WriteRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                            + OSD_ACT),Tmp1 | Tmp2);
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
    U32                     RegionType,NextHard;
    U32                     word1,word2,Record;
    U32                     XStart,XStop,YCurrent;
    osd_RegionHeader_t *    Next_p;

    interrupt_lock();
    
    /* set the boundary weight register to a arbitrary value */
    STSYS_WriteRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                        + OSD_BDW),32);

    /* Set OSD_CFG register */
    Tmp = STSYS_ReadRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                            + OSD_CFG));
    Tmp = Tmp |   OSD_CFG_LLG ;
    Tmp = Tmp & (~OSD_CFG_NOR); /* no filter */
    STSYS_WriteRegDev8((void*)(stlayer_osd_context.stosd_BaseAddress 
                                        + OSD_CFG),Tmp);

    /* Set Video registers used by OSD */
    /* OSD_POL bit in VID_DCF used for OSD filtering */
    STSYS_WriteRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress+VID_DCF), 
            STSYS_ReadRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                                + VID_DCF))| VID_DCF_OSDPOL);
    /* NOS bit in VID_OUT  */
    STSYS_WriteRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress+VID_OUT), 
            STSYS_ReadRegDev8((void *)(stlayer_osd_context.stosd_BaseAddress 
                    + VID_OUT))& ~VID_OUT_NOS);
    interrupt_unlock();

    stosd_ResetActiveSignalMode();
    stosd_SetActiveSignalDelay(0);

    /* Init Ghost header */
    stlayer_osd_context.GhostHeader_p = OsdAllocHeader(&GhostHandle);
    STSYS_WriteRegMem32BE(&(stlayer_osd_context.GhostHeader_p->word1)
                                        ,0xFFFFFFFF);
    STSYS_WriteRegMem32BE(&(stlayer_osd_context.GhostHeader_p->word2)
                                        , 0xFFFFFFFF);
    /* Init Dummy Headers */
    stlayer_osd_context.FirstDummyHeader_p  = OsdAllocHeader(&TopHandle);
    stlayer_osd_context.SecondDummyHeader_p = OsdAllocHeader(&BotHandle);

    /* link dummy -> Ghost */
    /* ------------------- */
    Next_p      = stlayer_osd_context.GhostHeader_p;
#ifdef HW_5510
    NextHard    = ((U32)DeviceAdd(Next_p)) / 8;
#else /* Other chips : double granularity */
    NextHard    = ((U32)DeviceAdd(Next_p)) / 16;
#endif 
    YCurrent    = (YOFFSET-4)/2;
    XStart      = 100;
    XStop       = 300;
    RegionType  = OSD_NO_PALETTE;
    Record      = 1;
    word1  = (RegionType                << 26)
            |(Record                    << 25)
            |(YCurrent                  << 16)
            |(0                         << 12)
            |(((NextHard >> 4) & 0x07)  << 9)
            |(YCurrent                  << 0);
    word2  = (((NextHard >> 7) & 0x3F)  << 26)
            |(XStart                    << 16)
            |(((NextHard >> 13) & 0x3F) << 10)
            |(XStop                     << 0);
    STSYS_WriteRegMem32BE((void*)(stlayer_osd_context.FirstDummyHeader_p),
                                                                word1);
    STSYS_WriteRegMem32BE((void*)((U8*)stlayer_osd_context.FirstDummyHeader_p 
                                                    + sizeof(U32)),word2);
    STSYS_WriteRegMem32BE((void*)(stlayer_osd_context.SecondDummyHeader_p),
                                                                word1);
    STSYS_WriteRegMem32BE((void*)((U8*)stlayer_osd_context.SecondDummyHeader_p
                                                    + sizeof(U32)),word2);

    /* Link TOP -> first dummy */
    /* ----------------------- */
    Next_p = stlayer_osd_context.FirstDummyHeader_p;
    NextHard   = ((U32)DeviceAdd(Next_p)) / 256;
    STSYS_WriteRegDev8((void*)(stlayer_osd_context.stosd_BaseAddress + OSD_OTP),
                        ((U32)NextHard >> 8 ) & 0x3F);
    STSYS_WriteRegDev8((void*)(stlayer_osd_context.stosd_BaseAddress + OSD_OTP),
                        (U32)NextHard & 0xFF);
    /* Link BOT -> second dummy */
    /* ------------------------ */
    Next_p = stlayer_osd_context.SecondDummyHeader_p;
    NextHard   = ((U32)DeviceAdd(Next_p)) / 256;
    STSYS_WriteRegDev8((void*)(stlayer_osd_context.stosd_BaseAddress + OSD_OBP),
                        ((U32)NextHard >> 8 ) & 0x3F);
    STSYS_WriteRegDev8((void*)(stlayer_osd_context.stosd_BaseAddress + OSD_OBP),
                        (U32)NextHard & 0xFF);
}

/*******************************************************************************
Name        : stlayer_HardTerm
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_HardTerm(void)
{
    STAVMEM_FreeBlockParams_t   FreeParams;

    stlayer_DisableDisplay();
    FreeParams.PartitionHandle = stlayer_osd_context.stosd_AVMEMPartition;
    STAVMEM_FreeBlock(&FreeParams,&GhostHandle);
    STAVMEM_FreeBlock(&FreeParams,&TopHandle);
    STAVMEM_FreeBlock(&FreeParams,&BotHandle);
}

/*******************************************************************************
Name        : AllocHeader
Description : 
Parameters  : 
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
static osd_RegionHeader_t * OsdAllocHeader(STAVMEM_BlockHandle_t * Handle)
{
    osd_RegionHeader_t * Pointer;
    STAVMEM_AllocBlockParams_t AllocBlockParams;
    STAVMEM_MemoryRange_t      ForbiddenArea[1];

    ForbiddenArea[0].StartAddr_p = (void *)
           ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p));
    ForbiddenArea[0].StopAddr_p  =  (void *)
           ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p)+ 3);
    ForbiddenArea[1].StartAddr_p = (void *)
           ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p) 
          + (U32)(stlayer_osd_context.VirtualMapping.VirtualWindowOffset)
          + (U32)(stlayer_osd_context.VirtualMapping.VirtualWindowSize));
    ForbiddenArea[1].StopAddr_p  =  (void *)
           ((U32)(stlayer_osd_context.VirtualMapping.VirtualBaseAddress_p) 
          + (U32)(stlayer_osd_context.VirtualMapping.VirtualSize));
    AllocBlockParams.PartitionHandle         = stlayer_osd_context.stosd_AVMEMPartition;
    AllocBlockParams.Size                    = sizeof(osd_RegionHeader_t);
    AllocBlockParams.Alignment               = 256;
    AllocBlockParams.AllocMode               = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges = 2;
    AllocBlockParams.ForbiddenRangeArray_p   = &ForbiddenArea[0];
    AllocBlockParams.NumberOfForbiddenBorders= 0;
    AllocBlockParams.ForbiddenBorderArray_p  = NULL;
    STAVMEM_AllocBlock(&AllocBlockParams, Handle);
    STAVMEM_GetBlockAddress(*Handle ,(void**)&Pointer);
    Pointer = STAVMEM_VirtualToCPU(Pointer,&stlayer_osd_context.VirtualMapping);

    return(Pointer);
}
/*******************************************************************************
Name        : OsdSetNext
Description : 
Parameters  : 
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
static void OsdSetNext(osd_RegionHeader_t * Header_p,
                       osd_RegionHeader_t * Next_p)
{
    U32 word1,word2,NextHard;

#ifdef HW_5510
    NextHard   = ((U32)DeviceAdd(Next_p)) / 8;
#else /* Other chips : Double granularity */
    NextHard   = ((U32)DeviceAdd(Next_p)) / 16;
#endif
    word1 = STSYS_ReadRegMem32BE((void*)((U8*)Header_p ));
    word2 = STSYS_ReadRegMem32BE((void*)((U8*)Header_p + sizeof(U32)));
    word1 &= ~((U32)0x7 << 9);
    word1 |= (((NextHard >> 4) & 0x07)  << 9);
    word2 &= ~(((U32)0x3F << 26) | ((U32)0x3F << 10));
    word2 |= (((NextHard >> 7) & 0x3F)  << 26)
            |(((NextHard >> 13) & 0x3F) << 10);
    STSYS_WriteRegMem32BE((void*)((U8*)Header_p),              word1);
    STSYS_WriteRegMem32BE((void*)((U8*)Header_p + sizeof(U32)),word2);
}

/******************************************************************************/
