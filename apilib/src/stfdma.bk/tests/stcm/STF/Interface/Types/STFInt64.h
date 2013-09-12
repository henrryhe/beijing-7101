///
/// @file	STF/Interface/Types/STFInt64.h
///
/// @brief	datatype for 64bit signed Integer
///
/// @par		OWNER:
/// STF-Team
///
/// @par		SCOPE:
///			EXTERNAL Header File
///
/// this class is a 64-Bit-Integer for all integer arithmetics
///
/// &copy; 2002 ST Microelectronics. All Rights Reserved.
///


#ifndef STFINT64_H
#define STFINT64_H

#include "STF/Interface/Types/STFBasicTypes.h"
#include "STF/Interface/Types/STFString.h"
#include "STF/Interface/STFDataManipulationMacros.h" 

// this defines a new data type as a class,
//! a 64-Bit-Integer data type for all integer arithmetics 
class STFInt64
	{
	private:
		//! lower 32 bit
		uint32 lower;
		//! upper 32 bit
		int32  upper;
	public:
		
		//! empty constructor sets value to zero
		STFInt64(void)
			{
			lower = 0;
			upper = 0;
			}

		//! uint32 constructor, generates a 64-bit value from an uint32
		STFInt64(uint32 val) 
			{
			lower = val; upper = 0;
			}

		//! int32 constructor, with sign.
		/*! upper int32 carries the sign */
		STFInt64(int32 val)
			{
			lower = val;
			upper = (val < 0 ) ? -1 : 0;
			}

		//! copy constructor. dublicates the value
		STFInt64(const STFInt64 & val)
			{
			lower = val.lower;
			upper = val.upper;
			}

		//! assignment operator
		STFInt64 & operator= (const STFInt64 & val)
			{
			lower = val.lower;
			upper = val.upper;
			return *this;
			}
		
		//! construct one STFInt64 from two int
		STFInt64(uint32 lower,int32 upper)
			{
			this->lower = lower;
			this->upper = upper;
			}

		//! construct one STFInt64 from two int
		STFInt64(int32 lower,int32 upper)
			{
			this->lower = lower;
			this->upper = upper;
			}

		//! construct one STFInt64 from two int
		STFInt64(int32 lower,uint32 upper)
			{
			this->lower = lower;
			this->upper = upper;
			}

		//! construct one STFInt64 from two int
		STFInt64(uint32 lower,uint32 upper)
			{
			this->lower = lower;
			this->upper = upper;
			}
/*		
		STFInt64(uint64 val)
			{
			this->lower = (uint32)(val & 0xffffffff);
			this->upper = (uint32)(val >> 32);
			}

		STFInt64(int64 val)
			{
			this->lower = (uint32)(val & 0xffffffff);
			this->upper = (int32)(val >> 32);
			}
*/
		STFInt64(STFString str, int32 base = 10);

		STFString ToString(int32 digits = 0, int32 base = 10, char fill = '0');

		uint32 ToUint32(void) const
			{
			if (upper < 0) 
				return 0;
			else if (upper > 0)
				return 0xffffffff;
			else
				return lower;
			}

		//!convert 64bit value to 32bit value with saturation
		//!in case of overflow a predefined value is returned
		int32 ToInt32(void) const
			{
			if (upper == (int32)0x00000000 && !(lower & 0x80000000))
				return lower; //positive and within range of int32
			else if (upper == (int32)0xffffffff && (lower & 0x80000000))
				return lower; //negative and within range of int32
			else if (upper < 0)
				return 0x80000000; //max_neg
			else
				return 0x7fffffff; //max_pos
			}


		int16 ToInt16(void) const
			{
			int32 tmp;
			tmp = ToInt32();
			if (tmp > 0x7fff)
				return (int16)0x7fff;
			else if(tmp < 0x8000)
				return (int16)0x8000;
			else
				return (int16)lower;
			}
			
		//!return lower part
		uint32 Lower(void) const
			{
			return lower;
			}

		//!return upper part
		int32 Upper(void) const
			{
			return upper;
			}

      STFInt64 Abs(void) const
         {
         if (*this < 0) 
            return (*this * (-1));
         else
            return *this;
         }

		//! The logical-negation (logical-NOT) operator produces the value 0 if its operand 
      //! is true (nonzero) and the value 1 if its operand is false (0). The result has int type. 
      //! The operand must be an integral, floating, or pointer value.
		inline int operator! (void) const {return !lower && !upper;}

		//! minus operator
		inline STFInt64 operator- (void) const;

      // The one's complement operator, sometimes called the "bitwise complement" or "bitwise NOT"
      // operator, produces the bitwise one's complement of its operand. The operand must be of 
      // integral type. This operator performs usual arithmetic conversions; the result has the 
      // type of the operand after conversion. 
		inline STFInt64 operator~ (void) const {return STFInt64(~lower, ~upper);}

      // FRIEND
      // The friend keyword allows a function or class to gain access to the private
      // and protected members of a class. In some circumstances, it is more convenient to grant 
      // member-level access to functions that are not members of a class or to all functions in 
      // a separate class. With the friend keyword, programmers can designate either the specific 
      // functions or the classes whose functions can access not only public members but also protected 
      // and private members

      // integer arithmetic operators for different input types/vaiations
		inline friend STFInt64 operator+ (const STFInt64 & u, const STFInt64 & v);
		inline friend STFInt64 operator- (const STFInt64 & u, const STFInt64 & v);
		inline friend STFInt64 operator+ (const int32 u, const STFInt64 & v);
		inline friend STFInt64 operator- (const int32 u, const STFInt64 & v);
		inline friend STFInt64 operator+ (const STFInt64 & u, const int32 v);
		inline friend STFInt64 operator- (const STFInt64 & u, const int32 v);
		friend STFInt64 operator* (const STFInt64 & u, const STFInt64 & v);
		friend STFInt64 operator/ (const STFInt64 & u, const STFInt64 & v);
		inline friend STFInt64 operator% (const STFInt64 & u, const STFInt64 & v);

		inline STFInt64 & operator+= (const STFInt64 & u);
		inline STFInt64 & operator-= (const STFInt64 & u);
		inline STFInt64 & operator+= (const int32 u);
		inline STFInt64 & operator-= (const int32 u);
		inline STFInt64 & operator*= (const STFInt64 & u);
		inline STFInt64 & operator/= (const STFInt64 & u);
		inline STFInt64 & operator%= (const STFInt64 & u);

		inline STFInt64 & operator++ (void);
		inline STFInt64 & operator-- (void);

      // shift operators
		inline friend STFInt64 operator << (const STFInt64 & u, const int32 shl);
		inline friend STFInt64 operator >> (const STFInt64 & u, const int32 shl);

		inline STFInt64 & operator <<= (const int32 shl);
		inline STFInt64 & operator >>= (const int32 shl);
		
		inline int Compare(const STFInt64 & u) const;

		friend bool operator==(const STFInt64 & u, const STFInt64 & v);
		friend bool operator!=(const STFInt64 & u, const STFInt64 & v);
		friend bool operator<(const STFInt64 & u, const STFInt64 & v);
		friend bool operator>(const STFInt64 & u, const STFInt64 & v);
		friend bool operator<=(const STFInt64 & u, const STFInt64 & v);
		friend bool operator>=(const STFInt64 & u, const STFInt64 & v);

		friend STFInt64 operator& (const STFInt64 & u, const STFInt64 & v) 
			{
			return STFInt64(u.lower & v.lower, u.upper & v.upper);
			}
		friend STFInt64 operator| (const STFInt64 & u, const STFInt64 & v) 
			{
			return STFInt64(u.lower | v.lower, u.upper | v.upper);
			}
		
		STFInt64 & operator&= (const STFInt64 & u) 
			{
			lower &= u.lower; 
			upper &= u.upper; 
			return *this;
			}
		STFInt64 & operator|= (const STFInt64 & u) 
			{
			lower |= u.lower; 
			upper |= u.upper; 
			return *this;
			}		
	};	

inline STFInt64 & STFInt64::operator+= (const STFInt64 & u)
	{	
	lower += u.lower; 
   // check and propagare overflow from lower part up to upper part
	if (lower < u.lower)
      // overflow
		upper += u.upper+1;
	else
      // no overflow
		upper += u.upper;
	return *this;
	}
	
inline STFInt64 & STFInt64::operator-= (const STFInt64 & u)
	{
	uint32 sum = lower - u.lower;

   // check and propagare overflow from lower part up to upper part
	if (sum > lower)
      // overflow...
		upper -= u.upper+1;
	else
      // none...
		upper -= u.upper;
	lower = sum;
	return *this;		
	}

inline STFInt64 & STFInt64::operator+= (const int32 u)
	{
   // check sign of u
	if (u < 0)
      // already use new defined -= operator (see above) and negation operator
      // to invert and subtract u from this (type STFInt64)
		*this -= -u;
	else
		{
		lower += u;
      // check overflow and eventually propagate it to upper part
		if (lower < (uint32)u)
         // yes, overflow
			upper += 1;
		}
	return *this;
	}
	
inline STFInt64 & STFInt64::operator++ (void)
	{
	lower ++;
	if (!lower)
      // pass on overflow upwards...
		upper ++;
	return *this;
	}

inline STFInt64 & STFInt64::operator-- (void)
	{
   // check overflow
	if (!lower)
      // .. and paas on
		upper --;
	lower --;
	return *this;
	}

inline STFInt64 & STFInt64::operator-= (const int32 u)
	{
	if (u < 0)
		*this += -u;
	else
		{
      // check and handle overflow
		uint32 sum = lower - u;
		if (sum > lower)
         // propagate overflow
			upper -= 1;
		lower = sum;
		}
	return *this;		
	}


// the following additions and subtractions is rearranged from the add/sub of two STFInto64 objects
// with each having an upper and lower part to a separate addition of the lower part
// then passing the result to another add/sub  of the two added/subtracted lower parts with the 
// added/subtracted upper parts


// add two STFInt64 objects
inline STFInt64 operator+ (const STFInt64 & u, const STFInt64 & v)
	{
   // add into temporary var
	uint32 sum = u.lower + v.lower;
   // check overflow from lower to upper
	if (sum < u.lower)
      // overflow, so add one to pass this on
		return STFInt64(sum, u.upper + v.upper + 1);
	else
      // no overflow
		return STFInt64(sum, u.upper + v.upper);		
	}

inline STFInt64 operator- (const STFInt64 & u, const STFInt64 & v)
	{
	uint32 sum = u.lower - v.lower;
	if (sum > u.lower)
      // overflow
		return STFInt64(sum, u.upper - v.upper - 1);
	else
      // no overflow
		return STFInt64(sum, u.upper - v.upper);		
	}

inline STFInt64 operator+ (const STFInt64 & u, const int32 v)
	{
	if (v < 0)
		return u - -v;
	else
		{
		uint32 sum = u.lower + v;
		if (sum < u.lower)
			return STFInt64(sum, u.upper + 1);
		else
			return STFInt64(sum, u.upper);		
		}
	}

inline STFInt64 operator- (const STFInt64 & u, const int32 v)
	{
	if (v < 0)
		return u + -v;
	else
		{
		uint32 sum = u.lower - v;
		if (sum > u.lower)
			return STFInt64(sum, u.upper - 1);
		else
			return STFInt64(sum, u.upper);		
		}
	}

	
inline STFInt64 operator+ (const int32 u, const STFInt64 & v)
	{
	if (u < 0)
		return v - -u;
	else
		{
		uint32 sum = u + v.lower;
		if (sum < v.lower)
			return STFInt64(sum, v.upper + 1);
		else
			return STFInt64(sum, v.upper);		
		}
	}

inline STFInt64 operator- (const int32 u, const STFInt64 & v)
	{
   // use negation operation and addition opeartor to define minus operator
	return u + -v;
	}

inline STFInt64 STFInt64::operator- (void) const
	{
	if (lower == 0)
		return STFInt64(0, -upper);
	else
		return STFInt64((uint32)-(int32)lower, ~upper);
	}
	
inline int STFInt64::Compare(const STFInt64 & u) const
	{
	if (upper < u.upper) return -1;
	else if (upper > u.upper) return 1;
	else if (lower < u.lower) return -1;
	else if (lower > u.lower) return 1;
	else return 0;
	}

inline bool operator==(const STFInt64 & u, const STFInt64 & v) 
	{
	return (u.upper == v.upper) && (u.lower == v.lower);
	}

inline bool operator!=(const STFInt64 & u, const STFInt64 & v) 
	{
	return (u.upper != v.upper) || (u.lower != v.lower);
	}

inline bool operator<(const STFInt64 & u, const STFInt64 & v)  
	{
	return (u.upper < v.upper) || (u.upper == v.upper) && (u.lower < v.lower);
	}

inline bool operator>(const STFInt64 & u, const STFInt64 & v)  
	{
	return (u.upper > v.upper) || (u.upper == v.upper) && (u.lower > v.lower);
	}

inline bool operator<=(const STFInt64 & u, const STFInt64 & v) 
	{
	return (u.upper < v.upper) || (u.upper == v.upper) && (u.lower <= v.lower);
	}

inline bool operator>=(const STFInt64 & u, const STFInt64 & v) 
	{
	return (u.upper > v.upper) || (u.upper == v.upper) && (u.lower >= v.lower);
	}		


// define shift operators

// left shift of STFInt64 object for shl bits
inline STFInt64 operator<< (const STFInt64 & u, const int shl)
	{
   // use definition of <<= operator to do this

   // first make a copy of u to do the shift with
	STFInt64 v = u;
	v <<= shl;
	return v;
	}

// right shift of STFInt64 object for shr bits	
inline STFInt64 operator>> (const STFInt64 & u, const int32 shr)
	{
   // use definition of >>= operator to do this

   // do shift with copy
	STFInt64 v = u;
	v >>= shr;
	return v;
	}

inline STFInt64 & STFInt64::operator*= (const STFInt64 & u)
	{
   // use "normal" mult to do *=
	*this = *this * u;
	return *this;
	}
	
inline STFInt64 & STFInt64::operator/= (const STFInt64 & u)
	{
   // use normal division to do /=
	*this = *this / u;
	return *this;
	}
	
inline STFInt64 & STFInt64::operator%= (const STFInt64 & u)
	{
   // use normal modulo op to do %=
	*this = *this % u;
	return *this;
	}
	
inline STFInt64 operator % (const STFInt64 & u, const STFInt64 & v)
	{
   // normal modulo op is done with / and *
	return u - (u / v) * v;
	}
	
inline STFInt64 & STFInt64::operator<<= (const int32 shl)
	{
	int32 s = shl;
	
   // shift data shl times left by 1 
	while (s > 0)
		{
      // upper part left 1 bit, lowest bit becomes always 0
		upper <<= 1;
      // check lower part before shift: if highest bit is set, carry this to upper part
      // i.e. set lowest bit of upper part
		if (lower & 0x80000000) upper |= 1;
		lower <<= 1;
		s--;
		}
		
	return *this;
	}
	
inline STFInt64 & STFInt64::operator>>= (const int32 shl)
	{
	int32 s = shl;
	
   // lower part right by 1, highest bit becomes always 0
	while (s > 0)
		{
		lower >>= 1;
      // check lowest bit of upper part, and take carry to highest bit of lower part
		if (upper & 0x00000001) lower |= 0x80000000;
		upper >>= 1;
		s--;
		}
	
	return *this;
	}



#endif //STFINT64_H
