/************************************************************************
COPYRIGHT STMicroelectronics (C) 2004
Source file name : staud_core.c
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifndef ST_OSLINUX
    #include "sttbx.h"
#endif

#define ST_OPTIMIZE_BUFFER 1
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
#include "staud_driverconfig.h"
#include "audio_decodertypes.h"
#include "aud_evt.h"
/*Puting Initial Maximux size for MPEG
But on the run time we will get the size from Audio Bin */
U32 MAX_SAMPLES_PER_FRAME                   = 1152;
static  U32 MAX_INPUT_COMPRESSED_FRAME_SIZE = 1152;

extern ST_DeviceName_t MemPool_DeviceName[];
extern ST_DeviceName_t PES_ES_DeviceName[];
extern ST_DeviceName_t Decoder_DeviceName[];
extern ST_DeviceName_t Encoder_DeviceName[];
extern ST_DeviceName_t PostProcss_DeviceName[];
extern ST_DeviceName_t PCMPlayer_DeviceName[];
extern ST_DeviceName_t SPDIFPlayer_DeviceName[];
extern ST_DeviceName_t PCMReader_DeviceName[];
extern ST_DeviceName_t Mixer_DeviceName[];

#define MIXER_INPUT_FROM_FILE       0
#define NUM_PARSER                  3
#define NUM_DECODER                 3
#define NUM_ENCODER                 1
#define NUM_MIXER                   1
#define NUM_PCMPLAYER               5       // pcm player + 1 hdmi pcm
#define NUM_SPDIFPLAYER             2   // 1 spdif + 1 hdmi spdif
#define NUM_DATAPROCESSOR           4   // 1byte swapper+ 1 byte converter + 2 frame processor
#define NUM_POSTPROCESSOR           3
#define NUM_PCMINPUT                1
#define NUM_PCMREADER               1

PESESInitParams_t                   *PESESInitParams[NUM_PARSER];
STAud_DecInitParams_t               *DecInitParams[NUM_DECODER];
STAud_EncInitParams_t               *EncInitParams[NUM_ENCODER];
STAud_MixInitParams_t               *MixerInitParams[NUM_MIXER];
PCMPlayerInitParams_t               *PCMPInitParams[NUM_PCMPLAYER];
SPDIFPlayerInitParams_t             *SPDIFPInitParams[NUM_SPDIFPLAYER];
DataProcesserInitParams_t           *DataPInitParams[NUM_DATAPROCESSOR];
STAud_PPInitParams_t                *PPInitParams[NUM_POSTPROCESSOR];
STAudPCMInputInit_t                 *PCMIInitParams[NUM_PCMINPUT];
PCMReaderInitParams_t               *PCMRInitParams[NUM_PCMREADER];


/* Queue of initialized Decoders */
static AudDriver_t *AudDriverHead_p = NULL;

/*----------------------------------------------------------------------------
Audio Chain Creations functions
-----------------------------------------------------------------------------*/

static ST_ErrorCode_t       InitDependentModuleParams(ObjectSet CurObjectSet, STMEMBLK_Handle_t *mem_handle, STAUD_InitParams_t *InitParams_p);
#if defined(ST_OPTIMIZE_BUFFER)
    static ST_ErrorCode_t   GetNumOfBlocks(ObjectSet    CurObjectSet, U32 *NumBlock);
#endif
static U32                  GetModuleIdentifier(STAUD_Object_t CurObject);
static U32                  GetModuleOffset(STAUD_Object_t CurObject);
static ST_ErrorCode_t       GetOutputPageSize(ObjectSet CurObjectSet, U32 *OutputPageSize);
static ST_ErrorCode_t       InitCurrentModule(STAUD_Object_t ObjectID, STAUD_InitParams_t *InitParams_p, STMEMBLK_Handle_t *mem_handle, AudDriver_t *audDriver_p);
static ST_ErrorCode_t       InitMemoryPool(U32 PoolAllocationCount, U32 NumBlocks, U32 BlockSize, BOOL AllocateFromEMBX,    STAVMEM_PartitionHandle_t AVMEMPartition,
                   ST_Partition_t *CPUPartition_p, STMEMBLK_Handle_t * mem_handle, AudDriver_t *audDriver_p);
static ST_ErrorCode_t       GetFirstFreeMemBlock(U8 *PoolAllocationCount_p);

static ST_ErrorCode_t       AddToDriver(AudDriver_t *AudDriver_p,STAUD_Object_t Object_Instance, STAUD_Handle_t Handle);

static ST_ErrorCode_t       AddToTopofDriver(AudDriver_t    *AudDriver_p,STAUD_Object_t Object_Instance, STAUD_Handle_t Handle);


/*****************************************************************************
AudDri_QueueAdd()
Description:
This routine appends an allocated decoder control block to the
queue.

NOTE: The semaphore lock must be held when calling this routine.

Parameters:
Item_p, the control block to append to the queue.
Return Value:
Nothing.
See Also:
AudDri_QueueRemove()
*****************************************************************************/
void AudDri_QueueAdd(AudDriver_t *Item_p)
{

/* Check the base element is defined, otherwise, append to the end of the
* linked list.
*/
    if(Item_p)
    {
        if (AudDriverHead_p == NULL)
        {
            /* As this is the first item, no need to append */
            AudDriverHead_p = Item_p;
        }
        else
        {
            /* Iterate along the list until we reach the final item */
            AudDriver_t *qp;

            qp = AudDriverHead_p;
            while (qp->Next_p)
            {
                qp = qp->Next_p;
            }

            /* Append the new item */
            qp->Next_p = Item_p;
        }

        Item_p->Next_p = NULL;
    }
} /*AudDri_QueueAdd*/

/*****************************************************************************
AudDri_IsInit()
Description:
Runs through the queue of initialized objects and checks that
the object with the specified device name has not already been added.

NOTE: Must be called with the queue semaphore held.

Parameters:
Name, text string indicating the device name to check.
Return Value:
TRUE, the device has already been initialized.
FALSE, the device is not in the queue and therefore is not initialized.
See Also:
AudDri_Init()
*****************************************************************************/
BOOL AudDri_IsInit(const ST_DeviceName_t Name)
{
    BOOL Initialized = FALSE;
    AudDriver_t *qp = AudDriverHead_p; /* Global queue head pointer */

    while (qp)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(qp->Name, Name) != 0)
        {
            /* Next Mem Pool in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The object is already initialized */
            Initialized = TRUE;
            break;
        }
    }

    /* Boolean return value reflecting whether the device is initialized */
    return Initialized;
} /* AudDri_IsInit() */


/*****************************************************************************
AudDri_QueueRemove()
Description:
This routine removes an allocated Decoder control block from the
Parser queue.

NOTE: The semaphore lock must be held when calling this routine.
Use PESES_IsInit() or PESES_GetControlBlockFromName() to verify whether
or not a particular item is in the queue prior to calling this routine.

Parameters:
Item_p, the control block to remove from the queue.
Return Value:
Nothing.
See Also:
AudDri_QueueAdd()
*****************************************************************************/
void AudDri_QueueRemove(AudDriver_t *Item_p)
{
    AudDriver_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
    * present on the queue.
    */
    if (AudDriverHead_p && Item_p)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = AudDriverHead_p;

        /* Iterate over each queue element until we find the required
        * queue item to remove.
        */
        while (this_p && (this_p != Item_p))
        {
            last_p = this_p;
            this_p = this_p->Next_p;
        }

        /* Check we found the queue item */
        if (this_p == Item_p)
        {
            /* Unlink the item from the queue */
            if (last_p)
            {
                last_p->Next_p = this_p->Next_p;
            }
            else
            {
                /* Recalculate the new head of the queue i.e.,
                * we have just removed the queue head.
                */
                AudDriverHead_p = this_p->Next_p;
            }
        }
    }
}/*AudDri_QueueRemove*/

/*****************************************************************************
AudDri_GetBlockFromName()
Description:
Runs through the queue of initialized objects and checks for
the Mem Pool with the specified name.

NOTE: Must be called with the queue semaphore held.

Parameters:
Name, text string indicating the device name to check.
Return Value:
NULL, end of queue encountered - invalid device name.
Otherwise, returns a pointer to the control block for the device.
See Also:
PESES_IsInit()
*****************************************************************************/

AudDriver_t *AudDri_GetBlockFromName(const ST_DeviceName_t Name)
{
    AudDriver_t *qp = AudDriverHead_p; /* Global queue head pointer */

    while (qp)
    {
        /* Check the name for a match with the item in the queue */
        if (strcmp(qp->Name, Name) != 0)
        {
            /* Next object in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The entry has been found */
            break;
        }
    }

    /* Return the control block (or NULL) to the caller */
    return qp;

}/*AudDri_GetBlockFromName()*/

/*****************************************************************************
AudDri_GetBlockFromHandle()
Description:
Runs through the queue of initialized objects and checks for
the object with the specified handle.

NOTE: Must be called with the queue semaphore held.

Parameters:
Handle, an open handle.
Return Value:
NULL, end of queue encountered - invalid handle.
Otherwise, returns a pointer to the control block for the device.
See Also:
    AudDri_GetBlockFromName()
*****************************************************************************/
AudDriver_t *AudDri_GetBlockFromHandle(const STAUD_Handle_t Handle)
{
    AudDriver_t *qp = AudDriverHead_p; /* Global queue head pointer */

    if(Handle)
    {
        /* Iterate over the Mem Pool queue */
        while (qp)
        {
            /* Check for a matching handle */
            if (Handle == (STAUD_Handle_t)qp)
            {
                break;  /* This is a valid handle */
            }

            /* Try the next object */
            qp = qp->Next_p;
        }
    }
    else
    {
        qp = NULL;
    }

    if(qp)
    {
        qp=(qp->Valid)?qp:NULL;
    }

    return qp;
} /*PESES_GetControlBlockFromHandle()*/


static ST_ErrorCode_t AddToDriver(AudDriver_t   *AudDriver_p,STAUD_Object_t Object_Instance, STAUD_Handle_t Handle)
{
    STAUD_DrvChainConstituent_t *element ;
    STAUD_DrvChainConstituent_t *Temp;

    if(AudDriver_p)
    {
        Temp = (STAUD_DrvChainConstituent_t *)STOS_MemoryAllocate(AudDriver_p->CPUPartition_p,sizeof(STAUD_DrvChainConstituent_t));

        if(!Temp)
        {
            return ST_ERROR_NO_MEMORY;
        }

        memset(Temp,0,sizeof(STAUD_DrvChainConstituent_t));
        Temp->Handle            = Handle;
        Temp->Object_Instance   = Object_Instance;

        if((U32)AudDriver_p->Handle == AUDIO_INVALID_HANDLE)
        {
            AudDriver_p->Handle = Temp;
        }
        else
        {
            element = AudDriver_p->Handle;
            while(element->Next_p != NULL)
            {
                element = element->Next_p;
            }
            element->Next_p = Temp;
        }
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    return ST_NO_ERROR;
}

static ST_ErrorCode_t AddToTopofDriver(AudDriver_t  *AudDriver_p,STAUD_Object_t Object_Instance, STAUD_Handle_t Handle)
{
    STAUD_DrvChainConstituent_t *element ;
    STAUD_DrvChainConstituent_t *Temp;

    if(AudDriver_p)
    {
        Temp = (STAUD_DrvChainConstituent_t *)STOS_MemoryAllocate(AudDriver_p->CPUPartition_p,sizeof(STAUD_DrvChainConstituent_t));

        if(!Temp)
        {
            return ST_ERROR_NO_MEMORY;
        }

        memset(Temp,0,sizeof(STAUD_DrvChainConstituent_t));
        Temp->Handle                = Handle;
        Temp->Object_Instance   = Object_Instance;

        if((U32)AudDriver_p->Handle == AUDIO_INVALID_HANDLE)
        {
            AudDriver_p->Handle = Temp;
        }
        else
        {
            element = AudDriver_p->Handle;
            AudDriver_p->Handle = Temp; /*insert at the top*/
            Temp->Next_p = element;     /*push the stack down*/
        }
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }
    return ST_NO_ERROR;
}



ST_ErrorCode_t RemoveFromDriver(AudDriver_t *AudDriver_p,STAUD_Handle_t Handle)
{
    STAUD_DrvChainConstituent_t *this_p, *last_p ;

    if(AudDriver_p)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = AudDriver_p->Handle;
        if(this_p->Handle != AUDIO_INVALID_HANDLE)
        {
            /* Iterate over each queue element until we find the required
            * queue item to remove.
            */
            while(this_p->Handle != Handle)
            {
                last_p = this_p;
                this_p = this_p->Next_p;
            }

            /* Check we found the queue item */
            if (this_p->Handle == Handle)
            {
                /* Unlink the item from the queue */
                if (last_p != NULL)
                {
                    last_p->Next_p = this_p->Next_p;
                }
                else
                {
                    /* Recalculate the new head of the queue i.e.,
                    * we have just removed the queue head.*/
                    AudDriver_p->Handle = this_p->Next_p;
                }

                memset(this_p,0,sizeof(STAUD_DrvChainConstituent_t));
                /*Now Deallocate the memory*/
                STOS_MemoryDeallocate(AudDriver_p->CPUPartition_p,this_p);
            }
        }
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    return ST_NO_ERROR;
}


/*
******************************************************************************
* Internal  Functions
******************************************************************************/
STAUD_Handle_t  GetHandle(AudDriver_t *drv_p,STAUD_Object_t Object)
{
    STAUD_DrvChainConstituent_t *Unit_p = NULL;

    if(drv_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    Unit_p = drv_p->Handle;
    while(Unit_p)
    {
        if(Unit_p->Object_Instance == Object)
        {
            return Unit_p->Handle;
        }

    Unit_p = Unit_p->Next_p;
    }

    return 0;
}

/**************************************************************************************
PRIVATE FUNCTIONS
**************************************************************************************/
static ST_ErrorCode_t   InitMemoryPool( U32 PoolAllocationCount, U32 NumBlocks, U32 BlockSize, BOOL AllocateFromEMBX,
                                        STAVMEM_PartitionHandle_t AVMEMPartition, ST_Partition_t *CPUPartition_p,
                                        STMEMBLK_Handle_t * mem_handle, AudDriver_t *audDriver_p)
{
    MemPoolInitParams_t mem_InitParams;
    ST_ErrorCode_t Error            = ST_NO_ERROR;
    memset(&mem_InitParams,0,sizeof(MemPoolInitParams_t));
    /*Allocate buffer pool */
    mem_InitParams.NumBlocks        = NumBlocks;
    mem_InitParams.BlockSize        = BlockSize;
    mem_InitParams.AllocateFromEMBX = AllocateFromEMBX;
    mem_InitParams.AVMEMPartition   = AVMEMPartition;
    mem_InitParams.CPUPartition_p   = CPUPartition_p;

    Error = MemPool_Init(MemPool_DeviceName[PoolAllocationCount], &mem_InitParams, mem_handle);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print((" %d MemPool initialtion failed \n", PoolAllocationCount));
    }
    else
    {
        Error |= AddToDriver(audDriver_p, STAUD_OBJECT_BLCKMGR, *mem_handle);
    }

    return Error;
}

static U32 GetModuleOffset(STAUD_Object_t CurObject)
{
    U32 Offset = 0;

    switch (CurObject)
    {
        case STAUD_OBJECT_INPUT_CD0:
        case STAUD_OBJECT_DECODER_COMPRESSED0:
        case STAUD_OBJECT_POST_PROCESSOR0:
        case STAUD_OBJECT_OUTPUT_PCMP0:
        case STAUD_OBJECT_OUTPUT_SPDIF0:
        case STAUD_OBJECT_INPUT_PCMREADER0:
        case STAUD_OBJECT_INPUT_PCM0:
        case STAUD_OBJECT_ENCODER_COMPRESSED0:
        case STAUD_OBJECT_MIXER0:
        case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
            Offset = 0;
            break;

        case STAUD_OBJECT_INPUT_CD1:
        case STAUD_OBJECT_DECODER_COMPRESSED1:
        case STAUD_OBJECT_POST_PROCESSOR1:
        case STAUD_OBJECT_OUTPUT_PCMP1:
        case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
        case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
            Offset = 1;
            break;

        case STAUD_OBJECT_INPUT_CD2:
        case STAUD_OBJECT_DECODER_COMPRESSED2:
        case STAUD_OBJECT_POST_PROCESSOR2:
        case STAUD_OBJECT_OUTPUT_PCMP2:
        case STAUD_OBJECT_FRAME_PROCESSOR:
            Offset = 2;
            break;

        case STAUD_OBJECT_OUTPUT_PCMP3:
        case STAUD_OBJECT_FRAME_PROCESSOR1:
            Offset = 3;
            break;

        case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
            Offset = 4;
            break;

        default:
            break;
    }
    return Offset;
}

static U32 GetModuleIdentifier(STAUD_Object_t CurObject)
{

    U32 Identifier = 0;

    switch (CurObject)
    {
        case STAUD_OBJECT_OUTPUT_PCMP0:
            Identifier = PCM_PLAYER_0;
            break;

        case STAUD_OBJECT_OUTPUT_PCMP1:
            Identifier = PCM_PLAYER_1;
            break;

        case STAUD_OBJECT_OUTPUT_PCMP2:
            Identifier = PCM_PLAYER_2;
            break;

        case STAUD_OBJECT_OUTPUT_PCMP3:
            Identifier = PCM_PLAYER_3;
            break;

        case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
            Identifier = HDMI_PCM_PLAYER_0;
            break;

        case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
            Identifier = BYTE_SWAPPER;
            break;

        case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
            Identifier = BIT_CONVERTER;
            break;

        case STAUD_OBJECT_FRAME_PROCESSOR:
            Identifier = FRAME_PROCESSOR;
            break;

        case STAUD_OBJECT_FRAME_PROCESSOR1:
            Identifier = FRAME_PROCESSOR1;
            break;

        case STAUD_OBJECT_OUTPUT_SPDIF0:
            Identifier = SPDIF_PLAYER_0;
            break;

        case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
            Identifier = HDMI_SPDIF_PLAYER_0;
            break;

        case STAUD_OBJECT_INPUT_PCMREADER0:
            Identifier = PCM_READER_0;
            break;

        default:
            break;
    }
    return Identifier;
}

static ST_ErrorCode_t   InitCurrentModule(STAUD_Object_t ObjectID, STAUD_InitParams_t *InitParams_p, STMEMBLK_Handle_t *mem_handle, AudDriver_t *audDriver_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 ModuleOffset = 0;

    switch(ObjectID)
    {
        case STAUD_OBJECT_INPUT_CD0:
        case STAUD_OBJECT_INPUT_CD1:
        case STAUD_OBJECT_INPUT_CD2:
        {
            STPESES_Handle_t        PESESHandle;
            ModuleOffset = GetModuleOffset(ObjectID);
            if (PESESInitParams[ModuleOffset] == NULL)
            {
                PESESInitParams[ModuleOffset] = (PESESInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(PESESInitParams_t));

                if(!PESESInitParams[ModuleOffset])
                {
                    return ST_ERROR_NO_MEMORY;
                }
                memset(PESESInitParams[ModuleOffset],0,sizeof(PESESInitParams_t));
            }

            PESESInitParams[ModuleOffset]->PESbufferSize        = PES_BUFFER_SIZE;

            PESESInitParams[ModuleOffset]->OpBufHandle          = mem_handle[0];
            PESESInitParams[ModuleOffset]->AVMEMPartition       = InitParams_p->AVMEMPartition;

            PESESInitParams[ModuleOffset]->AudioBufferPartition = (InitParams_p->AllocateFromEMBX)?0:InitParams_p->BufferPartition;

            PESESInitParams[ModuleOffset]->CPUPartition_p       = InitParams_p->CPUPartition_p;
            PESESInitParams[ModuleOffset]->EnableMSPPSupport    =  InitParams_p->EnableMSPPSupport;
            strncpy(PESESInitParams[ModuleOffset]->EvtHandlerName, InitParams_p->EvtHandlerName,ST_MAX_DEVICE_NAME_TOCOPY);
            strncpy(PESESInitParams[ModuleOffset]->Name,PES_ES_DeviceName[ModuleOffset],ST_MAX_DEVICE_NAME_TOCOPY);

            #ifdef STAUD_COMMON_EVENTS
                strncpy(PESESInitParams[ModuleOffset]->EvtGeneratorName,InitParams_p->RegistrantDeviceName,ST_MAX_DEVICE_NAME_TOCOPY);
            #else
                strncpy(PESESInitParams[ModuleOffset]->EvtGeneratorName,PES_ES_DeviceName[ModuleOffset],ST_MAX_DEVICE_NAME_TOCOPY);
            #endif

            #ifndef STAUD_REMOVE_CLKRV_SUPPORT
                PESESInitParams[ModuleOffset]->CLKRV_Handle             = audDriver_p->CLKRV_Handle;
                /* CLKRV_Handle should be different from clock and synchro */
                PESESInitParams[ModuleOffset]->CLKRV_HandleForSynchro   = audDriver_p->CLKRV_HandleForSynchro;
            #endif

        PESESInitParams[ModuleOffset]->ObjectID = ObjectID;//STAUD_OBJECT_INPUT_CD0;
        error = PESES_Init(PES_ES_DeviceName[ModuleOffset], PESESInitParams[ModuleOffset], &PESESHandle);

        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("PESES initialtion failed\n"));
        }
        else
        {
            error = AddToTopofDriver(audDriver_p, ObjectID,PESESHandle);
        }

        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PESESInitParams[ModuleOffset]);
        PESESInitParams[ModuleOffset] = NULL;
    }
    break;

    case STAUD_OBJECT_INPUT_PCM0:
    {
        STPCMInput_Handle_t PCMInputHandle;
        ModuleOffset                        = GetModuleOffset(ObjectID);
        if(PCMIInitParams[ModuleOffset] == NULL)
        {
            PCMIInitParams[ModuleOffset]    = (STAudPCMInputInit_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAudPCMInputInit_t));
            if(!PCMIInitParams[ModuleOffset])
            {
                return ST_ERROR_NO_MEMORY;
            }
            memset(PCMIInitParams[ModuleOffset],0,sizeof(STAudPCMInputInit_t));
        }
        PCMIInitParams[ModuleOffset]->CPUPartition_p = InitParams_p->CPUPartition_p;
        PCMIInitParams[ModuleOffset]->AVMEMPartition = InitParams_p->AVMEMPartition;
        PCMIInitParams[ModuleOffset]->PCMBlockHandle = mem_handle[0];
        PCMIInitParams[ModuleOffset]->NumChannels    = InitParams_p->NumChannels;

        error = STAud_InitPCMInput(PCMIInitParams[ModuleOffset], &PCMInputHandle);
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("STAud_Input_pcm%d initialtion failed\n",ModuleOffset));
        }
        else
        {
            error = AddToTopofDriver(audDriver_p, ObjectID, PCMInputHandle);
        }
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PCMIInitParams[ModuleOffset]);
        PCMIInitParams[ModuleOffset] = NULL;

        break;
    }

    case STAUD_OBJECT_INPUT_PCMREADER0:
    {
        STPCMREADER_Handle_t PCMReaderhandle;
        ModuleOffset = GetModuleOffset(ObjectID);

        if(PCMRInitParams[ModuleOffset] == NULL)
        {
            PCMRInitParams[ModuleOffset] = (PCMReaderInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(PCMReaderInitParams_t));

            if(!PCMRInitParams[ModuleOffset])
            {
                return ST_ERROR_NO_MEMORY;
            }
            memset(PCMRInitParams[ModuleOffset],0,sizeof(PCMReaderInitParams_t));
        }

        PCMRInitParams[ModuleOffset]->AVMEMPartition                = InitParams_p->AVMEMPartition;
        PCMRInitParams[ModuleOffset]->CPUPartition                  = InitParams_p->CPUPartition_p;

        #ifdef STAUD_COMMON_EVENTS
            strncpy(PCMRInitParams[ModuleOffset]->EvtGeneraterName,InitParams_p->RegistrantDeviceName,ST_MAX_DEVICE_NAME_TOCOPY);
        #else
            strncpy(PCMRInitParams[ModuleOffset]->EvtGeneraterName,PCMReader_DeviceName[ModuleOffset],ST_MAX_DEVICE_NAME_TOCOPY);
        #endif

        strncpy(PCMRInitParams[ModuleOffset]->EvtHandlerName, InitParams_p->EvtHandlerName, ST_MAX_DEVICE_NAME_TOCOPY);
        PCMRInitParams[ModuleOffset]->memoryBlockManagerHandle      = mem_handle[0];
        PCMRInitParams[ModuleOffset]->NumChannels                   = InitParams_p->NumChPCMReader;
        PCMRInitParams[ModuleOffset]->pcmReaderDataConfig           = InitParams_p->PCMReaderMode;
        PCMRInitParams[ModuleOffset]->pcmReaderIdentifier           = GetModuleIdentifier(ObjectID);//PCM_READER_0;
        PCMRInitParams[ModuleOffset]->pcmReaderNumNode              = 6;
        PCMRInitParams[ModuleOffset]->Frequency                     = InitParams_p->PCMReaderMode.Frequency;

        error = STAud_PCMReaderInit(PCMReader_DeviceName[ModuleOffset], &PCMReaderhandle, PCMRInitParams[ModuleOffset]);
        if(error != ST_NO_ERROR)
        {
            STTBX_Print(("STAud_PCMReaderInit initialtion failed\n"));
        }
        else
        {
            error = AddToTopofDriver(audDriver_p, ObjectID, PCMReaderhandle);
        }

        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PCMRInitParams[ModuleOffset]);
        PCMRInitParams[ModuleOffset] = NULL;

        break;
    }

    case STAUD_OBJECT_DECODER_COMPRESSED0:
    case STAUD_OBJECT_DECODER_COMPRESSED1:
    case STAUD_OBJECT_DECODER_COMPRESSED2:
    {
        STAud_DecHandle_t       DecoderHandle;
        ModuleOffset = GetModuleOffset(ObjectID);

        if (DecInitParams[ModuleOffset]== NULL)
        {
            DecInitParams[ModuleOffset] = (STAud_DecInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAud_DecInitParams_t));
            if(!DecInitParams[ModuleOffset])
            {
                return ST_ERROR_NO_MEMORY;
            }
            memset(DecInitParams[ModuleOffset],0,sizeof(STAud_DecInitParams_t));
        }

        DecInitParams[ModuleOffset]->AVMEMPartition     = InitParams_p->AVMEMPartition;
        DecInitParams[ModuleOffset]->DriverPartition    = InitParams_p->CPUPartition_p;
        DecInitParams[ModuleOffset]->OutBufHandle0      = mem_handle[0];
        DecInitParams[ModuleOffset]->OutBufHandle1      = mem_handle[1];
        DecInitParams[ModuleOffset]->NumChannels        =   InitParams_p->NumChannels;

        error = STAud_DecInit(Decoder_DeviceName[ModuleOffset],DecInitParams[ModuleOffset]);
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("STAud_Dec initialtion failed\n"));
        }
        else
        {
            error = STAud_DecOpen(Decoder_DeviceName[ModuleOffset],&DecoderHandle);
            if (error != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_Dec Open failed\n"));
            }
            else
            {
                error = AddToTopofDriver(audDriver_p, ObjectID,DecoderHandle);
            }
        }

        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DecInitParams[ModuleOffset]);
        DecInitParams[ModuleOffset] = NULL;

        break;
    }

    case STAUD_OBJECT_ENCODER_COMPRESSED0:
    {
        STAud_EncHandle_t       EncoderHandle;
        ModuleOffset = GetModuleOffset(ObjectID);

        if (EncInitParams[ModuleOffset] == NULL)
        {
            EncInitParams[ModuleOffset] = (STAud_EncInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAud_EncInitParams_t));
            if(!EncInitParams[ModuleOffset])
            {
                return ST_ERROR_NO_MEMORY;
            }
            memset(EncInitParams[ModuleOffset],0,sizeof(STAud_EncInitParams_t));
        }

        EncInitParams[ModuleOffset]->AVMEMPartition     = InitParams_p->AVMEMPartition;
        EncInitParams[ModuleOffset]->DriverPartition    = InitParams_p->CPUPartition_p;
        EncInitParams[ModuleOffset]->OutBufHandle       = mem_handle[0];
        EncInitParams[ModuleOffset]->NumChannels        = InitParams_p->NumChannels;

        error = STAud_EncInit(Encoder_DeviceName[ModuleOffset],EncInitParams[ModuleOffset]);
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("STAud_Enc initialtion failed\n"));
        }
        else
        {
            error = STAud_EncOpen(Encoder_DeviceName[ModuleOffset],&EncoderHandle);
            if (error != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_Enc Open failed\n"));
            }
            else
            {
                error = AddToTopofDriver(audDriver_p, ObjectID,EncoderHandle);
            }
        }

        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, EncInitParams[ModuleOffset]);
        EncInitParams[ModuleOffset] = NULL;

        break;
    }

    case STAUD_OBJECT_MIXER0:
    {
        STAud_MixerHandle_t     MixerHandle;
        ModuleOffset = GetModuleOffset(ObjectID);

        if (MixerInitParams[ModuleOffset] == NULL)
        {
            MixerInitParams[ModuleOffset] = (STAud_MixInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAud_MixInitParams_t));
            if(!MixerInitParams[ModuleOffset])
            {
                return ST_ERROR_NO_MEMORY;
            }
            memset(MixerInitParams[ModuleOffset],0,sizeof(STAud_MixInitParams_t));
        }

        MixerInitParams[ModuleOffset]->AVMEMPartition   = InitParams_p->AVMEMPartition;
        MixerInitParams[ModuleOffset]->DriverPartition  = InitParams_p->CPUPartition_p;
        strncpy(MixerInitParams[ModuleOffset]->EvtHandlerName, InitParams_p->EvtHandlerName,ST_MAX_DEVICE_NAME_TOCOPY);

        #ifdef STAUD_COMMON_EVENTS
            strncpy(MixerInitParams[ModuleOffset]->EvtGeneratorName,InitParams_p->RegistrantDeviceName,ST_MAX_DEVICE_NAME_TOCOPY);
        #else
            strncpy(MixerInitParams[ModuleOffset]->EvtGeneratorName,Mixer_DeviceName[ObjectID],ST_MAX_DEVICE_NAME_TOCOPY);
        #endif

        MixerInitParams[ModuleOffset]->OutBMHandle  = mem_handle[0];
        MixerInitParams[ModuleOffset]->NumChannels  = InitParams_p->NumChannels;
        MixerInitParams[ModuleOffset]->ObjectID     = ObjectID;//STAUD_OBJECT_MIXER0;

        error = STAud_MixerInit(Mixer_DeviceName[ModuleOffset], MixerInitParams[ModuleOffset]);
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("STAud_MixerInit initialtion failed\n"));
        }
        else
        {
            error = STAud_MixerOpen(Mixer_DeviceName[ModuleOffset],&MixerHandle);
            if (error != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_MixerOpens Open failed\n"));
            }
            else
            {
                error = AddToTopofDriver(audDriver_p, ObjectID,MixerHandle);
            }
        }

        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, MixerInitParams[ModuleOffset]);
        MixerInitParams[ModuleOffset] = NULL;

        break;
    }

    case STAUD_OBJECT_POST_PROCESSOR0:
    case STAUD_OBJECT_POST_PROCESSOR1:
    case STAUD_OBJECT_POST_PROCESSOR2:
    {
        STAud_PPHandle_t        PPHandle;
        ModuleOffset = GetModuleOffset(ObjectID);

        if (PPInitParams[ModuleOffset]== NULL)
        {
            PPInitParams[ModuleOffset] = (STAud_PPInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAud_PPInitParams_t));
            if(!PPInitParams[ModuleOffset])
            {
                return ST_ERROR_NO_MEMORY;
            }
            memset(PPInitParams[ModuleOffset],0,sizeof(STAud_PPInitParams_t));
        }

        PPInitParams[ModuleOffset]->AVMEMPartition      = InitParams_p->AVMEMPartition;
        PPInitParams[ModuleOffset]->DriverPartition     = InitParams_p->CPUPartition_p;
        PPInitParams[ModuleOffset]->OutBufHandle        = mem_handle[0];
        PPInitParams[ModuleOffset]->NumChannels         = InitParams_p->NumChannels;
        error = STAud_PPInit(PostProcss_DeviceName[ModuleOffset], PPInitParams[ModuleOffset]);
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("STAud_PPInit initialtion failed\n"));
        }
        else
        {
            error = STAud_PPOpen(PostProcss_DeviceName[ModuleOffset],&PPHandle);
            if (error != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_PPOpen Open failed\n"));
            }
            else
            {
                error = AddToTopofDriver(audDriver_p, ObjectID, PPHandle);
            }
        }

        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PPInitParams[ModuleOffset]);
        PPInitParams[ModuleOffset] = NULL;

        break;
    }

    case STAUD_OBJECT_OUTPUT_PCMP0:
    case STAUD_OBJECT_OUTPUT_PCMP1:
    case STAUD_OBJECT_OUTPUT_PCMP2:
    case STAUD_OBJECT_OUTPUT_PCMP3:
    case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
    {
        STPCMPLAYER_Handle_t    pcmPlayerHandle;
        ModuleOffset = GetModuleOffset(ObjectID);

        if (PCMPInitParams[ModuleOffset] == NULL)
        {
            PCMPInitParams[ModuleOffset] = (PCMPlayerInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(PCMPlayerInitParams_t));
            if(!PCMPInitParams[ModuleOffset])
            {
                return ST_ERROR_NO_MEMORY;
            }
            memset(PCMPInitParams[ModuleOffset],0,sizeof(PCMPlayerInitParams_t));
        }

        PCMPInitParams[ModuleOffset]->pcmPlayerIdentifier           = GetModuleIdentifier(ObjectID);//PCM_PLAYER_0;
        PCMPInitParams[ModuleOffset]->pcmPlayerNumNode              = 6;
        PCMPInitParams[ModuleOffset]->CPUPartition                  = InitParams_p->CPUPartition_p;
        PCMPInitParams[ModuleOffset]->AVMEMPartition                = InitParams_p->AVMEMPartition;

        PCMPInitParams[ModuleOffset]->DummyBufferPartition          = (InitParams_p->AllocateFromEMBX)?0:InitParams_p->BufferPartition;

        strncpy(PCMPInitParams[ModuleOffset]->EvtHandlerName, InitParams_p->EvtHandlerName,ST_MAX_DEVICE_NAME_TOCOPY);

        #ifdef STAUD_COMMON_EVENTS
            strncpy(PCMPInitParams[ModuleOffset]->EvtGeneratorName,InitParams_p->RegistrantDeviceName,ST_MAX_DEVICE_NAME_TOCOPY);
        #else
        strncpy(PCMPInitParams[ModuleOffset]->EvtGeneratorName, PCMPlayer_DeviceName[ModuleOffset],ST_MAX_DEVICE_NAME_TOCOPY); //PCM player 0 will recive his own events
        #endif

        PCMPInitParams[ModuleOffset]->skipEventID                   = (U32)STAUD_AVSYNC_SKIP_EVT;
        PCMPInitParams[ModuleOffset]->pauseEventID                  = (U32)STAUD_AVSYNC_PAUSE_EVT;

        PCMPInitParams[ModuleOffset]->pcmPlayerDataConfig           = InitParams_p->PCMOutParams;
        PCMPInitParams[ModuleOffset]->NumChannels                   = InitParams_p->NumChannels;
        PCMPInitParams[ModuleOffset]->EnableMSPPSupport            = InitParams_p->EnableMSPPSupport;

        #ifndef STAUD_REMOVE_CLKRV_SUPPORT
            PCMPInitParams[ModuleOffset]->CLKRV_Handle = audDriver_p->CLKRV_Handle;
            /* CLKRV_Handle should be different from clock and synchro */
            PCMPInitParams[ModuleOffset]->CLKRV_HandleForSynchro = audDriver_p->CLKRV_HandleForSynchro;
        #endif

        PCMPInitParams[ModuleOffset]->ObjectID = ObjectID;//STAUD_OBJECT_OUTPUT_PCMP0;
        error = STAud_PCMPlayerInit(PCMPlayer_DeviceName[ModuleOffset], &pcmPlayerHandle, PCMPInitParams[ModuleOffset]);

        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("PCM player 0 initialized failed\n"));
        }
        else
        {
            error = AddToTopofDriver(audDriver_p, ObjectID, pcmPlayerHandle);
        }

        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PCMPInitParams[ModuleOffset]);
        PCMPInitParams[ModuleOffset] = NULL;

        break;
    }

    case STAUD_OBJECT_OUTPUT_SPDIF0:
    case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
    {
        STSPDIFPLAYER_Handle_t  spdifPlayerHandle;
        ModuleOffset = GetModuleOffset(ObjectID);

        if (SPDIFPInitParams[ModuleOffset]== NULL)
        {
            SPDIFPInitParams[ModuleOffset] = (SPDIFPlayerInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(SPDIFPlayerInitParams_t));
            if(!SPDIFPInitParams[ModuleOffset])
            {
                return ST_ERROR_NO_MEMORY;
            }
            memset(SPDIFPInitParams[ModuleOffset],0,sizeof(SPDIFPlayerInitParams_t));
        }

        SPDIFPInitParams[ModuleOffset]->spdifPlayerIdentifier       = GetModuleIdentifier(ObjectID);//SPDIF_PLAYER_0;
        SPDIFPInitParams[ModuleOffset]->spdifPlayerNumNode          = 6;
        SPDIFPInitParams[ModuleOffset]->CPUPartition                = InitParams_p->CPUPartition_p;
        SPDIFPInitParams[ModuleOffset]->AVMEMPartition              = InitParams_p->AVMEMPartition;
        SPDIFPInitParams[ModuleOffset]->DummyBufferPartition        = (InitParams_p->AllocateFromEMBX)?0:InitParams_p->BufferPartition;
        SPDIFPInitParams[ModuleOffset]->SPDIFMode                   = InitParams_p->SPDIFMode;
        SPDIFPInitParams[ModuleOffset]->SPDIFPlayerOutParams        = InitParams_p->SPDIFOutParams;
        strncpy(SPDIFPInitParams[ModuleOffset]->EvtHandlerName, InitParams_p->EvtHandlerName,ST_MAX_DEVICE_NAME_TOCOPY);

        #ifdef STAUD_COMMON_EVENTS
            strncpy(SPDIFPInitParams[ModuleOffset]->EvtGeneratorName,InitParams_p->RegistrantDeviceName,ST_MAX_DEVICE_NAME_TOCOPY);
        #else
            strncpy(SPDIFPInitParams[ModuleOffset]->EvtGeneratorName, SPDIFPlayer_DeviceName[ModuleOffset],ST_MAX_DEVICE_NAME_TOCOPY); //PCM player 0 will recive his own events
        #endif

        SPDIFPInitParams[ModuleOffset]->skipEventID                 = (U32)STAUD_AVSYNC_SKIP_EVT;
        SPDIFPInitParams[ModuleOffset]->pauseEventID                = (U32)STAUD_AVSYNC_PAUSE_EVT;
        SPDIFPInitParams[ModuleOffset]->NumChannels                 = InitParams_p->NumChannels;
        SPDIFPInitParams[ModuleOffset]->EnableMSPPSupport           = InitParams_p->EnableMSPPSupport;

        #ifndef STAUD_REMOVE_CLKRV_SUPPORT
            SPDIFPInitParams[ModuleOffset]->CLKRV_Handle = audDriver_p->CLKRV_Handle;
            /* CLKRV_Handle should be different from clock and synchro */
            SPDIFPInitParams[ModuleOffset]->CLKRV_HandleForSynchro = audDriver_p->CLKRV_HandleForSynchro;
        #endif

        SPDIFPInitParams[ModuleOffset]->ObjectID = ObjectID;//STAUD_OBJECT_OUTPUT_SPDIF0;

        error = STAud_SPDIFPlayerInit(SPDIFPlayer_DeviceName[ModuleOffset], &spdifPlayerHandle, SPDIFPInitParams[ModuleOffset]);

        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("SPDIF player 0 initialtion failed\n"));
        }
        else
        {
            error = AddToTopofDriver(audDriver_p, ObjectID, spdifPlayerHandle);
        }

        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, SPDIFPInitParams[ModuleOffset]);
        SPDIFPInitParams[ModuleOffset] = NULL;

        break;
    }

    case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
    case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
    case STAUD_OBJECT_FRAME_PROCESSOR:
    case STAUD_OBJECT_FRAME_PROCESSOR1:
    {
        STDataProcesser_Handle_t dataProcesserHandleByteSwapper;
        ModuleOffset = GetModuleOffset(ObjectID);

        if (DataPInitParams[ModuleOffset] == NULL)
        {
            DataPInitParams[ModuleOffset] = (DataProcesserInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(DataProcesserInitParams_t));
            if(!DataPInitParams[ModuleOffset])
            {
                return ST_ERROR_NO_MEMORY;
            }
            memset(DataPInitParams[ModuleOffset],0,sizeof(DataProcesserInitParams_t));
        }

        DataPInitParams[ModuleOffset]->CPUPartition                     = InitParams_p->CPUPartition_p;
        DataPInitParams[ModuleOffset]->AVMEMPartition                   = InitParams_p->AVMEMPartition;
        DataPInitParams[ModuleOffset]->dataProcesserIdentifier          = GetModuleIdentifier(ObjectID);//BYTE_SWAPPER;
        DataPInitParams[ModuleOffset]->outputMemoryBlockManagerHandle   = mem_handle[0];
        DataPInitParams[ModuleOffset]->EnableMSPPSupport                = InitParams_p->EnableMSPPSupport;
        error = STAud_DataProcesserInit(&dataProcesserHandleByteSwapper, DataPInitParams[ModuleOffset]);
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("data processer  initialtion failed\n"));
        }
        else
        {
            error = AddToTopofDriver(audDriver_p, ObjectID,dataProcesserHandleByteSwapper);
        }

        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DataPInitParams[ModuleOffset]);
        DataPInitParams[ModuleOffset] = NULL;
    }
    break;

    default:
        break;
    }

    return error;
}

#if defined (ST_OPTIMIZE_BUFFER)
    static ST_ErrorCode_t   GetNumOfBlocks(ObjectSet    CurObjectSet, U32 *NumBlock)
    {
        ST_ErrorCode_t  Error   = ST_NO_ERROR;
        U32 currentObjectCount  = 0;
        STAUD_Object_t  CurObject;

        CurObject               = CurObjectSet[currentObjectCount];
        *NumBlock               = 2; /*Default value */
        while (CurObject != OBJECTSET_DONE)
        {
            switch (CurObject)
            {
                case STAUD_OBJECT_INPUT_CD0:
                case STAUD_OBJECT_INPUT_CD1:
                case STAUD_OBJECT_INPUT_CD2:
                *NumBlock   = NUM_NODES_PARSER;
                    break;

                case STAUD_OBJECT_MIXER0:
                case STAUD_OBJECT_OUTPUT_PCMP0:
                case STAUD_OBJECT_OUTPUT_PCMP1:
                case STAUD_OBJECT_OUTPUT_PCMP2:
                case STAUD_OBJECT_OUTPUT_PCMP3:
                case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                case STAUD_OBJECT_OUTPUT_SPDIF0:
                case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:
                    *NumBlock   = NUM_NODES_BEFORE_PLAYER;
                break;

                default:
                    break;
            }
            currentObjectCount++;
            CurObject   = CurObjectSet[currentObjectCount];

        }

        return Error;
    }
#endif

static ST_ErrorCode_t GetOutputPageSize(ObjectSet   CurObjectSet, U32 *OutputPageSize)
{
    ST_ErrorCode_t  Error               = ST_NO_ERROR;
    U32             currentObjectCount  = 0;
    STAUD_Object_t  CurObject;

    CurObject                           = CurObjectSet[currentObjectCount];
    *OutputPageSize                     = MAX_SAMPLES_PER_FRAME; /*Default value */
    while (CurObject != OBJECTSET_DONE)
    {
        switch (CurObject)
        {
            case STAUD_OBJECT_OUTPUT_PCMP3:
                *OutputPageSize = 2*MAX_SAMPLES_PER_FRAME;
            break;

            default:
                break;
        }
        currentObjectCount++;
        CurObject           = CurObjectSet[currentObjectCount];

    }

    return Error;
}
static ST_ErrorCode_t   InitDependentModuleParams(ObjectSet CurObjectSet, STMEMBLK_Handle_t *mem_handle, STAUD_InitParams_t *InitParams_p)
{
    ST_ErrorCode_t  Error   = ST_NO_ERROR;
    U32 currentObjectCount;
    STAUD_Object_t  CurObject;
    U32 ModuleOffset        = 0;
    currentObjectCount      = 1;
    CurObject               = CurObjectSet[currentObjectCount];

    while (CurObject != OBJECTSET_DONE)
    {
        switch (CurObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:
                ModuleOffset = GetModuleOffset(CurObject);
                if (PESESInitParams[ModuleOffset] == NULL)
                {
                    PESESInitParams[ModuleOffset] = (PESESInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(PESESInitParams_t));
                    if(!PESESInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(PESESInitParams[ModuleOffset],0,sizeof(PESESInitParams_t));
                }
                PESESInitParams[ModuleOffset]->IpBufHandle = mem_handle[0];

                break;

            case STAUD_OBJECT_INPUT_PCM0:
                ModuleOffset = GetModuleOffset(CurObject);
                if(PCMIInitParams[ModuleOffset] == NULL)
                {
                    PCMIInitParams[ModuleOffset] = (STAudPCMInputInit_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAudPCMInputInit_t));
                    if(!PCMIInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(PCMIInitParams[ModuleOffset],0,sizeof(STAudPCMInputInit_t));
                }
                break;

            case STAUD_OBJECT_INPUT_PCMREADER0:
                ModuleOffset = GetModuleOffset(CurObject);
                if(PCMRInitParams[ModuleOffset] == NULL)
                {
                    PCMRInitParams[ModuleOffset] = (PCMReaderInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(PCMReaderInitParams_t));

                    if(!PCMRInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(PCMRInitParams[ModuleOffset],0,sizeof(PCMReaderInitParams_t));
                }
                break;

            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_DECODER_COMPRESSED2:
                ModuleOffset = GetModuleOffset(CurObject);
                if (DecInitParams[ModuleOffset] == NULL)
                {
                    DecInitParams[ModuleOffset] = (STAud_DecInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAud_DecInitParams_t));
                    if(!DecInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(DecInitParams[ModuleOffset],0,sizeof(STAud_DecInitParams_t));
                }
                DecInitParams[ModuleOffset]->InBufHandle = mem_handle[0];
                break;

            case STAUD_OBJECT_ENCODER_COMPRESSED0:
                ModuleOffset = GetModuleOffset(CurObject);
                if (EncInitParams[ModuleOffset] == NULL)
                {
                    EncInitParams[ModuleOffset] = (STAud_EncInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAud_EncInitParams_t));
                    if(!EncInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(EncInitParams[ModuleOffset],0,sizeof(STAud_EncInitParams_t));
                }
                EncInitParams[ModuleOffset]->InBufHandle = mem_handle[0];
                break;

            case STAUD_OBJECT_MIXER0:
                ModuleOffset = GetModuleOffset(CurObject);
                if (MixerInitParams[ModuleOffset] == NULL)
                {
                    MixerInitParams[ModuleOffset] = (STAud_MixInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAud_MixInitParams_t));
                    if(!MixerInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(MixerInitParams[ModuleOffset],0,sizeof(STAud_MixInitParams_t));
                }
                //check if we have space for more inputs.
                if(MixerInitParams[ModuleOffset]->NoOfRegisteredInput == ACC_MIXER_MAX_NB_INPUT)
                {
                    STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, MixerInitParams[ModuleOffset]);
                    MixerInitParams[ModuleOffset] = NULL;
                    return ST_ERROR_NO_MEMORY;
                }
                // Add cases for mixer inputs
                MixerInitParams[ModuleOffset]->InBMHandle[MixerInitParams[ModuleOffset]->NoOfRegisteredInput] = mem_handle[0];
                MixerInitParams[ModuleOffset]->NoOfRegisteredInput ++;
                break;

            case STAUD_OBJECT_POST_PROCESSOR0:
            case STAUD_OBJECT_POST_PROCESSOR1:
            case STAUD_OBJECT_POST_PROCESSOR2:
                ModuleOffset = GetModuleOffset(CurObject);
                if (PPInitParams[ModuleOffset] == NULL)
                {
                    PPInitParams[ModuleOffset] = (STAud_PPInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAud_PPInitParams_t));
                    if(!PPInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(PPInitParams[ModuleOffset],0,sizeof(STAud_PPInitParams_t));
                }
                PPInitParams[ModuleOffset]->InBufHandle = mem_handle[0];
                break;

            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_PCMP1:
            case STAUD_OBJECT_OUTPUT_PCMP2:
            case STAUD_OBJECT_OUTPUT_PCMP3:
            case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
                ModuleOffset = GetModuleOffset(CurObject);
                if (PCMPInitParams[ModuleOffset] == NULL)
                {
                    PCMPInitParams[ModuleOffset] = (PCMPlayerInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(PCMPlayerInitParams_t));
                    if(!PCMPInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(PCMPInitParams[ModuleOffset],0,sizeof(PCMPlayerInitParams_t));
                }
                PCMPInitParams[ModuleOffset]->memoryBlockManagerHandle = mem_handle[0];
            break;

            case STAUD_OBJECT_OUTPUT_SPDIF0:
            case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:

                ModuleOffset = GetModuleOffset(CurObject);
                if (SPDIFPInitParams[ModuleOffset] == NULL)
                {
                    SPDIFPInitParams[ModuleOffset] = (SPDIFPlayerInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(SPDIFPlayerInitParams_t));
                    if(!SPDIFPInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(SPDIFPInitParams[ModuleOffset],0,sizeof(SPDIFPlayerInitParams_t));
                }

                if ((CurObjectSet[0] == STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER) || ((CurObjectSet[0] & STAUD_CLASS_INPUT) == STAUD_CLASS_INPUT))
                {
                    if (CurObjectSet[0] == STAUD_OBJECT_INPUT_PCM0)
                    {
                        SPDIFPInitParams[ModuleOffset]->pcmMemoryBlockManagerHandle             = mem_handle[0];
                    }
                    else
                    {
                        SPDIFPInitParams[ModuleOffset]->compressedMemoryBlockManagerHandle0     = mem_handle[0];
                    }
                    // Temp change for DDP transcoding
                    //SPDIFPInitParams[ModuleOffset]->compressedMemoryBlockManagerHandle1           = mem_handle[0];
                }
                else if ((CurObjectSet[0] & STAUD_CLASS_DECODER) == STAUD_CLASS_DECODER)
                {
                    // Modification done so that SPDIF player can get data from decoder
                    //if (CurObjectSet[0] == STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER)
                    // We consider this to be an decoder object
                    SPDIFPInitParams[ModuleOffset]->pcmMemoryBlockManagerHandle                 = mem_handle[0];
                    SPDIFPInitParams[ModuleOffset]->compressedMemoryBlockManagerHandle1         = mem_handle[1];
                }
                else if (((CurObjectSet[0] & STAUD_CLASS_POSTPROC) == STAUD_CLASS_POSTPROC) || ((CurObjectSet[0] & STAUD_CLASS_MIXER) == STAUD_CLASS_MIXER))
                {
                    SPDIFPInitParams[ModuleOffset]->pcmMemoryBlockManagerHandle                 = mem_handle[0];
                }
                else if ((CurObjectSet[0] & STAUD_CLASS_ENCODER) == STAUD_CLASS_ENCODER)
                {
                    SPDIFPInitParams[ModuleOffset]->compressedMemoryBlockManagerHandle2        = mem_handle[0];
                }

                break;

            case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
            case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
            case STAUD_OBJECT_FRAME_PROCESSOR:
            case STAUD_OBJECT_FRAME_PROCESSOR1:
                ModuleOffset = GetModuleOffset(CurObject);
                if (DataPInitParams[ModuleOffset] == NULL)
                {
                    DataPInitParams[ModuleOffset] = (DataProcesserInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(DataProcesserInitParams_t));
                    if(!DataPInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(DataPInitParams[ModuleOffset],0,sizeof(DataProcesserInitParams_t));
                }

                DataPInitParams[ModuleOffset]->inputMemoryBlockManagerHandle0 = mem_handle[0];

                break;

            default:
                break;
            }
            currentObjectCount++;
            CurObject       = CurObjectSet[currentObjectCount];

        }

    return Error;
}

static ST_ErrorCode_t GetFirstFreeMemBlock(U8 *PoolAllocationCount_p)
{
    U8 PoolCount = 0;
    while(PoolCount < MEMBLK_MAX_NUMBER)
    {
        if(MemPool_IsInit(MemPool_DeviceName[PoolCount]))
        {
            PoolCount += 1;
        }
        else
        {
            *PoolAllocationCount_p = PoolCount;
            return ST_NO_ERROR;
        }

    }
    return ST_ERROR_NO_FREE_HANDLES;
}

ST_ErrorCode_t  STAud_CreateDriver(STAUD_InitParams_t *InitParams_p, AudDriver_t *audDriver_p)
{
    ST_ErrorCode_t  Error                           = ST_NO_ERROR;
    U8              DriverIndex                     = 0;    /* Index into driver array, for selecting particular driver configuration */
    U8              CurObjectSetCount;
    STAUD_Object_t  *CurObject;
    ObjectSet       *CurObjectSet;  /* Current object set */
    DriverCofig     *CurDriverConfig;   /* Current Driver config */
    U8              PoolAllocationCount             = 0;
    U8              CountObjectSet                  = 0;
    U32             NumBlock;
    STMEMBLK_Handle_t       mem_handle[MAX_OUTPUT_BLOCKS_PER_MODULE];
    DecoderControlBlock_t   *TempBlock              = NULL;
    BOOL            DDPlusCodec                     = FALSE;
    STAUD_ENCCapability_t   EncCapability;
    U32             maxEncodedCompressedFrameSize   = MAX_ENCODED_FRAMESIZE;

    #if MIXER_INPUT_FROM_FILE
        STMEMBLK_Handle_t   mem_handle1;
    #endif

    DriverIndex                         = InitParams_p->DriverIndex;
    CurObjectSetCount                   = 0;

    /* Get hold of current driver */
    CurDriverConfig = &(AudioDriver[DriverIndex]);

    /* Get hold of current object Set */
    CurObjectSet                        = (ObjectSet*)((ObjectSet*)CurDriverConfig + CurObjectSetCount);
    CurObject                           = (STAUD_Object_t*)((STAUD_Object_t*)CurObjectSet+0);
    memset((U8*)PESESInitParams,  0, sizeof(PESESInitParams[0])*NUM_PARSER);
    memset((U8*)DecInitParams,    0, sizeof(DecInitParams[0])*NUM_DECODER);
    memset((U8*)MixerInitParams,  0, sizeof(MixerInitParams[0])*NUM_MIXER);
    memset((U8*)PCMPInitParams,   0, sizeof(PCMPInitParams[0])*NUM_PCMPLAYER);
    memset((U8*)SPDIFPInitParams, 0, sizeof(SPDIFPInitParams[0])*NUM_SPDIFPLAYER);
    memset((U8*)DataPInitParams,  0, sizeof(DataPInitParams[0])*NUM_DATAPROCESSOR);
    memset((U8*)PPInitParams,     0, sizeof(PPInitParams[0])*NUM_POSTPROCESSOR);
    memset((U8*)EncInitParams,    0, sizeof(EncInitParams[0])*NUM_ENCODER);
    memset((U8*)PCMIInitParams,   0, sizeof(PCMIInitParams[0])*NUM_PCMINPUT);
    memset((U8*)PCMRInitParams,   0, sizeof(PCMRInitParams[0])*NUM_PCMREADER);



    Error = GetFirstFreeMemBlock(&PoolAllocationCount);
    if(Error != ST_NO_ERROR)
    {
        return ST_ERROR_NO_FREE_HANDLES;
    }
    /* Get the Size for the Buffer Run- Time Allocation*/


    TempBlock   = (DecoderControlBlock_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(DecoderControlBlock_t));
    if(TempBlock)
    {
        Error   = STAud_InitDecCapability(TempBlock);
        if(Error)
        {
            /*Use the default sizes */
            MAX_SAMPLES_PER_FRAME           = MAX_SAMPLES_PER_FRAME1;
            MAX_INPUT_COMPRESSED_FRAME_SIZE = MAX_INPUT_COMPRESSED_FRAME_SIZE1;
        }
        else
        {

            /*Set the Decoded Buffers */
            if(TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_DDPLUS )
            {
                DDPlusCodec = TRUE;
            }

            if (TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_WMA ||
            TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_WMAPROLSL ||
            TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_DTS)
            {
                MAX_SAMPLES_PER_FRAME = 4*1024;
            }

            else if (   TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_PCM ||
                TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_LPCM  ||
                TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_CDDA  )
            {
                MAX_SAMPLES_PER_FRAME = 2560;
            }
            else
            {
                MAX_SAMPLES_PER_FRAME = 2*1024;
            }
            #ifdef MSPP_PARSER
                if(TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_WMAPROLSL ||
                TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_WMA)
                {
                    MAX_INPUT_COMPRESSED_FRAME_SIZE = 12*1024;
                }
                else
                {
                    MAX_INPUT_COMPRESSED_FRAME_SIZE = 4*1024;
                }
            #else
                if( TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_WMAPROLSL ||
                TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_WMA ||
                TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_PCM ||
                TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_LPCM  ||
                TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_CDDA  )
                {
                    MAX_INPUT_COMPRESSED_FRAME_SIZE = 36*1024;
                }

                else if (   TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_AC3 ||
                    TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_DTS ||
                    TempBlock->DecCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_MLP)
                {
                    MAX_INPUT_COMPRESSED_FRAME_SIZE = 16*1024;
                }
                else
                {
                    MAX_INPUT_COMPRESSED_FRAME_SIZE = 4*1024;
                }

            #endif
        }
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p,TempBlock);
    }
    else
    {
        return ST_ERROR_NO_MEMORY;
    }

    memset((U8 *)&EncCapability, 0, sizeof(STAUD_ENCCapability_t));
    STAUD_GetEncCapability(&EncCapability);

    if (EncCapability.SupportedStreamContents == 0)
    {
        maxEncodedCompressedFrameSize = 0;
    }
    else
    {
        if ((EncCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_AC3) ||
            (EncCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_MPEG2))
        {
            maxEncodedCompressedFrameSize = 1536;
        }

        if (EncCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_DTS)
        {
            maxEncodedCompressedFrameSize = 2048;
        }

        if (EncCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_MPEG_AAC)
        {
            maxEncodedCompressedFrameSize = 4096; // Not given by LX team need to be confirmed
        }

        if (EncCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_MP3)
        {
            maxEncodedCompressedFrameSize = 7500;
        }

        if (EncCapability.SupportedStreamContents & STAUD_STREAM_CONTENT_WMA)
        {
            maxEncodedCompressedFrameSize = 18432;
        }
    }

    /* Parse and build the full driver structure */
    while((*CurObject  != DRIVER_DONE)&& (CountObjectSet <= MAX_OBJECTSET_PER_DRIVER))
    {
        U32 PPOutputScatterPageSize=0;
        memset(mem_handle, 0, sizeof(STMEMBLK_Handle_t)*MAX_OUTPUT_BLOCKS_PER_MODULE);
        /* Initialize modules/objects */
        switch(*CurObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
            case STAUD_OBJECT_INPUT_CD2:

            #if USER_LATENCY_ENABLE
                NumBlock = NUM_NODES_PARSER_LATENCY;
            #else
                #if defined (ST_OPTIMIZE_BUFFER)
                    Error |= GetNumOfBlocks(*CurObjectSet,&NumBlock);
                #else
                    NumBlock = NUM_NODES_PARSER;
                #endif
            #endif
            /* Initialize the output buffer pool for this module*/
            Error |= InitMemoryPool(PoolAllocationCount, NumBlock, MAX_INPUT_COMPRESSED_FRAME_SIZE, InitParams_p->AllocateFromEMBX,
            InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);

            /* Initialize current Module */
            Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

            /* Increment pool count for next unit's use */
            PoolAllocationCount++;
            CountObjectSet ++;
            break;

        case STAUD_OBJECT_INPUT_PCM0:
            #if defined (ST_OPTIMIZE_BUFFER)
                Error |= GetNumOfBlocks(*CurObjectSet,&NumBlock);
            #else
                NumBlock = NUM_NODES_PCMINPUT;
            #endif
            /* Initialize the output buffer pool for this module*/

            Error |= InitMemoryPool(PoolAllocationCount, NumBlock, (U32)(InitParams_p->NumChannels * 4 * MAX_SAMPLES_PER_FRAME), InitParams_p->AllocateFromEMBX,
            InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);

            /* Initialize current Module */
            Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

            /* Increment pool count for next unit's use */
            PoolAllocationCount++;
            CountObjectSet ++;
            break;

        case STAUD_OBJECT_INPUT_PCMREADER0:

            NumBlock = NUM_NODES_PCMREADER;
            /* Initialize the output buffer pool for this module*/

            if((InitParams_p->NumChPCMReader != 2) && (InitParams_p->NumChPCMReader !=10))
            {
                InitParams_p->NumChPCMReader = InitParams_p->NumChannels;
            }

            Error |= InitMemoryPool(PoolAllocationCount, NumBlock, (U32)(InitParams_p->NumChPCMReader * 4 * NUM_SAMPLES_PER_PCM_NODE), InitParams_p->AllocateFromEMBX,
            InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);

            /* Initialize current Module */
            Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

            /* Increment pool count for next unit's use */
            PoolAllocationCount++;
            CountObjectSet ++;

            break;

        case STAUD_OBJECT_DECODER_COMPRESSED0:
        case STAUD_OBJECT_DECODER_COMPRESSED1:
        case STAUD_OBJECT_DECODER_COMPRESSED2:
            /* Initialize the output buffer pool for this module*/
            #if defined (ST_OPTIMIZE_BUFFER)
                Error |= GetNumOfBlocks(*CurObjectSet,&NumBlock);
            #else
                NumBlock = NUM_NODES_DECODER;
            #endif
            Error |= InitMemoryPool(PoolAllocationCount, NumBlock, (U32)(InitParams_p->NumChannels * 4 * MAX_SAMPLES_PER_FRAME), InitParams_p->AllocateFromEMBX,
            InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);

            PoolAllocationCount++;

            if(DDPlusCodec)
            {
                Error |= InitMemoryPool(PoolAllocationCount, NumBlock, MAX_INPUT_COMPRESSED_FRAME_SIZE, InitParams_p->AllocateFromEMBX,
                InitParams_p->BufferPartition, InitParams_p->CPUPartition_p,  &(mem_handle[1]), audDriver_p);
            }

            /* Initialize current Module */
            Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

            /* Increment pool count for next unit's use */
            PoolAllocationCount++;
            CountObjectSet ++;

        break;

        case STAUD_OBJECT_ENCODER_COMPRESSED0:

            NumBlock = NUM_NODES_ENCODER + 16;

            Error |= InitMemoryPool(PoolAllocationCount, NumBlock, maxEncodedCompressedFrameSize, InitParams_p->AllocateFromEMBX,
                                    InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);

            /* Initialize current Module */
            Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

            /* Increment pool count for next unit's use */
            PoolAllocationCount++;
            CountObjectSet ++;
            break;

        case STAUD_OBJECT_MIXER0:
            #if defined(ST_OPTIMIZE_BUFFER)
                Error |= GetNumOfBlocks(*CurObjectSet,&NumBlock);
            #else
                NumBlock = NUM_NODES_MIXER;
            #endif

            /* Initialize the output buffer pool for this module*/
            Error |= InitMemoryPool(PoolAllocationCount, NumBlock, (U32)(InitParams_p->NumChannels * 4 * MIXER_OUT_PCM_SAMPLES), InitParams_p->AllocateFromEMBX,
            InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);


            /* This code is specially added to test mixer with a dummy input. Should be removed from here*/
            #if MIXER_INPUT_FROM_FILE
                if (MixerInitParams[ModuleOffset] == NULL)
                {
                    MixerInitParams[ModuleOffset] = (STAud_MixInitParams_t *)STOS_MemoryAllocate(InitParams_p->CPUPartition_p,sizeof(STAud_MixInitParams_t));
                    if(!MixerInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(MixerInitParams[ModuleOffset],0,sizeof(STAud_MixInitParams_t));
                }
                PoolAllocationCount++;
                Error |= InitMemoryPool(PoolAllocationCount, NUM_NODES_MIXER, (U32)(InitParams_p->NumChannels * 4 * MAX_SAMPLES_PER_FRAME), InitParams_p->AllocateFromEMBX,
                InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[1]), audDriver_p);

                /* Till This point. When removing this code remember to remove "- 1" from next few statements*/
                MixerInitParams[ModuleOffset]->InMBHandle[1] = mem_handle[1];
            #endif

            /* Initialize current Module */
            Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

            /* Increment pool count for next unit's use */
            PoolAllocationCount++;
            CountObjectSet ++;
            break;

        case STAUD_OBJECT_POST_PROCESSOR0:
        case STAUD_OBJECT_POST_PROCESSOR1:
        case STAUD_OBJECT_POST_PROCESSOR2:

            #if defined(ST_OPTIMIZE_BUFFER)
                /* Initialize the output buffer pool for this module*/
                Error |= GetNumOfBlocks(*CurObjectSet,&NumBlock);
            #else
                NumBlock = NUM_NODES_PP;
            #endif

            Error = GetOutputPageSize(*CurObjectSet,&PPOutputScatterPageSize);

            Error |= InitMemoryPool(PoolAllocationCount, NumBlock, (U32)((InitParams_p->NumChannels) * 4 * PPOutputScatterPageSize), InitParams_p->AllocateFromEMBX,
            InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);

            /* Initialize current Module */
            Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

            /* Increment pool count for next unit's use */
            PoolAllocationCount++;
            CountObjectSet ++;

            break;

        case STAUD_OBJECT_OUTPUT_PCMP0:
        case STAUD_OBJECT_OUTPUT_PCMP1:
        case STAUD_OBJECT_OUTPUT_PCMP2:
        case STAUD_OBJECT_OUTPUT_PCMP3:
        case STAUD_OBJECT_OUTPUT_HDMI_PCMP0:
            /* Initialize current Module */

            Error |= InitCurrentModule(*CurObject, InitParams_p, 0, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, 0, InitParams_p);
            CountObjectSet ++;

            break;

        case STAUD_OBJECT_OUTPUT_SPDIF0:
        case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0:

            /* Initialize current Module */

            Error |= InitCurrentModule(*CurObject, InitParams_p, 0, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, 0, InitParams_p);
            CountObjectSet ++;
            break;

        case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
        case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
            /* Initialize the output buffer pool for this module*/
            #if defined(ST_OPTIMIZE_BUFFER)
                Error |= GetNumOfBlocks(*CurObjectSet,&NumBlock);
            #else
                NumBlock = NUM_NODES_DATAPROS;
            #endif

            Error |= InitMemoryPool(PoolAllocationCount, NumBlock, 2 * 4 * MAX_SAMPLES_PER_FRAME, InitParams_p->AllocateFromEMBX,
            InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);

        case STAUD_OBJECT_FRAME_PROCESSOR:
        case STAUD_OBJECT_FRAME_PROCESSOR1:
            /* Initialize current Module */
            Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

            /* Initialize dependent module parameters*/
            Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

            /* Increment pool count for next unit's use */
            PoolAllocationCount++;
            CountObjectSet ++;
            break;

        case OBJECTSET_DONE:
            //add bms to driver queue
            break;

        default:
            break;

        }

    CurObjectSetCount++;
    CurObjectSet    = (ObjectSet*)((ObjectSet*)CurDriverConfig + CurObjectSetCount);
    CurObject       = (STAUD_Object_t*)((STAUD_Object_t*)CurObjectSet+0);

    }

    return Error;
}


/* Register events to be sent */
ST_ErrorCode_t STAud_RegisterEvents(AudDriver_t *audDriver_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_OpenParams_t EVT_OpenParams;

    /* Open the ST Event */
    Error       |= STEVT_Open(audDriver_p->EvtHandlerName,&EVT_OpenParams,&audDriver_p->EvtHandle);

    /* Register to the events */
    if(Error == ST_NO_ERROR)
    {
        /*Stopped Event*/
        Error   |= STEVT_RegisterDeviceEvent(audDriver_p->EvtHandle,audDriver_p->Name,STAUD_STOPPED_EVT,&audDriver_p->EventIDStopped);
        /*Resume event*/
        Error   |= STEVT_RegisterDeviceEvent(audDriver_p->EvtHandle,audDriver_p->Name,STAUD_RESUME_EVT,&audDriver_p->EventIDResumed);
    }

    return Error;
}

/* Unsubscribe the parser */
ST_ErrorCode_t STAud_UnSubscribeEvents(AudDriver_t *audDriver_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    if(audDriver_p->EvtHandle)
    {
        Error |= STEVT_Close(audDriver_p->EvtHandle);
        if(Error == ST_NO_ERROR)
        {
            audDriver_p->EvtHandle = 0;
        }
    }
    return Error;
}


