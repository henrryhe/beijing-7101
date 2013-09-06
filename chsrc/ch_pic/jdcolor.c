/*
 * jdcolor.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains output colorspace conversion routines.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Private subobject */

typedef struct {
  struct jpeg__color__deconverter pub; /* public fields */

  /* Private state for YCC->RGB conversion */
  int * Cr__r__tab;		/* => table for Cr to R conversion */
  int * Cb__b__tab;		/* => table for Cb to B conversion */
  INT32 * Cr__g__tab;		/* => table for Cr to G conversion */
  INT32 * Cb__g__tab;		/* => table for Cb to G conversion */
} my__color__deconverter;

typedef my__color__deconverter * my__cconvert__ptr;


/**************** YCbCr -> RGB conversion: most common case **************/

/*
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	R = Y                + 1.40200 * Cr
 *	G = Y - 0.34414 * Cb - 0.71414 * Cr
 *	B = Y + 1.77200 * Cb
 * where Cb and Cr represent the incoming values less CENTERJSAMPLE.
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 * Notice that Y, being an integral input, does not contribute any fraction
 * so it need not participate in the rounding.
 *
 * For even more speed, we avoid doing any multiplications in the inner loop
 * by precalculating the constants times Cb and Cr for all possible values.
 * For 8-bit JSAMPLEs this is very reasonable (only 256 entries per table);
 * for 12-bit samples it is still acceptable.  It's not very reasonable for
 * 16-bit samples, but if you want lossless storage you shouldn't be changing
 * colorspace anyway.
 * The Cr=>R and Cb=>B values can be rounded to integers in advance; the
 * values for the G calculation are left scaled up, since we must add them
 * together before rounding.
 */

#define SCALEBITS	16	/* speediest right-shift on some machines */
#define ONE__HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))


/*
 * Initialize tables for YCC->RGB colorspace conversion.
 */

LOCAL(void)
build__ycc__rgb__table (j__decompress__ptr cinfo)
{
  my__cconvert__ptr cconvert = (my__cconvert__ptr) cinfo->cconvert;
  int i;
  INT32 x;
  SHIFT__TEMPS

  cconvert->Cr__r__tab = (int *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  cconvert->Cb__b__tab = (int *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  cconvert->Cr__g__tab = (INT32 *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));
  cconvert->Cb__g__tab = (INT32 *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));

  for (i = 0, x = -CENTERJSAMPLE; i <= MAXJSAMPLE; i++, x++) {
    /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
    /* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
    /* Cr=>R value is nearest int to 1.40200 * x */
    cconvert->Cr__r__tab[i] = (int)
		    RIGHT__SHIFT(FIX(1.40200) * x + ONE__HALF, SCALEBITS);
    /* Cb=>B value is nearest int to 1.77200 * x */
    cconvert->Cb__b__tab[i] = (int)
		    RIGHT__SHIFT(FIX(1.77200) * x + ONE__HALF, SCALEBITS);
    /* Cr=>G value is scaled-up -0.71414 * x */
    cconvert->Cr__g__tab[i] = (- FIX(0.71414)) * x;
    /* Cb=>G value is scaled-up -0.34414 * x */
    /* We also add in ONE__HALF so that need not do it in inner loop */
    cconvert->Cb__g__tab[i] = (- FIX(0.34414)) * x + ONE__HALF;
  }
}


/*
 * Convert some rows of samples to the output colorspace.
 *
 * Note that we change from noninterleaved, one-plane-per-component format
 * to interleaved-pixel format.  The output buffer is therefore three times
 * as wide as the input buffer.
 * A starting row offset is provided only for the input buffer.  The caller
 * can easily adjust the passed output__buf value to accommodate any row
 * offset required on that side.
 */

METHODDEF(void)
ycc__rgb__convert (j__decompress__ptr cinfo,
		 JSAMPIMAGE input__buf, JDIMENSION input__row,
		 JSAMPARRAY output__buf, int num__rows)
{
  my__cconvert__ptr cconvert = (my__cconvert__ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JDIMENSION col;
  JDIMENSION num__cols = cinfo->output__width;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range__limit = cinfo->sample__range__limit;
  register int * Crrtab = cconvert->Cr__r__tab;
  register int * Cbbtab = cconvert->Cb__b__tab;
  register INT32 * Crgtab = cconvert->Cr__g__tab;
  register INT32 * Cbgtab = cconvert->Cb__g__tab;
  SHIFT__TEMPS

  while (--num__rows >= 0) {
    inptr0 = input__buf[0][input__row];
    inptr1 = input__buf[1][input__row];
    inptr2 = input__buf[2][input__row];
    input__row++;
   outptr = *output__buf++;
/*	outptr = output__buf++;*/
    for (col = 0; col < num__cols; col++) {
      y  = GETJSAMPLE(inptr0[col]);
      cb = GETJSAMPLE(inptr1[col]);
      cr = GETJSAMPLE(inptr2[col]);
      /* Range-limiting is essential due to noise introduced by DCT losses. */
      outptr[RGB__RED] =   range__limit[y + Crrtab[cr]];
      outptr[RGB__GREEN] = range__limit[y +
			      ((int) RIGHT__SHIFT(Cbgtab[cb] + Crgtab[cr],
						 SCALEBITS))];
      outptr[RGB__BLUE] =  range__limit[y + Cbbtab[cb]];
      outptr += RGB__PIXELSIZE;
    }
  }
}


/**************** Cases other than YCbCr -> RGB **************/


/*
 * Color conversion for no colorspace change: just copy the data,
 * converting from separate-planes to interleaved representation.
 */

METHODDEF(void)
null__convert (j__decompress__ptr cinfo,
	      JSAMPIMAGE input__buf, JDIMENSION input__row,
	      JSAMPARRAY output__buf, int num__rows)
{
  register JSAMPROW inptr, outptr;
  register JDIMENSION count;
  register int num__components = cinfo->num__components;
  JDIMENSION num__cols = cinfo->output__width;
  int ci;

  while (--num__rows >= 0) {
    for (ci = 0; ci < num__components; ci++) {
      inptr = input__buf[ci][input__row];
      outptr = output__buf[0] + ci;
      for (count = num__cols; count > 0; count--) {
	*outptr = *inptr++;	/* needn't bother with GETJSAMPLE() here */
	outptr += num__components;
      }
    }
    input__row++;
    output__buf++;
  }
}


/*
 * Color conversion for grayscale: just copy the data.
 * This also works for YCbCr -> grayscale conversion, in which
 * we just copy the Y (luminance) component and ignore chrominance.
 */

METHODDEF(void)
grayscale__convert (j__decompress__ptr cinfo,
		   JSAMPIMAGE input__buf, JDIMENSION input__row,
		   JSAMPARRAY output__buf, int num__rows)
{
  jcopy__sample__rows(input__buf[0], (int) input__row, output__buf, 0,
		    num__rows, cinfo->output__width);
}


/*
 * Convert grayscale to RGB: just duplicate the graylevel three times.
 * This is provided to support applications that don't want to cope
 * with grayscale as a separate case.
 */

METHODDEF(void)
gray__rgb__convert (j__decompress__ptr cinfo,
		  JSAMPIMAGE input__buf, JDIMENSION input__row,
		  JSAMPARRAY output__buf, int num__rows)
{
  register JSAMPROW inptr, outptr;
  register JDIMENSION col;
  JDIMENSION num__cols = cinfo->output__width;

  while (--num__rows >= 0) {
    inptr = input__buf[0][input__row++];
    outptr = *output__buf++;
    for (col = 0; col < num__cols; col++) {
      /* We can dispense with GETJSAMPLE() here */
      outptr[RGB__RED] = outptr[RGB__GREEN] = outptr[RGB__BLUE] = inptr[col];
      outptr += RGB__PIXELSIZE;
    }
  }
}


/*
 * Adobe-style YCCK->CMYK conversion.
 * We convert YCbCr to R=1-C, G=1-M, and B=1-Y using the same
 * conversion as above, while passing K (black) unchanged.
 * We assume build__ycc__rgb__table has been called.
 */

METHODDEF(void)
ycck__cmyk__convert (j__decompress__ptr cinfo,
		   JSAMPIMAGE input__buf, JDIMENSION input__row,
		   JSAMPARRAY output__buf, int num__rows)
{
  my__cconvert__ptr cconvert = (my__cconvert__ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2, inptr3;
  register JDIMENSION col;
  JDIMENSION num__cols = cinfo->output__width;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range__limit = cinfo->sample__range__limit;
  register int * Crrtab = cconvert->Cr__r__tab;
  register int * Cbbtab = cconvert->Cb__b__tab;
  register INT32 * Crgtab = cconvert->Cr__g__tab;
  register INT32 * Cbgtab = cconvert->Cb__g__tab;
  SHIFT__TEMPS

  while (--num__rows >= 0) {
    inptr0 = input__buf[0][input__row];
    inptr1 = input__buf[1][input__row];
    inptr2 = input__buf[2][input__row];
    inptr3 = input__buf[3][input__row];
    input__row++;
    outptr = *output__buf++;
    for (col = 0; col < num__cols; col++) {
      y  = GETJSAMPLE(inptr0[col]);
      cb = GETJSAMPLE(inptr1[col]);
      cr = GETJSAMPLE(inptr2[col]);
      /* Range-limiting is essential due to noise introduced by DCT losses. */
      outptr[0] = range__limit[MAXJSAMPLE - (y + Crrtab[cr])];	/* red */
      outptr[1] = range__limit[MAXJSAMPLE - (y +			/* green */
			      ((int) RIGHT__SHIFT(Cbgtab[cb] + Crgtab[cr],
						 SCALEBITS)))];
      outptr[2] = range__limit[MAXJSAMPLE - (y + Cbbtab[cb])];	/* blue */
      /* K passes through unchanged */
      outptr[3] = inptr3[col];	/* don't need GETJSAMPLE here */
      outptr += 4;
    }
  }
}


/*
 * Empty method for start__pass.
 */

METHODDEF(void)
start__pass__dcolor (j__decompress__ptr cinfo)
{
  /* no work needed */
}


/*
 * Module initialization routine for output colorspace conversion.
 */

GLOBAL(void)
jinit__color__deconverter (j__decompress__ptr cinfo)
{
  my__cconvert__ptr cconvert;
  int ci;

  cconvert = (my__cconvert__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__color__deconverter));
  cinfo->cconvert = (struct jpeg__color__deconverter *) cconvert;
  cconvert->pub.start__pass = start__pass__dcolor;

  /* Make sure num__components agrees with jpeg__color__space */
  switch (cinfo->jpeg__color__space) {
  case JCS__GRAYSCALE:
    if (cinfo->num__components != 1)
      ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
    break;

  case JCS__RGB:
  case JCS__YCbCr:
    if (cinfo->num__components != 3)
      ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
    break;

  case JCS__CMYK:
  case JCS__YCCK:
    if (cinfo->num__components != 4)
      ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
    break;

  default:			/* JCS__UNKNOWN can be anything */
    if (cinfo->num__components < 1)
      ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
    break;
  }

  /* Set out__color__components and conversion method based on requested space.
   * Also clear the component__needed flags for any unused components,
   * so that earlier pipeline stages can avoid useless computation.
   */

  switch (cinfo->out__color__space) {
  case JCS__GRAYSCALE:
    cinfo->out__color__components = 1;
    if (cinfo->jpeg__color__space == JCS__GRAYSCALE ||
	cinfo->jpeg__color__space == JCS__YCbCr) {
      cconvert->pub.color__convert = grayscale__convert;
      /* For color->grayscale conversion, only the Y (0) component is needed */
      for (ci = 1; ci < cinfo->num__components; ci++)
	cinfo->comp__info[ci].component__needed = FALSE;
    } else
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    break;

  case JCS__RGB:
    cinfo->out__color__components = RGB__PIXELSIZE;
    if (cinfo->jpeg__color__space == JCS__YCbCr) {
      cconvert->pub.color__convert = ycc__rgb__convert;
      build__ycc__rgb__table(cinfo);
    } else if (cinfo->jpeg__color__space == JCS__GRAYSCALE) {
      cconvert->pub.color__convert = gray__rgb__convert;
    } else if (cinfo->jpeg__color__space == JCS__RGB && RGB__PIXELSIZE == 3) {
      cconvert->pub.color__convert = null__convert;
    } else
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    break;

  case JCS__CMYK:
    cinfo->out__color__components = 4;
    if (cinfo->jpeg__color__space == JCS__YCCK) {
      cconvert->pub.color__convert = ycck__cmyk__convert;
      build__ycc__rgb__table(cinfo);
    } else if (cinfo->jpeg__color__space == JCS__CMYK) {
      cconvert->pub.color__convert = null__convert;
    } else
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    break;

  default:
    /* Permit null conversion to same output space */
    if (cinfo->out__color__space == cinfo->jpeg__color__space) {
      cinfo->out__color__components = cinfo->num__components;
      cconvert->pub.color__convert = null__convert;
    } else			/* unsupported non-null conversion */
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    break;
  }

  if (cinfo->quantize__colors)
    cinfo->output__components = 1; /* single colormapped output component */
  else
    cinfo->output__components = cinfo->out__color__components;
}
