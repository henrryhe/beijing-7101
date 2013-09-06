/*
 * jdct.h
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This include file contains common declarations for the forward and
 * inverse DCT modules.  These declarations are private to the DCT managers
 * (jcdctmgr.c, jddctmgr.c) and the individual DCT algorithms.
 * The individual DCT algorithms are kept in separate files to ease 
 * machine-dependent tuning (e.g., assembly coding).
 */


/*
 * A forward DCT routine is given a pointer to a work area of type DCTELEM[];
 * the DCT is to be performed in-place in that buffer.  Type DCTELEM is int
 * for 8-bit samples, INT32 for 12-bit samples.  (NOTE: Floating-point DCT
 * implementations use an array of type FAST__FLOAT, instead.)
 * The DCT inputs are expected to be signed (range +-CENTERJSAMPLE).
 * The DCT outputs are returned scaled up by a factor of 8; they therefore
 * have a range of +-8K for 8-bit data, +-128K for 12-bit data.  This
 * convention improves accuracy in integer implementations and saves some
 * work in floating-point ones.
 * Quantization of the output coefficients is done by jcdctmgr.c.
 */

#if BITS__IN__JSAMPLE == 8
typedef int DCTELEM;		/* 16 or 32 bits is fine */
#else
typedef INT32 DCTELEM;		/* must have 32 bits */
#endif

typedef JMETHOD(void, forward__DCT__method__ptr, (DCTELEM * data));
typedef JMETHOD(void, float__DCT__method__ptr, (FAST__FLOAT * data));


/*
 * An inverse DCT routine is given a pointer to the input JBLOCK and a pointer
 * to an output sample array.  The routine must dequantize the input data as
 * well as perform the IDCT; for dequantization, it uses the multiplier table
 * pointed to by compptr->dct__table.  The output data is to be placed into the
 * sample array starting at a specified column.  (Any row offset needed will
 * be applied to the array pointer before it is passed to the IDCT code.)
 * Note that the number of samples emitted by the IDCT routine is
 * DCT__scaled__size * DCT__scaled__size.
 */

/* typedef inverse__DCT__method__ptr is declared in jpegint.h */

/*
 * Each IDCT routine has its own ideas about the best dct__table element type.
 */

typedef MULTIPLIER ISLOW__MULT__TYPE; /* short or int, whichever is faster */
#if BITS__IN__JSAMPLE == 8
typedef MULTIPLIER IFAST__MULT__TYPE; /* 16 bits is OK, use short if faster */
#define IFAST__SCALE__BITS  2	/* fractional bits in scale factors */
#else
typedef INT32 IFAST__MULT__TYPE;	/* need 32 bits for scaled quantizers */
#define IFAST__SCALE__BITS  13	/* fractional bits in scale factors */
#endif
typedef FAST__FLOAT FLOAT__MULT__TYPE; /* preferred floating type */


/*
 * Each IDCT routine is responsible for range-limiting its results and
 * converting them to unsigned form (0..MAXJSAMPLE).  The raw outputs could
 * be quite far out of range if the input data is corrupt, so a bulletproof
 * range-limiting step is required.  We use a mask-and-table-lookup method
 * to do the combined operations quickly.  See the comments with
 * prepare__range__limit__table (in jdmaster.c) for more info.
 */

#define IDCT__range__limit(cinfo)  ((cinfo)->sample__range__limit + CENTERJSAMPLE)

#define RANGE__MASK  (MAXJSAMPLE * 4 + 3) /* 2 bits wider than legal samples */


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED__SHORT__EXTERNAL__NAMES
#define jpeg__fdct__islow		jFDislow
#define jpeg__fdct__ifast		jFDifast
#define jpeg__fdct__float		jFDfloat
#define jpeg__idct__islow		jRDislow
#define jpeg__idct__ifast		jRDifast
#define jpeg__idct__float		jRDfloat
#define jpeg__idct__4x4		jRD4x4
#define jpeg__idct__2x2		jRD2x2
#define jpeg__idct__1x1		jRD1x1
#endif /* NEED__SHORT__EXTERNAL__NAMES */

/* Extern declarations for the forward and inverse DCT routines. */

EXTERN(void) jpeg__fdct__islow JPP((DCTELEM * data));
EXTERN(void) jpeg__fdct__ifast JPP((DCTELEM * data));
EXTERN(void) jpeg__fdct__float JPP((FAST__FLOAT * data));

EXTERN(void) jpeg__idct__islow
    JPP((j__decompress__ptr cinfo, jpeg__component__info * compptr,
	 JCOEFPTR coef__block, JSAMPARRAY output__buf, JDIMENSION output__col));
EXTERN(void) jpeg__idct__ifast
    JPP((j__decompress__ptr cinfo, jpeg__component__info * compptr,
	 JCOEFPTR coef__block, JSAMPARRAY output__buf, JDIMENSION output__col));
EXTERN(void) jpeg__idct__float
    JPP((j__decompress__ptr cinfo, jpeg__component__info * compptr,
	 JCOEFPTR coef__block, JSAMPARRAY output__buf, JDIMENSION output__col));
EXTERN(void) jpeg__idct__4x4
    JPP((j__decompress__ptr cinfo, jpeg__component__info * compptr,
	 JCOEFPTR coef__block, JSAMPARRAY output__buf, JDIMENSION output__col));
EXTERN(void) jpeg__idct__2x2
    JPP((j__decompress__ptr cinfo, jpeg__component__info * compptr,
	 JCOEFPTR coef__block, JSAMPARRAY output__buf, JDIMENSION output__col));
EXTERN(void) jpeg__idct__1x1
    JPP((j__decompress__ptr cinfo, jpeg__component__info * compptr,
	 JCOEFPTR coef__block, JSAMPARRAY output__buf, JDIMENSION output__col));


/*
 * Macros for handling fixed-point arithmetic; these are used by many
 * but not all of the DCT/IDCT modules.
 *
 * All values are expected to be of type INT32.
 * Fractional constants are scaled left by CONST__BITS bits.
 * CONST__BITS is defined within each module using these macros,
 * and may differ from one module to the next.
 */

#define ONE	((INT32) 1)
#define CONST__SCALE (ONE << CONST__BITS)

/* Convert a positive real constant to an integer scaled by CONST__SCALE.
 * Caution: some C compilers fail to reduce "FIX(constant)" at compile time,
 * thus causing a lot of useless floating-point operations at run time.
 */

#define FIX(x)	((INT32) ((x) * CONST__SCALE + 0.5))

/* Descale and correctly round an INT32 value that's scaled by N bits.
 * We assume RIGHT__SHIFT rounds towards minus infinity, so adding
 * the fudge factor is correct for either sign of X.
 */

#define DESCALE(x,n)  RIGHT__SHIFT((x) + (ONE << ((n)-1)), n)

/* Multiply an INT32 variable by an INT32 constant to yield an INT32 result.
 * This macro is used only when the two inputs will actually be no more than
 * 16 bits wide, so that a 16x16->32 bit multiply can be used instead of a
 * full 32x32 multiply.  This provides a useful speedup on many machines.
 * Unfortunately there is no way to specify a 16x16->32 multiply portably
 * in C, but some C compilers will do the right thing if you provide the
 * correct combination of casts.
 */

#ifdef SHORTxSHORT__32		/* may work if 'int' is 32 bits */
#define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT16) (const)))
#endif
#ifdef SHORTxLCONST__32		/* known to work with Microsoft C 6.0 */
#define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT32) (const)))
#endif

#ifndef MULTIPLY16C16		/* default definition */
#define MULTIPLY16C16(var,const)  ((var) * (const))
#endif

/* Same except both inputs are variables. */

#ifdef SHORTxSHORT__32		/* may work if 'int' is 32 bits */
#define MULTIPLY16V16(var1,var2)  (((INT16) (var1)) * ((INT16) (var2)))
#endif

#ifndef MULTIPLY16V16		/* default definition */
#define MULTIPLY16V16(var1,var2)  ((var1) * (var2))
#endif
