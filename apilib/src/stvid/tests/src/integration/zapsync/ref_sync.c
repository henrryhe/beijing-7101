/*******************************************************************************

File name   : ref_sync.c

Description : video sync reference file
              Linux file ONLY

COPYRIGHT (C) STMicroelectronics 2006

Note : 

Date               Modification                                     Name
----               ------------                                     ----
20 Jan  2005        Created                                          LM

*******************************************************************************/

/* Assumptions :

   Only one PTI device
   Two video slots
   One audio slot
   7100 Rev B Board
   DVD_TRANSPORT_STPTI4
   DVB
   STVOUT_OUTPUT_ANALOG_CVBS
   Single decode (MPEG or H264), Single display (MAIN) CVBS
   PAL or NTSC format
   VID1 ONLY
   LMI_VID used                                                                              

*/

#include <stdio.h>

#include "ref_sync.h"
#include "ref_init.h"
#include "ref_term.h"

/* Global Variables --------------------------------------------------------- */

int                      *SystemPartition_p;
int                      *NCachePartition_p;
int                      *DriverPartition_p;
int                      *InternalPartition_p;

int                      VideoPid = 0x32, AudioPid = 0x32, PCRPid = 0x33;
REF_Format_e             Format = STREAM_FORMAT_PAL;
REF_Codec_e              Codec  = STREAM_CODEC_MPEG;
int                      HD = 0;

STOS_Clock_t             MaxDelayForLastPacketTransferWhenStptiPidCleared = 0;

STPTI_Buffer_t           HandleForPTI[PTICMD_MAX_SLOTS];
STPTI_Slot_t             SlotHandle[PTICMD_MAX_SLOTS];
STPTI_Handle_t           PTI_Handle;
STPTI_Buffer_t           PTI_AudioBufferHandle;

BOOL                     PtiCmdAudioStarted[PTICMD_MAX_AUDIO_SLOT];
BOOL                     PtiCmdPCRStarted[PTICMD_MAX_PCR_SLOT];

int                      PCRState       = PTI_STCLKRV_STATE_PCR_INVALID;
int                      PCRValidCount  = 0;
U32                      SlotNumber= 0;

int                      AUDState       = AUD_STAUDLX_STATE_OUT_OF_SYNC;

STVOUT_OutputParams_t    Str_OutParam;

static int FR60 = 0, FR59 = 0, FR50 = 0, FR50_1 = 0;
static int AC3 = 0;

/* Extern Global Variables --------------------------------------------------- */

extern BOOL                     PCRValidCheck; /* Flag to enable the check of STCLKRV state machine */
extern BOOL                     PCRInvalidCheck; /* Flag to enable the check of STCLKRV state machine */

extern semaphore_t              *PCRValidFlag_p;
extern semaphore_t              *PCRInvalidFlag_p;

extern BOOL                     AUDValidCheck;
extern semaphore_t              *AUDValidFlag_p;

extern STCLKRV_Handle_t         CLKRV_Handle;
extern STAUD_Handle_t           AUD_Handle;
extern STDENC_Handle_t          DENC_Handle;
extern STVOUT_Handle_t          VOUT_Handle;
extern STVOUT_Handle_t          VOUT_Handle_HD;
extern STVTG_Handle_t           VTG_Handle;
extern STVMIX_Handle_t          VMIX_Handle;
extern STLAYER_Handle_t         LAYER_Handle;
extern STVID_Handle_t           VID_Handle;

STVID_ViewPortHandle_t          VID_ViewPortHandle;

/*******************************************************************************
Name        : main
Description : Main function 
Parameters  : video PID, audio PID, pcr PID, video PID2, audio PID2, ...
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
int main(int argc, char *argv[])
{
    BOOL RetOK = TRUE;
    int  i,loop = 10;
    
    printf("            **************               \n");
    printf("          **** ref sync ****             \n");
    printf("            **************               \n");

    /* Getting command line parameters: 
    #1: Format: PAL or NTSC or HD
    #2: Codec: MPEG or H264
    #3 to #6: PIDs: VPID/APID/PPID
    */
    
    for ( i = 1; i < argc; i++ )
    {
        if (strcmp(argv[i],"PAL") == 0)
        {
            Format = STREAM_FORMAT_PAL;
            printf("Format is PAL\n");
        }
        else if (strcmp(argv[i],"NTSC") == 0)
        {
            Format = STREAM_FORMAT_NTSC;
            printf("Format is NTSC\n");
        }
        else if (strcmp(argv[i],"HD1080I") == 0)
        {
            Format = STREAM_FORMAT_HD1080I;
            HD = 1;
            printf("Format is 1080i HD\n");
        }
        else if (strcmp(argv[i],"HD720P") == 0)
        {
            Format = STREAM_FORMAT_HD720P;
            HD = 2;
            printf("Format is 720P HD\n");
        }
        else if (strcmp(argv[i],"MPEG") == 0)
        {
            Codec = STREAM_CODEC_MPEG;
            printf("Codec is MPEG\n");
        }
        else if (strcmp(argv[i],"H264") == 0)
        {
            Codec = STREAM_CODEC_H264;
            printf("Codec is H264\n");
        }
        else if (memcmp(argv[i],"VPID=", 5) == 0)
        {
        	VideoPid = atoi(argv[i] + 5);
        	printf("VideoPid=%d\n", VideoPid);
        }
        else if (memcmp(argv[i],"APID=", 5) == 0)
        {
        	AudioPid = atoi(argv[i] + 5);
        	printf("AudioPid=%d\n", AudioPid);
        }
        else if (memcmp(argv[i],"PPID=", 5) == 0)
        {
        	PCRPid = atoi(argv[i] + 5);
        	printf("PCRPid=%d\n", PCRPid);
        }
        else if (strcmp(argv[i],"AC3") == 0)
        {
        	AC3 = 1;
        	printf("AC3 sound used\n");
        }
        
        if (HD == 1)
        {
            if (strcmp(argv[i],"FR60") == 0)
                FR60 = 1;
            if (strcmp(argv[i],"FR59") == 0)
                FR59 = 1;
            if (strcmp(argv[i],"FR50") == 0)
                FR50 = 1;
            if (strcmp(argv[i],"FR50_1") == 0)
                FR50_1 = 1;
        }
    }        

    /* Initialization */
    RetOK = startup();

    /* Starting Video, Audio and synchronizing */
    sub_start(VideoPid, AudioPid, PCRPid);
    i = 1;
    do
    {
    printf( "*********************************************\nNow we will switch channels %d times, ok ?", i);
    if (getchar() == 'n')
       break;
    
    loop --;
    i++;
    printf("New Video PID:"); scanf("%d", &VideoPid);
    printf("New Audio PID:"); scanf("%d", &AudioPid);
    printf("New Pcr PID:"); scanf("%d", &PCRPid);

    printf( "Press return to switch\n");
    getchar();
    
    /* Channel change */
    channel_change(VideoPid, AudioPid, PCRPid);
    } while (loop > 0);

    printf( "Press return to end\n");
    getchar();

    /* Termination */
    enddown();
    
    return(0);
}

/* *************************************************************************** */
/* ***************************** STARTUP  ************************************ */
/* *************************************************************************** */


/*******************************************************************************
Name        : startup
Description : Main startup function 
Parameters  : 
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
BOOL startup()
{
    BOOL               RetOK = TRUE;
    ST_ErrorCode_t     ErrorCode;
    U8                 j;
    
    /* ---------------------------------------------------- */

    printf("Partitions initializing...");
	/* Following pointer must not be NULL but are not used with LINUX */
	SystemPartition_p = InternalPartition_p = NCachePartition_p = DriverPartition_p = (int *) 0xFFFFFFFF;
    printf("Ok\n");

    /* ------------ Event init ---------------------------- */
    (RetOK) ? RetOK = EVT_Init(): FALSE;
    
    /* ------------ Event init ---------------------------- */
    (RetOK) ? RetOK = VIDTEST_LocalInit(): FALSE;
    
    /* ------------ PTI init ------------------------------ */

    if (RetOK)
    {
        printf("STPTI init...");
        /* Min bit rate = 1000000 bit/s ? (we have streams at 500000, but we take low risk)
           Max size of one packet in STPTI = 188 bytes ? */
        MaxDelayForLastPacketTransferWhenStptiPidCleared = (ST_GetClocksPerSecond() * 188 * 8) / 1000000;
        if (MaxDelayForLastPacketTransferWhenStptiPidCleared == 0)
        {
            MaxDelayForLastPacketTransferWhenStptiPidCleared = 1;
        }
        
        for(j=0; j< PTICMD_MAX_AUDIO_SLOT; j++)
        {
            PtiCmdAudioStarted[j] = FALSE;
        }

        for(j=0; j< PTICMD_MAX_PCR_SLOT; j++)
        {
            PtiCmdPCRStarted[j] = FALSE;
        }
        PTI_Handle = 0;

        RetOK ? RetOK &= MERGE_Init(): FALSE;
        RetOK ? RetOK &= PTI_Init(): FALSE;
    }

    /* ------------------------------------------------------ */

    RetOK ? RetOK &= CLKRV_Init(): FALSE;

    RetOK ? RetOK &= AUD_Init(): FALSE;

    RetOK ? RetOK &= DENC_Init(): FALSE;

    RetOK ? RetOK &= VOUT_Init(): FALSE;
        
    RetOK ? RetOK &= VTG_Init(): FALSE;

    RetOK ? RetOK &= VMIX_Init(): FALSE;

    RetOK ? RetOK &= LAYER_Init(): FALSE;

    RetOK ? RetOK &= VID_Init(): FALSE;

    /* ------------------------------------------------------ */

    if (RetOK)
    {
        printf("Initialisation OK\n");
    }
    else
    {
        printf("Pb in init\n");
        return(RetOK);
    }

    printf("======================================================================\n");
    printf("                          Test Application \n");
    printf("                       %s - %s \n", __DATE__, __TIME__  );
    printf("                    CPU Clock Speed = %d Hz\n", ST_GetClockSpeed() );
    printf("                    CPU Clock per second = %d clk/sec\n", ST_GetClocksPerSecond() );
    printf("======================================================================\n");

    /* ------------------------------------------------------ */
    /* DENC_SetMode (DENC_Handle,"PAL"); */
    /* ------------------------------------------------------ */
    if (HD == 0)
    {
    if (RetOK)
    {
        STDENC_EncodingMode_t       ModeDenc;

        /* Default init */
        memset(&ModeDenc, 0xff, sizeof(STDENC_EncodingMode_t));

        if (Format == STREAM_FORMAT_PAL)    
        {
            printf("Setting Mode PAL\n");
            ModeDenc.Mode = (STDENC_Mode_t)STDENC_MODE_PALBDGHI;
            ModeDenc.Option.Pal.SquarePixel = FALSE;
            ModeDenc.Option.Pal.Interlaced = TRUE;
        }
        if (Format == STREAM_FORMAT_NTSC)    
        {
            printf("Setting Mode NTSC\n");
            ModeDenc.Mode = (STDENC_Mode_t)STDENC_MODE_NTSCM;
            ModeDenc.Option.Ntsc.SquarePixel = FALSE;
            ModeDenc.Option.Ntsc.Interlaced = TRUE;
            ModeDenc.Option.Ntsc.FieldRate60Hz = FALSE;
        }
        
        ErrorCode = STDENC_SetEncodingMode(DENC_Handle, &ModeDenc);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STDENC_SetMode OK\n");
        }
    }
    }

    /* ------------------------------------------------------ */
    /* VOUT_SetParams (VOUT_Handle);                        */
    /* ------------------------------------------------------ */

    if (RetOK)
    {
        STVOUT_OutputParams_t *OutParam    = &Str_OutParam;
 
        OutParam->Analog.StateAnalogLevel  = STVOUT_PARAMS_DEFAULT;
        OutParam->Analog.StateBCSLevel     = STVOUT_PARAMS_DEFAULT;
        OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;
        OutParam->Analog.EmbeddedType      = FALSE;
        OutParam->Analog.InvertedOutput    = FALSE;
        OutParam->Analog.SyncroInChroma    = FALSE;
        OutParam->Analog.ColorSpace        = STVOUT_ITU_R_601;

        ErrorCode = STVOUT_SetOutputParams(VOUT_Handle, OutParam);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("VOUT_SetParams OK\n");
        }
    }
    
    /* ------------------------------------------------------ */
    /* VOUT_Enable (VOUT_Handle); */
    /* ------------------------------------------------------ */
    
    if (RetOK)
    {
        ErrorCode = STVOUT_Enable(VOUT_Handle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVOUT_Enable OK\n");
        }
    }

    if (RetOK)
    {
        if (HD != 0)
        {
        ErrorCode = STVOUT_Enable(VOUT_Handle_HD);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVOUT_Enable for HD OK\n");
        }
        }
    }
    /* ------------------------------------------------------ */
    /* VTG_SetMode (VTG_Handle); */
    /* ------------------------------------------------------ */
    
    if (RetOK)
    {
        STVTG_TimingMode_t ModeVtg;

        /* Workaround attempt */
        ModeVtg=(STVTG_TimingMode_t)STVTG_TIMING_MODE_480I59940_13500; 
        ErrorCode = STVTG_SetMode(VTG_Handle, ModeVtg);
        
        if (Format == STREAM_FORMAT_PAL)    
        {
            ModeVtg=(STVTG_TimingMode_t)STVTG_TIMING_MODE_576I50000_13500;
        }
        if (Format == STREAM_FORMAT_NTSC)    
        {
            ModeVtg=(STVTG_TimingMode_t)STVTG_TIMING_MODE_480I59940_13500; 
        }
        if (Format == STREAM_FORMAT_HD1080I)    
        {
            /* default setting */
            ModeVtg=(STVTG_TimingMode_t)STVTG_TIMING_MODE_1080I60000_74250; 
            if (FR60 == 1)
                ModeVtg=(STVTG_TimingMode_t)STVTG_TIMING_MODE_1080I60000_74250; 
            if (FR59 == 1)
                ModeVtg=(STVTG_TimingMode_t)STVTG_TIMING_MODE_1080I59940_74176; 
            if (FR50 == 1)            
                ModeVtg=(STVTG_TimingMode_t)STVTG_TIMING_MODE_1080I50000_74250; 
            if (FR50_1 == 1)
                ModeVtg=(STVTG_TimingMode_t)STVTG_TIMING_MODE_1080I50000_74250_1; 
        }
        if (Format == STREAM_FORMAT_HD720P)    
        {
            ModeVtg=(STVTG_TimingMode_t)STVTG_TIMING_MODE_720P60000_74250; 
        }
        
        ErrorCode = STVTG_SetMode(VTG_Handle, ModeVtg);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVTG_SetMode OK\n");
        }
    }
    
    if (RetOK)
    {
         ErrorCode = STVMIX_Disable(VMIX_Handle);
         if (ErrorCode != ST_NO_ERROR)
         {
             printf("STVMIX_Disable() failed\n");
             RetOK = FALSE;
         }
         else
             RetOK = TRUE;
    }
     
    if (RetOK)
    {
         ErrorCode = STVMIX_SetTimeBase(VMIX_Handle,STVTG_DEVICE_NAME);
         if (ErrorCode != ST_NO_ERROR)
         {
             printf("STVMIX_SetTimeBase() failed\n");
             RetOK = FALSE;
         }
         else
             RetOK = TRUE;
    }

    if (RetOK)
    {
        STVMIX_ScreenParams_t  ScreenParams;
        STVTG_TimingMode_t vtg_timingmode;
        STVTG_ModeParams_t vtg_modeparams;

        ErrorCode = STVTG_GetMode(VTG_Handle, &vtg_timingmode, &vtg_modeparams);
        ScreenParams.ScanType = vtg_modeparams.ScanType/*STGXOBJ_INTERLACED_SCAN*/;
        ScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3; /* 1 */
            ScreenParams.FrameRate  = vtg_modeparams.FrameRate;
            ScreenParams.Width      = vtg_modeparams.ActiveAreaWidth;
            ScreenParams.Height     = vtg_modeparams.ActiveAreaHeight;
            ScreenParams.XStart     = vtg_modeparams.ActiveAreaXStart;
            ScreenParams.YStart     = vtg_modeparams.ActiveAreaYStart;

         printf("FrameRate=%d, Width x Height = %d x %d, Xstart = %d, Ystart = %d\n", vtg_modeparams.FrameRate, vtg_modeparams.ActiveAreaWidth, vtg_modeparams.ActiveAreaHeight, vtg_modeparams.ActiveAreaXStart, vtg_modeparams.ActiveAreaYStart);

#if 0
        if (Format == STREAM_FORMAT_PAL)    
        {
            ScreenParams.FrameRate  = 50000; /* PAL */
            ScreenParams.Width      = 720;
            ScreenParams.Height     = 576;
            ScreenParams.XStart     = 137;
            ScreenParams.YStart     = 48;
        }
        if (Format == STREAM_FORMAT_NTSC)    
        {
            ScreenParams.FrameRate  = 59940; /* NTSC ? */
            ScreenParams.Width      = 720;
            ScreenParams.Height     = 480; /* NTSC */
            ScreenParams.XStart     = 122;
            ScreenParams.YStart     = 20;
        }
#endif    
        ErrorCode = STVMIX_SetScreenParams(VMIX_Handle, &ScreenParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVMIX_SetScreenParams OK\n");
        }
    }
    
    if (RetOK)
    {
         ErrorCode = STVMIX_Enable(VMIX_Handle);
         if (ErrorCode != ST_NO_ERROR)
         {
             printf("STVMIX_Enable() failed\n");
             RetOK = FALSE;
         }
         else
             RetOK = TRUE;
    }
#if 0
    if (RetOK)
    {
        STLAYER_LayerParams_t LayerParams = {0};
 


    }
#endif
    /* ------------------------------------------------------ */
    /* VMIX_Connect (VOUT_Handle, "LAYER"); */
    /* ------------------------------------------------------ */

    if (RetOK)
    {
        STVMIX_LayerDisplayParams_t*  LayerArray[MAX_LAYER];
        STVMIX_LayerDisplayParams_t   LayerParam[MAX_LAYER]; /* Layer parameters */

        /* Only one layer to connect */
        strcpy(LayerParam[0].DeviceName, STLAYER_DEVICE_NAME);
        LayerArray[0] = &LayerParam[0];
        LayerParam[0].ActiveSignal = FALSE ;

        ErrorCode = STVMIX_ConnectLayers(VMIX_Handle, (const STVMIX_LayerDisplayParams_t**)LayerArray, 1);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STVMIX_ConnectLayers()---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVMIX_ConnectLayers OK\n");
        }
    }
    
    /* ------------------------------------------------------ */
    /*VID_OpenVP (VID_Handle, "LAYER");*/
    /* ------------------------------------------------------ */
    
    if (RetOK)
    {
        STVID_ViewPortParams_t ViewPortParams;

        memset(&ViewPortParams, 0, sizeof(ViewPortParams));
        strcpy(ViewPortParams.LayerName, STLAYER_DEVICE_NAME);

        ErrorCode = STVID_OpenViewPort(VID_Handle, &ViewPortParams, &VID_ViewPortHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVID_OpenViewPort OK\n");
        }
    }
    
    /* ------------------------------------------------------ */
    /* VMIX_Ssreen (VMIX_Handle, 1 50000 720 576 137 48);     */
    /* ------------------------------------------------------ */
#if 0 
    if (RetOK)
    {
        STVMIX_ScreenParams_t  ScreenParams;

        STVTG_TimingMode_t vtg_timingmode;
        STVTG_ModeParams_t vtg_modeparams;

        ErrorCode = STVTG_GetMode(VTG_Handle, &vtg_timingmode, &vtg_modeparams);
        
        ScreenParams.ScanType = vtg_modeparams.ScanType/*STGXOBJ_INTERLACED_SCAN*/;
        ScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3; /* 1 */
            ScreenParams.FrameRate  = vtg_modeparams.FrameRate;
            ScreenParams.Width      = vtg_modeparams.ActiveAreaWidth;
            ScreenParams.Height     = vtg_modeparams.ActiveAreaHeight;
            ScreenParams.XStart     = vtg_modeparams.ActiveAreaXStart;
            ScreenParams.YStart     = vtg_modeparams.ActiveAreaYStart;

         printf("FrameRate=%d, Width x Height = %d x %d, Xstart = %d, Ystart = %d\n", vtg_modeparams.FrameRate, vtg_modeparams.ActiveAreaWidth, vtg_modeparams.ActiveAreaHeight, vtg_modeparams.ActiveAreaXStart, vtg_modeparams.ActiveAreaYStart);

#if 0
        if (Format == STREAM_FORMAT_PAL)    
        {
            ScreenParams.FrameRate  = 50000; /* PAL */
            ScreenParams.Width      = 720;
            ScreenParams.Height     = 576;
            ScreenParams.XStart     = 137;
            ScreenParams.YStart     = 48;
        }
        if (Format == STREAM_FORMAT_NTSC)    
        {
            ScreenParams.FrameRate  = 59940; /* NTSC ? */
            ScreenParams.Width      = 720;
            ScreenParams.Height     = 480; /* NTSC */
            ScreenParams.XStart     = 122;
            ScreenParams.YStart     = 20;
        }
#endif    
        ErrorCode = STVMIX_SetScreenParams(VMIX_Handle, &ScreenParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVMIX_SetScreenParams OK\n");
        }
    }
#endif
    
    /* ------------------------------------------------------ */
    /* VID_Setup (VID_Handle, 1, LMI_VID); STVID_SETUP_FRAME_BUFFERS_PARTITION */
    /* VID_Setup (VID_Handle, 2, LMI_VID); STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION */
    /* ------------------------------------------------------ */
    
    if (RetOK)
    {
        STVID_SetupParams_t SetupParams;

        SetupParams.SetupObject = STVID_SETUP_FRAME_BUFFERS_PARTITION;
        SetupParams.SetupSettings.AVMEMPartition = 1 ; /* LMI_VID */
        ErrorCode = STVID_Setup(VID_Handle, &SetupParams );

        if (Codec == STREAM_CODEC_H264)    
        {
            SetupParams.SetupObject = STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION;
            ErrorCode = STVID_Setup(VID_Handle, &SetupParams );
        }
        
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVID_Setup OK\n");
        }
    }

    /* ------------------------------------------------------ */
    /* VID_SetMemProfile (VID_Handle, 720 576 5 1 0); */
    /* ------------------------------------------------------ */
        
    if (RetOK)
    {
        STVID_MemoryProfile_t MemoryProfile;
        if (HD != 0)
        {
            MemoryProfile.MaxWidth =  1920;
            MemoryProfile.MaxHeight = 1088;
            MemoryProfile.NbFrameStore = 5; /* May be up to 8 in H264 */
        }
        else
        {
            MemoryProfile.MaxWidth =  720;
            if (Codec == STREAM_CODEC_H264)
            {
                MemoryProfile.MaxHeight = 608;
                /* MemoryProfile.MaxHeight = 480; */ /* NTSC */
                MemoryProfile.NbFrameStore = 8; /* May be up to 8 in H264 */
            }
            else
            {
                MemoryProfile.MaxHeight = 576;
                /* MemoryProfile.MaxHeight = 480; */ /* NTSC */
                MemoryProfile.NbFrameStore = 5; /* May be up to 8 in H264 */
            }
        }
        if (Format == STREAM_FORMAT_HD720P)
            MemoryProfile.DecimationFactor = STVID_DECIMATION_FACTOR_H2;
        else
            MemoryProfile.DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
        MemoryProfile.CompressionLevel = STVID_COMPRESSION_LEVEL_NONE;

        ErrorCode = STVID_SetMemoryProfile(VID_Handle, &MemoryProfile );
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVID_SetMemoryProfile OK\n");
        }
    }

    /* ------------------------------------------------------ */
    /* VMIX_Enable();     */
    /* ------------------------------------------------------ */
#if 0
    if (RetOK)
    {
        ErrorCode = STVMIX_Enable(VMIX_Handle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVMIX_Enable OK\n");
        }
    }
#endif
    /* ------------------------------------------------------ */
    /* VID_Disp(); */
    /* ------------------------------------------------------ */

    if (RetOK)
    {
        ErrorCode = STVID_EnableOutputWindow(VID_ViewPortHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STVID_EnableOutputWindow()---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVID_EnableOutputWindow OK\n");
        }
    }

    /* ------------------------------------------------------ */
    return(RetOK);

} /* end startup */

/*******************************************************************************
Name        : sub_start
Description : Function that starts channels and sync (video, audio & pcr PIDs)
Parameters  : 
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
BOOL sub_start(int VideoPID, int AudioPID, int PcrPID)
{
    BOOL               RetOK = TRUE;
    ST_ErrorCode_t     ErrorCode;

    /* ------------------------------------------------------ */
    /* VID_Start(); */
    /* ------------------------------------------------------ */

    if (RetOK)
    {
        STVID_StartParams_t StartParams;

        StartParams.RealTime        = TRUE;
        StartParams.UpdateDisplay   = TRUE;
        StartParams.StreamType      = STVID_STREAM_TYPE_PES;
        StartParams.StreamID        = STVID_IGNORE_ID;
        StartParams.BrdCstProfile   = STVID_BROADCAST_DVB;
        StartParams.DecodeOnce      = FALSE;

        ErrorCode = STVID_Start(VID_Handle, &StartParams );
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVID_Start OK\n");
        }
    }

    /* ------------------------------------------------------ */
    /* PTI_Vidstart(); */
    /* ------------------------------------------------------ */
   
    if (RetOK)
    {
        ProgId_t    PID = 0x32;
        void        *GetWriteAdd, *InformReadAdd; /* These are pointers on kernel functions */
        void        *Add;
        U32         Size;
    
        /* Pid given by parameter */
        printf("VID_InjectionPTI_PID:%x\n", VideoPid);
    
        /* No cd-fifo: Initialise a destination buffer */
        ErrorCode = STVID_GetDataInputBufferParams(VID_Handle, &Add, &Size);
        if (ErrorCode == ST_NO_ERROR)
        {
            printf("Injection buffer: \nAdd:%lu\nSize:%x\n", (unsigned long) Add, Size);
    
            ErrorCode = STPTI_BufferAllocateManual(PTI_Handle, (U8*)Add, Size, 1, &HandleForPTI[SlotNumber]);
            /* No cd-fifo: Initialise the link between pti and video */
        }

        if (ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = STPTI_SlotLinkToBuffer(SlotHandle[SlotNumber], HandleForPTI[SlotNumber]);
        }
        
        if (ErrorCode == ST_NO_ERROR)
        {
            /* In Linux, we need to get function addresses in kernel space */
            /* So we ask the kernel */
            ErrorCode = STVIDTEST_GetKernelInjectionFunctionPointers(STVIDTEST_INJECT_PTI, &GetWriteAdd, &InformReadAdd);
        }
        
        /* Share the living pointers=register fcts */
        if (ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = STVID_SetDataInputInterface(
                        VID_Handle,
                        GetWriteAdd, InformReadAdd,
                                (void * const)HandleForPTI[SlotNumber]);
        }

        if (ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = STPTI_BufferSetOverflowControl(HandleForPTI[SlotNumber],TRUE);
        }

        if (ErrorCode == ST_NO_ERROR)
        {
            printf("Injection PTI: \nVideoPid:h%x\nSlotNb:%d\n",VideoPid/*PID*/,SlotNumber);
            ErrorCode = STPTI_SlotSetPid(SlotHandle[SlotNumber], VideoPid);
            if (ErrorCode != ST_NO_ERROR)
            {
                printf("---> Error:%x\n",ErrorCode);
                RetOK = FALSE;    
            }
            else
            {
                printf("STPTI_SlotSetPid OK\n");
            }
        }
    }


    /* ------------------------------------------------------ */
    /* PTI_PCRPid(); */
    /* ------------------------------------------------------ */

    if (RetOK)
    {
        STCLKRV_SourceParams_t  SourceParams;
        STCLKRV_ExtendedSTC_t   ExtendedSTC;
        STOS_Clock_t            PCRTimeout;
    
        printf("STPIGAT PTI_PCRStart\n");
            
        /* CLKRV is already opened */
        /* Invalidate the PCR synchronization on PCR channel change */
        STCLKRV_InvDecodeClk(CLKRV_Handle);
    
        ErrorCode = STPTI_SlotSetPid(SlotHandle[PTICMD_BASE_PCR+SlotNumber], PCRPid);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("----> PTI_PCRStart(slot=%d,pid=%d) : failed error=%d!\n",
                        SlotNumber, PCRPid, ErrorCode);
            return(FALSE);
        }
        printf("PTI_PCRStart(slot=%d,pid=%d) : done\n", SlotNumber, PCRPid);
    
        ErrorCode = STCLKRV_SetApplicationMode(CLKRV_Handle, STCLKRV_APPLICATION_MODE_NORMAL);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STCLKRV_SetApplicationMode failed. Err=0x%x !\n",  ErrorCode);
            return(FALSE);
        }
    
        SourceParams.Source = STCLKRV_PCR_SOURCE_PTI;
        SourceParams.Source_u.STPTI_s.Slot = SlotHandle[PTICMD_BASE_PCR+SlotNumber];
    
        ErrorCode = STCLKRV_SetPCRSource(CLKRV_Handle, &SourceParams); 
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STCLKRV_SetPCRSource failed. Err=0x%x !\n", ErrorCode);
            return(FALSE);
        }
        
        ErrorCode = STCLKRV_SetSTCSource(CLKRV_Handle, STCLKRV_STC_SOURCE_PCR);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STCLKRV_SetSTCSource failed. Err=0x%x !\n", ErrorCode);
            return(FALSE);
        }
    

        /* **************************************** */
        /* Starting process of STCLKRV status check */
        /* **************************************** */

        ErrorCode = STCLKRV_Enable(CLKRV_Handle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STCLKRV_Enable error:%x\n",ErrorCode);
        }
        
        /* LM: Invalidate the PCR synchronization because STCLKRV_SetSTCSource always sends
               STCLKRV_PCR_VALID_EVT events.
               We want to be sure to catch the PCR_DISCONTINUITY_EVT, so we enable the
               detection here.
        */
        while (STOS_SemaphoreWaitTimeOut(PCRInvalidFlag_p, TIMEOUT_IMMEDIATE) == 0);
        PCRTimeout = STOS_time_plus(STOS_time_now(), PTI_STCLKRV_PCR_DISCONTINUITY_TIMEOUT);
        if (PCRState != PTI_STCLKRV_STATE_PCR_INVALID)
        {
            PCRInvalidCheck = TRUE; /* enable check  */              
        
            if (semaphore_wait_timeout(PCRInvalidFlag_p, &PCRTimeout) == 0)
            {
                printf("STCLKRV PCR discontinuity event\n");
            }
            else
            {
                printf("STCLKRV PCR no discontinuity event !\n");
            }

            PCRInvalidCheck = FALSE; /* disabled check */
        }
        printf("STCLKRV in discontinuity state\n");
                        
        /* We are now sure STCLKRV is in PTI_STCLKRV_STATE_PCR_INVALID state */
        /* We will wait and check STCLKRV_PCR_VALID_EVT during that time */
        while (STOS_SemaphoreWaitTimeOut(PCRValidFlag_p, TIMEOUT_IMMEDIATE) == 0);
        PCRTimeout = STOS_time_plus(STOS_time_now(), PTI_STCLKRV_PCR_VALID_TIMEOUT);
        if (PCRState != PTI_STCLKRV_STATE_PCR_OK)
        {
            PCRValidCheck = TRUE; /* enable check  */              
            
            if (semaphore_wait_timeout(PCRValidFlag_p, &PCRTimeout) == 0)
            {
                printf("STCLKRV PCR valid event\n");
            }
            else
            {
                printf("STCLKRV PCR timeout: STCLKRV not in sync !\n");
            }

            PCRValidCheck = FALSE; /* disabled check */

        }
        else
        {
            printf("STCLKRV in valid state\n");
        }

        /* The next call should be valid if the PCR pid is valid */                    
        ErrorCode = STCLKRV_GetExtendedSTC(CLKRV_Handle, &ExtendedSTC);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STCLKRV_GetExtendedSTC error 0x%x !\n", ErrorCode);
            printf("---> No PCR available\n");
        }
        else
        {
            printf("STC=0x%x,%x\n", ExtendedSTC.BaseBit32, ExtendedSTC.BaseValue);
            printf("PTI_PCRPid OK\n");
 
            /* ------------------------------------------------------ */
            /* vid_sync(); if PCR available                           */ 
            /* ------------------------------------------------------ */
     
            ErrorCode = STVID_EnableSynchronisation(VID_Handle);
            if (ErrorCode != ST_NO_ERROR)
            {
                printf("---> Sync: Error:%x\n",ErrorCode);
                RetOK = FALSE;    
            }
            else
            {
                printf("STVID_EnableSynchronisation OK\n");
            }
        }
    }


    /* ------------------------------------------------------ */
    /* PTI_AUDStart(); if PCR available                           */ 
    /* ------------------------------------------------------ */
    if (RetOK)
    {
        STAUD_StreamParams_t            StreamParams;
        STAUD_BufferParams_t            BufferParams;
    
        /* First Linking PTI and Audio driver */

        StreamParams.StreamContent      = STAUD_STREAM_CONTENT_MPEG2;
        StreamParams.StreamType         = STAUD_STREAM_TYPE_PES;
    	StreamParams.SamplingFrequency  = 48000;
    	StreamParams.StreamID           = 0xff;
    
        /* Get the PES Buffer params */
        ErrorCode = STAUD_IPGetInputBufferParams(AUD_Handle, STAUD_OBJECT_INPUT_CD0, &BufferParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("AUD_GetDataInputBufferParams fails: Error:%x\n",ErrorCode);
            return(FALSE);
        }

    	printf("BufferBaseAddr_p(%lu) BufferSize(%d)\n", (unsigned long) BufferParams.BufferBaseAddr_p, BufferParams.BufferSize);
 
        ErrorCode = STPTI_BufferAllocateManual(PTI_Handle,
                                                BufferParams.BufferBaseAddr_p, BufferParams.BufferSize,
                                                1, &PTI_AudioBufferHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("PTI_BufferAllocateManual fails \n");
            return(FALSE);
        }

        /* No cd-fifo: Initialise the link between pti and Audio */
        ErrorCode = STPTI_SlotLinkToBuffer(SlotHandle[PTICMD_BASE_AUDIO], PTI_AudioBufferHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("PTI_SlotLinkToBuffer fails \n");
            return(FALSE);
        }

        ErrorCode = STPTI_BufferSetOverflowControl(PTI_AudioBufferHandle, TRUE);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("PTI_BufferSetOverflowControl fails \n");
            return(FALSE);
        }

    	ErrorCode = STAUD_IPSetDataInputInterface(AUD_Handle, STAUD_OBJECT_INPUT_CD0,
                                                    NULL, NULL, (void*)PTI_AudioBufferHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STAUD_IPSetDataInputInterface fails \n");
            return(FALSE);
        }

        printf("Audio Linked to PTI...\n");
        
        /* Second: setting PID */
        
        ErrorCode = STPTI_SlotSetPid(SlotHandle[PTICMD_BASE_AUDIO+SlotNumber], AudioPid);
        if ( ErrorCode != ST_NO_ERROR )
        {
            printf("PTI_AudioStart(slot=%d,pid=%d): failed ! error=%d\n",
                         PTICMD_MAX_VIDEO_SLOT+SlotNumber, AudioPid, ErrorCode);
            RetOK = FALSE;
        }
        else
        {
            STTBX_Print(("PTI_AudioStart(slot=%d,pid=%d): done\n",
                         PTICMD_MAX_VIDEO_SLOT+SlotNumber, AudioPid ));
        }
   
    }

    /* ------------------------------------------------------ */
    /* aud_start() */
    /* ------------------------------------------------------ */
    
    if (RetOK)
    {
        STAUD_StreamParams_t StreamParams;
    
    	/*StreamParams.StreamContent      = (STAUD_StreamContent_t)STAUD_STREAM_CONTENT_AC3;*/
        if (AC3 == 1)
        {
    	    StreamParams.StreamContent      = (STAUD_StreamContent_t)STAUD_STREAM_CONTENT_AC3;
            StreamParams.StreamType         = (STAUD_StreamType_t)STAUD_STREAM_TYPE_ES;
        }
        else
        {
    	    StreamParams.StreamContent      = (STAUD_StreamContent_t)STAUD_STREAM_CONTENT_MPEG2;
            StreamParams.StreamType         = (STAUD_StreamType_t)STAUD_STREAM_TYPE_PES;
        }
        StreamParams.SamplingFrequency  = (U32)48000;
        StreamParams.StreamID           = (U8)STAUD_IGNORE_ID;
              
        ErrorCode = STAUD_DRStart(AUD_Handle, STAUD_OBJECT_INPUT_CD0, &StreamParams );
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        
        ErrorCode = STAUD_DRUnMute(AUD_Handle, STAUD_OBJECT_INPUT_CD0);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STAUD_Start OK\n");
        }
    }

    /* ------------------------------------------------------ */
    /* aud_sync() */
    /* ------------------------------------------------------ */

    if (RetOK)
    {
        STAUD_Object_t          DecodeObject, OutputObject;   
        S32                     STCOffset;
        STOS_Clock_t            AUDTimeout;

        /* Setting the CLKRV Offset */
        STCOffset = STANDARD_STC_SHIFT;
        ErrorCode = STCLKRV_SetSTCOffset(CLKRV_Handle, STCOffset);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("Call STCLKRV_SetSTCOffset with %d offset (RetCode:%x)\n", STCOffset, ErrorCode);
        }

        /* Muting */
        OutputObject = (STAUD_Object_t) STAUD_OBJECT_DECODER_COMPRESSED0;
        ErrorCode = STAUD_DRMute(AUD_Handle, OutputObject);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STAUD_DRMute OK\n");
        }

        /* Enabling synchronisation */
        DecodeObject = (STAUD_Object_t)STAUD_OBJECT_OUTPUT_PCMP0;
        //ErrorCode = STAUD_OPEnableSynchronization(AUD_Handle, DecodeObject);
        ErrorCode = STAUD_EnableSynchronisation(AUD_Handle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            //printf("STAUD_OPEnableSynchronization OK\n");
            printf("STAUD_EnableSynchronisation OK\n");
        }

        /* Checking if audio status is in sync */
        while (STOS_SemaphoreWaitTimeOut(AUDValidFlag_p, TIMEOUT_IMMEDIATE) == 0);
        AUDTimeout = STOS_time_plus(STOS_time_now(), AUD_STAUDLX_AUD_VALID_TIMEOUT);
        if (AUDState != AUD_STAUDLX_STATE_IN_SYNC)
        {
            AUDValidCheck = TRUE; /* enable check  */              
            
            if (semaphore_wait_timeout(AUDValidFlag_p, &AUDTimeout) == 0)
            {
                printf("STAUDLX AUD valid event\n");
            }
            else
            {
                printf("STAUDLX AUD timeout: STAUDLX not in sync !\n");
            }

            AUDValidCheck = FALSE; /* disabled check */

        }
        else
        {
            printf("STAUDLX in valid state\n");
        }

        /* Unmuting */
        ErrorCode = STAUD_DRUnMute(AUD_Handle, OutputObject);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STAUD_DRUnMute OK\n");
        }
    }
    /* ------------------------------------------------------ */
    return(RetOK);

} /* end sub_start */

/*******************************************************************************
Name        : sub_stop
Description : Function that stop channel all flows
Parameters  : 
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
BOOL sub_stop(void)
{
    BOOL               RetOK = TRUE;
    ST_ErrorCode_t     ErrorCode;

    /* ------------------------------------------------------ */
    /* Stop audio synchronization */
    /* ------------------------------------------------------ */
    if (RetOK)
    {
        STAUD_Object_t DecodeObject;   

        /* Setting the CLKRV Offset */
        DecodeObject = (STAUD_Object_t)STAUD_OBJECT_OUTPUT_PCMP0;

        //ErrorCode = STAUD_OPDisableSynchronization(AUD_Handle, DecodeObject);
        ErrorCode = STAUD_DisableSynchronisation(AUD_Handle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            //printf("STAUD_OPDisableSynchronization OK\n");
            printf("STAUD_DisableSynchronisation OK\n");
        }
    }     
    
    /* ------------------------------------------------------ */
    /* Stop video synchronization */
    /* ------------------------------------------------------ */
    if (RetOK)
    {
        ErrorCode = STVID_DisableSynchronisation(VID_Handle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Sync: Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVID_DisableSynchronisation OK\n");
        }
    }    

    /* ------------------------------------------------------ */
    /* Stop audio */
    /* ------------------------------------------------------ */
    if (RetOK)
    {
	    STAUD_Stop_t StopMode;
	    STAUD_Fade_t Fade;
	    
	    /* Default */
	    StopMode = STAUD_STOP_NOW;
	    
        ErrorCode = STAUD_DRStop(AUD_Handle, STAUD_OBJECT_INPUT_CD0, StopMode, &Fade );
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STAUD_DRStop OK\n");
        }
    }

    /* ------------------------------------------------------ */
    /* Stop video */
    /* ------------------------------------------------------ */

    if (RetOK)
    {
        STVID_Stop_t StopMode;
        STVID_Freeze_t Freeze;

        /* default modes */
        StopMode = STVID_STOP_NOW;
        Freeze.Mode = STVID_FREEZE_MODE_FORCE;
        Freeze.Field = STVID_FREEZE_FIELD_TOP;

        ErrorCode = STVID_Stop(VID_Handle, StopMode, &Freeze );
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> Error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            printf("STVID_Stop OK\n");
        }
    }

    /* ------------------------------------------------------ */
    /* Stop PTI for video & audio */
    /* ------------------------------------------------------ */

    if (RetOK)
    {
        /* Video */
        ErrorCode = STPTI_SlotClearPid(SlotHandle[PTICMD_BASE_VIDEO+SlotNumber]);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("---> PTI_vidstop error:%x\n",ErrorCode);
            RetOK = FALSE;    
        }
        else
        {
            /* Audio */
            ErrorCode = STPTI_SlotClearPid(SlotHandle[PTICMD_BASE_AUDIO+SlotNumber]);
            if (ErrorCode != ST_NO_ERROR)
            {
                printf("---> PTI_audstop error:%x\n",ErrorCode);
                RetOK = FALSE;    
            }
            else
            {
                /* Caution: return from STPTI_SlotClearPid() is immediate, but there can
                   still be transfer of last packect going on. So wait a little bit. */
                /* at least this should be done for the STVID_INJECTION_BREAK_WORKAROUND */
                task_delay(MaxDelayForLastPacketTransferWhenStptiPidCleared);
                printf("Video & Audio PTI slots cleared\n");
            }
        }                
    }            

 
    /* ------------------------------------------------------ */
    return(RetOK);
}

/*******************************************************************************
Name        : channel_change
Description : Function that changes channel (video, audio & pcr PIDs)
Parameters  : 
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
BOOL channel_change(int NewVideoPID, int NewAudioPID, int NewPcrPID)
{
    BOOL               RetOK = TRUE;

    /* Stopping flows */
    RetOK &= sub_stop();
    
    /* Restarting with new parameters */
    RetOK &= sub_start(NewVideoPID, NewAudioPID, NewPcrPID);

    /* ------------------------------------------------------ */
    if (RetOK == FALSE)
    {
        printf("---> channel_change error\n");
    }
    
    return(RetOK);
    
} /* channel_change */

/*******************************************************************************
Name        : enddown
Description : Main end function 
Parameters  : 
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
void enddown()
{
    printf("Ending all\n");

    /* ------------------------------------------------------ */
    /* aud_nosync();                                         */ 
    /* ------------------------------------------------------ */
    
    {
        STAUD_Object_t DecodeObject;   

        DecodeObject = (STAUD_Object_t)STAUD_OBJECT_OUTPUT_PCMP0;
        //STAUD_OPDisableSynchronization(AUD_Handle, DecodeObject);
        STAUD_DisableSynchronisation(AUD_Handle);

        printf("aud_nosync done\n");
    }

    /* ------------------------------------------------------ */
    /* aud_stop();                                         */ 
    /* ------------------------------------------------------ */
    
    {
        STAUD_Stop_t StopMode;
        STAUD_Fade_t Fade;

        StopMode = STAUD_STOP_NOW;
        STAUD_DRStop(AUD_Handle, STAUD_OBJECT_INPUT_CD0, StopMode, &Fade );
        printf("STAUD_DRStop done\n");
    } 
    
    /* ------------------------------------------------------ */
    /* PTI_AUDStop();                                         */ 
    /* ------------------------------------------------------ */

    STPTI_SlotClearPid(SlotHandle[PTICMD_BASE_AUDIO+SlotNumber]);
    printf("PTI_AUDStop done\n");
    
    /* ------------------------------------------------------ */
    /* VID_nosync();                                         */ 
    /* ------------------------------------------------------ */

    STVID_DisableSynchronisation(VID_Handle);
    /* Caution: return from STPTI_SlotClearPid() is immediate, but there can
       still be transfer of last packect going on. So wait a little bit. */
    /* at least this should be done for the STVID_INJECTION_BREAK_WORKAROUND */
    task_delay(MaxDelayForLastPacketTransferWhenStptiPidCleared);
    printf("VID_nosync done\n");

    /* ------------------------------------------------------ */
    /* VID_stop();                                         */ 
    /* ------------------------------------------------------ */

    {
        STVID_Stop_t    StopMode;
        STVID_Freeze_t  Freeze;
        
        Freeze.Field = STVID_FREEZE_FIELD_NEXT;
        Freeze.Mode = STVID_STOP_NOW;
        StopMode = STVID_STOP_NOW;
        
        STVID_Stop(VID_Handle, StopMode, &Freeze);
        printf("VID_Stop done\n");
    }
    getchar();

    /* ------------------------------------------------------ */
    /* PTI_VIDStop();                                         */ 
    /* ------------------------------------------------------ */

    STPTI_SlotClearPid(SlotHandle[PTICMD_BASE_VIDEO+SlotNumber]);
    task_delay(MaxDelayForLastPacketTransferWhenStptiPidCleared);
    printf("PTI_VideoStop done\n");
    getchar();
    
    printf("Closing down all drivers\n");

    VID_Term();
    VMIX_Term();
    LAYER_Term();
    VOUT_Term();
    VTG_Term();
    DENC_Term();
    AUD_Term();
    getchar();
    PTI_Term();
    getchar();
    MERGE_Term();
    getchar();
    VIDTEST_Term();
    getchar();
    CLKRV_Term();
    getchar();
    EVT_Term();
    getchar();

} /* end enddown */




/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */





/* ******************************************************************** */


/* End of ref_sync.c */
