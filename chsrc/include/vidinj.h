/************************************************************************
COPYRIGHT (C) STMicroelectronics 2004

File name   : vidinj.h

************************************************************************/
#include "stvid.h"
#include "stpti.h"


#define VIDEO_MAX_DEVICE                1
#define VID_INJECTEDSIZEHISTORYSIZE	    16
#define VID_MEMINJECT_MAXJOBNUMBER      64 /* 64 * 512ko = 32 Mo */
#define VID_INJECTION_TASK_PRIORITY     (MIN_USER_PRIORITY+10)
#define VID_MAX_BUFFER_LOAD 			1
#define VID_MAX_STREAM_NAME 			256
#define VID_INVALID_INJECTEDSIZE        ((U32)-1)


typedef enum VID_InjectionState_s
{
    STATE_INJECTION_NORMAL = 0,
    STATE_INJECTION_STOPPED_BY_EVT = 1  /* Bit 0 = 1 => Stopped by evt  */
} VID_InjectionState_e;

typedef enum VID_DriverState_s
{
    STATE_DRIVER_STARTED,
    STATE_DRIVER_STOPPED,
    STATE_DRIVER_STOP_ENDOFDATA_ASKED
} VID_DriverState_e;

typedef enum VID_InjectionType_s
{
    NO_INJECTION = 0,
    VID_HDD_INJECTION = 1,
    VID_LIVE_INJECTION = 2,
    VID_MEMORY_INJECTION = 3,
    VID_LAST_INJECTION = VID_MEMORY_INJECTION
} VID_InjectionType_e;

typedef struct
{
    U32 RequestedSize;
} PtiMemInjectJob_t;

typedef struct VID_MemInjection_s
{
    U32            Requested;
    U8 *           SrcBuffer_p;
    U8             LoadBufferNb;
    U8 *           CurrentSrcPtr_p;
    U32            SrcSize;
    S32            LoopNbr;
    S32            LoopNbrDone;
    PtiMemInjectJob_t Jobs[VID_MEMINJECT_MAXJOBNUMBER];
    U32 	   	   JobWriteIndex;
    U32 	       JobReadIndex;
    BOOL           JobNumberOverflow;
    BOOL           SynchronizedByUnderflowEvent;
    U32 	       InjectedSizeHistory[VID_INJECTEDSIZEHISTORYSIZE];
    U32 	       InjectedSizeHistoryIndex;
} VID_MemInjection_t;

typedef struct VID_HDDInjection_s
{
    U32 Dummy;
} VID_HDDInjection_t;

typedef struct VID_LiveInjection_s
{
    U32 Pid;
    U32 SlotNb;
    U32 DeviceNb;
} VID_LiveInjection_t;

typedef struct VID_PtiInjection_s
{
     U32 Dummy;
} VID_PtiInjection_t;

typedef struct VID_VideoDriver_s
{
    STVID_Handle_t Handle;
    VID_DriverState_e DriverState;
    ST_DeviceName_t DeviceName;
} VID_VideoDriver_t;

typedef struct VID_Injection_s
{
    semaphore_t* pAccess;           /* Access protection to structure */
    VID_InjectionType_e Type;     /* Injection type memory, hdd,... */
    VID_InjectionState_e State;   /* Injection state normal, stopped by evt,.. */
    U32 Address;                  /* CD fifo address */
    U32 Number;                   /* CD fifo number */
    U32 BitBufferSize;            /* Bit buffer size (necessary for video on 7015) */
    /* video input-buffer parameter */
    void *          Base_p;
    void *          Top_p;
    void *          Write_p;
    void *          Read_p;
    STPTI_Buffer_t  HandleForPTI;
    VID_VideoDriver_t Driver;  /* Pointer to infos of video driver used */
    union
    {
        VID_HDDInjection_t HDD;
        VID_LiveInjection_t Live;
        VID_MemInjection_t Memory;
    } Config;
} VID_Injection_t;

/*-------------------------------------------------------------------------
 * Function : VID_DecodeFromMemory
 *            Inject Video in Memory to DMA
 * Input    : STVID_Handle_t VidHandle, S32 FifoNb, S32 InjectLoop, S32 BufferNb
 * Output   : 
 * Return   : ST_ErrorCode_t ErrCode
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t VID_DecodeFromMemory(STVID_Handle_t VidHandle, S32 FifoNb, S32 NbLoops, S32 BuffNb);

/*-------------------------------------------------------------------------
 * Function : VID_MemInject
 *            Inject Video in Memory to DMA
 * Input    : STVID_Handle_t VidHandle, S32 FifoNb, S32 InjectLoop, S32 BufferNb
 * Output   : 
 * Return   : ST_ErrorCode_t ErrCode
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t VID_MemInject(STVID_Handle_t VidHandle, S32 FifoNb, S32 InjectLoop, S32 BufferNb);

/*-------------------------------------------------------------------------
 * Function : VID_Load
 *            Load an mpeg file in memory
 * Input    : char *Filename, S32 LVar
 * Output   : 
 * Return   : ST_ErrorCode_t ErrCode
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t VID_Load(char *Filename, S32 LVar);

/*-------------------------------------------------------------------------
 * Function : VID_PrepareInjection
 *            Prepare for memory injection
 * Input    : 
 * Parameter:
 * Output   : 
 * Return   : 
 * ----------------------------------------------------------------------*/
void VID_PrepareInjection(void);

