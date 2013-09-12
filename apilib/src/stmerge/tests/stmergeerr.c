/*****************************************************************************
File Name   : stmergeerr.c

Description : API check.

History     :

Copyright (C) 2005 STMicroelectronics

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "inject_swts.h"
#include "stcommon.h"

#if defined(ST_OSLINUX)
extern void* Inject_Stream(void* Data);
extern void* Inject_Status(void* Data);
extern BOOL STLINUXInjectionDone;
#endif

#ifdef STMERGE_USE_TUNER
#include "sttuner.h"
#endif

#ifdef STMERGE_DVBCI_TEST
#include "stpccrd.h"
#endif

#ifdef STMERGE_CHANNEL_CHANGE
#include "lists.h"
#endif

extern BOOL Term_Injection_Status;

/* global includes */

#ifdef STMERGE_USE_TUNER
extern U32 Pid[3];
void NotifyCallback(STEVT_CallReason_t    Reason,
                    const ST_DeviceName_t RegistrantName,
                    STEVT_EventConstant_t Event,
                    const void           *EventData,
                    const void           *SubscriberData_p);
#endif

#ifdef STMERGE_CHANNEL_CHANGE
int  index1 = 0;
int  transponder = 0, channel = 0;
extern STTUNER_Handle_t   TUNER_Handle;
extern ST_ErrorCode_t SelectFrequency( int );
#endif

/* local function defination */
#if defined(POLL_PTI_ERROR_EVENTS)
static void  StptiEvent_CallbackProc(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event,
                                     const void *EventData );
#endif

#if defined(STMERGE_DVBCI_TEST)
#if defined(ST_5100) || defined(ST_7710)
#define DVBCI_EPLD_ADDRESS 0x41900000
#define DVBCI_MAX_HANDLE   2
#endif
#endif

int GetPTIStreamId( U32 SrcId )
{
    switch ( SrcId )
    {
        case STMERGE_TSIN_0:
            return ( STPTI_STREAM_ID_TSIN0 );
        case STMERGE_TSIN_1:
            return ( STPTI_STREAM_ID_TSIN1 );
        case STMERGE_TSIN_2:
            return ( STPTI_STREAM_ID_TSIN2 );
#if defined(ST_5524) || defined(ST_5525)
        case STMERGE_TSIN_3:
            return ( STPTI_STREAM_ID_TSIN3 );
        case STMERGE_TSIN_4:
            return ( STPTI_STREAM_ID_TSIN4 );
#endif
        case STMERGE_SWTS_0:
            return ( STPTI_STREAM_ID_SWTS0 );
#if (NUMBER_OF_SWTS > 1)
        case STMERGE_SWTS_1:
            return ( STPTI_STREAM_ID_SWTS1 );
#endif
#if (NUMBER_OF_SWTS > 2)
        case STMERGE_SWTS_2:
            return ( STPTI_STREAM_ID_SWTS2 );
#endif
#if (NUMBER_OF_SWTS > 3)
        case STMERGE_SWTS_3:
            return ( STPTI_STREAM_ID_SWTS3 );
#endif
        case STMERGE_ALTOUT_0:
            return ( STPTI_STREAM_ID_ALTOUT );
        default:
            return ( STPTI_STREAM_ID_NONE );
    }

}

int IsValidSWTSId(U32 SrcId)
{
    switch(SrcId)
    {
        case STMERGE_SWTS_0:
#if (NUMBER_OF_SWTS > 1)
        case STMERGE_SWTS_1:
#endif

#if (NUMBER_OF_SWTS > 2)
        case STMERGE_SWTS_2:
#endif

#if (NUMBER_OF_SWTS > 3)
        case STMERGE_SWTS_3:
#endif
            return(1);

        default:
            return(0);
   }
}

int IsValidTSINId(U32 SrcId)
{
    switch(SrcId)
    {
        case STMERGE_TSIN_0:
        case STMERGE_TSIN_1:
        case STMERGE_TSIN_2:

#if (NUMBER_OF_TSIN > 3)
        case STMERGE_TSIN_3:
#endif

#if (NUMBER_OF_TSIN > 4)
        case STMERGE_TSIN_4:
#endif
            return(1);

        default:
            return(0);
   }
}

int IsValidPTIId(U32 DestId)
{
    switch(DestId)
    {
        case STMERGE_PTI_0:

#if (NUMBER_OF_PTI > 1)
        case STMERGE_PTI_1:
#endif

             return (1);

        default:
           return 0;
     }
}

#define PTIA_IIF_SYNC_PERIOD  (PTIA_BASE_ADDRESS+0x6020)
#define PTIA_IIF_SYNC_CONFIG  (PTIA_BASE_ADDRESS+0x601C)

ST_ErrorCode_t TSMERGER_IIFSetSyncPeriod(U32 OutputInjectionSize)
{
    /* Set up sync period to work with DTV or DVB streams. 
       Both use 188 byte packets in the test harness
       THIS OVERWRITES THE SETTINGS IN STPTI_INIT()  */

    *((device_word_t *) PTIA_IIF_SYNC_PERIOD) = OutputInjectionSize;
    *((device_word_t *) PTIA_IIF_SYNC_CONFIG) = 1;
     return ST_NO_ERROR;
}
/* Functions --------------------------------------------------------------- */
/****************************************************************************
Name         : stmerge_ErrantTest()

Description  : Uses the STMERGE driver under "errant" conditions.
               Executes the ErrantUse group of test.

Parameters   :

Return Value : none

See Also     : Test plan
****************************************************************************/
BOOL stmerge_ErrantTest(parse_t *pars_p, char *result_sym_p)
{
    STMERGE_ObjectConfig_t      STMERGE_Object,STMERGE_ObjectGetParams;
    STMERGE_ObjectConfig_t      STMERGE_ObjectSWTS;
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    ST_ErrorCode_t              StoredError = ST_NO_ERROR;
    ST_DeviceName_t             STMERGE_Name;
    STMERGE_InitParams_t        STMERGE_InitParams;
    STMERGE_Status_t            STMERGE_Status;
    U32                         SimpleRAMMap[3][2] = { {STMERGE_TSIN_0,768},
                                                       {STMERGE_SWTS_0,768},
                                                       {0,0}
                                                     };
    /* Init params Setting */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = &SimpleRAMMap;
    STMERGE_InitParams.Mode = STMERGE_NORMAL_OPERATION;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;

#if defined(STMERGE_INTERRUPT_SUPPORT)
    STMERGE_InitParams.InterruptNumber   = INTERRUPT_NUM;
    STMERGE_InitParams.InterruptLevel    = INTERRUPT_LEVEL;
#endif

    /* Config Parameter Setting */
    STMERGE_Object.Priority         = STMERGE_PRIORITY_MID;
    STMERGE_Object.SyncLock         = 0;
    STMERGE_Object.SyncDrop         = 0;
    STMERGE_Object.SOPSymbol        = 0x47;
    STMERGE_Object.PacketLength     = STMERGE_PACKET_LENGTH_DVB;
    STMERGE_ObjectSWTS.Priority     = STMERGE_PRIORITY_MID;
    STMERGE_ObjectSWTS.SyncLock     = 0;
    STMERGE_ObjectSWTS.SyncDrop     = 0;
    STMERGE_ObjectSWTS.SOPSymbol    = 0x47;
    STMERGE_ObjectSWTS.PacketLength = STMERGE_PACKET_LENGTH_DVB;

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);


    STTBX_Print(("\n============== ErrantUse test       ======================\n\n"));

#if !defined(ST_OSLINUX)
    /* The tests are not applicable to Linux */
    /* Since we do not have a valid File descriptor in this case */
    /* It returns with error from the file ioctl_lib.c only */
    /* Without Initializing */
    STTBX_Print(("Use driver without initialize test\n"));
    STTBX_Print(("-------------------------------------\n"));
    stmergetst_SetSuccessState(TRUE);

    /* Connect Driver without Initialization */
    Error = STMERGE_Connect(STMERGE_TSIN_0,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_NOT_INITIALIZED);

    /* Term Driver without Initialization */
    strcpy(STMERGE_Name,"MERGE0");
    Error = STMERGE_Term(STMERGE_Name,NULL);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_NOT_INITIALIZED);

#endif

    STMERGE_Object.u.TSIN.SerialNotParallel  = 0;
    STMERGE_Object.u.TSIN.SyncNotAsync       = 0;
    STMERGE_Object.u.TSIN.InvertByteClk      = 0;
    STMERGE_Object.u.TSIN.ByteAlignSOPSymbol = 0;
    STMERGE_Object.u.TSIN.ReplaceSOPSymbol   = 0;
    STMERGE_ObjectSWTS.u.SWTS.Tagging        = STMERGE_TAG_REPLACE_ID;
    STMERGE_ObjectSWTS.u.SWTS.Pace           = STMERGE_PACE_AUTO;
    STMERGE_ObjectSWTS.u.SWTS.Counter.Value  = STMERGE_COUNTER_AUTO_LOAD;
    STMERGE_ObjectSWTS.u.SWTS.Counter.Rate   = STMERGE_COUNTER_NO_INC;

#if !defined(ST_OSLINUX)
    /* The tests are not applicable to Linux */
    /* Since we do not have a valid File descriptor in this case */
    /* It returns with error from the file ioctl_lib.c only */
    /* Without Initializing */

    /* Get default Params without Initialization */
    Error = STMERGE_GetDefaultParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_NOT_INITIALIZED);

    /* Get params without Initialization */
    Error = STMERGE_GetParams(STMERGE_TSIN_0,&STMERGE_ObjectGetParams);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_NOT_INITIALIZED);

    /* Get Status without Initialization */
    Error = STMERGE_GetStatus(STMERGE_TSIN_0,&STMERGE_Status);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_NOT_INITIALIZED);

    /* Test Conclusion */
    stmergetst_TestConclusion("Use Driver without Initialize Test");
#endif /* ST_OSLINUX */
    /* Init Tests */
    STTBX_Print(("Init Tests\n"));
    STTBX_Print(("-------------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* NULL Parameter */
    Error = STMERGE_Init(NULL,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_ERROR_BAD_PARAMETER);

    /* DeviceName too long */
    Error = STMERGE_Init("ABCDEFGHIJKLMNOPQRST",&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_ERROR_BAD_PARAMETER);

    /* Invalid DeviceType */
    STMERGE_InitParams.DeviceType = (STMERGE_Device_t)2; /* STMERGE_DEVICE_2; */
    strcpy(STMERGE_Name,"MERGE0");
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_DEVICE_NOT_SUPPORTED);
    STMERGE_InitParams.DeviceType = STMERGE_DEVICE_1;

    /* Invalid MemoryMap Id */
    SimpleRAMMap[1][0] = 0xaaaaaa; /* Invalid Value */
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_ID);
    SimpleRAMMap[1][0] = STMERGE_SWTS_0;

    /* Invalid Mode */
    STMERGE_InitParams.Mode=(STMERGE_Mode_t)4;
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_MODE);
    STMERGE_InitParams.Mode=STMERGE_NORMAL_OPERATION;

    /* Good Parameter */
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Same Parameter */
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_ERROR_ALREADY_INITIALIZED);

    /* Test Conclusion */
    stmergetst_TestConclusion("Init Test");

    /*------ TEST SETPARAMETERS ------*/
    STTBX_Print(("Set Params Test\n"));
    STTBX_Print(("-----------------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Get & Connect fails before Setparams */
    Error = STMERGE_GetParams(STMERGE_TSIN_0,&STMERGE_ObjectGetParams);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_PARAMETERS_NOT_SET);

    Error = STMERGE_Connect(STMERGE_TSIN_0,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_PARAMETERS_NOT_SET);

    /* Disconnect fails before Setparams */
    Error = STMERGE_Disconnect(STMERGE_TSIN_0,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_PARAMETERS_NOT_SET);
	
    /* NULL ObjectConfig_t structure*/
    Error = STMERGE_SetParams(STMERGE_TSIN_0,NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_ERROR_BAD_PARAMETER);

    /* Invalid ID */
    Error = STMERGE_SetParams(100/*STMERGE_TSIN_100*/,&STMERGE_Object);/*Not present in ST5528 and 5100*/
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_ID);

    /* Invalid Priority */
    STMERGE_Object.Priority = (STMERGE_Priority_t)17;
    Error = STMERGE_SetParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_PRIORITY);
    STMERGE_Object.Priority = STMERGE_PRIORITY_MID;

    /* Invalid SyncLock */
    STMERGE_Object.SyncLock = 17;
    Error = STMERGE_SetParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_SYNCLOCK);
    STMERGE_Object.SyncLock = 0;

    /* Invalid SyncDrop */
    STMERGE_Object.SyncDrop = 17;
    Error = STMERGE_SetParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_SYNCDROP);
    STMERGE_Object.SyncDrop = 0;

    /* Invalid ByteAlignSOP */
    STMERGE_Object.u.TSIN.ByteAlignSOPSymbol = TRUE;

    Error = STMERGE_SetParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_BYTEALIGN);
    STMERGE_Object.u.TSIN.ByteAlignSOPSymbol = 0;

    /* Invalid Tagging */
    STMERGE_ObjectSWTS.u.SWTS.Tagging = (STMERGE_Tagging_t)5;
    Error = STMERGE_SetParams(STMERGE_SWTS_0,&STMERGE_ObjectSWTS);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_TAGGING);
    STMERGE_ObjectSWTS.u.SWTS.Tagging = STMERGE_TAG_REPLACE_ID;

    /* Invalid Pace */
    STMERGE_ObjectSWTS.u.SWTS.Pace = 32769;
    Error = STMERGE_SetParams(STMERGE_SWTS_0,&STMERGE_ObjectSWTS);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_PACE);
    STMERGE_ObjectSWTS.u.SWTS.Pace = STMERGE_PACE_AUTO;

    /* Invalid Value */
    STMERGE_ObjectSWTS.u.SWTS.Counter.Value = 8388609;
    Error = STMERGE_SetParams(STMERGE_SWTS_0,&STMERGE_ObjectSWTS);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_COUNTER_VALUE);
    STMERGE_ObjectSWTS.u.SWTS.Counter.Value = STMERGE_COUNTER_AUTO_LOAD;

    /* Invalid Rate */
    STMERGE_ObjectSWTS.u.SWTS.Counter.Rate = 17;
    Error = STMERGE_SetParams(STMERGE_SWTS_0,&STMERGE_ObjectSWTS);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_COUNTER_RATE);
    STMERGE_ObjectSWTS.u.SWTS.Counter.Rate = STMERGE_COUNTER_NO_INC;

#if !defined(ST_5528) /* since there isn't any trigger bit on 5528 */
    /* Invalid Trigger */
    STMERGE_ObjectSWTS.u.SWTS.Trigger= 17;
    Error = STMERGE_SetParams(STMERGE_SWTS_0,&STMERGE_ObjectSWTS);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_TRIGGER);
    STMERGE_ObjectSWTS.u.SWTS.Trigger= 10;
#endif

    /* Set TSIN 0 and SWTS 0 with good parameters */
    Error = STMERGE_SetParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    Error = STMERGE_SetParams(STMERGE_SWTS_0,&STMERGE_ObjectSWTS);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    stmergetst_TestConclusion("Set Parameter test");

    /*------ TEST GET DEFAULT PARAMETERS ------*/
    STTBX_Print(("GetDefault Params Test\n"));
    STTBX_Print(("----------------------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Invalid object Id */
    Error = STMERGE_GetDefaultParams(10/*STMERGE_TSIN_10*/,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_ID);

    /* Null ObjectConfig structue */
    Error = STMERGE_GetDefaultParams(STMERGE_TSIN_0,NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_ERROR_BAD_PARAMETER);

    /* Good Parameters */
#if defined(STMERGE_DEFAULT_PARAMETERS)
    Error = STMERGE_GetDefaultParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    Error = STMERGE_GetDefaultParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_FEATURE_NOT_SUPPORTED);
#endif

    stmergetst_TestConclusion("Get Default Parameter test");

    /*------ TEST GETPARAMETERS ------*/
    STTBX_Print(("Get Params Test\n"));
    STTBX_Print(("----------------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Invalid Object Id */
    Error = STMERGE_GetParams(10/*STMERGE_TSIN_10*/,&STMERGE_ObjectGetParams);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_ID);

    /* Null ObjectConfig structue */
    Error = STMERGE_GetParams(STMERGE_TSIN_0,NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_ERROR_BAD_PARAMETER);

    /* Good Parameters */
    Error = STMERGE_GetParams(STMERGE_TSIN_0,&STMERGE_ObjectGetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    stmergetst_TestConclusion("Get Params test");

    /*------ TEST GETSTATUS ------*/
    STTBX_Print(("Get Status Test\n"));
    STTBX_Print(("------------------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Invalid Object ID */
    Error = STMERGE_GetStatus(10/*STMERGE_TSIN_10*/,&STMERGE_Status);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_ID);

    /* Null ObjectConfig structure */
    Error = STMERGE_GetStatus(STMERGE_TSIN_0,NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_ERROR_BAD_PARAMETER);

    /* Good Parameters */
    Error = STMERGE_GetStatus(STMERGE_TSIN_0,&STMERGE_Status);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    stmergetst_TestConclusion("Get Status test");

    /*------ TEST CONNECT ------*/
    STTBX_Print(("Connect Test\n"));
    STTBX_Print(("--------------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Invalid Source Id */
    Error = STMERGE_Connect(10/*STMERGE_TSIN_10*/,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_SOURCE_ID);

    /* Invalid Destination Id */
    Error = STMERGE_Connect(STMERGE_TSIN_0,5/*STMERGE_PTI_5*/);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_DESTINATION_ID);

    /* Invalid Connection */
#ifdef ST_5528 /* 1394in is only on 5528 */
    Error = STMERGE_Connect(STMERGE_1394IN_0,STMERGE_1394OUT_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_ILLEGAL_CONNECTION);
#endif

    /* Good parameters */
    Error = STMERGE_Connect(STMERGE_TSIN_0,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    stmergetst_TestConclusion("Connect test");

#if defined(ST_7100) || defined(ST_7109) || defined(ST_5100)

    /*------ TEST DUPLICATE INPUT STREAM ------*/
    STTBX_Print(("Duplicate Input Test\n"));
    STTBX_Print(("--------------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Invalid Source Id */
    Error = STMERGE_DuplicateInput(10/*STMERGE_TSIN_10*/);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_SOURCE_ID);

    /* Good parameters */
    Error = STMERGE_DuplicateInput(STMERGE_TSIN_0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    stmergetst_TestConclusion("Duplicate Input test");

#endif


   /*------ TEST DISCONNECT ------*/
    STTBX_Print(("Disconnect Test\n"));
    STTBX_Print(("--------------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Invalid Source Id */
    Error = STMERGE_Disconnect(10/*STMERGE_TSIN_10*/,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_SOURCE_ID);

    /* Invalid Destination Id */
    Error = STMERGE_Disconnect(STMERGE_TSIN_0,5/*STMERGE_PTI_5*/);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_INVALID_DESTINATION_ID);

    /* Invalid Connection */
#ifdef ST_5528 /* 1394in is only on 5528 */
    Error = STMERGE_Disconnect(STMERGE_1394IN_0,STMERGE_1394OUT_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_ILLEGAL_CONNECTION);
#endif

    /* Good parameters */
    Error = STMERGE_Disconnect(STMERGE_TSIN_0,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    stmergetst_TestConclusion("Disconnect test");

    /* Term test */
    STTBX_Print(("Term Test\n"));
    STTBX_Print(("-----------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Invalid Device name */
    Error= STMERGE_Term("MERGE1",NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_ERROR_UNKNOWN_DEVICE);

    /* good Parameter */
    Error= STMERGE_Term(STMERGE_Name,NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    stmergetst_TestConclusion("Term test");

    /*------ TEST BYPASS ------*/
    STTBX_Print(("ByPass Test\n"));
    STTBX_Print(("-------------\n"));
    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Init In Bypass mode */
    STMERGE_InitParams.Mode = STMERGE_BYPASS_TSIN_TO_PTI_ONLY;
    Error = STMERGE_Init("MERGE0",&STMERGE_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Set Params */
    Error = STMERGE_SetParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_BYPASS_MODE);

    /* Get defaultParams */
    Error = STMERGE_GetDefaultParams(STMERGE_TSIN_0,&STMERGE_Object);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_BYPASS_MODE);

    /* Get params */
    Error = STMERGE_GetParams(STMERGE_TSIN_0,&STMERGE_ObjectGetParams);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_BYPASS_MODE);

    /* Get Status without Initialize */
    Error = STMERGE_GetStatus(STMERGE_TSIN_0,&STMERGE_Status);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_BYPASS_MODE);

    /* Connect */
    Error = STMERGE_Connect(STMERGE_TSIN_0,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_BYPASS_MODE);

    /* Disconnect */   
    Error = STMERGE_Disconnect(STMERGE_TSIN_0,STMERGE_PTI_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_BYPASS_MODE);

#if defined(ST_7100) || defined(ST_7109) || defined(ST_5100)
    /* Duplicate Input Stream */
    Error = STMERGE_DuplicateInput(STMERGE_TSIN_0);
    stmergetst_ErrorReport(&StoredError,Error,STMERGE_ERROR_BYPASS_MODE);
#endif

    /* Term */
    Error = STMERGE_Term("MERGE0",NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    stmergetst_TestConclusion("ByPass test");

    STTBX_Print(("============== ErrantUse Exit       ======================\n"));
    return TRUE;

}/* stmerge_ErrantTest */

#if !defined(ST_OSLINUX)
/****************************************************************************
Name         : stmerge_LeakTest()

Description  : Executes the memory leak test

Parameters   :

Return Value : none

See Also     : Test plan
****************************************************************************/

BOOL stmerge_LeakTest(parse_t * pars_p, char *result_sym_p)
{

    STMERGE_InitParams_t            STMERGE_InitParams;
    partition_status_t              pstatus;
    U32                             PrevMem,AfterMem;
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;

    STTBX_Print(("============== Memory Leak Test     ======================\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);
    if (0 != partition_status(DriverPartition, &pstatus,(partition_status_flags_t)0))
    {

      STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "partition_status FAILED")) ;

    }

   PrevMem = pstatus.partition_status_free;

   /* Init params Setting */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)STMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = STMERGE_DEFAULT_MERGE_MEMORY_MAP;
    STMERGE_InitParams.Mode              = STMERGE_NORMAL_OPERATION;
    STMERGE_InitParams.DriverPartition_p = DriverPartition;
#if defined(STMERGE_INTERRUPT_SUPPORT)
    STMERGE_InitParams.InterruptNumber   = INTERRUPT_NUM;
    STMERGE_InitParams.InterruptLevel    = INTERRUPT_LEVEL;
#endif

    strcpy(STMERGE_Name,"MERGE0");
    Error = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("\nCall to STMERGE_Init falied\n"));
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }

    Error = STMERGE_Term(STMERGE_Name,NULL);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("\nCall to STMERGE_Term falied\n"));
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }

    if (0 != partition_status(DriverPartition, &pstatus,(partition_status_flags_t)0))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "partition_status FAILED")) ;
    }

   AfterMem = pstatus.partition_status_free;

   if (PrevMem == AfterMem)
   {
        STTBX_Print(("No Errors detected\n"));
   }
   else
   {
        stmergetst_SetSuccessState(FALSE);
   }

   stmergetst_TestConclusion("Memory Leak Test");

   STTBX_Print(("============== Memory Leak Exit     ======================\n"));
   return TRUE;

} /* stmerge_LeakTest */

#if defined(ST_7100) || defined(ST_7109)

/****************************************************************************
Name         : stmerge_ConfigTest()

Description  : Checks configurations of new registers on 7100

Parameters   :

Return Value : none

See Also     : Test plan
****************************************************************************/

BOOL stmerge_ConfigTest(parse_t * pars_p, char *result_sym_p)
{

    STMERGE_InitParams_t            STMERGE_InitParams;
    U32                             SimpleRAMMap[5][2] = {
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

    STMERGE_ObjectConfig_t          TestTSIN_SetParams;
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    STMERGE_Status_t                TestTSIN0_GetStatus;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle;
    U16                             packetcountpre  = 0;
    STPTI_TermParams_t              STPTI_TermParams;
    STPTI_Slot_t                    STPTI_Slot[1];
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
    int            i =0;
    S32 LVar;
    U32 SrcId,DestId;
    BOOL RetErr = FALSE;

    STTBX_Print(("\n============== Config test ======================\n"));
    STTBX_Print((" A Stream should be Input at TSIN 0 for this test to execute\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
    }

    SrcId = LVar;

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
    STTBX_Print(("Set and Get Params Test\n"));
    STTBX_Print(("-----------------------\n"));

    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Set Parameters */
    Error = STMERGE_SetParams(SrcId,&TestTSIN_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    SetupDefaultInitparams( &STPTI_InitParams );
    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(DestId);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(DestId);

#ifdef STMERGE_NO_TAG
    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_NOTAGS;
#else
    STPTI_InitParams.StreamID          = GetPTIStreamId(SrcId);
#endif

#if !defined(ST_OSLINUX)
#if defined(STMERGE_DTV_PACKET)
    STPTI_InitParams.TCLoader_p        = STPTI_DTVTCLoader;
#else
    STPTI_InitParams.TCLoader_p        = STPTI_DVBTCLoader;
#endif
#endif /* ST_OSLINUX */

    STPTI_InitParams.SyncLock          = 0;
    STPTI_InitParams.SyncDrop          = 0;

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

    /* Init PTI 0 */
    Error = STPTI_Init( DeviceName[i], &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    /* Open PTI 0 */
    Error = STPTI_Open( DeviceName[i],  &STPTI_OpenParams, &STPTI_Handle );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle, &STPTI_Slot[i], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if defined(STMERGE_USE_TUNER)
    /* Slot set PID for TSIN 0 */
    Error = STPTI_SlotSetPid( STPTI_Slot[i],Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    /* Slot set PID for TSIN 0 */
    Error = STPTI_SlotSetPid( STPTI_Slot[i],TSIN_PID);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Get packet Count */
    Error = STPTI_GetInputPacketCount ( DeviceName[i], &packetcountpre);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet count before connection are %d \n",packetcountpre));

    /* Packet count should be zero before Connect */
    if (packetcountpre != 0)
    {
        stmergetst_SetSuccessState(FALSE);
    }

    /* Connect TSIN0 and PTI0 */
    Error = STMERGE_Connect(SrcId,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Get Status */
    Error = STMERGE_GetStatus(SrcId,&TestTSIN0_GetStatus);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Display the current status */
    if ( Error == ST_NO_ERROR )
    {
        STTBX_Print(("\nLock = %d\n",TestTSIN0_GetStatus.Lock));
        STTBX_Print(("DestinationId = %d\n",TestTSIN0_GetStatus.DestinationId));
        STTBX_Print(("RAMOverflow = %d\n",TestTSIN0_GetStatus.RAMOverflow));
        STTBX_Print(("InputOverflow = %d\n",TestTSIN0_GetStatus.InputOverflow));
        STTBX_Print(("Read ptr = %d\n",TestTSIN0_GetStatus.Read_p));
        STTBX_Print(("Write ptr = %d\n",TestTSIN0_GetStatus.Write_p));
    }

#ifndef STMERGE_USE_TUNER
    /* Term Event Handler */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term (DeviceName[i], &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);

    STTBX_Print(("============== Config test Exit  ======================\n"));
    return TRUE;

} /* stmerge_ConfigTest */

#endif
#endif /* ST_OSLINUX */

/****************************************************************************
Name         : stmerge_NormalTest()

Description  : Executes the Normal test

Parameters   :

Return Value : none

See Also     : Test plan
****************************************************************************/
BOOL stmerge_NormalTest(parse_t * pars_p, char *result_sym_p)
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

    STMERGE_ObjectConfig_t          TestStream_SetParams={0},TestStream_GetParams;
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    STMERGE_Status_t                TestTSIN_GetStatus,TestTSIN_ExpectedStatus;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle;
    U16                             packetcountpre  = 0,packetcountpost = 0;
    U16                             slotcount       = 0;
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
#if !defined(ST_OSLINUX)
    STMERGE_TaskParams_t            Params;
#endif
    int            Injection = 0,i =0;
    S32 LVar;
    U32 SrcId,DestId;
    BOOL RetErr = FALSE;

#if defined(ST_OSLINUX)
#if defined(SWTS_PID)
#undef SWTS_PID
#endif

/* This PID is for Canal10.trp */
#define SWTS_PID 0x3C
#endif

    STTBX_Print(("\n============== NormalUse test ======================\n"));

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
        STTST_TagCurrentLine(pars_p, "expected dest id (from 0 to 1)");
        return FALSE;
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
    TestStream_SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    TestStream_SetParams.SOPSymbol = 0x47;

   if ( IsValidSWTSId(SrcId) )
    {

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
    }
    else
    {
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
    }

    /* Set params and Get Params function checking */
    STTBX_Print(("Set and Get Params Test\n"));
    STTBX_Print(("-----------------------\n"));

    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Set Parameters */
    Error = STMERGE_SetParams(SrcId,&TestStream_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Get Parameters */
    Error = STMERGE_GetParams(SrcId,&TestStream_GetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Used same function with twice values  values */
    CompareWithDefaultValues(&TestStream_SetParams, &TestStream_GetParams,
                             &TestStream_SetParams, &TestStream_GetParams);

    stmergetst_TestConclusion("Set and Get Params Test");

    STTBX_Print(("Get Status and Packet Count test\n"));
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
    STPTI_InitParams.StreamID          = GetPTIStreamId(SrcId);
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

    Error = STPTI_Init( DeviceName[i], &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    Error = STPTI_Open( DeviceName[i],  &STPTI_OpenParams, &STPTI_Handle );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if defined(STMERGE_DTV_PACKET)
    if ( IsValidSWTSId(SrcId) )
    {
         STPTI_SetDiscardParams(DeviceName[i], 2);
         TSMERGER_IIFSetSyncPeriod(140);
    }
#endif

    Error = STPTI_SlotAllocate( STPTI_Handle, &STPTI_Slot[0], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    if ( IsValidSWTSId(SrcId) )
    {
        Error = STPTI_SlotSetPid( STPTI_Slot[0],SWTS_PID);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
    else
    {
#if defined(STMERGE_USE_TUNER)
        Error = STPTI_SlotSetPid( STPTI_Slot[0],Pid[1]);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
        Error = STPTI_SlotSetPid( STPTI_Slot[0],TSIN_PID);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif
    }
	
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
    Error = STPTI_EnableErrorEvent( DeviceName[i], STPTI_EVENT_CC_ERROR_EVT);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Enable receipt of STPTI_EVENT_PACKET_ERROR_EVT event */
    Error = STPTI_EnableErrorEvent( DeviceName[i], STPTI_EVENT_PACKET_ERROR_EVT);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    Packet_Err = 0;
    CC_Err     = 0;
#endif

    /* Get packet Count */
    Error = STPTI_GetInputPacketCount ( DeviceName[i], &packetcountpre);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet count before connection are %d \n",packetcountpre));

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

    if(! IsValidSWTSId(SrcId))
    {
        /* Get Status and compare with Expected Status */
        /* Expected Status */
        TestTSIN_ExpectedStatus.Lock          = 1;
        TestTSIN_ExpectedStatus.DestinationId = DestId;
        TestTSIN_ExpectedStatus.RAMOverflow   = 0;
        TestTSIN_ExpectedStatus.InputOverflow = 0;

        /* Get Status */
        Error = STMERGE_GetStatus(SrcId,&TestTSIN_GetStatus);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        /* Compare it with expected one */
        if ((TestTSIN_ExpectedStatus.Lock          != TestTSIN_GetStatus.Lock)           ||
            (TestTSIN_ExpectedStatus.DestinationId != TestTSIN_GetStatus.DestinationId ) ||
            (TestTSIN_ExpectedStatus.RAMOverflow   != TestTSIN_GetStatus.RAMOverflow)    ||
            (TestTSIN_ExpectedStatus.InputOverflow != TestTSIN_GetStatus.InputOverflow))
        {
            STTBX_Print(("\nCorrect status of stream is not received"));
 
        if ((TestTSIN_ExpectedStatus.Lock          != TestTSIN_GetStatus.Lock))
            STTBX_Print(("\nStream is not locked "));
 
        if  ((TestTSIN_ExpectedStatus.DestinationId != TestTSIN_GetStatus.DestinationId )) 
            STTBX_Print(("\nDestinationId is not correct"));

        if((TestTSIN_ExpectedStatus.RAMOverflow   != TestTSIN_GetStatus.RAMOverflow))    
            STTBX_Print(("\nRam has overflown"));

        if((TestTSIN_ExpectedStatus.InputOverflow != TestTSIN_GetStatus.InputOverflow))
            STTBX_Print(("\nInput Fifo has overflown"));

            
            stmergetst_SetSuccessState(FALSE);
        }
     }

    if ( IsValidSWTSId(SrcId)  )
    {
#if !defined(ST_OSLINUX)
 
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

        strcpy(Params.DeviceName[0],DeviceName[0]);
        strcpy(Params.InputInterface[0],"SWTS0");
        /* Done this change. changed STPTI_Slot[i] to STPTI_Slot[0]. Need to verify this */
        Params.NoInterfacesSet = 1;
        Params.SlotHandle[0] = STPTI_Slot[0];
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
        InjectStream(SrcId);

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
#else
#define PRIORITY_MIN 0
        pthread_attr_t attr;
        pthread_t task_inject_stream,task_inject_status;
        struct sched_param sparam;
        int priority=PRIORITY_MIN;
        int rc;
        U16 PacketCountAtPTI;
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
        if(rc) return FALSE;
       
        InjectParams.NoInterfacesSet=1;
        strcpy(InjectParams.DeviceName[0],DeviceName[0]);
        strcpy(InjectParams.InputInterface[0],"SWTS0");
        InjectParams.SlotHandle[0] = STPTI_Slot[0];
        sparam.sched_priority = 0;
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setschedparam(&attr, &sparam);
        printf(" Creating thread 2 \n");
        rc = pthread_create(&task_inject_status,&attr,Inject_Status,(void*)&InjectParams);
        if(rc) return FALSE;

        pthread_join(task_inject_stream,NULL);
        pthread_join(task_inject_status,NULL);
        STTBX_Print(("\n"));
        STTBX_Print(("\n================ FILE PLAY EXITTING ==================\n")) ;

        /* Get Packet count At PTI A */
        Error = STPTI_GetInputPacketCount (DeviceName[0] , &PacketCountAtPTI);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nPacket count at PTIA         = %d",PacketCountAtPTI));

        Error =  STPTI_SlotPacketCount( STPTI_Slot[0], &slotcount);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        STTBX_Print(("\nSlot count after connection  = %d",slotcount));
       
        /* Set Injection to MAX_INJECTION_TIME */
        Injection = MAX_INJECTION_TIME;
#endif

    }


    while (Injection < MAX_INJECTION_TIME)
    {
        task_delay( ST_GetClocksPerSecond()/10);

        /* Again Get packet count */
        Error = STPTI_GetInputPacketCount ( DeviceName[i], &packetcountpost);
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
    STTBX_Print(("\n**************************************\n"));

/* During testing an observation found that the RAM Overflow interrupt comes again and again even after STMERGE_Term() in certain situations.
   But this was in situations when STPTI_Term() was called before STMERGE_Term().

   The sequence below does not cause a RAM Overflow interrupt after STMERGE_Term(). Tested it more than 10 times */
    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term (DeviceName[i], &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);





#ifndef STMERGE_USE_TUNER
    /* Term Event Handler */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    stmergetst_TestConclusion("Get Status and Packet Count test");
    STTBX_Print(("============== Normal Use Exit     ======================\n"));
    return TRUE;

}/* stmerge_NormalTest */

#ifdef STMERGE_DVBCI_TEST

/****************************************************************************
Name         : stmerge_DVBCITest()

Description  : Executes the Normal test via DVBCI

Parameters   :

Return Value : none

See Also     : Test plan
****************************************************************************/

BOOL stmerge_DVBCITest(parse_t * pars_p, char *result_sym_p)
{

    STMERGE_InitParams_t            STMERGE_InitParams;
    U32                             SimpleRAMMap[5][2] = {
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

    STMERGE_ObjectConfig_t          TestTSIN_SetParams;
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    STMERGE_Status_t                TestTSIN0_GetStatus,TestTSIN0_ExpectedStatus;
    STPCCRD_InitParams_t            STPCCRD_InitParams;
    STPCCRD_OpenParams_t            STPCCRD_OpenParams;
    STPCCRD_Handle_t                STPCCRD_Handle;
    STPCCRD_TermParams_t            STPCCRD_TermParams;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle;
    U16                             packetcountpre  = 0,packetcountpost = 0;
    U16                             slotcount       = 0;
    STPTI_TermParams_t              STPTI_TermParams;
    STPTI_Slot_t                    STPTI_Slot[1];
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
    int                             Injection = 0,i=0;
    S32 LVar;
    U32 SrcId,DestId;
    BOOL RetErr = FALSE;

    STTBX_Print(("\n============== DVBCI test ======================\n"));
    STTBX_Print((" A Stream should be Input at TSIN 0 for this test to execute\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
    }

    SrcId = LVar;

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected dest id (from 0 to 1)");
    }

    DestId = LVar;

    STPCCRD_InitParams.DeviceCode = DVBCI_EPLD_ADDRESS;
    STPCCRD_InitParams.MaxHandle  = DVBCI_MAX_HANDLE;
    STPCCRD_InitParams.DriverPartition_p = DriverPartition;

    /* STPCCRD Initialization */
    Error =  STPCCRD_Init("stpccrd_0",&STPCCRD_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Open the STPCCRD A */
    STPCCRD_OpenParams.ModName = STPCCRD_MOD_A;
    Error = STPCCRD_Open("stpccrd_0", &STPCCRD_OpenParams, &STPCCRD_Handle);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Check that a card is inserted before commencing ?*/
    Error = STPCCRD_ModCheck(STPCCRD_Handle);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* If Module is present, reset it and check for CIS */
    Error = STPCCRD_ModReset(STPCCRD_Handle);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    Error = STPCCRD_ControlTS(STPCCRD_Handle, STPCCRD_DISABLE_TS_PROCESSING);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

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
    Error = STMERGE_SetParams(SrcId,&TestTSIN_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    SetupDefaultInitparams( &STPTI_InitParams );
    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(DestId);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(DestId);

#ifdef STMERGE_NO_TAG
    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_NOTAGS;
#else
    STPTI_InitParams.StreamID          = GetPTIStreamId(SrcId);
#endif

#if !defined(ST_OSLINUX)
    STPTI_InitParams.TCLoader_p = SupportedLoaders();
#endif

    STPTI_InitParams.SyncLock          = 0;
    STPTI_InitParams.SyncDrop          = 0;

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

    /* Init PTI 0 */
    Error = STPTI_Init( DeviceName[i], &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    /* Open PTI 0 */
    Error = STPTI_Open( DeviceName[i],  &STPTI_OpenParams, &STPTI_Handle );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle, &STPTI_Slot[i], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#if defined(STMERGE_USE_TUNER)
    /* Slot set PID for TSIN 0 */
    Error = STPTI_SlotSetPid( STPTI_Slot[i],Pid[1]);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#else
    /* Slot set PID for TSIN 0 */
    Error = STPTI_SlotSetPid( STPTI_Slot[i],TSIN_PID);
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

    Error = STPTI_SlotLinkToCdFifo(STPTI_Slot[i], &STPTI_DMAParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Enable receipt of STPTI_EVENT_CC_ERROR_EVT event */
    Error = STPTI_EnableErrorEvent( DeviceName[i], STPTI_EVENT_CC_ERROR_EVT);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Enable receipt of STPTI_EVENT_PACKET_ERROR_EVT event */
    Error = STPTI_EnableErrorEvent( DeviceName[i], STPTI_EVENT_PACKET_ERROR_EVT);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Get packet Count */
    Error = STPTI_GetInputPacketCount ( DeviceName[i], &packetcountpre);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STTBX_Print(("Packet count before connection are %d \n",packetcountpre));

    /* Packet count should be zero before Connect */
    if (packetcountpre != 0)
    {
        stmergetst_SetSuccessState(FALSE);
    }

    /* Connect TSIN0 and PTI0 */
    Error = STMERGE_Connect(SrcId,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Get Status and compare with Expected Status */
    /* Expected Status */
    TestTSIN0_ExpectedStatus.Lock          = 1;
    TestTSIN0_ExpectedStatus.DestinationId = STMERGE_PTI_0;
    TestTSIN0_ExpectedStatus.RAMOverflow   = 0;
    TestTSIN0_ExpectedStatus.InputOverflow = 0;

    /* Get Status */
    Error = STMERGE_GetStatus(SrcId,&TestTSIN0_GetStatus);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Compare it with expected one */
    if ((TestTSIN0_ExpectedStatus.Lock          != TestTSIN0_GetStatus.Lock)           ||
        (TestTSIN0_ExpectedStatus.DestinationId != TestTSIN0_GetStatus.DestinationId ) ||
        (TestTSIN0_ExpectedStatus.RAMOverflow   != TestTSIN0_GetStatus.RAMOverflow)    ||
        (TestTSIN0_ExpectedStatus.InputOverflow != TestTSIN0_GetStatus.InputOverflow))
    {
        STTBX_Print(("\nCorrect status of stream is not received"));
        stmergetst_SetSuccessState(FALSE);
    }

    while (Injection < MAX_INJECTION_TIME)
    {

        task_delay( ST_GetClocksPerSecond()/10);

        /* Again Get packet count */
        Error = STPTI_GetInputPacketCount ( DeviceName[i], &packetcountpost);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\n**************************************"));
        STTBX_Print(("\nPacket count after connection = %d",packetcountpost));
        if (packetcountpre == packetcountpost)
        {
            stmergetst_SetSuccessState(FALSE);
        }

        Error =  STPTI_SlotPacketCount( STPTI_Slot[i], &slotcount);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        STTBX_Print(("\nSlot count after connection   = %d",slotcount));

        if (slotcount == 0)
        {
            STTBX_Print(("\nSlot count is not increasing"));
            stmergetst_SetSuccessState(FALSE);
        }

       Injection ++;

    }
    STTBX_Print(("\n**************************************\n"));

#ifndef STMERGE_USE_TUNER
    /* Term Event Handler */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term (DeviceName[i], &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);

    /* Term STPCCRD */
    STPCCRD_TermParams.ForceTerminate = TRUE;
    Error = STPCCRD_Term("stpccrd_0",&STPCCRD_TermParams);

    stmergetst_TestConclusion("DVBCI bypass test");
    STTBX_Print(("============== DVBCI Test Exit ======================\n"));
    return TRUE;

}/* stmerge_DVBCITest */

#endif

#ifdef STMERGE_CHANNEL_CHANGE
/****************************************************************************
Name         : stmerge_ChannelChangeTest()

Description  : Executes the Channel change test with TUNER.

Parameters   :

Return Value : none

See Also     : Test plan
****************************************************************************/

BOOL stmerge_ChannelChangeTest(parse_t * pars_p, char *result_sym_p)
{

    STMERGE_InitParams_t            STMERGE_InitParams;
    U32                             SimpleRAMMap[5][2] = {
                                                              {STMERGE_TSIN_0,384},
                                                              {STMERGE_TSIN_1,384},
                                                              {STMERGE_TSIN_2,384},
                                                              {STMERGE_SWTS_0,384},
                                                              {0,0}
                                                             };

    STMERGE_ObjectConfig_t          TestTSIN_SetParams;
    ST_ErrorCode_t                  Error = ST_NO_ERROR;
    ST_ErrorCode_t                  StoredError = ST_NO_ERROR;
    ST_DeviceName_t                 STMERGE_Name;
    STPTI_InitParams_t              STPTI_InitParams;
    STPTI_OpenParams_t              STPTI_OpenParams;
    STPTI_Handle_t                  STPTI_Handle;
    U16                             packetcountpre  = 0,packetcountpost = 0;
    U16                             slotcount       = 0;
    STPTI_TermParams_t              STPTI_TermParams;
    STPTI_Slot_t                    STPTI_Slot[1];
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
    int                             Injection = 0,i=0;
    S32 LVar;
    U32 SrcId,DestId;
    BOOL RetErr = FALSE;

    STTBX_Print(("\n============== Channel Change test ======================\n"));
    STTBX_Print((" A Stream should be Input at TSIN0 for this test to execute\n"));

    stmergetst_ClearTestCount();
    stmergetst_SetSuccessState(TRUE);

    RetErr = STTST_GetInteger(pars_p, STMERGE_IGNORE_ID, &LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected src id (from 0 to 11)");
    }

    SrcId = LVar;

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

    stmergetst_SetSuccessState(TRUE);
    StoredError = ST_NO_ERROR;

    /* Set Parameters */
    Error = STMERGE_SetParams(SrcId,&TestTSIN_SetParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    SetupDefaultInitparams( &STPTI_InitParams );
    STPTI_InitParams.TCDeviceAddress_p = GetBaseAddress(DestId);
    STPTI_InitParams.InterruptNumber   = GetInterruptNumber(DestId);

#ifdef STMERGE_NO_TAG
    STPTI_InitParams.StreamID          = STPTI_STREAM_ID_NOTAGS;
#else
    STPTI_InitParams.StreamID          = GetPTIStreamId(SrcId);
#endif

#if !defined(ST_OSLINUX)
#if defined(STMERGE_DTV_PACKET)
    STPTI_InitParams.TCLoader_p        = STPTI_DTVTCLoader;
#else
    STPTI_InitParams.TCLoader_p        = STPTI_DVBTCLoader;
#endif
#endif /* ST_OSLINUX */

    STPTI_InitParams.SyncLock          = 0;
    STPTI_InitParams.SyncDrop          = 0;

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

    /* Init PTI 0 */
    Error = STPTI_Init( DeviceName[i], &STPTI_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPTI_OpenParams.DriverPartition_p    = DriverPartition;
    STPTI_OpenParams.NonCachedPartition_p = NcachePartition;

    /* Open PTI 0 */
    Error = STPTI_Open( DeviceName[i],  &STPTI_OpenParams, &STPTI_Handle );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Allocate Slot */
    Error = STPTI_SlotAllocate( STPTI_Handle, &STPTI_Slot[i], &STPTI_SlotData );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

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

    /* Connect TSIN0 and PTI0 */
    Error = STMERGE_Connect(SrcId,DestId);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Added for validating 5100 cut2.0 */
    while (LISTS_ChannelSelection[index1] != ENDOFLIST)
    {
        channel     = LISTS_ChannelSelection[index1];
        transponder = LISTS_ChannelList[channel].Transponder;

        STTBX_Print(( "Lock to transponder/channel #%d: %s\n", transponder, LISTS_TransponderList[transponder].InfoString ));
        Error = SelectFrequency( transponder );
        if (Error == ST_NO_ERROR)
        {
            Pid[0] = LISTS_ChannelList[channel].AudioPid;
            Pid[1] = LISTS_ChannelList[channel].VideoPid;
            Pid[2] = LISTS_ChannelList[channel].PcrPid;

            /* Slot set PID for TSIN 0 */
            Error = STPTI_SlotSetPid( STPTI_Slot[0],Pid[1]);
            stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

        }
        else
        {
            STTBX_Print(( "Could not Lock to transponder/channel #%d:\n",transponder));
            index1++;
            continue;
        }

        while (Injection < MAX_INJECTION_TIME)
        {


            task_delay(ST_GetClocksPerSecond()/10);

            /* Again Get packet count */
            Error = STPTI_GetInputPacketCount ( DeviceName[i], &packetcountpost);
            stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

            STTBX_Print(("\n**************************************"));
            STTBX_Print(("\nPacket count after connection = %d",packetcountpost));
            if (packetcountpre == packetcountpost)
            {
                stmergetst_SetSuccessState(FALSE);
            }

            Error =  STPTI_SlotPacketCount( STPTI_Slot[i], &slotcount);
            stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

            STTBX_Print(("\nSlot count after connection   = %d",slotcount));

            if (slotcount == 0)
            {
                STTBX_Print(("\nSlot count is not increasing"));
                stmergetst_SetSuccessState(FALSE);
            }

           Injection ++;

        }
        Injection = 0;
        STTBX_Print(("\n**************************************\n"));
        index1++;

    }

#ifndef STMERGE_USE_TUNER
    /* Term Event Handler */
    Error = EVENT_Term( EVENT_HANDLER_NAME );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
#endif

    /* Term PTI */
    STPTI_TermParams.ForceTermination = TRUE;
    Error = STPTI_Term (DeviceName[i], &STPTI_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Term STMERGE */
    Error = STMERGE_Term(STMERGE_Name,NULL);

    STTBX_Print(("============== Channel change Exit     ======================\n"));
    return TRUE;

}/* stmerge_ChannelChangeTest */

#endif

/******************************************************************************
Name         : CompareWithDefaultValues()

Description  : Compares first and third STMERGE_ObjectConfig_t argument
               structure with second and forth STMERGE_ObjectConfig_t structure

Parameters   :

Return Value : none

*******************************************************************************/

void CompareWithDefaultValues(STMERGE_ObjectConfig_t *DefaultConfig_TSIN,
                              STMERGE_ObjectConfig_t *TestData_TSIN,
                              STMERGE_ObjectConfig_t *DefaultConfig_SWTS,
                              STMERGE_ObjectConfig_t *TestData_SWTS)
{

    /* Compare TSIN */
    if ((DefaultConfig_TSIN->Priority                  != TestData_TSIN->Priority)||
        (DefaultConfig_TSIN->SyncLock                  != TestData_TSIN->SyncLock) ||
        (DefaultConfig_TSIN->SyncDrop                  != TestData_TSIN->SyncDrop) ||
        (DefaultConfig_TSIN->SOPSymbol                 != TestData_TSIN->SOPSymbol) ||
        (DefaultConfig_TSIN->PacketLength              != TestData_TSIN->PacketLength) ||
        (DefaultConfig_TSIN->u.TSIN.SerialNotParallel  != TestData_TSIN->u.TSIN.SerialNotParallel) ||
        (DefaultConfig_TSIN->u.TSIN.SyncNotAsync       != TestData_TSIN->u.TSIN.SyncNotAsync) ||
        (DefaultConfig_TSIN->u.TSIN.InvertByteClk      != TestData_TSIN->u.TSIN.InvertByteClk) ||
        (DefaultConfig_TSIN->u.TSIN.ByteAlignSOPSymbol != TestData_TSIN->u.TSIN.ByteAlignSOPSymbol) ||
        (DefaultConfig_TSIN->u.TSIN.ReplaceSOPSymbol   != TestData_TSIN->u.TSIN.ReplaceSOPSymbol))
    {
        STTBX_Print(("Default TSIN data is not Valid \n"));
        stmergetst_SetSuccessState(FALSE);

    }

    /* Compare SWTS */
    if ((DefaultConfig_SWTS->Priority             != TestData_SWTS->Priority) ||
        (DefaultConfig_SWTS->SyncLock             != TestData_SWTS->SyncLock) ||
        (DefaultConfig_SWTS->SyncDrop             != TestData_SWTS->SyncDrop) ||
        (DefaultConfig_SWTS->SOPSymbol            != TestData_SWTS->SOPSymbol) ||
        (DefaultConfig_SWTS->PacketLength         != TestData_SWTS->PacketLength) ||
        (DefaultConfig_SWTS->u.SWTS.Tagging       != TestData_SWTS->u.SWTS.Tagging) ||
        (DefaultConfig_SWTS->u.SWTS.Pace          != TestData_SWTS->u.SWTS.Pace) ||
        (DefaultConfig_SWTS->u.SWTS.Counter.Value != TestData_SWTS->u.SWTS.Counter.Value) ||
        (DefaultConfig_SWTS->u.SWTS.Counter.Rate  != TestData_SWTS->u.SWTS.Counter.Rate) )
    {
        STTBX_Print(("Default TSIN data is not valid \n"));
        stmergetst_SetSuccessState(FALSE);

    }

}

STPTI_SupportedTCCodes_t SupportedTCCodes (void)
{
#ifdef STMERGE_DTV_PACKET
        return (STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV);
#else
        return (STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB);
#endif
}

#if !defined(ST_OSLINUX)
TCLoader_t SupportedLoaders (void)
{
#ifdef STMERGE_DTV_PACKET
#if defined(ST_5528)
            return (STPTI_DTVTCLoader);
#elif defined(ST_5100) || defined(ST_5301) || defined(ST_5302) || defined(ST_7710) || defined(ST_7100)
            return (STPTI_DTVTCLoaderL);    
#elif defined(ST_5524) || defined(ST_5525) || defined(ST_7101) || defined(ST_7109) || defined(ST_7120) || defined(ST_7200)
            return (STPTI_DTVTCLoaderSL);  
#else
            #error Please supply architecture information!
#endif
#else
#if defined(ST_5528)
            return (STPTI_DVBTCLoader);
#elif defined(ST_5100) || defined(ST_5301) || defined(ST_5302) || defined(ST_7710) || defined(ST_7100)
            return (STPTI_DVBTCLoaderL);    
#elif defined(ST_5524) || defined(ST_5525) || defined(ST_7101) || defined(ST_7109) || defined(ST_7120) || defined(ST_7200)
            return (STPTI_DVBTCLoaderSL);  
#else
            #error Please supply architecture information!
#endif
#endif
}
#endif

/*******************************************************************************
Name         : SetupDefaultInitparams()

Description  : Setup default STPTI Init parameters

Parameters   :

Return Value : none
*******************************************************************************/

void SetupDefaultInitparams( STPTI_InitParams_t *STPTI_InitParams )
{

    STPTI_InitParams->Device                     = STPTI_DEVICE_PTI_4;

#ifdef STMERGE_DTV_PACKET
    STPTI_InitParams->TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV;
#else
    STPTI_InitParams->TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
#endif

    STPTI_InitParams->TCDeviceAddress_p          = GetBaseAddress(STMERGE_PTI_1);
    STPTI_InitParams->SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_8x8;
    STPTI_InitParams->InterruptLevel             = PTI_INTERRUPT_LEVEL;
    STPTI_InitParams->InterruptNumber            = GetInterruptNumber(STMERGE_PTI_1);

#ifdef STMERGE_DTV_PACKET
    STPTI_InitParams->SyncLock                   = 0;
    STPTI_InitParams->SyncDrop                   = 0;
#else
    STPTI_InitParams->SyncLock                   = DEFAULT_SYNC_LOCK;
    STPTI_InitParams->SyncDrop                   = DEFAULT_SYNC_DROP;
#endif

    STPTI_InitParams->NCachePartition_p          = NcachePartition;
    STPTI_InitParams->Partition_p                = DriverPartition;

    STPTI_InitParams->DescramblerAssociation     = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;

    STPTI_InitParams->EventProcessPriority       = MIN_USER_PRIORITY;
    STPTI_InitParams->InterruptProcessPriority   = MAX_USER_PRIORITY;

#ifndef STPTI_NO_INDEX_SUPPORT
    STPTI_InitParams->IndexAssociation           = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTI_InitParams->IndexProcessPriority       = MIN_USER_PRIORITY;
#endif

#ifdef STPTI_CAROUSEL_SUPPORT
    STPTI_InitParams->CarouselProcessPriority    = MIN_USER_PRIORITY;
    STPTI_InitParams->CarouselEntryType          = STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE;
#endif

#ifdef STMERGE_DTV_PACKET
    STPTI_InitParams->DiscardSyncByte            = FALSE; /* when using Packet Injector */
#endif

#ifdef STMERGE_NO_TAG
    STPTI_InitParams->StreamID                   = STPTI_STREAM_ID_NOTAGS;
#else
    STPTI_InitParams->StreamID                   = STPTI_STREAM_ID_SWTS0;
#endif

    STPTI_InitParams->NumberOfSlots              = 10;

#ifdef STMERGE_DTV_PACKET
    STPTI_InitParams->DiscardOnCrcError          = TRUE;
#else
    STPTI_InitParams->DiscardOnCrcError          = FALSE;
#endif
    STPTI_InitParams->PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_PTI;
    STPTI_InitParams->AlternateOutputLatency     = 42;

    memcpy (&(STPTI_InitParams->EventHandlerName), EVENT_HANDLER_NAME, sizeof (EVENT_HANDLER_NAME));

}/* SetupDefaultInitparams */

/******************************************************************************
Function Name : GetBaseAddress

Description   : Pass in an integer 0 or 1 and get the corresponding base
                address for PTIA or B

Parameters    : DeviceNo - integer 0 or 1 meaning pti A or B
******************************************************************************/

STPTI_DevicePtr_t GetBaseAddress(U32 DestinationId)
{
    switch (DestinationId)
    {
        case STMERGE_PTI_0:
            return ((STPTI_DevicePtr_t) PTIA_BASE_ADDRESS);

        case STMERGE_PTI_1:
            return ((STPTI_DevicePtr_t) PTIB_BASE_ADDRESS);

        default:
            return ((STPTI_DevicePtr_t) NULL);    /* Invalid device no */
    }

}/* GetBaseAddress */

/******************************************************************************
Function Name : GetInterruptNumber

Description   : Pass in an integer 0, 1 and get the corresponding pti
                interrupt number for PTIA or B

Parameters    : DeviceNo - integer 0, 1 or meaning pti A or B
******************************************************************************/
S32 GetInterruptNumber (U32 DestinationId)
{
    switch (DestinationId)
    {
        case STMERGE_PTI_0:
            return ((S32) PTIA_INTERRUPT);

       case STMERGE_PTI_1:
            return ((S32) PTIB_INTERRUPT);



       default:
            return 0;       /* Invalid device no */
    }

}/* GetInterruptNumber */
/******************************************************************************
Function Name : EVENT_Setup

Description   : Init EVT handler

Parameters    : ST_DeviceName_t
******************************************************************************/

ST_ErrorCode_t EVENT_Setup(ST_DeviceName_t EventHandlerName)
{
    ST_ErrorCode_t Error;
    STEVT_InitParams_t EVTInitParams;
    STEVT_OpenParams_t EVTOpenParams;
    STEVT_Handle_t     EVTHandle;

#if defined(STMERGE_USE_TUNER)
    STEVT_DeviceSubscribeParams_t STEVT_DeviceSubscribeParams;
#endif

#if defined(POLL_PTI_ERROR_EVENTS)
    STEVT_SubscribeParams_t       STEVT_SubscribeParams;
    int event;
#endif

    /* Initialise Event Handler */
    EVTInitParams.EventMaxNum     = 90;
    EVTInitParams.ConnectMaxNum   = 60;
    EVTInitParams.SubscrMaxNum    = 90;
    EVTInitParams.MemoryPartition = DriverPartition;
    EVTInitParams.MemorySizeFlag  = STEVT_UNKNOWN_SIZE;

    Error = STEVT_Init(EventHandlerName, &EVTInitParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("\nSTEVT_Init FAILED"));
    }

    Error = STEVT_Open(EventHandlerName,&EVTOpenParams,&EVTHandle );
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("\nSTEVT_Open FAILED"));
    }

#if defined(STMERGE_USE_TUNER)

    STEVT_DeviceSubscribeParams.NotifyCallback     = NotifyCallback;
    STEVT_DeviceSubscribeParams.SubscriberData_p   = NULL;

    Error = STEVT_SubscribeDeviceEvent(EVTHandle, "EVT_TUNER", (U32) STTUNER_EV_LOCKED, &STEVT_DeviceSubscribeParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("\nSTEVT_Subscribe(STTUNER_EV_LOCKED)- FAILED" ));
    }

    Error = STEVT_SubscribeDeviceEvent(EVTHandle, "EVT_TUNER", (U32) STTUNER_EV_UNLOCKED, &STEVT_DeviceSubscribeParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("\nSTEVT_Subscribe(STTUNER_EV_UNLOCKED)- FAILED" ));
    }

    Error = STEVT_SubscribeDeviceEvent(EVTHandle, "EVT_TUNER", (U32) STTUNER_EV_TIMEOUT, &STEVT_DeviceSubscribeParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("\nSTEVT_Subscribe(STTUNER_EV_TIMEOUT)- FAILED"));
    }

    Error = STEVT_SubscribeDeviceEvent(EVTHandle, "EVT_TUNER", (U32) STTUNER_EV_SCAN_FAILED, &STEVT_DeviceSubscribeParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("\nSTEVT_Subscribe(STTUNER_EV_SCAN_FAILED)- FAILED" ));

    }

    Error = STEVT_SubscribeDeviceEvent(EVTHandle, "EVT_TUNER", (U32) STTUNER_EV_SIGNAL_CHANGE, &STEVT_DeviceSubscribeParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("\nSTEVT_Subscribe(STTUNER_EV_SIGNAL_CHANGE) - FAILED" ));
    }
#endif

#if defined(POLL_PTI_ERROR_EVENTS)

    /* subscribe to stpti events */
    STEVT_SubscribeParams.NotifyCallback = StptiEvent_CallbackProc;

    for (event = STPTI_EVENT_BASE; event <= (int)STPTI_EVENT_TC_CODE_FAULT_EVT; event++)
    {
        Error = STEVT_Subscribe(EVTHandle, event, &STEVT_SubscribeParams);
        if (Error != ST_NO_ERROR)
        {
           STTBX_Print(("\nSTEVT_Subscribe FAILED"));
        }
    }
#endif

    return Error;

}/* EVENT_Setup */

/******************************************************************************
Function Name : EVENT_Term

Description   : Terminate Event Handler

Parameters    : ST_DeviceName_t
******************************************************************************/
ST_ErrorCode_t EVENT_Term(ST_DeviceName_t EventHandlerName)
{
    ST_ErrorCode_t Error;
    STEVT_TermParams_t EVTTermParams;

    /* Terminate Event Handler */
    EVTTermParams.ForceTerminate = TRUE;
    Error = STEVT_Term(EventHandlerName, &EVTTermParams);

    return Error;

}/* EVENT_Term */

#if defined(POLL_PTI_ERROR_EVENTS)
/*-------------------------------------------------------------------------
 * Function : StptiEvent_CallbackProc
 * Input    : event data
 * Output   :
 * Return   : nothing
 * ----------------------------------------------------------------------*/
static void StptiEvent_CallbackProc(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (Reason == CALL_REASON_NOTIFY_CALL)
    {
        switch(Event)
        {
            /* no reporting on these events */
#ifdef STPTI_CAROUSEL_SUPPORT
            case STPTI_EVENT_CAROUSEL_CYCLE_COMPLETE_EVT:
            case STPTI_EVENT_CAROUSEL_ENTRY_TIMEOUT_EVT:
            case STPTI_EVENT_CAROUSEL_TIMED_ENTRY_EVT:
#endif
#ifdef STPTI_INDEX_SUPPORT
            case STPTI_EVENT_INDEX_MATCH_EVT:
#endif
#ifdef STMERGE_DTV_PACKET
            case STPTI_EVENT_CWP_BLOCK_EVT:
#endif
            case STPTI_EVENT_CLEAR_TO_SCRAMBLED_EVT:
            case STPTI_EVENT_DMA_COMPLETE_EVT:
            case STPTI_EVENT_PCR_RECEIVED_EVT:
            case STPTI_EVENT_SCRAMBLED_TO_CLEAR_EVT:
                break;

            /* general report on these events with error count */
            case STPTI_EVENT_BUFFER_OVERFLOW_EVT:
            case STPTI_EVENT_INTERRUPT_FAIL_EVT:
            case STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT:
            case STPTI_EVENT_INVALID_LINK_EVT:
            case STPTI_EVENT_INVALID_PARAMETER_EVT:
            case STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT:
            case STPTI_EVENT_TC_CODE_FAULT_EVT:
                break;

            case STPTI_EVENT_PACKET_ERROR_EVT:
                STTBX_Print(("\nPTI_EVENT:STPTI_EVENT_PACKET_ERROR_EVT"));
                break;

            case STPTI_EVENT_CC_ERROR_EVT:
                STTBX_Print(("\nPTI_EVENT:STPTI_EVENT_CC_ERROR_EVT"));
                break;

            default:
                break;
        }
    }

    /* Re-enable receipt of STPTI_EVENT_CC_ERROR_EVT event */
    Error = STPTI_EnableErrorEvent( "stpti_0", STPTI_EVENT_CC_ERROR_EVT);

    /* Reenable receipt of STPTI_EVENT_PACKET_ERROR_EVT event */
    Error = STPTI_EnableErrorEvent( "stpti_0", STPTI_EVENT_PACKET_ERROR_EVT);


}/* StptiEvent_CallbackProc */
#endif
/* End of stmergeerr.c */
