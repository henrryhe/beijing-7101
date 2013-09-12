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
#define ST_OPTIMIZE_BUFFER 1
#include "stos.h"
#include "staud.h"
#include "aud_amphiondecoder.h"
#include "aud_pcmplayer.h"
#include "aud_pes_es_parser.h"
#include "aud_spdifplayer.h"
#include "aud_utils.h"
#include "staud_driverconfig.h"
#include "aud_evt.h"
#ifdef INPUT_PCM_SUPPORT
#include "pcminput.h"
#endif
#ifdef MIXER_SUPPORT
#include "aud_mmeaudiomixer.h"
#endif
#include "staudlx_prints.h"
#define NUM_BYTES_PER_CHANNEL 2


#define STAUD_MAX_OPEN	      1


extern ST_DeviceName_t MemPool_DeviceName[];
extern ST_DeviceName_t PES_ES_DeviceName[];
extern ST_DeviceName_t Decoder_DeviceName[];
extern ST_DeviceName_t PCMPlayer_DeviceName[];
extern ST_DeviceName_t SPDIFPlayer_DeviceName[];
extern ST_DeviceName_t Mixer_DeviceName[];

#define	AVSYNC_ADDED	1
#define REV_STRING_LEN 	(256)

#define MIXER_INPUT_FROM_FILE 	0

#define NUM_PARSER			2
#define NUM_DECODER 			2
#define NUM_MIXER				1
#define NUM_PCMPLAYER		1 	   // pcm player
#define NUM_SPDIFPLAYER		1   //spdif
#define NUM_DATAPROCESSOR 	4 	// 1byte swapper+ 1 byte converter + 2 frame processor
PESESInitParams_t				*PESESInitParams[NUM_PARSER];
STAud_DecInitParams_t		*DecInitParams[NUM_DECODER];
#ifdef MIXER_SUPPORT
STAud_MixInitParams_t		*MixerInitParams[NUM_MIXER];
#endif
PCMPlayerInitParams_t		*PCMPInitParams[NUM_PCMPLAYER];
SPDIFPlayerInitParams_t	   *SPDIFPInitParams[NUM_SPDIFPLAYER];
#ifdef FRAME_PROCESSOR_SUPPORT
DataProcesserInitParams_t	*DataPInitParams[NUM_DATAPROCESSOR];
#endif
#ifdef INPUT_PCM_SUPPORT
STAudPCMInputInit_t			*PCMIInitParams[NUM_PCMINPUT];
#endif

/* Queue of initialized Decoders */
static AudDriver_t *AudDriverHead_p = NULL;

/*----------------------------------------------------------------------------
Audio Chain Creations functions
-----------------------------------------------------------------------------*/

static ST_ErrorCode_t	InitDependentModuleParams(ObjectSet	CurObjectSet, STMEMBLK_Handle_t *mem_handle, STAUD_InitParams_t *InitParams_p);
#if defined(ST_OPTIMIZE_BUFFER)
static ST_ErrorCode_t	GetNumOfBlocks(ObjectSet	CurObjectSet, U32 *NumBlock);
#endif
static U32 GetModuleIdentifier(STAUD_Object_t CurObject);
static U32 GetModuleOffset(STAUD_Object_t CurObject);
static ST_ErrorCode_t	InitCurrentModule(STAUD_Object_t ObjectID, STAUD_InitParams_t *InitParams_p, STMEMBLK_Handle_t *mem_handle, AudDriver_t *audDriver_p);
static ST_ErrorCode_t	InitMemoryPool(U32 PoolAllocationCount, U32 NumBlocks, U32 BlockSize, BOOL AllocateFromEMBX,	STAVMEM_PartitionHandle_t AVMEMPartition,
										ST_Partition_t *CPUPartition_p, STMEMBLK_Handle_t * mem_handle, AudDriver_t *audDriver_p);
static ST_ErrorCode_t GetFirstFreeMemBlock(U8 *PoolAllocationCount_p);
static ST_ErrorCode_t AllocateDummyBuffer(STAUD_InitParams_t *InitParams_p, AudDriver_t *audDriver_p);

/*----------------------------------------------------------------------------
Decoder management functions
-----------------------------------------------------------------------------*/

static ST_ErrorCode_t AddToDriver(AudDriver_t	*AudDriver_p,STAUD_Object_t	Object_Instance, STAUD_Handle_t	Handle);

static ST_ErrorCode_t AddToTopofDriver(AudDriver_t	*AudDriver_p,STAUD_Object_t	Object_Instance, STAUD_Handle_t	Handle);

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
    /* Return the object control block (or NULL) to the caller */
    return qp;
} /*PESES_GetControlBlockFromHandle()*/


static ST_ErrorCode_t AddToDriver(AudDriver_t	*AudDriver_p,STAUD_Object_t	Object_Instance, STAUD_Handle_t	Handle)
{
    STAUD_DrvChainConstituent_t *element ;
    STAUD_DrvChainConstituent_t *Temp;
    if(AudDriver_p)
    {
        Temp = (STAUD_DrvChainConstituent_t *)memory_allocate(AudDriver_p->CPUPartition_p,sizeof(STAUD_DrvChainConstituent_t));
        if(!Temp)
        {
            return ST_ERROR_NO_MEMORY;
        }
        memset(Temp,0,sizeof(STAUD_DrvChainConstituent_t));
        Temp->Handle				= Handle;
        Temp->Object_Instance	= Object_Instance;
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

static ST_ErrorCode_t AddToTopofDriver(AudDriver_t	*AudDriver_p,STAUD_Object_t	Object_Instance, STAUD_Handle_t	Handle)
{
    STAUD_DrvChainConstituent_t *element ;
    STAUD_DrvChainConstituent_t *Temp;

    if(AudDriver_p)
    {
        Temp = (STAUD_DrvChainConstituent_t *)memory_allocate(AudDriver_p->CPUPartition_p,sizeof(STAUD_DrvChainConstituent_t));
        if(!Temp)
        {
            return ST_ERROR_NO_MEMORY;
        }
        memset(Temp,0,sizeof(STAUD_DrvChainConstituent_t));
        Temp->Handle				= Handle;
        Temp->Object_Instance	= Object_Instance;
        if((U32)AudDriver_p->Handle == AUDIO_INVALID_HANDLE)
        {
            AudDriver_p->Handle = Temp;
        }
        else
        {
            element = AudDriver_p->Handle;
            AudDriver_p->Handle = Temp; /*insert at the top*/
            Temp->Next_p = element;		/*push the stack down*/
        }
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    return ST_NO_ERROR;
}



ST_ErrorCode_t RemoveFromDriver(AudDriver_t	*AudDriver_p,STAUD_Handle_t	Handle)
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
                    * we have just removed the queue head.
                    */
                    AudDriver_p->Handle = this_p->Next_p;
                }

                memset(this_p,0,sizeof(STAUD_DrvChainConstituent_t));
                /*Now Deallocate the memory*/
                memory_deallocate(AudDriver_p->CPUPartition_p,this_p);
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
STAUD_Handle_t	GetHandle(AudDriver_t *drv_p,STAUD_Object_t	Object)
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

static ST_ErrorCode_t	InitMemoryPool(U32 PoolAllocationCount, U32 NumBlocks, U32 BlockSize, BOOL AllocateFromEMBX,
										STAVMEM_PartitionHandle_t AVMEMPartition, ST_Partition_t *CPUPartition_p,
										STMEMBLK_Handle_t * mem_handle, AudDriver_t *audDriver_p)
{
    MemPoolInitParams_t	mem_InitParams;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    memset(&mem_InitParams,0,sizeof(MemPoolInitParams_t));

    mem_InitParams.NumBlocks = NumBlocks;
    mem_InitParams.BlockSize = BlockSize;
    mem_InitParams.AllocateFromEMBX = AllocateFromEMBX;
    mem_InitParams.AVMEMPartition = AVMEMPartition;
    mem_InitParams.CPUPartition_p = CPUPartition_p;

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
        case STAUD_OBJECT_OUTPUT_PCMP0:
        case STAUD_OBJECT_OUTPUT_SPDIF0:
        case STAUD_OBJECT_INPUT_PCM0:
        case STAUD_OBJECT_MIXER0:
            Offset = 0;
            break;
        case STAUD_OBJECT_INPUT_CD1:
        case STAUD_OBJECT_DECODER_COMPRESSED1:
            Offset = 1;
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
        case STAUD_OBJECT_OUTPUT_SPDIF0:
            Identifier = SPDIF_PLAYER_0;
            break;
        default:
            break;
    }
    return Identifier;
}

ST_ErrorCode_t 	CheckMixerInChain(STAUD_Object_t CurObjectSetInUse,STAUD_InitParams_t *InitParams_p)
{
    U8				DriverIndex = 0;	/* Index into driver array, for selecting particular driver configuration */
    U8				CurObjectSetCount,currentObjectCount;
    STAUD_Object_t	*CurObject;
    ObjectSet		*CurObjectSet;	/* Current object set */
    DriverCofig		*CurDriverConfig;	/* Current Driver config */

    U8 CountObjectSet = 0;
    DriverIndex = InitParams_p->DriverIndex;
    CurObjectSetCount= 0;

    /* Get hold of current driver */
    CurDriverConfig = &(AudioDriver[DriverIndex]);

    /* Get hold of current object Set */
    CurObjectSet = (ObjectSet*)((ObjectSet*)CurDriverConfig + CurObjectSetCount);
    CurObject = (STAUD_Object_t*)((STAUD_Object_t*)CurObjectSet+0);
    while(CurObjectSetInUse != *CurObject)
    {
        CurObjectSetCount++;
        CurObjectSet = (ObjectSet*)((ObjectSet*)CurDriverConfig + CurObjectSetCount);
        CurObject = (STAUD_Object_t* )((STAUD_Object_t* )CurObjectSet + 0);
    }
    while((*CurObject  != DRIVER_DONE) && (CountObjectSet <= MAX_OBJECTSET_PER_DRIVER))
    {
        if(*CurObject == OBJECTSET_DONE)
        {
            CurObjectSetCount++;
            CurObjectSet = (ObjectSet*)((ObjectSet*)CurDriverConfig + CurObjectSetCount);
            CurObject = (STAUD_Object_t* )((STAUD_Object_t* )CurObjectSet + 0);
            currentObjectCount = 0;
        }
        if(*CurObject == STAUD_OBJECT_MIXER0)
        {
            return 1;
        }
        else
        {
            if(*CurObject  == DRIVER_DONE)
                continue;
            else
                CurObject++;
        }
    }
    return 0;
}
static ST_ErrorCode_t	InitCurrentModule(STAUD_Object_t ObjectID, STAUD_InitParams_t *InitParams_p, STMEMBLK_Handle_t *mem_handle, AudDriver_t *audDriver_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 ModuleOffset = 0;

    switch(ObjectID)
    {
        case STAUD_OBJECT_INPUT_CD0:
        case STAUD_OBJECT_INPUT_CD1:
            {
                STPESES_Handle_t		PESESHandle;
                ModuleOffset = GetModuleOffset(ObjectID);
                if (PESESInitParams[ModuleOffset] == NULL)
                {
                    PESESInitParams[ModuleOffset] = (PESESInitParams_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(PESESInitParams_t));

                    if(!PESESInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(PESESInitParams[ModuleOffset],0,sizeof(PESESInitParams_t));
                }

                PESESInitParams[ModuleOffset]->PESbufferSize	= PES_BUFFER_SIZE;

                PESESInitParams[ModuleOffset]->OpBufHandle	= mem_handle[0];
                PESESInitParams[ModuleOffset]->AVMEMPartition = InitParams_p->AVMEMPartition;

                PESESInitParams[ModuleOffset]->AudioBufferPartition = (InitParams_p->AllocateFromEMBX)?0:InitParams_p->BufferPartition;

                PESESInitParams[ModuleOffset]->CPUPartition_p = InitParams_p->CPUPartition_p;
                PESESInitParams[ModuleOffset]->EnableMSPPSupport =  InitParams_p->EnableMSPPSupport;
                strcpy(PESESInitParams[ModuleOffset]->EvtHandlerName, InitParams_p->EvtHandlerName);
                strcpy(PESESInitParams[ModuleOffset]->Name,PES_ES_DeviceName[ModuleOffset]);
#ifdef STAUD_COMMON_EVENTS
                strcpy(PESESInitParams[ModuleOffset]->EvtGeneratorName,InitParams_p->RegistrantDeviceName);
#else
                strcpy(PESESInitParams[ModuleOffset]->EvtGeneratorName,PES_ES_DeviceName[ModuleOffset]);
#endif
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                PESESInitParams[ModuleOffset]->CLKRV_Handle = audDriver_p->CLKRV_Handle;
                /* CLKRV_Handle should be different from clock and synchro */
                PESESInitParams[ModuleOffset]->CLKRV_HandleForSynchro = audDriver_p->CLKRV_HandleForSynchro;
#endif
                PESESInitParams[ModuleOffset]->ObjectID = ObjectID;
                PESESInitParams[ModuleOffset]->MixerEnabled = CheckMixerInChain(ObjectID, InitParams_p);
                error = PESES_Init(PES_ES_DeviceName[ModuleOffset], PESESInitParams[ModuleOffset], &PESESHandle);
                if (error != ST_NO_ERROR)
                {
                    STTBX_Print(("PESES initialtion failed\n"));
                }
                else
                {
                    error = AddToTopofDriver(audDriver_p, ObjectID,PESESHandle);
                }

                memory_deallocate(InitParams_p->CPUPartition_p, PESESInitParams[ModuleOffset]);
                PESESInitParams[ModuleOffset] = NULL;
            }
            break;
#ifdef INPUT_PCM_SUPPORT
        case STAUD_OBJECT_INPUT_PCM0:
            {
                STPCMInput_Handle_t PCMInputHandle;
                ModuleOffset = GetModuleOffset(ObjectID);
                if(PCMIInitParams[ModuleOffset] == NULL)
                {
                    PCMIInitParams[ModuleOffset] = (STAudPCMInputInit_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(STAudPCMInputInit_t));
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
                    STTBX_Print(("STAud_Input_pcm0 initialtion failed\n"));
                }
                else
                {
                    error = AddToTopofDriver(audDriver_p, ObjectID, PCMInputHandle);
                }

                memory_deallocate(InitParams_p->CPUPartition_p, PCMIInitParams[ModuleOffset]);
                PCMIInitParams[ModuleOffset] = NULL;
            }
            break;
#endif
        case STAUD_OBJECT_DECODER_COMPRESSED0:
        case STAUD_OBJECT_DECODER_COMPRESSED1:
            {
                STAud_DecHandle_t		DecoderHandle;
                ModuleOffset = GetModuleOffset(ObjectID);
                if (DecInitParams[ModuleOffset] == NULL)
                {
                    DecInitParams[ModuleOffset] = (STAud_DecInitParams_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(STAud_DecInitParams_t));
                    if(!DecInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(DecInitParams[ModuleOffset],0,sizeof(STAud_DecInitParams_t));
                }

                DecInitParams[ModuleOffset]->AVMEMPartition  = InitParams_p->AVMEMPartition;
                DecInitParams[ModuleOffset]->CPUPartition    = InitParams_p->CPUPartition_p;
                DecInitParams[ModuleOffset]->DriverPartition	= InitParams_p->CPUPartition_p;
                DecInitParams[ModuleOffset]->OutBufHandle    = mem_handle[0];
                DecInitParams[ModuleOffset]->NumChannels     = InitParams_p->NumChannels;
                DecInitParams[ModuleOffset]->AmphionAccess   = audDriver_p->AmphionAccess_p;
                DecInitParams[ModuleOffset]->AmphionInChannel= audDriver_p->AmphionInChannel;
                DecInitParams[ModuleOffset]->AmphionOutChannel= audDriver_p->AmphionOutChannel;

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

                memory_deallocate(InitParams_p->CPUPartition_p, DecInitParams[ModuleOffset]);
                DecInitParams[ModuleOffset] = NULL;

            }
            break;
#ifdef MIXER_SUPPORT

        case STAUD_OBJECT_MIXER0:
            {
                STAud_MixerHandle_t		MixerHandle;
                ModuleOffset = GetModuleOffset(ObjectID);
                if (MixerInitParams[ModuleOffset] == NULL)
                {
                    MixerInitParams[ModuleOffset] = (STAud_MixInitParams_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(STAud_MixInitParams_t));
                    if(!MixerInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(MixerInitParams[ModuleOffset],0,sizeof(STAud_MixInitParams_t));
                }

                MixerInitParams[ModuleOffset]->AVMEMPartition  = InitParams_p->AVMEMPartition;
                MixerInitParams[ModuleOffset]->DriverPartition = InitParams_p->CPUPartition_p;
                strcpy(MixerInitParams[ModuleOffset]->EvtHandlerName, InitParams_p->EvtHandlerName);
#ifdef STAUD_COMMON_EVENTS
                strcpy(MixerInitParams[ModuleOffset]->EvtGeneratorName,InitParams_p->RegistrantDeviceName);
#else
                strcpy(MixerInitParams[ModuleOffset]->EvtGeneratorName,Mixer_DeviceName[0]);
#endif
                MixerInitParams[ModuleOffset]->OutBMHandle = mem_handle[0];
                MixerInitParams[ModuleOffset]->NumChannels = InitParams_p->NumChannels;
                MixerInitParams[ModuleOffset]->ObjectID = ObjectID;

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

                memory_deallocate(InitParams_p->CPUPartition_p, MixerInitParams[ModuleOffset]);
                MixerInitParams[ModuleOffset] = NULL;

            }
            break;
#endif


        case STAUD_OBJECT_OUTPUT_PCMP0:
            {
                STPCMPLAYER_Handle_t	pcmPlayerHandle;
                ModuleOffset = GetModuleOffset(ObjectID);
                if (PCMPInitParams[ModuleOffset] == NULL)
                {
                    PCMPInitParams[ModuleOffset] = (PCMPlayerInitParams_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(PCMPlayerInitParams_t));
                    if(!PCMPInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(PCMPInitParams[ModuleOffset],0,sizeof(PCMPlayerInitParams_t));
                }

                PCMPInitParams[ModuleOffset]->pcmPlayerIdentifier			= GetModuleIdentifier(ObjectID);//PCM_PLAYER_0;
                PCMPInitParams[ModuleOffset]->pcmPlayerNumNode				= 6;
                PCMPInitParams[ModuleOffset]->CPUPartition					= InitParams_p->CPUPartition_p;
                PCMPInitParams[ModuleOffset]->AVMEMPartition				= InitParams_p->AVMEMPartition;

                PCMPInitParams[ModuleOffset]->DummyBufferPartition			= (InitParams_p->AllocateFromEMBX)?0:InitParams_p->BufferPartition;
                PCMPInitParams[ModuleOffset]->dummyBufferSize				= audDriver_p->dummyBufferSize;
                PCMPInitParams[ModuleOffset]->dummyBufferBaseAddress		= audDriver_p->dummyBufferBaseAddress;


                strcpy(PCMPInitParams[ModuleOffset]->EvtHandlerName, InitParams_p->EvtHandlerName);
#ifdef STAUD_COMMON_EVENTS
                strcpy(PCMPInitParams[ModuleOffset]->EvtGeneratorName,InitParams_p->RegistrantDeviceName);
#else
                strcpy(PCMPInitParams[ModuleOffset]->EvtGeneratorName, PCMPlayer_DeviceName[ModuleOffset]); //PCM player 0 will recive his own events
#endif

                PCMPInitParams[ModuleOffset]->skipEventID					= (U32)STAUD_AVSYNC_SKIP_EVT;
                PCMPInitParams[ModuleOffset]->pauseEventID					= (U32)STAUD_AVSYNC_PAUSE_EVT;

                PCMPInitParams[ModuleOffset]->pcmPlayerDataConfig			= InitParams_p->PCMOutParams;
                PCMPInitParams[ModuleOffset]->NumChannels           		= InitParams_p->NumChannels;
                PCMPInitParams[ModuleOffset]->EnableMSPPSupport            = InitParams_p->EnableMSPPSupport;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                PCMPInitParams[ModuleOffset]->CLKRV_Handle = audDriver_p->CLKRV_Handle;
                /* CLKRV_Handle should be different from clock and synchro */
                PCMPInitParams[ModuleOffset]->CLKRV_HandleForSynchro = audDriver_p->CLKRV_HandleForSynchro;
#endif
                PCMPInitParams[ModuleOffset]->ObjectID = STAUD_OBJECT_OUTPUT_PCMP0;
                error = STAud_PCMPlayerInit(PCMPlayer_DeviceName[ModuleOffset], &pcmPlayerHandle, PCMPInitParams[ModuleOffset]);
                if (error != ST_NO_ERROR)
                {
                    STTBX_Print(("PCM player 0 initialized failed\n"));
                }
                else
                {
                    error = AddToTopofDriver(audDriver_p, ObjectID, pcmPlayerHandle);
                }

                memory_deallocate(InitParams_p->CPUPartition_p, PCMPInitParams[ModuleOffset]);
                PCMPInitParams[ModuleOffset] = NULL;
            }
            break;


        case STAUD_OBJECT_OUTPUT_SPDIF0:
            {
                STSPDIFPLAYER_Handle_t	spdifPlayerHandle;
                ModuleOffset = GetModuleOffset(ObjectID);

                if (SPDIFPInitParams[ModuleOffset] == NULL)
                {
                    SPDIFPInitParams[ModuleOffset] = (SPDIFPlayerInitParams_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(SPDIFPlayerInitParams_t));
                    if(!SPDIFPInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(SPDIFPInitParams[ModuleOffset],0,sizeof(SPDIFPlayerInitParams_t));
                }

                SPDIFPInitParams[ModuleOffset]->spdifPlayerIdentifier					= GetModuleIdentifier(ObjectID);
                SPDIFPInitParams[ModuleOffset]->spdifPlayerNumNode						= 6;
                SPDIFPInitParams[ModuleOffset]->CPUPartition							= InitParams_p->CPUPartition_p;
                SPDIFPInitParams[ModuleOffset]->AVMEMPartition							= InitParams_p->AVMEMPartition;

                SPDIFPInitParams[ModuleOffset]->DummyBufferPartition					= (InitParams_p->AllocateFromEMBX)?0:InitParams_p->BufferPartition;
                SPDIFPInitParams[ModuleOffset]->dummyBufferSize						= audDriver_p->dummyBufferSize;
                SPDIFPInitParams[ModuleOffset]->dummyBufferBaseAddress					= audDriver_p->dummyBufferBaseAddress;


                SPDIFPInitParams[ModuleOffset]->SPDIFMode								= InitParams_p->SPDIFMode;
                SPDIFPInitParams[ModuleOffset]->SPDIFPlayerOutParams					= InitParams_p->SPDIFOutParams;
                strcpy(SPDIFPInitParams[ModuleOffset]->EvtHandlerName, InitParams_p->EvtHandlerName);
#ifdef STAUD_COMMON_EVENTS
                strcpy(SPDIFPInitParams[ModuleOffset]->EvtGeneratorName,InitParams_p->RegistrantDeviceName);
#else
                strcpy(SPDIFPInitParams[ModuleOffset]->EvtGeneratorName, SPDIFPlayer_DeviceName[ModuleOffset]); //PCM player 0 will recive his own events
#endif
                SPDIFPInitParams[ModuleOffset]->skipEventID				= (U32)STAUD_AVSYNC_SKIP_EVT;
                SPDIFPInitParams[ModuleOffset]->pauseEventID				= (U32)STAUD_AVSYNC_PAUSE_EVT;
                SPDIFPInitParams[ModuleOffset]->NumChannels                = InitParams_p->NumChannels;
                SPDIFPInitParams[ModuleOffset]->EnableMSPPSupport          = InitParams_p->EnableMSPPSupport;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                SPDIFPInitParams[ModuleOffset]->CLKRV_Handle = audDriver_p->CLKRV_Handle;
                /* CLKRV_Handle should be different from clock and synchro */
                SPDIFPInitParams[ModuleOffset]->CLKRV_HandleForSynchro = audDriver_p->CLKRV_HandleForSynchro;
#endif
                SPDIFPInitParams[ModuleOffset]->ObjectID = ObjectID;
                //SPDIFPInitParams[ModuleOffset]->compressedMemoryBlockManagerHandle1 = 0;
                error = STAud_SPDIFPlayerInit(SPDIFPlayer_DeviceName[ModuleOffset], &spdifPlayerHandle, SPDIFPInitParams[ModuleOffset]);
                if (error != ST_NO_ERROR)
                {
                    STTBX_Print(("SPDIF player 0 initialtion failed\n"));
                }
                else
                {
                    error = AddToTopofDriver(audDriver_p, ObjectID, spdifPlayerHandle);
                }

                memory_deallocate(InitParams_p->CPUPartition_p, SPDIFPInitParams[ModuleOffset]);
                SPDIFPInitParams[ModuleOffset] = NULL;

            }

            break;
        default:
            break;
    }

    return error;
}

#if defined (ST_OPTIMIZE_BUFFER)
static ST_ErrorCode_t	GetNumOfBlocks(ObjectSet	CurObjectSet, U32 *NumBlock)
{
    ST_ErrorCode_t	Error = ST_NO_ERROR;
    U32 currentObjectCount = 0;
    STAUD_Object_t	CurObject;//, CurInputObject;

    CurObject		= CurObjectSet[currentObjectCount];
    *NumBlock = 2; /*Default value */
    while (CurObject != OBJECTSET_DONE)
    {
        switch (CurObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
                *NumBlock = NUM_NODES_PARSER-3;
                break;
            case STAUD_OBJECT_INPUT_CD1:
                *NumBlock = NUM_NODES_PARSER;
                break;
            case STAUD_OBJECT_INPUT_PCM0:
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
            case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER:
            case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER:
            case STAUD_OBJECT_FRAME_PROCESSOR:

                break;

            case STAUD_OBJECT_MIXER0:
            case STAUD_OBJECT_OUTPUT_PCMP0:
            case STAUD_OBJECT_OUTPUT_SPDIF0:
                *NumBlock = NUM_NODES_BEFORE_PLAYER;
                break;


            default:
                break;
        }
        currentObjectCount++;
        CurObject		= CurObjectSet[currentObjectCount];

    }

    return Error;
}
#endif

static  ST_ErrorCode_t	InitDependentModuleParams(ObjectSet	CurObjectSet, STMEMBLK_Handle_t *mem_handle, STAUD_InitParams_t *InitParams_p)
{
    ST_ErrorCode_t	Error = ST_NO_ERROR;
    U32 currentObjectCount;
    STAUD_Object_t	CurObject;/*, CurInputObject;*/
    U32 ModuleOffset = 0;

    currentObjectCount = 1;
    CurObject		= CurObjectSet[currentObjectCount];

    while (CurObject != OBJECTSET_DONE)
    {
        switch (CurObject)
        {
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:
                ModuleOffset = GetModuleOffset(CurObject);
                if (DecInitParams[ModuleOffset] == NULL)
                {
                    DecInitParams[ModuleOffset] = (STAud_DecInitParams_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(STAud_DecInitParams_t));
                    if(!DecInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(DecInitParams[ModuleOffset],0,sizeof(STAud_DecInitParams_t));
                }
                DecInitParams[ModuleOffset]->InBufHandle = mem_handle[0];
                break;

#ifdef MIXER_SUPPORT

            case STAUD_OBJECT_MIXER0:
                ModuleOffset = GetModuleOffset(CurObject);
                if (MixerInitParams[ModuleOffset] == NULL)
                {
                    MixerInitParams[ModuleOffset] = (STAud_MixInitParams_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(STAud_MixInitParams_t));
                    if(!MixerInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(MixerInitParams[ModuleOffset],0,sizeof(STAud_MixInitParams_t));
                }
                //check if we have space for more inputs.
                if(MixerInitParams[ModuleOffset]->NoOfRegisteredInput == ACC_MIXER_MAX_NB_INPUT)
                {
                    memory_deallocate(InitParams_p->CPUPartition_p, MixerInitParams[ModuleOffset]);
                    MixerInitParams[ModuleOffset] = NULL;
                    return ST_ERROR_NO_MEMORY;
                }
                // Add cases for mixer inputs
                MixerInitParams[ModuleOffset]->InBMHandle[MixerInitParams[ModuleOffset]->NoOfRegisteredInput] = mem_handle[0];
                MixerInitParams[ModuleOffset]->NoOfRegisteredInput ++;
                break;
#endif


            case STAUD_OBJECT_OUTPUT_PCMP0:
                ModuleOffset = GetModuleOffset(CurObject);
                if (PCMPInitParams[ModuleOffset] == NULL)
                {
                    PCMPInitParams[ModuleOffset] = (PCMPlayerInitParams_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(PCMPlayerInitParams_t));
                    if(!PCMPInitParams[ModuleOffset])
                    {
                        return ST_ERROR_NO_MEMORY;
                    }
                    memset(PCMPInitParams[ModuleOffset],0,sizeof(PCMPlayerInitParams_t));
                }
                PCMPInitParams[ModuleOffset]->memoryBlockManagerHandle = mem_handle[0];
                break;


            case STAUD_OBJECT_OUTPUT_SPDIF0:
                ModuleOffset = GetModuleOffset(CurObject);
                if (SPDIFPInitParams[ModuleOffset] == NULL)
                {
                    SPDIFPInitParams[ModuleOffset] = (SPDIFPlayerInitParams_t *)memory_allocate(InitParams_p->CPUPartition_p,sizeof(SPDIFPlayerInitParams_t));
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
                        SPDIFPInitParams[ModuleOffset]->pcmMemoryBlockManagerHandle				= mem_handle[0];
                    }
                    else
                    {
                        SPDIFPInitParams[ModuleOffset]->compressedMemoryBlockManagerHandle0		= mem_handle[0];
                    }
                    // Temp change for DDP transcoding
                    //SPDIFPInitParams[ModuleOffset]->compressedMemoryBlockManagerHandle1		= mem_handle[0];
                }
                else if ((CurObjectSet[0] & STAUD_CLASS_DECODER) == STAUD_CLASS_DECODER)
                {
                    // Modification done so that SPDIF player can get data from decoder
                    //if (CurObjectSet[0] == STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER)
                    // We consider this to be an decoder object
                    SPDIFPInitParams[ModuleOffset]->pcmMemoryBlockManagerHandle				= mem_handle[0];
                    SPDIFPInitParams[ModuleOffset]->compressedMemoryBlockManagerHandle1        = mem_handle[1];
                }
                else if (((CurObjectSet[0] & STAUD_CLASS_POSTPROC) == STAUD_CLASS_POSTPROC) || ((CurObjectSet[0] & STAUD_CLASS_MIXER) == STAUD_CLASS_MIXER))
                {
                    SPDIFPInitParams[ModuleOffset]->pcmMemoryBlockManagerHandle				= mem_handle[0];
                }
                else if ((CurObjectSet[0] & STAUD_CLASS_ENCODER) == STAUD_CLASS_ENCODER)
                {
                    SPDIFPInitParams[ModuleOffset]->compressedMemoryBlockManagerHandle2        = mem_handle[0];
                }

                break;
            case STAUD_OBJECT_FRAME_PROCESSOR:

                break;
            default:
                break;
        }
        currentObjectCount++;
        CurObject		= CurObjectSet[currentObjectCount];

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
ST_ErrorCode_t FreeDummyBuffer(AudDriver_t *audDriver_p)
{
    ST_ErrorCode_t	Error = ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    if(audDriver_p->DummyBufferHandle)
    {
        FreeBlockParams.PartitionHandle	= audDriver_p->BufferPartition;
        Error |= STAVMEM_FreeBlock(&FreeBlockParams, &(audDriver_p->DummyBufferHandle));
        audDriver_p->DummyBufferHandle = 0;
    }
    return Error;
}

static ST_ErrorCode_t AllocateDummyBuffer(STAUD_InitParams_t *InitParams_p, AudDriver_t *audDriver_p)
{
    ST_ErrorCode_t	Error = ST_NO_ERROR;
    STAVMEM_AllocBlockParams_t  AllocParams;
    audDriver_p->dummyBufferSize = (((MAX_SAMPLES_PER_FRAME/192)*192) * InitParams_p->NumChannels * NUM_BYTES_PER_CHANNEL) * 2; // We need double the size of max pcm buffer

    if(InitParams_p->AllocateFromEMBX == FALSE && audDriver_p->DummyBufferHandle == 0)
    {
        AllocParams.PartitionHandle = audDriver_p->BufferPartition;
        AllocParams.Alignment = 64;
        AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocParams.ForbiddenRangeArray_p = NULL;
        AllocParams.ForbiddenBorderArray_p = NULL;
        AllocParams.NumberOfForbiddenRanges = 0;
        AllocParams.NumberOfForbiddenBorders = 0;
        AllocParams.Size = audDriver_p->dummyBufferSize;

        Error = STAVMEM_AllocBlock(&AllocParams, &audDriver_p->DummyBufferHandle);
        if(Error != ST_NO_ERROR)
        {
            return Error;
        }

        Error = STAVMEM_GetBlockAddress(audDriver_p->DummyBufferHandle, (void *)(&(audDriver_p->dummyBufferBaseAddress)));
        if(Error != ST_NO_ERROR)
        {
            return Error;
        }

    }
#ifndef STAUD_REMOVE_EMBX
    else
    {
        Error = EMBX_OpenTransport(EMBX_TRANSPORT_NAME, &audDriver_p->tp);
        if(Error != EMBX_SUCCESS)
        {
            return Error;
        }

        Error = EMBX_Alloc(audDriver_p->tp, audDriver_p->dummyBufferSize, (EMBX_VOID **)(&audDriver_p->dummyBufferBaseAddress));
        if(Error != EMBX_SUCCESS)
        {
            return Error;
        }
    }
#endif

    return Error;
}


ST_ErrorCode_t 	STAud_CreateDriver(STAUD_InitParams_t *InitParams_p, AudDriver_t *audDriver_p)
{
    ST_ErrorCode_t	Error = ST_NO_ERROR;
    U8				DriverIndex = 0;	/* Index into driver array, for selecting particular driver configuration */
    U8				CurObjectSetCount;
    STAUD_Object_t	*CurObject;
    ObjectSet		*CurObjectSet;	/* Current object set */
    DriverCofig		*CurDriverConfig;	/* Current Driver config */

    U8	PoolAllocationCount = 0;
    U8 CountObjectSet = 0;
    STAUD_Handle_t Obj_Handle;

    U32 NumBlock;
    U32 DecoderCount=0;
    STMEMBLK_Handle_t 	mem_handle[MAX_OUTPUT_BLOCKS_PER_MODULE];
#if MIXER_INPUT_FROM_FILE
    STMEMBLK_Handle_t 	mem_handle1;
#endif

    DriverIndex = InitParams_p->DriverIndex;
    CurObjectSetCount= 0;

    /* Get hold of current driver */
    CurDriverConfig = &(AudioDriver[DriverIndex]);

    /* Get hold of current object Set */
    CurObjectSet = (ObjectSet*)((ObjectSet*)CurDriverConfig + CurObjectSetCount);
    CurObject = (STAUD_Object_t*)((STAUD_Object_t*)CurObjectSet+0);
    {
	    memset((U8*)PESESInitParams, 0, sizeof(PESESInitParams[0])*NUM_PARSER);
    	memset((U8*)DecInitParams, 0, sizeof(DecInitParams[0])*NUM_DECODER);
#ifdef MIXER_SUPPORT
    	memset((U8*)MixerInitParams, 0, sizeof(MixerInitParams[0])*NUM_MIXER);
#endif
    	memset((U8*)PCMPInitParams, 0, sizeof(PCMPInitParams[0])*NUM_PCMPLAYER);
    	memset((U8*)SPDIFPInitParams, 0, sizeof(SPDIFPInitParams[0])*NUM_SPDIFPLAYER);
#ifdef FRAME_PROCESSOR_SUPPORT
    	memset((U8*)DataPInitParams, 0, sizeof(DataPInitParams[0])*NUM_DATAPROCESSOR);
#endif
#ifdef INPUT_PCM_SUPPORT
    	memset((U8*)PCMIInitParams, 0, sizeof(PCMIInitParams[0])*NUM_PCMINPUT);
#endif
    }
    Error = GetFirstFreeMemBlock(&PoolAllocationCount);
    if(Error != ST_NO_ERROR)
    {
        return ST_ERROR_NO_FREE_HANDLES;
    }
    /* Parse and build the full driver structure */
    while((*CurObject  != DRIVER_DONE)&& (CountObjectSet <= MAX_OBJECTSET_PER_DRIVER))
    {
        /* Initialize modules/objects */
        memset(mem_handle, 0, sizeof(STMEMBLK_Handle_t)*MAX_OUTPUT_BLOCKS_PER_MODULE);

        switch(*CurObject)
        {
            case STAUD_OBJECT_INPUT_CD0:
            case STAUD_OBJECT_INPUT_CD1:
                {
                    U32 CompressedBufferSize = (*CurObject == STAUD_OBJECT_INPUT_CD0)?MAX_INPUT_COMPRESSED_FRAME_SIZE_MPEG:MAX_INPUT_COMPRESSED_FRAME_SIZE_AC3;
#if defined(PCM_SUPPORT)
                    if(!CheckMixerInChain(*CurObject, InitParams_p))
                    {
                        NumBlock = (*CurObject == STAUD_OBJECT_INPUT_CD0)? 6 : 6;
                    }
                    else
                    {
                        NumBlock = (*CurObject == STAUD_OBJECT_INPUT_CD0)? 3 : 6;
                    }
#else
                    NumBlock = (*CurObject == STAUD_OBJECT_INPUT_CD0)?3:6;
#endif

                    /* Initialize the output buffer pool for this module*/
                    Error |= InitMemoryPool(PoolAllocationCount, NumBlock, CompressedBufferSize, InitParams_p->AllocateFromEMBX,
                    	InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);
                    /* Initialize current Module */
                    Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);
                    /* Initialize dependent module parameters*/
                    Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

                    /* Increment pool count for next unit's use */
                    PoolAllocationCount++;
                    CountObjectSet ++;
                }
                break;

#ifdef INPUT_PCM_SUPPORT
            case STAUD_OBJECT_INPUT_PCM0:
#if defined (ST_OPTIMIZE_BUFFER)
                Error |= GetNumOfBlocks(*CurObjectSet,&NumBlock);
#else
                NumBlock = NUM_NODES_PCMINPUT;
#endif
                memset(mem_handle, 0, sizeof(STMEMBLK_Handle_t)*4);
                /* Initialize the output buffer pool for this module*/

                Error |= InitMemoryPool(PoolAllocationCount, NumBlock, (U32)(InitParams_p->NumChannels * NUM_BYTES_PER_CHANNEL * MAX_SAMPLES_PER_FRAME), InitParams_p->AllocateFromEMBX,
                InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);

                /* Initialize current Module */
                Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

                /* Initialize dependent module parameters*/
                Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

                /* Increment pool count for next unit's use */
                PoolAllocationCount++;
                CountObjectSet ++;
                break;
#endif
            case STAUD_OBJECT_DECODER_COMPRESSED0:
            case STAUD_OBJECT_DECODER_COMPRESSED1:

            /* Initialize the output buffer pool for this module*/
#if defined (ST_OPTIMIZE_BUFFER)
                Error |= GetNumOfBlocks(*CurObjectSet,&NumBlock);
#else
                NumBlock = NUM_NODES_DECODER;
#endif
                Error |= InitMemoryPool(PoolAllocationCount, NumBlock, (U32)(InitParams_p->NumChannels * NUM_BYTES_PER_CHANNEL * MAX_SAMPLES_PER_FRAME), InitParams_p->AllocateFromEMBX,
                InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);

                PoolAllocationCount++;

                /* Initialize current Module */
                Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

                /* Initialize dependent module parameters*/
                Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

                /* Increment pool count for next unit's use */
                PoolAllocationCount++;
                CountObjectSet ++;
                DecoderCount++;
                break;

#ifdef MIXER_SUPPORT
            case STAUD_OBJECT_MIXER0:
#if defined(ST_OPTIMIZE_BUFFER)
                Error |= GetNumOfBlocks(*CurObjectSet,&NumBlock);
#else
                NumBlock = NUM_NODES_MIXER;
#endif
                memset(mem_handle, 0, sizeof(STMEMBLK_Handle_t)*4);
                /* Initialize the output buffer pool for this module*/
                Error |= InitMemoryPool(PoolAllocationCount, NumBlock, (U32)(InitParams_p->NumChannels * NUM_BYTES_PER_CHANNEL * MIXER_OUT_PCM_SAMPLES), InitParams_p->AllocateFromEMBX,
                InitParams_p->BufferPartition, InitParams_p->CPUPartition_p, &(mem_handle[0]), audDriver_p);


                /* This code is specially added to test mixer with a dummy input. Should be removed from here*/
                /* Initialize current Module */
                Error |= InitCurrentModule(*CurObject, InitParams_p, mem_handle, audDriver_p);

                /* Initialize dependent module parameters*/
                Error |= InitDependentModuleParams(*CurObjectSet, mem_handle, InitParams_p);

                /* Increment pool count for next unit's use */
                PoolAllocationCount++;
                CountObjectSet ++;
                break;
#endif

            case STAUD_OBJECT_OUTPUT_PCMP0:
                AllocateDummyBuffer(InitParams_p, audDriver_p);
                /* Initialize current Module */

                Error |= InitCurrentModule(*CurObject, InitParams_p, 0, audDriver_p);

                /* Initialize dependent module parameters*/
                Error |= InitDependentModuleParams(*CurObjectSet, 0, InitParams_p);
                CountObjectSet ++;

                break;


            case STAUD_OBJECT_OUTPUT_SPDIF0:

                AllocateDummyBuffer(InitParams_p, audDriver_p);
                /*For ALSA Driver Parser --> SPDIF Player and Parser has PCM data*/
                /* Initialize current Module */


                Error |= InitCurrentModule(*CurObject, InitParams_p, 0, audDriver_p);

                /* Initialize dependent module parameters*/
                Error |= InitDependentModuleParams(*CurObjectSet, 0, InitParams_p);
                CountObjectSet ++;
                break;
#ifdef FRAME_PROCESSOR_SUPPORT

                case STAUD_OBJECT_FRAME_PROCESSOR:
                memset(mem_handle, 0, sizeof(STMEMBLK_Handle_t));
                /* Initialize current Module */
                Error |= InitCurrentModule(*CurObject, InitParams_p, 0, audDriver_p);

                /* Initialize dependent module parameters*/
                Error |= InitDependentModuleParams(*CurObjectSet, 0, InitParams_p);
                CountObjectSet ++;

                break;
#endif
            case OBJECTSET_DONE:
            //add bms to driver queue
                break;

            default:
                break;

        }
        CurObjectSetCount++;
        CurObjectSet = (ObjectSet*)((ObjectSet*)CurDriverConfig + CurObjectSetCount);
        CurObject = (STAUD_Object_t*)((STAUD_Object_t*)CurObjectSet+0);

    }
    if(DecoderCount==1)
    {
        Obj_Handle = GetHandle(audDriver_p,	(STAUD_Object_t)STAUD_OBJECT_DECODER_COMPRESSED0);

        if(!Obj_Handle)
        {
            Obj_Handle = GetHandle(audDriver_p,	(STAUD_Object_t)STAUD_OBJECT_DECODER_COMPRESSED1);
        }
        if(Obj_Handle)
        {
            Error |= STAUD_SetDecodingType(Obj_Handle);
        }
        else
        {
            Error = ST_ERROR_INVALID_HANDLE;
        }
    }
    return Error;
}

/* Register events to be sent */
ST_ErrorCode_t STAud_RegisterEvents(AudDriver_t *audDriver_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_OpenParams_t EVT_OpenParams;

    /* Open the ST Event */
    Error |= STEVT_Open(audDriver_p->EvtHandlerName,&EVT_OpenParams,&audDriver_p->EvtHandle);

    /* Register to the events */
    if(Error == ST_NO_ERROR)
    {
        /*Stopped Event*/
        Error |= STEVT_RegisterDeviceEvent(audDriver_p->EvtHandle,audDriver_p->Name,STAUD_STOPPED_EVT,&audDriver_p->EventIDStopped);
        /*Resume event*/
        Error |= STEVT_RegisterDeviceEvent(audDriver_p->EvtHandle,audDriver_p->Name,STAUD_RESUME_EVT,&audDriver_p->EventIDResumed);
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


