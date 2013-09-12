/*****************************************************************************
File Name   : ruwido.c

Description : STBLAST ruwido protocol routines

Copyright (C) 2005 STMicroelectronics

History     : Created June 2005

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#include "stdio.h"
#endif

#include "stddefs.h"
#include "stblast.h"
#include "stcommon.h"
#include "blastint.h"
#include "ruwido.h"

#if defined(IR_INVERT)
#include "invinput.h"
#endif

/* The allowed drift is defined to be 25% (1/4) */
#define DRIFT_THRESHOLD         4


#define SYMBOL_TIMEOUT          10000
#define RUWIDO_MAX_FRAME_SIZE   44
#define FRAMETYPE_BIT_START     7
#define FRAMETYPE_BIT_END       8
#define COMMAND_LENGTH          10

/* macros */
#define MICROSECONDS_TO_CARRIERPULSES(f, m)   ( ((m)*(f))/1000000 )
#define GenerateMark(mark, Buffer_p,SubCarrierFreq) \
        ( Buffer_p->MarkPeriod = MICROSECONDS_TO_CARRIERPULSES((SubCarrierFreq), (mark)) )

#define GenerateSymbol(symbol, Buffer_p,SubCarrierFreq) \
        ( Buffer_p->SymbolPeriod =  MICROSECONDS_TO_CARRIERPULSES((SubCarrierFreq), (symbol)) )

#define GenerateValue(Value,SubCarrierFreq) \
        ( MICROSECONDS_TO_CARRIERPULSES((SubCarrierFreq), (Value)) )



static BOOL IsExtended(STBLAST_Symbol_t Symbol, BOOL MarkOrSpace, const U16 HalfLevelDuration );

                         
enum
{
    MARK,
    SPACE
};

enum
{
    FRAME_TYPE_KEYBOARD,
    FRAME_TYPE_MOUSE,
    FRAME_TYPE_REMOTE,
    FRAME_TYPE_ERROR
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
    return ((LocalPeriod > HalfLevelDuration) ? 1 : 0 ); /* greater than 1.5 times allowed value */

      
}


/*****************************************************************************
BLAST_RuwidoDecode()
Description:
    This routine will decode a shift coded packet to a number representing
    the 
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
    BLAST_ShiftEncode
*****************************************************************************/
ST_ErrorCode_t BLAST_RuwidoDecode(U32                  *UserBuf_p,
	                              STBLAST_Symbol_t     *SymbolBuf_p,
	                              U32                  SymbolsAvailable,
	                              U32                  *NumDecoded_p,
	                              U32                  *SymbolsUsed_p)
	{
    U8  j;
    U8  PayloadHalfLevel;
    U16 HalfLevelDuration;
    U32 SymbolIndex=0;
    U32 Data,data_index=0;
    U32 SymbolsUsed;
    U8  Up,Down;
    BOOL TransitionArray[RUWIDO_MAX_FRAME_SIZE] = {0}; /* only one BufferElementPerPayload is supported so this can be a fixed value */
    BOOL ExtendedTrace;   
    /* Frame Type */
    U8 FrameType = 0;
    U8 BitstoByte = 0;
    *NumDecoded_p = 0;    
    
    /* This is for Ruwido */
    HalfLevelDuration = RUWIDO_HALF_LEVEL_DURATION_MAX;        
    
#if defined (IR_INVERT) /*TBC */
    Down = 0;   /* define the relationship between    */
    Up   = 1;   /* transition direction and bit value */
#else
    Down = 1;   /* define the relationship between    */
    Up   = 0;   /* transition direction and bit value */
#endif
    
    /* We've now found a starting symbol, so let's decode it
      to a manchester-coded bitmask.*/
    /* traverse the BIT ARRAY, setting bits as we go */
    /* Bit 0 */
    
    PayloadHalfLevel = 1; /* the start of the symbol and START of the bit-period must coincide*/ 
    SymbolIndex = 0; 
   
    TransitionArray[0] = Down;
    ExtendedTrace = IsExtended(SymbolBuf_p[SymbolIndex], SPACE, HalfLevelDuration);

    {
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
                    if (!ExtendedTrace)
                    {
                        if (SymbolBuf_p[SymbolIndex].SymbolPeriod >= SYMBOL_TIMEOUT) /* Time Out Value */
                        {
                            break;
                        }
                    }
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
                    if (SymbolBuf_p[SymbolIndex].SymbolPeriod >= SYMBOL_TIMEOUT) /* Time Out Value */
                    {
                        break;
                    }
                }
                else
                {
                    TransitionArray[j] = Up;
                    ExtendedTrace = IsExtended(SymbolBuf_p[++SymbolIndex], MARK, HalfLevelDuration);
                }
            }
        }
    
        data_index = j; 
        Data = 0;
        
        if (data_index > COMMAND_LENGTH + 1 )
        {
        
            for ( j = FRAMETYPE_BIT_START; j <= FRAMETYPE_BIT_END; j++)
            {
                FrameType <<= 1;
                FrameType |= TransitionArray[j];
            }
         
            /* 1st word contains 10bit command */
            for (j = 0; j < COMMAND_LENGTH; j++)
            {
                Data <<= 1;
                Data |= (TransitionArray[j]);
            }
         
            /* Set the user data element */
            *UserBuf_p = Data;
            UserBuf_p++;  
            (*NumDecoded_p)++;  
            
              
            /* return successive data bytes */
            switch ( FrameType )
            {        
                case FRAME_TYPE_KEYBOARD: 
                {
                    Data = 0;
              
                    /* Check CB bit */
                    if ( TransitionArray[data_index] != ( !TransitionArray[data_index-1]))
                    {                   
                        *NumDecoded_p = 0;    	   
                    }
                    else
                    {
                        for (j = COMMAND_LENGTH ; j < data_index; j++)
                        {
                            Data <<= 1;
                            Data |= (TransitionArray[j]);
                            BitstoByte++;
                            if ( BitstoByte == 8 )
                            {
                                /* Set the user data element */
                                *UserBuf_p = Data;
                                UserBuf_p++;   
                                (*NumDecoded_p)++;   
                                Data = 0;
                                BitstoByte = 0;	
                            }
                        }  
                    } /* CB check ended */                  
          
              }
              break;
      
              case FRAME_TYPE_REMOTE: 
              case FRAME_TYPE_ERROR: 
              {        
               
                    Data = 0;
                    for (j = COMMAND_LENGTH ; j <= data_index; j++)
                    {
                        Data <<= 1;
                        Data |= (TransitionArray[j]);
                        BitstoByte++;
                        if ( BitstoByte == 8 )
                        {
                            /* Set the user data element */
                            *UserBuf_p = Data;
                            UserBuf_p++;   
                            (*NumDecoded_p)++;   
                            Data = 0;
                            BitstoByte = 0;	
                        }
                    }  
                }                   
                break;
                   
                case FRAME_TYPE_MOUSE:
                default:
                    *NumDecoded_p = 0;
                    break;
                
            }       
        
        }
        else if ( data_index == COMMAND_LENGTH + 1 )
        {
            
            /* 1st word contains 10bit command */
            for (j =0; j < COMMAND_LENGTH; j++)
            {
                Data <<= 1;
                Data |= (TransitionArray[j]);
            }
            
            /* Set the user data element */
            *UserBuf_p = Data;
            UserBuf_p++;  
            (*NumDecoded_p)++;  
        	
        }
        else
        {
            *NumDecoded_p = 0;
        }
     
        if ( *NumDecoded_p > 0 )    
        {   
            /* Store the total number of symbols used for single decode */
            SymbolsUsed     = SymbolIndex+1;
    
            /* Increment symbols used */
            *SymbolsUsed_p += SymbolsUsed;

        }
    
    }

    return ST_NO_ERROR;
}
/* EOF */
