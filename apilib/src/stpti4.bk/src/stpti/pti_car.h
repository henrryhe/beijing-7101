/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_car.h
 Description: carousel support

******************************************************************************/


#ifndef STPTI_CAROUSEL_SUPPORT
 #error Incorrectly included file!
#endif


#ifndef __PTI_CAR_H
 #define __PTI_CAR_H


/* Includes ------------------------------------------------------------ */


#include "pti_loc.h"


/* Exported Types ------------------------------------------------------ */


typedef volatile struct CarouselSimpleEntry_s 
{
    STPTI_Handle_t            OwnerSession;
    STPTI_CarouselEntry_t     Handle;
    STPTI_InjectionType_t     InjectionType;
    STPTI_CarouselEntryType_t EntryType;
    FullHandle_t              NextEntry;
    FullHandle_t              PrevEntry;
    FullHandle_t              NextPidEntry;
    FullHandle_t              PrevPidEntry;
    BOOL                      Linked;
    U8                        CCValue;
    STPTI_Pid_t               Pid;
    U8                        FromByte;
    FullHandle_t              CarouselListHandle;   /* Associations */
    STPTI_TSPacket_t          PacketData;           /* 'Simple' specific data */   
} CarouselSimpleEntry_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct CarouselTimedEntry_s 
{
    STPTI_Handle_t            OwnerSession;
    STPTI_CarouselEntry_t     Handle;
    STPTI_InjectionType_t     InjectionType;
    STPTI_CarouselEntryType_t EntryType;
    FullHandle_t              NextEntry;
    FullHandle_t              PrevEntry;
    FullHandle_t              NextPidEntry;
    FullHandle_t              PrevPidEntry;
    BOOL                      Linked;
    U8                        LastCC;
    STPTI_Pid_t               Pid;
    FullHandle_t              CarouselListHandle;    /* Associations */
    U8                       *PacketData_p;   /* 'Timed' specific data */    
    STPTI_TimeStamp_t         MinOutputTime;
    STPTI_TimeStamp_t         MaxOutputTime;
    BOOL                      EventOnOutput;
    BOOL                      EventOnTimeout;
    U32                       PrivateData;    
    U8                        FromByte;
    U8                        CCValue;
} CarouselTimedEntry_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct Carousel_s 
{
    STPTI_Handle_t      OwnerSession;
    STPTI_Carousel_t    Handle;
    FullHandle_t        LockingSession;
    STPTI_AllowOutput_t OutputAllowed;
    FullHandle_t        StartEntry;
    FullHandle_t        EndEntry;
    FullHandle_t        NextEntry;
    FullHandle_t        CurrentEntry;   /* Stored Current Entry Parameters to save time in Carousel Process */
    BOOL                UnlinkOnNextSem;
    STPTI_TimeStamp_t   MaxOutputTime;
    BOOL                EventOnOutput;
    BOOL                EventOnTimeout;
    U32                 PrivateData;    
    U32                 PacketTimeTicks;
    U32                 CycleTimeTicks;
    task_t             *CarouselTaskID_p;
    /* to store Carousel Task stack pointer <GNBvd21068>*/
    void	        *CarTaskStack;
    semaphore_t        *Sem_p;
    BOOL                AbortFlag;
    FullHandle_t        CarouselEntryListHandle;    /* Associations */
    FullHandle_t        BufferListHandle;
} Carousel_t;



/* Exported Function Prototypes ---------------------------------------- */



#endif  /* __PTI_CAR_H */
