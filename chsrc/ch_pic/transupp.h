/*
 * transupp.h
 *
 * Copyright (C) 1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains declarations for image transformation routines and
 * other utility code used by the jpegtran sample application.  These are
 * NOT part of the core JPEG library.  But we keep these routines separate
 * from jpegtran.c to ease the task of maintaining jpegtran-like programs
 * that have other user interfaces.
 *
 * NOTE: all the routines declared here have very specific requirements
 * about when they are to be executed during the reading and writing of the
 * source and destination files.  See the comments in transupp.c, or see
 * jpegtran.c for an example of correct usage.
 */

/* If you happen not to want the image transform support, disable it here */
#ifndef TRANSFORMS__SUPPORTED
#define TRANSFORMS__SUPPORTED 1		/* 0 disables transform code */
#endif

/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED__SHORT__EXTERNAL__NAMES
#define jtransform__request__workspace		jTrRequest
#define jtransform__adjust__parameters		jTrAdjust
#define jtransform__execute__transformation	jTrExec
#define jcopy__markers__setup			jCMrkSetup
#define jcopy__markers__execute			jCMrkExec
#endif /* NEED__SHORT__EXTERNAL__NAMES */


/*
 * Codes for supported types of image transformations.
 */

typedef enum {
	JXFORM__NONE,		/* no transformation */
	JXFORM__FLIP__H,		/* horizontal flip */
	JXFORM__FLIP__V,		/* vertical flip */
	JXFORM__TRANSPOSE,	/* transpose across UL-to-LR axis */
	JXFORM__TRANSVERSE,	/* transpose across UR-to-LL axis */
	JXFORM__ROT__90,		/* 90-degree clockwise rotation */
	JXFORM__ROT__180,		/* 180-degree rotation */
	JXFORM__ROT__270		/* 270-degree clockwise (or 90 ccw) */
} JXFORM__CODE;

/*
 * Although rotating and flipping data expressed as DCT coefficients is not
 * hard, there is an asymmetry in the JPEG format specification for images
 * whose dimensions aren't multiples of the iMCU size.  The right and bottom
 * image edges are padded out to the next iMCU boundary with junk data; but
 * no padding is possible at the top and left edges.  If we were to flip
 * the whole image including the pad data, then pad garbage would become
 * visible at the top and/or left, and real pixels would disappear into the
 * pad margins --- perhaps permanently, since encoders & decoders may not
 * bother to preserve DCT blocks that appear to be completely outside the
 * nominal image area.  So, we have to exclude any partial iMCUs from the
 * basic transformation.
 *
 * Transpose is the only transformation that can handle partial iMCUs at the
 * right and bottom edges completely cleanly.  flip__h can flip partial iMCUs
 * at the bottom, but leaves any partial iMCUs at the right edge untouched.
 * Similarly flip__v leaves any partial iMCUs at the bottom edge untouched.
 * The other transforms are defined as combinations of these basic transforms
 * and process edge blocks in a way that preserves the equivalence.
 *
 * The "trim" option causes untransformable partial iMCUs to be dropped;
 * this is not strictly lossless, but it usually gives the best-looking
 * result for odd-size images.  Note that when this option is active,
 * the expected mathematical equivalences between the transforms may not hold.
 * (For example, -rot 270 -trim trims only the bottom edge, but -rot 90 -trim
 * followed by -rot 180 -trim trims both edges.)
 *
 * We also offer a "force to grayscale" option, which simply discards the
 * chrominance channels of a YCbCr image.  This is lossless in the sense that
 * the luminance channel is preserved exactly.  It's not the same kind of
 * thing as the rotate/flip transformations, but it's convenient to handle it
 * as part of this package, mainly because the transformation routines have to
 * be aware of the option to know how many components to work on.
 */

typedef struct {
  /* Options: set by caller */
  JXFORM__CODE transform;	/* image transform operator */
  CH_BOOL trim;			/* if TRUE, trim partial MCUs as needed */
  CH_BOOL force__grayscale;	/* if TRUE, convert color image to grayscale */

  /* Internal workspace: caller should not touch these */
  int num__components;		/* # of components in workspace */
  jvirt__barray__ptr * workspace__coef__arrays; /* workspace for transformations */
} jpeg__transform__info;


#if TRANSFORMS__SUPPORTED

/* Request any required workspace */
EXTERN(void) jtransform__request__workspace
	JPP((j__decompress__ptr srcinfo, jpeg__transform__info *info));
/* Adjust output image parameters */
EXTERN(jvirt__barray__ptr *) jtransform__adjust__parameters
	JPP((j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	     jvirt__barray__ptr *src__coef__arrays,
	     jpeg__transform__info *info));
/* Execute the actual transformation, if any */
EXTERN(void) jtransform__execute__transformation
	JPP((j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	     jvirt__barray__ptr *src__coef__arrays,
	     jpeg__transform__info *info));

#endif /* TRANSFORMS__SUPPORTED */


/*
 * Support for copying optional markers from source to destination file.
 */

typedef enum {
	JCOPYOPT__NONE,		/* copy no optional markers */
	JCOPYOPT__COMMENTS,	/* copy only comment (COM) markers */
	JCOPYOPT__ALL		/* copy all optional markers */
} JCOPY__OPTION;

#define JCOPYOPT__DEFAULT  JCOPYOPT__COMMENTS	/* recommended default */

/* Setup decompression object to save desired markers in memory */
EXTERN(void) jcopy__markers__setup
	JPP((j__decompress__ptr srcinfo, JCOPY__OPTION option));
/* Copy markers saved in the given source object to the destination object */
EXTERN(void) jcopy__markers__execute
	JPP((j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	     JCOPY__OPTION option));
