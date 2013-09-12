/*****************************************************************************
File Name   : inject_swts.h

Description : SWTS header file.

History     :

Copyright (C) 2003 STMicroelectronics

Reference   :
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STINJECTSWTS_H
#define __STINJECTSWTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */
#include "stmergetst.h"

/* wrapper macros */
#ifdef ST_OS21

#define DEBUGOPEN(_filename_, _format_)     	fopen(_filename_, _format_)
#define DEBUGREAD(_file_, _buffer_, _size_) 	fread(_buffer_, 1,_size_, _file_)
#define DEBUGCLOSE(_file_); 	    	    	fclose(_file_)
#define FILE_TYPE   	    	    	    	FILE *
#define FILE_EXISTING_COMPARE	    	    	== NULL

#else

#define DEBUGOPEN(_filename_, _format_)     	debugopen(_filename_, _format_)
#define DEBUGREAD(_file_, _buffer_, _size_) 	debugread(_file_, _buffer_, _size_)
#define DEBUGCLOSE(_file_)  	    	    	debugclose(_file_)
#define FILE_TYPE   	    	    	    	long
#define FILE_EXISTING_COMPARE	    	    	< 0
#endif

/* Task control struct */
typedef struct stmergefn_Task_s
{
    U8          TaskStack[2048];
#ifndef ST_OS21
    task_t      TaskID;
    tdesc_t     TaskDesc;
#endif
}stmergefn_Task_t;

typedef struct stmergefn_SWTS_s
{
    U32         SWTSWord;
}stmergefn_SWTS_t;

/* SWTS write control data */
typedef struct stmergefn_SWTSWriteControlBlock_s
{
    stmergefn_Task_t TaskData ;           /* Task realted control data  */
    partition_t *Partition_p ;            /* Mem partition */
    U8 *Filename_p ;                      /* Name of file to write */
    U32 PIDVal ;                          /* PID to look for */
    stmergefn_SWTS_t *SWTSValues_p ;      /* Pointer to SWTS data array */
    U32 WordCount ;                       /* Number of byte to read */
    STMERGE_ObjectId_t SWTSObject ;       /* Object ID for SWTS */
} stmergefn_SWTSWriteControlBlock_t ;

#if defined SWTS_PID
    #undef SWTS_PID
#endif

#define DTV_PACKET_SIZE 130
#define DVB_PACKET_SIZE 188

#if defined(STMERGE_DTV_PACKET)

    /* The stream is allslot1.tps. For this the SCID is 0x10 */
    #define SWTS_PID 0x10

    #define BYTES_TO_READ DTV_PACKET_SIZE
    #define NUMBER_OF_WORDS_TO_WRITE 33
#else
    /* The stream is canal10.trp .For this the PID is 0x3C */
    #define SWTS_PID 0x3C

    /* The stream is multi2.ts. For this the PID is 0x10A4 */
    /* #define SWTS_PID 0x10A4 */

    #define BYTES_TO_READ DVB_PACKET_SIZE
    #define NUMBER_OF_WORDS_TO_WRITE 47
#endif

#if defined(ST_OSLINUX)
#define MAX_PTI 3
struct Inject_StatusParams
{
    U16 NoInterfacesSet;
    ST_DeviceName_t DeviceName[MAX_PTI];
    ST_DeviceName_t InputInterface[MAX_PTI];
    STPTI_Slot_t SlotHandle[MAX_PTI];
};
#endif
semaphore_t     *InjectSema_p ;
semaphore_t     *SynchroniseInput_p;

/* Local functions */
void InjectStream (STMERGE_ObjectId_t SWTS_Object );
void PlayFileBySWTSWrite(void *SWTSDataHandle_p);
void PlayFileBySWTSDMA (void *SWTSDataHandle_p ) ;
void stmerge_PlayFileToSWTSTask ( void* SWTSDataHandle_p );

#ifdef __cplusplus
}
#endif

#endif
