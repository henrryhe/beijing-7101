/******************************************************************************\
 *
 * File Name   : mem_handler.h
 *  
 * Description : Internal memory handler
 *
 * Copyright STMicroelectronics - 2000
 *  
\******************************************************************************/

#ifndef __MEM_HANDLER_H
#define __MEM_HANDLER_H

typedef struct _free_header_s
{
   struct _free_header_s *next;
   U32 size;
   U32 dummy1;
   U32 dummy2;
} free_header_t;


typedef struct _MemHandle_s
{   
    U32            MEM_Address;
    free_header_t *free_head;
    U32            ValidHandle;
    U32            AllocatedMemory;
} MemHandle_t;

ST_ErrorCode_t stevt_InitMem(U32 mem_addr, U32 mem_size, MemHandle_t *MemHandle);
ST_ErrorCode_t stevt_DeinitMem(MemHandle_t *MemHandle);
void *stevt_AllocMem(unsigned int size, MemHandle_t *MemHandle);
void stevt_FreeMem(void *block, MemHandle_t *MemHandle);
#endif

