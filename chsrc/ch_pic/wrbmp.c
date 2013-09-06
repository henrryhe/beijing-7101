/*
 * wrbmp.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write output images in Microsoft "BMP"
 * format (MS Windows 3.x and OS/2 1.x flavors).
 * Either 8-bit colormapped or 24-bit full-color format can be written.
 * No compression is supported.
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.
 *
 * This code contributed by James Arthur Boucher.
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

#ifdef BMP__SUPPORTED


/*
 * To support 12-bit JPEG data, we'd have to scale output down to 8 bits.
 * This is not yet implemented.
 */

#if BITS__IN__JSAMPLE != 8
  Sorry, this code only copes with 8-bit JSAMPLEs. /* deliberate syntax err */
#endif

/*
 * Since BMP stores scanlines bottom-to-top, we have to invert the image
 * from JPEG's top-to-bottom order.  To do this, we save the outgoing data
 * in a virtual array during put__pixel__row calls, then actually emit the
 * BMP file during finish__output.  The virtual array contains one JSAMPLE per
 * pixel if the output is grayscale or colormapped, three if it is full color.
 */

/* Private version of data destination object */

typedef struct {
  struct djpeg__dest__struct pub;	/* public fields */

  CH_BOOL is__os2;		/* saves the OS2 format request flag */

  jvirt__sarray__ptr whole__image;	/* needed to reverse row order */
  JDIMENSION data__width;	/* JSAMPLEs per row */
  JDIMENSION row__width;		/* physical width of one row in the BMP file */
  int pad__bytes;		/* number of padding bytes needed per row */
  JDIMENSION cur__output__row;	/* next row# to write to virtual array */
} bmp__dest__struct;

typedef bmp__dest__struct * bmp__dest__ptr;


/* Forward declarations */
LOCAL(void) write__colormap
	JPP((j__decompress__ptr cinfo, bmp__dest__ptr dest,
	     int map__colors, int map__entry__size));


/*
 * Write some pixel data.
 * In this module rows__supplied will always be 1.
 */

METHODDEF(void)
put__pixel__rows (j__decompress__ptr cinfo, djpeg__dest__ptr dinfo,
		JDIMENSION rows__supplied)
/* This version is for writing 24-bit pixels */
{
  bmp__dest__ptr dest = (bmp__dest__ptr) dinfo;
  JSAMPARRAY image__ptr;
  register JSAMPROW inptr, outptr;
  register JDIMENSION col;
  int pad;

  /* Access next row in virtual array */
  image__ptr = (*cinfo->mem->access__virt__sarray)
    ((j__common__ptr) cinfo, dest->whole__image,
     dest->cur__output__row, (JDIMENSION) 1, TRUE);
  dest->cur__output__row++;

  /* Transfer data.  Note destination values must be in BGR order
   * (even though Microsoft's own documents say the opposite).
   */
  inptr = dest->pub.buffer[0];
  outptr = image__ptr[0];
  for (col = cinfo->output__width; col > 0; col--) {
    outptr[2] = *inptr++;	/* can omit GETJSAMPLE() safely */
    outptr[1] = *inptr++;
    outptr[0] = *inptr++;
    outptr += 3;
  }

  /* Zero out the pad bytes. */
  pad = dest->pad__bytes;
  while (--pad >= 0)
    *outptr++ = 0;
}

METHODDEF(void)
put__gray__rows (j__decompress__ptr cinfo, djpeg__dest__ptr dinfo,
	       JDIMENSION rows__supplied)
/* This version is for grayscale OR quantized color output */
{
  bmp__dest__ptr dest = (bmp__dest__ptr) dinfo;
  JSAMPARRAY image__ptr;
  register JSAMPROW inptr, outptr;
  register JDIMENSION col;
  int pad;

  /* Access next row in virtual array */
  image__ptr = (*cinfo->mem->access__virt__sarray)
    ((j__common__ptr) cinfo, dest->whole__image,
     dest->cur__output__row, (JDIMENSION) 1, TRUE);
  dest->cur__output__row++;

  /* Transfer data. */
  inptr = dest->pub.buffer[0];
  outptr = image__ptr[0];
  for (col = cinfo->output__width; col > 0; col--) {
    *outptr++ = *inptr++;	/* can omit GETJSAMPLE() safely */
  }

  /* Zero out the pad bytes. */
  pad = dest->pad__bytes;
  while (--pad >= 0)
    *outptr++ = 0;
}


/*
 * Startup: normally writes the file header.
 * In this module we may as well postpone everything until finish__output.
 */

METHODDEF(void)
start__output__bmp (j__decompress__ptr cinfo, djpeg__dest__ptr dinfo)
{
  /* no work here */
}


/*
 * Finish up at the end of the file.
 *
 * Here is where we really output the BMP file.
 *
 * First, routines to write the Windows and OS/2 variants of the file header.
 */

LOCAL(void)
write__bmp__header (j__decompress__ptr cinfo, bmp__dest__ptr dest)
/* Write a Windows-style BMP file header, including colormap if needed */
{
  char bmpfileheader[14];
  char bmpinfoheader[40];
#define PUT__2B(array,offset,value)  \
	(array[offset] = (char) ((value) & 0xFF), \
	 array[offset+1] = (char) (((value) >> 8) & 0xFF))
#define PUT__4B(array,offset,value)  \
	(array[offset] = (char) ((value) & 0xFF), \
	 array[offset+1] = (char) (((value) >> 8) & 0xFF), \
	 array[offset+2] = (char) (((value) >> 16) & 0xFF), \
	 array[offset+3] = (char) (((value) >> 24) & 0xFF))
  INT32 headersize, bfSize;
  int bits__per__pixel, cmap__entries;

  /* Compute colormap size and total file size */
  if (cinfo->out__color__space == JCS__RGB) {
    if (cinfo->quantize__colors) {
      /* Colormapped RGB */
      bits__per__pixel = 8;
      cmap__entries = 256;
    } else {
      /* Unquantized, full color RGB */
      bits__per__pixel = 24;
      cmap__entries = 0;
    }
  } else {
    /* Grayscale output.  We need to fake a 256-entry colormap. */
    bits__per__pixel = 8;
    cmap__entries = 256;
  }
  /* File size */
  headersize = 14 + 40 + cmap__entries * 4; /* Header and colormap */
  bfSize = headersize + (INT32) dest->row__width * (INT32) cinfo->output__height;
  
  /* Set unused fields of header to 0 */
  MEMZERO(bmpfileheader, SIZEOF(bmpfileheader));
  MEMZERO(bmpinfoheader, SIZEOF(bmpinfoheader));

  /* Fill the file header */
  bmpfileheader[0] = 0x42;	/* first 2 bytes are ASCII 'B', 'M' */
  bmpfileheader[1] = 0x4D;
  PUT__4B(bmpfileheader, 2, bfSize); /* bfSize */
  /* we leave bfReserved1 & bfReserved2 = 0 */
  PUT__4B(bmpfileheader, 10, headersize); /* bfOffBits */

  /* Fill the info header (Microsoft calls this a BITMAPINFOHEADER) */
  PUT__2B(bmpinfoheader, 0, 40);	/* biSize */
  PUT__4B(bmpinfoheader, 4, cinfo->output__width); /* biWidth */
  PUT__4B(bmpinfoheader, 8, cinfo->output__height); /* biHeight */
  PUT__2B(bmpinfoheader, 12, 1);	/* biPlanes - must be 1 */
  PUT__2B(bmpinfoheader, 14, bits__per__pixel); /* biBitCount */
  /* we leave biCompression = 0, for none */
  /* we leave biSizeImage = 0; this is correct for uncompressed data */
  if (cinfo->density__unit == 2) { /* if have density in dots/cm, then */
    PUT__4B(bmpinfoheader, 24, (INT32) (cinfo->X__density*100)); /* XPels/M */
    PUT__4B(bmpinfoheader, 28, (INT32) (cinfo->Y__density*100)); /* XPels/M */
  }
  PUT__2B(bmpinfoheader, 32, cmap__entries); /* biClrUsed */
  /* we leave biClrImportant = 0 */

  if (JFWRITE(dest->pub.output__file, bmpfileheader, 14) != (size_t) 14)
    ERREXIT(cinfo, JERR__FILE__WRITE);
  if (JFWRITE(dest->pub.output__file, bmpinfoheader, 40) != (size_t) 40)
    ERREXIT(cinfo, JERR__FILE__WRITE);

  if (cmap__entries > 0)
    write__colormap(cinfo, dest, cmap__entries, 4);
}


LOCAL(void)
write__os2__header (j__decompress__ptr cinfo, bmp__dest__ptr dest)
/* Write an OS2-style BMP file header, including colormap if needed */
{
  char bmpfileheader[14];
  char bmpcoreheader[12];
  INT32 headersize, bfSize;
  int bits__per__pixel, cmap__entries;

  /* Compute colormap size and total file size */
  if (cinfo->out__color__space == JCS__RGB) {
    if (cinfo->quantize__colors) {
      /* Colormapped RGB */
      bits__per__pixel = 8;
      cmap__entries = 256;
    } else {
      /* Unquantized, full color RGB */
      bits__per__pixel = 24;
      cmap__entries = 0;
    }
  } else {
    /* Grayscale output.  We need to fake a 256-entry colormap. */
    bits__per__pixel = 8;
    cmap__entries = 256;
  }
  /* File size */
  headersize = 14 + 12 + cmap__entries * 3; /* Header and colormap */
  bfSize = headersize + (INT32) dest->row__width * (INT32) cinfo->output__height;
  
  /* Set unused fields of header to 0 */
  MEMZERO(bmpfileheader, SIZEOF(bmpfileheader));
  MEMZERO(bmpcoreheader, SIZEOF(bmpcoreheader));

  /* Fill the file header */
  bmpfileheader[0] = 0x42;	/* first 2 bytes are ASCII 'B', 'M' */
  bmpfileheader[1] = 0x4D;
  PUT__4B(bmpfileheader, 2, bfSize); /* bfSize */
  /* we leave bfReserved1 & bfReserved2 = 0 */
  PUT__4B(bmpfileheader, 10, headersize); /* bfOffBits */

  /* Fill the info header (Microsoft calls this a BITMAPCOREHEADER) */
  PUT__2B(bmpcoreheader, 0, 12);	/* bcSize */
  PUT__2B(bmpcoreheader, 4, cinfo->output__width); /* bcWidth */
  PUT__2B(bmpcoreheader, 6, cinfo->output__height); /* bcHeight */
  PUT__2B(bmpcoreheader, 8, 1);	/* bcPlanes - must be 1 */
  PUT__2B(bmpcoreheader, 10, bits__per__pixel); /* bcBitCount */

  if (JFWRITE(dest->pub.output__file, bmpfileheader, 14) != (size_t) 14)
    ERREXIT(cinfo, JERR__FILE__WRITE);
  if (JFWRITE(dest->pub.output__file, bmpcoreheader, 12) != (size_t) 12)
    ERREXIT(cinfo, JERR__FILE__WRITE);

  if (cmap__entries > 0)
    write__colormap(cinfo, dest, cmap__entries, 3);
}


/*
 * Write the colormap.
 * Windows uses BGR0 map entries; OS/2 uses BGR entries.
 */

LOCAL(void)
write__colormap (j__decompress__ptr cinfo, bmp__dest__ptr dest,
		int map__colors, int map__entry__size)
{
  JSAMPARRAY colormap = cinfo->colormap;
  int num__colors = cinfo->actual__number__of__colors;
  graph_file * outfile = dest->pub.output__file;
  int i;

  if (colormap != NULL) {
    if (cinfo->out__color__components == 3) {
      /* Normal case with RGB colormap */
      for (i = 0; i < num__colors; i++) {
	putc(GETJSAMPLE(colormap[2][i]), outfile);
	putc(GETJSAMPLE(colormap[1][i]), outfile);
	putc(GETJSAMPLE(colormap[0][i]), outfile);
	if (map__entry__size == 4)
	  putc(0, outfile);
      }
    } else {
      /* Grayscale colormap (only happens with grayscale quantization) */
      for (i = 0; i < num__colors; i++) {
	putc(GETJSAMPLE(colormap[0][i]), outfile);
	putc(GETJSAMPLE(colormap[0][i]), outfile);
	putc(GETJSAMPLE(colormap[0][i]), outfile);
	if (map__entry__size == 4)
	  putc(0, outfile);
      }
    }
  } else {
    /* If no colormap, must be grayscale data.  Generate a linear "map". */
    for (i = 0; i < 256; i++) {
      putc(i, outfile);
      putc(i, outfile);
      putc(i, outfile);
      if (map__entry__size == 4)
	putc(0, outfile);
    }
  }
  /* Pad colormap with zeros to ensure specified number of colormap entries */ 
  if (i > map__colors)
    ERREXIT1(cinfo, JERR__TOO__MANY__COLORS, i);
  for (; i < map__colors; i++) {
    putc(0, outfile);
    putc(0, outfile);
    putc(0, outfile);
    if (map__entry__size == 4)
      putc(0, outfile);
  }
}


METHODDEF(void)
finish__output__bmp (j__decompress__ptr cinfo, djpeg__dest__ptr dinfo)
{
  bmp__dest__ptr dest = (bmp__dest__ptr) dinfo;
  register graph_file * outfile = dest->pub.output__file;
  JSAMPARRAY image__ptr;
  register JSAMPROW data__ptr;
  JDIMENSION row;
  register JDIMENSION col;
  cd__progress__ptr progress = (cd__progress__ptr) cinfo->progress;

  /* Write the header and colormap */
  if (dest->is__os2)
    write__os2__header(cinfo, dest);
  else
    write__bmp__header(cinfo, dest);

  /* Write the file body from our virtual array */
  for (row = cinfo->output__height; row > 0; row--) {
    if (progress != NULL) {
      progress->pub.pass__counter = (long) (cinfo->output__height - row);
      progress->pub.pass__limit = (long) cinfo->output__height;
      (*progress->pub.progress__monitor) ((j__common__ptr) cinfo);
    }
    image__ptr = (*cinfo->mem->access__virt__sarray)
      ((j__common__ptr) cinfo, dest->whole__image, row-1, (JDIMENSION) 1, FALSE);
    data__ptr = image__ptr[0];
    for (col = dest->row__width; col > 0; col--) {
      putc(GETJSAMPLE(*data__ptr), outfile);
      data__ptr++;
    }
  }
  if (progress != NULL)
    progress->completed__extra__passes++;

  /* Make sure we wrote the output file OK */
  fflush(outfile);
  if (ferror(outfile))
    ERREXIT(cinfo, JERR__FILE__WRITE);
}


/*
 * The module selection routine for BMP format output.
 */

GLOBAL(djpeg__dest__ptr)
jinit__write__bmp (j__decompress__ptr cinfo, CH_BOOL is__os2)
{
  bmp__dest__ptr dest;
  JDIMENSION row__width;

  /* Create module interface object, fill in method pointers */
  dest = (bmp__dest__ptr)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  SIZEOF(bmp__dest__struct));
  dest->pub.start__output = start__output__bmp;
  dest->pub.finish__output = finish__output__bmp;
  dest->is__os2 = is__os2;

  if (cinfo->out__color__space == JCS__GRAYSCALE) {
    dest->pub.put__pixel__rows = put__gray__rows;
  } else if (cinfo->out__color__space == JCS__RGB) {
    if (cinfo->quantize__colors)
      dest->pub.put__pixel__rows = put__gray__rows;
    else
      dest->pub.put__pixel__rows = put__pixel__rows;
  } else {
    ERREXIT(cinfo, JERR__BMP__COLORSPACE);
  }

  /* Calculate output image dimensions so we can allocate space */
  jpeg__calc__output__dimensions(cinfo);

  /* Determine width of rows in the BMP file (padded to 4-byte boundary). */
  row__width = cinfo->output__width * cinfo->output__components;
  dest->data__width = row__width;
  while ((row__width & 3) != 0) row__width++;
  dest->row__width = row__width;
  dest->pad__bytes = (int) (row__width - dest->data__width);

  /* Allocate space for inversion array, prepare for write pass */
  dest->whole__image = (*cinfo->mem->request__virt__sarray)
    ((j__common__ptr) cinfo, JPOOL__IMAGE, FALSE,
     row__width, cinfo->output__height, (JDIMENSION) 1);
  dest->cur__output__row = 0;
  if (cinfo->progress != NULL) {
    cd__progress__ptr progress = (cd__progress__ptr) cinfo->progress;
    progress->total__extra__passes++; /* count file input as separate pass */
  }

  /* Create decompressor output buffer. */
  dest->pub.buffer = (*cinfo->mem->alloc__sarray)
    ((j__common__ptr) cinfo, JPOOL__IMAGE, row__width, (JDIMENSION) 1);
  dest->pub.buffer__height = 1;

  return (djpeg__dest__ptr) dest;
}

#endif /* BMP__SUPPORTED */
