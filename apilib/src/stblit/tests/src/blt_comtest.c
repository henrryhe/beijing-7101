/******************************************************************************
*	COPYRIGHT (C) STMicroelectronics 1998
*
*   File name   : BLT_comtest.c
*   Description : Debug macros
*
*	Date          Modification                                    Initials
*	----          ------------                                    --------
*   05 Feb 2001   Creation                                        VV
*
******************************************************************************/

/*###########################################################################*/
/*############################## INCLUDES FILE ##############################*/
/*###########################################################################*/

#include <stdio.h>
#include <string.h>

#ifdef ST_OS21
    #include "os21debug.h"
    #include <sys/stat.h>
    #include <fcntl.h>
	#include "stlite.h"
#endif /*End of ST_OS21*/

#ifdef ST_OS20
	#include <debug.h>  /* used to read a file */
	#include "stlite.h"
#endif /*End of ST_OS20*/

#include "stddefs.h"
#include "stdevice.h"
#include "testtool.h"
#ifdef ST_OSLINUX
#include "iocstapigat.h"
#else
#include "sttbx.h"
#endif
#include "api_test.h"
#include "stblit.h"
#include "stcommon.h"
#include "startup.h"
#include "stevt.h"
#ifndef ST_OSLINUX
#include "stavmem.h"
#else
#include "stlayer.h"
#endif
#include "stsys.h"
#include "clevt.h"
#ifndef ST_OSLINUX
#include "clavmem.h"
#endif
#include "config.h"



#ifndef STBLIT_EMULATOR
#include "clintmr.h"
#endif


/*###########################################################################*/
/*############################### DEFINITION ################################*/
/*###########################################################################*/

/* --- Constants --- */
#define FILL_METHOD
#define TYPE_CLUT2 2
#define TYPE_CLUT4 4
#define TYPE_CLUT8 8
#define TYPE_RGB565 565
#define TYPE_RGB888 888
#define TYPE_YCbCr422R 422
/* for 7109 cut2.0 (secured chip)*/
#if defined (ST_OSLINUX)
#if defined (DVD_SECURED_CHIP)
#define SECURED_DST_BITMAP TRUE
#if defined (SECURED_BITMAP)
#define SECURED_SRC_BITMAP TRUE
#else
#define SECURED_SRC_BITMAP FALSE
#endif
#else
#define SECURED_SRC_BITMAP FALSE
#define SECURED_DST_BITMAP FALSE
#endif

#else /* ! (ST_OSLINUX) */
#if defined (DVD_SECURED_CHIP)
#if defined (SECURED_NODE)
#define AVMEM_PARTITION_NUM   0      /*secured partition to allocate node, CLUT and filters coeifficients*/
#else
#define AVMEM_PARTITION_NUM   1      /*un-secured partition to allocate node, CLUT and filters coeifficients*/
#endif
#if defined (SECURED_BITMAP)
#define SRC_BITMAP_AVMEM_PARTITION_NUM 0  /*secured partition for source bitmap allocation*/
#else
#define SRC_BITMAP_AVMEM_PARTITION_NUM 1  /*un-secured partition for source bitmap allocation*/
#endif
#define DST_BITMAP_AVMEM_PARTITION_NUM 0  /* the destination Bitmap is alawys in secured partition*/

#else
#define AVMEM_PARTITION_NUM   0
#define SRC_BITMAP_AVMEM_PARTITION_NUM 0
#define DST_BITMAP_AVMEM_PARTITION_NUM 0
#endif
#endif

#if defined(ST_OSLINUX)
#define TEST_DELAY 100000 /* for test prupose to avoid timeout problems */

/*#define  CALC_TIME 1*/
#endif

/* --- Prototypes --- */

/* --- Global variables --- */
#ifdef ST_OSLINUX
static char SrcFileName[]="./blt/files/suzieRGB565.gam";
static char DstFileName[]="./blt/files/merouRGB565.gam";
#else
static char SrcFileName[30]="../../suzieRGB565.gam";
static char DstFileName[30]="../../merouRGB565.gam";
#endif
static char UserTagString[]="BLIT";

/* RASTER_TOP_BOTTOM format test related */
#ifdef ST_OSLINUX
static char TopBottomSourceFileName[]      = "./blt/files/crowRGB565.gam"   ;
static char TopFieldFileName[]    = "./blt/files/crowRGB565_TopField.bin";
static char BottomFieldFileName[] = "./blt/files/crowRGB565_BottomField.bin";
#else
static char TopBottomSourceFileName[]      = "../../crowRGB565.gam"   ;
static char TopFieldFileName[]    = "../../crowRGB565_TopField.bin";
static char BottomFieldFileName[] = "../../crowRGB565_BottomField.bin";
#endif
static U32 NbBytePerPixel = 2; /* For RGB565 */
static STGXOBJ_ColorType_t TopBottomFieldColorType = STGXOBJ_COLOR_TYPE_RGB565;
static STGXOBJ_BitmapType_t TopBottomFieldBitmapType = STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM;
static BOOL TopBottomFieldPreMultipliedColor = FALSE;
static STGXOBJ_ColorSpaceConversionMode_t TopBottomFieldColorSpaceConversion = STGXOBJ_ITU_R_BT709;
static STGXOBJ_AspectRatio_t TopBottomFieldAspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
static U32 TopBottomFieldWidth = 741;
static U32 TopBottomFieldHeight = 492;
static U32 TopBottomFieldAllocAlignment = 2; /* For RGB565 */

/* --- Externals --- */
extern U8                           BlitterRegister[];
#ifndef ST_OSLINUX
extern STAVMEM_PartitionHandle_t    AvmemPartitionHandl[];
#if defined (DVD_SECURED_CHIP)
#define  SECURED_PARTITION_START 0x5480200
#define  SECURED_PARTITION_STOP  0x8000000
#endif
#else
#if defined (DVD_SECURED_CHIP)
#define  SECURED_PARTITION_START 0x7b00000
#define  SECURED_PARTITION_STOP  0x8000000
#endif
#endif



extern ST_Partition_t*              SystemPartition_p;
extern char BLT_Msg[200];                              /* text for trace */

#if defined(ST_OS21) || defined(ST_OSLINUX)
    extern semaphore_t* BlitCompletion;
    extern semaphore_t* JobCompletion;

#endif
#ifdef ST_OS20
    extern semaphore_t BlitCompletion;
    extern semaphore_t JobCompletion;
#endif

extern ST_ErrorCode_t ConvertBitmapToGamma ( char*, STGXOBJ_Bitmap_t*, STGXOBJ_Palette_t* );
#ifdef ST_OSLINUX
extern ST_ErrorCode_t ConvertGammaToBitmap ( char*,BOOL , STGXOBJ_Bitmap_t*, STGXOBJ_Palette_t* );
#else
extern ST_ErrorCode_t ConvertGammaToBitmap ( char*, U8, STGXOBJ_Bitmap_t*, STGXOBJ_Palette_t* );
#endif

extern void BlitCompletedHandler (STEVT_CallReason_t, const ST_DeviceName_t, STEVT_EventConstant_t, const void*, const void* );
extern void JobCompletedHandler (STEVT_CallReason_t, const ST_DeviceName_t, STEVT_EventConstant_t, const void*, const void* );

#ifdef ST_OSLINUX



extern ST_ErrorCode_t AllocBitmapBuffer (STGXOBJ_Bitmap_t* Bitmap_p, BOOL SecuredData, void* SharedMemoryBaseAddress_p,
                        STGXOBJ_BitmapAllocParams_t* Params1_p,
                        STGXOBJ_BitmapAllocParams_t* Params2_p);
extern ST_ErrorCode_t GUTIL_Free(void *ptr);
extern ST_ErrorCode_t GUTIL_Allocate (STLAYER_AllocDataParams_t *AllocDataParams_p, void **ptr);


extern STLAYER_Handle_t FakeLayerHndl ;
#else
extern ST_ErrorCode_t AllocBitmapBuffer (STGXOBJ_Bitmap_t* Bitmap_p , void* SharedMemoryBaseAddress_p, STAVMEM_PartitionHandle_t AVMEMPartition,
                        STAVMEM_BlockHandle_t*  BlockHandle1_p,
                        STGXOBJ_BitmapAllocParams_t* Params1_p, STAVMEM_BlockHandle_t*  BlockHandle2_p,
                        STGXOBJ_BitmapAllocParams_t* Params2_p);
ST_ErrorCode_t AllocPaletteBuffer (STGXOBJ_Palette_t* Palette_p ,
                                    void* SharedMemoryBaseAddress_p,
                                    STAVMEM_PartitionHandle_t AVMEMPartition,
                                    STAVMEM_BlockHandle_t*  BlockHandle_p,
                                    STGXOBJ_PaletteAllocParams_t* Params_p );
extern ST_ErrorCode_t GUTIL_Free(void *ptr);
extern ST_ErrorCode_t GUTIL_Allocate (STAVMEM_AllocBlockParams_t *AllocBlockParams_p, void **ptr);
#endif

extern U32 NodeID[20];
extern U32 Index;

#if defined(ST_OS21) || defined(ST_OSLINUX)
	extern semaphore_t* BlitCompletion;
	extern semaphore_t* BlitCompletionTimeout;
	extern semaphore_t* JobCompletion;
	extern semaphore_t* JobCompletionTimeout;
#endif
#ifdef ST_OS20
	extern semaphore_t BlitCompletion;
	extern semaphore_t BlitCompletionTimeout;
	extern semaphore_t JobCompletion;
	extern semaphore_t JobCompletionTimeout;
#endif

#ifndef ST_OSLINUX
extern STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
#endif

#ifdef ST_OSLINUX
extern MapParams_t   BlitterMap;
#define LINUX_BLITTER_BASE_ADDRESS    BlitterMap.MappedBaseAddress
#endif

/* --- Private functions prototypes---*/

/* --- Private types --- */

/* --- Functions --- */
#ifndef STBLIT_EMULATOR
static void ResetEngine(void)
{
#if defined(ST_GX1)

#else
    void *Address;

#ifdef ST_OSLINUX
    Address = LINUX_BLITTER_BASE_ADDRESS;
#else
    Address = CURRENT_BASE_ADDRESS;
#endif

#ifndef ST_OSLINUX   /*not implemented for linux */
    STSYS_WriteRegDev32LE(Address, 1);
    STSYS_WriteRegDev32LE(Address, 0);
#endif
#endif
}
#endif


#ifdef STBLIT_LINUX_FULL_USER_VERSION
/*-----------------------------------------------------------------------------
 * Function : BlitCompleted_Task
 * Input    : *Context_p
 * Output   :
 * Return   : void
 * --------------------------------------------------------------------------*/
static void BlitCompleted_Task(void* data_p)
{

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"BlitCompleted_Task: Enter\n"));

    while (1)
    {
        if ( STBLIT_NotifyCompletion(TRUE) == ST_NO_ERROR )
        {
            semaphore_signal( BlitCompletionTimeout );
        }
    }
}
/*-----------------------------------------------------------------------------
 * Function : JobCompleted_Task
 * Input    : *Context_p
 * Output   :
 * Return   : void
 * --------------------------------------------------------------------------*/
static void JobCompleted_Task(void* data_p)
{

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"BlitCompleted_Task: Enter\n"));

    while (1)
    {
        if ( STBLIT_NotifyCompletion(FALSE) == ST_NO_ERROR )
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"JobCompleted_Task(): Blit completion\n"));
            semaphore_signal( JobCompletionTimeout );
        }
    }
}

#endif                                  /* ST_OSLINUX */


#ifdef ST_OSLINUX
void layer_init( void )
{
    STLAYER_InitParams_t    STLAYER_InitParams;
    STLAYER_LayerParams_t   STLAYER_LayerParams;
    ST_ErrorCode_t          Err;

    STLAYER_LayerParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
    STLAYER_LayerParams.ScanType    = STGXOBJ_INTERLACED_SCAN;
    /* init vtg before layer! */
    STLAYER_LayerParams.Width       = 720;
    STLAYER_LayerParams.Height      = 480;

    STLAYER_InitParams.DeviceBaseAddress_p         = (void *)0;
    STLAYER_InitParams.SharedMemoryBaseAddress_p   = (void *)0;

    STLAYER_InitParams.LayerParams_p               = &STLAYER_LayerParams;
    STLAYER_InitParams.CPUPartition_p              = DriverPartition_p;
    STLAYER_InitParams.CPUBigEndian                = false;
    STLAYER_InitParams.AVMEM_Partition             = (STAVMEM_PartitionHandle_t)0;
    STLAYER_InitParams.NodeBufferUserAllocated     = FALSE;
    STLAYER_InitParams.ViewPortNodeBuffer_p        = (void *)NULL;
    STLAYER_InitParams.ViewPortBufferUserAllocated = FALSE;
    STLAYER_InitParams.ViewPortBuffer_p            = (void *)NULL;

    strcpy(STLAYER_InitParams.EventHandlerName, STEVT_DEVICE_NAME);

    STLAYER_InitParams.MaxHandles          = 1;
    STLAYER_InitParams.MaxViewPorts        = 1;
#ifdef ST_7100
    STLAYER_InitParams.LayerType           = STLAYER_HDDISPO2_VIDEO1;
    STLAYER_InitParams.BaseAddress_p       = (void *)ST7100_VID1_LAYER_BASE_ADDRESS;
    STLAYER_InitParams.BaseAddress2_p      = (void *)ST7100_DISP0_BASE_ADDRESS;
#endif
#ifdef ST_7109
    STLAYER_InitParams.LayerType           = STLAYER_DISPLAYPIPE_VIDEO1;
    STLAYER_InitParams.BaseAddress_p       = (void *)ST7109_VID1_LAYER_BASE_ADDRESS;
    STLAYER_InitParams.BaseAddress2_p      = (void *)ST7109_DISP0_BASE_ADDRESS;
#endif

    Err = STLAYER_Init("FOR_BLIT",&STLAYER_InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
    }
}

void layer_term( void )
{
    STLAYER_TermParams_t    STLAYER_TermParams;
    ST_ErrorCode_t          Err;

    STLAYER_TermParams.ForceTerminate = TRUE;
    Err =  STLAYER_Term( "FOR_BLIT", &STLAYER_TermParams ) ;
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"STLAYER_Term=%s\n", Err ));
    }
}
#endif


/*-----------------------------------------------------------------------------
 * Function :  SetSrcFileName
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL SetSrcFileName (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean SetSrcFileName (parse_t *pars_p, char *result_sym_p)
#endif

{
    #if defined(ST_OS21) || defined(ST_OSLINUX)
        BOOL ErrCode=FALSE;
    #endif
    #ifdef ST_OS20
        boolean ErrCode=FALSE;
    #endif
    S32 Lvar;

    ErrCode = STTST_GetInteger( pars_p, 1, &Lvar);
    if (ErrCode == TRUE)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Expected file format\n"));
    }


    switch ( Lvar )
    {
        case  TYPE_CLUT2 :
            #ifdef ST_OSLINUX
                strcpy(SrcFileName,"./blt/files/suzieCLUT2.gam");
            #else
                strcpy(SrcFileName,"../../suzieCLUT2.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Src file format is CLUT2"));
            break;
        case  TYPE_CLUT4 :
            #ifdef ST_OSLINUX
                strcpy(SrcFileName,"./blt/files/suzieCLUT4.gam");
            #else
                strcpy(SrcFileName,"../../suzieCLUT4.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Src file format is CLUT4"));
            break;
        case  TYPE_CLUT8 :
            #ifdef ST_OSLINUX
                strcpy(SrcFileName,"./blt/files/suzieCLUT8.gam");
            #else
                strcpy(SrcFileName,"../../suzieCLUT8.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Src file format is CLUT8"));
            break;
        case  TYPE_RGB565 :
            #ifdef ST_OSLINUX
                strcpy(SrcFileName,"./blt/files/suzieRGB565.gam");
            #else
                strcpy(SrcFileName,"../../suzieRGB565.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Src file format is RGB565"));
            break;
        case  TYPE_RGB888 :
            #ifdef ST_OSLINUX
                strcpy(SrcFileName,"./blt/files/suzieRGB888.gam");
            #else
                strcpy(SrcFileName,"../../suzieRGB888.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Src file format is RGB888"));
            break;
        case  TYPE_YCbCr422R :
            #ifdef ST_OSLINUX
                strcpy(SrcFileName,"./blt/files/suzieYCbCr422R.gam");
            #else
                strcpy(SrcFileName,"../../suzieYCbCr422R.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Src file format is YCbCr422R"));
            break;
        default:
            #ifdef ST_OSLINUX
                strcpy(SrcFileName,"./blt/files/suzieRGB565.gam");
            #else
                strcpy(SrcFileName,"../../suzieRGB565.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Src file format is RGB565"));
            break;
    }

    return ErrCode;
}

/*-----------------------------------------------------------------------------
 * Function :  SetDstFileName
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL SetDstFileName (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean SetDstFileName (parse_t *pars_p, char *result_sym_p)
#endif
{
    #if defined(ST_OS21) || defined(ST_OSLINUX)
        BOOL ErrCode=FALSE;
    #endif
    #ifdef ST_OS20
        boolean ErrCode=FALSE;
    #endif
    S32 Lvar;

    ErrCode = STTST_GetInteger( pars_p, 1, &Lvar);
    if (ErrCode == TRUE)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Expected file format\n"));
    }


    switch ( Lvar )
    {
        case  TYPE_CLUT2 :
            #ifdef ST_OSLINUX
                strcpy(DstFileName,"./blt/files/merouCLUT2.gam");
            #else
                strcpy(DstFileName,"../../merouCLUT2.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Dst file format is CLUT2"));
            break;
        case  TYPE_CLUT4 :
            #ifdef ST_OSLINUX
                strcpy(DstFileName,"./blt/files/merouCLUT4.gam");
            #else
                strcpy(DstFileName,"../../merouCLUT4.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Dst file format is CLUT4"));
            break;
        case  TYPE_CLUT8 :
            #ifdef ST_OSLINUX
                strcpy(DstFileName,"./blt/files/merouCLUT8.gam");
            #else
                strcpy(DstFileName,"../../merouCLUT8.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Dst file format is CLUT8"));
            break;
        case  TYPE_RGB565 :
            #ifdef ST_OSLINUX
                strcpy(DstFileName,"./blt/files/merouRGB565.gam");
            #else
                strcpy(DstFileName,"../../merouRGB565.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Dst file format is RGB565"));
            break;
        case  TYPE_RGB888 :
            #ifdef ST_OSLINUX
                strcpy(DstFileName,"./blt/files/merouRGB888.gam");
            #else
                strcpy(DstFileName,"../../merouRGB888.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Dst file format is RGB888"));
            break;
        case  TYPE_YCbCr422R :
            #ifdef ST_OSLINUX
                strcpy(DstFileName,"./blt/files/merouYCbCr422R.gam");
            #else
                strcpy(DstFileName,"../../merouYCbCr422R.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Dst file format is YCbCr422R"));
            break;
        default:
            #ifdef ST_OSLINUX
                strcpy(DstFileName,"./blt/files/merouRGB565.gam");
            #else
                strcpy(DstFileName,"../../merouRGB565.gam");
            #endif
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Dst file format is RGB565"));
            break;
    }

    return ErrCode;
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_JobTest; this test aims to operate a list of job
 * some of them will be submit back (job 1 2 & 3) some other will be submit
 * front (job 4 & 5). The Job 4 & 5 must stop the others to be run firstly.
 * The result will be save in the result.gam file
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#ifndef ST_OSLINUX
#ifdef ST_OS21
	BOOL BLIT_JobTest (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_JobTest (parse_t *pars_p, char *result_sym_p)
#endif
{
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;
    STEVT_SubscriberID_t    SubscriberID;
    ST_ErrorCode_t          ErrCode;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     Open_Params;
    STBLIT_TermParams_t     Term_Params;
    STBLIT_Handle_t         Hdl;
    STBLIT_Source_t         Src1, Src2;
    STBLIT_Destination_t    Dst;
    STBLIT_BlitContext_t    Context;
    STGXOBJ_Bitmap_t        Bmp1, Bmp2, Bmp3;
    STGXOBJ_Palette_t       Pal;

    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_JobParams_t      Job_Params;
    STBLIT_JobHandle_t      Job1, Job2, Job3, Job4;
#ifndef STBLIT_EMULATOR
    int                     i;
    BOOL                    TimeOut = FALSE;
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t           time;
    #endif
    #ifdef ST_OS20
		clock_t             time;
    #endif
#endif


    /* ------------ Blit Device Init ------------ */

    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 6;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);


#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    ErrCode = STBLIT_Open(Name,&Open_Params,&Hdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Process Blit operation ------------ */

    /* ----- Set Job Params ----*/
    Job_Params.NotifyJobCompletion = TRUE;

    /* Set Context */
    Context.ColorKeyCopyMode            = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                     = STBLIT_ALU_COPY;
    Context.EnableMaskWord              = FALSE;
    Context.EnableMaskBitmap            = FALSE;
    Context.EnableColorCorrection       = FALSE;
    Context.Trigger.EnableTrigger       = FALSE;
    Context.GlobalAlpha                 = 128;
    Context.EnableClipRectangle         = FALSE;
    Context.WriteInsideClipRectangle    = TRUE;
    Context.EnableFlickerFilter         = FALSE;
    Context.UserTag_p                   = NULL;
    Context.NotifyBlitCompletion        = FALSE;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		JobCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&JobCompletionTimeout, 0);
    #endif
    /* Open Event handler */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",ErrCode));
        return (TRUE);
    }


    EvtSubscribeParams.NotifyCallback   = JobCompletedHandler;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, Name,STBLIT_JOB_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Job completion : %d\n",ErrCode));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
    Job_Params.EventSubscriberID = SubscriberID;

    /* Set Bitmap */

    Bmp1.Data1_p=NULL;
    Bmp2.Data1_p=NULL;
    Bmp3.Data1_p=NULL;
    Pal.Data_p=NULL;
    ErrCode = ConvertGammaToBitmap("../../Red.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("../../Blue.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("../../green.gam",0, &Bmp3, &Pal);

    /* ---- Set Job 1 --------*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 1: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job1;

    /* Set Src1 */
    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src1.Data.Bitmap_p        = &Bmp1;
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 0;
    Src1.Rectangle.Width      = 10;
    Src1.Rectangle.Height     = 100;
    Src1.Palette_p            = NULL;

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Bmp2;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Rectangle.Width      = 10;
    Src2.Rectangle.Height     = 100;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &Bmp1;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = 10;
    Dst.Rectangle.Height      = 100;
    Dst.Palette_p             = NULL;

    /* first Blit of job 1*/
    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    /* Set Src */

    Src1.Rectangle.PositionX  = 15;
    Src1.Rectangle.PositionY  = 0;

    Src2.Rectangle.PositionX  = 15;
    Src2.Rectangle.PositionY  = 0;

    Dst.Rectangle.PositionX  = 15;
    Dst.Rectangle.PositionY  = 0;

    /* second Blit of job 1*/
    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    /*set Job2*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job2);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 2: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job2;

    /* Set Src */

    Src1.Rectangle.PositionX  = 30;
    Src1.Rectangle.PositionY  = 0;

    Src2.Rectangle.PositionX  = 30;
    Src2.Rectangle.PositionY  = 0;

    Dst.Rectangle.PositionX  = 30;
    Dst.Rectangle.PositionY  = 0;

    /* first Blit of job 2*/
    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    /* Set Src */

    Src1.Rectangle.PositionX  = 45;
    Src1.Rectangle.PositionY  = 0;

    Src2.Rectangle.PositionX  = 45;
    Src2.Rectangle.PositionY  = 0;

    Dst.Rectangle.PositionX  = 45;
    Dst.Rectangle.PositionY  = 0;

    /* second Blit of job 2*/
    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    /*set Job3*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job3);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 3: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job3;

    /* Set Src */

    Src1.Rectangle.PositionX  = 60;
    Src1.Rectangle.PositionY  = 0;

    Src2.Rectangle.PositionX  = 60;
    Src2.Rectangle.PositionY  = 0;

    Dst.Rectangle.PositionX  = 60;
    Dst.Rectangle.PositionY  = 0;

    /* first Blit of job 3*/
    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    /* Set Src */

    Src1.Rectangle.PositionX  = 75;
    Src1.Rectangle.PositionY  = 0;

    Src2.Rectangle.PositionX  = 75;
    Src2.Rectangle.PositionY  = 0;

    Dst.Rectangle.PositionX  = 75;
    Dst.Rectangle.PositionY  = 0;

    /* second Blit of job 3*/
    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    /*set Job4*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job4);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 4: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job4;

    /* Set Src1 */
    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src1.Data.Bitmap_p        = &Bmp1;
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 40;
    Src1.Rectangle.Width      = 100;
    Src1.Rectangle.Height     = 10;
    Src1.Palette_p            = NULL;

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Bmp3;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 40;
    Src2.Rectangle.Width      = 100;
    Src2.Rectangle.Height     = 10;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &Bmp1;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 40;
    Dst.Rectangle.Width       = 100;
    Dst.Rectangle.Height      = 10;
    Dst.Palette_p             = NULL;

    /* first Blit of job 4*/
    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    /* Set Src */

    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 50;

    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 50;

    Dst.Rectangle.PositionX  = 0;
    Dst.Rectangle.PositionY  = 50;

    /* second Blit of job 4*/
    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );



    /* Submit Job */

    /* get the flag for Lock/Unlock test */
    /*RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected flag (0):without lock/unlock tests" );
        tag_current_line( pars_p, "              (1):with    lock/unlock tests" );
    }
    switch ( Lvar)
    {
        case 0 :
            tag_current_line( pars_p, "Lock/Unlock test disabled" );
            break;
        case 1 :
            tag_current_line( pars_p, "Lock/Unlock test enabled" );
            STBLIT_Lock(Hdl, Job1);
            break;
        default:
            tag_current_line( pars_p, "Lock/Unlock test disabled" );
            break;
    }
       */
    ErrCode = STBLIT_SubmitJobBack(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job 1: %d\n",ErrCode));
        return (TRUE);
    }
    ErrCode = STBLIT_SubmitJobBack(Hdl, Job2);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job 2: %d\n",ErrCode));
        return (TRUE);
    }
    ErrCode = STBLIT_SubmitJobBack(Hdl, Job3);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job 3: %d\n",ErrCode));
        return (TRUE);
    }
    ErrCode = STBLIT_SubmitJobFront(Hdl, Job4);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job 4: %d\n",ErrCode));
        return (TRUE);
    }
    /*if ( Lvar==1 )
    {
        tag_current_line( pars_p, "Lock/Unlock test enabled" );
        STBLIT_Unlock(Hdl, STBLIT_NO_JOB_HANDLE);
    } */


	#ifndef STBLIT_EMULATOR
		time = time_plus(time_now(), 15625*5);

		for (i=0;i<4 ;i++ )
		{
            #if defined(ST_OS21) || defined(ST_OSLINUX)
				/* Wait for Job blit to be completed */
				if(semaphore_wait_timeout(JobCompletionTimeout,&time)!=0)
				{
					STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
					ResetEngine();
					TimeOut = TRUE;
					break;
				}
            #endif
            #ifdef ST_OS20
				/* Wait for Job blit to be completed */
				if(semaphore_wait_timeout(&JobCompletionTimeout,&time)!=0)
				{
					STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
					ResetEngine();
					TimeOut = TRUE;
					break;
				}
            #endif
			else
			{
				STTBX_Report((STTBX_REPORT_LEVEL_INFO, "JOB DONE"));
			}
		}

		/* Generate Bitmap */
		if (TimeOut == FALSE)
		{
			ErrCode = ConvertBitmapToGamma("../../../result/test_job.gam", &Bmp1, &Pal);
			if (ErrCode != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
				return (TRUE);
			}
		}
	#else
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));
			semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));
			semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));
			semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));
			semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));
			semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));
			semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));
        #endif
		/* Generate Bitmap */
		ErrCode = ConvertBitmapToGamma("../../../result/test_job.gam", &Bmp1, &Pal);
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
			return (TRUE);
		}
	#endif


    /* Delete Job */
    ErrCode = STBLIT_DeleteJob(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 1: %d\n",ErrCode));
        return (TRUE);
    }
    ErrCode = STBLIT_DeleteJob(Hdl, Job2);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 2: %d\n",ErrCode));
        return (TRUE);
    }
    ErrCode = STBLIT_DeleteJob(Hdl, Job3);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 3: %d\n",ErrCode));
        return (TRUE);
    }
    ErrCode = STBLIT_DeleteJob(Hdl, Job4);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 4: %d\n",ErrCode));
        return (TRUE);
    }

    GUTIL_Free(Bmp1.Data1_p);
    GUTIL_Free(Bmp2.Data1_p);
    GUTIL_Free(Bmp3.Data1_p);

    /* --------------- Close Evt ----------*/
    ErrCode = STEVT_Close(EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Blit Term ------------ */
    Term_Params.ForceTerminate = TRUE;
    ErrCode = STBLIT_Term(Name,&Term_Params);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",ErrCode));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}
#endif /*ST_OSLINUX*/






/*-----------------------------------------------------------------------------
 * Function : BLIT_TestResize;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestResize (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestResize (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif
    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;

	#ifndef STBLIT_EMULATOR
        osclock_t       time;
	#endif

    U32                     i;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/
    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)

    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif


    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif

#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        /*STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));*/
        /*return (TRUE);*/
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 120;
    Src.Rectangle.PositionY  = 50;
    Src.Rectangle.Width      = 100;
    Src.Rectangle.Height     = 100;
    Src.Palette_p            = &Palette;

    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(DstFileName,SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(DstFileName,DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Width       = TargetBitmap.Width-40;
    Dst.Rectangle.Height      = Src.Rectangle.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
/*    Context.WriteInsideClipRectangle = TRUE;*/
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start horizontal expansion \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_2.3_1_resize_H_expansion.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_2.3_1_resize_H_expansion.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_2.3_1_resize_H_expansion.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_2.3_1_resize_H_expansion.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

#ifdef ST_OSLINUX
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 120;
    Src.Rectangle.PositionY  = 50;
    Src.Rectangle.Width      = 100;
    Src.Rectangle.Height     = 100;
    Src.Palette_p            = &Palette;
#endif


    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(DstFileName,SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(DstFileName,DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Width       = 20;
    Dst.Rectangle.Height      = Src.Rectangle.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
/*    Context.WriteInsideClipRectangle = TRUE;*/
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start horizontal compression \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
            else
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
            else
        #endif
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_2.3_2_resize_H_compression.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_2.3_2_resize_H_compression.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_2.3_2_resize_H_compression.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_2.3_2_resize_H_compression.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

#ifdef ST_OSLINUX
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 120;
    Src.Rectangle.PositionY  = 50;
    Src.Rectangle.Width      = 100;
    Src.Rectangle.Height     = 100;
    Src.Palette_p            = &Palette;

#endif


    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(DstFileName,SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(DstFileName,DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Width       = Src.Rectangle.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height-40;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
/*    Context.WriteInsideClipRectangle = TRUE;*/
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start vertical expansion \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_2.3_3_resize_V_expansion.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_2.3_3_resize_V_expansion.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_2.3_3_resize_V_expansion.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_2.3_3_resize_V_expansion.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

#ifdef ST_OSLINUX
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 120;
    Src.Rectangle.PositionY  = 50;
    Src.Rectangle.Width      = 100;
    Src.Rectangle.Height     = 100;
    Src.Palette_p            = &Palette;

#endif

    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(DstFileName,SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(DstFileName,DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Width       = Src.Rectangle.Width;
    Dst.Rectangle.Height      = Src.Rectangle.Height/2;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
/*    Context.WriteInsideClipRectangle = TRUE;*/
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start vertical compression \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_2.3_4_resize_V_compression.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_2.3_4_resize_V_compression.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_2.3_4_resize_V_compression.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_2.3_4_resize_V_compression.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif



    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask,
            &(BlitCompletedTask_Attribute) );

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestFill;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
 BOOL BLIT_TestFill (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
 boolean BLIT_TestFill (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        TargetBitmap;
    STGXOBJ_Rectangle_t     Rectangle;
    STGXOBJ_Palette_t       Palette;
    STGXOBJ_Color_t         Color;
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;
    STEVT_SubscriberID_t    SubscriberID;
#endif
#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif
#ifdef CALC_TIME
    clock_t                 t1, t2 ;
#endif
    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init TestFill : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
	    BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }
    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }
    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);

    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /*-----------------2/26/01 9:04AM-------------------
     * Fill
     * --------------------------------------------------*/

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 20 ;
    Rectangle.PositionY = 20 ;
    Rectangle.Width     = 180 ;
    Rectangle.Height    = 180 ;

    /* ------------ Set Color ------------ */
    Color.Type                 = STGXOBJ_COLOR_TYPE_RGB565 ;

    Color.Value.RGB565.R     = 0xff ;
    Color.Value.RGB565.G     = 0x00 ;
    Color.Value.RGB565.B     = 0x00 ;

    /* ------------ Set Dest ------------ */
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = TRUE;
    Context.ClipRectangle           = Rectangle;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */


    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Fill rectangle \n"));

#ifdef CALC_TIME
    {
        clock_t     Tdiff, Tav, Ttot ;
        int         Count;
        Ttot = 0 ;
        for ( Count = 1 ; Count <= 20 ; Count++ )
        {
            Rectangle.PositionX = 20 + Count % 50 ;
            Rectangle.PositionY = 20 + Count % 50 ;

            t1 = get_time_now();
            Err = STBLIT_FillRectangle(Handle,&TargetBitmap,&Rectangle,&Color,&Context );
            t2 = get_time_now();
            Tdiff = t2 - t1 ;
            printf("-------> Time diff=%d \n", Tdiff );
            if ( Tdiff <  10000 )
            {
                Ttot += Tdiff ;
            }

        }
        Tav = Ttot / Count-1 ;
        printf("-------> Time avarage=%d \n", Tav );
    }
#endif

    Err = STBLIT_FillRectangle(Handle,&TargetBitmap,&Rectangle,&Color,&Context );

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifndef ST_OSLINUX
            time = time_plus(time_now(), 15625*5);
        #endif

        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
                ResetEngine();
            }
        #endif
        #if defined(ST_OS21)
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT\n"));
                ResetEngine();
            }
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
            }
        #endif
        else
		{
	        /* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_2.4_Fill.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_2.4_Fill.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_2.4_Fill.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_2.4_Fill.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
	        return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(TargetBitmap.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif
    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }


#ifdef ST_OSLINUX
    layer_term();
#endif


    return(FALSE);
}


/*-----------------------------------------------------------------------------
 * Function : BLIT_*TestCopy;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestCopy (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestCopy (parse_t *pars_p, char *result_sym_p)
#endif
{


    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Rectangle_t     Rectangle;
    STGXOBJ_Palette_t       Palette;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif

#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                       = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188)|| defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else

    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
    Context.UserTag_p   = &UserTagString;
    /* ------------ Set Security Params ------------ */
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 40 ;
    Rectangle.PositionY = 30 ;
    Rectangle.Width     = 200 ;
    Rectangle.Height    = 200 ;

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(DstFileName,SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(DstFileName,DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
    /*Context.EnableClipRectangle     = TRUE;*/
    Context.ClipRectangle           = Rectangle;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&TargetBitmap,20,20,&Context );
     if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }



	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
                time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
                time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)


            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
            /* Generate Bitmap */
            #ifdef ST_OSLINUX

                Err = ConvertBitmapToGamma("./blt/result/test_2.5_Copy.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_2.5_Copy.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_2.5_Copy.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_2.5_Copy.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestExtractFields;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestExtractFields (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestExtractFields (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Rectangle_t     Rectangle;
    STGXOBJ_Palette_t       Palette;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif

#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif

    STAVMEM_BlockHandle_t       BlockHandle1;
    STAVMEM_BlockHandle_t       BlockHandle2;
    STGXOBJ_BitmapAllocParams_t Params1;
    STGXOBJ_BitmapAllocParams_t Params2;
    FILE*                       fstream;        /* File handle for read operation          */
    U32                         size;


    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

    fstream = NULL;        /* File handle for read operation          */
    size = 0;

#ifdef ST_OSLINUX
    layer_init();
#endif

    /* ------------ Blit Init ------------ */
    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif


    Context.UserTag_p   = &UserTagString;


    /*=================================================
                    Extracting Top Field
      =================================================*/
    STTBX_Print(("--------- Extracting Top Field ---------\n"));

    /* ------------ Load Src ------------ */
    SourceBitmap.Data1_p = NULL;

    Err = ConvertGammaToBitmap(TopBottomSourceFileName,0,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set Top Field Height ------------ */
    SourceBitmap.Height = ( SourceBitmap.Height + 1 ) / 2;

    /* ------------ Alloc Top Field ------------*/
    TargetBitmap = SourceBitmap;

    Err = STBLIT_GetBitmapAllocParams(Handle,&SourceBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    AllocBitmapBuffer (&TargetBitmap,FALSE,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    AllocBitmapBuffer (&TargetBitmap, (void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[0],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    /* Set Field mabagment params */
    SourceBitmap.Pitch  = SourceBitmap.Pitch * 2;

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 0 ;
    Rectangle.PositionY = 0 ;
    Rectangle.Width     = SourceBitmap.Width ;
    Rectangle.Height    = SourceBitmap.Height ;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
    /*Context.EnableClipRectangle     = TRUE;*/
    Context.ClipRectangle           = Rectangle;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Copy ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&TargetBitmap,0,0,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Wait For Evt ------------ */
#ifndef STBLIT_EMULATOR
    time = time_plus(time_now(), 15625*5);
#if defined(ST_OSLINUX)
    if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
    }
#endif
#ifdef ST_OS21
    if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
        ResetEngine();
    }
#endif
#ifdef ST_OS20
    if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
        ResetEngine();

    }
#endif
    else
    {
        /* Open stream */
        fstream = fopen(TopFieldFileName, "wb");

        /* Generate BinFile */
        STTBX_Print(("Saving top data ...\n"));
#ifdef  ST_OSLINUX
        size = fwrite((void *)TargetBitmap.Data1_p, 1,(TargetBitmap.Size1), fstream);
#else
        size = fwrite((void *)STAVMEM_VirtualToCPU((void *)TargetBitmap.Data1_p,&VirtualMapping), 1,(TargetBitmap.Size1), fstream);
#endif
        if (size != (TargetBitmap.Size1))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Write Bitmap error %d byte instead of %d\n",size,(TargetBitmap.Size1)));
            Err = ST_ERROR_BAD_PARAMETER;
        }

        /* Close stream */
        fclose (fstream);
    }
#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        Err = ConvertBitmapToGamma(TopFieldFileName,&TargetBitmap,&Palette);
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
#endif


    /*=================================================
                    Extracting Bottom Field
      =================================================*/

    STTBX_Print(("--------- Extracting Bottom Field ---------\n"));

    /* ------------ Load Src ------------ */
    SourceBitmap.Data1_p = NULL;

    Err = ConvertGammaToBitmap(TopBottomSourceFileName,0,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set Bottom Field Height ------------ */
    SourceBitmap.Height = SourceBitmap.Height / 2;

    /* ------------ Alloc Bottom Field ------------*/
    TargetBitmap = SourceBitmap;

    Err = STBLIT_GetBitmapAllocParams(Handle,&SourceBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    AllocBitmapBuffer (&TargetBitmap,FALSE,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    AllocBitmapBuffer (&TargetBitmap, (void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[0],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    /* Set Field mabagment params */
    SourceBitmap.Data1_p  = (void*)( (U32)SourceBitmap.Data1_p + SourceBitmap.Pitch );
    SourceBitmap.Pitch  = SourceBitmap.Pitch * 2;

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 0 ;
    Rectangle.PositionY = 0 ;
    Rectangle.Width     = SourceBitmap.Width;
    Rectangle.Height    = SourceBitmap.Height;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
    /*Context.EnableClipRectangle     = TRUE;*/
    Context.ClipRectangle           = Rectangle;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Copy ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&TargetBitmap,0,0,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Wait For Evt ------------ */
#ifndef STBLIT_EMULATOR
    time = time_plus(time_now(), 15625*5);
#if defined(ST_OSLINUX)
    if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
    }
#endif
#ifdef ST_OS21
    if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
        ResetEngine();
    }
#endif
#ifdef ST_OS20
    if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
        ResetEngine();

    }
#endif
    else
    {
        /* Open stream */
        fstream = fopen(BottomFieldFileName, "wb");

        /* Generate BinFile */
        STTBX_Print(("Saving Bottom data ...\n"));
#ifdef ST_OSLINUX
        size = fwrite((void *)TargetBitmap.Data1_p, 1,(TargetBitmap.Size1), fstream);
#else
        size = fwrite((void *)STAVMEM_VirtualToCPU((void *)TargetBitmap.Data1_p,&VirtualMapping), 1,(TargetBitmap.Size1), fstream);
#endif
        if (size != (TargetBitmap.Size1))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Write Bitmap error %d byte instead of %d\n",size,(TargetBitmap.Size1)));
            Err = ST_ERROR_BAD_PARAMETER;
        }

        /* Close stream */
        fclose (fstream);
    }
#else
        /*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        Err = ConvertBitmapToGamma(BottomFieldFileName,&TargetBitmap,&Palette);
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
#endif


    /* --------------- Close Evt ----------*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}


/*-----------------------------------------------------------------------------
 * Function  : BLIT_TestTypeRasterTopBottom;
 * Input     : *pars_p, *result_sym_p
 * Output    :
 * Return    : TRUE if error, FALSE if success
 * Assuption : BLIT_TestExtractFields sould be called before this function
 *             at least one time
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestTypeRasterTopBottom (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestTypeRasterTopBottom (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif

	#ifndef STBLIT_EMULATOR
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			osclock_t       time;
        #endif
        #ifdef ST_OS20
			clock_t         time;
        #endif
	#endif

#ifndef ST_OSLINUX
    STAVMEM_BlockHandle_t  BlockHandle1;
    STAVMEM_BlockHandle_t  BlockHandle2;
#endif
    STGXOBJ_BitmapAllocParams_t Params1;
    STGXOBJ_BitmapAllocParams_t Params2;
    FILE*                       fstream;        /* File handle for read operation          */
    U32                         size;
    STGXOBJ_Rectangle_t     Rectangle;



    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_7109) || defined (ST_5188) || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)\
 || defined (ST_5188) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    /* ------------ Blit Init ------------ */
    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif


    /* ------------ Set source ------------ */

    /* Source Initialisations */
    SourceBitmap.ColorType            = TopBottomFieldColorType;
    SourceBitmap.BitmapType           = TopBottomFieldBitmapType;
    SourceBitmap.PreMultipliedColor   = TopBottomFieldPreMultipliedColor;
    SourceBitmap.ColorSpaceConversion = TopBottomFieldColorSpaceConversion;
    SourceBitmap.AspectRatio          = TopBottomFieldAspectRatio;
    SourceBitmap.Width                = TopBottomFieldWidth;
    SourceBitmap.Height               = TopBottomFieldHeight;
    SourceBitmap.Pitch                = ( TopBottomFieldWidth * NbBytePerPixel );
    SourceBitmap.Size1                = ( TopBottomFieldWidth * TopBottomFieldHeight * NbBytePerPixel ) / 2;
    SourceBitmap.Size2                = SourceBitmap.Size1;
    SourceBitmap.Data1_p              = NULL;
    SourceBitmap.Data2_p              = NULL;

    /* Set alloc params */
    Params1.AllocBlockParams.Size      = SourceBitmap.Size1;
    Params1.AllocBlockParams.Alignment = TopBottomFieldAllocAlignment;
    Params1.Pitch                      = SourceBitmap.Pitch;

    /* Alloc bottom Field */
#ifdef ST_OSLINUX
    AllocBitmapBuffer (&SourceBitmap,FALSE,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    AllocBitmapBuffer (&SourceBitmap, (void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[0],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    SourceBitmap.Data2_p=SourceBitmap.Data1_p;

    /* Alloc Top Field */
#ifdef ST_OSLINUX
    AllocBitmapBuffer (&SourceBitmap,FALSE,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    AllocBitmapBuffer (&SourceBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[0],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    /* Read Top Field from file */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Reading top Bitmap data"));
    fstream = fopen(TopFieldFileName, "rb");
    if( fstream == NULL )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to open \'%s\'\n", "c:/scripts/Chat_data1_p.bin" ));
        Err = ST_ERROR_BAD_PARAMETER;
    }

    size = fread ((void *)(SourceBitmap.Data1_p), 1,(SourceBitmap.Size1), fstream);
    if (size != (SourceBitmap.Size1))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,SourceBitmap.Size1));
    }
    fclose (fstream);

    /* Read Bottom Field from file */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Reading bottom Bitmap data"));
    fstream = fopen(BottomFieldFileName, "rb");
    if( fstream == NULL )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to open \'%s\'\n", "c:/scripts/Chat_data1_p.bin" ));
        Err = ST_ERROR_BAD_PARAMETER;
    }

    size = fread ((void *)(SourceBitmap.Data2_p), 1,(SourceBitmap.Size2), fstream);
    if (size != (SourceBitmap.Size2))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,SourceBitmap.Size1));
    }
    fclose (fstream);


    /*=================================
             TEST COPY FUNCTION
      =================================*/
    STTBX_Print(("--------- COPY TEST ---------\n"));

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 100;
    Rectangle.PositionY = 100 ;
    Rectangle.Width     = SourceBitmap.Width - 200 ;
    Rectangle.Height    = SourceBitmap.Height - 200 ;

    /* ------------ Set Dest ------------ */
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/BATEAU.GAM",0,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../BATEAU.GAM",0,&TargetBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
    Context.EnableMaskBitmap        = FALSE;
    Context.EnableColorCorrection   = FALSE;
    Context.Trigger.EnableTrigger   = FALSE;
    Context.EnableClipRectangle     = FALSE;
    Context.ClipRectangle           = Rectangle;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Copy ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle\n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&TargetBitmap,40,40,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

    #ifndef STBLIT_EMULATOR
		time = time_plus(time_now(), 15625*5);
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_TypeRasterTopBottom_Copy.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_TypeRasterTopBottom_Copy.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_TypeRasterTopBottom_Copy.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_TypeRasterTopBottom_Copy.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif


    /*=================================
             TEST BLIT FUNCTION
      =================================*/
    STTBX_Print(("--------- BLEND TEST ---------\n"));

    /* ------------ Set Dest ------------ */
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/BATEAU.GAM",0,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../BATEAU.GAM",0,&TargetBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 50;
    Dst.Rectangle.PositionY   = 50;
    Dst.Rectangle.Width       = TargetBitmap.Width - 100;
    Dst.Rectangle.Height      = TargetBitmap.Height - 100;
    Dst.Palette_p             = NULL;


    /* ------------ Set Src ------------ */
    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = Dst.Rectangle.Width / 4;
    Src.Rectangle.PositionY  = Dst.Rectangle.Height / 4;
    Src.Rectangle.Width      = Dst.Rectangle.Width / 2;
    Src.Rectangle.Height     = Dst.Rectangle.Height / 2;
    Src.Palette_p            = &Palette;


    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_ALPHA_BLEND;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 75;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Bitmap Blit\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        time = time_plus(time_now(), 15625*5);
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_TypeRasterTopBottom_Blit.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_TypeRasterTopBottom_Blit.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_TypeRasterTopBottom_Blit.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_TypeRasterTopBottom_Blit.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(TargetBitmap.Data1_p);

    /* --------------- Close Evt ----------*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestOverlapCopy;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestOverlapCopy (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestOverlapCopy (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Rectangle_t     Rectangle;
    STGXOBJ_Palette_t       Palette;


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188)|| defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710)  || defined (ST_5301) || defined (ST_7109)\
 || defined (ST_7100) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else

    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif

    Context.UserTag_p   = &UserTagString;
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /**********************************************************/
    /*  Case 1 of Overlap copy: source is up left destination */
    /**********************************************************/

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 50 ;
    Rectangle.PositionY = 40 ;
    Rectangle.Width     = 140;
    Rectangle.Height    = 120;

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

     /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
    /*Context.EnableClipRectangle     = TRUE;*/
    Context.ClipRectangle           = Rectangle;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle case 1 \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&SourceBitmap,80,90,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy1.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy1.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy1.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy1.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /************************************************************/
    /*  Case 2 of Overlap copy: source is up middle destination */
    /***********************************************************/

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 50 ;
    Rectangle.PositionY = 40 ;
    Rectangle.Width     = 140;
    Rectangle.Height    = 120;

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle case 2 \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&SourceBitmap,50,90,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy2.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy2.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy2.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy2.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /************************************************************/
    /*  Case 3 of Overlap copy: source is up right destination */
    /***********************************************************/

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 50 ;
    Rectangle.PositionY = 40 ;
    Rectangle.Width     = 140;
    Rectangle.Height    = 120;

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

 #ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
   if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle case 3 \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&SourceBitmap,20,90,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy3.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy3.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy3.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy3.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /*************************************************************/
    /*  Case 4 of Overlap copy: source is down right destination */
    /*************************************************************/

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 80 ;
    Rectangle.PositionY = 90 ;
    Rectangle.Width     = 140;
    Rectangle.Height    = 120;

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

 #ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
   if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle case 4 \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&SourceBitmap,50,40,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy4.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy4.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy4.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy4.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /**************************************************************/
    /*  Case 5 of Overlap copy: source is down middle destination */
    /**************************************************************/

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 50 ;
    Rectangle.PositionY = 90 ;
    Rectangle.Width     = 140;
    Rectangle.Height    = 120;

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle case 5 \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&SourceBitmap,50,40,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy5.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy5.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy5.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy5.gam",&SourceBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /*************************************************************/
    /*  Case 6 of Overlap copy: source is down left destination  */
    /*************************************************************/

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 20 ;
    Rectangle.PositionY = 90 ;
    Rectangle.Width     = 140;
    Rectangle.Height    = 120;

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

 #ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
   if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle case 6 \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&SourceBitmap,50,40,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy6.gam",&SourceBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy6.gam",&SourceBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy6.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy6.gam",&SourceBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /**************************************************************/
    /*  Case 7 of Overlap copy: source is middle left destination */
    /**************************************************************/

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 20 ;
    Rectangle.PositionY = 40 ;
    Rectangle.Width     = 140;
    Rectangle.Height    = 120;

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

 #ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
   if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle case 7 \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&SourceBitmap,50,40,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy7.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy7.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy7.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy7.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /***************************************************************/
    /*  Case 8 of Overlap copy: source is middle right destination */
    /***************************************************************/

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 90 ;
    Rectangle.PositionY = 40 ;
    Rectangle.Width     = 140;
    Rectangle.Height    = 120;

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
  if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy rectangle case 8 \n"));
    Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&SourceBitmap,50,40,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy8.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy8.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapCopy8.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapCopy8.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);

    /* --------------- Close Evt ----------*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestOverlapBlit;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestOverlapBlit (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestOverlapBlit (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STBLIT_Destination_t    Dst;
    STBLIT_Source_t         Src2;

    STGXOBJ_Palette_t       Palette;


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
        osclock_t                 time;
    #endif
    #ifdef ST_OS20
        clock_t                 time;
    #endif
#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188) || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710)  || defined (ST_5301) || defined (ST_7109)\
 || defined (ST_7100) || defined (ST_5188) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif


    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
        BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
        semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else

    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif

    Context.UserTag_p   = &UserTagString;
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /**********************************************************/
    /*  Case 1 of Overlap Blit: source is up left destination */
    /**********************************************************/

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

 #ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
   if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }









     /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
    /*Context.EnableClipRectangle     = TRUE;*/
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */
    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &SourceBitmap ;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Rectangle.Width      = 100;
    Src2.Rectangle.Height     = 100;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &SourceBitmap;


    Dst.Rectangle.PositionX  = 80;
    Dst.Rectangle.PositionY  = 90;
    Dst.Rectangle.Width      = 150;
    Dst.Rectangle.Height     = 100;
    Dst.Palette_p            = NULL;
    Dst.Palette_p            = NULL;

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Blit operation case 1 \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit1.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit1.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit1.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit1.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /************************************************************/
    /*  Case 2 of Overlap Blit: source is up middle destination */
    /***********************************************************/

    /* ------------ Set Rectangle ------------ */
    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }









    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &SourceBitmap ;
    Src2.Rectangle.PositionX  = 50;
    Src2.Rectangle.PositionY  = 40;
    Src2.Rectangle.Width      = 100;
    Src2.Rectangle.Height     = 50;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &SourceBitmap;
    Dst.Rectangle.PositionX  = 50;
    Dst.Rectangle.PositionY  = 90;
    Dst.Rectangle.Width      = 100;
    Dst.Rectangle.Height     = 120;
    Dst.Palette_p            = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Blit operation case 2 \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }


	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit2.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit2.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit2.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit2.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif
    /************************************************************/
    /*  Case 3 of Overlap Blit: source is up right destination */
    /***********************************************************/

    /* ------------ Set Rectangle ------------ */
    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Err = ConvertGammaToBitmap(DstFileName,0,&TargetBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }









    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &SourceBitmap ;
    Src2.Rectangle.PositionX  = 50;
    Src2.Rectangle.PositionY  = 40;
    Src2.Rectangle.Width      = 100;
    Src2.Rectangle.Height     = 120;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &SourceBitmap;
    Dst.Rectangle.PositionX  = 20;
    Dst.Rectangle.PositionY  = 90;
    Dst.Rectangle.Width      = 80;
    Dst.Rectangle.Height     = 120;
    Dst.Palette_p            = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Blit operation case 3 \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit3.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit3.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit3.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit3.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /*************************************************************/
    /*  Case 4 of Overlap Blit: source is down right destination */
    /*************************************************************/

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }









    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &SourceBitmap ;
    Src2.Rectangle.PositionX  = 80;
    Src2.Rectangle.PositionY  = 90;
    Src2.Rectangle.Width      = 100;
    Src2.Rectangle.Height     = 120;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &SourceBitmap;
    Dst.Rectangle.PositionX  = 50;
    Dst.Rectangle.PositionY  = 40;
    Dst.Rectangle.Width      = 120;
    Dst.Rectangle.Height     = 120;
    Dst.Palette_p            = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Blit operation case 4 \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #ifdef ST_OSLINUX
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #ifdef ST_OS21
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit4.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit4.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit4.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit4.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif
    /**************************************************************/
    /*  Case 5 of Overlap Blit: source is down middle destination */
    /**************************************************************/
    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }









    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &SourceBitmap ;
    Src2.Rectangle.PositionX  = 50;
    Src2.Rectangle.PositionY  = 90;
    Src2.Rectangle.Width      = 100;
    Src2.Rectangle.Height     = 100;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &SourceBitmap;
    Dst.Rectangle.PositionX  = 50;
    Dst.Rectangle.PositionY  = 40;
    Dst.Rectangle.Width      = 100;
    Dst.Rectangle.Height     = 140;
    Dst.Palette_p            = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Blit operation case 5 \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit5.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit5.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit5.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit5.gam",&SourceBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif


    /*************************************************************/
    /*  Case 6 of Overlap Blit: source is down left destination  */
    /*************************************************************/

    /* ------------ Set Rectangle ------------ */
    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }









    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &SourceBitmap ;
    Src2.Rectangle.PositionX  = 20;
    Src2.Rectangle.PositionY  = 90;
    Src2.Rectangle.Width      = 100;
    Src2.Rectangle.Height     = 80;
    Src2.Palette_p            = NULL;
    /* Set Dst */
    Dst.Bitmap_p              = &SourceBitmap;
    Dst.Rectangle.PositionX  = 50;
    Dst.Rectangle.PositionY  = 40;
    Dst.Rectangle.Width      = 200;
    Dst.Rectangle.Height     = 120;
    Dst.Palette_p            = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Blit operation case 6 \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
    #ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit6.gam",&SourceBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit6.gam",&SourceBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit6.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit6.gam",&SourceBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /**************************************************************/
    /*  Case 7 of Overlap Blit: source is middle left destination */
    /**************************************************************/


    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &SourceBitmap ;
    Src2.Rectangle.PositionX  = 20;
    Src2.Rectangle.PositionY  = 40;
    Src2.Rectangle.Width      = 100;
    Src2.Rectangle.Height     = 120;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &SourceBitmap;
    Dst.Rectangle.PositionX  = 80;
    Dst.Rectangle.PositionY  = 40;
    Dst.Rectangle.Width      = 130;
    Dst.Rectangle.Height     = 120;
    Dst.Palette_p            = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Blit operation case 7 \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
    #ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit7.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit7.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit7.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit7.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /***************************************************************/
    /*  Case 8 of Overlap Blit: source is middle right destination */
    /***************************************************************/

    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &SourceBitmap ;
    Src2.Rectangle.PositionX  = 90;
    Src2.Rectangle.PositionY  = 40;
    Src2.Rectangle.Width      = 100;
    Src2.Rectangle.Height     = 120;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &SourceBitmap;
    Dst.Rectangle.PositionX  = 50;
    Dst.Rectangle.PositionY  = 90;
    Dst.Rectangle.Width      = 120;
    Dst.Rectangle.Height     = 120;
    Dst.Palette_p            = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Blit operation case 8 \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();

            }
        #endif
        else
        {
			/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit8.gam",&SourceBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit8.gam",&SourceBitmap,&Palette);
        #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
        }
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_OverlapBlit8.gam",&SourceBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_OverlapBlit8.gam",&SourceBitmap,&Palette);
    #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif


    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);


    /* --------------- Close Evt ----------*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*----------------------------------------------------------------
 * Function : BLIT_TestDraw;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestDraw (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestDraw (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

#ifdef ST_OSLINUX
    STGXOBJ_Bitmap_t        SourceBitmap;
#endif
    STGXOBJ_Bitmap_t        TargetBitmap;
    STGXOBJ_Palette_t       Palette;
    STGXOBJ_Color_t         Color;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif

#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t           time;
    #endif
    #ifdef ST_OS20
		clock_t             time;
    #endif
#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;
    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188)|| defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif


    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /* ------------ Set Color ------------ */
    Color.Type                 = STGXOBJ_COLOR_TYPE_ARGB8888 ;
    Color.Value.ARGB8888.Alpha = 0x50 ;
    Color.Value.ARGB8888.R     = 0xff ;
    Color.Value.ARGB8888.G     = 0x00 ;
    Color.Value.ARGB8888.B     = 0x00 ;

#ifdef ST_OSLINUX
    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif


    /* ------------ Set Dest ------------ */
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
/*    Context.WriteInsideClipRectangle = TRUE;*/
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Draw \n"));
    Err = STBLIT_DrawHLine(Handle,&TargetBitmap,20,20,(TargetBitmap.Width-40),&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
/*            time = time_plus(time_now(), 15625*TEST_DELAY);*/
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
	#endif
    Err = STBLIT_DrawHLine(Handle,&TargetBitmap,20,(TargetBitmap.Height-20),(TargetBitmap.Width-40),&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
	#endif

    Err = STBLIT_DrawVLine(Handle,&TargetBitmap,20,20,(TargetBitmap.Height-40),&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
	#endif

    Err = STBLIT_DrawVLine(Handle,&TargetBitmap,(TargetBitmap.Width-20),20,(TargetBitmap.Height-40),&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_2.6.1_2.6.2_DrawVHline.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_2.6.1_2.6.2_DrawVHline.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_2.6.1_2.6.2_DrawVHline.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_2.6.1_2.6.2_DrawVHline.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(TargetBitmap.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestPix;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestPix (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestPix (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

#ifdef ST_OSLINUX
    STGXOBJ_Bitmap_t        SourceBitmap;
#endif

    STGXOBJ_Bitmap_t        TargetBitmap;
    STGXOBJ_Palette_t       Palette;
    STGXOBJ_Color_t       Color;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif

#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t           time;
    #endif
    #ifdef ST_OS20
		clock_t             time;
    #endif
#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;
    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)\
 || defined (ST_5301) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /* ------------ Set Color ------------ */
    Color.Type                 = STGXOBJ_COLOR_TYPE_ARGB8888 ;
    Color.Value.ARGB8888.Alpha = 0x50 ;
    Color.Value.ARGB8888.R     = 0xff ;
    Color.Value.ARGB8888.G     = 0x00 ;
    Color.Value.ARGB8888.B     = 0x00 ;

#ifdef ST_OSLINUX
    /* ------------ Set Src ------------ */
    SourceBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif


    /* ------------ Set Dest ------------ */
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
/*    Context.WriteInsideClipRectangle = TRUE;*/
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Set Pixel \n"));
    Err = STBLIT_SetPixel(Handle,&TargetBitmap,0,0,&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
	#endif
    Err = STBLIT_SetPixel(Handle,&TargetBitmap,0,5,&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
	#endif
    Err = STBLIT_SetPixel(Handle,&TargetBitmap,5,0,&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
		    semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
	#endif
    Err = STBLIT_SetPixel(Handle,&TargetBitmap,5,5,&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
		else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_2.6.3_setpix.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_2.6.3_setpix.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
		    semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_2.6.3_setpix.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_2.6.3_setpix.gam",&TargetBitmap,&Palette);
        #endif
	    if (Err != ST_NO_ERROR)
		{
	 	    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
	 	    return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(TargetBitmap.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestColorKey;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestColorKey (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestColorKey (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t           time;
    #endif
    #ifdef ST_OS20
		clock_t             time;
    #endif
#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined(ST_7109)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif


#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/suzie.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../green.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;

    TargetBitmap.Data1_p = NULL;
    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("./blt/files/merouARGB8888.gam",SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#endif
#else /* !ST_OSLINUX */
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif /* ST_OSLINUX */

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Palette_p             = NULL;

    if ( SourceBitmap.Width >= TargetBitmap.Width)
    {
        Src.Rectangle.Width = TargetBitmap.Width;
        Dst.Rectangle.Width = TargetBitmap.Width;
    }
    else
    {
        Dst.Rectangle.Width = SourceBitmap.Width;
    }
    if ( SourceBitmap.Height >= TargetBitmap.Height)
    {
        Src.Rectangle.Height = TargetBitmap.Height;
        Dst.Rectangle.Height = TargetBitmap.Height;
    }
    else
    {
        Dst.Rectangle.Height = SourceBitmap.Height;
    }


    /* ------------ Set Context ------------ */
    Context.ColorKey.Type           = STGXOBJ_COLOR_KEY_TYPE_RGB565;
    Context.ColorKey.Value.RGB888.RMin    = 0x0;
    Context.ColorKey.Value.RGB888.RMax    = 0x10;
    Context.ColorKey.Value.RGB888.ROut    = 1;
    Context.ColorKey.Value.RGB888.REnable = 1;
    Context.ColorKey.Value.RGB888.GMin    = 0x20;
    Context.ColorKey.Value.RGB888.GMax    = 0x30;
    Context.ColorKey.Value.RGB888.GOut    = 1;
    Context.ColorKey.Value.RGB888.GEnable = 1;
    Context.ColorKey.Value.RGB888.BMin    = 0x0;
    Context.ColorKey.Value.RGB888.BMax    = 0x10;
    Context.ColorKey.Value.RGB888.BOut    = 1;
    Context.ColorKey.Value.RGB888.BEnable = 1;
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
    Context.WriteInsideClipRectangle = FALSE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /*=======================================================================*/
    /* Test1 : Test ColorKey mode source                                     */
    /*=======================================================================*/

    /* ------------ Set color key mode ------------ */
     Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_SRC;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start ColorKey mode source\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifndef ST_OSLINUX
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();

			}
        #endif
		else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_ColorKey_source.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_ColorKey_source.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_ColorKey_source.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_ColorKey_source.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /*=======================================================================*/
    /* Test2 : Test ColorKey mode destination                                */
    /*=======================================================================*/

    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("./blt/files/merouARGB8888.gam",SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#endif
#else /* !ST_OSLINUX */
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif /* ST_OSLINUX */


    /* ------------ Set color key mode ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_DST;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start ColorKey mode destination\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifndef ST_OSLINUX
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();

			}
        #endif
		else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_ColorKey_destination.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_ColorKey_destination.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_ColorKey_destination.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_ColorKey_destination.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif



    /* --------------- Free Bitmap ------------ */

    GUTIL_Free(SourceBitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif
    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestALU;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestALU (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestALU (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src1,Src2;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        Source1Bitmap,Source2Bitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;

#ifndef ST_OSLINUX
    STAVMEM_BlockHandle_t  BlockHandle1;
    STAVMEM_BlockHandle_t  BlockHandle2;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
#endif
    STGXOBJ_BitmapAllocParams_t Params1;
    STGXOBJ_BitmapAllocParams_t Params2;


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined(ST_7109)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif

/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }



    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif


#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /* ------------ Set source 1 ------------ */
    Source1Bitmap.Data1_p = NULL;
    Source2Bitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
    TargetBitmap.Data1_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/Logical_zero.gam",SECURED_SRC_BITMAP,&Source1Bitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../Logical_zero.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&Source1Bitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src1.Data.Bitmap_p        = &Source1Bitmap;
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 0;
    Src1.Palette_p            = &Palette;

    /* ------------ Set src 2 ------------ */
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/Logical_one.gam",SECURED_SRC_BITMAP,&Source2Bitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../Logical_one.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&Source2Bitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Source2Bitmap;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Palette_p            = &Palette;

    if ( Source1Bitmap.Width >= Source2Bitmap.Width )
    {
        Src1.Rectangle.Width = Source2Bitmap.Width;
        Src2.Rectangle.Width = Source2Bitmap.Width;
    }
    else
    {
        Src1.Rectangle.Width = Source1Bitmap.Width;
        Src2.Rectangle.Width = Source1Bitmap.Width;
    }

    if ( Source1Bitmap.Height >= Source2Bitmap.Height )
    {
        Src1.Rectangle.Height = Source2Bitmap.Height;
        Src2.Rectangle.Height = Source2Bitmap.Height;
    }
    else
    {
        Src1.Rectangle.Height = Source1Bitmap.Height;
        Src2.Rectangle.Height = Source1Bitmap.Height;
    }

    /* ------------ Set Dst ------------ */

    TargetBitmap.ColorType              = Source1Bitmap.ColorType;
    TargetBitmap.BitmapType             = Source1Bitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = Src1.Rectangle.Width;
    TargetBitmap.Height                 = Src1.Rectangle.Height;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err TargetBitmap Allocation : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;
/*    Rectangle                 = Dst.Rectangle;*/



    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
/*    Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = /*TRUE*/FALSE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /*=======================================================================*/
    /* Test1 : Test ALU "AND"                                                */
    /*=======================================================================*/

    /* ------------ Set AluMode ------------ */
    Context.AluMode                 = STBLIT_ALU_AND;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Test ALU : S1(0) AND S2(1)\n"));
    Err = STBLIT_Blit(Handle,&Src1,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
            #ifdef ST_OSLINUX
                time = time_plus(time_now(), 15625*TEST_DELAY);
            #else
                time = time_plus(time_now(), 15625*5);
            #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
		else
		{
            /* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_ALU_S1_AND_S2.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_ALU_S1_AND_S2.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		 /* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_ALU_S1_AND_S2.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_ALU_S1_AND_S2.gam",&TargetBitmap,&Palette);
         #endif
		 if (Err != ST_NO_ERROR)
		 {
			 STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			 return (TRUE);
		 }
	#endif

    /*=======================================================================*/
    /* Test2: Test ALU "OR"                                                  */
    /*=======================================================================*/

    /* ------------ Set AluMode ------------ */
    Context.AluMode                 = STBLIT_ALU_OR;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Test ALU : S1(0) OR S2(1)\n"));
    Err = STBLIT_Blit(Handle,&Src1,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
            #ifdef ST_OSLINUX
                time = time_plus(time_now(), 15625*TEST_DELAY);
            #else
                time = time_plus(time_now(), 15625*5);
            #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
		else
		{
            /* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_ALU_S1_OR_S2.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_ALU_S1_OR_S2.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		 /* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_ALU_S1_OR_S2.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_ALU_S1_OR_S2.gam",&TargetBitmap,&Palette);
         #endif
		 if (Err != ST_NO_ERROR)
		 {
			 STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			 return (TRUE);
		 }
	#endif

    /*=======================================================================*/
    /* Test3: Test ALU "NAND"                                                */
    /*=======================================================================*/

    /* ------------ Set AluMode ------------ */
    Context.AluMode                 = STBLIT_ALU_NAND;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Test ALU : S1(0) NAND S2(1)\n"));
    Err = STBLIT_Blit(Handle,&Src1,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
            #ifdef ST_OSLINUX
                time = time_plus(time_now(), 15625*TEST_DELAY);
            #else
                time = time_plus(time_now(), 15625*5);
            #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
		else
		{
            /* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_ALU_S1_NAND_S2.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_ALU_S1_NAND_S2.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		 /* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_ALU_S1_NAND_S2.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_ALU_S1_NAND_S2.gam",&TargetBitmap,&Palette);
         #endif
		 if (Err != ST_NO_ERROR)
		 {
			 STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			 return (TRUE);
		 }
	#endif

    /*=======================================================================*/
    /* Test4: Test ALU "NOOP"                                                */
    /*=======================================================================*/

    /* ------------ Set AluMode ------------ */
    Context.AluMode                 = STBLIT_ALU_NOOP;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Test ALU : NOOP, Result = S1(0)\n"));
    Err = STBLIT_Blit(Handle,&Src1,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
            #ifdef ST_OSLINUX
                time = time_plus(time_now(), 15625*TEST_DELAY);
            #else
                time = time_plus(time_now(), 15625*5);
            #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
		else
		{
            /* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_ALU_NOOP.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_ALU_NOOP.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		 /* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_ALU_NOOP.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_ALU_NOOP.gam",&TargetBitmap,&Palette);
         #endif
		 if (Err != ST_NO_ERROR)
		 {
			 STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			 return (TRUE);
		 }
	#endif


    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(Source1Bitmap.Data1_p);
    GUTIL_Free(Source2Bitmap.Data1_p);



#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    /* Free TargetBitmap */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestMaskWord;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestMaskWord (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestMaskWord (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap,TargetBitmap1,TargetBitmap2,TargetBitmap3;
    STGXOBJ_Palette_t       Palette;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t           time;
    #endif
    #ifdef ST_OS20
		clock_t             time;
    #endif
#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined(ST_7109)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p  = NULL;
    TargetBitmap1.Data1_p = NULL;
    TargetBitmap2.Data1_p = NULL;
    TargetBitmap3.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/suzie.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../suzie.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;

    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;


    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_SRC_BITMAP,&TargetBitmap1,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap1,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap2,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap2,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap3,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap3,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap1;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap1.Width;
    Dst.Rectangle.Height      = TargetBitmap1.Height;
    Dst.Palette_p             = NULL;

    if ( TargetBitmap1.Width >= SourceBitmap.Width )
    {
        Src.Rectangle.Width = SourceBitmap.Width;
        Dst.Rectangle.Width = SourceBitmap.Width;
    }
    else
    {
        Src.Rectangle.Width = TargetBitmap1.Width;
        Dst.Rectangle.Width = TargetBitmap1.Width;
    }

    if ( TargetBitmap1.Height >= SourceBitmap.Height )
    {
        Src.Rectangle.Height = SourceBitmap.Height;
        Dst.Rectangle.Height = SourceBitmap.Height;
    }
    else
    {
        Src.Rectangle.Height = TargetBitmap1.Height;
        Dst.Rectangle.Height = TargetBitmap1.Height;
    }

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = TRUE;
    Context.MaskWord                = 0xF800;
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
    Context.WriteInsideClipRectangle = FALSE/*TRUE*/;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Mask Word \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();

			}
        #endif
		else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_MaskWord_red.gam",&TargetBitmap1,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_MaskWord_red.gam",&TargetBitmap1,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_MaskWord_red.gam",&TargetBitmap1,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_MaskWord_red.gam",&TargetBitmap1,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    Dst.Bitmap_p              = &TargetBitmap2;
    Context.MaskWord          = 0x7E0;

    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
		else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_MaskWord_green.gam",&TargetBitmap2,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_MaskWord_green.gam",&TargetBitmap2,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
		    semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_MaskWord_green.gam",&TargetBitmap2,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_MaskWord_green.gam",&TargetBitmap2,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
	#endif
    Context.MaskWord                = 0x001F;
    Dst.Bitmap_p              = &TargetBitmap3;


    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
		else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_MaskWord_blue.gam",&TargetBitmap3,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_MaskWord_blue.gam",&TargetBitmap3,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
		    semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_MaskWord_blue.gam",&TargetBitmap3,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_MaskWord_blue.gam",&TargetBitmap3,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }

	#endif


    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);
    GUTIL_Free(TargetBitmap1.Data1_p);
    GUTIL_Free(TargetBitmap2.Data1_p);
    GUTIL_Free(TargetBitmap3.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}


/*-----------------------------------------------------------------------------
 * Function : BLIT_TestMaskBitmap;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestMaskBitmap (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestMaskBitmap (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Foreground;
    STBLIT_Destination_t    Dst;

    STGXOBJ_Bitmap_t        ForegroundBitmap,MaskBitmap,TargetBitmap;
    STGXOBJ_Palette_t       Palette;


#ifdef ST_OSLINUX
    STLAYER_AllocDataParams_t    AllocDataParams;
#else
    STAVMEM_BlockHandle_t  Work_Handle;
    STAVMEM_AllocBlockParams_t  AllocBlockParams; /* Allocation param*/
    STAVMEM_MemoryRange_t  RangeArea[2];
    STAVMEM_FreeBlockParams_t FreeBlockParams;
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif
    U8                      NbForbiddenRange;
    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif

    /* ------------ Set Foreground and Mask------------ */
    ForegroundBitmap.Data1_p = NULL;
    MaskBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("./blt/files/merouARGB8888.gam",0,&ForegroundBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0,&ForegroundBitmap,&Palette);
#endif

#else

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("../../merouargb8888.gam",0,&ForegroundBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",0,&ForegroundBitmap,&Palette);
#endif

#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/mmask.gam",0,&MaskBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../mmask.gam",0,&MaskBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Foreground.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Foreground.Data.Bitmap_p        = &ForegroundBitmap;
    Foreground.Rectangle.PositionX  = 0;
    Foreground.Rectangle.PositionY  = 0;
    Foreground.Rectangle.Width      = MaskBitmap.Width;
    Foreground.Rectangle.Height     = MaskBitmap.Height;
    Foreground.Palette_p            = &Palette;

    /* ------------ Set Dst ------------ */
#ifdef ST_OSLINUX

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("./blt/files/BATEAU.GAM",0,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("./blt/files/suzie.gam",0,&TargetBitmap,&Palette);
#endif

#else

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("../../crow.gam",0,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../suzie.gam",0,&TargetBitmap,&Palette);
#endif

#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle             = Foreground.Rectangle;
    Dst.Palette_p             = NULL;

#ifdef ST_OSLINUX

    AllocDataParams.Alignment = 16;
    AllocDataParams.Size = ForegroundBitmap.Size1;

    Err = STLAYER_AllocData( FakeLayerHndl, &AllocDataParams, (void**)&(Context.WorkBuffer_p) );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }

#else
    /* ------------- Allocation Work buffer ---------------*/
    NbForbiddenRange = 1;
    if (VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
         VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) +
                                             (U32)(VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange= 2;
    }
    AllocBlockParams.PartitionHandle            = AvmemPartitionHandle[0];
    AllocBlockParams.AllocMode                  = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders   = 0;
    AllocBlockParams.ForbiddenBorderArray_p     = NULL;
    AllocBlockParams.Alignment = 16;

    AllocBlockParams.Size = ForegroundBitmap.Size1;

    Err = STAVMEM_AllocBlock(&AllocBlockParams,&Work_Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
    STAVMEM_GetBlockAddress(Work_Handle,(void**)&(Context.WorkBuffer_p));
#endif

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;

    Context.EnableMaskBitmap        = TRUE;
    Context.MaskBitmap_p            = &MaskBitmap;
    Context.MaskRectangle           = Foreground.Rectangle;

    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
/*    Context.WriteInsideClipRectangle = TRUE;*/
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Mask Bitmap \n"));
    Err = STBLIT_Blit(Handle,NULL,&Foreground,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
     #ifndef ST_OSLINUX
            time = time_plus(time_now(), 15625*10);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)

            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();

			}
        #endif
/*        else*/
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_Mask_bitmap.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_Mask_bitmap.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
		    semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Mask_bitmap.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Mask_bitmap.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(MaskBitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);
    GUTIL_Free(ForegroundBitmap.Data1_p);

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,Context.WorkBuffer_p );
#else
    /* Free Work buffer */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&Work_Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestMaskBitmapFill;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestMaskBitmapFill (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestMaskBitmapFill (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Foreground;
    STBLIT_Destination_t    Dst;

    STGXOBJ_Bitmap_t        ForegroundBitmap,MaskBitmap,TargetBitmap;
    STGXOBJ_Palette_t       Palette;

#ifdef ST_OSLINUX
    STLAYER_AllocDataParams_t    AllocDataParams;
#else
    STAVMEM_BlockHandle_t  Work_Handle;
    STAVMEM_AllocBlockParams_t  AllocBlockParams; /* Allocation param*/
    STAVMEM_MemoryRange_t  RangeArea[2];
    STAVMEM_FreeBlockParams_t FreeBlockParams;
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif
    U8                      NbForbiddenRange;
    U32                     i;
    STGXOBJ_Color_t         Color;
    STGXOBJ_Rectangle_t     Rectangle;
    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif

    /* ------------ Set Foreground and Mask------------ */
    ForegroundBitmap.Data1_p = NULL;
    MaskBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("./blt/files/merouARGB8888.gam",0,&ForegroundBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0,&ForegroundBitmap,&Palette);
#endif

#else

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("../../merouargb8888.gam",0,&ForegroundBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",0,&ForegroundBitmap,&Palette);
#endif

#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }


#ifdef ST_OSLINUX
    ConvertGammaToBitmap("./blt/files/MASK6.gam",0,&MaskBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../mask6.gam",0,&MaskBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Color.Type                 = STGXOBJ_COLOR_TYPE_ARGB8888 ;
    Color.Value.ARGB8888.Alpha = 128 ;
    Color.Value.ARGB8888.R     = 0x00 ;
    Color.Value.ARGB8888.G     = 0x00 ;
    Color.Value.ARGB8888.B     = 0xFF ;


    Foreground.Type                 = STBLIT_SOURCE_TYPE_COLOR;
    Foreground.Data.Color_p        = &Color;


    Rectangle.PositionX  = 0;
    Rectangle.PositionY  = 0;
    Rectangle.Width      = MaskBitmap.Width;
    Rectangle.Height     = MaskBitmap.Height;

    /* ------------ Set Dst ------------ */
#ifdef ST_OSLINUX

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("./blt/files/crow.gam",0,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("./blt/file/suzie.gam",0,&TargetBitmap,&Palette);
#endif

#else

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)
    Err = ConvertGammaToBitmap("../../crow.gam",0,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../suzie.gam",0,&TargetBitmap,&Palette);
#endif


#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle             = Rectangle;
    Dst.Palette_p             = NULL;

#ifdef ST_OSLINUX

    AllocDataParams.Alignment = 16;

#ifdef FILL_METHOD
    AllocDataParams.Size = 3000;  /* More than 256 * 4 for palette */
#else
    AllocDataParams.Size = ForegroundBitmap.Size1;
#endif

    Err = STLAYER_AllocData( FakeLayerHndl, &AllocDataParams, (void**)&(Context.WorkBuffer_p) );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }


#else

    /* ------------- Allocation Work buffer ---------------*/
    NbForbiddenRange = 1;
    if (VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
         VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) +
                                             (U32)(VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange= 2;
    }
    AllocBlockParams.PartitionHandle            = AvmemPartitionHandle[0];
    AllocBlockParams.AllocMode                  = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges    = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p      = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders   = 0;
    AllocBlockParams.ForbiddenBorderArray_p     = NULL;
    AllocBlockParams.Alignment                  = 16;

#ifdef FILL_METHOD
    AllocBlockParams.Size = 3000;  /* More than 256 * 4 for palette */
#else
    AllocBlockParams.Size = ForegroundBitmap.Size1;
#endif
    Err = STAVMEM_AllocBlock(&AllocBlockParams,&Work_Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
    STAVMEM_GetBlockAddress(Work_Handle,(void**)&(Context.WorkBuffer_p));
#endif

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;

    Context.EnableMaskBitmap        = TRUE;
    Context.MaskBitmap_p            = &MaskBitmap;
    Context.MaskRectangle           = Rectangle;

    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
/*    Context.WriteInsideClipRectangle = TRUE;*/
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;



    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Mask Bitmap Fill \n"));
#ifdef FILL_METHOD
    Err = STBLIT_FillRectangle(Handle,&TargetBitmap,&Rectangle,&Color,&Context );
#else
    Err = STBLIT_Blit(Handle,NULL,&Foreground,&Dst,&Context );
#endif

    if (Err != ST_NO_ERROR)
    {
        #ifdef FILL_METHOD
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Fill : %d\n",Err));
        #else
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        #endif
        return (TRUE);
    }
	#ifndef STBLIT_EMULATOR
        #ifndef ST_OSLINUX
            time = time_plus(time_now(), 15625*10);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();

			}
        #endif
		else
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_Mask_bitmapFill.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_Mask_bitmapFill.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
		    semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Mask_bitmapFill.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Mask_bitmapFill.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(MaskBitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);
    GUTIL_Free(ForegroundBitmap.Data1_p);

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,Context.WorkBuffer_p );
#else
    /* Free Work buffer */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&Work_Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestClip;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestClip (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestClip (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap, TargetBitmap2;
#ifdef ST_OSLINUX
    STGXOBJ_Bitmap_t        TargetBitmap3;
#endif
    STGXOBJ_Palette_t       Palette;
    STGXOBJ_Rectangle_t     Rectangle;
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;
    STEVT_SubscriberID_t    SubscriberID;
#endif

#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
			osclock_t       time;
    #endif
    #ifdef ST_OS20
			clock_t         time;
    #endif
#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188)|| defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif

    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    TargetBitmap2.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 40;
    Src.Rectangle.PositionY  = 30;
    Src.Rectangle.Width      = 200;/*SourceBitmap.Width;*/
    Src.Rectangle.Height     = 200;/*SourceBitmap.Height; */
    Src.Palette_p            = &Palette;

    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(DstFileName,SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(DstFileName,DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap2,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap2,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }


    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Width       = 200;
    Dst.Rectangle.Height      = 200;
    Dst.Palette_p             = NULL;

    /* ------------ Set Clip Rectangle ------------ */
    Rectangle.PositionX = 50 ;
    Rectangle.PositionY = 50 ;
    Rectangle.Width     = 140 ;
    Rectangle.Height    = 140 ;


    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
    Context.EnableMaskBitmap        = FALSE;
    Context.EnableColorCorrection   = FALSE;
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = TRUE;
    Context.ClipRectangle           = Rectangle;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Clipping : Write inside \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
 #ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
        /*time = time_plus(time_now(), 15625*5);*/
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
	 /*  Wait for Blit to be completed */
    #if defined(ST_OSLINUX)
        if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
        }
    #endif
    #if defined(ST_OS21) /*|| defined(ST_OSLINUX)*/
        if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
            ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    else
    {
        /* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Clipping_inside.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Clipping_inside.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    /* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Clipping_inside.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Clipping_inside.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
#endif

/*    TargetBitmap2.Data1_p=NULL;
    Err = ConvertGammaToBitmap(DstFileName,&TargetBitmap2,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }*/

    Dst.Bitmap_p              = &TargetBitmap2;
    Context.WriteInsideClipRectangle = FALSE;

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Clipping : Write outside \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
	/*  Wait for Blit to be completed */
    #if defined(ST_OSLINUX)
        if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
        }
    #endif
    #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
            ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    else
    {
        /* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Clipping_outside.gam",&TargetBitmap2,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Clipping_outside.gam",&TargetBitmap2,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    /* Generate Bitmap */
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Clipping_outside.gam",&TargetBitmap2,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Clipping_outside.gam",&TargetBitmap2,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif


    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);
    GUTIL_Free(TargetBitmap2.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif
    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestColor;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestColor (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestColor (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;

#ifndef ST_OSLINUX
    STAVMEM_BlockHandle_t  BlockHandle1;
    STAVMEM_BlockHandle_t  BlockHandle2;
#endif
    STGXOBJ_BitmapAllocParams_t Params1;
    STGXOBJ_BitmapAllocParams_t Params2;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    U32                     i;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif

#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif

#if !defined(ST_5100) && !defined(ST_5105) && !defined(ST_5301) && !defined(ST_5188) && !defined (ST_5525) && !defined(ST_5107)

#ifdef ST_OSLINUX
    STLAYER_AllocDataParams_t    AllocDataParams;
#else
    U8                      NbForbiddenRange;
    STAVMEM_BlockHandle_t  PaletteHandle;
    STAVMEM_AllocBlockParams_t  AllocBlockParams; /* Allocation param*/
    STAVMEM_MemoryRange_t  RangeArea[2];
#endif

                            /* A R G B  */
    U32 DataPalette[256]={  0xFF000000, 0xFF000033, 0xFF000066, 0xFF000099, 0xFF0000CC, 0xFF0000FF,
                            0xFF003300, 0xFF003333, 0xFF003366, 0xFF003399, 0xFF0033CC, 0xFF0033FF,
                            0xFF006600, 0xFF006633, 0xFF006666, 0xFF006699, 0xFF0066CC, 0xFF0066FF,
                            0xFF009900, 0xFF009933, 0xFF009966, 0xFF009999, 0xFF0099CC, 0xFF0099FF,
                            0xFF00CC00, 0xFF00CC33, 0xFF00CC66, 0xFF00CC99, 0xFF00CCCC, 0xFF00CCFF,
                            0xFF00FF00, 0xFF00FF33, 0xFF00FF66, 0xFF00FF99, 0xFF00FFCC, 0xFF00FFFF,

                            0xFF330000, 0xFF330033, 0xFF330066, 0xFF330099, 0xFF3300CC, 0xFF3300FF,
                            0xFF333300, 0xFF333333, 0xFF333366, 0xFF333399, 0xFF3333CC, 0xFF3333FF,
                            0xFF336600, 0xFF336633, 0xFF336666, 0xFF336699, 0xFF3366CC, 0xFF3366FF,
                            0xFF339900, 0xFF339933, 0xFF339966, 0xFF339999, 0xFF3399CC, 0xFF3399FF,
                            0xFF33CC00, 0xFF33CC33, 0xFF33CC66, 0xFF33CC99, 0xFF33CCCC, 0xFF33CCFF,
                            0xFF33FF00, 0xFF33FF33, 0xFF33FF66, 0xFF33FF99, 0xFF33FFCC, 0xFF33FFFF,

                            0xFF660000, 0xFF660033, 0xFF660066, 0xFF660099, 0xFF6600CC, 0xFF6600FF,
                            0xFF663300, 0xFF663333, 0xFF663366, 0xFF663399, 0xFF6633CC, 0xFF6633FF,
                            0xFF666600, 0xFF666633, 0xFF666666, 0xFF666699, 0xFF6666CC, 0xFF6666FF,
                            0xFF669900, 0xFF669933, 0xFF669966, 0xFF669999, 0xFF6699CC, 0xFF6699FF,
                            0xFF66CC00, 0xFF66CC33, 0xFF66CC66, 0xFF66CC99, 0xFF66CCCC, 0xFF66CCFF,
                            0xFF66FF00, 0xFF66FF33, 0xFF66FF66, 0xFF66FF99, 0xFF66FFCC, 0xFF66FFFF,

                            0xFF990000, 0xFF990033, 0xFF990066, 0xFF990099, 0xFF9900CC, 0xFF9900FF,
                            0xFF993300, 0xFF993333, 0xFF993366, 0xFF993399, 0xFF9933CC, 0xFF9933FF,
                            0xFF996600, 0xFF996633, 0xFF996666, 0xFF996699, 0xFF9966CC, 0xFF9966FF,
                            0xFF999900, 0xFF999933, 0xFF999966, 0xFF999999, 0xFF9999CC, 0xFF9999FF,
                            0xFF99CC00, 0xFF99CC33, 0xFF99CC66, 0xFF99CC99, 0xFF99CCCC, 0xFF99CCFF,
                            0xFF99FF00, 0xFF99FF33, 0xFF99FF66, 0xFF99FF99, 0xFF99FFCC, 0xFF99FFFF,

                            0xFFCC0000, 0xFFCC0033, 0xFFCC0066, 0xFFCC0099, 0xFFCC00CC, 0xFFCC00FF,
                            0xFFCC3300, 0xFFCC3333, 0xFFCC3366, 0xFFCC3399, 0xFFCC33CC, 0xFFCC33FF,
                            0xFFCC6600, 0xFFCC6633, 0xFFCC6666, 0xFFCC6699, 0xFFCC66CC, 0xFFCC66FF,
                            0xFFCC9900, 0xFFCC9933, 0xFFCC9966, 0xFFCC9999, 0xFFCC99CC, 0xFFCC99FF,
                            0xFFCCCC00, 0xFFCCCC33, 0xFFCCCC66, 0xFFCCCC99, 0xFFCCCCCC, 0xFFCCCCFF,
                            0xFFCCFF00, 0xFFCCFF33, 0xFFCCFF66, 0xFFCCFF99, 0xFFCCFFCC, 0xFFCCFFFF,

                            0xFFFF0000, 0xFFFF0033, 0xFFFF0066, 0xFFFF0099, 0xFFFF00CC, 0xFFFF00FF,
                            0xFFFF3300, 0xFFFF3333, 0xFFFF3366, 0xFFFF3399, 0xFFFF33CC, 0xFFFF33FF,
                            0xFFFF6600, 0xFFFF6633, 0xFFFF6666, 0xFFFF6699, 0xFFFF66CC, 0xFFFF66FF,
                            0xFFFF9900, 0xFFFF9933, 0xFFFF9966, 0xFFFF9999, 0xFFFF99CC, 0xFFFF99FF,
                            0xFFFFCC00, 0xFFFFCC33, 0xFFFFCC66, 0xFFFFCC99, 0xFFFFCCCC, 0xFFFFCCFF,
                            0xFFFFFF00, 0xFFFFFF33, 0xFFFFFF66, 0xFFFFFF99, 0xFFFFFFCC, 0xFFFFFFFF};
#endif


    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_ARGB8888;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
    Context.EnableMaskBitmap        = FALSE;
    Context.EnableColorCorrection   = FALSE;
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion RGB565 -> ARGB8888\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
    #if defined(ST_OSLINUX)
        if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
        }
    #endif
    #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    else
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_RGB565_to_ARGB8888.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_RGB565_to_ARGB8888.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_RGB565_to_ARGB8888.gam"&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_RGB565_to_ARGB8888.gam",&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif

    /* --------------- Free Bitmap ------------ */
#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    /* Free TargetBitmap */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion RGB565 -> YCbCr422R\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*50);
    #endif
    #if defined(ST_OSLINUX)
        if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
        }
    #endif
    #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    else
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_RGB565_to_YCbCr422R.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_RGB565_to_YCbCr422R.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_RGB565_to_YCbCr422R.gam",&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_RGB565_to_YCbCr422R.gam",&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif

#if !defined (ST_5100) &&  !defined (ST_5105) &&  !defined (ST_5301) &&  !defined (ST_7109) &&  !defined (ST_5188)\
&&  !defined (ST_5525) &&  !defined (ST_5107)
    /* --------------- Free Bitmap ------------ */

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    /* Free TargetBitmap */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
    TargetBitmap.Data1_p=NULL;
#endif

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_CLUT8;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    Palette.PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
    Palette.ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
    Palette.ColorDepth  = 8;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX

    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif


#ifdef ST_OSLINUX

    AllocDataParams.Alignment = 16;
    AllocDataParams.Size = 256*4;
    Err = STLAYER_AllocData( FakeLayerHndl, &AllocDataParams, (void**)&(Palette.Data_p) );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }


#else

    /* Block allocation parameter */
    NbForbiddenRange = 1;
    if (VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
         VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) +
                                             (U32)(VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange= 2;
    }


    AllocBlockParams.PartitionHandle            = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    AllocBlockParams.AllocMode                  = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges    = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p      = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders   = 0;
    AllocBlockParams.ForbiddenBorderArray_p     = NULL;
    AllocBlockParams.Alignment = 16;

    AllocBlockParams.Size = 256*4;


    Err = STAVMEM_AllocBlock(&AllocBlockParams,&PaletteHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
    STAVMEM_GetBlockAddress(PaletteHandle,(void**)&(Palette.Data_p));
#endif

#if ST_OSLINUX
    {
        U32 Tmp;
        for ( Tmp = 0; Tmp <  256 ; Tmp++ )
        {
            *((U32 *) (((U32 *) Palette.Data_p) + Tmp)) = DataPalette[Tmp];
        }
    }
#else
    Err=STAVMEM_CopyBlock1D((void*)  DataPalette, STAVMEM_VirtualToCPU((void*)Palette.Data_p,&VirtualMapping),
                             AllocBlockParams.Size);
    if (Err != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to fill for palette\n"));
    }
#endif


    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = &Palette;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion RGB565 -> CLUT8\n"));
	Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
	if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY );  /* Timeout period was not enough in case of Os21*/
    #endif
    #ifdef ST_OS21
        time = time_plus(time_now(), 15625*13 );  /* Timeout period was not enough in case of Os21*/
    #endif
    #ifdef ST_OS20
        time = time_plus(time_now(), 15625*50);
    #endif
    #if defined(ST_OSLINUX)
        if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
        }
    #endif
    #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
        #if !defined( ST_7710 )         /* Bug : timeout with 7710 */
            if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
                ResetEngine();
            }
        #else
            if( 1 )
            {
            }
        #endif
    #endif
    else
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_RGB565_to_CLUT8.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_RGB565_to_CLUT8.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_RGB565_to_CLUT8.gam",&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_RGB565_to_CLUT8.gam",&TargetBitmap,&Palette);
    #endif
	if (Err != ST_NO_ERROR)
	{
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
		return (TRUE);
	}
#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
    STLAYER_FreeData( FakeLayerHndl,Palette.Data_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }

    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&PaletteHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;
    Palette.Data_p=NULL;
    SourceBitmap.Data1_p=NULL;
#endif /* !defined (ST_5100) &&  !defined (ST_5105)  && !defined (ST_7109) &&  !defined (ST_5188) &&  !defined (ST_5107)*/

    /* ------------ Set source ------------ */

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouARGB8888.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouARGB8888.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap 3: %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_RGB565;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX

    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion ARGB8888 -> RGB565\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    else
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ARGB8888_to_RGB565.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_ARGB8888_to_RGB565.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ARGB8888_to_RGB565.gam",&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_ARGB8888_to_RGB565.gam",&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
		return (TRUE);
    }
#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;
    SourceBitmap.Data1_p=NULL;


#if !defined (ST_5105) &&  !defined (ST_5188) &&  !defined (ST_5107)

    /* ------------ Set source ------------ */
    #ifdef ST_OSLINUX
        Err = ConvertGammaToBitmap("./blt/files/YcbCr422r.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
    #else
        Err = ConvertGammaToBitmap("../../YcbCr422r.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_RGB565;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX

    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);

#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif
    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;
    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion YCbCr422R -> RGB565\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    else
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_YCbCr422R_to_RGB565.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_YCbCr422R_to_RGB565.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_YCbCr422R_to_RGB565.gam",&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_YCbCr422R_to_RGB565.gam",&TargetBitmap,&Palette);
    #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
#endif

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
     /* --------------- Free Target Bitmap ------------ */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;
#endif  /* !defined (ST_5100) */


#if !defined (ST_5100) &&  !defined (ST_5105) &&  !defined (ST_5301) && !defined (ST_7109) &&  !defined (ST_5188)\
&&  !defined (ST_5525) &&  !defined (ST_5107)
    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_CLUT8;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    Palette.PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
    Palette.ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
    Palette.ColorDepth  = 8;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

#ifdef ST_OSLINUX

    AllocDataParams.Alignment = 16;
    AllocDataParams.Size = 256*4;

    Err = STLAYER_AllocData( FakeLayerHndl, &AllocDataParams, (void**)&(Palette.Data_p) );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }

#else

    /* Block allocation parameter */
    NbForbiddenRange = 1;
    if (VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
         VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) +
                                             (U32)(VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange= 2;
    }

    AllocBlockParams.PartitionHandle            = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    AllocBlockParams.Size                       = 0;
    AllocBlockParams.AllocMode                  = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges    = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p      = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders   = 0;
    AllocBlockParams.ForbiddenBorderArray_p     = NULL;
    AllocBlockParams.Alignment = 16;

    AllocBlockParams.Size = 256*4;

    Err = STAVMEM_AllocBlock(&AllocBlockParams,&PaletteHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
    STAVMEM_GetBlockAddress(PaletteHandle,(void**)&(Palette.Data_p));
#endif

#if ST_OSLINUX
    {
        U32 Tmp;
        for ( Tmp = 0; Tmp <  256 ; Tmp++ )
        {
            *((U32 *) (((U32 *) Palette.Data_p) + Tmp)) = DataPalette[Tmp];
        }
    }

#else
    Err=STAVMEM_CopyBlock1D((void*)DataPalette,STAVMEM_VirtualToCPU((void*)Palette.Data_p,&VirtualMapping),
                            AllocBlockParams.Size );
    if (Err != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to fill for palette\n"));
    }
#endif
    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = &Palette;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion YCbCr422R -> CLUT8\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*50);
    #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    else
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_YCbCr422R_to_CLUT8.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_YCbCr422R_to_CLUT8.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_YCbCr422R_to_CLUT8.gam",&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_YCbCr422R_to_CLUT8.gam",&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
    STLAYER_FreeData( FakeLayerHndl,Palette.Data_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }

    Err = STAVMEM_FreeBlock(&FreeBlockParams,&PaletteHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;
    Palette.Data_p=NULL;
    SourceBitmap.Data1_p=NULL;
#endif /* !defined (ST_5100) &&  !defined (ST_5105)  && !defined (ST_7109) &&  !defined (ST_5188) &&  !defined (ST_5107) */


     STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"START CLUT4-->RGB Convert Gamma to Bitmap\n"));
    /* ------------ Set source ------------ */
    #ifdef ST_OSLINUX
        Err = ConvertGammaToBitmap("./blt/files/Chrisb4.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
    #else
        Err = ConvertGammaToBitmap("../../Chrisb4.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_RGB565;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif


    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion ACLUT4 -> RGB565\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();

		}
    #endif
    else
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ACLUT4_to_RGB565.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_ACLUT4_to_RGB565.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ACLUT4_to_RGB565.gam",&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_ACLUT4_to_RGB565.gam",&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    /* --------------- Free Bitmap ------------ */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;


    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion ACLUT4 -> YCbCr422R\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*5);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    else
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ACLUT4_to_YCbCr422R.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_ACLUT4_to_YCbCr422R.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ACLUT4_to_YCbCr422R.gam",&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_ACLUT4_to_YCbCr422R.gam",&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;
    SourceBitmap.Data1_p=NULL;

    /* ------------ Set source ------------ */
    #ifdef ST_OSLINUX
        Err = ConvertGammaToBitmap("./blt/files/Chrisb8.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
    #else
        Err = ConvertGammaToBitmap("../../Chrisb8.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_RGB565;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion ACLUT8 -> RGB565\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
        #if defined(ST_OSLINUX)
/*            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }*/
        #endif
        #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();

		}
    #endif
/*    else*/
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ACLUT8_to_RGB565.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_ACLUT8_to_RGB565.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ACLUT8_to_RGB565.gam",&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_ACLUT8_to_RGB565.gam",&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    /* --------------- Free Bitmap ------------ */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;


    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
    TargetBitmap.BitmapType             = SourceBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);

#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion ACLUT8 -> YCbCr422R\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    else
    {
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ACLUT8_to_YCbCr422R.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Conversion_ACLUT8_to_YCbCr422R.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_Conversion_ACLUT8_to_YCbCr422R.gam",&TargetBitmap,&Palette);
    #else
        Err = ConvertBitmapToGamma("../../../result/test_Conversion_ACLUT8_to_YCbCr422R.gam",&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }
#endif

    /* --------------- Free Bitmap ------------ */

    GUTIL_Free(SourceBitmap.Data1_p);

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;
    SourceBitmap.Data1_p=NULL;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);

}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestCopy2Src;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestCopy2Src (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestCopy2Src (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src1,Src2;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap;
    STGXOBJ_Palette_t       Palette;



    STGXOBJ_Bitmap_t        Source1Bitmap,Source2Bitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette1,Palette2,PaletteT;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif

	#ifndef STBLIT_EMULATOR
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			osclock_t       time;
        #endif
        #ifdef ST_OS20
			clock_t         time;
        #endif
	#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105)  || defined (ST_5301) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif


    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif
    /* ------------ Set source 1 ------------ */
    Source1Bitmap.Data1_p = NULL;
    Palette1.Data_p = NULL;


#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/suzie.gam",SECURED_SRC_BITMAP,&Source1Bitmap,&Palette1);
#else
    Err = ConvertGammaToBitmap("../../suzie.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&Source1Bitmap,&Palette1);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }


    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src1.Data.Bitmap_p        = &Source1Bitmap;
    Src1.Rectangle.PositionX  = 50;
    Src1.Rectangle.PositionY  = 50;
    Src1.Rectangle.Width      = 150;
    Src1.Rectangle.Height     = 150;
    Src1.Palette_p            = &Palette1;

    /* ------------ Set source ------------ */
    Source2Bitmap.Data1_p = NULL;
    Palette2.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/BATEAU.GAM",SECURED_SRC_BITMAP,&Source2Bitmap,&Palette2);
#else
    Err = ConvertGammaToBitmap("../../bateau.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&Source2Bitmap,&Palette1);
  #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Source2Bitmap;
    Src2.Rectangle.PositionX  = 100;
    Src2.Rectangle.PositionY  = 100;
    Src2.Rectangle.Width      = /*60*/150;
    Src2.Rectangle.Height     = 150;
    Src2.Palette_p            = &Palette2;













    /* ------------ Set Dest ------------ */
    TargetBitmap.Data1_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap,&PaletteT);
#else
    Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&PaletteT);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 50;
    Dst.Rectangle.PositionY   = 50;
    Dst.Rectangle.Width       = 150;
    Dst.Rectangle.Height      = 150;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = /*STBLIT_ALU_COPY*/STBLIT_ALU_ALPHA_BLEND;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 20;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Copy 2Src \n"));
    Err = STBLIT_Blit(Handle,&Src1,&Src2,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_Copy_2Src.gam",&TargetBitmap,&PaletteT);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_Copy_2Src.gam",&TargetBitmap,&PaletteT);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Copy_2Src.gam",&TargetBitmap,&PaletteT);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Copy_2Src.gam",&TargetBitmap,&PaletteT);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(Source1Bitmap.Data1_p);
    GUTIL_Free(Source2Bitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}



/*-----------------------------------------------------------------------------
 * Function : BLIT_TestConcat
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
 BOOL BLIT_TestConcat (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
 boolean BLIT_TestConcat (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;

    STGXOBJ_Bitmap_t        AlphaBitmap, ColorBitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;

    STGXOBJ_Rectangle_t     SrcRectangle;

#ifndef ST_OSLINUX
    STAVMEM_BlockHandle_t  BlockHandle1;
    STAVMEM_BlockHandle_t  BlockHandle2;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
#endif
    STGXOBJ_BitmapAllocParams_t Params1;
    STGXOBJ_BitmapAllocParams_t Params2;


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;
    STEVT_SubscriberID_t    SubscriberID;
#endif
#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif
#ifdef CALC_TIME
    clock_t                 t1, t2 ;
#endif
    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
	    BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }
    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }
    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif

    /*-----------------2/26/01 9:04AM-------------------
     * Concat
     * --------------------------------------------------*/

    AlphaBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/MaskAlpha8.gam",0,&AlphaBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../MaskAlpha8.gam",0,&AlphaBitmap,&Palette);
#endif
    AlphaBitmap.ColorType = STGXOBJ_COLOR_TYPE_ALPHA8;
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    ColorBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/m888.gam",0,&ColorBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../m888.gam",0,&ColorBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set Dest ------------ */
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;


    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_ARGB8888;
    TargetBitmap.BitmapType             = ColorBitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = /*STGXOBJ_ITU_R_BT601*/STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = AlphaBitmap.Width;
    TargetBitmap.Height                 = AlphaBitmap.Height;
    TargetBitmap.Data1_p                = NULL;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[0],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif


    /* ------------ Set Context ------------ */
    /* Set Context */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = TRUE;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */


    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Concat operation\n"));


    /* Concatenation */

    SrcRectangle.PositionX  = 0;
    SrcRectangle.PositionY  = 0;
    SrcRectangle.Width      = 720;
    SrcRectangle.Height     = 480;

    Err = STBLIT_Concat(Handle,&AlphaBitmap,&SrcRectangle,&ColorBitmap,&SrcRectangle, &TargetBitmap, &SrcRectangle,&Context);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Concat: %d\n",Err));
        return (TRUE);
    }

    #ifndef STBLIT_EMULATOR
        #ifndef ST_OSLINUX
            time = time_plus(time_now(), 15625*5);
        #endif

        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
                ResetEngine();
            }
        #endif
        #if defined(ST_OS21)
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT\n"));
                ResetEngine();
            }
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
            }
        #endif
        else
		{
	        /* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_Concat_Bitmap.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_Concat_Bitmap.gam",&TargetBitmap,&Palette);
            #endif
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Concat_Bitmap.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Concat_Bitmap.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
	        return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */

    /* Free Bitmapq */
    GUTIL_Free(AlphaBitmap.Data1_p);
    GUTIL_Free(ColorBitmap.Data1_p);
/*    GUTIL_Free(TargetBitmap.Data1_p);*/

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif
    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}


/*-----------------------------------------------------------------------------
 * Function : BLIT_TestBlend;

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestBlend (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestBlend (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;
    STGXOBJ_Color_t         Color;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


    #ifndef STBLIT_EMULATOR
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			osclock_t       time;
        #endif
        #ifdef ST_OS20
			clock_t         time;
        #endif
	#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 20;
    Src.Rectangle.PositionY  = 20;
    Src.Rectangle.Width      = 200;
    Src.Rectangle.Height     = 200;
    Src.Palette_p            = &Palette;

    /* ------------ Set Dest ------------ */
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(DstFileName,SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(DstFileName,DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
/*    Dst.Rectangle.PositionX   = 0;*/
/*    Dst.Rectangle.PositionY   = 0;*/
/*    Dst.Rectangle.Width       = 100;*/
/*    Dst.Rectangle.Height      = 200;*/
    Dst.Rectangle.PositionX   = 10;
    Dst.Rectangle.PositionY   = 10;
    Dst.Rectangle.Width       = 200;
    Dst.Rectangle.Height      = 200;


    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = /*STBLIT_ALU_COPY*/STBLIT_ALU_ALPHA_BLEND;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 60;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;



    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Bitmap Blend \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_Blend_Bitmap.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_Blend_Bitmap.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_Blend_Bitmap.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Blend_Bitmap.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* ------------ Set Source ------------ */
    Color.Type                 = STGXOBJ_COLOR_TYPE_RGB565 ;
    Color.Value.RGB565.R     = 0x00 ;
    Color.Value.RGB565.G     = 0x00 ;
    Color.Value.RGB565.B     = 0xff ;

    Src.Type                 = STBLIT_SOURCE_TYPE_COLOR;
    Src.Data.Color_p=&Color;

    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    SourceBitmap.Data1_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(SrcFileName,SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(SrcFileName,SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 20;
    Src.Rectangle.PositionY  = 20;
    Src.Rectangle.Width      = 200;
    Src.Rectangle.Height     = 200;
    Src.Palette_p            = &Palette;
#endif


    /* ------------ Set Dest ------------ */
    #ifdef ST_OSLINUX
        Err = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_DST_BITMAP,&TargetBitmap,&Palette);
    #else
        Err = ConvertGammaToBitmap("../../merouRGB565.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 10;
    Dst.Rectangle.PositionY   = 10;
    Dst.Rectangle.Width       = 200;
    Dst.Rectangle.Height      = 200;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = /*STBLIT_ALU_COPY*/STBLIT_ALU_ALPHA_BLEND;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 60;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Color Blend \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #if defined(ST_OSLINUX)
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_Blend_Color.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_Blend_Color.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertGammaToBitmap("./blt/result/test_Blend_Color.gam",0,&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_Blend_Color.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif
    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}


/*-----------------------------------------------------------------------------
 * Function : BLIT_TestFilterFlicker;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestFilterFlicker (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestFilterFlicker (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


    #ifndef STBLIT_EMULATOR
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			osclock_t       time;
        #endif
        #ifdef ST_OS20
			clock_t         time;
        #endif
	#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188) || defined (ST_5525)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

    Err = ConvertGammaToBitmap("../../bateau_test1.gam",0,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = 720;
    Src.Rectangle.Height     = 576;
    Src.Palette_p            = &Palette;

    /* ------------ Set Dest ------------ */
    Err = ConvertGammaToBitmap("../../bateau_test1.gam",0,&TargetBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = 720;
    Dst.Rectangle.Height      = 576;


    Dst.Palette_p             = NULL;


    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Update The FliterFlicker Mode SIMPLE \n"));
    Err = STBLIT_SetFlickerFilterMode(Handle,STBLIT_FLICKER_FILTER_MODE_SIMPLE);

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = /*STBLIT_ALU_COPY*/STBLIT_ALU_ALPHA_BLEND;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 128;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter      = TRUE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Bitmap FliterFlicker Mode SIMPLE \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_FlickerFilter_Mode_SIMPLE.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_FlickerFilter_Mode_SIMPLE.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_FlickerFilter_Mode_SIMPLE.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_FlickerFilter_Mode_SIMPLE.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

    Err = ConvertGammaToBitmap("../../bateau_test1.gam",0,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = 720;
    Src.Rectangle.Height     = 576;
    Src.Palette_p            = &Palette;


    /* ------------ Set Dest ------------ */
    #ifdef ST_OSLINUX
        Err = ConvertGammaToBitmap("./blt/files/bateau_test1.gam",0,&TargetBitmap,&Palette);
    #else
        Err = ConvertGammaToBitmap("../../bateau_test1.gam",0,&TargetBitmap,&Palette);
    #endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = 720;
    Dst.Rectangle.Height      = 576;
    Dst.Palette_p             = NULL;

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Bitmap FliterFlicker Mode ADAPTIVE \n"));
    Err = STBLIT_SetFlickerFilterMode(Handle,STBLIT_FLICKER_FILTER_MODE_ADAPTIVE);



    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = /*STBLIT_ALU_COPY*/STBLIT_ALU_ALPHA_BLEND;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 128;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter      = TRUE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Bitmap FliterFlicker Mode ADAPTIVE \n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #if defined(ST_OSLINUX)
            time = time_plus(time_now(), 15625*TEST_DELAY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #if defined(ST_OSLINUX)
            if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
            }
        #endif
        #if defined(ST_OS21)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_FlickerFilter_Mode_ADAPTIVE.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_FlickerFilter_Mode_ADAPTIVE.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertGammaToBitmap("./blt/result/test_FlickerFilter_Mode_ADAPTIVE.gam",0,&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_FlickerFilter_Mode_ADAPTIVE.gam",0,&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif
    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestVC1Range;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestVC1Range (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestVC1Range (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;


#ifndef ST_OSLINUX
    STAVMEM_BlockHandle_t  BlockHandle1;
    STAVMEM_BlockHandle_t  BlockHandle2;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
#endif
    STGXOBJ_BitmapAllocParams_t Params1;
    STGXOBJ_BitmapAllocParams_t Params2;


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;
    STEVT_SubscriberID_t    SubscriberID;
#endif

    #ifndef STBLIT_EMULATOR
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			osclock_t       time;
        #endif
        #ifdef ST_OS20
			clock_t         time;
        #endif
	#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                       = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)

    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif

    /*=======================================================================*/
    /* Test1 : Range Mapping test                                            */
    /*=======================================================================*/
    /* ------------ Set source ------------ */

    SourceBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/bateauMB420.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../bateauMB420.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Data.Bitmap_p->BitmapType = STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP;
    /* set Src YUVScaling factors for range Reduction */
    Src.Data.Bitmap_p->YUVScaling.ScalingFactorY  = YUV_RANGE_MAP_6;
    Src.Data.Bitmap_p->YUVScaling.ScalingFactorUV = YUV_RANGE_MAP_6;

    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;


    /* ------------ Set Dst ------------ */
    TargetBitmap.Data1_p=NULL;
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
    TargetBitmap.BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;
    TargetBitmap.Data1_p                = NULL;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY /*STBLIT_ALU_ALPHA_BLEND*/;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 60;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start VC1 Range Mapping\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #ifdef ST_OS21
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
        {
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_MB_To_YCbCr422R_VC1_RangeMapping.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_MB_To_YCbCr422R_VC1_RangeMapping.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #ifdef ST_OS21
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_MB_To_YCbCr422R_VC1_RangeMapping.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_MB_To_YCbCr422R_VC1_RangeMapping.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    /* Free TargetBitmap */

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif


    TargetBitmap.Data1_p=NULL;

    /*=======================================================================*/
    /* Test2 : Range Reduction Test                                          */
    /*=======================================================================*/
#ifdef ST_OSLINUX
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

    Err = ConvertGammaToBitmap("./blt/files/bateauMB420.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    /* set Src YUVScaling factors for range Reduction */
    Src.Data.Bitmap_p->YUVScaling.ScalingFactorY  = YUV_RANGE_MAP_7;
    Src.Data.Bitmap_p->YUVScaling.ScalingFactorUV = YUV_RANGE_MAP_7;

    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;
#endif


    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
    TargetBitmap.BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;
    TargetBitmap.Data1_p                = NULL;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY /*STBLIT_ALU_ALPHA_BLEND*/;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 60;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Range Reduction\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #ifdef ST_OS21
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
        {
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_MB_To_YCbCr422R_VC1_RangeReduction.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_MB_To_YCbCr422R_VC1_RangeReduction.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #ifdef ST_OS21
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_MB_To_YCbCr422R_VC1_RangeReduction.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_MB_To_YCbCr422R_VC1_RangeReduction.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    /* Free TargetBitmap */

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);

    /* --------------- Close Evt ----------*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}
/*-----------------------------------------------------------------------------
 * Function : BLIT_TestMacroBlocConversion;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestMBConversion (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestMBConversion (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;
    STGXOBJ_Palette_t       Palette;


#ifndef ST_OSLINUX
    STAVMEM_BlockHandle_t  BlockHandle1;
    STAVMEM_BlockHandle_t  BlockHandle2;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
#endif
    STGXOBJ_BitmapAllocParams_t Params1;
    STGXOBJ_BitmapAllocParams_t Params2;


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;
    STEVT_SubscriberID_t    SubscriberID;
#endif

    #ifndef STBLIT_EMULATOR
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			osclock_t       time;
        #endif
        #ifdef ST_OS20
			clock_t         time;
        #endif
	#endif

    U32                     i;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                       = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)

    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif


#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    Err = STBLIT_SetSecurityRange(Handle, &SecurityParams);
#endif
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/bateauMB420.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../bateauMB420.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&SourceBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;

    /*=======================================================================*/
    /* Test1 : Test color conversion from MB to RGB565                       */
    /*=======================================================================*/

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_RGB565;
    TargetBitmap.BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;
    TargetBitmap.Data1_p                = NULL;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,TRUE,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;


    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY /*STBLIT_ALU_ALPHA_BLEND*/;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 60;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */
  STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion from MB to RGB565\n"));
   Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR

        #ifdef ST_OSLINUX
            semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif

        #ifdef ST_OS21
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_MB_To_RGB565_Conversion.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_MB_To_RGB565_Conversion.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_MB_To_YCbCr422R_Conversion.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_MB_To_RGB565_Conversion.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    /* Free TargetBitmap */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    TargetBitmap.Data1_p=NULL;

    /*=======================================================================*/
    /* Test2 : Test color conversion from MB to YCbCr422                     */
    /*=======================================================================*/
#ifdef ST_OSLINUX
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

    Err = ConvertGammaToBitmap("./blt/files/bateauMB420.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;
#endif


    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
    TargetBitmap.BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;
    TargetBitmap.Data1_p                = NULL;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY /*STBLIT_ALU_ALPHA_BLEND*/;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 60;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion from MB to YCbCr422R\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #ifdef ST_OS21
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
        {
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_MB_To_YCbCr422R_Conversion.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_MB_To_YCbCr422R_Conversion.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #ifdef ST_OS21
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_MB_To_YCbCr422R_Conversion.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_MB_To_YCbCr422R_Conversion.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* --------------- Free Bitmap ------------ */
    /* Free TargetBitmap */

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif


    TargetBitmap.Data1_p=NULL;
    Palette.Data_p = NULL;

#if defined(ST_OSLINUX) && defined(ST_7109)
#else

    /*=======================================================================*/
    /* Test3 : Test MB Copy                                                  */
    /*=======================================================================*/

#ifdef ST_OSLINUX
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
    Err = ConvertGammaToBitmap("./blt/files/bateauMB420.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Palette_p            = &Palette;
#endif


    Src.Rectangle.PositionX  = 200;
    Src.Rectangle.PositionY  = 200;
    Src.Rectangle.Width      = 200;
    Src.Rectangle.Height     = 200;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(DstFileName,SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(DstFileName,DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Width       = Src.Rectangle.Width;
    Dst.Rectangle.Height      = Src.Rectangle.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY /*STBLIT_ALU_ALPHA_BLEND*/;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 60;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start MB Copy\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #ifdef ST_OSLINUX
            semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif
        #ifdef ST_OS21
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_MB_Copy.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_MB_Copy.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #ifdef ST_OS21
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_MB_Copy.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_MB_Copy.gam",&TargetBitmap,&Palette);
        #endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif
    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(TargetBitmap.Data1_p);

    /*=======================================================================*/
    /* Test4 : Test MB Resize                                                */
    /*=======================================================================*/

#ifdef ST_OSLINUX
    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

    Err = ConvertGammaToBitmap("./blt/files/bateauMB420.gam",SECURED_SRC_BITMAP,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Palette_p            = &Palette;
#endif


    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap(DstFileName,SECURED_DST_BITMAP,&TargetBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap(DstFileName,DST_BITMAP_AVMEM_PARTITION_NUM,&TargetBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }


    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Width       = TargetBitmap.Width - 40;
    Dst.Rectangle.Height      = TargetBitmap.Height - 40;


    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 60;
    Context.EnableClipRectangle     = /*TRUE*/FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;


    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start MB Resize\n"));
    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

	#ifndef STBLIT_EMULATOR
        #if defined(ST_OSLINUX)
            semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #else
            time = time_plus(time_now(), 15625*200);
        #endif

        #ifdef ST_OS21
            if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
	   /* else */
		{
			/* Generate Bitmap */
            #ifdef ST_OSLINUX
                Err = ConvertBitmapToGamma("./blt/result/test_MB_Resize.gam",&TargetBitmap,&Palette);
            #else
                Err = ConvertBitmapToGamma("../../../result/test_MB_Resize.gam",&TargetBitmap,&Palette);
            #endif
			fflush(stdout);
			if (Err != ST_NO_ERROR)
	        {
		        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			    return (TRUE);
	        }
		}
	#else
		/*  Wait for Blit to be completed */
        #ifdef ST_OS21
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));

		/* Generate Bitmap */
#ifdef ST_OSLINUX
        Err = ConvertBitmapToGamma("./blt/result/test_MB_Resize.gam",&TargetBitmap,&Palette);
#else
        Err = ConvertBitmapToGamma("../../../result/test_MB_Resize.gam",&TargetBitmap,&Palette);
#endif
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif
#endif

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(SourceBitmap.Data1_p);

    /* --------------- Close Evt ----------*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestJob;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
    BOOL BLIT_TestJob (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
    boolean BLIT_TestJob (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          ErrCode;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     Open_Params;
    STBLIT_TermParams_t     Term_Params;
    STBLIT_Handle_t         Hdl;
    STBLIT_Source_t         Src1, Src2, Src3, Src4, SrcForeground;
    STBLIT_Destination_t    Dst1,Dst2,Dst3;
    STGXOBJ_Color_t         ColorFill;
    STBLIT_BlitContext_t    Context;
#if defined (DVD_SECURED_CHIP)
    STBLIT_SecurityParams_t SecurityParams;
#endif

    STGXOBJ_Bitmap_t        Bmp1, Bmp2, Bmp3, Bmp4, BmpT;
    STGXOBJ_Palette_t       Pal;
    STGXOBJ_Rectangle_t     PositionRectangle1,PositionRectangle2,PositionRectangle3,ClipRectangle;
    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_JobParams_t      Job_Params;
    STBLIT_JobHandle_t      Job1;
    STBLIT_JobBlitHandle_t  JBHandle_1,JBHandle_2,JBHandle_3;
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               JobCompletedTask;
    pthread_attr_t          JobCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


    /* ------------ Blit Device Init ------------ */

    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[AVMEM_PARTITION_NUM];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
     InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188) || defined (ST_5525)\
 || defined (ST_5107)
    InitParams.JobBlitNodeMaxNumber                 = 1000;
#else
    InitParams.JobBlitNodeMaxNumber                 = 30;
#endif
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 6;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)

    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    ErrCode = STBLIT_Init(Name,&InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    ErrCode = STBLIT_Open(Name,&Open_Params,&Hdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Process Blit operation ------------ */

    /* ----- Set Job Params ----*/
    Job_Params.NotifyJobCompletion = TRUE;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		JobCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20

		semaphore_init_fifo_timeout(&JobCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    ErrCode = STBLIT_UserTaskCreate( (void*) JobCompleted_Task,
            &(JobCompletedTask),
            &(JobCompletedTask_Attribute),
            NULL );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :JobCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }

#else
    /* Open Event handler */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",ErrCode));
        return (TRUE);
    }


    EvtSubscribeParams.NotifyCallback   = JobCompletedHandler;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, Name,STBLIT_JOB_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Job completion : %d\n",ErrCode));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
    Job_Params.EventSubscriberID = SubscriberID;
#endif
#if defined (DVD_SECURED_CHIP)
    SecurityParams.SecuredAreaStart_p = (void *) (SECURED_PARTITION_START);
    SecurityParams.SecuredAreaStop_p  = (void *) (SECURED_PARTITION_STOP);
    SecurityParams.SecuredAreaNumber = 1;
    ErrCode = STBLIT_SetSecurityRange(Hdl, &SecurityParams);
#endif

    /* ---- Set Job 1 --------*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 1: %d\n",ErrCode));
        return (TRUE);
    }


    /* Set Bitmap */
    Bmp1.Data1_p=NULL;
    Bmp2.Data1_p=NULL;
    Bmp3.Data1_p=NULL;
    Bmp4.Data1_p=NULL;
    BmpT.Data1_p=NULL;
    Pal.Data_p=NULL;

#ifdef ST_OSLINUX
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",SECURED_SRC_BITMAP, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/tempo1.gam",SECURED_SRC_BITMAP, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/bateau_test1.gam",SECURED_SRC_BITMAP, &Bmp3, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/JO.GAM",SECURED_SRC_BITMAP, &Bmp4, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/BATEAU.GAM",SECURED_DST_BITMAP, &BmpT, &Pal);
#else

    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&Bmp1,&Pal);
    ErrCode = ConvertGammaToBitmap("../../tempo1.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&Bmp2,&Pal);
    ErrCode = ConvertGammaToBitmap("../../bateau_test1.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&Bmp3,&Pal);
    ErrCode = ConvertGammaToBitmap("../../JO.gam",SRC_BITMAP_AVMEM_PARTITION_NUM,&Bmp4,&Pal);
    ErrCode = ConvertGammaToBitmap("../../BATEAU.gam",DST_BITMAP_AVMEM_PARTITION_NUM,&BmpT,&Pal);

#endif


    /* Set Src1 */
    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src1.Data.Bitmap_p        = &Bmp1;
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 0;
    Src1.Rectangle.Width  = 150;
    Src1.Rectangle.Height  = 150;
    Src1.Palette_p            = NULL;

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Bmp2;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Rectangle.Width  = 150;
    Src2.Rectangle.Height  = 150;
    Src2.Palette_p            = NULL;

    /* Set Src3 */
    Src3.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src3.Data.Bitmap_p        = &Bmp3;
    Src3.Rectangle.PositionX  = 0;
    Src3.Rectangle.PositionY  = 0;
    Src3.Rectangle.Width  = 150;
    Src3.Rectangle.Height  = 150;
    Src3.Palette_p            = NULL;

    /* Set Src4 */
    Src4.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src4.Data.Bitmap_p        = &Bmp4;
    Src4.Rectangle.PositionX  = 0;
    Src4.Rectangle.PositionY  = 0;
    Src4.Rectangle.Width  = 150;
    Src4.Rectangle.Height  = 150;
    Src4.Palette_p            = NULL;

    SrcForeground.Type                 = STBLIT_SOURCE_TYPE_COLOR;
    SrcForeground.Data.Color_p         = &ColorFill;

    /* Set Dst */
    Dst1.Bitmap_p              = &BmpT;
    Dst1.Rectangle.PositionX   = 30;
    Dst1.Rectangle.PositionY   = 30;
    Dst1.Rectangle.Width  = 150;
    Dst1.Rectangle.Height  = 150;
    Dst1.Palette_p             = NULL;

    Dst2.Bitmap_p              = &BmpT;
    Dst2.Rectangle.PositionX   = 30;
    Dst2.Rectangle.PositionY   = 200;
    Dst2.Rectangle.Width  = 150;
    Dst2.Rectangle.Height  = 150;
    Dst2.Palette_p             = NULL;

    Dst3.Bitmap_p              = &BmpT;
    Dst3.Rectangle.PositionX   = 30;
    Dst3.Rectangle.PositionY   = 370;
    Dst3.Rectangle.Width  = 150;
    Dst3.Rectangle.Height  = 150;
    Dst3.Palette_p             = NULL;

    /* Set Main Context */
    Context.ColorKeyCopyMode            = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                     = STBLIT_ALU_COPY;
    Context.EnableMaskWord              = FALSE;
    Context.EnableMaskBitmap            = FALSE;
    Context.EnableColorCorrection       = FALSE;
    Context.Trigger.EnableTrigger       = FALSE;
    Context.GlobalAlpha                 = 50;
    Context.EnableFlickerFilter         = FALSE;
    Context.UserTag_p                   = NULL;
    Context.NotifyBlitCompletion        = FALSE;
    Context.JobHandle                   = Job1;


    /* Set Job Blit 1 Context */
    Context.EnableClipRectangle      = FALSE;
    Context.ClipRectangle            = ClipRectangle;
    Context.WriteInsideClipRectangle = TRUE;

    STBLIT_Blit(Hdl,NULL,&Src1,&Dst1,&Context );

    STBLIT_GetJobBlitHandle( Hdl, Job1, &JBHandle_1) ;

    /* Set Job Blit 2 Context */
    ClipRectangle.PositionX = 50;
    ClipRectangle.PositionY = 220;
    ClipRectangle.Width     = 80;
    ClipRectangle.Height    = 80;

    Context.EnableClipRectangle      = TRUE;
    Context.ClipRectangle            = ClipRectangle;
    Context.WriteInsideClipRectangle = FALSE;

    STBLIT_Blit(Hdl,NULL,&Src1,&Dst2,&Context );

    STBLIT_GetJobBlitHandle( Hdl, Job1, &JBHandle_2) ;

    /* Set Job Blit 3 Context */
    Context.EnableClipRectangle      = FALSE;
    Context.ClipRectangle            = ClipRectangle;
    Context.WriteInsideClipRectangle = TRUE;

    ColorFill.Type                 = STGXOBJ_COLOR_TYPE_RGB565 ;
    ColorFill.Value.RGB565.R     = 0x00 ;
    ColorFill.Value.RGB565.G     = 0x00 ;
    ColorFill.Value.RGB565.B     = 0x1F ;

    STBLIT_Blit(Hdl,NULL,&SrcForeground,&Dst3,&Context );

    STBLIT_GetJobBlitHandle( Hdl, Job1, &JBHandle_3) ;

    /* ---- 1st Passe--------*/

    /* SubmitJob */
    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }

    /* Wait for Job blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));

    /* ---- 2d Passe--------*/
    PositionRectangle1.PositionX   = 200;
    PositionRectangle1.PositionY   = 30;
    PositionRectangle1.Width       = 150;
    PositionRectangle1.Height      = 150;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle_1, STBLIT_DATA_TYPE_DESTINATION, &PositionRectangle1 );
    ErrCode = STBLIT_SetJobBlitBitmap   ( Hdl, JBHandle_1, STBLIT_DATA_TYPE_FOREGROUND , &Bmp2               );

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    ClipRectangle.PositionX = 220;
    ClipRectangle.PositionY = 220;
    ClipRectangle.Width     = 80;
    ClipRectangle.Height    = 80;

    PositionRectangle2.PositionX   = 220;
    PositionRectangle2.PositionY   = 200;
    PositionRectangle2.Width       = 150;
    PositionRectangle2.Height      = 150;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle_2, STBLIT_DATA_TYPE_DESTINATION, &PositionRectangle2 ) ;
    ErrCode = STBLIT_SetJobBlitClipRectangle(Hdl, JBHandle_2, TRUE,&ClipRectangle);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    ColorFill.Type                 = STGXOBJ_COLOR_TYPE_RGB565 ;
    ColorFill.Value.RGB565.R     = 0x00 ;
    ColorFill.Value.RGB565.G     = 0x3F ;
    ColorFill.Value.RGB565.B     = 0x00 ;

    PositionRectangle3.PositionX   = 200;
    PositionRectangle3.PositionY   = 370;
    PositionRectangle3.Width       = 150;
    PositionRectangle3.Height      = 150;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle_3, STBLIT_DATA_TYPE_DESTINATION, &PositionRectangle3 ) ;
    ErrCode = STBLIT_SetJobBlitColorFill( Hdl, JBHandle_3, STBLIT_DATA_TYPE_FOREGROUND , &ColorFill          );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    /* SubmitJob */
    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }

    /* Wait for Job blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));

    /* ---- 3d Passe--------*/

    PositionRectangle1.PositionX   = 370;
    PositionRectangle1.PositionY   = 30;
    PositionRectangle1.Width       = 150;
    PositionRectangle1.Height      = 150;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle_1, STBLIT_DATA_TYPE_DESTINATION, &PositionRectangle1 ) ;
    ErrCode = STBLIT_SetJobBlitBitmap   ( Hdl, JBHandle_1, STBLIT_DATA_TYPE_FOREGROUND, &Bmp3               );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    ClipRectangle.PositionX = 420;
    ClipRectangle.PositionY = 250;
    ClipRectangle.Width     = 80;
    ClipRectangle.Height    = 80;

    PositionRectangle2.PositionX   = 370;
    PositionRectangle2.PositionY   = 200;
    PositionRectangle2.Width       = 150;
    PositionRectangle2.Height      = 150;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle_2, STBLIT_DATA_TYPE_DESTINATION, &PositionRectangle2 ) ;
    ErrCode = STBLIT_SetJobBlitClipRectangle(Hdl, JBHandle_2, FALSE,&ClipRectangle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    ColorFill.Type                 = STGXOBJ_COLOR_TYPE_RGB565 ;
    ColorFill.Value.RGB565.R     = 0x1F ;
    ColorFill.Value.RGB565.G     = 0x00 ;
    ColorFill.Value.RGB565.B     = 0x00 ;

    PositionRectangle3.PositionX   = 370;
    PositionRectangle3.PositionY   = 370;
    PositionRectangle3.Width       = 150;
    PositionRectangle3.Height      = 150;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle_3, STBLIT_DATA_TYPE_DESTINATION, &PositionRectangle3 ) ;
    ErrCode = STBLIT_SetJobBlitColorFill( Hdl, JBHandle_3, STBLIT_DATA_TYPE_FOREGROUND , &ColorFill          );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    /* SubmitJob */
    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }

    /* Wait for Job blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));

    /* ---- 4d Passe--------*/
    PositionRectangle1.PositionX   = 540;
    PositionRectangle1.PositionY   = 30;
    PositionRectangle1.Width       = 150;
    PositionRectangle1.Height      = 150;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle_1, STBLIT_DATA_TYPE_DESTINATION, &PositionRectangle1 ) ;
    ErrCode = STBLIT_SetJobBlitBitmap   ( Hdl, JBHandle_1, STBLIT_DATA_TYPE_FOREGROUND, &Bmp4               );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    ClipRectangle.PositionX = 590;
    ClipRectangle.PositionY = 250;
    ClipRectangle.Width     = 80;
    ClipRectangle.Height    = 80;

    PositionRectangle2.PositionX   = 540;
    PositionRectangle2.PositionY   = 200;
    PositionRectangle2.Width       = 150;
    PositionRectangle2.Height      = 150;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle_2, STBLIT_DATA_TYPE_DESTINATION, &PositionRectangle2 ) ;
    ErrCode = STBLIT_SetJobBlitClipRectangle(Hdl, JBHandle_2, TRUE,&ClipRectangle);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    ColorFill.Type                 = STGXOBJ_COLOR_TYPE_RGB565 ;
    ColorFill.Value.RGB565.R     = 0x1F ;
    ColorFill.Value.RGB565.G     = 0x3F ;
    ColorFill.Value.RGB565.B     = 0x1F ;

    PositionRectangle3.PositionX   = 540;
    PositionRectangle3.PositionY   = 370;
    PositionRectangle3.Width       = 150;
    PositionRectangle3.Height      = 150;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle_3, STBLIT_DATA_TYPE_DESTINATION, &PositionRectangle3 ) ;
    ErrCode = STBLIT_SetJobBlitColorFill( Hdl, JBHandle_3, STBLIT_DATA_TYPE_FOREGROUND , &ColorFill          );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    /* SubmitJob */
    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }

    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));


    /* Generate Bitmap */
#ifdef ST_OSLINUX
    ErrCode = ConvertBitmapToGamma("./blt/result/test_job.gam", &BmpT, &Pal);
#else
    ErrCode = ConvertBitmapToGamma("../../../result/test_job.gam", &BmpT, &Pal);
#endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
        return (TRUE);
    }


    /* Delete Job */
    ErrCode = STBLIT_DeleteJob(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 1: %d\n",ErrCode));
        return (TRUE);
    }

    GUTIL_Free(Bmp1.Data1_p);
    GUTIL_Free(Bmp2.Data1_p);
    GUTIL_Free(Bmp3.Data1_p);
    GUTIL_Free(Bmp4.Data1_p);
    GUTIL_Free(BmpT.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    ErrCode = STBLIT_UserTaskTerm( JobCompletedTask, &(JobCompletedTask_Attribute) );


    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :JobCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    ErrCode = STEVT_Close(EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",ErrCode));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    Term_Params.ForceTerminate = TRUE;
    ErrCode = STBLIT_Term(Name,&Term_Params);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",ErrCode));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}


/*-----------------------------------------------------------------------------
 * Function : BLIT_TestSlice
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestSlice (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestSlice (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t                  Err;
    ST_DeviceName_t                 Name="Blitter\0";
    STAVMEM_BlockHandle_t           BlockHandle1;
    STAVMEM_BlockHandle_t           BlockHandle2;
    STAVMEM_FreeBlockParams_t       FreeBlockParams;

    STGXOBJ_Bitmap_t                Bitmap, TargetBitmap;
    STGXOBJ_Palette_t               Palette;
    STGXOBJ_BitmapAllocParams_t     Params1;
    STGXOBJ_BitmapAllocParams_t     Params2;
    STGXOBJ_Rectangle_t             FillRectangle;
    STGXOBJ_Color_t                 Color;
    STBLIT_InitParams_t             InitParams;
    STBLIT_OpenParams_t             OpenParams;
    STBLIT_TermParams_t             TermParams;
    STBLIT_Handle_t                 Handle;
    STBLIT_SliceRectangleBuffer_t   Buffer;
    STBLIT_SliceRectangle_t         SliceRectangle[100];
    STBLIT_SliceRectangle_t*        SliceRectangle_p;
    STBLIT_BlitContext_t            Context;
    STBLIT_SliceData_t              Dst,Src;
#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                   time;
    #endif
    #ifdef ST_OS20
		clock_t                     time;
    #endif
#endif
    BOOL                            TimeOut = FALSE;
    U32                             i;
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif



    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p             = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p  = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                 = 1;

    InitParams.SingleBlitNodeBufferUserAllocated = FALSE;
    InitParams.SingleBlitNodeMaxNumber    = 30;
/*    InitParams.SingleBlitNodeBuffer_p     = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated = FALSE;
    InitParams.JobBlitNodeMaxNumber       = 30;
/*    InitParams.JobBlitNodeBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated = FALSE;
    InitParams.JobMaxNumber               = 2;
/*    InitParams.JobBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber               = 30;
/*    InitParams.JobBlitBuffer_p                = NULL;*/

    InitParams.WorkBufferUserAllocated = FALSE;
/*    InitParams.WorkBuffer_p               = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }



    /* ------------ Process Blit operation ------------ */

    Bitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/grid2_rgb565.gam",0,&Bitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../grid2_rgb565.gam",0,&Bitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* Set Context */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* Initialize global semaphores */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* Open Event handler */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif

    /* Allocate TargetBitmap */
    TargetBitmap.ColorType              = Bitmap.ColorType;
/*    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_ARGB4444;*/
    TargetBitmap.BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = Bitmap.Width * 2;
    TargetBitmap.Height                 = Bitmap.Height * 2;
/*    TargetBitmap.Width                  = 200;*/
/*    TargetBitmap.Height                 = 200;*/

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    /* Set Color */
    Color.Type  = TargetBitmap.ColorType;
    Color.Value.ARGB1555.Alpha = 128;
    Color.Value.ARGB1555.R = 10;
    Color.Value.ARGB1555.G = 20;
    Color.Value.ARGB1555.B = 30;

    /* Set FillRectangle */
    FillRectangle.PositionX   = 0;
    FillRectangle.PositionY   = 0;
    FillRectangle.Width       = TargetBitmap.Width;
    FillRectangle.Height      = TargetBitmap.Height;

    Context.EnableClipRectangle     = TRUE;
    Context.ClipRectangle     = FillRectangle;

    Err = STBLIT_FillRectangle(Handle,&TargetBitmap,&FillRectangle,&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
    #if defined(ST_OSLINUX)
        semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT1"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
#else
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Fill Done"));
#endif


    /* Set Slice Buffer */
    Buffer.FixedRectangleType = STBLIT_SLICE_FIXED_RECTANGLE_TYPE_DESTINATION;
    Buffer.FixedRectangle.Width = 100;
    Buffer.FixedRectangle.Height = 100;
/*    Buffer.FixedRectangle.Height = TargetBitmap.Height;*/
    Buffer.FullSizeSrcRectangle.PositionX = 0;
    Buffer.FullSizeSrcRectangle.PositionY = 0;
    Buffer.FullSizeSrcRectangle.Width = Bitmap.Width;
    Buffer.FullSizeSrcRectangle.Height = Bitmap.Height;
    Buffer.FullSizeDstRectangle.PositionX = 0;
    Buffer.FullSizeDstRectangle.PositionY = 0;
    Buffer.FullSizeDstRectangle.Width  = TargetBitmap.Width;
    Buffer.FullSizeDstRectangle.Height  = TargetBitmap.Height;
    Buffer.SliceRectangleArray = SliceRectangle;
    /* Get Slices Number */
    Err = STBLIT_GetSliceRectangleNumber(Handle,&Buffer);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Slice Rectangle Number : %d\n",Err));
        return (TRUE);
    }

    /* set Slice rectangle buffer */
    Err = STBLIT_GetSliceRectangleBuffer(Handle,&Buffer);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Slice Rectangle : %d\n",Err));
        return (TRUE);
    }

    /* Slice Blit */
    Src.Bitmap_p        = &Bitmap;
    Src.Palette_p       = &Palette;
    Src.UseSliceRectanglePosition = TRUE;

    /* Set Dst */
    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Palette_p             = NULL;
    Dst.UseSliceRectanglePosition = TRUE;

    SliceRectangle_p = Buffer.SliceRectangleArray;
/*    SliceRectangle_p++;*/
    for (i=0;i < (Buffer.HorizontalSlicingNumber * Buffer.VerticalSlicingNumber) ;i++ )
    {

        Err = STBLIT_BlitSliceRectangle(Handle,NULL,&Src,&Dst,SliceRectangle_p,&Context);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
            return (TRUE);
        }
        SliceRectangle_p++;
#ifndef STBLIT_EMULATOR
    /*  Wait for Blit to be completed */
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
    #if defined(ST_OSLINUX)
        if (semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY)!=0)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ERROR"));
        }
    #endif
    #if defined(ST_OS21)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
			TimeOut = TRUE;
			break;
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
			TimeOut = TRUE;
			break;
		}
    #endif
    else
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));
#endif
    }

    if (TimeOut == FALSE)
    {
        /* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_BlitSlice.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_BlitSlice.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }

    /* Free Bitmap */
    GUTIL_Free(Bitmap.Data1_p);

    /* Free TargetBitmap */
#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif


    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestSliceReference
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestSliceReference (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestSliceReference (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t                  Err;
    ST_DeviceName_t                 Name="Blitter\0";
    STAVMEM_BlockHandle_t           BlockHandle1;
    STAVMEM_BlockHandle_t           BlockHandle2;
    STAVMEM_FreeBlockParams_t       FreeBlockParams;
    STGXOBJ_Bitmap_t                Bitmap, TargetBitmap;
    STGXOBJ_Palette_t               Palette;
    STGXOBJ_BitmapAllocParams_t     Params1;
    STGXOBJ_BitmapAllocParams_t     Params2;
    STGXOBJ_Rectangle_t             FillRectangle;
    STGXOBJ_Color_t                 Color;
    STBLIT_InitParams_t             InitParams;
    STBLIT_OpenParams_t             OpenParams;
    STBLIT_TermParams_t             TermParams;
    STBLIT_Handle_t                 Handle;
    STBLIT_BlitContext_t            Context;
    STBLIT_Source_t                 Source;
    STBLIT_Destination_t            Dest;
#ifndef STBLIT_EMULATOR
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                   time;
    #endif
    #ifdef ST_OS20
		clock_t                     time;
    #endif
#endif
    BOOL                            TimeOut = FALSE;
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               BlitCompletedTask;
    pthread_attr_t          BlitCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif



    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p             = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p  = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                 = 1;

    InitParams.SingleBlitNodeBufferUserAllocated = FALSE;
    InitParams.SingleBlitNodeMaxNumber    = 30;
/*    InitParams.SingleBlitNodeBuffer_p     = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated = FALSE;
    InitParams.JobBlitNodeMaxNumber       = 30;
/*    InitParams.JobBlitNodeBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated = FALSE;
    InitParams.JobMaxNumber               = 2;
/*    InitParams.JobBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber               = 30;
/*    InitParams.JobBlitBuffer_p                = NULL;*/

    InitParams.WorkBufferUserAllocated = FALSE;
/*    InitParams.WorkBuffer_p               = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }



    /* ------------ Process Blit operation ------------ */

    Bitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("./blt/files/grid2_rgb565.gam",0,&Bitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../grid2_rgb565.gam",0,&Bitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* Set Context */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
/*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
/*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;
    Context.EnableClipRectangle     = TRUE;

    /* Initialize global semaphores */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
        BlitCompletionTimeout = semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskCreate( (void*) BlitCompleted_Task,
            &(BlitCompletedTask),
            &(BlitCompletedTask_Attribute),
            NULL );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :BlitCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* Open Event handler */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
#endif


    /* ------------ Set source ------------ */

    Source.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Source.Data.Bitmap_p        = &Bitmap;
    Source.Rectangle.PositionX  = 0;
    Source.Rectangle.PositionY  = 0;
    Source.Rectangle.Width      = Bitmap.Width;
    Source.Rectangle.Height     = Bitmap.Height;
    Source.Palette_p            = &Palette;

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_RGB565;
    TargetBitmap.BitmapType             = Bitmap.BitmapType;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = Bitmap.Width /** 2*/;
    TargetBitmap.Height                 = Bitmap.Height /** 2*/;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(Handle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    Err = AllocBitmapBuffer (&TargetBitmap,SECURED_DST_BITMAP,(void*)TEST_SHARED_MEM_BASE_ADDRESS,
                             &Params1, &Params2);
#else
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[DST_BITMAP_AVMEM_PARTITION_NUM],
                             &BlockHandle1,&Params1, &BlockHandle2,&Params2);
#endif

    /* Set Color */
    Color.Type  = TargetBitmap.ColorType;
    Color.Value.ARGB1555.Alpha = 128;
    Color.Value.ARGB1555.R = 10;
    Color.Value.ARGB1555.G = 20;
    Color.Value.ARGB1555.B = 30;

    /* Set FillRectangle */
    FillRectangle.PositionX   = 0;
    FillRectangle.PositionY   = 0;
    FillRectangle.Width       = TargetBitmap.Width;
    FillRectangle.Height      = TargetBitmap.Height;

    Context.ClipRectangle     = FillRectangle;


    Dest.Bitmap_p              = &TargetBitmap;
    Dest.Rectangle.PositionX   = 0;
    Dest.Rectangle.PositionY   = 0;
    Dest.Rectangle.Width       = TargetBitmap.Width;
    Dest.Rectangle.Height      = TargetBitmap.Height;
    Dest.Palette_p             = NULL;

    Err = STBLIT_FillRectangle(Handle,&TargetBitmap,&FillRectangle,&Color,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
		}
    #endif
#else
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Fill Done"));
#endif

    Err = STBLIT_Blit(Handle,NULL,&Source,&Dest,&Context);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }

#ifndef STBLIT_EMULATOR
    /*  Wait for Blit to be completed */
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
			TimeOut = TRUE;
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
			ResetEngine();
			TimeOut = TRUE;
		}
    #endif
    else
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));
    }
#else
    /*  Wait for Blit to be completed */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit Done"));
#endif

    if (TimeOut == FALSE)
    {
        /* Generate Bitmap */
        #ifdef ST_OSLINUX
            Err = ConvertBitmapToGamma("./blt/result/test_BlitSliceref.gam",&TargetBitmap,&Palette);
        #else
            Err = ConvertBitmapToGamma("../../../result/test_BlitSliceref.gam",&TargetBitmap,&Palette);
        #endif
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
            return (TRUE);
        }
    }


    /* Free Bitmap */
    GUTIL_Free(Bitmap.Data1_p);

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,TargetBitmap.Data1_p );
#else
    /* Free TargetBitmap */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&BlockHandle1);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = STBLIT_UserTaskTerm( BlitCompletedTask, &(BlitCompletedTask_Attribute) );


    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :BlitCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif


    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestSetJobBlitClippingRectangle;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestJobBlitClippingRectangle (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestJobBlitClippingRectangle (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          ErrCode;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     Open_Params;
    STBLIT_TermParams_t     Term_Params;
    STBLIT_Handle_t         Hdl;
    STBLIT_Source_t         Src1, Src2;
    STBLIT_Destination_t    Dst;
    STBLIT_BlitContext_t    Context;
    STGXOBJ_Bitmap_t        Bmp1, Bmp2, Bmp3;
    STGXOBJ_Rectangle_t     Rectangle;
    STGXOBJ_Palette_t       Pal;

    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_JobParams_t      Job_Params;
    STBLIT_JobHandle_t      Job1;
    STBLIT_JobBlitHandle_t  JBHandle;
#ifndef STBLIT_EMULATOR
    int                     i;
    BOOL                    TimeOut = FALSE;
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t           time;
    #endif
    #ifdef ST_OS20
		clock_t             time;
    #endif
#endif
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               JobCompletedTask;
    pthread_attr_t          JobCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


    /* ------------ Blit Device Init ------------ */

    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                       = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 6;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    ErrCode = STBLIT_Init(Name,&InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    ErrCode = STBLIT_Open(Name,&Open_Params,&Hdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Process Blit operation ------------ */

    /* ----- Set Job Params ----*/
    Job_Params.NotifyJobCompletion = TRUE;

    /* Set Context */
    Context.ColorKeyCopyMode            = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                     = STBLIT_ALU_COPY;
    Context.EnableMaskWord              = FALSE;
    Context.EnableMaskBitmap            = FALSE;
    Context.EnableColorCorrection       = FALSE;
    Context.Trigger.EnableTrigger       = FALSE;
    Context.GlobalAlpha                 = 128;
    Context.EnableClipRectangle         = FALSE;
    Context.WriteInsideClipRectangle    = TRUE;
    Context.EnableFlickerFilter         = FALSE;
    Context.UserTag_p                   = NULL;
    Context.NotifyBlitCompletion        = FALSE;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		JobCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&JobCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    ErrCode = STBLIT_UserTaskCreate( (void*) JobCompleted_Task,
            &(JobCompletedTask),
            &(JobCompletedTask_Attribute),
            NULL );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :JobCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* Open Event handler */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",ErrCode));
        return (TRUE);
    }


    EvtSubscribeParams.NotifyCallback   = JobCompletedHandler;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, Name,STBLIT_JOB_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Job completion : %d\n",ErrCode));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
    Job_Params.EventSubscriberID = SubscriberID;
#endif

    /* Set Bitmap */

    Bmp1.Data1_p=NULL;
    Bmp2.Data1_p=NULL;
    Bmp3.Data1_p=NULL;
    Pal.Data_p=NULL;

#ifdef ST_OSLINUX
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/suzie.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp3, &Pal);
#else
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("../../suzie.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp3, &Pal);
#endif

    /* ---- Set Job 1 --------*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 1: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job1;

    if ( Bmp1.Width>Bmp2.Width )
    {
        Dst.Rectangle.Width = Bmp2.Width;
        Src1.Rectangle.Width = Bmp2.Width;
        Src2.Rectangle.Width = Bmp2.Width;
    }
    else
    {
        Dst.Rectangle.Width = Bmp1.Width;
        Src1.Rectangle.Width = Bmp1.Width;
        Src2.Rectangle.Width = Bmp1.Width;
    }

    if ( Bmp1.Height>Bmp2.Height )
    {
        Dst.Rectangle.Height = Bmp2.Height;
        Src1.Rectangle.Height = Bmp2.Height;
        Src2.Rectangle.Height = Bmp2.Height;
    }
    else
    {
        Dst.Rectangle.Height = Bmp1.Height;
        Src1.Rectangle.Height = Bmp1.Height;
        Src2.Rectangle.Height = Bmp1.Height;
    }

    /* Set Src1 */
    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
#ifdef ST_OSLINUX
    Src1.Data.Bitmap_p        = &Bmp3;
#else
    Src1.Data.Bitmap_p        = &Bmp1;
#endif
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 0;
    Src1.Palette_p            = NULL;

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Bmp2;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &Bmp1;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Palette_p             = NULL;

    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    STBLIT_GetJobBlitHandle( Hdl, Job1, &JBHandle) ;

    Rectangle.PositionX   = 0;
    Rectangle.PositionY   = 0;
    Rectangle.Width       = 20;
    Rectangle.Height      = 20;
    STBLIT_SetJobBlitClipRectangle( Hdl, JBHandle, TRUE, &Rectangle ) ;
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set Job Blit Clip rectangle : %d\n",ErrCode));
        return (TRUE);
    }

    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }


#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif

    for (i=0;i<1 ;i++ )
    {
        /* Wait for Job blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			if(semaphore_wait_timeout(JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "JOB DONE"));
        }
    }

    /* Generate Bitmap */
    if (TimeOut == FALSE)
    {
        #ifdef ST_OSLINUX
            ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_clip.gam", &Bmp1, &Pal);
        #else
            ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_clip.gam", &Bmp1, &Pal);
        #endif
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
            return (TRUE);
        }
    }
#else
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));

    /* Generate Bitmap */
    #ifdef ST_OSLINUX
        ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_clip.gam", &Bmp1, &Pal);
    #else
        ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_clip.gam", &Bmp1, &Pal);
    #endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
        return (TRUE);
    }
#endif


    /* Delete Job */
    ErrCode = STBLIT_DeleteJob(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 1: %d\n",ErrCode));
        return (TRUE);
    }

    GUTIL_Free(Bmp1.Data1_p);
    GUTIL_Free(Bmp2.Data1_p);
    GUTIL_Free(Bmp3.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    ErrCode = STBLIT_UserTaskTerm( JobCompletedTask, &(JobCompletedTask_Attribute) );


    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :JobCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    ErrCode = STEVT_Close(EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",ErrCode));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    Term_Params.ForceTerminate = TRUE;
    ErrCode = STBLIT_Term(Name,&Term_Params);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",ErrCode));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestSetJobBlitBitmap;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestJobBlitBitmap (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestJobBlitBitmap (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          ErrCode;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     Open_Params;
    STBLIT_TermParams_t     Term_Params;
    STBLIT_Handle_t         Hdl;
    STBLIT_Source_t         Src1, Src2;
    STBLIT_Destination_t    Dst;
    STBLIT_BlitContext_t    Context;
    STGXOBJ_Bitmap_t        Bmp1, Bmp2, Bmp3, Bmp4;
    STGXOBJ_Palette_t       Pal;

    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_JobParams_t      Job_Params;
    STBLIT_JobHandle_t      Job1;
    STBLIT_JobBlitHandle_t  JBHandle;
#ifndef STBLIT_EMULATOR
    int                     i;
    BOOL                    TimeOut = FALSE;
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t           time;
    #endif
    #ifdef ST_OS20
		clock_t             time;
    #endif
#endif
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               JobCompletedTask;
    pthread_attr_t          JobCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif



    /* ------------ Blit Device Init ------------ */

    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 6;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    ErrCode = STBLIT_Init(Name,&InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    ErrCode = STBLIT_Open(Name,&Open_Params,&Hdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Process Blit operation ------------ */

    /* ----- Set Job Params ----*/
    Job_Params.NotifyJobCompletion = TRUE;

    /* Set Context */
    Context.ColorKeyCopyMode            = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                     = STBLIT_ALU_COPY;
    Context.EnableMaskWord              = FALSE;
    Context.EnableMaskBitmap            = FALSE;
    Context.EnableColorCorrection       = FALSE;
    Context.Trigger.EnableTrigger       = FALSE;
    Context.GlobalAlpha                 = 128;
    Context.EnableClipRectangle         = FALSE;
    Context.WriteInsideClipRectangle    = TRUE;
    Context.EnableFlickerFilter         = FALSE;
    Context.UserTag_p                   = NULL;
    Context.NotifyBlitCompletion        = FALSE;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
        JobCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&JobCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    ErrCode = STBLIT_UserTaskCreate( (void*) JobCompleted_Task,
            &(JobCompletedTask),
            &(JobCompletedTask_Attribute),
            NULL );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :JobCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* Open Event handler */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",ErrCode));
        return (TRUE);
    }


    EvtSubscribeParams.NotifyCallback   = JobCompletedHandler;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, Name,STBLIT_JOB_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Job completion : %d\n",ErrCode));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
    Job_Params.EventSubscriberID = SubscriberID;
#endif

    /* Set Bitmap */

    Bmp1.Data1_p=NULL;
    Bmp2.Data1_p=NULL;
    Bmp3.Data1_p=NULL;
    Bmp4.Data1_p=NULL;
    Pal.Data_p=NULL;

#ifdef ST_OSLINUX
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp3, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/suzie.gam",0, &Bmp4, &Pal);
#else
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp3, &Pal);
    ErrCode = ConvertGammaToBitmap("../../suzie.gam",0, &Bmp4, &Pal);
#endif


    /* ---- Set Job 1 --------*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 1: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job1;

    if ( Bmp1.Width>Bmp4.Width )
    {
        Dst.Rectangle.Width = Bmp4.Width;
        Src1.Rectangle.Width = Bmp4.Width;
        Src2.Rectangle.Width = Bmp4.Width;
    }
    else
    {
        Dst.Rectangle.Width = Bmp1.Width;
        Src1.Rectangle.Width = Bmp1.Width;
        Src2.Rectangle.Width = Bmp1.Width;
    }

    if ( Bmp1.Height>Bmp4.Height )
    {
        Dst.Rectangle.Height = Bmp4.Height;
        Src1.Rectangle.Height = Bmp4.Height;
        Src2.Rectangle.Height = Bmp4.Height;
    }
    else
    {
        Dst.Rectangle.Height = Bmp1.Height;
        Src1.Rectangle.Height = Bmp1.Height;
        Src2.Rectangle.Height = Bmp1.Height;
    }

    /* Set Src1 */
    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
#ifdef ST_OSLINUX
    Src1.Data.Bitmap_p        = &Bmp3;
#else
    Src1.Data.Bitmap_p        = &Bmp1;
#endif
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 0;
    Src1.Palette_p            = NULL;

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Bmp2;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &Bmp1;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Palette_p             = NULL;

    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit: %d\n",ErrCode));
        return (TRUE);
    }

    STBLIT_GetJobBlitHandle( Hdl, Job1, &JBHandle) ;
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get job blit handle: %d\n",ErrCode));
        return (TRUE);
    }

    ErrCode = STBLIT_SetJobBlitBitmap( Hdl, JBHandle, STBLIT_DATA_TYPE_FOREGROUND, &Bmp4 ) ;
     if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set Job Blit bitmap : %d\n",ErrCode));
        return (TRUE);
    }

    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }


#ifndef STBLIT_EMULATOR
    #if defined(ST_OSLINUX)
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*50);
    #endif

    for (i=0;i<1 ;i++ )
    {
        /* Wait for Job blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
            if(semaphore_wait_timeout(JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
            }
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "JOB DONE"));
        }
    }
    /* Generate Bitmap */
    if (TimeOut == FALSE)
    {
#ifdef ST_OSLINUX
        ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_bitmap.gam", &Bmp1, &Pal);
#else
        ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_bitmap.gam", &Bmp1, &Pal);
#endif
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
            return (TRUE);
        }
    }
#else
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));

    /* Generate Bitmap */
    #ifdef ST_OSLIUNX
        ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_bitmap.gam", &Bmp1, &Pal);
    #else
        ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_bitmap.gam", &Bmp1, &Pal);
    #endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
        return (TRUE);
    }
#endif


    /* Delete Job */
    ErrCode = STBLIT_DeleteJob(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 1: %d\n",ErrCode));
        return (TRUE);
    }

    GUTIL_Free(Bmp1.Data1_p);
    GUTIL_Free(Bmp2.Data1_p);
    GUTIL_Free(Bmp3.Data1_p);
    GUTIL_Free(Bmp4.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    ErrCode = STBLIT_UserTaskTerm( JobCompletedTask, &(JobCompletedTask_Attribute) );


    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :JobCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    ErrCode = STEVT_Close(EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",ErrCode));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    Term_Params.ForceTerminate = TRUE;
    ErrCode = STBLIT_Term(Name,&Term_Params);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",ErrCode));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestSetJobBlitColorKey;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestJobBlitColorKey (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestJobBlitColorKey (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          ErrCode;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     Open_Params;
    STBLIT_TermParams_t     Term_Params;
    STBLIT_Handle_t         Hdl;
    STBLIT_Source_t         Src1, Src2;
    STBLIT_Destination_t    Dst;
    STBLIT_BlitContext_t    Context;
    STGXOBJ_Bitmap_t        Bmp1, Bmp2;
    STGXOBJ_Palette_t       Pal;
    STGXOBJ_ColorKey_t      ColorKey;
    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_JobParams_t      Job_Params;
    STBLIT_JobHandle_t      Job1;
    STBLIT_JobBlitHandle_t  JBHandle;
#ifndef STBLIT_EMULATOR
    int                     i;
    BOOL                    TimeOut = FALSE;
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               JobCompletedTask;
    pthread_attr_t          JobCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif
#ifdef ST_OSLINUX
    STGXOBJ_Bitmap_t        Bmp3;
#endif

    /* ------------ Blit Device Init ------------ */

    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 6;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    ErrCode = STBLIT_Init(Name,&InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    ErrCode = STBLIT_Open(Name,&Open_Params,&Hdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Process Blit operation ------------ */

    /* ----- Set Job Params ----*/
    Job_Params.NotifyJobCompletion = TRUE;

    /* Set Context */
    Context.ColorKeyCopyMode            = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                     = STBLIT_ALU_COPY;
    Context.EnableMaskWord              = FALSE;
    Context.EnableMaskBitmap            = FALSE;
    Context.EnableColorCorrection       = FALSE;
    Context.Trigger.EnableTrigger       = FALSE;
    Context.GlobalAlpha                 = 128;
    Context.EnableClipRectangle         = FALSE;
    Context.WriteInsideClipRectangle    = TRUE;
    Context.EnableFlickerFilter         = FALSE;
    Context.UserTag_p                   = NULL;
    Context.NotifyBlitCompletion        = FALSE;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		JobCompletionTimeout= semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&JobCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    ErrCode = STBLIT_UserTaskCreate( (void*) JobCompleted_Task,
            &(JobCompletedTask),
            &(JobCompletedTask_Attribute),
            NULL );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :JobCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* Open Event handler */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",ErrCode));
        return (TRUE);
    }


    EvtSubscribeParams.NotifyCallback   = JobCompletedHandler;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, Name,STBLIT_JOB_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Job completion : %d\n",ErrCode));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
    Job_Params.EventSubscriberID = SubscriberID;
#endif

    /* Set Bitmap */

    Bmp1.Data1_p=NULL;
    Bmp2.Data1_p=NULL;

    Pal.Data_p=NULL;
#ifdef ST_OSLINUX
    Bmp3.Data1_p=NULL;
    ErrCode = ConvertGammaToBitmap("./blt/files/merouARGB8888.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/suzieARGB8888.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/merouARGB8888.gam",0, &Bmp3, &Pal);
#else
    ErrCode = ConvertGammaToBitmap("../../merouARGB8888.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("../../suzieARGB8888.gam",0, &Bmp2, &Pal);
#endif

    /* ---- Set Job 1 --------*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 1: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job1;


    /* Set Src1 */
    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
#ifdef ST_OSLINUX
    Src1.Data.Bitmap_p        = &Bmp3;
#else
    Src1.Data.Bitmap_p        = &Bmp1;
#endif
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 0;
    Src1.Rectangle.Width      = 200;
    Src1.Rectangle.Height     = 200;

    Src1.Palette_p            = NULL;

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Bmp2;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Rectangle.Width      = 200;
    Src2.Rectangle.Height     = 200;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &Bmp1;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = 200;
    Dst.Rectangle.Height      = 200;
    Dst.Palette_p             = NULL;

    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    STBLIT_GetJobBlitHandle( Hdl, Job1, &JBHandle) ;

    ColorKey.Type = STGXOBJ_COLOR_KEY_TYPE_RGB888;
    ColorKey.Value.RGB888.RMin = 0xc8;
    ColorKey.Value.RGB888.RMax = 0xff;
    ColorKey.Value.RGB888.ROut = FALSE;
    ColorKey.Value.RGB888.REnable = TRUE;
    ColorKey.Value.RGB888.GMin = 0xc8;
    ColorKey.Value.RGB888.GMax = 0xff;
    ColorKey.Value.RGB888.GOut = FALSE;
    ColorKey.Value.RGB888.GEnable = TRUE;
    ErrCode = STBLIT_SetJobBlitColorKey( Hdl, JBHandle, STBLIT_COLOR_KEY_MODE_SRC, &ColorKey ) ;
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set Job Blit Color Key : %d\n",ErrCode));
        return (TRUE);
    }

    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }


#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
    for (i=0;i<1 ;i++ )
    {
        /* Wait for Job blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			if(semaphore_wait_timeout(JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "JOB DONE"));
        }
    }

    /* Generate Bitmap */
    if (TimeOut == FALSE)
    {
        #ifdef ST_OSLINUX
            ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_color_key.gam", &Bmp1, &Pal);
        #else
            ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_color_key.gam", &Bmp1, &Pal);
        #endif
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
            return (TRUE);
        }
    }
#else
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));

    /* Generate Bitmap */
    #ifdef ST_OSLINUX
        ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_color_key.gam", &Bmp1, &Pal);
    #else
        ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_color_key.gam", &Bmp1, &Pal);
    #endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
        return (TRUE);
    }
#endif


    /* Delete Job */
    ErrCode = STBLIT_DeleteJob(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 1: %d\n",ErrCode));
        return (TRUE);
    }

    GUTIL_Free(Bmp1.Data1_p);
    GUTIL_Free(Bmp2.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    ErrCode = STBLIT_UserTaskTerm( JobCompletedTask, &(JobCompletedTask_Attribute) );


    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :JobCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    ErrCode = STEVT_Close(EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",ErrCode));
        return (TRUE);
    }
#endif
    /* ------------ Blit Term ------------ */
    Term_Params.ForceTerminate = TRUE;
    ErrCode = STBLIT_Term(Name,&Term_Params);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",ErrCode));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestSetJobBlitMaskWord;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestJobBlitMaskWord (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestJobBlitMaskWord (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          ErrCode;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     Open_Params;
    STBLIT_TermParams_t     Term_Params;
    STBLIT_Handle_t         Hdl;
    STBLIT_Source_t         Src1, Src2;
    STBLIT_Destination_t    Dst;
    STBLIT_BlitContext_t    Context;
    STGXOBJ_Bitmap_t        Bmp1, Bmp2, Bmp3;
    STGXOBJ_Palette_t       Pal;
    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_JobParams_t      Job_Params;
    STBLIT_JobHandle_t      Job1;
    STBLIT_JobBlitHandle_t  JBHandle;
#ifndef STBLIT_EMULATOR
    int                     i;
    BOOL                    TimeOut = FALSE;
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t           time;
    #endif
    #ifdef ST_OS20
		clock_t             time;
    #endif
#endif
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               JobCompletedTask;
    pthread_attr_t          JobCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


    /* ------------ Blit Device Init ------------ */

    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 6;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    ErrCode = STBLIT_Init(Name,&InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    ErrCode = STBLIT_Open(Name,&Open_Params,&Hdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Process Blit operation ------------ */

    /* ----- Set Job Params ----*/
    Job_Params.NotifyJobCompletion = TRUE;

    /* Set Context */
    Context.ColorKeyCopyMode            = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                     = STBLIT_ALU_COPY;
    Context.EnableMaskWord              = TRUE;
    Context.MaskWord                    = 0xFFFF;
    Context.EnableMaskBitmap            = FALSE;
    Context.EnableColorCorrection       = FALSE;
    Context.Trigger.EnableTrigger       = FALSE;
    Context.GlobalAlpha                 = 128;
    Context.EnableClipRectangle         = FALSE;
    Context.WriteInsideClipRectangle    = TRUE;
    Context.EnableFlickerFilter         = FALSE;
    Context.UserTag_p                   = NULL;
    Context.NotifyBlitCompletion        = FALSE;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		JobCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&JobCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    ErrCode = STBLIT_UserTaskCreate( (void*) JobCompleted_Task,
            &(JobCompletedTask),
            &(JobCompletedTask_Attribute),
            NULL );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :JobCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* Open Event handler */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",ErrCode));
        return (TRUE);
    }


    EvtSubscribeParams.NotifyCallback   = JobCompletedHandler;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, Name,STBLIT_JOB_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Job completion : %d\n",ErrCode));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
    Job_Params.EventSubscriberID = SubscriberID;
#endif

    /* Set Bitmap */

    Bmp1.Data1_p=NULL;
    Bmp2.Data1_p=NULL;
    Bmp3.Data1_p=NULL;
    Pal.Data_p=NULL;
#ifdef ST_OSLINUX
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/suzie.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp3, &Pal);
#else
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("../../suzie.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp3, &Pal);
#endif
    /* ---- Set Job 1 --------*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 1: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job1;

    if ( Bmp1.Width>Bmp2.Width )
    {
        Dst.Rectangle.Width = Bmp2.Width;
        Src1.Rectangle.Width = Bmp2.Width;
        Src2.Rectangle.Width = Bmp2.Width;
    }
    else
    {
        Dst.Rectangle.Width = Bmp1.Width;
        Src1.Rectangle.Width = Bmp1.Width;
        Src2.Rectangle.Width = Bmp1.Width;
    }

    if ( Bmp1.Height>Bmp2.Height )
    {
        Dst.Rectangle.Height = Bmp2.Height;
        Src1.Rectangle.Height = Bmp2.Height;
        Src2.Rectangle.Height = Bmp2.Height;
    }
    else
    {
        Dst.Rectangle.Height = Bmp1.Height;
        Src1.Rectangle.Height = Bmp1.Height;
        Src2.Rectangle.Height = Bmp1.Height;
    }

    /* Set Src1 */
    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
#ifdef ST_OSLINUX
    Src1.Data.Bitmap_p        = &Bmp3;
#else
    Src1.Data.Bitmap_p        = &Bmp1;
#endif
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 0;
    Src1.Palette_p            = NULL;

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Bmp2;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &Bmp1;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Palette_p             = NULL;

    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    STBLIT_GetJobBlitHandle( Hdl, Job1, &JBHandle) ;

    ErrCode = STBLIT_SetJobBlitMaskWord( Hdl, JBHandle, 0x7E0) ;   /* Only copy Green component from RGB565 foreground */
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set Job Blit Mask Word : %d\n",ErrCode));
        return (TRUE);
    }


    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }


#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif

    for (i=0;i<1 ;i++ )
    {
        /* Wait for Job blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			if(semaphore_wait_timeout(JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "JOB DONE"));
        }
    }

    /* Generate Bitmap */
    if (TimeOut == FALSE)
    {
        #ifdef ST_OSLINUX
            ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_mask_word.gam", &Bmp1, &Pal);
        #else
            ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_mask_word.gam", &Bmp1, &Pal);
        #endif
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
            return (TRUE);
        }
    }
#else
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));

    /* Generate Bitmap */
    #ifdef ST_OSLINUX
        ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_mask_word.gam", &Bmp1, &Pal);
    #else
        ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_mask_word.gam", &Bmp1, &Pal);
    #endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
        return (TRUE);
    }
#endif


    /* Delete Job */
    ErrCode = STBLIT_DeleteJob(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 1: %d\n",ErrCode));
        return (TRUE);
    }

    GUTIL_Free(Bmp1.Data1_p);
    GUTIL_Free(Bmp2.Data1_p);
    GUTIL_Free(Bmp3.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    ErrCode = STBLIT_UserTaskTerm( JobCompletedTask, &(JobCompletedTask_Attribute) );


    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :JobCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    ErrCode = STEVT_Close(EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",ErrCode));
        return (TRUE);
    }
#endif
    /* ------------ Blit Term ------------ */
    Term_Params.ForceTerminate = TRUE;
    ErrCode = STBLIT_Term(Name,&Term_Params);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",ErrCode));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestSetJobBlitRectangle;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestJobBlitRectangle (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestJobBlitRectangle (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          ErrCode;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     Open_Params;
    STBLIT_TermParams_t     Term_Params;
    STBLIT_Handle_t         Hdl;
    STBLIT_Source_t         Src1, Src2;
    STBLIT_Destination_t    Dst;
    STBLIT_BlitContext_t    Context;
    STGXOBJ_Bitmap_t        Bmp1, Bmp2, Bmp3;
    STGXOBJ_Palette_t       Pal;
    STGXOBJ_Rectangle_t     Rectangle;
    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_JobParams_t      Job_Params;
    STBLIT_JobHandle_t      Job1;
    STBLIT_JobBlitHandle_t  JBHandle;
#ifndef STBLIT_EMULATOR
    int                     i;
    BOOL                    TimeOut = FALSE;
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               JobCompletedTask;
    pthread_attr_t          JobCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


    /* ------------ Blit Device Init ------------ */

    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                      = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                      = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 6;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    ErrCode = STBLIT_Init(Name,&InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    ErrCode = STBLIT_Open(Name,&Open_Params,&Hdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Process Blit operation ------------ */

    /* ----- Set Job Params ----*/
    Job_Params.NotifyJobCompletion = TRUE;

    /* Set Context */
    Context.ColorKeyCopyMode            = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                     = STBLIT_ALU_COPY;
    Context.EnableMaskWord              = FALSE;
    Context.EnableMaskBitmap            = FALSE;
    Context.EnableColorCorrection       = FALSE;
    Context.Trigger.EnableTrigger       = FALSE;
    Context.GlobalAlpha                 = 128;
    Context.EnableClipRectangle         = FALSE;
    Context.WriteInsideClipRectangle    = TRUE;
    Context.EnableFlickerFilter         = FALSE;
    Context.UserTag_p                   = NULL;
    Context.NotifyBlitCompletion        = FALSE;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		JobCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&JobCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    ErrCode = STBLIT_UserTaskCreate( (void*) JobCompleted_Task,
            &(JobCompletedTask),
            &(JobCompletedTask_Attribute),
            NULL );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :JobCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* Open Event handler */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",ErrCode));
        return (TRUE);
    }


    EvtSubscribeParams.NotifyCallback   = JobCompletedHandler;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, Name,STBLIT_JOB_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Job completion : %d\n",ErrCode));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
    Job_Params.EventSubscriberID = SubscriberID;
#endif

    /* Set Bitmap */

    Bmp1.Data1_p=NULL;
    Bmp2.Data1_p=NULL;
    Bmp3.Data1_p=NULL;
    Pal.Data_p=NULL;
#ifdef ST_OSLINUX
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/suzie.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp3, &Pal);
#else
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("../../suzie.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp3, &Pal);
#endif

    /* ---- Set Job 1 --------*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 1: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job1;

 /*   if ( Bmp1.Width>Bmp2.Width )
    {
        Dst.Rectangle.Width = Bmp2.Width;
        Src1.Rectangle.Width = Bmp2.Width;
        Src2.Rectangle.Width = Bmp2.Width;
    }
    else
    {
        Dst.Rectangle.Width = Bmp1.Width;
        Src1.Rectangle.Width = Bmp1.Width;
        Src2.Rectangle.Width = Bmp1.Width;
    }

    if ( Bmp1.Height>Bmp2.Height )
    {
        Dst.Rectangle.Height = Bmp2.Height;
        Src1.Rectangle.Height = Bmp2.Height;
        Src2.Rectangle.Height = Bmp2.Height;
    }
    else
    {
        Dst.Rectangle.Height = Bmp1.Height;
        Src1.Rectangle.Height = Bmp1.Height;
        Src2.Rectangle.Height = Bmp1.Height;
    } */

    /* Set Src1 */
    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src1.Data.Bitmap_p        = &Bmp1;
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 0;
    Src1.Rectangle.Width  = 100;
    Src1.Rectangle.Height  = 100;
    Src1.Palette_p            = NULL;

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Bmp2;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Rectangle.Width  = 100;
    Src2.Rectangle.Height  = 100;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &Bmp3;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width  = 100;
    Dst.Rectangle.Height  = 100;
    Dst.Palette_p             = NULL;

    STBLIT_Blit(Hdl,&Src1,&Src2,&Dst,&Context );

    STBLIT_GetJobBlitHandle( Hdl, Job1, &JBHandle) ;

    Rectangle.PositionX   = 50;
    Rectangle.PositionY   = 50;
    Rectangle.Width       = 100;
    Rectangle.Height      = 100;

    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle, STBLIT_DATA_TYPE_DESTINATION, &Rectangle ) ;
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }
    ErrCode = STBLIT_SetJobBlitRectangle( Hdl, JBHandle, STBLIT_DATA_TYPE_DESTINATION, &Rectangle ) ;
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job : %d\n",ErrCode));
        return (TRUE);
    }

    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }


#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif

    for (i=0;i<1 ;i++ )
    {
        /* Wait for Job blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			if(semaphore_wait_timeout(JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "JOB DONE"));
        }
    }

    /* Generate Bitmap */
    if (TimeOut == FALSE)
    {
        #ifdef ST_OSLINUX
            ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_rectangle.gam", &Bmp3, &Pal);
        #else
            ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_rectangle.gam", &Bmp3, &Pal);
        #endif
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
            return (TRUE);
        }
    }
#else
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));

    /* Generate Bitmap */
    #ifdef ST_OSLINUX
        ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_rectangle.gam", &Bmp3, &Pal);
    #else
        ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_rectangle.gam", &Bmp3, &Pal);
    #endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
        return (TRUE);
    }
#endif


    /* Delete Job */
    ErrCode = STBLIT_DeleteJob(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 1: %d\n",ErrCode));
        return (TRUE);
    }

    GUTIL_Free(Bmp1.Data1_p);
    GUTIL_Free(Bmp2.Data1_p);
    GUTIL_Free(Bmp3.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    ErrCode = STBLIT_UserTaskTerm( JobCompletedTask, &(JobCompletedTask_Attribute) );


    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :JobCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    ErrCode = STEVT_Close(EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",ErrCode));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    Term_Params.ForceTerminate = TRUE;
    ErrCode = STBLIT_Term(Name,&Term_Params);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",ErrCode));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : BLIT_TestSetJobBlitColorFill;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestJobBlitColorFill (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestJobBlitColorFill (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          ErrCode;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     Open_Params;
    STBLIT_TermParams_t     Term_Params;
    STBLIT_Handle_t         Hdl;
    STBLIT_Source_t         Src1, Src2;
    STBLIT_Destination_t    Dst;
    STBLIT_BlitContext_t    Context;
    STGXOBJ_Bitmap_t        Bmp1, Bmp2, Bmp3;
    STGXOBJ_Palette_t       Pal;
    STGXOBJ_Rectangle_t       Rectangle;
    STGXOBJ_Color_t       Color;
    ST_DeviceName_t         Name="Blitter\0";
    STBLIT_JobParams_t      Job_Params;
    STBLIT_JobHandle_t      Job1;
    STBLIT_JobBlitHandle_t  JBHandle;
#ifndef STBLIT_EMULATOR
    int                     i;
    BOOL                    TimeOut = FALSE;
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		osclock_t                 time;
    #endif
    #ifdef ST_OS20
		clock_t                 time;
    #endif
#endif
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    pthread_t               JobCompletedTask;
    pthread_attr_t          JobCompletedTask_Attribute;   /* Task attributes */
#else
    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
#endif


    /* ------------ Blit Device Init ------------ */

    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                       = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 6;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    ErrCode = STBLIT_Init(Name,&InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    ErrCode = STBLIT_Open(Name,&Open_Params,&Hdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* ------------ Process Blit operation ------------ */

    /* ----- Set Job Params ----*/
    Job_Params.NotifyJobCompletion = TRUE;

    /* Set Context */
    Context.ColorKeyCopyMode            = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                     = STBLIT_ALU_COPY;
    Context.EnableMaskWord              = FALSE;
    Context.EnableMaskBitmap            = FALSE;
    Context.EnableColorCorrection       = FALSE;
    Context.Trigger.EnableTrigger       = FALSE;
    Context.GlobalAlpha                 = 128;
    Context.EnableClipRectangle         = FALSE;
    Context.WriteInsideClipRectangle    = TRUE;
    Context.EnableFlickerFilter         = FALSE;
    Context.UserTag_p                   = NULL;
    Context.NotifyBlitCompletion        = FALSE;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		JobCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&JobCompletionTimeout, 0);
    #endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    ErrCode = STBLIT_UserTaskCreate( (void*) JobCompleted_Task,
            &(JobCompletedTask),
            &(JobCompletedTask_Attribute),
            NULL );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :JobCompleted_Task\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* Open Event handler */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",ErrCode));
        return (TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",ErrCode));
        return (TRUE);
    }


    EvtSubscribeParams.NotifyCallback   = JobCompletedHandler;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, Name,STBLIT_JOB_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Job completion : %d\n",ErrCode));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
    Job_Params.EventSubscriberID = SubscriberID;
#endif

    /* Set Bitmap */

    Bmp1.Data1_p=NULL;
    Bmp2.Data1_p=NULL;
    Bmp3.Data1_p=NULL;
    Pal.Data_p=NULL;
#ifdef ST_OSLINUX
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/suzie.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("./blt/files/merouRGB565.gam",0, &Bmp3, &Pal);
#else
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp1, &Pal);
    ErrCode = ConvertGammaToBitmap("../../suzie.gam",0, &Bmp2, &Pal);
    ErrCode = ConvertGammaToBitmap("../../merouRGB565.gam",0, &Bmp3, &Pal);
#endif

    /* ---- Set Job 1 --------*/
    ErrCode = STBLIT_CreateJob(Hdl,&Job_Params,&Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Create job 1: %d\n",ErrCode));
        return (TRUE);
    }
    Context.JobHandle               = Job1;

    if ( Bmp1.Width>Bmp2.Width )
    {
        Dst.Rectangle.Width = Bmp2.Width;
        Src1.Rectangle.Width = Bmp2.Width;
        Src2.Rectangle.Width = Bmp2.Width;
    }
    else
    {
        Dst.Rectangle.Width = Bmp1.Width;
        Src1.Rectangle.Width = Bmp1.Width;
        Src2.Rectangle.Width = Bmp1.Width;
    }

    if ( Bmp1.Height>Bmp2.Height )
    {
        Dst.Rectangle.Height = Bmp2.Height;
        Src1.Rectangle.Height = Bmp2.Height;
        Src2.Rectangle.Height = Bmp2.Height;
    }
    else
    {
        Dst.Rectangle.Height = Bmp1.Height;
        Src1.Rectangle.Height = Bmp1.Height;
        Src2.Rectangle.Height = Bmp1.Height;
    }

    /* Set Src1 */
    Src1.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src1.Data.Bitmap_p        = &Bmp1;
    Src1.Rectangle.PositionX  = 0;
    Src1.Rectangle.PositionY  = 0;
    Src1.Palette_p            = NULL;

    /* Set Src2 */
    Src2.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src2.Data.Bitmap_p        = &Bmp2;
    Src2.Rectangle.PositionX  = 0;
    Src2.Rectangle.PositionY  = 0;
    Src2.Palette_p            = NULL;

    /* Set Dst */
    Dst.Bitmap_p              = &Bmp1;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Palette_p             = NULL;

    /* ------------ Set Color ------------ */
    Color.Type                 = STGXOBJ_COLOR_TYPE_RGB565 ;
    Color.Value.RGB565.R     = 0x00 ;
    Color.Value.RGB565.G     = 0x00 ;
    Color.Value.RGB565.B     = 0xFF ;

    /* ------------ Set Rectangle ------------ */
    Rectangle.PositionX = 20 ;
    Rectangle.PositionY = 20 ;
    Rectangle.Width     = 180 ;
    Rectangle.Height    = 180 ;

    STBLIT_FillRectangle(Hdl,&Bmp3,&Rectangle,&Color,&Context );

    STBLIT_GetJobBlitHandle( Hdl, Job1, &JBHandle) ;

    /* ------------ Set Color ------------ */
    Color.Type                 = STGXOBJ_COLOR_TYPE_RGB565 ;
    Color.Value.RGB565.R     = 0xff ;
    Color.Value.RGB565.G     = 0x00 ;
    Color.Value.RGB565.B     = 0x00 ;

    ErrCode = STBLIT_SetJobBlitColorFill( Hdl, JBHandle, STBLIT_DATA_TYPE_FOREGROUND, &Color ) ;
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Set job blit color fill : %d\n",ErrCode));
        return (TRUE);
    }

    ErrCode = STBLIT_SubmitJobFront(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Submit job : %d\n",ErrCode));
        return (TRUE);
    }


#ifndef STBLIT_EMULATOR
    #ifdef ST_OSLINUX
        time = time_plus(time_now(), 15625*TEST_DELAY);
    #else
        time = time_plus(time_now(), 15625*5);
    #endif
    for (i=0;i<1 ;i++ )
    {
        /* Wait for Job blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			if(semaphore_wait_timeout(JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&JobCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
				TimeOut = TRUE;
				break;
			}
        #endif
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "JOB DONE"));
        }
    }

    /* Generate Bitmap */
    if (TimeOut == FALSE)
    {
        #ifdef ST_OSLINUX
            ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_colorfill.gam", &Bmp3, &Pal);
        #else
            ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_colorfill.gam", &Bmp3, &Pal);
        #endif
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
            return (TRUE);
        }
    }
#else
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait_timeout(JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    #ifdef ST_OS20
		semaphore_wait_timeout(&JobCompletionTimeout, TIMEOUT_INFINITY);
    #endif
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Job Done"));

    /* Generate Bitmap */
    #ifdef ST_OSLINUX
        ErrCode = ConvertBitmapToGamma("./blt/result/test_set_job_colorfill.gam", &Bmp3, &Pal);
    #else
        ErrCode = ConvertBitmapToGamma("../../../result/test_set_job_colorfill.gam", &Bmp3, &Pal);
    #endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
        return (TRUE);
    }
#endif


    /* Delete Job */
    ErrCode = STBLIT_DeleteJob(Hdl, Job1);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err delete job 1: %d\n",ErrCode));
        return (TRUE);
    }

    GUTIL_Free(Bmp1.Data1_p);
    GUTIL_Free(Bmp2.Data1_p);
    GUTIL_Free(Bmp3.Data1_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    ErrCode = STBLIT_UserTaskTerm( JobCompletedTask, &(JobCompletedTask_Attribute) );


    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :JobCompletedTask\n" ));
        return(ST_ERROR_BAD_PARAMETER);
    }
#else
    /* --------------- Close Evt ----------*/
    ErrCode = STEVT_Close(EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",ErrCode));
        return (TRUE);
    }
#endif

    /* ------------ Blit Term ------------ */
    Term_Params.ForceTerminate = TRUE;
    ErrCode = STBLIT_Term(Name,&Term_Params);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",ErrCode));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

#ifdef STBLIT_TEST_STACK

 typedef struct
{
    STBLIT_Handle_t          Handle;
    STBLIT_Source_t*         Src2_p;
    STBLIT_Destination_t*    Dst_p;
    STBLIT_BlitContext_t*    Context_p;
} BenchmarkData_t;

#define  BENCHMARK_TASK_STACK_SIZE  1024 * 16
    #ifdef ST_OSLINUX
        pthread_t                   *BenchmarkTask;
        static pthread_attr_t       BENCHMARK_TaskAttribute;
    #endif

    #ifdef ST_OS21
		task_t*   BenchmarkTask;
    #endif
    #ifdef ST_OS20
		task_t   BenchmarkTask;
		tdesc_t  BenchmarkTaskDesc;
    #endif
int      StackBenchmarkTask[BENCHMARK_TASK_STACK_SIZE/ sizeof(int)];

/*-----------------------------------------------------------------------------
 * Function : MyBenchmarkTask (High Priority task)
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
static void MyBenchmarkTask(void* data_p)
{
    BenchmarkData_t* BenchmarkData_p = (BenchmarkData_t*)data_p;

    STBLIT_Blit(BenchmarkData_p->Handle,NULL,
                BenchmarkData_p->Src2_p,BenchmarkData_p->Dst_p,
                BenchmarkData_p->Context_p );
}



 /*-----------------------------------------------------------------------------
 * Function : BLIT_TestStackBlit;
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
#if defined(ST_OS21) || defined(ST_OSLINUX)
	BOOL BLIT_TestStackBlit (parse_t *pars_p, char *result_sym_p)
#endif
#ifdef ST_OS20
	boolean BLIT_TestStackBlit (parse_t *pars_p, char *result_sym_p)
#endif
{
    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Foreground;
    STBLIT_Destination_t    Dst;

    STGXOBJ_Bitmap_t        ForegroundBitmap,MaskBitmap,TargetBitmap;
    STGXOBJ_Palette_t       Palette;

    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

#ifdef ST_OSLINUX
    STLAYER_AllocDataParams_t    AllocDataParams;
#else
    STAVMEM_BlockHandle_t  Work_Handle;
    STAVMEM_AllocBlockParams_t  AllocBlockParams; /* Allocation param*/
    STAVMEM_MemoryRange_t  RangeArea[2];
    STAVMEM_FreeBlockParams_t FreeBlockParams;
#endif
    STEVT_SubscriberID_t    SubscriberID;

	#ifndef STBLIT_EMULATOR
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			osclock_t       time;
        #endif
        #ifdef ST_OS20
			clock_t         time;
        #endif
	#endif
    U8                      NbForbiddenRange;
    U32                     i;
    BenchmarkData_t         BenchmarkData;
    task_t*                 BenchmarkTaskList[1];
    task_status_t status;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
#ifdef ST_OSLINUX
    InitParams.AVMEMPartition                       = (STAVMEM_PartitionHandle_t)NULL;
#else
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
#endif
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

#ifdef ST_OSLINUX
    layer_init();
#endif

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }

    /* Init NodeID */
    for (i=0;i<20;i++)
    {
        NodeID[i] = 0;
    }
    Index = 0;

    /* ------------ Initialize global semaphores ------------ */
    #if defined(ST_OS21) || defined(ST_OSLINUX)
		BlitCompletionTimeout=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;

    /* ------------ Set Foreground and Mask------------ */
    ForegroundBitmap.Data1_p = NULL;
    MaskBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;

#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("../../merouARGB8888.gam",0,&ForegroundBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../merouargb8888.gam",0,&ForegroundBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }


#ifdef ST_OSLINUX
    Err = ConvertGammaToBitmap("../../mmask.gam",0,&MaskBitmap,&Palette);
#else
    Err = ConvertGammaToBitmap("../../mmask.gam",0,&MaskBitmap,&Palette);
#endif
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Foreground.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Foreground.Data.Bitmap_p        = &ForegroundBitmap;
    Foreground.Rectangle.PositionX  = 0;
    Foreground.Rectangle.PositionY  = 0;
    Foreground.Rectangle.Width      = MaskBitmap.Width;
    Foreground.Rectangle.Height     = MaskBitmap.Height;
    Foreground.Palette_p            = &Palette;

    /* ------------ Set Dst ------------ */

    Err = ConvertGammaToBitmap("../../crow.gam",0,&TargetBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle             = Foreground.Rectangle;
    Dst.Palette_p             = NULL;

#ifdef ST_OSLINUX

    AllocDataParams.Alignment = 16;
    AllocDataParams.Size = 256*4;
    Err = STLAYER_AllocData( FakeLayerHndl, &AllocDataParams, (void**)&(Context.WorkBuffer_p) );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }


#else

    /* ------------- Allocation Work buffer ---------------*/
    NbForbiddenRange = 1;
    if (VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
         VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) +
                                             (U32)(VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange= 2;
    }
    AllocBlockParams.PartitionHandle            = AvmemPartitionHandle[0];
    AllocBlockParams.AllocMode                  = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders   = 0;
    AllocBlockParams.ForbiddenBorderArray_p     = NULL;
    AllocBlockParams.Alignment = 16;

    AllocBlockParams.Size = ForegroundBitmap.Size1;

    Err = STAVMEM_AllocBlock(&AllocBlockParams,&Work_Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
    STAVMEM_GetBlockAddress(Work_Handle,(void**)&(Context.WorkBuffer_p));

#endif

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;

    Context.EnableMaskBitmap        = TRUE;
    Context.MaskBitmap_p            = &MaskBitmap;
    Context.MaskRectangle           = Foreground.Rectangle;

    Context.EnableColorCorrection   = FALSE;
/*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
/*    Context.ClipRectangle           = Dst.Rectangle;*/
/*    Context.WriteInsideClipRectangle = TRUE;*/
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
/*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    /* Set Benchmark Data */
    BenchmarkData.Handle        = Handle;
    BenchmarkData.Src2_p        = &Foreground;
    BenchmarkData.Dst_p         = &Dst;
    BenchmarkData.Context_p     = &Context;


    #ifdef ST_OSLINUX
        pthread_attr_init(&BENCHMARK_TaskAttribute);
        if (pthread_create(&BenchmarkTask, &BENCHMARK_TaskAttribute, (void(*)(void*))MyBenchmarkTask, (void*)&BenchmarkData))
        {
            RetErr = TRUE;
        }
        else
        {
            printf("STBLIT - BENCHMARK : created task.\n");
        }
    #endif

    /* Create High priority task */
    #ifdef ST_OS21
		task_create ((void(*)(void*))MyBenchmarkTask,
				(void*)&BenchmarkData,
				StackBenchmarkTask, BENCHMARK_TASK_STACK_SIZE,
				0,
				"BenchmarkTask",task_flags_high_priority_process);
    #endif
    #ifdef ST_OS20
		task_init ((void(*)(void*))MyBenchmarkTask,
				(void*)&BenchmarkData,
				StackBenchmarkTask, BENCHMARK_TASK_STACK_SIZE,
				&(BenchmarkTask), &(BenchmarkTaskDesc),
				0,
				"BenchmarkTask",task_flags_high_priority_process);
    #endif


    /* delete tasks */
    BenchmarkTaskList[0] = (task_t*)&BenchmarkTask;
    task_wait((task_t**)&BenchmarkTaskList,1,TIMEOUT_INFINITY);


	#ifndef STBLIT_EMULATOR
		time = time_plus(time_now(), 15625*10);
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			if(semaphore_wait_timeout(BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();
			}
        #endif
        #ifdef ST_OS20
			if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
			{
				STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
				ResetEngine();

			}
        #endif
		else
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit done"));

			/* Dump stack usage */
            #if defined(ST_OS21) || defined(ST_OSLINUX)
				task_status(BenchmarkTask, &status, 1);

				task_delete(BenchmarkTask);
            #endif
            #ifdef ST_OS20
				task_status(&BenchmarkTask, &status, 1);

				task_delete(&BenchmarkTask);
            #endif

			/* Generate Bitmap */
			Err = ConvertBitmapToGamma("Mask_bitmap.gam",&TargetBitmap,&Palette);
			if (Err != ST_NO_ERROR)
			{
				STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
				return (TRUE);
			}
		}
	#else
		/*  Wait for Blit to be completed */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			semaphore_wait_timeout(BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
        #ifdef ST_OS20
			semaphore_wait_timeout(&BlitCompletionTimeout, TIMEOUT_INFINITY);
        #endif
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Blit done"));

		/* Dump stack usage */
        #if defined(ST_OS21) || defined(ST_OSLINUX)
			task_status(BenchmarkTask, &status, 1);

			task_delete(BenchmarkTask);
        #endif
        #ifdef ST_OS20
			task_status(&BenchmarkTask, &status, 1);

			task_delete(&BenchmarkTask);
        #endif

		/* Generate Bitmap */
		Err = ConvertBitmapToGamma("Mask_bitmap.gam",&TargetBitmap,&Palette);
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Convert Gamma to Bitmap : %d\n",Err));
			return (TRUE);
		}
	#endif

    /* print status */
    printf("Blit stack = %d bytes used \n",status.task_stack_used);

    /* --------------- Free Bitmap ------------ */
    GUTIL_Free(MaskBitmap.Data1_p);
    GUTIL_Free(TargetBitmap.Data1_p);
    GUTIL_Free(ForegroundBitmap.Data1_p);

#ifdef ST_OSLINUX
    STLAYER_FreeData( FakeLayerHndl,Context.WorkBuffer_p );
#else
    /* Free Work buffer */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&Work_Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
#endif

    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Close : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }

#ifdef ST_OSLINUX
    layer_term();
#endif

    return(FALSE);
}

#endif

