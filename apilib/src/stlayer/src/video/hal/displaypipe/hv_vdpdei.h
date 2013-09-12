/*******************************************************************************

File name   : hv_vdpdei.h

Description :

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                 ----
02 Jun 2006       Created                                          OL
*******************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __HAL_VDP_DEI_H
#define __HAL_VDP_DEI_H


/* Includes ------------------------------------------------------------ */


/* Private Macros ----------------------------------------------------- */

#define FIELD_TYPE(Field)         (Field.FieldType == STLAYER_TOP_FIELD? "T": "B")

#define COMMON_DATA_PTR(LayerHandle)  ( (HALLAYER_VDPLayerCommonData_t *) ( ( (HALLAYER_LayerProperties_t *) LayerHandle)->PrivateData_p) )




/* Exported Types ------------------------------------------------------ */

/* Bitfield containg the requested DEI Mode (VDP_DEI_SET_MODE_YC_FM,  VDP_DEI_SET_MODE_YC_VI...etc) */
typedef U32 VDP_DEI_Setting_t;


typedef struct
{
    U32     LumaFieldEndianess;
    U32     ChromaFieldEndianess;

    S32     YFOffsetL0, CFOffsetL0;
    U32     YFCeilL0, CFCeilL0;

    S32     YFOffsetL1, CFOffsetL1;
    U32     YFCeilL1, CFCeilL1;

    S32     YFOffsetL2, CFOffsetL2;
    U32     YFCeilL2, CFCeilL2;

    S32     YFOffsetL3, CFOffsetL3;
    U32     YFCeilL3, CFCeilL3;

    S32     YFOffsetL4, CFOffsetL4;
    U32     YFCeilL4, CFCeilL4;

    S32     YFOffsetP0, CFOffsetP0;
    U32     YFCeilP0, CFCeilP0;

    S32     YFOffsetP1, CFOffsetP1;
    U32     YFCeilP1, CFCeilP1;

    S32     YFOffsetP2, CFOffsetP2;
    U32     YFCeilP2, CFCeilP2;

    /* Ceil and Offset for accessing Motion Buffers */
    S32     MFOffsetL0, MFOffsetP0;
    U32     MFCeilL0, MFCeilP0;

} HALLAYER_VDPDeiMemoryAccess_t;

/* Exported Constants -------------------------------------------------- */

/* Default DEI Mode */
#define VDP_DEI_DEFAULT_MODE    STLAYER_DEI_MODE_MLD

/* Exported Variables -------------------------------------------------- */

/* Exported Macros ----------------------------------------------------- */

/* Exported Functions -------------------------------------------------- */


ST_ErrorCode_t vdp_dei_Init (STLAYER_Handle_t   LayerHandle);
ST_ErrorCode_t vdp_dei_Term (STLAYER_Handle_t   LayerHandle);

ST_ErrorCode_t vdp_dei_SetDeinterlacer (STLAYER_Handle_t LayerHandle);
BOOL           vdp_dei_IsDeiActivated (STLAYER_Handle_t LayerHandle);
ST_ErrorCode_t vdp_dei_SetOutputViewPortOrigin (HALLAYER_VDPDeiRegisterMap_t  *DeiRegister_p, STGXOBJ_Rectangle_t *InputRectangle_p);
ST_ErrorCode_t vdp_dei_SetOutputViewPortSize (HALLAYER_VDPDeiRegisterMap_t  *DeiRegister_p, STGXOBJ_Rectangle_t *InputRectangle_p);
ST_ErrorCode_t vdp_dei_SetInputVideoFieldSize (HALLAYER_VDPDeiRegisterMap_t  *DeiRegister_p, U32 SourcePictureHorizontalSize);
ST_ErrorCode_t vdp_dei_SetT3Interconnect (const STLAYER_Handle_t LayerHandle,
                                          HALLAYER_VDPDeiRegisterMap_t  *DeiRegister_p,
                                          HALLAYER_VDPFormatInfo_t  *SrcFormatInfo_p);

ST_ErrorCode_t vdp_dei_ComputeMemoryAccess (U32        SourcePictureHorizontalSize,
                                            U32        SourcePictureVerticalSize,
                                            HALLAYER_VDPFormatInfo_t  *SrcFormatInfo_p,
                                            HALLAYER_VDPDeiMemoryAccess_t   *MemoryAccess_p,
                                            STLAYER_Handle_t LayerHandle
                                            );

ST_ErrorCode_t vdp_dei_SetMemoryAccess (HALLAYER_VDPDeiRegisterMap_t  *DeiRegister_p,
                                        HALLAYER_VDPDeiMemoryAccess_t   *MemoryAccess_p,
                                        STLAYER_Handle_t LayerHandle
                                        );

ST_ErrorCode_t vdp_dei_SetCvfType (STLAYER_Handle_t    LayerHandle);

BOOL           vdp_dei_IsBypassModeNecessary (const STLAYER_Handle_t  LayerHandle);

ST_ErrorCode_t vdp_dei_SetDeiMode (const STLAYER_Handle_t   LayerHandle,
                                   VDP_DEI_Setting_t        DeiMode);

#endif /* #ifndef __HAL_VDP_DEI_H */
/* End of hv_vdpdei.h */
