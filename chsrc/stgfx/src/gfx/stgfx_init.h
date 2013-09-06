/*******************************************************************************

File name   : stgfx_init.h

Description : stgfx init header file (device/unit-related definitions)

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                Name
----               ------------                                ----
2000-08-24         Created                                     Adriano Melis
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STGFX_INIT_H
#define __STGFX_INIT_H


/* Includes ----------------------------------------------------------------- */
#include "stgfx.h" /* includes stblit, stlite, stddefs */
#include "stavmem.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STGFX_INTERNAL_ACCURACY    6
#define STGFX_MAX_NUM_POLY_POINTS 25
#define STGFX_MAX_LINE_WIDTH      100 /*25*/


/* Exported Types ----------------------------------------------------------- */

/* device descriptor */
typedef struct
{
  ST_DeviceName_t            DeviceName;     /* STGFX device name */
  ST_Partition_t*            CPUPartition_p; /* CPU mem partition used at Init time */
  void*                      MemoryAllocated_p; /* Ptr to CPU memory to free at term */
  STAVMEM_PartitionHandle_t  AVMemPartitionHandle; /* AVMEM partition to use */
  STAVMEM_SharedMemoryVirtualMapping_t SharedMemMap;
  U8                         NumBitsAccuracy; /* bits for fixed point */
  ST_Partition_t*            DevicePartition_p; /* CPU internal part */
  ST_DeviceName_t            STBLITDeviceName;  /* blitter device name */
  STBLIT_Handle_t            BlitHandle;        /* blitter handle */
  ST_DeviceName_t            EventHandlerName;
  
} stgfx_Device_t;

/* connection (unit) descriptor */
typedef struct
{
  /* pointer to the device descriptor (NULL for unused slot) */
  stgfx_Device_t*               Device_p;

  /* semaphore used to signal completion of all pending blitter operations */
#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
  semaphore_t                   SyncSemaphore;
#else /*yxl 2006-11-30 add below line*/
  semaphore_t*                  pSyncSemaphore;
#endif/*end  yxl 2006-11-30 add below line*/

  /* data needed to communicate with the EventHandler */
  STEVT_Handle_t                EventHandlerHandle;
  STEVT_SubscriberID_t          GfxSubscriberID;
  STEVT_DeviceSubscribeParams_t GfxSubscribeParams;
  BOOL                          BlitterRegistered;        

  /* data needed by the stolf library */
  void*                         PfrData_p;

} stgfx_Unit_t;


/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#if defined(ST_GX1) || defined (ST_7020)
/* hardware blitters do bad things if passed an XYL element with L = 0 */
#define STGFX_AVOID_ZERO_LENGTH
#endif

#if defined(ST_7020)
/* On some two-chip systems, we get corruption if we try to operate bytewise on
  the SDRAM attached to the backend chip. For some cases (bitmap fonts), it is
  possible and more efficient to write wordwise anyway; for others (teletext)
  the core algorithms currently work bytewise, and we have to introduce an
  extra wordwise copy to AVMEM at the end */
#define STGFX_WRITE_WORDS_TO_AVMEM
#endif

/* macros to support old code */
#define stgfx_GetUserAccuracy(Handle) \
            (((stgfx_Unit_t*)(Handle))->Device_p->NumBitsAccuracy)

#define stgfx_GetEventSubscriberID(Handle) \
            (((stgfx_Unit_t*)(Handle))->GfxSubscriberID)
            
#define stgfx_GetBlitterHandle(Handle) \
            (((stgfx_Unit_t*)(Handle))->Device_p->BlitHandle)
            
#define stgfx_DEVMEM_malloc(Handle, size) \
  memory_allocate(((stgfx_Unit_t*)(Handle))->Device_p->DevicePartition_p, (size))

#define stgfx_DEVMEM_free(Handle, p) \
  memory_deallocate(((stgfx_Unit_t*)(Handle))->Device_p->DevicePartition_p, (p))

#define stgfx_CPUMEM_malloc(Handle, size) \
  memory_allocate(((stgfx_Unit_t*)(Handle))->Device_p->CPUPartition_p, (size))

#define stgfx_CPUMEM_free(Handle, p) \
  memory_deallocate(((stgfx_Unit_t*)(Handle))->Device_p->CPUPartition_p, (p))


/* Exported Functions ------------------------------------------------------- */

/* malloc/free for CPU memory */
/* their use is straightforward */
/*void* stgfx_DEVMEM_malloc(STGFX_Handle_t Handle, size_t size);*/
/*void stgfx_DEVMEM_free(STGFX_Handle_t Handle, void* p);*/
/* Now defined by macros above */

/* malloc/free for shared memory */
/* returns the physical address of the allocated block */
ST_ErrorCode_t stgfx_AVMEM_malloc(
  STGFX_Handle_t          Handle,
  U32                     Size,
  STAVMEM_BlockHandle_t*  AVMEM_BlockHandle_p,
  void **                 PAddr_p
);

/* Gets physical address for a block you allocated some other way */
ST_ErrorCode_t stgfx_AVMEM_GetPhysAddr(
  STGFX_Handle_t          Handle,
  STAVMEM_BlockHandle_t   AVMEM_BlockHandle_p,
  void **                 PAddr_p
);

ST_ErrorCode_t stgfx_AVMEM_free(
  STGFX_Handle_t           Handle,
  STAVMEM_BlockHandle_t*   AVMEM_BlockHandle_p
);

/* delayed avmem free, when pending BLIT operations have completed */
ST_ErrorCode_t stgfx_ScheduleAVMEM_free(
  STGFX_Handle_t           Handle,
  STAVMEM_BlockHandle_t*   AVMEM_BlockHandle_p
);


/* get the unit structure for a given handle, or NULL if invalid
  (caller should return ST_ERROR_INVALID_HANDLE) */
stgfx_Unit_t * stgfx_HandleToUnit(STGFX_Handle_t Handle);

/* test optimised stblit */
#ifdef STGFX_QUICK_DRAW_XYL_ARRAY
ST_ErrorCode_t stgfx_QuickDrawXYLArray( STBLIT_Handle_t       Handle,
                                        STGXOBJ_Bitmap_t*     Bitmap_p,
                                        STBLIT_XYL_t*         XYLArray_p,
                                        U32                   SegmentsNumber,
                                        STGXOBJ_Color_t*      Color_p,
                                        STBLIT_BlitContext_t* BlitContext_p );
#define STBLIT_DrawXYLArray stgfx_QuickDrawXYLArray
#endif

/* Time parts of operations
 - move to unit structure to make thread-safe */
#ifdef STGFX_TIME_OPS

typedef struct
{
    clock_t Time;
    char* Descr_p;
} stgfx_Timing_t;

#define STGFX_MAX_TIMINGS 0x10
#define STGFX_STOP_TIMING 0x20
extern int stgfx_NextTimeSlot; /* bits 0-3 indicate the next slot to use,
                                 bit 4 whether recording is stopped */
extern stgfx_Timing_t stgfx_Timings[STGFX_MAX_TIMINGS];

#define STOP_TIMING() stgfx_NextTimeSlot |= STGFX_STOP_TIMING;
#define START_TIMING() stgfx_NextTimeSlot &= ~STGFX_STOP_TIMING;
#define CLEAR_TIMINGS() stgfx_NextTimeSlot &= STGFX_STOP_TIMING;
#define GET_NUM_TIMINGS() (stgfx_NextTimeSlot & ~STGFX_STOP_TIMING)
#define TIME_HERE(Descr) \
    if(stgfx_NextTimeSlot < STGFX_MAX_TIMINGS) \
    { \
        stgfx_Timings[stgfx_NextTimeSlot].Time = time_now(); \
        stgfx_Timings[stgfx_NextTimeSlot].Descr_p = Descr; \
        ++stgfx_NextTimeSlot; \
    }
    
#ifndef STGFX_QUICK_DRAW_XYL_ARRAY
#define TIME_SYNC(GFXHandle) STGFX_Sync(GFXHandle)
#else
#define TIME_SYNC(GFXHandle)
#endif

#else
#define STOP_TIMING()
#define START_TIMING() 
#define CLEAR_TIMINGS()
#define TIME_HERE(Descr)
#define TIME_SYNC(GFXHandle)
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STGFX_INIT_H */

/* End of stgfx_init.h */
