/*
 * rdbmp.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in Microsoft "BMP"
 * format (MS Windows 3.x, OS/2 1.x, and OS/2 2.x flavors).
 * Currently, only 8-bit and 24-bit images are supported, not 1-bit or
 * 4-bit (feeding such low-depth images into JPEG would be silly anyway).
 * Also, we don't support RLE-compressed files.
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; start__input may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed BMP format).
 *
 * This code contributed by James Arthur Boucher.
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */
#include "graphconfig.h"

#ifdef BMP__SUPPORTED


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


/* Private version of data source object */

typedef struct __bmp__source__struct * bmp__source__ptr;

typedef struct __bmp__source__struct {
 struct cjpeg__source__struct pub; /* public fields */
 
 j__compress__ptr cinfo;		/* back link saves passing separate parm */
 JSAMPARRAY colormap;		/* BMP colormap (converted to my format) */
 
 jvirt__sarray__ptr whole__image;	/* Needed to reverse row order */
 JDIMENSION source__row;	/* Current source row number */
 JDIMENSION row__width;		/* Physical width of scanlines in file */

  int bits__per__pixel;		/* remembers 8- or 24-bit format */
} bmp__source__struct;


LOCAL(int)
read__byte (bmp__source__ptr sinfo)
/* Read next byte from BMP file */
{
  register FILE *infile = sinfo->pub.input__file;
  register int c;

  if ((c = getc(infile)) == EOF)
    ERREXIT(sinfo->cinfo, JERR__INPUT__EOF);
  return c;
}


LOCAL(void)
read__colormap (bmp__source__ptr sinfo, int cmaplen, int mapentrysize)
/* Read the colormap from a BMP file */
{
  int i;

  switch (mapentrysize) {
  case 3:
    /* BGR format (occurs in OS/2 files) */
    for (i = 0; i < cmaplen; i++) {
      sinfo->colormap[2][i] = (JSAMPLE) read__byte(sinfo);
      sinfo->colormap[1][i] = (JSAMPLE) read__byte(sinfo);
      sinfo->colormap[0][i] = (JSAMPLE) read__byte(sinfo);
    }
    break;
  case 4:
    /* BGR0 format (occurs in MS Windows files) */
    for (i = 0; i < cmaplen; i++) {
      sinfo->colormap[2][i] = (JSAMPLE) read__byte(sinfo);
      sinfo->colormap[1][i] = (JSAMPLE) read__byte(sinfo);
      sinfo->colormap[0][i] = (JSAMPLE) read__byte(sinfo);
      (void) read__byte(sinfo);
    }
    break;
  default:
    ERREXIT(sinfo->cinfo, JERR__BMP__BADCMAP);
    break;
  }
}


/*
 * Read one row of pixels.
 * The image has been read into the whole__image array, but is otherwise
 * unprocessed.  We must read it out in top-to-bottom row order, and if
 * it is an 8-bit image, we must expand colormapped pixels to 24bit format.
 */

METHODDEF(JDIMENSION)
get__8bit__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading 8-bit colormap indexes */
{
  bmp__source__ptr source = (bmp__source__ptr) sinfo;
  register JSAMPARRAY colormap = source->colormap;
  JSAMPARRAY image__ptr;
  register int t;
  register JSAMPROW inptr, outptr;
  register JDIMENSION col;

  /* Fetch next row from virtual array */
  source->source__row--;
  image__ptr = (*cinfo->mem->access__virt__sarray)
    ((j__common__ptr) cinfo, source->whole__image,
     source->source__row, (JDIMENSION) 1, FALSE);

  /* Expand the colormap indexes to real data */
  inptr = image__ptr[0];
  outptr = source->pub.buffer[0];
  for (col = cinfo->image__width; col > 0; col--) {
    t = GETJSAMPLE(*inptr++);
    *outptr++ = colormap[0][t];	/* can omit GETJSAMPLE() safely */
    *outptr++ = colormap[1][t];
    *outptr++ = colormap[2][t];
  }

  return 1;
}


METHODDEF(JDIMENSION)
get__24bit__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading 24-bit pixels */
{
  bmp__source__ptr source = (bmp__source__ptr) sinfo;
  JSAMPARRAY image__ptr;
  register JSAMPROW inptr, outptr;
  register JDIMENSION col;

  /* Fetch next row from virtual array */
  source->source__row--;
  image__ptr = (*cinfo->mem->access__virt__sarray)
    ((j__common__ptr) cinfo, source->whole__image,
     source->source__row, (JDIMENSION) 1, FALSE);

  /* Transfer data.  Note source values are in BGR order
   * (even though Microsoft's own documents say the opposite).
   */
  inptr = image__ptr[0];
  outptr = source->pub.buffer[0];
  for (col = cinfo->image__width; col > 0; col--) {
    outptr[2] = *inptr++;	/* can omit GETJSAMPLE() safely */
    outptr[1] = *inptr++;
    outptr[0] = *inptr++;
    outptr += 3;
  }

  return 1;
}


/*
 * This method loads the image into whole__image during the first call on
 * get__pixel__rows.  The get__pixel__rows pointer is then adjusted to call
 * get__8bit__row or get__24bit__row on subsequent calls.
 */

METHODDEF(JDIMENSION)
preload__image (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  bmp__source__ptr source = (bmp__source__ptr) sinfo;
  register FILE *infile = source->pub.input__file;
  register int c;
  register JSAMPROW out__ptr;
  JSAMPARRAY image__ptr;
  JDIMENSION row, col;
  cd__progress__ptr progress = (cd__progress__ptr) cinfo->progress;

  /* Read the data into a virtual array in input-file row order. */
  for (row = 0; row < cinfo->image__height; row++) {
    if (progress != NULL) {
      progress->pub.pass__counter = (long) row;
      progress->pub.pass__limit = (long) cinfo->image__height;
      (*progress->pub.progress__monitor) ((j__common__ptr) cinfo);
    }
    image__ptr = (*cinfo->mem->access__virt__sarray)
      ((j__common__ptr) cinfo, source->whole__image,
       row, (JDIMENSION) 1, TRUE);
    out__ptr = image__ptr[0];
    for (col = source->row__width; col > 0; col--) {
      /* inline copy of read__byte() for speed */
      if ((c = graph_getc(infile)) == EOF)
	ERREXIT(cinfo, JERR__INPUT__EOF);
      *out__ptr++ = (JSAMPLE) c;
    }
  }
  if (progress != NULL)
    progress->completed__extra__passes++;

  /* Set up to read from the virtual array in top-to-bottom order */
  switch (source->bits__per__pixel) {
  case 8:
    source->pub.get__pixel__rows = get__8bit__row;
    break;
  case 24:
    source->pub.get__pixel__rows = get__24bit__row;
    break;
  default:
    ERREXIT(cinfo, JERR__BMP__BADDEPTH);
  }
  source->source__row = cinfo->image__height;

  /* And read the first row */
  return (*source->pub.get__pixel__rows) (cinfo, sinfo);
}


/*
 * Read the file header; return image size and component count.
 */

METHODDEF(void)
start__input__bmp (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  bmp__source__ptr source = (bmp__source__ptr) sinfo;
  U__CHAR bmpfileheader[14];
  U__CHAR bmpinfoheader[64];
#define GET__2B(array,offset)  ((unsigned int) UCH(array[offset]) + \
			       (((unsigned int) UCH(array[offset+1])) << 8))
#define GET__4B(array,offset)  ((INT32) UCH(array[offset]) + \
			       (((INT32) UCH(array[offset+1])) << 8) + \
			       (((INT32) UCH(array[offset+2])) << 16) + \
			       (((INT32) UCH(array[offset+3])) << 24))
  INT32 bfOffBits;
  INT32 headerSize;
  INT32 biWidth = 0;		/* initialize to avoid compiler warning */
  INT32 biHeight = 0;
  unsigned int biPlanes;
  INT32 biCompression;
  INT32 biXPelsPerMeter,biYPelsPerMeter;
  INT32 biClrUsed = 0;
  int mapentrysize = 0;		/* 0 indicates no colormap */
  INT32 bPad;
  JDIMENSION row__width;

  /* Read and verify the bitmap file header */
  if (! ReadOK(source->pub.input__file, bmpfileheader, 14))
    ERREXIT(cinfo, JERR__INPUT__EOF);
  if (GET__2B(bmpfileheader,0) != 0x4D42) /* 'BM' */
    ERREXIT(cinfo, JERR__BMP__NOT);
  bfOffBits = (INT32) GET__4B(bmpfileheader,10);
  /* We ignore the remaining fileheader fields */

  /* The infoheader might be 12 bytes (OS/2 1.x), 40 bytes (Windows),
   * or 64 bytes (OS/2 2.x).  Check the first 4 bytes to find out which.
   */
  if (! ReadOK(source->pub.input__file, bmpinfoheader, 4))
    ERREXIT(cinfo, JERR__INPUT__EOF);
  headerSize = (INT32) GET__4B(bmpinfoheader,0);
  if (headerSize < 12 || headerSize > 64)
    ERREXIT(cinfo, JERR__BMP__BADHEADER);
  if (! ReadOK(source->pub.input__file, bmpinfoheader+4, headerSize-4))
    ERREXIT(cinfo, JERR__INPUT__EOF);

  switch ((int) headerSize) {
  case 12:
    /* Decode OS/2 1.x header (Microsoft calls this a BITMAPCOREHEADER) */
    biWidth = (INT32) GET__2B(bmpinfoheader,4);
    biHeight = (INT32) GET__2B(bmpinfoheader,6);
    biPlanes = GET__2B(bmpinfoheader,8);
    source->bits__per__pixel = (int) GET__2B(bmpinfoheader,10);

    switch (source->bits__per__pixel) {
    case 8:			/* colormapped image */
      mapentrysize = 3;		/* OS/2 uses RGBTRIPLE colormap */
      TRACEMS2(cinfo, 1, JTRC__BMP__OS2__MAPPED, (int) biWidth, (int) biHeight);
      break;
    case 24:			/* RGB image */
      TRACEMS2(cinfo, 1, JTRC__BMP__OS2, (int) biWidth, (int) biHeight);
      break;
    default:
      ERREXIT(cinfo, JERR__BMP__BADDEPTH);
      break;
    }
    if (biPlanes != 1)
      ERREXIT(cinfo, JERR__BMP__BADPLANES);
    break;
  case 40:
  case 64:
    /* Decode Windows 3.x header (Microsoft calls this a BITMAPINFOHEADER) */
    /* or OS/2 2.x header, which has additional fields that we ignore */
    biWidth = GET__4B(bmpinfoheader,4);
    biHeight = GET__4B(bmpinfoheader,8);
    biPlanes = GET__2B(bmpinfoheader,12);
    source->bits__per__pixel = (int) GET__2B(bmpinfoheader,14);
    biCompression = GET__4B(bmpinfoheader,16);
    biXPelsPerMeter = GET__4B(bmpinfoheader,24);
    biYPelsPerMeter = GET__4B(bmpinfoheader,28);
    biClrUsed = GET__4B(bmpinfoheader,32);
    /* biSizeImage, biClrImportant fields are ignored */

    switch (source->bits__per__pixel) {
    case 8:			/* colormapped image */
      mapentrysize = 4;		/* Windows uses RGBQUAD colormap */
      TRACEMS2(cinfo, 1, JTRC__BMP__MAPPED, (int) biWidth, (int) biHeight);
      break;
    case 24:			/* RGB image */
      TRACEMS2(cinfo, 1, JTRC__BMP, (int) biWidth, (int) biHeight);
      break;
    default:
      ERREXIT(cinfo, JERR__BMP__BADDEPTH);
      break;
    }
    if (biPlanes != 1)
      ERREXIT(cinfo, JERR__BMP__BADPLANES);
    if (biCompression != 0)
      ERREXIT(cinfo, JERR__BMP__COMPRESSED);

    if (biXPelsPerMeter > 0 && biYPelsPerMeter > 0) {
      /* Set JFIF density parameters from the BMP data */
      cinfo->X__density = (UINT16) (biXPelsPerMeter/100); /* 100 cm per meter */
      cinfo->Y__density = (UINT16) (biYPelsPerMeter/100);
      cinfo->density__unit = 2;	/* dots/cm */
    }
    break;
  default:
    ERREXIT(cinfo, JERR__BMP__BADHEADER);
    break;
  }

  /* Compute distance to bitmap data --- will adjust for colormap below */
  bPad = bfOffBits - (headerSize + 14);

  /* Read the colormap, if any */
  if (mapentrysize > 0) {
    if (biClrUsed <= 0)
      biClrUsed = 256;		/* assume it's 256 */
    else if (biClrUsed > 256)
      ERREXIT(cinfo, JERR__BMP__BADCMAP);
    /* Allocate space to store the colormap */
    source->colormap = (*cinfo->mem->alloc__sarray)
      ((j__common__ptr) cinfo, JPOOL__IMAGE,
       (JDIMENSION) biClrUsed, (JDIMENSION) 3);
    /* and read it from the file */
    read__colormap(source, (int) biClrUsed, mapentrysize);
    /* account for size of colormap */
    bPad -= biClrUsed * mapentrysize;
  }

  /* Skip any remaining pad bytes */
  if (bPad < 0)			/* incorrect bfOffBits value? */
    ERREXIT(cinfo, JERR__BMP__BADHEADER);
  while (--bPad >= 0) {
    (void) read__byte(source);
  }

  /* Compute row width in file, including padding to 4-byte boundary */
  if (source->bits__per__pixel == 24)
    row__width = (JDIMENSION) (biWidth * 3);
  else
    row__width = (JDIMENSION) biWidth;
  while ((row__width & 3) != 0) row__width++;
  source->row__width = row__width;

  /* Allocate space for inversion array, prepare for preload pass */
  source->whole__image = (*cinfo->mem->request__virt__sarray)
    ((j__common__ptr) cinfo, JPOOL__IMAGE, FALSE,
     row__width, (JDIMENSION) biHeight, (JDIMENSION) 1);
  source->pub.get__pixel__rows = preload__image;
  if (cinfo->progress != NULL) {
    cd__progress__ptr progress = (cd__progress__ptr) cinfo->progress;
    progress->total__extra__passes++; /* count file input as separate pass */
  }

  /* Allocate one-row buffer for returned data */
  source->pub.buffer = (*cinfo->mem->alloc__sarray)
    ((j__common__ptr) cinfo, JPOOL__IMAGE,
     (JDIMENSION) (biWidth * 3), (JDIMENSION) 1);
  source->pub.buffer__height = 1;

  cinfo->in__color__space = JCS__RGB;
  cinfo->input__components = 3;
  cinfo->data__precision = 8;
  cinfo->image__width = (JDIMENSION) biWidth;
  cinfo->image__height = (JDIMENSION) biHeight;
}


/*
 * Finish up at the end of the file.
 */

METHODDEF(void)
finish__input__bmp (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  /* no work */
}


/*
 * The module selection routine for BMP format input.
 */

GLOBAL(cjpeg__source__ptr)
jinit__read__bmp (j__compress__ptr cinfo)
{
  bmp__source__ptr source;

  /* Create module interface object */
  source = (bmp__source__ptr)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  SIZEOF(bmp__source__struct));
  source->cinfo = cinfo;	/* make back link for subroutines */
  /* Fill in method ptrs, except get__pixel__rows which start__input sets */
  source->pub.start__input = start__input__bmp;
  source->pub.finish__input = finish__input__bmp;

  return (cjpeg__source__ptr) source;
}

#endif /* BMP__SUPPORTED */
