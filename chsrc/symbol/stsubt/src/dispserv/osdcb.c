/******************************************************************************\
 * File Name : osdcb.c
 * Description:
 *     This module provides an implementation of low-level display primitives
 *     for the display service based on the OSD display device.
 *
 *     See SDD document for more details.
 * 
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Marino Congiu - Jan 1999
 *
\*****************************************************************************/
 
#include <stdlib.h>
#include <time.h>

#include <stddefs.h>
#include <stlite.h>
#include <stcommon.h>

#ifndef RDE_DISPLAY_SERVICE 
#include <stosd.h>
#endif
#include <sttbx.h>
 
#include <compbuf.h>
#include <pixbuf.h>
 
#ifndef DISABLE_OSD_DISPLAY_SERVICE 

#include "dispserv.h"
#include "osdcb.h"

/* --- Get and register number of clocks per second --- */

static clock_t clocks_per_sec;


/* ------------------------------------------------ */
/* --- Predefined OSD Display Service Structure --- */
/* ------------------------------------------------ */
 
STSUBT_DisplayService_t STSUBT_OSD_DisplayServiceStructure = {
  STSUBT_OSD_InitializeService,
  STSUBT_OSD_PrepareDisplay,
  STSUBT_OSD_ShowDisplay,
  STSUBT_OSD_HideDisplay,
  STSUBT_OSD_TerminateService,
} ;

/* ------------------------------------ */
/* --- Defines the default Palettes --- */
/* ------------------------------------ */

/* Note: Entry color model is YCbCr (8/8/8) */
 
static STOSD_Color_t two_bit_entry_default_Palette[4]; 
static STOSD_Color_t four_bit_entry_default_Palette[16];
static STOSD_Color_t eight_bit_entry_default_Palette[256];


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

#define Y_RED_MULTIPLIER     	 66
#define Y_GREEN_MULTIPLIER  	129
#define Y_BLUE_MULTIPLIER   	 25
 
#define CR_RED_MULTIPLIER    	112
#define CR_GREEN_MULTIPLIER 	-94
#define CR_BLUE_MULTIPLIER  	-18
 
#define CB_RED_MULTIPLIER   	-38
#define CB_GREEN_MULTIPLIER 	-74
#define CB_BLUE_MULTIPLIER   	112

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

static
void init_2_bit_entry_default_Palette (void)
{
  U8  Y, Cr, Cb;

  RGB_2_YCrCb (0x00, 0x00, 0x00, &Y, &Cr, &Cb);
 
  two_bit_entry_default_Palette[0].MixWeight = 0x0;
  two_bit_entry_default_Palette[0].Type = STOSD_COLOR_TYPE_YCRCB888;
  two_bit_entry_default_Palette[0].Value.YCrCb888.Y = Y;
  two_bit_entry_default_Palette[0].Value.YCrCb888.Cr = Cr;
  two_bit_entry_default_Palette[0].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0xff, 0xff, 0xff, &Y, &Cr, &Cb);
 
  two_bit_entry_default_Palette[1].MixWeight = 0x3f;
  two_bit_entry_default_Palette[1].Type = STOSD_COLOR_TYPE_YCRCB888;
  two_bit_entry_default_Palette[1].Value.YCrCb888.Y = Y;
  two_bit_entry_default_Palette[1].Value.YCrCb888.Cr = Cr;
  two_bit_entry_default_Palette[1].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x00, 0x00, 0x00, &Y, &Cr, &Cb);
 
  two_bit_entry_default_Palette[2].MixWeight = 0x3f;
  two_bit_entry_default_Palette[2].Type = STOSD_COLOR_TYPE_YCRCB888;
  two_bit_entry_default_Palette[2].Value.YCrCb888.Y = Y;
  two_bit_entry_default_Palette[2].Value.YCrCb888.Cr = Cr;
  two_bit_entry_default_Palette[2].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x7f, 0x7f, &Y, &Cr, &Cb);
 
  two_bit_entry_default_Palette[3].MixWeight = 0x3f;
  two_bit_entry_default_Palette[3].Type = STOSD_COLOR_TYPE_YCRCB888;
  two_bit_entry_default_Palette[3].Value.YCrCb888.Y = Y;
  two_bit_entry_default_Palette[3].Value.YCrCb888.Cr = Cr;
  two_bit_entry_default_Palette[3].Value.YCrCb888.Cb = Cb;

}

/* 16-entry default Palette */

static
void init_4_bit_entry_default_Palette (void)
{
  U8  Y, Cr, Cb;

  RGB_2_YCrCb (0x00, 0x00, 0x00, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[0].MixWeight = 0x0;
  four_bit_entry_default_Palette[0].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[0].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[0].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[0].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0xff, 0x00, 0x00, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[1].MixWeight = 0x3f;
  four_bit_entry_default_Palette[1].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[1].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[1].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[1].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x00, 0xff, 0x00, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[2].MixWeight = 0x3f;
  four_bit_entry_default_Palette[2].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[2].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[2].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[2].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0xff, 0xff, 0x00, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[3].MixWeight = 0x3f;
  four_bit_entry_default_Palette[3].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[3].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[3].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[3].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x00, 0x00, 0xff, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[4].MixWeight = 0x3f;
  four_bit_entry_default_Palette[4].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[4].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[4].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[4].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0xff, 0x00, 0xff, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[5].MixWeight = 0x3f;
  four_bit_entry_default_Palette[5].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[5].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[5].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[5].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x00, 0xff, 0xff, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[6].MixWeight = 0x3f;
  four_bit_entry_default_Palette[6].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[6].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[6].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[6].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0xff, 0xff, 0xff, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[7].MixWeight = 0x3f;
  four_bit_entry_default_Palette[7].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[7].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[7].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[7].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x00, 0x00, 0x00, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[8].MixWeight = 0x3f;
  four_bit_entry_default_Palette[8].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[8].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[8].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[8].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x00, 0x00, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[9].MixWeight = 0x3f;
  four_bit_entry_default_Palette[9].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[9].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[9].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[9].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x00, 0x7f, 0x00, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[10].MixWeight = 0x3f;
  four_bit_entry_default_Palette[10].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[10].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[10].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[10].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x7f, 0x00, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[11].MixWeight = 0x3f;
  four_bit_entry_default_Palette[11].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[11].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[11].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[11].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x00, 0x00, 0x7f, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[12].MixWeight = 0x3f;
  four_bit_entry_default_Palette[12].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[12].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[12].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[12].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x00, 0x7f, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[13].MixWeight = 0x3f;
  four_bit_entry_default_Palette[13].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[13].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[13].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[13].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x00, 0x7f, 0x7f, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[14].MixWeight = 0x3f;
  four_bit_entry_default_Palette[14].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[14].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[14].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[14].Value.YCrCb888.Cb = Cb;

  RGB_2_YCrCb (0x7f, 0x7f, 0x7f, &Y, &Cr, &Cb);
 
  four_bit_entry_default_Palette[15].MixWeight = 0x3f;
  four_bit_entry_default_Palette[15].Type = STOSD_COLOR_TYPE_YCRCB888;
  four_bit_entry_default_Palette[15].Value.YCrCb888.Y = Y;
  four_bit_entry_default_Palette[15].Value.YCrCb888.Cr = Cr;
  four_bit_entry_default_Palette[15].Value.YCrCb888.Cb = Cb;

}

/* 256-entry default Palette */

static void init_8_bit_entry_default_Palette (void)
{
  U32 i;
  U8  r, g, b;
  U8  Y, Cr, Cb;

  RGB_2_YCrCb (0, 0, 0, &Y, &Cr, &Cb);

  eight_bit_entry_default_Palette[0].MixWeight = 0x0;
  eight_bit_entry_default_Palette[0].Type = STOSD_COLOR_TYPE_YCRCB888;
  eight_bit_entry_default_Palette[0].Value.YCrCb888.Y = Y;
  eight_bit_entry_default_Palette[0].Value.YCrCb888.Cr = Cr;
  eight_bit_entry_default_Palette[0].Value.YCrCb888.Cb = Cb;

  for (i = 1; i <= 7; i++)
  {
      r = ((i & 0x1) != 0) * 0xff;
      g = ((i & 0x2) != 0) * 0xff;
      b = ((i & 0x4) != 0) * 0xff;
      RGB_2_YCrCb (r, g, b, &Y, &Cr, &Cb);

      eight_bit_entry_default_Palette[i].MixWeight = 0x2f;
      eight_bit_entry_default_Palette[i].Type = STOSD_COLOR_TYPE_YCRCB888;
      eight_bit_entry_default_Palette[i].Value.YCrCb888.Y = Y;
      eight_bit_entry_default_Palette[i].Value.YCrCb888.Cr = Cr;
      eight_bit_entry_default_Palette[i].Value.YCrCb888.Cb = Cb;

  }

  for (i = 8; i <= 0x7f; i++)
  {
      r = (((i & 0x1) != 0) * 0x54 + ((i & 0x10) != 0) * 0xaa);
      g = (((i & 0x2) != 0) * 0x54 + ((i & 0x20) != 0) * 0xaa);
      b = (((i & 0x4) != 0) * 0x54 + ((i & 0x40) != 0) * 0xaa);
      RGB_2_YCrCb (r, g, b, &Y, &Cr, &Cb);

      eight_bit_entry_default_Palette[i].MixWeight = (0x3f - (((i & 0x8) != 0) * 0x1f));
      eight_bit_entry_default_Palette[i].Type = STOSD_COLOR_TYPE_YCRCB888;
      eight_bit_entry_default_Palette[i].Value.YCrCb888.Y = Y;
      eight_bit_entry_default_Palette[i].Value.YCrCb888.Cr = Cr;
      eight_bit_entry_default_Palette[i].Value.YCrCb888.Cb = Cb;

  }

  for (i = 0x80; i <= 0xff; i++)
  {
      U8 delta = ((i & 0x8) == 0) * 0x7f;	/* + 50% */
 
      r = (((i & 0x1) != 0) * 0x2a + ((i & 0x10) != 0) * 0x54) + delta;
      g = (((i & 0x2) != 0) * 0x2a + ((i & 0x20) != 0) * 0x54) + delta;
      b = (((i & 0x4) != 0) * 0x2a + ((i & 0x40) != 0) * 0x54) + delta;
      RGB_2_YCrCb (r, g, b, &Y, &Cr, &Cb);

      eight_bit_entry_default_Palette[i].MixWeight = 0x3f;
      eight_bit_entry_default_Palette[i].Type = STOSD_COLOR_TYPE_YCRCB888;
      eight_bit_entry_default_Palette[i].Value.YCrCb888.Y = Y;
      eight_bit_entry_default_Palette[i].Value.YCrCb888.Cr = Cr;
      eight_bit_entry_default_Palette[i].Value.YCrCb888.Cb = Cb;

  }
}


/******************************************************************************\
 * Function: UpdateRegionPalette
 * Purpose : Update Region palette entries, but only if their contents
 *           changed (entry version == palette version)
 * Return  : 
\******************************************************************************/
 
static
ST_ErrorCode_t UpdateRegionPalette (
                     STOSD_RegionHandle_t RegionHandle, 
                     CLUT_t *region_clut, 
                     U8 region_depth)
{
  CLUT_Entry_t *SubT_CLUT;
  U16 NumEntries, nEntry;
  BOOL default_flag;
  ST_ErrorCode_t exit_code = ST_NO_ERROR, res;
  S16  CLUT_version_number = region_clut->CLUT_version_number;

  /* choose CLUT in the CLUT family, depending on region_depth */

  switch (region_depth) {
    case 2:  SubT_CLUT = region_clut->two_bit_entry_CLUT_p;
             default_flag = region_clut->two_bit_entry_CLUT_flag;
             NumEntries = 4;
             break;
    case 4:  SubT_CLUT = region_clut->four_bit_entry_CLUT_p;
             default_flag = region_clut->four_bit_entry_CLUT_flag;
             NumEntries = 16;
             break;
    case 8:  SubT_CLUT = region_clut->eight_bit_entry_CLUT_p;
             default_flag = region_clut->eight_bit_entry_CLUT_flag;
             NumEntries = 256;
             break;
  }

  /* if the CLUT is a default CLUT then nothing needs to be updated */

  if (default_flag == TRUE)
  {
      return (ST_NO_ERROR);
  }

  for (nEntry = 0; nEntry < NumEntries; nEntry++)
  {
      /* check if CLUT entry has been changed 
       * i.e. entry_version_number == CLUT_version_number 
       */

      if (SubT_CLUT[nEntry].entry_version_number == CLUT_version_number)
      {
          U8 Y, Cr, Cb, T;
          U32 Entry;
          STOSD_Color_t Color;

          STTBX_Report((STTBX_REPORT_LEVEL_USER1,"#### updating entry %d\n", nEntry));

          /* get YCrCb components and transparency values */

          Entry  = SubT_CLUT[nEntry].entry_color_value; 

          Y  = (Entry & 0xff000000) >> 24; 
          Cr = (Entry & 0x00ff0000) >> 16; 
          Cb = (Entry & 0x0000ff00) >>  8;

          /* DDTS 10770 and 10419: Transparency contro,  akem */
          if (Y == 0)
          { /* Force full transparency */
              T = 0xff;
          }
          else
          {  /* Set transparency as required by definition */
              T  = (Entry & 0x000000ff);
          }
       
          /* set new palette color */
          
          Color.Type = STOSD_COLOR_TYPE_YCRCB888;
          Color.Value.YCrCb888.Y = Y;
          Color.Value.YCrCb888.Cr = Cr;
          Color.Value.YCrCb888.Cb = Cb;
          Color.MixWeight = 0x3f - (T >> 2);

          res = STOSD_SetRegionPaletteColor (RegionHandle, nEntry, Color);
          if (res != ST_NO_ERROR)
          {
             STTBX_Report(
             (STTBX_REPORT_LEVEL_ERROR,"####UpdateRegionPalette:STOSD_SetRegionPaletteColor, %d",res));
             exit_code = res;
          }
      }
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
STSUBT_OSD_ServiceHandle_t *NewServiceHandle (ST_Partition_t *MemoryPartition,
                                              U32 num_descriptors,
                                              U32 region_per_epoch,
                                              U32 region_per_display)
{
    STSUBT_OSD_ServiceHandle_t *ServiceHandle;
    U32 BufferSize;
    U32 section_1_size;
    U32 section_2_size;
    U32 section_3_size;
    U32 section_4_size;
    U32 section_5_size;
 
    /* Alloc memory for the handle and initialize structure  */
 
    ServiceHandle = (STSUBT_OSD_ServiceHandle_t*)
        memory_allocate(MemoryPartition, sizeof(STSUBT_OSD_ServiceHandle_t));
    if (ServiceHandle == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"#### NewServiceHandle: no space **\n"));
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
 
    section_1_size = num_descriptors * sizeof(STSUBT_OSD_DisplayDescriptor_t);
    section_2_size = region_per_epoch * sizeof(STSUBT_OSD_RegionDescriptor_t);
    section_3_size = num_descriptors * region_per_display 
                   * sizeof(STSUBT_OSD_DisplayRegion_t);
    section_4_size = region_per_epoch * sizeof(STOSD_RegionHandle_t);
    section_5_size = region_per_epoch * sizeof(STSUBT_OSD_PaletteDescriptor_t);

    BufferSize = section_1_size + section_2_size 
               + section_3_size + section_4_size + section_5_size;

    if ((ServiceHandle->Buffer_p = memory_allocate(MemoryPartition,
                                                   BufferSize)) == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"#### NewServiceHandle: no space **\n"));
        semaphore_delete (ServiceHandle->DisplaySemaphore);
        memory_deallocate(MemoryPartition, ServiceHandle->DisplaySemaphore);
        memory_deallocate(MemoryPartition, ServiceHandle);
        return (NULL);
    }
 
    /* initialize the Service Handle structure */
 
    ServiceHandle->base_section_1 = ServiceHandle->Buffer_p;
    ServiceHandle->free_section_1_p = 
        (STSUBT_OSD_DisplayDescriptor_t*)ServiceHandle->base_section_1;
    ServiceHandle->MaxNumDescriptors = num_descriptors;
    ServiceHandle->nDescriptorsAllocated = 0;

    ServiceHandle->base_section_2 = ServiceHandle->base_section_1 
                                  + section_1_size;
    ServiceHandle->free_section_2_p = 
        (STSUBT_OSD_RegionDescriptor_t*)ServiceHandle->base_section_2;
    ServiceHandle->MaxNumRegions = region_per_epoch;
    ServiceHandle->nRegionsAllocated = 0;

    ServiceHandle->base_section_3 = ServiceHandle->base_section_2 
                                  + section_2_size;
    ServiceHandle->free_section_3_p = 
        (STSUBT_OSD_DisplayRegion_t*)ServiceHandle->base_section_3;
    ServiceHandle->MaxNumDisplayRegions = region_per_display;
    ServiceHandle->nDisplayRegionsAllocated = 0;

    ServiceHandle->base_section_4 = ServiceHandle->base_section_3
                                  + section_3_size;
    ServiceHandle->free_section_4_p =
        (STOSD_RegionHandle_t*)ServiceHandle->base_section_4;
    ServiceHandle->MaxNumRegionHandlers = region_per_epoch;
    ServiceHandle->nRegionsHandlersAllocated = 0;

    ServiceHandle->base_section_5 = ServiceHandle->base_section_4
                                  + section_4_size;
    ServiceHandle->free_section_5_p =
        (STSUBT_OSD_PaletteDescriptor_t*)ServiceHandle->base_section_5;
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
void ResetServiceHandle (STSUBT_OSD_ServiceHandle_t *ServiceHandle)
{
    ServiceHandle->free_section_1_p =
        (STSUBT_OSD_DisplayDescriptor_t*)ServiceHandle->base_section_1;
    ServiceHandle->nDescriptorsAllocated = 0;
 
    ServiceHandle->free_section_2_p =
        (STSUBT_OSD_RegionDescriptor_t*)ServiceHandle->base_section_2;
    ServiceHandle->nRegionsAllocated = 0;
 
    ServiceHandle->free_section_3_p =
        (STSUBT_OSD_DisplayRegion_t*)ServiceHandle->base_section_3;
    ServiceHandle->nDisplayRegionsAllocated = 0;
 
    ServiceHandle->free_section_4_p =
        (STOSD_RegionHandle_t*)ServiceHandle->base_section_4;
    ServiceHandle->nRegionsHandlersAllocated = 0;

    ServiceHandle->free_section_5_p =
        (STSUBT_OSD_PaletteDescriptor_t*)ServiceHandle->base_section_5;
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
 
static void DeleteServiceHandle(STSUBT_OSD_ServiceHandle_t *ServiceHandle)
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
 
static __inline STSUBT_OSD_DisplayDescriptor_t * 
NewDisplayDescriptor (STSUBT_OSD_ServiceHandle_t *ServiceHandle)
{
    STSUBT_OSD_DisplayDescriptor_t *descriptor_p;
 
    /* Get a descriptor from ServiceHandle (section 1) */
 
    descriptor_p = ServiceHandle->free_section_1_p;

    /* update control information */

    ServiceHandle->nDescriptorsAllocated = 
    (ServiceHandle->nDescriptorsAllocated+1) % ServiceHandle->MaxNumDescriptors;

    if (ServiceHandle->nDescriptorsAllocated == 0)
    {
        ServiceHandle->free_section_1_p = 
            (STSUBT_OSD_DisplayDescriptor_t*)ServiceHandle->base_section_1;
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
 *           will be identified throught the OSD region handle (U32 value).
 * Return  : A pointer to allocated Palette Descriptor (always successes)
 * Note    : Palette Descriptors are just use to optimize palette entry updates
\******************************************************************************/
 
static STSUBT_OSD_PaletteDescriptor_t *
AddToPaletteList (STOSD_RegionHandle_t RegionHandle, 
                  STSUBT_OSD_ServiceHandle_t *ServiceHandle)
{
    STSUBT_OSD_PaletteDescriptor_t *PaletteDescriptor;
 
    /* Get a Palette descriptor from ServiceHandle (section 5) */
 
    PaletteDescriptor = ServiceHandle->free_section_5_p;
 
    /* update control information */
 
    ServiceHandle->nPaletteDescriptorsAllocated =
      (ServiceHandle->nPaletteDescriptorsAllocated+1) 
        % ServiceHandle->MaxNumPaletteDescriptors;
 
    if (ServiceHandle->nPaletteDescriptorsAllocated == 0)
    {
        ServiceHandle->free_section_5_p =
            (STSUBT_OSD_PaletteDescriptor_t*)ServiceHandle->base_section_5;
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
 *           Palettes are identified by the OSD region handle (U32 value).
 * Return  : A pointer to found PaletteDescriptor, NULL if PaletteDescriptor 
 *           was not found
\******************************************************************************/
 
static __inline STSUBT_OSD_PaletteDescriptor_t *
GetPaletteDescriptor (STOSD_RegionHandle_t RegionHandle,
                      STSUBT_OSD_ServiceHandle_t *ServiceHandle)
{
  STSUBT_OSD_PaletteDescriptor_t *EpochPaletteList;
 
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
 * Function: NewOSDRegion
 * Purpose : Allocate a new OSD Region and a new Region Descriptor including 
 *           it in the EpochRegionList
 * Parameters:
 * Return  : 
\******************************************************************************/
 
static 
STOSD_RegionHandle_t NewOSDRegion (U8 region_id, 
                             STOSD_RegionParams_t *RegionParams,
                             STSUBT_OSD_ServiceHandle_t *ServiceHandle)
{
  STSUBT_OSD_RegionDescriptor_t *RegionDescriptor;
  STOSD_RegionHandle_t          *RegionHandle;
  STOSD_Color_t                 *Palette_p;
  ST_ErrorCode_t                 res;
 
  /* Allocate an OSD RegionHandle from ServiceHandle (section 4) */
 
  RegionHandle = ServiceHandle->free_section_4_p;

  /* update control information */

  ServiceHandle->nRegionsHandlersAllocated =
      (ServiceHandle->nRegionsHandlersAllocated+1) 
          % ServiceHandle->MaxNumRegionHandlers;

  if (ServiceHandle->nRegionsHandlersAllocated == 0)
  {
      ServiceHandle->free_section_4_p =
          (STOSD_RegionHandle_t*)ServiceHandle->base_section_4;
  }
  else
  {
      ServiceHandle->free_section_4_p = ServiceHandle->free_section_4_p + 1;
  }
  
  /* --- get region position and size satisfying hw constraints --- */
 
  if ((res = STOSD_AlignRegion(RegionParams)) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### NewOSDRegion: Error Aligning region (%d)",res));
      return (0);
  }

  /* --- Identify Region Palette --- */

  switch (RegionParams->NbBitsPerPel) {
      case 2: Palette_p = two_bit_entry_default_Palette;
              break;
      case 4: Palette_p = four_bit_entry_default_Palette;
              break;
      case 8: Palette_p = eight_bit_entry_default_Palette;
              break;
      default:Palette_p = NULL;
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### NewOSDRegion: unsupported depth (%d)",
                         RegionParams->NbBitsPerPel));
              return (0);
  }
 
  /* --- Create osd region --- */

  if ((res = STOSD_CreateRegion(RegionParams, RegionHandle)) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### NewOSDRegion: Error creating OSD region (%d)",res));
      return (0);
  }

  /* --- Set Region Palette --- */

  res = STOSD_SetRegionPalette(*RegionHandle, Palette_p);
  if (res != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### NewOSDRegion: Error setting palette (%d)\n", res));
      return (*RegionHandle);
  }

  /* --- Allocate a Region Item from section 2 --- */
 
  RegionDescriptor = ServiceHandle->free_section_2_p;
 
  /* --- Update control information --- */

  ServiceHandle->nRegionsAllocated =
      (ServiceHandle->nRegionsAllocated + 1) % ServiceHandle->MaxNumRegions;
 
  if (ServiceHandle->nRegionsAllocated == 0)
  {
      ServiceHandle->free_section_2_p = 
           (STSUBT_OSD_RegionDescriptor_t*)ServiceHandle->base_section_2;
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
 
static __inline STSUBT_OSD_RegionDescriptor_t *
GetRegionDescriptor (U8 region_id, STSUBT_OSD_ServiceHandle_t *ServiceHandle)
{
  STSUBT_OSD_RegionDescriptor_t *EpochRegionList;
 
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
void AddRegionToDisplay (STSUBT_OSD_RegionDescriptor_t *RegionDescriptor,
                         STSUBT_OSD_ServiceHandle_t *ServiceHandle,
                         STSUBT_OSD_DisplayDescriptor_t *descriptor)
{
  STSUBT_OSD_DisplayRegion_t    *DisplayRegion;
 
  /* --- Allocate a Display Region Item from section 3 --- */
 
  DisplayRegion = ServiceHandle->free_section_3_p;
 
  /* --- Update control information --- */
 
  ServiceHandle->nDisplayRegionsAllocated =
      (ServiceHandle->nDisplayRegionsAllocated + 1) 
          % ServiceHandle->MaxNumDisplayRegions;
 
  if (ServiceHandle->nDisplayRegionsAllocated == 0)
  {
      ServiceHandle->free_section_3_p =
           (STSUBT_OSD_DisplayRegion_t*)ServiceHandle->base_section_3;
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


/******************************************************************************\
 * Function: UpdateRegionBitmap
 * Purpose : Update region bitmap contents, but only if really needed
 *           i.e. different version numbers
 * Return  : ST_NO_ERROR on success
\******************************************************************************/
 
static __inline ST_ErrorCode_t
UpdateRegionBitmap (STOSD_RegionHandle_t *RegionHandle,
                    STSUBT_Bitmap_t      *SUBT_Bitmap)
{
  STOSD_Bitmap_t OSD_Bitmap;
 
  OSD_Bitmap.BitmapType = STOSD_BITMAP_TYPE_DIB_BYTE_ALIGN;
  OSD_Bitmap.ColorType = STOSD_COLOR_TYPE_PALETTE;
  OSD_Bitmap.BitsPerPel = SUBT_Bitmap->BitsPerPixel;
  OSD_Bitmap.Width = SUBT_Bitmap->Width;
  OSD_Bitmap.Height = SUBT_Bitmap->Height;
  OSD_Bitmap.Data_p = SUBT_Bitmap->DataPtr;
 
  return (STOSD_PutRectangle(*RegionHandle, 0, 0, &OSD_Bitmap));
}
 

/******************************************************************************\
 * Function: STSUBT_OSD_InitializeService
 * Purpose : To be called before any display is achived  
 *           This is the implementation of STSUBT_InitializeService callback 
 *           function related to OSD device
 *           Purpose of this function is mainly to create and initialize
 *           the Service Handle
 * Arguments:
 *     - MemoryPartition: Partition from which memory will be allocated.
 * Return  : A pointer to allocated Displaying Service Handle
\******************************************************************************/
 
void *STSUBT_OSD_InitializeService (STSUBT_DisplayServiceParams_t *DisplayServiceParams)
{
  STSUBT_OSD_ServiceHandle_t *ServiceHandle;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n\n#### STSUBT_OSD_InitializeService: Called"));

  /* --- Initialize default Palettes --- */

  init_2_bit_entry_default_Palette();
  init_4_bit_entry_default_Palette();
  init_8_bit_entry_default_Palette();

  /* --- Create Service Handle --- */

  ServiceHandle = NewServiceHandle (DisplayServiceParams->MemoryPartition,
                                    STSUBT_OSD_NUM_DESCRIPTORS,
                                    STSUBT_OSD_REGION_PER_EPOCH,
                                    STSUBT_OSD_REGION_PER_DISPLAY);
  if (ServiceHandle == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"#### OSD_Initialize_Display: Error creating Service Handle"));
      return (NULL);
  }

  if (STOSD_EnableDisplay() != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"STSUBT_OSD_InitializeService: Error Enabling OSD"));
      return (NULL);
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"#### STSUBT_OSD_InitializeService: Service Initialized"));

  clocks_per_sec = ST_GetClocksPerSecond();

  return ((void*)ServiceHandle);
}


/******************************************************************************\
 * Function: STSUBT_OSD_TerminateService
 * Purpose : This is the implementation of STSUBT_Terminate_Display callback  
 *           function related to OSD device 
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t STSUBT_OSD_TerminateService (void *ServiceHandle)
{  
  STSUBT_OSD_ServiceHandle_t *OSD_ServiceHandle;
  STSUBT_OSD_RegionDescriptor_t    *EpochRegionList;
  ST_ErrorCode_t              res = ST_NO_ERROR;  
 
  OSD_ServiceHandle = (STSUBT_OSD_ServiceHandle_t*)ServiceHandle;

  semaphore_wait(OSD_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n\n#### STSUBT_OSD_TerminateService: Called\n"));

  /* --- Delete all allocated regions --- */

  EpochRegionList = OSD_ServiceHandle->EpochRegionList;
  while (EpochRegionList != NULL)
  {
      res = STOSD_DeleteRegion(*(EpochRegionList->RegionHandle));
      if (res != ST_NO_ERROR)
      {
          STTBX_Report(
          (STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_TerminateService: error deleting region (%d)",res));
      }
      EpochRegionList = EpochRegionList->NextRegion_p;
  }
 
  semaphore_signal(OSD_ServiceHandle->DisplaySemaphore);

  /* --- delete the Service Handle --- */

  DeleteServiceHandle (OSD_ServiceHandle);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"#### STSUBT_OSD_TerminateService: Service Terminated\n"));

  return (res);
}


/******************************************************************************\
 * Function: STSUBT_OSD_PlanEpoch
 * Purpose : Plan the display service for the starting epoch
 * Parameters:
 *     PageComposition_p:
 *           A Page Composition of Mode Change. It contains the list of all
 *           regions planned for the starting epoch.
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t STSUBT_OSD_PlanEpoch (
                      PCS_t *PageComposition_p,
                      STSUBT_OSD_ServiceHandle_t *OSD_ServiceHandle)
{
  STSUBT_OSD_RegionDescriptor_t  *EpochRegionList;
  STOSD_RegionHandle_t            RegionHandle;
  ST_ErrorCode_t                  ReturnCode = ST_NO_ERROR;
  ST_ErrorCode_t                  res;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"#### STSUBT_OSD_PlanEpoch: Planning epoch\n"));

  /* Current display is disabled,
   * old regions are deleted and new regions created.
   * New regions are linked to create an EpochRegionList.
   */

  /* --- Delete OSD regions created for the just finished epoch --- */

  EpochRegionList = OSD_ServiceHandle->EpochRegionList;
  while (EpochRegionList != NULL)
  {
      RegionHandle = *EpochRegionList->RegionHandle;
      res = STOSD_DeleteRegion(RegionHandle);
      if (res != ST_NO_ERROR)
      {
          STTBX_Report(
          (STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_PlanEpoch: error deleting region %d\n", res));
          ReturnCode = 3;
      }
      EpochRegionList = EpochRegionList->NextRegion_p;
  }

  /* task delayed in order to allow OSD to update register */

  task_delay(clocks_per_sec / 25);

  /* --- reset the Service Handle --- */

  ResetServiceHandle(OSD_ServiceHandle);

  /* --- Create OSD regions --- */

  {
      STOSD_RegionParams_t RegionParams;
      STSUBT_OSD_PaletteDescriptor_t *PaletteDescriptor;
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
       * and create the new OSD epoch regions 
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
          STTBX_Report(
          (STTBX_REPORT_LEVEL_USER1,"#### STSUBT_OSD_PlanEpoch: processing region (%d)\n", region_id));

          /* fill in RegionParams */

          RegionParams.PositionX    = horizontal_position;
          RegionParams.PositionY    = vertical_position;
          RegionParams.Width        = region_width;
          RegionParams.Height       = region_height;
          RegionParams.ColorType    = STOSD_COLOR_TYPE_PALETTE;
          RegionParams.NbBitsPerPel = region_depth;
          RegionParams.Recordable   = TRUE;
          RegionParams.MixWeight    = 0x3f; /* DDTS 10770 and 10419, akem */
          RegionParams.HResolution  = STOSD_HORIZONTAL_RESOLUTION_FULL;
          RegionParams.VResolution  = STOSD_VERTICAL_RESOLUTION_FULL;
          
          /* create a new OSD region, 
           * allocate a new Region descriptor 
           * and include it in the EpochRegionList 
           */

          RegionHandle = 
              NewOSDRegion(region_id, &RegionParams, OSD_ServiceHandle);
          if (RegionHandle == 0)
          {
              ReturnCode = 1;
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_PlanEpoch: Cannot create region\n"));
          }

          /* --- Add region palette to EpochPaletteList (if not) --- */

          PaletteDescriptor = GetPaletteDescriptor (RegionHandle, 
                                                    OSD_ServiceHandle);
          if (PaletteDescriptor == NULL)
          {
              PaletteDescriptor = 
                  AddToPaletteList (RegionHandle, OSD_ServiceHandle);
          }

          /* --- Update region Palette, but only if CLUT --- */
          /* --- contents changed (different versions)   --- */

          /* check if CLUT contents changed */
 
          if (region_clut->CLUT_version_number != 
              PaletteDescriptor->Palette_version)
          { 
              res = UpdateRegionPalette(RegionHandle,region_clut,region_depth);
              if (res != ST_NO_ERROR)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### Error updating palette (%d)\n", res));
              }

              /* update palette version */
 
              PaletteDescriptor->Palette_version = 
                  region_clut->CLUT_version_number;
          }
          else
          {
              STTBX_Report((STTBX_REPORT_LEVEL_USER1,"#### STSUBT_OSD_PlanEpoch: CLUT contents unchanged"));
          }
      }
  }

  OSD_ServiceHandle->Starting = FALSE;

  return (ReturnCode);
}   


/******************************************************************************\
 * Function: STSUBT_OSD_PrepareDisplay
 * Purpose : This is the implementation of STSUBT_PrepareDisplay callback
 *           function related to OSD device
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t STSUBT_OSD_PrepareDisplay (
                      PCS_t *PageComposition_p,
                      void  *ServiceHandle,
                      void **DisplayDescriptor)
{
  STSUBT_OSD_ServiceHandle_t *OSD_ServiceHandle;
  ST_ErrorCode_t              res = ST_NO_ERROR;
 
  OSD_ServiceHandle = (STSUBT_OSD_ServiceHandle_t*)ServiceHandle;

  semaphore_wait(OSD_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n\n#### STSUBT_OSD_PrepareDisplay: Called\n"));

  /* --- check the acquisition mode of PCS --- */

  if ((PageComposition_p->acquisition_mode == STSUBT_MODE_CHANGE)
  ||  (OSD_ServiceHandle->Starting == TRUE))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"#### STSUBT_OSD_PrepareDisplay: STSUBT_MODE_CHANGE\n"));

      /* --- Plan the display service for the starting epoch --- */

      res = STSUBT_OSD_PlanEpoch (PageComposition_p, OSD_ServiceHandle);

      if (res != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_PrepareDisplay: error planning epoch\n"));
          semaphore_wait(OSD_ServiceHandle->DisplaySemaphore);
          return (res);
      }
  }   

  /* --- get a new display descriptor --- */

  *DisplayDescriptor = NewDisplayDescriptor(OSD_ServiceHandle);

  /* --- process page composition display --- */

  {
      /* The VisibleRegion_p field in a PCS 
       * contains the list of regions visible in the current display set.
       */

      /* Update Display Descriptor to include the provided list of regions. */
 
      STSUBT_OSD_PaletteDescriptor_t *PaletteDescriptor;
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

      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"#### STSUBT_OSD_PrepareDisplay: STSUBT_NORMAL_CASE\n"));

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
          STSUBT_OSD_RegionDescriptor_t *RegionDescriptor_p;
          STOSD_RegionHandle_t          *RegionHandle;
 
          /* check if at least a video line is left between
           * adjacent regions (8bpp only)
           */
 
          if (LastVerticalPosition >= vertical_position)
          {
              if (region_depth == 8)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"#### STSUBT_OSD_PrepareDisplay: region abutted"));
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
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_PrepareDisplay: outside screen\n"));
              continue;
          }
 
          /* Find corresponding RegionDescriptor into EpochRegionList */

          RegionDescriptor_p = GetRegionDescriptor(region_id, ServiceHandle);
          if (RegionDescriptor_p == NULL)
          {
              STTBX_Report(
              (STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_PrepareDisplay:region not found %d",region_id));
              continue;
          }

          /* set region coordinates in region descriptor */

          RegionDescriptor_p->horizontal_position = horizontal_position;
          RegionDescriptor_p->vertical_position = vertical_position;

          /* Add RegionDescriptor to the DisplayDescriptor RegionList */
 
          AddRegionToDisplay (RegionDescriptor_p, OSD_ServiceHandle, 
                             *DisplayDescriptor);
 
          /* --- Get region Handle --- */
          
          RegionHandle = RegionDescriptor_p->RegionHandle;

          /* --- Update regions contents and Palette entries, but only --- */
          /* --- if they really changed (different region versions)    --- */
          /* --- Note: clut and region contents may have changed only  --- */
          /* --- version changed.                                      --- */
         
          if (RegionDescriptor_p->region_version == region_version) 
          {
              STTBX_Report(
              (STTBX_REPORT_LEVEL_USER1,"#### STSUBT_OSD_PrepareDisplay: Region contents unchanged\n"));
              continue;
          }

          /* --- Update region version --- */

          RegionDescriptor_p->region_version = region_version;

          /* --- Update region Palette, but only if CLUT --- */
          /* --- contents changed (different versions)   --- */
 
          PaletteDescriptor = 
              GetPaletteDescriptor (*RegionHandle, OSD_ServiceHandle);

          if (region_clut->CLUT_version_number != 
              PaletteDescriptor->Palette_version)
          {
              /* update palette contents */

              res = UpdateRegionPalette(*RegionHandle,region_clut,region_depth);
              if (res != ST_NO_ERROR)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### Error updating palette (%d)\n", res));
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
              (STTBX_REPORT_LEVEL_USER1,"#### STSUBT_OSD_PrepareDisplay: bitmap not changed\n\n"));
              continue;
          }
          
          res = UpdateRegionBitmap(RegionHandle, region_bitmap);
          if (res != ST_NO_ERROR)
          {
              STTBX_Report(
              (STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_PrepareDisplay: Error bitmap update %d\n",res));
              continue;
          }

          /* --- reset bitmap update flag --- */

          region_bitmap->UpdateFlag = FALSE;

      } /* end while */
  }   

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"#### STSUBT_OSD_PrepareDisplay: Display Prepared\n"));

  semaphore_signal(OSD_ServiceHandle->DisplaySemaphore);

  return (res);
}


/******************************************************************************\
 * Function: STSUBT_OSD_ShowDisplay
 * Purpose : This the implementation of STSUBT_ShowDisplay callback
 *           function related to OSD device
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t STSUBT_OSD_ShowDisplay (void *ServiceHandle, 
                                       void *DisplayDescriptor)
{
  STSUBT_OSD_DisplayDescriptor_t *OSD_DisplayDescriptor;
  STSUBT_OSD_DisplayRegion_t     *RegionList;
  STSUBT_OSD_ServiceHandle_t     *OSD_ServiceHandle;
  STOSD_RegionHandle_t           *RegionHandle;
  U16                             horizontal_position;
  U16                             vertical_position;
  ST_ErrorCode_t                  res = ST_NO_ERROR;
 
  OSD_ServiceHandle = (STSUBT_OSD_ServiceHandle_t*)ServiceHandle;
  OSD_DisplayDescriptor = (STSUBT_OSD_DisplayDescriptor_t*)DisplayDescriptor;

  semaphore_wait(OSD_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n\n#### STSUBT_OSD_ShowDisplay: Showing display\n"));

  /* --- refresh active display (if any) --- */

  if (OSD_ServiceHandle->ActiveDisplayDescriptor != NULL)
  {
      STSUBT_OSD_DisplayRegion_t *RegionList;

      /* --- Change status of current displayed regions to unvisible --- */

      RegionList = (OSD_ServiceHandle->ActiveDisplayDescriptor)->RegionList;

      while (RegionList != NULL)
      {
          res=STOSD_HideRegion(*((RegionList->RegionDescriptor)->RegionHandle));
          if (res != ST_NO_ERROR)
          {
              STTBX_Report(
              (STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_ShowDisplay: error hiding region (%d)\n", res));
          }
          RegionList = RegionList->NextDisplayRegion_p;
      }

      /* --- reset active display descriptor in ServiceHandle --- */

      OSD_ServiceHandle->ActiveDisplayDescriptor = NULL;
  }

  /* --- Change status of new display regions to visible --- */
 
  RegionList = OSD_DisplayDescriptor->RegionList;
  while (RegionList != NULL)
  {

      RegionHandle = (RegionList->RegionDescriptor)->RegionHandle;
      vertical_position = (RegionList->RegionDescriptor)->vertical_position;
      horizontal_position = (RegionList->RegionDescriptor)->horizontal_position;

      /* --- set new region position --- */
      res = ST_NO_ERROR;

      if ((res != ST_NO_ERROR)
      ||  ((RegionList->RegionDescriptor)->old_horizontal_position != horizontal_position)
      ||  ( (RegionList->RegionDescriptor)->old_vertical_position != vertical_position) )
      {
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"#### STSUBT_OSD_ShowDisplay: Moving region\n"));

	(RegionList->RegionDescriptor)->old_vertical_position=vertical_position;
	(RegionList->RegionDescriptor)->old_horizontal_position=horizontal_position;

          if ((res = STOSD_MoveRegion(
                          *RegionHandle,
                           horizontal_position,
                           vertical_position)) != ST_NO_ERROR)
          {
              STTBX_Report(
              (STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_ShowDisplay: move failed (%d)\n", res));
              continue;
          }
      }

      res = STOSD_ShowRegion(*RegionHandle);
      if (res != ST_NO_ERROR)
      {
          STTBX_Report(
          (STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_ShowDisplay: error show Region (%d)\n", res));
      }
      RegionList = RegionList->NextDisplayRegion_p;
  }

  /* --- update active display descriptor in ServiceHandle --- */
 
  OSD_ServiceHandle->ActiveDisplayDescriptor = OSD_DisplayDescriptor;

  semaphore_signal(OSD_ServiceHandle->DisplaySemaphore);

  return (res);
}
 

/******************************************************************************\
 * Function: STSUBT_OSD_HideDisplay
 * Purpose : This the implementation of the STSUBT_HideDisplay callback
 *           function related to OSD display device
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t STSUBT_OSD_HideDisplay (void *ServiceHandle,
                                       void *DisplayDescriptor)
{
  ST_ErrorCode_t                  res = ST_NO_ERROR;
  STSUBT_OSD_DisplayRegion_t     *RegionList;
  STOSD_RegionHandle_t           *RegionHandle;
  STSUBT_OSD_DisplayDescriptor_t *OSD_DisplayDescriptor;
  STSUBT_OSD_ServiceHandle_t     *OSD_ServiceHandle;

  OSD_ServiceHandle = (STSUBT_OSD_ServiceHandle_t*)ServiceHandle;
  OSD_DisplayDescriptor = (STSUBT_OSD_DisplayDescriptor_t*)DisplayDescriptor;

  semaphore_wait(OSD_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n\n#### STSUBT_OSD_HideDisplay: Hiding display\n"));

  /* --- Disables all the regions currently displayed on the screen --- */

  /* Change status of current displayed regions to unvisible */

  RegionList = OSD_DisplayDescriptor->RegionList;
  while (RegionList != NULL)
  {  
      RegionHandle = (RegionList->RegionDescriptor)->RegionHandle;
      res = STOSD_HideRegion(*RegionHandle);
      if (res != ST_NO_ERROR)
      {
          STTBX_Report(
          (STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_OSD_HideDisplay: error hiding Region (%d)\n", res));
      }
      RegionList = RegionList->NextDisplayRegion_p;
  }
  
  /* --- reset the visible region list --- */

  OSD_DisplayDescriptor->RegionList = NULL;

  /* --- reset active display descriptor in ServiceHandle --- */

  OSD_ServiceHandle->ActiveDisplayDescriptor = NULL;

  semaphore_signal(OSD_ServiceHandle->DisplaySemaphore);
  
  return (res);
}

#endif /* end not def DISABLE_OSD_DISPLAY_SERVICE */
