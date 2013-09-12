/*****************************************************************************
File Name   : rc5.c

Description : STBLAST RC5 encoding/decoding routines

Copyright (C) 2001 STMicroelectronics

History     : Split from blastint May 2001 (PW)
            : Fixed DDTS 28070

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif
#include "stddefs.h"
#include "stblast.h"
#include "blastint.h"
#include "rc5.h"

#if defined(IR_INVERT)
#include "invinput.h"
#endif



/*      System Bit allocation
        0   TV1 Receiver
        1   TV2(Function and command numbers as system 0
        2   TXT
        3   Extension to TV1 and TV2
        4   LV(Laser vision Player
        5   VCR 1
        6   VCR 2 
        7   Reserved
        8   SAT 1 receiver
        9   Extension to VCR1 and VCR2
        10  SAT 2
        11  reserved
        12  CD-VIDEO
        13  reserved
        14  CD-PHOTO
        15  Reserved
        16  PREAMP 1(Audio Preamplifier 1)
        17  TUNER (radio tuner)
        18  REC 1(analog compact cassete recorder)
        19  PREAMP 2
        20  CD
        21  COMBI
        22  SAT
        23  REC 2
        24  reserved
        25  reserved
        26  CD-R
        27  reserved
        28  reserved
        29  reserved
        30  reserved
        31  reserved
 */


/*****************************************************************************
BLAST_RC5Decode()
Description:
    Decode RC5 symbols into user buffer

Parameters:
    UserBuf_p           user buffer to place data into
    SymbolBuf_p         symbol buffer to consume
    SymbolsAvailable    how many symbols present in buffer
    NumDecoded_p        how many data words we decoded
    SymbolsUsed_p       how many symbols we used

Return Value:
    ST_NO_ERROR

See Also:
    BLAST_SpaceDecode
    BLAST_RC5Encode
*****************************************************************************/

ST_ErrorCode_t BLAST_RC5Decode(U32                  *UserBuf_p,
                               STBLAST_Symbol_t     *SymbolBuf_p,
                               U32                   SymbolsAvailable,
                               U32                  *NumDecoded_p,
                               U32                  *SymbolsUsed_p)

{
    U32 Coded;
    U32 Data, j, SymbolPeriod, MarkPeriod,AdSymbolPeriod,AdMarkPeriod;
    U32 SymbolsLeft;
    U32 UserCount = 0;
    U8  StartFound = 0;  

#if defined (IR_INVERT)
    {
        STBLAST_Symbol_t *LastSymbol_p;
        if (InvertedInputCompensate(SymbolBuf_p,
                                (RC5_BIT_PERIOD),
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

    
    SymbolPeriod = (SymbolBuf_p->SymbolPeriod + (BOUNDARY_LIMIT)) /
                    RC5_BIT_PERIOD;
    MarkPeriod = (SymbolBuf_p->MarkPeriod + (BOUNDARY_LIMIT)) /
                    RC5_BIT_PERIOD;
    AdSymbolPeriod = (SymbolBuf_p->SymbolPeriod - (BOUNDARY_LIMIT)) /
                    RC5_BIT_PERIOD;
    AdMarkPeriod = (SymbolBuf_p->MarkPeriod - (BOUNDARY_LIMIT)) /
                    RC5_BIT_PERIOD;         

    /* Check for valid of start symbol:
     *
     * Remember that bi-phase coding means that we
     * can have at most two successive same states.
     * Therefore we know that the symbol period and mark period
     * must be in the combinations(4,2),(3,2),(2,1).
    */
                 
                          
    if ((SymbolPeriod == 2  && MarkPeriod == 1 && AdSymbolPeriod == 1 && AdMarkPeriod == 0 )||
       (SymbolPeriod == 3  && MarkPeriod == 2 && AdSymbolPeriod == 2 && AdMarkPeriod == 1 )||
       (SymbolPeriod == 4  && MarkPeriod == 2 && AdSymbolPeriod == 3 && AdMarkPeriod == 1 )) 
       {    
            StartFound=1;
       }    
           

           
    if( StartFound !=1)
    {
        *NumDecoded_p = 0;
        /*Though ST_NO_ERROR is returned,*NumDecoded_p = 0 tells the calling
         *function that the incoming signal is in error*/
        return ST_NO_ERROR;
    }     

   /* We've now found a starting symbol, so let's decode it
    * to a manchester-coded bitmask.
   */
    Coded = 0;
    for(j = 0; j < 28 && SymbolsLeft > 0; )
    {
        /* We round-up to the nearest entire bit-period and
         * also see to that the Symbol and Mark period must be centering
         * around the ideal value.The maximum units of variation is given by the 
         * boundary limit.AdSymbolPeriod & AdMarkPeriod are used to check the 
         * boundary limits
        */
            
        SymbolPeriod = (SymbolBuf_p->SymbolPeriod + (BOUNDARY_LIMIT)) /
                        RC5_BIT_PERIOD;
        MarkPeriod = (SymbolBuf_p->MarkPeriod + (BOUNDARY_LIMIT)) /
                        RC5_BIT_PERIOD;
        AdSymbolPeriod = (SymbolBuf_p->SymbolPeriod - (BOUNDARY_LIMIT)) /
                        RC5_BIT_PERIOD;
        AdMarkPeriod = (SymbolBuf_p->MarkPeriod - (BOUNDARY_LIMIT)) /
                       RC5_BIT_PERIOD; 
                          
        /*
         *In biphase encoding,a symbol will have the following combinations,
         *         SymbolPeriod    MarkPeriod                
         *             4               2
         *             3               2
         *             3               1
         *             2               1
         * The above will be true except for the last symbol which will have
         * a large SymbolPeriod and Markperiod of either 1 or 2.
         */
        /* Set mark bits */
        if ((SymbolPeriod == 4  && MarkPeriod == 2 && AdSymbolPeriod == 3 && AdMarkPeriod == 1 )||
            (SymbolPeriod == 3  && MarkPeriod == 2 && AdSymbolPeriod == 2 && AdMarkPeriod == 1 )|| 
            (SymbolPeriod == 3  && MarkPeriod == 1 && AdSymbolPeriod == 2 && AdMarkPeriod == 0 )||
            (SymbolPeriod == 2  && MarkPeriod == 1 && AdSymbolPeriod == 1 && AdMarkPeriod == 0 ))
        {
            /* Set mark bits */
            while (MarkPeriod-- > 0)
            {
                Coded |= (1 << j++);
                SymbolPeriod--;
            }

            /* Skip over space bits */
            j += SymbolPeriod;

                
        }
        else
        {   
            /*
             *The last symbol must have its MarkPeriod as either 2 or 1
             *and in the bit positions mentioned below.If not there is an error
             *in the incoming signal
             */
            U8 Error=0;
            switch(j)
            {
                case 24:
                    if(MarkPeriod == 2 && j== 24)
                    {
                        Coded |= (1 << j++);
                        Coded |= (1 << j++);
                        j+=2;
                    }
                    else
                    {
                        Error = 1;
                    }    
                    break;
                case 25:
                    if(MarkPeriod == 1 && j== 25)
                    {
                        Coded |= (1<<j++);
                        j+=2;
                    }
                    else
                    {
                         Error = 1;
                    }        
                    break;
                case 26:
                    if(MarkPeriod == 1 && j== 26)
                    {
                        Coded |= (1 << j++);
                        j++;
                    }
                    else
                    {
                        Error = 1;
                    }    
                    break;
                default:
                     Error=1;   
                     break;               
            }    
            if (Error ==1)
            {
                *NumDecoded_p = 0;
                /*Though ST_NO_ERROR is returned,*NumDecoded_p = 0 tells 
                *the calling function that the incoming signal is in error*/
                return ST_NO_ERROR;
            }    
        }/*else loop ends*/
            
        /* Next symbol */
        SymbolBuf_p++;
        SymbolsLeft--;
        
    }/*for loop ends here*/

    /* Now create an RC5 command-code from the coded value */
    Data = 0;                       /* Zero data-byte */

    /* Every other half-bit represents the final bit state.
     * Note that the final command bit-ordering is reversed
     * to ensure the start-bit is the MSB.  We skip the start-bit
     * as it is not required in the RC5 command.
    */
    for(j = 2; j < 28; j += 2)
    {
        if((Coded & (1 << j)) != 0)  /* Coded high? */
        {
            Data |= (1 << (13-(j/2))); /* Set command bit */
        }
    }

    /* Set the user data element */
    *UserBuf_p = Data;
    UserCount++;

     /* Set count information for user */
    *NumDecoded_p = UserCount;

    return ST_NO_ERROR;
}

/*****************************************************************************
BLAST_RC5Encode()
Description:
    Encode user buffer into rc5 symbols

Parameters:
    UserBuf_p           user buffer to encode
    UserBufSize         how many data words present
    SymbolBuf_p         symbol buffer to fill
    SymbolBufSize       symbol buffer size
    SymbolsEncoded_p    how many symbols we generated
    RcProtocol_p        protocol block

Return Value:
    ST_NO_ERROR

See Also:
    BLAST_SpaceEncode()
    BLAST_RC5Decode()
*****************************************************************************/
ST_ErrorCode_t BLAST_RC5Encode(const U32                  *UserBuf_p,
  							   const U32                  UserBufSize,
                               STBLAST_Symbol_t           *SymbolBuf_p,
                               U32                        *SymbolsEncoded_p)
                               
{
    U32 SymbolCount = 0, i;
    U32 j, Data, SymbolPeriod, MarkPeriod;
    U32 Coded;
    U32 Bit, Last;

    *SymbolsEncoded_p = 0;              /* Reset symbol count */
    
    /**************************************************************
     * The format of an RC5 command is as follows:
     *
     * | S[13] | F[12] | C[11] | SYSTEM[10:6] | COMMAND[5:0] |
     *
     */

    for(i = 0; i < UserBufSize; i++, UserBuf_p++)
    {
        /* Copy user buffer */
        Data = *UserBuf_p;

        /* Add start-bit */
        Data |= (1 << 13);

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

        /* Coded data is a bit-mask of 26 half-bit periods Coded[27:0] */
        Coded = 0;
        for(j = 0; j < 14; j++)        /* 14-bits to process */
        {
            /* Calculate bit value */
            Bit = ((Data >> (13 - j) & 1));

            /* First half-bit period */
            Coded |= ((Bit ^ 1) << (j * 2));

            /* Second half-bit period */
            Coded |= ((Bit ^ 0) << ((j * 2) + 1));
        }

        /* Now generate symbol buffer from coded data - the data
         * is stored in 26-bits coded LSB.
         */
        Last = (U8)-1;
        SymbolPeriod = 0;
        MarkPeriod = 0;

        /* We skip the first half-bit.  We know that this must be
         * a start-bit i.e., low->transition (logic 1).  This simplifies
         * the processing...
         */
        for(j = 1; j < 28; j++)
        {
            /* Check for low->high transition, as this means we must
             * start a new symbol.
             */
            Bit = (Coded & (1 << j));
            if(Last == 0 && Bit != 0)
            {
                /* Set symbol values - each symbol period is
                 * 32 x symbol_time, where the sub-carrier
                 * is 36Khz.
                 */
                SymbolBuf_p->MarkPeriod = MarkPeriod * 32;
                SymbolBuf_p->SymbolPeriod = SymbolPeriod * 32;

                /* Reset period counters */
                SymbolPeriod = 0;
                MarkPeriod = 0;

                /* Next symbol */
                SymbolBuf_p++;
                SymbolCount++;
            }

            /* Check for mark or space */
            if(Bit != 0)
                MarkPeriod++;
            SymbolPeriod++;             /* Always inc symbol period */

            /* Only store last bit if this is not the first
             * time around the loop.
             */
            if(j > 1)
                Last = Bit;             /* Store last bit */
        }

        /* Write out the last symbol, because the above loop
         * contruction lags one half bit-period behind.
         * Note the special case where we end on a high half-bit period.
         * We add on and extra symbol period for this case.
         */
        SymbolPeriod++;
        SymbolBuf_p->MarkPeriod = MarkPeriod * 32;
        SymbolBuf_p->SymbolPeriod = SymbolPeriod * 32;

        /* Increment count */
        SymbolCount++;
    }

    /* Set symbols encoded count */
    *SymbolsEncoded_p = SymbolCount;

    /* Can't really go wrong with this */
    return ST_NO_ERROR;
}

/* EOF */
