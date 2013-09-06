/*
 * jchuff.h
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains declarations for Huffman entropy encoding routines
 * that are shared between the sequential encoder (jchuff.c) and the
 * progressive encoder (jcphuff.c).  No other modules need to see these.
 */

/* The legal range of a DCT coefficient is
 *  -1024 .. +1023  for 8-bit data;
 * -16384 .. +16383 for 12-bit data.
 * Hence the magnitude should always fit in 10 or 14 bits respectively.
 */

#if BITS__IN__JSAMPLE == 8
#define MAX__COEF__BITS 10
#else
#define MAX__COEF__BITS 14
#endif

/* Derived data constructed for each Huffman table */

typedef struct {
  unsigned int ehufco[256];	/* code for each symbol */
  char ehufsi[256];		/* length of code for each symbol */
  /* If no code has been allocated for a symbol S, ehufsi[S] contains 0 */
} c__derived__tbl;

/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED__SHORT__EXTERNAL__NAMES
#define jpeg__make__c__derived__tbl	jMkCDerived
#define jpeg__gen__optimal__table	jGenOptTbl
#endif /* NEED__SHORT__EXTERNAL__NAMES */

/* Expand a Huffman table definition into the derived format */
EXTERN(void) jpeg__make__c__derived__tbl
	JPP((j__compress__ptr cinfo, boolean isDC, int tblno,
	     c__derived__tbl ** pdtbl));

/* Generate an optimal table definition given the specified counts */
EXTERN(void) jpeg__gen__optimal__table
	JPP((j__compress__ptr cinfo, JHUFF__TBL * htbl, long freq[]));
