/*
 * jdinput.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains input control logic for the JPEG decompressor.
 * These routines are concerned with controlling the decompressor's input
 * processing (marker reading and coefficient decoding).  The actual input
 * reading is done in jdmarker.c, jdhuff.c, and jdphuff.c.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Private state */

typedef struct {
  struct jpeg__input__controller pub; /* public fields */

  CH_BOOL inheaders;		/* TRUE until first SOS is reached */
} my__input__controller;

typedef my__input__controller * my__inputctl__ptr;


/* Forward declarations */
METHODDEF(int) consume__markers JPP((j__decompress__ptr cinfo));


/*
 * Routines to calculate various quantities related to the size of the image.
 */

LOCAL(void)
initial__setup (j__decompress__ptr cinfo)
/* Called once, when first SOS marker is reached */
{
  int ci;
  jpeg__component__info *compptr;

  /* Make sure image isn't bigger than I can handle */
  if ((long) cinfo->image__height > (long) JPEG__MAX__DIMENSION ||
      (long) cinfo->image__width > (long) JPEG__MAX__DIMENSION)
    ERREXIT1(cinfo, JERR__IMAGE__TOO__BIG, (unsigned int) JPEG__MAX__DIMENSION);

  /* For now, precision must match compiled-in value... */
  if (cinfo->data__precision != BITS__IN__JSAMPLE)
    ERREXIT1(cinfo, JERR__BAD__PRECISION, cinfo->data__precision);

  /* Check that number of components won't exceed internal array sizes */
  if (cinfo->num__components > MAX__COMPONENTS)
    ERREXIT2(cinfo, JERR__COMPONENT__COUNT, cinfo->num__components,
	     MAX__COMPONENTS);

  /* Compute maximum sampling factors; check factor validity */
  cinfo->max__h__samp__factor = 1;
  cinfo->max__v__samp__factor = 1;
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    if (compptr->h__samp__factor<=0 || compptr->h__samp__factor>MAX__SAMP__FACTOR ||
	compptr->v__samp__factor<=0 || compptr->v__samp__factor>MAX__SAMP__FACTOR)
      ERREXIT(cinfo, JERR__BAD__SAMPLING);
    cinfo->max__h__samp__factor = MAX(cinfo->max__h__samp__factor,
				   compptr->h__samp__factor);
    cinfo->max__v__samp__factor = MAX(cinfo->max__v__samp__factor,
				   compptr->v__samp__factor);
  }

  /* We initialize DCT__scaled__size and min__DCT__scaled__size to DCTSIZE.
   * In the full decompressor, this will be overridden by jdmaster.c;
   * but in the transcoder, jdmaster.c is not used, so we must do it here.
   */
  cinfo->min__DCT__scaled__size = DCTSIZE;

  /* Compute dimensions of components */
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    compptr->DCT__scaled__size = DCTSIZE;
    /* Size in DCT blocks */
    compptr->width__in__blocks = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__width * (long) compptr->h__samp__factor,
		    (long) (cinfo->max__h__samp__factor * DCTSIZE));
    compptr->height__in__blocks = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__height * (long) compptr->v__samp__factor,
		    (long) (cinfo->max__v__samp__factor * DCTSIZE));
    /* downsampled__width and downsampled__height will also be overridden by
     * jdmaster.c if we are doing full decompression.  The transcoder library
     * doesn't use these values, but the calling application might.
     */
    /* Size in samples */
    compptr->downsampled__width = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__width * (long) compptr->h__samp__factor,
		    (long) cinfo->max__h__samp__factor);
    compptr->downsampled__height = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__height * (long) compptr->v__samp__factor,
		    (long) cinfo->max__v__samp__factor);
    /* Mark component needed, until color conversion says otherwise */
    compptr->component__needed = TRUE;
    /* Mark no quantization table yet saved for component */
    compptr->quant__table = NULL;
  }

  /* Compute number of fully interleaved MCU rows. */
  cinfo->total__iMCU__rows = (JDIMENSION)
    jdiv__round__up((long) cinfo->image__height,
		  (long) (cinfo->max__v__samp__factor*DCTSIZE));

  /* Decide whether file contains multiple scans */
  if (cinfo->comps__in__scan < cinfo->num__components || cinfo->progressive__mode)
    cinfo->inputctl->has__multiple__scans = TRUE;
  else
    cinfo->inputctl->has__multiple__scans = FALSE;
}


LOCAL(void)
per__scan__setup (j__decompress__ptr cinfo)
/* Do computations that are needed before processing a JPEG scan */
/* cinfo->comps__in__scan and cinfo->cur__comp__info[] were set from SOS marker */
{
  int ci, mcublks, tmp;
  jpeg__component__info *compptr;
  
  if (cinfo->comps__in__scan == 1) {
    
    /* Noninterleaved (single-component) scan */
    compptr = cinfo->cur__comp__info[0];
    
    /* Overall image size in MCUs */
    cinfo->MCUs__per__row = compptr->width__in__blocks;
    cinfo->MCU__rows__in__scan = compptr->height__in__blocks;
    
    /* For noninterleaved scan, always one block per MCU */
    compptr->MCU__width = 1;
    compptr->MCU__height = 1;
    compptr->MCU__blocks = 1;
    compptr->MCU__sample__width = compptr->DCT__scaled__size;
    compptr->last__col__width = 1;
    /* For noninterleaved scans, it is convenient to define last__row__height
     * as the number of block rows present in the last iMCU row.
     */
    tmp = (int) (compptr->height__in__blocks % compptr->v__samp__factor);
    if (tmp == 0) tmp = compptr->v__samp__factor;
    compptr->last__row__height = tmp;
    
    /* Prepare array describing MCU composition */
    cinfo->blocks__in__MCU = 1;
    cinfo->MCU__membership[0] = 0;
    
  } else {
    
    /* Interleaved (multi-component) scan */
    if (cinfo->comps__in__scan <= 0 || cinfo->comps__in__scan > MAX__COMPS__IN__SCAN)
      ERREXIT2(cinfo, JERR__COMPONENT__COUNT, cinfo->comps__in__scan,
	       MAX__COMPS__IN__SCAN);
    
    /* Overall image size in MCUs */
    cinfo->MCUs__per__row = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__width,
		    (long) (cinfo->max__h__samp__factor*DCTSIZE));
    cinfo->MCU__rows__in__scan = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__height,
		    (long) (cinfo->max__v__samp__factor*DCTSIZE));
    
    cinfo->blocks__in__MCU = 0;
    
    for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
      compptr = cinfo->cur__comp__info[ci];
      /* Sampling factors give # of blocks of component in each MCU */
      compptr->MCU__width = compptr->h__samp__factor;
      compptr->MCU__height = compptr->v__samp__factor;
      compptr->MCU__blocks = compptr->MCU__width * compptr->MCU__height;
      compptr->MCU__sample__width = compptr->MCU__width * compptr->DCT__scaled__size;
      /* Figure number of non-dummy blocks in last MCU column & row */
      tmp = (int) (compptr->width__in__blocks % compptr->MCU__width);
      if (tmp == 0) tmp = compptr->MCU__width;
      compptr->last__col__width = tmp;
      tmp = (int) (compptr->height__in__blocks % compptr->MCU__height);
      if (tmp == 0) tmp = compptr->MCU__height;
      compptr->last__row__height = tmp;
      /* Prepare array describing MCU composition */
      mcublks = compptr->MCU__blocks;
      if (cinfo->blocks__in__MCU + mcublks > D__MAX__BLOCKS__IN__MCU)
	ERREXIT(cinfo, JERR__BAD__MCU__SIZE);
      while (mcublks-- > 0) {
	cinfo->MCU__membership[cinfo->blocks__in__MCU++] = ci;
      }
    }
    
  }
}


/*
 * Save away a copy of the Q-table referenced by each component present
 * in the current scan, unless already saved during a prior scan.
 *
 * In a multiple-scan JPEG file, the encoder could assign different components
 * the same Q-table slot number, but change table definitions between scans
 * so that each component uses a different Q-table.  (The IJG encoder is not
 * currently capable of doing this, but other encoders might.)  Since we want
 * to be able to dequantize all the components at the end of the file, this
 * means that we have to save away the table actually used for each component.
 * We do this by copying the table at the start of the first scan containing
 * the component.
 * The JPEG spec prohibits the encoder from changing the contents of a Q-table
 * slot between scans of a component using that slot.  If the encoder does so
 * anyway, this decoder will simply use the Q-table values that were current
 * at the start of the first scan for the component.
 *
 * The decompressor output side looks only at the saved quant tables,
 * not at the current Q-table slots.
 */

LOCAL(void)
latCH_quant__tables (j__decompress__ptr cinfo)
{
  int ci, qtblno;
  jpeg__component__info *compptr;
  JQUANT__TBL * qtbl;

  for (ci = 0; ci < cinfo->comps__in__scan; ci++) {
    compptr = cinfo->cur__comp__info[ci];
    /* No work if we already saved Q-table for this component */
    if (compptr->quant__table != NULL)
      continue;
    /* Make sure specified quantization table is present */
    qtblno = compptr->quant__tbl__no;
    if (qtblno < 0 || qtblno >= NUM__QUANT__TBLS ||
	cinfo->quant__tbl__ptrs[qtblno] == NULL)
      ERREXIT1(cinfo, JERR__NO__QUANT__TABLE, qtblno);
    /* OK, save away the quantization table */
    qtbl = (JQUANT__TBL *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  SIZEOF(JQUANT__TBL));
    MEMCOPY(qtbl, cinfo->quant__tbl__ptrs[qtblno], SIZEOF(JQUANT__TBL));
    compptr->quant__table = qtbl;
  }
}


/*
 * Initialize the input modules to read a scan of compressed data.
 * The first call to this is done by jdmaster.c after initializing
 * the entire decompressor (during jpeg__start__decompress).
 * Subsequent calls come from consume__markers, below.
 */

METHODDEF(void)
start__input__pass (j__decompress__ptr cinfo)
{
  per__scan__setup(cinfo);
  latCH_quant__tables(cinfo);
  (*cinfo->entropy->start__pass) (cinfo);
  (*cinfo->coef->start__input__pass) (cinfo);
  cinfo->inputctl->consume__input = cinfo->coef->consume__data;
}


/*
 * Finish up after inputting a compressed-data scan.
 * This is called by the coefficient controller after it's read all
 * the expected data of the scan.
 */

METHODDEF(void)
finish__input__pass (j__decompress__ptr cinfo)
{
  cinfo->inputctl->consume__input = consume__markers;
}


/*
 * Read JPEG markers before, between, or after compressed-data scans.
 * Change state as necessary when a new scan is reached.
 * Return value is JPEG__SUSPENDED, JPEG__REACHED__SOS, or JPEG__REACHED__EOI.
 *
 * The consume__input method pointer points either here or to the
 * coefficient controller's consume__data routine, depending on whether
 * we are reading a compressed data segment or inter-segment markers.
 */

METHODDEF(int)
consume__markers (j__decompress__ptr cinfo)
{
  my__inputctl__ptr inputctl = (my__inputctl__ptr) cinfo->inputctl;
  int val;

  if (inputctl->pub.eoi__reached) /* After hitting EOI, read no further */
    return JPEG__REACHED__EOI;

  val = (*cinfo->marker->read__markers) (cinfo);

  switch (val) {
  case JPEG__REACHED__SOS:	/* Found SOS */
    if (inputctl->inheaders) {	/* 1st SOS */
      initial__setup(cinfo);
      inputctl->inheaders = FALSE;
      /* Note: start__input__pass must be called by jdmaster.c
       * before any more input can be consumed.  jdapimin.c is
       * responsible for enforcing this sequencing.
       */
    } else {			/* 2nd or later SOS marker */
      if (! inputctl->pub.has__multiple__scans)
	ERREXIT(cinfo, JERR__EOI__EXPECTED); /* Oops, I wasn't expecting this! */
      start__input__pass(cinfo);
    }
    break;
  case JPEG__REACHED__EOI:	/* Found EOI */
    inputctl->pub.eoi__reached = TRUE;
    if (inputctl->inheaders) {	/* Tables-only datastream, apparently */
      if (cinfo->marker->saw__SOF)
	ERREXIT(cinfo, JERR__SOF__NO__SOS);
    } else {
      /* Prevent infinite loop in coef ctlr's decompress__data routine
       * if user set output__scan__number larger than number of scans.
       */
      if (cinfo->output__scan__number > cinfo->input__scan__number)
	cinfo->output__scan__number = cinfo->input__scan__number;
    }
    break;
  case JPEG__SUSPENDED:
    break;
  }

  return val;
}


/*
 * Reset state to begin a fresh datastream.
 */

METHODDEF(void)
reset__input__controller (j__decompress__ptr cinfo)
{
  my__inputctl__ptr inputctl = (my__inputctl__ptr) cinfo->inputctl;

  inputctl->pub.consume__input = consume__markers;
  inputctl->pub.has__multiple__scans = FALSE; /* "unknown" would be better */
  inputctl->pub.eoi__reached = FALSE;
  inputctl->inheaders = TRUE;
  /* Reset other modules */
  (*cinfo->err->reset__error__mgr) ((j__common__ptr) cinfo);
  (*cinfo->marker->reset__marker__reader) (cinfo);
  /* Reset progression state -- would be cleaner if entropy decoder did this */
  cinfo->coef__bits = NULL;
}


/*
 * Initialize the input controller module.
 * This is called only once, when the decompression object is created.
 */

GLOBAL(void)
jinit__input__controller (j__decompress__ptr cinfo)
{
  my__inputctl__ptr inputctl;

  /* Create subobject in permanent pool */
  inputctl = (my__inputctl__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__PERMANENT,
				SIZEOF(my__input__controller));
  cinfo->inputctl = (struct jpeg__input__controller *) inputctl;
  /* Initialize method pointers */
  inputctl->pub.consume__input = consume__markers;
  inputctl->pub.reset__input__controller = reset__input__controller;
  inputctl->pub.start__input__pass = start__input__pass;
  inputctl->pub.finish__input__pass = finish__input__pass;
  /* Initialize state: can't use reset__input__controller since we don't
   * want to try to reset other modules yet.
   */
  inputctl->pub.has__multiple__scans = FALSE; /* "unknown" would be better */
  inputctl->pub.eoi__reached = FALSE;
  inputctl->inheaders = TRUE;
}
