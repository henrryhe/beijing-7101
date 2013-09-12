/*****************************************************************************

File name   : MemoryTrace_utils.c

Description : Memory Trace managment

COPYRIGHT (C) STMicroelectronics 2006.

Date                    Modification                                     Name
----                    ------------                                     ----
26 Ferbruary 2006       Created                                          ellafiha
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "stddefs.h"
#include "sttbx.h"
#include "stos.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#define MAX_TRACE_BUFFER_SIZE                   (200 * 1024)      /* 200 Kbytes */
#define MAX_DUMP_FILE_SIZE                      (2* 1024 * 1024)     /* 2 Mbytes */
#define MAX_CONFIG_BUFFER_SIZE                  1000
#define MAX_MESSAGE_LENGTH                      100
#define MEMORY_TRACE_TASK_STACK_SIZE            5 *1024
#define MEMORY_TRACE__TASK_PRIORITY             0


/* Private Variables (static)------------------------------------------------ */
static char *          MemoryTrace_Buffer_p;
static char *          MemoryTrace_Config_Buffer_p;
static U32             MemoryTrace_start_Index, MemoryTrace_end_Index, MemoryTrace_write_Index,
                       MemoryTrace_Print_write_Index ,MemoryTrace_Print_start_Index, MemoryTrace_Print_end_Index, MemoryTrace_Print_Count,
                       MemoryTrace_Config_start_Index, MemoryTrace_Config_write_Index;
static BOOL            MemoryTrace_On = FALSE;
static semaphore_t*    MemoryTrace_AccessSemaphore_p;
static semaphore_t*    MemoryTrace_PrintSemaphore_p;

static task_t*         MemoryTraceTask_p;
static void*           MemoryTraceTaskStack;
static tdesc_t         MemoryTraceTaskDesc;           /* Interrupt Process task descriptor */

static FILE*           fstream;      /* File Handle for write operation        */
static U32             MaxFileAccessNumber;
static U32             FileAccessCount;


/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
void            MemoryTraceProcess ( void );

/* Local Functions ---------------------------------------------------------------- */
static void CreateFileName (char* FileName, const char *format,...)
{
    va_list list;


    va_start (list, format);
    vsprintf ((char*) FileName , format, list);
}


/* Functions ---------------------------------------------------------------- */
/*******************************************************************************
Name        : MemoryTrace_Init
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void MemoryTrace_Init( ST_Partition_t* AllocationPartition_p )
{
    U32 SelectedSize;

    if ( MemoryTrace_On == FALSE )
    {
        /* Buffer allocation for config messages */
        MemoryTrace_Config_Buffer_p = STOS_MemoryAllocate(AllocationPartition_p, MAX_CONFIG_BUFFER_SIZE);

        if ( MemoryTrace_Config_Buffer_p != NULL )
        {
            /* Look for available memory for buffer allocation */
            MemoryTrace_Buffer_p = NULL;

            SelectedSize = MAX_TRACE_BUFFER_SIZE + 500;

            do
            {
                SelectedSize -= 500;
                MemoryTrace_Buffer_p = STOS_MemoryAllocate(AllocationPartition_p, (size_t)SelectedSize);
            }
            while ( MemoryTrace_Buffer_p == NULL );
        }

        if ( MemoryTrace_Buffer_p == NULL )
        {
            STTBX_Print (("--> Unable to allocate buffer for debug trace\n"));
            STTBX_Print(("--> Memory Trace Init : Error\n"));
        }
        else
        {
            /* Init semaphores */
            MemoryTrace_AccessSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);
            MemoryTrace_PrintSemaphore_p  = STOS_SemaphoreCreateFifo(NULL,0);

            /* Init Index */
            MemoryTrace_On = TRUE;
            MemoryTrace_start_Index = ( U32 ) MemoryTrace_Buffer_p;
            MemoryTrace_end_Index   = ( U32 ) MemoryTrace_start_Index + SelectedSize/2 - MAX_MESSAGE_LENGTH;
            MemoryTrace_write_Index = MemoryTrace_start_Index;

            MemoryTrace_Print_Count = 0;
            MemoryTrace_Print_start_Index = ( U32 ) MemoryTrace_Buffer_p + SelectedSize/2;
            MemoryTrace_Print_end_Index   = ( U32 ) MemoryTrace_Print_start_Index + SelectedSize/2 - MAX_MESSAGE_LENGTH;

            MemoryTrace_Config_start_Index = ( U32 ) MemoryTrace_Config_Buffer_p;
            MemoryTrace_Config_write_Index = MemoryTrace_Config_start_Index;

            MaxFileAccessNumber = MAX_DUMP_FILE_SIZE / SelectedSize;
            FileAccessCount = 0;

            /* Init task */
            STOS_TaskCreate ((void(*)(void*))MemoryTraceProcess,
                                (void*)NULL,
                                AllocationPartition_p,
                                MEMORY_TRACE_TASK_STACK_SIZE,
                                &(MemoryTraceTaskStack),
                                AllocationPartition_p,
                                &(MemoryTraceTask_p),
                                &(MemoryTraceTaskDesc),
                                MEMORY_TRACE__TASK_PRIORITY,
                                "MemoryTraceProcess",
                                0 /*task_flags_no_min_stack_size*/ );

            STTBX_Print(("--> Memory Trace Init : OK\n"));
        }
    }
}

/*******************************************************************************
Name        : MemoryTrace_Write
Description : local function that stores data to print into
                the print buffer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void MemoryTrace_Write (const char *format,...)
{
    va_list         list;
    U32             Aux;

    if ( MemoryTrace_On == TRUE )
    {
        STOS_SemaphoreWait(MemoryTrace_AccessSemaphore_p);

        va_start (list, format);
        MemoryTrace_write_Index += vsprintf ((char*) MemoryTrace_write_Index , format, list);
        if ( MemoryTrace_write_Index > MemoryTrace_end_Index )
        { /* if end of the buffer almost reached, wrap around */
            Aux = MemoryTrace_start_Index;
            MemoryTrace_start_Index = MemoryTrace_Print_start_Index;
            MemoryTrace_Print_start_Index = Aux;

            Aux = MemoryTrace_end_Index;
            MemoryTrace_end_Index = MemoryTrace_Print_end_Index;
            MemoryTrace_Print_end_Index = Aux;

            MemoryTrace_Print_write_Index = MemoryTrace_write_Index;
            MemoryTrace_write_Index = MemoryTrace_start_Index;

            STOS_SemaphoreSignal(MemoryTrace_PrintSemaphore_p);
        }
        va_end (list);

        STOS_SemaphoreSignal(MemoryTrace_AccessSemaphore_p);
    }
}

/*******************************************************************************
Name        : MemoryTrace_Write_Cfg
Description : local function that stores data to print into
                the print buffer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void MemoryTrace_Write_Config (const char *format,...)
{
    va_list         list;

    if ( MemoryTrace_On == TRUE )
    {
        STOS_SemaphoreWait(MemoryTrace_AccessSemaphore_p);

        va_start (list, format);
        MemoryTrace_Config_write_Index += vsprintf ((char*) MemoryTrace_Config_write_Index , format, list);
        va_end (list);

        STOS_SemaphoreSignal(MemoryTrace_AccessSemaphore_p);
    }
}


/*******************************************************************************
Name        : MemoryTrace_DumpMemory
Description : Dump the containing of the memory into dump file
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MemoryTrace_DumpMemory()
{
    FILE*   fstream;      /* File Handle for write operation        */

    if ( MemoryTrace_On == TRUE )
    {
        STOS_SemaphoreWait(MemoryTrace_AccessSemaphore_p);

        /* Open stream */
        fstream = fopen("MEMORY_TRACE_DUMP.txt", "wb");
        if (fstream == NULL)
        {
            return ST_ERROR_BAD_PARAMETER;
        }

        /* Write data in file */
        fwrite((void *)MemoryTrace_Buffer_p, 1, ( MemoryTrace_write_Index - MemoryTrace_start_Index ), fstream);

        /* Close stream */
        fclose (fstream);

        STOS_SemaphoreSignal(MemoryTrace_AccessSemaphore_p);
    }

    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : MemoryTraceProcess
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void            MemoryTraceProcess ( void )
{
    char FileName[30];


    while ( 1 )
    {
        STOS_SemaphoreWait(MemoryTrace_PrintSemaphore_p);

        if (FileAccessCount == 0)
        {
            CreateFileName(FileName, "MEMORY_TRACE_DUMP_%d.txt", MemoryTrace_Print_Count );
            fstream = fopen(FileName, "wb");
            MemoryTrace_Print_Count ++;
        }


        if (fstream != NULL)
        {
            if (FileAccessCount == 0)
            {
                /* Write config data in file */
                fwrite((void *)MemoryTrace_Config_start_Index, 1, ( MemoryTrace_Config_write_Index - MemoryTrace_Config_start_Index ), fstream);
            }

            /* Write data in file */
            fwrite((void *)MemoryTrace_Print_start_Index, 1, ( MemoryTrace_Print_write_Index - MemoryTrace_Print_start_Index ), fstream);
        }


        if (FileAccessCount == MaxFileAccessNumber )
        {
            /* Close stream */
            fclose (fstream);

            FileAccessCount = 0;
        }
        else
        {
            FileAccessCount++;
        }
    }
    /* never exits ( in theory )*/
}


