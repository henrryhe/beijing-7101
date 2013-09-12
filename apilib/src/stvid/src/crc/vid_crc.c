/*******************************************************************************

File name   : vid_crc.c

Description : CRC computation/Check module source file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
24 Aug 2006        Created                                           PC
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <string.h>
#include <assert.h>
#include <ctype.h>
#endif

#include "stdevice.h"
#include "stsys.h"
#include "vid_ctcm.h"
#include "vid_crc.h"

#ifndef STTBX_REPORT
    #define STTBX_REPORT
#endif /* STTBX_REPORT */
#include "sttbx.h"

#if defined(TRACE_VIDCRC)
#define TRACE_UART
#endif /* TRACE_VIDCRC */

#if defined TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */
#include "vid_trace.h"

#ifdef USE_COLOR_TRACES
/* Print STTBX message in red using ANSI Escape code */
/* Need a terminal supporting escape codes           */
#define STTBX_ErrorPrint(x)  \
{ \
    STTBX_Print(("\033[31m")); \
    STTBX_Print(x); \
    STTBX_Print(("\033[0m")); \
}
#else
#define STTBX_ErrorPrint(x)  STTBX_Print(x)
#endif


/* Private Constants -------------------------------------------------------- */
/* Defines for vidprod_SavePicture */
#define OMEGA2_420 0x94

#if defined(TRACE_UART) && defined(VALID_TOOLS)
#define VIDCRC_TRACE_BLOCKNAME "VIDCRC"

/* !!!! WARNING !!!! : Any modification of this table should be reported in emun in vid_disp.h */
static TRACE_Item_t vidcrc_TraceItemList[] =
{
    {"GLOBAL",          TRACE_VIDCRC_COLOR,                        TRUE, FALSE},
    {"CHECK",           TRACE_VIDCRC_COLOR,                        TRUE, FALSE},
    {"VD_CHECK",        TRACE_VIDCRC_COLOR,                        TRUE, FALSE}
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
extern char stvid_ValidPictureStructStr[3][2][3];
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

/* External functions ------------------------------------------------------- */
extern BOOL vidprod_SavePicture(U8 format, U32 width, U32 height, void * LumaAddress_p, void * ChromaAddress_p, char * Filename_p);

/* Private functions -------------------------------------------------------- */

#if defined(STVID_SOFTWARE_CRC_MODE)
/* -------------------------------------------------------------------------- */
/* Gam2yuv()                                                                  */
/* -------------------------------------------------------------------------- */
static void Gam2yuv(U32 Width,U32 Height,U32 *LumaInputBuffer,U32 *ChromaInputBuffer,U8 *LumaOutputBuffer,U8 *CbOutputBuffer,U8 *CrOutputBuffer)
{
    U32     rows=0;
    U32     cols=0;
    U32     LumaBytes=0;
    U32     ChromaBytes=0;

    U8      IsCrPixel;
    int     i;
    int     j;
    int     PixPerU32;
    U32     MBSize;
    U32   PictureMBWidth;
    U32   PictureMBHeight;
    U32   StoredMBWidth;
    U32   StoredMBHeight;
    U32   CurrentMBx;
    U32   CurrentMBy;
    U32   CurrentMBSubY;
    U32   MBNumber;
    U32   captured_word = 0;
    U32   wordYPix;
    U32   wordCPix;
    U32   TmpU32;
    U32   PixelX;
    U32   PixelY;
    U32   PixelTotX;
    U32   PixelTotY;

    rows = ((Height+31) >> 5) << 1;
    cols = (Width+15) >> 4;
    LumaBytes = rows*cols*256;  /* size of luma data buffer */
    ChromaBytes = (cols & 1) && ((rows>>1) & 1) ? rows*(cols+1)*128 : rows*cols*128;    /* size of chroma data buffer */


    PixPerU32 = 4;
    MBSize = 256;
    /* Picture Width must be an integer number of macrobloc */
    PictureMBWidth  = (Width  + 15) / 16;
    /* Picture Height must be an integer number of macrobloc */
    PictureMBHeight = (Height + 15) / 16;

    /* Translate Luma */
    StoredMBWidth = PictureMBWidth;
    /* Stored Picture Height must be an even number of macrobloc */
    StoredMBHeight = ((PictureMBHeight + 1) / 2) * 2;
    MBNumber = 0;
    for(CurrentMBy = 0 ; CurrentMBy < StoredMBHeight ; CurrentMBy += 2)
    {
        for(CurrentMBx = 0 ; CurrentMBx < StoredMBWidth ; CurrentMBx++)
        {
            for(CurrentMBSubY = 0 ; CurrentMBSubY < 2 ; CurrentMBSubY++)
            {
                for (i=0 , j = 0 ; i < MBSize; i++, j = (j+1) % PixPerU32)
                {
                    if(j == 0)
                    {
                            captured_word = LumaInputBuffer[((CurrentMBy*StoredMBWidth + CurrentMBx*2 + CurrentMBSubY)*MBSize+i)/PixPerU32];
                    }
                    captured_word = captured_word & 0xFFFFFFFF;

                    wordYPix = (captured_word >> (8*j)) & 0xFF;

                    if((CurrentMBx < PictureMBWidth) &&
                       ((CurrentMBy + CurrentMBSubY) < PictureMBHeight))
                    {
                        TmpU32 = (i / 16) * 16;
                        TmpU32 += 15-(i % 16);
                        PixelX = TmpU32 % 8;
                        if((TmpU32 / 64) % 2)
                            PixelX += 8;
                        PixelY = (TmpU32 % 64) / 8;
                        PixelY *= 2;
                        if(TmpU32 >= 128)
                            PixelY++;
                        PixelTotX = (CurrentMBx*16) + PixelX;
                        PixelTotY = ((CurrentMBy + CurrentMBSubY)*16) + PixelY;

                        if((PixelTotX < Width) && (PixelTotY < Height))
                        {
                            LumaOutputBuffer[PixelTotX+PixelTotY*Width] = wordYPix;
                        }
                    }
                }
                MBNumber++;
            }
        }
    }

    /* Chroma */
    MBSize = 128;
    for(MBNumber = 0 ; MBNumber < ChromaBytes/MBSize ; MBNumber++)
    {
        CurrentMBy = (MBNumber / (PictureMBWidth*4)) * 4;
        TmpU32 = MBNumber % (PictureMBWidth*4);
        CurrentMBx = (TmpU32 / 4) * 2;
        if(TmpU32 & 1)
            CurrentMBx++;
        if(TmpU32/2 & 1)
        {
            CurrentMBy++;
        }
        if(CurrentMBx >= PictureMBWidth)
        {
            CurrentMBx -= PictureMBWidth;
            CurrentMBy += 2;
        }
        for (i=0 , j = 0 ; i < MBSize; i++, j = (j+1) % PixPerU32)
        {
            if(j == 0)
            {
                captured_word = ChromaInputBuffer[((MBNumber*MBSize)+i)/PixPerU32];
            }
            captured_word =  captured_word & 0xFFFFFFFF;
            wordCPix  = (captured_word >> (8*j)) & 0xFF;

            if((CurrentMBx < PictureMBWidth) &&
               (CurrentMBy < PictureMBHeight))
            {

                TmpU32 = (i / 16) * 16;
                TmpU32 += 15-(i % 16);
                if((TmpU32 / 4) & 1)
                    IsCrPixel = TRUE;
                else
                    IsCrPixel = FALSE;
                PixelX = (TmpU32 % 4) * 2;
                if((TmpU32 / 32) % 2)
                    PixelX += 8;
                MBSize = 128;
                PixelY = (TmpU32 % 32) / 8;
                PixelY *= 4;
                if(TmpU32 >= 64)
                    PixelY += 2;
                PixelTotX = (CurrentMBx*16) + PixelX;
                PixelTotY = (CurrentMBy*16) + PixelY;
                if((PixelTotX < Width) && (PixelTotY < Height))
                {
                    if(IsCrPixel)
                    {
                        CrOutputBuffer[PixelTotX/2+(PixelTotY/2)*(Width/2)] = wordCPix;
                    }
                    else
                    {
                        CbOutputBuffer[PixelTotX/2+(PixelTotY/2)*(Width/2)] = wordCPix;
                    }
                }
            }
        }
    }
} /* End of Gam2yuv() function */

/* -------------------------------------------------------------------------- */
/* vidcrc_InitCRC32table()                                                    */
/*      Initializes CRC coeficients table                                     */
/* -------------------------------------------------------------------------- */
static void vidcrc_InitCRC32table(VIDCRC_Data_t *VIDCRC_Data_p)
{
    U32 i;
    U32 j;
    U32 crc_precalc;
    U32 POLYNOM = 0x04c11db7;

    for (i = 0; i < 256; i++)
    {
        crc_precalc = i << 24;
        for (j = 0; j < 8; j++)
        {
            if ((crc_precalc & 0x80000000)==0)
                crc_precalc = (crc_precalc << 1);
            else
                crc_precalc = (crc_precalc << 1) ^  POLYNOM;
        }
        VIDCRC_Data_p->crc32_table[i] = crc_precalc;
    }
} /* End of vidcrc_InitCRC32table() function */

/* -------------------------------------------------------------------------- */
/* vidcrc_CalcCRC()                                                           */
/*      Computes CRC on a given picture                                       */
/* -------------------------------------------------------------------------- */
static ST_ErrorCode_t vidcrc_CalcCRC(const VIDCRC_Data_t *VIDCRC_Data_p,
                                     U32 Width, U32 Height, STVID_PictureStructure_t PictureStructure,
                                     U8 *LumaAddress, U8 *CbAddress, U8 *CrAddress,
                                     U32 *LumaCRC, U32 *ChromaCRC)
{
    U32 size_Y;
    U32 size_C;
    U32 CRC_Y;
    U32 CRC_C;
    U32 i;
    U32 j;

    size_Y = Width * Height;
    size_C = size_Y/2;

/*init crc*/
    CRC_Y = 0xffffffff;
    CRC_C = 0xffffffff;

    if(PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
    {   /* Frame not MBAFF */
        for (i=0;i<size_Y;i++)
            CRC_Y = (CRC_Y << 8) ^ VIDCRC_Data_p->crc32_table[(CRC_Y >> 24) ^ *LumaAddress++];
        for (i=0;i<size_C;i++)
        {
            CRC_C = (CRC_C << 8) ^ VIDCRC_Data_p->crc32_table[(CRC_C >> 24) ^ *CbAddress++];
            CRC_C = (CRC_C << 8) ^ VIDCRC_Data_p->crc32_table[(CRC_C >> 24) ^ *CrAddress++];
        }
    }
    else if(PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
    {   /* Top Field */
        for (i=0;i<Height;i++)
        {
            for (j=0;j<Width;j++)
            {

                if(i%2==0)
                    CRC_Y = (CRC_Y << 8) ^ VIDCRC_Data_p->crc32_table[(CRC_Y >> 24) ^ *LumaAddress++];
                else
                    LumaAddress++;
            }
        }
        for (i=0;i<Height/2;i++)
        {
            for (j=0;j<Width/2;j++)
            {
                if(i%2==0)
                {
                    CRC_C = (CRC_C << 8) ^ VIDCRC_Data_p->crc32_table[(CRC_C >> 24) ^ *CbAddress++];
                    CRC_C = (CRC_C << 8) ^ VIDCRC_Data_p->crc32_table[(CRC_C >> 24) ^ *CrAddress++];
                }
                else
                {
                    CbAddress++;
                    CrAddress++;
                }
            }
        }
    }
    else if(PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
    {   /* Bottom Field */
        for (i=0;i<Height;i++)
        {
            for (j=0;j<Width;j++)
            {

            if(i%2==1)
                CRC_Y = (CRC_Y << 8) ^ VIDCRC_Data_p->crc32_table[(CRC_Y >> 24) ^ *LumaAddress++];
            else
                LumaAddress++;
            }
        }
        for (i=0;i<Height/2;i++)
        {
            for (j=0;j<Width/2;j++)
            {
                if(i%2==1)
                {
                    CRC_C = (CRC_C << 8) ^ VIDCRC_Data_p->crc32_table[(CRC_C >> 24) ^ *CbAddress++];
                    CRC_C = (CRC_C << 8) ^ VIDCRC_Data_p->crc32_table[(CRC_C >> 24) ^ *CrAddress++];
                }
                else
                {
                    CbAddress++;
                    CrAddress++;
                }
            }
        }
    }

    return ST_NO_ERROR;
} /* End of vidcrc_CalcCRC() function */
#endif /* defined(STVID_SOFTWARE_CRC_MODE) */

/* -------------------------------------------------------------------------- */
/* vidcrc_CompareCRC()                                                        */
/*      Compares CRC taking IsMonochrome flag into account                    */
/* -------------------------------------------------------------------------- */
static void vidcrc_CompareCRC(VIDCRC_Data_t *VIDCRC_Data_p, STVID_PictureInfos_t *PictureInfo, U32 RefCRCIndex, U32 LumaCRC, U32 ChromaCRC, BOOL *LumaCRCMatch, BOOL *ChromaCRCMatch)
{
    *LumaCRCMatch = (LumaCRC == VIDCRC_Data_p->RefCRCTable[RefCRCIndex % VIDCRC_Data_p->NbPictureInRefCRC].LumaCRC);
    if((PictureInfo->VideoParams.IsMonochrome) &&
       (VIDCRC_Data_p->VideoStandard == VIDCRC_VIDEO_STANDARD_H264))
    {
        *ChromaCRCMatch = TRUE;
    }
    else
    {
        *ChromaCRCMatch = (ChromaCRC == VIDCRC_Data_p->RefCRCTable[RefCRCIndex % VIDCRC_Data_p->NbPictureInRefCRC].ChromaCRC);
    }
} /* End of vidcrc_CompareCRC() function */

/*----------------------------------------------------
   vidcrc_CheckCRC()
        Check and register CRC for picture passed as parameter.
Returns     : ST_NO_ERROR on success, error code on fail
 * --------------------------------------------------*/
static ST_ErrorCode_t vidcrc_CheckCRC(const VIDCRC_Handle_t CRCHandle,
                                STVID_PictureInfos_t *PictureInfos_p,
                                VIDCOM_PictureID_t ExtendedPresentationOrderPictureID,
                                U32 DecodingOrderFrameID, U32 PictureNumber,
                                BOOL IsRepeatedLastPicture, BOOL IsRepeatedLastButOnePicture,
                                void *LumaAddress, void *ChromaAddress,
                                U32 LumaCRC, U32 ChromaCRC)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    VIDCRC_Data_t              *VIDCRC_Data_p;
    U32                         ExpectedRefPictureIndex;
    STVID_CRCFieldInversion_t   InvertedFields;
    STVID_CRCErrorLogEntry_t   *ErrorLogEntry;
    STVID_CompCRCEntry_t       *CompCRCEntry;
#ifdef STVID_CRC_ALLOW_FRAME_DUMP
    char                        Filename[FILENAME_MAX];
#endif
    BOOL                        LumaCRCMatch=FALSE;
    BOOL                        InvertedFieldsLumaCRCMatch=FALSE;
    BOOL                        ChromaCRCMatch=FALSE;
    BOOL                        InvertedFieldsChromaCRCMatch=FALSE;
    BOOL                        LogError=FALSE;
    BOOL                        NewCompCRCEntry=FALSE;
    BOOL                        IsFieldCRC=FALSE;

#ifndef STVID_CRC_ALLOW_FRAME_DUMP
    UNUSED_PARAMETER(LumaAddress);
    UNUSED_PARAMETER(ChromaAddress);
#endif

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    IsFieldCRC = VIDCRC_IsCRCModeField(CRCHandle, PictureInfos_p, VIDCRC_Data_p->VideoStandard);

    /* Compute index of reference picture */
    if(IsRepeatedLastPicture)
    {
        if(VIDCRC_Data_p->NextRefCRCIndex < 1)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "First picture can't be repeated previous one !!! BUG !"));
            ExpectedRefPictureIndex = VIDCRC_Data_p->NextRefCRCIndex; /* Set only to avoid crash */
        }
        else
        {
            ExpectedRefPictureIndex = VIDCRC_Data_p->NextRefCRCIndex - 1;
        }
    }
    else if(IsRepeatedLastButOnePicture)
    {
        if(VIDCRC_Data_p->NextRefCRCIndex < 2)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "First picture can't be repeated previous one !!! BUG !"));
            ExpectedRefPictureIndex = VIDCRC_Data_p->NextRefCRCIndex; /* Set only to avoid crash */
        }
        else
        {
            ExpectedRefPictureIndex = VIDCRC_Data_p->NextRefCRCIndex - 2;
        }
    }
    else
    {
        ExpectedRefPictureIndex = VIDCRC_Data_p->NextRefCRCIndex;
    }

    /* Check if computed CRC matches expected picture CRC */
    InvertedFields = STVID_CRC_INVFIELD_NONE;
    vidcrc_CompareCRC(VIDCRC_Data_p, PictureInfos_p, ExpectedRefPictureIndex, LumaCRC, ChromaCRC, &LumaCRCMatch, &ChromaCRCMatch);
    if((!LumaCRCMatch ||
        !ChromaCRCMatch) &&
        IsFieldCRC)
    {   /* If CRC doesn't match and
           If field picture or H264 MBAFF picture, try to check with inverted fields */
        /* First try with next Ref CRC */
        vidcrc_CompareCRC(VIDCRC_Data_p, PictureInfos_p, ExpectedRefPictureIndex+1, LumaCRC, ChromaCRC, &InvertedFieldsLumaCRCMatch, &InvertedFieldsChromaCRCMatch);
        if(InvertedFieldsLumaCRCMatch && InvertedFieldsChromaCRCMatch)
        {
            InvertedFields = STVID_CRC_INVFIELD_WITH_NEXT;
            ExpectedRefPictureIndex++;
            LumaCRCMatch = InvertedFieldsLumaCRCMatch;
            ChromaCRCMatch = InvertedFieldsChromaCRCMatch;
        }
        else if(ExpectedRefPictureIndex >= 1)
        { /* If doesn't match with next Ref CRC, tryprevious one if available*/
            vidcrc_CompareCRC(VIDCRC_Data_p, PictureInfos_p, ExpectedRefPictureIndex-1, LumaCRC, ChromaCRC, &InvertedFieldsLumaCRCMatch, &InvertedFieldsChromaCRCMatch);
            if(InvertedFieldsLumaCRCMatch && InvertedFieldsChromaCRCMatch)
            {
                InvertedFields = STVID_CRC_INVFIELD_WITH_PREVIOUS;
                ExpectedRefPictureIndex--;
                LumaCRCMatch = InvertedFieldsLumaCRCMatch;
                ChromaCRCMatch = InvertedFieldsChromaCRCMatch;
            }
        }
    }
#if defined(TRACE_UART) && defined(VALID_TOOLS)
    {
        char ResultString[20];
        if(LumaCRCMatch && ChromaCRCMatch)
        {
            switch(InvertedFields)
            {
                case STVID_CRC_INVFIELD_NONE :
                    strcpy(ResultString, "OK");
                    break;
                case STVID_CRC_INVFIELD_WITH_NEXT :
                    strcpy(ResultString, "OK_INV_NEXT");
                    break;
                case STVID_CRC_INVFIELD_WITH_PREVIOUS :
                    strcpy(ResultString, "OK_INV_PREV");
                    break;
            }
        }
        else
        {
            if(!LumaCRCMatch && ChromaCRCMatch)
            {
                strcpy(ResultString, "KO_LUMA");
            }
            else if(LumaCRCMatch && !ChromaCRCMatch)
            {
                strcpy(ResultString, "KO_CHROMA");
            }
            else
            {
                strcpy(ResultString, "KO_BOTH");
            }
        }
        if(IsRepeatedLastPicture)
        {
            strcat(ResultString, "_REP");
        }
        if(IsRepeatedLastButOnePicture)
        {
            strcat(ResultString, "_REPLBO");
        }
        TraceBufferColorConditional((VIDCRC_TRACE_BLOCKID, VIDCRC_TRACE_CHECK, "CRC_%s(%d-%d/%d-%s)[%u/%u]Ref(%u:%u/%u) ",
                                                    ResultString,
                                                    DecodingOrderFrameID,
                                                    ExtendedPresentationOrderPictureID.IDExtension,
                                                    ExtendedPresentationOrderPictureID.ID,
                                                    stvid_ValidPictureStructStr[PictureInfos_p->VideoParams.PictureStructure][PictureInfos_p->VideoParams.TopFieldFirst],
                                                    LumaCRC,
                                                    ChromaCRC,
                                                    ExpectedRefPictureIndex,
                                                    VIDCRC_Data_p->RefCRCTable[ExpectedRefPictureIndex % VIDCRC_Data_p->NbPictureInRefCRC].LumaCRC,
                                                    VIDCRC_Data_p->RefCRCTable[ExpectedRefPictureIndex % VIDCRC_Data_p->NbPictureInRefCRC].ChromaCRC));
        TRACE_VisualConditional(VIDCRC_TRACE_BLOCKID, VIDCRC_TRACE_VD_CHECK, TRC_VISUAL_MESSAGE, "CRC", "CRC_%s(%d-%d/%d-%s)[%u/%u]Ref(%u:%u/%u) ",
                                                ResultString,
                                                DecodingOrderFrameID,
                                                ExtendedPresentationOrderPictureID.IDExtension,
                                                ExtendedPresentationOrderPictureID.ID,
                                                stvid_ValidPictureStructStr[PictureInfos_p->VideoParams.PictureStructure][PictureInfos_p->VideoParams.TopFieldFirst],
                                                LumaCRC,
                                                ChromaCRC,
                                                ExpectedRefPictureIndex,
                                                VIDCRC_Data_p->RefCRCTable[ExpectedRefPictureIndex % VIDCRC_Data_p->NbPictureInRefCRC].LumaCRC,
                                                VIDCRC_Data_p->RefCRCTable[ExpectedRefPictureIndex % VIDCRC_Data_p->NbPictureInRefCRC].ChromaCRC);
    }
#endif

    NewCompCRCEntry = TRUE;
    if(IsRepeatedLastPicture)
    {
        if(VIDCRC_Data_p->NextCompCRCIndex >= 1)
        {
            CompCRCEntry = &(VIDCRC_Data_p->CompCRCTable[(VIDCRC_Data_p->NextCompCRCIndex-1) % VIDCRC_Data_p->MaxNbPictureInCompCRC]);
            if((ExtendedPresentationOrderPictureID.ID == CompCRCEntry->ID) &&
               (ExtendedPresentationOrderPictureID.IDExtension == CompCRCEntry->IDExtension) &&
               (LumaCRC == CompCRCEntry->LumaCRC) &&
               (ChromaCRC == CompCRCEntry->ChromaCRC))
            {   /* If repeated picture with same CRCs, increment repeat counter and don't log again */
                CompCRCEntry->NbRepeatedLastPicture++;
                NewCompCRCEntry = FALSE;
            }
        }
    }
    if(IsRepeatedLastButOnePicture)
    {
        if(VIDCRC_Data_p->NextCompCRCIndex >= 2)
        {
            CompCRCEntry = &(VIDCRC_Data_p->CompCRCTable[(VIDCRC_Data_p->NextCompCRCIndex-2) % VIDCRC_Data_p->MaxNbPictureInCompCRC]);
            if((ExtendedPresentationOrderPictureID.ID == CompCRCEntry->ID) &&
               (ExtendedPresentationOrderPictureID.IDExtension == CompCRCEntry->IDExtension) &&
               (LumaCRC == CompCRCEntry->LumaCRC) &&
               (ChromaCRC == CompCRCEntry->ChromaCRC))
            {   /* If repeated picture with same CRCs, increment repeat counter and don't log again */
                CompCRCEntry->NbRepeatedLastButOnePicture++;
                NewCompCRCEntry = FALSE;
            }
        }
    }
    if(NewCompCRCEntry)
    {
        /* Log computed CRC values */
        CompCRCEntry = &(VIDCRC_Data_p->CompCRCTable[VIDCRC_Data_p->NextCompCRCIndex % VIDCRC_Data_p->MaxNbPictureInCompCRC]);
        CompCRCEntry->ID                            = ExtendedPresentationOrderPictureID.ID;
        CompCRCEntry->IDExtension                   = ExtendedPresentationOrderPictureID.IDExtension;
        CompCRCEntry->DecodingOrderFrameID          = DecodingOrderFrameID;
        CompCRCEntry->LumaCRC                       = LumaCRC;
        CompCRCEntry->ChromaCRC                     = ChromaCRC;
        CompCRCEntry->FieldPicture                  = (PictureInfos_p->VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME);
        CompCRCEntry->FieldCRC                      = IsFieldCRC;
        CompCRCEntry->IsMonochrome                  = (PictureInfos_p->VideoParams.IsMonochrome &&
                                                       (VIDCRC_Data_p->VideoStandard == VIDCRC_VIDEO_STANDARD_H264));
        CompCRCEntry->NbRepeatedLastPicture         = 0;
        CompCRCEntry->NbRepeatedLastButOnePicture   = 0;
        VIDCRC_Data_p->NextCompCRCIndex++;
    }
    /* If CRC error detected then log error */
#ifdef FIRST_FIELD_ISSUE_WA
    /* if this is the first CRC values received through vidcrc_CheckCRC */
    /* Don't report error */
    if(VIDCRC_Data_p->IsFirstPicture)
    {
        VIDCRC_Data_p->IsFirstPicture = FALSE;
    }
    else
    {
#endif /* FIRST_FIELD_ISSUE_WA */
    LogError = FALSE;
    if(!LumaCRCMatch ||
       !ChromaCRCMatch ||
       (InvertedFields != STVID_CRC_INVFIELD_NONE))
    {
        if(IsRepeatedLastPicture)
        {
            if(VIDCRC_Data_p->NextErrorLogIndex >= 1)
            {
                ErrorLogEntry = &(VIDCRC_Data_p->ErrorLogTable[(VIDCRC_Data_p->NextErrorLogIndex-1) % VIDCRC_Data_p->MaxNbPictureInErrorLog]);
                if((ExpectedRefPictureIndex != ErrorLogEntry->RefCRCIndex) ||
                   (LumaCRC != ErrorLogEntry->LumaCRC) ||
                   (ChromaCRC != ErrorLogEntry->ChromaCRC))
                {   /* if not same error as previous occurence of this picture, log new error */
                    STTBX_ErrorPrint(("CRC Error for repeated image %d !!\n", PictureNumber));
                    TraceOn(("CRC Error for repeated image %d !!\n", PictureNumber));
                    LogError = TRUE;
                }
            }
        }
        else if(IsRepeatedLastButOnePicture)
        {
            if(VIDCRC_Data_p->NextErrorLogIndex >= 2)
            {
                ErrorLogEntry = &(VIDCRC_Data_p->ErrorLogTable[(VIDCRC_Data_p->NextErrorLogIndex-2) % VIDCRC_Data_p->MaxNbPictureInErrorLog]);
                if((ExpectedRefPictureIndex != ErrorLogEntry->RefCRCIndex) ||
                   (LumaCRC != ErrorLogEntry->LumaCRC) ||
                   (ChromaCRC != ErrorLogEntry->ChromaCRC))
                {   /* if same error as previous occurence of this picture, don't log again */
                    STTBX_ErrorPrint(("CRC Error for repeated last image %d !!\n", PictureNumber));
                    TraceOn(("CRC Error for last repeated image %d !!\n", PictureNumber));
                    LogError = TRUE;
                }
            }
        }
        else
        {
            STTBX_ErrorPrint(("CRC Error for non repeated image %d !!\n", PictureNumber));
            TraceOn(("CRC Error for non repeated image %d !!\n", PictureNumber));
            LogError = TRUE;
        }
    }
    if(LogError)
    {
        ErrorLogEntry = &(VIDCRC_Data_p->ErrorLogTable[VIDCRC_Data_p->NextErrorLogIndex % VIDCRC_Data_p->MaxNbPictureInErrorLog]);
        ErrorLogEntry->RefCRCIndex              = ExpectedRefPictureIndex;
        ErrorLogEntry->CompCRCIndex             = VIDCRC_Data_p->NextCompCRCIndex - 1;
        ErrorLogEntry->ID                       = ExtendedPresentationOrderPictureID.ID;
        ErrorLogEntry->IDExtension              = ExtendedPresentationOrderPictureID.IDExtension;
        ErrorLogEntry->DecodingOrderFrameID     = DecodingOrderFrameID;
        ErrorLogEntry->LumaCRC                  = LumaCRC;
        ErrorLogEntry->ChromaCRC                = ChromaCRC;
        ErrorLogEntry->LumaError                = !LumaCRCMatch;
        ErrorLogEntry->ChromaError              = !ChromaCRCMatch;
        ErrorLogEntry->InvertedFields           = InvertedFields;
        ErrorLogEntry->FieldPicture             = (PictureInfos_p->VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME);
        ErrorLogEntry->FieldCRC                 = IsFieldCRC;
        ErrorLogEntry->IsMonochrome             = (PictureInfos_p->VideoParams.IsMonochrome &&
                                                   (VIDCRC_Data_p->VideoStandard == VIDCRC_VIDEO_STANDARD_H264));
        VIDCRC_Data_p->NextErrorLogIndex++;
        if(!LumaCRCMatch || !ChromaCRCMatch)
        {
            VIDCRC_Data_p->NbErrors++;
        }

        if(VIDCRC_Data_p->DumpFailingFrames &&
           (!LumaCRCMatch ||
            !ChromaCRCMatch))
        {
#ifdef STVID_CRC_ALLOW_FRAME_DUMP
            sprintf(Filename, "../../decoded_frames/frame%03d_%03d_%03d_%03d",
                      DecodingOrderFrameID,
                      ExtendedPresentationOrderPictureID.IDExtension,
                      ExtendedPresentationOrderPictureID.ID,
                      PictureNumber);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Dumping %s file because of failing CRC", Filename));
            vidprod_SavePicture(OMEGA2_420, PictureInfos_p->BitmapParams.Width, PictureInfos_p->BitmapParams.Height,
                                    STAVMEM_DeviceToCPU(LumaAddress, &VIDCRC_Data_p->VirtualMapping),
                                    STAVMEM_DeviceToCPU(ChromaAddress, &VIDCRC_Data_p->VirtualMapping),
                                    Filename);
#else
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Dumping of frame is not allowed"));
#endif
        }
    }
#ifdef FIRST_FIELD_ISSUE_WA
    }
#endif /* FIRST_FIELD_ISSUE_WA */

    /* Increment Next Reference CRC index only if current picture is not a repeated one */
    if(!IsRepeatedLastPicture && !IsRepeatedLastButOnePicture)
    {
        VIDCRC_Data_p->NextRefCRCIndex++;
    }

    if((VIDCRC_Data_p->NbPicturesToCheck != 0) &&
       (VIDCRC_Data_p->NextRefCRCIndex >= VIDCRC_Data_p->NbPicturesToCheck))
    {   /* If Nb of requested picture reached, stop CRC check and report results */
        VIDCRC_CRCStopCheck(CRCHandle);
    }
    return(ErrorCode);
} /* End of vidcrc_CheckCRC() function */

/* Public functions --------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* VIDCRC_Init()                                                    */
/* -------------------------------------------------------------------------- */
ST_ErrorCode_t VIDCRC_Init(const VIDCRC_InitParams_t * const InitParams_p,
                  VIDCRC_Handle_t * const CRCHandle_p)
{
    ST_ErrorCode_t      ErrorCode;
    STEVT_OpenParams_t  STEVT_OpenParams;
    VIDCRC_Data_t      *VIDCRC_Data_p;

    assert(InitParams_p != NULL);
    /* Allocate the main structure for the Display module. */
    SAFE_MEMORY_ALLOCATE(VIDCRC_Data_p, VIDCRC_Data_t*, InitParams_p->CPUPartition_p, sizeof(VIDCRC_Data_t));
    if (VIDCRC_Data_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Memory allocation failed\n"));
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *CRCHandle_p = (VIDCRC_Handle_t *) VIDCRC_Data_p;

    VIDCRC_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;
    VIDCRC_Data_p->DecoderNumber = InitParams_p->DecoderNumber;
    VIDCRC_Data_p->DeviceType = InitParams_p->DeviceType;
    switch(VIDCRC_Data_p->DeviceType)
    {
        case    STVID_DEVICE_TYPE_5508_MPEG :
        case    STVID_DEVICE_TYPE_5510_MPEG :
        case    STVID_DEVICE_TYPE_5512_MPEG :
        case    STVID_DEVICE_TYPE_5514_MPEG :
        case    STVID_DEVICE_TYPE_5516_MPEG :
        case    STVID_DEVICE_TYPE_5517_MPEG :
        case    STVID_DEVICE_TYPE_5518_MPEG :
        case    STVID_DEVICE_TYPE_7015_MPEG :
        case    STVID_DEVICE_TYPE_7020_MPEG :
        case    STVID_DEVICE_TYPE_5528_MPEG :
        case    STVID_DEVICE_TYPE_5100_MPEG :
/*        case    STVID_DEVICE_TYPE_5107_MPEG : Same a previous */
        case    STVID_DEVICE_TYPE_5525_MPEG :
        case    STVID_DEVICE_TYPE_7710_MPEG :
        case    STVID_DEVICE_TYPE_5105_MPEG :
/*        case    STVID_DEVICE_TYPE_5188_MPEG : Same as previous */
        case    STVID_DEVICE_TYPE_STD2000_MPEG :
        case    STVID_DEVICE_TYPE_5301_MPEG :
        case    STVID_DEVICE_TYPE_7100_MPEG :
        case    STVID_DEVICE_TYPE_7109_MPEG :
            VIDCRC_Data_p->VideoStandard = VIDCRC_VIDEO_STANDARD_MPEG2;
            break;
        case    STVID_DEVICE_TYPE_7109D_MPEG :
        case    STVID_DEVICE_TYPE_7200_MPEG :
        case    STVID_DEVICE_TYPE_ZEUS_MPEG :
            VIDCRC_Data_p->VideoStandard = VIDCRC_VIDEO_STANDARD_MPEG2_DELTA;
            break;
        case    STVID_DEVICE_TYPE_7100_H264 :
        case    STVID_DEVICE_TYPE_7109_H264 :
        case    STVID_DEVICE_TYPE_7200_H264 :
        case    STVID_DEVICE_TYPE_ZEUS_H264 :
            VIDCRC_Data_p->VideoStandard = VIDCRC_VIDEO_STANDARD_H264;
            break;
        case    STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case    STVID_DEVICE_TYPE_7109_MPEG4P2 :
        case    STVID_DEVICE_TYPE_7200_MPEG4P2 :
            VIDCRC_Data_p->VideoStandard = VIDCRC_VIDEO_STANDARD_MPEG4P2;
            break;
        case    STVID_DEVICE_TYPE_7109_VC1 :
        case    STVID_DEVICE_TYPE_7200_VC1 :
        case    STVID_DEVICE_TYPE_ZEUS_VC1 :
            VIDCRC_Data_p->VideoStandard = VIDCRC_VIDEO_STANDARD_VC1;
            break;
        default :
            VIDCRC_Data_p->VideoStandard = VIDCRC_VIDEO_STANDARD_UNKNOWN;
            break;
    }

    VIDCRC_Data_p->State = VIDCRC_STATE_STOPPED;
    VIDCRC_Data_p->RefCRCTable = NULL;
    VIDCRC_Data_p->NbPictureInRefCRC = 0;
    VIDCRC_Data_p->CompCRCTable = NULL;
    VIDCRC_Data_p->MaxNbPictureInCompCRC = 0;
    VIDCRC_Data_p->ErrorLogTable = NULL;
    VIDCRC_Data_p->MaxNbPictureInErrorLog = 0;
#ifdef VALID_TOOLS
    VIDCRC_Data_p->MSGLOG_Handle = 0;
#endif /* VALID_TOOLS */

#ifdef STVID_SOFTWARE_CRC_MODE
    vidcrc_InitCRC32table(VIDCRC_Data_p);
    VIDCRC_Data_p->YUVBuffer.BufferSize = 0;
    VIDCRC_Data_p->YUVBuffer.Buffer = NULL;
    VIDCRC_Data_p->YUVBuffer.CbAddress = NULL;
    VIDCRC_Data_p->YUVBuffer.CrAddress = NULL;
#endif /* STVID_SOFTWARE_CRC_MODE */

    STAVMEM_GetSharedMemoryVirtualMapping(&(VIDCRC_Data_p->VirtualMapping));

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName,
                                            &STEVT_OpenParams,
                                            &(VIDCRC_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        VIDCRC_Term(*CRCHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDCRC_Data_p->EventsHandle,
                                                                        InitParams_p->VideoName,
                                                                        STVID_END_OF_CRC_CHECK_EVT,
                                                                        &(VIDCRC_Data_p->RegisteredEventsID
                                                                        [VIDCRC_END_OF_CRC_CHECK_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        VIDCRC_Term(*CRCHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    ErrorCode = STEVT_RegisterDeviceEvent(VIDCRC_Data_p->EventsHandle,
                                                                        InitParams_p->VideoName,
                                                                        STVID_NEW_CRC_QUEUED_EVT,
                                                                        &(VIDCRC_Data_p->RegisteredEventsID
                                                                        [VIDCRC_NEW_CRC_QUEUED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        VIDCRC_Term(*CRCHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    return(ST_NO_ERROR);
}/* end VIDCRC_Init */

/*******************************************************************************
Name        : VIDCRC_Term
Description : Terminate CRC computation/Check module undo all what was done at init
Parameters  : CRC handle
Assumptions : Term can be called in init when init process fails: function
                to undo things done at init should not cause trouble to the
                system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDCRC_Term(const VIDCRC_Handle_t CRCHandle)
{
    VIDCRC_Data_t * VIDCRC_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

#ifdef STVID_SOFTWARE_CRC_MODE
    if(VIDCRC_Data_p->YUVBuffer.BufferSize != 0)
    {
        SAFE_MEMORY_DEALLOCATE(VIDCRC_Data_p->YUVBuffer.Buffer, VIDCRC_Data_p->CPUPartition_p, VIDCRC_Data_p->YUVBuffer.BufferSize);
        VIDCRC_Data_p->YUVBuffer.Buffer = NULL;
    }
#endif /* STVID_SOFTWARE_CRC_MODE */

    ErrorCode = STEVT_Close(VIDCRC_Data_p->EventsHandle);

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDCRC_Data_p, VIDCRC_Data_p->CPUPartition_p, sizeof(VIDCRC_Data_t));

    return(ErrorCode);
}/* end VIDCRC_Term */

#ifdef VALID_TOOLS
/*----------------------------------------------------
   VIDCRC_SetMSGLOGHandle
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if parameter problem
              STVID_ERROR_MEMORY_ACCESS if memory copy fails
 * --------------------------------------------------*/
ST_ErrorCode_t VIDCRC_SetMSGLOGHandle(const VIDCRC_Handle_t CRCHandle, MSGLOG_Handle_t MSGLOG_Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDCRC_Data_t * VIDCRC_Data_p;

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    VIDCRC_Data_p->MSGLOG_Handle = MSGLOG_Handle;

    return(ErrorCode);
} /* End of VIDCRC_SetMSGLOGHandle() function */
#endif /* VALID_TOOLS */

/*----------------------------------------------------
   VIDCRC_CRCStartCheck
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if parameter problem
              STVID_ERROR_MEMORY_ACCESS if memory copy fails
 * --------------------------------------------------*/
ST_ErrorCode_t VIDCRC_CRCStartCheck(const VIDCRC_Handle_t CRCHandle, STVID_CRCStartParams_t *StartParams)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDCRC_Data_t * VIDCRC_Data_p;

#if !defined(ST_7100) && !defined(ST_7109) && !defined(ST_7200) && !defined(ST_ZEUS) && !defined(ST_DELTAMU_HE)
    return(ST_ERROR_BAD_PARAMETER);
#else
    /* Find structure */
    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    if(VIDCRC_Data_p->State != VIDCRC_STATE_STOPPED)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
/* Store parameters in private data */
    VIDCRC_Data_p->NbPictureInRefCRC        = StartParams->NbPictureInRefCRC;
    VIDCRC_Data_p->RefCRCTable              = StartParams->RefCRCTable;
    VIDCRC_Data_p->MaxNbPictureInCompCRC    = StartParams->MaxNbPictureInCompCRC;
    VIDCRC_Data_p->CompCRCTable             = StartParams->CompCRCTable;
    VIDCRC_Data_p->MaxNbPictureInErrorLog   = StartParams->MaxNbPictureInErrorLog;
    VIDCRC_Data_p->ErrorLogTable            = StartParams->ErrorLogTable;
    VIDCRC_Data_p->NbPicturesToCheck        = StartParams->NbPicturesToCheck;
    VIDCRC_Data_p->DumpFailingFrames        = StartParams->DumpFailingFrames;

#ifdef FIRST_FIELD_ISSUE_WA
    /* The first CRC values received through vidcrc_CheckCRC will not be checked */
    VIDCRC_Data_p->IsFirstPicture           = TRUE;
    /* Add 1 picture to the number of pictures to check in order to be sure that first picture will
       be checked at second loop of stream */
    VIDCRC_Data_p->NbPicturesToCheck++;
#endif /* FIRST_FIELD_ISSUE_WA */

    STTBX_Print(("Stream contains %d pictures\n", VIDCRC_Data_p->NbPictureInRefCRC));

    VIDCRC_Data_p->NextRefCRCIndex = 0;
    VIDCRC_Data_p->NextCompCRCIndex = 0;
    VIDCRC_Data_p->NextErrorLogIndex = 0;
    VIDCRC_Data_p->NbErrors = 0;

    VIDCRC_Data_p->State = VIDCRC_STATE_CRC_CHECKING;
    return  ErrorCode;
#endif /*  not (!defined(ST_7100) && !defined(ST_7109) && !defined(ST_DELTAMU_HE)) */
} /* End of VIDCRC_CRCStartCheck() function */

/*----------------------------------------------------
   VIDCRC_IsCRCRunning
Returns     : TRUE if CRC is running, FALSE if not
 * --------------------------------------------------*/
BOOL VIDCRC_IsCRCRunning(const VIDCRC_Handle_t CRCHandle)
{
    VIDCRC_Data_t *VIDCRC_Data_p;

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    return((VIDCRC_Data_p->State == VIDCRC_STATE_CRC_CHECKING) || (VIDCRC_Data_p->State == VIDCRC_STATE_CRC_QUEUEING));
} /* End of VIDCRC_IsCRCCheckRunning() function */

/*----------------------------------------------------
   VIDCRC_IsCRCQueueingRunning
Returns     : TRUE if CRC queueing is running, FALSE if not
 * --------------------------------------------------*/
BOOL VIDCRC_IsCRCQueueingRunning(const VIDCRC_Handle_t CRCHandle)
{
    VIDCRC_Data_t *VIDCRC_Data_p;

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    return(VIDCRC_Data_p->State == VIDCRC_STATE_CRC_QUEUEING);
} /* End of VIDCRC_IsCRCCheckRunning() function */

/*----------------------------------------------------
   VIDCRC_IsCRCCheckRunning
Returns     : TRUE if CRC check is running, FALSE if not
 * --------------------------------------------------*/
BOOL VIDCRC_IsCRCCheckRunning(const VIDCRC_Handle_t CRCHandle)
{
    VIDCRC_Data_t *VIDCRC_Data_p;

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    return(VIDCRC_Data_p->State == VIDCRC_STATE_CRC_CHECKING);
} /* End of VIDCRC_IsCRCCheckRunning() function */

/*----------------------------------------------------
   VIDCRC_IsCRCModeField
Returns     : TRUE if CRC check should be done in field mode for this picture
              FALSE if CRC check should be done in frame mode
 * --------------------------------------------------*/
BOOL VIDCRC_IsCRCModeField(const VIDCRC_Handle_t CRCHandle,
                           STVID_PictureInfos_t *PictureInfos_p,
                           STVID_MPEGStandard_t MPEGStandard)
{
    BOOL          IsFieldCRC;

/*
if(VC1)
  frame_CRC
else if (VSize > 720 lines)
  field_CRC
else if (field_pic_flag == 1)
  field_CRC
else if (mb_adaptive_frame_field_flag == 1) && H264
  field_CRC
else
  frame_CRC
*/
#ifdef STVID_VALID_FIELDCRC
        IsFieldCRC = TRUE;
#else
    if(PictureInfos_p->BitmapParams.Height > 720)
    {
        IsFieldCRC = TRUE;
    }
    else
    {
        if(PictureInfos_p->VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
            IsFieldCRC = TRUE;
        else
        {
            if((PictureInfos_p->VideoParams.ForceFieldCRC) &&
               (MPEGStandard == STVID_MPEG_STANDARD_ISO_IEC_14496))
                IsFieldCRC = TRUE;
            else
                IsFieldCRC = FALSE;
        }
    }
#endif /* #ifdef STVID_VALID_FIELDCRC */

    return(IsFieldCRC);
} /* End of VIDCRC_IsCRCModeField() function */

/*----------------------------------------------------
   VIDCRC_CheckCRC()
        Check and register CRC for picture passed as parameter.
Returns     : ST_NO_ERROR on success, error code on fail
 * --------------------------------------------------*/
ST_ErrorCode_t VIDCRC_CheckCRC(const VIDCRC_Handle_t CRCHandle,
                                STVID_PictureInfos_t *PictureInfos_p,
                                VIDCOM_PictureID_t ExtendedPresentationOrderPictureID,
                                U32 DecodingOrderFrameID, U32 PictureNumber,
                                BOOL IsRepeatedLastPicture, BOOL IsRepeatedLastButOnePicture,
                                void *LumaAddress, void *ChromaAddress,
                                U32 LumaCRC, U32 ChromaCRC)
{
    VIDCRC_Data_t          *VIDCRC_Data_p;
#ifdef STVID_SOFTWARE_CRC_MODE
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
    U32                     YUVLumaSize;
    U32                     YUVChromaSize;
    U32                     YUVBufferSize;
    BOOL                   IsFieldCRC;
    static U32             LastPictureID = -1;
    static U32             RepeatCount = 0;
#endif /* STVID_SOFTWARE_CRC_MODE */
    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    if(VIDCRC_Data_p->State != VIDCRC_STATE_CRC_CHECKING)
    {
        return(ST_NO_ERROR);
    }

#ifdef STVID_SOFTWARE_CRC_MODE
/* For software mode repeat picture information is not relevant since a computation of CRC
   can be very long and we can skip VSYNC calls.
   So we only check CRC once by picture */
    if(ExtendedPresentationOrderPictureID.ID == LastPictureID)
    {
        RepeatCount++;
    }
    else
    {
        LastPictureID = ExtendedPresentationOrderPictureID.ID;
        RepeatCount = 0;
    }

    IsFieldCRC = VIDCRC_IsCRCModeField(CRCHandle, PictureInfos_p, VIDCRC_Data_p->VideoStandard);

     /* Allow no repeat for field picture */
    if(IsFieldCRC)
    {
        if(RepeatCount > 0)
        {
            return(ST_NO_ERROR);
        }
    }
    else
     /* Allow only one repeat for frame picture */
    {
        if(RepeatCount > 1)
        {
            return(ST_NO_ERROR);
        }
    }

    YUVLumaSize = PictureInfos_p->BitmapParams.Width * PictureInfos_p->BitmapParams.Height;
    YUVLumaSize = ((YUVLumaSize + 3)/4)*4;
    YUVChromaSize = YUVLumaSize / 4;
    YUVBufferSize = YUVLumaSize + (YUVChromaSize*2);
    if(YUVBufferSize > VIDCRC_Data_p->YUVBuffer.BufferSize)
    {
        if(VIDCRC_Data_p->YUVBuffer.BufferSize != 0)
        {
            SAFE_MEMORY_DEALLOCATE(VIDCRC_Data_p->YUVBuffer.Buffer, VIDCRC_Data_p->CPUPartition_p, VIDCRC_Data_p->YUVBuffer.BufferSize);
        }
        SAFE_MEMORY_ALLOCATE(VIDCRC_Data_p->YUVBuffer.Buffer, U8 *, VIDCRC_Data_p->CPUPartition_p, YUVBufferSize);
        VIDCRC_Data_p->YUVBuffer.CbAddress = VIDCRC_Data_p->YUVBuffer.Buffer + YUVLumaSize;
        VIDCRC_Data_p->YUVBuffer.CrAddress = VIDCRC_Data_p->YUVBuffer.CbAddress + YUVChromaSize;
        VIDCRC_Data_p->YUVBuffer.BufferSize = YUVBufferSize;
    }

    Gam2yuv(PictureInfos_p->BitmapParams.Width, PictureInfos_p->BitmapParams.Height,
            (U32 *)STAVMEM_DeviceToCPU(LumaAddress, &VIDCRC_Data_p->VirtualMapping),(U32 *)STAVMEM_DeviceToCPU(ChromaAddress, &VIDCRC_Data_p->VirtualMapping),
            VIDCRC_Data_p->YUVBuffer.Buffer, VIDCRC_Data_p->YUVBuffer.CbAddress, VIDCRC_Data_p->YUVBuffer.CrAddress);

    if(!IsFieldCRC)
    {
        /* Compute and check top field for frame picture */
        vidcrc_CalcCRC(VIDCRC_Data_p,
                                   PictureInfos_p->BitmapParams.Width, PictureInfos_p->BitmapParams.Height, STVID_PICTURE_STRUCTURE_TOP_FIELD,
                                   VIDCRC_Data_p->YUVBuffer.Buffer, VIDCRC_Data_p->YUVBuffer.CbAddress, VIDCRC_Data_p->YUVBuffer.CrAddress,
                                   &LumaCRC, &ChromaCRC);
        ErrorCode = vidcrc_CheckCRC(CRCHandle, PictureInfos_p,
                                                        ExtendedPresentationOrderPictureID, DecodingOrderFrameID, PictureNumber,
                                                        IsRepeatedLastPicture, IsRepeatedLastButOnePicture,
                                                        LumaAddress, ChromaAddress,
                                                        LumaCRC, ChromaCRC);
#if 0
        if(ErrorCode == ST_NO_ERROR)
        {
            /* Compute and check bottom field */
            vidcrc_CalcCRC(VIDCRC_Data_p,
                                       PictureInfos_p->BitmapParams.Width, PictureInfos_p->BitmapParams.Height, STVID_PICTURE_STRUCTURE_BOTTOM_FIELD,
                                       VIDCRC_Data_p->YUVBuffer.Buffer, VIDCRC_Data_p->YUVBuffer.CbAddress, VIDCRC_Data_p->YUVBuffer.CrAddress,
                                       &LumaCRC, &ChromaCRC);
            ErrorCode = vidcrc_CheckCRC(CRCHandle, PictureInfos_p,
                                        ExtendedPresentationOrderPictureID, DecodingOrderFrameID, PictureNumber,
                                        IsRepeatedLastPicture, IsRepeatedLastButOnePicture,
                                        LumaAddress, ChromaAddress,
                                        LumaCRC, ChromaCRC);
        }
#endif
    }
    else
    {
        vidcrc_CalcCRC(VIDCRC_Data_p,
                                   PictureInfos_p->BitmapParams.Width, PictureInfos_p->BitmapParams.Height, PictureInfos_p->VideoParams.PictureStructure,
                                   VIDCRC_Data_p->YUVBuffer.Buffer, VIDCRC_Data_p->YUVBuffer.CbAddress, VIDCRC_Data_p->YUVBuffer.CrAddress,
                                   &LumaCRC, &ChromaCRC);
        ErrorCode = vidcrc_CheckCRC(CRCHandle, PictureInfos_p,
                                                        ExtendedPresentationOrderPictureID, DecodingOrderFrameID, PictureNumber,
                                                        IsRepeatedLastPicture, IsRepeatedLastButOnePicture,
                                                        LumaAddress, ChromaAddress,
                                                        LumaCRC, ChromaCRC);
    }
    return(ErrorCode);
#else /* STVID_SOFTWARE_CRC_MODE */
    return(vidcrc_CheckCRC(CRCHandle, PictureInfos_p,
                                             ExtendedPresentationOrderPictureID, DecodingOrderFrameID, PictureNumber,
                                             IsRepeatedLastPicture, IsRepeatedLastButOnePicture,
                                             LumaAddress, ChromaAddress,
                                             LumaCRC, ChromaCRC));
#endif /* not STVID_SOFTWARE_CRC_MODE */
} /* End of VIDCRC_CheckCRC() function */

/*----------------------------------------------------
   VIDCRC_CRCStopCheck()
        Wakes up the CRC Report task to finish CRC check.
Returns     : ST_NO_ERROR on success, error code on fail
 * --------------------------------------------------*/
ST_ErrorCode_t VIDCRC_CRCStopCheck(const VIDCRC_Handle_t CRCHandle)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VIDCRC_Data_t          *VIDCRC_Data_p;
    STVID_CRCCheckResult_t  CRCCheckResult;

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    if((VIDCRC_Data_p->State != VIDCRC_STATE_CRC_CHECKING) &&
       (VIDCRC_Data_p->State != VIDCRC_STATE_PERF_CHECKING))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Can't stop a CRC/Perf check which is not started !!"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    VIDCRC_Data_p->State = VIDCRC_STATE_STOPPED;

    CRCCheckResult.NbRefCRCChecked      = VIDCRC_Data_p->NextRefCRCIndex;
    CRCCheckResult.NbCompCRC            = VIDCRC_Data_p->NextCompCRCIndex;
    CRCCheckResult.NbErrorsAndWarnings  = VIDCRC_Data_p->NextErrorLogIndex;
    CRCCheckResult.NbErrors             = VIDCRC_Data_p->NbErrors;

    /* Notify application that CRC check is finished and send results */
    STEVT_Notify(VIDCRC_Data_p->EventsHandle,
        VIDCRC_Data_p->RegisteredEventsID[VIDCRC_END_OF_CRC_CHECK_EVT_ID],
        STEVT_EVENT_DATA_TYPE_CAST &CRCCheckResult);

    return(ErrorCode);
} /* End of VIDCRC_CRCStopCheck() function */

/*----------------------------------------------------
   VIDCRC_CRCGetCurrentResults()
        Returns Current CRC Check results.
Returns     : ST_NO_ERROR on success, error code on fail
 * --------------------------------------------------*/
ST_ErrorCode_t VIDCRC_CRCGetCurrentResults(const VIDCRC_Handle_t CRCHandle, STVID_CRCCheckResult_t *CRCCheckResult_p)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VIDCRC_Data_t          *VIDCRC_Data_p;

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    CRCCheckResult_p->NbRefCRCChecked       = VIDCRC_Data_p->NextRefCRCIndex;
    CRCCheckResult_p->NbCompCRC             = VIDCRC_Data_p->NextCompCRCIndex;
    CRCCheckResult_p->NbErrorsAndWarnings   = VIDCRC_Data_p->NextErrorLogIndex;
    CRCCheckResult_p->NbErrors              = VIDCRC_Data_p->NbErrors;

    return(ErrorCode);
} /* End of VIDCRC_CRCGetCurrentResults() function */

/*----------------------------------------------------
   VIDCRC_CRCStartQueueing()
        Start the CRC queueing i.e crc being stored in a queue.
Returns     : ST_NO_ERROR on success, error code on fail
 * --------------------------------------------------*/
ST_ErrorCode_t VIDCRC_CRCStartQueueing(const VIDCRC_Handle_t CRCHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDCRC_Data_t * VIDCRC_Data_p;

#if !defined(ST_7100) && !defined(ST_7109) && !defined(ST_7200) && !defined(ST_ZEUS) && !defined(ST_DELTAMU_HE)
    return(ST_ERROR_BAD_PARAMETER);
#else
    /* Find structure */
    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    if(VIDCRC_Data_p->State != VIDCRC_STATE_STOPPED)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* Reset the queue */
    VIDCRC_Data_p->CRCMessageQueue.ReadIndex = 0;
    VIDCRC_Data_p->CRCMessageQueue.WriteIndex = 0;
    VIDCRC_Data_p->CRCMessageQueue.DataCount = 0;

    VIDCRC_Data_p->State = VIDCRC_STATE_CRC_QUEUEING;
    return  ErrorCode;
#endif /*  not (!defined(ST_7100) && !defined(ST_7109) && !defined(ST_DELTAMU_HE)) */
}

/*----------------------------------------------------
   VIDCRC_CRCStopQueueing()
        Stop the CRC queueing.
Returns     : ST_NO_ERROR on success, error code on fail
 * --------------------------------------------------*/
ST_ErrorCode_t VIDCRC_CRCStopQueueing(const VIDCRC_Handle_t CRCHandle)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VIDCRC_Data_t          *VIDCRC_Data_p;

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    if(VIDCRC_Data_p->State != VIDCRC_STATE_CRC_QUEUEING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Can't stop a CRC queueing which is not started !!"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    VIDCRC_Data_p->State = VIDCRC_STATE_STOPPED;

    /* Flush the queue */
    VIDCRC_Data_p->CRCMessageQueue.ReadIndex = 0;
    VIDCRC_Data_p->CRCMessageQueue.WriteIndex = 0;
    VIDCRC_Data_p->CRCMessageQueue.DataCount = 0;

    return(ErrorCode);
}

/*----------------------------------------------------
   VIDCRC_CRCEnqueue()
        Enqueue the current CRC. If queue is full value is discarded and no event is generated
Returns     : ST_NO_ERROR on success, error code on fail
 * --------------------------------------------------*/
ST_ErrorCode_t VIDCRC_CRCEnqueue(const VIDCRC_Handle_t CRCHandle, STVID_CRCDataMessage_t DataToEnqueue)
{

    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VIDCRC_Data_t          *VIDCRC_Data_p;

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    /* Data is discarded if queue is full */
    if(VIDCRC_Data_p->CRCMessageQueue.DataCount < VIDEO_DECODER_CRC_MAX_Q_SIZE)
    {
        /* Add data int the queue */
        VIDCRC_Data_p->CRCMessageQueue.CRCData[VIDCRC_Data_p->CRCMessageQueue.WriteIndex] = DataToEnqueue;
        VIDCRC_Data_p->CRCMessageQueue.DataCount++;

        /* manage queue write indexe */
        VIDCRC_Data_p->CRCMessageQueue.WriteIndex++;
        if(VIDCRC_Data_p->CRCMessageQueue.WriteIndex >= VIDEO_DECODER_CRC_MAX_Q_SIZE)
        {
            VIDCRC_Data_p->CRCMessageQueue.WriteIndex = 0;
        }

        /* Notify application that a new CRC was enqueued */
        ErrorCode = STEVT_Notify(VIDCRC_Data_p->EventsHandle,
                                                  VIDCRC_Data_p->RegisteredEventsID[VIDCRC_NEW_CRC_QUEUED_EVT_ID],
                                                  NULL);
    }

    return(ErrorCode);
}

/*----------------------------------------------------
   VIDCRC_CRCGetQueue()
        Get the current CRC queue data.
        Data is removed from the queue when read
Returns     : ST_NO_ERROR on success, error code on fail
 * --------------------------------------------------*/
ST_ErrorCode_t VIDCRC_CRCGetQueue(const VIDCRC_Handle_t CRCHandle, STVID_CRCReadMessages_t *ReadCRC_p)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VIDCRC_Data_t          *VIDCRC_Data_p;
    int DestIndex;

    VIDCRC_Data_p = (VIDCRC_Data_t *) CRCHandle;

    if(ReadCRC_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    for( DestIndex=0 ; DestIndex< ReadCRC_p->NbToRead; DestIndex++)
    {
        if( DestIndex >= VIDCRC_Data_p->CRCMessageQueue.DataCount )
        {   /* No more data available => exit loop */
            break;
        }
        else
        {
            /* dequeue data */
            ReadCRC_p->Messages_p[DestIndex]= VIDCRC_Data_p->CRCMessageQueue.CRCData[VIDCRC_Data_p->CRCMessageQueue.ReadIndex];

            /* manage queue read indexe */
            VIDCRC_Data_p->CRCMessageQueue.ReadIndex++;
            if(VIDCRC_Data_p->CRCMessageQueue.ReadIndex >= VIDEO_DECODER_CRC_MAX_Q_SIZE)    /* wrap around queue if needed */
            {
               VIDCRC_Data_p->CRCMessageQueue.ReadIndex= 0;
            }
        }
    }
    VIDCRC_Data_p->CRCMessageQueue.DataCount-=DestIndex;
    ReadCRC_p->NbReturned = DestIndex;

    return(ErrorCode);
}

#ifdef VALID_TOOLS
/*******************************************************************************
Name        : VIDCRC_RegisterTrace
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDCRC_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef TRACE_UART
    TRACE_ConditionalRegister(VIDCRC_TRACE_BLOCKID, VIDCRC_TRACE_BLOCKNAME, vidcrc_TraceItemList, sizeof(vidcrc_TraceItemList)/sizeof(TRACE_Item_t));
#endif /* TRACE_UART */

    return(ErrorCode);
}
#endif /* VALID_TOOLS */

/* End of vid_crc.c file */
