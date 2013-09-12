/*****************************************************************************
File Name   : lowlatencyPro.c

Description : STBLAST Low Latency Pro decoding routines.
			  This protocols is meant for RF

Copyright (C) 2002 STMicroelectronics

History     : Created June 2007
Author      : Megha Gupta
Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif
#include "stddefs.h"
#include "stblast.h"
#include "blastint.h"
#include "lowlatencyPro.h"

#if defined(IR_INVERT)
#include "invinput.h"
#endif

/* One bit period is defined to be 600 us */
#define T (600)

/* used for decoding */
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
#define MIN_4T   2180  /* 550 * 4 */
#define MAX_3T   2000  /* 650 * 3 */ 
#define MIN_3T   1590  /* 550 * 3 */
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
#define SubCarrierFreq (U32)56000

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
U8 DurationTransform2_LowLatencyPro(U16 Value);
void DurationTransform_LowLatencyPro(STBLAST_Symbol_t* SymbolBuf_p,
                        const U32 SymbolsAvailable, 
                        U8 * DurationArray);

/***************************** WORKER ROUTINES  *****************************/

void DurationTransform_LowLatencyPro(STBLAST_Symbol_t* SymbolBuf_p,
                        const U32 SymbolsAvailable, 
                        U8 * DurationArray)
{
    U32 i; /* changed from U32 to int to remove lint warning*/
    
    for (i=0; i<SymbolsAvailable; i++)
    {
        /* Symbol value = mark + Space */ 
        if(SymbolBuf_p->SymbolPeriod != 1)
        {
        	*DurationArray = DurationTransform2_LowLatencyPro( SymbolBuf_p->SymbolPeriod);
             DurationArray++;
        }
        if(SymbolBuf_p->MarkPeriod != 1)
        {
	        /* Mark Value */
	        *DurationArray = DurationTransform2_LowLatencyPro( SymbolBuf_p->MarkPeriod );
       		 DurationArray++;
        }
        SymbolBuf_p++;         
    }

}

U8 DurationTransform2_LowLatencyPro(U16 Value)
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

/*****************************************************************************
BLAST_LowLatencyProDecode()
Description:
    Decode LowLatency symbols into user buffer

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
ST_ErrorCode_t BLAST_LowLatencyProDecode(U32                  *UserBuf_p,
                                         STBLAST_Symbol_t     *SymbolBuf_p,
                                         U32                  SymbolsAvailable,
                                         U32                  *NumDecoded_p,
                                         U32                  *SymbolsUsed_p,
                                         const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U8 i;
    U8 j;
    U8 k;
    U32 SymbolsLeft;
    U8 DurationArray[200];
    U8 LevelArraySize;
    U8 LevelArray[400];  /* check these values */

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
    DurationTransform_LowLatencyPro(SymbolBuf_p, SymbolsAvailable, DurationArray);

    /*** HEADER VALIDATION ***/  
    /* no AGC check required */ 
          
    if(SymbolsAvailable < 18)
    {
    	  *NumDecoded_p = 0;
           return ST_NO_ERROR;
    }       
    else
    {
    	SymbolsAvailable = 18;
    }
    /* Convert remaining durations Bit Values  1T/2T-> 00 , 2T/3T-> 10 , 1T/3T-> 01 , 2T/4T -> 11*/    
    /* loop is executed till (SymbolsAvailable-1) as Header symbol is not needed */
    j = 0;
    for(i = 0;i < 32; i++, j=j+2)
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

    /* check remaining number of levels is 2*(custcode size + user code + long address payloadsize) */  
    /* 2bits corresponds to one symbol */

    if (LevelArraySize != (ProtocolParams_p->RFLowLatencyPro.NumberPayloadBits + 
    					   LOW_LATENCYPRO_LONG_ADDR_BITS + LOW_LATENCYPRO_CHECKSUM_BITS))
    {
        *NumDecoded_p = 0;
        return ST_NO_ERROR;
    }
    /* SYSTEM CODE and USER CODE VALIDATION not Required*/
    /*** LONG ADDRESS VALIDATION ***/
    {
        U8 LongAddr = 0;
    
        for(i = 0; i < LOW_LATENCYPRO_LONG_ADDR_BITS ; i++)
        {
           LongAddr <<= 1;                
           LongAddr |= LevelArray[i];  /* only need to check 4 bits only */
        }
                
        if (LongAddr != ProtocolParams_p->RFLowLatencyPro.LongAddress )
        {
            *NumDecoded_p = 0;
            return ST_NO_ERROR;        
        }
    }
    {
        U16 temp = 0;
        U16 temp2;
        U8 Cmd = 0,ChkSum=0;
        
        /* Form the Cmd */
        /* Level array subscript 28 - 36 is data */
        
        j = LOW_LATENCYPRO_LONG_ADDR_BITS;
        
        for(i= j ;i < (ProtocolParams_p->RFLowLatencyPro.NumberPayloadBits + j) ; i++)
        {
            Cmd <<= 1;
            Cmd |= LevelArray[i];
        }
        
        /* Form the check Sum */
        
        /* Check sum starts at i = 36 */
        j = ProtocolParams_p->RFLowLatencyPro.NumberPayloadBits + LOW_LATENCYPRO_LONG_ADDR_BITS;
        
        /* Transmitted check sum */            
        for(i = j ; i < ( j + LOW_LATENCYPRO_CHECKSUM_BITS); i++)
        {
            ChkSum <<=1;
            ChkSum |= LevelArray[i];
        }

        /* calculate checksum from data received */
        j = 0;
        temp2 = 0; 

        k =  ProtocolParams_p->RFLowLatencyPro.NumberPayloadBits + LOW_LATENCYPRO_LONG_ADDR_BITS;
            
        for(i = 0 ; i < k; i++)
        {
            temp <<= 1;
            temp |= LevelArray[i];
           /* Add on all 9 nibbles */     
            if(i == (j + 3))
            { 
            	temp2 += temp;
            	j = j + 4;
            	temp = 0;
            }
        }
        temp2 += ProtocolParams_p->RFLowLatencyPro.UserCode + \
        		 ProtocolParams_p->RFLowLatencyPro.SystemCode;
        temp2 = temp2 % 16;
        
        if (temp2 != ChkSum)
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
BLAST_LowLatencyProEncode()
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

ST_ErrorCode_t BLAST_LowLatencyProEncode(const U32                  *UserBuf_p,
                                         STBLAST_Symbol_t           *SymbolBuf_p,
                                         U32                        *SymbolsEncoded_p,
                                         const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U32 SymbolCount = 0;
    int i,j;
    U32 CalCheckSum[10];          
    
    *SymbolsEncoded_p = 0;              /* Reset symbol count */

    /**************************************************************
     * The format of an Low Latency command is as follows:
     *
     * | Header| System bits |Command | checksum
     *
     */

    /* generate header  bit */   
    /* generate AGC */ 
    GenerateSymbol(MID_10T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    
    /* Compile the System Bits (typically 4) */
    {
        U8 Currentbits;
        U8 temp=0xC,temp2,Cmd;
        U32 BitChkSum = 0;
        U8  SystemCode;
        U8  UserCode;
        U32 LongAddress;
        
        SystemCode = (ProtocolParams_p->RFLowLatencyPro.SystemCode) & 0xf; /* Only last 4 bits are required */
        
        for(i=0; i< LOW_LATENCYPRO_SYSTEM_BITS; i=i+2)
        {
            Currentbits = ((SystemCode & temp)>>(LOW_LATENCYPRO_SYSTEM_BITS - i - 2));
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
        CalCheckSum[0] = SystemCode & 0xF;
        
        temp=0xC;
        /* Compile the User code Bits (typically 4) */
        UserCode = (ProtocolParams_p->RFLowLatencyPro.UserCode) & 0xf; /* Only last 4 bits are required */
        
        for(i=0; i< LOW_LATENCYPRO_USER_BITS; i=i+2)
        {
            Currentbits = ((UserCode & temp)>>(LOW_LATENCYPRO_USER_BITS - i - 2));
            temp >>= 2;
            
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
        CalCheckSum[1] = UserCode & 0xF;
        temp=0xC;
        /* Compile the Long address Bits (typically 20) */
        LongAddress = (ProtocolParams_p->RFLowLatencyPro.LongAddress) & 0x000fffff; /* Only last 4 bits are required */
        
        for(i=0; i< LOW_LATENCYPRO_LONG_ADDR_BITS; i=i+2)
        {
            Currentbits = ((LongAddress  & temp)>>(LOW_LATENCYPRO_LONG_ADDR_BITS - i - 2));
            temp >>= 2;
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
        temp2 = LongAddress;
        
        for(i = 2,j = 0; j < 5; j++,i++ )
        {
        	CalCheckSum[i] = temp2 & 0xF;
        	temp2 >>= 4;
        }
        
        Cmd  = *UserBuf_p;
        temp =  0xC0;
        
        for(i=0 ; i<ProtocolParams_p->RFLowLatencyPro.NumberPayloadBits; i=i+2)
        {
            Currentbits = ((Cmd & temp )>>(ProtocolParams_p->RFLowLatencyPro.NumberPayloadBits - i - 2));
 			temp >>= 2;            
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
        
        CalCheckSum[7] = (Cmd >> 4) & 0xF;
        CalCheckSum[8] = Cmd & 0x0F;
        
        /* Add all 9 nibbles to calculate checksum */
        for(i = 0; i < 9 ; i++)
        {
        	BitChkSum += CalCheckSum[i];
        }
		/* Modulus 16 */
        BitChkSum = BitChkSum % 16;
        BitChkSum &= 0xF; /* Needed only last 4 bits */
        
   
        /* Compileing chksum bits */
        temp = 0xC;
        for(i=0 ; i<LOW_LATENCY_CHECKSUM_BITS; i=i+2)
        {
            Currentbits = ((BitChkSum & temp )>>(LOW_LATENCYPRO_CHECKSUM_BITS - i - 2));
            temp >>= 2;  
            
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


