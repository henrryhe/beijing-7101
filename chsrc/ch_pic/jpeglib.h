/*
 * jpeglib.h
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file defines the application interface for the JPEG library.
 * Most applications using the library need only include this file,
 * and perhaps jerror.h if they want to know the exact error codes.
 */

#ifndef JPEGLIB__H
#define JPEGLIB__H

/*
 * First we include the configuration files that record how this
 * installation of the JPEG library is set up.  jconfig.h can be
 * generated automatically for many systems.  jmorecfg.h contains
 * manual configuration options that most people need not worry about.
 */

#ifndef JCONFIG__INCLUDED	/* in case jinclude.h already did */
#include "jconfig.h"		/* widely used configuration options */
#endif
#include "jmorecfg.h"		/* seldom changed options */
#include "graphconfig.h"

/* Version ID for the JPEG library.
 * Might be useful for tests like "#if JPEG__LIB__VERSION >= 60".
 */

#define JPEG__LIB__VERSION  62	/* Version 6b */


/* Various constants determining the sizes of things.
 * All of these are specified by the JPEG standard, so don't change them
 * if you want to be compatible.
 */

#define DCTSIZE		    8	/* The basic DCT block is 8x8 samples */
#define DCTSIZE2	    64	/* DCTSIZE squared; # of elements in a block */
#define NUM__QUANT__TBLS      4	/* Quantization tables are numbered 0..3 */
#define NUM__HUFF__TBLS       4	/* Huffman tables are numbered 0..3 */
#define NUM__ARITH__TBLS      16	/* Arith-coding tables are numbered 0..15 */
#define MAX__COMPS__IN__SCAN   4	/* JPEG limit on # of components in one scan */
#define MAX__SAMP__FACTOR     4	/* JPEG limit on sampling factors */
/* Unfortunately, some bozo at Adobe saw no reason to be bound by the standard;
 * the PostScript DCT filter can emit files with many more than 10 blocks/MCU.
 * If you happen to run across such a file, you can up D__MAX__BLOCKS__IN__MCU
 * to handle it.  We even let you do this from the jconfig.h file.  However,
 * we strongly discourage changing C__MAX__BLOCKS__IN__MCU; just because Adobe
 * sometimes emits noncompliant files doesn't mean you should too.
 */
#define C__MAX__BLOCKS__IN__MCU   10 /* compressor's limit on blocks per MCU */
#ifndef D__MAX__BLOCKS__IN__MCU
#define D__MAX__BLOCKS__IN__MCU   10 /* decompressor's limit on blocks per MCU */
#endif


/* Data structures for images (arrays of samples and of DCT coefficients).
 * On 80x86 machines, the image arrays are too big for near pointers,
 * but the pointer arrays can fit in near memory.
 */

typedef JSAMPLE FAR *JSAMPROW;	/* ptr to one image row of pixel samples. */
typedef JSAMPROW *JSAMPARRAY;	/* ptr to some rows (a 2-D sample array) */
typedef JSAMPARRAY *JSAMPIMAGE;	/* a 3-D sample array: top index is color */

typedef JCOEF JBLOCK[DCTSIZE2];	/* one block of coefficients */
typedef JBLOCK FAR *JBLOCKROW;	/* pointer to one row of coefficient blocks */
typedef JBLOCKROW *JBLOCKARRAY;		/* a 2-D array of coefficient blocks */
typedef JBLOCKARRAY *JBLOCKIMAGE;	/* a 3-D array of coefficient blocks */

typedef JCOEF FAR *JCOEFPTR;	/* useful in a couple of places */


/* Types for JPEG compression parameters and working tables. */


/* DCT coefficient quantization tables. */

typedef struct {
  /* This array gives the coefficient quantizers in natural array order
   * (not the zigzag order in which they are stored in a JPEG DQT marker).
   * CAUTION: IJG versions prior to v6a kept this array in zigzag order.
   */
  UINT16 quantval[DCTSIZE2];	/* quantization step for each coefficient */
  /* This field is used only during compression.  It's initialized FALSE when
   * the table is created, and set TRUE when it's been output to the file.
   * You could suppress output of a table by setting this to TRUE.
   * (See jpeg__suppress__tables for an example.)
   */
  CH_BOOL sent__table;		/* TRUE when table has been output */
} JQUANT__TBL;


/* Huffman coding tables. */

typedef struct {
  /* These two fields directly represent the contents of a JPEG DHT marker */
  UINT8 bits[17];		/* bits[k] = # of symbols with codes of */
				/* length k bits; bits[0] is unused */
  UINT8 huffval[256];		/* The symbols, in order of incr code length */
  /* This field is used only during compression.  It's initialized FALSE when
   * the table is created, and set TRUE when it's been output to the file.
   * You could suppress output of a table by setting this to TRUE.
   * (See jpeg__suppress__tables for an example.)
   */
  CH_BOOL sent__table;		/* TRUE when table has been output */
} JHUFF__TBL;


/* Basic info about one component (color channel). */

typedef struct {
  /* These values are fixed over the whole image. */
  /* For compression, they must be supplied by parameter setup; */
  /* for decompression, they are read from the SOF marker. */
  int component__id;		/* identifier for this component (0..255) */
  int component__index;		/* its index in SOF or cinfo->comp__info[] */
  int h__samp__factor;		/* horizontal sampling factor (1..4) */
  int v__samp__factor;		/* vertical sampling factor (1..4) */
  int quant__tbl__no;		/* quantization table selector (0..3) */
  /* These values may vary between scans. */
  /* For compression, they must be supplied by parameter setup; */
  /* for decompression, they are read from the SOS marker. */
  /* The decompressor output side may not use these variables. */
  int dc__tbl__no;		/* DC entropy table selector (0..3) */
  int ac__tbl__no;		/* AC entropy table selector (0..3) */
  
  /* Remaining fields should be treated as private by applications. */
  
  /* These values are computed during compression or decompression startup: */
  /* Component's size in DCT blocks.
   * Any dummy blocks added to complete an MCU are not counted; therefore
   * these values do not depend on whether a scan is interleaved or not.
   */
  JDIMENSION width__in__blocks;
  JDIMENSION height__in__blocks;
  /* Size of a DCT block in samples.  Always DCTSIZE for compression.
   * For decompression this is the size of the output from one DCT block,
   * reflecting any scaling we choose to apply during the IDCT step.
   * Values of 1,2,4,8 are likely to be supported.  Note that different
   * components may receive different IDCT scalings.
   */
  int DCT__scaled__size;
  /* The downsampled dimensions are the component's actual, unpadded number
   * of samples at the main buffer (preprocessing/compression interface), thus
   * downsampled__width = ceil(image__width * Hi/Hmax)
   * and similarly for height.  For decompression, IDCT scaling is included, so
   * downsampled__width = ceil(image__width * Hi/Hmax * DCT__scaled__size/DCTSIZE)
   */
  JDIMENSION downsampled__width;	 /* actual width in samples */
  JDIMENSION downsampled__height; /* actual height in samples */
  /* This flag is used only for decompression.  In cases where some of the
   * components will be ignored (eg grayscale output from YCbCr image),
   * we can skip most computations for the unused components.
   */
  CH_BOOL component__needed;	/* do we need the value of this component? */

  /* These values are computed before starting a scan of the component. */
  /* The decompressor output side may not use these variables. */
  int MCU__width;		/* number of blocks per MCU, horizontally */
  int MCU__height;		/* number of blocks per MCU, vertically */
  int MCU__blocks;		/* MCU__width * MCU__height */
  int MCU__sample__width;		/* MCU width in samples, MCU__width*DCT__scaled__size */
  int last__col__width;		/* # of non-dummy blocks across in last MCU */
  int last__row__height;		/* # of non-dummy blocks down in last MCU */

  /* Saved quantization table for component; NULL if none yet saved.
   * See jdinput.c comments about the need for this information.
   * This field is currently used only for decompression.
   */
  JQUANT__TBL * quant__table;

  /* Private per-component storage for DCT or IDCT subsystem. */
  void * dct__table;
} jpeg__component__info;


/* The script for encoding a multiple-scan file is an array of these: */

typedef struct {
  int comps__in__scan;		/* number of components encoded in this scan */
  int component__index[MAX__COMPS__IN__SCAN]; /* their SOF/comp__info[] indexes */
  int Ss, Se;			/* progressive JPEG spectral selection parms */
  int Ah, Al;			/* progressive JPEG successive approx. parms */
} jpeg__scan__info;

/* The decompressor can save APPn and COM markers in a list of these: */

typedef struct jpeg__marker__struct FAR * jpeg__saved__marker__ptr;

struct jpeg__marker__struct {
  jpeg__saved__marker__ptr next;	/* next in list, or NULL */
  UINT8 marker;			/* marker code: JPEG__COM, or JPEG__APP0+n */
  unsigned int original__length;	/* # bytes of data in the file */
  unsigned int data__length;	/* # bytes of data saved at data[] */
  JOCTET FAR * data;		/* the data contained in the marker */
  /* the marker length word is not counted in data__length or original__length */
};

/* Known color spaces. */

typedef enum {
	JCS__UNKNOWN,		/* error/unspecified */
	JCS__GRAYSCALE,		/* monochrome */
	JCS__RGB,		/* red/green/blue */
	JCS__YCbCr,		/* Y/Cb/Cr (also known as YUV) */
	JCS__CMYK,		/* C/M/Y/K */
	JCS__YCCK		/* Y/Cb/Cr/K */
} J__COLOR__SPACE;

/* DCT/IDCT algorithm options. */

typedef enum {
	JDCT__ISLOW,		/* slow but accurate integer algorithm */
	JDCT__IFAST,		/* faster, less accurate integer method */
	JDCT__FLOAT		/* floating-point: accurate, fast on fast HW */
} J__DCT__METHOD;

#ifndef JDCT__DEFAULT		/* may be overridden in jconfig.h */
#define JDCT__DEFAULT  JDCT__ISLOW
#endif
#ifndef JDCT__FASTEST		/* may be overridden in jconfig.h */
#define JDCT__FASTEST  JDCT__IFAST
#endif

/* Dithering options for decompression. */

typedef enum {
	JDITHER__NONE,		/* no dithering */
	JDITHER__ORDERED,	/* simple ordered dither */
	JDITHER__FS		/* Floyd-Steinberg error diffusion dither */
} J__DITHER__MODE;


/* Common fields between JPEG compression and decompression master structs. */

#define jpeg__common__fields \
  struct jpeg__error__mgr * err;	/* Error handler module */\
  struct jpeg__memory__mgr * mem;	/* Memory manager module */\
  struct jpeg__progress__mgr * progress; /* Progress monitor, or NULL if none */\
  void * client__data;		/* Available for use by application */\
  CH_BOOL is__decompressor;	/* So common code can tell which is which */\
  int global__state		/* For checking call sequence validity */

/* Routines that are to be used by both halves of the library are declared
 * to receive a pointer to this structure.  There are no actual instances of
 * jpeg__common__struct, only of jpeg__compress__struct and jpeg__decompress__struct.
 */
struct jpeg__common__struct {
  jpeg__common__fields;		/* Fields common to both master struct types */
  /* Additional fields follow in an actual jpeg__compress__struct or
   * jpeg__decompress__struct.  All three structs must agree on these
   * initial fields!  (This would be a lot cleaner in C++.)
   */
};

typedef struct jpeg__common__struct * j__common__ptr;
typedef struct jpeg__compress__struct * j__compress__ptr;
typedef struct jpeg__decompress__struct * j__decompress__ptr;


/* Master record for a compression instance */

struct jpeg__compress__struct {
  jpeg__common__fields;		/* Fields shared with jpeg__decompress__struct */

  /* Destination for compressed data */
  struct jpeg__destination__mgr * dest;

  /* Description of source image --- these fields must be filled in by
   * outer application before starting compression.  in__color__space must
   * be correct before you can even call jpeg__set__defaults().
   */

  JDIMENSION image__width;	/* input image width */
  JDIMENSION image__height;	/* input image height */
  int input__components;		/* # of color components in input image */
  J__COLOR__SPACE in__color__space;	/* colorspace of input image */

  double input__gamma;		/* image gamma of input image */

  /* Compression parameters --- these fields must be set before calling
   * jpeg__start__compress().  We recommend calling jpeg__set__defaults() to
   * initialize everything to reasonable defaults, then changing anything
   * the application specifically wants to change.  That way you won't get
   * burnt when new parameters are added.  Also note that there are several
   * helper routines to simplify changing parameters.
   */

  int data__precision;		/* bits of precision in image data */

  int num__components;		/* # of color components in JPEG image */
  J__COLOR__SPACE jpeg__color__space; /* colorspace of JPEG image */

  jpeg__component__info * comp__info;
  /* comp__info[i] describes component that appears i'th in SOF */
  
  JQUANT__TBL * quant__tbl__ptrs[NUM__QUANT__TBLS];
  /* ptrs to coefficient quantization tables, or NULL if not defined */
  
  JHUFF__TBL * dc__huff__tbl__ptrs[NUM__HUFF__TBLS];
  JHUFF__TBL * ac__huff__tbl__ptrs[NUM__HUFF__TBLS];
  /* ptrs to Huffman coding tables, or NULL if not defined */
  
  UINT8 arith__dc__L[NUM__ARITH__TBLS]; /* L values for DC arith-coding tables */
  UINT8 arith__dc__U[NUM__ARITH__TBLS]; /* U values for DC arith-coding tables */
  UINT8 arith__ac__K[NUM__ARITH__TBLS]; /* Kx values for AC arith-coding tables */

  int num__scans;		/* # of entries in scan__info array */
  const jpeg__scan__info * scan__info; /* script for multi-scan file, or NULL */
  /* The default value of scan__info is NULL, which causes a single-scan
   * sequential JPEG file to be emitted.  To create a multi-scan file,
   * set num__scans and scan__info to point to an array of scan definitions.
   */

  CH_BOOL raw__data__in;		/* TRUE=caller supplies downsampled data */
  CH_BOOL arith__code;		/* TRUE=arithmetic coding, FALSE=Huffman */
  CH_BOOL optimize__coding;	/* TRUE=optimize entropy encoding parms */
  CH_BOOL CCIR601__sampling;	/* TRUE=first samples are cosited */
  int smoothing__factor;		/* 1..100, or 0 for no input smoothing */
  J__DCT__METHOD dct__method;	/* DCT algorithm selector */

  /* The restart interval can be specified in absolute MCUs by setting
   * restart__interval, or in MCU rows by setting restart__in__rows
   * (in which case the correct restart__interval will be figured
   * for each scan).
   */
  unsigned int restart__interval; /* MCUs per restart, or 0 for no restart */
  int restart__in__rows;		/* if > 0, MCU rows per restart interval */

  /* Parameters controlling emission of special markers. */

  CH_BOOL write__JFIF__header;	/* should a JFIF marker be written? */
  UINT8 JFIF__major__version;	/* What to write for the JFIF version number */
  UINT8 JFIF__minor__version;
  /* These three values are not used by the JPEG code, merely copied */
  /* into the JFIF APP0 marker.  density__unit can be 0 for unknown, */
  /* 1 for dots/inch, or 2 for dots/cm.  Note that the pixel aspect */
  /* ratio is defined by X__density/Y__density even when density__unit=0. */
  UINT8 density__unit;		/* JFIF code for pixel size units */
  UINT16 X__density;		/* Horizontal pixel density */
  UINT16 Y__density;		/* Vertical pixel density */
  CH_BOOL write__Adobe__marker;	/* should an Adobe marker be written? */
  
  /* State variable: index of next scanline to be written to
   * jpeg__write__scanlines().  Application may use this to control its
   * processing loop, e.g., "while (next__scanline < image__height)".
   */

  JDIMENSION next__scanline;	/* 0 .. image__height-1  */

  /* Remaining fields are known throughout compressor, but generally
   * should not be touched by a surrounding application.
   */

  /*
   * These fields are computed during compression startup
   */
  CH_BOOL progressive__mode;	/* TRUE if scan script uses progressive mode */
  int max__h__samp__factor;	/* largest h__samp__factor */
  int max__v__samp__factor;	/* largest v__samp__factor */

  JDIMENSION total__iMCU__rows;	/* # of iMCU rows to be input to coef ctlr */
  /* The coefficient controller receives data in units of MCU rows as defined
   * for fully interleaved scans (whether the JPEG file is interleaved or not).
   * There are v__samp__factor * DCTSIZE sample rows of each component in an
   * "iMCU" (interleaved MCU) row.
   */
  
  /*
   * These fields are valid during any one scan.
   * They describe the components and MCUs actually appearing in the scan.
   */
  int comps__in__scan;		/* # of JPEG components in this scan */
  jpeg__component__info * cur__comp__info[MAX__COMPS__IN__SCAN];
  /* *cur__comp__info[i] describes component that appears i'th in SOS */
  
  JDIMENSION MCUs__per__row;	/* # of MCUs across the image */
  JDIMENSION MCU__rows__in__scan;	/* # of MCU rows in the image */
  
  int blocks__in__MCU;		/* # of DCT blocks per MCU */
  int MCU__membership[C__MAX__BLOCKS__IN__MCU];
  /* MCU__membership[i] is index in cur__comp__info of component owning */
  /* i'th block in an MCU */

  int Ss, Se, Ah, Al;		/* progressive JPEG parameters for scan */

  /*
   * Links to compression subobjects (methods and private variables of modules)
   */
  struct jpeg__comp__master * master;
  struct jpeg__c__main__controller * main;
  struct jpeg__c__prep__controller * prep;
  struct jpeg__c__coef__controller * coef;
  struct jpeg__marker__writer * marker;
  struct jpeg__color__converter * cconvert;
  struct jpeg__downsampler * downsample;
  struct jpeg__forward__dct * fdct;
  struct jpeg__entropy__encoder * entropy;
  jpeg__scan__info * script__space; /* workspace for jpeg__simple__progression */
  int script__space__size;
};


/* Master record for a decompression instance */

struct jpeg__decompress__struct {
  jpeg__common__fields;		/* Fields shared with jpeg__compress__struct */

  /* Source of compressed data */
  struct jpeg__source__mgr * src;

  /* Basic description of image --- filled in by jpeg__read__header(). */
  /* Application may inspect these values to decide how to process image. */

  JDIMENSION image__width;	/* nominal image width (from SOF marker) */
  JDIMENSION image__height;	/* nominal image height */
  int num__components;		/* # of color components in JPEG image */
  J__COLOR__SPACE jpeg__color__space; /* colorspace of JPEG image */

  /* Decompression processing parameters --- these fields must be set before
   * calling jpeg__start__decompress().  Note that jpeg__read__header() initializes
   * them to default values.
   */

  J__COLOR__SPACE out__color__space; /* colorspace for output */

  unsigned int scale__num, scale__denom; /* fraction by which to scale image */

  double output__gamma;		/* image gamma wanted in output */

  CH_BOOL buffered__image;	/* TRUE=multiple output passes */
  CH_BOOL raw__data__out;		/* TRUE=downsampled data wanted */

  J__DCT__METHOD dct__method;	/* IDCT algorithm selector */
  CH_BOOL do__fancy__upsampling;	/* TRUE=apply fancy upsampling */
  CH_BOOL do__block__smoothing;	/* TRUE=apply interblock smoothing */

  CH_BOOL quantize__colors;	/* TRUE=colormapped output wanted */
  /* the following are ignored if not quantize__colors: */
  J__DITHER__MODE dither__mode;	/* type of color dithering to use */
  CH_BOOL two__pass__quantize;	/* TRUE=use two-pass color quantization */
  int desired__number__of__colors;	/* max # colors to use in created colormap */
  /* these are significant only in buffered-image mode: */
  CH_BOOL enable__1pass__quant;	/* enable future use of 1-pass quantizer */
  CH_BOOL enable__external__quant;/* enable future use of external colormap */
  CH_BOOL enable__2pass__quant;	/* enable future use of 2-pass quantizer */

  /* Description of actual output image that will be returned to application.
   * These fields are computed by jpeg__start__decompress().
   * You can also use jpeg__calc__output__dimensions() to determine these values
   * in advance of calling jpeg__start__decompress().
   */

  JDIMENSION output__width;	/* scaled image width */
  JDIMENSION output__height;	/* scaled image height */
  int out__color__components;	/* # of color components in out__color__space */
  int output__components;	/* # of color components returned */
  /* output__components is 1 (a colormap index) when quantizing colors;
   * otherwise it equals out__color__components.
   */
  int rec__outbuf__height;	/* min recommended height of scanline buffer */
  /* If the buffer passed to jpeg__read__scanlines() is less than this many rows
   * high, space and time will be wasted due to unnecessary data copying.
   * Usually rec__outbuf__height will be 1 or 2, at most 4.
   */

  /* When quantizing colors, the output colormap is described by these fields.
   * The application can supply a colormap by setting colormap non-NULL before
   * calling jpeg__start__decompress; otherwise a colormap is created during
   * jpeg__start__decompress or jpeg__start__output.
   * The map has out__color__components rows and actual__number__of__colors columns.
   */
  int actual__number__of__colors;	/* number of entries in use */
  JSAMPARRAY colormap;		/* The color map as a 2-D pixel array */

  /* State variables: these variables indicate the progress of decompression.
   * The application may examine these but must not modify them.
   */

  /* Row index of next scanline to be read from jpeg__read__scanlines().
   * Application may use this to control its processing loop, e.g.,
   * "while (output__scanline < output__height)".
   */
  JDIMENSION output__scanline;	/* 0 .. output__height-1  */

  /* Current input scan number and number of iMCU rows completed in scan.
   * These indicate the progress of the decompressor input side.
   */
  int input__scan__number;	/* Number of SOS markers seen so far */
  JDIMENSION input__iMCU__row;	/* Number of iMCU rows completed */

  /* The "output scan number" is the notional scan being displayed by the
   * output side.  The decompressor will not allow output scan/row number
   * to get ahead of input scan/row, but it can fall arbitrarily far behind.
   */
  int output__scan__number;	/* Nominal scan number being displayed */
  JDIMENSION output__iMCU__row;	/* Number of iMCU rows read */

  /* Current progression status.  coef__bits[c][i] indicates the precision
   * with which component c's DCT coefficient i (in zigzag order) is known.
   * It is -1 when no data has yet been received, otherwise it is the point
   * transform (shift) value for the most recent scan of the coefficient
   * (thus, 0 at completion of the progression).
   * This pointer is NULL when reading a non-progressive file.
   */
  int (*coef__bits)[DCTSIZE2];	/* -1 or current Al value for each coef */

  /* Internal JPEG parameters --- the application usually need not look at
   * these fields.  Note that the decompressor output side may not use
   * any parameters that can change between scans.
   */

  /* Quantization and Huffman tables are carried forward across input
   * datastreams when processing abbreviated JPEG datastreams.
   */

  JQUANT__TBL * quant__tbl__ptrs[NUM__QUANT__TBLS];
  /* ptrs to coefficient quantization tables, or NULL if not defined */

  JHUFF__TBL * dc__huff__tbl__ptrs[NUM__HUFF__TBLS];
  JHUFF__TBL * ac__huff__tbl__ptrs[NUM__HUFF__TBLS];
  /* ptrs to Huffman coding tables, or NULL if not defined */

  /* These parameters are never carried across datastreams, since they
   * are given in SOF/SOS markers or defined to be reset by SOI.
   */

  int data__precision;		/* bits of precision in image data */

  jpeg__component__info * comp__info;
  /* comp__info[i] describes component that appears i'th in SOF */

  CH_BOOL progressive__mode;	/* TRUE if SOFn specifies progressive mode */
  CH_BOOL arith__code;		/* TRUE=arithmetic coding, FALSE=Huffman */

  UINT8 arith__dc__L[NUM__ARITH__TBLS]; /* L values for DC arith-coding tables */
  UINT8 arith__dc__U[NUM__ARITH__TBLS]; /* U values for DC arith-coding tables */
  UINT8 arith__ac__K[NUM__ARITH__TBLS]; /* Kx values for AC arith-coding tables */

  unsigned int restart__interval; /* MCUs per restart interval, or 0 for no restart */

  /* These fields record data obtained from optional markers recognized by
   * the JPEG library.
   */
  CH_BOOL saw__JFIF__marker;	/* TRUE iff a JFIF APP0 marker was found */
  /* Data copied from JFIF marker; only valid if saw__JFIF__marker is TRUE: */
  UINT8 JFIF__major__version;	/* JFIF version number */
  UINT8 JFIF__minor__version;
  UINT8 density__unit;		/* JFIF code for pixel size units */
  UINT16 X__density;		/* Horizontal pixel density */
  UINT16 Y__density;		/* Vertical pixel density */
  CH_BOOL saw__Adobe__marker;	/* TRUE iff an Adobe APP14 marker was found */
  UINT8 Adobe__transform;	/* Color transform code from Adobe marker */

  CH_BOOL CCIR601__sampling;	/* TRUE=first samples are cosited */

  /* Aside from the specific data retained from APPn markers known to the
   * library, the uninterpreted contents of any or all APPn and COM markers
   * can be saved in a list for examination by the application.
   */
  jpeg__saved__marker__ptr marker__list; /* Head of list of saved markers */

  /* Remaining fields are known throughout decompressor, but generally
   * should not be touched by a surrounding application.
   */

  /*
   * These fields are computed during decompression startup
   */
  int max__h__samp__factor;	/* largest h__samp__factor */
  int max__v__samp__factor;	/* largest v__samp__factor */

  int min__DCT__scaled__size;	/* smallest DCT__scaled__size of any component */

  JDIMENSION total__iMCU__rows;	/* # of iMCU rows in image */
  /* The coefficient controller's input and output progress is measured in
   * units of "iMCU" (interleaved MCU) rows.  These are the same as MCU rows
   * in fully interleaved JPEG scans, but are used whether the scan is
   * interleaved or not.  We define an iMCU row as v__samp__factor DCT block
   * rows of each component.  Therefore, the IDCT output contains
   * v__samp__factor*DCT__scaled__size sample rows of a component per iMCU row.
   */

  JSAMPLE * sample__range__limit; /* table for fast range-limiting */

  /*
   * These fields are valid during any one scan.
   * They describe the components and MCUs actually appearing in the scan.
   * Note that the decompressor output side must not use these fields.
   */
  int comps__in__scan;		/* # of JPEG components in this scan */
  jpeg__component__info * cur__comp__info[MAX__COMPS__IN__SCAN];
  /* *cur__comp__info[i] describes component that appears i'th in SOS */

  JDIMENSION MCUs__per__row;	/* # of MCUs across the image */
  JDIMENSION MCU__rows__in__scan;	/* # of MCU rows in the image */

  int blocks__in__MCU;		/* # of DCT blocks per MCU */
  int MCU__membership[D__MAX__BLOCKS__IN__MCU];
  /* MCU__membership[i] is index in cur__comp__info of component owning */
  /* i'th block in an MCU */

  int Ss, Se, Ah, Al;		/* progressive JPEG parameters for scan */

  /* This field is shared between entropy decoder and marker parser.
   * It is either zero or the code of a JPEG marker that has been
   * read from the data source, but has not yet been processed.
   */
  int unread__marker;

  /*
   * Links to decompression subobjects (methods, private variables of modules)
   */
  struct jpeg__decomp__master * master;
  struct jpeg__d__main__controller * main;
  struct jpeg__d__coef__controller * coef;
  struct jpeg__d__post__controller * post;
  struct jpeg__input__controller * inputctl;
  struct jpeg__marker__reader * marker;
  struct jpeg__entropy__decoder * entropy;
  struct jpeg__inverse__dct * idct;
  struct jpeg__upsampler * upsample;
  struct jpeg__color__deconverter * cconvert;
  struct jpeg__color__quantizer * cquantize;
};


/* "Object" declarations for JPEG modules that may be supplied or called
 * directly by the surrounding application.
 * As with all objects in the JPEG library, these structs only define the
 * publicly visible methods and state variables of a module.  Additional
 * private fields may exist after the public ones.
 */


/* Error handler object */

struct jpeg__error__mgr {
  /* Error exit handler: does not return to caller */
  JMETHOD(void, error__exit, (j__common__ptr cinfo));
  /* Conditionally emit a trace or warning message */
  JMETHOD(void, emit__message, (j__common__ptr cinfo, int msg__level));
  /* Routine that actually outputs a trace or error message */
  JMETHOD(void, output__message, (j__common__ptr cinfo));
  /* Format a message string for the most recent JPEG error or message */
  JMETHOD(void, format__message, (j__common__ptr cinfo, char * buffer));
#define JMSG__LENGTH__MAX  200	/* recommended size of format__message buffer */
  /* Reset error state variables at start of a new image */
  JMETHOD(void, reset__error__mgr, (j__common__ptr cinfo));
  
  /* The message ID code and any parameters are saved here.
   * A message can have one string parameter or up to 8 int parameters.
   */
  int msg__code;
#define JMSG__STR__PARM__MAX  80
  union {
    int i[8];
    char s[JMSG__STR__PARM__MAX];
  } msg__parm;
  
  /* Standard state variables for error facility */
  
  int trace__level;		/* max msg__level that will be displayed */
  
  /* For recoverable corrupt-data errors, we emit a warning message,
   * but keep going unless emit__message chooses to abort.  emit__message
   * should count warnings in num__warnings.  The surrounding application
   * can check for bad data by seeing if num__warnings is nonzero at the
   * end of processing.
   */
  long num__warnings;		/* number of corrupt-data warnings */

  /* These fields point to the table(s) of error message strings.
   * An application can change the table pointer to switch to a different
   * message list (typically, to change the language in which errors are
   * reported).  Some applications may wish to add additional error codes
   * that will be handled by the JPEG library error mechanism; the second
   * table pointer is used for this purpose.
   *
   * First table includes all errors generated by JPEG library itself.
   * Error code 0 is reserved for a "no such error string" message.
   */
  const char * const * jpeg__message__table; /* Library errors */
  int last__jpeg__message;    /* Table contains strings 0..last__jpeg__message */
  /* Second table can be added by application (see cjpeg/djpeg for example).
   * It contains strings numbered first__addon__message..last__addon__message.
   */
  const char * const * addon__message__table; /* Non-library errors */
  int first__addon__message;	/* code for first string in addon table */
  int last__addon__message;	/* code for last string in addon table */
};


/* Progress monitor object */

struct jpeg__progress__mgr {
  JMETHOD(void, progress__monitor, (j__common__ptr cinfo));

  long pass__counter;		/* work units completed in this pass */
  long pass__limit;		/* total number of work units in this pass */
  int completed__passes;		/* passes completed so far */
  int total__passes;		/* total number of passes expected */
};


/* Data destination object for compression */

struct jpeg__destination__mgr {
  JOCTET * next__output__byte;	/* => next byte to write in buffer */
  size_t free__in__buffer;	/* # of byte spaces remaining in buffer */

  JMETHOD(void, init__destination, (j__compress__ptr cinfo));
  JMETHOD(CH_BOOL, empty__output__buffer, (j__compress__ptr cinfo));
  JMETHOD(void, term__destination, (j__compress__ptr cinfo));
};


/* Data source object for decompression */

struct jpeg__source__mgr {
  const JOCTET * next__input__byte; /* => next byte to read from buffer */
  size_t bytes__in__buffer;	/* # of bytes remaining in buffer */

  JMETHOD(void, init__source, (j__decompress__ptr cinfo));
  JMETHOD(CH_BOOL, fill__input__buffer, (j__decompress__ptr cinfo));
  JMETHOD(void, skip__input__data, (j__decompress__ptr cinfo, long num__bytes));
  JMETHOD(CH_BOOL, resync__to__restart, (j__decompress__ptr cinfo, int desired));
  JMETHOD(void, term__source, (j__decompress__ptr cinfo));
};


/* Memory manager object.
 * Allocates "small" objects (a few K total), "large" objects (tens of K),
 * and "really big" objects (virtual arrays with backing store if needed).
 * The memory manager does not allow individual objects to be freed; rather,
 * each created object is assigned to a pool, and whole pools can be freed
 * at once.  This is faster and more convenient than remembering exactly what
 * to free, especially where malloc()/free() are not too speedy.
 * NB: alloc routines never return NULL.  They exit to error__exit if not
 * successful.
 */

#define JPOOL__PERMANENT	0	/* lasts until master record is destroyed */
#define JPOOL__IMAGE	1	/* lasts until done with image/datastream */
#define JPOOL__NUMPOOLS	2

typedef struct jvirt__sarray__control * jvirt__sarray__ptr;
typedef struct jvirt__barray__control * jvirt__barray__ptr;


struct jpeg__memory__mgr {
  /* Method pointers */
  JMETHOD(void *, alloc__small, (j__common__ptr cinfo, int pool__id,
				size_t sizeofobject));
  JMETHOD(void FAR *, alloc__large, (j__common__ptr cinfo, int pool__id,
				     size_t sizeofobject));
  JMETHOD(JSAMPARRAY, alloc__sarray, (j__common__ptr cinfo, int pool__id,
				     JDIMENSION samplesperrow,
				     JDIMENSION numrows));
  JMETHOD(JBLOCKARRAY, alloc__barray, (j__common__ptr cinfo, int pool__id,
				      JDIMENSION blocksperrow,
				      JDIMENSION numrows));
  JMETHOD(jvirt__sarray__ptr, request__virt__sarray, (j__common__ptr cinfo,
						  int pool__id,
						  CH_BOOL pre__zero,
						  JDIMENSION samplesperrow,
						  JDIMENSION numrows,
						  JDIMENSION maxaccess));
  JMETHOD(jvirt__barray__ptr, request__virt__barray, (j__common__ptr cinfo,
						  int pool__id,
						  CH_BOOL pre__zero,
						  JDIMENSION blocksperrow,
						  JDIMENSION numrows,
						  JDIMENSION maxaccess));
  JMETHOD(void, realize__virt__arrays, (j__common__ptr cinfo));
  JMETHOD(JSAMPARRAY, access__virt__sarray, (j__common__ptr cinfo,
					   jvirt__sarray__ptr ptr,
					   JDIMENSION start__row,
					   JDIMENSION num__rows,
					   CH_BOOL writable));
  JMETHOD(JBLOCKARRAY, access__virt__barray, (j__common__ptr cinfo,
					    jvirt__barray__ptr ptr,
					    JDIMENSION start__row,
					    JDIMENSION num__rows,
					    CH_BOOL writable));
  JMETHOD(void, free__pool, (j__common__ptr cinfo, int pool__id));
  JMETHOD(void, self__destruct, (j__common__ptr cinfo));

  /* Limit on memory allocation for this JPEG object.  (Note that this is
   * merely advisory, not a guaranteed maximum; it only affects the space
   * used for virtual-array buffers.)  May be changed by outer application
   * after creating the JPEG object.
   */
  long max__memory__to__use;

  /* Maximum allocation request accepted by alloc__large. */
  long max__alloc__chunk;
};


/* Routine signature for application-supplied marker processing methods.
 * Need not pass marker code since it is stored in cinfo->unread__marker.
 */
typedef JMETHOD(CH_BOOL, jpeg__marker__parser__method, (j__decompress__ptr cinfo));


/* Declarations for routines called by application.
 * The JPP macro hides prototype parameters from compilers that can't cope.
 * Note JPP requires double parentheses.
 */

#ifdef HAVE__PROTOTYPES
#define JPP(arglist)	arglist
#else
#define JPP(arglist)	()
#endif


/* Short forms of external names for systems with brain-damaged linkers.
 * We shorten external names to be unique in the first six letters, which
 * is good enough for all known systems.
 * (If your compiler itself needs names to be unique in less than 15 
 * characters, you are out of luck.  Get a better compiler.)
 */

#ifdef NEED__SHORT__EXTERNAL__NAMES
#define jpeg__std__error		jStdError
#define jpeg__CreateCompress	jCreaCompress
#define jpeg__CreateDecompress	jCreaDecompress
#define jpeg__destroy__compress	jDestCompress
#define jpeg__destroy__decompress	jDestDecompress
#define jpeg__stdio__dest		jStdDest
#define jpeg__stdio__src		jStdSrc
#define jpeg__set__defaults	jSetDefaults
#define jpeg__set__colorspace	jSetColorspace
#define jpeg__default__colorspace	jDefColorspace
#define jpeg__set__quality	jSetQuality
#define jpeg__set__linear__quality	jSetLQuality
#define jpeg__add__quant__table	jAddQuantTable
#define jpeg__quality__scaling	jQualityScaling
#define jpeg__simple__progression	jSimProgress
#define jpeg__suppress__tables	jSuppressTables
#define jpeg__alloc__quant__table	jAlcQTable
#define jpeg__alloc__huff__table	jAlcHTable
#define jpeg__start__compress	jStrtCompress
#define jpeg__write__scanlines	jWrtScanlines
#define jpeg__finish__compress	jFinCompress
#define jpeg__write__raw__data	jWrtRawData
#define jpeg__write__marker	jWrtMarker
#define jpeg__write__m__header	jWrtMHeader
#define jpeg__write__m__byte	jWrtMByte
#define jpeg__write__tables	jWrtTables
#define jpeg__read__header	jReadHeader
#define jpeg__start__decompress	jStrtDecompress
#define jpeg__read__scanlines	jReadScanlines
#define jpeg__finish__decompress	jFinDecompress
#define jpeg__read__raw__data	jReadRawData
#define jpeg__has__multiple__scans	jHasMultScn
#define jpeg__start__output	jStrtOutput
#define jpeg__finish__output	jFinOutput
#define jpeg__input__complete	jInComplete
#define jpeg__new__colormap	jNewCMap
#define jpeg__consume__input	jConsumeInput
#define jpeg__calc__output__dimensions	jCalcDimensions
#define jpeg__save__markers	jSaveMarkers
#define jpeg__set__marker__processor	jSetMarker
#define jpeg__read__coefficients	jReadCoefs
#define jpeg__write__coefficients	jWrtCoefs
#define jpeg__copy__critical__parameters	jCopyCrit
#define jpeg__abort__compress	jAbrtCompress
#define jpeg__abort__decompress	jAbrtDecompress
#define jpeg__abort		jAbort
#define jpeg__destroy		jDestroy
#define jpeg__resync__to__restart	jResyncRestart
#endif /* NEED__SHORT__EXTERNAL__NAMES */


/* Default error-management setup */
EXTERN(struct jpeg__error__mgr *) jpeg__std__error
	JPP((struct jpeg__error__mgr * err));

/* Initialization of JPEG compression objects.
 * jpeg__create__compress() and jpeg__create__decompress() are the exported
 * names that applications should call.  These expand to calls on
 * jpeg__CreateCompress and jpeg__CreateDecompress with additional information
 * passed for version mismatch checking.
 * NB: you must set up the error-manager BEFORE calling jpeg__create__xxx.
 */
#define jpeg__create__compress(cinfo) \
    jpeg__CreateCompress((cinfo), JPEG__LIB__VERSION, \
			(size_t) sizeof(struct jpeg__compress__struct))
#define jpeg__create__decompress(cinfo) \
    jpeg__CreateDecompress((cinfo), JPEG__LIB__VERSION, \
			  (size_t) sizeof(struct jpeg__decompress__struct))
EXTERN(void) jpeg__CreateCompress JPP((j__compress__ptr cinfo,
				      int version, size_t structsize));
EXTERN(void) jpeg__CreateDecompress JPP((j__decompress__ptr cinfo,
					int version, size_t structsize));
/* Destruction of JPEG compression objects */
EXTERN(void) jpeg__destroy__compress JPP((j__compress__ptr cinfo));
EXTERN(void) jpeg__destroy__decompress JPP((j__decompress__ptr cinfo));

/* Standard data source and destination managers: stdio streams. */
/* Caller is responsible for opening the file before and closing after. */
EXTERN(void) jpeg__stdio__dest JPP((j__compress__ptr cinfo, FILE * outfile));
EXTERN(void) jpeg__stdio__src JPP((j__decompress__ptr cinfo, FILE * infile));

/* Default parameter setup for compression */
EXTERN(void) jpeg__set__defaults JPP((j__compress__ptr cinfo));
/* Compression parameter setup aids */
EXTERN(void) jpeg__set__colorspace JPP((j__compress__ptr cinfo,
				      J__COLOR__SPACE colorspace));
EXTERN(void) jpeg__default__colorspace JPP((j__compress__ptr cinfo));
EXTERN(void) jpeg__set__quality JPP((j__compress__ptr cinfo, int quality,
				   CH_BOOL force__baseline));
EXTERN(void) jpeg__set__linear__quality JPP((j__compress__ptr cinfo,
					  int scale__factor,
					  CH_BOOL force__baseline));
EXTERN(void) jpeg__add__quant__table JPP((j__compress__ptr cinfo, int whiCH_tbl,
				       const unsigned int *basic__table,
				       int scale__factor,
				       CH_BOOL force__baseline));
EXTERN(int) jpeg__quality__scaling JPP((int quality));
EXTERN(void) jpeg__simple__progression JPP((j__compress__ptr cinfo));
EXTERN(void) jpeg__suppress__tables JPP((j__compress__ptr cinfo,
				       CH_BOOL suppress));
EXTERN(JQUANT__TBL *) jpeg__alloc__quant__table JPP((j__common__ptr cinfo));
EXTERN(JHUFF__TBL *) jpeg__alloc__huff__table JPP((j__common__ptr cinfo));

/* Main entry points for compression */
EXTERN(void) jpeg__start__compress JPP((j__compress__ptr cinfo,
				      CH_BOOL write__all__tables));
EXTERN(JDIMENSION) jpeg__write__scanlines JPP((j__compress__ptr cinfo,
					     JSAMPARRAY scanlines,
					     JDIMENSION num__lines));
EXTERN(void) jpeg__finish__compress JPP((j__compress__ptr cinfo));

/* Replaces jpeg__write__scanlines when writing raw downsampled data. */
EXTERN(JDIMENSION) jpeg__write__raw__data JPP((j__compress__ptr cinfo,
					    JSAMPIMAGE data,
					    JDIMENSION num__lines));

/* Write a special marker.  See libjpeg.doc concerning safe usage. */
EXTERN(void) jpeg__write__marker
	JPP((j__compress__ptr cinfo, int marker,
	     const JOCTET * dataptr, unsigned int datalen));
/* Same, but piecemeal. */
EXTERN(void) jpeg__write__m__header
	JPP((j__compress__ptr cinfo, int marker, unsigned int datalen));
EXTERN(void) jpeg__write__m__byte
	JPP((j__compress__ptr cinfo, int val));

/* Alternate compression function: just write an abbreviated table file */
EXTERN(void) jpeg__write__tables JPP((j__compress__ptr cinfo));

/* Decompression startup: read start of JPEG datastream to see what's there */
EXTERN(int) jpeg__read__header JPP((j__decompress__ptr cinfo,
				  CH_BOOL require__image));
/* Return value is one of: */
#define JPEG__SUSPENDED		0 /* Suspended due to lack of input data */
#define JPEG__HEADER__OK		1 /* Found valid image datastream */
#define JPEG__HEADER__TABLES__ONLY	2 /* Found valid table-specs-only datastream */
/* If you pass require__image = TRUE (normal case), you need not check for
 * a TABLES__ONLY return code; an abbreviated file will cause an error exit.
 * JPEG__SUSPENDED is only possible if you use a data source module that can
 * give a suspension return (the stdio source module doesn't).
 */

/* Main entry points for decompression */
EXTERN(CH_BOOL) jpeg__start__decompress JPP((j__decompress__ptr cinfo));
EXTERN(JDIMENSION) jpeg__read__scanlines JPP((j__decompress__ptr cinfo,
					    JSAMPARRAY scanlines,
					    JDIMENSION max__lines));
EXTERN(CH_BOOL) jpeg__finish__decompress JPP((j__decompress__ptr cinfo));

/* Replaces jpeg__read__scanlines when reading raw downsampled data. */
EXTERN(JDIMENSION) jpeg__read__raw__data JPP((j__decompress__ptr cinfo,
					   JSAMPIMAGE data,
					   JDIMENSION max__lines));

/* Additional entry points for buffered-image mode. */
EXTERN(CH_BOOL) jpeg__has__multiple__scans JPP((j__decompress__ptr cinfo));
EXTERN(CH_BOOL) jpeg__start__output JPP((j__decompress__ptr cinfo,
				       int scan__number));
EXTERN(CH_BOOL) jpeg__finish__output JPP((j__decompress__ptr cinfo));
EXTERN(CH_BOOL) jpeg__input__complete JPP((j__decompress__ptr cinfo));
EXTERN(void) jpeg__new__colormap JPP((j__decompress__ptr cinfo));
EXTERN(int) jpeg__consume__input JPP((j__decompress__ptr cinfo));
/* Return value is one of: */
/* #define JPEG__SUSPENDED	0    Suspended due to lack of input data */
#define JPEG__REACHED__SOS	1 /* Reached start of new scan */
#define JPEG__REACHED__EOI	2 /* Reached end of image */
#define JPEG__ROW__COMPLETED	3 /* Completed one iMCU row */
#define JPEG__SCAN__COMPLETED	4 /* Completed last iMCU row of a scan */

/* Precalculate output dimensions for current decompression parameters. */
EXTERN(void) jpeg__calc__output__dimensions JPP((j__decompress__ptr cinfo));

/* Control saving of COM and APPn markers into marker__list. */
EXTERN(void) jpeg__save__markers
	JPP((j__decompress__ptr cinfo, int marker__code,
	     unsigned int length__limit));

/* Install a special processing method for COM or APPn markers. */
EXTERN(void) jpeg__set__marker__processor
	JPP((j__decompress__ptr cinfo, int marker__code,
	     jpeg__marker__parser__method routine));

/* Read or write raw DCT coefficients --- useful for lossless transcoding. */
EXTERN(jvirt__barray__ptr *) jpeg__read__coefficients JPP((j__decompress__ptr cinfo));
EXTERN(void) jpeg__write__coefficients JPP((j__compress__ptr cinfo,
					  jvirt__barray__ptr * coef__arrays));
EXTERN(void) jpeg__copy__critical__parameters JPP((j__decompress__ptr srcinfo,
						j__compress__ptr dstinfo));

/* If you choose to abort compression or decompression before completing
 * jpeg__finish__(de)compress, then you need to clean up to release memory,
 * temporary files, etc.  You can just call jpeg__destroy__(de)compress
 * if you're done with the JPEG object, but if you want to clean it up and
 * reuse it, call this:
 */
EXTERN(void) jpeg__abort__compress JPP((j__compress__ptr cinfo));
EXTERN(void) jpeg__abort__decompress JPP((j__decompress__ptr cinfo));

/* Generic versions of jpeg__abort and jpeg__destroy that work on either
 * flavor of JPEG object.  These may be more convenient in some places.
 */
EXTERN(void) jpeg__abort JPP((j__common__ptr cinfo));
EXTERN(void) jpeg__destroy JPP((j__common__ptr cinfo));

/* Default restart-marker-resync procedure for use by data source modules */
EXTERN(CH_BOOL) jpeg__resync__to__restart JPP((j__decompress__ptr cinfo,
					    int desired));


/* These marker codes are exported since applications and data source modules
 * are likely to want to use them.
 */

#define JPEG__RST0	0xD0	/* RST0 marker code */
#define JPEG__EOI	0xD9	/* EOI marker code */
#define JPEG__APP0	0xE0	/* APP0 marker code */
#define JPEG__COM	0xFE	/* COM marker code */


/* If we have a brain-damaged compiler that emits warnings (or worse, errors)
 * for structure definitions that are never filled in, keep it quiet by
 * supplying dummy definitions for the various substructures.
 */

#ifdef INCOMPLETE__TYPES__BROKEN
#ifndef JPEG__INTERNALS		/* will be defined in jpegint.h */
struct jvirt__sarray__control { long dummy; };
struct jvirt__barray__control { long dummy; };
struct jpeg__comp__master { long dummy; };
struct jpeg__c__main__controller { long dummy; };
struct jpeg__c__prep__controller { long dummy; };
struct jpeg__c__coef__controller { long dummy; };
struct jpeg__marker__writer { long dummy; };
struct jpeg__color__converter { long dummy; };
struct jpeg__downsampler { long dummy; };
struct jpeg__forward__dct { long dummy; };
struct jpeg__entropy__encoder { long dummy; };
struct jpeg__decomp__master { long dummy; };
struct jpeg__d__main__controller { long dummy; };
struct jpeg__d__coef__controller { long dummy; };
struct jpeg__d__post__controller { long dummy; };
struct jpeg__input__controller { long dummy; };
struct jpeg__marker__reader { long dummy; };
struct jpeg__entropy__decoder { long dummy; };
struct jpeg__inverse__dct { long dummy; };
struct jpeg__upsampler { long dummy; };
struct jpeg__color__deconverter { long dummy; };
struct jpeg__color__quantizer { long dummy; };
#endif /* JPEG__INTERNALS */
#endif /* INCOMPLETE__TYPES__BROKEN */


/*
 * The JPEG library modules define JPEG__INTERNALS before including this file.
 * The internal structure declarations are read only when that is true.
 * Applications using the library should not include jpegint.h, but may wish
 * to include jerror.h.
 */

#ifdef JPEG__INTERNALS
#include "jpegint.h"		/* fetch private declarations */
#include "jerror.h"		/* fetch error codes too */
#endif

#endif /* JPEGLIB__H */
