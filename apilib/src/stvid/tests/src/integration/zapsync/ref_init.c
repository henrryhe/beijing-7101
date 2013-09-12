/* ************************************************************************* */
/* Init functions                                                            */
/* ************************************************************************* */

#include "ref_sync.h"
#include "ref_init.h"

/* Global Variables --------------------------------------------------------- */

STPTI_Slot_t             SlotHandle[PTICMD_MAX_SLOTS];
BOOL                     PCRValidCheck = FALSE; /* Flag to enable the check of STCLKRV state machine */
BOOL                     PCRInvalidCheck = FALSE; /* Flag to enable the check of STCLKRV state machine */
semaphore_t              *PCRValidFlag_p;
semaphore_t              *PCRInvalidFlag_p;

BOOL                     AUDValidCheck = FALSE; /* Flag to enable the check of STAUDLX state machine */
semaphore_t              *AUDValidFlag_p;

LocalDACConfiguration_t  LocalDACConfiguration;

STEVT_Handle_t           EVT_Handle;
STAUD_Handle_t           AUD_Handle;
STCLKRV_Handle_t         CLKRV_Handle;
STDENC_Handle_t          DENC_Handle;
STVOUT_Handle_t          VOUT_Handle;
STVOUT_Handle_t          VOUT_Handle_HD;
STVTG_Handle_t           VTG_Handle;
STVMIX_Handle_t          VMIX_Handle;
STLAYER_Handle_t         LAYER_Handle;
STVID_Handle_t           VID_Handle;
STPTI_Handle_t           PTI_Handle;

/* Extern Variables --------------------------------------------------------- */

extern int               *SystemPartition_p;
extern int               *NCachePartition_p;
extern int               *DriverPartition_p;
extern int               PCRState;
extern int               AUDState;
extern REF_Codec_e       Codec;

extern int               HD;
/* ************************************************************************* */
/* Callback functions                                                        */
/* ************************************************************************* */

/*****************************************************************************
Name        : CallBack_STAUD_IN_SYNC_EVT
Description : 
Parameters  : None
Assumptions :
Limitations :
Returns     : 
*****************************************************************************/
void CallBack_STAUD_IN_SYNC_EVT(
                            STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p)
{
/*    printf("AUDLX: In sync\n");*/

    AUDState = AUD_STAUDLX_STATE_IN_SYNC;   
    
    if (AUDValidCheck == TRUE)
    {
        /* We want to get a signal if we fall into this state */
        semaphore_signal(AUDValidFlag_p);
    }

    return;
}

/*****************************************************************************
Name        : CallBack_STAUD_OUTOF_SYNC_EVT
Description : 
Parameters  : None
Assumptions :
Limitations :
Returns     : 
*****************************************************************************/
void CallBack_STAUD_OUTOF_SYNC_EVT(
                            STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p)
{
/*    printf("AUDLX: Out of sync\n");*/

    AUDState = AUD_STAUDLX_STATE_OUT_OF_SYNC;   
    
    return;
}

/*****************************************************************************
Name        : CallBack_STCLKRV_PCR_VALID_EVT
Description : 
Parameters  : None
Assumptions :
Limitations :
Returns     : 
*****************************************************************************/
void CallBack_STCLKRV_PCR_VALID_EVT(
                            STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p)
{
/*    STTBX_Print(("CLKRV: PCR_VALID_EVT\n"));*/

    PCRState = PTI_STCLKRV_STATE_PCR_OK;   
    
    if (PCRValidCheck == TRUE)
    {
        /* We want to get a signal if we fall into this state */
        semaphore_signal(PCRValidFlag_p);
    }

    return;
}

/*****************************************************************************
Name        : CallBack_STCLKRV_PCR_DISCONTINUITY_EVT
Description : 
Parameters  : None
Assumptions :
Limitations :
Returns     : 
*****************************************************************************/
void CallBack_STCLKRV_PCR_DISCONTINUITY_EVT(
                            STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p)
{
/*    STTBX_Print(("CLKRV: PCR_DISCONTINUITY_EVT\n"));*/
    PCRState = PTI_STCLKRV_STATE_PCR_INVALID;   

    if (PCRInvalidCheck == TRUE)
    {
        /* We want to get a signal if we fall into this state */
        semaphore_signal(PCRInvalidFlag_p);
    }
   
    return;
}

/*******************************************************************************
Name        : EVT_Init
Description : Init the STEVT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/

BOOL  EVT_Init()
{
    BOOL                              RetOK = TRUE;
    ST_ErrorCode_t                    ErrorCode;
    STEVT_InitParams_t                EVTInitParams;
    STEVT_OpenParams_t                EVTOpenParams;

    /* Initialize the Event Handle */
    EVTInitParams.EventMaxNum       = EVT_EVENT_MAX_NUM;
    EVTInitParams.ConnectMaxNum     = EVT_CONNECT_MAX_NUM;
    EVTInitParams.SubscrMaxNum      = EVT_SUBSCR_MAX_NUM;
    EVTInitParams.MemoryPartition   = DriverPartition_p;
    EVTInitParams.MemorySizeFlag    = STEVT_UNKNOWN_SIZE;
    EVTInitParams.MemoryPoolSize    = 0;

    ErrorCode = STEVT_Init(STEVT_DEVICE_NAME,&EVTInitParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        printf("STEVT_Init() failed: error=0x%0x !\n", ErrorCode);
        RetOK = FALSE;
    }
    else
    {
        ErrorCode = STEVT_Open(STEVT_DEVICE_NAME, &EVTOpenParams, &EVT_Handle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STEVT_Open for STCLKRV subscription failed. Error 0x%x ! ", ErrorCode);
            RetOK = FALSE;
        }
        else
        {
            printf("STEVT initialized, \trevision=%s\n",STEVT_GetRevision());
        }
            
    }
    
    return(RetOK);
}

/*******************************************************************************
Name        : VIDTEST_LocalInit
Description : Initialise VIDTEST
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/
BOOL VIDTEST_LocalInit()
{
    BOOL                              RetOK = TRUE;
    ST_ErrorCode_t                    ErrorCode;
    ST_DeviceName_t                   DeviceName;
    STVIDTEST_InitParams_t            InitParams;

    strcpy((char *)DeviceName, STVIDTEST_DEVICE_NAME);

    /* We can initialize with some parameters if needed */
    InitParams.InitFlag = TRUE; /* Dummy: just to show how to use it */
   
    ErrorCode = STVIDTEST_Init(DeviceName, &InitParams);
    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STVIDTEST initialized\n");
    }
    else
    {
        printf("STVIDTEST() failed: error=0x%0x !\n", ErrorCode);
        RetOK = FALSE;
    }
    return(RetOK);

} 

/*******************************************************************************
Name        : MERGE_Init
Description : Init the STMERGE driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/

BOOL MERGE_Init()
{
    STMERGE_InitParams_t            STMInitParams;
    STMERGE_ObjectConfig_t          TSIN;
    U32 TSMERGER_SRAM_MemoryMap[6][2] = {
                                        {STMERGE_TSIN_0,6*128},
                                        {STMERGE_TSIN_1,6*128},
                                        {STMERGE_TSIN_2,1*128},
                                        {STMERGE_SWTS_0,6*128},
                                        {STMERGE_ALTOUT_0,1*128},
                                        {0,0}
                                        };

    BOOL                            RetOK = TRUE;
    ST_ErrorCode_t                  ErrorCode;

	/* Initialise TSMerge so TSIN bypass block */
    printf( "STMERGE initializing, \trevision=%-21.21s ...\n", STMERGE_GetRevision() );

    STMInitParams.DeviceType = STMERGE_DEVICE_1;
    STMInitParams.DriverPartition_p = DriverPartition_p;
    STMInitParams.BaseAddress_p = (U32 *)ST7100_TSMERGE_BASE_ADDRESS;
    STMInitParams.MergeMemoryMap_p = TSMERGER_SRAM_MemoryMap;
    //STMInitParams.Mode = STMERGE_NORMAL_OPERATION;
    STMInitParams.Mode = STMERGE_NORMAL_OPERATION_TO_PTI0;
    
    ErrorCode = STMERGE_Init("MERGE0",&STMInitParams);

    if (ErrorCode != ST_NO_ERROR)
    {
        printf("%s init. error 0x%x ! ", "MERGE0", ErrorCode);
        RetOK = FALSE;
    }

    /* SETUP_TSIN0 */
    printf("NORMAL mode, Setup for TSin0\n");
    TSIN.Priority = STMERGE_PRIORITY_HIGH;
    TSIN.SyncLock = 0;
    TSIN.SyncDrop = 0;
    TSIN.SOPSymbol = 0x47;
    TSIN.PacketLength = STMERGE_PACKET_LENGTH_DVB;
    TSIN.u.TSIN.SerialNotParallel = TRUE;
    TSIN.u.TSIN.SyncNotAsync = FALSE;
    TSIN.u.TSIN.InvertByteClk = FALSE;
    TSIN.u.TSIN.ByteAlignSOPSymbol = TRUE;
    TSIN.u.TSIN.ReplaceSOPSymbol = FALSE;
    TSIN.SyncLock = 3;
    TSIN.SyncDrop = 3;
    TSIN.u.TSIN.SerialNotParallel = FALSE;
    TSIN.u.TSIN.ByteAlignSOPSymbol = FALSE;

    ErrorCode = STMERGE_SetParams(STMERGE_TSIN_0, &TSIN);

    if (ErrorCode != ST_NO_ERROR)
    {
        printf("%s set params error 0x%x for TSin0 ! \n", "MERGE0", ErrorCode);
        RetOK = FALSE;
    }

    /* Connect to PTI */
    ErrorCode = STMERGE_Connect(STMERGE_TSIN_0, STMERGE_PTI_0);
    if (ErrorCode != ST_NO_ERROR)
    {
        printf("%s connect TSIN_0 to PTI_0 error 0x%x for TSin1 ! \n", "MERGE0", ErrorCode);
        RetOK = FALSE;
    }

    return(RetOK);

}

/*******************************************************************************
Name        : PTI_Init
Description : Init the STPTI driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/

BOOL PTI_Init()
{
    STPTI_InitParams_t              STPTIInitParams;
    STPTI_OpenParams_t              STPTIOpenParams;
    STPTI_SlotData_t                SlotData;

    STEVT_DeviceSubscribeParams_t   DevSubscribeParams;

    BOOL                            RetOK = TRUE;
    ST_ErrorCode_t                  ErrorCode;
    
    U32                             i;
    
    printf( "STPTI initializing...\n");

    /* --- Driver initialization --- */
    printf("Device adress      %x \n" , ST7100_PTI_BASE_ADDRESS);
    strcpy( STPTIInitParams.EventHandlerName, STEVT_DEVICE_NAME);

    STPTIInitParams.InterruptNumber            = ST7100_PTI_INTERRUPT;

    memset(&STPTIInitParams,0,sizeof(STPTI_InitParams_t));

    STPTIInitParams.Partition_p                = DriverPartition_p;
    STPTIInitParams.NCachePartition_p          = NCachePartition_p;

    STPTIInitParams.EventProcessPriority       = 1;
    STPTIInitParams.InterruptProcessPriority   = 14;

    STPTIInitParams.Device                     = 0; /* The driver knows */
    STPTIInitParams.TCLoader_p                 = NULL; /* The driver knows */
    STPTIInitParams.TCDeviceAddress_p          = (U32 *)0; /* The driver knows */
    STPTIInitParams.SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_16x8;

    STPTIInitParams.SyncLock                   = 0;
    STPTIInitParams.SyncDrop                   = 0;

    STPTIInitParams.TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
    STPTIInitParams.DescramblerAssociation     = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;

    STPTIInitParams.StreamID                   = STPTI_STREAM_ID_TSIN0;

    STPTIInitParams.NumberOfSlots              = 10;
    STPTIInitParams.AlternateOutputLatency     = 0;
    STPTIInitParams.DiscardOnCrcError          = TRUE;

    STPTIInitParams.PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_TSMERGER;

    strcpy(STPTIInitParams.EventHandlerName, STEVT_DEVICE_NAME);

    ErrorCode = STPTI_Init(STPTI_DEVICE_NAME, &STPTIInitParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        printf("%s init. error 0x%x ! ", STPTI_DEVICE_NAME, ErrorCode);
        RetOK = FALSE;
    }
    else
    {
        printf("%s Initialized, \trevision=%-21.21s\n", STPTI_DEVICE_NAME, STPTI_GetRevision());
    }

    /* --- Event from clkrv setting --- */

    if (RetOK)
    {
        /* The semaphore must not be signaled */
        PCRValidCheck = FALSE;

        /* callback subcription */
        DevSubscribeParams.SubscriberData_p = NULL;
        DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) CallBack_STCLKRV_PCR_VALID_EVT ;
    
        ErrorCode = STEVT_SubscribeDeviceEvent(EVT_Handle,
                                                     STCLKRV_DEVICE_NAME, 
                                                     (STEVT_EventConstant_t) STCLKRV_PCR_VALID_EVT,
                                                     &DevSubscribeParams);

        if (ErrorCode != ST_NO_ERROR)
        {
            printf ("Failed to subscribe STCLKRV_PCR_VALID_EVT  !!\n" );
        }
        else
        {
            printf ("STCLKRV_PCR_VALID_EVT subcribed\n");
        }

        DevSubscribeParams.SubscriberData_p = NULL;
        DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) CallBack_STCLKRV_PCR_DISCONTINUITY_EVT ;
    
        ErrorCode = STEVT_SubscribeDeviceEvent(EVT_Handle,
                                                     STCLKRV_DEVICE_NAME, 
                                                     (STEVT_EventConstant_t) STCLKRV_PCR_DISCONTINUITY_EVT,
                                                     &DevSubscribeParams);

        if (ErrorCode != ST_NO_ERROR)
        {
            printf ("Failed to subscribe STCLKRV_PCR_DISCONTINUITY_EVT  !!\n" );
        }
        else
        {
            printf ("STCLKRV_PCR_DISCONTINUITY_EVT subcribed\n");
        }
    }

    /* --- Connection opening --- */

    if (RetOK)
    {
        memset(&STPTIOpenParams,0,sizeof(STPTI_OpenParams_t));
        STPTIOpenParams.DriverPartition_p = DriverPartition_p;
        STPTIOpenParams.NonCachedPartition_p = NCachePartition_p;

        ErrorCode = STPTI_Open(STPTI_DEVICE_NAME, &STPTIOpenParams, &PTI_Handle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STPTI_Open(%s,hndl=%d) : failed ! error=%d ",
                         STPTI_DEVICE_NAME, PTI_Handle, ErrorCode);
            RetOK = FALSE;
        }
        else
        {
            printf("                                 %s opened as device 0, hndl=%d\n",
                     STPTI_DEVICE_NAME, PTI_Handle);
        }
    }

    /* --------------------- */
    /* --- Slots opening --- */
    /* --------------------- */

    /* Allocation for VIDEO */
    SlotData.SlotType = STPTI_SLOT_TYPE_PES;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE; /* ?value: not supported in DVB */
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.SoftwareCDFifo = TRUE;

    /* Initialise all video slots */
    for(i=PTICMD_BASE_VIDEO; (i<PTICMD_BASE_VIDEO+PTICMD_MAX_VIDEO_SLOT) && RetOK; i++)
    {
        ErrorCode = STPTI_SlotAllocate(PTI_Handle, &SlotHandle[i], &SlotData);
        if ( ErrorCode != ST_NO_ERROR )
        {
            printf("STPTI_SlotAllocate(hndl=%d) : failed for video ! error=%d ",
                         PTI_Handle, ErrorCode);
            RetOK = FALSE;
        }
        else
        {
            /* Initialize video slot */
            ErrorCode = STPTI_SlotClearPid(SlotHandle[i]);
            if ( ErrorCode != ST_NO_ERROR )
            {
                printf("STPTI_SlotClearPid(slotHndl=%d) : failed for video ! error=%d ",
                             SlotHandle[i], ErrorCode);
                RetOK = FALSE;
            }
        }
    }
    if (RetOK)
    {
        printf("                                 %d video slot(s) ready for PTI device=0\n",
                 PTICMD_MAX_VIDEO_SLOT);
    }

    /* Allocation for AUDIO */
    SlotData.SlotType = STPTI_SLOT_TYPE_PES;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;

    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;

    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData       = FALSE;
    SlotData.SlotFlags.SoftwareCDFifo = TRUE;

    /* Initialise all audio slots */
    for(i=PTICMD_BASE_AUDIO; (i<(PTICMD_BASE_AUDIO+PTICMD_MAX_AUDIO_SLOT)) && RetOK; i++)
    {
        ErrorCode = STPTI_SlotAllocate(PTI_Handle, &SlotHandle[i], &SlotData);
        if ( ErrorCode != ST_NO_ERROR )
        {
            printf("STPTI_SlotAllocate(hndl=%d) : failed for audio ! error=%d ",
                         PTI_Handle, ErrorCode);
            RetOK = FALSE;
        }
        else
        {
            ErrorCode = STPTI_SlotClearPid(SlotHandle[i]);
            if ( ErrorCode != ST_NO_ERROR )
            {
                printf("STPTI_SlotClearPid(slotHndl=%d) : failed for audio ! error=%d ",
                             SlotHandle[i], ErrorCode);
                RetOK = FALSE;
            }
        }
    }
    if (RetOK)
    {
        printf("                                 %d audio slot(s) ready for PTI device=0g\n",
                 PTICMD_MAX_AUDIO_SLOT);
    }

    /* Allocation for PCR */
    SlotData.SlotType = STPTI_SLOT_TYPE_PCR;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;

    /* Initialise all pcr slots */
    for(i=PTICMD_BASE_PCR; (i<(PTICMD_BASE_PCR+PTICMD_MAX_PCR_SLOT)) && RetOK; i++)
    {
        ErrorCode = STPTI_SlotAllocate( PTI_Handle, &SlotHandle[i], &SlotData);
        if ( ErrorCode != ST_NO_ERROR )
        {
            printf("STPTI_SlotAllocate(hndl=%d) : failed ! error=%d ",
                         PTI_Handle, ErrorCode);
            RetOK = FALSE;
        }
        /* Don't clear PCR ?? Why ??*/
    }
    if (RetOK)
    {
        printf("                                 %d pcr slot(s) ready for PTI device=0\n",
                 PTICMD_MAX_PCR_SLOT);
    }

    PCRValidFlag_p = semaphore_create_fifo_timeout(0);
    PCRInvalidFlag_p = semaphore_create_fifo_timeout(0);

    return(RetOK);
    
} /* end of PTI_Init() */

/*******************************************************************************
Name        : CLKRV_Init
Description : Init the STCLKRV driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/
BOOL CLKRV_Init(void)
{
    BOOL                            RetOK = TRUE;
    STCLKRV_InitParams_t            stclkrv_InitParams_s;
    STCLKRV_OpenParams_t            stclkrv_OpenParams;

    /* Initialization & Open code fragments */
    /* assumed that all dependent drivers are already initialised */

    /* Specific initializations (with 5.0.0 revision and later) */

    stclkrv_InitParams_s.DeviceType         = STCLKRV_DEVICE_TYPE_7100;
    stclkrv_InitParams_s.Partition_p        = DriverPartition_p;

    /* STPTI & Clock recovery Events */
    strcpy( stclkrv_InitParams_s.EVTDeviceName,     STEVT_DEVICE_NAME);
    strcpy( stclkrv_InitParams_s.PCREvtHandlerName, STEVT_DEVICE_NAME);
    strcpy( stclkrv_InitParams_s.PTIDeviceName,     STPTI_DEVICE_NAME);

    /* Clock recovery Filter */
    stclkrv_InitParams_s.PCRMaxGlitch   = STCLKRV_PCR_MAX_GLITCH;
    stclkrv_InitParams_s.PCRDriftThres  = STCLKRV_PCR_DRIFT_THRES;
    stclkrv_InitParams_s.MinSampleThres = STCLKRV_MIN_SAMPLE_THRESHOLD;
    stclkrv_InitParams_s.MaxWindowSize  = STCLKRV_MAX_WINDOW_SIZE;
    stclkrv_InitParams_s.InterruptNumber= ST7100_MPEG_CLK_REC_INTERRUPT;
    stclkrv_InitParams_s.InterruptLevel = 6;

    /* device type dependant parameters */
    stclkrv_InitParams_s.FSBaseAddress_p  = (U32 *)ST7100_CKG_BASE_ADDRESS;  /* SD/PCM/SPDIF or HD clock FS and clkrv tracking registers */
    stclkrv_InitParams_s.AUDCFGBaseAddress_p = (U32 *)ST7100_AUDIO_IF_BASE_ADDRESS ;

    if( STCLKRV_Init(STCLKRV_DEVICE_NAME, &stclkrv_InitParams_s) != ST_NO_ERROR )
    {
        RetOK = FALSE;
        printf("CLKRV_Init() failed !\n");
    }
    else
    {
        if( STCLKRV_Open(STCLKRV_DEVICE_NAME, &stclkrv_OpenParams, &CLKRV_Handle) != ST_NO_ERROR)
        {
            printf("CLKRV_Open() failed !\n");
            RetOK = FALSE;
        }
        else
        {
         printf("STCLKRV initialized, \trevision=%s\n", STCLKRV_GetRevision() );
        }
    }
    
    return(RetOK);
   
}

/*******************************************************************************
Name        : AUD_Init
Description : Init the STAUDLT/LX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/
BOOL AUD_Init(void)
{
    BOOL                            RetOK = TRUE;
    ST_ErrorCode_t                  ErrorCode;
    STAUD_InitParams_t              STAUD_InitParams;
    STAUD_OpenParams_t              STAUD_OpenParams;
    STEVT_SubscribeParams_t         SubscribeParams;

	memset(&STAUD_InitParams, 0, sizeof(STAUD_InitParams_t));
	STAUD_InitParams.DeviceType                                   = STAUD_DEVICE_STI7100;
	STAUD_InitParams.PCMInterruptNumber                           = 530;
	STAUD_InitParams.PCMInterruptLevel                            = 0;
	STAUD_InitParams.InterruptNumber                              = 0;
	STAUD_InitParams.InterruptLevel                               = 0;   
	STAUD_InitParams.InterfaceType                                = STAUD_INTERFACE_EMI;
	STAUD_InitParams.Configuration                                = STAUD_CONFIG_DVB_COMPACT; 
	STAUD_InitParams.CD1BufferAddress_p                           = NULL;
	STAUD_InitParams.CD1BufferSize                                = 0;
	STAUD_InitParams.CD2BufferAddress_p                           = NULL;
	STAUD_InitParams.CD2BufferSize                                = 0;
	STAUD_InitParams.CDInterfaceType                              = STAUD_INTERFACE_EMI;
	STAUD_InitParams.InternalPLL                                  = TRUE;
	STAUD_InitParams.DACClockToFsRatio                            = 256; 
	STAUD_InitParams.PCMOutParams.InvertWordClock                 = FALSE; 
	STAUD_InitParams.PCMOutParams.Format                          = STAUD_DAC_DATA_FORMAT_I2S;
	STAUD_InitParams.PCMOutParams.InvertBitClock                  = FALSE;
	STAUD_InitParams.PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_24BITS;
	STAUD_InitParams.PCMOutParams.Alignment                       = STAUD_DAC_DATA_ALIGNMENT_LEFT_SIGNED;
	STAUD_InitParams.PCMOutParams.MSBFirst                        = TRUE;
	STAUD_InitParams.SPDIFOutParams.AutoLatency					= TRUE;
	STAUD_InitParams.SPDIFOutParams.AutoCategoryCode				= TRUE;
	STAUD_InitParams.SPDIFOutParams.AutoDTDI						= TRUE;
	STAUD_InitParams.SPDIFOutParams.CopyPermitted					= STAUD_COPYRIGHT_MODE_NO_COPY;
	STAUD_InitParams.CD2InterfaceType                             = STAUD_INTERFACE_EMI;
	STAUD_InitParams.MaxOpen                                      = 1;
	STAUD_InitParams.CPUPartition_p                               = SystemPartition_p;
    STAUD_InitParams.AVMEMPartition                               = 0;
	STAUD_InitParams.CD1InterruptNumber                           = 0; 
	STAUD_InitParams.CD2InterruptNumber                           = 0; 
	STAUD_InitParams.CD1InterruptLevel                            = 0; 
	STAUD_InitParams.InterfaceParams.EMIParams.BaseAddress_p      = NULL; 
	STAUD_InitParams.InterfaceParams.EMIParams.RegisterWordWidth  = STAUD_WORD_WIDTH_32; 
	STAUD_InitParams.PCM1QueueSize                                = 5; 
	STAUD_InitParams.PCMMode                                      = PCM_ON;
	STAUD_InitParams.SPDIFMode                                    = STAUD_DIGITAL_MODE_NONCOMPRESSED ;/*STAUD_DIGITAL_MODE_COMPRESSED;*/
	STAUD_InitParams.UseInternalDac                               = FALSE;
	
	strcpy(STAUD_InitParams.EvtHandlerName, STEVT_DEVICE_NAME);
	strcpy(STAUD_InitParams.ClockRecoveryName, STCLKRV_DEVICE_NAME);

    STAUD_InitParams.PCMMode=PCM_ON;/*PCM_OFF;*/

    printf("AUD_Init(%s)=", STAUD_DEVICE_NAME );

    ErrorCode = STAUD_Init(STAUD_DEVICE_NAME, &STAUD_InitParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        printf("(error)%d\n", (ErrorCode) );
        return (FALSE);
    }

    printf("%s\n", STAUD_GetRevision() );

    STAUD_OpenParams.SyncDelay = 0;

    ErrorCode = STAUD_Open(STAUD_DEVICE_NAME,&STAUD_OpenParams, &AUD_Handle);

    if (ErrorCode != ST_NO_ERROR)
    {
        RetOK = FALSE;
    }
    else
    {
        printf("AUD_Open OK\n");
    }

    /* Init of callbacks */
    if (RetOK)
    {
        /* The semaphore must not be signaled */
        AUDValidCheck = FALSE;

        /* callback subcription */
        SubscribeParams.NotifyCallback   = (STEVT_CallbackProc_t) CallBack_STAUD_IN_SYNC_EVT ;
    
        ErrorCode = STEVT_Subscribe(EVT_Handle,
                                                     (STEVT_EventConstant_t) STAUD_IN_SYNC,
                                                     &SubscribeParams);

        if (ErrorCode != ST_NO_ERROR)
        {
            printf ("Failed to subscribe STAUD_IN_SYNC event !!\n" );
            RetOK = FALSE;
        }
        else
        {
            printf ("STAUD_IN_SYNC event subcribed\n");
        }

        SubscribeParams.NotifyCallback   = (STEVT_CallbackProc_t) CallBack_STAUD_OUTOF_SYNC_EVT ;
    
        ErrorCode = STEVT_Subscribe(EVT_Handle,
                                                     (STEVT_EventConstant_t) STAUD_OUTOF_SYNC,
                                                     &SubscribeParams);

        if (ErrorCode != ST_NO_ERROR)
        {
            printf ("Failed to subscribe STAUD_OUTOF_SYNC_EVT  !!\n" );
            RetOK = FALSE;
        }
        else
        {
            printf ("STAUD_OUTOF_SYNC_EVT subcribed\n");
        }
    }

    AUDValidFlag_p = semaphore_create_fifo_timeout(0);

    return(RetOK);

}

/*******************************************************************************
Name        : VTG_Init
Description : Init the STVTG driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/
BOOL VTG_Init(void)
{
    BOOL                            RetOK = TRUE;
    ST_ErrorCode_t                  ErrorCode;
    STVTG_InitParams_t VTGInitParams;
    STVTG_OpenParams_t VTGOpenParams;

    VTGInitParams.DeviceType = STVTG_DEVICE_TYPE_VTG_CELL_7100;

    VTGInitParams.Target.VtgCell3.BaseAddress_p = (void*)ST7100_VTG1_BASE_ADDRESS;
    VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)0;
    VTGInitParams.Target.VtgCell3.InterruptNumber = ST7100_VTG_0_INTERRUPT;
    VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;
    VTGInitParams.Target.VtgCell2.InterruptLevel      = 7;

    VTGInitParams.MaxOpen = 2;

    strcpy(VTGInitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    strcpy(VTGInitParams.Target.VtgCell3.ClkrvName, STCLKRV_DEVICE_NAME);

    printf("VTG_Init(%s)\n", STVTG_DEVICE_NAME);
    ErrorCode = STVTG_Init(STVTG_DEVICE_NAME, &VTGInitParams);
    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STVG Init Ok \n");
        ErrorCode = STVTG_Open(STVTG_DEVICE_NAME, &VTGOpenParams, &VTG_Handle);
        if (ErrorCode == ST_NO_ERROR)
        {
            printf("STVG Open =%d \n", VTG_Handle);
        }
        else
        {
            RetOK = FALSE;
        }
    }
    else
    {
        RetOK = FALSE;
    }
    
    return(RetOK);
}

/*******************************************************************************
Name        : VOUT_Init
Description : Init the STVOUT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/
BOOL VOUT_Init(void)
{
    BOOL                            RetOK = TRUE;
    ST_ErrorCode_t                  ErrorCode;
    STVOUT_OpenParams_t             VOUTOpenParams;
    STVOUT_InitParams_t             VOUTInitParams;
 
    VOUTInitParams.DeviceType = STVOUT_DEVICE_TYPE_7100 ;
    if (HD != 0)
        VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_RGB;
    else
        VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;

    strcpy(VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);

    VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*) ST7100_VOUT_BASE_ADDRESS;
    VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)0;
    if (HD != 0)
        VOUTInitParams.Target.DualTriDacCell.DacSelect = 3;
    else
        VOUTInitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_3;
    VOUTInitParams.Target.DualTriDacCell.HD_Dacs = 0;

    VOUTInitParams.CPUPartition_p  = DriverPartition_p;
    VOUTInitParams.MaxOpen         = 3;

    /* init */
    ErrorCode = STVOUT_Init(STVOUT_DEVICE_NAME, &VOUTInitParams);

    if (ErrorCode != ST_NO_ERROR)
    {
        printf("VOUT Init error\n");
        RetOK = FALSE;
    }
    
    STVOUT_Open(STVOUT_DEVICE_NAME, &VOUTOpenParams, &VOUT_Handle);

    if (ErrorCode != ST_NO_ERROR)
    {
        printf("VOUT Open error\n");
        RetOK = FALSE;
    }
    else
    {
        //STVOUT_SetInputSource(VOUT_Handle,0);
        printf("VOUT Init & Open OK\n");
    }
    
    if (HD != 0)
    {
        printf("Initializing VOUT for HD...\n");
        ErrorCode = STVOUT_Init(STVOUT_DEVICE_NAME_HD, &VOUTInitParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("VOUT Init for HD error\n");
            RetOK = FALSE;
        }
        ErrorCode = STVOUT_Open(STVOUT_DEVICE_NAME_HD, &VOUTOpenParams, &VOUT_Handle_HD);

        if (ErrorCode != ST_NO_ERROR)
        {
            printf("VOUT Open for HD error\n");
            RetOK = FALSE;
        }
    }

    return(RetOK);
}

/*******************************************************************************
Name        : DENC_Init
Description : Init the STDENC driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/
BOOL DENC_Init(void)
{
    BOOL                            RetOK = TRUE;
    ST_ErrorCode_t                  ErrorCode;
    STDENC_InitParams_t DENCInitParams;
    STDENC_OpenParams_t DENCOpenParams;

    DENCInitParams.MaxOpen = 5;

    DENCInitParams.DeviceType = STDENC_DEVICE_TYPE_DENC;
    DENCInitParams.AccessType = STDENC_ACCESS_TYPE_EMI;
   
    DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p    = (void *)0;
    DENCInitParams.STDENC_Access.EMI.BaseAddress_p          = (void *)ST7100_DENC_BASE_ADDRESS;
    DENCInitParams.STDENC_Access.EMI.Width                  = STDENC_EMI_ACCESS_WIDTH_32_BITS;

    ErrorCode = STDENC_Init(STDENC_DEVICE_NAME, &DENCInitParams);

    if (ErrorCode != ST_NO_ERROR)
    {
        printf("DENC Init error\n");
        RetOK = FALSE;
    }
    
    ErrorCode = STDENC_Open(STDENC_DEVICE_NAME, &DENCOpenParams, &DENC_Handle);
 
    if (ErrorCode != ST_NO_ERROR)
    {
        printf("DENC Open error\n");
        RetOK = FALSE;
    }
    else
    {
        printf("DENC Init & Open OK\n");
    }
    
    return(RetOK);
}

/*******************************************************************************
Name        : VMIX_Init
Description : Init the STVMIX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/
BOOL VMIX_Init(void)
{
    BOOL                            RetOK = TRUE;
    ST_ErrorCode_t                  ErrorCode;
    STVMIX_InitParams_t             InitParams;
    STVMIX_OpenParams_t             OpenParams;
    ST_DeviceName_t                 VoutArray[MAX_VOUT]={{"\0"},
                                                         {"\0"},
                                                         {"\0"},
                                                         {"\0"},
                                                         {"\0"},
                                                         {"\0"}};

    InitParams.DeviceType       = STVMIX_GENERIC_GAMMA_TYPE_7100;
    InitParams.BaseAddress_p    = (void*)ST7100_VMIX1_BASE_ADDRESS;
    strcpy(InitParams.VTGName, STVTG_DEVICE_NAME);
    InitParams.MaxOpen          = 2; /*1;*/
    InitParams.MaxLayer         = 5;

    if (HD == 0)
        strcpy(VoutArray[0], STVOUT_DEVICE_NAME);
    else
    {
        strcpy(VoutArray[0], STVOUT_DEVICE_NAME_HD);
        strcpy(VoutArray[1], STVOUT_DEVICE_NAME);
    }

    InitParams.OutputArray_p    = VoutArray;

    {
        STVOUT_Capability_t  VoutCapa;
        ErrorCode = STVOUT_GetCapability(*VoutArray, &VoutCapa);
        if(ErrorCode != ST_NO_ERROR)
        {
            printf("STVOUT_GetCapability error:%x\n",ErrorCode); 
        }
    }



    InitParams.CPUPartition_p   = DriverPartition_p;
    InitParams.DeviceBaseAddress_p = (void *)0;
    strcpy(InitParams.EvtHandlerName, STEVT_DEVICE_NAME);

    ErrorCode = STVMIX_Init(STVMIX_DEVICE_NAME, &InitParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        printf("VMIX_Init error:%x\n",ErrorCode);
        RetOK = FALSE;
    }
    else
    {
        printf("VMIX_Init OK\n");
        ErrorCode = STVMIX_Open( STVMIX_DEVICE_NAME, &OpenParams, &VMIX_Handle);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("VMIX Open error\n");
            RetOK = FALSE;
        }
        else
        {
            printf("VMIX_Open = %d\n", VMIX_Handle );
        }
    }
    
    return(RetOK);
} /* end of VMIX_Init() */


/*******************************************************************************
Name        : LAYER_Init
Description : Init the STLAYER driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/
BOOL LAYER_Init(void)
{
    BOOL                            RetOK = TRUE;
    ST_ErrorCode_t                  ErrorCode;
    STLAYER_InitParams_t            InitParams;
    STLAYER_OpenParams_t            OpenParams;
    STLAYER_LayerParams_t           LayerParams;

    InitParams.LayerType                = STLAYER_HDDISPO2_VIDEO1;
    InitParams.BaseAddress_p            = (void *)(ST7100_VID1_LAYER_BASE_ADDRESS);
    InitParams.BaseAddress2_p           =  (void *)(ST7100_DISP0_BASE_ADDRESS);
    InitParams.MaxViewPorts             = LAYER_MAX_VIEWPORT;
    InitParams.DeviceBaseAddress_p      = (void *)0;
    InitParams.CPUPartition_p           = SystemPartition_p;
    InitParams.MaxHandles               = LAYER_MAX_OPEN_HANDLES;
    InitParams.AVMEM_Partition          = (STAVMEM_PartitionHandle_t)NULL;
    InitParams.CPUBigEndian             = FALSE;
    InitParams.ViewPortNodeBuffer_p     = NULL;
    InitParams.ViewPortBuffer_p         = NULL;
    InitParams.ViewPortBufferUserAllocated  = FALSE;
    InitParams.NodeBufferUserAllocated      = FALSE;
    InitParams.SharedMemoryBaseAddress_p = (void *)NULL;

    strcpy(InitParams.EventHandlerName, STEVT_DEVICE_NAME);

    if (HD == 0)
    {
        LayerParams.Width = 720;
        LayerParams.Height = 576;
    }
    else
    {
        if ( HD == 1)
        {
            LayerParams.Width = 1920;
            LayerParams.Height = 1080;
        }
        if ( HD == 2)
        {
            LayerParams.Width = 1280;
            LayerParams.Height = 720;
        }
    }
    LayerParams.ScanType = STGXOBJ_INTERLACED_SCAN; /* 1 */
    LayerParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3; /* 0 */
    InitParams.LayerParams_p = &LayerParams;

    ErrorCode = STLAYER_Init(STLAYER_DEVICE_NAME, &InitParams ) ;
    if (ErrorCode != ST_NO_ERROR)
    {
        printf("LAYER_Init error\n");
        RetOK = FALSE;
    }
    else
    {
        printf("LAYER_Init OK\n");
        ErrorCode = STLAYER_Open(STLAYER_DEVICE_NAME, &OpenParams, &LAYER_Handle) ;
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("LAYER Open error\n");
            RetOK = FALSE;
        }
        else
        {
            printf("LAYER_Open = %d\n", LAYER_Handle );
        }
    }
    
    return(RetOK);
}


/*******************************************************************************
Name        : VID_Init
Description : Init the STVID driver
Parameters  : None
Assumptions :
Limitations :
Returns     : RetOK
*******************************************************************************/
BOOL VID_Init(void)
{
    BOOL                            RetOK = TRUE;
    ST_ErrorCode_t                  ErrorCode;
    STVID_InitParams_t              InitParams;
    STVID_OpenParams_t              OpenParams;
    
    memset(&InitParams, 0, sizeof(InitParams)); /* set all params to null */

    InitParams.DeviceBaseAddress_p = (void*) 0;
    InitParams.SharedMemoryBaseAddress_p = (void*) SDRAM_BASE_ADDRESS;
    InitParams.InstallVideoInterruptHandler = TRUE;
    InitParams.InterruptLevel = VIDEO_INTERRUPT_LEVEL;
    
    /* Default is MPEG */
    InitParams.DeviceType = STVID_DEVICE_TYPE_7100_MPEG;
    InitParams.BaseAddress_p = (void*) VIDEO_BASE_ADDRESS;
    InitParams.BaseAddress2_p = (void *) ST7100_DISP0_BASE_ADDRESS;
    InitParams.BaseAddress3_p =  (void *) 0;
    InitParams.InterruptNumber = VIDEO_INTERRUPT;
    
    if (Codec == STREAM_CODEC_MPEG)
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7100_MPEG;
        InitParams.BaseAddress_p = (void*) VIDEO_BASE_ADDRESS;
        InitParams.BaseAddress2_p = (void *) ST7100_DISP0_BASE_ADDRESS;
        InitParams.BaseAddress3_p =  (void *) 0;
        InitParams.InterruptNumber = VIDEO_INTERRUPT;
        InitParams.InterruptLevel = 4;
    }
    if (Codec == STREAM_CODEC_H264)
    {
        InitParams.DeviceType = STVID_DEVICE_TYPE_7100_H264;
        InitParams.BaseAddress_p = (void*) ST7100_DELTA_BASE_ADDRESS;
        InitParams.InterruptNumber = ST7100_DELTA_PP0_INTERRUPT_NUMBER;
        InitParams.BaseAddress3_p =  (void *) 0;
        InitParams.BaseAddress2_p = (void *) ST7100_DISP0_BASE_ADDRESS;
    }

    strcpy(InitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    strcpy(InitParams.ClockRecoveryName, STCLKRV_DEVICE_NAME);

    InitParams.MaxOpen = 2; /* 1  */
    InitParams.BitBufferAllocated = FALSE;
    InitParams.BitBufferSize = 0;
    InitParams.BitBufferAddress_p = 0x11;/* added by Barry */

    InitParams.UserDataSize = 75; /* limited by FQ for overflow test in m_event12 */
    InitParams.AVMEMPartition = (STAVMEM_PartitionHandle_t) 1; /* Partition 1: Only the index is passed, conversion done in kstvid */
    InitParams.CPUPartition_p = DriverPartition_p;
    InitParams.AVSYNCDriftThreshold = 2 * (90000 / 50); /* 2 * VSync max time */
    
    ErrorCode = STVID_Init(STVID_DEVICE_NAME, &InitParams);

    if (ErrorCode != ST_NO_ERROR)
    {
        printf("VID_Init error\n");
        RetOK = FALSE;
    }
    else
    {
        printf("VID_Init OK\n");
        
        OpenParams.SyncDelay = 0;
        ErrorCode = STVID_Open(STVID_DEVICE_NAME, &OpenParams, &VID_Handle) ;
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("VID Open error\n");
            RetOK = FALSE;
        }
        else
        {
            printf("VID_Open = %d\n", VID_Handle );
        }
    }
    
    return(RetOK);
    
}




