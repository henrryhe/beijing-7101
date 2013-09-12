#ifndef _SHARE_H
#define _SHARE_H

#define gam_emul_readInt(Address_p)   ((int)((U8)(*((U8 *)  (Address_p)))) | \
                                            ((U8)(*(((U8 *) (Address_p)) + 1)) << 8 ) | \
                                            ((U8)(*(((U8 *) (Address_p)) + 2)) << 16) | \
                                            ((U8)(*(((U8 *) (Address_p)) + 3)) << 24))

#define gam_emul_readChar(Address_p)  ((char)(*((char *)  (Address_p))))    /* Be carefull it has to be signed */

#define gam_emul_writeInt(Address_p, Value)                    \
    {                                                          \
        *(((U8 *) (Address_p))    ) = (U8) ((Value));          \
        *(((U8 *) (Address_p)) + 1) = (U8) ((Value) >> 8);     \
        *(((U8 *) (Address_p)) + 2) = (U8) ((Value) >> 16 );   \
        *(((U8 *) (Address_p)) + 3) = (U8) ((Value) >> 24 );   \
    }

#define gam_emul_writeUChar(Address_p, Value)                    \
    {                                                            \
        *((U8 *) (Address_p)) = (U8) (Value);                    \
    }

#define gam_emul_writeShort(Address_p, Value)                         \
    {                                                                 \
        *(((U8 *) (Address_p))    ) = (U8) ((Value));       \
        *(((U8 *) (Address_p)) + 1) = (U8) ((Value) >> 8    );       \
    }


void gam_emul_ConvertYCbCr2RGB (int *y,int *cb,int *cr,int *r,int *g,int *b,int type,int nob_in,int nob_out,int c_format,int matrixNoGfxVideo);
int gam_emul_CheckAddressRange(int Address,int MemorySize);
int gam_emul_swap_16 (int data, int bigNotLittleBitmap);
int gam_emul_swap_24 (int data, int bigNotLittleBitmap);
int gam_emul_swap_32 (int data, int bigNotLittleBitmap);
int gam_emul_Read42XLuma(int x,int y,int LumaAddress,int Width);
int gam_emul_Read42XCbCr(int x,int y,int ChromaAddress,int Width);
int gam_emul_ReadHDpipLuma(int x,int y,int LumaAddress,int Width);
int gam_emul_ReadHDpipCbCr(int x,int y,int ChromaAddress,int Width);

#endif /* _SHARE_H */

