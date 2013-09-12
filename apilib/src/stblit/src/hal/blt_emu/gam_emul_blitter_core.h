#ifndef _BLITTER_CORE_H
#define _BLITTER_CORE_H

#include "blt_init.h"

int gam_emul_ExtractBlitterParameters(int NodeAddress,gam_emul_BlitterParameters *Blt);
void gam_emul_DirectCopy(gam_emul_BlitterParameters *Blt);
void gam_emul_DirectFill(gam_emul_BlitterParameters *Blt);
void gam_emul_GeneralBlitterOperation(stblit_Device_t* Device_p, gam_emul_BlitterParameters *Blt);
int gam_emul_ReadBlitterPixel(int x,int y,gam_emul_BlitterParameters *Blt,int Source);
void gam_emul_WriteBlitterPixel(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix);
void gam_emul_FetchNewSourceLine(int Source,gam_emul_BlitterParameters *Blt,int *SXOut,int *SaveBuffer,int y);
void gam_emul_SamplingPatternConversion42XTo444(gam_emul_BlitterParameters *Blt,int *S1In,int *S2In,int *S2Out,int y);
void gam_emul_ColorSpaceConverter(gam_emul_BlitterParameters *Blt,int *In,int *Out,int InputOutput);
void gam_emul_CLUTOperator(gam_emul_BLTNode* Blitter,gam_emul_BlitterParameters *Blt,int *S2In,int *S2Out,int PostResizer,int y);
void gam_emul_ALUOperator(gam_emul_BlitterParameters *Blt,int *S1In,int *S2In,int *TOut,int y);
void gam_emul_SetWriteEnableMask(gam_emul_BlitterParameters *Blt,int *TMask,int Type,int *Sa,int *Sb,int *Sc,int y);
int gam_emul_RescaleOperator(stblit_Device_t* Device_p, gam_emul_BlitterParameters *Blt,int *S2In,int *S2Out,int *RequestS2Line);
void gam_emul_Newline (int *source,int width);
void gam_emul_VerticalFilter(int* VFilter,int nbtaps,int quantization,int subposition,gam_emul_BlitterParameters *Blt);
int gam_emul_FlickerFilter(int LineNumber,gam_emul_BlitterParameters *Blt);
int gam_emul_GetNewPixel(int width,int x,int *delay);
int gam_emul_HorizontalFilter(int* Hfilter,int nbtaps, int quantization,int subposition,int* delay,gam_emul_BlitterParameters *Blt);
void gam_emul_SamplingPatternConversion444To422(gam_emul_BlitterParameters *Blt,int *TIn,int *TOut);
void gam_emul_WriteTargetLine(gam_emul_BlitterParameters *Blt,int *TIn,int *S1In,int *TMask,int y);
void gam_emul_ClearMaskArray(gam_emul_BlitterParameters *Blt,int *TMask);
void gam_emul_ConvertRGB2YCbCr (int *r,int *g,int *b,int *luma,int *cb,int *cr,int type,int c_format,int nob_in, int matrix_ngfx_vid);
void gam_emul_Write42XLuma(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix);
void gam_emul_Write42XCbCr(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix);
void gam_emul_WriteHDpipLuma(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix);
void gam_emul_WriteHDpipCbCr(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix);
int gam_emul_ReadBlitter42XPixel(int x,int y,gam_emul_BlitterParameters *Blt,int MASK);
int gam_emul_ReadBlitterHDpipPixel(int x,int y,gam_emul_BlitterParameters *Blt,int MASK);
int gam_emul_GetBpp(int Format);
int gam_emul_SearchXYL(int x, int y,gam_emul_BlitterParameters *Blt, int *length, int *color_P);
void gam_emul_XYLBlitterOperation(gam_emul_BlitterParameters *Blt);
void gam_emul_ALUOperatorXYL(gam_emul_BlitterParameters *Blt,int *S1In,int *S2In,int *TOut, int newGlobalAlpha);
void gam_emul_FetchPixel(int source, gam_emul_BlitterParameters *Blt,int *SXOut,int *SaveBuffer,int x, int y);
void gam_emul_WriteTargetPixel(gam_emul_BlitterParameters *Blt,int *TIn,int *S1In,int *TMask,int x, int y);
void gam_emul_SetWriteEnableMaskXYL(gam_emul_BlitterParameters *Blt,int *TMask,int Type,int *Sa,int x, int y);
void gam_emul_ColorSpaceConverterXYL(gam_emul_BlitterParameters *Blt,int *In,int *Out);

#endif /* _BLITTER_CORE_H */
