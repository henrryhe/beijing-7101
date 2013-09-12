/*****************************************************************************
File Name   : rcrf8.c

Description : STBLAST RCRF8 encoding/decoding routines

Copyright (C) 2005 STMicroelectronics

History     :
Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif

#include "stddefs.h"
#include "stblast.h"
#include "blastint.h"
#include "rcrf8.h"


#define FEC_LOOKUP_TABLE_SIZE 300 /* 256 */
#define HALF_LEVEL_DURATION             100
#define MAX_DATA_LENGTH                 200
#define HEADER_START                     10
#define HEADER_END                       11
#define HEADER_LENGTH                    2
#define PREAMBLE_START                   0
#define PREAMBLE_END                     9
#define PREAMBLE_MAX_LEN                 10 /* max 2ms, 1bit period = 200us */
#define CHECKSUM_LENGTH                  4
#define WORD_SIZE                        8
#define RRA_START                        0
#define RRA_END                          7
#define MODE_START                       8
#define MODE_END                         10
#define CUSTOMER_ID_START                11
#define CUSTOMER_ID_END                  19
#define SUB_MODE_START                   20
#define SUB_MODE_END                     21
#define ADDRESS_START                    22
#define ADDRESS_END                      23
#define DATA_START                       24
#define DATA_END                         39

static BOOL IsExtended(STBLAST_Symbol_t Symbol, BOOL MarkOrSpace, const U16 HalfLevelDuration );
void EncodeData ( U32 SrcData, U8 BitsToEncode , U8* Data );                         
enum
{
    MARK,
    SPACE
};

enum
{
    SUB_MODE_TYPE_REMOTE,
    SUB_MODE_TYPE_MOUSE,
    SUB_MODE_TYPE_KEYBOARD,
    SUB_MODE_TYPE_FREE_DATA

};

/* LUT */
/* Lookup table */

static U32 FEC_EncodingLUT[16] =
{
0, /* 0 - 00 */
14,/* 1 - 1E */
13,/* 2 - 2D */
3, /* 3 - 33 */
11,/* 4 - 4B */
5, /* 5 - 55 */
6, /* 6 - 66 */
8, /* 7 - 78 */
7, /* 8 - 87 */
9, /* 9 - 99 */
10,/* A - AA */
4, /* B - B4 */
12,/* C - CC */
2, /* D - D2 */
1, /* E - E1 */
15 /* F - FF */
};

static U32 FEC_DecodingLUT[FEC_LOOKUP_TABLE_SIZE] =
{
0,/*0*/
0,
0,
-1,
0,
-1,/*5*/
-1,
8,
0,
-1,
-1, /*10*/
4,
-1,
2,
1,
-1,/*15*/
0,
-1,
-1,
3,
-1,/*20*/
5,
1,
-1,
-1,
9, /*25*/
1,
-1,
1,
-1,
1, /*30 */
1,
0,
-1,
-1,
3, /*35*/
-1,
2,
6,
-1,
-1,/*40, 0x28*/
2,
10,
-1,
2,
2,/*45, 0x2D*/
-1,
2,
-1,
3,
3,/*50,0x32*/
3,
11,
-1,
-1,
3,/*55, 0x37*/
7,
-1,
-1,
3,
-1,/*60,0x3C*/
2,
1,
-1,
0,
-1,/*65,0x41*/
-1,
4,
-1,
5,
6,/*70,0x46*/
-1,
-1,
4,
4,
4,/*75,0x4B */
12,
-1,
-1,
4,
-1,/*80,0x50*/
5,
13,
-1,
5,
5,/*85,0x55*/
-1,
5,
7,
-1,
-1,/*90,0x5A*/
4,
-1,
5,
1,
-1,/*95,0x5F*/
-1,
14,
6,
-1,
6,/*100,0x64*/
-1,
6,
6,
7,
-1,/*105,0x69 */
-1,
4,
-1,
2,
6,/* 110,0x6E */
-1,
7,
-1,
-1,
3, /* 115, 0x73*/
-1,
5,
6,
-1,
7, /* 120,0x78 */
7,
7,
-1,
7,
-1, /*125,0x7D*/
-1,
15,
0,
-1,
-1,/*130,0x82 */
8,
-1,
8,
8,
8, /*135,0x87 */
-1,
9,
10,
-1,
12,/*140,0x8C*/
-1,
-1,
8,
-1,
9,/*145,0x91*/
13,
-1,
11,
-1,
-1,/* 150, 0x96 */
8,
9,
9,
-1,
9, /* 155,0x9B */
-1,
9,
1,
-1,
-1,/* 160,0xA0 */
14,
10,
-1,
11,
-1,/* 165,0xA5 */
-1,
8,
10,
-1,
10,/* 170,0xAA*/
10,
-1,
2,
10,
-1,/* 175, 0xAF*/
11,
-1,
-1,
3,
11,/* 180, 0xB4*/
11,
11,
-1,
-1,
9,/* 185, 0xB9 */
10,
-1,
11,
-1,
-1,/* 190,0xBE */
15,
-1,
14,
13,
-1,/* 195, 0xC3*/
12,
-1,
-1,
8,
12,/* 200, 0xc8*/
-1,
-1,
4,
12,
12,/* 205,0xCD*/
12,
-1,
13,
-1,
13,
13,/* 210,0xD2*/
-1,
5,
13,
-1,
-1,/* 215, 0xD7*/
-1,
9,
13,
-1,
12,/* 220, 0xDC*/
-1,
-1,
15,
14,
14,/* 225,0xE1 */
-1,
14,
-1,
14,
6,/* 230,0xE6 */
-1,
-1,
14,
10,
-1,/*235,0xEB*/
12,
-1,
-1,
15,
-1,/*240, 0xF0*/
14,
13,
-1,
11,
-1,/*245,0xF5*/
-1,
15,
7,
-1,
-1,/*250,0xFA */
15,
-1,
15,
15,
15/*255,0x55 */
};
/*****************************************************************************
IsExtended
Description:
    Determines the number of halfbit periods that contributed to  

Parameters:
    Buf         the symbols to compare
    Ref         the 'reference' symbols to compare them against
    Count       how many symbols to compare

Return Value:
    TRUE        if symbols are basically the same
    FALSE       otherwise

See Also:
    BLAST_SpaceDecode
*****************************************************************************/
static BOOL IsExtended(STBLAST_Symbol_t Symbol, BOOL MarkOrSpace , const U16 HalfLevelDuration )
{
    U16 LocalPeriod;
    if (MarkOrSpace == MARK)
    {
        LocalPeriod = Symbol.MarkPeriod;
    }
    else  /* SPACE */ 
    {
        LocalPeriod = Symbol.SymbolPeriod - Symbol.MarkPeriod;
    }
        
    /* if >1.5 times expected, level is extended */
    return ((LocalPeriod > (HalfLevelDuration*3)/2) ? 1 : 0 ); /* greater than 1.5 times allowed value */
}

void EncodeData ( U32 SrcData, U8 BitsToEncode , U8* Data )
{
    
    int i = 0;
    U8 TmpData[128] = {0};
    
    for ( i = 0; i < BitsToEncode; i++ )
    {
        TmpData[i] = (SrcData >> i ) & 0x01;
    }
    
    for ( i = BitsToEncode-1; i >= 0; i--)
    {
        *(Data++) = TmpData[i];
    }
}

/*****************************************************************************
BLAST_RCRF8Decode()
Description:
    Decode RCRF8 symbols into user buffer

Parameters:
    UserBuf_p           user buffer to place data into
    SymbolBuf_p         symbol buffer to consume
    SymbolsAvailable    how many symbols present in buffer
    NumDecoded_p        how many data words we decoded
    SymbolsUsed_p       how many symbols we used
    RcProtocol_p        protocol block

Return Value:
    ST_NO_ERROR
************************************************************************/

ST_ErrorCode_t BLAST_RCRF8Decode(U32                  *UserBuf_p,
                                 STBLAST_Symbol_t     *SymbolBuf_p,
                                 const U32            SymbolsAvailable,
                                 U32                  *NumDecoded_p,
                                 U32                  *SymbolsUsed_p,
                                 const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U32 i;
    U32 SymbolsLeft,SymbolsUsed;
    U8  PayloadHalfLevel;
    U8  Up,Down;
    U32 SymbolIndex=0;
    U32 data_index=0;
    BOOL ExtendedTrace;
    U8  temp[MAX_DATA_LENGTH]= {0};
    U8  Data[MAX_DATA_LENGTH];
    S32 DataNibble[MAX_DATA_LENGTH] = {0},j = 0;
    U16  CompareData = 0;
    BOOL TransitionArray[MAX_DATA_LENGTH] = {0};
    U32 Bits = 0;
    U16 HalfLevelDuration;
    U32 Bits_In_Groups = 0,Number_Of_Groups =0;

    /* Set number of symbols allowed */
    SymbolsLeft = SymbolsAvailable;
    *SymbolsUsed_p = 0;
    *NumDecoded_p  = 0;

    /* Assuming MANCHESTER DECODING */
    HalfLevelDuration = HALF_LEVEL_DURATION;        
    Down = 1;   /* define the relationship between    */
    Up   = 0;   /* transition direction and bit value */
    
    /* We've now found a starting symbol, so let's decode it
       to a manchester-coded bitmask */
    /* traverse the BIT ARRAY, setting bits as we go */
    /* Bit 0 */
    
    PayloadHalfLevel = 1; /* the start of the symbol and START of the bit-period must coincide*/ 
    SymbolIndex = 0; 
   
    TransitionArray[0] = Down;
    ExtendedTrace = IsExtended(SymbolBuf_p[SymbolIndex], SPACE, HalfLevelDuration);

    /* remaining bits */
    for (j=1; (SymbolIndex < SymbolsAvailable) ; j++)
    {
        /* Remaining payload bits */
        if (TransitionArray[j-1] == Down)
        {                
            if (ExtendedTrace)
            {
                TransitionArray[j] = Up;
                ExtendedTrace = IsExtended(SymbolBuf_p[++SymbolIndex], MARK, HalfLevelDuration);
 
            }
            else
            {
                TransitionArray[j] = Down;
                ExtendedTrace = IsExtended(SymbolBuf_p[++SymbolIndex], SPACE, HalfLevelDuration);
                
            }
        }
        else /* last transition was Up */
        {
            if (ExtendedTrace)
            {
                TransitionArray[j] = Down;
                ExtendedTrace = IsExtended(SymbolBuf_p[SymbolIndex], SPACE, HalfLevelDuration);

            }
            else
            {
                TransitionArray[j] = Up;
                ExtendedTrace = IsExtended(SymbolBuf_p[++SymbolIndex], MARK, HalfLevelDuration);
            }
        }
    }
    
    data_index = j; 

    /* remove the redundant bits to get message */
    {
        
        /* Check the Preamble */
        {
            for (i = PREAMBLE_START; i <= PREAMBLE_END; i++)
            {
                CompareData = CompareData << 1 | TransitionArray[i];
            }

        }
        
        /* Check The Header */
        {
            for (i = HEADER_START; i <= HEADER_END; i++)
            {
                CompareData = CompareData << 1 | TransitionArray[i];
            }
        }

        Bits_In_Groups   = (data_index - HEADER_LENGTH - PREAMBLE_MAX_LEN);
        Number_Of_Groups = Bits_In_Groups/8;
         
        for ( i = 0 ; i < data_index ; i++)
        {
            TransitionArray[i] = TransitionArray[HEADER_LENGTH+PREAMBLE_MAX_LEN+i];	
            
        }
        data_index = 0;
        
        /* Perform De-interleaving */
        {
            for ( i=0 ; i < Number_Of_Groups; i++ )
            {
                for ( j=0 ; j < WORD_SIZE; j ++  )
                {
                    temp[data_index++] = TransitionArray[i+Number_Of_Groups*j];
                }
            }
        }
        
        data_index = 0;
        
        /* Decoding parity bits, perform FEC */
        {
            U8 CodeWord[MAX_DATA_LENGTH] = {0};
            U32 Sqr = 128;

            /* convert binary to decimal to get index of Look Up table */
            for ( i=0 ; i < Number_Of_Groups; i++ )
            {
                Sqr = 128;
                for ( j = 0; j < WORD_SIZE; j ++)
                {
                    if ( temp[data_index] != 0 )
                    {
                        CodeWord[i] += (temp[data_index] * Sqr);
                    }
                    data_index++;
                    Sqr = Sqr/2;
                }
                
            }

            /* for each group find the correct data from FEC LUT */
            for ( i = 0; i< Number_Of_Groups; i++ )
            {
                DataNibble[i] = FEC_DecodingLUT[CodeWord[i]];
                if ( DataNibble[i] == -1 )
                {
                    *NumDecoded_p = 0;
                    return ST_NO_ERROR;
                }
            }
        }

        /* Remove Checksum */
        {
            U32 CheckSum = 0;
            U32 Dec2Bin[MAX_DATA_LENGTH]  = {0};

            for ( i = 0; i< Number_Of_Groups-1 ; i++ )
            {
                 CheckSum += DataNibble[i];
            }
            
            /* Check sum = Sum of all nibbles (except last) modulo-16 */
            CheckSum = CheckSum % 16;

            if ( CheckSum != (U32)DataNibble[i++])
            {
                *NumDecoded_p = 0;
                return ST_NO_ERROR;
            }
            
            data_index = 0;
        
            /* convert nibble from decimal to binary */
            for ( i = 0; i< Number_Of_Groups-1 ; i++ )
            {
                if ( DataNibble[i] > 0 )
                {

                    for ( j = 0; j < 4 ; j++ )
                    {
                        if ( DataNibble[i] > 0 )
                        {
                            Dec2Bin[j]    = DataNibble[i] % 2;
                            DataNibble[i] = DataNibble[i] / 2;
                        }
                        else
                        {
                            Dec2Bin[j] = 0;
                        }	
                    }
         
                    /* reverse the binary value */
                    for ( j = 3; j >= 0 ; j-- )
                    {
                        Data[data_index++] = Dec2Bin[j];
                    }
                }
                else
                {
                    for ( j = 0; j < 4 ; j++ )
                    {
                        Data[data_index++] = 0;
                    }
                }
            }
        }

        /* Get Message - RRA = 8bits, mode = 3bits,customerid = 9bits, submode =2bits */
        /* Address =2bits ,data = 8bits & optional data = 8bits */
        {

            for ( i = RRA_START; i<= RRA_END ; i++ )
            {
                CompareData <<= 1;
                CompareData |= Data[i];
            }
            
            if ( ProtocolParams_p->RCRF8.RRA != CompareData )
            {
            	*NumDecoded_p = 0;
            
            }
            CompareData = 0;
            
            for ( i = MODE_START; i<= MODE_END ; i++ )
            {
                CompareData <<= 1;
                CompareData |= Data[i];
            } 
             
            if ( ProtocolParams_p->RCRF8.Mode != CompareData )
            {
            	*NumDecoded_p = 0;
            
            }
            CompareData = 0;
            
            for ( i = CUSTOMER_ID_START; i<= CUSTOMER_ID_END ; i++ )
            {
                CompareData <<= 1;
                CompareData |= Data[i];
            }
            if ( ProtocolParams_p->RCRF8.CustomerId != CompareData )
            {
            	*NumDecoded_p = 0;            
            }          
            CompareData = 0;

            for ( i = ADDRESS_START; i< ADDRESS_END ; i++ )
            {
                CompareData <<= 1;
                CompareData |= Data[i];
            }
            if ( ProtocolParams_p->RCRF8.Address != CompareData )
            {
            	*NumDecoded_p = 0;            
            }          
            CompareData = 0;
            
            for ( i = SUB_MODE_START; i<= SUB_MODE_END ; i++ )
            {
                CompareData <<= 1;
                CompareData |= Data[i];
            }
            if ( ProtocolParams_p->RCRF8.SubMode != CompareData )
            {
            	*NumDecoded_p = 0;            
            }          

            switch (CompareData)
            {
                case SUB_MODE_TYPE_REMOTE:
                case SUB_MODE_TYPE_KEYBOARD:
                {                        
                    for ( i = DATA_START; i <= DATA_END; i++)
                    {
                        Bits <<= 1;
                        Bits |= (Data[i]);                              
                    }        
                    *UserBuf_p++ = Bits;
                    (*NumDecoded_p)++;             
                }
                break;

                case SUB_MODE_TYPE_MOUSE:
                case SUB_MODE_TYPE_FREE_DATA:
                default:
                    *NumDecoded_p = 0;
                    break;

            }
        }
    }

    /* Store the total number of symbols used for single decode */
    SymbolsUsed     = SymbolIndex+1;
    
    /* Increment symbols used */
    *SymbolsUsed_p += SymbolsUsed;

    return ST_NO_ERROR;
}

/*****************************************************************************
BLAST_RCRF8Encode()
Description:
    Encode user buffer into RCRF8 symbols

Parameters:
    UserBuf_p           user buffer to encode
    UserBufSize         how many data words present
    SymbolBuf_p         symbol buffer to fill
    SymbolBufSize       symbol buffer size
    SymbolsEncoded_p    how many symbols we generated
    RcProtocol_p        protocol block

Return Value:
    ST_ERROR_FEATURE_NOT_SUPPORTED

See Also:
    BLAST_SpaceEncode()
    BLAST_RC5Decode()
*****************************************************************************/

ST_ErrorCode_t BLAST_RCRF8Encode(const U32                  *UserBuf_p,
                                 const U32                  UserBufSize,
                                 STBLAST_Symbol_t           *SymbolBuf_p,
                                 U32                        *SymbolsEncoded_p,
                                 const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U32 SymbolCount = 0, i, j, k;
    U32 SymbolPeriod, MarkPeriod;
    U32 count = 0,bits = 0,Nibble = 0,SumOfNibble =0,CheckSum = 0;
    U8 Data[MAX_DATA_LENGTH] = {0},TmpData[MAX_DATA_LENGTH] = {0},FECData[MAX_DATA_LENGTH] = {0};
    U8 BinToDec[MAX_DATA_LENGTH] = {0};
    U32 Coded[2*MAX_DATA_LENGTH] = {0};
    U8 Sqr = 8,Bit, Last;

    *SymbolsEncoded_p = 0;              /* Reset symbol count */
    
    /********************************************************************
    * The format of an RCRF8 command is as follows:
    *
    * | Preamble| Header | Message | SFT |  
    * 
    * Message is formed after interleaving, FEC & checksum of
    * |RRA(8)|Mode(3)|CustomId(9)|SB(2)|Address(2)|Data(8)|Op Data(8)|                    
    ********************************************************************/
    
    /* Form Message from Data bits - Add Address, Mode & RRA bits */
    for (i = 0; i < UserBufSize; i++, UserBuf_p++)
    {
        
        /* Add 8 RRA bits */   
        EncodeData ( ProtocolParams_p->RCRF8.RRA,8,&Data[count] );
        count= count + 8;
        
        /* Add 3 mode bits */
        EncodeData ( ProtocolParams_p->RCRF8.Mode,3,&Data[count] );
        count= count + 3;
        
        /* Add 9 custom id bits */
        EncodeData ( ProtocolParams_p->RCRF8.CustomerId,9,&Data[count] );
        count = count + 9;
        
        /* Add 2 bit submode */
        EncodeData ( ProtocolParams_p->RCRF8.SubMode,2,&Data[count] );
        count = count + 2;
        
        /* Add 2 bit address */
        EncodeData ( ProtocolParams_p->RCRF8.Address,2,&Data[count]);
        count = count + 2;
                
        switch(ProtocolParams_p->RCRF8.Mode)
        {
            case 1: 
                /* Copy user buffer */
                EncodeData (*UserBuf_p,16,&Data[count]);
                count = count + 16;
                break; 
                
            default:
               break;
        }
        
        
        /* Now add checksum to message- add all message nibble and take */
        /* modulo-16 fraction as check sum */
        /* RRA(8) mode(2) data(18) checksum(4)*/
        /* convert nibble from decimal to binary */
        for ( j = 0; j < count; j=j+4 )
        {
            for ( k = 0; k < 4 ; k++ )
            {
                if (Data[k+j] == 1)
                {
                    BinToDec[Nibble] += Sqr;
                }
                Sqr /= 2;
            }
            Sqr = 8;
            Nibble++;
        }
            
        for ( j = 0; j < Nibble; j++)
        {
            SumOfNibble += BinToDec[j];
        }
        
        CheckSum = SumOfNibble % 16; 
        
        for ( j = 0 ; j < Nibble ; j++ )
        {
            FECData[j] = FEC_EncodingLUT[BinToDec[j]];
        }
        
        /* Add FEC bits from lookup table */
        FECData[Nibble] = FEC_EncodingLUT[CheckSum];         
        
        /* Now form new buffer with FEC bits */      
        count = 0;
          
        for ( j = 0; j < Nibble; j++)
        {
            EncodeData ( BinToDec[j],4,&TmpData[count] );
            count = count + 4; 
            
            EncodeData ( FECData[j],4,&TmpData[count] );
            count = count + 4;  
        }
                
        EncodeData ( CheckSum,4,&TmpData[count] );
        count = count + 4; 
        
        EncodeData ( FECData[Nibble],4,&TmpData[count] );
        count = count + 4;          
            
        /* Add Preamble */
        for ( j = 0 ; j < 10 ; j++ )
        {
            Data[bits++] = 1;
        }
        
        /* Add 2 bits Header */
        Data[bits++] = 0;
        Data[bits++] = 0;
             
        /* Add interleaving bits */
        for ( j = 0 ; j < 8 ; j++ )
        {
            for ( k = 0; k < count/8; k++ )
            {
                Data[bits++] = TmpData[j+8*k];
            }        
        }
        
        /* Add SFT - signal free time , almost 6 bits */
        /*for ( j = 0 ; j < 7 ; j++ )
        {
            Data[bits++] = 0;
        }*/
        
        /**********************************************************
         * Generate manchester-coded bitmask of half-bit values:
         *
         * The following algorithm is used to calculate the coded
         * data:
         *
         * XOR(!NRZ_CLOCK, DATA)
         *
         * E.g.,
         *
         * NRZ_CLOCK    0 1  0 1  0 1  0 1  0 1  0 1  0 1  0 1
         * ---------------------------------------------------
         * DATA          1    0    0    0    1    1    1    0
         * ---------------------------------------------------
         * CODED        0 1  1 0  1 0  1 0  0 1  0 1  0 1  1 0
         *
         * Where NRZ_CLOCK is the half-bit clock and DATA is the
         * data to code.
         */
        count = 0;
        
        for(j = 0; j < bits; j++)       
        {
            /* Calculate bit value */
            Bit = Data[j];            

            /* First half-bit period */
            Coded[count++] = (Bit ^ 0);
            
            /* Second half-bit period */
            Coded[count++] = (Bit ^ 1);

        }

        /* Now generate symbol buffer from coded data - the data
         * is stored in 26-bits coded LSB.
         */
        Last = (U8)-1;
        SymbolPeriod = 0;
        MarkPeriod   = 0;

        for (j = 0; j < count; j++)
        {
            /* Check for low->high transition, as this means we must
             * start a new symbol.
             */
            Bit = Coded[j];
            if ( Last == 0 && Bit != 0 )
            {

                SymbolBuf_p->MarkPeriod   = MarkPeriod * 100;
                SymbolBuf_p->SymbolPeriod = SymbolPeriod * 100;

                /* Reset period counters */
                SymbolPeriod = 0;
                MarkPeriod   = 0;

                /* Next symbol */
                SymbolBuf_p++;
                SymbolCount++;
            }

            /* Check for mark or space */
            if (Bit != 0)
            {
                MarkPeriod++;
            }
            
            SymbolPeriod++; /* Always inc symbol period */

            /* Only store last bit if this is not the first
             * time around the loop.
             */
            if (j > 0)
            {
                Last = Bit;  /* Store last bit */
            }
            
            if ( j == count-1 && Bit == 0 )
            {
                SymbolBuf_p->MarkPeriod   = MarkPeriod * 100;
                SymbolBuf_p->SymbolPeriod = SymbolPeriod * 100;
                
                /* Next symbol */
                SymbolBuf_p++;
                SymbolCount++;
            }
            
        }
    }

    /* Set symbols encoded count */
    *SymbolsEncoded_p = SymbolCount;  
    
    /* Can't really go wrong with this */
    return ST_NO_ERROR;   

}
/* EOF */
