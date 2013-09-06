/*
 * jdhuff.h
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains declarations for Huffman entropy decoding routines
 * that are shared between the sequential decoder (jdhuff.c) and the
 * progressive decoder (jdphuff.c).  No other modules need to see these.
 */

/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED__SHORT__EXTERNAL__NAMES
#define jpeg__make__d__derived__tbl	jMkDDerived
#define jpeg__fill__bit__buffer	jFilBitBuf
#define jpeg__huff__decode	jHufDecode
#endif /* NEED__SHORT__EXTERNAL__NAMES */


/* Derived data constructed for each Huffman table */

#define HUFF__LOOKAHEAD	8	/* # of bits of lookahead */

typedef struct {
  /* Basic tables: (element [0] of each array is unused) */
  INT32 maxcode[18];		/* largest code of length k (-1 if none) */
  /* (maxcode[17] is a sentinel to ensure jpeg__huff__decode terminates) */
  INT32 valoffset[17];		/* huffval[] offset for codes of length k */
  /* valoffset[k] = huffval[] index of 1st symbol of code length k, less
   * the smallest code of length k; so given a code of length k, the
   * corresponding symbol is huffval[code + valoffset[k]]
   */

  /* Link to public Huffman table (needed only in jpeg__huff__decode) */
  JHUFF__TBL *pub;

  /* Lookahead tables: indexed by the next HUFF__LOOKAHEAD bits of
   * the input data stream.  If the next Huffman code is no more
   * than HUFF__LOOKAHEAD bits long, we can obtain its length and
   * the corresponding symbol directly from these tables.
   */
  int look__nbits[1<<HUFF__LOOKAHEAD]; /* # bits, or 0 if too long */
  UINT8 look__sym[1<<HUFF__LOOKAHEAD]; /* symbol, or unused */
} d__derived__tbl;

/* Expand a Huffman table definition into the derived format */
EXTERN(void) jpeg__make__d__derived__tbl
	JPP((j__decompress__ptr cinfo, CH_BOOL isDC, int tblno,
	     d__derived__tbl ** pdtbl));


/*
 * Fetching the next N bits from the input stream is a time-critical operation
 * for the Huffman decoders.  We implement it with a combination of inline
 * macros and out-of-line subroutines.  Note that N (the number of bits
 * demanded at one time) never exceeds 15 for JPEG use.
 *
 * We read source bytes into get__buffer and dole out bits as needed.
 * If get__buffer already contains enough bits, they are fetched in-line
 * by the macros CHECK__BIT__BUFFER and GET__BITS.  When there aren't enough
 * bits, jpeg__fill__bit__buffer is called; it will attempt to fill get__buffer
 * as full as possible (not just to the number of bits needed; this
 * prefetching reduces the overhead cost of calling jpeg__fill__bit__buffer).
 * Note that jpeg__fill__bit__buffer may return FALSE to indicate suspension.
 * On TRUE return, jpeg__fill__bit__buffer guarantees that get__buffer contains
 * at least the requested number of bits --- dummy zeroes are inserted if
 * necessary.
 */

typedef INT32 bit__buf__type;	/* type of bit-extraction buffer */
#define BIT__BUF__SIZE  32	/* size of buffer in bits */

/* If long is > 32 bits on your machine, and shifting/masking longs is
 * reasonably fast, making bit__buf__type be long and setting BIT__BUF__SIZE
 * appropriately should be a win.  Unfortunately we can't define the size
 * with something like  #define BIT__BUF__SIZE (sizeof(bit__buf__type)*8)
 * because not all machines measure sizeof in 8-bit bytes.
 */

typedef struct {		/* Bitreading state saved across MCUs */
  bit__buf__type get__buffer;	/* current bit-extraction buffer */
  int bits__left;		/* # of unused bits in it */
} bitread__perm__state;

typedef struct {		/* Bitreading working state within an MCU */
  /* Current data source location */
  /* We need a copy, rather than munging the original, in case of suspension */
  const JOCTET * next__input__byte; /* => next byte to read from source */
  size_t bytes__in__buffer;	/* # of bytes remaining in source buffer */
  /* Bit input buffer --- note these values are kept in register variables,
   * not in this struct, inside the inner loops.
   */
  bit__buf__type get__buffer;	/* current bit-extraction buffer */
  int bits__left;		/* # of unused bits in it */
  /* Pointer needed by jpeg__fill__bit__buffer. */
  j__decompress__ptr cinfo;	/* back link to decompress master record */
} bitread__working__state;

/* Macros to declare and load/save bitread local variables. */
#define BITREAD__STATE__VARS  \
	register bit__buf__type get__buffer;  \
	register int bits__left;  \
	bitread__working__state br__state

#define BITREAD__LOAD__STATE(cinfop,permstate)  \
	br__state.cinfo = cinfop; \
	br__state.next__input__byte = cinfop->src->next__input__byte; \
	br__state.bytes__in__buffer = cinfop->src->bytes__in__buffer; \
	get__buffer = permstate.get__buffer; \
	bits__left = permstate.bits__left;

#define BITREAD__SAVE__STATE(cinfop,permstate)  \
	cinfop->src->next__input__byte = br__state.next__input__byte; \
	cinfop->src->bytes__in__buffer = br__state.bytes__in__buffer; \
	permstate.get__buffer = get__buffer; \
	permstate.bits__left = bits__left

/*
 * These macros provide the in-line portion of bit fetching.
 * Use CHECK__BIT__BUFFER to ensure there are N bits in get__buffer
 * before using GET__BITS, PEEK__BITS, or DROP__BITS.
 * The variables get__buffer and bits__left are assumed to be locals,
 * but the state struct might not be (jpeg__huff__decode needs this).
 *	CHECK__BIT__BUFFER(state,n,action);
 *		Ensure there are N bits in get__buffer; if suspend, take action.
 *      val = GET__BITS(n);
 *		Fetch next N bits.
 *      val = PEEK__BITS(n);
 *		Fetch next N bits without removing them from the buffer.
 *	DROP__BITS(n);
 *		Discard next N bits.
 * The value N should be a simple variable, not an expression, because it
 * is evaluated multiple times.
 */

#define CHECK__BIT__BUFFER(state,nbits,action) \
	{ if (bits__left < (nbits)) {  \
	    if (! jpeg__fill__bit__buffer(&(state),get__buffer,bits__left,nbits))  \
	      { action; }  \
	    get__buffer = (state).get__buffer; bits__left = (state).bits__left; } }

#define GET__BITS(nbits) \
	(((int) (get__buffer >> (bits__left -= (nbits)))) & ((1<<(nbits))-1))

#define PEEK__BITS(nbits) \
	(((int) (get__buffer >> (bits__left -  (nbits)))) & ((1<<(nbits))-1))

#define DROP__BITS(nbits) \
	(bits__left -= (nbits))

/* Load up the bit buffer to a depth of at least nbits */
EXTERN(CH_BOOL) jpeg__fill__bit__buffer
	JPP((bitread__working__state * state, register bit__buf__type get__buffer,
	     register int bits__left, int nbits));


/*
 * Code for extracting next Huffman-coded symbol from input bit stream.
 * Again, this is time-critical and we make the main paths be macros.
 *
 * We use a lookahead table to process codes of up to HUFF__LOOKAHEAD bits
 * without looping.  Usually, more than 95% of the Huffman codes will be 8
 * or fewer bits long.  The few overlength codes are handled with a loop,
 * which need not be inline code.
 *
 * Notes about the HUFF__DECODE macro:
 * 1. Near the end of the data segment, we may fail to get enough bits
 *    for a lookahead.  In that case, we do it the hard way.
 * 2. If the lookahead table contains no entry, the next code must be
 *    more than HUFF__LOOKAHEAD bits long.
 * 3. jpeg__huff__decode returns -1 if forced to suspend.
 */

#define HUFF__DECODE(result,state,htbl,failaction,slowlabel) \
{ register int nb, look; \
  if (bits__left < HUFF__LOOKAHEAD) { \
    if (! jpeg__fill__bit__buffer(&state,get__buffer,bits__left, 0)) {failaction;} \
    get__buffer = state.get__buffer; bits__left = state.bits__left; \
    if (bits__left < HUFF__LOOKAHEAD) { \
      nb = 1; goto slowlabel; \
    } \
  } \
  look = PEEK__BITS(HUFF__LOOKAHEAD); \
  if ((nb = htbl->look__nbits[look]) != 0) { \
    DROP__BITS(nb); \
    result = htbl->look__sym[look]; \
  } else { \
    nb = HUFF__LOOKAHEAD+1; \
slowlabel: \
    if ((result=jpeg__huff__decode(&state,get__buffer,bits__left,htbl,nb)) < 0) \
	{ failaction; } \
    get__buffer = state.get__buffer; bits__left = state.bits__left; \
  } \
}

/* Out-of-line case for Huffman code fetching */
EXTERN(int) jpeg__huff__decode
	JPP((bitread__working__state * state, register bit__buf__type get__buffer,
	     register int bits__left, d__derived__tbl * htbl, int min__bits));
