/*
 * jchuff.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy encoding routines.
 *
 * Much of the complexity here has to do with supporting output suspension.
 * If the data destination module demands suspension, we want to be able to
 * back up to the start of the current MCU.  To do this, we copy state
 * variables into local working storage, and update them back to the
 * permanent JPEG objects only upon successful completion of an MCU.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jchuff.h"		/* Declarations shared with jcphuff.c */


/* Expanded entropy encoder object for Huffman encoding.
 *
 * The savable__state subrecord contains fields that change within an MCU,
 * but must not be updated permanently until we complete the MCU.
 */

typedef struct {
  INT32 put__buffer;		/* current bit-accumulation buffer */
  int put__bits;			/* # of bits now in it */
  int last__dc__val[MAX__COMPS__IN__SCAN]; /* last DC coef for each component */
} savable__state;

/* This macro is to work around compilers with missing or broken
 * structure assignment.  You'll need to fix this code if you have
 * such a compiler and you change MAX__COMPS__IN__SCAN.
 */

#ifndef NO__STRUCT__ASSIGN
#define ASSIGN__STATE(dest,src)  ((dest) = (src))
#else
#if MAX__COMPS__IN__SCAN == 4
#define ASSIGN__STATE(dest,src)  \
	((dest).put__buffer = (src).put__buffer, \
	 (dest).put__bits = (src).put__bits, \
	 (dest).last__dc__val[0] = (src).last__dc__val[0], \
	 (dest).last__dc__val[1] = (src).last__dc__val[1], \
	 (dest).last__dc__val[2] = (src).last__dc__val[2], \
	 (dest).last__dc__val[3] = (src).last__dc__val[3])
#endif
#endif


typedef struct {
  struct jpeg__entropy__encoder pub; /* public fields */

  savable__state saved;		/* Bit buffer & DC state at start of MCU */

  /* These fields are NOT loaded into local working state. */
  unsigned int restarts__to__go;	/* MCUs left in this restart interval */
  int next__restart__num;		/* next restart number to write (0-7) */

  /* Pointers to derived tables (these workspaces have image lifespan) */
  c__derived__tbl * dc__derived__tbls[NUM__HUFF__TBLS];
  c__derived__tbl * ac__derived__tbls[NUM__HUFF__TBLS];

#ifdef ENTROPY__OPT__SUPPORTED	/* Statistics tables for optimization */
  long * dc__count__ptrs[NUM__HUFF__TBLS];
  long * ac__count__ptrs[NUM__HUFF__TBLS];
#endif
} huff__entropy__encoder;

typedef huff__entropy__encoder * huff__entropy__ptr;

/* Working state while writing an MCU.
 * This struct contains all the fields that are needed by subroutines.
 */

typedef struct {
  JOCTET * next__output__byte;	/* => next byte to write in buffer */
  size_t free__in__buffer;	/* # of byte spaces remaining in buffer */
  savable__state cur;		/* Current bit buffer & DC state */
  j__compress__ptr cinfo;		/* dump__buffer needs access to this */
} working__state;


/* Forward declarations */
METHODDEF(CH_BOOL) encode__mcu__huff JPP((j__compress__ptr cinfo,
					JBLOCKROW *MCU__data));
METHODDEF(void) finish__pass__huff JPP((j__compress__ptr cinfo));
#ifdef ENTROPY__OPT__SUPPORTED
METHODDEF(CH_BOOL) encode__mcu__gather JPP((j__compress__ptr cinfo,
					  JBLOCKROW *MCU__data));
METHODDEF(void) finish__pass__gather JPP((j__compress__ptr cinfo));
#endif


/*
 * Initialize for a Huffman-compressed scan.
 * If gather__statistics is TRUE, we do not output anything during the scan,
 * just count the Huffman symbols used and generate Huffman code tables.
 */

METHODDEF(void)
start__pass__huff (j__compress__ptr cinfo, CH_BOOL gather__statistics)
{
  huff__entropy__ptr entropy = (huff__entropy__ptr) cinfo->entropy;
  int ci, dctbl, actbl;
  jpeg__component__info * compptr;

  if (gather__statistics) {
#ifdef ENTROPY__OPT__SUPPORTED
    entropy->pub.encode__mcu = encode__mcu__gather;
    entropy->pub.finish__pass = finish__pass__gather;
#else
    ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
  } else {
    entropy->pub.encode__mcu = encode__mcu__huff;
    entropy->pub.finish__pass = finish__pass__huff;
  }

  for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
    compptr = cinfo->cur__comp__info[ci];
    dctbl = compptr->dc__tbl__no;
    actbl = compptr->ac__tbl__no;
    if (gather__statistics) {
#ifdef ENTROPY__OPT__SUPPORTED
      /* Check for invalid table indexes */
      /* (make__c__derived__tbl does this in the other path) */
      if (dctbl < 0 || dctbl >= NUM__HUFF__TBLS)
	ERREXIT1(cinfo, JERR__NO__HUFF__TABLE, dctbl);
      if (actbl < 0 || actbl >= NUM__HUFF__TBLS)
	ERREXIT1(cinfo, JERR__NO__HUFF__TABLE, actbl);
      /* Allocate and zero the statistics tables */
      /* Note that jpeg__gen__optimal__table expects 257 entries in each table! */
      if (entropy->dc__count__ptrs[dctbl] == NULL)
	entropy->dc__count__ptrs[dctbl] = (long *)
	  (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				      257 * SIZEOF(long));
      MEMZERO(entropy->dc__count__ptrs[dctbl], 257 * SIZEOF(long));
      if (entropy->ac__count__ptrs[actbl] == NULL)
	entropy->ac__count__ptrs[actbl] = (long *)
	  (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				      257 * SIZEOF(long));
      MEMZERO(entropy->ac__count__ptrs[actbl], 257 * SIZEOF(long));
#endif
    } else {
      /* Compute derived values for Huffman tables */
      /* We may do this more than once for a table, but it's not expensive */
      jpeg__make__c__derived__tbl(cinfo, TRUE, dctbl,
			      & entropy->dc__derived__tbls[dctbl]);
      jpeg__make__c__derived__tbl(cinfo, FALSE, actbl,
			      & entropy->ac__derived__tbls[actbl]);
    }
    /* Initialize DC predictions to 0 */
    entropy->saved.last__dc__val[ci] = 0;
  }

  /* Initialize bit buffer to empty */
  entropy->saved.put__buffer = 0;
  entropy->saved.put__bits = 0;

  /* Initialize restart stuff */
  entropy->restarts__to__go = cinfo->restart__interval;
  entropy->next__restart__num = 0;
}


/*
 * Compute the derived values for a Huffman table.
 * This routine also performs some validation checks on the table.
 *
 * Note this is also used by jcphuff.c.
 */

GLOBAL(void)
jpeg__make__c__derived__tbl (j__compress__ptr cinfo, CH_BOOL isDC, int tblno,
			 c__derived__tbl ** pdtbl)
{
  JHUFF__TBL *htbl;
  c__derived__tbl *dtbl;
  int p, i, l, lastp, si, maxsymbol;
  char huffsize[257];
  unsigned int huffcode[257];
  unsigned int code;

  /* Note that huffsize[] and huffcode[] are filled in code-length order,
   * paralleling the order of the symbols themselves in htbl->huffval[].
   */

  /* Find the input Huffman table */
  if (tblno < 0 || tblno >= NUM__HUFF__TBLS)
    ERREXIT1(cinfo, JERR__NO__HUFF__TABLE, tblno);
  htbl =
    isDC ? cinfo->dc__huff__tbl__ptrs[tblno] : cinfo->ac__huff__tbl__ptrs[tblno];
  if (htbl == NULL)
    ERREXIT1(cinfo, JERR__NO__HUFF__TABLE, tblno);

  /* Allocate a workspace if we haven't already done so. */
  if (*pdtbl == NULL)
    *pdtbl = (c__derived__tbl *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  SIZEOF(c__derived__tbl));
  dtbl = *pdtbl;
  
  /* Figure C.1: make table of Huffman code length for each symbol */

  p = 0;
  for (l = 1; l <= 16; l++) {
    i = (int) htbl->bits[l];
    if (i < 0 || p + i > 256)	/* protect against table overrun */
      ERREXIT(cinfo, JERR__BAD__HUFF__TABLE);
    while (i--)
      huffsize[p++] = (char) l;
  }
  huffsize[p] = 0;
  lastp = p;
  
  /* Figure C.2: generate the codes themselves */
  /* We also validate that the counts represent a legal Huffman code tree. */

  code = 0;
  si = huffsize[0];
  p = 0;
  while (huffsize[p]) {
    while (((int) huffsize[p]) == si) {
      huffcode[p++] = code;
      code++;
    }
    /* code is now 1 more than the last code used for codelength si; but
     * it must still fit in si bits, since no code is allowed to be all ones.
     */
    if (((INT32) code) >= (((INT32) 1) << si))
      ERREXIT(cinfo, JERR__BAD__HUFF__TABLE);
    code <<= 1;
    si++;
  }
  
  /* Figure C.3: generate encoding tables */
  /* These are code and size indexed by symbol value */

  /* Set all codeless symbols to have code length 0;
   * this lets us detect duplicate VAL entries here, and later
   * allows emit__bits to detect any attempt to emit such symbols.
   */
  MEMZERO(dtbl->ehufsi, SIZEOF(dtbl->ehufsi));

  /* This is also a convenient place to check for out-of-range
   * and duplicated VAL entries.  We allow 0..255 for AC symbols
   * but only 0..15 for DC.  (We could constrain them further
   * based on data depth and mode, but this seems enough.)
   */
  maxsymbol = isDC ? 15 : 255;

  for (p = 0; p < lastp; p++) {
    i = htbl->huffval[p];
    if (i < 0 || i > maxsymbol || dtbl->ehufsi[i])
      ERREXIT(cinfo, JERR__BAD__HUFF__TABLE);
    dtbl->ehufco[i] = huffcode[p];
    dtbl->ehufsi[i] = huffsize[p];
  }
}


/* Outputting bytes to the file */

/* Emit a byte, taking 'action' if must suspend. */
#define emit__byte(state,val,action)  \
	{ *(state)->next__output__byte++ = (JOCTET) (val);  \
	  if (--(state)->free__in__buffer == 0)  \
	    if (! dump__buffer(state))  \
	      { action; } }


LOCAL(CH_BOOL)
dump__buffer (working__state * state)
/* Empty the output buffer; return TRUE if successful, FALSE if must suspend */
{
  struct jpeg__destination__mgr * dest = state->cinfo->dest;

  if (! (*dest->empty__output__buffer) (state->cinfo))
    return FALSE;
  /* After a successful buffer dump, must reset buffer pointers */
  state->next__output__byte = dest->next__output__byte;
  state->free__in__buffer = dest->free__in__buffer;
  return TRUE;
}


/* Outputting bits to the file */

/* Only the right 24 bits of put__buffer are used; the valid bits are
 * left-justified in this part.  At most 16 bits can be passed to emit__bits
 * in one call, and we never retain more than 7 bits in put__buffer
 * between calls, so 24 bits are sufficient.
 */

INLINE
LOCAL(CH_BOOL)
emit__bits (working__state * state, unsigned int code, int size)
/* Emit some bits; return TRUE if successful, FALSE if must suspend */
{
  /* This routine is heavily used, so it's worth coding tightly. */
  register INT32 put__buffer = (INT32) code;
  register int put__bits = state->cur.put__bits;

  /* if size is 0, caller used an invalid Huffman table entry */
  if (size == 0)
    ERREXIT(state->cinfo, JERR__HUFF__MISSING__CODE);

  put__buffer &= (((INT32) 1)<<size) - 1; /* mask off any extra bits in code */
  
  put__bits += size;		/* new number of bits in buffer */
  
  put__buffer <<= 24 - put__bits; /* align incoming bits */

  put__buffer |= state->cur.put__buffer; /* and merge with old buffer contents */
  
  while (put__bits >= 8) {
    int c = (int) ((put__buffer >> 16) & 0xFF);
    
    emit__byte(state, c, return FALSE);
    if (c == 0xFF) {		/* need to stuff a zero byte? */
      emit__byte(state, 0, return FALSE);
    }
    put__buffer <<= 8;
    put__bits -= 8;
  }

  state->cur.put__buffer = put__buffer; /* update state variables */
  state->cur.put__bits = put__bits;

  return TRUE;
}


LOCAL(CH_BOOL)
flush__bits (working__state * state)
{
  if (! emit__bits(state, 0x7F, 7)) /* fill any partial byte with ones */
    return FALSE;
  state->cur.put__buffer = 0;	/* and reset bit-buffer to empty */
  state->cur.put__bits = 0;
  return TRUE;
}


/* Encode a single block's worth of coefficients */

LOCAL(CH_BOOL)
encode__one__block (working__state * state, JCOEFPTR block, int last__dc__val,
		  c__derived__tbl *dctbl, c__derived__tbl *actbl)
{
  register int temp, temp2;
  register int nbits;
  register int k, r, i;
  
  /* Encode the DC coefficient difference per section F.1.2.1 */
  
  temp = temp2 = block[0] - last__dc__val;

  if (temp < 0) {
    temp = -temp;		/* temp is abs value of input */
    /* For a negative input, want temp2 = bitwise complement of abs(input) */
    /* This code assumes we are on a two's complement machine */
    temp2--;
  }
  
  /* Find the number of bits needed for the magnitude of the coefficient */
  nbits = 0;
  while (temp) {
    nbits++;
    temp >>= 1;
  }
  /* Check for out-of-range coefficient values.
   * Since we're encoding a difference, the range limit is twice as much.
   */
  if (nbits > MAX__COEF__BITS+1)
    ERREXIT(state->cinfo, JERR__BAD__DCT__COEF);
  
  /* Emit the Huffman-coded symbol for the number of bits */
  if (! emit__bits(state, dctbl->ehufco[nbits], dctbl->ehufsi[nbits]))
    return FALSE;

  /* Emit that number of bits of the value, if positive, */
  /* or the complement of its magnitude, if negative. */
  if (nbits)			/* emit__bits rejects calls with size 0 */
    if (! emit__bits(state, (unsigned int) temp2, nbits))
      return FALSE;

  /* Encode the AC coefficients per section F.1.2.2 */
  
  r = 0;			/* r = run length of zeros */
  
  for (k = 1; k < DCTSIZE2; k++) {
    if ((temp = block[jpeg__natural__order[k]]) == 0) {
      r++;
    } else {
      /* if run length > 15, must emit special run-length-16 codes (0xF0) */
      while (r > 15) {
	if (! emit__bits(state, actbl->ehufco[0xF0], actbl->ehufsi[0xF0]))
	  return FALSE;
	r -= 16;
      }

      temp2 = temp;
      if (temp < 0) {
	temp = -temp;		/* temp is abs value of input */
	/* This code assumes we are on a two's complement machine */
	temp2--;
      }
      
      /* Find the number of bits needed for the magnitude of the coefficient */
      nbits = 1;		/* there must be at least one 1 bit */
      while ((temp >>= 1))
	nbits++;
      /* Check for out-of-range coefficient values */
      if (nbits > MAX__COEF__BITS)
	ERREXIT(state->cinfo, JERR__BAD__DCT__COEF);
      
      /* Emit Huffman symbol for run length / number of bits */
      i = (r << 4) + nbits;
      if (! emit__bits(state, actbl->ehufco[i], actbl->ehufsi[i]))
	return FALSE;

      /* Emit that number of bits of the value, if positive, */
      /* or the complement of its magnitude, if negative. */
      if (! emit__bits(state, (unsigned int) temp2, nbits))
	return FALSE;
      
      r = 0;
    }
  }

  /* If the last coef(s) were zero, emit an end-of-block code */
  if (r > 0)
    if (! emit__bits(state, actbl->ehufco[0], actbl->ehufsi[0]))
      return FALSE;

  return TRUE;
}


/*
 * Emit a restart marker & resynchronize predictions.
 */

LOCAL(CH_BOOL)
emit__restart (working__state * state, int restart__num)
{
  int ci;

  if (! flush__bits(state))
    return FALSE;

  emit__byte(state, 0xFF, return FALSE);
  emit__byte(state, JPEG__RST0 + restart__num, return FALSE);

  /* Re-initialize DC predictions to 0 */
  for (ci = 0; ci < state->cinfo->comps__in__scan; ci++)
    state->cur.last__dc__val[ci] = 0;

  /* The restart counter is not updated until we successfully write the MCU. */

  return TRUE;
}


/*
 * Encode and output one MCU's worth of Huffman-compressed coefficients.
 */

METHODDEF(CH_BOOL)
encode__mcu__huff (j__compress__ptr cinfo, JBLOCKROW *MCU__data)
{
  huff__entropy__ptr entropy = (huff__entropy__ptr) cinfo->entropy;
  working__state state;
  int blkn, ci;
  jpeg__component__info * compptr;

  /* Load up working state */
  state.next__output__byte = cinfo->dest->next__output__byte;
  state.free__in__buffer = cinfo->dest->free__in__buffer;
  ASSIGN__STATE(state.cur, entropy->saved);
  state.cinfo = cinfo;

  /* Emit restart marker if needed */
  if (cinfo->restart__interval) {
    if (entropy->restarts__to__go == 0)
      if (! emit__restart(&state, entropy->next__restart__num))
	return FALSE;
  }

  /* Encode the MCU data blocks */
  for (blkn = 0; blkn < cinfo->blocks__in__MCU; blkn++) {
    ci = cinfo->MCU__membership[blkn];
    compptr = cinfo->cur__comp__info[ci];
    if (! encode__one__block(&state,
			   MCU__data[blkn][0], state.cur.last__dc__val[ci],
			   entropy->dc__derived__tbls[compptr->dc__tbl__no],
			   entropy->ac__derived__tbls[compptr->ac__tbl__no]))
      return FALSE;
    /* Update last__dc__val */
    state.cur.last__dc__val[ci] = MCU__data[blkn][0][0];
  }

  /* Completed MCU, so update state */
  cinfo->dest->next__output__byte = state.next__output__byte;
  cinfo->dest->free__in__buffer = state.free__in__buffer;
  ASSIGN__STATE(entropy->saved, state.cur);

  /* Update restart-interval state too */
  if (cinfo->restart__interval) {
    if (entropy->restarts__to__go == 0) {
      entropy->restarts__to__go = cinfo->restart__interval;
      entropy->next__restart__num++;
      entropy->next__restart__num &= 7;
    }
    entropy->restarts__to__go--;
  }

  return TRUE;
}


/*
 * Finish up at the end of a Huffman-compressed scan.
 */

METHODDEF(void)
finish__pass__huff (j__compress__ptr cinfo)
{
  huff__entropy__ptr entropy = (huff__entropy__ptr) cinfo->entropy;
  working__state state;

  /* Load up working state ... flush__bits needs it */
  state.next__output__byte = cinfo->dest->next__output__byte;
  state.free__in__buffer = cinfo->dest->free__in__buffer;
  ASSIGN__STATE(state.cur, entropy->saved);
  state.cinfo = cinfo;

  /* Flush out the last data */
  if (! flush__bits(&state))
    ERREXIT(cinfo, JERR__CANT__SUSPEND);

  /* Update state */
  cinfo->dest->next__output__byte = state.next__output__byte;
  cinfo->dest->free__in__buffer = state.free__in__buffer;
  ASSIGN__STATE(entropy->saved, state.cur);
}


/*
 * Huffman coding optimization.
 *
 * We first scan the supplied data and count the number of uses of each symbol
 * that is to be Huffman-coded. (This process MUST agree with the code above.)
 * Then we build a Huffman coding tree for the observed counts.
 * Symbols which are not needed at all for the particular image are not
 * assigned any code, which saves space in the DHT marker as well as in
 * the compressed data.
 */

#ifdef ENTROPY__OPT__SUPPORTED


/* Process a single block's worth of coefficients */

LOCAL(void)
htest__one__block (j__compress__ptr cinfo, JCOEFPTR block, int last__dc__val,
		 long dc__counts[], long ac__counts[])
{
  register int temp;
  register int nbits;
  register int k, r;
  
  /* Encode the DC coefficient difference per section F.1.2.1 */
  
  temp = block[0] - last__dc__val;
  if (temp < 0)
    temp = -temp;
  
  /* Find the number of bits needed for the magnitude of the coefficient */
  nbits = 0;
  while (temp) {
    nbits++;
    temp >>= 1;
  }
  /* Check for out-of-range coefficient values.
   * Since we're encoding a difference, the range limit is twice as much.
   */
  if (nbits > MAX__COEF__BITS+1)
    ERREXIT(cinfo, JERR__BAD__DCT__COEF);

  /* Count the Huffman symbol for the number of bits */
  dc__counts[nbits]++;
  
  /* Encode the AC coefficients per section F.1.2.2 */
  
  r = 0;			/* r = run length of zeros */
  
  for (k = 1; k < DCTSIZE2; k++) {
    if ((temp = block[jpeg__natural__order[k]]) == 0) {
      r++;
    } else {
      /* if run length > 15, must emit special run-length-16 codes (0xF0) */
      while (r > 15) {
	ac__counts[0xF0]++;
	r -= 16;
      }
      
      /* Find the number of bits needed for the magnitude of the coefficient */
      if (temp < 0)
	temp = -temp;
      
      /* Find the number of bits needed for the magnitude of the coefficient */
      nbits = 1;		/* there must be at least one 1 bit */
      while ((temp >>= 1))
	nbits++;
      /* Check for out-of-range coefficient values */
      if (nbits > MAX__COEF__BITS)
	ERREXIT(cinfo, JERR__BAD__DCT__COEF);
      
      /* Count Huffman symbol for run length / number of bits */
      ac__counts[(r << 4) + nbits]++;
      
      r = 0;
    }
  }

  /* If the last coef(s) were zero, emit an end-of-block code */
  if (r > 0)
    ac__counts[0]++;
}


/*
 * Trial-encode one MCU's worth of Huffman-compressed coefficients.
 * No data is actually output, so no suspension return is possible.
 */

METHODDEF(CH_BOOL)
encode__mcu__gather (j__compress__ptr cinfo, JBLOCKROW *MCU__data)
{
  huff__entropy__ptr entropy = (huff__entropy__ptr) cinfo->entropy;
  int blkn, ci;
  jpeg__component__info * compptr;

  /* Take care of restart intervals if needed */
  if (cinfo->restart__interval) {
    if (entropy->restarts__to__go == 0) {
      /* Re-initialize DC predictions to 0 */
      for (ci = 0; ci < cinfo->comps__in__scan; ci++)
	entropy->saved.last__dc__val[ci] = 0;
      /* Update restart state */
      entropy->restarts__to__go = cinfo->restart__interval;
    }
    entropy->restarts__to__go--;
  }

  for (blkn = 0; blkn < cinfo->blocks__in__MCU; blkn++) {
    ci = cinfo->MCU__membership[blkn];
    compptr = cinfo->cur__comp__info[ci];
    htest__one__block(cinfo, MCU__data[blkn][0], entropy->saved.last__dc__val[ci],
		    entropy->dc__count__ptrs[compptr->dc__tbl__no],
		    entropy->ac__count__ptrs[compptr->ac__tbl__no]);
    entropy->saved.last__dc__val[ci] = MCU__data[blkn][0][0];
  }

  return TRUE;
}


/*
 * Generate the best Huffman code table for the given counts, fill htbl.
 * Note this is also used by jcphuff.c.
 *
 * The JPEG standard requires that no symbol be assigned a codeword of all
 * one bits (so that padding bits added at the end of a compressed segment
 * can't look like a valid code).  Because of the canonical ordering of
 * codewords, this just means that there must be an unused slot in the
 * longest codeword length category.  Section K.2 of the JPEG spec suggests
 * reserving such a slot by pretending that symbol 256 is a valid symbol
 * with count 1.  In theory that's not optimal; giving it count zero but
 * including it in the symbol set anyway should give a better Huffman code.
 * But the theoretically better code actually seems to come out worse in
 * practice, because it produces more all-ones bytes (which incur stuffed
 * zero bytes in the final file).  In any case the difference is tiny.
 *
 * The JPEG standard requires Huffman codes to be no more than 16 bits long.
 * If some symbols have a very small but nonzero probability, the Huffman tree
 * must be adjusted to meet the code length restriction.  We currently use
 * the adjustment method suggested in JPEG section K.2.  This method is *not*
 * optimal; it may not choose the best possible limited-length code.  But
 * typically only very-low-frequency symbols will be given less-than-optimal
 * lengths, so the code is almost optimal.  Experimental comparisons against
 * an optimal limited-length-code algorithm indicate that the difference is
 * microscopic --- usually less than a hundredth of a percent of total size.
 * So the extra complexity of an optimal algorithm doesn't seem worthwhile.
 */

GLOBAL(void)
jpeg__gen__optimal__table (j__compress__ptr cinfo, JHUFF__TBL * htbl, long freq[])
{
#define MAX__CLEN 32		/* assumed maximum initial code length */
  UINT8 bits[MAX__CLEN+1];	/* bits[k] = # of symbols with code length k */
  int codesize[257];		/* codesize[k] = code length of symbol k */
  int others[257];		/* next symbol in current branch of tree */
  int c1, c2;
  int p, i, j;
  long v;

  /* This algorithm is explained in section K.2 of the JPEG standard */

  MEMZERO(bits, SIZEOF(bits));
  MEMZERO(codesize, SIZEOF(codesize));
  for (i = 0; i < 257; i++)
    others[i] = -1;		/* init links to empty */
  
  freq[256] = 1;		/* make sure 256 has a nonzero count */
  /* Including the pseudo-symbol 256 in the Huffman procedure guarantees
   * that no real symbol is given code-value of all ones, because 256
   * will be placed last in the largest codeword category.
   */

  /* Huffman's basic algorithm to assign optimal code lengths to symbols */

  for (;;) {
    /* Find the smallest nonzero frequency, set c1 = its symbol */
    /* In case of ties, take the larger symbol number */
    c1 = -1;
    v = 1000000000L;
    for (i = 0; i <= 256; i++) {
      if (freq[i] && freq[i] <= v) {
	v = freq[i];
	c1 = i;
      }
    }

    /* Find the next smallest nonzero frequency, set c2 = its symbol */
    /* In case of ties, take the larger symbol number */
    c2 = -1;
    v = 1000000000L;
    for (i = 0; i <= 256; i++) {
      if (freq[i] && freq[i] <= v && i != c1) {
	v = freq[i];
	c2 = i;
      }
    }

    /* Done if we've merged everything into one frequency */
    if (c2 < 0)
      break;
    
    /* Else merge the two counts/trees */
    freq[c1] += freq[c2];
    freq[c2] = 0;

    /* Increment the codesize of everything in c1's tree branch */
    codesize[c1]++;
    while (others[c1] >= 0) {
      c1 = others[c1];
      codesize[c1]++;
    }
    
    others[c1] = c2;		/* chain c2 onto c1's tree branch */
    
    /* Increment the codesize of everything in c2's tree branch */
    codesize[c2]++;
    while (others[c2] >= 0) {
      c2 = others[c2];
      codesize[c2]++;
    }
  }

  /* Now count the number of symbols of each code length */
  for (i = 0; i <= 256; i++) {
    if (codesize[i]) {
      /* The JPEG standard seems to think that this can't happen, */
      /* but I'm paranoid... */
      if (codesize[i] > MAX__CLEN)
	ERREXIT(cinfo, JERR__HUFF__CLEN__OVERFLOW);

      bits[codesize[i]]++;
    }
  }

  /* JPEG doesn't allow symbols with code lengths over 16 bits, so if the pure
   * Huffman procedure assigned any such lengths, we must adjust the coding.
   * Here is what the JPEG spec says about how this next bit works:
   * Since symbols are paired for the longest Huffman code, the symbols are
   * removed from this length category two at a time.  The prefix for the pair
   * (which is one bit shorter) is allocated to one of the pair; then,
   * skipping the BITS entry for that prefix length, a code word from the next
   * shortest nonzero BITS entry is converted into a prefix for two code words
   * one bit longer.
   */
  
  for (i = MAX__CLEN; i > 16; i--) {
    while (bits[i] > 0) {
      j = i - 2;		/* find length of new prefix to be used */
      while (bits[j] == 0)
	j--;
      
      bits[i] -= 2;		/* remove two symbols */
      bits[i-1]++;		/* one goes in this length */
      bits[j+1] += 2;		/* two new symbols in this length */
      bits[j]--;		/* symbol of this length is now a prefix */
    }
  }

  /* Remove the count for the pseudo-symbol 256 from the largest codelength */
  while (bits[i] == 0)		/* find largest codelength still in use */
    i--;
  bits[i]--;
  
  /* Return final symbol counts (only for lengths 0..16) */
  MEMCOPY(htbl->bits, bits, SIZEOF(htbl->bits));
  
  /* Return a list of the symbols sorted by code length */
  /* It's not real clear to me why we don't need to consider the codelength
   * changes made above, but the JPEG spec seems to think this works.
   */
  p = 0;
  for (i = 1; i <= MAX__CLEN; i++) {
    for (j = 0; j <= 255; j++) {
      if (codesize[j] == i) {
	htbl->huffval[p] = (UINT8) j;
	p++;
      }
    }
  }

  /* Set sent__table FALSE so updated table will be written to JPEG file. */
  htbl->sent__table = FALSE;
}


/*
 * Finish up a statistics-gathering pass and create the new Huffman tables.
 */

METHODDEF(void)
finish__pass__gather (j__compress__ptr cinfo)
{
  huff__entropy__ptr entropy = (huff__entropy__ptr) cinfo->entropy;
  int ci, dctbl, actbl;
  jpeg__component__info * compptr;
  JHUFF__TBL **htblptr;
  CH_BOOL did__dc[NUM__HUFF__TBLS];
  CH_BOOL did__ac[NUM__HUFF__TBLS];

  /* It's important not to apply jpeg__gen__optimal__table more than once
   * per table, because it clobbers the input frequency counts!
   */
  MEMZERO(did__dc, SIZEOF(did__dc));
  MEMZERO(did__ac, SIZEOF(did__ac));

  for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
    compptr = cinfo->cur__comp__info[ci];
    dctbl = compptr->dc__tbl__no;
    actbl = compptr->ac__tbl__no;
    if (! did__dc[dctbl]) {
      htblptr = & cinfo->dc__huff__tbl__ptrs[dctbl];
      if (*htblptr == NULL)
	*htblptr = jpeg__alloc__huff__table((j__common__ptr) cinfo);
      jpeg__gen__optimal__table(cinfo, *htblptr, entropy->dc__count__ptrs[dctbl]);
      did__dc[dctbl] = TRUE;
    }
    if (! did__ac[actbl]) {
      htblptr = & cinfo->ac__huff__tbl__ptrs[actbl];
      if (*htblptr == NULL)
	*htblptr = jpeg__alloc__huff__table((j__common__ptr) cinfo);
      jpeg__gen__optimal__table(cinfo, *htblptr, entropy->ac__count__ptrs[actbl]);
      did__ac[actbl] = TRUE;
    }
  }
}


#endif /* ENTROPY__OPT__SUPPORTED */


/*
 * Module initialization routine for Huffman entropy encoding.
 */

GLOBAL(void)
jinit__huff__encoder (j__compress__ptr cinfo)
{
  huff__entropy__ptr entropy;
  int i;

  entropy = (huff__entropy__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(huff__entropy__encoder));
  cinfo->entropy = (struct jpeg__entropy__encoder *) entropy;
  entropy->pub.start__pass = start__pass__huff;

  /* Mark tables unallocated */
  for (i = 0; i < NUM__HUFF__TBLS; i++) {
    entropy->dc__derived__tbls[i] = entropy->ac__derived__tbls[i] = NULL;
#ifdef ENTROPY__OPT__SUPPORTED
    entropy->dc__count__ptrs[i] = entropy->ac__count__ptrs[i] = NULL;
#endif
  }
}
