/*
 * jdatadst.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains compression data destination routines for the case of
 * emitting JPEG data to a file (or any stdio stream).  While these routines
 * are sufficient for most applications, some will want to use a different
 * destination manager.
 * IMPORTANT: we assume that fwrite() will correctly transcribe an array of
 * JOCTETs into 8-bit-wide elements on external storage.  If char is wider
 * than 8 bits on your machine, you may need to do some tweaking.
 */

/* this is not a core library module, so it doesn't define JPEG__INTERNALS */
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"


/* Expanded data destination object for stdio output */

typedef struct {
  struct jpeg__destination__mgr pub; /* public fields */

  graph_file * outfile;		/* target stream */
  JOCTET * buffer;		/* start of buffer */
} my__destination__mgr;

typedef my__destination__mgr * my__dest__ptr;

#define OUTPUT__BUF__SIZE  4096	/* choose an efficiently fwrite'able size */


/*
 * Initialize destination --- called by jpeg__start__compress
 * before any data is actually written.
 */

METHODDEF(void)
init__destination (j__compress__ptr cinfo)
{
  my__dest__ptr dest = (my__dest__ptr) cinfo->dest;

  /* Allocate the output buffer --- it will be released when done with image */
  dest->buffer = (JOCTET *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  OUTPUT__BUF__SIZE * SIZEOF(JOCTET));

  dest->pub.next__output__byte = dest->buffer;
  dest->pub.free__in__buffer = OUTPUT__BUF__SIZE;
}


/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next__output__byte & free__in__buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).  The
 * application should resume compression after it has made more room in the
 * output buffer.  Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next__output__byte & free__in__buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */

METHODDEF(CH_BOOL)
empty__output__buffer (j__compress__ptr cinfo)
{
  my__dest__ptr dest = (my__dest__ptr) cinfo->dest;

  if (JFWRITE( (FILE*)dest->outfile, dest->buffer, OUTPUT__BUF__SIZE) !=
      (size_t) OUTPUT__BUF__SIZE)
    ERREXIT(cinfo, JERR__FILE__WRITE);

  dest->pub.next__output__byte = dest->buffer;
  dest->pub.free__in__buffer = OUTPUT__BUF__SIZE;

  return TRUE;
}


/*
 * Terminate destination --- called by jpeg__finish__compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg__abort or jpeg__destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

METHODDEF(void)
term__destination (j__compress__ptr cinfo)
{
  my__dest__ptr dest = (my__dest__ptr) cinfo->dest;
  size_t datacount = OUTPUT__BUF__SIZE - dest->pub.free__in__buffer;

  /* Write any data remaining in the buffer */
  if (datacount > 0) {
    if (JFWRITE( (FILE*)dest->outfile, dest->buffer, datacount) != datacount)
      ERREXIT(cinfo, JERR__FILE__WRITE);
  }
  fflush( (FILE*)dest->outfile);
  /* Make sure we wrote the output file OK */
  #if 0  /**/
  if (ferror(dest->outfile))
    ERREXIT(cinfo, JERR__FILE__WRITE);
  #endif
}


/*
 * Prepare for output to a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing compression.
 */

GLOBAL(void)
jpeg__stdio__dest (j__compress__ptr cinfo, graph_file * outfile)
{
  my__dest__ptr dest;

  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg__stdio__dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */
  if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
    cinfo->dest = (struct jpeg__destination__mgr *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__PERMANENT,
				  SIZEOF(my__destination__mgr));
  }

  dest = (my__dest__ptr) cinfo->dest;
  dest->pub.init__destination = init__destination;
  dest->pub.empty__output__buffer = empty__output__buffer;
  dest->pub.term__destination = term__destination;
  dest->outfile = outfile;
}
