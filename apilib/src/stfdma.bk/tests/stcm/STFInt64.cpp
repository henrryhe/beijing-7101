///
/// @file STF\Source\Types\STFInt64.cpp
///
/// @brief 
///
/// @par OWNER:
///	STF-Team
/// @par SCOPE:
///	INTTERNAL Source File
/// 
///
/// &copy; 2002 ST Microelectronics. All Rights Reserved.
///
//


#include "STF/Interface/Types/STFInt64.h"



STFInt64 operator* (const STFInt64 & u, const STFInt64 & v)
	{
	bool sign;
	uint32 ll0, ll1, lh, hl, dummy;
	STFInt64 u1, v1;
	
	if (u < 0)
		{
		u1 = -u;
		sign = true;
		}
	else
		{
		u1 = u;
		sign = false;
		}
		
	if (v < 0)
		{
		v1 = -v;
		sign = !sign;
		}
	else
		{
		v1 = v;
		}

	MUL32x32(v1.lower, u1.lower, ll1, ll0);
	MUL32x32(v1.lower, u1.upper, dummy, lh);
	MUL32x32(v1.upper, u1.lower, dummy, hl);

	if (sign)
		return -STFInt64(ll0, lh+hl+ll1);
	else
		return STFInt64(ll0, lh+hl+ll1);
	}

STFInt64 operator/ (const STFInt64 & u, const STFInt64 & v)
	{
	bool sign;
	STFInt64 u1, v1, acc;
	uint32 a0, a1, a2;
	
	if (u < 0)
		{
		u1 = -u;
		sign = true;
		}
	else
		{
		u1 = u;
		sign = false;
		}
		
	if (v < 0)
		{
		v1 = -v;
		sign = !sign;
		}
	else
		{
		v1 = v;
		}
		
	if (v1 != 0)
		{
		if (v1.upper)
			{
			if (u1.upper <= v1.upper)
				{
				a0 = u1.upper / v1.upper;
				MUL32x32(a0, v1.lower, a1, a2);
				a2 += a0 * v1.upper;
				acc = STFInt64(a1, a2);
				if (acc > u1) a0--;

				acc = STFInt64(a0, (int32)0L);
				}
			else
				acc = 0;
			}
		else
			{
			if ((uint32)u1.upper < v1.lower)
				{
				a0 = DIV64x32(u1.upper, u1.lower, v1.lower);
				acc = STFInt64(a0, (int32)0L);
				}
			else
				{
				a0 = u1.upper / v1.lower;
				a1 = DIV64x32(u1.upper % v1.lower, u1.lower, v1.lower);
				acc = STFInt64(a1, a0);
				}
			}
		if (sign)
			return -acc;
		else
			return acc;		
		}
	else
		{
		if (sign)
			return STFInt64(0x00000000, 0x80000000);
		else
			return STFInt64((uint32)0xffffffff, (uint32)0x7fffffff);
		}
	}

STFInt64::STFInt64(STFString str, int32 base)
	{
	int32 i = 0;
	bool sign = false;
	char c;
	
	*this = 0;
	
	if (str[0] == '-')
		{
		sign = true;
		i++;
		}
	
	while ((c = str[i++]) != 0)
		{
		*this *= base;
		
		if (c >= 'a' && c <= 'f')
			*this += c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			*this += c - 'A' + 10;
		else if (c >= '0' && c <= '9')
			*this += c - '0';			
		}
	
	if (sign)
		*this = -*this;
	}
	
STFString STFInt64::ToString(int32 digits, int32 base, char fill)
	{
	STFString s;
	STFInt64 a;
	bool sign;
	
	if (*this < 0)
		{
		a = -*this;
		sign = true;
		}
	else
		{
		a = *this;
		sign = false;
		}
		
	if (a == 0)
		{
		s = "0";
		}
	else
		{
		while (a > 0)
			{
			int val = (a % base).ToInt32();
			
			if (val < 10)			
				s = STFString((char)('0' + val)) + s;
			else
				s = STFString((char)('A' + val - 10)) + s;
	 		
			a = a / base;
			}
		}
	
	if (sign)
		s = STFString('-') + s;
		
	if (digits)
		{
		while (s.Length() < digits) s = STFString(fill) + s;
		}
	
	return s;
	}

