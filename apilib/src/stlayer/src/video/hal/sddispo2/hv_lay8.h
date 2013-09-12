/*******************************************************************************

File name   : hv_lay8.h

Description : HAL video display sddispo2 family header file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
04 Nov 2002        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HAL_VIDEO_LAYER_DISPLAY_SDDISPO2_H
#define __HAL_VIDEO_LAYER_DISPLAY_SDDISPO2_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "halv_lay.h"
#include "hv_reg8.h"
#include "stsys.h"
#include "stgxobj.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* Debug Options -------------------------------------------------------- */

/* Use this define to track corruptions of the Horizontal/Vertical Filter Coeffs */
/*#define DBG_FILTER_COEFFS */

/* Use this define to see the VHSRC settings and to be able to modify them on the fly */
/*#define DBG_USE_MANUAL_VHSRC_PARAMS */

#ifdef VIDEO_DEBUG
    /* Use this define monitor the field inversions */
    #define DBG_FIELD_INVERSIONS
#endif

/* Exported Constants -------------------------------------------------------- */


/* Work around management section.                                             */
#if defined (HW_5528)

#define ST_5528_CUT_10  /* Define the current cut of targetted chip.           */

/* Work around to be applied on cut 1.x */
#ifdef ST_5528_CUT_10
/* All theses bugs are seen in STi5528 cut 1.x and should be resolved in cut 2.x            */
/*#define WA_GNBvd16935 */  /* Corrupted hor./ver. lines with zoom factor steps less than 1/32 */
/* NB concerning the implementation of WA. If InputWidth(Height) is greater than            */
/* OutputWidth(Height) (i.e. zoom out), the input window may be clipped. If not (i.e zoom   */
/* in) the output window will be clipped. */
#define WA_GNBvd25658 /* Hardware limitation: Repeat line can not be greater than 2 */
/* Workaround activated but it is not possible to set the correct phase. The entire image */
/* may be jerky colored as the line repeat number should be 3 with another phase. */
#endif /* ST_5528_CUT_10 */
#endif /* HW_5528 */

#if defined (HW_7710)
#if defined(STI7710_CUT2x)
#define WA_GNBvd39777   /* Display is corrupted when LMU is enabled */      /* Should be removed for Cut 2.0 */
#endif
#define WA_GNBvd39528   /* Work-around for DDTS GNBvd39527 and GNBvd39528 "HD Display Pan/Scan - Green Flashes and chroma shift", both seen on STi7710 and STi7100 (not STi5528) */
#endif /* HW_7710 */

#if defined (HW_7100)
#define WA_GNBvd39528   /* Work-around for DDTS GNBvd39527 and GNBvd39528 "HD Display Pan/Scan - Green Flashes and chroma shift", both seen on STi7710 and STi7100 (not STi5528) */
#define WA_SHIFT_TINT   /* Work-around for TINT hard ware issue: left shift done by the hard*/
#endif /* HW_7100 */

#if defined (HW_7109)
#define WA_GNBvd39528   /* Work-around for DDTS GNBvd39527, GNBvd39528 and GNBvd48201 "HD Display Pan/Scan - Green Flashes and chroma shift", seen on STi7710, STi7100 and 7109 */
#define WA_SHIFT_TINT   /* Work-around for TINT hard ware issue: left shift done by the hard*/
#endif /* HW_7109 */

#define HFC_NB_COEFS                35
#define VFC_NB_COEFS                22

#define NO_ZOOM_FACTOR              (1<<13)     /* As SRC are 3.13 (2^13)=8192 */


#define LUMA_VERTICAL_MAX_ZOOM_OUT   (4 * NO_ZOOM_FACTOR)  /* Max zoom out is 4 */
#define LUMA_VERTICAL_NO_ZOOM        (1 * NO_ZOOM_FACTOR)  /* no zoom */

#define LUMA_HORIZONTAL_MAX_ZOOM_OUT (4 * NO_ZOOM_FACTOR)  /* Max zoom out is 4 */


/* ******   LMU  MANAGEMENT   ************************************************** */
#define HALLAYER_LMU_MAX_WIDTH      720
#define HALLAYER_LMU_MAX_HEIGHT     576

/* Max Luma Data Rate in MB/s */
#define MAX_LUMA_DATA_RATE      100

/* Exported Macros ---------------------------------------------------------- */
#define HAL_Read32(Address_p)               STSYS_ReadRegDev32LE ((void *) (Address_p))
#define HAL_Write32(Address_p, Value)       STSYS_WriteRegDev32LE((void *) (Address_p), (Value))

#define HAL_WriteDisp32(Address_p, Value)\
       STSYS_WriteRegDev32LE((void *) (((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p) + Address_p), (Value))

#define HAL_ReadDisp32(Address_p)\
       STSYS_ReadRegDev32LE((void *) (((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p) + Address_p))

#define HAL_WriteGam32(Address_p, Value)\
       STSYS_WriteRegDev32LE((void *) (((U8 *)LayerProperties_p->GammaBaseAddress_p) + Address_p), (Value))

#define HAL_ReadGam32(Address_p)\
       STSYS_ReadRegDev32LE((void *) (((U8 *)LayerProperties_p->GammaBaseAddress_p) + Address_p))

#define HAL_SetRegister32DefaultValue(Address_p, RegMask, Value)                                \
{                                                                                               \
       U32 Tmp32;                                                                                  \
       Tmp32 = HAL_Read32 (Address_p);                                                             \
       HAL_Write32(Address_p, (Tmp32 & (~RegMask)) | (Value & RegMask));                           \
}
#define HAL_SetRegister32Value(Address_p, RegMask, Mask, Emp, Value)                            \
{                                                                                               \
       U32 Tmp32;                                                                                  \
       Tmp32 = HAL_Read32 (Address_p);                                                             \
       HAL_Write32 (Address_p, (Tmp32 & RegMask & (~((U32)Mask << Emp)) ) | ((U32)Value << Emp));  \
    }

/* Exported Types ----------------------------------------------------------- */
typedef struct
{
    U32                           Width;
    U32                           Height;
} HALLAYER_Dimensions_t;

typedef struct
{
    U32                         XStart;
    U32                         YStart;
    U32                         XOffset;
    U32                         YOffset;
} HALLAYER_Position_t;

typedef struct
{
    U32 LumaVertical;
    U32 LumaHorizontal;
    U32 ChromaVertical;
    U32 ChromaHorizontal;
} HALLAYER_Increment_t;

typedef struct
{
    U32 LumaVerticalTop;
    U32 LumaVerticalBottom;
    U32 ChromaVerticalTop;
    U32 ChromaVerticalBottom;
    U32 LumaHorizontal;
    U32 ChromaHorizontal;
} HALLAYER_Phase_t;

typedef struct
{
    U32 LumaTop;
    U32 LumaBottom;
    U32 ChromaTop;
    U32 ChromaBottom;
    U32 LumaProgressive;
    U32 ChromaProgressive;
} HALLAYER_FirstPixelRepetition_t;

typedef struct
{
    U32 X1;
    U32 X2;
    U32 Y1Top;
    U32 Y1Bottom;
    U32 Y2Top;
    U32 Y2Bottom;
} HALLAYER_MemoryPosition_t;

typedef struct
{
    U32 MemoryPosition;
    U32 Phase;
    U32 FirstPixelRepetition;
} HALLAYER_PolyphaseFilterSettings_t;

typedef struct
{
    HALLAYER_MemoryPosition_t         MemoryPosition;
    HALLAYER_Phase_t                  Phase;
    HALLAYER_FirstPixelRepetition_t   FirstPixelRepetition;
} HALLAYER_PositionDefinition_t;

typedef struct HALLAYER_FormatInfo_s
{
    BOOL   IsFormat422Not420;
    BOOL   IsInputScantypeProgressive;         /* Picture Scantype */
    BOOL   IsFormatRasterNotMacroblock;
} HALLAYER_FormatInfo_t;

typedef struct
{
    U32 Height1Top;
    U32 Height1Bottom;
    U32 Height2Top;
    U32 Height2Bottom;
    U32 Width1;
    U32 Width2;
} HALLAYER_MemorySize_t;

typedef struct
{
    U32 RegDISP_CTL;
    U32 RegDISP_LUMA_HSRC;
    U32 RegDISP_LUMA_VSRC;
    U32 RegDISP_CHR_HSRC;
    U32 RegDISP_CHR_VSRC;
    U32 RegDISP_TARGET_SIZE;
    U32 RegDISP_NLZZD_Y;
    U32 RegDISP_NLZZD_C;
    U32 RegDISP_PDELTA;
    U32 RegDISP_MA_CTL;
    U32 RegDISP_LUMA_SIZE;
    U32 RegDISP_CHR_SIZE;
    U32 RegDISP_HFP;
    U32 RegDISP_VFP;
    U32 RegDISP_PKZ;
} HALLAYER_RegisterMap_t;


typedef struct
{
    HALLAYER_RegisterMap_t RegisterMapTop;
    HALLAYER_RegisterMap_t RegisterMapBottom;
} HALLAYER_RegisterConfiguration_t;

typedef struct
{
    STAVMEM_BlockHandle_t       HorizontalBlockHandle;                  /* Horizontal Filters buffer avmem allocation/deallocation. */
    STAVMEM_BlockHandle_t       VerticalBlockHandle;                    /* Vertical Filters buffer avmem allocation/deallocation.   */
    void *                      HorizontalAddress_p;                    /* Horizontal Filters buffer address.                       */
    void *                      VerticalAddress_p;                      /* Vertical Filters buffer address.                         */

    STAVMEM_BlockHandle_t       AlternateHorizontalBlockHandle;         /* Alternate Horizontal Filters buffer avmem allocation/deallocation. */
    STAVMEM_BlockHandle_t       AlternateVerticalBlockHandle;           /* Alternate Vertical Filters buffer avmem allocation/deallocation.   */
    void *                      AlternateHorizontalAddress_p;           /* Alternate Horizontal Filters buffer address.                       */
    void *                      AlternateVerticalAddress_p;             /* Alternate Vertical Filters buffer address.                         */

    void *                      HorizontalAddressUsedByHw_p;            /* Horizontal Filters buffer address currently used by hardware.      */
    void *                      HorizontalAddressAvailForWritting_p;    /* Horizontal Filters buffer address available for writting           */
    void *                      VerticalAddressUsedByHw_p;              /* Vertical Filters buffer address currently used by hardware.      */
    void *                      VerticalAddressAvailForWritting_p;      /* Vertical Filters buffer address available for writting           */
} HALLAYER_FilterBuffer_t;

typedef enum {
    FILTER_NO_SET,
    FILTER_NULL,
    FILTER_A,
    FILTER_B,
    FILTER_Y_C,
    FILTER_C_C,
    FILTER_Y_D,
    FILTER_C_D,
    FILTER_Y_E,
    FILTER_C_E,
    FILTER_Y_F,
    FILTER_C_F,
    FILTER_H_CHRX2,
    FILTER_V_CHRX2
} HALLAYER_Filter_t;

typedef struct
{
    HALLAYER_FilterBuffer_t FilterBuffer;

    HALLAYER_Filter_t    CurrentVerticalLumaFilter;        /* Currently loaded vertical filters                      */
    HALLAYER_Filter_t    CurrentVerticalChromaFilter;      /* Currently loaded vertical filters                      */
    HALLAYER_Filter_t    CurrentHorizontalLumaFilter;      /* Currently loaded horizontal filters                    */
    HALLAYER_Filter_t    CurrentHorizontalChromaFilter;    /* Currently loaded horizontal filters                    */

    HALLAYER_RegisterConfiguration_t RegisterConfiguration;
} HALLAYER_DisplayInstance_t;

typedef struct
{
    /*************************                         Video LAYER parameters                         *************************/
    U8                          MaxViewportNumber;                  /* Maximum allowed opened viewports.                      */
    STLAYER_LayerParams_t       LayerParams;                        /* Layer parameters (Height, Width, ScanType, ...).       */
    U32                         VTGFrameRate;                       /* Output frame rate.                                     */
    ST_DeviceName_t             VTGName;                            /* VTG name                                               */
    HALLAYER_Position_t         LayerPosition;                      /* Complementary layer parameters (Start and Offset).     */
    void *                      ViewportProperties_p;               /* Viewport properties...                                 */
    BOOL                        PresentTopRegisters;
    U32                         LumaDataRateInBypassMode;           /* Luma Data Rate when DEI is Bypassed */
    U32                         VSyncCounter;                       /* Counter used to display periodical status */

    /*************************                       Video VIEWPORT parameters                        *************************/
    /* NB: These parameters should be viewport properties, but hardware doesn't allow separate viewport management. So the    */
    /*     viewport properties are store at hal_layer level. So only one viewport is then managerd.                           */
    STLAYER_ViewPortParams_t    ViewPortParams;                     /* Input/Output rectangle properties (position & size).   */
    BOOL                        IsViewportEnabled;                  /* Flag to know the enable/disable state of viewport.     */
    U32                         ViewportOffset;                     /* Viewport pos    (formula: (OutVStart<<16) | OutHStart) */
    U32                         ViewportSize;                       /* Viewport size   (formula: (OutHeight <<16) | OutWidth) */
    BOOL                        IsDecimationActivated;
    BOOL                        AdvancedHDecimation;
    STLAYER_DecimationFactor_t  RecommendedVerticalDecimation;
    STLAYER_DecimationFactor_t  RecommendedHorizontalDecimation;
    BOOL                        IsDisplayStopped;
    BOOL                        PeriodicFieldInversionDueToFRC;

    /*************************                       Video Source parameters                          *************************/
    HALLAYER_Dimensions_t       SourcePictureDimensions;            /* Source picture dimensions (Width and Height).          */
    STGXOBJ_ColorType_t         ColorType;                          /* Color type of the source picture.                      */
    HALLAYER_FormatInfo_t       SourceFormatInfo;
    BOOL                        UpdateViewport;                     /* Used to update VID plane registers */
    U32                         CurrentPictureHorizontalDecimationFactor;
    U32                         CurrentPictureVerticalDecimationFactor;

    /*************************  Post LMU Format Info    *************************/
    HALLAYER_FormatInfo_t       PostDeiFormatInfo;

    /*************************  Output Format Info    *************************/
    BOOL                        IsOutputScantypeProgressive;

    /*************************                       Display Instances                                *************************/
    HALLAYER_DisplayInstance_t  DisplayInstance[2];
    BOOL IsPeriodicalUpdateNeeded;
    BOOL ReloadOfHorizontalFiltersInProgress;                       /* Is a reload of horizontal filters in progress? */
    BOOL ReloadOfVerticalFiltersInProgress;                         /* Is a reload of vertical filters in progress? */

    /*************************                       Alloc/ Deallocations                             *************************/
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;               /* Used for avmem allocation/deallocation.                */

#if (defined (HW_7100) || defined (HW_7710)) && !defined(WA_GNBvd39777) && !defined(DISABLE_LMU)
    /************************                       Specific HDDISPO2 cell                            *************************/
    BOOL                    LmuBlockActivated;
    STAVMEM_BlockHandle_t   LmuChromaBlockHandle;               /* Used for LMU buffer avmem allocation/deallocation.         */
    STAVMEM_BlockHandle_t   LmuLumaBlockHandle;                 /* Used for LMU buffer avmem allocation/deallocation.         */
    void *                  LmuLumaBuffer_p;                    /* LMU luma buffer address.                                   */
    void *                  LmuChromaBuffer_p;                  /* LMU chroma buffer address.                                 */
#endif
    U32                     CurrentGlobalAlpha;
    U32                     StateVSyncForLMU;
#if defined(HW_7100) || defined(HW_7109)
    struct PSI_Parameters_s
    {
        STLAYER_PSI_t           CurrentPSIParameters[STLAYER_MAX_VIDEO_FILTER_POSITION];/* PSI parameters for the layer.      */
        U32                     ActivityMask;                       /* Internal mask to summurize PSI activity.               */

    }PSI_Parameters;
#endif /* defined(HW_7100) || defined(HW_7109) */

#ifdef DBG_USE_MANUAL_VHSRC_PARAMS
    /*************************                       Debug Vertical Pixels Position                   *************************/
    BOOL                        ManualParams;
    U32                         FirstPixelRepetitionLumaTop;
    U32                         FirstPixelRepetitionLumaBottom;
    U32                         PhaseRepetitionLumaTop;
    U32                         PhaseRepetitionLumaBottom;
#endif

#ifdef DBG_FIELD_INVERSIONS
    STLAYER_Field_t             OldCurrentField;
    U32                         FieldsWithFieldInversion;
    U32                         FieldsSetOnOddInputPosition;
    U32                         FieldsSetOnOddOutputPosition;
#endif
} HALLAYER_LayerCommonData_t;



/* Exported Variables ------------------------------------------------------- */
extern const HALLAYER_FunctionsTable_t HALLAYER_Xddispo2Functions;


/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_LAYER_DISPLAY_REGISTERS_SDDISPO2_H */

/* End of hv_lay8.h */
