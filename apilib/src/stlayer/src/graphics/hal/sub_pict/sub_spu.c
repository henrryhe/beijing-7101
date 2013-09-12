/*******************************************************************************

File name   : sub_spu.c

Description : Sub picture unit source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
18 Dec 2000        Created                                           TM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sub_spu.h"
#include "stavmem.h"
#include "sub_copy.h"
#include "sttbx.h"
#include "sub_reg.h"
#include "stsys.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : layersub_SetSPUHeader
Description : Write SPU header (2 bytes)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_SetSPUHeader(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p)
{
    ST_ErrorCode_t  Err         = ST_NO_ERROR;
    U8*             Address_p;


    Address_p = (U8*)STAVMEM_VirtualToCPU((void*)ViewPort_p->Header_p,&(Device_p->VirtualMapping));

    /* Size of SPU */
    *(Address_p)        = (LAYERSUB_SPU_MAX_SIZE >> 8) & 0xFF;
    *(Address_p + 1)    = LAYERSUB_SPU_MAX_SIZE & 0xFF;

    /* Offset to DCSQ */
    *(Address_p + 2)    = ((LAYERSUB_SPU_HEADER_SIZE + LAYERSUB_SPU_MAX_DATA_SIZE)  >> 8) & 0xFF;
    *(Address_p + 3)    = (LAYERSUB_SPU_HEADER_SIZE + LAYERSUB_SPU_MAX_DATA_SIZE) & 0xFF;

    return(Err);
}

/*******************************************************************************
Name        : layersub_UpdateContrast
Description : Update Contrast values in DSCQ
Parameters  :
Assumptions : ViewPort_p->Alpha[i] is at least a 4 elements array. range 0->128 /  0->16 on subpicture!
              Take into account Global Alpha
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_UpdateContrast(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p)
{
    ST_ErrorCode_t  Err         = ST_NO_ERROR;
    U8*             Address_p;
    U8              Alpha4[4];
    U8              ResultingAlpha;
    int             i;

   Address_p = (U8*)STAVMEM_VirtualToCPU((void*)ViewPort_p->DCSQ_p,&(Device_p->VirtualMapping));

   /* TBD */
    for (i=0;i<4;i++)
    {
        ResultingAlpha = (U8)(((U32)(ViewPort_p->Alpha[i]) * (U32)(ViewPort_p->GlobalAlpha.A0) / 128) & 0xFF);
        Alpha4[i] = ResultingAlpha >> 3;
        if (Alpha4[i] >= 16)
        {
            Alpha4[i] = 0xF; /* opaque */
        }
    }

    *(Address_p + 9)     = (U8)(((Alpha4[3] & 0xF) << 4) | (Alpha4[2] & 0xF));
    *(Address_p + 10)    = (U8)(((Alpha4[1] & 0xF) << 4) | (Alpha4[0] & 0xF));

    return(Err);
}



/*******************************************************************************
Name        : layersub_SetDCSQ
Description : Write SPU DCSQ (24 bytes in our case)
Parameters  :
Assumptions : YStart position is even! (lowest bit set to 0)
              Width, Height and Positions are considered to be adjusted !
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_SetDCSQ(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p)
{
    ST_ErrorCode_t  Err         = ST_NO_ERROR;
    U8*             Address_p;

    Address_p = (U8*)STAVMEM_VirtualToCPU((void*)ViewPort_p->DCSQ_p,&(Device_p->VirtualMapping));

    /* Start Time */
    *(Address_p)        = 0;
    *(Address_p + 1)    = 0;

    /* Start Address of next = itself */
    *(Address_p + 2)    = ((LAYERSUB_SPU_HEADER_SIZE + LAYERSUB_SPU_MAX_DATA_SIZE)  >> 8) & 0xFF;;
    *(Address_p + 3)    = (LAYERSUB_SPU_HEADER_SIZE + LAYERSUB_SPU_MAX_DATA_SIZE) & 0xFF;

    /* Command STA_DSP */
    *(Address_p + 4)    = 0x1;

    /* Command SET_COLOR */
    *(Address_p + 5)    = 0x3;
    *(Address_p + 6)    = 0x32;
    *(Address_p + 7)    = 0x10;

    /* Command SET_CONTR (Opaque per default) */
    *(Address_p + 8)     = 0x4;
    *(Address_p + 9)     = 0xFF;
    *(Address_p + 10)    = 0xFF;

    /* Command SET_DAREA (Rectangle Width / height, Position is O,O (Rect PositionX and Y already taken into account when RLE))*/
    *(Address_p + 11)    = 0x5;
    *(Address_p + 12)    = 0;
    *(Address_p + 13)    = ((ViewPort_p->InputRectangle.Width - 1) >> 8) & 0x3 ;
    *(Address_p + 14)    = (ViewPort_p->InputRectangle.Width  - 1) & 0xFF;
    *(Address_p + 15)    = 0;
    *(Address_p + 16)    = ((ViewPort_p->InputRectangle.Height - 1) >> 8) & 0x3 ;
    *(Address_p + 17)    = (ViewPort_p->InputRectangle.Height  - 1) & 0xFF;

    /* Command SET_DSPXA */
    *(Address_p + 18)    = 0x6;
    if ((ViewPort_p->OutputRectangle.PositionY % 2) == 0) /* First line on Top field */
    {
        *(Address_p + 19)    = (LAYERSUB_SPU_HEADER_SIZE >> 8) & 0xFF;
        *(Address_p + 20)    = LAYERSUB_SPU_HEADER_SIZE & 0xFF;
        *(Address_p + 21)    = ((LAYERSUB_SPU_HEADER_SIZE + (LAYERSUB_SPU_MAX_DATA_SIZE / 2)) >> 8) & 0xFF;
        *(Address_p + 22)    = (LAYERSUB_SPU_HEADER_SIZE + (LAYERSUB_SPU_MAX_DATA_SIZE / 2)) & 0xFF;
    }
    else /* First line on Bottom field */
    {
        *(Address_p + 19)    = ((LAYERSUB_SPU_HEADER_SIZE + (LAYERSUB_SPU_MAX_DATA_SIZE / 2)) >> 8) & 0xFF;
        *(Address_p + 20)    = (LAYERSUB_SPU_HEADER_SIZE + (LAYERSUB_SPU_MAX_DATA_SIZE / 2)) & 0xFF;
        *(Address_p + 21)    = (LAYERSUB_SPU_HEADER_SIZE >> 8) & 0xFF;
        *(Address_p + 22)    = LAYERSUB_SPU_HEADER_SIZE & 0xFF;
    }

    /* Command CMD_END */
    *(Address_p + 23)    = 0xFF;


    return(Err);
}


/*******************************************************************************
Name        : layersub_SetData
Description : Copy Top/Bot in SPU
Parameters  :
Assumptions : Bitmap is 2bpp, Non NULL and valid ViewPort
              Input Rectangle included fully into bitmap
              ViewPort_p->InputRectangle.Height must be even!
              First Impementation : Positions are > 0
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_SetData(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p)
{
    ST_ErrorCode_t      Err      = ST_NO_ERROR;
    U8*                 AddressFirstLine_p;
    U8*                 AddressSecondLine_p;
    STGXOBJ_Bitmap_t*   Bitmap_p = &(ViewPort_p->Bitmap);
    U32                 SrcByteWidth;
    U8                  Rest;
    U32                 LeftPixel, LastPixelInLine, DstPitch;
    U8                  LeftCorrection, RightCorrection;
    U8                  BitsSLeft, BitsSRight, BitsDLeft, BitsDRight;

    /* Find address of byte containing the first pixel of input rectangle (firstline and second line)*/
    AddressFirstLine_p = (U8*)(Bitmap_p->Pitch * ViewPort_p->InputRectangle.PositionY + (ViewPort_p->InputRectangle.PositionX) / 4  +
                          (U32)(Bitmap_p->Data1_p) + Bitmap_p->Offset);
    AddressFirstLine_p = (U8*)STAVMEM_VirtualToCPU((void*)AddressFirstLine_p,&(Device_p->VirtualMapping));

    AddressSecondLine_p = (U8*)((U32)AddressFirstLine_p  + Bitmap_p->Pitch);

    /* Number of remaining pixels at left side of Input rectangle */
    LeftPixel = (ViewPort_p->InputRectangle.PositionX) % 4;

    /* Calculate bit offset between the left side of Input rectangle and the next byte*/
    BitsSLeft = (4 - LeftPixel) * 2;
    LeftCorrection = LeftPixel;

    /* Calculate bit offset between the last byte align pixel in Input rect and pixel at the right side of the Input Rect */
    LastPixelInLine = ViewPort_p->InputRectangle.PositionX + ViewPort_p->InputRectangle.Width - 1;
    Rest            = (LastPixelInLine + 1) % 4;
    if (Rest != 0)
    {
        BitsSRight      = Rest * 2 ;
        RightCorrection = 4 - Rest ;
    }
    else
    {
        BitsSRight      = 8;
        RightCorrection = 0;
    }

    /* Calculate Byte with to copy taking into account non byte align pixels at left and right sire of input rectangle */
    SrcByteWidth = (ViewPort_p->InputRectangle.Width + LeftCorrection + RightCorrection) / 4;

    /* First left pixel of output rectangle always byte aligned in SPU*/
    BitsDLeft = 8;

    /* Calculate bit offset between the last byte align pixel in Output rect and pixel at the right side of the Output Rect
     * Assumption : First left pixel of output rectangle always byte aligned in SPU */
    Rest = ViewPort_p->OutputRectangle.Width % 4;
    if (Rest != 0)
    {
        BitsDRight = Rest * 2 ;
        RightCorrection = 4 - Rest ;
    }
    else
    {
        BitsDRight  = 8;
        RightCorrection = 0;
    }

    /* Calculate Dst Pitch (left correction is always 0)*/
    DstPitch = (ViewPort_p->OutputRectangle.Width + RightCorrection) / 4;

    /* Copy Data */
    if ((BitsSLeft == 8) && ( BitsSRight == 8) && (BitsDLeft == 8) && (BitsDRight == 8))
    {
        /* Copy Top */
        STAVMEM_CopyBlock2D(AddressFirstLine_p, SrcByteWidth, ViewPort_p->InputRectangle.Height / 2, Bitmap_p->Pitch * 2,
                            STAVMEM_VirtualToCPU((void*)ViewPort_p->RLETop_p,&(Device_p->VirtualMapping)), DstPitch);


        /* Copy Bottom */
        STAVMEM_CopyBlock2D(AddressSecondLine_p, SrcByteWidth, ViewPort_p->InputRectangle.Height / 2, Bitmap_p->Pitch * 2,
                            STAVMEM_VirtualToCPU((void*)ViewPort_p->RLEBottom_p,&(Device_p->VirtualMapping)), DstPitch);
    }
    else
    {
        /* Copy Top */
        layersub_MaskedCopyBlock2D(AddressFirstLine_p,SrcByteWidth,ViewPort_p->InputRectangle.Height / 2, Bitmap_p->Pitch * 2,
                                   STAVMEM_VirtualToCPU((void*)ViewPort_p->RLETop_p,&(Device_p->VirtualMapping)),
                                   DstPitch, BitsSLeft, BitsSRight, BitsDLeft, BitsDRight, FALSE, FALSE);

       /* Copy Bottom */
       layersub_MaskedCopyBlock2D(AddressSecondLine_p,SrcByteWidth,ViewPort_p->InputRectangle.Height / 2, Bitmap_p->Pitch * 2,
                                  STAVMEM_VirtualToCPU((void*)ViewPort_p->RLEBottom_p,&(Device_p->VirtualMapping)),
                                  DstPitch, BitsSLeft, BitsSRight, BitsDLeft, BitsDRight, FALSE, FALSE);
    }

    return(Err);
}

/*******************************************************************************
Name        : layersub_SwapActiveSPU
Description : change the active SPU :
              Prepare new SPU, swap with old one, release old one and update ViewPort Descriptor
Parameters  :
Assumptions : SPU is active (already started)!
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_SwapActiveSPU(layersub_Device_t* Device_p, layersub_ViewPort_t* ViewPort_p)
{
    ST_ErrorCode_t  Err         = ST_NO_ERROR;
    STAVMEM_BlockHandle_t       NewSubPictureUnitMemoryHandle;
    STAVMEM_AllocBlockParams_t  AllocBlockParams;
    STAVMEM_FreeBlockParams_t   FreeParams;
    U8                          NbForbiddenRange = 0;
    STAVMEM_MemoryRange_t       RangeArea[2];

    /* Allocation of a STU in shared memory
    * First implementation for Size = Max Size*/
    /* Forbidden range : All Virtual memory range but Virtual window
     *  VirtualWindowOffset may be 0
     *  Virtual Size is always > 0
     *  Assumption : Physical base Address from device = 0 !!!! */
    if (Device_p->VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) Device_p->VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(Device_p->VirtualMapping.VirtualWindowOffset) - 1);
        NbForbiddenRange++;
    }

    if ((Device_p->VirtualMapping.VirtualWindowOffset + Device_p->VirtualMapping.VirtualWindowSize) !=
         Device_p->VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(Device_p->VirtualMapping.VirtualWindowOffset) +
                                             (U32)(Device_p->VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(Device_p->VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(Device_p->VirtualMapping.VirtualSize) - 1);
        NbForbiddenRange++;
    }

    AllocBlockParams.PartitionHandle          = Device_p->AVMEMPartition;
    AllocBlockParams.Size                     = LAYERSUB_SPU_MAX_SIZE;
    AllocBlockParams.Alignment                = 2048;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    Err = STAVMEM_AllocBlock(&AllocBlockParams,&NewSubPictureUnitMemoryHandle);
    if (Err != ST_NO_ERROR)
    {
       STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
       return(STLAYER_ERROR_NO_AV_MEMORY);
    }

    /* Update ViewPort Descriptor to point to new SPU */
    STAVMEM_GetBlockAddress(NewSubPictureUnitMemoryHandle,(void**)&(ViewPort_p->Header_p));
    ViewPort_p->RLETop_p    = (void*)((U32)ViewPort_p->Header_p + LAYERSUB_SPU_HEADER_SIZE);
    ViewPort_p->RLEBottom_p = (void*)((U32)ViewPort_p->RLETop_p + (LAYERSUB_SPU_MAX_DATA_SIZE / 2));
    ViewPort_p->DCSQ_p      = (void*)((U32)ViewPort_p->Header_p + LAYERSUB_SPU_HEADER_SIZE + LAYERSUB_SPU_MAX_DATA_SIZE);

    /* Write SPU Header */
    layersub_SetSPUHeader(Device_p, ViewPort_p);

    /* Write Top and Bottom data */
    layersub_SetData(Device_p, ViewPort_p);

    /* Write Display Control sequence commands */
    layersub_SetDCSQ(Device_p, ViewPort_p);

    /* -------- At this stage new SPU is ready to be swapped --------------*/

    if (Device_p->IsEvtVsyncSubscribed == FALSE)
    {
        /* Swap SPU */
        layersub_StopSPU(Device_p);

        /* Sub picture decoder Soft reset */
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SPR_OFFSET),LAYERSUB_SPD_SPR);
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SPR_OFFSET),0);
        /* Reset auto increment address counter */
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_CTL2_OFFSET), LAYERSUB_SPD_CTL2_RC);
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_CTL2_OFFSET), 0);

        layersub_StartSPU(Device_p);
    }
    else
    {
        Device_p->SwapActiveSPU  = TRUE;
    }


    /* Release old SPU */
    FreeParams.PartitionHandle = Device_p->AVMEMPartition;
    Err = STAVMEM_FreeBlock(&FreeParams,&(ViewPort_p->SubPictureUnitMemoryHandle));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Shared memory : %d\n",Err));
        /* return (); */
    }

    /* Update ViewPort SPU Memory handle */
    ViewPort_p->SubPictureUnitMemoryHandle = NewSubPictureUnitMemoryHandle;


    return(Err);
}






/*
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
*/


/* End of sub_spu.c */

