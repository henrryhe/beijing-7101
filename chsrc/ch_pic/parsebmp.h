#ifndef BMP_H
#define BMP_H
#include "graphconfig.h"

#define BI_RGB  	0
#define BI_RLE4  	1
#define BI_RLE8	2

#define ALPHA    0x00

typedef struct tagMYBITMAPINFOHEADER {
	
    U16  bfType;             /* Specifies the type of file. This member must be BM. */
    U32  bfSize;             /* Specifies the size of the file, in bytes. */
    U16  bfReserved1;        /* Reserved; must be set to zero. */
    U16  bfReserved2;        /* Reserved; must be set to zero.*/
    U32 bfOffBits;          /* Specifies the byte offset from the BITMAPFILEHEADER*/ 
    U32  biSize;             /* Specifies the number of bytes required by the*/
                                /* BITMAPINFOHEADER structure.*/
    U32  biWidth;            /* Specifies the width of the bitmap, in pixels.*/
    U32  biHeight;           /* Specifies the height of the bitmap, in pixels.*/
    U16  biPlanes;           /* Specifies the number of planes for the target device.*/ 
                                /* This member must be set to 1.*/
    U16  biBitCount;         /* Specifies the number of bits per pixel.*/ 
                                /* This value must be 1, 4, 8, or 24.*/
    U32  biCompression;      /* Specifies the type of compression for a compressed */
                                /* bitmap. It can be one of the following values:*/
                                /* BI_RGB  (0)  bitmap is not compressed. */
                                /* BI_RLE8 (1)  8 BPP run-length encoded bitmap.*/
                                /* Compression format is a 2-byte format - a count byte*/ 
                                /* followed by a byte containing a color index.*/
                                /* BI_RLE4 (2?) 4 BPP run-length encoded bitmap*/
                                /* Compression format is a 2-byte format - a count byte*/ 
                                /* followed by two U16-length color indexes.  */
    U32  biSizeImage;        /* The size, in bytes, of the image. It is valid to set this*/ 
                                /* member to zero if the bitmap is in the BI_RGB format.*/
    U32    biXPelsPerMeter;    /* The horizontal resolution, in pixels per meter, of the*/ 
                                /* target device for the bitmap. An application can use this */
                                /* value to select a bitmap from a resource group that best */
                                /* matches the characteristics of the current device.*/
    U32    biYPelsPerMeter;    /* The vertical resolution, in pixels per meter, of the*/ 
                                /* target device for the bitmap.*/
    U32  biClrUsed;          /* the number of color indexes in the color table actually*/ 
                                /* used by the bitmap. If this value is zero, the bitmap uses */
                                /* the maximum number of colors corresponding to the value of*/ 
                                /* the biBitCount member.*/
    U32  biClrImportant;     /* the number of color indexes that are considered important*/ 
                                /* for displaying the bitmap. If this value is zero, all */
                                /* colors are important.*/
} MYBITMAPINFOHEADER;

typedef struct tagMYRGBQUAD {     /* rgbq */
    U8    rgbBlue;
    U8    rgbGreen;
    U8    rgbRed;
    U8    rgbReserved;
} MYRGBQUAD;

typedef struct
{
	U32	Depth;
	U32	Height;
	U32	Size;
	U32	Width;
	U8* Data;
} BMPImage;

#endif
