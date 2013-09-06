/*
 * transupp.c
 *
 * Copyright (C) 1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains image transformation routines and other utility code
 * used by the jpegtran sample application.  These are NOT part of the core
 * JPEG library.  But we keep these routines separate from jpegtran.c to
 * ease the task of maintaining jpegtran-like programs that have other user
 * interfaces.
 */

/* Although this file really shouldn't have access to the library internals,
 * it's helpful to let it call jround__up() and jcopy__block__row().
 */
#define JPEG__INTERNALS

#include "jinclude.h"
#include "jpeglib.h"
#include "transupp.h"		/* My own external interface */


#if TRANSFORMS__SUPPORTED

/*
 * Lossless image transformation routines.  These routines work on DCT
 * coefficient arrays and thus do not require any lossy decompression
 * or recompression of the image.
 * Thanks to Guido Vollbeding for the initial design and code of this feature.
 *
 * Horizontal flipping is done in-place, using a single top-to-bottom
 * pass through the virtual source array.  It will thus be much the
 * fastest option for images larger than main memory.
 *
 * The other routines require a set of destination virtual arrays, so they
 * need twice as much memory as jpegtran normally does.  The destination
 * arrays are always written in normal scan order (top to bottom) because
 * the virtual array manager expects this.  The source arrays will be scanned
 * in the corresponding order, which means multiple passes through the source
 * arrays for most of the transforms.  That could result in much thrashing
 * if the image is larger than main memory.
 *
 * Some notes about the operating environment of the individual transform
 * routines:
 * 1. Both the source and destination virtual arrays are allocated from the
 *    source JPEG object, and therefore should be manipulated by calling the
 *    source's memory manager.
 * 2. The destination's component count should be used.  It may be smaller
 *    than the source's when forcing to grayscale.
 * 3. Likewise the destination's sampling factors should be used.  When
 *    forcing to grayscale the destination's sampling factors will be all 1,
 *    and we may as well take that as the effective iMCU size.
 * 4. When "trim" is in effect, the destination's dimensions will be the
 *    trimmed values but the source's will be untrimmed.
 * 5. All the routines assume that the source and destination buffers are
 *    padded out to a full iMCU boundary.  This is true, although for the
 *    source buffer it is an undocumented property of jdcoefct.c.
 * Notes 2,3,4 boil down to this: generally we should use the destination's
 * dimensions and ignore the source's.
 */


LOCAL(void)
do__flip__h (j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	   jvirt__barray__ptr *src__coef__arrays)
/* Horizontal flip; done in-place, so no separate dest array is required */
{
  JDIMENSION MCU__cols, comp__width, blk__x, blk__y;
  int ci, k, offset__y;
  JBLOCKARRAY buffer;
  JCOEFPTR ptr1, ptr2;
  JCOEF temp1, temp2;
  jpeg__component__info *compptr;

  /* Horizontal mirroring of DCT blocks is accomplished by swapping
   * pairs of blocks in-place.  Within a DCT block, we perform horizontal
   * mirroring by changing the signs of odd-numbered columns.
   * Partial iMCUs at the right edge are left untouched.
   */
  MCU__cols = dstinfo->image__width / (dstinfo->max__h__samp__factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num__components; ci++) {
    compptr = dstinfo->comp__info + ci;
    comp__width = MCU__cols * compptr->h__samp__factor;
    for (blk__y = 0; blk__y < compptr->height__in__blocks;
	 blk__y += compptr->v__samp__factor) {
      buffer = (*srcinfo->mem->access__virt__barray)
	((j__common__ptr) srcinfo, src__coef__arrays[ci], blk__y,
	 (JDIMENSION) compptr->v__samp__factor, TRUE);
      for (offset__y = 0; offset__y < compptr->v__samp__factor; offset__y++) {
	for (blk__x = 0; blk__x * 2 < comp__width; blk__x++) {
	  ptr1 = buffer[offset__y][blk__x];
	  ptr2 = buffer[offset__y][comp__width - blk__x - 1];
	  /* this unrolled loop doesn't need to know which row it's on... */
	  for (k = 0; k < DCTSIZE2; k += 2) {
	    temp1 = *ptr1;	/* swap even column */
	    temp2 = *ptr2;
	    *ptr1++ = temp2;
	    *ptr2++ = temp1;
	    temp1 = *ptr1;	/* swap odd column with sign change */
	    temp2 = *ptr2;
	    *ptr1++ = -temp2;
	    *ptr2++ = -temp1;
	  }
	}
      }
    }
  }
}


LOCAL(void)
do__flip__v (j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	   jvirt__barray__ptr *src__coef__arrays,
	   jvirt__barray__ptr *dst__coef__arrays)
/* Vertical flip */
{
  JDIMENSION MCU__rows, comp__height, dst__blk__x, dst__blk__y;
  int ci, i, j, offset__y;
  JBLOCKARRAY src__buffer, dst__buffer;
  JBLOCKROW src__row__ptr, dst__row__ptr;
  JCOEFPTR src__ptr, dst__ptr;
  jpeg__component__info *compptr;

  /* We output into a separate array because we can't touch different
   * rows of the source virtual array simultaneously.  Otherwise, this
   * is a pretty straightforward analog of horizontal flip.
   * Within a DCT block, vertical mirroring is done by changing the signs
   * of odd-numbered rows.
   * Partial iMCUs at the bottom edge are copied verbatim.
   */
  MCU__rows = dstinfo->image__height / (dstinfo->max__v__samp__factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num__components; ci++) {
    compptr = dstinfo->comp__info + ci;
    comp__height = MCU__rows * compptr->v__samp__factor;
    for (dst__blk__y = 0; dst__blk__y < compptr->height__in__blocks;
	 dst__blk__y += compptr->v__samp__factor) {
      dst__buffer = (*srcinfo->mem->access__virt__barray)
	((j__common__ptr) srcinfo, dst__coef__arrays[ci], dst__blk__y,
	 (JDIMENSION) compptr->v__samp__factor, TRUE);
      if (dst__blk__y < comp__height) {
	/* Row is within the mirrorable area. */
	src__buffer = (*srcinfo->mem->access__virt__barray)
	  ((j__common__ptr) srcinfo, src__coef__arrays[ci],
	   comp__height - dst__blk__y - (JDIMENSION) compptr->v__samp__factor,
	   (JDIMENSION) compptr->v__samp__factor, FALSE);
      } else {
	/* Bottom-edge blocks will be copied verbatim. */
	src__buffer = (*srcinfo->mem->access__virt__barray)
	  ((j__common__ptr) srcinfo, src__coef__arrays[ci], dst__blk__y,
	   (JDIMENSION) compptr->v__samp__factor, FALSE);
      }
      for (offset__y = 0; offset__y < compptr->v__samp__factor; offset__y++) {
	if (dst__blk__y < comp__height) {
	  /* Row is within the mirrorable area. */
	  dst__row__ptr = dst__buffer[offset__y];
	  src__row__ptr = src__buffer[compptr->v__samp__factor - offset__y - 1];
	  for (dst__blk__x = 0; dst__blk__x < compptr->width__in__blocks;
	       dst__blk__x++) {
	    dst__ptr = dst__row__ptr[dst__blk__x];
	    src__ptr = src__row__ptr[dst__blk__x];
	    for (i = 0; i < DCTSIZE; i += 2) {
	      /* copy even row */
	      for (j = 0; j < DCTSIZE; j++)
		*dst__ptr++ = *src__ptr++;
	      /* copy odd row with sign change */
	      for (j = 0; j < DCTSIZE; j++)
		*dst__ptr++ = - *src__ptr++;
	    }
	  }
	} else {
	  /* Just copy row verbatim. */
	  jcopy__block__row(src__buffer[offset__y], dst__buffer[offset__y],
			  compptr->width__in__blocks);
	}
      }
    }
  }
}


LOCAL(void)
do__transpose (j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	      jvirt__barray__ptr *src__coef__arrays,
	      jvirt__barray__ptr *dst__coef__arrays)
/* Transpose source into destination */
{
  JDIMENSION dst__blk__x, dst__blk__y;
  int ci, i, j, offset__x, offset__y;
  JBLOCKARRAY src__buffer, dst__buffer;
  JCOEFPTR src__ptr, dst__ptr;
  jpeg__component__info *compptr;

  /* Transposing pixels within a block just requires transposing the
   * DCT coefficients.
   * Partial iMCUs at the edges require no special treatment; we simply
   * process all the available DCT blocks for every component.
   */
  for (ci = 0; ci < dstinfo->num__components; ci++) {
    compptr = dstinfo->comp__info + ci;
    for (dst__blk__y = 0; dst__blk__y < compptr->height__in__blocks;
	 dst__blk__y += compptr->v__samp__factor) {
      dst__buffer = (*srcinfo->mem->access__virt__barray)
	((j__common__ptr) srcinfo, dst__coef__arrays[ci], dst__blk__y,
	 (JDIMENSION) compptr->v__samp__factor, TRUE);
      for (offset__y = 0; offset__y < compptr->v__samp__factor; offset__y++) {
	for (dst__blk__x = 0; dst__blk__x < compptr->width__in__blocks;
	     dst__blk__x += compptr->h__samp__factor) {
	  src__buffer = (*srcinfo->mem->access__virt__barray)
	    ((j__common__ptr) srcinfo, src__coef__arrays[ci], dst__blk__x,
	     (JDIMENSION) compptr->h__samp__factor, FALSE);
	  for (offset__x = 0; offset__x < compptr->h__samp__factor; offset__x++) {
	    src__ptr = src__buffer[offset__x][dst__blk__y + offset__y];
	    dst__ptr = dst__buffer[offset__y][dst__blk__x + offset__x];
	    for (i = 0; i < DCTSIZE; i++)
	      for (j = 0; j < DCTSIZE; j++)
		dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
	  }
	}
      }
    }
  }
}


LOCAL(void)
do__rot__90 (j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	   jvirt__barray__ptr *src__coef__arrays,
	   jvirt__barray__ptr *dst__coef__arrays)
/* 90 degree rotation is equivalent to
 *   1. Transposing the image;
 *   2. Horizontal mirroring.
 * These two steps are merged into a single processing routine.
 */
{
  JDIMENSION MCU__cols, comp__width, dst__blk__x, dst__blk__y;
  int ci, i, j, offset__x, offset__y;
  JBLOCKARRAY src__buffer, dst__buffer;
  JCOEFPTR src__ptr, dst__ptr;
  jpeg__component__info *compptr;

  /* Because of the horizontal mirror step, we can't process partial iMCUs
   * at the (output) right edge properly.  They just get transposed and
   * not mirrored.
   */
  MCU__cols = dstinfo->image__width / (dstinfo->max__h__samp__factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num__components; ci++) {
    compptr = dstinfo->comp__info + ci;
    comp__width = MCU__cols * compptr->h__samp__factor;
    for (dst__blk__y = 0; dst__blk__y < compptr->height__in__blocks;
	 dst__blk__y += compptr->v__samp__factor) {
      dst__buffer = (*srcinfo->mem->access__virt__barray)
	((j__common__ptr) srcinfo, dst__coef__arrays[ci], dst__blk__y,
	 (JDIMENSION) compptr->v__samp__factor, TRUE);
      for (offset__y = 0; offset__y < compptr->v__samp__factor; offset__y++) {
	for (dst__blk__x = 0; dst__blk__x < compptr->width__in__blocks;
	     dst__blk__x += compptr->h__samp__factor) {
	  src__buffer = (*srcinfo->mem->access__virt__barray)
	    ((j__common__ptr) srcinfo, src__coef__arrays[ci], dst__blk__x,
	     (JDIMENSION) compptr->h__samp__factor, FALSE);
	  for (offset__x = 0; offset__x < compptr->h__samp__factor; offset__x++) {
	    src__ptr = src__buffer[offset__x][dst__blk__y + offset__y];
	    if (dst__blk__x < comp__width) {
	      /* Block is within the mirrorable area. */
	      dst__ptr = dst__buffer[offset__y]
		[comp__width - dst__blk__x - offset__x - 1];
	      for (i = 0; i < DCTSIZE; i++) {
		for (j = 0; j < DCTSIZE; j++)
		  dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
		i++;
		for (j = 0; j < DCTSIZE; j++)
		  dst__ptr[j*DCTSIZE+i] = -src__ptr[i*DCTSIZE+j];
	      }
	    } else {
	      /* Edge blocks are transposed but not mirrored. */
	      dst__ptr = dst__buffer[offset__y][dst__blk__x + offset__x];
	      for (i = 0; i < DCTSIZE; i++)
		for (j = 0; j < DCTSIZE; j++)
		  dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
	    }
	  }
	}
      }
    }
  }
}


LOCAL(void)
do__rot__270 (j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	    jvirt__barray__ptr *src__coef__arrays,
	    jvirt__barray__ptr *dst__coef__arrays)
/* 270 degree rotation is equivalent to
 *   1. Horizontal mirroring;
 *   2. Transposing the image.
 * These two steps are merged into a single processing routine.
 */
{
  JDIMENSION MCU__rows, comp__height, dst__blk__x, dst__blk__y;
  int ci, i, j, offset__x, offset__y;
  JBLOCKARRAY src__buffer, dst__buffer;
  JCOEFPTR src__ptr, dst__ptr;
  jpeg__component__info *compptr;

  /* Because of the horizontal mirror step, we can't process partial iMCUs
   * at the (output) bottom edge properly.  They just get transposed and
   * not mirrored.
   */
  MCU__rows = dstinfo->image__height / (dstinfo->max__v__samp__factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num__components; ci++) {
    compptr = dstinfo->comp__info + ci;
    comp__height = MCU__rows * compptr->v__samp__factor;
    for (dst__blk__y = 0; dst__blk__y < compptr->height__in__blocks;
	 dst__blk__y += compptr->v__samp__factor) {
      dst__buffer = (*srcinfo->mem->access__virt__barray)
	((j__common__ptr) srcinfo, dst__coef__arrays[ci], dst__blk__y,
	 (JDIMENSION) compptr->v__samp__factor, TRUE);
      for (offset__y = 0; offset__y < compptr->v__samp__factor; offset__y++) {
	for (dst__blk__x = 0; dst__blk__x < compptr->width__in__blocks;
	     dst__blk__x += compptr->h__samp__factor) {
	  src__buffer = (*srcinfo->mem->access__virt__barray)
	    ((j__common__ptr) srcinfo, src__coef__arrays[ci], dst__blk__x,
	     (JDIMENSION) compptr->h__samp__factor, FALSE);
	  for (offset__x = 0; offset__x < compptr->h__samp__factor; offset__x++) {
	    dst__ptr = dst__buffer[offset__y][dst__blk__x + offset__x];
	    if (dst__blk__y < comp__height) {
	      /* Block is within the mirrorable area. */
	      src__ptr = src__buffer[offset__x]
		[comp__height - dst__blk__y - offset__y - 1];
	      for (i = 0; i < DCTSIZE; i++) {
		for (j = 0; j < DCTSIZE; j++) {
		  dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
		  j++;
		  dst__ptr[j*DCTSIZE+i] = -src__ptr[i*DCTSIZE+j];
		}
	      }
	    } else {
	      /* Edge blocks are transposed but not mirrored. */
	      src__ptr = src__buffer[offset__x][dst__blk__y + offset__y];
	      for (i = 0; i < DCTSIZE; i++)
		for (j = 0; j < DCTSIZE; j++)
		  dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
	    }
	  }
	}
      }
    }
  }
}


LOCAL(void)
do__rot__180 (j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	    jvirt__barray__ptr *src__coef__arrays,
	    jvirt__barray__ptr *dst__coef__arrays)
/* 180 degree rotation is equivalent to
 *   1. Vertical mirroring;
 *   2. Horizontal mirroring.
 * These two steps are merged into a single processing routine.
 */
{
  JDIMENSION MCU__cols, MCU__rows, comp__width, comp__height, dst__blk__x, dst__blk__y;
  int ci, i, j, offset__y;
  JBLOCKARRAY src__buffer, dst__buffer;
  JBLOCKROW src__row__ptr, dst__row__ptr;
  JCOEFPTR src__ptr, dst__ptr;
  jpeg__component__info *compptr;

  MCU__cols = dstinfo->image__width / (dstinfo->max__h__samp__factor * DCTSIZE);
  MCU__rows = dstinfo->image__height / (dstinfo->max__v__samp__factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num__components; ci++) {
    compptr = dstinfo->comp__info + ci;
    comp__width = MCU__cols * compptr->h__samp__factor;
    comp__height = MCU__rows * compptr->v__samp__factor;
    for (dst__blk__y = 0; dst__blk__y < compptr->height__in__blocks;
	 dst__blk__y += compptr->v__samp__factor) {
      dst__buffer = (*srcinfo->mem->access__virt__barray)
	((j__common__ptr) srcinfo, dst__coef__arrays[ci], dst__blk__y,
	 (JDIMENSION) compptr->v__samp__factor, TRUE);
      if (dst__blk__y < comp__height) {
	/* Row is within the vertically mirrorable area. */
	src__buffer = (*srcinfo->mem->access__virt__barray)
	  ((j__common__ptr) srcinfo, src__coef__arrays[ci],
	   comp__height - dst__blk__y - (JDIMENSION) compptr->v__samp__factor,
	   (JDIMENSION) compptr->v__samp__factor, FALSE);
      } else {
	/* Bottom-edge rows are only mirrored horizontally. */
	src__buffer = (*srcinfo->mem->access__virt__barray)
	  ((j__common__ptr) srcinfo, src__coef__arrays[ci], dst__blk__y,
	   (JDIMENSION) compptr->v__samp__factor, FALSE);
      }
      for (offset__y = 0; offset__y < compptr->v__samp__factor; offset__y++) {
	if (dst__blk__y < comp__height) {
	  /* Row is within the mirrorable area. */
	  dst__row__ptr = dst__buffer[offset__y];
	  src__row__ptr = src__buffer[compptr->v__samp__factor - offset__y - 1];
	  /* Process the blocks that can be mirrored both ways. */
	  for (dst__blk__x = 0; dst__blk__x < comp__width; dst__blk__x++) {
	    dst__ptr = dst__row__ptr[dst__blk__x];
	    src__ptr = src__row__ptr[comp__width - dst__blk__x - 1];
	    for (i = 0; i < DCTSIZE; i += 2) {
	      /* For even row, negate every odd column. */
	      for (j = 0; j < DCTSIZE; j += 2) {
		*dst__ptr++ = *src__ptr++;
		*dst__ptr++ = - *src__ptr++;
	      }
	      /* For odd row, negate every even column. */
	      for (j = 0; j < DCTSIZE; j += 2) {
		*dst__ptr++ = - *src__ptr++;
		*dst__ptr++ = *src__ptr++;
	      }
	    }
	  }
	  /* Any remaining right-edge blocks are only mirrored vertically. */
	  for (; dst__blk__x < compptr->width__in__blocks; dst__blk__x++) {
	    dst__ptr = dst__row__ptr[dst__blk__x];
	    src__ptr = src__row__ptr[dst__blk__x];
	    for (i = 0; i < DCTSIZE; i += 2) {
	      for (j = 0; j < DCTSIZE; j++)
		*dst__ptr++ = *src__ptr++;
	      for (j = 0; j < DCTSIZE; j++)
		*dst__ptr++ = - *src__ptr++;
	    }
	  }
	} else {
	  /* Remaining rows are just mirrored horizontally. */
	  dst__row__ptr = dst__buffer[offset__y];
	  src__row__ptr = src__buffer[offset__y];
	  /* Process the blocks that can be mirrored. */
	  for (dst__blk__x = 0; dst__blk__x < comp__width; dst__blk__x++) {
	    dst__ptr = dst__row__ptr[dst__blk__x];
	    src__ptr = src__row__ptr[comp__width - dst__blk__x - 1];
	    for (i = 0; i < DCTSIZE2; i += 2) {
	      *dst__ptr++ = *src__ptr++;
	      *dst__ptr++ = - *src__ptr++;
	    }
	  }
	  /* Any remaining right-edge blocks are only copied. */
	  for (; dst__blk__x < compptr->width__in__blocks; dst__blk__x++) {
	    dst__ptr = dst__row__ptr[dst__blk__x];
	    src__ptr = src__row__ptr[dst__blk__x];
	    for (i = 0; i < DCTSIZE2; i++)
	      *dst__ptr++ = *src__ptr++;
	  }
	}
      }
    }
  }
}


LOCAL(void)
do__transverse (j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
	       jvirt__barray__ptr *src__coef__arrays,
	       jvirt__barray__ptr *dst__coef__arrays)
/* Transverse transpose is equivalent to
 *   1. 180 degree rotation;
 *   2. Transposition;
 * or
 *   1. Horizontal mirroring;
 *   2. Transposition;
 *   3. Horizontal mirroring.
 * These steps are merged into a single processing routine.
 */
{
  JDIMENSION MCU__cols, MCU__rows, comp__width, comp__height, dst__blk__x, dst__blk__y;
  int ci, i, j, offset__x, offset__y;
  JBLOCKARRAY src__buffer, dst__buffer;
  JCOEFPTR src__ptr, dst__ptr;
  jpeg__component__info *compptr;

  MCU__cols = dstinfo->image__width / (dstinfo->max__h__samp__factor * DCTSIZE);
  MCU__rows = dstinfo->image__height / (dstinfo->max__v__samp__factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num__components; ci++) {
    compptr = dstinfo->comp__info + ci;
    comp__width = MCU__cols * compptr->h__samp__factor;
    comp__height = MCU__rows * compptr->v__samp__factor;
    for (dst__blk__y = 0; dst__blk__y < compptr->height__in__blocks;
	 dst__blk__y += compptr->v__samp__factor) {
      dst__buffer = (*srcinfo->mem->access__virt__barray)
	((j__common__ptr) srcinfo, dst__coef__arrays[ci], dst__blk__y,
	 (JDIMENSION) compptr->v__samp__factor, TRUE);
      for (offset__y = 0; offset__y < compptr->v__samp__factor; offset__y++) {
	for (dst__blk__x = 0; dst__blk__x < compptr->width__in__blocks;
	     dst__blk__x += compptr->h__samp__factor) {
	  src__buffer = (*srcinfo->mem->access__virt__barray)
	    ((j__common__ptr) srcinfo, src__coef__arrays[ci], dst__blk__x,
	     (JDIMENSION) compptr->h__samp__factor, FALSE);
	  for (offset__x = 0; offset__x < compptr->h__samp__factor; offset__x++) {
	    if (dst__blk__y < comp__height) {
	      src__ptr = src__buffer[offset__x]
		[comp__height - dst__blk__y - offset__y - 1];
	      if (dst__blk__x < comp__width) {
		/* Block is within the mirrorable area. */
		dst__ptr = dst__buffer[offset__y]
		  [comp__width - dst__blk__x - offset__x - 1];
		for (i = 0; i < DCTSIZE; i++) {
		  for (j = 0; j < DCTSIZE; j++) {
		    dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
		    j++;
		    dst__ptr[j*DCTSIZE+i] = -src__ptr[i*DCTSIZE+j];
		  }
		  i++;
		  for (j = 0; j < DCTSIZE; j++) {
		    dst__ptr[j*DCTSIZE+i] = -src__ptr[i*DCTSIZE+j];
		    j++;
		    dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
		  }
		}
	      } else {
		/* Right-edge blocks are mirrored in y only */
		dst__ptr = dst__buffer[offset__y][dst__blk__x + offset__x];
		for (i = 0; i < DCTSIZE; i++) {
		  for (j = 0; j < DCTSIZE; j++) {
		    dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
		    j++;
		    dst__ptr[j*DCTSIZE+i] = -src__ptr[i*DCTSIZE+j];
		  }
		}
	      }
	    } else {
	      src__ptr = src__buffer[offset__x][dst__blk__y + offset__y];
	      if (dst__blk__x < comp__width) {
		/* Bottom-edge blocks are mirrored in x only */
		dst__ptr = dst__buffer[offset__y]
		  [comp__width - dst__blk__x - offset__x - 1];
		for (i = 0; i < DCTSIZE; i++) {
		  for (j = 0; j < DCTSIZE; j++)
		    dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
		  i++;
		  for (j = 0; j < DCTSIZE; j++)
		    dst__ptr[j*DCTSIZE+i] = -src__ptr[i*DCTSIZE+j];
		}
	      } else {
		/* At lower right corner, just transpose, no mirroring */
		dst__ptr = dst__buffer[offset__y][dst__blk__x + offset__x];
		for (i = 0; i < DCTSIZE; i++)
		  for (j = 0; j < DCTSIZE; j++)
		    dst__ptr[j*DCTSIZE+i] = src__ptr[i*DCTSIZE+j];
	      }
	    }
	  }
	}
      }
    }
  }
}


/* Request any required workspace.
 *
 * We allocate the workspace virtual arrays from the source decompression
 * object, so that all the arrays (both the original data and the workspace)
 * will be taken into account while making memory management decisions.
 * Hence, this routine must be called after jpeg__read__header (which reads
 * the image dimensions) and before jpeg__read__coefficients (which realizes
 * the source's virtual arrays).
 */

GLOBAL(void)
jtransform__request__workspace (j__decompress__ptr srcinfo,
			      jpeg__transform__info *info)
{
  jvirt__barray__ptr *coef__arrays = NULL;
  jpeg__component__info *compptr;
  int ci;

  if (info->force__grayscale &&
      srcinfo->jpeg__color__space == JCS__YCbCr &&
      srcinfo->num__components == 3) {
    /* We'll only process the first component */
    info->num__components = 1;
  } else {
    /* Process all the components */
    info->num__components = srcinfo->num__components;
  }

  switch (info->transform) {
  case JXFORM__NONE:
  case JXFORM__FLIP__H:
    /* Don't need a workspace array */
    break;
  case JXFORM__FLIP__V:
  case JXFORM__ROT__180:
    /* Need workspace arrays having same dimensions as source image.
     * Note that we allocate arrays padded out to the next iMCU boundary,
     * so that transform routines need not worry about missing edge blocks.
     */
    coef__arrays = (jvirt__barray__ptr *)
      (*srcinfo->mem->alloc__small) ((j__common__ptr) srcinfo, JPOOL__IMAGE,
	SIZEOF(jvirt__barray__ptr) * info->num__components);
    for (ci = 0; ci < info->num__components; ci++) {
      compptr = srcinfo->comp__info + ci;
      coef__arrays[ci] = (*srcinfo->mem->request__virt__barray)
	((j__common__ptr) srcinfo, JPOOL__IMAGE, FALSE,
	 (JDIMENSION) jround__up((long) compptr->width__in__blocks,
				(long) compptr->h__samp__factor),
	 (JDIMENSION) jround__up((long) compptr->height__in__blocks,
				(long) compptr->v__samp__factor),
	 (JDIMENSION) compptr->v__samp__factor);
    }
    break;
  case JXFORM__TRANSPOSE:
  case JXFORM__TRANSVERSE:
  case JXFORM__ROT__90:
  case JXFORM__ROT__270:
    /* Need workspace arrays having transposed dimensions.
     * Note that we allocate arrays padded out to the next iMCU boundary,
     * so that transform routines need not worry about missing edge blocks.
     */
    coef__arrays = (jvirt__barray__ptr *)
      (*srcinfo->mem->alloc__small) ((j__common__ptr) srcinfo, JPOOL__IMAGE,
	SIZEOF(jvirt__barray__ptr) * info->num__components);
    for (ci = 0; ci < info->num__components; ci++) {
      compptr = srcinfo->comp__info + ci;
      coef__arrays[ci] = (*srcinfo->mem->request__virt__barray)
	((j__common__ptr) srcinfo, JPOOL__IMAGE, FALSE,
	 (JDIMENSION) jround__up((long) compptr->height__in__blocks,
				(long) compptr->v__samp__factor),
	 (JDIMENSION) jround__up((long) compptr->width__in__blocks,
				(long) compptr->h__samp__factor),
	 (JDIMENSION) compptr->h__samp__factor);
    }
    break;
  }
  info->workspace__coef__arrays = coef__arrays;
}


/* Transpose destination image parameters */

LOCAL(void)
transpose__critical__parameters (j__compress__ptr dstinfo)
{
  int tblno, i, j, ci, itemp;
  jpeg__component__info *compptr;
  JQUANT__TBL *qtblptr;
  JDIMENSION dtemp;
  UINT16 qtemp;

  /* Transpose basic image dimensions */
  dtemp = dstinfo->image__width;
  dstinfo->image__width = dstinfo->image__height;
  dstinfo->image__height = dtemp;

  /* Transpose sampling factors */
  for (ci = 0; ci < dstinfo->num__components; ci++) {
    compptr = dstinfo->comp__info + ci;
    itemp = compptr->h__samp__factor;
    compptr->h__samp__factor = compptr->v__samp__factor;
    compptr->v__samp__factor = itemp;
  }

  /* Transpose quantization tables */
  for (tblno = 0; tblno < NUM__QUANT__TBLS; tblno++) {
    qtblptr = dstinfo->quant__tbl__ptrs[tblno];
    if (qtblptr != NULL) {
      for (i = 0; i < DCTSIZE; i++) {
	for (j = 0; j < i; j++) {
	  qtemp = qtblptr->quantval[i*DCTSIZE+j];
	  qtblptr->quantval[i*DCTSIZE+j] = qtblptr->quantval[j*DCTSIZE+i];
	  qtblptr->quantval[j*DCTSIZE+i] = qtemp;
	}
      }
    }
  }
}


/* Trim off any partial iMCUs on the indicated destination edge */

LOCAL(void)
trim__right__edge (j__compress__ptr dstinfo)
{
  int ci, max__h__samp__factor;
  JDIMENSION MCU__cols;

  /* We have to compute max__h__samp__factor ourselves,
   * because it hasn't been set yet in the destination
   * (and we don't want to use the source's value).
   */
  max__h__samp__factor = 1;
  for (ci = 0; ci < dstinfo->num__components; ci++) {
    int h__samp__factor = dstinfo->comp__info[ci].h__samp__factor;
    max__h__samp__factor = MAX(max__h__samp__factor, h__samp__factor);
  }
  MCU__cols = dstinfo->image__width / (max__h__samp__factor * DCTSIZE);
  if (MCU__cols > 0)		/* can't trim to 0 pixels */
    dstinfo->image__width = MCU__cols * (max__h__samp__factor * DCTSIZE);
}

LOCAL(void)
trim__bottom__edge (j__compress__ptr dstinfo)
{
  int ci, max__v__samp__factor;
  JDIMENSION MCU__rows;

  /* We have to compute max__v__samp__factor ourselves,
   * because it hasn't been set yet in the destination
   * (and we don't want to use the source's value).
   */
  max__v__samp__factor = 1;
  for (ci = 0; ci < dstinfo->num__components; ci++) {
    int v__samp__factor = dstinfo->comp__info[ci].v__samp__factor;
    max__v__samp__factor = MAX(max__v__samp__factor, v__samp__factor);
  }
  MCU__rows = dstinfo->image__height / (max__v__samp__factor * DCTSIZE);
  if (MCU__rows > 0)		/* can't trim to 0 pixels */
    dstinfo->image__height = MCU__rows * (max__v__samp__factor * DCTSIZE);
}


/* Adjust output image parameters as needed.
 *
 * This must be called after jpeg__copy__critical__parameters()
 * and before jpeg__write__coefficients().
 *
 * The return value is the set of virtual coefficient arrays to be written
 * (either the ones allocated by jtransform__request__workspace, or the
 * original source data arrays).  The caller will need to pass this value
 * to jpeg__write__coefficients().
 */

GLOBAL(jvirt__barray__ptr *)
jtransform__adjust__parameters (j__decompress__ptr srcinfo,
			      j__compress__ptr dstinfo,
			      jvirt__barray__ptr *src__coef__arrays,
			      jpeg__transform__info *info)
{
  /* If force-to-grayscale is requested, adjust destination parameters */
  if (info->force__grayscale) {
    /* We use jpeg__set__colorspace to make sure subsidiary settings get fixed
     * properly.  Among other things, the target h__samp__factor & v__samp__factor
     * will get set to 1, which typically won't match the source.
     * In fact we do this even if the source is already grayscale; that
     * provides an easy way of coercing a grayscale JPEG with funny sampling
     * factors to the customary 1,1.  (Some decoders fail on other factors.)
     */
    if ((dstinfo->jpeg__color__space == JCS__YCbCr &&
	 dstinfo->num__components == 3) ||
	(dstinfo->jpeg__color__space == JCS__GRAYSCALE &&
	 dstinfo->num__components == 1)) {
      /* We have to preserve the source's quantization table number. */
      int sv__quant__tbl__no = dstinfo->comp__info[0].quant__tbl__no;
      jpeg__set__colorspace(dstinfo, JCS__GRAYSCALE);
      dstinfo->comp__info[0].quant__tbl__no = sv__quant__tbl__no;
    } else {
      /* Sorry, can't do it */
      ERREXIT(dstinfo, JERR__CONVERSION__NOTIMPL);
    }
  }

  /* Correct the destination's image dimensions etc if necessary */
  switch (info->transform) {
  case JXFORM__NONE:
    /* Nothing to do */
    break;
  case JXFORM__FLIP__H:
    if (info->trim)
      trim__right__edge(dstinfo);
    break;
  case JXFORM__FLIP__V:
    if (info->trim)
      trim__bottom__edge(dstinfo);
    break;
  case JXFORM__TRANSPOSE:
    transpose__critical__parameters(dstinfo);
    /* transpose does NOT have to trim anything */
    break;
  case JXFORM__TRANSVERSE:
    transpose__critical__parameters(dstinfo);
    if (info->trim) {
      trim__right__edge(dstinfo);
      trim__bottom__edge(dstinfo);
    }
    break;
  case JXFORM__ROT__90:
    transpose__critical__parameters(dstinfo);
    if (info->trim)
      trim__right__edge(dstinfo);
    break;
  case JXFORM__ROT__180:
    if (info->trim) {
      trim__right__edge(dstinfo);
      trim__bottom__edge(dstinfo);
    }
    break;
  case JXFORM__ROT__270:
    transpose__critical__parameters(dstinfo);
    if (info->trim)
      trim__bottom__edge(dstinfo);
    break;
  }

  /* Return the appropriate output data set */
  if (info->workspace__coef__arrays != NULL)
    return info->workspace__coef__arrays;
  return src__coef__arrays;
}


/* Execute the actual transformation, if any.
 *
 * This must be called *after* jpeg__write__coefficients, because it depends
 * on jpeg__write__coefficients to have computed subsidiary values such as
 * the per-component width and height fields in the destination object.
 *
 * Note that some transformations will modify the source data arrays!
 */

GLOBAL(void)
jtransform__execute__transformation (j__decompress__ptr srcinfo,
				   j__compress__ptr dstinfo,
				   jvirt__barray__ptr *src__coef__arrays,
				   jpeg__transform__info *info)
{
  jvirt__barray__ptr *dst__coef__arrays = info->workspace__coef__arrays;

  switch (info->transform) {
  case JXFORM__NONE:
    break;
  case JXFORM__FLIP__H:
    do__flip__h(srcinfo, dstinfo, src__coef__arrays);
    break;
  case JXFORM__FLIP__V:
    do__flip__v(srcinfo, dstinfo, src__coef__arrays, dst__coef__arrays);
    break;
  case JXFORM__TRANSPOSE:
    do__transpose(srcinfo, dstinfo, src__coef__arrays, dst__coef__arrays);
    break;
  case JXFORM__TRANSVERSE:
    do__transverse(srcinfo, dstinfo, src__coef__arrays, dst__coef__arrays);
    break;
  case JXFORM__ROT__90:
    do__rot__90(srcinfo, dstinfo, src__coef__arrays, dst__coef__arrays);
    break;
  case JXFORM__ROT__180:
    do__rot__180(srcinfo, dstinfo, src__coef__arrays, dst__coef__arrays);
    break;
  case JXFORM__ROT__270:
    do__rot__270(srcinfo, dstinfo, src__coef__arrays, dst__coef__arrays);
    break;
  }
}

#endif /* TRANSFORMS__SUPPORTED */


/* Setup decompression object to save desired markers in memory.
 * This must be called before jpeg__read__header() to have the desired effect.
 */

GLOBAL(void)
jcopy__markers__setup (j__decompress__ptr srcinfo, JCOPY__OPTION option)
{
#ifdef SAVE__MARKERS__SUPPORTED
  int m;

  /* Save comments except under NONE option */
  if (option != JCOPYOPT__NONE) {
    jpeg__save__markers(srcinfo, JPEG__COM, 0xFFFF);
  }
  /* Save all types of APPn markers iff ALL option */
  if (option == JCOPYOPT__ALL) {
    for (m = 0; m < 16; m++)
      jpeg__save__markers(srcinfo, JPEG__APP0 + m, 0xFFFF);
  }
#endif /* SAVE__MARKERS__SUPPORTED */
}

/* Copy markers saved in the given source object to the destination object.
 * This should be called just after jpeg__start__compress() or
 * jpeg__write__coefficients().
 * Note that those routines will have written the SOI, and also the
 * JFIF APP0 or Adobe APP14 markers if selected.
 */

GLOBAL(void)
jcopy__markers__execute (j__decompress__ptr srcinfo, j__compress__ptr dstinfo,
		       JCOPY__OPTION option)
{
  jpeg__saved__marker__ptr marker;

  /* In the current implementation, we don't actually need to examine the
   * option flag here; we just copy everything that got saved.
   * But to avoid confusion, we do not output JFIF and Adobe APP14 markers
   * if the encoder library already wrote one.
   */
  for (marker = srcinfo->marker__list; marker != NULL; marker = marker->next) {
    if (dstinfo->write__JFIF__header &&
	marker->marker == JPEG__APP0 &&
	marker->data__length >= 5 &&
	GETJOCTET(marker->data[0]) == 0x4A &&
	GETJOCTET(marker->data[1]) == 0x46 &&
	GETJOCTET(marker->data[2]) == 0x49 &&
	GETJOCTET(marker->data[3]) == 0x46 &&
	GETJOCTET(marker->data[4]) == 0)
      continue;			/* reject duplicate JFIF */
    if (dstinfo->write__Adobe__marker &&
	marker->marker == JPEG__APP0+14 &&
	marker->data__length >= 5 &&
	GETJOCTET(marker->data[0]) == 0x41 &&
	GETJOCTET(marker->data[1]) == 0x64 &&
	GETJOCTET(marker->data[2]) == 0x6F &&
	GETJOCTET(marker->data[3]) == 0x62 &&
	GETJOCTET(marker->data[4]) == 0x65)
      continue;			/* reject duplicate Adobe */
#ifdef NEED__FAR__POINTERS
    /* We could use jpeg__write__marker if the data weren't FAR... */
    {
      unsigned int i;
      jpeg__write__m__header(dstinfo, marker->marker, marker->data__length);
      for (i = 0; i < marker->data__length; i++)
	jpeg__write__m__byte(dstinfo, marker->data[i]);
    }
#else
    jpeg__write__marker(dstinfo, marker->marker,
		      marker->data, marker->data__length);
#endif
  }
}
