/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005-2007.  All rights reserved.

  File Name: handle.h
Description: stpti internal handle

******************************************************************************/

#ifndef __HANDLE_H
 #define __HANDLE_H


/* Includes ------------------------------------------------------------ */


#include "stddefs.h"


/* --------------------------------------------------------------------- */


typedef volatile union FullHandle_u 
{
    struct
    {
        unsigned int Object:14;     /* 16384 objects, e.g. 2048 per session */
        unsigned int ObjectType:6;  /*    64 types of data objects          */
        unsigned int Session:8;     /*   256 sessions per PTI cell          */
        unsigned int Device:4;      /*    16 PTI cells                      */
    } member;

    U32 word;

} FullHandle_t;


/* --------------------------------------------------------------------- */


typedef volatile enum ObjectType_e 
{
    OBJECT_TYPE_BUFFER = 1,     /* Public */
    OBJECT_TYPE_DESCRAMBLER,
    OBJECT_TYPE_FILTER,
    OBJECT_TYPE_SESSION,
    OBJECT_TYPE_SIGNAL,
    OBJECT_TYPE_SLOT,
#ifdef STPTI_CAROUSEL_SUPPORT
    OBJECT_TYPE_CAROUSEL,
    OBJECT_TYPE_CAROUSEL_ENTRY,
#endif
#ifndef STPTI_NO_INDEX_SUPPORT
    OBJECT_TYPE_INDEX,
#endif

    OBJECT_TYPE_DEVICE,     /*  Private (not exposed in the memory manager api) */
    OBJECT_TYPE_DRIVER,
    OBJECT_TYPE_FRONTEND,
    OBJECT_TYPE_LIST        /* This must be the last item in the enum */
} ObjectType_t;


/* --------------------------------------------------------------------- */


#endif  /* __HANDLE_H */
