/*****************************************************************************
File Name   : shift.c

Description : STBLAST shift encoding/decoding routines

Copyright (C) 2002 STMicroelectronics

History     : Created Feb 2003

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif

#include "stddefs.h"
#include "stblast.h"
#include "blastint.h"
#include "shift.h"

#if defined (IR_INVERT)
#include "invinput.h"
#endif

/* The allowed drift is defined to be 20% (1/5) */
#define DRIFT_THRESHOLD 5

/* macros */
#define MICROSECONDS_TO_CARRIERPULSES(f, m)   ( ((m)*(f))/1000000 )
#define GenerateMark(mark, Buffer_p,SubCarrierFreq) \
        ( Buffer_p->MarkPeriod = MICROSECONDS_TO_CARRIERPULSES((SubCarrierFreq), (mark)) )

#define GenerateSymbol(symbol, Buffer_p,SubCarrierFreq) \
        ( Buffer_p->SymbolPeriod =  MICROSECONDS_TO_CARRIERPULSES((SubCarrierFreq), (symbol)) )

#define GenerateValue(Value,SubCarrierFreq) \
        ( MICROSECONDS_TO_CARRIERPULSES((SubCarrierFreq), (Value)) )

/* declarations */
static BOOL ValidateHeader(const STBLAST_Symbol_t *Buf_p,
                           const STBLAST_Symbol_t *Ref_p,
                           const U16 HalfLevelDuration,
                           U8    *PayloadHalfLevel_p,
                           const U32 BufSize,
                           const U32 RefSize,
                           U32   *HeaderLastIndex_p
                           );

static BOOL SymbolsOk(const STBLAST_Symbol_t *Buf_p,
                      const STBLAST_Symbol_t *Ref_p,
                      const U32 RefSize,            /* the size of the Ref (header) array */
                      const U16 HalfLevelDuration,  /* the duration of each level surrounding a transition */
                      U8    *PayloadHalfLevel_p,    /* the level of the first payload bit */
                      U32   *HeaderLastIndex_p      /* the position of the last recognised header symbol within Buf */
                      );

static BOOL ValidateStop (const STBLAST_Symbol_t *Buf_p,
                          const STBLAST_Symbol_t *Ref_p,
                          const U32 RefSize,
                          U32   HeaderLastIndex,
                          U32   *HeaderLastIndex_p
                         );

static BOOL IsExtended(STBLAST_Symbol_t Symbol, BOOL MarkOrSpace, const U16 HalfLevelDuration );
                         
enum
{
    MARK,
    SPACE
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

/*****************************************************************************
ValidateHeader
Description:
    Verify that two buffers of symbols are identical (within certain drift
    limits)

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
static BOOL ValidateHeader(const STBLAST_Symbol_t *Buf_p,
                           const STBLAST_Symbol_t *Ref_p,
                           const U16 HalfLevelDuration,
                           U8    *PayloadHalfLevel_p,
                           const U32 BufSize,
                           const U32 RefSize,
                           U32   *HeaderLastIndex_p
                          )
{     
    U8 BufCounter;
    BOOL Result = FALSE;
  
    BufCounter = 0;
    while (BufCounter < BufSize-RefSize)
    {   
        Result=SymbolsOk(&Buf_p[BufCounter],Ref_p ,RefSize, HalfLevelDuration, PayloadHalfLevel_p, HeaderLastIndex_p);
        if(Result==TRUE)
        {
            /* header validated */
            break;
        }
        BufCounter++;
    }  
    return Result;
}


static BOOL ValidateStop (const STBLAST_Symbol_t *Buf_p,
                          const STBLAST_Symbol_t *Ref_p,
                          const U32 RefSize,
                          U32   HeaderLastIndex,
                          U32   *HeaderLastIndexOut_p  )
{     
    BOOL Okay = TRUE;
    U32 AllowedMarkDrift, AllowedSymbolDrift;
    U32 MarkDrift, SymbolDrift;
    U8 j;
    
    /* check if all but the last received StartSymbols are valid */
    for (j = 0; j < RefSize; j++)   /* all expected stop symbols */
    {
        AllowedMarkDrift = Ref_p[j].MarkPeriod / DRIFT_THRESHOLD;
        AllowedSymbolDrift = Ref_p[j].SymbolPeriod / DRIFT_THRESHOLD;
        MarkDrift = abs(Buf_p[HeaderLastIndex+j].MarkPeriod - Ref_p[HeaderLastIndex+j].MarkPeriod);
        SymbolDrift = abs(Buf_p[HeaderLastIndex+j].SymbolPeriod - Ref_p[HeaderLastIndex+j].SymbolPeriod);
       
        if((MarkDrift > AllowedMarkDrift) ||     /* outside permitted range */
            (SymbolDrift > AllowedSymbolDrift))
        {
            Okay = FALSE;
            break;
        }
    }
    
    if(Okay==TRUE)
    {
       *HeaderLastIndexOut_p=HeaderLastIndex+RefSize-1;
    }
    return Okay;
}

static BOOL SymbolsOk(const STBLAST_Symbol_t *Buf_p,
                      const STBLAST_Symbol_t *Ref_p,
                      const U32 RefSize,            /* the size of the Ref (header) array */
                      const U16 HalfLevelDuration,  /* the duration of each level surrounding a transition */
                      U8    *PayloadHalfLevel_p,    /* the level of the first payload bit */
                      U32   *HeaderLastIndex_p      /* the position of the last recognised header symbol within Buf */
                      )
{ 
    BOOL Okay = TRUE;
    U32 AllowedMarkDrift, AllowedSymbolDrift;
    U32 MarkDrift, SymbolDrift;
    U8 j;
    U32 Limit1,Limit2,Limit3,Limit4;

    for(j = 0; j < RefSize-1; j++)   /* all but the last header symbol */
    {
        AllowedMarkDrift = Ref_p[j].MarkPeriod / DRIFT_THRESHOLD;
        AllowedSymbolDrift = Ref_p[j].SymbolPeriod / DRIFT_THRESHOLD;
        MarkDrift = abs(Buf_p[j].MarkPeriod - Ref_p[j].MarkPeriod);
        SymbolDrift = abs(Buf_p[j].SymbolPeriod - Ref_p[j].SymbolPeriod);
       
        if((MarkDrift > AllowedMarkDrift) ||     /* outside permitted range */
            (SymbolDrift > AllowedSymbolDrift)
           )
        {
            Okay = FALSE;
            break;
        }
    }
    
    if(Okay == TRUE)
    {
        /* check if the final Symbol is valid */
        /* we have to cover two cases - the first payload half-level is LOW or HIGH */
           
        /*check the received symbol time in a range of f-.2f & f+.2d*/
        Limit1 = Ref_p[RefSize-1].SymbolPeriod - (Ref_p[RefSize-1].SymbolPeriod/DRIFT_THRESHOLD);
        Limit2 = Ref_p[RefSize-1].SymbolPeriod + (HalfLevelDuration/DRIFT_THRESHOLD);

        /*check the received symbol time in a range of f+d-.2d & f+d+.2d*/
        Limit3 = Ref_p[RefSize-1].SymbolPeriod+ HalfLevelDuration - (HalfLevelDuration/DRIFT_THRESHOLD);
        Limit4 = Ref_p[RefSize-1].SymbolPeriod+ HalfLevelDuration + (HalfLevelDuration/DRIFT_THRESHOLD);
      
        /*Check whether the first payload half-level is High */     
        if((Buf_p[j+RefSize-1].SymbolPeriod > Limit1) && (Buf_p[j+RefSize-1].SymbolPeriod < Limit2) )
        {
            *PayloadHalfLevel_p=1; /* first payload half-level is HIGH */
            *HeaderLastIndex_p=j+RefSize-1;
        }
        else if((Buf_p[j+RefSize-1].SymbolPeriod > Limit3) && (Buf_p[j+RefSize-1].SymbolPeriod < Limit4) )
        {
            *PayloadHalfLevel_p=0; /* first payload half-level is LOW */
            *HeaderLastIndex_p=j+RefSize-1;
        }  
        else
        {
            Okay = FALSE; /* invalid final start symbol */
        }
    }
    return Okay;
}


/*****************************************************************************
BLAST_ShiftDecode()
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
ST_ErrorCode_t BLAST_ShiftDecode(U32                *UserBuf_p,
                               const U32            UserBufSize,
                               STBLAST_Symbol_t     *SymbolBuf_p,
                               const U32            SymbolsAvailable,
                               U32                  *NumDecoded_p,
                               U32                  *SymbolsUsed_p,
                               const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U32 i, Coded;
    U8  j;
    U8  PayloadHalfLevel;
    U16 HalfLevelDuration;
    U32 HeaderLastIndex;
    U32 SymbolIndex;
    U32 Data;
    U32 SymbolsLeft, SymbolsUsed;
    BOOL ProtocolError = FALSE;
    U8  Up,Down;
    BOOL TransitionArray[32]; /* only one BufferElementPerPayload is supported so this can be a fixed value */
    BOOL ExtendedTrace;   
    
    *NumDecoded_p = 0;
    
    /* generate correct interpretation from inverted IR receive input */
#if defined (IR_INVERT)
    if (InvertedInputCompensate(SymbolBuf_p,
                            ProtocolParams_p->UserDefined.StartSymbols[0].MarkPeriod,
                            SymbolsAvailable) == FALSE)
    {
        ProtocolError = TRUE;
    }                      
#endif

    /* check  user setting for mark period*/    
    if(ProtocolParams_p->UserDefined.HighDataSymbol.MarkPeriod == 0)
    {
        HalfLevelDuration = ProtocolParams_p->UserDefined.LowDataSymbol.MarkPeriod;
        Down = 0;   /* define the relationship between    */ 
        Up   = 1;   /* transition direction and bit value */
    }
    else
    {
        HalfLevelDuration = ProtocolParams_p->UserDefined.HighDataSymbol.MarkPeriod;        
        Down = 1;   /* define the relationship between    */
        Up   = 0;   /* transition direction and bit value */
    }
  
    SymbolsLeft = SymbolsAvailable;
    /* Process as many user commands as are required */
    for(i = 0; i < UserBufSize && SymbolsLeft > 0; i++)
    { 
        /*** HEADER VALIDATION ***/  
        /* Check the symbols match the allowed start symbols */
        if(ValidateHeader( SymbolBuf_p, 
                        ProtocolParams_p->UserDefined.StartSymbols,
                        HalfLevelDuration,&PayloadHalfLevel,
                        SymbolsAvailable,
                        ProtocolParams_p->UserDefined.NumberStartSymbols,
                        &HeaderLastIndex
                       ) == FALSE)
        {
             return  ProtocolError;
        }
  
        /* calculate total number of symbols available now after header validation*/ 
        SymbolsLeft= SymbolsAvailable-HeaderLastIndex-1;
        SymbolsUsed     = HeaderLastIndex+1;
        
        /* increment the index just to point to the next symbol after the header validation*/
        HeaderLastIndex = HeaderLastIndex+1;
    
        /* We've now found a starting symbol, so let's decode it
        to a manchester-coded bitmask.*/
   
        Coded = 0;
        
        SymbolIndex = HeaderLastIndex; 
        /* traverse the BIT ARRAY, setting bits as we go */
        /* Bit 0 */
        if(ProtocolParams_p->UserDefined.NumberStartSymbols == 0)
        {
            PayloadHalfLevel=1; /* the start of the symbol and START of the bit-period must coincide*/ 
        }

        if(PayloadHalfLevel == 0)
        {
            TransitionArray[0] = Up;                
            ExtendedTrace = IsExtended(SymbolBuf_p[++SymbolIndex], MARK, HalfLevelDuration);
        }
        else /* PayloadHalfLevel==1 */
        {
            TransitionArray[0] = Down;
            ExtendedTrace = IsExtended(SymbolBuf_p[SymbolIndex], SPACE, HalfLevelDuration);
        }

        /* remaining bits */
        for(j=1; j<ProtocolParams_p->UserDefined.NumberPayloadBits && SymbolsLeft>0; j++)
        {
            /* Remaining payload bits */
            if(TransitionArray[j-1] == Down)
            {                
                if(ExtendedTrace)
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
                if(ExtendedTrace)
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

        /* array into a single value, which each element translated to a bit */
        Data = 0;
        for(j=0; j<ProtocolParams_p->UserDefined.NumberPayloadBits; j++)
        {
            Data |= (TransitionArray[j]) << (ProtocolParams_p->UserDefined.NumberPayloadBits - j -1);
        }
            
        /* increment the index just to point to the next symbol after the payload validation*/
        SymbolIndex++;
        
        /* Check the symbols match the allowed start symbols */
        if(ValidateStop( SymbolBuf_p, 
                       ProtocolParams_p->UserDefined.StopSymbols,                       
                       ProtocolParams_p->UserDefined.NumberStopSymbols,
                       SymbolIndex,
                       &HeaderLastIndex
                     ) == FALSE)    
        {
            return  ProtocolError;
        }
    
        /* we got the valid result for single decode*/
        /* Set the user data element */
        *UserBuf_p = Data;
        UserBuf_p++;
        (*NumDecoded_p)++;
        /* Store the total number of symbols used for single decode */
        SymbolsUsed     = HeaderLastIndex+1;
        /* Increment symbols used */
        *SymbolsUsed_p += SymbolsUsed;
    }  
    
    return ST_NO_ERROR;
}


ST_ErrorCode_t BLAST_ShiftEncode(const U32                  *UserBuf_p,
                                 const U32                  UserBufSize,
                                 STBLAST_Symbol_t           *SymbolBuf_p,
                                 const U32                  SymbolBufSize,
                                 U32                        *SymbolsEncoded_p,
                                 const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U32 SymbolCount = 0, i;
    U32 Data, SymbolPeriod, MarkPeriod,Temp;
    U8  j;
    U32 Coded;
    U32 Bit, Last,HighToLowTransition;
    U8 MsbVal=0,LsbVal=0;
    U16 HalfLevelDuration;
    U16 SubCarrierFreq;
    U8 SymLoop;
    *SymbolsEncoded_p = 0;              /* Reset symbol count */
    /* Check buffer size is large enough for the maximum number of
     * symbols that might be generated by the encoder function.
     */
    SubCarrierFreq = ProtocolParams_p->UserDefined.SubCarrierFreq;
    if (SymbolBufSize < ((ProtocolParams_p->UserDefined.NumberStartSymbols + 
                          ProtocolParams_p->UserDefined.NumberStopSymbols + 
                          ProtocolParams_p->UserDefined.NumberPayloadBits) * UserBufSize))
    {
        return ST_ERROR_NO_MEMORY;
    }
    
    /* Copy user buffer */
    Data = *UserBuf_p;

    /* obtain the used defined transition*/
    if(ProtocolParams_p->UserDefined.HighDataSymbol.MarkPeriod!=0)
    {
        HighToLowTransition=TRUE;
        HalfLevelDuration=GenerateValue( ProtocolParams_p->UserDefined.HighDataSymbol.MarkPeriod, 
                                         ProtocolParams_p->UserDefined.SubCarrierFreq );
    }
    else
    {
        HighToLowTransition=FALSE;
        HalfLevelDuration=GenerateValue( ProtocolParams_p->UserDefined.LowDataSymbol.MarkPeriod, 
                                         ProtocolParams_p->UserDefined.SubCarrierFreq );
    }

    for(i = 0; i < UserBufSize; i++, UserBuf_p++)
    {
        /* Add start symbols at the front, if required */
        for(SymLoop = 0; SymLoop < ProtocolParams_p->UserDefined.NumberStartSymbols; SymLoop++)
        {   
            /* check for First start symbol*/
            if (SymLoop== ProtocolParams_p->UserDefined.NumberStartSymbols-1)
            {   
                MsbVal=  (Data>>(ProtocolParams_p->UserDefined.NumberPayloadBits-1))&0x01;
                LsbVal=Data&0x01;
                if(HighToLowTransition==TRUE)
                {   
                    /*check the MSB of data bit if Low it mean we have to add halfbit period in start symbol*/
                    if(MsbVal==0x00)
                    { 
                        Temp= ProtocolParams_p->UserDefined.StartSymbols[SymLoop].SymbolPeriod 
                            +ProtocolParams_p->UserDefined.HighDataSymbol.MarkPeriod;
                        GenerateSymbol(Temp,SymbolBuf_p,SubCarrierFreq);
                    }  
                    else
                    {   GenerateSymbol(ProtocolParams_p->UserDefined.StartSymbols[SymLoop].SymbolPeriod, 
                                        SymbolBuf_p,SubCarrierFreq); 
                    }     
                    GenerateMark(ProtocolParams_p->UserDefined.StartSymbols[SymLoop].MarkPeriod, 
                                 SymbolBuf_p,SubCarrierFreq);
                }
                else
                {   /*check the MSB of data bit if high it mean we have to add halfbit period in start symbol*/
                    if(MsbVal==0x01)
                    { 
                        Temp=ProtocolParams_p->UserDefined.StartSymbols[SymLoop].SymbolPeriod 
                             +ProtocolParams_p->UserDefined.LowDataSymbol.MarkPeriod;
                        GenerateSymbol(Temp,SymbolBuf_p,SubCarrierFreq);
                    }
                    else
                    {   
                        GenerateSymbol(ProtocolParams_p->UserDefined.StartSymbols[SymLoop].SymbolPeriod,SymbolBuf_p,SubCarrierFreq);  
                    }
                }
                GenerateMark(ProtocolParams_p->UserDefined.StartSymbols[SymLoop].MarkPeriod,SymbolBuf_p,SubCarrierFreq);
            }
            else
            {   
                GenerateMark(ProtocolParams_p->UserDefined.StartSymbols[SymLoop].MarkPeriod, SymbolBuf_p,SubCarrierFreq);
                GenerateSymbol(ProtocolParams_p->UserDefined.StartSymbols[SymLoop].SymbolPeriod,SymbolBuf_p,SubCarrierFreq);
            }
            SymbolCount++;
            SymbolBuf_p++;
        }
       
        /**********************************************************
         * Generate manchester-coded bitmask of half-bit values:
         *
         * The following algorithm is used to calculate the coded
         * data:
         *
         * E.g.,
         *
         * NRZ_CLOCK    0 1  0 1  0 1  0 1  0 1  0 1  0 1  0 1
         * ---------------------------------------------------
         * DATA          1    0    0    0    1    1    1    0
         * ---------------------------------------------------
         *In case of HighToLowTransition==TRUE     coded data= XNOR(!NRZ_CLOCK, DATA)
         * CODED        1 0  0 1  0 1  0 1  1 0  1 0  1 0  0 1
         *In case of HighToLowTransition==FALSE  coded data= XOR(!NRZ_CLOCK, DATA)
         * CODED        0 1  1 0  1 0  1 0  0 1  0 1  0 1  1 0
         *
         * Where NRZ_CLOCK is the half-bit clock and DATA is the
         * data to code.
         */
         
        /* Coded data is a bit-mask of 2*NumberPayload half-bit periods  */
        /* User defined NumberPayloadBits to process */
        Coded=0;
        for(j = 0; j <  ProtocolParams_p->UserDefined.NumberPayloadBits; j++)      
        {
            /* Calculate bit value */
            Bit = ((Data >> ((ProtocolParams_p->UserDefined.NumberPayloadBits-1) - j) & 1));
            /* First half-bit period */
            if(HighToLowTransition==TRUE)
            {
                Coded |= ((Bit ^ 0) << (j * 2));
                /* Second half-bit period */
                Coded |= ((Bit ^ 1) << ((j * 2) + 1));
            }
            else
            {    
                Coded |= ((Bit ^ 1) << (j * 2));
                /* Second half-bit period */
                Coded |= ((Bit ^ 0) << ((j * 2) + 1));
            }
        }                
      
        SymbolPeriod = 0;
        MarkPeriod = 0;
        
        /* Now generate symbol buffer from coded data */ 
        Last = (U8)-1;        
        for(j = 0; j < (ProtocolParams_p->UserDefined.NumberPayloadBits * 2); j++)
        {
            /* Check for high->Low transition, as this means we must
             * start a new symbol.
             */
            Bit = (Coded & (1 << j));
            if(Last == 0 && Bit != 0)
            {
                SymbolBuf_p->MarkPeriod = MarkPeriod * HalfLevelDuration;
                SymbolBuf_p->SymbolPeriod = SymbolPeriod * HalfLevelDuration;
                /* Reset period counters */
                SymbolPeriod = 0;
                MarkPeriod = 0;
                /* Next symbol */
                SymbolBuf_p++;
                SymbolCount++;
             }
            
             /* Check for mark or space */
             if(Bit != 0)
             {
                 MarkPeriod++;
             }
             
             /*skip only first time as this has been absorbed by start symbol*/
             if(HighToLowTransition==TRUE)
             {
                if(MsbVal==0x01)
                {
                    SymbolPeriod++;             /* Always inc symbol period execept for the first time if MSB is HIGH*/
                }
                MsbVal=0x01;
            }
            else           
            {
                if(MsbVal==0x00)
                {
                    SymbolPeriod++;             /* Always inc symbol period execept for the first time if MSB is HIGH*/
                }
                MsbVal=0x00;
            } 
            /* Only store last bit if this is not the first time around the loop.*/
            if(j > 0)
            {
                Last = Bit;
            }
        }
        
        if(((HighToLowTransition==TRUE) && (LsbVal==0x01)) || ((HighToLowTransition==FALSE) && (LsbVal==0x00)) )
        {                
                SymbolBuf_p->MarkPeriod = MarkPeriod * HalfLevelDuration;
                SymbolBuf_p->SymbolPeriod = SymbolPeriod * HalfLevelDuration;
                /* Increment count */
                SymbolCount++;
                SymbolBuf_p++;
        }

        /* If No stop symbols are defined then we have to prepare the last Symbol */       
        if(ProtocolParams_p->UserDefined.NumberStopSymbols==0)
        {  
            if(((HighToLowTransition==TRUE) && (LsbVal==0x00)) || ((HighToLowTransition==FALSE) && (LsbVal==0x01)) )
            {
                SymbolBuf_p->MarkPeriod   =  HalfLevelDuration;
                SymbolBuf_p->SymbolPeriod =  2 * HalfLevelDuration;
                /* Increment count */
                SymbolCount++;
                SymbolBuf_p++;
            }
        }
        
        /* Add stop symbols, if required */       
        for(SymLoop = 0; SymLoop < ProtocolParams_p->UserDefined.NumberStopSymbols; SymLoop++)
        {  
            if(SymLoop==0)
            {
                if((HighToLowTransition==TRUE))
                {
                    if(LsbVal==0x00)
                    {   Temp= ProtocolParams_p->UserDefined.StopSymbols[SymLoop].SymbolPeriod 
                          +ProtocolParams_p->UserDefined.HighDataSymbol.MarkPeriod;
                        GenerateMark(Temp, SymbolBuf_p,SubCarrierFreq);
                    }
                    else
                    {
                        GenerateMark(ProtocolParams_p->UserDefined.StopSymbols[SymLoop].SymbolPeriod , SymbolBuf_p,SubCarrierFreq);
                    }
                }
                else
                { 
                    if(LsbVal==0x01)
                    {
                        Temp= ProtocolParams_p->UserDefined.StopSymbols[SymLoop].SymbolPeriod 
                          +ProtocolParams_p->UserDefined.LowDataSymbol.MarkPeriod;
                        GenerateMark(Temp, SymbolBuf_p,SubCarrierFreq);
                    }
                    else
                    {
                        GenerateMark(ProtocolParams_p->UserDefined.StopSymbols[SymLoop].SymbolPeriod , SymbolBuf_p,SubCarrierFreq);
                    }
                }   
                GenerateSymbol(ProtocolParams_p->UserDefined.StopSymbols[SymLoop].SymbolPeriod,SymbolBuf_p,SubCarrierFreq);
            }  
            else
            {   
                GenerateMark( ProtocolParams_p->UserDefined.StopSymbols[SymLoop].MarkPeriod, SymbolBuf_p,SubCarrierFreq);
                GenerateSymbol(ProtocolParams_p->UserDefined.StopSymbols[SymLoop].SymbolPeriod,SymbolBuf_p,SubCarrierFreq);
            }
            SymbolCount++;
            SymbolBuf_p++;
        } 
    }  
    /* Set symbols encoded count */
    *SymbolsEncoded_p = SymbolCount;
    /* Can't really go wrong with this */                          
    return ST_NO_ERROR;
}
/* EOF */
