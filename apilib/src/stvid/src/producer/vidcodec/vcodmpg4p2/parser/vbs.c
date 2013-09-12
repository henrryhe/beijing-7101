/*!
 *******************************************************************************
 * \file VBS.c
 *
 * \brief mpeg4p2 Visual Bit Stream extraction functions
 *
 * \author
 * Sibnath Ghosh \n
 * HVD/DVD-STB \n
 * Copyright (C) STMicroelectronics 2007
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#endif
#include "sttbx.h"

/* mpeg4p2 Parser specific */
#include "mpg4p2parser.h"

/* Functions ---------------------------------------------------------------- */

/******************************************************************************/
/*! \brief Moves the ByteStream.ByteOffset to the next byte in the bytestream.
 *
 * ByteStream.ByteOffset is modified according to the endianness of the byte stream
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_MoveToNextByteInByteStream(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    if (PARSER_Data_p->ByteStream.WordSwapFlag == FALSE)
    {
        /* Bytes are just stored in successive addresses in memory */
        PARSER_Data_p->ByteStream.ByteOffset ++;
        /* Adjust with Bit buffer boundaries */
        if (((U32)PARSER_Data_p->ByteStream.StartByte_p + PARSER_Data_p->ByteStream.ByteOffset) > (U32)STAVMEM_DeviceToCPU(PARSER_Data_p->BitBuffer.ES_Stop_p,&VirtualMapping))
        {
            PARSER_Data_p->ByteStream.StartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->BitBuffer.ES_Start_p,&VirtualMapping);
            PARSER_Data_p->ByteStream.ByteOffset = 0;
        }
    }
    else
    {
        /* Word swapping occurs */
        if ((PARSER_Data_p->ByteStream.ByteOffset & 0x3) == 0)
        {
            /* if current ByteOffset is aligned on a word boundary, jump to next word */
            PARSER_Data_p->ByteStream.ByteOffset += 7;
            /* Adjust with Bit buffer boundaries */
            if (((U32)PARSER_Data_p->ByteStream.StartByte_p + PARSER_Data_p->ByteStream.ByteOffset) > (U32)STAVMEM_DeviceToCPU(PARSER_Data_p->BitBuffer.ES_Stop_p,&VirtualMapping))
            {
                PARSER_Data_p->ByteStream.StartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->BitBuffer.ES_Start_p,&VirtualMapping);
                PARSER_Data_p->ByteStream.ByteOffset = 3;
            }
        }
        else
        {
            /* Else, keep in the same word (then decrement byte address) */
            PARSER_Data_p->ByteStream.ByteOffset --;
        }
    }
    PARSER_Data_p->ByteStream.ByteCounter++; /* One more byte read */
}

/******************************************************************************/
/*! \brief Returns the current byte in the bytestream.
 *         Discards emulation prevention bytes
 *         Exits if the byte to extract is not aligned
 *
 * ByteStream.ByteOffset is incremented by two if current byte is an emulation prevention byte \n
 *                           is incremented by one otherwise \n
 * ByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the extracted byte
 */
/******************************************************************************/
U8 mpeg4p2par_GetByteFromByteStream(const PARSER_Handle_t  ParserHandle)
{
    U8 Byte; /* the returned value */
    /* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Extract next byte in byte stream */
    mpeg4p2par_MoveToNextByteInByteStream(ParserHandle);

    /* Check if the byte to extract is still inside the stream */
    if (PARSER_Data_p->ByteStream.ByteCounter == PARSER_Data_p->ByteStream.Len)
    {
        /* The byte to extract is out of the stream boundary */
        PARSER_Data_p->ByteStream.EndOfStreamFlag = TRUE;
    }
    Byte = PARSER_Data_p->ByteStream.StartByte_p[PARSER_Data_p->ByteStream.ByteOffset];

    /* Store last and last but one byte for further emulation prevention detection */
    PARSER_Data_p->ByteStream.LastButOneByte = PARSER_Data_p->ByteStream.LastByte;
    PARSER_Data_p->ByteStream.LastByte = Byte;

    return Byte;

} /* end of GetByteFromByteStream */

/******************************************************************************/
/*! \brief returns the current bit in the bytestream.
 *
 * ByteStream.BitOffset is decremented by one and clipped to [7:-1] \n
 * ByteStream.ByteOffset may be incremented by one or two if ByteStream.BitOffset was -1 on entering \n
 * ByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the extracted bit
 */
/******************************************************************************/
U8 mpeg4p2par_GetBitFromByteStream(const PARSER_Handle_t  ParserHandle)
{
    U8 Byte;
    U8 Bit;
    S8 BitOffset;
    /* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Check if new byte is required */
    if (PARSER_Data_p->ByteStream.BitOffset == -1)
    {
        PARSER_Data_p->ByteStream.BitOffset = 7;
        Byte = mpeg4p2par_GetByteFromByteStream(ParserHandle);
    }
    else
    {
        /* the default byte where to extract the bit from is the current byte */
        Byte = PARSER_Data_p->ByteStream.StartByte_p[PARSER_Data_p->ByteStream.ByteOffset];
    }

    /* Set BitOffset versus ByteSwapFlag */
    BitOffset = PARSER_Data_p->ByteStream.BitOffset;
    if (PARSER_Data_p->ByteStream.ByteSwapFlag == TRUE)
    {
        BitOffset = 7 - BitOffset;
    }

    if ((Byte & (1 << BitOffset)) != 0)
    {
        Bit =  1;
    }
    else
    {
        Bit =  0;
    }
    PARSER_Data_p->ByteStream.BitOffset --;
    return Bit;
}

/******************************************************************************/
/*! \brief Return TRUE is there is more data in unit
 *
 * ByteStream.ByteOffset is incremented by two if current byte is an emulation prevention byte \n
 *                           is incremented by one otherwise \n
 * ByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return TRUE/FALSE
 */
/******************************************************************************/
BOOL mpeg4p2par_IsMoreRbspData(const PARSER_Handle_t  ParserHandle)
{
    ByteStream_t SavedByteStream;  /* Save all ByteStream pointers in
                                           order to get back after using following bytes */
    U8      Bit;
    BOOL    IsMoreRbspData = FALSE;
    /* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Save all ByteStream pointers */
    SavedByteStream = PARSER_Data_p->ByteStream;

   /* Look for rbsp_stop_one_bit which close any rbsp */
   /* (mpeg4p2 standard G050r1d1 "7.4.1.1 Encapsulation of an SODB within an RBSP" */
    Bit = mpeg4p2par_GetUnsigned(1, ParserHandle);
    if(Bit != 1)
    {   /* This is not a rbsp_stop_one_bit thus this is addition data */
        IsMoreRbspData = TRUE;
    }
    else
    {
        /* Check if there are other "Ones" before next start code */
        while(((PARSER_Data_p->ByteStream.ByteCounter != PARSER_Data_p->ByteStream.Len - 3) ||
               (PARSER_Data_p->ByteStream.BitOffset != -1)) &&
              !IsMoreRbspData)
        { /* While not at first byte of start code */
            Bit = mpeg4p2par_GetUnsigned(1, ParserHandle);
            if(Bit == 1)
            {
                IsMoreRbspData = TRUE;
            }
        }
    }
    /* Restore all ByteStream pointers in order to process remaining rbsp_data */
    PARSER_Data_p->ByteStream = SavedByteStream;
    return(IsMoreRbspData);
}

/******************************************************************************/
/*! \brief Returns a Len- bit integer read from the byte stream (MSB first)
 *
 * ByteStream.BitOffset is modified \n
 * ByteStream.ByteOffset is modified \n
 * ByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte \n
 * \n
 * Info: mpeg4p2 refers to this function as "u(n)" or "f(n)" or "b(n)"
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param Len the length in bits of the integer to extract
 * \return the integer extracted from the byte stream
 */
/******************************************************************************/
U32 mpeg4p2par_GetUnsigned(U8 Len, const PARSER_Handle_t  ParserHandle)
{
    U32 UnsInt;
    /* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    UnsInt = 0;
    while (Len)
    {
        /* Assumption: read MSBits first */
        UnsInt <<= 1;
        UnsInt += mpeg4p2par_GetBitFromByteStream(ParserHandle);
        Len--;
    }
    return UnsInt;
}

/******************************************************************************/
/*! \brief Returns a Len- bit integer read from the byte stream (MSB first)
 * without incrementing the pointer
 *
 * ByteStream.BitOffset is not modified \n
 * ByteStream.ByteOffset is not modified \n
 * ByteStream.EndOfStreamFlag is not modified \n
 * \n
 * Info: mpeg4p2 refers to this function as "u(n)" or "f(n)" or "b(n)"
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param Len the length in bits of the integer to read only
 * \return the integer read from the byte stream
 */
/******************************************************************************/
U32 mpeg4p2par_ReadUnsigned(U8 Len, const PARSER_Handle_t  ParserHandle)
{
    ByteStream_t SavedByteStream;  /* Save all ByteStream pointers in
                                           order to get back after using following bytes */
    U32 UnsInt;
    /* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Save all ByteStream pointers */
    SavedByteStream = PARSER_Data_p->ByteStream;

    UnsInt = 0;
    while (Len)
    {
        /* Assumption: read MSBits first */
        UnsInt <<= 1;
        UnsInt += mpeg4p2par_GetBitFromByteStream(ParserHandle);
        Len--;
    }

    /* Restore all ByteStream pointers in order to process remaining data */
    PARSER_Data_p->ByteStream = SavedByteStream;

    return UnsInt;
}

/******************************************************************************/
/*! \brief returns a Len- bit signed integer read from the byte stream (MSB first)
 *
 * ByteStream.BitOffset is modified \n
 * ByteStream.ByteOffset is modified \n
 * ByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte \n
 * \n
 * Info: mpeg4p2 refers to this function as "i(n)"
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param Len the length in bits of the signed integer to extract
 * \return the integer extracted from the byte stream
 */
/******************************************************************************/
S32 mpeg4p2par_GetSigned(U8 Len, const PARSER_Handle_t  ParserHandle)
{
    S32 SiInt;
    /* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Len >= 2 is assumed: 1 bit for sign, 1 bit (minimum) for value */
    /* First bit in stream is the sign */
    /* if first bit = 1: negative number is expected: initialize to -1 (to shift 111 left later on) */
    /* if first bit = 0: positive number is expected: intialize to 0 */
    SiInt = - mpeg4p2par_GetBitFromByteStream(ParserHandle);

    Len--;

    while (Len)
    {
        /* Assumption: read MSBits first */
        SiInt <<= 1;
        SiInt += mpeg4p2par_GetBitFromByteStream(ParserHandle);
        Len--;
    }

    return SiInt;
}

/******************************************************************************/
/*! \brief given a table defining the VLC, it returns a variable length code.
 *			This can handle a maximum of 32 bit vlc
 *
 ******************************************************************************/
U32 mpeg4p2par_GetVariableLengthCode(U32 *VLCTable, U32 MaxLength, const PARSER_Handle_t  ParserHandle)
{
	U32 Index=0;
	U32 NumBits=0;
	U32 JumpIndex, FinalValue;
	S32 ReturnValue = 0xFFFFFFFF;

	while (NumBits++ <= MaxLength)
	{
		FinalValue = mpeg4p2par_GetUnsigned(1, ParserHandle);
		JumpIndex= (FinalValue==1) ? VLCTable[Index+1]:VLCTable[Index];
        /* if 0, we've reached the leaf of the vlc tree */
		if (JumpIndex==0)
		{
			ReturnValue = (FinalValue == 1) ? (Index + 1):Index;
			break;
		}
		else
		{
			Index = JumpIndex;
		}
	}

	if(NumBits > MaxLength)
	{
		STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR!!INCORRECT VLC...EXCEEDED MAXIMUM PERMISSIBLE LENGTH!!!\n\n"));
	}

	return ReturnValue;
}/* end GetVariableLengthCode() */

/******************************************************************************/
/*! \brief Causes the Parser to read in the bytes in the order
 *         that they exist in the bit buffer because
 *         the bytes in a word have the MSB first and the LSB last
 *
 *  CAUTION:  This should be called only at the beginning of a
 *            32 bit word.
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return nothing
 */
/******************************************************************************/
void mpeg4p2par_SwitchFromLittleEndianToBigEndianParsing(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->ByteStream.WordSwapFlag = FALSE;

	/* Assuming that we have been reading in Little Endian
	   words and we are now needing to read in Big Endian words
	   and that we have just completed reading in a Little Endian
	   word.  Therefore, ByteOffset should now be referring to
	   the  first byte in memory (the LSB) of the word what was
	   just read in.  If we leave it there, we will once again
	   read in the same word in the wrong order.  So advance
	   the ByteOffset to the last Byte in memory of the current word
	   so that the next Byte read in will be the first Byte of the
	   next word.  The Byte reader always advances to the next
	   byte before reading in that next byte.*/
	if ((PARSER_Data_p->ByteStream.ByteOffset & 0x3) == 0)
    {
		PARSER_Data_p->ByteStream.ByteOffset += 3;
	}

}/* end mpeg4p2par_SwitchFromLittleEndianToBigEndianParsing() */

/******************************************************************************/
/*! \brief Causes the Parser to read in the bytes in reverse order because
 *         the bytes in a word have the LSB first and the MSB last in the
 *         bitstream.  In other words, the word
 *           0x98765432   would occur in the following order in the
 *         stream (bit buffer)   0x32 0x54 0x76 0x98.  Setting this
 *         causes the ordering of the bytes to be automatically
 *         reversed "behind your back" as you read in bits.
 *
 *  CAUTION:  This should be called only at the beginning of a
 *            32 bit word.
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return nothing
 */
/******************************************************************************/
void mpeg4p2par_SwitchFromBigEndianToLittleEndianParsing(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->ByteStream.WordSwapFlag = TRUE;

	/* Assuming that we are at the end of the previous 32 bit word,
	   We now need to advance to the first byte of the 32 bit word so that
	   the little endian parser will know that we are now at the end
	   of the word and need to advance to the next word to read it.
	   The parser always advances to the next byte before reading
	   in the next byte.*/
	if ((PARSER_Data_p->ByteStream.ByteOffset & 0x3) == 0x3)
    {
		PARSER_Data_p->ByteStream.ByteOffset -= 3;
	}

}/* end mpeg4p2par_SwitchFromBigEndianToLittleEndianParsing() */




