/*******************************************************************************

File name   : hv_vdpdei.c

Description :

COPYRIGHT (C) STMicroelectronics 2006-2007

Date               Modification                                     Name
----               ------------                                 ----
02 Jun 2006       Created                                          OL
*******************************************************************************/

/* Includes ----------------------------------------------------------- */
#include "stddefs.h"
#include "stdevice.h"

#if !defined ST_OSLINUX
#include <string.h>
#endif

#include "hv_vdplay.h"
#include "hv_vdpdei.h"

#ifdef ST_fmdenabled
    #include "hv_vdpfmd.h"
#endif

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/*#define TRACE_UART */

/* Select the Traces you want */
#define TrMain                 TraceOff
#define TrMode                 TraceOff
#define TrFormatInfo           TraceOff
#define TrCvfType              TraceOff
#define TrViewport             TraceOff
#define TrT3Interc             TraceOff
#define TrMemAccess            TraceOff
#define TrStatus               TraceOff
#define TrMotionBuffer         TraceOff
#define TrMotion               TraceOff
#define Tr3DConf               TraceOff

#define TrWarning              TraceOff
/* TrError is now defined in layer_trace.h */

/* layer_trace.h" should be included AFTER the definition of the traces wanted */
#include "layer_trace.h"

/* Private Types ------------------------------------------------------ */


/* Private Constants -------------------------------------------------- */

#define VDP_END_PROCESSING_INTERRUPT_NUMBER      VDP_END_PROCESSING_INTERRUPT
#define VDP_END_PROCESSING_INTERRUPT_LEVEL          0       /* InterruptLevel is not used */

/* See 7109 video pipe Integration report page 7:
    "A DE-Interlacer which supports up to 720 pixels per lines (only for SD or decimated input formats)" */
#define MOTION_BUFFER_WIDTH       720
#define MOTION_BUFFER_HEIGHT      576

#define MAX_SOURCE_PICTURE_WIDTH   MOTION_BUFFER_WIDTH

#define DEFAULT_ST_BUS_PRIORITY                 0
#define DEFAULT_RASTER_LUMA_CHROMA_ENDIANESS    0
#define DEFAULT_WAIT_AFTER_VSYNC_COUNT          0
#define DEFAULT_ST_BUS_OPCODESIZE               0x03     /* ST8, LD8 primitive operations are used */
#define DEFAULT_ST_BUS_CHUNK_SIZE               0x0F
#define DEFAULT_MIN_SPEC_BETWEEN_REQS           0

/* Ratio (output height/input height) under which the DEI is bypassed */
/* This ratio is expressed in percent so: 60 corresponds to 0.60 */
#define HALLAYER_VERTICAL_ZOOM_DEI_ACTIVATION_THRESHOLD   60

#define VDP_FIFO_EMPTY_INTERRUPT_NUMBER         VDP_FIFO_EMPTY_INTERRUPT
#define VDP_FIFO_EMPTY_INTERRUPT_LEVEL          0       /* InterruptLevel is not used */

/* Private Function prototypes ---------------------------------------- */

static ST_ErrorCode_t   vdp_dei_DisplayFormatInfo (const STLAYER_Handle_t    LayerHandle);
static ST_ErrorCode_t   vdp_dei_Set3DConfiguration (const STLAYER_Handle_t LayerHandle);
static ST_ErrorCode_t   vdp_dei_SetMotionMode (const STLAYER_Handle_t LayerHandle,
                                               VDP_DEI_Setting_t * DeiMode_p);
static void             vdp_dei_CheckFieldsParity(const STLAYER_Handle_t LayerHandle);
static void             vdp_dei_DisplayFieldAvailable(const STLAYER_Handle_t LayerHandle);
static void             vdp_dei_SelectDeiMode(const STLAYER_Handle_t LayerHandle);
static void             vdp_dei_ApplyMode(const STLAYER_Handle_t LayerHandle,
                                            STLAYER_DeiMode_t Mode);
static void             vdp_dei_ResetField(const STLAYER_Handle_t LayerHandle, STLAYER_Field_t * Field_p);
static void             vdp_dei_CheckFieldConsecutiveness(const STLAYER_Handle_t LayerHandle);
static void             vdp_dei_ResetFieldArray(const STLAYER_Handle_t LayerHandle,
                                                HALLAYER_VDPFieldArray_t * FieldArray_p);
static void             vdp_dei_SaveFieldsUsedByHardware(const STLAYER_Handle_t LayerHandle);
static void             vdp_dei_SaveFieldsUsedAtNextVSync(const STLAYER_Handle_t LayerHandle);
static BOOL             vdp_dei_IsTemporalInterpolationAllowed(const STLAYER_Handle_t LayerHandle);

#if !defined (STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI)
static ST_ErrorCode_t   vdp_dei_InitMotionBuffers (const STLAYER_Handle_t LayerHandle);
static ST_ErrorCode_t   vdp_dei_FreeMotionBuffers (const STLAYER_Handle_t LayerHandle);
static ST_ErrorCode_t   vdp_dei_AllocateMotionBuffer (const STLAYER_Handle_t LayerHandle,
                                                      HALLAYER_VDPMotionBuffer_t * MotionBuffer_p);
static ST_ErrorCode_t   vdp_dei_RotateMotionBuffers (const STLAYER_Handle_t LayerHandle);
static void             vdp_dei_SetMotionBufferAddress (const STLAYER_Handle_t LayerHandle,
                                                        void * BufferAddress_p,
                                                        U32 MotionBufferRegisterOffset);
static BOOL             vdp_dei_RectanglesAreIdentical(STGXOBJ_Rectangle_t * Rectangle1_p, STGXOBJ_Rectangle_t * Rectangle2_p);
#endif /* STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI */

#ifdef DBG_CHECK_VDP_CONSISTENCY
    static ST_ErrorCode_t vdp_dei_StartVdpFifoEmptyInterrupt(const STLAYER_Handle_t LayerHandle);
    static ST_ErrorCode_t vdp_dei_StopVdpFifoEmptyInterrupt(const STLAYER_Handle_t LayerHandle);

    static                  STOS_INTERRUPT_DECLARE(VdpFifoEmptyInterruptHandler, Param);
#endif

/* Functions ---------------------------------------------------------- */

/******************************************************************************/
/*************************      API FUNCTIONS    **********************************/
/******************************************************************************/

/*******************************************************************************
Name        : vdp_dei_Init
Description : Initialisation of DEI module
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t vdp_dei_Init (STLAYER_Handle_t   LayerHandle)
{
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);
#if defined (ST_7200)
    HALLAYER_LayerProperties_t *  LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
#endif /* 7200 */

    vdp_dei_ResetFieldArray(LayerHandle, &LayerCommonData_p->FieldsCurrentlyUsedByHw);
    vdp_dei_ResetFieldArray(LayerHandle, &LayerCommonData_p->FieldsToUseAtNextVSync);

    LayerCommonData_p->CurrentMotionMode = HALLAYER_VDP_MOTION_DETECTION_OFF;

    LayerCommonData_p->LastInputRectangle.PositionX  = 0;
    LayerCommonData_p->LastInputRectangle.PositionY  = 0;
    LayerCommonData_p->LastInputRectangle.Width      = 0;
    LayerCommonData_p->LastInputRectangle.Height     = 0;

    LayerCommonData_p->LastOutputRectangle.PositionX  = 0;
    LayerCommonData_p->LastOutputRectangle.PositionY  = 0;
    LayerCommonData_p->LastOutputRectangle.Width      = 0;
    LayerCommonData_p->LastOutputRectangle.Height     = 0;

#ifdef STLAYER_NO_DEINTERLACER
    /* There is a request to Bypass the Deinterlacer all the time */
    LayerCommonData_p->RequestedDeiMode = STLAYER_DEI_MODE_BYPASS;
#else
    /* Normal mode: The default requested mode is the MLD Mode (= 3D Mode) */
    LayerCommonData_p->RequestedDeiMode = VDP_DEI_DEFAULT_MODE;
#endif

    /* DEI is off during initialization. It will be set to ON when displaying the first picture */
    vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_OFF);

#ifdef DBG_CHECK_VDP_CONSISTENCY
    /* Use Pixel FIFO empty interrupt to detect bad settings of the VDP */
    vdp_dei_StartVdpFifoEmptyInterrupt(LayerHandle);
#endif

#if defined (ST_7200)
    if (LayerProperties_p->LayerType == MAIN_DISPLAY)
#endif /* 7200 */
    {
        /* Set default 3D configuration */
        vdp_dei_Set3DConfiguration(LayerHandle);

#if !defined (STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI)
        vdp_dei_InitMotionBuffers(LayerHandle);
#endif /* STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI */
    }

    return (ErrorCode);
} /* End of vdp_dei_Init() function. */

/*******************************************************************************
Name        : vdp_dei_Term
Description : Termination of DEI module: Release memory.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vdp_dei_Term (const STLAYER_Handle_t LayerHandle)
{
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
#if defined (ST_7200)
    HALLAYER_LayerProperties_t *  LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
#endif /* 7200 */

    TrStatus(("\r\nvdp_dei_Term"));

    vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_OFF);

#if defined (ST_7200)
    if (LayerProperties_p->LayerType == MAIN_DISPLAY)
#endif /* 7200 */
    {
    #if !defined (STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI)
            /* Free the Motion Buffers */
            vdp_dei_FreeMotionBuffers(LayerHandle);
    #else
            UNUSED_PARAMETER(LayerHandle);
    #endif /* STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI */
    }

#ifdef DBG_CHECK_VDP_CONSISTENCY
    vdp_dei_StopVdpFifoEmptyInterrupt(LayerHandle);
#endif

    return (ErrorCode);
}

/*******************************************************************************
Name        : vdp_dei_SetDeinterlacer (API Function)
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vdp_dei_SetDeinterlacer (STLAYER_Handle_t LayerHandle)
{
    /* Skip a line to separate each VSync */
    TrMain(("\r\n"));

    /* A new VSync has occured:
        The "FieldsToUseAtNextVSync" now become the Fields used by the Hardware. */
    vdp_dei_SaveFieldsUsedByHardware(LayerHandle);

    /* Save the references to the Fields that will be used at the next VSync */
    vdp_dei_SaveFieldsUsedAtNextVSync(LayerHandle);

    vdp_dei_CheckFieldConsecutiveness(LayerHandle);

    /* The 3 function bellow display Debug information. They have no impact on the Deinterlacing */
    vdp_dei_CheckFieldsParity(LayerHandle);
    vdp_dei_DisplayFormatInfo(LayerHandle);
    vdp_dei_DisplayFieldAvailable(LayerHandle);

    /* Select the appropriate DEI mode */
    vdp_dei_SelectDeiMode(LayerHandle);

    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : vdp_dei_IsDeiActivated
Description : This function indicates if the DEI is activated.
Parameters  :
Assumptions :
Limitations :
Returns     : A Boolean indicating if the DEI is activated.
*******************************************************************************/
BOOL  vdp_dei_IsDeiActivated (STLAYER_Handle_t LayerHandle)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    U32                             DeiCtlRegisterValue;

    LayerProperties_p   = (HALLAYER_LayerProperties_t *) LayerHandle;

    if (LayerProperties_p->LayerType != MAIN_DISPLAY)
    {
        return FALSE;
    }

    /* Read DEI_CTL register in order to get the current DEI mode */
    DeiCtlRegisterValue = HAL_ReadDisp32(VDP_DEI_CTL);

    if  ( (DeiCtlRegisterValue & (VDP_DEI_CTL_INACTIVE << VDP_DEI_CTL_INACTIVE_EMP) ) ||
          (DeiCtlRegisterValue & (VDP_DEI_CTL_BYPASS_ON << VDP_DEI_CTL_BYPASS_EMP) ) )
    {
        /* DEI is inactive or bypassed */
        return (FALSE);
    }
    else
    {
        /* DEI is active */
        return (TRUE);
    }
}/* End of vdp_dei_IsDeiActivated() function. */

/*******************************************************************************
Name        : vdp_dei_SetOutputViewPortOrigin
Description : This function sets the Output Viewport origin
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, if no error occurs
*******************************************************************************/
ST_ErrorCode_t vdp_dei_SetOutputViewPortOrigin (HALLAYER_VDPDeiRegisterMap_t  *DeiRegister_p, STGXOBJ_Rectangle_t *InputRectangle_p)
{
    U32     Orig_X, Orig_Y;

    if ( (DeiRegister_p == NULL) || (InputRectangle_p == NULL) )
    {
        TrError(("\r\nError! Invalid arg in vdp_dei_SetOutputViewPortOrigin"));
        return ST_ERROR_BAD_PARAMETER;
    }

    Orig_Y = (InputRectangle_p->PositionY / 16) & VDP_DEI_VIEWPORT_ORIG_Y_MASK;
    /* The X position must be an even value */
    Orig_X = ((InputRectangle_p->PositionX / 16) & ~1) & VDP_DEI_VIEWPORT_ORIG_X_MASK;

    DeiRegister_p->RegDISP_DEI_VIEWPORT_ORIG =   (Orig_Y << VDP_DEI_VIEWPORT_ORIG_Y_EMP)
                                               | (Orig_X << VDP_DEI_VIEWPORT_ORIG_X_EMP);

    TrViewport(("\r\n vdp_dei_SetOutputViewPortOrigine Orig_X=%d, Orig_Y=%d", Orig_X, Orig_Y));

    return ST_NO_ERROR;
}/* End of vdp_dei_SetOutputViewPortOrigin() function. */

/*******************************************************************************
Name        : vdp_dei_SetOutputViewPortSize
Description : This function sets the Output Viewport size
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, if no error occurs
*******************************************************************************/
ST_ErrorCode_t vdp_dei_SetOutputViewPortSize (HALLAYER_VDPDeiRegisterMap_t  *DeiRegister_p, STGXOBJ_Rectangle_t *InputRectangle_p)
{
    U32 ViewportHeight, ViewportWidth;

    if ( (DeiRegister_p == NULL) || (InputRectangle_p == NULL) )
    {
        TrError(("\r\nError! Invalid arg in vdp_dei_SetOutputViewPortSize"));
        return ST_ERROR_BAD_PARAMETER;
    }

    ViewportHeight = InputRectangle_p->Height & VDP_DEI_VIEWPORT_SIZE_H_MASK;
    /* Width must be an even value */
    ViewportWidth  = (InputRectangle_p->Width & ~1) & VDP_DEI_VIEWPORT_SIZE_W_MASK;

    DeiRegister_p->RegDISP_DEI_VIEWPORT_SIZE =  (ViewportHeight << VDP_DEI_VIEWPORT_SIZE_HEIGHT_EMP)
                                               |(ViewportWidth << VDP_DEI_VIEWPORT_SIZE_WIDTH_EMP);

    TrViewport(("\r\n vdp_dei_SetOutputViewPortSize ViewportHeight=%d, ViewportWidth=%d", ViewportHeight, ViewportWidth));

    return ST_NO_ERROR;
}/* End of vdp_dei_SetOutputViewPortSize() function. */

/*******************************************************************************
Name        : vdp_dei_SetInputVideoFieldSize
Description : This function sets the input video field size
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, if no error occurs
*******************************************************************************/
ST_ErrorCode_t vdp_dei_SetInputVideoFieldSize (HALLAYER_VDPDeiRegisterMap_t  *DeiRegister_p, U32 SourcePictureHorizontalSize)
{
    if (DeiRegister_p == NULL)
    {
        TrError(("\r\nError! Invalid arg in vdp_dei_SetInputVideoFieldSize"));
        return ST_ERROR_BAD_PARAMETER;
    }

    TrViewport(("\r\n vdp_dei_SetInputVideoFieldSize: %d", SourcePictureHorizontalSize));

    DeiRegister_p->RegDISP_DEI_VF_SIZE = SourcePictureHorizontalSize;

    return ST_NO_ERROR;
} /* End of vdp_dei_SetInputVideoFieldSize() function. */


/*******************************************************************************
Name        : vdp_dei_SetT3Interconnect
Description : This function programs the access to the T3 Bus
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, if no error occurs
*******************************************************************************/
ST_ErrorCode_t vdp_dei_SetT3Interconnect (const STLAYER_Handle_t LayerHandle,
                                          HALLAYER_VDPDeiRegisterMap_t *DeiRegister_p,
                                          HALLAYER_VDPFormatInfo_t *SrcFormatInfo_p)
{
    HALLAYER_VDPLayerCommonData_t *  LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);
    U8 STBusChunkSize = DEFAULT_ST_BUS_CHUNK_SIZE;
    U8 OpCodeSize = DEFAULT_ST_BUS_OPCODESIZE;

    if ((DeiRegister_p == NULL) || (SrcFormatInfo_p == NULL) || (LayerCommonData_p == NULL))
    {
        TrError(("\r\nError! Invalid arg in vdp_dei_SetT3Interconnect"));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Check the DEI mode to program the STBus chunk size */
    switch(LayerCommonData_p->AppliedDeiMode)
    {
        case STLAYER_DEI_MODE_MLD:
        case STLAYER_DEI_MODE_LMU:
            /* 3-D mode */
            if (SrcFormatInfo_p->IsFormatRasterNotMacroblock)
            {
                OpCodeSize     = 0x03;
                STBusChunkSize = 0x00;
            }
            else
            {
                OpCodeSize     = 0x03;
                /* The architects advise using 8 LD8 transfers but it generates flashes on screen on big zoom-outs */
                /*STBusChunkSize = 0x07;*/
                STBusChunkSize = 0x03;  /* 4 transfers */
            }
            break;

        case STLAYER_DEI_MODE_OFF:
        case STLAYER_DEI_MODE_BYPASS:
        case STLAYER_DEI_MODE_VERTICAL:
        case STLAYER_DEI_MODE_DIRECTIONAL:
        case STLAYER_DEI_MODE_FIELD_MERGING:
        case STLAYER_DEI_MODE_MEDIAN_FILTER:
            /* Bypass or spatial mode */
            if (SrcFormatInfo_p->IsFormatRasterNotMacroblock)
            {
                OpCodeSize     = 0x03;
                STBusChunkSize = 0x00;
            }
            else
            {
                OpCodeSize     = 0x03;
                /* The architects advise using 8 LD8 transfers but it generates flashes on screen on big zoom-outs */
                STBusChunkSize = 0x0F;
            }
            break;

        default:
            TrWarning(("\r\nWarning! Using default values for opcode and chunk sizes because DEI mode not recognized!"));
    }

    TrT3Interc(("\r\nSetT3Interconnect: OpCodeSize = 0x%x, STBusChunkSize = 0x%x", OpCodeSize, STBusChunkSize));

    DeiRegister_p->RegDISP_DEI_T3I_CTL = (U32) (DEFAULT_ST_BUS_PRIORITY & VDP_DEI_T3I_PRIO_MASK) << VDP_DEI_T3I_PRIO_EMP
                                        |(U32) (DEFAULT_MIN_SPEC_BETWEEN_REQS &VDP_DEI_T3I_MSBR_MASK) << VDP_DEI_T3I_MSBR_EMP
                                        |(U32) (SrcFormatInfo_p->IsFormatRasterNotMacroblock) << VDP_DEI_T3I_YC_BUFFERS_EMP /* Luma+Chroma in same buffers */
                                        |(U32) (SrcFormatInfo_p->IsFormatRasterNotMacroblock & DEFAULT_RASTER_LUMA_CHROMA_ENDIANESS) << VDP_DEI_T3I_YC_ENDIANESS_EMP
                                        |(U32) (!SrcFormatInfo_p->IsFormatRasterNotMacroblock) << VDP_DEI_T3I_MB_ENABLE_EMP
                                        |(U32) (DEFAULT_WAIT_AFTER_VSYNC_COUNT & VDP_DEI_T3I_WAVSYNC_MASK) << VDP_DEI_T3I_WAVSYNC_EMP
                                        |(U32) (OpCodeSize & VDP_DEI_T3I_OPSIZE_MASK) << VDP_DEI_T3I_OPSIZE_EMP
                                        |(U32) (STBusChunkSize & VDP_DEI_T3I_CHKSIZE_MASK) << VDP_DEI_T3I_CHKSIZE_EMP;

    return ST_NO_ERROR;
}/* End of vdp_dei_SetT3Interconnect() function. */


/*******************************************************************************
Name        : vdp_dei_ComputeMemoryAccess
Description : This function computes how to access Macroblocks in memory
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, if no error occurs
*******************************************************************************/
ST_ErrorCode_t vdp_dei_ComputeMemoryAccess (U32 SourcePictureHorizontalSize,
                                            U32 SourcePictureVerticalSize,
                                            HALLAYER_VDPFormatInfo_t  *SrcFormatInfo_p,
                                            HALLAYER_VDPDeiMemoryAccess_t   *MemoryAccess_p,
                                            STLAYER_Handle_t    LayerHandle)
{
    U32 VideoHeightInMB;
    U32 VideoWidthInMB;
    U32 ZoomFactor;
#if defined (ST_7200)
    HALLAYER_LayerProperties_t * LayerProperties_p= (HALLAYER_LayerProperties_t *) LayerHandle;
#else
    UNUSED_PARAMETER(LayerHandle);
#endif /* 7200 */

    if ((SrcFormatInfo_p == NULL) || (MemoryAccess_p == NULL))
    {
        TrError(("\r\nError! Invalid arg in vdp_dei_ComputeMemoryAccess"));
        return ST_ERROR_BAD_PARAMETER;
    }

    TrMemAccess(("\r\n vdp_dei_ComputeMemoryAccess"));

    /* Display Pipe Memory Access */

    MemoryAccess_p->YFOffsetL0 = 0; MemoryAccess_p->YFCeilL0 = 0;  MemoryAccess_p->CFOffsetL0 = 0;  MemoryAccess_p->CFCeilL0 = 0;
    MemoryAccess_p->YFOffsetL1 = 0; MemoryAccess_p->YFCeilL1 = 0;  MemoryAccess_p->CFOffsetL1 = 0;  MemoryAccess_p->CFCeilL1 = 0;
    MemoryAccess_p->YFOffsetL2 = 0; MemoryAccess_p->YFCeilL2 = 0;  MemoryAccess_p->CFOffsetL2 = 0;  MemoryAccess_p->CFCeilL2 = 0;
    MemoryAccess_p->YFOffsetL3 = 0; MemoryAccess_p->YFCeilL3 = 0;  MemoryAccess_p->CFOffsetL3 = 0;  MemoryAccess_p->CFCeilL3 = 0;
    MemoryAccess_p->YFOffsetL4 = 0; MemoryAccess_p->YFCeilL4 = 0;  MemoryAccess_p->CFOffsetL4 = 0;  MemoryAccess_p->CFCeilL4 = 0;
    MemoryAccess_p->YFOffsetP0 = 0; MemoryAccess_p->YFCeilP0 = 0;  MemoryAccess_p->CFOffsetP0 = 0;  MemoryAccess_p->CFCeilP0 = 0;
    MemoryAccess_p->YFOffsetP1 = 0; MemoryAccess_p->YFCeilP1 = 0;  MemoryAccess_p->CFOffsetP1 = 0;  MemoryAccess_p->CFCeilP1 = 0;
    MemoryAccess_p->YFOffsetP2 = 0; MemoryAccess_p->YFCeilP2 = 0;  MemoryAccess_p->CFOffsetP2 = 0;  MemoryAccess_p->CFCeilP2 = 0;

    /* 4:2:2 Raster */
    if (SrcFormatInfo_p->IsFormatRasterNotMacroblock && SrcFormatInfo_p->IsFormat422Not420 )
    {
        MemoryAccess_p->LumaFieldEndianess = 0x7;
        MemoryAccess_p->ChromaFieldEndianess = 0x7;

        MemoryAccess_p->YFOffsetP0 = 1;
        MemoryAccess_p->CFOffsetP0 = 1;

        MemoryAccess_p->YFCeilP0 = SourcePictureHorizontalSize / 8;
        MemoryAccess_p->CFCeilP0 = SourcePictureVerticalSize;       /* DG: Shouldn't CFCeilP0 = YFCeilP0? */

        if(!SrcFormatInfo_p->IsInputScantypeProgressive) /* Interlaced Input */
        {
            /*y[picnum][posy][posx]*/
            MemoryAccess_p->YFOffsetL0 = (SourcePictureHorizontalSize * 2) / 8;    /* byte address offset from y[i][j][nb_column] to y[i][j+2][1] */
            MemoryAccess_p->YFCeilL0    = SourcePictureVerticalSize / 2 ;          /* nb_line / 2 */
            /* Chroma stack not programmed */
        }
        else /*progressive input */
        {
            MemoryAccess_p->YFOffsetL0 = SourcePictureHorizontalSize / 8;           /* byte address offset from y[i][j][nb_column] to y[i][j+1][1] */
            MemoryAccess_p->YFCeilL0    = SourcePictureVerticalSize;                /* nb_line */
            /* Chroma stack not programmed */
        }
    }
    /* 4:2:0 MacroBlock */
    else if (!SrcFormatInfo_p->IsFormatRasterNotMacroblock && !SrcFormatInfo_p->IsFormat422Not420 )
    {
        MemoryAccess_p->LumaFieldEndianess = 0x0;
        MemoryAccess_p->ChromaFieldEndianess = 0x20;

        VideoWidthInMB       = ((SourcePictureHorizontalSize + 15) / 16);           /* 'n' in App Note   */
        VideoHeightInMB      = ((SourcePictureVerticalSize + 31) / 32);             /* 'p' in app note   */

        /* Pixel Address generation in a Line is constant */
        MemoryAccess_p->YFOffsetP0 = 56;      MemoryAccess_p->YFCeilP0 = VideoWidthInMB;
        MemoryAccess_p->YFOffsetP1 = 8;       MemoryAccess_p->YFCeilP1 = 2;
        MemoryAccess_p->YFOffsetP2 = 0;       MemoryAccess_p->YFCeilP2 = 0;

        MemoryAccess_p->CFOffsetP0 = 44;      MemoryAccess_p->CFCeilP0 = (VideoWidthInMB + 1) / 2;
        MemoryAccess_p->CFOffsetP1 = 12;      MemoryAccess_p->CFCeilP1 = 2;
        MemoryAccess_p->CFOffsetP2 = 4;       MemoryAccess_p->CFCeilP2 = 2;

        /* DG: To do, check if horizontal or vertical zoom only */
        /*ZoomFactor = SourcePictureVerticalSize / OutputRectangle.Height;*/
        ZoomFactor = 1;

        /* Line Address generation */
        /* This is dependent on endianness conversion done by STBUS */
        /* See DEI App Note, Case covered here is Little Endian 128bits->64bits conversion */
        /* The base address to program in the registers is dependent on scan */
        if (SrcFormatInfo_p->IsInputScantypeProgressive)
        {
            if (ZoomFactor < 4)
            {
                /* Progressive picture settings */
                MemoryAccess_p->YFOffsetL0 = 64 * VideoWidthInMB - 53;     MemoryAccess_p->YFCeilL0 = VideoHeightInMB;
                MemoryAccess_p->YFOffsetL1 = 11;                           MemoryAccess_p->YFCeilL1 = 2;
                MemoryAccess_p->YFOffsetL2 =-13;                           MemoryAccess_p->YFCeilL2 = 4;
                MemoryAccess_p->YFOffsetL3 =-17;                           MemoryAccess_p->YFCeilL3 = 2;
                MemoryAccess_p->YFOffsetL4 = 16;                           MemoryAccess_p->YFCeilL4 = 2;

                MemoryAccess_p->CFOffsetL0 = 32 * VideoWidthInMB - 41;     MemoryAccess_p->CFCeilL0 = VideoHeightInMB;
                MemoryAccess_p->CFOffsetL1 = 23;                           MemoryAccess_p->CFCeilL1 = 2;
                MemoryAccess_p->CFOffsetL2 = -5;                           MemoryAccess_p->CFCeilL2 = 2;
                MemoryAccess_p->CFOffsetL3 = -9;                           MemoryAccess_p->CFCeilL3 = 2;
                MemoryAccess_p->CFOffsetL4 =  8;                           MemoryAccess_p->CFCeilL4 = 2;
            }
            else if (ZoomFactor <= 8)
            {
                /* Progressive picture settings, zoom out between 1/4 and 1/8 */
                MemoryAccess_p->YFOffsetL0 = 64 * VideoWidthInMB - 38;     MemoryAccess_p->YFCeilL0 = VideoHeightInMB;
                MemoryAccess_p->YFOffsetL1 = 26;                           MemoryAccess_p->YFCeilL1 = 2;
                MemoryAccess_p->YFOffsetL2 =  2;                           MemoryAccess_p->YFCeilL2 = 4;
                MemoryAccess_p->YFOffsetL3 =  0;                           MemoryAccess_p->YFCeilL3 = 0;
                MemoryAccess_p->YFOffsetL4 =  0;                           MemoryAccess_p->YFCeilL4 = 0;

                MemoryAccess_p->CFOffsetL0 = 32 * VideoWidthInMB - 34;     MemoryAccess_p->CFCeilL0 = VideoHeightInMB;
                MemoryAccess_p->CFOffsetL1 = 30;                           MemoryAccess_p->CFCeilL1 = 2;
                MemoryAccess_p->CFOffsetL2 =  2;                           MemoryAccess_p->CFCeilL2 = 2;
                MemoryAccess_p->CFOffsetL3 =  0;                           MemoryAccess_p->CFCeilL3 = 0;
                MemoryAccess_p->CFOffsetL4 =  0;                           MemoryAccess_p->CFCeilL4 = 0;
            }
        }
        else
        {
            if (ZoomFactor < 2)
            {
                /*Interlaced picture settings */
                MemoryAccess_p->YFOffsetL0 = 64 * VideoWidthInMB - 37;     MemoryAccess_p->YFCeilL0 = VideoHeightInMB;
                MemoryAccess_p->YFOffsetL1 = 27;                           MemoryAccess_p->YFCeilL1 = 2;
                MemoryAccess_p->YFOffsetL2 =  3;                           MemoryAccess_p->YFCeilL2 = 4;
                MemoryAccess_p->YFOffsetL3 = -1;                           MemoryAccess_p->YFCeilL3 = 2;
                MemoryAccess_p->YFOffsetL4 =  0;                           MemoryAccess_p->YFCeilL4 = 0;

                MemoryAccess_p->CFOffsetL0 = 32 * VideoWidthInMB - 33;     MemoryAccess_p->CFCeilL0 = VideoHeightInMB;
                MemoryAccess_p->CFOffsetL1 = 31;                           MemoryAccess_p->CFCeilL1 = 2;
                MemoryAccess_p->CFOffsetL2 =  3;                           MemoryAccess_p->CFCeilL2 = 2;
                MemoryAccess_p->CFOffsetL3 = -1;                           MemoryAccess_p->CFCeilL3 = 2;
                MemoryAccess_p->CFOffsetL4 =  0;                           MemoryAccess_p->CFCeilL4 = 0;
            }
            else if (ZoomFactor <= 4)
            {
                /*Interlaced picture settings, zoom out between 1/2 and 1/4 */
                MemoryAccess_p->YFOffsetL0 = 64 * VideoWidthInMB - 38;     MemoryAccess_p->YFCeilL0 = VideoHeightInMB;
                MemoryAccess_p->YFOffsetL1 = 26;                           MemoryAccess_p->YFCeilL1 = 2;
                MemoryAccess_p->YFOffsetL2 =  2;                           MemoryAccess_p->YFCeilL2 = 4;
                MemoryAccess_p->YFOffsetL3 =  0;                           MemoryAccess_p->YFCeilL3 = 0;
                MemoryAccess_p->YFOffsetL4 =  0;                           MemoryAccess_p->YFCeilL4 = 0;

                MemoryAccess_p->CFOffsetL0 = 32 * VideoWidthInMB - 34;     MemoryAccess_p->CFCeilL0 = VideoHeightInMB;
                MemoryAccess_p->CFOffsetL1 = 30;                           MemoryAccess_p->CFCeilL1 = 2;
                MemoryAccess_p->CFOffsetL2 =  2;                           MemoryAccess_p->CFCeilL2 = 2;
                MemoryAccess_p->CFOffsetL3 =  0;                           MemoryAccess_p->CFCeilL3 = 0;
                MemoryAccess_p->CFOffsetL4 =  0;                           MemoryAccess_p->CFCeilL4 = 0;
            }
        }
    }

#if defined (ST_7200)
    if(LayerProperties_p->LayerType == MAIN_DISPLAY)
#endif /* 7200 */
    {
        /* Set Memory Access to Motion Buffers */

        /* Line */
        MemoryAccess_p->MFCeilL0 = MOTION_BUFFER_HEIGHT / 2;
        MemoryAccess_p->MFOffsetL0 = MOTION_BUFFER_WIDTH / 8;

        /* Pixel */
        MemoryAccess_p->MFCeilP0 = MOTION_BUFFER_WIDTH / 8;
        MemoryAccess_p->MFOffsetP0 = 1;
    }

    return ST_NO_ERROR;
} /* End of vdp_dei_ComputeMemoryAccess() function. */

/*******************************************************************************
Name        : vdp_dei_SetMemoryAccess
Description : This function set the way to access Macroblocks in memory
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, if no error occurs
*******************************************************************************/
ST_ErrorCode_t vdp_dei_SetMemoryAccess (HALLAYER_VDPDeiRegisterMap_t  *DeiRegister_p,
                                        HALLAYER_VDPDeiMemoryAccess_t   *MemoryAccess_p,
                                        STLAYER_Handle_t LayerHandle)
{

#if defined (ST_7200)
    HALLAYER_LayerProperties_t * LayerProperties_p   = (HALLAYER_LayerProperties_t *) LayerHandle;
#else
    UNUSED_PARAMETER(LayerHandle);
#endif /* 7200 */

    if ((DeiRegister_p == NULL) || (MemoryAccess_p == NULL))
    {
        TrError(("\r\nError! Invalid arg in vdp_dei_SetMemoryAccess"));
        return ST_ERROR_BAD_PARAMETER;
    }

    TrMemAccess(("\r\n vdp_dei_SetMemoryAccess"));

    DeiRegister_p->RegDISP_DEI_YF_FORMAT   =  MemoryAccess_p->LumaFieldEndianess;
    DeiRegister_p->RegDISP_DEI_YF_STACK[0] = (MemoryAccess_p->YFOffsetL0 & VDP_DEI_YF_STACK_OFFSET_MASK) << VDP_DEI_YF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->YFCeilL0 & VDP_DEI_YF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_YF_STACK[1] = (MemoryAccess_p->YFOffsetL1 & VDP_DEI_YF_STACK_OFFSET_MASK) << VDP_DEI_YF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->YFCeilL1 & VDP_DEI_YF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_YF_STACK[2] = (MemoryAccess_p->YFOffsetL2 & VDP_DEI_YF_STACK_OFFSET_MASK) << VDP_DEI_YF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->YFCeilL2 & VDP_DEI_YF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_YF_STACK[3] = (MemoryAccess_p->YFOffsetL3 & VDP_DEI_YF_STACK_OFFSET_MASK) << VDP_DEI_YF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->YFCeilL3 & VDP_DEI_YF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_YF_STACK[4] = (MemoryAccess_p->YFOffsetL4 & VDP_DEI_YF_STACK_OFFSET_MASK) << VDP_DEI_YF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->YFCeilL4 & VDP_DEI_YF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_YF_STACK[5] = (MemoryAccess_p->YFOffsetP0 & VDP_DEI_YF_STACK_OFFSET_MASK) << VDP_DEI_YF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->YFCeilP0 & VDP_DEI_YF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_YF_STACK[6] = (MemoryAccess_p->YFOffsetP1 & VDP_DEI_YF_STACK_OFFSET_MASK) << VDP_DEI_YF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->YFCeilP1 & VDP_DEI_YF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_YF_STACK[7] = (MemoryAccess_p->YFOffsetP2 & VDP_DEI_YF_STACK_OFFSET_MASK) << VDP_DEI_YF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->YFCeilP2 & VDP_DEI_YF_STACK_CEIL_MASK);

    DeiRegister_p->RegDISP_DEI_CF_FORMAT   =  MemoryAccess_p->ChromaFieldEndianess;
    DeiRegister_p->RegDISP_DEI_CF_STACK[0] = (MemoryAccess_p->CFOffsetL0 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->CFCeilL0 & VDP_DEI_CF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_CF_STACK[1] = (MemoryAccess_p->CFOffsetL1 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->CFCeilL1 & VDP_DEI_CF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_CF_STACK[2] = (MemoryAccess_p->CFOffsetL2 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->CFCeilL2 & VDP_DEI_CF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_CF_STACK[3] = (MemoryAccess_p->CFOffsetL3 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->CFCeilL3 & VDP_DEI_CF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_CF_STACK[4] = (MemoryAccess_p->CFOffsetL4 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->CFCeilL4 & VDP_DEI_CF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_CF_STACK[5] = (MemoryAccess_p->CFOffsetP0 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->CFCeilP0 & VDP_DEI_CF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_CF_STACK[6] = (MemoryAccess_p->CFOffsetP1 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->CFCeilP1 & VDP_DEI_CF_STACK_CEIL_MASK);
    DeiRegister_p->RegDISP_DEI_CF_STACK[7] = (MemoryAccess_p->CFOffsetP2 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                             (MemoryAccess_p->CFCeilP2 & VDP_DEI_CF_STACK_CEIL_MASK);
#if defined (ST_7200)
    if(LayerProperties_p->LayerType == MAIN_DISPLAY)
#endif /* 7200 */
    {
        DeiRegister_p->RegDISP_DEI_MF_STACK_L0 = (MemoryAccess_p->MFOffsetL0 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                                 (MemoryAccess_p->MFCeilL0 & VDP_DEI_CF_STACK_CEIL_MASK);

        DeiRegister_p->RegDISP_DEI_MF_STACK_P0 = (MemoryAccess_p->MFOffsetP0 & VDP_DEI_CF_STACK_OFFSET_MASK) << VDP_DEI_CF_STACK_OFFSET_EMP |
                                                 (MemoryAccess_p->MFCeilP0 & VDP_DEI_CF_STACK_CEIL_MASK);
    }

    return ST_NO_ERROR;
} /* End of vdp_dei_SetMemoryAccess() function. */

/******************************************************************************
Name        : vdp_dei_SetCvfType
Description : This function set in the DEI CTL Register the nature of the Input
              Video Field presented at the next VSync.
                    - 4:2:2 or 4:2:0
                    - Odd or Even
                    - Interlaced or Progressive
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, if no error occurs
*******************************************************************************/
 ST_ErrorCode_t vdp_dei_SetCvfType (STLAYER_Handle_t   LayerHandle)
{
    U32                              CurrentVideoFieldType = 0;
    HALLAYER_LayerProperties_t    *  LayerProperties_p = (HALLAYER_LayerProperties_t *) LayerHandle;
    HALLAYER_VDPLayerCommonData_t *  LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    if (LayerCommonData_p->SourceFormatInfo.IsFormat422Not420)
    {
        /* 4:2:2 Source */
        CurrentVideoFieldType |= VDP_DEI_CVF_4_2_2;
    }

    if (LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive)
    {
        /* Input video is progressive */
        CurrentVideoFieldType |= VDP_DEI_CVF_PROGRESSIVE;
    }
    else
    {
        /* Input video is interlaced */
        if (LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField.FieldType == STLAYER_TOP_FIELD)

        {
            /* The field presented on next VSync will be a TOP field.
               For VDP IP, the frame starts with Line #1 so Top field = Odd Field */
            CurrentVideoFieldType |= VDP_DEI_CVF_ODD_FIELD;
        }
    }

    TrCvfType(("\r\nCvf Type=0x%x", CurrentVideoFieldType));

    HAL_SetRegister32Value( (void *) ( ((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p) + VDP_DEI_CTL),
                                                 VDP_DEI_CTL_REG_MASK,
                                                 VDP_DEI_CVF_TYPE_MASK,
                                                 VDP_DEI_CVF_TYPE_EMP,
                                                 CurrentVideoFieldType);

    return (ST_NO_ERROR);

}/* End of vdp_dei_SetCvfType() function. */


/********************************************************************************/
/*************************   LOCAL FUNCTIONS   **********************************/
/********************************************************************************/
#if !defined (STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI)
/*******************************************************************************
Name        : vdp_dei_InitMotionBuffers
Description : Allocation of Motion Buffers required (for Previous, Current and Next Fields)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t vdp_dei_InitMotionBuffers (const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Allocate 3 Motion Buffers */
    vdp_dei_AllocateMotionBuffer(LayerHandle, &LayerCommonData_p->MotionBuffer1);
    vdp_dei_AllocateMotionBuffer(LayerHandle, &LayerCommonData_p->MotionBuffer2);
    vdp_dei_AllocateMotionBuffer(LayerHandle, &LayerCommonData_p->MotionBuffer3);

    /* Init Motion Buffer Pointers */
    LayerCommonData_p->PreviousMotionBuffer_p = &LayerCommonData_p->MotionBuffer1;
    LayerCommonData_p->CurrentMotionBuffer_p  = &LayerCommonData_p->MotionBuffer2;
    LayerCommonData_p->NextMotionBuffer_p     = &LayerCommonData_p->MotionBuffer3;

    return (ErrorCode);
}


/*******************************************************************************
Name        : vdp_dei_FreeMotionBuffers
Description :  Free every motion buffers...
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t vdp_dei_FreeMotionBuffers (const STLAYER_Handle_t LayerHandle)
{
    STAVMEM_FreeBlockParams_t          AvmemFreeParams;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;

    if (AvmemFreeParams.PartitionHandle == 0)
    {
        TrError(("\r\nError! Invalid AvmemPartitionHandle"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Delete all references to Motions Buffers */
    LayerCommonData_p->PreviousMotionBuffer_p = NULL;
    LayerCommonData_p->CurrentMotionBuffer_p  = NULL;
    LayerCommonData_p->NextMotionBuffer_p     = NULL;

    /* Free motion buffers */
    STAVMEM_FreeBlock(&AvmemFreeParams, &LayerCommonData_p->MotionBuffer1.BufferHandle);
    STAVMEM_FreeBlock(&AvmemFreeParams, &LayerCommonData_p->MotionBuffer2.BufferHandle);
    STAVMEM_FreeBlock(&AvmemFreeParams, &LayerCommonData_p->MotionBuffer3.BufferHandle);

    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : vdp_dei_AllocateMotionBuffer
Description : Allocate ONE motion buffer in LMI_VID
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t vdp_dei_AllocateMotionBuffer (const STLAYER_Handle_t LayerHandle,
                                                    HALLAYER_VDPMotionBuffer_t * MotionBuffer_p)
{
    ST_ErrorCode_t                 ErrorCode = ST_NO_ERROR;
    STAVMEM_AllocBlockParams_t     AvmemAllocParams;
    void *                         VirtualAddress_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    /* Reset MotionBuffer_p in case memory allocation would fail */
    MotionBuffer_p->BufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    MotionBuffer_p->BaseAddress_p = NULL;

    AvmemAllocParams.Alignment       = 8;
    AvmemAllocParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
    AvmemAllocParams.Size            = MOTION_BUFFER_WIDTH * MOTION_BUFFER_HEIGHT;
    AvmemAllocParams.AllocMode       = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AvmemAllocParams.NumberOfForbiddenRanges    = 0;
    AvmemAllocParams.ForbiddenRangeArray_p      = NULL;
    AvmemAllocParams.NumberOfForbiddenBorders   = 0;
    AvmemAllocParams.ForbiddenBorderArray_p     = NULL;

    ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &MotionBuffer_p->BufferHandle);
    TrMotionBuffer(("\r\nBufferHandle=0x%x", MotionBuffer_p->BufferHandle));

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STAVMEM_GetBlockAddress(MotionBuffer_p->BufferHandle, &VirtualAddress_p);
        TrMotionBuffer(("\r\nVirtualAddress_p=0x%x", VirtualAddress_p));

        if (ErrorCode == ST_NO_ERROR)
        {
            ErrorCode= STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

            if (ErrorCode == ST_NO_ERROR)
            {
                MotionBuffer_p->BaseAddress_p = STAVMEM_VirtualToDevice(VirtualAddress_p, &VirtualMapping);
                TrMotionBuffer(("\r\nBaseAddress_p=0x%x", MotionBuffer_p->BaseAddress_p));
            }
        }
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        TrError(("\r\nError during Motion Buffer Allocation!"));
    }

    return (ErrorCode);
}

/*******************************************************************************
Name        : vdp_dei_RotateMotionBuffers
Description : At each VSync a rotation should be done on Motion Buffers

              "Next Motion Buffer" becomes the "Current Motion Buffer"
              "Current Motion Buffer" becomes the "Previous Motion Buffer"
              "Previous Motion Buffer" becomes available for writting which means that it is now the "Next Motion Buffer"

              Only the Next Motion Buffer is written by hardware. The 2 other buffers are used as references.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t vdp_dei_RotateMotionBuffers (const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *  LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);
    HALLAYER_VDPMotionBuffer_t *     MotionBufferAvailable_p;

    if ( (LayerCommonData_p->PreviousMotionBuffer_p == NULL)                ||
         (LayerCommonData_p->PreviousMotionBuffer_p->BaseAddress_p == NULL) ||
         (LayerCommonData_p->CurrentMotionBuffer_p == NULL)                 ||
         (LayerCommonData_p->CurrentMotionBuffer_p->BaseAddress_p == NULL)  ||
         (LayerCommonData_p->NextMotionBuffer_p == NULL)                    ||
         (LayerCommonData_p->NextMotionBuffer_p->BaseAddress_p == NULL)     )
    {
        TrError(("\r\nError! Motion buffers not initialized"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Save the reference to PreviousMotionBuffer_p before initiating the rotation */
    MotionBufferAvailable_p = LayerCommonData_p->PreviousMotionBuffer_p;

    /* Operate a rotation on Motion buffers: */
    LayerCommonData_p->PreviousMotionBuffer_p = LayerCommonData_p->CurrentMotionBuffer_p;
    LayerCommonData_p->CurrentMotionBuffer_p = LayerCommonData_p->NextMotionBuffer_p;
    LayerCommonData_p->NextMotionBuffer_p = MotionBufferAvailable_p;

    /* Write the Motion Buffer Addresses in the registers */
    vdp_dei_SetMotionBufferAddress(LayerHandle,
                                   LayerCommonData_p->PreviousMotionBuffer_p->BaseAddress_p,
                                   VDP_DEI_PMF_BA);

    vdp_dei_SetMotionBufferAddress(LayerHandle,
                                   LayerCommonData_p->CurrentMotionBuffer_p->BaseAddress_p,
                                   VDP_DEI_CMF_BA);

    vdp_dei_SetMotionBufferAddress(LayerHandle,
                                   LayerCommonData_p->NextMotionBuffer_p->BaseAddress_p,
                                   VDP_DEI_NMF_BA);

    return (ST_NO_ERROR);
} /* End of vdp_dei_RotateMotionBuffers */

/*******************************************************************************
Name        : vdp_dei_SetMotionBufferAddress
Description : Write the Motion Buffer Address in the specified DEI register
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vdp_dei_SetMotionBufferAddress (const STLAYER_Handle_t LayerHandle,
                                            void * BufferAddress_p,
                                            U32 MotionBufferRegisterOffset)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    HAL_WriteDisp32(MotionBufferRegisterOffset, BufferAddress_p);
}
#endif /* STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI */

/*******************************************************************************
Name        : vdp_dei_SetDeiMode
Description : Set the requested mode in the DEI CTL register
                    This function also ensure that the Motion Mode follows the phases Init -> Low -> Full
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vdp_dei_SetDeiMode (const STLAYER_Handle_t LayerHandle, VDP_DEI_Setting_t DeiMode)
{
    U32     DeiCtlRegisterValue;
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    TrMode(("\r\nDEI Mode=0x%04x", DeiMode));
#if defined (ST_7200)
    if (LayerProperties_p->LayerType == MAIN_DISPLAY)
#endif
    {
        /* Management of Motion Mode initialization (NB: "DeiMode" can be modified) */
        vdp_dei_SetMotionMode(LayerHandle, &DeiMode);
    }

    /* Ensure that only the bits about the mode are changed */
    DeiMode &= VDP_DEI_SET_MODE_MASK;

    /* Get the current DEI CTL register value */
    DeiCtlRegisterValue = HAL_ReadDisp32(VDP_DEI_CTL);

    /* Clear the bits corresponding to the mode and set the new one */
    DeiCtlRegisterValue = (DeiCtlRegisterValue & ~VDP_DEI_SET_MODE_MASK) | DeiMode;

    TrMode(("\r\nDEI CTL=0x%08x", DeiCtlRegisterValue));

    /* Write the value to the register */
    HAL_WriteDisp32(VDP_DEI_CTL, DeiCtlRegisterValue);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name        : vdp_dei_Set3DConfiguration
Description : Function used to set the 3D parameters (ex: KCor, KMov...etc)
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t vdp_dei_Set3DConfiguration (STLAYER_Handle_t   LayerHandle)
{
    U32     DeiCtlRegisterValue, Configuration;
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *) LayerHandle;

    Tr3DConf(("\r\nvdp_dei_Set3DConfiguration"));

    /* Read DEI_CTL register */
    DeiCtlRegisterValue = HAL_ReadDisp32(VDP_DEI_CTL);

    Configuration = (VDP_DEI_CTL_DD_MODE_DIAG9 << VDP_DEI_CTL_DD_MODE_EMP)              |
                    (VDP_DEI_CTL_DIRECTIONAL_CLAMPING << VDP_DEI_CTL_CLAMP_MODE_EMP)    |
                    (VDP_DEI_CTL_3RD_DIR_USED << VDP_DEI_CTL_DIR3_EN_EMP)               |
                    (VDP_DEI_CTL_DIR_RECURSIVITY_ON << VDP_DEI_CTL_DIR_REC_EN_EMP)      |
                    (VDP_DEI_CTL_BEST_KCOR_VALUE << VDP_DEI_CTL_KCOR_EMP)               |
                    (VDP_DEI_CTL_BEST_DETAIL_VALUE << VDP_DEI_CTL_T_DETAIL_EMP)         |
                    (VDP_DEI_CTL_KMOV_14_16 << VDP_DEI_CTL_KMOV_FACTOR_EMP);

   /* Ensure that we change only the bits corresponding to the 3DMode */
    Configuration &= VDP_DEI_3D_PARAMS_MASK;

    /* Clear the bits about the 3D mode in DeiCtlRegisterValue */
    DeiCtlRegisterValue &= ~VDP_DEI_3D_PARAMS_MASK;

    /* Write the new 3D mode*/
    DeiCtlRegisterValue |= Configuration;
    Tr3DConf(("\r\nDEI CTL=0x%08x", DeiCtlRegisterValue));

    /* Write the value to the register */
    HAL_WriteDisp32(VDP_DEI_CTL, DeiCtlRegisterValue);

    return(ST_NO_ERROR);
}/* End of vdp_dei_Set3DConfiguration() function. */

/*******************************************************************************
Name        : vdp_dei_SetMotionMode
Description :  Function ensuring that the Motion Mode follows the phases Init -> Low -> Full
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t vdp_dei_SetMotionMode (const STLAYER_Handle_t LayerHandle, VDP_DEI_Setting_t * DeiMode_p)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    /* Check if Motion Detection is requested */
    if ( (*DeiMode_p & (VDP_DEI_CTL_MD_MODE_INIT << VDP_DEI_CTL_MD_MODE_EMP) )  ||
         (*DeiMode_p & (VDP_DEI_CTL_MD_MODE_LOW << VDP_DEI_CTL_MD_MODE_EMP) )  ||
         (*DeiMode_p & (VDP_DEI_CTL_MD_MODE_FULL << VDP_DEI_CTL_MD_MODE_EMP) ) )
    {
        /* Motion Detection is requested */
        /* A state machine is used to ensure that the Motion Mode goes always through the steps Init -> Low -> Full */

        /* Clear the Motion bits */
        *DeiMode_p &= ~(VDP_DEI_CTL_MD_MODE_MASK << VDP_DEI_CTL_MD_MODE_EMP);

        switch (LayerCommonData_p->CurrentMotionMode)
        {
            case HALLAYER_VDP_MOTION_DETECTION_OFF:
                LayerCommonData_p->CurrentMotionMode =  HALLAYER_VDP_MOTION_DETECTION_INIT;
                *DeiMode_p |= (VDP_DEI_CTL_MD_MODE_INIT << VDP_DEI_CTL_MD_MODE_EMP);
                break;

            case HALLAYER_VDP_MOTION_DETECTION_INIT:
                LayerCommonData_p->CurrentMotionMode =  HALLAYER_VDP_MOTION_DETECTION_LOW;
                *DeiMode_p |= (VDP_DEI_CTL_MD_MODE_LOW << VDP_DEI_CTL_MD_MODE_EMP);
                break;

            case HALLAYER_VDP_MOTION_DETECTION_LOW:
                LayerCommonData_p->CurrentMotionMode =  HALLAYER_VDP_MOTION_DETECTION_FULL;
                *DeiMode_p |= (VDP_DEI_CTL_MD_MODE_FULL << VDP_DEI_CTL_MD_MODE_EMP);
                break;

            case HALLAYER_VDP_MOTION_DETECTION_FULL:
                LayerCommonData_p->CurrentMotionMode =  HALLAYER_VDP_MOTION_DETECTION_FULL;
                *DeiMode_p |= (VDP_DEI_CTL_MD_MODE_FULL << VDP_DEI_CTL_MD_MODE_EMP);
                break;

            default:
                TrError(("\r\nInvalid CurrentMotionMode: %d", LayerCommonData_p->CurrentMotionMode));
                LayerCommonData_p->CurrentMotionMode =  HALLAYER_VDP_MOTION_DETECTION_OFF;
                break;
        }
    }
    else
    {
        /* Reset MotionMode state */
        LayerCommonData_p->CurrentMotionMode =  HALLAYER_VDP_MOTION_DETECTION_OFF;
    }

    TrMotion(("\r\nMotion: %d", LayerCommonData_p->CurrentMotionMode));

    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : vdp_dei_CheckFieldsParity
Description : This function displays a warning if the Fields are not alternately Top / Bottom
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vdp_dei_CheckFieldsParity(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    /* OLO_TO_DO: Shall we ignore Fields with bad parity??? */

    /* Check if Previous and Current Fields have the same parity */
    if ( (LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.FieldAvailable) &&
          (LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.FieldAvailable)   &&
          ( LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.FieldType ==
            LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.FieldType)  )
    {
        TrWarning(("\r\nPrev and Curr have the same parity!"));
    }

    /* Check if Current and Next Fields have the same parity */
    if ( (LayerCommonData_p->FieldsToUseAtNextVSync.NextField.FieldAvailable) &&
          (LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.FieldAvailable)   &&
          ( LayerCommonData_p->FieldsToUseAtNextVSync.NextField.FieldType ==
            LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.FieldType)  )
    {
        TrWarning(("\r\nCurr and Next have the same parity!"));
    }
}

/*******************************************************************************
Name        : vdp_dei_DisplayFieldAvailable
Description : Function called to display the available Fields References
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vdp_dei_DisplayFieldAvailable(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    /* Previous */
    if (LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.FieldAvailable)
    {
        TrMain(("\r\nPrev=%d%s",
                LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.PictureIndex,
                FIELD_TYPE(LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField) ));
    }
    else
    {
        TrMain(("\r\nPrev Field not avail"));
    }

    /* Current (always available) */
    TrMain(("\r\nCur=%d%s",
                LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.PictureIndex,
                FIELD_TYPE(LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField) ));

    /* Next */
    if (LayerCommonData_p->FieldsToUseAtNextVSync.NextField.FieldAvailable)
    {
        TrMain(("\r\nNext=%d%s",
                LayerCommonData_p->FieldsToUseAtNextVSync.NextField.PictureIndex,
                FIELD_TYPE(LayerCommonData_p->FieldsToUseAtNextVSync.NextField) ));
    }
    else
    {
        TrMain(("\r\nNext Field not avail"));
    }

}

/*******************************************************************************
Name        : vdp_dei_DisplayFormatInfo
Description : This function displays debug information about the Format of the source
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, if no error occurs
*******************************************************************************/
static ST_ErrorCode_t vdp_dei_DisplayFormatInfo (STLAYER_Handle_t    LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    UNUSED_PARAMETER(LayerCommonData_p);    /* Added to remove the warning when Debug Print are not activated... */

    /* Input Format */
    TrFormatInfo(("\r\n"));
    TrFormatInfo(("\r\n IsFormat422Not420=%d", LayerCommonData_p->SourceFormatInfo.IsFormat422Not420));
    TrFormatInfo(("\r\n IsInputScantypeProgressive=%d", LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive));
    TrFormatInfo(("\r\n IsFormatRasterNotMacroblock=%d", LayerCommonData_p->SourceFormatInfo.IsFormatRasterNotMacroblock));

    /* Source size */
    TrFormatInfo(("\r\n Height=%d", LayerCommonData_p->SourcePictureDimensions.Height ));
    TrFormatInfo(("\r\n Width=%d", LayerCommonData_p->SourcePictureDimensions.Width ));

    /* Output Format */
    TrFormatInfo(("\r\n IsOutputScantypeProgressive=%d", LayerCommonData_p->IsOutputScantypeProgressive));

    return(ST_NO_ERROR);
}/* End of vdp_dei_DisplayFormatInfo() function. */

/*******************************************************************************
Name        : vdp_dei_SelectDeiMode
Description :  This function Select the appropriate DEI mode according to the user request and the
                    Field buffers available.
                    This is the DECISION function.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vdp_dei_SelectDeiMode(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);
#if defined (ST_7200)
    HALLAYER_LayerProperties_t    * LayerProperties_p =  (HALLAYER_LayerProperties_t *) LayerHandle;
#endif /* 7200 */

    /* Check if the VDP is enabled */
    if (!LayerCommonData_p->IsViewportEnabled)
    {
        vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_OFF);
        goto done;
    }

    /* Check all the conditions which can lead to put the DEI in Bypass mode */
#if defined (ST_7200)
    if(LayerProperties_p->LayerType == MAIN_DISPLAY)
#endif /* 7200 */
    {
        if (vdp_dei_IsBypassModeNecessary(LayerHandle) )
        {
            vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_BYPASS);
            goto done;
        }
    }

    /* Temporal Interpolation should be done only if the Input and Output Rectangle are unchanged */
    if (!vdp_dei_IsTemporalInterpolationAllowed(LayerHandle))
    {
        vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_DIRECTIONAL);
        goto done;
    }

    TrMode(("\r\nRequested Mode %d", LayerCommonData_p->RequestedDeiMode));

    switch (LayerCommonData_p->RequestedDeiMode)
    {
        case STLAYER_DEI_MODE_OFF:
            vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_OFF);
            break;

        case STLAYER_DEI_MODE_AUTO:
            /* The Automatic mode is not used for the moment: Call the Bypass mode instead */
            vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_BYPASS);
            break;

        default:
        case STLAYER_DEI_MODE_BYPASS:
            vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_BYPASS);
            break;

        case STLAYER_DEI_MODE_VERTICAL:
            vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_VERTICAL);
            break;

        case STLAYER_DEI_MODE_DIRECTIONAL:
             vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_DIRECTIONAL);
            break;

        case STLAYER_DEI_MODE_FIELD_MERGING:
            if ( (LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.FieldAvailable) &&
                  (LayerCommonData_p->FieldsToUseAtNextVSync.NextField.FieldAvailable) )
            {
                vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_FIELD_MERGING);
            }
            else
            {
                /* Fields not available => we use a mode which needs only the current field */
                vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_DIRECTIONAL);
            }
            break;

        case STLAYER_DEI_MODE_MEDIAN_FILTER:
            if (LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.FieldAvailable)
            {
                vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_MEDIAN_FILTER);
            }
            else
            {
                /* Field not available => we use a mode which needs only the current field */
                vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_DIRECTIONAL);
            }
            break;

        case STLAYER_DEI_MODE_MLD:
            if (LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.FieldAvailable)
            {
                if (LayerCommonData_p->FieldsToUseAtNextVSync.NextField.FieldAvailable)
                {
                    vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_MLD);
                }
                else
                {
                    /* Prevous Field available but not Next Field => we use a mode which needs only the previous and current fields */
                    vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_MEDIAN_FILTER);
                }
            }
            else
            {
                /* Prevous Field not available => we use a mode which needs only the current field */
                vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_DIRECTIONAL);
            }
            break;

        case STLAYER_DEI_MODE_LMU:
            if (LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.FieldAvailable)
            {
                if (LayerCommonData_p->FieldsToUseAtNextVSync.NextField.FieldAvailable)
                {
                    vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_LMU);
                }
                else
                {
                    /* Prevous Field available but not Next Field => we use a mode which needs only the previous and current fields */
                    vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_MEDIAN_FILTER);
                }
            }
            else
            {
                /* Prevous Field not available => we use a mode which needs only the current field */
                vdp_dei_ApplyMode(LayerHandle, STLAYER_DEI_MODE_DIRECTIONAL);
            }
            break;
    }

done:
    return;
}

/*******************************************************************************
Name        : vdp_dei_ApplyMode
Description : This function simply applies a mode.
                    It doesn't check if the required Fields are present. This work is done by
                    "vdp_dei_SelectDeiMode"
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vdp_dei_ApplyMode(const STLAYER_Handle_t LayerHandle,
                              STLAYER_DeiMode_t Mode)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);
#if defined (ST_7200)
    HALLAYER_LayerProperties_t  *   LayerProperties_p = (HALLAYER_LayerProperties_t *) LayerHandle;
#endif /* 7200 */

    TrMain(("\r\nApply Mode %d", Mode));

    /* Save the mode chosen */
    LayerCommonData_p->AppliedDeiMode = Mode;

    if ( (LayerCommonData_p->RequestedDeiMode != STLAYER_DEI_MODE_AUTO)    &&
          (LayerCommonData_p->AppliedDeiMode != LayerCommonData_p->RequestedDeiMode)  )
    {
        /* Inform that the requested mode could not be set (this has no meaning in case of Automatic Mode request) */
        TrMode(("\r\nRequestedDeiMode=%d <==> AppliedDeiMode=%d",
                                LayerCommonData_p->RequestedDeiMode,
                                LayerCommonData_p->AppliedDeiMode));
    }

#ifdef ST_fmdenabled
    vdp_fmd_SetDefaultMode(LayerHandle);
#endif

#if !defined (STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI)
    #if defined (ST_7200)
        if (LayerProperties_p->LayerType == MAIN_DISPLAY)
    #endif /* 7200 */
        {
            /* At each VSync a rotation should be done on Motion Buffers */
            vdp_dei_RotateMotionBuffers(LayerHandle);
        }
#endif /* STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI */

    switch (LayerCommonData_p->AppliedDeiMode)
    {
        case STLAYER_DEI_MODE_OFF:
            vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_OFF);
            break;

        case STLAYER_DEI_MODE_AUTO:
            TrError(("\r\nError in VDP_DEI_ApplyMode: STLAYER_DEI_MODE_AUTO is not an accepted value"));
            /* Set the DEI in Bypass mode... */
            vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_BYPASS);
            break;

        default:
        case STLAYER_DEI_MODE_BYPASS:
            vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_BYPASS);
            break;

        case STLAYER_DEI_MODE_VERTICAL:
            vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_YC_VI);
            break;

        case STLAYER_DEI_MODE_DIRECTIONAL:
            vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_YC_DI);
            break;

        case STLAYER_DEI_MODE_FIELD_MERGING:
            /* When doing Field Merging, the merge should be done between the Current and the Previous OR Next Field.
                This is known only when the FMD result will be available, so this let a very short time before the Next VSync.
                In order to minimize the processing that should be done during the Vertical Front Porch, we
                prepare the buffers in the "usual" order (Previous, Current, Next) and, when we get the
                FMD result, we may swap the Previous and Next Buffers if necessary
            */
            vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_YC_FM);
            break;

        case STLAYER_DEI_MODE_MEDIAN_FILTER:
            vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_YC_MF);
            break;

        case STLAYER_DEI_MODE_MLD:
            vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_YC_MLD);
            break;

        case STLAYER_DEI_MODE_LMU:
            vdp_dei_SetDeiMode(LayerHandle, VDP_DEI_SET_MODE_YC_LMU);
            break;
    }
}

/*******************************************************************************
Name        : vdp_dei_ResetField
Description : Reset a structure "STLAYER_Field_t"
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vdp_dei_ResetField(const STLAYER_Handle_t LayerHandle, STLAYER_Field_t * Field_p)
{
    UNUSED_PARAMETER(LayerHandle);

    Field_p->FieldAvailable = FALSE;
    Field_p->PictureIndex = 0;
    /* "Field_p->FieldType" does not matter */
}


/*******************************************************************************
Name        : vdp_dei_CheckFieldConsecutiveness
Description : Function used to detect if the Previous, Current and Next Fields are really consecutive.
                   Otherwise we may decide to discard this reference in order to avoid interpolation
                   between non consecutive Fields.
                   The PictureIndex is used to detect the consecutivity.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vdp_dei_CheckFieldConsecutiveness(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    /* NB: Current Field is always available */

    if (LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.FieldAvailable)
    {
        if ( (LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.PictureIndex -LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.PictureIndex) > 1 )
        {
            /* Previous and Current are not consecutive */
            #ifdef INTERPOLATE_ONLY_WITH_CONSECUTIVE_FIELDS
                /* Previous and Current Fields not consecutive: Remove reference to Previous Field */
                vdp_dei_ResetField(LayerHandle, &LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField);
            #else
                TrWarning(("\r\nInterpolation with Previous Field not consecutive!"));
                /* OLO_TO_DO: Add a test in order to allow the merge only if the Field are distant of less than a given threshold */
            #endif
        }
    }

    if (LayerCommonData_p->FieldsToUseAtNextVSync.NextField.FieldAvailable)
    {
        if ( (LayerCommonData_p->FieldsToUseAtNextVSync.NextField.PictureIndex -LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.PictureIndex) > 1 )
        {
            /* Current and Next are not consecutive */
            #ifdef INTERPOLATE_ONLY_WITH_CONSECUTIVE_FIELDS
                /* Current and Next Fields not consecutive: Remove reference to Next Field */
                vdp_dei_ResetField(LayerHandle, &LayerCommonData_p->FieldsToUseAtNextVSync.NextField);
            #else
                TrWarning(("\r\nInterpolation with Next Field not consecutive!"));
            #endif
        }
    }
} /* End of vdp_dei_CheckFieldConsecutiveness */

/*******************************************************************************
Name        : vdp_dei_ResetFieldArray
Description : Reset a Field Triplet
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vdp_dei_ResetFieldArray(const STLAYER_Handle_t LayerHandle,
                                    HALLAYER_VDPFieldArray_t * FieldArray_p)
{
    vdp_dei_ResetField(LayerHandle, &FieldArray_p->PreviousField);
    vdp_dei_ResetField(LayerHandle, &FieldArray_p->CurrentField);
    vdp_dei_ResetField(LayerHandle, &FieldArray_p->NextField);
}

/*******************************************************************************
Name        : vdp_dei_SaveFieldsUsedByHardware
Description : Function called when a new VSync has occured: The "FieldsToUseAtNextVSync"
                    now become the Fields used by the Harware.
                    This information will be used when we collect the FMD result.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vdp_dei_SaveFieldsUsedByHardware(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    memcpy( &LayerCommonData_p->FieldsCurrentlyUsedByHw.PreviousField,
                    &LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField,
                    sizeof(STLAYER_Field_t) );

    memcpy( &LayerCommonData_p->FieldsCurrentlyUsedByHw.CurrentField,
                    &LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField,
                    sizeof(STLAYER_Field_t) );

    memcpy( &LayerCommonData_p->FieldsCurrentlyUsedByHw.NextField,
                    &LayerCommonData_p->FieldsToUseAtNextVSync.NextField,
                    sizeof(STLAYER_Field_t) );
} /* End of vdp_dei_SaveFieldsUsedByHardware */

/*******************************************************************************
Name        : vdp_dei_SaveFieldsUsedAtNextVSync
Description : Save the references to the Fields that will be used at next VSync
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vdp_dei_SaveFieldsUsedAtNextVSync(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    memcpy( &LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField,
                    &LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->PreviousField,
                    sizeof(STLAYER_Field_t) );

    memcpy( &LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField,
                    &LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField,
                    sizeof(STLAYER_Field_t) );

    memcpy( &LayerCommonData_p->FieldsToUseAtNextVSync.NextField,
                    &LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->NextField,
                    sizeof(STLAYER_Field_t) );

} /* End of vdp_dei_SaveFieldsUsedAtNextVSync */

/*******************************************************************************
Name        : vdp_dei_IsBypassModeNecessary
Description : This function determines if the DEI should be activated.
Parameters  :
Assumptions :
Limitations :
Returns     : A Boolean indicating if the DEI should be Bypassed
*******************************************************************************/
BOOL vdp_dei_IsBypassModeNecessary (const STLAYER_Handle_t     LayerHandle)
{
    BOOL                            IsBypassNeeded = FALSE;
    HALLAYER_VDPLayerCommonData_t   *LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

#ifdef ST_XVP_ENABLE_FGT
    HALLAYER_LayerProperties_t      *LayerProperties_p = (HALLAYER_LayerProperties_t*)LayerHandle;
#endif /* ST_XVP_ENABLE_FGT */

#ifdef FORCE_DEI_BYPASS
    IsBypassNeeded = TRUE;
    goto done;
#endif

    if (LayerCommonData_p->RequestedDeiMode == STLAYER_DEI_MODE_BYPASS)
    {
        TrWarning(("\r\nBypass mode requested by used"));
        IsBypassNeeded = TRUE;
        goto done;
    }

    /* Deinterlacing is necessary only if the source is interlaced */
    if (LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive)
    {
        TrWarning(("\r\nProgressive stream => DEI Bypassed"));
        IsBypassNeeded = TRUE;
        goto done;
    }

#ifdef ST_XVP_ENABLE_FGT
    if (LayerProperties_p->IsFGTBypassed == FALSE)
    {
        TrWarning(("\r\nFilm Grain active => DEI Bypassed"));
        IsBypassNeeded = TRUE;
        goto done;
    }
#endif /* ST_XVP_ENABLE_FGT */

    /* Check if the Picture is Decimated */
    if (LayerCommonData_p->IsDecimationActivated)
    {
        TrWarning(("\r\nPicture Decimated => DEI Bypassed"));
        IsBypassNeeded = TRUE;
        goto done;
    }

    if (LayerCommonData_p->ViewPortParams.InputRectangle.Height == 0)
    {
        TrWarning(("\r\n Input height is NULL => DEI Bypassed"));
        IsBypassNeeded = TRUE;
        goto done;
    }

    /* Check if the Input width is not too big for the deinterlacer (720 Pixels per line maximum) */
    if (LayerCommonData_p->ViewPortParams.InputRectangle.Width > MAX_SOURCE_PICTURE_WIDTH)
    {
        TrWarning(("\r\nInput width too big => DEI Bypassed"));
        IsBypassNeeded = TRUE;
        goto done;
    }

    /* Do not use DEI for interlaced output types if (output viewport height)/(source height) < HALLAYER_VERTICAL_ZOOM_DEI_ACTIVATION_THRESHOLD */
    if (!LayerCommonData_p->IsOutputScantypeProgressive &&
       ((100 * LayerCommonData_p->ViewPortParams.OutputRectangle.Height
         / LayerCommonData_p->ViewPortParams.InputRectangle.Height) < HALLAYER_VERTICAL_ZOOM_DEI_ACTIVATION_THRESHOLD))
    {
        TrWarning(("\r\n (Output Height/Source Height) < Threshold => DEI Bypassed"));
        IsBypassNeeded = TRUE;
        goto done;
    }

    /* Check that the Luma Bitrate will not be too high when DEI is activated.
      When DEI is activated, there is not only the Current Luma Field going through the STBus. There is also:
      - Previous Luma Field
      - Next Luma Field
      - 3 Motion Fields (two are read, one is written)
      Thus the Bitrate is 6 time higher compare to Bypasse mode */
    if ( (6 * LayerCommonData_p->LumaDataRateInBypassMode) > MAX_LUMA_DATA_RATE)
    {
        TrWarning(("\r\n LumaDataRate too high (6 * %d): DEI not activated", LayerCommonData_p->LumaDataRateInBypassMode));
        IsBypassNeeded = TRUE;
        goto done;
    }

done:
    return(IsBypassNeeded);
}/* End of vdp_dei_IsBypassModeNecessary() function. */


/*******************************************************************************
Name        : vdp_dei_IsTemporalInterpolationAllowed
Description :  This function check Temporal Interpolation is allowed. This kind of interpolation
               is allowed only if the Input and Output Rectangle haven't changed.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL vdp_dei_IsTemporalInterpolationAllowed(const STLAYER_Handle_t LayerHandle)
{
    BOOL  IsTemporalInterpolationAllowed;

#if !defined (STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI)
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    if ( (vdp_dei_RectanglesAreIdentical(&LayerCommonData_p->ViewPortParams.InputRectangle, &LayerCommonData_p->LastInputRectangle) ) &&
         (vdp_dei_RectanglesAreIdentical(&LayerCommonData_p->ViewPortParams.OutputRectangle, &LayerCommonData_p->LastOutputRectangle) ) )
    {
        /* Input AND Output rectangles did not change */
        IsTemporalInterpolationAllowed = TRUE;
    }
    else
    {
        TrWarning(("\r\nInput or Output Rectangle has changed => Temporal Interpolation not allowed"));
        IsTemporalInterpolationAllowed = FALSE;

        /* Save the new Rectangles */
        LayerCommonData_p->LastInputRectangle = LayerCommonData_p->ViewPortParams.InputRectangle;
        LayerCommonData_p->LastOutputRectangle = LayerCommonData_p->ViewPortParams.OutputRectangle;
    }
#else /* STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI */
    UNUSED_PARAMETER(LayerHandle);

    TrWarning(("\r\nNo extra motion buffers allocated => Temporal Interpolation not allowed"));
    IsTemporalInterpolationAllowed = FALSE;
#endif /* STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI */

    return(IsTemporalInterpolationAllowed);
} /* End of vdp_dei_IsTemporalInterpolationAllowed */


#if !defined (STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI)
/*******************************************************************************
Name        : vdp_dei_RectanglesAreIdentical
Description :  This function check if 2 rectangles are identical (same size, same position)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL vdp_dei_RectanglesAreIdentical(STGXOBJ_Rectangle_t * Rectangle1_p, STGXOBJ_Rectangle_t * Rectangle2_p)
{
    if ( (Rectangle1_p->Height    == Rectangle2_p->Height)    &&
         (Rectangle1_p->Width     == Rectangle2_p->Width)     &&
         (Rectangle1_p->PositionX == Rectangle2_p->PositionX) &&
         (Rectangle1_p->PositionY == Rectangle2_p->PositionY) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
#endif /* STLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI */

#ifdef DBG_CHECK_VDP_CONSISTENCY
/*******************************************************************************
Name        : vdp_dei_StartVdpFifoEmptyInterrupt
Description : Enable the Fifo Empty Interrupt
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t vdp_dei_StartVdpFifoEmptyInterrupt(const STLAYER_Handle_t LayerHandle)
{
    STOS_Error_t           ErrorCode;
    U32                         PxfITMask;
    char                        IrqName[30] = "VDP_FIFO_EMPTY_IRQ";
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    /* Install the IT Handler for the VdpFifoEmpty Interrupt */
    STOS_InterruptLock();
    ErrorCode = STOS_InterruptInstall(
                    VDP_FIFO_EMPTY_INTERRUPT_NUMBER,
                    VDP_FIFO_EMPTY_INTERRUPT_LEVEL,
                    VdpFifoEmptyInterruptHandler,
                    IrqName,
                    (void *) LayerHandle);
    STOS_InterruptUnlock();

    if (ErrorCode != STOS_SUCCESS)
    {
        TrError(("\r\nError during VDP_FIFO_EMPTY_IRQ install"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Enable the Interrupt Handler */
    if (STOS_InterruptEnable(VDP_FIFO_EMPTY_INTERRUPT_NUMBER,
                                              VDP_FIFO_EMPTY_INTERRUPT_LEVEL) != STOS_SUCCESS)
    {
        TrError(("\r\nError during VDP_FIFO_EMPTY_IRQ enabling"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* The Interrupt Handler is declared so we can now enable the Interrupt (at the VDP level) */
    PxfITMask = HAL_ReadDisp32(VDP_P2I_PXF_IT_MASK);
    HAL_WriteDisp32(VDP_P2I_PXF_IT_MASK,  PxfITMask | VDP_PXF_FIFO_EMPTY);

    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : vdp_dei_StopVdpFifoEmptyInterrupt
Description : Disable the Fifo Empty Interrupt
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t vdp_dei_StopVdpFifoEmptyInterrupt(const STLAYER_Handle_t LayerHandle)
{
    STOS_Error_t           ErrorCode;
    U32                         PxfITMask;
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    /* Disable the Interrupt (at the VDP level) */
    PxfITMask = HAL_ReadDisp32(VDP_P2I_PXF_IT_MASK);
    HAL_WriteDisp32(VDP_P2I_PXF_IT_MASK,  PxfITMask & ~VDP_PXF_FIFO_EMPTY);

    /* Disable the Interrupt (at the OS level) */
    if (STOS_InterruptDisable(VDP_FIFO_EMPTY_INTERRUPT_NUMBER,
                                               VDP_FIFO_EMPTY_INTERRUPT_LEVEL) != STOS_SUCCESS)
    {
        TrError(("\r\nError during VDP_FIFO_EMPTY_IRQ disabling"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Uninstall the IT Handler */
    STOS_InterruptLock();
    ErrorCode = STOS_InterruptUninstall(VDP_FIFO_EMPTY_INTERRUPT_NUMBER,
                                                                VDP_FIFO_EMPTY_INTERRUPT_LEVEL,
                                                                NULL );
    STOS_InterruptUnlock();

    if (ErrorCode != STOS_SUCCESS)
    {
        TrError(("\r\nError during VDP_FIFO_EMPTY_IRQ uninstallation"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    return ST_NO_ERROR;
}
#endif /* DBG_CHECK_VDP_CONSISTENCY */

/******************************************************************************/
/*************************         INTERRUPT        **********************************/
/******************************************************************************/

#ifdef DBG_CHECK_VDP_CONSISTENCY
/*******************************************************************************
Name        : VdpFifoEmptyInterruptHandler
Description : PXF Fifo empty interrupt handler : !!! we are under interrupt context !!
Parameters  : Params contains in fact the LayerHandle
Assumptions :
Limitations :
Returns     :
Comment     : STOS_INTERRUPT_DECLARE is used in order to be free from
            : OS dependent interrupt function prototype
*******************************************************************************/
static STOS_INTERRUPT_DECLARE(VdpFifoEmptyInterruptHandler, Param)
{
    STLAYER_Handle_t    LayerHandle = (STLAYER_Handle_t) Param;
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *) LayerHandle;
    /*HALLAYER_VDPLayerCommonData_t * LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);*/

    TrError(("\r\nIT FIFO_EMPTY !!!"));

    /* Write '1' in "VDP_PXF_FIFO_EMPTY" in order to reset the interrupt */
    HAL_WriteDisp32(VDP_P2I_PXF_IT_STATUS, VDP_PXF_FIFO_EMPTY);

    STOS_INTERRUPT_EXIT(STOS_SUCCESS);
}
#endif /* DBG_CHECK_VDP_CONSISTENCY */

/******************************************************************************/

/* End of hv_vdpdei.c */
