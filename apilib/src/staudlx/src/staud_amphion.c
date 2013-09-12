/************************************************************************
COPYRIGHT STMicroelectronics (C) 2004
Source file name : staud.c
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
//#define STTBX_PRINT
#define API_Trace STTBX_Print
#include "sttbx.h"
#include "stos.h"
#include "staud.h"
#include "aud_amphiondecoder.h"
#include "aud_pcmplayer.h"
#include "aud_pes_es_parser.h"
#include "aud_spdifplayer.h"
#include "aud_utils.h"
#include "aud_evt.h"
#ifdef INPUT_PCM_SUPPORT
    #include "pcminput.h"
#endif
#ifdef MIXER_SUPPORT
    #include "aud_mmeaudiomixer.h"
#endif
#include "staudlx_prints.h"

#define STAUD_MAX_OPEN	      1

#define	AVSYNC_ADDED	      1
#define REV_STRING_LEN 	      (256)

#define MIXER_INPUT_FROM_FILE 0

/* Queue of initialized Decoders */

static semaphore_t *InitSemaphore_p = NULL;
/*----------------------------------------------------------------------------
Audio Chain Creations functions
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
Decoder management functions
-----------------------------------------------------------------------------*/
U32	GetClassHandles(STAUD_Handle_t Handle, U32	ObjectClass, STAUD_Object_t *ObjectArray);


/************************************************************************************
Name         : STAUD_Init()

Description  : Initializes the Audio driver. Creates an instance of the audio driver
and initializes the hardware

Parameters   : ST_DeviceName_t      DeviceName      AUDIO driver name
STAUD_InitParams_t   *InitParams     Pointer to the params structure

Return Value :
ST_NO_ERROR                     Success.
ST_ERROR_ALREADY_INITIALIZED    The named Audio Decoder has already been initialized
ST_ERROR_BAD_PARAMETER          One or more of the supplied parameters is invalid.
ST_ERROR_NO_MEMORY              Not enough memory to initialize an Audio Decoder.
STAUD_ERROR_AVSYNC_TASK         An error occurred when trying to start the audio
video synchronization process.
STAUD_ERROR_CLKRV_OPEN          An error occurred when trying to open a connection to
the clock recovery driver
ST_ERROR_INTERRUPT_INSTALL      An error occured when installing interrupt
************************************************************************************/

ST_ErrorCode_t STAUD_Init(const ST_DeviceName_t DeviceName,
                          STAUD_InitParams_t *InitParams_p)
{

    ST_ErrorCode_t	error = ST_NO_ERROR;
    AudDriver_t		*qp_p = NULL;
    API_Trace((">>> STAUD_Init >>> \n "));
    PrintSTAUD_Init(DeviceName, InitParams_p);

    /*****Initializing the MME**********************/
    #ifdef MIXER_SUPPORT
        MME_Init();
    #endif

    STTBX_Print(("STAUD_Init DeviceName=[%s], InitParams_p=[%x]\n", DeviceName, InitParams_p ));

    /*initialize the Driver pointer*/
    STOS_TaskLock();
    if(InitSemaphore_p == NULL)
    {
        InitSemaphore_p = STOS_SemaphoreCreateFifo(NULL, 1);
    }
    STOS_TaskUnlock();

    STOS_SemaphoreWait(InitSemaphore_p);

    /**for checking Number of output channels**/
    if((InitParams_p->NumChannels != 2))
    {
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] InitP, DeviceName, Max, Open \n "));
        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_BAD_PARAMETER);
    }


    if( (InitParams_p				== NULL) ||  /* NULL structure ptr? */
        (*DeviceName				== 0   ) ||  /* Device Name undefined? */
        (InitParams_p->MaxOpen		== 0   ) ||  /* Zero max opens? */
        (strlen( DeviceName )		>=  ST_MAX_DEVICE_NAME ) )

    {
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] Num Channels \n "));
        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_BAD_PARAMETER);
    }

    if((InitParams_p->DeviceType != STAUD_DEVICE_STI5105) && (InitParams_p->DeviceType!= STAUD_DEVICE_STI5107) && (InitParams_p->DeviceType != STAUD_DEVICE_STI5188) && (InitParams_p->DeviceType != STAUD_DEVICE_STI5162))
    {
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] Device Type \n "));
        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_BAD_PARAMETER);
    }

    #ifndef STAUD_REMOVE_CLKRV_SUPPORT
        if((strlen(InitParams_p->ClockRecoveryName) == 0)||(strlen(InitParams_p->ClockRecoveryName) > ST_MAX_DEVICE_NAME))
        {
            API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] Clock Recovery Name \n "));
            STOS_SemaphoreSignal(InitSemaphore_p);
            return (ST_ERROR_BAD_PARAMETER);
        }
    #endif


    if ((InitParams_p->MaxOpen > STAUD_MAX_OPEN) || (InitParams_p->MaxOpen == 0))
    {
        STOS_SemaphoreSignal(InitSemaphore_p);
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] Max Open \n "));
        return (ST_ERROR_BAD_PARAMETER);
    }


    if(AudDri_IsInit(DeviceName))
    {
        STOS_SemaphoreSignal(InitSemaphore_p);
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_ALREADY_INITIALIZED]  \n "));
        return ST_ERROR_ALREADY_INITIALIZED;
    }


    strcpy(InitParams_p->RegistrantDeviceName, DeviceName);
    qp_p = (AudDriver_t *)memory_allocate(InitParams_p->CPUPartition_p, sizeof(AudDriver_t));
    if(!qp_p)
    {
        STOS_SemaphoreSignal(InitSemaphore_p);
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_NO_MEMORY] AUDDriver_t Line = %d \n   ", __LINE__));
        return ST_ERROR_NO_MEMORY;
    }
    memset(qp_p, 0, sizeof(AudDriver_t));
    strcpy(qp_p->Name, DeviceName);
    qp_p->Handle            = (STAUD_DrvChainConstituent_t *)AUDIO_INVALID_HANDLE;
    qp_p->Configuration     = InitParams_p->Configuration;
    qp_p->CPUPartition_p    = InitParams_p->CPUPartition_p;
    qp_p->BufferPartition	= InitParams_p->BufferPartition;
    qp_p->Speed			    = 100;

    /* Create amphion related info*/
    qp_p->AmphionAccess_p   = STOS_SemaphoreCreateFifo(NULL, 1);
    qp_p->FDMABlock         = STFDMA_1;
    error  |= STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL, &(qp_p->AmphionInChannel), qp_p->FDMABlock);
    error  |= STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL, &(qp_p->AmphionOutChannel), qp_p->FDMABlock);

    strcpy(qp_p->EvtHandlerName, InitParams_p->EvtHandlerName);
    AudDri_QueueAdd(qp_p);

    if (error == ST_NO_ERROR)
    {
        #ifndef STAUD_REMOVE_CLKRV_SUPPORT
            error = STCLKRV_Open( InitParams_p->ClockRecoveryName, NULL, &(qp_p->CLKRV_Handle));

            if(error != ST_NO_ERROR)
            {
                STTBX_Print(("STCLKRV_Open Failed: error %x", error));
                /*Remove the driver from queue here*/
                AudDri_QueueRemove(qp_p);
                API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] Clock open Error  \n"));
                STOS_SemaphoreSignal(InitSemaphore_p);
                return ST_ERROR_BAD_PARAMETER;
            }

            /* By default the handle for the synchro is the same as the handle for clock config */
            qp_p->CLKRV_HandleForSynchro = qp_p->CLKRV_Handle;
        #endif
        if(InitParams_p->Configuration <= STAUD_CONFIG_STB_COMPACT)
        {
            error = STAud_CreateDriver(InitParams_p, qp_p);
            /*Register events*/
            if(error == ST_NO_ERROR)
            {
                error = STAud_RegisterEvents(qp_p);
            }
            else
            {
                /*remove all the added elements later*/
                STTBX_Print(("Create Driver Failed: error %x", error));
            }
        }
        else
        {
            API_Trace((" STAUD_Init Error [ST_ERROR_BAD_PARAMETER] Configuration  \n"));
            error = ST_ERROR_BAD_PARAMETER;
        }
    }
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(error)
    {
        /*Problem in initialization*/
        /*Revert back all allocation*/
        STAUD_TermParams_t TermParams;
        ST_ErrorCode_t	   TermError = ST_NO_ERROR;

        TermError |= STAUD_Term(DeviceName, &TermParams);

        if (TermError != ST_NO_ERROR)
        {
            STTBX_Print(("Term Driver Failed: error %x", TermError));
        }
    }

    if (error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_Init : failed, error %x", error));
    }

    API_Trace(("<<< STAUD_Init <<< Error =%d  \n", error));
    return (error);
}

/************************************************************************************
Name         : STAUD_Open()

Description  : Opens a connection to an audio decoder device.

Parameters   : ST_DeviceName_t      DeviceName      AUDIO driver name
					STAUD_OpenParams_t   *OpenParams     Pointer to the params


ST_NO_ERROR                 Success
ST_ERROR_BAD_PARAMETER      One or more of the supplied parameters is invalid.
ST_ERROR_NO_FREE_HANDLES    The limit on the number of open connections has been
reached.
ST_ERROR_UNKNOWN_DEVICE     The specified Audio Decoder has not been initialized.
************************************************************************************/
ST_ErrorCode_t STAUD_Open(const ST_DeviceName_t DeviceName,
									const STAUD_OpenParams_t *Params,
									STAUD_Handle_t *Handle_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t *drv_p   = NULL;
    API_Trace((" >>> STAUD_Open >>> \n"));
    STTBX_Print(("STAUD_Open DeviceName=[%s], OpenParams=[%x]\n", DeviceName,Params ));

    if( ( Params               == NULL  )   ||           /* NULL structure ptr? */
        ( DeviceName[0]        == '\0'  )   ||           /* Device Name undefined? */
        ( strlen( DeviceName ) >= ST_MAX_DEVICE_NAME ) || /* string too long? */
        Handle_p == NULL) /* Handle Pointer is NULL */
    {
        API_Trace((" <<< STAUD_Open <<< Error  ST_ERROR_BAD_PARAMETER \n"));
        return( ST_ERROR_BAD_PARAMETER );
    }

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_Open <<< Error  ST_ERROR_UNKNOWN_DEVICE \n"));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromName(DeviceName);
    if(drv_p)
    {
        if(drv_p->Valid == FALSE)
        {
            drv_p->Valid = TRUE;
            *Handle_p = (STAUD_Handle_t)drv_p;// uncomment after all the places have been substituted
        }
        else
        {
            API_Trace((" <<< STAUD_Open <<< Error  ST_ERROR_NO_FREE_HANDLES \n"));
            Error = ST_ERROR_NO_FREE_HANDLES;
        }
    }
    else
    {
        Error = ST_ERROR_UNKNOWN_DEVICE;
    }

    STOS_SemaphoreSignal(InitSemaphore_p);

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_Open : done"));
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("STAUD_Open failed, error %x\n", Error ));
    }

    PrintSTAUD_Open(DeviceName, Params, Handle_p);
    API_Trace((" <<< STAUD_Open <<< Error =%d \n ", Error));
    return (Error);
}


/************************************************************************************
Name         : STAUD_Close()

Description  : Closes a connection to an audio decoder device.

Parameters   :
STAUD_Handle_t   Handle

Return Value :
ST_NO_ERROR                 Success
ST_ERROR_INVALID_HANDLE     The specified handle is invalid.
ST_ERROR_UNKNOWN_DEVICE

************************************************************************************/
ST_ErrorCode_t STAUD_Close(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t *drv_p   = NULL;
    API_Trace((" >>> STAUD_Close >>> \n"));
    PrintSTAUD_Close(Handle);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_Close <<< Error  ST_ERROR_UNKNOWN_DEVICE \n"));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    if(drv_p == NULL)
    {
        Error |=  ST_ERROR_INVALID_HANDLE;
    }

    STOS_SemaphoreSignal(InitSemaphore_p);

    if ( Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_Close failed, error %x", Error));
    }
    else
    {
        if(drv_p->Valid == TRUE)
        {
            drv_p->Valid = FALSE ;
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_Close Done"));
        }
        else
        {
            Error |= ST_ERROR_INVALID_HANDLE;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_Close failed Invalid Handle \n "));
        }
    }

    API_Trace((" <<< STAUD_Close <<< Error  %d  \n", Error));
    return(Error);
}


/************************************************************************************
Name         : STAUD_Term()

Description  : This function terminates the audio driver

Parameters   :
	     ST_DeviceName_t 		DeviceName
	     const STAUD_TermParams_t  *TermParams_p
Return Value :
ST_NO_ERROR                 Success
ST_ERROR_INVALID_HANDLE     The specified handle is invalid.
ST_ERROR_UNKNOWN_DEVICE

************************************************************************************/
ST_ErrorCode_t STAUD_Term(const ST_DeviceName_t DeviceName,
const STAUD_TermParams_t *TermParams_p)
{
    ST_ErrorCode_t              Error   = ST_NO_ERROR;
    AudDriver_t                 *drv_p  = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    ST_Partition_t              *CPUPartition_p;
    API_Trace((" >>> STAUD_Term >>> \n "));
    PrintSTAUD_Term(DeviceName, TermParams_p);


    if((TermParams_p == NULL))      /* NULL structure ptr? */
    {
        API_Trace((" <<< STAUD_Term <<< Error ST_ERROR_BAD_PARAMETER \n"));
        return( ST_ERROR_BAD_PARAMETER );
    }

    if((DeviceName[0] == '\0')   ||       /* Device Name undefined? */
        ( strlen( DeviceName ) >= ST_MAX_DEVICE_NAME ))
    {
        API_Trace((" <<< STAUD_Term <<< Error ST_ERROR_UNKNOWN_DEVICE"));
        return ST_ERROR_UNKNOWN_DEVICE;
    }


    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_Term <<< Error ST_ERROR_UNKNOWN_DEVICE"));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromName(DeviceName);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_Term <<< Error ST_ERROR_UNKNOWN_DEVICE"));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    #ifndef STAUD_REMOVE_CLKRV_SUPPORT
        if(drv_p->CLKRV_Handle)
        {
            Error |= STCLKRV_Close(drv_p->CLKRV_Handle);
            if(Error == ST_NO_ERROR)
            {
                drv_p->CLKRV_Handle = 0;
            }
        }
    #endif

    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        switch(Unit_p->Object_Instance)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle, PCMPLAYER_TERMINATE);
                Error |= RemoveFromDriver(drv_p, Unit_p->Handle);
                break;
            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error |= STAud_SPDIFPlayerSetCmd(Unit_p->Handle, SPDIFPLAYER_TERMINATE);
                Error |= RemoveFromDriver(drv_p, Unit_p->Handle);
                break;
            #ifdef FRAME_PROCESSOR_SUPPORT
                case STAUD_OBJECT_FRAME_PROCESSOR:
                    Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_TERMINATE);
                    Error |= RemoveFromDriver(drv_p, Unit_p->Handle);
                    break;

            #endif
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Error |= SendPES_ES_ParseCmd(Unit_p->Handle, TIMER_TERMINATE);
                Error |= RemoveFromDriver(drv_p, Unit_p->Handle);
                break;

            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
                Error |= STAud_DecTerm(Unit_p->Handle);
                Error |= RemoveFromDriver(drv_p, Unit_p->Handle);
                break;

            #ifdef MIXER_SUPPORT
                case STAUD_OBJECT_MIXER0:
                    Error |=STAud_MixerSetCmd(Unit_p->Handle, MIXER_STATE_TERMINATE);
                    Error |= RemoveFromDriver(drv_p, Unit_p->Handle);
                    break;

            #endif
            case STAUD_OBJECT_BLCKMGR:
            /*Must be memory pool*/
                Error |= MemPool_Term(Unit_p->Handle); /*Linear buffer pool*/
                Error |= RemoveFromDriver(drv_p, Unit_p->Handle);
                break;

            #ifdef INPUT_PCM_SUPPORT
                case STAUD_OBJECT_INPUT_PCM0:
                    Error |= STAud_TermPCMInput(Unit_p->Handle);
                    Error |= RemoveFromDriver(drv_p, Unit_p->Handle);
                    break;

            #endif
            default:
                break;

        }

        Unit_p = drv_p->Handle;
    }

    /*Unsubscribe from events*/
    Error |= STAud_UnSubscribeEvents(drv_p);

    /* Release dummy buffer*/
    FreeDummyBuffer(drv_p);

    /* Delete amphion related info*/
    STOS_SemaphoreDelete(NULL, drv_p->AmphionAccess_p);
    STFDMA_UnlockChannel(drv_p->AmphionInChannel, drv_p->FDMABlock);
    STFDMA_UnlockChannel(drv_p->AmphionOutChannel, drv_p->FDMABlock);


    /*deallocat the driver structure*/
    CPUPartition_p = drv_p->CPUPartition_p;
    AudDri_QueueRemove(drv_p);
    memory_deallocate(CPUPartition_p, drv_p);

    STOS_SemaphoreSignal(InitSemaphore_p);

    if (Error != ST_NO_ERROR)
    {
        Error=STAUD_ERROR_MEMORY_DEALLOCATION_FAILED;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, " STAUD_Term fail"));
    }
    API_Trace((" <<< STAUD_Term <<< Error %d \n", Error));
    return (Error);
}

/*-------------------------------------------------------------------------
 * Function : STAUD_GetBufferParam
 *                This function returns the buffer parameters (size and start
 *                address) for any buffer given the handle and the related
 *                object (eg.Frame Processor). This calls another function
 *		     STAud_GetBufferParameter which gets the size and buffer start
 *		     address by making a block manager call using the correct handle.
 *
 * Input    : Handle, InputObject BufferParam (pointer to STAUD_GetBufferParam_t)
 * Output   : Length (buffersize) and StartBase (start address)
 * Return   : error code(ST_ErrorCode_t) if error, ST_NO_ERROR if success
 * -------------------------------------------------------------------------
*/

ST_ErrorCode_t STAUD_GetBufferParam (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_GetBufferParam_t *BufferParam)
{
    ST_ErrorCode_t Error      = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    Error = ST_NO_ERROR;
    API_Trace((" >>> STAUD_GetBufferParam >>> \n"));
    STTBX_Print(("STAUD_GetBufferParam : Obtaining Buffer Parameters \n"));

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_GetBufferParam <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_GetBufferParam <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p, InputObject);

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            #ifdef FRAME_PROCESSOR_SUPPORT

                case STAUD_OBJECT_FRAME_PROCESSOR:
                    Error |= STAud_GetBufferParameter(Obj_Handle, BufferParam);
                    if (Error != ST_NO_ERROR)
                    {
                    STTBX_Print(("STAud_GetBufferParameter() Failed\n"));
                    }
                    break;

            #endif
            default	:
                Error = ST_ERROR_INVALID_HANDLE;
                break;

        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    PrintSTAUD_GetBufferParam(Handle, InputObject, BufferParam);
    API_Trace((" <<< STAUD_GetBufferParam <<< Error %d \n", Error ));
    return Error;
}

/************************************************************************************
Name         : STAUD_GetRevision()

Description  : Retrieves the driver revision.

Parameters   :

Return Value :

************************************************************************************/
ST_Revision_t STAUD_GetRevision(void)
{
    static const ST_Revision_t Revision = "Aud Drv = [STAUDLX-REL_1.0.8]";
    API_Trace((" >>> STAUD_GetRevision >>> \n"));
    API_Trace((" <<< STAUD_GetRevision <<< \n"));
    return(Revision);
}

/************************************************************************************
Name         : STAUD_GetCapability()

Description  : Gets the capabilities of the named Audio Decoder.

Parameters   :

DeviceName        Name of the Audio Decoder from which to get capabilities.
Capability        Allocated structure

Return Value :

ST_ERROR_UNKNOWN_DEVICE     The specified Audio Decoder has not been initialized.
ST_ERROR_BAD_PARAMETER      One of the supplied parameter is invalid.
ST_NO_ERROR                 Success

************************************************************************************/
ST_ErrorCode_t  STAUD_GetCapability (const ST_DeviceName_t DeviceName,
										STAUD_Capability_t * Capability_p)
{
    ST_ErrorCode_t              Error   = ST_NO_ERROR;
    AudDriver_t                 *drv_p  = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    API_Trace((" >>> STAUD_GetCapability >>> \n"));
    PrintSTAUD_GetCapability(DeviceName, Capability_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_GetCapability <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromName(DeviceName);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_GetCapability <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    if (Capability_p == NULL)
    {
        API_Trace((" <<< STAUD_GetCapability <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Get capabilities from primary units(object 0) in the driver  */
    Unit_p = drv_p->Handle;

    Capability_p->NumberOfInputObjects		= 0;
    Capability_p->NumberOfDecoderObjects	= 0;
    Capability_p->NumberOfMixerObjects		= 0;
    Capability_p->NumberOfOutputObjects		= 0;

    while(Unit_p)
    {
        switch(Unit_p->Object_Instance)
        {

            case STAUD_OBJECT_OUTPUT_PCMP0:
                Capability_p->OutputObjects[Capability_p->NumberOfOutputObjects] = Unit_p->Object_Instance;
                Error |= STAud_PCMPlayerGetCapability(Unit_p->Handle, &Capability_p->OPCapability);
                Capability_p->NumberOfOutputObjects++;
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Capability_p->OutputObjects[Capability_p->NumberOfOutputObjects] = Unit_p->Object_Instance;
                Error |= STAud_SPDIFPlayerGetCapability(Unit_p->Handle, &Capability_p->OPCapability);
                Capability_p->NumberOfOutputObjects++;
                break;

            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Capability_p->InputObjects[Capability_p->NumberOfInputObjects] = Unit_p->Object_Instance;
                Error |= PESES_GetInputCapability(Unit_p->Handle, &Capability_p->IPCapability);
                Capability_p->NumberOfInputObjects++;
                break;

            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
                Capability_p->DecoderObjects[Capability_p->NumberOfDecoderObjects] = Unit_p->Object_Instance;
                Error |= STAud_DecGetCapability(Unit_p->Handle, &Capability_p->DRCapability);
                Capability_p->NumberOfDecoderObjects++;
                break;

            #ifdef MIXER_SUPPORT
                case STAUD_OBJECT_MIXER0:
                    Capability_p->MixerObjects[Capability_p->NumberOfMixerObjects] = Unit_p->Object_Instance;
                    Error |= STAud_MixerGetCapability(Unit_p->Handle, &Capability_p->MixerCapability);
                    Capability_p->NumberOfMixerObjects++;
                    break;

            #endif
            default:
                break;

        }

        Unit_p = Unit_p->Next_p;
    }
    API_Trace((" <<< STAUD_GetCapability <<< Error %d \n", Error ));
    return (Error);
 }

/*****************************************************************************
STAUD_OPDisableSynchronization()
Description:
Disables audio synchronization with the local system clock.
Parameters:

Handle           Handle to audio decoder.

OutputObject    References a specific audio decoder.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      Synchronization is not supported by the decoder.

ST_ERROR_INVALID_HANDLE             Handle invalid or not open.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
STAUD_OPEnableSynchronization
*****************************************************************************/

ST_ErrorCode_t STAUD_OPDisableSynchronization (STAUD_Handle_t Handle,
											STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t  Error      = ST_NO_ERROR;
    AudDriver_t		*drv_p     = NULL;
    STAUD_Handle_t  Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPDisableSynchronization >>> \n"));
    PrintSTAUD_OPDisableSynchronization(Handle, OutputObject);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPDisableSynchronization <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPDisableSynchronization <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    Obj_Handle = GetHandle(drv_p,	OutputObject);
    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error |= STAud_PCMPlayerAVSyncCmd(Obj_Handle, FALSE);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
               Error |= STAud_SPDIFPlayerAVSyncCmd(Obj_Handle, FALSE);
               break;

            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Error |= PESES_AVSyncCmd(Obj_Handle, FALSE);
            default:
                break;

        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_OPDisableSynchronization <<< Error %d  \n", Error ));

    return Error;
}
/*****************************************************************************
STAUD_OPEnableSynchronization()

Description:
Disables audio synchronization with the local system clock.
Parameters:

Handle           Handle to audio decoder.

OutputObject    References a specific audio decoder.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      Synchronization is not supported by the decoder.

ST_ERROR_INVALID_HANDLE             Handle invalid or not open.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
	STAUD_OPDisableSynchronization
*****************************************************************************/

ST_ErrorCode_t STAUD_OPEnableSynchronization (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t  Error      = ST_NO_ERROR;
    AudDriver_t		*drv_p     = NULL;
    STAUD_Handle_t  Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPEnableSynchronization >>> \n"));
    PrintSTAUD_OPEnableSynchronization(Handle, OutputObject);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPEnableSynchronization <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPEnableSynchronization <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    Obj_Handle = GetHandle(drv_p, OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error |= STAud_PCMPlayerAVSyncCmd(Obj_Handle, TRUE);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error |= STAud_SPDIFPlayerAVSyncCmd(Obj_Handle, TRUE);
                break;

            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Error |= PESES_AVSyncCmd(Obj_Handle, TRUE);
            default:
                break;

        }

    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_OPEnableSynchronization <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_IPGetBroadcastProfile()
-Description:
Retrieves the expected profile of the broadcasted transport stream.

-Parameters:
Handle               Handle to audio decoder.

InputObject        References a specific audio decoder.

BroadcastProfile_p   Stores the profile of the broadcasted transport stream the audio
stream is extracted from.

-Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPGetBroadcastProfile (STAUD_Handle_t Handle,
															STAUD_Object_t InputObject,
															STAUD_BroadcastProfile_t *
															BroadcastProfile_p)
{
    AudDriver_t      *drv_p     = NULL;
    STPESES_Handle_t Obj_Handle = 0;
    ST_ErrorCode_t   Error      = ST_ERROR_INVALID_HANDLE;
    API_Trace((" >>> STAUD_IPGetBroadcastProfile >>> \n"));
    PrintSTAUD_IPGetBroadcastProfile(Handle, InputObject, BroadcastProfile_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetBroadcastProfile <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetBroadcastProfile <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }
    Obj_Handle = GetHandle(drv_p, InputObject);
    if(Obj_Handle)
    {
        Error = PESES_GetBroadcastProfile(Obj_Handle, BroadcastProfile_p);
    }
    STTBX_Print(("STAUD_DRGetBroadcastProfile\n"));
    API_Trace((" <<< STAUD_IPGetBroadcastProfile <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_IPGetSamplingFrequency()

-Description:
Retrieves the sampling frequency of the input data stream.

-Parameters:
Handle              Handle to audio decoder.

DecoderObject       References a specific audio decoder.

SamplingFrequency_p Stores the result of the current sampling frequency.

-Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     The handle is not valid.

STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not avail-able
with the current configuration.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_IPGetSamplingFrequency (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
U32 * SamplingFrequency_p)
{
    ST_ErrorCode_t  Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t		*drv_p     = NULL;
    STAUD_Handle_t  Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetSamplingFrequency >>> \n"));
    PrintSTAUD_IPGetSamplingFrequency(Handle, InputObject, SamplingFrequency_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetSamplingFrequency <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetSamplingFrequency <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    Obj_Handle = GetHandle(drv_p, InputObject);
    if(Obj_Handle)
    {
        Error = PESES_GetSamplingFreq(Obj_Handle, SamplingFrequency_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Parser Get Freq failed\n"));
        }
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetSamplingFrequency <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPGetSamplingFrequency <<< Error %d \n", Error ));
    return Error;
}

 /*****************************************************************************
STAUD_IPGetStreamInfo()
-Description:
Returns information pertaining to the audio stream
currently being decoded.

-Parameters:
Handle          Handle to audio decoder.
InputObject     References a specific input object
StreamInfo_p    Pointer to a structure for returning data.

-Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     The handle is not valid.
STAUD_ERROR_DECODER_STOPPED Specified decoder is not running
STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_IPGetStreamInfo(STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_StreamInfo_t *StreamInfo_p)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetStreamInfo >>> \n"));
    PrintSTAUD_IPGetStreamInfo(Handle, InputObject, StreamInfo_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetStreamInfo <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetStreamInfo <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(StreamInfo_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetStreamInfo <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p, InputObject);

    if(Obj_Handle)
    {
        Error = PESES_GetStreamInfo(Obj_Handle, StreamInfo_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Parser Get Stream Info failed\n"));
        }
    }
    API_Trace((" <<< STAUD_IPGetStreamInfo <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_IPGetBitBufferFreeSize()
-Description:
Returns information pertaining to the audio stream
currently being decoded.

-Parameters:
Handle          Handle to audio decoder.
InputObject     References a specific input object
Size_p          Pointer to a variable that contains the bit buffur size.

-Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     The handle is not valid.
STAUD_ERROR_DECODER_STOPPED Specified decoder is not running
STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_IPGetBitBufferFreeSize(STAUD_Handle_t Handle,
STAUD_Object_t InputObject, U32 * Size_p)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetBitBufferFreeSize >>> \n"));
    PrintSTAUD_IPGetBitBufferFreeSize(Handle, InputObject, Size_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetBitBufferFreeSize <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetBitBufferFreeSize <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    if(Size_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetBitBufferFreeSize <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }
    Obj_Handle = GetHandle(drv_p, InputObject);
    if(Obj_Handle)
    {
        Error = PESES_GetBitBufferFreeSize(Obj_Handle, Size_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Parser Get Stream Info failed\n"));
        }
    }
    API_Trace((" <<< STAUD_IPGetBitBufferFreeSize <<< Error %d  \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRGetSyncOffset()

Description:
Retrieves the current offset being applied in the audio sync manager.

Parameters:
Handle           Handle to audio decoder.
DecoderObject    References a specific audio decoder.
Offset_p         Stores the result of the current sync offset

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.
STAUD_ERROR_INVALID_DEVICE     The audio decoder is unrecognised or not available
with the current configuration.
ST_FEATURE_NOT_SUPPORTED       The referenced decoder does not support sync offsets

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetSyncOffset (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject,
S32 *Offset_p)
{
    ST_ErrorCode_t Error       = ST_NO_ERROR;
    AudDriver_t    *drv_p      = NULL;
    STAUD_Handle_t Obj_Handle  = 0;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32            ObjectCount = 0;
    API_Trace((" >>> STAUD_DRGetSyncOffset >>> \n"));
    PrintSTAUD_DRGetSyncOffset(Handle, DecoderObject, Offset_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetSyncOffset <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetSyncOffset <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Offset_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetSyncOffset <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    ObjectCount = GetClassHandles(Handle,	STAUD_CLASS_OUTPUT, ObjectArray);

    if(ObjectCount)
    {
        ObjectCount--;
        Obj_Handle = GetHandle(drv_p, ObjectArray[ObjectCount]);
        switch(ObjectArray[ObjectCount])
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error = STAud_PCMPlayerGetOffset(Obj_Handle, Offset_p);
                break;
            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error = STAud_SPDIFPlayerGetOffset(Obj_Handle, Offset_p);
                break;
            default:
                Error = ST_ERROR_INVALID_HANDLE;
                break;
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_DRGetSyncOffset <<< Error %d \n", Error ));
    return Error;
}


/*****************************************************************************
STAUD_DRPause()
Description:
Pauses the audio decoder.

Parameters:
Handle          Handle to audio decoder.

DecoderObject   References a specific audio decoder.

Fade_p          Specifies the fade parameters for ending the audio sound.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_DECODER_PAUSING      Decoder is already paused.

STAUD_ERROR_DECODER_STOPPED     Decoder is stopped and therefore can not be
paused.

STAUD_ERROR_INVALID_DEVICE      The audio decoder is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRPause (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_Fade_t * Fade_p)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRPause >>> \n"));
    PrintSTAUD_DRPause(Handle, InputObject,Fade_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRPause <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRPause <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    Obj_Handle = GetHandle(drv_p, InputObject);
    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Error = SendPES_ES_ParseCmd(Obj_Handle, TIMER_TRANSFER_PAUSE);
                break;

            default:
                break;

        }
    }
    API_Trace((" <<< STAUD_DRPause <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRPrepare()
Description:
Starts the audio decoder with rendering disabled.

Parameters:
Handle          Handle to audio decoder.

DecoderObject   References a specific audio decoder.

Params_p        Decoder preparation parameters.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     The handle is not valid.

STAUD_ERROR_DECODER_PAUSING  Decoder is paused and therefore can not be
started.

STAUD_ERROR_DECODER_RUNNING Decoder is already running.

STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRPrepare (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject,
STAUD_StreamParams_t *StreamParams_p)
{
    API_Trace((" >>> STAUD_DRPrepare >>> \n"));
    PrintSTAUD_DRPrepare(Handle,DecoderObject,StreamParams_p);
    API_Trace((" <<< STAUD_DRPrepare <<<  Error  ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}

/*****************************************************************************
STAUD_DRResume()
Description:
Resumes the decoder from the paused state.
If the decoder is not paused it does nothing
Parameters:
Handle             Handle to audio decoder.

DecoderObject      References a specific audio decoder.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_INVALID_DEVICE      The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_DRResume (STAUD_Handle_t Handle,
STAUD_Object_t InputObject)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRResume >>> \n"));
    PrintSTAUD_DRResume(Handle, InputObject);

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRResume <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRResume <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    Obj_Handle = GetHandle(drv_p, InputObject);
    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Error = SendPES_ES_ParseCmd(Obj_Handle, TIMER_TRANSFER_RESUME);
                break;
            default:
                break;
        }

    }

    /*Notify that the driver has resumed*/
    Error |= STAudEVT_Notify(drv_p->EvtHandle, drv_p->EventIDResumed, NULL, 0, 0);
    API_Trace((" <<< STAUD_DRResume <<< Error %d \n", Error ));
    return Error;
}


/*****************************************************************************

STAUD_IPSetBroadcastProfile()
Description: Sets the Broadcast Profile for the Driver (default is DVB)
Parameters:

Handle  				Handle to the driver
InputObject				Name of the input object for which the profile needs to be set
BroadcastProfile 			Type of the Broadcast Profile to be set

Return Value:

ST_NO_ERROR                         	No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      	The audio driver does not support this
					profile type

ST_ERROR_INVALID_HANDLE             	The handle is not valid.

STAUD_ERROR_UNKNOWN_DEVICE          	The input object is unrecognised or not available
					with the current configuration.

See Also:
PESES_SetBroadcastProfile		in the aud_es_pes_parser file
*****************************************************************************/

ST_ErrorCode_t STAUD_IPSetBroadcastProfile (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_BroadcastProfile_t
BroadcastProfile)
{
    ST_ErrorCode_t   Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t		 *drv_p     = NULL;
    STPESES_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetBroadcastProfile >>> \n"));
    PrintSTAUD_IPSetBroadcastProfile(Handle, InputObject, BroadcastProfile);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPSetBroadcastProfile <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSetBroadcastProfile <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }
    Obj_Handle = GetHandle(drv_p, InputObject);
    if(Obj_Handle)
    {
        if((BroadcastProfile == STAUD_BROADCAST_DVB)||(BroadcastProfile == STAUD_BROADCAST_DIRECTV))
        {
            Error = PESES_SetBroadcastProfile(Obj_Handle, BroadcastProfile);
        }
        else
        {
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
    }
    API_Trace((" <<< STAUD_IPSetBroadcastProfile <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_IPHandleEOF()
Description: Sets the EOF Falg in the Parser
Parameters:

Handle  				Handle to the driver
InputObject				Name of the input object for which the EOF needs to be set

Return Value:

ST_NO_ERROR                         	No error.

ST_ERROR_INVALID_HANDLE             	The handle is not valid.

STAUD_ERROR_UNKNOWN_DEVICE          	The input object is unrecognised or not available
					with the current configuration.

See Also:
PESES_EnableEOFHandling		in the aud_es_pes_parser file
*****************************************************************************/
ST_ErrorCode_t STAUD_IPHandleEOF (STAUD_Handle_t Handle,
STAUD_Object_t InputObject)
{
    ST_ErrorCode_t   Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t		 *drv_p     = NULL;
    STPESES_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPHandleEOF >>> \n"));
    PrintSTAUD_IPHandleEOF(Handle, InputObject);

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPHandleEOF <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPHandleEOF <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }
    Obj_Handle = GetHandle(drv_p, InputObject);

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Error = PESES_EnableEOFHandling(Obj_Handle);
                break;

            #ifdef INPUT_PCM_SUPPORT
                case STAUD_OBJECT_INPUT_PCM0:
                    Error = STAud_PCMInputHandleEOF(Obj_Handle);
                    break;

            #endif
            default:
                Error = ST_ERROR_INVALID_HANDLE;
                break;

        }
    }
    API_Trace((" <<< STAUD_IPHandleEOF <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_IPSetStreamID()
Description:
Sets the stream identifier for filtering on an audio sub-stream.
Parameters:
Handle          Handle to audio decoder.

DecoderObject   References a specific audio decoder.

StreamID        Stream identifier value.

Return Value:
ST_NO_ERROR                No error.

ST_ERROR_BAD_PARAMETER     Stream identifier value is out of range.

ST_ERROR_INVALID_HANDLE    The handle is not valid.

STAUD_ERROR_INVALID_DEVICE The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPSetStreamID (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
U8 StreamID)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetStreamID >>> \n"));
    PrintSTAUD_IPSetStreamID(Handle, InputObject, StreamID);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPSetStreamID <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSetStreamID <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    Obj_Handle = GetHandle(drv_p, InputObject);
    if(Obj_Handle)
    {
        Error = PESES_SetStreamID(Obj_Handle, (U32)StreamID);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Parser Get Freq failed\n"));
        }
    }
    API_Trace((" <<< STAUD_IPSetStreamID <<< Error %d \n", Error ));
    return Error;
}

 /*****************************************************************************
STAUD_DRSetClockRecoverySource()

Description:
Sets the Clock recovery source.
Parameters:
Handle           Handle to audio decoder.
OutputObject    References a specific audio decoder.
ClkSource        STCLKRV Handle.

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.

See Also:
*****************************************************************************/
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    ST_ErrorCode_t STAUD_DRSetClockRecoverySource (STAUD_Handle_t Handle,
    STAUD_Object_t Object, STCLKRV_Handle_t ClkSource)
    {
        ST_ErrorCode_t Error      = ST_NO_ERROR;
        AudDriver_t	   *drv_p     = NULL;
        STAUD_Handle_t Obj_Handle = 0;
        API_Trace((" >>> STAUD_DRSetClockRecoverySource >>> \n"));
        PrintSTAUD_DRSetClockRecoverySource(Handle, Object, ClkSource);
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_DRSetClockRecoverySource <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }

        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_DRSetClockRecoverySource <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return (ST_ERROR_INVALID_HANDLE);
        }

        if (!ClkSource)
        {
            API_Trace((" <<< STAUD_DRSetClockRecoverySource <<< Error ST_ERROR_BAD_PARAMETER \n" ));
            return (ST_ERROR_BAD_PARAMETER);
        }

        #ifndef STAUD_REMOVE_CLKRV_SUPPORT
            Obj_Handle = GetHandle(drv_p, Object);
            if (Obj_Handle)
            {
                switch(Object)
                {
                    case STAUD_OBJECT_INPUT_CD0:
                    case STAUD_OBJECT_INPUT_CD1:

                        Error |= PESES_SetClkSynchro(Obj_Handle, ClkSource);
                        break;

                    case STAUD_OBJECT_OUTPUT_PCMP0:
                        Error |= STAud_PCMPlayerSetClkSynchro(Obj_Handle, ClkSource);
                        break;

                    case STAUD_OBJECT_OUTPUT_SPDIF0:
                        Error |= STAud_SPDIFPlayerSetClkSynchro(Obj_Handle, ClkSource);
                        break;

                    default:
                        break;

                }

            }
            else
            {
                Error = ST_ERROR_INVALID_HANDLE;
            }
        #endif
        /*need to implement for DVD*/
        API_Trace((" <<< STAUD_DRSetClockRecoverySource <<< Error %d \n", Error ));
        return Error;
    }
#endif // Added Because of 8010 //

/*****************************************************************************
STAUD_OPSetDigitalMode()

Description:
Sets the SPDIF output mode for a particular SPDIF player.
Parameters:
Handle				Handle to audio decoder.
OutputObject		References a specific spdif player.
STAUD_DigitalMode_t   OutputMode.

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_OPSetDigitalMode (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject,
STAUD_DigitalMode_t OutputMode)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPSetDigitalMode >>> \n"));
    PrintSTAUD_OPSetDigitalMode(Handle, OutputObject, OutputMode);

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPSetDigitalMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPSetDigitalMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }

    Obj_Handle = GetHandle(drv_p, OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error = STAud_SPDIFPlayerSetSPDIFMode(Obj_Handle, OutputMode);
                break;
            default:
                Error = ST_ERROR_INVALID_HANDLE;
                break;
        }

    }
    API_Trace((" <<< STAUD_OPSetDigitalMode <<< Error %d  \n", Error ));
    return Error;
}


/*****************************************************************************
STAUD_OPGetDigitalMode()

Description:
Gets the SPDIF output mode for a particular SPDIF player.
Parameters:
Handle				Handle to audio decoder.
OutputObject		References a specific spdif player.
STAUD_DigitalMode_t   OutputMode.

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_OPGetDigitalMode (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject,
STAUD_DigitalMode_t * OutputMode)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPGetDigitalMode >>> \n"));
    PrintSTAUD_OPGetDigitalMode(Handle,OutputObject, OutputMode);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPGetDigitalMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPGetDigitalMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }
    Obj_Handle = GetHandle(drv_p, OutputObject);
    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error = STAud_SPDIFPlayerGetSPDIFMode(Obj_Handle, OutputMode);
                break;

            default:
                Error = ST_ERROR_INVALID_HANDLE;
                break;

        }

    }
    API_Trace((" <<< STAUD_OPGetDigitalMode <<< Error %d \n", Error ));
    return Error;
}
/*****************************************************************************
STAUD_DRSetSyncOffset()

Description:
Sets the offset to be applied in the audio sync manager.
Offset is in milliseconds.

Parameters:
Handle           Handle to audio decoder.
DecoderObject    References a specific audio decoder.
Offset           Offset to be applied.

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.
STAUD_ERROR_INVALID_DEVICE     The audio decoder is unrecognised or not available
with the current configuration.
ST_FEATURE_NOT_SUPPORTED       The referenced decoder does not support sync offsets

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetSyncOffset (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject,
S32 Offset)
{
    ST_ErrorCode_t Error       = ST_ERROR_INVALID_HANDLE;
    AudDriver_t    *drv_p      = NULL;
    STAUD_Handle_t Obj_Handle  = 0;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32            ObjectCount = 0;
    API_Trace((" >>> STAUD_DRSetSyncOffset >>> \n"));
    PrintSTAUD_DRSetSyncOffset(Handle, DecoderObject, Offset);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetSyncOffset <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetSyncOffset <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }

    ObjectCount = GetClassHandles(Handle, STAUD_CLASS_OUTPUT, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount--;
        Obj_Handle = GetHandle(drv_p, ObjectArray[ObjectCount]);
        switch(ObjectArray[ObjectCount])
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error = STAud_PCMPlayerSetOffset(Obj_Handle, Offset);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error = STAud_SPDIFPlayerSetOffset(Obj_Handle, Offset);
                break;

            default:
                Error = ST_ERROR_INVALID_HANDLE;
                break;

        }
    }
    API_Trace((" <<< STAUD_DRSetSyncOffset <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRStart()
Description:
start an audio decoder.
Parameters:
Handle IN :        Handle of the Audio Decoder.

Params IN :        Decoding and display parameters

Return Value:
ST_ERROR_INVALID_HANDLE            The specified handle is invalid.

STAUD_ERROR_DECODER_RUNNING        The decoder is already decoding.

STAUD_ERROR_DECODER_PAUSING        The decoder is currently pausing.

ST_ERROR_BAD_PARAMETER             One of the supplied parameter is invalid.

ST_NO_ERROR                        Success.

See Also:
*****************************************************************************/
STAUD_Handle_t audHandle;
STAUD_StreamParams_t audStreamParams;

ST_ErrorCode_t STAUD_DRStart (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject,
STAUD_StreamParams_t * StreamParams_p)
{
    ST_ErrorCode_t              Error   = ST_NO_ERROR;
    AudDriver_t		            *drv_p  = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    API_Trace((" >>> STAUD_DRStart >>> \n"));
    PrintSTAUD_DRStart(Handle, DecoderObject, StreamParams_p);
    if(
        !( 0
        #ifdef STAUD_INSTALL_AC3
        ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_AC3)
        #endif
        #ifdef STAUD_INSTALL_MPEG
        ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG1)
        ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG2)
        ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MP3)
        #endif
        #ifdef STAUD_INSTALL_CDDA
        ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_CDDA)
        ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_PCM)
        #endif
    ))
    {
        STTBX_Print(("STAUD_DRStart wrong config"));
        API_Trace((" <<< STAUD_DRStart <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRStart <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRStart <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        switch(Unit_p->Object_Instance)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle, PCMPLAYER_START);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCMPlayer start failed\n"));
                }
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error |= STAud_SPDIFPlayerSetStreamParams(Unit_p->Handle, StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("SPDIFPlayer set failed\n"));
                }
                Error |= STAud_SPDIFPlayerSetCmd(Unit_p->Handle, SPDIFPLAYER_START);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("SPDIFPlayer Start failed\n"));
                }
                break;

            #ifdef FRAME_PROCESSOR_SUPPORT
                case STAUD_OBJECT_FRAME_PROCESSOR:
                    Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_GET_INPUT_BLOCK);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("DataFormatter Start failed\n"));
                    }
                    break;

            #endif
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Error |= PESESSetAudioType(Unit_p->Handle, StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PESESS set failed\n"));
                }
                if(Error == ST_NO_ERROR)
                {
                    Error |= SendPES_ES_ParseCmd(Unit_p->Handle, TIMER_TRANSFER_START);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("PESESS Start failed\n"));
                    }
                }
                break;

            #ifdef INPUT_PCM_SUPPORT
                case STAUD_OBJECT_INPUT_PCM0:
                    Error |= STAud_StartPCMInput(Unit_p->Handle);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("Input PCM Start failed\n"));
                    }
                    break;

            #endif
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
                Error |= STAud_DecSetStreamParams(Unit_p->Handle, StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Decoder Set failed\n"));
                }
                Error |= STAud_DecStart(Unit_p->Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Decoder Start failed\n"));
                }
                break;

            #ifdef MIXER_SUPPORT
                case STAUD_OBJECT_MIXER0:

                    Error |= STAud_MixerSetCmd(Unit_p->Handle, MIXER_STATE_START);
                    if(Error != ST_NO_ERROR)
                    {
                    //printf(" Mixer start failed \n ");
                    }
                    break;

            #endif
            default:
                /*Must be memory */
                Error |= MemPool_Start(Unit_p->Handle); /*Linear buffer pool*/
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("MemPool Start failed\n"));
                }
                break;

        }

        Unit_p = Unit_p->Next_p;
    }
    audHandle       = Handle;
    audStreamParams = *(StreamParams_p);
    API_Trace((" <<< STAUD_DRStart <<< Error %d  \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRStop()
Description:
Stops the audio decoder.
Parameters:
Handle         Handle to audio decoder.

DecoderObject  References a specific audio decoder.

StopMode       Indicates when to stop decoding.

Fade_p         Specifies the fade parameters for ending the audio decode.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_INVALID_DEVICE      The audio decoder is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_DRStop (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject,
STAUD_Stop_t StopMode, STAUD_Fade_t * Fade_p)
{
    ST_ErrorCode_t				Error   = ST_NO_ERROR;
    AudDriver_t					*drv_p  = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    API_Trace((" >>> STAUD_DRStop >>> \n"));
    PrintSTAUD_DRStop(Handle, DecoderObject, StopMode, Fade_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRStop <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRStop <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        switch(Unit_p->Object_Instance)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle, PCMPLAYER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCMPlayer Stop failed\n"));
                }
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error |= STAud_SPDIFPlayerSetCmd(Unit_p->Handle,  SPDIFPLAYER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("SPDIFPlayer Stop failed\n"));
                }
                break;

            #ifdef FRAME_PROCESSOR_SUPPORT
                case STAUD_OBJECT_FRAME_PROCESSOR:
                    Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_STOPPED);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("DataFormatter Stop failed\n"));
                    }
                    break;

            #endif
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Error |= SendPES_ES_ParseCmd(Unit_p->Handle, TIMER_STOP);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PESES Stop failed\n"));
                }
                break;

            #ifdef INPUT_PCM_SUPPORT
                case STAUD_OBJECT_INPUT_PCM0:
                    Error |= STAud_StopPCMInput(Unit_p->Handle);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("Input PCM Stop failed\n"));
                    }
                    break;

            #endif
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_DECODER_COMPRESSED2:
                Error |= STAud_DecStop(Unit_p->Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Decoder Stop failed\n"));
                }
                break;

            #ifdef MIXER_SUPPORT
                case STAUD_OBJECT_MIXER0:
                    Error |= STAud_MixerSetCmd(Unit_p->Handle, MIXER_STATE_STOP);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("Mixer Stop failed\n"));
                    }
                    break;

            #endif
            default:
                /*Must be memory */
                Error |= MemPool_Flush(Unit_p->Handle); /*Linear buffer pool*/
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("MemPool Flush failed\n"));
                }
                break;

        }

        Unit_p = Unit_p->Next_p;
    }

    /*Notify that the driver has stopped*/
    Error |= STAudEVT_Notify(drv_p->EvtHandle, drv_p->EventIDStopped, NULL, 0, 0);
    API_Trace((" <<< STAUD_DRStop <<< Error %d \n", Error ));
    return Error;
}


/*****************************************************************************
STAUD_IPGetInputBufferParams()
Description: to get  PESES buffer parameters.
Parameters:
		STAUD_Handle_t 				Handle
		STAUD_Object_t 				InputObject
		STAUD_BufferParams_t* 		DataParams_p
Return Value:
		ST_NO_ERROR                     No error.

		ST_ERROR_BAD_PARAMETER          The stereo output parameter is invalid.

		ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support stereo
		                                output configuration.

		ST_ERROR_INVALID_HANDLE         The handle is not valid.

		STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not
		                                available with the current configuration
See Also:
******************************************************************************/
ST_ErrorCode_t STAUD_IPGetInputBufferParams(STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_BufferParams_t* DataParams_p)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetInputBufferParams >>> \n "));
    PrintSTAUD_IPGetInputBufferParams(Handle, InputObject, DataParams_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetInputBufferParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetInputBufferParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (DataParams_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetInputBufferParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p, InputObject);

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                Error = PESES_GetInputBufferParams(Obj_Handle, DataParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Parser Get Input buffer Params failed\n"));
                }
                break;

            #ifdef INPUT_PCM_SUPPORT
                case STAUD_OBJECT_INPUT_PCM0:
                    Error = STAud_PCMInputBufferParams(Obj_Handle, DataParams_p);
                    break;

            #endif
            default:
                break;

        }
    }
    STTBX_Print((" STAUD_IPGetInputBufferParams Add = %x \n", DataParams_p->BufferBaseAddr_p));
    STTBX_Print((" STAUD_IPGetInputBufferParams exited \n" ));
    API_Trace((" <<< STAUD_IPGetInputBufferParams <<< Error %d \n", Error ));
    return(Error);
}

/*****************************************************************************
STAUD_IPSetDataInputInterface()
Description:Sets the data input interface.
Parameters:STAUD_Handle_t 				Handle
		STAUD_Object_t 				InputObject
		GetWriteAddress_t 		        GetWriteAddress_p
		InformReadAddress_t 			InformReadAddress_p
		void * const                            BufferHandle_p
Return Value:
		ST_NO_ERROR                     No error.

		ST_ERROR_BAD_PARAMETER          The stereo output parameter is invalid.

		ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support stereo
		                                output configuration.

		ST_ERROR_INVALID_HANDLE         The handle is not valid.

		STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not
		                                available with the current configuration
See Also:
*******************************************************************************/

ST_ErrorCode_t  STAUD_IPSetDataInputInterface(STAUD_Handle_t Handle,
															STAUD_Object_t InputObject,
															GetWriteAddress_t   GetWriteAddress_p,
															InformReadAddress_t InformReadAddress_p,
															void * const BufferHandle_p)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetDataInputInterface >>> \n"));
    PrintSTAUD_IPSetDataInputInterface(Handle, InputObject, GetWriteAddress_p, InformReadAddress_p, BufferHandle_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPSetDataInputInterface <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSetDataInputInterface <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p, InputObject);

    if(Obj_Handle)
    {
        Error = PESES_SetDataInputInterface(Obj_Handle, GetWriteAddress_p, InformReadAddress_p, BufferHandle_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Parser Get Input buffer Params failed\n"));
        }
    }
    API_Trace((" <<< STAUD_IPSetDataInputInterface <<< Error %d \n", Error ));
    return(Error);
}

/*****************************************************************************
STAUD_IPGetCapability()
Description:
Returns the capabilities of a specific input object.

Parameters:

DeviceName      Instance of audio device.
InputObject     Specific input object to return capabilites for.
Capability_p    Pointer to returned capabilities.

Return Value:

ST_NO_ERROR                 No error.
ST_ERROR_UNKNOWN_DEVICE     The specified device is not initialized
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPGetCapability(const ST_DeviceName_t DeviceName,
STAUD_Object_t InputObject,
STAUD_IPCapability_t * Capability_p)
{
   ST_ErrorCode_t  Error      =ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetCapability >>> \n"));
    PrintSTAUD_IPGetCapability(DeviceName, InputObject, Capability_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetCapability <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromName(DeviceName);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetCapability <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Capability_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetCapability <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }
    Obj_Handle = GetHandle(drv_p,	InputObject);

    if(Obj_Handle)
    {
        Error = PESES_GetInputCapability(Obj_Handle, Capability_p);
    }
    API_Trace((" <<< STAUD_IPGetCapability <<< Error %d \n", Error ));
    return (Error);
}

/*****************************************************************************
STAUD_IPGetDataInterfaceParams()
Description:
Obtains the DMA parameters for executing a DMA transfer to an input interface
of an audio decoder.
Parameters:

Handle          Handle to audio input device.
InputObject     References a specific input interface
DMAParams_p     Stores result of the DMA parameters for transferring data to
the input interface.

Return Value:

ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPGetDataInterfaceParams (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_DataInterfaceParams_t *DMAParams_p)
{
    API_Trace((" >>> STAUD_IPGetDataInterfaceParams >>> \n "));
    PrintSTAUD_IPGetDataInterfaceParams(Handle, InputObject, DMAParams_p);
    API_Trace((" <<<  STAUD_IPGetDataInterfaceParams <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}


/*****************************************************************************
STAUD_IPGetParams()
Description:
Obtains the current configuration parameters for an input to an audio decoder.

Parameters:
Handle              Handle to audio input device.
InputObject         References a specific input interface
InputParams_p       Stores result of current input configuration parameters.

Return Value:

ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPGetParams (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_InputParams_t * InputParams_p)
{
    API_Trace((" >>> STAUD_IPGetParams >>> \n "));
    PrintSTAUD_IPGetParams(Handle, InputObject, InputParams_p);
    API_Trace((" <<<  STAUD_IPGetParams <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}

/*****************************************************************************
STAUD_IPQueuePCMBuffer()

Description:
Queues one or more PCM buffers on a PCM input interface.
Parameters:
Handle          Handle to audio input device.
InputObject     References a specific input interface
PCMBuffer_p     An array of PCM buffers.
NumBuffers      Number of PCM buffers in the array.
NumQueued_p     Number of PCM buffers successfully queued.

Return Value:

ST_NO_ERROR                     No error.
ST_ERROR_INVALID_HANDLE         Handle invalid or not open.
ST_ERROR_FEATURE_NOT_SUPPORTED  The device does not support a PCM buffering.
STAUD_ERROR_INVALID_DEVICE      The input interface is unrecognised or not
available with the current configuration.

See Also:
*****************************************************************************/
#ifdef INPUT_PCM_SUPPORT
    ST_ErrorCode_t STAUD_IPQueuePCMBuffer (STAUD_Handle_t Handle,
    STAUD_Object_t InputObject,
    STAUD_PCMBuffer_t * PCMBuffer_p,
    U32 NumBuffers, U32 * NumQueued_p)
    {
        ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
        AudDriver_t	   *drv_p     = NULL;
        STAUD_Handle_t Obj_Handle = 0;
        API_Trace((" >>> STAUD_IPQueuePCMBuffer >>> \n"));
        PrintSTAUD_IPQueuePCMBuffer(Handle, InputObject, PCMBuffer_p, NumBuffers, NumQueued_p);
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_IPQueuePCMBuffer <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }

        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_IPQueuePCMBuffer <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }

        Obj_Handle = GetHandle(drv_p, InputObject);

        if(Obj_Handle)
        {
            Error = STAud_QueuePCMBuffer(Obj_Handle, PCMBuffer_p, NumBuffers, NumQueued_p);
            if (Error != ST_NO_ERROR)
            {

            }
        }
        API_Trace((" <<< STAUD_IPQueuePCMBuffer <<< Error %d \n", Error ));
        return Error;
    }

    /*****************************************************************************
    STAUD_IPGetPCMBuffer()

    Description:
    Queues one or more PCM buffers on a PCM input interface.
    Parameters:
    Handle          Handle to audio input device.
    InputObject     References a specific input interface
    PCMBuffer_p     An array of PCM buffers.

    Return Value:

    ST_NO_ERROR                     No error.
    ST_ERROR_INVALID_HANDLE         Handle invalid or not open.
    ST_ERROR_FEATURE_NOT_SUPPORTED  The device does not support a PCM buffering.
    STAUD_ERROR_INVALID_DEVICE      The input interface is unrecognised or not
    available with the current configuration.

    See Also:
    *****************************************************************************/
    ST_ErrorCode_t STAUD_IPGetPCMBuffer (STAUD_Handle_t Handle,
    STAUD_Object_t InputObject,
    STAUD_PCMBuffer_t * PCMBuffer_p)
    {
        ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
        AudDriver_t	   *drv_p     = NULL;
        STAUD_Handle_t Obj_Handle = 0;
        API_Trace((" >>> STAUD_IPGetPCMBuffer >>> \n "));
        PrintSTAUD_IPGetPCMBuffer(Handle, InputObject, PCMBuffer_p);
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_IPGetPCMBuffer <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }

        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_IPGetPCMBuffer <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }
        Obj_Handle = GetHandle(drv_p, InputObject);
        if(Obj_Handle)
        {
            Error = STAud_GetPCMBuffer(Obj_Handle, PCMBuffer_p);
            if (Error != ST_NO_ERROR)
            {

            }
        }
        API_Trace((" <<< STAUD_IPGetPCMBuffer <<< Error %d \n", Error ));
        return Error;
    }

    /*****************************************************************************
    STAUD_IPGetPCMBufferSize()

    Description:
    This routine return the total buffers size for PCM input..
    Parameters:
    Handle          Handle to audio input device.
    InputObject     References a specific input interface
    U32 * BufferSize

    Return Value:

    ST_NO_ERROR                     No error.
    ST_ERROR_INVALID_HANDLE         Handle invalid or not open.
    ST_ERROR_FEATURE_NOT_SUPPORTED  The device does not support a PCM buffering.
    STAUD_ERROR_INVALID_DEVICE      The input interface is unrecognised or not
    available with the current configuration.

    See Also:
    *****************************************************************************/
    ST_ErrorCode_t STAUD_IPGetPCMBufferSize (STAUD_Handle_t Handle,
    											STAUD_Object_t InputObject,
    											U32 * BufferSize_p)
    {
        ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
        AudDriver_t	   *drv_p     = NULL;
        STAUD_Handle_t Obj_Handle = 0;
        API_Trace((" >>> STAUD_IPGetPCMBufferSize >>> \n"));
        PrintSTAUD_IPGetPCMBufferSize(Handle, InputObject, BufferSize_p);
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_IPGetPCMBufferSize <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }

        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_IPGetPCMBufferSize <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }

        Obj_Handle = GetHandle(drv_p, InputObject);

        if(Obj_Handle)
        {
            Error = STAud_GetPCMBufferSize(Obj_Handle, BufferSize_p);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_GetPCMBufferSize() Failed\n"));
            }
        }
        API_Trace((" <<< STAUD_IPGetPCMBufferSize <<< Error %d \n", Error ));
        return Error;
    }
#endif
/******************************************************************************
STAUD_IPLowDataLevelEvent()
Description:
Sets the low data threshold for a given decoder.
Parameters:
Handle          Handle to audio decoder.
DecoderObject   References a specific audio decoder.
Level           Low Data threshold as a % (0-100)
Return Value:
ST_NO_ERROR                     No error.
ST_ERROR_INVALID_HANDLE         The handle is not valid.
STAUD_ERROR_INVALID_DEVICE      The audio decoder is unrecognised or not available
with the current configuration.
See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_IPSetLowDataLevelEvent (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
U8             Level)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetLowDataLevelEvent >>> \n"));
    PrintSTAUD_IPSetLowDataLevelEvent(Handle,InputObject, Level);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPSetLowDataLevelEvent <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSetLowDataLevelEvent <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p, InputObject);

    if(Obj_Handle)
    {
        Error = PESES_SetBitBufferLevel(Obj_Handle, (U32)Level);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Set PES Buffer level events\n"));
        }
    }
    API_Trace((" <<< STAUD_IPSetLowDataLevelEvent <<< Error %d \n", Error ));
    return Error;
}

/****************************************************************************
STAUD_IPSetParams()
Description:
Sets the configuration parameters for an input to an audio device.
Parameters:
Handle          Handle to audio input device.
InputObject     References a specific input interface
InputParams_p   Input configuration parameters.

Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or
not available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPSetParams (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_InputParams_t * InputParams_p)
{
    API_Trace((" >>> STAUD_IPSetParams >>> \n"));
    PrintSTAUD_IPSetParams(Handle, InputObject, InputParams_p);
    API_Trace((" <<< STAUD_IPSetParams <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}

/****************************************************************************
STAUD_IPSetPCMParams()
Description:
Sets the configuration parameters for an input to an audio device.
Parameters:
Handle          Handle to audio input device.
InputObject     References a specific input interface
InputParams_p   Input PCM configuration parameters.

Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or
not available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPSetPCMParams (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_PCMInputParams_t * InputParams_p)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    U32	           WordSize;
    API_Trace((" >>> STAUD_IPSetPCMParams >>> \n"));
    PrintSTAUD_IPSetPCMParams(Handle, InputObject, InputParams_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPSetPCMParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSetPCMParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (InputParams_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSetPCMParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p, InputObject);

    if(Obj_Handle)
    {
        if((InputObject == STAUD_OBJECT_INPUT_CD0) ||
            (InputObject == STAUD_OBJECT_INPUT_CD1))
        {
            WordSize = InputParams_p->DataPrecision;
            Error = PESES_SetPCMParams(Obj_Handle, InputParams_p->Frequency, WordSize, (U32)InputParams_p->NumChannels);
        }
        #ifdef INPUT_PCM_SUPPORT
            if(InputObject == STAUD_OBJECT_INPUT_PCM0)
            {
                Error = STAud_SetPCMParams(Obj_Handle, InputParams_p->Frequency, WordSize, (U32)InputParams_p->NumChannels);
            }
        #endif
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Set PCM Buffer params\n"));
        }
    }
        API_Trace((" <<< STAUD_IPSetPCMParams <<< Error %d \n", Error ));
        return Error;
}

/*****************************************************************************
STAUD_MXConnectSource()
Description:Connect the input source to the mixer.
Parameters:
    STAUD_Handle_t                 Handle,
    STAUD_Object_t                 MixerObject,
    STAUD_Object_t                 *InputSources_p,
    STAUD_MixerInputParams_t*      InputParams_p,
    U32 			   NumInputs
Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or
not available with the current configuration.
See Also:
*****************************************************************************/
#ifdef MIXER_SUPPORT
    ST_ErrorCode_t STAUD_MXConnectSource (STAUD_Handle_t Handle,
    STAUD_Object_t MixerObject,
    STAUD_Object_t *InputSources_p,
    STAUD_MixerInputParams_t * InputParams_p,
    U32 NumInputs)
    {
        API_Trace((" >>> STAUD_MXConnectSource >>> \n"));
        PrintSTAUD_MXConnectSource(Handle, MixerObject, InputSources_p, InputParams_p, NumInputs);
        API_Trace((" <<< STAUD_MXConnectSource <<< \n"));
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }


 /*****************************************************************************
    STAUD_MXGetCapability()
    Description:
    Returns the capabilities of a specific mixer object.

    Parameters:

    DeviceName      Instance of audio device.
    MixerObject     Specific mixer object to return capabilites for.
    Capability_p    Pointer to returned capabilities.

    Return Value:

    ST_NO_ERROR                 No error.
    ST_ERROR_UNKNOWN_DEVICE     The specified device is not initialized
    STAUD_ERROR_INVALID_DEVICE  The mixer device is unrecognised or not available
    with the current configuration.

    See Also:
    *****************************************************************************/
    ST_ErrorCode_t STAUD_MXGetCapability(const ST_DeviceName_t DeviceName,
    											STAUD_Object_t MixerObject,
    											STAUD_MXCapability_t * Capability_p)
    {
        ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
        AudDriver_t	   *drv_p     = NULL;
        STAUD_Handle_t Obj_Handle = 0;
        API_Trace((" >>> STAUD_MXGetCapability >>> \n"));
        PrintSTAUD_MXGetCapability(DeviceName, MixerObject, Capability_p);
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_MXGetCapability <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }
        drv_p = AudDri_GetBlockFromName(DeviceName);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_MXGetCapability <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }
        if (Capability_p == NULL)
        {
            API_Trace((" <<< STAUD_MXGetCapability <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }
        Obj_Handle = GetHandle(drv_p,	MixerObject);
        if(Obj_Handle)
        {
            Error = STAud_MixerGetCapability(Obj_Handle, Capability_p);
        }
        API_Trace((" <<< STAUD_MXGetCapability <<< Error %d \n", Error));
        return (Error);
    }


    /*****************************************************************************
    STAUD_MXSetInputParams()
    Description:
    Parameters:
    Return Value:
    See Also:
    *****************************************************************************/

    ST_ErrorCode_t STAUD_MXSetInputParams (STAUD_Handle_t Handle,
    STAUD_Object_t MixerObject,
    STAUD_Object_t InputSource,
    STAUD_MixerInputParams_t *
    InputParams_p)
    {
        API_Trace((" >>> STAUD_MXSetInputParams >>> \n"));
        PrintSTAUD_MXSetInputParams(Handle, MixerObject, InputSource, InputParams_p);
        API_Trace((" <<< STAUD_MXSetInputParams <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
#endif
/***
**************************************************************************
STAUD_OPGetCapability()
Description:
Returns the capabilities of a specific output object.

Parameters:

DeviceName      Instance of audio device.
OutputObject    Specific output object to return capabilites for.
Capability_p    Pointer to returned capabilities.

Return Value:

ST_NO_ERROR                 No error.
ST_ERROR_UNKNOWN_DEVICE     The specified device is not initialized
STAUD_ERROR_INVALID_DEVICE  The output device is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_OPGetCapability(const ST_DeviceName_t DeviceName,
											STAUD_Object_t OutputObject,
											STAUD_OPCapability_t * Capability_p)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPGetCapability >>> \n"));
    PrintSTAUD_OPGetCapability(DeviceName,OutputObject, Capability_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPGetCapability <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromName(DeviceName);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPGetCapability <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Capability_p == NULL)
    {
        API_Trace((" <<< STAUD_OPGetCapability <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if(Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error = STAud_PCMPlayerGetCapability(Obj_Handle, Capability_p);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error = STAud_SPDIFPlayerGetCapability(Obj_Handle, Capability_p);
                break;

            default:
                break;

        }//		Error = STAud_OutputGetCapability(Obj_Handle, Capability_p);
    }
    API_Trace((" <<< STAUD_OPGetCapability <<< Error %d \n", Error ));
    return (Error);
}

/*****************************************************************************
STAUD_OPGetParams()

Description:
Obtains the current output configuration parameters.

Parameters:
Handle          Handle to audio output device.

OutputObject    References a specific output interface

Params_p        Stores result of current output configuration parameters.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_OPGetParams (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject,
STAUD_OutputParams_t * Params_p)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPGetParams >>> \n"));
    PrintSTAUD_OPGetParams(Handle, OutputObject, Params_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPGetParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPGetParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error = STAud_PCMPlayerGetOutputParams(Obj_Handle, &(Params_p->PCMOutParams));
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error = STAud_SPDIFPlayerGetOutputParams(Obj_Handle, &(Params_p->SPDIFOutParams));
                break;

            default:
                Error = ST_ERROR_INVALID_HANDLE;
                break;

        }
    }
    API_Trace((" <<< STAUD_OPGetParams <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_OPMute()

Description:
Mutes the output device.

Parameters:
Handle           Handle to audio output.

OutputObject     References a specific output interface

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_OPMute (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPMute >>> \n"));
    PrintSTAUD_OPMute(Handle,OutputObject);
    #if !MUTE_AT_DECODER
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_OPMute <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }

        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);

        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_OPMute <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }

        Obj_Handle = GetHandle(drv_p,	OutputObject);

        if(Obj_Handle)
        {
            if((OutputObject == STAUD_OBJECT_OUTPUT_SPDIF0))
            {
                Error = STAud_SPDIFPlayerMute(Obj_Handle, TRUE); // Mute the SPDIF Player
            }
            if((OutputObject == STAUD_OBJECT_OUTPUT_PCMP0) )
            {
                Error = STAud_PCMPlayerMute(Obj_Handle, TRUE); // Mute the PCM Player
            }
        }
    #else
        Error = STAUD_DRMute(Handle, OutputObject);
    #endif
    API_Trace((" <<< STAUD_OPMute <<< Error %d \n", Error ));
    return Error;
}
/*****************************************************************************
STAUD_MXSetMixLevel()

Description:
Sets the mixer level wrt to InputID

Parameters:
Handle           Handle to audio output.

MixerObject     References a specific output interface

InputID		The inputID with respect to Mixer inputs
Mix_Level	The mixing Level

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

ST_ERROR_UNKNOWN_DEVICE


See Also:STAUD_MXGetMixLevel
*****************************************************************************/
#ifdef MIXER_SUPPORT
    ST_ErrorCode_t STAUD_MXSetMixLevel (STAUD_Handle_t Handle,
    											STAUD_Object_t MixerObject, U32 InputID, U16 MixLevel)
    {
        ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
        AudDriver_t	   *drv_p     = NULL;
        STAUD_Handle_t Obj_Handle = 0;
        API_Trace((" >>> STAUD_MXSetMixLevel >>> \n"));
        PrintSTAUD_MXSetMixLevel(Handle, MixerObject, InputID, MixLevel);
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_MXSetMixLevel <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }

        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_MXSetMixLevel <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }

        Obj_Handle = GetHandle(drv_p,	MixerObject);
        if(Obj_Handle)
        {
            Error = STAud_SetMixerMixLevel(Obj_Handle, InputID, MixLevel);
        }
        API_Trace((" <<< STAUD_MXSetMixLevel <<< Error %d \n", Error ));
        return Error;
    }
#endif
 /*****************************************************************************
STAUD_OPSetParams()

Description:
Sets the output configuration parameters.

Parameters:
Handle           Handle to audio output device.

OutputObject     References a specific output interface

Params_p         Output configuration parameters.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_OPSetParams (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject,
STAUD_OutputParams_t * Params_p)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPSetParams >>> \n"));
    PrintSTAUD_OPSetParams(Handle, OutputObject, Params_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPSetParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPSetParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error = STAud_PCMPlayerSetOutputParams(Obj_Handle, Params_p->PCMOutParams);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error = STAud_SPDIFPlayerSetOutputParams(Obj_Handle, Params_p->SPDIFOutParams);
                break;

            default:
                Error = ST_ERROR_INVALID_HANDLE;
                break;

        }
    }
    API_Trace((" <<< STAUD_OPSetParams <<< Error %d \n", Error ));
    return Error;
}

 /*****************************************************************************
STAUD_OPUnMute()

Description:
The output device is un-muted.
Parameters:
Handle           Handle to audio output.

OutputObject     References a specific output interface

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_OPUnMute (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPUnMute >>> \n"));
    PrintSTAUD_OPUnMute(Handle,OutputObject);
    #if !MUTE_AT_DECODER
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_OPUnMute <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }
        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_OPUnMute <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }
        Obj_Handle = GetHandle(drv_p,	OutputObject);

        if(Obj_Handle)
        {
            if((OutputObject == STAUD_OBJECT_OUTPUT_SPDIF0))
            {
                Error = STAud_SPDIFPlayerMute(Obj_Handle, FALSE); // Mute the SPDIF Player
            }
            if((OutputObject == STAUD_OBJECT_OUTPUT_PCMP0))
            {
                Error = STAud_PCMPlayerMute(Obj_Handle, FALSE); // Mute the SPDIF Player
            }
        }
    #else
        Error = STAUD_DRUnMute(Handle, OutputObject);
    #endif
    API_Trace((" <<< STAUD_OPUnMute <<< Error %d \n", Error ));
    return Error;
}


/*****************************************************************************
STAUD_MXGetMixLevel()

Description:
Returns the Mix_Level of respective Mixer input.

Parameters:

Handle     	 Handle to audio driver.
MixerObject  	Specific MixerObject object to return Mix_Level
InputID    	ID to track requested mixer input
Mix_Level 	Pointer to returned Mix_Level

Return Value:

ST_NO_ERROR                 No error.
ST_ERROR_UNKNOWN_DEVICE     The specified device is not initialized
ST_ERROR_INVALID_HANDLE
ST_ERROR_BAD_PARAMETER
ST_ERROR_INVALID_HANDLE

See Also:
****************************************************************************/
#ifdef MIXER_SUPPORT
    ST_ErrorCode_t STAUD_MXGetMixLevel (STAUD_Handle_t Handle,
    											STAUD_Object_t MixerObject, U32 InputID, U16 *MixLevel_p)
    {
        ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
        AudDriver_t	   *drv_p     = NULL;
        STAUD_Handle_t Obj_Handle = 0;
        API_Trace((" >>> STAUD_MXGetMixLevel >>> \n"));
        PrintSTAUD_MXGetMixLevel(Handle,MixerObject, InputID, MixLevel_p);
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_MXGetMixLevel <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }

        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_MXGetMixLevel <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }

        if(MixLevel_p == NULL)
        {
            API_Trace((" <<< STAUD_MXGetMixLevel <<< Error ST_ERROR_BAD_PARAMETER \n" ));
            return ST_ERROR_BAD_PARAMETER;
        }

        Obj_Handle = GetHandle(drv_p,	MixerObject);
        if(Obj_Handle)
        {
            Error = STAud_GetMixerMixLevel(Obj_Handle, InputID, MixLevel_p);
        }
        API_Trace((" <<< STAUD_MXGetMixLevel <<< Error %d \n", Error ));
        return Error;
    }
#endif


U32	GetClassHandles(STAUD_Handle_t Handle,U32	ObjectClass, STAUD_Object_t *ObjectArray)
{
    STAUD_DrvChainConstituent_t *Unit_p     = NULL;
    U32                         ObjectCount = 0;
    AudDriver_t                 *drv_p;
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        if((Unit_p->Object_Instance & 0xffff0000) == ObjectClass)
        {
            ObjectArray[ObjectCount] = Unit_p->Object_Instance;
            ObjectCount++;
        }
        Unit_p = Unit_p->Next_p;
    }
    return (ObjectCount);
}


/*****************************************************************************
STAUD_MXUpdatePTSStatus()

Description:
Sets the PTS availability of the input stream.


Parameters:
Handle             Handle to audio Object.

MixerObject      References the mixer object.

InputId			Specifies the mixer input.

PTSStaus		TRUE:PTS Present, FALSE:PTS Absent

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/
#ifdef MIXER_SUPPORT

    ST_ErrorCode_t STAUD_MXUpdatePTSStatus(STAUD_Handle_t Handle,
    	STAUD_Object_t MixerObject, U32 InputID, BOOL PTSStatus)
    {
        ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
        AudDriver_t	   *drv_p     = NULL;
        STAUD_Handle_t Obj_Handle = 0;
        API_Trace((" >>> STAUD_MXUpdatePTSStatus >>> \n"));
        PrintSTAUD_MXUpdatePTSStatus(Handle, MixerObject, InputID, PTSStatus);
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_MXUpdatePTSStatus <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }

        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_MXUpdatePTSStatus <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }

        Obj_Handle = GetHandle(drv_p, MixerObject);

        if(Obj_Handle)
        {
            Error = STAud_MixerUpdatePTSStatus(Obj_Handle, InputID, PTSStatus);
        }
        API_Trace((" <<< STAUD_MXUpdatePTSStatus <<< Error %d \n", Error ));
        return 	Error;
    }

    /*****************************************************************************
    STAUD_MXUpdateMaster()

    Description:
    Sets the Given Input as Master


    Parameters:
    Handle             Handle to audio Object.

    MixerObject      References the mixer object.

    InputId			Specifies the mixer input.

    Return             Value.

    ST_NO_ERROR                         No error.

    ST_ERROR_INVALID_HANDLE             The handle is not valid.

    STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
    with the current configuration.
    See Also:
    *****************************************************************************/

    ST_ErrorCode_t STAUD_MXUpdateMaster(STAUD_Handle_t Handle,
    	STAUD_Object_t MixerObject, U32 InputID, BOOL FreeRun)
    {
        ST_ErrorCode_t Error      = ST_ERROR_INVALID_HANDLE;
        AudDriver_t	   *drv_p     = NULL;
        STAUD_Handle_t Obj_Handle = 0;
        API_Trace((" >>> STAUD_MXUpdateMaster >>> \n"));
        PrintSTAUD_MXUpdateMaster(Handle, MixerObject, InputID, FreeRun);
        if(InitSemaphore_p)
        {
            STOS_SemaphoreWait(InitSemaphore_p);
        }
        else
        {
            API_Trace((" <<< STAUD_MXUpdateMaster <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
            return ST_ERROR_UNKNOWN_DEVICE;
        }
        drv_p = AudDri_GetBlockFromHandle(Handle);
        STOS_SemaphoreSignal(InitSemaphore_p);
        if(drv_p == NULL)
        {
            API_Trace((" <<< STAUD_MXUpdateMaster <<< Error ST_ERROR_INVALID_HANDLE \n" ));
            return ST_ERROR_INVALID_HANDLE;
        }

        Obj_Handle = GetHandle(drv_p, MixerObject);

        if(Obj_Handle)
        {
            Error = STAud_MixerUpdateMaster(Obj_Handle, InputID, FreeRun);
        }
        API_Trace((" <<< STAUD_MXUpdateMaster <<< Error %d \n", Error ));
        return 	Error;
    }

#endif


/*****************************************************************************
STAUD_ModuleStart()
Description:
starts each audio module for which it is called.
Parameters:
Handle IN :        Handle of the Audio Decoder.

Params IN :        Module and Stream parameters

Return Value:
ST_ERROR_INVALID_HANDLE            The specified handle is invalid.

ST_ERROR_BAD_PARAMETER             One of the supplied parameter is invalid.

ST_NO_ERROR                        Success.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_ModuleStart (STAUD_Handle_t Handle,
STAUD_Object_t ModuleObject,
STAUD_StreamParams_t * StreamParams_p)
{
    ST_ErrorCode_t              Error   = ST_NO_ERROR;
    AudDriver_t		            *drv_p  = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    API_Trace((" >>> STAUD_ModuleStart >>> \n"));
    PrintSTAUD_ModuleStart(Handle, ModuleObject, StreamParams_p);
    if(
        !( 0
        #ifdef STAUD_INSTALL_AC3
            ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_AC3)
        #endif
        #ifdef STAUD_INSTALL_MPEG
            ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG1)
            ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG2)
            ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MP3)
        #endif
        #ifdef STAUD_INSTALL_CDDA
            ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_CDDA)
            ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_PCM)
        #endif
    ))
    {
        STTBX_Print(("STAUD_DRStart wrong config"));
        API_Trace((" <<< STAUD_ModuleStart <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }
    if(InitSemaphore_p)
    {
        semaphore_wait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_ModuleStart <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    semaphore_signal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_ModuleStart <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        if (Unit_p->Object_Instance == ModuleObject)
        {
            switch(Unit_p->Object_Instance)
            {
                case STAUD_OBJECT_OUTPUT_SPDIF0:
                    Error = STAud_SPDIFPlayerSetStreamParams(Unit_p->Handle, StreamParams_p);
                    if (Error != ST_NO_ERROR)
                    {
                        	STTBX_Print(("SPDIFPlayer set failed\n"));
                    }
                    Error |= STAud_SPDIFPlayerSetCmd(Unit_p->Handle, SPDIFPLAYER_START);
                    if (Error != ST_NO_ERROR)
                    {
                        	STTBX_Print(("SPDIFPlayer Start failed\n"));
                    }
                    API_Trace((" <<< STAUD_ModuleStart <<< Error %d \n", Error ));
                    return Error;
                    break;

                case STAUD_OBJECT_INPUT_CD0:
                case STAUD_OBJECT_INPUT_CD1:
                    Error = PESESSetAudioType(Unit_p->Handle, StreamParams_p);
                    if (Error != ST_NO_ERROR)
                    {
                        	STTBX_Print(("PESESS set failed\n"));
                    }

                    if(Error == ST_NO_ERROR)
                    {
                        Error |= SendPES_ES_ParseCmd(Unit_p->Handle, TIMER_TRANSFER_START);
                        if (Error != ST_NO_ERROR)
                        {
                            STTBX_Print(("PESESS Start failed\n"));
                        }
                    }
                    API_Trace((" <<< STAUD_ModuleStart <<< Error %d \n", Error ));
                    return Error;
                    break;

                case STAUD_OBJECT_DECODER_COMPRESSED0:
                case STAUD_OBJECT_DECODER_COMPRESSED1:
                    Error = STAud_DecSetStreamParams(Unit_p->Handle, StreamParams_p);
                    if (Error != ST_NO_ERROR)
                    {
                    	    STTBX_Print(("Decoder Set failed\n"));
                    }
                    Error |= STAud_DecStart(Unit_p->Handle);
                    if (Error != ST_NO_ERROR)
                    {
                    	    STTBX_Print(("Decoder Start failed\n"));
                    }
                    API_Trace((" <<< STAUD_ModuleStart <<< Error %d \n", Error ));
                    return Error;
                    break;

                #ifdef FRAME_PROCESSOR_SUPPORT

                    case STAUD_OBJECT_FRAME_PROCESSOR:
                    {
                        Error = STAud_DataProcessorSetStreamParams(Unit_p->Handle, StreamParams_p);
                        if (Error != ST_NO_ERROR)
                        {
                            STTBX_Print(("Data Processor set failed\n"));
                        }
                        API_Trace((" <<< STAUD_ModuleStart <<< Error %d \n", Error ));
                        return Error;
                    }
                    break;

                #endif
                default:
                    break;

            }

        }
        Unit_p = Unit_p->Next_p;
    }
    API_Trace((" <<< STAUD_ModuleStart <<< Error %d \n", Error ));
    return Error;
}


/*****************************************************************************
STAUD_GenericStart()
Description:
start all audio required modules to start the driver
Parameters:
Handle IN :        Handle of the Audio Decoder.

Return Value:
ST_ERROR_INVALID_HANDLE            The specified handle is invalid.

ST_ERROR_BAD_PARAMETER             One of the supplied parameter is invalid.

ST_NO_ERROR                        Success.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_GenericStart (STAUD_Handle_t Handle)
{
    ST_ErrorCode_t              Error   = ST_NO_ERROR;
    AudDriver_t		            *drv_p  = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    API_Trace((" >>> STAUD_GenericStart >>> \n"));
    PrintSTAUD_GenericStart(Handle);
    if(InitSemaphore_p)
    {
        semaphore_wait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_GenericStart <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    semaphore_signal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_GenericStart <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        switch(Unit_p->Object_Instance)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle, PCMPLAYER_START);
                if (Error != ST_NO_ERROR)
                {
                STTBX_Print(("PCMPlayer start failed\n"));
                }
                break;

            #ifdef FRAME_PROCESSOR_SUPPORT
                case STAUD_OBJECT_FRAME_PROCESSOR:
                    Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_GET_INPUT_BLOCK);
                    if (Error != ST_NO_ERROR)
                    {
                    STTBX_Print(("DataFormatter Start failed\n"));
                    }
                    break;

            #endif
            case STAUD_OBJECT_INPUT_PCM0:
                break;

            #ifdef MIXER_SUPPORT
                case STAUD_OBJECT_MIXER0:
                    Error |= STAud_MixerSetCmd(Unit_p->Handle, MIXER_STATE_START);
                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" Mixer start failed \n "));
                    }
                    break;

            #endif
            default:
                /*Must be memory */
                if (Unit_p->Object_Instance == STAUD_OBJECT_BLCKMGR)
                {
                    Error |= MemPool_Start(Unit_p->Handle); /*Linear buffer pool*/
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("MemPool Start failed\n"));
                    }
                }
                break;

        }

        Unit_p = Unit_p->Next_p;
    }
    API_Trace((" <<< STAUD_GenericStart <<< Error %d \n", Error ));
    return Error;
}
/*****************************************************************************
STAUD_ModuleStop()
Description:
stops each audio module for which it is called.
Parameters:
Handle IN :   STAUD_Handle_t       Handle             Handle of the Audio Decoder.

Params IN :  STAUD_Object_t      ModuleObject     Module and Stream parameters

Return Value:
ST_ERROR_INVALID_HANDLE            The specified handle is invalid.

ST_ERROR_BAD_PARAMETER             One of the supplied parameter is invalid.

ST_NO_ERROR                        Success.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_ModuleStop (STAUD_Handle_t Handle, STAUD_Object_t ModuleObject)
{
    ST_ErrorCode_t	            Error 	= ST_NO_ERROR;
    AudDriver_t	                *drv_p 	= NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    API_Trace((" >>> STAUD_ModuleStop >>> \n"));
    PrintSTAUD_ModuleStop(Handle, ModuleObject);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_ModuleStop <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_ModuleStop <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        if (Unit_p->Object_Instance == ModuleObject)
        {
            switch(Unit_p->Object_Instance)
            {
                case STAUD_OBJECT_OUTPUT_SPDIF0:
                    Error |= STAud_SPDIFPlayerSetCmd(Unit_p->Handle, SPDIFPLAYER_STOPPED);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("SPDIFPlayer Stop failed\n"));
                    }
                    API_Trace((" <<< STAUD_ModuleStop <<< Error %d \n" , Error));
                    return Error;
                    break;

                case STAUD_OBJECT_INPUT_CD0:
                case STAUD_OBJECT_INPUT_CD1:
                    Error |= SendPES_ES_ParseCmd(Unit_p->Handle, TIMER_STOP);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("PESES Stop failed\n"));
                    }
                    API_Trace((" <<< STAUD_ModuleStop <<< Error %d \n" , Error));
                    return Error;
                    break;

                case STAUD_OBJECT_DECODER_COMPRESSED0:
                case STAUD_OBJECT_DECODER_COMPRESSED1:
                    Error |= STAud_DecStop(Unit_p->Handle);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("Decoder Stop failed\n"));
                    }
                    API_Trace((" <<< STAUD_ModuleStop <<< Error %d \n" , Error));
                    return Error;
                    break;


                #ifdef FRAME_PROCESSOR_SUPPORT
                    case STAUD_OBJECT_FRAME_PROCESSOR:
                        Error = ST_NO_ERROR;
                        API_Trace((" <<< STAUD_ModuleStop <<< Error %d \n" , Error));
                        return Error;
                        break;

                #endif
                default:
                    break;

            }
        }

        Unit_p = Unit_p->Next_p;
    }
    API_Trace((" <<< STAUD_ModuleStop <<< Error %d \n" , Error));
    return Error;
}

ST_ErrorCode_t STAUD_GenericStop(STAUD_Handle_t Handle)
{
    ST_ErrorCode_t				Error   = ST_NO_ERROR;
    AudDriver_t					*drv_p  = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    API_Trace((" >>> STAUD_GenericStop >>> \n"));
    PrintSTAUD_GenericStop(Handle);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_GenericStop <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_GenericStop <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        switch(Unit_p->Object_Instance)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle, PCMPLAYER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCMPlayer Stop failed\n"));
                }
                break;

            #ifdef FRAME_PROCESSOR_SUPPORT
                case STAUD_OBJECT_FRAME_PROCESSOR:
                    Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_STOPPED);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("DataFormatter Stop failed\n"));
                    }
                    break;

            #endif
            case STAUD_OBJECT_INPUT_PCM0:
                break;

            #ifdef MIXER_SUPPORT
                case STAUD_OBJECT_MIXER0:
                    Error |= STAud_MixerSetCmd(Unit_p->Handle, MIXER_STATE_STOP);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("Mixer Stop failed\n"));
                    }
                    break;

            #endif
            default:
                /*Must be memory */
                Error |= MemPool_Flush(Unit_p->Handle); /*Linear buffer pool*/
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("MemPool Flush failed\n"));
                }
                break;

        }

        Unit_p = Unit_p->Next_p;
    }

    /*Notify that the driver has stopped*/
    Error |= STAudEVT_Notify(drv_p->EvtHandle, drv_p->EventIDStopped, NULL, 0, 0);
    API_Trace((" <<< STAUD_GenericStop <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_GetModuleAttenuation()
Description:
Gets the current volume attenuation applied to the output channels.
Parameters:
Handle       :   Handle to audio output.

ModuleObject :    References a specific audio driver module

Attenuation_p   Stores the result of the current attenuation applied
to the output channels.

Return Value:
ST_NO_ERROR No error.
ST_ERROR_INVALID_HANDLE Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_GetModuleAttenuation (STAUD_Handle_t Handle,
STAUD_Object_t ModuleObject,
STAUD_Attenuation_t * Attenuation_p)
{
    ST_ErrorCode_t Error      = ST_NO_ERROR;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_GetModuleAttenuation >>> \n"));
    PrintSTAUD_GetModuleAttenuation(Handle, ModuleObject, Attenuation_p);

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_GetModuleAttenuation <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_GetModuleAttenuation <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Attenuation_p == NULL)
    {
        API_Trace((" <<< STAUD_GetModuleAttenuation <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p, ModuleObject);

    if(Obj_Handle)
    {
        switch (ModuleObject & 0xffff0000)
        {
            case STAUD_CLASS_DECODER:
                Error |= STAUD_GetAmphionAttenuation(Obj_Handle, Attenuation_p);
                break;
            #ifdef MIXER_SUPPORT
                case STAUD_CLASS_MIXER:
                    Error |= STAud_MixerGetVol(Obj_Handle, Attenuation_p);
                    break;
            #endif
            default:
                Error = ST_ERROR_INVALID_HANDLE;

        }

    }
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_GetModuleAttenuation <<< Error %d \n", Error ));
    return Error;
}

/*****************************************************************************
STAUD_SetModuleAttenuation()

Description:
Sets the volume attenuation applied to the output channels.

Parameters:
Handle           Handle to audio output.

ModuleObject     References a specific audio driver module
Attenuation_p    Specifies attenuation to apply to output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_SetModuleAttenuation (STAUD_Handle_t Handle,
STAUD_Object_t ModuleObject,
STAUD_Attenuation_t * Attenuation_p)
{
    ST_ErrorCode_t Error      = ST_NO_ERROR;
    AudDriver_t	   *drv_p     = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_SetModuleAttenuation >>> \n"));
    PrintSTAUD_SetModuleAttenuation(Handle, ModuleObject, Attenuation_p);

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_SetModuleAttenuation <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_SetModuleAttenuation <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Attenuation_p == NULL)
    {
        API_Trace((" <<< STAUD_SetModuleAttenuation <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p, ModuleObject);

    if(Obj_Handle)
    {
        switch (ModuleObject & 0xffff0000)
        {
            case STAUD_CLASS_DECODER:
                Error |= STAUD_SetAmphionAttenuation(Obj_Handle, Attenuation_p);
                break;

            #ifdef MIXER_SUPPORT
                case STAUD_CLASS_MIXER:
                    Error |= STAud_MixerSetVol(Obj_Handle, Attenuation_p);
                    break;
            #endif
            default:
                Error = ST_ERROR_INVALID_HANDLE;

        }
    }
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_SetModuleAttenuation <<< Error %d \n", Error ));
    return Error;
}
/*****************************************************************************
STAUD_DRSetStereoMode()

Description:
Set the stereo output configuration. The new stereo mode
will be effective next time the decoder is started.

Parameters:
Handle           Handle to post processing block.

PostProcObject   References a specific post processor.

StereoMode       The required stereo output configuration.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_BAD_PARAMETER          The stereo output parameter is invalid.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support stereo
output configuration.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not
available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetStereoMode (STAUD_Handle_t Handle,
				STAUD_Object_t DecoderObject,STAUD_StereoMode_t StereoMode)
{
	ST_ErrorCode_t Error      = ST_NO_ERROR;
	AudDriver_t	   *drv_p     = NULL;
	STAUD_Handle_t Obj_Handle = 0;

	if(InitSemaphore_p)
	{
		STOS_SemaphoreWait(InitSemaphore_p);
	}
	else
	{
		return ST_ERROR_UNKNOWN_DEVICE;
	}

	drv_p = AudDri_GetBlockFromHandle(Handle);
	STOS_SemaphoreSignal(InitSemaphore_p);
	if(drv_p == NULL)
	{
		return ST_ERROR_INVALID_HANDLE;
	}

	Obj_Handle = GetHandle(drv_p,	DecoderObject);

	if(Obj_Handle)
	{
		Error = STAUD_SetAmphionStereoMode(Obj_Handle, StereoMode);
	}
	else
	{
		Error =  ST_ERROR_INVALID_HANDLE;
	}

	return Error;
}
/*****************************************************************************
STAUD_DRGetStereoMode()

Description:
Set the stereo output configuration. The new stereo mode
will be effective next time the decoder is started.

Parameters:
Handle           Handle to post processing block.

PostProcObject   References a specific post processor.

StereoMode       The required stereo output configuration.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_BAD_PARAMETER          The stereo output parameter is invalid.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support stereo
output configuration.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not
available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetStereoMode (STAUD_Handle_t Handle,
				STAUD_Object_t DecoderObject,STAUD_StereoMode_t * StereoMode_p)
{
	ST_ErrorCode_t Error      = ST_NO_ERROR;
	AudDriver_t	   *drv_p     = NULL;
	STAUD_Handle_t Obj_Handle = 0;

	if(InitSemaphore_p)
	{
		STOS_SemaphoreWait(InitSemaphore_p);
	}
	else
	{
		return ST_ERROR_UNKNOWN_DEVICE;
	}

	drv_p = AudDri_GetBlockFromHandle(Handle);
	STOS_SemaphoreSignal(InitSemaphore_p);
	if(drv_p == NULL)
	{
		return ST_ERROR_INVALID_HANDLE;
	}

	Obj_Handle = GetHandle(drv_p,	DecoderObject);

	if(Obj_Handle)
	{
		Error = STAUD_GetAmphionStereoMode(Obj_Handle, StereoMode_p);
	}
	else
	{
		Error =  ST_ERROR_INVALID_HANDLE;
	}

	return Error;
}


