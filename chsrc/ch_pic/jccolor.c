/*
 * jccolor.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains input colorspace conversion routines.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Private subobject */

typedef struct {
  struct jpeg__color__converter pub; /* public fields */

  /* Private state for RGB->YCC conversion */
  INT32 * rgb__ycc__tab;		/* => table for RGB to YCbCr conversion */
} my__color__converter;

typedef my__color__converter * my__cconvert__ptr;


/**************** RGB -> YCbCr conversion: most common case **************/

/*
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
 *	Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B  + CENTERJSAMPLE
 *	Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B  + CENTERJSAMPLE
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 * Note: older versions of the IJG code used a zero offset of MAXJSAMPLE/2,
 * rather than CENTERJSAMPLE, for Cb and Cr.  This gave equal positive and
 * negative swings for Cb/Cr, but meant that grayscale values (Cb=Cr=0)
 * were not represented exactly.  Now we sacrifice exact representation of
 * maximum red and maximum blue in order to get exact grayscales.
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 *
 * For even more speed, we avoid doing any multiplications in the inner loop
 * by precalculating the constants times R,G,B for all possible values.
 * For 8-bit JSAMPLEs this is very reasonable (only 256 entries per table);
 * for 12-bit samples it is still acceptable.  It's not very reasonable for
 * 16-bit samples, but if you want lossless storage you shouldn't be changing
 * colorspace anyway.
 * The CENTERJSAMPLE offsets and the rounding fudge-factor of 0.5 are included
 * in the tables to save adding them separately in the inner loop.
 */

#define SCALEBITS	16	/* speediest right-shift on some machines */
#define CBCR__OFFSET	((INT32) CENTERJSAMPLE << SCALEBITS)
#define ONE__HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))

/* We allocate one big table and divide it up into eight parts, instead of
 * doing eight alloc__small requests.  This lets us use a single table base
 * address, which can be held in a register in the inner loops on many
 * machines (more than can hold all eight addresses, anyway).
 */

#define R__Y__OFF		0			/* offset to R => Y section */
#define G__Y__OFF		(1*(MAXJSAMPLE+1))	/* offset to G => Y section */
#define B__Y__OFF		(2*(MAXJSAMPLE+1))	/* etc. */
#define R__CB__OFF	(3*(MAXJSAMPLE+1))
#define G__CB__OFF	(4*(MAXJSAMPLE+1))
#define B__CB__OFF	(5*(MAXJSAMPLE+1))
#define R__CR__OFF	B__CB__OFF		/* B=>Cb, R=>Cr are the same */
#define G__CR__OFF	(6*(MAXJSAMPLE+1))
#define B__CR__OFF	(7*(MAXJSAMPLE+1))
#define TABLE__SIZE	(8*(MAXJSAMPLE+1))


/*
 * Initialize for RGB->YCC colorspace conversion.
 */

METHODDEF(void)
rgb__ycc__start (j__compress__ptr cinfo)
{
  my__cconvert__ptr cconvert = (my__cconvert__ptr) cinfo->cconvert;
  INT32 * rgb__ycc__tab;
  INT32 i;

  /* Allocate and fill in the conversion tables. */
  cconvert->rgb__ycc__tab = rgb__ycc__tab = (INT32 *)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				(TABLE__SIZE * SIZEOF(INT32)));

  for (i = 0; i <= MAXJSAMPLE; i++) {
    rgb__ycc__tab[i+R__Y__OFF] = FIX(0.29900) * i;
    rgb__ycc__tab[i+G__Y__OFF] = FIX(0.58700) * i;
    rgb__ycc__tab[i+B__Y__OFF] = FIX(0.11400) * i     + ONE__HALF;
    rgb__ycc__tab[i+R__CB__OFF] = (-FIX(0.16874)) * i;
    rgb__ycc__tab[i+G__CB__OFF] = (-FIX(0.33126)) * i;
    /* We use a rounding fudge-factor of 0.5-epsilon for Cb and Cr.
     * This ensures that the maximum output will round to MAXJSAMPLE
     * not MAXJSAMPLE+1, and thus that we don't have to range-limit.
     */
    rgb__ycc__tab[i+B__CB__OFF] = FIX(0.50000) * i    + CBCR__OFFSET + ONE__HALF-1;
/*  B=>Cb and R=>Cr tables are the same
    rgb__ycc__tab[i+R__CR__OFF] = FIX(0.50000) * i    + CBCR__OFFSET + ONE__HALF-1;
*/
    rgb__ycc__tab[i+G__CR__OFF] = (-FIX(0.41869)) * i;
    rgb__ycc__tab[i+B__CR__OFF] = (-FIX(0.08131)) * i;
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 *
 * Note that we change from the application's interleaved-pixel format
 * to our internal noninterleaved, one-plane-per-component format.
 * The input buffer is therefore three times as wide as the output buffer.
 *
 * A starting row offset is provided only for the output buffer.  The caller
 * can easily adjust the passed input__buf value to accommodate any row
 * offset required on that side.
 */

METHODDEF(void)
rgb__ycc__convert (j__compress__ptr cinfo,
		 JSAMPARRAY input__buf, JSAMPIMAGE output__buf,
		 JDIMENSION output__row, int num__rows)
{
  my__cconvert__ptr cconvert = (my__cconvert__ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb__ycc__tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2;
  register JDIMENSION col;
  JDIMENSION num__cols = cinfo->image__width;

  while (--num__rows >= 0) {
    inptr = *input__buf++;
    outptr0 = output__buf[0][output__row];
    outptr1 = output__buf[1][output__row];
    outptr2 = output__buf[2][output__row];
    output__row++;
    for (col = 0; col < num__cols; col++) {
      r = GETJSAMPLE(inptr[RGB__RED]);
      g = GETJSAMPLE(inptr[RGB__GREEN]);
      b = GETJSAMPLE(inptr[RGB__BLUE]);
      inptr += RGB__PIXELSIZE;
      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
       * must be too; we do not need an explicit range-limiting operation.
       * Hence the value being shifted is never negative, and we don't
       * need the general RIGHT__SHIFT macro.
       */
      /* Y */
      outptr0[col] = (JSAMPLE)
		((ctab[r+R__Y__OFF] + ctab[g+G__Y__OFF] + ctab[b+B__Y__OFF])
		 >> SCALEBITS);
      /* Cb */
      outptr1[col] = (JSAMPLE)
		((ctab[r+R__CB__OFF] + ctab[g+G__CB__OFF] + ctab[b+B__CB__OFF])
		 >> SCALEBITS);
      /* Cr */
      outptr2[col] = (JSAMPLE)
		((ctab[r+R__CR__OFF] + ctab[g+G__CR__OFF] + ctab[b+B__CR__OFF])
		 >> SCALEBITS);
    }
  }
}


/**************** Cases other than RGB -> YCbCr **************/


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles RGB->grayscale conversion, which is the same
 * as the RGB->Y portion of RGB->YCbCr.
 * We assume rgb__ycc__start has been called (we only use the Y tables).
 */

METHODDEF(void)
rgb__gray__convert (j__compress__ptr cinfo,
		  JSAMPARRAY input__buf, JSAMPIMAGE output__buf,
		  JDIMENSION output__row, int num__rows)
{
  my__cconvert__ptr cconvert = (my__cconvert__ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb__ycc__tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  JDIMENSION num__cols = cinfo->image__width;

  while (--num__rows >= 0) {
    inptr = *input__buf++;
    outptr = output__buf[0][output__row];
    output__row++;
    for (col = 0; col < num__cols; col++) {
      r = GETJSAMPLE(inptr[RGB__RED]);
      g = GETJSAMPLE(inptr[RGB__GREEN]);
      b = GETJSAMPLE(inptr[RGB__BLUE]);
      inptr += RGB__PIXELSIZE;
      /* Y */
      outptr[col] = (JSAMPLE)
		((ctab[r+R__Y__OFF] + ctab[g+G__Y__OFF] + ctab[b+B__Y__OFF])
		 >> SCALEBITS);
    }
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles Adobe-style CMYK->YCCK conversion,
 * where we convert R=1-C, G=1-M, and B=1-Y to YCbCr using the same
 * conversion as above, while passing K (black) unchanged.
 * We assume rgb__ycc__start has been called.
 */

METHODDEF(void)
cmyk__ycck__convert (j__compress__ptr cinfo,
		   JSAMPARRAY input__buf, JSAMPIMAGE output__buf,
		   JDIMENSION output__row, int num__rows)
{
  my__cconvert__ptr cconvert = (my__cconvert__ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb__ycc__tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2, outptr3;
  register JDIMENSION col;
  JDIMENSION num__cols = cinfo->image__width;

  while (--num__rows >= 0) {
    inptr = *input__buf++;
    outptr0 = output__buf[0][output__row];
    outptr1 = output__buf[1][output__row];
    outptr2 = output__buf[2][output__row];
    outptr3 = output__buf[3][output__row];
    output__row++;
    for (col = 0; col < num__cols; col++) {
      r = MAXJSAMPLE - GETJSAMPLE(inptr[0]);
      g = MAXJSAMPLE - GETJSAMPLE(inptr[1]);
      b = MAXJSAMPLE - GETJSAMPLE(inptr[2]);
      /* K passes through as-is */
      outptr3[col] = inptr[3];	/* don't need GETJSAMPLE here */
      inptr += 4;
      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
       * must be too; we do not need an explicit range-limiting operation.
       * Hence the value being shifted is never negative, and we don't
       * need the general RIGHT__SHIFT macro.
       */
      /* Y */
      outptr0[col] = (JSAMPLE)
		((ctab[r+R__Y__OFF] + ctab[g+G__Y__OFF] + ctab[b+B__Y__OFF])
		 >> SCALEBITS);
      /* Cb */
      outptr1[col] = (JSAMPLE)
		((ctab[r+R__CB__OFF] + ctab[g+G__CB__OFF] + ctab[b+B__CB__OFF])
		 >> SCALEBITS);
      /* Cr */
      outptr2[col] = (JSAMPLE)
		((ctab[r+R__CR__OFF] + ctab[g+G__CR__OFF] + ctab[b+B__CR__OFF])
		 >> SCALEBITS);
    }
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles grayscale output with no conversion.
 * The source can be either plain grayscale or YCbCr (since Y == gray).
 */

METHODDEF(void)
grayscale__convert (j__compress__ptr cinfo,
		   JSAMPARRAY input__buf, JSAMPIMAGE output__buf,
		   JDIMENSION output__row, int num__rows)
{
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  JDIMENSION num__cols = cinfo->image__width;
  int instride = cinfo->input__components;

  while (--num__rows >= 0) {
    inptr = *input__buf++;
    outptr = output__buf[0][output__row];
    output__row++;
    for (col = 0; col < num__cols; col++) {
      outptr[col] = inptr[0];	/* don't need GETJSAMPLE() here */
      inptr += instride;
    }
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles multi-component colorspaces without conversion.
 * We assume input__components == num__components.
 */

METHODDEF(void)
null__convert (j__compress__ptr cinfo,
	      JSAMPARRAY input__buf, JSAMPIMAGE output__buf,
	      JDIMENSION output__row, int num__rows)
{
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  register int ci;
  int nc = cinfo->num__components;
  JDIMENSION num__cols = cinfo->image__width;

  while (--num__rows >= 0) {
    /* It seems fastest to make a separate pass for each component. */
    for (ci = 0; ci < nc; ci++) {
      inptr = *input__buf;
      outptr = output__buf[ci][output__row];
      for (col = 0; col < num__cols; col++) {
	outptr[col] = inptr[ci]; /* don't need GETJSAMPLE() here */
	inptr += nc;
      }
    }
    input__buf++;
    output__row++;
  }
}


/*
 * Empty method for start__pass.
 */

METHODDEF(void)
null__method (j__compress__ptr cinfo)
{
  /* no work needed */
}


/*
 * Module initialization routine for input colorspace conversion.
 */

GLOBAL(void)
jinit__color__converter (j__compress__ptr cinfo)
{
  my__cconvert__ptr cconvert;

  cconvert = (my__cconvert__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__color__converter));
  cinfo->cconvert = (struct jpeg__color__converter *) cconvert;
  /* set start__pass to null method until we find out differently */
  cconvert->pub.start__pass = null__method;

  /* Make sure input__components agrees with in__color__space */
  switch (cinfo->in__color__space) {
  case JCS__GRAYSCALE:
    if (cinfo->input__components != 1)
      ERREXIT(cinfo, JERR__BAD__IN__COLORSPACE);
    break;

  case JCS__RGB:
#if RGB__PIXELSIZE != 3
    if (cinfo->input__components != RGB__PIXELSIZE)
      ERREXIT(cinfo, JERR__BAD__IN__COLORSPACE);
    break;
#endif /* else share code with YCbCr */

  case JCS__YCbCr:
    if (cinfo->input__components != 3)
      ERREXIT(cinfo, JERR__BAD__IN__COLORSPACE);
    break;

  case JCS__CMYK:
  case JCS__YCCK:
    if (cinfo->input__components != 4)
      ERREXIT(cinfo, JERR__BAD__IN__COLORSPACE);
    break;

  default:			/* JCS__UNKNOWN can be anything */
    if (cinfo->input__components < 1)
      ERREXIT(cinfo, JERR__BAD__IN__COLORSPACE);
    break;
  }

  /* Check num__components, set conversion method based on requested space */
  switch (cinfo->jpeg__color__space) {
  case JCS__GRAYSCALE:
    if (cinfo->num__components != 1)
      ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
    if (cinfo->in__color__space == JCS__GRAYSCALE)
      cconvert->pub.color__convert = grayscale__convert;
    else if (cinfo->in__color__space == JCS__RGB) {
      cconvert->pub.start__pass = rgb__ycc__start;
      cconvert->pub.color__convert = rgb__gray__convert;
    } else if (cinfo->in__color__space == JCS__YCbCr)
      cconvert->pub.color__convert = grayscale__convert;
    else
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    break;

  case JCS__RGB:
    if (cinfo->num__components != 3)
      ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
    if (cinfo->in__color__space == JCS__RGB && RGB__PIXELSIZE == 3)
      cconvert->pub.color__convert = null__convert;
    else
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    break;

  case JCS__YCbCr:
    if (cinfo->num__components != 3)
      ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
    if (cinfo->in__color__space == JCS__RGB) {
      cconvert->pub.start__pass = rgb__ycc__start;
      cconvert->pub.color__convert = rgb__ycc__convert;
    } else if (cinfo->in__color__space == JCS__YCbCr)
      cconvert->pub.color__convert = null__convert;
    else
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    break;

  case JCS__CMYK:
    if (cinfo->num__components != 4)
      ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
    if (cinfo->in__color__space == JCS__CMYK)
      cconvert->pub.color__convert = null__convert;
    else
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    break;

  case JCS__YCCK:
    if (cinfo->num__components != 4)
      ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
    if (cinfo->in__color__space == JCS__CMYK) {
      cconvert->pub.start__pass = rgb__ycc__start;
      cconvert->pub.color__convert = cmyk__ycck__convert;
    } else if (cinfo->in__color__space == JCS__YCCK)
      cconvert->pub.color__convert = null__convert;
    else
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    break;

  default:			/* allow null conversion of JCS__UNKNOWN */
    if (cinfo->jpeg__color__space != cinfo->in__color__space ||
	cinfo->num__components != cinfo->input__components)
      ERREXIT(cinfo, JERR__CONVERSION__NOTIMPL);
    cconvert->pub.color__convert = null__convert;
    break;
  }
}
