#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#endif

#include "stevt.h"
#if !defined(ST_OSLINUX)
#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif
#endif
#include "mem_handler.h"
#include "stevtint.h"

#define COLLAPS_RIGHT   1
#define COLLAPS_LEFT    2

#define VALID_MEMHDL 0x87654321

typedef struct _alloc_header_s
{
   struct _alloc_header_s *next;
   U32 block_size;
   U32 port;
   U32 data_size;
} alloc_header_t;



ST_ErrorCode_t stevt_InitMem(U32 mem_addr, U32 mem_size, MemHandle_t *MemHandle)
{
   free_header_t *dummy;
   free_header_t *memPtr;
   ST_ErrorCode_t res = ST_NO_ERROR;

   STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,">>>>InitMem"));

   STEVT_Report((STTBX_REPORT_LEVEL_INFO,"InitMem: Initialized %d bytes of memory",mem_size));
   
   if (mem_size <= sizeof(free_header_t))
   {
       STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"\tInitMem Error: Not enough memory")); 
       return STEVT_ERROR_NO_MORE_SPACE;
   }

   if (MemHandle->ValidHandle == VALID_MEMHDL )
   {
       STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"\tInitMem Error: Memory handler already initialized"));
       return ST_ERROR_ALREADY_INITIALIZED;
   }

   MemHandle->MEM_Address = mem_addr;
   MemHandle->ValidHandle = VALID_MEMHDL;
   MemHandle->AllocatedMemory = sizeof(free_header_t);

   STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tInitMem: Memory Address = 0x%X", 
                 MemHandle->MEM_Address));

   dummy  = (free_header_t *) mem_addr;
   memPtr = (free_header_t *) (mem_addr + sizeof(free_header_t));

   memPtr->next = (free_header_t *)dummy;
   memPtr->size = mem_size - sizeof(free_header_t);

   dummy->next = (free_header_t *)memPtr;
   dummy->size = 0;

   MemHandle->free_head = (free_header_t *)dummy;

   STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"<<<<InitMem"));

   return res;
}

ST_ErrorCode_t stevt_DeinitMem(MemHandle_t *MemHandle)
{
   ST_ErrorCode_t res=ST_NO_ERROR;

   STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,">>>>DeinitMem"));

   if (MemHandle->ValidHandle != VALID_MEMHDL)
   {
       STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"\tDeinitMem Error: Invalid Memory handler"));
       return ST_ERROR_INVALID_HANDLE;
   }
   if (MemHandle->free_head == NULL )
   {
       STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"\tDeinitMem Error: Memory handler not initialized"));
       return ST_ERROR_INVALID_HANDLE;
   }

   MemHandle->free_head = (free_header_t *)NULL;
   MemHandle->MEM_Address = (U32)NULL;
   MemHandle->ValidHandle = 0;
   MemHandle->AllocatedMemory = 0;
   STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"<<<<DeinitMem"));

   return res;
}


void *stevt_AllocMem(U32 size, MemHandle_t *MemHandle)
{
   free_header_t *block, *prev_block, *free_block;
   alloc_header_t *alloc_block;
   U32 found;
   U32 real_size;

   STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,">>>>AllocMem"));

   if (MemHandle->ValidHandle != VALID_MEMHDL)
   {
       STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"\tDeinitMem Error: Invalid Memory handler"));
       return NULL;
   }
   if (MemHandle->free_head == NULL )
   {
       STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"\tAllocMem Error: Memory handler not initialized"));
       return NULL;
   }
   
   found = 0;
   alloc_block = NULL;

   /* free_head points to the dummy block... */
   block = (free_header_t *)MemHandle->free_head->next ;
   prev_block = (free_header_t *)MemHandle->free_head;

   real_size = size + sizeof(alloc_header_t);

   if (real_size % sizeof(alloc_header_t))
   {
      real_size += (sizeof(alloc_header_t) - 
                  (real_size % sizeof(alloc_header_t)));
   }

   STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: obj size  = %d",size));
   STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: real size = %d",real_size));

   do
   {
      STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: block->size >= real_size"));
      if (block->size >= real_size)
      {
         STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: found a block!"));
         found = 1;

         if ((block->size - real_size) <= (sizeof(alloc_header_t) * 2))
         {
            /* allocate the whole block */
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: allocating the whole block."));

            prev_block->next = block->next;
            real_size = block->size;
         }
         else
         {
            /* divide the block */
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: dividing the block."));

            free_block = (free_header_t *)
                        (((U32) block) + real_size);
            free_block->next = block->next;
            free_block->size = block->size - real_size;
            prev_block->next = (free_header_t *)free_block;
         }

         alloc_block = (alloc_header_t *) block;
         alloc_block->block_size = real_size;
         alloc_block->next = NULL;
         STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: block = 0x%08X",
                        (U32) alloc_block));
      }
      else
      {
         STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: not found a block!"));
         prev_block = block;
         block = (free_header_t *)block->next;
      }

   } while ((!found) && (block != (free_header_t *)MemHandle->free_head));

   if (found)
   {
      STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: block = 0x%08X",
                     (U32) alloc_block));
      MemHandle->AllocatedMemory += real_size;
      alloc_block++;
   }


   STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: Available memory at 0x%08X",
                     (U32) alloc_block));
   STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tAllocMem: Max allocation %d Bytes",
                 MemHandle->AllocatedMemory));
   STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"<<<<AllocMem"));
   return alloc_block;
}


void stevt_FreeMem(void *block, MemHandle_t *MemHandle)
{
   alloc_header_t *alloc_block;
   free_header_t *free_block,
                 *current_block,
                 *next_addr;
   int collaps, size;

   STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,">>>>FreeMem"));

   if (MemHandle->ValidHandle != VALID_MEMHDL)
   {
       STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"\tDeinitMem Error: Invalid Memory handler"));
       return ;
   }
   alloc_block = (alloc_header_t *) block;
   alloc_block--;

   size = alloc_block->block_size;
   free_block = (free_header_t *) alloc_block;
   free_block->size = size;

   MemHandle->AllocatedMemory -= size;
   current_block = (free_header_t *)MemHandle->free_head;

   STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tFreeMem: looking for current free block"));

   while ((current_block->next != MemHandle->free_head) &&
          ((free_header_t *)free_block) > current_block->next)
   {
      current_block = (free_header_t *)current_block->next;
   }

   collaps = 0;
   next_addr = (free_header_t *)
               ((U32) free_block + free_block->size);
   if (next_addr == (free_header_t *)current_block->next)
   {
      /* can collaps free_block and current_block->next (right) */
      collaps |= COLLAPS_RIGHT;
   }

   next_addr = (free_header_t *)
               ((U32) current_block + current_block->size);
   if (next_addr == free_block)
   {
      /* can collaps current_block and free_block (left)*/
      collaps |= COLLAPS_LEFT;
   }

   switch (collaps)
   {
      case COLLAPS_RIGHT:
         free_block->size += ((free_header_t *)current_block->next) -> size;
         free_block->next =
                     ((free_header_t *)current_block->next) -> next;
         current_block->next = (free_header_t *)free_block;
         STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tFreeMem: collaps right"));
         break;

      case COLLAPS_LEFT:
         current_block->size += free_block->size;
         STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tFreeMem: collaps left"));
         break;

      case COLLAPS_RIGHT | COLLAPS_LEFT:
         current_block->size += free_block->size +
                                ((free_header_t *)current_block->next) -> size;
         current_block->next =
                     ((free_header_t *)current_block->next) -> next;
         STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tFreeMem: collaps both"));
         break;

      default:
         free_block->next = current_block->next;
         current_block->next = (free_header_t *)free_block;
         STEVT_Report((STTBX_REPORT_LEVEL_INFO,"\tFreeMem: add block"));
         break;
   }

   STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"<<<<FreeMem"));
}

