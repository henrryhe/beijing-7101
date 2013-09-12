/*****************************************************************************
 *
 *  Module      : stvid_ioctl_lib.c
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description : Implementation for calling STAPI ioctl functions
 *  Copyright   : STMicroelectronics (c)2006
 *****************************************************************************/
/*#define DEBUG_FB_MAPPING */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include "stvid.h"
#include "stvid_ioctl.h"

#include "stddefs.h"        /* for ST_MAX_DEVICE_NAME */
#include "api.h"            /* for STVID_MAX_DEVICE & STVID_MAX_OPEN */

#include "buffers.h"        /* for FrameBuffers_t & VIDBUFF_FrameBufferList_t */
#include "stos.h"

/*** CONSTANTS **********************************************************/
#ifdef DEBUG_FB_MAPPING
#define FBMapping_Debug_Info(x)     printf x
#else
#define FBMapping_Debug_Info(x)
#endif

/* Priority (from vid_inj.c) */
#define STVID_CALLBACK_INJECTOR_TASK_PRIORITY       ((MAX_USER_PRIORITY-15)+12)

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/

/* PES mapping management */
typedef struct
{
    /* Call backs management */
    ST_ErrorCode_t    (*GetWriteAddress_p)  (void *const Handle,void ** const Address_p);
    void              (*InformReadAddress_p)(void *const Handle,void  * const Address);
    void               *BufferHandle;
    
    /* PES buffer management */
    size_t              size;
    void               *uaddr;
    void               *kaddr;
} VidCallbackMap_t;

/* Per video device data intended to be available by pointers from API */
typedef struct APIVideoInstance_s
{
    ST_DeviceName_t DeviceName;
    STVID_Handle_t      VideoHandlers[STVID_MAX_OPEN];

    STEVT_Handle_t      EventsHandle;
    struct {
         STVID_ScalingFactors_t HorizontalFactors[2];
         STVID_ScalingFactors_t VerticalFactors[7];
    } VideoDeviceData;

    /* Frame buffers */
    U32                     NbFrameBuffers;
    FrameBuffers_t        * FrameBuffers_p;
        
    /* User/kernel callbacks */
    VidCallbackMap_t    CallbackMap;
} APIVideoInstance_t;

/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/
static int                  fd = -1; /* not open */
static int                  UseCount = 0; /* Number of initialised devices */

static semaphore_t         *VideoInstanceSemaphore_p = NULL;
static APIVideoInstance_t   VideoInstanceArray[STVID_MAX_DEVICE];

/* Call backs management */
static semaphore_t         *VIDi_Callback_Locker = NULL;
static pthread_t            callback_thread;
static void                *VID_CallbackThread(void *v);
static BOOL                 callback_exit = FALSE;


/*** LOCAL FUNCTIONS *********************************************************/
static APIVideoInstance_t * GetVidInstanceFromHandle(STVID_Handle_t Handle);
static FrameBuffers_t * GetVideoFrameBuffer(const ST_DeviceName_t DeviceName, void * KernelAddress_p);
static BOOL Transform_PictureInfos(APIVideoInstance_t * VidInstance_p, STVID_PictureInfos_t * PictureInfos_p);
static BOOL Transform_PictureParams(APIVideoInstance_t * VidInstance_p, STVID_PictureParams_t * Params_p);
static void NewAllocatedFrameBuffer_CB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void NewRemovedFrameBuffer_CB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);

static ST_ErrorCode_t MapPESBufferToUserSpace(VidCallbackMap_t * PESMap_p, void ** Address_pp, U32 Size);
static ST_ErrorCode_t UnmapPESBufferFromUserSpace(VidCallbackMap_t * PESMap_p);


/*******************************************************************************
Name        : GetVidInstanceFromName
Description : Get a video instance from its name
*******************************************************************************/
static APIVideoInstance_t * GetVidInstanceFromName(ST_DeviceName_t DeviceName)
{
    U32     i;

    if (strcmp(DeviceName, "") == 0)
    {
        return((APIVideoInstance_t *)NULL);
    }

    for (i=0 ; i<STVID_MAX_DEVICE ; i++)
    {
        if (strcmp(VideoInstanceArray[i].DeviceName, DeviceName) == 0)
        {
            /* Device Name is found, returns pointer */
            return(&VideoInstanceArray[i]);
        }
    }

    FBMapping_Debug_Info(("%s: WARNING '%s' device not found !!!!\n", __FUNCTION__, DeviceName));

    return((APIVideoInstance_t *)NULL);
}

/*******************************************************************************
Name        : GetVidInstanceFromHandle
Description : Get a video instance from one of its handles
*******************************************************************************/
static APIVideoInstance_t * GetVidInstanceFromHandle(STVID_Handle_t Handle)
{
    U32     i, j;
    
    if ((void *)Handle == NULL)
    {
        return((APIVideoInstance_t*)NULL);
    }

    for (i=0 ; i<STVID_MAX_DEVICE ; i++)
    {
        for (j=0 ; j<STVID_MAX_OPEN ; j++)
        {
            if (VideoInstanceArray[i].VideoHandlers[j] == Handle)
            {
                /* Video Instance is found, returns pointer */
                return(&VideoInstanceArray[i]);
            }
        }
    }

    FBMapping_Debug_Info(("%s: WARNING handle 0x%.8x not found !!!!\n", __FUNCTION__, (U32)Handle));

    return((APIVideoInstance_t*)NULL);
}

/*******************************************************************************
Name        : GetVideoFrameBuffer
Description : Get associated mapped address of a frame buffer
*******************************************************************************/
static FrameBuffers_t * GetVideoFrameBuffer(const ST_DeviceName_t DeviceName, void * KernelAddress_p)
{
    APIVideoInstance_t    * VidInstance_p;
    U32                     i, j;

    if (strcmp(DeviceName, "") == 0)
    {
        /* Device Name is empty: event subscribe has been made without a registrant name */
        /* Try then to find the kernel address among all the stored addresses in order to identify the video instance */
        for (j=0 , VidInstance_p=VideoInstanceArray ; j<STVID_MAX_DEVICE ; j++, VidInstance_p++)
        {
            if (VidInstance_p->FrameBuffers_p == NULL)
                continue;

            for (i=0 ; i<VidInstance_p->NbFrameBuffers ; i++)
            {
                if (((U32)VidInstance_p->FrameBuffers_p[i].KernelAddress_p | REGION2) == ((U32)KernelAddress_p | REGION2))
                {
                    /* Kernel address is found, return mapping info pointer */
                    return(&(VidInstance_p->FrameBuffers_p[i]));
                }
            }
        }
    }
    else
    {
        /* Device Name is not empty, event subscribe has been made with a registrant name */
        /* Video instance is identified */
        if ((VidInstance_p=GetVidInstanceFromName((void *)DeviceName)) != (APIVideoInstance_t *)NULL)
        {
            for (i=0 ; i<VidInstance_p->NbFrameBuffers ; i++)
            {
                if (((U32)VidInstance_p->FrameBuffers_p[i].KernelAddress_p | REGION2) == ((U32)KernelAddress_p | REGION2))
                {
                    /* Kernel address is found, return mapping info pointer */
                    return(&(VidInstance_p->FrameBuffers_p[i]));
                }
            }
        }
    }

    FBMapping_Debug_Info(("%s(): WARNING 0x%.8x -> Not found !!!!!\n", __FUNCTION__, (U32)KernelAddress_p));

    return((FrameBuffers_t*)NULL);
}

/*******************************************************************************
Name        : Transform_PictureInfos
Description : Transforms STVID_PictureInfos_t structure content from kernel to user space
*******************************************************************************/
static BOOL Transform_PictureInfos(APIVideoInstance_t * VidInstance_p, STVID_PictureInfos_t * PictureInfos_p)
{
    BOOL    Found = FALSE;
    U32     i;

    if (   (VidInstance_p->FrameBuffers_p == NULL)
        || (PictureInfos_p == NULL))
    {
        return FALSE;
    }

    if (   (PictureInfos_p->VideoParams.DecimationFactors == STVID_DECIMATION_FACTOR_NONE)
        || (PictureInfos_p->VideoParams.DoesNonDecimatedExist))
    {
        /* Process main frame buffer */
        for (i=0 , Found=FALSE ; (i<VidInstance_p->NbFrameBuffers) && !Found ; i++)
        {
            if (((U32)VidInstance_p->FrameBuffers_p[i].KernelAddress_p | REGION2) == ((U32)PictureInfos_p->BitmapParams.Data1_p | REGION2))
            {
                U32  Offset = PictureInfos_p->BitmapParams.Data2_p - PictureInfos_p->BitmapParams.Data1_p;

                PictureInfos_p->BitmapParams.Data1_p = VidInstance_p->FrameBuffers_p[i].MappedAddress_p;
                PictureInfos_p->BitmapParams.Data2_p = (void *)((U32)VidInstance_p->FrameBuffers_p[i].MappedAddress_p + Offset);

                /* Address has been found and processed, exits */
                Found = TRUE;
            }
        }

        if (!Found)
        {
            /* Address has not been found, no need to continue */
            return Found;
        }
    }

    /* Process decimated frame buffer */
    if (PictureInfos_p->VideoParams.DecimationFactors != STVID_DECIMATION_FACTOR_NONE)
    {
        for (i=0 , Found=FALSE ; (i<VidInstance_p->NbFrameBuffers) && !Found ; i++)
        {
            if (((U32)VidInstance_p->FrameBuffers_p[i].KernelAddress_p | REGION2) == ((U32)PictureInfos_p->VideoParams.DecimatedBitmapParams.Data1_p | REGION2))
            {
                U32  Offset = PictureInfos_p->VideoParams.DecimatedBitmapParams.Data2_p - PictureInfos_p->VideoParams.DecimatedBitmapParams.Data1_p;

                PictureInfos_p->VideoParams.DecimatedBitmapParams.Data1_p = VidInstance_p->FrameBuffers_p[i].MappedAddress_p;
                PictureInfos_p->VideoParams.DecimatedBitmapParams.Data2_p = (void *)((U32)VidInstance_p->FrameBuffers_p[i].MappedAddress_p + Offset);

                /* Address has been found and processed, exits */
                Found = TRUE;
            }
        }
    }

    return(Found);
}

/*******************************************************************************
Name        : Transform_PictureParams
Description : Transforms STVID_PictureParams_t structure content from kernel to user space
*******************************************************************************/
static BOOL Transform_PictureParams(APIVideoInstance_t * VidInstance_p, STVID_PictureParams_t * Params_p)
{
    U32     i;

    if (   (VidInstance_p->FrameBuffers_p == NULL)
        || (Params_p == NULL))
    {
        return FALSE;
    }
    
    for (i=0 ; i<VidInstance_p->NbFrameBuffers ; i++)
    {
        if (((U32)VidInstance_p->FrameBuffers_p[i].KernelAddress_p | REGION2) == ((U32)Params_p->Data | REGION2))
        {
            Params_p->Data = VidInstance_p->FrameBuffers_p[i].MappedAddress_p;

            /* Address has been found exit */
            return(TRUE);
        }
    }

    return(FALSE);
}

/*******************************************************************************
Name        : NewAllocatedFrameBuffer_CB
Description :
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void NewAllocatedFrameBuffer_CB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    VIDBUFF_FrameBufferList_t * FrameBufferList_p = (VIDBUFF_FrameBufferList_t *)EventData_p;
    APIVideoInstance_t        * VidInstance_p;
    FrameBuffers_t            * FrameBuffers_p;
    U32                         i;

    if ((ErrorCode=STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData_p)) != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s: event error (0x%.8x)!!!", __FUNCTION__, ErrorCode));
    }

    if (   (FrameBufferList_p->NbFrameBuffers == 0)
        || (FrameBufferList_p->FrameBuffers_p == NULL))
    {
        return;
    }

    STOS_SemaphoreWait(VideoInstanceSemaphore_p);
    if ((VidInstance_p=GetVidInstanceFromName((void *)RegistrantName)) != (APIVideoInstance_t *)NULL)
    {
        /* Allocates new frame buffer array for old frame buffers + new allocated ones */
        FrameBuffers_p = (FrameBuffers_t *)memory_allocate(NULL, (VidInstance_p->NbFrameBuffers + FrameBufferList_p->NbFrameBuffers) * sizeof(FrameBuffers_t));
        if (FrameBuffers_p != NULL)
        {
            /* Copy newly allocated frame buffers to storage structure */
            memcpy(FrameBuffers_p, FrameBufferList_p->FrameBuffers_p, FrameBufferList_p->NbFrameBuffers * sizeof(FrameBuffers_t));

            /* mmap only newly allocated buffers since already existing have alreday been mmapped */
            for (i=0 ; i<FrameBufferList_p->NbFrameBuffers ; i++)
            {
                if (FrameBuffers_p[i].MappedAddress_p != NULL)
                {
                    /* Kernel has been ioremapped into MappedAddress_p field */
                    /* mmap is then possible ... */
                    FrameBuffers_p[i].MappedAddress_p = mmap(0,
                                                             FrameBuffers_p[i].Size,
                                                             PROT_READ|PROT_WRITE,
                                                             MAP_SHARED,
                                                             fd,
                                                             (U32)FrameBuffers_p[i].MappedAddress_p);
                }
            }
            /* Copies old allocated frame buffers to storage structure */
            if (VidInstance_p->FrameBuffers_p != NULL)
            {
                memcpy(FrameBuffers_p + FrameBufferList_p->NbFrameBuffers,
                       VidInstance_p->FrameBuffers_p,
                       VidInstance_p->NbFrameBuffers * sizeof(FrameBuffers_t));
                memory_deallocate(NULL, VidInstance_p->FrameBuffers_p);
            }

            /* The array contains now all allocated and mmaped buffers (old + newly allocated) */
            VidInstance_p->FrameBuffers_p = FrameBuffers_p;
            VidInstance_p->NbFrameBuffers = VidInstance_p->NbFrameBuffers + FrameBufferList_p->NbFrameBuffers;

#ifdef DEBUG_FB_MAPPING
            FBMapping_Debug_Info(("FB List: "));
            for (i=0 ; i<VidInstance_p->NbFrameBuffers ; i++)
            {
               FBMapping_Debug_Info(("0x%.8x-> ", (U32)VidInstance_p->FrameBuffers_p[i].KernelAddress_p));
            }
            FBMapping_Debug_Info(("NULL\n"));
#endif
        }
    }
    STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
}

/*******************************************************************************
Name        : NewRemovedFrameBuffer_CB
Description :
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void NewRemovedFrameBuffer_CB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    VIDBUFF_FrameBufferList_t * FrameBufferList_p = (VIDBUFF_FrameBufferList_t *)EventData_p;
    APIVideoInstance_t        * VidInstance_p;
    FrameBuffers_t            * RemovedFrameBuffers_p;
    U32                         NewNbFrameBuffers;
    U32                         i, j;

    if ((ErrorCode=STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData_p)) != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s: event error (0x%.8x)!!!", __FUNCTION__, ErrorCode));
    }

    if (   (FrameBufferList_p->NbFrameBuffers == 0)
        || (FrameBufferList_p->FrameBuffers_p == NULL))
    {
        return;
    }

    STOS_SemaphoreWait(VideoInstanceSemaphore_p);
    if ((VidInstance_p=GetVidInstanceFromName((void *)RegistrantName)) != (APIVideoInstance_t *)NULL)
    {
        /* FrameBufferList_p contains all deleted frame buffers */
        /* unmap all listed frame buffers */
        for (i=0 ; i<FrameBufferList_p->NbFrameBuffers ; i++)
        {
            /* Seek frame buffer into allocated FB array */
            RemovedFrameBuffers_p = NULL;
            for (j=0 ; (j<VidInstance_p->NbFrameBuffers) && (RemovedFrameBuffers_p == NULL); j++)
            {
                if (VidInstance_p->FrameBuffers_p[j].KernelAddress_p == FrameBufferList_p->FrameBuffers_p[i].KernelAddress_p)
                {
                    RemovedFrameBuffers_p = &(VidInstance_p->FrameBuffers_p[j]);
                }
            }

            /* FB has been found, unmap it ... */
            if (RemovedFrameBuffers_p != NULL)
            {
                /* Unmap the address */
                munmap((void *)RemovedFrameBuffers_p->MappedAddress_p, RemovedFrameBuffers_p->Size);

                /* Remove entry */
                RemovedFrameBuffers_p->MappedAddress_p = NULL;
                RemovedFrameBuffers_p->KernelAddress_p = NULL;
                RemovedFrameBuffers_p->Size = 0;
            }
        }

        /* Rebuild allocated FB array with remaining FB which have not been unmapped */
        NewNbFrameBuffers = 0;
        for (i=0 ; i<VidInstance_p->NbFrameBuffers ; i++)
        {
            if (VidInstance_p->FrameBuffers_p[i].KernelAddress_p != NULL)
            {
                /* FB is not NULL, has not then been unmapped, keep it in the new FB array */
                memcpy(&(VidInstance_p->FrameBuffers_p[NewNbFrameBuffers]), &(VidInstance_p->FrameBuffers_p[i]), sizeof(FrameBuffers_t));
                NewNbFrameBuffers++;
            }
        }
        VidInstance_p->NbFrameBuffers = NewNbFrameBuffers;

#ifdef DEBUG_FB_MAPPING
        FBMapping_Debug_Info(("FB List: "));
        for (i=0 ; i<VidInstance_p->NbFrameBuffers ; i++)
        {
           FBMapping_Debug_Info(("0x%.8x-> ", (U32)VidInstance_p->FrameBuffers_p[i].KernelAddress_p));
        }
        FBMapping_Debug_Info(("NULL\n"));
#endif
    }
    STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
}

/* ========================================================================
   Name:        VID_CallbackThread
   Description: Call Back thread
   ======================================================================== */
static void * VID_CallbackThread(void *v)
{
    STVID_Ioctl_CallbackData_t  CallbackData;
    VidCallbackMap_t           *callback;
    void                       *address = NULL;

    memset(&CallbackData, 0, sizeof(STVID_Ioctl_CallbackData_t));
    
    /* Infinite loop */
    /* ============= */
    while(1)
    {
        /* Cancelation entry point */
        /* ----------------------- */
        pthread_testcancel();
        
        /* Wait for a callback asking from the video injector */
        /* ------------------------------------------------- */
        if (ioctl(fd, STVID_IOC_AWAIT_CALLBACK, &CallbackData)==0)
        {
            /* Check if thread has to be killed */
            if (callback_exit) 
            {
                break;
            }
            
            /* Lock callback thread */
            STOS_SemaphoreWait(VIDi_Callback_Locker);
            
            /* RELEASE_CALLBACK IOCTL are not sent in a task termination purpose */
            CallbackData.Terminate = FALSE;

            /* Get callback descriptor */
            callback =(VidCallbackMap_t*)CallbackData.Handle;
            if (callback == NULL)
            {
                CallbackData.ErrorCode = ST_ERROR_BAD_PARAMETER;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): Callback is null !", __FUNCTION__));

                ioctl(fd, STVID_IOC_RELEASE_CALLBACK, &CallbackData);
                STOS_SemaphoreSignal(VIDi_Callback_Locker);
                continue;
            }
            
            /* If it is a get write pointer callback */
            if (CallbackData.CBType == STVID_CB_WRITE_PTR)
            {
                /* Check callback */
                if (callback->GetWriteAddress_p == NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): GetWriteAddress_p() is NULL !", __FUNCTION__));

                    ioctl(fd, STVID_IOC_RELEASE_CALLBACK, &CallbackData);
                    STOS_SemaphoreSignal(VIDi_Callback_Locker);
                    continue;
                }
                
                /* Call the user callback */
                CallbackData.ErrorCode = callback->GetWriteAddress_p(callback->BufferHandle, &address);
                if (CallbackData.ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): GetWriteAddress_p() returns an error 0x%08x !", __FUNCTION__, CallbackData.ErrorCode));

                    ioctl(fd, STVID_IOC_RELEASE_CALLBACK, &CallbackData);
                    STOS_SemaphoreSignal(VIDi_Callback_Locker);
                    continue;
                }
                /* If the address returned is null, inform the video driver */
                if (address == NULL)
                {
                    CallbackData.ErrorCode = ST_ERROR_BAD_PARAMETER;
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): GetWriteAddress_p() returns null address !", __FUNCTION__));

                    ioctl(fd, STVID_IOC_RELEASE_CALLBACK, &CallbackData);
                    STOS_SemaphoreSignal(VIDi_Callback_Locker);
                    continue;
                }

                /* Convert address to kernel space */
                CallbackData.Address = (void *)NULL;
                if (   (callback->size != 0)
                    && ((U32)address >= (U32)callback->uaddr)
                    && ((U32)address < (U32)callback->uaddr + (U32)callback->size))
                {
                    CallbackData.Address = (void *)((U32)callback->kaddr + ((U32)address - (U32)callback->uaddr));
                }
                else   /* address not in mapping range */
                {
                    CallbackData.ErrorCode = ST_ERROR_BAD_PARAMETER;

                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): GetWriteAddress_p() returns an invalid address 0x%8p !", __FUNCTION__, address));
                    ioctl(fd, STVID_IOC_RELEASE_CALLBACK, &CallbackData);
                    STOS_SemaphoreSignal(VIDi_Callback_Locker);
                    continue;
                }
            }
            
            /* If it is a set read pointer callback */
            if (CallbackData.CBType == STVID_CB_READ_PTR)
            {
                /* Check callback */
                if (callback->InformReadAddress_p == NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): InformReadAddress_p() is NULL !", __FUNCTION__));

                    ioctl(fd, STVID_IOC_RELEASE_CALLBACK, &CallbackData);
                    STOS_SemaphoreSignal(VIDi_Callback_Locker);
                    continue;
                }
                
                /* Check if callback address is NULL */
                if (CallbackData.Address == NULL)
                {
                    CallbackData.ErrorCode = ST_NO_ERROR;

                    /*STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): video instance ask to set read pointer to NULL !",__FUNCTION__));*/
                    ioctl(fd, STVID_IOC_RELEASE_CALLBACK, &CallbackData);
                    STOS_SemaphoreSignal(VIDi_Callback_Locker);
                    continue;
                }       

                /* Convert address to user space */
                address = (void *)NULL;
                if (   (callback->size != 0)
                    && ((U32)CallbackData.Address >= (U32)callback->kaddr)
                    && ((U32)CallbackData.Address < (U32)callback->kaddr + (U32)callback->size))
                {
                    address = (void *)((U32)callback->uaddr + ((U32)CallbackData.Address - (U32)callback->kaddr));
                }
                else   /* address not in mapping range */
                {
                    CallbackData.ErrorCode = ST_ERROR_BAD_PARAMETER;
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): Unable to find the kernel address in the entry table 0x%8p !", __FUNCTION__, CallbackData.Address));

                    ioctl(fd,STVID_IOC_RELEASE_CALLBACK,&CallbackData);
                    STOS_SemaphoreSignal(VIDi_Callback_Locker);
                    continue;
                } 

                /* Inform the user */
                callback->InformReadAddress_p(callback->BufferHandle, address);
                CallbackData.ErrorCode = ST_NO_ERROR;
            }
            /* Unlock callback thread */
            STOS_SemaphoreSignal(VIDi_Callback_Locker);
            /* Release the video injector callback */
            CallbackData.Terminate = FALSE;
            ioctl(fd, STVID_IOC_RELEASE_CALLBACK, &CallbackData);
        }
        /* Something wrong happens with IOCTL */
        /* ---------------------------------- */
        else
        {
            perror(strerror(errno));
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): STVID_IOC_AWAIT_CALLBACK(): Ioctl error\n", __FUNCTION__));

            sleep(1);
        }
    }
    
    return NULL;
}

/* ========================================================================
   Name:        STVID_MapPESBufferToUserSpace
   Description: Map the pes buffer to user space
   ======================================================================== */
#if !defined STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
ST_ErrorCode_t STVID_MapPESBufferToUserSpace(STVID_Handle_t Handle, void ** Address_pp, U32 Size)
{
    APIVideoInstance_t    * VidInstance_p;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if ((void *)Handle == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Map PES buffer address to user space in order to be used by application */
    STOS_SemaphoreWait(VideoInstanceSemaphore_p);
    if ((VidInstance_p=GetVidInstanceFromHandle(Handle)) != NULL)
    {
        ErrorCode = MapPESBufferToUserSpace(&VidInstance_p->CallbackMap, Address_pp, Size);
    }
    else
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
    
    return ErrorCode;
}
#endif

/* ========================================================================
   Name:        MapPESBufferToUserSpace
   Description: Map the pes buffer to user space
   ======================================================================== */
static ST_ErrorCode_t MapPESBufferToUserSpace(VidCallbackMap_t * PESMap_p, void ** Address_pp, U32 Size)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    off_t               pgoffset = (U32)(*Address_pp) % getpagesize();
    void              * kaddr = *Address_pp;
    void              * address;
    BOOL                AlreadyMapped = FALSE;

    /* Lock call back */
    /* -------------- */
    STOS_SemaphoreWait(VIDi_Callback_Locker);
    
    /* Check whenever not already mapped */
    if (PESMap_p->size != 0)
    {
        AlreadyMapped = TRUE;
    
        if (   (PESMap_p->size == Size)
            && (PESMap_p->kaddr == kaddr))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s(): pes buffer entry already mmapped",__FUNCTION__));

            *Address_pp = PESMap_p->uaddr;
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): Video pes buffer already mapped (0x%.8x)!", __FUNCTION__, (U32)kaddr));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    
    if (   (!AlreadyMapped)
        && (ErrorCode == ST_NO_ERROR))
    {
        /* Map the video buffer */
        /* -------------------- */
        address = mmap( 0,
                        Size + pgoffset,
                        PROT_READ|PROT_WRITE,
                        MAP_SHARED,
                        fd,
                        (off_t)kaddr - pgoffset);
        if (address != MAP_FAILED)
        {
            *Address_pp = address + pgoffset;
        
            /* Store mapping lookup for callbacks */
            /* ---------------------------------- */
            PESMap_p->size  = Size;
            PESMap_p->kaddr = kaddr;
            PESMap_p->uaddr = *Address_pp;
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): Unable to mmap a video pes buffer (0x%.8x)!", __FUNCTION__, (U32)kaddr));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    
    /* Release callback task */
    /* --------------------- */
    STOS_SemaphoreSignal(VIDi_Callback_Locker);
    
    return ErrorCode;
}

/* ========================================================================
   Name:        STVID_UnmapPESBufferFromUserSpace
   Description: Unmap the pes buffer from user space
   ======================================================================== */
#if !defined STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
ST_ErrorCode_t STVID_UnmapPESBufferFromUserSpace(STVID_Handle_t Handle)
{
    APIVideoInstance_t    * VidInstance_p;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if ((void *)Handle == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Map PES buffer address to user space in order to be used by application */
    STOS_SemaphoreWait(VideoInstanceSemaphore_p);
    if ((VidInstance_p=GetVidInstanceFromHandle(Handle)) != NULL)
    {
        ErrorCode = UnmapPESBufferFromUserSpace(&VidInstance_p->CallbackMap);
    }
    else
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
    
    return ErrorCode;
}
#endif

/* ========================================================================
   Name:        UnmapPESBufferFromUserSpace
   Description: Unmap the pes buffer from user space
   ======================================================================== */
static ST_ErrorCode_t UnmapPESBufferFromUserSpace(VidCallbackMap_t * PESMap_p)
{
    off_t   pgoffset = (off_t)(PESMap_p->uaddr) % getpagesize();

    /* Lock call back */
    /* -------------- */
    STOS_SemaphoreWait(VIDi_Callback_Locker);

    if (PESMap_p->uaddr != NULL)
    {
        /* Unmap the buffer */
        /* ---------------- */
        if (munmap(PESMap_p->uaddr - pgoffset, PESMap_p->size + pgoffset) == 0)
        {
            PESMap_p->size  = 0;
            PESMap_p->kaddr = NULL;
            PESMap_p->uaddr = NULL;
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): Unable to unmap the video pes buffer (0x%0.8x)!", __FUNCTION__, (U32)PESMap_p->uaddr));
            /* continue anyway ... */ 
        }
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s(): No video pes buffer mmapped !", __FUNCTION__));
        /* continue anyway ... */ 
    }   
    
    /* Release callback task */
    /* --------------------- */
    STOS_SemaphoreSignal(VIDi_Callback_Locker);

    /* Return no errors */
    /* ---------------- */
    return(ST_NO_ERROR);
}




/*** METHODS ****************************************************************/

/*******************************************************************************
Name        : STVID_Init
Description : Init VID driver
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Init(const ST_DeviceName_t DeviceName, const STVID_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    VID_Ioctl_Init_t    VID_Ioctl_Init;
    char               *devpath;
    U32                 i, j;

    STEVT_OpenParams_t              STEVT_OpenParams;
    STEVT_DeviceSubscribeParams_t   STEVT_SubscribeParams;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (InitParams_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)      /* First time: initialise devices and units as empty */
    {
        /* Creates a semaphore already reserved in order to  */
        /* be released only when everything is correctly initialized */
        if (VideoInstanceSemaphore_p == NULL)
        {
            VideoInstanceSemaphore_p = STOS_SemaphoreCreateFifo(NULL, 1);
        }
        
        STOS_SemaphoreWait(VideoInstanceSemaphore_p);
        for (i = 0; i < STVID_MAX_DEVICE; i++)
        {
            strncpy( VideoInstanceArray[i].DeviceName, "", ST_MAX_DEVICE_NAME-1);
            for (j=0 ; j<STVID_MAX_OPEN ; j++)
            {
                VideoInstanceArray[i].VideoHandlers[j] = (STVID_Handle_t)NULL;
            }
            /* Init Frame buffers mapping */
            VideoInstanceArray[i].NbFrameBuffers = 0;
            VideoInstanceArray[i].FrameBuffers_p = NULL;

            /* Init Callbacks & PES buffer mapping */
            VideoInstanceArray[i].CallbackMap.GetWriteAddress_p     = NULL;
            VideoInstanceArray[i].CallbackMap.InformReadAddress_p   = NULL;
            VideoInstanceArray[i].CallbackMap.BufferHandle          = NULL;
            VideoInstanceArray[i].CallbackMap.size = 0;
            VideoInstanceArray[i].CallbackMap.uaddr = NULL;
            VideoInstanceArray[i].CallbackMap.kaddr = NULL;
        }
        /* Semaphore can be signaled now ... */
        STOS_SemaphoreSignal(VideoInstanceSemaphore_p);

        devpath = getenv("STVID_IOCTL_DEV_PATH");       /* get the path for the device */
        if (devpath)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Opening %s", devpath));
            if ((fd = open(devpath, O_RDWR)) == -1)   /* Open device */
            {
                perror(strerror(errno));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                /* Device is opened, use counter is incremented */
                UseCount++;
            }
        }
        else
        {
            fprintf(stderr,"The dev path enviroment variable is not defined: STVID_IOCTL_DEV_PATH \n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }

        /* Add injector support in user mode */
        if (ErrorCode == ST_NO_ERROR)
        {
            callback_exit = FALSE;

            VIDi_Callback_Locker = STOS_SemaphoreCreateFifo(NULL,1);
            if (VIDi_Callback_Locker != NULL)
            {
                if (pthread_create(&callback_thread, NULL, VID_CallbackThread, NULL) == 0)
                {
                    struct sched_param params;

                    params.sched_priority = STVID_CALLBACK_INJECTOR_TASK_PRIORITY;
                    if (pthread_setschedparam(callback_thread, SCHED_RR, &params) != 0) 
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): Unable to change priority of the video callback read task !", __FUNCTION__));
                        /* We try to continue anyway... */
                    }
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): Unable to create the callback task !", __FUNCTION__));
                    STOS_SemaphoreDelete(NULL, VIDi_Callback_Locker);
                    ErrorCode = ST_ERROR_NO_MEMORY;
                }
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): Unable to create a semaphore for callback task !",__FUNCTION__));
                ErrorCode = ST_ERROR_NO_MEMORY;
            }

            if (ErrorCode != ST_NO_ERROR)
            {
                UseCount--;
                if (UseCount == 0)
                {
                    close(fd);
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s(): Closing module VID !!!", __FUNCTION__));
                }
            }
        }
    }
    else
    {
        /* Device already opened, increments use counter */
        UseCount++;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Copy parameters into IOCTL structure */
        strcpy(VID_Ioctl_Init.DeviceName, DeviceName);
        VID_Ioctl_Init.InitParams = *InitParams_p;

        /* IOCTL the command */
        if (ErrorCode == ST_NO_ERROR)
        {
            if (ioctl(fd, STVID_IOC_INIT, &VID_Ioctl_Init) != 0)
            {
                /* IOCTL failed */
                perror(strerror(errno));
                ErrorCode = ST_ERROR_BAD_PARAMETER;

                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
            }
            else
            {
                /* IOCTL is successfull: retrieve Error code */
                ErrorCode = VID_Ioctl_Init.ErrorCode;
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Init():Device '%s' initialised", VID_Ioctl_Init.DeviceName));
            }
        }

        if (ErrorCode == ST_NO_ERROR)
        {
            STOS_SemaphoreWait(VideoInstanceSemaphore_p);
            for (i = 0; i < STVID_MAX_DEVICE; i++)
            {
                if (strcmp( VideoInstanceArray[i].DeviceName, "") == 0)
                {   /* this element is free */
                    strncpy( VideoInstanceArray[i].DeviceName, DeviceName, ST_MAX_DEVICE_NAME-1);

                    /* Initialize Frame buffer info with default values */
                    VideoInstanceArray[i].NbFrameBuffers = 0;
                    VideoInstanceArray[i].FrameBuffers_p = NULL;

                    /* Init Callbacks & PES buffer mapping */
                    VideoInstanceArray[i].CallbackMap.GetWriteAddress_p     = NULL;
                    VideoInstanceArray[i].CallbackMap.InformReadAddress_p   = NULL;
                    VideoInstanceArray[i].CallbackMap.BufferHandle          = NULL;
                    VideoInstanceArray[i].CallbackMap.size = 0;
                    VideoInstanceArray[i].CallbackMap.uaddr = NULL;
                    VideoInstanceArray[i].CallbackMap.kaddr = NULL;

                    ErrorCode = STEVT_Open(InitParams_p->EvtHandlerName, &STEVT_OpenParams, &(VideoInstanceArray[i].EventsHandle));
                    /* Subscribe to events */
                    STEVT_SubscribeParams.NotifyCallback = NewAllocatedFrameBuffer_CB;
                    STEVT_SubscribeParams.SubscriberData_p = NULL;
                    ErrorCode = STEVT_SubscribeDeviceEvent(VideoInstanceArray[i].EventsHandle, DeviceName, VIDBUFF_NEW_ALLOCATED_FRAMEBUFFERS_EVT, &STEVT_SubscribeParams);

                    STEVT_SubscribeParams.NotifyCallback = NewRemovedFrameBuffer_CB;
                    STEVT_SubscribeParams.SubscriberData_p = NULL;
                    ErrorCode = STEVT_SubscribeDeviceEvent(VideoInstanceArray[i].EventsHandle, DeviceName, VIDBUFF_DEALLOCATED_FRAMEBUFFERS_EVT, &STEVT_SubscribeParams);
                    break;
                }
                else
                if (strcmp( VideoInstanceArray[i].DeviceName, DeviceName) == 0)
                {   /* Device already exists */
                    break;
                }
            }
            STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
        }
        else
        {
            UseCount--;
            if (UseCount == 0)
            {
                close(fd);
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s(): Closing module VID !!!", __FUNCTION__));
            }
        }
    }

    return(ErrorCode);
}   /* STVID_Init */


/*******************************************************************************
Name        : STVID_Open
Description : Open an instance of VID driver
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Open( const ST_DeviceName_t DeviceName,
                                const STVID_OpenParams_t * const OpenParams_p,
                                STVID_Handle_t * const Handle_p)
{
    VID_Ioctl_Open_t        VID_Ioctl_Open;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    APIVideoInstance_t    * VidInstance_p;
    U32                     i;

    /* Exit now if parameters are bad */
    if (   (OpenParams_p == NULL)                           /* There must be parameters ! */
        || (Handle_p == NULL)                               /* Pointer to handle should be valid ! */
        || (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    strcpy(VID_Ioctl_Open.DeviceName, DeviceName);
    VID_Ioctl_Open.OpenParams = *OpenParams_p;
    VID_Ioctl_Open.Handle = *Handle_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_OPEN, &VID_Ioctl_Open) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Open.ErrorCode;

        /* Copy parameters from IOCTL structure */
        *Handle_p = VID_Ioctl_Open.Handle;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Open():Device '%s' opened (Hdl: 0x%.8x)", VID_Ioctl_Open.DeviceName, (U32)VID_Ioctl_Open.Handle));

        STOS_SemaphoreWait(VideoInstanceSemaphore_p);
        if ((VidInstance_p=GetVidInstanceFromName((void *)DeviceName)) != (APIVideoInstance_t *)NULL)
        {
            for (i=0 ; i<STVID_MAX_OPEN ; i++)
            {
                if (VidInstance_p->VideoHandlers[i] == (STVID_Handle_t)NULL)
                {   /* This element is free */
                    VidInstance_p->VideoHandlers[i] = VID_Ioctl_Open.Handle;
                    break;
                }
                else
                if (VidInstance_p->VideoHandlers[i] == VID_Ioctl_Open.Handle)
                {   /* Already exists */
                    break;
                }
            }
        }
        STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_Close
Description : Close an instance of VID driver
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Close(STVID_Handle_t Handle)
{
    VID_Ioctl_Close_t       VID_Ioctl_Close;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    APIVideoInstance_t    * VidInstance_p;
    U32                     i;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_Close.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CLOSE, &VID_Ioctl_Close) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Close.ErrorCode;

        /* No parameter from IOCTL structure */
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Close():Device closed"));
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        STOS_SemaphoreWait(VideoInstanceSemaphore_p);
        if ((VidInstance_p=GetVidInstanceFromHandle(Handle)) != NULL)
        {
            for (i=0 ; i<STVID_MAX_OPEN ; i++)
            {
                if (VidInstance_p->VideoHandlers[i] == Handle)
                {
                    /* Video Instance is found, returns pointer */
                    VidInstance_p->VideoHandlers[i] = (STVID_Handle_t)NULL;
                    break;
                }
            }
        }
        STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_Term
Description : Terminate VID driver
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Term(const ST_DeviceName_t DeviceName, const STVID_TermParams_t *const TermParams_p)
{
    VID_Ioctl_Term_t        VID_Ioctl_Term;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    APIVideoInstance_t    * VidInstance_p;
    U32                     i;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (TermParams_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    strcpy(VID_Ioctl_Term.DeviceName, DeviceName);
    VID_Ioctl_Term.TermParams = *TermParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_TERM, &VID_Ioctl_Term) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Term.ErrorCode;
    }

    if (   (ErrorCode == ST_NO_ERROR)
        && (UseCount))
    {
        STOS_SemaphoreWait(VideoInstanceSemaphore_p);
        if ((VidInstance_p=GetVidInstanceFromName((void *)DeviceName)) != (APIVideoInstance_t *)NULL)
        {
            strncpy(VidInstance_p->DeviceName, "", ST_MAX_DEVICE_NAME-1);
            for (i=0 ; i<STVID_MAX_OPEN ; i++)
            {
                VidInstance_p->VideoHandlers[i] = (STVID_Handle_t)NULL;
            }

            if (VidInstance_p->FrameBuffers_p != NULL)
            {
                for (i=0 ; i<VidInstance_p->NbFrameBuffers ; i++)
                {
                    /* Unmap the address */
                    munmap((void *)VidInstance_p->FrameBuffers_p[i].MappedAddress_p, VidInstance_p->FrameBuffers_p[i].Size);
                }

                memory_deallocate(NULL, VidInstance_p->FrameBuffers_p);
                VidInstance_p->FrameBuffers_p = NULL;
            }
            VidInstance_p->NbFrameBuffers = 0;

            STEVT_UnsubscribeDeviceEvent(VidInstance_p->EventsHandle, DeviceName, VIDBUFF_NEW_ALLOCATED_FRAMEBUFFERS_EVT);
            STEVT_UnsubscribeDeviceEvent(VidInstance_p->EventsHandle, DeviceName, VIDBUFF_DEALLOCATED_FRAMEBUFFERS_EVT);
            STEVT_Close(VidInstance_p->EventsHandle);
            
            /* Unmap the PES buffers */
            UnmapPESBufferFromUserSpace(&VidInstance_p->CallbackMap);

            /* Reinit Callbacks */
            VidInstance_p->CallbackMap.GetWriteAddress_p     = NULL;
            VidInstance_p->CallbackMap.InformReadAddress_p   = NULL;
            VidInstance_p->CallbackMap.BufferHandle          = NULL;
        }
        STOS_SemaphoreSignal(VideoInstanceSemaphore_p);

        UseCount--;
        if (UseCount == 0)
        {
            STVID_Ioctl_CallbackData_t  CallbackData;

            callback_exit = TRUE;

            /* Release the video injector callback */
            CallbackData.Terminate = TRUE;
            ioctl(fd, STVID_IOC_RELEASE_CALLBACK, &CallbackData);

            pthread_join(callback_thread, NULL);
            pthread_cancel(callback_thread);

            STOS_SemaphoreDelete(NULL, VIDi_Callback_Locker);

            close(fd);
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s(): Device VID closed ", __FUNCTION__));
        }
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s():Device '%s' terminated", __FUNCTION__, VID_Ioctl_Term.DeviceName));
    }

    return(ErrorCode);
}   /* STVID_Term */

/*******************************************************************************
Name        : STVID_GetRevision
Description : Revision
Parameters  : void
Assumptions : void
Limitations : void
Returns     : ST_Revision_t
*******************************************************************************/
ST_Revision_t  STVID_GetRevision(void)
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return(VID_Revision);
} /* End of STVID_GetRevision() function */

/*******************************************************************************
Name        : STVID_GetCapability
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t  STVID_GetCapability( const ST_DeviceName_t DeviceName,
                                    STVID_Capability_t *  const Capability_p)
{
    VID_Ioctl_GetCapability_t           VID_Ioctl_GetCapability;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;
    APIVideoInstance_t        * VidInstance_p;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (Capability_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    strcpy( VID_Ioctl_GetCapability.DeviceName, DeviceName);
    VID_Ioctl_GetCapability.Capability = *Capability_p;

    STOS_SemaphoreWait(VideoInstanceSemaphore_p);
    if ((VidInstance_p=GetVidInstanceFromName((void *)DeviceName)) != (APIVideoInstance_t *)NULL)
    {
        VID_Ioctl_GetCapability.Capability.SupportedVerticalScaling.ScalingFactors_p = VidInstance_p->VideoDeviceData.VerticalFactors;
        VID_Ioctl_GetCapability.Capability.SupportedHorizontalScaling.ScalingFactors_p = VidInstance_p->VideoDeviceData.HorizontalFactors;
    }
    else
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    STOS_SemaphoreSignal(VideoInstanceSemaphore_p);

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Instance management error */
        return(ErrorCode);
    }

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETCAPABILITY, &VID_Ioctl_GetCapability) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetCapability.ErrorCode;

        /* Parameters from IOCTL structure */
        *Capability_p = VID_Ioctl_GetCapability.Capability;
    }

    return(ErrorCode);


}

#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : STVID_GetStatistics
Description :
Parameters  : void
Assumptions : void
Limitations : void
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVID_GetStatistics(const ST_DeviceName_t DeviceName, STVID_Statistics_t * const Statistics_p)
{

    VID_Ioctl_GetStatistics_t           VID_Ioctl_GetStatistics;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (Statistics_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    strcpy( VID_Ioctl_GetStatistics.DeviceName, DeviceName);
    VID_Ioctl_GetStatistics.Statistics = *Statistics_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETSTATISTICS, &VID_Ioctl_GetStatistics) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetStatistics.ErrorCode;

        /* Parameters from IOCTL structure */
        *Statistics_p = VID_Ioctl_GetStatistics.Statistics;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_ResetStatistics
Description :
Parameters  : void
Assumptions : void
Limitations : void
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVID_ResetStatistics(const ST_DeviceName_t DeviceName, const STVID_Statistics_t * const Statistics_p)
{
    VID_Ioctl_ResetStatistics_t           VID_Ioctl_ResetStatistics;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (Statistics_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }


    /* Copy parameters into IOCTL structure */
    strcpy( VID_Ioctl_ResetStatistics.DeviceName, DeviceName);
    VID_Ioctl_ResetStatistics.Statistics = *Statistics_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_RESETSTATISTICS, &VID_Ioctl_ResetStatistics) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_ResetStatistics.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}

#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
/*******************************************************************************
Name        : STVID_GetStatus
Description :
Parameters  : void
Assumptions : void
Limitations : void
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVID_GetStatus(const ST_DeviceName_t DeviceName, STVID_Status_t * const Status_p)
{

    VID_Ioctl_GetStatus_t               VID_Ioctl_GetStatus;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (   (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1)    /* Device name length should be respected */
        || ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
        || (Status_p == NULL)                           /* Parameters should not be NULL */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    strcpy( VID_Ioctl_GetStatus.DeviceName, DeviceName);
    VID_Ioctl_GetStatus.Status = *Status_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETSTATUS, &VID_Ioctl_GetStatus) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetStatus.ErrorCode;

        /* Parameters from IOCTL structure */
        *Status_p = VID_Ioctl_GetStatus.Status;
    }

    return(ErrorCode);
}
#endif /* STVID_DEBUG_GET_STATUS */


/*******************************************************************************
Name        : STVID_Clear
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Clear(const STVID_Handle_t Handle, const STVID_ClearParams_t * const Params_p)
{
    VID_Ioctl_Clear_t       VID_Ioctl_Clear;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Params_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_Clear.Handle = Handle;
    VID_Ioctl_Clear.Params = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CLEAR, &VID_Ioctl_Clear) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Clear.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_CloseViewPort
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_CloseViewPort(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VID_Ioctl_CloseViewPort_t      VID_Ioctl_CloseViewPort;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_CloseViewPort.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CLOSEVIEWPORT, &VID_Ioctl_CloseViewPort) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_CloseViewPort.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_ConfigureEvent
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
********************************************************************************/
ST_ErrorCode_t STVID_ConfigureEvent(const STVID_Handle_t Handle, const STEVT_EventID_t Event, const STVID_ConfigureEventParams_t * const Params_p)
{
    VID_Ioctl_ConfigureEvent_t       VID_Ioctl_ConfigureEvent;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Params_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_ConfigureEvent.Handle = Handle;
    VID_Ioctl_ConfigureEvent.Event = Event;
    VID_Ioctl_ConfigureEvent.Params = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CONFIGUREEVENT, &VID_Ioctl_ConfigureEvent) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_ConfigureEvent.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_DataInjectionCompleted
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DataInjectionCompleted(const STVID_Handle_t Handle, const STVID_DataInjectionCompletedParams_t * const Params_p)
{
    VID_Ioctl_DataInjectionCompleted_t       VID_Ioctl_DataInjectionCompleted;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Params_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DataInjectionCompleted.Handle = Handle;
    VID_Ioctl_DataInjectionCompleted.Params = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DATAINJECTIONCOMPLETED, &VID_Ioctl_DataInjectionCompleted) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DataInjectionCompleted.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_DeleteDataInputInterface
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DeleteDataInputInterface(const STVID_Handle_t Handle)
{
    VID_Ioctl_DeleteDataInputInterface_t       VID_Ioctl_DeleteDataInputInterface;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    APIVideoInstance_t    * VidInstance_p;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DeleteDataInputInterface.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DELETEDATAINPUTINTERFACE, &VID_Ioctl_DeleteDataInputInterface) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DeleteDataInputInterface.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    /* Remove the entry */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Reinit Callbacks of current instance */
        STOS_SemaphoreWait(VideoInstanceSemaphore_p);
        if ((VidInstance_p=GetVidInstanceFromHandle(Handle)) != NULL)
        {
            VidInstance_p->CallbackMap.GetWriteAddress_p     = NULL;
            VidInstance_p->CallbackMap.InformReadAddress_p   = NULL;
            VidInstance_p->CallbackMap.BufferHandle          = NULL;
        }
        STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_DisableBorderAlpha
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DisableBorderAlpha(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VID_Ioctl_DisableBorderAlpha_t      VID_Ioctl_DisableBorderAlpha;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DisableBorderAlpha.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DISABLEBORDERALPHA, &VID_Ioctl_DisableBorderAlpha) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DisableBorderAlpha.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_DisableColorKey
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DisableColorKey(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VID_Ioctl_DisableColorKey_t      VID_Ioctl_DisableColorKey;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DisableColorKey.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DISABLECOLORKEY, &VID_Ioctl_DisableColorKey) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DisableColorKey.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_DisableFrameRateConversion
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DisableFrameRateConversion(const STVID_Handle_t Handle)
{
    VID_Ioctl_DisableFrameRateConversion_t       VID_Ioctl_DisableFrameRateConversion;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DisableFrameRateConversion.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DISABLEFRAMERATECONVERSION, &VID_Ioctl_DisableFrameRateConversion) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DisableFrameRateConversion.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_DisableOutputWindow
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DisableOutputWindow(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VID_Ioctl_DisableOutputWindow_t      VID_Ioctl_DisableOutputWindow;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DisableOutputWindow.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DISABLEOUTPUTWINDOW, &VID_Ioctl_DisableOutputWindow) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DisableOutputWindow.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_DisableSynchronisation
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DisableSynchronisation(const STVID_Handle_t Handle)
{
    VID_Ioctl_DisableSynchronisation_t       VID_Ioctl_DisableSynchronisation;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DisableSynchronisation.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DISABLESYNCHRONISATION, &VID_Ioctl_DisableSynchronisation) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DisableSynchronisation.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_DuplicatePicture
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DuplicatePicture(const STVID_PictureParams_t * const Source_p, STVID_PictureParams_t * const Dest_p)
{
    VID_Ioctl_DuplicatePicture_t       VID_Ioctl_DuplicatePicture;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (Source_p == NULL)
        || (Dest_p == NULL))
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DuplicatePicture.Source = *Source_p;
    VID_Ioctl_DuplicatePicture.Dest = *Dest_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DUPLICATEPICTURE, &VID_Ioctl_DuplicatePicture) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DuplicatePicture.ErrorCode;
        *Dest_p = VID_Ioctl_DuplicatePicture.Dest;
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_EnableBorderAlpha
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_EnableBorderAlpha(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VID_Ioctl_EnableBorderAlpha_t      VID_Ioctl_EnableBorderAlpha;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_EnableBorderAlpha.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_ENABLEBORDERALPHA, &VID_Ioctl_EnableBorderAlpha) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_EnableBorderAlpha.ErrorCode;
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_EnableColorKey
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_EnableColorKey(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VID_Ioctl_EnableColorKey_t      VID_Ioctl_EnableColorKey;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_EnableColorKey.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_ENABLECOLORKEY, &VID_Ioctl_EnableColorKey) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_EnableColorKey.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_EnableFrameRateConversion
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_EnableFrameRateConversion(const STVID_Handle_t Handle)
{
    VID_Ioctl_EnableFrameRateConversion_t       VID_Ioctl_EnableFrameRateConversion;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_EnableFrameRateConversion.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_ENABLEFRAMERATECONVERSION, &VID_Ioctl_EnableFrameRateConversion) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_EnableFrameRateConversion.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_EnableOutputWindow
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_EnableOutputWindow(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VID_Ioctl_EnableOutputWindow_t      VID_Ioctl_EnableOutputWindow;
    ST_ErrorCode_t                      ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_EnableOutputWindow.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_ENABLEOUTPUTWINDOW, &VID_Ioctl_EnableOutputWindow) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_EnableOutputWindow.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_EnableSynchronisation
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_EnableSynchronisation(const STVID_Handle_t Handle)
{
    VID_Ioctl_EnableSynchronisation_t       VID_Ioctl_EnableSynchronisation;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_EnableSynchronisation.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_ENABLESYNCHRONISATION, &VID_Ioctl_EnableSynchronisation) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_EnableSynchronisation.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_ForceDecimationFactor
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_ForceDecimationFactor(const STVID_Handle_t Handle, const STVID_ForceDecimationFactorParams_t * const Params_p)
{
    VID_Ioctl_ForceDecimationFactor_t VID_Ioctl_ForceDecimationFactor;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Params_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_ForceDecimationFactor.Handle = Handle;
    VID_Ioctl_ForceDecimationFactor.ForceDecimationFactorParams = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_FORCEDECIMATIONFACTOR, &VID_Ioctl_ForceDecimationFactor) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_ForceDecimationFactor.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_Freeze
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Freeze(const STVID_Handle_t Handle, const STVID_Freeze_t * const Freeze_p)
{
    VID_Ioctl_Freeze_t      VID_Ioctl_Freeze;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Freeze_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_Freeze.Handle = Handle;
    VID_Ioctl_Freeze.Freeze = *Freeze_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_FREEZE, &VID_Ioctl_Freeze) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Freeze.ErrorCode;

        /* No parameter from IOCTL structure */
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_GetAlignIOWindows
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetAlignIOWindows(const STVID_ViewPortHandle_t ViewPortHandle,
                    S32 * const InputWinX_p,  S32 * const InputWinY_p,  U32 * const InputWinWidth_p,  U32 * const InputWinHeight_p,
                    S32 * const OutputWinX_p, S32 * const OutputWinY_p, U32 * const OutputWinWidth_p, U32 * const OutputWinHeight_p)
{
    VID_Ioctl_GetAlignIOWindows_t      VID_Ioctl_GetAlignIOWindows;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (InputWinX_p == NULL)
        || (InputWinY_p == NULL)
        || (InputWinWidth_p == NULL)
        || (InputWinHeight_p == NULL)
        || (OutputWinX_p == NULL)
        || (OutputWinY_p == NULL)
        || (OutputWinWidth_p == NULL)
        || (OutputWinHeight_p == NULL))
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetAlignIOWindows.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETALIGNIOWINDOWS, &VID_Ioctl_GetAlignIOWindows) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetAlignIOWindows.ErrorCode;

        /* Parameters from IOCTL structure */
        *InputWinX_p = VID_Ioctl_GetAlignIOWindows.InputWinX;
        *InputWinY_p = VID_Ioctl_GetAlignIOWindows.InputWinY;
        *InputWinWidth_p = VID_Ioctl_GetAlignIOWindows.InputWinWidth;
        *InputWinHeight_p = VID_Ioctl_GetAlignIOWindows.InputWinHeight;
        *OutputWinX_p = VID_Ioctl_GetAlignIOWindows.OutputWinX;
        *OutputWinY_p = VID_Ioctl_GetAlignIOWindows.OutputWinY;
        *OutputWinWidth_p = VID_Ioctl_GetAlignIOWindows.OutputWinWidth;
        *OutputWinHeight_p = VID_Ioctl_GetAlignIOWindows.OutputWinHeight;
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_GetBitBufferFreeSize
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetBitBufferFreeSize(const STVID_Handle_t Handle, U32 * const FreeSize_p)
{
    VID_Ioctl_GetBitBufferFreeSize_t      VID_Ioctl_GetBitBufferFreeSize;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (FreeSize_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetBitBufferFreeSize.Handle = Handle;
    VID_Ioctl_GetBitBufferFreeSize.FreeSize = *FreeSize_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETBITBUFFERFREESIZE, &VID_Ioctl_GetBitBufferFreeSize) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetBitBufferFreeSize.ErrorCode;

        /* Parameters from IOCTL structure */
        *FreeSize_p = VID_Ioctl_GetBitBufferFreeSize.FreeSize;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetBitBufferParams
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetBitBufferParams(const STVID_Handle_t Handle, void ** const BaseAddress_p, U32 * const InitSize_p)
{
    VID_Ioctl_GetBitBufferParams_t      VID_Ioctl_GetBitBufferParams;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if ( (BaseAddress_p == NULL) || (InitSize_p == NULL) )
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetBitBufferParams.Handle = Handle;
    VID_Ioctl_GetBitBufferParams.BaseAddress_p = *BaseAddress_p;
    VID_Ioctl_GetBitBufferParams.InitSize = *InitSize_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETBITBUFFERPARAMS, &VID_Ioctl_GetBitBufferParams) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetBitBufferParams.ErrorCode;

        /* Parameters from IOCTL structure */
        *BaseAddress_p = VID_Ioctl_GetBitBufferParams.BaseAddress_p;
        *InitSize_p = VID_Ioctl_GetBitBufferParams.InitSize;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetDataInputBufferParams
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetDataInputBufferParams(const STVID_Handle_t Handle,
                        void ** const BaseAddress_p, U32 * const Size_p)
{
    VID_Ioctl_GetDataInputBufferParams_t      VID_Ioctl_GetDataInputBufferParams;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
#ifdef STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
    APIVideoInstance_t    * VidInstance_p;
#endif

    if (   (Size_p == NULL)
        || (BaseAddress_p == NULL))
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetDataInputBufferParams.Handle = Handle;
    VID_Ioctl_GetDataInputBufferParams.Size = *Size_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETDATAINPUTBUFFERPARAMS, &VID_Ioctl_GetDataInputBufferParams) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetDataInputBufferParams.ErrorCode;

        /* Parameters from IOCTL structure */
        *BaseAddress_p = (void*)(VID_Ioctl_GetDataInputBufferParams.BaseAddress_p);
        *Size_p = VID_Ioctl_GetDataInputBufferParams.Size;
    }

#ifdef STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Map PES buffer address to user space in order to be used by application */
        STOS_SemaphoreWait(VideoInstanceSemaphore_p);
        if ((VidInstance_p=GetVidInstanceFromHandle(Handle)) != NULL)
        {
            ErrorCode = MapPESBufferToUserSpace(&VidInstance_p->CallbackMap, BaseAddress_p, *Size_p);
        }
        else
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        STOS_SemaphoreSignal(VideoInstanceSemaphore_p);

        if (ErrorCode != ST_NO_ERROR)
        {
            *BaseAddress_p = (void*)NULL;
            *Size_p = 0;
        }
    }
#endif

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetDecimationFactor
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetDecimationFactor(const STVID_Handle_t Handle, STVID_DecimationFactor_t * const DecimationFactor_p)
{
    VID_Ioctl_GetDecimationFactor_t      VID_Ioctl_GetDecimationFactor;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (DecimationFactor_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetDecimationFactor.Handle = Handle;
    /* VID_Ioctl_GetDecimationFactor.DecimationFactor = *DecimationFactor_p; */

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETDECIMATIONFACTOR, &VID_Ioctl_GetDecimationFactor) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetDecimationFactor.ErrorCode;

        /* Parameters from IOCTL structure */
        *DecimationFactor_p = VID_Ioctl_GetDecimationFactor.DecimationFactor;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetDecodedPictures
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetDecodedPictures(const STVID_Handle_t Handle, STVID_DecodedPictures_t * const DecodedPictures_p)
{
    VID_Ioctl_GetDecodedPictures_t      VID_Ioctl_GetDecodedPictures;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (DecodedPictures_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetDecodedPictures.Handle = Handle;
    VID_Ioctl_GetDecodedPictures.DecodedPictures = *DecodedPictures_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETDECODEDPICTURES, &VID_Ioctl_GetDecodedPictures) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetDecodedPictures.ErrorCode;

        /* Parameters from IOCTL structure */
        *DecodedPictures_p = VID_Ioctl_GetDecodedPictures.DecodedPictures;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetDisplayAspectRatioConversion
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetDisplayAspectRatioConversion(const STVID_ViewPortHandle_t ViewPortHandle, STVID_DisplayAspectRatioConversion_t * const Conversion_p)
{
    VID_Ioctl_GetDisplayAspectRatioConversion_t      VID_Ioctl_GetDisplayAspectRatioConversion;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Conversion_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetDisplayAspectRatioConversion.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_GetDisplayAspectRatioConversion.Conversion = *Conversion_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETDISPLAYASPECTRATIOCONVERSION, &VID_Ioctl_GetDisplayAspectRatioConversion) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetDisplayAspectRatioConversion.ErrorCode;

        /* Parameters from IOCTL structure */
        *Conversion_p = VID_Ioctl_GetDisplayAspectRatioConversion.Conversion;
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_GetErrorRecoveryMode
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetErrorRecoveryMode(const STVID_Handle_t Handle, STVID_ErrorRecoveryMode_t * const Mode_p)
{
    VID_Ioctl_GetErrorRecoveryMode_t      VID_Ioctl_GetErrorRecoveryMode;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Mode_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetErrorRecoveryMode.Handle = Handle;
    VID_Ioctl_GetErrorRecoveryMode.Mode = *Mode_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETERRORRECOVERYMODE, &VID_Ioctl_GetErrorRecoveryMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetErrorRecoveryMode.ErrorCode;

        /* Parameters from IOCTL structure */
        *Mode_p = VID_Ioctl_GetErrorRecoveryMode.Mode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetHDPIPParams
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetHDPIPParams(const STVID_Handle_t Handle, STVID_HDPIPParams_t * const HDPIPParams_p)
{
    VID_Ioctl_GetHDPIPParams_t      VID_Ioctl_GetHDPIPParams;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (HDPIPParams_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetHDPIPParams.Handle = Handle;
    VID_Ioctl_GetHDPIPParams.HDPIPParams = *HDPIPParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_VID_GETHDPIPPARAMS, &VID_Ioctl_GetHDPIPParams) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetHDPIPParams.ErrorCode;

        /* Parameters from IOCTL structure */
        *HDPIPParams_p = VID_Ioctl_GetHDPIPParams.HDPIPParams;

    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetInputWindowMode
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetInputWindowMode(const STVID_ViewPortHandle_t ViewPortHandle, BOOL * const AutoMode_p, STVID_WindowParams_t * const WinParams_p)
{
    VID_Ioctl_GetInputWindowMode_t      VID_Ioctl_GetInputWindowMode;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (AutoMode_p == NULL)
        || (WinParams_p == NULL))
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetInputWindowMode.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_GetInputWindowMode.AutoMode = *AutoMode_p;
    VID_Ioctl_GetInputWindowMode.WinParams = *WinParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETINPUTWINDOWMODE, &VID_Ioctl_GetInputWindowMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetInputWindowMode.ErrorCode;

        /* Parameters from IOCTL structure */
        *AutoMode_p = VID_Ioctl_GetInputWindowMode.AutoMode;
        *WinParams_p = VID_Ioctl_GetInputWindowMode.WinParams;
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_GetIOWindows
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetIOWindows(const STVID_ViewPortHandle_t ViewPortHandle,
                    S32 * const InputWinX_p,  S32 * const InputWinY_p,  U32 * const InputWinWidth_p,  U32 * const InputWinHeight_p,
                    S32 * const OutputWinX_p, S32 * const OutputWinY_p, U32 * const OutputWinWidth_p, U32 * const OutputWinHeight_p)
{
    VID_Ioctl_GetIOWindows_t      VID_Ioctl_GetIOWindows;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (InputWinX_p == NULL)
        || (InputWinY_p == NULL)
        || (InputWinWidth_p == NULL)
        || (InputWinHeight_p == NULL)
        || (OutputWinX_p == NULL)
        || (OutputWinY_p == NULL)
        || (OutputWinWidth_p == NULL)
        || (OutputWinHeight_p == NULL))
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetIOWindows.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETIOWINDOWS, &VID_Ioctl_GetIOWindows) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetIOWindows.ErrorCode;

        /* Parameters from IOCTL structure */
        *InputWinX_p = VID_Ioctl_GetIOWindows.InputWinX;
        *InputWinY_p = VID_Ioctl_GetIOWindows.InputWinY;
        *InputWinWidth_p = VID_Ioctl_GetIOWindows.InputWinWidth;
        *InputWinHeight_p = VID_Ioctl_GetIOWindows.InputWinHeight;

        *OutputWinX_p = VID_Ioctl_GetIOWindows.OutputWinX;
        *OutputWinY_p = VID_Ioctl_GetIOWindows.OutputWinY;
        *OutputWinWidth_p = VID_Ioctl_GetIOWindows.OutputWinWidth;
        *OutputWinHeight_p = VID_Ioctl_GetIOWindows.OutputWinHeight;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetMemoryProfile
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetMemoryProfile(const STVID_Handle_t Handle, STVID_MemoryProfile_t * const MemoryProfile_p)
{
    VID_Ioctl_GetMemoryProfile_t      VID_Ioctl_GetMemoryProfile;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (MemoryProfile_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetMemoryProfile.Handle = Handle;
    VID_Ioctl_GetMemoryProfile.MemoryProfile = *MemoryProfile_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETMEMORYPROFILE, &VID_Ioctl_GetMemoryProfile) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetMemoryProfile.ErrorCode;

        /* Parameters from IOCTL structure */
        *MemoryProfile_p = VID_Ioctl_GetMemoryProfile.MemoryProfile;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_GetOutputWindowMode
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetOutputWindowMode(const STVID_ViewPortHandle_t ViewPortHandle, BOOL * const AutoMode_p, STVID_WindowParams_t * const WinParams_p)
{
    VID_Ioctl_GetOutputWindowMode_t      VID_Ioctl_GetOutputWindowMode;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (AutoMode_p == NULL)
        || (WinParams_p == NULL))
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetOutputWindowMode.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETOUTPUTWINDOWMODE, &VID_Ioctl_GetOutputWindowMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetOutputWindowMode.ErrorCode;

        /* Parameters from IOCTL structure */
        *AutoMode_p = VID_Ioctl_GetOutputWindowMode.AutoMode;
        *WinParams_p = VID_Ioctl_GetOutputWindowMode.WinParams;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_GetPictureAllocParams
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetPictureAllocParams(const STVID_Handle_t Handle, const STVID_PictureParams_t * const Params_p, STAVMEM_AllocBlockParams_t * const AllocParams_p)
{
    VID_Ioctl_GetPictureAllocParams_t      VID_Ioctl_GetPictureAllocParams;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (Params_p == NULL)
        || (AllocParams_p == NULL))
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetPictureAllocParams.Handle = Handle;
    VID_Ioctl_GetPictureAllocParams.Params = *Params_p;
    VID_Ioctl_GetPictureAllocParams.AllocParams = *AllocParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETPICTUREALLOCPARAMS, &VID_Ioctl_GetPictureAllocParams) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetPictureAllocParams.ErrorCode;

        /* Parameters from IOCTL structure */
        *AllocParams_p = VID_Ioctl_GetPictureAllocParams.AllocParams;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_GetPictureParams
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetPictureParams(const STVID_Handle_t Handle, const STVID_Picture_t PictureType, STVID_PictureParams_t * const Params_p)
{
    VID_Ioctl_GetPictureParams_t      VID_Ioctl_GetPictureParams;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    APIVideoInstance_t            * VidInstance_p;

    if (Params_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetPictureParams.Handle = Handle;
    VID_Ioctl_GetPictureParams.PictureType = PictureType;
    VID_Ioctl_GetPictureParams.Params = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETPICTUREPARAMS, &VID_Ioctl_GetPictureParams) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetPictureParams.ErrorCode;

        /* Parameters from IOCTL structure */
        *Params_p = VID_Ioctl_GetPictureParams.Params;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Transform Frame buffer addresses from Kernel space to User space */
        STOS_SemaphoreWait(VideoInstanceSemaphore_p);
        if ((VidInstance_p=GetVidInstanceFromHandle(Handle)) != NULL)
        {
            if (!Transform_PictureParams(VidInstance_p, Params_p))
            {
                /* Frame buffer user space address has NOT been found: error case */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }
        STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_GetPictureInfos
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetPictureInfos(const STVID_Handle_t Handle, const STVID_Picture_t PictureType, STVID_PictureInfos_t * const PictureInfos_p)
{
    VID_Ioctl_GetPictureInfos_t      VID_Ioctl_GetPictureInfos;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    APIVideoInstance_t            * VidInstance_p;

    if (PictureInfos_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetPictureInfos.Handle = Handle;
    VID_Ioctl_GetPictureInfos.PictureType = PictureType;
    VID_Ioctl_GetPictureInfos.PictureInfos = *PictureInfos_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETPICTUREINFOS, &VID_Ioctl_GetPictureInfos) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetPictureInfos.ErrorCode;

        /* Parameters from IOCTL structure */
        *PictureInfos_p = VID_Ioctl_GetPictureInfos.PictureInfos;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Transform Frame buffer addresses from Kernel space to User space */
        STOS_SemaphoreWait(VideoInstanceSemaphore_p);
        if ((VidInstance_p=GetVidInstanceFromHandle(Handle)) != NULL)
        {
            if (!Transform_PictureInfos(VidInstance_p, PictureInfos_p))
            {
                /* Frame buffer user space address has NOT been found: error case */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }
        STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetDisplayPictureInfo
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetDisplayPictureInfo(const STVID_Handle_t Handle, const STVID_PictureBufferHandle_t PictureBufferHandle, STVID_DisplayPictureInfos_t * const DisplayPictureInfos_p)
{
    VID_Ioctl_GetDisplayPictureInfo_t      VID_Ioctl_GetDisplayPictureInfo;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if ((DisplayPictureInfos_p == NULL) || (PictureBufferHandle == (STVID_PictureBufferHandle_t)NULL))
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetDisplayPictureInfo.Handle = Handle;
    VID_Ioctl_GetDisplayPictureInfo.PictureBufferHandle = PictureBufferHandle;
    VID_Ioctl_GetDisplayPictureInfo.DisplayPictureInfos = *DisplayPictureInfos_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETDISPLAYPICTUREINFO, &VID_Ioctl_GetDisplayPictureInfo) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetDisplayPictureInfo.ErrorCode;

        /* Parameters from IOCTL structure */
        *DisplayPictureInfos_p = VID_Ioctl_GetDisplayPictureInfo.DisplayPictureInfos;
    }


    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetSpeed
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetSpeed(const STVID_Handle_t Handle, S32 * const Speed_p)
{
    VID_Ioctl_GetSpeed_t      VID_Ioctl_GetSpeed;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Speed_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetSpeed.Handle = Handle;
    VID_Ioctl_GetSpeed.Speed = *Speed_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETSPEED, &VID_Ioctl_GetSpeed) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetSpeed.ErrorCode;

        /* Parameters from IOCTL structure */
        *Speed_p = VID_Ioctl_GetSpeed.Speed;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_GetViewPortAlpha
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortAlpha(const STVID_ViewPortHandle_t ViewPortHandle, STLAYER_GlobalAlpha_t * const GlobalAlpha_p)
{
        VID_Ioctl_GetViewPortAlpha_t      VID_Ioctl_GetViewPortAlpha;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (GlobalAlpha_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetViewPortAlpha.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETVIEWPORTALPHA, &VID_Ioctl_GetViewPortAlpha) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetViewPortAlpha.ErrorCode;

        /* Parameters from IOCTL structure */
        *GlobalAlpha_p = VID_Ioctl_GetViewPortAlpha.GlobalAlpha;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_GetViewPortColorKey
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortColorKey(const STVID_ViewPortHandle_t ViewPortHandle, STGXOBJ_ColorKey_t * const ColorKey_p)
{
        VID_Ioctl_GetViewPortColorKey_t      VID_Ioctl_GetViewPortColorKey;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (ColorKey_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetViewPortColorKey.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETVIEWPORTCOLORKEY, &VID_Ioctl_GetViewPortColorKey) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetViewPortColorKey.ErrorCode;

        /* Parameters from IOCTL structure */
        *ColorKey_p = VID_Ioctl_GetViewPortColorKey.ColorKey;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetViewPortPSI
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortPSI(const STVID_ViewPortHandle_t ViewPortHandle, STLAYER_PSI_t * const VPPSI_p)
{
        VID_Ioctl_GetViewPortPSI_t      VID_Ioctl_GetViewPortPSI;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (VPPSI_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetViewPortPSI.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETVIEWPORTPSI, &VID_Ioctl_GetViewPortPSI) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetViewPortPSI.ErrorCode;

        /* Parameters from IOCTL structure */
        *VPPSI_p = VID_Ioctl_GetViewPortPSI.VPPSI;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_GetViewPortSpecialMode
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortSpecialMode (const STVID_ViewPortHandle_t ViewPortHandle, STLAYER_OutputMode_t * const OuputMode_p, STLAYER_OutputWindowSpecialModeParams_t * const Params_p)
{
        VID_Ioctl_GetViewPortSpecialMode_t      VID_Ioctl_GetViewPortSpecialMode;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (OuputMode_p == NULL)
        || (Params_p == NULL))
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetViewPortSpecialMode.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETVIEWPORTSPECIALMODE, &VID_Ioctl_GetViewPortSpecialMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetViewPortSpecialMode.ErrorCode;

        /* Parameters from IOCTL structure */
        *OuputMode_p = VID_Ioctl_GetViewPortSpecialMode.OuputMode;
        *Params_p = VID_Ioctl_GetViewPortSpecialMode.Params;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_HidePicture
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_HidePicture(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VID_Ioctl_HidePicture_t      VID_Ioctl_HidePicture;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_HidePicture.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_HIDEPICTURE, &VID_Ioctl_HidePicture) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_HidePicture.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_InjectDiscontinuity
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_InjectDiscontinuity(const STVID_Handle_t Handle)
{
    VID_Ioctl_InjectDiscontinuity_t      VID_Ioctl_InjectDiscontinuity;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_InjectDiscontinuity.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_INJECTDISCONTINUITY, &VID_Ioctl_InjectDiscontinuity) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_InjectDiscontinuity.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_OpenViewPort
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_OpenViewPort(const STVID_Handle_t VideoHandle,
                                  const STVID_ViewPortParams_t * const ViewPortParams_p,
                                  STVID_ViewPortHandle_t * const ViewPortHandle_p)
{
    VID_Ioctl_OpenViewPort_t      VID_Ioctl_OpenViewPort;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (ViewPortParams_p == NULL)
        || (ViewPortHandle_p == NULL))
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_OpenViewPort.VideoHandle = VideoHandle;
    VID_Ioctl_OpenViewPort.ViewPortParams = *ViewPortParams_p;
    VID_Ioctl_OpenViewPort.ViewPortHandle = *ViewPortHandle_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_OPENVIEWPORT, &VID_Ioctl_OpenViewPort) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_OpenViewPort.ErrorCode;

        /* Parameters from IOCTL structure */
        *ViewPortHandle_p = VID_Ioctl_OpenViewPort.ViewPortHandle;
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_Pause
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Pause(const STVID_Handle_t Handle, const STVID_Freeze_t * const Freeze_p)
{
    VID_Ioctl_Pause_t      VID_Ioctl_Pause;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Freeze_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_Pause.Handle = Handle;
    VID_Ioctl_Pause.Freeze = *Freeze_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_PAUSE, &VID_Ioctl_Pause) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Pause.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_PauseSynchro
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_PauseSynchro(const STVID_Handle_t Handle, const STTS_t Time)
{
    VID_Ioctl_PauseSynchro_t      VID_Ioctl_PauseSynchro;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_PauseSynchro.Handle = Handle;
    VID_Ioctl_PauseSynchro.Time = Time;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_PAUSESYNCHRO, &VID_Ioctl_PauseSynchro) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_PauseSynchro.ErrorCode;
    }

    return(ErrorCode);
}

#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
/*******************************************************************************
Name        : STVID_GetVideoDisplayParams
Description : This function retreive params used for VHSRC.
Parameters  : VPHandle IN: Handle of the viewport, Params_p OUT: Pointer
              to an allocated VHSRCParams data type
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetVideoDisplayParams(const STVID_ViewPortHandle_t ViewPortHandle,
                                  STLAYER_VideoDisplayParams_t * Params_p)
{
    VID_Ioctl_GetVideoDisplayParams_t VID_Ioctl_GetVideoDisplayParams;
    ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;

    if (Params_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetVideoDisplayParams.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETVIDEODISPLAYPARAMS, &VID_Ioctl_GetVideoDisplayParams) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetVideoDisplayParams.ErrorCode;

        /* Parameters from IOCTL structure */
        *Params_p = VID_Ioctl_GetVideoDisplayParams.Params;
    }

    return(ErrorCode);
}
#endif /* STVID_DEBUG_GET_DISPLAY_PARAMS */

/*******************************************************************************
Name        : STVID_SetViewPortQualityOptimizations
Description : This function set the quality optimizations used in viewport.
Parameters  : VPHandle IN: Handle of the viewport, Params_p IN: Pointer
              to an allocated STLAYER_QualityOptimizations_t data type
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortQualityOptimizations(const STVID_ViewPortHandle_t ViewPortHandle,
                                                     const STLAYER_QualityOptimizations_t * Params_p)
{
    VID_Ioctl_SetViewPortQualityOptimizations_t   VID_Ioctl_SetViewPortQualityOptimizations;
    ST_ErrorCode_t                              ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetViewPortQualityOptimizations.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetViewPortQualityOptimizations.Optimizations= *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETVIEWPORTQUALITYOPTIMIZATIONS, &VID_Ioctl_SetViewPortQualityOptimizations) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetViewPortQualityOptimizations.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_SetViewPortQualityOptimizations
Description : This function retreive the quality optimizations used in viewport.
Parameters  : VPHandle IN: Handle of the viewport, Params_p OUT: Pointer
              to an allocated STLAYER_QualityOptimizations_t data type
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortQualityOptimizations(const STVID_ViewPortHandle_t ViewPortHandle,
                                                     STLAYER_QualityOptimizations_t * const Params_p)
{
    VID_Ioctl_GetViewPortQualityOptimizations_t VID_Ioctl_GetViewPortQualityOptimizations;
    ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;

    if (Params_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetViewPortQualityOptimizations.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETVIEWPORTQUALITYOPTIMIZATIONS, &VID_Ioctl_GetViewPortQualityOptimizations) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetViewPortQualityOptimizations.ErrorCode;

        /* Parameters from IOCTL structure */
        *Params_p = VID_Ioctl_GetViewPortQualityOptimizations.Optimizations;
    }

    return(ErrorCode);
}

#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
/*******************************************************************************
Name        : STVID_GetRequestedDeinterlacingMode
Description : This function can be used to know the currently requested Deinterlacing mode.
Parameters  : The Requested mode can be one of the following values:
              0: Off (nothing displayed)
              1: Reserved for future use
              2: Bypass
              3: Vertical Intperolation
              4: Directional Intperolation
              5: Field Merging
              6: Median Filter
              7: MLD Mode (=3D Mode) = Default value
              8: LMU Mode
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetRequestedDeinterlacingMode(const STVID_ViewPortHandle_t ViewPortHandle,
        STLAYER_DeiMode_t * const RequestedMode_p)
{
    VID_Ioctl_GetRequestedDeinterlacingMode_t      VID_Ioctl_GetRequestedDeinterlacingMode;
    ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;

    if (RequestedMode_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetRequestedDeinterlacingMode.ViewPortHandle = ViewPortHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETREQUESTEDDEINTERLACINGMODE, &VID_Ioctl_GetRequestedDeinterlacingMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetRequestedDeinterlacingMode.ErrorCode;

        /* Parameters from IOCTL structure */
        *RequestedMode_p = VID_Ioctl_GetRequestedDeinterlacingMode.RequestedMode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_SetRequestedDeinterlacingMode
Description : This function can be used to request a Deinterlacing mode.

              This function is provided FOR TEST PURPOSE ONLY. By default the requested
              DEI mode should not be changed.

Parameters  : The Requested mode can be one of the following values:
              0: Off (nothing displayed)
              1: Reserved for future use
              2: Bypass
              3: Vertical Intperolation
              4: Directional Intperolation
              5: Field Merging
              6: Median Filter
              7: MLD Mode (=3D Mode) = Default value
              8: LMU Mode
Assumptions :
Limitations : The requested mode will be applied only if the necessary conditions are met
              (for example the availability of the Previous and Current fields)
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetRequestedDeinterlacingMode(const STVID_ViewPortHandle_t ViewPortHandle,
        const STLAYER_DeiMode_t RequestedMode)
{
    VID_Ioctl_SetRequestedDeinterlacingMode_t   VID_Ioctl_SetRequestedDeinterlacingMode;
    ST_ErrorCode_t                              ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetRequestedDeinterlacingMode.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetRequestedDeinterlacingMode.RequestedMode = RequestedMode;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETREQUESTEDDEINTERLACINGMODE, &VID_Ioctl_SetRequestedDeinterlacingMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetRequestedDeinterlacingMode.ErrorCode;
    }

    return(ErrorCode);
}
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */

/*******************************************************************************
Name        : STVID_Resume
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Resume(const STVID_Handle_t Handle)
{
    VID_Ioctl_Resume_t      VID_Ioctl_Resume;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_Resume.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_RESUME, &VID_Ioctl_Resume) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Resume.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_SetDataInputInterface
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
/* This function called by user autorizes video to call external functions */
ST_ErrorCode_t STVID_SetDataInputInterface(const STVID_Handle_t Handle,
     ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
     void (*InformReadAddress)(void * const Handle, void * const Address),
     void * const FunctionsHandle)
{
    VID_Ioctl_SetDataInputInterface_t      VID_Ioctl_SetDataInputInterface;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VidCallbackMap_t                  * SubstituteBufferHandle = NULL;
    APIVideoInstance_t                * VidInstance_p;
   
    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (   (GetWriteAddress == NULL)
        || (InformReadAddress == NULL))
    {
#ifdef STVID_DIRECT_INTERFACE_PTI
        if (FunctionsHandle == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): NULL Handle error", __FUNCTION__));
            return ST_ERROR_BAD_PARAMETER;
        }

        /* PTI - both should be NULL */
        GetWriteAddress = NULL;
        InformReadAddress = NULL;

        /* Copy function handle into IOCTL structure */
    VID_Ioctl_SetDataInputInterface.FunctionsHandle = FunctionsHandle;

#else

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s(): NULL functions error", __FUNCTION__));
        return ST_ERROR_BAD_PARAMETER;
#endif
    }
    else
    {
        /* Add the callbacks to the lookup table */
        /* Overwrite previous if present */


        /* Reinit Callbacks of current instance */
        STOS_SemaphoreWait(VideoInstanceSemaphore_p);
        if ((VidInstance_p=GetVidInstanceFromHandle(Handle)) != NULL)
        {
            VidInstance_p->CallbackMap.GetWriteAddress_p     = GetWriteAddress;
            VidInstance_p->CallbackMap.InformReadAddress_p   = InformReadAddress;
            /* Update buffer handle, otherwise instability under linux */
            VidInstance_p->CallbackMap.BufferHandle          = FunctionsHandle;

            SubstituteBufferHandle = &VidInstance_p->CallbackMap;
        }
        STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
        
        if (SubstituteBufferHandle == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
            return ST_ERROR_NO_MEMORY;
        }
        
        /* Copy function handle into IOCTL structure */
        VID_Ioctl_SetDataInputInterface.FunctionsHandle = (void*)SubstituteBufferHandle;
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetDataInputInterface.Handle = Handle;
    VID_Ioctl_SetDataInputInterface.GetWriteAddress = GetWriteAddress;
    VID_Ioctl_SetDataInputInterface.InformReadAddress = InformReadAddress;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETDATAINPUTINTERFACE, &VID_Ioctl_SetDataInputInterface) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetDataInputInterface.ErrorCode;
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_SetDecodedPictures
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetDecodedPictures(const STVID_Handle_t Handle, const STVID_DecodedPictures_t DecodedPictures)
{
    VID_Ioctl_SetDecodedPictures_t      VID_Ioctl_SetDecodedPictures;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetDecodedPictures.Handle = Handle;
    VID_Ioctl_SetDecodedPictures.DecodedPictures = DecodedPictures;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETDECODEDPICTURES, &VID_Ioctl_SetDecodedPictures) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetDecodedPictures.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_SetDisplayAspectRatioConversion
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetDisplayAspectRatioConversion(const STVID_ViewPortHandle_t ViewPortHandle, const STVID_DisplayAspectRatioConversion_t Mode)
{
    VID_Ioctl_SetDisplayAspectRatioConversion_t      VID_Ioctl_SetDisplayAspectRatioConversion;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetDisplayAspectRatioConversion.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetDisplayAspectRatioConversion.Mode = Mode;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETDISPLAYASPECTRATIOCONVERSION, &VID_Ioctl_SetDisplayAspectRatioConversion) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetDisplayAspectRatioConversion.ErrorCode;
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : STVID_SetErrorRecoveryMode
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetErrorRecoveryMode(const STVID_Handle_t Handle, const STVID_ErrorRecoveryMode_t Mode)
{
    VID_Ioctl_SetErrorRecoveryMode_t      VID_Ioctl_SetErrorRecoveryMode;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetErrorRecoveryMode.Handle = Handle;
    VID_Ioctl_SetErrorRecoveryMode.Mode = Mode;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETERRORRECOVERYMODE, &VID_Ioctl_SetErrorRecoveryMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetErrorRecoveryMode.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_SetHDPIPParams
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetHDPIPParams(const STVID_Handle_t Handle, const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    VID_Ioctl_SetHDPIPParams_t      VID_Ioctl_SetHDPIPParams;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (HDPIPParams_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetHDPIPParams.Handle = Handle;
    VID_Ioctl_SetHDPIPParams.HDPIPParams = *HDPIPParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETHDPIPPARAMS, &VID_Ioctl_SetHDPIPParams) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetHDPIPParams.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_SetInputWindowMode
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetInputWindowMode(const STVID_ViewPortHandle_t ViewPortHandle, const BOOL AutoMode, const STVID_WindowParams_t * const WinParams_p)
{
    VID_Ioctl_SetInputWindowMode_t      VID_Ioctl_SetInputWindowMode;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (WinParams_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetInputWindowMode.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetInputWindowMode.AutoMode = AutoMode;
    VID_Ioctl_SetInputWindowMode.WinParams = *WinParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETINPUTWINDOWMODE, &VID_Ioctl_SetInputWindowMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetInputWindowMode.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_SetIOWindows
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetIOWindows(const STVID_ViewPortHandle_t ViewPortHandle,
                    const S32 InputWinX,  const S32 InputWinY,  const U32 InputWinWidth,  const U32 InputWinHeight,
                    const S32 OutputWinX, const S32 OutputWinY, const U32 OutputWinWidth, const U32 OutputWinHeight)
{
    VID_Ioctl_SetIOWindows_t      VID_Ioctl_SetIOWindows;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetIOWindows.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetIOWindows.InputWinX = InputWinX;
    VID_Ioctl_SetIOWindows.InputWinY = InputWinY;
    VID_Ioctl_SetIOWindows.InputWinWidth = InputWinWidth;
    VID_Ioctl_SetIOWindows.InputWinHeight = InputWinHeight;
    VID_Ioctl_SetIOWindows.OutputWinX = OutputWinX;
    VID_Ioctl_SetIOWindows.OutputWinY = OutputWinY;
    VID_Ioctl_SetIOWindows.OutputWinWidth = OutputWinWidth;
    VID_Ioctl_SetIOWindows.OutputWinHeight = OutputWinHeight;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETIOWINDOWS, &VID_Ioctl_SetIOWindows) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetIOWindows.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_SetMemoryProfile
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetMemoryProfile(const STVID_Handle_t Handle, const STVID_MemoryProfile_t * const MemoryProfile_p)
{
    VID_Ioctl_SetMemoryProfile_t      VID_Ioctl_SetMemoryProfile;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (MemoryProfile_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetMemoryProfile.Handle = Handle;
    VID_Ioctl_SetMemoryProfile.MemoryProfile = *MemoryProfile_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETMEMORYPROFILE, &VID_Ioctl_SetMemoryProfile) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetMemoryProfile.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_SetOutputWindowMode
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetOutputWindowMode(const STVID_ViewPortHandle_t ViewPortHandle, const BOOL AutoMode, const STVID_WindowParams_t * const WinParams_p)
{
    VID_Ioctl_SetOutputWindowMode_t      VID_Ioctl_SetOutputWindowMode;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (WinParams_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetOutputWindowMode.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetOutputWindowMode.AutoMode = AutoMode;
    VID_Ioctl_SetOutputWindowMode.WinParams = *WinParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETOUTPUTWINDOWMODE, &VID_Ioctl_SetOutputWindowMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetOutputWindowMode.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_SetSpeed
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetSpeed(const STVID_Handle_t Handle, const S32 Speed)
{
    VID_Ioctl_SetSpeed_t      VID_Ioctl_SetSpeed;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetSpeed.Handle = Handle;
    VID_Ioctl_SetSpeed.Speed = Speed;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETSPEED, &VID_Ioctl_SetSpeed) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetSpeed.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_Setup
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Setup(const STVID_Handle_t Handle, const STVID_SetupParams_t * const SetupParams_p)
{
#ifdef STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
    VID_Ioctl_GetDataInputBufferParams_t    VID_Ioctl_GetDataInputBufferParams;
    APIVideoInstance_t                    * VidInstance_p;
#endif
    VID_Ioctl_Setup_t                       VID_Ioctl_Setup;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    if (SetupParams_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_Setup.Handle = Handle;
    VID_Ioctl_Setup.SetupParams = *SetupParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETUP, &VID_Ioctl_Setup) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Setup.ErrorCode;
    }

#ifdef STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
    if (ErrorCode == ST_NO_ERROR)
    {
        /* PES buffer size has been modified, remmap buffer ... */
        if (SetupParams_p->SetupObject == STVID_SETUP_DATA_INPUT_BUFFER_PARTITION)
        {
            STOS_SemaphoreWait(VideoInstanceSemaphore_p);
            if ((VidInstance_p=GetVidInstanceFromHandle(Handle)) != (APIVideoInstance_t *)NULL)
            {
                if (VidInstance_p->CallbackMap.size != 0)   /* Remmap only if already mmapped ... */
                {
                    /* Unmap the PES buffers */
                    UnmapPESBufferFromUserSpace(&VidInstance_p->CallbackMap);

                    /* Copy parameters into IOCTL structure */
                    VID_Ioctl_GetDataInputBufferParams.Handle = Handle;
                
                    /* IOCTL the command */
                    if (ioctl(fd, STVID_IOC_GETDATAINPUTBUFFERPARAMS, &VID_Ioctl_GetDataInputBufferParams) != 0)
                    {
                        /* IOCTL failed */
                        perror(strerror(errno));
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
                    }
                    else
                    {
                        /* IOCTL is successfull: retrieve Error code */
                        ErrorCode = VID_Ioctl_GetDataInputBufferParams.ErrorCode;
                    }

                    if (ErrorCode == ST_NO_ERROR)
                    {
                        /* Map PES buffer address to user space in order to be used by application */
                        ErrorCode = MapPESBufferToUserSpace(&VidInstance_p->CallbackMap,
                                                            &VID_Ioctl_GetDataInputBufferParams.BaseAddress_p,
                                                            VID_Ioctl_GetDataInputBufferParams.Size);
                    }
                }
            }
            STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
        }
    }
#endif

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_SetViewPortAlpha
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortAlpha(const STVID_ViewPortHandle_t ViewPortHandle, const STLAYER_GlobalAlpha_t * const GlobalAlpha_p)
{
    VID_Ioctl_SetViewPortAlpha_t      VID_Ioctl_SetViewPortAlpha;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (GlobalAlpha_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetViewPortAlpha.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetViewPortAlpha.GlobalAlpha = *GlobalAlpha_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETVIEWPORTALPHA, &VID_Ioctl_SetViewPortAlpha) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetViewPortAlpha.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_SetViewPortColorKey
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortColorKey(const STVID_ViewPortHandle_t ViewPortHandle, const STGXOBJ_ColorKey_t * const ColorKey_p)
{
    VID_Ioctl_SetViewPortColorKey_t      VID_Ioctl_SetViewPortColorKey;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (ColorKey_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetViewPortColorKey.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetViewPortColorKey.ColorKey = *ColorKey_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETVIEWPORTCOLORKEY, &VID_Ioctl_SetViewPortColorKey) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetViewPortColorKey.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_SetViewPortPSI
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortPSI(const STVID_ViewPortHandle_t ViewPortHandle, const STLAYER_PSI_t * const VPPSI_p)
{
    VID_Ioctl_SetViewPortPSI_t      VID_Ioctl_SetViewPortPSI;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (VPPSI_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetViewPortPSI.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetViewPortPSI.VPPSI = *VPPSI_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETVIEWPORTPSI, &VID_Ioctl_SetViewPortPSI) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetViewPortPSI.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_SetViewPortSpecialMode
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortSpecialMode (const STVID_ViewPortHandle_t ViewPortHandle, const STLAYER_OutputMode_t OuputMode, const STLAYER_OutputWindowSpecialModeParams_t * const Params_p)
{
    VID_Ioctl_SetViewPortSpecialMode_t      VID_Ioctl_SetViewPortSpecialMode;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Params_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetViewPortSpecialMode.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_SetViewPortSpecialMode.OuputMode = OuputMode;
    VID_Ioctl_SetViewPortSpecialMode.Params = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETVIEWPORTSPECIALMODE, &VID_Ioctl_SetViewPortSpecialMode) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetViewPortSpecialMode.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_ShowPicture
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_ShowPicture(const STVID_ViewPortHandle_t ViewPortHandle, STVID_PictureParams_t * const Params_p, const STVID_Freeze_t * const Freeze_p)
{
    VID_Ioctl_ShowPicture_t      VID_Ioctl_ShowPicture;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (Params_p == NULL)
        || (Freeze_p == NULL))
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_ShowPicture.ViewPortHandle = ViewPortHandle;
    VID_Ioctl_ShowPicture.Params = *Params_p;
    VID_Ioctl_ShowPicture.Freeze = *Freeze_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SHOWPICTURE, &VID_Ioctl_ShowPicture) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_ShowPicture.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_SkipSynchro
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_SkipSynchro(const STVID_Handle_t Handle, const STTS_t Time)
{
    VID_Ioctl_SkipSynchro_t      VID_Ioctl_SkipSynchro;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SkipSynchro.Handle = Handle;
    VID_Ioctl_SkipSynchro.Time = Time;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SKIPSYNCHRO, &VID_Ioctl_SkipSynchro) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SkipSynchro.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_Start
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Start(const STVID_Handle_t Handle, const STVID_StartParams_t * const Params_p)
{
    VID_Ioctl_Start_t      VID_Ioctl_Start;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Params_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_Start.Handle = Handle;
    VID_Ioctl_Start.Params = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_START, &VID_Ioctl_Start) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Start.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_StartUpdatingDisplay
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_StartUpdatingDisplay(const STVID_Handle_t Handle)
{
    VID_Ioctl_StartUpdatingDisplay_t      VID_Ioctl_StartUpdatingDisplay;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_StartUpdatingDisplay.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_STARTUPDATINGDISPLAY, &VID_Ioctl_StartUpdatingDisplay) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_StartUpdatingDisplay.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_Step
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Step(const STVID_Handle_t Handle)
{
    VID_Ioctl_Step_t      VID_Ioctl_Step;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_Step.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_STEP, &VID_Ioctl_Step) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Step.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_Stop
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_Stop(const STVID_Handle_t Handle, const STVID_Stop_t StopMode, const STVID_Freeze_t * const Freeze_p)
{
    VID_Ioctl_Stop_t      VID_Ioctl_Stop;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Freeze_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_Stop.Handle = Handle;
    VID_Ioctl_Stop.StopMode = StopMode;
    VID_Ioctl_Stop.Freeze = *Freeze_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_STOP, &VID_Ioctl_Stop) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_Stop.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_EnableDeblocking
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_EnableDeblocking(const STVID_Handle_t Handle)
{
    VID_Ioctl_EnableDeblocking_t      VID_Ioctl_EnableDeblocking;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;


    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_EnableDeblocking.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_ENABLEDEBLOCKING, &VID_Ioctl_EnableDeblocking) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_EnableDeblocking.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_DisableDeblocking
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DisableDeblocking(const STVID_Handle_t Handle)
{
    VID_Ioctl_DisableDeblocking_t      VID_Ioctl_DisableDeblocking;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DisableDeblocking.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DISABLEDEBLOCKING, &VID_Ioctl_DisableDeblocking) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DisableDeblocking.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_EnableDisplay
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_EnableDisplay(const STVID_Handle_t Handle)
{
    VID_Ioctl_EnableDisplay_t      VID_Ioctl_EnableDisplay;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_EnableDisplay.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_ENABLEDISPLAY, &VID_Ioctl_EnableDisplay) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_EnableDisplay.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_DisableDisplay
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DisableDisplay(const STVID_Handle_t Handle)
{
    VID_Ioctl_DisableDisplay_t      VID_Ioctl_DisableDisplay;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DisableDisplay.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DISABLEDISPLAY, &VID_Ioctl_DisableDisplay) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DisableDisplay.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_GetSynchronizationDelay
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
ST_ErrorCode_t STVID_GetSynchronizationDelay(const STVID_Handle_t VideoHandle, S32 * const SyncDelay_p)
{
    VID_Ioctl_GetSynchronizationDelay_t      VID_Ioctl_GetSynchronizationDelay;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (SyncDelay_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetSynchronizationDelay.VideoHandle = VideoHandle;
    VID_Ioctl_GetSynchronizationDelay.SyncDelay = *SyncDelay_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETSYNCHRONIZATIONDELAY, &VID_Ioctl_GetSynchronizationDelay) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetSynchronizationDelay.ErrorCode;

        /* Parameters from IOCTL structure */
        *SyncDelay_p = VID_Ioctl_GetSynchronizationDelay.SyncDelay;
    }

    return(ErrorCode);
}
#endif

/*******************************************************************************
Name        : STVID_SetSynchronizationDelay
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
ST_ErrorCode_t STVID_SetSynchronizationDelay(const STVID_Handle_t VideoHandle, const S32 SyncDelay)
{
    VID_Ioctl_SetSynchronizationDelay_t      VID_Ioctl_SetSynchronizationDelay;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_SetSynchronizationDelay.VideoHandle = VideoHandle;
    VID_Ioctl_SetSynchronizationDelay.SyncDelay = SyncDelay;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SETSYNCHRONIZATIONDELAY, &VID_Ioctl_SetSynchronizationDelay) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_SetSynchronizationDelay.ErrorCode;
    }

    return(ErrorCode);
}
#endif /* STVID_ENABLE_SYNCHRONIZATION_DELAY */

/*******************************************************************************
Name        : STVID_GetPictureBuffer
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_GetPictureBuffer(const STVID_Handle_t Handle, const STVID_GetPictureBufferParams_t * const Params_p,
                                      STVID_PictureBufferDataParams_t * const PictureBufferParams_p,
                                      STVID_PictureBufferHandle_t * const PictureBufferHandle_p)
{
    VID_Ioctl_GetPictureBuffer_t      VID_Ioctl_GetPictureBuffer;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (   (PictureBufferParams_p == NULL)
        || (PictureBufferHandle_p == NULL))
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_GetPictureBuffer.Handle = Handle;
    VID_Ioctl_GetPictureBuffer.Params = *Params_p;
    VID_Ioctl_GetPictureBuffer.PictureBufferParams = *PictureBufferParams_p;
    VID_Ioctl_GetPictureBuffer.PictureBufferHandle = *PictureBufferHandle_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_GETPICTUREBUFFER, &VID_Ioctl_GetPictureBuffer) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_GetPictureBuffer.ErrorCode;

        /* Parameters from IOCTL structure */
        *PictureBufferParams_p = VID_Ioctl_GetPictureBuffer.PictureBufferParams;
        *PictureBufferHandle_p = VID_Ioctl_GetPictureBuffer.PictureBufferHandle;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_ReleasePictureBuffer
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_ReleasePictureBuffer(const STVID_Handle_t Handle, const STVID_PictureBufferHandle_t  PictureBufferHandle)
{
    VID_Ioctl_ReleasePictureBuffer_t      VID_Ioctl_ReleasePictureBuffer;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_ReleasePictureBuffer.Handle = Handle;
    VID_Ioctl_ReleasePictureBuffer.PictureBufferHandle = PictureBufferHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_RELEASEPICTUREBUFFER, &VID_Ioctl_ReleasePictureBuffer) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_ReleasePictureBuffer.ErrorCode;
    }

    return(ErrorCode);
}
/*******************************************************************************
Name        : STVID_DisplayPictureBuffer
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DisplayPictureBuffer(const STVID_Handle_t Handle, const STVID_PictureBufferHandle_t  PictureBufferHandle,
                                          const STVID_PictureInfos_t*  const PictureInfos_p)
{
    VID_Ioctl_DisplayPictureBuffer_t      VID_Ioctl_DisplayPictureBuffer;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (PictureInfos_p == NULL)
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_DisplayPictureBuffer.Handle = Handle;
    VID_Ioctl_DisplayPictureBuffer.PictureBufferHandle = PictureBufferHandle;
    VID_Ioctl_DisplayPictureBuffer.PictureInfos = *PictureInfos_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DISPLAYPICTUREBUFFER, &VID_Ioctl_DisplayPictureBuffer) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_DisplayPictureBuffer.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_TakePictureBuffer
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_TakePictureBuffer(const STVID_Handle_t Handle, const STVID_PictureBufferHandle_t PictureBufferHandle)
{
    VID_Ioctl_TakePictureBuffer_t      VID_Ioctl_TakePictureBuffer;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_TakePictureBuffer.Handle = Handle;
    VID_Ioctl_TakePictureBuffer.PictureBufferHandle = PictureBufferHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_TAKEPICTUREBUFFER, &VID_Ioctl_TakePictureBuffer) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_TakePictureBuffer.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : StubInject_SetStreamSize
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
#ifdef STUB_INJECT
ST_ErrorCode_t StubInject_SetStreamSize(U32 Size)
{
    VID_Ioctl_StreamSize_t  VID_Ioctl_StreamSize;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_StreamSize.Size = Size;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_VID_STREAMSIZE, &VID_Ioctl_StreamSize) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = ST_NO_ERROR;
    }

    return(ErrorCode);
}
#endif  /* STUB_INJECT */

/*******************************************************************************
Name        : StubInject_GetBBInfo
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
#ifdef STUB_INJECT
ST_ErrorCode_t StubInject_GetBBInfo(void ** BaseAddress_p, U32 * Size_p)
{
    VID_Ioctl_StubInjectGetBBInfo_t  VID_Ioctl_StubInjectGetBBInfo;
    ST_ErrorCode_t                   ErrorCode = ST_NO_ERROR;

    if (   (BaseAddress_p == NULL)
        || (Size_p == NULL))
    {
        /* Check parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Copy parameters into IOCTL structure */
    /* None */

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_STUBINJ_GETBBINFO, &VID_Ioctl_StubInjectGetBBInfo) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = ST_NO_ERROR;

        /* Parameters from IOCTL structure */
        *BaseAddress_p = VID_Ioctl_StubInjectGetBBInfo.BaseAddress_p;
        *Size_p = VID_Ioctl_StubInjectGetBBInfo.Size;
    }

    return(ErrorCode);
}
#endif  /* STUB_INJECT */

#ifdef ST_XVP_ENABLE_FLEXVP
/*******************************************************************************
Name        : STVID_ActivateXVPProcess
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_ActivateXVPProcess(    const STVID_ViewPortHandle_t ViewPortHandle,
                                            const STLAYER_ProcessId_t ProcessID)
{
    VID_Ioctl_XVP_t     VID_Ioctl_XVP;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_XVP.ViewPortHandle    = ViewPortHandle;
    VID_Ioctl_XVP.ProcessID         = ProcessID;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_ACTIXVPPROC, &VID_Ioctl_XVP) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_XVP.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_DeactivateXVPProcess
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_DeactivateXVPProcess(  const STVID_ViewPortHandle_t ViewPortHandle,
                                            const STLAYER_ProcessId_t ProcessID)
{
    VID_Ioctl_XVP_t     VID_Ioctl_XVP;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_XVP.ViewPortHandle    = ViewPortHandle;
    VID_Ioctl_XVP.ProcessID         = ProcessID;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_DEACXVPPROC, &VID_Ioctl_XVP) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_XVP.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_ShowXVPProcess
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_ShowXVPProcess(    const STVID_ViewPortHandle_t ViewPortHandle,
                                        const STLAYER_ProcessId_t ProcessID)
{
    VID_Ioctl_XVP_t     VID_Ioctl_XVP;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_XVP.ViewPortHandle    = ViewPortHandle;
    VID_Ioctl_XVP.ProcessID         = ProcessID;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_SHOWXVPPROC, &VID_Ioctl_XVP) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_XVP.ErrorCode;
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_HideXVPProcess
Description : To be filled ....
Parameters  : To be filled ....
Assumptions : To be filled ....
Limitations : To be filled ....
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_HideXVPProcess(    const STVID_ViewPortHandle_t ViewPortHandle,
                                        const STLAYER_ProcessId_t ProcessID)
{
    VID_Ioctl_XVP_t     VID_Ioctl_XVP;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_XVP.ViewPortHandle    = ViewPortHandle;
    VID_Ioctl_XVP.ProcessID         = ProcessID;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_HIDEXVPPROC, &VID_Ioctl_XVP) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_XVP.ErrorCode;
    }

    return(ErrorCode);
}
#endif /* ST_XVP_ENABLE_FLEXVP */

#ifdef STVID_USE_CRC
/*******************************************************************************
Name        : STVID_CRCStartCheck
Description : Start the CRC checking according to parameters. Launch CRC check
              and compare values to those in specified reference file
Parameters  :
Assumptions :
Limitations :
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_CRCStartCheck(const STVID_Handle_t VideoHandle, STVID_CRCStartParams_t *StartParams_p)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VID_Ioctl_CRCStartCheck_t VID_Ioctl_CRCStartCheck;

    if (StartParams_p == NULL)
    {
        /* Check parameter */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_CRCStartCheck.Handle = VideoHandle;
    VID_Ioctl_CRCStartCheck.StartParams = *StartParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CRCSTARTCHECK, &VID_Ioctl_CRCStartCheck) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_CRCStartCheck.ErrorCode;
    }

    return(ErrorCode);
} /* End of STVID_CRCStartCheck() function */

/*******************************************************************************
Name        : STVID_CRCStopCheck
Description : Stop the CRC checking.
Parameters  :
Assumptions :
Limitations :
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_CRCStopCheck(const STVID_Handle_t VideoHandle)
{

    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VID_Ioctl_CRCStopCheck_t VID_Ioctl_CRCStopCheck;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_CRCStopCheck.Handle = VideoHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CRCSTOPCHECK, &VID_Ioctl_CRCStopCheck) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_CRCStopCheck.ErrorCode;
    }

    return(ErrorCode);
} /* End of STVID_CRCStopCheck() function */


/*******************************************************************************
Name        : STVID_CRCGetCurrentResults
Description : Get the current results of CRC checking.
Parameters  :
Assumptions :
Limitations :
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_CRCGetCurrentResults(const STVID_Handle_t VideoHandle, STVID_CRCCheckResult_t *CRCCheckResult_p)
{

    VID_Ioctl_CRCGetCurrentResults_t VID_Ioctl_CRCGetCurrentResults;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (CRCCheckResult_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_CRCGetCurrentResults.Handle = VideoHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CRCGETCURRENTRESULTS, &VID_Ioctl_CRCGetCurrentResults) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_CRCGetCurrentResults.ErrorCode;

        /* Parameters from IOCTL structure */
        *CRCCheckResult_p = VID_Ioctl_CRCGetCurrentResults.CRCCheckResult;
    }

    return(ErrorCode);
} /* End of STVID_CRCGetCurrentResults() function */

/*******************************************************************************
Name        : STVID_CRCStartQueueing
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_CRCStartQueueing(const STVID_Handle_t VideoHandle)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VID_Ioctl_CRCStartQueueing_t VID_Ioctl_CRCStartQueueing;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_CRCStartQueueing.Handle = VideoHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CRCSTARTQUEUEING, &VID_Ioctl_CRCStartQueueing) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_CRCStartQueueing.ErrorCode;
    }

    return(ErrorCode);
} /* End of STVID_CRCStartQueueing() function */

/*******************************************************************************
Name        : STVID_CRCStopQueueing
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_CRCStopQueueing(const STVID_Handle_t VideoHandle)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VID_Ioctl_CRCStopQueueing_t VID_Ioctl_CRCStopQueueing;

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_CRCStopQueueing.Handle = VideoHandle;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CRCSTOPQUEUEING, &VID_Ioctl_CRCStopQueueing) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_CRCStopQueueing.ErrorCode;
    }

    return(ErrorCode);
} /* End of STVID_CRCStopQueueing() function */

/*******************************************************************************
Name        : STVID_CRCGetQueue
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_CRCGetQueue(const STVID_Handle_t VideoHandle, STVID_CRCReadMessages_t *ReadCRC_p)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VID_Ioctl_CRCGetQueue_t VID_Ioctl_CRCGetQueue;

    if (ReadCRC_p == NULL)
    {
        /* Check parameterso */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!UseCount)
    {
        /* Check if device has been opened and return error if not so */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Copy parameters into IOCTL structure */
    VID_Ioctl_CRCGetQueue.Handle = VideoHandle;
    VID_Ioctl_CRCGetQueue.CRCReadMessages = *ReadCRC_p;

    /* IOCTL the command */
    if (ioctl(fd, STVID_IOC_CRCGETQUEUE, &VID_Ioctl_CRCGetQueue) != 0)
    {
        /* IOCTL failed */
        perror(strerror(errno));
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s():Ioctl error !", __FUNCTION__));
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = VID_Ioctl_CRCGetQueue.ErrorCode;
    }

    return(ErrorCode);
} /* End of STVID_CRCGetQueue() function */


#endif /* STVID_USE_CRC */

/*******************************************************************************
Name        : STVID_PreprocessEvent
Description : This function preprocess events data in order to be compliant with user space
Parameters  : devicename, event, data
Assumptions :
Limitations :
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STVID_PreprocessEvent(const ST_DeviceName_t DeviceName, STEVT_EventConstant_t Event, void * EventData)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    FrameBuffers_t    * FrameBuffers_p = NULL;

    switch (Event)
    {
        case STVID_DISPLAY_NEW_FRAME_EVT:
        case STVID_FRAME_RATE_CHANGE_EVT:
        case STVID_PICTURE_DECODING_ERROR_EVT:
        case STVID_RESOLUTION_CHANGE_EVT:
        case STVID_SCAN_TYPE_CHANGE_EVT:
        case STVID_STOPPED_EVT:
            if (EventData == NULL)
            {
                /* Check parameters */
                return(ST_ERROR_BAD_PARAMETER);
            }

            STOS_SemaphoreWait(VideoInstanceSemaphore_p);
            if ((FrameBuffers_p=GetVideoFrameBuffer((void *)DeviceName, ((STVID_PictureParams_t *)EventData)->Data)) != NULL)
            {
                ((STVID_PictureParams_t *)EventData)->Data = FrameBuffers_p->MappedAddress_p;
            }
            else
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
            break;

        case STVID_USER_DATA_EVT:
            if (EventData == NULL)
            {
                /* Check parameters */
                return(ST_ERROR_BAD_PARAMETER);
            }

            if (((STVID_UserData_t *)EventData)->Buff_p != NULL)
            {
                U32     StructAlignedSize = ((sizeof(STVID_UserData_t) + 3)/4)*4;     /* Flattened data are aligned on U32 (cf vid_ctcm.c) */

                /* Data have been flattenned after structure content */
                ((STVID_UserData_t *)EventData)->Buff_p = (void *)((U32)EventData + StructAlignedSize);    /* Point on flattened data */
            }
            break;

        case VIDBUFF_NEW_ALLOCATED_FRAMEBUFFERS_EVT:
        case VIDBUFF_DEALLOCATED_FRAMEBUFFERS_EVT:
            if (EventData == NULL)
            {
                /* Check parameters */
                return(ST_ERROR_BAD_PARAMETER);
            }

            if (((VIDBUFF_FrameBufferList_t *)EventData)->FrameBuffers_p != NULL)
            {
                U32     StructAlignedSize = ((sizeof(VIDBUFF_FrameBufferList_t) + 3)/4)*4;     /* Flattened data are aligned on U32 (cf vid_buff.c) */

                /* Data have been flattenned after structure content */
                ((VIDBUFF_FrameBufferList_t *)EventData)->FrameBuffers_p = (void *)((U32)EventData + StructAlignedSize);    /* Point on flattened data */
            }
            break;

        case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
        case STVID_NEW_PICTURE_DECODED_EVT:
        case STVID_NEW_PICTURE_ORDERED_EVT:
            if (EventData == NULL)
            {
                /* Check parameters */
                return(ST_ERROR_BAD_PARAMETER);
            }

            STOS_SemaphoreWait(VideoInstanceSemaphore_p);
            if (   (((STVID_PictureInfos_t *)EventData)->VideoParams.DecimationFactors == STVID_DECIMATION_FACTOR_NONE)
                || (((STVID_PictureInfos_t *)EventData)->VideoParams.DoesNonDecimatedExist))
            {
                if ((FrameBuffers_p=GetVideoFrameBuffer((void *)DeviceName, ((STVID_PictureInfos_t *)EventData)->BitmapParams.Data1_p)) != NULL)
                {
                    U32  Offset = ((STVID_PictureInfos_t *)EventData)->BitmapParams.Data2_p - ((STVID_PictureInfos_t *)EventData)->BitmapParams.Data1_p;

                    ((STVID_PictureInfos_t *)EventData)->BitmapParams.Data1_p = FrameBuffers_p->MappedAddress_p;
                    ((STVID_PictureInfos_t *)EventData)->BitmapParams.Data2_p = (void *)((U32)FrameBuffers_p->MappedAddress_p + Offset);
                }
                else
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }

            if (((STVID_PictureInfos_t *)EventData)->VideoParams.DecimationFactors != STVID_DECIMATION_FACTOR_NONE)
            {
                if ((FrameBuffers_p=GetVideoFrameBuffer((void *)DeviceName, ((STVID_PictureInfos_t *)EventData)->VideoParams.DecimatedBitmapParams.Data1_p)) != NULL)
                {
                    U32  Offset = ((STVID_PictureInfos_t *)EventData)->VideoParams.DecimatedBitmapParams.Data2_p - ((STVID_PictureInfos_t *)EventData)->VideoParams.DecimatedBitmapParams.Data1_p;

                    ((STVID_PictureInfos_t *)EventData)->VideoParams.DecimatedBitmapParams.Data1_p = FrameBuffers_p->MappedAddress_p;
                    ((STVID_PictureInfos_t *)EventData)->VideoParams.DecimatedBitmapParams.Data2_p = (void *)((U32)FrameBuffers_p->MappedAddress_p + Offset);
                }
                else
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            STOS_SemaphoreSignal(VideoInstanceSemaphore_p);
            break;

        default:
            /* Nothing to do for other events */
            break;
    }

    return(ErrorCode);
}

/* End of stvid_ioctl_lib.c */
