/*******************************************************************************

File name   : layer2.h

Description : HAL video decode omega2 family private data definition

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
10 Jan 2003        Created                                           GG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HAL_VIDEO_PRIVATE_LAYER_OMEGA2_H
#define __HAL_VIDEO_PRIVATE_LAYER_OMEGA2_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "hv_reg.h"
#include "hv_filt.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Enable/Disable */
#define HALLAYER_DISABLE                                        0
#define HALLAYER_ENABLE                                         1

/* Number Max of Viewports */
#define MAX_MAIN_LAYER_VIEWPORTS_MAX_NB                         1       /* Because of harware limitations, the architecture   */
                                                                        /* of the API doesn't allow more than 1 viewports.    */
#define MAX_AUX_LAYER_VIEWPORTS_MAX_NB                          1

/* ************************************************************************** */
/* **********************   DISPLAY CLOCK MANAGEMENT   ********************** */
/* ************************************************************************** */
#define     SECURITY_COEFF                      98   /* Clock security coeff. */
#define     DEFAULT_DISPLAY_CLOCK               81000000

/* ******************* STi7015 DISPLAY/AUX DISPLAY clocks ******************* */
#define     STi7015_DISPLAY_CLOCK_SD            81000000
#define     STi7015_DISPLAY_CLOCK_HD            81000000
#define     STi7015_DISPLAY_CLOCK_SQUARE        81000000

#define     STi7015_AUX_DISPLAY_CLOCK_SD        81000000
#define     STi7015_AUX_DISPLAY_CLOCK_HD        81000000
#define     STi7015_AUX_DISPLAY_CLOCK_SQUARE    73350000

/* ******************* STi7020 DISPLAY/AUX DISPLAY clocks ******************* */
/* *************** STi7020 (cut 1) DISPLAY/AUX DISPLAY clocks *************** */
#ifdef IS_CHIP_STi7020_CUT_1
# define     STi7020_DISPLAY_CLOCK_SD           86400000
# define     STi7020_DISPLAY_CLOCK_HD           86400000
# define     STi7020_DISPLAY_CLOCK_SQUARE       86400000

# define     STi7020_AUX_DISPLAY_CLOCK_SD       72000000
# define     STi7020_AUX_DISPLAY_CLOCK_HD       72000000
# define     STi7020_AUX_DISPLAY_CLOCK_SQUARE   73350000
#endif
/* *************** STi7020 (cut 2) DISPLAY/AUX DISPLAY clocks *************** */
#ifdef IS_CHIP_STi7020_CUT_2
# define     STi7020_DISPLAY_CLOCK_SD           81000000
# define     STi7020_DISPLAY_CLOCK_HD           87000000
# define     STi7020_DISPLAY_CLOCK_SQUARE       63000000

# define     STi7020_AUX_DISPLAY_CLOCK_SD       72000000
# define     STi7020_AUX_DISPLAY_CLOCK_HD       72000000
# define     STi7020_AUX_DISPLAY_CLOCK_SQUARE   72000000
#endif
/* ************************************************************************** */

/* ************************************************************************** */
/* *************************   HFC/VFC  MANAGEMENT   ************************ */
/* ************************************************************************** */
#define HALLAYER_SCALING_FACTOR                8192

/* Maximum zoom out factor du to hardware limitation.         */
/* They are used as : x/10000                                 */
#define HFC_MAIN_DISPLAY_MAX_ZOOM_FACTOR       5000

/* Define the maximum allowed ratio between output and input. */
/* The theorical value is 33%, but we apply 36%.              */
#define HALLAYER_HORIZONTAL_MAX_RATIO           36

#define HALLAYER_MAX_AUXILIARY_DISPLAY_WIDTH    720

/* Filtering managemet.                                       */
typedef enum
{
    HALLAYER_NONE_FILTER,
    HALLAYER_A_FILTER,
    HALLAYER_B_FILTER,
    HALLAYER_C_FILTER
} HALLAYER_Filter_t;

/* Format filters parameters */
#define HFC_NB_PHASES                          32
#define HFC_NB_CHROMA_TAPS                     4
#define HFC_NB_LUMA_TAPS                       8
#define VFC_NB_PHASES                          32
#define VFC_NB_TAPS                            4

#define HFC_ZONE_NBR                           5           /* Number of zones for Main HFC  */
#define VFC_ZONE_NBR                           5           /* Number of zones for PIP HFC   */

/* Synchronized updates needed on next VSync ( use 1 bit for 1 update) */
#define     HORIZONTAL_A_FILTER             (1 << 0)
#define     HORIZONTAL_B_FILTER             (1 << 1)
#define     HORIZONTAL_C_FILTER             (1 << 2)
#define     VERTICAL_A_FILTER               (1 << 3)
#define     VERTICAL_B_FILTER               (1 << 4)
#define     VERTICAL_C_FILTER               (1 << 5)
#define     DCI_FILTER                      (1 << 6)
#define     GLOBAL_ALPHA                    (1 << 7)
/* ... may be continued                     (1 << n) */


/* ************************************************************************** */

/* ************************************************************************** */
/* ****************************   LMU  MANAGEMENT   ************************* */
/* ************************************************************************** */
/* LMU limitation in term of input stream size. */
#define HALLAYER_LMU_MAX_WIDTH       720
#define HALLAYER_LMU_MAX_HEIGHT      576

#define HALLAYER_LMU_USE_THRESHOLD_PROGRESSIVE   98  /* (percent basis) */
#define HALLAYER_LMU_USE_THRESHOLD_INTERLACED    90  /* (percent basis) */

#define HALLAYER_MAX_LMU_WIDTH                  720
/* ************************************************************************** */

/* ************************************************************************** */
/* ****************************   PSI  MANAGEMENT   ************************* */
/* ************************************************************************** */
/* Specific mode used to calculate parameters to set the dual segment         */
/* transfert function :                                                       */
#define WS_MODE     /* White stretch mode   */
#define BS_MODE     /* Black stretch mode   */
#define AP_MODE     /* Auto-Pix mode        */

/*  User preferences.           */

/*#define RESPONSE_TIME   16*/
/* Response time set to 4 because too many artefact seen if the brightness    */
/* changes quickly.                                                           */
#define RESPONSE_TIME   4

/* Average brightness filter */
#define AB_ATT  RESPONSE_TIME                      /* Attack */
#define AB_DEC  RESPONSE_TIME                      /* Decay  */

/* Dark sample distribution filter */
#define DS_ATT  RESPONSE_TIME                      /* Attack */
#define DS_DEC  RESPONSE_TIME                      /* Decay  */

/* Peak detection filter */
#define PK_ATT  RESPONSE_TIME                      /* Attack */
#define PK_DEC  RESPONSE_TIME                      /* Decay  */

 /* End of user preferences.     */

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define LIM(v,l,h) ((v) > (l) ? (((v) < (h) ? (v) : (h))) : (l))
#define ROUND_TO_2(a) (((a)&1) ? ((a)+1) : (a))

/* ************************************************************************** */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* ************************ Register access macros ************************** */
/* Macro that get the video display base address from a layer handle          */
#define VideoBaseAddress(Handle) ((U8 *)((HALLAYER_LayerProperties_t *)Handle)->VideoDisplayBaseAddress_p)

#define HAL_Read16(Address_p)        STSYS_ReadRegDev16LE ((void *) (Address_p))
#define HAL_Read8(Address_p)         STSYS_ReadRegDev8 ((void *) (Address_p))

#define HAL_Read32(Address_p)        STSYS_ReadRegDev32LE ((void *) (Address_p))
#define HAL_Write32(Address_p,Value) STSYS_WriteRegDev32LE((void *) (Address_p),(Value))

#define HAL_SetRegister32DefaultValue(Address_p, RegMask, Value)               \
{                                                                              \
    U32 Tmp32;                                                                 \
    Tmp32 = HAL_Read32 (Address_p);                                            \
    HAL_Write32(Address_p, (Tmp32 & (~(RegMask))) | ((Value) & (RegMask)))     \
}

#define HAL_SetRegister32Value(Address_p, RegMask, Mask, Emp, Value)           \
{                                                                              \
    U32 Tmp32;                                                                 \
    Tmp32 = HAL_Read32 (Address_p);                                            \
    HAL_Write32 (Address_p,(Tmp32&(RegMask)&(~((U32)(Mask)<<(Emp))))|((U32)(Value)<<(Emp)));\
}

#define HAL_GetRegister32Value(Address_p, Mask, Emp)                           \
    (((U32)(HAL_Read32(Address_p))&((U32)(Mask)<<(Emp)))>>(Emp))

/* Exported Functions ------------------------------------------------------- */

/* Exported Structures ------------------------------------------------------ */

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
    /*************************                         Video LAYER parameters                         *************************/
    STLAYER_Handle_t            AssociatedLayerHandle;          /* Associated layer handle, to retrive its characteristics.   */
    BOOL                        Opened;                         /* Viewport's status (opened.Close).                          */
    U8                          ViewportId;                     /* Viewport Id.                                               */
    U32                         SourceNumber;                   /* Source number linked to it.                                */

} HALLAYER_ViewportProperties_t;

typedef struct
{
    /*************************                         Video LAYER parameters                         *************************/
    U8                          MaxViewportNumber;                  /* Maximum allowed opened viewports.                      */
    STLAYER_LayerParams_t       LayerParams;                        /* Layer parameters (Height, Width, ScanType, ...).       */
    U32                         VTGFrameRate;                       /* Output frame rate.                                     */
    STEVT_Handle_t              EvtHandle;
    HALLAYER_Position_t         LayerPosition;                      /* Complementary layer parameters (Start and Offset).     */
    void *                      ViewportProperties_p;               /* Viewport properties...                                 */
    struct PSI_Parameters_s
    {   struct DCI_SpecificParameters_s
        {
            int  iAvrgbf;               /* Latest value of filtered average brightness (must be static !)                     */
            int  iBsgf;                 /* Latest value of filtered dark dample distribution value (must be static !)         */
            int  iYpf;                  /* Latest value of filtered sum of peak samples (must be static !)                    */
        }DCI_SpecificParameters;
        STLAYER_PSI_t           CurrentPSIParameters[STLAYER_MAX_VIDEO_FILTER_POSITION];/* PSI parameters for the layer.      */
        STLAYER_PSI_t           JustUpdatedParameters;
        U32                     InterruptMask;                      /* DCI Internal interrupt mask                            */
        U32                     ActivityMask;                       /* Internal mask to summurize PSI activity.               */

    }PSI_Parameters;
    STAVMEM_BlockHandle_t       LmuChromaBlockHandle;               /* Used for LMU buffer avmem allocation/deallocation.     */
    STAVMEM_BlockHandle_t       LmuLumaBlockHandle;                 /* Used for LMU buffer avmem allocation/deallocation.     */
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;               /* Used for LMU buffer avmem allocation/deallocation.     */
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;            /* Avmem mapping.                                         */
    void *                      LmuLumaBuffer_p;                    /* LMU luma buffer address.                               */
    void *                      LmuChromaBuffer_p;                  /* LMU chroma buffer address.                             */

    /*************************                       Video VIEWPORT parameters                        *************************/
    /* NB: These parameters should be viewport properties, but hardware doesn't allow separate viewport management. So the    */
    /*     viewport properties are store at hal_layer level. So only one viewport is then managerd.                           */
    STLAYER_ViewPortParams_t    ViewPortParams;                     /* Input/Output rectangle properties (position & size).   */
    HALLAYER_Dimensions_t       SourcePictureDimensions;            /* Source picture dimensions (Width and Height).          */
    U32                         RealInputWidth;
    U32                         RealOutputWidth;
    STGXOBJ_ColorType_t         ColorType;                          /* Color type of the source picture.                      */
    BOOL                        IsViewportEnable;                   /* Flag to know the enable/disable state of viewport.     */
    U32                         ViewportOffset;                     /* Viewport position (formula: (OutVStart<<16)|OutHStart) */
    U32                         ViewportStop;                       /* Viewport end      (formula: (OutVStop <<16)| OutHStop) */
    HALLAYER_Filter_t           VerticalFormatFilter;
    HALLAYER_Filter_t           HorizontalFormatFilter;
    /* synchronized registers updates */
    semaphore_t *               MultiAccessSem_p;
    BOOL                        IsUpdateNeeded;
    U32                         RegToUpdate;                        /* 32bits = 32 available updates to perform on VSync */
    U32                         CurrentGlobalAlpha;

    STLAYER_OutputMode_t                      OutputMode;
    STLAYER_OutputWindowSpecialModeParams_t   OutputModeParams;
    U16                                       SizeOfZone[HFC_ZONE_NBR];

} HALLAYER_LayerCommonData_t;

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_LAYER_OMEGA2_H */

/* End of layer2.h */
