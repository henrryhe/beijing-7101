/******************************************************************************\
 *
 * File Name   : obj.c
 *
 * Description : Contain implementation of Object processor. It process a
 *               single Object data segment.
 *
 *               See description of single funtions and SDD document for
 *               more details.
 *
 * Copyright 1998, 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Alessandro Innocenti - February 1999
 *
\******************************************************************************/
 
#include <stdlib.h>
#include <string.h>

 
#include <stddefs.h>
#include <stlite.h>

#include <subtdev.h>
#include "compbuf.h"
#include "pixbuf.h"
#include "decoder.h"

static __inline 
BOOL CheckLenLine(STSUBT_InternalHandle_t *,ReferingRegion_t *, U16,U8); 

static __inline 
void SetBadLine(ReferingRegion_t *, U16 ,U8 );


enum
{
    FIELD_0, 
    FIELD_1 
};

typedef struct Map_Table_s 
{
    U8    Two_to_4_bit_map_table[4];
    U8    Two_to_8_bit_map_table[4];
    U8    Four_to_8_bit_map_table[16];
} Map_Table_t;


/******************************************************************************\
 * Function: Read_next_2_bits
 *
 * Purpose : Extract 2 bits from the segment. 
 *
 * Description: The first time the function is called it read a byte from the 
 *              coded_data_buffer (with the function get_byte()), and return
 *              only the first 2 bits. It uses 1 static variable to remember
 *              the other 6 bits that will be returned at the following 
 *              call. Another static variable is used to know when when all 
 *              bits have been returned and is necessary to read another byte.
 *
 * Return  : U8 with 6 higth bits set to 0.
\******************************************************************************/

static __inline
U8 Read_next_2_bits (Segment_t *Segment,
                     U16 *processed_length,
                     U8  reset)
{
    static U8 GetNewByte = 0;
    static U8 TmpByte = 0;
    U8  PixelCode = 0;

    if(reset)
    {
        GetNewByte = 0;
        return(0);
    }

    switch (GetNewByte)
    {
        case  0 :
            TmpByte = get_byte(Segment);
            *processed_length += 1;
            PixelCode = TmpByte >> 6;
            GetNewByte = 1;
            break;
        case 1 :
            PixelCode = ((TmpByte & 0x30) >> 4);
            GetNewByte = 2;
            break;
        case 2 :
            PixelCode = ((TmpByte & 0x0c) >> 2);
            GetNewByte = 3;
            break;
        case 3 :
            PixelCode = (TmpByte & 0x03);
            GetNewByte = 0;
            break;
        default :
            break;
    }
    return(PixelCode);
}


/******************************************************************************\
 * Function: Read_next_4_bits
 *
 * Purpose : Extract 4 bits from the segment. 
 *
 * Description: The first time the function is called it read a byte from the 
 *              coded_data_buffer (with the function get_byte()), and return
 *              only the first 4 bits. It uses 1 static variable to remember
 *              the other 4 bits that will be returned at the following 
 *              call. Another static variable is used to know when when all 
 *              bits have been returned and is necessary to read another byte.
 *
 * Return  : U8 with 4 higth bits set to 0.
\******************************************************************************/

static __inline
U8 Read_next_4_bits (Segment_t *Segment,
                     U16 *processed_length,
                     U8 reset)
{
    static U8 GetNewByte = 0 ;
    static U8 TmpByte = 0;
    U8  PixelCode = 0;

    if(reset)
    {
        GetNewByte = 0;
        return(0);
    }

    if (GetNewByte == 0)
    {
        TmpByte = get_byte(Segment);
        *processed_length += 1;
        PixelCode = (TmpByte >> 4);
        GetNewByte = 1;
    }
    else
    {
        PixelCode = (TmpByte & 0x0f);
        GetNewByte = 0;
    }
    return(PixelCode);
}



/******************************************************************************\
 * Function: decode_2_pel_code_string
 *
 * Purpose : Process a single 2bits/pel code string to read the number and
 *           the color of the pixels. These value are returned in the two
 *           variable NumPixel and ColorEntry
 *
 * Description: 
 *
\******************************************************************************/

static __inline 
void decode_2_pel_code_string (Segment_t *Segment,
                               U8 *ColorEntry,U16 *NumPixel,
                               U16 *processed_length)
{
    U8    PixelCode;


    PixelCode = Read_next_2_bits(Segment, processed_length, 0);
    if (PixelCode != 0x00)  
    {   /* handle 01, 10, 11 cases */
        *NumPixel = 1;
        *ColorEntry = PixelCode;
    }
    else
    {   
        PixelCode = Read_next_2_bits(Segment, processed_length, 0);
        if (PixelCode == 0x01)
        {   /* handles 00 01 case */
            *NumPixel = 1;
            *ColorEntry = 0x00;
        } 
        else if (PixelCode >= 0x2)
        {   /* handles 00 1L LL CC case */
            *NumPixel = ((PixelCode & 1) <<2) +
                        Read_next_2_bits(Segment, processed_length,0) + 3;
            *ColorEntry = Read_next_2_bits(Segment, processed_length, 0);
        }
        else if (PixelCode == 0)
        {
            PixelCode = Read_next_2_bits(Segment, processed_length, 0);
            if (PixelCode == 0x1)
            {   /* handles 00 00 01 case */
                *NumPixel = 2;
                *ColorEntry = 0;
            }
            else if (PixelCode == 0x2)
            {   /* handles 00 00 10 LL LL CC case */
                *NumPixel = (Read_next_2_bits(Segment, processed_length,0) << 2) + 
                            Read_next_2_bits(Segment, processed_length,0) + 12;
                *ColorEntry = Read_next_2_bits(Segment, processed_length, 0);
            }
            else if (PixelCode == 0x3)
            {   /* handles 00 00 11 LL LL LL LL CC case */
                *NumPixel = (Read_next_2_bits(Segment, processed_length,0) << 6) + 
                            (Read_next_2_bits(Segment, processed_length,0) << 4) + 
                            (Read_next_2_bits(Segment, processed_length,0) << 2) +
                            Read_next_2_bits(Segment, processed_length,0) + 29;
                *ColorEntry = Read_next_2_bits(Segment, processed_length, 0);
            }
            else
            {   /* handles 00 00 00 case */
                *NumPixel = 0;
                *ColorEntry = 0;
            }
        }        
    }
}




/******************************************************************************\
 * Function: decode_4_pel_code_string
 *
 * Purpose : Process a single 4bits/pel code string to read the number and
 *           the color of the pixels. These value are returned in the two
 *           variable NumPixel and ColorEntry
 *
 * Description: 
 *
\******************************************************************************/

static __inline 
void decode_4_pel_code_string (Segment_t *Segment,
                               U8 *ColorEntry,U16 *NumPixel,
                               U16 *processed_length)
{
    U8    PixelCode;


    PixelCode = Read_next_4_bits(Segment, processed_length,0);
    if (PixelCode >  0x00)
    {
        *NumPixel = 1;
        *ColorEntry = PixelCode;
    }
    else
    {
        PixelCode = Read_next_4_bits(Segment, processed_length,0);

        if (PixelCode == 0x00)
        {
   	     /*   end_of_string_signal   */

             *NumPixel = 0x00;
             *ColorEntry = 0;
        }
        else if (PixelCode <= 0x07)
        {
	   /* run_length_3-9 	*/
              *NumPixel = (PixelCode + 2);
              *ColorEntry = 0;
        }
        else if (PixelCode <= 0x0b)
        {
	   /* run_length_4-7 	*/
            *NumPixel = ((PixelCode & 0x03) + 4);
            *ColorEntry = Read_next_4_bits(Segment, processed_length,0);
        }
        else if (PixelCode <= 0x0d)
        {
            *NumPixel = ((PixelCode & 0x03) + 1);
            *ColorEntry = 0;
        }
        else if (PixelCode == 0x0e)
        {
	   /* run_length_9-24 	*/
          *NumPixel = (Read_next_4_bits(Segment, processed_length,0) + 9);
          *ColorEntry = Read_next_4_bits(Segment, processed_length,0);
        }
        else if (PixelCode == 0x0f) 
        {
	   /* run_length_25-280 	*/
          *NumPixel = ((Read_next_4_bits(Segment,processed_length,0) << 4)
		 & 0xf0);
          *NumPixel += Read_next_4_bits(Segment, processed_length,0);
          *NumPixel += 25;
          *ColorEntry = Read_next_4_bits(Segment, processed_length,0);
        }
    }
}


/******************************************************************************\
 * Function: decode_8_pel_code_string
 *
 * Purpose : Process a single 8 bits/pel code string to read the number and
 *           the color of the pixels. These value are returned in the two
 *           variable NumPixel and ColorEntry
 *
 * Description: 
 *
\******************************************************************************/
static __inline 
void decode_8_pel_code_string (Segment_t *Segment,
                               U8 *ColorEntry,U16 *NumPixel,
                               U16 *processed_length)
{
    U8    PixelCode;


    PixelCode = get_byte(Segment);
    *processed_length += 1;
    if (PixelCode != 0x00)
    {
        *NumPixel = 1;
        *ColorEntry = PixelCode;
    }
    else
    {
        PixelCode = get_byte(Segment);
        *processed_length += 1;
        if (PixelCode == 0x00)
        {
            *NumPixel = 0x00;
            *ColorEntry = 0;
        }
        else if (PixelCode <= 0x7f)
        {
            *NumPixel = PixelCode;
            *ColorEntry = 0;
        }
        else 
        {
            *NumPixel = (PixelCode & 0x7f);
            *ColorEntry = get_byte(Segment);
            *processed_length += 1;
        }
    }
}


/******************************************************************************\
 * Function: WriteNBits
 *
 * Purpose : Write in the Region's Pixel Buffer memory a number of bits equal
 *           to region_depth. 
 *
 * Description: 
 *
\******************************************************************************/
static __inline 
void  WriteNBits(ReferingRegion_t *RefReg,
                          U8 ColorEntry, U8 OffsetMask)
{
    *(RefReg->ObjLookahead_p) = ((*(RefReg->ObjLookahead_p)) &
                                (~(OffsetMask >> (RefReg->ObjOffset)))) +
             (ColorEntry << (8 - (RefReg->region_depth) - (RefReg->ObjOffset)));

    (RefReg->ObjOffset) += (RefReg->region_depth);
    if ((RefReg->ObjOffset) == 8)
    {
        (RefReg->ObjOffset) = 0;
        (RefReg->ObjLookahead_p) += 1;
    }
}


/******************************************************************************\
 * Function: PixelBuffer_WriteNPixels
 *
 * Purpose :Write in the Region's Pixel Buffer memory 'NumPixel' pixels of
 *          'ColorEntry' color. The number of bits written for each pixel
 *          depends by region_depth
 *
 * Description: First the memory is aligned to the byte then byte are written
 *              in one block copy. Eventualy remaining bits are written
 *              at the end.
 *
\******************************************************************************/
static __inline 
void PixelBuffer_WriteNPixels(ReferingRegion_t *RefReg, U8 ColorEntry,
                              U16 NumPixel, U8 non_modifying_colour_flag)
{
    U8  cont = 0;
    U8  OffsetMask = 0;
    U8  ByteMask = 0;
    U16 ResPixel = 0;
    U32 PixelsWritten = 0;
    U32 ByteToWrite = 0;

    /***   check non_modifying_colour    ***/

    if ((non_modifying_colour_flag == 1) && (ColorEntry == 1))
    {
        if (NumPixel >= ((8 - RefReg->ObjOffset) / RefReg->region_depth))
        {
            ResPixel = NumPixel - 
                       ((8 - RefReg->ObjOffset) / RefReg->region_depth);

            RefReg->ObjLookahead_p += (1+ (ResPixel / RefReg->NumPixelperByte));

            RefReg->ObjOffset = (ResPixel % RefReg->NumPixelperByte);
        }
        else
        {
        RefReg->ObjOffset += NumPixel * RefReg->region_depth;
        }
        return;
    }


    OffsetMask= ((1 << RefReg->region_depth) - 1) << (8 - RefReg->region_depth);

    while ((RefReg->ObjOffset != 0) && (PixelsWritten < NumPixel))
    {
        WriteNBits(RefReg,ColorEntry,OffsetMask);
        PixelsWritten += 1;
    }


    ByteToWrite = (NumPixel - PixelsWritten) / (RefReg->NumPixelperByte);

    if(ByteToWrite > 0)
    {
        for (cont = 0; cont < (RefReg->NumPixelperByte); cont++)
            ByteMask = (ByteMask << (RefReg->region_depth)) + ColorEntry;

       	memset(RefReg->ObjLookahead_p, ByteMask, ByteToWrite); 

        (RefReg->ObjLookahead_p) += ByteToWrite;

        PixelsWritten += (ByteToWrite * (RefReg->NumPixelperByte));
    }



    while (PixelsWritten < NumPixel)
    {
        WriteNBits(RefReg,ColorEntry,OffsetMask); 
        PixelsWritten += 1;
    }


}


/******************************************************************************\
 * Function: Field_data_decoder
 *
 * Purpose : Read a single field_data_block and write decoded pixel in
 *           pixel buffer memory of refering region.
 *
 * Description: 
 *
 * Comments: It is assumed that object don't exit from region boundary.
 *           Such control is possible but it is very onerous so at the moment 
 *           is not implemented.
 *
\******************************************************************************/
 
static __inline
void Field_data_decoder (STSUBT_InternalHandle_t *DecoderHandle,
                         Segment_t *Segment,
                         ReferingRegion_t *ReferingRegion_list,
                         U16 field_data_block_length,
                         Map_Table_t *Map_Table,
                         U8 non_modifying_colour_flag,
			 U8 Field)
{
    U8    TmpByte = 0;
    U8    cont = 0;
    U8    data_type = 0;
    U8    ColorEntry = 0;
    U8    MappedColorEntry = 0;
    U16   NumPixel = 0;
    U16   NumLine = 0;
    U16   processed_length = 0;
	

    ReferingRegion_t  *TmpRefReg = NULL;
    STSUBT_Bitmap_t   *RegBitmapHead = NULL;


    while(processed_length < field_data_block_length)
    {

        TmpByte = get_byte(Segment);

        processed_length += 1;

        /*** We are not sure if there are sync_byte, segment_type,    ***/
        /*** page_id and segment_length in each pixel_data_sub_block. ***/
        /*** Check if these byte are present                          ***/

        if(TmpByte == 0x0f)
        {
            /*** Skip next 5 bytes with redundant information  ***/
            skip_bytes (Segment,5);
            data_type = get_byte(Segment);
            processed_length += 6;
        }
        else
        {
            data_type = TmpByte;
        }

        switch (data_type)
        {
            case 0x10 :
            {
                do
                {
                    decode_2_pel_code_string(Segment, &ColorEntry,
                                                &NumPixel,&processed_length);

                    if (NumPixel != 0)
                    {
                        TmpRefReg = ReferingRegion_list;

                        while (TmpRefReg != NULL)
                        {
                            switch (TmpRefReg->region_depth)
                            {
                                case 2 :
                                    MappedColorEntry = ColorEntry; 
                                    break;

                                case 4 :
                                    MappedColorEntry = Map_Table->
                                            Two_to_4_bit_map_table[ColorEntry];
                                    break;

                                case 8 :
                                    MappedColorEntry = Map_Table->
                                            Two_to_8_bit_map_table[ColorEntry];
                                    break;

                                default :
                                    break;
                            }

                            PixelBuffer_WriteNPixels(TmpRefReg,
                                                     MappedColorEntry,
                                                     NumPixel,
                                                     non_modifying_colour_flag);

                            TmpRefReg = TmpRefReg->next_ReferingRegion_p;
                        }
                    }

                 } while(NumPixel != 0);

                 Read_next_2_bits(Segment, &processed_length, 1);

                TmpRefReg = ReferingRegion_list;

		if(!CheckLenLine(DecoderHandle,TmpRefReg,NumLine,Field))  
			SetBadLine(TmpRefReg,NumLine,Field);

                 break;
            }

            case 0x11 :
            {
                do
                {
                    decode_4_pel_code_string(Segment, &ColorEntry,
                                                &NumPixel,&processed_length);

                    if (NumPixel != 0)
                    {

                        TmpRefReg = ReferingRegion_list;
                        while (TmpRefReg != NULL)
                        {

                            switch (TmpRefReg->region_depth)
                            {
                                case 2 :
                                    MappedColorEntry=((ColorEntry & 0x80) >>6) |
                                              ((ColorEntry & 0x70) != 0) ;
                                    break;

                                case 4 :
                                    MappedColorEntry = ColorEntry;
                                    break;

                                case 8 :
                                    MappedColorEntry = Map_Table->
                                            Four_to_8_bit_map_table[ColorEntry];
                                    break;

                                default :
                                    break;
                            }    

                            PixelBuffer_WriteNPixels(TmpRefReg,
                                                     MappedColorEntry,
                                                     NumPixel,
                                                     non_modifying_colour_flag);
                            TmpRefReg = TmpRefReg->next_ReferingRegion_p; 
                        }
                    }

                 } while(NumPixel != 0);
                 Read_next_4_bits(Segment, &processed_length,1);

                TmpRefReg = ReferingRegion_list;

		if(!CheckLenLine(DecoderHandle,TmpRefReg,NumLine,Field))  
			SetBadLine(TmpRefReg,NumLine,Field);

                break;
            }

            case 0x12 :
            {
                do
		{
             
                    decode_8_pel_code_string(Segment, &ColorEntry,
                                             &NumPixel,&processed_length);

                    if (NumPixel != 0)
                    {
                        TmpRefReg = ReferingRegion_list;

                        while (TmpRefReg != NULL)
                        {
                            switch (TmpRefReg->region_depth)
                            {
                                case 2 :
                                    MappedColorEntry=((ColorEntry & 0x80) >>6) |
                                              ((ColorEntry & 0x70) != 0) ;
                                    break;

                                case 4 :
                                    MappedColorEntry=((ColorEntry & 0xf0) >> 4);
                                    break;

                                case 8 :
                                    MappedColorEntry = ColorEntry ;
                                    break;

                                default :
                                    break;
                            }

                            PixelBuffer_WriteNPixels(TmpRefReg,
                                                     MappedColorEntry,
                                                     NumPixel,
                                                     non_modifying_colour_flag);
													

                            TmpRefReg = TmpRefReg->next_ReferingRegion_p;
                        }
                    }

                } while (NumPixel != 0);
            
                TmpRefReg = ReferingRegion_list;

		if(!CheckLenLine(DecoderHandle,TmpRefReg,NumLine,Field))  
			SetBadLine(TmpRefReg,NumLine,Field);
                break; 
            }

            case 0x20 :
            {
                for (cont = 0; cont < 4; cont++)
                {
                    (Map_Table->Two_to_4_bit_map_table)[cont] = 
                           Read_next_4_bits(Segment, &processed_length,0);

                    STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: 2to4maptable[%d]= %d **\n",
                               cont,(Map_Table->Two_to_4_bit_map_table)[cont]));

                }
                break;
            }

            case 0x21 :
            {
                for (cont = 0; cont < 4; cont++)
                {
                    (Map_Table->Two_to_8_bit_map_table)[cont] =
                                                        get_byte(Segment);
                   processed_length += 1;

                    STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: 2to8maptable[%d]= %d **\n",
                               cont,(Map_Table->Two_to_8_bit_map_table)[cont]));

                }
                break;
            }

            case 0x22 :
            {
                for (cont = 0; cont < 16; cont++)
                {
                    (Map_Table->Four_to_8_bit_map_table)[cont] =
                                                        get_byte(Segment);
                    processed_length += 1;

                    STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: 4to8maptable[%d]= %d **\n",
                              cont,(Map_Table->Four_to_8_bit_map_table)[cont]));

                }
                break;
            }

            case 0xf0 :
            {

                TmpRefReg = ReferingRegion_list;

		if(!CheckLenLine(DecoderHandle,TmpRefReg,NumLine,Field))
			SetBadLine(TmpRefReg,NumLine,Field);

                NumLine += 2;

                while (TmpRefReg != NULL)
                {


                    TmpRefReg->ObjLookahead_p = TmpRefReg->region_pixel_p +
                                          ((TmpRefReg->region_width *
                                            (TmpRefReg->ObjVertPos + NumLine)) +
                                           TmpRefReg->ObjHorizPos) /
                                                     TmpRefReg->NumPixelperByte;

                    TmpRefReg->ObjOffset = (((TmpRefReg->region_width *
                                            (TmpRefReg->ObjVertPos + NumLine)) +
                                            TmpRefReg->ObjHorizPos) % 
                                            TmpRefReg->NumPixelperByte) *
                                            TmpRefReg->region_depth;

                    TmpRefReg = TmpRefReg->next_ReferingRegion_p;
                }

                STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: End of line %d reached **\n",
                           NumLine));
                break;
            }

            default :
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** obj_processor: data type reserved **\n"));
                if (STEVT_Notify(
                          DecoderHandle->stevt_handle,
                          DecoderHandle->stevt_event_table
                                             [STSUBT_EVENT_BAD_SEGMENT & 0x0000FFFF],
                          DecoderHandle))
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** obj_processor: Error notifying event**\n"));
                }
                break;
            }
        }

        TmpRefReg = ReferingRegion_list;

        while (TmpRefReg != NULL)
        {
            RegBitmapHead = (STSUBT_Bitmap_t *) ((TmpRefReg->region_pixel_p) - 
                                                 BITMAP_HEADER_SIZE);
            RegBitmapHead->UpdateFlag = TRUE;

            TmpRefReg = TmpRefReg->next_ReferingRegion_p;
        }

    }
}



/******************************************************************************\
 * Function: object_processor
 *
 * Purpose : Process a single object segment.
 *
 * Description: First it check the state of the decoder.
 *              If the state is not starting the obj processor scan each
 *              RCS object list to check which region( and how many time) refer
 *              the current object. If the object is referenced at least one
 *              time, two lists of refering region are created.
 *              Two lists are used because the pixel data for a frame is 
 *              divided in data for top field and data for bottom field.
 *              Then pixel data is decoded and written in pixel buffer of
 *              region present in the lists
 *
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t object_processor (STSUBT_InternalHandle_t *DecoderHandle,
                                 Segment_t *Segment)
{
    U8    TmpByte;
    U8    region_depth;
    U8    NumPixelperByte;
    U8    object_version_number;
    U8    object_coding_method ;
    U8    non_modifying_colour_flag ;
    U8   *Ds_Free_p;
    U8    TopFieldObjOffset;
    U8   *TopFieldObjLookahead_p;
    U8    BottomFieldObjOffset;
    U8   *BottomFieldObjLookahead_p;
    U8   *region_pixel_p;

    U16   object_id;
    U16   segment_length;
    U16   region_width;
    U16   region_height;
    U16   TopField_region_height;
    U16   BottomField_region_height;
    U16   TopField_ObjVertPos;
    U16   BottomField_ObjVertPos;
    U16   ObjHorizPos;
    U16   ObjVertPos;
    U16   top_field_data_block; 
    U16   bottom_field_data_block; 

    U32   Ds_AllocatedBuffer;

    Map_Table_t  Map_Table = {{0x00,0x07,0x08,0x0f} , {0x00,0x77,0x88,0xff} ,
                              {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
                               0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff}} ;

    RCS_t             *ScanReg_p = NULL;
    Object_t          *ScanObj_p = NULL;
    ReferingRegion_t  *TopReferingRegion_list = NULL;
    ReferingRegion_t  *BottomReferingRegion_list = NULL;
    ReferingRegion_t  *TopTmpRefReg = NULL;
    ReferingRegion_t  *BottomTmpRefReg = NULL;

    CompBuffer_t   *CurrentCompBuf = DecoderHandle->CompositionBuffer; 

    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** obj_processor: Start processing OBJECT **\n"));


    /*** Verify the state of the decoder ***/ 

    if (DecoderHandle->AcquisitionMode == STSUBT_Starting)
    {
        /*** Decoder is starting skip the segment ***/
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** obj_processor: skip segment decoder is starting **\n"));
        return ST_NO_ERROR;
    }


    /*** Skip first 4 bytes of segment with redundant information  ***/ 

    skip_bytes (Segment,4);

    segment_length = ((U16)get_byte(Segment) << 8) + get_byte(Segment) + 6 ;

    /* --- check segment length --- */

    if (segment_length != Segment->Size)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** obj_processor: bad segment **\n"));
        if (STEVT_Notify(
            DecoderHandle->stevt_handle,
            DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_SEGMENT & 0x0000FFFF],
            DecoderHandle))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** obj_processor: Error notifying event**\n"));
        }
        return (STSUBT_ERROR_BAD_SEGMENT);
    }

    object_id = ((U16)get_byte(Segment) << 8) + get_byte(Segment);

    TmpByte = get_byte(Segment);
    object_version_number = TmpByte >> 4;
    object_coding_method = ((TmpByte & 0x0c) >> 2);
    non_modifying_colour_flag = ((TmpByte & 0x02) >> 1);

    top_field_data_block = ((U16)get_byte(Segment) << 8) + get_byte(Segment);

    bottom_field_data_block = ((U16)get_byte(Segment) << 8) + get_byte(Segment);


    STOP_PROCESSING_IF_REQUIRED(DecoderHandle);

    STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: seg_length=%d Object_id=%d Version=%d *\n",
               segment_length,object_id,object_version_number));


    /*** Scan the RCS object list to check which region refer ***/
    /*** the current object .If the object is present insert  ***/
    /*** region in the refering region list                   ***/

    ScanReg_p = CurrentCompBuf->EpochRegionList_p;

    CompBuffer_GetInfo(CurrentCompBuf, &Ds_AllocatedBuffer, &Ds_Free_p);

    while(ScanReg_p != NULL)
    {
        ScanObj_p = ScanReg_p->object_list_p;

        while(ScanObj_p != NULL)
        {
            if(ScanObj_p->object_id == object_id)
            {

		
                region_depth = ScanReg_p->region_depth;

                NumPixelperByte = 8 / region_depth;

                region_width = ScanReg_p->region_width;

                region_height = ScanReg_p->region_height;

                TopField_region_height = ((ScanReg_p->region_height) +1) / 2;

                BottomField_region_height = (ScanReg_p->region_height) / 2;

                region_pixel_p = ScanReg_p->region_pixel_p + BITMAP_HEADER_SIZE;

                ObjVertPos = ScanObj_p->vertical_position;

                TopField_ObjVertPos =ObjVertPos + (((ObjVertPos + 1) % 2) == 0);

                BottomField_ObjVertPos =ObjVertPos + ((ObjVertPos % 2) == 0);

                ObjHorizPos = ScanObj_p->horizontal_position;



                TopFieldObjLookahead_p = region_pixel_p +
                                        (((region_width * TopField_ObjVertPos) +
                                          ObjHorizPos) / NumPixelperByte);

                TopFieldObjOffset =(((region_width * TopField_ObjVertPos) +
                                 ObjHorizPos) % NumPixelperByte) * region_depth;

                BottomFieldObjLookahead_p = region_pixel_p +
                                    (((region_width * BottomField_ObjVertPos) +
                                          ObjHorizPos) / NumPixelperByte);

                BottomFieldObjOffset=(((region_width * BottomField_ObjVertPos) +
                                 ObjHorizPos) % NumPixelperByte) * region_depth;


                TopTmpRefReg = CompBuffer_NewReferingRegion(CurrentCompBuf,
                                                        region_depth,
                                                        NumPixelperByte,
                                                        region_width,
                                                        region_height,
                                                        region_pixel_p,
                                                        TopField_ObjVertPos,
                                                        ObjHorizPos,
                                                        TopFieldObjLookahead_p,
                                                        TopFieldObjOffset);

                BottomTmpRefReg = CompBuffer_NewReferingRegion(CurrentCompBuf,
                                                     region_depth,
                                                     NumPixelperByte,
                                                     region_width,
                                                     region_height,
                                                     region_pixel_p,
                                                     BottomField_ObjVertPos,
                                                     ObjHorizPos,
                                                     BottomFieldObjLookahead_p,
                                                     BottomFieldObjOffset);

                if ((TopTmpRefReg == NULL) || (BottomTmpRefReg == NULL))
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** obj_processor: Composition Buffer FULL\n"));
                    if (STEVT_Notify(
                              DecoderHandle->stevt_handle,
                              DecoderHandle->stevt_event_table
                                           [STSUBT_EVENT_COMP_BUFFER_NO_SPACE & 0x0000FFFF],
                              DecoderHandle))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"**obj_processor: Error notifying event\n"));
                    }
                    continue;
                }

                if (((ScanObj_p->vertical_position) % 2) == 0)
                {
                   TopTmpRefReg->next_ReferingRegion_p = TopReferingRegion_list;
                   TopReferingRegion_list = TopTmpRefReg;
                   if (bottom_field_data_block == 0)
                   {
                       BottomTmpRefReg->next_ReferingRegion_p =
                                                         TopReferingRegion_list;

                       TopReferingRegion_list = BottomTmpRefReg;
                   }
                   else
                   {
                       BottomTmpRefReg->next_ReferingRegion_p =
                                                      BottomReferingRegion_list;

                       BottomReferingRegion_list = BottomTmpRefReg;
                   }
                }
                else
                {
                   BottomTmpRefReg->next_ReferingRegion_p =
                                                         TopReferingRegion_list;
                   TopReferingRegion_list = BottomTmpRefReg;
                   if (bottom_field_data_block == 0)
                   {
                       TopTmpRefReg->next_ReferingRegion_p =
                                                         TopReferingRegion_list;

                       TopReferingRegion_list = TopTmpRefReg;
                   }
                   else
                   {
                       TopTmpRefReg->next_ReferingRegion_p =
                                                      BottomReferingRegion_list;

                       BottomReferingRegion_list = TopTmpRefReg;
                   }
                }
		
             }
             ScanObj_p = ScanObj_p->next_object_p;
         }
         ScanReg_p = ScanReg_p->next_region_p;
    }
         
    STOP_PROCESSING_IF_REQUIRED(DecoderHandle);

    if(TopReferingRegion_list == NULL)
    {
        /*** The object is not referenced. Skip the Object ***/

        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** obj_processor: Object skipped not refereced **\n"));
        if (STEVT_Notify(
                  DecoderHandle->stevt_handle,
                  DecoderHandle->stevt_event_table
                                     [STSUBT_EVENT_UNKNOWN_OBJECT & 0x0000FFFF],
                  DecoderHandle))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** object_processor: Error notifying event**\n"));
        }
        return ST_NO_ERROR;
    }


#ifdef DEBUG_REPORTING

    TopTmpRefReg = TopReferingRegion_list;
    while(TopTmpRefReg != NULL)
    {

        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: Top : Object %d reg_depth %d = %d  **\n",
                   object_id,
                   TopTmpRefReg->region_depth,
                   TopTmpRefReg->NumPixelperByte));
        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: Top : reg_width %d reg_height %d **\n",
                   TopTmpRefReg->region_width,
                   TopTmpRefReg->region_height));
        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: Top : ObjVertPos %d ObjHorizPos %d **\n",
                   TopTmpRefReg->ObjVertPos,
                   TopTmpRefReg->ObjHorizPos));

        TopTmpRefReg = TopTmpRefReg->next_ReferingRegion_p;
    }

    BottomTmpRefReg = BottomReferingRegion_list;
    while(BottomTmpRefReg != NULL)
    {

        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: Bottom : Object %d reg_depth %d = %d**\n",
                   object_id,
                   BottomTmpRefReg->region_depth,
                   BottomTmpRefReg->NumPixelperByte));
        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: Bottom : reg_width %d reg_height %d **\n",
                   BottomTmpRefReg->region_width,
                   BottomTmpRefReg->region_height));
        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** obj_processor: Bottom : ObjVertPos %d ObjHorPos %d **\n",
                   BottomTmpRefReg->ObjVertPos,
                   BottomTmpRefReg->ObjHorizPos));

        BottomTmpRefReg = BottomTmpRefReg->next_ReferingRegion_p;
    }

#endif



    if (object_coding_method != 0x00)
    {
        /*** The (object is coded as a string of characters ***/
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** obj_processor: OBJECT is a string of characters **\n"));

        if (STEVT_Notify(
                  DecoderHandle->stevt_handle,
                  DecoderHandle->stevt_event_table
                                    [STSUBT_EVENT_NOT_SUPPORTED_OBJECT_CODING & 0x0000FFFF],
                  DecoderHandle))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** object_processor: Error notifying event**\n"));
        }
        return ST_NO_ERROR;
    }

    Field_data_decoder (DecoderHandle, Segment, TopReferingRegion_list,
                        top_field_data_block, &Map_Table,
                        non_modifying_colour_flag,FIELD_0);

    if (bottom_field_data_block != 0)
    {
        Field_data_decoder (DecoderHandle, Segment, BottomReferingRegion_list,
                            bottom_field_data_block, &Map_Table,
                            non_modifying_colour_flag,FIELD_1);
    }


    CompBuffer_Restore(CurrentCompBuf, Ds_AllocatedBuffer, Ds_Free_p);
   
    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** obj_processor: OBJECT decoded **\n"));

    return ST_NO_ERROR;
}



/*************************************************************************\
 *
 * Function: CheckLenLine
 *
 * Purpose : Check if lenght of line is bad
 *
 * Description: Copy the lenght of first tree line and if  lenght 
 *              of lines foward is not egual return FALSE. 
 *             
 *
 * Comments: 
 *
\**************************************************************************/

static __inline 
BOOL CheckLenLine(STSUBT_InternalHandle_t *DecoderHandle,ReferingRegion_t *Reg, U16 NumberLine,U8 Field)
{
static U16 WidthLine[3];
static U16 ObjWidth;


	if(DecoderHandle->RecoveryByte!=STSUBT_Recovery)
		return (TRUE); /* No Euristic Recovery */

	if(Field == FIELD_0)
	{
	        switch(NumberLine)
        	{
                	case 0:
                       		WidthLine[0]=Reg->ObjLookahead_p-
                                        Reg->region_pixel_p;

				return (TRUE);  
                        	break;

                	case 2:
                                WidthLine[1]=Reg->ObjLookahead_p-
                                        Reg->region_pixel_p-
                                        Reg->region_width;

				return (TRUE);  
                        	break;

                	case 4:
                        	WidthLine[2]=Reg->ObjLookahead_p-
                                	Reg->region_pixel_p-
                                	(Reg->region_width*2);

				STTBX_Report((STTBX_REPORT_LEVEL_USER1,"RecoveryByte %d, %d %d %d ",
					DecoderHandle->RecoveryByte,
						WidthLine[0],
						WidthLine[1],
						WidthLine[2]));
 				Reg->ObjWidth=0;

				if(WidthLine[0]==WidthLine[1]) 
				{
					Reg->ObjWidth=WidthLine[0];
				}
				else if(WidthLine[1]==WidthLine[2])
	        		{
					Reg->ObjWidth=WidthLine[1];
				}
				else if(WidthLine[0]==WidthLine[2])
	        		{
 					Reg->ObjWidth=WidthLine[0];
				}

				ObjWidth=Reg->ObjWidth;

				return (TRUE);  

                        	break;
        	}
	}
	else
	{
		Reg->ObjWidth=ObjWidth;
	}


	if(((Reg->ObjLookahead_p-Reg->region_pixel_p)-
                (Reg->region_width*(NumberLine+Field)/Reg->NumPixelperByte)
		!= ObjWidth) && (ObjWidth>0))
	{
		STTBX_Report((STTBX_REPORT_LEVEL_USER1,"Line %d is bad [%d], should be %d\n",
		NumberLine, 
		((Reg->ObjLookahead_p-Reg->region_pixel_p)-
		(Reg->region_width*(NumberLine+Field)/Reg->NumPixelperByte)),
		ObjWidth));

		return (FALSE);  /* The current line is bad */
	}	
	else 
	     /* The current line is ok, or Reg->ObjWidth is invalid.   */

	{
		return (TRUE);  
	}	

}


/*************************************************************************\
 *
 * Function: SetBadLine
 *
 * Purpose : Overwrite a bad pixel line
 *
 * Description: If the bad line is in the field_1 overwrite (NumberLine) with 
 *              a line to field0, else copy the (NumberLine-1) from field_0
 *
 * Comments: 
 *
\**************************************************************************/

static __inline 
void SetBadLine(ReferingRegion_t *Reg, U16 NumberLine,U8 Field)
{
U16 RegionWidth;

	/* Field=0 (even) Field=1 (odd) */ 

	NumberLine+=Field; /* if field is odd NumberLine=NumberLine+1; */

	RegionWidth=Reg->region_width/Reg->NumPixelperByte;

	/* copy RegionWidth pixel from NumberLine-(2-Field) to NumberLine */

	memcpy( Reg->region_pixel_p + (RegionWidth * (NumberLine)),
		Reg->region_pixel_p + (RegionWidth * (NumberLine-(2 - Field))),
		RegionWidth);

}

