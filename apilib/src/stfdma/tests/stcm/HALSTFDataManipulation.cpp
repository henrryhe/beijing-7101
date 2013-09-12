//
// Platform: OS20-c2 processor
//

#include "STF/Interface/Types/STFBasicTypes.h"

#ifdef __c2__

void MUL32x32 (uint32 op1, uint32 op2, uint32 & upper, uint32 & lower)
	{
	uint32 a, b;

	__asm {
		ldc	0;
		ldl	op2;
		ldl	op1;
		lmul;
		stl	a;
		stl	b;
		}
	lower = a;
	upper = b;
	}

uint32 DIV64x32 (uint32 upper, uint32 lower, uint32 op)
	{
	uint32 result;
	__asm {
		ldl	upper;
		ldl	lower;
		ldl	op;
		ldiv;
		stl	result;
		stl	op;
		}
	return result;
	}

uint32 ScaleDWord (uint32 op, uint32 from, uint32 to)
	{
	uint32 result;
	__asm {
		ldc	0;
		ldl	op;
		ldl	to;
		lmul;
		ldl	from;
		ldiv;
		stl	result;
		stl	op;
		}
	return result;
	}

int32 ScaleLong (int32 op, int32 from, int32 to)
	{
	uint32 result;
	int signCount = 0;
	if (op < 0)
		{
		op = -op;
		signCount++;
		}
	if (from < 0)
		{
		from = -from;
		signCount++;
		}
	if (to < 0)
		{
		to = -to;
		signCount++;
		}
	__asm {
		ldc	0;
		ldl	op;
		ldl	to;
		lmul;
		ldl	from;
		ldiv;
		stl	result;
		stl	op;
		}
	return (signCount & 1) ? -result : result;
	}

#elif __c1__

void MUL32x32 (uint32 op1, uint32 op2, uint32 & upper, uint32 & lower)
	{
	uint32 a, b;

	__asm {
		ldc	0;
		ldl	op2;
		ldl	op1;
		umac;
		stl	a;
		stl	b;
		}
	lower = a;
	upper = b;
	}

uint32 DIV64x32( uint32 dividend_high, uint32 dividend_low, uint32 divisor )
   {
   if(( dividend_high | divisor ) & 0x80000000 )   /* msb of either dividend or divisor is 1 */
   /* software */
      {
      int loopcount, remainder_b32;
      uint32 remainder = dividend_high;
      uint32 quotient  = dividend_low;
       
      for (loopcount=0;loopcount<32;loopcount++) 
         {
         remainder_b32 = remainder & 0x80000000;
         remainder     = ( remainder << 1 ) | ( quotient >> 31 );
         quotient      = quotient << 1;
       
         if( remainder_b32 || (remainder >= divisor)) 
            {
            remainder -= divisor;
            quotient |= 1;
            }
         }
      return( quotient );
      }
   else 
   /* hardware (divstep)*/
      {
      unsigned quotient;
     
      __optasm{
         ldabc divisor, dividend_low, dividend_high;   /* A = divisor B = dividend_low C = dividend_high */
         divstep; divstep; divstep; divstep;
         divstep; divstep; divstep; divstep;           /* A = divisor B = quotient     C = remainder     */
         rot;
         st quotient;
         }
       
      return( quotient );
      }
   }

uint32 ScaleDWord (uint32 op, uint32 divisor, uint32 to)
	{
   uint32 dividend_high, dividend_low;
   uint32 result;

	__asm {
		ldc	0;
		ldl	op;
		ldl	to;
		umac;
		stl	dividend_low;
		stl	dividend_high;
		}

   result = DIV64x32 ( dividend_high, dividend_low, divisor );
   }

int32 ScaleLong (int32 op, int32 from, int32 to)
   {
   uint32 dividend_high, dividend_low;
   uint32 result;

   int signCount = 0;
	if (op < 0)
		{
		op = -op;
		signCount++;
		}
	if (from < 0)
		{
		from = -from;
		signCount++;
		}
	if (to < 0)
		{
		to = -to;
		signCount++;
		}

   __asm {
		ldc	0;
		ldl	op;
		ldl	to;
		umac;
		stl	dividend_low;
		stl	dividend_high;
		}

   result = DIV64x32 ( dividend_high, dividend_low, from );

   return (signCount & 1) ? -result : result;
   }

#else
#error ST20 Core Not Defined
#endif

