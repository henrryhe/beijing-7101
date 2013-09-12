
/* ----------------------------------------------------------------------------
File Name: util.c

Description: 

     Misc utility functions useful at all levels of this driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP from work by SD (on STSCART)
comment: new

   date: 31-August-2001
version: 3.1.1
 author: GJP
comment: Added support for 5510, 5514, 5516, 5508, 5518 and 5580.

---------------------------------------------------------------------------- */


/* includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   
/* C libs */
#include <string.h>
#include <stdio.h>
#endif
#include "stlite.h" /* Standard includes */

/* STAPI */
#include "sttbx.h"

#include "sttuner.h"

/* local */
#include "dbtypes.h"    /* data types for databases */

#include "util.h"       /* header for this file */




/* ----------------------------------------------------------------------------
Name: STTUNER_Util_CheckPtrNull()

Description:
    checks a data pointer whether its NULL or not
    
Parameters:
    any pointer into memory (not I/O space)
    
Return Value:
   STTUNER_ERROR_POINTER if pointer is NULL else ST_NO_ERROR
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_Util_CheckPtrNull(void *Pointer)
{
#ifndef STTUNER_DEBUG_MODULE_UTIL_CHECKING_OFF

#ifdef STTUNER_DEBUG_MODULE_UTIL
    const char *identity = "STTUNER util.c STTUNER_Util_CheckPtrNull()";
#endif

    /* Check the pointer is not null (new or set back to null) */
    if (Pointer == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_UTIL
        STTBX_Print(("%s fail pointer is NULL\n", identity));
#endif
	return (STTUNER_ERROR_POINTER);
    }
    return (ST_NO_ERROR);

#else /* STTUNER_DEBUG_MODULE_UTIL_CHECKING_OFF */
    return (ST_NO_ERROR);
#endif
}

/* ----------------------------------------------------------------------------
Name: STTUNER_Util_PowOf2()

Description:
    Compute  2^n (where n is an integer)
    
Parameters:
    number -> n

Return Value:
    2^n
---------------------------------------------------------------------------- */
long STTUNER_Util_PowOf2(int number)
{
    int i;
    long result = 1;

    for(i = 0; i < number; i++)
    {
        result = result * 2;
    }    
    return(result);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_Util_LongSqrt()

Description:
    Compute the square root of n (where n is a long integer)

Parameters:
    number -> n

Return Value:
    sqrt(n)
---------------------------------------------------------------------------- */
long STTUNER_Util_LongSqrt(long Value)
{
    /* this performs a classical root extraction on long integers */

    long Factor=1;
    long Root=1;
    long R;
    long Ltmp;

    if(Value <= 1)
    {
        return (Value);
    }
    
    Ltmp = Value;

    while (Ltmp > 3)	
    {
        Factor<<=2;
        Ltmp>>=2;
    }

    R = Value - Factor; /* Ratio */ 
    Factor>>=2;

    while(Factor > 0)
    {
        Root<<=1;
        Ltmp = (Root << 1);
        Ltmp *= Factor;
        Ltmp += Factor;
        Factor>>=2;

        if( (R - Ltmp) >= 0)
        {
            R = R - Ltmp;
            Root += 1;
        }

    }

    return(Root);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_Util_BinaryFloatDiv()

Description:
    Compute the square root of n (where n is a long integer)

Parameters:
    number -> n

Return Value:
    sqrt(n)
---------------------------------------------------------------------------- */
long STTUNER_Util_BinaryFloatDiv(long n1, long n2, int precision)
{
    int i = 0;
    long result = 0;

    /*	division de N1 par N2 avec N1<N2 */
    while(i <= precision)   /* n1 > 0	*/
    {
        if(n1<n2)
        {
            result *= 2;      
            n1 *= 2;
        }
        else
        {
            result = result * 2 + 1;
            n1 = (n1-n2) * 2;
        }
        i++;
    }

    return result;
}



/* end of util.c */
