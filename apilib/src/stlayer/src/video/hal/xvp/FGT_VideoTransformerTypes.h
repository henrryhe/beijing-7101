/*
**  @file     : FGT_VideoTransformerTypes.h
**
**  @brief    : FGT transformer specific types
**
**  @par OWNER: Didier SIRON / Didier SIRON
**
**  @author   : Didier SIRON / Didier SIRON
**
**  @par SCOPE:
**
**  @date     : November 3th 2005
**
**  &copy; 2005 ST Microelectronics. All Rights Reserved.
*/


/* Define to prevent recursive inclusion */
#ifndef __FGT_TRANSFORMERTYPES_H
#define __FGT_TRANSFORMERTYPES_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h" /* U32, BOOL... */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Defines --------------------------------------------------------- */

/* Identifies the API version used by the firmware. */

#define FGT_API_VERSION "1.2"

/* Flags definition (see Flags field inside FGT_TransformParam_t)

   If set, this bit indicates that the film grain process is bypassed =>
   - no LUTs or patterns are transfered into DMEM
   - nothing is sent to the external memory/VDP */
#define FGT_FLAGS_BYPASS_BIT                0
/* If set, this bit indicates that the firmware has to download the LUT tables (i.e the
   FGT_Lut_t record pointed by LutAddr).
   Otherwise, it means that the LUTs are the same as the ones used during the
   previous decode. */
#define FGT_FLAGS_DOWNLOAD_LUTS_BIT         2
/* If set, this bit indicates that firmware has to compute luma grain.
   Otherwise, it means that it has to send instead 0 to the external memory/VDP. */
#define FGT_FLAGS_LUMA_GRAIN_BIT            3
/* If set, this bit indicates that firmware has to compute Cb and Cr grain.
   Otherwise, it means that it has to send instead 0 to the external memory/VDP. */
#define FGT_FLAGS_CHROMA_GRAIN_BIT          4
/* If set, this bit indicates that firmware has to send the grain to the VDP. */
#define FGT_FLAGS_VDP_BIT                   5
/* If set, this bit indicates that firmware has to send the grain to the external memory. */
#define FGT_FLAGS_EXT_MEM_BIT               6
/* If set, this bit indicates that the DEI block is bypassed. */
#define FGT_FLAGS_BYPASS_DEI_BIT            7
/* If set, this bit indicates that the display is progressive.
   Otherwise, it means that the display is interlaced. */
#define FGT_FLAGS_PROGRESSIVE_DISPLAY_BIT   8
/* This bit has a meaning only if the display is interlaced
   (i.e. FGT_FLAGS_PROGRESSIVE_DISPLAY_BIT bit is cleared).
   If set, this bit indicates that the top lines of the grain shall be sent to the external memory/VDP.
   Otherwise, it means that the bottom lines of the grain shall be sent to the external memory/VDP. */
#define FGT_FLAGS_TOP_FIELD_BIT             9

#define FGT_FLAGS_BYPASS_MASK               (1 << FGT_FLAGS_BYPASS_BIT)
#define FGT_FLAGS_DOWNLOAD_LUTS_MASK        (1 << FGT_FLAGS_DOWNLOAD_LUTS_BIT)
#define FGT_FLAGS_LUMA_GRAIN_MASK           (1 << FGT_FLAGS_LUMA_GRAIN_BIT)
#define FGT_FLAGS_CHROMA_GRAIN_MASK         (1 << FGT_FLAGS_CHROMA_GRAIN_BIT)
#define FGT_FLAGS_VDP_MASK                  (1 << FGT_FLAGS_VDP_BIT)
#define FGT_FLAGS_EXT_MEM_MASK              (1 << FGT_FLAGS_EXT_MEM_BIT)
#define FGT_FLAGS_BYPASS_DEI_MASK           (1 << FGT_FLAGS_BYPASS_DEI_BIT)
#define FGT_FLAGS_PROGRESSIVE_DISPLAY_MASK  (1 << FGT_FLAGS_PROGRESSIVE_DISPLAY_BIT)
#define FGT_FLAGS_TOP_FIELD_MASK            (1 << FGT_FLAGS_TOP_FIELD_BIT)

/* Maximum number of grain patterns */
#define MAX_GRAIN_PATTERN   10

/* Size of a grain pattern (in bytes) */
#define GRAIN_PATTERN_SIZE  (4 * 1024)

/* Number of color space components */
#define MAX_COLOR_SPACE     3

/* Constants to access arrays indexed by MAX_COLOR_SPACE */
#define Y_INDEX             0
#define CB_INDEX            1
#define CR_INDEX            2

/* Maximum number of elements in a LUT (grain LUT or scaling LUT) */
#define MAX_LUT             256

/* Constants to decode FGT_TransformParam_t.PictureSize and
   FGT_TransformParam_t.ViewPortSize */
#define FGT_WIDTH_BIT       0
#define FGT_HEIGHT_BIT      16

#define FGT_WIDTH_MASK      (0x7ff << FGT_WIDTH_BIT)
#define FGT_HEIGHT_MASK     (0x7ff << FGT_HEIGHT_BIT)

/* Constants to decode FGT_TransformParam_t.ViewPortOrigin */
#define FGT_X_BIT           0
#define FGT_Y_BIT           16

#define FGT_X_MASK          (0x7ff << FGT_X_BIT)
#define FGT_Y_MASK          (0x7ff << FGT_Y_BIT)

/* Constants to decode FGT_TransformParam_t.LumaZoom */
#define FGT_HSRC_INCR_BIT   0
#define FGT_VSRC_INCR_BIT   16

#define FGT_HSRC_INCR_MASK  (0xffff << FGT_HSRC_INCR_BIT)
#define FGT_VSRC_INCR_MASK  (0xffff << FGT_VSRC_INCR_BIT)

/* Constants to decode FGT_TransformParam_t.UserZoomOut */
#define FGT_USER_ZOOM_BIT   0

#define FGT_USER_ZOOM_MASK  (0x1f << FGT_USER_ZOOM_BIT)

/* Constants to decode FGT_TransformParam_t.UserZoomOutThreshold */
#define FGT_USER_ZOOM_PROG_THR1_BIT     0
#define FGT_USER_ZOOM_PROG_THR2_BIT     8
#define FGT_USER_ZOOM_INTL_THR1_BIT     16
#define FGT_USER_ZOOM_INTL_THR2_BIT     24

#define FGT_USER_ZOOM_PROG_THR1_MASK    (0x1f << FGT_USER_ZOOM_PROG_THR1_BIT)
#define FGT_USER_ZOOM_PROG_THR2_MASK    (0x1f << FGT_USER_ZOOM_PROG_THR2_BIT)
#define FGT_USER_ZOOM_INTL_THR1_MASK    (0x1f << FGT_USER_ZOOM_INTL_THR1_BIT)
#define FGT_USER_ZOOM_INTL_THR2_MASK    (0x1f << FGT_USER_ZOOM_INTL_THR2_BIT)

/* Exported Types ----------------------------------------------------------- */

/* Status returned by the firmware in MBX_INFO_TO_HOST register. */

typedef enum
{
   /* The firmware has been sucessfull */
   FGT_NO_ERROR = 0,
   /* The Vsync interrupt arised and grain was not fully
      sent to the external memory/VDP */
   FGT_FIRM_TOO_SLOW = 1,
   /* A downgraded grain was produced. It can occur in different cases:
      - a too big zoom factor was set => a downgraded grain is produced in order
        to be real time.
      - the firmware was too slow during the previous Vsync. During the following Vsync,
        the firmware produces a downgraded grain in order to be sure to be realtime
        (because the firmware starts late). */
   FGT_DOWNGRADED_GRAIN = 2,
} FGT_Error_t;

/* Structure containing all the LUTs */

typedef struct {
   /* GrainLut[c][i] gives an offset to access to the 4K grain pattern for a 8x8 average equal to <i>
      and for the color component <c>. */
   U16  GrainLut[MAX_COLOR_SPACE][MAX_LUT];

   /* ScalingLut[c][i] gives the scaling value which shall be applied to the grain pattern for
      a 8x8 average equal to i and for the color component <c>. */
   U8   ScalingLut[MAX_COLOR_SPACE][MAX_LUT];
} FGT_Lut_t;

/* Structure containing all the decode parameters */

typedef struct {

   /* Bitfield to pass different information to the firmware.
      Note that at least FGT_FLAGS_VDP_BIT bit or FGT_FLAGS_EXT_MEM_BIT bit shall be set. */
   U32  Flags;

   /* Begin address (in external memory) of the luma accumulations for the whole
      picture (it doesn't take into account the viewport coordinates).
      These accumulations are stored in omega2 MB format.
      The address shall be aligned on a 512 bytes boundary (9 less significant bits equal to 0). */
   U32  LumaAccBufferAddr;
   /* Begin address (in external memory) of the chroma accumulations for the whole
      picture (it doesn't take into account the viewport coordinates).
      These accumulations are stored in omega2 MB format.
      The address shall be aligned on a 512 bytes boundary (9 less significant bits equal to 0). */
   U32  ChromaAccBufferAddr;

   /* Size of the picture (in pixels):
      - bits 26 to 16 contains the height (>= 1). Shall be a multiple of 16 pixels.
      - bits 10 to 0 contains the width (>= 1). Shall be a multiple of 16 pixels. */
   U32 PictureSize;
   /* Top left coorner coordinates of the viewport (in pixels):
      - bits 26 to 16 contains the Y coordinate (>= 0).
      - bits 10 to 0 contains the X coordinate (>= 0). */
   U32  ViewPortOrigin;
   /* Viewport size (in pixels):
      - bits 26 to 16 contains the height (>= 2).
      - bits 10 to 0 contains the width (>= 2). */
   U32  ViewPortSize;
   /* This field contains the vertical and horizontal luma sample rate converter
      state-machine increments, in 3.13 format.
      - bits 31 to 16 contains the vertical increment
        It shall have the same value as in the VHSRC_Y_VSRC register.
      - bits 15 to 0 contains the horizontal increment
        It shall have the same value as in the VHSRC_Y_HSRC register. */
   U32  LumaZoom;
   /* This field contains the user zoom out in 2.3 format (bits 4 to 0). */
   U32  UserZoomOut;
   /* This field contains the user zoom out thresholds, in 2.3 format:
      - bits 28 to 24 contains the second interlaced threshold
      - bits 20 to 16 contains the first interlaced threshold
      - bits 12 to 8 contains the second progressive threshold
      - bits  4 to 0 contains the first progressive threshold */
   U32  UserZoomOutThreshold;

   /* Number of elements in GrainPatternArray
      From 0 to MAX_GRAIN_PATTERN */
   U32  GrainPatternCount;
   /* GrainPatternAddrArray[i] is the address in external memory of a 4K section of memory
      containing a grain pattern. */
   U32  GrainPatternAddrArray[MAX_GRAIN_PATTERN];
   /* Address in external memory of a FGT_Lut_t record. */
   U32  LutAddr;

   /* Initial seed values for the whole picture. */
   U32  LumaSeed;
   U32  CbSeed;
   U32  CrSeed;

   /* Begin addresses (in external memory) of buffers where the firmware will store
      the grain it has computed. Storage in external memory will be done only if bit
      FGT_FLAGS_EXT_MEM_BIT is set (otherwise grain will be sent to the VDP).
      Grain is stored in 4:2:0 raster 3 buffers format. Only pixels belonging to
      the viewport are written in memory. */
   U32  LumaGrainBufferAddr;
   U32  CbGrainBufferAddr;
   U32  CrGrainBufferAddr;

   /* Logarithmic scale factor.
      Shall be in [2..7]. */
   U32 Log2ScaleFactor;
} FGT_TransformParam_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

#endif /* #ifndef __FGT_TRANSFORMERTYPES_H */
