/*
 * jdmaster.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains master control logic for the JPEG decompressor.
 * These routines are concerned with selecting the modules to be executed
 * and with determining the number of passes and the work to be done in each
 * pass.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Private state */

typedef struct {
  struct jpeg__decomp__master pub; /* public fields */

  int pass__number;		/* # of passes completed */

  CH_BOOL using__merged__upsample; /* TRUE if using merged upsample/cconvert */

  /* Saved references to initialized quantizer modules,
   * in case we need to switch modes.
   */
  struct jpeg__color__quantizer * quantizer__1pass;
  struct jpeg__color__quantizer * quantizer__2pass;
} my__decomp__master;

typedef my__decomp__master * my__master__ptr;


/*
 * Determine whether merged upsample/color conversion should be used.
 * CRUCIAL: this must match the actual capabilities of jdmerge.c!
 */

LOCAL(CH_BOOL)
use__merged__upsample (j__decompress__ptr cinfo)
{
#ifdef UPSAMPLE__MERGING__SUPPORTED
  /* Merging is the equivalent of plain box-filter upsampling */
  if (cinfo->do__fancy__upsampling || cinfo->CCIR601__sampling)
    return FALSE;
  /* jdmerge.c only supports YCC=>RGB color conversion */
  if (cinfo->jpeg__color__space != JCS__YCbCr || cinfo->num__components != 3 ||
      cinfo->out__color__space != JCS__RGB ||
      cinfo->out__color__components != RGB__PIXELSIZE)
    return FALSE;
  /* and it only handles 2h1v or 2h2v sampling ratios */
  if (cinfo->comp__info[0].h__samp__factor != 2 ||
      cinfo->comp__info[1].h__samp__factor != 1 ||
      cinfo->comp__info[2].h__samp__factor != 1 ||
      cinfo->comp__info[0].v__samp__factor >  2 ||
      cinfo->comp__info[1].v__samp__factor != 1 ||
      cinfo->comp__info[2].v__samp__factor != 1)
    return FALSE;
  /* furthermore, it doesn't work if we've scaled the IDCTs differently */
  if (cinfo->comp__info[0].DCT__scaled__size != cinfo->min__DCT__scaled__size ||
      cinfo->comp__info[1].DCT__scaled__size != cinfo->min__DCT__scaled__size ||
      cinfo->comp__info[2].DCT__scaled__size != cinfo->min__DCT__scaled__size)
    return FALSE;
  /* ??? also need to test for upsample-time rescaling, when & if supported */
  return TRUE;			/* by golly, it'll work... */
#else
  return FALSE;
#endif
}


/*
 * Compute output image dimensions and related values.
 * NOTE: this is exported for possible use by application.
 * Hence it mustn't do anything that can't be done twice.
 * Also note that it may be called before the master module is initialized!
 */

GLOBAL(void)
jpeg__calc__output__dimensions (j__decompress__ptr cinfo)
/* Do computations that are needed before master selection phase */
{
#ifdef IDCT__SCALING__SUPPORTED
  int ci;
  jpeg__component__info *compptr;
#endif

  /* Prevent application from calling me at wrong times */
  if (cinfo->global__state != DSTATE__READY)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

#ifdef IDCT__SCALING__SUPPORTED

  /* Compute actual output image dimensions and DCT scaling choices. */
  if (cinfo->scale__num * 8 <= cinfo->scale__denom) {
    /* Provide 1/8 scaling */
    cinfo->output__width = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__width, 8L);
    cinfo->output__height = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__height, 8L);
    cinfo->min__DCT__scaled__size = 1;
  } else if (cinfo->scale__num * 4 <= cinfo->scale__denom) {
    /* Provide 1/4 scaling */
    cinfo->output__width = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__width, 4L);
    cinfo->output__height = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__height, 4L);
    cinfo->min__DCT__scaled__size = 2;
  } else if (cinfo->scale__num * 2 <= cinfo->scale__denom) {
    /* Provide 1/2 scaling */
    cinfo->output__width = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__width, 2L);
    cinfo->output__height = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__height, 2L);
    cinfo->min__DCT__scaled__size = 4;
  } else {
    /* Provide 1/1 scaling */
    cinfo->output__width = cinfo->image__width;
    cinfo->output__height = cinfo->image__height;
    cinfo->min__DCT__scaled__size = DCTSIZE;
  }
  /* In selecting the actual DCT scaling for each component, we try to
   * scale up the chroma components via IDCT scaling rather than upsampling.
   * This saves time if the upsampler gets to use 1:1 scaling.
   * Note this code assumes that the supported DCT scalings are powers of 2.
   */
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    int ssize = cinfo->min__DCT__scaled__size;
    while (ssize < DCTSIZE &&
	   (compptr->h__samp__factor * ssize * 2 <=
	    cinfo->max__h__samp__factor * cinfo->min__DCT__scaled__size) &&
	   (compptr->v__samp__factor * ssize * 2 <=
	    cinfo->max__v__samp__factor * cinfo->min__DCT__scaled__size)) {
      ssize = ssize * 2;
    }
    compptr->DCT__scaled__size = ssize;
  }

  /* Recompute downsampled dimensions of components;
   * application needs to know these if using raw downsampled data.
   */
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    /* Size in samples, after IDCT scaling */
    compptr->downsampled__width = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__width *
		    (long) (compptr->h__samp__factor * compptr->DCT__scaled__size),
		    (long) (cinfo->max__h__samp__factor * DCTSIZE));
    compptr->downsampled__height = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__height *
		    (long) (compptr->v__samp__factor * compptr->DCT__scaled__size),
		    (long) (cinfo->max__v__samp__factor * DCTSIZE));
  }

#else /* !IDCT__SCALING__SUPPORTED */

  /* Hardwire it to "no scaling" */
  cinfo->output__width = cinfo->image__width;
  cinfo->output__height = cinfo->image__height;
  /* jdinput.c has already initialized DCT__scaled__size to DCTSIZE,
   * and has computed unscaled downsampled__width and downsampled__height.
   */

#endif /* IDCT__SCALING__SUPPORTED */

  /* Report number of components in selected colorspace. */
  /* Probably this should be in the color conversion module... */
  switch (cinfo->out__color__space) {
  case JCS__GRAYSCALE:
    cinfo->out__color__components = 1;
    break;
  case JCS__RGB:
#if RGB__PIXELSIZE != 3
    cinfo->out__color__components = RGB__PIXELSIZE;
    break;
#endif /* else share code with YCbCr */
  case JCS__YCbCr:
    cinfo->out__color__components = 3;
    break;
  case JCS__CMYK:
  case JCS__YCCK:
    cinfo->out__color__components = 4;
    break;
  default:			/* else must be same colorspace as in file */
    cinfo->out__color__components = cinfo->num__components;
    break;
  }
  cinfo->output__components = (cinfo->quantize__colors ? 1 :
			      cinfo->out__color__components);

  /* See if upsampler will want to emit more than one row at a time */
  if (use__merged__upsample(cinfo))
    cinfo->rec__outbuf__height = cinfo->max__v__samp__factor;
  else
    cinfo->rec__outbuf__height = 1;
}


/*
 * Several decompression processes need to range-limit values to the range
 * 0..MAXJSAMPLE; the input value may fall somewhat outside this range
 * due to noise introduced by quantization, roundoff error, etc.  These
 * processes are inner loops and need to be as fast as possible.  On most
 * machines, particularly CPUs with pipelines or instruction prefetch,
 * a (subscript-check-less) C table lookup
 *		x = sample__range__limit[x];
 * is faster than explicit tests
 *		if (x < 0)  x = 0;
 *		else if (x > MAXJSAMPLE)  x = MAXJSAMPLE;
 * These processes all use a common table prepared by the routine below.
 *
 * For most steps we can mathematically guarantee that the initial value
 * of x is within MAXJSAMPLE+1 of the legal range, so a table running from
 * -(MAXJSAMPLE+1) to 2*MAXJSAMPLE+1 is sufficient.  But for the initial
 * limiting step (just after the IDCT), a wildly out-of-range value is 
 * possible if the input data is corrupt.  To avoid any chance of indexing
 * off the end of memory and getting a bad-pointer trap, we perform the
 * post-IDCT limiting thus:
 *		x = range__limit[x & MASK];
 * where MASK is 2 bits wider than legal sample data, ie 10 bits for 8-bit
 * samples.  Under normal circumstances this is more than enough range and
 * a correct output will be generated; with bogus input data the mask will
 * cause wraparound, and we will safely generate a bogus-but-in-range output.
 * For the post-IDCT step, we want to convert the data from signed to unsigned
 * representation by adding CENTERJSAMPLE at the same time that we limit it.
 * So the post-IDCT limiting table ends up looking like this:
 *   CENTERJSAMPLE,CENTERJSAMPLE+1,...,MAXJSAMPLE,
 *   MAXJSAMPLE (repeat 2*(MAXJSAMPLE+1)-CENTERJSAMPLE times),
 *   0          (repeat 2*(MAXJSAMPLE+1)-CENTERJSAMPLE times),
 *   0,1,...,CENTERJSAMPLE-1
 * Negative inputs select values from the upper half of the table after
 * masking.
 *
 * We can save some space by overlapping the start of the post-IDCT table
 * with the simpler range limiting table.  The post-IDCT table begins at
 * sample__range__limit + CENTERJSAMPLE.
 *
 * Note that the table is allocated in near data space on PCs; it's small
 * enough and used often enough to justify this.
 */

LOCAL(void)
prepare__range__limit__table (j__decompress__ptr cinfo)
/* Allocate and fill in the sample__range__limit table */
{
  JSAMPLE * table;
  int i;

  table = (JSAMPLE *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
		(5 * (MAXJSAMPLE+1) + CENTERJSAMPLE) * SIZEOF(JSAMPLE));
  table += (MAXJSAMPLE+1);	/* allow negative subscripts of simple table */
  cinfo->sample__range__limit = table;
  /* First segment of "simple" table: limit[x] = 0 for x < 0 */
  MEMZERO(table - (MAXJSAMPLE+1), (MAXJSAMPLE+1) * SIZEOF(JSAMPLE));
  /* Main part of "simple" table: limit[x] = x */
  for (i = 0; i <= MAXJSAMPLE; i++)
    table[i] = (JSAMPLE) i;
  table += CENTERJSAMPLE;	/* Point to where post-IDCT table starts */
  /* End of simple table, rest of first half of post-IDCT table */
  for (i = CENTERJSAMPLE; i < 2*(MAXJSAMPLE+1); i++)
    table[i] = (char)MAXJSAMPLE;
  /* Second half of post-IDCT table */
  MEMZERO(table + (2 * (MAXJSAMPLE+1)),
	  (2 * (MAXJSAMPLE+1) - CENTERJSAMPLE) * SIZEOF(JSAMPLE));
  MEMCOPY(table + (4 * (MAXJSAMPLE+1) - CENTERJSAMPLE),
	  cinfo->sample__range__limit, CENTERJSAMPLE * SIZEOF(JSAMPLE));
}


/*
 * Master selection of decompression modules.
 * This is done once at jpeg__start__decompress time.  We determine
 * which modules will be used and give them appropriate initialization calls.
 * We also initialize the decompressor input side to begin consuming data.
 *
 * Since jpeg__read__header has finished, we know what is in the SOF
 * and (first) SOS markers.  We also have all the application parameter
 * settings.
 */

LOCAL(void)
master__selection (j__decompress__ptr cinfo)
{
  my__master__ptr master = (my__master__ptr) cinfo->master;
  CH_BOOL use__c__buffer;
  long samplesperrow;
  JDIMENSION jd__samplesperrow;

  /* Initialize dimensions and other stuff */
  jpeg__calc__output__dimensions(cinfo);
  prepare__range__limit__table(cinfo);

  /* Width of an output scanline must be representable as JDIMENSION. */
  samplesperrow = (long) cinfo->output__width * (long) cinfo->out__color__components;
  jd__samplesperrow = (JDIMENSION) samplesperrow;
  if ((long) jd__samplesperrow != samplesperrow)
    ERREXIT(cinfo, JERR__WIDTH__OVERFLOW);

  /* Initialize my private state */
  master->pass__number = 0;
  master->using__merged__upsample = use__merged__upsample(cinfo);

  /* Color quantizer selection */
  master->quantizer__1pass = NULL;
  master->quantizer__2pass = NULL;
  /* No mode changes if not using buffered-image mode. */
  if (! cinfo->quantize__colors || ! cinfo->buffered__image) {
    cinfo->enable__1pass__quant = FALSE;
    cinfo->enable__external__quant = FALSE;
    cinfo->enable__2pass__quant = FALSE;
  }
  if (cinfo->quantize__colors) {
    if (cinfo->raw__data__out)
      ERREXIT(cinfo, JERR__NOTIMPL);
    /* 2-pass quantizer only works in 3-component color space. */
    if (cinfo->out__color__components != 3) {
      cinfo->enable__1pass__quant = TRUE;
      cinfo->enable__external__quant = FALSE;
      cinfo->enable__2pass__quant = FALSE;
      cinfo->colormap = NULL;
    } else if (cinfo->colormap != NULL) {
      cinfo->enable__external__quant = TRUE;
    } else if (cinfo->two__pass__quantize) {
      cinfo->enable__2pass__quant = TRUE;
    } else {
      cinfo->enable__1pass__quant = TRUE;
    }

    if (cinfo->enable__1pass__quant) {
#ifdef QUANT__1PASS__SUPPORTED
      jinit__1pass__quantizer(cinfo);
      master->quantizer__1pass = cinfo->cquantize;
#else
      ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
    }

    /* We use the 2-pass code to map to external colormaps. */
    if (cinfo->enable__2pass__quant || cinfo->enable__external__quant) {
#ifdef QUANT__2PASS__SUPPORTED
      jinit__2pass__quantizer(cinfo);
      master->quantizer__2pass = cinfo->cquantize;
#else
      ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
    }
    /* If both quantizers are initialized, the 2-pass one is left active;
     * this is necessary for starting with quantization to an external map.
     */
  }

  /* Post-processing: in particular, color conversion first */
  if (! cinfo->raw__data__out) {
    if (master->using__merged__upsample) {
#ifdef UPSAMPLE__MERGING__SUPPORTED
      jinit__merged__upsampler(cinfo); /* does color conversion too */
#else
      ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
    } else {
      jinit__color__deconverter(cinfo);
      jinit__upsampler(cinfo);
    }
    jinit__d__post__controller(cinfo, cinfo->enable__2pass__quant);
  }
  /* Inverse DCT */
  jinit__inverse__dct(cinfo);
  /* Entropy decoding: either Huffman or arithmetic coding. */
  if (cinfo->arith__code) {
    ERREXIT(cinfo, JERR__ARITH__NOTIMPL);
  } else {
    if (cinfo->progressive__mode) {
#ifdef D__PROGRESSIVE__SUPPORTED
      jinit__phuff__decoder(cinfo);
#else
      ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
    } else
      jinit__huff__decoder(cinfo);
  }

  /* Initialize principal buffer controllers. */
  use__c__buffer = cinfo->inputctl->has__multiple__scans || cinfo->buffered__image;
  jinit__d__coef__controller(cinfo, use__c__buffer);

  if (! cinfo->raw__data__out)
    jinit__d__main__controller(cinfo, FALSE /* never need full buffer here */);

  /* We can now tell the memory manager to allocate virtual arrays. */
  (*cinfo->mem->realize__virt__arrays) ((j__common__ptr) cinfo);

  /* Initialize input side of decompressor to consume first scan. */
  (*cinfo->inputctl->start__input__pass) (cinfo);

#ifdef D__MULTISCAN__FILES__SUPPORTED
  /* If jpeg__start__decompress will read the whole file, initialize
   * progress monitoring appropriately.  The input step is counted
   * as one pass.
   */
  if (cinfo->progress != NULL && ! cinfo->buffered__image &&
      cinfo->inputctl->has__multiple__scans) {
    int nscans;
    /* Estimate number of scans to set pass__limit. */
    if (cinfo->progressive__mode) {
      /* Arbitrarily estimate 2 interleaved DC scans + 3 AC scans/component. */
      nscans = 2 + 3 * cinfo->num__components;
    } else {
      /* For a nonprogressive multiscan file, estimate 1 scan per component. */
      nscans = cinfo->num__components;
    }
    cinfo->progress->pass__counter = 0L;
    cinfo->progress->pass__limit = (long) cinfo->total__iMCU__rows * nscans;
    cinfo->progress->completed__passes = 0;
    cinfo->progress->total__passes = (cinfo->enable__2pass__quant ? 3 : 2);
    /* Count the input pass as done */
    master->pass__number++;
  }
#endif /* D__MULTISCAN__FILES__SUPPORTED */
}


/*
 * Per-pass setup.
 * This is called at the beginning of each output pass.  We determine which
 * modules will be active during this pass and give them appropriate
 * start__pass calls.  We also set is__dummy__pass to indicate whether this
 * is a "real" output pass or a dummy pass for color quantization.
 * (In the latter case, jdapistd.c will crank the pass to completion.)
 */

METHODDEF(void)
prepare__for__output__pass (j__decompress__ptr cinfo)
{
  my__master__ptr master = (my__master__ptr) cinfo->master;

  if (master->pub.is__dummy__pass) {
#ifdef QUANT__2PASS__SUPPORTED
    /* Final pass of 2-pass quantization */
    master->pub.is__dummy__pass = FALSE;
    (*cinfo->cquantize->start__pass) (cinfo, FALSE);
    (*cinfo->post->start__pass) (cinfo, JBUF__CRANK__DEST);
    (*cinfo->main->start__pass) (cinfo, JBUF__CRANK__DEST);
#else
    ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif /* QUANT__2PASS__SUPPORTED */
  } else {
    if (cinfo->quantize__colors && cinfo->colormap == NULL) {
      /* Select new quantization method */
      if (cinfo->two__pass__quantize && cinfo->enable__2pass__quant) {
	cinfo->cquantize = master->quantizer__2pass;
	master->pub.is__dummy__pass = TRUE;
      } else if (cinfo->enable__1pass__quant) {
	cinfo->cquantize = master->quantizer__1pass;
      } else {
	ERREXIT(cinfo, JERR__MODE__CHANGE);
      }
    }
    (*cinfo->idct->start__pass) (cinfo);
    (*cinfo->coef->start__output__pass) (cinfo);
    if (! cinfo->raw__data__out) {
      if (! master->using__merged__upsample)
	(*cinfo->cconvert->start__pass) (cinfo);
      (*cinfo->upsample->start__pass) (cinfo);
      if (cinfo->quantize__colors)
	(*cinfo->cquantize->start__pass) (cinfo, master->pub.is__dummy__pass);
      (*cinfo->post->start__pass) (cinfo,
	    (master->pub.is__dummy__pass ? JBUF__SAVE__AND__PASS : JBUF__PASS__THRU));
      (*cinfo->main->start__pass) (cinfo, JBUF__PASS__THRU);
    }
  }

  /* Set up progress monitor's pass info if present */
  if (cinfo->progress != NULL) {
    cinfo->progress->completed__passes = master->pass__number;
    cinfo->progress->total__passes = master->pass__number +
				    (master->pub.is__dummy__pass ? 2 : 1);
    /* In buffered-image mode, we assume one more output pass if EOI not
     * yet reached, but no more passes if EOI has been reached.
     */
    if (cinfo->buffered__image && ! cinfo->inputctl->eoi__reached) {
      cinfo->progress->total__passes += (cinfo->enable__2pass__quant ? 2 : 1);
    }
  }
}


/*
 * Finish up at end of an output pass.
 */

METHODDEF(void)
finish__output__pass (j__decompress__ptr cinfo)
{
  my__master__ptr master = (my__master__ptr) cinfo->master;

  if (cinfo->quantize__colors)
    (*cinfo->cquantize->finish__pass) (cinfo);
  master->pass__number++;
}


#ifdef D__MULTISCAN__FILES__SUPPORTED

/*
 * Switch to a new external colormap between output passes.
 */

GLOBAL(void)
jpeg__new__colormap (j__decompress__ptr cinfo)
{
  my__master__ptr master = (my__master__ptr) cinfo->master;

  /* Prevent application from calling me at wrong times */
  if (cinfo->global__state != DSTATE__BUFIMAGE)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  if (cinfo->quantize__colors && cinfo->enable__external__quant &&
      cinfo->colormap != NULL) {
    /* Select 2-pass quantizer for external colormap use */
    cinfo->cquantize = master->quantizer__2pass;
    /* Notify quantizer of colormap change */
    (*cinfo->cquantize->new__color__map) (cinfo);
    master->pub.is__dummy__pass = FALSE; /* just in case */
  } else
    ERREXIT(cinfo, JERR__MODE__CHANGE);
}

#endif /* D__MULTISCAN__FILES__SUPPORTED */


/*
 * Initialize master decompression control and select active modules.
 * This is performed at the start of jpeg__start__decompress.
 */

GLOBAL(void)
jinit__master__decompress (j__decompress__ptr cinfo)
{
  my__master__ptr master;

  master = (my__master__ptr)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  SIZEOF(my__decomp__master));
  cinfo->master = (struct jpeg__decomp__master *) master;
  master->pub.prepare__for__output__pass = prepare__for__output__pass;
  master->pub.finish__output__pass = finish__output__pass;

  master->pub.is__dummy__pass = FALSE;

  master__selection(cinfo);
}
