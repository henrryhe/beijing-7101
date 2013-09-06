/******************************************************************************\
 *
 * File Name   : objcache.h
 *
 * Description : Definition of the Object Cache
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
\******************************************************************************/

#ifndef __OBJCACHE_H
#define __OBJCACHE_H

#include <stddefs.h>

#define OBJECT_CACHE_NO_MEMORY 2   /* must be 2 */

/* ------------------------------------- */
/* --- Defines an entry in the index --- */
/* ------------------------------------- */

typedef struct {
  U16   ObjectId;
  U16   ObjectSize;
  U8   *ObjectSegment_p;
} CacheIndexEntry_t;


/* ---------------------------------------------- */
/* --- Defines an Object Cache data structure --- */
/* ---------------------------------------------- */

typedef struct ObjectCache_s {
  U8   *Base_p;
  U32 	CacheBufferSize;
  U32 	NumBytesStored;
  U32 	NumObjectCached;
  U8   *Free_p;
  BOOL 	DataInvalidated;
  CacheIndexEntry_t *Index_p;
  ST_Partition_t    *MemoryPartition;
} ObjectCache_t ;


/* ---------------------------------------- */
/* --- Object Cache: Exported Functions --- */
/* ---------------------------------------- */

/* Create/Delete the Object Cache */

ObjectCache_t * ObjectCache_Create (ST_Partition_t *MemoryPartition,
                                    U32 CacheBufferSize);

/* Delete the Object Cache */

void ObjectCache_Delete (ObjectCache_t *cache);

/* Store an object data segment into the Object Cache */

ST_ErrorCode_t ObjectCache_StoreObject (ObjectCache_t *cache, 
                           U16   ObjectId,
                           U16   ObjectSize,
                           void *ObjectDataSegment);

/* Get a pointer to an object data segment into the Object Cache */

void *ObjectCache_GetObject (ObjectCache_t *cache, 
                             U16 ObjectId, U16 *ObjectSize);

/* Purge the Object Cache contents */

void ObjectCache_Flush (ObjectCache_t *cache);

#endif  /* #ifndef __OBJCACHE_H */

