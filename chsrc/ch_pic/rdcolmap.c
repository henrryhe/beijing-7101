/*
 * rdcolmap.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file implements djpeg's "-map file" switch.  It reads a source image
 * and constructs a colormap to be supplied to the JPEG decompressor.
 *
 * Currently, these file formats are supported for the map file:
 *   GIF: the contents of the GIF's global colormap are used.
 *   PPM (either text or raw flavor): the entire file is read and
 *      each unique pixel value is entered in the map.
 * Note that reading a large PPM file will be horrendously slow.
 * Typically, a PPM-format map file should contain just one pixel
 * of each desired color.  Such a file can be extracted from an
 * ordinary image PPM file with ppmtomap(1).
 *
 * Rescaling a PPM that has a maxval unequal to MAXJSAMPLE is not
 * currently implemented.
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */
#include "graphconfig.h"

#ifdef QUANT__2PASS__SUPPORTED	/* otherwise can't quantize to supplied map */

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


/*
 * Add a (potentially) new color to the color map.
 */

LOCAL(void)
add__map__entry (j__decompress__ptr cinfo, int R, int G, int B)
{
  JSAMPROW colormap0 = cinfo->colormap[0];
  JSAMPROW colormap1 = cinfo->colormap[1];
  JSAMPROW colormap2 = cinfo->colormap[2];
  int ncolors = cinfo->actual__number__of__colors;
  int index;

  /* Check for duplicate color. */
  for (index = 0; index < ncolors; index++) {
    if (GETJSAMPLE(colormap0[index]) == R &&
	GETJSAMPLE(colormap1[index]) == G &&
	GETJSAMPLE(colormap2[index]) == B)
      return;			/* color is already in map */
  }

  /* Check for map overflow. */
  if (ncolors >= (MAXJSAMPLE+1))
    ERREXIT1(cinfo, JERR__QUANT__MANY__COLORS, (MAXJSAMPLE+1));

  /* OK, add color to map. */
  colormap0[ncolors] = (JSAMPLE) R;
  colormap1[ncolors] = (JSAMPLE) G;
  colormap2[ncolors] = (JSAMPLE) B;
  cinfo->actual__number__of__colors++;
}


/*
 * Extract color map from a GIF file.
 */

LOCAL(void)
read__gif__map (j__decompress__ptr cinfo, graph_file * infile)
{
  int header[13];
  int i, colormaplen;
  int R, G, B;

  /* Initial 'G' has already been read by read__color__map */
  /* Read the rest of the GIF header and logical screen descriptor */
  for (i = 1; i < 13; i++) {
    if ((header[i] = graph_getc(infile)) == EOF)
      ERREXIT(cinfo, JERR__BAD__CMAP__FILE);
  }

  /* Verify GIF Header */
  if (header[1] != 'I' || header[2] != 'F')
    ERREXIT(cinfo, JERR__BAD__CMAP__FILE);

  /* There must be a global color map. */
  if ((header[10] & 0x80) == 0)
    ERREXIT(cinfo, JERR__BAD__CMAP__FILE);

  /* OK, fetch it. */
  colormaplen = 2 << (header[10] & 0x07);

  for (i = 0; i < colormaplen; i++) {
    R = graph_getc(infile);
    G = graph_getc(infile);
    B = graph_getc(infile);
    if (R == EOF || G == EOF || B == EOF)
      ERREXIT(cinfo, JERR__BAD__CMAP__FILE);
    add__map__entry(cinfo,
		  R << (BITS__IN__JSAMPLE-8),
		  G << (BITS__IN__JSAMPLE-8),
		  B << (BITS__IN__JSAMPLE-8));
  }
}


/* Support routines for reading PPM */


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
read__pbm__integer (j__decompress__ptr cinfo, graph_file * infile)
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
      ERREXIT(cinfo, JERR__BAD__CMAP__FILE);
  } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
  
  if (ch < '0' || ch > '9')
    ERREXIT(cinfo, JERR__BAD__CMAP__FILE);
  
  val = ch - '0';
  while ((ch = pbm__getc(infile)) >= '0' && ch <= '9') {
    val *= 10;
    val += ch - '0';
  }
  return val;
}


/*
 * Extract color map from a PPM file.
 */

LOCAL(void)
read__ppm__map (j__decompress__ptr cinfo, graph_file * infile)
{
  int c;
  unsigned int w, h, maxval, row, col;
  int R, G, B;

  /* Initial 'P' has already been read by read__color__map */
  c = graph_getc(infile);		/* save format discriminator for a sec */

  /* while we fetch the remaining header info */
  w = read__pbm__integer(cinfo, infile);
  h = read__pbm__integer(cinfo, infile);
  maxval = read__pbm__integer(cinfo, infile);

  if (w <= 0 || h <= 0 || maxval <= 0) /* error check */
    ERREXIT(cinfo, JERR__BAD__CMAP__FILE);

  /* For now, we don't support rescaling from an unusual maxval. */
  if (maxval != (unsigned int) MAXJSAMPLE)
    ERREXIT(cinfo, JERR__BAD__CMAP__FILE);

  switch (c) {
  case '3':			/* it's a text-format PPM file */
    for (row = 0; row < h; row++) {
      for (col = 0; col < w; col++) {
	R = read__pbm__integer(cinfo, infile);
	G = read__pbm__integer(cinfo, infile);
	B = read__pbm__integer(cinfo, infile);
	add__map__entry(cinfo, R, G, B);
      }
    }
    break;

  case '6':			/* it's a raw-format PPM file */
    for (row = 0; row < h; row++) {
      for (col = 0; col < w; col++) {
	R = graph_getc(infile);
	G = graph_getc(infile);
	B = graph_getc(infile);
	if (R == EOF || G == EOF || B == EOF)
	  ERREXIT(cinfo, JERR__BAD__CMAP__FILE);
	add__map__entry(cinfo, R, G, B);
      }
    }
    break;

  default:
    ERREXIT(cinfo, JERR__BAD__CMAP__FILE);
    break;
  }
}


/*
 * Main entry point from djpeg.c.
 *  Input: opened input file (from file name argument on command line).
 *  Output: colormap and actual__number__of__colors fields are set in cinfo.
 */

GLOBAL(void)
read__color__map (j__decompress__ptr cinfo, graph_file * infile)
{
  /* Allocate space for a color map of maximum supported size. */
  cinfo->colormap = (*cinfo->mem->alloc__sarray)
    ((j__common__ptr) cinfo, JPOOL__IMAGE,
     (JDIMENSION) (MAXJSAMPLE+1), (JDIMENSION) 3);
  cinfo->actual__number__of__colors = 0; /* initialize map to empty */

  /* Read first byte to determine file format */
  switch (graph_getc(infile)) {
  case 'G':
    read__gif__map(cinfo, infile);
    break;
  case 'P':
    read__ppm__map(cinfo, infile);
    break;
  default:
    ERREXIT(cinfo, JERR__BAD__CMAP__FILE);
    break;
  }
}

#endif /* QUANT__2PASS__SUPPORTED */
