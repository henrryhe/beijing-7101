/*!
 *******************************************************************************
 * \file ref_pic_list.c
 *
 * \brief H264 reference picture list initialisation process
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#include "stos.h"

/* H264 Parser specific */
#include "h264parser.h"

/* Functions ---------------------------------------------------------------- */

#ifdef ST_OSLINUX
static void qsort(void * base, size_t nmemb, size_t size, int(*compar)(const void *, const void*))
{
    char small_buf[SMALL_BUFF_BYTES];
    U32     i, j;
    void  * tmp;
    U32     elem1, elem2;
    BOOL    OnceAgain = TRUE;

    if (size > SMALL_BUFF_BYTES)
    {
        tmp = STOS_MemoryAllocate(NULL, size);
        STTBX_Report((STTBX_REPORT_LEVEL_WARNING, "[%s]%s:%d: increase SMALL_BUFF_BYTES to %d\n", __FUNCTION__, __FILE__, __LINE__, size));
    }
    else
    {
        tmp = (void *)small_buf;
    }

    for (j=0 ; j<nmemb && OnceAgain ; j++)
    {
        OnceAgain = FALSE;
        elem1 = (U32)base;
        for (i=0 ; i<nmemb-1 ; i++)
        {
            elem2 = elem1 + size;
            if (compar((void *)elem1, (void *)elem2) > 0)
            {
                /* Invert elements */
                memcpy(tmp, (void *)elem1, size);
                memcpy((void *)elem1, (void *)elem2, size);
                memcpy((void *)elem2, tmp, size);

                /* At least one swap, sort algorithm needed once again ... */
                OnceAgain = TRUE;
            }
            elem1 += size;
        }
    }
    if (size > SMALL_BUFF_BYTES)
    {
        STOS_MemoryDeallocate(NULL, tmp);
    }
}
#endif

/******************************************************************************/
/*! \brief Computes the full reference lists for the current picture.
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_InitFullList(const PARSER_Handle_t  ParserHandle)
{
    U8 i; /* to loop over the DPBRef */
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
    {
        if (DPBRefFrameIsUsed(i))
        {
            /* Non existing frames do not appear in the "IsValidIndex"
               because the producer shall not perform any computing on them
               Nevertheless, they are included in the "FullReferenceFrameList
               because the decoder is taking care of them
            */
            if (PARSER_Data_p->DPBFrameReference[i].IsNonExisting == FALSE)
            {
                PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i] = TRUE;
                PARSER_Data_p->PictureSpecificData.FullReferenceTopFieldList[i].NonExistingPicOrderCnt = PARSER_Data_p->DPBFrameReference[i].TopFieldOrderCnt;
                PARSER_Data_p->PictureSpecificData.FullReferenceBottomFieldList[i].NonExistingPicOrderCnt = PARSER_Data_p->DPBFrameReference[i].BottomFieldOrderCnt;
            }
            else
            {
                PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i] = FALSE;
                PARSER_Data_p->PictureSpecificData.FullReferenceTopFieldList[i].NonExistingPicOrderCnt = DONT_CARE;
                PARSER_Data_p->PictureSpecificData.FullReferenceBottomFieldList[i].NonExistingPicOrderCnt = DONT_CARE;
            }
            PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = PARSER_Data_p->DPBFrameReference[i].IsBrokenLink;
            PARSER_Data_p->PictureGenericData.FullReferenceFrameList[i] = PARSER_Data_p->DPBFrameReference[i].DecodingOrderFrameID;
            PARSER_Data_p->PictureSpecificData.FullReferenceTopFieldList[i].IsLongTerm = PARSER_Data_p->DPBFrameReference[i].TopFieldIsLongTerm;
            PARSER_Data_p->PictureSpecificData.FullReferenceBottomFieldList[i].IsLongTerm = PARSER_Data_p->DPBFrameReference[i].BottomFieldIsLongTerm;
            PARSER_Data_p->PictureSpecificData.FullReferenceTopFieldList[i].IsNonExisting = PARSER_Data_p->DPBFrameReference[i].IsNonExisting;
            PARSER_Data_p->PictureSpecificData.FullReferenceBottomFieldList[i].IsNonExisting = PARSER_Data_p->DPBFrameReference[i].IsNonExisting;
            if (PARSER_Data_p->DPBFrameReference[i].TopFieldIsLongTerm)
            {
                PARSER_Data_p->PictureSpecificData.FullReferenceTopFieldList[i].PictureNumber = PARSER_Data_p->DPBFrameReference[i].TopFieldLongTermPicNum;
            }
            else
            {
                PARSER_Data_p->PictureSpecificData.FullReferenceTopFieldList[i].PictureNumber = PARSER_Data_p->DPBFrameReference[i].TopFieldPicNum;
            }
            if (PARSER_Data_p->DPBFrameReference[i].BottomFieldIsLongTerm)
            {
                PARSER_Data_p->PictureSpecificData.FullReferenceBottomFieldList[i].PictureNumber = PARSER_Data_p->DPBFrameReference[i].BottomFieldLongTermPicNum;
            }
            else
            {
                PARSER_Data_p->PictureSpecificData.FullReferenceBottomFieldList[i].PictureNumber = PARSER_Data_p->DPBFrameReference[i].BottomFieldPicNum;
            }
        }
        else
        {
            PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i] = FALSE;
            PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = FALSE;
            PARSER_Data_p->PictureSpecificData.FullReferenceTopFieldList[i].NonExistingPicOrderCnt = DONT_CARE;
            PARSER_Data_p->PictureSpecificData.FullReferenceBottomFieldList[i].NonExistingPicOrderCnt = DONT_CARE;
            PARSER_Data_p->PictureSpecificData.FullReferenceTopFieldList[i].IsLongTerm = FALSE;
            PARSER_Data_p->PictureSpecificData.FullReferenceBottomFieldList[i].IsLongTerm = FALSE;
            PARSER_Data_p->PictureSpecificData.FullReferenceTopFieldList[i].IsNonExisting = FALSE;
            PARSER_Data_p->PictureSpecificData.FullReferenceBottomFieldList[i].IsNonExisting = FALSE;
        }
    }
}

/******************************************************************************/
/*! \brief Sort a list of DPBRef elements by descending TopFieldPicNum
 *
 * \param Elem1 an element of the list to sort
 * \param Elem2 an element of the list to sort
 * \return comparison result (1: Elem1 < Elem2), (0: Elem1 = Elem2), (-1: Elem1 > Elem2)
 */
/******************************************************************************/
S32 h264par_SortDescendingPicNumFunction(const void *Elem1, const void *Elem2)
{
    if ( (*(const DPBFrameReference_t *)Elem1).TopFieldPicNum > (*(const DPBFrameReference_t *)Elem2).TopFieldPicNum)
    {
        return -1;
    }
    else if ( (*(const DPBFrameReference_t *)Elem1).TopFieldPicNum < (*(const DPBFrameReference_t *)Elem2).TopFieldPicNum)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/******************************************************************************/
/*! \brief Sort a list of DPBRef elements by ascending TopFieldLongTermPicNum
 *
 * \param Elem1 an element of the list to sort
 * \param Elem2 an element of the list to sort
 * \return comparison result (1: Elem1 > Elem2), (0: Elem1 = Elem2), (-1: Elem1 < Elem2)
 */
/******************************************************************************/
S32 h264par_SortAscendingTopFieldLongTermPicNumFunction(const void *Elem1, const void *Elem2)
{
    if ( (*(const DPBFrameReference_t *)Elem1).TopFieldLongTermPicNum < (*(const DPBFrameReference_t *)Elem2).TopFieldLongTermPicNum)
    {
        return -1;
    }
    else if ( (*(const DPBFrameReference_t *)Elem1).TopFieldLongTermPicNum > (*(const DPBFrameReference_t *)Elem2).TopFieldLongTermPicNum)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/******************************************************************************/
/*! \brief Computes the reference picture list for P slices in frame
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_InitRefPicListForPSlicesInFrame(const PARSER_Handle_t ParserHandle)
{
    U8 i; /* to loop over the DPBRef */
    U8 NumberOfShortTermFrames;
    U8 NumberOfLongTermFrames;
    U8 IndexInList;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* The processing is described in the standard 8.2.4.2.1 */

    /* Build two lists: the short-term and the long-term reference frames */
    /* Non-paired fields are not considered */
    NumberOfShortTermFrames = 0;
    NumberOfLongTermFrames = 0;
    for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
    {
        if (DPBRefFrameIsShortTermFrame(i))
        {
            PARSER_Data_p->RefFrameList0ShortTerm[NumberOfShortTermFrames] = PARSER_Data_p->DPBFrameReference[i];
            NumberOfShortTermFrames++;
        }
        else if (DPBRefFrameIsLongTermFrame(i))
        {
            PARSER_Data_p->RefFrameList0LongTerm[NumberOfLongTermFrames] = PARSER_Data_p->DPBFrameReference[i];
            NumberOfLongTermFrames++;
        }
    }

    /* Sort the short-term list with descending PicNum */
    qsort((void *)PARSER_Data_p->RefFrameList0ShortTerm, NumberOfShortTermFrames, sizeof(PARSER_Data_p->RefFrameList0ShortTerm[0]), h264par_SortDescendingPicNumFunction);

    /* Sort the long-term list with ascending PicNum */
    qsort((void *)PARSER_Data_p->RefFrameList0LongTerm, NumberOfLongTermFrames, sizeof(PARSER_Data_p->RefFrameList0LongTerm[0]), h264par_SortAscendingTopFieldLongTermPicNumFunction);

    /* Build ReferenceListP0 and IsReferenceTopFieldP0 */
    PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = NumberOfShortTermFrames + NumberOfLongTermFrames;

    /* Short-term frames are listed first */
    for (i = 0; i < NumberOfShortTermFrames; i ++)
    {
        PARSER_Data_p->PictureGenericData.ReferenceListP0[i] = PARSER_Data_p->RefFrameList0ShortTerm[i].DecodingOrderFrameID;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[i] = FALSE; /* FALSE for frames */
    }

    /* Long-term frames are listed after short-term frames */
    for (i = 0; i < NumberOfLongTermFrames; i ++)
    {
        IndexInList = NumberOfShortTermFrames + i;
        PARSER_Data_p->PictureGenericData.ReferenceListP0[IndexInList] = PARSER_Data_p->RefFrameList0LongTerm[i].DecodingOrderFrameID;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[IndexInList] = FALSE; /* FALSE for frames */
    }
}

/******************************************************************************/
/*! \brief Computes PicOrderCnt (see equation 8-1 in standard)
 *
 * \param Elem1 an element of the list to sort
 * \param Elem2 an element of the list to sort
 * \return minimum the minimum element among Elem1 and Elem2
 */
/******************************************************************************/
S32 h264par_PicOrderCnt(S32 Elem1, S32 Elem2)
{
    if (Elem1 < Elem2)
    {
        return (Elem1);
    }
    else
    {
        return (Elem2);
    }
}

/******************************************************************************/
/*! \brief Computes PicOrderCnt (see equation 8-1 in standard)
 *
 * \param Elem1 an element of the list to sort
 * \param Elem2 an element of the list to sort
 * \return minimum the minimum element among Elem1 and Elem2
 */
/******************************************************************************/
S32 h264par_PicOrderCntDPB(const DPBFrameReference_t * DPBFrameReference)
{
    if (DPBFrameReference->TopFieldIsReference == FALSE)
    {
        return (DPBFrameReference->BottomFieldOrderCnt);
    }
    if (DPBFrameReference->BottomFieldIsReference == FALSE)
    {
        return (DPBFrameReference->TopFieldOrderCnt);
    }
    /* Both fields are reference, thus compare their Poc */
    if (DPBFrameReference->TopFieldOrderCnt < DPBFrameReference->BottomFieldOrderCnt)
    {
        return (DPBFrameReference->TopFieldOrderCnt);
    }
    else
    {
        return (DPBFrameReference->BottomFieldOrderCnt);
    }
}

/******************************************************************************/
/*! \brief Sort a list of DPBRef elements by descending PicOrderCnt
 *
 * \param Elem1 an element of the list to sort
 * \param Elem2 an element of the list to sort
 * \return comparison result (1: Elem1 < Elem2), (0: Elem1 = Elem2), (-1: Elem1 > Elem2)
 */
/******************************************************************************/
S32 h264par_SortDescendingPicOrderCntFunction(const void *Elem1, const void *Elem2)
{
    S32 Elem1PicOrderCnt;
    S32 Elem2PicOrderCnt;

    Elem1PicOrderCnt = h264par_PicOrderCntDPB((const DPBFrameReference_t *)Elem1);

    Elem2PicOrderCnt = h264par_PicOrderCntDPB((const DPBFrameReference_t *)Elem2);

    if (Elem1PicOrderCnt > Elem2PicOrderCnt)
    {
        return -1;
    }
    else if (Elem1PicOrderCnt < Elem2PicOrderCnt)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/******************************************************************************/
/*! \brief Sort a list of DPBRef elements by ascending PicOrderCnt
 *
 * \param Elem1 an element of the list to sort
 * \param Elem2 an element of the list to sort
 * \return comparison result (1: Elem1 > Elem2), (0: Elem1 = Elem2), (-1: Elem1 < Elem2)
 */
/******************************************************************************/
S32 h264par_SortAscendingPicOrderCntFunction(const void *Elem1, const void *Elem2)
{
    S32 Elem1PicOrderCnt;
    S32 Elem2PicOrderCnt;

    Elem1PicOrderCnt = h264par_PicOrderCntDPB((const DPBFrameReference_t *)Elem1);

    Elem2PicOrderCnt = h264par_PicOrderCntDPB((const DPBFrameReference_t *)Elem2);

    if (Elem1PicOrderCnt < Elem2PicOrderCnt)
    {
        return -1;
    }
    else if (Elem1PicOrderCnt > Elem2PicOrderCnt)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/******************************************************************************/
/*! \brief check if reference lists B0 and B1 are identical.
 *
 * If yes, swap the first 2 elements in List B1
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_CheckIfB0AndB1ListAreEqual(const PARSER_Handle_t ParserHandle)
{
    BOOL ListB0EqualsListB1;
    U32  SwapDecodingOrderFrameID;
    BOOL SwapIsReferenceTopField;
    U8   i; /* to loop over the references */
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* When the reference picture list RefPicList1 has more than one entry and RefPicList1 is identical to the reference
     * picture list RefPicList0, the first two entries RefPicList1[0] and RefPicList1[1] are switched
     */
     if (PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 > 1)
     {
        ListB0EqualsListB1 = TRUE;
        for (i = 0; i < PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0; i ++)
        {
            if (PARSER_Data_p->PictureGenericData.ReferenceListB1[i] != PARSER_Data_p->PictureGenericData.ReferenceListB0[i])
            {
                ListB0EqualsListB1 = FALSE;
                i = PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0; /* exit faster */
            }
        }
        if (ListB0EqualsListB1)
        {
            /* Swap the ReferenceListB1 indexes 0 and 1 */
            SwapDecodingOrderFrameID = PARSER_Data_p->PictureGenericData.ReferenceListB1[0];
            PARSER_Data_p->PictureGenericData.ReferenceListB1[0] = PARSER_Data_p->PictureGenericData.ReferenceListB1[1];
            PARSER_Data_p->PictureGenericData.ReferenceListB1[1] = SwapDecodingOrderFrameID;
            /* And the IsReferenceTopFieldB1 indexes 0 and 1 */
            SwapIsReferenceTopField = PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[0];
            PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[0] = PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[1];
            PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[1] = SwapIsReferenceTopField;
        }
    }
}

/******************************************************************************/
/*! \brief Computes the reference picture list for B slices in frame
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_InitRefPicListForBSlicesInFrame(const PARSER_Handle_t  ParserHandle)
{
    U8 i; /* to loop over the DPBRef */
    U8 NumberOfShortTermFramesBelowPOC;
    U8 NumberOfShortTermFramesAbovePOC;
    U8 NumberOfLongTermFrames;
    U8 IndexInList;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* The processing is described in the standard 8.2.4.2.3 */

    /* Build two lists: the short-term and the long-term reference frames */
    /* Non-paired fields are not considered */
    NumberOfShortTermFramesBelowPOC = 0;
    NumberOfShortTermFramesAbovePOC = 0;
    NumberOfLongTermFrames = 0;
    for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
    {
        if (DPBRefFrameIsShortTermFrame(i))
        {
            if ((PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->pic_order_cnt_type != 0) || (PARSER_Data_p->DPBFrameReference[i].IsNonExisting == FALSE))
            {
                /* Non-existing frames have no POC when pic_order_cnt_type = 0, thus they cannot be sorted in this process. */
                /* See corrigendum K012 */
                if (h264par_PicOrderCntDPB(&(PARSER_Data_p->DPBFrameReference[i])) <=
                    PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCnt
                   )
                {
                    PARSER_Data_p->ShortTermFramesBelowPOC[NumberOfShortTermFramesBelowPOC] = PARSER_Data_p->DPBFrameReference[i];
                    NumberOfShortTermFramesBelowPOC++;
                }
                else
                {
                    PARSER_Data_p->ShortTermFramesAbovePOC[NumberOfShortTermFramesAbovePOC] = PARSER_Data_p->DPBFrameReference[i];
                    NumberOfShortTermFramesAbovePOC++;
                }
            }
        }
        else if (DPBRefFrameIsLongTermFrame(i))
        {
            PARSER_Data_p->RefFrameList0LongTerm[NumberOfLongTermFrames] = PARSER_Data_p->DPBFrameReference[i];
            NumberOfLongTermFrames++;
        }
    }

    /* Sort the short-term list "below POC" with descending PicOrderCnt */
    qsort((void *)PARSER_Data_p->ShortTermFramesBelowPOC, NumberOfShortTermFramesBelowPOC, sizeof(PARSER_Data_p->ShortTermFramesBelowPOC[0]), h264par_SortDescendingPicOrderCntFunction);

    /* Sort the short-term list "above POC" with ascending PicOrderCnt */
    qsort((void *)PARSER_Data_p->ShortTermFramesAbovePOC, NumberOfShortTermFramesAbovePOC, sizeof(PARSER_Data_p->ShortTermFramesAbovePOC[0]), h264par_SortAscendingPicOrderCntFunction);

    /* Sort the long-term list with ascending PicNum */
    qsort((void *)PARSER_Data_p->RefFrameList0LongTerm, NumberOfLongTermFrames, sizeof(PARSER_Data_p->RefFrameList0LongTerm[0]), h264par_SortAscendingTopFieldLongTermPicNumFunction);

    /* Build ReferenceListB0 and IsReferenceTopFieldB0 */
    PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = NumberOfShortTermFramesBelowPOC +
                                                       NumberOfShortTermFramesAbovePOC +
                                                       NumberOfLongTermFrames;

    /* Short-term frames "below POC" are listed first */
    for (i = 0; i < NumberOfShortTermFramesBelowPOC; i ++)
    {
        PARSER_Data_p->PictureGenericData.ReferenceListB0[i] = PARSER_Data_p->ShortTermFramesBelowPOC[i].DecodingOrderFrameID;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[i] = FALSE; /* FALSE for frames */
    }

    /* Followed by Short-term frames "above POC" */
    for (i = 0; i < NumberOfShortTermFramesAbovePOC; i ++)
    {
        IndexInList = NumberOfShortTermFramesBelowPOC + i;
        PARSER_Data_p->PictureGenericData.ReferenceListB0[IndexInList] = PARSER_Data_p->ShortTermFramesAbovePOC[i].DecodingOrderFrameID;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[IndexInList] = FALSE; /* FALSE for frames */
    }

    /* Followed by Long-term frames */
    for (i = 0; i < NumberOfLongTermFrames; i ++)
    {
        IndexInList = NumberOfShortTermFramesBelowPOC + NumberOfShortTermFramesAbovePOC + i;
        PARSER_Data_p->PictureGenericData.ReferenceListB0[IndexInList] = PARSER_Data_p->RefFrameList0LongTerm[i].DecodingOrderFrameID;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[IndexInList] = FALSE; /* FALSE for frames */
    }

    /* Build ReferenceListB1 and IsReferenceTopFieldB1 */
    /* Same elements than in B0 but sorted another way */
    PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0;
    /* Short-term frames "above POC" are listed first */
    for (i = 0; i < NumberOfShortTermFramesAbovePOC; i ++)
    {
        PARSER_Data_p->PictureGenericData.ReferenceListB1[i] = PARSER_Data_p->ShortTermFramesAbovePOC[i].DecodingOrderFrameID;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[i] = FALSE; /* FALSE for frames */
    }

    /* Followed by Short-term frames "below POC" */
    for (i = 0; i < NumberOfShortTermFramesBelowPOC; i ++)
    {
        IndexInList = NumberOfShortTermFramesAbovePOC + i;
        PARSER_Data_p->PictureGenericData.ReferenceListB1[IndexInList] = PARSER_Data_p->ShortTermFramesBelowPOC[i].DecodingOrderFrameID;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[IndexInList] = FALSE; /* FALSE for frames */
    }

    /* Followed by Long-term frames */
    for (i = 0; i < NumberOfLongTermFrames; i ++)
    {
        IndexInList = NumberOfShortTermFramesBelowPOC + NumberOfShortTermFramesAbovePOC + i;
        PARSER_Data_p->PictureGenericData.ReferenceListB1[IndexInList] = PARSER_Data_p->RefFrameList0LongTerm[i].DecodingOrderFrameID;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[IndexInList] = FALSE; /* FALSE for frames */
    }

    h264par_CheckIfB0AndB1ListAreEqual(ParserHandle);
}

/******************************************************************************/
/*! \brief Sort a list of DPBRef elements by descending FrameNumWrap
 *
 * \param Elem1 an element of the list to sort
 * \param Elem2 an element of the list to sort
 * \return comparison result (1: Elem1 < Elem2), (0: Elem1 = Elem2), (-1: Elem1 > Elem2)
 */
/******************************************************************************/
S32 h264par_SortDescendingFrameNumWrapFunction(const void *Elem1, const void *Elem2)
{
    if ( (*(const DPBFrameReference_t *)Elem1).FrameNumWrap > (*(const DPBFrameReference_t *)Elem2).FrameNumWrap)
    {
        return -1;
    }
    else if ( (*(const DPBFrameReference_t *)Elem1).FrameNumWrap < (*(const DPBFrameReference_t *)Elem2).FrameNumWrap)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/******************************************************************************/
/*! \brief Sort a list of DPBRef elements by ascending LongTermFrameIdx
 *
 * \param Elem1 an element of the list to sort
 * \param Elem2 an element of the list to sort
 * \return comparison result (1: Elem1 > Elem2), (0: Elem1 = Elem2), (-1: Elem1 < Elem2)
 */
/******************************************************************************/
S32 h264par_SortAscendingLongTermFrameIdxFunction(const void *Elem1, const void *Elem2)
{
    if ( (*(const DPBFrameReference_t *)Elem1).LongTermFrameIdx < (*(const DPBFrameReference_t *)Elem2).LongTermFrameIdx)
    {
        return -1;
    }
    else if ( (*(const DPBFrameReference_t *)Elem1).LongTermFrameIdx > (*(const DPBFrameReference_t *)Elem2).LongTermFrameIdx)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/******************************************************************************/
/*! \brief returns whether the top field of the DPBRef element is reference
 *
 * \param RefFrameShortTerm DPBRef element
 * \return BOOL TRUE if top field of element is reference
 */
/******************************************************************************/
BOOL h264par_IsTopFieldShortRef(DPBFrameReference_t RefFrameShortTerm)
{
    return (RefFrameShortTerm.TopFieldIsReference);
}

/******************************************************************************/
/*! \brief returns whether the bottom field of the DPBRef element is reference
 *
 * \param RefFrameShortTerm DPBRef element
 * \return BOOL TRUE if bottom field of element is reference
 */
/******************************************************************************/
BOOL h264par_IsBottomFieldShortRef(DPBFrameReference_t RefFrameShortTerm)
{
    return (RefFrameShortTerm.BottomFieldIsReference);
}

/******************************************************************************/
/*! \brief returns whether the top field of the DPBRef element is long-term reference
 *
 * \param RefFrameLongTerm DPBRef element
 * \return BOOL TRUE if top field of element is reference
 */
/******************************************************************************/
BOOL h264par_IsTopFieldLongRef(DPBFrameReference_t RefFrameLongTerm)
{
    return (RefFrameLongTerm.TopFieldIsReference && RefFrameLongTerm.TopFieldIsLongTerm);
}

/******************************************************************************/
/*! \brief returns whether the bottom field of the DPBRef element is long-term reference
 *
 * \param RefFrameLongTerm DPBRef element
 * \return BOOL TRUE if bottom field of element is reference
 */
/******************************************************************************/
BOOL h264par_IsBottomFieldLongRef(DPBFrameReference_t RefFrameLongTerm)
{
    return (RefFrameLongTerm.BottomFieldIsReference && RefFrameLongTerm.BottomFieldIsLongTerm);
}

/******************************************************************************/
/*! \brief Alternate fields to build the reference list
 *
 * The processing involved here is found in standard: 8.2.4.2.5
 *
 * \param RefFrameListXShortTerm list of short-term frames sorted by FrameNumWrap
 * \param RefFrameListXLongTerm list of long-term frames sorted by LongTermPicNum
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_AlternateFieldsInList(U32 *                 UsedSizeInReferenceListX,
                                   U32 *                 ReferenceListX,
                                   BOOL *                IsReferenceTopField,
                                   U8                    NumberOfFrames,
                                   DPBFrameReference_t * RefFrameListX,
                                   BOOL (* h264par_IsCurrentParity) (DPBFrameReference_t RefFrame),
                                   BOOL (* h264par_IsAlternateParity) (DPBFrameReference_t RefFrame),
                                   const PARSER_Handle_t ParserHandle)
{
    U8 CurrentParityIndex;
    U8 AlternateParityIndex;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    CurrentParityIndex = 0;
    AlternateParityIndex = 0;
    while ((CurrentParityIndex < NumberOfFrames) || (AlternateParityIndex < NumberOfFrames))
    {
        /* Always start on the same parity as the current picture */
        for (; CurrentParityIndex < NumberOfFrames; CurrentParityIndex ++)
        {
            if (h264par_IsCurrentParity(RefFrameListX[CurrentParityIndex]))
            {
                ReferenceListX[* UsedSizeInReferenceListX] = RefFrameListX[CurrentParityIndex].DecodingOrderFrameID;
                IsReferenceTopField[* UsedSizeInReferenceListX] = CurrentPictureIsTopField;
                (* UsedSizeInReferenceListX)++;
                CurrentParityIndex++;
                break; /* to continue on the alternate parity */
            }
        }
        for (; AlternateParityIndex < NumberOfFrames; AlternateParityIndex ++)
        {
            if (h264par_IsAlternateParity(RefFrameListX[AlternateParityIndex]))
            {
                ReferenceListX[* UsedSizeInReferenceListX] = RefFrameListX[AlternateParityIndex].DecodingOrderFrameID;
                IsReferenceTopField[* UsedSizeInReferenceListX] = CurrentPictureIsBottomField;
                (* UsedSizeInReferenceListX)++;
                AlternateParityIndex++;
                break; /* to continue on the alternate parity */
            }
        }
    }
}

/******************************************************************************/
/*! \brief Build a reference list by inserting short term fields followed by
 *  long term fields
 *
 * The processing involved here is found in standard: 8.2.4.2.5
 *
 * \param RefFrameListXShortTerm list of short-term frames sorted by FrameNumWrap
 * \param RefFrameListXLongTerm list of long-term frames sorted by LongTermPicNum
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_BuildReferenceFieldsList(U32 *                 UsedSizeInReferenceListX,
                                      U32 *                 ReferenceListX,
                                      BOOL *                IsReferenceTopField,
                                      U8                    NumberOfShortTermFrames,
                                      U8                    NumberOfLongTermFrames,
                                      DPBFrameReference_t * RefFrameListXShortTerm,
                                      DPBFrameReference_t * RefFrameListXLongTerm,
                                      const PARSER_Handle_t ParserHandle)
{
    BOOL (* h264par_IsCurrentParityShortRef)    (DPBFrameReference_t RefFrameShortTerm);
    BOOL (* h264par_IsAlternateParityShortRef) (DPBFrameReference_t RefFrameShortTerm);
    BOOL (* h264par_IsCurrentParityLongRef)     (DPBFrameReference_t RefFrameLongTerm);
    BOOL (* h264par_IsAlternateParityLongRef)  (DPBFrameReference_t RefFrameLongTerm);
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    * UsedSizeInReferenceListX = 0;

    if (CurrentPictureIsTopField)
    {
        h264par_IsCurrentParityShortRef    = h264par_IsTopFieldShortRef;
        h264par_IsAlternateParityShortRef = h264par_IsBottomFieldShortRef;
        h264par_IsCurrentParityLongRef     = h264par_IsTopFieldLongRef;
        h264par_IsAlternateParityLongRef  = h264par_IsBottomFieldLongRef;
    }
    else
    {
        h264par_IsCurrentParityShortRef    = h264par_IsBottomFieldShortRef;
        h264par_IsAlternateParityShortRef = h264par_IsTopFieldShortRef;
        h264par_IsCurrentParityLongRef     = h264par_IsBottomFieldLongRef;
        h264par_IsAlternateParityLongRef  = h264par_IsTopFieldLongRef;
    }

    /* Build list with Short-term fields first, followed by long-term fields */
    /* Short-term fields */
    h264par_AlternateFieldsInList(UsedSizeInReferenceListX,
                                  ReferenceListX,
                                  IsReferenceTopField,
                                  NumberOfShortTermFrames,
                                  RefFrameListXShortTerm,
                                  h264par_IsCurrentParityShortRef,
                                  h264par_IsAlternateParityShortRef,
                                  ParserHandle);
    /* Followed by long term fields */
    h264par_AlternateFieldsInList(UsedSizeInReferenceListX,
                                  ReferenceListX,
                                  IsReferenceTopField,
                                  NumberOfLongTermFrames,
                                  RefFrameListXLongTerm,
                                  h264par_IsCurrentParityLongRef,
                                  h264par_IsAlternateParityLongRef,
                                  ParserHandle);
}

/******************************************************************************/
/*! \brief Computes the reference picture list for P slices in field
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_InitRefPicListForPSlicesInField(const PARSER_Handle_t ParserHandle)
{
    U8 i; /* to loop over the DPBRef */
    U8 NumberOfShortTermFrames;
    U8 NumberOfLongTermFrames;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* The processing is described in the standard 8.2.4.2.2 and 8.2.4.2.5 */

    /* Build two lists: the short-term and the long-term reference frames */
    /* All fields are considered */
    NumberOfShortTermFrames = 0;
    NumberOfLongTermFrames = 0;
    for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
    {
        if (DPBRefFrameHasShortTermField(i))
        {
            PARSER_Data_p->RefFrameList0ShortTerm[NumberOfShortTermFrames] = PARSER_Data_p->DPBFrameReference[i];
            NumberOfShortTermFrames++;
        }
        else if (DPBRefFrameHasLongTermField(i))
        {
            PARSER_Data_p->RefFrameList0LongTerm[NumberOfLongTermFrames] = PARSER_Data_p->DPBFrameReference[i];
            NumberOfLongTermFrames++;
        }
    }

    /* Sort the short-term list with descending FrameNumWrap */
    qsort((void *)PARSER_Data_p->RefFrameList0ShortTerm, NumberOfShortTermFrames, sizeof(PARSER_Data_p->RefFrameList0ShortTerm[0]), h264par_SortDescendingFrameNumWrapFunction);

    /* Sort the long-term list with ascending LongTermFrameIdx */
    qsort((void *)PARSER_Data_p->RefFrameList0LongTerm, NumberOfLongTermFrames, sizeof(PARSER_Data_p->RefFrameList0LongTerm[0]), h264par_SortAscendingLongTermFrameIdxFunction);

    /* Build ReferenceListP0 and IsReferenceTopFieldP0 */
    h264par_BuildReferenceFieldsList(&(PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0),
                                     PARSER_Data_p->PictureGenericData.ReferenceListP0,
                                     PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0,
                                     NumberOfShortTermFrames,
                                     NumberOfLongTermFrames,
                                     PARSER_Data_p->RefFrameList0ShortTerm,
                                     PARSER_Data_p->RefFrameList0LongTerm,
                                     ParserHandle);
}

/******************************************************************************/
/*! \brief Computes the reference picture list for B slices in field
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_InitRefPicListForBSlicesInField(const PARSER_Handle_t  ParserHandle)
{
    U8 i; /* to loop over the DPBRef */
    U8 NumberOfShortTermFrames;
    U8 NumberOfShortTermFramesBelowPOC;
    U8 NumberOfShortTermFramesAbovePOC;
    U8 NumberOfRefFrameListLongTerm;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* The processing is described in the standard 8.2.4.2.3 */

    /* Build two lists: the short-term and the long-term reference frames */
    /* All fields are not considered */
    NumberOfShortTermFramesBelowPOC = 0;
    NumberOfShortTermFramesAbovePOC = 0;
    NumberOfRefFrameListLongTerm = 0;
    for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
    {
        if (DPBRefFrameHasShortTermField(i))
        {
            if ((PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->pic_order_cnt_type != 0) || (PARSER_Data_p->DPBFrameReference[i].IsNonExisting == FALSE))
            {
                /* Non-existing frames have no POC when pic_order_cnt_type = 0, thus they cannot be sorted in this process. */
                /* See corrigendum K012 */
                /* In the following test, The i-th index in DPB is assumed to have a defined Top and Bottom POC so that the
                 * comparison can take place ( even if the i-th index stores a single field)
                 * This is done when marking the picture
                 */
                if (h264par_PicOrderCntDPB(&(PARSER_Data_p->DPBFrameReference[i])) <= PARSER_Data_p->PictureSpecificData.PreDecodingPicOrderCnt)
                {
                    PARSER_Data_p->ShortTermFramesBelowPOC[NumberOfShortTermFramesBelowPOC] = PARSER_Data_p->DPBFrameReference[i];
                    NumberOfShortTermFramesBelowPOC++;
                }
                else
                {
                    PARSER_Data_p->ShortTermFramesAbovePOC[NumberOfShortTermFramesAbovePOC] = PARSER_Data_p->DPBFrameReference[i];
                    NumberOfShortTermFramesAbovePOC++;
                }
            }
        }
        else if (DPBRefFrameHasLongTermField(i))
        {
            PARSER_Data_p->RefFrameList0LongTerm[NumberOfRefFrameListLongTerm] = PARSER_Data_p->DPBFrameReference[i];
            NumberOfRefFrameListLongTerm++;
        }
    }

    /* Sort the short-term list "below POC" with descending PicOrderCnt */
    qsort((void *)PARSER_Data_p->ShortTermFramesBelowPOC, NumberOfShortTermFramesBelowPOC, sizeof(PARSER_Data_p->ShortTermFramesBelowPOC[0]), h264par_SortDescendingPicOrderCntFunction);

    /* Sort the short-term list "above POC" with ascending PicOrderCnt */
    qsort((void *)PARSER_Data_p->ShortTermFramesAbovePOC, NumberOfShortTermFramesAbovePOC, sizeof(PARSER_Data_p->ShortTermFramesAbovePOC[0]), h264par_SortAscendingPicOrderCntFunction);

    /* Sort the long-term list with ascending LongTermFrameIdx */
    qsort((void *)PARSER_Data_p->RefFrameList0LongTerm, NumberOfRefFrameListLongTerm, sizeof(PARSER_Data_p->RefFrameList0LongTerm[0]), h264par_SortAscendingLongTermFrameIdxFunction);

    /* Build PARSER_Data_p->RefFrameList0ShortTerm: PARSER_Data_p->ShortTermFramesBelowPOC followed by PARSER_Data_p->ShortTermFramesAbovePOC */
    for (i = 0; i < NumberOfShortTermFramesBelowPOC; i ++)
    {
        PARSER_Data_p->RefFrameList0ShortTerm[i] = PARSER_Data_p->ShortTermFramesBelowPOC[i];
    }
    for (i = 0; i < NumberOfShortTermFramesAbovePOC; i ++)
    {
        PARSER_Data_p->RefFrameList0ShortTerm[NumberOfShortTermFramesBelowPOC + i] = PARSER_Data_p->ShortTermFramesAbovePOC[i];
    }

    /* Build PARSER_Data_p->RefFrameList1ShortTerm: PARSER_Data_p->ShortTermFramesAbovePOC followed by PARSER_Data_p->ShortTermFramesBelowPOC */
    for (i = 0; i < NumberOfShortTermFramesAbovePOC; i ++)
    {
        PARSER_Data_p->RefFrameList1ShortTerm[i] = PARSER_Data_p->ShortTermFramesAbovePOC[i];
    }
    for (i = 0; i < NumberOfShortTermFramesBelowPOC; i ++)
    {
        PARSER_Data_p->RefFrameList1ShortTerm[NumberOfShortTermFramesAbovePOC + i] = PARSER_Data_p->ShortTermFramesBelowPOC[i];
    }
    NumberOfShortTermFrames = NumberOfShortTermFramesBelowPOC + NumberOfShortTermFramesAbovePOC;

    /* Build RefPicListB0 and IsReferenceTopFieldB0 */
    h264par_BuildReferenceFieldsList(&(PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0),
                                     PARSER_Data_p->PictureGenericData.ReferenceListB0,
                                     PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0,
                                     NumberOfShortTermFrames,
                                     NumberOfRefFrameListLongTerm,
                                     PARSER_Data_p->RefFrameList0ShortTerm,
                                     PARSER_Data_p->RefFrameList0LongTerm,
                                     ParserHandle);

    /* Build RefPicListB1 and IsReferenceTopFieldB1 */
    h264par_BuildReferenceFieldsList(&(PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1),
                                     PARSER_Data_p->PictureGenericData.ReferenceListB1,
                                     PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1,
                                     NumberOfShortTermFrames,
                                     NumberOfRefFrameListLongTerm,
                                     PARSER_Data_p->RefFrameList1ShortTerm,
                                     PARSER_Data_p->RefFrameList0LongTerm,
                                     ParserHandle);

    h264par_CheckIfB0AndB1ListAreEqual(ParserHandle);
}

/******************************************************************************/
/*! \brief Computes the reference picture lists for the current picture.
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ReferencePictureListsInitialisation(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if ((PARSER_Data_p->PictureSpecificData.IsIDR) || /* nal_unit_type == 5 */
        ((PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable == TRUE) && /* or change detected in global context */
        ((PARSER_Data_p->PreviousPictureInDecodingOrder.Width != PARSER_Data_p->PictureLocalData.Width) ||
        (PARSER_Data_p->PreviousPictureInDecodingOrder.Height != PARSER_Data_p->PictureLocalData.Height) ||
        (PARSER_Data_p->PreviousPictureInDecodingOrder.MaxDecFrameBuffering != PARSER_Data_p->PictureLocalData.MaxDecFrameBuffering))))
    {
        /* All reference pictures shall be marked as "unused for reference" */
        h264par_MarkAllPicturesAsUnusedForReference(ParserHandle);
        PARSER_Data_p->PictureGenericData.AsynchronousCommands.FlushPresentationQueue = TRUE;
    }

    /* First pass: computes the full reference frame list and type list */
    h264par_InitFullList(ParserHandle);

    if (CurrentPictureIsFrame)
    {
        /* Second pass: computes the Reference List for P slices */
        h264par_InitRefPicListForPSlicesInFrame(ParserHandle);

        /* Third pass: computes the Reference Lists for B slices */
        h264par_InitRefPicListForBSlicesInFrame(ParserHandle);
    }
    else
    {
        /* Second pass: computes the Reference List for P slices */
        h264par_InitRefPicListForPSlicesInField(ParserHandle);

        /* Third pass: computes the Reference Lists for B slices */
        h264par_InitRefPicListForBSlicesInField(ParserHandle);
    }
}
