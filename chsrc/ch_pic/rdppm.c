/*
 * rdppm.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in PPM/PGM format.
 * The extended 2-byte-per-sample raw PPM/PGM formats are supported.
 * The PBMPLUS library is NOT required to compile this software
 * (but it is highly useful as a set of PPM image manipulation programs).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; start__input may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed PPM format).
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */
#include "graphconfig.h"

#ifdef PPM__SUPPORTED


/* Portions of this code are based on the PBMPLUS library, which is:
**
** Copyright (C) 1988 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/


/* Macros to deal with unsigned chars as efficiently as compiler allows */

#ifdef HAVE__UNSIGNED__CHAR
typedef unsigned char U__CHAR;
#define UCH(x)	((int) (x))
#else /* !HAVE__UNSIGNED__CHAR */
#ifdef CHAR__IS__UNSIGNED
typedef char U__CHAR;
#define UCH(x)	((int) (x))
#else
typedef char U__CHAR;
#define UCH(x)	((int) (x) & 0xFF)
#endif
#endif /* HAVE__UNSIGNED__CHAR */


#define	ReadOK(file,buffer,len)	(JFREAD(file,buffer,len) == ((size_t) (len)))


/*
 * On most systems, reading individual bytes with getc() is drastically less
 * efficient than buffering a row at a time with fread().  On PCs, we must
 * allocate the buffer in near data space, because we are assuming small-data
 * memory model, wherein fread() can't reach far memory.  If you need to
 * process very wide images on a PC, you might have to compile in large-memory
 * model, or else replace fread() with a getc() loop --- which will be much
 * slower.
 */


/* Private version of data source object */

typedef struct {
  struct cjpeg__source__struct pub; /* public fields */

  U__CHAR *iobuffer;		/* non-FAR pointer to I/O buffer */
  JSAMPROW pixrow;		/* FAR pointer to same */
  size_t buffer__width;		/* width of I/O buffer */
  JSAMPLE *rescale;		/* => maxval-remapping array, or NULL */
} ppm__source__struct;

typedef ppm__source__struct * ppm__source__ptr;


LOCAL(int)
pbm__getc (graph_file * infile)
/* Read next char, skipping over any comments */
/* A comment/newline sequence is returned as a newline */
{
  register int ch;

  ch = graph_getc(infile);
  if (ch == '#') {
    do {
      ch = graph_getc(infile);
    } while (ch != '\n' && ch != EOF);
  }
  return ch;
}


LOCAL(unsigned int)
read__pbm__integer (j__compress__ptr cinfo, graph_file * infile)
/* Read an unsigned decimal integer from the PPM file */
/* Swallows one trailing character after the integer */
/* Note that on a 16-bit-int machine, only values up to 64k can be read. */
/* This should not be a problem in practice. */
{
  register int ch;
  register unsigned int val;

  /* Skip any leading whitespace */
  do {
    ch = pbm__getc(infile);
    if (ch == EOF)
      ERREXIT(cinfo, JERR__INPUT__EOF);
  } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

  if (ch < '0' || ch > '9')
    ERREXIT(cinfo, JERR__PPM__NONNUMERIC);

  val = ch - '0';
  while ((ch = pbm__getc(infile)) >= '0' && ch <= '9') {
    val *= 10;
    val += ch - '0';
  }
  return val;
}


/*
 * Read one row of pixels.
 *
 * We provide several different versions depending on input file format.
 * In all cases, input is scaled to the size of JSAMPLE.
 *
 * A really fast path is provided for reading byte/sample raw files with
 * maxval = MAXJSAMPLE, which is the normal case for 8-bit data.
 */


METHODDEF(JDIMENSION)
get__text__gray__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading text-format PGM files with any maxval */
{
  ppm__source__ptr source = (ppm__source__ptr) sinfo;
  graph_file * infile = source->pub.input__file;
  register JSAMPROW ptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image__width; col > 0; col--) {
    *ptr++ = rescale[read__pbm__integer(cinfo, infile)];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get__text__rgb__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading text-format PPM files with any maxval */
{
  ppm__source__ptr source = (ppm__source__ptr) sinfo;
  graph_file * infile = source->pub.input__file;
  register JSAMPROW ptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image__width; col > 0; col--) {
    *ptr++ = rescale[read__pbm__integer(cinfo, infile)];
    *ptr++ = rescale[read__pbm__integer(cinfo, infile)];
    *ptr++ = rescale[read__pbm__integer(cinfo, infile)];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get__scaled__gray__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading raw-byte-format PGM files with any maxval */
{
  ppm__source__ptr source = (ppm__source__ptr) sinfo;
  register JSAMPROW ptr;
  register U__CHAR * bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  if (! ReadOK(source->pub.input__file, source->iobuffer, source->buffer__width))
    ERREXIT(cinfo, JERR__INPUT__EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image__width; col > 0; col--) {
    *ptr++ = rescale[UCH(*bufferptr++)];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get__scaled__rgb__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading raw-byte-format PPM files with any maxval */
{
  ppm__source__ptr source = (ppm__source__ptr) sinfo;
  register JSAMPROW ptr;
  register U__CHAR * bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  if (! ReadOK(source->pub.input__file, source->iobuffer, source->buffer__width))
    ERREXIT(cinfo, JERR__INPUT__EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image__width; col > 0; col--) {
    *ptr++ = rescale[UCH(*bufferptr++)];
    *ptr++ = rescale[UCH(*bufferptr++)];
    *ptr++ = rescale[UCH(*bufferptr++)];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get__raw__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading raw-byte-format files with maxval = MAXJSAMPLE.
 * In this case we just read right into the JSAMPLE buffer!
 * Note that same code works for PPM and PGM files.
 */
{
  ppm__source__ptr source = (ppm__source__ptr) sinfo;

  if (! ReadOK(source->pub.input__file, source->iobuffer, source->buffer__width))
    ERREXIT(cinfo, JERR__INPUT__EOF);
  return 1;
}


METHODDEF(JDIMENSION)
get__word__gray__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading raw-word-format PGM files with any maxval */
{
  ppm__source__ptr source = (ppm__source__ptr) sinfo;
  register JSAMPROW ptr;
  register U__CHAR * bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  if (! ReadOK(source->pub.input__file, source->iobuffer, source->buffer__width))
    ERREXIT(cinfo, JERR__INPUT__EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image__width; col > 0; col--) {
    register int temp;
    temp  = UCH(*bufferptr++);
    temp |= UCH(*bufferptr++) << 8;
    *ptr++ = rescale[temp];
  }
  return 1;
}


METHODDEF(JDIMENSION)
get__word__rgb__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading raw-word-format PPM files with any maxval */
{
  ppm__source__ptr source = (ppm__source__ptr) sinfo;
  register JSAMPROW ptr;
  register U__CHAR * bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  if (! ReadOK(source->pub.input__file, source->iobuffer, source->buffer__width))
    ERREXIT(cinfo, JERR__INPUT__EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image__width; col > 0; col--) {
    register int temp;
    temp  = UCH(*bufferptr++);
    temp |= UCH(*bufferptr++) << 8;
    *ptr++ = rescale[temp];
    temp  = UCH(*bufferptr++);
    temp |= UCH(*bufferptr++) << 8;
    *ptr++ = rescale[temp];
    temp  = UCH(*bufferptr++);
    temp |= UCH(*bufferptr++) << 8;
    *ptr++ = rescale[temp];
  }
  return 1;
}


/*
 * Read the file header; return image size and component count.
 */

METHODDEF(void)
start__input__ppm (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  ppm__source__ptr source = (ppm__source__ptr) sinfo;
  int c;
  unsigned int w, h, maxval;
  CH_BOOL need__iobuffer, use__raw__buffer, need__rescale;

  if (graph_getc(source->pub.input__file) != 'P')
    ERREXIT(cinfo, JERR__PPM__NOT);

  c = graph_getc(source->pub.input__file); /* subformat discriminator character */

  /* detect unsupported variants (ie, PBM) before trying to read header */
  switch (c) {
  case '2':			/* it's a text-format PGM file */
  case '3':			/* it's a text-format PPM file */
  case '5':			/* it's a raw-format PGM file */
  case '6':			/* it's a raw-format PPM file */
    break;
  default:
    ERREXIT(cinfo, JERR__PPM__NOT);
    break;
  }

  /* fetch the remaining header info */
  w = read__pbm__integer(cinfo, source->pub.input__file);
  h = read__pbm__integer(cinfo, source->pub.input__file);
  maxval = read__pbm__integer(cinfo, source->pub.input__file);

  if (w <= 0 || h <= 0 || maxval <= 0) /* error check */
    ERREXIT(cinfo, JERR__PPM__NOT);

  cinfo->data__precision = BITS__IN__JSAMPLE; /* we always rescale data to this */
  cinfo->image__width = (JDIMENSION) w;
  cinfo->image__height = (JDIMENSION) h;

  /* initialize flags to most common settings */
  need__iobuffer = TRUE;		/* do we need an I/O buffer? */
  use__raw__buffer = FALSE;	/* do we map input buffer onto I/O buffer? */
  need__rescale = TRUE;		/* do we need a rescale array? */

  switch (c) {
  case '2':			/* it's a text-format PGM file */
    cinfo->input__components = 1;
    cinfo->in__color__space = JCS__GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC__PGM__TEXT, w, h);
    source->pub.get__pixel__rows = get__text__gray__row;
    need__iobuffer = FALSE;
    break;

  case '3':			/* it's a text-format PPM file */
    cinfo->input__components = 3;
    cinfo->in__color__space = JCS__RGB;
    TRACEMS2(cinfo, 1, JTRC__PPM__TEXT, w, h);
    source->pub.get__pixel__rows = get__text__rgb__row;
    need__iobuffer = FALSE;
    break;

  case '5':			/* it's a raw-format PGM file */
    cinfo->input__components = 1;
    cinfo->in__color__space = JCS__GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC__PGM, w, h);
    if (maxval > 255) {
      source->pub.get__pixel__rows = get__word__gray__row;
    } else if (maxval == MAXJSAMPLE && SIZEOF(JSAMPLE) == SIZEOF(U__CHAR)) {
      source->pub.get__pixel__rows = get__raw__row;
      use__raw__buffer = TRUE;
      need__rescale = FALSE;
    } else {
      source->pub.get__pixel__rows = get__scaled__gray__row;
    }
    break;

  case '6':			/* it's a raw-format PPM file */
    cinfo->input__components = 3;
    cinfo->in__color__space = JCS__RGB;
    TRACEMS2(cinfo, 1, JTRC__PPM, w, h);
    if (maxval > 255) {
      source->pub.get__pixel__rows = get__word__rgb__row;
    } else if (maxval == MAXJSAMPLE && SIZEOF(JSAMPLE) == SIZEOF(U__CHAR)) {
      source->pub.get__pixel__rows = get__raw__row;
      use__raw__buffer = TRUE;
      need__rescale = FALSE;
    } else {
      source->pub.get__pixel__rows = get__scaled__rgb__row;
    }
    break;
  }

  /* Allocate space for I/O buffer: 1 or 3 bytes or words/pixel. */
  if (need__iobuffer) {
    source->buffer__width = (size_t) w * cinfo->input__components *
      ((maxval<=255) ? SIZEOF(U__CHAR) : (2*SIZEOF(U__CHAR)));
    source->iobuffer = (U__CHAR *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  source->buffer__width);
  }

  /* Create compressor input buffer. */
  if (use__raw__buffer) {
    /* For unscaled raw-input case, we can just map it onto the I/O buffer. */
    /* Synthesize a JSAMPARRAY pointer structure */
    /* Cast here implies near->far pointer conversion on PCs */
    source->pixrow = (JSAMPROW) source->iobuffer;
    source->pub.buffer = & source->pixrow;
    source->pub.buffer__height = 1;
  } else {
    /* Need to translate anyway, so make a separate sample buffer. */
    source->pub.buffer = (*cinfo->mem->alloc__sarray)
      ((j__common__ptr) cinfo, JPOOL__IMAGE,
       (JDIMENSION) w * cinfo->input__components, (JDIMENSION) 1);
    source->pub.buffer__height = 1;
  }

  /* Compute the rescaling array if required. */
  if (need__rescale) {
    INT32 val, half__maxval;

    /* On 16-bit-int machines we have to be careful of maxval = 65535 */
    source->rescale = (JSAMPLE *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  (size_t) (((long) maxval + 1L) * SIZEOF(JSAMPLE)));
    half__maxval = maxval / 2;
    for (val = 0; val <= (INT32) maxval; val++) {
      /* The multiplication here must be done in 32 bits to avoid overflow */
      source->rescale[val] = (JSAMPLE) ((val*MAXJSAMPLE + half__maxval)/maxval);
    }
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF(void)
finish__input__ppm (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  /* no work */
}


/*
 * The module selection routine for PPM format input.
 */

GLOBAL(cjpeg__source__ptr)
jinit__read__ppm (j__compress__ptr cinfo)
{
  ppm__source__ptr source;

  /* Create module interface object */
  source = (ppm__source__ptr)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  SIZEOF(ppm__source__struct));
  /* Fill in method ptrs, except get__pixel__rows which start__input sets */
  source->pub.start__input = start__input__ppm;
  source->pub.finish__input = finish__input__ppm;

  return (cjpeg__source__ptr) source;
}

#endif /* PPM__SUPPORTED */
