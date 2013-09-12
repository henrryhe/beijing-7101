/******************************************************************************\
 
 * File Name   : MIDrvb.c
 *  
 * Description :
 *
 * Author      : Tonino Barone
 *  
 * Copyright STMicroelectronics - August 2000
 *  
\******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MIDrv.h"
#include "evt_test.h"

STDRVB_Driver_t STDRVB_DriverArray[STDRVB_MAX_DRIVER];

static STDRVB_Driver_t *GetDriverByName(const ST_DeviceName_t DriverName);

extern int DevNotCBCounter;
extern int DevRegCBCounter;
extern int NotCBCounter;
extern int RegCBCounter;

void NotifyCallBack ( STEVT_CallReason_t Reason, 
                      STEVT_EventConstant_t Event,
                      const void *EventData)
{
    NotCBCounter++;
}




void DevNotifyCallBack ( STEVT_CallReason_t Reason, 
                         const ST_DeviceName_t RegName,
                         STEVT_EventConstant_t Event,
                         const void *EventData,
                         const void *SubscriberData_p )
{
    DevNotCBCounter++;
}




/*********************************************************************
Name:         STDRVB_Init

Description:  Initializes a STDRVB driver and gives it the name DeviceName

Arguments:    DeviceName: IN  The name of the STDRVB driver  
              InitParams: IN  parameters to initialize the STDRVB driver

ReturnValue:  ST_ERROR_BAD_PARAMETER       One or more of the supplied
                                           parameters is invalid
              ST_ERROR_ALREADY_INITIALISED The named driver has 
                                           already been initialized
              ST_NO_ERROR                  Function succeeds 
              ST_ERROR_NO_MEMORY
*********************************************************************/
ST_ErrorCode_t STDRVB_Init ( ST_DeviceName_t DriverName, 
                             STDRVB_InitParams_t *InitParams_p)
{
    static BOOL FirstTime = TRUE;
    STDRVB_Driver_t *Driver_p, *FreeDriver_p;
    U32 Driver;


    /******************************/
    /* Check InitParams coherency */
    /******************************/
    if ((InitParams_p->DriverMaxNum > STDRVB_MAX_DRIVER ) ||
        (InitParams_p->DriverMaxNum == 0))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if (FirstTime)
    {
        FirstTime = FALSE;
        for (Driver_p = STDRVB_DriverArray, Driver= 0; Driver < STDRVB_MAX_DRIVER; Driver_p++, Driver++)
        {
            Driver_p->DriverName[0] = '\0';
            Driver_p->Initialized = FALSE;
            Driver_p->Handle = STDRVB_INVALID_HANDLE;
        }
    }
  
    /********************************************************************/
    /* Check if a driver called DriverName has been already initialized */
    /********************************************************************/
    if ( GetDriverByName(DriverName) )
    {
        return (ST_ERROR_ALREADY_INITIALIZED);
    }
    
    /************************************************/
    /* look for a free driver in STDRVB_DriverArray */
    /************************************************/
    FreeDriver_p = NULL;
    Driver_p = STDRVB_DriverArray;
    for (Driver = 0; Driver < STDRVB_MAX_DRIVER; Driver_p++, Driver++)
    {
        if (Driver_p->Initialized != TRUE)
        {
            FreeDriver_p = Driver_p;
            break;
        }
    }

    if (FreeDriver_p==NULL)
    {
        return (ST_ERROR_NO_MEMORY);
    }
    
    strncpy(FreeDriver_p->DriverName, DriverName, ST_MAX_DEVICE_NAME);
    FreeDriver_p->Initialized = TRUE;
    strncpy(FreeDriver_p->EHName, InitParams_p->EHName, ST_MAX_DEVICE_NAME);
    FreeDriver_p->DriverMaxNum = InitParams_p->DriverMaxNum;
    strncpy(FreeDriver_p->RegName, InitParams_p->RegName, ST_MAX_DEVICE_NAME);
    FreeDriver_p->Flag = STDRVB_INVALID_HANDLE;

    return (ST_NO_ERROR);
} 


/*********************************************************************
Name:         STDRVB_Open ()

Description:  Opens a STDRVB driver and gives back the Handle

Arguments:    DriverName: IN  The name of the driver to be opened  
              Handle:     OUT Handle on open connection to the still driver

ReturnValue:  ST_ERROR_UNKNOWN_DEVICE            The named driver has 
                                                 not been initialized
              ST_NO_ERROR                        Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVB_Open ( ST_DeviceName_t     DriverName, 
                             STDRVB_Handle_t     *Handle_p)
{
    STDRVB_Driver_t *Driver_p;
    
 
    Driver_p = GetDriverByName ( DriverName );
    /* DriverName  not initialised */
    if (! Driver_p)
    {
        *Handle_p = STDRVB_INVALID_HANDLE;
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    *Handle_p = (STDRVB_Handle_t)Driver_p;
    Driver_p->Flag = STDRVB_VALID_HANDLE;

    return ST_NO_ERROR;
}


ST_ErrorCode_t STDRVB_EvtHndlOpen ( STDRVB_Handle_t Handle )
{
    ST_ErrorCode_t ErrorCode;

    STDRVB_Driver_t *Driver_p;
    STEVT_OpenParams_t OpenParams;
    ST_DeviceName_t EHName;
    STEVT_Handle_t EHHandle;
    

    Driver_p = (STDRVB_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVB_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }

    /* the STDRVB open the Event Handler and preserve the EHHandle */ 
    strncpy(EHName, Driver_p->EHName, ST_MAX_DEVICE_NAME);
    
    ErrorCode = STEVT_Open ( EHName, &OpenParams, &EHHandle);  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVB STEVT_Open failed") );     */
       return ErrorCode;
    }
    Driver_p->EHHandle = EHHandle;
 
    return (ST_NO_ERROR);
}


/*********************************************************************
Name:         STDRVB_EvtHndlClose

Description:  Closes the STEVT open connection with the STDRVB driver 
              referenced by the handle

Arguments:    Handle:   IN Handle of the still driver

ReturnValue:  ST_ERROR_INVALID_HANDLE  The handle is invalid
              ST_NO_ERROR              Function succeeds 
********************************************************************/
ST_ErrorCode_t STDRVB_EvtHndlClose ( STDRVB_Handle_t Handle)
{
    STDRVB_Driver_t *Driver_p;
    ST_ErrorCode_t ErrorCode;
    STEVT_Handle_t EHHandle;
 
 
    Driver_p = (STDRVB_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVB_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    EHHandle = ( STEVT_Handle_t ) Driver_p->EHHandle;

    ErrorCode = STEVT_Close ( EHHandle);  
    if (ErrorCode != ST_NO_ERROR )
    {
       STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVB STEVT_Close failed") );    
       return ErrorCode;
    }
 
    return(ErrorCode);
}


/*********************************************************************
Name:         STDRVB_SubscribeEvent ()

Description:  Implement the SubscribeEvent functionality
              
Arguments:    Handle:     OUT Handle on open connection to the still driver

ReturnValue:  ST_ERROR_INVALID_HANDLE    The provided Handle is not valid
              ST_NO_ERROR                Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVB_SubscribeEvent ( STDRVB_Handle_t Handle )  
{

    ST_ErrorCode_t ErrorCode;
    STEVT_Handle_t EHHandle;
    ST_DeviceName_t RegName;
    STDRVA_Handle_t DRVAHandle;  
    STDRVB_Driver_t *Driver_p;
    STEVT_SubscriberID_t SubscriberID;  
    STEVT_SubscribeParams_t SubscribeParams;
    SubscribeParams.NotifyCallback     = (STEVT_CallbackProc_t)NotifyCallBack;
    


    /*****************************************************************/
    /* the EHName is extracted to be used in the STEVT_Open function */
    /*****************************************************************/
    Driver_p = (STDRVB_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVB_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    EHHandle = ( STEVT_Handle_t ) Driver_p->EHHandle;
    strncpy(RegName, Driver_p->RegName, ST_MAX_DEVICE_NAME);

    /************************************************/
    /* the STDRVB subscribe the STDRVA_EVENTA event */ 
    /************************************************/
    ErrorCode = STEVT_Subscribe ( EHHandle, STDRVA_EVENTA, &SubscribeParams);  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVB STEVT_Subscribe failed") );     */

       return ErrorCode;
    }

    /***********************************************/
    /* STDRVB get its SubscriberID given by the EH */ 
    /***********************************************/
    ErrorCode = STEVT_GetSubscriberID ( EHHandle, &SubscriberID );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVB STEVT_GetSubscriberID failed") );  */
       return ErrorCode;
    }

    /***************************************************************************/
    /* STDRVB open STDRVA driver and put its SubscriberID in the STDRVA struct */ 
    /***************************************************************************/
    ErrorCode = STDRVA_Open ( RegName, &DRVAHandle );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVB STDRVA_Open failed") ); */
       return ErrorCode;
    }

    ErrorCode = STDRVA_PutSubscriberID ( DRVAHandle, SubscriberID );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "STDRVA_PutSubscriberID failed") );     */
       return ErrorCode;
    }

    return (ST_NO_ERROR);
}


/*********************************************************************
Name:         STDRVB_SubscribeDeviceEvent ()

Description:  Implement the SubscribeDeviceEvent functionality
              
Arguments:    Handle:  IN Handle on open connection to the driver

ReturnValue:  ST_ERROR_INVALID_HANDLE    The provided Handle is not valid
              ST_NO_ERROR                Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVB_SubscribeDeviceEvent ( STDRVB_Handle_t Handle)  
{

    ST_ErrorCode_t ErrorCode;
    ST_DeviceName_t RegName;
    STDRVA_Handle_t DRVAHandle;  
    STDRVB_Driver_t *Driver_p;
    STEVT_Handle_t EHHandle;
    STEVT_SubscriberID_t SubscriberID;  

    STEVT_DeviceSubscribeParams_t DeviceSubscribeParams;
    DeviceSubscribeParams.NotifyCallback     = (STEVT_DeviceCallbackProc_t)DevNotifyCallBack;
  

    Driver_p = (STDRVB_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVB_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    EHHandle = ( STEVT_Handle_t ) Driver_p->EHHandle;
    strncpy(RegName, Driver_p->RegName, ST_MAX_DEVICE_NAME);

    /********************************************************************/
    /* STDRVB subscribe to the STDRVA_EVENTA event choosing the RegName */ 
    /********************************************************************/
    ErrorCode = STEVT_SubscribeDeviceEvent ( EHHandle, 
                                             RegName,
                                             STDRVA_EVENTA, 
                                             &DeviceSubscribeParams);  
    if (ErrorCode != ST_NO_ERROR )
    {
       return ErrorCode;
    }

    /***********************************************/
    /* STDRVB get its SubscriberID given by the EH */ 
    /***********************************************/
    ErrorCode = STEVT_GetSubscriberID ( EHHandle, &SubscriberID );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVB STEVT_GetSubscriberID failed") );  */
       return ErrorCode;
    }

    /***************************************************************************/
    /* STDRVB open STDRVA driver and put its SubscriberID in the STDRVA struct */ 
    /***************************************************************************/
    ErrorCode = STDRVA_Open ( RegName, &DRVAHandle );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVB STDRVA_Open failed") ); */
       return ErrorCode;
    }

    ErrorCode = STDRVA_PutSubscriberID ( DRVAHandle, SubscriberID );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "STDRVA_PutSubscriberID failed") );     */
       return ErrorCode;
    }

    return (ST_NO_ERROR);
}


/*********************************************************************
Name:         STDRVB_UnsubscribeDeviceEvent ()

Description: The following steps are performed:
              
Arguments:    Handle: IN Handle on open connection to the driver

ReturnValue:  ST_ERROR_INVALID_HANDLE    The provided Handle is not valid
              ST_NO_ERROR                Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVB_UnsubscribeDeviceEvent ( STDRVB_Handle_t Handle, 
                                               ST_DeviceName_t RegName )  
{

    ST_ErrorCode_t ErrorCode;
    STDRVB_Driver_t *Driver_p;
    STEVT_Handle_t EHHandle;



    /*****************************************************************/
    /* ACTUNG: NO CHECK ON THE HANDLE????? */
    /* the EHName is extracted to be used in the STEVT_Open function */
    /*****************************************************************/
    Driver_p = (STDRVB_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVB_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    EHHandle = ( STEVT_Handle_t ) Driver_p->EHHandle;

    /********************************************************************/
    /* STDRVB subscribe to the STDRVA_EVENTA event choosing the RegName */ 
    /********************************************************************/
    ErrorCode = STEVT_UnsubscribeDeviceEvent ( EHHandle, 
                                               RegName,
                                               STDRVA_EVENTA ); 
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVB STEVT_UnsubscribeDeviceEvent failed") );     */
       return ErrorCode;
    }

    return (ST_NO_ERROR);
}



/*********************************************************************
Name:         GetDriverByName ()

Description:  Given the field Drivername of a STDRVB_Driver_p structure
              is able to extract it from a STDRVB_ArrayDrivers

*********************************************************************/
static STDRVB_Driver_t *GetDriverByName(const ST_DeviceName_t DriverName)
{
    U32 Tmp;
    STDRVB_Driver_t *WantedDriver_p;
    BOOL Found;
 
    WantedDriver_p = NULL;
 
    for (Tmp= 0, Found = FALSE; (Tmp < STDRVB_MAX_DRIVER) && (!Found); Tmp++)
    {
        if ((! strcmp(STDRVB_DriverArray[Tmp].DriverName, DriverName)) && 
              (STDRVB_DriverArray[Tmp].Initialized == TRUE)   )
        {
            Found = TRUE;
            WantedDriver_p = &(STDRVB_DriverArray[Tmp]);
        }
    }

    return (WantedDriver_p);
}
