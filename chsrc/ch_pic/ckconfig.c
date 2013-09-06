/*
 * ckconfig.c
 *
 * Copyright (C) 1991-1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 */

/*
 * This program is intended to help you determine how to configure the JPEG
 * software for installation on a particular system.  The idea is to try to
 * compile and execute this program.  If your compiler fails to compile the
 * program, make changes as indicated in the comments below.  Once you can
 * compile the program, run it, and it will produce a "jconfig.h" file for
 * your system.
 *
 * As a general rule, each time you try to compile this program,
 * pay attention only to the *first* error message you get from the compiler.
 * Many C compilers will issue lots of spurious error messages once they
 * have gotten confused.  Go to the line indicated in the first error message,
 * and read the comments preceding that line to see what to change.
 *
 * Almost all of the edits you may need to make to this program consist of
 * changing a line that reads "#define SOME__SYMBOL" to "#undef SOME__SYMBOL",
 * or vice versa.  This is called defining or undefining that symbol.
 */


/* First we must see if your system has the include files we need.
 * We start out with the assumption that your system has all the ANSI-standard
 * include files.  If you get any error trying to include one of these files,
 * undefine the corresponding HAVE__xxx symbol.
 */

#define HAVE__STDDEF__H		/* replace 'define' by 'undef' if error here */
#ifdef HAVE__STDDEF__H		/* next line will be skipped if you undef... */
#include <stddef.h>
#endif

#define HAVE__STDLIB__H		/* same thing for stdlib.h */
#ifdef HAVE__STDLIB__H
#include <stdlib.h>
#endif

#include <stdio.h>		/* If you ain't got this, you ain't got C. */

/* We have to see if your string functions are defined by
 * strings.h (old BSD convention) or string.h (everybody else).
 * We try the non-BSD convention first; define NEED__BSD__STRINGS
 * if the compiler says it can't find string.h.
 */

#undef NEED__BSD__STRINGS

#ifdef NEED__BSD__STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

/* On some systems (especially older Unix machines), type size_t is
 * defined only in the include file <sys/types.h>.  If you get a failure
 * on the size_t test below, try defining NEED__SYS__TYPES__H.
 */

#undef NEED__SYS__TYPES__H		/* start by assuming we don't need it */
#ifdef NEED__SYS__TYPES__H
#include <sys/types.h>
#endif


/* Usually type size_t is defined in one of the include files we've included
 * above.  If not, you'll get an error on the "typedef size_t my__size_t;" line.
 * In that case, first try defining NEED__SYS__TYPES__H just above.
 * If that doesn't work, you'll have to search through your system library
 * to figure out which include file defines "size_t".  Look for a line that
 * says "typedef something-or-other size_t;".  Then, change the line below
 * that says "#include <someincludefile.h>" to instead include the file
 * you found size_t in, and define NEED__SPECIAL__INCLUDE.  If you can't find
 * type size_t anywhere, try replacing "#include <someincludefile.h>" with
 * "typedef unsigned int size_t;".
 */

#undef NEED__SPECIAL__INCLUDE	/* assume we DON'T need it, for starters */

#ifdef NEED__SPECIAL__INCLUDE
#include <someincludefile.h>
#endif

typedef size_t my__size_t;	/* The payoff: do we have size_t now? */


/* The next question is whether your compiler supports ANSI-style function
 * prototypes.  You need to know this in order to choose between using
 * makefile.ansi and using makefile.unix.
 * The #define line below is set to assume you have ANSI function prototypes.
 * If you get an error in this group of lines, undefine HAVE__PROTOTYPES.
 */
#define HAVE__PROTOTYPES

#ifdef HAVE__PROTOTYPES
int testfunction (int arg1, int * arg2); /* check prototypes */

struct methods__struct {		/* check method-pointer declarations */
  int (*error__exit) (char *msgtext);
  int (*trace__message) (char *msgtext);
  int (*another__method) (void);
};

int testfunction (int arg1, int * arg2) /* check definitions */
{
  return arg2[arg1];
}

int test2function (void)	/* check void arg list */
{
  return 0;
}
#endif


/* Now we want to find out if your compiler knows what "unsigned char" means.
 * If you get an error on the "unsigned char un__char;" line,
 * then undefine HAVE__UNSIGNED__CHAR.
 */

#define HAVE__UNSIGNED__CHAR

#ifdef HAVE__UNSIGNED__CHAR
unsigned char un__char;
#endif


/* Now we want to find out if your compiler knows what "unsigned short" means.
 * If you get an error on the "unsigned short un__short;" line,
 * then undefine HAVE__UNSIGNED__SHORT.
 */

#define HAVE__UNSIGNED__SHORT

#ifdef HAVE__UNSIGNED__SHORT
unsigned short un__short;
#endif


/* Now we want to find out if your compiler understands type "void".
 * If you get an error anywhere in here, undefine HAVE__VOID.
 */

#define HAVE__VOID

#ifdef HAVE__VOID
/* Caution: a C++ compiler will insist on complete prototypes */
typedef void * void__ptr;	/* check void * */
#ifdef HAVE__PROTOTYPES		/* check ptr to function returning void */
typedef void (*void__func) (int a, int b);
#else
typedef void (*void__func) ();
#endif

#ifdef HAVE__PROTOTYPES		/* check void function result */
void test3function (void__ptr arg1, void__func arg2)
#else
void test3function (arg1, arg2)
     void__ptr arg1;
     void__func arg2;
#endif
{
  char * locptr = (char *) arg1; /* check casting to and from void * */
  arg1 = (void *) locptr;
  (*arg2) (1, 2);		/* check call of fcn returning void */
}
#endif


/* Now we want to find out if your compiler knows what "const" means.
 * If you get an error here, undefine HAVE__CONST.
 */

#define HAVE__CONST

#ifdef HAVE__CONST
static const int carray[3] = {1, 2, 3};

#ifdef HAVE__PROTOTYPES
int test4function (const int arg1)
#else
int test4function (arg1)
     const int arg1;
#endif
{
  return carray[arg1];
}
#endif


/* If you get an error or warning about this structure definition,
 * define INCOMPLETE__TYPES__BROKEN.
 */

#undef INCOMPLETE__TYPES__BROKEN

#ifndef INCOMPLETE__TYPES__BROKEN
typedef struct undefined__structure * undef__struct__ptr;
#endif


/* If you get an error about duplicate names,
 * define NEED__SHORT__EXTERNAL__NAMES.
 */

#undef NEED__SHORT__EXTERNAL__NAMES

#ifndef NEED__SHORT__EXTERNAL__NAMES

int possibly__duplicate__function ()
{
  return 0;
}

int possibly__dupli__function ()
{
  return 1;
}

#endif



/************************************************************************
 *  OK, that's it.  You should not have to change anything beyond this
 *  point in order to compile and execute this program.  (You might get
 *  some warnings, but you can ignore them.)
 *  When you run the program, it will make a couple more tests that it
 *  can do automatically, and then it will create jconfig.h and print out
 *  any additional suggestions it has.
 ************************************************************************
 */


#ifdef HAVE__PROTOTYPES
int is__char__signed (int arg)
#else
int is__char__signed (arg)
     int arg;
#endif
{
  if (arg == 189) {		/* expected result for unsigned char */
    return 0;			/* type char is unsigned */
  }
  else if (arg != -67) {	/* expected result for signed char */
#ifdef   	USE__printFunc
    printf("Hmm, it seems 'char' is not eight bits wide on your machine.\n");
    printf("I fear the JPEG software will not work at all.\n\n");
#endif	
  }
  return 1;			/* assume char is signed otherwise */
}


#ifdef HAVE__PROTOTYPES
int is__shifting__signed (long arg)
#else
int is__shifting__signed (arg)
     long arg;
#endif
/* See whether right-shift on a long is signed or not. */
{
  long res = arg >> 4;

  if (res == -0x7F7E80CL) {	/* expected result for signed shift */
    return 1;			/* right shift is signed */
  }
  /* see if unsigned-shift hack will fix it. */
  /* we can't just test exact value since it depends on width of long... */
  res |= (~0L) << (32-4);
  if (res == -0x7F7E80CL) {	/* expected result now? */
    return 0;			/* right shift is unsigned */
  }
 #ifdef   	USE__printFunc
  printf("Right shift isn't acting as I expect it to.\n");
  printf("I fear the JPEG software will not work at all.\n\n");
#endif  
  return 0;			/* try it with unsigned anyway */
}

#if 0
#ifdef HAVE__PROTOTYPES
int ckconfig__main (int argc, char ** argv)
#else
int ckconfig__main (argc, argv) /*ÎÒ¸ÃµÄ*/
     int argc;
     char ** argv;
#endif
{
  char signed__char__check = (char) (-67);
  FILE *outfile;

  /* Attempt to write jconfig.h */
  if ((outfile = fopen("jconfig.h", "w")) == NULL) {
    printf("Failed to write jconfig.h\n");
    return 1;
  }

  /* Write out all the info */
  fprintf(outfile, "/* jconfig.h --- generated by ckconfig.c */\n");
  fprintf(outfile, "/* see jconfig.doc for explanations */\n\n");
#ifdef HAVE__PROTOTYPES
  fprintf(outfile, "#define HAVE__PROTOTYPES\n");
#else
  fprintf(outfile, "#undef HAVE__PROTOTYPES\n");
#endif
#ifdef HAVE__UNSIGNED__CHAR
  fprintf(outfile, "#define HAVE__UNSIGNED__CHAR\n");
#else
  fprintf(outfile, "#undef HAVE__UNSIGNED__CHAR\n");
#endif
#ifdef HAVE__UNSIGNED__SHORT
  fprintf(outfile, "#define HAVE__UNSIGNED__SHORT\n");
#else
  fprintf(outfile, "#undef HAVE__UNSIGNED__SHORT\n");
#endif
#ifdef HAVE__VOID
  fprintf(outfile, "/* #define void char */\n");
#else
  fprintf(outfile, "#define void char\n");
#endif
#ifdef HAVE__CONST
  fprintf(outfile, "/* #define const */\n");
#else
  fprintf(outfile, "#define const\n");
#endif
  if (is__char__signed((int) signed__char__check))
    fprintf(outfile, "#undef CHAR__IS__UNSIGNED\n");
  else
    fprintf(outfile, "#define CHAR__IS__UNSIGNED\n");
#ifdef HAVE__STDDEF__H
  fprintf(outfile, "#define HAVE__STDDEF__H\n");
#else
  fprintf(outfile, "#undef HAVE__STDDEF__H\n");
#endif
#ifdef HAVE__STDLIB__H
  fprintf(outfile, "#define HAVE__STDLIB__H\n");
#else
  fprintf(outfile, "#undef HAVE__STDLIB__H\n");
#endif
#ifdef NEED__BSD__STRINGS
  fprintf(outfile, "#define NEED__BSD__STRINGS\n");
#else
  fprintf(outfile, "#undef NEED__BSD__STRINGS\n");
#endif
#ifdef NEED__SYS__TYPES__H
  fprintf(outfile, "#define NEED__SYS__TYPES__H\n");
#else
  fprintf(outfile, "#undef NEED__SYS__TYPES__H\n");
#endif
  fprintf(outfile, "#undef NEED__FAR__POINTERS\n");
#ifdef NEED__SHORT__EXTERNAL__NAMES
  fprintf(outfile, "#define NEED__SHORT__EXTERNAL__NAMES\n");
#else
  fprintf(outfile, "#undef NEED__SHORT__EXTERNAL__NAMES\n");
#endif
#ifdef INCOMPLETE__TYPES__BROKEN
  fprintf(outfile, "#define INCOMPLETE__TYPES__BROKEN\n");
#else
  fprintf(outfile, "#undef INCOMPLETE__TYPES__BROKEN\n");
#endif
  fprintf(outfile, "\n#ifdef JPEG__INTERNALS\n\n");
  if (is__shifting__signed(-0x7F7E80B1L))
    fprintf(outfile, "#undef RIGHT__SHIFT__IS__UNSIGNED\n");
  else
    fprintf(outfile, "#define RIGHT__SHIFT__IS__UNSIGNED\n");
  fprintf(outfile, "\n#endif /* JPEG__INTERNALS */\n");
  fprintf(outfile, "\n#ifdef JPEG__CJPEG__DJPEG\n\n");
  fprintf(outfile, "#define BMP__SUPPORTED		/* BMP image file format */\n");
  fprintf(outfile, "#define GIF__SUPPORTED		/* GIF image file format */\n");
  fprintf(outfile, "#define PPM__SUPPORTED		/* PBMPLUS PPM/PGM image file format */\n");
  fprintf(outfile, "#undef RLE__SUPPORTED		/* Utah RLE image file format */\n");
  fprintf(outfile, "#define TARGA__SUPPORTED		/* Targa image file format */\n\n");
  fprintf(outfile, "#undef TWO__FILE__COMMANDLINE	/* You may need this on non-Unix systems */\n");
  fprintf(outfile, "#undef NEED__SIGNAL__CATCHER	/* Define this if you use jmemname.c */\n");
  fprintf(outfile, "#undef DONT__USE__B__MODE\n");
  fprintf(outfile, "/* #define PROGRESS__REPORT */	/* optional */\n");
  fprintf(outfile, "\n#endif /* JPEG__CJPEG__DJPEG */\n");

  /* Close the jconfig.h file */
  fclose(outfile);

  /* User report */
  printf("Configuration check for Independent JPEG Group's software done.\n");
  printf("\nI have written the jconfig.h file for you.\n\n");
#ifdef HAVE__PROTOTYPES
  printf("You should use makefile.ansi as the starting point for your Makefile.\n");
#else
  printf("You should use makefile.unix as the starting point for your Makefile.\n");
#endif

#ifdef NEED__SPECIAL__INCLUDE
  printf("\nYou'll need to change jconfig.h to include the system include file\n");
  printf("that you found type size_t in, or add a direct definition of type\n");
  printf("size_t if that's what you used.  Just add it to the end.\n");
#endif

  return 0;
}
#endif
