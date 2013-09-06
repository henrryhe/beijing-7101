/*
 * jcsample.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains downsampling routines.
 *
 * Downsampling input data is counted in "row groups".  A row group
 * is defined to be max__v__samp__factor pixel rows of each component,
 * from which the downsampler produces v__samp__factor sample rows.
 * A single row group is processed in each call to the downsampler module.
 *
 * The downsampler is responsible for edge-expansion of its output data
 * to fill an integral number of DCT blocks horizontally.  The source buffer
 * may be modified if it is helpful for this purpose (the source buffer is
 * allocated wide enough to correspond to the desired output width).
 * The caller (the prep controller) is responsible for vertical padding.
 *
 * The downsampler may request "context rows" by setting need__context__rows
 * during startup.  In this case, the input arrays will contain at least
 * one row group's worth of pixels above and below the passed-in data;
 * the caller will create dummy rows at image top and bottom by replicating
 * the first or last real pixel row.
 *
 * An excellent reference for image resampling is
 *   Digital Image Warping, George Wolberg, 1990.
 *   Pub. by IEEE Computer Society Press, Los Alamitos, CA. ISBN 0-8186-8944-7.
 *
 * The downsampling algorithm used here is a simple average of the source
 * pixels covered by the output pixel.  The hi-falutin sampling literature
 * refers to this as a "box filter".  In general the characteristics of a box
 * filter are not very good, but for the specific cases we normally use (1:1
 * and 2:1 ratios) the box is equivalent to a "triangle filter" which is not
 * nearly so bad.  If you intend to use other sampling ratios, you'd be well
 * advised to improve this code.
 *
 * A simple input-smoothing capability is provided.  This is mainly intended
 * for cleaning up color-dithered GIF input files (if you find it inadequate,
 * we suggest using an external filtering program such as pnmconvol).  When
 * enabled, each input pixel P is replaced by a weighted sum of itself and its
 * eight neighbors.  P's weight is 1-8*SF and each neighbor's weight is SF,
 * where SF = (smoothing__factor / 1024).
 * Currently, smoothing is only supported for 2h2v sampling factors.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Pointer to routine to downsample a single component */
typedef JMETHOD(void, downsample1__ptr,
		(j__compress__ptr cinfo, jpeg__component__info * compptr,
		 JSAMPARRAY input__data, JSAMPARRAY output__data));

/* Private subobject */

typedef struct {
  struct jpeg__downsampler pub;	/* public fields */

  /* Downsampling method pointers, one per component */
  downsample1__ptr methods[MAX__COMPONENTS];
} my__downsampler;

typedef my__downsampler * my__downsample__ptr;


/*
 * Initialize for a downsampling pass.
 */

METHODDEF(void)
start__pass__downsample (j__compress__ptr cinfo)
{
  /* no work for now */
}


/*
 * Expand a component horizontally from width input__cols to width output__cols,
 * by duplicating the rightmost samples.
 */

LOCAL(void)
expand__right__edge (JSAMPARRAY image__data, int num__rows,
		   JDIMENSION input__cols, JDIMENSION output__cols)
{
  register JSAMPROW ptr;
  register JSAMPLE pixval;
  register int count;
  int row;
  int numcols = (int) (output__cols - input__cols);

  if (numcols > 0) {
    for (row = 0; row < num__rows; row++) {
      ptr = image__data[row] + input__cols;
      pixval = ptr[-1];		/* don't need GETJSAMPLE() here */
      for (count = numcols; count > 0; count--)
	*ptr++ = pixval;
    }
  }
}


/*
 * Do downsampling for a whole row group (all components).
 *
 * In this version we simply downsample each component independently.
 */

METHODDEF(void)
sep__downsample (j__compress__ptr cinfo,
		JSAMPIMAGE input__buf, JDIMENSION in__row__index,
		JSAMPIMAGE output__buf, JDIMENSION out__row__group__index)
{
  my__downsample__ptr downsample = (my__downsample__ptr) cinfo->downsample;
  int ci;
  jpeg__component__info * compptr;
  JSAMPARRAY in__ptr, out__ptr;

  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    in__ptr = input__buf[ci] + in__row__index;
    out__ptr = output__buf[ci] + (out__row__group__index * compptr->v__samp__factor);
    (*downsample->methods[ci]) (cinfo, compptr, in__ptr, out__ptr);
  }
}


/*
 * Downsample pixel values of a single component.
 * One row group is processed per call.
 * This version handles arbitrary integral sampling ratios, without smoothing.
 * Note that this version is not actually used for customary sampling ratios.
 */

METHODDEF(void)
int__downsample (j__compress__ptr cinfo, jpeg__component__info * compptr,
		JSAMPARRAY input__data, JSAMPARRAY output__data)
{
  int inrow, outrow, h__expand, v__expand, numpix, numpix2, h, v;
  JDIMENSION outcol, outcol__h;	/* outcol__h == outcol*h__expand */
  JDIMENSION output__cols = compptr->width__in__blocks * DCTSIZE;
  JSAMPROW inptr, outptr;
  INT32 outvalue;

  h__expand = cinfo->max__h__samp__factor / compptr->h__samp__factor;
  v__expand = cinfo->max__v__samp__factor / compptr->v__samp__factor;
  numpix = h__expand * v__expand;
  numpix2 = numpix/2;

  /* Expand input data enough to let all the output samples be generated
   * by the standard loop.  Special-casing padded output would be more
   * efficient.
   */
  expand__right__edge(input__data, cinfo->max__v__samp__factor,
		    cinfo->image__width, output__cols * h__expand);

  inrow = 0;
  for (outrow = 0; outrow < compptr->v__samp__factor; outrow++) {
    outptr = output__data[outrow];
    for (outcol = 0, outcol__h = 0; outcol < output__cols;
	 outcol++, outcol__h += h__expand) {
      outvalue = 0;
      for (v = 0; v < v__expand; v++) {
	inptr = input__data[inrow+v] + outcol__h;
	for (h = 0; h < h__expand; h++) {
	  outvalue += (INT32) GETJSAMPLE(*inptr++);
	}
      }
      *outptr++ = (JSAMPLE) ((outvalue + numpix2) / numpix);
    }
    inrow += v__expand;
  }
}


/*
 * Downsample pixel values of a single component.
 * This version handles the special case of a full-size component,
 * without smoothing.
 */

METHODDEF(void)
fullsize__downsample (j__compress__ptr cinfo, jpeg__component__info * compptr,
		     JSAMPARRAY input__data, JSAMPARRAY output__data)
{
  /* Copy the data */
  jcopy__sample__rows(input__data, 0, output__data, 0,
		    cinfo->max__v__samp__factor, cinfo->image__width);
  /* Edge-expand */
  expand__right__edge(output__data, cinfo->max__v__samp__factor,
		    cinfo->image__width, compptr->width__in__blocks * DCTSIZE);
}


/*
 * Downsample pixel values of a single component.
 * This version handles the common case of 2:1 horizontal and 1:1 vertical,
 * without smoothing.
 *
 * A note about the "bias" calculations: when rounding fractional values to
 * integer, we do not want to always round 0.5 up to the next integer.
 * If we did that, we'd introduce a noticeable bias towards larger values.
 * Instead, this code is arranged so that 0.5 will be rounded up or down at
 * alternate pixel locations (a simple ordered dither pattern).
 */

METHODDEF(void)
h2v1__downsample (j__compress__ptr cinfo, jpeg__component__info * compptr,
		 JSAMPARRAY input__data, JSAMPARRAY output__data)
{
  int outrow;
  JDIMENSION outcol;
  JDIMENSION output__cols = compptr->width__in__blocks * DCTSIZE;
  register JSAMPROW inptr, outptr;
  register int bias;

  /* Expand input data enough to let all the output samples be generated
   * by the standard loop.  Special-casing padded output would be more
   * efficient.
   */
  expand__right__edge(input__data, cinfo->max__v__samp__factor,
		    cinfo->image__width, output__cols * 2);

  for (outrow = 0; outrow < compptr->v__samp__factor; outrow++) {
    outptr = output__data[outrow];
    inptr = input__data[outrow];
    bias = 0;			/* bias = 0,1,0,1,... for successive samples */
    for (outcol = 0; outcol < output__cols; outcol++) {
      *outptr++ = (JSAMPLE) ((GETJSAMPLE(*inptr) + GETJSAMPLE(inptr[1])
			      + bias) >> 1);
      bias ^= 1;		/* 0=>1, 1=>0 */
      inptr += 2;
    }
  }
}


/*
 * Downsample pixel values of a single component.
 * This version handles the standard case of 2:1 horizontal and 2:1 vertical,
 * without smoothing.
 */

METHODDEF(void)
h2v2__downsample (j__compress__ptr cinfo, jpeg__component__info * compptr,
		 JSAMPARRAY input__data, JSAMPARRAY output__data)
{
  int inrow, outrow;
  JDIMENSION outcol;
  JDIMENSION output__cols = compptr->width__in__blocks * DCTSIZE;
  register JSAMPROW inptr0, inptr1, outptr;
  register int bias;

  /* Expand input data enough to let all the output samples be generated
   * by the standard loop.  Special-casing padded output would be more
   * efficient.
   */
  expand__right__edge(input__data, cinfo->max__v__samp__factor,
		    cinfo->image__width, output__cols * 2);

  inrow = 0;
  for (outrow = 0; outrow < compptr->v__samp__factor; outrow++) {
    outptr = output__data[outrow];
    inptr0 = input__data[inrow];
    inptr1 = input__data[inrow+1];
    bias = 1;			/* bias = 1,2,1,2,... for successive samples */
    for (outcol = 0; outcol < output__cols; outcol++) {
      *outptr++ = (JSAMPLE) ((GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[1]) +
			      GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[1])
			      + bias) >> 2);
      bias ^= 3;		/* 1=>2, 2=>1 */
      inptr0 += 2; inptr1 += 2;
    }
    inrow += 2;
  }
}


#ifdef INPUT__SMOOTHING__SUPPORTED

/*
 * Downsample pixel values of a single component.
 * This version handles the standard case of 2:1 horizontal and 2:1 vertical,
 * with smoothing.  One row of context is required.
 */

METHODDEF(void)
h2v2__smooth__downsample (j__compress__ptr cinfo, jpeg__component__info * compptr,
			JSAMPARRAY input__data, JSAMPARRAY output__data)
{
  int inrow, outrow;
  JDIMENSION colctr;
  JDIMENSION output__cols = compptr->width__in__blocks * DCTSIZE;
  register JSAMPROW inptr0, inptr1, above__ptr, below__ptr, outptr;
  INT32 membersum, neighsum, memberscale, neighscale;

  /* Expand input data enough to let all the output samples be generated
   * by the standard loop.  Special-casing padded output would be more
   * efficient.
   */
  expand__right__edge(input__data - 1, cinfo->max__v__samp__factor + 2,
		    cinfo->image__width, output__cols * 2);

  /* We don't bother to form the individual "smoothed" input pixel values;
   * we can directly compute the output which is the average of the four
   * smoothed values.  Each of the four member pixels contributes a fraction
   * (1-8*SF) to its own smoothed image and a fraction SF to each of the three
   * other smoothed pixels, therefore a total fraction (1-5*SF)/4 to the final
   * output.  The four corner-adjacent neighbor pixels contribute a fraction
   * SF to just one smoothed pixel, or SF/4 to the final output; while the
   * eight edge-adjacent neighbors contribute SF to each of two smoothed
   * pixels, or SF/2 overall.  In order to use integer arithmetic, these
   * factors are scaled by 2^16 = 65536.
   * Also recall that SF = smoothing__factor / 1024.
   */

  memberscale = 16384 - cinfo->smoothing__factor * 80; /* scaled (1-5*SF)/4 */
  neighscale = cinfo->smoothing__factor * 16; /* scaled SF/4 */

  inrow = 0;
  for (outrow = 0; outrow < compptr->v__samp__factor; outrow++) {
    outptr = output__data[outrow];
    inptr0 = input__data[inrow];
    inptr1 = input__data[inrow+1];
    above__ptr = input__data[inrow-1];
    below__ptr = input__data[inrow+2];

    /* Special case for first column: pretend column -1 is same as column 0 */
    membersum = GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[1]) +
		GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[1]);
    neighsum = GETJSAMPLE(*above__ptr) + GETJSAMPLE(above__ptr[1]) +
	       GETJSAMPLE(*below__ptr) + GETJSAMPLE(below__ptr[1]) +
	       GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[2]) +
	       GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[2]);
    neighsum += neighsum;
    neighsum += GETJSAMPLE(*above__ptr) + GETJSAMPLE(above__ptr[2]) +
		GETJSAMPLE(*below__ptr) + GETJSAMPLE(below__ptr[2]);
    membersum = membersum * memberscale + neighsum * neighscale;
    *outptr++ = (JSAMPLE) ((membersum + 32768) >> 16);
    inptr0 += 2; inptr1 += 2; above__ptr += 2; below__ptr += 2;

    for (colctr = output__cols - 2; colctr > 0; colctr--) {
      /* sum of pixels directly mapped to this output element */
      membersum = GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[1]) +
		  GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[1]);
      /* sum of edge-neighbor pixels */
      neighsum = GETJSAMPLE(*above__ptr) + GETJSAMPLE(above__ptr[1]) +
		 GETJSAMPLE(*below__ptr) + GETJSAMPLE(below__ptr[1]) +
		 GETJSAMPLE(inptr0[-1]) + GETJSAMPLE(inptr0[2]) +
		 GETJSAMPLE(inptr1[-1]) + GETJSAMPLE(inptr1[2]);
      /* The edge-neighbors count twice as much as corner-neighbors */
      neighsum += neighsum;
      /* Add in the corner-neighbors */
      neighsum += GETJSAMPLE(above__ptr[-1]) + GETJSAMPLE(above__ptr[2]) +
		  GETJSAMPLE(below__ptr[-1]) + GETJSAMPLE(below__ptr[2]);
      /* form final output scaled up by 2^16 */
      membersum = membersum * memberscale + neighsum * neighscale;
      /* round, descale and output it */
      *outptr++ = (JSAMPLE) ((membersum + 32768) >> 16);
      inptr0 += 2; inptr1 += 2; above__ptr += 2; below__ptr += 2;
    }

    /* Special case for last column */
    membersum = GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[1]) +
		GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[1]);
    neighsum = GETJSAMPLE(*above__ptr) + GETJSAMPLE(above__ptr[1]) +
	       GETJSAMPLE(*below__ptr) + GETJSAMPLE(below__ptr[1]) +
	       GETJSAMPLE(inptr0[-1]) + GETJSAMPLE(inptr0[1]) +
	       GETJSAMPLE(inptr1[-1]) + GETJSAMPLE(inptr1[1]);
    neighsum += neighsum;
    neighsum += GETJSAMPLE(above__ptr[-1]) + GETJSAMPLE(above__ptr[1]) +
		GETJSAMPLE(below__ptr[-1]) + GETJSAMPLE(below__ptr[1]);
    membersum = membersum * memberscale + neighsum * neighscale;
    *outptr = (JSAMPLE) ((membersum + 32768) >> 16);

    inrow += 2;
  }
}


/*
 * Downsample pixel values of a single component.
 * This version handles the special case of a full-size component,
 * with smoothing.  One row of context is required.
 */

METHODDEF(void)
fullsize__smooth__downsample (j__compress__ptr cinfo, jpeg__component__info *compptr,
			    JSAMPARRAY input__data, JSAMPARRAY output__data)
{
  int outrow;
  JDIMENSION colctr;
  JDIMENSION output__cols = compptr->width__in__blocks * DCTSIZE;
  register JSAMPROW inptr, above__ptr, below__ptr, outptr;
  INT32 membersum, neighsum, memberscale, neighscale;
  int colsum, lastcolsum, nextcolsum;

  /* Expand input data enough to let all the output samples be generated
   * by the standard loop.  Special-casing padded output would be more
   * efficient.
   */
  expand__right__edge(input__data - 1, cinfo->max__v__samp__factor + 2,
		    cinfo->image__width, output__cols);

  /* Each of the eight neighbor pixels contributes a fraction SF to the
   * smoothed pixel, while the main pixel contributes (1-8*SF).  In order
   * to use integer arithmetic, these factors are multiplied by 2^16 = 65536.
   * Also recall that SF = smoothing__factor / 1024.
   */

  memberscale = 65536L - cinfo->smoothing__factor * 512L; /* scaled 1-8*SF */
  neighscale = cinfo->smoothing__factor * 64; /* scaled SF */

  for (outrow = 0; outrow < compptr->v__samp__factor; outrow++) {
    outptr = output__data[outrow];
    inptr = input__data[outrow];
    above__ptr = input__data[outrow-1];
    below__ptr = input__data[outrow+1];

    /* Special case for first column */
    colsum = GETJSAMPLE(*above__ptr++) + GETJSAMPLE(*below__ptr++) +
	     GETJSAMPLE(*inptr);
    membersum = GETJSAMPLE(*inptr++);
    nextcolsum = GETJSAMPLE(*above__ptr) + GETJSAMPLE(*below__ptr) +
		 GETJSAMPLE(*inptr);
    neighsum = colsum + (colsum - membersum) + nextcolsum;
    membersum = membersum * memberscale + neighsum * neighscale;
    *outptr++ = (JSAMPLE) ((membersum + 32768) >> 16);
    lastcolsum = colsum; colsum = nextcolsum;

    for (colctr = output__cols - 2; colctr > 0; colctr--) {
      membersum = GETJSAMPLE(*inptr++);
      above__ptr++; below__ptr++;
      nextcolsum = GETJSAMPLE(*above__ptr) + GETJSAMPLE(*below__ptr) +
		   GETJSAMPLE(*inptr);
      neighsum = lastcolsum + (colsum - membersum) + nextcolsum;
      membersum = membersum * memberscale + neighsum * neighscale;
      *outptr++ = (JSAMPLE) ((membersum + 32768) >> 16);
      lastcolsum = colsum; colsum = nextcolsum;
    }

    /* Special case for last column */
    membersum = GETJSAMPLE(*inptr);
    neighsum = lastcolsum + (colsum - membersum) + colsum;
    membersum = membersum * memberscale + neighsum * neighscale;
    *outptr = (JSAMPLE) ((membersum + 32768) >> 16);

  }
}

#endif /* INPUT__SMOOTHING__SUPPORTED */


/*
 * Module initialization routine for downsampling.
 * Note that we must select a routine for each component.
 */

GLOBAL(void)
jinit__downsampler (j__compress__ptr cinfo)
{
  my__downsample__ptr downsample;
  int ci;
  jpeg__component__info * compptr;
  CH_BOOL smoothok = TRUE;

  downsample = (my__downsample__ptr)
    (*cinfo->mem->alloc__small) ((j__common__ptr) cinfo, JPOOL__IMAGE,
				SIZEOF(my__downsampler));
  cinfo->downsample = (struct jpeg__downsampler *) downsample;
  downsample->pub.start__pass = start__pass__downsample;
  downsample->pub.downsample = sep__downsample;
  downsample->pub.need__context__rows = FALSE;

  if (cinfo->CCIR601__sampling)
    ERREXIT(cinfo, JERR__CCIR601__NOTIMPL);

  /* Verify we can handle the sampling factors, and set up method pointers */
  for (ci = 0, compptr = cinfo->comp__info; ci < cinfo->num__components;
       ci++, compptr++) {
    if (compptr->h__samp__factor == cinfo->max__h__samp__factor &&
	compptr->v__samp__factor == cinfo->max__v__samp__factor) {
#ifdef INPUT__SMOOTHING__SUPPORTED
      if (cinfo->smoothing__factor) {
	downsample->methods[ci] = fullsize__smooth__downsample;
	downsample->pub.need__context__rows = TRUE;
      } else
#endif
	downsample->methods[ci] = fullsize__downsample;
    } else if (compptr->h__samp__factor * 2 == cinfo->max__h__samp__factor &&
	       compptr->v__samp__factor == cinfo->max__v__samp__factor) {
      smoothok = FALSE;
      downsample->methods[ci] = h2v1__downsample;
    } else if (compptr->h__samp__factor * 2 == cinfo->max__h__samp__factor &&
	       compptr->v__samp__factor * 2 == cinfo->max__v__samp__factor) {
#ifdef INPUT__SMOOTHING__SUPPORTED
      if (cinfo->smoothing__factor) {
	downsample->methods[ci] = h2v2__smooth__downsample;
	downsample->pub.need__context__rows = TRUE;
      } else
#endif
	downsample->methods[ci] = h2v2__downsample;
    } else if ((cinfo->max__h__samp__factor % compptr->h__samp__factor) == 0 &&
	       (cinfo->max__v__samp__factor % compptr->v__samp__factor) == 0) {
      smoothok = FALSE;
      downsample->methods[ci] = int__downsample;
    } else
      ERREXIT(cinfo, JERR__FRACT__SAMPLE__NOTIMPL);
  }

#ifdef INPUT__SMOOTHING__SUPPORTED
  if (cinfo->smoothing__factor && !smoothok)
    TRACEMS(cinfo, 0, JTRC__SMOOTH__NOTIMPL);
#endif
}
