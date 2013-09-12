/************************************************************************
COPYRIGHT STMicroelectronics (C) 2004
Source file name : staud.c
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
//#define STTBX_PRINT 1
#define API_Trace STTBX_Print
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif

#define MAX_SPEED 200
#define MIN_SPEED 80
#define NORMAL_SPEED 100
#include "stos.h"
#include "staud.h"
#include "aud_mmeaudiostreamdecoder.h"
#include "aud_mmeaudiostreamencoder.h"
#include "aud_mmeaudiomixer.h"
#include "aud_mmeaudiodecoderpp.h"
#include "aud_pcmplayer.h"
#include "aud_pcmreader.h"
#include "aud_pes_es_parser.h"
#include "aud_spdifplayer.h"
#include "aud_utils.h"
#include "aud_mmepp.h"
#include "pcminput.h"
#include "audio_decodertypes.h"
#include "aud_evt.h"
#include "staudlx_prints.h"
#ifdef ST_OSLINUX
#include "acc_mme.h" /* This is being included for MME wrapper implementation for Linux*/
#endif

BOOL Internal_Dacs = FALSE;

#define STAUD_MAX_DEVICE      	1
#define STAUD_MAX_OPEN	      	1


#if defined(ST_8010)
#define	AVSYNC_ADDED			0
#else
#define	AVSYNC_ADDED			1
#endif
#define REV_STRING_LEN 			(256)

#define ATTENU_AT_DECODER		    0
#define MIXER_INPUT_FROM_FILE 	0

#ifdef ST_OSLINUX
void * remappedAddress=NULL; /* This will be used for unmapping kernel input buffer in case of FP*/
#endif

static semaphore_t *InitSemaphore_p = NULL;
BOOL STAUD_SelectDac(void);
U32	GetClassHandles(STAUD_Handle_t Handle,U32	ObjectClass, STAUD_Object_t *ObjectArray);

#ifdef ST_OSLINUX
/* ACC Wrapper functions */
#if _ACC_WRAPP_MME_
ST_ErrorCode_t Init_MME_Dump(STAVMEM_PartitionHandle_t AVMEMPartition);
void 		acc_mme_logreset(void);
ST_ErrorCode_t DeInit_MME_Dump(void);
#endif
#endif

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
    U8				CurrentLXRevision[REV_STRING_LEN];

    API_Trace((">>> STAUD_Init >>> \n "));
    PrintSTAUD_Init(DeviceName,InitParams_p);
    STTBX_Print(("STAUD_Init DeviceName=[%s], InitParams_p=[%x] \n",DeviceName,(U32)InitParams_p ));

    /*initialize the Driver Protection Semaphore*/
    STOS_TaskLock();
    if(InitSemaphore_p == NULL)
    {
        InitSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);
    }
    STOS_TaskUnlock();

    STOS_SemaphoreWait(InitSemaphore_p);

    if( (InitParams_p== NULL) ||(*DeviceName	== 0   ) ||
    (InitParams_p->MaxOpen == 0   ) || (strlen( DeviceName )>=  ST_MAX_DEVICE_NAME ) )
    {
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] InitP, DeviceName, Max, Open \n "));
        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_BAD_PARAMETER);
    }

    /**for checking Number of output channels**/
    if((InitParams_p->NumChannels!=2)&&(InitParams_p->NumChannels!=10))
    {
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] Num Channels \n "));
        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_BAD_PARAMETER);
    }


    if((InitParams_p->DeviceType!= STAUD_DEVICE_STI7109) && (InitParams_p->DeviceType!= STAUD_DEVICE_STI7100) && (InitParams_p->DeviceType!= STAUD_DEVICE_STI8010) && (InitParams_p->DeviceType!= STAUD_DEVICE_STI5525)&& (InitParams_p->DeviceType!= STAUD_DEVICE_STI7200))
    {
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] Device Type \n "));
        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_BAD_PARAMETER);
    }

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    if((strlen(InitParams_p->ClockRecoveryName) == 0)||(strlen(InitParams_p->ClockRecoveryName)>ST_MAX_DEVICE_NAME))
    {
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_BAD_PARAMETER] Clock Recovery Name \n "));
        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_BAD_PARAMETER);
    }
#endif

#ifdef ST_OSLINUX
    InitParams_p->CPUPartition_p = (ST_Partition_t *)-1;
    InitParams_p->AVMEMPartition = STAVMEM_GetPartitionHandle(0); /* LMI_SYS */
    /* Check if we need to allocate memory from AVMem instead of EMBX */
    if(InitParams_p->AllocateFromEMBX == FALSE)
    {
        if(InitParams_p->EnableMSPPSupport == FALSE)
        {
            InitParams_p->BufferPartition = STAVMEM_GetPartitionHandle(0);
        }
        else
        {
            InitParams_p->BufferPartition = STAVMEM_GetPartitionHandle(1); /* This should be confirmed by MSSP user */
        }
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

#ifdef ST_OSLINUX
    #if _ACC_WRAPP_MME_
        /* Allocate memory for storing mme communication */
        error = Init_MME_Dump(InitParams_p->AVMEMPartition);
        if(error == ST_NO_ERROR)
        {
            STTBX_Print(("Init_MME_Dump Successful \n"));
        }
        else
        {
            STTBX_Print(("Init_MME_Dump Failed %x \n", error));
        }
    #endif
#endif

    /* Ping the LX to check if its properly initialized */
    error = STAud_DecPingLx(CurrentLXRevision, REV_STRING_LEN );
    if(error != ST_NO_ERROR)
    {
        /* LX is not yet initialised so we can't initialize audio driver. */
        STOS_SemaphoreSignal(InitSemaphore_p);
        API_Trace(("<<< STAUD_Init <<< Error [LX Ping Failed] =%d \n   ",error));
        return error;
    }

    strncpy(InitParams_p->RegistrantDeviceName,DeviceName,ST_MAX_DEVICE_NAME_TOCOPY);
    qp_p = (AudDriver_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(AudDriver_t));
    if(!qp_p)
    {
        STOS_SemaphoreSignal(InitSemaphore_p);
        API_Trace(("<<< STAUD_Init <<< Error [ST_ERROR_NO_MEMORY] AUDDriver_t Line = %d \n   ",__LINE__));
        return ST_ERROR_NO_MEMORY;
    }
    memset(qp_p,0,sizeof(AudDriver_t));
    strncpy(qp_p->Name,DeviceName, ST_MAX_DEVICE_NAME_TOCOPY);
    qp_p->Handle = (STAUD_DrvChainConstituent_t *)AUDIO_INVALID_HANDLE;
    qp_p->Configuration = InitParams_p->Configuration;
    qp_p->CPUPartition_p = InitParams_p->CPUPartition_p;
    qp_p->Speed			= 100;
    strncpy(qp_p->EvtHandlerName,InitParams_p->EvtHandlerName,ST_MAX_DEVICE_NAME_TOCOPY);
    AudDri_QueueAdd(qp_p);

    if (error == ST_NO_ERROR)
    {
#ifndef STAUD_REMOVE_CLKRV_SUPPORT

        error = STCLKRV_Open( InitParams_p->ClockRecoveryName,NULL,&(qp_p->CLKRV_Handle));

        if(error!=ST_NO_ERROR)
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
        ST_ErrorCode_t		TermError = ST_NO_ERROR;
        TermError |= STAUD_Term(DeviceName,&TermParams);
        if (TermError != ST_NO_ERROR)
        {
            STTBX_Print(("Term Driver Failed: error %x", TermError));
        }
    }

    if (error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_Init : failed, error %x", error));
    }

    API_Trace(("<<< STAUD_Init <<< Error =%d  \n",error));
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
    AudDriver_t *drv_p = NULL;
    API_Trace((" >>> STAUD_Open >>> \n"));
    STTBX_Print(("STAUD_Open DeviceName=[%s],OpenParams=[%x] \n",DeviceName,(U32)Params ));

    if(( Params == NULL  )   ||	( DeviceName[0]  == '\0'  )   ||
    ( strlen( DeviceName ) >= ST_MAX_DEVICE_NAME ) || 	Handle_p == NULL)
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
        STTBX_Print(("STAUD_Open failed, error %x\n",Error ));
    }

    PrintSTAUD_Open(DeviceName,Params,Handle_p);
    API_Trace((" <<< STAUD_Open <<< \n"));
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
    AudDriver_t *drv_p = NULL;
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

 #ifdef ST_OSLINUX
    if(remappedAddress)
        iounmap(remappedAddress); // unmapping the remapped kernel space address of the kernel input buffer
 #endif

    if ( Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_Close failed, error %x", Error));
    }
    else
    {
        if(drv_p->Valid==TRUE)
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

    API_Trace((" <<< STAUD_Close <<< Error  %d  \n",Error));
    return(Error);
}

/*----------------------------------------------------------------------------
Name   : STAUD_Term
Purpose: This function terminates the audio driver
Note   : -
-----------------------------------------------------------------------------*/

ST_ErrorCode_t STAUD_Term(const ST_DeviceName_t DeviceName,
const STAUD_TermParams_t *TermParams_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t *drv_p = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    ST_Partition_t	*CPUPartition_p;
    API_Trace((" >>> STAUD_Term >>> \n "));
    PrintSTAUD_Term(DeviceName,TermParams_p);

    if((TermParams_p== NULL))      /* NULL structure ptr? */
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
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle,PCMPLAYER_TERMINATE);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerSetCmd(Unit_p->Handle, SPDIFPLAYER_TERMINATE);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
            case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
                Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_TERMINATE);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_FRAME_PROCESSOR:
            case STAUD_OBJECT_FRAME_PROCESSOR1:
                Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_TERMINATE);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error |= SendPES_ES_ParseCmd(Unit_p->Handle,TIMER_TERMINATE);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_DECODER_COMPRESSED2:
                Error |= STAud_DecTerm(Unit_p->Handle);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_ENCODER_COMPRESSED0:
                Error |= STAud_EncTerm(Unit_p->Handle);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_POST_PROCESSOR0:
            case STAUD_OBJECT_POST_PROCESSOR1:
            case STAUD_OBJECT_POST_PROCESSOR2:
                Error |=STAud_PPTerm(Unit_p->Handle);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_MIXER0:
                Error |=STAud_MixerSetCmd(Unit_p->Handle,MIXER_STATE_TERMINATE);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_BLCKMGR:
            /*Must be memory pool*/
                Error |= MemPool_Term(Unit_p->Handle); /*Linear buffer pool*/
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_INPUT_PCM0:
                Error |= STAud_TermPCMInput(Unit_p->Handle);
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;
            case STAUD_OBJECT_INPUT_PCMREADER0:
                Error |= STAud_PCMReaderSetCmd(Unit_p->Handle, PCMREADER_TERMINATE);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCM READER Term failed\n"));
                }
                Error |= RemoveFromDriver(drv_p,Unit_p->Handle);
                break;

            default:
                break;
        }

        Unit_p = drv_p->Handle;
    }

    /*Unsubscribe from events*/
    Error |= STAud_UnSubscribeEvents(drv_p);

#ifdef ST_OSLINUX
    #if _ACC_WRAPP_MME_
    /* Allocate memory for storing mme communication */
    Error = DeInit_MME_Dump();
    if(Error == ST_NO_ERROR)
    {
        STTBX_Print(("DeInit_MME_Dump Successful \n"));
    }
    else
    {
        STTBX_Print(("DeInit_MME_Dump Failed %x \n", Error));
    }
    #endif
#endif
    /*deallocat the driver structure*/
    CPUPartition_p = drv_p->CPUPartition_p;
    AudDri_QueueRemove(drv_p);
    STOS_MemoryDeallocate(CPUPartition_p,drv_p);

    STOS_SemaphoreSignal(InitSemaphore_p);

    if (Error != ST_NO_ERROR)
    {
        Error=STAUD_ERROR_MEMORY_DEALLOCATION_FAILED;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, " STAUD_Term fail"));
    }
    API_Trace((" <<< STAUD_Term <<< Error %d \n",Error));
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
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t	*drv_p = NULL;
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        switch(InputObject)
        {

            case STAUD_OBJECT_FRAME_PROCESSOR:
            case STAUD_OBJECT_FRAME_PROCESSOR1:
                Error |= STAud_GetBufferParameter(Obj_Handle,BufferParam);
                if (Error != ST_NO_ERROR)
                {
                STTBX_Print(("STAud_GetBufferParameter() Failed\n"));
                }
                break;
            default	:
                Error = ST_ERROR_INVALID_HANDLE;
                break;
        }
#ifdef ST_OSLINUX
    //BufferParam->StartBase=(void*)ioremap_nocache(virt_to_phys(BufferParam->StartBase),BufferParam->Length);
   // BufferParam->StartBase= (void*)(virt_to_phys(BufferParam->StartBase));
    remappedAddress=(void *)BufferParam->StartBase; //will be used in STAUD_DRSTOP for unmapping
#endif
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    PrintSTAUD_GetBufferParam(Handle,InputObject,BufferParam);
    API_Trace((" <<< STAUD_GetBufferParam <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    static U8 Revision[REV_STRING_LEN];
    char CurrentLXRevision[REV_STRING_LEN];
    API_Trace((" >>> STAUD_GetRevision >>> \n"));
    memset(Revision,0,REV_STRING_LEN);
    memset(CurrentLXRevision,0,REV_STRING_LEN);

    strncpy((void *)Revision,"Aud Drv = [STAUDLX-REL_1.0.8]:Recommended LX = [CBL_ACF_ST200Audio_BL021p2]:Current LX =",(REV_STRING_LEN-1));
    /* Get hold of current LX irmware used in system */
    Error = STAud_DecPingLx((U8 *)CurrentLXRevision, REV_STRING_LEN );
    if(Error == ST_NO_ERROR)
    {
        /* Concatenate the LX firmware version */
        strncat((void *)Revision,(void *)CurrentLXRevision,strlen(CurrentLXRevision));

    }
    else
    {
        strncat((void *)Revision,"NOT KNOWN",9); /* strelen(""NOT KNOWN"") is 9*/
    }
    API_Trace((" <<< STAUD_GetRevision <<< \n"));
    return (ST_Revision_t)&Revision;
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t *drv_p = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    API_Trace((" >>> STAUD_GetCapability >>> \n"));
    PrintSTAUD_GetCapability(DeviceName,Capability_p);
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
    Capability_p->NumberOfPostProcObjects	= 0;
    Capability_p->NumberOfMixerObjects		= 0;
    Capability_p->NumberOfOutputObjects		= 0;
    Capability_p->NumberOfEncoderObjects	= 0;

    while(Unit_p)
  {
        switch(Unit_p->Object_Instance)
        {

            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Capability_p->OutputObjects[Capability_p->NumberOfOutputObjects] = Unit_p->Object_Instance;
                Error |= STAud_PCMPlayerGetCapability(Unit_p->Handle,&Capability_p->OPCapability);
                Capability_p->NumberOfOutputObjects++;
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Capability_p->OutputObjects[Capability_p->NumberOfOutputObjects] = Unit_p->Object_Instance;
                Error |= STAud_SPDIFPlayerGetCapability(Unit_p->Handle,&Capability_p->OPCapability);
                Capability_p->NumberOfOutputObjects++;
                break;

            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Capability_p->InputObjects[Capability_p->NumberOfInputObjects] = Unit_p->Object_Instance;
                Error |= PESES_GetInputCapability(Unit_p->Handle,&Capability_p->IPCapability);
                Capability_p->NumberOfInputObjects++;
                break;

            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_DECODER_COMPRESSED2:
                Capability_p->DecoderObjects[Capability_p->NumberOfDecoderObjects] = Unit_p->Object_Instance;
                Error |= STAud_DecGetCapability(Unit_p->Handle,&Capability_p->DRCapability);
                Capability_p->NumberOfDecoderObjects++;
                break;

            case STAUD_OBJECT_ENCODER_COMPRESSED0:
                Capability_p->EncoderObjects[Capability_p->NumberOfEncoderObjects] = Unit_p->Object_Instance;
                Error |= STAud_EncoderGetCapability(Unit_p->Handle,&Capability_p->ENCCapability);
                Capability_p->NumberOfEncoderObjects++;
                break;

            case STAUD_OBJECT_POST_PROCESSOR0:
            case STAUD_OBJECT_POST_PROCESSOR1:
            case STAUD_OBJECT_POST_PROCESSOR2:
                Capability_p->PostProcObjects[Capability_p->NumberOfPostProcObjects] = Unit_p->Object_Instance;
                Error |=STAud_PostProcGetCapability(Unit_p->Handle,&Capability_p->PostProcCapability);
                Capability_p->NumberOfPostProcObjects++;
                break;

            case STAUD_OBJECT_MIXER0:
                Capability_p->MixerObjects[Capability_p->NumberOfMixerObjects] = Unit_p->Object_Instance;
                Error |=STAud_MixerGetCapability(Unit_p->Handle,&Capability_p->MixerCapability);
                Capability_p->NumberOfMixerObjects++;
                break;

            case STAUD_OBJECT_INPUT_PCMREADER0:
                Capability_p->InputObjects[Capability_p->NumberOfInputObjects] = Unit_p->Object_Instance;
                Error |=STAud_PCMReaderGetCapability(Unit_p->Handle,&Capability_p->ReaderCapability);
                Capability_p->NumberOfMixerObjects++;
                break;

            default:
                break;
        }

    Unit_p = Unit_p->Next_p;
    }
    API_Trace((" <<< STAUD_GetCapability <<< Error %d \n",Error ));
    return (Error);

}


/*****************************************************************************
STAUD_DRConnectSource()

Description:
Connects an input source to an audio decoder device.
Parameters:
Handle          Handle to audio output.
DecoderObject   References a specific audio decoder.
InputSource     Input interface to connect to decoder device.



Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_BAD_PARAMETER      Invalid input source.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not available
with the current configuration.
STAUD_ERROR_DECODER_RUNNING The decoder was not in the appropiate state to
perform the connection
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRConnectSource (STAUD_Handle_t Handle,
                                      STAUD_Object_t DecoderObject,
                                      STAUD_Object_t InputSource)
{
    ST_ErrorCode_t	ErrorCode = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    API_Trace((" >>> STAUD_DRConnectSource >>> \n"));
    PrintSTAUD_DRConnectSource(Handle,DecoderObject,InputSource);

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRConnectSource <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);

    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRConnectSource <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }

    /* Feature not supported yet */
    API_Trace((" <<< STAUD_DRConnectSource <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n" ));
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;

    return ErrorCode;
}



/*****************************************************************************
STAUD_ConnectSrcDst()

Description:
Connects an input source to an destination Object.
Parameters:
Handle          Handle to audio output.
SrcObject       References a specific producer object.
SrcId				 Producer Id
DestObject   	 References a specific consumer object.
DstId				 Consumer Id

Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_BAD_PARAMETER      Invalid input source.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

STAUD_ERROR_INVALID_DEVICE  The audio driver is unrecognised or not available
with the current configuration.
STAUD_ERROR_INVALID_STATE 	The audio driver is not in the appropiate state to
perform the connection
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_ConnectSrcDst (STAUD_Handle_t Handle,
													STAUD_Object_t SrcObject,U32 SrcId,
													STAUD_Object_t DestObject,U32 DstId)
{
    ST_ErrorCode_t	ErrorCode = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    API_Trace((" >>> STAUD_ConnectSrcDst >>> \n"));
    PrintSTAUD_ConnectSrcDst(Handle,SrcObject,SrcId,DestObject,DstId);

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_ConnectSrcDst <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);

    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p)
    {
        STMEMBLK_Handle_t BMHandle = 0; /*BM handle from Producer*/
        STAUD_Handle_t Src_Handle = 0;
        STAUD_Handle_t Dst_Handle = 0;

        Src_Handle = GetHandle(drv_p,	SrcObject);
        Dst_Handle = GetHandle(drv_p,	DestObject);

        if(Src_Handle && Dst_Handle)
        {
            switch(SrcObject)
            {
                case STAUD_OBJECT_DECODER_COMPRESSED0:
                case STAUD_OBJECT_DECODER_COMPRESSED1:
                case STAUD_OBJECT_DECODER_COMPRESSED2:
                    ErrorCode |= STAud_DecGetOPBMHandle(Src_Handle, SrcId, &BMHandle);
                    break;
                case STAUD_OBJECT_MIXER0:
                    ErrorCode |= STAud_MxGetOPBMHandle(Src_Handle, SrcId, &BMHandle);
                    break;
                case STAUD_OBJECT_POST_PROCESSOR0:
                case STAUD_OBJECT_POST_PROCESSOR1:
                case STAUD_OBJECT_POST_PROCESSOR2:
                    ErrorCode |= STAud_PPGetOPBMHandle(Src_Handle, SrcId, &BMHandle);
                    break;
                default:
                    ErrorCode |= ST_ERROR_INVALID_HANDLE;
                break;
            }

            if(BMHandle == 0)
            {
            return ErrorCode;
            }

            switch(DestObject)
            {
                case STAUD_OBJECT_DECODER_COMPRESSED0:
                case STAUD_OBJECT_DECODER_COMPRESSED1:
                case STAUD_OBJECT_DECODER_COMPRESSED2:
                    ErrorCode |= STAud_DecSetIPBMHandle(Dst_Handle, DstId, BMHandle);
                    break;
                case STAUD_OBJECT_MIXER0:
                    ErrorCode |= STAud_MxSetIPBMHandle(Dst_Handle, DstId, BMHandle);
                    break;
                case STAUD_OBJECT_POST_PROCESSOR0:
                case STAUD_OBJECT_POST_PROCESSOR1:
                case STAUD_OBJECT_POST_PROCESSOR2:
                    ErrorCode |= STAud_PPSetIPBMHandle(Dst_Handle, DstId, BMHandle);
                    break;
                default:
                    ErrorCode |= ST_ERROR_INVALID_HANDLE;
                    break;
            }
        }
        else
        {
            ErrorCode = ST_ERROR_INVALID_HANDLE;
        }
    }
    else
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_ConnectSrcDst <<< Error %d \n",ErrorCode));
    return ErrorCode;

}

/*****************************************************************************
STAUD_DisconnectInput()

Description:
Disconnects an output Object from an Consumer .
Parameters:
Handle          Handle to audio output.
Object   	 References a specific consumer object.


Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_BAD_PARAMETER      Invalid input source.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

STAUD_ERROR_INVALID_DEVICE  The audio driver is unrecognised or not available
with the current configuration.
STAUD_ERROR_INVALID_STATE 	The audio driver is not in the appropiate state to
perform the connection
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DisconnectInput(STAUD_Handle_t Handle,
                                      STAUD_Object_t Object,U32 InputId)
{
    ST_ErrorCode_t	ErrorCode = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    API_Trace((" >>> STAUD_DisconnectInput >>> \n"));
    PrintSTAUD_DisconnectInput(Handle,Object,InputId);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DisconnectInput <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);

    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p)
    {
        STMEMBLK_Handle_t BMHandle = 0;
        STAUD_Handle_t ObjHandle = 0;

        ObjHandle = GetHandle(drv_p, Object);

        if(Handle)
        {
            switch(Object)
            {
                case STAUD_OBJECT_DECODER_COMPRESSED0:
                case STAUD_OBJECT_DECODER_COMPRESSED1:
                case STAUD_OBJECT_DECODER_COMPRESSED2:
                    ErrorCode |= STAud_DecSetIPBMHandle(ObjHandle, InputId, BMHandle);
                    break;
                case STAUD_OBJECT_MIXER0:
                    ErrorCode |= STAud_MxSetIPBMHandle(ObjHandle, InputId, BMHandle);
                    break;
                case STAUD_OBJECT_POST_PROCESSOR0:
                case STAUD_OBJECT_POST_PROCESSOR1:
                case STAUD_OBJECT_POST_PROCESSOR2:
                    ErrorCode |= STAud_PPSetIPBMHandle(ObjHandle, InputId, BMHandle);
                    break;
                default:
                    ErrorCode |= ST_ERROR_INVALID_HANDLE;
                    break;
            }
        }
        else
        {
            ErrorCode = ST_ERROR_INVALID_HANDLE;
        }
    }
    else
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_DisconnectInput <<< Error %d \n",ErrorCode ));
    return ErrorCode;
}


/*****************************************************************************
STAUD_PPDisableDownsampling()
Description:
Disable downsampling filter.
When decoding LPCM streams it is possible to downsample streams at certain sampling
frequencies. Calling this function disables the downsampling filter so the sampling frequency
of the output PCM data will be the same as the sampling frequency of the input stream.

Parameters:

Handle           Handle to audio decoder.

PostProcObject    References a specific audio PP.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      Downsampling is not supported by the decoder.

ST_ERROR_INVALID_HANDLE             Handle invalid or not open.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.

See Also:

*****************************************************************************/

ST_ErrorCode_t STAUD_PPDisableDownsampling(STAUD_Handle_t Handle,
											STAUD_Object_t PostProcObject)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPDisableDownsampling >>> \n"));
    PrintSTAUD_PPDisableDownsampling(Handle,PostProcObject);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPDisableDownsampling <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error=STAud_PPDisableDownSampling(Obj_Handle);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_PPDisableDownsampling <<< Error =%d \n",Error ));
    return Error;
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPDisableSynchronization >>> \n"));
    PrintSTAUD_OPDisableSynchronization(Handle,OutputObject);
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
        return ST_ERROR_INVALID_HANDLE;
    }


    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerAVSyncCmd(Obj_Handle,FALSE);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerAVSyncCmd(Obj_Handle,FALSE);
                break;
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error |= PESES_AVSyncCmd(Obj_Handle, FALSE);
            default:
                break;
        }

    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_OPDisableSynchronization <<< Error %d  \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_OPEnableHDMIOutput()
Description:
Enables output to HDMI interface.
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

*****************************************************************************/

ST_ErrorCode_t STAUD_OPEnableHDMIOutput (STAUD_Handle_t Handle,
											STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;

    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPEnableHDMIOutput >>> \n"));
    PrintSTAUD_OPEnableHDMIOutput(Handle,OutputObject);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPEnableHDMIOutput <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }


    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error |= STAud_PCMEnableHDMIOutput(Obj_Handle,TRUE);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error |= STAud_SPDIFEnableHDMIOutput(Obj_Handle,TRUE);
                break;

            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMEnableHDMIOutput(Obj_Handle,TRUE);
                if(Error == ST_NO_ERROR)
                {
#if defined (ST_7200)
                    Error |= STAUD_OPSetAnalogMode(Handle,STAUD_OBJECT_OUTPUT_HDMI_PCMP0,PCM_ON);
#endif
                      /*Removed Error updation to handle case when application use only one HDMI player in chain*/
		     STAUD_OPSetDigitalMode (Handle,STAUD_OBJECT_OUTPUT_HDMI_SPDIF0,STAUD_DIGITAL_MODE_OFF);

                }
                break;

            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFEnableHDMIOutput(Obj_Handle,TRUE);
                if(Error == ST_NO_ERROR)
                {
                    Error |= STAUD_OPSetDigitalMode (Handle,STAUD_OBJECT_OUTPUT_HDMI_SPDIF0,STAUD_DIGITAL_MODE_NONCOMPRESSED);
#if defined (ST_7200)
                          /*Removed Error updation to handle case when application use only one HDMI player in chain*/
				STAUD_OPSetAnalogMode(Handle,STAUD_OBJECT_OUTPUT_HDMI_PCMP0,PCM_OFF);

#endif
                }
                break;

            default:
                Error = ST_ERROR_BAD_PARAMETER;
                break;
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_OPEnableHDMIOutput <<< Error %d  \n",Error ));
    return Error;
}
/*****************************************************************************
STAUD_OPDisableHDMIOutput()
Description:
Disables output to HDMI interface.
Parameters:

Handle           Handle to audio decoder.

OutputObject    References a specific audio decoder.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      Not supported by the decoder.

ST_ERROR_INVALID_HANDLE             Handle invalid or not open.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:

*****************************************************************************/
ST_ErrorCode_t STAUD_OPDisableHDMIOutput (STAUD_Handle_t Handle,
											STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;

    STAUD_Handle_t Obj_Handle = 0;

    API_Trace((" >>> STAUD_OPDisableHDMIOutput >>> \n"));
    PrintSTAUD_OPDisableHDMIOutput(Handle,OutputObject);

    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPDisableHDMIOutput <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
        {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
                Error |= STAud_PCMEnableHDMIOutput(Obj_Handle,FALSE);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
                Error |= STAud_SPDIFEnableHDMIOutput(Obj_Handle,FALSE);
                break;

            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMEnableHDMIOutput(Obj_Handle,FALSE);
                if(Error == ST_NO_ERROR)
                {
#if defined (ST_7200)
                    Error |= STAUD_OPSetAnalogMode(Handle,STAUD_OBJECT_OUTPUT_HDMI_PCMP0,PCM_OFF);
#endif
                }
                break;

            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFEnableHDMIOutput(Obj_Handle,FALSE);
                if(Error == ST_NO_ERROR)
                {
                    Error |= STAUD_OPSetDigitalMode (Handle,STAUD_OBJECT_OUTPUT_HDMI_SPDIF0,STAUD_DIGITAL_MODE_OFF);
                }
                break;

            default:
                Error = ST_ERROR_BAD_PARAMETER;
                break;
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_OPDisableHDMIOutput <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_PPEnableDownsampling()
Description:
Enable downsampling filter.
When decoding LPCM streams it is possible to downsample streams at certain sampling
frequencies. Calling this function disables the downsampling filter so the sampling frequency
of the output PCM data will be the same as the sampling frequency of the input stream.

Parameters:

Handle           Handle to audio decoder.

DecoderObject    References a specific audio decoder.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      Downsampling is not supported by the decoder.

ST_ERROR_INVALID_HANDLE             Handle invalid or not open.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.

See Also:

*****************************************************************************/

ST_ErrorCode_t STAUD_PPEnableDownsampling(STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject, U32 OutPutFrequency)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPEnableDownsampling >>> \n"));
    PrintSTAUD_PPEnableDownsampling(Handle,PostProcObject,OutPutFrequency);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPEnableDownsampling <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error=STAud_PPEnableDownSampling(Obj_Handle,OutPutFrequency);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_PPEnableDownsampling <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPEnableSynchronization >>> \n"));
    PrintSTAUD_OPEnableSynchronization(Handle,OutputObject);

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


    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerAVSyncCmd(Obj_Handle,TRUE);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerAVSyncCmd(Obj_Handle,TRUE);
                break;
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:

            //Error |= PESES_AVSyncCmd(Obj_Handle,TRUE);
            default:
                break;
        }

    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_OPEnableSynchronization <<< Error %d \n",Error ));
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
    AudDriver_t *drv_p = NULL;
    STPESES_Handle_t Obj_Handle = 0;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    API_Trace((" >>> STAUD_IPGetBroadcastProfile >>> \n"));
    PrintSTAUD_IPGetBroadcastProfile(Handle,InputObject,BroadcastProfile_p);
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
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    STTBX_Print(("STAUD_DRGetBroadcastProfile\n"));
    API_Trace((" <<< STAUD_IPGetBroadcastProfile <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_DRGetCapability()
Description:
Returns the capabilities of a specific decoder object.

Parameters:

DeviceName      Instance of audio device.
DecoderObject   Specific decoder object to return capabilites for.
Capability_p    Pointer to returned capabilities.

Return Value:

ST_NO_ERROR                 No error.
ST_ERROR_UNKNOWN_DEVICE     The specified device is not initialized
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetCapability(const ST_DeviceName_t DeviceName,
												STAUD_Object_t DecoderObject,
												STAUD_DRCapability_t * Capability_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t	*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetCapability >>> \n"));
    PrintSTAUD_DRGetCapability(DeviceName, DecoderObject, Capability_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetCapability <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromName(DeviceName);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetCapability <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Capability_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecGetCapability(Obj_Handle, Capability_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetCapability <<< Error %d \n",Error ));
    return (Error);
}


/*****************************************************************************
STAUD_DRGetDynamicRangeControl()
Description:
Retrieves the current dynamic range control settings.
Parameters:
Handle          Handle to audio decoder.

DecoderObject   References a specific audio decoder.

DynamicRange_p  Stores the result of the current dynamic range control settings.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetDynamicRangeControl (STAUD_Handle_t Handle,
														STAUD_Object_t DecoderObject,
														STAUD_DynamicRange_t * DynamicRange_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetDynamicRangeControl >>> \n"));
    PrintSTAUD_DRGetDynamicRangeControl(Handle,DecoderObject,DynamicRange_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetDynamicRangeControl <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetDynamicRangeControl <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (DynamicRange_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetDynamicRange(Obj_Handle,DynamicRange_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    //Error = STAud_DecPPGetDynamicRange(DecoderHandle,DynamicRange_p);
    API_Trace((" <<< STAUD_DRGetDynamicRangeControl <<< Error %d  \n",Error ));
    return Error;

}


/*****************************************************************************
STAUD_DRGetDownMix()
Description:
Retrieves the current downmix settings.
Parameters:
Handle          Handle to audio decoder.

DecoderObject   References a specific audio decoder.

DownMixParam_p  Stores the result of the current downmix settings.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetDownMix (STAUD_Handle_t Handle,
														STAUD_Object_t DecoderObject,
														STAUD_DownmixParams_t * DownMixParam_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetDownMix >>> \n"));
    PrintSTAUD_DRGetDownMix(Handle,DecoderObject,DownMixParam_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetDownMix <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetDownMix <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (DownMixParam_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetDownMix <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetDownMix(Obj_Handle,DownMixParam_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_DRGetDownMix <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetSamplingFrequency >>> \n"));
    PrintSTAUD_IPGetSamplingFrequency(Handle,InputObject,SamplingFrequency_p);

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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = PESES_GetSamplingFreq(Obj_Handle,SamplingFrequency_p);
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
    API_Trace((" <<< STAUD_IPGetSamplingFrequency <<< Error %d \n",Error ));
    return Error;

}


/*****************************************************************************
STAUD_DRGetSpeed()
-Description:
Gets the current decode speed and direction.

-Parameters:
Handle          Handle to audio decoder.

DecoderObject   References a specific audio decoder.

Speed_p         Stores the value representing the current speed and direction of
decoding.

-Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     The handle is not valid.

STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not avail-able
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetSpeed (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject, S32 * Speed_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t *drv_p = NULL;
    API_Trace((" >>> STAUD_DRGetSpeed >>> \n"));
    PrintSTAUD_DRGetSpeed(Handle,DecoderObject,Speed_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetSpeed <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetSpeed <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }

    if(Speed_p==NULL)
    {
        API_Trace((" <<< STAUD_DRGetSpeed <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return (ST_ERROR_BAD_PARAMETER);
    }


    *Speed_p	= drv_p->Speed;

    API_Trace((" <<< STAUD_DRGetSpeed <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetStreamInfo >>> \n"));
    PrintSTAUD_IPGetStreamInfo(Handle,InputObject,StreamInfo_p);

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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = PESES_GetStreamInfo(Obj_Handle,StreamInfo_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Parser Get Stream Info failed\n"));
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPGetStreamInfo <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetBitBufferFreeSize >>> \n"));
    PrintSTAUD_IPGetBitBufferFreeSize(Handle,InputObject,Size_p);

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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = PESES_GetBitBufferFreeSize(Obj_Handle, Size_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Parser Get Stream Info failed\n"));
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPGetBitBufferFreeSize <<< Error %d  \n",Error ));
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t *drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;
    API_Trace((" >>> STAUD_DRGetSyncOffset >>> \n"));
    PrintSTAUD_DRGetSyncOffset(Handle,DecoderObject,Offset_p);
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
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error = STAud_PCMPlayerGetOffset(Obj_Handle, Offset_p);
                break;
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
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

    API_Trace((" <<< STAUD_DRGetSyncOffset <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRPause >>> \n"));
    PrintSTAUD_DRPause(Handle,InputObject,Fade_p);
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error = SendPES_ES_ParseCmd(Obj_Handle,TIMER_TRANSFER_PAUSE);
                break;
            case STAUD_OBJECT_INPUT_PCMREADER0:
                Error = STAud_PCMReaderSetCmd(Obj_Handle,PCMREADER_PAUSE);
                break;
            default:
                break;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRPause <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_DRPrepare >>> \n"));
    PrintSTAUD_DRPrepare(Handle,DecoderObject,StreamParams_p);
    API_Trace((" <<< STAUD_DRPrepare <<<  Error  ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return Error;
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRResume >>> \n"));
    PrintSTAUD_DRResume(Handle,InputObject);
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error = SendPES_ES_ParseCmd(Obj_Handle,TIMER_TRANSFER_RESUME);
                break;
            case STAUD_OBJECT_INPUT_PCMREADER0:
                Error = STAud_PCMReaderSetCmd(Obj_Handle,PCMREADER_START);
                break;
            default:
                break;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    /*Notify that the driver has resumed*/
    Error |= STAudEVT_Notify(drv_p->EvtHandle, drv_p->EventIDResumed, NULL, 0, 0);
    API_Trace((" <<< STAUD_DRResume <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STPESES_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetBroadcastProfile >>> \n"));
    PrintSTAUD_IPSetBroadcastProfile(Handle,InputObject,BroadcastProfile);
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
        if((BroadcastProfile==STAUD_BROADCAST_DVB)||(BroadcastProfile==STAUD_BROADCAST_DIRECTV))
        {
            Error = PESES_SetBroadcastProfile(Obj_Handle, BroadcastProfile);
        }
        else
        {
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPSetBroadcastProfile <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STPESES_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPHandleEOF >>> \n"));
    PrintSTAUD_IPHandleEOF(Handle,InputObject);
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
            case STAUD_OBJECT_INPUT_CD2:
                Error = PESES_EnableEOFHandling(Obj_Handle);
                break;
            case STAUD_OBJECT_INPUT_PCM0:
                Error = STAud_PCMInputHandleEOF(Obj_Handle);
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
    API_Trace((" <<< STAUD_IPHandleEOF <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRGetBeepToneFrequency()
Description:
Retrieves the current beep tone freq.
Parameters:
Handle          Handle to audio decoder.

DecoderObject   References a specific audio decoder.

BeepToneFreq_p  Stores the result of the current beep tone freq.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetBeepToneFrequency (STAUD_Handle_t Handle,
														STAUD_Object_t DecoderObject,
														U32 * BeepToneFreq_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetBeepToneFrequency >>> \n"));
    PrintSTAUD_DRGetBeepToneFrequency(Handle,DecoderObject,BeepToneFreq_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetBeepToneFrequency <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetBeepToneFrequency <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (BeepToneFreq_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetBeepToneFrequency <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        ST_ErrorCode_t Temp;
        Temp = STAud_DecGetBeepToneFreq(Obj_Handle, BeepToneFreq_p );
        if(Temp == ST_ERROR_INVALID_HANDLE)
        {
            Error = Temp;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_DRGetBeepToneFrequency <<< Error %d \n",Error ));

    return Error;

}

/*****************************************************************************
STAUD_DRSetBeepToneFrequency()

Description:
If possible sets Beep Tone Frequency.

Parameters:
Handle             Handle to audio decoder.

DecoderObject      References a specific audio decoder.

Frequency			Beep tone Frequency

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetBeepToneFrequency (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,U32 BeepToneFrequency)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetBeepToneFrequency >>> \n"));
    PrintSTAUD_DRSetBeepToneFrequency(Handle,DecoderObject,BeepToneFrequency);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetBeepToneFrequency <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetBeepToneFrequency <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecSetBeepToneFreq(Obj_Handle,BeepToneFrequency);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetBeepToneFrequency <<< Error %d \n",Error ));
    return Error;
}



/*****************************************************************************
STAUD_DRSetDynamicRangeControl()

Description:
If possible sets the Dynamic Range Control. Only valid for compressed decoders.
And particulary to MPEG and AC3 stream

Parameters:
Handle             Handle to audio decoder.

DecoderObject      References a specific audio decoder.

DynamicRange_p     Specifies the dynamic range control settings.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The audio decoder does not support dynamic
range control.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetDynamicRangeControl (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,STAUD_DynamicRange_t * DynamicRange_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetDynamicRangeControl >>> \n"));
    PrintSTAUD_DRSetDynamicRangeControl(Handle,DecoderObject,DynamicRange_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<<STAUD_DRSetDynamicRangeControl <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<<STAUD_DRSetDynamicRangeControl <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetDynamicRange(Obj_Handle,DynamicRange_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<<STAUD_DRSetDynamicRangeControl <<< Error %d  \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRSetDownMix()

Description:
If possible sets the downmix configuration on audio decoder

Parameters:
Handle             Handle to audio decoder.

DecoderObject      References a specific audio decoder.

DownMixParam_p     Specifies the downmix settings.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The audio decoder does not support downmixing

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetDownMix (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,STAUD_DownmixParams_t * DownMixParam_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetDownMix >>> \n"));
    PrintSTAUD_DRSetDownMix(Handle,DecoderObject,DownMixParam_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetDownMix <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetDownMix <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetDownMix(Obj_Handle,DownMixParam_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetDownMix <<< Error %d  \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRSetSpeed()
Description:
Sets the decode speed and direction.
Parameters:
Handle          Handle to audio decoder.
DecoderObject   References a specific audio decoder.
Speed           Enum type defining the possible speeds.
Return Value:
ST_NO_ERROR                     No error.
ST_ERROR_INVALID_HANDLE         The handle is not valid.
STAUD_ERROR_INVALID_DEVICE      The audio decoder is unrecognised or not available
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetSpeed (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject, S32 Speed)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;
    STAUD_Handle_t 	Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetSpeed >>> \n"));
    PrintSTAUD_DRSetSpeed(Handle,DecoderObject,Speed);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetSpeed <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetSpeed <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }


    if(Speed != drv_p->Speed)
    {
        ObjectCount = GetClassHandles(Handle, STAUD_CLASS_INPUT, ObjectArray);
        while (ObjectCount)
        {
            ObjectCount --;
            if(ObjectArray[ObjectCount] != STAUD_OBJECT_INPUT_PCM0)
            {
                Obj_Handle = GetHandle(drv_p,ObjectArray[ObjectCount]);
                if( ((drv_p->Speed>MAX_SPEED) || (drv_p->Speed<MIN_SPEED)) && ((Speed<=MAX_SPEED) && (Speed>=MIN_SPEED)) )
                    Error |= PESES_SetSpeed( Obj_Handle,NORMAL_SPEED);

                if ((Speed >MAX_SPEED) || (Speed < MIN_SPEED))
                    Error |= PESES_SetSpeed( Obj_Handle,Speed);		/*Set New Speed*/
            }
        }
        ObjectCount = GetClassHandles(Handle, STAUD_CLASS_DECODER, ObjectArray);
        while (ObjectCount)
        {
            ObjectCount --;
            Obj_Handle = GetHandle(drv_p,ObjectArray[ObjectCount]);

            if(Speed>=MIN_SPEED && Speed <= MAX_SPEED)
                Error = STAud_DecPPSetSpeed( Obj_Handle,Speed);
            else
            {
                Error = STAud_DecPPSetSpeed( Obj_Handle,NORMAL_SPEED);
            }
        }

        /*call to any other module if required*/
        /*Update the speed*/
        if (Error == ST_ERROR_FEATURE_NOT_SUPPORTED)
        {
            if (Speed >MAX_SPEED || Speed< MIN_SPEED)
                drv_p->Speed = Speed;
            if (Speed < MAX_SPEED && Speed > MIN_SPEED)
                drv_p->Speed = NORMAL_SPEED;
        }
        else
        {
            drv_p->Speed = Speed;
        }

    }
    API_Trace((" <<< STAUD_DRSetSpeed <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_DRGetSamplingFrequency()

-Description:
Retrieves the sampling frequency of the input data stream.

-Parameters:
Handle              Handle to audio driver.

DecoderObject       References a specific audio decoder.

SamplingFrequency_p Stores the result of the current sampling frequency.

-Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     The handle is not valid.

STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not avail-able
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetSamplingFrequency (STAUD_Handle_t Handle,
STAUD_Object_t InputObject, U32 * SamplingFrequency_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetSamplingFrequency >>> \n"));
    PrintSTAUD_DRGetSamplingFrequency(Handle,InputObject,SamplingFrequency_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetSamplingFrequency <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetSamplingFrequency <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = STAud_DecGetSamplingFrequency(Obj_Handle,SamplingFrequency_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Decoder Get Freq failed\n"));
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetSamplingFrequency <<< Error %d \n",Error ));
    return Error;

}



/*****************************************************************************
STAUD_DRSetInitParams()

-Description:
Set decoder specific parameters

-Parameters:
Handle              Handle to audio driver.

DecoderObject       References a specific audio decoder.

InitParams Values of init params to be set.

-Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     The handle is not valid.

STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not avail-able
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetInitParams (STAUD_Handle_t Handle,
STAUD_Object_t InputObject, STAUD_DRInitParams_t * InitParams)
{

    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetInitParams >>> \n"));
    PrintSTAUD_DRSetInitParams(Handle,InputObject,InitParams);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetInitParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetInitParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = STAud_DecSetInitParams(Obj_Handle,InitParams);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Decoder Set init params failed\n"));
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetInitParams <<< Error %d \n",Error));
    return Error;

}

/*****************************************************************************
STAUD_DRGetStreamInfo()

-Description:
Get Stream dependent information

-Parameters:
Handle              Handle to audio driver.

DecoderObject       References a specific audio decoder.

InitParams Values of init params to be set.

-Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     The handle is not valid.

STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not avail-able
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetStreamInfo (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject, STAUD_StreamInfo_t * StreamInfo_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetStreamInfo >>> \n"));
    PrintSTAUD_DRGetStreamInfo(Handle,DecoderObject,StreamInfo_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetStreamInfo <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetStreamInfo <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecGetStreamInfo(Obj_Handle,StreamInfo_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Decoder get stream info failed\n"));
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetStreamInfo <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetStreamID >>> \n"));
    PrintSTAUD_IPSetStreamID(Handle,InputObject,StreamID);
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = PESES_SetStreamID(Obj_Handle,(U32)StreamID);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Parser Get Freq failed\n"));
        }

    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPSetStreamID <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_IPGetSynchroUnit()

Description:
Retrieves the time unit precision for audio skip and pause synchronization functions.

Parameters:
Handle           Handle to audio decoder.
DecoderObject    References a specific audio decoder.
SynchroUnit_p    Stores the result of the current time unit precision for audio
synchronization   functions.

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.
STAUD_ERROR_INVALID_DEVICE     The audio decoder is unrecognised or not available
with the current configuration.
STAUD_ERROR_DECODER_STOPPED

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPGetSynchroUnit (STAUD_Handle_t Handle,
STAUD_Object_t InputObject, STAUD_SynchroUnit_t *SynchroUnit_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t *drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetSynchroUnit >>> \n"));
    PrintSTAUD_IPGetSynchroUnit(Handle,InputObject,SynchroUnit_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetSynchroUnit <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetSynchroUnit <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }

    Obj_Handle = GetHandle(drv_p,InputObject);

    STOS_TaskLock();

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error = PESES_GetSynchroUnit(Obj_Handle, SynchroUnit_p);
                break;
            default:
            Error =  ST_ERROR_INVALID_HANDLE;
            break;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    STOS_TaskUnlock();
    API_Trace((" <<< STAUD_IPGetSynchroUnit <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_IPSkipSynchro()
Description:
Skips the decode of audio frames for a specified time interval.

Parameters:
Handle         Handle to audio decoder.

DecoderObject  References a specific audio decoder.

Delay          Specifies a time delay in N ms intervals, where N is the pause time
precision returned by STAUD_DRGetSynchroUnit().

Return Value:
ST_NO_ERROR                    No error.

ST_ERROR_INVALID_HANDLE        The handle is not valid.

STAUD_ERROR_DECODER_PAUSING     Decoder is paused.

STAUD_ERROR_DECODER_STOPPED    Decoder is stopped.

STAUD_ERROR_INVALID_DEVICE     The audio decoder is unrecognised or not avail-able
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPSkipSynchro (STAUD_Handle_t Handle,
STAUD_Object_t InputObject, U32 Delay)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSkipSynchro >>> \n"));
    PrintSTAUD_IPSkipSynchro(Handle, InputObject, Delay);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPSkipSynchro <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSkipSynchro <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,InputObject);

    STOS_TaskLock();

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error = PESES_SkipSynchro(Obj_Handle, Delay);
                break;
            default:
                Error =  ST_ERROR_INVALID_HANDLE;
                break;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    STOS_TaskUnlock();
    API_Trace((" <<< STAUD_IPSkipSynchro <<< Error %d \n",Error));
    return Error;
}

/*****************************************************************************
STAUD_IPPauseSynchro()

Description:
Pauses the audio decoder for a specified time interval.
Parameters:
Handle          Handle to audio decoder.

DecoderObject   References a specific audio decoder.

Delay           Specifies a time delay in N ms intervals, where N is the pause time
precision returned by STAUD_DRGetSynchroUnit().
Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_DECODER_PAUSING      Decoder is paused.

STAUD_ERROR_DECODER_STOPPED     Decoder is stopped.

STAUD_ERROR_INVALID_DEVICE      The audio decoder is unrecognised or not avai-able
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPPauseSynchro (STAUD_Handle_t Handle,
STAUD_Object_t InputObject, U32 Delay)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPPauseSynchro >>> \n"));
    PrintSTAUD_IPPauseSynchro(Handle,InputObject,Delay);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPPauseSynchro <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPPauseSynchro <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,InputObject);

    STOS_TaskLock();

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error = PESES_PauseSynchro(Obj_Handle, Delay);
                break;
            default:
                Error =  ST_ERROR_INVALID_HANDLE;
                break;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    STOS_TaskUnlock();
    API_Trace((" <<< STAUD_IPPauseSynchro <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_IPSetWMAStreamID()

Description:
Sets the stream ID for the WMA stream to be played
Parameters:
Handle          Handle to audio decoder.

InputObject		References a specific WMA Parser.

WMAStreamNumber Specifies stream ID for the WMA stream to be played
Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_INVALID_STREAM_TYPE Stream type is not valid

STAUD_ERROR_INVALID_DEVICE      The audio decoder is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_IPSetWMAStreamID(STAUD_Handle_t Handle,
STAUD_Object_t InputObject, U8 WMAStreamNumber)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetWMAStreamID >>> \n"));
    PrintSTAUD_IPSetWMAStreamID(Handle,InputObject,WMAStreamNumber);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPSetWMAStreamID <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSetWMAStreamID <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,InputObject);

    STOS_TaskLock();

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error = PESES_SetWMAStreamID(Obj_Handle, WMAStreamNumber);
                break;
            default:
                Error =  ST_ERROR_INVALID_HANDLE;
                break;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    STOS_TaskUnlock();
    API_Trace((" <<< STAUD_IPSetWMAStreamID <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetClockRecoverySource >>> \n"));
    PrintSTAUD_DRSetClockRecoverySource(Handle,Object,ClkSource);
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

    Obj_Handle = GetHandle(drv_p,	Object);

    if (Obj_Handle)
    {
        switch(Object)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error |= PESES_SetClkSynchro(Obj_Handle, ClkSource);
                break;
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerSetClkSynchro(Obj_Handle,ClkSource);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerSetClkSynchro(Obj_Handle,ClkSource);
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
    API_Trace((" <<< STAUD_DRSetClockRecoverySource <<< Error %d \n",Error ));
    return Error;
}
#endif // Added Because of 8010 //


/*****************************************************************************
STAUD_OPSetEncoderDigitalOutput()

Description:
Enables/Disables enc output on SDPIF in compressed mode.
This function should be called in conjunction with STAUD_OPSetDigitalMode().

Parameters:
Handle				Handle to audio decoder.
OutputObject		References a specific spdif player.
EnableDisableEncOutput Bool to enable/disable enc output

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_OPSetEncDigitalOutput(const STAUD_Handle_t Handle,
            STAUD_Object_t OutputObject,
            BOOL EnableDisableEncOutput)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPSetEncDigitalOutput  >>> \n"));
    PrintSTAUD_OPSetEncDigitalOutput(Handle,OutputObject,EnableDisableEncOutput);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPSetEncDigitalOutput <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPSetEncDigitalOutput <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerSetEncOutput(Obj_Handle, EnableDisableEncOutput);
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
    API_Trace((" <<< STAUD_OPSetEncDigitalOutput <<< Error %d \n",Error ));
    return Error;
}

#if defined(ST_7200)

/*****************************************************************************
STAUD_OPSetAnalogMode()

Description:
Sets the analog output mode for a particular pcm player
Parameters:
Handle				Handle to audio decoder.
OutputObject		References a specific spdif player.
STAUD_PCMMode_t   OutputMode.

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_OPSetAnalogMode (STAUD_Handle_t Handle,STAUD_Object_t OutputObject,
                                                                                STAUD_PCMMode_t OutputMode)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPSetAnalogMode >>> \n"));
    PrintSTAUD_OPSetAnalogMode(Handle,OutputObject,OutputMode);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPSetAnalogMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPSetAnalogMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerSetAnalogMode(Obj_Handle, OutputMode);
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
    API_Trace((" <<< STAUD_OPSetAnalogMode <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_OPGetAnalogMode()

Description:
Gets the analog output mode for a particular PCM player.
Parameters:
Handle				Handle to audio decoder.
OutputObject		References a specific spdif player.
STAUD_PCMMode_t   OutputMode.

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_OPGetAnalogMode (STAUD_Handle_t Handle,STAUD_Object_t OutputObject,
                                                                                STAUD_PCMMode_t * OutputMode)
{

    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPGetAnalogMode >>> \n"));
    PrintSTAUD_OPGetAnalogMode(Handle,OutputObject,OutputMode);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPGetAnalogMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPGetAnalogMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerGetAnalogMode(Obj_Handle, OutputMode);
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
    API_Trace((" <<< STAUD_OPGetAnalogMode <<< Error %d \n",Error ));
    return Error;
}

#endif


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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPSetDigitalMode >>> \n"));
    PrintSTAUD_OPSetDigitalMode(Handle,OutputObject,OutputMode);
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

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerSetSPDIFMode(Obj_Handle, OutputMode);
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
    API_Trace((" <<< STAUD_OPSetDigitalMode <<< Error %d  \n",Error ));
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPGetDigitalMode >>> \n"));
    PrintSTAUD_OPGetDigitalMode(Handle,OutputObject,OutputMode);
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

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if (Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerGetSPDIFMode(Obj_Handle, OutputMode);
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
    API_Trace((" <<< STAUD_OPGetDigitalMode <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t *drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;
    API_Trace((" >>> STAUD_DRSetSyncOffset >>> \n"));
    PrintSTAUD_DRSetSyncOffset(Handle,DecoderObject,Offset);
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
        return (ST_ERROR_INVALID_HANDLE);
    }

    ObjectCount = GetClassHandles(Handle,	STAUD_CLASS_OUTPUT, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount--;
        Obj_Handle = GetHandle(drv_p, ObjectArray[ObjectCount]);
        switch(ObjectArray[ObjectCount])
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error = STAud_PCMPlayerSetOffset(Obj_Handle,Offset);
                break;
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error = STAud_SPDIFPlayerSetOffset(Obj_Handle,Offset);
                break;
            default:
                Error = ST_ERROR_INVALID_HANDLE;
                break;
        }
    }
    API_Trace((" <<< STAUD_DRSetSyncOffset <<< Error %d \n",Error ));
    return Error;
}



/*****************************************************************************
STAUD_OPSetLatency()

Description:
Sets the offset to be applied in the audio sync manager.
Offset is in milliseconds.

Parameters:
Handle           Handle to audio decoder.
OutputObject    References a specific audio output.
Latency           Offset to be applied.

Return Value:
ST_NO_ERROR                    No error.
ST_ERROR_INVALID_HANDLE        The handle is not valid.
STAUD_ERROR_INVALID_DEVICE     The audio decoder is unrecognised or not available
with the current configuration.
ST_FEATURE_NOT_SUPPORTED       The referenced decoder does not support sync offsets

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_OPSetLatency (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject, U32 Latency)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t *drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPSetLatency >>> \n"));
    PrintSTAUD_OPSetLatency(Handle,OutputObject,Latency);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_OPSetLatency <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_OPSetLatency <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return (ST_ERROR_INVALID_HANDLE);
    }

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    /*Set Latency*/

    if(Obj_Handle)
    {
        switch(OutputObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerSetLatency(Obj_Handle,Latency);
                break;
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerSetLatency(Obj_Handle,Latency);
                break;
            default:
                break;
        }

    }
    API_Trace((" <<< STAUD_OPSetLatency <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    STAUD_StreamParams_t TempStreamParams_p;
#if STFAE_HARDCODE
    STAUD_DrvChainConstituent_t *TempUnit_p = NULL;
    STAUD_DigitalMode_t	SPDIFMode;
    BOOL				StartEncoder;
#endif
    API_Trace((" >>> STAUD_DRStart >>> \n"));
    PrintSTAUD_DRStart(Handle,DecoderObject,StreamParams_p);
    if(
    !( 0
#ifdef STAUD_INSTALL_AC3
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_AC3)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_DDPLUS)
#endif
#ifdef STAUD_INSTALL_MPEG
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG1)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG2)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MP3)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG_AAC)
#endif
#ifdef STAUD_INSTALL_HEAAC
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_HE_AAC)
#endif
#if defined(STAUD_INSTALL_LPCMV) || defined(STAUD_INSTALL_LPCMA)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_LPCM)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_LPCM_DVDA)
#endif
#ifdef STAUD_INSTALL_DTS
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_DTS)
#endif
#ifdef STAUD_INSTALL_CDDA
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_CDDA)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_PCM)
#endif
#ifdef STAUD_INSTALL_WAVE
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_WAVE)
#endif
#ifdef STAUD_INSTALL_AIFF
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_AIFF)
#endif
#ifdef STAUD_INSTALL_MLP
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MLP)
#endif
#ifdef STAUD_INSTALL_WMA
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_WMA)
#endif
#ifdef STAUD_INSTALL_WMAPROLSL
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_WMAPROLSL)
#endif

#ifdef STAUD_INSTALL_ALSA
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_ALSA)
#endif
#ifdef STAUD_INSTALL_AAC
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_ADIF)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MP4_FILE)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_RAW_AAC)
#endif
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_BEEP_TONE)

    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_PINK_NOISE)
    ))
    {
        STTBX_Print(("STAUD_DRStart wrong config"));
        API_Trace((" <<< STAUD_DRStart <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }
#ifdef ST_OSLINUX
    #if _ACC_WRAPP_MME_
    acc_mme_logreset();
    #endif
#endif
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
#if STFAE_HARDCODE
    TempUnit_p = drv_p->Handle;
    StartEncoder = FALSE;
#endif
    while(Unit_p)
    {
        switch(Unit_p->Object_Instance)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle,PCMPLAYER_START);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCMPlayer start failed\n"));
                }
                break;
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                TempStreamParams_p.StreamID          = StreamParams_p->StreamID;
                TempStreamParams_p.StreamType        = StreamParams_p->StreamType;
                TempStreamParams_p.StreamContent     = StreamParams_p->StreamContent;
                TempStreamParams_p.SamplingFrequency = StreamParams_p->SamplingFrequency;

#if STFAE_HARDCODE

                STAud_SPDIFPlayerSetEncOutput(GetHandle(drv_p, STAUD_OBJECT_OUTPUT_SPDIF0), FALSE);
                STAud_SPDIFPlayerGetSPDIFMode(GetHandle(drv_p, STAUD_OBJECT_OUTPUT_SPDIF0), &SPDIFMode);
                if((SPDIFMode == STAUD_DIGITAL_MODE_COMPRESSED) && ((StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_HE_AAC)
                ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG_AAC)))
                {

                    while(TempUnit_p)
                    {
                        if (TempUnit_p->Object_Instance == STAUD_OBJECT_ENCODER_COMPRESSED0)
                        {
                            STAud_SPDIFPlayerSetEncOutput(GetHandle(drv_p, STAUD_OBJECT_OUTPUT_SPDIF0), TRUE);
                            TempStreamParams_p.StreamType        = STAUD_STREAM_TYPE_ES;
                            TempStreamParams_p.StreamContent     = STAUD_STREAM_CONTENT_DTS;
                            StartEncoder = TRUE;
                            break;
                        }
                            TempUnit_p = TempUnit_p->Next_p;
                    }

                }
#endif
                Error |= STAud_SPDIFPlayerSetStreamParams(Unit_p->Handle, &TempStreamParams_p);
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

            case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
            case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
                Error |= STAud_DataProcessorSetStreamParams(Unit_p->Handle, StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Data Processor set failed\n"));
                }
                Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_GET_INPUT_BLOCK);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("DataFormatter Start failed\n"));
                }
                break;
            case STAUD_OBJECT_FRAME_PROCESSOR:
            case STAUD_OBJECT_FRAME_PROCESSOR1:
#ifdef STAUDLX_ALSA_SUPPORT
                if(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_ALSA)
                    Error |= STAud_DataProcessorSetStreamParams(Unit_p->Handle, StreamParams_p);
 #endif

                Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_GET_INPUT_BLOCK);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("DataFormatter Start failed\n"));
                }
                break;
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error |= PESESSetAudioType(Unit_p->Handle,StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PESESS set failed\n"));
                }

                if(Error == ST_NO_ERROR)
                {
                    Error |= SendPES_ES_ParseCmd(Unit_p->Handle,TIMER_TRANSFER_START);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("PESESS Start failed\n"));
                    }
                }
                break;
            case STAUD_OBJECT_INPUT_PCM0:
                Error |= STAud_StartPCMInput(Unit_p->Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Input PCM Start failed\n"));
                }
                break;
            case STAUD_OBJECT_INPUT_PCMREADER0:
                Error |= STAud_PCMReaderSetCmd(Unit_p->Handle, PCMREADER_START);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCM READER start failed\n"));
                }
                break;
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_DECODER_COMPRESSED2:
                Error |= STAud_DecSetStreamParams(Unit_p->Handle,StreamParams_p);
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

            case STAUD_OBJECT_ENCODER_COMPRESSED0:

                TempStreamParams_p.StreamID          = StreamParams_p->StreamID;
                TempStreamParams_p.StreamType        = StreamParams_p->StreamType;
                TempStreamParams_p.StreamContent     = StreamParams_p->StreamContent;
                TempStreamParams_p.SamplingFrequency = StreamParams_p->SamplingFrequency;

#if STFAE_HARDCODE
                if((StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_HE_AAC) || (StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG_AAC))
                {
                    TempStreamParams_p.StreamType        = STAUD_STREAM_TYPE_ES;
                    TempStreamParams_p.StreamContent     = STAUD_STREAM_CONTENT_DTS;
                }

#endif
                Error |= STAud_EncSetStreamParams(Unit_p->Handle,&TempStreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Encoder Set failed\n"));
                }

                {
                    BOOL EnableEncOutput = FALSE;

                    STAud_SPDIFPlayerGetEncOutput(GetHandle(drv_p, STAUD_OBJECT_OUTPUT_SPDIF0), &EnableEncOutput);
                    if (EnableEncOutput == TRUE)
                    {
                        STAUD_Handle_t Obj_Handle = 0;
                        STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
                        U32 ObjectCount = 0;

                        ObjectCount = GetClassHandles(Handle,	STAUD_CLASS_POSTPROC, ObjectArray);

                        while(ObjectCount)
                        {
                            ObjectCount--;
                            Obj_Handle = GetHandle(drv_p, ObjectArray[ObjectCount]);
                            switch(ObjectArray[ObjectCount])
                            {
                                case STAUD_OBJECT_POST_PROCESSOR0:
                                case STAUD_OBJECT_POST_PROCESSOR1:
                                case STAUD_OBJECT_POST_PROCESSOR2:
                                    Error = STAud_PPSetOutputStreamType(Obj_Handle, TempStreamParams_p.StreamContent);
                                    break;
                                default:
                                    Error = ST_ERROR_INVALID_HANDLE;
                                    break;

                            }
                        }

                    }
                }
#if STFAE_HARDCODE
                if(StartEncoder)
                    Error |= STAud_EncStart(Unit_p->Handle);
#else
                Error |= STAud_EncStart(Unit_p->Handle);
#endif
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Encoder Start failed\n"));
                }
                break;

            case STAUD_OBJECT_POST_PROCESSOR0:
            {
                STAud_PPStartParams_t StartParams;
                memset(&StartParams,0,sizeof(STAud_PPStartParams_t));
                StartParams.Frequency= StreamParams_p->SamplingFrequency;
                Error |= STAud_PPStart(Unit_p->Handle, &StartParams);
            }
            break;
            case STAUD_OBJECT_POST_PROCESSOR1:
            {
                STAud_PPStartParams_t StartParams;
                memset(&StartParams,0,sizeof(STAud_PPStartParams_t));
                StartParams.Frequency= StreamParams_p->SamplingFrequency;
                Error |= STAud_PPStart(Unit_p->Handle, &StartParams);
            }
            break;

            case STAUD_OBJECT_POST_PROCESSOR2:
            {
                STAud_PPStartParams_t StartParams;
                memset(&StartParams,0,sizeof(STAud_PPStartParams_t));
                StartParams.Frequency= StreamParams_p->SamplingFrequency;

                Error |= STAud_PPStart(Unit_p->Handle, &StartParams);
            }
            break;

            case STAUD_OBJECT_MIXER0:
                Error |= STAud_MixerSetCmd(Unit_p->Handle, MIXER_STATE_START);
                if(Error != ST_NO_ERROR)
                {

                }
                break;
            default:
            /*Must be memory */
                Error |= MemPool_Start(Unit_p->Handle); /*Linear buffer pool*/
                if (Error != ST_NO_ERROR)
                {
                STTBX_Print(("MemPool Start failed\n"));
                }
                break;
            break;
        }
        Unit_p = Unit_p->Next_p;
    }


    audHandle = Handle;
    audStreamParams = *(StreamParams_p);
#if STFAE_HARDCODE
    drv_p->StartEncoder = StartEncoder;
#endif
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

    ST_ErrorCode_t					Error	= ST_NO_ERROR;
    AudDriver_t						*drv_p	= NULL;
    STAUD_DrvChainConstituent_t		*Unit_p = NULL;
#if STFAE_HARDCODE
    BOOL							StartEncoder	= FALSE;
#endif
    API_Trace((" >>> STAUD_DRStop >>> \n"));
    PrintSTAUD_DRStop(Handle,DecoderObject,StopMode,Fade_p);
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
#if STFAE_HARDCODE
    StartEncoder = drv_p->StartEncoder;
#endif
    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        switch(Unit_p->Object_Instance)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle,PCMPLAYER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCMPlayer Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerSetCmd(Unit_p->Handle, SPDIFPLAYER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("SPDIFPlayer Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
            case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
                Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("DataFormatter Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_FRAME_PROCESSOR:
            case STAUD_OBJECT_FRAME_PROCESSOR1:
                Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("DataFormatter Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error |= SendPES_ES_ParseCmd(Unit_p->Handle,TIMER_STOP);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PESES Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_INPUT_PCM0:
                Error |= STAud_StopPCMInput(Unit_p->Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Input PCM Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_INPUT_PCMREADER0:
                Error |= STAud_PCMReaderSetCmd(Unit_p->Handle, PCMREADER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCM READER stop failed\n"));
                }
                break;
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_DECODER_COMPRESSED2:
                Error |= STAud_DecStop(Unit_p->Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Decoder Stop failed\n"));
                }
                break;

            case STAUD_OBJECT_ENCODER_COMPRESSED0:
#if STFAE_HARDCODE
                if(StartEncoder)
                    Error |= STAud_EncStop(Unit_p->Handle);
#else
                Error |= STAud_EncStop(Unit_p->Handle);
#endif
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Decoder Stop failed\n"));
                }
                break;

            case STAUD_OBJECT_MIXER0:
                Error |= STAud_MixerSetCmd(Unit_p->Handle, MIXER_STATE_STOP);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Mixer Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_POST_PROCESSOR0:
                Error |= STAud_PPStop(Unit_p->Handle);
                break;

            case STAUD_OBJECT_POST_PROCESSOR1:
                Error |= STAud_PPStop(Unit_p->Handle);
                break;

            case STAUD_OBJECT_POST_PROCESSOR2:
                Error |= STAud_PPStop(Unit_p->Handle);
                break;

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
    API_Trace((" <<< STAUD_DRStop <<< Error %d \n",Error ));
    return Error;

}

/*****************************************************************************
STAUD_GetInputBufferParams()
Description:
Parameters:
Return Value:
See Also:
******************************************************************************/

ST_ErrorCode_t STAUD_IPGetInputBufferParams(STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_BufferParams_t* DataParams_p)
{
    ST_ErrorCode_t	Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetInputBufferParams >>> \n "));
    PrintSTAUD_IPGetInputBufferParams(Handle,InputObject,DataParams_p);
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        switch(InputObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error = PESES_GetInputBufferParams(Obj_Handle, DataParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Parser Get Input buffer Params failed\n"));
                }
                break;
            case STAUD_OBJECT_INPUT_PCMREADER0:
                Error = STAud_PCMReaderGetBufferParams(Obj_Handle, DataParams_p);
                break;
            case STAUD_OBJECT_INPUT_PCM0:
                Error = STAud_PCMInputBufferParams(Obj_Handle, DataParams_p);
                break;
            default:
                break;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }

    STTBX_Print((" STAUD_IPGetInputBufferParams Add = %x \n", (U32)DataParams_p->BufferBaseAddr_p));
#ifdef ST_OSLINUX
   // DataParams_p->BufferBaseAddr_p = (void*)(STOS_VirtToPhys(DataParams_p->BufferBaseAddr_p));
#endif
    STTBX_Print((" STAUD_IPGetInputBufferParams exited \n" ));
    API_Trace((" <<< STAUD_IPGetInputBufferParams <<< Error %d \n",Error ));
    return(Error);

}

/*****************************************************************************
STAUD_GetInputBufferParams()
Description:
Parameters:
Return Value:
See Also:
********************/

ST_ErrorCode_t  STAUD_IPSetDataInputInterface(STAUD_Handle_t Handle,
															STAUD_Object_t InputObject,
															GetWriteAddress_t   GetWriteAddress_p,
															InformReadAddress_t InformReadAddress_p,
															void * const BufferHandle_p)
{
    ST_ErrorCode_t	Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetDataInputInterface >>> \n"));
    PrintSTAUD_IPSetDataInputInterface(Handle,InputObject,GetWriteAddress_p,InformReadAddress_p,BufferHandle_p);
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


    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = PESES_SetDataInputInterface(Obj_Handle,GetWriteAddress_p,InformReadAddress_p,BufferHandle_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Parser Get Input buffer Params failed\n"));
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPSetDataInputInterface <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t	*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetCapability >>> \n"));
    PrintSTAUD_IPGetCapability(DeviceName,InputObject,Capability_p);
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
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPGetCapability <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_IPGetDataInterfaceParams >>> \n "));
    PrintSTAUD_IPGetDataInterfaceParams(Handle,InputObject,DMAParams_p);
    API_Trace((" <<<  STAUD_IPGetDataInterfaceParams <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return Error;
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_IPGetParams >>> \n "));
    PrintSTAUD_IPGetParams(Handle,InputObject,InputParams_p);
    API_Trace((" <<<  STAUD_IPGetParams <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return Error;
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

ST_ErrorCode_t STAUD_IPQueuePCMBuffer (STAUD_Handle_t Handle,
STAUD_Object_t InputObject,
STAUD_PCMBuffer_t * PCMBuffer_p,
U32 NumBuffers, U32 * NumQueued_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    Error = ST_NO_ERROR;
    API_Trace((" >>> STAUD_IPQueuePCMBuffer >>> \n"));
    PrintSTAUD_IPQueuePCMBuffer(Handle,InputObject,PCMBuffer_p,NumBuffers,NumQueued_p);
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error |= STAud_QueuePCMBuffer(Obj_Handle,PCMBuffer_p,NumBuffers,NumQueued_p);
        if (Error != ST_NO_ERROR)
        {
            //STTBX_Print(("could not queue the PCM buffer\n"));
        }

    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPQueuePCMBuffer <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    Error = ST_NO_ERROR;
    API_Trace((" >>> STAUD_IPGetPCMBuffer >>> \n "));
    PrintSTAUD_IPGetPCMBuffer(Handle,InputObject,PCMBuffer_p);
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error |= STAud_GetPCMBuffer(Obj_Handle,PCMBuffer_p);
        if (Error != ST_NO_ERROR)
        {
            //STTBX_Print(("could not queue the PCM buffer\n"));
        }

    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPGetPCMBuffer <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    Error = ST_NO_ERROR;
    API_Trace((" >>> STAUD_IPGetPCMBufferSize >>> \n"));
    PrintSTAUD_IPGetPCMBufferSize(Handle,InputObject,BufferSize_p);
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error |= STAud_GetPCMBufferSize(Obj_Handle,BufferSize_p);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("STAud_GetPCMBufferSize() Failed\n"));
        }

    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPGetPCMBufferSize <<< Error %d \n",Error ));
    return Error;
}

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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetLowDataLevelEvent >>> \n"));
    PrintSTAUD_IPSetLowDataLevelEvent(Handle,InputObject,Level);
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = PESES_SetBitBufferLevel(Obj_Handle,(U32)Level);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Set PES Buffer level events\n"));
        }

    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPSetLowDataLevelEvent <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_IPSetParams >>> \n"));
    PrintSTAUD_IPSetParams(Handle,InputObject,InputParams_p);
    API_Trace((" <<< STAUD_IPSetParams <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return Error;
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    U32	WordSize;
    API_Trace((" >>> STAUD_IPSetPCMParams >>> \n"));
    PrintSTAUD_IPSetPCMParams(Handle,InputObject,InputParams_p);
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

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {

        if((InputObject == STAUD_OBJECT_INPUT_CD0) ||
        (InputObject == STAUD_OBJECT_INPUT_CD1)||
        (InputObject == STAUD_OBJECT_INPUT_CD2))
        {
            WordSize = CovertToLpcmWSCode(InputParams_p->DataPrecision);
            Error = PESES_SetPCMParams(Obj_Handle, InputParams_p->Frequency, WordSize, (U32)InputParams_p->NumChannels);
        }

        if(InputObject == STAUD_OBJECT_INPUT_PCM0)
        {
            WordSize = ConvertToAccWSCode(InputParams_p->DataPrecision);
            Error = STAud_SetPCMParams(Obj_Handle, InputParams_p->Frequency, WordSize, (U32)InputParams_p->NumChannels);
        }

        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Set PCM Buffer params\n"));
        }

    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPSetPCMParams <<< Error %d \n",Error ));
    return Error;
}

/****************************************************************************
STAUD_IPSetPCMReaderParams()
Description:
Sets the configuration parameters for an PCM reader of an audio device.
Parameters:
Handle          Handle to audio input device.
InputObject     References a specific input interface
InputParams_p   PCM Reader configuration parameters pointer.

Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or
not available with the current configuration.

See Also: STAUD_IPGetPCMReaderParams
*****************************************************************************/
ST_ErrorCode_t STAUD_IPSetPCMReaderParams(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_PCMReaderConf_t *InputParams_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPSetPCMReaderParams >>> \n"));
    PrintSTAUD_IPSetPCMReaderParams(Handle,InputObject,InputParams_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPSetPCMReaderParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSetPCMReaderParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (InputParams_p == NULL)
    {
        API_Trace((" <<< STAUD_IPSetPCMReaderParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = STAud_PCMReaderSetInputParams(Obj_Handle, InputParams_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_IPSetPCMReaderParams <<< Error %d \n",Error ));
    return Error;
}

/****************************************************************************
STAUD_IPGetPCMReaderParams()
Description:
Gets the configuration parameters for an PCM reader of an audio device.
Parameters:
Handle          Handle to audio input device.
InputObject     References a specific input interface
InputParams_p   PCM Reader configuration parameters pointer.

Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or
not available with the current configuration.

See Also: STAUD_IPSetPCMReaderParams
*****************************************************************************/
ST_ErrorCode_t STAUD_IPGetPCMReaderParams(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_PCMReaderConf_t *InputParams_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetPCMReaderParams >>> \n"));
    PrintSTAUD_IPGetPCMReaderParams(Handle,InputObject,InputParams_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetPCMReaderParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetPCMReaderParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (InputParams_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetPCMReaderParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = STAud_PCMReaderGetInputParams(Obj_Handle, InputParams_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPGetPCMReaderParams <<< Error %d \n",Error ));
    return Error;
}

/****************************************************************************
STAUD_IPGetPCMReaderCapability()
Description:
Gets the configuration parameters for an PCM reader of an audio device.
Parameters:
Handle          Handle to audio input device.
InputObject     References a specific input interface
InputParams_p   PCM Reader capability parameters pointer.

Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.
STAUD_ERROR_INVALID_DEVICE  The input interface is unrecognised or
not available with the current configuration.

See Also: STAUD_IPSetPCMReaderParams
*****************************************************************************/
ST_ErrorCode_t STAUD_IPGetPCMReaderCapability(STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_ReaderCapability_t *InputParams_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_IPGetPCMReaderCapability >>> \n"));
    PrintSTAUD_IPGetPCMReaderCapability(Handle,InputObject,InputParams_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_IPGetPCMReaderCapability <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetPCMReaderCapability <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (InputParams_p == NULL)
    {
        API_Trace((" <<< STAUD_IPGetPCMReaderCapability <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,InputObject);

    if(Obj_Handle)
    {
        Error = STAud_PCMReaderGetCapability(Obj_Handle, InputParams_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_IPGetPCMReaderCapability <<< Error %d \n",Error ));

    return Error;
}

/*****************************************************************************
STAUD_MXConnectSource()
Description:
Parameters:
Return Value:
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_MXConnectSource (STAUD_Handle_t Handle,
STAUD_Object_t MixerObject,
STAUD_Object_t *InputSources_p,
STAUD_MixerInputParams_t * InputParams_p,
U32 NumInputs)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_MXConnectSource >>> \n"));
    PrintSTAUD_MXConnectSource(Handle,MixerObject,InputSources_p,InputParams_p,NumInputs);
    API_Trace((" <<< STAUD_MXConnectSource <<< \n"));
    return Error;
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t	*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_MXGetCapability >>> \n"));
    PrintSTAUD_MXGetCapability(DeviceName,MixerObject,Capability_p);
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
        API_Trace((" <<< STAUD_MXGetCapability <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	MixerObject);

    if(Obj_Handle)
    {
        Error = STAud_MixerGetCapability(Obj_Handle, Capability_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_MXGetCapability <<< Error %d \n",Error));
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
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_MXSetInputParams >>> \n"));
    PrintSTAUD_MXSetInputParams(Handle,MixerObject,InputSource,InputParams_p);
    API_Trace((" <<< STAUD_MXSetInputParams <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return Error;
}
/*****************************************************************************
STAUD_PPGetAttenuation()
Description:
Gets the current volume attenuation applied to the output channels.
Parameters:
Handle          Handle to audio output.

OutputObject    References a specific output interface

Attenuation_p   Stores the result of the current attenuation applied
to the output channels.

Return Value:
ST_NO_ERROR No error.
ST_ERROR_INVALID_HANDLE Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPGetAttenuation (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Attenuation_t * Attenuation_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Attenuation_t   ChannelAtt;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPGetAttenuation >>> \n"));
    PrintSTAUD_PPGetAttenuation(Handle,PostProcObject,Attenuation_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPGetAttenuation <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPGetAttenuation <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Attenuation_p==NULL)
    {
        API_Trace((" <<< STAUD_PPGetAttenuation <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

#ifdef STAUDLX_ALSA_SUPPORT
    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error |= STAud_DecPPGetVol(Obj_Handle,Attenuation_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error ;
#endif

#if ATTENU_AT_DECODER
    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error |= STAud_DecPPGetVol(Obj_Handle,Attenuation_p);
    }
    else
    {
        Error= ST_ERROR_INVALID_HANDLE;
    }
#else

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error = STAud_PPGetVolParams(Obj_Handle,&ChannelAtt);
        *Attenuation_p=* ((STAUD_Attenuation_t*)&ChannelAtt);
    }
    else
    {
        Error= ST_ERROR_INVALID_HANDLE;
    }
#endif
    API_Trace((" <<< STAUD_PPGetAttenuation <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_PPGetEqualizationParams()

Description:
Returns the equalization parameters applied to the output channels.

Parameters:
Handle          Handle to audio output.

OutputObject    References a specific output interface

Equalization_p  Stores the result of the current equalization applied to the output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPGetEqualizationParams(STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Equalization_t *Equalization_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPGetEqualizationParams >>> \n "));
    PrintSTAUD_PPGetEqualizationParams(Handle,PostProcObject,Equalization_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPGetEqualizationParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPGetEqualizationParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Equalization_p==NULL)
    {
        API_Trace((" <<< STAUD_PPGetEqualizationParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error = STAud_PPGetEqualizationParams(Obj_Handle,Equalization_p);
    }
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }
        API_Trace((" <<< STAUD_PPGetEqualizationParams <<< Error %d \n",Error));
        return Error;
}


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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t	*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPGetCapability >>> \n"));
    PrintSTAUD_OPGetCapability(DeviceName,OutputObject,Capability_p);
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
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerGetCapability(Obj_Handle, Capability_p);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerGetCapability(Obj_Handle, Capability_p);
                break;
            default:
                break;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_OPGetCapability <<< Error %d \n",Error ));
    return (Error);
}


/*****************************************************************************
STAUD_PPGetDelay()
Description:
Gets the current delay applied to the output channels.
Parameters:
Handle          Handle to audio decoder.

OutputObject    References a specific output interface

Delay_p         Stores the result of the current delays applied
to the output channels.

Return Value:
ST_NO_ERROR No error.
ST_ERROR_INVALID_HANDLE Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPGetDelay (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Delay_t *Delay_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t *drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPGetDelay >>> \n"));
    PrintSTAUD_PPGetDelay(Handle,PostProcObject,Delay_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPGetDelay <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPGetDelay <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error = STAud_PPGetChannelDelay(Obj_Handle,Delay_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_PPGetDelay <<< Error %d \n",Error ));
    return Error;
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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPGetParams >>> \n"));
    PrintSTAUD_OPGetParams(Handle,OutputObject,Params_p);
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
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerGetOutputParams(Obj_Handle, &(Params_p->PCMOutParams));
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerGetOutputParams(Obj_Handle, &(Params_p->SPDIFOutParams));
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
    API_Trace((" <<< STAUD_OPGetParams <<< Error %d \n",Error ));
    return Error;
}

#ifndef STAUD_NO_WRAPPER_LAYER
/*****************************************************************************
staud_OPGetEnable()
Description:
Identifies whether the given audio output device is enabled
(needed by wrapper layer STAUD_GetDigitalOutput)

Parameters:
Handle          Handle to audio output.

OutputObject    References a specific output interface

InputSource     Stores current enable status for output device.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

ST_ERROR_UNKNOWN_DEVICE     Unknown device name specified.

See Also:
*****************************************************************************/
/*
ST_ErrorCode_t staud_OPGetEnable (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject,
BOOL*          Enable_p)
{


	ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

	return Error;
}
*/

/*****************************************************************************
staud_OPGetSource()
Description:
Identifies the current input source to an audio output device.
(needed by wrapper layer STAUD_GetDigitalOutput)

Parameters:
Handle          Handle to audio output.

OutputObject    References a specific output interface

InputSource     Stores current Input source for output device.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

ST_ERROR_UNKNOWN_DEVICE     Unknown device name specified.

See Also:
*****************************************************************************/
/*
ST_ErrorCode_t staud_OPGetSource (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject,
STAUD_Object_t * InputSource_p)
{
	ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

	return Error;
}
*/
#endif /* STAUD_NO_WRAPPER_LAYER */


/*****************************************************************************
STAUD_PPGetSpeakerEnable()

Description:
Retrieves which speakers are currently used in the listening room.

Parameters:
Handle          Handle to audio output.
PostProcObject    References a specific output interface
Speaker_p       Stores result of which speakers are in use.

Return Value:
ST_NO_ERROR                  No error.

ST_ERROR_INVALID_HANDLE      Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPGetSpeakerEnable (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_SpeakerEnable_t *Speaker_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_PPGetSpeakerEnable >>> \n"));
    PrintSTAUD_PPGetSpeakerEnable(Handle,PostProcObject,Speaker_p);
    API_Trace((" <<< STAUD_PPGetSpeakerEnable <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return Error;
}


/*****************************************************************************
STAUD_PPGetSpeakerConfig()
Description:
Obtains the current configuration of speakers in the listening room.

Parameters:
Handle           Handle to audio output.

PostProcObject     References a specific output interface

SpeakerConfig_p  Stores the result of the current speaker configuration.

Return Value:
ST_NO_ERROR              No error.

ST_ERROR_INVALID_HANDLE  Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPGetSpeakerConfig (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_SpeakerConfiguration_t *SpeakerConfig_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    STAud_PPSpeakerConfiguration_t SpeakerConfig;
    AudDriver_t *drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPGetSpeakerConfig >>> \n"));
    PrintSTAUD_PPGetSpeakerConfig(Handle,PostProcObject,SpeakerConfig_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPGetSpeakerConfig <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPGetSpeakerConfig <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error=STAud_PPGetSpeakerConfig(Obj_Handle,&SpeakerConfig);
        *SpeakerConfig_p=* ((STAUD_SpeakerConfiguration_t *)&SpeakerConfig);
    }
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_PPGetSpeakerConfig <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
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
        if((OutputObject == STAUD_OBJECT_OUTPUT_SPDIF0) ||(OutputObject == STAUD_OBJECT_OUTPUT_HDMI_SPDIF0) )
        {
            Error = STAud_SPDIFPlayerMute(Obj_Handle, TRUE); // Mute the SPDIF Player
        }
        if((OutputObject == STAUD_OBJECT_OUTPUT_PCMP0) || (OutputObject == STAUD_OBJECT_OUTPUT_PCMP1)
        || (OutputObject == STAUD_OBJECT_OUTPUT_PCMP2) || (OutputObject == STAUD_OBJECT_OUTPUT_PCMP3)
        || (OutputObject == STAUD_OBJECT_OUTPUT_HDMI_PCMP0))
        {
            Error = STAud_PCMPlayerMute(Obj_Handle, TRUE); // Mute the PCM Player
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
#else
    Error = STAUD_DRMute(Handle,OutputObject);
#endif
    API_Trace((" <<< STAUD_OPMute <<< Error %d \n",Error ));
    return Error;
}

ST_ErrorCode_t STAUD_DRMute (STAUD_Handle_t Handle,
										STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRMute >>> \n"));
    PrintSTAUD_DRMute(Handle,OutputObject);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRMute <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRMute <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if(Obj_Handle)
    {
        if((OutputObject == STAUD_OBJECT_DECODER_COMPRESSED0) ||
        (OutputObject == STAUD_OBJECT_DECODER_COMPRESSED1)||
        (OutputObject == STAUD_OBJECT_DECODER_COMPRESSED2))
        {
            Error = STAud_DecMute(Obj_Handle, TRUE); // Mute the decoder
        }

#if MUTE_AT_DECODER
        if((OutputObject == STAUD_OBJECT_OUTPUT_SPDIF0) || (OutputObject == STAUD_OBJECT_OUTPUT_HDMI_SPDIF0))
        {
            Error = STAud_SPDIFPlayerMute(Obj_Handle, TRUE); // Mute the SPDIF Player
        }
#endif
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRMute <<< Error %d \n",Error ));
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

ST_ErrorCode_t STAUD_MXSetMixLevel (STAUD_Handle_t Handle,
											STAUD_Object_t MixerObject, U32 InputID, U16 MixLevel)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_MXSetMixLevel >>> \n"));
    PrintSTAUD_MXSetMixLevel(Handle,MixerObject,InputID,MixLevel);
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
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_MXSetMixLevel <<< Error %d \n", Error ));
    return Error;
}




/*****************************************************************************
STAUD_PPSetAttenuation()

Description:
Sets the volume attenuation applied to the output channels.

Parameters:
Handle           Handle to audio output.

PostProcObject     References a specific output interface

Attenuation_p    Specifies attenuation to apply to output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPSetAttenuation (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Attenuation_t * Attenuation_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPSetAttenuation >>> \n"));
    PrintSTAUD_PPSetAttenuation(Handle,PostProcObject,Attenuation_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPSetAttenuation <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPSetAttenuation <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Attenuation_p == NULL)
    {
        API_Trace((" <<< STAUD_PPSetAttenuation <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

#ifdef STAUDLX_ALSA_SUPPORT
    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error |= STAud_DecPPSetVol(Obj_Handle,Attenuation_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;

#endif

#if ATTENU_AT_DECODER
    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error |= STAud_DecPPSetVol(Obj_Handle,(STAUD_Attenuation_t *)Attenuation_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

#else

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error = STAud_PPSetVolParams(Obj_Handle,(STAUD_Attenuation_t  *)Attenuation_p);
    }
    else
    {
        Error= ST_ERROR_INVALID_HANDLE;
    }

#endif
    API_Trace((" <<< STAUD_PPSetAttenuation <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_PPSetDelay()

Description:
Sets the delay applied to the output channels.

Parameters:
Handle          Handle to audio output.

OutputObject    References a specific output interface

Delay_p         Specifies the delay to apply to the output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPSetDelay (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Delay_t * Delay_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPSetDelay >>> \n"));
    PrintSTAUD_PPSetDelay(Handle,PostProcObject,Delay_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPSetDelay <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPSetDelay <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error = STAud_PPSetChannelDelay(Obj_Handle,Delay_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_PPSetDelay <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_PPSetEqualizationParams()

Description:
Sets the equalization parameters applied to the output channels.

Parameters:
Handle          Handle to audio output.

OutputObject    References a specific output interface

Equalization_p  Specifies the equalization to apply to the output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_PPSetEqualizationParams(STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Equalization_t *Equalization_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPSetEqualizationParams >>> \n"));
    PrintSTAUD_PPSetEqualizationParams(Handle,PostProcObject,Equalization_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPSetEqualizationParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPSetEqualizationParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Equalization_p == NULL)
    {
        API_Trace((" <<< STAUD_PPSetEqualizationParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error = STAud_PPSetEqualizationParams(Obj_Handle,Equalization_p);
    }
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_PPSetEqualizationParams <<< Error %d \n",Error ));
    return Error;
}



/*****************************************************************************
STAUD_DPSetCallback()

Description:
Sets the delay applied to the output channels.

Parameters:
Handle          Handle to audio output.
DPObject		References a specific DataProcessor object
Func_fp			Function pointer
clientInfo		ClientInfo

Return Value:
ST_NO_ERROR                 No error.
ST_ERROR_INVALID_HANDLE     Handle invalid or not open.
ST_ERROR_UNKNOWN_DEVICE		The Audio Device is not recognised

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_DPSetCallback (STAUD_Handle_t Handle,
STAUD_Object_t DPObject, FrameDeliverFunc_t Func_fp, void * clientInfo)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DPSetCallback >>> \n"));
    PrintSTAUD_DPSetCallback(Handle,DPObject,Func_fp,clientInfo);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DPSetCallback <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DPSetCallback <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	DPObject);

    if(Obj_Handle)
    {
        Error = STAud_DataProcesserUpdateCallback(Obj_Handle,Func_fp,clientInfo);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DPSetCallback <<< Error %d \n",Error ));
    return Error;
}

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
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_OPSetParams >>> \n"));
    PrintSTAUD_OPSetParams(Handle,OutputObject,Params_p);
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
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerSetOutputParams(Obj_Handle, Params_p->PCMOutParams);
                break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error |= STAud_SPDIFPlayerSetOutputParams(Obj_Handle, Params_p->SPDIFOutParams);
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
    API_Trace((" <<< STAUD_OPSetParams <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_PPSetSpeakerEnable()

Description:
Sets which speakers are used in the listening room.

Parameters:
Handle           Handle to audio output.

OutputObject     References a specific output interface

Speaker_p        Defines which speakers are in use.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPSetSpeakerEnable (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject,
STAUD_SpeakerEnable_t * Speaker_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_PPSetSpeakerEnable >>> \n"));
    PrintSTAUD_PPSetSpeakerEnable(Handle,OutputObject,Speaker_p);
    API_Trace((" <<< STAUD_PPSetSpeakerEnable <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return Error;
}


/*****************************************************************************
STAUD_PPSetSpeakerConfig()

Description:
Defines the types of speakers in the listening room.

Parameters:
Handle           Handle to audio output.

OutputObject     References a specific output interface

SpeakerConfig_p  Defines the speaker configuration to apply.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPSetSpeakerConfig (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_SpeakerConfiguration_t *
SpeakerConfig_p, STAUD_BassMgtType_t BassType)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPSetSpeakerConfig >>> \n"));
    PrintSTAUD_PPSetSpeakerConfig(Handle, PostProcObject, SpeakerConfig_p, BassType);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPSetSpeakerConfig <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPSetSpeakerConfig <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(SpeakerConfig_p == NULL)
    {
        API_Trace((" <<< STAUD_PPSetSpeakerConfig <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error=STAud_PPSetSpeakerConfig(Obj_Handle,(STAud_PPSpeakerConfiguration_t*)SpeakerConfig_p,BassType);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_PPSetSpeakerConfig <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
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
        if((OutputObject == STAUD_OBJECT_OUTPUT_SPDIF0) || (OutputObject == STAUD_OBJECT_OUTPUT_HDMI_SPDIF0))
        {
            Error = STAud_SPDIFPlayerMute(Obj_Handle, FALSE); // Mute the SPDIF Player
        }
        if((OutputObject == STAUD_OBJECT_OUTPUT_PCMP0) || (OutputObject == STAUD_OBJECT_OUTPUT_PCMP1)
        || (OutputObject == STAUD_OBJECT_OUTPUT_PCMP2) || (OutputObject == STAUD_OBJECT_OUTPUT_PCMP3)
        || (OutputObject == STAUD_OBJECT_OUTPUT_HDMI_PCMP0))
        {
            Error = STAud_PCMPlayerMute(Obj_Handle, FALSE); // Mute the SPDIF Player
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
#else
    Error = STAUD_DRUnMute(Handle,OutputObject);
#endif
    API_Trace((" <<< STAUD_OPUnMute <<< Error %d \n",Error ));
    return Error;
}


ST_ErrorCode_t STAUD_DRUnMute (STAUD_Handle_t Handle,
										STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRUnMute >>> \n"));
    PrintSTAUD_DRUnMute(Handle,OutputObject);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRUnMute <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRUnMute <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	OutputObject);

    if(Obj_Handle)
    {
        if((OutputObject == STAUD_OBJECT_DECODER_COMPRESSED0)||
        (OutputObject == STAUD_OBJECT_DECODER_COMPRESSED1)||
        (OutputObject == STAUD_OBJECT_DECODER_COMPRESSED2))
        {
            Error = STAud_DecMute(Obj_Handle, FALSE); // Unmute the decoder
        }

#if MUTE_AT_DECODER
    if((OutputObject == STAUD_OBJECT_OUTPUT_SPDIF0) || (OutputObject == STAUD_OBJECT_OUTPUT_HDMI_SPDIF0))
    {
        Error = STAud_SPDIFPlayerMute(Obj_Handle, FALSE);// Unmute the SPDIF Player
    }
#endif
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRUnMute <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_PPConnectSource()
Description:
Connects an input source to an audio post-processing device.
Parameters:

Handle          Handle to audio output.
PostProcObject  References a specific post processor.
InputSrc        Input source to connect to post-processor device.

Return Value:

ST_NO_ERROR                No error.

ST_ERROR_BAD_PARAMETER     Invalid input device.

ST_ERROR_INVALID_HANDLE    Handle invalid or not open.

ST_ERROR_UNKNOWN_DEVICE    Unknown device name specified.

STAUD_ERROR_INVALID_DEVICE The post processor is unrecognised or not available
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPConnectSource (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Object_t InputSource)
{
    ST_ErrorCode_t	ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_PPConnectSource >>> \n"));
    PrintSTAUD_PPConnectSource(Handle,PostProcObject,InputSource);
    API_Trace((" <<< STAUD_PPConnectSource <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return ErrorCode;
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

ST_ErrorCode_t STAUD_MXGetMixLevel (STAUD_Handle_t Handle,
											STAUD_Object_t MixerObject, U32 InputID, U16 *MixLevel_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_MXGetMixLevel >>> \n"));
    PrintSTAUD_MXGetMixLevel(Handle,MixerObject,InputID,MixLevel_p);
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
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_MXGetMixLevel <<< Error %d \n",Error ));
    return Error;

}



/*****************************************************************************
STAUD_PPGetCapability()
Description:
Returns the capabilities of a specific postprocessor object.

Parameters:

DeviceName      Instance of audio device.
PostProcObject  Specific postproc object to return capabilites for.
Capability_p    Pointer to returned capabilities.

Return Value:

ST_NO_ERROR                 No error.
ST_ERROR_UNKNOWN_DEVICE     The specified device is not initialized
STAUD_ERROR_INVALID_DEVICE  The psotproc device is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPGetCapability(const ST_DeviceName_t DeviceName,
STAUD_Object_t PostProcObject,
STAUD_PPCapability_t * Capability_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t	*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPGetCapability >>> \n"));
    PrintSTAUD_PPGetCapability(DeviceName,PostProcObject,Capability_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPGetCapability <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromName(DeviceName);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPGetCapability <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Capability_p == NULL)
    {
        API_Trace((" <<< STAUD_PPGetCapability <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	PostProcObject);

    if(Obj_Handle)
    {
        Error = STAud_PostProcGetCapability(Obj_Handle, Capability_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_PPGetCapability <<< Error %d \n",Error ));
    return (Error);
}

/*****************************************************************************
STAUD_DRGetCircleSurroundParams()
Description:
Obtains the current CircleSurrond parameters applied.
Parameters:
Handle          Handle to post processing block.
DecoderObject   References a specific post processor.
Omni_p			Stores result of current omni parameters applied.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The post processing block does
not support omni post processing.

STAUD_ERROR_INVALID_DEVICE          The post processor is unrecognised or
not available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetCircleSurroundParams (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject, STAUD_CircleSurrondII_t * Csii)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetCircleSurroundParams >>> \n"));
    PrintSTAUD_DRGetCircleSurroundParams(Handle,DecoderObject,Csii);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetCircleSurroundParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetCircleSurroundParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Csii == NULL)
    {
        API_Trace((" <<< STAUD_DRGetCircleSurroundParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetCircleSurroundParams(Obj_Handle,Csii);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetCircleSurroundParams <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_DRGetDeEmphasisFilter()

Description:
Retrieves the state of the de-emphasis filter.
Parameters:
Handle          Handle to post processing block.
PostProcObject  References a specific post processor.
Emphasis_p      Stores the result of the de-emphasis filter.
TRUE : De-emphasis enabled.
FALSE : De-emphasis disabled.

Return Value:
ST_NO_ERROR                      No error.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support
de-emphasis filtering.

ST_ERROR_INVALID_HANDLE         The handle is not a valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetDeEmphasisFilter (STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject,BOOL *Emphasis_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    DeEmphParams_t		DeEmph;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetDeEmphasisFilter >>> \n"));
    PrintSTAUD_DRGetDeEmphasisFilter(Handle,DecoderObject,Emphasis_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetDeEmphasisFilter <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetDeEmphasisFilter <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Emphasis_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetDeEmphasisFilter <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error |= STAud_DecPPGetDeEmph(Obj_Handle,&DeEmph);
        if(DeEmph.Apply == ACC_MME_DISABLED)
            *Emphasis_p = FALSE ;
    else
        *Emphasis_p = TRUE ;
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetDeEmphasisFilter <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRGetDolbyDigitalEx()
Description:
Obtains the current dolby digital Ex (AC3Ex) settings applied.
Parameters:
Handle				Handle to post processing block.
DecoderObject		References a specific post processor.
DolbyDigitalEx_p	Stores result of current dolby digital Ex settings applied.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The post processing block does
not support AC3Ex (Dolby Digital Ex) post processing.

STAUD_ERROR_INVALID_DEVICE          The post processor is unrecognised or
not available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetDolbyDigitalEx (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject, BOOL * DolbyDigitalEx_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetDolbyDigitalEx >>> \n"));
    PrintSTAUD_DRGetDolbyDigitalEx(Handle,DecoderObject,DolbyDigitalEx_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetDolbyDigitalEx <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetDolbyDigitalEx <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (DolbyDigitalEx_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetDolbyDigitalEx <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetDolbyDigitalEx(Obj_Handle,DolbyDigitalEx_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetDolbyDigitalEx <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRGetEffect()
Description:
Obtains the current stereo effects applied.
Parameters:
Handle           Handle to post processing block.
DecoderObject   References a specific post processor.
Effect_p        Stores result of current stereo effects applied.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The post processing block does
not support stereo effects.

STAUD_ERROR_INVALID_DEVICE          The post processor is unrecognised or
not available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetEffect (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject, STAUD_Effect_t * Effect_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetEffect >>> \n"));
    PrintSTAUD_DRGetEffect(Handle,DecoderObject,Effect_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetEffect <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetEffect <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Effect_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetEffect <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetEffect(Obj_Handle,Effect_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetEffect <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRGetOmniParams()
Description:
Obtains the current omni parameters applied.
Parameters:
Handle          Handle to post processing block.
DecoderObject   References a specific post processor.
Omni_p			Stores result of current omni parameters applied.

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The post processing block does
not support omni post processing.

STAUD_ERROR_INVALID_DEVICE          The post processor is unrecognised or
not available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetOmniParams (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject, STAUD_Omni_t * Omni_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetOmniParams >>> \n"));
    PrintSTAUD_DRGetOmniParams(Handle,DecoderObject,Omni_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetOmniParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetOmniParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Omni_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetOmniParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetOmniParams(Obj_Handle,Omni_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetOmniParams <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRGetSRSEffectParams()
Description:
Obtains the current stereo effects applied.
Parameters:
Handle          Handle to decoder block.
DecoderObject   References a specific Decoder
ParamType        Type of Parameter required
Value_p			Return Value to Store

Return Value:
ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The post processing block does
not support stereo effects.

STAUD_ERROR_INVALID_DEVICE          The post processor is unrecognised or
not available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetSRSEffectParams (STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject, STAUD_EffectSRSParams_t ParamType,S16 *Value_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetSRSEffectParams >>> \n"));
    PrintSTAUD_DRGetSRSEffectParams(Handle,DecoderObject,ParamType,Value_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetSRSEffectParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetSRSEffectParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Value_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetSRSEffectParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetSRSEffectParams(Obj_Handle,ParamType, Value_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetSRSEffectParams <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_PPGetKaraoke()
Description:
Obtains the current karaoke downmix mode.
Parameters:
Handle           Handle to post processing block.

PostProcObject   References a specific post processor.

Karaoke_p        Stores result of current karaoke downmix applied.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support
karaoke downmixing.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not
available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPGetKaraoke (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Karaoke_t * Karaoke_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_PPGetKaraoke >>> \n"));
    PrintSTAUD_PPGetKaraoke(Handle,PostProcObject,Karaoke_p);
    API_Trace((" <<< STAUD_PPGetKaraoke <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return Error;
}


/*****************************************************************************
STAUD_DRGetPrologic()
Description:
Obtains the current Dolby Surround decoder state.
Parameters:
Handle           Handle to post processing block.
DecoderObject   References a specific post processor.
Prologic_p       Stores result of the current Dolby Surround decoder state.
TRUE : Dolby Surround decoder active.
FALSE : Dolby Surround decoder inactive.

Return Value:
ST_NO_ERROR                     No error.
ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support Dolby
Surround decoding.
STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not
available  with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetPrologic(STAUD_Handle_t Handle,
		STAUD_Object_t DecoderObject,BOOL * Prologic_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetPrologic >>> \n"));
    PrintSTAUD_DRGetPrologic(Handle,DecoderObject,Prologic_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetPrologic <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetPrologic <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Prologic_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetPrologic <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetProLogic(Obj_Handle,Prologic_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetPrologic <<< Error %d \n",Error ));
    return Error;
}
/*****************************************************************************
STAUD_DRGetPrologicAdvance()
Description:
Obtains the current Dolby Surround decoder state.
Parameters:
Handle           Handle to post processing block.
DecoderObject   References a specific post processor.
PLIIParams    return value

Return Value:
ST_NO_ERROR                     No error.
ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support Dolby
Surround decoding.
STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not
available  with the current configuration.

See Also:
*****************************************************************************/

 ST_ErrorCode_t STAUD_DRGetPrologicAdvance (STAUD_Handle_t Handle,
				STAUD_Object_t DecoderObject, STAUD_PLIIParams_t *PLIIParams)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetPrologicAdvance >>> \n"));
    PrintSTAUD_DRGetPrologicAdvance(Handle,DecoderObject,PLIIParams);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetPrologicAdvance <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetPrologicAdvance <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (PLIIParams == NULL)
    {
        API_Trace((" <<< STAUD_DRGetPrologicAdvance <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetProLogicAdvance(Obj_Handle,PLIIParams);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetPrologicAdvance <<< Error %d \n",Error ));
    return Error;
}
/*****************************************************************************
STAUD_DRGetStereoMode()
Description:
Obtains the current stereo output configuration.
Parameters:
Handle              Handle to post processing block.

DecoderObject      References a specific post processor.

StereoMode_p        Stores result of the current stereo output configuration.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support stereo
output configuration.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not avail-able
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetStereoMode (STAUD_Handle_t Handle,
			STAUD_Object_t DecoderObject,STAUD_StereoMode_t * StereoMode_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetStereoMode >>> \n"));
    PrintSTAUD_DRGetStereoMode(Handle,DecoderObject,StereoMode_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetStereoMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetStereoMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (StereoMode_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetStereoMode <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetStereoMode(Obj_Handle,StereoMode_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }


    API_Trace((" <<< STAUD_DRGetStereoMode <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRGetInStereoMode()
Description:
Obtains the current stereo input configuration.
Parameters:
Handle              Handle to decoder block.

DecoderObject      References a specific decoder

StereoMode_p        Stores result of the current stereo output configuration.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_FEATURE_NOT_SUPPORTED  The decoder block does not support stereo
output configuration.

STAUD_ERROR_INVALID_DEVICE      The decoder is unrecognised or not avail-able
with the current configuration.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_DRGetInStereoMode (STAUD_Handle_t Handle,
			STAUD_Object_t DecoderObject,STAUD_StereoMode_t * StereoMode_p)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	AudDriver_t		*drv_p = NULL;
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

	if (StereoMode_p == NULL)
	{
		return ST_ERROR_BAD_PARAMETER;
	}

	Obj_Handle = GetHandle(drv_p,	DecoderObject);

	if(Obj_Handle)
	{
		Error = STAud_DecPPGetInStereoMode(Obj_Handle,StereoMode_p);
	}
	else
	{
		Error =  ST_ERROR_INVALID_HANDLE;
	}

	return Error;
}


/*****************************************************************************
STAUD_DRSetCircleSurroundParams()

Description:
Sets the Circle Surround parameters to be applied.

Parameters:
Handle           Handle to post processing block.
PostProcObject   References a specific post processor.
Omni			 Omni Parameters to be applied.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_BAD_PARAMETER          The stereo effect parameter is invalid.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support omni post processing.

ST_ERROR_INVALID_HANDLE         The handle is not a valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetCircleSurroundParams (STAUD_Handle_t Handle,
									STAUD_Object_t DecoderObject,STAUD_CircleSurrondII_t Csii)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetCircleSurroundParams >>> \n"));
    PrintSTAUD_DRSetCircleSurroundParams(Handle,DecoderObject,Csii);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetCircleSurroundParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetCircleSurroundParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetCircleSurrondParams(Obj_Handle,&Csii);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetCircleSurroundParams <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRSetDeEmphasisFilter()
Description:
Sets the state of the de-emphasis filter.
Parameters:
Handle           Handle to post processing block.

PostProcObject   References a specific post processor.

Emphasis         Emphasis filter enable.

TRUE : Enable the emphasis filter.
FALSE : Disable the emphasis filter.

Return Value:
ST_NO_ERROR                      No error.

ST_ERROR_FEATURE_NOT_SUPPORTED   The post processing block does not support
de-emphasis filtering.

ST_ERROR_INVALID_HANDLE          The handle is not a valid.

STAUD_ERROR_INVALID_DEVICE       The post processor is unrecognised or not
available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetDeEmphasisFilter (STAUD_Handle_t Handle,
							STAUD_Object_t DecoderObject,BOOL Emphasis)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    DeEmphParams_t		DeEmph;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetDeEmphasisFilter >>> \n"));
    PrintSTAUD_DRSetDeEmphasisFilter(Handle,DecoderObject,Emphasis);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetDeEmphasisFilter <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetDeEmphasisFilter <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        DeEmph.Mode = 0; //default

        if(Emphasis)
        {
            DeEmph.Apply = ACC_MME_ENABLED;
        }
        else
        {
            DeEmph.Apply = ACC_MME_DISABLED;
        }

    Error = STAud_DecPPSetDeEmph(Obj_Handle,DeEmph);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetDeEmphasisFilter <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRSetDolbyDigitalEx()

Description:
Enables/Disables the DolbyDigitalEx to be applied.

Parameters:
Handle           Handle to post processing block.
PostProcObject   References a specific post processor.
BOOL			 To enable or disable AC3Ex processing

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_INVALID_HANDLE         The handle is not a valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetDolbyDigitalEx (STAUD_Handle_t Handle,
							STAUD_Object_t DecoderObject,BOOL DolbyDigitalEx)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetDolbyDigitalEx >>> \n"));
    PrintSTAUD_DRSetDolbyDigitalEx(Handle,DecoderObject,DolbyDigitalEx);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetDolbyDigitalEx <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetDolbyDigitalEx <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetDolbyDigitalEx(Obj_Handle,DolbyDigitalEx);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetDolbyDigitalEx <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_DRSetEffect()

Description:
Sets the stereo output effect to be applied.

Parameters:
Handle           Handle to post processing block.
PostProcObject   References a specific post processor.
Effect           Stereo output effect to be applied.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_BAD_PARAMETER          The stereo effect parameter is invalid.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support stereo
effects.

ST_ERROR_INVALID_HANDLE         The handle is not a valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetEffect (STAUD_Handle_t Handle,
									STAUD_Object_t DecoderObject,STAUD_Effect_t Effect)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetEffect >>> \n"));
    PrintSTAUD_DRSetEffect(Handle,DecoderObject,Effect);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetEffect <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetEffect <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetEffect(Obj_Handle,Effect);
    }
    else
    {
    Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetEffect <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRSetOmniParams()

Description:
Sets the omni parameters to be applied.

Parameters:
Handle           Handle to post processing block.
PostProcObject   References a specific post processor.
Omni			 Omni Parameters to be applied.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_BAD_PARAMETER          The stereo effect parameter is invalid.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support omni post processing.

ST_ERROR_INVALID_HANDLE         The handle is not a valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetOmniParams (STAUD_Handle_t Handle,
									STAUD_Object_t DecoderObject,STAUD_Omni_t Omni)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetOmniParams >>> \n"));
    PrintSTAUD_DRSetOmniParams(Handle,DecoderObject,Omni);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetOmniParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetOmniParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetOmniParams(Obj_Handle,Omni);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetOmniParams <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRSetSRSEffectParams()

Description:
Sets the parameters for SRS effects

Parameters:
Handle           Handle to post processing block.
DecoderObject   References a specific Decoder.
Params			Which Parameters to be Set
Value           Value of parameter

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_BAD_PARAMETER          The stereo effect parameter is invalid.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support stereo
effects.

ST_ERROR_INVALID_HANDLE         The handle is not a valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not available
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetSRSEffectParams (STAUD_Handle_t Handle,
									STAUD_Object_t DecoderObject,STAUD_EffectSRSParams_t ParamsType,S16 Value)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetSRSEffectParams >>> \n"));
    PrintSTAUD_DRSetSRSEffectParams(Handle,DecoderObject,ParamsType,Value);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetSRSEffectParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetSRSEffectParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetSRSEffectParams(Obj_Handle,ParamsType,Value);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetSRSEffectParams <<< Error %d \n",Error ));
    return Error;
}
/*****************************************************************************
STAUD_PPSetKaraoke()

Description:
Sets the karaoke output downmix.

Parameters:
Handle          Handle to post processing block.
PostProcObject  References a specific post processor.
Karaoke         Value of karaoke downmix to be applied.

Return Value:
ST_NO_ERROR                      No error.
ST_ERROR_BAD_PARAMETER           The karaoke downmix is invalid.

ST_ERROR_FEATURE_NOT_SUPPORTED   The post processing block does not support
karaoke downmixing.

ST_ERROR_INVALID_HANDLE          The handle is not valid.

STAUD_ERROR_INVALID_DEVICE       The post processor is unrecognised or not
available  with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_PPSetKaraoke (STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Karaoke_t Karaoke)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    API_Trace((" >>> STAUD_PPSetKaraoke >>> \n"));
    PrintSTAUD_PPSetKaraoke(Handle,PostProcObject,Karaoke);
    API_Trace((" <<< STAUD_PPSetKaraoke <<< Error ST_ERROR_FEATURE_NOT_SUPPORTED \n"));
    return Error;
}


/*****************************************************************************
STAUD_DRSetPrologic()
Description:
Set the Dolby Surround decoder active or inactive.

Parameters:
Handle           Handle to post processing block.

PostProcObject   References a specific post processor.

Prologic         Dolby Surround decoder state.

TRUE : Activate the Dolby Surround decoder.
FALSE : De-activate the Dolby Surround decoder.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support Dolby
Surround decoding.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not
available with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetPrologic (STAUD_Handle_t Handle,
							STAUD_Object_t Decoder,BOOL Prologic)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetPrologic >>> \n"));
    PrintSTAUD_DRSetPrologic(Handle,Decoder,Prologic);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetPrologic <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetPrologic <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	Decoder);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetProLogic(Obj_Handle,Prologic);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetPrologic <<< Error %d \n",Error ));
    return Error;
}
/*****************************************************************************
STAUD_DRSetPrologicAdvance()
Description:
Set the Dolby Prologic setting. We can pass various params in this function. This function is mainly added for
advanced users such as certification engineers etc.

Parameters:
Handle           Handle to post processing block.

Decoder   References a specific Decoder.

PLIIParams PLII Params

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_FEATURE_NOT_SUPPORTED  The post processing block does not support Dolby
Surround decoding.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_INVALID_DEVICE      The post processor is unrecognised or not
available with the current configuration.

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_DRSetPrologicAdvance (STAUD_Handle_t Handle,
					STAUD_Object_t DecoderObject, STAUD_PLIIParams_t PLIIParams)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    STAUD_Delay_t  Delay;
    API_Trace((" >>> STAUD_DRSetPrologicAdvance >>> \n"));
    PrintSTAUD_DRSetPrologicAdvance(Handle,DecoderObject,PLIIParams);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetPrologicAdvance <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetPrologicAdvance <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetProLogicAdvance(Obj_Handle,PLIIParams);
    }
    else
    {
        Error |=  ST_ERROR_INVALID_HANDLE;
    }

    /* 	If the DecMode is Movie or prologic, we need to set a fix delay of 10ms on Ls and Rs Channels.
    This is done using Postprocessor module as decoder doesn't support this feature
    */
    if((PLIIParams.DecMode == STAUD_DECMODE_MOVIE)||
    (PLIIParams.DecMode == STAUD_DECMODE_PROLOGIC))
    {
        memset(&Delay,0,sizeof(STAUD_Delay_t));
        Delay.LeftSurround = 10;
        Delay.RightSurround = 10;
        Error |= STAUD_PPSetDelay(Handle,STAUD_OBJECT_POST_PROCESSOR0,&Delay);
    }
    API_Trace((" <<< STAUD_DRSetPrologicAdvance <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetStereoMode >>> \n"));
    PrintSTAUD_DRSetStereoMode(Handle,DecoderObject,StereoMode);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetStereoMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetStereoMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetStereoMode(Obj_Handle,StereoMode);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetStereoMode <<< Error %d \n",Error ));
    return Error;
}

/************************************************************************************
Name         : STAUD_SelectDac()

Description  : Selects the internal/external DAC.

Parameters   :none

Return Value :
TRUE         Internal DAC is selected
FALSE        Internal DAC is not selected

************************************************************************************/
BOOL STAUD_SelectDac(void)
{
    API_Trace((" >>> STAUD_SelectDac >>> \n"));
    PrintSTAUD_SelectDac();
    if (Internal_Dacs)
    {
        Internal_Dacs = false;
    }
    else
    {
        Internal_Dacs = true;
    }
    API_Trace((" <<< STAUD_SelectDac <<< \n"));
    return(Internal_Dacs);
}


ST_ErrorCode_t STAUD_PPDcRemove(STAUD_Handle_t Handle, STAUD_Object_t PostProcObject,
								STAUD_DCRemove_t *Params_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t *drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPDcRemove >>> \n "));
    PrintSTAUD_PPDcRemove(Handle,PostProcObject,Params_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPDcRemove <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPDcRemove <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Params_p == NULL)
    {
        API_Trace((" <<< STAUD_PPDcRemove <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,PostProcObject);

    if(Obj_Handle)
    {
        Error = STAud_PPDcRemove(Obj_Handle,(enum eAccProcessApply *)Params_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_PPDcRemove <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRSetCompressionMode()

Description:
If possible sets the Compression Mode. Only valid for AC3 decoder.


Parameters:
Handle             Handle to audio decoder.

DecoderObject      References a specific audio decoder.

CompressionMode_p  Specifies the Compression Mode settings.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The audio decoder does not support the Compression Mode.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetCompressionMode (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,STAUD_CompressionMode_t CompressionMode_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetCompressionMode >>> \n"));
    PrintSTAUD_DRSetCompressionMode(Handle,DecoderObject,CompressionMode_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetCompressionMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetCompressionMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(CompressionMode_p > STAUD_RF_MODE)
    {
        API_Trace((" <<< STAUD_DRSetCompressionMode <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }


    Obj_Handle = GetHandle(drv_p,DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetCompressionMode(Obj_Handle,CompressionMode_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetCompressionMode <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_DRGetCompressionMode()

Description:
If possible Get the Compression Mode. Only valid for AC3 decoder.


Parameters:
Handle             Handle to audio decoder.

DecoderObject      References a specific audio decoder.

CompressionMode_p  The Compression Mode pointer.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The audio decoder does not support the Compression Mode.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

ST_ERROR_BAD_PARAMETER          One or more of the supplied parameters is invalid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetCompressionMode (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,STAUD_CompressionMode_t *CompressionMode_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetCompressionMode >>> \n"));
    PrintSTAUD_DRGetCompressionMode(Handle,DecoderObject,CompressionMode_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetCompressionMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetCompressionMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(CompressionMode_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetCompressionMode <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetCompressionMode(Obj_Handle,CompressionMode_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetCompressionMode <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_DRSetAudioCodingMode()

Description:
If possible sets the Audio Coding Mode. Valid for AC3 decoder, and may be later for other decoders.


Parameters:
Handle             Handle to audio decoder.

DecoderObject      References a specific audio decoder.

AudioCodingMode_p  Specifies the Audio Coding Mode settings.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The audio decoder does not support the Audio Coding Mode.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

ST_ERROR_BAD_PARAMETER          One or more of the supplied parameters is invalid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetAudioCodingMode (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,STAUD_AudioCodingMode_t AudioCodingMode_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetAudioCodingMode >>> \n"));
    PrintSTAUD_DRSetAudioCodingMode(Handle,DecoderObject,AudioCodingMode_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetAudioCodingMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetAudioCodingMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    /*GNBvd60440
    SetAudioCodingMode is returning error for some DDPlus 7.1 out_mode*/
    if( AudioCodingMode_p > STAUD_ACC_MODE_ID)
    {
        return ST_ERROR_BAD_PARAMETER;
    }


    Obj_Handle = GetHandle(drv_p,DecoderObject);
    if(Obj_Handle)
    {
        switch(DecoderObject)
        {
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_DECODER_COMPRESSED2:
                Error = STAud_DecPPSetAudioCodingMode(Obj_Handle,AudioCodingMode_p);
                break;

            case STAUD_OBJECT_POST_PROCESSOR0:
            case STAUD_OBJECT_POST_PROCESSOR1:
            case STAUD_OBJECT_POST_PROCESSOR2:
                Error= STAud_PPSetCodingMode(Obj_Handle,AudioCodingMode_p);
                break;

            default:
                Error=ST_ERROR_INVALID_HANDLE;
                break;
        }
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetAudioCodingMode <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_DRGetAudioCodingMode()

Description:
If possible Get the Audio Coding Mode. Valid for AC3 decoder, and may be later for other decoders.


Parameters:
Handle             Handle to audio decoder.

DecoderObject      References a specific audio decoder.

AudioCodingMode_p  Specifies the Audio Coding Mode pointer.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_FEATURE_NOT_SUPPORTED      The audio decoder does not support the Audio Coding Mode.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

ST_ERROR_BAD_PARAMETER				One or more of the supplied parameters is invalid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRGetAudioCodingMode (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,STAUD_AudioCodingMode_t * AudioCodingMode_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetAudioCodingMode >>> \n"));
    PrintSTAUD_DRGetAudioCodingMode(Handle,DecoderObject,AudioCodingMode_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetAudioCodingMode <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetAudioCodingMode <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(AudioCodingMode_p  == NULL)
    {
        API_Trace((" <<< STAUD_DRGetAudioCodingMode <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }


    Obj_Handle = GetHandle(drv_p,DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetAudioCodingMode(Obj_Handle,AudioCodingMode_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetAudioCodingMode <<< Error %d \n",Error ));
    return Error;
}


/*****************************************************************************
STAUD_DRSetDDPOP()

Description:
 Set the output setting for a DD+ stream.


Parameters:
Handle             Handle to audio decoder.

DecoderObject     References a specific audio decoder.

DDPOPSetting		Specifies the Output setting (Only the first two bits b[1..0] valid for now)

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetDDPOP (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,U32 DDPOPSetting)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetDDPOP >>> \n"));
    PrintSTAUD_DRSetDDPOP(Handle,DecoderObject,DDPOPSetting);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetDDPOP <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetDDPOP <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecSetDDPOP(Obj_Handle, DDPOPSetting);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRSetDDPOP <<< Error %d \n",Error ));
    return Error;

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

ST_ErrorCode_t STAUD_MXUpdatePTSStatus(STAUD_Handle_t Handle,
	STAUD_Object_t MixerObject, U32 InputID, BOOL PTSStatus)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_MXUpdatePTSStatus >>> \n"));
    PrintSTAUD_MXUpdatePTSStatus(Handle,MixerObject,InputID,PTSStatus);
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
        API_Trace((" <<< STAUD_MXUpdatePTSStatus <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,MixerObject);

    if(Obj_Handle)
    {
        Error = STAud_MixerUpdatePTSStatus(Obj_Handle, InputID, PTSStatus);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_MXUpdatePTSStatus <<< Error %d \n",Error ));
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
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_MXUpdateMaster >>> \n"));
    PrintSTAUD_MXUpdateMaster(Handle,MixerObject,InputID,FreeRun);
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

    Obj_Handle = GetHandle(drv_p,MixerObject);

    if(Obj_Handle)
    {
        Error = STAud_MixerUpdateMaster(Obj_Handle, InputID, FreeRun);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_MXUpdateMaster <<< Error %d \n",Error ));
    return 	Error;
}




/*****************************************************************************
STAUD_ENCGetCapability()

Description:
Gets the capability of audio encoder.



Parameters   :
DeviceName      AUDIO driver name
EncoderObject   References the particular encoder object.
Capability_p	Pointer to the encoder capability structure.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_ENCGetCapability(const ST_DeviceName_t DeviceName,
                                        STAUD_Object_t EncoderObject,
                                        STAUD_ENCCapability_t *Capability_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    AudDriver_t	*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_ENCGetCapability >>> \n"));
    PrintSTAUD_ENCGetCapability(DeviceName,EncoderObject,Capability_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_ENCGetCapability <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromName(DeviceName);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_ENCGetCapability <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Capability_p == NULL)
    {
        API_Trace((" <<< STAUD_ENCGetCapability <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	EncoderObject);

    if(Obj_Handle)
    {
        Error = STAud_EncoderGetCapability(Obj_Handle,Capability_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_ENCGetCapability <<< Error %d \n",Error ));
    return (Error);
}

/*****************************************************************************
STAUD_ENCGetOutputParams()

Description:
Gets output parameters of audio encoder.



Parameters   :
Handle				Handle to audio Object.
EncoderObject		References the particular encoder object.
EncoderOPParams_p	Pointer to the encoder output params to be returned.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_ENCGetOutputParams(STAUD_Handle_t Handle,
                                STAUD_Object_t EncoderObject,
                                STAUD_ENCOutputParams_s *EncoderOPParams_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_ENCGetOutputParams >>> \n"));
    PrintSTAUD_ENCGetOutputParams(Handle,EncoderObject,EncoderOPParams_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_ENCGetOutputParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_ENCGetOutputParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,EncoderObject);

    if(Obj_Handle)
    {
        Error = STAud_EncoderGetOutputParams(Obj_Handle, EncoderOPParams_p);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_ENCGetOutputParams <<< Error %d \n",Error));
    return 	Error;
}

/*****************************************************************************
STAUD_ENCSetOutputParams()

Description:
Gets output parameters of audio encoder.



Parameters   :
Handle				Handle to audio Object.
EncoderObject		References the particular encoder object.
EncoderOPParams		Encoder output params to be set to a particular audio encoder.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_ENCSetOutputParams(STAUD_Handle_t Handle,
                                STAUD_Object_t EncoderObject,
                                STAUD_ENCOutputParams_s EncoderOPParams)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_ENCSetOutputParams >>> \n"));
    PrintSTAUD_ENCSetOutputParams(Handle,EncoderObject,EncoderOPParams);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_ENCSetOutputParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_ENCSetOutputParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,EncoderObject);

    if(Obj_Handle)
    {
        Error = STAud_EncoderSetOutputParams(Obj_Handle, EncoderOPParams);
    }
    else
    {
        Error =  ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_ENCSetOutputParams <<< Error %d \n",Error ));
    return 	Error;
}

/*****************************************************************************
STAUD_ModuleStart()
Description:
starts each audio module for which it is called.
Parameters:
Handle 		 IN :        Handle of the Audio Decoder.

ModuleObject IN:		 Object to be started

StreamParams_p IN :        Stream parameters

Return Value:
ST_ERROR_INVALID_HANDLE            The specified handle is invalid.

ST_ERROR_BAD_PARAMETER             One of the supplied parameter is invalid.

ST_NO_ERROR                        Success.

See Also:
STAUD_GenericStart()
*****************************************************************************/


ST_ErrorCode_t STAUD_ModuleStart (STAUD_Handle_t Handle,
STAUD_Object_t ModuleObject,
STAUD_StreamParams_t * StreamParams_p)
{

    ST_ErrorCode_t Error = ST_ERROR_INVALID_HANDLE;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Module_Obj_Handle = 0;
    API_Trace((" >>> STAUD_ModuleStart >>> \n"));
    PrintSTAUD_ModuleStart(Handle,ModuleObject,StreamParams_p);
    if(
    !( 0
#ifdef STAUD_INSTALL_AC3
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_AC3)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_DDPLUS)
#endif
#ifdef STAUD_INSTALL_MPEG
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG1)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG2)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MP3)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG_AAC)
#endif
#ifdef STAUD_INSTALL_HEAAC
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_HE_AAC)
#endif
#if defined(STAUD_INSTALL_LPCMV) || defined(STAUD_INSTALL_LPCMA)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_LPCM)
#endif
#ifdef STAUD_INSTALL_DTS
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_DTS)
#endif
#ifdef STAUD_INSTALL_CDDA
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_CDDA)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_PCM)
#endif
#ifdef STAUD_INSTALL_MLP
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MLP)
#endif
#ifdef STAUD_INSTALL_WMA
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_WMA)
#endif
#ifdef STAUD_INSTALL_WMAPROLSL
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_WMAPROLSL)
#endif
#ifdef STAUD_INSTALL_AAC
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_ADIF)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_MP4_FILE)
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_RAW_AAC)
#endif
#ifdef STAUD_INSTALL_WAVE
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_WAVE)
#endif
#ifdef STAUD_INSTALL_AIFF
    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_AIFF)
#endif

    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_BEEP_TONE)

    ||(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_PINK_NOISE)
    ))
    {
        STTBX_Print(("STAUD_ModuleStart wrong config"));
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

    Module_Obj_Handle = GetHandle(drv_p,ModuleObject);

    if(Module_Obj_Handle)
    {
        switch(ModuleObject)
        {
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error = STAud_SPDIFPlayerSetStreamParams(Module_Obj_Handle, StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("SPDIFPlayer set failed\n"));
                }
                Error |= STAud_SPDIFPlayerSetCmd(Module_Obj_Handle, SPDIFPLAYER_START);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("SPDIFPlayer Start failed\n"));
                }
                break;
            case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
            case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
                Error = STAud_DataProcessorSetStreamParams(Module_Obj_Handle, StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Data Processor set failed\n"));
                }
                Error |= STAud_DataProcesserSetCmd(Module_Obj_Handle, DATAPROCESSER_GET_INPUT_BLOCK);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("DataFormatter Start failed\n"));
                }
                break;
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error = PESESSetAudioType(Module_Obj_Handle,StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PESESS set failed\n"));
                }

                if(Error == ST_NO_ERROR)
                {

                    Error |= SendPES_ES_ParseCmd(Module_Obj_Handle,TIMER_TRANSFER_START);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("PESESS Start failed\n"));
                    }
                }
                break;
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_DECODER_COMPRESSED2:
                Error = STAud_DecSetStreamParams(Module_Obj_Handle,StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Decoder Set failed\n"));
                }
                Error |= STAud_DecStart(Module_Obj_Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Decoder Start failed\n"));
                }
                break;

            case STAUD_OBJECT_ENCODER_COMPRESSED0:
                Error = STAud_EncSetStreamParams(Module_Obj_Handle,StreamParams_p);
                if (Error != ST_NO_ERROR)
                {
                STTBX_Print(("Encoder Set failed\n"));
                }
#if 1

                {
                    BOOL EnableEncOutput = FALSE;

                    STAud_SPDIFPlayerGetEncOutput(GetHandle(drv_p, STAUD_OBJECT_OUTPUT_SPDIF0), &EnableEncOutput);
                    if (EnableEncOutput == TRUE)
                    {
                        STAUD_Handle_t Obj_Handle = 0;
                        STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
                        U32 ObjectCount = 0;

                        ObjectCount = GetClassHandles(Handle,	STAUD_CLASS_POSTPROC, ObjectArray);

                        while(ObjectCount)
                        {
                            ObjectCount--;
                            Obj_Handle = GetHandle(drv_p, ObjectArray[ObjectCount]);
                            switch(ObjectArray[ObjectCount])
                            {
                                case STAUD_OBJECT_POST_PROCESSOR0:
                                case STAUD_OBJECT_POST_PROCESSOR1:
                                case STAUD_OBJECT_POST_PROCESSOR2:
                                    Error = STAud_PPSetOutputStreamType(Obj_Handle, StreamParams_p->StreamContent);
                                    break;
                                default:
                                    Error = ST_ERROR_INVALID_HANDLE;
                                    break;

                            }
                        }
                    }
                }
#endif
                Error |= STAud_EncStart(Module_Obj_Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Encoder Start failed\n"));
                }
                break;

            case STAUD_OBJECT_POST_PROCESSOR0:
            case STAUD_OBJECT_POST_PROCESSOR1:
            case STAUD_OBJECT_POST_PROCESSOR2:
                {
                    STAud_PPStartParams_t StartParams;
                    memset(&StartParams,0,sizeof(STAud_PPStartParams_t));
                    StartParams.Frequency= StreamParams_p->SamplingFrequency;
                    Error = STAud_PPStart(Module_Obj_Handle, &StartParams);
                    if (Error != ST_NO_ERROR)
                    {
                        STTBX_Print(("PostProcessor Start failed\n"));
                    }
                }
                break;
            case STAUD_OBJECT_FRAME_PROCESSOR:
            case STAUD_OBJECT_FRAME_PROCESSOR1:
                {
                    Error = STAud_DataProcessorSetStreamParams(Module_Obj_Handle, StreamParams_p);
                    if (Error != ST_NO_ERROR)
                    {
                    STTBX_Print(("Data Processor set failed\n"));
                    }
                }
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
    API_Trace((" <<< STAUD_ModuleStart <<< Error %d \n",Error ));
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
STAUD_ModuleStart()
*****************************************************************************/

ST_ErrorCode_t STAUD_GenericStart (STAUD_Handle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
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
        API_Trace((" <<< STAUD_GenericStart <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        switch(Unit_p->Object_Instance)
        {
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle,PCMPLAYER_START);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCMPlayer start failed\n"));
                }
                break;

            case STAUD_OBJECT_FRAME_PROCESSOR:
            case STAUD_OBJECT_FRAME_PROCESSOR1:
                Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_GET_INPUT_BLOCK);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("DataFormatter Start failed\n"));
                }
                break;


            case STAUD_OBJECT_INPUT_PCM0:
		Error |= STAud_StartPCMInput(Unit_p->Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Input PCM Start failed\n"));
                }
                break;

            case STAUD_OBJECT_INPUT_PCMREADER0:
                Error |= STAud_PCMReaderSetCmd(Unit_p->Handle, PCMREADER_START);
                break;

            case STAUD_OBJECT_MIXER0:
                Error |= STAud_MixerSetCmd(Unit_p->Handle, MIXER_STATE_START);
                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print((" Mixer start failed \n "));
                }
                break;
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
    API_Trace((" <<< STAUD_GenericStart <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_ModuleStop()
Description:
Stops a specific module.
Parameters:
Handle       :  Handle to audio decoder.

ModuleObject :  Object to be stopped

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_INVALID_DEVICE      The audio decoder is unrecognised or not available
with the current configuration.

See Also:
STAUD_GenericStop()
*****************************************************************************/
ST_ErrorCode_t STAUD_ModuleStop (STAUD_Handle_t Handle,STAUD_Object_t ModuleObject)
{
    ST_ErrorCode_t	Error 	= ST_ERROR_INVALID_HANDLE;
    AudDriver_t	*drv_p 	= NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_ModuleStop >>> \n"));
    PrintSTAUD_ModuleStop(Handle,ModuleObject);
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
        API_Trace((" <<< STAUD_ModuleStop <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    Obj_Handle = GetHandle(drv_p,ModuleObject);

    if(Obj_Handle)
    {
        switch(ModuleObject)
        {
            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                Error = STAud_SPDIFPlayerSetCmd(Obj_Handle, SPDIFPLAYER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("SPDIFPlayer Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
            case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
                Error = STAud_DataProcesserSetCmd(Obj_Handle, DATAPROCESSER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("DataFormatter Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                Error = SendPES_ES_ParseCmd(Obj_Handle,TIMER_STOP);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PESES Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_DECODER_COMPRESSED2:
                Error = STAud_DecStop(Obj_Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Decoder Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_ENCODER_COMPRESSED0:
                Error = STAud_EncStop(Obj_Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Encoder Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_POST_PROCESSOR0:
            case STAUD_OBJECT_POST_PROCESSOR1:
            case STAUD_OBJECT_POST_PROCESSOR2:
                Error = STAud_PPStop(Obj_Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PP Stop failed\n"));
                }
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
    API_Trace((" <<< STAUD_ModuleStop <<< Error %d \n" ,Error));
    return Error;

}

/*****************************************************************************
STAUD_GenericStop()
Description:
Stops all modules that can't be started by STAUD_ModuleStop().
Parameters:
Handle       :  Handle to audio decoder.

Return Value:
ST_NO_ERROR                     No error.

ST_ERROR_INVALID_HANDLE         The handle is not valid.

STAUD_ERROR_INVALID_DEVICE      The audio decoder is unrecognised or not available
with the current configuration.

See Also:
STAUD_ModuleStop()
*****************************************************************************/
ST_ErrorCode_t STAUD_GenericStop(STAUD_Handle_t Handle)
{
    ST_ErrorCode_t					Error = ST_NO_ERROR;
    AudDriver_t						*drv_p = NULL;
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
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                Error |= STAud_PCMPlayerSetCmd(Unit_p->Handle,PCMPLAYER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCMPlayer Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_FRAME_PROCESSOR:
            case STAUD_OBJECT_FRAME_PROCESSOR1:
                Error |= STAud_DataProcesserSetCmd(Unit_p->Handle, DATAPROCESSER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("DataFormatter Stop failed\n"));
                }
                break;

            case STAUD_OBJECT_INPUT_PCM0:
		Error |= STAud_StopPCMInput(Unit_p->Handle);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Input PCM Stop failed\n"));
                }
                break;
            case STAUD_OBJECT_INPUT_PCMREADER0:
                Error |= STAud_PCMReaderSetCmd(Unit_p->Handle, PCMREADER_STOPPED);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCM READER stop failed\n"));
                }
                break;

            case STAUD_OBJECT_MIXER0:
                Error |= STAud_MixerSetCmd(Unit_p->Handle, MIXER_STATE_STOP);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Mixer Stop failed\n"));
                }
                break;

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
    API_Trace((" <<< STAUD_GenericStop <<< Error %d \n",Error ));
    return Error;

}

/*****************************************************************************
STAUD_SetModuleAttenuation()

Description:
Sets the volume attenuation applied to the output channels.

Parameters:
Handle        :   Handle to audio output.

ModuleObject  :   References a specific audio driver module
Attenuation_p :   Specifies attenuation to apply to output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_SetModuleAttenuation (STAUD_Handle_t Handle,
STAUD_Object_t ModuleObject,
STAUD_Attenuation_t * Attenuation_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_SetModuleAttenuation >>> \n"));
    PrintSTAUD_SetModuleAttenuation(Handle,ModuleObject,Attenuation_p);
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
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p, ModuleObject);

    if(Obj_Handle)
    {
        switch (ModuleObject & 0xffff0000)
        {
            case STAUD_CLASS_DECODER:
                Error |= STAud_DecPPSetVol(Obj_Handle, Attenuation_p);
                break;
            case STAUD_CLASS_POSTPROC:
                Error |= STAud_PPSetVolParams(Obj_Handle,Attenuation_p);
                break;
            case STAUD_CLASS_MIXER:
                Error |= STAud_MixerSetVol(Obj_Handle, Attenuation_p);
                break;
            default:
                Error = ST_ERROR_INVALID_HANDLE;

        }
    }
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_SetModuleAttenuation <<< Error %d \n",Error ));
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
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_GetModuleAttenuation >>> \n"));
    PrintSTAUD_GetModuleAttenuation(Handle,ModuleObject,Attenuation_p);

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

    if(Attenuation_p==NULL)
    {
        API_Trace((" <<< STAUD_GetModuleAttenuation <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }

    Obj_Handle = GetHandle(drv_p,	ModuleObject);

    if(Obj_Handle)
    {
        switch (ModuleObject & 0xffff0000)
        {
            case STAUD_CLASS_DECODER:
                Error |= STAud_DecPPGetVol(Obj_Handle,Attenuation_p);
                break;
            case STAUD_CLASS_POSTPROC:
                Error |= STAud_PPGetVolParams(Obj_Handle,Attenuation_p);
                break;
            case STAUD_CLASS_MIXER:
                Error |= STAud_MixerGetVol(Obj_Handle,Attenuation_p);
                break;
            default:
                Error = ST_ERROR_INVALID_HANDLE;

        }

    }
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_GetModuleAttenuation <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRSetDTSNeoParams()

Description:
Sets the DTS Neo Params

Parameters:
Handle           Handle to audio output.

PostProcObject     References a specific output interface

Attenuation_p    Specifies attenuation to apply to output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

ST_ERROR_BAD_PARAMETERS   One of the Suppiled Parameter is bad

See Also:
*****************************************************************************/

  ST_ErrorCode_t STAUD_DRSetDTSNeoParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t DecoderObject,
                                          STAUD_DTSNeo_t *DTSNeo_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRSetDTSNeoParams >>> \n"));
    PrintSTAUD_DRSetDTSNeoParams(Handle,DecoderObject,DTSNeo_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRSetDTSNeoParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetDTSNeoParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (DTSNeo_p == NULL)
    {
        API_Trace((" <<< STAUD_DRSetDTSNeoParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }



    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPSetDTSNeo(Obj_Handle,DTSNeo_p);
    }
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_DRSetDTSNeoParams <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRSetDTSNeoParams()

Description:
Sets the DTS Neo Params

Parameters:
Handle           Handle to audio output.

PostProcObject     References a specific output interface

Attenuation_p    Specifies attenuation to apply to output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

ST_ERROR_BAD_PARAMETERS   One of the Suppiled Parameter is bad

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_DRGetDTSNeoParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t DecoderObject,
                                          STAUD_DTSNeo_t *DTSNeo_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    /*STAudPP_Channel_t   ChannelAtt;*/
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_DRGetDTSNeoParams >>> \n"));
    PrintSTAUD_DRGetDTSNeoParams(Handle,DecoderObject,DTSNeo_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetDTSNeoParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_DRGetDTSNeoParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(DTSNeo_p==NULL)
    {
        API_Trace((" <<< STAUD_DRGetDTSNeoParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }


    Obj_Handle = GetHandle(drv_p,	DecoderObject);

    if(Obj_Handle)
    {
        Error = STAud_DecPPGetDTSNeo(Obj_Handle,DTSNeo_p);
    }
    else
    {
        API_Trace((" <<< STAUD_DRGetDTSNeoParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
    API_Trace((" <<< STAUD_DRGetDTSNeoParams <<< Error %d \n",Error ));


    return Error;
}


/*****************************************************************************
STAUD_DRBTSCParams()

Description:
Sets the BTSC Params

Parameters:
Handle           Handle to audio output.

PostProcObject     References a specific output interface

Attenuation_p    Specifies attenuation to apply to output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

ST_ERROR_BAD_PARAMETERS   One of the Suppiled Parameter is bad

See Also:
*****************************************************************************/

  ST_ErrorCode_t STAUD_PPSetBTSCParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t PostProObject,
                                          STAUD_BTSC_t *BTSC_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPSetBTSCParams >>> \n"));
    PrintSTAUD_PPSetBTSCParams(Handle,PostProObject,BTSC_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPSetBTSCParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPSetBTSCParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (BTSC_p == NULL)
    {
        API_Trace((" <<< STAUD_PPSetBTSCParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }



    Obj_Handle = GetHandle(drv_p,	PostProObject);

    if(Obj_Handle)
    {
        Error = STAud_PPSetBTSCParams(Obj_Handle,BTSC_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPSetBTSCParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_PPSetBTSCParams <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_DRGetBTSCParams()

Description:
Get the BTSCParams

Parameters:
Handle           Handle to audio output.

PostProcObject     References a specific output interface

Attenuation_p    Specifies attenuation to apply to output channels.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

ST_ERROR_BAD_PARAMETERS   One of the Suppiled Parameter is bad

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_PPGetBTSCParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t PostProObject,
                                          STAUD_BTSC_t *BTSC_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    STAUD_Handle_t Obj_Handle = 0;
    API_Trace((" >>> STAUD_PPGetBTSCParams >>> \n"));
    PrintSTAUD_PPGetBTSCParams(Handle,PostProObject,BTSC_p);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_PPGetBTSCParams <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);

    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_PPGetBTSCParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(BTSC_p==NULL)
    {
        API_Trace((" <<< STAUD_PPGetBTSCParams <<< Error ST_ERROR_BAD_PARAMETER \n" ));
        return ST_ERROR_BAD_PARAMETER;
    }


    Obj_Handle = GetHandle(drv_p,	PostProObject);

    if(Obj_Handle)
    {
        Error = STAud_PPGetBTSCParams(Obj_Handle,BTSC_p);

    }
    else
    {
        API_Trace((" <<< STAUD_PPGetBTSCParams <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }

    API_Trace((" <<< STAUD_PPGetBTSCParams <<< Error %d \n",Error ));
    return Error;
}

/*****************************************************************************
STAUD_SetScenario()

Description:
Set the audio scenario for 7200 chipset

Parameters:
Handle           Handle to audio driver.

Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     Handle invalid or not open.

ST_ERROR_BAD_PARAMETERS   One of the Suppiled Parameter is bad

See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_SetScenario(STAUD_Handle_t Handle, STAUD_Scenario_t Scenario)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    AudDriver_t		*drv_p = NULL;
    API_Trace((" >>> STAUD_SetScenario >>> \n"));
    PrintSTAUD_SetScenario(Handle,Scenario);
    if(InitSemaphore_p)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        API_Trace((" <<< STAUD_SetScenario <<< Error ST_ERROR_UNKNOWN_DEVICE \n" ));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    drv_p = AudDri_GetBlockFromHandle(Handle);
    STOS_SemaphoreSignal(InitSemaphore_p);
    if(drv_p == NULL)
    {
        API_Trace((" <<< STAUD_SetScenario <<< Error ST_ERROR_INVALID_HANDLE \n" ));
        return ST_ERROR_INVALID_HANDLE;
    }
#if defined(ST_7200)
    Error = STAud_OPSetScenario(Scenario);
#else
    Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif
    API_Trace((" <<< STAUD_SetScenario <<< Error %d  \n",Error ));
    return Error;
}


U32	GetClassHandles(STAUD_Handle_t Handle,U32	ObjectClass, STAUD_Object_t *ObjectArray)
{
    STAUD_DrvChainConstituent_t *Unit_p = NULL;
    U32 ObjectCount = 0;
    AudDriver_t *drv_p;


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



