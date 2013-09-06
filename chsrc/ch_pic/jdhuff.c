/*
 * jdhuff.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy decoding routines.
 *
 * Much of the complexity here has to do with supporting input suspension.
 * If the data source module demands suspension, we want to be able to back
 * up to the start of the current MCU.  To do this, we copy state variables
 * into local working storage, and update them back to the permanent
 * storage only upon successful completion of an MCU.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdhuff.h"		/* Declarations shared with jdphuff.c */


/*
 * Expanded entropy decoder object for Huffman decoding.
 *
 * The savable__state subrecord contains fields that change within an MCU,
 * but must not be updated permanently until we complete the MCU.
 */

typedef struct {
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
	((dest).last__dc__val[0] = (src).last__dc__val[0], \
	 (dest).last__dc__val[1] = (src).last__dc__val[1], \
	 (dest).last__dc__val[2] = (src).last__dc__val[2], \
	 (dest).last__dc__val[3] = (src).last__dc__val[3])
#endif
#endif


typedef struct {
  struct jpeg__entropy__decoder pub; /* public fields */

  /* These fields are loaded into local variables at start of each MCU.
   * In case of suspension, we exit WITHOUT updating them.
   */
  bitread__perm__state bitstate;	/* Bit buffer at start of MCU */
  savable__state saved;		/* Other state at start of MCU */

  /* These fields are NOT loaded into local working state. */
  unsigned int restarts__to__go;	/* MCUs left in this restart interval */

  /* Pointers to derived tables (these workspaces have image lifespan) */
  d__derived__tbl * dc__derived__tbls[NUM__HUFF__TBLS];
  d__derived__tbl * ac__derived__tbls[NUM__HUFF__TBLS];

  /* Precalculated info set up by start__pass for use in decode__mcu: */

  /* Pointers to derived tables to be used for each block within an MCU */
  d__derived__tbl * dc__cur__tbls[D__MAX__BLOCKS__IN__MCU];
  d__derived__tbl * ac__cur__tbls[D__MAX__BLOCKS__IN__MCU];
  /* Whether we care about the DC and AC coefficient values for each block */
  CH_BOOL dc__needed[D__MAX__BLOCKS__IN__MCU];
  CH_BOOL ac__needed[D__MAX__BLOCKS__IN__MCU];
} huff__entropy__decoder;

typedef huff__entropy__decoder * huff__entropy__ptr;


/*
 * Initialize for a Huffman-compressed scan.
 */

METHODDEF(void)
start__pass__huff__decoder (j__decompress__ptr cinfo)
{
  huff__entropy__ptr entropy = (huff__entropy__ptr) cinfo->entropy;
  int ci, blkn, dctbl, actbl;
  jpeg__component__info * compptr;

  /* Check that the scan parameters Ss, Se, Ah/Al are OK for sequential JPEG.
   * This ought to be an error condition, but we make it a warning because
   * there are some baseline files out there with all zeroes in these bytes.
   */
  if (cinfo->Ss != 0 || cinfo->Se != DCTSIZE2-1 ||
      cinfo->Ah != 0 || cinfo->Al != 0)
    WARNMS(cinfo, JWRN__NOT__SEQUENTIAL);

  for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
    compptr = cinfo->cur__comp__info[ci];
    dctbl = compptr->dc__tbl__no;
    actbl = compptr->ac__tbl__no;
    /* Compute derived values for Huffman tables */
    /* We may do this more than once for a table, but it's not expensive */
    jpeg__make__d__derived__tbl(cinfo, TRUE, dctbl,
			    & entropy->dc__derived__tbls[dctbl]);
    jpeg__make__d__derived__tbl(cinfo, FALSE, actbl,
			    & entropy->ac__derived__tbls[actbl]);
    /* Initialize DC predictions to 0 */
    entropy->saved.last__dc__val[ci] = 0;
  }

  /* Precalculate decoding info for each block in an MCU of this scan */
  for (blkn = 0; blkn < cinfo->blocks__in__MCU; blkn++) {
    ci = cinfo->MCU__membership[blkn];
    compptr = cinfo->cur__comp__info[ci];
    /* Precalculate which table to use for each block */
    entropy->dc__cur__tbls[blkn] = entropy->dc__derived__tbls[compptr->dc__tbl__no];
    entropy->ac__cur__tbls[blkn] = entropy->ac__derived__tbls[compptr->ac__tbl__no];
    /* Decide whether we really care about the coefficient values */
    if (compptr->component__needed) {
      entropy->dc__needed[blkn] = TRUE;
      /* we don't need the ACs if producing a 1/8th-size image */
      entropy->ac__needed[blkn] = (compptr->DCT__scaled__size > 1);
    } else {
      entropy->dc__needed[blkn] = entropy->ac__needed[blkn] = FALSE;
    }
  }

  /* Initialize bitread state variables */
  entropy->bitstate.bits__left = 0;
  entropy->bitstate.get__buffer = 0; /* unnecessary, but keeps Purify quiet */
  entropy->pub.insufficient__data = FALSE;

  /* Initialize restart counter */
  entropy->restarts__to__go = cinfo->restart__interval;
}


/*
 * Compute the derived values for a Huffman table.
 * This routine also performs some validation checks on the table.
 *
 * Note this is also used by jdphuff.c.
 */

GLOBAL(void)
jpeg__make__d__derived__tbl (j__decompress__ptr cinfo, CH_BOOL isDC, int tblno,
			 d__derived__tbl ** pdtbl)
{
  JHUFF__TBL *htbl;
  d__derived__tbl *dtbl;
  int p, i, l, si, numsymbols;
  int lookbits, ctr;
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
    *pdtbl = (d__derived__tbl *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  SIZEOF(d__derived__tbl));
  dtbl = *pdtbl;
  dtbl->pub = htbl;		/* fill in back link */
  
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
  numsymbols = p;
  
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

  /* Figure F.15: generate decoding tables for bit-sequential decoding */

  p = 0;
  for (l = 1; l <= 16; l++) {
    if (htbl->bits[l]) {
      /* valoffset[l] = huffval[] index of 1st symbol of code length l,
       * minus the minimum code of length l
       */
      dtbl->valoffset[l] = (INT32) p - (INT32) huffcode[p];
      p += htbl->bits[l];
      dtbl->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
    } else {
      dtbl->maxcode[l] = -1;	/* -1 if no codes of this length */
    }
  }
  dtbl->maxcode[17] = 0xFFFFFL; /* ensures jpeg__huff__decode terminates */

  /* Compute lookahead tables to speed up decoding.
   * First we set all the table entries to 0, indicating "too long";
   * then we iterate through the Huffman codes that are short enough and
   * fill in all the entries that correspond to bit sequences starting
   * with that code.
   */

  MEMZERO(dtbl->look__nbits, SIZEOF(dtbl->look__nbits));

  p = 0;
  for (l = 1; l <= HUFF__LOOKAHEAD; l++) {
    for (i = 1; i <= (int) htbl->bits[l]; i++, p++) {
      /* l = current code's length, p = its index in huffcode[] & huffval[]. */
      /* Generate left-justified code followed by all possible bit sequences */
      lookbits = huffcode[p] << (HUFF__LOOKAHEAD-l);
      for (ctr = 1 << (HUFF__LOOKAHEAD-l); ctr > 0; ctr--) {
	dtbl->look__nbits[lookbits] = l;
	dtbl->look__sym[lookbits] = htbl->huffval[p];
	lookbits++;
      }
    }
  }

  /* Validate symbols as being reasonable.
   * For AC tables, we make no check, but accept all byte values 0..255.
   * For DC tables, we require the symbols to be in range 0..15.
   * (Tighter bounds could be applied depending on the data depth and mode,
   * but this is sufficient to ensure safe decoding.)
   */
  if (isDC) {
    for (i = 0; i < numsymbols; i++) {
      int sym = htbl->huffval[i];
      if (sym < 0 || sym > 15)
	ERREXIT(cinfo, JERR__BAD__HUFF__TABLE);
    }
  }
}


/*
 * Out-of-line code for bit fetching (shared with jdphuff.c).
 * See jdhuff.h for info about usage.
 * Note: current values of get__buffer and bits__left are passed as parameters,
 * but are returned in the corresponding fields of the state struct.
 *
 * On most machines MIN__GET__BITS should be 25 to allow the full 32-bit width
 * of get__buffer to be used.  (On machines with wider words, an even larger
 * buffer could be used.)  However, on some machines 32-bit shifts are
 * quite slow and take time proportional to the number of places shifted.
 * (This is true with most PC compilers, for instance.)  In this case it may
 * be a win to set MIN__GET__BITS to the minimum value of 15.  This reduces the
 * average shift distance at the cost of more calls to jpeg__fill__bit__buffer.
 */

#ifdef SLOW__SHIFT__32
#define MIN__GET__BITS  15	/* minimum allowable value */
#else
#define MIN__GET__BITS  (BIT__BUF__SIZE-7)
#endif


GLOBAL(CH_BOOL)
jpeg__fill__bit__buffer (bitread__working__state * state,
		      register bit__buf__type get__buffer, register int bits__left,
		      int nbits)
/* Load up the bit buffer to a depth of at least nbits */
{
  /* Copy heavily used state fields into locals (hopefully registers) */
  register const JOCTET * next__input__byte = state->next__input__byte;
  register size_t bytes__in__buffer = state->bytes__in__buffer;
  j__decompress__ptr cinfo = state->cinfo;

  /* Attempt to load at least MIN__GET__BITS bits into get__buffer. */
  /* (It is assumed that no request will be for more than that many bits.) */
  /* We fail to do so only if we hit a marker or are forced to suspend. */

  if (cinfo->unread__marker == 0) {	/* cannot advance past a marker */
    while (bits__left < MIN__GET__BITS) {
      register int c;

      /* Attempt to read a byte */
      if (bytes__in__buffer == 0) {
	if (! (*cinfo->src->fill__input__buffer) (cinfo))
	  return FALSE;
	next__input__byte = cinfo->src->next__input__byte;
	bytes__in__buffer = cinfo->src->bytes__in__buffer;
      }
      bytes__in__buffer--;
      c = GETJOCTET(*next__input__byte++);

      /* If it's 0xFF, check and discard stuffed zero byte */
      if (c == 0xFF) {
	/* Loop here to discard any padding FF's on terminating marker,
	 * so that we can save a valid unread__marker value.  NOTE: we will
	 * accept multiple FF's followed by a 0 as meaning a single FF data
	 * byte.  This data pattern is not valid according to the standard.
	 */
	do {
	  if (bytes__in__buffer == 0) {
	    if (! (*cinfo->src->fill__input__buffer) (cinfo))
	      return FALSE;
	    next__input__byte = cinfo->src->next__input__byte;
	    bytes__in__buffer = cinfo->src->bytes__in__buffer;
	  }
	  bytes__in__buffer--;
	  c = GETJOCTET(*next__input__byte++);
	} while (c == 0xFF);

	if (c == 0) {
	  /* Found FF/00, which represents an FF data byte */
	  c = 0xFF;
	} else {
	  /* Oops, it's actually a marker indicating end of compressed data.
	   * Save the marker code for later use.
	   * Fine point: it might appear that we should save the marker into
	   * bitread working state, not straight into permanent state.  But
	   * once we have hit a marker, we cannot need to suspend within the
	   * current MCU, because we will read no more bytes from the data
	   * source.  So it is OK to update permanent state right away.
	   */
	  cinfo->unread__marker = c;
	  /* See if we need to insert some fake zero bits. */
	  goto no__more__bytes;
	}
      }

      /* OK, load c into get__buffer */
      get__buffer = (get__buffer << 8) | c;
      bits__left += 8;
    } /* end while */
  } else {
  no__more__bytes:
    /* We get here if we've read the marker that terminates the compressed
     * data segment.  There should be enough bits in the buffer register
     * to satisfy the request; if so, no problem.
     */
    if (nbits > bits__left) {
      /* Uh-oh.  Report corrupted data to user and stuff zeroes into
       * the data stream, so that we can produce some kind of image.
       * We use a nonvolatile flag to ensure that only one warning message
       * appears per data segment.
       */
      if (! cinfo->entropy->insufficient__data) {
	WARNMS(cinfo, JWRN__HIT__MARKER);
	cinfo->entropy->insufficient__data = TRUE;
      }
      /* Fill the buffer with zero bits */
      get__buffer <<= MIN__GET__BITS - bits__left;
      bits__left = MIN__GET__BITS;
    }
  }

  /* Unload the local registers */
  state->next__input__byte = next__input__byte;
  state->bytes__in__buffer = bytes__in__buffer;
  state->get__buffer = get__buffer;
  state->bits__left = bits__left;

  return TRUE;
}


/*
 * Out-of-line code for Huffman code decoding.
 * See jdhuff.h for info about usage.
 */

GLOBAL(int)
jpeg__huff__decode (bitread__working__state * state,
		  register bit__buf__type get__buffer, register int bits__left,
		  d__derived__tbl * htbl, int min__bits)
{
  register int l = min__bits;
  register INT32 code;

  /* HUFF__DECODE has determined that the code is at least min__bits */
  /* bits long, so fetch that many bits in one swoop. */

  CHECK__BIT__BUFFER(*state, l, return -1);
  code = GET__BITS(l);

  /* Collect the rest of the Huffman code one bit at a time. */
  /* This is per Figure F.16 in the JPEG spec. */

  while (code > htbl->maxcode[l]) {
    code <<= 1;
    CHECK__BIT__BUFFER(*state, 1, return -1);
    code |= GET__BITS(1);
    l++;
  }

  /* Unload the local registers */
  state->get__buffer = get__buffer;
  state->bits__left = bits__left;

  /* With garbage input we may reach the sentinel value l = 17. */

  if (l > 16) {
    WARNMS(state->cinfo, JWRN__HUFF__BAD__CODE);
    return 0;			/* fake a zero as the safest result */
  }

  return htbl->pub->huffval[ (int) (code + htbl->valoffset[l]) ];
}


/*
 * Figure F.12: extend sign bit.
 * On some machines, a shift and add will be faster than a table lookup.
 */

#ifdef AVOID__TABLES

#define HUFF__EXTEND(x,s)  ((x) < (1<<((s)-1)) ? (x) + (((-1)<<(s)) + 1) : (x))

#else

#define HUFF__EXTEND(x,s)  ((x) < extend__test[s] ? (x) + extend__offset[s] : (x))

static const int extend__test[16] =   /* entry n is 2**(n-1) */
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

static const int extend__offset[16] = /* entry n is (-1 << n) + 1 */
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };

#endif /* AVOID__TABLES */


/*
 * Check for a restart marker & resynchronize decoder.
 * Returns FALSE if must suspend.
 */

LOCAL(CH_BOOL)
process__restart (j__decompress__ptr cinfo)
{
  huff__entropy__ptr entropy = (huff__entropy__ptr) cinfo->entropy;
  int ci;

  /* Throw away any unused bits remaining in bit buffer; */
  /* include any full bytes in next__marker's count of discarded bytes */
  cinfo->marker->discarded__bytes += entropy->bitstate.bits__left / 8;
  entropy->bitstate.bits__left = 0;

  /* Advance past the RSTn marker */
  if (! (*cinfo->marker->read__restart__marker) (cinfo))
    return FALSE;

  /* Re-initialize DC predictions to 0 */
  for (ci = 0; ci < cinfo->comps__in__scan; ci++)
    entropy->saved.last__dc__val[ci] = 0;

  /* Reset restart counter */
  entropy->restarts__to__go = cinfo->restart__interval;

  /* Reset out-of-data flag, unless read__restart__marker left us smack up
   * against a marker.  In that case we will end up treating the next data
   * segment as empty, and we can avoid producing bogus output pixels by
   * leaving the flag set.
   */
  if (cinfo->unread__marker == 0)
    entropy->pub.insufficient__data = FALSE;

  return TRUE;
}


/*
 * Decode and return one MCU's worth of Huffman-compressed coefficients.
 * The coefficients are reordered from zigzag order into natural array order,
 * but are not dequantized.
 *
 * The i'th block of the MCU is stored into the block pointed to by
 * MCU__data[i].  WE ASSUME THIS AREA HAS BEEN ZEROED BY THE CALLER.
 * (Wholesale zeroing is usually a little faster than retail...)
 *
 * Returns FALSE if data source requested suspension.  In that case no
 * changes have been made to permanent state.  (Exception: some output
 * coefficients may already have been assigned.  This is harmless for
 * this module, since we'll just re-assign them on the next call.)
 */

METHODDEF(CH_BOOL)
decode__mcu (j__decompress__ptr cinfo, JBLOCKROW *MCU__data)
{
  huff__entropy__ptr entropy = (huff__entropy__ptr) cinfo->entropy;
  int blkn;
  BITREAD__STATE__VARS;
  savable__state state;

  /* Process restart marker if needed; may have to suspend */
  if (cinfo->restart__interval) {
    if (entropy->restarts__to__go == 0)
      if (! process__restart(cinfo))
	return FALSE;
  }

  /* If we've run out of data, just leave the MCU set to zeroes.
   * This way, we return uniform gray for the remainder of the segment.
   */
  if (! entropy->pub.insufficient__data) {

    /* Load up working state */
    BITREAD__LOAD__STATE(cinfo,entropy->bitstate);
    ASSIGN__STATE(state, entropy->saved);

    /* Outer loop handles each block in the MCU */

    for (blkn = 0; blkn < cinfo->blocks__in__MCU; blkn++) {
      JBLOCKROW block = MCU__data[blkn];
      d__derived__tbl * dctbl = entropy->dc__cur__tbls[blkn];
      d__derived__tbl * actbl = entropy->ac__cur__tbls[blkn];
      register int s, k, r;

      /* Decode a single block's worth of coefficients */

      /* Section F.2.2.1: decode the DC coefficient difference */
      HUFF__DECODE(s, br__state, dctbl, return FALSE, label1);
      if (s) {
	CHECK__BIT__BUFFER(br__state, s, return FALSE);
	r = GET__BITS(s);
	s = HUFF__EXTEND(r, s);
      }

      if (entropy->dc__needed[blkn]) {
	/* Convert DC difference to actual value, update last__dc__val */
	int ci = cinfo->MCU__membership[blkn];
	s += state.last__dc__val[ci];
	state.last__dc__val[ci] = s;
	/* Output the DC coefficient (assumes jpeg__natural__order[0] = 0) */
	(*block)[0] = (JCOEF) s;
      }

      if (entropy->ac__needed[blkn]) {

	/* Section F.2.2.2: decode the AC coefficients */
	/* Since zeroes are skipped, output area must be cleared beforehand */
	for (k = 1; k < DCTSIZE2; k++) {
	  HUFF__DECODE(s, br__state, actbl, return FALSE, label2);
      
	  r = s >> 4;
	  s &= 15;
      
	  if (s) {
	    k += r;
	    CHECK__BIT__BUFFER(br__state, s, return FALSE);
	    r = GET__BITS(s);
	    s = HUFF__EXTEND(r, s);
	    /* Output coefficient in natural (dezigzagged) order.
	     * Note: the extra entries in jpeg__natural__order[] will save us
	     * if k >= DCTSIZE2, which could happen if the data is corrupted.
	     */
	    (*block)[jpeg__natural__order[k]] = (JCOEF) s;
	  } else {
	    if (r != 15)
	      break;
	    k += 15;
	  }
	}

      } else {

	/* Section F.2.2.2: decode the AC coefficients */
	/* In this path we just discard the values */
	for (k = 1; k < DCTSIZE2; k++) {
	  HUFF__DECODE(s, br__state, actbl, return FALSE, label3);
      
	  r = s >> 4;
	  s &= 15;
      
	  if (s) {
	    k += r;
	    CHECK__BIT__BUFFER(br__state, s, return FALSE);
	    DROP__BITS(s);
	  } else {
	    if (r != 15)
	      break;
	    k += 15;
	  }
	}

      }
    }

    /* Completed MCU, so update state */
    BITREAD__SAVE__STATE(cinfo,entropy->bitstate);
    ASSIGN__STATE(entropy->saved, state);
  }

  /* Account for restart interval (no-op if not using restarts) */
  entropy->restarts__to__go--;

  return TRUE;
}


/*
 * Module initialization routine for Huffman entropy decoding.
 */

GLOBAL(void)
jinit__huff__decoder (j__decompress__ptr cinfo)
{
  huff__entropy__ptr entropy;
  int i;

  entropy = (huff__entropy__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(huff__entropy__decoder));
  cinfo->entropy = (struct jpeg__entropy__decoder *) entropy;
  entropy->pub.start__pass = start__pass__huff__decoder;
  entropy->pub.decode__mcu = decode__mcu;

  /* Mark tables unallocated */
  for (i = 0; i < NUM__HUFF__TBLS; i++) {
    entropy->dc__derived__tbls[i] = entropy->ac__derived__tbls[i] = NULL;
  }
}
