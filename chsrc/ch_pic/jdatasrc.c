/*
 * jdatasrc.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains decompression data source routines for the case of
 * reading JPEG data from a file (or any stdio stream).  While these routines
 * are sufficient for most applications, some will want to use a different
 * source manager.
 * IMPORTANT: we assume that fread() will correctly transcribe an array of
 * JOCTETs from 8-bit-wide elements on external storage.  If char is wider
 * than 8 bits on your machine, you may need to do some tweaking.
 */

/* this is not a core library module, so it doesn't define JPEG__INTERNALS */
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"


/* Expanded data source object for stdio input */

typedef struct {
  struct jpeg__source__mgr pub;	/* public fields */

  graph_file * infile;		/* source stream */
  JOCTET * buffer;		/* start of buffer */
  CH_BOOL start__of__file;	/* have we gotten any data yet? */
} my__source__mgr;

typedef my__source__mgr * my__src__ptr;

#define INPUT__BUF__SIZE  4096	/* choose an efficiently fread'able size */


/*
 * Initialize source --- called by jpeg__read__header
 * before any data is actually read.
 */

METHODDEF(void)
init__source (j__decompress__ptr cinfo)
{
  my__src__ptr src = (my__src__ptr) cinfo->src;

  /* We reset the empty-input-file flag for each image,
   * but we don't clear the input buffer.
   * This is correct behavior for reading a series of images from one source.
   */
  src->start__of__file = TRUE;
}


/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next__input__byte & bytes__in__buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of ERREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 *
 * In applications that need to be able to suspend compression due to input
 * not being available yet, a FALSE return indicates that no more data can be
 * obtained right now, but more may be forthcoming later.  In this situation,
 * the decompressor will return to its caller (with an indication of the
 * number of scanlines it has read, if any).  The application should resume
 * decompression after it has loaded more data into the input buffer.  Note
 * that there are substantial restrictions on the use of suspension --- see
 * the documentation.
 *
 * When suspending, the decompressor will back up to a convenient restart point
 * (typically the start of the current MCU). next__input__byte & bytes__in__buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point must be rescanned after resumption, so move it to
 * the front of the buffer rather than discarding it.
 */

METHODDEF(CH_BOOL)
fill__input__buffer (j__decompress__ptr cinfo)
{
  my__src__ptr src = (my__src__ptr) cinfo->src;
  size_t nbytes;

  nbytes = JFREAD(src->infile, src->buffer, INPUT__BUF__SIZE);

  if (nbytes <= 0) {
    if (src->start__of__file)	/* Treat empty input file as fatal error */
      ERREXIT(cinfo, JERR__INPUT__EMPTY);
    WARNMS(cinfo, JWRN__JPEG__EOF);
    /* Insert a fake EOI marker */
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG__EOI;
    nbytes = 2;
  }

  src->pub.next__input__byte = src->buffer;
  src->pub.bytes__in__buffer = nbytes;
  src->start__of__file = FALSE;

  return TRUE;
}


/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip__input__data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill__input__buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */

METHODDEF(void)
skip__input__data (j__decompress__ptr cinfo, long num__bytes)
{
  my__src__ptr src = (my__src__ptr) cinfo->src;

  /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.
   */
  if (num__bytes > 0) {
    while (num__bytes > (long) src->pub.bytes__in__buffer) {
      num__bytes -= (long) src->pub.bytes__in__buffer;
      (void) fill__input__buffer(cinfo);
      /* note we assume that fill__input__buffer will never return FALSE,
       * so suspension need not be handled.
       */
    }
    src->pub.next__input__byte += (size_t) num__bytes;
    src->pub.bytes__in__buffer -= (size_t) num__bytes;
  }
}


/*
 * An additional method that can be provided by data source modules is the
 * resync__to__restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */


/*
 * Terminate source --- called by jpeg__finish__decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg__abort or jpeg__destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

METHODDEF(void)
term__source (j__decompress__ptr cinfo)
{
  /* no work necessary here */
}


/*
 * Prepare for input from a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing decompression.
 */

GLOBAL(void)
jpeg__stdio__src (j__decompress__ptr cinfo, graph_file * infile)
{
  my__src__ptr src;

  /* The source object and input buffer are made permanent so that a series
   * of JPEG images can be read from the same file by calling jpeg__stdio__src
   * only before the first one.  (If we discarded the buffer at the end of
   * one image, we'd likely lose the start of the next one.)
   * This makes it unsafe to use this manager and a different source
   * manager serially with the same JPEG object.  Caveat programmer.
   */
  if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    cinfo->src = (struct jpeg__source__mgr *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__PERMANENT,
				  SIZEOF(my__source__mgr));
    src = (my__src__ptr) cinfo->src;
    src->buffer = (JOCTET *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__PERMANENT,
				  INPUT__BUF__SIZE * SIZEOF(JOCTET));
  }

  src = (my__src__ptr) cinfo->src;
  src->pub.init__source = init__source;
  src->pub.fill__input__buffer = fill__input__buffer;
  src->pub.skip__input__data = skip__input__data;
  src->pub.resync__to__restart = jpeg__resync__to__restart; /* use default method */
  src->pub.term__source = term__source;
  src->infile = infile;
  src->pub.bytes__in__buffer = 0; /* forces fill__input__buffer on first read */
  src->pub.next__input__byte = NULL; /* until buffer loaded */
}
