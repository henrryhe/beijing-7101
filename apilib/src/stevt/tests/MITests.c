/******************************************************************************\
 *
 * File Name   : basic.c
 *
 * Description : Test Multi Instance EH BackCompatibility and New features
 *
 * Author      : Tonino Barone
 *
 * Copyright STMicroelectronics - August 2000
 *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "evt_test.h"
#include "MIDrv.h"

int NotCBCounter = 0;
int RegCBCounter = 0;
int DevNotCBCounter = 0;
int DevRegCBCounter = 0;

/* Defines and Defines -------------------------------------------- */
#define CHECKERR(__msg_err, __exp_err)  if (RepError != __exp_err) { STEVT_Print (__msg_err);  }

#define CHECKPASSED(__msg_failed, __msg_passed, __exp_error)  if (RepError != __exp_error) { NumErrors++; STEVT_Print (__msg_failed); } else { STEVT_Print (__msg_passed);}

#define CHECKNOTIFIED(__msg_failed, __msg_passed, __exp_error, __exp_not, __exp_devnot)  if ( (RepError != __exp_error) | (NotCBCounter != __exp_not) | (DevNotCBCounter != __exp_devnot) ) { NumErrors++; STEVT_Print (__msg_failed);} else { STEVT_Print (__msg_passed);}

#define CHECKREGNOTIFIED(__msg_failed, __msg_passed, __exp_error, __exp_not, __exp_devnot, __exp_reg, __exp_devreg)  if ( (RepError != __exp_error) | (NotCBCounter != __exp_not) | (DevNotCBCounter != __exp_devnot) | (RegCBCounter != __exp_reg) | (DevRegCBCounter != __exp_devreg) ) { NumErrors++; STEVT_Print (__msg_failed);} else { STEVT_Print (__msg_passed);}


/*********************************************************************
Name:         TESTEVT_MultiInstance

Description:  The function tests the new features required by STEVT
              to manage MultiInstance drivers and assure Backward
              Compatibility toward SingoleInstance drivers
Arguments:

ReturnValue:  ST_ERROR_BAD_PARAMETER       One or more of the supplied
                                           parameters is invalid
              ST_ERROR_UNKNOWN_DEVICE      The named still driver has
                                           not been initialized
              ST_ERROR_ALREADY_INITIALISED The named still driver has
                                           already been initialized
              ST_NO_ERROR                  Function succeeds
*********************************************************************/
U32 TESTEVT_MultiInstance()
{
    ST_ErrorCode_t RepError;
    U32     NumErrors =0 ;
    STEVT_InitParams_t EVTInitParam;
    STEVT_TermParams_t EVTTermParam;
    STDRVA_InitParams_t R1_InitParam, R2_InitParam;
    STDRVA_InitParams_t R3_InitParam, R4_InitParam;
    STDRVA_InitParams_t R5_InitParam;
    STDRVA_Handle_t R1_Handle, R2_Handle, R3_Handle, R4_Handle, R5_Handle;
    STDRVB_InitParams_t S1_InitParam, S2_InitParam;
    STDRVB_InitParams_t S3_InitParam, S4_InitParam;
    STDRVB_InitParams_t S5_InitParam;
    STDRVB_Handle_t S1_Handle, S2_Handle, S3_Handle, S4_Handle, S5_Handle;

    /*********************/
    /* EventHandler init */
    /*********************/
    EVTInitParam.EventMaxNum = 4;
    EVTInitParam.ConnectMaxNum = 20;
    EVTInitParam.SubscrMaxNum = 4;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    EVTInitParam.MemoryPartition = system_partition;
#else
    EVTInitParam.MemoryPartition = SystemPartition;
#endif
    EVTInitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    /********************************************/
    /* NewEH init function */
    /********************************************/
    RepError = STEVT_Init("NewEH", &EVTInitParam);
    CHECKERR(( "NewEH STEVT_Init failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    /********************************************/
    /* Registrant R1 (DRVA instance) init/open */
    /********************************************/
    R1_InitParam.DriverMaxNum = 5;
    strncpy((char*)R1_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    RepError = STDRVA_Init ( "R1", &R1_InitParam);
    CHECKERR(( "R1 STDRVA_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVA_Open ( "R1", &R1_Handle);
    CHECKERR(( "R1 STDRVA_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    /********************************************/
    /* Registrant R2 (DRVA instance) init/open */
    /********************************************/
    R2_InitParam.DriverMaxNum = 5;
    strncpy((char*)R2_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    RepError = STDRVA_Init ( "R2", &R2_InitParam);
    CHECKERR(( "R2 STDRVA_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVA_Open ( "R2", &R2_Handle);
    CHECKERR(( "R2 STDRVA_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    /********************************************/
    /* Registrant R3 (DRVA instance) init/open */
    /********************************************/
    R3_InitParam.DriverMaxNum = 5;
    strncpy((char*)R3_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    RepError = STDRVA_Init ( "R3", &R3_InitParam);
    CHECKERR(( "R3 STDRVA_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVA_Open ( "R3", &R3_Handle);
    CHECKERR(( "R3 STDRVA_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    /********************************************/
    /* Registrant R4 init/open */
    /********************************************/
    R4_InitParam.DriverMaxNum = 5;
    strncpy((char*)R4_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    RepError = STDRVA_Init ( "R4", &R4_InitParam);
    CHECKERR(( "R4 STDRVA_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVA_Open ( "R4", &R4_Handle);
    CHECKERR(( "R4 STDRVA_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    /********************************************/
    /* Registrant R5 init/open */
    /********************************************/
    R5_InitParam.DriverMaxNum = 5;
    strncpy((char*)R5_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    RepError = STDRVA_Init ( "R5", &R5_InitParam);
    CHECKERR(( "R5 STDRVA_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVA_Open ( "R5", &R5_Handle);
    CHECKERR(( "R5 STDRVA_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    /********************************************/
    /* Subscriber S1 (DRVA instance) init/open */
    /********************************************/
    S1_InitParam.DriverMaxNum = 5;
    strncpy((char*)S1_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    strncpy(S1_InitParam.RegName, "R1", ST_MAX_DEVICE_NAME);
    RepError = STDRVB_Init ( "S1", &S1_InitParam);
    CHECKERR(( "S1 STDRVB_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVB_Open ( "S1", &S1_Handle);
    CHECKERR(( "S1 STDRVB_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    /********************************************/
    /* Subscriber S2 (DRVA instance) init/open */
    /********************************************/
    S2_InitParam.DriverMaxNum = 5;
    strncpy((char*)S2_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    strncpy(S2_InitParam.RegName, "R2", ST_MAX_DEVICE_NAME);
    RepError = STDRVB_Init ( "S2", &S2_InitParam);
    CHECKERR(( "S2 STDRVB_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVB_Open ( "S2", &S2_Handle);
    CHECKERR(( "S2 STDRVB_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    /********************************************/
    /* Subscriber S3 (DRVA instance) init/open */
    /********************************************/
    S3_InitParam.DriverMaxNum = 5;
    strncpy((char*)S3_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    strncpy(S3_InitParam.RegName, "R3", ST_MAX_DEVICE_NAME);
    RepError = STDRVB_Init ( "S3", &S3_InitParam);
    CHECKERR(( "S3 STDRVB_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVB_Open ( "S3", &S3_Handle);
    CHECKERR(( "S3 STDRVB_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    /********************************************/
    /* Subscriber S4 init/open */
    /********************************************/
    S4_InitParam.DriverMaxNum = 5;
    strncpy((char*)S4_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    strncpy(S4_InitParam.RegName, "R4", ST_MAX_DEVICE_NAME);
    RepError = STDRVB_Init ( "S4", &S4_InitParam);
    CHECKERR(( "S4 STDRVB_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVB_Open ( "S4", &S4_Handle);
    CHECKERR(( "S4 STDRVB_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    /********************************************/
    /* Subscriber S5 (DRVA instance) init/open */
    /********************************************/
    S5_InitParam.DriverMaxNum = 5;
    strncpy((char*)S5_InitParam.EHName, "NewEH", ST_MAX_DEVICE_NAME);
    strncpy(S5_InitParam.RegName, "R5", ST_MAX_DEVICE_NAME);
    RepError = STDRVB_Init ( "S5", &S5_InitParam);
    CHECKERR(( "S5 STDRVB_Init failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    RepError = STDRVB_Open ( "S5", &S5_Handle);
    CHECKERR(( "S5 STDRVB_Open failed\nReported error #%X\n", RepError ), ST_NO_ERROR);


    /******************************************************************/
    /* Sequence of RegDev/UnrDev to stress the new EventList structure */
    /******************************************************************/
    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    CHECKPASSED(( "^^^Failed Test Case 1 == RegisterDevice by R1\nNumber of errors #%d\n\n", NumErrors ), ( "^^^Passed Test Case 1 == RegisterDevice by R1\n"), ST_NO_ERROR );


    RepError = STDRVA_EvtHndlOpen ( R2_Handle );
    CHECKERR(( "R2 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R2_Handle );
    CHECKERR(( "R2 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError ), ST_NO_ERROR);
    CHECKPASSED(( "^^^Failed Test Case 2 == RegisterDevice by R2\nNumber of errors #%d\n\n", NumErrors ), ( "^^^Passed Test Case 2 == RegisterDevice by R2\n"), ST_NO_ERROR );


    RepError = STDRVA_EvtHndlOpen ( R3_Handle );
    CHECKERR(( "R3 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R3_Handle );
    CHECKERR(( "R3 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKPASSED(( "^^^Failed Test Case 3 == RegisterDevice by R3\nNumber of errors #%d\n\n", NumErrors ), ( "^^^Passed Test Case 3 == RegisterDevice by R3\n"), ST_NO_ERROR );


    RepError = STDRVA_EvtHndlOpen ( R4_Handle );
    CHECKERR(( "R4 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError ), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R4_Handle );
    CHECKERR(( "R4 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKPASSED(( "^^^Failed Test Case 4 == RegisterDevice by R4\nNumber of errors #%d\n\n", NumErrors ), ( "^^^Passed Test Case 4 == RegisterDevice by R4\n"), ST_NO_ERROR );


    RepError = STDRVA_UnregisterDeviceEvent ( R2_Handle );
    CHECKERR(( "R2 STDRVA_UnregisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKPASSED(( "^^^Failed Test Case 5 == UnregisterDevice by R2\nNumber of errors #%d\n\n", NumErrors ), ( "^^^Passed Test Case 5 == UnregisterDevice by R2\n"), ST_NO_ERROR );


    RepError = STDRVA_UnregisterDeviceEvent ( R3_Handle );
    CHECKERR(( "R3 STDRVA_UnregisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKPASSED(( "^^^Failed Test Case 6 == UnregisterDevice by R3\nNumber of errors #%d\n\n", NumErrors ), ( "^^^Passed Test Case 6 == UnregisterDevice by R3\n"), ST_NO_ERROR );


    RepError = STDRVA_UnregisterDeviceEvent ( R4_Handle );
    CHECKERR(( "R4 STDRVA_UnregisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKPASSED(( "^^^Failed Test Case 7 == UnregisterDevice by R4\nNumber of errors #%d\n\n", NumErrors ), ( "^^^Passed Test Case 7 == UnregisterDevice by R4\n"), ST_NO_ERROR );


    RepError = STDRVA_UnregisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_UnregisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKPASSED(( "^^^Failed Test Case 8 == UnregisterDevice by R1\nNumber of errors #%d\n\n", NumErrors ), ( "^^^Passed Test Case 8 == UnregisterDevice by R1\n"), ST_NO_ERROR );


    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_EvtHndlClose ( R2_Handle );
    CHECKERR(( "R2 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_EvtHndlClose ( R3_Handle );
    CHECKERR(( "R3 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_EvtHndlClose ( R4_Handle );
    CHECKERR(( "R4 STDRVA_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);



    /***************************************************/
    /* UnregisterDevice functionality test */
    /***************************************************/
    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_UnregisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_UnregisterDeviceEvent failed\nReported error #%X\n", RepError), STEVT_ERROR_INVALID_EVENT_NAME);
    CHECKPASSED(( "^^^Failed Test Case 9 == Unregister an unsubscribed and unregistered event\nNumber of errors #%d\n", NumErrors ), ( "^^^Passed Test Case 9 == Unregister an unsubscribed and unregistered event\n"), STEVT_ERROR_INVALID_EVENT_NAME );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);


    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVB_SubscribeDeviceEvent ( S1_Handle );
    CHECKERR(( "S1 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_UnregisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_UnregisterDeviceEvent failed\nReported error #%X\n", RepError), ST_ERROR_INVALID_HANDLE);
    CHECKPASSED(( "^^^Failed Test Case 10 == Unregister a subscribed but unregistered event\nNumber of errors #%d\n", NumErrors ), ( "^^^Passed Test Case 10 == Unregister a subscribed but unregistered event\n"), ST_ERROR_INVALID_HANDLE );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);


    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_SubscribeDeviceEvent ( S1_Handle );
    CHECKERR(( "S1 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_UnregisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_UnregisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_NotifyEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_NotifyEvent failed\nReported error #%X\n", RepError), STEVT_ERROR_INVALID_EVENT_ID);
    CHECKPASSED(( "^^^Failed Test Case 11 == Unregister a subscribed and registered event\nNumber of errors #%d\n", NumErrors ), ( "^^^Passed Test Case 11 == Unregister a subscribed and registered event\n"), STEVT_ERROR_INVALID_EVENT_ID );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);



    /***************************************************/
    /* UnsubscribeDevice functionality test            */
    /***************************************************/
    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVB_SubscribeDeviceEvent ( S1_Handle );
    CHECKERR(( "S1 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_UnsubscribeDeviceEvent ( S1_Handle, "R1" );
    CHECKERR(( "S1 STDRVB_UnsubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_NotifyEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_NotifyEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKPASSED(( "^^^Failed Test Case 12 == Unsubscribe a subscribed and registered event\nNumber of errors #%d\n", NumErrors ), ( "^^^Passed Test Case 12 == Unsubscribe a subscribed and registered event\n"), ST_NO_ERROR );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);


    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_UnsubscribeDeviceEvent ( S1_Handle, "R1" );
    CHECKERR(( "S1 STDRVB_UnsubscribeDeviceEvent failed\nReported error #%X\n", RepError), STEVT_ERROR_NOT_SUBSCRIBED);
    CHECKPASSED(( "^^^Failed Test Case 13 == Unsubscribe an unsubscribed but registered event\nNumber of errors #%d\n", NumErrors ), ( "^^^Passed Test Case 13 == Unsubscribe an unsubscribed but registered event\n"), STEVT_ERROR_NOT_SUBSCRIBED);

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);



    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVB_UnsubscribeDeviceEvent ( S1_Handle, "R1" );
    CHECKERR(( "S1 STDRVB_UnsubscribeDeviceEvent failed\nReported error #%X\n", RepError), STEVT_ERROR_INVALID_EVENT_NAME);
    CHECKPASSED(( "^^^Failed Test Case 14 == Unsubscribe an unsubscribed and unregistered event\nNumber of errors #%d\n", NumErrors ), ( "^^^Passed Test Case 14 == Unsubscribe an unsubscribed and unregistered event\n"), STEVT_ERROR_INVALID_EVENT_NAME );

    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);



    /***************************************************/
    /* Multiple RegisterDevice */
    /***************************************************/
    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), STEVT_ERROR_ALREADY_REGISTERED);
    CHECKPASSED(( "^^^Failed Test Case 15 == Multiple RegisterDevice of the same event\nNumber of errors #%d\n", NumErrors ), ( "^^^Passed Test Case 15 == Multiple RegisterDevice of the same event\n"), STEVT_ERROR_ALREADY_REGISTERED );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);


    /***************************************************/
    /* Multiple SubscribeDevice */
    /***************************************************/
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVB_SubscribeDeviceEvent (S1_Handle );
    CHECKERR(( "S1 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_SubscribeDeviceEvent (S1_Handle );
    CHECKERR(( "S1 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), STEVT_ERROR_ALREADY_SUBSCRIBED);
    CHECKPASSED(( "^^^Failed Test Case 16 == Multiple SubscribeDevice to the same event\nNumber of errors #%d\n", NumErrors ), ( "^^^Passed Test Case 16 == Multiple SubscribeDevice to the same event\n"), STEVT_ERROR_ALREADY_SUBSCRIBED );

    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);


    /***************************************************/
    /* Notify funtionality test */
    /***************************************************/
    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S2_Handle );
    CHECKERR(( "S2 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_SubscribeDeviceEvent (S2_Handle );
    CHECKERR(( "S2 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_NotifyEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_NotifyEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKNOTIFIED(( "^^^Failed Test Case 17 == Notify by a registrant not required by any subscriber\nNumber of errors #%d\nRepError = %x\nNotCBCounter = %d\nDevNotCBCounter = %d\n", NumErrors, RepError, NotCBCounter, DevNotCBCounter ), ( "^^^Passed Test Case 17 == Notify by a registrant not required by any subscriber\n"), ST_NO_ERROR, 0, 0 );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S2_Handle );
    CHECKERR(( "S2 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);


    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_SubscribeDeviceEvent (S1_Handle );
    CHECKERR(( "S2 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_NotifyEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_NotifyEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKNOTIFIED(( "^^^Failed Test Case 18 == Notify by a registrant required by a subscriber\nNumber of errors #%d\nRepError = %x\nNotCBCounter = %d\nDevNotCBCounter = %d\n", NumErrors, RepError, NotCBCounter, DevNotCBCounter ), ( "^^^Passed Test Case 18 == Notify by a registrant required by a subscriber\n"), ST_NO_ERROR, 0, 1 );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);


    /***************************************************/
    /* NotifySubscribe funtionality test */
    /***************************************************/
    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_SubscribeDeviceEvent (S1_Handle );
    CHECKERR(( "S2 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_NotifySubscriberEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_NotifySubscriberEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKNOTIFIED(( "^^^Failed Test Case 19 == NotifySubscriber with a valid SubscriberID\nNumber of errors #%d\nRepError = %x\nNotCBCounter = %d\nDevNotCBCounter = %d", NumErrors, RepError, NotCBCounter, DevNotCBCounter ), ( "^^^Passed Test Case 19 == NotifySubscriber with a valid SubscriberID\n"), ST_NO_ERROR,0 ,1 );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);



    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_SubscribeDeviceEvent (S1_Handle );
    CHECKERR(( "S2 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_NotifySubscriberEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_NotifySubscriberEvent failed\nReported error #%X\n", RepError), STEVT_ERROR_INVALID_SUBSCRIBER_ID);
    CHECKNOTIFIED(( "^^^Failed Test Case 20 == NotifySubscriber with an invalid SubscriberID\nNumber of errors #%d\nRepError = %x\nNotCBCounter = %d\nDevNotCBCounter = %d\n", NumErrors, RepError, NotCBCounter, DevNotCBCounter ), ( "^^^Passed Test Case 20 == NotifySubscriber with an invalid SubscriberID\n"), STEVT_ERROR_INVALID_SUBSCRIBER_ID, 0, 0 );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n\n", RepError),  ST_NO_ERROR);



    /***************************************************/
    /* BackCompatibility test */
    /***************************************************/
    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_EvtHndlOpen ( R2_Handle );
    CHECKERR(( "R2 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S2_Handle );
    CHECKERR(( "S2 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_SubscribeEvent (S1_Handle );
    CHECKERR(( "S1 STDRVB_SubscribeEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_SubscribeDeviceEvent (S2_Handle );
    CHECKERR(( "S2 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_RegisterEvent ( R2_Handle );
    CHECKERR(( "R2 STDRVA_RegisterEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKREGNOTIFIED(( "^^^Failed Test Case 21 == BComp. Multi/Single Instance Registration\nNumber of errors #%d\nRepError = %x\nNotCBCounter = %d\nDevNotCBCounter = %d\nRegCBCounter = %d\nDevRegCBCounter = %d\n", NumErrors, RepError, NotCBCounter, DevNotCBCounter, RegCBCounter, DevRegCBCounter ), ( "^^^Passed Test Case 21 == BComp. Multi/Single Instance Registration\n"), ST_NO_ERROR, 0, 0, 0, 0 );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_EvtHndlClose ( R2_Handle );
    CHECKERR(( "R2 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S2_Handle );
    CHECKERR(( "S2 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);


    RepError = STDRVA_EvtHndlOpen ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVB_SubscribeEvent (S1_Handle );
    CHECKERR(( "S1 STDRVB_SubscribeEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_SubscribeDeviceEvent (S1_Handle );
    CHECKERR(( "S2 STDRVB_SubscribeDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_RegisterEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_RegisterEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_NotifyEvent ( R1_Handle );
    CHECKERR(( "R1 STDRVA_NotifyEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKREGNOTIFIED(( "^^^Failed Test Case 22 == BComp. SI Register and MI Subscribe\nNumber of errors #%d\nRepError = %x\nNotCBCounter = %d\nDevNotCBCounter = %d\nRegCBCounter = %d\nDevRegCBCounter = %d\n", NumErrors, RepError, NotCBCounter, DevNotCBCounter, RegCBCounter, DevRegCBCounter ), ( "^^^Passed Test Case 22 == BComp. SI Register and MI Subscribe\n"), ST_NO_ERROR, 1, 0, 0, 0 );

    RepError = STDRVA_EvtHndlClose ( R1_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S1_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);


    RepError = STDRVA_EvtHndlOpen ( R3_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S4_Handle );
    CHECKERR(( "S1 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlOpen ( S5_Handle );
    CHECKERR(( "S2 STDRVB_EvtHndlOpen failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVB_SubscribeEvent (S4_Handle );
    CHECKERR(( "S1 STDRVB_SubscribeEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVB_SubscribeEvent (S5_Handle );
    CHECKERR(( "S2 STDRVB_SubscribeEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    RepError = STDRVA_RegisterDeviceEvent ( R3_Handle );
    CHECKERR(( "R1 STDRVA_RegisterDeviceEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVA_NotifyEvent ( R3_Handle );
    CHECKERR(( "R1 STDRVA_NotifyEvent failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    CHECKREGNOTIFIED(( "^^^Failed Test Case 23 == BComp. SI Subscribe and MI Register\nNumber of errors #%d\nRepError = %x\nNotCBCounter = %d\nDevNotCBCounter = %d\nRegCBCounter = %d\nDevRegCBCounter = %d\n", NumErrors, RepError, NotCBCounter, DevNotCBCounter, RegCBCounter, DevRegCBCounter ), ( "^^^Passed Test Case 23 == BComp. SI Subscribe and MI Register\n"), ST_NO_ERROR, 2, 0, 0, 0 );

    RepError = STDRVA_EvtHndlClose ( R3_Handle );
    CHECKERR(( "R1 STDRVA_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S4_Handle );
    CHECKERR(( "S4 STDRVB_EvtHndlClose failed\nReported error #%X\n", RepError), ST_NO_ERROR);
    RepError = STDRVB_EvtHndlClose ( S5_Handle );
    CHECKERR(( "S5 STDRVB_EvtHndlClose failed\nReported error #%X\n\n", RepError), ST_NO_ERROR);

    /********************************************/
    /* NewEH init function */
    /********************************************/
    RepError = STEVT_Term("NewEH", &EVTTermParam);
    CHECKERR(( "NewEH STEVT_Term failed\nReported error #%X\n", RepError), ST_NO_ERROR);

    return NumErrors;
}
