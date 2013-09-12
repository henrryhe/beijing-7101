/*!
 *******************************************************************************
 * \file BDU.c
 *
 * \brief vc1 BDU unit bitstream extraction functions
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) STMicroelectronics 2007
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined (ST_OSLINUX)
#include <stdio.h>
#endif

#include "sttbx.h"
/* vc1 Parser specific */
#include "vc1parser.h"

/* Functions ---------------------------------------------------------------- */
static U8 vc1par_GetByteFromBDU(const PARSER_Handle_t  ParserHandle);
static U8 vc1par_GetBitFromBDU(const PARSER_Handle_t  ParserHandle);


/******************************************************************************/
/*! \brief Moves the BDUByteStream.ByteOffset to the next byte in the bytestream.
 *
 * BDUByteStream.ByteOffset is modified according to the endianness of the byte stream
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void vc1par_MoveToNextByteFromBDU(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    if (!(PARSER_Data_p->BDUByteStream.WordSwapFlag))
    {
        /* Bytes are just stored in successive addresses in memory */
        PARSER_Data_p->BDUByteStream.ByteOffset ++;
        /* Adjust with Bit buffer boundaries */
#if defined(DVD_SECURED_CHIP)
        if (((U32)PARSER_Data_p->BDUByteStream.StartByte_p + PARSER_Data_p->BDUByteStream.ByteOffset) > (U32)STAVMEM_DeviceToCPU(PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p,&VirtualMapping))
        {
            PARSER_Data_p->BDUByteStream.StartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,&VirtualMapping);
            PARSER_Data_p->BDUByteStream.ByteOffset = 0;
        }
#else /* defined(DVD_SECURED_CHIP) */
        if (((U32)PARSER_Data_p->BDUByteStream.StartByte_p + PARSER_Data_p->BDUByteStream.ByteOffset) > (U32)STAVMEM_DeviceToCPU(PARSER_Data_p->BitBuffer.ES_Stop_p,&VirtualMapping))
        {
            PARSER_Data_p->BDUByteStream.StartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->BitBuffer.ES_Start_p,&VirtualMapping);
            PARSER_Data_p->BDUByteStream.ByteOffset = 0;
        }
#endif /* defined(DVD_SECURED_CHIP) */
    }
    else
    {
        /* Word swapping occurs */
        if ((PARSER_Data_p->BDUByteStream.ByteOffset & 0x3) == 0)
        {
            /* if current ByteOffset is aligned on a word boundary, jump to next word */
            PARSER_Data_p->BDUByteStream.ByteOffset += 7;
            /* Adjust with Bit buffer boundaries */

#if defined(DVD_SECURED_CHIP)
            if (((U32)PARSER_Data_p->BDUByteStream.StartByte_p + PARSER_Data_p->BDUByteStream.ByteOffset) > (U32)STAVMEM_DeviceToCPU(PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p,&VirtualMapping))
            {
                PARSER_Data_p->BDUByteStream.StartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,&VirtualMapping);
                PARSER_Data_p->BDUByteStream.ByteOffset = 3;
            }
#else /* defined(DVD_SECURED_CHIP) */
            if (((U32)PARSER_Data_p->BDUByteStream.StartByte_p + PARSER_Data_p->BDUByteStream.ByteOffset) > (U32)STAVMEM_DeviceToCPU(PARSER_Data_p->BitBuffer.ES_Stop_p,&VirtualMapping))
            {
                PARSER_Data_p->BDUByteStream.StartByte_p = STAVMEM_DeviceToCPU(PARSER_Data_p->BitBuffer.ES_Start_p,&VirtualMapping);
                PARSER_Data_p->BDUByteStream.ByteOffset = 3;
            }
#endif /* defined(DVD_SECURED_CHIP) */
        }
        else
        {
            /* Else, keep in the same word (then decrement byte address) */
            PARSER_Data_p->BDUByteStream.ByteOffset --;
        }
    }
    PARSER_Data_p->BDUByteStream.ByteCounter++; /* One more byte read */
}

/******************************************************************************/
/*! \brief Returns the current byte in the bytestream.
 *         Discards emulation prevention bytes
 *         Exits if the byte to extract is not aligned
 *
 * BDUByteStream.ByteOffset is incremented by two if current byte is an emulation prevention byte \n
 *                           is incremented by one otherwise \n
 * BDUByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the extracted byte
 */
/******************************************************************************/
static U8 vc1par_GetByteFromBDU(const PARSER_Handle_t  ParserHandle)
{
    U8 BDUByte; /* the returned value */
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Extract next byte in byte stream */
    vc1par_MoveToNextByteFromBDU(ParserHandle);

    /* Only look for start code emulation prevention bytes if we know that
	   start code emulation has been used in this part of the stream. */
    if(PARSER_Data_p->BDUByteStream.StartCodeEmulationPreventionBytesUsed)
	{
		/* Check if this byte is not a emulation prevention byte */
        if ( ( (!(PARSER_Data_p->BDUByteStream.ByteSwapFlag)) &&
			(PARSER_Data_p->BDUByteStream.StartByte_p[PARSER_Data_p->BDUByteStream.ByteOffset] == EMULATION_PREVENTION_BYTE)
			) ||
            ( (PARSER_Data_p->BDUByteStream.ByteSwapFlag) &&
			(PARSER_Data_p->BDUByteStream.StartByte_p[PARSER_Data_p->BDUByteStream.ByteOffset] == ETYB_NOITNEVERP_NOITALUME)
			)
			)
		{
            if (PARSER_Data_p->BDUByteStream.ByteOffset >= 2)
            {
                /* Check for pattern 0x00 0x00 0x03 */
                if ((PARSER_Data_p->BDUByteStream.LastByte == 0) && /* previous byte */
                    (PARSER_Data_p->BDUByteStream.LastButOneByte == 0))   /* and the byte before */
                {
				/* The byte to extract is an emulation prevention byte as it is
				* preceeded by two 0-byte.
				* Let's get the next byte in the byte stream: this one (if any) is not
				* an emulation prevention byte for sure!
					*/
                    vc1par_MoveToNextByteFromBDU(ParserHandle);
                    /* Reset the fifo not to get spurious emulation bytes */
                    PARSER_Data_p->BDUByteStream.LastByte = 0xff;
                    PARSER_Data_p->BDUByteStream.LastButOneByte = 0xff;
                }
			}
		}
    }/* end if(PARSER_Data_p->BDUByteStream.StartCodeEmulationPreventionBytesUsed) */

    /* Check if the byte to extract is still inside the stream */
    if (PARSER_Data_p->BDUByteStream.ByteCounter == PARSER_Data_p->BDUByteStream.Len)
    {
        /* The byte to extract is out of the stream boundary */
        PARSER_Data_p->BDUByteStream.EndOfStreamFlag = TRUE;
    }
    BDUByte = PARSER_Data_p->BDUByteStream.StartByte_p[PARSER_Data_p->BDUByteStream.ByteOffset];

    /* Store last and last but one byte for further emulation prevention detection */
    PARSER_Data_p->BDUByteStream.LastButOneByte = PARSER_Data_p->BDUByteStream.LastByte;
    PARSER_Data_p->BDUByteStream.LastByte = BDUByte;

    return BDUByte;

} /* end of GetByteFromBDU */

/******************************************************************************/
/*! \brief returns the current bit in the bytestream.
 *
 * BDUByteStream.BitOffset is decremented by one and clipped to [7:-1] \n
 * BDUByteStream.ByteOffset may be incremented by one or two if BDUByteStream.BitOffset was -1 on entering \n
 * BDUByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return the extracted bit
 */
/******************************************************************************/
static U8 vc1par_GetBitFromBDU(const PARSER_Handle_t  ParserHandle)
{
    U8 BDUByte;
    U8 BDUBit;
    S8 BitOffset;
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Check if new byte is required */
    if (PARSER_Data_p->BDUByteStream.BitOffset == -1)
    {
        PARSER_Data_p->BDUByteStream.BitOffset = 7;
        BDUByte = vc1par_GetByteFromBDU(ParserHandle);
    }
    else
    {
        /* the default byte where to extract the bit from is the current byte */
        BDUByte = PARSER_Data_p->BDUByteStream.StartByte_p[PARSER_Data_p->BDUByteStream.ByteOffset];
    }

    /* Set BitOffset versus ByteSwapFlag */
    BitOffset = PARSER_Data_p->BDUByteStream.BitOffset;
    if (PARSER_Data_p->BDUByteStream.ByteSwapFlag)
    {
        BitOffset = 7 - BitOffset;
    }

    if ((BDUByte & (1 << BitOffset)) != 0)
    {
        BDUBit =  1;
    }
    else
    {
        BDUBit =  0;
    }
    PARSER_Data_p->BDUByteStream.BitOffset --;
    return BDUBit;
}

/******************************************************************************/
/*! \brief Return TRUE is there is more data in BDU unit
 *
 * BDUByteStream.ByteOffset is incremented by two if current byte is an emulation prevention byte \n
 *                           is incremented by one otherwise \n
 * BDUByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return TRUE/FALSE
 */
/******************************************************************************/
BOOL vc1par_IsMoreRbspData(const PARSER_Handle_t  ParserHandle)
{
    BDUByteStream_t SavedBDUByteStream;  /* Save all BDUByteStream pointers in
                                           order to get back after aBDUysing following bytes */
    U8      BDUBit;
    BOOL    IsMoreRbspData = FALSE;
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Save all BDUByteStream pointers */
    SavedBDUByteStream = PARSER_Data_p->BDUByteStream;

   /* Look for rbsp_stop_one_bit which close any rbsp */
   /* (vc1 standard G050r1d1 "7.4.1.1 Encapsulation of an SODB within an RBSP" */
    BDUBit = vc1par_GetUnsigned(1, ParserHandle);
    if(BDUBit != 1)
    {   /* This is not a rbsp_stop_one_bit thus this is additionBDU data */
        IsMoreRbspData = TRUE;
    }
    else
    {
        /* Check if there are other "Ones" before next start code */
        while(((PARSER_Data_p->BDUByteStream.ByteCounter != PARSER_Data_p->BDUByteStream.Len - 3) ||
               (PARSER_Data_p->BDUByteStream.BitOffset != -1)) &&
              !IsMoreRbspData)
        { /* While not at first byte of start code */
            BDUBit = vc1par_GetUnsigned(1, ParserHandle);
            if(BDUBit == 1)
            {
                IsMoreRbspData = TRUE;
            }
        }
    }
    /* Restore all BDUByteStream pointers in order to process remaining rbsp_data */
    PARSER_Data_p->BDUByteStream = SavedBDUByteStream;
    return(IsMoreRbspData);
}

/******************************************************************************/
/*! \brief Returns a Len- bit integer read from the byte stream (MSB first)
 *
 * BDUByteStream.BitOffset is modified \n
 * BDUByteStream.ByteOffset is modified \n
 * BDUByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte \n
 * \n
 * Info: vc1 refers to this function as "u(n)" or "f(n)" or "b(n)"
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param Len the length in bits of the integer to extract
 * \return the integer extracted from the byte stream
 */
/******************************************************************************/
U32 vc1par_GetUnsigned(U8 Len, const PARSER_Handle_t  ParserHandle)
{
    U32 UnsInt;
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    UnsInt = 0;
    while (Len)
    {
        /* Assumption: read MSBits first */
        UnsInt <<= 1;
        UnsInt += vc1par_GetBitFromBDU(ParserHandle);
        Len--;
    }
    return UnsInt;
}

/******************************************************************************/
/*! \brief returns a Len- bit signed integer read from the byte stream (MSB first)
 *
 * BDUByteStream.BitOffset is modified \n
 * BDUByteStream.ByteOffset is modified \n
 * BDUByteStream.EndOfStreamFlag is set to TRUE if the function parses a out of bound byte \n
 * \n
 * Info: vc1 refers to this function as "i(n)"
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param Len the length in bits of the signed integer to extract
 * \return the integer extracted from the byte stream
 */
/******************************************************************************/
S32 vc1par_GetSigned(U8 Len, const PARSER_Handle_t  ParserHandle)
{
    S32 SiInt;
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Len >= 2 is assumed: 1 bit for sign, 1 bit (minimum) for value */
    /* First bit in stream is the sign */
    /* if first bit = 1: negative number is expected: initialize to -1 (to shift 111 left later on) */
    /* if first bit = 0: positive number is expected: intialize to 0 */
    SiInt = - vc1par_GetBitFromBDU(ParserHandle);

    Len--;

    while (Len)
    {
        /* Assumption: read MSBits first */
        SiInt <<= 1;
        SiInt += vc1par_GetBitFromBDU(ParserHandle);
        Len--;
    }

    return SiInt;
}

/******************************************************************************/
/*! \brief given a table defining the VLC, it returns a variable length code.
 *			This can handle a maximum of 32 bit vlc
 *
 ******************************************************************************/
U32 vc1par_GetVariableLengthCode(U32 *VLCTable, U32 MaxLength, const PARSER_Handle_t  ParserHandle)
{
	U32 Index=0;
	U32 NumBits=0;
	U32 JumpIndex, FinalValue;
	S32 ReturnValue = 0xFFFFFFFF;

	while (NumBits++ <= MaxLength)
	{
		FinalValue = vc1par_GetUnsigned(1, ParserHandle);
/*	    "Final Value is ",FinalValue,"<br>"; */
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
		STTBX_Print(("ERROR!!INCORRECT VLC...EXCEEDED MAXIMUM PERMISSIBLE LENGTH!!!\n\n"));
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
void vc1par_SwitchFromLittleEndianToBigEndianParsing(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->BDUByteStream.WordSwapFlag = FALSE;

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
	if ((PARSER_Data_p->BDUByteStream.ByteOffset & 0x3) == 0)
    {
		PARSER_Data_p->BDUByteStream.ByteOffset += 3;
	}

}/* end vc1par_SwitchFromLittleEndianToBigEndianParsing() */

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
void vc1par_SwitchFromBigEndianToLittleEndianParsing(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->BDUByteStream.WordSwapFlag = TRUE;

	/* Assuming that we are at the end of the previous 32 bit word,
	   We now need to advance to the first byte of the 32 bit word so that
	   the little endian parser will know that we are now at the end
	   of the word and need to advance to the next word to read it.
	   The parser always advances to the next byte before reading
	   in the next byte.*/
	if ((PARSER_Data_p->BDUByteStream.ByteOffset & 0x3) == 0x3)
    {
		PARSER_Data_p->BDUByteStream.ByteOffset -= 3;
	}

}/* end vc1par_SwitchFromBigEndianToLittleEndianParsing() */


/******************************************************************************/
/*! \brief enables the detection of Start Code Emulation Prevention bytes
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return nothing
 */
/******************************************************************************/
void vc1par_EnableDetectionOfStartCodeEmulationPreventionBytes(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->BDUByteStream.StartCodeEmulationPreventionBytesUsed = TRUE;
}/* vc1par_EnableDetectionOfStartCodeEmulationPreventionBytes() */


/******************************************************************************/
/*! \brief enables the detection of Start Code Emulation Prevention bytes
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return nothing
 */
/******************************************************************************/
void vc1par_DisableDetectionOfStartCodeEmulationPreventionBytes(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->BDUByteStream.StartCodeEmulationPreventionBytesUsed = FALSE;
}/* vc1par_DisableDetectionOfStartCodeEmulationPreventionBytes() */


