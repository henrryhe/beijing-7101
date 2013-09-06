/*
 * cderror.h
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file defines the error and message codes for the cjpeg/djpeg
 * applications.  These strings are not needed as part of the JPEG library
 * proper.
 * Edit this file to add new codes, or to translate the message strings to
 * some other language.
 */

/*
 * To define the enum list of message codes, include this file without
 * defining macro JMESSAGE.  To create a message string table, include it
 * again with a suitable JMESSAGE definition (see jerror.c for an example).
 */
#ifndef JMESSAGE
#ifndef CDERROR__H
#define CDERROR__H
/* First time through, define the enum list */
#define JMAKE__ENUM__LIST
#else
/* Repeated inclusions of this file are no-ops unless JMESSAGE is defined */
#define JMESSAGE(code,string)
#endif /* CDERROR__H */
#endif /* JMESSAGE */

#ifdef JMAKE__ENUM__LIST

typedef enum {

#define JMESSAGE(code,string)	code ,

#endif /* JMAKE__ENUM__LIST */

JMESSAGE(JMSG__FIRSTADDONCODE=1000, NULL) /* Must be first entry! */

#ifdef BMP__SUPPORTED
JMESSAGE(JERR__BMP__BADCMAP, "Unsupported BMP colormap format")
JMESSAGE(JERR__BMP__BADDEPTH, "Only 8- and 24-bit BMP files are supported")
JMESSAGE(JERR__BMP__BADHEADER, "Invalid BMP file: bad header length")
JMESSAGE(JERR__BMP__BADPLANES, "Invalid BMP file: biPlanes not equal to 1")
JMESSAGE(JERR__BMP__COLORSPACE, "BMP output must be grayscale or RGB")
JMESSAGE(JERR__BMP__COMPRESSED, "Sorry, compressed BMPs not yet supported")
JMESSAGE(JERR__BMP__NOT, "Not a BMP file - does not start with BM")
JMESSAGE(JTRC__BMP, "%ux%u 24-bit BMP image")
JMESSAGE(JTRC__BMP__MAPPED, "%ux%u 8-bit colormapped BMP image")
JMESSAGE(JTRC__BMP__OS2, "%ux%u 24-bit OS2 BMP image")
JMESSAGE(JTRC__BMP__OS2__MAPPED, "%ux%u 8-bit colormapped OS2 BMP image")
#endif /* BMP__SUPPORTED */

#ifdef GIF__SUPPORTED
JMESSAGE(JERR__GIF__BUG, "GIF output got confused")
JMESSAGE(JERR__GIF__CODESIZE, "Bogus GIF codesize %d")
JMESSAGE(JERR__GIF__COLORSPACE, "GIF output must be grayscale or RGB")
JMESSAGE(JERR__GIF__IMAGENOTFOUND, "Too few images in GIF file")
JMESSAGE(JERR__GIF__NOT, "Not a GIF file")
JMESSAGE(JTRC__GIF, "%ux%ux%d GIF image")
JMESSAGE(JTRC__GIF__BADVERSION,
	 "Warning: unexpected GIF version number '%c%c%c'")
JMESSAGE(JTRC__GIF__EXTENSION, "Ignoring GIF extension block of type 0x%02x")
JMESSAGE(JTRC__GIF__NONSQUARE, "Caution: nonsquare pixels in input")
JMESSAGE(JWRN__GIF__BADDATA, "Corrupt data in GIF file")
JMESSAGE(JWRN__GIF__CHAR, "Bogus char 0x%02x in GIF file, ignoring")
JMESSAGE(JWRN__GIF__ENDCODE, "Premature end of GIF image")
JMESSAGE(JWRN__GIF__NOMOREDATA, "Ran out of GIF bits")
#endif /* GIF__SUPPORTED */

#ifdef PPM__SUPPORTED
JMESSAGE(JERR__PPM__COLORSPACE, "PPM output must be grayscale or RGB")
JMESSAGE(JERR__PPM__NONNUMERIC, "Nonnumeric data in PPM file")
JMESSAGE(JERR__PPM__NOT, "Not a PPM/PGM file")
JMESSAGE(JTRC__PGM, "%ux%u PGM image")
JMESSAGE(JTRC__PGM__TEXT, "%ux%u text PGM image")
JMESSAGE(JTRC__PPM, "%ux%u PPM image")
JMESSAGE(JTRC__PPM__TEXT, "%ux%u text PPM image")
#endif /* PPM__SUPPORTED */

#ifdef RLE__SUPPORTED
JMESSAGE(JERR__RLE__BADERROR, "Bogus error code from RLE library")
JMESSAGE(JERR__RLE__COLORSPACE, "RLE output must be grayscale or RGB")
JMESSAGE(JERR__RLE__DIMENSIONS, "Image dimensions (%ux%u) too large for RLE")
JMESSAGE(JERR__RLE__EMPTY, "Empty RLE file")
JMESSAGE(JERR__RLE__EOF, "Premature EOF in RLE header")
JMESSAGE(JERR__RLE__MEM, "Insufficient memory for RLE header")
JMESSAGE(JERR__RLE__NOT, "Not an RLE file")
JMESSAGE(JERR__RLE__TOOMANYCHANNELS, "Cannot handle %d output channels for RLE")
JMESSAGE(JERR__RLE__UNSUPPORTED, "Cannot handle this RLE setup")
JMESSAGE(JTRC__RLE, "%ux%u full-color RLE file")
JMESSAGE(JTRC__RLE__FULLMAP, "%ux%u full-color RLE file with map of length %d")
JMESSAGE(JTRC__RLE__GRAY, "%ux%u grayscale RLE file")
JMESSAGE(JTRC__RLE__MAPGRAY, "%ux%u grayscale RLE file with map of length %d")
JMESSAGE(JTRC__RLE__MAPPED, "%ux%u colormapped RLE file with map of length %d")
#endif /* RLE__SUPPORTED */

#ifdef TARGA__SUPPORTED
JMESSAGE(JERR__TGA__BADCMAP, "Unsupported Targa colormap format")
JMESSAGE(JERR__TGA__BADPARMS, "Invalid or unsupported Targa file")
JMESSAGE(JERR__TGA__COLORSPACE, "Targa output must be grayscale or RGB")
JMESSAGE(JTRC__TGA, "%ux%u RGB Targa image")
JMESSAGE(JTRC__TGA__GRAY, "%ux%u grayscale Targa image")
JMESSAGE(JTRC__TGA__MAPPED, "%ux%u colormapped Targa image")
#else
JMESSAGE(JERR__TGA__NOTCOMP, "Targa support was not compiled")
#endif /* TARGA__SUPPORTED */

JMESSAGE(JERR__BAD__CMAP__FILE,
	 "Color map file is invalid or of unsupported format")
JMESSAGE(JERR__TOO__MANY__COLORS,
	 "Output file format cannot handle %d colormap entries")
JMESSAGE(JERR__UNGETC__FAILED, "ungetc failed")
#ifdef TARGA__SUPPORTED
JMESSAGE(JERR__UNKNOWN__FORMAT,
	 "Unrecognized input file format --- perhaps you need -targa")
#else
JMESSAGE(JERR__UNKNOWN__FORMAT, "Unrecognized input file format")
#endif
JMESSAGE(JERR__UNSUPPORTED__FORMAT, "Unsupported output file format")

#ifdef JMAKE__ENUM__LIST

  JMSG__LASTADDONCODE
} ADDON__MESSAGE__CODE;

#undef JMAKE__ENUM__LIST
#endif /* JMAKE__ENUM__LIST */

/* Zap JMESSAGE macro so that future re-inclusions do nothing by default */
#undef JMESSAGE
