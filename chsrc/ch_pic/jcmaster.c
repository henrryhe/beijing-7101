/*
 * jcmaster.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains master control logic for the JPEG compressor.
 * These routines are concerned with parameter validation, initial setup,
 * and inter-pass control (determining the number of passes and the work 
 * to be done in each pass).
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Private state */

typedef enum {
	main__pass,		/* input data, also do first output step */
	huff__opt__pass,		/* Huffman code optimization pass */
	output__pass		/* data output pass */
} c__pass__type;

typedef struct {
  struct jpeg__comp__master pub;	/* public fields */

  c__pass__type pass__type;	/* the type of the current pass */

  int pass__number;		/* # of passes completed */
  int total__passes;		/* total # of passes needed */

  int scan__number;		/* current index in scan__info[] */
} my__comp__master;

typedef my__comp__master * my__master__ptr;


/*
 * Support routines that do various essential calculations.
 */

LOCAL(void)
initial__setup (j__compress__ptr cinfo)
/* Do computations that are needed before master selection phase */
{
  int ci;
  jpeg__component__info *compptr;
  long samplesperrow;
  JDIMENSION jd__samplesperrow;

  /* Sanity check on image dimensions */
  if (cinfo->image__height <= 0 || cinfo->image__width <= 0
      || cinfo->num__components <= 0 || cinfo->input__components <= 0)
    ERREXIT(cinfo, JERR__EMPTY__IMAGE);

  /* Make sure image isn't bigger than I can handle */
  if ((long) cinfo->image__height > (long) JPEG__MAX__DIMENSION ||
      (long) cinfo->image__width > (long) JPEG__MAX__DIMENSION)
    ERREXIT1(cinfo, JERR__IMAGE__TOO__BIG, (unsigned int) JPEG__MAX__DIMENSION);

  /* Width of an input scanline must be representable as JDIMENSION. */
  samplesperrow = (long) cinfo->image__width * (long) cinfo->input__components;
  jd__samplesperrow = (JDIMENSION) samplesperrow;
  if ((long) jd__samplesperrow != samplesperrow)
    ERREXIT(cinfo, JERR__WIDTH__OVERFLOW);

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

  /* Compute dimensions of components */
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    /* Fill in the correct component__index value; don't rely on application */
    compptr->component__index = ci;
    /* For compression, we never do DCT scaling. */
    compptr->DCT__scaled__size = DCTSIZE;
    /* Size in DCT blocks */
    compptr->width__in__blocks = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__width * (long) compptr->h__samp__factor,
		    (long) (cinfo->max__h__samp__factor * DCTSIZE));
    compptr->height__in__blocks = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__height * (long) compptr->v__samp__factor,
		    (long) (cinfo->max__v__samp__factor * DCTSIZE));
    /* Size in samples */
    compptr->downsampled__width = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__width * (long) compptr->h__samp__factor,
		    (long) cinfo->max__h__samp__factor);
    compptr->downsampled__height = (JDIMENSION)
      jdiv__round__up((long) cinfo->image__height * (long) compptr->v__samp__factor,
		    (long) cinfo->max__v__samp__factor);
    /* Mark component needed (this flag isn't actually used for compression) */
    compptr->component__needed = TRUE;
  }

  /* Compute number of fully interleaved MCU rows (number of times that
   * main controller will call coefficient controller).
   */
  cinfo->total__iMCU__rows = (JDIMENSION)
    jdiv__round__up((long) cinfo->image__height,
		  (long) (cinfo->max__v__samp__factor*DCTSIZE));
}


#ifdef C__MULTISCAN__FILES__SUPPORTED

LOCAL(void)
validate__script (j__compress__ptr cinfo)
/* Verify that the scan script in cinfo->scan__info[] is valid; also
 * determine whether it uses progressive JPEG, and set cinfo->progressive__mode.
 */
{
  const jpeg__scan__info * scanptr;
  int scanno, ncomps, ci, coefi, thisi;
  int Ss, Se, Ah, Al;
  CH_BOOL component__sent[MAX__COMPONENTS];
#ifdef C__PROGRESSIVE__SUPPORTED
  int * last__bitpos__ptr;
  int last__bitpos[MAX__COMPONENTS][DCTSIZE2];
  /* -1 until that coefficient has been seen; then last Al for it */
#endif

  if (cinfo->num__scans <= 0)
    ERREXIT1(cinfo, JERR__BAD__SCAN__SCRIPT, 0);

  /* For sequential JPEG, all scans must have Ss=0, Se=DCTSIZE2-1;
   * for progressive JPEG, no scan can have this.
   */
  scanptr = cinfo->scan__info;
  if (scanptr->Ss != 0 || scanptr->Se != DCTSIZE2-1) {
#ifdef C__PROGRESSIVE__SUPPORTED
    cinfo->progressive__mode = TRUE;
    last__bitpos__ptr = & last__bitpos[0][0];
    for (ci = 0; ci < cinfo->num__components; ci++) 
      for (coefi = 0; coefi < DCTSIZE2; coefi++)
	*last__bitpos__ptr++ = -1;
#else
    ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
  } else {
    cinfo->progressive__mode = FALSE;
    for (ci = 0; ci < cinfo->num__components; ci++) 
      component__sent[ci] = FALSE;
  }

  for (scanno = 1; scanno <= cinfo->num__scans; scanptr++, scanno++) {
    /* Validate component indexes */
    ncomps = scanptr->comps__in__scan;
    if (ncomps <= 0 || ncomps > MAX__COMPS__IN__SCAN)
      ERREXIT2(cinfo, JERR__COMPONENT__COUNT, ncomps, MAX__COMPS__IN__SCAN);
    for (ci = 0; ci < ncomps; ci++) {
      thisi = scanptr->component__index[ci];
      if (thisi < 0 || thisi >= cinfo->num__components)
	ERREXIT1(cinfo, JERR__BAD__SCAN__SCRIPT, scanno);
      /* Components must appear in SOF order within each scan */
      if (ci > 0 && thisi <= scanptr->component__index[ci-1])
	ERREXIT1(cinfo, JERR__BAD__SCAN__SCRIPT, scanno);
    }
    /* Validate progression parameters */
    Ss = scanptr->Ss;
    Se = scanptr->Se;
    Ah = scanptr->Ah;
    Al = scanptr->Al;
    if (cinfo->progressive__mode) {
#ifdef C__PROGRESSIVE__SUPPORTED
      /* The JPEG spec simply gives the ranges 0..13 for Ah and Al, but that
       * seems wrong: the upper bound ought to depend on data precision.
       * Perhaps they really meant 0..N+1 for N-bit precision.
       * Here we allow 0..10 for 8-bit data; Al larger than 10 results in
       * out-of-range reconstructed DC values during the first DC scan,
       * which might cause problems for some decoders.
       */
#if BITS__IN__JSAMPLE == 8
#define MAX__AH__AL 10
#else
#define MAX__AH__AL 13
#endif
      if (Ss < 0 || Ss >= DCTSIZE2 || Se < Ss || Se >= DCTSIZE2 ||
	  Ah < 0 || Ah > MAX__AH__AL || Al < 0 || Al > MAX__AH__AL)
	ERREXIT1(cinfo, JERR__BAD__PROG__SCRIPT, scanno);
      if (Ss == 0) {
	if (Se != 0)		/* DC and AC together not OK */
	  ERREXIT1(cinfo, JERR__BAD__PROG__SCRIPT, scanno);
      } else {
	if (ncomps != 1)	/* AC scans must be for only one component */
	  ERREXIT1(cinfo, JERR__BAD__PROG__SCRIPT, scanno);
      }
      for (ci = 0; ci < ncomps; ci++) {
	last__bitpos__ptr = & last__bitpos[scanptr->component__index[ci]][0];
	if (Ss != 0 && last__bitpos__ptr[0] < 0) /* AC without prior DC scan */
	  ERREXIT1(cinfo, JERR__BAD__PROG__SCRIPT, scanno);
	for (coefi = Ss; coefi <= Se; coefi++) {
	  if (last__bitpos__ptr[coefi] < 0) {
	    /* first scan of this coefficient */
	    if (Ah != 0)
	      ERREXIT1(cinfo, JERR__BAD__PROG__SCRIPT, scanno);
	  } else {
	    /* not first scan */
	    if (Ah != last__bitpos__ptr[coefi] || Al != Ah-1)
	      ERREXIT1(cinfo, JERR__BAD__PROG__SCRIPT, scanno);
	  }
	  last__bitpos__ptr[coefi] = Al;
	}
      }
#endif
    } else {
      /* For sequential JPEG, all progression parameters must be these: */
      if (Ss != 0 || Se != DCTSIZE2-1 || Ah != 0 || Al != 0)
	ERREXIT1(cinfo, JERR__BAD__PROG__SCRIPT, scanno);
      /* Make sure components are not sent twice */
      for (ci = 0; ci < ncomps; ci++) {
	thisi = scanptr->component__index[ci];
	if (component__sent[thisi])
	  ERREXIT1(cinfo, JERR__BAD__SCAN__SCRIPT, scanno);
	component__sent[thisi] = TRUE;
      }
    }
  }

  /* Now verify that everything got sent. */
  if (cinfo->progressive__mode) {
#ifdef C__PROGRESSIVE__SUPPORTED
    /* For progressive mode, we only check that at least some DC data
     * got sent for each component; the spec does not require that all bits
     * of all coefficients be transmitted.  Would it be wiser to enforce
     * transmission of all coefficient bits??
     */
    for (ci = 0; ci < cinfo->num__components; ci++) {
      if (last__bitpos[ci][0] < 0)
	ERREXIT(cinfo, JERR__MISSING__DATA);
    }
#endif
  } else {
    for (ci = 0; ci < cinfo->num__components; ci++) {
      if (! component__sent[ci])
	ERREXIT(cinfo, JERR__MISSING__DATA);
    }
  }
}

#endif /* C__MULTISCAN__FILES__SUPPORTED */


LOCAL(void)
select__scan__parameters (j__compress__ptr cinfo)
/* Set up the scan parameters for the current scan */
{
  int ci;

#ifdef C__MULTISCAN__FILES__SUPPORTED
  if (cinfo->scan__info != NULL) {
    /* Prepare for current scan --- the script is already validated */
    my__master__ptr master = (my__master__ptr) cinfo->master;
    const jpeg__scan__info * scanptr = cinfo->scan__info + master->scan__number;

    cinfo->comps__in__scan = scanptr->comps__in__scan;
    for (ci = 0; ci < scanptr->comps__in__scan; ci++) {
      cinfo->cur__comp__info[ci] =
	&cinfo->comp__info[scanptr->component__index[ci]];
    }
    cinfo->Ss = scanptr->Ss;
    cinfo->Se = scanptr->Se;
    cinfo->Ah = scanptr->Ah;
    cinfo->Al = scanptr->Al;
  }
  else
#endif
  {
    /* Prepare for single sequential-JPEG scan containing all components */
    if (cinfo->num__components > MAX__COMPS__IN__SCAN)
      ERREXIT2(cinfo, JERR__COMPONENT__COUNT, cinfo->num__components,
	       MAX__COMPS__IN__SCAN);
    cinfo->comps__in__scan = cinfo->num__components;
    for (ci = 0; ci < cinfo->num__components; ci++) {
      cinfo->cur__comp__info[ci] = &cinfo->comp__info[ci];
    }
    cinfo->Ss = 0;
    cinfo->Se = DCTSIZE2-1;
    cinfo->Ah = 0;
    cinfo->Al = 0;
  }
}


LOCAL(void)
per__scan__setup (j__compress__ptr cinfo)
/* Do computations that are needed before processing a JPEG scan */
/* cinfo->comps__in__scan and cinfo->cur__comp__info[] are already set */
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
    compptr->MCU__sample__width = DCTSIZE;
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
      compptr->MCU__sample__width = compptr->MCU__width * DCTSIZE;
      /* Figure number of non-dummy blocks in last MCU column & row */
      tmp = (int) (compptr->width__in__blocks % compptr->MCU__width);
      if (tmp == 0) tmp = compptr->MCU__width;
      compptr->last__col__width = tmp;
      tmp = (int) (compptr->height__in__blocks % compptr->MCU__height);
      if (tmp == 0) tmp = compptr->MCU__height;
      compptr->last__row__height = tmp;
      /* Prepare array describing MCU composition */
      mcublks = compptr->MCU__blocks;
      if (cinfo->blocks__in__MCU + mcublks > C__MAX__BLOCKS__IN__MCU)
	ERREXIT(cinfo, JERR__BAD__MCU__SIZE);
      while (mcublks-- > 0) {
	cinfo->MCU__membership[cinfo->blocks__in__MCU++] = ci;
      }
    }
    
  }

  /* Convert restart specified in rows to actual MCU count. */
  /* Note that count must fit in 16 bits, so we provide limiting. */
  if (cinfo->restart__in__rows > 0) {
    long nominal = (long) cinfo->restart__in__rows * (long) cinfo->MCUs__per__row;
    cinfo->restart__interval = (unsigned int) MIN(nominal, 65535L);
  }
}


/*
 * Per-pass setup.
 * This is called at the beginning of each pass.  We determine which modules
 * will be active during this pass and give them appropriate start__pass calls.
 * We also set is__last__pass to indicate whether any more passes will be
 * required.
 */

METHODDEF(void)
prepare__for__pass (j__compress__ptr cinfo)
{
  my__master__ptr master = (my__master__ptr) cinfo->master;

  switch (master->pass__type) {
  case main__pass:
    /* Initial pass: will collect input data, and do either Huffman
     * optimization or data output for the first scan.
     */
    select__scan__parameters(cinfo);
    per__scan__setup(cinfo);
    if (! cinfo->raw__data__in) {
      (*cinfo->cconvert->start__pass) (cinfo);
      (*cinfo->downsample->start__pass) (cinfo);
      (*cinfo->prep->start__pass) (cinfo, JBUF__PASS__THRU);
    }
    (*cinfo->fdct->start__pass) (cinfo);
    (*cinfo->entropy->start__pass) (cinfo, cinfo->optimize__coding);
    (*cinfo->coef->start__pass) (cinfo,
				(master->total__passes > 1 ?
				 JBUF__SAVE__AND__PASS : JBUF__PASS__THRU));
    (*cinfo->main->start__pass) (cinfo, JBUF__PASS__THRU);
    if (cinfo->optimize__coding) {
      /* No immediate data output; postpone writing frame/scan headers */
      master->pub.call__pass__startup = FALSE;
    } else {
      /* Will write frame/scan headers at first jpeg__write__scanlines call */
      master->pub.call__pass__startup = TRUE;
    }
    break;
#ifdef ENTROPY__OPT__SUPPORTED
  case huff__opt__pass:
    /* Do Huffman optimization for a scan after the first one. */
    select__scan__parameters(cinfo);
    per__scan__setup(cinfo);
    if (cinfo->Ss != 0 || cinfo->Ah == 0 || cinfo->arith__code) {
      (*cinfo->entropy->start__pass) (cinfo, TRUE);
      (*cinfo->coef->start__pass) (cinfo, JBUF__CRANK__DEST);
      master->pub.call__pass__startup = FALSE;
      break;
    }
    /* Special case: Huffman DC refinement scans need no Huffman table
     * and therefore we can skip the optimization pass for them.
     */
    master->pass__type = output__pass;
    master->pass__number++;
    /*FALLTHROUGH*/
#endif
  case output__pass:
    /* Do a data-output pass. */
    /* We need not repeat per-scan setup if prior optimization pass did it. */
    if (! cinfo->optimize__coding) {
      select__scan__parameters(cinfo);
      per__scan__setup(cinfo);
    }
    (*cinfo->entropy->start__pass) (cinfo, FALSE);
    (*cinfo->coef->start__pass) (cinfo, JBUF__CRANK__DEST);
    /* We emit frame/scan headers now */
    if (master->scan__number == 0)
      (*cinfo->marker->write__frame__header) (cinfo);
    (*cinfo->marker->write__scan__header) (cinfo);
    master->pub.call__pass__startup = FALSE;
    break;
  default:
    ERREXIT(cinfo, JERR__NOT__COMPILED);
  }

  master->pub.is__last__pass = (master->pass__number == master->total__passes-1);

  /* Set up progress monitor's pass info if present */
  if (cinfo->progress != NULL) {
    cinfo->progress->completed__passes = master->pass__number;
    cinfo->progress->total__passes = master->total__passes;
  }
}


/*
 * Special start-of-pass hook.
 * This is called by jpeg__write__scanlines if call__pass__startup is TRUE.
 * In single-pass processing, we need this hook because we don't want to
 * write frame/scan headers during jpeg__start__compress; we want to let the
 * application write COM markers etc. between jpeg__start__compress and the
 * jpeg__write__scanlines loop.
 * In multi-pass processing, this routine is not used.
 */

METHODDEF(void)
pass__startup (j__compress__ptr cinfo)
{
  cinfo->master->call__pass__startup = FALSE; /* reset flag so call only once */

  (*cinfo->marker->write__frame__header) (cinfo);
  (*cinfo->marker->write__scan__header) (cinfo);
}


/*
 * Finish up at end of pass.
 */

METHODDEF(void)
finish__pass__master (j__compress__ptr cinfo)
{
  my__master__ptr master = (my__master__ptr) cinfo->master;

  /* The entropy coder always needs an end-of-pass call,
   * either to analyze statistics or to flush its output buffer.
   */
  (*cinfo->entropy->finish__pass) (cinfo);

  /* Update state for next pass */
  switch (master->pass__type) {
  case main__pass:
    /* next pass is either output of scan 0 (after optimization)
     * or output of scan 1 (if no optimization).
     */
    master->pass__type = output__pass;
    if (! cinfo->optimize__coding)
      master->scan__number++;
    break;
  case huff__opt__pass:
    /* next pass is always output of current scan */
    master->pass__type = output__pass;
    break;
  case output__pass:
    /* next pass is either optimization or output of next scan */
    if (cinfo->optimize__coding)
      master->pass__type = huff__opt__pass;
    master->scan__number++;
    break;
  }

  master->pass__number++;
}


/*
 * Initialize master compression control.
 */

GLOBAL(void)
jinit__c__master__control (j__compress__ptr cinfo, CH_BOOL transcode__only)
{
  my__master__ptr master;

  master = (my__master__ptr)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  SIZEOF(my__comp__master));
  cinfo->master = (struct jpeg__comp__master *) master;
  master->pub.prepare__for__pass = prepare__for__pass;
  master->pub.pass__startup = pass__startup;
  master->pub.finish__pass = finish__pass__master;
  master->pub.is__last__pass = FALSE;

  /* Validate parameters, determine derived values */
  initial__setup(cinfo);

  if (cinfo->scan__info != NULL) {
#ifdef C__MULTISCAN__FILES__SUPPORTED
    validate__script(cinfo);
#else
    ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
  } else {
    cinfo->progressive__mode = FALSE;
    cinfo->num__scans = 1;
  }

  if (cinfo->progressive__mode)	/*  TEMPORARY HACK ??? */
    cinfo->optimize__coding = TRUE; /* assume default tables no good for progressive mode */

  /* Initialize my private state */
  if (transcode__only) {
    /* no main pass in transcoding */
    if (cinfo->optimize__coding)
      master->pass__type = huff__opt__pass;
    else
      master->pass__type = output__pass;
  } else {
    /* for normal compression, first pass is always this type: */
    master->pass__type = main__pass;
  }
  master->scan__number = 0;
  master->pass__number = 0;
  if (cinfo->optimize__coding)
    master->total__passes = cinfo->num__scans * 2;
  else
    master->total__passes = cinfo->num__scans;
}
