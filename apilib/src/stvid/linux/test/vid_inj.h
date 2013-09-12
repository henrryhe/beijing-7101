/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : vid_inj.h
Description : Definitions for memory injection management
 *            for video injection with vid_decode and vid_inj
************************************************************************/
#ifndef __VID_INJ_H__
#define __VID_INJ_H__

#include "stvid.h"

/* --- Constants ------------------------------------------------------- */

#define VID_INJECTEDSIZEHISTORYSIZE	    16

#define VID_MEMINJECT_MAXJOBNUMBER      64 /* 64 * 512ko = 32 Mo */

typedef enum VID_DriverState_s
{
    STATE_DRIVER_STARTED,
    STATE_DRIVER_STOPPED,
    STATE_DRIVER_STOP_ENDOFDATA_ASKED
} VID_DriverState_e;

typedef enum VID_InjectionState_s
{
    STATE_INJECTION_NORMAL = 0,
    STATE_INJECTION_STOPPED_BY_EVT = 1  /* Bit 0 = 1 => Stopped by evt  */
} VID_InjectionState_e;


typedef enum VID_InjectionType_s
{
    NO_INJECTION = 0,
    VID_HDD_INJECTION = 1,
    VID_LIVE_INJECTION = 2,
    VID_MEMORY_INJECTION = 3,
    VID_FAST_FILESYSTEM_INJECTION = 4,
    VID_LAST_INJECTION = VID_FAST_FILESYSTEM_INJECTION
} VID_InjectionType_e;

/* --- Structures ------------------------------------------------------ */

typedef struct VID_VideoDriver_s
{
    STVID_Handle_t Handle;
    VID_DriverState_e DriverState;
    ST_DeviceName_t DeviceName;
} VID_VideoDriver_t;

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
    U32            JobWriteIndex;
    U32            JobReadIndex;
    BOOL           JobNumberOverflow;
    BOOL           SynchronizedByUnderflowEvent;
    U32            InjectedSizeHistory[VID_INJECTEDSIZEHISTORYSIZE];
    U32            InjectedSizeHistoryIndex;
    char           FileName[256];
    void           *FileHandle;
    U32            NeedToRealignTransfer;
    U32            MaxDmaTransferSize;
    U32            MinDmaTransferSize;
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


typedef struct VID_Injection_s
{
#ifdef ST_OSLINUX
    /* This structure has to be checked against Linux */
    /* since some of its fields are either used in kernel or in user or both ... */
#endif

    semaphore_t* Access_p;        /* Access protection to structure */
    VID_InjectionType_e Type;     /* Injection type hdd, memory, ... */
    VID_InjectionState_e State;   /* Injection state normal, stopped by evt,.. */
    U32 Address;                  /* CD fifo address */
    U32 Number;                   /* CD fifo number */
    U32 BitBufferSize;            /* Bit buffer size */
#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109) || defined(ST_ZEUS)|| defined (ST_7200)
    /* video input-buffer parameter */
    void *          Base_p;
    void *          Top_p;
    void *          Write_p;
    void *          Read_p;
#if defined (USE_PTI)  && ! defined(MODULE)
    STPTI_Buffer_t  HandleForPTI;
#endif /* USE_PTI */
#ifdef USE_DEMUX
    STDEMUX_Buffer_t HandleForPTI;
#endif /* USE_DEMUX */
#endif /* ST_51xx_Device ST_7710 ST_7100 ST_7109 ST_ZEUS ST_7200 */

    VID_VideoDriver_t Driver;    /* Pointer to infos of video driver used */

    union
    {
        VID_HDDInjection_t HDD;
        VID_LiveInjection_t Live;
        VID_MemInjection_t Memory;
    } Config;
} VID_Injection_t;

/* --- Externals ------------------------------------------------------- */

extern BOOL VID_Inj_GetDeviceName (U32 index, char *name);
extern STVID_Handle_t  VID_Inj_GetDriverHandle (U32 index);
extern S16  VID_Inj_GetInjectionType (U32 index);
extern S16  VID_Inj_GetVideoDriverState (U32 index);
extern BOOL VID_Inj_IsDeviceNameUndefined (U32 index);
extern BOOL VID_Inj_IsDeviceNameIdentical (U32 index, char *name);
extern BOOL VID_Inj_SetDriverHandle (U32 index, STVID_Handle_t value);
extern BOOL VID_Inj_SetDeviceName (U32 index, char *name);
extern BOOL VID_Inj_SetInjectionType (U32 index, VID_InjectionType_e type);
extern BOOL VID_Inj_SetVideoDriverState (U32 index, VID_DriverState_e state);
extern void VID_Inj_SignalAccess (U32 index);
extern BOOL VID_Inj_ResetBitBufferSize (U32 index);
extern BOOL VID_Inj_ResetDeviceAndHandle (U32 index);
extern void VID_Inj_WaitAccess (U32 index);

#endif /* #ifndef __VID_INJ_H__ */

/* End of vid_inj.h */
