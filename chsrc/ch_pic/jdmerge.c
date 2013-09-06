/*
 * jdmerge.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains code for merged upsampling/color conversion.
 *
 * This file combines functions from jdsample.c and jdcolor.c;
 * read those files first to understand what's going on.
 *
 * When the chroma components are to be upsampled by simple replication
 * (ie, box filtering), we can save some work in color conversion by
 * calculating all the output pixels corresponding to a pair of chroma
 * samples at one time.  In the conversion equations
 *	R = Y           + K1 * Cr
 *	G = Y + K2 * Cb + K3 * Cr
 *	B = Y + K4 * Cb
 * only the Y term varies among the group of pixels corresponding to a pair
 * of chroma samples, so the rest of the terms can be calculated just once.
 * At typical sampling ratios, this eliminates half or three-quarters of the
 * multiplications needed for color conversion.
 *
 * This file currently provides implementations for the following cases:
 *	YCbCr => RGB color conversion only.
 *	Sampling ratios of 2h1v or 2h2v.
 *	No scaling needed at upsample time.
 *	Corner-aligned (non-CCIR601) sampling alignment.
 * Other special cases could be added, but in most applications these are
 * the only common cases.  (For uncommon cases we fall back on the more
 * general code in jdsample.c and jdcolor.c.)
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#ifdef UPSAMPLE__MERGING__SUPPORTED


/* Private subobject */

typedef struct {
  struct jpeg__upsampler pub;	/* public fields */

  /* Pointer to routine to do actual upsampling/conversion of one row group */
  JMETHOD(void, upmethod, (j__decompress__ptr cinfo,
			   JSAMPIMAGE input__buf, JDIMENSION in__row__group__ctr,
			   JSAMPARRAY output__buf));

  /* Private state for YCC->RGB conversion */
  int * Cr__r__tab;		/* => table for Cr to R conversion */
  int * Cb__b__tab;		/* => table for Cb to B conversion */
  INT32 * Cr__g__tab;		/* => table for Cr to G conversion */
  INT32 * Cb__g__tab;		/* => table for Cb to G conversion */

  /* For 2:1 vertical sampling, we produce two output rows at a time.
   * We need a "spare" row buffer to hold the second output row if the
   * application provides just a one-row buffer; we also use the spare
   * to discard the dummy last row if the image height is odd.
   */
  JSAMPROW spare__row;
  CH_BOOL spare__full;		/* T if spare buffer is occupied */

  JDIMENSION out__row__width;	/* samples per output row */
  JDIMENSION rows__to__go;	/* counts rows remaining in image */
} my__upsampler;

typedef my__upsampler * my__upsample__ptr;

#define SCALEBITS	16	/* speediest right-shift on some machines */
#define ONE__HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))


/*
 * Initialize tables for YCC->RGB colorspace conversion.
 * This is taken directly from jdcolor.c; see that file for more info.
 */

LOCAL(void)
build__ycc__rgb__table (j__decompress__ptr cinfo)
{
  my__upsample__ptr upsample = (my__upsample__ptr) cinfo->upsample;
  int i;
  INT32 x;
  SHIFT__TEMPS

  upsample->Cr__r__tab = (int *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  upsample->Cb__b__tab = (int *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  upsample->Cr__g__tab = (INT32 *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));
  upsample->Cb__g__tab = (INT32 *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));

  for (i = 0, x = -CENTERJSAMPLE; i <= MAXJSAMPLE; i++, x++) {
    /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
    /* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
    /* Cr=>R value is nearest int to 1.40200 * x */
    upsample->Cr__r__tab[i] = (int)
		    RIGHT__SHIFT(FIX(1.40200) * x + ONE__HALF, SCALEBITS);
    /* Cb=>B value is nearest int to 1.77200 * x */
    upsample->Cb__b__tab[i] = (int)
		    RIGHT__SHIFT(FIX(1.77200) * x + ONE__HALF, SCALEBITS);
    /* Cr=>G value is scaled-up -0.71414 * x */
    upsample->Cr__g__tab[i] = (- FIX(0.71414)) * x;
    /* Cb=>G value is scaled-up -0.34414 * x */
    /* We also add in ONE__HALF so that need not do it in inner loop */
    upsample->Cb__g__tab[i] = (- FIX(0.34414)) * x + ONE__HALF;
  }
}


/*
 * Initialize for an upsampling pass.
 */

METHODDEF(void)
start__pass__merged__upsample (j__decompress__ptr cinfo)
{
  my__upsample__ptr upsample = (my__upsample__ptr) cinfo->upsample;

  /* Mark the spare buffer empty */
  upsample->spare__full = FALSE;
  /* Initialize total-height counter for detecting bottom of image */
  upsample->rows__to__go = cinfo->output__height;
}


/*
 * Control routine to do upsampling (and color conversion).
 *
 * The control routine just handles the row buffering considerations.
 */

METHODDEF(void)
merged__2v__upsample (j__decompress__ptr cinfo,
		    JSAMPIMAGE input__buf, JDIMENSION *in__row__group__ctr,
		    JDIMENSION in__row__groups__avail,
		    JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
		    JDIMENSION out__rows__avail)
/* 2:1 vertical sampling case: may need a spare row. */
{
  my__upsample__ptr upsample = (my__upsample__ptr) cinfo->upsample;
  JSAMPROW work__ptrs[2];
  JDIMENSION num__rows;		/* number of rows returned to caller */

  if (upsample->spare__full) {
    /* If we have a spare row saved from a previous cycle, just return it. */
    jcopy__sample__rows(& upsample->spare__row, 0, output__buf + *out__row__ctr, 0,
		      1, upsample->out__row__width);
    num__rows = 1;
    upsample->spare__full = FALSE;
  } else {
    /* Figure number of rows to return to caller. */
    num__rows = 2;
    /* Not more than the distance to the end of the image. */
    if (num__rows > upsample->rows__to__go)
      num__rows = upsample->rows__to__go;
    /* And not more than what the client can accept: */
    out__rows__avail -= *out__row__ctr;
    if (num__rows > out__rows__avail)
      num__rows = out__rows__avail;
    /* Create output pointer array for upsampler. */
    work__ptrs[0] = output__buf[*out__row__ctr];
    if (num__rows > 1) {
      work__ptrs[1] = output__buf[*out__row__ctr + 1];
    } else {
      work__ptrs[1] = upsample->spare__row;
      upsample->spare__full = TRUE;
    }
    /* Now do the upsampling. */
    (*upsample->upmethod) (cinfo, input__buf, *in__row__group__ctr, work__ptrs);
  }

  /* Adjust counts */
  *out__row__ctr += num__rows;
  upsample->rows__to__go -= num__rows;
  /* When the buffer is emptied, declare this input row group consumed */
  if (! upsample->spare__full)
    (*in__row__group__ctr)++;
}


METHODDEF(void)
merged__1v__upsample (j__decompress__ptr cinfo,
		    JSAMPIMAGE input__buf, JDIMENSION *in__row__group__ctr,
		    JDIMENSION in__row__groups__avail,
		    JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
		    JDIMENSION out__rows__avail)
/* 1:1 vertical sampling case: much easier, never need a spare row. */
{
  my__upsample__ptr upsample = (my__upsample__ptr) cinfo->upsample;

  /* Just do the upsampling. */
  (*upsample->upmethod) (cinfo, input__buf, *in__row__group__ctr,
			 output__buf + *out__row__ctr);
  /* Adjust counts */
  (*out__row__ctr)++;
  (*in__row__group__ctr)++;
}


/*
 * These are the routines invoked by the control routines to do
 * the actual upsampling/conversion.  One row group is processed per call.
 *
 * Note: since we may be writing directly into application-supplied buffers,
 * we have to be honest about the output width; we can't assume the buffer
 * has been rounded up to an even width.
 */


/*
 * Upsample and color convert for the case of 2:1 horizontal and 1:1 vertical.
 */

METHODDEF(void)
h2v1__merged__upsample (j__decompress__ptr cinfo,
		      JSAMPIMAGE input__buf, JDIMENSION in__row__group__ctr,
		      JSAMPARRAY output__buf)
{
  my__upsample__ptr upsample = (my__upsample__ptr) cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr;
  JSAMPROW inptr0, inptr1, inptr2;
  JDIMENSION col;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range__limit = cinfo->sample__range__limit;
  int * Crrtab = upsample->Cr__r__tab;
  int * Cbbtab = upsample->Cb__b__tab;
  INT32 * Crgtab = upsample->Cr__g__tab;
  INT32 * Cbgtab = upsample->Cb__g__tab;
  SHIFT__TEMPS

  inptr0 = input__buf[0][in__row__group__ctr];
  inptr1 = input__buf[1][in__row__group__ctr];
  inptr2 = input__buf[2][in__row__group__ctr];
  outptr = output__buf[0];
  /* Loop for each pair of output pixels */
  for (col = cinfo->output__width >> 1; col > 0; col--) {
    /* Do the chroma part of the calculation */
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT__SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    /* Fetch 2 Y values and emit 2 pixels */
    y  = GETJSAMPLE(*inptr0++);
    outptr[RGB__RED] =   range__limit[y + cred];
    outptr[RGB__GREEN] = range__limit[y + cgreen];
    outptr[RGB__BLUE] =  range__limit[y + cblue];
    outptr += RGB__PIXELSIZE;
    y  = GETJSAMPLE(*inptr0++);
    outptr[RGB__RED] =   range__limit[y + cred];
    outptr[RGB__GREEN] = range__limit[y + cgreen];
    outptr[RGB__BLUE] =  range__limit[y + cblue];
    outptr += RGB__PIXELSIZE;
  }
  /* If image width is odd, do the last output column separately */
  if (cinfo->output__width & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT__SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    y  = GETJSAMPLE(*inptr0);
    outptr[RGB__RED] =   range__limit[y + cred];
    outptr[RGB__GREEN] = range__limit[y + cgreen];
    outptr[RGB__BLUE] =  range__limit[y + cblue];
  }
}


/*
 * Upsample and color convert for the case of 2:1 horizontal and 2:1 vertical.
 */

METHODDEF(void)
h2v2__merged__upsample (j__decompress__ptr cinfo,
		      JSAMPIMAGE input__buf, JDIMENSION in__row__group__ctr,
		      JSAMPARRAY output__buf)
{
  my__upsample__ptr upsample = (my__upsample__ptr) cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr0, outptr1;
  JSAMPROW inptr00, inptr01, inptr1, inptr2;
  JDIMENSION col;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range__limit = cinfo->sample__range__limit;
  int * Crrtab = upsample->Cr__r__tab;
  int * Cbbtab = upsample->Cb__b__tab;
  INT32 * Crgtab = upsample->Cr__g__tab;
  INT32 * Cbgtab = upsample->Cb__g__tab;
  SHIFT__TEMPS

  inptr00 = input__buf[0][in__row__group__ctr*2];
  inptr01 = input__buf[0][in__row__group__ctr*2 + 1];
  inptr1 = input__buf[1][in__row__group__ctr];
  inptr2 = input__buf[2][in__row__group__ctr];
  outptr0 = output__buf[0];
  outptr1 = output__buf[1];
  /* Loop for each group of output pixels */
  for (col = cinfo->output__width >> 1; col > 0; col--) {
    /* Do the chroma part of the calculation */
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT__SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    /* Fetch 4 Y values and emit 4 pixels */
    y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB__RED] =   range__limit[y + cred];
    outptr0[RGB__GREEN] = range__limit[y + cgreen];
    outptr0[RGB__BLUE] =  range__limit[y + cblue];
    outptr0 += RGB__PIXELSIZE;
    y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB__RED] =   range__limit[y + cred];
    outptr0[RGB__GREEN] = range__limit[y + cgreen];
    outptr0[RGB__BLUE] =  range__limit[y + cblue];
    outptr0 += RGB__PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB__RED] =   range__limit[y + cred];
    outptr1[RGB__GREEN] = range__limit[y + cgreen];
    outptr1[RGB__BLUE] =  range__limit[y + cblue];
    outptr1 += RGB__PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB__RED] =   range__limit[y + cred];
    outptr1[RGB__GREEN] = range__limit[y + cgreen];
    outptr1[RGB__BLUE] =  range__limit[y + cblue];
    outptr1 += RGB__PIXELSIZE;
  }
  /* If image width is odd, do the last output column separately */
  if (cinfo->output__width & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT__SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    y  = GETJSAMPLE(*inptr00);
    outptr0[RGB__RED] =   range__limit[y + cred];
    outptr0[RGB__GREEN] = range__limit[y + cgreen];
    outptr0[RGB__BLUE] =  range__limit[y + cblue];
    y  = GETJSAMPLE(*inptr01);
    outptr1[RGB__RED] =   range__limit[y + cred];
    outptr1[RGB__GREEN] = range__limit[y + cgreen];
    outptr1[RGB__BLUE] =  range__limit[y + cblue];
  }
}


/*
 * Module initialization routine for merged upsampling/color conversion.
 *
 * NB: this is called under the conditions determined by use__merged__upsample()
 * in jdmaster.c.  That routine MUST correspond to the actual capabilities
 * of this module; no safety checks are made here.
 */

GLOBAL(void)
jinit__merged__upsampler (j__decompress__ptr cinfo)
{
  my__upsample__ptr upsample;

  upsample = (my__upsample__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__upsampler));
  cinfo->upsample = (struct jpeg__upsampler *) upsample;
  upsample->pub.start__pass = start__pass__merged__upsample;
  upsample->pub.need__context__rows = FALSE;

  upsample->out__row__width = cinfo->output__width * cinfo->out__color__components;

  if (cinfo->max__v__samp__factor == 2) {
    upsample->pub.upsample = merged__2v__upsample;
    upsample->upmethod = h2v2__merged__upsample;
    /* Allocate a spare row buffer */
    upsample->spare__row = (JSAMPROW)
      (*cinfo->mem->alloc__large) ((j__common__ptr) cinfo, JPOOL__IMAGE,
		(size_t) (upsample->out__row__width * SIZEOF(JSAMPLE)));
  } else {
    upsample->pub.upsample = merged__1v__upsample;
    upsample->upmethod = h2v1__merged__upsample;
    /* No spare row needed */
    upsample->spare__row = NULL;
  }

  build__ycc__rgb__table(cinfo);
}

#endif /* UPSAMPLE__MERGING__SUPPORTED */
