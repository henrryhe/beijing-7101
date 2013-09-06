/*
 * rdtarga.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in Targa format.
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; start__input may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed Targa format).
 *
 * Based on code contributed by Lee Daniel Crocker.
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */
#include "graphconfig.h"

#ifdef TARGA__SUPPORTED


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

typedef struct __tga__source__struct * tga__source__ptr;

typedef struct __tga__source__struct {
  struct cjpeg__source__struct pub; /* public fields */

  j__compress__ptr cinfo;		/* back link saves passing separate parm */

  JSAMPARRAY colormap;		/* Targa colormap (converted to my format) */

  jvirt__sarray__ptr whole__image;	/* Needed if funny input row order */
  JDIMENSION current__row;	/* Current logical row number to read */

  /* Pointer to routine to extract next Targa pixel from input file */
  JMETHOD(void, read__pixel, (tga__source__ptr sinfo));

  /* Result of read__pixel is delivered here: */
  U__CHAR tga__pixel[4];

  int pixel__size;		/* Bytes per Targa pixel (1 to 4) */

  /* State info for reading RLE-coded pixels; both counts must be init to 0 */
  int block__count;		/* # of pixels remaining in RLE block */
  int dup__pixel__count;		/* # of times to duplicate previous pixel */

  /* This saves the correct pixel-row-expansion method for preload__image */
  JMETHOD(JDIMENSION, get__pixel__rows, (j__compress__ptr cinfo,
				       cjpeg__source__ptr sinfo));
} tga__source__struct;


/* For expanding 5-bit pixel values to 8-bit with best rounding */

static const UINT8 c5to8bits[32] = {
    0,   8,  16,  25,  33,  41,  49,  58,
   66,  74,  82,  90,  99, 107, 115, 123,
  132, 140, 148, 156, 165, 173, 181, 189,
  197, 206, 214, 222, 230, 239, 247, 255
};



LOCAL(int)
read__byte (tga__source__ptr sinfo)
/* Read next byte from Targa file */
{
  register FILE *infile = sinfo->pub.input__file;
  register int c;

  if ((c = graph_getc(infile)) == EOF)
    ERREXIT(sinfo->cinfo, JERR__INPUT__EOF);
  return c;
}


LOCAL(void)
read__colormap (tga__source__ptr sinfo, int cmaplen, int mapentrysize)
/* Read the colormap from a Targa file */
{
  int i;

  /* Presently only handles 24-bit BGR format */
  if (mapentrysize != 24)
    ERREXIT(sinfo->cinfo, JERR__TGA__BADCMAP);

  for (i = 0; i < cmaplen; i++) {
    sinfo->colormap[2][i] = (JSAMPLE) read__byte(sinfo);
    sinfo->colormap[1][i] = (JSAMPLE) read__byte(sinfo);
    sinfo->colormap[0][i] = (JSAMPLE) read__byte(sinfo);
  }
}


/*
 * read__pixel methods: get a single pixel from Targa file into tga__pixel[]
 */

METHODDEF(void)
read__non__rle__pixel (tga__source__ptr sinfo)
/* Read one Targa pixel from the input file; no RLE expansion */
{
  register FILE *infile = sinfo->pub.input__file;
  register int i;

  for (i = 0; i < sinfo->pixel__size; i++) {
    sinfo->tga__pixel[i] = (U__CHAR) graph_getc(infile);
  }
}


METHODDEF(void)
read__rle__pixel (tga__source__ptr sinfo)
/* Read one Targa pixel from the input file, expanding RLE data as needed */
{
  register FILE *infile = sinfo->pub.input__file;
  register int i;

  /* Duplicate previously read pixel? */
  if (sinfo->dup__pixel__count > 0) {
    sinfo->dup__pixel__count--;
    return;
  }

  /* Time to read RLE block header? */
  if (--sinfo->block__count < 0) { /* decrement pixels remaining in block */
    i = read__byte(sinfo);
    if (i & 0x80) {		/* Start of duplicate-pixel block? */
      sinfo->dup__pixel__count = i & 0x7F; /* number of dups after this one */
      sinfo->block__count = 0;	/* then read new block header */
    } else {
      sinfo->block__count = i & 0x7F; /* number of pixels after this one */
    }
  }

  /* Read next pixel */
  for (i = 0; i < sinfo->pixel__size; i++) {
    sinfo->tga__pixel[i] = (U__CHAR) graph_getc(infile);
  }
}


/*
 * Read one row of pixels.
 *
 * We provide several different versions depending on input file format.
 */


METHODDEF(JDIMENSION)
get__8bit__gray__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading 8-bit grayscale pixels */
{
  tga__source__ptr source = (tga__source__ptr) sinfo;
  register JSAMPROW ptr;
  register JDIMENSION col;
  
  ptr = source->pub.buffer[0];
  for (col = cinfo->image__width; col > 0; col--) {
    (*source->read__pixel) (source); /* Load next pixel into tga__pixel */
    *ptr++ = (JSAMPLE) UCH(source->tga__pixel[0]);
  }
  return 1;
}

METHODDEF(JDIMENSION)
get__8bit__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading 8-bit colormap indexes */
{
  tga__source__ptr source = (tga__source__ptr) sinfo;
  register int t;
  register JSAMPROW ptr;
  register JDIMENSION col;
  register JSAMPARRAY colormap = source->colormap;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image__width; col > 0; col--) {
    (*source->read__pixel) (source); /* Load next pixel into tga__pixel */
    t = UCH(source->tga__pixel[0]);
    *ptr++ = colormap[0][t];
    *ptr++ = colormap[1][t];
    *ptr++ = colormap[2][t];
  }
  return 1;
}

METHODDEF(JDIMENSION)
get__16bit__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading 16-bit pixels */
{
  tga__source__ptr source = (tga__source__ptr) sinfo;
  register int t;
  register JSAMPROW ptr;
  register JDIMENSION col;
  
  ptr = source->pub.buffer[0];
  for (col = cinfo->image__width; col > 0; col--) {
    (*source->read__pixel) (source); /* Load next pixel into tga__pixel */
    t = UCH(source->tga__pixel[0]);
    t += UCH(source->tga__pixel[1]) << 8;
    /* We expand 5 bit data to 8 bit sample width.
     * The format of the 16-bit (LSB first) input word is
     *     xRRRRRGGGGGBBBBB
     */
    ptr[2] = (JSAMPLE) c5to8bits[t & 0x1F];
    t >>= 5;
    ptr[1] = (JSAMPLE) c5to8bits[t & 0x1F];
    t >>= 5;
    ptr[0] = (JSAMPLE) c5to8bits[t & 0x1F];
    ptr += 3;
  }
  return 1;
}

METHODDEF(JDIMENSION)
get__24bit__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
/* This version is for reading 24-bit pixels */
{
  tga__source__ptr source = (tga__source__ptr) sinfo;
  register JSAMPROW ptr;
  register JDIMENSION col;
  
  ptr = source->pub.buffer[0];
  for (col = cinfo->image__width; col > 0; col--) {
    (*source->read__pixel) (source); /* Load next pixel into tga__pixel */
    *ptr++ = (JSAMPLE) UCH(source->tga__pixel[2]); /* change BGR to RGB order */
    *ptr++ = (JSAMPLE) UCH(source->tga__pixel[1]);
    *ptr++ = (JSAMPLE) UCH(source->tga__pixel[0]);
  }
  return 1;
}

/*
 * Targa also defines a 32-bit pixel format with order B,G,R,A.
 * We presently ignore the attribute byte, so the code for reading
 * these pixels is identical to the 24-bit routine above.
 * This works because the actual pixel length is only known to read__pixel.
 */

#define get__32bit__row  get__24bit__row


/*
 * This method is for re-reading the input data in standard top-down
 * row order.  The entire image has already been read into whole__image
 * with proper conversion of pixel format, but it's in a funny row order.
 */

METHODDEF(JDIMENSION)
get__memory__row (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  tga__source__ptr source = (tga__source__ptr) sinfo;
  JDIMENSION source__row;

  /* Compute row of source that maps to current__row of normal order */
  /* For now, assume image is bottom-up and not interlaced. */
  /* NEEDS WORK to support interlaced images! */
  source__row = cinfo->image__height - source->current__row - 1;

  /* Fetch that row from virtual array */
  source->pub.buffer = (*cinfo->mem->access__virt__sarray)
    ((j__common__ptr) cinfo, source->whole__image,
     source__row, (JDIMENSION) 1, FALSE);

  source->current__row++;
  return 1;
}


/*
 * This method loads the image into whole__image during the first call on
 * get__pixel__rows.  The get__pixel__rows pointer is then adjusted to call
 * get__memory__row on subsequent calls.
 */

METHODDEF(JDIMENSION)
preload__image (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  tga__source__ptr source = (tga__source__ptr) sinfo;
  JDIMENSION row;
  cd__progress__ptr progress = (cd__progress__ptr) cinfo->progress;

  /* Read the data into a virtual array in input-file row order. */
  for (row = 0; row < cinfo->image__height; row++) {
    if (progress != NULL) {
      progress->pub.pass__counter = (long) row;
      progress->pub.pass__limit = (long) cinfo->image__height;
      (*progress->pub.progress__monitor) ((j__common__ptr) cinfo);
    }
    source->pub.buffer = (*cinfo->mem->access__virt__sarray)
      ((j__common__ptr) cinfo, source->whole__image, row, (JDIMENSION) 1, TRUE);
    (*source->get__pixel__rows) (cinfo, sinfo);
  }
  if (progress != NULL)
    progress->completed__extra__passes++;

  /* Set up to read from the virtual array in unscrambled order */
  source->pub.get__pixel__rows = get__memory__row;
  source->current__row = 0;
  /* And read the first row */
  return get__memory__row(cinfo, sinfo);
}


/*
 * Read the file header; return image size and component count.
 */

METHODDEF(void)
start__input__tga (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  tga__source__ptr source = (tga__source__ptr) sinfo;
  U__CHAR targaheader[18];
  int idlen, cmaptype, subtype, flags, interlace__type, components;
  unsigned int width, height, maplen;
  CH_BOOL is__bottom__up;

#define GET__2B(offset)	((unsigned int) UCH(targaheader[offset]) + \
			 (((unsigned int) UCH(targaheader[offset+1])) << 8))

  if (! ReadOK(source->pub.input__file, targaheader, 18))
    ERREXIT(cinfo, JERR__INPUT__EOF);

  /* Pretend "15-bit" pixels are 16-bit --- we ignore attribute bit anyway */
  if (targaheader[16] == 15)
    targaheader[16] = 16;

  idlen = UCH(targaheader[0]);
  cmaptype = UCH(targaheader[1]);
  subtype = UCH(targaheader[2]);
  maplen = GET__2B(5);
  width = GET__2B(12);
  height = GET__2B(14);
  source->pixel__size = UCH(targaheader[16]) >> 3;
  flags = UCH(targaheader[17]);	/* Image Descriptor byte */

  is__bottom__up = ((flags & 0x20) == 0);	/* bit 5 set => top-down */
  interlace__type = flags >> 6;	/* bits 6/7 are interlace code */

  if (cmaptype > 1 ||		/* cmaptype must be 0 or 1 */
      source->pixel__size < 1 || source->pixel__size > 4 ||
      (UCH(targaheader[16]) & 7) != 0 || /* bits/pixel must be multiple of 8 */
      interlace__type != 0)	/* currently don't allow interlaced image */
    ERREXIT(cinfo, JERR__TGA__BADPARMS);
  
  if (subtype > 8) {
    /* It's an RLE-coded file */
    source->read__pixel = read__rle__pixel;
    source->block__count = source->dup__pixel__count = 0;
    subtype -= 8;
  } else {
    /* Non-RLE file */
    source->read__pixel = read__non__rle__pixel;
  }

  /* Now should have subtype 1, 2, or 3 */
  components = 3;		/* until proven different */
  cinfo->in__color__space = JCS__RGB;

  switch (subtype) {
  case 1:			/* Colormapped image */
    if (source->pixel__size == 1 && cmaptype == 1)
      source->get__pixel__rows = get__8bit__row;
    else
      ERREXIT(cinfo, JERR__TGA__BADPARMS);
    TRACEMS2(cinfo, 1, JTRC__TGA__MAPPED, width, height);
    break;
  case 2:			/* RGB image */
    switch (source->pixel__size) {
    case 2:
      source->get__pixel__rows = get__16bit__row;
      break;
    case 3:
      source->get__pixel__rows = get__24bit__row;
      break;
    case 4:
      source->get__pixel__rows = get__32bit__row;
      break;
    default:
      ERREXIT(cinfo, JERR__TGA__BADPARMS);
      break;
    }
    TRACEMS2(cinfo, 1, JTRC__TGA, width, height);
    break;
  case 3:			/* Grayscale image */
    components = 1;
    cinfo->in__color__space = JCS__GRAYSCALE;
    if (source->pixel__size == 1)
      source->get__pixel__rows = get__8bit__gray__row;
    else
      ERREXIT(cinfo, JERR__TGA__BADPARMS);
    TRACEMS2(cinfo, 1, JTRC__TGA__GRAY, width, height);
    break;
  default:
    ERREXIT(cinfo, JERR__TGA__BADPARMS);
    break;
  }

  if (is__bottom__up) {
    /* Create a virtual array to buffer the upside-down image. */
    source->whole__image = (*cinfo->mem->request__virt__sarray)
      ((j__common__ptr) cinfo, JPOOL__IMAGE, FALSE,
       (JDIMENSION) width * components, (JDIMENSION) height, (JDIMENSION) 1);
    if (cinfo->progress != NULL) {
      cd__progress__ptr progress = (cd__progress__ptr) cinfo->progress;
      progress->total__extra__passes++; /* count file input as separate pass */
    }
    /* source->pub.buffer will point to the virtual array. */
    source->pub.buffer__height = 1; /* in case anyone looks at it */
    source->pub.get__pixel__rows = preload__image;
  } else {
    /* Don't need a virtual array, but do need a one-row input buffer. */
    source->whole__image = NULL;
    source->pub.buffer = (*cinfo->mem->alloc__sarray)
      ((j__common__ptr) cinfo, JPOOL__IMAGE,
       (JDIMENSION) width * components, (JDIMENSION) 1);
    source->pub.buffer__height = 1;
    source->pub.get__pixel__rows = source->get__pixel__rows;
  }
  
  while (idlen--)		/* Throw away ID field */
    (void) read__byte(source);

  if (maplen > 0) {
    if (maplen > 256 || GET__2B(3) != 0)
      ERREXIT(cinfo, JERR__TGA__BADCMAP);
    /* Allocate space to store the colormap */
    source->colormap = (*cinfo->mem->alloc__sarray)
      ((j__common__ptr) cinfo, JPOOL__IMAGE, (JDIMENSION) maplen, (JDIMENSION) 3);
    /* and read it from the file */
    read__colormap(source, (int) maplen, UCH(targaheader[7]));
  } else {
    if (cmaptype)		/* but you promised a cmap! */
      ERREXIT(cinfo, JERR__TGA__BADPARMS);
    source->colormap = NULL;
  }

  cinfo->input__components = components;
  cinfo->data__precision = 8;
  cinfo->image__width = width;
  cinfo->image__height = height;
}


/*
 * Finish up at the end of the file.
 */

METHODDEF(void)
finish__input__tga (j__compress__ptr cinfo, cjpeg__source__ptr sinfo)
{
  /* no work */
}


/*
 * The module selection routine for Targa format input.
 */

GLOBAL(cjpeg__source__ptr)
jinit__read__targa (j__compress__ptr cinfo)
{
  tga__source__ptr source;

  /* Create module interface object */
  source = (tga__source__ptr)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  SIZEOF(tga__source__struct));
  source->cinfo = cinfo;	/* make back link for subroutines */
  /* Fill in method ptrs, except get__pixel__rows which start__input sets */
  source->pub.start__input = start__input__tga;
  source->pub.finish__input = finish__input__tga;

  return (cjpeg__source__ptr) source;
}

#endif /* TARGA__SUPPORTED */
