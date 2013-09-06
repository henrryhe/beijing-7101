/******************************************************************************\
 * File Name : rdecb.c
 * Description:
 *     This module provides an implementation of low-level display primitives
 *     for the display service based on the RDE display device.
 *
 *     See API document for more details.
 * 
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Nunzio Raciti - Dec 2000 
 *
\******************************************************************************/
 
#include <stdlib.h>
#include <time.h>

#include <stddefs.h>
#include <stlite.h>
#include <stcommon.h>
#include <ostime.h>
#include <string.h>
#include <stsubt.h>
#include <stgxobj.h>
 
#include <compbuf.h>
#include <pixbuf.h>
 
#ifdef RDE_DISPLAY_SERVICE 

#define WAIT_MS_IF_RDE_IS_BUSY 200

#include <sttbx.h>
#include "dispserv.h"
#include "rdecb.h"

static __inline ST_ErrorCode_t
STRDE_RegionCreate(STSUBT_InternalHandle_t *,
                   STSUBT_RDE_RegionParams_t*,
                   STSUBT_RDE_RegionId_t *);

ST_ErrorCode_t
STRDE_SetPalette(STSUBT_InternalHandle_t *,
                 STSUBT_RDE_RegionId_t,
                 STGXOBJ_Palette_t *);

ST_ErrorCode_t
STRDE_SetDefaultPalette(STSUBT_InternalHandle_t *,
                        STGXOBJ_Palette_t *);

static __inline ST_ErrorCode_t
STRDE_RegionMove(STSUBT_InternalHandle_t *,
                 STSUBT_RDE_RegionId_t, 
                 U16, U16 );

static __inline ST_ErrorCode_t
STRDE_RegionDelete(STSUBT_InternalHandle_t *,
                   STSUBT_RDE_RegionId_t);

static __inline ST_ErrorCode_t
STRDE_RegionShow(STSUBT_InternalHandle_t *,
                 STSUBT_RDE_RegionId_t);

static __inline ST_ErrorCode_t
STRDE_RegionHide(STSUBT_InternalHandle_t *,
                 STSUBT_RDE_RegionId_t);

static void init_color_4_default_Palette (void);
static void init_color_16_default_Palette (void);
static void init_color_256_default_Palette (void);

static __inline BOOL WaitWhileRdeIsBusy(STSUBT_InternalHandle_t *);
static __inline void ProducerWriteValid(STSUBT_InternalHandle_t *);
static __inline void ProducerWriteResetRequest(STSUBT_InternalHandle_t *);
static __inline void *RdeMemCopy(void *,const void *, size_t);

/* --- Get and register number of clocks per second --- */

static clock_t            clocks_per_sec;
static STSUBT_RDE_Data_t  *RDE_SharedBuffer_p;
static U8                 *RDE_Data_p;
static U8                 ProducerRegionState[RDE_MAX_REGIONS+1];
/*zxg add for store region info*/
typedef struct RegionInfo
{
  boolean UseMyPallete;
  STSUBT_RDE_RegionParams_t       SubRegionPara;
  STGXOBJ_Bitmap_t                SubRegionGfxBmp;
  STGXOBJ_Palette_t               SubRegionPallete;
}MySubtileRegion;

MySubtileRegion ProducerRegionINfo[RDE_MAX_REGIONS+1];
/*******************************/
/* Note: Entry color model is YCbCr (8/8/8) */

static STGXOBJ_ColorUnsignedAYCbCr_t two_bit_entry_default_Palette[4]; 
static STGXOBJ_ColorUnsignedAYCbCr_t four_bit_entry_default_Palette[16];
static STGXOBJ_ColorUnsignedAYCbCr_t eight_bit_entry_default_Palette[256];
static STGXOBJ_ColorUnsignedAYCbCr_t eight_bit_entry_tmp_Palette[256];

static STGXOBJ_Palette_t 	color_4_default_Palette; 
static STGXOBJ_Palette_t 	color_16_default_Palette;
static STGXOBJ_Palette_t 	color_256_default_Palette;
static STGXOBJ_Palette_t 	color_256_tmp_Palette;

/* ------------------------------------------------ */
/* --- Predefined RDE Display Service Structure --- */
/* ------------------------------------------------ */
 
const STSUBT_DisplayService_t STSUBT_RDE_DisplayServiceStructure = {
  STSUBT_RDE_InitializeService,
  STSUBT_RDE_PrepareDisplay,
  STSUBT_RDE_ShowDisplay,
  STSUBT_RDE_HideDisplay,
  STSUBT_RDE_TerminateService,
} ;



/******************************************************************************\
 * Function: init_2_bit_entry_default_Palette,
 *           init_4_bit_entry_default_Palette,
 *           init_8_bit_entry_default_Palette
 * Purpose : Create and Initialize default Palettes
 *           Their entry colors are the default CLUT entries defined into the
 *           subtitling standard.
\******************************************************************************/

static const U16 gamma_values[256] =
{
  16,  23,  28,  32,  36,  39,  42,  45,  48,  50,  53,  55,  58,  60,  62,  64,
  66,  68,  70,  71,  73,  75,  77,  78,  80,  81,  83,  84,  86,  87,  89,  90,
  92,  93,  94,  96,  97,  98, 100, 101, 102, 103, 105, 106, 107, 108, 109, 111,
 112, 113, 114, 115, 116, 117, 118, 119, 121, 122, 123, 124, 125, 126, 127, 128,
 129, 130, 131, 132, 133, 134, 135, 135, 136, 137, 138, 139, 140, 141, 142, 143,
 144, 145, 145, 146, 147, 148, 149, 150, 151, 151, 152, 153, 154, 155, 156, 156,
 157, 158, 159, 160, 160, 161, 162, 163, 164, 164, 165, 166, 167, 167, 168, 169,
 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 179, 179, 180, 181,
 181, 182, 183, 183, 184, 185, 186, 186, 187, 188, 188, 189, 190, 190, 191, 192,
 192, 193, 194, 194, 195, 196, 196, 197, 198, 198, 199, 199, 200, 201, 201, 202,
 203, 203, 204, 204, 205, 206, 206, 207, 208, 208, 209, 209, 210, 211, 211, 212,
 212, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 220, 220, 221, 221,
 222, 222, 223, 224, 224, 225, 225, 226, 226, 227, 228, 228, 229, 229, 230, 230,
 231, 231, 232, 233, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238, 239,
 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247,
 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255, 255
};

#define Y_RED_MULTIPLIER         66
#define Y_GREEN_MULTIPLIER      129
#define Y_BLUE_MULTIPLIER        25

#define CR_RED_MULTIPLIER       112
#define CR_GREEN_MULTIPLIER     -94
#define CR_BLUE_MULTIPLIER      -18

#define CB_RED_MULTIPLIER       -38
#define CB_GREEN_MULTIPLIER     -74
#define CB_BLUE_MULTIPLIER      112

/* Convert from RGB to YCbCr (8,8,8) */

static __inline
void RGB_2_YCrCb (U8 r, U8 g, U8 b, U8 *Y, U8 *Cr, U8 *Cb)
{
   S32 y, cb, cr;

   y  = ((Y_RED_MULTIPLIER    * gamma_values[r]) +
         (Y_GREEN_MULTIPLIER  * gamma_values[g]) +
         (Y_BLUE_MULTIPLIER   * gamma_values[b])) >> 8;
   cr = ((CR_RED_MULTIPLIER   * gamma_values[r]) +
         (CR_GREEN_MULTIPLIER * gamma_values[g]) +
         (CR_BLUE_MULTIPLIER  * gamma_values[b])) >> 8;
   cb = ((CB_RED_MULTIPLIER   * gamma_values[r]) +
         (CB_GREEN_MULTIPLIER * gamma_values[g]) +

     (CB_BLUE_MULTIPLIER  * gamma_values[b])) >> 8;
   *Y = ( 16 + (U8)y ) ;
   *Cr = (128 + (U8)cr) ;
   *Cb = (128 + (U8)cb) ;
}

/******************************************************************************\
 * Function: 
 *           
 *           
 * Purpose : Create and Initialize default Palettes
 *           Their entry colors are the default CLUT entries defined into the 
 *           subtitling standard.
\******************************************************************************/
 

static void init_color_4_default_Palette (void)
{
 U8  Y, Cr, Cb;

  color_4_default_Palette.ColorType  	 = STGXOBJ_COLOR_TYPE_CLUT2; 
  color_4_default_Palette.ColorDepth  	 = 2 ; 
  color_4_default_Palette.PaletteType 	= 
								STGXOBJ_PALETTE_TYPE_DEVICE_DEPENDENT; 
  color_4_default_Palette.Data_p		= two_bit_entry_default_Palette;

  RGB_2_YCrCb (0x00, 0x00, 0x00, &Y, &Cr, &Cb);
 
  two_bit_entry_default_Palette[0].Alpha = 0x00;
  two_bit_entry_default_Palette[0].Y = Y;
  two_bit_entry_default_Palette[0].Cr = Cr;
  two_bit_entry_default_Palette[0].Cb = Cb;

  RGB_2_YCrCb (0xff, 0xff, 0xff, &Y, &Cr, &Cb);

  two_bit_entry_default_Palette[1].Alpha = 0x3f;
  two_bit_entry_default_Palette[1].Y = Y;
  two_bit_entry_default_Palette[1].Cr = Cr;
  two_bit_entry_default_Palette[1].Cb = Cb;

  RGB_2_YCrCb (0x00, 0x00, 0x00, &Y, &Cr, &Cb);

  two_bit_entry_default_Palette[2].Alpha = 0x3f;
  two_bit_entry_default_Palette[2].Y = Y;
  two_bit_entry_default_Palette[2].Cr = Cr;
  two_bit_entry_default_Palette[2].Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x7f, 0x7f, &Y, &Cr, &Cb);

  two_bit_entry_default_Palette[3].Alpha = 0x3f;
  two_bit_entry_default_Palette[3].Y = Y;
  two_bit_entry_default_Palette[3].Cr = Cr;
  two_bit_entry_default_Palette[3].Cb = Cb;


}

/* 16-entry default Palette */

static void init_color_16_default_Palette (void)
{
	U8  Y, Cr, Cb;


  color_16_default_Palette.ColorType   = STGXOBJ_COLOR_TYPE_CLUT4; 
  color_16_default_Palette.ColorDepth   = 4; 
  color_16_default_Palette.PaletteType 	= STGXOBJ_PALETTE_TYPE_DEVICE_DEPENDENT; 
  color_16_default_Palette.Data_p		= four_bit_entry_default_Palette;
 
  RGB_2_YCrCb (0x00, 0x00, 0x00, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[0].Alpha = 0x0;
  four_bit_entry_default_Palette[0].Y = Y;
  four_bit_entry_default_Palette[0].Cr = Cr;
  four_bit_entry_default_Palette[0].Cb = Cb;

  RGB_2_YCrCb (0xff, 0x00, 0x00, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[1].Alpha = 0x3f;
  four_bit_entry_default_Palette[1].Y = Y;
  four_bit_entry_default_Palette[1].Cr = Cr;
  four_bit_entry_default_Palette[1].Cb = Cb;

  RGB_2_YCrCb (0x00, 0xff, 0x00, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[2].Alpha = 0x3f;
  four_bit_entry_default_Palette[2].Y = Y;
  four_bit_entry_default_Palette[2].Cr = Cr;
  four_bit_entry_default_Palette[2].Cb = Cb;

  RGB_2_YCrCb (0xff, 0xff, 0x00, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[3].Alpha = 0x3f;
  four_bit_entry_default_Palette[3].Y = Y;
  four_bit_entry_default_Palette[3].Cr = Cr;
  four_bit_entry_default_Palette[3].Cb = Cb;

  RGB_2_YCrCb (0x00, 0x00, 0xff, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[4].Alpha = 0x3f;
  four_bit_entry_default_Palette[4].Y = Y;
  four_bit_entry_default_Palette[4].Cr = Cr;
  four_bit_entry_default_Palette[4].Cb = Cb;

  RGB_2_YCrCb (0xff, 0x00, 0xff, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[5].Alpha = 0x3f;
  four_bit_entry_default_Palette[5].Y = Y;
  four_bit_entry_default_Palette[5].Cr = Cr;
  four_bit_entry_default_Palette[5].Cb = Cb;

  RGB_2_YCrCb (0x00, 0xff, 0xff, &Y, &Cr, &Cb);
  four_bit_entry_default_Palette[6].Alpha = 0x3f;
  four_bit_entry_default_Palette[6].Y = Y;
  four_bit_entry_default_Palette[6].Cr = Cr;
  four_bit_entry_default_Palette[6].Cb = Cb;

  RGB_2_YCrCb (0xff, 0xff, 0xff, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[7].Alpha = 0x3f;
  four_bit_entry_default_Palette[7].Y = Y;
  four_bit_entry_default_Palette[7].Cr = Cr;
  four_bit_entry_default_Palette[7].Cb = Cb;

  RGB_2_YCrCb (0x00, 0x00, 0x00, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[8].Alpha = 0x3f;
  four_bit_entry_default_Palette[8].Y = Y;
  four_bit_entry_default_Palette[8].Cr = Cr;
  four_bit_entry_default_Palette[8].Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x00, 0x00, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[9].Alpha = 0x3f;
  four_bit_entry_default_Palette[9].Y = Y;
  four_bit_entry_default_Palette[9].Cr = Cr;
  four_bit_entry_default_Palette[9].Cb = Cb;

  RGB_2_YCrCb (0x00, 0x7f, 0x00, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[10].Alpha = 0x3f;
  four_bit_entry_default_Palette[10].Y = Y;
  four_bit_entry_default_Palette[10].Cr = Cr;
  four_bit_entry_default_Palette[10].Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x7f, 0x00, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[11].Alpha = 0x3f;
  four_bit_entry_default_Palette[11].Y = Y;
  four_bit_entry_default_Palette[11].Cr = Cr;
  four_bit_entry_default_Palette[11].Cb = Cb;

  RGB_2_YCrCb (0x00, 0x00, 0x7f, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[12].Alpha = 0x3f;
  four_bit_entry_default_Palette[12].Y = Y;
  four_bit_entry_default_Palette[12].Cr = Cr;
  four_bit_entry_default_Palette[12].Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x00, 0x7f, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[13].Alpha = 0x3f;
  four_bit_entry_default_Palette[13].Y = Y;
  four_bit_entry_default_Palette[13].Cr = Cr;
  four_bit_entry_default_Palette[13].Cb = Cb;

  RGB_2_YCrCb (0x00, 0x7f, 0x7f, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[14].Alpha = 0x3f;
  four_bit_entry_default_Palette[14].Y = Y;
  four_bit_entry_default_Palette[14].Cr = Cr;
  four_bit_entry_default_Palette[14].Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x7f, 0x7f, &Y, &Cr, &Cb);

  four_bit_entry_default_Palette[15].Alpha = 0x3f;
  four_bit_entry_default_Palette[15].Y = Y;
  four_bit_entry_default_Palette[15].Cr = Cr;
  four_bit_entry_default_Palette[15].Cb = Cb;


}

/* 256-entry default Palette */

static void init_color_256_default_Palette (void)
{
  U32 i;
  U8  r, g, b;
  U8  Y, Cr, Cb;


  color_256_default_Palette.ColorType   	= STGXOBJ_COLOR_TYPE_CLUT8; 
  color_256_default_Palette.ColorDepth   	= 8; 
  color_256_default_Palette.PaletteType 	= STGXOBJ_PALETTE_TYPE_DEVICE_DEPENDENT; 
  color_256_default_Palette.Data_p		= eight_bit_entry_default_Palette;


   RGB_2_YCrCb (0, 0, 0, &Y, &Cr, &Cb); 

  eight_bit_entry_default_Palette[0].Alpha = 0x00;
  eight_bit_entry_default_Palette[0].Y = Y;
  eight_bit_entry_default_Palette[0].Cr = Cr;
  eight_bit_entry_default_Palette[0].Cb = Cb;

  for (i = 1; i <= 7; i++)
  {
      r = ((i & 0x1) != 0) * 0xff;
      g = ((i & 0x2) != 0) * 0xff;
      b = ((i & 0x4) != 0) * 0xff;

      RGB_2_YCrCb (r, g, b, &Y, &Cr, &Cb);


  	  eight_bit_entry_default_Palette[i].Alpha = 0x2f;
  	  eight_bit_entry_default_Palette[i].Y = Y;
  	  eight_bit_entry_default_Palette[i].Cr = Cr;
  	  eight_bit_entry_default_Palette[i].Cb = Cb;

  }

  for (i = 8; i <= 0x7f; i++)
  {
      r = (((i & 0x1) != 0) * 0x54 + ((i & 0x10) != 0) * 0xaa);
      g = (((i & 0x2) != 0) * 0x54 + ((i & 0x20) != 0) * 0xaa);
      b = (((i & 0x4) != 0) * 0x54 + ((i & 0x40) != 0) * 0xaa);
 	  RGB_2_YCrCb (r, g, b, &Y, &Cr, &Cb);


  	  eight_bit_entry_default_Palette[i].Alpha = (0x3f - (((i & 0x8) != 0) * 0x1f));
  	  eight_bit_entry_default_Palette[i].Y = Y;
  	  eight_bit_entry_default_Palette[i].Cr = Cr;
  	  eight_bit_entry_default_Palette[i].Cb = Cb;

  }

  for (i = 0x80; i <= 0xff; i++)
  {
      U8 delta = ((i & 0x8) == 0) * 0x7f;	/* + 50% */
 
      r = (((i & 0x1) != 0) * 0x2a + ((i & 0x10) != 0) * 0x54) + delta;
      g = (((i & 0x2) != 0) * 0x2a + ((i & 0x20) != 0) * 0x54) + delta;
      b = (((i & 0x4) != 0) * 0x2a + ((i & 0x40) != 0) * 0x54) + delta;
 	  RGB_2_YCrCb (r, g, b, &Y, &Cr, &Cb);

  	  eight_bit_entry_default_Palette[i].Alpha = 0x3f;
  	  eight_bit_entry_default_Palette[i].Y = Y;
  	  eight_bit_entry_default_Palette[i].Cr = Cr;
  	  eight_bit_entry_default_Palette[i].Cb = Cb;

  }
}


/******************************************************************************\
 * Function: UpdateRegionPalette
 * Purpose : Update Region palette entries, but only if their contents
 *           changed (entry version == palette version)
 * Return  : 
\******************************************************************************/
 
static ST_ErrorCode_t
UpdateRegionPalette (STSUBT_InternalHandle_t *DecoderHandle,
                     STSUBT_RDE_RegionId_t RegionHandle, 
                     CLUT_t *region_clut, 
                     U8 region_depth)
{
  CLUT_Entry_t *SubT_CLUT;
  U16 NumEntries, nEntry;
  U32 Entry;
  BOOL default_flag;
  ST_ErrorCode_t exit_code = ST_NO_ERROR, res;
  S16  CLUT_version_number = region_clut->CLUT_version_number;

  /* choose CLUT in the CLUT family, depending on region_depth */

  switch (region_depth) {
    case 2:  SubT_CLUT = region_clut->two_bit_entry_CLUT_p;
             default_flag = region_clut->two_bit_entry_CLUT_flag;
             color_256_tmp_Palette.ColorType   	= STGXOBJ_COLOR_TYPE_CLUT2; 
             NumEntries = 4;
             break;
    case 4:  SubT_CLUT = region_clut->four_bit_entry_CLUT_p;
             default_flag = region_clut->four_bit_entry_CLUT_flag;
             color_256_tmp_Palette.ColorType   	= STGXOBJ_COLOR_TYPE_CLUT4; 
             NumEntries = 16;
             break;
    case 8:  SubT_CLUT = region_clut->eight_bit_entry_CLUT_p;
             default_flag = region_clut->eight_bit_entry_CLUT_flag;
             color_256_tmp_Palette.ColorType   	= STGXOBJ_COLOR_TYPE_CLUT8; 
             NumEntries = 256;
             break;
  }

  /* if the CLUT is a default CLUT then nothing needs to be updated */

  if (default_flag == TRUE)
  {
      return (ST_NO_ERROR);
  }

  color_256_tmp_Palette.ColorDepth   	= region_depth; 
  
  for (nEntry = 0; nEntry < NumEntries; nEntry++)
  {
      /* check if CLUT entry has been changed 
       * i.e. entry_version_number == CLUT_version_number 
       */
      if (SubT_CLUT[nEntry].entry_version_number == CLUT_version_number)
      {
          U8 Y, Cr, Cb, T;
		 /*  STGXOBJ_ColorUnsignedAYCbCr_t Color;  */

          STTBX_Report((STTBX_REPORT_LEVEL_USER1,
					"#### updating entry %d\n", nEntry));

          /* get YCrCb components and transparency values */

          Entry  = SubT_CLUT[nEntry].entry_color_value; 

          Y  = (Entry & 0xff000000) >> 24; 
          Cr = (Entry & 0x00ff0000) >> 16; 
          Cb = (Entry & 0x0000ff00) >>  8; 
          T  = (Entry & 0x000000ff); 
       
          /* set new palette color */

  	      eight_bit_entry_tmp_Palette[nEntry].Alpha = 0x3f - (T >> 2);
  	      eight_bit_entry_tmp_Palette[nEntry].Y = Y;
  	      eight_bit_entry_tmp_Palette[nEntry].Cr = Cr;
  	      eight_bit_entry_tmp_Palette[nEntry].Cb = Cb;

      }
      else
      {

  	      eight_bit_entry_tmp_Palette[nEntry].Alpha = 
  	        four_bit_entry_default_Palette[nEntry].Alpha;  
  	      eight_bit_entry_tmp_Palette[nEntry].Y = 
  	        four_bit_entry_default_Palette[nEntry].Y;  
  	      eight_bit_entry_tmp_Palette[nEntry].Cr = 
  	        four_bit_entry_default_Palette[nEntry].Cr;  
  	      eight_bit_entry_tmp_Palette[nEntry].Cb = 
  	        four_bit_entry_default_Palette[nEntry].Cb;  
      }
  }

  res=STRDE_SetPalette(DecoderHandle,RegionHandle,
                       &color_256_tmp_Palette);

  if (res != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
        "###UpdateRegionPalette:STRDE_SetPalette, %d",res));
       exit_code = res;
  }

  return (exit_code);
}



/******************************************************************************\
 * Function: GetSubtRegionList
 * Return  : List of (subt) regions visible in a page composition,
 *           NULL if it is empty.
\******************************************************************************/
 
static __inline
VisibleRegion_t *GetSubtRegionList (PCS_t *PageComposition)
{
  return (PageComposition->VisibleRegion_p);
}


/******************************************************************************\
 * Function: GetNextSubtRegionInfo
 * Purpose : Get information about next (subt) regions visible in a 
 *           page composition,
 * Parameters:
 *      region_id:
 *           returned region id
 *      horizontal_position:
 *           returned region horizontal_position
 *      vertical_position:
 *           returned region vertical_position
 *      region_width:
 *           returned region width
 *      region_height:
 *           returned region height
 *      region_depth:
 *           returned region depth (number of bits per pixel)
 *      region_version:
 *           returned region version
 *      region_clut:
 *           returned pointer to region CLUT descriptor
 *      region_bitmap:
 *           returned pointer to region bitmap
 * Return  : ST_NO_ERROR on success, 1 in case no subt region is present
\******************************************************************************/
 
static __inline
ST_ErrorCode_t GetNextSubtRegionInfo (U8     *region_id,
                                      S32    *horizontal_position,
                                      S32    *vertical_position,
                                      U32    *region_width,
                                      U32    *region_height,
                                      U8     *region_depth,
                                      U8     *region_version,
                                      CLUT_t          **region_clut,
                                      STSUBT_Bitmap_t **region_bitmap,
                                      VisibleRegion_t **SubtRegionList)
{
  VisibleRegion_t *SubtRegion = *SubtRegionList;

  if (SubtRegion == NULL) 
  {
      return (1);
  }

  *region_id = SubtRegion->region_id;
  *horizontal_position = SubtRegion->horizontal_position;
  *vertical_position = SubtRegion->vertical_position;
  *region_width = (SubtRegion->region_p)->region_width;
  *region_height = (SubtRegion->region_p)->region_height;
  *region_depth = (SubtRegion->region_p)->region_depth;
  *region_version = (SubtRegion->region_p)->region_version_number;
  *region_clut = (SubtRegion->region_p)->CLUT_p;
  *region_bitmap = (STSUBT_Bitmap_t*)((SubtRegion->region_p)->region_pixel_p);

  *SubtRegionList = (*SubtRegionList)->next_VisibleRegion_p;

  return (ST_NO_ERROR);
}

/******************************************************************************\
 * Function: GetEpochRegionList
 * Return  : List of (subt) epoch regions list
 *           NULL if it is empty.
\******************************************************************************/

static __inline
RCS_t *GetEpochRegionList (PCS_t *PageComposition)
{
  return (PageComposition->EpochRegionList);
}


/******************************************************************************\
 * Function: GetNextEpochRegionInfo
 * Purpose : Get information about next (subt) epoch regions visible in a
 *           page composition,
 * Parameters:
 *      region_id:
 *           returned region id
 *      horizontal_position:
 *           returned region horizontal_position
 *      vertical_position:
 *           returned region vertical_position
 *      region_width:
 *           returned region width
 *      region_height:
 *           returned region height
 *      region_depth:
 *           returned region depth (number of bits per pixel)
 *      region_version:
 *           returned region version
 *      region_clut:
 *           returned pointer to region CLUT descriptor
 *      region_bitmap:
 *           returned pointer to region bitmap
 * Return  : ST_NO_ERROR on success, 1 in case no subt region is present
\******************************************************************************/

static __inline
ST_ErrorCode_t GetNextEpochRegionInfo (U8              *region_id,
                                      S32              *horizontal_position,
                                      S32              *vertical_position,
                                      U32              *region_width,
                                      U32              *region_height,
                                      U8               *region_depth,
                                      U8               *region_version,
                                      CLUT_t          **region_clut,
                                      STSUBT_Bitmap_t **region_bitmap,
                                      RCS_t           **EpochRegionList)
{
  RCS_t *SubtRegion = *EpochRegionList;

  if (SubtRegion == NULL)
  {
      return (1);
  }

  *region_id = SubtRegion->region_id;
  *horizontal_position = 0;
  *vertical_position = 0;
  *region_width = SubtRegion->region_width;
  *region_height = SubtRegion->region_height;
  *region_depth = SubtRegion->region_depth;
  *region_version = SubtRegion->region_version_number;
  *region_clut = SubtRegion->CLUT_p;
  *region_bitmap = (STSUBT_Bitmap_t*)(SubtRegion->region_pixel_p);

  *EpochRegionList = (RCS_t*)(*EpochRegionList)->next_region_p;

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: NewServiceHandle
 * Purpose : Create a Display Service Handle
 * Parameters:
 *      MemoryPartition: 
 *           Partition from which memory will be allocated.
 *      num_descriptors:
 *           Max num of storable display descriptors
 *      region_per_epoch:
 *           Max number of regions storable for each epoch
 *      region_per_display:
 *           estimated max number of visible regions per display
 * Return  : A pointer to created Service Handle structure, NULL on error
\******************************************************************************/
 
static __inline
STSUBT_RDE_ServiceHandle_t *NewServiceHandle (ST_Partition_t *MemoryPartition,
                            U32 num_descriptors,
                            U32 region_per_epoch,
                            U32 region_per_display)
{
    STSUBT_RDE_ServiceHandle_t *ServiceHandle;
    U32 BufferSize;
    U32 section_1_size;
    U32 section_2_size;
    U32 section_3_size;
    U32 section_4_size;
    U32 section_5_size;
 
    /* Alloc memory for the handle and initialize structure  */
 
    ServiceHandle = (STSUBT_RDE_ServiceHandle_t*)
        memory_allocate(MemoryPartition, sizeof(STSUBT_RDE_ServiceHandle_t));

    if (ServiceHandle == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"#### NewServiceHandle: no space **\n"));
        return (NULL);
    }
 
    /* --- register MemoryPartition --- */

    ServiceHandle->MemoryPartition = MemoryPartition;

    /* --- Allocate DisplaySemaphore --- */
 
    ServiceHandle->DisplaySemaphore = 
        (semaphore_t *)memory_allocate(MemoryPartition, sizeof(semaphore_t));

    if (ServiceHandle->DisplaySemaphore == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"#### NewServiceHandle: error creating semaphore **\n"));
        memory_deallocate(MemoryPartition, ServiceHandle);
        return(NULL);
    }

    semaphore_init_fifo(ServiceHandle->DisplaySemaphore, 1);

    /* Alloc memory buffer */
 
    section_1_size = num_descriptors * sizeof(STSUBT_RDE_DisplayDescriptor_t);
    section_2_size = region_per_epoch * sizeof(STSUBT_RDE_RegionDescriptor_t);
    section_3_size = num_descriptors * region_per_display 
                   * sizeof(STSUBT_RDE_DisplayRegion_t);
    section_4_size = region_per_epoch * sizeof(STSUBT_RDE_RegionId_t);
    section_5_size = region_per_epoch * sizeof(STSUBT_RDE_PaletteDescriptor_t);

    BufferSize = section_1_size + section_2_size 
               + section_3_size + section_4_size + section_5_size;

    if ((ServiceHandle->Buffer_p = memory_allocate(MemoryPartition,
                                                   BufferSize)) == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"#### NewServiceHandle: no space **\n"));
        semaphore_delete (ServiceHandle->DisplaySemaphore);        
        memory_deallocate(MemoryPartition, ServiceHandle->DisplaySemaphore);
        memory_deallocate(MemoryPartition, ServiceHandle);
        return (NULL);
    }
 
    /* initialize the Service Handle structure */
 
    ServiceHandle->base_section_1 = ServiceHandle->Buffer_p;
    ServiceHandle->free_section_1_p = 
        (STSUBT_RDE_DisplayDescriptor_t*)ServiceHandle->base_section_1;
    ServiceHandle->MaxNumDescriptors = num_descriptors;
    ServiceHandle->nDescriptorsAllocated = 0;

    ServiceHandle->base_section_2 = ServiceHandle->base_section_1 
                                  + section_1_size;
    ServiceHandle->free_section_2_p = 
        (STSUBT_RDE_RegionDescriptor_t*)ServiceHandle->base_section_2;
    ServiceHandle->MaxNumRegions = region_per_epoch;
    ServiceHandle->nRegionsAllocated = 0;

    ServiceHandle->base_section_3 = ServiceHandle->base_section_2 
                                  + section_2_size;
    ServiceHandle->free_section_3_p = 
        (STSUBT_RDE_DisplayRegion_t*)ServiceHandle->base_section_3;
    ServiceHandle->MaxNumDisplayRegions = region_per_display;
    ServiceHandle->nDisplayRegionsAllocated = 0;

    ServiceHandle->base_section_4 = ServiceHandle->base_section_3
                                  + section_3_size;
    ServiceHandle->free_section_4_p =
        (STSUBT_RDE_RegionId_t*)ServiceHandle->base_section_4;
    ServiceHandle->MaxNumRegionHandlers = region_per_epoch;
    ServiceHandle->nRegionsHandlersAllocated = 0;

    ServiceHandle->base_section_5 = ServiceHandle->base_section_4
                                  + section_4_size;
    ServiceHandle->free_section_5_p =
        (STSUBT_RDE_PaletteDescriptor_t*)ServiceHandle->base_section_5;
    ServiceHandle->MaxNumPaletteDescriptors = region_per_epoch;
    ServiceHandle->nPaletteDescriptorsAllocated = 0;

    ServiceHandle->Starting = TRUE;
    ServiceHandle->ActiveDisplayDescriptor = NULL;
    ServiceHandle->EpochRegionList = NULL;
    ServiceHandle->EpochPaletteList = NULL;
 
    return (ServiceHandle);
}


/******************************************************************************\
 * Function: ResetServiceHandle
 * Purpose : Reset the contents of Service Handle
\******************************************************************************/
 
static 
void ResetServiceHandle (STSUBT_RDE_ServiceHandle_t *ServiceHandle)
{
    ServiceHandle->free_section_1_p =
        (STSUBT_RDE_DisplayDescriptor_t*)ServiceHandle->base_section_1;
    ServiceHandle->nDescriptorsAllocated = 0;
 
    ServiceHandle->free_section_2_p =
        (STSUBT_RDE_RegionDescriptor_t*)ServiceHandle->base_section_2;
    ServiceHandle->nRegionsAllocated = 0;
 
    ServiceHandle->free_section_3_p =
        (STSUBT_RDE_DisplayRegion_t*)ServiceHandle->base_section_3;
    ServiceHandle->nDisplayRegionsAllocated = 0;
 
    ServiceHandle->free_section_4_p =
        (STSUBT_RDE_RegionId_t*)ServiceHandle->base_section_4;
    ServiceHandle->nRegionsHandlersAllocated = 0;

    ServiceHandle->free_section_5_p =
        (STSUBT_RDE_PaletteDescriptor_t*)ServiceHandle->base_section_5;
    ServiceHandle->nPaletteDescriptorsAllocated = 0;

    ServiceHandle->ActiveDisplayDescriptor = NULL;
    ServiceHandle->EpochRegionList = NULL;
    ServiceHandle->EpochPaletteList = NULL;
}
 

/******************************************************************************\
 * Function: DeleteServiceHandle
 * Purpose : Delete the display service handle
 *           All allocated resources are freed.
\******************************************************************************/
 
static void DeleteServiceHandle(STSUBT_RDE_ServiceHandle_t *ServiceHandle)
{
    ST_Partition_t *MemoryPartition = ServiceHandle->MemoryPartition;
    memory_deallocate(MemoryPartition, ServiceHandle->Buffer_p);
    semaphore_delete (ServiceHandle->DisplaySemaphore);    
    memory_deallocate(MemoryPartition, ServiceHandle->DisplaySemaphore);
    memory_deallocate(MemoryPartition, ServiceHandle);
}


/******************************************************************************\
 * Function: NewDisplayDescriptor
 * Purpose : Allocate space for a new Display Descriptor 
 * Return  : A pointer to allocated Display Descriptor (always successes)
\******************************************************************************/
 
static __inline STSUBT_RDE_DisplayDescriptor_t * 
NewDisplayDescriptor (STSUBT_RDE_ServiceHandle_t *ServiceHandle)
{
    STSUBT_RDE_DisplayDescriptor_t *descriptor_p;
 
    /* Get a descriptor from ServiceHandle (section 1) */
 
    descriptor_p = ServiceHandle->free_section_1_p;

    /* update control information */

    ServiceHandle->nDescriptorsAllocated = 
    (ServiceHandle->nDescriptorsAllocated+1) % ServiceHandle->MaxNumDescriptors;

    if (ServiceHandle->nDescriptorsAllocated == 0)
    {
        ServiceHandle->free_section_1_p = 
            (STSUBT_RDE_DisplayDescriptor_t*)ServiceHandle->base_section_1;
    }
    else
    {
        ServiceHandle->free_section_1_p = ServiceHandle->free_section_1_p + 1;
    }

    /* initialize the descriptor */
 
    descriptor_p->RegionList = NULL;
    descriptor_p->TailRegionList = NULL;
 
    return (descriptor_p);
}
 

/******************************************************************************\
 * Function: AddToPaletteList
 * Purpose : Allocate space for a new Palette Descriptor
 *           and include it into the EpochPaletteList
 *           Unlike SubT CLUTs, which can be associated to many regions
 *           palettes are associated to a unique region. So palettes
 *           will be identified throught the RDE region handle (U32 value).
 * Return  : A pointer to allocated Palette Descriptor (always successes)
 * Note    : Palette Descriptors are just use to optimize palette entry updates
\******************************************************************************/
 
static STSUBT_RDE_PaletteDescriptor_t *
AddToPaletteList ( STSUBT_RDE_RegionId_t RegionHandle, 
                  STSUBT_RDE_ServiceHandle_t *ServiceHandle)
{

    STSUBT_RDE_PaletteDescriptor_t *PaletteDescriptor;
 
    /* Get a Palette descriptor from ServiceHandle (section 5) */
 
    PaletteDescriptor = ServiceHandle->free_section_5_p;
 
    /* update control information */
 
    ServiceHandle->nPaletteDescriptorsAllocated =
      (ServiceHandle->nPaletteDescriptorsAllocated+1) 
        % ServiceHandle->MaxNumPaletteDescriptors;
 
    if (ServiceHandle->nPaletteDescriptorsAllocated == 0)
    {
        ServiceHandle->free_section_5_p =
            (STSUBT_RDE_PaletteDescriptor_t*)ServiceHandle->base_section_5;
    }
    else
    {
        ServiceHandle->free_section_5_p = ServiceHandle->free_section_5_p + 1;
    }
 
    /* initialize the descriptor */
 
    PaletteDescriptor->Palette_id = RegionHandle;
    PaletteDescriptor->Palette_version = -1;	/* any palette will be new */

    /* insert PaletteDescriptor in EpochPaletteList */
 
    PaletteDescriptor->NextPalette_p = (void*)ServiceHandle->EpochPaletteList;
    ServiceHandle->EpochPaletteList = PaletteDescriptor;

    return (PaletteDescriptor);
}
 
                                                    
/******************************************************************************\
 * Function: GetPaletteDescriptor
 * Purpose : Try to retrieve corresponding PaletteDescriptor into the 
 *           EpochPaletteList
 *           Palettes are identified by the RDE region handle (U32 value).
 * Return  : A pointer to found PaletteDescriptor, NULL if PaletteDescriptor 
 *           was not found
\******************************************************************************/
 
static __inline STSUBT_RDE_PaletteDescriptor_t *
GetPaletteDescriptor (STSUBT_RDE_RegionId_t RegionHandle,
                      STSUBT_RDE_ServiceHandle_t *ServiceHandle)
{
  STSUBT_RDE_PaletteDescriptor_t *EpochPaletteList;
 
  EpochPaletteList = ServiceHandle->EpochPaletteList;
  while (EpochPaletteList != NULL)
  {
      if (EpochPaletteList->Palette_id == RegionHandle)
          return (EpochPaletteList);
      EpochPaletteList = EpochPaletteList->NextPalette_p;
  }
  return (EpochPaletteList);
}
 

/******************************************************************************\
 * Function: NewRDERegion
 * Purpose : Allocate a new RDE Region and a new Region Descriptor including 
 *           it in the EpochRegionList
 * Parameters:
 * Return  : 
\******************************************************************************/
 
static STSUBT_RDE_RegionId_t
NewRDERegion ( U8 region_id, 
              STSUBT_RDE_RegionParams_t *RegionParams,
              STSUBT_RDE_ServiceHandle_t *ServiceHandle,
              STSUBT_InternalHandle_t *DecoderHandle)
{
  STSUBT_RDE_RegionDescriptor_t *RegionDescriptor;
  STSUBT_RDE_RegionId_t         *RegionHandle;
 
  /* Allocate an RDE RegionId_t from ServiceHandle (section 4) */
 
  RegionHandle = ServiceHandle->free_section_4_p;

  /* update control information */

  ServiceHandle->nRegionsHandlersAllocated =
      (ServiceHandle->nRegionsHandlersAllocated+1) 
          % ServiceHandle->MaxNumRegionHandlers;

  if (ServiceHandle->nRegionsHandlersAllocated == 0)
  {
      ServiceHandle->free_section_4_p =
          (STSUBT_RDE_RegionId_t*)ServiceHandle->base_section_4;
  }
  else
  {
      ServiceHandle->free_section_4_p =
          ServiceHandle->free_section_4_p + 1;
  }
  
  /* --- Create RDE region --- */

  if (STRDE_RegionCreate(DecoderHandle, RegionParams,
                          RegionHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
		"#### NewRDERegion: Error creating RDE region (%d)",res));
      return (0);
  }

  /* --- Allocate a Region Item from section 2 --- */
 
  RegionDescriptor = ServiceHandle->free_section_2_p;
 
  /* --- Update control information --- */

  ServiceHandle->nRegionsAllocated =
      (ServiceHandle->nRegionsAllocated + 1) % ServiceHandle->MaxNumRegions;
 
  if (ServiceHandle->nRegionsAllocated == 0)
  {
      ServiceHandle->free_section_2_p = 
           (STSUBT_RDE_RegionDescriptor_t*)ServiceHandle->base_section_2;
  }
  else 
  {
      ServiceHandle->free_section_2_p = ServiceHandle->free_section_2_p + 1;
  }

  /* --- Fill in RegionDescriptor data structure --- */
 
  RegionDescriptor->region_id = region_id;
  RegionDescriptor->old_vertical_position=0; 
  RegionDescriptor->old_horizontal_position=0; 
  RegionDescriptor->region_version = -1;
  RegionDescriptor->RegionHandle = RegionHandle;

  /* insert RegionDescriptor in EpochRegionList */

  RegionDescriptor->NextRegion_p = (void*)ServiceHandle->EpochRegionList;
  ServiceHandle->EpochRegionList = RegionDescriptor;

  return (*RegionHandle);
}


/******************************************************************************\
 * Function: GetRegionDescriptor
 * Purpose : Try to retrieve a RegionDescriptor into the EpochRegionList
 * Return  : A pointer to found RegionDescriptor, NULL if RegionDescriptor was not found
\******************************************************************************/
 
static __inline STSUBT_RDE_RegionDescriptor_t *
GetRegionDescriptor (U8 region_id, STSUBT_RDE_ServiceHandle_t *ServiceHandle)
{
  STSUBT_RDE_RegionDescriptor_t *EpochRegionList;
 
  EpochRegionList = ServiceHandle->EpochRegionList;
  while (EpochRegionList != NULL)
  {
      if (EpochRegionList->region_id == region_id)
          return (EpochRegionList);
      EpochRegionList = EpochRegionList->NextRegion_p;
  }
  return (EpochRegionList);
}
 

/******************************************************************************\
 * Function: AddRegionToDisplay
 * Purpose : Allocate a new visible region item from the service handle,
 *           including it in the descriptor RegionList
 *           Corresponding Region item is linked from the region item.
 * Return  : Nom zero if region item was not found
\******************************************************************************/
 
static __inline
void AddRegionToDisplay (STSUBT_RDE_RegionDescriptor_t *RegionDescriptor,
                         STSUBT_RDE_ServiceHandle_t *ServiceHandle,
                         STSUBT_RDE_DisplayDescriptor_t *descriptor)
{
  STSUBT_RDE_DisplayRegion_t    *DisplayRegion;
 
  /* --- Allocate a Display Region Item from section 3 --- */
 
  DisplayRegion = ServiceHandle->free_section_3_p;
 
  /* --- Update control information --- */
 
  ServiceHandle->nDisplayRegionsAllocated =
      (ServiceHandle->nDisplayRegionsAllocated + 1) 
          % ServiceHandle->MaxNumDisplayRegions;
 
  if (ServiceHandle->nDisplayRegionsAllocated == 0)
  {
      ServiceHandle->free_section_3_p =
           (STSUBT_RDE_DisplayRegion_t*)ServiceHandle->base_section_3;
  }
  else
  {
      ServiceHandle->free_section_3_p = ServiceHandle->free_section_3_p + 1;
  }
 
  DisplayRegion->RegionDescriptor = RegionDescriptor;
  DisplayRegion->NextDisplayRegion_p = NULL;

  /* --- insert DisplayRegion to the tail of the display RegionList --- */
 
  if (descriptor->TailRegionList == NULL) 
  {
      descriptor->RegionList = DisplayRegion;
      descriptor->TailRegionList = descriptor->RegionList;
  }
  else
  {
      descriptor->TailRegionList->NextDisplayRegion_p = DisplayRegion;
      descriptor->TailRegionList = 
                  descriptor->TailRegionList->NextDisplayRegion_p;
  }
}
/*zxg add*/
/*得到当前REGION调色板数据*/
void CH_GetPalleteData(STGXOBJ_Palette_t *Subtitle_Palette)
{
	memcpy((U8 *)Subtitle_Palette,(U8 *)&color_256_tmp_Palette/*four_bit_entry_default_Palette*/,sizeof(STGXOBJ_Palette_t));
}

/*********************/
/******************************************************************************\
 * Function: UpdateRegionBitmap
 * Purpose : Update region bitmap contents, but only if really needed
 *           i.e. different version numbers
 * Return  : ST_NO_ERROR on success
\******************************************************************************/
#if 0
static __inline ST_ErrorCode_t
UpdateRegionBitmap (STSUBT_RDE_RegionId_t   *RegionHandle,
                    STSUBT_Bitmap_t         *SUBT_Bitmap,
                    STSUBT_InternalHandle_t *DecodeHandle) 
#else
static __inline ST_ErrorCode_t
UpdateRegionBitmap (STSUBT_RDE_RegionId_t   *RegionHandle,
                    STSUBT_Bitmap_t         *SUBT_Bitmap,
                    STSUBT_InternalHandle_t *DecodeHandle,void  *ServiceHandle) 

#endif
{
    STGXOBJ_Bitmap_t RDE_Bitmap;

    if(DecodeHandle->RDEStatusFlag == RDE_RESET)
        return ST_NO_ERROR;
   
	/*zxg add for update bitmap*/
	{
		switch(SUBT_Bitmap->BitsPerPixel)
		{
        case 2:
            ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.ColorType = STGXOBJ_COLOR_TYPE_CLUT2;
            break;
        case 4:
            ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.ColorType = STGXOBJ_COLOR_TYPE_CLUT4;
            break;
        case 8:
            ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.ColorType = STGXOBJ_COLOR_TYPE_CLUT8;
            break;
		}
		
		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.PreMultipliedColor = FALSE;
		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.Width    = SUBT_Bitmap->Width;
		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.Height   = SUBT_Bitmap->Height;
		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.Pitch    = SUBT_Bitmap->Width; 
		/*ProducerRegionINfo[*RegionHandle].Data1_p  = SUBT_Bitmap->DataPtr;*/

		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.Size1    = (SUBT_Bitmap->Width)*(SUBT_Bitmap->Height)
			/(8/SUBT_Bitmap->BitsPerPixel);
		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.Data2_p  =  NULL;
		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.Size2    = 0;
		ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.SubByteFormat =  STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
		

		RdeMemCopy((void*)(ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.Data1_p ),
               (const void*)(SUBT_Bitmap->DataPtr),
               (size_t)(ProducerRegionINfo[*RegionHandle].SubRegionGfxBmp.Size1));

		
		
	}
	/***************************/
#if 0
    switch(SUBT_Bitmap->BitsPerPixel)
    {
        case 2:
            RDE_Bitmap.ColorType = STGXOBJ_COLOR_TYPE_CLUT2;
            break;
        case 4:
            RDE_Bitmap.ColorType = STGXOBJ_COLOR_TYPE_CLUT4;
            break;
        case 8:
            RDE_Bitmap.ColorType = STGXOBJ_COLOR_TYPE_CLUT8;
            break;
    }

    RDE_Bitmap.BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    RDE_Bitmap.PreMultipliedColor = FALSE;
    RDE_Bitmap.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
    RDE_Bitmap.Width    = SUBT_Bitmap->Width;
    RDE_Bitmap.Height   = SUBT_Bitmap->Height;
    RDE_Bitmap.Pitch    = SUBT_Bitmap->Width; 
    RDE_Bitmap.Data1_p  = SUBT_Bitmap->DataPtr;
    RDE_Bitmap.Size1    = (SUBT_Bitmap->Width)*(SUBT_Bitmap->Height)
						/(8/SUBT_Bitmap->BitsPerPixel);
    RDE_Bitmap.Data2_p  =  NULL;
    RDE_Bitmap.Size2    = 0;
    RDE_Bitmap.SubByteFormat =  STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;


 /*zxg for display subtitle*/
   {
   	int x=0;
   	int y=440;
   	STSUBT_RDE_RegionDescriptor_t *RegionDescriptor_p;

   	STGXOBJ_Palette_t Subtitle_STGXOBJ_Palette_t;
   	extern void CH_GetBitmapData(STGXOBJ_Bitmap_t *Subtitle_Bitmap,STSUBT_Bitmap_t         *SUBT_Bitmap);
    extern    void CH6_ShowClutBitmap( int uStartX, int uStartY,STGXOBJ_Bitmap_t Bitmap,STGXOBJ_Palette_t Palette );

	RegionDescriptor_p=GetRegionDescriptor(*RegionHandle, ServiceHandle);
	if(RegionDescriptor_p!=NULL)
	{
		x=RegionDescriptor_p->horizontal_position;
		y=RegionDescriptor_p->vertical_position;
	}
    CH_GetPalleteData(&Subtitle_STGXOBJ_Palette_t);
    CH6_ShowClutBitmap(x, y,RDE_Bitmap,Subtitle_STGXOBJ_Palette_t );
   }
   /*************************/	

    if(RDE_Bitmap.Size1+(sizeof(STGXOBJ_Bitmap_t))+
        +sizeof(STSUBT_RDE_Data_t) > DecodeHandle->RDEBufferSize)
    {
        if (STEVT_Notify(DecodeHandle->stevt_handle,
            DecodeHandle->stevt_event_table[STSUBT_EVENT_RDE_BUFFER_IS_SMALL
            & 0x0000FFFF], DecodeHandle) != ST_NO_ERROR)
        {

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                "RDE >>> Error notifying event** \n"));
        }
        return ST_NO_ERROR;
    }
#endif
#if 0/*zxg del*/
    if(WaitWhileRdeIsBusy(DecodeHandle))
    {
#if 0/*zxg del*/
        RdeMemCopy((void*)(RDE_Data_p),
               (const void*)(RDE_Bitmap.Data1_p),
               (size_t)(RDE_Bitmap.Size1));

        RdeMemCopy((void*)(&RDE_SharedBuffer_p->Params.Bitmap),
               (const void*)(&RDE_Bitmap),
               (size_t)(sizeof(STGXOBJ_Bitmap_t)));
#endif

	    RDE_SharedBuffer_p->RegionId=*RegionHandle;
	    RDE_SharedBuffer_p->Command=STSUBT_RDE_REGION_UPDATE;

        ProducerWriteValid(DecodeHandle);

    }
    else
    {
        ProducerWriteResetRequest(DecodeHandle);
    }

#endif
    return ST_NO_ERROR;
}
 

/******************************************************************************\
 * Function: STSUBT_RDE_InitializeService
 * Purpose : To be called before any display is achived  
 *           This is the implementation of STSUBT_InitializeService callback 
 *           function related to RDE device
 *           Purpose of this function is mainly to create and initialize
 *           the Service Handle
 * Arguments:
 *     - MemoryPartition: Partition from which memory will be allocated.
 * Return  : A pointer to allocated Displaying Service Handle
\******************************************************************************/
 
void *STSUBT_RDE_InitializeService (STSUBT_DisplayServiceParams_t *DisplayServiceParams)
{
  STSUBT_RDE_ServiceHandle_t    *ServiceHandle;
  STSUBT_InternalHandle_t       *DecodeHandle;
  U32                           Region;

  DecodeHandle =   (STSUBT_InternalHandle_t *)
                        (DisplayServiceParams->Handle_p);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"#### STSUBT_RDE_InitializeService: Called (BufferSize=%d)",
					DisplayServiceParams->RDEBufferSize));

  if(DisplayServiceParams->RDEBufferSize < 
            ((sizeof(STSUBT_RDE_Data_t)) +
            (sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*256)))
  {
        if (STEVT_Notify(DecodeHandle->stevt_handle,
            DecodeHandle->stevt_event_table[STSUBT_EVENT_RDE_BUFFER_IS_SMALL
            & 0x0000FFFF],DecodeHandle) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                "RDE >>> Error notifying event** Ox%X\n",res));
        }

    	STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		  "#### RDE_Initialize_Display: Error Buffer is small."));

      return (NULL);
  }

  if(DisplayServiceParams->RDEBuffer_p==NULL)
  {	
    	STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		  "#### RDE_Initialize_Display: Error Buffer is NULL."));
      return (NULL);
  }

  RDE_SharedBuffer_p=(STSUBT_RDE_Data_t*)DisplayServiceParams->RDEBuffer_p;
  RDE_Data_p=(U8*)RDE_SharedBuffer_p+sizeof(STSUBT_RDE_Data_t);

  RDE_SharedBuffer_p->LockState =    STSUBT_RDE_DATA_DIRTY;
  RDE_SharedBuffer_p->Command   =   STSUBT_RDE_NOP;

  /* --- Initialize default Palettes --- */

  init_color_4_default_Palette();
  init_color_16_default_Palette();
  init_color_256_default_Palette();

  for(Region=0;Region <= RDE_MAX_REGIONS;Region++)
  {
     ProducerRegionState[Region] =   RDE_REGION_FREE;
	 /*zxg add*/
	  memset((U8 *)&ProducerRegionINfo[Region],0,sizeof(MySubtileRegion));
  }


  /* --- Create Service Handle --- */

  ServiceHandle = NewServiceHandle (
                        DisplayServiceParams->MemoryPartition,
                        STSUBT_RDE_NUM_DESCRIPTORS,
                        STSUBT_RDE_REGION_PER_EPOCH,
                        STSUBT_RDE_REGION_PER_DISPLAY);

  ServiceHandle->Handle_p = DecodeHandle;


  DecodeHandle->RDEBufferSize = DisplayServiceParams->RDEBufferSize;
  DecodeHandle->RDEStatusFlag = RDE_OK;
  
  if (ServiceHandle == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		"#### RDE_Initialize_Display: Error creating Service Handle"));
      return (NULL);
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"#### STSUBT_RDE_InitializeService: Service Initialized"));

  clocks_per_sec = ST_GetClocksPerSecond();

  color_256_tmp_Palette.PaletteType 	=
            STGXOBJ_PALETTE_TYPE_DEVICE_DEPENDENT; 
  color_256_tmp_Palette.Data_p		    =
             eight_bit_entry_tmp_Palette;

  STRDE_SetDefaultPalette(DecodeHandle,&color_256_default_Palette);
  STRDE_SetDefaultPalette(DecodeHandle,&color_16_default_Palette);
  STRDE_SetDefaultPalette(DecodeHandle,&color_4_default_Palette);


  return ((void*)ServiceHandle);
}


/******************************************************************************\
 * Function: STSUBT_RDE_TerminateService
 * Purpose : This is the implementation of STSUBT_Terminate_Display callback  
 *           function related to RDE device 
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t STSUBT_RDE_TerminateService (void *ServiceHandle)
{  
  STSUBT_RDE_ServiceHandle_t        *RDE_ServiceHandle;
  STSUBT_RDE_RegionDescriptor_t     *EpochRegionList;
  STSUBT_InternalHandle_t           *DecoderHandle;
  ST_ErrorCode_t                    res = ST_NO_ERROR;  
 
  RDE_ServiceHandle = (STSUBT_RDE_ServiceHandle_t*)ServiceHandle;
  DecoderHandle =   RDE_ServiceHandle->Handle_p;

  semaphore_wait(RDE_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"\n\n#### STSUBT_RDE_TerminateService: Called\n"));

  /* --- Delete all allocated regions --- */

  EpochRegionList = RDE_ServiceHandle->EpochRegionList;
  while (EpochRegionList != NULL)
  {
	  /*zxg add clear ALL region*/
       res = STRDE_RegionHide(DecoderHandle,
                       *(EpochRegionList->RegionHandle));
	  /**************************/
      res = STRDE_RegionDelete(DecoderHandle,
                       *(EpochRegionList->RegionHandle));
      if (res != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
			"#### STSUBT_RDE_TerminateService: error deleting region (%d)",
						res));
      }
      EpochRegionList = EpochRegionList->NextRegion_p;
  }
 
  semaphore_signal(RDE_ServiceHandle->DisplaySemaphore);

  /* --- delete the Service Handle --- */

  DeleteServiceHandle (RDE_ServiceHandle);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"#### STSUBT_RDE_TerminateService: Service Terminated\n"));

  return (res);
}


/******************************************************************************\
 * Function: STSUBT_RDE_PlanEpoch
 * Purpose : Plan the display service for the starting epoch
 * Parameters:
 *     PageComposition_p:
 *           A Page Composition of Mode Change. It contains the list of all
 *           regions planned for the starting epoch.
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t STSUBT_RDE_PlanEpoch (
                      PCS_t *PageComposition_p,
                      STSUBT_RDE_ServiceHandle_t *RDE_ServiceHandle,
                      STSUBT_InternalHandle_t *DecoderHandle)
{
  STSUBT_RDE_RegionDescriptor_t     *EpochRegionList;
  STSUBT_RDE_RegionId_t             RegionHandle;
  ST_ErrorCode_t                    ReturnCode = ST_NO_ERROR;
  ST_ErrorCode_t                    res;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"#### STSUBT_RDE_PlanEpoch: Planning epoch\n"));

  /* Current display is disabled,
   * old regions are deleted and new regions created.
   * New regions are linked to create an EpochRegionList.
   */

  /* --- Delete RDE regions created for the just finished epoch --- */

  EpochRegionList = RDE_ServiceHandle->EpochRegionList;
  while (EpochRegionList != NULL)
  {
      RegionHandle = *EpochRegionList->RegionHandle;
      res = STRDE_RegionDelete(DecoderHandle,RegionHandle);
      if (res != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
			"#### STSUBT_RDE_PlanEpoch: error deleting region %d\n", res));
          ReturnCode = 3;
      }
      EpochRegionList = EpochRegionList->NextRegion_p;
  }


  /* --- reset the Service Handle --- */

  ResetServiceHandle(RDE_ServiceHandle);

  /* --- Create RDE regions --- */

  {
      STSUBT_RDE_RegionParams_t RegionParams;
      STSUBT_RDE_PaletteDescriptor_t *PaletteDescriptor;
      U8     region_id;
      U8     region_depth;
      S32    horizontal_position;
      S32    vertical_position;
      U32    region_width;
      U32    region_height;
      U8     region_version;
      CLUT_t *region_clut;

      STSUBT_Bitmap_t *region_bitmap;
      RCS_t *EpochRegionList = GetEpochRegionList(PageComposition_p);

      /* Get description of subt regions which compose the PageComposition
       * and create the new RDE epoch regions 
       */

      while (GetNextEpochRegionInfo (&region_id,
                                     &horizontal_position,
                                     &vertical_position,
                                     &region_width,
                                     &region_height,
                                     &region_depth,
                                     &region_version,
                                     &region_clut,
                                     &region_bitmap,
                                     &EpochRegionList) == ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_USER1,
			"#### STSUBT_RDE_PlanEpoch: processing region (%d)\n",
					 region_id));


          /* fill in RegionParams */

          RegionParams.PositionX    = horizontal_position;
          RegionParams.PositionY    = vertical_position;
          RegionParams.Width        = region_width;
          RegionParams.Height       = region_height;

          RegionParams.NumBitsPixel = region_depth;
          /* RegionParams.Alpha    = 0xff; */
          
          /* create a new RDE region, 
           * allocate a new Region descriptor 
           * and include it in the EpochRegionList 
           */

          RegionHandle = NewRDERegion(region_id, &RegionParams,
                          RDE_ServiceHandle, DecoderHandle);
          if (RegionHandle == 0)
          {
              ReturnCode = 1;
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
				"#### STSUBT_RDE_PlanEpoch: Cannot create region\n"));
          }

          /* --- Add region palette to EpochPaletteList (if not) --- */

          PaletteDescriptor = GetPaletteDescriptor (RegionHandle, 
                                                    RDE_ServiceHandle);
          if (PaletteDescriptor == NULL)
          {
              PaletteDescriptor = 
                  AddToPaletteList (RegionHandle, RDE_ServiceHandle);
          }

          /* --- Update region Palette, but only if CLUT --- */
          /* --- contents changed (different versions)   --- */

          /* check if CLUT contents changed */
 

          if (region_clut->CLUT_version_number != 
              PaletteDescriptor->Palette_version)
          { 
              res = UpdateRegionPalette(DecoderHandle,
                                        RegionHandle,
                                        region_clut,
                                        region_depth);
              if (res != ST_NO_ERROR)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"#### Error updating palette (%d)\n", res));
              }

              /* update palette version */
 
              PaletteDescriptor->Palette_version = 
                  region_clut->CLUT_version_number;
          }
          else
          {
              STTBX_Report((STTBX_REPORT_LEVEL_USER1,
				"#### STSUBT_RDE_PlanEpoch: CLUT contents unchanged"));
          }
      }
  }

  RDE_ServiceHandle->Starting = FALSE;

  return (ReturnCode);
}   


/******************************************************************************\
 * Function: STSUBT_RDE_PrepareDisplay
 * Purpose : This is the implementation of STSUBT_PrepareDisplay callback
 *           function related to RDE device
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t STSUBT_RDE_PrepareDisplay (
                      PCS_t *PageComposition_p,
                      void  *ServiceHandle,
                      void **DisplayDescriptor)
{ 
  STSUBT_RDE_ServiceHandle_t *RDE_ServiceHandle;
  STSUBT_InternalHandle_t        *DecoderHandle;
  ST_ErrorCode_t              res = ST_NO_ERROR;
 
  RDE_ServiceHandle = (STSUBT_RDE_ServiceHandle_t*)ServiceHandle;
  DecoderHandle =   RDE_ServiceHandle->Handle_p;

  semaphore_wait(RDE_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"\n\n#### STSUBT_RDE_PrepareDisplay: Called\n"));

  /* --- check the acquisition mode of PCS --- */

  if ((PageComposition_p->acquisition_mode == STSUBT_MODE_CHANGE)
  ||  (RDE_ServiceHandle->Starting == TRUE))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,
			"#### STSUBT_RDE_PrepareDisplay: STSUBT_MODE_CHANGE\n"));

      /* --- Plan the display service for the starting epoch --- */

      res = STSUBT_RDE_PlanEpoch (PageComposition_p,
                                  RDE_ServiceHandle,
                                  DecoderHandle);

      if (res != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
			"#### STSUBT_RDE_PrepareDisplay: error planning epoch\n"));
          semaphore_wait(RDE_ServiceHandle->DisplaySemaphore);
          return (res);
      }
  }   

  /* --- get a new display descriptor --- */

  *DisplayDescriptor = NewDisplayDescriptor(RDE_ServiceHandle);

  /* --- process page composition display --- */

  {
      /* The VisibleRegion_p field in a PCS 
       * contains the list of regions visible in the current display set.
       */

      /* Update Display Descriptor to include the provided list of regions. */
 
      STSUBT_RDE_PaletteDescriptor_t *PaletteDescriptor;
      STSUBT_Bitmap_t     *region_bitmap;
      U8                   region_id;
      U8                   region_depth;
      S32                  horizontal_position;
      S32                  vertical_position;
      U32                  region_width;
      U32                  region_height;
      U8                   region_version;
      CLUT_t              *region_clut;
      VisibleRegion_t     *SubtRegionList;
      U32                  LastVerticalPosition = 0;


      STTBX_Report((STTBX_REPORT_LEVEL_USER1,
			"#### STSUBT_RDE_PrepareDisplay: STSUBT_NORMAL_CASE\n"));

      SubtRegionList = GetSubtRegionList (PageComposition_p);
   
      /* --- get info about all regions in the PCS --- */
 
      while (GetNextSubtRegionInfo (&region_id,
                                    &horizontal_position,
                                    &vertical_position,
                                    &region_width,
                                    &region_height,
                                    &region_depth,
                                    &region_version,
                                    &region_clut,
                                    &region_bitmap,
                                    &SubtRegionList) == ST_NO_ERROR)
      {
          STSUBT_RDE_RegionDescriptor_t *RegionDescriptor_p;
          STSUBT_RDE_RegionId_t          *RegionHandle;

          /* check if at least a video line is left between
           * adjacent regions (8bpp only)
           */
 

          if (LastVerticalPosition >= vertical_position)
          {
              if (region_depth == 8)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
					"#### STSUBT_RDE_PrepareDisplay: region abutted"));
                  vertical_position = LastVerticalPosition + 1;
              }
              else
              {
                  vertical_position = LastVerticalPosition;
              }
          }
          LastVerticalPosition = vertical_position + region_height;
          if (LastVerticalPosition > 576)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"#### STSUBT_RDE_PrepareDisplay: outside screen\n"));
              continue;
          }
 
          /* Find corresponding RegionDescriptor into EpochRegionList */

          RegionDescriptor_p = GetRegionDescriptor(region_id, ServiceHandle);
          if (RegionDescriptor_p == NULL)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
				"#### STSUBT_RDE_PrepareDisplay:region not found %d",
							region_id));
              continue;
          }

          /* set region coordinates in region descriptor */

          RegionDescriptor_p->horizontal_position = horizontal_position;
          RegionDescriptor_p->vertical_position = vertical_position;

          /* Add RegionDescriptor to the DisplayDescriptor RegionList */
 
          AddRegionToDisplay (RegionDescriptor_p, RDE_ServiceHandle, 
                             *DisplayDescriptor);
 
          /* --- Get region Handle --- */
          
          RegionHandle = RegionDescriptor_p->RegionHandle;

          /* --- Update regions contents and Palette entries, but only --- */
          /* --- if they really changed (different region versions)    --- */
          /* --- Note: clut and region contents may have changed only  --- */
          /* --- version changed.                                      --- */
         
          if (RegionDescriptor_p->region_version == region_version) 
          {
              STTBX_Report((STTBX_REPORT_LEVEL_USER1,
				"#### STSUBT_RDE_PrepareDisplay: Region contents unchanged\n"));
              continue;
          }

          /* --- Update region version --- */

          RegionDescriptor_p->region_version = region_version;

          /* --- Update region Palette, but only if CLUT --- */
          /* --- contents changed (different versions)   --- */
 
          PaletteDescriptor = 
              GetPaletteDescriptor (*RegionHandle, RDE_ServiceHandle);


          if (region_clut->CLUT_version_number != 
              PaletteDescriptor->Palette_version)
          {
              /* update palette contents */

              res = UpdateRegionPalette(DecoderHandle,
                                        *RegionHandle,
                                        region_clut,
                                        region_depth);
              if (res != ST_NO_ERROR)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"#### Error updating palette (%d)\n", res));
              }

              /* update palette version */
 
              PaletteDescriptor->Palette_version = 
                  region_clut->CLUT_version_number;
          }

          /* --- Update region bitmap, but only if it really --- */
          /* --- changed, i.e. (UpdateFlag == TRUE)          --- */

          if (region_bitmap->UpdateFlag == FALSE)
          {   
              STTBX_Report(
              (STTBX_REPORT_LEVEL_USER1,
				"#### STSUBT_RDE_PrepareDisplay: bitmap not changed\n\n"));
              continue;
          }
#if 0/*zxg change*/
          res = UpdateRegionBitmap(RegionHandle,
                                   region_bitmap,DecoderHandle);
#else
            res = UpdateRegionBitmap(RegionHandle,
                                   region_bitmap,DecoderHandle,ServiceHandle)  ;
#endif
          if (res != ST_NO_ERROR)
          {
              STTBX_Report(
              (STTBX_REPORT_LEVEL_ERROR,
				"#### STSUBT_RDE_PrepareDisplay: Error bitmap update %d\n",
									res));
              continue;
          }

          /* --- reset bitmap update flag --- */

          region_bitmap->UpdateFlag = FALSE;

      } /* end while */
  }   

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"#### STSUBT_RDE_PrepareDisplay: Display Prepared\n"));

  semaphore_signal(RDE_ServiceHandle->DisplaySemaphore);

  return (res);
}


/******************************************************************************\
 * Function: STSUBT_RDE_ShowDisplay
 * Purpose : This the implementation of STSUBT_ShowDisplay callback
 *           function related to RDE device
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t STSUBT_RDE_ShowDisplay (void *ServiceHandle, 
                                       void *DisplayDescriptor)
{
  STSUBT_RDE_DisplayDescriptor_t *RDE_DisplayDescriptor;
  STSUBT_RDE_DisplayRegion_t     *RegionList;
  STSUBT_RDE_ServiceHandle_t     *RDE_ServiceHandle;
  STSUBT_InternalHandle_t        *DecoderHandle;
  STSUBT_RDE_RegionId_t           *RegionHandle;
  U16                             horizontal_position;
  U16                             vertical_position;
  ST_ErrorCode_t                  res = ST_NO_ERROR;
  U16                             Region;
 
  RDE_ServiceHandle = (STSUBT_RDE_ServiceHandle_t*)ServiceHandle;
  RDE_DisplayDescriptor = (STSUBT_RDE_DisplayDescriptor_t*)DisplayDescriptor;

  DecoderHandle =   RDE_ServiceHandle->Handle_p;
  
  semaphore_wait(RDE_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n\n#### STSUBT_RDE_ShowDisplay: Showing display\n"));

  /* --- Change status of old display regions to hidden --- */

  if (RDE_ServiceHandle->ActiveDisplayDescriptor != NULL)
  {
      STSUBT_RDE_DisplayRegion_t *RegionList;

      /* --- Change status of current displayed regions to unvisible --- */

      RegionList = (RDE_ServiceHandle->ActiveDisplayDescriptor)->RegionList;

      while (RegionList != NULL)
      {
          ProducerRegionState[*(RegionList->RegionDescriptor->RegionHandle)] 
              &= ~(RDE_REGION_BIT_DISPLAY);
          RegionList = RegionList->NextDisplayRegion_p;
      }

      /* --- reset active display descriptor in ServiceHandle --- */

      RDE_ServiceHandle->ActiveDisplayDescriptor = NULL;
  }

  /* --- Change status of new display regions to visible --- */
 
  RegionList = RDE_DisplayDescriptor->RegionList;
  
  while (RegionList != NULL)
  {
      RegionHandle = (RegionList->RegionDescriptor)->RegionHandle;

      vertical_position =
          (RegionList->RegionDescriptor)->vertical_position;

      horizontal_position =
          (RegionList->RegionDescriptor)->horizontal_position;

      /* --- set new region position --- */

      if (((RegionList->RegionDescriptor)->old_horizontal_position
           != horizontal_position)
      ||  ((RegionList->RegionDescriptor)->old_vertical_position
           != vertical_position))
      {
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                     "#### STSUBT_RDE_ShowDisplay: Moving region\n"));

	      (RegionList->RegionDescriptor)->old_vertical_position=
                vertical_position;

	      (RegionList->RegionDescriptor)->old_horizontal_position=
                horizontal_position;

          if ((res = STRDE_RegionMove(
                            DecoderHandle,
                            *RegionHandle,
                            horizontal_position,
                            vertical_position)) != ST_NO_ERROR)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                 "#### STSUBT_RDE_ShowDisplay: move failed (%d)\n", res));

          }
      }
/*zxg add*/
     /*if (ProducerRegionState[Region] & RDE_REGION_BIT_DISPLAY)*/
     {
        if((res=STRDE_RegionShow(DecoderHandle,*RegionHandle)) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                "#### STSUBT_RDE_ShowDisplay: error show Region (%d)\n",
                                res));
        }
     }
      
/**********/
      ProducerRegionState[*RegionHandle] |= RDE_REGION_BIT_DISPLAY;
      
      
     


      RegionList = RegionList->NextDisplayRegion_p;
  }

  /* Hidden all regions with RDE_REGION_BIT_DISPLAY bit set */

  for(Region=1;Region <= RDE_MAX_REGIONS;Region++)
  {
     if(ProducerRegionState[Region] == RDE_REGION_FREE)
     {
        continue;
     }

     if (!(ProducerRegionState[Region] & RDE_REGION_BIT_DISPLAY))
     {
        if((res=STRDE_RegionHide(DecoderHandle,Region)) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
				"#### STSUBT_RDE_ShowDisplay: error hiding region (%d)\n",
                                res));
        }
     }
  }
#if 0/*zxg chang*/
  /* Show all regions with RDE_REGION_BIT_DISPLAY bit clear */

  for(Region=1;Region <= RDE_MAX_REGIONS;Region++)
  {
     if(ProducerRegionState[Region] == RDE_REGION_FREE)
     {
        continue;
     }

     if (ProducerRegionState[Region] & RDE_REGION_BIT_DISPLAY)
     {
        if((res=STRDE_RegionShow(DecoderHandle,Region)) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                "#### STSUBT_RDE_ShowDisplay: error show Region (%d)\n",
                                res));
        }
     }
  }
#endif
  /* --- update active display descriptor in ServiceHandle --- */
 
  RDE_ServiceHandle->ActiveDisplayDescriptor = RDE_DisplayDescriptor;

  semaphore_signal(RDE_ServiceHandle->DisplaySemaphore);

  return (res);
}
 

/******************************************************************************\
 * Function: STSUBT_RDE_HideDisplay
 * Purpose : This the implementation of the STSUBT_HideDisplay callback
 *           function related to RDE display device
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t STSUBT_RDE_HideDisplay (void *ServiceHandle,
                                       void *DisplayDescriptor)
{
  ST_ErrorCode_t                  res = ST_NO_ERROR;
  STSUBT_RDE_DisplayRegion_t     *RegionList;
  STSUBT_RDE_RegionId_t           *RegionHandle;
  STSUBT_RDE_DisplayDescriptor_t *RDE_DisplayDescriptor;
  STSUBT_RDE_ServiceHandle_t     *RDE_ServiceHandle;
  STSUBT_InternalHandle_t        *DecoderHandle;
  RDE_ServiceHandle = (STSUBT_RDE_ServiceHandle_t*)ServiceHandle;
  RDE_DisplayDescriptor = (STSUBT_RDE_DisplayDescriptor_t*)DisplayDescriptor;

  DecoderHandle =   RDE_ServiceHandle->Handle_p;

  semaphore_wait(RDE_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n\n#### STSUBT_RDE_HideDisplay: Hiding display\n"));

  /* --- Disables all the regions currently displayed on the screen --- */

  /* Change status of current displayed regions to unvisible */

  RegionList = RDE_DisplayDescriptor->RegionList;
  while (RegionList != NULL)
  {  
      RegionHandle = (RegionList->RegionDescriptor)->RegionHandle;
      res = STRDE_RegionHide(DecoderHandle,*RegionHandle);
      if (res != ST_NO_ERROR)
      {
          STTBX_Report(
          (STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_RDE_HideDisplay: error hiding Region (%d)\n", res));
      }
      RegionList = RegionList->NextDisplayRegion_p;
  }
  
  /* --- reset the visible region list --- */

  RDE_DisplayDescriptor->RegionList = NULL;

  /* --- reset active display descriptor in ServiceHandle --- */

  RDE_ServiceHandle->ActiveDisplayDescriptor = NULL;

  semaphore_signal(RDE_ServiceHandle->DisplaySemaphore);
  
  return (res);
}



static __inline ST_ErrorCode_t STRDE_RegionCreate(
        STSUBT_InternalHandle_t   *DecodeHandle,
        STSUBT_RDE_RegionParams_t *RegionParams,
		STSUBT_RDE_RegionId_t * RegionId)
{
    U8 cc=1;

    DecodeHandle->RDEStatusFlag = RDE_OK;

	while (ProducerRegionState[cc] & RDE_REGION_BIT_BUSY)
	{
		if (cc > RDE_MAX_REGIONS )
		{
    		STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"RDE >>> Error STRDE_RegionCreate(,%d) \n",cc));
			return  ST_ERROR_DEVICE_BUSY;
		}
	 	cc++;
	 }

	ProducerRegionState[cc] |= RDE_REGION_BIT_BUSY;

    *RegionId=cc;
/*zxg add for create SubRegion info*/
RdeMemCopy((void*)(&ProducerRegionINfo[cc].SubRegionPara),(const void*)(RegionParams),(size_t)(sizeof(STSUBT_RDE_RegionParams_t)));

switch(RegionParams->NumBitsPixel)
{
case 8:
    ProducerRegionINfo[cc].SubRegionPallete.Data_p=memory_allocate(CHSysPartition,sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*256);
    ProducerRegionINfo[cc].SubRegionGfxBmp.Data1_p=memory_allocate(CHSysPartition, RegionParams->Width*RegionParams->Height );
	break;
case 4:
    ProducerRegionINfo[cc].SubRegionPallete.Data_p=memory_allocate(CHSysPartition,sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*16);
    ProducerRegionINfo[cc].SubRegionGfxBmp.Data1_p=memory_allocate(CHSysPartition, ( RegionParams->Width*RegionParams->Height /2|0x0F ));
	break;
case 2:
    ProducerRegionINfo[cc].SubRegionPallete.Data_p=memory_allocate(CHSysPartition,sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*4);
    ProducerRegionINfo[cc].SubRegionGfxBmp.Data1_p=memory_allocate(CHSysPartition, ( RegionParams->Width*RegionParams->Height /4|0x0F ));
	break;
case 1:
    ProducerRegionINfo[cc].SubRegionPallete.Data_p=memory_allocate(CHSysPartition,sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)/8);
    ProducerRegionINfo[cc].SubRegionGfxBmp.Data1_p=memory_allocate(CHSysPartition, ( RegionParams->Width*RegionParams->Height /8|0x0F ));
	break;
default:
    ProducerRegionINfo[cc].SubRegionPallete.Data_p=memory_allocate(CHSysPartition,sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*256);
    ProducerRegionINfo[cc].SubRegionGfxBmp.Data1_p=memory_allocate(CHSysPartition, RegionParams->Width*RegionParams->Height );
	break;
	
}
/**********************************/
#if 0/*zxg del*/
    if(WaitWhileRdeIsBusy(DecodeHandle))
    {
	    RDE_SharedBuffer_p->Command=STSUBT_RDE_REGION_CREATE;
	    RDE_SharedBuffer_p->RegionId=*RegionId;

        RdeMemCopy((void*)(&RDE_SharedBuffer_p->Params.RegionParams),
               (const void*)(RegionParams),
               (size_t)(sizeof(STSUBT_RDE_RegionParams_t)));
    
        ProducerWriteValid(DecodeHandle);
    }
    else
    {
        ProducerWriteResetRequest(DecodeHandle);
    }
#endif
    return ST_NO_ERROR;

}
/***************************************************
****************************************************/
static __inline ST_ErrorCode_t
STRDE_RegionDelete(STSUBT_InternalHandle_t *DecodeHandle,
                      STSUBT_RDE_RegionId_t RegionId) 
{

   if(DecodeHandle->RDEStatusFlag == RDE_RESET)
        return ST_NO_ERROR;

   ProducerRegionState[RegionId] = RDE_REGION_FREE; 

   /*zxg add for delete SubRegionInfo*/
   if(ProducerRegionINfo[RegionId].SubRegionGfxBmp.Data1_p!=NULL)
   {
      memory_deallocate(CHSysPartition,ProducerRegionINfo[RegionId].SubRegionGfxBmp.Data1_p);	  
   }
    if(ProducerRegionINfo[RegionId].SubRegionPallete.Data_p!=NULL)
   {
     memory_deallocate(CHSysPartition,ProducerRegionINfo[RegionId].SubRegionPallete.Data_p);
   }
   memset((U8 *)&ProducerRegionINfo[RegionId],0,sizeof(MySubtileRegion));
   /***********************************/
 

   

#if 0/*zxg del*/
   if(WaitWhileRdeIsBusy(DecodeHandle))
   {
    	RDE_SharedBuffer_p->Command=STSUBT_RDE_REGION_DELETE;
    	RDE_SharedBuffer_p->RegionId=RegionId;
        ProducerWriteValid(DecodeHandle);
   }
   else
   {
        ProducerWriteResetRequest(DecodeHandle);
   }
#endif
   return ST_NO_ERROR;
}

/***************************************************
****************************************************/
ST_ErrorCode_t
STRDE_SetDefaultPalette(STSUBT_InternalHandle_t *DecodeHandle,
                             STGXOBJ_Palette_t *Palette_p)
{
   U32 Size;

   if(DecodeHandle->RDEStatusFlag == RDE_RESET)
        return ST_NO_ERROR;
   
#if 0/*zxg del*/
   if(WaitWhileRdeIsBusy(DecodeHandle))
   {
	    RDE_SharedBuffer_p->Command=STSUBT_RDE_SET_DEFAULT_PALETTE;
	    RDE_SharedBuffer_p->RegionId=0;
    
        RdeMemCopy((void*)(&RDE_SharedBuffer_p->Params.Palette),
               (const void*)(Palette_p),
               (size_t)(sizeof(STGXOBJ_Palette_t)));

        switch(Palette_p->ColorType)
        {
            case STGXOBJ_COLOR_TYPE_CLUT2: 
                Size=sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*4;
                break;
            case STGXOBJ_COLOR_TYPE_CLUT4: 
                Size=sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*16;
                break;
            case STGXOBJ_COLOR_TYPE_CLUT8: 
                Size=sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*256;
                break;
        }
    
        RdeMemCopy((void*)(RDE_Data_p),
                (const void*)(Palette_p->Data_p),
                (size_t)(sizeof(Size)));
    
        ProducerWriteValid(DecodeHandle);
   }
   else
   {
        ProducerWriteResetRequest(DecodeHandle);
   }
#endif
	return ST_NO_ERROR;
}
/***************************************************
****************************************************/
ST_ErrorCode_t
STRDE_SetPalette(STSUBT_InternalHandle_t *DecodeHandle,
                        STSUBT_RDE_RegionId_t RegionId,
						STGXOBJ_Palette_t *Palette_p)
{
   U32 Size;

   if(DecodeHandle->RDEStatusFlag == RDE_RESET)
        return ST_NO_ERROR;
   /*zxg add for Update Pallete information*/
   if(ProducerRegionINfo[RegionId].SubRegionPallete.Data_p!=NULL)
   {
       int NumBitsPixel=1;
	   switch(Palette_p->ColorType)
	   {
	   case STGXOBJ_COLOR_TYPE_CLUT2: 
		   Size=sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*4;
		   NumBitsPixel=2;
		   break;
	   case STGXOBJ_COLOR_TYPE_CLUT4: 
		   Size=sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*16;
		   NumBitsPixel=4;
			break;
	   case STGXOBJ_COLOR_TYPE_CLUT8: 
		   Size=sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*256;
		   NumBitsPixel=8;
		   break;
	   }
	   /*调色板位深度必须和REGION位深度一致*/
	   if(NumBitsPixel==ProducerRegionINfo[RegionId].SubRegionPara.NumBitsPixel)
	   {
		   RdeMemCopy((void*)(&ProducerRegionINfo[RegionId].SubRegionPallete),
			   (const void*)(Palette_p),
			   (size_t)(sizeof(STGXOBJ_Palette_t)));
		   
		   RdeMemCopy((void*)(ProducerRegionINfo[RegionId].SubRegionPallete.Data_p),
			   (const void*)(Palette_p->Data_p),
			   (size_t)(sizeof(Size)));
		   
		   ProducerRegionINfo[RegionId].UseMyPallete=true;		   
	   }
   }
   /***************************/
#if 0/*zxg del*/
   if(WaitWhileRdeIsBusy(DecodeHandle))
   {
	    RDE_SharedBuffer_p->Command=STSUBT_RDE_SET_PALETTE;
	    RDE_SharedBuffer_p->RegionId=RegionId;
    
        RdeMemCopy((void*)(&RDE_SharedBuffer_p->Params.Palette),
               (const void*)(Palette_p),
               (size_t)(sizeof(STGXOBJ_Palette_t)));

        switch(Palette_p->ColorType)
        {
            case STGXOBJ_COLOR_TYPE_CLUT2: 
                Size=sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*4;
                break;
            case STGXOBJ_COLOR_TYPE_CLUT4: 
                Size=sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*16;
                break;
            case STGXOBJ_COLOR_TYPE_CLUT8: 
                Size=sizeof(STGXOBJ_ColorUnsignedAYCbCr_t)*256;
                break;
        }
    
        RdeMemCopy((void*)(RDE_Data_p),
                (const void*)(Palette_p->Data_p),
                (size_t)(sizeof(Size)));
    
        ProducerWriteValid(DecodeHandle);

   }
   else
   {
        ProducerWriteResetRequest(DecodeHandle);
   }
#endif
   return ST_NO_ERROR;
}
/***************************************************
****************************************************/

static __inline ST_ErrorCode_t
STRDE_RegionMove(STSUBT_InternalHandle_t *DecodeHandle,
                 STSUBT_RDE_RegionId_t RegionId,
                 U16 horizontal_position, U16 vertical_position)
{
   if(DecodeHandle->RDEStatusFlag == RDE_RESET)
        return ST_NO_ERROR;
   /*zxg add for Region Move*/
   ProducerRegionINfo[RegionId].SubRegionPara.PositionX=horizontal_position;
   ProducerRegionINfo[RegionId].SubRegionPara.PositionY=vertical_position;
  /***********************/
#if 0/*zxg del*/
   if(WaitWhileRdeIsBusy(DecodeHandle))
   {
	    RDE_SharedBuffer_p->Command=STSUBT_RDE_REGION_MOVE;
	    RDE_SharedBuffer_p->RegionId=RegionId;
        RDE_SharedBuffer_p->Params.MoveParams.PositionX=
                            horizontal_position;
        RDE_SharedBuffer_p->Params.MoveParams.PositionY=
                            vertical_position;
        ProducerWriteValid(DecodeHandle);
   }
   else
   {
        ProducerWriteResetRequest(DecodeHandle);
   }
#endif
   return ST_NO_ERROR;
}


/***************************************************
****************************************************/
static __inline ST_ErrorCode_t
STRDE_RegionShow(STSUBT_InternalHandle_t *DecodeHandle,
                 STSUBT_RDE_RegionId_t RegionId)
{
	extern void CH6_ShowClutBitmap( int uStartX, int uStartY,STGXOBJ_Bitmap_t Bitmap,STGXOBJ_Palette_t Palette );
    if(DecodeHandle->RDEStatusFlag == RDE_RESET)
        return ST_NO_ERROR;

    if(ProducerRegionState[RegionId] & RDE_REGION_BIT_SHOW)
        return ST_NO_ERROR;

    ProducerRegionState[RegionId] |=  RDE_REGION_BIT_SHOW;
	/*zxg add for Display Region information*/
	if(ProducerRegionINfo[RegionId].UseMyPallete==true)/*使用前端发送调色板*/
	{
        CH6_ShowClutBitmap( ProducerRegionINfo[RegionId].SubRegionPara.PositionX, 
			                ProducerRegionINfo[RegionId].SubRegionPara.PositionY,
							ProducerRegionINfo[RegionId].SubRegionGfxBmp,
							ProducerRegionINfo[RegionId].SubRegionPallete);							
	}
	else/*使用缺省调色板*/
	{
		switch(ProducerRegionINfo[RegionId].SubRegionPara.NumBitsPixel)
		{
			
		case 2:
			CH6_ShowClutBitmap( ProducerRegionINfo[RegionId].SubRegionPara.PositionX, 
				ProducerRegionINfo[RegionId].SubRegionPara.PositionY,
				ProducerRegionINfo[RegionId].SubRegionGfxBmp,
				color_4_default_Palette);	
			break;
		case 4:
			CH6_ShowClutBitmap( ProducerRegionINfo[RegionId].SubRegionPara.PositionX, 
				ProducerRegionINfo[RegionId].SubRegionPara.PositionY,
				ProducerRegionINfo[RegionId].SubRegionGfxBmp,
				color_16_default_Palette);	
			break;
		case 8:
			CH6_ShowClutBitmap( ProducerRegionINfo[RegionId].SubRegionPara.PositionX, 
				ProducerRegionINfo[RegionId].SubRegionPara.PositionY,
				ProducerRegionINfo[RegionId].SubRegionGfxBmp,
				color_256_default_Palette);	
			break;
		default:
			break;
			
		}
	}
    /****************************************/
#if 0/*zxg del*/
    if(WaitWhileRdeIsBusy(DecodeHandle))
    {
  
	    RDE_SharedBuffer_p->Command=STSUBT_RDE_REGION_SHOW;
	    RDE_SharedBuffer_p->RegionId=RegionId;
        

        ProducerWriteValid(DecodeHandle);
    }
    else
    {
        ProducerWriteResetRequest(DecodeHandle);
    }
#endif
    return ST_NO_ERROR;
}

/***************************************************
****************************************************/
static __inline ST_ErrorCode_t
STRDE_RegionHide(STSUBT_InternalHandle_t *DecodeHandle,
                 STSUBT_RDE_RegionId_t RegionId)
{
	extern void Hide_SubRegion(int x,int y,int width,int height);
    if(DecodeHandle->RDEStatusFlag == RDE_RESET)
        return ST_NO_ERROR;

    if(!(ProducerRegionState[RegionId] & RDE_REGION_BIT_SHOW))
        return ST_NO_ERROR;

    ProducerRegionState[RegionId] &= ~(RDE_REGION_BIT_SHOW);

	/*zxg add for Hide Region*/
     Hide_SubRegion(
		            ProducerRegionINfo[RegionId].SubRegionPara.PositionX,
		            ProducerRegionINfo[RegionId].SubRegionPara.PositionY,
				    ProducerRegionINfo[RegionId].SubRegionPara.Width,
					ProducerRegionINfo[RegionId].SubRegionPara.Height);
    /**/

#if 0/*zxg del*/
    if(WaitWhileRdeIsBusy(DecodeHandle))
    {
	    RDE_SharedBuffer_p->Command=STSUBT_RDE_REGION_HIDE;
	    RDE_SharedBuffer_p->RegionId=RegionId;

        ProducerWriteValid(DecodeHandle);
    }
    else
    {
        ProducerWriteResetRequest(DecodeHandle);
    }
#endif
    return ST_NO_ERROR;
}

static __inline BOOL
WaitWhileRdeIsBusy(STSUBT_InternalHandle_t *DecodeHandle)
{
    ST_ErrorCode_t  res;
    clock_t         timeout;

	return TRUE;/*zxg 2005 add for quick display*/

    timeout = time_plus(time_now(),
                    clocks_per_sec/(1000/WAIT_MS_IF_RDE_IS_BUSY));

    while (time_after(timeout,time_now()))
    {
        switch(RDE_SharedBuffer_p->LockState)
        {
            case STSUBT_RDE_DATA_VALID:
            case STSUBT_RDE_DATA_READING:
                break;
            case STSUBT_RDE_DATA_DIRTY:
		        RDE_SharedBuffer_p->LockState = STSUBT_RDE_DATA_WRITING;
                return TRUE; /* Ok */
                break;
            case STSUBT_RDE_DATA_WRITING:
                break;
        }
        task_delay(1);
    }

    if((res=STEVT_Notify(DecodeHandle->stevt_handle,
        DecodeHandle->stevt_event_table[STSUBT_EVENT_RDE_TIMEOUT
        & 0x0000FFFF],
        DecodeHandle)) != ST_NO_ERROR)
    {

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
            "RDE >>> Error notifying event** Ox%X\n",res));
    }

    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
        "WaitWhileRdeIsBusy timeout (%d state)\n",
	        RDE_SharedBuffer_p->LockState));



    return FALSE; /* Timeout */
}
static  void
ProducerWriteResetRequest(STSUBT_InternalHandle_t *DecodeHandle)
{
    U32 cc;

    DecodeHandle->RDEStatusFlag = RDE_RESET;

	RDE_SharedBuffer_p->Command  = STSUBT_RDE_RESET;
	RDE_SharedBuffer_p->RegionId = 0;
	RDE_SharedBuffer_p->LockState=STSUBT_RDE_DATA_VALID;

    for(cc=0;cc <= RDE_MAX_REGIONS;cc++)
    {
        ProducerRegionState[cc] =   RDE_REGION_FREE;
    }

}


static __inline void
ProducerWriteValid(STSUBT_InternalHandle_t *DecodeHandle)
{
#ifdef RDE_ENABLE_NOTIFY_COMMAND_IS_POSTED
    ST_ErrorCode_t res;
#endif

	RDE_SharedBuffer_p->LockState=STSUBT_RDE_DATA_VALID;

#ifdef RDE_ENABLE_NOTIFY_COMMAND_IS_POSTED

    if ((res=STEVT_Notify(DecodeHandle->stevt_handle,
        DecodeHandle->stevt_event_table[STSUBT_EVENT_RDE_COMMAND_IS_POSTED
        & 0x0000FFFF],
        DecodeHndl)) != ST_NO_ERROR)
    {

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
            "RDE >>> Error notifying event** Ox%X\n",res));
    }

#endif

}

void *RdeMemCopy(void *dest,const void *source, size_t num_byte)
{
    return(memcpy(dest,source,num_byte));
}


#endif /* end not def DISABLE_RDE_DISPLAY_SERVICE */
