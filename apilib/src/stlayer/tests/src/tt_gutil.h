
/**********************************************************************

File Name   : tt_gutil.h

Description : Graphics Object header file

Copyright (C) 2000 STMicroelectronics

Name  Date             Modification
----  ----             ------------
XD    12/07/2000       File creation

**********************************************************************/

#ifndef __TT_GUTIL_H
#define __TT_GUTIL_H

/*---------------------------------------------------------------------
 * Function : GUTIL_InitCommand
 *            Init Testtool command for the Graphics Object
 * Input    : N/A
 * Output   : N/A
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------- */
BOOL GUTIL_InitCommand (void);

/*---------------------------------------------------------------------
 * Function : GUTIL_LoadBitmap
 *            Load a Bitmap and his palette.
 *            Store information into provided structure.
 * Warning  : This function is responsible for the memory allocation of
 *            bitmap and palette buffer. It would first unallocate buffer 
 *            if the provided data pointer are not NULL. Be very carefull
 *            with this !
 * Input    : char *filename, 
 *            STGXOBJ_Bitmap_t *Bitmap_p
 *            STGXOBJ_Palette_t *Palette_p
 * Info     : Following bitmap type are supported :
 *            CLUT8 with ARGB8888 palette,
 *            RGB565, ARGB1555, ARGB4444, YCbCR 4:2:2 Raster
 * Output   : N/A
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_LoadBitmap     ( char *filename, 
                                      STGXOBJ_Bitmap_t *Bitmap_p,
                                      STGXOBJ_Palette_t *Palette_p );

/*---------------------------------------------------------------------
 * Function : GUTIL_SaveBitmap
 *            Save a Bitmap and his palette.
 * Input    : char *filename, name of the file to be created on disk
 *            STGXOBJ_Bitmap_t *Bitmap_p, Bitmap to save
 *            STGXOBJ_Palette_t *Palette_p, Palette to save
 * Info     : Following bitmap type are supported :
 *            CLUT8 with ARGB8888 palette,
 *            RGB565, ARGB1555, ARGB4444, YCbCR 4:2:2 Raster
 * Output   : N/A
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_SaveBitmap     ( char *filename, 
                                      STGXOBJ_Bitmap_t *Bitmap_p,
                                      STGXOBJ_Palette_t *Palette_p );

/*---------------------------------------------------------------------
 * Function : GUTIL_AllocateBitmap
 *            Allocate a Bitmap data space, and set the structure
 * In/out   : STGXOBJ_Bitmap_t *Bitmap_p, Bitmap structure
 *             Field needed : Width, Height, ColorType
 *             Field updated : Pitch, Data1_p, Size1, Data2_p, Size2
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_AllocateBitmap(STGXOBJ_Bitmap_t *Bitmap_p);

/*---------------------------------------------------------------------
 * Function : GUTIL_AllocatePalette
 *            Allocate a Palette data space, and set the structure
 * In/out   : STGXOB_Palette_t *Palette_p, Palette structure
 *             Field needed : ColorType, ColorDepth
 *             Field updated : Data_p
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_AllocatePalette(STGXOBJ_Palette_t *Palette_p);

/*---------------------------------------------------------------------
 * Function : GUTIL_Free
 *            Free a ptr previously allocated with 
 *            GUTIL_AllocateBitmap/Palette. 
 *            Mecanism used was STAVMEM, internaly we manage the
 *            corespondance between ptr and BlockHandlde. 
 * Input    : void *ptr
 * Output   : 
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_Free(void *ptr);

/*---------------------------------------------------------------------
 * Function : GUTIL_ListAll
 *            List all ptr previously allocated with GUTIL_Allocate. 
 *            Mecanism used was STAVMEM, internaly we manage the
 *            corespondance between ptr and BlockHandlde. 
 * Input    : N/A
 * Output   : N/A
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_ListAll(void);

/*---------------------------------------------------------------------
 * Function : GUTIL_MD5Buffer
 *            Calculate the Message Disgest 5 (MD5) of a buffer
 *            Store result into provided buffer.
 * Input    : U8 *buffer_p, input buffer to calculate MD5 
 *            U32 length, length of the buffer to calculate checksum
 * Output   : U8 *digest_p, MD5 will be stored in this U8[16] buffer
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_MD5Buffer(U8 *buffer_p,U32 length, U8 *digest_p);

#endif /* __TT_GUTIL_H */

/* EOF */






