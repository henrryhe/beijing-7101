/*****************************************************************************
File Name   : rc6a.c

Description : STBLAST RC6A encoding/decoding routines

Copyright (C) 2002 STMicroelectronics

History     : Created April 2002

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif
#include "stddefs.h"
#include "stblast.h"
#include "blastint.h"
#include "lowlatency.h"

#if defined(IR_INVERT)
#include "invinput.h"
#endif

/* One bit period is defined to be 600 us */
#define T (600)


/* used for decoding */
#define MAX_50T 32500  /* 650 * 50 */
#define MIN_50T 27500  /* 550 * 50 */
#define MAX_10T  6800  /* 650 * 10 */ 
#define MIN_10T  5500  /* 550 * 10 */
#define MAX_7T   4650  /* 650 * 7 */  
#define MIN_7T   3850  /* 550 * 7 */ 
#define MAX_5T   3250  /* 650 * 5 */
#define MIN_5T   2750  /* 550 * 5 */
#define MAX_4T   2700  /* 650 * 4 */ 
#define MIN_4T   2200  /* 550 * 4 */
#define MAX_3T   2000  /* 650 * 3 */ 
#define MIN_3T   1650  /* 550 * 3 */
#define MAX_2T   1500  /* 750 * 2  */
#define MIN_2T    900  /* 450 * 2  */
#define MAX_1T    850  /* For 1T the range is from 450 to 850 is taken */
#define MIN_1T    450

 /* used for encoding */
#define MID_1T   (U16)600
#define MID_2T   (U16)1200
#define MID_3T   (U16)1800
#define MID_4T   (U16)2400
#define MID_5T   (U16)3000
#define MID_7T   (U16)4200
#define MID_10T  (U16)6000
#define MID_12T  (U16)7200
#define MID_50T  (U16)30000 

#define SubCarrierFreq (U32)38000

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
U8 DurationTransform2_LowLatency(U16 Value);
void DurationTransform_LowLatency(STBLAST_Symbol_t* SymbolBuf_p,
                        const U32 SymbolsAvailable, 
                        U8 * DurationArray);

U8 ValidateHeader_LowLatency( U8 *DurationArray);

/***************************** WORKER ROUTINES  *****************************/

void DurationTransform_LowLatency(STBLAST_Symbol_t* SymbolBuf_p,
                        const U32 SymbolsAvailable, 
                        U8 * DurationArray)
{
    U32 i; /* changed from U32 to int to remove lint warning*/
    
    for (i=0; i<SymbolsAvailable; i++)
    {
        /* Symbol value = mark + Space */ 
        *DurationArray = DurationTransform2_LowLatency( SymbolBuf_p->SymbolPeriod);
        DurationArray++;
        
        /* Mark Value */
        *DurationArray = DurationTransform2_LowLatency( SymbolBuf_p->MarkPeriod );
        DurationArray++;
        
        SymbolBuf_p++;         
    }

}

U8 DurationTransform2_LowLatency(U16 Value)
{    
    if(Value >MAX_50T)
    {
        return 0xFF; /* time out value */
    }
    else if( Value > LOW_MAX_12T)
    {
        if(Value > MIN_50T)
        {
            return 50;
        }
        else
        {
            return 0;
        }
    }
    
    else if(Value > MAX_10T)
    {
       if(Value > LOW_MIN_12T)
       {
            return 12;
       }
       else
       {
            return 0;
       }
    }
    
    else if(Value > MAX_7T)
    {
        if(Value > MIN_10T)
        {
            return 10;
        }
        else
        {
            return 0;
        }
    }
    
    else if(Value > MAX_5T)
    {
        if(Value > MIN_7T)
        {
            return 7;
        }
        else
        {
            return 0;
        }
    }
    
    else if(Value > MAX_4T)
    {
        if(Value > MIN_5T )
        {
            return 5;
        }
        else
        {
            return 0;
        }
    }
    
    else if (Value > MAX_3T)
    {
        if(Value > MIN_4T)
        {
            return 4;
        }
        else
        {
            return 0;        
        }
    }
    
    else if (Value > MAX_2T)
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
    
    else if (Value > MAX_1T)
    {
        if(Value > MIN_2T)
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


/*  this function validates the header in the symbols */
U8 ValidateHeader_LowLatency(U8 *DurationArray)
{
    
    U8 DurationArrayCounter;
    
    DurationArrayCounter=0;
    
    
     /* Symbol value would be 12 or 7 depending upon the frame 
       For Initial frmae it will be 12 for subsequent frmae it will 
       be 7
    */
    if(DurationArray[DurationArrayCounter] != 12 && DurationArray[DurationArrayCounter] != 7)
    {
        return 0;
    }
    
    else
    {
       DurationArrayCounter++;
    }
    
    /* AGC burst is 10T on initial frame , 5T on Subsequent frames */
    if(DurationArray[DurationArrayCounter] !=10 && DurationArray[DurationArrayCounter] !=5)
    {
        return 0;
    }     
   
    else
    {
        DurationArrayCounter++;
        return DurationArrayCounter;
    }
}
 


/*****************************************************************************
BLAST_LowLatencyDecode()
Description:
    Decode LowLatency symbols into user buffer

Parameters:
    UserBuf_p           user buffer to place data into
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

ST_ErrorCode_t BLAST_LowLatencyDecode(U32                    *UserBuf_p,
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
    U8 CustCodeDurationOneIndex;

    /* generate correct interpretation from inverted IR receive input */
#if defined (IR_INVERT)
    {
        STBLAST_Symbol_t *LastSymbol_p;
        /* if the key is remained prees for 5518 board
           we will get only one Cmd coz in that case first symbol
           will be 5T */
        if (InvertedInputCompensate(SymbolBuf_p,
                                (MID_10T),
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

    /* Set number of symbols allowed */
    SymbolsLeft = SymbolsAvailable;
    *SymbolsUsed_p = 0;
    *NumDecoded_p = 0;
    
    
    /* parse symbol buffer with a ticker.  Each segment is of duration 2T */    
    /* Generate array of durations as multiples of T */    
    DurationTransform_LowLatency(SymbolBuf_p, SymbolsAvailable, DurationArray);


    /*** HEADER VALIDATION ***/  
    CustCodeDurationOneIndex = ValidateHeader_LowLatency(DurationArray);
    if(CustCodeDurationOneIndex == 0)
    {
            *NumDecoded_p = 0;
            return ST_NO_ERROR;
    }
   
           
    /* Convert remaining durations Bit Values  1T/2T-> 00 , 2T/3T-> 10 , 1T/3T-> 01 , 2T/4T -> 11*/    
    /* loop is executed till (SymbolsAvailable-1) as Header symbol is not needed */
    j = 0;
    for(i=CustCodeDurationOneIndex; i<(2*(SymbolsAvailable-1)); i++, j=j+2)
    {
        /* Symbol period 2*/
        if(DurationArray[i] == 2)
        {
            i++;
            if(DurationArray[i] == 1)
            {
                LevelArray[j] = 0;
                LevelArray[j+1] = 0;    
            }
            else
            {
                *NumDecoded_p = 0;
                return ST_NO_ERROR;
            }
        }
        /* Symbol Period 3 either 01 or 10*/
        else if(DurationArray[i] == 3)
        {
            i++;
            if(DurationArray[i] == 1)
            {
                LevelArray[j] = 0;
                LevelArray[j+1] = 1;
            }
            else if(DurationArray[i] == 2) 
            {
                LevelArray[j] = 1;
                LevelArray[j+1] = 0;
            }
            else
            {
                *NumDecoded_p = 0;
                return ST_NO_ERROR;        
            }
        }
        
        /* Symbol Period 4 so 11 */
        else if (DurationArray[i] == 4)
        {
            i++;
            if(DurationArray[i] == 2)
            {
                LevelArray[j] = 1;
                LevelArray[j+1] =1;
            }
            else
            {
                *NumDecoded_p = 0;
                return ST_NO_ERROR;        
            }
        }
        else if(DurationArray[i] == 50 || DurationArray[i] == 0xff)
        {
            break ;
        }
        
        else
        {
            *NumDecoded_p = 0;
            return ST_NO_ERROR;            
        }
         
     }
             
    LevelArraySize = j;
    
    /* check remaining number of levels is 2*(custcode size + payloadsize) */    
    if (LevelArraySize != (ProtocolParams_p->LowLatency.NumberPayloadBits + LOW_LATENCY_SYSTEM_BITS + LOW_LATENCY_CHECKSUM_BITS))
    {
        *NumDecoded_p = 0;
        return ST_NO_ERROR;
    } 

    /*** SYSTEM CODE VALIDATION ***/
    {
        
        U8 SystemBits = 0;
        
        for(i=0; i < LOW_LATENCY_SYSTEM_BITS ; i++)
        {
           SystemBits <<= 1;                
           SystemBits |= LevelArray[i];  /* only need to check 4 bits only */
        }
                
        if (SystemBits != ProtocolParams_p->LowLatency.SystemCode )
        {
            *NumDecoded_p = 0;
            return ST_NO_ERROR;        
        }
    }


    
    
    {
        U16 temp;
        U16 temp11,temp12,temp13,temp14;
        U8 Cmd = 0,ChkSum=0;
        
        /* Form the Cmd */
        for(i= LOW_LATENCY_SYSTEM_BITS; i < (ProtocolParams_p->LowLatency.NumberPayloadBits + LOW_LATENCY_SYSTEM_BITS) ; i++)
        {
            Cmd <<= 1;
            Cmd |= LevelArray[i];
        }
        
        /* Form the check Sum */
        j = (LOW_LATENCY_SYSTEM_BITS +ProtocolParams_p->LowLatency.NumberPayloadBits);
        for(i = j ; i < ( j + LOW_LATENCY_CHECKSUM_BITS); i++)
        {
            ChkSum <<=1;
            ChkSum |= LevelArray[i];
        
        }
        
        temp11 = Cmd & 0x3;
        temp12 = (Cmd >> 2) & 0x3;
        temp13 = (Cmd >> 4) & 0x3;
        temp14 = (Cmd >> 6) & 0x3;
        
        temp = (temp11*1) + (temp12*3) + (temp13*5) + (temp14*7) ;
        
        /* Taking only last 4 bit in to account */
        
        temp = temp & 0x000f;
        
        if (temp != ChkSum)
        {
             *NumDecoded_p = 0;
             return ST_NO_ERROR;   
        }
        
         
    *UserBuf_p = Cmd;
    *NumDecoded_p = 1;
    return ST_NO_ERROR;
    }
    
}


/*****************************************************************************
BLAST_LowLatencyEncode()
Description:
    Encode user buffer into Low latency symbols

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
    BLAST_LowLatencyDecode()
*****************************************************************************/

ST_ErrorCode_t BLAST_LowLatencyEncode(const U32                  *UserBuf_p,
                                        STBLAST_Symbol_t         *SymbolBuf_p,
                                        U32                      *SymbolsEncoded_p,
                                        const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U32 SymbolCount = 0;
    int i;
        
    *SymbolsEncoded_p = 0;              /* Reset symbol count */

    /**************************************************************
     * The format of an Low Latency command is as follows:
     *
     * | Header| System bits |Command | checksum
     *
     */

    /* generate header  bit */    
        
    GenerateSymbol(MID_10T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    
    /* Compile the System Bits (typically 4) */
    {
        U8 Currentbits;
        U8 temp=0xC,Cmd;
        U32 BitChkSum = 0;
        U8  SystemCode;
        
        SystemCode = (ProtocolParams_p->LowLatency.SystemCode) & 0xf; /* Only last 4 bits are required */
        
        for(i=0; i< LOW_LATENCY_SYSTEM_BITS; i=i+2)
        {
            Currentbits = ((SystemCode & temp)>>(LOW_LATENCY_SYSTEM_BITS - i - 2));
            temp >>=2;
            switch(Currentbits)
            {
            
                case 0:
                    GenerateSymbol( MID_1T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                    break;
                
                case 1:
                    GenerateSymbol( MID_1T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++; 
                    break;
                   
                case 2:
                    GenerateSymbol( MID_2T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                    break;
                   
                case 3:
                    GenerateSymbol( MID_2T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++; 
                    break;
            }
 
        
        }
        temp=0xC0;
        Cmd = *UserBuf_p;
        for(i=0 ; i<ProtocolParams_p->LowLatency.NumberPayloadBits; i=i+2)
        {
            Currentbits = ((Cmd & temp)>>(ProtocolParams_p->LowLatency.NumberPayloadBits - i - 2));
            temp >>=2;
            switch(Currentbits)
            {
            
                case 0:
                    GenerateSymbol( MID_1T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                    break;
                
                case 1:
                    GenerateSymbol( MID_1T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++; 
                    break;
                   
                case 2:
                    GenerateSymbol( MID_2T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                    break;
                   
                case 3:
                    GenerateSymbol( MID_2T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++; 
                    break;
            }
            
            switch(i)
            {
                case 0:
                    BitChkSum += (Currentbits * 7);
                    break;
                    
                case 2:
                    BitChkSum += (Currentbits * 5);
                    break;
                    
                case 4:
                    BitChkSum += (Currentbits * 3);
                    break;
                
                case 6:
                    BitChkSum += (Currentbits * 1);
                    break; 
            
            }
            
        } /* End of For*/
        
        BitChkSum &= 0xF; /* Needed only last 4 bits */
        
        
        /* Compileing chksum bits */
        
        temp = 0xC;
        
        for(i=0 ; i<LOW_LATENCY_CHECKSUM_BITS; i=i+2)
        {
            Currentbits = ((BitChkSum & temp)>>(LOW_LATENCY_CHECKSUM_BITS - i - 2));
            temp >>=2;
            
            switch(Currentbits)
            {
            
                case 0:
                    GenerateSymbol( MID_1T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                    break;
                
                case 1:
                    GenerateSymbol( MID_1T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++; 
                    break;
                   
                case 2:
                    GenerateSymbol( MID_2T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                    break;
                   
                case 3:
                    GenerateSymbol( MID_2T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++; 
                    break;
            }
 
        
        } /* End of For*/
        
    }
              
    /* For End of frame */
    
    GenerateMark(MID_1T, SymbolBuf_p);           
                      
    SymbolCount++;
               
    /* Set symbols encoded count */
    *SymbolsEncoded_p = SymbolCount;

    /* Can't really go wrong with this */
    return ST_NO_ERROR;
}


/* EOF */
