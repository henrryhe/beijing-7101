#ifndef STFDATAMANIPULATIONMACROS_INC
#define STFDATAMANIPULATIONMACROS_INC
 
#include "STF/Interface/Types/STFBasicTypes.h"

//
// Data extraction from basic types macros
//

#ifndef LOBYTE
#define LOBYTE(w)	    	((uint8)(w))
#endif

#ifndef HIBYTE
#define HIBYTE(w)     	((uint8)(((uint32)(w) >> 8) & 0xFF))
#endif

#ifndef LOWORD
#define LOWORD(l)     	((uint16)(uint32)(l))
#endif

#ifndef HIWORD
#define HIWORD(l)     	((uint16)((((uint32)(l)) >> 16) & 0xFFFF))
#endif

#ifndef LODWORD
#define LODWORD(q)		(*((uint32*)(&q)))
#endif

#ifndef HIDWORD
#define HIDWORD(q)		(((uint32*)(&q))[1])
#endif

#define LBYTE0(w) ((uint8)((w) & 0xff))
#define LBYTE1(w) ((uint8)(((uint32)(w) >> 8) & 0xff))
#define LBYTE2(w) ((uint8)(((uint32)(w) >> 16) & 0xff))
#define LBYTE3(w) ((uint8)(((uint32)(w) >> 24) & 0xff))


//
// Basic type compositions from other basic types
//

#ifndef MAKEWORD
#define MAKEWORD(low, high) ((uint16)(uint8)(low) | ((uint16)(uint8)(high) << 8))
#endif

#ifndef MAKELONG
#define MAKELONG(low, high) ((uint32)(uint16)(low) | ((uint32)(uint16)(high) << 16))
#endif

#define MAKELONG4(low, lmid, hmid, high) ((uint32)(uint8)(low) | ((uint32)(uint8)(lmid) << 8) | ((uint32)(uint8)(hmid) << 16) | ((uint32)(uint8)(high) << 24))


//
// arithmetric macros
//

#ifndef max
#define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif


inline uint32 FLIPENDIAN(uint32 x)
	{
	return MAKELONG4(LBYTE3(x), LBYTE2(x), LBYTE1(x), LBYTE0(x));
	}



// 
// Flag construction macro
//
#define MKFLAG(x)		(1UL << (x))



//
// uint32 Bitfield construction and extraction functions
//

//! Create bitfield by taking specified bits from 32 bit value. 
/*! Position of bits remains unchanged
	\param bit lowest bit to return 
	\param num number of bits to return 
	\param val value to take num bits from bit on.
*/
inline uint32 MKBF(int32 bit, int32 num, uint32 val) 
	{
	return (((uint32)val & ((1UL << num) -1)) << bit);
	}

//! Set bit on specified position in uint32
/*! \param bit bit position to set value to 
	 \param val value to set on this position
*/	
inline uint32 MKBF(int32 bit, bool val) 
	{
	return (val ? (1UL << bit) : 0);
	}

//! Extract specified number of bits from 32 bit value. 
/*! Result will be in lowest num bits.
	\param bit position of lowest bit to get
	\param num number of bits to return 
	\param bf  bitfield from which to extract bits
*/
inline uint32 XTBF(int32 bit, int32 num, uint32 bf) 
	{
	return ((bf >> bit) & ((1UL << num) -1));
	}

//! Extract single specified bit from 32 bit value
/*! 
	\param bit position of lowest bit to get
	\param bf  bitfield from which to extract bit
*/
inline bool XTBF(int32 bit, uint32 bf) 
	{
	return ((bf & (1UL << bit)) != 0);
	}

//! Set a number of bit in a 32 bit bitfield to a specified value
/*!
\	\param bf  bitfield to set bits in 
	\param bit position of lowest bit to set.
	\param num number	of bits to set
	\param val bits to set on this position. We set the lowest 
              num bits from this value to bf.
	@return    the modified bitfield
*/
inline uint32 WRBF(uint32 bf, int32 bit, int32 num, uint32 val)
	{
	uint32 mask = ((1UL << num)-1) << bit;
	return (bf & ~mask) | ((val << bit) & mask);
	}


//! Set a single bit in 32 bit bitfield to a specified value
/*! 
	\param bf  bitfield to set a bit in
	\param bit position of the bit to set
	\param val value to set for specified bit
*/
inline uint32 WRBF(uint32 bf, int32 bit, bool val) 
	{
	return (val ? (bf | (1UL << bit)) : (bf & ~(1UL << bit)));
	}


//
// uint16 Bitfield construction and extraction functions
//

//! Create bitfield by taking specified bits from 16 bit value. 
/*! Position of bits remains unchanged
	\param bit lowest bit to return 
	\param num number of bits to return 
	\param val value to take num bits from bit on.
*/
inline uint16 MKBFW(int32 bit, int32 num, uint16 val) 
	{
	return (((uint16)val & ((1 << num) -1)) << bit);
	}

//! Set bit on specified position in 16 bit value
/*! \param bit bit position to set value to 
	 \param val value to set on this position
*/	
inline uint16 MKBFW(int32 bit, bool val) 
	{
	return (val ? (1 << bit) : 0);
	}

//! Extract specified number of bits from 16 bit value. 
/*! Result will be in lowest num bits.
	\param bit position of lowest bit to get
	\param num number of bits to return 
	\param bf  bitfield from which to extract bits
*/
inline uint16 XTBFW(int32 bit, int32 num, uint16 bf) 
	{
	return ((bf >> bit) & ((1 << num) -1));
	}

//! Extract single specified bit from 16 bit value
/*! 
	\param bit position of lowest bit to get
	\param bf  bitfield from which to extract bit
*/
inline bool XTBFW(int32 bit, uint16 bf) 
	{
	return ((bf & (1 << bit)) != 0);
	}

//! Set a number of bit in a 16 bit bitfield to a specified value
/*!
	\param bf  bitfield to set bits in 
	\param bit position of lowest bit to set.
	\param num number	of bits to set
	\param val bits to set on this position. We set the lowest 
              num bits from this value to bf.
	@return    the modified bitfield
*/

inline uint16 WRBFW(uint16 bf, int32 bit, int32 num, uint16 val)
	{
	uint16 mask = ((1 << num)-1) << bit;
	return (bf & ~mask) | ((val << bit) & mask);
	}

//! Set a single bit in 16 bit bitfield to a specified value
/*! 
	\param bf  bitfield to set a bit in
	\param bit position of the bit to set
	\param val value to set for specified bit
*/
inline uint16 WRBFW(uint16 bf, int32 bit, bool val) 
	{
	return (val ? (bf | (1 << bit)) : (bf & ~(1 << bit)));
	}



//
// uint8 Bitfield construction and extraction functions
//

//! Create bitfield by taking specified bits from 8 bit value. 
/*! Position of bits remains unchanged
	\param bit lowest bit to return 
	\param num number of bits to return 
	\param val value to take num bits from bit on.
*/
inline uint8 MKBFB(int32 bit, int32 num, uint8 val) 
	{
	return (((uint8)val & ((1 << num) -1)) << bit);
	}	 

//! Set bit on specified position in 8 bit value
/*! \param bit bit position to set value to 
	 \param val value to set on this position
*/	
inline uint8 MKBFB(int32 bit, bool val) 
	{
	return (val ? (1 << bit) : 0);
	}

//! Extract specified number of bits from 8 bit value. 
/*! Result will be in lowest num bits.
	\param bit position of lowest bit to get
	\param num number of bits to return 
	\param bf  bitfield from which to extract bits
*/
inline uint8 XTBFB(int32 bit, int32 num, uint8 bf) 
	{
	return ((bf >> bit) & ((1 << num) -1));
	}

//! Extract single specified bit from 8 bit value
/*! 
	\param bit position of lowest bit to get
	\param bf  bitfield from which to extract bit
*/
inline bool XTBFB(int32 bit, uint8 bf) 
	{
	return ((bf & (1 << bit)) != 0);
	}

//! Set a number of bit in a 8 bit bitfield to a specified value
/*!
	\param bf  bitfield to set bits in 
	\param bit position of lowest bit to set.
	\param num number	of bits to set
	\param val bits to set on this position. We set the lowest 
              num bits from this value to bf.
	@return    the modified bitfield
*/
inline uint8 WRBFB(uint8 bf, int32 bit, int32 num, uint8 val)
	{
	uint8 mask = ((1 << num)-1) << bit;
	return (bf & ~mask) | ((val << bit) & mask);
	}

//! Set a single bit in 8 bit bitfield to a specified value
/*! 
	\param bf  bitfield to set a bit in
	\param bit position of the bit to set
	\param val value to set for specified bit
*/
inline uint8 WRBFB(uint8 bf, int32 bit, bool val) 
	{
	return (val ? (bf | (1 << bit)) : (bf & ~(1 << bit)));
	}


//
// Find the most significate one bit or the least significant one bit in a double word
//
inline int32 FindMSB(uint32 bf) {int32 i; for(i=31; i>=0; i--) {if (XTBF(31, bf)) return i; bf <<= 1;} return -1;}
inline int32 FindLSB(uint32 bf) {int32 i; for(i=0; i<=31; i++) {if (XTBF( 0, bf)) return i; bf >>= 1;} return 32;}



//
// Scaling of values of uint16 range to values of uint16 range
// System dependent. Definied in HAL cpp files
//
inline uint16 ScaleWord(uint16 op, uint16 from, uint16 to) {return (uint16)((uint32)op * (uint32) to / (uint32) from);}

uint32 ScaleDWord(uint32 op, uint32 from, uint32 to);

int32 ScaleLong(int32 op, int32 from, int32 to);

uint32 DIV64x32(uint32 upper, uint32 lower, uint32 op);

void MUL32x32(uint32 op1, uint32 op2, uint32 & upper, uint32 & lower);

	
	
//
// Most values in drivers etc. are scaled in a range from 0 to 10000, this functions transfer from and
// to this range
//
inline uint16 ScaleFrom10k(uint16 op, uint16 to) 
	{
	return ScaleWord(op, 10000, to);
	}
inline uint8 ScaleByteFrom10k(uint16 op, uint8 to) 
	{
	return (uint8)ScaleWord(op, 10000, to);
	}
inline uint16 ScaleTo10k(uint16 op, uint16 from) 
	{
	return ScaleWord(op, from, 10000);
	}



#ifndef ONLY_EXTERNAL_VISIBLE

#define FAND(x, y) ScaleWord(x, 10000, y)
#define FAND3(x, y, z) FAND(FAND(x, y), z)

//
// Fix a value inside a boundary
//
inline uint32 BoundTo(uint32 op, uint32 lower, uint32 upper) 
	{
	if (op<lower) 
		return lower; 
		else if (op>upper) 
			return upper; 
		else 
			return op;
	}

#endif

#endif // STFDATAMANIPULATIONMACROS_INC
