/*
 * jcparam.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains optional default-setting code for the JPEG compressor.
 * Applications do not have to use this file, but those that don't use it
 * must know a lot more about the innards of the JPEG code.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/*
 * Quantization table setup routines
 */

GLOBAL(void)
jpeg__add__quant__table (j__compress__ptr cinfo, int whiCH_tbl,
		      const unsigned int *basic__table,
		      int scale__factor, CH_BOOL force__baseline)
/* Define a quantization table equal to the basic__table times
 * a scale factor (given as a percentage).
 * If force__baseline is TRUE, the computed quantization table entries
 * are limited to 1..255 for JPEG baseline compatibility.
 */
{
  JQUANT__TBL ** qtblptr;
  int i;
  long temp;

  /* Safety check to ensure start__compress not called yet. */
  if (cinfo->global__state != CSTATE__START)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  if (whiCH_tbl < 0 || whiCH_tbl >= NUM__QUANT__TBLS)
    ERREXIT1(cinfo, JERR__DQT__INDEX, whiCH_tbl);

  qtblptr = & cinfo->quant__tbl__ptrs[whiCH_tbl];

  if (*qtblptr == NULL)
    *qtblptr = jpeg__alloc__quant__table((j__common__ptr) cinfo);

  for (i = 0; i < DCTSIZE2; i++) {
    temp = ((long) basic__table[i] * scale__factor + 50L) / 100L;
    /* limit the values to the valid range */
    if (temp <= 0L) temp = 1L;
    if (temp > 32767L) temp = 32767L; /* max quantizer needed for 12 bits */
    if (force__baseline && temp > 255L)
      temp = 255L;		/* limit to baseline range if requested */
    (*qtblptr)->quantval[i] = (UINT16) temp;
  }

  /* Initialize sent__table FALSE so table will be written to JPEG file. */
  (*qtblptr)->sent__table = FALSE;
}


GLOBAL(void)
jpeg__set__linear__quality (j__compress__ptr cinfo, int scale__factor,
			 CH_BOOL force__baseline)
/* Set or change the 'quality' (quantization) setting, using default tables
 * and a straight percentage-scaling quality scale.  In most cases it's better
 * to use jpeg__set__quality (below); this entry point is provided for
 * applications that insist on a linear percentage scaling.
 */
{
  /* These are the sample quantization tables given in JPEG spec section K.1.
   * The spec says that the values given produce "good" quality, and
   * when divided by 2, "very good" quality.
   */
  static const unsigned int std__luminance__quant__tbl[DCTSIZE2] = {
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
  };
  static const unsigned int std__chrominance__quant__tbl[DCTSIZE2] = {
    17,  18,  24,  47,  99,  99,  99,  99,
    18,  21,  26,  66,  99,  99,  99,  99,
    24,  26,  56,  99,  99,  99,  99,  99,
    47,  66,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99
  };

  /* Set up two quantization tables using the specified scaling */
  jpeg__add__quant__table(cinfo, 0, std__luminance__quant__tbl,
		       scale__factor, force__baseline);
  jpeg__add__quant__table(cinfo, 1, std__chrominance__quant__tbl,
		       scale__factor, force__baseline);
}


GLOBAL(int)
jpeg__quality__scaling (int quality)
/* Convert a user-specified quality rating to a percentage scaling factor
 * for an underlying quantization table, using our recommended scaling curve.
 * The input 'quality' factor should be 0 (terrible) to 100 (very good).
 */
{
  /* Safety limit on quality factor.  Convert 0 to 1 to avoid zero divide. */
  if (quality <= 0) quality = 1;
  if (quality > 100) quality = 100;

  /* The basic table is used as-is (scaling 100) for a quality of 50.
   * Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
   * note that at Q=100 the scaling is 0, which will cause jpeg__add__quant__table
   * to make all the table entries 1 (hence, minimum quantization loss).
   * Qualities 1..50 are converted to scaling percentage 5000/Q.
   */
  if (quality < 50)
    quality = 5000 / quality;
  else
    quality = 200 - quality*2;

  return quality;
}


GLOBAL(void)
jpeg__set__quality (j__compress__ptr cinfo, int quality, CH_BOOL force__baseline)
/* Set or change the 'quality' (quantization) setting, using default tables.
 * This is the standard quality-adjusting entry point for typical user
 * interfaces; only those who want detailed control over quantization tables
 * would use the preceding three routines directly.
 */
{
  /* Convert user 0-100 rating to percentage scaling */
  quality = jpeg__quality__scaling(quality);

  /* Set up standard quality tables */
  jpeg__set__linear__quality(cinfo, quality, force__baseline);
}


/*
 * Huffman table setup routines
 */

LOCAL(void)
add__huff__table (j__compress__ptr cinfo,
		JHUFF__TBL **htblptr, const UINT8 *bits, const UINT8 *val)
/* Define a Huffman table */
{
  int nsymbols, len;

  if (*htblptr == NULL)
    *htblptr = jpeg__alloc__huff__table((j__common__ptr) cinfo);

  /* Copy the number-of-symbols-of-each-code-length counts */
  MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));

  /* Validate the counts.  We do this here mainly so we can copy the right
   * number of symbols from the val[] array, without risking marching off
   * the end of memory.  jchuff.c will do a more thorough test later.
   */
  nsymbols = 0;
  for (len = 1; len <= 16; len++)
    nsymbols += bits[len];
  if (nsymbols < 1 || nsymbols > 256)
    ERREXIT(cinfo, JERR__BAD__HUFF__TABLE);

  MEMCOPY((*htblptr)->huffval, val, nsymbols * SIZEOF(UINT8));

  /* Initialize sent__table FALSE so table will be written to JPEG file. */
  (*htblptr)->sent__table = FALSE;
}


LOCAL(void)
std__huff__tables (j__compress__ptr cinfo)
/* Set up the standard Huffman tables (cf. JPEG standard section K.3) */
/* IMPORTANT: these are only valid for 8-bit data precision! */
{
  static const UINT8 bits__dc__luminance[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const UINT8 val__dc__luminance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 bits__dc__chrominance[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const UINT8 val__dc__chrominance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 bits__ac__luminance[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const UINT8 val__ac__luminance[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
      0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
      0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
      0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
      0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
      0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
      0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
      0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
      0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  static const UINT8 bits__ac__chrominance[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const UINT8 val__ac__chrominance[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
      0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
      0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
      0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
      0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
      0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
      0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
      0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
      0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
      0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
      0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
      0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
      0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
      0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
      0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
      0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  add__huff__table(cinfo, &cinfo->dc__huff__tbl__ptrs[0],
		 bits__dc__luminance, val__dc__luminance);
  add__huff__table(cinfo, &cinfo->ac__huff__tbl__ptrs[0],
		 bits__ac__luminance, val__ac__luminance);
  add__huff__table(cinfo, &cinfo->dc__huff__tbl__ptrs[1],
		 bits__dc__chrominance, val__dc__chrominance);
  add__huff__table(cinfo, &cinfo->ac__huff__tbl__ptrs[1],
		 bits__ac__chrominance, val__ac__chrominance);
}


/*
 * Default parameter setup for compression.
 *
 * Applications that don't choose to use this routine must do their
 * own setup of all these parameters.  Alternately, you can call this
 * to establish defaults and then alter parameters selectively.  This
 * is the recommended approach since, if we add any new parameters,
 * your code will still work (they'll be set to reasonable defaults).
 */

GLOBAL(void)
jpeg__set__defaults (j__compress__ptr cinfo)
{
  int i;

  /* Safety check to ensure start__compress not called yet. */
  if (cinfo->global__state != CSTATE__START)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  /* Allocate comp__info array large enough for maximum component count.
   * Array is made permanent in case application wants to compress
   * multiple images at same param settings.
   */
  if (cinfo->comp__info == NULL)
    cinfo->comp__info = (jpeg__component__info *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__PERMANENT,
				  MAX__COMPONENTS * SIZEOF(jpeg__component__info));

  /* Initialize everything not dependent on the color space */

  cinfo->data__precision = BITS__IN__JSAMPLE;
  /* Set up two quantization tables using default quality of 75 */
  jpeg__set__quality(cinfo, 75, TRUE);
  /* Set up two Huffman tables */
  std__huff__tables(cinfo);

  /* Initialize default arithmetic coding conditioning */
  for (i = 0; i < NUM__ARITH__TBLS; i++) {
    cinfo->arith__dc__L[i] = 0;
    cinfo->arith__dc__U[i] = 1;
    cinfo->arith__ac__K[i] = 5;
  }

  /* Default is no multiple-scan output */
  cinfo->scan__info = NULL;
  cinfo->num__scans = 0;

  /* Expect normal source image, not raw downsampled data */
  cinfo->raw__data__in = FALSE;

  /* Use Huffman coding, not arithmetic coding, by default */
  cinfo->arith__code = FALSE;

  /* By default, don't do extra passes to optimize entropy coding */
  cinfo->optimize__coding = FALSE;
  /* The standard Huffman tables are only valid for 8-bit data precision.
   * If the precision is higher, force optimization on so that usable
   * tables will be computed.  This test can be removed if default tables
   * are supplied that are valid for the desired precision.
   */
  if (cinfo->data__precision > 8)
    cinfo->optimize__coding = TRUE;

  /* By default, use the simpler non-cosited sampling alignment */
  cinfo->CCIR601__sampling = FALSE;

  /* No input smoothing */
  cinfo->smoothing__factor = 0;

  /* DCT algorithm preference */
  cinfo->dct__method = JDCT__DEFAULT;

  /* No restart markers */
  cinfo->restart__interval = 0;
  cinfo->restart__in__rows = 0;

  /* Fill in default JFIF marker parameters.  Note that whether the marker
   * will actually be written is determined by jpeg__set__colorspace.
   *
   * By default, the library emits JFIF version code 1.01.
   * An application that wants to emit JFIF 1.02 extension markers should set
   * JFIF__minor__version to 2.  We could probably get away with just defaulting
   * to 1.02, but there may still be some decoders in use that will complain
   * about that; saying 1.01 should minimize compatibility problems.
   */
  cinfo->JFIF__major__version = 1; /* Default JFIF version = 1.01 */
  cinfo->JFIF__minor__version = 1;
  cinfo->density__unit = 0;	/* Pixel size is unknown by default */
  cinfo->X__density = 1;		/* Pixel aspect ratio is square by default */
  cinfo->Y__density = 1;

  /* Choose JPEG colorspace based on input space, set defaults accordingly */

  jpeg__default__colorspace(cinfo);
}


/*
 * Select an appropriate JPEG colorspace for in__color__space.
 */

GLOBAL(void)
jpeg__default__colorspace (j__compress__ptr cinfo)
{
  switch (cinfo->in__color__space) {
  case JCS__GRAYSCALE:
    jpeg__set__colorspace(cinfo, JCS__GRAYSCALE);
    break;
  case JCS__RGB:
    jpeg__set__colorspace(cinfo, JCS__YCbCr);
    break;
  case JCS__YCbCr:
    jpeg__set__colorspace(cinfo, JCS__YCbCr);
    break;
  case JCS__CMYK:
    jpeg__set__colorspace(cinfo, JCS__CMYK); /* By default, no translation */
    break;
  case JCS__YCCK:
    jpeg__set__colorspace(cinfo, JCS__YCCK);
    break;
  case JCS__UNKNOWN:
    jpeg__set__colorspace(cinfo, JCS__UNKNOWN);
    break;
  default:
    ERREXIT(cinfo, JERR__BAD__IN__COLORSPACE);
  }
}


/*
 * Set the JPEG colorspace, and choose colorspace-dependent default values.
 */

GLOBAL(void)
jpeg__set__colorspace (j__compress__ptr cinfo, J__COLOR__SPACE colorspace)
{
  jpeg__component__info * compptr;
  int ci;

#define SET__COMP(index,id,hsamp,vsamp,quant,dctbl,actbl)  \
  (compptr = &cinfo->comp__info[index], \
   compptr->component__id = (id), \
   compptr->h__samp__factor = (hsamp), \
   compptr->v__samp__factor = (vsamp), \
   compptr->quant__tbl__no = (quant), \
   compptr->dc__tbl__no = (dctbl), \
   compptr->ac__tbl__no = (actbl) )

  /* Safety check to ensure start__compress not called yet. */
  if (cinfo->global__state != CSTATE__START)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  /* For all colorspaces, we use Q and Huff tables 0 for luminance components,
   * tables 1 for chrominance components.
   */

  cinfo->jpeg__color__space = colorspace;

  cinfo->write__JFIF__header = FALSE; /* No marker for non-JFIF colorspaces */
  cinfo->write__Adobe__marker = FALSE; /* write no Adobe marker by default */

  switch (colorspace) {
  case JCS__GRAYSCALE:
    cinfo->write__JFIF__header = TRUE; /* Write a JFIF marker */
    cinfo->num__components = 1;
    /* JFIF specifies component ID 1 */
    SET__COMP(0, 1, 1,1, 0, 0,0);
    break;
  case JCS__RGB:
    cinfo->write__Adobe__marker = TRUE; /* write Adobe marker to flag RGB */
    cinfo->num__components = 3;
    SET__COMP(0, 0x52 /* 'R' */, 1,1, 0, 0,0);
    SET__COMP(1, 0x47 /* 'G' */, 1,1, 0, 0,0);
    SET__COMP(2, 0x42 /* 'B' */, 1,1, 0, 0,0);
    break;
  case JCS__YCbCr:
    cinfo->write__JFIF__header = TRUE; /* Write a JFIF marker */
    cinfo->num__components = 3;
    /* JFIF specifies component IDs 1,2,3 */
    /* We default to 2x2 subsamples of chrominance */
    SET__COMP(0, 1, 2,2, 0, 0,0);
    SET__COMP(1, 2, 1,1, 1, 1,1);
    SET__COMP(2, 3, 1,1, 1, 1,1);
    break;
  case JCS__CMYK:
    cinfo->write__Adobe__marker = TRUE; /* write Adobe marker to flag CMYK */
    cinfo->num__components = 4;
    SET__COMP(0, 0x43 /* 'C' */, 1,1, 0, 0,0);
    SET__COMP(1, 0x4D /* 'M' */, 1,1, 0, 0,0);
    SET__COMP(2, 0x59 /* 'Y' */, 1,1, 0, 0,0);
    SET__COMP(3, 0x4B /* 'K' */, 1,1, 0, 0,0);
    break;
  case JCS__YCCK:
    cinfo->write__Adobe__marker = TRUE; /* write Adobe marker to flag YCCK */
    cinfo->num__components = 4;
    SET__COMP(0, 1, 2,2, 0, 0,0);
    SET__COMP(1, 2, 1,1, 1, 1,1);
    SET__COMP(2, 3, 1,1, 1, 1,1);
    SET__COMP(3, 4, 2,2, 0, 0,0);
    break;
  case JCS__UNKNOWN:
    cinfo->num__components = cinfo->input__components;
    if (cinfo->num__components < 1 || cinfo->num__components > MAX__COMPONENTS)
      ERREXIT2(cinfo, JERR__COMPONENT__COUNT, cinfo->num__components,
	       MAX__COMPONENTS);
    for (ci = 0; ci < cinfo->num__components; ci++) {
      SET__COMP(ci, ci, 1,1, 0, 0,0);
    }
    break;
  default:
    ERREXIT(cinfo, JERR__BAD__J__COLORSPACE);
  }
}


#ifdef C__PROGRESSIVE__SUPPORTED

LOCAL(jpeg__scan__info *)
fill__a__scan (jpeg__scan__info * scanptr, int ci,
	     int Ss, int Se, int Ah, int Al)
/* Support routine: generate one scan for specified component */
{
  scanptr->comps__in__scan = 1;
  scanptr->component__index[0] = ci;
  scanptr->Ss = Ss;
  scanptr->Se = Se;
  scanptr->Ah = Ah;
  scanptr->Al = Al;
  scanptr++;
  return scanptr;
}

LOCAL(jpeg__scan__info *)
fill__scans (jpeg__scan__info * scanptr, int ncomps,
	    int Ss, int Se, int Ah, int Al)
/* Support routine: generate one scan for each component */
{
  int ci;

  for (ci = 0; ci < ncomps; ci++) {
    scanptr->comps__in__scan = 1;
    scanptr->component__index[0] = ci;
    scanptr->Ss = Ss;
    scanptr->Se = Se;
    scanptr->Ah = Ah;
    scanptr->Al = Al;
    scanptr++;
  }
  return scanptr;
}

LOCAL(jpeg__scan__info *)
fill__dc__scans (jpeg__scan__info * scanptr, int ncomps, int Ah, int Al)
/* Support routine: generate interleaved DC scan if possible, else N scans */
{
  int ci;

  if (ncomps <= MAX__COMPS__IN__SCAN) {
    /* Single interleaved DC scan */
    scanptr->comps__in__scan = ncomps;
    for (ci = 0; ci < ncomps; ci++)
      scanptr->component__index[ci] = ci;
    scanptr->Ss = scanptr->Se = 0;
    scanptr->Ah = Ah;
    scanptr->Al = Al;
    scanptr++;
  } else {
    /* Noninterleaved DC scan for each component */
    scanptr = fill__scans(scanptr, ncomps, 0, 0, Ah, Al);
  }
  return scanptr;
}


/*
 * Create a recommended progressive-JPEG script.
 * cinfo->num__components and cinfo->jpeg__color__space must be correct.
 */

GLOBAL(void)
jpeg__simple__progression (j__compress__ptr cinfo)
{
  int ncomps = cinfo->num__components;
  int nscans;
  jpeg__scan__info * scanptr;

  /* Safety check to ensure start__compress not called yet. */
  if (cinfo->global__state != CSTATE__START)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  /* Figure space needed for script.  Calculation must match code below! */
  if (ncomps == 3 && cinfo->jpeg__color__space == JCS__YCbCr) {
    /* Custom script for YCbCr color images. */
    nscans = 10;
  } else {
    /* All-purpose script for other color spaces. */
    if (ncomps > MAX__COMPS__IN__SCAN)
      nscans = 6 * ncomps;	/* 2 DC + 4 AC scans per component */
    else
      nscans = 2 + 4 * ncomps;	/* 2 DC scans; 4 AC scans per component */
  }

  /* Allocate space for script.
   * We need to put it in the permanent pool in case the application performs
   * multiple compressions without changing the settings.  To avoid a memory
   * leak if jpeg__simple__progression is called repeatedly for the same JPEG
   * object, we try to re-use previously allocated space, and we allocate
   * enough space to handle YCbCr even if initially asked for grayscale.
   */
  if (cinfo->script__space == NULL || cinfo->script__space__size < nscans) {
    cinfo->script__space__size = MAX(nscans, 10);
    cinfo->script__space = (jpeg__scan__info *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__PERMANENT,
			cinfo->script__space__size * SIZEOF(jpeg__scan__info));
  }
  scanptr = cinfo->script__space;
  cinfo->scan__info = scanptr;
  cinfo->num__scans = nscans;

  if (ncomps == 3 && cinfo->jpeg__color__space == JCS__YCbCr) {
    /* Custom script for YCbCr color images. */
    /* Initial DC scan */
    scanptr = fill__dc__scans(scanptr, ncomps, 0, 1);
    /* Initial AC scan: get some luma data out in a hurry */
    scanptr = fill__a__scan(scanptr, 0, 1, 5, 0, 2);
    /* Chroma data is too small to be worth expending many scans on */
    scanptr = fill__a__scan(scanptr, 2, 1, 63, 0, 1);
    scanptr = fill__a__scan(scanptr, 1, 1, 63, 0, 1);
    /* Complete spectral selection for luma AC */
    scanptr = fill__a__scan(scanptr, 0, 6, 63, 0, 2);
    /* Refine next bit of luma AC */
    scanptr = fill__a__scan(scanptr, 0, 1, 63, 2, 1);
    /* Finish DC successive approximation */
    scanptr = fill__dc__scans(scanptr, ncomps, 1, 0);
    /* Finish AC successive approximation */
    scanptr = fill__a__scan(scanptr, 2, 1, 63, 1, 0);
    scanptr = fill__a__scan(scanptr, 1, 1, 63, 1, 0);
    /* Luma bottom bit comes last since it's usually largest scan */
    scanptr = fill__a__scan(scanptr, 0, 1, 63, 1, 0);
  } else {
    /* All-purpose script for other color spaces. */
    /* Successive approximation first pass */
    scanptr = fill__dc__scans(scanptr, ncomps, 0, 1);
    scanptr = fill__scans(scanptr, ncomps, 1, 5, 0, 2);
    scanptr = fill__scans(scanptr, ncomps, 6, 63, 0, 2);
    /* Successive approximation second pass */
    scanptr = fill__scans(scanptr, ncomps, 1, 63, 2, 1);
    /* Successive approximation final pass */
    scanptr = fill__dc__scans(scanptr, ncomps, 1, 0);
    scanptr = fill__scans(scanptr, ncomps, 1, 63, 1, 0);
  }
}

#endif /* C__PROGRESSIVE__SUPPORTED */
