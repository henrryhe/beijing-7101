/*****************************************************************************
File Name   : rcmm.c

Description : STBLAST RCMM 24/32 encoding/decoding routines

Copyright (C) 2004 STMicroelectronics

History     : Created April 2004

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif

#include "stddefs.h"
#include "stblast.h"
#include "blastint.h"
#include "rcmm.h"

#if defined(IR_INVERT)
#include "invinput.h"
#endif

static U32 count;

#define MIN_00          386
#define MAX_00          502

#define MIN_01          553
#define MAX_01          669

#define MIN_10          719
#define MAX_10          835

#define MIN_11          886
#define MAX_11          1002


#define MID_HEADER_ON   416
#define MID_HEADER      694

#define MID_00          444
#define MID_01          611

#define MID_10          777
#define MID_11          994

/* delta is -52 to +65 micro sec */
#define MIN_ON           80
#define MID_ON          166
#define MAX_ON          275


#define SubCarrierFreq          (U32)36000

#define MODE_CUST_ID_LENGTH     12
#define SUB_MODE_LENGTH          2
#define ADDRESS_LENTH            2
#define DATA_LENGTH              8
#define OPTIONAL_DATA_LENGTH     8

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
BOOL Header_Mark_Found = FALSE, HeaderFound = FALSE, Header_Symbol_Found=FALSE; 

U8 DurationTransform2_rcmm(U16 Value);
void DurationTransform_rcmm(STBLAST_Symbol_t* SymbolBuf_p,
                        const U32 SymbolsAvailable, 
                        U8 * DurationArray);


/***************************** WORKER ROUTINES  *****************************/

void DurationTransform_rcmm(STBLAST_Symbol_t* SymbolBuf_p,
                        const U32 SymbolsAvailable, 
                        U8 * DurationArray)
{
    U32 i; /* changed from U32 to int to remove lint warning*/
    
    for (i=0; i<SymbolsAvailable; i++)
    {
        /* Symbol value = mark + Space */ 
        *DurationArray = DurationTransform2_rcmm( SymbolBuf_p->SymbolPeriod);
        DurationArray++;
        
        /* Mark Value */
        *DurationArray = DurationTransform2_rcmm( SymbolBuf_p->MarkPeriod );
        DurationArray++;
        
        SymbolBuf_p++;         
    }
}

U8 DurationTransform2_rcmm(U16 Value)
{    
   
    if (HeaderFound == FALSE)
    {
        if(Value > RCMM_MAX_HEADER_TIME)
        {
            HeaderFound = FALSE;
            Header_Mark_Found = FALSE;
            Header_Symbol_Found = FALSE;
            return 0xFF; /* time out value */
            
        }
        else if(Value > RCMM_MIN_HEADER_TIME)
        {
            if ( (Value > RCMM_MAX_HEADER_ON) && (Header_Mark_Found == FALSE))
            {
                Header_Symbol_Found = TRUE;
                return 25;
            }
        } 
        else if ((Value > RCMM_MIN_HEADER_ON) && (Value <= RCMM_MAX_HEADER_ON) && (Header_Symbol_Found == TRUE) )
        {
            Header_Mark_Found = TRUE;
            HeaderFound = TRUE; 
            return  15;
        }
        else
        {
            HeaderFound = FALSE;
            Header_Mark_Found = FALSE;
            return 0;
        }
        
    }
    else 
    {
        if(Value >MAX_11)
        {
            return 0xFF; /* time out value */
        }
    
        else if( Value > MAX_10)
        {
            if(Value > MIN_11)
            {
                return 34; /* 34 periods */
            }
            else
            {
            return 0;
            }
        }
    
        else if(Value > MAX_01)
        {
            if(Value > MIN_10)
            {
                return 28;
            }
            else
            {
                return 0;
            }
        }
    
        else if(Value > MAX_00)
        {
            if(Value > MIN_01)
            {
                return 22;
            }
            else
            {
                return 0;
            }
        }
        else if (Value > MAX_ON)
        {
            if(Value > MIN_00)
            {
                return 16;
            }
            else
            {
                return 0;
            }
        }
        else if( Value > MIN_ON)
        {
            return 6;
        }
        else
        {
            return 0;
        }
    
    }
    
    return 0;
}



/*****************************************************************************
BLAST_RcmmDecode()
Description:
    Decode rcmm symbols into user buffer

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

ST_ErrorCode_t BLAST_RcmmDecode(U32                  *UserBuf_p,
                                STBLAST_Symbol_t     *SymbolBuf_p,
                                U32                   SymbolsAvailable,
                                U32                  *NumDecoded_p,
                                U32                  *SymbolsUsed_p,
                                const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U8 i;
    U8 j;
    U32 SymbolsLeft;
    U8 DurationArray[200];
    U8 LevelArraySize;
    U8 LevelArray[400]={0};  /* check these values */
       
   
    /* Set number of symbols allowed */
    SymbolsLeft = SymbolsAvailable;
    *SymbolsUsed_p = 0;
    *NumDecoded_p = 0;
    HeaderFound = FALSE;
    Header_Mark_Found = FALSE;
    Header_Symbol_Found = FALSE;
    
   
    /* Check The number of symbols, it should be either 14(1(H) + 12 (data) + 1 (End)
       or 18(1 (H) + 16(data) + 1(end)) */
       
    if(ProtocolParams_p->Rcmm.NumberPayloadBits == 20)
    {
        if(SymbolsAvailable != 18)
        {
            *NumDecoded_p = 0;
            return ST_NO_ERROR; 
        }
        
    }
    else if(ProtocolParams_p->Rcmm.NumberPayloadBits == 12)
    {
        if(SymbolsAvailable != 14)
        {
            *NumDecoded_p = 0;
            return ST_NO_ERROR; 
        }
    }
    else
    {
        *NumDecoded_p = 0;
        return ST_NO_ERROR; 
    }
    
    count++;
          
    /* After verifing header it changes the time in to periods */    
    DurationTransform_rcmm(SymbolBuf_p, SymbolsAvailable, DurationArray);


   /* Header validation*/
   if (HeaderFound == FALSE)
   {
        *NumDecoded_p = 0;
        return ST_NO_ERROR;
   }
           
    /* Convert remaining durations Bit Values  00-> 6 and 16, 01-> 6 and 22, 
       10-> 6 and 28, 11-> 6 and 34 */    
    /* loop is executed till (SymbolsAvailable-1) as Header symbol is not needed */
    j = 0;
    for(i=2; i<(2*(SymbolsAvailable-1)); i++, j=j+2)
    {
        /* Symbol period for 11*/
        if(DurationArray[i] == 34)
        {
            i++;
            /* Mark period */
            if(DurationArray[i] == 6)
            {
                LevelArray[j] = 1;
                LevelArray[j+1] = 1;
            }
            else
            {
                *NumDecoded_p = 0;
                return ST_NO_ERROR;
            }
        }
        /* Symbol Period for 10*/
        else if(DurationArray[i] == 28)
        {
            i++;
            /* Mark period */
            if(DurationArray[i] == 6)
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
        
        /* Symbol Period for 01 */
        else if (DurationArray[i] == 22)
        {
            i++;
            if(DurationArray[i] == 6)
            {
                LevelArray[j] = 0;
                LevelArray[j+1] =1;
                
            }
            else
            {
                *NumDecoded_p = 0;
                return ST_NO_ERROR;        
            }
        }
        
        /* Symbol Period for 00 */
        else if (DurationArray[i] == 16)
        {
            i++;
            if(DurationArray[i] == 6)
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
             
        else
        {
            *NumDecoded_p = 0;
            return ST_NO_ERROR;            
        }
         
    }
     
    /* Check Mode bits */
    {
     
        U8 ModeLength,ModeValue = 0;   
        
        /* For short ID the mode will be 000011
        * and for Long ID  the mode will be 001 */
        if (!ProtocolParams_p->Rcmm.ShortID)  
        {
            ModeLength = 3;
        }
        else
        {
            ModeLength = 6;
        }
        
        for(i=0; i < ModeLength; i++)
        {
            ModeValue <<= 1;
            ModeValue |= LevelArray[i];    
        }
        
        if (!ProtocolParams_p->Rcmm.ShortID)  
        {
            if(ModeValue != 1)
            {
                *NumDecoded_p = 0;
                return ST_NO_ERROR;    
            }
        }
        else
        {
            if(ModeValue != 3)
            {
                *NumDecoded_p = 0;
                return ST_NO_ERROR;    
            }
        }
    }
     
             
    /* the size of Array */
    LevelArraySize = j;
      
    {
        U32 Cmd = 0;
        
        /* Form the Cmd */
        for(i= 0; i < LevelArraySize ; i++)
        {
            Cmd <<= 1;
            Cmd |= LevelArray[i];
        }
        
    *UserBuf_p = Cmd;
    *NumDecoded_p = 1;
    return ST_NO_ERROR;
    }
    
}


/*****************************************************************************
BLAST_RcmmEncode()
Description:
    Encode user buffer into Rcmm symbols

Parameters:
    UserBuf_p           user buffer to encode
    SymbolBuf_p         symbol buffer to fill
    SymbolsEncoded_p    how many symbols we generated
    ProtocolParams_p    protocol params

Return Value:
    ST_NO_ERROR

See Also:
    BLAST_SpaceEncode()
    BLAST_LowLatencyDecode()
*****************************************************************************/

ST_ErrorCode_t BLAST_RcmmEncode(const U32                  *UserBuf_p,
                                STBLAST_Symbol_t           *SymbolBuf_p,
                                U32                        *SymbolsEncoded_p,
                                const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U32 SymbolCount = 0;
    U32 i;
        
    *SymbolsEncoded_p = 0;              /* Reset symbol count */

    /**************************************************************
     * The format of an rcmm  command is as follows:
     *
     * | Header| Mode | Customer ID | Address + Data
     *
     */
   
    /* generate header  bit */    
    GenerateSymbol(MID_HEADER_ON,(MID_HEADER - MID_HEADER_ON ), SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    
    /* Form Command and genrate mark and symbols according */
    {
        U16 Cmd;
        U16 Cust_Code;
        U8  Sub_Mode,Address;
        U32 Oem_ID_Length;
        U8  Command[32];
        U32 CmdLength=0;
        
        /* form the cmd */
        if(ProtocolParams_p->Rcmm.ShortID)
        {
            Oem_ID_Length = (MODE_CUST_ID_LENGTH - 6);
            Command[0] = 0; Command[1] = 0;
            Command[2] = 0; Command[3] = 0;
            Command[4] = 1; Command[5] = 1;
                       
        }
        else
        {
            Oem_ID_Length = (MODE_CUST_ID_LENGTH -3);
            Command[0] = 0; Command[1] = 0;
            Command[2] = 1;
            
        }
        
        /* For Customer Code */
        Cust_Code = ProtocolParams_p->Rcmm.CustomerID;
        for(i = (MODE_CUST_ID_LENGTH - 1); i >= (MODE_CUST_ID_LENGTH - Oem_ID_Length); i--)
        {
            Command[i] = Cust_Code % 2;
            Cust_Code /= 2 ;
        }
        
        /* For sub mode */
        Sub_Mode = ProtocolParams_p->Rcmm.SubMode;
        for(i=(MODE_CUST_ID_LENGTH + SUB_MODE_LENGTH -1) ; i >= MODE_CUST_ID_LENGTH ; i--)
        {
            Command[i] = Sub_Mode % 2;
            Sub_Mode /= 2;
        } 
        
        /* For address mode */
        Address = ProtocolParams_p->Rcmm.Address;
        for(i= (MODE_CUST_ID_LENGTH + SUB_MODE_LENGTH + ADDRESS_LENTH -1) ; i >= (MODE_CUST_ID_LENGTH +  SUB_MODE_LENGTH) ; i--)
        {
            Command[i] = Address % 2;
            Address /= 2;
        } 
        
        if(ProtocolParams_p->Rcmm.NumberPayloadBits == 20)
        {
            CmdLength = 32;
        }
        else if(ProtocolParams_p->Rcmm.NumberPayloadBits == 12)
        {
            CmdLength = 24;
        }
        
        Cmd = *UserBuf_p;
        
        /* For command */
        for(i = (CmdLength - 1); i >= (MODE_CUST_ID_LENGTH + SUB_MODE_LENGTH + ADDRESS_LENTH) ; i--)
        {
            Command[i] = Cmd % 2;
            Cmd /=2;
        }
                
        /* Genrate symbols for command */
        for(i=0; i < CmdLength; i++)
        {
            if(Command[i] == 0)
            {
                i++;
                if(Command[i] == 0)
                {
                    /* Genrate for 00 */
                    GenerateSymbol(MID_ON,(MID_00 - MID_ON ), SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                    
                }
                else if(Command[i] == 1)
                {
                    /* Genrate for 01 */
                    GenerateSymbol(MID_ON,(MID_01 - MID_ON ), SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                }
             }
             else if(Command[i] == 1)
             {
                i++;
                if(Command[i] == 0)
                {
                    /* Genrate for 10 */
                    GenerateSymbol(MID_ON,(MID_10 - MID_ON ), SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                }
                else if(Command[i] == 1)
                {
                    /* Genrate for 11 */
                    GenerateSymbol(MID_ON,(MID_11 - MID_ON ), SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
                }
             
             }
        
        }
    }
        
     
    /* For End of frame */
    GenerateMark(MID_ON, SymbolBuf_p);           
                      
    SymbolCount++;
               
    /* Set symbols encoded count */
    *SymbolsEncoded_p = SymbolCount;

    /* Can't really go wrong with this */
    return ST_NO_ERROR;
}



/* EOF */

