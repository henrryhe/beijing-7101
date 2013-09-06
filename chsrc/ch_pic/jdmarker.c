/*
 * jdmarker.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to decode JPEG datastream markers.
 * Most of the complexity arises from our desire to support input
 * suspension: if not all of the data for a marker is available,
 * we must exit back to the application.  On resumption, we reprocess
 * the marker.
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
  struct jpeg__marker__reader pub; /* public fields */

  /* Application-overridable marker processing methods */
  jpeg__marker__parser__method process__COM;
  jpeg__marker__parser__method process__APPn[16];

  /* Limit on marker data length to save for each marker type */
  unsigned int length__limit__COM;
  unsigned int length__limit__APPn[16];

  /* Status of COM/APPn marker saving */
  jpeg__saved__marker__ptr cur__marker;	/* NULL if not processing a marker */
  unsigned int bytes__read;		/* data bytes read so far in marker */
  /* Note: cur__marker is not linked into marker__list until it's all read. */
} my__marker__reader;

typedef my__marker__reader * my__marker__ptr;


/*
 * Macros for fetching data from the data source module.
 *
 * At all times, cinfo->src->next__input__byte and ->bytes__in__buffer reflect
 * the current restart point; we update them only when we have reached a
 * suitable place to restart if a suspension occurs.
 */

/* Declare and initialize local copies of input pointer/count */
#define INPUT__VARS(cinfo)  \
	struct jpeg__source__mgr * datasrc = (cinfo)->src;  \
	const JOCTET * next__input__byte = datasrc->next__input__byte;  \
	size_t bytes__in__buffer = datasrc->bytes__in__buffer

/* Unload the local copies --- do this only at a restart boundary */
#define INPUT__SYNC(cinfo)  \
	( datasrc->next__input__byte = next__input__byte,  \
	  datasrc->bytes__in__buffer = bytes__in__buffer )

/* Reload the local copies --- used only in MAKE__BYTE__AVAIL */
#define INPUT__RELOAD(cinfo)  \
	( next__input__byte = datasrc->next__input__byte,  \
	  bytes__in__buffer = datasrc->bytes__in__buffer )

/* Internal macro for INPUT__BYTE and INPUT__2BYTES: make a byte available.
 * Note we do *not* do INPUT__SYNC before calling fill__input__buffer,
 * but we must reload the local copies after a successful fill.
 */
#define MAKE__BYTE__AVAIL(cinfo,action)  \
	if (bytes__in__buffer == 0) {  \
	  if (! (*datasrc->fill__input__buffer) (cinfo))  \
	    { action; }  \
	  INPUT__RELOAD(cinfo);  \
	}

/* Read a byte into variable V.
 * If must suspend, take the specified action (typically "return FALSE").
 */
#define INPUT__BYTE(cinfo,V,action)  \
	MAKESTMT( MAKE__BYTE__AVAIL(cinfo,action); \
		  bytes__in__buffer--; \
		  V = GETJOCTET(*next__input__byte++); )

/* As above, but read two bytes interpreted as an unsigned 16-bit integer.
 * V should be declared unsigned int or perhaps INT32.
 */
#define INPUT__2BYTES(cinfo,V,action)  \
	MAKESTMT( MAKE__BYTE__AVAIL(cinfo,action); \
		  bytes__in__buffer--; \
		  V = ((unsigned int) GETJOCTET(*next__input__byte++)) << 8; \
		  MAKE__BYTE__AVAIL(cinfo,action); \
		  bytes__in__buffer--; \
		  V += GETJOCTET(*next__input__byte++); )


/*
 * Routines to process JPEG markers.
 *
 * Entry condition: JPEG marker itself has been read and its code saved
 *   in cinfo->unread__marker; input restart point is just after the marker.
 *
 * Exit: if return TRUE, have read and processed any parameters, and have
 *   updated the restart point to point after the parameters.
 *   If return FALSE, was forced to suspend before reaching end of
 *   marker parameters; restart point has not been moved.  Same routine
 *   will be called again after application supplies more input data.
 *
 * This approach to suspension assumes that all of a marker's parameters
 * can fit into a single input bufferload.  This should hold for "normal"
 * markers.  Some COM/APPn markers might have large parameter segments
 * that might not fit.  If we are simply dropping such a marker, we use
 * skip__input__data to get past it, and thereby put the problem on the
 * source manager's shoulders.  If we are saving the marker's contents
 * into memory, we use a slightly different convention: when forced to
 * suspend, the marker processor updates the restart point to the end of
 * what it's consumed (ie, the end of the buffer) before returning FALSE.
 * On resumption, cinfo->unread__marker still contains the marker code,
 * but the data source will point to the next chunk of marker data.
 * The marker processor must retain internal state to deal with this.
 *
 * Note that we don't bother to avoid duplicate trace messages if a
 * suspension occurs within marker parameters.  Other side effects
 * require more care.
 */


LOCAL(CH_BOOL)
get__soi (j__decompress__ptr cinfo)
/* Process an SOI marker */
{
  int i;
  
  TRACEMS(cinfo, 1, JTRC__SOI);

  if (cinfo->marker->saw__SOI)
    ERREXIT(cinfo, JERR__SOI__DUPLICATE);

  /* Reset all parameters that are defined to be reset by SOI */

  for (i = 0; i < NUM__ARITH__TBLS; i++) {
    cinfo->arith__dc__L[i] = 0;
    cinfo->arith__dc__U[i] = 1;
    cinfo->arith__ac__K[i] = 5;
  }
  cinfo->restart__interval = 0;

  /* Set initial assumptions for colorspace etc */

  cinfo->jpeg__color__space = JCS__UNKNOWN;
  cinfo->CCIR601__sampling = FALSE; /* Assume non-CCIR sampling??? */

  cinfo->saw__JFIF__marker = FALSE;
  cinfo->JFIF__major__version = 1; /* set default JFIF APP0 values */
  cinfo->JFIF__minor__version = 1;
  cinfo->density__unit = 0;
  cinfo->X__density = 1;
  cinfo->Y__density = 1;
  cinfo->saw__Adobe__marker = FALSE;
  cinfo->Adobe__transform = 0;

  cinfo->marker->saw__SOI = TRUE;

  return TRUE;
}


LOCAL(CH_BOOL)
get__sof (j__decompress__ptr cinfo, CH_BOOL is__prog, CH_BOOL is__arith)
/* Process a SOFn marker */
{
  INT32 length;
  int c, ci;
  jpeg__component__info * compptr;
  INPUT__VARS(cinfo);

  cinfo->progressive__mode = is__prog;
  cinfo->arith__code = is__arith;

  INPUT__2BYTES(cinfo, length, return FALSE);

  INPUT__BYTE(cinfo, cinfo->data__precision, return FALSE);
  INPUT__2BYTES(cinfo, cinfo->image__height, return FALSE);
  INPUT__2BYTES(cinfo, cinfo->image__width, return FALSE);
  INPUT__BYTE(cinfo, cinfo->num__components, return FALSE);

  length -= 8;

  TRACEMS4(cinfo, 1, JTRC__SOF, cinfo->unread__marker,
	   (int) cinfo->image__width, (int) cinfo->image__height,
	   cinfo->num__components);

  if (cinfo->marker->saw__SOF)
    ERREXIT(cinfo, JERR__SOF__DUPLICATE);

  /* We don't support files in which the image height is initially specified */
  /* as 0 and is later redefined by DNL.  As long as we have to check that,  */
  /* might as well have a general sanity check. */
  if (cinfo->image__height <= 0 || cinfo->image__width <= 0
      || cinfo->num__components <= 0)
    ERREXIT(cinfo, JERR__EMPTY__IMAGE);

  if (length != (cinfo->num__components * 3))
    ERREXIT(cinfo, JERR__BAD__LENGTH);

  if (cinfo->comp__info == NULL)	/* do only once, even if suspend */
    cinfo->comp__info = (jpeg__component__info *) (*cinfo->mem->alloc__small)
			((j__common__ptr) cinfo, JPOOL__IMAGE,
			 cinfo->num__components * SIZEOF(jpeg__component__info));
  
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    compptr->component__index = ci;
    INPUT__BYTE(cinfo, compptr->component__id, return FALSE);
    INPUT__BYTE(cinfo, c, return FALSE);
    compptr->h__samp__factor = (c >> 4) & 15;
    compptr->v__samp__factor = (c     ) & 15;
    INPUT__BYTE(cinfo, compptr->quant__tbl__no, return FALSE);

    TRACEMS4(cinfo, 1, JTRC__SOF__COMPONENT,
	     compptr->component__id, compptr->h__samp__factor,
	     compptr->v__samp__factor, compptr->quant__tbl__no);
  }

  cinfo->marker->saw__SOF = TRUE;

  INPUT__SYNC(cinfo);
  return TRUE;
}


LOCAL(CH_BOOL)
get__sos (j__decompress__ptr cinfo)
/* Process a SOS marker */
{
  INT32 length;
  int i, ci, n, c, cc;
  jpeg__component__info * compptr;
  INPUT__VARS(cinfo);

  if (! cinfo->marker->saw__SOF)
    ERREXIT(cinfo, JERR__SOS__NO__SOF);

  INPUT__2BYTES(cinfo, length, return FALSE);

  INPUT__BYTE(cinfo, n, return FALSE); /* Number of components */

  TRACEMS1(cinfo, 1, JTRC__SOS, n);

  if (length != (n * 2 + 6) || n < 1 || n > MAX__COMPS__IN__SCAN)
    ERREXIT(cinfo, JERR__BAD__LENGTH);

  cinfo->comps__in__scan = n;

  /* Collect the component-spec parameters */

  for (i = 0; i < n; i++) {
    INPUT__BYTE(cinfo, cc, return FALSE);
    INPUT__BYTE(cinfo, c, return FALSE);
    
    for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
	 ci++, compptr++) {
      if (cc == compptr->component__id)
	goto id__found;
    }

    ERREXIT1(cinfo, JERR__BAD__COMPONENT__ID, cc);

  id__found:

    cinfo->cur__comp__info[i] = compptr;
    compptr->dc__tbl__no = (c >> 4) & 15;
    compptr->ac__tbl__no = (c     ) & 15;
    
    TRACEMS3(cinfo, 1, JTRC__SOS__COMPONENT, cc,
	     compptr->dc__tbl__no, compptr->ac__tbl__no);
  }

  /* Collect the additional scan parameters Ss, Se, Ah/Al. */
  INPUT__BYTE(cinfo, c, return FALSE);
  cinfo->Ss = c;
  INPUT__BYTE(cinfo, c, return FALSE);
  cinfo->Se = c;
  INPUT__BYTE(cinfo, c, return FALSE);
  cinfo->Ah = (c >> 4) & 15;
  cinfo->Al = (c     ) & 15;

  TRACEMS4(cinfo, 1, JTRC__SOS__PARAMS, cinfo->Ss, cinfo->Se,
	   cinfo->Ah, cinfo->Al);

  /* Prepare to scan data & restart markers */
  cinfo->marker->next__restart__num = 0;

  /* Count another SOS marker */
  cinfo->input__scan__number++;

  INPUT__SYNC(cinfo);
  return TRUE;
}


#ifdef D__ARITH__CODING__SUPPORTED

LOCAL(CH_BOOL)
get__dac (j__decompress__ptr cinfo)
/* Process a DAC marker */
{
  INT32 length;
  int index, val;
  INPUT__VARS(cinfo);

  INPUT__2BYTES(cinfo, length, return FALSE);
  length -= 2;
  
  while (length > 0) {
    INPUT__BYTE(cinfo, index, return FALSE);
    INPUT__BYTE(cinfo, val, return FALSE);

    length -= 2;

    TRACEMS2(cinfo, 1, JTRC__DAC, index, val);

    if (index < 0 || index >= (2*NUM__ARITH__TBLS))
      ERREXIT1(cinfo, JERR__DAC__INDEX, index);

    if (index >= NUM__ARITH__TBLS) { /* define AC table */
      cinfo->arith__ac__K[index-NUM__ARITH__TBLS] = (UINT8) val;
    } else {			/* define DC table */
      cinfo->arith__dc__L[index] = (UINT8) (val & 0x0F);
      cinfo->arith__dc__U[index] = (UINT8) (val >> 4);
      if (cinfo->arith__dc__L[index] > cinfo->arith__dc__U[index])
	ERREXIT1(cinfo, JERR__DAC__VALUE, val);
    }
  }

  if (length != 0)
    ERREXIT(cinfo, JERR__BAD__LENGTH);

  INPUT__SYNC(cinfo);
  return TRUE;
}

#else /* ! D__ARITH__CODING__SUPPORTED */

#define get__dac(cinfo)  skip__variable(cinfo)

#endif /* D__ARITH__CODING__SUPPORTED */


LOCAL(CH_BOOL)
get__dht (j__decompress__ptr cinfo)
/* Process a DHT marker */
{
  INT32 length;
  UINT8 bits[17];
  UINT8 huffval[256];
  int i, index, count;
  JHUFF__TBL **htblptr;
  INPUT__VARS(cinfo);

  INPUT__2BYTES(cinfo, length, return FALSE);
  length -= 2;
  
  while (length > 16) {
    INPUT__BYTE(cinfo, index, return FALSE);

    TRACEMS1(cinfo, 1, JTRC__DHT, index);
      
    bits[0] = 0;
    count = 0;
    for (i = 1; i <= 16; i++) {
      INPUT__BYTE(cinfo, bits[i], return FALSE);
      count += bits[i];
    }

    length -= 1 + 16;

    TRACEMS8(cinfo, 2, JTRC__HUFFBITS,
	     bits[1], bits[2], bits[3], bits[4],
	     bits[5], bits[6], bits[7], bits[8]);
    TRACEMS8(cinfo, 2, JTRC__HUFFBITS,
	     bits[9], bits[10], bits[11], bits[12],
	     bits[13], bits[14], bits[15], bits[16]);

    /* Here we just do minimal validation of the counts to avoid walking
     * off the end of our table space.  jdhuff.c will check more carefully.
     */
    if (count > 256 || ((INT32) count) > length)
      ERREXIT(cinfo, JERR__BAD__HUFF__TABLE);

    for (i = 0; i < count; i++)
      INPUT__BYTE(cinfo, huffval[i], return FALSE);

    length -= count;

    if (index & 0x10) {		/* AC table definition */
      index -= 0x10;
      htblptr = &cinfo->ac__huff__tbl__ptrs[index];
    } else {			/* DC table definition */
      htblptr = &cinfo->dc__huff__tbl__ptrs[index];
    }

    if (index < 0 || index >= NUM__HUFF__TBLS)
      ERREXIT1(cinfo, JERR__DHT__INDEX, index);

    if (*htblptr == NULL)
      *htblptr = jpeg__alloc__huff__table((j__common__ptr) cinfo);
  
    MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
    MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));
  }

  if (length != 0)
    ERREXIT(cinfo, JERR__BAD__LENGTH);

  INPUT__SYNC(cinfo);
  return TRUE;
}


LOCAL(CH_BOOL)
get__dqt (j__decompress__ptr cinfo)
/* Process a DQT marker */
{
  INT32 length;
  int n, i, prec;
  unsigned int tmp;
  JQUANT__TBL *quant__ptr;
  INPUT__VARS(cinfo);

  INPUT__2BYTES(cinfo, length, return FALSE);
  length -= 2;

  while (length > 0) {
    INPUT__BYTE(cinfo, n, return FALSE);
    prec = n >> 4;
    n &= 0x0F;

    TRACEMS2(cinfo, 1, JTRC__DQT, n, prec);

    if (n >= NUM__QUANT__TBLS)
      ERREXIT1(cinfo, JERR__DQT__INDEX, n);
      
    if (cinfo->quant__tbl__ptrs[n] == NULL)
      cinfo->quant__tbl__ptrs[n] = jpeg__alloc__quant__table((j__common__ptr) cinfo);
    quant__ptr = cinfo->quant__tbl__ptrs[n];

    for (i = 0; i < DCTSIZE2; i++) {
      if (prec)
	INPUT__2BYTES(cinfo, tmp, return FALSE);
      else
	INPUT__BYTE(cinfo, tmp, return FALSE);
      /* We convert the zigzag-order table to natural array order. */
      quant__ptr->quantval[jpeg__natural__order[i]] = (UINT16) tmp;
    }

    if (cinfo->err->trace__level >= 2) {
      for (i = 0; i < DCTSIZE2; i += 8) {
	TRACEMS8(cinfo, 2, JTRC__QUANTVALS,
		 quant__ptr->quantval[i],   quant__ptr->quantval[i+1],
		 quant__ptr->quantval[i+2], quant__ptr->quantval[i+3],
		 quant__ptr->quantval[i+4], quant__ptr->quantval[i+5],
		 quant__ptr->quantval[i+6], quant__ptr->quantval[i+7]);
      }
    }

    length -= DCTSIZE2+1;
    if (prec) length -= DCTSIZE2;
  }

  if (length != 0)
    ERREXIT(cinfo, JERR__BAD__LENGTH);

  INPUT__SYNC(cinfo);
  return TRUE;
}


LOCAL(CH_BOOL)
get__dri (j__decompress__ptr cinfo)
/* Process a DRI marker */
{
  INT32 length;
  unsigned int tmp;
  INPUT__VARS(cinfo);

  INPUT__2BYTES(cinfo, length, return FALSE);
  
  if (length != 4)
    ERREXIT(cinfo, JERR__BAD__LENGTH);

  INPUT__2BYTES(cinfo, tmp, return FALSE);

  TRACEMS1(cinfo, 1, JTRC__DRI, tmp);

  cinfo->restart__interval = tmp;

  INPUT__SYNC(cinfo);
  return TRUE;
}


/*
 * Routines for processing APPn and COM markers.
 * These are either saved in memory or discarded, per application request.
 * APP0 and APP14 are specially checked to see if they are
 * JFIF and Adobe markers, respectively.
 */

#define APP0__DATA__LEN	14	/* Length of interesting data in APP0 */
#define APP14__DATA__LEN	12	/* Length of interesting data in APP14 */
#define APPN__DATA__LEN	14	/* Must be the largest of the above!! */


LOCAL(void)
examine__app0 (j__decompress__ptr cinfo, JOCTET FAR * data,
	      unsigned int datalen, INT32 remaining)
/* Examine first few bytes from an APP0.
 * Take appropriate action if it is a JFIF marker.
 * datalen is # of bytes at data[], remaining is length of rest of marker data.
 */
{
  INT32 totallen = (INT32) datalen + remaining;

  if (datalen >= APP0__DATA__LEN &&
      GETJOCTET(data[0]) == 0x4A &&
      GETJOCTET(data[1]) == 0x46 &&
      GETJOCTET(data[2]) == 0x49 &&
      GETJOCTET(data[3]) == 0x46 &&
      GETJOCTET(data[4]) == 0) {
    /* Found JFIF APP0 marker: save info */
    cinfo->saw__JFIF__marker = TRUE;
    cinfo->JFIF__major__version = GETJOCTET(data[5]);
    cinfo->JFIF__minor__version = GETJOCTET(data[6]);
    cinfo->density__unit = GETJOCTET(data[7]);
    cinfo->X__density = (GETJOCTET(data[8]) << 8) + GETJOCTET(data[9]);
    cinfo->Y__density = (GETJOCTET(data[10]) << 8) + GETJOCTET(data[11]);
    /* Check version.
     * Major version must be 1, anything else signals an incompatible change.
     * (We used to treat this as an error, but now it's a nonfatal warning,
     * because some bozo at Hijaak couldn't read the spec.)
     * Minor version should be 0..2, but process anyway if newer.
     */
    if (cinfo->JFIF__major__version != 1)
      WARNMS2(cinfo, JWRN__JFIF__MAJOR,
	      cinfo->JFIF__major__version, cinfo->JFIF__minor__version);
    /* Generate trace messages */
    TRACEMS5(cinfo, 1, JTRC__JFIF,
	     cinfo->JFIF__major__version, cinfo->JFIF__minor__version,
	     cinfo->X__density, cinfo->Y__density, cinfo->density__unit);
    /* Validate thumbnail dimensions and issue appropriate messages */
    if (GETJOCTET(data[12]) | GETJOCTET(data[13]))
      TRACEMS2(cinfo, 1, JTRC__JFIF__THUMBNAIL,
	       GETJOCTET(data[12]), GETJOCTET(data[13]));
    totallen -= APP0__DATA__LEN;
    if (totallen !=
	((INT32)GETJOCTET(data[12]) * (INT32)GETJOCTET(data[13]) * (INT32) 3))
      TRACEMS1(cinfo, 1, JTRC__JFIF__BADTHUMBNAILSIZE, (int) totallen);
  } else if (datalen >= 6 &&
      GETJOCTET(data[0]) == 0x4A &&
      GETJOCTET(data[1]) == 0x46 &&
      GETJOCTET(data[2]) == 0x58 &&
      GETJOCTET(data[3]) == 0x58 &&
      GETJOCTET(data[4]) == 0) {
    /* Found JFIF "JFXX" extension APP0 marker */
    /* The library doesn't actually do anything with these,
     * but we try to produce a helpful trace message.
     */
    switch (GETJOCTET(data[5])) {
    case 0x10:
      TRACEMS1(cinfo, 1, JTRC__THUMB__JPEG, (int) totallen);
      break;
    case 0x11:
      TRACEMS1(cinfo, 1, JTRC__THUMB__PALETTE, (int) totallen);
      break;
    case 0x13:
      TRACEMS1(cinfo, 1, JTRC__THUMB__RGB, (int) totallen);
      break;
    default:
      TRACEMS2(cinfo, 1, JTRC__JFIF__EXTENSION,
	       GETJOCTET(data[5]), (int) totallen);
      break;
    }
  } else {
    /* Start of APP0 does not match "JFIF" or "JFXX", or too short */
    TRACEMS1(cinfo, 1, JTRC__APP0, (int) totallen);
  }
}


LOCAL(void)
examine__app14 (j__decompress__ptr cinfo, JOCTET FAR * data,
	       unsigned int datalen, INT32 remaining)
/* Examine first few bytes from an APP14.
 * Take appropriate action if it is an Adobe marker.
 * datalen is # of bytes at data[], remaining is length of rest of marker data.
 */
{
  unsigned int version, flags0, flags1, transform;

  if (datalen >= APP14__DATA__LEN &&
      GETJOCTET(data[0]) == 0x41 &&
      GETJOCTET(data[1]) == 0x64 &&
      GETJOCTET(data[2]) == 0x6F &&
      GETJOCTET(data[3]) == 0x62 &&
      GETJOCTET(data[4]) == 0x65) {
    /* Found Adobe APP14 marker */
    version = (GETJOCTET(data[5]) << 8) + GETJOCTET(data[6]);
    flags0 = (GETJOCTET(data[7]) << 8) + GETJOCTET(data[8]);
    flags1 = (GETJOCTET(data[9]) << 8) + GETJOCTET(data[10]);
    transform = GETJOCTET(data[11]);
    TRACEMS4(cinfo, 1, JTRC__ADOBE, version, flags0, flags1, transform);
    cinfo->saw__Adobe__marker = TRUE;
    cinfo->Adobe__transform = (UINT8) transform;
  } else {
    /* Start of APP14 does not match "Adobe", or too short */
    TRACEMS1(cinfo, 1, JTRC__APP14, (int) (datalen + remaining));
  }
}


METHODDEF(CH_BOOL)
get__interesting__appn (j__decompress__ptr cinfo)
/* Process an APP0 or APP14 marker without saving it */
{
  INT32 length;
  JOCTET b[APPN__DATA__LEN];
  unsigned int i, numtoread;
  INPUT__VARS(cinfo);

  INPUT__2BYTES(cinfo, length, return FALSE);
  length -= 2;

  /* get the interesting part of the marker data */
  if (length >= APPN__DATA__LEN)
    numtoread = APPN__DATA__LEN;
  else if (length > 0)
    numtoread = (unsigned int) length;
  else
    numtoread = 0;
  for (i = 0; i < numtoread; i++)
    INPUT__BYTE(cinfo, b[i], return FALSE);
  length -= numtoread;

  /* process it */
  switch (cinfo->unread__marker) {
  case M__APP0:
    examine__app0(cinfo, (JOCTET FAR *) b, numtoread, length);
    break;
  case M__APP14:
    examine__app14(cinfo, (JOCTET FAR *) b, numtoread, length);
    break;
  default:
    /* can't get here unless jpeg__save__markers chooses wrong processor */
    ERREXIT1(cinfo, JERR__UNKNOWN__MARKER, cinfo->unread__marker);
    break;
  }

  /* skip any remaining data -- could be lots */
  INPUT__SYNC(cinfo);
  if (length > 0)
    (*cinfo->src->skip__input__data) (cinfo, (long) length);

  return TRUE;
}


#ifdef SAVE__MARKERS__SUPPORTED

METHODDEF(CH_BOOL)
save__marker (j__decompress__ptr cinfo)
/* Save an APPn or COM marker into the marker list */
{
  my__marker__ptr marker = (my__marker__ptr) cinfo->marker;
  jpeg__saved__marker__ptr cur__marker = marker->cur__marker;
  unsigned int bytes__read, data__length;
  JOCTET FAR * data;
  INT32 length = 0;
  INPUT__VARS(cinfo);

  if (cur__marker == NULL) {
    /* begin reading a marker */
    INPUT__2BYTES(cinfo, length, return FALSE);
    length -= 2;
    if (length >= 0) {		/* watch out for bogus length word */
      /* figure out how much we want to save */
      unsigned int limit;
      if (cinfo->unread__marker == (int) M__COM)
	limit = marker->length__limit__COM;
      else
	limit = marker->length__limit__APPn[cinfo->unread__marker - (int) M__APP0];
      if ((unsigned int) length < limit)
	limit = (unsigned int) length;
      /* allocate and initialize the marker item */
      cur__marker = (jpeg__saved__marker__ptr)
	(*cinfo->mem->alloc__large) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				    SIZEOF(struct jpeg__marker__struct) + limit);
      cur__marker->next = NULL;
      cur__marker->marker = (UINT8) cinfo->unread__marker;
      cur__marker->original__length = (unsigned int) length;
      cur__marker->data__length = limit;
      /* data area is just beyond the jpeg__marker__struct */
      data = cur__marker->data = (JOCTET FAR *) (cur__marker + 1);
      marker->cur__marker = cur__marker;
      marker->bytes__read = 0;
      bytes__read = 0;
      data__length = limit;
    } else {
      /* deal with bogus length word */
      bytes__read = data__length = 0;
      data = NULL;
    }
  } else {
    /* resume reading a marker */
    bytes__read = marker->bytes__read;
    data__length = cur__marker->data__length;
    data = cur__marker->data + bytes__read;
  }

  while (bytes__read < data__length) {
    INPUT__SYNC(cinfo);		/* move the restart point to here */
    marker->bytes__read = bytes__read;
    /* If there's not at least one byte in buffer, suspend */
    MAKE__BYTE__AVAIL(cinfo, return FALSE);
    /* Copy bytes with reasonable rapidity */
    while (bytes__read < data__length && bytes__in__buffer > 0) {
      *data++ = *next__input__byte++;
      bytes__in__buffer--;
      bytes__read++;
    }
  }

  /* Done reading what we want to read */
  if (cur__marker != NULL) {	/* will be NULL if bogus length word */
    /* Add new marker to end of list */
    if (cinfo->marker__list == NULL) {
      cinfo->marker__list = cur__marker;
    } else {
      jpeg__saved__marker__ptr prev = cinfo->marker__list;
      while (prev->next != NULL)
	prev = prev->next;
      prev->next = cur__marker;
    }
    /* Reset pointer & calc remaining data length */
    data = cur__marker->data;
    length = cur__marker->original__length - data__length;
  }
  /* Reset to initial state for next marker */
  marker->cur__marker = NULL;

  /* Process the marker if interesting; else just make a generic trace msg */
  switch (cinfo->unread__marker) {
  case M__APP0:
    examine__app0(cinfo, data, data__length, length);
    break;
  case M__APP14:
    examine__app14(cinfo, data, data__length, length);
    break;
  default:
    TRACEMS2(cinfo, 1, JTRC__MISC__MARKER, cinfo->unread__marker,
	     (int) (data__length + length));
    break;
  }

  /* skip any remaining data -- could be lots */
  INPUT__SYNC(cinfo);		/* do before skip__input__data */
  if (length > 0)
    (*cinfo->src->skip__input__data) (cinfo, (long) length);

  return TRUE;
}

#endif /* SAVE__MARKERS__SUPPORTED */


METHODDEF(CH_BOOL)
skip__variable (j__decompress__ptr cinfo)
/* Skip over an unknown or uninteresting variable-length marker */
{
  INT32 length;
  INPUT__VARS(cinfo);

  INPUT__2BYTES(cinfo, length, return FALSE);
  length -= 2;
  
  TRACEMS2(cinfo, 1, JTRC__MISC__MARKER, cinfo->unread__marker, (int) length);

  INPUT__SYNC(cinfo);		/* do before skip__input__data */
  if (length > 0)
    (*cinfo->src->skip__input__data) (cinfo, (long) length);

  return TRUE;
}


/*
 * Find the next JPEG marker, save it in cinfo->unread__marker.
 * Returns FALSE if had to suspend before reaching a marker;
 * in that case cinfo->unread__marker is unchanged.
 *
 * Note that the result might not be a valid marker code,
 * but it will never be 0 or FF.
 */

LOCAL(CH_BOOL)
next__marker (j__decompress__ptr cinfo)
{
  int c;
  INPUT__VARS(cinfo);

  for (;;) {
    INPUT__BYTE(cinfo, c, return FALSE);
    /* Skip any non-FF bytes.
     * This may look a bit inefficient, but it will not occur in a valid file.
     * We sync after each discarded byte so that a suspending data source
     * can discard the byte from its buffer.
     */
    while (c != 0xFF) {
      cinfo->marker->discarded__bytes++;
      INPUT__SYNC(cinfo);
      INPUT__BYTE(cinfo, c, return FALSE);
    }
    /* This loop swallows any duplicate FF bytes.  Extra FFs are legal as
     * pad bytes, so don't count them in discarded__bytes.  We assume there
     * will not be so many consecutive FF bytes as to overflow a suspending
     * data source's input buffer.
     */
    do {
      INPUT__BYTE(cinfo, c, return FALSE);
    } while (c == 0xFF);
    if (c != 0)
      break;			/* found a valid marker, exit loop */
    /* Reach here if we found a stuffed-zero data sequence (FF/00).
     * Discard it and loop back to try again.
     */
    cinfo->marker->discarded__bytes += 2;
    INPUT__SYNC(cinfo);
  }

  if (cinfo->marker->discarded__bytes != 0) {
    WARNMS2(cinfo, JWRN__EXTRANEOUS__DATA, cinfo->marker->discarded__bytes, c);
    cinfo->marker->discarded__bytes = 0;
  }

  cinfo->unread__marker = c;

  INPUT__SYNC(cinfo);
  return TRUE;
}


LOCAL(CH_BOOL)
first__marker (j__decompress__ptr cinfo)
/* Like next__marker, but used to obtain the initial SOI marker. */
/* For this marker, we do not allow preceding garbage or fill; otherwise,
 * we might well scan an entire input file before realizing it ain't JPEG.
 * If an application wants to process non-JFIF files, it must seek to the
 * SOI before calling the JPEG library.
 */
{
  int c, c2;
  INPUT__VARS(cinfo);

  INPUT__BYTE(cinfo, c, return FALSE);
  INPUT__BYTE(cinfo, c2, return FALSE);
  if (c != 0xFF || c2 != (int) M__SOI)
    ERREXIT2(cinfo, JERR__NO__SOI, c, c2);

  cinfo->unread__marker = c2;

  INPUT__SYNC(cinfo);
  return TRUE;
}


/*
 * Read markers until SOS or EOI.
 *
 * Returns same codes as are defined for jpeg__consume__input:
 * JPEG__SUSPENDED, JPEG__REACHED__SOS, or JPEG__REACHED__EOI.
 */

METHODDEF(int)
read__markers (j__decompress__ptr cinfo)
{
  /* Outer loop repeats once for each marker. */
  for (;;) {
    /* Collect the marker proper, unless we already did. */
    /* NB: first__marker() enforces the requirement that SOI appear first. */
    if (cinfo->unread__marker == 0) {
      if (! cinfo->marker->saw__SOI) {
	if (! first__marker(cinfo))
	  return JPEG__SUSPENDED;
      } else {
	if (! next__marker(cinfo))
	  return JPEG__SUSPENDED;
      }
    }
    /* At this point cinfo->unread__marker contains the marker code and the
     * input point is just past the marker proper, but before any parameters.
     * A suspension will cause us to return with this state still true.
     */
    switch (cinfo->unread__marker) {
    case M__SOI:
      if (! get__soi(cinfo))
	return JPEG__SUSPENDED;
      break;

    case M__SOF0:		/* Baseline */
    case M__SOF1:		/* Extended sequential, Huffman */
      if (! get__sof(cinfo, FALSE, FALSE))
	return JPEG__SUSPENDED;
      break;

    case M__SOF2:		/* Progressive, Huffman */
      if (! get__sof(cinfo, TRUE, FALSE))
	return JPEG__SUSPENDED;
      break;

    case M__SOF9:		/* Extended sequential, arithmetic */
      if (! get__sof(cinfo, FALSE, TRUE))
	return JPEG__SUSPENDED;
      break;

    case M__SOF10:		/* Progressive, arithmetic */
      if (! get__sof(cinfo, TRUE, TRUE))
	return JPEG__SUSPENDED;
      break;

    /* Currently unsupported SOFn types */
    case M__SOF3:		/* Lossless, Huffman */
    case M__SOF5:		/* Differential sequential, Huffman */
    case M__SOF6:		/* Differential progressive, Huffman */
    case M__SOF7:		/* Differential lossless, Huffman */
    case M__JPG:			/* Reserved for JPEG extensions */
    case M__SOF11:		/* Lossless, arithmetic */
    case M__SOF13:		/* Differential sequential, arithmetic */
    case M__SOF14:		/* Differential progressive, arithmetic */
    case M__SOF15:		/* Differential lossless, arithmetic */
      ERREXIT1(cinfo, JERR__SOF__UNSUPPORTED, cinfo->unread__marker);
      break;

    case M__SOS:
      if (! get__sos(cinfo))
	return JPEG__SUSPENDED;
      cinfo->unread__marker = 0;	/* processed the marker */
      return JPEG__REACHED__SOS;
    
    case M__EOI:
      TRACEMS(cinfo, 1, JTRC__EOI);
      cinfo->unread__marker = 0;	/* processed the marker */
      return JPEG__REACHED__EOI;
      
    case M__DAC:
      if (! get__dac(cinfo))
	return JPEG__SUSPENDED;
      break;
      
    case M__DHT:
      if (! get__dht(cinfo))
	return JPEG__SUSPENDED;
      break;
      
    case M__DQT:
      if (! get__dqt(cinfo))
	return JPEG__SUSPENDED;
      break;
      
    case M__DRI:
      if (! get__dri(cinfo))
	return JPEG__SUSPENDED;
      break;
      
    case M__APP0:
    case M__APP1:
    case M__APP2:
    case M__APP3:
    case M__APP4:
    case M__APP5:
    case M__APP6:
    case M__APP7:
    case M__APP8:
    case M__APP9:
    case M__APP10:
    case M__APP11:
    case M__APP12:
    case M__APP13:
    case M__APP14:
    case M__APP15:
      if (! (*((my__marker__ptr) cinfo->marker)->process__APPn[
		cinfo->unread__marker - (int) M__APP0]) (cinfo))
	return JPEG__SUSPENDED;
      break;
      
    case M__COM:
      if (! (*((my__marker__ptr) cinfo->marker)->process__COM) (cinfo))
	return JPEG__SUSPENDED;
      break;

    case M__RST0:		/* these are all parameterless */
    case M__RST1:
    case M__RST2:
    case M__RST3:
    case M__RST4:
    case M__RST5:
    case M__RST6:
    case M__RST7:
    case M__TEM:
      TRACEMS1(cinfo, 1, JTRC__PARMLESS__MARKER, cinfo->unread__marker);
      break;

    case M__DNL:			/* Ignore DNL ... perhaps the wrong thing */
      if (! skip__variable(cinfo))
	return JPEG__SUSPENDED;
      break;

    default:			/* must be DHP, EXP, JPGn, or RESn */
      /* For now, we treat the reserved markers as fatal errors since they are
       * likely to be used to signal incompatible JPEG Part 3 extensions.
       * Once the JPEG 3 version-number marker is well defined, this code
       * ought to change!
       */
      ERREXIT1(cinfo, JERR__UNKNOWN__MARKER, cinfo->unread__marker);
      break;
    }
    /* Successfully processed marker, so reset state variable */
    cinfo->unread__marker = 0;
  } /* end loop */
}


/*
 * Read a restart marker, which is expected to appear next in the datastream;
 * if the marker is not there, take appropriate recovery action.
 * Returns FALSE if suspension is required.
 *
 * This is called by the entropy decoder after it has read an appropriate
 * number of MCUs.  cinfo->unread__marker may be nonzero if the entropy decoder
 * has already read a marker from the data source.  Under normal conditions
 * cinfo->unread__marker will be reset to 0 before returning; if not reset,
 * it holds a marker which the decoder will be unable to read past.
 */

METHODDEF(CH_BOOL)
read__restart__marker (j__decompress__ptr cinfo)
{
  /* Obtain a marker unless we already did. */
  /* Note that next__marker will complain if it skips any data. */
  if (cinfo->unread__marker == 0) {
    if (! next__marker(cinfo))
      return FALSE;
  }

  if (cinfo->unread__marker ==
      ((int) M__RST0 + cinfo->marker->next__restart__num)) {
    /* Normal case --- swallow the marker and let entropy decoder continue */
    TRACEMS1(cinfo, 3, JTRC__RST, cinfo->marker->next__restart__num);
    cinfo->unread__marker = 0;
  } else {
    /* Uh-oh, the restart markers have been messed up. */
    /* Let the data source manager determine how to resync. */
    if (! (*cinfo->src->resync__to__restart) (cinfo,
					    cinfo->marker->next__restart__num))
      return FALSE;
  }

  /* Update next-restart state */
  cinfo->marker->next__restart__num = (cinfo->marker->next__restart__num + 1) & 7;

  return TRUE;
}


/*
 * This is the default resync__to__restart method for data source managers
 * to use if they don't have any better approach.  Some data source managers
 * may be able to back up, or may have additional knowledge about the data
 * which permits a more intelligent recovery strategy; such managers would
 * presumably supply their own resync method.
 *
 * read__restart__marker calls resync__to__restart if it finds a marker other than
 * the restart marker it was expecting.  (This code is *not* used unless
 * a nonzero restart interval has been declared.)  cinfo->unread__marker is
 * the marker code actually found (might be anything, except 0 or FF).
 * The desired restart marker number (0..7) is passed as a parameter.
 * This routine is supposed to apply whatever error recovery strategy seems
 * appropriate in order to position the input stream to the next data segment.
 * Note that cinfo->unread__marker is treated as a marker appearing before
 * the current data-source input point; usually it should be reset to zero
 * before returning.
 * Returns FALSE if suspension is required.
 *
 * This implementation is substantially constrained by wanting to treat the
 * input as a data stream; this means we can't back up.  Therefore, we have
 * only the following actions to work with:
 *   1. Simply discard the marker and let the entropy decoder resume at next
 *      byte of file.
 *   2. Read forward until we find another marker, discarding intervening
 *      data.  (In theory we could look ahead within the current bufferload,
 *      without having to discard data if we don't find the desired marker.
 *      This idea is not implemented here, in part because it makes behavior
 *      dependent on buffer size and chance buffer-boundary positions.)
 *   3. Leave the marker unread (by failing to zero cinfo->unread__marker).
 *      This will cause the entropy decoder to process an empty data segment,
 *      inserting dummy zeroes, and then we will reprocess the marker.
 *
 * #2 is appropriate if we think the desired marker lies ahead, while #3 is
 * appropriate if the found marker is a future restart marker (indicating
 * that we have missed the desired restart marker, probably because it got
 * corrupted).
 * We apply #2 or #3 if the found marker is a restart marker no more than
 * two counts behind or ahead of the expected one.  We also apply #2 if the
 * found marker is not a legal JPEG marker code (it's certainly bogus data).
 * If the found marker is a restart marker more than 2 counts away, we do #1
 * (too much risk that the marker is erroneous; with luck we will be able to
 * resync at some future point).
 * For any valid non-restart JPEG marker, we apply #3.  This keeps us from
 * overrunning the end of a scan.  An implementation limited to single-scan
 * files might find it better to apply #2 for markers other than EOI, since
 * any other marker would have to be bogus data in that case.
 */

GLOBAL(CH_BOOL)
jpeg__resync__to__restart (j__decompress__ptr cinfo, int desired)
{
  int marker = cinfo->unread__marker;
  int action = 1;
  
  /* Always put up a warning. */
  WARNMS2(cinfo, JWRN__MUST__RESYNC, marker, desired);
  
  /* Outer loop handles repeated decision after scanning forward. */
  for (;;) {
    if (marker < (int) M__SOF0)
      action = 2;		/* invalid marker */
    else if (marker < (int) M__RST0 || marker > (int) M__RST7)
      action = 3;		/* valid non-restart marker */
    else {
      if (marker == ((int) M__RST0 + ((desired+1) & 7)) ||
	  marker == ((int) M__RST0 + ((desired+2) & 7)))
	action = 3;		/* one of the next two expected restarts */
      else if (marker == ((int) M__RST0 + ((desired-1) & 7)) ||
	       marker == ((int) M__RST0 + ((desired-2) & 7)))
	action = 2;		/* a prior restart, so advance */
      else
	action = 1;		/* desired restart or too far away */
    }
    TRACEMS2(cinfo, 4, JTRC__RECOVERY__ACTION, marker, action);
    switch (action) {
    case 1:
      /* Discard marker and let entropy decoder resume processing. */
      cinfo->unread__marker = 0;
      return TRUE;
    case 2:
      /* Scan to the next marker, and repeat the decision loop. */
      if (! next__marker(cinfo))
	return FALSE;
      marker = cinfo->unread__marker;
      break;
    case 3:
      /* Return without advancing past this marker. */
      /* Entropy decoder will be forced to process an empty segment. */
      return TRUE;
    }
  } /* end loop */
}


/*
 * Reset marker processing state to begin a fresh datastream.
 */

METHODDEF(void)
reset__marker__reader (j__decompress__ptr cinfo)
{
  my__marker__ptr marker = (my__marker__ptr) cinfo->marker;

  cinfo->comp__info = NULL;		/* until allocated by get__sof */
  cinfo->input__scan__number = 0;		/* no SOS seen yet */
  cinfo->unread__marker = 0;		/* no pending marker */
  marker->pub.saw__SOI = FALSE;		/* set internal state too */
  marker->pub.saw__SOF = FALSE;
  marker->pub.discarded__bytes = 0;
  marker->cur__marker = NULL;
}


/*
 * Initialize the marker reader module.
 * This is called only once, when the decompression object is created.
 */

GLOBAL(void)
jinit__marker__reader (j__decompress__ptr cinfo)
{
  my__marker__ptr marker;
  int i;

  /* Create subobject in permanent pool */
  marker = (my__marker__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__PERMANENT,
				SIZEOF(my__marker__reader));
  cinfo->marker = (struct jpeg__marker__reader *) marker;
  /* Initialize public method pointers */
  marker->pub.reset__marker__reader = reset__marker__reader;
  marker->pub.read__markers = read__markers;
  marker->pub.read__restart__marker = read__restart__marker;
  /* Initialize COM/APPn processing.
   * By default, we examine and then discard APP0 and APP14,
   * but simply discard COM and all other APPn.
   */
  marker->process__COM = skip__variable;
  marker->length__limit__COM = 0;
  for (i = 0; i < 16; i++) {
    marker->process__APPn[i] = skip__variable;
    marker->length__limit__APPn[i] = 0;
  }
  marker->process__APPn[0] = get__interesting__appn;
  marker->process__APPn[14] = get__interesting__appn;
  /* Reset marker processing state */
  reset__marker__reader(cinfo);
}


/*
 * Control saving of COM and APPn markers into marker__list.
 */

#ifdef SAVE__MARKERS__SUPPORTED

GLOBAL(void)
jpeg__save__markers (j__decompress__ptr cinfo, int marker__code,
		   unsigned int length__limit)
{
  my__marker__ptr marker = (my__marker__ptr) cinfo->marker;
  long maxlength;
  jpeg__marker__parser__method processor;

  /* Length limit mustn't be larger than what we can allocate
   * (should only be a concern in a 16-bit environment).
   */
  maxlength = cinfo->mem->max__alloc__chunk - SIZEOF(struct jpeg__marker__struct);
  if (((long) length__limit) > maxlength)
    length__limit = (unsigned int) maxlength;

  /* Choose processor routine to use.
   * APP0/APP14 have special requirements.
   */
  if (length__limit) {
    processor = save__marker;
    /* If saving APP0/APP14, save at least enough for our internal use. */
    if (marker__code == (int) M__APP0 && length__limit < APP0__DATA__LEN)
      length__limit = APP0__DATA__LEN;
    else if (marker__code == (int) M__APP14 && length__limit < APP14__DATA__LEN)
      length__limit = APP14__DATA__LEN;
  } else {
    processor = skip__variable;
    /* If discarding APP0/APP14, use our regular on-the-fly processor. */
    if (marker__code == (int) M__APP0 || marker__code == (int) M__APP14)
      processor = get__interesting__appn;
  }

  if (marker__code == (int) M__COM) {
    marker->process__COM = processor;
    marker->length__limit__COM = length__limit;
  } else if (marker__code >= (int) M__APP0 && marker__code <= (int) M__APP15) {
    marker->process__APPn[marker__code - (int) M__APP0] = processor;
    marker->length__limit__APPn[marker__code - (int) M__APP0] = length__limit;
  } else
    ERREXIT1(cinfo, JERR__UNKNOWN__MARKER, marker__code);
}

#endif /* SAVE__MARKERS__SUPPORTED */


/*
 * Install a special processing method for COM or APPn markers.
 */

GLOBAL(void)
jpeg__set__marker__processor (j__decompress__ptr cinfo, int marker__code,
			   jpeg__marker__parser__method routine)
{
  my__marker__ptr marker = (my__marker__ptr) cinfo->marker;

  if (marker__code == (int) M__COM)
    marker->process__COM = routine;
  else if (marker__code >= (int) M__APP0 && marker__code <= (int) M__APP15)
    marker->process__APPn[marker__code - (int) M__APP0] = routine;
  else
    ERREXIT1(cinfo, JERR__UNKNOWN__MARKER, marker__code);
}
