/*
 * jdmainct.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the main buffer controller for decompression.
 * The main buffer lies between the JPEG decompressor proper and the
 * post-processor; it holds downsampled data in the JPEG colorspace.
 *
 * Note that this code is bypassed in raw-data mode, since the application
 * supplies the equivalent of the main buffer in that case.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/*
 * In the current system design, the main buffer need never be a full-image
 * buffer; any full-height buffers will be found inside the coefficient or
 * postprocessing controllers.  Nonetheless, the main controller is not
 * trivial.  Its responsibility is to provide context rows for upsampling/
 * rescaling, and doing this in an efficient fashion is a bit tricky.
 *
 * Postprocessor input data is counted in "row groups".  A row group
 * is defined to be (v__samp__factor * DCT__scaled__size / min__DCT__scaled__size)
 * sample rows of each component.  (We require DCT__scaled__size values to be
 * chosen such that these numbers are integers.  In practice DCT__scaled__size
 * values will likely be powers of two, so we actually have the stronger
 * condition that DCT__scaled__size / min__DCT__scaled__size is an integer.)
 * Upsampling will typically produce max__v__samp__factor pixel rows from each
 * row group (times any additional scale factor that the upsampler is
 * applying).
 *
 * The coefficient controller will deliver data to us one iMCU row at a time;
 * each iMCU row contains v__samp__factor * DCT__scaled__size sample rows, or
 * exactly min__DCT__scaled__size row groups.  (This amount of data corresponds
 * to one row of MCUs when the image is fully interleaved.)  Note that the
 * number of sample rows varies across components, but the number of row
 * groups does not.  Some garbage sample rows may be included in the last iMCU
 * row at the bottom of the image.
 *
 * Depending on the vertical scaling algorithm used, the upsampler may need
 * access to the sample row(s) above and below its current input row group.
 * The upsampler is required to set need__context__rows TRUE at global selection
 * time if so.  When need__context__rows is FALSE, this controller can simply
 * obtain one iMCU row at a time from the coefficient controller and dole it
 * out as row groups to the postprocessor.
 *
 * When need__context__rows is TRUE, this controller guarantees that the buffer
 * passed to postprocessing contains at least one row group's worth of samples
 * above and below the row group(s) being processed.  Note that the context
 * rows "above" the first passed row group appear at negative row offsets in
 * the passed buffer.  At the top and bottom of the image, the required
 * context rows are manufactured by duplicating the first or last real sample
 * row; this avoids having special cases in the upsampling inner loops.
 *
 * The amount of context is fixed at one row group just because that's a
 * convenient number for this controller to work with.  The existing
 * upsamplers really only need one sample row of context.  An upsampler
 * supporting arbitrary output rescaling might wish for more than one row
 * group of context when shrinking the image; tough, we don't handle that.
 * (This is justified by the assumption that downsizing will be handled mostly
 * by adjusting the DCT__scaled__size values, so that the actual scale factor at
 * the upsample step needn't be much less than one.)
 *
 * To provide the desired context, we have to retain the last two row groups
 * of one iMCU row while reading in the next iMCU row.  (The last row group
 * can't be processed until we have another row group for its below-context,
 * and so we have to save the next-to-last group too for its above-context.)
 * We could do this most simply by copying data around in our buffer, but
 * that'd be very slow.  We can avoid copying any data by creating a rather
 * strange pointer structure.  Here's how it works.  We allocate a workspace
 * consisting of M+2 row groups (where M = min__DCT__scaled__size is the number
 * of row groups per iMCU row).  We create two sets of redundant pointers to
 * the workspace.  Labeling the physical row groups 0 to M+1, the synthesized
 * pointer lists look like this:
 *                   M+1                          M-1
 * master pointer --> 0         master pointer --> 0
 *                    1                            1
 *                   ...                          ...
 *                   M-3                          M-3
 *                   M-2                           M
 *                   M-1                          M+1
 *                    M                           M-2
 *                   M+1                          M-1
 *                    0                            0
 * We read alternate iMCU rows using each master pointer; thus the last two
 * row groups of the previous iMCU row remain un-overwritten in the workspace.
 * The pointer lists are set up so that the required context rows appear to
 * be adjacent to the proper places when we pass the pointer lists to the
 * upsampler.
 *
 * The above pictures describe the normal state of the pointer lists.
 * At top and bottom of the image, we diddle the pointer lists to duplicate
 * the first or last sample row as necessary (this is cheaper than copying
 * sample rows around).
 *
 * This scheme breaks down if M < 2, ie, min__DCT__scaled__size is 1.  In that
 * situation each iMCU row provides only one row group so the buffering logic
 * must be different (eg, we must read two iMCU rows before we can emit the
 * first row group).  For now, we simply do not support providing context
 * rows when min__DCT__scaled__size is 1.  That combination seems unlikely to
 * be worth providing --- if someone wants a 1/8th-size preview, they probably
 * want it quick and dirty, so a context-free upsampler is sufficient.
 */


/* Private buffer controller object */

typedef struct {
  struct jpeg__d__main__controller pub; /* public fields */

  /* Pointer to allocated workspace (M or M+2 row groups). */
  JSAMPARRAY buffer[MAX__COMPONENTS];

  CH_BOOL buffer__full;		/* Have we gotten an iMCU row from decoder? */
  JDIMENSION rowgroup__ctr;	/* counts row groups output to postprocessor */

  /* Remaining fields are only used in the context case. */

  /* These are the master pointers to the funny-order pointer lists. */
  JSAMPIMAGE xbuffer[2];	/* pointers to weird pointer lists */

  int whichptr;			/* indicates which pointer set is now in use */
  int context__state;		/* process__data state machine status */
  JDIMENSION rowgroups__avail;	/* row groups available to postprocessor */
  JDIMENSION iMCU__row__ctr;	/* counts iMCU rows to detect image top/bot */
} my__main__controller;

typedef my__main__controller * my__main__ptr;

/* context__state values: */
#define CTX__PREPARE__FOR__IMCU	0	/* need to prepare for MCU row */
#define CTX__PROCESS__IMCU	1	/* feeding iMCU to postprocessor */
#define CTX__POSTPONED__ROW	2	/* feeding postponed row group */


/* Forward declarations */
METHODDEF(void) process__data__simple__main
	JPP((j__decompress__ptr cinfo, JSAMPARRAY output__buf,
	     JDIMENSION *out__row__ctr, JDIMENSION out__rows__avail));
METHODDEF(void) process__data__context__main
	JPP((j__decompress__ptr cinfo, JSAMPARRAY output__buf,
	     JDIMENSION *out__row__ctr, JDIMENSION out__rows__avail));
#ifdef QUANT__2PASS__SUPPORTED
METHODDEF(void) process__data__crank__post
	JPP((j__decompress__ptr cinfo, JSAMPARRAY output__buf,
	     JDIMENSION *out__row__ctr, JDIMENSION out__rows__avail));
#endif


LOCAL(void)
alloc__funny__pointers (j__decompress__ptr cinfo)
/* Allocate space for the funny pointer lists.
 * This is done only once, not once per pass.
 */
{
  my__main__ptr main = (my__main__ptr) cinfo->main;
  int ci, rgroup;
  int M = cinfo->min__DCT__scaled__size;
  jpeg__component__info *compptr;
  JSAMPARRAY xbuf;

  /* Get top-level space for component array pointers.
   * We alloc both arrays with one call to save a few cycles.
   */
  main->xbuffer[0] = (JSAMPIMAGE)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				cinfo->num__components * 2 * SIZEOF(JSAMPARRAY));
  main->xbuffer[1] = main->xbuffer[0] + cinfo->num__components;

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    rgroup = (compptr->v__samp__factor * compptr->DCT__scaled__size) /
      cinfo->min__DCT__scaled__size; /* height of a row group of component */
    /* Get space for pointer lists --- M+4 row groups in each list.
     * We alloc both pointer lists with one call to save a few cycles.
     */
    xbuf = (JSAMPARRAY)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  2 * (rgroup * (M + 4)) * SIZEOF(JSAMPROW));
    xbuf += rgroup;		/* want one row group at negative offsets */
    main->xbuffer[0][ci] = xbuf;
    xbuf += rgroup * (M + 4);
    main->xbuffer[1][ci] = xbuf;
  }
}


LOCAL(void)
make__funny__pointers (j__decompress__ptr cinfo)
/* Create the funny pointer lists discussed in the comments above.
 * The actual workspace is already allocated (in main->buffer),
 * and the space for the pointer lists is allocated too.
 * This routine just fills in the curiously ordered lists.
 * This will be repeated at the beginning of each pass.
 */
{
  my__main__ptr main = (my__main__ptr) cinfo->main;
  int ci, i, rgroup;
  int M = cinfo->min__DCT__scaled__size;
  jpeg__component__info *compptr;
  JSAMPARRAY buf, xbuf0, xbuf1;

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    rgroup = (compptr->v__samp__factor * compptr->DCT__scaled__size) /
      cinfo->min__DCT__scaled__size; /* height of a row group of component */
    xbuf0 = main->xbuffer[0][ci];
    xbuf1 = main->xbuffer[1][ci];
    /* First copy the workspace pointers as-is */
    buf = main->buffer[ci];
    for (i = 0; i < rgroup * (M + 2); i++) {
      xbuf0[i] = xbuf1[i] = buf[i];
    }
    /* In the second list, put the last four row groups in swapped order */
    for (i = 0; i < rgroup * 2; i++) {
      xbuf1[rgroup*(M-2) + i] = buf[rgroup*M + i];
      xbuf1[rgroup*M + i] = buf[rgroup*(M-2) + i];
    }
    /* The wraparound pointers at top and bottom will be filled later
     * (see set__wraparound__pointers, below).  Initially we want the "above"
     * pointers to duplicate the first actual data line.  This only needs
     * to happen in xbuffer[0].
     */
    for (i = 0; i < rgroup; i++) {
      xbuf0[i - rgroup] = xbuf0[0];
    }
  }
}


LOCAL(void)
set__wraparound__pointers (j__decompress__ptr cinfo)
/* Set up the "wraparound" pointers at top and bottom of the pointer lists.
 * This changes the pointer list state from top-of-image to the normal state.
 */
{
  my__main__ptr main = (my__main__ptr) cinfo->main;
  int ci, i, rgroup;
  int M = cinfo->min__DCT__scaled__size;
  jpeg__component__info *compptr;
  JSAMPARRAY xbuf0, xbuf1;

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    rgroup = (compptr->v__samp__factor * compptr->DCT__scaled__size) /
      cinfo->min__DCT__scaled__size; /* height of a row group of component */
    xbuf0 = main->xbuffer[0][ci];
    xbuf1 = main->xbuffer[1][ci];
    for (i = 0; i < rgroup; i++) {
      xbuf0[i - rgroup] = xbuf0[rgroup*(M+1) + i];
      xbuf1[i - rgroup] = xbuf1[rgroup*(M+1) + i];
      xbuf0[rgroup*(M+2) + i] = xbuf0[i];
      xbuf1[rgroup*(M+2) + i] = xbuf1[i];
    }
  }
}


LOCAL(void)
set__bottom__pointers (j__decompress__ptr cinfo)
/* Change the pointer lists to duplicate the last sample row at the bottom
 * of the image.  whichptr indicates which xbuffer holds the final iMCU row.
 * Also sets rowgroups__avail to indicate number of nondummy row groups in row.
 */
{
  my__main__ptr main = (my__main__ptr) cinfo->main;
  int ci, i, rgroup, iMCUheight, rows__left;
  jpeg__component__info *compptr;
  JSAMPARRAY xbuf;

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    /* Count sample rows in one iMCU row and in one row group */
    iMCUheight = compptr->v__samp__factor * compptr->DCT__scaled__size;
    rgroup = iMCUheight / cinfo->min__DCT__scaled__size;
    /* Count nondummy sample rows remaining for this component */
    rows__left = (int) (compptr->downsampled__height % (JDIMENSION) iMCUheight);
    if (rows__left == 0) rows__left = iMCUheight;
    /* Count nondummy row groups.  Should get same answer for each component,
     * so we need only do it once.
     */
    if (ci == 0) {
      main->rowgroups__avail = (JDIMENSION) ((rows__left-1) / rgroup + 1);
    }
    /* Duplicate the last real sample row rgroup*2 times; this pads out the
     * last partial rowgroup and ensures at least one full rowgroup of context.
     */
    xbuf = main->xbuffer[main->whichptr][ci];
    for (i = 0; i < rgroup * 2; i++) {
      xbuf[rows__left + i] = xbuf[rows__left-1];
    }
  }
}


/*
 * Initialize for a processing pass.
 */

METHODDEF(void)
start__pass__main (j__decompress__ptr cinfo, J__BUF__MODE pass__mode)
{
  my__main__ptr main = (my__main__ptr) cinfo->main;

  switch (pass__mode) {
  case JBUF__PASS__THRU:
    if (cinfo->upsample->need__context__rows) {
      main->pub.process__data = process__data__context__main;
      make__funny__pointers(cinfo); /* Create the xbuffer[] lists */
      main->whichptr = 0;	/* Read first iMCU row into xbuffer[0] */
      main->context__state = CTX__PREPARE__FOR__IMCU;
      main->iMCU__row__ctr = 0;
    } else {
      /* Simple case with no context needed */
      main->pub.process__data = process__data__simple__main;
    }
    main->buffer__full = FALSE;	/* Mark buffer empty */
    main->rowgroup__ctr = 0;
    break;
#ifdef QUANT__2PASS__SUPPORTED
  case JBUF__CRANK__DEST:
    /* For last pass of 2-pass quantization, just crank the postprocessor */
    main->pub.process__data = process__data__crank__post;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);
    break;
  }
}


/*
 * Process some data.
 * This handles the simple case where no context is required.
 */

METHODDEF(void)
process__data__simple__main (j__decompress__ptr cinfo,
			  JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
			  JDIMENSION out__rows__avail)
{
  my__main__ptr main = (my__main__ptr) cinfo->main;
  JDIMENSION rowgroups__avail;

  /* Read input data if we haven't filled the main buffer yet */
  if (! main->buffer__full) {
    if (! (*cinfo->coef->decompress__data) (cinfo, main->buffer))
      return;			/* suspension forced, can do nothing more */
    main->buffer__full = TRUE;	/* OK, we have an iMCU row to work with */
  }

  /* There are always min__DCT__scaled__size row groups in an iMCU row. */
  rowgroups__avail = (JDIMENSION) cinfo->min__DCT__scaled__size;
  /* Note: at the bottom of the image, we may pass extra garbage row groups
   * to the postprocessor.  The postprocessor has to check for bottom
   * of image anyway (at row resolution), so no point in us doing it too.
   */

  /* Feed the postprocessor */
  (*cinfo->post->post__process__data) (cinfo, main->buffer,
				     &main->rowgroup__ctr, rowgroups__avail,
				     output__buf, out__row__ctr, out__rows__avail);

  /* Has postprocessor consumed all the data yet? If so, mark buffer empty */
  if (main->rowgroup__ctr >= rowgroups__avail) {
    main->buffer__full = FALSE;
    main->rowgroup__ctr = 0;
  }
}


/*
 * Process some data.
 * This handles the case where context rows must be provided.
 */

METHODDEF(void)
process__data__context__main (j__decompress__ptr cinfo,
			   JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
			   JDIMENSION out__rows__avail)
{
  my__main__ptr main = (my__main__ptr) cinfo->main;

  /* Read input data if we haven't filled the main buffer yet */
  if (! main->buffer__full) {
    if (! (*cinfo->coef->decompress__data) (cinfo,
					   main->xbuffer[main->whichptr]))
      return;			/* suspension forced, can do nothing more */
    main->buffer__full = TRUE;	/* OK, we have an iMCU row to work with */
    main->iMCU__row__ctr++;	/* count rows received */
  }

  /* Postprocessor typically will not swallow all the input data it is handed
   * in one call (due to filling the output buffer first).  Must be prepared
   * to exit and restart.  This switch lets us keep track of how far we got.
   * Note that each case falls through to the next on successful completion.
   */
  switch (main->context__state) {
  case CTX__POSTPONED__ROW:
    /* Call postprocessor using previously set pointers for postponed row */
    (*cinfo->post->post__process__data) (cinfo, main->xbuffer[main->whichptr],
			&main->rowgroup__ctr, main->rowgroups__avail,
			output__buf, out__row__ctr, out__rows__avail);
    if (main->rowgroup__ctr < main->rowgroups__avail)
      return;			/* Need to suspend */
    main->context__state = CTX__PREPARE__FOR__IMCU;
    if (*out__row__ctr >= out__rows__avail)
      return;			/* Postprocessor exactly filled output buf */
    /*FALLTHROUGH*/
  case CTX__PREPARE__FOR__IMCU:
    /* Prepare to process first M-1 row groups of this iMCU row */
    main->rowgroup__ctr = 0;
    main->rowgroups__avail = (JDIMENSION) (cinfo->min__DCT__scaled__size - 1);
    /* Check for bottom of image: if so, tweak pointers to "duplicate"
     * the last sample row, and adjust rowgroups__avail to ignore padding rows.
     */
    if (main->iMCU__row__ctr == cinfo->total__iMCU__rows)
      set__bottom__pointers(cinfo);
    main->context__state = CTX__PROCESS__IMCU;
    /*FALLTHROUGH*/
  case CTX__PROCESS__IMCU:
    /* Call postprocessor using previously set pointers */
    (*cinfo->post->post__process__data) (cinfo, main->xbuffer[main->whichptr],
			&main->rowgroup__ctr, main->rowgroups__avail,
			output__buf, out__row__ctr, out__rows__avail);
    if (main->rowgroup__ctr < main->rowgroups__avail)
      return;			/* Need to suspend */
    /* After the first iMCU, change wraparound pointers to normal state */
    if (main->iMCU__row__ctr == 1)
      set__wraparound__pointers(cinfo);
    /* Prepare to load new iMCU row using other xbuffer list */
    main->whichptr ^= 1;	/* 0=>1 or 1=>0 */
    main->buffer__full = FALSE;
    /* Still need to process last row group of this iMCU row, */
    /* which is saved at index M+1 of the other xbuffer */
    main->rowgroup__ctr = (JDIMENSION) (cinfo->min__DCT__scaled__size + 1);
    main->rowgroups__avail = (JDIMENSION) (cinfo->min__DCT__scaled__size + 2);
    main->context__state = CTX__POSTPONED__ROW;
  }
  /*
  {
		static int t = 0;
		if(t==0)
		{
		CH_Printf("jdmainct : process__data__context__main3 {{");
		for(t = 0; t < 20; t++)
			CH_Printf("%d,", output__buf[0][t]);
		CH_Printf("}}");
		t++;
		}
}*/
}


/*
 * Process some data.
 * Final pass of two-pass quantization: just call the postprocessor.
 * Source data will be the postprocessor controller's internal buffer.
 */

#ifdef QUANT__2PASS__SUPPORTED

METHODDEF(void)
process__data__crank__post (j__decompress__ptr cinfo,
			 JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
			 JDIMENSION out__rows__avail)
{
  (*cinfo->post->post__process__data) (cinfo, (JSAMPIMAGE) NULL,
				     (JDIMENSION *) NULL, (JDIMENSION) 0,
				     output__buf, out__row__ctr, out__rows__avail);
}

#endif /* QUANT__2PASS__SUPPORTED */


/*
 * Initialize main buffer controller.
 */

GLOBAL(void)
jinit__d__main__controller (j__decompress__ptr cinfo, CH_BOOL need__full__buffer)
{
  my__main__ptr main;
  int ci, rgroup, ngroups;
  jpeg__component__info *compptr;

  main = (my__main__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__main__controller));
  cinfo->main = (struct jpeg__d__main__controller *) main;
  main->pub.start__pass = start__pass__main;

  if (need__full__buffer)		/* shouldn't happen */
    ERREXIT(cinfo, JERR__BAD__BUFFER__MODE);

  /* Allocate the workspace.
   * ngroups is the number of row groups we need.
   */
  if (cinfo->upsample->need__context__rows) {
    if (cinfo->min__DCT__scaled__size < 2) /* unsupported, see comments above */
      ERREXIT(cinfo, JERR__NOTIMPL);
    alloc__funny__pointers(cinfo); /* Alloc space for xbuffer[] lists */
    ngroups = cinfo->min__DCT__scaled__size + 2;
  } else {
    ngroups = cinfo->min__DCT__scaled__size;
  }

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    rgroup = (compptr->v__samp__factor * compptr->DCT__scaled__size) /
      cinfo->min__DCT__scaled__size; /* height of a row group of component */
    main->buffer[ci] = (*cinfo->mem->alloc__sarray)
			((j__common__ptr) cinfo, JPOOL__IMAGE,
			 compptr->width__in__blocks * compptr->DCT__scaled__size,
			 (JDIMENSION) (rgroup * ngroups));
  }
}
