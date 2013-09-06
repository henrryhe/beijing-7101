/*
 * rdrle.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in Utah RLE format.
 * The Utah Raster Toolkit library is required (version 3.1 or later).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; start__input may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed RLE format).
 *
 * Based on code contributed by Mike Lijewski,
 * with updates from Robert Hutchinson.
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

#ifdef RLE__SUPPORTED

/* rle.h is provided by the Utah Raster Toolkit. */

#include <rle.h>

/*
 * We assume that JSAMPLE has the same representation as rle__pixel,
 * to wit, "unsigned char".  Hence we can't cope with 12- or 16-bit samples.
 */

#if BITS__IN__JSAMPLE != 8
  Sorry, this code only copes with 8-bit JSAMPLEs. /* deliberate syntax err */
#endif

/*
 * We support the following types of RLE files:
 *   
 *   GRAYSCALE   - 8 bits, no colormap
 *   MAPPEDGRAY  - 8 bits, 1 channel colomap
 *   PSEUDOCOLOR - 8 bits, 3 channel colormap
 *   TRUECOLOR   - 24 bits, 3 channel colormap
 *   DIRECTCOLOR - 24 bits, no colormap
 *
 * For now, we ignore any alpha channel in the image.
 */

typedef enum
  { GRAYSCALE, MAPPEDGRAY, PSEUDOCOLOR, TRUECOLOR, DIRECTCOLOR } rle__kind;


/*
 * Since RLE stores scanlines bottom-to-top, we have to invert the image
 * to conform to JPEG's top-to-bottom order.  To do this, we read the
 * incoming image into a virtual array on the first get__pixel__rows call,
 * then fetch the required row from the virtual array on subsequent calls.
 */

typedef struct __rle__source__struct * rle__source__ptr;

typedef struct __rle__source__struct {
  struct cjpeg__source__struct pub; /* public fields */

  rle__kind visual;              /* actual type of input file */
  jvirt__sarray__ptr image;       /* virtual array to hold the image */
  JDIMENSION row;		/* current row # in the virtual array */
  rle__hdr header;               /* Input file information */
  rle__pixel** rle__row;          /* holds a row returned by rle__getrow() */

} rle__source__struct;


/*
 * Read the file header; return image size and component count.
 */

METHODDEF(void)
start__input__rle (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  rle__source__ptr source = (rle__source__ptr) sinfo;
  JDIMENSION width, height;
#ifdef PROGRESS__REPORT
  cd__progress__ptr progress = (cd__progress__ptr) cinfo->progress;
#endif

  /* Use RLE library routine to get the header info */
  source->header = *rle__hdr__init(NULL);
  source->header.rle__file = source->pub.input__file;
  switch (rle__get__setup(&(source->header))) {
  case RLE__SUCCESS:
    /* A-OK */
    break;
  case RLE__NOT__RLE:
    ERREXIT(cinfo, JERR__RLE__NOT);
    break;
  case RLE__NO__SPACE:
    ERREXIT(cinfo, JERR__RLE__MEM);
    break;
  case RLE__EMPTY:
    ERREXIT(cinfo, JERR__RLE__EMPTY);
    break;
  case RLE__EOF:
    ERREXIT(cinfo, JERR__RLE__EOF);
    break;
  default:
    ERREXIT(cinfo, JERR__RLE__BADERROR);
    break;
  }

  /* Figure out what we have, set private vars and return values accordingly */
  
  width  = source->header.xmax - source->header.xmin + 1;
  height = source->header.ymax - source->header.ymin + 1;
  source->header.xmin = 0;		/* realign horizontally */
  source->header.xmax = width-1;

  cinfo->image__width      = width;
  cinfo->image__height     = height;
  cinfo->data__precision   = 8;  /* we can only handle 8 bit data */

  if (source->header.ncolors == 1 && source->header.ncmap == 0) {
    source->visual     = GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC__RLE__GRAY, width, height);
  } else if (source->header.ncolors == 1 && source->header.ncmap == 1) {
    source->visual     = MAPPEDGRAY;
    TRACEMS3(cinfo, 1, JTRC__RLE__MAPGRAY, width, height,
             1 << source->header.cmaplen);
  } else if (source->header.ncolors == 1 && source->header.ncmap == 3) {
    source->visual     = PSEUDOCOLOR;
    TRACEMS3(cinfo, 1, JTRC__RLE__MAPPED, width, height,
	     1 << source->header.cmaplen);
  } else if (source->header.ncolors == 3 && source->header.ncmap == 3) {
    source->visual     = TRUECOLOR;
    TRACEMS3(cinfo, 1, JTRC__RLE__FULLMAP, width, height,
	     1 << source->header.cmaplen);
  } else if (source->header.ncolors == 3 && source->header.ncmap == 0) {
    source->visual     = DIRECTCOLOR;
    TRACEMS2(cinfo, 1, JTRC__RLE, width, height);
  } else
    ERREXIT(cinfo, JERR__RLE__UNSUPPORTED);
  
  if (source->visual == GRAYSCALE || source->visual == MAPPEDGRAY) {
    cinfo->in__color__space   = JCS__GRAYSCALE;
    cinfo->input__components = 1;
  } else {
    cinfo->in__color__space   = JCS__RGB;
    cinfo->input__components = 3;
  }

  /*
   * A place to hold each scanline while it's converted.
   * (GRAYSCALE scanlines don't need converting)
   */
  if (source->visual != GRAYSCALE) {
    source->rle__row = (rle__pixel**) (*cinfo->mem->alloc__sarray)
      ((j__common__ptr) cinfo, JPOOL__IMAGE,
       (JDIMENSION) width, (JDIMENSION) cinfo->input__components);
  }

  /* request a virtual array to hold the image */
  source->image = (*cinfo->mem->request__virt__sarray)
    ((j__common__ptr) cinfo, JPOOL__IMAGE, FALSE,
     (JDIMENSION) (width * source->header.ncolors),
     (JDIMENSION) height, (JDIMENSION) 1);

#ifdef PROGRESS__REPORT
  if (progress != NULL) {
    /* count file input as separate pass */
    progress->total__extra__passes++;
  }
#endif

  source->pub.buffer__height = 1;
}


/*
 * Read one row of pixels.
 * Called only after load__image has read the image into the virtual array.
 * Used for GRAYSCALE, MAPPEDGRAY, TRUECOLOR, and DIRECTCOLOR images.
 */

METHODDEF(JDIMENSION)
get__rle__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  rle__source__ptr source = (rle__source__ptr) sinfo;

  source->row--;
  source->pub.buffer = (*cinfo->mem->access__virt__sarray)
    ((j__common__ptr) cinfo, source->image, source->row, (JDIMENSION) 1, FALSE);

  return 1;
}

/*
 * Read one row of pixels.
 * Called only after load__image has read the image into the virtual array.
 * Used for PSEUDOCOLOR images.
 */

METHODDEF(JDIMENSION)
get__pseudocolor__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  rle__source__ptr source = (rle__source__ptr) sinfo;
  JSAMPROW src__row, dest__row;
  JDIMENSION col;
  rle__map *colormap;
  int val;

  colormap = source->header.cmap;
  dest__row = source->pub.buffer[0];
  source->row--;
  src__row = * (*cinfo->mem->access__virt__sarray)
    ((j__common__ptr) cinfo, source->image, source->row, (JDIMENSION) 1, FALSE);

  for (col = cinfo->image__width; col > 0; col--) {
    val = GETJSAMPLE(*src__row++);
    *dest__row++ = (JSAMPLE) (colormap[val      ] >> 8);
    *dest__row++ = (JSAMPLE) (colormap[val + 256] >> 8);
    *dest__row++ = (JSAMPLE) (colormap[val + 512] >> 8);
  }

  return 1;
}


/*
 * Load the image into a virtual array.  We have to do this because RLE
 * files start at the lower left while the JPEG standard has them starting
 * in the upper left.  This is called the first time we want to get a row
 * of input.  What we do is load the RLE data into the array and then call
 * the appropriate routine to read one row from the array.  Before returning,
 * we set source->pub.get__pixel__rows so that subsequent calls go straight to
 * the appropriate row-reading routine.
 */

METHODDEF(JDIMENSION)
load__image (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  rle__source__ptr source = (rle__source__ptr) sinfo;
  JDIMENSION row, col;
  JSAMPROW  scanline, red__ptr, green__ptr, blue__ptr;
  rle__pixel **rle__row;
  rle__map *colormap;
  char channel;
#ifdef PROGRESS__REPORT
  cd__progress__ptr progress = (cd__progress__ptr) cinfo->progress;
#endif

  colormap = source->header.cmap;
  rle__row = source->rle__row;

  /* Read the RLE data into our virtual array.
   * We assume here that (a) rle__pixel is represented the same as JSAMPLE,
   * and (b) we are not on a machine where FAR pointers differ from regular.
   */
  RLE__CLR__BIT(source->header, RLE__ALPHA); /* don't read the alpha channel */

#ifdef PROGRESS__REPORT
  if (progress != NULL) {
    progress->pub.pass__limit = cinfo->image__height;
    progress->pub.pass__counter = 0;
    (*progress->pub.progress__monitor) ((j__common__ptr) cinfo);
  }
#endif

  switch (source->visual) {

  case GRAYSCALE:
  case PSEUDOCOLOR:
    for (row = 0; row < cinfo->image__height; row++) {
      rle__row = (rle__pixel **) (*cinfo->mem->access__virt__sarray)
         ((j__common__ptr) cinfo, source->image, row, (JDIMENSION) 1, TRUE);
      rle__getrow(&source->header, rle__row);
#ifdef PROGRESS__REPORT
      if (progress != NULL) {
        progress->pub.pass__counter++;
        (*progress->pub.progress__monitor) ((j__common__ptr) cinfo);
      }
#endif
    }
    break;

  case MAPPEDGRAY:
  case TRUECOLOR:
    for (row = 0; row < cinfo->image__height; row++) {
      scanline = * (*cinfo->mem->access__virt__sarray)
        ((j__common__ptr) cinfo, source->image, row, (JDIMENSION) 1, TRUE);
      rle__row = source->rle__row;
      rle__getrow(&source->header, rle__row);

      for (col = 0; col < cinfo->image__width; col++) {
        for (channel = 0; channel < source->header.ncolors; channel++) {
          *scanline++ = (JSAMPLE)
            (colormap[GETJSAMPLE(rle__row[channel][col]) + 256 * channel] >> 8);
        }
      }

#ifdef PROGRESS__REPORT
      if (progress != NULL) {
        progress->pub.pass__counter++;
        (*progress->pub.progress__monitor) ((j__common__ptr) cinfo);
      }
#endif
    }
    break;

  case DIRECTCOLOR:
    for (row = 0; row < cinfo->image__height; row++) {
      scanline = * (*cinfo->mem->access__virt__sarray)
        ((j__common__ptr) cinfo, source->image, row, (JDIMENSION) 1, TRUE);
      rle__getrow(&source->header, rle__row);

      red__ptr   = rle__row[0];
      green__ptr = rle__row[1];
      blue__ptr  = rle__row[2];

      for (col = cinfo->image__width; col > 0; col--) {
        *scanline++ = *red__ptr++;
        *scanline++ = *green__ptr++;
        *scanline++ = *blue__ptr++;
      }

#ifdef PROGRESS__REPORT
      if (progress != NULL) {
        progress->pub.pass__counter++;
        (*progress->pub.progress__monitor) ((j__common__ptr) cinfo);
      }
#endif
    }
  }

#ifdef PROGRESS__REPORT
  if (progress != NULL)
    progress->completed__extra__passes++;
#endif

  /* Set up to call proper row-extraction routine in future */
  if (source->visual == PSEUDOCOLOR) {
    source->pub.buffer = source->rle__row;
    source->pub.get__pixel__rows = get__pseudocolor__row;
  } else {
    source->pub.get__pixel__rows = get__rle__row;
  }
  source->row = cinfo->image__height;

  /* And fetch the topmost (bottommost) row */
  return (*source->pub.get__pixel__rows) (cinfo, sinfo);   
}


/*
 * Finish up at the end of the file.
 */

METHODDEF(void)
finish__input__rle (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  /* no work */
}


/*
 * The module selection routine for RLE format input.
 */

GLOBAL(cjpeg__source__ptr)
jinit__read__rle (j__compress__ptr cinfo)
{
  rle__source__ptr source;

  /* Create module interface object */
  source = (rle__source__ptr)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
                                  SIZEOF(rle__source__struct));
  /* Fill in method ptrs */
  source->pub.start__input = start__input__rle;
  source->pub.finish__input = finish__input__rle;
  source->pub.get__pixel__rows = load__image;

  return (cjpeg__source__ptr) source;
}

#endif /* RLE__SUPPORTED */
