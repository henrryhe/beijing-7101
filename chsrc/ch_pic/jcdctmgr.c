/*
 * jcdctmgr.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the forward-DCT management logic.
 * This code selects a particular DCT implementation to be used,
 * and it performs related housekeeping chores including coefficient
 * quantization.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */


/* Private subobject for this module */

typedef struct {
  struct jpeg__forward__dct pub;	/* public fields */

  /* Pointer to the DCT routine actually in use */
  forward__DCT__method__ptr do__dct;

  /* The actual post-DCT divisors --- not identical to the quant table
   * entries, because of scaling (especially for an unnormalized DCT).
   * Each table is given in normal array order.
   */
  DCTELEM * divisors[NUM__QUANT__TBLS];

#ifdef DCT__FLOAT__SUPPORTED
  /* Same as above for the floating-point case. */
  float__DCT__method__ptr do__float__dct;
  FAST__FLOAT * float__divisors[NUM__QUANT__TBLS];
#endif
} my__fdct__controller;

typedef my__fdct__controller * my__fdct__ptr;


/*
 * Initialize for a processing pass.
 * Verify that all referenced Q-tables are present, and set up
 * the divisor table for each one.
 * In the current implementation, DCT of all components is done during
 * the first pass, even if only some components will be output in the
 * first scan.  Hence all components should be examined here.
 */

METHODDEF(void)
start__pass__fdctmgr (j__compress__ptr cinfo)
{
  my__fdct__ptr fdct = (my__fdct__ptr) cinfo->fdct;
  int ci, qtblno, i;
  jpeg__component__info *compptr;
  JQUANT__TBL * qtbl;
  DCTELEM * dtbl;

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    qtblno = compptr->quant__tbl__no;
    /* Make sure specified quantization table is present */
    if (qtblno < 0 || qtblno >= NUM__QUANT__TBLS ||
	cinfo->quant__tbl__ptrs[qtblno] == NULL)
      ERREXIT1(cinfo, JERR__NO__QUANT__TABLE, qtblno);
    qtbl = cinfo->quant__tbl__ptrs[qtblno];
    /* Compute divisors for this quant table */
    /* We may do this more than once for same table, but it's not a big deal */
    switch (cinfo->dct__method) {
#ifdef DCT__ISLOW__SUPPORTED
    case JDCT__ISLOW:
      /* For LL&M IDCT method, divisors are equal to raw quantization
       * coefficients multiplied by 8 (to counteract scaling).
       */
      if (fdct->divisors[qtblno] == NULL) {
	fdct->divisors[qtblno] = (DCTELEM *)
	  (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				      DCTSIZE2 * SIZEOF(DCTELEM));
      }
      dtbl = fdct->divisors[qtblno];
      for (i = 0; i < DCTSIZE2; i++) {
	dtbl[i] = ((DCTELEM) qtbl->quantval[i]) << 3;
      }
      break;
#endif
#ifdef DCT__IFAST__SUPPORTED
    case JDCT__IFAST:
      {
	/* For AA&N IDCT method, divisors are equal to quantization
	 * coefficients scaled by scalefactor[row]*scalefactor[col], where
	 *   scalefactor[0] = 1
	 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
	 * We apply a further scale factor of 8.
	 */
#define CONST__BITS 14
	static const INT16 aanscales[DCTSIZE2] = {
	  /* precomputed values scaled up by 14 bits */
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
	  21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
	  19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
	   8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
	   4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
	};
	SHIFT__TEMPS

	if (fdct->divisors[qtblno] == NULL) {
	  fdct->divisors[qtblno] = (DCTELEM *)
	    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
					DCTSIZE2 * SIZEOF(DCTELEM));
	}
	dtbl = fdct->divisors[qtblno];
	for (i = 0; i < DCTSIZE2; i++) {
	  dtbl[i] = (DCTELEM)
	    DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[i],
				  (INT32) aanscales[i]),
		    CONST__BITS-3);
	}
      }
      break;
#endif
#ifdef DCT__FLOAT__SUPPORTED
    case JDCT__FLOAT:
      {
	/* For float AA&N IDCT method, divisors are equal to quantization
	 * coefficients scaled by scalefactor[row]*scalefactor[col], where
	 *   scalefactor[0] = 1
	 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
	 * We apply a further scale factor of 8.
	 * What's actually stored is 1/divisor so that the inner loop can
	 * use a multiplication rather than a division.
	 */
	FAST__FLOAT * fdtbl;
	int row, col;
	static const double aanscalefactor[DCTSIZE] = {
	  1.0, 1.387039845, 1.306562965, 1.175875602,
	  1.0, 0.785694958, 0.541196100, 0.275899379
	};

	if (fdct->float__divisors[qtblno] == NULL) {
	  fdct->float__divisors[qtblno] = (FAST__FLOAT *)
	    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
					DCTSIZE2 * SIZEOF(FAST__FLOAT));
	}
	fdtbl = fdct->float__divisors[qtblno];
	i = 0;
	for (row = 0; row < DCTSIZE; row++) {
	  for (col = 0; col < DCTSIZE; col++) {
	    fdtbl[i] = (FAST__FLOAT)
	      (1.0 / (((double) qtbl->quantval[i] *
		       aanscalefactor[row] * aanscalefactor[col] * 8.0)));
	    i++;
	  }
	}
      }
      break;
#endif
    default:
      ERREXIT(cinfo, JERR__NOT__COMPILED);
      break;
    }
  }
}


/*
 * Perform forward DCT on one or more blocks of a component.
 *
 * The input samples are taken from the sample__data[] array starting at
 * position start__row/start__col, and moving to the right for any additional
 * blocks. The quantized coefficients are returned in coef__blocks[].
 */

METHODDEF(void)
forward__DCT (j__compress__ptr cinfo, jpeg__component__info * compptr,
	     JSAMPARRAY sample__data, JBLOCKROW coef__blocks,
	     JDIMENSION start__row, JDIMENSION start__col,
	     JDIMENSION num__blocks)
/* This version is used for integer DCT implementations. */
{
  /* This routine is heavily used, so it's worth coding it tightly. */
  my__fdct__ptr fdct = (my__fdct__ptr) cinfo->fdct;
  forward__DCT__method__ptr do__dct = fdct->do__dct;
  DCTELEM * divisors = fdct->divisors[compptr->quant__tbl__no];
  DCTELEM workspace[DCTSIZE2];	/* work area for FDCT subroutine */
  JDIMENSION bi;

  sample__data += start__row;	/* fold in the vertical offset once */

  for (bi = 0; bi < num__blocks; bi++, start__col += DCTSIZE) {
    /* Load data into workspace, applying unsigned->signed conversion */
    { register DCTELEM *workspaceptr;
      register JSAMPROW elemptr;
      register int elemr;

      workspaceptr = workspace;
      for (elemr = 0; elemr < DCTSIZE; elemr++) {
	elemptr = sample__data[elemr] + start__col;
#if DCTSIZE == 8		/* unroll the inner loop */
	*workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
	*workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
	*workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
	*workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
	*workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
	*workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
	*workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
	*workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
#else
	{ register int elemc;
	  for (elemc = DCTSIZE; elemc > 0; elemc--) {
	    *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
	  }
	}
#endif
      }
    }

    /* Perform the DCT */
    (*do__dct) (workspace);

    /* Quantize/descale the coefficients, and store into coef__blocks[] */
    { register DCTELEM temp, qval;
      register int i;
      register JCOEFPTR output__ptr = coef__blocks[bi];

      for (i = 0; i < DCTSIZE2; i++) {
	qval = divisors[i];
	temp = workspace[i];
	/* Divide the coefficient value by qval, ensuring proper rounding.
	 * Since C does not specify the direction of rounding for negative
	 * quotients, we have to force the dividend positive for portability.
	 *
	 * In most files, at least half of the output values will be zero
	 * (at default quantization settings, more like three-quarters...)
	 * so we should ensure that this case is fast.  On many machines,
	 * a comparison is enough cheaper than a divide to make a special test
	 * a win.  Since both inputs will be nonnegative, we need only test
	 * for a < b to discover whether a/b is 0.
	 * If your machine's division is fast enough, define FAST__DIVIDE.
	 */
#ifdef FAST__DIVIDE
#define DIVIDE__BY(a,b)	a /= b
#else
#define DIVIDE__BY(a,b)	if (a >= b) a /= b; else a = 0
#endif
	if (temp < 0) {
	  temp = -temp;
	  temp += qval>>1;	/* for rounding */
	  DIVIDE__BY(temp, qval);
	  temp = -temp;
	} else {
	  temp += qval>>1;	/* for rounding */
	  DIVIDE__BY(temp, qval);
	}
	output__ptr[i] = (JCOEF) temp;
      }
    }
  }
}


#ifdef DCT__FLOAT__SUPPORTED

METHODDEF(void)
forward__DCT__float (j__compress__ptr cinfo, jpeg__component__info * compptr,
		   JSAMPARRAY sample__data, JBLOCKROW coef__blocks,
		   JDIMENSION start__row, JDIMENSION start__col,
		   JDIMENSION num__blocks)
/* This version is used for floating-point DCT implementations. */
{
  /* This routine is heavily used, so it's worth coding it tightly. */
  my__fdct__ptr fdct = (my__fdct__ptr) cinfo->fdct;
  float__DCT__method__ptr do__dct = fdct->do__float__dct;
  FAST__FLOAT * divisors = fdct->float__divisors[compptr->quant__tbl__no];
  FAST__FLOAT workspace[DCTSIZE2]; /* work area for FDCT subroutine */
  JDIMENSION bi;

  sample__data += start__row;	/* fold in the vertical offset once */

  for (bi = 0; bi < num__blocks; bi++, start__col += DCTSIZE) {
    /* Load data into workspace, applying unsigned->signed conversion */
    { register FAST__FLOAT *workspaceptr;
      register JSAMPROW elemptr;
      register int elemr;

      workspaceptr = workspace;
      for (elemr = 0; elemr < DCTSIZE; elemr++) {
	elemptr = sample__data[elemr] + start__col;
#if DCTSIZE == 8		/* unroll the inner loop */
	*workspaceptr++ = (FAST__FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST__FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST__FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST__FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST__FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST__FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST__FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST__FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
#else
	{ register int elemc;
	  for (elemc = DCTSIZE; elemc > 0; elemc--) {
	    *workspaceptr++ = (FAST__FLOAT)
	      (GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	  }
	}
#endif
      }
    }

    /* Perform the DCT */
    (*do__dct) (workspace);

    /* Quantize/descale the coefficients, and store into coef__blocks[] */
    { register FAST__FLOAT temp;
      register int i;
      register JCOEFPTR output__ptr = coef__blocks[bi];

      for (i = 0; i < DCTSIZE2; i++) {
	/* Apply the quantization and scaling factor */
	temp = workspace[i] * divisors[i];
	/* Round to nearest integer.
	 * Since C does not specify the direction of rounding for negative
	 * quotients, we have to force the dividend positive for portability.
	 * The maximum coefficient size is +-16K (for 12-bit data), so this
	 * code should work for either 16-bit or 32-bit ints.
	 */
	output__ptr[i] = (JCOEF) ((int) (temp + (FAST__FLOAT) 16384.5) - 16384);
      }
    }
  }
}

#endif /* DCT__FLOAT__SUPPORTED */


/*
 * Initialize FDCT manager.
 */

GLOBAL(void)
jinit__forward__dct (j__compress__ptr cinfo)
{
  my__fdct__ptr fdct;
  int i;

  fdct = (my__fdct__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__fdct__controller));
  cinfo->fdct = (struct jpeg__forward__dct *) fdct;
  fdct->pub.start__pass = start__pass__fdctmgr;

  switch (cinfo->dct__method) {
#ifdef DCT__ISLOW__SUPPORTED
  case JDCT__ISLOW:
    fdct->pub.forward__DCT = forward__DCT;
    fdct->do__dct = jpeg__fdct__islow;
    break;
#endif
#ifdef DCT__IFAST__SUPPORTED
  case JDCT__IFAST:
    fdct->pub.forward__DCT = forward__DCT;
    fdct->do__dct = jpeg__fdct__ifast;
    break;
#endif
#ifdef DCT__FLOAT__SUPPORTED
  case JDCT__FLOAT:
    fdct->pub.forward__DCT = forward__DCT__float;
    fdct->do__float__dct = jpeg__fdct__float;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR__NOT__COMPILED);
    break;
  }

  /* Mark divisor tables unallocated */
  for (i = 0; i < NUM__QUANT__TBLS; i++) {
    fdct->divisors[i] = NULL;
#ifdef DCT__FLOAT__SUPPORTED
    fdct->float__divisors[i] = NULL;
#endif
  }
}
