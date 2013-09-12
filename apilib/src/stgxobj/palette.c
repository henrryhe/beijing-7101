/*******************************************************************************

File name   : palette.c

Description : palette source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                  Name
----               ------------                                  ----
2000-06-19         Created                                       Adriano Melis
2000-10-18         Use stsys                                     Michel Bruant
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include "stddefs.h"
#include "stgxobj.h"
#include "stsys.h"
#if !defined(ST_OSLINUX)
#include "stavmem.h"
#endif
/* Functions ---------------------------------------------------------------- */

static ST_ErrorCode_t stgxobj_CpuAdd(U8 ** Add_p, U32 Size)
{

#if !defined(ST_OSLINUX)
    STAVMEM_SharedMemoryVirtualMapping_t VM;

    STAVMEM_GetSharedMemoryVirtualMapping(&VM);
    if ((STAVMEM_IsAddressVirtual(*Add_p, &VM))
     && !(STAVMEM_IsDataInVirtualWindow(*Add_p,Size, &VM)))
    {
	    return(ST_ERROR_BAD_PARAMETER);
    }
    else
    {
	    *Add_p = (U8 *)STAVMEM_IfVirtualThenToCPU(*Add_p,&VM);
    }
#else
    UNUSED_PARAMETER(Add_p);
    UNUSED_PARAMETER(Size);
#endif
	return(ST_NO_ERROR);
}

/* -------------------------------------------------------------------------- */

static ST_ErrorCode_t TestHardCoherence(STGXOBJ_HardUse_t HardUse)
{
    /* osd frame / osd top-bot incompatibility) */
    if(((HardUse & STGXOBJ_OSDTOP_BOT_CELL) == STGXOBJ_OSDTOP_BOT_CELL)
     &&((HardUse & STGXOBJ_OSDFRAME_CELL) == STGXOBJ_OSDFRAME_CELL))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    return (ST_NO_ERROR);
}

/*
--------------------------------------------------------------------------------
  STGXOBJ_ConvertPalette()
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STGXOBJ_ConvertPalette(  STGXOBJ_Palette_t*    SrcPalette_p,
                                        STGXOBJ_Palette_t*    DstPalette_p,
                                STGXOBJ_ColorSpaceConversionMode_t ConvMode)
{
    U32 i;
    U32 NumEntries;
    U32 src32,dst32;
    U16 dst16;
    S32 Y, Cb, Cr;
    U32 A,  R,  G,  B;
    U8 * AddSrc;
    U8 * AddDst;

    if((SrcPalette_p->ColorDepth != DstPalette_p->ColorDepth )
    || (DstPalette_p->ColorType  != STGXOBJ_COLOR_TYPE_ARGB8888
     && DstPalette_p->ColorType  != STGXOBJ_COLOR_TYPE_ARGB4444
     && DstPalette_p->ColorType  != STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444
     && DstPalette_p->ColorType  != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444
     && DstPalette_p->ColorType  != STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888 )
    || (SrcPalette_p->ColorType  != STGXOBJ_COLOR_TYPE_ARGB8888 ))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    NumEntries = 1 << DstPalette_p->ColorDepth;
    AddSrc = (U8*)(SrcPalette_p->Data_p);
    AddDst = (U8*)(DstPalette_p->Data_p);
    if((stgxobj_CpuAdd(&AddSrc,sizeof(U32) * NumEntries) != 0)
     ||(stgxobj_CpuAdd(&AddDst,sizeof(U32) * NumEntries) != 0))
    {
	    return(ST_ERROR_BAD_PARAMETER);
    }

    switch (DstPalette_p->ColorType)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        /* nothing to do : just recopy the palette */
        for(i=0; i<NumEntries; i++)
        {
            src32 = STSYS_ReadRegMem32LE((U32)AddSrc + sizeof(U32) * i);
            STSYS_WriteRegMem32LE((U32)AddDst + sizeof(U32) * i, src32);
        }
        break;

        case STGXOBJ_COLOR_TYPE_ARGB4444:
        for(i=0; i<NumEntries; i++)
        {
            src32 = STSYS_ReadRegMem32LE((U32)AddSrc + sizeof(U32) * i);
            dst16 = (src32 & 0x000000F0) >> 4 |
                    (src32 & 0x0000F000) >> 8 |
                    (src32 & 0x00F00000) >> 12;
            A = (src32 & 0xFF000000) >> 24;
            /* Note A8 : 0->128(opaque) */
            /*      A4 : 0->15 (opaque) */
            /* Bits mask is unavailable */
            A = A * 15;
            A = A / 128;
            dst16 = dst16 | ( A << 12);
            STSYS_WriteRegMem16LE((U32)AddDst + sizeof(U16) * i, dst16);
        }
        break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
        for(i=0; i<NumEntries; i++)
        {
            src32 = STSYS_ReadRegMem32LE((U32)AddSrc + sizeof(U32) * i);
            A  = ((src32 & 0xFF000000) >> 24);
            R  = ((src32 & 0xFF0000) >> 16);
            G  = ((src32 & 0xFF00) >> 8);
            B  =  (src32 & 0xFF);
            switch(ConvMode)
            {
                case STGXOBJ_ITU_R_BT601:
                    Y  = ( 66*(S32)R + 129*(S32)G +  25*(S32)B)/256 + 16;
                    Cb = (112*(S32)B -  38*(S32)R -  74*(S32)G)/256 + 128;
                    Cr = (112*(S32)R -  94*(S32)G -  18*(S32)B)/256 + 128;
                break;
                case STGXOBJ_ITU_R_BT709:
                    Y  = ( 47*(S32)R + 157*(S32)G +  16*(S32)B)/256 + 16;
                    Cb = (112*(S32)B   -26*(S32)R -  86*(S32)G)/256 + 128;
                    Cr = (112*(S32)R - 102*(S32)G -  10*(S32)B)/256 + 128;
                break;
                default:
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
            }
            A = A * 63 / 128; /* 8bits to 6bits */
            if(Y < 16)
                Y = 16;
            else if(Y > 235)
                Y = 235;
            if(Cb < 16)
                Cb = 16;
            else if(Cb > 240)
                Cb = 240;
            if(Cr < 16)
                Cr = 16;
            else if(Cr > 240)
                Cr = 240;
            dst32 = (A<<24)|(Y<<16)|(Cb<<8)|(Cr);
            /* Take care : Use Big endians */
            STSYS_WriteRegMem32BE((U32)AddDst + sizeof(U32) * i, dst32);
        } /* next i */
        break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:

        for(i=0; i<NumEntries; i++)
        {
            src32 = STSYS_ReadRegMem32LE((U32)AddSrc + sizeof(U32) * i);
            R  = ((src32 & 0xFF0000) >> 16);
            G  = ((src32 & 0xFF00) >> 8);
            B  =  (src32 & 0xFF);
            switch(ConvMode)
            {
                case STGXOBJ_ITU_R_BT601:
                    Y  = ( 66*(S32)R + 129*(S32)G +  25*(S32)B)/256 + 16;
                    Cb = (112*(S32)B -  38*(S32)R -  74*(S32)G)/256 + 128;
                    Cr = (112*(S32)R -  94*(S32)G -  18*(S32)B)/256 + 128;
                break;
                case STGXOBJ_ITU_R_BT709:
                    Y  = ( 47*(S32)R + 157*(S32)G +  16*(S32)B)/256 + 16;
                    Cb = (112*(S32)B   -26*(S32)R -  86*(S32)G)/256 + 128;
                    Cr = (112*(S32)R - 102*(S32)G -  10*(S32)B)/256 + 128;
                break;
                default:
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
            }
            if(Y < 16)
                Y = 16;
            else if(Y > 235)
                Y = 235;
            if(Cb < 16)
                Cb = 16;
            else if(Cb > 240)
                Cb = 240;
            if(Cr < 16)
                Cr = 16;
            else if(Cr > 240)
                Cr = 240;

            dst32 = ((U32)128<<24)|(Cr<<16)|(Y<<8)|(Cb);
            STSYS_WriteRegMem32LE((U32)AddDst + sizeof(U32) * i, dst32);
        } /* next i */
        break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
        for(i=0; i<NumEntries; i++)
        {
            src32 = STSYS_ReadRegMem32LE((U32)AddSrc + sizeof(U32) * i);
            A  = ((src32 & 0xFF000000) >> 24);
            R  = ((src32 & 0xFF0000) >> 16);
            G  = ((src32 & 0xFF00) >> 8);
            B  =  (src32 & 0xFF);
            switch(ConvMode)
            {
                case STGXOBJ_ITU_R_BT601:
                    Y  = ( 66*(S32)R + 129*(S32)G +  25*(S32)B)/256 + 16;
                    Cb = (112*(S32)B -  38*(S32)R -  74*(S32)G)/256 + 128;
                    Cr = (112*(S32)R -  94*(S32)G -  18*(S32)B)/256 + 128;
                break;
                case STGXOBJ_ITU_R_BT709:
                    Y  = ( 47*(S32)R + 157*(S32)G +  16*(S32)B)/256 + 16;
                    Cb = (112*(S32)B   -26*(S32)R -  86*(S32)G)/256 + 128;
                    Cr = (112*(S32)R - 102*(S32)G -  10*(S32)B)/256 + 128;
                break;
                default:
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
            }
            if(Y < 16)
                Y = 16;
            else if(Y > 235)
                Y = 235;
            if(Cb < 16)
                Cb = 16;
            else if(Cb > 240)
                Cb = 240;
            if(Cr < 16)
                Cr = 16;
            else if(Cr > 240)
                Cr = 240;

            dst32 = (A<<24)|(Cr<<16)|(Y<<8)|(Cb);
            STSYS_WriteRegMem32LE((U32)AddDst + sizeof(U32) * i, dst32);
        } /* next i */
        break;

        default:
        break;
    } /* end switch-case */
    return(ST_NO_ERROR);
}

/*
--------------------------------------------------------------------------------
  STGXOBJ_GetPaletteColor()
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STGXOBJ_GetPaletteColor(STGXOBJ_Palette_t*   Palette_p,
                                       U8                   PaletteIndex,
                                       STGXOBJ_Color_t*     Color_p )
{
    U32 Size;
    U16 Value16;
    U32 Value32;
    U8* Dest_p;

    /* Note : OSD 5514 uses STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444 */
    /*                  Clut 2, Clut 4 or Clut 8                       */
    /*        Gamma uses STGXOBJ_COLOR_TYPE_ARGB4444                   */
    /*                  Clut 4                                         */

    if(PaletteIndex > ((1 << Palette_p->ColorDepth) -1))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (Palette_p->ColorType)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
            /* Palette available in gamma cell : Direct access to color */
            Size = sizeof(U32);
            Color_p->Type = STGXOBJ_COLOR_TYPE_ARGB8888;
            Dest_p = (U8*)((U32)Palette_p->Data_p + (PaletteIndex * Size));
            if(stgxobj_CpuAdd(&Dest_p,Size) != 0)
            {
	            return(ST_ERROR_BAD_PARAMETER);
            }
            Value32 = STSYS_ReadRegMem32LE((U32)Dest_p);
            Color_p->Value.ARGB8888.Alpha = (Value32 & 0xff000000) >> 24;
            Color_p->Value.ARGB8888.R     = (Value32 & 0x00ff0000) >> 16;
            Color_p->Value.ARGB8888.G     = (Value32 & 0x0000ff00) >>  8;
            Color_p->Value.ARGB8888.B     = (Value32 & 0x000000ff)      ;
            break;

        case STGXOBJ_COLOR_TYPE_ARGB4444:
            /* Palette available in gamma cell : Direct access to color */
            Size = sizeof(U16);
            Color_p->Type  = STGXOBJ_COLOR_TYPE_ARGB4444;
            Dest_p = (U8*)((U32)Palette_p->Data_p + (PaletteIndex * Size));
            if(stgxobj_CpuAdd(&Dest_p,Size) != 0)
            {
	            return(ST_ERROR_BAD_PARAMETER);
            }
            Value16 = STSYS_ReadRegMem16LE((U32)Dest_p);
            Color_p->Value.ARGB4444.Alpha = (Value16 & 0xf000) >> 12;
            Color_p->Value.ARGB4444.R     = (Value16 & 0x0f00) >>  8;
            Color_p->Value.ARGB4444.G     = (Value16 & 0x00f0) >>  4;
            Color_p->Value.ARGB4444.B     = (Value16 & 0x000f)      ;
            break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
            /* Palette available on 5514 OSD */
            Size = sizeof(U32);
            Color_p->Type  = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444;
            Dest_p = (U8*)((U32)Palette_p->Data_p + (PaletteIndex * Size));
            if(stgxobj_CpuAdd(&Dest_p,Size) != 0)
            {
	            return(ST_ERROR_BAD_PARAMETER);
            }
            Value32 = STSYS_ReadRegMem32BE(Dest_p);
            Color_p->Value.UnsignedAYCbCr6888_444.Alpha
                                        = (Value32 & 0x3f000000) >> 24;
            Color_p->Value.UnsignedAYCbCr6888_444.Y
                                        = (Value32 & 0x00ff0000) >> 16;
            Color_p->Value.UnsignedAYCbCr6888_444.Cb
                                        = (Value32 & 0x0000ff00) >>  8;
            Color_p->Value.UnsignedAYCbCr6888_444.Cr
                                        = (Value32 & 0x000000ff)      ;
            break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
            /* Palette available on 51xx/8010 */
            Size = sizeof(U32);
            Color_p->Type  = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888;
            Dest_p = (U8*)((U32)Palette_p->Data_p + (PaletteIndex * Size));
            if(stgxobj_CpuAdd(&Dest_p,Size) != 0)
            {
	            return(ST_ERROR_BAD_PARAMETER);
            }
            Value32 = STSYS_ReadRegMem32LE((U32)Dest_p);
            Color_p->Value.UnsignedAYCbCr8888.Alpha
                                        = (Value32 & 0xff000000) >> 24;
            Color_p->Value.UnsignedAYCbCr8888.Cr
                                        = (Value32 & 0x00ff0000) >> 16;
            Color_p->Value.UnsignedAYCbCr8888.Y
                                        = (Value32 & 0x0000ff00) >>  8;
            Color_p->Value.UnsignedAYCbCr8888.Cb
                                        = (Value32 & 0x000000ff)      ;
            break;



        default:
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
}

/*
--------------------------------------------------------------------------------
  STGXOBJ_SetPaletteColor()
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STGXOBJ_SetPaletteColor(STGXOBJ_Palette_t*   Palette_p,
                                       U8                   PaletteIndex,
                                       STGXOBJ_Color_t*     Color_p)
{
    /* Note : OSD 5514 uses STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444 */
    /*                  Clut 2, Clut 4 or Clut 8                       */
    /*        Gamma uses STGXOBJ_COLOR_TYPE_ARGB4444                   */
    /*                  Clut 4                                         */

    U32     Size;
    U8 *    Dest_p;
    U32     Value32;
    U16     Value16;

    if(PaletteIndex > ((1 << Palette_p->ColorDepth) -1) ||
        Palette_p->ColorType != Color_p->Type)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (Color_p->Type)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        /* Palette available in gamma cell : Direct access to color */
            Size = sizeof(U32);
            Dest_p = (U8*)((U32)Palette_p->Data_p + (PaletteIndex * Size));
            if(stgxobj_CpuAdd(&Dest_p,Size) != 0)
            {
	            return(ST_ERROR_BAD_PARAMETER);
            }
            Value32 = ((U32)(Color_p->Value.ARGB8888.B     & 0xFF)       )
                    | ((U32)(Color_p->Value.ARGB8888.G     & 0xFF) << 8  )
                    | ((U32)(Color_p->Value.ARGB8888.R     & 0xFF) << 16 )
                    | ((U32)(Color_p->Value.ARGB8888.Alpha & 0xFF) << 24 );
            STSYS_WriteRegMem32LE((void*)Dest_p,Value32);
            break;

        case STGXOBJ_COLOR_TYPE_ARGB4444:
        /* Palette available in gamma cell : Direct access to color */
            Size = sizeof(U16);
            Dest_p = (U8*)((U32)Palette_p->Data_p + (PaletteIndex * Size));
            if(stgxobj_CpuAdd(&Dest_p,Size) != 0)
            {
	            return(ST_ERROR_BAD_PARAMETER);
            }
            Value16 = ((U16)(Color_p->Value.ARGB4444.B)     & 0x0F       )
                    | (((U16)(Color_p->Value.ARGB4444.G)    & 0x0F) << 4 )
                    | (((U16)(Color_p->Value.ARGB4444.R)    & 0x0F) << 8 )
                    | (((U16)(Color_p->Value.ARGB4444.Alpha)& 0x0F) << 12);
            STSYS_WriteRegMem16LE((void*)Dest_p,Value16);
            break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
            /* Palette available on 5514 OSD */
            Size = sizeof(U32);
            Dest_p = (U8*)((U32)Palette_p->Data_p + (PaletteIndex * Size));
            if(stgxobj_CpuAdd(&Dest_p,Size) != 0)
            {
	            return(ST_ERROR_BAD_PARAMETER);
            }
            Value32 =
               ((U32)(Color_p->Value.UnsignedAYCbCr6888_444.Cr    & 0xFF)     )
              |((U32)(Color_p->Value.UnsignedAYCbCr6888_444.Cb    & 0xFF) << 8)
              |((U32)(Color_p->Value.UnsignedAYCbCr6888_444.Y     & 0xFF) <<16)
              |((U32)(Color_p->Value.UnsignedAYCbCr6888_444.Alpha & 0x3F) <<24);
            STSYS_WriteRegMem32BE((void*)Dest_p,Value32 );
            break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
            /* Palette available in 51xx/8010 */
            Size = sizeof(U32);
            Dest_p = (U8*)((U32)Palette_p->Data_p + (PaletteIndex * Size));
            if(stgxobj_CpuAdd(&Dest_p,Size) != 0)
            {
	            return(ST_ERROR_BAD_PARAMETER);
            }
            Value32 =
               ((U32)(Color_p->Value.UnsignedAYCbCr8888.Cb    & 0xFF)     )
              |((U32)(Color_p->Value.UnsignedAYCbCr8888.Y     & 0xFF) << 8)
              |((U32)(Color_p->Value.UnsignedAYCbCr8888.Cr    & 0xFF) <<16)
              |((U32)(Color_p->Value.UnsignedAYCbCr8888.Alpha & 0xFF) <<24) ;
            STSYS_WriteRegMem32LE((void*)Dest_p,Value32 );
            break;

         default:
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : STGXOBJ_GetRevision
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_Revision_t STGXOBJ_GetRevision(void)
{
    static const char Revision[] = "STGXOBJ-REL_4.0.3";
    return((ST_Revision_t) Revision);
}

/*******************************************************************************
Name        : STGXOBJ_GetBitmapAllocParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STGXOBJ_GetBitmapAllocParams(STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STGXOBJ_HardUse_t             HardUse,
                                    STGXOBJ_BitmapAllocParams_t*  Params1_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params2_p )
{
    U32 MBRow,MBCol;

    if ((Bitmap_p == NULL) || (Params1_p == NULL) || ( Params2_p == NULL ))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (TestHardCoherence(HardUse) != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Reset returned values */
    Params1_p->AllocBlockParams.Alignment = 0;
    Params1_p->AllocBlockParams.Size      = 0;
    Params1_p->Pitch                      = 0;
    Params1_p->Offset                     = 0;
    *Params2_p                            = *Params1_p;

    switch(Bitmap_p->ColorType)
    {
        /* 1pix -> 4 bytes */
        case STGXOBJ_COLOR_TYPE_ARGB8888:                   /* used on gamma */
            Params1_p->AllocBlockParams.Alignment   = 4;
            Params1_p->Pitch                        = 4 * Bitmap_p->Width;
        break;

        /* 1pix -> 3 bytes */
        case STGXOBJ_COLOR_TYPE_ARGB8565:                   /* used on gamma */
        case STGXOBJ_COLOR_TYPE_RGB888:                     /* used on gamma */
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:        /* used on gamma */
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:      /* used on gamma */
            Params1_p->AllocBlockParams.Alignment   = 4;
            Params1_p->Pitch                        = (Bitmap_p->Width * 3);
        break;

        /* 1pix -> 1 bytes | 1pix -> 3 bytes */
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:  /* used on gamma+ still */
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:/* used on gamma+ still */
            Params1_p->AllocBlockParams.Alignment   = 4;
            if(HardUse == STGXOBJ_STILLPIC_CELL)
            {
                Params1_p->AllocBlockParams.Alignment   = 128;
            }
            Params1_p->Pitch = (Bitmap_p->Width*2)
                             + ((Bitmap_p->Width & 0x1) ? 2 : 0);
            break;


        /* 1pix -> 2 bytes */
        case STGXOBJ_COLOR_TYPE_RGB565:                     /* used on gamma */
        case STGXOBJ_COLOR_TYPE_ARGB1555:                   /* used on gamma */
        case STGXOBJ_COLOR_TYPE_ARGB4444:                   /* used on gamma */
        case STGXOBJ_COLOR_TYPE_ACLUT88:                    /* used on gamma */
            /* supported on BKL-7015; GDP-GX1; ... */
            Params1_p->AllocBlockParams.Alignment   = 2;
            Params1_p->Pitch                        = 2 * Bitmap_p->Width;
        break;

        /* 1pix -> 1 byte */
        case STGXOBJ_COLOR_TYPE_CLUT8:                      /* used on osd */
        case STGXOBJ_COLOR_TYPE_ACLUT44:
        case STGXOBJ_COLOR_TYPE_ALPHA8:
        case STGXOBJ_COLOR_TYPE_BYTE:
            Params1_p->AllocBlockParams.Alignment   = 1;   /* case gamma curs */
            if(HardUse == STGXOBJ_OSDFRAME_CELL)
            {
                Params1_p->AllocBlockParams.Alignment   = 8; /* case osd 5514 */
            }
            if(HardUse == STGXOBJ_OSDTOP_BOT_CELL)
            {
                Params1_p->AllocBlockParams.Alignment   = 128; /* case osd xx */
                Params1_p->Offset                       = 16;
            }
            Params1_p->Pitch                        = Bitmap_p->Width;
        break;

        /* 1pix -> 1/2 byte */
        case STGXOBJ_COLOR_TYPE_CLUT4:
        case STGXOBJ_COLOR_TYPE_ALPHA4:
            Params1_p->AllocBlockParams.Alignment   = 1;
            if(HardUse == STGXOBJ_OSDFRAME_CELL)
            {
                Params1_p->AllocBlockParams.Alignment   = 8;
            }
            if(HardUse == STGXOBJ_OSDTOP_BOT_CELL)
            {
                Params1_p->AllocBlockParams.Alignment   = 128;
                Params1_p->Offset                       = 16;
            }
            Params1_p->Pitch = (Bitmap_p->Width/2) + (Bitmap_p->Width & 0x1) ;
        break;

        /* 1pix -> 1/4 byte */
        case STGXOBJ_COLOR_TYPE_CLUT2:
            Params1_p->AllocBlockParams.Alignment   = 1;
            if(HardUse == STGXOBJ_OSDFRAME_CELL)
            {
                Params1_p->AllocBlockParams.Alignment   = 8;
            }
            if(HardUse == STGXOBJ_OSDTOP_BOT_CELL)
            {
                Params1_p->AllocBlockParams.Alignment   = 128;
                Params1_p->Offset                       = 16;
            }
            Params1_p->Pitch = (Bitmap_p->Width/4)
                             + ((Bitmap_p->Width%4) ? 1 : 0);
        break;

        /* 1pix -> 1/8 byte */
        case STGXOBJ_COLOR_TYPE_CLUT1:
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            Params1_p->AllocBlockParams.Alignment   = 1;
            Params1_p->Pitch = (Bitmap_p->Width/8) + ((Bitmap_p->Width%8)? 1:0);
        break;

        default:
            Params1_p->AllocBlockParams.Alignment = 1;
            Params1_p->AllocBlockParams.Size      = 0;
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        break;

    } /* end switch color type */

    /* common overwrites */
    /*-------------------*/
    /* Round pitch */
    Params1_p->Pitch = Params1_p->Pitch + Params1_p->Offset;
    if ((Params1_p->Pitch % Params1_p->AllocBlockParams.Alignment) != 0)
    {
        Params1_p->Pitch = Params1_p->Pitch
               + Params1_p->AllocBlockParams.Alignment
               - (Params1_p->Pitch % Params1_p->AllocBlockParams.Alignment);
    }

    /* Calculate block size */
    Params1_p->AllocBlockParams.Size = Params1_p->Pitch * Bitmap_p->Height;

    /* Special overwrites : Top-Bottom storage */
    /*-----------------------------------------*/
    if(Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM)
    {
        Params1_p->AllocBlockParams.Size    =
                                       (Params1_p->AllocBlockParams.Size / 2)
                                       + (Params1_p->AllocBlockParams.Size % 2);
        *Params2_p                          = *Params1_p;
    }

    /* Special overwrites : macro block / chroma - luma storage */
    /*----------------------------------------------------------*/
    else  if (Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB)
        /* Note that a MB is a 16*16 Pixel area */
    {
        if ((Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)
           && (Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
           && (Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420)
           && (Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        else
        {
            /* MB Horizontal */
            MBRow = Bitmap_p->Width / 16
                  + ((Bitmap_p->Width % 16 != 0)? 1:0);
            /* MB Vertical (MBCol must be even) */
            MBCol = Bitmap_p->Height / 16
                  + ((Bitmap_p->Height % 16 != 0)? 1:0);
            if ((MBCol % 2) != 0)
            {
                MBCol++;
            }
            /* Luma buffer related */
            Params1_p->AllocBlockParams.Size      = MBRow * MBCol * 256;
            Params1_p->AllocBlockParams.Alignment = 1;
            Params1_p->Pitch                      = MBRow * 16;
            Params1_p->Offset                     = 0;

            if ((Bitmap_p->ColorType
                               == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)
                ||(Bitmap_p->ColorType
                               == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422))
            {
                /* Chroma buffer related */
                *Params2_p = *Params1_p;
            }
            else  /* case : 888_420 Signed or Unsigned  */
            {
                if (((MBRow % 2) == 1)  && ((MBCol % 4) != 0))
                {
                    /* Chroma buffer related */
                    Params2_p->AllocBlockParams.Size
                                                = MBRow * (MBCol + 1) * 128;
                    Params2_p->AllocBlockParams.Alignment = 1;
                    Params2_p->Pitch            = Params1_p->Pitch;
                    Params2_p->Offset           = 0;
                }
                else
                {
                    /* Chroma buffer related */
                    Params2_p->AllocBlockParams.Size = MBRow * MBCol * 128;
                    Params2_p->AllocBlockParams.Alignment = 1;
                    Params2_p->Pitch                 = Params1_p->Pitch;
                    Params2_p->Offset                = 0;
                }
            } /* end if 888_420 Signed or Unsigned  */
        } /* end if supported */
    } /* end if strage macro-block */
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : STGXOBJ_GetPaletteAllocParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STGXOBJ_GetPaletteAllocParams(STGXOBJ_Palette_t*     Palette_p,
                                        STGXOBJ_HardUse_t             HardUse,
                                        STGXOBJ_PaletteAllocParams_t* Params_p )
{
    if ((Palette_p == NULL) || (Params_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (TestHardCoherence(HardUse) != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    switch(Palette_p->ColorType)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
            /* provided on gamma and on sub-pic (cursor 55xx) */
            Params_p->AllocBlockParams.Alignment    = 16;
            if(HardUse == STGXOBJ_SUBPIC_CELL)
            {
                Params_p->AllocBlockParams.Alignment   = 1;
            }
            Params_p->AllocBlockParams.Size = 4 * (1 << Palette_p->ColorDepth);
        break;

        case STGXOBJ_COLOR_TYPE_ARGB4444:
            /* provided on gamma */
            Params_p->AllocBlockParams.Alignment = 16;
            Params_p->AllocBlockParams.Size = 2 * (1 << Palette_p->ColorDepth);
        break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
            /* provided on OSD 55xx */
            Params_p->AllocBlockParams.Alignment = 4;
            Params_p->AllocBlockParams.Size =
                                     sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)
                                     * (1 << Palette_p->ColorDepth);
        break;

        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
            /* provided on 51xx/8010 */
            Params_p->AllocBlockParams.Alignment = 16;
            Params_p->AllocBlockParams.Size = 4 * (1 << Palette_p->ColorDepth);
        break;

        default:
            Params_p->AllocBlockParams.Alignment = 0;
            Params_p->AllocBlockParams.Size      = 0;
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        break;
    }
    return(ST_NO_ERROR);

}

/*----------------------------------------------------------------------------*/
