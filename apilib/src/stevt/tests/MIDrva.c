/******************************************************************************\
 *
 * File Name   : MIDrva.c
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

STDRVA_Driver_t STDRVA_DriverArray[STDRVA_MAX_DRIVER];

static STDRVA_Driver_t *GetDriverByName(const ST_DeviceName_t DriverName);

extern int NotCBCounter;
extern int RegCBCounter;
extern int DevNotCBCounter;
extern int DevRegCBCounter;


/*********************************************************************
Name:         STDRVA_Init

Description:  Initializes a STDRVA driver and gives it the name DeviceName

Arguments:    DeviceName: IN  The name of the STDRVA driver  
              InitParams: IN  parameters to initialize the STDRVA driver

ReturnValue:  ST_ERROR_BAD_PARAMETER       One or more of the supplied
                                           parameters is invalid
              ST_ERROR_ALREADY_INITIALISED The named driver has 
                                           already been initialized
              ST_NO_ERROR                  Function succeeds 
              ST_ERROR_NO_MEMORY
*********************************************************************/
ST_ErrorCode_t STDRVA_Init ( ST_DeviceName_t DriverName, 
                             STDRVA_InitParams_t *InitParams_p)
{
    static BOOL FirstTime = TRUE;
    STDRVA_Driver_t *Driver_p, *FreeDriver_p;
    U32 Driver;

 
    /******************************/
    /* Check InitParams coherency */
    /******************************/
    if ((InitParams_p->DriverMaxNum > STDRVA_MAX_DRIVER ) ||
        (InitParams_p->DriverMaxNum == 0))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if (FirstTime)
    {
        FirstTime = FALSE;
        for (Driver_p = STDRVA_DriverArray, Driver= 0; Driver < STDRVA_MAX_DRIVER; Driver_p++, Driver++)
        {
            Driver_p->DriverName[0] = '\0';
            Driver_p->Initialized = FALSE;
            Driver_p->Handle = STDRVA_INVALID_HANDLE;
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
    /* look for a free driver in STDRVA_DriverArray */
    /************************************************/
    FreeDriver_p = NULL;
    Driver_p = STDRVA_DriverArray;
    for (Driver = 0; Driver < STDRVA_MAX_DRIVER; Driver_p++, Driver++)
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
    FreeDriver_p->Flag = STDRVA_INVALID_HANDLE;

    return (ST_NO_ERROR);
} 


/*********************************************************************
Name:         STDRVA_EvtHndlClose

Description:  Closes the open connection with the STDRVA driver 
              referenced by the handle

Arguments:    Handle:   IN Handle of the STDRVA driver

ReturnValue:  ST_ERROR_INVALID_HANDLE  The handle is invalid
              ST_NO_ERROR              Function succeeds 
********************************************************************/
ST_ErrorCode_t STDRVA_EvtHndlClose ( STDRVA_Handle_t Handle)
{
    STDRVA_Driver_t *Driver_p;
    ST_ErrorCode_t ErrorCode;
    STEVT_Handle_t EHHandle;
 
 
    Driver_p = (STDRVA_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVA_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    EHHandle = (STEVT_Handle_t ) Driver_p->EHHandle;

    ErrorCode = STEVT_Close ( EHHandle);  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVA STEVT_Close failed") );     */
       return ErrorCode;
    }
 
    return(ErrorCode);
}



/*********************************************************************
Name:         STDRVA_Open ()

Description:  Open a STDRVA driver and gives back the Handle

Arguments:    DriverName: IN  The name of the DRVA driver to be opened  
              Handle:     OUT Handle on open connection to the DRVA driver

ReturnValue:  ST_ERROR_UNKNOWN_DEVICE            The named driver has 
                                                 not been initialized
              ST_NO_ERROR                        Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVA_Open ( ST_DeviceName_t     DriverName, 
                             STDRVA_Handle_t     *Handle_p)
{
    STDRVA_Driver_t *Driver_p;
    
 
    Driver_p = GetDriverByName ( DriverName );

    /* DriverName  not initialised */
    if (! Driver_p)
    {
        *Handle_p = STDRVA_INVALID_HANDLE;
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    *Handle_p = (STDRVA_Handle_t)Driver_p;
    Driver_p->Flag = STDRVA_VALID_HANDLE;

    return (ST_NO_ERROR);
}


ST_ErrorCode_t STDRVA_EvtHndlOpen ( ST_Handle_t Handle )
{
    ST_ErrorCode_t ErrorCode;

    STDRVA_Driver_t *Driver_p;
    STEVT_OpenParams_t OpenParams;
    ST_DeviceName_t EHName;
    STEVT_Handle_t EHHandle;
    

    NotCBCounter = 0;
    RegCBCounter = 0;
    DevNotCBCounter = 0;
    DevRegCBCounter = 0;

    Driver_p = (STDRVA_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVA_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }

    /* the STDRVA open the Event Handler and preserve the EHHandle */ 
    strncpy(EHName, Driver_p->EHName, ST_MAX_DEVICE_NAME);
    
    ErrorCode = STEVT_Open ( EHName, &OpenParams, &EHHandle);  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVA STEVT_Open failed") );     */
       return ErrorCode;
    }
    Driver_p->EHHandle = EHHandle;
 
    return (ST_NO_ERROR);
}


/*********************************************************************
Name:         STDRVA_UnregisterDeviceEvent ()

Description:  Provide unregisterDevice functionality
              
Arguments:    Handle:     IN Handle on open connection to the driver

ReturnValue:  ST_INVALID_HANDLE                  Handle is not valid 
              ST_NO_ERROR                        Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVA_UnregisterDeviceEvent ( STDRVA_Handle_t Handle )  
{

    ST_ErrorCode_t ErrorCode;
    ST_DeviceName_t RegName;
    STDRVA_Driver_t *Driver_p;
    STEVT_Handle_t EHHandle;

    Driver_p = (STDRVA_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVA_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }

    EHHandle = ( STEVT_Handle_t ) Driver_p->EHHandle;
    strncpy(RegName, Driver_p->DriverName, ST_MAX_DEVICE_NAME);

    ErrorCode = STEVT_UnregisterDeviceEvent ( EHHandle, 
                                              RegName,
                                              STDRVA_EVENTA );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVA STEVT_UnregisterDeviceEvent failed") );     */
       return ErrorCode;
    }
    Driver_p->Event = INVALID_EVENT;

    return (ST_NO_ERROR);
}


/*********************************************************************
Name:         STDRVA_RegisterEvent ()

Description:  Provide RegisterEvent functionality

Arguments:    Handle:     IN Handle on open connection to the driver

ReturnValue:  ST_INVALID_HANDLE                  Handle is not valid 
              ST_NO_ERROR                        Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVA_RegisterEvent ( STDRVA_Handle_t Handle )  
{

    ST_ErrorCode_t ErrorCode;
    STDRVA_Driver_t *Driver_p;
    STEVT_Handle_t EHHandle;
    STEVT_EventID_t Event1ID;  


    /* ACTUNG: NO CHECK ON THE HANDLE????? */
    Driver_p = (STDRVA_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVA_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    EHHandle = ( STEVT_Handle_t ) Driver_p->EHHandle;

    ErrorCode = STEVT_Register ( EHHandle, STDRVA_EVENTA, &Event1ID );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVA STEVT_Register failed") );     */
       return ErrorCode;
    }
    /* store EventID to be used by the STDRVA for notifying the Event */ 
    Driver_p->Event = Event1ID;

    return (ST_NO_ERROR);
}


/*********************************************************************
Name:         STDRVA_NotifyEvent

Description:  Notify the Event occurrence valid for all Subscriber 
              
Arguments:    EHName: IN  The name of the driver to be opened  

ReturnValue:  ST_INVALID_HANDLE                  Handle is not valid 
              ST_NO_ERROR                        Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVA_NotifyEvent ( STDRVA_Handle_t Handle )
{

    ST_ErrorCode_t ErrorCode;
    STDRVA_Driver_t *Driver_p;
    STEVT_EventID_t EventID;
    STEVT_Handle_t EHHandle;

    
    Driver_p = (STDRVA_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVA_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    EventID = ( STEVT_EventID_t ) Driver_p->Event;
    EHHandle = ( STEVT_Handle_t ) Driver_p->EHHandle;

    ErrorCode = STEVT_Notify   ( EHHandle, EventID, NULL );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVA STEVT_Notify failed") );     */
       return ErrorCode;
    }
 
    return (ST_NO_ERROR);
}


/*********************************************************************
Name:         STDRVA_NotifySubscriberEvent

Description:  Notify the Event occurrence valid only for the named Subscriber 
              
Arguments:    EHName: IN  The name of the still driver to be opened  

ReturnValue:  ST_INVALID_HANDLE                  Handle is not valid 
              ST_NO_ERROR                        Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVA_NotifySubscriberEvent ( STDRVA_Handle_t Handle )
{

    ST_ErrorCode_t ErrorCode;
    STDRVA_Driver_t *Driver_p;
    STEVT_EventID_t EventID;
    STEVT_Handle_t EHHandle;
    STEVT_SubscriberID_t SubscriberID;

    
    /* ACTUNG: NO CHECK ON THE HANDLE????? */
    Driver_p = (STDRVA_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVA_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    EventID = ( STEVT_EventID_t ) Driver_p->Event;
    EHHandle = ( STEVT_Handle_t ) Driver_p->EHHandle;
    SubscriberID = Driver_p->SubscriberID;

    ErrorCode = STEVT_NotifySubscriber   ( EHHandle, EventID, NULL, SubscriberID );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVA STEVT_NotifySubscriber failed") );     */
       return ErrorCode;
    }
 
    return (ST_NO_ERROR);
}


/*********************************************************************
Name:         STDRVA_RegisterDeviceEvent()

Description:  Implement the RegisterdeviceEvent functionality 

Arguments:    Handle: IN Handle on open connection to the still driver

ReturnValue:  ST_INVALID_HANDLE                  Handle is not valid 
              ST_NO_ERROR                        Function succeeds 
*********************************************************************/
ST_ErrorCode_t STDRVA_RegisterDeviceEvent ( STDRVA_Handle_t Handle )  
{

    ST_ErrorCode_t ErrorCode;
    ST_DeviceName_t RegName;
    STDRVA_Driver_t *Driver_p;
    STEVT_Handle_t EHHandle;
    STEVT_EventID_t Event1ID;  


    Driver_p = (STDRVA_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVA_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    EHHandle = ( STEVT_Handle_t ) Driver_p->EHHandle;

    strncpy(RegName, Driver_p->DriverName, ST_MAX_DEVICE_NAME);
    ErrorCode = STEVT_RegisterDeviceEvent ( EHHandle, 
                                            RegName,
                                            STDRVA_EVENTA, 
                                            &Event1ID );  
    if (ErrorCode != ST_NO_ERROR )
    {
/*        STEVT_Report ( ( STTBX_REPORT_LEVEL_ERROR, "DRVA STEVT_RegisterDeviceEvent failed") );     */
       return ErrorCode;
    }
    Driver_p->Event = Event1ID;

    return (ST_NO_ERROR);
}


ST_ErrorCode_t STDRVA_PutSubscriberID  ( STDRVA_Handle_t Handle,
                                         STEVT_SubscriberID_t SubscriberID )  
{
    STDRVA_Driver_t *Driver_p;
    ST_ErrorCode_t ErrorCode;

    Driver_p = (STDRVA_Driver_t *) Handle;
    if (Driver_p->Flag != STDRVA_VALID_HANDLE )
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return ErrorCode;
    }
    Driver_p->SubscriberID = SubscriberID;

    return ST_NO_ERROR;
 
}


/*********************************************************************
Name:         GetDriverByName ()

Description:  Given the field Drivername of a STDRVA_Driver_p structure
              is able to extract it from a STDRVA_ArrayDrivers

*********************************************************************/
static STDRVA_Driver_t *GetDriverByName(const ST_DeviceName_t DriverName)
{
    U32 Tmp;
    STDRVA_Driver_t *WantedDriver_p;
    BOOL Found;
 
    WantedDriver_p = NULL;
 
    for (Tmp= 0, Found = FALSE; (Tmp < STDRVA_MAX_DRIVER) && (!Found); Tmp++)
    {
        if ((! strcmp(STDRVA_DriverArray[Tmp].DriverName, DriverName)) && 
              (STDRVA_DriverArray[Tmp].Initialized == TRUE)   )
        {
            Found = TRUE;
            WantedDriver_p = &(STDRVA_DriverArray[Tmp]);
        }
    }

    return (WantedDriver_p);
}
