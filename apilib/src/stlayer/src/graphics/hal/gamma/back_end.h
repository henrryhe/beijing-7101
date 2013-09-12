/*******************************************************************************

File name   : back_end.h

Description :

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BACKEND_H
#define __BACKEND_H

/* Includes ----------------------------------------------------------------- */
#include "pool_mng.h"
#include "layergfx.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define PKZ_OFFSET  0xFC

/*#define ST_7109C3*/



        /* StBus Plug register for STi7109 Cut3.0 */
#define PAS_OFFSET   0xEC
#define MAOS_OFFSET  0xF0
#define MIOS_OFFSET  0xF4
#define MACS_OFFSET  0xF8
#define MAMS_OFFSET  0xFC

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
#include "stmes.h"
#define STLAYER_SECURE_ON    0x40000000
#define STLAYER_SECURE_MASK  0xBFFFFFFF
#define MAX_GDP_NODE_SIZE    144 /* 36 fields of 4 bytes */
#endif



/* Exported Types ----------------------------------------------------------- */

typedef struct
{
    U32 CTL;    /* ConTroL register */
    U32 AGC;    /* Alpha Gain Constant */
    U32 HSRC;   /* Horizontal Sample Rate Converter */
    U32 VPO;    /* View Port Offset */

    U32 VPS;    /* View Port Stop */
    U32 PML;    /* Pixmap Memory Location */
    U32 PMP;    /* Pixmap Memory Pitch */
    U32 SIZE;   /* pixmap memory SIZE */

    U32 VSRC;   /* Vertical Sample Rate Converter */
    U32 NVN;    /* Next Viewport Node */
    U32 KEY1;   /* color KEY 1 */
    U32 KEY2;   /* color KEY 2 */

    U32 HFP;        /* Horizontal Filter Pointer */
    U32 PPT;        /* Viewport properties */
    U32 VFP;         /* Reserved */
    U32 CML;        /* Clut memory location */

    U32 HFC0;       /*  */
    U32 HFC1;       /*  */
    U32 HFC2;       /*  */
    U32 HFC3;       /*  */

    U32 HFC4;       /*  */
    U32 HFC5;       /*  */
    U32 HFC6;       /*  */
    U32 HFC7;       /*  */

    U32 HFC8;       /*  */
    U32 HFC9;       /*  */
    U32 Reserved6;  /*  */
    U32 Reserved7;  /*  */



    U32 VFC0;       /* VF/FF coefficents */
    U32 VFC1;       /* VF/FF coefficents */
    U32 VFC2;       /* VF/FF coefficents */
    U32 VFC3;       /* VF/FF coefficents */

    U32 VFC4;       /* VF/FF coefficents */
    U32 VFC5;       /* VF/FF coefficents */
    U32 Reserved8;  /*  */
    U32 Reserved9;  /*  */

} stlayer_GDPNode_t;

typedef stlayer_GDPNode_t stlayer_BKLNode_t;

typedef struct
{
    U32 CTL;        /* ConTroL register */
    U32 Reserved1 ; /* Reserved */
    U32 Reserved2 ; /* Reserved */
    U32 VPO;        /* View Port Offset */

    U32 Reserved3 ; /* Reserved */
    U32 PML;        /* Pixmap Memory Location */
    U32 PMP;        /* Pixmap Memory Pitch */
    U32 SIZE;       /* pixmap memory SIZE */

    U32 CML;        /* Clur Memory Location */
    U32 Reserved4 ; /* Reserved */
    U32 AWS;        /* Active Window Start */
    U32 AWE;        /* Active Window End */
} stlayer_CURNode_t;

typedef union
{
    stlayer_GDPNode_t       GdpNode;
    stlayer_BKLNode_t       BklNode;
    stlayer_CURNode_t       CurNode;
} stlayer_Node_t;

typedef struct stlayer_ViewPortDescriptor_t
{
    struct stlayer_ViewPortDescriptor_t *  Next_p;  /* use for link       */
    struct stlayer_ViewPortDescriptor_t *  Prev_p;  /* use for link       */
    stlayer_Node_t *                TopNode_p;      /* associated top node */
    stlayer_Node_t *                BotNode_p;      /* associated bottom node */
    STLAYER_Handle_t                LayerHandle;    /* in which layer       */
    BOOL                            Enabled;
    BOOL                            TotalClipped;
    STGXOBJ_Bitmap_t                Bitmap;
    STGXOBJ_Palette_t               Palette;
    STGXOBJ_Rectangle_t             InputRect;
    STGXOBJ_Rectangle_t             OutputRect;
    STGXOBJ_Rectangle_t             ClippedIn;
    STGXOBJ_Rectangle_t             ClippedOut;
    BOOL                            ColorKeyEnabled;
    BOOL                            FFilterEnabled;
    BOOL                            ColorFillEnabled;
    STGXOBJ_ColorKey_t              ColorKeyValue;
    STGXOBJ_ColorARGB_t             ColorFillValue;
    STLAYER_GlobalAlpha_t           Alpha;
    BOOL                            BorderAlphaOn;
    STLAYER_GainParams_t            Gain;
    U32                             AssociatedToMainLayer;
    U32                             FilterPointer;
    BOOL                            HideOnMix2;
    STLAYER_FlickerFilterMode_t     FFilterMode;
} stlayer_ViewPortDescriptor_t;

typedef stlayer_ViewPortDescriptor_t * stlayer_ViewPortHandle_t;


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t stlayer_InsertNodeLayerType(stlayer_ViewPortHandle_t VPHandle);
ST_ErrorCode_t stlayer_ExtractNodeLayerType(stlayer_ViewPortHandle_t VPHandle);
ST_ErrorCode_t stlayer_UpdateNodeGeneric(stlayer_ViewPortHandle_t VPHandle);
ST_ErrorCode_t stlayer_EnableCursor(stlayer_ViewPortHandle_t VPHandle);
ST_ErrorCode_t stlayer_DisableCursor(stlayer_ViewPortHandle_t VPHandle);
ST_ErrorCode_t stlayer_RemapDevice(STLAYER_Handle_t Layer,U32 DeviceId);
void stlayer_DifferedToTopWrite(stlayer_Node_t *,stlayer_Node_t *,
                                STLAYER_Handle_t);
void stlayer_DifferedToBotWrite(stlayer_Node_t *,stlayer_Node_t *,
                                STLAYER_Handle_t);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BACKEND_H */

/* End of .h */
