/*
 * jdcoefct.c
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the coefficient buffer controller for decompression.
 * This controller is the top level of the JPEG decompressor proper.
 * The coefficient buffer lies between entropy decoding and inverse-DCT steps.
 *
 * In buffered-image mode, this controller is the interface between
 * input-oriented processing and output-oriented processing.
 * Also, the input side (only) is used when reading a file for transcoding.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

/* Block smoothing is only applicable for progressive JPEG, so: */
#ifndef D__PROGRESSIVE__SUPPORTED
#undef BLOCK__SMOOTHING__SUPPORTED
#endif

/* Private buffer controller object */

typedef struct {
  struct jpeg__d__coef__controller pub; /* public fields */

  /* These variables keep track of the current location of the input side. */
  /* cinfo->input__iMCU__row is also used for this. */
  JDIMENSION MCU__ctr;		/* counts MCUs processed in current row */
  int MCU__vert__offset;		/* counts MCU rows within iMCU row */
  int MCU__rows__per__iMCU__row;	/* number of such rows needed */

  /* The output side's location is represented by cinfo->output__iMCU__row. */

  /* In single-pass modes, it's sufficient to buffer just one MCU.
   * We allocate a workspace of D__MAX__BLOCKS__IN__MCU coefficient blocks,
   * and let the entropy decoder write into that workspace each time.
   * (On 80x86, the workspace is FAR even though it's not really very big;
   * this is to keep the module interfaces unchanged when a large coefficient
   * buffer is necessary.)
   * In multi-pass modes, this array points to the current MCU's blocks
   * within the virtual arrays; it is used only by the input side.
   */
  JBLOCKROW MCU__buffer[D__MAX__BLOCKS__IN__MCU];

#ifdef D__MULTISCAN__FILES__SUPPORTED
  /* In multi-pass modes, we need a virtual block array for each component. */
  jvirt__barray__ptr whole__image[MAX__COMPONENTS];
#endif

#ifdef BLOCK__SMOOTHING__SUPPORTED
  /* When doing block smoothing, we latch coefficient Al values here */
  int * coef__bits__latch;
#define SAVED__COEFS  6		/* we save coef__bits[0..5] */
#endif
} my__coef__controller;

typedef my__coef__controller * my__coef__ptr;

/* Forward declarations */
METHODDEF(int) decompress__onepass
	JPP((j__decompress__ptr cinfo, JSAMPIMAGE output__buf));
#ifdef D__MULTISCAN__FILES__SUPPORTED
METHODDEF(int) decompress__data
	JPP((j__decompress__ptr cinfo, JSAMPIMAGE output__buf));
#endif
#ifdef BLOCK__SMOOTHING__SUPPORTED
LOCAL(CH_BOOL) smoothing__ok JPP((j__decompress__ptr cinfo));
METHODDEF(int) decompress__smooth__data
	JPP((j__decompress__ptr cinfo, JSAMPIMAGE output__buf));
#endif


LOCAL(void)
start__iMCU__row (j__decompress__ptr cinfo)
/* Reset within-iMCU-row counters for a new row (input side) */
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;

  /* In an interleaved scan, an MCU row is the same as an iMCU row.
   * In a noninterleaved scan, an iMCU row has v__samp__factor MCU rows.
   * But at the bottom of the image, process only what's left.
   */
  if (cinfo->comps__in__scan > 1) {
    coef->MCU__rows__per__iMCU__row = 1;
  } else {
    if (cinfo->input__iMCU__row < (cinfo->total__iMCU__rows-1))
      coef->MCU__rows__per__iMCU__row = cinfo->cur__comp__info[0]->v__samp__factor;
    else
      coef->MCU__rows__per__iMCU__row = cinfo->cur__comp__info[0]->last__row__height;
  }

  coef->MCU__ctr = 0;
  coef->MCU__vert__offset = 0;
}


/*
 * Initialize for an input processing pass.
 */

METHODDEF(void)
start__input__pass (j__decompress__ptr cinfo)
{
  cinfo->input__iMCU__row = 0;
  start__iMCU__row(cinfo);
}


/*
 * Initialize for an output processing pass.
 */

METHODDEF(void)
start__output__pass (j__decompress__ptr cinfo)
{
#ifdef BLOCK__SMOOTHING__SUPPORTED
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;

  /* If multipass, check to see whether to use block smoothing on this pass */
  if (coef->pub.coef__arrays != NULL) {
    if (cinfo->do__block__smoothing && smoothing__ok(cinfo))
      coef->pub.decompress__data = decompress__smooth__data;
    else
      coef->pub.decompress__data = decompress__data;
  }
#endif
  cinfo->output__iMCU__row = 0;
}


/*
 * Decompress and return some data in the single-pass case.
 * Always attempts to emit one fully interleaved MCU row ("iMCU" row).
 * Input and output must run in lockstep since we have only a one-MCU buffer.
 * Return value is JPEG__ROW__COMPLETED, JPEG__SCAN__COMPLETED, or JPEG__SUSPENDED.
 *
 * NB: output__buf contains a plane for each component in image,
 * which we index according to the component's SOF position.
 */

METHODDEF(int)
decompress__onepass (j__decompress__ptr cinfo, JSAMPIMAGE output__buf)
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;
  JDIMENSION MCU__col__num;	/* index of current MCU within row */
  JDIMENSION last__MCU__col = cinfo->MCUs__per__row - 1;
  JDIMENSION last__iMCU__row = cinfo->total__iMCU__rows - 1;
  int blkn, ci, xindex, yindex, yoffset, useful__width;
  JSAMPARRAY output__ptr;
  JDIMENSION start__col, output__col;
  jpeg__component__info *compptr;
  inverse__DCT__method__ptr inverse__DCT;

  /* Loop to process as much as one whole iMCU row */
  for (yoffset = coef->MCU__vert__offset; yoffset < coef->MCU__rows__per__iMCU__row;
       yoffset++) {
    for (MCU__col__num = coef->MCU__ctr; MCU__col__num <= last__MCU__col;
	 MCU__col__num++) {
      /* Try to fetch an MCU.  Entropy decoder expects buffer to be zeroed. */
      jzero__far((void FAR *) coef->MCU__buffer[0],
		(size_t) (cinfo->blocks__in__MCU * SIZEOF(JBLOCK)));
      if (! (*cinfo->entropy->decode__mcu) (cinfo, coef->MCU__buffer)) {
	/* Suspension forced; update state counters and exit */
	coef->MCU__vert__offset = yoffset;
	coef->MCU__ctr = MCU__col__num;
	return JPEG__SUSPENDED;
      }
      /* Determine where data should go in output__buf and do the IDCT thing.
       * We skip dummy blocks at the right and bottom edges (but blkn gets
       * incremented past them!).  Note the inner loop relies on having
       * allocated the MCU__buffer[] blocks sequentially.
       */
      blkn = 0;			/* index of current DCT block within MCU */
      for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
	compptr = cinfo->cur__comp__info[ci];
	/* Don't bother to IDCT an uninteresting component. */
	if (! compptr->component__needed) {
	  blkn += compptr->MCU__blocks;
	  continue;
	}
	inverse__DCT = cinfo->idct->inverse__DCT[compptr->component__index];
	useful__width = (MCU__col__num < last__MCU__col) ? compptr->MCU__width
						    : compptr->last__col__width;
	output__ptr = output__buf[compptr->component__index] +
	  yoffset * compptr->DCT__scaled__size;
	start__col = MCU__col__num * compptr->MCU__sample__width;
	for (yindex = 0; yindex < compptr->MCU__height; yindex++) {
	  if (cinfo->input__iMCU__row < last__iMCU__row ||
	      yoffset+yindex < compptr->last__row__height) {
	    output__col = start__col;
	    for (xindex = 0; xindex < useful__width; xindex++) {
	      (*inverse__DCT) (cinfo, compptr,
			      (JCOEFPTR) coef->MCU__buffer[blkn+xindex],
			      output__ptr, output__col);
	      output__col += compptr->DCT__scaled__size;
	    }
	  }
	  blkn += compptr->MCU__width;
	  output__ptr += compptr->DCT__scaled__size;
	}
      }
    }
    /* Completed an MCU row, but perhaps not an iMCU row */
    coef->MCU__ctr = 0;
  }
  /* Completed the iMCU row, advance counters for next one */
  cinfo->output__iMCU__row++;
  if (++(cinfo->input__iMCU__row) < cinfo->total__iMCU__rows) {
    start__iMCU__row(cinfo);
    return JPEG__ROW__COMPLETED;
  }
  /* Completed the scan */
  (*cinfo->inputctl->finish__input__pass) (cinfo);
  return JPEG__SCAN__COMPLETED;
}


/*
 * Dummy consume-input routine for single-pass operation.
 */

METHODDEF(int)
dummy__consume__data (j__decompress__ptr cinfo)
{
  return JPEG__SUSPENDED;	/* Always indicate nothing was done */
}


#ifdef D__MULTISCAN__FILES__SUPPORTED

/*
 * Consume input data and store it in the full-image coefficient buffer.
 * We read as much as one fully interleaved MCU row ("iMCU" row) per call,
 * ie, v__samp__factor block rows for each component in the scan.
 * Return value is JPEG__ROW__COMPLETED, JPEG__SCAN__COMPLETED, or JPEG__SUSPENDED.
 */

METHODDEF(int)
consume__data (j__decompress__ptr cinfo)
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;
  JDIMENSION MCU__col__num;	/* index of current MCU within row */
  int blkn, ci, xindex, yindex, yoffset;
  JDIMENSION start__col;
  JBLOCKARRAY buffer[MAX__COMPS__IN__SCAN];
  JBLOCKROW buffer__ptr;
  jpeg__component__info *compptr;

  /* Align the virtual buffers for the components used in this scan. */
  for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
    compptr = cinfo->cur__comp__info[ci];
    buffer[ci] = (*cinfo->mem->access__virt__barray)
      ((j__common__ptr) cinfo, coef->whole__image[compptr->component__index],
       cinfo->input__iMCU__row * compptr->v__samp__factor,
       (JDIMENSION) compptr->v__samp__factor, TRUE);
    /* Note: entropy decoder expects buffer to be zeroed,
     * but this is handled automatically by the memory manager
     * because we requested a pre-zeroed array.
     */
  }

  /* Loop to process one whole iMCU row */
  for (yoffset = coef->MCU__vert__offset; yoffset < coef->MCU__rows__per__iMCU__row;
       yoffset++) {
    for (MCU__col__num = coef->MCU__ctr; MCU__col__num < cinfo->MCUs__per__row;
	 MCU__col__num++) {
      /* Construct list of pointers to DCT blocks belonging to this MCU */
      blkn = 0;			/* index of current DCT block within MCU */
      for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
	compptr = cinfo->cur__comp__info[ci];
	start__col = MCU__col__num * compptr->MCU__width;
	for (yindex = 0; yindex < compptr->MCU__height; yindex++) {
	  buffer__ptr = buffer[ci][yindex+yoffset] + start__col;
	  for (xindex = 0; xindex < compptr->MCU__width; xindex++) {
	    coef->MCU__buffer[blkn++] = buffer__ptr++;
	  }
	}
      }
      /* Try to fetch the MCU. */
      if (! (*cinfo->entropy->decode__mcu) (cinfo, coef->MCU__buffer)) {
	/* Suspension forced; update state counters and exit */
	coef->MCU__vert__offset = yoffset;
	coef->MCU__ctr = MCU__col__num;
	return JPEG__SUSPENDED;
      }
    }
    /* Completed an MCU row, but perhaps not an iMCU row */
    coef->MCU__ctr = 0;
  }
  /* Completed the iMCU row, advance counters for next one */
  if (++(cinfo->input__iMCU__row) < cinfo->total__iMCU__rows) {
    start__iMCU__row(cinfo);
    return JPEG__ROW__COMPLETED;
  }
  /* Completed the scan */
  (*cinfo->inputctl->finish__input__pass) (cinfo);
  return JPEG__SCAN__COMPLETED;
}


/*
 * Decompress and return some data in the multi-pass case.
 * Always attempts to emit one fully interleaved MCU row ("iMCU" row).
 * Return value is JPEG__ROW__COMPLETED, JPEG__SCAN__COMPLETED, or JPEG__SUSPENDED.
 *
 * NB: output__buf contains a plane for each component in image.
 */

METHODDEF(int)
decompress__data (j__decompress__ptr cinfo, JSAMPIMAGE output__buf)
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;
  JDIMENSION last__iMCU__row = cinfo->total__iMCU__rows - 1;
  JDIMENSION block__num;
  int ci, block__row, block__rows;
  JBLOCKARRAY buffer;
  JBLOCKROW buffer__ptr;
  JSAMPARRAY output__ptr;
  JDIMENSION output__col;
  jpeg__component__info *compptr;
  inverse__DCT__method__ptr inverse__DCT;

  /* Force some input to be done if we are getting ahead of the input. */
  while (cinfo->input__scan__number < cinfo->output__scan__number ||
	 (cinfo->input__scan__number == cinfo->output__scan__number &&
	  cinfo->input__iMCU__row <= cinfo->output__iMCU__row)) {
    if ((*cinfo->inputctl->consume__input)(cinfo) == JPEG__SUSPENDED)
      return JPEG__SUSPENDED;
  }

  /* OK, output from the virtual arrays. */
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    /* Don't bother to IDCT an uninteresting component. */
    if (! compptr->component__needed)
      continue;
    /* Align the virtual buffer for this component. */
    buffer = (*cinfo->mem->access__virt__barray)
      ((j__common__ptr) cinfo, coef->whole__image[ci],
       cinfo->output__iMCU__row * compptr->v__samp__factor,
       (JDIMENSION) compptr->v__samp__factor, FALSE);
    /* Count non-dummy DCT block rows in this iMCU row. */
    if (cinfo->output__iMCU__row < last__iMCU__row)
      block__rows = compptr->v__samp__factor;
    else {
      /* NB: can't use last__row__height here; it is input-side-dependent! */
      block__rows = (int) (compptr->height__in__blocks % compptr->v__samp__factor);
      if (block__rows == 0) block__rows = compptr->v__samp__factor;
    }
    inverse__DCT = cinfo->idct->inverse__DCT[ci];
    output__ptr = output__buf[ci];
    /* Loop over all DCT blocks to be processed. */
    for (block__row = 0; block__row < block__rows; block__row++) {
      buffer__ptr = buffer[block__row];
      output__col = 0;
      for (block__num = 0; block__num < compptr->width__in__blocks; block__num++) {
	(*inverse__DCT) (cinfo, compptr, (JCOEFPTR) buffer__ptr,
			output__ptr, output__col);
	buffer__ptr++;
	output__col += compptr->DCT__scaled__size;
      }
      output__ptr += compptr->DCT__scaled__size;
    }
  }

  if (++(cinfo->output__iMCU__row) < cinfo->total__iMCU__rows)
    return JPEG__ROW__COMPLETED;
  return JPEG__SCAN__COMPLETED;
}

#endif /* D__MULTISCAN__FILES__SUPPORTED */


#ifdef BLOCK__SMOOTHING__SUPPORTED

/*
 * This code applies interblock smoothing as described by section K.8
 * of the JPEG standard: the first 5 AC coefficients are estimated from
 * the DC values of a DCT block and its 8 neighboring blocks.
 * We apply smoothing only for progressive JPEG decoding, and only if
 * the coefficients it can estimate are not yet known to full precision.
 */

/* Natural-order array positions of the first 5 zigzag-order coefficients */
#define Q01__POS  1
#define Q10__POS  8
#define Q20__POS  16
#define Q11__POS  9
#define Q02__POS  2

/*
 * Determine whether block smoothing is applicable and safe.
 * We also latch the current states of the coef__bits[] entries for the
 * AC coefficients; otherwise, if the input side of the decompressor
 * advances into a new scan, we might think the coefficients are known
 * more accurately than they really are.
 */

LOCAL(CH_BOOL)
smoothing__ok (j__decompress__ptr cinfo)
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;
  CH_BOOL smoothing__useful = FALSE;
  int ci, coefi;
  jpeg__component__info *compptr;
  JQUANT__TBL * qtable;
  int * coef__bits;
  int * coef__bits__latch;

  if (! cinfo->progressive__mode || cinfo->coef__bits == NULL)
    return FALSE;

  /* Allocate latch area if not already done */
  if (coef->coef__bits__latch == NULL)
    coef->coef__bits__latch = (int *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  cinfo->num__components *
				  (SAVED__COEFS * SIZEOF(int)));
  coef__bits__latch = coef->coef__bits__latch;

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    /* All components' quantization values must already be latched. */
    if ((qtable = compptr->quant__table) == NULL)
      return FALSE;
    /* Verify DC & first 5 AC quantizers are nonzero to avoid zero-divide. */
    if (qtable->quantval[0] == 0 ||
	qtable->quantval[Q01__POS] == 0 ||
	qtable->quantval[Q10__POS] == 0 ||
	qtable->quantval[Q20__POS] == 0 ||
	qtable->quantval[Q11__POS] == 0 ||
	qtable->quantval[Q02__POS] == 0)
      return FALSE;
    /* DC values must be at least partly known for all components. */
    coef__bits = cinfo->coef__bits[ci];
    if (coef__bits[0] < 0)
      return FALSE;
    /* Block smoothing is helpful if some AC coefficients remain inaccurate. */
    for (coefi = 1; coefi <= 5; coefi++) {
      coef__bits__latch[coefi] = coef__bits[coefi];
      if (coef__bits[coefi] != 0)
	smoothing__useful = TRUE;
    }
    coef__bits__latch += SAVED__COEFS;
  }

  return smoothing__useful;
}


/*
 * Variant of decompress__data for use when doing block smoothing.
 */

METHODDEF(int)
decompress__smooth__data (j__decompress__ptr cinfo, JSAMPIMAGE output__buf)
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;
  JDIMENSION last__iMCU__row = cinfo->total__iMCU__rows - 1;
  JDIMENSION block__num, last__block__column;
  int ci, block__row, block__rows, access__rows;
  JBLOCKARRAY buffer;
  JBLOCKROW buffer__ptr, prev__block__row, next__block__row;
  JSAMPARRAY output__ptr;
  JDIMENSION output__col;
  jpeg__component__info *compptr;
  inverse__DCT__method__ptr inverse__DCT;
  CH_BOOL first__row, last__row;
  JBLOCK workspace;
  int *coef__bits;
  JQUANT__TBL *quanttbl;
  INT32 Q00,Q01,Q02,Q10,Q11,Q20, num;
  int DC1,DC2,DC3,DC4,DC5,DC6,DC7,DC8,DC9;
  int Al, pred;

  /* Force some input to be done if we are getting ahead of the input. */
  while (cinfo->input__scan__number <= cinfo->output__scan__number &&
	 ! cinfo->inputctl->eoi__reached) {
    if (cinfo->input__scan__number == cinfo->output__scan__number) {
      /* If input is working on current scan, we ordinarily want it to
       * have completed the current row.  But if input scan is DC,
       * we want it to keep one row ahead so that next block row's DC
       * values are up to date.
       */
      JDIMENSION delta = (cinfo->Ss == 0) ? 1 : 0;
      if (cinfo->input__iMCU__row > cinfo->output__iMCU__row+delta)
	break;
    }
    if ((*cinfo->inputctl->consume__input)(cinfo) == JPEG__SUSPENDED)
      return JPEG__SUSPENDED;
  }

  /* OK, output from the virtual arrays. */
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    /* Don't bother to IDCT an uninteresting component. */
    if (! compptr->component__needed)
      continue;
    /* Count non-dummy DCT block rows in this iMCU row. */
    if (cinfo->output__iMCU__row < last__iMCU__row) {
      block__rows = compptr->v__samp__factor;
      access__rows = block__rows * 2; /* this and next iMCU row */
      last__row = FALSE;
    } else {
      /* NB: can't use last__row__height here; it is input-side-dependent! */
      block__rows = (int) (compptr->height__in__blocks % compptr->v__samp__factor);
      if (block__rows == 0) block__rows = compptr->v__samp__factor;
      access__rows = block__rows; /* this iMCU row only */
      last__row = TRUE;
    }
    /* Align the virtual buffer for this component. */
    if (cinfo->output__iMCU__row > 0) {
      access__rows += compptr->v__samp__factor; /* prior iMCU row too */
      buffer = (*cinfo->mem->access__virt__barray)
	((j__common__ptr) cinfo, coef->whole__image[ci],
	 (cinfo->output__iMCU__row - 1) * compptr->v__samp__factor,
	 (JDIMENSION) access__rows, FALSE);
      buffer += compptr->v__samp__factor;	/* point to current iMCU row */
      first__row = FALSE;
    } else {
      buffer = (*cinfo->mem->access__virt__barray)
	((j__common__ptr) cinfo, coef->whole__image[ci],
	 (JDIMENSION) 0, (JDIMENSION) access__rows, FALSE);
      first__row = TRUE;
    }
    /* Fetch component-dependent info */
    coef__bits = coef->coef__bits__latch + (ci * SAVED__COEFS);
    quanttbl = compptr->quant__table;
    Q00 = quanttbl->quantval[0];
    Q01 = quanttbl->quantval[Q01__POS];
    Q10 = quanttbl->quantval[Q10__POS];
    Q20 = quanttbl->quantval[Q20__POS];
    Q11 = quanttbl->quantval[Q11__POS];
    Q02 = quanttbl->quantval[Q02__POS];
    inverse__DCT = cinfo->idct->inverse__DCT[ci];
    output__ptr = output__buf[ci];
    /* Loop over all DCT blocks to be processed. */
    for (block__row = 0; block__row < block__rows; block__row++) {
      buffer__ptr = buffer[block__row];
      if (first__row && block__row == 0)
	prev__block__row = buffer__ptr;
      else
	prev__block__row = buffer[block__row-1];
      if (last__row && block__row == block__rows-1)
	next__block__row = buffer__ptr;
      else
	next__block__row = buffer[block__row+1];
      /* We fetch the surrounding DC values using a sliding-register approach.
       * Initialize all nine here so as to do the right thing on narrow pics.
       */
      DC1 = DC2 = DC3 = (int) prev__block__row[0][0];
      DC4 = DC5 = DC6 = (int) buffer__ptr[0][0];
      DC7 = DC8 = DC9 = (int) next__block__row[0][0];
      output__col = 0;
      last__block__column = compptr->width__in__blocks - 1;
      for (block__num = 0; block__num <= last__block__column; block__num++) {
	/* Fetch current DCT block into workspace so we can modify it. */
	jcopy__block__row(buffer__ptr, (JBLOCKROW) workspace, (JDIMENSION) 1);
	/* Update DC values */
	if (block__num < last__block__column) {
	  DC3 = (int) prev__block__row[1][0];
	  DC6 = (int) buffer__ptr[1][0];
	  DC9 = (int) next__block__row[1][0];
	}
	/* Compute coefficient estimates per K.8.
	 * An estimate is applied only if coefficient is still zero,
	 * and is not known to be fully accurate.
	 */
	/* AC01 */
	if ((Al=coef__bits[1]) != 0 && workspace[1] == 0) {
	  num = 36 * Q00 * (DC4 - DC6);
	  if (num >= 0) {
	    pred = (int) (((Q01<<7) + num) / (Q01<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	  } else {
	    pred = (int) (((Q01<<7) - num) / (Q01<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	    pred = -pred;
	  }
	  workspace[1] = (JCOEF) pred;
	}
	/* AC10 */
	if ((Al=coef__bits[2]) != 0 && workspace[8] == 0) {
	  num = 36 * Q00 * (DC2 - DC8);
	  if (num >= 0) {
	    pred = (int) (((Q10<<7) + num) / (Q10<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	  } else {
	    pred = (int) (((Q10<<7) - num) / (Q10<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	    pred = -pred;
	  }
	  workspace[8] = (JCOEF) pred;
	}
	/* AC20 */
	if ((Al=coef__bits[3]) != 0 && workspace[16] == 0) {
	  num = 9 * Q00 * (DC2 + DC8 - 2*DC5);
	  if (num >= 0) {
	    pred = (int) (((Q20<<7) + num) / (Q20<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	  } else {
	    pred = (int) (((Q20<<7) - num) / (Q20<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	    pred = -pred;
	  }
	  workspace[16] = (JCOEF) pred;
	}
	/* AC11 */
	if ((Al=coef__bits[4]) != 0 && workspace[9] == 0) {
	  num = 5 * Q00 * (DC1 - DC3 - DC7 + DC9);
	  if (num >= 0) {
	    pred = (int) (((Q11<<7) + num) / (Q11<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	  } else {
	    pred = (int) (((Q11<<7) - num) / (Q11<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	    pred = -pred;
	  }
	  workspace[9] = (JCOEF) pred;
	}
	/* AC02 */
	if ((Al=coef__bits[5]) != 0 && workspace[2] == 0) {
	  num = 9 * Q00 * (DC4 + DC6 - 2*DC5);
	  if (num >= 0) {
	    pred = (int) (((Q02<<7) + num) / (Q02<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	  } else {
	    pred = (int) (((Q02<<7) - num) / (Q02<<8));
	    if (Al > 0 && pred >= (1<<Al))
	      pred = (1<<Al)-1;
	    pred = -pred;
	  }
	  workspace[2] = (JCOEF) pred;
	}
	/* OK, do the IDCT */
	(*inverse__DCT) (cinfo, compptr, (JCOEFPTR) workspace,
			output__ptr, output__col);
	/* Advance for next column */
	DC1 = DC2; DC2 = DC3;
	DC4 = DC5; DC5 = DC6;
	DC7 = DC8; DC8 = DC9;
	buffer__ptr++, prev__block__row++, next__block__row++;
	output__col += compptr->DCT__scaled__size;
      }
      output__ptr += compptr->DCT__scaled__size;
    }
  }

  if (++(cinfo->output__iMCU__row) < cinfo->total__iMCU__rows)
    return JPEG__ROW__COMPLETED;
  return JPEG__SCAN__COMPLETED;
}

#endif /* BLOCK__SMOOTHING__SUPPORTED */


/*
 * Initialize coefficient buffer controller.
 */

GLOBAL(void)
jinit__d__coef__controller (j__decompress__ptr cinfo, CH_BOOL need__full__buffer)
{
  my__coef__ptr coef;

  coef = (my__coef__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__coef__controller));
  cinfo->coef = (struct jpeg__d__coef__controller *) coef;
  coef->pub.start__input__pass = start__input__pass;
  coef->pub.start__output__pass = start__output__pass;
#ifdef BLOCK__SMOOTHING__SUPPORTED
  coef->coef__bits__latch = NULL;
#endif

  /* Create the coefficient buffer. */
  if (need__full__buffer) {
#ifdef D__MULTISCAN__FILES__SUPPORTED
    /* Allocate a full-image virtual array for each component, */
    /* padded to a multiple of samp__factor DCT blocks in each direction. */
    /* Note we ask for a pre-zeroed array. */
    int ci, access__rows;
    jpeg__component__info *compptr;

    for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	 ci++, compptr++) {
      access__rows = compptr->v__samp__factor;
#ifdef BLOCK__SMOOTHING__SUPPORTED
      /* If block smoothing could be used, need a bigger window */
      if (cinfo->progressive__mode)
	access__rows *= 3;
#endif
      coef->whole__image[ci] = (*cinfo->mem->request__virt__barray)
	((j__common__ptr) cinfo, JPOOL__IMAGE, TRUE,
	 (JDIMENSION) jround__up((long) compptr->width__in__blocks,
				(long) compptr->h__samp__factor),
	 (JDIMENSION) jround__up((long) compptr->height__in__blocks,
				(long) compptr->v__samp__factor),
	 (JDIMENSION) access__rows);
    }
    coef->pub.consume__data = consume__data;
    coef->pub.decompress__data = decompress__data;
    coef->pub.coef__arrays = coef->whole__image; /* link to virtual arrays */
#else
    ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
  } else {
    /* We only need a single-MCU buffer. */
    JBLOCKROW buffer;
    int i;

    buffer = (JBLOCKROW)
      (*cinfo->mem->alloc__large) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  D__MAX__BLOCKS__IN__MCU * SIZEOF(JBLOCK));
    for (i = 0; i < D__MAX__BLOCKS__IN__MCU; i++) {
      coef->MCU__buffer[i] = buffer + i;
    }
    coef->pub.consume__data = dummy__consume__data;
    coef->pub.decompress__data = decompress__onepass;
    coef->pub.coef__arrays = NULL; /* flag for no virtual arrays */
  }
}
