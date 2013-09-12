/*******************************************************************************
COPYRIGHT (C) STMicroelectronics 2001

File name   : aud_test.c
Description : Macros used to test STAUD_xxx functions of the Audio Driver
Note        : All functions return TRUE if error for CLILIB compatibility
 *
Date          Modification                                      Initials
----          ------------                                    --------
08 Jun 2001   Complete overhaul - supports multi-instance driver  MDV
10 Apr 2000   Add DMA inject task command                     CL
              and add missing commands to match API documentation
03 Mar 2000   Errors fwded by result_sym_p + API changes      CL
              The API use is based on the STAUD API documentation v1.2.0
09 Jun 1999   Add AUD_CmdStart()                              FQ
08 Mar 1999   New error code (API changes)                    FQ
30 Sep 1998   Creation                                        PhL

*******************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/
#include <stdio.h>
#include <time.h>
#include <string.h>
#ifdef ST_OSLINUX
#include <sys/sysinfo.h>
#else
#include "os21.h"
#include "stlite.h"
#include "embx.h"
#include "string.h"
#include "mme.h"
#include "embxshm.h"
#include "embxmailbox.h"
#include "embx_types.h"
#endif

#include "sttbx.h"
#include "stsys.h"

#include "stddefs.h"
#include "stdevice.h"
#include "stcommon.h"
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
#include "stclkrv.h"
#endif
#include "staudlx.h"
#include "stevt.h"
#include "clevt.h"

#include "api_cmd.h"
#include "startup.h"
#include "stos.h"
#include "testtool.h"
#include "audlx_cmd.h"
#include "stfdma.h"
#include "staudlx.h"

#define AUD_MAX_NUMBER 2
#define USE_WRAPPER 0

#define STEVT_HANDLER	STEVT_DEVICE_NAME
#define VFS_MAX_BLOCK  (16*1024) /* Maximum alignment need to avoid intermediate memcpy from filesystem */
#define PTI_BUFFER_SIZE       (8 * 1024)
#define PTICMD_AUDIO_SLOT      1
#define PTICMD_PCR_SLOT        2
#define PTICMD_MAX_DEVICES     1
#define PTICMD_MAX_SLOTS       4
#define PTICMD_BASE_AUDIO      2

#ifdef ST_OS21
#define INT_LX_AUDIO                        OS21_INTERRUPT_MB_LX_AUDIO
#define INT_LX_VIDEO						OS21_INTERRUPT_MB_LX_DPHI
#define VIDEO_COMPANION_NAME                "../../companion_video.bin"
#define VIDEO_COMPANION_NAME_CUT2           "../../companion_video_cut2.bin"
#define AUDIO_COMPANION_NAME                "../../companion_0x04200000_017_0.bin"

/* Memory mapping for LMI SYS */
/* -------------------------- */
#define RAM1_BASE_ADDRESS                      tlm_to_virtual(0xA4000000) /* LMI_SYS */
#define RAM1_SIZE                              tlm_to_virtual(0x04000000) /* 64 Mbytes */
#define RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1    tlm_to_virtual(0x04000000)


/* Lx code position in LMI SYS */
/* --------------------------- */

#define VIDEO_COMPANION_OFFSET                 0x00000000
#define AUDIO_COMPANION_OFFSET                 0x00200000

#define DMA_INTERRUPT_LEVEL         13

#if defined(ST_7100)
#ifndef ST_OSLINUX
static EMBX_FACTORY          MBX_Factory;
static U32                   MBX_Address = 0;
static U32                   MBX_Size    = 0;
static STAVMEM_BlockHandle_t MBX_AVMEM_Handle;
#endif
#endif

#if defined(ST_7109)
#ifndef ST_OSLINUX
static EMBX_FACTORY          MBX_Factory;
static U32                   MBX_Address = 0;
static U32                   MBX_Size    = 0;
static STAVMEM_BlockHandle_t MBX_AVMEM_Handle;
#endif
#endif

#ifndef DU32
#if !defined(ST_OSLINUX)
#pragma ST_device(DU32)
#endif
typedef volatile unsigned int DU32;
#endif
#endif



typedef enum
{
 AUD_INJECT_SLEEP,
 AUD_INJECT_START,
 AUD_INJECT_STOP
} AUD_InjectState_t;

#ifndef ST_OSLINUX
#define MAX_PARTITIONS                      2
extern STAVMEM_PartitionHandle_t AvmemPartitionHandle[MAX_PARTITIONS+1];
#endif
ST_DeviceName_t     AVMEM_DeviceName[]={"AVMEM"};
ST_DeviceName_t 	EVT_DeviceName[]={"EVT"};
ST_DeviceName_t 	STCKLRV_DeviceName[]={"CLKRV"};

#ifndef ST_OS21
extern STPTI_Handle_t STPTIHandle[PTICMD_MAX_DEVICES];
extern STPTI_Slot_t SlotHandle[PTICMD_MAX_DEVICES][PTICMD_MAX_SLOTS];
extern STPTI_Buffer_t PTI_AudioBufferHandle;
extern STPTI_Buffer_t  PTI_AudioBufferHandle_p[2];
#endif

/* ++Channel Delay++*/

int LeftChDelay =0;
int RightChDelay =0;
int LeftSurroundChDelay =0;
int RightSurroundChDelay =0;
int CenterChDelay =0;
int SubwooferChDelay =0;

STAUD_StereoMode_t StereoEffect;
int EmphasisFilter=0;
/*+++ Channel Attenuation+++*/
int LeftAttenuation =0;
int RightAttenuation =0;
int LeftSurroundAttenuation =0;
int RightSurroundAttenuation =0;
int SubwooferAttenuation = 0;
int CenterAttenuation = 0;
/*--- Channel Attenuation ---*/

#define AUDi_TraceERR(x) {}
#define AUDi_TraceDBG(x) {}


typedef struct SYS_FileHandle_s
{
 U32   FileSystem;
 void *FileHandle;
} SYS_FileHandle_t;

/* Inject node structure */
/* --------------------- */
typedef struct AUD_InjectContext_s
{

 U32                   InjectorId;            /* Injector identifier                  */

 STAUD_Handle_t        AUD_Handle;            /* Audio handle                         */
 STEVT_Handle_t        EVT_Handle;            /* Event handle                         */
 STAUD_StartParams_t   AUD_StartParams;       /* Audio start parameters               */
 /*U8                           *FILE_Buffer;*/
 /*U8                           *FILE_Buffer_Aligned;*/
 /*U32                           FILE_BufferSize;*/
 void                 *FileHandle;            /* File handle descriptor               */
 U32                   FileSize;              /* File size to play                    */
 U32                   FilePosition;          /* Current file position                */
 U32                   BufferPtr;             /* Buffer to read the file              */
 U32                   BufferBase;            /* Buffer base to read the file         */
 U32                   BufferProducer;        /* Producer pointer in the file buffer  */
 U32                   BufferConsumer;        /* Consumer pointer in the file buffer  */
 U32                   BufferSize;            /* Buffer size to read the file         */
 U32                   BufferAudio;           /* Audio pes buffer                     */
 U32                   BufferAudioProducer;   /* Producer pointer in the audio buffer */
 U32                   BufferAudioConsumer;   /* Consumer pointer in the audio buffer */
 U32                   BufferAudioSize;       /* Audio pes buffer size                */
 U32                   BitBufferAudioSize;    /* Audio bit buffer empty size          */
 AUD_InjectState_t     State;                 /* State of the injector                */
 U32                   NbLoops;               /* Nb loops to do (0=Infinity)          */
 U32                   NbLoopsAlreadyDone;    /* Nb loops already done                */
 U8                    DoByteReverseOnFly;         /* To byte reverse Larger Str             */
 STAUD_Object_t       InputObject;         /*Parser Object to which injection will be giving the data*/

 semaphore_t           *SemStart;              /* Start semaphore to injector task     */

 semaphore_t           *SemStop;               /* Stop  semaphore to injector task     */
 semaphore_t           *SemLock;               /* Lock  semaphore for ptr protection   */
 semaphore_t           *SemWaitEOF;      /*EOF semaphore to wait for complete playback */

#ifdef ST_OSLINUX
	 BOOL              BufferMapped;           /* True if buffer mapped to user space  */
	 pthread_t 		   PCMInjectTask;		   /* used for PCM injection task */
#else
 task_t               *Task;                  /* Task descriptor of the injector      */
 void*                 TaskStack;
 tdesc_t               TaskDesc;

#endif
 char				   FileName[256];

#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
 U8                   *FdmaNode;              /* Node pointer allocated               */
 STFDMA_Node_t        *FdmaNodeAligned;       /* Node aligned address                 */
 STFDMA_ChannelId_t    FdmaChannelId;         /* Channel id                           */
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5528))
 STGPDMA_Handle_t      GpdmaHandle;           /* Handle of DMA                        */
#endif
BOOL                            ForceStop;
BOOL                            LoadFlag;
BOOL                            SetEOFFlag;
} AUD_InjectContext_t;


STAUD_Handle_t  AUD_Handle[AUD_MAX_NUMBER];
ST_DeviceName_t AUD_DeviceName[]={"AUD0","AUD1"};
AUD_InjectContext_t *AUDi_InjectContext[AUD_MAX_NUMBER];

ST_ErrorCode_t AUD_InjectStart(U32 DeviceId,U8 *FileName,STAUD_StartParams_t *AUD_StartParams,U32 NbLoops);
ST_ErrorCode_t AUD_InjectStop(U32 DeviceId);
ST_ErrorCode_t AUD_Debug(void);

static void  AUDi_InjectTask(void *AUD_InjectContext);
static void  AUDi_PCMInjectTask(void *AUD_InjectContext);

/*#define SYS_FClose(x)       (NULL)*/

#define DEFAULT_AUD_NAME    "LONGAUDDEVNAME"   /* device name */

/* CD2 Params are ignored on 5518, but the following seems the most robust default */
#ifndef AUDIO_IF_BASE_ADDRESS
#error ERROR: not defined AUDIO_IF_BASE_ADDRESS
#define AUDIO_IF_BASE_ADDRESS NULL
#endif

#define AUDIO_WORKSPACE         4096
#define MAX_PCMBUFFERS          5
#define MAGICVAL                113

/* Events */
#define NB_OF_AUDIO_EVENTS         STAUD_EVENT_NUMBER
#define NB_MAX_OF_STACKED_EVENTS   100
/* Number of audio injection tasks */
#define MAX_INJECT_TASK                 2


typedef union
{
    STAUD_Emphasis_t   Emphasis;
    STAUD_Stereo_t     StreamRouting;
    U32                NewFrequency;
    STAUD_PTS_t        NewFrame;
    U8 Error;

} EventData_t;

typedef struct
{
    long EvtNum;
    EventData_t EvtData;
} RcvEventInfo_t;

typedef char ObjectString_t[40];

typedef struct {
    unsigned char *FileBuffer;      /* address of stream buffer */
    int           NbBytes;          /* size of stream buffer */
    int           Loop;
#ifdef ST_OSLINUX
     BOOL         BufferMapped;
#else
    task_t        *TaskID;
#endif
    void          (*Injector_p)(void *Loop);
    int           Stack[AUDIO_WORKSPACE];
    char          *AllocatedBuffer;
    unsigned char *WritePES_p;
    unsigned char *ReadPES_p;
    unsigned char *BasePES_p;
    unsigned char *TopPES_p;
    char          FirstTime;
} AUD_InjectTask_t;

typedef struct {
    BOOL Initialized;
    BOOL InternalPLL;
    U32  DACClockToFsRatio;
    STAUD_PCMDataConf_t DACConfiguration;
} LocalDACConfiguration_t;

enum {
    UNKNOWN_CELL,
    MPEG1_CELL,
    MMDSP_CELL,
    AMPHION_CELL,
	LX_CELL
};
#define LX_AUDIO

typedef struct structure1{
    STAUD_Handle_t DecoderHandle;
    struct structure1 *next;
} Open_t;

typedef Open_t *Open_p;

typedef struct structure2{
    char DeviceName[40];
    Open_p Open_head;
    struct structure2 *next;
} Init_t;

typedef Init_t *Init_p;

/* --- Global Variables --- */

static char *list_ptr=NULL,*cur_ptr=NULL;

static STEVT_Handle_t  EvtRegHandle;                /* handle for the registrant */
static STEVT_Handle_t  EvtSubHandle;                /* handle for the subscriber */
static STEVT_EventID_t EventID[NB_OF_AUDIO_EVENTS]; /* event IDs (one ID per event number) */
static RcvEventInfo_t  RcvEventStack[NB_MAX_OF_STACKED_EVENTS]; /* list of event numbers occured */

static short RcvEventCnt[NB_OF_AUDIO_EVENTS];      /* events counting (one count per event number)*/
static short NbOfRcvEvents=0;                      /* nb of events in stack */
static short NewRcvEvtIndex=0;                     /* top of the stack */
static short LastRcvEvtIndex=0;                    /* bottom of the stack */
static boolean EvtInitAlreadyDone=FALSE;           /* flag */
static boolean EvtEnableAlreadyDone=FALSE;         /* flag */
static char  AUD_Msg[200];            /* text for trace */

static AUD_InjectTask_t InjectTaskTable[MAX_INJECT_TASK];       /* Injection task(s) data */

/*static int count;*/
STAUD_Handle_t AudDecHndl0;           /* handle for audio */
/*STAUD_Object_t DecoderObject;*/

/*#ifndef ST_OSLINUX*/
/*extern ST_Partition_t *DriverPartition_p;*/
/*extern STAVMEM_PartitionHandle_t AvmemPartitionHandle;*/
/*#endif*/
/*extern ST_Partition_t *NCachePartition; */
extern ST_Partition_t       *system_partition_stfae ;
/*extern ST_Partition_t *     system_partition;*/
#ifndef ST_OSLINUX
extern U32 GetTaskDelay(U32 CpuDelay);
#endif

volatile BOOL SwitchTaskInject = FALSE;
semaphore_t *SwitchTaskOKSemaphore_p;

#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5107)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
static void           AUDi_InjectCopy_FDMA(AUD_InjectContext_t *AUD_InjectContext,U32 SizeToCopy);
#endif

/*** Interactive DAC configuration structure  ***/
static LocalDACConfiguration_t LocalDACConfiguration;
static void AUDi_InjectCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);
static ST_ErrorCode_t AUDi_GetWritePtr(void *const Handle,void **const Address);
static ST_ErrorCode_t AUDi_SetReadPtr(void *const Handle,void *const Address);
/* used in AUD_Verbose to make functions print details or not */
ST_ErrorCode_t PutFrame( U8 *MemoryStart,	U32 Size,	U32 PTS,	U32 PTS33, void * clientInfo);
static BOOL Verbose;

Init_p Init_head = NULL;

#ifdef  USE_HDD_INJECT

#define LOW_DATA_LEVEL          50               /* 50% threshold */

/* Semaphore used to control 'HDD style' injection task */
#if !defined(ST_OS21)
static semaphore_t LowDataSemaphore;
#endif
static semaphore_t *LowDataSemaphore_p;

static void AUD_RcvLowDataEvt ( STEVT_CallReason_t Reason,
                                STEVT_EventConstant_t Event, const void *EventData);
#endif
/* Macros */
#define GetMin(a, b)    ( ( (a) < (b) ) ? (a) : (b) )

/* Function prototypes --------------------------------------------------- */

static void DeviceToText( STAUD_Device_t Device, ObjectString_t * Text );
static void ObjectToText( STAUD_Object_t Object, ObjectString_t * Text );
static void StreamContentToText( STAUD_StreamContent_t, char *Text );


/*static U32 SYS_ChipVersion(void);*/
#ifdef ST_OS21
static void *SYS_FOpen(const char *filename,const char *mode);
/*static U32 SYS_FWrite(void *ptr,U32 size,U32 nmemb,void *FileContext);*/
static U32 SYS_FRead(void *ptr,U32 size,U32 nmemb,void *FileContext);
static U32 SYS_FSeek(void *FileContext,U32 offset,U32 whence);
static U32 SYS_FClose(void *FileContext);
static U32 SYS_FTell(void *FileContext);
void AUD_ReceiveEvt (STEVT_CallReason_t Reason,
     STEVT_EventConstant_t Event, void *EventData);
static boolean AUD_EnableEvt(parse_t *pars_p, char *result_sym_p);
#endif
BOOL AUDTEST_ErrorReport( ST_ErrorCode_t ErrCode, char *AUD_Msg1 );
BOOL AUD_CmdStart(void);

/*-------------------------------------------------------------------------
 * Function : SYS_ChipVersion
 * Input    :
 *
 * Output   :
 * Return   : ChipVersion
 * ----------------------------------------------------------------------*/
#if 0
static U32 SYS_ChipVersion(void)
{
 U32 ChipVersion=0;

 /* For 7100 */
 /* -------- */
#if defined(ST_7100)
 ChipVersion = *((U32*)(0xB9001000+0x000));
 ChipVersion = ((ChipVersion>>28)&0x0F);
#endif

#if defined(ST_7109)
 ChipVersion = *((U32*)(0xB9001000+0x000));
 ChipVersion = ((ChipVersion>>28)&0x0F);
#endif
 /* Return chip version */
 /* ------------------- */
return(ChipVersion);
}
#endif
#ifdef ST_OS21
/*-------------------------------------------------------------------------
 * Function : SYS_FOpen
 * Input    :
 *
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void *SYS_FOpen(const char *filename,const char *mode)
{
 SYS_FileHandle_t *File;

 /* Allocate a file handle descriptor */
 /* ================================= */
 File=(SYS_FileHandle_t *)STOS_MemoryAllocate(SystemPartition_p,sizeof(SYS_FileHandle_t));
 if (File==NULL)
  {
   return(NULL);
  }


#if defined(OSPLUS)
 if (filename[0]=='/')
  {
   U32 i,j;
   char local_mode[8];
   File->FileSystem = 1; /* For VFS */
   /* Remove 'b'inary flag from the mode string for the virtual file system */
   for (i=0,j=0;i<strlen(mode);i++)
    {
     if (mode[i]!='b') local_mode[j++]=mode[i];
    }
   local_mode[j]='\0';
   File->FileHandle = (void *)vfs_fopen(filename,local_mode);
  }
#endif

 /* For JTAG OS20 */
 /* ============= */
#if defined(ST_OS20)
 if (filename[0]!='/')
  {
   File->FileSystem = 2;
   File->FileHandle = (SYS_FileHandle_t *)debugopen(filename,mode);
   if (File->FileHandle==((SYS_FileHandle_t *)0xFFFFFFFF)) File->FileHandle=0;
  }
#endif

 /* For JTAG 0S21 */
 /* ============= */
#if defined(ST_OS21) | defined(ST_OSLINUX)
 if (filename[0]!='/')
  {
   File->FileSystem = 2;
   File->FileHandle = (SYS_FileHandle_t *)fopen(filename,mode);
  }
#endif

 /* Return file system */
 /* ================== */
 if (File->FileHandle==NULL)
  {
   STOS_MemoryDeallocate(SystemPartition_p,File);
   File=NULL;
  }

 /* Return file handle */
 /* ================== */
 return(File);
}
/*-------------------------------------------------------------------------
* Function : SYS_FClose
* Input    :
*
* Output   :
* Return   :
* ----------------------------------------------------------------------*/
static U32 SYS_FClose(void *FileContext)
{
 U32 ReturnValue;
 SYS_FileHandle_t *File=(SYS_FileHandle_t *)FileContext;

 /* VFS File system */
 /* =============== */
#if defined(OSPLUS)
 if (File->FileSystem==1)
  {
   ReturnValue=vfs_fclose(File->FileHandle);
  }
#endif

 /* For OS20 */
 /* ======== */
#if defined(ST_OS20)
 if (File->FileSystem==2)
  {
   ReturnValue=(U32)debugclose((S32)File->FileHandle);
  }
#endif

 /* For OS21 */
 /* ======== */
#if defined(ST_OS21) | defined(ST_OSLINUX)
 if (File->FileSystem==2)
  {
   ReturnValue=fclose(File->FileHandle);
  }
#endif

 /* Free the handle */
 /* =============== */
 STOS_MemoryDeallocate(SystemPartition_p,File);
 return(ReturnValue);
}

/*-------------------------------------------------------------------------
* Function : SYS_FRead
* Input    :
*
* Output   :
* Return   :
* ----------------------------------------------------------------------*/
static U32 SYS_FRead(void *ptr,U32 size,U32 nmemb,void *FileContext)
{
 U32 ReturnValue;
 SYS_FileHandle_t *File=(SYS_FileHandle_t *)FileContext;

 /* VFS File system */
 /* =============== */
#if defined(OSPLUS)
 if (File->FileSystem==1)
  {
   ReturnValue=vfs_fread(ptr,size,nmemb,File->FileHandle);
  }
#endif

 /* For OS20 */
 /* ======== */
#if defined(ST_OS20)
 if (File->FileSystem==2)
  {
   ReturnValue=(U32)debugread((S32)File->FileHandle,ptr,size*nmemb);
   ReturnValue=ReturnValue/size;
  }
#endif

 /* For OS21 */
 /* ======== */
#if defined(ST_OS21) | defined(ST_OSLINUX)
 if (File->FileSystem==2)
  {
   ReturnValue=fread(ptr,size,nmemb,File->FileHandle);
  }
#endif

 /* Return nb bytes read */
 /* ==================== */
 return(ReturnValue);
}
/*-------------------------------------------------------------------------
* Function : SYS_FSeek
* Input    :
*
* Output   :
* Return   :
* ----------------------------------------------------------------------*/
static U32 SYS_FSeek(void *FileContext,U32 offset,U32 whence)
{
 U32 ReturnValue;
 SYS_FileHandle_t *File=(SYS_FileHandle_t *)FileContext;

 /* VFS File system */
 /* =============== */
#if defined(OSPLUS)
 if (File->FileSystem==1)
  {
   ReturnValue=vfs_fseek(File->FileHandle,offset,whence);
  }
#endif

 /* For OS20 */
 /* ======== */
#if defined(ST_OS20)
 if (File->FileSystem==2)
  {
   ReturnValue=(U32)debugseek((S32)File->FileHandle,offset,whence);
  }
#endif

 /* For OS21 */
 /* ======== */
#if defined(ST_OS21) | defined(ST_OSLINUX)
 if (File->FileSystem==2)
  {
   ReturnValue=fseek((FILE *)File->FileHandle,(long int)offset,(int)whence);
  }
#endif

 /* Return nb bytes read */
 /* ==================== */
 return(ReturnValue);
}
/*-------------------------------------------------------------------------
* Function : SYS_FTell
* Input    :
*
* Output   :
* Return   :
* ----------------------------------------------------------------------*/
static U32 SYS_FTell(void *FileContext)
{
 U32 ReturnValue;
 SYS_FileHandle_t *File=(SYS_FileHandle_t *)FileContext;

 /* VFS File system */
 /* =============== */
#if defined(OSPLUS)
 if (File->FileSystem==1)
  {
   ReturnValue=vfs_ftell(File->FileHandle);
  }
#endif

 /* For OS20 */
 /* ======== */
#if defined(ST_OS20)
 if (File->FileSystem==2)
  {
   ReturnValue=(U32)debugtell((S32)File->FileHandle);
  }
#endif

 /* For OS21 */
 /* ======== */
#if defined(ST_OS21) | defined(ST_OSLINUX)
 if (File->FileSystem==2)
  {
   ReturnValue=ftell(File->FileHandle);
  }
#endif

 /* Return nb bytes read */
 /* ==================== */
 return(ReturnValue);
}
#endif
/*-------------------------------------------------------------------------
 * Function : GetFreeSpaceAndAllocatedSpace
 *            Displays the amount of free and Allocated memory in a all partition
 * Input    : Msg         Text string to display
 *            Partition_p Partition to report on
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
#if defined (ST_OS21)
#if 0
static boolean AUD_GetPartitionStatus( parse_t *pars_p, char *result_sym_p )
{
	BOOL RetErr = FALSE;
	partition_status_t PartitionStatus;
	long LVar;
	RetErr = cget_integer( pars_p, 0, &LVar );
	if(LVar>2)
		STTBX_Print(("Invalid Device ID \n "));
	STTBX_Print(("******************************************* \n"));
	STTBX_Print(("Status of Heap \n "));
	if(partition_status(NULL, &PartitionStatus, 0 )==OS21_SUCCESS)
	{
		STTBX_Print(("Free space = %d, Allocated Space = %d \n", PartitionStatus.partition_status_free, PartitionStatus.partition_status_used));
	}
	else
	{
		RetErr = TRUE;
	}
	STTBX_Print(("End Heap \n "));
	STTBX_Print(("******************************************* \n \n \n"));


	STTBX_Print(("******************************************* \n"));
	STTBX_Print(("Status of Ncache Partition \n "));
	if (partition_status(ncache_partition_stfae, &PartitionStatus, 0 ) == OS21_SUCCESS)
	{
		STTBX_Print(("Free space = %d, Allocated Space = %d \n", PartitionStatus.partition_status_free, PartitionStatus.partition_status_used));
	}
	else
	{
		RetErr = TRUE;
	}
	STTBX_Print(("End Ncache \n "));
	STTBX_Print(("******************************************* \n \n \n"));


	STTBX_Print(("******************************************* \n"));
	STTBX_Print(("Status of System Partition \n "));
	if (partition_status(system_partition_stfae, &PartitionStatus, 0 ) == OS21_SUCCESS)
	{
		STTBX_Print(("Free space = %d, Allocated Space = %d \n", PartitionStatus.partition_status_free, PartitionStatus.partition_status_used));
	}
	else
	{
		RetErr = TRUE;
	}
	STTBX_Print(("End System \n "));
	STTBX_Print(("******************************************* \n \n \n"));
	assign_integer(result_sym_p,RetErr,false);
	return ( API_EnableError ? RetErr : FALSE );
}
#endif
#endif

/*-------------------------------------------------------------------------
 * Function : GetPartitionFreeSpace
 *            Displays the amount of free memory in a particular partition
 * Input    : Msg         Text string to display
 *            Partition_p Partition to report on
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
#if defined STTBX_REPORT
static void GetPartitionFreeSpace(char *Msg,  ST_Partition_t *Partition_p)
{
U32 FreeSpace = 0;
#ifdef ST_OSLINUX
    struct sysinfo info;
    sysinfo(&info);
    FreeSpace = info.freeram;
#endif

    STTBX_Report(( STTBX_REPORT_LEVEL_INFO, "%s = 0x%08X", Msg, FreeSpace ));
}
#else
/* NULL macro if TBX not in use */
#define GetPartitionFreeSpace( a, b )
#endif

/*-------------------------------------------------------------------------
 * Function : AUDTEST_ErrorReport
 *            Converts an error code to a text string
 * Input    : ErrCode
 * Output   :
 * Return   : FALSE if ErrCode = ST_NO_ERROR, else TRUE
 * ----------------------------------------------------------------------*/
BOOL AUDTEST_ErrorReport( ST_ErrorCode_t ErrCode, char *AUD_Msge )
{
    BOOL RetErr = TRUE;

    switch ( ErrCode )
    {
      case ST_NO_ERROR:
        RetErr = FALSE;
        strcat( AUD_Msge, "ok\n" );
        break;
      case ST_ERROR_INVALID_HANDLE:
        API_ErrorCount++;
        strcat( AUD_Msge, "the handle is invalid !\n");
        break;
      case ST_ERROR_ALREADY_INITIALIZED:
        API_ErrorCount++;
        strcat( AUD_Msge, "the decoder is already initialized !\n" );
        break;
      case ST_ERROR_OPEN_HANDLE:
        API_ErrorCount++;
        strcat( AUD_Msge, "at least one handle is still open !\n" );
        break;
      case ST_ERROR_NO_MEMORY:
        API_ErrorCount++;
        strcat( AUD_Msge, "not enough memory !\n" );
        break;
      case ST_ERROR_BAD_PARAMETER:
        API_ErrorCount++;
        strcat( AUD_Msge, "one parameter is invalid !\n" );
        break;
      case ST_ERROR_FEATURE_NOT_SUPPORTED:
        API_ErrorCount++;
        strcat( AUD_Msge, "feature or parameter not supported !\n" );
        break;
      case ST_ERROR_UNKNOWN_DEVICE:
        API_ErrorCount++;
        strcat( AUD_Msge, "Unknown device (not initialized) !\n" );
        break;
      case ST_ERROR_NO_FREE_HANDLES:
        API_ErrorCount++;
        strcat( AUD_Msge, "too many connections !\n" );
        break;
      case STAUD_ERROR_DECODER_RUNNING:
        API_ErrorCount++;
        strcat( AUD_Msge, "decoder already decoding !\n" );
        break;
      case STAUD_ERROR_DECODER_PAUSING:
        API_ErrorCount++;
        strcat( AUD_Msge, "decoder currently pausing !\n" );
        break;
      case STAUD_ERROR_DECODER_STOPPED:
        API_ErrorCount++;
        strcat( AUD_Msge, "decoder stopped !\n" );
        break;
      case STAUD_ERROR_DECODER_PREPARING:
        API_ErrorCount++;
        strcat( AUD_Msge, "decoder already preparing !\n" );
        break;
      case STAUD_ERROR_DECODER_PREPARED:
        API_ErrorCount++;
        strcat( AUD_Msge, "decoder already prepared !\n" );
        break;
      case STAUD_ERROR_DECODER_NOT_PAUSING:
        API_ErrorCount++;
        strcat( AUD_Msge, "decoder is not pausing !\n" );
        break;
      case STAUD_ERROR_INVALID_STREAM_ID:
        API_ErrorCount++;
        strcat( AUD_Msge, "StreamId is invalid !\n" );
        break;
      case STAUD_ERROR_INVALID_STREAM_TYPE:
        API_ErrorCount++;
        strcat( AUD_Msge, "StreamType is invalid !\n" );
        break;
      case STAUD_ERROR_INVALID_DEVICE:
        API_ErrorCount++;
        strcat( AUD_Msge, "STAUD_ERROR_INVALID_DEVICE\n" );
        break;
      case ST_ERROR_INTERRUPT_INSTALL:
        API_ErrorCount++;
        strcat( AUD_Msge, "Interrupt install failed !\n" );
        break;
      case ST_ERROR_INTERRUPT_UNINSTALL:
        API_ErrorCount++;
        strcat( AUD_Msge, "Interrupt unistall failed !\n" );
        break;
      default:
        API_ErrorCount++;
        sprintf( AUD_Msge, "%s unexpected error [%X] !\n", AUD_Msg, ErrCode );
        break;
    }

    return( RetErr ); /* Returns TRUE if and error was detected */
}

/*#######################################################################*/
/*####### STATIC FUNCTIONS : TESTTOOL USER INTERFACE ####################*/
/*#######                                            ####################*/
/*#######################################################################*/
/*-------------------------------------------------------------------------
 * Function : AUD_IsCDDACapable
 *            Determines if decoder is CD-DA capable or not
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if DTSCapable, FALSE if not
 * ----------------------------------------------------------------------*/
static boolean AUD_IsCDDACapable( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    char DeviceName[80];
    char DefaultName[80];
    STAUD_DRCapability_t Capability;

    RetErr = TRUE;
    sprintf(DefaultName, DEFAULT_AUD_NAME);
    RetErr = cget_string( pars_p, DefaultName, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected DeviceName" );
    }

    if ( RetErr == FALSE)
    {
        sprintf( AUD_Msg, "STAUD_DRGetCapability(%s) : ", DeviceName );
        ErrCode = STAUD_DRGetCapability(DeviceName,
                                        STAUD_OBJECT_DECODER_COMPRESSED1,
                                        &Capability);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
        if( (ErrCode == ST_NO_ERROR) &&
            (Capability.SupportedStreamContents & STAUD_STREAM_CONTENT_CDDA) )
        {
            assign_integer(result_sym_p, 1, false);
        }
        else
        {
            assign_integer(result_sym_p, 0, false);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

}

/*-------------------------------------------------------------------------
 * Function : AUD_GetCapability
 *            Retrieve driver's capabilities
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_GetCapability( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_Capability_t Capability;

    U8  DeviceId;
    long LVar;
    RetErr = TRUE;




   RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

	DeviceId = (U8)LVar;

    if ( RetErr == FALSE)
    {

        sprintf( AUD_Msg, "STAUD_GetCapability(%s) : ", AUD_DeviceName[DeviceId] );
        ErrCode = STAUD_GetCapability(AUD_DeviceName[DeviceId], &Capability);

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
        if ( (ErrCode == ST_NO_ERROR) && (Verbose == TRUE))
        {
            sprintf( AUD_Msg, "FadingCapable             %d\n", Capability.DRCapability.FadingCapable);
            STTBX_Print(( AUD_Msg ));
            sprintf( AUD_Msg, "MultiChannelOutputCapable %d\n", Capability.PostProcCapability.MultiChannelOutputCapable);
            STTBX_Print(( AUD_Msg ));
            sprintf( AUD_Msg, "ChannelMuteCapable        %d\n", Capability.OPCapability.ChannelMuteCapable);
            STTBX_Print(( AUD_Msg ));
            sprintf( AUD_Msg, "ChannelAttenuationCapable %d\n",Capability.PostProcCapability.ChannelAttenuationCapable);
            STTBX_Print(( AUD_Msg ));
            sprintf( AUD_Msg, "DeEmphasisCapable         %d\n",Capability.DRCapability.DeEmphasisCapable);
            STTBX_Print(( AUD_Msg ));
            sprintf( AUD_Msg, "DelayCapable              %d\n",Capability.PostProcCapability.DelayCapable);
            STTBX_Print(( AUD_Msg ));
            sprintf( AUD_Msg, "DynamicRangeCapable       %d\n",Capability.DRCapability.DynamicRangeCapable);
            STTBX_Print(( AUD_Msg ));
            sprintf( AUD_Msg, "PrologicDecodingCapable   %d\n",Capability.DRCapability.PrologicDecodingCapable);
            STTBX_Print(( AUD_Msg ));
            sprintf( AUD_Msg, "MaxAttenuation            %d\n",Capability.PostProcCapability.MaxAttenuation);
            STTBX_Print(( AUD_Msg ));
            sprintf( AUD_Msg, "AttenuationPrecision       %d\n",Capability.PostProcCapability.AttenuationPrecision);
            STTBX_Print(( AUD_Msg ));

            strcat( AUD_Msg, "\n");
            STTBX_Print(( AUD_Msg ));

        }
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* AUD_GetCapability */


/*-------------------------------------------------------------------------
 * Function : AUD_DRGetStreamInfo
 *            Retrieve current stream info
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_DRGetStreamInfo( parse_t *pars_p, char *result_sym_p )
{
    boolean RetErr;
    ST_ErrorCode_t ErrorCode;
    long LVar;
    STAUD_Object_t DecoderObject;
    STAUD_StreamInfo_t StreamInfo;
    ObjectString_t DRObject;
    U8  DeviceId;
    char Text[80];

    RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

   DeviceId = (U8)LVar;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_INPUT_CD0, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected Object: OBJ_IPCD0 or OBJ_IPCD1\n" );
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        DecoderObject = (STAUD_Object_t)LVar;
        ObjectToText( DecoderObject, &DRObject );
    }

    if ( RetErr == FALSE)
    {
        sprintf( AUD_Msg, "STAUD_DRGetStreamInfo(%s) : ", DRObject );
        ErrorCode = STAUD_IPGetStreamInfo(AUD_Handle[DeviceId], DecoderObject, &StreamInfo );
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
        if ( (ErrorCode == ST_NO_ERROR)) /* && (Verbose == TRUE))*/
        {

            StreamContentToText( StreamInfo.StreamContent, Text );
            STTBX_Print(("StreamContent               = %s \n",
                        Text ));
            STTBX_Print(("SamplingFrequency           = %u Hz\n",
                         StreamInfo.SamplingFrequency ));

        }
    }

    assign_integer(result_sym_p, (long)ErrorCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}
/*-------------------------------------------------------------------------
 * Function : AUD_DRGetSamplingFrequency
 *            Retrieve current sampling stream info
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_DRGetSamplingFrequency( parse_t *pars_p, char *result_sym_p )
{
    boolean RetErr;
    ST_ErrorCode_t ErrorCode;
    long LVar;
    STAUD_Object_t DecoderObject;
    STAUD_StreamInfo_t StreamInfo;
    ObjectString_t DRObject;
    U8  DeviceId;
    U32 SamplingFrequency_t;

    RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

   DeviceId = (U8)LVar;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_INPUT_CD0, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected Object: OBJ_IPCD0 or OBJ_IPCD1\n" );
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        DecoderObject = (STAUD_Object_t)LVar;
        ObjectToText( DecoderObject, &DRObject );
    }


   RetErr = cget_integer( pars_p, 48000, &LVar );
   SamplingFrequency_t = (U32)LVar;


    if ( RetErr == FALSE)
    {
        sprintf( AUD_Msg, "STAUD_DRGetStreamInfo(%s) : ", DRObject );
        ;ErrorCode = STAUD_IPGetStreamInfo(AUD_Handle[DeviceId], DecoderObject, &StreamInfo );
        ErrorCode = STAUD_IPGetSamplingFrequency(AUD_Handle[DeviceId], DecoderObject, &SamplingFrequency_t );
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
        if ( (ErrorCode == ST_NO_ERROR)) /* && (Verbose == TRUE))*/
        {
/*            char Text[80];*/
            /*
            StreamContentToText( StreamInfo.StreamContent, Text );
            STTBX_Print(("StreamContent               = %s \n",
                         Text ));*/
            STTBX_Print(("SamplingFrequency           = %u Hz\n",
                         SamplingFrequency_t ));

        }
    }

    assign_integer(result_sym_p, (long)ErrorCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}



/* aud_checkinfo DECODER CONTENT RATE SFR CHANS FRONT REAR LFE MULTI SURR */
/*-------------------------------------------------------------------------
 * Function : AUD_CheckStreamInfo
 *            Retrieve current stream info
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_CheckStreamInfo( parse_t *pars_p, char *result_sym_p )
{
    boolean RetErr;
    ST_ErrorCode_t ErrorCode;
    long LVar;
    STAUD_Object_t DecoderObject;
    STAUD_StreamInfo_t StreamInfo;
    ObjectString_t DRObject;
    STAUD_StreamContent_t       StreamContent;
    U32                         BitRate;
    U32                         SamplingFrequency;
    U16                         NumberOfChannels;
    U16                         NumberOfFrontChannels;
    U16                         NumberOfRearChannels;
    U16                         NumberOfLFEChannels;
    U16                         LayerID;
    BOOL                        MultiProgram;
    BOOL                        SurroundEncoded;
    U8                          DeviceId;



    RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

   DeviceId = (U8)LVar;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_INPUT_CD0, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected Object: OBJ_IPCD0 or OBJ_IPCD1\n" );
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        DecoderObject = (STAUD_Object_t)LVar;
        ObjectToText( DecoderObject, &DRObject );
    }
    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_STREAM_CONTENT_AC3, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected StreamContent\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            StreamContent = (STAUD_StreamContent_t)LVar;
        }
    }
    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected DataRate (bytes/sec)\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            BitRate = (U32)LVar;
        }
    }
    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected SamplingFrequency (Hz)\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            SamplingFrequency = (U32)LVar;
        }
    }
    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected NumberOfChannels\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            NumberOfChannels = (U16)LVar;
        }
    }
    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected NumberOfFrontChannels\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            NumberOfFrontChannels = (U16)LVar;
        }
    }
    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected NumberOfRearChannels\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            NumberOfRearChannels = (U16)LVar;
        }
    }
    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected NumberOfLFEChannels\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            NumberOfLFEChannels = (U16)LVar;
        }
    }
    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected MultiProgram\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            MultiProgram = (BOOL)LVar;
        }
    }
    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected SurroundEncoded\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            SurroundEncoded = (BOOL)LVar;
        }
    }

    if ( RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected LAYER ID \n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            LayerID = (U16)LVar;
        }
    }

    if ( RetErr == FALSE)
    {
        sprintf( AUD_Msg, "STAUD_DRGetStreamInfo(%s) : ", DRObject );
        ErrorCode = STAUD_IPGetStreamInfo(AUD_Handle[DeviceId], DecoderObject, &StreamInfo );
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
        if(StreamInfo.StreamContent != StreamContent)
        {
            char Text[80];
            StreamContentToText( StreamInfo.StreamContent, Text );
            STTBX_Print(("ERROR: StreamContent = %s expected ", Text ));
            StreamContentToText( StreamContent, Text );
            STTBX_Print(("%s \n", Text));
            RetErr = TRUE;
        }
        if(StreamInfo.BitRate != BitRate)
        {
            STTBX_Print(("ERROR: DataRate = %u expected %u\n",
                         StreamInfo.BitRate, BitRate));
            RetErr = TRUE;
        }
        if(StreamInfo.SamplingFrequency != SamplingFrequency)
        {
            STTBX_Print(("ERROR: SamplingFrequency = %u expected %u\n",
                         StreamInfo.SamplingFrequency, SamplingFrequency));
            RetErr = TRUE;
        }



        if(RetErr == TRUE)
        {
             API_ErrorCount++;
        }
    }

    /* Command returns 0 if Ok, 1 if failed */
    assign_integer(result_sym_p, (long)RetErr, false);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : AUD_GetRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_GetRevision( parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;
	long LVar;

	cget_integer( pars_p, 0, &LVar );
	if(LVar>2)
		STTBX_Print(("Invalid Device ID \n "));

    RevisionNumber = STAUD_GetRevision( );
    sprintf( AUD_Msg, "STAUD_GetRevision() : revision=%s\n", RevisionNumber );
    STTBX_Print(( AUD_Msg ));
    assign_string(result_sym_p, /*(char *)*/RevisionNumber, false);
    return ( FALSE );

} /* end AUD_GetRevision */


/*-------------------------------------------------------------------------
 * Function : AUD_Init
 *              Initialisation
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_Init( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    char LocalDeviceName[40];
    char EvtHandlerName[40];
    char DefaultDeviceName[40] = DEFAULT_AUD_NAME;
    char ClkRecoveryName[40];
    STAUD_InitParams_t AUD_InitParams;
    long LVar;
    boolean RetErr = FALSE;
    Init_p new=NULL;
    Init_p WorkPointer=NULL;
	 U8 DeviceId;
	 U8 DeviceIndex;
	 /* Set the initialization structure */
	/* -------------------------------- */
	RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

	DeviceId = (U8)LVar;

	RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceIndex" );
	}

	DeviceIndex = (U8)LVar;

	memset(&AUD_InitParams      ,0,sizeof(STAUD_InitParams_t));
	AUD_InitParams.DeviceType                                   = STAUD_DEVICE_STI7100;
	AUD_InitParams.Configuration                                = STAUD_CONFIG_DVB_COMPACT;
	AUD_InitParams.DACClockToFsRatio                            = 256;
	AUD_InitParams.PCMOutParams.InvertWordClock                 = FALSE;
	AUD_InitParams.PCMOutParams.Format                          = STAUD_DAC_DATA_FORMAT_I2S;
	AUD_InitParams.PCMOutParams.InvertBitClock                  = FALSE;
	AUD_InitParams.PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_24BITS;
	AUD_InitParams.PCMOutParams.Alignment                       = STAUD_DAC_DATA_ALIGNMENT_LEFT;
	AUD_InitParams.PCMOutParams.MSBFirst                        = TRUE;
    AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier	= 128;
	AUD_InitParams.SPDIFOutParams.AutoLatency					= TRUE;
	AUD_InitParams.SPDIFOutParams.AutoCategoryCode				= TRUE;
	AUD_InitParams.SPDIFOutParams.CategoryCode					= 0;
	AUD_InitParams.SPDIFOutParams.AutoDTDI						= TRUE;
    AUD_InitParams.SPDIFOutParams.CopyPermitted                 = STAUD_COPYRIGHT_MODE_ONE_COPY;/*STAUD_COPYRIGHT_MODE_NO_RESTRICTION; STAUD_COPYRIGHT_MODE_NO_COPY;*/
	AUD_InitParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier= 128;
	AUD_InitParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode		= STAUD_SPDIF_DATA_PRECISION_24BITS;
	AUD_InitParams.SPDIFOutParams.Emphasis						= STAUD_SPDIF_EMPHASIS_CD_TYPE;
	AUD_InitParams.MaxOpen                                      = 1;
	AUD_InitParams.CPUPartition_p                               = SystemPartition_p;
#ifdef ST_OSLINUX
	AUD_InitParams.BufferPartition								= 0;
	AUD_InitParams.AllocateFromEMBX								= TRUE;
    AUD_InitParams.AVMEMPartition                               = 0;
	STTBX_Print(("AUD_InitParams.DriverIndex %d \n",AUD_InitParams.DriverIndex));
#else

    AUD_InitParams.AVMEMPartition                               = AvmemPartitionHandle[0];

#if MSPP_SUPPORT
    AUD_InitParams.BufferPartition                              = AvmemPartitionHandle[1];
	AUD_InitParams.AllocateFromEMBX								= FALSE;
#else /*MSPP_SUPPORT*/
	AUD_InitParams.BufferPartition								= 0;
	AUD_InitParams.AllocateFromEMBX								= TRUE;
#endif /*MSPP_SUPPORT*/

#endif
	AUD_InitParams.DriverIndex				                    = DeviceIndex;
	STTBX_Print(("Driver index %d \n ", AUD_InitParams.DriverIndex));

	AUD_InitParams.PCMMode                                      = PCM_ON;
	AUD_InitParams.SPDIFMode                                    = STAUD_DIGITAL_MODE_NONCOMPRESSED ;/*STAUD_DIGITAL_MODE_COMPRESSED;*/
	AUD_InitParams.NumChannels                                  = 2;

	strcpy(AUD_InitParams.EvtHandlerName,EVT_DeviceName[0]);
	strcpy(AUD_InitParams.ClockRecoveryName,STCKLRV_DeviceName[0]);
/*-----------------Setup (user configurable) InitParams-----------------------*/

    /* get DeviceName */
    if ( RetErr == FALSE )
    {
        RetErr = cget_string( pars_p, DefaultDeviceName, LocalDeviceName, sizeof(LocalDeviceName) );
        if( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected DeviceName" );
        }
    }


    /* get MaxOpen */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, AUD_InitParams.MaxOpen, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected integer (MaxOpen)" );
        }
        else
        {
            AUD_InitParams.MaxOpen = (U32)LVar;
        }
    }
       /* get EvtHandlerName */
    if ( RetErr == FALSE )
    {
        RetErr = cget_string( pars_p, AUD_InitParams.EvtHandlerName, EvtHandlerName, sizeof(EvtHandlerName) );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected EvtHandlerName" );
        }
        else
        {
            strcpy(AUD_InitParams.EvtHandlerName, EvtHandlerName);
        }
    }

    /* get ClockRecoveryName */
    if ( RetErr == FALSE )
    {
        RetErr = cget_string( pars_p, AUD_InitParams.ClockRecoveryName, ClkRecoveryName, sizeof(ClkRecoveryName) );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected ClockRecoveryName" );
        }
        else
        {
            strcpy(AUD_InitParams.ClockRecoveryName, ClkRecoveryName);
        }
    }

/*-----------------------------call STAUD_Init-------------------------------*/

    if ( RetErr == FALSE )
    {
		GetPartitionFreeSpace( "Free space before Init", AUD_InitParams.CPUPartition_p );
		AUD_InitParams.PCMMode=PCM_ON;/*PCM_OFF;*/



       ErrCode = STAUD_Init( AUD_DeviceName[DeviceId], &AUD_InitParams );




        GetPartitionFreeSpace( "Free space after Init", AUD_InitParams.CPUPartition_p );
    }


/*--------------add structure to the global linked list------------------*/

    if( (RetErr == FALSE) && (ErrCode == ST_NO_ERROR) )    /* create new Init structure */
    {
        new = (Init_p)STOS_MemoryAllocate( SystemPartition_p, sizeof(Init_t));
        new -> next = NULL;
        new -> Open_head = NULL;
        strcpy( new -> DeviceName, LocalDeviceName );

        if(Init_head == NULL)                /* list empty */
        {
            Init_head = new;
        }
        else
        {
            WorkPointer = Init_head;
            while( WorkPointer->next != NULL )
            {
                WorkPointer = WorkPointer->next;
            }
            WorkPointer->next = new;
        }
    }

/*-----------------------------report errors----------------------------*/

    if ( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "STAUD_Init(%s,%s) Driver version ", LocalDeviceName, STAUD_GetRevision());

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));

        if ( (ErrCode == ST_NO_ERROR) && (Verbose == TRUE))
        {
            {
                ObjectString_t DeviceName;

                DeviceToText(AUD_InitParams.DeviceType, &DeviceName);
                sprintf(AUD_Msg, "Device Type        %s\n",DeviceName);
                STTBX_Print(( AUD_Msg ));
            }
            sprintf(AUD_Msg, "Max Open           %X\n", AUD_InitParams.MaxOpen);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "EvtHandlerName     %s\n", AUD_InitParams.EvtHandlerName);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "ClkRecoveryName    %s\n", AUD_InitParams.ClockRecoveryName);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "DACClockToFsRatio  %d\n", AUD_InitParams.DACClockToFsRatio);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "InvertWordClock    %d\n", AUD_InitParams.PCMOutParams.InvertWordClock);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "InvertBitClock     %d\n", AUD_InitParams.PCMOutParams.InvertBitClock);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "DAC Format         %d\n", AUD_InitParams.PCMOutParams.Format);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "DAC Precision      %d\n", AUD_InitParams.PCMOutParams.Precision);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "Alignment          %d\n", AUD_InitParams.PCMOutParams.Alignment);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "MSBFirst           %d\n", AUD_InitParams.PCMOutParams.MSBFirst);
            STTBX_Print(( AUD_Msg ));
        }
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    sprintf(AUD_Msg," Aud Init is over[%d]\n",RetErr);
     STTBX_Print(( AUD_Msg ));
    return ( API_EnableError ? RetErr : FALSE );
}


/*-------------------------------------------------------------------------
 * Function : AUD_Term
 *            Terminate the audio decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_Term( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    char LocalDeviceName[40];
    char DefaultName[40] = DEFAULT_AUD_NAME;   /*make into a #define for modularity*/
    STAUD_TermParams_t TermParams;
    long LVar;
    boolean RetErr;
    Init_p *SearchPointer = NULL;
    Init_p ResultPointer = NULL;
	 U8 DeviceId;

	 RetErr = cget_integer( pars_p, 0,(long *) &LVar );

	DeviceId = (U8)LVar;

    /* get device name */
    RetErr = cget_string( pars_p, DefaultName, LocalDeviceName, sizeof(LocalDeviceName) );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected DeviceName" );
    }
    /* get term param */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected ForceTerminate" );
        }
        TermParams.ForceTerminate = (U32)LVar;
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        LocalDACConfiguration.Initialized = FALSE;
        sprintf( AUD_Msg, "STAUD_Term(%s,%d) : ",AUD_DeviceName[DeviceId] , TermParams.ForceTerminate );

#ifdef ST_OSLINUX
        ErrCode = STAUD_Term( AUD_DeviceName[DeviceId], &TermParams );
#else
        GetPartitionFreeSpace( "Free space before term", DriverPartition_p );
        ErrCode = STAUD_Term( AUD_DeviceName[DeviceId], &TermParams );
        GetPartitionFreeSpace( "Free space after term", DriverPartition_p );
#endif
    }

/*--------------remove structure from the global linked list------------------*/

    if( ErrCode == ST_NO_ERROR )               /* remove Init structure */
    {
        /******find correct structure *******/
        SearchPointer = &Init_head;             /*initialise pointers*/
        ResultPointer = NULL;

        while( *SearchPointer != NULL )         /*loop*/
        {
            if( strcmp((*SearchPointer)->DeviceName, LocalDeviceName) == 0 )
            {
                ResultPointer = *SearchPointer;
                STTBX_Report(( STTBX_REPORT_LEVEL_INFO,
                               "Found correct devicename\n" ));
                break;
            }
            else
            {
                SearchPointer = &((*SearchPointer)->next);
            }
        }

        /*check if it was found*/
        if( ResultPointer == NULL )
        {
            STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                           "This device was not initialised in the test harness\n" ));
        }
        else
        {
            /* remove it, restore Head pointer */
            *SearchPointer = (*SearchPointer)->next;
            STOS_MemoryDeallocate( SystemPartition_p, ResultPointer );
        }
    }

/*-----------------------------report errors----------------------------*/

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
        assign_integer(result_sym_p, (long)ErrCode, false);
    }

    sprintf(AUD_Msg," Aud Term is over[%d]\n",RetErr);
     STTBX_Print(( AUD_Msg ));
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_Term */


/*-------------------------------------------------------------------------
 * Function : AUD_Open
 *            Share physical decoder usage
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_Open( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_OpenParams_t OpenParams;
    char LocalDeviceName[40];
    char DefaultDeviceName[40] = DEFAULT_AUD_NAME;
    long LVar;
	U8 DeviceId;
    boolean RetErr;
    Init_p SearchPointer=NULL;
    Init_p ResultPointer=NULL;
    Open_p new=NULL;
    Open_p WorkPointer=NULL;

    ErrCode = ST_NO_ERROR;


	 RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	 if( RetErr == TRUE )
	 {
		tag_current_line( pars_p, "expected DeviceId" );
	 }
	 DeviceId = (U8)LVar;

    RetErr = cget_string( pars_p, DefaultDeviceName, LocalDeviceName, sizeof(LocalDeviceName) );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected DeviceName" );
    }

    if ( RetErr == TRUE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected SyncDelay" );
        }
        else
        {
            OpenParams.SyncDelay=(U32)LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        ErrCode = STAUD_Open( AUD_DeviceName[DeviceId], &OpenParams, &AUD_Handle[DeviceId]);
    }

/*--------------add structure to the linked list------------------*/
    if( ErrCode == ST_NO_ERROR )
    {
        /*get appropriate Init - maybe this should be in a separate function*/
        SearchPointer = Init_head;             /*initialise pointers*/
        ResultPointer = NULL;

        while( SearchPointer != NULL )
        {
            if( strcmp(SearchPointer->DeviceName, LocalDeviceName) == 0 )
            {
                ResultPointer = SearchPointer;
                STTBX_Report(( STTBX_REPORT_LEVEL_INFO,
                               "Found correct devicename, storing handle\n" ));
                break;
            }
            else
            {
                SearchPointer = SearchPointer->next;
            }
        }

        if( ResultPointer == NULL )
        {
            STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                           "This device was not initialised in the test harness\n" ));
        }
        else                                /* create new Open structure */
        {
            new = (Open_p)STOS_MemoryAllocate( SystemPartition_p, sizeof(Open_t));
            new -> next = NULL;
            new -> DecoderHandle = AudDecHndl0;

            if(ResultPointer -> Open_head == NULL)                /* list empty */
            {
                ResultPointer -> Open_head = new;
            }
            else
            {
                WorkPointer = ResultPointer -> Open_head;
                while( WorkPointer->next != NULL )
                {
                    WorkPointer = WorkPointer->next;
                }
                WorkPointer->next = new;
            }
        }
    }

    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "STAUD_Open(%s,%d,%u) : ", LocalDeviceName, OpenParams.SyncDelay, (U32)AudDecHndl0 );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);

#ifdef DVD_SERVICE_DIRECTV
    if( RetErr == FALSE )
    {
        /* need to set BroadcastProfile to get correct CLKRV conversion factor */
		ErrCode = STAUD_IPSetBroadcastProfile(AUD_Handle[DeviceId], STAUD_OBJECT_INPUT_CD0,
                                              STAUD_BROADCAST_DIRECTV);
        sprintf( AUD_Msg, "STAUD_DRSetBroadcastProfile(STAUD_BROADCAST_DIRECTV) : ");
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
#endif

    return ( API_EnableError ? RetErr : FALSE );
} /* end AUD_Open */

/*-------------------------------------------------------------------------
 * Function : AUD_Close
 *            Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_Close( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    boolean RetErr;
    Init_p SearchPointer=NULL;
    Open_p *SearchPointer2=NULL;
    Open_p ResultPointer2=NULL;
    long LVar;
	 U8 DeviceId;

	 RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	 if(RetErr == TRUE)
	 {
		tag_current_line( pars_p, "expected DeviceId" );
	 }

	 DeviceId = (U8)LVar;

    /*----------------------- call STAUD_Close ------------------------*/
    sprintf( AUD_Msg, "STAUD_Close(%u) : ", (U32)AUD_Handle[DeviceId] );
    ErrCode = STAUD_Close( AUD_Handle[DeviceId] );


    /* ---------------update linked list structure -----------------*/
    if( ErrCode == ST_NO_ERROR )
    {
        /* first search through every single Open until handle is found*/
        SearchPointer = Init_head;             /*initialise pointers*/

        while( SearchPointer != NULL )         /* outer loop for inits*/
        {
            SearchPointer2 = &(SearchPointer->Open_head);
            while (*SearchPointer2 != NULL )    /* inner loop for opens*/
            {
                if( (*SearchPointer2)->DecoderHandle == AudDecHndl0 )
                {
                    ResultPointer2 = *SearchPointer2;
                    break;
                }
                else
                {
                    SearchPointer2 = &( (*SearchPointer2)->next );
                }
            }

            if( ResultPointer2 == NULL )
            {
                SearchPointer = SearchPointer -> next;
            }
            else
            {
                *SearchPointer2 = (*SearchPointer2)->next;
                STOS_MemoryDeallocate( SystemPartition_p, ResultPointer2 );

                STTBX_Report(( STTBX_REPORT_LEVEL_INFO,
                               "Found correct handle\n" ));
                break;
            }
        }

        if( ResultPointer2 == NULL )
        {
            STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                           "Handle not found\n" ));
        }
    }
    /*---------------------report Results, Errors------------------- */

    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
} /* end AUD_Close */


/*-------------------------------------------------------------------------
 * Function : AUD_DRPause
 *            Stop the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_DRPause( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	STAUD_Fade_t Fade;
	boolean RetErr;
	long LVar;
	U8 DeviceId;
	RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

	DeviceId = (U8)LVar;
	ErrCode = STAUD_DRPause(AUD_Handle[DeviceId],STAUD_OBJECT_INPUT_CD0,&Fade );
    sprintf( AUD_Msg, "STAUD_DRPause(%u) : ", (U32)AUD_Handle[DeviceId]);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRPause */

/*-------------------------------------------------------------------------
 * Function : AUD_DRResume
 *            Resume the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_DRResume( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
	boolean RetErr;
	long LVar;
	U8 DeviceId;
	RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

	DeviceId = (U8)LVar;
    ErrCode = STAUD_DRResume( AUD_Handle[DeviceId],STAUD_OBJECT_INPUT_CD0);
    sprintf( AUD_Msg, "STAUD_DRResume(%u) : ", (U32)AUD_Handle[DeviceId] );
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRResume */

/*-------------------------------------------------------------------------
 * Function : AUD_STPause
 *            Stop the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_STPause( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	STAUD_Fade_t Fade;
	boolean RetErr;
	long LVar;
	U8 DeviceId;
	RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

	DeviceId = (U8)LVar;
    ErrCode = STAUD_Pause(AUD_Handle[DeviceId],&Fade );
    sprintf( AUD_Msg, "STAUD_Pause(%u) : ", (U32)AUD_Handle[DeviceId]);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRPause */

/*-------------------------------------------------------------------------
 * Function : AUD_STResume
 *            Resume the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_STResume( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
	boolean RetErr;
	long LVar;
	U8 DeviceId;
	RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

	DeviceId = (U8)LVar;
    ErrCode = STAUD_Resume( AUD_Handle[DeviceId]);
    sprintf( AUD_Msg, "STAUD_Resume(%u) : ", (U32)AUD_Handle[DeviceId] );
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRResume */

/*-------------------------------------------------------------------------
 * Function : AUD_PauseSynchro
 *            Pause the decoder for the indicated number of blocks
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_PauseSynchro( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;

    RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Number of blocks to pause expected");
    }
    else
    {

        ErrCode = STAUD_IPPauseSynchro( AudDecHndl0,
                                      STAUD_OBJECT_DECODER_COMPRESSED1,
                                      (U32)LVar );
        sprintf( AUD_Msg, "STAUD_DRPauseSynchro(%u,%u,%u) : ",
                (U32)AudDecHndl0, STAUD_OBJECT_DECODER_COMPRESSED1, (U32)LVar);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PauseSynchro */

/*-------------------------------------------------------------------------
 * Function : AUD_SkipSynchro
 *            Skip forward the indicated number of frames
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_SkipSynchro( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;

    RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Number of frames to skip expected");
    }
    else
    {
        ErrCode = STAUD_IPSkipSynchro( AudDecHndl0,
                                      STAUD_OBJECT_DECODER_COMPRESSED1,
                                      (U32)LVar );
        sprintf( AUD_Msg, "STAUD_DRSkipSynchro(%u,%u,%u) : ",
                (U32)AudDecHndl0, STAUD_OBJECT_DECODER_COMPRESSED1, (U32)LVar);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_SkipSynchro */

/*-------------------------------------------------------------------------
 * Function : AUD_PPSetSpeakerEnable (OP to PP)
 *            Defines the state of the speakers
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPSetSpeakerEnable( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;
    STAUD_SpeakerEnable_t SpeakerOn;
    U8 DeviceId;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    /* Default to stereo output */
    SpeakerOn.Left = TRUE;
    SpeakerOn.Right = TRUE;
    SpeakerOn.LeftSurround = FALSE;
    SpeakerOn.RightSurround = FALSE;
    SpeakerOn.Center = FALSE;
    SpeakerOn.Subwoofer = FALSE;



    RetErr = cget_integer( pars_p, 0, &LVar );

    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   DeviceId = (U8)LVar;




    /* Parse command parameters */
    RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Left speaker active expected 0 or 1");
    }
    else
    {
        SpeakerOn.Left = (BOOL) LVar;

        RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right speaker active expected 0 or 1");
        }
        else
        {
            SpeakerOn.Right = (BOOL) LVar;

            RetErr = cget_integer( pars_p, FALSE, (long *)&LVar );
            if ( RetErr == TRUE )
            {
                tag_current_line( pars_p, "Left Surround speaker active expected 0 or 1");
            }
            else
            {
                SpeakerOn.LeftSurround = (BOOL) LVar;

                RetErr = cget_integer( pars_p, FALSE, (long *)&LVar );
                if ( RetErr == TRUE )
                {
                    tag_current_line( pars_p, "Right Surround speaker active expected 0 or 1");
                }
                else
                {
                    SpeakerOn.RightSurround = (BOOL) LVar;

                    RetErr = cget_integer( pars_p, FALSE, (long *)&LVar );
                    if ( RetErr == TRUE )
                    {
                        tag_current_line( pars_p, "Center speaker active expected 0 or 1");
                    }
                    else
                    {
                        SpeakerOn.Center = (BOOL) LVar;

                        RetErr = cget_integer( pars_p, FALSE, (long *)&LVar );
                        if ( RetErr == TRUE )
                        {
                            tag_current_line( pars_p, "Subwoofer speaker active expected 0 or 1");
                        }
                        else
                        {
                            SpeakerOn.Subwoofer = (BOOL) LVar;
                        }
                    }
                }
            }
        }
    }
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPMULTI, OBJ_OPSPDIF\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    /* Only setup speakers if all params are ok */
    if (RetErr == FALSE)
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_PPSetSpeakerEnable(%u, %s, %u,%u,%u,%u,%u,%u) : ", (U32)AudDecHndl0,
                 OPObject,
                 SpeakerOn.Left, SpeakerOn.Right,
                 SpeakerOn.LeftSurround, SpeakerOn.RightSurround,
                 SpeakerOn.Center, SpeakerOn.Subwoofer);

        ErrCode = STAUD_PPSetSpeakerEnable( AudDecHndl0, OutputObject, &SpeakerOn );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetSpeakerEnable */


/*-------------------------------------------------------------------------
 * Function : AUD_SetSpeed
 *            Sets the trick mode speed
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_SetSpeed( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    S32 Speed;

    /* Default speed is x1 forward */
    RetErr = cget_integer( pars_p, 100, (long *)&Speed );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Speed expected");
    }

    if (RetErr == FALSE)
    {
        sprintf( AUD_Msg, "STAUD_DRSetSpeed(%u,%d) : ", (U32)AudDecHndl0, Speed );
        ErrCode = STAUD_DRSetSpeed(AudDecHndl0,
                                   STAUD_OBJECT_DECODER_COMPRESSED1,
                                   Speed);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_SetSpeed */


/*-------------------------------------------------------------------------
 * Function : AUD_PPGetSpeaker
 *            Show the state of the speakers
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPGetSpeaker( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_SpeakerEnable_t SpeakerEnable;
    long LVar;
    U8 DeviceId;
    SpeakerEnable.Left = FALSE;
    SpeakerEnable.Right = FALSE;
    SpeakerEnable.LeftSurround = FALSE;
    SpeakerEnable.RightSurround = FALSE;
    SpeakerEnable.Center = FALSE;
    SpeakerEnable.Subwoofer = FALSE;


    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;



    RetErr = TRUE;
    sprintf( AUD_Msg, "STAUD_OPGetSpeakerEnable(%u) : ", (U32)AUD_Handle[DeviceId]);
    ErrCode = STAUD_PPGetSpeakerEnable(AUD_Handle[DeviceId],STAUD_OBJECT_POST_PROCESSOR0, &SpeakerEnable);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        if (SpeakerEnable.Left         == TRUE)
        {
            strcat( AUD_Msg, " Left" );
        }
        if (SpeakerEnable.Right         == TRUE)
        {
            strcat( AUD_Msg, " Right" );
        }

        if (SpeakerEnable.LeftSurround  == TRUE)
        {
            strcat( AUD_Msg, " LeftSurround" );
        }
        if (SpeakerEnable.RightSurround == TRUE)
        {
            strcat( AUD_Msg, " RightSurround" );
        }
        if (SpeakerEnable.Center        == TRUE)
        {
            strcat( AUD_Msg, " Center" );
        }
        if (SpeakerEnable.Subwoofer     == TRUE)
        {
            strcat( AUD_Msg, " Subwoofer" );
        }
        strcat( AUD_Msg, "\n" );
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPGetSpeaker */


/*-------------------------------------------------------------------------
 * Function : AUD_DRSetStereoOutput
 *            Set the type of the output
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetStereoOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;
    STAUD_StereoMode_t StereoMode;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    	U8 DeviceId;

    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPPCMP0, OBJ_OPPCMP0\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }





    /* get mode (default : STEREO) */
    RetErr = cget_integer( pars_p, STAUD_STEREO_MODE_STEREO, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected MONO|STEREO|PROLOGIC|DLEFT|DRIGHT|DMONO|SECSTE");
    }
    StereoMode = (STAUD_StereoMode_t)LVar;

    sprintf( AUD_Msg, "STAUD_PPSetStereoMode(%u,%d) : ", (U32)AUD_Handle[DeviceId], StereoMode );
    ErrCode = STAUD_DRSetStereoMode( AUD_Handle[DeviceId],OutputObject, StereoMode );
    StereoEffect = StereoMode;

    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetStereoOutput */

/*-------------------------------------------------------------------------
 * Function : AUD_DRGetStereoOutput
 *            Get the type of the output
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRGetStereoOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_StereoMode_t StereoMode;
    long LVar;
    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_PCMP0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPPCMP0, OBJ_OPPCMP0\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }






    sprintf( AUD_Msg, "STAUD_PPGetStereoMode(%u) : ", (U32)AUD_Handle[DeviceId]);
    ErrCode = STAUD_DRGetStereoMode( AUD_Handle[DeviceId],OutputObject, &StereoMode );
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        switch ( StereoMode )
        {
          case STAUD_STEREO_MODE_STEREO:
            sprintf( AUD_Msg, "%s Stereo mode \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_PROLOGIC:
            sprintf( AUD_Msg, "%s Stereo prologic mode \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_DUAL_LEFT:
            sprintf( AUD_Msg, "%s Dual left mode \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_DUAL_RIGHT:
            sprintf( AUD_Msg, "%s Dual right mode \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_DUAL_MONO:
            sprintf( AUD_Msg, "%s Dual Mono \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_SECOND_STEREO:
            sprintf( AUD_Msg, "%s Second Stereo Mode \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_AUTO:
            sprintf( AUD_Msg, "%s mode Auto \n", AUD_Msg);
            break;

          default:
            sprintf( AUD_Msg, "%s Not handled Mode !!!\n", AUD_Msg);

        }
    }
    STTBX_Print(( AUD_Msg ));
    if(StereoEffect== StereoMode)
    {
    sprintf( AUD_Msg, "%s Stereo Effect-Pass \n", AUD_Msg);
    STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetStereoOutput */

/*-------------------------------------------------------------------------
 * Function : AUD_PPGetAttenuation

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPGetAttenuation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Attenuation_t Attenuation;
    boolean RetErr;
    long LVar;
    STAUD_Object_t OutPutObject;
    ObjectString_t OPObject;
    U8 DeviceId;

    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if (RetErr == false)
    {
    RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected Object: OBJ_PP0 or OBJ_PP1 or OBJ_PP2\n" );
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        OutPutObject = (STAUD_Object_t)LVar;
        ObjectToText( OutPutObject, &OPObject );
    }
   }
    sprintf( AUD_Msg, "STAUD_PPGetAttenuation(%u)) : ",(U32)AUD_Handle[DeviceId]);
    ErrCode = STAUD_PPGetAttenuation( AUD_Handle[DeviceId],OutPutObject, &Attenuation);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s Left %d, Right %d, "
                 "LSurr %d, RSurr %d, Center %d, Sub %d Output %s\n", AUD_Msg,
                 Attenuation.Left, Attenuation.Right,
                 Attenuation.LeftSurround, Attenuation.RightSurround,
                 Attenuation.Center, Attenuation.Subwoofer, OPObject);
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPGetAttenuation */



/*-------------------------------------------------------------------------
 * Function : AUD_PPCompareAttenuation

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPCompareAttenuation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Attenuation_t Attenuation;
    boolean RetErr;
    long LVar;
    STAUD_Object_t OutPutObject;
    ObjectString_t OPObject;

    U8 DeviceId;

    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;


    RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected Object: OBJ_PP0 or OBJ_PP1\n" );
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        OutPutObject = (STAUD_Object_t)LVar;
        ObjectToText( OutPutObject, &OPObject );
    }

    sprintf( AUD_Msg, "Device ID(%u)) : ",(U32)AUD_Handle[DeviceId]);
    ErrCode = STAUD_PPGetAttenuation( AUD_Handle[DeviceId],OutPutObject, &Attenuation);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s Left %d, Right %d, "
                 "LSurr %d, RSurr %d, Center %d, Sub %d Output %s\n", AUD_Msg,
                 Attenuation.Left, Attenuation.Right,
                 Attenuation.LeftSurround, Attenuation.RightSurround,
                 Attenuation.Center, Attenuation.Subwoofer, OPObject);
    }
    STTBX_Print(( AUD_Msg ));

  if(LeftAttenuation ==  Attenuation.Left)
    {
    sprintf( AUD_Msg, "%sPass--Left Atten\n ",AUD_Msg) ;
    STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%sFail--Left Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }



    if(RightAttenuation ==  Attenuation.Right)
    {
    	sprintf( AUD_Msg, "%sPass--Right Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%sFail--Right Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }

      if(LeftSurroundAttenuation ==  Attenuation.LeftSurround)
    {
    	sprintf( AUD_Msg, "%sPass--Left Surround Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%sFail--Left Surround Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }


      if(RightSurroundAttenuation ==  Attenuation.RightSurround)
    {
    	sprintf( AUD_Msg, "%sPass--Right Surround Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%sFail--Right Surround Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }


      if(CenterAttenuation == Attenuation.Center)
      {
    	sprintf( AUD_Msg, "%sPass--Centre Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
      }
      else
      {
    	sprintf( AUD_Msg, "%sFail--Centtre Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
      }

       if(SubwooferAttenuation ==  Attenuation.Subwoofer)
       {
    	sprintf( AUD_Msg, "%sPass--Subwoofer Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
       }
       else
      {
    	sprintf( AUD_Msg, "%sFail--Subwoofer Atten\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
      }





    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPGetAttenuation */




/*-------------------------------------------------------------------------
 * Function : AUD_PPSetAttenuation

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPSetAttenuation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Attenuation_t Attenuation;
    long LVar;
    boolean RetErr;


    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, 0, &LVar );

    if ( RetErr == TRUE )
	{
	      tag_current_line( pars_p, "expected deviceId" );
	 }
    DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_PP0, OBJ_PP0\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    /* get attenuation Left */
    RetErr = cget_integer( pars_p, 0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Left front speaker expected Attenuation" );
    }
    Attenuation.Left = (U16)LVar;

    /* get attenuation Right */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right front speaker expected Attenuation" );
        }
        Attenuation.Right = (U16)LVar;
    }

    /* get attenuation LeftSurround */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Left surround speaker expected Attenuation" );
        }
        Attenuation.LeftSurround = (U16)LVar;
    }

    /* get attenuation RightSurround */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right surround speaker expected Attenuation" );
        }
        Attenuation.RightSurround = (U16)LVar;
    }

    /* get attenuation Center */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Center speaker expected Attenuation" );
        }
        Attenuation.Center = (U16)LVar;
    }

    /* get attenuation Subwoofer */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Subwoofer speaker expected Attenuation" );
        }
        Attenuation.Subwoofer = (U16)LVar;
    }

    /* Get Output object */
    /*
    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected Object: OBJ_OPMULTI or OBJ_OPDAC\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            OutPutObject = (STAUD_Object_t)LVar;
            ObjectToText( OutPutObject, &OPObject );
        }
    }
*/
    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        sprintf( AUD_Msg, "STAUD_OPSetAttenuation(%u,%s,%d,%d,%d,%d,%d,%d) : ",
			(U32)AUD_Handle[DeviceId],OPObject,Attenuation.Left, Attenuation.Right,
            Attenuation.LeftSurround, Attenuation.RightSurround,
            Attenuation.Center, Attenuation.Subwoofer);
        /* set parameters and start */
        ErrCode = STAUD_PPSetAttenuation( AUD_Handle[DeviceId],OutputObject,&Attenuation);
        LeftAttenuation = Attenuation.Left;
        RightAttenuation = Attenuation.Right;
        CenterAttenuation = Attenuation.Center;
        LeftSurroundAttenuation = Attenuation.LeftSurround;
        RightSurroundAttenuation = Attenuation.RightSurround;
        SubwooferAttenuation = Attenuation.Subwoofer;

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_SetAttenuation */
/*-------------------------------------------------------------------------
 * Function : AUD_OPSetDigitalOutput

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_OPSetDigitalOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    long LVar;
    boolean RetErr;

    U8 DeviceId;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   DeviceId = (U8)LVar;

   if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_SPDIF0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPSPDIF0\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    /* Get SPDIF is DISABLE or ENABLE */
    RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );


    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected TRUE | FALSE" );
    }



    if(FALSE == LVar)
    {
        sprintf( AUD_Msg, "STAUD_OPDisable(%u,OutPut:%d)) : ",
            (U32)AUD_Handle[DeviceId], STAUD_OBJECT_OUTPUT_SPDIF0);
        ErrorCode = STAUD_OPSetDigitalMode (AUD_Handle[DeviceId], OutputObject,STAUD_DIGITAL_MODE_NONCOMPRESSED);
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
        return ( API_EnableError ? RetErr : FALSE );
    }
    else if (TRUE == LVar)
    {
        sprintf( AUD_Msg, "STAUD_OPEnable(%u,OutPut:%d)) : ",
            (U32)AUD_Handle[DeviceId], STAUD_OBJECT_OUTPUT_SPDIF0);
        ErrorCode = STAUD_OPSetDigitalMode (AUD_Handle[DeviceId], OutputObject, STAUD_DIGITAL_MODE_COMPRESSED);
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
    }

    STTBX_Print(( AUD_Msg ));

    assign_integer(result_sym_p,RetErr,false);

    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPSetDigitalOutput */
/*-------------------------------------------------------------------------
 * Function : AUD_DRSetTempo

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetTempo( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	long LVar;
	boolean RetErr;
	U8 DeviceId;
    /*STAUD_DownmixParams_t DownMixParam;*/
	STAUD_Object_t DecoderObject;    ObjectString_t OPObject;
	S32 speed;

    UNUSED_PARAMETER(result_sym_p);

	RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected TRUE | FALSE" );
	}
	sprintf( AUD_Msg, "AUD_DRSetTempo(%u,TempoApply:%d)) : ",
			(U32)AUD_Handle[DeviceId], (U16)LVar);

	speed=(U8)LVar;
	RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0/ OBJ_DRCOMP1\n");
		}
		else
		{
			DecoderObject = (STAUD_Object_t)LVar;
			ObjectToText( DecoderObject, &OPObject);
		}
	}

	ErrorCode = 	STAUD_DRSetSpeed (AUD_Handle[DeviceId],DecoderObject,speed);
	RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );

	STTBX_Print(( AUD_Msg ));

	return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRSetTempo */


/*-------------------------------------------------------------------------
 * Function : AUD_DRSetDownmix

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetDownmix( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	long LVar;
	boolean RetErr;
	U8 DeviceId;
	STAUD_DownmixParams_t DownMixParam;
	STAUD_Object_t DecoderObject;    ObjectString_t OPObject;

	RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0/ OBJ_DRCOMP1\n");
		}
		else
		{
			DecoderObject = (STAUD_Object_t)LVar;
			ObjectToText( DecoderObject, &OPObject);
		}
	}

	/* Get SPDIF is DISABLE or ENABLE */
	RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected TRUE | FALSE" );
	}
	sprintf( AUD_Msg, "STAUD_DRSetDownMix(%u,DownmixApply:%d)) : ",
			(U32)AUD_Handle[DeviceId], (U16)LVar);

	DownMixParam.Apply = (U16)LVar;
	DownMixParam.stereoUpmix = FALSE;
	DownMixParam.monoUpmix = FALSE;
	DownMixParam.meanSurround = TRUE;
	DownMixParam.secondStereo = TRUE;
	DownMixParam.normalize = TRUE;
	DownMixParam.normalizeTableIndex = 0;
	DownMixParam.dialogEnhance = FALSE;

	ErrorCode = 	STAUD_DRSetDownMix (AUD_Handle[DeviceId],DecoderObject,&DownMixParam);
	RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );

	STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p,RetErr,false);

	return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRSetDownmix */



static boolean AUD_STSetDigitalOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    long LVar = 0;
    STAUD_DigitalOutputConfiguration_t spdifOutParams;
    U8 DeviceId ;
    U8 SpdifMode ;


    RetErr = cget_integer( pars_p,0, (long *)&LVar );
    if ( RetErr == TRUE )
       {
	 tag_current_line( pars_p, "expected value of mute" );
	}
   	SpdifMode = (U8)LVar;

    RetErr = cget_integer( pars_p, 0, (long *)&LVar );

    if ( RetErr == TRUE )
       {
	 tag_current_line( pars_p, "expected deviceId" );
	}
   	DeviceId = (U8)LVar;


       /*
    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    */

    if ( RetErr == FALSE )
    {
	sprintf( AUD_Msg, "AUD_STSetDigitalOutput(%u, %d, %d, %u) : \n", (U32)AUD_Handle[DeviceId], spdifOutParams.DigitalMode,
		spdifOutParams.Copyright,spdifOutParams.Latency);
        switch(SpdifMode)
        {
         case 0:
        spdifOutParams.DigitalMode = STAUD_DIGITAL_MODE_OFF;
        ErrCode = STAUD_SetDigitalOutput( AUD_Handle[DeviceId],spdifOutParams);
        break;


        case 1:
        spdifOutParams.DigitalMode = STAUD_DIGITAL_MODE_NONCOMPRESSED;
        ErrCode = STAUD_SetDigitalOutput( AUD_Handle[DeviceId],spdifOutParams );
        break;

        case 2:
        spdifOutParams.DigitalMode = STAUD_DIGITAL_MODE_COMPRESSED;
        ErrCode = STAUD_SetDigitalOutput( AUD_Handle[DeviceId],spdifOutParams );
        break;

        default: break;


	}
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);

       return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPSetDigitalOutput */






/*-------------------------------------------------------------------------
 * Function : AUD_OPGetDigitalOutput

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_OPGetDigitalOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_OutputParams_t spdifOutParams;
    STAUD_DigitalMode_t  SPDIFMode;
    long LVar;
    U8 DeviceId;
    /* get digital output parameters  */


    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   DeviceId = (U8)LVar;

   if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_SPDIF0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPSPDIF0\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }


    ErrCode = STAUD_OPGetDigitalMode(AUD_Handle[DeviceId],OutputObject,&SPDIFMode);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s (Mode:%d, latency:%d, CopyRight: %d)) \n",
                 AUD_Msg, spdifOutParams.SPDIFOutParams.AutoLatency,
                 spdifOutParams.SPDIFOutParams.Latency,
                 spdifOutParams.SPDIFOutParams.CopyPermitted
                 );
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPGetDigitalOutput */


/*-------------------------------------------------------------------------
 * Function : AUD_STGetDigitalOutput

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STGetDigitalOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;

    STAUD_DigitalOutputConfiguration_t Mode_p;

    U8 DeviceId;
    long LVar;
	STAUD_OutputParams_t spdifOutParams;
    /*STAUD_DigitalMode_t  SPDIFMode;*/



    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;

    sprintf( AUD_Msg, "STAUD_OPGetParams(%u) : ", (U32)AUD_Handle[DeviceId]);
    /* get digital output parameters  */
    ErrCode = STAUD_GetDigitalOutput(AUD_Handle[DeviceId],&Mode_p);

    if (Mode_p.DigitalMode == STAUD_DIGITAL_MODE_COMPRESSED)
    {
     sprintf( AUD_Msg, "SPDIF = Compress Dig mode\n");
    }
    else if (Mode_p.DigitalMode == STAUD_DIGITAL_MODE_NONCOMPRESSED)
    {
     sprintf( AUD_Msg, "SPDIF = Non Compress Dig mode\n");
    }
    else if (Mode_p.DigitalMode == STAUD_DIGITAL_MODE_OFF)
    {
     sprintf( AUD_Msg, "SPDIF = OFF\n");
    }
    else
     sprintf( AUD_Msg, "SPDIF = Invalid config\n");


    STTBX_Print(( AUD_Msg ));

    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s (Mode:%d, latency:%d, CopyRight: %d)) \n", AUD_Msg,
			spdifOutParams.SPDIFOutParams.AutoLatency,
			spdifOutParams.SPDIFOutParams.Latency,
			spdifOutParams.SPDIFOutParams.CopyPermitted
			);
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPGetDigitalOutput */


/*-------------------------------------------------------------------------
 * Function : AUD_STSetPCMInputParams

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetPCMInputParams( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	boolean RetErr;
	U8 DeviceId;
	STAUD_Object_t InputObject;
	long LVar;
	STAUD_PCMInputParams_t   PCMInputParams;

	RetErr = cget_integer( pars_p, 0, &LVar );

	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;

	RetErr = cget_integer( pars_p, STAUD_OBJECT_INPUT_CD0, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "Object expected: OBJ_IPCD0\n");
	}
	else
	{
		InputObject = (STAUD_Object_t)LVar;
	}

	RetErr = cget_integer( pars_p, 44100, &LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected frequency" );
	}
	PCMInputParams.Frequency = (U32)LVar;

	RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected data precision" );
	}
	PCMInputParams.DataPrecision = (U32)LVar;

	RetErr = cget_integer( pars_p, 2, &LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected num channels" );
	}
	PCMInputParams.NumChannels = (U8)LVar;

	/* get digital output parameters  */
    ErrCode = STAUD_IPSetPCMParams (AUD_Handle[DeviceId],InputObject,&PCMInputParams);


    if( RetErr == FALSE )
    {
        sprintf(AUD_Msg,"AUD_STSetPCMInputParams (Frequency:%d, precision:%d, channels: %d) \n",
                 PCMInputParams.Frequency,
                 PCMInputParams.DataPrecision,
                 PCMInputParams.NumChannels
                 );

    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPGetDigitalOutput */

/*-------------------------------------------------------------------------
 * Function : AUD_STSetEOF

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetEOF( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	boolean RetErr;
	U8 DeviceId;
	STAUD_Object_t InputObject;
	long LVar;


	RetErr = cget_integer( pars_p, 0, &LVar );

	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;

	RetErr = cget_integer( pars_p, STAUD_OBJECT_INPUT_CD0, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "Object expected: OBJ_IPCD0\n");
	}
	else
	{
		InputObject = (STAUD_Object_t)LVar;
	}


#ifndef ST_OSLINUX
	/* Set EOF output parameters  */
    ErrCode = STAUD_IPHandleEOF (AUD_Handle[DeviceId],InputObject);
#endif

    if( RetErr == FALSE )
    {
        sprintf(AUD_Msg,"STAUD_IPHandleEOF () \n");

    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPGetDigitalOutput */

/*-------------------------------------------------------------------------
 * Function : AUD_STSetLatency

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetLatency( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	boolean RetErr;
	U8 DeviceId;
	STAUD_Object_t OutputObject;
	long LVar;
	U32	Latency;

	RetErr = cget_integer( pars_p, 0, &LVar );

	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;

	RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_PCMP0, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "Object expected: OBJ_OPPCMP0\\OBJ_OPPCMP1\\OBJ_OPSPDIF0\n");
	}
	else
	{
		OutputObject = (STAUD_Object_t)LVar;
	}

    /* get latency*/
	RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected Latency" );
	}
	Latency = (U32)LVar;

#ifndef ST_OSLINUX
	/* get digital output parameters  */
    ErrCode = STAUD_OPSetLatency (AUD_Handle[DeviceId],OutputObject,Latency);
#endif
    if( RetErr == FALSE )
    {
        sprintf(AUD_Msg,"STAUD_OPSetLatency (Latency:%d) \n",
                 Latency
                 );

    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_STSetLatency */


/*-------------------------------------------------------------------------
 * Function : AUD_DRStop
 *            Stop decoding of input stream
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL AUD_DRStop( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode = ST_NO_ERROR;
	STAUD_Stop_t StopMode;
	STAUD_Fade_t Fade;
	long LVar;
	boolean RetErr;
	STAUD_Object_t DecoderObject;
	U8 DeviceId;
	ObjectString_t DRObject;

    RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	 if(RetErr == TRUE)
	 {
		tag_current_line( pars_p, "expected DeviceId" );
	 }

	 DeviceId = (U8)LVar;

    RetErr = cget_integer( pars_p, STAUD_STOP_NOW, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected StopMode (%d=end of data,%d=now)",
            STAUD_STOP_WHEN_END_OF_DATA, STAUD_STOP_NOW);
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        StopMode = (STAUD_Stop_t)LVar;
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED1, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected Object: OBJ_DRCOMP or OBJ_DRPCM\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            DecoderObject = (STAUD_Object_t)LVar;
            ObjectToText( DecoderObject, &DRObject );
        }
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        sprintf( AUD_Msg, "STAUD_DRStop(%u,%d, %s)) :",
                 (U32)AUD_Handle[DeviceId], StopMode, DRObject);
        ErrCode = STAUD_DRStop( AUD_Handle[DeviceId], DecoderObject, StopMode, &Fade );

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );

        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRStop */


/*-------------------------------------------------------------------------
 * Function : AUD_DRSetDeEmphasis
 *            Enable or disable de-emphasis filter
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetDeEmphasis( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    long LVar;
    boolean RetErr;
    BOOL EnableFilter;
    U8 DeviceId;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    ErrCode = ST_NO_ERROR;
    /* get boolean param */
    RetErr = cget_integer( pars_p, 1, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected TRUE (1) or FALSE (0)" );
    }
    else
    {
        EnableFilter = (BOOL)LVar;
        EmphasisFilter = EnableFilter;
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        if (EnableFilter == TRUE)
        {
			sprintf( AUD_Msg, "STAUD_DRSetDeEmphasisFilter(%u) %s : ", (U32)AUD_Handle[DeviceId],OPObject );
            ErrCode = STAUD_DRSetDeEmphasisFilter(AUD_Handle[DeviceId],OutputObject, TRUE);
        }
        else
        {
            sprintf( AUD_Msg, "DeviceID(%u) : ", (U8)AUD_Handle[DeviceId] );
            ErrCode = STAUD_DRSetDeEmphasisFilter(AUD_Handle[DeviceId],OutputObject, FALSE);
        }

        EmphasisFilter = ErrCode;
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetDeEmphasis */

/*-------------------------------------------------------------------------
 * Function : AUD_DRGetDeEmphasis
 *            Returns the state of the de-emphasis filter (on or off)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRGetDeEmphasis( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    BOOL EnableFilter;
    U8 DeviceId;
    long LVar;



    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }


    ErrCode = ST_NO_ERROR;
    ErrCode = STAUD_DRGetDeEmphasisFilter( AUD_Handle[DeviceId], OutputObject, &EnableFilter );
    if(EmphasisFilter == EnableFilter)
    {
    sprintf( AUD_Msg, "EMphasis test Ok");
    STTBX_Print(( AUD_Msg ));
    }

    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "Filter Enabled: %d", EnableFilter);
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* AUD_PPGetDeEmphasis  */

/*-------------------------------------------------------------------------
 * Function : AUD_PPSetChannelDelay
              set the delay in ms for each channel
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPSetChannelDelay( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Delay_t DelayConfig;
    long LVar;
	U8 DeviceId;
    boolean RetErr;
    ObjectString_t TextObject;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    ErrCode = ST_NO_ERROR;
  	memset(&DelayConfig,0,sizeof(STAUD_Delay_t));

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_PP0, OBJ_PP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }


    RetErr = cget_integer( pars_p, 0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Left speaker delay expected.");
    }
    if (RetErr == FALSE)
    {
        DelayConfig.Left = (U16) LVar;
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.Right = (U16) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 5, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Left Surround speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.LeftSurround = (U16) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 5, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right Surround speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.RightSurround = (U16) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Center speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.Center = (U16) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        /* default no delay as indicated in the STAUD API documentation v1.2.0*/
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Subwoofer speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.Subwoofer = (U16) LVar;
        }
    }

	if (RetErr == FALSE)
    {
        /* default no delay as indicated in the STAUD API documentation v1.2.0*/
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "CsLeft speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.CsLeft = (U16) LVar;
        }
    }

	if (RetErr == FALSE)
    {
        /* default no delay as indicated in the STAUD API documentation v1.2.0*/
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "CsRight speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.CsRight = (U16) LVar;
        }
    }


    /* It is for Multipcm only */
    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        sprintf( AUD_Msg, "STAUD_PPSetDelay(%u,%s,%d,%d,%d,%d,%d,%d,%s) : ",
        (U32)AUD_Handle[DeviceId], TextObject,DelayConfig.Left, DelayConfig.Right,
        DelayConfig.LeftSurround, DelayConfig.RightSurround,
        DelayConfig.Center, DelayConfig.Subwoofer, OPObject);
        ErrCode = STAUD_PPSetDelay(AUD_Handle[DeviceId], OutputObject, &DelayConfig);

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    LeftChDelay=DelayConfig.Left;
    RightChDelay=DelayConfig.Right;
    LeftSurroundChDelay = DelayConfig.LeftSurround;
    RightSurroundChDelay = DelayConfig.RightSurround;
    CenterChDelay = DelayConfig.Center;
    SubwooferChDelay = DelayConfig.Subwoofer;
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetChannelDelay */



/*-------------------------------------------------------------------------
 * Function : AUD_DRSetCompressionMode
            set the compression mode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetCompressionMode( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    long LVar;
    boolean RetErr;
    U8 DeviceId;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
	static STAUD_CompressionMode_t Mode =0;
    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    ErrCode = ST_NO_ERROR;
    /* get boolean param */
    RetErr = cget_integer( pars_p, 1, &LVar );
    	Mode = (STAUD_CompressionMode_t)LVar;
	sprintf( AUD_Msg, "DeviceID(%u) : ", (U8)AUD_Handle[DeviceId] );
	ErrCode = STAUD_DRSetCompressionMode(AUD_Handle[DeviceId],OutputObject, Mode);

	RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
	STTBX_Print(( AUD_Msg ));

	assign_integer(result_sym_p, (long)ErrCode, false);
	return ( API_EnableError ? RetErr : FALSE );

} /* end  */
/*-------------------------------------------------------------------------
 * Function : AUD_DRSetBeepToneFrequency
              set the compression mode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetBeepToneFrequency( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	long LVar;
	boolean RetErr;

	U8 DeviceId;
	STAUD_Object_t OutputObject;    ObjectString_t OPObject;
	U32 BeepToneFreq = 0 ;
	ErrCode = ST_NO_ERROR;

	RetErr = cget_integer( pars_p, 0, &LVar );

	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
		}
		else
		{
			OutputObject = (STAUD_Object_t)LVar;
			ObjectToText( OutputObject, &OPObject);
		}
	}

	ErrCode = ST_NO_ERROR;
	/*Get Freq.*/
	RetErr = cget_integer( pars_p, 1, &LVar );

	BeepToneFreq = (U32)LVar;
	sprintf( AUD_Msg, "DeviceID(%u) : ", (U8)AUD_Handle[DeviceId] );
	ErrCode = STAUD_DRSetBeepToneFrequency(AUD_Handle[DeviceId],OutputObject, BeepToneFreq);

	RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
	STTBX_Print(( AUD_Msg ));

	assign_integer(result_sym_p, (long)ErrCode, false);
	return ( API_EnableError ? RetErr : FALSE );

} /* end  */


/*-------------------------------------------------------------------------
 * Function : AUD_DRSetBeepToneFrequency
 *            Retrieve current stream info
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_DRGetBeepToneFrequency( parse_t *pars_p, char *result_sym_p )
{
    boolean RetErr;
    ST_ErrorCode_t ErrorCode;
    long LVar;
    STAUD_Object_t DecoderObject;
    U32 BeepToneFreq = 0;
    ObjectString_t DRObject;
    U8  DeviceId;


    RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

   DeviceId = (U8)LVar;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected Object: OBJ_DRCOMP0 or OBJ_DRCOMP1 \n" );
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        DecoderObject = (STAUD_Object_t)LVar;
        ObjectToText( DecoderObject, &DRObject );
    }





    if ( RetErr == FALSE)
    {
        sprintf( AUD_Msg, "STAUD_DRGetBeepToneFrequency(%s) : ", DRObject );
        ErrorCode = STAUD_DRGetBeepToneFrequency(AUD_Handle[DeviceId], DecoderObject, &BeepToneFreq );
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
        if ( (ErrorCode == ST_NO_ERROR))
        {
            STTBX_Print(("Beep Tone Frequency = %u Hz\n",
                         BeepToneFreq ));

        }
    }

    assign_integer(result_sym_p, (long)ErrorCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}


/*-------------------------------------------------------------------------
 * Function : AUD_DRSetAudioCodingMode
              set the Output Mode configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetAudioCodingMode( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode= ST_NO_ERROR;
	long LVar;
	boolean RetErr;
	STAUD_AudioCodingMode_t AudioCodingMode_p;
    U8 DeviceId;
	STAUD_Object_t OutputObject;    ObjectString_t OPObject;

	RetErr = cget_integer( pars_p, 0, &LVar );

	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
		}
		else
		{
			OutputObject = (STAUD_Object_t)LVar;
			ObjectToText( OutputObject, &OPObject);
		}
	}


	RetErr = cget_integer( pars_p, 2, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "[0..7] integer value expected.\n");
	}

	if (RetErr == FALSE)
	{
		if ( (LVar > 11))
		{
			sprintf( AUD_Msg, "Value should be 0..7 \n");
			STTBX_Print(( AUD_Msg ));
			RetErr = TRUE;
		}
		else
		{
			AudioCodingMode_p = (STAUD_AudioCodingMode_t) LVar;
		}
	}

	if ( RetErr == FALSE ) /* if parameters are ok */
	{

	sprintf( AUD_Msg, "STAUD_DRSetAudioCodingMode(%u,%d) : \n", (U32)AUD_Handle[DeviceId], AudioCodingMode_p);
	/* set Audio Coding Output */
	ErrCode = STAUD_DRSetAudioCodingMode(AUD_Handle[DeviceId], STAUD_OBJECT_DECODER_COMPRESSED0, AudioCodingMode_p);
	RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
	STTBX_Print(( AUD_Msg ));
	} /* end if */
	assign_integer(result_sym_p, (long)ErrCode, false);
	return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRSetAudioCodingMode */
/*-------------------------------------------------------------------------
 * Function : AUD_DRSetMixerLevel
              set the Output Mode configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetMixerLevel( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode= ST_NO_ERROR;
	long LVar;
	boolean RetErr;
	U16 Mix_Level;
	U8 DeviceId;
	U16 InputId;
	STAUD_Object_t OutputObject;
	ObjectString_t OPObject;

	RetErr = cget_integer( pars_p, 0, &LVar );

	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, STAUD_OBJECT_MIXER0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "Object expected: OBJ_MIX0, OBJ_MIX1\n");
		}
		else
		{
			OutputObject = (STAUD_Object_t)LVar;
			ObjectToText( OutputObject, &OPObject);
		}
	}

	RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "0,1 integer value expected.\n");
	}
	if (RetErr == FALSE)
	{
		InputId = (U16) LVar;
	}


	RetErr = cget_integer( pars_p, 2, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "Mixing Level [0x0 - 0xFFFF] expected.\n");
	}

	if (RetErr == FALSE)
	{

		Mix_Level = (U16) LVar;
	}

	if ( RetErr == FALSE ) /* if parameters are ok */
	{

	sprintf( AUD_Msg, "STAud_SetMixerUpdateMixLevel(%u,%d) : \n", (U32)AUD_Handle[DeviceId], Mix_Level);
	/* set Audio Coding Output */

	ErrCode = STAUD_MXSetMixLevel(AUD_Handle[DeviceId], STAUD_OBJECT_MIXER0,InputId, Mix_Level);

	RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
	STTBX_Print(( AUD_Msg ));
	} /* end if */
	assign_integer(result_sym_p, (long)ErrCode, false);
	return ( API_EnableError ? RetErr : FALSE );

}




/*-------------------------------------------------------------------------
 * Function : AUD_PPGetChannelDelay
              Retrieves the delay in ms set for each channel
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPGetChannelDelay( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Delay_t DelayConfig;
    boolean RetErr;
    U8 DeviceId;
    long LVar;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_PP0, OBJ_PP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    /* It is for Multipcm only */
    /*LocalObject = STAUD_OBJECT_POST_PROCESSOR0;
    ObjectToText( LocalObject, &TextObject );*/
	sprintf( AUD_Msg, "STAUD_DRCompareChannelDynamicRange(%u  %s ]) : ", (U32)AUD_Handle[DeviceId],OPObject );

    sprintf( AUD_Msg, "STAUD_PPGetDelay(%u,%s,%d,%d,%d,%d,%d,%d,%s) : ",
    (U32)AUD_Handle[DeviceId], OPObject,DelayConfig.Left, DelayConfig.Right,
    DelayConfig.LeftSurround, DelayConfig.RightSurround,
    DelayConfig.Center, DelayConfig.Subwoofer, OPObject);

    ErrCode = STAUD_PPGetDelay(AUD_Handle[DeviceId],OutputObject, &DelayConfig );

    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetChannelDelay */




/*-------------------------------------------------------------------------
 * Function : AUD_PPCompareChannelDelay
              Compares the delay for each channel
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPCompareChannelDelay( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Delay_t DelayConfig;
    boolean RetErr;
    U8 DeviceId;
    long LVar;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_PP0, OBJ_PP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    sprintf( AUD_Msg, "STAUD_DRCompareChannelDynamicRange(%u %u]) : ", (U32)AUD_Handle[DeviceId],(U32)&OPObject );

    /* It is for Multipcm only */
    /*
    LocalObject = STAUD_OBJECT_POST_PROCESSOR0;
    ObjectToText( LocalObject, &TextObject );
    */
    sprintf( AUD_Msg, "STAUD_PPGetDelay(%u,%u,%d,%d,%d,%d,%d,%d,%u) : ",
    (U32)AUD_Handle[DeviceId], (U32)OutputObject,DelayConfig.Left, DelayConfig.Right,
    DelayConfig.LeftSurround, DelayConfig.RightSurround,
    DelayConfig.Center, DelayConfig.Subwoofer,(U32)OPObject);

    ErrCode = STAUD_PPGetDelay(AUD_Handle[DeviceId],OutputObject, &DelayConfig );

    if(LeftChDelay ==  DelayConfig.Left)
    {
    	sprintf( AUD_Msg, "%s Pass--Left Channel Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%s Fail--Left Channel Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }



    if(RightChDelay ==  DelayConfig.Right)
    {
    	sprintf( AUD_Msg, "%s Pass--Right Channel Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%s Fail--Right Channel Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }

     if(LeftSurroundChDelay ==  DelayConfig.LeftSurround)
    {
    	sprintf( AUD_Msg, "%s Pass--Right Surround Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%s Fail--Left Surround Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }


     if(RightSurroundChDelay ==  DelayConfig.RightSurround)
    {
    	sprintf( AUD_Msg, "%s Pass--Right Surround Channel Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%s Fail--Right Surround Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }

     if(CenterChDelay ==  DelayConfig.Center)
    {
    	sprintf( AUD_Msg, "%s Pass--Center Channel Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%s Fail--Center Channel Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }

     if(SubwooferChDelay ==  DelayConfig.Subwoofer)
    {
    	sprintf( AUD_Msg, "%s Pass--Subwoofer Channel Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "%s Fail--Subwoofer Delay\n ",AUD_Msg) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }



    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPCompareChannelDelay */

#if 0
/*-------------------------------------------------------------------------
 * Function : AUD_SetLowDataLevel
              set the threshhold value for low data level
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_SetLowDataLevel( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    U32 Level  = 20;
    boolean RetErr;
    STAUD_Object_t InputObject;
    ObjectString_t IPObject;
    long LVar;

    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_INPUT_CD1, (long *)&LVar );

    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: OBJ_IPCD1, OBJ_IPCD2 \n");
    }
    else
    {
        InputObject = (STAUD_Object_t)LVar;
        ObjectToText( InputObject, &IPObject);
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 20, (long *)&Level );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Data Level expected(0-100 %)");
        }
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_IPSetLowDataLevelEvent(%u,%s,%d) : ",
                                AudDecHndl0,IPObject,Level);
        /* Sel Data Level */
        ErrCode = STAUD_IPSetLowDataLevelEvent(AudDecHndl0, InputObject, Level);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    } /* end if */

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRSetDynRange */
#endif

/*-------------------------------------------------------------------------
 * Function : AUD_DRSetDynRange
              set the dynamic range configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetDynRange( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode= ST_NO_ERROR;
    /*U8 Control;*/
    long LVar;
    boolean RetErr;
    STAUD_DynamicRange_t DynamicRange;
    U8 DeviceId;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    	sprintf( AUD_Msg, "STAUD_DRGetDynamicRange(%u  %s) : ", (U32)AUD_Handle[DeviceId],OPObject );

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, 0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "[0,1] integer value expected.\n");
		}
		else
		{
			DynamicRange.Enable = (BOOL)LVar;

		}
	}

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, 0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "[0..255] integer value expected \n");
		}
		else
		{
			DynamicRange.CutFactor = (U8)LVar;

		}
	}

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, 0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "[0..255] integer value expected.\n");
		}
		else
		{
			DynamicRange.BoostFactor = (U8)LVar;

		}
	}

	if ( RetErr == FALSE ) /* if parameters are ok */
	{
		RetErr = TRUE;
		sprintf( AUD_Msg, "STAUD_DRSetDynamicRange(%u,%d) : ", (U32)AUD_Handle[DeviceId], DynamicRange.Enable);

        /* set dynamic range control configuration */
        ErrCode = STAUD_DRSetDynamicRangeControl(AUD_Handle[DeviceId], OutputObject, &DynamicRange);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRSetDynRange */
/*-------------------------------------------------------------------------
 * Function : AUD_DRGetDynRange
              Retrieves the dynamic range configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRGetDynRange( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_DynamicRange_t DynamicRange;
    U8 DeviceId;
    long LVar;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    sprintf( AUD_Msg, "STAUD_DRGetDynamicRange(%u  %s) : ", (U32)AUD_Handle[DeviceId],OPObject );
    /* get channel delay parameters  */
    ErrCode = STAUD_DRGetDynamicRangeControl(AUD_Handle[DeviceId],OutputObject, &DynamicRange);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s Enable: %d CutFactor: %d BoostFacot: %d", AUD_Msg, DynamicRange.Enable,DynamicRange.CutFactor,DynamicRange.BoostFactor);
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRGetDynRange */

/*-------------------------------------------------------------------------
 * Function : AUD_DRSetEffect
              Set the stereo effect configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetEffect( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Effect_t Effect;
    char Buff[80];
    long LVar;
    boolean RetErr;
    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    ErrCode = ST_NO_ERROR;
    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }


    RetErr = cget_integer( pars_p, STAUD_EFFECT_SRS_3D, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        sprintf( Buff,"expected effect SRS_3D (3D Effect), TRU (Trusurround), BAS (TruBASS), FOCUS (Focus), XT (TruSurXT) or NONE\n" );
        tag_current_line( pars_p, Buff );
    }
    if (RetErr == FALSE)
    {
        Effect = (STAUD_Effect_t)LVar;
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_DRSetEffect(%u, Object Id[%u], effect[%d]) : ", (U32)AUD_Handle[DeviceId],(U32)OPObject, Effect);
        ErrCode = STAUD_DRSetEffect(AUD_Handle[DeviceId], OutputObject, Effect);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetEffect */


/*-------------------------------------------------------------------------
 * Function : AUD_PPGetEffect
              Retrieves the stereo effect configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRGetEffect( parse_t *pars_p, char *result_sym_p )
{
	STAUD_Object_t OutputObject;
	ObjectString_t OPObject;
    ST_ErrorCode_t ErrCode;
    STAUD_Effect_t Effect;
    boolean RetErr;
    long LVar;
	U8 DeviceId;
    ErrCode = ST_NO_ERROR;




    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
    DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    sprintf( AUD_Msg, "STAUD_DRGetEffect(%u) obj= [%u] & efect[%u]:\n ", (U32)AUD_Handle[DeviceId], (U32)OPObject,(U32) &Effect);
    ErrCode = STAUD_DRGetEffect(AUD_Handle[DeviceId], OutputObject, &Effect);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        switch( Effect )
        {
            case STAUD_EFFECT_NONE:
                strcat( AUD_Msg, " NONE\n");
                break;
            case STAUD_EFFECT_SRS_3D:
                strcat( AUD_Msg, " SRS 3D\n");
                break;
            case STAUD_EFFECT_TRUSURROUND:
                strcat( AUD_Msg, " TRUE SURROUND\n");
                break;
            case STAUD_EFFECT_SRS_TRUBASS:
                strcat( AUD_Msg, " TRUE BASS\n");
                break;
            case STAUD_EFFECT_SRS_FOCUS:
                strcat( AUD_Msg, " TRUE FOCUS\n");
                break;
            case STAUD_EFFECT_SRS_TRUSUR_XT:
                strcat( AUD_Msg, " TRUE FOCUS\n");
                break;
            default:
                sprintf( AUD_Msg, " %s Unknown effect %d\n", AUD_Msg, Effect);
                break;
        }
    }


    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetEffect */

/*-------------------------------------------------------------------------
 * Function : AUD_DRSetSRSEffectParams
              Set the stereo effect configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetSRSEffectParams( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	char Buff[80];
	long LVar;
	boolean RetErr;
	U8 DeviceId;
	STAUD_Object_t OutputObject;    ObjectString_t OPObject;
	STAUD_EffectSRSParams_t	ParamType;
	S16 Value;
	ErrCode = ST_NO_ERROR;
	RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
		}
		else
		{
			OutputObject = (STAUD_Object_t)LVar;
			ObjectToText( OutputObject, &OPObject);
		}
	}


	RetErr = cget_integer( pars_p, STAUD_EFFECT_TSXT_HEADPHONE, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		sprintf( Buff,"expected effect type STAUD_EFFECT_FOCUS_ELEVATION , STAUD_EFFECT_FOCUS_TWEETER_ELEVATION ,STAUD_EFFECT_TRUBASS_LEVEL, STAUD_EFFECT_INPUT_GAIN ,STAUD_EFFECT_TSXT_HEADPHONE,STAUD_EFFECT_TRUBASS_SIZE ,	STAUD_EFFECT_TXST_MODE \n" );
		tag_current_line( pars_p, Buff );
	}
	if (RetErr == FALSE)
	{
		ParamType = (STAUD_EffectSRSParams_t)LVar;
	}

	RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		sprintf( Buff,"expected effect type 	0 to 0x7fff \n" );
		tag_current_line( pars_p, Buff );
	}
	if (RetErr == FALSE)
	{
		Value = (S16)LVar;
	}

	if ( RetErr == FALSE ) /* if parameters are ok */
	{
		RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_DRSetSRSEffectParams(%u, Object Id[%u], Type[%u], Value[%d]) : ", (U32)AUD_Handle[DeviceId],(U32)OPObject, (U32)ParamType,Value);
		ErrCode= STAUD_DRSetSRSEffectParams (AUD_Handle[DeviceId],OutputObject,ParamType,Value);
		RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
		STTBX_Print(( AUD_Msg ));
	} /* end if */
	assign_integer(result_sym_p, (long)ErrCode, false);
	return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetEffect */


/*-------------------------------------------------------------------------
 * Function : AUD_PPSetKaraoke
              Set the Karaoke downmix configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPSetKaraoke( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Karaoke_t Mode;
    char Buff[80];
    long LVar;
    boolean RetErr;

    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, STAUD_KARAOKE_AWARE, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        sprintf( Buff,"expected mode (%d=AWARE, %d=MUSIC, "
                 "%d=VOCAL1, %d=VOCAL2, %d=VOCALS)",
                 STAUD_KARAOKE_AWARE, STAUD_KARAOKE_MUSIC,
                 STAUD_KARAOKE_VOCAL1, STAUD_KARAOKE_VOCAL2,
                 STAUD_KARAOKE_VOCALS);
        tag_current_line( pars_p, Buff );
    }
    if (RetErr == FALSE)
    {
        Mode = (STAUD_Karaoke_t) LVar;
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_PPSetKaraoke(%u,%d) : ", (U32)AudDecHndl0, Mode);
        ErrCode = STAUD_PPSetKaraoke(AudDecHndl0, STAUD_OBJECT_POST_PROCESSOR0, Mode);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetKaraoke */


/*-------------------------------------------------------------------------
 * Function : AUD_PPGetKaraoke
              Retrieves the Karaoke downmix configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPGetKaraoke( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Karaoke_t Mode;
    boolean RetErr;

	U8 DeviceId;
	long LVar;
	RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;
    sprintf( AUD_Msg, "STAUD_PPGetKaraoke(%u) : ", (U32)AUD_Handle[DeviceId]);
    ErrCode = STAUD_PPGetKaraoke(AUD_Handle[DeviceId], STAUD_OBJECT_POST_PROCESSOR0, &Mode);

    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        switch ( Mode )
        {
          case STAUD_KARAOKE_AWARE:
            strcat( AUD_Msg, " AWARE\n");
            break;
          case STAUD_KARAOKE_MUSIC:
            strcat( AUD_Msg, " MUSIC\n");
            break;
          case STAUD_KARAOKE_VOCAL1:
            strcat( AUD_Msg, " VOCAL1\n");
            break;
          case STAUD_KARAOKE_VOCAL2:
            strcat( AUD_Msg, " VOCAL2\n");
            break;
          case STAUD_KARAOKE_VOCALS:
            strcat( AUD_Msg, " VOCALS\n");
            break;
		  case STAUD_KARAOKE_NONE:
			  strcat( AUD_Msg, " NONE \n");
            break;

          default :
            sprintf( AUD_Msg, "%s Unknown karaoke mode :%d\n", AUD_Msg, Mode);
            break;
        }
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetKaraoke */


/*-------------------------------------------------------------------------
 * Function : AUD_DRSetPrologic
              Force the prologic state of the decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRSetPrologic( parse_t *pars_p, char *result_sym_p )
{
	STAUD_Object_t OutputObject;
	ObjectString_t OPObject;
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;
	U8 DeviceId;
    boolean PrologicVal;
	ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
    DeviceId = (U8)LVar;

     if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }


    RetErr = cget_integer( pars_p, 1, (long *)&LVar );
    PrologicVal = (boolean)LVar ;

    if( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Parameter required, 0=OFF, 1=0N");
    }
    else
    {
        sprintf( AUD_Msg, "STAUD_SetPrologic(%u,Object=%u Pro=%d) : ", (U32)AUD_Handle[DeviceId],(U32)&OPObject,(int)LVar);
        ErrCode = STAUD_DRSetPrologic(AUD_Handle[DeviceId], OutputObject, (BOOL)LVar);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetPrologic */


/*-------------------------------------------------------------------------
 * Function : AUD_DRGetPrologic
              Retrieves the prologic state of the audio decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_DRGetPrologic( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL PrologicState;
    boolean RetErr;
    U8 DeviceId;
    long LVar;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
    DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    sprintf( AUD_Msg, "STAUD_GetPrologic(%u) Object(%u): ", (U32)AUD_Handle[DeviceId],(U32)ObjectToText);
    ErrCode = STAUD_DRGetPrologic(AUD_Handle[DeviceId], OutputObject, &PrologicState);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s PrologicState:%d", AUD_Msg, PrologicState);
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, PrologicState, false);
   ; assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetPrologic */


/*-------------------------------------------------------------------------
 * Function : AUD_PPSetSpeakerConfig
              set the speakers configuration (speaker size)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPSetSpeakerConfig( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_SpeakerConfiguration_t Speaker;
    long LVar;
	U8 DeviceId;
	STAUD_BassMgtType_t  BassType;
	STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;


    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
    DeviceId = (U8)LVar;

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPMULTI, OBJ_OPSPDIF\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    RetErr = cget_integer( pars_p, 2, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Front speakers size expected, 1:SMALL 2:LARGE");
    }
    if (RetErr == FALSE)
    {
        Speaker.Front = (STAUD_SpeakerType_t) LVar;
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 2, (long *)&LVar );
        if ( RetErr == TRUE )
        {
        tag_current_line( pars_p, "Center speaker size expected, 1:SMALL 2:LARGE");
        }
        if (RetErr == FALSE)
        {
            Speaker.Center = (STAUD_SpeakerType_t) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 1, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Left Surround speaker size expected, 1:SMALL 2:LARGE");
        }
        if (RetErr == FALSE)
        {
            Speaker.LeftSurround = (STAUD_SpeakerType_t) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 1, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right Surround speaker size expected, 1:SMALL 2:LARGE");
        }
        if (RetErr == FALSE)
        {
            Speaker.RightSurround = (STAUD_SpeakerType_t) LVar;
        }
    }


	if (RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, 0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "BassMgtType should be 0 to 6");
		}
		if (RetErr == FALSE)
		{
			BassType = (STAUD_BassMgtType_t )LVar;
		}
	}

	if ( RetErr == FALSE ) /* if parameters are ok */
	{
	RetErr = TRUE;

	sprintf( AUD_Msg, "STAUD_PPSetSpeakerConfig(%u,%s,%d,%d,%d,%d) : ",
	(U32)AUD_Handle[DeviceId], OPObject, Speaker.Front, Speaker.Center,
	Speaker.LeftSurround, Speaker.RightSurround);

	ErrCode = STAUD_PPSetSpeakerConfig(AUD_Handle[DeviceId],OutputObject, &Speaker, BassType);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    } /* end if */

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetSpeakerConfig */


/*-------------------------------------------------------------------------
 * Function : AUD_PPGetSpeakerConfig
              Retrieves the speakers size configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_PPGetSpeakerConfig( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_SpeakerConfiguration_t Speaker;
    boolean RetErr;
    long LVar;
    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
    if ( RetErr == TRUE )
	{
	      tag_current_line( pars_p, "expected deviceId" );
	 }
   DeviceId = (U8)LVar;

   if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPPCMP0, OBJ_OPPCMP1\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    sprintf( AUD_Msg, "STAUD_PPGetSpeakerConfiguration(%u) : ", (U32)AUD_Handle[DeviceId]);

    ErrCode = STAUD_PPGetSpeakerConfig(AUD_Handle[DeviceId], OutputObject, &Speaker);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {

        sprintf( AUD_Msg, "%s (Front:%d, Center:%d, LSurr:%d, RSurr:%d\n",
                 AUD_Msg, Speaker.Front, Speaker.Center,
                 Speaker.LeftSurround, Speaker.RightSurround);

    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetSpeakerConfig */


/*#######################################################################*/
/*########################## MISC FUNCTIONS #############################*/
/*#######################################################################*/
/*-------------------------------------------------------------------------
 * Function : LoadStream
 *            Load a mpeg file in memory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/

AUD_InjectTask_t *InjectTask_p;

unsigned char *UserBufferStart_p;
unsigned char *UserBufferStop_p;
unsigned char *UserBufferCurrent_p;
#define PES_INJECT_SIZE     2048 /*2k Size */
#define FILE_BLOCK_SIZE 102400


static boolean LoadStream(char *FileName, U32 DriverId, BOOL FileIsByteReversed)
{
    U8    *FILE_Buffer_Aligned;
	U16   *FILE_Buffer_Aligned1;
	STEVT_OpenParams_t            EVT_OpenParams;
	STEVT_DeviceSubscribeParams_t EVT_SubcribeParams;
	ST_ErrorCode_t                ErrCode=ST_NO_ERROR;
	U8 DeviceId = DriverId;
	U32  tmp,Index ;



	/* If the injecter is already in place, go out */
	/* ------------------------------------------- */

	if (AUDi_InjectContext[DeviceId]!=NULL)
	{
		AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Injector %d  is already running !!!\n",DeviceId));
		return(ST_ERROR_ALREADY_INITIALIZED);
	}
	else
	{
        AUDi_InjectContext[DeviceId]=STOS_MemoryAllocate(SystemPartition_p,sizeof(AUD_InjectContext_t));
		if(AUDi_InjectContext[DeviceId]==NULL)
		{
			STTBX_Print(("Memory for Injection not allocated\n"));
			return TRUE;
		}
	}
	AUDi_InjectContext[DeviceId]->LoadFlag = FALSE;
	strcpy(AUDi_InjectContext[DeviceId]->FileName,FileName);
	AUDi_InjectContext[DeviceId]->InjectorId     = DeviceId;
	/* Check presence of the file */
	/* -------------------------- */
	#ifdef ST_OS21
    AUDi_InjectContext[DeviceId]->FileHandle=(void *)SYS_FOpen((char *)FileName,"rb");
    #else
    AUDi_InjectContext[DeviceId]->FileHandle=(void *)fopen((char *)FileName,"rb");
    #endif
	if (AUDi_InjectContext[DeviceId]->FileHandle==NULL)
	{
		AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! File doesn't exists !!!\n",DeviceId));
		return(ST_ERROR_BAD_PARAMETER);
	}
	#ifdef ST_OS21
    SYS_FSeek(AUDi_InjectContext[DeviceId]->FileHandle,0,SEEK_END);
    AUDi_InjectContext[DeviceId]->FileSize=SYS_FTell(AUDi_InjectContext[DeviceId]->FileHandle);
    #else
    fseek(AUDi_InjectContext[DeviceId]->FileHandle,0,SEEK_END);
    AUDi_InjectContext[DeviceId]->FileSize=ftell(AUDi_InjectContext[DeviceId]->FileHandle);
    #endif

    AUDi_InjectContext[DeviceId]->BufferPtr      =(U32)(U32*) STOS_MemoryAllocate(SystemPartition_p,AUDi_InjectContext[DeviceId]->FileSize+256);
    if(AUDi_InjectContext[DeviceId]->BufferPtr!=(U32)NULL)
		{
			AUDi_InjectContext[DeviceId]->LoadFlag = TRUE;
		}
	/* Simplify injection task for DMA alignments */
	#ifdef ST_OS21
    SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
    #else
    fclose(AUDi_InjectContext[DeviceId]->FileHandle);
    #endif

	/* Select method of injection, read from disk or read from memory */
	/* -------------------------------------------------------------- */
	if (AUDi_InjectContext[DeviceId]->FileSize<=(3*1024*1024)||AUDi_InjectContext[DeviceId]->LoadFlag == TRUE)
	{
		/* This file is coming from an external file system, try to load all the file into memory */
		AUDi_InjectContext[DeviceId]->BufferSize     = AUDi_InjectContext[DeviceId]->FileSize;
		if(AUDi_InjectContext[DeviceId]->LoadFlag == FALSE)
		{
			AUDi_InjectContext[DeviceId]->BufferSize   = 3*1024*1024;
            AUDi_InjectContext[DeviceId]->BufferPtr     = (U32)(U32*)STOS_MemoryAllocate(SystemPartition_p,AUDi_InjectContext[DeviceId]->BufferSize+256);
		}
		if(!FileIsByteReversed)
		{
			FILE_Buffer_Aligned = (U8 *)((((U32)AUDi_InjectContext[DeviceId]->BufferPtr)+255)&0xFFFFFF00);
		}
		else
		{
			FILE_Buffer_Aligned1 = (U16 *)((((U32)AUDi_InjectContext[DeviceId]->BufferPtr)+255)&0xFFFFFF00);
		}

		if ((void *)AUDi_InjectContext[DeviceId]->BufferPtr==NULL)
		{
			AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to allocate memory to load the file !!!\n",DeviceId));
			return(ST_ERROR_NO_MEMORY);
		}
		/* Load the file into memory */
		#ifdef ST_OS21
        AUDi_InjectContext[DeviceId]->FileHandle=(void *)SYS_FOpen((char *)FileName,"rb");
        #else
        AUDi_InjectContext[DeviceId]->FileHandle=(void *)fopen((char *)FileName,"rb");
        #endif

		if (AUDi_InjectContext[DeviceId]->FileHandle==NULL)
		{
			AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to open the file !!!\n",DeviceId));
            STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
			return(ST_ERROR_NO_MEMORY);
		}

		if(!FileIsByteReversed)
		{

			#ifdef ST_OS21
            tmp= SYS_FRead(FILE_Buffer_Aligned,AUDi_InjectContext[DeviceId]->FileSize,1,AUDi_InjectContext[DeviceId]->FileHandle);
            SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
            #else
            tmp= fread(FILE_Buffer_Aligned,AUDi_InjectContext[DeviceId]->FileSize,1,AUDi_InjectContext[DeviceId]->FileHandle);
            fclose(AUDi_InjectContext[DeviceId]->FileHandle);
            #endif
		}

#ifdef ST_OSWINCE
		else
		{
			/* file is byte reversed. The file is read in small blocks and reversed before copying */
            {
            	U8 * Fileptr;
				static char *BlockTampon;
				static long int BytesRead;
                AUDi_InjectContext[DeviceId]->BufferSize     = 1*1024*1024;
                AUDi_InjectContext[DeviceId]->BufferPtr      =(U32)Memory_Allocate(ncache_partition_stfae,AUDi_InjectContext[DeviceId]->BufferSize+256);
   				FILE_Buffer_Aligned = (U8 *)((((U32)AUDi_InjectContext[DeviceId]->BufferPtr)+255)&0xFFFFFF00);
   				Fileptr = FILE_Buffer_Aligned;

                BlockTampon = (char *)STOS_MemoryAllocate(ncache_partition_stfae, (U32) (FILE_BLOCK_SIZE + 256));
                FILE_Buffer_Aligned = (U8 *)((((U32)BlockTampon)+255)&0xFFFFFF00);
                BytesRead = 0;
                do
                {
                    /*tmp = debugread(FileDescriptor, BlockTampon, FILE_BLOCK_SIZE );*/
                    #ifdef ST_OS21
                    tmp= SYS_FRead(FILE_Buffer_Aligned,1,FILE_BLOCK_SIZE,AUDi_InjectContext[DeviceId]->FileHandle);
                    #else
                    tmp= fread(FILE_Buffer_Aligned,1,FILE_BLOCK_SIZE,AUDi_InjectContext[DeviceId]->FileHandle);
                    #endif
                    for ( Index = 0; Index <= tmp; Index+=2 )
                    {

                        *Fileptr = *(FILE_Buffer_Aligned+Index+1);
                        Fileptr += 1;
                        *Fileptr = *(FILE_Buffer_Aligned+Index);
                        Fileptr +=1;

                    }
                    if( tmp > 0 )
                    {
                        BytesRead += tmp;
                    }
                } while (BytesRead < (AUDi_InjectContext[DeviceId]->FileSize - FILE_BLOCK_SIZE));
                #ifdef ST_OS21
                SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
                #else
                fclose(AUDi_InjectContext[DeviceId]->FileHandle);
                #endif
                STOS_MemoryDeallocate(ncache_partition_stfae, BlockTampon);

            }

		}
#else
		else
		{
			#ifdef ST_OS21
            tmp= SYS_FRead(FILE_Buffer_Aligned1,AUDi_InjectContext[DeviceId]->FileSize,1,AUDi_InjectContext[DeviceId]->FileHandle);
			SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
            #else
            tmp= fread(FILE_Buffer_Aligned1,AUDi_InjectContext[DeviceId]->FileSize,1,AUDi_InjectContext[DeviceId]->FileHandle);
			fclose(AUDi_InjectContext[DeviceId]->FileHandle);
            #endif

			for ( Index = 0 ; Index < (AUDi_InjectContext[DeviceId]->FileSize >> 1); Index++ )
			{
			__asm__("swap.b %1, %0" : "=r" (FILE_Buffer_Aligned1[Index]): "r" (FILE_Buffer_Aligned1[Index]));
			}
		}
#endif

		/* No more access to the file */
		AUDi_InjectContext[DeviceId]->FileHandle=0;
	}

	/* Else, this is a read file on the fly */
	/* ------------------------------------ */
	else
	{
		AUDi_InjectContext[DeviceId]->BufferSize     = 3*1024*1024;
        AUDi_InjectContext[DeviceId]->BufferPtr         =(U32)(U32*)STOS_MemoryAllocate(SystemPartition_p,AUDi_InjectContext[DeviceId]->BufferSize+256);
		FILE_Buffer_Aligned = (U8 *)((((U32)AUDi_InjectContext[DeviceId]->BufferPtr)+255)&0xFFFFFF00);
		if ((void *)AUDi_InjectContext[DeviceId]->BufferPtr==NULL)
		{
			AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to allocate memory to load the file !!!\n",DeviceId));
			return(ST_ERROR_NO_MEMORY);
		}
		/* Open the file */


		if(!FileIsByteReversed)
		{
			AUDi_InjectContext[DeviceId]->DoByteReverseOnFly=0;
		#ifdef ST_OS21
           AUDi_InjectContext[DeviceId]->FileHandle=(void *)SYS_FOpen((char *)FileName,"rb");
        #else
           AUDi_InjectContext[DeviceId]->FileHandle=(void *)fopen((char *)FileName,"rb");
        #endif
		if ((void *)AUDi_InjectContext[DeviceId]->FileHandle==NULL)
		{
			AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to open the file !!!\n",DeviceId));
            STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
			return(ST_ERROR_NO_MEMORY);
		}

		}
		else
		{
		AUDi_InjectContext[DeviceId]->DoByteReverseOnFly=1;
		#ifdef ST_OS21
        AUDi_InjectContext[DeviceId]->FileHandle=(void *)SYS_FOpen((char *)FileName,"rb");
        #else
        AUDi_InjectContext[DeviceId]->FileHandle=(void *)fopen((char *)FileName,"rb");
        #endif
		if ((void *)AUDi_InjectContext[DeviceId]->FileHandle==NULL)
		{
			AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to open the file !!!\n",DeviceId));
            STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
			return(ST_ERROR_NO_MEMORY);
		}

		}



	}

	/* Allocate a context */
	/* ------------------ */
	if (AUDi_InjectContext[DeviceId]==NULL)
	{
		AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to allocate memory !!!\n",DeviceId));
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL)
		#ifdef ST_OS21
        SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
        #else
        fclose(AUDi_InjectContext[DeviceId]->FileHandle);
        #endif
        STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
		return(ST_ERROR_NO_MEMORY);
	}

	/* Fill up the context */
	/* ------------------- */

	AUDi_InjectContext[DeviceId]->InjectorId          = DeviceId;

	AUDi_InjectContext[DeviceId]->BufferPtr           = (U32)AUDi_InjectContext[DeviceId]->BufferPtr;
	AUDi_InjectContext[DeviceId]->BufferBase 	   = ((((U32)AUDi_InjectContext[DeviceId]->BufferPtr)+255)&0xFFFFFF00);
	AUDi_InjectContext[DeviceId]->BufferConsumer      = AUDi_InjectContext[DeviceId]->BufferBase;
	AUDi_InjectContext[DeviceId]->BufferProducer      = AUDi_InjectContext[DeviceId]->BufferBase;

	AUDi_InjectContext[DeviceId]->BufferSize          = AUDi_InjectContext[DeviceId]->BufferSize;
	AUDi_InjectContext[DeviceId]->FileSize            = AUDi_InjectContext[DeviceId]->FileSize;
	AUDi_InjectContext[DeviceId]->FileHandle          = AUDi_InjectContext[DeviceId]->FileHandle;
	AUDi_InjectContext[DeviceId]->FilePosition        = 0;


	/* Subscribe to events */
	/* ------------------- */

	/* Subscription for EOF event from the player  */
	memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
	memset(&EVT_OpenParams    ,0,sizeof(STEVT_OpenParams_t));
	EVT_SubcribeParams.NotifyCallback=AUDi_InjectCallback;
	EVT_SubcribeParams.SubscriberData_p 	=	(void *)(AUDi_InjectContext[DeviceId]);
	ErrCode|=STEVT_Open(EVT_DeviceName[0],&EVT_OpenParams,&(AUDi_InjectContext[DeviceId]->EVT_Handle));
	ErrCode|=STEVT_SubscribeDeviceEvent(AUDi_InjectContext[DeviceId]->EVT_Handle,NULL,STAUD_EOF_EVT,&EVT_SubcribeParams);
	AUDi_InjectContext[DeviceId]->SetEOFFlag = FALSE;
	#if 0
	/* Subscribe to events */
	/* ------------------- */
	memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
	memset(&EVT_OpenParams    ,0,sizeof(STEVT_OpenParams_t));
	EVT_SubcribeParams.NotifyCallback=AUDi_InjectCallback;
	ErrCode|=STEVT_Open(EVT_DeviceName[0],&EVT_OpenParams,&(AUDi_InjectContext[DeviceId]->EVT_Handle));
	ErrCode|=STEVT_SubscribeDeviceEvent(AUDi_InjectContext[DeviceId]->EVT_Handle,AUD_DeviceName[DeviceId],STAUD_PCM_UNDERFLOW_EVT   ,&EVT_SubcribeParams);
	ErrCode|=STEVT_SubscribeDeviceEvent(AUDi_InjectContext[DeviceId]->EVT_Handle,AUD_DeviceName[DeviceId],STAUD_PCM_BUF_COMPLETE_EVT,&EVT_SubcribeParams);
	ErrCode|=STEVT_SubscribeDeviceEvent(AUDi_InjectContext[DeviceId]->EVT_Handle,AUD_DeviceName[DeviceId],STAUD_FIFO_OVERF_EVT      ,&EVT_SubcribeParams);
	ErrCode|=STEVT_SubscribeDeviceEvent(AUDi_InjectContext[DeviceId]->EVT_Handle,AUD_DeviceName[DeviceId],STAUD_CDFIFO_UNDERFLOW_EVT,&EVT_SubcribeParams);
	ErrCode|=STEVT_SubscribeDeviceEvent(AUDi_InjectContext[DeviceId]->EVT_Handle,AUD_DeviceName[DeviceId],STAUD_SYNC_ERROR_EVT      ,&EVT_SubcribeParams);
	ErrCode|=STEVT_SubscribeDeviceEvent(AUDi_InjectContext[DeviceId]->EVT_Handle,AUD_DeviceName[DeviceId],STAUD_LOW_DATA_LEVEL_EVT  ,&EVT_SubcribeParams);
	ErrCode|=STEVT_SubscribeDeviceEvent(AUDi_InjectContext[DeviceId]->EVT_Handle,AUD_DeviceName[DeviceId],STAUD_NEW_FRAME_EVT       ,&EVT_SubcribeParams);

        #endif

	if (ErrCode!=ST_NO_ERROR)
	{
		AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to subscribe to audio events !!!\n",DeviceId));
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL)
		#ifdef ST_OS21
        SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
        #else
        fclose(AUDi_InjectContext[DeviceId]->FileHandle);
        #endif
        STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
        STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]);
		AUDi_InjectContext[DeviceId]=NULL;
		return(ErrCode);
	}

    return(ErrCode);
} /* end LoadStream */


/*-------------------------------------------------------------------------
 * Function : Read Files List
 *            Load a mpeg file in memory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/


static boolean ReadFileList(char *FileName,BOOL FileIsByteReversed)
{

 ST_ErrorCode_t                ErrCode=ST_NO_ERROR;
 static long int file_size,  tmp,buffer_size;
 FILE * file_handle;

 file_handle=(void *) fopen((char *)FileName,"rb");
 if (file_handle==NULL)
  {
   AUDi_TraceERR(("**ERROR** !!! File doesn't exists !!!\n",));
   return(ST_ERROR_BAD_PARAMETER);
  }
 fseek(file_handle,0,SEEK_END);
 file_size=ftell(file_handle);

   buffer_size     = file_size;
   list_ptr         = (char *)STOS_MemoryAllocate(DriverPartition_p,buffer_size+50);


   if (list_ptr==NULL)
    {
     AUDi_TraceERR(("Unable to allocate memory to load the file !!!\n"));
     fclose(file_handle);
     return(ST_ERROR_NO_MEMORY);
    }


  if(!FileIsByteReversed)
  {
    fseek(file_handle,0,SEEK_SET);
    tmp= fread(list_ptr,file_size,1,file_handle);
    fclose(file_handle);
  }
   /* No more access to the file */
   file_handle=0;

 cur_ptr=list_ptr;
 return(ErrCode);
} /* end ReadFileList */

/*-------------------------------------------------------------------------
 * Function : AUD_Load
 *            Load a file in memory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static boolean AUD_Load(parse_t *pars_p, char *result_sym_p )
{
  boolean RetErr;
  char FileName[256];
  BOOL bMode;
  U32 DeviceId;

    RetErr = FALSE;
    /* get file name */

    memset( FileName, 0, sizeof(FileName));
    RetErr = cget_string( pars_p, "", FileName, sizeof(FileName) );
    if ( RetErr == TRUE )
    {
      tag_current_line( pars_p, "expected FileName" );
    }

    RetErr = cget_integer( pars_p, 0, &DeviceId );
    if ( RetErr == TRUE )
    {
      tag_current_line( pars_p, "expected driver Id" );
    }

    RetErr = cget_integer( pars_p, 0, (long *)&bMode );
    if ( RetErr == TRUE )
    {
      tag_current_line( pars_p, "expected 1 for reversed, 0 for normal" );
    }

    if (RetErr == FALSE)
    {
        RetErr = LoadStream(FileName, DeviceId, bMode);
	   /*store the file in global structure */

    }

    assign_integer(result_sym_p,RetErr,false);

    return(RetErr);

} /* end AUD_Load */



/*-------------------------------------------------------------------------
 * Function : AUD_Load_Dynamic
 *            Load a file in memory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static boolean AUD_Load_Dynamic(parse_t *pars_p, char *result_sym_p )
{
	boolean RetErr;
	char FileName[256],StreamName[256];
	static char Prev_FileName[256]={"\0"};
	int Index=0;
	BOOL bMode;
	U32 DeviceId;
	RetErr = FALSE;
    /* get file name */

    memset( FileName, 0, sizeof(FileName));
    RetErr = cget_string( pars_p, "", FileName, sizeof(FileName) );
    if ( RetErr == TRUE )
    {
      tag_current_line( pars_p, "expected FileName" );
    }

    RetErr = cget_integer( pars_p, 0, &DeviceId );
    if ( RetErr == TRUE )
    {
      tag_current_line( pars_p, "expected driver Id" );
    }

    RetErr = cget_integer( pars_p, 0, (long *)&bMode );
    if ( RetErr == TRUE )
    {
      tag_current_line( pars_p, "expected 1 for reversed, 0 for normal" );
    }
	if(list_ptr==NULL)
	{
		strcpy(Prev_FileName,FileName);
	}

    if(list_ptr!=NULL)
    {
    	if(*cur_ptr=='\0')
    	{
    	AUDi_TraceERR(("**ERROR** !!! file names consumed !!!\n"));
    	STOS_MemoryDeallocate(SystemPartition_p,(void *)list_ptr);
			cur_ptr=NULL;
		}
		if(strcmp(Prev_FileName,FileName)!=0&&cur_ptr!=NULL)
		{
            STOS_MemoryDeallocate(SystemPartition_p,(void *)list_ptr);
            cur_ptr=NULL;
			strcpy(Prev_FileName,FileName);
    	}
    }

    if (RetErr == FALSE&&cur_ptr==NULL)
    {
		RetErr = ReadFileList((char *)FileName,0);
	   /*store the file in global structure */
    }


    while(*cur_ptr!='\r')
    {
		StreamName[Index++]=*cur_ptr;
    	cur_ptr++;
    }
	StreamName[Index]='\0';
    cur_ptr++;cur_ptr++;

    if (RetErr == FALSE)
    {
        STTBX_Print(("%%%$-------------------------------------$\n"));
		STTBX_Print(("%%%$input file: %s$\n",StreamName));
		RetErr = LoadStream(StreamName, DeviceId, bMode);
	   /*store the file in global structure */


    }
    assign_integer(result_sym_p,RetErr,false);

    return(RetErr);

} /* end AUD_Load_Dynamic */

/*-------------------------------------------------------------------------
 * Function : AUD_StopInjection
 *            Stop Audio Inject task
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static boolean AUD_StopInjection( parse_t *pars_p, char *Result )
{
	boolean RetErr = FALSE;
	long LVar;
	U8 DeviceId;

	RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

	DeviceId = (U8)LVar;

	AUDi_InjectContext[DeviceId]->State = AUD_INJECT_STOP;
    STOS_TaskDelay((signed long)10000); /*wait to compelete the injection*/
	assign_integer(Result,0,false);
	return FALSE;
}




/*-------------------------------------------------------------------------
 * Function : AUD_KillTask
 *            Kill Audio Inject task
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static boolean AUD_KillTask( parse_t *pars_p, char *Result )
{
    boolean RetErr = FALSE;
    long LVar;
    U8 DeviceId;

    RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	 if(RetErr == TRUE)
	 {
		tag_current_line( pars_p, "expected DeviceId" );
	 }

	 DeviceId = (U8)LVar;

#ifndef ST_OSLINUX
    if ( AUDi_InjectContext[DeviceId]->Task != NULL )
    {
#endif
		while(AUDi_InjectContext[DeviceId]->State != AUD_INJECT_STOP)
		{
            STOS_TaskDelay((signed long)1000); /*wait to compelete the injection*/
		}
			/*Wait for the read-write pointer to be equal  so that parser has picked up all the injected data*/
		while(AUDi_InjectContext[DeviceId]->BufferAudioProducer != AUDi_InjectContext[DeviceId]->BufferAudioConsumer)
		{
    	    STOS_TaskDelay((signed long)1000);
		}


#ifndef ST_OSLINUX
	}
	    STOS_TaskDelete(AUDi_InjectContext[DeviceId]->Task,NULL,NULL,NULL);
		AUDi_InjectContext[DeviceId]->Task=NULL;
#else
	if ( AUDi_InjectContext[DeviceId]->BufferMapped )
    {
        STAUD_BufferParams_t AUD_BufferParams;
        ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
        AUD_BufferParams.BufferSize = AUDi_InjectContext[DeviceId]->BufferAudioSize;
        AUD_BufferParams.BufferBaseAddr_p = (void*)AUDi_InjectContext[DeviceId]->BufferAudio;
        AUDi_InjectContext[DeviceId]->BufferMapped = FALSE;
	AUDi_TraceDBG(("Buffer Map, in AudKill %d \n",AUDi_InjectContext[DeviceId]->BufferMapped));
       ErrorCode = STAUD_UnmapBufferFromUserSpace(&AUD_BufferParams);
	if (ErrorCode != ST_NO_ERROR)
	{
		STTBX_Print(("AUD_KillTask - STAUD_UnmapBufferFromUserSpace FAILED \n"));
	}
	else
	{
		STTBX_Print(("AUD_KillTask - STAUD_UnmapBufferFromUserSpace SUCCESS \n"));
	}

    }
#endif

	STOS_SemaphoreDelete(NULL,AUDi_InjectContext[DeviceId]->SemStart);
	STOS_SemaphoreDelete(NULL,AUDi_InjectContext[DeviceId]->SemStop);
	STOS_SemaphoreDelete(NULL,AUDi_InjectContext[DeviceId]->SemLock);
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7710)||defined(ST_7109))
		if(AUDi_InjectContext[DeviceId]->FdmaNode!=NULL)
			 STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->FdmaNode);
        AUDi_InjectContext[DeviceId]->FdmaNode=(U8*)NULL;
#endif
		if((void *)AUDi_InjectContext[DeviceId]->BufferPtr!=NULL)
			   STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
		AUDi_InjectContext[DeviceId]->BufferPtr=(U32)NULL;
    	if(AUDi_InjectContext[DeviceId]->FileHandle !=0)
    	{
            #ifdef ST_OS21
            SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
            #else
            fclose(AUDi_InjectContext[DeviceId]->FileHandle);
            #endif
    		AUDi_InjectContext[DeviceId]->FileHandle = 0;
    	}
		if(AUDi_InjectContext[DeviceId]!=NULL)
			STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]);
		AUDi_InjectContext[DeviceId]=NULL;
		sprintf(AUD_Msg,"Aud Kill is over[%d]\n",RetErr);
     		STTBX_Print(( AUD_Msg ));
    assign_integer(Result,0,false);

    return FALSE;
} /* end AUD_KillTask */

/*-------------------------------------------------------------------------
 * Function : AUD_KillPCMTask
 *            Kill PCM Inject task
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static boolean AUD_KillPCMTask( parse_t *pars_p, char *Result )
{
	boolean RetErr = FALSE;
    /*task_t *TaskList[1];*/
	long LVar;
    /*int InjectDest;*/
    /*ST_ErrorCode_t ErrorCode = ST_NO_ERROR;*/
	U8 DeviceId;

	RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

	DeviceId = (U8)LVar;
#ifndef ST_OSLINUX
	if ( AUDi_InjectContext[DeviceId]->Task != NULL )
	{
#else
	if ( AUDi_InjectContext[DeviceId]->PCMInjectTask != (pthread_t) NULL )
	{
#endif
		while(AUDi_InjectContext[DeviceId]->State != AUD_INJECT_STOP)
		{
            STOS_TaskDelay((signed long)1000); /*wait to compelete the injection*/
		}
	}

#ifndef ST_OSLINUX
	STOS_TaskDelete(AUDi_InjectContext[DeviceId]->Task,NULL,NULL,NULL);
	AUDi_InjectContext[DeviceId]->Task=NULL;
#else
	pthread_join(AUDi_InjectContext[DeviceId]->PCMInjectTask, NULL);
	AUDi_InjectContext[DeviceId]->PCMInjectTask = (pthread_t) NULL;
#endif

	STOS_SemaphoreDelete(NULL,AUDi_InjectContext[DeviceId]->SemStart);
	STOS_SemaphoreDelete(NULL,AUDi_InjectContext[DeviceId]->SemStop);
	STOS_SemaphoreDelete(NULL,AUDi_InjectContext[DeviceId]->SemLock);

#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7710)||defined(ST_7109))
	if(AUDi_InjectContext[DeviceId]->FdmaNode!=NULL)
	STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->FdmaNode);
    AUDi_InjectContext[DeviceId]->FdmaNode=NULL;
#endif
	if((void *)AUDi_InjectContext[DeviceId]->BufferPtr!=NULL)
	STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
	AUDi_InjectContext[DeviceId]->BufferPtr=(U32)NULL;
	if(AUDi_InjectContext[DeviceId]!=NULL)
	STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]);
	AUDi_InjectContext[DeviceId]=NULL;
	sprintf(AUD_Msg,"Aud PCM Kill is over[%d]\n",RetErr);
	STTBX_Print(( AUD_Msg ));
	assign_integer(Result,0,false);

	return FALSE;
} /* end AUD_KillPCMTask */

/*-------------------------------------------------------------------------
 * Function : PutFrame
 *            Callback function for frame delivery
 * Input    : *MemoryStart,	Size, PTS, PTS33, * clientInfo
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t PutFrame( U8 *MemoryStart,	U32 Size,	U32 PTS,	U32 PTS33, void * clientInfo)
{
	STTBX_Print(("Received Buffer=%d,%d,%d, %d\n",MemoryStart,Size,PTS,clientInfo));
	STTBX_Print(("PTS33=%u \n", PTS33));
	return ST_NO_ERROR;
}

/*-------------------------------------------------------------------------
 * Function : AUD_RegFrameProcess
 *            regiter the callback  function for frame delivery
 * Input    :  *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_RegFrameProcess(parse_t *pars_p, char *Result )
{
	 boolean RetErr = FALSE;
	 U32 DeviceId;
     ST_ErrorCode_t ErrCode;

	 RetErr = cget_integer( pars_p, 0, &DeviceId );
	 if(RetErr == TRUE)
	 {
		tag_current_line( pars_p, "expected DeviceId" );
	 }

 	 assign_integer(Result,RetErr,false);
#ifndef ST_OSLINUX
	 ErrCode=STAUD_DPSetCallback (AUDi_InjectContext[DeviceId]->AUD_Handle,STAUD_OBJECT_FRAME_PROCESSOR, (FrameDeliverFunc_t )PutFrame, NULL);
	 if(ErrCode==ST_NO_ERROR)
		 return FALSE;
	 else
	  return TRUE;
#endif

}
/*-------------------------------------------------------------------------
 * Function : AUD_OPEnableHdmi
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/

 static BOOL AUD_OPEnableHdmi( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	BOOL RetErr;
	long LVar = 0;
	U8 DeviceId = 0;

	STAUD_Object_t OutputObject;    ObjectString_t OPObject;

	RetErr = cget_integer( pars_p, 0, &LVar );

	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;


	RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_PCMP0, (long *)&LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "Object expected: OBJ_OPPCM0, OBJ_OPPCM1,OBJ_OPSPDIF0\n");
	}
	else
	{
		OutputObject = (STAUD_Object_t)LVar;
		ObjectToText( OutputObject, &OPObject);
	}


	if ( RetErr == FALSE )
	{
        sprintf( AUD_Msg, "STAUD_OPEnableHDMIOutput(%u, %u) : ", (U32)AUD_Handle[DeviceId], (U32)&OPObject);
		ErrCode = STAUD_OPEnableHDMIOutput( AUD_Handle[DeviceId], OutputObject);

		RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
		STTBX_Print(( AUD_Msg ));
	}

	assign_integer(result_sym_p, (long)ErrCode, false);
	return ( API_EnableError ? RetErr : FALSE );

}
/*-------------------------------------------------------------------------
 * Function : AUD_MemInject
 *            Inject Audio in Memory to DMA
 * Input    : *pars_p, *Result_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_MemInject( parse_t *pars_p, char *Result )
{
	boolean RetErr = FALSE;
	BOOL NotSubTask;
	U32 NbLoops;

	U32 DeviceId;
	U32 DriverId;
	U32 LVar;
	/* switch according to INJECT_DEST testtool variable */
	RetErr = cget_integer( pars_p, 1, &NbLoops );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected NbLoops" );
	}

	RetErr = cget_integer( pars_p, 0, &DeviceId );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

	RetErr = cget_integer( pars_p, 0, &DriverId );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DriverId" );
	}

	RetErr = cget_integer( pars_p, FALSE , &LVar );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected EOF" );
	}

	if(AUDi_InjectContext[DeviceId]==NULL)
	{
		STTBX_Print(("Injection Context not intialized\n"));
		return TRUE;
	}

	AUDi_InjectContext[DeviceId]->SetEOFFlag = (BOOL) LVar;
	AUDi_InjectContext[DeviceId]->NbLoops = NbLoops;

	InjectTask_p = &InjectTaskTable[DeviceId];

	cget_integer( pars_p, FALSE, (long *)&NotSubTask );
	AUDi_InjectContext[DeviceId]->NbLoopsAlreadyDone  = 0;
	AUDi_InjectContext[DeviceId]->State               = AUD_INJECT_SLEEP;

	AUDi_InjectContext[DeviceId]->AUD_Handle          = AUD_Handle[DriverId];


	/* Create the semaphores */
	/* --------------------- */

	AUDi_InjectContext[DeviceId]->SemStart = STOS_SemaphoreCreateFifo(NULL,0);
	AUDi_InjectContext[DeviceId]->SemStop = STOS_SemaphoreCreateFifo(NULL,0);
	AUDi_InjectContext[DeviceId]->SemLock = STOS_SemaphoreCreateFifo(NULL,1);
	AUDi_InjectContext[DeviceId]->SemWaitEOF = STOS_SemaphoreCreateFifo(NULL,0);


	/* Get buffer parameters for the audio */
	/* ----------------------------------- */
#if defined(ST_5105)||defined(ST_7100)||defined(ST_7109)
	{
		STAUD_BufferParams_t        AUD_BufferParams;
		STAUD_Object_t AudObject;
		switch(DeviceId)
		{
			case 0:
			AudObject = STAUD_OBJECT_INPUT_CD0;
			break;
			case 1:
			AudObject = STAUD_OBJECT_INPUT_CD1;
			break;
			default:
			AudObject = STAUD_OBJECT_INPUT_CD0;
			break;
		}
		AUDi_InjectContext[DeviceId]->InputObject = AudObject;
	RetErr|=STAUD_IPGetInputBufferParams (AUDi_InjectContext[DeviceId]->AUD_Handle,AudObject,&AUD_BufferParams);
	AUDi_TraceDBG(("AUD_BufferParams.BufferBaseAddr_p %x, size %d\n",AUD_BufferParams.BufferBaseAddr_p,
 					AUD_BufferParams.BufferSize));
#ifdef ST_OSLINUX
	AUDi_TraceDBG(("AUD_BufferParams.BufferBaseAddr_p %x, size %d\n",AUD_BufferParams.BufferBaseAddr_p,
 					AUD_BufferParams.BufferSize));
	RetErr|=STAUD_MapBufferToUserSpace(&AUD_BufferParams);
	AUDi_TraceDBG(("STAUD_MapBufferToUserSpace Error %d \n",RetErr));
	AUDi_TraceDBG(("AUD_BufferParams.BufferBaseAddr_p %x, size %d\n",AUD_BufferParams.BufferBaseAddr_p,
 					AUD_BufferParams.BufferSize));

	AUDi_InjectContext[DeviceId]->BufferMapped = TRUE;
	AUDi_TraceDBG(("Buffer Map %d \n",AUDi_InjectContext[DeviceId]->BufferMapped));
#endif
	AUDi_InjectContext[DeviceId]->BitBufferAudioSize  = 0;
	AUDi_InjectContext[DeviceId]->BufferAudioProducer = AUDi_InjectContext[DeviceId]->BufferAudioConsumer = AUDi_InjectContext[DeviceId]->BufferAudio = (U32)AUD_BufferParams.BufferBaseAddr_p;
	AUDi_InjectContext[DeviceId]->BufferAudioSize     = AUD_BufferParams.BufferSize;
	RetErr|=STAUD_IPSetDataInputInterface(AUDi_InjectContext[DeviceId]->AUD_Handle,AudObject,AUDi_GetWritePtr,AUDi_SetReadPtr,AUDi_InjectContext[DeviceId]);
	AUDi_TraceDBG(("STAUD_IPSetDataInputInterface Error %d, Aud Handle = %x \n",RetErr,AUDi_InjectContext[DeviceId]->AUD_Handle));
	}
#endif
	if (RetErr!=ST_NO_ERROR)
	{
		STTBX_Print(("AUD_MemInject - STAUD_IPSetDataInputInterface FAILED \n"));
#ifdef ST_OSLINUX
		STAUD_BufferParams_t AUD_BufferParams;
		AUD_BufferParams.BufferSize = AUDi_InjectContext[DeviceId]->BufferAudioSize;
		AUD_BufferParams.BufferBaseAddr_p = (void*)AUDi_InjectContext[DeviceId]->BufferAudio;
		AUDi_InjectContext[DeviceId]->BufferMapped = FALSE;
		AUDi_TraceDBG(("Buffer Map, Error in meminject %d \n",AUDi_InjectContext[DeviceId]->BufferMapped));
		STAUD_UnmapBufferFromUserSpace(&AUD_BufferParams);
#endif
		AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to setup the audio buffer !!!\n",DeviceId));
        if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL)
         #ifdef ST_OS21
          SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
         #else
          fclose(AUDi_InjectContext[DeviceId]->FileHandle);
         #endif
        STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
        STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]);
		AUDi_InjectContext[DeviceId]=NULL;
		return(RetErr);
	}

	/* Setup for FDMA transfer */
	/* ----------------------- */
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7710)||defined(ST_7109))
    AUDi_InjectContext[DeviceId]->FdmaNode=STOS_MemoryAllocate(SystemPartition_p,sizeof(STFDMA_Node_t)+256);
	if (AUDi_InjectContext[DeviceId]->FdmaNode==NULL)
	{
		AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to allocate a fdma node!!!\n",DeviceId));
        #ifdef ST_OS21
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL) SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
        #else
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL) fclose(AUDi_InjectContext[DeviceId]->FileHandle);
        #endif
        Memory_Deallocate(SystemPartition_p,(void*)AUDi_InjectContext[DeviceId]->BufferPtr);
        Memory_Deallocate(SystemPartition_p,AUDi_InjectContext[DeviceId]);
		AUDi_InjectContext[DeviceId]=NULL;
		return(ST_ERROR_NO_MEMORY);
	}
	AUDi_InjectContext[DeviceId]->FdmaNodeAligned=(STFDMA_Node_t *)(((U32)AUDi_InjectContext[DeviceId]->FdmaNode+255)&0xFFFFFF00);
	RetErr=STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL/*STFDMA_HIGH_BANDWIDTH_POOL*/,&AUDi_InjectContext[DeviceId]->FdmaChannelId,STFDMA_1);
	if (RetErr!=ST_NO_ERROR)
	{
		AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to lock a channel !!!\n",DeviceId));
        #ifdef ST_OS21
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL) SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
        #else
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL) fclose(AUDi_InjectContext[DeviceId]->FileHandle);
        #endif
        STOS_MemoryDeallocate(SystemPartition_p,(void*)AUDi_InjectContext[DeviceId]->BufferPtr);
        STOS_MemoryDeallocate(SystemPartition_p,AUDi_InjectContext[DeviceId]->FdmaNode);
        STOS_MemoryDeallocate(SystemPartition_p,AUDi_InjectContext[DeviceId]);
		AUDi_InjectContext[DeviceId]=NULL;
		return(ST_ERROR_NO_MEMORY);
	}
#endif


	if (RetErr!=ST_NO_ERROR)
	{
		AUDi_TraceERR(("AUD_InjectStart(%d):**ERROR** !!! Unable to start the audio decoder !!!\n",DeviceId));
        #ifdef ST_OS21
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL) SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
        #else
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL) fclose(AUDi_InjectContext[DeviceId]->FileHandle);
        #endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
        STOS_MemoryDeallocate(SystemPartition_p,AUDi_InjectContext[DeviceId]->FdmaNode);
		STFDMA_UnlockChannel(AUDi_InjectContext[DeviceId]->FdmaChannelId,STFDMA_1);
#endif
        STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
        STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]);
		AUDi_InjectContext[DeviceId]=NULL;
		return(RetErr);
	}

	/* Create the inject task */
	/* ---------------------- */
#ifdef ST_OSLINUX
	if(0)
#else

	STOS_TaskCreate(AUDi_InjectTask,AUDi_InjectContext[DeviceId],NULL,(2048+16384),NULL,NULL,&AUDi_InjectContext[DeviceId]->Task,
		NULL,MAX_USER_PRIORITY/2,"AUD_InjectorTask",0);



	if (AUDi_InjectContext[DeviceId]->Task==NULL)
#endif
	{
		STAUD_Fade_t AUD_Fade;
		AUD_Fade.FadeType =STAUD_FADE_SOFT_MUTE;
		STAUD_Stop(AUDi_InjectContext[DeviceId]->AUD_Handle,STAUD_STOP_NOW,&AUD_Fade);
        #ifdef ST_OS21
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL) SYS_FClose(AUDi_InjectContext[DeviceId]->FileHandle);
        #else
		if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL) fclose(AUDi_InjectContext[DeviceId]->FileHandle);
        #endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
        STOS_MemoryDeallocate(SystemPartition_p,AUDi_InjectContext[DeviceId]->FdmaNode);
		STFDMA_UnlockChannel(AUDi_InjectContext[DeviceId]->FdmaChannelId,STFDMA_1);
#endif
        STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
        STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]);
		AUDi_InjectContext[DeviceId]=NULL;
		AUDi_TraceERR(("AUD_InjectStart():**ERROR** !!! Unable to create the injector task !!!\n"));
		return(RetErr);
	}

	if(AUDi_InjectContext[DeviceId] != NULL)
	{
		AUDi_InjectContext[DeviceId]->State  = AUD_INJECT_START;
		AUDi_InjectContext[DeviceId]->ForceStop = FALSE;
		STOS_SemaphoreSignal(AUDi_InjectContext[DeviceId]->SemStart);
	}
	assign_integer(Result,RetErr,false);
	return RetErr;

}


static boolean AUD_PCMInject( parse_t *pars_p, char *Result )
{

	boolean RetErr = FALSE;
	BOOL NotSubTask;

	U32 NbLoops;
	U32 DriverId;

	U32 DeviceId;


    /* switch according to INJECT_DEST testtool variable */
	RetErr = cget_integer( pars_p, 0, &NbLoops );
	 if(RetErr == TRUE)
	 {
		tag_current_line( pars_p, "expected NbLoops" );
	 }

	 RetErr = cget_integer( pars_p, 0, &DeviceId );
	 if(RetErr == TRUE)
	 {
		tag_current_line( pars_p, "expected DeviceId" );
	 }

	RetErr = cget_integer( pars_p, 0, &DriverId );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DriverId" );
	}

	if(AUDi_InjectContext[DeviceId]==NULL)
	{
		STTBX_Print(("Injection Context not intialized\n"));
		return TRUE;
	}
	AUDi_InjectContext[DeviceId]->NbLoops = NbLoops;

	InjectTask_p = &InjectTaskTable[DeviceId];

	cget_integer( pars_p, FALSE, (long *)&NotSubTask );
	AUDi_InjectContext[DeviceId]->NbLoopsAlreadyDone  = 0;
	AUDi_InjectContext[DeviceId]->State               = AUD_INJECT_SLEEP;
	AUDi_InjectContext[DeviceId]->AUD_Handle          = AUD_Handle[DriverId];


	/* Create the semaphores */
	/* --------------------- */
	AUDi_InjectContext[DeviceId]->SemStart = STOS_SemaphoreCreateFifo(NULL,0);
	AUDi_InjectContext[DeviceId]->SemStop =  STOS_SemaphoreCreateFifo(NULL,0);
	AUDi_InjectContext[DeviceId]->SemLock =  STOS_SemaphoreCreateFifo(NULL,1);



	/* Create the inject task */
	/* ---------------------- */
#ifndef ST_OSLINUX
	STOS_TaskCreate(AUDi_PCMInjectTask,AUDi_InjectContext[DeviceId],NULL,(2048+16384),NULL,NULL,&AUDi_InjectContext[DeviceId]->Task,
		NULL,MAX_USER_PRIORITY/2,"AUD_InjectorTask",0);

 if (AUDi_InjectContext[DeviceId]->Task==NULL)
#else
	if(pthread_create(&(AUDi_InjectContext[DeviceId]->PCMInjectTask), NULL, (void*)AUDi_PCMInjectTask, AUDi_InjectContext[DeviceId]) < 0 )
#endif /*ST_OSLINUX*/

  {
   STAUD_Fade_t AUD_Fade;
   AUD_Fade.FadeType =STAUD_FADE_SOFT_MUTE;
   STAUD_Stop(AUDi_InjectContext[DeviceId]->AUD_Handle,STAUD_STOP_NOW,&AUD_Fade);
   if (AUDi_InjectContext[DeviceId]->FileHandle!=NULL) fclose(AUDi_InjectContext[DeviceId]->FileHandle);
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7710))
   STOS_MemoryDeallocate(SystemPartition_p,AUDi_InjectContext[DeviceId]->FdmaNode);
   STFDMA_UnlockChannel(AUDi_InjectContext[DeviceId]->FdmaChannelId,STFDMA_1);
#endif
   STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]->BufferPtr);
   STOS_MemoryDeallocate(SystemPartition_p,(void *)AUDi_InjectContext[DeviceId]);
   AUDi_InjectContext[DeviceId]=NULL;
   AUDi_TraceERR(("AUD_InjectStart():**ERROR** !!! Unable to create the injector task !!!\n"));
   return(RetErr);
  }
else
	{
		STTBX_Print(("AUDi_PCMInjectTask Created succesfully \n "));
	}

	if(AUDi_InjectContext[DeviceId]!=NULL)
	{
	/*We are in memory mode or in PTI Mode */
		AUDi_InjectContext[DeviceId]->State               = AUD_INJECT_START;
		STOS_SemaphoreSignal(AUDi_InjectContext[DeviceId]->SemStart);

	}
	assign_integer(Result,RetErr,false);
	return RetErr;

}

/*-------------------------------------------------------------------------
 * Function : AUD_Verbose
 *            set the Verbose flag used in functions to print or not details
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_Verbose( parse_t *pars_p, char *result_sym_p )
{
    long LVar;
    boolean RetErr;

    /* get Verbose param */
    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected TRUE (1) or FALSE (0)" );
    }
    else
    {
        Verbose = (BOOL)LVar;
    }
    STTBX_Print(( "AUD_Verbose(%s)\n", ( Verbose ) ? "TRUE" : "FALSE" ));
	assign_integer(result_sym_p,RetErr,false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_Verbose */


/*#######################################################################*/
/*########################### EVT FUNCTIONS #############################*/
/*#######################################################################*/

#if 0
/*-------------------------------------------------------------------------
 * Function : AUD_ConfigDAC
 *            Set the DAC configuration used in the AUD_Init function
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_ConfigDAC(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    char Msg[80];
    long LVar;
    LocalDACConfiguration_t TmpConfig;

    /* Set default DAC config */
    GetDefaultDACConfig(&TmpConfig);
    TmpConfig.Initialized = FALSE;

    /* use internal PLL or not */
    RetErr = cget_integer( pars_p, TmpConfig.InternalPLL, &LVar);
    if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
    {
        sprintf( Msg, "internalPLL : 0 or 1 expected ");
        tag_current_line( pars_p, Msg );
    }
    else
    {
        TmpConfig.InternalPLL = (BOOL) LVar;
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACClockToFsRatio, &LVar);
        if ( RetErr == TRUE )
        {
            sprintf( Msg, "DACClockToFsRatio : unsigned integer expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACClockToFsRatio = (U32) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.InvertWordClock, &LVar);
        if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
        {
            sprintf( Msg, "InvertWordClock : 0 or 1 expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.InvertWordClock = (BOOL) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.InvertBitClock, &LVar);
        if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
        {
            sprintf( Msg, "InvertBitClock : 0 or 1 expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.InvertBitClock = (BOOL) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.Format, &LVar);
        if ( (RetErr == TRUE ) ||
             ((LVar != STAUD_DAC_DATA_FORMAT_I2S) && (LVar != STAUD_DAC_DATA_FORMAT_STANDARD)))
        {
            sprintf( Msg, "Format : %d (I2S) or %d (STANDARD) expected ",
                     STAUD_DAC_DATA_FORMAT_I2S, STAUD_DAC_DATA_FORMAT_STANDARD);
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.Format = (STAUD_DACDataFormat_t) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.Precision, &LVar);
        if ( (RetErr == TRUE ) ||
             ((LVar < STAUD_DAC_DATA_PRECISION_16BITS) || (LVar > STAUD_DAC_DATA_PRECISION_24BITS)))
        {
            sprintf( Msg, "Precision : %d (16Bits) or %d (18bits) or %d (20Bits) or %d (24bits) expected ",
                     STAUD_DAC_DATA_PRECISION_16BITS, STAUD_DAC_DATA_PRECISION_18BITS,
                     STAUD_DAC_DATA_PRECISION_20BITS, STAUD_DAC_DATA_PRECISION_24BITS);
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.Precision = (STAUD_DACDataPrecision_t) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.Alignment,
                               &LVar);
        if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
        {
            sprintf( Msg, "RightPadded : 0 or 1 expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            if( (BOOL)LVar )
            {
                TmpConfig.DACConfiguration.Alignment = STAUD_DAC_DATA_ALIGNMENT_ZERO_EXT;
            }
            else
            {
                TmpConfig.DACConfiguration.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT_SIGNED;
            }
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.MSBFirst, &LVar);
        if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
        {
            sprintf( Msg, "MSBFirst : 0 or 1 expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.MSBFirst = (BOOL) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        TmpConfig.Initialized = TRUE;
        LocalDACConfiguration = TmpConfig;
        STTBX_Print(( "AUD_ConfigDAC(%d, %d, %d, %d, %d, %d, %d, %d) ok\n",
                      LocalDACConfiguration.InternalPLL,
                      LocalDACConfiguration.DACClockToFsRatio,
                      LocalDACConfiguration.DACConfiguration.InvertWordClock,
                      LocalDACConfiguration.DACConfiguration.InvertBitClock,
                      LocalDACConfiguration.DACConfiguration.Format,
                      LocalDACConfiguration.DACConfiguration.Precision,
                      LocalDACConfiguration.DACConfiguration.Alignment,
                      LocalDACConfiguration.DACConfiguration.MSBFirst));
    }
    else
    {
        STTBX_Print(( "AUD_ConfigDAC() error : %s\n", Msg));
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of AUD_ConfigDAC() */
#endif

/*-------------------------------------------------------------------------
 * Function : AUD_GetCellType
 *            Return audio decoder cell type (1:MPEG1, 2:MMDSP 3:AMPHION)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_GetCellType(parse_t *pars_p, char *result_sym_p)
{
	boolean RetErr;
	long DeviceID;
	RetErr = cget_integer( pars_p, 0, &DeviceID );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}
#if defined(AMPHION)
    assign_integer(result_sym_p, (long)AMPHION_CELL, false);
#elif defined(MMDSP)
    assign_integer(result_sym_p, (long)MMDSP_CELL, false);

#elif defined(MPEG1)
    assign_integer(result_sym_p, (long)MPEG1_CELL, false);
#elif defined(LX_AUDIO)
    assign_integer(result_sym_p, (long)LX_CELL, false);
#else
#error Error : Audio decoder cell type not defined (use AUDIOCELL variable)
#endif

  	assign_integer(result_sym_p,RetErr,false);

    return (FALSE);
} /* end of AUD_GetCellType() */

/*-------------------------------------------------------------------------
 * Function : AUD_STBDVD
 *            Return target sepcified in the application build time (1:STB, 2:DVD)
 *            reflects the STB_DVD environment variable state
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_STBDVD(parse_t *pars_p, char *result_sym_p)
{
	boolean RetErr;
	long DeviceID;
	RetErr = cget_integer( pars_p, 0, &DeviceID );
	if(RetErr == TRUE)
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

    assign_integer(result_sym_p, 1, false);
	/*Build for STB , So return STB from Here */

    return (FALSE);
} /* end of AUD_STBDVD() */


static BOOL AUD_DRStart( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_StreamParams_t StreamParams;
    STAUD_Object_t DecoderObject;

    char Buff[160];
    boolean RetErr;
    long LVar;
    U8 DeviceId;
    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, STAUD_STREAM_CONTENT_MPEG1, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( Buff,"expected Decoder ( %d=MPG1 "
                 "%d=MPG2 %d=PCM )",
                 STAUD_STREAM_CONTENT_MPEG1,
                 STAUD_STREAM_CONTENT_MPEG2,
                 STAUD_STREAM_CONTENT_PCM
                 );
        tag_current_line( pars_p, Buff );
    }
    else
    {
        StreamParams.StreamContent = (STAUD_StreamContent_t)LVar;

        RetErr = cget_integer( pars_p, STAUD_STREAM_TYPE_PES, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( Buff,"expected Decoder (%d=ES %d=PES "
					 "  %d=MPEG1_PACKET )",
						STAUD_STREAM_TYPE_ES,
						STAUD_STREAM_TYPE_PES,
						STAUD_STREAM_TYPE_MPEG1_PACKET );
            tag_current_line( pars_p, Buff );
        }
        else
        {
            StreamParams.StreamType = (STAUD_StreamType_t)LVar;

            /* get SamplingFrequency (default: 48000) */
            RetErr = cget_integer( pars_p, 48000, &LVar );
            if ( RetErr == TRUE )
            {
				tag_current_line( pars_p, "expected SamplingFrequency "
								"(32000, 44100, 48000)" );
            }
            else
            {
                StreamParams.SamplingFrequency = (U32)LVar;
            }
        }
    }

	 RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
   {
      tag_current_line( pars_p, "expected deviceId" );
   }
   DeviceId = (U8)LVar;

    if ( RetErr == FALSE ) /* only start if all parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_DRStart(%u,(%d, %d, %d, %2.2x)) : ",
                 (U32)AUD_Handle[DeviceId], StreamParams.StreamContent,
                 StreamParams.StreamType, StreamParams.SamplingFrequency,
                 StreamParams.StreamID );

        ErrCode = STAUD_DRStart(AUD_Handle[DeviceId], DecoderObject, &StreamParams );



		RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
		STTBX_Print(( AUD_Msg ));
		return(ST_NO_ERROR);

	} /* end if */

	assign_integer(result_sym_p, (long)ErrCode, false);
	return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRStart */

/*-------------------------------------------------------------------------
 * Function : AUD_MXSetInputParams
              connect objects with PCM outputs to mixer inputs
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_MXSetInputParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    long LVar = 0;
    boolean RetErr;
    STAUD_MixerInputParams_t MixerParams;
     STAUD_Object_t DecoderObject;   ObjectString_t DRObject;

    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, 0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Mixer level expected: 0 - 255");
    }
    else
    {
        MixerParams.MixLevel = (U8)LVar;

        /* obtain object */
        RetErr = cget_integer( pars_p, STAUD_OBJECT_MIXER0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRPCM, OBJ_DRCOMP\n");
        }
        else
        {
            DecoderObject = (STAUD_Object_t)LVar;
            ObjectToText( DecoderObject, &DRObject );
        }
    }


    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_MXSetInputParams(%u, %s): ", (U32)AudDecHndl0, DRObject );
        ErrCode = STAUD_MXSetInputParams( AudDecHndl0, STAUD_OBJECT_INPUT_PCM0,
                    DecoderObject, &MixerParams);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_MXSetInputParams */

/*-------------------------------------------------------------------------
 * Function : AUD_MXSetPTSStatus
 *            Set the type of the output
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_MXSetPTSStatus( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;
	U16 Input;
	BOOL PTSStatus;
    STAUD_Object_t MixerObject;    ObjectString_t OPObject;

	U8 DeviceId;

	RetErr = cget_integer( pars_p, 0, &LVar );

	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
   	DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_MIXER0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_MIX0\n");
        }
        else
        {
            MixerObject = (STAUD_Object_t)LVar;
            ObjectToText( MixerObject, &OPObject);
        }
    }

	if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Input ID expected: 0/1\n");
        }
        else
        {
            Input = (U16)LVar;
        }
    }

	if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
			tag_current_line( pars_p, "PTS Staus expected: 0/1\n");
        }
        else
        {
            PTSStatus = (BOOL)LVar;
        }
    }

    sprintf( AUD_Msg, "AUD_STSetMixerPTSStatus(%u,%s,%d,%d) : ", (U32)AUD_Handle[DeviceId], OPObject, Input,  PTSStatus );

    ErrCode = STAUD_MXUpdatePTSStatus( AUD_Handle[DeviceId], MixerObject, Input,  PTSStatus );

    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}



static BOOL AUD_IPSetParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Object_t InputObject;    ObjectString_t IPObject;
    BOOL RetErr;
    long LVar = 0;
    STAUD_InputParams_t InputParams;
    int LocalMode;
    /*U8 DeviceId;*/

    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_INPUT_CD0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: OBJ_IPCD1, OBJ_IPCD2 or OBJ_IPPCM\n");
    }
    else
    {
        InputObject = (STAUD_Object_t)LVar;
        ObjectToText( InputObject, &IPObject);
    }

    if (RetErr == FALSE )
    {
        if(InputObject == STAUD_OBJECT_INPUT_PCM0)
        {
            RetErr = cget_integer( pars_p, 16, (long *)&LVar );
            if ( RetErr == TRUE )
            {
                tag_current_line( pars_p, "Slot size expected: (8, 16, 32 bits)\n");
            }
            else
            {
                InputParams.PCMBufParams.SlotSize = (U32)LVar;
            }

            if(!RetErr)
            {
                RetErr = cget_integer( pars_p, 16, (long *)&LVar );
                if ( RetErr == TRUE )
                {
                    tag_current_line( pars_p, "Precision expected: (8, 16, 18, 20 or 24 bits)\n");
                }
                else
                {
                    InputParams.PCMBufParams.DataPrecision = (U32)LVar;
                }
            }

            if(!RetErr)
            {
                RetErr = cget_integer( pars_p, 0, (long *)&LVar );
                if ( RetErr == TRUE )
                {
                    tag_current_line( pars_p, "Invert word clock expected (1:0)\n");
                }
                else
                {
                    InputParams.PCMBufParams.InvertWordClock = (BOOL)LVar;
                }
            }

            if(!RetErr)
            {
                RetErr = cget_integer( pars_p, 0, (long *)&LVar );
                if ( RetErr == TRUE )
                {
                    tag_current_line( pars_p, "ByteSwapData (1:0)\n");
                }
                else
                {
                    InputParams.PCMBufParams.ByteSwapData = (BOOL)LVar;
                }
            }

            if(!RetErr)
            {
                RetErr = cget_integer( pars_p, 0, (long *)&LVar );
                if ( RetErr == TRUE )
                {
                    tag_current_line( pars_p, "UnsignedData (1:0)\n");
                }
                else
                {
                    InputParams.PCMBufParams.UnsignedData = (BOOL)LVar;
                }
            }

            if(!RetErr)
            {
                RetErr = cget_integer( pars_p, 0, (long *)&LVar );
                if ( RetErr == TRUE )
                {
                    tag_current_line( pars_p, "MonoData (1:0)\n");
                }
                else
                {
                    InputParams.PCMBufParams.MonoData = (BOOL)LVar;
                }
            }

            if(!RetErr)
            {
                RetErr = cget_integer( pars_p, 1, (long *)&LVar );
                if ( RetErr == TRUE )
                {
                    tag_current_line( pars_p, "MSBFirst (1:0)\n");
                }
                else
                {
                    InputParams.PCMBufParams.MSBFirst = (BOOL)LVar;
                }
            }
            sprintf( AUD_Msg, "STAUD_IPSetParams(%u %s PCM %u/%u bits)",
                     (U32)AudDecHndl0, IPObject,
                     InputParams.PCMBufParams.DataPrecision,
                     InputParams.PCMBufParams.SlotSize );
        }
        else /* CD fifo params */
        {

            RetErr = cget_integer( pars_p, STAUD_INPUT_MODE_DMA, (long *)&LVar );
            if ( RetErr == TRUE )
            {
                tag_current_line( pars_p, "Input mode expected: I2S, DIRECT_I2S or DMA \n");
            }
            else
            {
                InputParams.CDFIFOParams.InputMode = LVar;
                switch (LVar)
                {
                case STAUD_INPUT_MODE_DMA:
                    break;

                case STAUD_INPUT_MODE_I2S:
                case STAUD_INPUT_MODE_I2S_DIRECT:
                    if (RetErr == FALSE )
                    {
                        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
                        if ( RetErr == TRUE )
                        {
                            tag_current_line( pars_p, "I2S id expected: (0=I2S input1, 1=I2S input2)\n");
                        }
                        else
                        {
                            InputParams.CDFIFOParams.I2SParams.I2SInputId = (U32)LVar;
                        }
                    }


                    /* these values are dependent on whatever is supplying the I2S input*/
                    if (RetErr == FALSE )
                    {
                        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
                        if ( RetErr == TRUE )
                        {
                            tag_current_line( pars_p, "Mode expected \n");
                        }
                        else
                        {
                            LocalMode = (int)LVar;
                            switch( LVar )
                            {
                            case 0:
                                InputParams.CDFIFOParams.I2SParams.Precision = STAUD_DAC_DATA_PRECISION_16BITS;
                                InputParams.CDFIFOParams.I2SParams.InvertWordClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.InvertBitClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT;
                                InputParams.CDFIFOParams.I2SParams.Format = STAUD_DAC_DATA_FORMAT_STANDARD;
                                InputParams.CDFIFOParams.I2SParams.MSBFirst = TRUE;
                                break;

                            case 1:
                                InputParams.CDFIFOParams.I2SParams.Precision = STAUD_DAC_DATA_PRECISION_16BITS;
                                InputParams.CDFIFOParams.I2SParams.InvertWordClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.InvertBitClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT;
                                InputParams.CDFIFOParams.I2SParams.Format = STAUD_DAC_DATA_FORMAT_STANDARD;
                                InputParams.CDFIFOParams.I2SParams.MSBFirst = FALSE;
                                break;

                            case 2:
                                InputParams.CDFIFOParams.I2SParams.Precision = STAUD_DAC_DATA_PRECISION_18BITS;
                                InputParams.CDFIFOParams.I2SParams.InvertWordClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.InvertBitClock = TRUE;
                                InputParams.CDFIFOParams.I2SParams.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT;
                                InputParams.CDFIFOParams.I2SParams.Format = STAUD_DAC_DATA_FORMAT_STANDARD;
                                InputParams.CDFIFOParams.I2SParams.MSBFirst = TRUE;
                                break;

                            case 3:
                                InputParams.CDFIFOParams.I2SParams.Precision = STAUD_DAC_DATA_PRECISION_18BITS;
                                InputParams.CDFIFOParams.I2SParams.InvertWordClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.InvertBitClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.Alignment = STAUD_DAC_DATA_ALIGNMENT_RIGHT;
                                InputParams.CDFIFOParams.I2SParams.Format =  STAUD_DAC_DATA_FORMAT_STANDARD;
                                InputParams.CDFIFOParams.I2SParams.MSBFirst = TRUE;
                                break;

                            case 4:
                                InputParams.CDFIFOParams.I2SParams.Precision = STAUD_DAC_DATA_PRECISION_18BITS;
                                InputParams.CDFIFOParams.I2SParams.InvertWordClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.InvertBitClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT;
                                InputParams.CDFIFOParams.I2SParams.Format = STAUD_DAC_DATA_FORMAT_I2S;
                                InputParams.CDFIFOParams.I2SParams.MSBFirst = TRUE;
                                break;

                            case 5:
                                InputParams.CDFIFOParams.I2SParams.Precision = STAUD_DAC_DATA_PRECISION_18BITS;
                                InputParams.CDFIFOParams.I2SParams.InvertWordClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.InvertBitClock = FALSE;
                                InputParams.CDFIFOParams.I2SParams.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT;
                                InputParams.CDFIFOParams.I2SParams.Format = STAUD_DAC_DATA_FORMAT_STANDARD;
                                InputParams.CDFIFOParams.I2SParams.MSBFirst = TRUE;
                                break;
                            default:
								break;

                            }
                        }
                    }
                    break;

                default:
                    STTBX_Print(( "Invalid input mode entered\n" ));
                    break;
                }
            }
            InputParams.CDFIFOParams.PESParserParams.Mode = STAUD_PES_MODE_DISABLED;

            sprintf( AUD_Msg, "STAUD_IPSetParams(%u, %s, %s, %d, Mode: %d): ",
                     (U32)AudDecHndl0, IPObject, "I2S",
                     (int)InputParams.CDFIFOParams.I2SParams.I2SInputId+1, LocalMode );
        }
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        ErrCode = STAUD_IPSetParams( AudDecHndl0, InputObject, &InputParams );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}

static BOOL AUD_IPGetParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    long LVar = 0;
	U8 DeviceId;
    STAUD_InputParams_t InputParams;
    STAUD_Object_t InputObject;    ObjectString_t IPObject;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_INPUT_CD1, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: OBJ_IPCD1, OBJ_IPCD2 or OBJ_IPPCM\n");
    }
    else
    {
        InputObject = (STAUD_Object_t)LVar;
        ObjectToText( InputObject, &IPObject);
    }

    sprintf( AUD_Msg, "STAUD_IPGetParams(%u, %s) : ", (U32)AUD_Handle[DeviceId], IPObject);
	ErrCode = STAUD_IPGetParams(AUD_Handle[DeviceId], InputObject, &InputParams);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        strcat( AUD_Msg, "CDFIFOParams.");
        switch( InputParams.CDFIFOParams.InputMode )
        {
            case STAUD_INPUT_MODE_DMA:
                strcat( AUD_Msg, "InputMode: DMA\n" );
                break;
            case STAUD_INPUT_MODE_I2S:
                strcat( AUD_Msg, "InputMode: I2S\n" );
                break;
	        case STAUD_INPUT_MODE_I2S_DIRECT:
				strcat( AUD_Msg, "InputMode: I2SDirect \n" );
				break;

            default:
            strcat( AUD_Msg, "InputMode:< Not set>\n");
                break;
        }
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}


static BOOL AUD_DRMute( parse_t *pars_p, char *result_sym_p )
{
#if MUTE_AT_DECODER
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    long LVar = 0;
    U8 DeviceId = 0;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
	RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected deviceId" );
	}
	DeviceId = (U8)LVar;



    RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
    }
    else
    {
        OutputObject = (STAUD_Object_t)LVar;
        ObjectToText( OutputObject, &OPObject);
    }

    if ( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "STAUD_OPMute(%u, %s) : ", (U32)AUD_Handle[DeviceId], &OPObject);
        ErrCode = STAUD_DRMute( AUD_Handle[DeviceId], OutputObject);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
#else

    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    long LVar = 0;
    U8 DeviceId = 0;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;


    RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_PCMP0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: OBJ_OPPCM0, OBJ_OPPCM1,OBJ_OPSPDIF0\n");
    }
    else
    {
        OutputObject = (STAUD_Object_t)LVar;
        ObjectToText( OutputObject, &OPObject);
    }


    if ( RetErr == FALSE )
    {
		sprintf( AUD_Msg, "STAUD_OPMute(%u, %s) : ", (U32)AUD_Handle[DeviceId], OPObject);
        ErrCode = STAUD_OPMute( AUD_Handle[DeviceId], OutputObject);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );


#endif

}


static BOOL AUD_DRUnMute( parse_t *pars_p, char *result_sym_p )
{
#if MUTE_AT_DECODER
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    long LVar = 0;
    U8 DeviceId = 0;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

	/*task_delay(62500000);*/

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );

    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
    }
    else
    {
        OutputObject = (STAUD_Object_t)LVar;
        ObjectToText( OutputObject, &OPObject);
    }

    if ( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "STAUD_OPUnMute(%u, %s) : ", (U32)AUD_Handle[DeviceId], &OPObject);
        ErrCode = STAUD_DRUnMute( AUD_Handle[DeviceId], OutputObject );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
#else

    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    long LVar = 0;
    U8 DeviceId = 0;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;


    RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_PCMP0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: OBJ_OPPCM0, OBJ_OPPCM1,OBJ_OPSPDIF0\n");
    }
    else
    {
        OutputObject = (STAUD_Object_t)LVar;
        ObjectToText( OutputObject, &OPObject);
    }


    if ( RetErr == FALSE )
    {
		sprintf( AUD_Msg, "STAUD_OPUnMute(%u, %s) : ", (U32)AUD_Handle[DeviceId], OPObject);
        ErrCode = STAUD_OPUnMute( AUD_Handle[DeviceId], OutputObject);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );


#endif




}



static BOOL AUD_STMute( parse_t *pars_p, char *result_sym_p )
{

    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    long LVar = 0;
    U8 DeviceId = 0;
    U8 Mute = 0;


    RetErr = cget_integer( pars_p,0, (long *)&LVar );
    if ( RetErr == TRUE )
       {
	 tag_current_line( pars_p, "expected value of mute" );
	}
   	Mute = (U8)LVar;

    RetErr = cget_integer( pars_p, 0, (long *)&LVar );

    if ( RetErr == TRUE )
       {
	 tag_current_line( pars_p, "expected deviceId" );
	}
   	DeviceId = (U8)LVar;


    if ( RetErr == FALSE )
    {
       sprintf( AUD_Msg, "STAUD_OPMute(%u ) : ",DeviceId );
        switch(Mute)
        {
         case 0: /* mute Ananlog*/
        ErrCode = STAUD_Mute( AUD_Handle[DeviceId],TRUE,FALSE );
        break;
        case 1: /* Mute digital*/
        ErrCode = STAUD_Mute( AUD_Handle[DeviceId],FALSE,TRUE );
        break;

        case 2: /* Mute Both*/
        ErrCode = STAUD_Mute( AUD_Handle[DeviceId],TRUE,TRUE );
        break;

        case 3: /* UnMute Both*/
        ErrCode = STAUD_Mute( AUD_Handle[DeviceId],FALSE,FALSE );
        break;
    		default :
			ErrCode = STAUD_Mute( AUD_Handle[DeviceId],FALSE,FALSE );
		break;



	}
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);

    return ( API_EnableError ? RetErr : FALSE );
}


static BOOL AUD_DREnableDownsampling( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Object_t DecoderObject;  ObjectString_t DRObject;
    BOOL RetErr;
	U8  DeviceId;
	long LVar = 0;
	ErrCode = ST_NO_ERROR;

	RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}
	DeviceId = (U8)LVar;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Decoder object expected: OBJ_DRCOMP0 (Compressed) or OBJ_DRCOMP1 \n");
    }
    else
    {
        DecoderObject = ( STAUD_Object_t )LVar;
        ObjectToText( DecoderObject, &DRObject );
    }

    if ( RetErr == FALSE ) /* only start if all parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_PPEnableDownsampling(%u, %s) : ",
                 (U32)AudDecHndl0, DRObject );

        /* set parameters and start */
        ErrCode = STAUD_PPEnableDownsampling( AUD_Handle[DeviceId], DecoderObject,0);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));

    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}

static BOOL AUD_DRDisableDownsampling( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Object_t DecoderObject;  ObjectString_t DRObject;
    BOOL RetErr;
    long LVar = 0;
	U8   DeviceId;
    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED1, &LVar );

    RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}

   DeviceId = (U8)LVar;
    RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED1, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Decoder object expected: OBJ_DRCOMP (Compressed) or OBJ_DRPCM (PCMPlayer)\n");
    }
    else
    {
        DecoderObject = ( STAUD_Object_t )LVar;
        ObjectToText( DecoderObject, &DRObject );
    }

    if ( RetErr == FALSE ) /* only start if all parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_DRDisableDownsampling(%u, %s) : ",
                 (U32)AudDecHndl0, DRObject );

        /* set parameters and start */
        ErrCode = STAUD_PPDisableDownsampling( AUD_Handle[DeviceId], DecoderObject );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));

    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}


static BOOL AUD_ResetHandle( parse_t *pars_p, char *result_sym_p )
{
	long LVar;
	U8 DeviceId;
	boolean RetErr;
	RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}
	DeviceId =  (U8)LVar;
    AUD_Handle[DeviceId] = 0;
    STTBX_Print(( "Handle set to zero\n" ));
	assign_integer(result_sym_p,0,false);
    return FALSE;
}


static BOOL AUD_GetHandle( parse_t *pars_p, char *result_sym_p )
{
	long LVar;
	U8 DeviceId;
	boolean RetErr;
	RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}
	DeviceId =  (U8)LVar;
    assign_integer(result_sym_p, (long)AUD_Handle[DeviceId], false);
    return FALSE;
}

/***********Use this function if there is only one open per init*************/
static BOOL AUD_SetHandle( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    char LocalDeviceName[40];
    char DefaultDeviceName[40] = DEFAULT_AUD_NAME;
    Init_p SearchPointer, ResultPointer;

    RetErr = cget_string( pars_p, DefaultDeviceName, LocalDeviceName, sizeof(LocalDeviceName) );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Devicename expected\n");
    }
    else
    {
        /* get pointer to relevant init */
        SearchPointer = Init_head;             /*initialise pointers*/
        ResultPointer = NULL;

        while( SearchPointer != NULL )
        {
            if( strcmp(SearchPointer->DeviceName, LocalDeviceName) == 0 )
            {
                ResultPointer = SearchPointer;
                STTBX_Print(( "Found correct devicename\n" ));
                break;
            }
            else
            {
                SearchPointer = SearchPointer->next;
            }
        }

        if( ResultPointer == NULL )
        {
            STTBX_Print(( "Error: This device was not initialised in the test harness\n" ));
        }
        else
        {
        /* check if there is only one open */
            if( ResultPointer->Open_head->next == NULL )
            {
            /*if yes, store in AudDecHndl0 */
                AudDecHndl0 = ResultPointer->Open_head->DecoderHandle;
            }
            else
            {
            /*if no, ask user to specify which one is required */
                STTBX_Print(( "This device has more than one open handles, please specify which one" ));
            /* if a valid handle (or index) is entered, set AudDecHndl0 appropriately */
            }
        }
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}

static void DeviceToText( STAUD_Device_t Device, ObjectString_t * Text )
{
    switch (Device)
    {
      case STAUD_DEVICE_STI7100:
            strcpy( *Text, "STi7100" );
            break;
		case STAUD_DEVICE_STI8010:
            strcpy( *Text, "INVALID_DEVICE" );
            break;

		case STAUD_DEVICE_STI7109:
            strcpy( *Text, "STi7109" );
            break;

		case STAUD_DEVICE_STI5525:
            strcpy( *Text, "INVALID_DEVICE" );
            break;

        default:
            sprintf( *Text, "INVALID_DEVICE %x", (U32)Device );
            break;
    }
}
static void ObjectToText( STAUD_Object_t Object, ObjectString_t *Text )
{
    switch (Object)
    {
        default:
            strcpy( *Text, "INVALID_OBJECT" );
            break;
		case STAUD_OBJECT_INPUT_CD0 :
			strcpy( *Text, "STAUD_OBJECT_INPUT_CD0" );
            break;

		case STAUD_OBJECT_INPUT_CD1 :
			strcpy( *Text, "STAUD_OBJECT_INPUT_CD1" );
            break;

		case STAUD_OBJECT_INPUT_PCM0:
			strcpy( *Text, "STAUD_OBJECT_INPUT_PCM0" );
            break;



		case STAUD_OBJECT_DECODER_COMPRESSED0 :
			strcpy( *Text, "STAUD_OBJECT_DECODER_COMPRESSED0" );
        break;

		case STAUD_OBJECT_DECODER_COMPRESSED1 :
			strcpy( *Text, "STAUD_OBJECT_DECODER_COMPRESSED1" );
        break;

		case STAUD_OBJECT_ENCODER_COMPRESSED0 :
			strcpy( *Text, "STAUD_OBJECT_ENCODER_COMPRESSED0" );
        break;

		case STAUD_OBJECT_MIXER0 :
			strcpy( *Text, "STAUD_OBJECT_MIXER0" );
        break;

		case STAUD_OBJECT_POST_PROCESSOR0 :
			strcpy( *Text, "STAUD_OBJECT_POST_PROCESSOR0" );
        break;

		case STAUD_OBJECT_POST_PROCESSOR1 :
			strcpy( *Text, "STAUD_OBJECT_POST_PROCESSOR1" );
        break;

		case STAUD_OBJECT_POST_PROCESSOR2 :
			strcpy( *Text, "STAUD_OBJECT_POST_PROCESSOR2" );
        break;

		case STAUD_OBJECT_OUTPUT_PCMP0 :
			strcpy( *Text, "STAUD_OBJECT_OUTPUT_PCMP0" );
        break;

		case STAUD_OBJECT_OUTPUT_PCMP1 :
			strcpy( *Text, "STAUD_OBJECT_OUTPUT_PCMP1" );
        break;

		case STAUD_OBJECT_OUTPUT_PCMP2 :
			strcpy( *Text, "STAUD_OBJECT_OUTPUT_PCMP2" );
        break;

		case STAUD_OBJECT_OUTPUT_PCMP3 :
			strcpy( *Text, "STAUD_OBJECT_OUTPUT_PCMP3" );
        break;

		case STAUD_OBJECT_OUTPUT_SPDIF0 :
			strcpy( *Text, "STAUD_OBJECT_OUTPUT_SPDIF0" );
        break;

		case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER :
			strcpy( *Text, "STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER" );
        break;

		case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER :
			strcpy( *Text, "STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER" );
        break;

		case STAUD_OBJECT_FRAME_PROCESSOR :
			strcpy( *Text, "STAUD_OBJECT_FRAME_PROCESSOR" );
        break;

		case STAUD_OBJECT_BLCKMGR :
			strcpy( *Text, "STAUD_OBJECT_BLCKMGR" );
        break;

    }
}

static void StreamContentToText( STAUD_StreamContent_t StreamContent, char *Text )
{
    switch (StreamContent)
    {
        case STAUD_STREAM_CONTENT_MPEG1:
            strcpy( Text, "STAUD_STREAM_CONTENT_MPEG1" );
            break;
        case STAUD_STREAM_CONTENT_MPEG2:
            strcpy( Text, "STAUD_STREAM_CONTENT_MPEG2" );
            break;
        case STAUD_STREAM_CONTENT_BEEP_TONE:
            strcpy( Text, "STAUD_STREAM_CONTENT_BEEP_TONE" );
            break;
		case STAUD_STREAM_CONTENT_AC3:
			strcpy( Text, "STAUD_STREAM_CONTENT_AC3" );
            break;

		case STAUD_STREAM_CONTENT_DTS:
			strcpy( Text, "STAUD_STREAM_CONTENT_DTS" );
            break;

		case STAUD_STREAM_CONTENT_CDDA:
			strcpy( Text, "STAUD_STREAM_CONTENT_CDDA" );
            break;

		case STAUD_STREAM_CONTENT_PCM:
			strcpy( Text, "STAUD_STREAM_CONTENT_PCM" );
            break;

		case STAUD_STREAM_CONTENT_LPCM:
			strcpy( Text, "STAUD_STREAM_CONTENT_LPCM" );
            break;

		case STAUD_STREAM_CONTENT_PINK_NOISE:
			strcpy( Text, "STAUD_STREAM_CONTENT_PINK_NOISE" );
            break;

		case STAUD_STREAM_CONTENT_MP3:
			strcpy( Text, "STAUD_STREAM_CONTENT_MP3" );
            break;

		case STAUD_STREAM_CONTENT_MLP:
			strcpy( Text, "STAUD_STREAM_CONTENT_MLP" );
            break;

		case STAUD_STREAM_CONTENT_MPEG_AAC:
			strcpy( Text, "STAUD_STREAM_CONTENT_MPEG_AAC" );
            break;

		case STAUD_STREAM_CONTENT_WMA:
			strcpy( Text, "STAUD_STREAM_CONTENT_WMA" );
            break;

		case STAUD_STREAM_CONTENT_DV :
			strcpy( Text, "STAUD_STREAM_CONTENT_DV" );
            break;

		case STAUD_STREAM_CONTENT_CDDA_DTS	:
			strcpy( Text, "STAUD_STREAM_CONTENT_CDDA_DTS" );
            break;

		case STAUD_STREAM_CONTENT_LPCM_DVDA  :
			strcpy( Text, "STAUD_STREAM_CONTENT_LPCM_DVDA" );
            break;

		case STAUD_STREAM_CONTENT_HE_AAC :
			strcpy( Text, "STAUD_STREAM_CONTENT_HE_AAC" );
            break;

		case STAUD_STREAM_CONTENT_DDPLUS :
			strcpy( Text, "STAUD_STREAM_CONTENT_DDPLUS" );
            break;

		case STAUD_STREAM_CONTENT_WMAPROLSL:
			strcpy( Text, "STAUD_STREAM_CONTENT_WMAPROLSL" );
            break;

		case STAUD_STREAM_CONTENT_NULL :
			strcpy( Text, "STAUD_STREAM_CONTENT_NULL" );
            break;

        default:
            strcpy( Text, "INVALID STREAM CONTENT" );
            break;
    }
}

/* ######## Stack usage checking ######## */
#if defined (CHECK_STACK_USAGE)

/* Dummy function to calculate OS/20 task stack overhead */
static void test_overhead(void *dummy)
{
    /* Dummy */
}

/* Calculate Init() stack usage */
static void test_init(void *dummy)
{
    static char RetCode;

    AUD_Init(NULL, &RetCode);
}

/* Calculate typical scenario stack usage */
static void test_typical(void *dummy)
{
    static char RetCode;

    AUD_Open(NULL, &RetCode);
    AUD_OPSetAttenuation(NULL, &RetCode);
    AUD_SetChannelDelay(NULL, &RetCode);
    AUD_OPSetSpeaker(NULL, &RetCode);
    AUD_OPSetSpeakerConfig(NULL, &RetCode);
    AUD_DRStart(NULL, &RetCode);
    AUD_OPMute(NULL, &RetCode);
    AUD_DRPause(NULL, &RetCode);
    AUD_DRResume(NULL, &RetCode);
    AUD_DRStop(NULL, &RetCode);
    AUD_Close(NULL, &RetCode);
}

/* Calculate Term() stack usage */
static void test_term(void *dummy)
{
    static char RetCode;

    AUD_Term(NULL, &RetCode);
}

static void stack_test(void)
{
    Task_t task;
    tdesc_t tdesc;
    task_t *task_p;
    task_status_t status;
    char stack[4*1024];
    void (*func_table[])(void *) = {
        test_overhead,
        test_init,
        test_typical,
        test_term,
        NULL
    };
    void (*func)(void *);
    int i = 0;

    task_p = &task;
    while (func_table[i] != NULL)
    {
        func = func_table[i];

        /* Start the task */
        task_init(func,
                  NULL,
                  stack,
                  4*1024,
                  task_p,
                  &tdesc,
                  MAX_USER_PRIORITY,
                  "stack_test",
                  0);

        /* Wait for task to complete */
        task_wait(&task_p, 1, TIMEOUT_INFINITY);

        /* Dump stack usage */
        task_status(task_p, &status, task_status_flags_stack_used);
        STTBX_Print(("STACK CHECK >>>> func=0x%08x stack = %d\n", func,
                     status.task_stack_used));

        /* Tidy up */
        task_delete(task_p);
        i++;
    }
}
#endif




/*---------------------------------------------------------------------------*/
/*                    Synchronization functions                              */
/*---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function : AUD_DisableSynchronisation
 *            Disable synchronisation with PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_DisableSynchronisation( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	boolean RetErr;

	long LVar;
        U8 DeviceId;
        STAUD_Object_t OutputObject;    ObjectString_t OPObject;

	RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
	tag_current_line( pars_p, "expected driver Id" );
	}

    DeviceId = (U8)LVar;
   	sprintf( AUD_Msg, "STAUD_DisableSynchronisation(%u) : ", (U32)AUD_Handle[DeviceId] );

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_PCMP0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPPCMP0, OBJ_OPPCMP1\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    ErrCode = STAUD_DisableSynchronisation(AUD_Handle[DeviceId]);

	RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
	STTBX_Print(( AUD_Msg ));
	assign_integer(result_sym_p, (long)ErrCode, false);
	return ( API_EnableError ? RetErr : FALSE );






} /* end AUD_DisableSynchronisation */

/*-------------------------------------------------------------------------
 * Function : AUD_EnableSynchronisation
 *            Enable synchronisation with PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_EnableSynchronisation( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode;
	boolean RetErr;

	long LVar;
        U8 DeviceId;
        STAUD_Object_t OutputObject;    ObjectString_t OPObject;

	RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
	tag_current_line( pars_p, "expected driver Id" );

	}
	DeviceId = (U8)LVar;

	sprintf( AUD_Msg, "STAUD_EnableSynchronisation(%u) : ", (U32)AUD_Handle[DeviceId] );

	if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_PCMP0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPPCMP0, OBJ_OPPCMP0\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

	ErrCode = STAUD_EnableSynchronisation(AUD_Handle[DeviceId]);

	RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
	STTBX_Print(( AUD_Msg ));
	assign_integer(result_sym_p, (long)ErrCode, false);
	return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_EnableSynchronisation */



/*---------------------------------------------------------------------------*/
/*                       Decoding functions                                  */
/*---------------------------------------------------------------------------*/




/*-------------------------------------------------------------------------
 * Function : AUD_ReceiveEvt
 *            Log the occured event (callback function)
 * Input    :
 * Output   : RcvEventStack[]
 *            NewRcvEvtIndex
 *            NbOfRcvEvents
 *            RcvEventCnt[]
 * Return   :
 * ----------------------------------------------------------------------*/
void AUD_ReceiveEvt (STEVT_CallReason_t Reason,
     STEVT_EventConstant_t Event, void *EventData)
{
    short CountIndex;

    if (Event==STAUD_IN_SYNC)
        printf("STAUDLX in Sync\n");
    if (Event==STAUD_OUTOF_SYNC)
        printf("STAUDLX out of Sync\n");

    if ( NewRcvEvtIndex >= NB_MAX_OF_STACKED_EVENTS ) /* if the stack is full...*/
    {
        return; /* ignore current event */
    }
	if(Reason!=CALL_REASON_NOTIFY_CALL)
	{
		return; /* ignore current event */
	}

    switch(Event)
    {
        case STAUD_NEW_FRAME_EVT:
            if(NULL == EventData)
            {
                break;
            }
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STAUD_PTS_t) );
            CountIndex = 0;
            break;
        case STAUD_DATA_ERROR_EVT:
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(U8) );
            CountIndex = 1;
            break;
        case STAUD_EMPHASIS_EVT:
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STAUD_Emphasis_t) );
            CountIndex = 2;
            break;
        case STAUD_PREPARED_EVT:
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STAUD_PTS_t) );
            CountIndex = 3;
            break;
        case STAUD_RESUME_EVT:
            CountIndex = 4;
            break;
        case STAUD_STOPPED_EVT:
            CountIndex = 5;
            break;
        case STAUD_PCM_UNDERFLOW_EVT:
            CountIndex = 6;
            break;
        case STAUD_FIFO_OVERF_EVT:
            CountIndex = 7;
            break;
        case STAUD_NEW_ROUTING_EVT :
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STAUD_Stereo_t) );
            CountIndex = 8;
            break;
        case STAUD_NEW_FREQUENCY_EVT :
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(U32) );
            CountIndex = 9;
            break;

        case STAUD_CDFIFO_UNDERFLOW_EVT:
             CountIndex = 11;
             break;
        case STAUD_SYNC_ERROR_EVT:
             CountIndex = 12;
             break;

        case STAUD_LOW_DATA_LEVEL_EVT :  /* STAUD_Object_t */
            if((*(STAUD_Object_t*)EventData) == STAUD_OBJECT_INPUT_CD1)
            {
                 STTBX_Print(("****Low Level Event From CD1 ******\n"));
            }
   			 break;
         case STAUD_TEST_DECODER_STOP_EVT:
              AUDi_InjectContext[0]->State = AUD_INJECT_STOP;
              break;



        default:
            CountIndex = -1; /* unknown audio event */
    }
    RcvEventStack[NewRcvEvtIndex].EvtNum = Event;
    NewRcvEvtIndex++;
    /* counting */
    if ( CountIndex >= 0 )
    {
        RcvEventCnt[CountIndex]++;
    }
    NbOfRcvEvents++;
    if ( NbOfRcvEvents == 1 )
    {
        assign_integer("EVENT", Event, false);
    }

} /* end of AUD_ReceiveEvt() */





/*-------------------------------------------------------------------------
 * Function : AUD_EnableEvt
 *            If not already done, init the event handler (open/reg)
 *            If no argument, enable all events (subscribe)
 *                      else, enable one event (subscribe)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_EnableEvt(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STEVT_OpenParams_t OpenParams;
    STEVT_SubscribeParams_t SubscribeParams;
    char EvtMsg[120];
    long LVar;

    /* get event number */
    RetErr = cget_integer( pars_p, -1, &LVar);
    if ( ( RetErr == TRUE )
         || (LVar!=-1 && (LVar < STAUD_NEW_FRAME_EVT || LVar > STAUD_RESERVED_EVT)) )
    {
        sprintf( EvtMsg, "expected event number (%d to %d) or none if all events are requested",
                 STAUD_NEW_FRAME_EVT, STAUD_LOW_DATA_LEVEL_EVT );
        tag_current_line( pars_p, EvtMsg );
        return(API_EnableError ? TRUE : FALSE );
    }

    /* --- Initialization --- */
    if (! EvtInitAlreadyDone)
    {
        RetErr = ST_NO_ERROR;

        assign_integer("EVENT", 0, false); /* testtool variable used for event occurrence control in macro */

        /* Open the event handler for the registrant */
        RetErr = STEVT_Open(STEVT_HANDLER, &OpenParams, &EvtRegHandle);
        if ( RetErr!= ST_NO_ERROR )
        {
            STTBX_Print(( "AUD_EnableEvt() : STEVT_Open() error %d for registrant !\n", RetErr));
        }
        else
        {
            STTBX_Print(( "AUD_EnableEvt() : STEVT_Open(%d,.,.) for registrant done\n",EvtRegHandle));
            EvtInitAlreadyDone = TRUE;
        }

        /* Open the event handler for the subscriber */
        RetErr = STEVT_Open(STEVT_HANDLER, &OpenParams, &EvtSubHandle);
        if ( RetErr!= ST_NO_ERROR )
        {
            STTBX_Print(( "AUD_EnableEvt() : STEVT_Open() error %d for subscriber !\n", RetErr));
        }
        else
        {
            STTBX_Print(( "AUD_EnableEvt() : STEVT_Open(%d,.,.) for subscriber done\n",EvtSubHandle));
        }
        assign_integer("ERRNO", RetErr, false);
    } /* end if init not done */


    /* --- Subscription --- */
    if ((RetErr == ST_NO_ERROR) && EvtInitAlreadyDone)
    {
        SubscribeParams.NotifyCallback = (STEVT_CallbackProc_t)AUD_ReceiveEvt;
        if ( LVar == -1 ) /* if all events required ... */
        {
            RetErr  = STEVT_Subscribe(EvtSubHandle, (U32)STAUD_NEW_FRAME_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_DATA_ERROR_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_EMPHASIS_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_PREPARED_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_RESUME_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_STOPPED_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_PCM_UNDERFLOW_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_FIFO_OVERF_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_NEW_ROUTING_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_NEW_FREQUENCY_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_CDFIFO_UNDERFLOW_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_SYNC_ERROR_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_AVSYNC_SKIP_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_AVSYNC_PAUSE_EVT, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_IN_SYNC, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_OUTOF_SYNC, &SubscribeParams);
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_PTS_EVT, &SubscribeParams);

#ifdef USE_HDD_INJECT
            SubscribeParams.NotifyCallback = AUD_RcvLowDataEvt;
#endif
            RetErr |= STEVT_Subscribe(EvtSubHandle, (U32)STAUD_LOW_DATA_LEVEL_EVT, &SubscribeParams);
            if ( RetErr != ST_NO_ERROR )
            {
                STTBX_Print(( "AUD_EnableEvt() : subscribe events error %d !\n", RetErr));
                EvtEnableAlreadyDone = FALSE;
            }
            else
            {
               STTBX_Print(( "AUD_EnableEvt() : subscribe events done\n"));
                EvtEnableAlreadyDone = TRUE;
            }
        }
        else /* else only 1 event required... */
        {
            RetErr = STEVT_Subscribe(EvtSubHandle, (U32)LVar, &SubscribeParams);
            if ( RetErr != ST_NO_ERROR )
            {
                STTBX_Print(( "AUD_EnableEvt() : subscribe event %ld error %d !\n", LVar, RetErr));
                EvtEnableAlreadyDone = FALSE;
            }
            else
            {
                STTBX_Print(( "AUD_EnableEvt() : subscribe event %ld done\n", LVar));
                EvtEnableAlreadyDone = TRUE;
            }
        }
        assign_integer("ERRNO", RetErr, false);
    }
	assign_integer(result_sym_p,RetErr,false);

    return(API_EnableError ? RetErr : FALSE );

} /* end of AUD_EnableEvt() */

/*-------------------------------------------------------------------------
 * Function : AUD_DeleteEvt
 *            Reset the contents of the event stack
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_DeleteEvt(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
	long LVar;
	U8 DeviceId;
	RetErr = cget_integer( pars_p, 0, &LVar );
	if( RetErr == TRUE )
	{
		tag_current_line( pars_p, "expected DeviceId" );
	}
	DeviceId =  (U8)LVar;

    if ( EvtEnableAlreadyDone == FALSE )
    {
        NewRcvEvtIndex = 0;
        LastRcvEvtIndex = 0;
        NbOfRcvEvents = 0;
        memset(RcvEventStack, 0, sizeof(RcvEventStack));
        memset(RcvEventCnt, 0, sizeof(RcvEventCnt));
        STTBX_Print(( "AUD_DeleteEvt() : events stack purged \n"));
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print(( "AUD_DeleteEvt() : events stack is in used (evt. not disabled) ! \n"));
        RetErr = TRUE;
    }
    assign_integer(result_sym_p,RetErr,false);

    return(API_EnableError ? RetErr : FALSE );

} /* end of AUD_DeleteEvt() */


/*-------------------------------------------------------------------------
 * Function : AUD_DisableEvt
 *            Disable all or only one event (unsubscribe)
 *            Close the event handler (unreg/close/term)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_DisableEvt(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    /*STEVT_TermParams_t TermParams;*/
    char EvtMsg[100];
    long LVar;
    short Cpt;

    /* get event number */
    RetErr = cget_integer( pars_p, -1, &LVar);
	if ( ( RetErr == TRUE )|| (LVar!=-1 && (LVar < STAUD_NEW_FRAME_EVT || LVar > STAUD_LOW_DATA_LEVEL_EVT) ) )
    {
        sprintf( EvtMsg, "expected event number (%d to %d) or none if all events are requested",
                STAUD_NEW_FRAME_EVT, STAUD_LOW_DATA_LEVEL_EVT );
        tag_current_line( pars_p, EvtMsg );
        return(API_EnableError ? TRUE : FALSE );
    }

    /* Disable all unreleased events */
    if ( LVar == -1 ) /* all events */
    {
        RetErr = ST_NO_ERROR;
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_NEW_FRAME_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_DATA_ERROR_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_EMPHASIS_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_PREPARED_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_RESUME_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_STOPPED_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_PCM_UNDERFLOW_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_FIFO_OVERF_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_NEW_ROUTING_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_NEW_FREQUENCY_EVT);
        RetErr |= STEVT_Unsubscribe( EvtSubHandle, (U32)STAUD_LOW_DATA_LEVEL_EVT);
        if ( RetErr != ST_NO_ERROR )
        {
            STTBX_Print(( "AUD_DisableEvt() : unsubscribe events error %d !\n", RetErr));
        }
        else
        {
            STTBX_Print(( "AUD_DisableEvt() : unsubscribe events done\n"));
        }

        /* Close and terminate */

        RetErr = STEVT_Close(EvtRegHandle);
        if ( RetErr!= ST_NO_ERROR )
        {
            STTBX_Print(( "AUD_DisableEvt() : close registrant evt. handler error %d !\n", RetErr));
        }
        else
        {
            STTBX_Print(( "AUD_DisableEvt() : close registrant evt. handler done\n"));
        }
        RetErr = STEVT_Close(EvtSubHandle);
        if ( RetErr!= ST_NO_ERROR )
        {
            STTBX_Print(( "AUD_DisableEvt() : close subscriber evt. handler error %d !\n", RetErr));
        }
        else
        {
            STTBX_Print(( "AUD_DisableEvt() : close subscriber evt. handler done\n"));
            EvtEnableAlreadyDone = FALSE;
            EvtInitAlreadyDone = FALSE;
            for ( Cpt=0; Cpt<NB_OF_AUDIO_EVENTS; Cpt++ )
            {
                EventID[Cpt] = 0xffffffff;
            }
        }
        assign_integer("ERRNO", RetErr, false);
    }
    else
    {
        /* Disable only one event */

        RetErr = STEVT_Unsubscribe( EvtSubHandle, (U32)LVar);
        if ( RetErr != ST_NO_ERROR )
        {
            STTBX_Print(( "AUD_DisableEvt() : unsubscribe event %ld error %d !\n", LVar, RetErr));
        }
        else
        {
            STTBX_Print(( "AUD_DisableEvt() : unsubscribe event %ld done\n", LVar));
            EvtEnableAlreadyDone = FALSE;
        }
        assign_integer("ERRNO", RetErr, false);
    }
    assign_integer(result_sym_p,RetErr,false);
    return(API_EnableError ? RetErr : FALSE );

} /* end of AUD_DisableEvt() */

/*-------------------------------------------------------------------------
 * Function : AUD_ComposeEventMsg
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void AUD_ComposeEventMsg(char *EvtMsg, EventData_t LastEvtData,
        long ReqEvtNo, long ReqEvtData1, long ReqEvtData2)
{
        switch(ReqEvtNo)
            {
                case STAUD_NEW_FRAME_EVT:
                    sprintf( EvtMsg, "STAUD_NEW_FRAME_EVT : PTS.Lo=%d PTS.Hi=%d PTS.Intp=%d \n",
                            LastEvtData.NewFrame.Low,
                            LastEvtData.NewFrame.High,
                            LastEvtData.NewFrame.Interpolated);
                    break;
                case STAUD_DATA_ERROR_EVT:

		            if ((ReqEvtData1!=-1) && (LastEvtData.StreamRouting!=(STAUD_Stereo_t)ReqEvtData1))
                    {
                        sprintf( EvtMsg, "STAUD_DATA_ERROR_EVT : Error=%d instead of %d ! \n",
                                LastEvtData.Error, (U8)ReqEvtData1);
                        API_ErrorCount++;
                    }
                    else
                    {
                        sprintf( EvtMsg, "STAUD_DATA_ERROR_EVT : Error=%d\n", LastEvtData.Error);
                    }

                    break;
                case STAUD_EMPHASIS_EVT:
			        if ((ReqEvtData1!=-1) && (LastEvtData.Emphasis!=(STAUD_Emphasis_t)ReqEvtData1))
                    {
                        sprintf( EvtMsg, "STAUD_EMPHASIS_EVT : Emphasis=%d instead of %ld ! \n",
                           LastEvtData.Emphasis, ReqEvtData1 );
                        API_ErrorCount++;
                    }
                    else
                    {
                        sprintf( EvtMsg, "STAUD_EMPHASIS_EVT : Emphasis=%d \n",
                            LastEvtData.Emphasis );
                    }
                    break;
                case STAUD_PREPARED_EVT:
                    sprintf( EvtMsg, "STAUD_PREPARED_EVT : PTS.Lo=%d PTS.Hi=%d PTS.Intp=%d \n",
                            LastEvtData.NewFrame.Low,
                            LastEvtData.NewFrame.High,
                            LastEvtData.NewFrame.Interpolated);
                    break;
                case STAUD_RESUME_EVT:
                    sprintf( EvtMsg, "STAUD_RESUME_EVT \n");
                    break;
                case STAUD_STOPPED_EVT:
                    sprintf( EvtMsg, "STAUD_STOPPED_EVT \n");
                    break;
                case STAUD_PCM_UNDERFLOW_EVT:
                    sprintf( EvtMsg, "STAUD_PCM_UNDERFLOW_EVT \n");
                    break;
                case STAUD_FIFO_OVERF_EVT:
                    sprintf( EvtMsg, "STAUD_FIFO_OVERF_EVT \n");
                    break;
                case STAUD_LOW_DATA_LEVEL_EVT:
                    sprintf( EvtMsg, "STAUD_LOW_DATA_LEVEL_EVT \n");
                    break;
                case STAUD_NEW_ROUTING_EVT:
			        if ((ReqEvtData1!=-1) && (LastEvtData.StreamRouting!=(STAUD_Stereo_t)ReqEvtData1))
                    {
                        sprintf( EvtMsg, "STAUD_NEW_ROUTING_EVT : Routing mode %d instead of %ld ! \n",
                           LastEvtData.StreamRouting, ReqEvtData1 );
                        API_ErrorCount++;
                    }
                    else
                    {
                        sprintf( EvtMsg, "STAUD_NEW_ROUTING_EVT : Routing mode=%d \n",
                            LastEvtData.StreamRouting );
                    }
                    break;
                case STAUD_NEW_FREQUENCY_EVT:
			        if ((ReqEvtData1!=-1) && (LastEvtData.NewFrequency!=(U32)ReqEvtData1))
                    {
                        sprintf( EvtMsg, "STAUD_NEW_FREQUENCY_EVT : Frequency %d instead of %ld ! \n",
                           LastEvtData.NewFrequency, ReqEvtData1 );
                        API_ErrorCount++;
                    }
                    else
                    {
        				sprintf( EvtMsg, "STAUD_NEW_FREQUENCY_EVT : Frequency=%d, ReqEvtData2=%ld, \n",
        				LastEvtData.NewFrequency,ReqEvtData2 );
                    }
                    break;
                default:
                    break;
            }
} /* end of AUD_ComposeEventMsg() */

/*-------------------------------------------------------------------------
 * Function : AUD_TestEvt
 *            Wait for an event in the stack
 *            Test if it is the expected event number
 *            Test if associated are equal to expected values (optionnal)
 *            Remove the event from the stack
 * Input    :
 * Output   : Command returns 0 if Ok, 1 if failed in result_sym_p
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_TestEvt(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    long ReqEvtNo;
    long ReqEvtData1;
    long ReqEvtData2;
    char EvtMsg[300];
    long Cpt;
    long Old_ErrorCount;
    EventData_t LastEvtData;

    Old_ErrorCount = API_ErrorCount;
    /* get the requested event */
    RetErr = cget_integer( pars_p, -1, &ReqEvtNo);
	if ( ( RetErr == TRUE )|| ((ReqEvtNo < STAUD_NEW_FRAME_EVT || ReqEvtNo > STAUD_LOW_DATA_LEVEL_EVT)) )
    {
        sprintf( EvtMsg, "expected event number (%d to %d)",
            STAUD_NEW_FRAME_EVT, STAUD_LOW_DATA_LEVEL_EVT );
        tag_current_line( pars_p, EvtMsg );
        ReqEvtNo=-1; /* to satisfy error test if ReqEvtNo!=-1 but out of bounds */
    }
    else
    {
        /* get data1 */
        RetErr = cget_integer( pars_p, -1, &ReqEvtData1);
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected data1" );
        }
        else
        {
            /* get data2 */
            RetErr = cget_integer( pars_p, -1, &ReqEvtData2);
            if ( RetErr == TRUE )
            {
                tag_current_line( pars_p, "expected data2" );
            }
        }
    }

    RetErr = RetErr || (ReqEvtNo < STAUD_NEW_FRAME_EVT || ReqEvtNo > STAUD_LOW_DATA_LEVEL_EVT);

    if (RetErr == FALSE)
    {
        /* Wait until the stack is not empty */
        Cpt = 0;
        while ( NbOfRcvEvents == 0 && Cpt < 100 ) /* wait the 1st event */
        {
        	STOS_TaskDelay((signed long)ST_GetClocksPerSecond()/100);
            Cpt++;
        }
        if ( NbOfRcvEvents == 0 && Cpt>= 100 )
        {
            STTBX_Print(( "AUD_TestEvt() : event %ld not happened (timeout) ! \n", ReqEvtNo));
            API_ErrorCount++;
            RetErr = TRUE;
        }
    }

    if (RetErr == FALSE)
    {
        /* Is it the expected event ? */
        if ( RcvEventStack[LastRcvEvtIndex].EvtNum != ReqEvtNo )
        {
            STTBX_Print(( "AUD_TestEvt() : event %ld detected instead of %ld ! \n",
                    RcvEventStack[LastRcvEvtIndex].EvtNum, ReqEvtNo));
            API_ErrorCount++;
        }
        else
        {
            STTBX_Print(( "AUD_TestEvt() : event %ld detected \n", ReqEvtNo ));
            /* trace the last expected event data */
            LastEvtData = RcvEventStack[LastRcvEvtIndex].EvtData;

            AUD_ComposeEventMsg(EvtMsg, LastEvtData, ReqEvtNo, ReqEvtData1, ReqEvtData2);
            STTBX_Print(( EvtMsg));
        } /* end else expected evt */

        /* Purge last element in stack (bottom) */
        interrupt_lock();
        if ( LastRcvEvtIndex < NewRcvEvtIndex )
        {
            LastRcvEvtIndex++;
        }
        NbOfRcvEvents--;
        interrupt_unlock();
        assign_integer("EVENT", RcvEventStack[LastRcvEvtIndex].EvtNum, false);
    }
    /* Command returns 0 if Ok, 1 if failed */
    assign_integer(result_sym_p, (((long)API_ErrorCount)!=Old_ErrorCount), false);

    return(API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : AUD_ShowEvt
 *            Show event counters & stack
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_ShowEvt(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    char EvtMsg[300];
    char EvtMsgDetail[200];
    char Number[10];
    short Cpt;
    long FullDump;

    RetErr = cget_integer( pars_p, 0, &FullDump);
    if ( RetErr == TRUE )
    {
        sprintf( EvtMsg, "TRUE (1) or FALSE (0) expected");
        tag_current_line( pars_p, EvtMsg );
    }

    /* Show event counts */
    sprintf( EvtMsg,
             "AUD_ShowEvt() :\n   NewFrame=%d DataError=%d Emphasis=%d Prepared=%d Resume=%d Stop=%d\nPcmUnderflow=%d FifoOverf=%d NewRouting=%d NewFreq=%d\n   Nb of stacked events=%d",
             RcvEventCnt[0], RcvEventCnt[1], RcvEventCnt[2],
             RcvEventCnt[3], RcvEventCnt[4], RcvEventCnt[5],
             RcvEventCnt[6], RcvEventCnt[7], RcvEventCnt[8], RcvEventCnt[9],
             NbOfRcvEvents);
    if (NbOfRcvEvents>0)
    {
        sprintf(EvtMsgDetail," (index %d to %d)\n", LastRcvEvtIndex,
                ((LastRcvEvtIndex<NewRcvEvtIndex) ? NewRcvEvtIndex-1 : NewRcvEvtIndex));
    }
    else
    {
        sprintf(EvtMsgDetail,"\n");
    }
    strcat(EvtMsg, EvtMsgDetail);

    STTBX_Print(( EvtMsg));

    if (FullDump)
    {
        /* Show events stack with attached parameters */
        sprintf( EvtMsg, "%s", "   Last events : \n   ");
        Cpt = LastRcvEvtIndex;
        while ( Cpt < NbOfRcvEvents )
        {
            sprintf( EvtMsg, "   %02d ",Cpt);
            sprintf( Number, "%ld ", RcvEventStack[Cpt].EvtNum );
            AUD_ComposeEventMsg(EvtMsgDetail, RcvEventStack[Cpt].EvtData, RcvEventStack[Cpt].EvtNum, -1, -1);
            strcat( EvtMsg, Number );
            strcat( EvtMsg, " : " );
            strcat( EvtMsg, EvtMsgDetail );
            STTBX_Print(( EvtMsg));
            Cpt++;
        }
    }
    else
    {
        /* Show events stack */
        sprintf( EvtMsg, "%s", "   Last events : \n   ");
        Cpt = LastRcvEvtIndex;
        while ( Cpt < NbOfRcvEvents )
        {
            if ( Cpt%10==0 ) /* code added to avoid "task stopped" error */
            {
                strcat ( EvtMsg, "\n   " );
                STTBX_Print(( EvtMsg));
                EvtMsg[0] = '\0';
            }
            sprintf( Number, "%ld ", RcvEventStack[Cpt].EvtNum );
            strcat( EvtMsg, Number );
            Cpt++;
        }
        strcat ( EvtMsg, "\n" );
        STTBX_Print(( EvtMsg));
    }
    assign_integer(result_sym_p,RetErr,false);
    return(API_EnableError ? RetErr : FALSE );

} /* end of AUD_ShowEvt() */


/*++++++++aud_wrapper functions+++++++++*/

/*---------------------------------------------------------------------------*/
/*                       Decoding functions                                  */
/*---------------------------------------------------------------------------*/

#if (USE_WRAPPER)
/*
static STAUD_StreamParams_t StartParams =
    {STAUD_STREAM_CONTENT_MPEG1, STAUD_STREAM_TYPE_PES, 44100, STAUD_IGNORE_ID};
*/
/*-------------------------------------------------------------------------
 * Function : AUD_STStart
 *            Start decoding of input stream
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/


static boolean AUD_STStart( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_StartParams_t StartParams;
    char Buff[160];
    boolean RetErr;
    long LVar;
    U8 DeviceId;


    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;

	 /*task_delay(6250000);*/
    /* get Stream Content (default: STAUD_STREAM_CONTENT_MPEG1) */

    RetErr = cget_integer( pars_p, STAUD_STREAM_CONTENT_MPEG1, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( Buff,"expected Decoder ( %d=MPG1 "
                 "%d=MPG2 %d=PCM )",
                 STAUD_STREAM_CONTENT_MPEG1,
                 STAUD_STREAM_CONTENT_MPEG2,
                 STAUD_STREAM_CONTENT_PCM
                 );
        tag_current_line( pars_p, Buff );
    }
    else
    {
        StartParams.StreamContent = (STAUD_StreamContent_t)LVar;


        RetErr = cget_integer( pars_p, STAUD_STREAM_TYPE_PES, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( Buff,"expected Decoder (%d=ES %d=PES "
             "  %d=MPEG1_PACKET )",
                STAUD_STREAM_TYPE_ES,
                STAUD_STREAM_TYPE_PES,
                STAUD_STREAM_TYPE_MPEG1_PACKET );
            tag_current_line( pars_p, Buff );
        }
        else
        {

            StartParams.StreamType = (STAUD_StreamType_t)LVar;

            /* get SamplingFrequency (default: 48000) */
            RetErr = cget_integer( pars_p, 48000, &LVar );
            if ( RetErr == TRUE )
            {
            tag_current_line( pars_p, "expected SamplingFrequency "
                    "(32000, 44100, 48000)" );
            }
            else
            {

                StartParams.SamplingFrequency = (U32)LVar;

            }
        }
    }
              StartParams.StreamID =    STAUD_IGNORE_ID;
	 RetErr = cget_integer( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
   {
      tag_current_line( pars_p, "expected deviceId" );
   }
   DeviceId = (U8)LVar;

	if ( RetErr == FALSE ) /* only start if all parameters are ok */
	{
		RetErr = TRUE;
		sprintf( AUD_Msg, "STAUD_DRStart(%u,(%d, %d, %d, %02.2x)) : ",
				(U32)AUD_Handle[DeviceId], StartParams.StreamContent,
				StartParams.StreamType, StartParams.SamplingFrequency,
				StartParams.StreamID );
		/* set parameters and start */
		ErrCode = STAUD_Start(AUD_Handle[DeviceId],&StartParams );

		/* Start the injector */

		if(AUDi_InjectContext[DeviceId]!=NULL)
		{
			/*We are in memory mode or in PTI Mode */
			AUDi_InjectContext[DeviceId]->State               = AUD_INJECT_START;

			/*Copy Stream Params			*/

			AUDi_InjectContext[DeviceId]->AUD_StartParams.SamplingFrequency  = StartParams.SamplingFrequency;
			AUDi_InjectContext[DeviceId]->AUD_StartParams.StreamContent      = StartParams.StreamContent;
			AUDi_InjectContext[DeviceId]->AUD_StartParams.StreamID           = STAUD_IGNORE_ID;
			AUDi_InjectContext[DeviceId]->AUD_StartParams.StreamType         = StartParams.StreamType;

			/* ------------------ */
			STOS_SemaphoreSignal(AUDi_InjectContext[DeviceId]->SemStart);
		}

	RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
	STTBX_Print(( AUD_Msg ));
	return(ST_NO_ERROR);

	} /* end if */

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRStart */

/*-------------------------------------------------------------------------
 * Function : AUD_DRStop
 *            Stop decoding of input stream
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL AUD_STStop( parse_t *pars_p, char *result_sym_p )
{
	ST_ErrorCode_t ErrCode = ST_NO_ERROR;
	STAUD_Stop_t StopMode;
	STAUD_Fade_t Fade;
	long LVar;
	boolean RetErr;
	STAUD_Object_t DecoderObject;
	U8 DeviceId;
	ObjectString_t DRObject;

    RetErr = cget_integer( pars_p, 0, (long *)&LVar );
	 if(RetErr == TRUE)
	 {
		tag_current_line( pars_p, "expected DeviceId" );
	 }

	 DeviceId = (U8)LVar;

    RetErr = cget_integer( pars_p, STAUD_STOP_NOW, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected StopMode (%d=end of data,%d=now)",
            STAUD_STOP_WHEN_END_OF_DATA, STAUD_STOP_NOW);
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        StopMode = (STAUD_Stop_t)LVar;
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED1, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected Object: OBJ_DRCOMP or OBJ_DRPCM\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            DecoderObject = (STAUD_Object_t)LVar;
            ObjectToText( DecoderObject, &DRObject );
        }
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        sprintf( AUD_Msg, "STAUD_DRStop(%u,%d, %s)) :",
                 (U32)AUD_Handle[DeviceId], StopMode, DRObject);
        ErrCode = STAUD_Stop( AUD_Handle[DeviceId],StopMode, &Fade );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );

        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRStop */

/*-------------------------------------------------------------------------
 * Function : AUD_DRSetStereoOutput
 *            Set the type of the output
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetStereoOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;
    STAUD_StereoMode_t StereoMode;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    	U8 DeviceId;

    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPPCMP0, OBJ_OPPCMP0\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }





    /* get mode (default : STEREO) */
    RetErr = cget_integer( pars_p, STAUD_STEREO_MODE_STEREO, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected MONO|STEREO|PROLOGIC|DLEFT|DRIGHT|DMONO|SECSTE");
    }
    StereoMode = (STAUD_StereoMode_t)LVar;

    sprintf( AUD_Msg, "STAUD_PPSetStereoMode(%u,%d) : ", (U32)AUD_Handle[DeviceId], StereoMode );

    ErrCode = STAUD_SetStereoOutput( AUD_Handle[DeviceId],StereoMode );
    StereoEffect = StereoMode;

    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetStereoOutput */

/*-------------------------------------------------------------------------
 * Function : AUD_DRGetStereoOutput
 *            Get the type of the output
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STGetStereoOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_StereoMode_t StereoMode;
    long LVar;
    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_PCMP0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPPCMP0, OBJ_OPPCMP0\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }






    sprintf( AUD_Msg, "STAUD_PPGetStereoMode(%u) : ", (U32)AUD_Handle[DeviceId]);
    ErrCode = STAUD_GetStereoOutput( AUD_Handle[DeviceId],&StereoMode );

    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        switch ( StereoMode )
        {
          case STAUD_STEREO_MODE_STEREO:
            sprintf( AUD_Msg, "%s Stereo mode \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_PROLOGIC:
            sprintf( AUD_Msg, "%s Stereo prologic mode \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_DUAL_LEFT:
            sprintf( AUD_Msg, "%s Dual left mode \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_DUAL_RIGHT:
            sprintf( AUD_Msg, "%s Dual right mode \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_DUAL_MONO:
            sprintf( AUD_Msg, "%s Dual Mono \n", AUD_Msg);
            break;
          case STAUD_STEREO_MODE_SECOND_STEREO:
            sprintf( AUD_Msg, "%s Second Stereo Mode \n", AUD_Msg);
            break;

        }
    }
    STTBX_Print(( AUD_Msg ));
    if(StereoEffect== StereoMode)
    {
    sprintf( AUD_Msg, "%s Stereo Effect-Pass \n", AUD_Msg);
    STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetStereoOutput */


/*-------------------------------------------------------------------------
 * Function : AUD_DRSetPrologic
              Force the prologic state of the decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetPrologic( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;
    ErrCode = ST_NO_ERROR;
    boolean PrologicVal;
    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
    DeviceId = (U8)LVar;

     if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }


    RetErr = cget_integer( pars_p, 1, (long *)&LVar );
    PrologicVal = (boolean)LVar ;

    if( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Parameter required, 0=OFF, 1=0N");
    }
    else
    {
        sprintf( AUD_Msg, "STAUD_SetPrologic(%u,Object=%s Pro=%d) : ", (U32)AUD_Handle[DeviceId],&OPObject,(int)LVar);
        ErrCode = STAUD_SetPrologic(AUD_Handle[DeviceId]);

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetPrologic */


/*-------------------------------------------------------------------------
 * Function : AUD_DRGetPrologic
              Retrieves the prologic state of the audio decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STGetPrologic( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL PrologicState;
    boolean RetErr;
    U8 DeviceId;
    long LVar;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
    DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    sprintf( AUD_Msg, "STAUD_GetPrologic(%u) Object(%s): ", (U32)AUD_Handle[DeviceId],ObjectToText);

    ErrCode = STAUD_GetPrologic(AUD_Handle[DeviceId], &PrologicState);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s PrologicState:%d", AUD_Msg, PrologicState);
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, PrologicState, false);

    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetPrologic */


/*-------------------------------------------------------------------------
 * Function : AUD_PPSetSpeakerConfig
              set the speakers configuration (speaker size)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetSpeakerConfig( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_SpeakerConfiguration_t Speaker;
    long LVar;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;
    U8 DeviceId;

    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
    DeviceId = (U8)LVar;

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPMULTI, OBJ_OPSPDIF\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    RetErr = cget_integer( pars_p, 2, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Front speakers size expected, 1:SMALL 2:LARGE");
    }
    if (RetErr == FALSE)
    {
        Speaker.Front = (STAUD_SpeakerType_t) LVar;
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 2, (long *)&LVar );
        if ( RetErr == TRUE )
        {
        tag_current_line( pars_p, "Center speaker size expected, 1:SMALL 2:LARGE");
        }
        if (RetErr == FALSE)
        {
            Speaker.Center = (STAUD_SpeakerType_t) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 1, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Left Surround speaker size expected, 1:SMALL 2:LARGE");
        }
        if (RetErr == FALSE)
        {
            Speaker.LeftSurround = (STAUD_SpeakerType_t) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 1, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right Surround speaker size expected, 1:SMALL 2:LARGE");
        }
        if (RetErr == FALSE)
        {
            Speaker.RightSurround = (STAUD_SpeakerType_t) LVar;
        }
    }


    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_PPSetSpeakerConfig(%u,%s,%d,%d,%d,%d) : ",
                (U32)AUD_Handle[DeviceId], OPObject, Speaker.Front, Speaker.Center,
                Speaker.LeftSurround, Speaker.RightSurround);
        ErrCode = STAUD_SetSpeakerConfiguration(AUD_Handle[DeviceId], Speaker);

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    } /* end if */

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetSpeakerConfig */


/*-------------------------------------------------------------------------
 * Function : AUD_PPGetSpeakerConfig
              Retrieves the speakers size configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STGetSpeakerConfig( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_SpeakerConfiguration_t Speaker;
    boolean RetErr;
    long LVar;
    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
    if ( RetErr == TRUE )
	{
	      tag_current_line( pars_p, "expected deviceId" );
	 }
   DeviceId = (U8)LVar;

   if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_OPPCMP0, OBJ_OPPCMP1\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    sprintf( AUD_Msg, "STAUD_PPGetSpeakerConfiguration(%u) : ", (U32)AUD_Handle[DeviceId]);

    ErrCode = STAUD_GetSpeakerConfiguration(AUD_Handle[DeviceId],&Speaker);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s (Front:%d, Center:%d, LSurr:%d, RSurr:%d\n",
                 AUD_Msg, Speaker.Front, Speaker.Center,
                 Speaker.LeftSurround, Speaker.RightSurround);
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetSpeakerConfig */


/*-------------------------------------------------------------------------
 * Function : AUD_DRSetEffect
              Set the stereo effect configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetEffect( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Effect_t Effect;
    char Buff[80];
    long LVar;
    boolean RetErr;
    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    ErrCode = ST_NO_ERROR;
    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }


    RetErr = cget_integer( pars_p, STAUD_EFFECT_SRS_3D, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        sprintf( Buff,"expected effect SRS_3D (3D Effect), TRU (Trusurround), BAS (TruBASS), FOCUS (Focus), XT (TruSurXT) or NONE\n" );
        tag_current_line( pars_p, Buff );
    }
    if (RetErr == FALSE)
    {
        Effect = (STAUD_Effect_t)LVar;
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_DRSetEffect(%u, Object Id[%d], effect[%d]) : ", (U32)AUD_Handle[DeviceId],OPObject, Effect);

         ErrCode = STAUD_SetEffect(AUD_Handle[DeviceId], Effect);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetEffect */


/*-------------------------------------------------------------------------
 * Function : AUD_PPGetEffect
              Retrieves the stereo effect configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STGetEffect( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Effect_t Effect;
    boolean RetErr;
    long LVar;
    ErrCode = ST_NO_ERROR;

    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;


    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
    DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    sprintf( AUD_Msg, "STAUD_DRGetEffect(%u) obj= [%s] & efect[%d]:\n ", (U32)AUD_Handle[DeviceId], OPObject, &Effect);
    ErrCode = STAUD_GetEffect(AUD_Handle[DeviceId], &Effect);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        switch( Effect )
        {
            case STAUD_EFFECT_NONE:
                strcat( AUD_Msg, " NONE\n");
                break;
            case STAUD_EFFECT_SRS_3D:
                strcat( AUD_Msg, " SRS 3D\n");
                break;
            case STAUD_EFFECT_TRUSURROUND:
                strcat( AUD_Msg, " TRUE SURROUND\n");
                break;
            case STAUD_EFFECT_SRS_TRUBASS:
                strcat( AUD_Msg, " TRUE BASS\n");
                break;
            case STAUD_EFFECT_SRS_FOCUS:
                strcat( AUD_Msg, " TRUE FOCUS\n");
                break;
            case STAUD_EFFECT_SRS_TRUSUR_XT:
                strcat( AUD_Msg, " TRUE FOCUS\n");
                break;
            default:
                sprintf( AUD_Msg, " %s Unknown effect %d\n", AUD_Msg, Effect);
                break;
        }
    }


    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetEffect */

/*-------------------------------------------------------------------------
 * Function : AUD_DRSetDynRange
              set the dynamic range configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetDynamicRange( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode= ST_NO_ERROR;
    U8 Control;
    long LVar;
    boolean RetErr;
    STAUD_DynamicRange_t DynamicRange;
    U8 DeviceId;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }


    sprintf( AUD_Msg, "STAUD_DRGetDynamicRange(%u  %s) : ", (U32)AUD_Handle[DeviceId],&OPObject );
	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, 0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "[0,1] integer value expected.\n");
		}
		else
		{
			DynamicRange.Enable = (BOOL)LVar;

		}
	}

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, 0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "[0..255] integer value expected \n");
		}
		else
		{
			DynamicRange.CutFactor = (U8)LVar;

		}
	}

	if(RetErr == FALSE)
	{
		RetErr = cget_integer( pars_p, 0, (long *)&LVar );
		if ( RetErr == TRUE )
		{
			tag_current_line( pars_p, "[0..255] integer value expected.\n");
		}
		else
		{
			DynamicRange.BoostFactor = (U8)LVar;

		}
	}

	if ( RetErr == FALSE ) /* if parameters are ok */
	{
	RetErr = TRUE;
	sprintf( AUD_Msg, "STAUD_DRSetDynamicRange(%u,%d) : ", (U32)AUD_Handle[DeviceId], DynamicRange.Enable);

	    ErrCode = STAUD_SetDynamicRangeControl(AUD_Handle[DeviceId], &DynamicRange);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRSetDynRange */
/*-------------------------------------------------------------------------
 * Function : AUD_DRGetDynRange
              Retrieves the dynamic range configuration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STGetDynamicRange( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_DynamicRange_t DynamicRange;
    U8 DeviceId;
    long LVar;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_DRCOMP0, OBJ_DRCOMP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    sprintf( AUD_Msg, "STAUD_DRGetDynamicRange(%u  %s) : ", (U32)AUD_Handle[DeviceId],&OPObject );
    /* get channel delay parameters  */
    ErrCode = STAUD_GetDynamicRangeControl(AUD_Handle[DeviceId], &DynamicRange);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s Enable: %d CutFactor: %d BoostFacot: %d", AUD_Msg, DynamicRange.Enable,DynamicRange.CutFactor,DynamicRange.BoostFactor);
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRGetDynRange */

static BOOL AUD_STMuteAnaDig( parse_t *pars_p, char *result_sym_p )
{

    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    long LVar = 0;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    U8 DeviceId = 0;
    U8 AnalogMute = 0;
    U8 DigitalMute = 0;

    RetErr = cget_integer( pars_p,0, (long *)&LVar );

    if ( RetErr == TRUE )
       {
	 tag_current_line( pars_p, "expected deviceId" );
	}
   	DeviceId = (U8)LVar;

    RetErr = cget_integer( pars_p, 0, (long *)&LVar );
    if ( RetErr == TRUE )
       {
	 tag_current_line( pars_p, "expected value of analog mute" );
	}
   	AnalogMute = (U8)LVar;

    RetErr = cget_integer( pars_p, 0, (long *)&LVar );

    if ( RetErr == TRUE )
       {
	 tag_current_line( pars_p, "expected value of digital mute" );
	}
   	DigitalMute = (U8)LVar;

     ErrCode = STAUD_Mute(AUD_Handle[DeviceId],AnalogMute,DigitalMute);

    assign_integer(result_sym_p, (long)ErrCode, false);




    return ( API_EnableError ? RetErr : FALSE );
}


/*-------------------------------------------------------------------------
 * Function : AUD_PPGetAttenuation

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STGetAttenuation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Attenuation_t Attenuation;
    boolean RetErr;
    long LVar;
    STAUD_Object_t OutPutObject;
    ObjectString_t OPObject;
    U8 DeviceId;

    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if (RetErr == false)
    {
    RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected Object: OBJ_PP0 or OBJ_PP1 or OBJ_PP2\n" );
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        OutPutObject = (STAUD_Object_t)LVar;
        ObjectToText( OutPutObject, &OPObject );
    }
   }
    sprintf( AUD_Msg, "STAUD_PPGetAttenuation(%u)) : ",(U32)AUD_Handle[DeviceId]);

    ErrCode = STAUD_GetAttenuation( AUD_Handle[DeviceId], &Attenuation);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s Left %d, Right %d, "
                 "LSurr %d, RSurr %d, Center %d, Sub %d Output %s\n", AUD_Msg,
                 Attenuation.Left, Attenuation.Right,
                 Attenuation.LeftSurround, Attenuation.RightSurround,
                 Attenuation.Center, Attenuation.Subwoofer, OPObject);
    }
    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPGetAttenuation */



/*-------------------------------------------------------------------------
 * Function : AUD_PPCompareAttenuation

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STCompareAttenuation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Attenuation_t Attenuation;
    boolean RetErr;
    long LVar;
    STAUD_Object_t OutPutObject;
    ObjectString_t OPObject;

    U8 DeviceId;

    	 RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;


    RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected Object: OBJ_PP0 or OBJ_PP1\n" );
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        OutPutObject = (STAUD_Object_t)LVar;
        ObjectToText( OutPutObject, &OPObject );
    }

    sprintf( AUD_Msg, "Device ID(%u)) : ",(U32)AUD_Handle[DeviceId]);


    ErrCode = STAUD_GetAttenuation( AUD_Handle[DeviceId], &Attenuation);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "%s Left %d, Right %d, "
                 "LSurr %d, RSurr %d, Center %d, Sub %d Output %s\n", AUD_Msg,
                 Attenuation.Left, Attenuation.Right,
                 Attenuation.LeftSurround, Attenuation.RightSurround,
                 Attenuation.Center, Attenuation.Subwoofer, OPObject);
    }
    STTBX_Print(( AUD_Msg ));

    if(LeftAttenuation ==  Attenuation.Left)
    {
    sprintf( AUD_Msg, "Pass--Left Atten\n ",AUD_Msg, Attenuation.Left) ;
    STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Left Atten\n ",AUD_Msg, Attenuation.Left) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }



    if(RightAttenuation ==  Attenuation.Right)
    {
    	sprintf( AUD_Msg, "Pass--Right Atten\n ",AUD_Msg, Attenuation.Right) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Right Atten\n ",AUD_Msg, Attenuation.Right) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }

      if(LeftSurroundAttenuation ==  Attenuation.LeftSurround)
    {
    	sprintf( AUD_Msg, "Pass--Left Surround Atten\n ",AUD_Msg, Attenuation.LeftSurround) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Left Surround Atten\n ",AUD_Msg, Attenuation.LeftSurround) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }


      if(RightSurroundAttenuation ==  Attenuation.RightSurround)
    {
    	sprintf( AUD_Msg, "Pass--Right Surround Atten\n ",AUD_Msg, Attenuation.RightSurround) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Right Surround Atten\n ",AUD_Msg, Attenuation.RightSurround) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }


      if(CenterAttenuation == Attenuation.Center)
      {
    	sprintf( AUD_Msg, "Pass--Centre Atten\n ",AUD_Msg, Attenuation.Center) ;
    	STTBX_Print(( AUD_Msg ));
      }
      else
      {
    	sprintf( AUD_Msg, "Fail--Centtre Atten\n ",AUD_Msg,Attenuation.Center) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
      }

       if(SubwooferAttenuation ==  Attenuation.Subwoofer)
       {
    	sprintf( AUD_Msg, "Pass--Subwoofer Atten\n ",AUD_Msg, Attenuation.Subwoofer) ;
    	STTBX_Print(( AUD_Msg ));
       }
       else
      {
    	sprintf( AUD_Msg, "Fail--Subwoofer Atten\n ",AUD_Msg,Attenuation.Subwoofer) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
      }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPGetAttenuation */




/*-------------------------------------------------------------------------
 * Function : AUD_PPSetAttenuation

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetAttenuation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Attenuation_t Attenuation;
    long LVar;
    boolean RetErr;


    U8 DeviceId;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, 0, &LVar );

    if ( RetErr == TRUE )
	{
	      tag_current_line( pars_p, "expected deviceId" );
	 }
    DeviceId = (U8)LVar;

    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_PP0, OBJ_PP0\n");
        }
        else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    /* get attenuation Left */
    RetErr = cget_integer( pars_p, 0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Left front speaker expected Attenuation" );
    }
    Attenuation.Left = (U16)LVar;

    /* get attenuation Right */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right front speaker expected Attenuation" );
        }
        Attenuation.Right = (U16)LVar;
    }

    /* get attenuation LeftSurround */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Left surround speaker expected Attenuation" );
        }
        Attenuation.LeftSurround = (U16)LVar;
    }

    /* get attenuation RightSurround */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right surround speaker expected Attenuation" );
        }
        Attenuation.RightSurround = (U16)LVar;
    }

    /* get attenuation Center */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Center speaker expected Attenuation" );
        }
        Attenuation.Center = (U16)LVar;
    }

    /* get attenuation Subwoofer */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Subwoofer speaker expected Attenuation" );
        }
        Attenuation.Subwoofer = (U16)LVar;
    }

    /* Get Output object */
    /*
    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "expected Object: OBJ_OPMULTI or OBJ_OPDAC\n" );
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            OutPutObject = (STAUD_Object_t)LVar;
            ObjectToText( OutPutObject, &OPObject );
        }
    }
*/
    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        sprintf( AUD_Msg, "STAUD_OPSetAttenuation(%u,%s,%d,%d,%d,%d,%d,%d) : ",
            (U32)AUD_Handle[DeviceId],&OPObject,Attenuation.Left, Attenuation.Right,
            Attenuation.LeftSurround, Attenuation.RightSurround,
            Attenuation.Center, Attenuation.Subwoofer);
        /* set parameters and start */

        ErrCode = STAUD_SetAttenuation( AUD_Handle[DeviceId],Attenuation);

        LeftAttenuation = Attenuation.Left;
        RightAttenuation = Attenuation.Right;
        CenterAttenuation = Attenuation.Center;
        LeftSurroundAttenuation = Attenuation.LeftSurround;
        RightSurroundAttenuation = Attenuation.RightSurround;
        SubwooferAttenuation = Attenuation.Subwoofer;

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_SetAttenuation */



/*-------------------------------------------------------------------------
 * Function : AUD_STSetChannelDelay
              set the delay in ms for each channel
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STSetChannelDelay( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Delay_t DelayConfig;
    long LVar;
    boolean RetErr;
    STAUD_Object_t LocalObject;
    ObjectString_t TextObject;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;
    ErrCode = ST_NO_ERROR;
    U8 DeviceId;
    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_PP0, OBJ_PP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }


    RetErr = cget_integer( pars_p, 0, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Left speaker delay expected.");
    }
    if (RetErr == FALSE)
    {
        DelayConfig.Left = (U16) LVar;
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.Right = (U16) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 5, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Left Surround speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.LeftSurround = (U16) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 5, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right Surround speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.RightSurround = (U16) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Center speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.Center = (U16) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        /* default no delay as indicated in the STAUD API documentation v1.2.0*/
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Subwoofer speaker delay expected.");
        }
        if (RetErr == FALSE)
        {
            DelayConfig.Subwoofer = (U16) LVar;
        }
    }

    /* It is for Multipcm only */
    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        /*LocalObject = STAUD_OBJECT_POST_PROCESSOR0;
        ObjectToText( LocalObject, &TextObject );*/
        sprintf( AUD_Msg, "STAUD_PPSetDelay(%u,%s,%d,%d,%d,%d,%d,%d,%s) : ",
        (U32)AUD_Handle[DeviceId], TextObject,DelayConfig.Left, DelayConfig.Right,
        DelayConfig.LeftSurround, DelayConfig.RightSurround,
        DelayConfig.Center, DelayConfig.Subwoofer, OPObject);

        ErrCode = STAUD_SetChannelDelay(AUD_Handle[DeviceId], DelayConfig);

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    LeftChDelay=DelayConfig.Left;
    RightChDelay=DelayConfig.Right;
    LeftSurroundChDelay = DelayConfig.LeftSurround;
    RightSurroundChDelay = DelayConfig.RightSurround;
    CenterChDelay = DelayConfig.Center;
    SubwooferChDelay = DelayConfig.Subwoofer;
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPSetChannelDelay */


/*-------------------------------------------------------------------------
 * Function : AUD_PPGetChannelDelay
              Retrieves the delay in ms set for each channel
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STGetChannelDelay( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Delay_t DelayConfig;
    boolean RetErr;
    STAUD_Object_t LocalObject;
    ObjectString_t TextObject;
    U8 DeviceId;
    long LVar;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_PP0, OBJ_PP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }

    /* It is for Multipcm only */
    /*LocalObject = STAUD_OBJECT_POST_PROCESSOR0;
    ObjectToText( LocalObject, &TextObject );*/

    sprintf( AUD_Msg, "STAUD_PPGetDelay(%u,%s,%d,%d,%d,%d,%d,%d,%s) : ",
    (U32)AUD_Handle[DeviceId], OPObject,DelayConfig.Left, DelayConfig.Right,
    DelayConfig.LeftSurround, DelayConfig.RightSurround,
    DelayConfig.Center, DelayConfig.Subwoofer, OPObject);



    ErrCode = STAUD_GetChannelDelay(AUD_Handle[DeviceId], &DelayConfig );

    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPGetChannelDelay */




/*-------------------------------------------------------------------------
 * Function : AUD_PPCompareChannelDelay
              Compares the delay for each channel
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_STCompareChannelDelay( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Delay_t DelayConfig;
    boolean RetErr;
    STAUD_Object_t LocalObject;
    ObjectString_t TextObject;
    U8 DeviceId;
     long LVar;
    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, 0, &LVar );

    	 if ( RetErr == TRUE )
	   {
	      tag_current_line( pars_p, "expected deviceId" );
	   }
   	DeviceId = (U8)LVar;
    if(RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, STAUD_OBJECT_POST_PROCESSOR0, (long *)&LVar );
    if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Object expected: OBJ_PP0, OBJ_PP1\n");
        }
    else
        {
            OutputObject = (STAUD_Object_t)LVar;
            ObjectToText( OutputObject, &OPObject);
        }
    }



    sprintf( AUD_Msg, "STAUD_DRCompareChannelDynamicRange(%u  %s ]) : ", (U32)AUD_Handle[DeviceId],&OPObject );

    /* It is for Multipcm only */
    /*
    LocalObject = STAUD_OBJECT_POST_PROCESSOR0;
    ObjectToText( LocalObject, &TextObject );
    */
    sprintf( AUD_Msg, "STAUD_PPGetDelay(%u,%s,%d,%d,%d,%d,%d,%d,%s) : ",
    (U32)AUD_Handle[DeviceId], OutputObject,DelayConfig.Left, DelayConfig.Right,
    DelayConfig.LeftSurround, DelayConfig.RightSurround,
    DelayConfig.Center, DelayConfig.Subwoofer, TextObject);


    ErrCode = STAUD_GetChannelDelay(AUD_Handle[DeviceId], &DelayConfig );
    if(LeftChDelay ==  DelayConfig.Left)
    {
    	sprintf( AUD_Msg, "Pass--Left Channel Delay\n ",AUD_Msg, DelayConfig.Left) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Left Channel Delay\n ",AUD_Msg, DelayConfig.Left) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }



    if(RightChDelay ==  DelayConfig.Right)
    {
    	sprintf( AUD_Msg, "Pass--Right Channel Delay\n ",AUD_Msg, DelayConfig.Right) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Right Channel Delay\n ",AUD_Msg, DelayConfig.Left) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }

     if(LeftSurroundChDelay ==  DelayConfig.LeftSurround)
    {
    	sprintf( AUD_Msg, "Pass--Right Surround Delay\n ",AUD_Msg, DelayConfig.LeftSurround) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Left Surround Delay\n ",AUD_Msg, DelayConfig.LeftSurround) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }


     if(RightSurroundChDelay ==  DelayConfig.RightSurround)
    {
    	sprintf( AUD_Msg, "Pass--Right Surround Channel Delay\n ",AUD_Msg, DelayConfig.RightSurround) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Right Surround Delay\n ",AUD_Msg, DelayConfig.RightSurround) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }

     if(CenterChDelay ==  DelayConfig.Center)
    {
    	sprintf( AUD_Msg, "Pass--Center Channel Delay\n ",AUD_Msg, DelayConfig.Center) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Center Channel Delay\n ",AUD_Msg, DelayConfig.Center) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }

     if(SubwooferChDelay ==  DelayConfig.Subwoofer)
    {
    	sprintf( AUD_Msg, "Pass--Subwoofer Channel Delay\n ",AUD_Msg, DelayConfig.Subwoofer) ;
    	STTBX_Print(( AUD_Msg ));
    }
    else
    {
    	sprintf( AUD_Msg, "Fail--Subwoofer Delay\n ",AUD_Msg, DelayConfig.Subwoofer) ;
    	STTBX_Print(( AUD_Msg ));
    	return true;
     }



    STTBX_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_PPCompareChannelDelay */







#endif

#ifdef ST_OSLINUX

static ST_ErrorCode_t AUD_Link(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ST_ErrorCode = ST_NO_ERROR;
    STAUD_BufferParams_t    BufferParams;
    U32 DeviceId;
    BOOL RetErr = TRUE;
    U32 DeviceNb = 0;

	RetErr = cget_integer( pars_p, 0, &DeviceId );
	if (RetErr == TRUE)
    {
       tag_current_line( pars_p, "expected deviceId" );
    }

    /* Get the PES Buffer params */
    ST_ErrorCode = STAUD_IPGetInputBufferParams(AUD_Handle[DeviceId], STAUD_OBJECT_INPUT_CD0, &BufferParams);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print (("AUD_GetDataInputBufferParams fails \n"));
        printf("Error:%x\n",ST_ErrorCode);
        return(TRUE);
    }
    else
    {
    	STTBX_Print(("BufferBaseAddr_p(0x%x) BufferSize(%d)\n", BufferParams.BufferBaseAddr_p, BufferParams.BufferSize));
    }

    ST_ErrorCode = STPTI_BufferAllocateManual(STPTIHandle[DeviceNb],
                                                BufferParams.BufferBaseAddr_p, BufferParams.BufferSize,
                                                1, &PTI_AudioBufferHandle_p[0]);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print (("PTI_BufferAllocateManual fails \n"));
        return(TRUE);
    }

    /* No cd-fifo: Initialise the link between pti and Audio */
    ST_ErrorCode = STPTI_SlotLinkToBuffer(SlotHandle[DeviceNb][PTICMD_BASE_AUDIO], PTI_AudioBufferHandle_p[0]);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print (("PTI_SlotLinkToBuffer fails \n"));
        return(TRUE);
    }

    ST_ErrorCode = STPTI_BufferSetOverflowControl(PTI_AudioBufferHandle_p[0], TRUE);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print (("PTI_BufferSetOverflowControl fails \n"));
        return(ST_ErrorCode);
    }

	ST_ErrorCode = STAUD_IPSetDataInputInterface(AUD_Handle[DeviceId],STAUD_OBJECT_INPUT_CD0,
                                                NULL, NULL, (void*)PTI_AudioBufferHandle_p[0]);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print (("STAUD_IPSetDataInputInterface fails \n"));
        return(TRUE);
    }

    printf("Audio Linked OK\n");

    return(ST_ErrorCode);
 } /* end of AUD_Link() */

#endif









/*#######################################################################*/
/*########################### AUDIO COMMANDS ############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : AUD_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
static boolean AUD_InitCommand (void)
{
boolean RetErr;

    RetErr = FALSE;
    RetErr |= register_command( "AUD_CLOSE",       AUD_Close, "Close");
    RetErr |= register_command( "AUD_DRSETSPEED",  AUD_SetSpeed,  "<Speed> Sets trick mode speed");
    RetErr |= register_command( "AUD_GETCAPA",     AUD_GetCapability, "Get driver's capability");
    RetErr |= register_command( "AUD_DRGETINFO",   AUD_DRGetStreamInfo, "Get current stream info");
    RetErr |= register_command( "AUD_CHECKINFO",   AUD_CheckStreamInfo, "Get current stream info");
    RetErr |= register_command( "AUD_ISCDDACAPABLE", AUD_IsCDDACapable, "Returns true if decoder is CD-DA capable ");
	RetErr |= register_command( "AUD_DRGetSamplingFrequency", AUD_DRGetSamplingFrequency, "Returns sampling Frequency  ");
    RetErr |= register_command( "AUD_INIT",        AUD_Init, "[<DeviceName>] [<audio base address>] Initialisation, []:optional");
    RetErr |= register_command( "AUD_OPEN",        AUD_Open, "<DeviceName> [SyncDelay] Share physical decoder usage, SyncDelay optional");
    RetErr |= register_command( "AUD_DRPAUSE",       AUD_DRPause, "Pause the decoding");
    RetErr |= register_command( "AUD_DRRESUME",      AUD_DRResume, "Resume a previously paused decoding");

    RetErr |= register_command( "AUD_REV",         AUD_GetRevision, "Get driver revision number");
    RetErr |= register_command( "AUD_MXSetParams",  AUD_MXSetInputParams, "Mixlevel, Object: e.g. 50, PCMPLAYER");
    RetErr |= register_command( "AUD_TERM",         AUD_Term, "<DeviceName> <ForceTerminate> Terminate");
	RetErr |= register_command( "AUD_OPENABLEHDMI",  AUD_OPEnableHdmi, " <Object> EnableHdmi");
    RetErr |= register_command( "AUD_GETFILE",     AUD_Load, "Load a file from disk to memory");
    RetErr |= register_command( "AUD_GETFILE_DYNAMIC",     AUD_Load_Dynamic, "Load a file from disk to memory dynamically");
    RetErr |= register_command( "AUD_INJECT",      AUD_MemInject,  "Create DMA Inject Audio in memory Task");
	RetErr |= register_command( "AUD_REGFRAMEPROCESS",      AUD_RegFrameProcess,  "register Frame deliver process");
    RetErr |= register_command( "AUD_PCMINJECT",      AUD_PCMInject,  "Create DMA Inject Audio in memory Task");
    RetErr |= register_command( "AUD_STPAUSE",       AUD_STPause, "Pause the decoding");
    RetErr |= register_command( "AUD_STRESUME",      AUD_STResume, "Resume a previously paused decoding");
    RetErr |= register_command( "AUD_KILL",        AUD_KillTask,  "Kill Audio Inject Task");
	RetErr |= register_command( "AUD_PCMKILL",        AUD_KillPCMTask,  "Kill PCM Inject Task");
	RetErr |= register_command( "AUD_STOPINJ",		AUD_StopInjection,	"Stop Audio Injection");
    RetErr |= register_command( "AUD_VERBOSE",     AUD_Verbose, "<TRUE |FALSE> makes functions print details or not");
	RetErr |= register_command( "AUD_CELLTYPE",    AUD_GetCellType, "Return audio decoder cell type (1:MPEG1, 2:MMDSP, 3:AMPHION , 4:LX)");
    RetErr |= register_command( "AUD_STBDVD",      AUD_STBDVD,      "Return compile otpion used (1:STB or 2:DVD)");
    RetErr |= register_command( "AUD_IPSETPARAMS",  AUD_IPSetParams, "Connects decoder object to either the CD1, CD2 or PCM inputs: Decoder object, Input object");
    RetErr |= register_command( "AUD_IPGETPARAMS",  AUD_IPGetParams, "Returns input parameters");
    RetErr |= register_command( "AUD_DRENABLEDS",  AUD_DREnableDownsampling, " <Object> Enables downsampling");
    RetErr |= register_command( "AUD_DRDISABLEDS",  AUD_DRDisableDownsampling, " <Object> Disables downsampling");


    RetErr |= register_command( "AUD_RESETHANDLE",  AUD_ResetHandle, "Sets handle to zero");
    RetErr |= register_command( "AUD_SETHANDLE",  AUD_SetHandle, " <DeviceName> Sets handle as required by the user");
    RetErr |= register_command( "AUD_GETHANDLE",  AUD_GetHandle, " Returns the last opened handle ");
    RetErr |= register_command( "AUD_SYNC",            AUD_EnableSynchronisation, "Enable video synchronisation with PCR");
    RetErr |= register_command( "AUD_NOSYNC",          AUD_DisableSynchronisation, "Disable the video synchronisation with PCR");
    RetErr |= register_command( "AUD_DRMUTE",  AUD_DRMute, " <Object> Mutes output");
    RetErr |= register_command( "AUD_DRUNMUTE",  AUD_DRUnMute, " <Object> Unmutes output");

    RetErr |= register_command( "AUD_EVT",         AUD_EnableEvt, "<EVT> watch specified event or all events if no parameter");
    RetErr |= register_command( "AUD_DELEVT",      AUD_DeleteEvt, "Delete events report stack");
    RetErr |= register_command( "AUD_NOEVT",       AUD_DisableEvt,"<EVT> Disable 1 or all events");
    RetErr |= register_command( "AUD_SHOWEVT",     AUD_ShowEvt,   "Show counts + events");
    RetErr |= register_command( "AUD_TESTEVT",     AUD_TestEvt,   "<EVT><DATA1><DATA2> Wait for an event");

	RetErr |= register_command( "AUD_STMUTE",  AUD_STMute, " <Object> Mutes output");
	RetErr |= register_command( "AUD_STSETDIGOUT", AUD_STSetDigitalOutput , "ST SET DigitalO/P");
	RetErr |= register_command( "AUD_SetTempo", AUD_DRSetTempo , "ST SET tempo");
    RetErr |= register_command( "AUD_SETDOWNMIX", AUD_DRSetDownmix , "ST SET Downmix");
	RetErr |= register_command( "AUD_STGETDIGOUT", AUD_STGetDigitalOutput , "ST GET DigitalO/P");
	RetErr |= register_command( "AUD_DRSETCOMPRESSIONMODE", AUD_DRSetCompressionMode , "ST GET Compression Mode/P");
	RetErr |= register_command( "AUD_DRSETCODINGMODE", AUD_DRSetAudioCodingMode , "ST GET Compression Mode/P");
	RetErr |= register_command( "AUD_DRSETBEEPTONEFREQ", AUD_DRSetBeepToneFrequency, "ST Set Beep Tone");
	RetErr |= register_command( "AUD_DRGETBEEPTONEFREQ", AUD_DRGetBeepToneFrequency, "ST Get Beep Tone");
	RetErr |= register_command( "AUD_DRSETMIXERLEVEL", AUD_DRSetMixerLevel , "ST GET Mixer Level");
	RetErr |= register_command( "AUD_STSETPCMIP", AUD_STSetPCMInputParams , "ST SET PCM Input Params");
	RetErr |= register_command( "AUD_STSETLATENCY", AUD_STSetLatency , "ST SET Latency in OP");
	RetErr |= register_command( "AUD_STSETEOF", AUD_STSetEOF , "ST SET EOF in IP");
#if defined(ST_OS21)
    /*RetErr |= register_command( "AUD_PARTITION", AUD_GetPartitionStatus, "ST Audio Memory Status ");*/
#endif

#if (USE_WRAPPER)
    RetErr |= register_command( "AUD_DRSTART",     AUD_STStart, "<Stream: Content Type Frequency Object Id> Start the decoding of input stream");
    RetErr |= register_command( "AUD_DRSTOP",     AUD_STStop, "Stop audio decoder");

    RetErr |= register_command( "AUD_DRSETSTEREOMODE",  AUD_STSetStereoOutput   , " ");
    RetErr |= register_command( "AUD_DRGETSTEREOMODE",  AUD_STGetStereoOutput   , " ");

    RetErr |= register_command( "AUD_STMuteAnaDig",  AUD_STMuteAnaDig, " <Object> Mutes output");

    RetErr |= register_command( "AUD_DRSETPROLOGIC", AUD_STSetPrologic , "ST SETS PROLOGIC STATE");
    RetErr |= register_command( "AUD_DRGETPROLOGIC", AUD_STGetPrologic , "ST GETS PROLOGIC STATE");

    RetErr |= register_command( "AUD_PPSETCFGSPEAKER",     AUD_STSetSpeakerConfig, "");
    RetErr |= register_command( "AUD_PPGETCFGSPEAKER",     AUD_STGetSpeakerConfig, "");

    RetErr |= register_command( "AUD_DRSETEFFECT",  AUD_STSetEffect, " ");
    RetErr |= register_command( "AUD_DRGETEFFECT",  AUD_STGetEffect, " ");

    RetErr |= register_command( "AUD_DRSETDYNRANGE",     AUD_STSetDynamicRange, "   ");
     RetErr |= register_command( "AUD_DRGETDYNRANGE",     AUD_STGetDynamicRange, "   ");


    RetErr |= register_command( "AUD_PPGETATTENUATION",  AUD_STGetAttenuation, " ");

   RetErr |= register_command( "AUD_PPSETATTENUATION",  AUD_STSetAttenuation, " ");

    RetErr |= register_command( "AUD_PPSETCHDELAY",  AUD_STSetChannelDelay, "<Left, Right, LSurr, RSurr, Center, Subw> Set the delay for each speaker");
    RetErr |= register_command( "AUD_PPGETCHDELAY",  AUD_STGetChannelDelay, "<Left, Right, LSurr, RSurr, Center, Subw> Set the delay for each speaker");
    RetErr |= register_command( "AUD_PPCOMPARECHDLAY",  AUD_STCompareChannelDelay, "<Left, Right, LSurr, RSurr, Center, Subw> Set the delay for each speaker");
#else
    RetErr |= register_command( "AUD_DRSTART",     AUD_DRStart, "<Stream: Content Type Frequency Object Id> Start the decoding of input stream");
    RetErr |= register_command( "AUD_DRSTOP",      AUD_DRStop, "<<StopMode> <Object Id> Stop the decoding of input stream");
    RetErr |= register_command( "AUD_PPGETATTENUATION",  AUD_PPGetAttenuation, "Get Attenuation");
    RetErr |= register_command( "AUD_PPGETCHDELAY",  AUD_PPGetChannelDelay, "Retrieve the delay configuration set");
    RetErr |= register_command( "AUD_DRGETDEEMPHASIS",  AUD_DRGetDeEmphasis, "<TRUE(enabled) | FALSE(disabled)> De-emphasis enabled or disabled ");
    RetErr |= register_command( "AUD_OPGETDIGOUT",   AUD_OPGetDigitalOutput, "Get the configuration of the digital output.");
    RetErr |= register_command( "AUD_DRGETDYNRANGE", AUD_DRGetDynRange, "Retrieve the dynamic range configuration");
    RetErr |= register_command( "AUD_DRGETEFFECT",   AUD_DRGetEffect, "Retrieve the stereo effect configuration set");
    RetErr |= register_command( "AUD_PPGETKARAOKE",  AUD_PPGetKaraoke, "Retrieve the Karaoke configuration set");
    RetErr |= register_command( "AUD_DRGETPROLOGIC", AUD_DRGetPrologic, "Get the prologic state of the audio decoder");
    RetErr |= register_command( "AUD_PPGETCFGSPEAKER", AUD_PPGetSpeakerConfig, "Retrieve the speaker configuration set");
    RetErr |= register_command( "AUD_PPGETSPEAKER",  AUD_PPGetSpeaker, "Get the state of each speaker.");
    RetErr |= register_command( "AUD_DRGETSTEREOMODE",   AUD_DRGetStereoOutput, "Get the type of the output.");
    RetErr |= register_command( "AUD_PPSETATTENUATION",  AUD_PPSetAttenuation, "<Left, Right, LSurr, RSurr, Center, Subw> Set attenuation level for each speaker");
    RetErr |= register_command( "AUD_PPSETCHDELAY",  AUD_PPSetChannelDelay, "<Left, Right, LSurr, RSurr, Center, Subw> Set the delay for each speaker");
    RetErr |= register_command( "AUD_DRSETDEEMPHASIS",  AUD_DRSetDeEmphasis, "<TRUE(enable) | FALSE(disable)> Enable or disable the de-emphasis filter");
    RetErr |= register_command( "AUD_OPSETDIGOUT",   AUD_OPSetDigitalOutput, "SetDigitalOutput <TRUE/FALSE> [< Source >] [< Copy Right>],[<Latency>] ");
    RetErr |= register_command( "AUD_DRSETDYNRANGE", AUD_DRSetDynRange, "<0..255> Set the dynamic range configuration");
    RetErr |= register_command( "AUD_DRSETEFFECT",   AUD_DRSetEffect, "<1:SRS | 2:SURROUND> Set the stereo effect");
    RetErr |= register_command( "AUD_PPSETKARAOKE",  AUD_PPSetKaraoke, "<Mode> Set the Karaoke downmix");
    RetErr |= register_command( "AUD_DRSETPROLOGIC", AUD_DRSetPrologic, "Force prologic decoding");
    RetErr |= register_command( "AUD_PPSETCFGSPEAKER", AUD_PPSetSpeakerConfig, "<front, center, Lsurr, Rsurr> Configure speakers size (1:SMALL, 2:LARGE)");
    RetErr |= register_command( "AUD_PPSETSPEAKER",  AUD_PPSetSpeakerEnable, "<Front, LSurr, RSurr, Center, Sub> Define the state [0:off, 1:on] of each speaker ");
    RetErr |= register_command( "AUD_DRSETSTEREOMODE",   AUD_DRSetStereoOutput, "Set the routing of the output <Stereo> <Dual Left> <Dual Right>");
    RetErr |= register_command( "AUD_PPCOMPARECHDLAY",  AUD_PPCompareChannelDelay   , "<Stream: Content Type Frequency Object Id> Start the decoding of input stream");
    RetErr |= register_command( "AUD_PPCOMPAREATTENUATION",  AUD_PPCompareAttenuation   , "<Stream: Content Type Frequency Object Id> Start the decoding of input stream");
	RetErr |= register_command( "AUD_DRSETSRSEFFECTPARAMS",  AUD_DRSetSRSEffectParams, " ");
	RetErr |= register_command( "AUD_MXSETPTSSTATUS",  AUD_MXSetPTSStatus, "AUD_DRSetPTSStatus <deviceid> <mixer obj> <input> <ptsstatus>");
#endif
#ifdef ST_OSLINUX
    RetErr |= register_command( "AUD_LINK",                 AUD_Link   , "No parameter (Temporary function for Linux Audio Testing)");
#endif
    /* Assign default device type: OPEN if defined, 70xx if defined, otherwise 55xx */
#if defined(ST_7100)
    assign_integer  ("DEVICETYPE", 7100, false );
#endif

#if defined(ST_7109)
    assign_integer  ("DEVICETYPE", 7109, false );
#endif

/* Used to select OpConnectSource on Espresso and 4629 */
    assign_integer  ("SETOUTPUT", 0 , false );

    assign_integer  ("STEP", 0, false );
    assign_integer  ("SPEAKERS", 2, false );
    assign_integer  ("OBJECT", MAGICVAL, false );
    assign_integer  ("INJECT_DEST", 1, false );

    assign_integer  ("STOP_END_DATA",  STAUD_STOP_WHEN_END_OF_DATA, true );
    assign_integer  ("STOP_NOW",  STAUD_STOP_NOW, true );

    assign_integer  ("STEREOPCM",  0, true );
    assign_integer  ("MULTIPCM",   1, true );
    assign_integer  ("PPLAYER",  2, true );
    assign_integer  ("COMP", 3, true );
    assign_integer  ("EFFECT_NONE", STAUD_EFFECT_NONE, true);


    assign_integer  ("I2S", STAUD_INPUT_MODE_I2S, true);
    assign_integer  ("DIRECT_I2S", STAUD_INPUT_MODE_I2S_DIRECT, true);
    assign_integer  ("DMA", STAUD_INPUT_MODE_DMA, true);

    assign_integer  ("BITS_16", STAUD_DAC_DATA_PRECISION_16BITS, true);
    assign_integer  ("BITS_18", STAUD_DAC_DATA_PRECISION_18BITS, true);
    assign_integer  ("BITS_20", STAUD_DAC_DATA_PRECISION_20BITS, true);
    assign_integer  ("BITS_24", STAUD_DAC_DATA_PRECISION_24BITS, true);

 	assign_integer  ("OBJ_IPCD0", STAUD_OBJECT_INPUT_CD0, true );
	assign_integer  ("OBJ_IPCD1", STAUD_OBJECT_INPUT_CD1, true );
	assign_integer  ("OBJ_IPPCM0", STAUD_OBJECT_INPUT_PCM0, true );
	assign_integer  ("OBJ_MIX0", STAUD_OBJECT_MIXER0, true );
	assign_integer  ("OBJ_DRCOMP0", STAUD_OBJECT_DECODER_COMPRESSED0, true );
	assign_integer  ("OBJ_DRCOMP1", STAUD_OBJECT_DECODER_COMPRESSED1, true );
	assign_integer  ("EFFECT_FOCUSELE", STAUD_EFFECT_FOCUS_ELEVATION, true );
	assign_integer  ("EFFECT_FOCUSTWEETERELE", STAUD_EFFECT_FOCUS_TWEETER_ELEVATION, true );
	assign_integer  ("EFFECT_TBLEVEL", STAUD_EFFECT_TRUBASS_LEVEL, true );
	assign_integer  ("EFFECT_IPGAIN", STAUD_EFFECT_INPUT_GAIN, true );
	assign_integer  ("EFFECT_TXSTHEADPH", STAUD_EFFECT_TSXT_HEADPHONE, true );
	assign_integer  ("EFFECT_TBSIZE", STAUD_EFFECT_TRUBASS_SIZE, true );
	assign_integer  ("EFFECT_TXSTMODE", STAUD_EFFECT_TXST_MODE, true );
	assign_integer  ("OBJ_PP0", STAUD_OBJECT_POST_PROCESSOR0, true );
	assign_integer  ("OBJ_PP1", STAUD_OBJECT_POST_PROCESSOR1, true );
	assign_integer  ("OBJ_PP2", STAUD_OBJECT_POST_PROCESSOR2, true );

    assign_integer  ("OBJ_OPPCMP0", STAUD_OBJECT_OUTPUT_PCMP0, true );
    assign_integer  ("OBJ_OPPCMP1", STAUD_OBJECT_OUTPUT_PCMP1, true );


    assign_integer  ("OBJ_OPSPDIF0", STAUD_OBJECT_OUTPUT_SPDIF0, true );

    assign_integer  ("OBJ_SBITCONV", STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER, true );
    assign_integer  ("OBJ_SBYTESWAP", STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER, true );



    assign_integer  ("SPDIF",    STAUD_STREAM_TYPE_SPDIF, true );

    assign_integer  ("IGNID",    STAUD_IGNORE_ID, true );

#ifndef ST_OSLINUX
    assign_integer  ("MODE_STEREO",         STAUD_STEREO_MODE_STEREO, true );
    assign_integer  ("MODE_PROLOGIC",       STAUD_STEREO_MODE_PROLOGIC, true );
    assign_integer  ("MODE_DUALLEFT",       STAUD_STEREO_MODE_DUAL_LEFT, true );
    assign_integer  ("MODE_DUALRIGHT",      STAUD_STEREO_MODE_DUAL_RIGHT, true );
    assign_integer  ("MODE_DUALMONO",       STAUD_STEREO_MODE_DUAL_MONO, true );
    assign_integer  ("MODE_SECONDSTEREO",   STAUD_STEREO_MODE_SECOND_STEREO, true );
	assign_integer  ("MODE_AUTO",   STAUD_STEREO_MODE_AUTO, true );
#endif

    assign_integer  ("MONO",     STAUD_STEREO_MONO, true );
    assign_integer  ("STEREO",   STAUD_STEREO_STEREO, true );
    assign_integer  ("PROLOGIC", STAUD_STEREO_PROLOGIC, true );
    assign_integer  ("DLEFT",    STAUD_STEREO_DUAL_LEFT, true );
    assign_integer  ("DRIGHT",   STAUD_STEREO_DUAL_RIGHT, true );
    assign_integer  ("DMONO",    STAUD_STEREO_DUAL_MONO, true );
    assign_integer  ("SECSTE",   STAUD_STEREO_SECOND_STEREO, true );

    assign_integer  ("DIGCOMP",    STAUD_DIGITAL_MODE_COMPRESSED, true );
    assign_integer  ("DIGNONCOMP", STAUD_DIGITAL_MODE_NONCOMPRESSED, true );
    assign_integer  ("DIGOFF",     STAUD_DIGITAL_MODE_OFF, true );

    assign_integer  ("COPYRIGHT_FREE",     STAUD_COPYRIGHT_MODE_NO_RESTRICTION, true );
    assign_integer  ("COPYRIGHT_ONE_COPY", STAUD_COPYRIGHT_MODE_ONE_COPY, true );
    assign_integer  ("COPYRIGHT_NO_COPY",  STAUD_COPYRIGHT_MODE_NO_COPY, true );

    /* error code constants definition */
    assign_integer  ("ERR_INVALID_HANDLE",        ST_ERROR_INVALID_HANDLE, true);
    assign_integer  ("ERR_FEATURE_NOT_SUPPORTED", ST_ERROR_FEATURE_NOT_SUPPORTED, true);
    assign_integer  ("ERR_ALREADY_INITIALIZED",   ST_ERROR_ALREADY_INITIALIZED, true);
    assign_integer  ("ERR_BAD_PARAMETER",         ST_ERROR_BAD_PARAMETER, true);
    assign_integer  ("ERR_NO_MEMORY",             ST_ERROR_NO_MEMORY, true);
    assign_integer  ("ERR_UNKNOWN_DEVICE",        ST_ERROR_UNKNOWN_DEVICE, true);
    assign_integer  ("ERR_NO_FREE_HANDLES",       ST_ERROR_NO_FREE_HANDLES, true);
    assign_integer  ("ERR_OPEN_HANDLE",           ST_ERROR_OPEN_HANDLE, true);
    assign_integer  ("ERR_DECODER_PAUSING",       STAUD_ERROR_DECODER_PAUSING, true);
    assign_integer  ("ERR_DECODER_NOT_PAUSING",   STAUD_ERROR_DECODER_NOT_PAUSING, true);
    assign_integer  ("ERR_DECODER_STOPPED",       STAUD_ERROR_DECODER_STOPPED, true);
    assign_integer  ("ERR_DECODER_RUNNING",       STAUD_ERROR_DECODER_RUNNING, true);
    assign_integer  ("ERR_CLKRV_OPEN",            STAUD_ERROR_CLKRV_OPEN, true);

    assign_integer  ("EVT_NEW_FRAME",     STAUD_NEW_FRAME_EVT, true);
    assign_integer  ("EVT_DATA_ERROR",    STAUD_DATA_ERROR_EVT, true);
    assign_integer  ("EVT_LOW_DATA_LEVEL",STAUD_LOW_DATA_LEVEL_EVT, true);
    assign_integer  ("EVT_EMPHASIS",      STAUD_EMPHASIS_EVT, true);
    assign_integer  ("EVT_STOPPED",       STAUD_STOPPED_EVT, true);
    assign_integer  ("EVT_NEW_ROUTING",   STAUD_NEW_ROUTING_EVT, true);
    assign_integer  ("EVT_NEW_FREQUENCY", STAUD_NEW_FREQUENCY_EVT, true);
    assign_integer  ("EVT_PCM_UNDERFLOW", STAUD_PCM_UNDERFLOW_EVT, true);
    assign_integer  ("EVT_CDFIFO_UNDERFLOW", STAUD_CDFIFO_UNDERFLOW_EVT, true);
    assign_integer  ("EVT_LOW_DATA_LEVEL", STAUD_LOW_DATA_LEVEL_EVT, true);
	assign_integer  ("EVT_DECODER_STOP", STAUD_TEST_DECODER_STOP_EVT,true);
    assign_integer  ("OBJ_OPPCM", STAUD_OBJECT_DECODER_COMPRESSED0, true );
    assign_integer  ("OBJ_OPPCM1", STAUD_OBJECT_DECODER_COMPRESSED1, true );
    assign_integer  ("EFFECT_TRU", STAUD_EFFECT_TRUSURROUND, true);
    assign_integer  ("EFFECT_BASS", STAUD_EFFECT_SRS_TRUBASS, true);
    assign_integer  ("EFFECT_FOCUS", STAUD_EFFECT_SRS_FOCUS, true);

    assign_integer  ("EFFECT_TRU_XT", STAUD_EFFECT_SRS_TRUSUR_XT, true);
	assign_integer  ("EFFECT_FOCUS_EFFECT", STAUD_EFFECT_SRS_FOCUS, true);
    assign_integer  ("EFFECT_NONE", STAUD_EFFECT_NONE, true);




    if ( RetErr == TRUE )
    {
        STTBX_Print(( "AUD_InitCommand() : macros registrations failure !\n" ));
    }
    else
    {
        STTBX_Print(( "AUD_InitCommand() : macros registrations ok\n" ));
    }
    return( !RetErr );

} /* end AUD_InitCommand */
#ifdef ST_OS21
/*-------------------------------------------------------------------------
 * Function : MBX_Init
 *            Initialization of the mbx interface
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
BOOL MBX_Init(void)
{
  BOOL RetOk = TRUE;

#ifndef ST_OSLINUX
  /* With OS21, the firmware need to be loaded here */
 /* ============================================== */
 STAVMEM_AllocBlockParams_t            AVMEM_AllocBlockParams;
 STAVMEM_SharedMemoryVirtualMapping_t  AVMEM_VirtualMapping;
 void                                 *AVMEM_VirtualAddress;
 void                                 *AVMEM_DeviceAddress;
 ST_ErrorCode_t                        ErrCode  = ST_NO_ERROR;
 EMBX_ERROR                            EmbxCode = EMBX_SUCCESS;
 MME_ERROR                             MmeCode  = MME_SUCCESS;
#if defined(RUN_FROM_FLASH)
	U8                                   *Lx_Code_Address;
	U32                                   i;
#else
	U32                                  *FILE_Handle;
#endif
#if 0
    U32                                   Lx_Code_Size;
#endif
    U32 Chip_version = 0;
    U32 tmp;
    Chip_version = *(U32*)0xB9001000;
    Chip_version = ((Chip_version>>28) & 0x0F);

    STTBX_Print(( "Calling  MBX_Init \n" ));

  #if 1
	{
	/* Init the Lx for Video Decode */
	/* ============================ */
	/* Reset-out bypass for LX Deltaphi, Audio */
    #if defined(ST_7100)

    (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x124))  = 0x50000A8C;
	/* Enable reset lx_dh + set perih addr to 0x1A000000 */
    (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x174)) =  0x00000341;
	/* Enable lx_dh to boot at 0x11000000 in system_config28 (TODO: fixed value for boot address) */
    (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x170)) =  RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1+VIDEO_COMPANION_OFFSET+1;
            /*RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1+1;*/
    #endif

    #if defined(ST_7109)

    (*(DU32*) (ST7109_CFG_BASE_ADDRESS + 0x124))  = 0x50000A8C;
	/* Enable reset lx_dh + set perih addr to 0x1A000000 */
    (*(DU32*) (ST7109_CFG_BASE_ADDRESS + 0x174)) =  0x00000341;
	/* Enable lx_dh to boot at 0x11000000 in system_config28 (TODO: fixed value for boot address) */
    (*(DU32*) (ST7109_CFG_BASE_ADDRESS + 0x170)) =  RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1+VIDEO_COMPANION_OFFSET+1;

	#endif
	/* Load the lx code */
	/* ---------------- */
	#if defined(RUN_FROM_FLASH)
	Lx_Code_Address = (U8 *)(RAM1_BASE_ADDRESS+VIDEO_COMPANION_OFFSET);
	for (i=0;i<MAIN_COMPANION_VIDEO_CODE_SIZE;i++)
	{
		Lx_Code_Address[i] = Main_Companion_Video_Code[i];
	}
	#else
    STTBX_Print(("Loading Video firmware\n"));
    if (Chip_version == 0x00)
	FILE_Handle=(void *)SYS_FOpen(VIDEO_COMPANION_NAME,"rb");
	if (Chip_version == 0x01 || Chip_version == 0x02)
	FILE_Handle=(void *)SYS_FOpen(VIDEO_COMPANION_NAME_CUT2,"rb");

	if (FILE_Handle==NULL)
	{
		return(ST_ERROR_NO_MEMORY);
	}
	SYS_FRead((void *)(RAM1_BASE_ADDRESS+VIDEO_COMPANION_OFFSET),1,0x800000,FILE_Handle);
    SYS_FClose(FILE_Handle);
    #endif

    STTBX_Print(("Loading Video firmware finished \n"));
	/* Reset the LX and go on */
	/* ---------------------- */
	/* disable reset lx_dh -> boot (+ set periph address again...) */
    #if defined(ST_7100)
    tmp = (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x174));
    (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x174)) = 0x00000340;
    tmp = (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x174));

	#endif

	#if defined(ST_7109)

    (*(DU32*) (ST7109_CFG_BASE_ADDRESS + 0x174)) =  0x00000340;

	#endif

	}
#endif

   #if 1
	{
	/* Init the Lx for Audio Decode */
	/* ============================ */
	/* Reset-out bypass for LX Deltaphi, Audio */

	#if defined(ST_7100)

    (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x124))  = 0x50000A8C;
	/* Enable reset lx_dh + set perih addr to 0x1A000000 */
    (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x16c)) =  0x00000345;
	/* Enable lx_dh to boot at 0x11000000 in system_config28 (TODO: fixed value for boot address) */
    (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x168)) =  RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1+AUDIO_COMPANION_OFFSET/*0x00200000*/+1;
    #endif


	#if defined(ST_7109)

    (*(DU32*) (ST7109_CFG_BASE_ADDRESS + 0x124))  = 0x50000A8C;
	/* Enable reset lx_dh + set perih addr to 0x1A000000 */
    (*(DU32*) (ST7109_CFG_BASE_ADDRESS + 0x16c)) =  0x00000345;
	/* Enable lx_dh to boot at 0x11000000 in system_config28 (TODO: fixed value for boot address) */
    (*(DU32*) (ST7109_CFG_BASE_ADDRESS + 0x168)) =  RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1+AUDIO_COMPANION_OFFSET+1;

	#endif
	/* Load the lx code */
	/* ---------------- */
	#if defined(RUN_FROM_FLASH)
	Lx_Code_Address = (U8 *)(RAM1_BASE_ADDRESS+AUDIO_COMPANION_OFFSET);
	for (i=0;i<MAIN_COMPANION_AUDIO_CODE_SIZE;i++)
	{
		Lx_Code_Address[i] = Main_Companion_Audio_Code[i];
	}
	#else
    STTBX_Print(("Loading Audio firmware\n"));
    FILE_Handle=(void *)SYS_FOpen(AUDIO_COMPANION_NAME,"rb");
	if (FILE_Handle==NULL)
	{
        RetOk = FALSE;
        return(ST_ERROR_NO_MEMORY);
	}
	SYS_FRead((void *)(RAM1_BASE_ADDRESS+AUDIO_COMPANION_OFFSET),1,0x800000,FILE_Handle);
	SYS_FClose(FILE_Handle);
    #endif

    STTBX_Print(("Loading Audio firmware finished\n"));
	/* Reset the LX and go on */
	/* ---------------------- */
	/* disable reset lx_dh -> boot (+ set periph address again...) */
	#if defined(ST_7100)

    (*(DU32*) (ST7100_CFG_BASE_ADDRESS + 0x16c)) =  0x00000344;

	#endif

	#if defined(ST_7109)

    (*(DU32*) (ST7109_CFG_BASE_ADDRESS + 0x16c)) =  0x00000344;

	#endif
	}
#endif


	 /* Initialize the EMBX */
 /* =================== */

 /* Set size of the shared memory */
 /* ----------------------------- */
#if !MSPP_SUPPORT
    MBX_Size = 3*1024*1024;
#else
	MBX_Size = 512*1024;
#endif

	 /* Allocate shared memory between ST40 and Lx */
    /* ------------------------------------------ */
	memset(&AVMEM_AllocBlockParams,0,sizeof(AVMEM_AllocBlockParams));
    AVMEM_AllocBlockParams.PartitionHandle          = /*AVMEM_PartitionHandle[0]*/ AvmemPartitionHandle[0];
	AVMEM_AllocBlockParams.Alignment                = 8192;
	AVMEM_AllocBlockParams.Size                     = MBX_Size;
	AVMEM_AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
	AVMEM_AllocBlockParams.NumberOfForbiddenRanges  = 0;
	AVMEM_AllocBlockParams.ForbiddenRangeArray_p    = NULL;
	AVMEM_AllocBlockParams.NumberOfForbiddenBorders = 0;
	AVMEM_AllocBlockParams.ForbiddenBorderArray_p   = NULL;
	ErrCode = STAVMEM_AllocBlock(&AVMEM_AllocBlockParams,&MBX_AVMEM_Handle);

    if (ErrCode!=ST_NO_ERROR)
    {
      RetOk = FALSE;
      return(ErrCode);
    }
	ErrCode=STAVMEM_GetBlockAddress(MBX_AVMEM_Handle,&AVMEM_VirtualAddress);

  if (ErrCode!=ST_NO_ERROR)
  {
    RetOk = FALSE;
    return(ErrCode);
  }
	ErrCode=STAVMEM_GetSharedMemoryVirtualMapping(&AVMEM_VirtualMapping);
	if (ErrCode!=ST_NO_ERROR)
  {
    RetOk = FALSE;
    return(ErrCode);
  }
	AVMEM_DeviceAddress = STAVMEM_VirtualToDevice(AVMEM_VirtualAddress,&AVMEM_VirtualMapping);
    MBX_Address         = (U32)STAVMEM_DeviceToCPU(AVMEM_DeviceAddress,&AVMEM_VirtualMapping);

#if MSPP_SUPPORT
	AVMEM_DeviceAddress		= STAVMEM_VirtualToDevice(RAM1_BASE_ADDRESS,&AVMEM_VirtualMapping);
	WARP_Address			= (U32)STAVMEM_DeviceToCPU(AVMEM_DeviceAddress,&AVMEM_VirtualMapping);
	WARP_Size				= RAM1_SIZE;


#endif
	 /* Start the mailbox */
 /* ----------------- */
	 {
  #if 0
  EMBXSHM_MailboxConfig_t EMBX_Config = { "ups",                    /* Name of the transport                       */
                                          0,                        /* CpuID --> 0 means Master                    */
                                          { 1, 1, 1, 0 },           /* Participants --> ST40 + LX video + LX audio */
                                          0x60000000,               /* PointerWarp --> LMI_VID is seen             */
                                          0,                        /* MaxPorts                                    */
                                          16,                       /* MaxObjects                                  */
                                          16,                       /* FreeListSize                                */
                                          #if 1
                                          (EMBX_VOID *)MBX_Address, /* CPU Shared address                          */
                                          (EMBX_UINT)MBX_Size,      /* Size of shared memory                       */
#if !MSPP_SUPPORT
                                          (EMBX_VOID *)MBX_Address, /* WarpRangeAddr                               */
                                          (EMBX_UINT)MBX_Size      /* WarpRangeSize                               */
#else
                                          (EMBX_VOID *)WARP_Address, /* WarpRangeAddr                               */
                                          (EMBX_UINT)WARP_Size      /* WarpRangeSize                               */
#endif
                                          #else
                                          NULL,
                                          512 * 1024,
                                          NULL,
                                          0
                                          #endif
                                        };

#endif
#if 1
   EMBXSHM_MailboxConfig_t EMBX_Config = { "ups",                                            /* Name of the transport                       */
                                          0,                                                /* CpuID --> 0 means Master                    */
                                          { 1, 1, 1, 0 },                                   /* Participants --> ST40 + LX video + LX audio */
                                          0x60000000,                                       /* PointerWarp --> LMI_VID is seen             */
                                          0,                                                /* MaxPorts                                    */
                                          16,                                               /* MaxObjects                                  */
                                          16,                                               /* FreeListSize                                */
                                          (EMBX_VOID*)MBX_Address,                         /* CPU Shared address                          */
                                          (EMBX_UINT)MBX_Size,                              /* Size of shared memory                       */
                                          (EMBX_VOID *)RAM1_BASE_ADDRESS,                   /* WarpRangeAddr                               */
                                          (EMBX_UINT)0x20000000                             /* WarpRangeSize                               */
                                        };

  #endif
  /* Initialize the Mailbox */
  STTBX_Print(("Init Mailbox\n"));
  EmbxCode=EMBX_Mailbox_Init();
  if (EmbxCode!=EMBX_SUCCESS)
  {
     RetOk = FALSE;
     return(ST_ERROR_NO_MEMORY);
  }
  /* Register the Video Lx */
  STTBX_Print(("register mailbox Video\n"));

  #if defined(ST_7100)
  EmbxCode=EMBX_Mailbox_Register((void *)ST7100_MAILBOX_0_BASE_ADDRESS,(int)INT_LX_VIDEO,-1,EMBX_MAILBOX_FLAGS_SET2);
  #endif

  #if defined(ST_7109)
  EmbxCode=EMBX_Mailbox_Register((void *)ST7109_MAILBOX_0_BASE_ADDRESS,(int)INT_LX_VIDEO,-1,EMBX_MAILBOX_FLAGS_SET2);
  #endif

  if (EmbxCode!=EMBX_SUCCESS)
  {
    RetOk = FALSE;
    return(ST_ERROR_NO_MEMORY);
  }
  /* Register the Audio Lx */
  STTBX_Print(("register mailbox Audio\n"));

  #if defined(ST_7100)
  EmbxCode=EMBX_Mailbox_Register((void *)ST7100_MAILBOX_1_BASE_ADDRESS,(int)INT_LX_AUDIO,0,EMBX_MAILBOX_FLAGS_SET2);
  #endif

  #if defined(ST_7109)
  EmbxCode=EMBX_Mailbox_Register((void *)ST7109_MAILBOX_1_BASE_ADDRESS,(int)INT_LX_AUDIO,0,EMBX_MAILBOX_FLAGS_SET2);
  #endif
  if (EmbxCode!=EMBX_SUCCESS)
   {
    return(ST_ERROR_NO_MEMORY);
   }
  /* Setup the transport communication */
  STTBX_Print(("RegisterTransport\n"));
  EmbxCode=EMBX_RegisterTransport(EMBXSHM_mailbox_factory,&EMBX_Config,sizeof(EMBX_Config),&MBX_Factory);
  if (EmbxCode!=EMBX_SUCCESS)
   {
    return(ST_ERROR_NO_MEMORY);
   }
  /* Initialize the complete mailbox */
  STTBX_Print(("Embx Init\n"));
  EmbxCode=EMBX_Init();
  if (EmbxCode!=EMBX_SUCCESS)
   {
    return(ST_ERROR_NO_MEMORY);
   }
 }

	 /* Start the multicom */
 /* ================== */
    STTBX_Print(("MME_Init\n"));
	MmeCode=MME_Init();
	if (MmeCode!=MME_SUCCESS)
  {
    RetOk = FALSE;
    return(ST_ERROR_NO_MEMORY);
  }
    STTBX_Print(("MME register transport\n"));
    MmeCode=MME_RegisterTransport("ups");
	if (MmeCode!=MME_SUCCESS)
  {
    RetOk = FALSE;
    return(ST_ERROR_NO_MEMORY);
  }

   /* Return no errors */
 /* ================ */
    STTBX_Print(("Mbx init end\n"));
#endif


  return(RetOk);
}
#endif
/*-------------------------------------------------------------------------
 * Function : AUD_InitComponent
 *            Initialization of AUDLX
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/

BOOL AUD_InitComponent(void)
{
     ST_ErrorCode_t                ErrCode=ST_NO_ERROR;
 STAUD_InitParams_t            AUD_InitParams;
 STAUD_OpenParams_t            AUD_OpenParams;
 STAUD_BroadcastProfile_t      AUD_BroadcastProfile;
 STAUD_DynamicRange_t          AUD_DynamicRange;
 /*STEVT_OpenParams_t            EVT_OpenParams;*/
 /*STEVT_DeviceSubscribeParams_t EVT_SubcribeParams;*/
 STAUD_OutputParams_t          AUD_OutputParams;
  BOOL                            RetOK = TRUE;
  U32                             DeviceId = 0;

 /* Reset Handles */
 /* ============= */
 memset(AUD_Handle,0,sizeof(AUD_Handle));
 memset(AUDi_InjectContext,0,sizeof(AUDi_InjectContext));

 /* Initialize MAIN audio */
 /* ===================== */

 /* Set the initialization structure */
 /* -------------------------------- */
 memset(&AUD_InitParams      ,0,sizeof(STAUD_InitParams_t));
 memset(&AUD_OpenParams      ,0,sizeof(STAUD_OpenParams_t));
 memset(&AUD_DynamicRange    ,0,sizeof(STAUD_DynamicRange_t));
 memset(&AUD_BroadcastProfile,0,sizeof(STAUD_BroadcastProfile_t));
 AUD_InitParams.DeviceType                                    = STAUD_DEVICE_STI7100;
 AUD_InitParams.InterruptNumber                               = 0; /* Not used */
 AUD_InitParams.InterruptLevel                                = 0; /* Not used */
 AUD_InitParams.Configuration                                 = STAUD_CONFIG_DVB_COMPACT;
 AUD_InitParams.DACClockToFsRatio                             = 256;
 AUD_InitParams.PCMOutParams.InvertWordClock                  = FALSE;
 AUD_InitParams.PCMOutParams.InvertBitClock                   = FALSE;
 AUD_InitParams.PCMOutParams.Format                           = STAUD_DAC_DATA_FORMAT_I2S;
 AUD_InitParams.PCMOutParams.Precision                        = STAUD_DAC_DATA_PRECISION_24BITS;
 AUD_InitParams.PCMOutParams.Alignment                        = STAUD_DAC_DATA_ALIGNMENT_LEFT;
 AUD_InitParams.PCMOutParams.MSBFirst                         = TRUE;
 AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier     = 256;
 AUD_InitParams.SPDIFOutParams.AutoLatency                    = TRUE;
 AUD_InitParams.SPDIFOutParams.Latency                        = 0;
 AUD_InitParams.SPDIFOutParams.CopyPermitted                  = STAUD_COPYRIGHT_MODE_NO_COPY;
 AUD_InitParams.SPDIFOutParams.AutoCategoryCode               = TRUE;
 AUD_InitParams.SPDIFOutParams.CategoryCode                   = 0;
 AUD_InitParams.SPDIFOutParams.AutoDTDI                       = TRUE;
 AUD_InitParams.SPDIFOutParams.DTDI                           = 0;
 AUD_InitParams.SPDIFOutParams.Emphasis                       = STAUD_SPDIF_EMPHASIS_CD_TYPE;
 AUD_InitParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode      = STAUD_SPDIF_DATA_PRECISION_24BITS;
 AUD_InitParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier = 128;
 AUD_InitParams.MaxOpen                                       = 1;

 #ifdef ST_OSLINUX
    AUD_InitParams.BufferPartition                            = 0;
    AUD_InitParams.AllocateFromEMBX                           = TRUE;
    AUD_InitParams.AVMEMPartition                             = 0;
#else
    AUD_InitParams.AVMEMPartition                               = AvmemPartitionHandle[0];

#if MSPP_SUPPORT
    AUD_InitParams.BufferPartition                                = AvmemPartitionHandle[1];
    AUD_InitParams.AllocateFromEMBX                               = FALSE;
#else /*MSPP_SUPPORT*/
    AUD_InitParams.BufferPartition                                = AvmemPartitionHandle[0];
    AUD_InitParams.AllocateFromEMBX                               = TRUE;
#endif /*MSPP_SUPPORT*/

#endif
 AUD_InitParams.CPUPartition_p                                = SystemPartition_p;
 AUD_InitParams.PCMMode                                       = PCM_ON;
 AUD_InitParams.SPDIFMode                                     = STAUD_DIGITAL_MODE_NONCOMPRESSED;
 AUD_InitParams.NumChannels                                   = 2;
 AUD_InitParams.DriverIndex = 0;
 strcpy(AUD_InitParams.EvtHandlerName,EVT_DeviceName[0]);
 strcpy(AUD_InitParams.ClockRecoveryName,STCKLRV_DeviceName[0]);
 /* Init the AUDIO */
 /* -------------- */
 printf("AUD_Init(%s)=", AUD_DeviceName[DeviceId]);
 ErrCode=STAUD_Init(AUD_DeviceName[DeviceId],&AUD_InitParams);
 if (ErrCode!=ST_NO_ERROR)
  {
   return(ErrCode);
  }
 printf("%s\n", STAUD_GetRevision() );
 AUD_OpenParams.SyncDelay = 0;
 ErrCode=STAUD_Open(AUD_DeviceName[DeviceId],&AUD_OpenParams,&AUD_Handle[DeviceId]);
 if (ErrCode!=ST_NO_ERROR)
 {
   return(ErrCode);
 }

  ErrCode=STAUD_OPGetParams(AUD_Handle[DeviceId],STAUD_OBJECT_OUTPUT_PCMP0,&AUD_OutputParams);
  if (ErrCode!=ST_NO_ERROR)
   {
    return(ErrCode);
   }
  AUD_OutputParams.PCMOutParams.PcmPlayerFrequencyMultiplier=128;

  /* AUD_OutputParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier=128;*/
  ErrCode=STAUD_OPSetParams(AUD_Handle[DeviceId],STAUD_OBJECT_OUTPUT_PCMP0,&AUD_OutputParams);
  if (ErrCode!=ST_NO_ERROR)
  {
    return(ErrCode);
  }
  ErrCode=STAUD_OPEnableHDMIOutput(AUD_Handle[DeviceId],STAUD_OBJECT_OUTPUT_PCMP0);
  if (ErrCode!=ST_NO_ERROR)
  {
    return(ErrCode);
  }

 AUD_BroadcastProfile = STAUD_BROADCAST_DVB;
 ErrCode=STAUD_IPSetBroadcastProfile(AUD_Handle[DeviceId],STAUD_OBJECT_INPUT_CD0,AUD_BroadcastProfile);
 if (ErrCode!=ST_NO_ERROR)
  {
   return(ErrCode);
  }
  return(RetOK);
}


/*#######################################################################*/
/*###################  STB or DVD specific includes #####################*/
/*#######################################################################*/


/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

/* ========================================================================
   Name:        AUDi_InjectTask
   Description: Inject task
   ======================================================================== */
#if 1

/* ========================================================================
   Name:        AUDi_InjectCopy_CPU
   Description: Copy datas in the audio decoder
   ======================================================================== */

#if !defined(INJECTOR_USE_DMA)
static void AUDi_InjectCopy_CPU(AUD_InjectContext_t *AUD_InjectContext,U32 SizeToCopy)
{

 /* Copy datas using the cd fifo */
 /* ---------------------------- */
#if defined(ST_5100)||defined(ST_5301)||defined(ST_5528)||defined(ST_7710)
 {
 U32 Sample,i;
AUDi_TraceDBG(("AUDi_InjectCopy(%d): Size = 0x%08x - SrcPtr = 0x%08x\n",AUD_InjectContext->InjectorId,SizeToCopy,AUD_InjectContext->BufferConsumer));
	STTBX_Print(("AUDi_InjectCopy(%d): Size = 0x%08x - SrcPtr = 0x%08x\n",AUD_InjectContext->InjectorId,SizeToCopy,AUD_InjectContext->BufferConsumer));
 for (i=0;i<(SizeToCopy)/4;i++)
  {
   Sample=*((U32 *)AUD_InjectContext->BufferConsumer);
#if defined(ST_5100)
   STSYS_WriteRegDev32LE(ST5100_AUDIO_CD_FIFO_BASE_ADDRESS,Sample);
#endif
#if defined(ST_5301)
   STSYS_WriteRegDev32LE(ST5301_AUDIO_CD_FIFO_BASE_ADDRESS,Sample);
#endif
#if defined(ST_5528)
   STSYS_WriteRegDev32LE(ST5528_AUDIO_CD_FIFO_BASE_ADDRESS,Sample);
#endif
#if defined(ST_7710)
   STSYS_WriteRegDev32LE(ST7710_AUDIO_CD_FIFO_BASE_ADDRESS,Sample);
#endif
   AUD_InjectContext->BufferConsumer+=4;
  }
 }
#endif

 /* Copy into the PES buffer */
 /* ------------------------ */
#if defined(ST_5105)||defined(ST_7100)||defined(ST_7109)
 {
 U32 SizeToCopy1,SizeToCopy2;
 /* !!! First SizeToCopy has to be less than the pes audio buffer !!! */

 /* Get size to inject */
 /* ------------------ */
 SizeToCopy1 = SizeToCopy;
 SizeToCopy2 = 0;

 /* Check if we have to split the copy into pieces into the destination buffer */
 /* -------------------------------------------------------------------------- */
 if ((AUD_InjectContext->BufferAudioProducer+SizeToCopy)>(AUD_InjectContext->BufferAudio+AUD_InjectContext->BufferAudioSize))
  {
   SizeToCopy1 = (AUD_InjectContext->BufferAudio+AUD_InjectContext->BufferAudioSize)-AUD_InjectContext->BufferAudioProducer;
   SizeToCopy2 = SizeToCopy-SizeToCopy1;
  }
 AUDi_TraceDBG(("AUDi_InjectCopy(%d): SizeToCopy= 0x%08x :Size1 = 0x%08x - Size2 = 0x%08x - SrcPtr = 0x%08x - DstPtr = 0x%08x\n",AUD_InjectContext->InjectorId,SizeToCopy,SizeToCopy1,SizeToCopy2,AUD_InjectContext->BufferConsumer,AUD_InjectContext->BufferAudioProducer));

 /* First piece */
 /* ----------- */
#if (defined(ST_7100)||defined(ST_7109)) && !defined(ST_OSLINUX)
 memcpy((U8 *)(AUD_InjectContext->BufferAudioProducer|0xA0000000),(U8 *)AUD_InjectContext->BufferConsumer,SizeToCopy1);
#else
{

 memcpy((U8 *)(AUD_InjectContext->BufferAudioProducer),(U8 *)AUD_InjectContext->BufferConsumer,SizeToCopy1);

 }
#endif
	STOS_SemaphoreWait(AUD_InjectContext->SemLock);
	AUD_InjectContext->BufferAudioProducer += SizeToCopy1;
	AUD_InjectContext->BufferConsumer      += SizeToCopy1;
	if ((AUD_InjectContext->BufferAudioProducer)==(AUD_InjectContext->BufferAudio+AUD_InjectContext->BufferAudioSize)) AUD_InjectContext->BufferAudioProducer = AUD_InjectContext->BufferAudio;
	STOS_SemaphoreSignal(AUD_InjectContext->SemLock);

 /* Second piece */
 /* ------------ */
 if (SizeToCopy2!=0)
  {
   AUD_InjectContext->BufferAudioProducer = AUD_InjectContext->BufferAudio;
#if (defined(ST_7100)||defined(ST_7109)) && !defined(ST_OSLINUX)
   memcpy((U8 *)(AUD_InjectContext->BufferAudioProducer|0xA0000000),(U8 *)AUD_InjectContext->BufferConsumer,SizeToCopy2);
#else

   memcpy((U8 *)(AUD_InjectContext->BufferAudioProducer),(U8 *)AUD_InjectContext->BufferConsumer,SizeToCopy2);

#endif
	STOS_SemaphoreWait(AUD_InjectContext->SemLock);
	AUD_InjectContext->BufferAudioProducer += SizeToCopy2;
	AUD_InjectContext->BufferConsumer      += SizeToCopy2;
	if ((AUD_InjectContext->BufferAudioProducer)==(AUD_InjectContext->BufferAudio+AUD_InjectContext->BufferAudioSize)) AUD_InjectContext->BufferAudioProducer = AUD_InjectContext->BufferAudio;
	STOS_SemaphoreSignal(AUD_InjectContext->SemLock);
  }

 /* Trace for debug */
 /* --------------- */
 AUDi_TraceDBG(("AUDi_InjectCopy(%d): Size1 = 0x%08x - Size2 = 0x%08x - SrcPtr = 0x%08x - DstPtr = 0x%08x\n",AUD_InjectContext->InjectorId,SizeToCopy1,SizeToCopy2,AUD_InjectContext->BufferConsumer,AUD_InjectContext->BufferAudioProducer));
 }
#endif
}
#endif

/* ========================================================================
   Name:        AUDi_InjectCopy
   Description: Copy datas in the audio decoder
   ======================================================================== */

static void AUDi_InjectCopy(AUD_InjectContext_t *AUD_InjectContext,U32 SizeToCopy)
{
#if !defined(INJECTOR_USE_DMA)
 AUDi_InjectCopy_CPU(AUD_InjectContext,SizeToCopy);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
 AUDi_InjectCopy_FDMA(AUD_InjectContext,SizeToCopy);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5528))
 AUDi_InjectCopy_GPDMA(AUD_InjectContext,SizeToCopy);
#endif
}


/* ========================================================================
   Name:        AUDi_InjectGetFreeSize
   Description: Get free size able to be injected
   ======================================================================== */

static U32 AUDi_InjectGetFreeSize(AUD_InjectContext_t *AUD_InjectContext)
{
 U32 FreeSize;

 /* Get bit buffer size */
 /* ------------------- */
#if defined(ST_5100)||defined(ST_5528)||defined(ST_5301)||defined(ST_7710)
 {
  ST_ErrorCode_t ErrCode=ST_NO_ERROR;
  ErrCode=STAUD_IPGetBitBufferFreeSize(AUD_InjectContext->AUD_Handle,STAUD_OBJECT_INPUT_CD1,&FreeSize);
  if (ErrCode!=ST_NO_ERROR)
   {
    AUDi_TraceERR(("AUDi_InjectGetFreeSize(%d):**ERROR** !!! Unable to get the bit buffer free size !!!\n",AUD_InjectContext->InjectorId));
    return(0);
   }
 }
#endif

 /* Get free size into the PES buffer */
 /* --------------------------------- */
#if defined(ST_5105)||defined(ST_7100)||defined(ST_7109)
	STOS_SemaphoreWait(AUD_InjectContext->SemLock);
	if (AUD_InjectContext->BufferAudioProducer>=AUD_InjectContext->BufferAudioConsumer)
	{
	FreeSize=(AUD_InjectContext->BufferAudioSize-(AUD_InjectContext->BufferAudioProducer-AUD_InjectContext->BufferAudioConsumer));
	}
	else
	{
	FreeSize=(AUD_InjectContext->BufferAudioConsumer-AUD_InjectContext->BufferAudioProducer);
	}
	STOS_SemaphoreSignal(AUD_InjectContext->SemLock);
#endif

 /* Align it on 256 bytes to use external DMA */
 /* ----------------------------------------- */
 /* Here we assume the producer will be never equal to base+size           */
 /* If it's the case, this is managed into the fill producer procedure     */
 /* The audio decoder is working like this, if ptr=base+size, the ptr=base */
 if (FreeSize<=16) FreeSize=0; else FreeSize=FreeSize-16;
 FreeSize=(FreeSize/256)*256;
 return(FreeSize);
}
#ifndef ST_OSLINUX

static void AUDi_InjectTask(void *InjectContext)
{
   AUD_InjectContext_t *AUD_InjectContext;
	U32                  SizeToInject;
	U32                  SizeAvailable;

	/* Retrieve context */
	/* ================ */
	AUD_InjectContext=(AUD_InjectContext_t *)InjectContext;

	/* Wait start of play */
	/* ================== */
	STOS_SemaphoreWait(AUD_InjectContext->SemStart);
	SizeAvailable = SizeToInject = 0;
	/*AUD_InjectContext->NbLoops=0;*/
	/* Infinite loop */
	/* ============= */
	while(AUD_InjectContext->State==AUD_INJECT_START)
	{
	/* Do we have to read datas from a file */
	/* ------------------------------------ */
	if (AUD_InjectContext->FileHandle!=NULL)
	{
	 /* If we reach the end of the file, check number of loops */
	 if (AUD_InjectContext->FilePosition==AUD_InjectContext->FileSize)
	  {
	   /* If infinite play, restart from the beginning */
	   if (AUD_InjectContext->NbLoops==0)
		{
		 AUD_InjectContext->NbLoopsAlreadyDone++;
		 /* Seek to the beginning of the file */
		 #ifdef ST_OS21
         SYS_FSeek(AUD_InjectContext->FileHandle,0,SEEK_SET);
         #else
         fseek(AUD_InjectContext->FileHandle,0,SEEK_SET);
         #endif
		 /* Update file position */
		 AUD_InjectContext->FilePosition=0;
		}
	   /* Else check number of loops */
	   else
		{
		 /* If nb loops reached, switch in stop state */
		 if (AUD_InjectContext->NbLoops==(AUD_InjectContext->NbLoopsAlreadyDone+1))
		  {
		   /* Wait to inject remaining datas */
		   if (AUD_InjectContext->BufferConsumer==AUD_InjectContext->BufferProducer)
			{
#ifndef ST_OSLINUX
			 if(AUD_InjectContext->SetEOFFlag == TRUE)
			 {
			 	STAUD_IPHandleEOF(AUD_InjectContext->AUD_Handle, AUD_InjectContext->InputObject);
			 }
#endif
			 AUD_InjectContext->State = AUD_INJECT_STOP;
			 break;
			}
		  }
		 /* Else increment number of loops and seek to the beginning of the file */
		 else
		  {
		   AUD_InjectContext->NbLoopsAlreadyDone++;
		   /* Seek to the beginning of the file */
		   #ifdef ST_OS21
           SYS_FSeek(AUD_InjectContext->FileHandle,0,SEEK_SET);
           #else
           fseek(AUD_InjectContext->FileHandle,0,SEEK_SET);
           #endif
		   /* Update file position */
		   AUD_InjectContext->FilePosition=0;
		  }
		}
	  }
	 /* If we have to read something from the file */
	 if (AUD_InjectContext->FilePosition!=AUD_InjectContext->FileSize)
	  {
	   U32 SizeToRead;
	   /* Compute number of bytes which we have to read from the file */
	   SizeToRead=AUD_InjectContext->BufferSize-SizeAvailable;
	   /* Get free size into the circular buffer */
	   if ((AUD_InjectContext->BufferProducer+SizeToRead)>(AUD_InjectContext->BufferBase+AUD_InjectContext->BufferSize))
		{
		 SizeToRead=(AUD_InjectContext->BufferBase+AUD_InjectContext->BufferSize)-AUD_InjectContext->BufferProducer;
		}
	   if ((AUD_InjectContext->FileSize-AUD_InjectContext->FilePosition)<=SizeToRead)
		{
		 SizeToRead=(AUD_InjectContext->FileSize-AUD_InjectContext->FilePosition);
		}
	   /* Read the file */
	   if (SizeToRead!=0)
		{
		 SYS_FRead((U8 *)AUD_InjectContext->BufferProducer,SizeToRead,1,AUD_InjectContext->FileHandle);
        {   /* byte reverse the file on the fly*/
		 U32 Index;

		 U16   *FILE_Buffer1;

		if (AUD_InjectContext->DoByteReverseOnFly)
		{
		FILE_Buffer1=(U16 *)AUD_InjectContext->BufferProducer;
#ifndef ST_OSWINCE
		for ( Index = 0 ; Index < (SizeToRead >> 1); Index++ )
			{
				__asm__("swap.b %1, %0" : "=r" (FILE_Buffer1[Index]): "r" (FILE_Buffer1[Index]));
			}
#endif

			}

			}



		}



	   /* Increase size available to be injected */
	   SizeAvailable = SizeAvailable+SizeToRead;
	   /* Update file position & producer */
	   AUD_InjectContext->FilePosition   = AUD_InjectContext->FilePosition+SizeToRead;
	   AUD_InjectContext->BufferProducer = AUD_InjectContext->BufferProducer+SizeToRead;
	   if (AUD_InjectContext->BufferProducer==(AUD_InjectContext->BufferBase+AUD_InjectContext->BufferSize))
		{
		 AUD_InjectContext->BufferProducer=AUD_InjectContext->BufferBase;
		}
	  }
	}

	/* If there is no file and all the stream is in memory */
	/* --------------------------------------------------- */
	if (AUD_InjectContext->FileHandle==NULL)
	{
	 SizeAvailable = AUD_InjectContext->BufferSize;
	}

	/* Do we have to datas to inject */
	/* ----------------------------- */
	if ((SizeToInject=AUDi_InjectGetFreeSize(AUD_InjectContext))!=0)
	{
	 /* Check if we have to split the copy into pieces into the source buffer */
	 if ((AUD_InjectContext->BufferConsumer+SizeToInject)>=(AUD_InjectContext->BufferBase+AUD_InjectContext->BufferSize))
	  {
	   SizeToInject=(AUD_InjectContext->BufferBase+AUD_InjectContext->BufferSize)-(AUD_InjectContext->BufferConsumer);
	  }
	 if (SizeToInject>=SizeAvailable) SizeToInject=SizeAvailable;

	 /* Inject the piece of datas */
	 if (SizeToInject!=0) AUDi_InjectCopy(AUD_InjectContext,SizeToInject);
	 SizeAvailable = SizeAvailable-SizeToInject;

	 /* Wrapping of the buffer */
	 if ((AUD_InjectContext->BufferConsumer)==(AUD_InjectContext->BufferBase+AUD_InjectContext->BufferSize))
	  {
	   AUD_InjectContext->BufferConsumer=AUD_InjectContext->BufferBase;
	   /* If all the stream is in memory */
	   if (AUD_InjectContext->FileHandle==0)
		{
		 AUD_InjectContext->NbLoopsAlreadyDone++;
		 /* If nb loop is not infinite */
		 if (AUD_InjectContext->NbLoops!=0)
		  {
		   /* If nb loops reached, switch in stop state */
		   STTBX_Print(("--> No of loops[%d] %d!\n",AUD_InjectContext->NbLoopsAlreadyDone,AUD_InjectContext->InjectorId));

		   if (AUD_InjectContext->NbLoops==AUD_InjectContext->NbLoopsAlreadyDone)
			{
				/*send EOF now*/
#ifndef ST_OSLINUX
			 if(AUD_InjectContext->SetEOFFlag == TRUE)
			 {
			 	STAUD_IPHandleEOF(AUD_InjectContext->AUD_Handle, AUD_InjectContext->InputObject);
			 }
#endif
				AUD_InjectContext->State = AUD_INJECT_STOP;
			 	break;
			}
		  }
		}
	  }
	}

	/* Sleep a while */
	/* ------------- */
    STOS_TaskDelay((signed long)ST_GetClocksPerSecond()/400); /* Sleep 10ms */

	}

	/* Finish the task */
	/* =============== */
    STOS_SemaphoreSignal(AUD_InjectContext->SemStop);
}
#endif /*ST_OSLINUX*/


static void AUDi_PCMInjectTask(void *InjectContext)
{
 AUD_InjectContext_t *AUD_InjectContext;
 U32                  SizeToInject;
 U32                  SizeAvailable;
 ST_ErrorCode_t ErrCode;
 STAUD_PCMBuffer_t PCMBuffer;
 U32 TotalSize = 1536 * 2 * 4 * 10;
 U32 NumQueued;
 U32 sent=0;
 U32 BufferSize;


 /* Retrieve context */
 /* ================ */
 AUD_InjectContext=(AUD_InjectContext_t *)InjectContext;
 	STTBX_Print((" Entering AUDi_PCMInjectTask \n"));


 /* Wait start of play */
 /* ================== */
	STOS_SemaphoreWait(AUD_InjectContext->SemStart);
 	STTBX_Print((" AUDi_PCMInjectTask Started \n"));
 SizeAvailable = SizeToInject = 0;
/*AUD_InjectContext->NbLoops=0;*/
	/* Get hold of PCM buffer Size */
	do
    {
    	/* 	Calling this function will set internal structure for Linux mmap function.BufferSize param is essentially not used here
    		but in linux version STAUD_IPGetPCMBufferSize() functions
    	*/
    	ErrCode = STAUD_IPGetPCMBufferSize(AUD_InjectContext->AUD_Handle, STAUD_OBJECT_INPUT_PCM0, &BufferSize);
    	STOS_TaskDelay((signed long)ST_GetClocksPerSecond()/100);
	}while(ErrCode != ST_NO_ERROR);

	/* Infinite loop */
	/* ============= */
	while(AUD_InjectContext->State==AUD_INJECT_START)
	{

	if (AUD_InjectContext->FileHandle==NULL)
	{
	 SizeAvailable = AUD_InjectContext->BufferSize;
	}

	/* Do we have to datas to inject */
	/* ----------------------------- */


    SizeToInject = TotalSize/10;

     /* Inject the piece of datas */

     /* Wrapping of the buffer */
		if ((AUD_InjectContext->BufferConsumer + SizeToInject)>=(AUD_InjectContext->BufferBase+AUD_InjectContext->BufferSize))
		{
			AUD_InjectContext->BufferConsumer=AUD_InjectContext->BufferBase;
			/* If all the stream is in memory */

			AUD_InjectContext->NbLoopsAlreadyDone++;
			/* If nb loop is not infinite */
			if (AUD_InjectContext->NbLoops!=0)
				{
					/* If nb loops reached, switch in stop state */
					STTBX_Print(("--> No of loops[%d] %d!\n",AUD_InjectContext->NbLoopsAlreadyDone,AUD_InjectContext->InjectorId));

					if (AUD_InjectContext->NbLoops==AUD_InjectContext->NbLoopsAlreadyDone)
					{
						AUD_InjectContext->State = AUD_INJECT_STOP;
						break;
					}
				}
                /*STTBX_Print(("PCMloop1\n"));*/
		}
		else
		{
				ErrCode = STAUD_IPGetPCMBuffer(AUD_InjectContext->AUD_Handle,STAUD_OBJECT_INPUT_PCM0,
															&PCMBuffer);
				if(ErrCode == ST_NO_ERROR)
				{
					if(PCMBuffer.Length >= SizeToInject)
					{
						memcpy((U8 *)(PCMBuffer.StartOffset),(U8 *)AUD_InjectContext->BufferConsumer,SizeToInject);
						PCMBuffer.Length = SizeToInject;
						NumQueued = 0;
						ErrCode = STAUD_IPQueuePCMBuffer (AUD_InjectContext->AUD_Handle,STAUD_OBJECT_INPUT_PCM0,
															&PCMBuffer,1, &NumQueued);
						if(ErrCode == ST_NO_ERROR)
						{
							AUD_InjectContext->BufferConsumer      += SizeToInject;
							sent++;
                            /*STTBX_Print(("PCMsent %d\n",(sent-1)));*/
						}
					}
					else
					{
						SizeToInject = PCMBuffer.Length;
					}
				}

			STTBX_Print(("PCMloop = %d\n",sent));
		}


   /* Sleep a while */
   /* ------------- */
	STOS_TaskDelay((signed long)ST_GetClocksPerSecond()/100); /* Sleep 10ms */

  }

 /* Finish the task */
 /* =============== */
	STOS_SemaphoreSignal(AUD_InjectContext->SemStop);
}

/* =======================================================================
   Name:        AUDi_InjectCallback
   Description: Audio event callback
   ======================================================================== */

static void AUDi_InjectCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{

 /* Identify audio decoder */
 /* ====================== */

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(EventData);
    UNUSED_PARAMETER(SubscriberData_p);

    /* Identify audio decoder */
	/* ====================== */
	AUDi_TraceDBG((" CallBack Reason = %d Event Data =%u, Sub Data =%u ", Reason, (U32)EventData, (U32)SubscriberData_p ));
	switch(Event)
	{
	case STAUD_NEW_FRAME_EVT        : AUDi_TraceDBG(("Sender(%s):**ERROR** !!! STAUD_NEW_FRAME_EVT !!!\n",RegistrantName)); break;
	case STAUD_PCM_UNDERFLOW_EVT    : AUDi_TraceDBG(("Sender(%s):**ERROR** !!! STAUD_NEW_FRAME_EVT !!!\n",RegistrantName)); break;

	case STAUD_PCM_BUF_COMPLETE_EVT : AUDi_TraceDBG(("Sender(%s):**ERROR** !!! STAUD_PCM_BUF_COMPLETE_EVT !!!\n"       ,RegistrantName)); break;
	case STAUD_FIFO_OVERF_EVT       : AUDi_TraceDBG(("Sender(%s):**ERROR** !!! STAUD_FIFO_OVERF_EVT !!!\n"       ,RegistrantName)); break;
	case STAUD_CDFIFO_UNDERFLOW_EVT : AUDi_TraceDBG(("Sender(%s):**ERROR** !!! STAUD_CDFIFO_UNDERFLOW_EVT !!!\n"       ,RegistrantName)); break;
	case STAUD_SYNC_ERROR_EVT       : AUDi_TraceDBG(("Sender(%s):**ERROR** !!! STAUD_SYNC_ERROR_EVT !!!\n"       ,RegistrantName)); break;
	case STAUD_LOW_DATA_LEVEL_EVT   : AUDi_TraceDBG(("Sender(%s):**ERROR** !!! STAUD_LOW_DATA_LEVEL_EVT !!!\n"       ,RegistrantName)); break;

	case STAUD_STOPPED_EVT	   : AUDi_TraceDBG(("Sender(%s): STAUD_STOPPED_EVT \n"       ,RegistrantName)); break;


   /* Strange event */
   /* ------------- */
   default :
    AUDi_TraceERR(("AUDi_InjectCallback(%d):**ERROR** !!! Event 0x%08x received but not managed !!!\n",AUDi_InjectContext[i]->InjectorId,Event));
    break;

  }
}

/* ========================================================================
   Name:        AUDi_GetWritePtr
   Description: Return the current write pointer of the buffer
   ======================================================================== */

#if defined(ST_5105)||defined(ST_7100)||defined(ST_7109)
static ST_ErrorCode_t AUDi_GetWritePtr(void *const Handle,void **const Address)
{
 AUD_InjectContext_t *AUD_InjectContext;
U32 i;

 /* NULL address are ignored ! */
 /* -------------------------- */
 if (Address==NULL)
  {
   AUDi_TraceERR(("AUDi_SetReadPtr(?):**ERROR** !!! Address is NULL pointer !!!\n"));
   return(ST_ERROR_BAD_PARAMETER);
  }
for(i= 0;i<AUD_MAX_NUMBER;i++)
	{
	if(Handle == AUDi_InjectContext[i])
		break;
	if(i== AUD_MAX_NUMBER-1)
		return(ST_ERROR_BAD_PARAMETER);
	}

 /* Retrieve context */
 /* ---------------- */
 AUD_InjectContext=(AUD_InjectContext_t *)Handle;
 if (AUD_InjectContext==NULL)
  {
   AUDi_TraceERR(("AUDi_GetWritePtr(?):**ERROR** !!! Context is NULL pointer !!!\n"));
   return(ST_ERROR_BAD_PARAMETER);
  }


#ifdef ST_OSLINUX
 if (AUD_InjectContext->BufferMapped == FALSE)
  {
	{
                   AUDi_TraceDBG(("AUDi_GetWritePtr(?):**ERROR** !!! Buffer is not mapped to user space !!!\n"));

 	}

   return(ST_ERROR_BAD_PARAMETER);
  }
#else
if (AUD_InjectContext->Task==NULL)
  {
   AUDi_TraceERR(("AUDi_GetWritePtr(?):**ERROR** !!! Context is NULL pointer !!!\n"));
   return(ST_ERROR_BAD_PARAMETER);
  }
#endif
#ifdef ST_OSLINUX
    /* Don't inject if not running */
    if (AUD_InjectContext->State == AUD_INJECT_START)
    {
        U32 SizeToInject;
        U32 SizeAvailable;

        /* Determine how much space is in the PES buffer */
        if (AUD_InjectContext->BufferAudioProducer >= AUD_InjectContext->BufferAudioConsumer)
            SizeToInject = AUD_InjectContext->BufferAudioSize-
                          (AUD_InjectContext->BufferAudioProducer - AUD_InjectContext->BufferAudioConsumer);
        else
            SizeToInject = AUD_InjectContext->BufferAudioConsumer - AUD_InjectContext->BufferAudioProducer;


        /* "BufferAudioProducer == BufferAudioConsumer" is ambiguous - full or empty? */
        if (SizeToInject >= 4)
            SizeToInject -= 4;

        /* Amount left to inject */
        SizeAvailable = AUD_InjectContext->BufferSize - (AUD_InjectContext->BufferConsumer - AUD_InjectContext->BufferBase);

	/* Can only inject up to the amount of free space */
        if (SizeToInject > SizeAvailable)
            SizeToInject = SizeAvailable;

         AUDi_TraceDBG(("BufferAudioProducer=0x%08x, BufferAudioConsumer=0x%08x:SizeToInject = %d,SizeAvailable = %d \n",
                           AUD_InjectContext->BufferAudioProducer, AUD_InjectContext->BufferAudioConsumer,SizeToInject,SizeAvailable));

	/* Inject the data */
        if (SizeToInject != 0)
            AUDi_InjectCopy(AUD_InjectContext,SizeToInject);
        else
        {

            AUDi_TraceDBG(("No space to inject into: BufferAudioProducer=0x%08x, BufferAudioConsumer=0x%08x\n",
                           AUD_InjectContext->BufferAudioProducer, AUD_InjectContext->BufferAudioConsumer));

        }

        /* Determine if we have finished injecting */
        if (SizeToInject == SizeAvailable)
        {
            AUD_InjectContext->NbLoopsAlreadyDone++;
            AUD_InjectContext->BufferConsumer = AUD_InjectContext->BufferBase;
            STTBX_Print(("--> No of loops[%d] %d!\n",AUD_InjectContext->NbLoopsAlreadyDone,AUD_InjectContext->InjectorId));
            if (AUD_InjectContext->NbLoops == AUD_InjectContext->NbLoopsAlreadyDone)
                AUD_InjectContext->State = AUD_INJECT_STOP;
        }
    }
#endif
 /* Set the value of the write pointer */
 /* ---------------------------------- */
	STOS_SemaphoreWait(AUD_InjectContext->SemLock);
#if (defined(ST_7100)||defined(ST_7109)) && !defined(ST_OSLINUX)
 *Address=(void *)((U32)(AUD_InjectContext->BufferAudioProducer)|0xA0000000);
#else
 *Address=(void *)AUD_InjectContext->BufferAudioProducer;
#endif
	STOS_SemaphoreSignal(AUD_InjectContext->SemLock);

 /* Return no errors */
 /* ---------------- */
 /*AUDi_TraceDBG(("AUDi_GetWritePtr(%d): Address=0x%08x\n",AUD_InjectContext->InjectorId,*Address));*/
 return(ST_NO_ERROR);
}
#endif

/* ========================================================================
   Name:        AUDi_SetReadPtr
   Description: Set the read pointer of the back buffer
   ======================================================================== */

#if defined(ST_5105)||defined(ST_7100)||defined(ST_7109)
static ST_ErrorCode_t AUDi_SetReadPtr(void *const Handle,void *const Address)
{
 AUD_InjectContext_t *AUD_InjectContext;
U32 i;
 /* NULL address are ignored ! */
 /* -------------------------- */
 if (Address==NULL)
  {
   AUDi_TraceERR(("AUDi_SetReadPtr(?):**ERROR** !!! Address is NULL pointer !!!\n"));
   return(ST_ERROR_BAD_PARAMETER);
  }
for(i= 0;i<AUD_MAX_NUMBER;i++)
	{
	if(Handle == AUDi_InjectContext[i])
		break;
	if(i== AUD_MAX_NUMBER-1)
		return(ST_ERROR_BAD_PARAMETER);
	}

 /* Retrieve context */
 /* ---------------- */
 AUD_InjectContext=(AUD_InjectContext_t *)Handle;
 if (AUD_InjectContext==NULL)
  {
   AUDi_TraceERR(("AUDi_SetReadPtr(?):**ERROR** !!! Context is NULL pointer !!!\n"));
   return(ST_ERROR_BAD_PARAMETER);
  }

#ifdef ST_OSLINUX
 if (AUD_InjectContext->BufferMapped == FALSE)
  {
   STTBX_Print(("AUDi_SetReadPtr(?):**ERROR** !!! Buffer is not mapped to user space !!!\n"));
   return(ST_ERROR_BAD_PARAMETER);
  }
#else
 if (AUD_InjectContext->Task==NULL)
  {
   AUDi_TraceERR(("AUDi_SetReadPtr(?):**ERROR** !!! Context is NULL pointer !!!\n"));
   return(ST_ERROR_BAD_PARAMETER);
  }
#endif

 /* Set the value of the read pointer */
 /* --------------------------------- */
	STOS_SemaphoreWait(AUD_InjectContext->SemLock);
	AUD_InjectContext->BufferAudioConsumer=(U32)Address;
	STOS_SemaphoreSignal(AUD_InjectContext->SemLock);

 /* Return no errors */
 /* ---------------- */
 /*AUDi_TraceDBG(("AUDi_SetReadPtr(%d): Address=0x%08x\n",AUD_InjectContext->InjectorId,Address));*/
 return(ST_NO_ERROR);
}
#endif




#endif

BOOL AUD_RegisterCmd()
{

#if 1
    BOOL RetOk;
    int i;

#ifdef ST_OS21
	SwitchTaskOKSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);
#endif

    /* Initialize injection task routines */
    for(i=0; i<MAX_INJECT_TASK; i++)
    {
        InjectTaskTable[i].FileBuffer = NULL;
        InjectTaskTable[i].NbBytes = 0;
#ifdef ST_OSLINUX
        InjectTaskTable[i].BufferMapped = FALSE;
#else
        InjectTaskTable[i].TaskID = NULL;
#endif
        InjectTaskTable[i].Loop = 0;
        InjectTaskTable[i].Injector_p = NULL;
        InjectTaskTable[i].AllocatedBuffer = NULL;
        InjectTaskTable[i].WritePES_p = 0;
        InjectTaskTable[i].ReadPES_p = 0;
        InjectTaskTable[i].BasePES_p = 0;
		InjectTaskTable[i].TopPES_p = 0;

    }
   /* InjectTaskTable[0].Injector_p = &DoAudioInject;*/
	/*InjectTaskTable[1].Injector_p = &DoAC3AudioInject;*/

    LocalDACConfiguration.Initialized = FALSE;
    Verbose = FALSE;

    RetOk = AUD_InitCommand();

#if 1
    if (RetOk==TRUE)
    {
        RetOk = AUD_Debug();
    }
#endif

#if defined (CHECK_STACK_USAGE)
    stack_test();
#endif

    return(RetOk);
#endif

} /* end AUD_CmdStart */




/* ========================================================================
   Name:        AUD_Debug
   Description: Register the debug commands for AUD class
   ======================================================================== */


ST_ErrorCode_t AUD_Debug(void)
{
 /* Register command */
 /* ---------------- */
	BOOL RetOK = TRUE;

#if 0
 register_command("AUD_INJSTART"  , TT_AUD_InjectStart , "<Id> <FileName> <NbLoops> <Stream Content> <Sampling Frequency> <Stream Type> Play a file in injection");
 register_command("AUD_INJSTOP"   , TT_AUD_InjectStop  , "<Id> Stop an audio injection");
#endif
 /* Assign integers */
 /* --------------- */
 assign_integer("A_AC3"           , STAUD_STREAM_CONTENT_AC3         , TRUE);
 assign_integer("A_DTS"           , STAUD_STREAM_CONTENT_DTS         , TRUE);
 assign_integer("A_MP1"           , STAUD_STREAM_CONTENT_MPEG1       , TRUE);
 assign_integer("A_MP2"           , STAUD_STREAM_CONTENT_MPEG2       , TRUE);
 assign_integer("A_MP3"           , STAUD_STREAM_CONTENT_MP3         , TRUE);
 assign_integer("A_PCM"           , STAUD_STREAM_CONTENT_PCM         , TRUE);
 assign_integer("A_MLP"           , STAUD_STREAM_CONTENT_MLP         , TRUE);
 assign_integer("A_LPCM"           , STAUD_STREAM_CONTENT_LPCM       , TRUE);
 assign_integer("A_AAC"           , STAUD_STREAM_CONTENT_MPEG_AAC    , TRUE);
 assign_integer("A_HEAAC"		  ,	STAUD_STREAM_CONTENT_HE_AAC		 , TRUE);
 assign_integer("A_WMA"			  ,	STAUD_STREAM_CONTENT_WMA		 , TRUE);
 assign_integer("A_WMAPROLSL"	 , STAUD_STREAM_CONTENT_WMAPROLSL	, TRUE);
 assign_integer("A_DDP"           , STAUD_STREAM_CONTENT_DDPLUS      , TRUE);
 assign_integer("A_CDDA"           , STAUD_STREAM_CONTENT_CDDA     , TRUE);
 assign_integer("A_BEEP"          , STAUD_STREAM_CONTENT_BEEP_TONE   , TRUE);
 assign_integer("A_PINKNOISE"  , STAUD_STREAM_CONTENT_PINK_NOISE, TRUE);
 assign_integer("A_ES"            , STAUD_STREAM_TYPE_ES             , TRUE);
 assign_integer("A_PES"           , STAUD_STREAM_TYPE_PES            , TRUE);
 	assign_integer("A_PES_PKTMPEG1"  , STAUD_STREAM_TYPE_MPEG1_PACKET   , TRUE);
	assign_integer("A_PES_AD"        , STAUD_STREAM_TYPE_PES_AD         , TRUE);
	assign_integer("A_PES_DVD"		 , STAUD_STREAM_TYPE_PES_DVD        , TRUE);
	assign_integer("A_PES_DVDA"	     , STAUD_STREAM_TYPE_PES_DVD_AUDIO  , TRUE);
	assign_integer("A_OFF"           , STAUD_DIGITAL_MODE_OFF           , TRUE);
	assign_integer("A_COMPRESSED"    , STAUD_DIGITAL_MODE_COMPRESSED    , TRUE);
	assign_integer("A_NONCOMPRESSED" , STAUD_DIGITAL_MODE_NONCOMPRESSED , TRUE);
	assign_integer("A_SPDIF"         , STAUD_TYPE_OUTPUT_SPDIF          , TRUE);
	assign_integer("A_STEREO"        , STAUD_TYPE_OUTPUT_STEREO         , TRUE);


 /* Return no error */
 /* --------------- */
 return(RetOK);
}

/*#######################################################################*/




