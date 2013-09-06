/******************************************************************************\
 * File Name    : objcache.c
 *
 * Description  : 
 *     The object cache is a buffer which contains object data segments: 
 *     it allows the decoder to supports the DVB object data segment caching 
 *     mechanism as defined in [EN 50221]. 
 *     When a overlay mode session is opened, no data is present in the buffer, 
 *     and when the section is closed the contents of the cache are purge. 
 *     A flush_download() APDU also allows the cache to be purge. 
 *     When a region references an object in the cache the corresponding object 
 *     data segment is read from the cache and then processed. Any object data
 *     segment can be stored in the cache by means of subtitle_download() APDUs.
 *
 * Author : Marino Congiu - Nov 1999
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
\******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stddefs.h>
#include <sttbx.h>
#include "objcache.h"


/******************************************************************************\
 * Function: ObjectCache_Create
 * Purpose : Create a object cache of suitable size
 * Parameters:
 *      MemoryPartition: 
 *           Partition from which memory is allocated.
 *      CacheBufferSize: 
 *           Required size of buffer in bytes.
 * Return  : A pointer to created object cache data structure; 
 *           NULL in case of error
\******************************************************************************/

ObjectCache_t * ObjectCache_Create (ST_Partition_t *MemoryPartition,
                                    U32 CacheBufferSize)
{
    ObjectCache_t *Cache_p;

    /* Alloc memory for buffer and initialize structure  */

    if ((Cache_p = (ObjectCache_t *) 
        memory_allocate(MemoryPartition, sizeof(ObjectCache_t))) == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** ObjectCache_Create: no memory **\n"));
        return (NULL);
    }

    /* Alloc memory buffer */

    if ((Cache_p->Base_p = 
        memory_allocate(MemoryPartition, CacheBufferSize))  == NULL)   
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** ObjectCache_Create: no memory **\n"));
        memory_deallocate(MemoryPartition, Cache_p);
        return (NULL);
    }

    /* fill in control data structure */

    Cache_p->MemoryPartition = MemoryPartition;
    Cache_p->CacheBufferSize = CacheBufferSize;
    Cache_p->NumBytesStored  = 0;
    Cache_p->DataInvalidated = FALSE;
    Cache_p->Free_p  = Cache_p->Base_p;
    Cache_p->Index_p = (CacheIndexEntry_t*)(Cache_p->Base_p + CacheBufferSize);

    return (Cache_p);                                           
}


/******************************************************************************\
 * Function: ObjectCache_Delete
 * Purpose : Delete the object cache; 
 *           All allocated resources are released.
\******************************************************************************/

void ObjectCache_Delete(ObjectCache_t *Cache_p)
{
    ST_Partition_t *MemoryPartition = Cache_p->MemoryPartition;
    memory_deallocate (MemoryPartition, Cache_p->Base_p);
    memory_deallocate (MemoryPartition, Cache_p);
}


/******************************************************************************\
 * Function: ObjectCache_Flush
 * Purpose : Purge the Object Cache contents
\******************************************************************************/
 
void ObjectCache_Flush (ObjectCache_t *Cache_p)
{
    Cache_p->NumBytesStored  = 0;
    Cache_p->DataInvalidated = FALSE;
    Cache_p->Free_p  = Cache_p->Base_p;
    Cache_p->Index_p = 
        (CacheIndexEntry_t*)(Cache_p->Base_p + Cache_p->CacheBufferSize);
}


/******************************************************************************\
 * Function: ObjectCache_StoreObject
 * Purpose : Stores a object data segment in the cache.
 * Return  : ST_NO_ERROR on success
\******************************************************************************/

ST_ErrorCode_t ObjectCache_StoreObject (ObjectCache_t *Cache_p, 
                           U16   ObjectId,
                           U16   ObjectDataSize,
                           void *ObjectDataSegment)
{
  CacheIndexEntry_t *IndexEntry;
  U8 * ODS_p;
 
  /* check if there is enough space in cache */
 
  if ( (Cache_p->NumBytesStored + ObjectDataSize + sizeof(CacheIndexEntry_t)) 
     <= Cache_p->CacheBufferSize)
  {
      /* allocate ODS segment starting from the first free byte in cache */
 
      ODS_p = Cache_p->Free_p;
      memcpy(ODS_p, ObjectDataSegment, ObjectDataSize);
 
      /* allocate index entry */
 
      IndexEntry = Cache_p->Index_p - 1;
      IndexEntry->ObjectId = ObjectId;
      IndexEntry->ObjectSize = ObjectDataSize;
      IndexEntry->ObjectSegment_p = ODS_p;
 
      /* update control information */
 
      Cache_p->Index_p -= 1;
      Cache_p->Free_p += ObjectDataSize;
      Cache_p->NumObjectCached += 1;
      Cache_p->NumBytesStored += (ObjectDataSize + sizeof(CacheIndexEntry_t));
 
      return (ST_NO_ERROR);
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** ObjectCache_StoreObject: no more space **\n"));
      return (OBJECT_CACHE_NO_MEMORY);
  }
}

/******************************************************************************\
 * Function: ObjectCache_GetObject
 * Purpose : Get a pointer to an object data segment in the Object Cache
 * Return  : a pointer to object data segment found
 *           NULL if not found
\******************************************************************************/

void *ObjectCache_GetObject (ObjectCache_t *Cache_p, 
                             U16 ObjectId, U16 *ObjectSize)
{
  CacheIndexEntry_t IndexEntry;
  U32  i;

  /* --- Retrieve specified ODS segment --- */

  for (i = 0; i < Cache_p->NumObjectCached; i++)
  {
      IndexEntry = Cache_p->Index_p[i];
      if (IndexEntry.ObjectId == ObjectId)
      {
          *ObjectSize = IndexEntry.ObjectSize;
          return ((void*)IndexEntry.ObjectSegment_p);
      }
  }

  /* --- Specified ODS segment not found --- */

  STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"ObjectCache_GetObject: Object not found\n"));
  *ObjectSize = 0;
  return (NULL);
}

