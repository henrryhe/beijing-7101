/*****************************************************************************
File Name   : stmergefn.c

Description :

History     :


Copyright (C) 2005 STMicroelectronics

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "inject_swts.h"
#include "stcommon.h"

#if defined(ST_OSLINUX)
#include <sched.h>
BOOL InjectionStatus = TRUE;
void* Inject_Stream(void* Data);
void* Inject_Status(void* Data);
BOOL STLINUXInjectionDone = FALSE;
#endif

#define SWTS_STREAM_RATE  38*1000000

BOOL Term_Injection_Status = FALSE;
BOOL Injection_done = FALSE;

#if defined(STMERGE_USE_TUNER)
extern U32 Pid[3];
#endif


extern int GetPTIStreamId( U32 SrcId );
extern int IsValidSWTSId(U32 SrcId);
extern int IsValidTSINId(U32 SrcId);
extern int IsValidPTIId(U32 DestId);

/* Functions --------------------------------------------------------------- */
/****************************************************************************
Name         : stmerge_ByPass()

Description  : ByPass Mode testing of STMERGE

Parameters   :

Return Value : BOOL

See Also     : Test plan
****************************************************************************/

BOOL stmerge_ByPass(parse_t * pars_p, char *result_sym_p)
{
    STMERGE_InitParams_t                STMERGE_InitParams;
    ST_ErrorCode_t                      Error = ST_NO_ERROR;
    ST_ErrorCode_t                      StoredError = ST_NO_ERROR;
    ST_DeviceName_t                     STMERGE_Name;
#if defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
    U32                                 SimpleRAMMap[4][2] = { {STMERGE_TSIN_0,256},
                                                               {STMERGE_TSIN_1,256},
                                                               {STMERGE_SWTS_0,384},
                                                               {0,0}};
#else
    U32                                 SimpleRAMMap[3][2] = { {STMERGE_TSIN_0,256},
                                                               {STMERGE_SWTS_0,384},
                                                               {0,0}};
#endif
    STPTI_InitParams_t                  STPTI_InitParams;
    STPTI_OpenParams_t                  STPTI_OpenParams;
    U16                                 PacketCountAtPTI = 0,PacketCountAtPTI_Post = 0;
    U16                                 slotcount =0;
    STPTI_TermParams_t                  STPTI_TermParams;
    STPTI_Handle_t                      STPTI_Handle;

    STPTI_Slot_t                        STPTI_Slot[MAX_SESSIONS_PER_CELL];

#if defined(POLL_PTI_ERROR_EVENTS)
#ifdef STMERGE_DTV_PACKET
    STPTI_SlotData_t                    STPTI_SlotData = { STPTI_SLOT_TYPE_VIDEO_ES,
#else
    STPTI_SlotData_t                    STPTI_SlotData = { STPTI_SLOT_TYPE_PES,
#endif
#else
    STPTI_SlotData_t                    STPTI_SlotData = { STPTI_SLOT_TYPE_RAW,
#endif  /* POLL_PTI_ERROR_EVENTS */
                                                           {FALSE, FALSE, FALSE,
                                                            FALSE, FALSE, FALSE,
                                                            FALSE, FALSE}
                                                         };

    static char                         *DeviceName[2] = { "stpti_A", "stpti_B"};
#if defined(POLL_PTI_ERROR_EVENTS)
    STPTI_DMAParams_t                   STPTI_DMAParams;
#endif

#if !defined(ST_OSLINUX)
#ifndef ST_OS21
    semaphore_t                     InjectTerm;
#endif
    STMERGE_TaskParams_t            Params;
    semaphore_t                     *InjectTerm_p;
    task_t*                         Task_p ;
#endif

    S32 LVar;
    U32 SrcId,DestId;
    BOOL RetErr = FALSE;
    int Injection = 0 , i =0;

#if defined(ST_OSLINUX)
#if defined(SWTS_PID)
#undef SWTS_PID
#endif

/* This PID is for Canal10.trp */
#define SWTS_PID 0x3C
#endif

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);
    STTBX_Print(("\n============== ByPass Mode test       ======================\n\n"));
    STTBX_Print(("-----------------------------------------\n"));


    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
        return FALSE;
    }

    SrcId = LVar;

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected dest id (from 0 to 1)");
        return FALSE;
    }

    DestId = LVar;

#if !defined(STMERGE_USE_TUNER)
    /* EVENT Setup */
    Error = EVENT_Setup( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Configure PTI A to receive stream from TSIN0 with expected PID */
    SetupDefaultInitparams( &STPTI_InitParams );

#if !defined(ST_OSLINUX)
    STPTI_InitParams.TCLoader_p = SupportedLoaders();
#endif

    STPTI_InitParams.TCCodes      = SupportedTCCodes();

    if ( SrcId == STMERGE_SWTS_0 )
    {
        STPTI_InitParams.SyncLock          = 1;
        STPTI_InitParams.SyncDrop          = 3;
    }
    else
    {
        STPTI_InitParams.SyncLock          = 0;
        STPTI_InitParams.SyncDrop          = 0;

    }

    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_NOTAGS;
    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(DestId);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(DestId);

    if ( DestId == STMERGE_PTI_0 )
    {
        i = 0;
    }
    else
    {
        i = 1;
    }

    /* Init STPTI */
    Error = STPTI_Init(DeviceName[i], &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    Error = STPTI_Open( DeviceName[i],  &STPTI_OpenParams, &STPTI_Handle );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle, &STPTI_Slot[i], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);


    if ( SrcId == STMERGE_SWTS_0 )
    {
        /* Slot set PID for SWTS */
        Error = STPTI_SlotSetPid( STPTI_Slot[i],SWTS_PID);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
    else
#if defined(STMERGE_USE_TUNER)
    {
        Error = STPTI_SlotSetPid( STPTI_Slot[i],Pid[1]);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
#else
    {
        Error = STPTI_SlotSetPid( STPTI_Slot[i],TSIN_PID);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
#endif

#if defined(POLL_PTI_ERROR_EVENTS)

    /* Link audio and video slots to the CD-FIFO */
#if defined(ST_5528)
    STPTI_DMAParams.Destination = VIDEO_CD_FIFO;
#else
    STPTI_DMAParams.Destination = AUDIO_CD_FIFO;
#endif

    STPTI_DMAParams.Holdoff     = 4;
    STPTI_DMAParams.WriteLength = 4;
#if defined(ST_5528)
    STPTI_DMAParams.CDReqLine   = STPTI_CDREQ_VIDEO;
#else
    STPTI_DMAParams.CDReqLine   = STPTI_CDREQ_AUDIO;
#endif

    Error = STPTI_SlotLinkToCdFifo(STPTI_Slot[i], &STPTI_DMAParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Enable receipt of STPTI_EVENT_CC_ERROR_EVT event */
    Error = STPTI_EnableErrorEvent( DeviceName[i], STPTI_EVENT_CC_ERROR_EVT);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Enable receipt of STPTI_EVENT_PACKET_ERROR_EVT event */
    Error = STPTI_EnableErrorEvent( DeviceName[i], STPTI_EVENT_PACKET_ERROR_EVT);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#endif

#if defined(STMERGE_DTV_PACKET)
    if ( SrcId == STMERGE_SWTS_0 )
    {
         STPTI_SetDiscardParams(DeviceName[i],2);
#if 0
         /* This function can be required in the future. Right now the tests are passing without it, but
            the STPTI4 test harness is using this function. */
         TSMERGER_IIFSetSyncPeriod(132);
#endif
    }
#endif
    /* Init MERGE params Setting */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = &SimpleRAMMap;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;

    /* From TSIN0 to both PTI */
    STMERGE_InitParams.Mode = STMERGE_BYPASS_GENERIC;

    /* Init TSMERGE in BYPASS Mode */
    strcpy(STMERGE_Name,"MERGE0");

    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    Error = STMERGE_ConnectGeneric(SrcId, DestId , STMERGE_BYPASS_GENERIC);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    if ( SrcId == STMERGE_SWTS_0 )
    {
#if !defined(ST_OSLINUX)
        /* Initialise InjectTerm semaphore */
#ifdef ST_OS21
        InjectTerm_p = semaphore_create_fifo(0);
#else
        InjectTerm_p = &InjectTerm;
        semaphore_init_fifo(InjectTerm_p, 0);
#endif
        Params.NoInterfacesSet=1;
        strcpy(Params.DeviceName[0],DeviceName[0]);
        strcpy(Params.InputInterface[0],"SWTS0");
        Params.SlotHandle[0] = STPTI_Slot[i];
        Params.InjectTerm_p  = InjectTerm_p;

        Term_Injection_Status = FALSE;

        /* create a task for printing the stream injection status */
        Task_p = task_create ((void (*)(void *)) Injection_Status, &Params, TASK_SIZE, 2, "Injection_Status", 0 );
        if ( Task_p == NULL )
        {
            STTBX_Print (("\n ------ Task Creation Failed ------ \n")) ;
            stmergetst_SetSuccessState(FALSE);
        }

        STTBX_Print(("\n================ FILE PLAY RUNNING ==================\n")) ;


        /* Play the Stream through SWTS 0 */
        InjectStream(STMERGE_SWTS_0);

        Term_Injection_Status = TRUE;

        /* semaphore_wait for synchronisation */
        semaphore_wait(InjectTerm_p);

        STTBX_Print(("\n"));
        STTBX_Print(("\n================ FILE PLAY EXITTING ==================\n")) ;

        /* semaphore_delete to free heap */
        semaphore_delete(InjectTerm_p);

        /* delete the task */
        task_delete ( Task_p ) ;

        /* reset flag */
        Term_Injection_Status = FALSE;
#else
#define PRIORITY_MIN 0
        pthread_attr_t attr;
        pthread_t task_inject_stream,task_inject_status;
        struct sched_param sparam;
        int priority=PRIORITY_MIN;
        int rc;
        U16 slotcount;
        char p[16];
        struct Inject_StatusParams InjectParams; 
  
        STTBX_Print(("\n================ FILE PLAY RUNNING ==================\n")) ;
        STLINUXInjectionDone = FALSE;

        pthread_attr_init(&attr);
 
        if (priority < sched_get_priority_min(SCHED_RR))
            priority = sched_get_priority_min(SCHED_RR);
       
        if (priority > sched_get_priority_max(SCHED_RR))
            priority = sched_get_priority_max(SCHED_RR);
       
        sparam.sched_priority = 99;
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setschedparam(&attr, &sparam);

        strcpy(p,DeviceName[0]);
        printf(" Creating thread 1 \n");
        rc = pthread_create(&task_inject_stream,&attr,Inject_Stream,(void*)p);
        
        sparam.sched_priority = 0;
        pthread_attr_setschedparam(&attr, &sparam);

        InjectParams.NoInterfacesSet=1;
        strcpy(InjectParams.DeviceName[0],DeviceName[0]);
        strcpy(InjectParams.InputInterface[0],"SWTS0");
        InjectParams.SlotHandle[0] = STPTI_Slot[0];
        printf(" Creating thread 2 \n");
        rc = pthread_create(&task_inject_status,&attr,Inject_Status,(void*)&InjectParams);

        pthread_join(task_inject_stream,NULL);
        STTBX_Print(("\n"));
        STTBX_Print(("\n================ FILE PLAY EXITTING ==================\n")) ;

        /* Get Packet count At PTI A */
        Error = STPTI_GetInputPacketCount (DeviceName[0] , &PacketCountAtPTI);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTIA         = %d",PacketCountAtPTI));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count after connection  = %d",slotcount));
#endif
        Injection = MAX_INJECTION_TIME;

    }

    while (Injection < MAX_INJECTION_TIME)
    {
#if defined(ST_7100)
        task_delay(ST_GetClocksPerSecond());
#else
        task_delay(ST_GetClocksPerSecond()/10);
#endif

        STTBX_Print(("\n**************************************"));

        /* Get Packet count at PTI A */
        Error = STPTI_GetInputPacketCount (DeviceName[i] , &PacketCountAtPTI);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI         = %d",PacketCountAtPTI));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[i], &slotcount);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot   count at PTI         = %d",slotcount));

        /* Packet count at PTIA should increase after STMERGE_Init */
        if (PacketCountAtPTI == 0)
        {
            STTBX_Print(("\nConnection failed"));
            stmergetst_SetSuccessState(FALSE);
        }
        Injection++;
    }

    STTBX_Print(("\n"));

    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Get Packet count At PTI A after Term */
    Error = STPTI_GetInputPacketCount (DeviceName[i] , &PacketCountAtPTI);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);


    task_delay(ST_GetClocksPerSecond()/100);

    /* Get Packet count At PTI A after Term */
    Error = STPTI_GetInputPacketCount (DeviceName[i] , &PacketCountAtPTI_Post);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* After termination the packet should not increase */
    if (PacketCountAtPTI != PacketCountAtPTI_Post)
    {
        STTBX_Print(("\nDisconnection failed"));
        stmergetst_SetSuccessState(FALSE);
    }

    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term(DeviceName[i],&STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    stmergetst_TestConclusion("\nBYPASS TSIN/SWTS to PTI ");


#ifndef STMERGE_USE_TUNER
    Error = EVENT_Term(EVENT_HANDLER_NAME);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    STTBX_Print(("============== ByPass Exit       ======================\n"));

    return TRUE;

}/* stmerge_ByPass */

#if defined(STMERGE_1394_PRODUCER_CONSUMER)

#ifndef ST_OS21
    semaphore_t   Sync;
#endif
    semaphore_t   *Sync_p;

/****************************************************************************
Name         : stmerge_1394_Producer()

Description  : 1394 Producer task

****************************************************************************/

void stmerge_1394_Producer(void)
{

    STMERGE_ObjectConfig_t          TestTSIN_SetParams,Test1394_SetParams;
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;

    STTBX_Print(("\n==============Created 1394 Producer task======================\n"));
    STTBX_Print(("Stream is routed from TSIN0 then to 1394_out \n"));

    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Test data for TSIN0 */
    TestTSIN_SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    TestTSIN_SetParams.SyncLock  = 0;
    TestTSIN_SetParams.SyncDrop  = 0;
    TestTSIN_SetParams.SOPSymbol = 0x47;

    TestTSIN_SetParams.u.TSIN.SerialNotParallel  = TRUE; /* FALSE in Parallel Mode */
    TestTSIN_SetParams.u.TSIN.InvertByteClk      = FALSE;
    TestTSIN_SetParams.u.TSIN.ByteAlignSOPSymbol = TRUE; /* FALSE for Parallel Mode */

#if defined(STMERGE_DTV_PACKET)
    TestTSIN_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DTV;
    TestTSIN_SetParams.u.TSIN.SyncNotAsync       = FALSE; /* Async mode */
    TestTSIN_SetParams.u.TSIN.ReplaceSOPSymbol   = TRUE;  /* ReplaceSOP=1 for DTV */
#else
    TestTSIN_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
    TestTSIN_SetParams.u.TSIN.SyncNotAsync       = TRUE;
    TestTSIN_SetParams.u.TSIN.ReplaceSOPSymbol   = FALSE;
#endif

    /* Set Parameters for SWTS0 */
    Error = STMERGE_SetParams(STMERGE_TSIN_0,&TestTSIN_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Test data for 1394 */
    Test1394_SetParams.Priority  = STMERGE_PRIORITY_MID;
    Test1394_SetParams.SyncLock  = 0;
    Test1394_SetParams.SyncDrop  = 0;
    Test1394_SetParams.SOPSymbol = 0x47;
    Test1394_SetParams.PacketLength = STMERGE_PACKET_LENGTH_DVB;

    Test1394_SetParams.u.P1394.P1394ClkSrc    = FALSE; /* I/p clk src */
    Test1394_SetParams.u.P1394.Pace           = 0x0004;
    Test1394_SetParams.u.P1394.Tagging        = TRUE;

    /* Set Parameters for 1394 */
    Error = STMERGE_SetParams(STMERGE_1394OUT_0,&Test1394_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Connect TSIN0 and 1394 */
    Error = STMERGE_Connect(STMERGE_TSIN_0,STMERGE_1394OUT_0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    semaphore_signal(Sync_p);

}

/****************************************************************************
Name         : stmerge_1394_Consumer()

Description  : Consumes 1394 output data

****************************************************************************/

void stmerge_1394_Consumer(void)
{

    STMERGE_ObjectConfig_t          TestTSIN_SetParams;
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle;
    U16                             packetcountpre  = 0,packetcountpost = 0;
    U16                             slotcount       = 0;
    STPTI_TermParams_t              STPTI_TermParams;
    STPTI_Slot_t                    STPTI_Slot[1];

#if defined(POLL_PTI_ERROR_EVENTS)
#ifdef STMERGE_DTV_PACKET
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_VIDEO_ES,
#else
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_PES,
#endif
#else
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_RAW,
#endif  /* POLL_PTI_ERROR_EVENTS */
                                                       {FALSE, FALSE, FALSE,
                                                        FALSE, FALSE, FALSE,
                                                        FALSE, FALSE}
                                                     };

#if defined(POLL_PTI_ERROR_EVENTS)
    STPTI_DMAParams_t               STPTI_DMAParams;
#endif
    int                             Injection = 0;

    STTBX_Print(("\n==============Created 1394 Consumer task======================\n"));
    STTBX_Print(("A Stream is injected at TSIN1 & routed to PTI0\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    /* Test data for TSIN1 */
    TestTSIN_SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    TestTSIN_SetParams.SyncLock  = 0;
    TestTSIN_SetParams.SyncDrop  = 0;
    TestTSIN_SetParams.SOPSymbol = 0x47;

#ifdef ST_5528
    TestTSIN_SetParams.u.TSIN.SerialNotParallel  = TRUE;
    TestTSIN_SetParams.u.TSIN.ByteAlignSOPSymbol = TRUE;
#else /* 5100 ,7710 */
    TestTSIN_SetParams.u.TSIN.SerialNotParallel  = FALSE; /* FALSE in Parallel Mode */
    TestTSIN_SetParams.u.TSIN.ByteAlignSOPSymbol = FALSE; /* FALSE for Parallel Mode */
#endif

    TestTSIN_SetParams.u.TSIN.InvertByteClk      = FALSE;

#if defined(STMERGE_DTV_PACKET)
    TestTSIN_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DTV;
    TestTSIN_SetParams.u.TSIN.SyncNotAsync       = FALSE; /* Async mode */
    TestTSIN_SetParams.u.TSIN.ReplaceSOPSymbol   = TRUE;  /* ReplaceSOP=1 for DTV */
#else
    TestTSIN_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
    TestTSIN_SetParams.u.TSIN.SyncNotAsync       = TRUE;
    TestTSIN_SetParams.u.TSIN.ReplaceSOPSymbol   = FALSE;
#endif

    /* Set Parameters */
    Error = STMERGE_SetParams(STMERGE_TSIN_1,&TestTSIN_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    SetupDefaultInitparams( &STPTI_InitParams );
    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(STMERGE_PTI_0);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(STMERGE_PTI_0);

#ifdef STMERGE_NO_TAG
    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_NOTAGS;
#else
    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_TSIN1;
#endif

#if !defined(ST_OSLINUX)
    STPTI_InitParams.TCLoader_p = SupportedLoaders();
#endif

#if !defined(STMERGE_USE_TUNER)
    Error = EVENT_Setup( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Init PTI 0 */
    Error = STPTI_Init( "stpti_0", &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    /* Open PTI 0 */
    Error = STPTI_Open( "stpti_0",  &STPTI_OpenParams, &STPTI_Handle );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle, &STPTI_Slot[0], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if defined(STMERGE_USE_TUNER)
    /* Slot set PID for TSIN1 */
    Error = STPTI_SlotSetPid( STPTI_Slot[0],Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    /* Slot set PID for TSIN1 */
    Error = STPTI_SlotSetPid( STPTI_Slot[0],TSIN_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

#if defined(POLL_PTI_ERROR_EVENTS)

    /* Link audio and video slots to the CD-FIFO */
#if defined(ST_5528)
    STPTI_DMAParams.Destination = VIDEO_CD_FIFO;
#else
    STPTI_DMAParams.Destination = AUDIO_CD_FIFO;
#endif

    STPTI_DMAParams.Holdoff     = 4;
    STPTI_DMAParams.WriteLength = 4;
#if defined(ST_5528)
    STPTI_DMAParams.CDReqLine   = STPTI_CDREQ_VIDEO;
#else
    STPTI_DMAParams.CDReqLine   = STPTI_CDREQ_AUDIO;
#endif

    Error = STPTI_SlotLinkToCdFifo(STPTI_Slot[0], &STPTI_DMAParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Enable receipt of STPTI_EVENT_CC_ERROR_EVT event */
    Error = STPTI_EnableErrorEvent( "stpti_0", STPTI_EVENT_CC_ERROR_EVT);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Enable receipt of STPTI_EVENT_PACKET_ERROR_EVT event */
    Error = STPTI_EnableErrorEvent( "stpti_0", STPTI_EVENT_PACKET_ERROR_EVT);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#endif

    /* Connect TSIN0 and PTI0 */
    Error = STMERGE_Connect(STMERGE_TSIN_1,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    semaphore_wait(Sync_p);

    while (Injection < MAX_INJECTION_TIME)
    {

        task_delay(ST_GetClocksPerSecond()/100);

        /* Again Get packet count */
        Error = STPTI_GetInputPacketCount ( "stpti_0", &packetcountpost);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\n**************************************"));
        STTBX_Print(("\nPacket count after connection = %d",packetcountpost));
        if (packetcountpre == packetcountpost)
        {
            stmergetst_SetSuccessState(FALSE);
        }

        Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\nSlot count after connection   = %d",slotcount));

        if (slotcount == 0)
        {
            STTBX_Print(("\nSlot count is not increasing"));
            stmergetst_SetSuccessState(FALSE);
        }

       Injection ++;

    }
    STTBX_Print(("\n"));

#ifndef STMERGE_USE_TUNER
    /* Term Event Handler */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    semaphore_delete(Sync_p);

    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term ("stpti_0", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

}/* stmerge_1394_Consumer */

/****************************************************************************
Name         : stmerge_1394_Producer_Consumer()

Description  : Testing STMERGE 1394 In/Out

Parameters   :

Return Value : BOOL

See Also     : Test plan
****************************************************************************/

BOOL stmerge_1394_Producer_Consumer(parse_t * pars_p, char *result_sym_p)
{

    STMERGE_InitParams_t            STMERGE_InitParams;
    U32                             SimpleRAMMap[3][2] = { {STMERGE_TSIN_0,1024},
                                                           {STMERGE_TSIN_1,1024},
                                                           {0,0}
                                                         };
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    task_t *Prod_p, *Con_p;

    STTBX_Print(("\n==============Testing 1394 routing ========================\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    /* Init params Setting */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = &SimpleRAMMap;
    STMERGE_InitParams.Mode              = STMERGE_NORMAL_OPERATION;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;

    /* Init merge */
    strcpy(STMERGE_Name,"MERGE0");
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#ifdef ST_OS21
    Sync_p = semaphore_create_fifo(0);
#else
    Sync_p = &Sync;
    semaphore_init_fifo(Sync_p, 0);
#endif

    /* create a task for 1394_Consumer */
    Con_p = task_create ((void (*)(void *)) stmerge_1394_Consumer, NULL, TASK_SIZE, 10, "Consumer_Task", 0 );
    if (Con_p == NULL)
    {
        STTBX_Print(("Consumer task FAILED"));
        return TRUE;
    }

    /* create a task for 1394_Producer */
    Prod_p = task_create ((void (*)(void *)) stmerge_1394_Producer, NULL, TASK_SIZE, 10, "Producer_Task", 0 );
    if (Prod_p == NULL)
    {
        STTBX_Print(("Producer task FAILED"));
        return TRUE;
    }

    /* Wait for each task before deleting it */
    if (task_wait(&Prod_p,1,TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        task_delete(Prod_p);
    }

    if (task_wait(&Con_p,1,TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the tasks */
        task_delete(Con_p);
    }

    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);
    STTBX_Print(("\n**********************************************************************"));
    STTBX_Print(("\n============== 1394 Producer Consumer test exit ======================\n"));
    return TRUE;

}/* stmerge_1394_Producer_Consumer */

#endif

/****************************************************************************
Name         : stmerge_AltoutTest()

Description  : Testing STMERGE Altout

Parameters   :

Return Value : BOOL

See Also     : Test plan
****************************************************************************/

BOOL stmerge_AltoutTest(parse_t * pars_p, char *result_sym_p)
{

    STMERGE_InitParams_t            STMERGE_InitParams;
    STMERGE_ObjectConfig_t          TestConfig;
    U32                             SimpleRAMMap[4][2] = { {STMERGE_TSIN_0,768},
                                                           {STMERGE_TSIN_1,768},
                                                           {STMERGE_ALTOUT_0,768},
                                                           {0,0}
                                                         };
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle[2];
    U16                             packetcountpost0= 0,packetcountpost1= 0;
    U16                             slotcount0 = 0,slotcount1 = 0;
    STPTI_TermParams_t              STPTI_TermParams;
    STPTI_Slot_t                    STPTI_Slot[2];
    U32                             Injection=0;

    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_RAW,
                                                       {FALSE, FALSE, FALSE,
                                                        FALSE, FALSE, FALSE,
                                                        FALSE, FALSE}
                                                     };

    STTBX_Print(("\n==============Testing Altout ========================\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    /* STMERGE Init params Setting */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = &SimpleRAMMap;
    STMERGE_InitParams.Mode              = STMERGE_NORMAL_OPERATION;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;

    /* Init merge */
    strcpy(STMERGE_Name,"MERGE0");
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Set up PTI0 - 1st PTI instance */
    SetupDefaultInitparams( &STPTI_InitParams );
    STPTI_InitParams.TCDeviceAddress_p        = GetBaseAddress(STMERGE_PTI_0);
    STPTI_InitParams.InterruptNumber          = GetInterruptNumber(STMERGE_PTI_0);
    STPTI_InitParams.StreamID                 = STPTI_STREAM_ID_TSIN0;
    STPTI_InitParams.AlternateOutputLatency   = 0;

#if !defined(ST_OSLINUX)
    STPTI_InitParams.TCLoader_p = SupportedLoaders();
#endif

    STPTI_InitParams.SyncLock          = 0;
    STPTI_InitParams.SyncDrop          = 0;

#if !defined(STMERGE_USE_TUNER)
    Error = EVENT_Setup( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Init PTI 0 */
    Error = STPTI_Init( "stpti_0", &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);


    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    /* Open PTI 0 */
    Error = STPTI_Open( "stpti_0",  &STPTI_OpenParams, &STPTI_Handle[0] );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle[0], &STPTI_Slot[0], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot set PID for TSIN0 */
#ifdef STMERGE_USE_TUNER
    Error = STPTI_SlotSetPid( STPTI_Slot[0],Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    Error = STPTI_SlotSetPid( STPTI_Slot[0],TSIN_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    Error = STPTI_AlternateOutputSetDefaultAction( STPTI_Handle[0], STPTI_ALTERNATE_OUTPUT_TYPE_OUTPUT_AS_IS, 0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);


    STPTI_InitParams.SyncLock          = 0;
    STPTI_InitParams.SyncDrop          = 0;
    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_TSIN1;

    /* Init PTI0' */
    Error = STPTI_Init( "stpti_1", &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Open PTI0' */
    Error = STPTI_Open( "stpti_1",  &STPTI_OpenParams, &STPTI_Handle[1] );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle[1], &STPTI_Slot[1], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#ifdef STMERGE_USE_TUNER
    Error = STPTI_SlotSetPid( STPTI_Slot[1],Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    Error = STPTI_SlotSetPid( STPTI_Slot[1],TSIN_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* TSMERGER Configurations */
    /* Config TSIN0 */
    TestConfig.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
    TestConfig.SyncLock                  = 0;
    TestConfig.SyncDrop                  = 0;
    TestConfig.Priority                  = STMERGE_PRIORITY_HIGH;
    TestConfig.SOPSymbol                 = 0x47;

    TestConfig.u.TSIN.SyncNotAsync       = TRUE;
    TestConfig.u.TSIN.SerialNotParallel  = TRUE;
    TestConfig.u.TSIN.ByteAlignSOPSymbol = TRUE;
    TestConfig.u.TSIN.InvertByteClk      = FALSE;
    TestConfig.u.TSIN.ReplaceSOPSymbol   = FALSE;

    /* Set Parameters for TSIN0 */
    Error = STMERGE_SetParams(STMERGE_TSIN_0,&TestConfig);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Config for TSIN1 */
#if defined(ST_5100) || defined(ST_7710) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109)
    TestConfig.u.TSIN.SerialNotParallel  = FALSE;
    TestConfig.u.TSIN.ByteAlignSOPSymbol = FALSE;
#endif

    /* Set Parameters for TSIN1 */
    Error = STMERGE_SetParams(STMERGE_TSIN_1,&TestConfig);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Config for ALTOUT */
    TestConfig.u.ALTOUT.PTISource     = STMERGE_PTI_0;
    TestConfig.u.ALTOUT.Pace          = STMERGE_PACE_AUTO;
    TestConfig.u.ALTOUT.Counter.Value = STMERGE_COUNTER_AUTO_LOAD;
    TestConfig.u.ALTOUT.Counter.Rate  = STMERGE_COUNTER_NO_INC;

    /* Set Parameters for ALTOUT */
    Error = STMERGE_SetParams(STMERGE_ALTOUT_0,&TestConfig);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    TestConfig.u.P1394.P1394ClkSrc    = FALSE; /* I/p clk src */
    TestConfig.u.P1394.Pace           = 0x004;
    TestConfig.u.P1394.Tagging        = TRUE;

    /* Set Parameters for 1394 dest*/
    Error = STMERGE_SetParams(STMERGE_1394OUT_0,&TestConfig);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Setup all the connections */
    Error = STMERGE_Connect(STMERGE_TSIN_0,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    Error = STMERGE_Connect(STMERGE_ALTOUT_0,STMERGE_1394OUT_0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    Error = STMERGE_Connect(STMERGE_TSIN_1,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);


    while(Injection < MAX_INJECTION_TIME)
    {

        STTBX_Print(("\n*********************************************"));
        /* Get Packet count at PTI0 from TSIN0 */
        Error = STPTI_GetInputPacketCount ("stpti_0", &packetcountpost0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from TSIN0      = %d",packetcountpost0));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from TSIN0        = %d",slotcount0));

        STTBX_Print(("\n"));

        /* Get Packet count at PTI0 from TSIN1 */
        Error = STPTI_GetInputPacketCount ("stpti_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from TSIN1      = %d",packetcountpost1));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from TSIN1        = %d",slotcount1));

        Injection++;
        task_delay(ST_GetClocksPerSecond()/100);
    }

    STTBX_Print(("\n*********************************************\n"));

    if ((packetcountpost0 > 0) && (packetcountpost1 > 0) && (slotcount0 > 0) && (slotcount1 > 0))
    {
        STTBX_Print(("\nPackets are received both from TSIN0 & TSIN1 to PTI0' & PTI0''\n"));
        stmergetst_SetSuccessState(TRUE);
    }
    else
    {
        stmergetst_SetSuccessState(FALSE);
    }


    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term ("stpti_0", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    Error = STPTI_Term ("stpti_1", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);
    STTBX_Print(("\n**********************************************************************"));
    STTBX_Print(("\n============== ALTOUT test exit ======================\n"));
    return TRUE;

}/* stmerge_AltoutTest */

/****************************************************************************
Name         : stmerge_DisconnectTest()

Description  : Executes Disconnect test

Parameters   :

Return Value : 

See Also     : Test plan
****************************************************************************/

BOOL stmerge_DisconnectTest(parse_t * pars_p, char *result_sym_p)
{
    STMERGE_InitParams_t            STMERGE_InitParams;
#if defined(ST_7109)
    U32                             SimpleRAMMap[7][2] = {
                                                              {STMERGE_TSIN_0,384},
                                                              {STMERGE_TSIN_1,384},
                                                              {STMERGE_TSIN_2,384},
                                                              {STMERGE_SWTS_0,384},
                                                              {STMERGE_SWTS_1,384},
                                                              {STMERGE_SWTS_2,384},
                                                              {0,0}
                                                             };
#elif defined(ST_5525)
    U32                             SimpleRAMMap[8][2] = {
                                                              {STMERGE_TSIN_0,384},
                                                              {STMERGE_TSIN_1,384},
                                                              {STMERGE_TSIN_2,384},
                                                              {STMERGE_SWTS_0,384},
                                                              {STMERGE_SWTS_1,384},
                                                              {STMERGE_SWTS_2,384},
                                                              {STMERGE_SWTS_3,384},
                                                              {0,0}
                                                             };
#else
    U32                             SimpleRAMMap[5][2] = {
                                                              {STMERGE_TSIN_0,384},
                                                              {STMERGE_TSIN_1,384},
                                                              {STMERGE_TSIN_2,384},
                                                              {STMERGE_SWTS_0,384},
                                                              {0,0}
                                                             };
#endif
#if defined(STMERGE_DEFAULT_PARAMETERS)
    STMERGE_ObjectConfig_t          DefaultConfig_TSIN,DefaultConfig_SWTS;
    STMERGE_ObjectConfig_t          TestData_TSIN,TestData_SWTS;
#endif

    STMERGE_ObjectConfig_t          TestStream_SetParams={0};
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle;
    U16                             packetcountpre  = 0,packetcountpost0 = 0,packetcountpost1 = 0;
    U16                             slotcount0       = 0,slotcount1 = 0;
    U16                             packetcountpost2       = 0,slotcount2 = 0;
    STPTI_TermParams_t              STPTI_TermParams;
    STPTI_Slot_t                    STPTI_Slot[2];
    static char                     *DeviceName[2] = { "stpti_0", "stpti_1"};

#if defined(POLL_PTI_ERROR_EVENTS)
#ifdef STMERGE_DTV_PACKET
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_VIDEO_ES,
#else
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_PES,
#endif
#else
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_RAW,
#endif  /* POLL_PTI_ERROR_EVENTS */
                                                       {FALSE, FALSE, FALSE,
                                                        FALSE, FALSE, FALSE,
                                                        FALSE, FALSE}
                                                     };

#if defined(POLL_PTI_ERROR_EVENTS)
    STPTI_DMAParams_t               STPTI_DMAParams;
#endif
    
    int            Injection = 0,i =0;
    U32 SrcId,SrcId1,DestId;

#if defined(ST_OSLINUX)
#if defined(SWTS_PID)
#undef SWTS_PID
#endif

/* This PID is for Canal10.trp */
#define SWTS_PID 0x3C
#endif

#if !defined(ST_OSLINUX)
#ifndef ST_OS21
    semaphore_t                     InjectTerm;
#endif
        semaphore_t                 *InjectTerm_p;
        task_t*                         Task_p ;
        STMERGE_TaskParams_t            Params;
#endif

    STTBX_Print(("\n============== Disconnect test ======================\n"));
    STTBX_Print(("Streams are input at TSIN0 and SWTS0 for this test to execute\n"));
    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    SrcId = STMERGE_TSIN_0;
    SrcId1 = STMERGE_SWTS_0;
    DestId = STMERGE_PTI_0;

    /* Init params Setting */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = &SimpleRAMMap;
    STMERGE_InitParams.Mode              = STMERGE_NORMAL_OPERATION;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;

#if defined(STMERGE_INTERRUPT_SUPPORT)
    STMERGE_InitParams.InterruptNumber   = INTERRUPT_NUM;
    STMERGE_InitParams.InterruptLevel    = INTERRUPT_LEVEL;
#endif

    /* Init merge */
    strcpy(STMERGE_Name,"MERGE0");
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Test data for TSIN0 */
    TestStream_SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    TestStream_SetParams.SOPSymbol = 0x47;

#if defined(STMERGE_DTV_PACKET)
        /* this now assumes two sync bytes 0x47 0xff + (DTV TS Packet)*/
        TestStream_SetParams.PacketLength = STMERGE_PACKET_LENGTH_DTV_WITH_2B_STUFFING;
#else
        TestStream_SetParams.PacketLength = STMERGE_PACKET_LENGTH_DVB;
#endif

        TestStream_SetParams.u.SWTS.Tagging       = STMERGE_TAG_ADD;
#ifdef SWTS_MANUAL_PACE
        TestStream_SetParams.u.SWTS.Pace          = (U32)(ST_GetClockSpeed()/( SWTS_STREAM_RATE / 2 ));
#else
        TestStream_SetParams.u.SWTS.Pace          = STMERGE_PACE_AUTO;
#endif
        TestStream_SetParams.u.SWTS.Counter.Value = STMERGE_COUNTER_AUTO_LOAD;
        TestStream_SetParams.u.SWTS.Counter.Rate  = STMERGE_COUNTER_NO_INC;

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_5525)
       TestStream_SetParams.u.SWTS.Trigger  = 10;
#endif

        TestStream_SetParams.SyncLock  = 1;
        TestStream_SetParams.SyncDrop  = 3;

		    /* Set Parameters */
    Error = STMERGE_SetParams(SrcId1,&TestStream_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        TestStream_SetParams.u.TSIN.SerialNotParallel  = TRUE; /* FALSE in Parallel Mode */
        TestStream_SetParams.u.TSIN.InvertByteClk      = FALSE;
        TestStream_SetParams.u.TSIN.ByteAlignSOPSymbol = TRUE; /* FALSE for Parallel Mode */

#if defined(STMERGE_DTV_PACKET)
        TestStream_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DTV;
        TestStream_SetParams.u.TSIN.SyncNotAsync       = FALSE; /* Async mode */
        TestStream_SetParams.u.TSIN.ReplaceSOPSymbol   = TRUE;  /* ReplaceSOP=1 for DTV */
#else
        TestStream_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
        TestStream_SetParams.u.TSIN.SyncNotAsync       = TRUE;
        TestStream_SetParams.u.TSIN.ReplaceSOPSymbol   = FALSE;
#endif

        TestStream_SetParams.SyncLock  = 0;
        TestStream_SetParams.SyncDrop  = 0;

	    /* Set Parameters */
    Error = STMERGE_SetParams(SrcId,&TestStream_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet Count test\n"));
    STTBX_Print(("--------------------------------\n"));

    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;
    
    SetupDefaultInitparams( &STPTI_InitParams );

    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(DestId);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(DestId);
    STPTI_InitParams.TCCodes           = SupportedTCCodes();

#ifdef STMERGE_NO_TAG
    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_NOTAGS;
#else
    STPTI_InitParams.StreamID          = GetPTIStreamId(SrcId1);
#endif

#if !defined(ST_OSLINUX)
    STPTI_InitParams.TCLoader_p        = SupportedLoaders();
#endif

#if !defined(STMERGE_USE_TUNER)
    Error = EVENT_Setup( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    if ( DestId == STMERGE_PTI_0 )
    {
        i = 0;
    }
    else
    {
        i = 1;
    }

    Error = STPTI_Init( DeviceName[0], &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    Error = STPTI_Open( DeviceName[0],  &STPTI_OpenParams, &STPTI_Handle );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if defined(STMERGE_DTV_PACKET)
    if ( IsValidSWTSId(SrcId1) )
    {
         STPTI_SetDiscardParams(DeviceName[0], 2);
         TSMERGER_IIFSetSyncPeriod(140);
    }
#endif

    Error = STPTI_SlotAllocate( STPTI_Handle, &STPTI_Slot[0], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);


     Error = STPTI_SlotSetPid( STPTI_Slot[0],SWTS_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);



     STPTI_InitParams.SyncLock    = 0;
     STPTI_InitParams.SyncDrop    = 0;
#ifdef STMERGE_NO_TAG
    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_NOTAGS;
#else
    STPTI_InitParams.StreamID          = GetPTIStreamId(SrcId);
#endif

    Error = STPTI_Init( DeviceName[1], &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    Error = STPTI_Open( DeviceName[1],  &STPTI_OpenParams, &STPTI_Handle );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);


    Error = STPTI_SlotAllocate( STPTI_Handle, &STPTI_Slot[1], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if defined(STMERGE_USE_TUNER)
        Error = STPTI_SlotSetPid( STPTI_Slot[1],Pid[1]);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
        Error = STPTI_SlotSetPid( STPTI_Slot[1],TSIN_PID);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Get packet Count */
    Error = STPTI_GetInputPacketCount ( DeviceName[0], &packetcountpre);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet count before SWTS0 connection are %d \n",packetcountpre));

    /* Packet count should be zero before Connect */
    if (packetcountpre != 0)
    {
        stmergetst_SetSuccessState(FALSE);
    }

    /* Get packet Count */
    Error = STPTI_GetInputPacketCount ( DeviceName[1], &packetcountpre);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet count before TSIN0 connection are %d \n",packetcountpre));

    /* Packet count should be zero before Connect */
    if (packetcountpre != 0)
    {
        stmergetst_SetSuccessState(FALSE);
    }


#if defined(ST_7109) || defined(ST_5525)
    Error = STMERGE_ConnectGeneric(SrcId, DestId , STMERGE_InitParams.Mode);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    Error = STMERGE_Connect(SrcId,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif


#if defined(ST_7109) || defined(ST_5525)
    Error = STMERGE_ConnectGeneric(SrcId1, DestId , STMERGE_InitParams.Mode);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    Error = STMERGE_Connect(SrcId1,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    STTBX_Print(("\nPacket Count Test : TSIN0 connected and SWTS0 connected\n")) ;
    STTBX_Print(("--------------------------------\n"));

    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    
#if !defined(ST_OSLINUX)

/* Initialise InjectTerm semaphore */
#ifdef ST_OS21
        InjectTerm_p = semaphore_create_fifo(0);
#else
        InjectTerm_p = &InjectTerm;
        semaphore_init_fifo(InjectTerm_p, 0);
#endif

        strcpy(Params.DeviceName[0],DeviceName[0]);
        strcpy(Params.InputInterface[0],"SWTS0");
        /* Done this change. changed STPTI_Slot[i] to STPTI_Slot[0]. Need to verify this */
        Params.NoInterfacesSet = 2;
        Params.SlotHandle[0] = STPTI_Slot[0];
        Params.InjectTerm_p  = InjectTerm_p;

        strcpy(Params.DeviceName[1],DeviceName[1]);
        strcpy(Params.InputInterface[1],"TSIN0");
        Params.SlotHandle[1] = STPTI_Slot[1];
        Params.InjectTerm_p  = InjectTerm_p;

        Term_Injection_Status = FALSE;

        /* create a task for printing the stream injection status */
        Task_p = task_create ((void (*)(void *)) Injection_Status, &Params, TASK_SIZE, 2, "Injection_Status", 0 );
        if ( Task_p == NULL )
        {
            STTBX_Print (("\n ------ Task Creation Failed ------ \n")) ;
            stmergetst_SetSuccessState(FALSE);
        }

         /* Play the Stream through SWTS 0 */
        InjectStream(SrcId1);

        Term_Injection_Status = TRUE;

        /* semaphore_wait for synchronisation */
        semaphore_wait(InjectTerm_p);

         /* semaphore_delete to free heap */
        semaphore_delete(InjectTerm_p);

        /* delete the task */
        task_delete ( Task_p ) ;

        /* reset flag */
        Term_Injection_Status = FALSE;

        Injection = MAX_INJECTION_TIME;
        Error = STPTI_GetInputPacketCount ( "stpti_0" , &packetcountpost0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Packet count after SWTS0 connection at PTI0 are %d \n",packetcountpost0));

	 Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Slot count after SWTS0 connection  = %d\n",slotcount0));
	  
        /* Get packet Count */
        Error = STPTI_GetInputPacketCount ( "stpti_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Packet count after TSIN0 connection at PTI0 are %d \n",packetcountpost1));

 	 Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Slot count after TSIN0 connection  = %d\n",slotcount1));
#else
#define PRIORITY_MIN 0
        pthread_attr_t attr;
        pthread_t task_inject_stream,task_inject_status;
        struct sched_param sparam;
        int priority=PRIORITY_MIN;
        int rc;
        char p[16];
        struct Inject_StatusParams InjectParams; 
  
         STLINUXInjectionDone = FALSE;

        pthread_attr_init(&attr);
 
        priority = sched_get_priority_min(SCHED_RR);
        sparam.sched_priority = priority;
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setschedparam(&attr, &sparam);

        strcpy(p,"stpti_0");
        rc = pthread_create(&task_inject_stream,&attr,Inject_Stream,(void*)p);
        
        task_delay(1000);
        priority = sched_get_priority_min(SCHED_RR);
        sparam.sched_priority = priority;
        pthread_attr_setschedparam(&attr, &sparam);
	 pthread_attr_setschedpolicy(&attr, SCHED_RR);

	 InjectParams.NoInterfacesSet=1;
        strcpy(InjectParams.DeviceName[0],"stpti_0");
        strcpy(InjectParams.InputInterface[0],"SWTS0");
        InjectParams.SlotHandle[0] = STPTI_Slot[0];
        rc = pthread_create(&task_inject_status,&attr,Inject_Status,(void*)&InjectParams);
        rc = pthread_join(task_inject_stream,NULL);
        rc = pthread_join(task_inject_status,NULL);
        STTBX_Print(("\n"));        

	while (Injection < MAX_INJECTION_TIME)
    	{
        task_delay( ST_GetClocksPerSecond()/10);

        /* Again Get packet count */
        Error = STPTI_GetInputPacketCount ( "stpti_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\n**************************************"));
        STTBX_Print(("\nPacket count after connection = %d",packetcountpost1));
        if (packetcountpre == packetcountpost1)
        {
            stmergetst_SetSuccessState(FALSE);
        }

        Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\nSlot count after connection   = %d",slotcount1));

        Injection ++;

    }
    STTBX_Print(("\n**************************************\n"));
        /* Get Packet count at PTI0 from TSIN0 */
        Error = STPTI_GetInputPacketCount ( "stpti_0", &packetcountpost0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from SWTS0     = %d",packetcountpost0));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from SWTS0        = %d",slotcount0));

        /* Get Packet count at PTI0 from SWTS0 */
        Error = STPTI_GetInputPacketCount ("stpti_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from TSIN0      = %d",packetcountpost1));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from TSIN0        = %d",slotcount1));
#endif

        if ((packetcountpost0 > 0) && (packetcountpost1 > 0) && (slotcount0 > 0) && (slotcount1 > 0))
         {
             stmergetst_SetSuccessState(TRUE);
         }
         else
         {
             stmergetst_SetSuccessState(FALSE);
         }

    STMERGE_Disconnect( SrcId,  DestId);
 
    /* Get Packet count at PTI0 from SWTS0 */
        Error = STPTI_GetInputPacketCount ("stpti_1", &packetcountpost2);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
  
        Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount2);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
     
    STTBX_Print(("\nPacket Count Test :Now TSIN0 disconnected \n")) ;
    STTBX_Print(("--------------------------------\n"));

    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

#if !defined(ST_OSLINUX)
           /* Initialise InjectTerm semaphore */
#ifdef ST_OS21
        InjectTerm_p = semaphore_create_fifo(0);
#else
        InjectTerm_p = &InjectTerm;
        semaphore_init_fifo(InjectTerm_p, 0);
#endif

	 strcpy(Params.DeviceName[0],DeviceName[0]);
        strcpy(Params.InputInterface[0],"SWTS0");
        Params.NoInterfacesSet = 2;
        Params.SlotHandle[0] = STPTI_Slot[0];
        strcpy(Params.DeviceName[1],DeviceName[1]);
        strcpy(Params.InputInterface[1],"TSIN0");
        Params.SlotHandle[1] = STPTI_Slot[1];
        Params.InjectTerm_p  = InjectTerm_p;
         /* create a task for printing the stream injection status */
        Task_p = task_create ((void (*)(void *)) Injection_Status, &Params, TASK_SIZE, 2, "Injection_Status", 0 );
        if ( Task_p == NULL )
        {
            STTBX_Print (("\n ------ Task Creation Failed ------ \n")) ;
            stmergetst_SetSuccessState(FALSE);
        }

         /* Play the Stream through SWTS 0 */
        InjectStream(SrcId1);

        Term_Injection_Status = TRUE;

        /* semaphore_wait for synchronisation */
        semaphore_wait(InjectTerm_p);

        /* semaphore_delete to free heap */
        semaphore_delete(InjectTerm_p);

        /* delete the task */
        task_delete ( Task_p ) ;

        /* reset flag */
        Term_Injection_Status = FALSE;

        Error = STPTI_GetInputPacketCount ( "stpti_0" , &packetcountpost0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Packet count after SWTS0 connection at PTI0 are %d \n",packetcountpost0));

        Error=  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Slot count after SWTS0 connection  = %d\n",slotcount0));
		
        /* Get packet Count */
        Error = STPTI_GetInputPacketCount ( "stpti_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Packet count after TSIN0 disconnection at PTI0 are %d \n",packetcountpost1));

	 Error=  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Slot count after TSIN0 disconnection  = %d\n",slotcount1));
#else

        STLINUXInjectionDone = FALSE;

        pthread_attr_init(&attr);
 
        priority = sched_get_priority_min(SCHED_RR);
        sparam.sched_priority = priority;
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setschedparam(&attr, &sparam);

        strcpy(p,"stpti_0");

        rc = pthread_create(&task_inject_stream,&attr,Inject_Stream,(void*)p);
        
        task_delay(1000);
        priority = sched_get_priority_min(SCHED_RR);
        sparam.sched_priority = priority;
        pthread_attr_setschedparam(&attr, &sparam);
	 pthread_attr_setschedpolicy(&attr, SCHED_RR);

	 InjectParams.NoInterfacesSet=1;
        strcpy(InjectParams.DeviceName[0],"stpti_0");
        strcpy(InjectParams.InputInterface[0],"SWTS0");
        InjectParams.SlotHandle[0] = STPTI_Slot[0];

        rc = pthread_create(&task_inject_status,&attr,Inject_Status,(void*)&InjectParams);
        rc = pthread_join(task_inject_stream,NULL);

        rc = pthread_join(task_inject_status,NULL);

        STTBX_Print(("\n"));

	 Injection = 0 ;
        while (Injection < MAX_INJECTION_TIME)
    	{
        task_delay( ST_GetClocksPerSecond()/10);

        /* Again Get packet count */
        Error = STPTI_GetInputPacketCount ( "stpti_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\n**************************************"));
        STTBX_Print(("\nPacket count after connection = %d",packetcountpost1));
        if (packetcountpre == packetcountpost1)
        {
            stmergetst_SetSuccessState(FALSE);
        }

        Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\nSlot count after connection   = %d",slotcount1));

        Injection ++;

    }
    STTBX_Print(("\n**************************************\n"));
        /* Get Packet count at PTI0 from TSIN0 */
        Error = STPTI_GetInputPacketCount ( "stpti_0", &packetcountpost0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from SWTS0     = %d",packetcountpost0));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from SWTS0        = %d",slotcount0));

        /* Get Packet count at PTI0 from SWTS0 */
        Error = STPTI_GetInputPacketCount ("stpti_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from TSIN0      = %d",packetcountpost1));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from TSIN0        = %d",slotcount1));

 #endif

	if ((packetcountpost1 == packetcountpost2)  && (slotcount1 == slotcount2 ))
         {
		STTBX_Print(("\nSlot count and packet count after TSIN0 disconnection is not increasing\n"));
	 	stmergetst_SetSuccessState(TRUE);
         }
         else
         {
             stmergetst_SetSuccessState(FALSE);
         }

    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term (DeviceName[0], &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    Error = STPTI_Term (DeviceName[1], &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#ifndef STMERGE_USE_TUNER
    /* Term Event Handler */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    stmergetst_TestConclusion("Packet Count Test : TSIN0 disconnected and SWTS0 connected");
    STTBX_Print(("============== Disconnect Test Exit     ======================\n"));
    return TRUE;
 }/* stmerge_DisconnectTest */


/****************************************************************************
Name         : stmerge_DuplicateInputTest()

Description  : Executes the Duplicate Input test

Parameters   :

Return Value : 

See Also     : Test plan
****************************************************************************/
BOOL stmerge_DuplicateInputTest(parse_t * pars_p, char *result_sym_p)
{
#if !defined(ST_7100) && !defined(ST_7109) && !defined(ST_5100)
    STTBX_Print(("\n============== Duplicate Input test ======================\n"));
    STTBX_Print(("============== Not supported  ===========================\n"));
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif
    STMERGE_InitParams_t            STMERGE_InitParams;
#if defined(ST_7109)
    U32                             SimpleRAMMap[7][2] = {
                                                              {STMERGE_TSIN_0,384},
                                                              {STMERGE_TSIN_1,384},
                                                              {STMERGE_TSIN_2,384},
                                                              {STMERGE_SWTS_0,384},
                                                              {STMERGE_SWTS_1,384},
                                                              {STMERGE_SWTS_2,384},
                                                              {0,0}
                                                             };
#else
    U32                             SimpleRAMMap[5][2] = {
                                                              {STMERGE_TSIN_0,384},
                                                              {STMERGE_TSIN_1,384},
                                                              {STMERGE_TSIN_2,384},
                                                              {STMERGE_SWTS_0,384},
                                                              {0,0}
                                                             };
#endif

#if defined(STMERGE_DEFAULT_PARAMETERS)
    STMERGE_ObjectConfig_t          DefaultConfig_TSIN,DefaultConfig_SWTS;
    STMERGE_ObjectConfig_t          TestData_TSIN,TestData_SWTS;
#endif

    STMERGE_ObjectConfig_t          TestStream_SetParams={0};
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle0,STPTI_Handle1;
    U16                             packetcountpre0=0,packetcountpre1=0;
    U16                             packetcountpost0=0,packetcountpost1=0;
    U16                             slotcount0,slotcount1;
    STPTI_TermParams_t              STPTI_TermParams;
    STPTI_Slot_t                    STPTI_Slot[2];

#if defined(POLL_PTI_ERROR_EVENTS)
#ifdef STMERGE_DTV_PACKET
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_VIDEO_ES,
#else
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_PES,
#endif
#else
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_RAW,
#endif  /* POLL_PTI_ERROR_EVENTS */
                                                       {FALSE, FALSE, FALSE,
                                                        FALSE, FALSE, FALSE,
                                                        FALSE, FALSE}
                                                     };

#if defined(POLL_PTI_ERROR_EVENTS)
    STPTI_DMAParams_t               STPTI_DMAParams;
#endif
    int            Injection = 0,i =0;
    S32 LVar;
    U32 SrcId,SrcId1,DestId,DestId1;
    BOOL RetErr = FALSE;


    STTBX_Print(("\n============== Duplicate Input test ======================\n"));
    STTBX_Print(("A Stream should be Input at TSIN 0 or TSIN 1 for this test to execute\n"));	

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
        return FALSE;
    }

    SrcId = LVar;

    SrcId1 = STMERGE_TSIN_2;

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
        return FALSE;
    }

    DestId = LVar;

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected dest id (from 0 to 1)");
        return FALSE;
    }

    DestId1 = LVar;

    /* Init params Setting */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = &SimpleRAMMap;
    STMERGE_InitParams.Mode              = STMERGE_NORMAL_OPERATION;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;

#if defined(STMERGE_INTERRUPT_SUPPORT)
    STMERGE_InitParams.InterruptNumber   = INTERRUPT_NUM;
    STMERGE_InitParams.InterruptLevel    = INTERRUPT_LEVEL;
#endif

    /* Init merge */
    strcpy(STMERGE_Name,"MERGE0");
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Test data for TSIN0 */
    TestStream_SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    TestStream_SetParams.SOPSymbol = 0x47;

    TestStream_SetParams.u.TSIN.SerialNotParallel  = TRUE; /* FALSE in Parallel Mode */
    TestStream_SetParams.u.TSIN.InvertByteClk      = FALSE;
    TestStream_SetParams.u.TSIN.ByteAlignSOPSymbol = TRUE; /* FALSE for Parallel Mode */

#if defined(STMERGE_DTV_PACKET)
        TestStream_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DTV;
        TestStream_SetParams.u.TSIN.SyncNotAsync       = FALSE; /* Async mode */
        TestStream_SetParams.u.TSIN.ReplaceSOPSymbol   = TRUE;  /* ReplaceSOP=1 for DTV */
#else
        TestStream_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
        TestStream_SetParams.u.TSIN.SyncNotAsync       = TRUE;
        TestStream_SetParams.u.TSIN.ReplaceSOPSymbol   = FALSE;
#endif

     TestStream_SetParams.SyncLock  = 0;
     TestStream_SetParams.SyncDrop  = 0;
     STPTI_InitParams.SyncLock    = 0;
     STPTI_InitParams.SyncDrop    = 0;
    
    /* Set Parameters */
    Error = STMERGE_SetParams(SrcId,&TestStream_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    SetupDefaultInitparams( &STPTI_InitParams );
    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(DestId);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(DestId);
    STPTI_InitParams.TCCodes      = SupportedTCCodes();

#if !defined(ST_OSLINUX)
    STPTI_InitParams.TCLoader_p = SupportedLoaders();
#endif

#ifndef STMERGE_USE_TUNER
    Error = EVENT_Setup( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    STPTI_InitParams.StreamID          = GetPTIStreamId(SrcId);

    if ( DestId == STMERGE_PTI_0 )
    {
        i = 0;
    }
    else
    {
        i = 1;
    }

    /* Init PTI 0 */
    Error = STPTI_Init( "stpti_0", &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

   STMERGE_DuplicateInput(SrcId);
   stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    Error = STMERGE_SetParams(SrcId1,&TestStream_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet Count test\n"));
    STTBX_Print(("--------------------------------\n"));

    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    STPTI_InitParams.StreamID         = GetPTIStreamId(SrcId1);

    if ((IsValidPTIId(DestId1)) && (DestId1 != DestId) )
    {
	STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(DestId1);
    	STPTI_InitParams.InterruptNumber   = GetInterruptNumber(DestId1);
    }

    /* Init PTI*/
    Error = STPTI_Init(  "stpti_1" , &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    /* Open PTI0 */
    Error = STPTI_Open( "stpti_0",  &STPTI_OpenParams, &STPTI_Handle0 );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Open PTI1 */
    Error = STPTI_Open(  "stpti_1" ,  &STPTI_OpenParams, &STPTI_Handle1 );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle0, &STPTI_Slot[0], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot set PID for TSIN0 */
#ifdef STMERGE_USE_TUNER
    Error = STPTI_SlotSetPid( STPTI_Slot[0],Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    Error = STPTI_SlotSetPid( STPTI_Slot[0],TSIN_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle1, &STPTI_Slot[1], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot set PID for TSIN1 */
#ifdef STMERGE_USE_TUNER
    Error = STPTI_SlotSetPid( STPTI_Slot[1],Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    Error = STPTI_SlotSetPid( STPTI_Slot[1],TSIN_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Get packet Count */
    Error = STPTI_GetInputPacketCount ( "stpti_0" , &packetcountpre0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet count before connection at PTI0 are %d \n",packetcountpre0));
    /* Packet count should be zero before Connect */
    if (packetcountpre0 != 0)
    {
        stmergetst_SetSuccessState(FALSE);
    }

    /* Get packet Count */
    Error = STPTI_GetInputPacketCount ( "stpti_1", &packetcountpre1);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet count before connection at PTI1 are %d \n",packetcountpre1));
    /* Packet count should be zero before Connect */
    if (packetcountpre1 != 1)
    {
        stmergetst_SetSuccessState(FALSE);
    }

    /* Connect SrcId with DestId  */
    Error = STMERGE_Connect(SrcId,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Connect SrcId1 with DestId */
    if ((IsValidPTIId(DestId1)) && (DestId1 != DestId) )
    {
    Error = STMERGE_Connect(SrcId1,DestId1);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
    else
    {
    Error = STMERGE_Connect(SrcId1,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
	
    while (Injection < MAX_INJECTION_TIME)
    {
#if defined(ST_7100)
             task_delay(ST_GetClocksPerSecond());
#else
             task_delay(ST_GetClocksPerSecond()/100);
#endif
             STTBX_Print(("\n*********************************************"));
             /* Get Packet count at PTI0 from TSIN0 */
             Error = STPTI_GetInputPacketCount ( "stpti_0" , &packetcountpost0);
             stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
             STTBX_Print(("\nPacket count at PTI from TSIN0      = %d",packetcountpost0));

             Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount0);
             stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
             STTBX_Print(("\nSlot count at PTI from TSIN0        = %d",slotcount0));

             STTBX_Print(("\n"));

             /* Get Packet count at PTI0 from TSIN2 */
             Error = STPTI_GetInputPacketCount ("stpti_1", &packetcountpost1);
             stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
             STTBX_Print(("\nPacket count at PTI from TSIN2      = %d",packetcountpost1));

             Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
             stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
             STTBX_Print(("\nSlot count at PTI from TSIN2        = %d",slotcount1));

             Injection++;

     }
	
     STTBX_Print(("\n*********************************************\n"));

     if ((packetcountpost0 > 0) && (packetcountpost1 > 0) && (slotcount0 > 0) && (slotcount1 > 0))
     	{
       	stmergetst_SetSuccessState(TRUE);
	}
      else
      	{
       	stmergetst_SetSuccessState(FALSE);
        }

    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term ("stpti_0", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Term PTI */
    Error = STPTI_Term ("stpti_1", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#ifndef STMERGE_USE_TUNER
    /* Term Event Handler */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    stmergetst_TestConclusion("Duplicate Input test");
    STTBX_Print(("============== Duplicate Input Test Exit  ===========================\n"));
    return TRUE;

}/* stmerge_DuplicateInputTest */


#if defined(STMERGE_INTERRUPT_SUPPORT)
/****************************************************************************
Name         : stmerge_RAMOverflowIntTest()

Description  : Test ISR on SRAM overflow interrupt

Parameters   :

Return Value : BOOL

See Also     : Test plan
****************************************************************************/

BOOL stmerge_RAMOverflowIntTest(parse_t * pars_p, char *result_sym_p)
{
     STMERGE_InitParams_t            STMERGE_InitParams;
     U32                             SimpleRAMMap[3][2] = { {STMERGE_TSIN_0,256},
                                                           {STMERGE_SWTS_0,1024},
                                                           {0,0}
                                                         };


    STMERGE_ObjectConfig_t          TestTSIN_SetParams;
    ST_ErrorCode_t                  Error = ST_NO_ERROR, StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle;
    STPTI_TermParams_t              STPTI_TermParams;
    STPTI_Slot_t                    STPTI_Slot[1];
    U16                             packetcountpre  = 0,packetcountpost = 0;
    U16                             slotcount   = 0;
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_RAW,
                                                       {FALSE, FALSE, FALSE,
                                                        FALSE, FALSE, FALSE,
                                                        FALSE, FALSE}
                                                     };

    int                             Injection = 0;
    STTBX_Print(("\n============== RAM overflow test ======================\n"));
    STTBX_Print((" A Stream should be Input at TSIN 0 for this test to execute\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    /* Init params Setting */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = &SimpleRAMMap;
    STMERGE_InitParams.Mode              = STMERGE_NORMAL_OPERATION;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;

#if defined(STMERGE_INTERRUPT_SUPPORT)
    STMERGE_InitParams.InterruptNumber   = INTERRUPT_NUM;
    STMERGE_InitParams.InterruptLevel    = INTERRUPT_LEVEL;
#endif

    /* Init merge */
    strcpy(STMERGE_Name,"MERGE0");
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Test data for TSIN0 */
    TestTSIN_SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    TestTSIN_SetParams.SyncLock  = 0;
    TestTSIN_SetParams.SyncDrop  = 0;
    TestTSIN_SetParams.SOPSymbol = 0x47;

    TestTSIN_SetParams.u.TSIN.SerialNotParallel  = TRUE; /* FALSE in Parallel Mode */
    TestTSIN_SetParams.u.TSIN.InvertByteClk      = FALSE;
    TestTSIN_SetParams.u.TSIN.ByteAlignSOPSymbol = TRUE; /* FALSE for Parallel Mode */

#if defined(STMERGE_DTV_PACKET)
    TestTSIN_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DTV;
    TestTSIN_SetParams.u.TSIN.SyncNotAsync       = FALSE; /* Async mode */
    TestTSIN_SetParams.u.TSIN.ReplaceSOPSymbol   = TRUE;  /* ReplaceSOP=1 for DTV */
#else
    TestTSIN_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
    TestTSIN_SetParams.u.TSIN.SyncNotAsync       = TRUE;
    TestTSIN_SetParams.u.TSIN.ReplaceSOPSymbol   = FALSE;
#endif

    /* Set Parameters */
    Error = STMERGE_SetParams(STMERGE_TSIN_0,&TestTSIN_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    SetupDefaultInitparams( &STPTI_InitParams );
    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(STMERGE_PTI_0);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(STMERGE_PTI_0);
    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_TSIN0;

#if !defined(ST_OSLINUX)
    STPTI_InitParams.TCLoader_p = SupportedLoaders();
#endif

    STPTI_InitParams.SyncLock          = 0;
    STPTI_InitParams.SyncDrop          = 0;

#if !defined(STMERGE_USE_TUNER)
    Error = EVENT_Setup( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Init PTI 0 */
    Error = STPTI_Init( "stpti_0", &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    /* Open PTI 0 */
    Error = STPTI_Open( "stpti_0",  &STPTI_OpenParams, &STPTI_Handle );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle, &STPTI_Slot[0], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot set PID for TSIN 0 */
    Error = STPTI_SlotSetPid( STPTI_Slot[0],TSIN_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Connect TSIN0 and PTI0 */
    Error = STMERGE_Connect(STMERGE_TSIN_0,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("\nNow STMERGE ISR should be called \n"));
    while (Injection < MAX_INJECTION_TIME)
    {

        task_delay( ST_GetClocksPerSecond()/10);

        /* Again Get packet count */
        Error = STPTI_GetInputPacketCount ( "stpti_0", &packetcountpost);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\n**************************************"));
        STTBX_Print(("\nPacket count after connection = %d",packetcountpost));
        if (packetcountpre == packetcountpost)
        {
            stmergetst_SetSuccessState(FALSE);
        }

        Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\nSlot count after connection   = %d",slotcount));

        if (slotcount == 0)
        {
            STTBX_Print(("\nSlot count is not increasing"));
            stmergetst_SetSuccessState(FALSE);
        }

#if defined(ST_7710)
        STTBX_Print(("\nStatus Reg=0x%x \n",*(U32*)0x38020010));
#endif
       Injection ++;

    }

#ifndef STMERGE_USE_TUNER
    /* Term Event Handler */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term ("stpti_0", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);

    stmergetst_TestConclusion("RAM overflow test");
    STTBX_Print(("============== RAM Overflow Test Exit  ===========================\n"));
    return TRUE;

}/* stmerge_RAMOverflowIntTest */

#endif

void PrintPacketCount(char*p,char*q,STPTI_Slot_t slot)
{
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    U16                             packetcount,slotcount;

    STTBX_Print(("\n*********************************************"));
    Error = STPTI_GetInputPacketCount ( q , &packetcount);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    STTBX_Print(("\nPacket count at PTI0 from %s = %d",p,packetcount));

    STTBX_Print(("\n"));

    Error =  STPTI_SlotPacketCount( slot, &slotcount);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    STTBX_Print(("\nSlot count at PTI0 from %s = %d",q,slotcount));
    STTBX_Print(("\n*********************************************"));
}


/****************************************************************************
Name         : stmerge_Merge_2_1_Test()

Description  : Executes 2:1 Merge test

Parameters   :

Return Value : none

See Also     : Test plan
****************************************************************************/

BOOL stmerge_Merge_2_1_Test(parse_t * pars_p, char *result_sym_p)
{

    STMERGE_InitParams_t            STMERGE_InitParams;
    U32                                 SimpleRAMMap[5][2] = {
                                                              {STMERGE_TSIN_0,384},
                                                              {STMERGE_TSIN_1,384},
                                                              {STMERGE_TSIN_2,384},
                                                              {STMERGE_SWTS_0,384},
                                                              {0,0}
                                                             };

#if defined(STMERGE_DEFAULT_PARAMETERS)
    STMERGE_ObjectConfig_t          DefaultConfig_TSIN,DefaultConfig_SWTS;
    STMERGE_ObjectConfig_t          TestData_TSIN,TestData_SWTS;
#endif
    U16                             packetcountpre0=0,packetcountpre1=0;
    U16                             packetcountpost0=0,packetcountpost1=0;
    U16                             slotcount0,slotcount1;
    STMERGE_ObjectConfig_t          TestTSIN_SetParams,STMERGE_SWTSParameters;
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle0,STPTI_Handle1;
    STPTI_TermParams_t              STPTI_TermParams;
    STPTI_Slot_t                    STPTI_Slot[2];
    U32                             Injection = 0,i =0;
    S32 LVar;
    U32 SrcId,SrcId1,DestId;
    BOOL RetErr = FALSE;

#if defined(POLL_PTI_ERROR_EVENTS)
#ifdef STMERGE_DTV_PACKET
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_VIDEO_ES,
#else
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_PES,
#endif
#else
    STPTI_SlotData_t                STPTI_SlotData = { STPTI_SLOT_TYPE_RAW,
#endif  /* POLL_PTI_ERROR_EVENTS */
                                                       {FALSE, FALSE, FALSE,
                                                        FALSE, FALSE, FALSE,
                                                        FALSE, FALSE}
                                                     };

    STTBX_Print(("\n==============2:1 Merge test ======================\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

#if defined(STMERGE_DEFAULT_PARAMETERS)
    STTBX_Print(("Default values of TSIN and SWTS  Test\n"));
    STTBX_Print(("--------------------------------------\n"));
#endif

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
        return FALSE;
    }

    SrcId = LVar;

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected 2nd src id (from 0 to 11)");
        return FALSE;
    }

    SrcId1 = LVar;

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected dest id (from 0 to 1)");
    }

    DestId = LVar;

    /* Init params Setting */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = &SimpleRAMMap;
    STMERGE_InitParams.Mode              = STMERGE_NORMAL_OPERATION;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;

#if defined(STMERGE_INTERRUPT_SUPPORT)
    STMERGE_InitParams.InterruptNumber   = INTERRUPT_NUM;
    STMERGE_InitParams.InterruptLevel    = INTERRUPT_LEVEL;
#endif

    /* Init merge */
    strcpy(STMERGE_Name,"MERGE0");
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Test data for TSIN0 */
    TestTSIN_SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    TestTSIN_SetParams.SyncLock  = 0;
    TestTSIN_SetParams.SyncDrop  = 0;
    TestTSIN_SetParams.SOPSymbol = 0x47;

    TestTSIN_SetParams.u.TSIN.SerialNotParallel  = TRUE; /* FALSE in Parallel Mode */
    TestTSIN_SetParams.u.TSIN.InvertByteClk      = FALSE;
    TestTSIN_SetParams.u.TSIN.ByteAlignSOPSymbol = TRUE; /* FALSE for Parallel Mode */

#if defined(STMERGE_DTV_PACKET)
    TestTSIN_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DTV;
    TestTSIN_SetParams.u.TSIN.SyncNotAsync       = FALSE; /* Async mode */
    TestTSIN_SetParams.u.TSIN.ReplaceSOPSymbol   = TRUE;  /* ReplaceSOP=1 for DTV */
#else
    TestTSIN_SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
    TestTSIN_SetParams.u.TSIN.SyncNotAsync       = TRUE;
    TestTSIN_SetParams.u.TSIN.ReplaceSOPSymbol   = FALSE;
#endif

    /* Set params and Get Params function checking */
    STTBX_Print(("SetParams Test\n"));
    STTBX_Print(("-----------------------\n"));

    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Set Parameters for TSIN0 */
    Error = STMERGE_SetParams(SrcId,&TestTSIN_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    if ( IsValidSWTSId(SrcId1) )
    {

        /* SWTS0 Set Parameters */
        STMERGE_SWTSParameters.Priority  = STMERGE_PRIORITY_HIGHEST;
        STMERGE_SWTSParameters.SyncLock  = 1;
        STMERGE_SWTSParameters.SyncDrop  = 3;
        STMERGE_SWTSParameters.SOPSymbol = 0x47;

#if defined(STMERGE_DTV_PACKET)
        STMERGE_SWTSParameters.PacketLength = STMERGE_PACKET_LENGTH_DTV_WITH_2B_STUFFING;
#else
        STMERGE_SWTSParameters.PacketLength = STMERGE_PACKET_LENGTH_DVB;
#endif

        STMERGE_SWTSParameters.u.SWTS.Tagging       = STMERGE_TAG_ADD;
        STMERGE_SWTSParameters.u.SWTS.Pace          = STMERGE_PACE_AUTO;
        STMERGE_SWTSParameters.u.SWTS.Counter.Value = STMERGE_COUNTER_AUTO_LOAD;
        STMERGE_SWTSParameters.u.SWTS.Counter.Rate  = STMERGE_COUNTER_NO_INC;

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109)
        STMERGE_SWTSParameters.u.SWTS.Trigger  = 10;
#endif
        Error = STMERGE_SetParams(SrcId1,&STMERGE_SWTSParameters);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }else {
       /* Test data for TSIN1 */
       TestTSIN_SetParams.Priority  = STMERGE_PRIORITY_MID;

         /* These lines are commented because data is converted serially */
#if defined(STMERGE_PARALLEL_INPUT)
       TestTSIN_SetParams.u.TSIN.SerialNotParallel  = FALSE; /* FALSE in Parallel Mode */ 
       TestTSIN_SetParams.u.TSIN.ByteAlignSOPSymbol = FALSE; /* FALSE for Parallel Mode */
#endif
       /* Set Params for TSIN1 */
       Error = STMERGE_SetParams(SrcId1,&TestTSIN_SetParams);
       stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
   }

    SetupDefaultInitparams( &STPTI_InitParams );
    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(DestId);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(DestId);
    STPTI_InitParams.TCCodes      = SupportedTCCodes();

#if !defined(ST_OSLINUX)
    STPTI_InitParams.TCLoader_p = SupportedLoaders();
#endif

#ifndef STMERGE_USE_TUNER
    Error = EVENT_Setup( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    STPTI_InitParams.StreamID          = GetPTIStreamId(SrcId);

    if ( DestId == STMERGE_PTI_0 )
    {
        i = 0;
    }
    else
    {
        i = 1;
    }

    /* Init PTI 0 */
    Error = STPTI_Init( "stpti_0", &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_InitParams.StreamID          = GetPTIStreamId(SrcId1);

    /* Init PTI0 */
    Error = STPTI_Init(  "stpti_0_1" , &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    /* Open PTI0 */
    Error = STPTI_Open( "stpti_0",  &STPTI_OpenParams, &STPTI_Handle0 );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Open PTI1 */
    Error = STPTI_Open(  "stpti_0_1" ,  &STPTI_OpenParams, &STPTI_Handle1 );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if defined(STMERGE_DTV_PACKET)
    if ( SrcId1 == STMERGE_SWTS_0 )
    {
         STPTI_SetDiscardParams("stpti_0_1", 2);
#if 0
         /* This function can be required in the future. Right now the tests are passing without it, but
            the STPTI4 test harness is using this function. */
         TSMERGER_IIFSetSyncPeriod(132);
#endif
    }
#endif

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle0, &STPTI_Slot[0], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot set PID for TSIN0 */
#ifdef STMERGE_USE_TUNER
    Error = STPTI_SlotSetPid( STPTI_Slot[0],Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    Error = STPTI_SlotSetPid( STPTI_Slot[0],TSIN_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle1, &STPTI_Slot[1], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot set PID for SWTS0 */
    Error = STPTI_SlotSetPid( STPTI_Slot[1],SWTS_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Get packet Count */
    Error = STPTI_GetInputPacketCount ( "stpti_0" , &packetcountpre0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet count before connection at PTI0 are %d \n",packetcountpre0));
    /* Packet count should be zero before Connect */
    if (packetcountpre0 != 0)
    {
        stmergetst_SetSuccessState(FALSE);
    }

    /* Get packet Count */
    Error = STPTI_GetInputPacketCount ( "stpti_0_1", &packetcountpre1);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet count before connection at PTI1 are %d \n",packetcountpre1));
    /* Packet count should be zero before Connect */
    if (packetcountpre1 != 1)
    {
        stmergetst_SetSuccessState(FALSE);
    }

    /* Connect SrcId with DestId  */
    Error = STMERGE_Connect(SrcId,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Connect SrcId1 with DestId */
    Error = STMERGE_Connect(SrcId1,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    if ( IsValidSWTSId(SrcId) || IsValidSWTSId(SrcId1) )
    {
#if defined(ST_OSLINUX)
#define PRIORITY_MIN 0
        pthread_attr_t attr;
        pthread_t task_inject_stream,task_inject_status;
        struct sched_param sparam;
        int priority=PRIORITY_MIN;
        int rc;
        char p[16];
        struct Inject_StatusParams InjectParams; 
  
        STTBX_Print(("\n================ FILE PLAY RUNNING ==================\n")) ;
        STLINUXInjectionDone = FALSE;

        pthread_attr_init(&attr);
 
        priority = sched_get_priority_min(SCHED_RR);
        sparam.sched_priority = priority;
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setschedparam(&attr, &sparam);

        strcpy(p,"stpti_0");
        printf(" Creating thread 1 \n");
        rc = pthread_create(&task_inject_stream,&attr,Inject_Stream,(void*)p);
        
        task_delay(1000);
        priority = sched_get_priority_min(SCHED_RR);
        sparam.sched_priority = priority;
        pthread_attr_setschedparam(&attr, &sparam);

        InjectParams.NoInterfacesSet=2;
        strcpy(InjectParams.DeviceName[0],"stpti_0");
        strcpy(InjectParams.InputInterface[0],"TSIN0");
        InjectParams.SlotHandle[0] = STPTI_Slot[0];
        strcpy(InjectParams.DeviceName[1],"stpti_0_1");
        strcpy(InjectParams.InputInterface[1],"SWTS0");
        InjectParams.SlotHandle[1] = STPTI_Slot[1];
        printf(" Creating thread 2 \n");
        rc = pthread_create(&task_inject_status,&attr,Inject_Status,(void*)&InjectParams);

        rc = pthread_join(task_inject_stream,NULL);
        printf(" rc : %d \n",rc);
        rc = pthread_join(task_inject_status,NULL);
        printf(" rc : %d \n",rc);
        STTBX_Print(("\n"));
        STTBX_Print(("\n================ FILE PLAY EXITTING ==================\n")) ;

        /* Get Packet count at PTI0 from TSIN0 */
        Error = STPTI_GetInputPacketCount ( "stpti_0", &packetcountpost0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from TSIN0     = %d",packetcountpost0));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from TSIN0        = %d",slotcount0));

        /* Get Packet count at PTI0 from SWTS0 */
        Error = STPTI_GetInputPacketCount ("stpti_0_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from SWTS0      = %d",packetcountpost1));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from SWTS0        = %d",slotcount1));

        /* Set Injection to MAX_INJECTION_TIME */
        Injection = MAX_INJECTION_TIME;
#else
        STMERGE_TaskParams_t            Params;
#ifndef ST_OS21
        semaphore_t                     InjectTerm;
#endif
        semaphore_t                     *InjectTerm_p;
        task_t*                         Task_p ;
        /* Initialise InjectTerm semaphore */
#ifdef ST_OS21
        InjectTerm_p = semaphore_create_fifo(0);
#else
        InjectTerm_p = &InjectTerm;
        semaphore_init_fifo(InjectTerm_p, 0);
#endif
        Params.NoInterfacesSet = 2;

        strcpy(Params.DeviceName[0],"stpti_0");
        strcpy(Params.InputInterface[0],"TSIN0");
        Params.SlotHandle[0] = STPTI_Slot[0];
        Params.InjectTerm_p  = InjectTerm_p;

        strcpy(Params.DeviceName[1],"stpti_0_1");
        strcpy(Params.InputInterface[1],"SWTS0");
        Params.SlotHandle[1] = STPTI_Slot[1];
        Params.InjectTerm_p  = InjectTerm_p;

        Term_Injection_Status = FALSE;


        STTBX_Print(("\n================ FILE PLAY RUNNING ==================\n")) ;



        /* create a task for printing the stream injection status */
        Task_p = task_create ((void (*)(void *)) Injection_Status, &Params, TASK_SIZE, 0, "Injection_Status", 0 );
        if ( Task_p == NULL )
        {
            STTBX_Print (("\n ------ Task Creation Failed ------ \n")) ;
            stmergetst_SetSuccessState(FALSE);
        }
        /* Play the Stream through SWTS 0 */
        InjectStream(STMERGE_SWTS_0);

        Term_Injection_Status = TRUE;

        /* semaphore_wait for synchronisation */
        semaphore_wait(InjectTerm_p);

        STTBX_Print(("\n"));
        STTBX_Print(("\n================ FILE PLAY EXITTING ==================\n")) ;

        /* semaphore_delete to free heap */
        semaphore_delete(InjectTerm_p);

        /* delete the task */
        task_delete ( Task_p ) ;

        /* reset flag */
        Term_Injection_Status = FALSE;

        Injection = MAX_INJECTION_TIME;
        /* Get packet Count */
        Error = STPTI_GetInputPacketCount ( "stpti_0" , &packetcountpost0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Packet count after connection at PTI0 are %d \n",packetcountpost0));
        /* Get packet Count */
        Error = STPTI_GetInputPacketCount ( "stpti_0_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("Packet count after connection at PTI1 are %d \n",packetcountpost1));
#endif
}

    if( IsValidTSINId(SrcId) && IsValidTSINId(SrcId1))
    {
         while(Injection < MAX_INJECTION_TIME)
         {


#if defined(ST_7100)
             task_delay(ST_GetClocksPerSecond());
#else
             task_delay(ST_GetClocksPerSecond()/100);
#endif
             STTBX_Print(("\n*********************************************"));
             /* Get Packet count at PTI0 from TSIN0 */
             Error = STPTI_GetInputPacketCount ( "stpti_0" , &packetcountpost0);
             stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
             STTBX_Print(("\nPacket count at PTI0 from TSIN0      = %d",packetcountpost0));

             Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount0);
             stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
             STTBX_Print(("\nSlot count at PTI0 from TSIN0        = %d",slotcount0));

             STTBX_Print(("\n"));

             /* Get Packet count at PTI0 from TSIN1 */
             Error = STPTI_GetInputPacketCount ("stpti_0_1", &packetcountpost1);
             stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
             STTBX_Print(("\nPacket count at PTI0 from TSIN1      = %d",packetcountpost1));

             Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
             stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
             STTBX_Print(("\nSlot count at PTI0 from TSIN1        = %d",slotcount1));

             Injection++;

         }

         STTBX_Print(("\n*********************************************\n"));


         if ((packetcountpost0 > 0) && (packetcountpost1 > 0) && (slotcount0 > 0) && (slotcount1 > 0))
         {
             stmergetst_SetSuccessState(TRUE);
         }
         else
         {
             stmergetst_SetSuccessState(FALSE);
         }
    }

    if ( IsValidSWTSId(SrcId) || IsValidSWTSId(SrcId1) )
    {
#if defined(ST_OSLINUX)
         if ((packetcountpost0 > 0) && (packetcountpost1 > 0) && (slotcount0 > 0) && (slotcount1 > 0))
         {
             STTBX_Print(("\nPackets are received both from TSIN0 & TSIN1 to PTI0\n"));
             stmergetst_SetSuccessState(TRUE);
         }
         else
         {
             stmergetst_SetSuccessState(FALSE);
         }
#else
         if ((packetcountpost0 > 0) && (packetcountpost1 > 0) )
         {
             STTBX_Print(("\nPackets are received both from TSIN0 & TSIN1 to PTI0\n"));
             stmergetst_SetSuccessState(TRUE);
         }
         else
         {
             stmergetst_SetSuccessState(FALSE);
         }
#endif
    }
    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);

    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term ("stpti_0", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term ("stpti_0_1", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#ifndef STMERGE_USE_TUNER
    /* Term Event Handler */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    stmergetst_TestConclusion("2:1 Merge test");
    STTBX_Print(("============== 2:1 Merge Test Exit  ===========================\n"));
    return TRUE;

 }/* stmerge_Merge_2_1_Test */

/****************************************************************************
Name         : stmerge_Merge_3_1_Test()

Description  : TSIN merging test of STMERGE

Parameters   :

Return Value : none

See Also     : Test plan
****************************************************************************/

BOOL stmerge_Merge_3_1_Test(parse_t * pars_p, char *result_sym_p)
{
    STMERGE_InitParams_t                STMERGE_InitParams;
    STMERGE_ObjectConfig_t              STMERGE_TSINParameters,STMERGE_SWTSParameters;
    STPTI_InitParams_t                  STPTI_InitParams;
    STPTI_OpenParams_t                  STPTI_OpenParams;
    ST_ErrorCode_t                      Error = ST_NO_ERROR;
    ST_ErrorCode_t                      StoredError = ST_NO_ERROR;
    STPTI_TermParams_t                  STPTI_TermParams;
    STPTI_Handle_t                      STPTI_Handle[MAX_SESSIONS_PER_CELL];
    U32                                 SimpleRAMMap[5][2] = {
                                                              {STMERGE_TSIN_0,384},
                                                              {STMERGE_TSIN_1,384},
                                                              {STMERGE_TSIN_2,384},
                                                              {STMERGE_SWTS_0,384},
                                                              {0,0}
                                                             };
#if !defined(ST_OSLINUX)
    U16                                 packetcount[MAX_SESSIONS_PER_CELL]={0,0,0},
                                        slotcount[MAX_SESSIONS_PER_CELL]  ={0,0,0};
#endif
    STPTI_Slot_t                        STPTI_Slot[MAX_SESSIONS_PER_CELL];
#if defined(STMERGE_DTV_PACKET)
    STPTI_Pid_t                         STPTI_Pid[MAX_SESSIONS_PER_CELL] =
                                        {0x64/*TSIN0*/,0x14/*TSIN1*/,0x64/*SWTS1*/};
#else
    STPTI_Pid_t                         STPTI_Pid[MAX_SESSIONS_PER_CELL] =
                                        {0x280/*TSIN0/2*/,0x3C/*TSIN1/3*/,0x3C/*SWTS0*/};
#endif
    STPTI_SlotData_t                    STPTI_SlotData = { STPTI_SLOT_TYPE_RAW,
                                                        {FALSE, FALSE, FALSE,
                                                        FALSE, FALSE, FALSE,
                                                        FALSE, FALSE} };

    S32 LVar;
    U32 SrcId,SrcId1,SrcId2,DestId;
    BOOL RetErr = FALSE;

    STTBX_Print(("\n========================== 3:1 Merge test ==========================\n"));
    STTBX_Print(("Streams of diff PIDs should be injected at 2 TSINs to execute test\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
        return FALSE;
    }

    SrcId = LVar;

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
        return FALSE;
    }

    SrcId1 = LVar;

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
        return FALSE;
    }

    SrcId2 = LVar;

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected dest id (from 0 to 1)");
        return FALSE;
    }

    DestId = LVar;

    /* Initialize STMERGE */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = &SimpleRAMMap;
    STMERGE_InitParams.Mode              = STMERGE_NORMAL_OPERATION;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;
#if defined(STMERGE_INTERRUPT_SUPPORT)
    STMERGE_InitParams.InterruptNumber   = INTERRUPT_NUM;
    STMERGE_InitParams.InterruptLevel    = INTERRUPT_LEVEL;
#endif

    Error = STMERGE_Init("MERGE0",&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Generic test parameters */
    /* These settings are used for TSIN0 and TSIN1 */
    STMERGE_TSINParameters.Priority                  = STMERGE_PRIORITY_HIGHEST;
    STMERGE_TSINParameters.SyncLock                  = 0;
    STMERGE_TSINParameters.SyncDrop                  = 0;
    STMERGE_TSINParameters.SOPSymbol                 = 0x47;

    STMERGE_TSINParameters.u.TSIN.SerialNotParallel  = TRUE;
    STMERGE_TSINParameters.u.TSIN.InvertByteClk      = FALSE;
    STMERGE_TSINParameters.u.TSIN.ByteAlignSOPSymbol = TRUE;

#if defined(STMERGE_DTV_PACKET)
    STMERGE_TSINParameters.PacketLength              = STMERGE_PACKET_LENGTH_DTV;
    STMERGE_TSINParameters.u.TSIN.SyncNotAsync       = FALSE;/* Async mode */
    STMERGE_TSINParameters.u.TSIN.ReplaceSOPSymbol   = TRUE; /* ReplaceSOP=1 for DTV */
#else
    STMERGE_TSINParameters.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
    STMERGE_TSINParameters.u.TSIN.SyncNotAsync       = TRUE;
    STMERGE_TSINParameters.u.TSIN.ReplaceSOPSymbol   = FALSE;
#endif

    /* Set Parameters for TSIN0 */
    Error = STMERGE_SetParams(SrcId,&STMERGE_TSINParameters);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* This configuration has not been tested yet on LINUX. */
    /* Set Parameters for TSIN1 */
    STMERGE_TSINParameters.Priority                  = STMERGE_PRIORITY_MID;
#if defined(STMERGE_PARALLEL_INPUT)
    STMERGE_TSINParameters.u.TSIN.SerialNotParallel  = FALSE;/* parallel */
    STMERGE_TSINParameters.u.TSIN.ByteAlignSOPSymbol = FALSE;/* parallel */
#endif
    Error = STMERGE_SetParams(SrcId1,&STMERGE_TSINParameters);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* SWTS0 Set Parameters */
    STMERGE_SWTSParameters.Priority  = STMERGE_PRIORITY_HIGHEST;
    STMERGE_SWTSParameters.SyncLock  = 1;
    STMERGE_SWTSParameters.SyncDrop  = 3;
    STMERGE_SWTSParameters.SOPSymbol = 0x47;

#if defined(STMERGE_DTV_PACKET)
    STMERGE_SWTSParameters.PacketLength = STMERGE_PACKET_LENGTH_DTV_WITH_2B_STUFFING;
#else
    STMERGE_SWTSParameters.PacketLength = STMERGE_PACKET_LENGTH_DVB;
#endif

    STMERGE_SWTSParameters.u.SWTS.Tagging       = STMERGE_TAG_ADD;
    STMERGE_SWTSParameters.u.SWTS.Pace          = STMERGE_PACE_AUTO;
    STMERGE_SWTSParameters.u.SWTS.Counter.Value = STMERGE_COUNTER_AUTO_LOAD;
    STMERGE_SWTSParameters.u.SWTS.Counter.Rate  = STMERGE_COUNTER_NO_INC;

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109)
    STMERGE_SWTSParameters.u.SWTS.Trigger  = 10;
#endif

    /* Set Params for SWTS0 */
    Error = STMERGE_SetParams(SrcId2,&STMERGE_SWTSParameters);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#ifndef STMERGE_USE_TUNER
    Error = EVENT_Setup( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    SetupDefaultInitparams( &STPTI_InitParams );
    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(DestId);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(DestId);
    STPTI_InitParams.TCCodes      = SupportedTCCodes();

#if !defined(ST_OSLINUX)
    STPTI_InitParams.TCLoader_p = SupportedLoaders();
#endif
     
    /* Configure PTI for TSIN0 */
    STPTI_InitParams.StreamID = GetPTIStreamId(SrcId);
    Error = STPTI_Init("stpti",&STPTI_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Configure PTI for TSIN1  */
    STPTI_InitParams.StreamID = GetPTIStreamId(SrcId1);
    Error = STPTI_Init("stpti_1",&STPTI_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Configure PTI for SWTS0 */
    STPTI_InitParams.StreamID = GetPTIStreamId(SrcId2);
    Error = STPTI_Init("stpti_2",&STPTI_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Open  PTI */
    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    /* Open PTI0 */
    Error = STPTI_Open("stpti" , &STPTI_OpenParams, &STPTI_Handle[0]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Open PTI1 */
    Error = STPTI_Open("stpti_1" , &STPTI_OpenParams, &STPTI_Handle[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Open PTI22*/
    Error = STPTI_Open("stpti_2" , &STPTI_OpenParams, &STPTI_Handle[2]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if defined(STMERGE_DTV_PACKET)
    if ( SrcId2 == STMERGE_SWTS_0 )
    {
         STPTI_SetDiscardParams("stpti_2", 2);
#if 0
         /* This function can be required in the future. Right now the tests are passing without it, but
            the STPTI4 test harness is using this function. */
         TSMERGER_IIFSetSyncPeriod(132);
#endif
    }
#endif

    /* Allocate Slot for TSIN0 */
    Error = STPTI_SlotAllocate( STPTI_Handle[0], &STPTI_Slot[0], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if defined(STMERGE_USE_TUNER)
    /* Slot set PID for TSIN0 */
    Error = STPTI_SlotSetPid( STPTI_Slot[0],Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    Error = STPTI_SlotSetPid( STPTI_Slot[0],STPTI_Pid[0]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Allocate Slot for TSIN1 */
    Error = STPTI_SlotAllocate( STPTI_Handle[1], &STPTI_Slot[1], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot set PID for TSIN1 */
    Error = STPTI_SlotSetPid( STPTI_Slot[1], STPTI_Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot for SWTS0 */
    Error = STPTI_SlotAllocate( STPTI_Handle[2], &STPTI_Slot[2], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot set PID for SWTS0 */
#if !defined(ST_OSLINUX)
    Error = STPTI_SlotSetPid( STPTI_Slot[2],SWTS_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    /* This is for multi2.ts through SWTS0 */
    Error = STPTI_SlotSetPid( STPTI_Slot[2],0x10A4);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Connect SrcId with DestId  */
    Error = STMERGE_Connect(SrcId,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Connect SrcId1 with DestId  */
    Error = STMERGE_Connect(SrcId1,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Connect SrcId2 with DestId  */
    Error = STMERGE_Connect(SrcId2,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if !defined(ST_OSLINUX)
    if ( IsValidSWTSId(SrcId) )
    {
        InjectStream(SrcId);
    }
    else if (IsValidSWTSId(SrcId1))
    {
        InjectStream(SrcId1);
    }
    else if (IsValidSWTSId(SrcId2))
    {
        InjectStream(SrcId2);
    }

    /* Packet count */
    Error = STPTI_GetInputPacketCount ("stpti", &packetcount[0]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot Count */
    Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount[0]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("\n-----------------------------------------------------\n"));
    STTBX_Print(("Packet count at PTI from TSIN stream = %u\n",packetcount[0]));
    STTBX_Print(("Slot   count at PTI from TSIN stream = %u\n",slotcount[0]));
    STTBX_Print(("\n-----------------------------------------------------\n"));

    task_delay(ST_GetClocksPerSecond()/100);

    /* Packet count */
    Error = STPTI_GetInputPacketCount ("stpti_1", &packetcount[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot Count */
    Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("\n-----------------------------------------------------\n"));
    STTBX_Print(("Packet count at PTI from TSIN1 stream = %u\n",packetcount[1]));
    STTBX_Print(("Slot   count at PTI from TSIN1 stream = %u\n",slotcount[1]));
    STTBX_Print(("\n-----------------------------------------------------\n"));

    /* Packet count at PTI0 from SWTS0 */
    Error = STPTI_GetInputPacketCount ("stpti_2", &packetcount[2]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Slot count at PTI0 from SWTS0 */
    Error =  STPTI_SlotPacketCount( STPTI_Slot[2], &slotcount[2]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("\n-----------------------------------------------------\n"));
    STTBX_Print(("Packet count at PTI from SWTS0 stream = %u\n",packetcount[2]));
    STTBX_Print(("Slot   count at PTI from SWTS0 stream = %u\n",slotcount[2]));
    STTBX_Print(("\n-----------------------------------------------------\n"));

    if ((slotcount[0] > 0) && (slotcount[1] > 0) && (slotcount[2] > 0))
    {
        STTBX_Print(("All the 3 PIDs are obtained at PTI0\n"));
        STTBX_Print(("Merging is SUCCESSFUL\n"));
        stmergetst_SetSuccessState(TRUE);
    }
    else
    {
        STTBX_Print(("Merging FAILED\n"));
        stmergetst_SetSuccessState(FALSE);
    }
#else
#define PRIORITY_MIN 0
        pthread_attr_t attr;
        pthread_t task_inject_stream,task_inject_status;
        struct sched_param sparam;
        int priority=PRIORITY_MIN;
        int rc;
        char p[16];
        U16 packetcountpost0,slotcount0,packetcountpost1,slotcount1,packetcountpost2,slotcount2,Injection=0;
        struct Inject_StatusParams InjectParams; 
  
        STTBX_Print(("\n================ FILE PLAY RUNNING ==================\n")) ;
        STLINUXInjectionDone = FALSE;

        pthread_attr_init(&attr);
 
        priority = sched_get_priority_min(SCHED_RR);
        sparam.sched_priority = priority;
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setschedparam(&attr, &sparam);

        strcpy(p,"stpti_0");
        printf(" Creating thread 1 \n");
        rc = pthread_create(&task_inject_stream,&attr,Inject_Stream,(void*)p);
        
        task_delay(1000);
        priority = sched_get_priority_min(SCHED_RR);
        sparam.sched_priority = priority;
        pthread_attr_setschedparam(&attr, &sparam);

        InjectParams.NoInterfacesSet=3;
        strcpy(InjectParams.DeviceName[0],"stpti");
        strcpy(InjectParams.InputInterface[0],"TSIN0");
        InjectParams.SlotHandle[0] = STPTI_Slot[0];
        strcpy(InjectParams.DeviceName[1],"stpti_1");
        strcpy(InjectParams.InputInterface[1],"TSIN1");
        InjectParams.SlotHandle[1] = STPTI_Slot[1];
        strcpy(InjectParams.DeviceName[2],"stpti_2");
        strcpy(InjectParams.InputInterface[2],"SWTS0");
        InjectParams.SlotHandle[2] = STPTI_Slot[2];
        printf(" Creating thread 2 \n");
        rc = pthread_create(&task_inject_status,&attr,Inject_Status,(void*)&InjectParams);

        rc = pthread_join(task_inject_stream,NULL);
        printf(" rc : %d \n",rc);
        rc = pthread_join(task_inject_status,NULL);
        printf(" rc : %d \n",rc);
        STTBX_Print(("\n"));
        STTBX_Print(("\n================ FILE PLAY EXITTING ==================\n")) ;

        /* Get Packet count at PTI0 from TSIN0 */
        Error = STPTI_GetInputPacketCount ( "stpti", &packetcountpost0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from TSIN0     = %d",packetcountpost0));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount0);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from TSIN0        = %d",slotcount0));

        /* Get Packet count at PTI0 from TSIN1 */
        Error = STPTI_GetInputPacketCount ("stpti_1", &packetcountpost1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from TSIN1      = %d",packetcountpost1));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[1], &slotcount1);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from TSIN1        = %d",slotcount1));

        /* Get Packet count at PTI0 from SWTS0 */
        Error = STPTI_GetInputPacketCount ("stpti_2", &packetcountpost2);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTI0 from SWTS0      = %d",packetcountpost2));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[2], &slotcount2);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count at PTI0 from SWTS0        = %d",slotcount2));

        /* Set Injection to MAX_INJECTION_TIME */
        Injection = MAX_INJECTION_TIME;
#endif
    Error = STMERGE_Term("MERGE0",NULL);
    stmergetst_TestConclusion("3:1 Merge test");

    /* Terminate PTI0 connections */
    STPTI_TermParams.ForceTermination = TRUE;

    /* Terminate PTI Instance */
    Error = STPTI_Term("stpti", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Terminate PTI Instance */
    Error = STPTI_Term("stpti_1", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Terminate PTI Instance */
    Error = STPTI_Term("stpti_2", &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#ifndef STMERGE_USE_TUNER
    /* Terminate Event handler for PTI */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    STTBX_Print(("==============3:1 Merge Test Exit ===========================\n"));

    return TRUE;

}/* stmerge_Merge_3_1_Test */


/****************************************************************************
Name         : stmerge_Runall()

Description  : Execute all The tests

Parameters   :

Return Value : BOOL

See Also     : Test plan
****************************************************************************/

BOOL stmerge_Runall(parse_t * pars_p, char *result_sym_p)
{
    BOOL RetErr = FALSE;

    STTBX_Print(("============== Execute all the tests  =======================\n"));

    /* Execute Errant test  */
    RetErr |= stmerge_ErrantTest(pars_p, result_sym_p);

#if !defined(ST_OSLINUX)
    /* Ececute Leak test */
    RetErr |= stmerge_LeakTest(pars_p, result_sym_p);
#endif /* ST_OSLINUX */

    /* Execute ByPass Test */
    RetErr |= stmerge_ByPass(pars_p, result_sym_p);

    /* Execute Normal Test */
    RetErr |= stmerge_NormalTest(pars_p, result_sym_p);

    STTBX_Print(("============== Runall Exit  =======================\n"));

    return( RetErr );

}/* stmerge_Runall */

void Injection_Status(STMERGE_TaskParams_t *Params_p)
{
    ST_ErrorCode_t    Error = ST_NO_ERROR;
    ST_ErrorCode_t    StoredError = ST_NO_ERROR;
    U16 PacketCountAtPTI[3] = {0};
    U16 slotcount[3] = {0};
    U16 i;

    while (Term_Injection_Status == FALSE)
    {
        STTBX_Print(("\n**************************************"));
        for(i=0;i<Params_p->NoInterfacesSet;i++)
        {
              Error = STPTI_GetInputPacketCount (Params_p->DeviceName[i], &PacketCountAtPTI[i]);
              stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
              STTBX_Print(("\nPacket count from %s          = %d",Params_p->InputInterface[i],PacketCountAtPTI[i]));

              Error =  STPTI_SlotPacketCount( Params_p->SlotHandle[i], &slotcount[i]);
              stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
              STTBX_Print(("\nSlot count after connection    = %d",slotcount[i]));
       }
              task_delay(ST_GetClocksPerSecond()/10);

    }
    STTBX_Print(("\n"));

    for(i=0;i<Params_p->NoInterfacesSet;i++)
    {
        if ((PacketCountAtPTI[i] == 0) || (slotcount[i] == 0))
        {
            break;
        }
        else
        {
        }
    }
    STTBX_Print(("\n**************************************\n"));

    /* signal semaphore here */
    semaphore_signal(Params_p->InjectTerm_p);

}/* Injection_Status */

#ifdef ST_OSLINUX
#define BUF_SIZE 376
/****************************************************************************
Name         : Inject_Stream()

Description  : Injection Task

Parameters   :

Return Value : BOOL

See Also     : Test plan
****************************************************************************/
void* Inject_Stream(void* Data)
{
    /* Play Stream through SWTS 0*/
    FILE* Infile;
    long  Size;
    U8 TpBuffer[BUF_SIZE];
    int Injection;

    /* Define the filename */
    static U8 StreamFileName[] = "canal10.trp";
    char* DevId;
    Injection = 0;
    DevId=(char*)Data;
#if 0
    printf(" Devid in the thread (%s)", DevId);
    STTBX_Print(("\nIn the thread"));
#endif

    /* Open the file */
    Infile = fopen( (char*)StreamFileName, "rb");
    if (Infile == NULL)
    {
         STTBX_Print((" <FAIL> Cannot open file %s\n", StreamFileName));
         exit(0);
     }

    InjectionStatus = TRUE;
    do
    {
        Size = fread( TpBuffer, sizeof(U8), BUF_SIZE, Infile );

        /* inject packet */
        if( Size )
            STMERGE_SWTSXInjectBuf( STMERGE_SWTS_0, TpBuffer, Size );

        Injection++;
#if 0
        /* Can be used for debugging */
        STMERGE_GetStatus(STMERGE_SWTS_0,&TestSWTS0_GetStatus);

        STTBX_Print(("\nLock = %x\n",TestSWTS0_GetStatus.Lock));
        STTBX_Print(("DestinationId = %x\n",TestSWTS0_GetStatus.DestinationId));
        STTBX_Print(("RAMOverflow = %x\n",TestSWTS0_GetStatus.RAMOverflow));
        STTBX_Print(("InputOverflow = %x\n",TestSWTS0_GetStatus.InputOverflow));
        Error = STPTI_GetInputPacketCount (DevId,&PacketCount);
        STTBX_Print(("\nPacket count during injecting = %d",PacketCount));
#endif
    }
    while (Injection < 50);                  /* end do{}while */

    InjectionStatus = FALSE;
    STLINUXInjectionDone = TRUE;
    fclose(Infile);
    return NULL;
}

void* Inject_Status(void* Data)
{
    char* DevId;
    DevId=(char*)Data;
    U16 PacketCountAtPTI,SlotCount;
    struct Inject_StatusParams *InjectParams; 
    int idx;
    static int Cnt=0;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    InjectParams = (struct Inject_StatusParams*)Data;

    while(!STLINUXInjectionDone)
    {
       STTBX_Print(("\n**************************************"));
       for(idx=0;idx<InjectParams->NoInterfacesSet;idx++)
       {
            /* Get Packet count At PTI A */
            Error = STPTI_GetInputPacketCount (InjectParams->DeviceName[idx], &PacketCountAtPTI);
            STTBX_Print(("\nPacket count from %s          = %d",InjectParams->InputInterface[idx],PacketCountAtPTI));
     
            Error =  STPTI_SlotPacketCount( InjectParams->SlotHandle[idx], &SlotCount);
            STTBX_Print(("\nSlot count from %s            = %d",InjectParams->InputInterface[idx],SlotCount));
   
            Cnt++;
       }
    task_delay(10);
    }
return NULL;
}
#endif /* ST_OSLINUX*/


/* End of stmergefn.c */
