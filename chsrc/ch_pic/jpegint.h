/*
 * jpegint.h
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides common declarations for the various JPEG modules.
 * These declarations are considered internal to the JPEG library; most
 * applications using the library shouldn't need to include this file.
 */


/* Declarations for both compression & decompression */

typedef enum {			/* Operating modes for buffer controllers */
	JBUF__PASS__THRU,		/* Plain stripwise operation */
	/* Remaining modes require a full-image buffer to have been created */
	JBUF__SAVE__SOURCE,	/* Run source subobject only, save output */
	JBUF__CRANK__DEST,	/* Run dest subobject only, using saved data */
	JBUF__SAVE__AND__PASS	/* Run both subobjects, save output */
} J__BUF__MODE;

/* Values of global__state field (jdapi.c has some dependencies on ordering!) */
#define CSTATE__START	100	/* after create__compress */
#define CSTATE__SCANNING	101	/* start__compress done, write__scanlines OK */
#define CSTATE__RAW__OK	102	/* start__compress done, write__raw__data OK */
#define CSTATE__WRCOEFS	103	/* jpeg__write__coefficients done */
#define DSTATE__START	200	/* after create__decompress */
#define DSTATE__INHEADER	201	/* reading header markers, no SOS yet */
#define DSTATE__READY	202	/* found SOS, ready for start__decompress */
#define DSTATE__PRELOAD	203	/* reading multiscan file in start__decompress*/
#define DSTATE__PRESCAN	204	/* performing dummy pass for 2-pass quant */
#define DSTATE__SCANNING	205	/* start__decompress done, read__scanlines OK */
#define DSTATE__RAW__OK	206	/* start__decompress done, read__raw__data OK */
#define DSTATE__BUFIMAGE	207	/* expecting jpeg__start__output */
#define DSTATE__BUFPOST	208	/* looking for SOS/EOI in jpeg__finish__output */
#define DSTATE__RDCOEFS	209	/* reading file in jpeg__read__coefficients */
#define DSTATE__STOPPING	210	/* looking for EOI in jpeg__finish__decompress */


/* Declarations for compression modules */

/* Master control module */
struct jpeg__comp__master {
  JMETHOD(void, prepare__for__pass, (j__compress__ptr cinfo));
  JMETHOD(void, pass__startup, (j__compress__ptr cinfo));
  JMETHOD(void, finish__pass, (j__compress__ptr cinfo));

  /* State variables made visible to other modules */
  CH_BOOL call__pass__startup;	/* True if pass__startup must be called */
  CH_BOOL is__last__pass;		/* True during last pass */
};

/* Main buffer control (downsampled-data buffer) */
struct jpeg__c__main__controller {
  JMETHOD(void, start__pass, (j__compress__ptr cinfo, J__BUF__MODE pass__mode));
  JMETHOD(void, process__data, (j__compress__ptr cinfo,
			       JSAMPARRAY input__buf, JDIMENSION *in__row__ctr,
			       JDIMENSION in__rows__avail));
};

/* Compression preprocessing (downsampling input buffer control) */
struct jpeg__c__prep__controller {
  JMETHOD(void, start__pass, (j__compress__ptr cinfo, J__BUF__MODE pass__mode));
  JMETHOD(void, pre__process__data, (j__compress__ptr cinfo,
				   JSAMPARRAY input__buf,
				   JDIMENSION *in__row__ctr,
				   JDIMENSION in__rows__avail,
				   JSAMPIMAGE output__buf,
				   JDIMENSION *out__row__group__ctr,
				   JDIMENSION out__row__groups__avail));
};

/* Coefficient buffer control */
struct jpeg__c__coef__controller {
  JMETHOD(void, start__pass, (j__compress__ptr cinfo, J__BUF__MODE pass__mode));
  JMETHOD(CH_BOOL, compress__data, (j__compress__ptr cinfo,
				   JSAMPIMAGE input__buf));
};

/* Colorspace conversion */
struct jpeg__color__converter {
  JMETHOD(void, start__pass, (j__compress__ptr cinfo));
  JMETHOD(void, color__convert, (j__compress__ptr cinfo,
				JSAMPARRAY input__buf, JSAMPIMAGE output__buf,
				JDIMENSION output__row, int num__rows));
};

/* Downsampling */
struct jpeg__downsampler {
  JMETHOD(void, start__pass, (j__compress__ptr cinfo));
  JMETHOD(void, downsample, (j__compress__ptr cinfo,
			     JSAMPIMAGE input__buf, JDIMENSION in__row__index,
			     JSAMPIMAGE output__buf,
			     JDIMENSION out__row__group__index));

  CH_BOOL need__context__rows;	/* TRUE if need rows above & below */
};

/* Forward DCT (also controls coefficient quantization) */
struct jpeg__forward__dct {
  JMETHOD(void, start__pass, (j__compress__ptr cinfo));
  /* perhaps this should be an array??? */
  JMETHOD(void, forward__DCT, (j__compress__ptr cinfo,
			      jpeg__component__info * compptr,
			      JSAMPARRAY sample__data, JBLOCKROW coef__blocks,
			      JDIMENSION start__row, JDIMENSION start__col,
			      JDIMENSION num__blocks));
};

/* Entropy encoding */
struct jpeg__entropy__encoder {
  JMETHOD(void, start__pass, (j__compress__ptr cinfo, CH_BOOL gather__statistics));
  JMETHOD(CH_BOOL, encode__mcu, (j__compress__ptr cinfo, JBLOCKROW *MCU__data));
  JMETHOD(void, finish__pass, (j__compress__ptr cinfo));
};

/* Marker writing */
struct jpeg__marker__writer {
  JMETHOD(void, write__file__header, (j__compress__ptr cinfo));
  JMETHOD(void, write__frame__header, (j__compress__ptr cinfo));
  JMETHOD(void, write__scan__header, (j__compress__ptr cinfo));
  JMETHOD(void, write__file__trailer, (j__compress__ptr cinfo));
  JMETHOD(void, write__tables__only, (j__compress__ptr cinfo));
  /* These routines are exported to allow insertion of extra markers */
  /* Probably only COM and APPn markers should be written this way */
  JMETHOD(void, write__marker__header, (j__compress__ptr cinfo, int marker,
				      unsigned int datalen));
  JMETHOD(void, write__marker__byte, (j__compress__ptr cinfo, int val));
};


/* Declarations for decompression modules */

/* Master control module */
struct jpeg__decomp__master {
  JMETHOD(void, prepare__for__output__pass, (j__decompress__ptr cinfo));
  JMETHOD(void, finish__output__pass, (j__decompress__ptr cinfo));

  /* State variables made visible to other modules */
  CH_BOOL is__dummy__pass;	/* True during 1st pass for 2-pass quant */
};

/* Input control module */
struct jpeg__input__controller {
  JMETHOD(int, consume__input, (j__decompress__ptr cinfo));
  JMETHOD(void, reset__input__controller, (j__decompress__ptr cinfo));
  JMETHOD(void, start__input__pass, (j__decompress__ptr cinfo));
  JMETHOD(void, finish__input__pass, (j__decompress__ptr cinfo));

  /* State variables made visible to other modules */
  CH_BOOL has__multiple__scans;	/* True if file has multiple scans */
  CH_BOOL eoi__reached;		/* True when EOI has been consumed */
};

/* Main buffer control (downsampled-data buffer) */
struct jpeg__d__main__controller {
  JMETHOD(void, start__pass, (j__decompress__ptr cinfo, J__BUF__MODE pass__mode));
  JMETHOD(void, process__data, (j__decompress__ptr cinfo,
			       JSAMPARRAY output__buf, JDIMENSION *out__row__ctr,
			       JDIMENSION out__rows__avail));
};

/* Coefficient buffer control */
struct jpeg__d__coef__controller {
  JMETHOD(void, start__input__pass, (j__decompress__ptr cinfo));
  JMETHOD(int, consume__data, (j__decompress__ptr cinfo));
  JMETHOD(void, start__output__pass, (j__decompress__ptr cinfo));
  JMETHOD(int, decompress__data, (j__decompress__ptr cinfo,
				 JSAMPIMAGE output__buf));
  /* Pointer to array of coefficient virtual arrays, or NULL if none */
  jvirt__barray__ptr *coef__arrays;
};

/* Decompression postprocessing (color quantization buffer control) */
struct jpeg__d__post__controller {
  JMETHOD(void, start__pass, (j__decompress__ptr cinfo, J__BUF__MODE pass__mode));
  JMETHOD(void, post__process__data, (j__decompress__ptr cinfo,
				    JSAMPIMAGE input__buf,
				    JDIMENSION *in__row__group__ctr,
				    JDIMENSION in__row__groups__avail,
				    JSAMPARRAY output__buf,
				    JDIMENSION *out__row__ctr,
				    JDIMENSION out__rows__avail));
};

/* Marker reading & parsing */
struct jpeg__marker__reader {
  JMETHOD(void, reset__marker__reader, (j__decompress__ptr cinfo));
  /* Read markers until SOS or EOI.
   * Returns same codes as are defined for jpeg__consume__input:
   * JPEG__SUSPENDED, JPEG__REACHED__SOS, or JPEG__REACHED__EOI.
   */
  JMETHOD(int, read__markers, (j__decompress__ptr cinfo));
  /* Read a restart marker --- exported for use by entropy decoder only */
  jpeg__marker__parser__method read__restart__marker;

  /* State of marker reader --- nominally internal, but applications
   * supplying COM or APPn handlers might like to know the state.
   */
  CH_BOOL saw__SOI;		/* found SOI? */
  CH_BOOL saw__SOF;		/* found SOF? */
  int next__restart__num;		/* next restart number expected (0-7) */
  unsigned int discarded__bytes;	/* # of bytes skipped looking for a marker */
};

/* Entropy decoding */
struct jpeg__entropy__decoder {
  JMETHOD(void, start__pass, (j__decompress__ptr cinfo));
  JMETHOD(CH_BOOL, decode__mcu, (j__decompress__ptr cinfo,
				JBLOCKROW *MCU__data));

  /* This is here to share code between baseline and progressive decoders; */
  /* other modules probably should not use it */
  CH_BOOL insufficient__data;	/* set TRUE after emitting warning */
};

/* Inverse DCT (also performs dequantization) */
typedef JMETHOD(void, inverse__DCT__method__ptr,
		(j__decompress__ptr cinfo, jpeg__component__info * compptr,
		 JCOEFPTR coef__block,
		 JSAMPARRAY output__buf, JDIMENSION output__col));

struct jpeg__inverse__dct {
  JMETHOD(void, start__pass, (j__decompress__ptr cinfo));
  /* It is useful to allow each component to have a separate IDCT method. */
  inverse__DCT__method__ptr inverse__DCT[MAX__COMPONENTS];
};

/* Upsampling (note that upsampler must also call color converter) */
struct jpeg__upsampler {
  JMETHOD(void, start__pass, (j__decompress__ptr cinfo));
  JMETHOD(void, upsample, (j__decompress__ptr cinfo,
			   JSAMPIMAGE input__buf,
			   JDIMENSION *in__row__group__ctr,
			   JDIMENSION in__row__groups__avail,
			   JSAMPARRAY output__buf,
			   JDIMENSION *out__row__ctr,
			   JDIMENSION out__rows__avail));

  CH_BOOL need__context__rows;	/* TRUE if need rows above & below */
};

/* Colorspace conversion */
struct jpeg__color__deconverter {
  JMETHOD(void, start__pass, (j__decompress__ptr cinfo));
  JMETHOD(void, color__convert, (j__decompress__ptr cinfo,
				JSAMPIMAGE input__buf, JDIMENSION input__row,
				JSAMPARRAY output__buf, int num__rows));
};

/* Color quantization or color precision reduction */
struct jpeg__color__quantizer {
  JMETHOD(void, start__pass, (j__decompress__ptr cinfo, CH_BOOL is__pre__scan));/*ÊéÇ¿*/
  JMETHOD(void, color__quantize, (j__decompress__ptr cinfo,
				 JSAMPARRAY input__buf, JSAMPARRAY output__buf,
				 int num__rows));
  JMETHOD(void, finish__pass, (j__decompress__ptr cinfo));
  JMETHOD(void, new__color__map, (j__decompress__ptr cinfo));
};


/* Miscellaneous useful macros */

#undef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))


/* We assume that right shift corresponds to signed division by 2 with
 * rounding towards minus infinity.  This is correct for typical "arithmetic
 * shift" instructions that shift in copies of the sign bit.  But some
 * C compilers implement >> with an unsigned shift.  For these machines you
 * must define RIGHT__SHIFT__IS__UNSIGNED.
 * RIGHT__SHIFT provides a proper signed right shift of an INT32 quantity.
 * It is only applied with constant shift counts.  SHIFT__TEMPS must be
 * included in the variables of any routine using RIGHT__SHIFT.
 */

#ifdef RIGHT__SHIFT__IS__UNSIGNED
#define SHIFT__TEMPS	INT32 shift__temp;
#define RIGHT__SHIFT(x,shft)  \
	((shift__temp = (x)) < 0 ? \
	 (shift__temp >> (shft)) | ((~((INT32) 0)) << (32-(shft))) : \
	 (shift__temp >> (shft)))
#else
#define SHIFT__TEMPS
/*#define RIGHT__SHIFT(x,shft)	((x) >> (shft))*/
#define RIGHT__SHIFT(x,shft)	zjpg__getshft((x), (shft))
#endif


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED__SHORT__EXTERNAL__NAMES
#define jinit__compress__master	jICompress
#define jinit__c__master__control	jICMaster
#define jinit__c__main__controller	jICMainC
#define jinit__c__prep__controller	jICPrepC
#define jinit__c__coef__controller	jICCoefC
#define jinit__color__converter	jICColor
#define jinit__downsampler	jIDownsampler
#define jinit__forward__dct	jIFDCT
#define jinit__huff__encoder	jIHEncoder
#define jinit__phuff__encoder	jIPHEncoder
#define jinit__marker__writer	jIMWriter
#define jinit__master__decompress	jIDMaster
#define jinit__d__main__controller	jIDMainC
#define jinit__d__coef__controller	jIDCoefC
#define jinit__d__post__controller	jIDPostC
#define jinit__input__controller	jIInCtlr
#define jinit__marker__reader	jIMReader
#define jinit__huff__decoder	jIHDecoder
#define jinit__phuff__decoder	jIPHDecoder
#define jinit__inverse__dct	jIIDCT
#define jinit__upsampler		jIUpsampler
#define jinit__color__deconverter	jIDColor
#define jinit__1pass__quantizer	jI1Quant
#define jinit__2pass__quantizer	jI2Quant
#define jinit__merged__upsampler	jIMUpsampler
#define jinit__memory__mgr	jIMemMgr
#define jdiv__round__up		jDivRound
#define jround__up		jRound
#define jcopy__sample__rows	jCopySamples
#define jcopy__block__row		jCopyBlocks
#define jzero__far		jZeroFar
#define jpeg__zigzag__order	jZIGTable
#define jpeg__natural__order	jZAGTable
#endif /* NEED__SHORT__EXTERNAL__NAMES */


/* Compression module initialization routines */
EXTERN(void) jinit__compress__master JPP((j__compress__ptr cinfo));
EXTERN(void) jinit__c__master__control JPP((j__compress__ptr cinfo,
					 CH_BOOL transcode__only));
EXTERN(void) jinit__c__main__controller JPP((j__compress__ptr cinfo,
					  CH_BOOL need__full__buffer));
EXTERN(void) jinit__c__prep__controller JPP((j__compress__ptr cinfo,
					  CH_BOOL need__full__buffer));
EXTERN(void) jinit__c__coef__controller JPP((j__compress__ptr cinfo,
					  CH_BOOL need__full__buffer));
EXTERN(void) jinit__color__converter JPP((j__compress__ptr cinfo));
EXTERN(void) jinit__downsampler JPP((j__compress__ptr cinfo));
EXTERN(void) jinit__forward__dct JPP((j__compress__ptr cinfo));
EXTERN(void) jinit__huff__encoder JPP((j__compress__ptr cinfo));
EXTERN(void) jinit__phuff__encoder JPP((j__compress__ptr cinfo));
EXTERN(void) jinit__marker__writer JPP((j__compress__ptr cinfo));
/* Decompression module initialization routines */
EXTERN(void) jinit__master__decompress JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__d__main__controller JPP((j__decompress__ptr cinfo,
					  CH_BOOL need__full__buffer));
EXTERN(void) jinit__d__coef__controller JPP((j__decompress__ptr cinfo,
					  CH_BOOL need__full__buffer));
EXTERN(void) jinit__d__post__controller JPP((j__decompress__ptr cinfo,
					  CH_BOOL need__full__buffer));
EXTERN(void) jinit__input__controller JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__marker__reader JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__huff__decoder JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__phuff__decoder JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__inverse__dct JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__upsampler JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__color__deconverter JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__1pass__quantizer JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__2pass__quantizer JPP((j__decompress__ptr cinfo));
EXTERN(void) jinit__merged__upsampler JPP((j__decompress__ptr cinfo));
/* Memory manager initialization */
EXTERN(void) jinit__memory__mgr JPP((j__common__ptr cinfo));

/* Utility routines in jutils.c */
EXTERN(long) jdiv__round__up JPP((long a, long b));
EXTERN(long) jround__up JPP((long a, long b));
EXTERN(void) jcopy__sample__rows JPP((JSAMPARRAY input__array, int source__row,
				    JSAMPARRAY output__array, int dest__row,
				    int num__rows, JDIMENSION num__cols));
EXTERN(void) jcopy__block__row JPP((JBLOCKROW input__row, JBLOCKROW output__row,
				  JDIMENSION num__blocks));
EXTERN(void) jzero__far JPP((void FAR * target, size_t bytestozero));
/* Constant tables in jutils.c */
#if 0				/* This table is not actually needed in v6a */
extern const int jpeg__zigzag__order[]; /* natural coef order to zigzag order */
#endif
extern const int jpeg__natural__order[]; /* zigzag coef order to natural order */

/* Suppress undefined-structure complaints if necessary. */

#ifdef INCOMPLETE__TYPES__BROKEN
#ifndef AM__MEMORY__MANAGER	/* only jmemmgr.c defines these */
struct jvirt__sarray__control { long dummy; };
struct jvirt__barray__control { long dummy; };
#endif
#endif /* INCOMPLETE__TYPES__BROKEN */
