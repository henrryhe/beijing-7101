/*==========================================================================
  Mediaguard Conditional Access Kernel - Core Library
 *--------------------------------------------------------------------------

  %name: General.h %
  %version: 15 %
  %instance: STB_2 %
  %date_created: Fri Jun 17 10:43:53 2005 %

 *--------------------------------------------------------------------------

  DDI.Core.IDD  ver. 1.0 Rev. H
  API.Core.IDD	ver. 1.0 Rev. K

 *--------------------------------------------------------------------------

  General definitions.

 *==========================================================================*/

#ifndef _GENERAL_H_
#define _GENERAL_H_

/*==========================================================================
  Includes
 *==========================================================================*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stlite.h>
/*==========================================================================
  Basic defines and types
 *==========================================================================*/

/* NULL => Pointeur
 *--------------------------------------------------------------------------*/

#undef NULL
#define NULL 	0

/* boolean: TRUE, FALSE
 *--------------------------------------------------------------------------*/

#undef FALSE
#define FALSE 	( (boolean)0 )
#undef TRUE
#define TRUE 	( (boolean)1 )

/* Basic types
 *--------------------------------------------------------------------------*/

typedef signed char 	byte;
typedef unsigned char	ubyte;
typedef signed short	word;
typedef unsigned short	uword;
typedef signed long		dword;
typedef unsigned long	udword;
#if ( ULONG_MAX < 0xFFFFFFFFUL )
#error The platform must support a 32 bit data type to implement
#endif

typedef ubyte			boolean;/**/
typedef void*			HANDLE;

/* Common API and DDI definitions
 *--------------------------------------------------------------------------*/

/* Source system handle */
typedef struct sMGSysSrcHandle*		TMGSysSrcHandle;
/* Smart card Reader system handle */
typedef struct sMGSysSCRdrHandle*	TMGSysSCRdrHandle;

/* Event notification callback prototype
 *--------------------------------------------------------------------------*/

typedef void (*CALLBACK)( HANDLE Handle, byte EventCode, word ExCode, dword EventData );

/* Generic table
 *--------------------------------------------------------------------------*/

typedef struct
{
	uword n;
	union 
	{
		ubyte* lBytes;
		uword* lWords;
		udword* lDWords;
	} Table;
} TMGTable;

/* Bitfield operation defined as macro-instruction
 *--------------------------------------------------------------------------*/

typedef ubyte					BitField08;
typedef uword					BitField16;
typedef udword					BitField32;

#define BitMask1( n )			( 1U << (n) )
#define BitMask0( n )			( ~BitMask1( n ) )

#define SetBit( x, n )			x |= BitMask1( n )
#define ClrBit( x, n )			x &= BitMask0( n )
#define GetBit( x, n )			( (x) & BitMask1( n ) )
#define TstBit( x, n )			GetBit( x, n )

#define SetBits( x, m )			x |= m
#define ClrBits( x, m )			x &= ~(m)
#define GetBits( x, m )			( (x) & (m) )
#define SetBitsVal( x, m, v )	ClrBits( x, m ); SetBits( x, ((m)&(v)) )
#define TstBits( x, m, v )		( !( ((x)^(v)) & (m) ) )

#define LoByte( w )				GetBits( ( (uword)(w) ), 0x00FF )
#define HiByte( w )				GetBits( ( (uword)(w) >> 8 ), 0x00FF )
#define LoWord( dw )		 	GetBits( ( (udword)(dw) ), 0x0000FFFF )
#define HiWord( dw )			GetBits( ( (udword)(dw) >> 16 ), 0x0000FFFF )

/* MIN, MAX
 *--------------------------------------------------------------------------*/

#undef MAX
#define MAX( a, b )				( ( (a) > (b) ) ? (a) : (b) )
#undef MIN
#define MIN( a, b )				( ( (a) < (b) ) ? (a) : (b) )

/*==========================================================================
  Exported prototypes
 *==========================================================================*/

extern int GetBitIndex( register unsigned int value );

/*==========================================================================*/

#endif /* _GENERAL_H_ */

/*==========================================================================
   END OF FILE
 *==========================================================================*/
