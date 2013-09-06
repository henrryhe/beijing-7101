/*
 * jdpostct.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the decompression postprocessing controller.
 * This controller manages the upsampling, color conversion, and color
 * quantization/reduction steps; specifically, it controls the buffering
 * between upsample/color conversion and color quantization/reduction.
 *
 * If no color quantization/reduction is required, then this module has no
 * work to do, and it just hands off to the upsample/color conversion code.
 * An integrated upsample/convert/quantize process would replace this module
 * entirely.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Private buffer controller object */

typedef struct {
  struct jpeg__d__post__controller pub; /* public fields */

  /* Color quantization source buffer: this holds output data from
   * the upsample/color conversion step to be passed to the quantizer.
   * For two-pass color quantization, we need a full-image buffer;
   * for one-pass operation, a strip buffer is sufficient.
   */
  jvirt__sarray__ptr whole__image;	/* virtual array, or NULL if one-pass */
  JSAMPARRAY buffer;		/* strip buffer, or current strip of virtual */
  JDIMENSION strip__height;	/* buffer size in rows */
  /* for two-pass mode only: */
  JDIMENSION starting__row;	/* row # of first row in current strip */
  JDIMENSION next__row;		/* index of next row to fill/empty in strip */
} my__post__controller;

typedef my__post__controller * my__post__ptr;


/* Forward declarations */
METHODDEF(void) post__process__1pass
	JPP((j__decompress__ptr cinfo,
	     JSAMPIMAGE input__buf, JDIMENSION *in__row__group__ctr,
	     JDIMENSION in__row__groups__avail,
	     JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
	     JDIMENSION out__rows__avail));
#ifdef QUANT__2PASS__SUPPORTED
METHODDEF(void) post__process__prepass
	JPP((j__decompress__ptr cinfo,
	     JSAMPIMAGE input__buf, JDIMENSION *in__row__group__ctr,
	     JDIMENSION in__row__groups__avail,
	     JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
	     JDIMENSION out__rows__avail));
METHODDEF(void) post__process__2pass
	JPP((j__decompress__ptr cinfo,
	     JSAMPIMAGE input__buf, JDIMENSION *in__row__group__ctr,
	     JDIMENSION in__row__groups__avail,
	     JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
	     JDIMENSION out__rows__avail));
#endif


/*
 * Initialize for a processing pass.
 */

METHODDEF(void)
start__pass__dpost (j__decompress__ptr cinfo, J__BUF__MODE pass__mode)
{
  my__post__ptr post = (my__post__ptr) cinfo->post;

  switch (pass__mode) {
  case JBUF__PASS__THRU:
    if (cinfo->quantize__colors) {
      /* Single-pass processing with color quantization. */
      post->pub.post__process__data = post__process__1pass;
      /* We could be doing buffered-image output before starting a 2-pass
       * color quantization; in that case, jinit__d__post__controller did not
       * allocate a strip buffer.  Use the virtual-array buffer as workspace.
       */
      if (post->buffer == NULL) {
	post->buffer = (*cinfo->mem->access__virt__sarray)
	  ((j__common__ptr) cinfo, post->whole__image,
	   (JDIMENSION) 0, post->strip__height, TRUE);
      }
    } else {
      /* For single-pass processing without color quantization,
       * I have no work to do; just call the upsampler directly.
       */
      post->pub.post__process__data = cinfo->upsample->upsample;
    }
    break;
#ifdef QUANT__2PASS__SUPPORTED
  case JBUF__SAVE__AND__PASS:
    /* First pass of 2-pass quantization */
    if (post->whole__image == NULL)
      ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    post->pub.post__process__data = post__process__prepass;
    break;
  case JBUF__CRANK__DEST:
    /* Second pass of 2-pass quantization */
    if (post->whole__image == NULL)
      ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    post->pub.post__process__data = post__process__2pass;
    break;
#endif /* QUANT__2PASS__SUPPORTED */
  default:
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    break;
  }
  post->starting__row = post->next__row = 0;
}


/*
 * Process some data in the one-pass (strip buffer) case.
 * This is used for color precision reduction as well as one-pass quantization.
 */

METHODDEF(void)
post__process__1pass (j__decompress__ptr cinfo,
		    JSAMPIMAGE input__buf, JDIMENSION *in__row__group__ctr,
		    JDIMENSION in__row__groups__avail,
		    JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
		    JDIMENSION out__rows__avail)
{
  my__post__ptr post = (my__post__ptr) cinfo->post;
  JDIMENSION num__rows, max__rows;

  /* Fill the buffer, but not more than what we can dump out in one go. */
  /* Note we rely on the upsampler to detect bottom of image. */
  max__rows = out__rows__avail - *out__row__ctr;
  if (max__rows > post->strip__height)
    max__rows = post->strip__height;
  num__rows = 0;
  (*cinfo->upsample->upsample) (cinfo,
		input__buf, in__row__group__ctr, in__row__groups__avail,
		post->buffer, &num__rows, max__rows);
  /* Quantize and emit data. */
  (*cinfo->cquantize->color__quantize) (cinfo,
		post->buffer, output__buf + *out__row__ctr, (int) num__rows);
  *out__row__ctr += num__rows;
}


#ifdef QUANT__2PASS__SUPPORTED

/*
 * Process some data in the first pass of 2-pass quantization.
 */

METHODDEF(void)
post__process__prepass (j__decompress__ptr cinfo,
		      JSAMPIMAGE input__buf, JDIMENSION *in__row__group__ctr,
		      JDIMENSION in__row__groups__avail,
		      JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
		      JDIMENSION out__rows__avail)
{
  my__post__ptr post = (my__post__ptr) cinfo->post;
  JDIMENSION old__next__row, num__rows;

  /* Reposition virtual buffer if at start of strip. */
  if (post->next__row == 0) {
    post->buffer = (*cinfo->mem->access__virt__sarray)
	((j__common__ptr) cinfo, post->whole__image,
	 post->starting__row, post->strip__height, TRUE);
  }

  /* Upsample some data (up to a strip height's worth). */
  old__next__row = post->next__row;
  (*cinfo->upsample->upsample) (cinfo,
		input__buf, in__row__group__ctr, in__row__groups__avail,
		post->buffer, &post->next__row, post->strip__height);

  /* Allow quantizer to scan new data.  No data is emitted, */
  /* but we advance out__row__ctr so outer loop can tell when we're done. */
  if (post->next__row > old__next__row) {
    num__rows = post->next__row - old__next__row;
    (*cinfo->cquantize->color__quantize) (cinfo, post->buffer + old__next__row,
					 (JSAMPARRAY) NULL, (int) num__rows);
    *out__row__ctr += num__rows;
  }

  /* Advance if we filled the strip. */
  if (post->next__row >= post->strip__height) {
    post->starting__row += post->strip__height;
    post->next__row = 0;
  }
}


/*
 * Process some data in the second pass of 2-pass quantization.
 */

METHODDEF(void)
post__process__2pass (j__decompress__ptr cinfo,
		    JSAMPIMAGE input__buf, JDIMENSION *in__row__group__ctr,
		    JDIMENSION in__row__groups__avail,
		    JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
		    JDIMENSION out__rows__avail)
{
  my__post__ptr post = (my__post__ptr) cinfo->post;
  JDIMENSION num__rows, max__rows;

  /* Reposition virtual buffer if at start of strip. */
  if (post->next__row == 0) {
    post->buffer = (*cinfo->mem->access__virt__sarray)
	((j__common__ptr) cinfo, post->whole__image,
	 post->starting__row, post->strip__height, FALSE);
  }

  /* Determine number of rows to emit. */
  num__rows = post->strip__height - post->next__row; /* available in strip */
  max__rows = out__rows__avail - *out__row__ctr; /* available in output area */
  if (num__rows > max__rows)
    num__rows = max__rows;
  /* We have to check bottom of image here, can't depend on upsampler. */
  max__rows = cinfo->output__height - post->starting__row;
  if (num__rows > max__rows)
    num__rows = max__rows;

  /* Quantize and emit data. */
  (*cinfo->cquantize->color__quantize) (cinfo,
		post->buffer + post->next__row, output__buf + *out__row__ctr,
		(int) num__rows);
  *out__row__ctr += num__rows;

  /* Advance if we filled the strip. */
  post->next__row += num__rows;
  if (post->next__row >= post->strip__height) {
    post->starting__row += post->strip__height;
    post->next__row = 0;
  }
}

#endif /* QUANT__2PASS__SUPPORTED */


/*
 * Initialize postprocessing controller.
 */

GLOBAL(void)
jinit__d__post__controller (j__decompress__ptr cinfo, CH_BOOL need__full__buffer)
{
  my__post__ptr post;

  post = (my__post__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__post__controller));
  cinfo->post = (struct jpeg__d__post__controller *) post;
  post->pub.start__pass = start__pass__dpost;
  post->whole__image = NULL;	/* flag for no virtual arrays */
  post->buffer = NULL;		/* flag for no strip buffer */

  /* Create the quantization buffer, if needed */
  if (cinfo->quantize__colors) {
    /* The buffer strip height is max__v__samp__factor, which is typically
     * an efficient number of rows for upsampling to return.
     * (In the presence of output rescaling, we might want to be smarter?)
     */
    post->strip__height = (JDIMENSION) cinfo->max__v__samp__factor;
    if (need__full__buffer) {
      /* Two-pass color quantization: need full-image storage. */
      /* We round up the number of rows to a multiple of the strip height. */
#ifdef QUANT__2PASS__SUPPORTED
      post->whole__image = (*cinfo->mem->request__virt__sarray)
	((j__common__ptr) cinfo, JPOOL__IMAGE, FALSE,
	 cinfo->output__width * cinfo->out__color__components,
	 (JDIMENSION) jround__up((long) cinfo->output__height,
				(long) post->strip__height),
	 post->strip__height);
#else
      ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
#endif /* QUANT__2PASS__SUPPORTED */
    } else {
      /* One-pass color quantization: just make a strip buffer. */
      post->buffer = (*cinfo->mem->alloc__sarray)
	((j__common__ptr) cinfo, JPOOL__IMAGE,
	 cinfo->output__width * cinfo->out__color__components,
	 post->strip__height);
    }
  }
}
