/*
 * cdjpeg.h
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains common declarations for the sample applications
 * cjpeg and djpeg.  It is NOT used by the core JPEG library.
 */

#define JPEG__CJPEG__DJPEG	/* define proper options in jconfig.h */
#define JPEG__INTERNAL__OPTIONS	/* cjpeg.c,djpeg.c need to see xxx__SUPPORTED */
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"		/* get library error codes too */
#include "cderror.h"		/* get application-specific error codes */


/*
 * Object interface for cjpeg's source file decoding modules
 */

typedef struct cjpeg__source__struct * cjpeg__source__ptr;

struct cjpeg__source__struct {
  JMETHOD(void, start__input, (j__compress__ptr cinfo,
			      cjpeg__source__ptr sinfo));
  JMETHOD(JDIMENSION, get__pixel__rows, (j__compress__ptr cinfo,
				       cjpeg__source__ptr sinfo));
  JMETHOD(void, finish__input, (j__compress__ptr cinfo,
			       cjpeg__source__ptr sinfo));

  FILE *input__file;

  JSAMPARRAY buffer;
  JDIMENSION buffer__height;
};


/*
 * Object interface for djpeg's output file encoding modules
 */

typedef struct djpeg__dest__struct * djpeg__dest__ptr;

struct djpeg__dest__struct {
  /* start__output is called after jpeg__start__decompress finishes.
   * The color map will be ready at this time, if one is needed.
   */
  JMETHOD(void, start__output, (j__decompress__ptr cinfo,
			       djpeg__dest__ptr dinfo));
  /* Emit the specified number of pixel rows from the buffer. */
  JMETHOD(void, put__pixel__rows, (j__decompress__ptr cinfo,
				 djpeg__dest__ptr dinfo,
				 JDIMENSION rows__supplied));
  /* Finish up at the end of the image. */
  JMETHOD(void, finish__output, (j__decompress__ptr cinfo,
				djpeg__dest__ptr dinfo));

  /* Target file spec; filled in by djpeg.c after object is created. */
  graph_file * output__file;

  /* Output pixel-row buffer.  Created by module init or start__output.
   * Width is cinfo->output__width * cinfo->output__components;
   * height is buffer__height.
   */
  JSAMPARRAY buffer;
  JDIMENSION buffer__height;
};


/*
 * cjpeg/djpeg may need to perform extra passes to convert to or from
 * the source/destination file format.  The JPEG library does not know
 * about these passes, but we'd like them to be counted by the progress
 * monitor.  We use an expanded progress monitor object to hold the
 * additional pass count.
 */

struct cdjpeg__progress__mgr {
  struct jpeg__progress__mgr pub;	/* fields known to JPEG library */
  int completed__extra__passes;	/* extra passes completed */
  int total__extra__passes;	/* total extra */
  /* last printed percentage stored here to avoid multiple printouts */
  int percent__done;
};

typedef struct cdjpeg__progress__mgr * cd__progress__ptr;


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED__SHORT__EXTERNAL__NAMES
#define jinit__read__bmp		jIRdBMP
#define jinit__write__bmp		jIWrBMP
#define jinit__read__gif		jIRdGIF
#define jinit__write__gif		jIWrGIF
#define jinit__read__ppm		jIRdPPM
#define jinit__write__ppm		jIWrPPM
#define jinit__read__rle		jIRdRLE
#define jinit__write__rle		jIWrRLE
#define jinit__read__targa	jIRdTarga
#define jinit__write__targa	jIWrTarga
#define read__quant__tables	RdQTables
#define read__scan__script	RdScnScript
#define set__quant__slots		SetQSlots
#define set__sample__factors	SetSFacts
#define read__color__map		RdCMap
#define enable__signal__catcher	EnSigCatcher
#define start__progress__monitor	StProgMon
#define end__progress__monitor	EnProgMon
#define read__stdin		RdStdin
#define write__stdout		WrStdout
#endif /* NEED__SHORT__EXTERNAL__NAMES */

/* Module selection routines for I/O modules. */

EXTERN(cjpeg__source__ptr) jinit__read__bmp JPP((j__compress__ptr cinfo));
EXTERN(djpeg__dest__ptr) jinit__write__bmp JPP((j__decompress__ptr cinfo,
					    CH_BOOL is__os2));
EXTERN(cjpeg__source__ptr) jinit__read__gif JPP((j__compress__ptr cinfo));
EXTERN(djpeg__dest__ptr) jinit__write__gif JPP((j__decompress__ptr cinfo));
EXTERN(cjpeg__source__ptr) jinit__read__ppm JPP((j__compress__ptr cinfo));
EXTERN(djpeg__dest__ptr) jinit__write__ppm JPP((j__decompress__ptr cinfo));
EXTERN(cjpeg__source__ptr) jinit__read__rle JPP((j__compress__ptr cinfo));
EXTERN(djpeg__dest__ptr) jinit__write__rle JPP((j__decompress__ptr cinfo));
EXTERN(cjpeg__source__ptr) jinit__read__targa JPP((j__compress__ptr cinfo));
EXTERN(djpeg__dest__ptr) jinit__write__targa JPP((j__decompress__ptr cinfo));

/* cjpeg support routines (in rdswitch.c) */

EXTERN(CH_BOOL) read__quant__tables JPP((j__compress__ptr cinfo, char * filename,
				    int scale__factor, CH_BOOL force__baseline));
EXTERN(CH_BOOL) read__scan__script JPP((j__compress__ptr cinfo, char * filename));
EXTERN(CH_BOOL) set__quant__slots JPP((j__compress__ptr cinfo, char *arg));
EXTERN(CH_BOOL) set__sample__factors JPP((j__compress__ptr cinfo, char *arg));

/* djpeg support routines (in rdcolmap.c) */

EXTERN(void) read__color__map JPP((j__decompress__ptr cinfo, graph_file * infile));

/* common support routines (in cdjpeg.c) */

EXTERN(void) enable__signal__catcher JPP((j__common__ptr cinfo));
EXTERN(void) start__progress__monitor JPP((j__common__ptr cinfo,
					 cd__progress__ptr progress));
EXTERN(void) end__progress__monitor JPP((j__common__ptr cinfo));
EXTERN(CH_BOOL) keymatch JPP((char * arg, const char * keyword, int minchars));
EXTERN(graph_file *) read__stdin JPP((void));
EXTERN(graph_file *) write__stdout JPP((void));

/* miscellaneous useful macros */

#ifdef DONT__USE__B__MODE		/* define mode parameters for fopen() */
#define READ__BINARY	"r"
#define WRITE__BINARY	"w"
#else
#ifdef VMS			/* VMS is very nonstandard */
#define READ__BINARY	"rb", "ctx=stm"
#define WRITE__BINARY	"wb", "ctx=stm"
#else				/* standard ANSI-compliant case */
#define READ__BINARY	"rb"
#define WRITE__BINARY	"wb"
#endif
#endif

#ifndef EXIT__FAILURE		/* define exit() codes if not provided */
#define EXIT__FAILURE  1
#endif
#ifndef EXIT__SUCCESS
#ifdef VMS
#define EXIT__SUCCESS  1		/* VMS is very nonstandard */
#else
#define EXIT__SUCCESS  0
#endif
#endif
#ifndef EXIT__WARNING
#ifdef VMS
#define EXIT__WARNING  1		/* VMS is very nonstandard */
#else
#define EXIT__WARNING  2
#endif
#endif
