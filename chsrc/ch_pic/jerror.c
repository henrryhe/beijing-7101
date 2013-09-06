/*
 * jerror.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains simple error-reporting and trace-message routines.
 * These are suitable for Unix-like systems and others where writing to
 * stderr is the right thing to do.  Many applications will want to replace
 * some or all of these routines.
 *
 * If you define USE__WINDOWS__MESSAGEBOX in jconfig.h or in the makefile,
 * you get a Windows-specific hack to display error messages in a dialog box.
 * It ain't much, but it beats dropping error messages into the bit bucket,
 * which is what happens to output to stderr under most Windows C compilers.
 *
 * These routines are used by both the compression and decompression code.
 */

/* this is not a core library module, so it doesn't define JPEG__INTERNALS */
#include "jinclude.h"
#include "jpeglib.h"
#include "jversion.h"
#include "jerror.h"

#ifdef USE__WINDOWS__MESSAGEBOX
#include <windows.h>
#endif

#ifndef EXIT__FAILURE		/* define exit() codes if not provided */
#define EXIT__FAILURE  1
#endif


/*
 * Create the message string table.
 * We do this from the master message list in jerror.h by re-reading
 * jerror.h with a suitable definition for macro JMESSAGE.
 * The message table is made an external symbol just in case any applications
 * want to refer to it directly.
 */

#ifdef NEED__SHORT__EXTERNAL__NAMES
#define jpeg__std__message__table	jMsgTable
#endif

#define JMESSAGE(code,string)	string ,

const char * const jpeg__std__message__table[] = {
#include "jerror.h"
  NULL
};


/*
 * Error exit handler: must not return to caller.
 *
 * Applications may override this if they want to get control back after
 * an error.  Typically one would longjmp somewhere instead of exiting.
 * The setjmp buffer can be made a private field within an expanded error
 * handler object.  Note that the info needed to generate an error message
 * is stored in the error object, so you can generate the message now or
 * later, at your convenience.
 * You should make sure that the JPEG object is cleaned up (with jpeg__abort
 * or jpeg__destroy) at some point.
 */

METHODDEF(void)
error__exit (j__common__ptr cinfo)
{
  /* Always display the message */
  (*cinfo->err->output__message) (cinfo);

  /* Let the memory manager delete any temp files before we die */
  jpeg__destroy(cinfo);

  /*exit(EXIT__FAILURE);*/
  return;
}


/*
 * Actual output of an error or trace message.
 * Applications may override this method to send JPEG messages somewhere
 * other than stderr.
 *
 * On Windows, printing to stderr is generally completely useless,
 * so we provide optional code to produce an error-dialog popup.
 * Most Windows applications will still prefer to override this routine,
 * but if they don't, it'll do something at least marginally useful.
 *
 * NOTE: to use the library in an environment that doesn't support the
 * C stdio library, you may have to delete the call to fprintf() entirely,
 * not just not use this routine.
 */

METHODDEF(void)
output__message (j__common__ptr cinfo)
{
  char buffer[JMSG__LENGTH__MAX];

  /* Create the message */
  (*cinfo->err->format__message) (cinfo, buffer);

#ifdef USE__WINDOWS__MESSAGEBOX
  /* Display it in a message dialog box */
  MessageBox(GetActiveWindow(), buffer, "JPEG Library Error",
	     MB__OK | MB__ICONERROR);
#else
  /* Send it to stderr, adding a newline */
  fprintf(stderr, "%s\n", buffer);
#endif
}


/*
 * Decide whether to emit a trace or warning message.
 * msg__level is one of:
 *   -1: recoverable corrupt-data warning, may want to abort.
 *    0: important advisory messages (always display to user).
 *    1: first level of tracing detail.
 *    2,3,...: successively more detailed tracing messages.
 * An application might override this method if it wanted to abort on warnings
 * or change the policy about which messages to display.
 */

METHODDEF(void)
emit__message (j__common__ptr cinfo, int msg__level)
{
  struct jpeg__error__mgr * err = cinfo->err;

  if (msg__level < 0) {
    /* It's a warning message.  Since corrupt files may generate many warnings,
     * the policy implemented here is to show only the first warning,
     * unless trace__level >= 3.
     */
    if (err->num__warnings == 0 || err->trace__level >= 3)
      (*err->output__message) (cinfo);
    /* Always count warnings in num__warnings. */
    err->num__warnings++;
  } else {
    /* It's a trace message.  Show it if trace__level >= msg__level. */
    if (err->trace__level >= msg__level)
      (*err->output__message) (cinfo);
  }
}


/*
 * Format a message string for the most recent JPEG error or message.
 * The message is stored into buffer, which should be at least JMSG__LENGTH__MAX
 * characters.  Note that no '\n' character is added to the string.
 * Few applications should need to override this method.
 */

METHODDEF(void)
format__message (j__common__ptr cinfo, char * buffer)
{
  struct jpeg__error__mgr * err = cinfo->err;
  int msg__code = err->msg__code;
  const char * msgtext = NULL;
  const char * msgptr;
  char ch;
  CH_BOOL isstring;

  /* Look up message string in proper table */
  if (msg__code > 0 && msg__code <= err->last__jpeg__message) {
    msgtext = err->jpeg__message__table[msg__code];
  } else if (err->addon__message__table != NULL &&
	     msg__code >= err->first__addon__message &&
	     msg__code <= err->last__addon__message) {
    msgtext = err->addon__message__table[msg__code - err->first__addon__message];
  }

  /* Defend against bogus message number */
  if (msgtext == NULL) {
    err->msg__parm.i[0] = msg__code;
    msgtext = err->jpeg__message__table[0];
  }

  /* Check for string parameter, as indicated by %s in the message text */
  isstring = FALSE;
  msgptr = msgtext;
  while ((ch = *msgptr++) != '\0') {
    if (ch == '%') {
      if (*msgptr == 's') isstring = TRUE;
      break;
    }
  }

  /* Format the message into the passed buffer */
  if (isstring)
    sprintf(buffer, msgtext, err->msg__parm.s);
  else
    sprintf(buffer, msgtext,
	    err->msg__parm.i[0], err->msg__parm.i[1],
	    err->msg__parm.i[2], err->msg__parm.i[3],
	    err->msg__parm.i[4], err->msg__parm.i[5],
	    err->msg__parm.i[6], err->msg__parm.i[7]);
}


/*
 * Reset error state variables at start of a new image.
 * This is called during compression startup to reset trace/error
 * processing to default state, without losing any application-specific
 * method pointers.  An application might possibly want to override
 * this method if it has additional error processing state.
 */

METHODDEF(void)
reset__error__mgr (j__common__ptr cinfo)
{
  cinfo->err->num__warnings = 0;
  /* trace__level is not reset since it is an application-supplied parameter */
  cinfo->err->msg__code = 0;	/* may be useful as a flag for "no error" */
}


/*
 * Fill in the standard error-handling methods in a jpeg__error__mgr object.
 * Typical call is:
 *	struct jpeg__compress__struct cinfo;
 *	struct jpeg__error__mgr err;
 *
 *	cinfo.err = jpeg__std__error(&err);
 * after which the application may override some of the methods.
 */

GLOBAL(struct jpeg__error__mgr *)
jpeg__std__error (struct jpeg__error__mgr * err)
{
  err->error__exit = error__exit;
  err->emit__message = emit__message;
  err->output__message = output__message;
  err->format__message = format__message;
  err->reset__error__mgr = reset__error__mgr;

  err->trace__level = 0;		/* default = no tracing */
  err->num__warnings = 0;	/* no warnings emitted yet */
  err->msg__code = 0;		/* may be useful as a flag for "no error" */

  /* Initialize message table pointers */
  err->jpeg__message__table = jpeg__std__message__table;
  err->last__jpeg__message = (int) JMSG__LASTMSGCODE - 1;

  err->addon__message__table = NULL;
  err->first__addon__message = 0;	/* for safety */
  err->last__addon__message = 0;

  return err;
}
