/*
 * jcphuff.c
 *
 * Copyright (C) 1995-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy encoding routines for progressive JPEG.
 *
 * We do not support output suspension in this module, since the library
 * currently does not allow multiple-scan files to be written with output
 * suspension.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jchuff.h"		/* Declarations shared with jchuff.c */

#ifdef C__PROGRESSIVE__SUPPORTED

/* Expanded entropy encoder object for progressive Huffman encoding. */

typedef struct {
  struct jpeg__entropy__encoder pub; /* public fields */

  /* Mode flag: TRUE for optimization, FALSE for actual data output */
  CH_BOOL gather__statistics;

  /* Bit-level coding status.
   * next__output__byte/free__in__buffer are local copies of cinfo->dest fields.
   */
  JOCTET * next__output__byte;	/* => next byte to write in buffer */
  size_t free__in__buffer;	/* # of byte spaces remaining in buffer */
  INT32 put__buffer;		/* current bit-accumulation buffer */
  int put__bits;			/* # of bits now in it */
  j__compress__ptr cinfo;		/* link to cinfo (needed for dump__buffer) */

  /* Coding status for DC components */
  int last__dc__val[MAX__COMPS__IN__SCAN]; /* last DC coef for each component */

  /* Coding status for AC components */
  int ac__tbl__no;		/* the table number of the single component */
  unsigned int EOBRUN;		/* run length of EOBs */
  unsigned int BE;		/* # of buffered correction bits before MCU */
  char * bit__buffer;		/* buffer for correction bits (1 per char) */
  /* packing correction bits tightly would save some space but cost time... */

  unsigned int restarts__to__go;	/* MCUs left in this restart interval */
  int next__restart__num;		/* next restart number to write (0-7) */

  /* Pointers to derived tables (these workspaces have image lifespan).
   * Since any one scan codes only DC or only AC, we only need one set
   * of tables, not one for DC and one for AC.
   */
  c__derived__tbl * derived__tbls[NUM__HUFF__TBLS];

  /* Statistics tables for optimization; again, one set is enough */
  long * count__ptrs[NUM__HUFF__TBLS];
} phuff__entropy__encoder;

typedef phuff__entropy__encoder * phuff__entropy__ptr;

/* MAX__CORR__BITS is the number of bits the AC refinement correction-bit
 * buffer can hold.  Larger sizes may slightly improve compression, but
 * 1000 is already well into the realm of overkill.
 * The minimum safe size is 64 bits.
 */

#define MAX__CORR__BITS  1000	/* Max # of correction bits I can buffer */

/* IRIGHT__SHIFT is like RIGHT__SHIFT, but works on int rather than INT32.
 * We assume that int right shift is unsigned if INT32 right shift is,
 * which should be safe.
 */

#ifdef RIGHT__SHIFT__IS__UNSIGNED
#define ISHIFT__TEMPS	int ishift__temp;
#define IRIGHT__SHIFT(x,shft)  \
	((ishift__temp = (x)) < 0 ? \
	 (ishift__temp >> (shft)) | ((~0) << (16-(shft))) : \
	 (ishift__temp >> (shft)))
#else
#define ISHIFT__TEMPS
/*#define IRIGHT__SHIFT(x,shft)	((x) >> (shft))*/
#define IRIGHT__SHIFT(x,shft) zjpg__getshft((x), (shft))
#endif

/* Forward declarations */
METHODDEF(CH_BOOL) encode__mcu__DC__first JPP((j__compress__ptr cinfo,
					    JBLOCKROW *MCU__data));
METHODDEF(CH_BOOL) encode__mcu__AC__first JPP((j__compress__ptr cinfo,
					    JBLOCKROW *MCU__data));
METHODDEF(CH_BOOL) encode__mcu__DC__refine JPP((j__compress__ptr cinfo,
					     JBLOCKROW *MCU__data));
METHODDEF(CH_BOOL) encode__mcu__AC__refine JPP((j__compress__ptr cinfo,
					     JBLOCKROW *MCU__data));
METHODDEF(void) finish__pass__phuff JPP((j__compress__ptr cinfo));
METHODDEF(void) finish__pass__gather__phuff JPP((j__compress__ptr cinfo));


/*
 * Initialize for a Huffman-compressed scan using progressive JPEG.
 */

METHODDEF(void)
start__pass__phuff (j__compress__ptr cinfo, CH_BOOL gather__statistics)
{  
  phuff__entropy__ptr entropy = (phuff__entropy__ptr) cinfo->entropy;
  CH_BOOL is__DC__band;
  int ci, tbl;
  jpeg__component__info * compptr;

  entropy->cinfo = cinfo;
  entropy->gather__statistics = gather__statistics;

  is__DC__band = (cinfo->Ss == 0);

  /* We assume jcmaster.c already validated the scan parameters. */

  /* Select execution routines */
  if (cinfo->Ah == 0) {
    if (is__DC__band)
      entropy->pub.encode__mcu = encode__mcu__DC__first;
    else
      entropy->pub.encode__mcu = encode__mcu__AC__first;
  } else {
    if (is__DC__band)
      entropy->pub.encode__mcu = encode__mcu__DC__refine;
    else {
      entropy->pub.encode__mcu = encode__mcu__AC__refine;
      /* AC refinement needs a correction bit buffer */
      if (entropy->bit__buffer == NULL)
	entropy->bit__buffer = (char *)
	  (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				      MAX__CORR__BITS * SIZEOF(char));
    }
  }
  if (gather__statistics)
    entropy->pub.finish__pass = finish__pass__gather__phuff;
  else
    entropy->pub.finish__pass = finish__pass__phuff;

  /* Only DC coefficients may be interleaved, so cinfo->comps__in__scan = 1
   * for AC coefficients.
   */
  for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
    compptr = cinfo->cur__comp__info[ci];
    /* Initialize DC predictions to 0 */
    entropy->last__dc__val[ci] = 0;
    /* Get table index */
    if (is__DC__band) {
      if (cinfo->Ah != 0)	/* DC refinement needs no table */
	continue;
      tbl = compptr->dc__tbl__no;
    } else {
      entropy->ac__tbl__no = tbl = compptr->ac__tbl__no;
    }
    if (gather__statistics) {
      /* Check for invalid table index */
      /* (make__c__derived__tbl does this in the other path) */
      if (tbl < 0 || tbl >= NUM__HUFF__TBLS)
        ERREXIT1(cinfo, JERR__NO__HUFF__TABLE, tbl);
      /* Allocate and zero the statistics tables */
      /* Note that jpeg__gen__optimal__table expects 257 entries in each table! */
      if (entropy->count__ptrs[tbl] == NULL)
	entropy->count__ptrs[tbl] = (long *)
	  (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				      257 * SIZEOF(long));
      MEMZERO(entropy->count__ptrs[tbl], 257 * SIZEOF(long));
    } else {
      /* Compute derived values for Huffman table */
      /* We may do this more than once for a table, but it's not expensive */
      jpeg__make__c__derived__tbl(cinfo, is__DC__band, tbl,
			      & entropy->derived__tbls[tbl]);
    }
  }

  /* Initialize AC stuff */
  entropy->EOBRUN = 0;
  entropy->BE = 0;

  /* Initialize bit buffer to empty */
  entropy->put__buffer = 0;
  entropy->put__bits = 0;

  /* Initialize restart stuff */
  entropy->restarts__to__go = cinfo->restart__interval;
  entropy->next__restart__num = 0;
}


/* Outputting bytes to the file.
 * NB: these must be called only when actually outputting,
 * that is, entropy->gather__statistics == FALSE.
 */

/* Emit a byte */
#define emit__byte(entropy,val)  \
	{ *(entropy)->next__output__byte++ = (JOCTET) (val);  \
	  if (--(entropy)->free__in__buffer == 0)  \
	    dump__buffer(entropy); }


LOCAL(void)
dump__buffer (phuff__entropy__ptr entropy)
/* Empty the output buffer; we do not support suspension in this module. */
{
  struct jpeg__destination__mgr * dest = entropy->cinfo->dest;

  if (! (*dest->empty__output__buffer) (entropy->cinfo))
    ERREXIT(entropy->cinfo, JERR__CANT__SUSPEND);
  /* After a successful buffer dump, must reset buffer pointers */
  entropy->next__output__byte = dest->next__output__byte;
  entropy->free__in__buffer = dest->free__in__buffer;
}


/* Outputting bits to the file */

/* Only the right 24 bits of put__buffer are used; the valid bits are
 * left-justified in this part.  At most 16 bits can be passed to emit__bits
 * in one call, and we never retain more than 7 bits in put__buffer
 * between calls, so 24 bits are sufficient.
 */

INLINE
LOCAL(void)
emit__bits (phuff__entropy__ptr entropy, unsigned int code, int size)
/* Emit some bits, unless we are in gather mode */
{
  /* This routine is heavily used, so it's worth coding tightly. */
  register INT32 put__buffer = (INT32) code;
  register int put__bits = entropy->put__bits;

  /* if size is 0, caller used an invalid Huffman table entry */
  if (size == 0)
    ERREXIT(entropy->cinfo, JERR__HUFF__MISSING__CODE);

  if (entropy->gather__statistics)
    return;			/* do nothing if we're only getting stats */

  put__buffer &= (((INT32) 1)<<size) - 1; /* mask off any extra bits in code */
  
  put__bits += size;		/* new number of bits in buffer */
  
  put__buffer <<= 24 - put__bits; /* align incoming bits */

  put__buffer |= entropy->put__buffer; /* and merge with old buffer contents */

  while (put__bits >= 8) {
    int c = (int) ((put__buffer >> 16) & 0xFF);
    
    emit__byte(entropy, c);
    if (c == 0xFF) {		/* need to stuff a zero byte? */
      emit__byte(entropy, 0);
    }
    put__buffer <<= 8;
    put__bits -= 8;
  }

  entropy->put__buffer = put__buffer; /* update variables */
  entropy->put__bits = put__bits;
}


LOCAL(void)
flush__bits (phuff__entropy__ptr entropy)
{
  emit__bits(entropy, 0x7F, 7); /* fill any partial byte with ones */
  entropy->put__buffer = 0;     /* and reset bit-buffer to empty */
  entropy->put__bits = 0;
}


/*
 * Emit (or just count) a Huffman symbol.
 */

INLINE
LOCAL(void)
emit__symbol (phuff__entropy__ptr entropy, int tbl__no, int symbol)
{
  if (entropy->gather__statistics)
    entropy->count__ptrs[tbl__no][symbol]++;
  else {
    c__derived__tbl * tbl = entropy->derived__tbls[tbl__no];
    emit__bits(entropy, tbl->ehufco[symbol], tbl->ehufsi[symbol]);
  }
}


/*
 * Emit bits from a correction bit buffer.
 */

LOCAL(void)
emit__buffered__bits (phuff__entropy__ptr entropy, char * bufstart,
		    unsigned int nbits)
{
  if (entropy->gather__statistics)
    return;			/* no real work */

  while (nbits > 0) {
    emit__bits(entropy, (unsigned int) (*bufstart), 1);
    bufstart++;
    nbits--;
  }
}


/*
 * Emit any pending EOBRUN symbol.
 */

LOCAL(void)
emit__eobrun (phuff__entropy__ptr entropy)
{
  register int temp, nbits;

  if (entropy->EOBRUN > 0) {	/* if there is any pending EOBRUN */
    temp = entropy->EOBRUN;
    nbits = 0;
    while ((temp >>= 1))
      nbits++;
    /* safety check: shouldn't happen given limited correction-bit buffer */
    if (nbits > 14)
      ERREXIT(entropy->cinfo, JERR__HUFF__MISSING__CODE);

    emit__symbol(entropy, entropy->ac__tbl__no, nbits << 4);
    if (nbits)
      emit__bits(entropy, entropy->EOBRUN, nbits);

    entropy->EOBRUN = 0;

    /* Emit any buffered correction bits */
    emit__buffered__bits(entropy, entropy->bit__buffer, entropy->BE);
    entropy->BE = 0;
  }
}


/*
 * Emit a restart marker & resynchronize predictions.
 */

LOCAL(void)
emit__restart (phuff__entropy__ptr entropy, int restart__num)
{
  int ci;

  emit__eobrun(entropy);

  if (! entropy->gather__statistics) {
    flush__bits(entropy);
    emit__byte(entropy, 0xFF);
    emit__byte(entropy, JPEG__RST0 + restart__num);
  }

  if (entropy->cinfo->Ss == 0) {
    /* Re-initialize DC predictions to 0 */
    for (ci = 0; ci < entropy->cinfo->comps__in__scan; ci++)
      entropy->last__dc__val[ci] = 0;
  } else {
    /* Re-initialize all AC-related fields to 0 */
    entropy->EOBRUN = 0;
    entropy->BE = 0;
  }
}


/*
 * MCU encoding for DC initial scan (either spectral selection,
 * or first pass of successive approximation).
 */

METHODDEF(CH_BOOL)
encode__mcu__DC__first (j__compress__ptr cinfo, JBLOCKROW *MCU__data)
{
  phuff__entropy__ptr entropy = (phuff__entropy__ptr) cinfo->entropy;
  register int temp, temp2;
  register int nbits;
  int blkn, ci;
  int Al = cinfo->Al;
  JBLOCKROW block;
  jpeg__component__info * compptr;
  ISHIFT__TEMPS

  entropy->next__output__byte = cinfo->dest->next__output__byte;
  entropy->free__in__buffer = cinfo->dest->free__in__buffer;

  /* Emit restart marker if needed */
  if (cinfo->restart__interval)
    if (entropy->restarts__to__go == 0)
      emit__restart(entropy, entropy->next__restart__num);

  /* Encode the MCU data blocks */
  for (blkn = 0; blkn < cinfo->blocks__in__MCU; blkn++) {
    block = MCU__data[blkn];
    ci = cinfo->MCU__membership[blkn];
    compptr = cinfo->cur__comp__info[ci];

    /* Compute the DC value after the required point transform by Al.
     * This is simply an arithmetic right shift.
     */
    temp2 = IRIGHT__SHIFT((int) ((*block)[0]), Al);

    /* DC differences are figured on the point-transformed values. */
    temp = temp2 - entropy->last__dc__val[ci];
    entropy->last__dc__val[ci] = temp2;

    /* Encode the DC coefficient difference per section G.1.2.1 */
    temp2 = temp;
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
      ERREXIT(cinfo, JERR__BAD__DCT__COEF);
    
    /* Count/emit the Huffman-coded symbol for the number of bits */
    emit__symbol(entropy, compptr->dc__tbl__no, nbits);
    
    /* Emit that number of bits of the value, if positive, */
    /* or the complement of its magnitude, if negative. */
    if (nbits)			/* emit__bits rejects calls with size 0 */
      emit__bits(entropy, (unsigned int) temp2, nbits);
  }

  cinfo->dest->next__output__byte = entropy->next__output__byte;
  cinfo->dest->free__in__buffer = entropy->free__in__buffer;

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
 * MCU encoding for AC initial scan (either spectral selection,
 * or first pass of successive approximation).
 */

METHODDEF(CH_BOOL)
encode__mcu__AC__first (j__compress__ptr cinfo, JBLOCKROW *MCU__data)
{
  phuff__entropy__ptr entropy = (phuff__entropy__ptr) cinfo->entropy;
  register int temp, temp2;
  register int nbits;
  register int r, k;
  int Se = cinfo->Se;
  int Al = cinfo->Al;
  JBLOCKROW block;

  entropy->next__output__byte = cinfo->dest->next__output__byte;
  entropy->free__in__buffer = cinfo->dest->free__in__buffer;

  /* Emit restart marker if needed */
  if (cinfo->restart__interval)
    if (entropy->restarts__to__go == 0)
      emit__restart(entropy, entropy->next__restart__num);

  /* Encode the MCU data block */
  block = MCU__data[0];

  /* Encode the AC coefficients per section G.1.2.2, fig. G.3 */
  
  r = 0;			/* r = run length of zeros */
   
  for (k = cinfo->Ss; k <= Se; k++) {
    if ((temp = (*block)[jpeg__natural__order[k]]) == 0) {
      r++;
      continue;
    }
    /* We must apply the point transform by Al.  For AC coefficients this
     * is an integer division with rounding towards 0.  To do this portably
     * in C, we shift after obtaining the absolute value; so the code is
     * interwoven with finding the abs value (temp) and output bits (temp2).
     */
    if (temp < 0) {
      temp = -temp;		/* temp is abs value of input */
      temp >>= Al;		/* apply the point transform */
      /* For a negative coef, want temp2 = bitwise complement of abs(coef) */
      temp2 = ~temp;
    } else {
      temp >>= Al;		/* apply the point transform */
      temp2 = temp;
    }
    /* Watch out for case that nonzero coef is zero after point transform */
    if (temp == 0) {
      r++;
      continue;
    }

    /* Emit any pending EOBRUN */
    if (entropy->EOBRUN > 0)
      emit__eobrun(entropy);
    /* if run length > 15, must emit special run-length-16 codes (0xF0) */
    while (r > 15) {
      emit__symbol(entropy, entropy->ac__tbl__no, 0xF0);
      r -= 16;
    }

    /* Find the number of bits needed for the magnitude of the coefficient */
    nbits = 1;			/* there must be at least one 1 bit */
    while ((temp >>= 1))
      nbits++;
    /* Check for out-of-range coefficient values */
    if (nbits > MAX__COEF__BITS)
      ERREXIT(cinfo, JERR__BAD__DCT__COEF);

    /* Count/emit Huffman symbol for run length / number of bits */
    emit__symbol(entropy, entropy->ac__tbl__no, (r << 4) + nbits);

    /* Emit that number of bits of the value, if positive, */
    /* or the complement of its magnitude, if negative. */
    emit__bits(entropy, (unsigned int) temp2, nbits);

    r = 0;			/* reset zero run length */
  }

  if (r > 0) {			/* If there are trailing zeroes, */
    entropy->EOBRUN++;		/* count an EOB */
    if (entropy->EOBRUN == 0x7FFF)
      emit__eobrun(entropy);	/* force it out to avoid overflow */
  }

  cinfo->dest->next__output__byte = entropy->next__output__byte;
  cinfo->dest->free__in__buffer = entropy->free__in__buffer;

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
 * MCU encoding for DC successive approximation refinement scan.
 * Note: we assume such scans can be multi-component, although the spec
 * is not very clear on the point.
 */

METHODDEF(CH_BOOL)
encode__mcu__DC__refine (j__compress__ptr cinfo, JBLOCKROW *MCU__data)
{
  phuff__entropy__ptr entropy = (phuff__entropy__ptr) cinfo->entropy;
  register int temp;
  int blkn;
  int Al = cinfo->Al;
  JBLOCKROW block;

  entropy->next__output__byte = cinfo->dest->next__output__byte;
  entropy->free__in__buffer = cinfo->dest->free__in__buffer;

  /* Emit restart marker if needed */
  if (cinfo->restart__interval)
    if (entropy->restarts__to__go == 0)
      emit__restart(entropy, entropy->next__restart__num);

  /* Encode the MCU data blocks */
  for (blkn = 0; blkn < cinfo->blocks__in__MCU; blkn++) {
    block = MCU__data[blkn];

    /* We simply emit the Al'th bit of the DC coefficient value. */
    temp = (*block)[0];
    emit__bits(entropy, (unsigned int) (temp >> Al), 1);
  }

  cinfo->dest->next__output__byte = entropy->next__output__byte;
  cinfo->dest->free__in__buffer = entropy->free__in__buffer;

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
 * MCU encoding for AC successive approximation refinement scan.
 */

METHODDEF(CH_BOOL)
encode__mcu__AC__refine (j__compress__ptr cinfo, JBLOCKROW *MCU__data)
{
  phuff__entropy__ptr entropy = (phuff__entropy__ptr) cinfo->entropy;
  register int temp;
  register int r, k;
  int EOB;
  char *BR__buffer;
  unsigned int BR;
  int Se = cinfo->Se;
  int Al = cinfo->Al;
  JBLOCKROW block;
  int absvalues[DCTSIZE2];

  entropy->next__output__byte = cinfo->dest->next__output__byte;
  entropy->free__in__buffer = cinfo->dest->free__in__buffer;

  /* Emit restart marker if needed */
  if (cinfo->restart__interval)
    if (entropy->restarts__to__go == 0)
      emit__restart(entropy, entropy->next__restart__num);

  /* Encode the MCU data block */
  block = MCU__data[0];

  /* It is convenient to make a pre-pass to determine the transformed
   * coefficients' absolute values and the EOB position.
   */
  EOB = 0;
  for (k = cinfo->Ss; k <= Se; k++) {
    temp = (*block)[jpeg__natural__order[k]];
    /* We must apply the point transform by Al.  For AC coefficients this
     * is an integer division with rounding towards 0.  To do this portably
     * in C, we shift after obtaining the absolute value.
     */
    if (temp < 0)
      temp = -temp;		/* temp is abs value of input */
    temp >>= Al;		/* apply the point transform */
    absvalues[k] = temp;	/* save abs value for main pass */
    if (temp == 1)
      EOB = k;			/* EOB = index of last newly-nonzero coef */
  }

  /* Encode the AC coefficients per section G.1.2.3, fig. G.7 */
  
  r = 0;			/* r = run length of zeros */
  BR = 0;			/* BR = count of buffered bits added now */
  BR__buffer = entropy->bit__buffer + entropy->BE; /* Append bits to buffer */

  for (k = cinfo->Ss; k <= Se; k++) {
    if ((temp = absvalues[k]) == 0) {
      r++;
      continue;
    }

    /* Emit any required ZRLs, but not if they can be folded into EOB */
    while (r > 15 && k <= EOB) {
      /* emit any pending EOBRUN and the BE correction bits */
      emit__eobrun(entropy);
      /* Emit ZRL */
      emit__symbol(entropy, entropy->ac__tbl__no, 0xF0);
      r -= 16;
      /* Emit buffered correction bits that must be associated with ZRL */
      emit__buffered__bits(entropy, BR__buffer, BR);
      BR__buffer = entropy->bit__buffer; /* BE bits are gone now */
      BR = 0;
    }

    /* If the coef was previously nonzero, it only needs a correction bit.
     * NOTE: a straight translation of the spec's figure G.7 would suggest
     * that we also need to test r > 15.  But if r > 15, we can only get here
     * if k > EOB, which implies that this coefficient is not 1.
     */
    if (temp > 1) {
      /* The correction bit is the next bit of the absolute value. */
      BR__buffer[BR++] = (char) (temp & 1);
      continue;
    }

    /* Emit any pending EOBRUN and the BE correction bits */
    emit__eobrun(entropy);

    /* Count/emit Huffman symbol for run length / number of bits */
    emit__symbol(entropy, entropy->ac__tbl__no, (r << 4) + 1);

    /* Emit output bit for newly-nonzero coef */
    temp = ((*block)[jpeg__natural__order[k]] < 0) ? 0 : 1;
    emit__bits(entropy, (unsigned int) temp, 1);

    /* Emit buffered correction bits that must be associated with this code */
    emit__buffered__bits(entropy, BR__buffer, BR);
    BR__buffer = entropy->bit__buffer; /* BE bits are gone now */
    BR = 0;
    r = 0;			/* reset zero run length */
  }

  if (r > 0 || BR > 0) {	/* If there are trailing zeroes, */
    entropy->EOBRUN++;		/* count an EOB */
    entropy->BE += BR;		/* concat my correction bits to older ones */
    /* We force out the EOB if we risk either:
     * 1. overflow of the EOB counter;
     * 2. overflow of the correction bit buffer during the next MCU.
     */
    if (entropy->EOBRUN == 0x7FFF || entropy->BE > (MAX__CORR__BITS-DCTSIZE2+1))
      emit__eobrun(entropy);
  }

  cinfo->dest->next__output__byte = entropy->next__output__byte;
  cinfo->dest->free__in__buffer = entropy->free__in__buffer;

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
 * Finish up at the end of a Huffman-compressed progressive scan.
 */

METHODDEF(void)
finish__pass__phuff (j__compress__ptr cinfo)
{   
  phuff__entropy__ptr entropy = (phuff__entropy__ptr) cinfo->entropy;

  entropy->next__output__byte = cinfo->dest->next__output__byte;
  entropy->free__in__buffer = cinfo->dest->free__in__buffer;

  /* Flush out any buffered data */
  emit__eobrun(entropy);
  flush__bits(entropy);

  cinfo->dest->next__output__byte = entropy->next__output__byte;
  cinfo->dest->free__in__buffer = entropy->free__in__buffer;
}


/*
 * Finish up a statistics-gathering pass and create the new Huffman tables.
 */

METHODDEF(void)
finish__pass__gather__phuff (j__compress__ptr cinfo)
{
  phuff__entropy__ptr entropy = (phuff__entropy__ptr) cinfo->entropy;
  CH_BOOL is__DC__band;
  int ci, tbl;
  jpeg__component__info * compptr;
  JHUFF__TBL **htblptr;
  CH_BOOL did[NUM__HUFF__TBLS];

  /* Flush out buffered data (all we care about is counting the EOB symbol) */
  emit__eobrun(entropy);

  is__DC__band = (cinfo->Ss == 0);

  /* It's important not to apply jpeg__gen__optimal__table more than once
   * per table, because it clobbers the input frequency counts!
   */
  MEMZERO(did, SIZEOF(did));

  for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
    compptr = cinfo->cur__comp__info[ci];
    if (is__DC__band) {
      if (cinfo->Ah != 0)	/* DC refinement needs no table */
	continue;
      tbl = compptr->dc__tbl__no;
    } else {
      tbl = compptr->ac__tbl__no;
    }
    if (! did[tbl]) {
      if (is__DC__band)
        htblptr = & cinfo->dc__huff__tbl__ptrs[tbl];
      else
        htblptr = & cinfo->ac__huff__tbl__ptrs[tbl];
      if (*htblptr == NULL)
        *htblptr = jpeg__alloc__huff__table((j__common__ptr) cinfo);
      jpeg__gen__optimal__table(cinfo, *htblptr, entropy->count__ptrs[tbl]);
      did[tbl] = TRUE;
    }
  }
}


/*
 * Module initialization routine for progressive Huffman entropy encoding.
 */

GLOBAL(void)
jinit__phuff__encoder (j__compress__ptr cinfo)
{
  phuff__entropy__ptr entropy;
  int i;

  entropy = (phuff__entropy__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(phuff__entropy__encoder));
  cinfo->entropy = (struct jpeg__entropy__encoder *) entropy;
  entropy->pub.start__pass = start__pass__phuff;

  /* Mark tables unallocated */
  for (i = 0; i < NUM__HUFF__TBLS; i++) {
    entropy->derived__tbls[i] = NULL;
    entropy->count__ptrs[i] = NULL;
  }
  entropy->bit__buffer = NULL;	/* needed only in AC refinement scan */
}

#endif /* C__PROGRESSIVE__SUPPORTED */
