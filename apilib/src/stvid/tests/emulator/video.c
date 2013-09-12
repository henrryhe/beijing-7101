/*******************************************************************************

File name   :

Description :

COPYRIGHT (C) STMicroelectronics 2003.

   Date                     Modification                            Name
   ----                     ------------                            ----
 Nov 2003            Created                                         MB
 Jun 2004            STi7710 support modification                    GG

*******************************************************************************/


/* Global compilation flags. ------------------------------------------------ */
#ifdef ST_7710
# define TEST_HD
# define USE_BEIO
#endif

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "stddefs.h"
#include "stdevice.h"
#include "stvid.h"
#include "clavmem.h"
#include "clevt.h"
#include "testtool.h"
#include "stvmix.h"
#include "stlayer.h"
#include "debug.h"
#include "vid_cmd2.h"
#ifdef USE_BEIO
#include "stdenc.h"
#include "stvtg.h"
#include "stvout.h"
#endif /* USE_BEIO */

#include "vid_util.h"

/* --- Constants (default values) -------------------------------------- */
/* Default bit buffer size values */
#define DEFAULT_BIT_BUFFER_SIZE_SD    380928
#define DEFAULT_BIT_BUFFER_SIZE_HD   2105344

#ifdef ST_7710
#define BIT_BUFFER_SIZE DEFAULT_BIT_BUFFER_SIZE_HD
#else
#define BIT_BUFFER_SIZE DEFAULT_BIT_BUFFER_SIZE_SD
#endif

/* Limit in bit buffer to keep free. */
#define BIT_BUFFER_THRESHOLD    (BIT_BUFFER_SIZE/2)
#define MAX_TRANSFER_SIZE       (4*1024) /* Have to be less than Input buffer Size (90Kbytes).  */


/* --- Global variables ------------------------------------------------ */

/* Variables ---------------------------------------------------------------- */

extern ST_Partition_t *DriverPartition_p;
extern ST_Partition_t *DataPartition_p;

/* Transfer management variables.   */
void * VideoInputBuffWptr_p = NULL;
void * VideoInputBuffRptr_p = NULL;
void * VideoInputBuffBase_p = NULL;
void * VideoInputBuffTop_p  = NULL;
U32    VideoInputBufferSize = 0;

void * UserBufferBase_p     = NULL;
void * UserBufferTop_p      = NULL;
void * UserBufferRPtr_p     = NULL;
long int UserBufferSize     = 0;

/* Overall STVID access handle.     */
STVID_Handle_t  VideoHandle;

#ifdef USE_BEIO
STDENC_Handle_t DencHandle;
STVOUT_Handle_t VoutHandle;
STVTG_Handle_t  VtgHandle;
STVMIX_Handle_t VmixHandle;
#endif /* USE_BEIO */

/* Injection task characteristics.  */
#define TASK_STACK          3000
static U8                   Stack[TASK_STACK];
static task_t               Task;
static tdesc_t              Desc;

/* --- Prototype ------------------------------------------------------- */


/* --- Externals ------------------------------------------------------- */
extern void Init_Stub(void);


/* Functions ---------------------------------------------------------------- */

/*#######################################################################*/
/*##################### INJECTION MANAGEMENT ############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : GetWriteAddress
 *            Generic shared function with video driver that allows it to
 *              retrieve write pointer in its InputBuffer.
 * Input    : Handle, Address_p pointer address.
 * Output   :
 * Return   : ST_NO_ERROR
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t GetWriteAddress (void * const Handle,
                                         void ** const Address_p)
{
    *Address_p = VideoInputBuffWptr_p;
     return(ST_NO_ERROR);
}
/*-------------------------------------------------------------------------
 * Function : InformReadAddress
 *            Generic shared function with video driver that allows it to
 *              inform the read pointer of its InputBuffer.
 * Input    : Handle, Address_p.
 * Output   :
 * Return   : ST_NO_ERROR
 * ----------------------------------------------------------------------*/
static void InformReadAddress(void * const Handle, void * const Address_p)
{
    VideoInputBuffRptr_p = Address_p;
}

/*-------------------------------------------------------------------------
 * Function : DoVideoInject
 *            Main task that manages the stream injection from a UserBuffer
 *            to a LinearInputBuffer.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static void DoVideoInject( void )
{
    ST_ErrorCode_t ErrCode;

    /* UserBuffer pointers....          */
    U32 RemainingSourceDataSize;

    /* LinearInputBuffer pointers ....  */
    U32 SnapInputRead;
    U32 SnapInputWrite;

    U32 SizeToBeInjected    = 0;
    U32 BitBufferFreeSize   = 0;
    U32 InputBufferFreeSize = 0;

    BOOL FirstTransfer      = TRUE;

    while(1)
    {
        /* Delay the injection process. */
        task_delay(6000);

        /* Get available video bit buffer free size.    */
        ErrCode = STVID_GetBitBufferFreeSize(VideoHandle, &BitBufferFreeSize);
        if (ErrCode != ST_NO_ERROR)
        {
            continue;
        }

        if ((BitBufferFreeSize < BIT_BUFFER_THRESHOLD) || (VideoInputBuffRptr_p == NULL) ||
            (VideoInputBuffWptr_p == NULL) || (UserBufferRPtr_p == NULL) )
        {
            /* Video bit buffer filled enough for the moment. Delay the injection. */
            continue;
        }

        /* Now, check for available space in InputBuffer.               */
        SnapInputRead  = (U32)VideoInputBuffRptr_p;
        SnapInputWrite = (U32)VideoInputBuffWptr_p;

        if (SnapInputWrite == SnapInputRead)
        {
            InputBufferFreeSize = VideoInputBufferSize;
        }
        else if (SnapInputWrite > SnapInputRead)
        {
            InputBufferFreeSize = VideoInputBufferSize - (SnapInputWrite - SnapInputRead);
        }
        else
        {
            InputBufferFreeSize = ((U32)VideoInputBuffTop_p - SnapInputRead) +
                                  (SnapInputWrite - (U32)VideoInputBuffBase_p);
        }

        /* And remaining dtaa size to be injected from source buffer.   */
        RemainingSourceDataSize = (U32)UserBufferTop_p - (U32)UserBufferRPtr_p + 1;

        SizeToBeInjected = (RemainingSourceDataSize < InputBufferFreeSize ?
                            RemainingSourceDataSize : InputBufferFreeSize );

        /* Correct it now to avoid too big transfers.                   */
        SizeToBeInjected = (SizeToBeInjected > MAX_TRANSFER_SIZE ?
                MAX_TRANSFER_SIZE : SizeToBeInjected);

        /* Perform the copy.....                                        */
        memcpy(/* dest */   (void*)SnapInputWrite,
               /* src  */   UserBufferRPtr_p,
               /* size */   SizeToBeInjected);

        /* Push the W pointer in video input */
        if(( SnapInputWrite + SizeToBeInjected) > (U32)VideoInputBuffTop_p)
        {
            printf("injection loops video input\r\n");
            SnapInputWrite = (U32)VideoInputBuffBase_p;
        }
        else
        {
            SnapInputWrite =  SnapInputWrite + SizeToBeInjected;
        }
        /* Share the new input ptr */
        VideoInputBuffWptr_p = (void*)SnapInputWrite;

        /* Push the write pointer in source buffer too. */
        if ( ((U32)UserBufferRPtr_p + SizeToBeInjected) > (U32)UserBufferTop_p )
        {
            printf("injection loops the stream\r\n");
            UserBufferRPtr_p = UserBufferBase_p;
        }
        else
        {
            UserBufferRPtr_p = (void*)((U32)UserBufferRPtr_p + SizeToBeInjected);
        }
    } /* end of while(1) */
} /* End of DoVideoInject() task function. */

/* -------------------------------------------------------------------------- */

BOOL video_test(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t  Error;
#ifdef TEST_HD
    char FileName[30] = "../../WhatCut.m2v";
/*    char FileName[30] = "../../LucentCut.m2v";*/
#else
/*    char FileName[30] = "../../sony-ct4.m2v";*/
/*    char FileName[30] = "../../br05000k.p2v";*/
/*    char FileName[30] = "../../chrisb_1.m2v";*/
    char FileName[30] = "../../susiecut.m1v";
#endif
    long int FileDescriptor;
    STVID_InitParams_t      InitParams;
    STVID_OpenParams_t      OpenParams;
    STVID_MemoryProfile_t   MemProf;
    STVID_StartParams_t     StartParams;

#ifdef USE_BEIO
    /* Video part....   */
    STVID_ViewPortHandle_t  ViewPortHandle;
    STVID_ViewPortParams_t  ViewPortParams;

    /* Denc  part....   */
    STDENC_InitParams_t     DENCInitParams;
    STDENC_OpenParams_t     DENCOpenParams;
    STDENC_EncodingMode_t   DENCEncodingMode;
    /* Vout  part....   */
    STVOUT_InitParams_t     VOUTInitParams;
    STVOUT_OpenParams_t     VOUTOpenParams;
    STVOUT_OutputParams_t   VOUTOutpuParams;
    /* Vtg   part...    */
    STVTG_InitParams_t      VTGInitParams;
    STVTG_OpenParams_t      VTGOpenParams;
    /* Vmix  part...    */
    STVMIX_InitParams_t         VMIXInitParams;
    ST_DeviceName_t             OutputArray[2];
    STVMIX_OpenParams_t         VMIXOpenParams;
    STVMIX_LayerDisplayParams_t * LayerContext[2];
    STVMIX_LayerDisplayParams_t   LayerVideo1[2];

    /* Layer part...    */
    STLAYER_InitParams_t    LAYERInitParams;
    STLAYER_LayerParams_t   LayerParams;

/*    STAVMEM_AllocBlockParams_t  AllocBlockParams;*/
/*    STAVMEM_BlockHandle_t   MemHandle;*/
/*    STGXOBJ_Bitmap_t        CacheBitmap;*/

#endif /* USE_BEIO */

    printf("bonjour video test\n");

    /* INIT VIDEO */
    /*------------*/
#ifdef ST_5100
    InitParams.DeviceType               = STVID_DEVICE_TYPE_5100_MPEG;
    InitParams.DeviceBaseAddress_p      = (void*)ST5100_VIDEO_BASE_ADDRESS;
#elif defined ST_7710
    InitParams.DeviceType               = STVID_DEVICE_TYPE_7710_MPEG;
    InitParams.DeviceBaseAddress_p      = (void*)ST7710_VIDEO_BASE_ADDRESS;
#else
#   error mpeg decoder not compatible
#endif
    InitParams.BaseAddress_p            = 0;
    InitParams.BitBufferAllocated       = FALSE;
    InitParams.BitBufferAddress_p       = NULL;
    InitParams.BitBufferSize            = 0;
    InitParams.InterruptEvent           = 0;
    InitParams.UserDataSize             = VID_MAX_USER_DATA;
    InitParams.InstallVideoInterruptHandler = TRUE;
    InitParams.InterruptNumber          = VIDEO_INTERRUPT;
    InitParams.InterruptLevel           = VIDEO_INTERRUPT_LEVEL;
    InitParams.CPUPartition_p           = DriverPartition_p;
    InitParams.SharedMemoryBaseAddress_p= 0;
    InitParams.AVMEMPartition           = AvmemPartitionHandle[0];
    InitParams.MaxOpen                  = 1;
    InitParams.AVSYNCDriftThreshold     = 0;
    InitParams.BaseAddress2_p           = 0;
    InitParams.BaseAddress3_p           = 0;
    strcpy(InitParams.InterruptEventName,STEVT_DEVICE_NAME);
    strcpy(InitParams.EvtHandlerName,STEVT_DEVICE_NAME);
    strcpy(InitParams.ClockRecoveryName,"NO-CLKRV");
    Error = STVID_Init(VIDEO_NAME,&InitParams);
    if(Error)   printf("ERROR IN VIDEO INIT\r\n");
    else        printf("VIDEO INIT OK\r\n");

    /* OPEN */
    /*------*/
    OpenParams.SyncDelay                = 0;
    Error = STVID_Open(VIDEO_NAME,&OpenParams,&VideoHandle);
    if(Error)   printf("ERROR IN VIDEO OPEN\r\n");
    else        printf("VIDEO OPEN OK\r\n");

    /* MEMORY PROFILE */
    /*----------------*/
#ifdef TEST_HD
    MemProf.MaxWidth                    = 1920;
    MemProf.MaxHeight                   = 1088;
    MemProf.NbFrameStore                = 2;
    MemProf.CompressionLevel            = STVID_COMPRESSION_LEVEL_NONE;
    MemProf.DecimationFactor            = STVID_DECIMATION_FACTOR_H2;
#else
    MemProf.MaxWidth                    = 352;
    MemProf.MaxHeight                   = 240;
    MemProf.NbFrameStore                = 4;
    MemProf.CompressionLevel            = STVID_COMPRESSION_LEVEL_NONE;
    MemProf.DecimationFactor            = STVID_DECIMATION_FACTOR_NONE;
#endif
    Error = STVID_SetMemoryProfile(VideoHandle,&MemProf);
    if(Error)   printf("ERROR IN VIDEO PROFILE\r\n");
    else        printf("VIDEO PROFILE OK\r\n");
#ifdef USE_BEIO /* !!! Only available for STi7710 !!! */

    /* init of denc/vtg/vout */

    /* STDENC Initialization */
    /*-----------------------*/
    DENCInitParams.DeviceType = STDENC_DEVICE_TYPE_7710;
    DENCInitParams.MaxOpen    = 3;
    DENCInitParams.AccessType = STDENC_ACCESS_TYPE_EMI;
    DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p    = (void *)0;
    DENCInitParams.STDENC_Access.EMI.BaseAddress_p          = (void *)ST7710_DENC_BASE_ADDRESS;
    DENCInitParams.STDENC_Access.EMI.Width                  = 0;

    Error = STDENC_Init("DENC", &DENCInitParams);
    if(Error)   printf("ERROR IN DENC INIT\r\n");
    else        printf("DENC INIT OK \r\n");

    Error = STDENC_Open("DENC",&DENCOpenParams, &DencHandle);
    if(Error)   printf("ERROR IN DENC OPEN\r\n");
    else        printf("DENC OPEN OK \r\n");

    DENCEncodingMode.Mode = STDENC_MODE_NTSCM;
    DENCEncodingMode.Option.Ntsc.FieldRate60Hz = FALSE;
    DENCEncodingMode.Option.Ntsc.Interlaced    = TRUE;
    DENCEncodingMode.Option.Ntsc.SquarePixel   = FALSE;

    Error = STDENC_SetEncodingMode(DencHandle, &DENCEncodingMode);
    if(Error)   printf("ERROR IN DENC SETENC\r\n");
    else        printf("DENC SETENC OK\r\n");

    /* STVOUT Initialization */
    /*-----------------------*/
    VOUTInitParams.DeviceType       = STVOUT_DEVICE_TYPE_7710;
    VOUTInitParams.CPUPartition_p   = DriverPartition_p;
    VOUTInitParams.MaxOpen          = 3;
    VOUTInitParams.OutputType       = STVOUT_OUTPUT_ANALOG_CVBS;
    strcpy(VOUTInitParams.Target.GenericCell.DencName, "DENC");
    VOUTInitParams.Target.GenericCell.DeviceBaseAddress_p   = 0;
    VOUTInitParams.Target.GenericCell.BaseAddress_p         = (void*)VOUT_BASE_ADDRESS;

    Error = STVOUT_Init("VOUT", &VOUTInitParams);
    if(Error)   printf("ERROR IN VOUT INIT\r\n");
    else        printf("VOUT INIT OK\r\n");

    Error = STVOUT_Open("VOUT", &VOUTOpenParams, &VoutHandle);
    if(Error)   printf("ERROR IN VOUT OPEN\r\n");
    else        printf("VOUT OPEN OK\r\n");

    VOUTOutpuParams.Analog.StateAnalogLevel  = STVOUT_PARAMS_DEFAULT;
    VOUTOutpuParams.Analog.StateBCSLevel     = STVOUT_PARAMS_DEFAULT;
    VOUTOutpuParams.Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;
    VOUTOutpuParams.Analog.EmbeddedType      = FALSE;
    VOUTOutpuParams.Analog.InvertedOutput    = FALSE;
    VOUTOutpuParams.Analog.SyncroInChroma    = FALSE;
    VOUTOutpuParams.Analog.ColorSpace        = STVOUT_ITU_R_601;

    Error = STVOUT_SetOutputParams(VoutHandle, &VOUTOutpuParams);
    if(Error)   printf("ERROR IN VOUT SETPARAM\r\n");
    else        printf("VOUT SETPARAM OK\r\n");

    /* STVTG Initialization  */
    /*-----------------------*/
    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VTG_CELL_7710;
    VTGInitParams.MaxOpen    = 3;
    strcpy(VTGInitParams.EvtHandlerName,STEVT_DEVICE_NAME);

    VTGInitParams.Target.VtgCell2.BaseAddress_p         = (void*)ST7710_VTG1_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell2.DeviceBaseAddress_p   = (void*)0;
    VTGInitParams.Target.VtgCell2.InterruptNumber       = ST7710_VOS_0_INTERRUPT;
    VTGInitParams.Target.VtgCell2.InterruptLevel        = 4;

    Error = STVTG_Init("VTG", &VTGInitParams);
    if(Error)   printf("ERROR IN VTG INIT\r\n");
    else        printf("VTG INIT OK\r\n");

    Error = STVTG_Open("VTG", &VTGOpenParams, &VtgHandle);
    if(Error)   printf("ERROR IN VTG OPEN\r\n");
    else        printf("VTG OPEN OK\r\n");

    Error = STVTG_SetMode(VtgHandle, STVTG_TIMING_MODE_480I59940_13500);
    if(Error)   printf("ERROR IN VTG SETMODE\r\n");
    else        printf("VTG SETMODE OK\r\n");

    /* STVMIX Initialization */
    /*-----------------------*/
    VMIXInitParams.CPUPartition_p       = DriverPartition_p;
    VMIXInitParams.BaseAddress_p        = (void*)ST7710_VMIX1_BASE_ADDRESS;
    VMIXInitParams.DeviceBaseAddress_p  = 0;
    VMIXInitParams.DeviceType           = STVMIX_GENERIC_GAMMA_TYPE_7710;
    VMIXInitParams.MaxOpen              = 2;
    VMIXInitParams.MaxLayer             = 2;
    strcpy(VMIXInitParams.VTGName, "VTG");
    strcpy(VMIXInitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    strcpy(OutputArray[0], "VOUT");
    strcpy(OutputArray[1], "");
    VMIXInitParams.OutputArray_p        = OutputArray;
    VMIXInitParams.AVMEM_Partition      = AvmemPartitionHandle[0];;
/*    STVMIX_RegisterInfo_t       RegisterInfo; not used on STi7710 */
/*    STGXOBJ_Bitmap_t*           CacheBitmap_p;not used on STi7710 */

    Error = STVMIX_Init ("VMIX", &VMIXInitParams);
    if(Error)   printf("ERROR IN VMIX INIT\r\n");
    else        printf("VMIX INIT OK \r\n");

    Error = STVMIX_Open("VMIX", &VMIXOpenParams, &VmixHandle);
    Error = STVMIX_Disable(VmixHandle);
    Error = STVMIX_SetTimeBase(VmixHandle, "VTG");
    if(Error)   printf("ERROR IN VMIX SETTING\r\n");
    else        printf("VMIX SETTING OK \r\n");


    Error = STVOUT_Enable(VoutHandle);
    if(Error)   printf("ERROR IN VOUT ENABLE\r\n");
    else        printf("VOUT ENABLE OK \r\n");

    /* STLAYER Initialization */
    /*------------------------*/
    LAYERInitParams.LayerType                   = STLAYER_HDDISPO2_VIDEO1;
    LAYERInitParams.CPUPartition_p              = DriverPartition_p;
    LAYERInitParams.CPUBigEndian                = TRUE;

    LAYERInitParams.DeviceBaseAddress_p         = 0;
    LAYERInitParams.BaseAddress_p               = (void *)(ST7710_VID1_LAYER_BASE_ADDRESS);
    LAYERInitParams.BaseAddress2_p              = (void *)(ST7710_DISP0_BASE_ADDRESS);

    LAYERInitParams.SharedMemoryBaseAddress_p   = 0;
    LAYERInitParams.MaxHandles                  = 2;
    LAYERInitParams.AVMEM_Partition             = AvmemPartitionHandle[0];
    strcpy(LAYERInitParams.EventHandlerName,STEVT_DEVICE_NAME);
    strcpy(LAYERInitParams.InterruptEventName,STEVT_DEVICE_NAME);
    LAYERInitParams.MaxViewPorts                = 1;
    LAYERInitParams.NodeBufferUserAllocated     = FALSE;
    LAYERInitParams.ViewPortBufferUserAllocated = FALSE;
    LayerParams.Width               = 720;
    LayerParams.Height              = 480;
    LayerParams.AspectRatio         = STGXOBJ_ASPECT_RATIO_4TO3;
    LayerParams.ScanType            = STGXOBJ_INTERLACED_SCAN;
    LAYERInitParams.LayerParams_p = &LayerParams;
    Error = STLAYER_Init("LAYER",&LAYERInitParams);
    if(Error)   printf("ERROR IN LAYER INIT\r\n");
    else        printf("LAYER INIT OK\r\n");

    /* Viewport management */
    /*---------------------*/
    strcpy(ViewPortParams.LayerName,"LAYER");
    Error = STVID_OpenViewPort(VideoHandle,&ViewPortParams,&ViewPortHandle);
    if(Error)   printf("ERROR IN OPENVP\r\n");
    else        printf("OPENVP OK\r\n");

    /* Vimex / Layer connection */
    /*--------------------------*/
    strcpy(LayerVideo1[0].DeviceName,"LAYER");
    LayerVideo1[0].ActiveSignal = TRUE;
    LayerContext[0] = &LayerVideo1[0];
    Error = STVMIX_ConnectLayers(VmixHandle, (const STVMIX_LayerDisplayParams_t**)LayerContext, 1);
    if(Error)   printf("ERROR IN VMIX CONNECT\r\n");
    else        printf("VMIX CONNECT OK\r\n");
#endif /* USE_BEIO */

    /* LOAD STREAM */
    /*-------------*/
    printf("load stream ...");
    FileDescriptor = debugopen(FileName, "rb" );
    UserBufferSize = debugfilesize(FileDescriptor);
    UserBufferBase_p = (char *)memory_allocate(DataPartition_p,(U32)UserBufferSize);
    if(UserBufferBase_p == NULL)
    {
        UserBufferSize = 0x0080000 - 500;
        UserBufferBase_p = (char *)memory_allocate(DataPartition_p,
                        (U32)((UserBufferSize)));
    }
    debugread(FileDescriptor, UserBufferBase_p, (size_t)UserBufferSize);
    debugclose( FileDescriptor );
    printf("...done\r\n");

    UserBufferRPtr_p = UserBufferBase_p;
    UserBufferTop_p = (void*)((U32)UserBufferBase_p + UserBufferSize - 1);

    /* INSTALL INTERFACE OBJECT AND INJECTION TASK */
    /*---------------------------------------------*/
    STVID_GetDataInputBufferParams(VideoHandle,&VideoInputBuffBase_p,
            &VideoInputBufferSize);
    VideoInputBuffWptr_p = VideoInputBuffBase_p;
    VideoInputBuffRptr_p = VideoInputBuffBase_p;
    VideoInputBuffTop_p  = (void*)((U32)VideoInputBuffBase_p + VideoInputBufferSize - 1);

    Error = STVID_SetDataInputInterface(VideoHandle,
            GetWriteAddress,InformReadAddress,(void*)1);

    task_init( (void (*)(void *))DoVideoInject,NULL,Stack,
            TASK_STACK, &Task, &Desc,0/*prio*/,
            "INJECTION",0);


    /* ERROR RECOVERY MODE */
    /*---------------------*/
    Error = STVID_SetErrorRecoveryMode(VideoHandle,
            STVID_ERROR_RECOVERY_NONE);
    if(Error)   printf("ERROR IN VIDEO ERROR MODE\r\n");
    else        printf("VIDEO ERROR MODE OK\r\n");

    /* START DECODE */
    /*--------------*/
    StartParams.RealTime                = FALSE;
    StartParams.UpdateDisplay           = TRUE;
    StartParams.StreamType              = STVID_STREAM_TYPE_PES;
    StartParams.StreamID                = STVID_IGNORE_ID;
    StartParams.BrdCstProfile           = STVID_BROADCAST_DVB;
    StartParams.DecodeOnce              = FALSE;
    Error = STVID_Start(VideoHandle, &StartParams );
    if(Error)   printf("ERROR IN VIDEO START\r\n");
    else        printf("VIDEO START OK\r\n");

#ifdef USE_BEIO
    /* ENABLE VIDEO AND MIXER */
    /*------------------------*/
    task_delay(5000);
    Error = STVID_EnableOutputWindow(ViewPortHandle);
    if(Error)   printf("ERROR IN VP ENABLE\r\n");
    else        printf("VP ENABLE OK\r\n");
    Error = STVMIX_Enable(VmixHandle);
    if(Error)   printf("ERROR IN VMIX ENABLE\r\n");
    else        printf("VMIX ENABLE OK\r\n");
#endif /* USE_BEIO */

    return(TRUE);
} /* End of video_test() function. */

/*-------------------------------------------------------------------------
 * Function : Test_CmdStart
 *            Register video test application command
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL Test_CmdStart()
{
    VID_RegisterCmd3();
    register_command( "video_test",  video_test, "Debug of the video");
/*    Init_Stub();*/
    return(TRUE);

} /* End of Test_CmdStart() function. */


/* -------------------------------------------------------------------------- */
