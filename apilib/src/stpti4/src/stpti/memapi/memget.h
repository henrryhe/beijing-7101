/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005-2007.  All rights reserved.

   File Name: memget.h
 Description: function wrappers for memory access macros

******************************************************************************/


#ifndef __MEMGET_H
 #define __MEMGET_H


/* Includes ------------------------------------------------------------ */


#include "stddefs.h"

#include "pti_hndl.h"
#include "pti_list.h"
#include "pti_loc.h"


#ifdef STPTI_NOINLINE_MEMGET
 #define __INLINE
#else
 #define __INLINE __inline
#endif


/* ------------------------------------------------------------------------- */


__INLINE Device_t        *stptiMemGet_Device     ( FullHandle_t AnyHandle         );
__INLINE Slot_t          *stptiMemGet_Slot       ( FullHandle_t SlotHandle        );
__INLINE Buffer_t        *stptiMemGet_Buffer     ( FullHandle_t BufferHandle      );
__INLINE Filter_t        *stptiMemGet_Filter     ( FullHandle_t FilterHandle      );
#if defined (STPTI_FRONTEND_HYBRID)
__INLINE Frontend_t      *stptiMemGet_Frontend   ( FullHandle_t FrontendHandle    );
#endif
__INLINE List_t          *stptiMemGet_List       ( FullHandle_t ListHandle        );
__INLINE Signal_t        *stptiMemGet_Signal     ( FullHandle_t SignalHandle      );
__INLINE Descrambler_t   *stptiMemGet_Descrambler( FullHandle_t DescramblerHandle );
__INLINE TCPrivateData_t *stptiMemGet_PrivateData( FullHandle_t AnyHandle         );
#ifdef STPTI_CAROUSEL_SUPPORT
__INLINE Carousel_t      *stptiMemGet_Carousel   ( FullHandle_t CarouselHandle    );
__INLINE CarouselSimpleEntry_t *stptiMemGet_CarouselSimpleEntry( FullHandle_t SimpleEntryHandle );
__INLINE CarouselTimedEntry_t *stptiMemGet_CarouselTimedEntry( FullHandle_t TimedEntryHandle );
#endif
#ifndef STPTI_NO_INDEX_SUPPORT
__INLINE Index_t         *stptiMemGet_Index      ( FullHandle_t IndexHandle       );
#endif
__INLINE Session_t       *stptiMemGet_Session    ( FullHandle_t AnyHandle         );


/* ------------------------------------------------------------------------- */


#endif  /* __MEMGET_H */


/* EOF --------------------------------------------------------------------- */
