/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005-2007.  All rights reserved.

   File Name: memget.c
 Description: function wrappers for memory access macros

******************************************************************************/


/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>
#include <stdio.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "stddefs.h"
#include "sttbx.h"

#include "pti_hndl.h"
#include "pti_loc.h"

#include "memget.h"


/* ------------------------------------------------------------------------- */


#define DEVICE_PTR(FullHandle) \
                    ((Device_t*)&(STPTI_Driver.MemCtl.Handle_p[(FullHandle).member.Device].Data_p->data))

#define SESSION_PTR(FullHandle) \
                    ((Session_t*)&(DEVICE_PTR(FullHandle)->MemCtl.Handle_p[(FullHandle).member.Session].Data_p->data))

#define OBJECT_PTR(FullHandle) \
                    &(SESSION_PTR(FullHandle)->MemCtl.Handle_p[(FullHandle).member.Object].Data_p->data)

#define OBJECT_LIST_PTR(FullListHandle) \
                    ((List_t*)&(SESSION_PTR(FullListHandle)->MemCtl.Handle_p[FullListHandle.member.Object].Data_p->data))



/* ------------------------------------------------------------------------- */

#ifdef DEBUG
static void BreakPointTime(void)
{
    int i;

    /* Set your breakpoint here ! */
    i = i;

    /* OR */
    ASSERT (1 == 0);
}
#endif /* DEBUG */


__INLINE Device_t *stptiMemGet_Device( FullHandle_t AnyHandle )
{
#ifdef DEBUG
    if ((AnyHandle.word != STPTI_NullHandle()) &&
        (AnyHandle.word != 0))
    {
#endif /* DEBUG */
        return( DEVICE_PTR(AnyHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}


/* ------------------------------------------------------------------------- */


__INLINE TCPrivateData_t *stptiMemGet_PrivateData( FullHandle_t AnyHandle )
{
#ifdef DEBUG
    if ((AnyHandle.word != STPTI_NullHandle()) &&
        (AnyHandle.word != 0))
    {
#endif /* DEBUG */
        return( &(stptiMemGet_Device(AnyHandle)->TCPrivateData) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}


/* ------------------------------------------------------------------------- */


__INLINE Slot_t *stptiMemGet_Slot( FullHandle_t SlotHandle )
{
#ifdef DEBUG
    if ((SlotHandle.word != STPTI_NullHandle()) &&
        (SlotHandle.word != 0) &&
        (SlotHandle.member.ObjectType == OBJECT_TYPE_SLOT))
    {
#endif /* DEBUG */
        return( (Slot_t *)OBJECT_PTR(SlotHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}


/* ------------------------------------------------------------------------- */


__INLINE Buffer_t *stptiMemGet_Buffer( FullHandle_t BufferHandle )
{
#ifdef DEBUG
    if ((BufferHandle.word != STPTI_NullHandle()) &&
        (BufferHandle.word != 0) &&
        (BufferHandle.member.ObjectType == OBJECT_TYPE_BUFFER))
    {
#endif /* DEBUG */
        return( (Buffer_t *)OBJECT_PTR(BufferHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}


/* ------------------------------------------------------------------------- */


__INLINE Filter_t *stptiMemGet_Filter( FullHandle_t FilterHandle )
{
#ifdef DEBUG
    if ((FilterHandle.word != STPTI_NullHandle()) &&
        (FilterHandle.word != 0) &&
        (FilterHandle.member.ObjectType == OBJECT_TYPE_FILTER))
    {
#endif /* DEBUG */
        return( (Filter_t *)OBJECT_PTR(FilterHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}

/* ------------------------------------------------------------------------- */

#if defined (STPTI_FRONTEND_HYBRID)
__INLINE Frontend_t *stptiMemGet_Frontend( FullHandle_t FrontendHandle )
{
#ifdef DEBUG
    if ((FrontendHandle.word != STPTI_NullHandle()) &&
        (FrontendHandle.word != 0) &&
        (FrontendHandle.member.ObjectType == OBJECT_TYPE_FRONTEND))
    {
#endif /* DEBUG */
        return( (Frontend_t *)OBJECT_PTR(FrontendHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}
#endif /* #if defined (STPTI_FRONTEND_HYBRID) */
/* ------------------------------------------------------------------------- */


__INLINE Signal_t *stptiMemGet_Signal( FullHandle_t SignalHandle )
{
#ifdef DEBUG
    if ((SignalHandle.word != STPTI_NullHandle()) &&
        (SignalHandle.word != 0) &&
        (SignalHandle.member.ObjectType == OBJECT_TYPE_SIGNAL))
    {
#endif /* DEBUG */
        return( (Signal_t *)OBJECT_PTR(SignalHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}


/* ------------------------------------------------------------------------- */


__INLINE List_t *stptiMemGet_List( FullHandle_t ListHandle )
{
#ifdef DEBUG
    if ((ListHandle.word != STPTI_NullHandle()) &&
        (ListHandle.word != 0) &&
        (ListHandle.member.ObjectType == OBJECT_TYPE_LIST))
    {
#endif /* DEBUG */
        return( OBJECT_LIST_PTR(ListHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}


/* ------------------------------------------------------------------------- */


#ifdef STPTI_CAROUSEL_SUPPORT
__INLINE Carousel_t *stptiMemGet_Carousel( FullHandle_t CarouselHandle )
{
#ifdef DEBUG
    if ((CarouselHandle.word != STPTI_NullHandle()) &&
        (CarouselHandle.word != 0) &&
        (CarouselHandle.member.ObjectType == OBJECT_TYPE_CAROUSEL))
    {
#endif /* DEBUG */
        return( (Carousel_t *)OBJECT_PTR(CarouselHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}
#endif


/* ------------------------------------------------------------------------- */


#if !defined ( STPTI_BSL_SUPPORT )
__INLINE Descrambler_t *stptiMemGet_Descrambler( FullHandle_t DescramblerHandle )
{
#ifdef DEBUG
    if ((DescramblerHandle.word != STPTI_NullHandle()) &&
        (DescramblerHandle.word != 0) &&
        (DescramblerHandle.member.ObjectType == OBJECT_TYPE_DESCRAMBLER))
    {
#endif /* DEBUG */
        return( (Descrambler_t *)OBJECT_PTR(DescramblerHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/* ------------------------------------------------------------------------- */


#ifndef STPTI_NO_INDEX_SUPPORT
__INLINE Index_t *stptiMemGet_Index( FullHandle_t IndexHandle )
{
#ifdef DEBUG
    if ((IndexHandle.word != STPTI_NullHandle()) &&
        (IndexHandle.word != 0) &&
        (IndexHandle.member.ObjectType == OBJECT_TYPE_INDEX))
    {
#endif /* DEBUG */
        return( (Index_t *)OBJECT_PTR(IndexHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}
#endif


/* ------------------------------------------------------------------------- */


__INLINE Session_t *stptiMemGet_Session( FullHandle_t AnyHandle )
{
#ifdef DEBUG
    if ((AnyHandle.word != STPTI_NullHandle()) &&
        (AnyHandle.word != 0))
    {
#endif /* DEBUG */
        return( (Session_t *)SESSION_PTR(AnyHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}


/* ------------------------------------------------------------------------- */


#ifdef STPTI_CAROUSEL_SUPPORT
__INLINE CarouselSimpleEntry_t *stptiMemGet_CarouselSimpleEntry( FullHandle_t SimpleEntryHandle )
{
#ifdef DEBUG
    if ((SimpleEntryHandle.word != STPTI_NullHandle()) &&
        (SimpleEntryHandle.word != 0) &&
        (SimpleEntryHandle.member.ObjectType == OBJECT_TYPE_CAROUSEL_ENTRY))
    {
#endif /* DEBUG */
        return( (CarouselSimpleEntry_t *)OBJECT_PTR(SimpleEntryHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}
#endif


/* ------------------------------------------------------------------------- */


#ifdef STPTI_CAROUSEL_SUPPORT
__INLINE CarouselTimedEntry_t *stptiMemGet_CarouselTimedEntry( FullHandle_t TimedEntryHandle )
{
#ifdef DEBUG
    if ((TimedEntryHandle.word != STPTI_NullHandle()) &&
        (TimedEntryHandle.word != 0) &&
        (TimedEntryHandle.member.ObjectType == OBJECT_TYPE_CAROUSEL_ENTRY))
    {
#endif /* DEBUG */
        return( (CarouselTimedEntry_t *)OBJECT_PTR(TimedEntryHandle) );
#ifdef DEBUG
    }
    else
    {
        BreakPointTime();
        return 0;
    }
#endif /* DEBUG */
}
#endif


/* EOF  -------------------------------------------------------------------- */

