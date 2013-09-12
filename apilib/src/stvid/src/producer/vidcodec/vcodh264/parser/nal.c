/*!
 *******************************************************************************
 * \file nal.c
 *
 * \brief H264 NAL unit bitstream extraction functions
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
#endif

/* H264 Parser specific */
#include "h264parser.h"
/* Functions ---------------------------------------------------------------- */

/******************************************************************************/
/*! \brief Moves the NALByteStream.ByteOffset to the next byte in the bytestream.
 *
 * NALByteStream.ByteOffset is modified according to the endianness of the byte stream
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_MoveToNextByteFromNALUnit(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#ifdef BYTE_SWAP_SUPPORT
    if (PARSER_Data_p->NALByteStream.WordSwapFlag == FALSE)
    {
#endif /*BYTE_SWAP_SUPPORT*/

        /* Bytes are just stored in successive addresses in memory */
        PARSER_Data_p->NALByteStream.ByteOffset ++;

        #if defined(DVD_SECURED_CHIP)
            /* Adjust with ES Copy buffer boundaries */
            if (((U32)PARSER_Data_p->NALByteStream.RBSPStartByte_p + PARSER_Data_p->NALByteStream.ByteOffset) > (U32)PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p)
            {
                PARSER_Data_p->NALByteStream.RBSPStartByte_p = PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p;
                PARSER_Data_p->NALByteStream.ByteOffset = 0;
            }
        #else
            /* Adjust with Bit buffer boundaries */
            if (((U32)PARSER_Data_p->NALByteStream.RBSPStartByte_p + PARSER_Data_p->NALByteStream.ByteOffset) > (U32)PARSER_Data_p->BitBuffer.ES_Stop_p)
            {
                PARSER_Data_p->NALByteStream.RBSPStartByte_p = PARSER_Data_p->BitBuffer.ES_Start_p;
                PARSER_Data_p->NALByteStream.ByteOffset = 0;
            }
        #endif /* DVD_SECURED_CHIP */
#ifdef BYTE_SWAP_SUPPORT
    }
    else
    {
        /* Word swapping occurs */
        if ((PARSER_Data_p->NALByteStream.ByteOffset & 0x3) == 0)
        {
            /* if current ByteOffset is aligned on a word boundary, jump to next word */
            PARSER_Data_p->NALByteStream.ByteOffset += 7;

            #if defined(DVD_SECURED_CHIP)
                /* Adjust with ES Copy buffer boundaries */
                if (((U32)PARSER_Data_p->NALByteStream.RBSPStartByte_p + PARSER_Data_p->NALByteStream.ByteOffset) > (U32)PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p)
                {
                    PARSER_Data_p->NALByteStream.RBSPStartByte_p = PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p;
                    PARSER_Data_p->NALByteStream.ByteOffset = 3;
                }
            #else
                /* Adjust with Bit buffer boundaries */
                if (((U32)PARSER_Data_p->NALByteStream.RBSPStartByte_p + PARSER_Data_p->NALByteStream.ByteOffset) > (U32)PARSER_Data_p->BitBuffer.ES_Stop_p)
                {
                    PARSER_Data_p->NALByteStream.RBSPStartByte_p = PARSER_Data_p->BitBuffer.ES_Start_p;
                    PARSER_Data_p->NALByteStream.ByteOffset = 3;
                }
            #endif /* DVD_SECURED_CHIP */
        }
        else
        {
            /* Else, keep in the same word (then decrement byte address) */
            PARSER_Data_p->NALByteStream.ByteOffset --;
        }
    }
#endif /*BYTE_SWAP_SUPPORT*/
    PARSER_Data_p->NALByteStream.ByteCounter++; /* One more byte read */
}
/******************************************************************************/
/*! \brief Returns the current byte in the bytestream.
 *         Discards emulation prevention bytes
 *         Exits if the byte to extract is not aligned
 *
 * NALByteStream.ByteOffset is incremented by two if current byte is an emulation prevention byte \n
 *                           is incremented by one otherwise \n
 * NALByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the extracted byte
 */
/******************************************************************************/
U8 h264par_GetByteFromNALUnit(const PARSER_Handle_t  ParserHandle)
{
    U8 NALByte; /* the returned value */
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    U8                                    * CPU_RBSPStartByte_p;

    /* Extract next byte in byte stream */
    h264par_MoveToNextByteFromNALUnit(ParserHandle);

    /* Translate "Device" address to "CPU" address */
    CPU_RBSPStartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->NALByteStream.RBSPStartByte_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    CPU_RBSPStartByte_p = STOS_MapPhysToCached(CPU_RBSPStartByte_p, 4);    /* we read 4 byte at most */

    /* Check if this byte is not a emulation prevention byte */
#ifdef BYTE_SWAP_SUPPORT
    if ( ( (PARSER_Data_p->NALByteStream.ByteSwapFlag == FALSE) &&
           (STSYS_ReadRegMemUncached8(&(CPU_RBSPStartByte_p[PARSER_Data_p->NALByteStream.ByteOffset])) == EMULATION_PREVENTION_BYTE)
         ) ||
         ( (PARSER_Data_p->NALByteStream.ByteSwapFlag == TRUE) &&
           (STSYS_ReadRegMemUncached8(&(CPU_RBSPStartByte_p[PARSER_Data_p->NALByteStream.ByteOffset])) == ETYB_NOITNEVERP_NOITALUME)
         )
       )
#else
    if ( STSYS_ReadRegMem8(&(CPU_RBSPStartByte_p[PARSER_Data_p->NALByteStream.ByteOffset])) == EMULATION_PREVENTION_BYTE )
#endif /*BYTE_SWAP_SUPPORT*/
    {
            if (PARSER_Data_p->NALByteStream.ByteOffset >= 2)
            {
                /* Check for pattern 0x00 0x00 0x03 */
                if ((PARSER_Data_p->NALByteStream.LastByte == 0) && /* previous byte */
                    (PARSER_Data_p->NALByteStream.LastButOneByte == 0))   /* and the byte before */
                {
                    /* The byte to extract is an emulation prevention byte as it is
                     * preceeded by two 0-byte.
                     * Let's get the next byte in the byte stream: this one (if any) is not
                     * an emulation prevention byte for sure!
                     */
                    h264par_MoveToNextByteFromNALUnit(ParserHandle);
                    /* PARSER_Data_p->NALByteStream.RBSPStartByte_p may have been changed by h264par_MoveToNextByteFromNALUnit() Translate "Device" address to "CPU" address */
                    CPU_RBSPStartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->NALByteStream.RBSPStartByte_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                    CPU_RBSPStartByte_p = STOS_MapPhysToCached(CPU_RBSPStartByte_p, 4);    /* we read 4 byte at most */
                    /* Reset the fifo not to get spurious emulation bytes */
                    PARSER_Data_p->NALByteStream.LastByte = 0xff;
                    PARSER_Data_p->NALByteStream.LastButOneByte = 0xff;
                }
        }
    }

    /* Check if the byte to extract is still inside the stream */
    if (PARSER_Data_p->NALByteStream.ByteCounter == PARSER_Data_p->NALByteStream.Len)
    {
        /* The byte to extract is out of the stream boundary */
        PARSER_Data_p->NALByteStream.EndOfStreamFlag = TRUE;
        PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
    }

    NALByte = STSYS_ReadRegMem8(&(CPU_RBSPStartByte_p[PARSER_Data_p->NALByteStream.ByteOffset]));
    /* Store last and last but one byte for further emulation prevention detection */
    PARSER_Data_p->NALByteStream.LastButOneByte = PARSER_Data_p->NALByteStream.LastByte;
    PARSER_Data_p->NALByteStream.LastByte = NALByte;

    return NALByte;

} /* end of GetByteFromNALUnit */

/******************************************************************************/
/*! \brief returns the current bit in the bytestream.
 *
 * NALByteStream.BitOffset is decremented by one and clipped to [7:-1] \n
 * NALByteStream.ByteOffset may be incremented by one or two if NALByteStream.BitOffset was -1 on entering \n
 * NALByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the extracted bit
 */
/******************************************************************************/
U8 h264par_GetBitFromNALUnit(const PARSER_Handle_t  ParserHandle)
{
    U8 NALByte;
    U8 NALBit;
    S8 BitOffset;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    U8   * CPU_RBSPStartByte_p;

    /* Check if new byte is required */
    if (PARSER_Data_p->NALByteStream.BitOffset == -1)
    {
        PARSER_Data_p->NALByteStream.BitOffset = 7;
        NALByte = h264par_GetByteFromNALUnit(ParserHandle);
    }
    else
    {
        /* the default byte where to extract the bit from is the current byte */
        /* Translate "Device" address to "CPU" address */
        CPU_RBSPStartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->NALByteStream.RBSPStartByte_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
        CPU_RBSPStartByte_p = STOS_MapPhysToCached(CPU_RBSPStartByte_p, 1);  /* we'll read just 1 byte */
        NALByte = STSYS_ReadRegMem8(&(CPU_RBSPStartByte_p[PARSER_Data_p->NALByteStream.ByteOffset]));
    }
    BitOffset = PARSER_Data_p->NALByteStream.BitOffset;
#ifdef BYTE_SWAP_SUPPORT
    if (PARSER_Data_p->NALByteStream.ByteSwapFlag == TRUE)
    {
        BitOffset = 7 - BitOffset;
    }
#endif /*BYTE_SWAP_SUPPORT*/

    if ((NALByte & (1 << BitOffset)) != 0)
    {
        NALBit =  1;
    }
    else
    {
        NALBit =  0;
    }
    PARSER_Data_p->NALByteStream.BitOffset --;
    return NALBit;
}

/******************************************************************************/
/*! \brief Return TRUE is there is more data in NAL unit
 *
 * NALByteStream.ByteOffset is incremented by two if current byte is an emulation prevention byte \n
 *                           is incremented by one otherwise \n
 * NALByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return TRUE/FALSE
 */
/******************************************************************************/
BOOL h264par_IsMoreRbspData(const PARSER_Handle_t  ParserHandle)
{
    NALByteStream_t SavedNALByteStream;  /* Save all NALByteStream pointers in
                                           order to get back after analysing following bytes */
    U8      NALBit;
    BOOL    IsMoreRbspData = FALSE;
    U8      NALByte1;
    U8      NALByte2;
    U8      NALByte3;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Save all NALByteStream pointers */
    SavedNALByteStream = PARSER_Data_p->NALByteStream;

   /* Look for rbsp_stop_one_bit which close any rbsp */
   /* (H264 standard G050r1d1 "7.4.1.1 Encapsulation of an SODB within an RBSP" */
    NALBit = h264par_GetUnsigned(1, ParserHandle);
    if(NALBit != 1)
    {   /* This is not a rbsp_stop_one_bit thus this is additionnal data */
        IsMoreRbspData = TRUE;
    }
    else
    {
        /* Check if there are other "Ones" before next start code */
        while(((PARSER_Data_p->NALByteStream.ByteCounter != PARSER_Data_p->NALByteStream.Len - 3) &&
               (PARSER_Data_p->NALByteStream.BitOffset != -1)) &&
              !IsMoreRbspData)
        { /* While not at aligned on 1 byte, look for non 0-bits */
            NALBit = h264par_GetUnsigned(1, ParserHandle);
            if(NALBit == 1)
            {
                IsMoreRbspData = TRUE;
            }
        }
        if (IsMoreRbspData == FALSE)
        {
            /* We are now byte aligned: look for patterns different from 0x00000N */
            /* With N = 0, 1 = Start code, 3 = emulation prevention byte */
            NALByte1 = h264par_GetUnsigned(8, ParserHandle);
            NALByte2 = h264par_GetUnsigned(8, ParserHandle);
            NALByte3 = h264par_GetUnsigned(8, ParserHandle);
            if ((NALByte1 != 0) ||
                (NALByte2 != 0) ||
                ((NALByte3 != 0) && (NALByte3 != 1) && (NALByte3 != 3))
               )
            {
                IsMoreRbspData = TRUE;
            }
        }
    }
    /* Restore all NALByteStream pointers in order to process remaining rbsp_data */
    PARSER_Data_p->NALByteStream = SavedNALByteStream;
    return(IsMoreRbspData);
}

/******************************************************************************/
/*! \brief Returns a Len- bit integer read from the byte stream (MSB first)
 *
 * NALByteStream.BitOffset is modified \n
 * NALByteStream.ByteOffset is modified \n
 * NALByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte \n
 * \n
 * Info: H264 refers to this function as "u(n)" or "f(n)" or "b(n)"
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param Len the length in bits of the integer to extract
 * \return the integer extracted from the byte stream
 */
/******************************************************************************/
U32 h264par_GetUnsigned(U8 Len, const PARSER_Handle_t  ParserHandle)
{
    U32 UnsInt;
    U8 NALByte;
    U8 NALBit;
    S8 BitOffset;

    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;
    U8   * CPU_RBSPStartByte_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    CPU_RBSPStartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->NALByteStream.RBSPStartByte_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    CPU_RBSPStartByte_p = STOS_MapPhysToCached(CPU_RBSPStartByte_p, 4);    /* we'll read at most 4 bytes */

    UnsInt = 0;
    while (Len)
    {
        /* Assumption: read MSBits first */
        UnsInt <<= 1;

        /* Check if new byte is required */
        if (PARSER_Data_p->NALByteStream.BitOffset == -1)
        {
            PARSER_Data_p->NALByteStream.BitOffset = 7;
            NALByte = h264par_GetByteFromNALUnit(ParserHandle);
            /* PARSER_Data_p->NALByteStream.RBSPStartByte_p may have been changed by h264par_GetByteFromNALUnit() Translate "Device" address to "CPU" address */
            CPU_RBSPStartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->NALByteStream.RBSPStartByte_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
            CPU_RBSPStartByte_p = STOS_MapPhysToCached(CPU_RBSPStartByte_p, 4);    /* we read 4 byte at most */
        }
        else
        {
            /* the default byte where to extract the bit from is the current byte */
            NALByte = STSYS_ReadRegMem8(&(CPU_RBSPStartByte_p[PARSER_Data_p->NALByteStream.ByteOffset]));
        }

    BitOffset = PARSER_Data_p->NALByteStream.BitOffset;
#ifdef BYTE_SWAP_SUPPORT
    if (PARSER_Data_p->NALByteStream.ByteSwapFlag == TRUE)
    {
        BitOffset = 7 - BitOffset;
    }
#endif /*BYTE_SWAP_SUPPORT*/

    if ((NALByte & (1 << BitOffset)) != 0)
        {
            NALBit =  1;
        }
        else
        {
            NALBit =  0;
        }
        PARSER_Data_p->NALByteStream.BitOffset --;


        UnsInt += NALBit;
        Len--;
    }
    return UnsInt;
}

/******************************************************************************/
/*! \brief returns a Len- bit signed integer read from the byte stream (MSB first)
 *
 * NALByteStream.BitOffset is modified \n
 * NALByteStream.ByteOffset is modified \n
 * NALByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte \n
 * \n
 * Info: H264 refers to this function as "i(n)"
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param Len the length in bits of the signed integer to extract
 * \return the integer extracted from the byte stream
 */
/******************************************************************************/
S32 h264par_GetSigned(U8 Len, const PARSER_Handle_t  ParserHandle)
{
    S32 SiInt;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Len >= 2 is assumed: 1 bit for sign, 1 bit (minimum) for value */
    /* First bit in stream is the sign */
    /* if first bit = 1: negative number is expected: initialize to -1 (to shift 111 left later on) */
    /* if first bit = 0: positive number is expected: intialize to 0 */
    SiInt = - h264par_GetBitFromNALUnit(ParserHandle);

    Len--;

    while (Len)
    {
        /* Assumption: read MSBits first */
        SiInt <<= 1;
        SiInt += h264par_GetBitFromNALUnit(ParserHandle);
        Len--;
    }

    return SiInt;
}

/******************************************************************************/
/*! \brief returns a Exp-Golomb unsigned integer read from the byte stream.
 *
 * NALByteStream.BitOffset is modified \n
 * NALByteStream.ByteOffset is modified \n
 * NALByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte \n
 * \n
 * Info: H264 refers to this function as "ue(v)"
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the Exp-Golomb decoded unsigned integer
 */
/******************************************************************************/
U32 h264par_GetUnsignedExpGolomb(const PARSER_Handle_t  ParserHandle)
{
    S8 LeadingZerosLoop;
    U32 UnsExpGolomb;
    U8 NALBit;
    U32 UnsInt;
    U8 NALByte;
    S8 BitOffset;
    U8 * CPU_RBSPStartByte_p;


    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    CPU_RBSPStartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->NALByteStream.RBSPStartByte_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    CPU_RBSPStartByte_p = STOS_MapPhysToCached(CPU_RBSPStartByte_p, 4);    /* we'll read at most 4 bytes */
    LeadingZerosLoop = -1;

    /* first, look at the leading zeros and count them */
    do
    {
        NALBit = h264par_GetBitFromNALUnit(ParserHandle);
        if (PARSER_Data_p->NALByteStream.EndOfStreamFlag == TRUE)
        {
            /* The end of stream is reached: the loop is stopped */
            /* This is to prevent infinite search on "0-bit" area */
            break;
        }
        LeadingZerosLoop++;
    } while (NALBit == 0);
    /* Then extract an unsigned integer (length =  "LeadingZerosLoop" bits) */
    if (!PARSER_Data_p->NALByteStream.EndOfStreamFlag)  /* Not at end of stream */
    {
       UnsExpGolomb = 1 << LeadingZerosLoop;
       UnsExpGolomb--;

       UnsInt = 0;
       while (LeadingZerosLoop)
       {
            /* Assumption: read MSBits first */
            UnsInt <<= 1;

            /* Check if new byte is required */
            if (PARSER_Data_p->NALByteStream.BitOffset == -1)
            {
                PARSER_Data_p->NALByteStream.BitOffset = 7;
                NALByte = h264par_GetByteFromNALUnit(ParserHandle);
                /* PARSER_Data_p->NALByteStream.RBSPStartByte_p may have been changed by h264par_GetByteFromNALUnit() Translate "Device" address to "CPU" address */
                CPU_RBSPStartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->NALByteStream.RBSPStartByte_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                CPU_RBSPStartByte_p = STOS_MapPhysToCached(CPU_RBSPStartByte_p, 4);    /* we read 4 byte at most */
            }
            else
            {
                /* the default byte where to extract the bit from is the current byte */
                NALByte = STSYS_ReadRegMem8(&(CPU_RBSPStartByte_p[PARSER_Data_p->NALByteStream.ByteOffset]));
            }

    		BitOffset = PARSER_Data_p->NALByteStream.BitOffset;
#ifdef BYTE_SWAP_SUPPORT
	    	if (PARSER_Data_p->NALByteStream.ByteSwapFlag == TRUE)
    		{
        		BitOffset = 7 - BitOffset;
    		}
#endif /*BYTE_SWAP_SUPPORT*/

    		if ((NALByte & (1 << BitOffset)) != 0)
            {
                NALBit =  1;
            }
            else
            {
                NALBit =  0;
            }
            PARSER_Data_p->NALByteStream.BitOffset --;

            UnsInt += NALBit;
            LeadingZerosLoop--;
        }
        UnsExpGolomb += UnsInt;
    }
    else
    {
        UnsExpGolomb = UNSIGNED_EXPGOLOMB_FAILED; /* end of stream reached: return a dummy value */
    }
    return UnsExpGolomb;
}

/******************************************************************************/
/*! \brief returns a Exp-Golomb signed integer read from the byte stream.
 *
 * NALByteStream.BitOffset is modified \n
 * NALByteStream.ByteOffset is modified \n
 * NALByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte \n
 * \n
 * Info: H264 refers to this function as "se(v)"
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the Exp-golomb decoded signed integer
 */
/******************************************************************************/
S32 h264par_GetSignedExpGolomb(const PARSER_Handle_t  ParserHandle)
{
    U32 UnsExpGolomb;
    S32 SiExpGolomb;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    /* First, extract an Unsigned ExpGolomb integer */
    UnsExpGolomb = h264par_GetUnsignedExpGolomb(ParserHandle);

    /* Then, map it onto a signed integer */
    /* the formula is: k being the result of GetUnsignedExpGolomb: (-1)^(k+1) * Ceil (k/2) */
    /* with ceil(x) is the smallest integer greater or equal x */

    /* And the implementation is: */
    SiExpGolomb = (UnsExpGolomb + 1) >> 1;
    if ((UnsExpGolomb & 0x1) == 0)
    {
        SiExpGolomb = - SiExpGolomb;
    }
    return SiExpGolomb;
}

/******************************************************************************/
/*! \brief Saves the read pointer of the NAL parsing
 *  This is used to extract usefull data before the current position to allow
 *  some conditionnal parese (like SEI picture timing for instance)
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param CurrentPositionInNAL (pointer supplied by the caller to store the current position)
 * \return void
 */
/******************************************************************************/
void h264par_SavePositionInNAL(const PARSER_Handle_t ParserHandle, NALByteStream_t * const CurrentPositionInNAL)
{
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    CurrentPositionInNAL->RBSPStartByte_p  = PARSER_Data_p->NALByteStream.RBSPStartByte_p;
    CurrentPositionInNAL->Len              = PARSER_Data_p->NALByteStream.Len;
    CurrentPositionInNAL->BitOffset        = PARSER_Data_p->NALByteStream.BitOffset;
    CurrentPositionInNAL->ByteOffset       = PARSER_Data_p->NALByteStream.ByteOffset;
    CurrentPositionInNAL->ByteCounter      = PARSER_Data_p->NALByteStream.ByteCounter;
    CurrentPositionInNAL->EndOfStreamFlag  = PARSER_Data_p->NALByteStream.EndOfStreamFlag;
    CurrentPositionInNAL->LastByte         = PARSER_Data_p->NALByteStream.LastByte;
    CurrentPositionInNAL->LastButOneByte   = PARSER_Data_p->NALByteStream.LastButOneByte;
#ifdef BYTE_SWAP_SUPPORT
    CurrentPositionInNAL->ByteSwapFlag     = PARSER_Data_p->NALByteStream.ByteSwapFlag;
    CurrentPositionInNAL->WordSwapFlag     = PARSER_Data_p->NALByteStream.WordSwapFlag;
#endif /*BYTE_SWAP_SUPPORT*/
}

/******************************************************************************/
/*! \brief Restores the read pointer of the NAL parsing
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param CurrentPositionInNAL (pointer supplied by the caller to restore the current position)
 * \return void
 */
/******************************************************************************/
void h264par_RestorePositionInNAL(const PARSER_Handle_t ParserHandle, const NALByteStream_t CurrentPositionInNAL)
{
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->NALByteStream.RBSPStartByte_p = CurrentPositionInNAL.RBSPStartByte_p;
    PARSER_Data_p->NALByteStream.Len             = CurrentPositionInNAL.Len;
    PARSER_Data_p->NALByteStream.BitOffset       = CurrentPositionInNAL.BitOffset;
    PARSER_Data_p->NALByteStream.ByteOffset      = CurrentPositionInNAL.ByteOffset;
    PARSER_Data_p->NALByteStream.ByteCounter     = CurrentPositionInNAL.ByteCounter;
    PARSER_Data_p->NALByteStream.EndOfStreamFlag = CurrentPositionInNAL.EndOfStreamFlag;
    PARSER_Data_p->NALByteStream.LastByte        = CurrentPositionInNAL.LastByte;
    PARSER_Data_p->NALByteStream.LastButOneByte  = CurrentPositionInNAL.LastButOneByte;
#ifdef BYTE_SWAP_SUPPORT
    PARSER_Data_p->NALByteStream.ByteSwapFlag    = CurrentPositionInNAL.ByteSwapFlag;
    PARSER_Data_p->NALByteStream.WordSwapFlag    = CurrentPositionInNAL.WordSwapFlag;
#endif /*BYTE_SWAP_SUPPORT*/
}


/******************************************************************************/
/*! \brief Pushes the read pointer of the NAL parsing in the SEIList fifo
 *    The current NALByteStream position is pushed at the end of SEIPositionList
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_PushSEIListItem(const PARSER_Handle_t ParserHandle)
{
    H264ParserPrivateData_t * PARSER_Data_p;
    U32 i;
    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    i=PARSER_Data_p->SEIPositionArray.Iterator;

    if (i < MAX_SEI_NUMBER )
    {
        STOS_memcpy(&(PARSER_Data_p->SEIPositionArray.NALByteStream[i]), &(PARSER_Data_p->NALByteStream), sizeof(NALByteStream_t));
        PARSER_Data_p->SEIPositionArray.Iterator++;
        PARSER_Data_p->PictureLocalData.HasSEIMessage = TRUE;
    }
    else
    {
        /* TODO: too many SEI messages parsed before the slice NAL: report pb */
    }


}


/******************************************************************************/
/*! \brief Restores the read pointer of the NAL parsing from the SEIlist fifo
 *            NALByteStream is retrieved from the first item in the list
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the new PARSER_Data_p->NALByteStream, or NULL if the list is empty
 */
/******************************************************************************/
NALByteStream_t* h264par_GetSEIListItem(const PARSER_Handle_t ParserHandle)
{
    H264ParserPrivateData_t * PARSER_Data_p;
    U32 i;
    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if(PARSER_Data_p->SEIPositionArray.Iterator == 0)
    {
        /* empty list */
        return NULL;
    }
    PARSER_Data_p->SEIPositionArray.Iterator--;
    i = PARSER_Data_p->SEIPositionArray.Iterator;

    STOS_memcpy(&(PARSER_Data_p->NALByteStream), &(PARSER_Data_p->SEIPositionArray.NALByteStream[i]), sizeof(NALByteStream_t));


    return &(PARSER_Data_p->NALByteStream);
}

/******************************************************************************/
/*! \brief Pushes the read pointer of the NAL parsing in the UserData fifo
 *    The current NALByteStream position is pushed at the end of UserDataPositionArray
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_PushUserDataItem(U32 PayLoadSize, U32 PayLoadType, const PARSER_Handle_t ParserHandle)
{
    H264ParserPrivateData_t * PARSER_Data_p;
    U32 i;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    i=PARSER_Data_p->UserDataPositionArray.Iterator;

    if (i < MAX_USER_DATA_NUMBER )
    {
        STOS_memcpy(&(PARSER_Data_p->UserDataPositionArray.NALByteStream[i]), &(PARSER_Data_p->NALByteStream), sizeof(NALByteStream_t));
        PARSER_Data_p->UserDataPositionArray.PayloadSize[i] = PayLoadSize;
        PARSER_Data_p->UserDataPositionArray.PayloadType[i] = PayLoadType;
        PARSER_Data_p->UserDataPositionArray.Iterator++;
        PARSER_Data_p->PictureLocalData.HasUserData = TRUE;
    }
    else
    {
        /* TODO: too many UserData messages parsed before the slice NAL: report pb */
    }


}


/******************************************************************************/
/*! \brief Restores the read pointer of the NAL parsing from the UserData fifo
 *            NALByteStream is retrieved from the first item in the list
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the new PARSER_Data_p->NALByteStream, or NULL if the list is empty
 */
/******************************************************************************/
NALByteStream_t* h264par_GetUserDataItem(U32 * PayLoadSize_p, U32 * PayLoadType_p, const PARSER_Handle_t ParserHandle)
{
    H264ParserPrivateData_t * PARSER_Data_p;
    U32 i;
    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if(PARSER_Data_p->UserDataPositionArray.Iterator == 0)
    {
        /* empty list */
        return NULL;
    }
    PARSER_Data_p->UserDataPositionArray.Iterator--;
    i = PARSER_Data_p->UserDataPositionArray.Iterator;

    STOS_memcpy(&(PARSER_Data_p->NALByteStream), &(PARSER_Data_p->UserDataPositionArray.NALByteStream[i]), sizeof(NALByteStream_t));
    *PayLoadSize_p = PARSER_Data_p->UserDataPositionArray.PayloadSize[i];
    *PayLoadType_p = PARSER_Data_p->UserDataPositionArray.PayloadType[i];

    return &(PARSER_Data_p->NALByteStream);
}

