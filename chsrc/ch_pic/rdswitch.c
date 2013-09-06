/*
 * rdswitch.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to process some of cjpeg's more complicated
 * command-line switches.  Switches processed here are:
 *	-qtables file		Read quantization tables from text file
 *	-scans file		Read scan script from text file
 *	-qslots N[,N,...]	Set component quantization table selectors
 *	-sample HxV[,HxV,...]	Set component sampling factors
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */
#include <ctype.h>		/* to declare isdigit(), isspace() */
#include "graphconfig.h"


LOCAL(int)
text__getc (graph_file * file)
/* Read next char, skipping over any comments (# to end of line) */
/* A comment/newline sequence is returned as a newline */
{
  register int ch;
  
  ch = graph_getc(file);
  if (ch == '#') {
    do {
      ch = graph_getc(file);
    } while (ch != '\n' && ch != EOF);
  }
  return ch;
}


LOCAL(CH_BOOL)
read__text__integer (graph_file * file, long * result, int * termchar)
/* Read an unsigned decimal integer from a file, store it in result */
/* Reads one trailing character after the integer; returns it in termchar */
{
  register int ch;
  register long val;
  
  /* Skip any leading whitespace, detect EOF */
  do {
    ch = text__getc(file);
    if (ch == EOF) {
      *termchar = ch;
      return FALSE;
    }
  } while (isspace(ch));
  
  if (! isdigit(ch)) {
    *termchar = ch;
    return FALSE;
  }

  val = ch - '0';
  while ((ch = text__getc(file)) != EOF) {
    if (! isdigit(ch))
      break;
    val *= 10;
    val += ch - '0';
  }
  *result = val;
  *termchar = ch;
  return TRUE;
}


GLOBAL(CH_BOOL)
read__quant__tables (j__compress__ptr cinfo, char * filename,
		   int scale__factor, CH_BOOL force__baseline)
/* Read a set of quantization tables from the specified file.
 * The file is plain ASCII text: decimal numbers with whitespace between.
 * Comments preceded by '#' may be included in the file.
 * There may be one to NUM__QUANT__TBLS tables in the file, each of 64 values.
 * The tables are implicitly numbered 0,1,etc.
 * NOTE: does not affect the qslots mapping, which will default to selecting
 * table 0 for luminance (or primary) components, 1 for chrominance components.
 * You must use -qslots if you want a different component->table mapping.
 */
{
  graph_file * fp;
  int tblno, i, termchar;
  long val;
  unsigned int table[DCTSIZE2];

  if ((fp = graph_open(filename, "r",0)) == NULL) {
    fprintf(stderr, "Can't open table file %s\n", filename);
    return FALSE;
  }
  tblno = 0;

  while (read__text__integer(fp, &val, &termchar)) { /* read 1st element of table */
    if (tblno >= NUM__QUANT__TBLS) {
      fprintf(stderr, "Too many tables in file %s\n", filename);
      graph_close(fp);
      return FALSE;
    }
    table[0] = (unsigned int) val;
    for (i = 1; i < DCTSIZE2; i++) {
      if (! read__text__integer(fp, &val, &termchar)) {
	fprintf(stderr, "Invalid table data in file %s\n", filename);
	graph_close(fp);
	return FALSE;
      }
      table[i] = (unsigned int) val;
    }
    jpeg__add__quant__table(cinfo, tblno, table, scale__factor, force__baseline);
    tblno++;
  }

  if (termchar != EOF) {
    fprintf(stderr, "Non-numeric data in file %s\n", filename);
    graph_close(fp);
    return FALSE;
  }

  graph_close(fp);
  return TRUE;
}


#ifdef C__MULTISCAN__FILES__SUPPORTED

LOCAL(CH_BOOL)
read__scan__integer (graph_file * file, long * result, int * termchar)
/* Variant of read__text__integer that always looks for a non-space termchar;
 * this simplifies parsing of punctuation in scan scripts.
 */
{
  register int ch;

  if (! read__text__integer(file, result, termchar))
    return FALSE;
  ch = *termchar;
  while (ch != EOF && isspace(ch))
    ch = text__getc(file);
  if (isdigit(ch)) {		/* oops, put it back */
    if (graph_ungetc(ch, file) == EOF)
      return FALSE;
    ch = ' ';
  } else {
    /* Any separators other than ';' and ':' are ignored;
     * this allows user to insert commas, etc, if desired.
     */
    if (ch != EOF && ch != ';' && ch != ':')
      ch = ' ';
  }
  *termchar = ch;
  return TRUE;
}


GLOBAL(CH_BOOL)
read__scan__script (j__compress__ptr cinfo, char * filename)
/* Read a scan script from the specified text file.
 * Each entry in the file defines one scan to be emitted.
 * Entries are separated by semicolons ';'.
 * An entry contains one to four component indexes,
 * optionally followed by a colon ':' and four progressive-JPEG parameters.
 * The component indexes denote which component(s) are to be transmitted
 * in the current scan.  The first component has index 0.
 * Sequential JPEG is used if the progressive-JPEG parameters are omitted.
 * The file is free format text: any whitespace may appear between numbers
 * and the ':' and ';' punctuation marks.  Also, other punctuation (such
 * as commas or dashes) can be placed between numbers if desired.
 * Comments preceded by '#' may be included in the file.
 * Note: we do very little validity checking here;
 * jcmaster.c will validate the script parameters.
 */
{
  graph_file * fp;
  int scanno, ncomps, termchar;
  long val;
  jpeg__scan__info * scanptr;
#define MAX__SCANS  100		/* quite arbitrary limit */
  jpeg__scan__info scans[MAX__SCANS];

  if ((fp = graph_open(filename, "r",0)) == NULL) {
    fprintf(stderr, "Can't open scan definition file %s\n", filename);
    return FALSE;
  }
  scanptr = scans;
  scanno = 0;

  while (read__scan__integer(fp, &val, &termchar)) {
    if (scanno >= MAX__SCANS) {
      fprintf(stderr, "Too many scans defined in file %s\n", filename);
      graph_close(fp);
      return FALSE;
    }
    scanptr->component__index[0] = (int) val;
    ncomps = 1;
    while (termchar == ' ') {
      if (ncomps >= MAX__COMPS__IN__SCAN) {
	fprintf(stderr, "Too many components in one scan in file %s\n",
		filename);
	graph_close(fp);
	return FALSE;
      }
      if (! read__scan__integer(fp, &val, &termchar))
	goto bogus;
      scanptr->component__index[ncomps] = (int) val;
      ncomps++;
    }
    scanptr->comps__in__scan = ncomps;
    if (termchar == ':') {
      if (! read__scan__integer(fp, &val, &termchar) || termchar != ' ')
	goto bogus;
      scanptr->Ss = (int) val;
      if (! read__scan__integer(fp, &val, &termchar) || termchar != ' ')
	goto bogus;
      scanptr->Se = (int) val;
      if (! read__scan__integer(fp, &val, &termchar) || termchar != ' ')
	goto bogus;
      scanptr->Ah = (int) val;
      if (! read__scan__integer(fp, &val, &termchar))
	goto bogus;
      scanptr->Al = (int) val;
    } else {
      /* set non-progressive parameters */
      scanptr->Ss = 0;
      scanptr->Se = DCTSIZE2-1;
      scanptr->Ah = 0;
      scanptr->Al = 0;
    }
    if (termchar != ';' && termchar != EOF) {
bogus:
      fprintf(stderr, "Invalid scan entry format in file %s\n", filename);
      graph_close(fp);
      return FALSE;
    }
    scanptr++, scanno++;
  }

  if (termchar != EOF) {
    fprintf(stderr, "Non-numeric data in file %s\n", filename);
    graph_close(fp);
    return FALSE;
  }

  if (scanno > 0) {
    /* Stash completed scan list in cinfo structure.
     * NOTE: for cjpeg's use, JPOOL__IMAGE is the right lifetime for this data,
     * but if you want to compress multiple images you'd want JPOOL__PERMANENT.
     */
    scanptr = (jpeg__scan__info *)
      (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				  scanno * SIZEOF(jpeg__scan__info));
    MEMCOPY(scanptr, scans, scanno * SIZEOF(jpeg__scan__info));
    cinfo->scan__info = scanptr;
    cinfo->num__scans = scanno;
  }

  graph_close(fp);
  return TRUE;
}

#endif /* C__MULTISCAN__FILES__SUPPORTED */


GLOBAL(CH_BOOL)
set__quant__slots (j__compress__ptr cinfo, char *arg)
/* Process a quantization-table-selectors parameter string, of the form
 *     N[,N,...]
 * If there are more components than parameters, the last value is replicated.
 */
{
  int val = 0;			/* default table # */
  int ci;
  char ch;

  for (ci = 0; ci < MAX__COMPONENTS; ci++) {
    if (*arg) {
      ch = ',';			/* if not set by sscanf, will be ',' */
      if (sscanf(arg, "%d%c", &val, &ch) < 1)
	return FALSE;
      if (ch != ',')		/* syntax check */
	return FALSE;
      if (val < 0 || val >= NUM__QUANT__TBLS) {
	fprintf(stderr, "JPEG quantization tables are numbered 0..%d\n",
		NUM__QUANT__TBLS-1);
	return FALSE;
      }
      cinfo->comp__info[ci].quant__tbl__no = val;
      while (*arg && *arg++ != ',') /* advance to next segment of arg string */
	;
    } else {
      /* reached end of parameter, set remaining components to last table */
      cinfo->comp__info[ci].quant__tbl__no = val;
    }
  }
  return TRUE;
}


GLOBAL(CH_BOOL)
set__sample__factors (j__compress__ptr cinfo, char *arg)
/* Process a sample-factors parameter string, of the form
 *     HxV[,HxV,...]
 * If there are more components than parameters, "1x1" is assumed for the rest.
 */
{
  int ci, val1, val2;
  char ch1, ch2;

  for (ci = 0; ci < MAX__COMPONENTS; ci++) {
    if (*arg) {
      ch2 = ',';		/* if not set by sscanf, will be ',' */
      if (sscanf(arg, "%d%c%d%c", &val1, &ch1, &val2, &ch2) < 3)
	return FALSE;
      if ((ch1 != 'x' && ch1 != 'X') || ch2 != ',') /* syntax check */
	return FALSE;
      if (val1 <= 0 || val1 > 4 || val2 <= 0 || val2 > 4) {
	fprintf(stderr, "JPEG sampling factors must be 1..4\n");
	return FALSE;
      }
      cinfo->comp__info[ci].h__samp__factor = val1;
      cinfo->comp__info[ci].v__samp__factor = val2;
      while (*arg && *arg++ != ',') /* advance to next segment of arg string */
	;
    } else {
      /* reached end of parameter, set remaining components to 1x1 sampling */
      cinfo->comp__info[ci].h__samp__factor = 1;
      cinfo->comp__info[ci].v__samp__factor = 1;
    }
  }
  return TRUE;
}
