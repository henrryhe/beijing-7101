/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_core_proc.c
 *  Date        : 06-00-2005
 *  Author      : STIEGLITZP
 *  Description : procfs implementation
 * 
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/proc_fs.h>

#include <linux/errno.h>   /* Defines standard error codes */

#include "stpti4_core_types.h"       /* Modules specific defines */

#include "stpti4_core_proc.h"        /* Prototytpes for this compile unit */

#include "stpti.h"
#include "pti4.h"
#include "pti_loc.h"
#include "pti_hndl.h"
#include "tchal.h"


/* Allow kernel developers to choose not to have the ProcFS support */
#if defined( CONFIG_PROC_FS )

/*** PROTOTYPES **************************************************************/

static int stpti4_core_read_memapi( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data );

static int stpti4_core_read_buffer( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data );

static int stpti4_core_read_signal( char *buf, char **start, off_t offset,
                             int count, int *eof, void *data );

#ifdef STPTI_DEBUG_SUPPORT
static int stpti4_core_read_procinthist( char *buf, char **start, off_t offset,
                               int count, int *eof, void *data );

static int stpti4_core_read_procstats( char *buf, char **start, off_t offset,
                               int count, int *eof, void *data );
#endif
static char *GetObjectType( FullHandle_t ot );
static int AddMemCtlData( char *buf, MemCtl_t *MemCtl_p );    
/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

char *ObjectTypeArray[] = {
                            "OBJECT_TYPE_BUFFER",        /* 1 */
                            "OBJECT_TYPE_DESCRAMBLER",
                            "OBJECT_TYPE_FILTER",
                            "OBJECT_TYPE_SESSION",
                            "OBJECT_TYPE_SIGNAL",
                            "OBJECT_TYPE_SLOT",
                            "OBJECT_TYPE_CAROUSEL",
                            "OBJECT_TYPE_CAROUSEL_ENTRY",
                            "OBJECT_TYPE_INDEX",
                            "OBJECT_TYPE_DEVICE",
                            "OBJECT_TYPE_DRIVER",
                            "OBJECT_TYPE_FRONTEND",
                            "OBJECT_TYPE_LIST"  };




/*** LOCAL VARIABLES *********************************************************/

typedef struct PTIProcEntry_s{
    char Name[ST_MAX_DEVICE_NAME];
    U32 Address;
    struct proc_dir_entry *proc_dir_p;
    struct proc_dir_entry *buff_dir_p;
    struct proc_dir_entry *sig_dir_p;
    struct PTIProcEntry_s *Next_p;
#ifdef STPTI_DEBUG_SUPPORT
#define INTERRUPT_HISTORY_LEN 4
    STPTI_DebugInterruptStatus_t History[INTERRUPT_HISTORY_LEN];
#endif
} PTIProcEntry_t;

PTIProcEntry_t *PTIProcEntries_p[16] = {0};

static struct proc_dir_entry *stpti4_core_proc_root_dir = NULL; /* All proc files for this module will appear in here. */

/*** METHODS ****************************************************************/

void AddPTIDeviceToProcFS( FullHandle_t DeviceHandle )
{
    PTIProcEntry_t **Next_p = &PTIProcEntries_p[DeviceHandle.member.Device];
    Device_t        *Device_p      = stptiMemGet_Device( DeviceHandle );
    
    while( *Next_p ){
        if( strncmp( (*Next_p)->Name, (char*)Device_p->DeviceName, ST_MAX_DEVICE_NAME) == 0 ){
            /* Entry exists so just return */
            return;
        }
        Next_p = &((*Next_p)->Next_p);
    }
    /* Not found so add entry */
    *Next_p = (PTIProcEntry_t *)kmalloc(sizeof(PTIProcEntry_t), GFP_KERNEL);

    (*Next_p)->Next_p = NULL;
    (*Next_p)->Address = (U32)Device_p->TCDeviceAddress_p;
    
    strncpy((*Next_p)->Name,(char*)Device_p->DeviceName,ST_MAX_DEVICE_NAME); /* Max 15 chars */
    (*Next_p)->Name[ST_MAX_DEVICE_NAME-1] = '\0';
    (*Next_p)->proc_dir_p = proc_mkdir((const char*)(*Next_p)->Name,stpti4_core_proc_root_dir);
    (*Next_p)->buff_dir_p = proc_mkdir("Buffers",(*Next_p)->proc_dir_p);
    (*Next_p)->sig_dir_p = proc_mkdir("Signals",(*Next_p)->proc_dir_p);

#ifdef STPTI_DEBUG_SUPPORT
    /* Start collecting interrupt data */
    STPTI_DebugInterruptHistoryStart ((char*)Device_p->DeviceName, (*Next_p)->History , INTERRUPT_HISTORY_LEN * sizeof (STPTI_DebugInterruptStatus_t));
    /* Start collecting stats */
    STPTI_DebugStatisticsStart ((char*)Device_p->DeviceName);
#endif
    /* This allows the PTI registers to be examined. */
    create_proc_read_entry("TCDevice_t",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_procregs,
                                       (void*)DeviceHandle.word);

    create_proc_read_entry("TCGlobalInfo_t",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_procglobal,
                                       (void*)DeviceHandle.word);

    create_proc_read_entry("Filters",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_procfilters,
                                       (void*)DeviceHandle.word);

    create_proc_read_entry("TCMainInfo_t",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_procmaininfo,
                                       (void*)DeviceHandle.word);
    
    create_proc_read_entry("TCSessionInfo_t",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_proc_sess,
                                       (void*)DeviceHandle.word);

    create_proc_read_entry("TCDMAConfig_t",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_proc_globaltcdmacfg,
                                       (void*)DeviceHandle.word);

    create_proc_read_entry("SlotLUT",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_proc_lut,
                                       (void*)DeviceHandle.word);

    create_proc_read_entry("TC_Data",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_proctcdata,
                                       (void*)DeviceHandle.word);

    create_proc_read_entry("STPTI_TCParameters_t",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_proc_tcparams,
                                       (void*)DeviceHandle.word);
    
    create_proc_read_entry("CAM",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_proc_cam,
                                       (void*)DeviceHandle.word);

    create_proc_read_entry("Objects",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_proc_objects,
                                       (void*)DeviceHandle.word);

    create_proc_read_entry("Misc",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stptiHAL_read_procmisc,
                                       NULL);
#ifdef STPTI_DEBUG_SUPPORT
    create_proc_read_entry("IntHistory",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stpti4_core_read_procinthist,
                                       *Next_p);

    create_proc_read_entry("Statistics",
                                       0444,
                                       (*Next_p)->proc_dir_p,
                                       stpti4_core_read_procstats,
                                       *Next_p);
#endif
}



void AddPTIBufferToProcFS( FullHandle_t BufferHandle )
{
    char name[64];
    PTIProcEntry_t *Next_p = PTIProcEntries_p[BufferHandle.member.Device];
    Device_t       *Device_p      = stptiMemGet_Device( BufferHandle );
    
    while( Next_p ){
        if( strncmp( Next_p->Name, (char*)(Device_p->DeviceName), ST_MAX_DEVICE_NAME) == 0 ){
            break;
        }
        Next_p = Next_p->Next_p;
    }
    
    if (Next_p){
    
        /* This allows the PTI registers to be examined. */
        sprintf(name, "%08X", (int)BufferHandle.word);
    
        create_proc_read_entry(name,
                               0444,
                               Next_p->buff_dir_p,
                               stpti4_core_read_buffer,
                               (void*)BufferHandle.word);
    }
}





void RemovePTIBufferFromProcFS( FullHandle_t BufferHandle )
{
    char name[64];
    PTIProcEntry_t **Next_p = &PTIProcEntries_p[BufferHandle.member.Device];
    Device_t        *Device_p      = stptiMemGet_Device( BufferHandle );

    sprintf(name, "%08X", (int)BufferHandle.word);
    
    while( *Next_p ){
        if( strncmp( (*Next_p)->Name, (char*)Device_p->DeviceName, ST_MAX_DEVICE_NAME) == 0 ){            
            remove_proc_entry(name,(*Next_p)->buff_dir_p);
        }
        Next_p = &((*Next_p)->Next_p);
    }
}

void AddPTISignalToProcFS( FullHandle_t SignalHandle )
{
    char name[64];
    PTIProcEntry_t *Next_p = PTIProcEntries_p[SignalHandle.member.Device];
    Device_t       *Device_p      = stptiMemGet_Device( SignalHandle );
    

    while( Next_p ){
        if( strncmp( Next_p->Name, (char*)Device_p->DeviceName, ST_MAX_DEVICE_NAME) == 0 ){        
            break;
        }
        Next_p = Next_p->Next_p;
    }
    
    if (Next_p){
    
        /* This allows the PTI registers to be examined. */
        sprintf(name, "%08X", (int)SignalHandle.word);
    
        create_proc_read_entry(name,
                               0444,
                               Next_p->sig_dir_p,
                               stpti4_core_read_signal,
                               (void*)SignalHandle.word);
    }
}

void RemovePTISignalFromProcFS( FullHandle_t SignalHandle )
{
    char name[64];
    PTIProcEntry_t **Next_p = &PTIProcEntries_p[SignalHandle.member.Device];
    Device_t        *Device_p      = stptiMemGet_Device( SignalHandle );

    sprintf(name, "%08X", (int)SignalHandle.word);
    
    while( *Next_p ){
        if( strncmp( (*Next_p)->Name, (char*)Device_p->DeviceName, ST_MAX_DEVICE_NAME) == 0 ){            
            remove_proc_entry(name,(*Next_p)->sig_dir_p);
        }
        Next_p = &((*Next_p)->Next_p);
    }
}

void RemovePTIDeviceFromProcFS( FullHandle_t DeviceHandle )
{
    PTIProcEntry_t **Next_p = &PTIProcEntries_p[DeviceHandle.member.Device];
    Device_t        *Device_p      = stptiMemGet_Device( DeviceHandle );

    while( *Next_p ){
        if( strncmp( (*Next_p)->Name, (char*)Device_p->DeviceName, ST_MAX_DEVICE_NAME) == 0 ){    
            PTIProcEntry_t *temp_p = (*Next_p)->Next_p;

            /* Entry exists so remove it */
            remove_proc_entry("TCDevice_t",(*Next_p)->proc_dir_p);
            remove_proc_entry("TCGlobalInfo_t",(*Next_p)->proc_dir_p);
            remove_proc_entry("TCMainInfo_t",(*Next_p)->proc_dir_p);
            remove_proc_entry("Filters",(*Next_p)->proc_dir_p);
            remove_proc_entry("TCSessionInfo_t",(*Next_p)->proc_dir_p);
            remove_proc_entry("TCDMAConfig_t",(*Next_p)->proc_dir_p);
            remove_proc_entry("SlotLUT",(*Next_p)->proc_dir_p);
            remove_proc_entry("TC_Data",(*Next_p)->proc_dir_p);
            remove_proc_entry("STPTI_TCParameters_t",(*Next_p)->proc_dir_p);
            remove_proc_entry("CAM",(*Next_p)->proc_dir_p);
            remove_proc_entry("Objects",(*Next_p)->proc_dir_p);
            remove_proc_entry("Buffers",(*Next_p)->proc_dir_p);
            remove_proc_entry("Signals",(*Next_p)->proc_dir_p);
            remove_proc_entry("Misc",(*Next_p)->proc_dir_p);
#ifdef STPTI_DEBUG_SUPPORT
            remove_proc_entry("IntHistory",(*Next_p)->proc_dir_p);
            remove_proc_entry("Statistics",(*Next_p)->proc_dir_p);
#endif
            remove_proc_entry((*Next_p)->Name,stpti4_core_proc_root_dir);
            kfree( *Next_p );
            *Next_p = temp_p;
            return;
        }
        Next_p = &((*Next_p)->Next_p);
    }
}

/*=============================================================================

   stpti4_core_init_proc_fs

   Initialise the proc file system
   Called from module initialisation routine.

   This function is called from  stpti4_core_init_module() and is not reentrant.

  ===========================================================================*/
int stpti4_core_init_proc_fs   ()
{
    stpti4_core_proc_root_dir = proc_mkdir("stpti4_core",NULL);
    if (stpti4_core_proc_root_dir == NULL) {

        goto fault_no_dir;
    }

    create_proc_read_entry("memapi",
                                       0444,
                                       stpti4_core_proc_root_dir,
                                       stpti4_core_read_memapi,
                                       NULL);
    

    return 0;  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

 fault_no_dir:
    return (-1);
}

/*=============================================================================

   stpti4_core_cleanup_proc_fs

   Remove the proc file system and and clean up.
   Called from module cleanup routine.

   This function is called from  stpti4_core_cleanup_module() and is not reentrant.

  ===========================================================================*/
int stpti4_core_cleanup_proc_fs   ()
{
    int err = 0;

    remove_proc_entry("memapi",stpti4_core_proc_root_dir);
    remove_proc_entry("stpti4_core",NULL);

    return (err);
}


extern Driver_t STPTI_Driver;

char *GetObjectType( FullHandle_t ot )
{
    if( ot.word == STPTI_NullHandle() )
        return "NULL_HANDLE";
    if( (ot.member.ObjectType < OBJECT_TYPE_BUFFER) || (ot.member.ObjectType>OBJECT_TYPE_LIST) )
        return "INVALID_OBJECT_TYPE";

    return ObjectTypeArray[ ot.member.ObjectType - 1 ];
}

int AddMemCtlData( char *buf, MemCtl_t *MemCtl_p )
{
    int len = 0;
    Cell_t *CellPtr;

    len+=sprintf(buf+len,"MemCtl_p->Partition_p (ST_Partition_t*):0x%08x (N/A)\n",(U32)MemCtl_p->Partition_p);
    len+=sprintf(buf+len,"MemCtl_p->MemSize (size_t)             :0x%08x\n",(U32)MemCtl_p->MemSize);
    len+=sprintf(buf+len,"MemCtl_p->Block_p (Cell_t *)           :0x%08x\n",(U32)MemCtl_p->Block_p);
    len+=sprintf(buf+len,"MemCtl_p->FreeList_p (Cell_t *)        :0x%08x\n",(U32)MemCtl_p->FreeList_p);
    len+=sprintf(buf+len,"MemCtl_p->Handle_p (Handle_t *)        :0x%08x\n",(U32)MemCtl_p->Handle_p);
    len+=sprintf(buf+len,"MemCtl_p->MaxHandles (U32)             :0x%08x\n",(U32)MemCtl_p->MaxHandles);

    len+=sprintf(buf+len,"\nWalk MemCtl_p->FreeList_p list\n");
    CellPtr = MemCtl_p->FreeList_p;

    len+=sprintf(buf+len,"FreeList_p  next_p   prev_p   Handle(O.T.S.D) Size Data Type\n");
    while(CellPtr){
        len+=sprintf(buf+len,"%08x %08x %08x %04d.%01d.%01d.%01d %08x %08x %s\n",
                    (U32)CellPtr,(U32)CellPtr->Header.next_p,(U32)CellPtr->Header.prev_p,
                    (U32)CellPtr->Header.Handle.member.Object,
                    (U32)CellPtr->Header.Handle.member.ObjectType,
                    (U32)CellPtr->Header.Handle.member.Session,
                    (U32)CellPtr->Header.Handle.member.Device,
                    (U32)CellPtr->Header.Size,
                    (U32)&CellPtr->data,
                    GetObjectType(CellPtr->Header.Handle));
        CellPtr = CellPtr->Header.next_p;
    }

    len+=sprintf(buf+len,"\nWalk MemCtl_p->Block_p list\n");
    CellPtr = MemCtl_p->Block_p;

    len+=sprintf(buf+len,"Block_p  next_p   prev_p   Handle(O.T.S.D) Size Data Type\n");
    while(CellPtr){
        len+=sprintf(buf+len,"%08x %08x %08x %04d.%01d.%01d.%01d %08x %08x %s\n",
                    (U32)CellPtr,(U32)CellPtr->Header.next_p,(U32)CellPtr->Header.prev_p,
                    (U32)CellPtr->Header.Handle.member.Object,
                    (U32)CellPtr->Header.Handle.member.ObjectType,
                    (U32)CellPtr->Header.Handle.member.Session,
                    (U32)CellPtr->Header.Handle.member.Device,
                    (U32)CellPtr->Header.Size,
                    (U32)&CellPtr->data,
                    GetObjectType(CellPtr->Header.Handle));
        CellPtr = CellPtr->Header.next_p;
    }

    {
        U16 indx;
        U16 max;

        len+=sprintf(buf+len,"\nWalk MemCtl_p->Handle_p (0x%08x) list\n",(U32)MemCtl_p->Handle_p);
        len+=sprintf(buf+len,"Index Hndl_u.word Size Data_p type\n");
        max = MemCtl_p->MaxHandles > 10?10: MemCtl_p->MaxHandles;
        for (indx = 0; indx < max; ++indx)
        {
            len+=sprintf(buf+len,"%05x 0x%08x 0x%08x 0x%08x %s\n",
                        indx,
                        MemCtl_p->Handle_p[indx].Hndl_u.word,
                        MemCtl_p->Handle_p[indx].Size,
                        (U32)MemCtl_p->Handle_p[indx].Data_p,
                        GetObjectType((FullHandle_t)MemCtl_p->Handle_p[indx].Hndl_u.word) );
        }
    }

    return len;
}

/* Display tcparams info */
int stpti4_core_read_memapi( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    int len = 0;
    unsigned int i, Demuxes;
 
    if( STPTI_Driver.MemCtl.Partition_p == NULL ){/* If initialised */
        len+=sprintf(buf+len,"stpti4_core: STPTI_Init() not called\n");
        *eof = 1;
        return len;
    }
    
    len+=sprintf(buf+len,"MEMAPI Internals.\n");

    len+=sprintf(buf+len,"STPTI_Driver.MemCtl:\n");

    Demuxes = stpti_GetNumberOfExistingDevices();
    
    len+=AddMemCtlData( buf + len, &STPTI_Driver.MemCtl );

    /* Check if the named event handler has been used with other STPTI devices */

    for ( i = 0; i < Demuxes; i++ )
    {

        Device_t *Device_p = stpti_GetDevice( i );

        if ( Device_p != NULL ){
            len+=sprintf(buf+len,"MemCtl for %s. Device_p->memCtl\n",Device_p->DeviceName);

            len+=AddMemCtlData( buf + len, &Device_p->MemCtl );
        }

    }

    for ( i = 0; i < Demuxes; i++ )
    {

        Device_t *Device_p = stpti_GetDevice( i );

        if ( Device_p != NULL ){
            Cell_t *CellPtr;

            CellPtr = Device_p->MemCtl.Block_p;

            while(CellPtr){
                if( CellPtr->Header.Handle.member.ObjectType == OBJECT_TYPE_SESSION ){
                    MemCtl_t *MemCtl_p;
                    len+=sprintf(buf+len,"MemCtl for Open Session %x on device %s.\n",
                        CellPtr->Header.Handle.word,Device_p->DeviceName);
                    len+=sprintf(buf+len,"Address %x.\n",(U32)&((Session_t*)&CellPtr->data)->MemCtl);
                    MemCtl_p = &((Session_t*)&CellPtr->data)->MemCtl;

                    len+=AddMemCtlData( buf + len, MemCtl_p );
                }
                CellPtr = CellPtr->Header.next_p;
            }

        }

    }

    *eof = 1;
    return len;
}



/* Display buffer info */
int stpti4_core_read_buffer( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    ST_ErrorCode_t Error;
    int len = 0;
    STPTI_Buffer_t BufferHandle; 
    
    BufferHandle = (U32)data;

    if( (Error = STPTI_BufferReadNBytes(BufferHandle,
                           (U8*)buf,
                           count,
                           NULL,
                           0,
                           (U32*)&len,
                           STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL,
                           count)) == ST_NO_ERROR ){
        
        if( len ){
            if( len == count ){
                *start = buf; /* Not at end of buffer */
                *eof = 0;                
            }
            else{ /* Must be at end of file with data to read */
                *start = buf; 
                *eof = 1;
            }
        }
        else{
            *start = NULL; /* At end of buffer */
            *eof = 1;            
        }
    }
    else{

        len = sprintf( buf+len,"STPTI_BufferReadNBytes Failed. Error = %x\n",Error);
        *eof = 1;      
    }
    printk(KERN_ALERT"len out %d eof %d\n",len,*eof);    
    return len;
}

/* Display signal info */
int stpti4_core_read_signal( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    int len = 0;
    FullHandle_t FullSignalHandle;
    Signal_t *Signal_p;
    FullHandle_t BufferListHandle;
    
    FullSignalHandle.word = (STPTI_Signal_t)data;

    Signal_p = stptiMemGet_Signal(FullSignalHandle);

    len += sprintf( buf+len,"OwnerSession  : 0x%08x\n",Signal_p->OwnerSession);   
    len += sprintf( buf+len,"SignalHandle  : 0x%08x\n",Signal_p->Handle);      
    len += sprintf( buf+len,"QueueWaiting  : 0x%08x\n",Signal_p->QueueWaiting);  
    len += sprintf( buf+len,"Queue_p       : 0x%08x\n",(int)Signal_p->Queue_p);
    if( Signal_p->BufferWithData == STPTI_NullHandle() )
        len += sprintf( buf+len,"BufferWithData: STPTI_NullHandle\n");     
    else
        len += sprintf( buf+len,"BufferWithData: 0x%08x\n",Signal_p->BufferWithData);      
    
    BufferListHandle = Signal_p->BufferListHandle;
    
    if( BufferListHandle.word != STPTI_NullHandle() ){
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(BufferListHandle)->MaxHandles;

        len += sprintf( buf+len,"Associated Buffers:\n");
        
        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t BufferHandle;

            BufferHandle.word = (&stptiMemGet_List(BufferListHandle)->Handle)[Indx];
            if (BufferHandle.word != STPTI_NullHandle())
            {
                len += sprintf( buf+len,"  %03d: 0x%08x\n",Indx,BufferHandle.word);                
                break;
            }
        }       
    }
    else{
        len += sprintf( buf+len,"No associated buffers\n");        
    }
    
    *eof = 1;      
    
    return len;
}

#ifdef STPTI_DEBUG_SUPPORT


/*=============================================================================

   stpti4_core_read_procinthist

   Output interrupt history using STPTI_DebugGetInterruptHistory.

  ===========================================================================*/
int stpti4_core_read_procinthist( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    ST_ErrorCode_t Error;
    int len = 0;
    int NoOfStructsStored = INTERRUPT_HISTORY_LEN;
    
    PTIProcEntry_t *PTIEntry = (PTIProcEntry_t *)data;

    Error = STPTI_DebugGetInterruptHistory(PTIEntry->Name, &NoOfStructsStored,buf,&len);
    if( Error != ST_NO_ERROR ){
        len += sprintf( buf+len,"Failed STPTI_DebugGetInterruptHistory: 0x%08x\n", Error);
    }
    
    *eof = 1;      
    
    return len;    
}

/*=============================================================================

   stpti4_core_read_procstats

   Output interrupt history using STPTI_DebugGetStatistics.

  ===========================================================================*/
int stpti4_core_read_procstats( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    ST_ErrorCode_t Error;
    STPTI_DebugStatistics_t Statistics;
    int len = 0;
    
    PTIProcEntry_t *PTIEntry = (PTIProcEntry_t *)data;

    Error = STPTI_DebugGetStatistics(PTIEntry->Name, &Statistics,buf,&len);
    if( Error != ST_NO_ERROR ){
        len += sprintf( buf+len,"Failed STPTI_DebugGetStatistics: 0x%08x\n", Error);
    }
    
    *eof = 1;      
    
    return len;    
}

#endif          /* STPTI_DEBUG_SUPPORT */


#else           /* #if defined( CONFIG_PROC_FS ) ... #else */

/* No ProcFS support in kernel so compile out */

int stpti4_core_init_proc_fs(void)
{
    /* Do nothing, successfully! */
    return 0;
}

int stpti4_core_cleanup_proc_fs(void)
{
    /* Do nothing, successfully! */
    return 0;
}

void AddPTIDeviceToProcFS( FullHandle_t DeviceHandle )
{
    DeviceHandle.word=DeviceHandle.word;    /* avoid compiler warning */
}

void RemovePTIDeviceFromProcFS( FullHandle_t DeviceHandle )
{
    DeviceHandle.word=DeviceHandle.word;    /* avoid compiler warning */
}

void AddPTIBufferToProcFS     ( FullHandle_t BufferHandle )
{
    BufferHandle.word=BufferHandle.word;    /* avoid compiler warning */
}

void RemovePTIBufferFromProcFS( FullHandle_t BufferHandle )
{
    BufferHandle.word=BufferHandle.word;    /* avoid compiler warning */
}

void AddPTISignalToProcFS( FullHandle_t SignalHandle )
{
    SignalHandle.word=SignalHandle.word;    /* avoid compiler warning */
}

void RemovePTISignalFromProcFS( FullHandle_t SignalHandle )
{
    SignalHandle.word=SignalHandle.word;    /* avoid compiler warning */
}

#endif           /* #if defined( CONFIG_PROC_FS ) ... #else  ... #endif*/
