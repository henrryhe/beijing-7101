/*
 * jcmarker.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write JPEG datastream markers.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


typedef enum {			/* JPEG marker codes */
  M__SOF0  = 0xc0,
  M__SOF1  = 0xc1,
  M__SOF2  = 0xc2,
  M__SOF3  = 0xc3,
  
  M__SOF5  = 0xc5,
  M__SOF6  = 0xc6,
  M__SOF7  = 0xc7,
  
  M__JPG   = 0xc8,
  M__SOF9  = 0xc9,
  M__SOF10 = 0xca,
  M__SOF11 = 0xcb,
  
  M__SOF13 = 0xcd,
  M__SOF14 = 0xce,
  M__SOF15 = 0xcf,
  
  M__DHT   = 0xc4,
  
  M__DAC   = 0xcc,
  
  M__RST0  = 0xd0,
  M__RST1  = 0xd1,
  M__RST2  = 0xd2,
  M__RST3  = 0xd3,
  M__RST4  = 0xd4,
  M__RST5  = 0xd5,
  M__RST6  = 0xd6,
  M__RST7  = 0xd7,
  
  M__SOI   = 0xd8,
  M__EOI   = 0xd9,
  M__SOS   = 0xda,
  M__DQT   = 0xdb,
  M__DNL   = 0xdc,
  M__DRI   = 0xdd,
  M__DHP   = 0xde,
  M__EXP   = 0xdf,
  
  M__APP0  = 0xe0,
  M__APP1  = 0xe1,
  M__APP2  = 0xe2,
  M__APP3  = 0xe3,
  M__APP4  = 0xe4,
  M__APP5  = 0xe5,
  M__APP6  = 0xe6,
  M__APP7  = 0xe7,
  M__APP8  = 0xe8,
  M__APP9  = 0xe9,
  M__APP10 = 0xea,
  M__APP11 = 0xeb,
  M__APP12 = 0xec,
  M__APP13 = 0xed,
  M__APP14 = 0xee,
  M__APP15 = 0xef,
  
  M__JPG0  = 0xf0,
  M__JPG13 = 0xfd,
  M__COM   = 0xfe,
  
  M__TEM   = 0x01,
  
  M__ERROR = 0x100
} JPEG__MARKER;


/* Private state */

typedef struct {
  struct jpeg__marker__writer pub; /* public fields */

  unsigned int last__restart__interval; /* last DRI value emitted; 0 after SOI */
} my__marker__writer;

typedef my__marker__writer * my__marker__ptr;


/*
 * Basic output routines.
 *
 * Note that we do not support suspension while writing a marker.
 * Therefore, an application using suspension must ensure that there is
 * enough buffer space for the initial markers (typ. 600-700 bytes) before
 * calling jpeg__start__compress, and enough space to write the trailing EOI
 * (a few bytes) before calling jpeg__finish__compress.  Multipass compression
 * modes are not supported at all with suspension, so those two are the only
 * points where markers will be written.
 */

LOCAL(void)
emit__byte (j__compress__ptr cinfo, int val)
/* Emit a byte */
{
  struct jpeg__destination__mgr * dest = cinfo->dest;

  *(dest->next__output__byte)++ = (JOCTET) val;
  if (--dest->free__in__buffer == 0) {
    if (! (*dest->empty__output__buffer) (cinfo))
      ERREXIT(cinfo, JERR__CANT__SUSPEND);
  }
}


LOCAL(void)
emit__marker (j__compress__ptr cinfo, JPEG__MARKER mark)
/* Emit a marker code */
{
  emit__byte(cinfo, 0xFF);
  emit__byte(cinfo, (int) mark);
}


LOCAL(void)
emit__2bytes (j__compress__ptr cinfo, int value)
/* Emit a 2-byte integer; these are always MSB first in JPEG files */
{
  emit__byte(cinfo, (value >> 8) & 0xFF);
  emit__byte(cinfo, value & 0xFF);
}


/*
 * Routines to write specific marker types.
 */

LOCAL(int)
emit__dqt (j__compress__ptr cinfo, int index)
/* Emit a DQT marker */
/* Returns the precision used (0 = 8bits, 1 = 16bits) for baseline checking */
{
  JQUANT__TBL * qtbl = cinfo->quant__tbl__ptrs[index];
  int prec;
  int i;

  if (qtbl == NULL)
    ERREXIT1(cinfo, JERR__NO__QUANT__TABLE, index);

  prec = 0;
  for (i = 0; i < DCTSIZE2; i++) {
    if (qtbl->quantval[i] > 255)
      prec = 1;
  }

  if (! qtbl->sent__table) {
    emit__marker(cinfo, M__DQT);

    emit__2bytes(cinfo, prec ? DCTSIZE2*2 + 1 + 2 : DCTSIZE2 + 1 + 2);

    emit__byte(cinfo, index + (prec<<4));

    for (i = 0; i < DCTSIZE2; i++) {
      /* The table entries must be emitted in zigzag order. */
      unsigned int qval = qtbl->quantval[jpeg__natural__order[i]];
      if (prec)
	emit__byte(cinfo, (int) (qval >> 8));
      emit__byte(cinfo, (int) (qval & 0xFF));
    }

    qtbl->sent__table = TRUE;
  }

  return prec;
}


LOCAL(void)
emit__dht (j__compress__ptr cinfo, int index, CH_BOOL is__ac)
/* Emit a DHT marker */
{
  JHUFF__TBL * htbl;
  int length, i;
  
  if (is__ac) {
    htbl = cinfo->ac__huff__tbl__ptrs[index];
    index += 0x10;		/* output index has AC bit set */
  } else {
    htbl = cinfo->dc__huff__tbl__ptrs[index];
  }

  if (htbl == NULL)
    ERREXIT1(cinfo, JERR__NO__HUFF__TABLE, index);
  
  if (! htbl->sent__table) {
    emit__marker(cinfo, M__DHT);
    
    length = 0;
    for (i = 1; i <= 16; i++)
      length += htbl->bits[i];
    
    emit__2bytes(cinfo, length + 2 + 1 + 16);
    emit__byte(cinfo, index);
    
    for (i = 1; i <= 16; i++)
      emit__byte(cinfo, htbl->bits[i]);
    
    for (i = 0; i < length; i++)
      emit__byte(cinfo, htbl->huffval[i]);
    
    htbl->sent__table = TRUE;
  }
}


LOCAL(void)
emit__dac (j__compress__ptr cinfo)
/* Emit a DAC marker */
/* Since the useful info is so small, we want to emit all the tables in */
/* one DAC marker.  Therefore this routine does its own scan of the table. */
{
#ifdef C__ARITH__CODING__SUPPORTED
  char dc__in__use[NUM__ARITH__TBLS];
  char ac__in__use[NUM__ARITH__TBLS];
  int length, i;
  jpeg__component__info *compptr;
  
  for (i = 0; i < NUM__ARITH__TBLS; i++)
    dc__in__use[i] = ac__in__use[i] = 0;
  
  for (i = 0; i < cinfo->comps__in__scan; i++) {
    compptr = cinfo->cur__comp__info[i];
    dc__in__use[compptr->dc__tbl__no] = 1;
    ac__in__use[compptr->ac__tbl__no] = 1;
  }
  
  length = 0;
  for (i = 0; i < NUM__ARITH__TBLS; i++)
    length += dc__in__use[i] + ac__in__use[i];
  
  emit__marker(cinfo, M__DAC);
  
  emit__2bytes(cinfo, length*2 + 2);
  
  for (i = 0; i < NUM__ARITH__TBLS; i++) {
    if (dc__in__use[i]) {
      emit__byte(cinfo, i);
      emit__byte(cinfo, cinfo->arith__dc__L[i] + (cinfo->arith__dc__U[i]<<4));
    }
    if (ac__in__use[i]) {
      emit__byte(cinfo, i + 0x10);
      emit__byte(cinfo, cinfo->arith__ac__K[i]);
    }
  }
#endif /* C__ARITH__CODING__SUPPORTED */
}


LOCAL(void)
emit__dri (j__compress__ptr cinfo)
/* Emit a DRI marker */
{
  emit__marker(cinfo, M__DRI);
  
  emit__2bytes(cinfo, 4);	/* fixed length */

  emit__2bytes(cinfo, (int) cinfo->restart__interval);
}


LOCAL(void)
emit__sof (j__compress__ptr cinfo, JPEG__MARKER code)
/* Emit a SOF marker */
{
  int ci;
  jpeg__component__info *compptr;
  
  emit__marker(cinfo, code);
  
  emit__2bytes(cinfo, 3 * cinfo->num__components + 2 + 5 + 1); /* length */

  /* Make sure image isn't bigger than SOF field can handle */
  if ((long) cinfo->image__height > 65535L ||
      (long) cinfo->image__width > 65535L)
    ERREXIT1(cinfo, JERR__IMAGE__TOO__BIG, (unsigned int) 65535);

  emit__byte(cinfo, cinfo->data__precision);
  emit__2bytes(cinfo, (int) cinfo->image__height);
  emit__2bytes(cinfo, (int) cinfo->image__width);

  emit__byte(cinfo, cinfo->num__components);

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    emit__byte(cinfo, compptr->component__id);
    emit__byte(cinfo, (compptr->h__samp__factor << 4) + compptr->v__samp__factor);
    emit__byte(cinfo, compptr->quant__tbl__no);
  }
}


LOCAL(void)
emit__sos (j__compress__ptr cinfo)
/* Emit a SOS marker */
{
  int i, td, ta;
  jpeg__component__info *compptr;
  
  emit__marker(cinfo, M__SOS);
  
  emit__2bytes(cinfo, 2 * cinfo->comps__in__scan + 2 + 1 + 3); /* length */
  
  emit__byte(cinfo, cinfo->comps__in__scan);
  
  for (i = 0; i < cinfo->comps__in__scan; i++) {
    compptr = cinfo->cur__comp__info[i];
    emit__byte(cinfo, compptr->component__id);
    td = compptr->dc__tbl__no;
    ta = compptr->ac__tbl__no;
    if (cinfo->progressive__mode) {
      /* Progressive mode: only DC or only AC tables are used in one scan;
       * furthermore, Huffman coding of DC refinement uses no table at all.
       * We emit 0 for unused field(s); this is recommended by the P&M text
       * but does not seem to be specified in the standard.
       */
      if (cinfo->Ss == 0) {
	ta = 0;			/* DC scan */
	if (cinfo->Ah != 0 && !cinfo->arith__code)
	  td = 0;		/* no DC table either */
      } else {
	td = 0;			/* AC scan */
      }
    }
    emit__byte(cinfo, (td << 4) + ta);
  }

  emit__byte(cinfo, cinfo->Ss);
  emit__byte(cinfo, cinfo->Se);
  emit__byte(cinfo, (cinfo->Ah << 4) + cinfo->Al);
}


LOCAL(void)
emit__jfif__app0 (j__compress__ptr cinfo)
/* Emit a JFIF-compliant APP0 marker */
{
  /*
   * Length of APP0 block	(2 bytes)
   * Block ID			(4 bytes - ASCII "JFIF")
   * Zero byte			(1 byte to terminate the ID string)
   * Version Major, Minor	(2 bytes - major first)
   * Units			(1 byte - 0x00 = none, 0x01 = inch, 0x02 = cm)
   * Xdpu			(2 bytes - dots per unit horizontal)
   * Ydpu			(2 bytes - dots per unit vertical)
   * Thumbnail X size		(1 byte)
   * Thumbnail Y size		(1 byte)
   */
  
  emit__marker(cinfo, M__APP0);
  
  emit__2bytes(cinfo, 2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1); /* length */

  emit__byte(cinfo, 0x4A);	/* Identifier: ASCII "JFIF" */
  emit__byte(cinfo, 0x46);
  emit__byte(cinfo, 0x49);
  emit__byte(cinfo, 0x46);
  emit__byte(cinfo, 0);
  emit__byte(cinfo, cinfo->JFIF__major__version); /* Version fields */
  emit__byte(cinfo, cinfo->JFIF__minor__version);
  emit__byte(cinfo, cinfo->density__unit); /* Pixel size information */
  emit__2bytes(cinfo, (int) cinfo->X__density);
  emit__2bytes(cinfo, (int) cinfo->Y__density);
  emit__byte(cinfo, 0);		/* No thumbnail image */
  emit__byte(cinfo, 0);
}


LOCAL(void)
emit__adobe__app14 (j__compress__ptr cinfo)
/* Emit an Adobe APP14 marker */
{
  /*
   * Length of APP14 block	(2 bytes)
   * Block ID			(5 bytes - ASCII "Adobe")
   * Version Number		(2 bytes - currently 100)
   * Flags0			(2 bytes - currently 0)
   * Flags1			(2 bytes - currently 0)
   * Color transform		(1 byte)
   *
   * Although Adobe TN 5116 mentions Version = 101, all the Adobe files
   * now in circulation seem to use Version = 100, so that's what we write.
   *
   * We write the color transform byte as 1 if the JPEG color space is
   * YCbCr, 2 if it's YCCK, 0 otherwise.  Adobe's definition has to do with
   * whether the encoder performed a transformation, which is pretty useless.
   */
  
  emit__marker(cinfo, M__APP14);
  
  emit__2bytes(cinfo, 2 + 5 + 2 + 2 + 2 + 1); /* length */

  emit__byte(cinfo, 0x41);	/* Identifier: ASCII "Adobe" */
  emit__byte(cinfo, 0x64);
  emit__byte(cinfo, 0x6F);
  emit__byte(cinfo, 0x62);
  emit__byte(cinfo, 0x65);
  emit__2bytes(cinfo, 100);	/* Version */
  emit__2bytes(cinfo, 0);	/* Flags0 */
  emit__2bytes(cinfo, 0);	/* Flags1 */
  switch (cinfo->jpeg__color__space) {
  case JCS__YCbCr:
    emit__byte(cinfo, 1);	/* Color transform = 1 */
    break;
  case JCS__YCCK:
    emit__byte(cinfo, 2);	/* Color transform = 2 */
    break;
  default:
    emit__byte(cinfo, 0);	/* Color transform = 0 */
    break;
  }
}


/*
 * These routines allow writing an arbitrary marker with parameters.
 * The only intended use is to emit COM or APPn markers after calling
 * write__file__header and before calling write__frame__header.
 * Other uses are not guaranteed to produce desirable results.
 * Counting the parameter bytes properly is the caller's responsibility.
 */

METHODDEF(void)
write__marker__header (j__compress__ptr cinfo, int marker, unsigned int datalen)
/* Emit an arbitrary marker header */
{
  if (datalen > (unsigned int) 65533)		/* safety check */
    ERREXIT(cinfo, JERR__BAD__LENGTH);

  emit__marker(cinfo, (JPEG__MARKER) marker);

  emit__2bytes(cinfo, (int) (datalen + 2));	/* total length */
}

METHODDEF(void)
write__marker__byte (j__compress__ptr cinfo, int val)
/* Emit one byte of marker parameters following write__marker__header */
{
  emit__byte(cinfo, val);
}


/*
 * Write datastream header.
 * This consists of an SOI and optional APPn markers.
 * We recommend use of the JFIF marker, but not the Adobe marker,
 * when using YCbCr or grayscale data.  The JFIF marker should NOT
 * be used for any other JPEG colorspace.  The Adobe marker is helpful
 * to distinguish RGB, CMYK, and YCCK colorspaces.
 * Note that an application can write additional header markers after
 * jpeg__start__compress returns.
 */

METHODDEF(void)
write__file__header (j__compress__ptr cinfo)
{
  my__marker__ptr marker = (my__marker__ptr) cinfo->marker;

  emit__marker(cinfo, M__SOI);	/* first the SOI */

  /* SOI is defined to reset restart interval to 0 */
  marker->last__restart__interval = 0;

  if (cinfo->write__JFIF__header)	/* next an optional JFIF APP0 */
    emit__jfif__app0(cinfo);
  if (cinfo->write__Adobe__marker) /* next an optional Adobe APP14 */
    emit__adobe__app14(cinfo);
}


/*
 * Write frame header.
 * This consists of DQT and SOFn markers.
 * Note that we do not emit the SOF until we have emitted the DQT(s).
 * This avoids compatibility problems with incorrect implementations that
 * try to error-check the quant table numbers as soon as they see the SOF.
 */

METHODDEF(void)
write__frame__header (j__compress__ptr cinfo)
{
  int ci, prec;
  CH_BOOL is__baseline;
  jpeg__component__info *compptr;
  
  /* Emit DQT for each quantization table.
   * Note that emit__dqt() suppresses any duplicate tables.
   */
  prec = 0;
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    prec += emit__dqt(cinfo, compptr->quant__tbl__no);
  }
  /* now prec is nonzero iff there are any 16-bit quant tables. */

  /* Check for a non-baseline specification.
   * Note we assume that Huffman table numbers won't be changed later.
   */
  if (cinfo->arith__code || cinfo->progressive__mode ||
      cinfo->data__precision != 8) {
    is__baseline = FALSE;
  } else {
    is__baseline = TRUE;
    for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	 ci++, compptr++) {
      if (compptr->dc__tbl__no > 1 || compptr->ac__tbl__no > 1)
	is__baseline = FALSE;
    }
    if (prec && is__baseline) {
      is__baseline = FALSE;
      /* If it's baseline except for quantizer size, warn the user */
      TRACEMS(cinfo, 0, JTRC__16BIT__TABLES);
    }
  }

  /* Emit the proper SOF marker */
  if (cinfo->arith__code) {
    emit__sof(cinfo, M__SOF9);	/* SOF code for arithmetic coding */
  } else {
    if (cinfo->progressive__mode)
      emit__sof(cinfo, M__SOF2);	/* SOF code for progressive Huffman */
    else if (is__baseline)
      emit__sof(cinfo, M__SOF0);	/* SOF code for baseline implementation */
    else
      emit__sof(cinfo, M__SOF1);	/* SOF code for non-baseline Huffman file */
  }
}


/*
 * Write scan header.
 * This consists of DHT or DAC markers, optional DRI, and SOS.
 * Compressed data will be written following the SOS.
 */

METHODDEF(void)
write__scan__header (j__compress__ptr cinfo)
{
  my__marker__ptr marker = (my__marker__ptr) cinfo->marker;
  int i;
  jpeg__component__info *compptr;

  if (cinfo->arith__code) {
    /* Emit arith conditioning info.  We may have some duplication
     * if the file has multiple scans, but it's so small it's hardly
     * worth worrying about.
     */
    emit__dac(cinfo);
  } else {
    /* Emit Huffman tables.
     * Note that emit__dht() suppresses any duplicate tables.
     */
    for (i = 0; i < cinfo->comps__in__scan; i++) {
      compptr = cinfo->cur__comp__info[i];
      if (cinfo->progressive__mode) {
	/* Progressive mode: only DC or only AC tables are used in one scan */
	if (cinfo->Ss == 0) {
	  if (cinfo->Ah == 0)	/* DC needs no table for refinement scan */
	    emit__dht(cinfo, compptr->dc__tbl__no, FALSE);
	} else {
	  emit__dht(cinfo, compptr->ac__tbl__no, TRUE);
	}
      } else {
	/* Sequential mode: need both DC and AC tables */
	emit__dht(cinfo, compptr->dc__tbl__no, FALSE);
	emit__dht(cinfo, compptr->ac__tbl__no, TRUE);
      }
    }
  }

  /* Emit DRI if required --- note that DRI value could change for each scan.
   * We avoid wasting space with unnecessary DRIs, however.
   */
  if (cinfo->restart__interval != marker->last__restart__interval) {
    emit__dri(cinfo);
    marker->last__restart__interval = cinfo->restart__interval;
  }

  emit__sos(cinfo);
}


/*
 * Write datastream trailer.
 */

METHODDEF(void)
write__file__trailer (j__compress__ptr cinfo)
{
  emit__marker(cinfo, M__EOI);
}


/*
 * Write an abbreviated table-specification datastream.
 * This consists of SOI, DQT and DHT tables, and EOI.
 * Any table that is defined and not marked sent__table = TRUE will be
 * emitted.  Note that all tables will be marked sent__table = TRUE at exit.
 */

METHODDEF(void)
write__tables__only (j__compress__ptr cinfo)
{
  int i;

  emit__marker(cinfo, M__SOI);

  for (i = 0; i < NUM__QUANT__TBLS; i++) {
    if (cinfo->quant__tbl__ptrs[i] != NULL)
      (void) emit__dqt(cinfo, i);
  }

  if (! cinfo->arith__code) {
    for (i = 0; i < NUM__HUFF__TBLS; i++) {
      if (cinfo->dc__huff__tbl__ptrs[i] != NULL)
	emit__dht(cinfo, i, FALSE);
      if (cinfo->ac__huff__tbl__ptrs[i] != NULL)
	emit__dht(cinfo, i, TRUE);
    }
  }

  emit__marker(cinfo, M__EOI);
}


/*
 * Initialize the marker writer module.
 */

GLOBAL(void)
jinit__marker__writer (j__compress__ptr cinfo)
{
  my__marker__ptr marker;

  /* Create the subobject */
  marker = (my__marker__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__marker__writer));
  cinfo->marker = (struct jpeg__marker__writer *) marker;
  /* Initialize method pointers */
  marker->pub.write__file__header = write__file__header;
  marker->pub.write__frame__header = write__frame__header;
  marker->pub.write__scan__header = write__scan__header;
  marker->pub.write__file__trailer = write__file__trailer;
  marker->pub.write__tables__only = write__tables__only;
  marker->pub.write__marker__header = write__marker__header;
  marker->pub.write__marker__byte = write__marker__byte;
  /* Initialize private state */
  marker->last__restart__interval = 0;
}
