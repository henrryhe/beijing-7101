/*
 * jdapimin.c
 *
 * Copyright (C) 1994-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains application interface code for the decompression half
 * of the JPEG library.  These are the "minimum" API routines that may be
 * needed in either the normal full-decompression case or the
 * transcoding-only case.
 *
 * Most of the routines intended to be called directly by an application
 * are in this file or in jdapistd.c.  But also see jcomapi.c for routines
 * shared by compression and decompression, and jdtrans.c for the transcoding
 * case.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/*
 * Initialization of a JPEG decompression object.
 * The error manager must already be set up (in case memory manager fails).

 #define  dz__DEBUG__DECOMPRESS */

GLOBAL(void)
jpeg__CreateDecompress (j__decompress__ptr cinfo, int version, size_t structsize)
{
  int i;

  #ifdef dz__DEBUG__DECOMPRESS
  	STTBX_Print("decompress 1\n");
  #endif
  /* Guard against version mismatches between library and caller. */
  cinfo->mem = NULL;		/* so jpeg__destroy knows mem mgr not called */
  if (version != JPEG__LIB__VERSION)
    ERREXIT2(cinfo, JERR__BAD__LIB__VERSION, JPEG__LIB__VERSION, version);
   #ifdef dz__DEBUG__DECOMPRESS
  	STTBX_Print("decompress 2\n");
  #endif
  if (structsize != SIZEOF(struct jpeg__decompress__struct))
    ERREXIT2(cinfo, JERR__BAD__STRUCT__SIZE, 
	     (int) SIZEOF(struct jpeg__decompress__struct), (int) structsize);

  /* For debugging purposes, we zero the whole master structure.
   * But the application has already set the err pointer, and may have set
   * client__data, so we have to save and restore those fields.
   * Note: if application hasn't set client__data, tools like Purify may
   * complain here.
   */
  {
    struct jpeg__error__mgr * err = cinfo->err;
    void * client__data = cinfo->client__data; /* ignore Purify complaint here */
    MEMZERO(cinfo, SIZEOF(struct jpeg__decompress__struct));
    cinfo->err = err;
    cinfo->client__data = client__data;
  }
   #ifdef dz__DEBUG__DECOMPRESS
  	STTBX_Print("decompress 3\n");
  #endif
  cinfo->is__decompressor = TRUE;

  /* Initialize a memory manager instance for this object */
  jinit__memory__mgr((j__common__ptr) cinfo);

 #ifdef dz__DEBUG__DECOMPRESS
  	STTBX_Print("decompress 4\n");
  #endif
  /* Zero out pointers to permanent structures. */
  cinfo->progress = NULL;
  cinfo->src = NULL;

  for (i = 0; i < NUM__QUANT__TBLS; i++)
    cinfo->quant__tbl__ptrs[i] = NULL;

  for (i = 0; i < NUM__HUFF__TBLS; i++) {
    cinfo->dc__huff__tbl__ptrs[i] = NULL;
    cinfo->ac__huff__tbl__ptrs[i] = NULL;
  }

  /* Initialize marker processor so application can override methods
   * for COM, APPn markers before calling jpeg__read__header.
   */
    #ifdef dz__DEBUG__DECOMPRESS
  	STTBX_Print("decompress 5\n");
  #endif
  cinfo->marker__list = NULL;
  jinit__marker__reader(cinfo);

 #ifdef dz__DEBUG__DECOMPRESS
  	STTBX_Print("decompress 6\n");
  #endif
  /* And initialize the overall input controller. */
  jinit__input__controller(cinfo);

   #ifdef dz__DEBUG__DECOMPRESS
  	STTBX_Print("decompress 7\n");
  #endif
  /* OK, I'm ready */
  cinfo->global__state = DSTATE__START;
}


/*
 * Destruction of a JPEG decompression object
 */

GLOBAL(void)
jpeg__destroy__decompress (j__decompress__ptr cinfo)
{
  jpeg__destroy((j__common__ptr) cinfo); /* use common routine */
}


/*
 * Abort processing of a JPEG decompression operation,
 * but don't destroy the object itself.
 */

GLOBAL(void)
jpeg__abort__decompress (j__decompress__ptr cinfo)
{
  jpeg__abort((j__common__ptr) cinfo); /* use common routine */
}


/*
 * Set default decompression parameters.
 */

LOCAL(void)
default__decompress__parms (j__decompress__ptr cinfo)
{
  /* Guess the input colorspace, and set output colorspace accordingly. */
  /* (Wish JPEG committee had provided a real way to specify this...) */
  /* Note application may override our guesses. */
  switch (cinfo->num__components) {
  case 1:
    cinfo->jpeg__color__space = JCS__GRAYSCALE;
    cinfo->out__color__space = JCS__GRAYSCALE;
    break;
    
  case 3:
    if (cinfo->saw__JFIF__marker) {
      cinfo->jpeg__color__space = JCS__YCbCr; /* JFIF implies YCbCr */
    } else if (cinfo->saw__Adobe__marker) {
      switch (cinfo->Adobe__transform) {
      case 0:
	cinfo->jpeg__color__space = JCS__RGB;
	break;
      case 1:
	cinfo->jpeg__color__space = JCS__YCbCr;
	break;
      default:
	WARNMS1(cinfo, JWRN__ADOBE__XFORM, cinfo->Adobe__transform);
	cinfo->jpeg__color__space = JCS__YCbCr; /* assume it's YCbCr */
	break;
      }
    } else {
      /* Saw no special markers, try to guess from the component IDs */
      int cid0 = cinfo->comp__info[0].component__id;
      int cid1 = cinfo->comp__info[1].component__id;
      int cid2 = cinfo->comp__info[2].component__id;

      if (cid0 == 1 && cid1 == 2 && cid2 == 3)
	cinfo->jpeg__color__space = JCS__YCbCr; /* assume JFIF w/out marker */
      else if (cid0 == 82 && cid1 == 71 && cid2 == 66)
	cinfo->jpeg__color__space = JCS__RGB; /* ASCII 'R', 'G', 'B' */
      else {
	TRACEMS3(cinfo, 1, JTRC__UNKNOWN__IDS, cid0, cid1, cid2);
	cinfo->jpeg__color__space = JCS__YCbCr; /* assume it's YCbCr */
      }
    }
    /* Always guess RGB is proper output colorspace. */
    cinfo->out__color__space = JCS__RGB;
    break;
    
  case 4:
    if (cinfo->saw__Adobe__marker) {
      switch (cinfo->Adobe__transform) {
      case 0:
	cinfo->jpeg__color__space = JCS__CMYK;
	break;
      case 2:
	cinfo->jpeg__color__space = JCS__YCCK;
	break;
      default:
	WARNMS1(cinfo, JWRN__ADOBE__XFORM, cinfo->Adobe__transform);
	cinfo->jpeg__color__space = JCS__YCCK; /* assume it's YCCK */
	break;
      }
    } else {
      /* No special markers, assume straight CMYK. */
      cinfo->jpeg__color__space = JCS__CMYK;
    }
    cinfo->out__color__space = JCS__CMYK;
    break;
    
  default:
    cinfo->jpeg__color__space = JCS__UNKNOWN;
    cinfo->out__color__space = JCS__UNKNOWN;
    break;
  }

  /* Set defaults for other decompression parameters. */
  cinfo->scale__num = 1;		/* 1:1 scaling */
  cinfo->scale__denom = 1;
  cinfo->output__gamma = 1.0;
  cinfo->buffered__image = FALSE;
  cinfo->raw__data__out = FALSE;
  cinfo->dct__method = JDCT__DEFAULT;
  cinfo->do__fancy__upsampling = TRUE;
  cinfo->do__block__smoothing = TRUE;
  cinfo->quantize__colors = FALSE;
  /* We set these in case application only sets quantize__colors. */
  cinfo->dither__mode = JDITHER__FS;
#ifdef QUANT__2PASS__SUPPORTED
  cinfo->two__pass__quantize = TRUE;
#else
  cinfo->two__pass__quantize = FALSE;
#endif
  cinfo->desired__number__of__colors = 256;
  cinfo->colormap = NULL;
  /* Initialize for no mode change in buffered-image mode. */
  cinfo->enable__1pass__quant = FALSE;
  cinfo->enable__external__quant = FALSE;
  cinfo->enable__2pass__quant = FALSE;
}


/*
 * Decompression startup: read start of JPEG datastream to see what's there.
 * Need only initialize JPEG object and supply a data source before calling.
 *
 * This routine will read as far as the first SOS marker (ie, actual start of
 * compressed data), and will save all tables and parameters in the JPEG
 * object.  It will also initialize the decompression parameters to default
 * values, and finally return JPEG__HEADER__OK.  On return, the application may
 * adjust the decompression parameters and then call jpeg__start__decompress.
 * (Or, if the application only wanted to determine the image parameters,
 * the data need not be decompressed.  In that case, call jpeg__abort or
 * jpeg__destroy to release any temporary space.)
 * If an abbreviated (tables only) datastream is presented, the routine will
 * return JPEG__HEADER__TABLES__ONLY upon reaching EOI.  The application may then
 * re-use the JPEG object to read the abbreviated image datastream(s).
 * It is unnecessary (but OK) to call jpeg__abort in this case.
 * The JPEG__SUSPENDED return code only occurs if the data source module
 * requests suspension of the decompressor.  In this case the application
 * should load more source data and then re-call jpeg__read__header to resume
 * processing.
 * If a non-suspending data source is used and require__image is TRUE, then the
 * return code need not be inspected since only JPEG__HEADER__OK is possible.
 *
 * This routine is now just a front end to jpeg__consume__input, with some
 * extra error checking.
 */

GLOBAL(int)
jpeg__read__header (j__decompress__ptr cinfo, CH_BOOL require__image)/*书强  原为CH_bool require__image*/
{
  int retcode;

  if (cinfo->global__state != DSTATE__START &&
      cinfo->global__state != DSTATE__INHEADER)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);

  retcode = jpeg__consume__input(cinfo);

  switch (retcode) {
  case JPEG__REACHED__SOS:
    retcode = JPEG__HEADER__OK;
    break;
  case JPEG__REACHED__EOI:
    if (require__image)		/* Complain if application wanted an image */
      ERREXIT(cinfo, JERR__NO__IMAGE);
    /* Reset to start state; it would be safer to require the application to
     * call jpeg__abort, but we can't change it now for compatibility reasons.
     * A side effect is to free any temporary memory (there shouldn't be any).
     */
    jpeg__abort((j__common__ptr) cinfo); /* sets state = DSTATE__START */
    retcode = JPEG__HEADER__TABLES__ONLY;
    break;
  case JPEG__SUSPENDED:
    /* no work */
    break;
  }

  return retcode;
}


/*
 * Consume data in advance of what the decompressor requires.
 * This can be called at any time once the decompressor object has
 * been created and a data source has been set up.
 *
 * This routine is essentially a state machine that handles a couple
 * of critical state-transition actions, namely initial setup and
 * transition from header scanning to ready-for-start__decompress.
 * All the actual input is done via the input controller's consume__input
 * method.
 */

GLOBAL(int)
jpeg__consume__input (j__decompress__ptr cinfo)
{
  int retcode = JPEG__SUSPENDED;

  /* NB: every possible DSTATE value should be listed in this switch */
  switch (cinfo->global__state) {
  case DSTATE__START:
    /* Start-of-datastream actions: reset appropriate modules */
    (*cinfo->inputctl->reset__input__controller) (cinfo);
    /* Initialize application's data source module */
    (*cinfo->src->init__source) (cinfo);
    cinfo->global__state = DSTATE__INHEADER;
    /*FALLTHROUGH*/
  case DSTATE__INHEADER:
    retcode = (*cinfo->inputctl->consume__input) (cinfo);
    if (retcode == JPEG__REACHED__SOS) { /* Found SOS, prepare to decompress */
      /* Set up default parameters based on header data */
      default__decompress__parms(cinfo);
      /* Set global state: ready for start__decompress */
      cinfo->global__state = DSTATE__READY;
    }
    break;
  case DSTATE__READY:
    /* Can't advance past first SOS until start__decompress is called */
    retcode = JPEG__REACHED__SOS;
    break;
  case DSTATE__PRELOAD:
  case DSTATE__PRESCAN:
  case DSTATE__SCANNING:
  case DSTATE__RAW__OK:
  case DSTATE__BUFIMAGE:
  case DSTATE__BUFPOST:
  case DSTATE__STOPPING:
    retcode = (*cinfo->inputctl->consume__input) (cinfo);
    break;
  default:
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  }
  return retcode;
}


/*
 * Have we finished reading the input file?
 */

GLOBAL(CH_BOOL)
jpeg__input__complete (j__decompress__ptr cinfo)
{
  /* Check for valid jpeg object */
  if (cinfo->global__state < DSTATE__START ||
      cinfo->global__state > DSTATE__STOPPING)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  return cinfo->inputctl->eoi__reached;
}


/*
 * Is there more than one scan?
 */

GLOBAL(CH_BOOL)
jpeg__has__multiple__scans (j__decompress__ptr cinfo)
{
  /* Only valid after jpeg__read__header completes */
  if (cinfo->global__state < DSTATE__READY ||
      cinfo->global__state > DSTATE__STOPPING)
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  return cinfo->inputctl->has__multiple__scans;
}


/*
 * Finish JPEG decompression.
 *
 * This will normally just verify the file trailer and release temp storage.
 *
 * Returns FALSE if suspended.  The return value need be inspected only if
 * a suspending data source is used.
 */

GLOBAL(CH_BOOL)
jpeg__finish__decompress (j__decompress__ptr cinfo)
{
  if ((cinfo->global__state == DSTATE__SCANNING ||
       cinfo->global__state == DSTATE__RAW__OK) && ! cinfo->buffered__image) {
    /* Terminate final pass of non-buffered mode */
    if (cinfo->output__scanline < cinfo->output__height)
      ERREXIT(cinfo, JERR__TOO__LITTLE__DATA);
    (*cinfo->master->finish__output__pass) (cinfo);
    cinfo->global__state = DSTATE__STOPPING;
  } else if (cinfo->global__state == DSTATE__BUFIMAGE) {
    /* Finishing after a buffered-image operation */
    cinfo->global__state = DSTATE__STOPPING;
  } else if (cinfo->global__state != DSTATE__STOPPING) {
    /* STOPPING = repeat call after a suspension, anything else is error */
    ERREXIT1(cinfo, JERR__BAD__STATE, cinfo->global__state);
  }
  /* Read until EOI */
  while (! cinfo->inputctl->eoi__reached) {
    if ((*cinfo->inputctl->consume__input) (cinfo) == JPEG__SUSPENDED)
      return FALSE;		/* Suspend, come back later */
  }
  /* Do final cleanup */
  (*cinfo->src->term__source) (cinfo);
  /* We can use jpeg__abort to release memory and reset global__state */
  jpeg__abort((j__common__ptr) cinfo);
  return TRUE;
}
