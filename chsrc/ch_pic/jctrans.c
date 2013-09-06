/*
 * jctrans.c
 *
 * Copyright (C) 1995-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains library routines for transcoding compression,
 * that is, writing raw DCT coefficient arrays to an output JPEG file.
 * The routines in jcapimin.c will also be needed by a transcoder.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Forward declarations */
LOCAL(void) transencode__master__selection
	JPP((j__compress__ptr cinfo, jvirt__barray__ptr * coef__arrays));
LOCAL(void) transencode__coef__controller
	JPP((j__compress__ptr cinfo, jvirt__barray__ptr * coef__arrays));


/*
 * Compression initialization for writing raw-coefficient data.
 * Before calling this, all parameters and a data destination must be set up.
 * Call jpeg__finish__compress() to actually write the data.
 *
 * The number of passed virtual arrays must match cinfo->num__components.
 * Note that the virtual arrays need not be filled or even realized at
 * the time write__coefficients is called; indeed, if the virtual arrays
 * were requested from this compression object's memory manager, they
 * typically will be realized during this routine and filled afterwards.
 */

GLOBAL(void)
jpeg__write__coefficients (j__compress__ptr cinfo, jvirt__barray__ptr * coef__arrays)
{
  if (cinfo->global__state != CSTATE__START)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  /* Mark all tables to be written */
  jpeg__suppress__tables(cinfo, FALSE);
  /* (Re)initialize error mgr and destination modules */
  (*cinfo->err->reset__error__mgr) ((j__common__ptr) cinfo);
  (*cinfo->dest->init__destination) (cinfo);
  /* Perform master selection of active modules */
  transencode__master__selection(cinfo, coef__arrays);
  /* Wait for jpeg__finish__compress() call */
  cinfo->next__scanline = 0;	/* so jpeg__write__marker works */
  cinfo->global__state = CSTATE__WRCOEFS;
}


/*
 * Initialize the compression object with default parameters,
 * then copy from the source object all parameters needed for lossless
 * transcoding.  Parameters that can be varied without loss (such as
 * scan script and Huffman optimization) are left in their default states.
 */

GLOBAL(void)
jpeg__copy__critical__parameters (j__decompress__ptr srcinfo,
			       j__compress__ptr dstinfo)
{
  JQUANT__TBL ** qtblptr;
  jpeg__component__info *incomp, *outcomp;
  JQUANT__TBL *c__quant, *slot__quant;
  int tblno, ci, coefi;

  /* Safety check to ensure start__compress not called yet. */
  if (dstinfo->global__state != CSTATE__START)
    ERREXIT1(dstinfo, JERR__BAD__STATE, dstinfo->global__state);
  /* Copy fundamental image dimensions */
  dstinfo->image__width = srcinfo->image__width;
  dstinfo->image__height = srcinfo->image__height;
  dstinfo->input__components = srcinfo->num__components;
  dstinfo->in__color__space = srcinfo->jpeg__color__space;
  /* Initialize all parameters to default values */
  jpeg__set__defaults(dstinfo);
  /* jpeg__set__defaults may choose wrong colorspace, eg YCbCr if input is RGB.
   * Fix it to get the right header markers for the image colorspace.
   */
  jpeg__set__colorspace(dstinfo, srcinfo->jpeg__color__space);
  dstinfo->data__precision = srcinfo->data__precision;
  dstinfo->CCIR601__sampling = srcinfo->CCIR601__sampling;
  /* Copy the source's quantization tables. */
  for (tblno = 0; tblno < NUM__QUANT__TBLS; tblno++) {
    if (srcinfo->quant__tbl__ptrs[tblno] != NULL) {
      qtblptr = & dstinfo->quant__tbl__ptrs[tblno];
      if (*qtblptr == NULL)
	*qtblptr = jpeg__alloc__quant__table((j__common__ptr) dstinfo);
      MEMCOPY((*qtblptr)->quantval,
	      srcinfo->quant__tbl__ptrs[tblno]->quantval,
	      SIZEOF((*qtblptr)->quantval));
      (*qtblptr)->sent__table = FALSE;
    }
  }
  /* Copy the source's per-component info.
   * Note we assume jpeg__set__defaults has allocated the dest comp__info array.
   */
  dstinfo->num__components = srcinfo->num__components;
  if (dstinfo->num__components < 1 || dstinfo->num__components > MAX__COMPONENTS)
    ERREXIT2(dstinfo, JERR__COMPONENT__COUNT, dstinfo->num__components,
	     MAX__COMPONENTS);
  for (ci = 0, incomp = srcinfo->comp__info, outcomp = dstinfo->comp__info;
       ci < dstinfo->num__components; ci++, incomp++, outcomp++) {
    outcomp->component__id = incomp->component__id;
    outcomp->h__samp__factor = incomp->h__samp__factor;
    outcomp->v__samp__factor = incomp->v__samp__factor;
    outcomp->quant__tbl__no = incomp->quant__tbl__no;
    /* Make sure saved quantization table for component matches the qtable
     * slot.  If not, the input file re-used this qtable slot.
     * IJG encoder currently cannot duplicate this.
     */
    tblno = outcomp->quant__tbl__no;
    if (tblno < 0 || tblno >= NUM__QUANT__TBLS ||
	srcinfo->quant__tbl__ptrs[tblno] == NULL)
      ERREXIT1(dstinfo, JERR__NO__QUANT__TABLE, tblno);
    slot__quant = srcinfo->quant__tbl__ptrs[tblno];
    c__quant = incomp->quant__table;
    if (c__quant != NULL) {
      for (coefi = 0; coefi < DCTSIZE2; coefi++) {
	if (c__quant->quantval[coefi] != slot__quant->quantval[coefi])
	  ERREXIT1(dstinfo, JERR__MISMATCHED__QUANT__TABLE, tblno);
      }
    }
    /* Note: we do not copy the source's Huffman table assignments;
     * instead we rely on jpeg__set__colorspace to have made a suitable choice.
     */
  }
  /* Also copy JFIF version and resolution information, if available.
   * Strictly speaking this isn't "critical" info, but it's nearly
   * always appropriate to copy it if available.  In particular,
   * if the application chooses to copy JFIF 1.02 extension markers from
   * the source file, we need to copy the version to make sure we don't
   * emit a file that has 1.02 extensions but a claimed version of 1.01.
   * We will *not*, however, copy version info from mislabeled "2.01" files.
   */
  if (srcinfo->saw__JFIF__marker) {
    if (srcinfo->JFIF__major__version == 1) {
      dstinfo->JFIF__major__version = srcinfo->JFIF__major__version;
      dstinfo->JFIF__minor__version = srcinfo->JFIF__minor__version;
    }
    dstinfo->density__unit = srcinfo->density__unit;
    dstinfo->X__density = srcinfo->X__density;
    dstinfo->Y__density = srcinfo->Y__density;
  }
}


/*
 * Master selection of compression modules for transcoding.
 * This substitutes for jcinit.c's initialization of the full compressor.
 */

LOCAL(void)
transencode__master__selection (j__compress__ptr cinfo,
			      jvirt__barray__ptr * coef__arrays)
{
  /* Although we don't actually use input__components for transcoding,
   * jcmaster.c's initial__setup will complain if input__components is 0.
   */
  cinfo->input__components = 1;
  /* Initialize master control (includes parameter checking/processing) */
  jinit__c__master__control(cinfo, TRUE /* transcode only */);

  /* Entropy encoding: either Huffman or arithmetic coding. */
  if (cinfo->arith__code) {
    ERREXIT(cinfo, JERR__ARITH__NOTIMPL);
  } else {
    if (cinfo->progressive__mode) {
#ifdef C__PROGRESSIVE__SUPPORTED
      jinit__phuff__encoder(cinfo);
#else
      ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
    } else
      jinit__huff__encoder(cinfo);
  }

  /* We need a special coefficient buffer controller. */
  transencode__coef__controller(cinfo, coef__arrays);

  jinit__marker__writer(cinfo);

  /* We can now tell the memory manager to allocate virtual arrays. */
  (*cinfo->mem->realize__virt__arrays) ((j__common__ptr) cinfo);

  /* Write the datastream header (SOI, JFIF) immediately.
   * Frame and scan headers are postponed till later.
   * This lets application insert special markers after the SOI.
   */
  (*cinfo->marker->write__file__header) (cinfo);
}


/*
 * The rest of this file is a special implementation of the coefficient
 * buffer controller.  This is similar to jccoefct.c, but it handles only
 * output from presupplied virtual arrays.  Furthermore, we generate any
 * dummy padding blocks on-the-fly rather than expecting them to be present
 * in the arrays.
 */

/* Private buffer controller object */

typedef struct {
  struct jpeg__c__coef__controller pub; /* public fields */

  JDIMENSION iMCU__row__num;	/* iMCU row # within image */
  JDIMENSION mcu__ctr;		/* counts MCUs processed in current row */
  int MCU__vert__offset;		/* counts MCU rows within iMCU row */
  int MCU__rows__per__iMCU__row;	/* number of such rows needed */

  /* Virtual block array for each component. */
  jvirt__barray__ptr * whole__image;

  /* Workspace for constructing dummy blocks at right/bottom edges. */
  JBLOCKROW dummy__buffer[C__MAX__BLOCKS__IN__MCU];
} my__coef__controller;

typedef my__coef__controller * my__coef__ptr;


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

  if (pass__mode != JBUF__CRANK__DEST)
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);

  coef->iMCU__row__num = 0;
  start__iMCU__row(cinfo);
}


/*
 * Process some data.
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
  JDIMENSION last__MCU__col = cinfo->MCUs__per__row - 1;
  JDIMENSION last__iMCU__row = cinfo->total__iMCU__rows - 1;
  int blkn, ci, xindex, yindex, yoffset, blockcnt;
  JDIMENSION start__col;
  JBLOCKARRAY buffer[MAX__COMPS__IN__SCAN];
  JBLOCKROW MCU__buffer[C__MAX__BLOCKS__IN__MCU];
  JBLOCKROW buffer__ptr;
  jpeg__component__info *compptr;

  /* Align the virtual buffers for the components used in this scan. */
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
	blockcnt = (MCU__col__num < last__MCU__col) ? compptr->MCU__width
						: compptr->last__col__width;
	for (yindex = 0; yindex < compptr->MCU__height; yindex++) {
	  if (coef->iMCU__row__num < last__iMCU__row ||
	      yindex+yoffset < compptr->last__row__height) {
	    /* Fill in pointers to real blocks in this row */
	    buffer__ptr = buffer[ci][yindex+yoffset] + start__col;
	    for (xindex = 0; xindex < blockcnt; xindex++)
	      MCU__buffer[blkn++] = buffer__ptr++;
	  } else {
	    /* At bottom of image, need a whole row of dummy blocks */
	    xindex = 0;
	  }
	  /* Fill in any dummy blocks needed in this row.
	   * Dummy blocks are filled in the same way as in jccoefct.c:
	   * all zeroes in the AC entries, DC entries equal to previous
	   * block's DC value.  The init routine has already zeroed the
	   * AC entries, so we need only set the DC entries correctly.
	   */
	  for (; xindex < compptr->MCU__width; xindex++) {
	    MCU__buffer[blkn] = coef->dummy__buffer[blkn];
	    MCU__buffer[blkn][0][0] = MCU__buffer[blkn-1][0][0];
	    blkn++;
	  }
	}
      }
      /* Try to write the MCU. */
      if (! (*cinfo->entropy->encode__mcu) (cinfo, MCU__buffer)) {
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


/*
 * Initialize coefficient buffer controller.
 *
 * Each passed coefficient array must be the right size for that
 * coefficient: width__in__blocks wide and height__in__blocks high,
 * with unitheight at least v__samp__factor.
 */

LOCAL(void)
transencode__coef__controller (j__compress__ptr cinfo,
			     jvirt__barray__ptr * coef__arrays)
{
  my__coef__ptr coef;
  JBLOCKROW buffer;
  int i;

  coef = (my__coef__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__coef__controller));
  cinfo->coef = (struct jpeg__c__coef__controller *) coef;
  coef->pub.start__pass = start__pass__coef;
  coef->pub.compress__data = compress__output;

  /* Save pointer to virtual arrays */
  coef->whole__image = coef__arrays;

  /* Allocate and pre-zero space for dummy DCT blocks. */
  buffer = (JBLOCKROW)
    (*cinfo->mem->alloc__large) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				C__MAX__BLOCKS__IN__MCU * SIZEOF(JBLOCK));
  jzero__far((void FAR *) buffer, C__MAX__BLOCKS__IN__MCU * SIZEOF(JBLOCK));
  for (i = 0; i < C__MAX__BLOCKS__IN__MCU; i++) {
    coef->dummy__buffer[i] = buffer + i;
  }
}
