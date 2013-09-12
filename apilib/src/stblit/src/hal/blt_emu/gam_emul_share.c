#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "gam_emul_util.h"
#include "gam_emul_global.h"
#include "gam_emul_share.h"


/*----------------------------------------------------------------------------------
 * Function : gam_emul_ConvertYCbCr2RGB
 * Parameters: y, cb, cr, r, g, b, type (601 -709), nob_in (number of bit by composant -> in)
 , nob_out (number of bit by composant -> out), c_format (Chroma format)
 * Return :
 * Description : to convert rgb to YCbCr format in function of fixed coefficients. if matrixNoGfxVideo
 * is equal to 0 then "Graphic RGB color space" is used else "Video RGB color space"
 *----------------------------------------------------------------------------------*/

void gam_emul_ConvertYCbCr2RGB (int *y,int *cb,int *cr,int *r,int *g,int *b,int type,int nob_in,int nob_out,int c_format, int matrixNoGfxVideo)
/* Coefficient format : S1.9 (1.0 = 256) */
{
	int chroma_offset,luma_offset;
	int chroma_min, chroma_max;

	luma_offset = 0;

	switch (nob_in)
	{
		case 8:
			if (c_format == 0)			/* means cb/cr in offset binary */
			{
				chroma_offset = 128;
				/* used only in graphic rgb color space */
				chroma_min = 16;
				chroma_max = 240;
			}
			else										/* cb/cr signed */
			{
				if ((*cb & 0x80) != 0)	/* if cb < 0, */
					*cb |= 0xFFFFFF00;		/* then sign extension */
				if ((*cr & 0x80) != 0)	/* if cr < 0, */
					*cr |= 0xFFFFFF00;		/* then sign extension */
				chroma_offset = 0;
				/* used only in graphic rgb color space */
				chroma_min = -112;
				chroma_max = 112;
			}
			/* used only in graphic rgb color space */
			if (matrixNoGfxVideo == 0)
			{
				luma_offset = 16;
				if (*y < 16)
					*y = 16;
				if (*y > 235)
					*y = 235;
				if (*cb < chroma_min)
					*cb = chroma_min;
				if (*cb > chroma_max)
					*cb = chroma_max;
				if (*cr < chroma_min)
					*cr = chroma_min;
				if (*cr > chroma_max)
					*cr = chroma_max;
			}
			break;
		case 10:
			if (c_format == 0)			/* means cb/cr in offset binary */
			{
				chroma_offset = 512;
				/* used only in graphic rgb color space */
				chroma_min = 64;
				chroma_max = 960;
			}
			else										/* cb/cr signed */
			{
				if ((*cb & 0x200) != 0)	/* if cb < 0, */
					*cb |= 0xFFFFFC00;		/* then sign extension */
				if ((*cr & 0x200) != 0)	/* if cr < 0, */
					*cr |= 0xFFFFFC00;		/* then sign extension */
				/* used only in graphic rgb color space */
				chroma_offset = 0;
				chroma_min = -448;
				chroma_max = 448;
			}
			/* used only in graphic rgb color space */
			if (matrixNoGfxVideo == 0)
			{
				luma_offset = 64;
				if (*y < 64)
					*y = 64;
				if (*y > 940)
					*y = 940;
				if (*cb < chroma_min)
					*cb = chroma_min;
				if (*cb > chroma_max)
					*cb = chroma_max;
				if (*cr < chroma_min)
					*cr = chroma_min;
				if (*cr > chroma_max)
					*cr = chroma_max;
			}
			break;
		default:
			chroma_offset = 0;
			luma_offset = 0;
			break;
	}

	if (matrixNoGfxVideo == 0)
	{
		/* used only in graphic rgb color space */
		switch (type)
		{
			case 0:				/* 601 matrix, fixed coefficients */
				*r  = 298 * (*y - luma_offset)
						+ 409 * (*cr - chroma_offset);
				*g  = 298 * (*y - luma_offset)
						- 208 * (*cr - chroma_offset)
						- 100 * (*cb - chroma_offset);
				*b  = 298 * (*y - luma_offset)
						+ 517 * (*cb - chroma_offset);
				break;
			case 1:				/* 709 matrix, fixed coefficients */
				*r  = 298 * (*y - luma_offset)
						+ 459 * (*cr - chroma_offset);
				*g  = 298 * (*y - luma_offset)
						- 136 * (*cr - chroma_offset)
						-  54 * (*cb - chroma_offset);
				*b  = 298 * (*y - luma_offset)
						+ 541 * (*cb - chroma_offset);
				break;
			default:
				break;
		}
	}
	else
	{
		/* used only in video rgb color space */
		switch (type)
		{
			case 0:				/* 601 matrix, fixed coefficients */
				*r  = *y * 256 + 351 * (*cr - chroma_offset);
				*g  = *y * 256 - 179 * (*cr - chroma_offset)
						- 86 * (*cb - chroma_offset);
				*b  = *y * 256 + 444 * (*cb - chroma_offset);
				break;
			case 1:				/* 709 matrix, fixed coefficients */
				*r  = *y * 256 + 394 * (*cr - chroma_offset);
				*g  = *y * 256 - 117 * (*cr - chroma_offset)
						-  47 * (*cb - chroma_offset);
				*b  = *y * 256 + 464 * (*cb - chroma_offset);
				break;
			default:
				break;
		}
	}
	if ((nob_in == 8) && (nob_out == 8))
	{
		*r = ((*r >> 7) + 1) >> 1;
		*g = ((*g >> 7) + 1) >> 1;
		*b = ((*b >> 7) + 1) >> 1;

		if (*r<0) *r = 0; if (*r>255) *r = 255;
		if (*g<0) *g = 0; if (*g>255) *g = 255;
		if (*b<0) *b = 0; if (*b>255) *b = 255;

	}
	if ((nob_in == 8) && (nob_out == 10))
	{
		*r = ((*r >> 7) + 1) >> 1;
		*g = ((*g >> 7) + 1) >> 1;
		*b = ((*b >> 7) + 1) >> 1;

		if (*r<-512) *r = -512; if (*r>511) *r = 511;
		if (*g<-512) *g = -512; if (*g>511) *g = 511;
		if (*b<-512) *b = -512; if (*b>511) *b = 511;
	}
	if ((nob_in == 8) && (nob_out == 12))
	{
		*r = ((*r >> 7) + 1) >> 1;
		*g = ((*g >> 7) + 1) >> 1;
		*b = ((*b >> 7) + 1) >> 1;

	}
	if ((nob_in == 10) && (nob_out == 8))
	{
		*r = ((*r >> 9) + 1) >> 1;
		*g = ((*g >> 9) + 1) >> 1;
		*b = ((*b >> 9) + 1) >> 1;

		if (*r<0) *r = 0; if (*r>255) *r = 255;
		if (*g<0) *g = 0; if (*g>255) *g = 255;
		if (*b<0) *b = 0; if (*b>255) *b = 255;
	}
	if ((nob_in == 10) && (nob_out == 10))
	{
		*r = ((*r >> 7) + 1) >> 1;
		*g = ((*g >> 7) + 1) >> 1;
		*b = ((*b >> 7) + 1) >> 1;

		if (*r<0) *r = 0; if (*r>1023) *r = 1023;
		if (*g<0) *g = 0; if (*g>1023) *g = 1023;
		if (*b<0) *b = 0; if (*b>1023) *b = 1023;

	}
	if ((nob_in == 10) && (nob_out == 12))
	{
		*r = ((*r >> 7) + 1) >> 1;
		*g = ((*g >> 7) + 1) >> 1;
		*b = ((*b >> 7) + 1) >> 1;
	}
}


/*----------------------------------------------------------------------------------
 * Function : gam_emul_CheckAddressRange
 * Parameters: Address : address to check , MemSize : size of reserved memory
 * Return : int, a correct address
 * Description : checks if address is in reserved space memory and returns a correct value
 *----------------------------------------------------------------------------------*/
int gam_emul_CheckAddressRange(int Address,int MemMask)
{
    return(Address & MemMask);
}


/*----------------------------------------------------------------------------------
 * Function : gam_emul_swap_16
 * Parameters: int PixelData, int bigNotLittleBitmap
 * Return : int data
 * Description : two bytes swap
 *----------------------------------------------------------------------------------*/

int gam_emul_swap_16 (int data, int bigNotLittleBitmap)
{

	if ((bigNotLittleBitmap & 0x1) == 0x1) /* big endian */
	{
		data = ((data & 0xFFFF0000)|(( data << 8) & 0x0FF00) | (( data >> 8) & 0x0FF));
	}

	return data;

}

/*----------------------------------------------------------------------------------
 * Function : gam_emul_swap_24
 * Parameters: int PixelData, int bigNotLittleBitmap
 * Return : int data
 * Description : two bytes swap
 *----------------------------------------------------------------------------------*/

int gam_emul_swap_24 (int data, int bigNotLittleBitmap)
{

	if ((bigNotLittleBitmap & 0x1) == 0x1) /* big endian */
	{
		data = ((data & 0xFF000000)|(( data << 16) & 0x0FF0000) | (data & 0x0FF00) |(( data >> 16) & 0x0FF));
	}

	return data;

}

/*----------------------------------------------------------------------------------
 * Function : gam_emul_swap_32
 * Parameters: int PixelData, int bigNotLittleBitmap
 * Return : int data
 * Description : two bytes swap
 *----------------------------------------------------------------------------------*/

int gam_emul_swap_32 (int data, int bigNotLittleBitmap)
{

	if ((bigNotLittleBitmap & 0x1) == 0x1) /* big endian */
	{
		data = ((( data << 24) & 0xFF000000) | ((data << 8) & 0x0FF0000) |(( data >> 8) & 0x0FF00)
			| (( data >> 24) & 0x0FF));
	}

	return data;

}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_Read42XLuma
 * Parameters: x, y, LumaAddress, Width
 * Return : -
 * Description : read luma from 4.2.X MB
 * ----------------------------------------------------------------------------------*/

/* 420/422 macroblock format : pixel access functions */

int gam_emul_Read42XLuma(int x,int y,int LumaAddress,int Width)
{
	int Offset,Block16x32Address;
	int PixelAddress,luma;

	Offset = 0;

	Block16x32Address = ((x/16) + ((y/32) * (Width/16))) << 9;
	if ((y & 0x10) == 0)			/* MB of rows 0,2,4... */
	{
		if (((x&0xF) <8) && ((y&1) == 0)) 	/* MB even row ..Area 1 */
			Offset = 0;
		if (((x&0xF)>=8) && ((y&1) == 0))		/* MB even row ..Area 2 */
			Offset = 64;
		if (((x&0xF) <8) && ((y&1) == 1))		/* MB even row ..Area 3 */
			Offset = 128;
		if (((x&0xF)>=8) && ((y&1) == 1))		/* MB even row ..Area 4 */
			Offset = 192;
	}
	else											/* MB of rows 1,3,5... */
	{
		if (((x&0xF) <8) && ((y&1) == 0))		/* MB odd row  ..Area 1 */
			Offset = 256;
		if (((x&0xF)>=8) && ((y&1) == 0))		/* MB odd row  ..Area 2 */
			Offset = 320;
		if (((x&0xF) <8) && ((y&1) == 1))		/* MB odd row  ..Area 3 */
			Offset = 384;
		if (((x&0xF)>=8) && ((y&1) == 1))		/* MB odd row  ..Area 4 */
			Offset = 448;
	}
	PixelAddress = LumaAddress
							 + Block16x32Address + Offset
							 + (((y & 0xF) >> 1) << 3)
							 + (x & 7);
	/* Now the endianess issue !!! */
	PixelAddress = (PixelAddress & 0xFFFFFFF0) + (15 - (PixelAddress & 0xF));
    luma = gam_emul_readChar(gam_emul_BLTMemory+PixelAddress) & 0xFF;        /* OUF !!! */
	return(luma);
}


/*-----------------------------------------------------------------------------------
 * Function : gam_emul_Read42XCbCr
 * Parameters: x, y,ChromaAddress, Width
 * Return : -
 * Description : return cb and cr from 4:2:X MacroBlock
 * ----------------------------------------------------------------------------------*/

int gam_emul_Read42XCbCr(int x,int y,int ChromaAddress,int Width)
{
	int Offset,Block16x16Address;
	int PixelAddress,cb,cr,xc,yc;

	/* Find equivalent xc/yc  */
	xc = x + ((y/16) * (Width/2));
	yc = y & 0xF;
	/* A 16x16 block represents 4 chroma MB (cb+cr), so 64x4x2 = 512 bytes */
	Block16x16Address = (xc/16) << 9; /*((xc/16) + ((yc/16) * (Width/32))) * 512;*/

	if ((yc&0xF) <8)  /* 0..255 */
		Block16x16Address += 0;
	else							/* 256..511 */
		Block16x16Address += 256;

	if ((xc&0xF) <8)
		Block16x16Address += 0;
	else
		Block16x16Address += 128;

	if ((yc&0x1) == 0)
	{
		if ((xc&0x7) <4)
			Offset = (xc & 7) + ((yc & 7)/2) * 8;
		else
			Offset = ((xc-4) & 7) + ((yc & 7)/2) * 8 + 32;
	}
	else
	{
		if ((xc&0x7) <4)
			Offset = (xc & 7) + ((yc & 7)/2) * 8 + 64;
		else
			Offset = ((xc-4) & 7) + ((yc & 7)/2) * 8 + 96;
	}
	PixelAddress = ChromaAddress
							 + Block16x16Address + Offset;
	/* Get Cb */
	PixelAddress = (PixelAddress & 0xFFFFFFF0) + (15 - (PixelAddress & 0xF));
    cb = gam_emul_readChar(gam_emul_BLTMemory+PixelAddress) & 0xFF;      /* OUF OUF !!! */
	/* Now get Cr */
	PixelAddress = ChromaAddress
							 + Block16x16Address + Offset + 4;
	PixelAddress = (PixelAddress & 0xFFFFFFF0) + (15 - (PixelAddress & 0xF));
    cr = gam_emul_readChar(gam_emul_BLTMemory+PixelAddress) & 0xFF;      /* OUF OUF OUF !!! */

	return (cb | (cr<<16));
}
/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ReadHDpipLuma
 * Parameters: x, y, LumaAddress, Width
 * Return : -
 * Description : read luma from 4.2.X MB HD pip
 * ----------------------------------------------------------------------------------*/

/* 420/422 macroblock format : pixel access functions */

int gam_emul_ReadHDpipLuma(int x,int y,int LumaAddress,int Width)
{
	int Offset,Block16x32Address;
	int PixelAddress,luma;

	Offset = 0;

	Block16x32Address = ((x/16) + ((y/32) * (Width/16))) << 10; /* *1024 each 1024 byte we can find macro-block*/
	if ((y & 0x10) == 0)			/* MB of rows 0,2,4... */
	{
		if (((x&0xF) <8) && ((y&1) == 0)) 	/* MB even row ..Area 1 */
			Offset = 0;
		if (((x&0xF)>=8) && ((y&1) == 0))		/* MB even row ..Area 2 */
			Offset = 64;
		if (((x&0xF) <8) && ((y&1) == 1))		/* MB even row ..Area 3 */
			Offset = 128;
		if (((x&0xF)>=8) && ((y&1) == 1))		/* MB even row ..Area 4 */
			Offset = 192;
	}
	else											/* MB of rows 1,3,5... */
	{
		if (((x&0xF) <8) && ((y&1) == 0))		/* MB odd row  ..Area 1 */
			Offset = 256;
		if (((x&0xF)>=8) && ((y&1) == 0))		/* MB odd row  ..Area 2 */
			Offset = 320;
		if (((x&0xF) <8) && ((y&1) == 1))		/* MB odd row  ..Area 3 */
			Offset = 384;
		if (((x&0xF)>=8) && ((y&1) == 1))		/* MB odd row  ..Area 4 */
			Offset = 448;
	}
	PixelAddress = LumaAddress
							 + Block16x32Address + Offset
							 + (((y & 0xF) >> 1) << 3)
							 + (x & 7);
	/* Now the endianess issue !!! */
	PixelAddress = (PixelAddress & 0xFFFFFFF0) + (15 - (PixelAddress & 0xF));
    luma = gam_emul_readChar(gam_emul_BLTMemory+PixelAddress) & 0xFF;        /* OUF !!! */
	return(luma);
}


/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ReadHDpipCbCr
 * Parameters: x, y,ChromaAddress, Width
 * Return : -
 * Description : return cb and cr from 4:2:X MacroBlock HD pip
 * ----------------------------------------------------------------------------------*/

int gam_emul_ReadHDpipCbCr(int x,int y,int ChromaAddress,int Width)
{
	int Offset,Block16x32Address;
	int PixelAddress,cb, cr;

	Offset = 0;

	Block16x32Address = ((x/8) + ((y/16) * (Width/16))) << 10; /* *1024 each 1024 byte we can find macro-block*/
	if ((y & 0x8) == 0)
	{
		if (((x&0x7) <4) && ((y&1) == 0)) 	/* MB even row ..Area 1 */
			Offset = 0;
		if (((x&0x7)>=4) && ((y&1) == 0))		/* MB even row ..Area 2 */
			Offset = 32;
		if (((x&0x7) <4) && ((y&1) == 1))		/* MB odd row ..Area 3 */
			Offset = 64;
		if (((x&0x7)>=4) && ((y&1) == 1))		/* MB odd row ..Area 4 */
			Offset = 96;
	}
	else
	{
		if (((x&0x7) <4) && ((y&1) == 0))		/* MB even row  ..Area 1 */
			Offset = 256;
		if (((x&0x7)>=4) && ((y&1) == 0))		/* MB even row  ..Area 2 */
			Offset = 288;
		if (((x&0x7) <4) && ((y&1) == 1))		/* MB odd row  ..Area 3 */
			Offset = 320;
		if (((x&0x7)>=4) && ((y&1) == 1))		/* MB odd row  ..Area 4 */
			Offset = 352;
	}

	PixelAddress = ChromaAddress
							 + Block16x32Address + Offset
							 + (((y & 0x7) >> 1) << 3)
							 + ((x & 7) < 4 ? (x & 0x7) : (x & 0x7)-4 ) ;
	/* Get Cb */
	PixelAddress = (PixelAddress & 0xFFFFFFF0) + (15 - (PixelAddress & 0xF));
    cb = gam_emul_readChar(gam_emul_BLTMemory+PixelAddress) & 0xFF;      /* OUF OUF !!! */

	/* Now get Cr */
	PixelAddress -= 4;

	/*PixelAddress = (PixelAddress & 0xFFFFFFF0) + (15 - (PixelAddress & 0xF));*/
    cr = gam_emul_readChar(gam_emul_BLTMemory+PixelAddress) & 0xFF;      /* OUF OUF OUF !!! */

	return (cb | (cr<<16));
}
