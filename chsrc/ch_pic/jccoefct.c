/*
 * jccoefct.c
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the coefficient buffer controller for compression.
 * This controller is the top level of the JPEG compressor proper.
 * The coefficient buffer lies between forward-DCT and entropy encoding steps.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* We use a full-image coefficient buffer when doing Huffman optimization,
 * and also for writing multiple-scan JPEG files.  In all cases, the DCT
 * step is run during the first pass, and subsequent passes need only read
 * the buffered coefficients.
 */
#ifdef ENTROPY__OPT__SUPPORTED
#define FULL__COEF__BUFFER__SUPPORTED
#else
#ifdef C__MULTISCAN__FILES__SUPPORTED
#define FULL__COEF__BUFFER__SUPPORTED
#endif
#endif


/* Private buffer controller object */

typedef struct {
  struct jpeg__c__coef__controller pub; /* public fields */

  JDIMENSION iMCU__row__num;	/* iMCU row # within image */
  JDIMENSION mcu__ctr;		/* counts MCUs processed in current row */
  int MCU__vert__offset;		/* counts MCU rows within iMCU row */
  int MCU__rows__per__iMCU__row;	/* number of such rows needed */

  /* For single-pass compression, it's sufficient to buffer just one MCU
   * (although this may prove a bit slow in practice).  We allocate a
   * workspace of C__MAX__BLOCKS__IN__MCU coefficient blocks, and reuse it for each
   * MCU constructed and sent.  (On 80x86, the workspace is FAR even though
   * it's not really very big; this is to keep the module interfaces unchanged
   * when a large coefficient buffer is necessary.)
   * In multi-pass modes, this array points to the current MCU's blocks
   * within the virtual arrays.
   */
  JBLOCKROW MCU__buffer[C__MAX__BLOCKS__IN__MCU];

  /* In multi-pass modes, we need a virtual block array for each component. */
  jvirt__barray__ptr whole__image[MAX__COMPONENTS];
} my__coef__controller;

typedef my__coef__controller * my__coef__ptr;


/* Forward declarations */
METHODDEF(CH_BOOL) compress__data
    JPP((j__compress__ptr cinfo, JSAMPIMAGE input__buf));
#ifdef FULL__COEF__BUFFER__SUPPORTED
METHODDEF(CH_BOOL) compress__first__pass
    JPP((j__compress__ptr cinfo, JSAMPIMAGE input__buf));
METHODDEF(CH_BOOL) compress__output
    JPP((j__compress__ptr cinfo, JSAMPIMAGE input__buf));
#endif


LOCAL(void)
start__iMCU__row (j__compress__ptr cinfo)
/* Reset within-iMCU-row counters for a new row */
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;

  /* In an interleaved scan, an MCU row is the same as an iMCU row.
   * In a noninterleaved scan, an iMCU row has v__samp__factor MCU rows.
   * But at the bottom of the image, process only what's left.
   */
  if (cinfo->comps__in__scan > 1) {
    coef->MCU__rows__per__iMCU__row = 1;
  } else {
    if (coef->iMCU__row__num < (cinfo->total__iMCU__rows-1))
      coef->MCU__rows__per__iMCU__row = cinfo->cur__comp__info[0]->v__samp__factor;
    else
      coef->MCU__rows__per__iMCU__row = cinfo->cur__comp__info[0]->last__row__height;
  }

  coef->mcu__ctr = 0;
  coef->MCU__vert__offset = 0;
}


/*
 * Initialize for a processing pass.
 */

METHODDEF(void)
start__pass__coef (j__compress__ptr cinfo, J__BUF__MODE pass__mode)
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;

  coef->iMCU__row__num = 0;
  start__iMCU__row(cinfo);

  switch (pass__mode) {
  case JBUF__PASS__THRU:
    if (coef->whole__image[0] != NULL)
      ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    coef->pub.compress__data = compress__data;
    break;
#ifdef FULL__COEF__BUFFER__SUPPORTED
  case JBUF__SAVE__AND__PASS:
    if (coef->whole__image[0] == NULL)
      ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    coef->pub.compress__data = compress__first__pass;
    break;
  case JBUF__CRANK__DEST:
    if (coef->whole__image[0] == NULL)
      ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    coef->pub.compress__data = compress__output;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    break;
  }
}


/*
 * Process some data in the single-pass case.
 * We process the equivalent of one fully interleaved MCU row ("iMCU" row)
 * per call, ie, v__samp__factor block rows for each component in the image.
 * Returns TRUE if the iMCU row is completed, FALSE if suspended.
 *
 * NB: input__buf contains a plane for each component in image,
 * which we index according to the component's SOF position.
 */

METHODDEF(CH_BOOL)
compress__data (j__compress__ptr cinfo, JSAMPIMAGE input__buf)
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;
  JDIMENSION MCU__col__num;	/* index of current MCU within row */
  JDIMENSION last__MCU__col = cinfo->MCUs__per__row - 1;
  JDIMENSION last__iMCU__row = cinfo->total__iMCU__rows - 1;
  int blkn, bi, ci, yindex, yoffset, blockcnt;
  JDIMENSION ypos, xpos;
  jpeg__component__info *compptr;

  /* Loop to write as much as one whole iMCU row */
  for (yoffset = coef->MCU__vert__offset; yoffset < coef->MCU__rows__per__iMCU__row;
       yoffset++) {
    for (MCU__col__num = coef->mcu__ctr; MCU__col__num <= last__MCU__col;
	 MCU__col__num++) {
      /* Determine where data comes from in input__buf and do the DCT thing.
       * Each call on forward__DCT processes a horizontal row of DCT blocks
       * as wide as an MCU; we rely on having allocated the MCU__buffer[] blocks
       * sequentially.  Dummy blocks at the right or bottom edge are filled in
       * specially.  The data in them does not matter for image reconstruction,
       * so we fill them with values that will encode to the smallest amount of
       * data, viz: all zeroes in the AC entries, DC entries equal to previous
       * block's DC value.  (Thanks to Thomas Kinsman for this idea.)
       */
      blkn = 0;
      for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
	compptr = cinfo->cur__comp__info[ci];
	blockcnt = (MCU__col__num < last__MCU__col) ? compptr->MCU__width
						: compptr->last__col__width;
	xpos = MCU__col__num * compptr->MCU__sample__width;
	ypos = yoffset * DCTSIZE; /* ypos == (yoffset+yindex) * DCTSIZE */
	for (yindex = 0; yindex < compptr->MCU__height; yindex++) {
	  if (coef->iMCU__row__num < last__iMCU__row ||
	      yoffset+yindex < compptr->last__row__height) {
	    (*cinfo->fdct->forward__DCT) (cinfo, compptr,
					 input__buf[compptr->component__index],
					 coef->MCU__buffer[blkn],
					 ypos, xpos, (JDIMENSION) blockcnt);
	    if (blockcnt < compptr->MCU__width) {
	      /* Create some dummy blocks at the right edge of the image. */
	      jzero__far((void FAR *) coef->MCU__buffer[blkn + blockcnt],
			(compptr->MCU__width - blockcnt) * SIZEOF(JBLOCK));
	      for (bi = blockcnt; bi < compptr->MCU__width; bi++) {
		coef->MCU__buffer[blkn+bi][0][0] = coef->MCU__buffer[blkn+bi-1][0][0];
	      }
	    }
	  } else {
	    /* Create a row of dummy blocks at the bottom of the image. */
	    jzero__far((void FAR *) coef->MCU__buffer[blkn],
		      compptr->MCU__width * SIZEOF(JBLOCK));
	    for (bi = 0; bi < compptr->MCU__width; bi++) {
	      coef->MCU__buffer[blkn+bi][0][0] = coef->MCU__buffer[blkn-1][0][0];
	    }
	  }
	  blkn += compptr->MCU__width;
	  ypos += DCTSIZE;
	}
      }
      /* Try to write the MCU.  In event of a suspension failure, we will
       * re-DCT the MCU on restart (a bit inefficient, could be fixed...)
       */
      if (! (*cinfo->entropy->encode__mcu) (cinfo, coef->MCU__buffer)) {
	/* Suspension forced; update state counters and exit */
	coef->MCU__vert__offset = yoffset;
	coef->mcu__ctr = MCU__col__num;
	return FALSE;
      }
    }
    /* Completed an MCU row, but perhaps not an iMCU row */
    coef->mcu__ctr = 0;
  }
  /* Completed the iMCU row, advance counters for next one */
  coef->iMCU__row__num++;
  start__iMCU__row(cinfo);
  return TRUE;
}


#ifdef FULL__COEF__BUFFER__SUPPORTED

/*
 * Process some data in the first pass of a multi-pass case.
 * We process the equivalent of one fully interleaved MCU row ("iMCU" row)
 * per call, ie, v__samp__factor block rows for each component in the image.
 * This amount of data is read from the source buffer, DCT'd and quantized,
 * and saved into the virtual arrays.  We also generate suitable dummy blocks
 * as needed at the right and lower edges.  (The dummy blocks are constructed
 * in the virtual arrays, which have been padded appropriately.)  This makes
 * it possible for subsequent passes not to worry about real vs. dummy blocks.
 *
 * We must also emit the data to the entropy encoder.  This is conveniently
 * done by calling compress__output() after we've loaded the current strip
 * of the virtual arrays.
 *
 * NB: input__buf contains a plane for each component in image.  All
 * components are DCT'd and loaded into the virtual arrays in this pass.
 * However, it may be that only a subset of the components are emitted to
 * the entropy encoder during this first pass; be careful about looking
 * at the scan-dependent variables (MCU dimensions, etc).
 */

METHODDEF(CH_BOOL)
compress__first__pass (j__compress__ptr cinfo, JSAMPIMAGE input__buf)
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;
  JDIMENSION last__iMCU__row = cinfo->total__iMCU__rows - 1;
  JDIMENSION blocks__across, MCUs__across, MCUindex;
  int bi, ci, h__samp__factor, block__row, block__rows, ndummy;
  JCOEF lastDC;
  jpeg__component__info *compptr;
  JBLOCKARRAY buffer;
  JBLOCKROW thisblockrow, lastblockrow;

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    /* Align the virtual buffer for this component. */
    buffer = (*cinfo->mem->access__virt__barray)
      ((j__common__ptr) cinfo, coef->whole__image[ci],
       coef->iMCU__row__num * compptr->v__samp__factor,
       (JDIMENSION) compptr->v__samp__factor, TRUE);
    /* Count non-dummy DCT block rows in this iMCU row. */
    if (coef->iMCU__row__num < last__iMCU__row)
      block__rows = compptr->v__samp__factor;
    else {
      /* NB: can't use last__row__height here, since may not be set! */
      block__rows = (int) (compptr->height__in__blocks % compptr->v__samp__factor);
      if (block__rows == 0) block__rows = compptr->v__samp__factor;
    }
    blocks__across = compptr->width__in__blocks;
    h__samp__factor = compptr->h__samp__factor;
    /* Count number of dummy blocks to be added at the right margin. */
    ndummy = (int) (blocks__across % h__samp__factor);
    if (ndummy > 0)
      ndummy = h__samp__factor - ndummy;
    /* Perform DCT for all non-dummy blocks in this iMCU row.  Each call
     * on forward__DCT processes a complete horizontal row of DCT blocks.
     */
    for (block__row = 0; block__row < block__rows; block__row++) {
      thisblockrow = buffer[block__row];
      (*cinfo->fdct->forward__DCT) (cinfo, compptr,
				   input__buf[ci], thisblockrow,
				   (JDIMENSION) (block__row * DCTSIZE),
				   (JDIMENSION) 0, blocks__across);
      if (ndummy > 0) {
	/* Create dummy blocks at the right edge of the image. */
	thisblockrow += blocks__across; /* => first dummy block */
	jzero__far((void FAR *) thisblockrow, ndummy * SIZEOF(JBLOCK));
	lastDC = thisblockrow[-1][0];
	for (bi = 0; bi < ndummy; bi++) {
	  thisblockrow[bi][0] = lastDC;
	}
      }
    }
    /* If at end of image, create dummy block rows as needed.
     * The tricky part here is that within each MCU, we want the DC values
     * of the dummy blocks to match the last real block's DC value.
     * This squeezes a few more bytes out of the resulting file...
     */
    if (coef->iMCU__row__num == last__iMCU__row) {
      blocks__across += ndummy;	/* include lower right corner */
      MCUs__across = blocks__across / h__samp__factor;
      for (block__row = block__rows; block__row < compptr->v__samp__factor;
	   block__row++) {
	thisblockrow = buffer[block__row];
	lastblockrow = buffer[block__row-1];
	jzero__far((void FAR *) thisblockrow,
		  (size_t) (blocks__across * SIZEOF(JBLOCK)));
	for (MCUindex = 0; MCUindex < MCUs__across; MCUindex++) {
	  lastDC = lastblockrow[h__samp__factor-1][0];
	  for (bi = 0; bi < h__samp__factor; bi++) {
	    thisblockrow[bi][0] = lastDC;
	  }
	  thisblockrow += h__samp__factor; /* advance to next MCU in row */
	  lastblockrow += h__samp__factor;
	}
      }
    }
  }
  /* NB: compress__output will increment iMCU__row__num if successful.
   * A suspension return will result in redoing all the work above next time.
   */

  /* Emit data to the entropy encoder, sharing code with subsequent passes */
  return compress__output(cinfo, input__buf);
}


/*
 * Process some data in subsequent passes of a multi-pass case.
 * We process the equivalent of one fully interleaved MCU row ("iMCU" row)
 * per call, ie, v__samp__factor block rows for each component in the scan.
 * The data is obtained from the virtual arrays and fed to the entropy coder.
 * Returns TRUE if the iMCU row is completed, FALSE if suspended.
 *
 * NB: input__buf is ignored; it is likely to be a NULL pointer.
 */

METHODDEF(CH_BOOL)
compress__output (j__compress__ptr cinfo, JSAMPIMAGE input__buf)
{
  my__coef__ptr coef = (my__coef__ptr) cinfo->coef;
  JDIMENSION MCU__col__num;	/* index of current MCU within row */
  int blkn, ci, xindex, yindex, yoffset;
  JDIMENSION start__col;
  JBLOCKARRAY buffer[MAX__COMPS__IN__SCAN];
  JBLOCKROW buffer__ptr;
  jpeg__component__info *compptr;

  /* Align the virtual buffers for the components used in this scan.
   * NB: during first pass, this is safe only because the buffers will
   * already be aligned properly, so jmemmgr.c won't need to do any I/O.
   */
  for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
    compptr = cinfo->cur__comp__info[ci];
    buffer[ci] = (*cinfo->mem->access__virt__barray)
      ((j__common__ptr) cinfo, coef->whole__image[compptr->component__index],
       coef->iMCU__row__num * compptr->v__samp__factor,
       (JDIMENSION) compptr->v__samp__factor, FALSE);
  }

  /* Loop to process one whole iMCU row */
  for (yoffset = coef->MCU__vert__offset; yoffset < coef->MCU__rows__per__iMCU__row;
       yoffset++) {
    for (MCU__col__num = coef->mcu__ctr; MCU__col__num < cinfo->MCUs__per__row;
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
      /* Try to write the MCU. */
      if (! (*cinfo->entropy->encode__mcu) (cinfo, coef->MCU__buffer)) {
	/* Suspension forced; update state counters and exit */
	coef->MCU__vert__offset = yoffset;
	coef->mcu__ctr = MCU__col__num;
	return FALSE;
      }
    }
    /* Completed an MCU row, but perhaps not an iMCU row */
    coef->mcu__ctr = 0;
  }
  /* Completed the iMCU row, advance counters for next one */
  coef->iMCU__row__num++;
  start__iMCU__row(cinfo);
  return TRUE;
}

#endif /* FULL__COEF__BUFFER__SUPPORTED */


/*
 * Initialize coefficient buffer controller.
 */

GLOBAL(void)
jinit__c__coef__controller (j__compress__ptr cinfo, CH_BOOL need__full__buffer)
{
  my__coef__ptr coef;

  coef = (my__coef__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__coef__controller));
  cinfo->coef = (struct jpeg__c__coef__controller *) coef;
  coef->pub.start__pass = start__pass__coef;

  /* Create the coefficient buffer. */
  if (need__full__buffer) {
#ifdef FULL__COEF__BUFFER__SUPPORTED
    /* Allocate a full-image virtual array for each component, */
    /* padded to a multiple of samp__factor DCT blocks in each direction. */
    int ci;
    jpeg__component__info *compptr;

    for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	 ci++, compptr++) {
      coef->whole__image[ci] = (*cinfo->mem->request__virt__barray)
	((j__common__ptr) cinfo, JPOOL__IMAGE, FALSE,
	 (JDIMENSION) jround__up((long) compptr->width__in__blocks,
				(long) compptr->h__samp__factor),
	 (JDIMENSION) jround__up((long) compptr->height__in__blocks,
				(long) compptr->v__samp__factor),
	 (JDIMENSION) compptr->v__samp__factor);
    }
#else
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
#endif
  } else {
    /* We only need a single-MCU buffer. */
    JBLOCKROW buffer;
    int i;

    buffer = (JBLOCKROW)
      (*cinfo->mem->alloc__large) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  C__MAX__BLOCKS__IN__MCU * SIZEOF(JBLOCK));
    for (i = 0; i < C__MAX__BLOCKS__IN__MCU; i++) {
      coef->MCU__buffer[i] = buffer + i;
    }
    coef->whole__image[0] = NULL; /* flag for no virtual arrays */
  }
}
