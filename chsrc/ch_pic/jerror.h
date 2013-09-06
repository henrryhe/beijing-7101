/*
 * jerror.h
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file defines the error and message codes for the JPEG library.
 * Edit this file to add new codes, or to translate the message strings to
 * some other language.
 * A set of error-reporting macros are defined too.  Some applications using
 * the JPEG library may wish to include this file to get the error codes
 * and/or the macros.
 */

/*
 * To define the enum list of message codes, include this file without
 * defining macro JMESSAGE.  To create a message string table, include it
 * again with a suitable JMESSAGE definition (see jerror.c for an example).
 */
#ifndef JMESSAGE
#ifndef JERROR__H
/* First time through, define the enum list */
#define JMAKE__ENUM__LIST
#else
/* Repeated inclusions of this file are no-ops unless JMESSAGE is defined */
#define JMESSAGE(code,string)
#endif /* JERROR__H */
#endif /* JMESSAGE */

#ifdef JMAKE__ENUM__LIST

typedef enum {

#define JMESSAGE(code,string)	code ,

#endif /* JMAKE__ENUM__LIST */

JMESSAGE(JMSG__NOMESSAGE, "Bogus message code %d") /* Must be first entry! */

/* For maintenance convenience, list is alphabetical by message code name */
JMESSAGE(JERR__ARITH__NOTIMPL,
	 "Sorry, there are legal restrictions on arithmetic coding")
JMESSAGE(JERR__BAD__ALIGN__TYPE, "ALIGN__TYPE is wrong, please fix")
JMESSAGE(JERR__BAD__ALLOC__CHUNK, "MAX__ALLOC__CHUNK is wrong, please fix")
JMESSAGE(JERR__BAD__BUFFER__MODE, "Bogus buffer control mode")
JMESSAGE(JERR__BAD__COMPONENT__ID, "Invalid component ID %d in SOS")
JMESSAGE(JERR__BAD__DCT__COEF, "DCT coefficient out of range")
JMESSAGE(JERR__BAD__DCTSIZE, "IDCT output block size %d not supported")
JMESSAGE(JERR__BAD__HUFF__TABLE, "Bogus Huffman table definition")
JMESSAGE(JERR__BAD__IN__COLORSPACE, "Bogus input colorspace")
JMESSAGE(JERR__BAD__J__COLORSPACE, "Bogus JPEG colorspace")
JMESSAGE(JERR__BAD__LENGTH, "Bogus marker length")
JMESSAGE(JERR__BAD__LIB__VERSION,
	 "Wrong JPEG library version: library is %d, caller expects %d")
JMESSAGE(JERR__BAD__MCU__SIZE, "Sampling factors too large for interleaved scan")
JMESSAGE(JERR__BAD__POOL__ID, "Invalid memory pool code %d")
JMESSAGE(JERR__BAD__PRECISION, "Unsupported JPEG data precision %d")
JMESSAGE(JERR__BAD__PROGRESSION,
	 "Invalid progressive parameters Ss=%d Se=%d Ah=%d Al=%d")
JMESSAGE(JERR__BAD__PROG__SCRIPT,
	 "Invalid progressive parameters at scan script entry %d")
JMESSAGE(JERR__BAD__SAMPLING, "Bogus sampling factors")
JMESSAGE(JERR__BAD__SCAN__SCRIPT, "Invalid scan script at entry %d")
JMESSAGE(JERR__BAD__STATE, "Improper call to JPEG library in state %d")
JMESSAGE(JERR__BAD__STRUCT__SIZE,
	 "JPEG parameter struct mismatch: library thinks size is %u, caller expects %u")
JMESSAGE(JERR__BAD__VIRTUAL__ACCESS, "Bogus virtual array access")
JMESSAGE(JERR__BUFFER__SIZE, "Buffer passed to JPEG library is too small")
JMESSAGE(JERR__CANT__SUSPEND, "Suspension not allowed here")
JMESSAGE(JERR__CCIR601__NOTIMPL, "CCIR601 sampling not implemented yet")
JMESSAGE(JERR__COMPONENT__COUNT, "Too many color components: %d, max %d")
JMESSAGE(JERR__CONVERSION__NOTIMPL, "Unsupported color conversion request")
JMESSAGE(JERR__DAC__INDEX, "Bogus DAC index %d")
JMESSAGE(JERR__DAC__VALUE, "Bogus DAC value 0x%x")
JMESSAGE(JERR__DHT__INDEX, "Bogus DHT index %d")
JMESSAGE(JERR__DQT__INDEX, "Bogus DQT index %d")
JMESSAGE(JERR__EMPTY__IMAGE, "Empty JPEG image (DNL not supported)")
JMESSAGE(JERR__EMS__READ, "Read from EMS failed")
JMESSAGE(JERR__EMS__WRITE, "Write to EMS failed")
JMESSAGE(JERR__EOI__EXPECTED, "Didn't expect more than one scan")
JMESSAGE(JERR__FILE__READ, "Input file read error")
JMESSAGE(JERR__FILE__WRITE, "Output file write error --- out of disk space?")
JMESSAGE(JERR__FRACT__SAMPLE__NOTIMPL, "Fractional sampling not implemented yet")
JMESSAGE(JERR__HUFF__CLEN__OVERFLOW, "Huffman code size table overflow")
JMESSAGE(JERR__HUFF__MISSING__CODE, "Missing Huffman code table entry")
JMESSAGE(JERR__IMAGE__TOO__BIG, "Maximum supported image dimension is %u pixels")
JMESSAGE(JERR__INPUT__EMPTY, "Empty input file")
JMESSAGE(JERR__INPUT__EOF, "Premature end of input file")
JMESSAGE(JERR__MISMATCHED__QUANT__TABLE,
	 "Cannot transcode due to multiple use of quantization table %d")
JMESSAGE(JERR__MISSING__DATA, "Scan script does not transmit all data")
JMESSAGE(JERR__MODE__CHANGE, "Invalid color quantization mode change")
JMESSAGE(JERR__NOTIMPL, "Not implemented yet")
JMESSAGE(JERR__NOT__COMPILED, "Requested feature was omitted at compile time")
JMESSAGE(JERR__NO__BACKING__STORE, "Backing store not supported")
JMESSAGE(JERR__NO__HUFF__TABLE, "Huffman table 0x%02x was not defined")
JMESSAGE(JERR__NO__IMAGE, "JPEG datastream contains no image")
JMESSAGE(JERR__NO__QUANT__TABLE, "Quantization table 0x%02x was not defined")
JMESSAGE(JERR__NO__SOI, "Not a JPEG file: starts with 0x%02x 0x%02x")
JMESSAGE(JERR__OUT__OF__MEMORY, "Insufficient memory (case %d)")
JMESSAGE(JERR__QUANT__COMPONENTS,
	 "Cannot quantize more than %d color components")
JMESSAGE(JERR__QUANT__FEW__COLORS, "Cannot quantize to fewer than %d colors")
JMESSAGE(JERR__QUANT__MANY__COLORS, "Cannot quantize to more than %d colors")
JMESSAGE(JERR__SOF__DUPLICATE, "Invalid JPEG file structure: two SOF markers")
JMESSAGE(JERR__SOF__NO__SOS, "Invalid JPEG file structure: missing SOS marker")
JMESSAGE(JERR__SOF__UNSUPPORTED, "Unsupported JPEG process: SOF type 0x%02x")
JMESSAGE(JERR__SOI__DUPLICATE, "Invalid JPEG file structure: two SOI markers")
JMESSAGE(JERR__SOS__NO__SOF, "Invalid JPEG file structure: SOS before SOF")
JMESSAGE(JERR__TFILE__CREATE, "Failed to create temporary file %s")
JMESSAGE(JERR__TFILE__READ, "Read failed on temporary file")
JMESSAGE(JERR__TFILE__SEEK, "Seek failed on temporary file")
JMESSAGE(JERR__TFILE__WRITE,
	 "Write failed on temporary file --- out of disk space?")
JMESSAGE(JERR__TOO__LITTLE__DATA, "Application transferred too few scanlines")
JMESSAGE(JERR__UNKNOWN__MARKER, "Unsupported marker type 0x%02x")
JMESSAGE(JERR__VIRTUAL__BUG, "Virtual array controller messed up")
JMESSAGE(JERR__WIDTH__OVERFLOW, "Image too wide for this implementation")
JMESSAGE(JERR__XMS__READ, "Read from XMS failed")
JMESSAGE(JERR__XMS__WRITE, "Write to XMS failed")
JMESSAGE(JMSG__COPYRIGHT, JCOPYRIGHT)
JMESSAGE(JMSG__VERSION, JVERSION)
JMESSAGE(JTRC__16BIT__TABLES,
	 "Caution: quantization tables are too coarse for baseline JPEG")
JMESSAGE(JTRC__ADOBE,
	 "Adobe APP14 marker: version %d, flags 0x%04x 0x%04x, transform %d")
JMESSAGE(JTRC__APP0, "Unknown APP0 marker (not JFIF), length %u")
JMESSAGE(JTRC__APP14, "Unknown APP14 marker (not Adobe), length %u")
JMESSAGE(JTRC__DAC, "Define Arithmetic Table 0x%02x: 0x%02x")
JMESSAGE(JTRC__DHT, "Define Huffman Table 0x%02x")
JMESSAGE(JTRC__DQT, "Define Quantization Table %d  precision %d")
JMESSAGE(JTRC__DRI, "Define Restart Interval %u")
JMESSAGE(JTRC__EMS__CLOSE, "Freed EMS handle %u")
JMESSAGE(JTRC__EMS__OPEN, "Obtained EMS handle %u")
JMESSAGE(JTRC__EOI, "End Of Image")
JMESSAGE(JTRC__HUFFBITS, "        %3d %3d %3d %3d %3d %3d %3d %3d")
JMESSAGE(JTRC__JFIF, "JFIF APP0 marker: version %d.%02d, density %dx%d  %d")
JMESSAGE(JTRC__JFIF__BADTHUMBNAILSIZE,
	 "Warning: thumbnail image size does not match data length %u")
JMESSAGE(JTRC__JFIF__EXTENSION,
	 "JFIF extension marker: type 0x%02x, length %u")
JMESSAGE(JTRC__JFIF__THUMBNAIL, "    with %d x %d thumbnail image")
JMESSAGE(JTRC__MISC__MARKER, "Miscellaneous marker 0x%02x, length %u")
JMESSAGE(JTRC__PARMLESS__MARKER, "Unexpected marker 0x%02x")
JMESSAGE(JTRC__QUANTVALS, "        %4u %4u %4u %4u %4u %4u %4u %4u")
JMESSAGE(JTRC__QUANT__3__NCOLORS, "Quantizing to %d = %d*%d*%d colors")
JMESSAGE(JTRC__QUANT__NCOLORS, "Quantizing to %d colors")
JMESSAGE(JTRC__QUANT__SELECTED, "Selected %d colors for quantization")
JMESSAGE(JTRC__RECOVERY__ACTION, "At marker 0x%02x, recovery action %d")
JMESSAGE(JTRC__RST, "RST%d")
JMESSAGE(JTRC__SMOOTH__NOTIMPL,
	 "Smoothing not supported with nonstandard sampling ratios")
JMESSAGE(JTRC__SOF, "Start Of Frame 0x%02x: width=%u, height=%u, components=%d")
JMESSAGE(JTRC__SOF__COMPONENT, "    Component %d: %dhx%dv q=%d")
JMESSAGE(JTRC__SOI, "Start of Image")
JMESSAGE(JTRC__SOS, "Start Of Scan: %d components")
JMESSAGE(JTRC__SOS__COMPONENT, "    Component %d: dc=%d ac=%d")
JMESSAGE(JTRC__SOS__PARAMS, "  Ss=%d, Se=%d, Ah=%d, Al=%d")
JMESSAGE(JTRC__TFILE__CLOSE, "Closed temporary file %s")
JMESSAGE(JTRC__TFILE__OPEN, "Opened temporary file %s")
JMESSAGE(JTRC__THUMB__JPEG,
	 "JFIF extension marker: JPEG-compressed thumbnail image, length %u")
JMESSAGE(JTRC__THUMB__PALETTE,
	 "JFIF extension marker: palette thumbnail image, length %u")
JMESSAGE(JTRC__THUMB__RGB,
	 "JFIF extension marker: RGB thumbnail image, length %u")
JMESSAGE(JTRC__UNKNOWN__IDS,
	 "Unrecognized component IDs %d %d %d, assuming YCbCr")
JMESSAGE(JTRC__XMS__CLOSE, "Freed XMS handle %u")
JMESSAGE(JTRC__XMS__OPEN, "Obtained XMS handle %u")
JMESSAGE(JWRN__ADOBE__XFORM, "Unknown Adobe color transform code %d")
JMESSAGE(JWRN__BOGUS__PROGRESSION,
	 "Inconsistent progression sequence for component %d coefficient %d")
JMESSAGE(JWRN__EXTRANEOUS__DATA,
	 "Corrupt JPEG data: %u extraneous bytes before marker 0x%02x")
JMESSAGE(JWRN__HIT__MARKER, "Corrupt JPEG data: premature end of data segment")
JMESSAGE(JWRN__HUFF__BAD__CODE, "Corrupt JPEG data: bad Huffman code")
JMESSAGE(JWRN__JFIF__MAJOR, "Warning: unknown JFIF revision number %d.%02d")
JMESSAGE(JWRN__JPEG__EOF, "Premature end of JPEG file")
JMESSAGE(JWRN__MUST__RESYNC,
	 "Corrupt JPEG data: found marker 0x%02x instead of RST%d")
JMESSAGE(JWRN__NOT__SEQUENTIAL, "Invalid SOS parameters for sequential JPEG")
JMESSAGE(JWRN__TOO__MUCH_DATA, "Application transferred too many scanlines")

#ifdef JMAKE__ENUM__LIST

  JMSG__LASTMSGCODE
} J__MESSAGE__CODE;

#undef JMAKE__ENUM__LIST
#endif /* JMAKE__ENUM__LIST */

/* Zap JMESSAGE macro so that future re-inclusions do nothing by default */
#undef JMESSAGE


#ifndef JERROR__H
#define JERROR__H

/* Macros to simplify using the error and trace message stuff */
/* The first parameter is either type of cinfo pointer */

/* Fatal errors (print message and exit) */
#define ERREXIT(cinfo,code)  \
  ((cinfo)->err->msg__code = (code), \
   (*(cinfo)->err->error__exit) ((j__common__ptr) (cinfo)))
#define ERREXIT1(cinfo,code,p1)  \
  ((cinfo)->err->msg__code = (code), \
   (cinfo)->err->msg__parm.i[0] = (p1), \
   (*(cinfo)->err->error__exit) ((j__common__ptr) (cinfo)))
#define ERREXIT2(cinfo,code,p1,p2)  \
  ((cinfo)->err->msg__code = (code), \
   (cinfo)->err->msg__parm.i[0] = (p1), \
   (cinfo)->err->msg__parm.i[1] = (p2), \
   (*(cinfo)->err->error__exit) ((j__common__ptr) (cinfo)))
#define ERREXIT3(cinfo,code,p1,p2,p3)  \
  ((cinfo)->err->msg__code = (code), \
   (cinfo)->err->msg__parm.i[0] = (p1), \
   (cinfo)->err->msg__parm.i[1] = (p2), \
   (cinfo)->err->msg__parm.i[2] = (p3), \
   (*(cinfo)->err->error__exit) ((j__common__ptr) (cinfo)))
#define ERREXIT4(cinfo,code,p1,p2,p3,p4)  \
  ((cinfo)->err->msg__code = (code), \
   (cinfo)->err->msg__parm.i[0] = (p1), \
   (cinfo)->err->msg__parm.i[1] = (p2), \
   (cinfo)->err->msg__parm.i[2] = (p3), \
   (cinfo)->err->msg__parm.i[3] = (p4), \
   (*(cinfo)->err->error__exit) ((j__common__ptr) (cinfo)))
#define ERREXITS(cinfo,code,str)  \
  ((cinfo)->err->msg__code = (code), \
   strncpy((cinfo)->err->msg__parm.s, (str), JMSG__STR__PARM__MAX), \
   (*(cinfo)->err->error__exit) ((j__common__ptr) (cinfo)))

#define MAKESTMT(stuff)		do { stuff } while (0)

/* Nonfatal errors (we can keep going, but the data is probably corrupt) */
#define WARNMS(cinfo,code)  \
  ((cinfo)->err->msg__code = (code), \
   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), -1))
#define WARNMS1(cinfo,code,p1)  \
  ((cinfo)->err->msg__code = (code), \
   (cinfo)->err->msg__parm.i[0] = (p1), \
   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), -1))
#define WARNMS2(cinfo,code,p1,p2)  \
  ((cinfo)->err->msg__code = (code), \
   (cinfo)->err->msg__parm.i[0] = (p1), \
   (cinfo)->err->msg__parm.i[1] = (p2), \
   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), -1))

/* Informational/debugging messages */
#define TRACEMS(cinfo,lvl,code)  \
  ((cinfo)->err->msg__code = (code), \
   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), (lvl)))
#define TRACEMS1(cinfo,lvl,code,p1)  \
  ((cinfo)->err->msg__code = (code), \
   (cinfo)->err->msg__parm.i[0] = (p1), \
   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), (lvl)))
#define TRACEMS2(cinfo,lvl,code,p1,p2)  \
  ((cinfo)->err->msg__code = (code), \
   (cinfo)->err->msg__parm.i[0] = (p1), \
   (cinfo)->err->msg__parm.i[1] = (p2), \
   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), (lvl)))
#define TRACEMS3(cinfo,lvl,code,p1,p2,p3)  \
  MAKESTMT(int * __mp = (cinfo)->err->msg__parm.i; \
	   __mp[0] = (p1); __mp[1] = (p2); __mp[2] = (p3); \
	   (cinfo)->err->msg__code = (code); \
	   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), (lvl)); )
#define TRACEMS4(cinfo,lvl,code,p1,p2,p3,p4)  \
  MAKESTMT(int * __mp = (cinfo)->err->msg__parm.i; \
	   __mp[0] = (p1); __mp[1] = (p2); __mp[2] = (p3); __mp[3] = (p4); \
	   (cinfo)->err->msg__code = (code); \
	   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), (lvl)); )
#define TRACEMS5(cinfo,lvl,code,p1,p2,p3,p4,p5)  \
  MAKESTMT(int * __mp = (cinfo)->err->msg__parm.i; \
	   __mp[0] = (p1); __mp[1] = (p2); __mp[2] = (p3); __mp[3] = (p4); \
	   __mp[4] = (p5); \
	   (cinfo)->err->msg__code = (code); \
	   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), (lvl)); )
#define TRACEMS8(cinfo,lvl,code,p1,p2,p3,p4,p5,p6,p7,p8)  \
  MAKESTMT(int * __mp = (cinfo)->err->msg__parm.i; \
	   __mp[0] = (p1); __mp[1] = (p2); __mp[2] = (p3); __mp[3] = (p4); \
	   __mp[4] = (p5); __mp[5] = (p6); __mp[6] = (p7); __mp[7] = (p8); \
	   (cinfo)->err->msg__code = (code); \
	   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), (lvl)); )
#define TRACEMSS(cinfo,lvl,code,str)  \
  ((cinfo)->err->msg__code = (code), \
   strncpy((cinfo)->err->msg__parm.s, (str), JMSG__STR__PARM__MAX), \
   (*(cinfo)->err->emit__message) ((j__common__ptr) (cinfo), (lvl)))

#endif /* JERROR__H */
