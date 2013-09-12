/*****************************************************************************
File Name   : rmap.c

Description : STBLAST rSTEP protocol routines

Copyright (C) 2006 STMicroelectronics

History     : Created Jan 2006

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
#include "rmap.h"

#if defined(IR_INVERT)
#include "invinput.h"
#endif

/* The allowed drift is defined to be 25% (1/4) */
#define DRIFT_THRESHOLD         4

#define HALF_LEVEL_DURATION     250
#define SYMBOL_TIMEOUT          10000
#define RMAP_MAX_FRAME_SIZE     44
#define RSTEP_MAX_FRAME_SIZE    46
#define CUSTOMID_BIT_START      1
#define CUSTOMID_BIT_END        6
#define DEVICEID_BIT_START      8
#define DEVICEID_BIT_END        10
#define ADDRESS_BIT_START       11
#define ADDRESS_BIT_END         13
#define RSTEP_COMMAND_BIT_START 15
#define RSTEP_COMMAND_BIT_END   22
#define RMAP_COMMAND_BIT_START  14
#define RMAP_COMMAND_BIT_END    21

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
    DEVICE_TYPE_REMOTE = 0,
    DEVICE_TYPE_KEYBOARD = 4,
    DEVICE_TYPE_MOUSE_MOVE,
    DEVICE_TYPE_MOUSE_BTTN
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
    return ((LocalPeriod > (HalfLevelDuration*4)/3) ? 1 : 0 ); /* greater than 1.5 times allowed value */

      
}


/*****************************************************************************
BLAST_RMapDecode()
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
ST_ErrorCode_t BLAST_RMapDecode(U32                 *UserBuf_p,
                               STBLAST_Symbol_t     *SymbolBuf_p,
                               U32                  SymbolsAvailable,
                               U32                  *NumDecoded_p,
                               const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U8  j;
    U8  PayloadHalfLevel;
    U16 HalfLevelDuration;
    U32 SymbolIndex=0;
    U8  Up,Down;
    BOOL TransitionArray[RSTEP_MAX_FRAME_SIZE] = {0}; /* only one BufferElementPerPayload is supported so this can be a fixed value */
    BOOL ExtendedTrace;   
    U8 DeviceId= 0;
    U8 CustomId = 0;
    U8 Address = 0;
    U8 Data = 0;
    *NumDecoded_p = 0;    
      
    /* This is for Ruwido */
    HalfLevelDuration = HALF_LEVEL_DURATION;        
    
#if defined (IR_INVERT)
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
    
        Data = 0;
        
        /* Check for start bit */
        if ( TransitionArray[0] != 1 )
        {
            *NumDecoded_p = 0;  
            return ST_NO_ERROR;
        }            	
            
        /* build up Custom Id */
        for ( j = CUSTOMID_BIT_START; j <= CUSTOMID_BIT_END; j++)
        {
            CustomId <<= 1;
            CustomId |= TransitionArray[j];
        }
            
        if ( CustomId != ProtocolParams_p->Ruwido.CustomID )            
        {
            *NumDecoded_p = 0;  
            return ST_NO_ERROR;
        }  
            
        /* build up device Id */
        for ( j = DEVICEID_BIT_START; j <= DEVICEID_BIT_END; j++)
        {
            DeviceId <<= 1;
            DeviceId |= TransitionArray[j];
        }
            
        if ( DeviceId != ProtocolParams_p->Ruwido.DeviceID )            
        {
            *NumDecoded_p = 0;  
            return ST_NO_ERROR;
        }  
            
        /* build up Address field */
        for ( j = ADDRESS_BIT_START; j <= ADDRESS_BIT_END; j++)
        {
            Address <<= 1;
            Address |= TransitionArray[j];
        }
            
        if ( Address != ProtocolParams_p->Ruwido.Address )            
        {
            *NumDecoded_p = 0;  
            return ST_NO_ERROR;
        }  
            
        if ( ProtocolParams_p->Ruwido.FrameLength == RMAP_MAX_FRAME_SIZE/2)
        {
        	     
            /* 1st word contains 8bit command */
            for (j = RMAP_COMMAND_BIT_START; j < RMAP_COMMAND_BIT_END; j++)
            {
                Data <<= 1;
                Data |= (TransitionArray[j]);
            }
        }
        else if (ProtocolParams_p->Ruwido.FrameLength == RSTEP_MAX_FRAME_SIZE/2)
        {
        	     
            /* 1st word contains 8bit command */
            for (j = RSTEP_COMMAND_BIT_START; j < RSTEP_COMMAND_BIT_END; j++)
            {
                Data <<= 1;
                Data |= (TransitionArray[j]);
            }
        }
        else
        {
            *NumDecoded_p = 0;  
            return ST_NO_ERROR;	
        }
        
         
        /* Set the user data element */
       *UserBuf_p = Data;
       *NumDecoded_p = 1;  
            
    }

    return ST_NO_ERROR;
}
/* EOF */
