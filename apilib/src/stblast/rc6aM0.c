
/*****************************************************************************
File Name   : rc6aM0.c

Description : STBLAST RC6A encoding/decoding routines

Copyright (C) 2002 STMicroelectronics

History     : Created April 2002

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdlib.h>
#endif

#include "stddefs.h"
#include "stblast.h"
#include "blastint.h"
#include "rc6aM0.h"
#if defined(IR_INVERT)
#include "invinput.h"
#endif


/*****************************************************************************
BLAST_RC6ADecode_Mode0()
Description:
    Decode RC6A symbols into user buffer

Parameters:
    UserBuf_p           user buffer to place data into
    UserBufSize         how big the user buff is
    SymbolBuf_p         symbol buffer to consume
    SymbolsAvailable    how many symbols present in buffer
    NumDecoded_p        how many data words we decoded
    SymbolsUsed_p       how many symbols we used
    RcProtocol_p        protocol block

Return Value:
    ST_NO_ERROR

See Also:
    BLAST_SpaceDecode
    BLAST_RC5Encode
*****************************************************************************/

/* used for decoding */
/* One bit period is defined to be 888.888 us */
#define T (889)

#define MAX_3T 1573
#define MIN_3T 1125
#define MAX_2T 1125
#define MIN_2T 671
#define MAX_1T 658
#define MIN_1T 249

#define HEADER_ARRAY_SIZE 10

/* used for encoding */
#define MID_1T   (U16)445
#define MID_2T   (U16)889
#define MID_3T   (U16)1334
#define MID_6T   (U16)2667

#define SubCarrierFreq (U16)36000

/* macros */
#define GenerateSymbol(mark, space, Buffer_p) \
        Buffer_p->MarkPeriod = MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, mark);    \
        Buffer_p->SymbolPeriod = MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, mark+space)

#define GenerateMark(mark, Buffer_p) \
        (Buffer_p->MarkPeriod = MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, mark) )

#define GenerateSpace(space, Buffer_p) \
        (Buffer_p->SymbolPeriod = Buffer_p->MarkPeriod + MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, space) )

#define ExtendMark(mark, Buffer_p) \
        (Buffer_p->MarkPeriod = Buffer_p->MarkPeriod + MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, mark) )

#define ExtendSpace(space, Buffer_p) \
        (Buffer_p->SymbolPeriod = Buffer_p->SymbolPeriod + MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, space) )

#define IsOdd(i) (i & 1)

/* declarations */
U8 DurationTransform2_Mode0(U16 Value);
void DurationTransform_Mode0(STBLAST_Symbol_t* SymbolBuf_p,
                        const U32 SymbolsAvailable,
                        U8 * DurationArray);

U8 ValidateHeader_Mode0(U8* LevelArray, U8 *DurationArray, U8 DurationArraySize );

/***************************** WORKER ROUTINES  *****************************/

void DurationTransform_Mode0(STBLAST_Symbol_t* SymbolBuf_p,
                        const U32 SymbolsAvailable,
                        U8 * DurationArray)
{
    U32 i; /* changed from U32 to int to remove lint warning*/

    for (i=0; i<SymbolsAvailable; i++)
    {
        *DurationArray = DurationTransform2_Mode0( SymbolBuf_p->MarkPeriod );
        DurationArray++;
        *DurationArray = DurationTransform2_Mode0( SymbolBuf_p->SymbolPeriod - SymbolBuf_p->MarkPeriod);
        DurationArray++;
        SymbolBuf_p++;
    }

}

U8 DurationTransform2_Mode0(U16 Value)
{
    if(Value > RC6AM0_MAX_6T)
    {
        return 0xFF;  /* timeout value */
    }
    else if(Value > MAX_3T)
    {
        if(Value > RC6AM0_MIN_6T)
        {
            return 6;
        }
        else
        {
            return 0;
        }
    }
    else if(Value > MAX_2T)
    {
        if(Value > MIN_3T)
        {
            return 3;
        }
        else
        {
            return 0;
        }
    }
    else if(Value > MAX_1T)
    {
        if(Value > MIN_2T )
        {
            return 2;
        }
        else
        {
            return 0;
        }
    }
    else if(Value > MIN_1T)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/*  this function finds the header sequence in the set of received symbols
    and returns the index in DurationArray of level ONE (not Zero) of the Cust Code
*/
U8 ValidateHeader_Mode0(U8* LevelArray, U8 *DurationArray, U8 DurationArraySize)
{
    U8 HeaderArray[] = { 6, 2, 1, 2, 1, 1, 1, 1,0 }; /* 0 is a termination code */


    U8 *HeaderArrayPtr;
    U8 DurationArrayCounter, PrevDurationArrayCounter;

    DurationArrayCounter = 0;
    HeaderArrayPtr = HeaderArray;

    while(DurationArrayCounter < DurationArraySize-HEADER_ARRAY_SIZE)
    {
        PrevDurationArrayCounter = DurationArrayCounter;

        while(DurationArray[DurationArrayCounter] == *HeaderArrayPtr)
        {
            HeaderArrayPtr++;
            DurationArrayCounter++;

            if (*HeaderArrayPtr == 0) /* First 8 duration */
            {
                /* Identify the trailer bit
                   whether it is 0 or 1 */
                /* trailer 0*/
                if(DurationArray[DurationArrayCounter] == 1)
                {
                    LevelArray[0] = 0;
                    LevelArray[1] = !LevelArray[0];
                    DurationArrayCounter++;

                    if(DurationArray[DurationArrayCounter] != 2)
                    {
                        return 0;
                    }
                    else
                    {
                        DurationArrayCounter++;
                    }

                    if(DurationArray[DurationArrayCounter] == 2)
                    {
                        LevelArray[2] = 0;
                        return DurationArrayCounter + 2;
                    }
                    else if(DurationArray[DurationArrayCounter] == 3)
                    {
                        LevelArray[2] = 1;
                        return DurationArrayCounter + 1;
                    }
                    else
                    {
                        return 0; /* trailer incorrect */
                    }

                }
                /* Trailer 1*/
                else if(DurationArray[DurationArrayCounter] == 3)
                {
                    LevelArray[0] = 1;
                    LevelArray[1] = !LevelArray[0];
                    DurationArrayCounter++;

                    if(DurationArray[DurationArrayCounter] == 3)
                    {
                        LevelArray[2] = 0;
                        return DurationArrayCounter + 1;
                    }
                    else if(DurationArray[DurationArrayCounter] == 2)
                    {
                        LevelArray[2] = 1;
                        return DurationArrayCounter + 2;
                    }
                    else
                    {
                        return 0; /* trailer incorrect */
                    }
                }
                else
                {
                    return 0; /* trailer incorrect */
                }

            }
        }

        HeaderArrayPtr = HeaderArray; /* reset pointer to the start of the HeaderArray */
        DurationArrayCounter = PrevDurationArrayCounter + 1; /* increment DurationArrayCounter if no match found */
    }

    /* if this point is reached, header was not found */
    return 0;
}


ST_ErrorCode_t BLAST_RC6ADecode_Mode0(U32           *UserBuf_p,
                                      STBLAST_Symbol_t     *SymbolBuf_p,
		                              U32                  SymbolsAvailable,
		                              U32                  *NumDecoded_p,
		                              U32                  *SymbolsUsed_p,
		                              const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U8 i;
    U8 j;
    U32 SymbolsLeft;
    U8 DurationArray[200];
    U8 LevelArraySize;
    U8 LevelArray[400];  /* check these values */
    U32 DecodedValue = 0;
    U8 CustCodeDurationOneIndex;

    /* Set number of symbols allowed */
    SymbolsLeft = SymbolsAvailable;
    *SymbolsUsed_p = 0;
    *NumDecoded_p = 0;
      
#if defined (IR_INVERT)
    {
        STBLAST_Symbol_t *LastSymbol_p;
        if (InvertedInputCompensate(SymbolBuf_p,
                                (MID_6T),
                                SymbolsAvailable) == FALSE)
        {

            *NumDecoded_p = 0;
            return ST_NO_ERROR;
        }
        SymbolsAvailable = SymbolsAvailable-1;
        LastSymbol_p = SymbolBuf_p + (SymbolsAvailable-1);
        LastSymbol_p->SymbolPeriod = 0xFFFF;
    }

#endif


    /* parse symbol buffer with a ticker.  Each segment is of duration 2T */
    /* Generate array of durations as multiples of T */
    DurationTransform_Mode0(SymbolBuf_p, SymbolsAvailable, DurationArray);


    /*** HEADER VALIDATION ***/
    CustCodeDurationOneIndex = ValidateHeader_Mode0(LevelArray, DurationArray, 2*SymbolsAvailable);
    if(CustCodeDurationOneIndex == 0)
    {
            *NumDecoded_p = 0;
            return ST_NO_ERROR;
    }


    /* Convert remaining durations into high/low levels */
    /* Index 0 contains toggle bit and index 1 contains bit 0 of customer code*/
    j = 3;
    for(i=CustCodeDurationOneIndex; i<(2*SymbolsAvailable); i++)
    {
        if(DurationArray[i] == 1)
        {
            LevelArray[j] = !LevelArray[j-1];
            j = j+1;
        }
        else if(DurationArray[i] == 2)
        {
            LevelArray[j] = LevelArray[j+1] = !LevelArray[j-1];
            j = j+2;
        }
        else if(DurationArray[i] == 0xFF) /* end of stream */
        {
            if(IsOdd(j))          /* an even number of levels are required */
            {
                LevelArray[j] = 0;
                j = j+1;
            }
        }
        else
        {
            *NumDecoded_p = 0;
            return ST_NO_ERROR;
        }
    }

    LevelArraySize = j;

    /* check remaining number of levels is 2*(custcode size + payloadsize + Toggle bit) */
    if (LevelArraySize != 2*(8 + ProtocolParams_p->RC6M0.NumberPayloadBits + 1))
    {
        *NumDecoded_p = 0;
        return ST_NO_ERROR;
    }

    /*** CUSTOM CODE VALIDATION ***/
    if(!ProtocolParams_p->RC6M0.DisableAddressCheck)
    {
        U8 CustCode=0;

        /* Parse this array of levels and output data */
        /* Take each pair of levels and determine bit from transition */
        /* first 2 bit will not be checked as these are toggle bit */
        for (j=2; j<18; j=j+2)
        {
            CustCode <<= 1;
            CustCode |= LevelArray[j];  /* only need to check the level of the first half-bit */
        }

        if (CustCode != ProtocolParams_p->RC6M0.CustomerCode )
        {
            *NumDecoded_p = 0;
            return ST_NO_ERROR;
        }
    }

    /*** PAYLOAD ENUMERATION ***/

    /* Parse this array of levels and output data */
    /* Take each pair of levels and determine bit from transition */
    for (j=0; j<LevelArraySize; j=j+2)
    {
        DecodedValue <<= 1;
        DecodedValue |= LevelArray[j];  /* is this robust enough? */
    }

    *NumDecoded_p = 1;
    *UserBuf_p = DecodedValue;
    return ST_NO_ERROR;
}

/*****************************************************************************
BLAST_RC6AEncode()
Description:
    Encode user buffer into rc5 symbols

Parameters:
    UserBuf_p           user buffer to encode
    UserBufSize         how many data words present
    SymbolBuf_p         symbol buffer to fill
    SymbolBufSize       symbol buffer size
    SymbolsEncoded_p    how many symbols we generated
    ProtocolParams_p    protocol params

Return Value:
    ST_NO_ERROR

See Also:
    BLAST_SpaceEncode()
    BLAST_RC6ADecode()
*****************************************************************************/

ST_ErrorCode_t BLAST_RC6AEncode_Mode0(const U32                  *UserBuf_p,
		                              STBLAST_Symbol_t           *SymbolBuf_p,
		                              U32                        *SymbolsEncoded_p,
		                              const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U32 SymbolCount = 0;
    int i;
    U32 Data = 0;
    BOOL LastUpdatedMark;

    *SymbolsEncoded_p = 0;              /* Reset symbol count */

    /**************************************************************
     * The format of an RC6A command is as follows:
     *
     * | H,S,M,T | C[8] | P[20]
     *
     * H,M.S,T - Header, Start, Mode, Trailer bits(Toggle is not implmented)
     * C       - Customer code
     * P       - Payload
     */


    /* generate header+startbit, mode0, trailer */
        
    GenerateSymbol( MID_6T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    GenerateSymbol( MID_1T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    GenerateSymbol( MID_1T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    GenerateSymbol( MID_1T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    /* Genrate Trailer bit(Toggle bit) as 0 */
    GenerateSymbol( MID_1T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;

    /* generate the end of the trailer (2T) plus the first bit of the cust code */
    if((ProtocolParams_p->RC6M0.CustomerCode & (1 << (7))) != 0)
    {
        /* Bit = 1 */
        GenerateMark( MID_3T, SymbolBuf_p);
        LastUpdatedMark = TRUE;
    }
    else
    {
        /* Bit = 0 */
        GenerateMark( MID_2T, SymbolBuf_p);
        LastUpdatedMark = TRUE;
    }

    /* compile the rest of the bits (typically 16) into a U32 Data*/
    /* Customercode */
    Data = (U32)ProtocolParams_p->RC6M0.CustomerCode;
    Data <<= ProtocolParams_p->RC6M0.NumberPayloadBits;

    /* Payload - before ORing with Data, ensure only NumberPayloadBits of data remain in *UserBuf */
    Data |= ((*UserBuf_p << (32-ProtocolParams_p->RC6M0.NumberPayloadBits)) >> (32-ProtocolParams_p->RC6M0.NumberPayloadBits) );


    /* generate end of trailer and customer code */
    for(i = 8+ProtocolParams_p->RC6M0.NumberPayloadBits-1; i > -1; i--)
    {
        if((Data & (1 << (i))) != 0)
        {
            /* Bit = 1 */
            if(LastUpdatedMark)
            {
                ExtendMark(MID_1T, SymbolBuf_p);
                GenerateSpace(MID_1T, SymbolBuf_p);
                LastUpdatedMark = FALSE;
            }
            else
            {
                SymbolBuf_p++;
                SymbolCount++;
                GenerateSymbol(MID_1T, MID_1T, SymbolBuf_p);
                LastUpdatedMark = FALSE;
            }
        }
        else
        {
             /* Bit = 0 */
            if(LastUpdatedMark)
            {
                GenerateSpace(MID_1T, SymbolBuf_p);
                SymbolBuf_p++;
                SymbolCount++;
                GenerateMark(MID_1T, SymbolBuf_p);
                LastUpdatedMark = TRUE;
            }
            else
            {
                ExtendSpace(MID_1T, SymbolBuf_p);
                SymbolBuf_p++;
                SymbolCount++;
                GenerateMark(MID_1T, SymbolBuf_p);
                LastUpdatedMark = TRUE;
            }
        }
    }

    SymbolCount++;

    /* Set symbols encoded count */
    *SymbolsEncoded_p = SymbolCount;

    /* Can't really go wrong with this */
    return ST_NO_ERROR;
}

/* EOF */
