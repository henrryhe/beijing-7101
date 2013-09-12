/*****************************************************************************
File Name   : invinput.h

Description : Prototype for InvertedInputCompensate routine.
              Provides a software inversion of received symbol stream
Copyright (C) 2002 STMicroelectronics

History     : Created 2002 

Reference   :

*****************************************************************************/
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif
#include "stddefs.h"
#include "invinput.h"

#if defined (IR_INVERT)
/*****************************************************************************
InvertedInputCompensate
Description:
        Generate the real symbol data from an inverted IRB input.  The "mark" 
        value of the start symbol is simply set to the expected value, as it 
        cannot be obtained from an inverted input.

Parameters:
    Buf                         the symbols to compare
    ExpectedStartSymbolMark     the first expected symbol
    SymbolsAvailable            the number of symbols in Buf       

Return Value:
    TRUE        if function succeeds
    FALSE       otherwise

See Also:
    BLAST_SpaceDecode
*****************************************************************************/

/* Algorithm description */

#if 0
  <--- TO ---><-- T1 --><-- T2 -->
   ______      ____      ____
  | M0   | S0 | M1 | S1 | M2 | S2 |           Correct input
__|      |____|    |____|    |____|

  
         <-- t0 --><-- t1 -->
__        ____      ____      ____ 
  |      | m0 | s0 | m1 | s1 | m2 |           Inverted input
  |______|    |____|    |____|    |

#endif /* if 0 ascii waveform */

/*
The objective is to generate the correct input from the inverted input, in terms of 
mark(M) and symbol(T) periods.  We have no way of extracting the value of M0, so we 
simply assign  this to be the value the user expects (the start symbol mark period. 
The conversion can be performed as follows:

M(0) = user expected                        (i=0)
T(0) = M(0) + m(0)

M(i) = s(i-1)       = t(i-1) - m(i-1)       (i>0)          
T(i) = M(i) + S(i)  = M(i) + m(i)

The inverted symbols captured by the board actually contain a spurious leading value
of  (0,0).  So the first useful incoming symbol is at i=1. This allows the buffer to 
be decoded in place.  Without the leading (0,0), an additional symbol would be needed 
to temporarily store the received(inverted) symbol whilst the element is overwritten.  
Adding 1 to each of the SOURCE buffer indices gives the following equations:

M(i) = s(i)         = t(i) - m(i)
T(i) = M(i) + S(i)  = M(i) + m(i+1)
*/


BOOL InvertedInputCompensate(STBLAST_Symbol_t *Buf,
                             const U32 ExpectedStartSymbolMark,
                             const U32 SymbolsAvailable)
{
    BOOL Okay = TRUE;
    int Index;

    /* first symbol, copy from user value */
    Index=0;
    Buf[Index].MarkPeriod   = ExpectedStartSymbolMark;
    Buf[Index].SymbolPeriod = Buf[Index].MarkPeriod + Buf[Index+1].MarkPeriod;
    Index++;
  
    while(Index < SymbolsAvailable)
    {
        Buf[Index].MarkPeriod   = Buf[Index].SymbolPeriod - Buf[Index].MarkPeriod;
        Buf[Index].SymbolPeriod = Buf[Index].MarkPeriod + Buf[Index+1].MarkPeriod;
        Index++;
    }
    return Okay;
}

#endif /* if defined(IR_INVERT) */
