/******************************************************************************
*	COPYRIGHT (C) STMicroelectronics 1998
*
*   File name   : BLT_debug.c
*   Description : Debug macros
*
*	Date          Modification                                    Initials
*	----          ------------                                    --------
*   06 Sept 2000   Creation                                        TM
*
******************************************************************************/

/*###########################################################################*/
/*############################## INCLUDES FILE ##############################*/
/*###########################################################################*/

#include <stdio.h>
#include <string.h>

#ifdef ST_OS21
    #include "os21debug.h"
    #include <sys/stat.h>
    #include <fcntl.h>
	#include "stlite.h"
#endif /*End of ST_OS21*/

#ifdef ST_OS20
	#include <debug.h>  /* used to read a file */
	#include "stlite.h"
#endif /*End of ST_OS20*/

#include "stddefs.h"
#include "stdevice.h"
#include "testtool.h"
#include "stos.h"
#ifdef ST_OSLINUX
#include "iocstapigat.h"
#else
#include "sttbx.h"
#endif
#include "startup.h"
#include "stblit.h"
#include "stcommon.h"
#include "stevt.h"
#ifdef ST_OSLINUX
#ifdef ST_ZEUS_PTV_MEM
#include "./linux/stblit_ioctl/stblit_ioctl_ptv.h"
#else
#include "stlayer.h"
#endif

#else

#include "stavmem.h"

#endif

#include "stsys.h"
#include "clevt.h"
#include <float.h>
#include <math.h>

#ifndef STBLIT_EMULATOR
#include "clintmr.h"
#endif

#include "blt_cmd.h"
#include "blt_debug.h"


/*###########################################################################*/
/*############################### DEFINITION ################################*/
/*###########################################################################*/

/* --- Constants --- */
#define GAMMA_FILE_RGB888    0x81
#define GAMMA_FILE_ALPHA1    0x98
#define GAMMA_FILE_ALPHA8    0x99
#define GAMMA_FILE_ACLUT2    0x89
#define GAMMA_FILE_ACLUT4    0x8A
#define GAMMA_FILE_ACLUT8    0x8B
#define GAMMA_FILE_RGB565    0x80
#define GAMMA_FILE_ARGB8565  0x84
#define GAMMA_FILE_ARGB8888  0x85
#define GAMMA_FILE_ARGB1555  0x86
#define GAMMA_FILE_ARGB4444  0x87
#define GAMMA_FILE_YCBCR422R 0x92
#define GAMMA_FILE_YCBCR420MB 0x94


/* Crc related */
#define CRC_TABLE_DEPTH 16
#define CRC_TABLE_SIZE  65536  /* 2 exp CRC_TABLE_DEPTH */
#define CRC_INDEX_MASK  0xFFFF /* CRC_TABLE_SIZE - 1 */
#define POLYNOM_VALUE   0x04c11db7
#define POLYNOM_DEGREE  8


/* --- Extern --- */
extern char Task1Tag[10];
extern char Task2Tag[10];
extern char Task3Tag[10];
extern char Task4Tag[10];


/* --- Prototypes --- */

/* --- Global variables --- */
semaphore_t* JobThreadCompletionTimeout;
semaphore_t* BlitThreadCompletionTimeout;
semaphore_t* Start;

#ifdef ST_OSLINUX
#define BENCHMARK_TASK_STACK_SIZE  1024 * 16

    pthread_t*   BenchmarkTask;
#endif

#ifdef ST_OS21
#define BENCHMARK_TASK_STACK_SIZE  1024 * 16

    task_t*   BenchmarkTask;
#endif
#ifdef ST_OS20
#define BENCHMARK_TASK_STACK_SIZE  1024 * 10

	task_t   BenchmarkTask;
	tdesc_t  BenchmarkTaskDesc;
#endif
int      StackBenchmarkTask[BENCHMARK_TASK_STACK_SIZE/ sizeof(int)];

/* --- Externals --- */
extern U8                           BlitterRegister[];
#ifndef ST_OSLINUX
extern STAVMEM_PartitionHandle_t    AvmemPartitionHandle[];
#endif
extern ST_Partition_t*              SystemPartition_p;


#ifdef ST_OSLINUX
STBLIT_Handle_t FakeBlitHndl = 0x123 ;
#endif

#ifdef ST_OSLINUX
STLAYER_Handle_t FakeLayerHndl = 0x123 ;
#endif

#ifndef ST_OSLINUX
STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
#endif


/* --- Private variables --- */
static U32 crc32_table[CRC_TABLE_SIZE];

/* --- Private functions prototypes---*/


/* --- Private types --- */
static Allocate_KeepInfo_t *Allocate_KeepInfo = NULL;
static Allocate_Number_t Allocate_Number = 0;


/* Interruption related */
U32 InterruptCount = 0;
U32 PrintInterruptCount = FALSE;

typedef struct
{
    STBLIT_Handle_t          Handle;
    STBLIT_Source_t*         Src1_p;
    STBLIT_Source_t*         Src2_p;
    STBLIT_Destination_t*    Dst_p;
    STBLIT_BlitContext_t*    Context_p;
} BenchmarkData_t;

#ifdef ST_OSLINUX



extern MapParams_t   BlitterMap;
#define LINUX_BLITTER_BASE_ADDRESS    BlitterMap.MappedBaseAddress
#endif

/* --- Extern --- */
extern char            SnakeDemoTag[];
extern semaphore_t*    SnakeDemo_BlitCompletionTimeoutSemaphore;
extern char            BlitDemoTagString[];
extern semaphore_t*    DemoBlitCompletionSemaphore_p;
extern char            RandomFillDemoTagString[];
extern semaphore_t*    RandomFillDemoCompletionSemaphore_p;
extern char            RandomCopyDemoTagString[];
extern semaphore_t*    RandomCopyDemoCompletionSemaphore_p;




/* --- Functions --- */
#if defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
/*-----------------------------------------------------------------------------
 * Function : Blit_LoadReferenceFile
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t Blit_LoadReferenceFile(char* filename, U32* CRCValuesTab)
{
    ST_ErrorCode_t              ErrCode;         /* Error management */
    FILE*                       fstream;        /* File handle for read operation          */
    U32                         i = 0;
    U32                         U32Value = 0;

    ErrCode = ST_NO_ERROR;

    /* Check parameter */
    if (filename == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error: Null Pointer, bad parameter\n"));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

    /* Open file handle */
    if (ErrCode == ST_NO_ERROR)
    {
        fstream = fopen(filename, "rb");
        if( fstream == NULL )
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to open \'%s\'\n", filename ));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Reading file data */
    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Print(("Reading reference file... \n"));

        while(U32Value != DEFAULT_EOF_VALUE)
        {
             fread((void *)&U32Value, 1, 4, fstream);

             CRCValuesTab[i++] = U32Value;
             /*STTBX_Print(("=> CRCValuesTab[%d]=%x\n",i,CRCValuesTab[i]));*/
        }
    }

    /* Close file handle */
    fclose (fstream);

    return (ErrCode);
}


/*-----------------------------------------------------------------------------
 * Function : Blit_InitCrc32table
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void Blit_InitCrc32table(void)
{
    U32 i, j;
    U32 crc_precalc;

    for (i = 0; i < CRC_TABLE_SIZE; i++)
    {
        crc_precalc = i << (32 - CRC_TABLE_DEPTH);
        for (j = 0; j < CRC_TABLE_DEPTH; j++)
        {
            if ((crc_precalc & 0x80000000) == 0)
            {
                crc_precalc = (crc_precalc << 1);
            }
            else
            {
                crc_precalc = ((crc_precalc << 1) ^  POLYNOM_VALUE);
            }
        }
        crc32_table[i] = crc_precalc;
    }
}


/*-----------------------------------------------------------------------------
 * Function : Blit_InitCrc32table
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
U32 GetBitmapCRC(STGXOBJ_Bitmap_t*        Bitmap_p)
{
    U32 Count, Nb32bits, CRCValue;
    volatile U32  U32Value;
    volatile U16* U16Value_p;

    /* Init CRCValue */
    CRCValue = 0xFFFFFFFF;

    /* Calculate nb 32bit value */
    Nb32bits = (Bitmap_p->Size1 / 4);

#if 0
    for (Count = 0 ; Count < CRC_TABLE_SIZE ; Count++)
    {
        STTBX_Print(("-> Table(%i)=0x%x\n", Count, crc32_table[Count]));
    }
#endif

    U16Value_p = (U16*) (&U32Value);

    for (Count = 0 ; Count < Nb32bits ; Count++)
    {
        /* Read 32bit value into two 16bits variables */
        U32Value = STSYS_ReadRegMem32LE(((U32)(Bitmap_p->Data1_p)) + Count*4);

        CRCValue = (CRCValue << POLYNOM_DEGREE) ^ crc32_table[(CRCValue >> (32 - CRC_TABLE_DEPTH)) ^ ( U16Value_p[0] & CRC_INDEX_MASK)];
        CRCValue = (CRCValue << POLYNOM_DEGREE) ^ crc32_table[(CRCValue >> (32 - CRC_TABLE_DEPTH)) ^ ( U16Value_p[1] & CRC_INDEX_MASK)];

    }

    return (CRCValue);
}

/*-----------------------------------------------------------------------------
 * Function : TaskCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void TaskCompletedHandler (STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event,
                           const void*            EventData_p,
                           const void*            SubscriberData_p)
{
    char* TaskTag = *((char**)EventData_p);

    if (strcmp(TaskTag, Task1Tag) == 0)
    {
        STOS_SemaphoreSignal(Task1CompletionSemaphore_p);
    }

    if (strcmp(TaskTag, Task2Tag) == 0)
    {
        STOS_SemaphoreSignal(Task2CompletionSemaphore_p);
    }

    if (strcmp(TaskTag, Task3Tag) == 0)
    {
        STOS_SemaphoreSignal(Task3CompletionSemaphore_p);
    }

    if (strcmp(TaskTag, Task4Tag) == 0)
    {
        STOS_SemaphoreSignal(Task4CompletionSemaphore_p);
    }
}
#endif /* defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109) */


/*-----------------------------------------------------------------------------
 * Function : BlitCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void NodeBlitCompletedHandler (STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event,
                           const void*            EventData_p,
                           const void*            SubscriberData_p)
{

    STOS_SemaphoreSignal(BlitCompletionSemaphore_p);
}

typedef void (*STBLIT_BlitCompleteCallback_t)(U32);

/*-----------------------------------------------------------------------------
 * Function : BlitCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void BlitCompletedHandler (STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event,
                           const void*            EventData_p,
                           const void*            SubscriberData_p)
{
    if (PrintInterruptCount)
    {
        STTBX_Print(("\n===> Blit Completion(%d)",++InterruptCount));
    }

#ifdef TEST_CALLBACK
    (*(STBLIT_BlitCompleteCallback_t*)(EventData_p))((U32)10);
#endif

#ifdef STBLIT_DEBUG
#ifdef ST_OSLINUX
    STTBX_Print(("\nBlitCompletedHandler (): signal BlitCompletion..... \n"));
#endif
#endif
    STOS_SemaphoreSignal(BlitCompletionSemaphore_p);
}


/*-----------------------------------------------------------------------------
 * Function : JobCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void JobCompletedHandler (STEVT_CallReason_t     Reason,
                          const ST_DeviceName_t  RegistrantName,
                          STEVT_EventConstant_t  Event,
                          const void*            EventData_p,
                          const void*            SubscriberData_p)
{
    STOS_SemaphoreSignal(JobCompletionSemaphore_p);
}

/*-----------------------------------------------------------------------------
 * Function : JobThreadCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void JobThreadCompletedHandler (STEVT_CallReason_t     Reason,
                          const ST_DeviceName_t  RegistrantName,
                          STEVT_EventConstant_t  Event,
                          const void*            EventData_p,
                          const void*            SubscriberData_p)
{
    STOS_SemaphoreSignal(JobThreadCompletionTimeout);
}

/*-----------------------------------------------------------------------------
 * Function : BlitThreadCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void BlitThreadCompletedHandler (STEVT_CallReason_t     Reason,
                          const ST_DeviceName_t  RegistrantName,
                          STEVT_EventConstant_t  Event,
                          const void*            EventData_p,
                          const void*            SubscriberData_p)
{
    STOS_SemaphoreSignal(BlitThreadCompletionTimeout);
}

/*-----------------------------------------------------------------------------
 * Function : BlitDemoCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void BlitDemoCompletedHandler (STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event,
                           const void*            EventData_p,
                           const void*            SubscriberData_p)
{
    char* TaskTag = *((char**)EventData_p);

    if (strcmp(TaskTag, SnakeDemoTag) == 0)
    {
		STOS_SemaphoreSignal(SnakeDemo_BlitCompletionTimeoutSemaphore);
	}

	if (strcmp(TaskTag, BlitDemoTagString) == 0)
    {
		STOS_SemaphoreSignal(DemoBlitCompletionSemaphore_p);
	}

    if (strcmp(TaskTag, RandomFillDemoTagString) == 0)
    {
		STOS_SemaphoreSignal(RandomFillDemoCompletionSemaphore_p);
	}

    if (strcmp(TaskTag, RandomCopyDemoTagString) == 0)
    {
		STOS_SemaphoreSignal(RandomCopyDemoCompletionSemaphore_p);
	}
}


#ifdef ST_OSLINUX
/*-----------------------------------------------------------------------------
 * Function : AllocBitmapBuffer
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/

ST_ErrorCode_t AllocBitmapBuffer (STGXOBJ_Bitmap_t* Bitmap_p, BOOL SecuredData, void* SharedMemoryBaseAddress_p,
                        STGXOBJ_BitmapAllocParams_t* Params1_p,
                        STGXOBJ_BitmapAllocParams_t* Params2_p)
{
    STLAYER_AllocDataParams_t    AllocParams;
    ST_ErrorCode_t              Err;

    AllocParams.Size                     = Params1_p->AllocBlockParams.Size;
    AllocParams.Alignment                = Params1_p->AllocBlockParams.Alignment;
#ifndef PTV
    if (SecuredData == TRUE)
    {
        Err = STLAYER_AllocDataSecure( FakeLayerHndl, &AllocParams, (void**)&(Bitmap_p->Data1_p) );
    }
    else /* !SecuredData */
#endif
    {
        Err = STLAYER_AllocData( FakeLayerHndl, &AllocParams, (void**)&(Bitmap_p->Data1_p) );
    }

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }

    Bitmap_p->Pitch =  Params1_p->Pitch;
    Bitmap_p->Size1  =  Params1_p->AllocBlockParams.Size;
    Bitmap_p->Offset = 0;
    Bitmap_p->BigNotLittle = 0;

    return(Err);


}

#else
/*-----------------------------------------------------------------------------
 * Function : AllocBitmapBuffer
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/

ST_ErrorCode_t AllocBitmapBuffer (STGXOBJ_Bitmap_t* Bitmap_p , void* SharedMemoryBaseAddress_p, STAVMEM_PartitionHandle_t AVMEMPartition,
                        STAVMEM_BlockHandle_t*  BlockHandle1_p,
                        STGXOBJ_BitmapAllocParams_t* Params1_p, STAVMEM_BlockHandle_t*  BlockHandle2_p,
                        STGXOBJ_BitmapAllocParams_t* Params2_p)
{

    STAVMEM_MemoryRange_t       RangeArea[2];
    STAVMEM_AllocBlockParams_t  AllocBlockParams;
    ST_ErrorCode_t              Err;
    U8                          NbForbiddenRange;

    /* Data1 related */
    NbForbiddenRange = 1;
    if (VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
         VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) +
                                             (U32)(VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange= 2;
    }

    AllocBlockParams.PartitionHandle          = AVMEMPartition;
    AllocBlockParams.Size                     = Params1_p->AllocBlockParams.Size;
    AllocBlockParams.Alignment                = Params1_p->AllocBlockParams.Alignment;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    Err = STAVMEM_AllocBlock(&AllocBlockParams,BlockHandle1_p);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
    STAVMEM_GetBlockAddress(*BlockHandle1_p,(void**)&(Bitmap_p->Data1_p));

    Bitmap_p->Pitch =  Params1_p->Pitch;
    Bitmap_p->Size1  =  Params1_p->AllocBlockParams.Size;
    Bitmap_p->Offset = 0;
    Bitmap_p->BigNotLittle = 0;

    return(Err);


}

/*-----------------------------------------------------------------------------
 * Function : AllocPaletteBuffer
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/

ST_ErrorCode_t AllocPaletteBuffer (STGXOBJ_Palette_t* Palette_p ,
                                    void* SharedMemoryBaseAddress_p,
                                    STAVMEM_PartitionHandle_t AVMEMPartition,
                                    STAVMEM_BlockHandle_t*  BlockHandle_p,
                                    STGXOBJ_PaletteAllocParams_t* Params_p )
{

    STAVMEM_MemoryRange_t  RangeArea[2];
    STAVMEM_AllocBlockParams_t AllocBlockParams;
    ST_ErrorCode_t          Err;
    U8                      NbForbiddenRange;

    /* Data1 related */
    NbForbiddenRange = 1;
    if (VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
         VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) +
                                             (U32)(VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange= 2;
    }


    AllocBlockParams.PartitionHandle          = AVMEMPartition;
    AllocBlockParams.Size                     = Params_p->AllocBlockParams.Size;
    AllocBlockParams.Alignment                = Params_p->AllocBlockParams.Alignment;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    Err = STAVMEM_AllocBlock(&AllocBlockParams,BlockHandle_p);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
    STAVMEM_GetBlockAddress(*BlockHandle_p,(void**)&(Palette_p->Data_p));


    return(Err);

}
#endif

/*---------------------------------------------------------------------
 * Function : GUTIL_Allocate
 *            Allocate a ptr using STAVMEM mecanism,
 *            Manage internaly corespondance between ptr and
 *            BlockHandlde. See STAVMEM documentation for
 *            STAVMEM_AllocBlockParams_t definition.
 * Input    : STAVMEM_AllocBlockParams_t *AllocBlockParams_p
 * Output   : void **ptr  Pointer to allocated address
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
#ifdef ST_OSLINUX
ST_ErrorCode_t GUTIL_Allocate (STLAYER_AllocDataParams_t *AllocDataParams_p, BOOL SecuredData, void **ptr)
#else
ST_ErrorCode_t GUTIL_Allocate (STAVMEM_AllocBlockParams_t *AllocBlockParams_p, void **ptr)
#endif
{
    ST_ErrorCode_t ErrCode;

    ErrCode = ST_NO_ERROR;
    *ptr = NULL;

    /* Allocate on more space for storing BlockHandle                */
    if(Allocate_Number == ((1 << (8*sizeof(Allocate_Number_t)))-1))  /* For U16 type, ((1 << (8*sizeof(Allocate_Number_t)))-1) == 0xFFFF */
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"GUTIL_Allocate Error : no more allocation allowed\n"));
        return ST_ERROR_NO_MEMORY;
    }
    Allocate_Number ++;

    Allocate_KeepInfo =
    STOS_MemoryReallocate(SystemPartition_p,
			  (void *)Allocate_KeepInfo,
              (U32)(Allocate_Number*sizeof(Allocate_KeepInfo_t)),
              (U32)(Allocate_Number*sizeof(Allocate_KeepInfo_t)));

    if ((void *)Allocate_KeepInfo == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
		      "GUTIL_Allocate Error : Allocation error\n"));
        return ST_ERROR_NO_MEMORY;
    }


#ifdef ST_OSLINUX
#ifndef PTV
        if (SecuredData == TRUE)
        {
            ErrCode = STLAYER_AllocDataSecure( FakeLayerHndl, AllocDataParams_p, &(Allocate_KeepInfo[Allocate_Number-1].ptr) );
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "GUTIL_Allocate, STLAYER_AllocDataSecure(0x%08x)=%d %d, %d\n",
                Allocate_KeepInfo[Allocate_Number-1].ptr,ErrCode, AllocDataParams_p->Size, AllocDataParams_p->Alignment));
            }
            else
            {
                *ptr = Allocate_KeepInfo[Allocate_Number-1].ptr;
            }
        }
        else /* SecuredData */
#endif
        {
            ErrCode = STLAYER_AllocData( FakeLayerHndl, AllocDataParams_p, &(Allocate_KeepInfo[Allocate_Number-1].ptr) );
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "GUTIL_Allocate, STLAYER_AllocData(0x%08x)=%d %d, %d\n",
                Allocate_KeepInfo[Allocate_Number-1].ptr,ErrCode, AllocDataParams_p->Size, AllocDataParams_p->Alignment));
            }
            else
            {
                *ptr = Allocate_KeepInfo[Allocate_Number-1].ptr;
            }
        }
#else

    /* Space for storing BlockHandle is done, allocate it            */
    ErrCode = STAVMEM_AllocBlock(AllocBlockParams_p,&(Allocate_KeepInfo[Allocate_Number-1].BlockHandle));

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
              "GUTIL_Allocate, STAVMEM_Allocate(0x%08x,0x%08x)=%d\n",
		      Allocate_KeepInfo[Allocate_Number-1].BlockHandle,
		      Allocate_KeepInfo[Allocate_Number-1].ptr,
              ErrCode));
    }
    else
    {
        /* Retrieve address from BlockHandle                         */
        ErrCode = STAVMEM_GetBlockAddress
	    (Allocate_KeepInfo[Allocate_Number-1].BlockHandle,
	     &(Allocate_KeepInfo[Allocate_Number-1].ptr));
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
              "GUTIL_Allocate, STAVMEM_GetBlockAddress(0x%08x,0x%08x)=%d\n",
			  Allocate_KeepInfo[Allocate_Number-1].BlockHandle,
              Allocate_KeepInfo[Allocate_Number-1].ptr,ErrCode));
        }
        else
        {
            *ptr = Allocate_KeepInfo[Allocate_Number-1].ptr;
        }
    }
#endif

    return ErrCode;
} /* GUTIL_Allocate() */


/*---------------------------------------------------------------------
 * Function : GUTIL_Free
 *            Free a ptr previously allocated with GUTIL_Allocate.
 *            Mecanism used was STAVMEM, internaly we manage the
 *            corespondance between ptr and BlockHandlde.
 * Input    : void *ptr
 * Output   :
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_Free(void *ptr)
{
    ST_ErrorCode_t ErrCode;
#ifndef ST_OSLINUX
    STAVMEM_FreeBlockParams_t FreeBlockParams;
#endif
    Allocate_Number_t loop;
    void *tmp_p;

    ErrCode = ST_NO_ERROR;
#ifndef ST_OSLINUX
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
#endif

#ifdef ST_OSLINUX
    printf("GUTIL_Free() : Address to free 0x%x\n", ptr );
#endif

    /* Search ptr into previously allocated by GUTIL_Allocate        */
    for(loop=0; loop<Allocate_Number;loop ++)
    {
        if (ptr == Allocate_KeepInfo[loop].ptr)
        {
            break;
        }
    } /* for(loop=0; loop<Allocate_Number;loop ++) */
    if (loop == Allocate_Number)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"GUTIL_Free error : Bad pointer, not allocated by GxAllocate\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

#ifdef ST_OSLINUX
    ErrCode = STLAYER_FreeData( FakeLayerHndl, ptr );
#else
    /* Free the ptr, its handle is used to do that with STAVMEM      */
    ErrCode = STAVMEM_FreeBlock(&FreeBlockParams, &(Allocate_KeepInfo[loop].BlockHandle));
#endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "GUTIL_Free error : Unable to free with STAVMEM the ptr 1\n"));
    }
    else
    {
        /* Now, we must clean our Allocate_KeepInfo array, and       */
        /* ralloc it smaller.                                        */
        Allocate_Number--;
        if (loop != Allocate_Number)
        {
            Allocate_KeepInfo[loop] = Allocate_KeepInfo[Allocate_Number];
        }
        /* If equal, this is the last on the tabs,                   */
        /* do not need to reorganise.                                */

        /* Clean last before unallocating                            */
        Allocate_KeepInfo[Allocate_Number].ptr = NULL;
        Allocate_KeepInfo[Allocate_Number].BlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;

        tmp_p = STOS_MemoryReallocate(SystemPartition_p,(void *)Allocate_KeepInfo,Allocate_Number*sizeof(Allocate_KeepInfo_t),Allocate_Number*sizeof(Allocate_KeepInfo_t));
        if ( ((void *)tmp_p == NULL) && (Allocate_Number != 0) )
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"GUTIL_Free Error : Reallocation error\n"));
            return ST_ERROR_NO_MEMORY;
        }
        Allocate_KeepInfo = (Allocate_KeepInfo_t *)tmp_p;
    }

    return ErrCode;

} /* GUTIL_Free() */

/*-----------------------------------------------------------------------------
 * Function : ConvertGammaToBitmap
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t ConvertGammaToBitmap(char*               filename,
                                    U8                  PartitionInfo,
                                    STGXOBJ_Bitmap_t*   Bitmap_p,
                                    STGXOBJ_Palette_t*  Palette_p)
{
#ifdef ST_OSLINUX
    BOOL                         SecuredData = (BOOL)PartitionInfo;
    STLAYER_AllocDataParams_t    AllocParams; /* Allocation param for linux*/
#else
    U8                          AvmemPartition_ID = PartitionInfo;
    STAVMEM_AllocBlockParams_t  AllocParams; /* Allocation param*/
    STAVMEM_MemoryRange_t       RangeArea[2];
    U8                          NbForbiddenRange;
#endif
    ST_ErrorCode_t              ErrCode;         /* Error management */
    FILE*                       fstream;        /* File handle for read operation          */
    void                        *buffer_p, *buffer2_p=NULL;       /* Temporay pointer storage for allocation */
    U32                         size;            /* Used to test returned size for a read   */
    U32                         dummy[2];         /* used to read colorkey, but not used     */
    GUTIL_GammaHeader           Gamma_Header; /* Header of Bitmap file         */
#ifdef ST_ZEUS
    GUTIL_GammaHeader           Gamma_Header_Temp; /* Header of Bitmap file         */
#endif
    int Rest;



    ErrCode = ST_NO_ERROR;

#ifndef ST_OSLINUX
    /* Block allocation parameter */
    NbForbiddenRange = 1;
    if (VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) - 1);

    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
         VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) +
                                             (U32)(VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(VirtualMapping.VirtualSize) - 1);
        NbForbiddenRange= 2;
    }

    AllocParams.PartitionHandle            = AvmemPartitionHandle[AvmemPartition_ID];
    AllocParams.Size                       = 0;
    AllocParams.AllocMode                  = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges    = NbForbiddenRange;
    AllocParams.ForbiddenRangeArray_p      = &RangeArea[0];
    AllocParams.NumberOfForbiddenBorders   = 0;
    AllocParams.ForbiddenBorderArray_p     = NULL;
#endif
    /* Check parameter */
    if ((filename == NULL) || (Bitmap_p == NULL) ||(Palette_p == NULL))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error: Null Pointer, bad parameter\n"));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

    /* Check if a bitmap was previously allocated here               */
    /* Check is based on the NULL value of the pointer, so init      */
    /* is very important.                                            */
    if (ErrCode == ST_NO_ERROR)
    {
        if (Bitmap_p->Data1_p != NULL)
        {
            /* Deallocate buffer before changing the pointer */
            /* GUTIL_Free(Bitmap_p->Data1_p);*/
            Bitmap_p->Data1_p = NULL;
        }
        if (Palette_p->Data_p != NULL)
        {
            /* Deallocate buffer before changing the pointer  */
            /*GUTIL_Free(Palette_p->Data_p);*/
            Palette_p->Data_p = NULL;
        }
    }

    /* Open file handle */
    if (ErrCode == ST_NO_ERROR)
    {
        fstream = fopen(filename, "rb");
        if( fstream == NULL )
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to open \'%s\'\n", filename ));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Read Header */
    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Print(("Reading file Header ... \n"));

#ifdef ST_ZEUS  /* temp hack */
        size = fread((void *)&Gamma_Header_Temp, 1,sizeof(GUTIL_GammaHeader), fstream);
#else
        size = fread((void *)&Gamma_Header, 1,sizeof(GUTIL_GammaHeader), fstream);
#endif


        if (size != sizeof(GUTIL_GammaHeader))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,sizeof(GUTIL_GammaHeader)));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            fclose (fstream);
        }
    }

#ifdef ST_ZEUS  /* temp hack */
    STSYS_WriteRegMem16BE(&(Gamma_Header.Header_size),Gamma_Header_Temp.Header_size);
	STSYS_WriteRegMem16BE(&(Gamma_Header.Signature),Gamma_Header_Temp.Signature);
	STSYS_WriteRegMem16BE(&(Gamma_Header.Type),Gamma_Header_Temp.Type);
	STSYS_WriteRegMem16BE(&(Gamma_Header.Properties),Gamma_Header_Temp.Properties);
	STSYS_WriteRegMem32BE(&(Gamma_Header.width),Gamma_Header_Temp.width);
	STSYS_WriteRegMem32BE(&(Gamma_Header.height),Gamma_Header_Temp.height);
	STSYS_WriteRegMem32BE(&(Gamma_Header.nb_pixel),Gamma_Header_Temp.nb_pixel);
	STSYS_WriteRegMem32BE(&(Gamma_Header.zero),Gamma_Header_Temp.zero);
#endif

    /* Check Header */
    if (ErrCode == ST_NO_ERROR)
    {
        if (  ( ( Gamma_Header.Header_size != 0x6 ) && ( Gamma_Header.Header_size != 0x8 ) ) ||
              ( ( Gamma_Header.Signature != 0x444F ) && ( Gamma_Header.Signature != 0x422F ) && ( Gamma_Header.Signature != 0x420F ) ) ||
              ( ( Gamma_Header.Type != GAMMA_FILE_YCBCR420MB ) && ( Gamma_Header.zero != 0x0 ) )
           )
        {
            STTBX_Print(("Read %d waited 0x6 or 0x8\n",Gamma_Header.Header_size));
            STTBX_Print(("Read %d waited 0x44F or 0x22F\n",Gamma_Header.Signature));
            STTBX_Print(("Read %d waited 0x0\n",Gamma_Header.Properties));
            STTBX_Print(("Read %d waited 0x0\n",Gamma_Header.zero));
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Not a valid file (Header corrupted)\n"));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            fclose (fstream);
        }
    }

    /* If present read colorkey but do not use it.                   */
    if (ErrCode == ST_NO_ERROR)
    {
        if (Gamma_Header.Header_size == 0x8)
        {
            /* colorkey are 2 32-bits word, but it's safer to use    */
            /* sizeof(dummy) when reading. And to check that it's 4. */
            size = fread((void *)dummy, 1, sizeof(dummy), fstream);
            if (size != 4)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n", size,4));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                fclose (fstream);
            }
        }
    }

    /* In function of bitmap type, configure some variables   */
    /* And do bitmap allocation                               */
    if (ErrCode == ST_NO_ERROR)
    {
        Bitmap_p->BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;

        /* Configure in function of the bitmap type           */
        switch (Gamma_Header.Type)
        {
            case GAMMA_FILE_ACLUT8 :
            {
                AllocParams.Alignment = 16; /* CLUT is 16 bytes aligned  */
                Bitmap_p->Pitch            = Gamma_Header.width;
                /* Allocate memory for palette                           */
                if (Gamma_Header.Type == 0x8B)
                {
                    STTBX_Print(("Allocate for palette ...\n"));
                    /* ARGB8888 palette for a CLUT8 bitmap  */
                    AllocParams.Size = 256*4;

#ifdef ST_OSLINUX
        GUTIL_Allocate(&AllocParams, SecuredData, &buffer_p);
#else
        GUTIL_Allocate(&AllocParams, &buffer_p);
#endif
                  if (buffer_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate for palette\n"));
                        ErrCode = ST_ERROR_NO_MEMORY;
                        fclose (fstream);
                    }
                }

                /* Read palette if necessary */
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Print(("Read palette ...\n"));
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                    Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                    Palette_p->ColorDepth  = 8;
                    Palette_p->Data_p = buffer_p;
#ifdef ST_OSLINUX
                    size = fread ((void *)Palette_p->Data_p, 1,(256*4), fstream);
#else
                    size = fread ((void *)(STAVMEM_VirtualToCPU((void*)Palette_p->Data_p,&VirtualMapping)), 1,(256*4), fstream);
#endif
                    STTBX_Print(("\tPalette_p->Data_p : 0X%08x\n",Palette_p->Data_p));
                    STTBX_Print(("\tAllocate_Number : %d\n",Allocate_Number));
                    STTBX_Print(("\tAllocate_KeepIndo : 0x%08x\n",Allocate_KeepInfo));

                    if (size != (256*4))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,(256*4)));
                        ErrCode = ST_ERROR_BAD_PARAMETER;
                        fclose (fstream);
                        GUTIL_Free(Palette_p->Data_p);
                    }
                }

                Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_CLUT8;
                Bitmap_p->Size1     = Gamma_Header.nb_pixel;
                break;
            }
            case GAMMA_FILE_ACLUT4 :
            {
                AllocParams.Alignment = 16; /* CLUT is 16 bytes aligned  */
                /* Allocate memory for palette                           */
                if (Gamma_Header.Type == 0x8A)
                {
                    STTBX_Print(("Allocate for palette ...\n"));
                    /* ARGB8888 palette for a CLUT4 bitmap  */
                    AllocParams.Size = 16*4;
#ifdef ST_OSLINUX
        GUTIL_Allocate(&AllocParams, SecuredData, &buffer_p);
#else
        GUTIL_Allocate(&AllocParams, &buffer_p);
#endif
                  if (buffer_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate for palette\n"));
                        ErrCode = ST_ERROR_NO_MEMORY;
                        fclose (fstream);
                    }
                }

                /* Read palette if necessary */
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Print(("Read palette ...\n"));
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                    Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                    Palette_p->ColorDepth  = 4;
                    Palette_p->Data_p = buffer_p;
#ifdef ST_OSLINUX
                    size = fread ((void *)Palette_p->Data_p, 1,(16*4), fstream);
#else
                    size = fread ((void *)(STAVMEM_VirtualToCPU((void*)Palette_p->Data_p,&VirtualMapping)), 1,(16*4), fstream);
#endif
                    STTBX_Print(("\tPalette_p->Data_p : 0X%08x\n",Palette_p->Data_p));
                    STTBX_Print(("\tAllocate_Number : %d\n",Allocate_Number));
                    STTBX_Print(("\tAllocate_KeepIndo : 0x%08x\n",Allocate_KeepInfo));

                    if (size != (16*4))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,(16*4)));
                        ErrCode = ST_ERROR_BAD_PARAMETER;
                        fclose (fstream);
                        GUTIL_Free(Palette_p->Data_p);
                    }
                }

                Rest = Gamma_Header.width % 2;
                if (Rest == 0)
                {
                    Bitmap_p->Pitch = Gamma_Header.width / 2;
                }
                else
                {
                    Bitmap_p->Pitch = (Gamma_Header.width / 2) + 1;

                }
                if (Gamma_Header.Properties == 0)
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
                }
                else
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
                }
                Bitmap_p->Size1 = Bitmap_p->Pitch * Gamma_Header.height;
                AllocParams.Alignment = 1; /* Byte aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_CLUT4;
                break;





            }

            case GAMMA_FILE_ACLUT2 :
            {
                AllocParams.Alignment = 16; /* CLUT is 16 bytes aligned  */
                /* Allocate memory for palette                           */
                if (Gamma_Header.Type == 0x89)
                {
                    STTBX_Print(("Allocate for palette ...\n"));
                    /* ARGB8888 palette for a CLUT4 bitmap  */
                    AllocParams.Size = 4*4;
 #ifdef ST_OSLINUX
        GUTIL_Allocate(&AllocParams, SecuredData, &buffer_p);
#else
        GUTIL_Allocate(&AllocParams, &buffer_p);
#endif
                   if (buffer_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate for palette\n"));
                        ErrCode = ST_ERROR_NO_MEMORY;
                        fclose (fstream);
                    }
                }

                /* Read palette if necessary */
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Print(("Read palette ...\n"));
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                    Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                    Palette_p->ColorDepth  = 2;
                    Palette_p->Data_p = buffer_p;
#ifdef ST_OSLINUX
                    size = fread ((void *)Palette_p->Data_p, 1,(4*4), fstream);
#else
                    size = fread ((void *)(STAVMEM_VirtualToCPU((void*)Palette_p->Data_p,&VirtualMapping)), 1,(4*4), fstream);
#endif
                    STTBX_Print(("\tPalette_p->Data_p : 0X%08x\n",Palette_p->Data_p));
                    STTBX_Print(("\tAllocate_Number : %d\n",Allocate_Number));
                    STTBX_Print(("\tAllocate_KeepIndo : 0x%08x\n",Allocate_KeepInfo));

                    if (size != (4*4))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,(4*4)));
                        ErrCode = ST_ERROR_BAD_PARAMETER;
                        fclose (fstream);
                        GUTIL_Free(Palette_p->Data_p);
                    }
                }

                Rest = Gamma_Header.width % 4;
                if (Rest == 0)
                {
                    Bitmap_p->Pitch = Gamma_Header.width / 4;
                }
                else
                {
                    Bitmap_p->Pitch = (Gamma_Header.width / 4) + 1;

                }
                if (Gamma_Header.Properties == 0)
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
                }
                else
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
                }
                Bitmap_p->Size1 = Bitmap_p->Pitch * Gamma_Header.height;
                AllocParams.Alignment = 1; /* Byte aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_CLUT2;
                break;

            }

            case GAMMA_FILE_RGB888:
            {
                Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_RGB888;
                Bitmap_p->Pitch         =  Gamma_Header.width * 3;
                Bitmap_p->Size1     = Gamma_Header.width * 4 * Gamma_Header.height ;
                AllocParams.Alignment = 3;
                break;
            }

            case GAMMA_FILE_ALPHA1 :
            {
                Rest = Gamma_Header.width % 8;
                if (Rest == 0)
                {
                    Bitmap_p->Pitch = Gamma_Header.width / 8;
                }
                else
                {
                    Bitmap_p->Pitch = (Gamma_Header.width / 8) + 1;

                }
                if (Gamma_Header.Properties == 0)
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
                }
                else
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
                }
                Bitmap_p->Size1 = Bitmap_p->Pitch * Gamma_Header.height;
                AllocParams.Alignment = 1; /* Byte aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_ALPHA1;
                break;
            }

            case GAMMA_FILE_ALPHA8 :
            {
                Bitmap_p->Pitch = Gamma_Header.width;
                if (Gamma_Header.Properties == 0)
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
                }
                else
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
                }
                Bitmap_p->Size1 = Bitmap_p->Pitch * Gamma_Header.height;
                AllocParams.Alignment = 1; /* Byte aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_ALPHA8;
                break;
            }

            case GAMMA_FILE_RGB565 :
            {
                Bitmap_p->Pitch            = Gamma_Header.width*2;
                AllocParams.Alignment = 2; /* 16-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_RGB565;
                Bitmap_p->Size1            = Gamma_Header.nb_pixel*2;
                break;
            }
            case GAMMA_FILE_ARGB8888 :
            {
                Bitmap_p->Pitch            = Gamma_Header.width*4;
                AllocParams.Alignment = 4; /* 16-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_ARGB8888;
                Bitmap_p->Size1            = Gamma_Header.nb_pixel*4;
                break;
            }

            case GAMMA_FILE_ARGB1555 :
            {
                Bitmap_p->Pitch            = Gamma_Header.width*2;
                AllocParams.Alignment = 2; /* 16-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_ARGB1555;
                Bitmap_p->Size1            = Gamma_Header.nb_pixel*2;
                break;
            }
            case GAMMA_FILE_ARGB4444 :
            {
                Bitmap_p->Pitch            = Gamma_Header.width*2;
                AllocParams.Alignment = 2; /* 16-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_ARGB4444;
                Bitmap_p->Size1            = Gamma_Header.nb_pixel*2;
                break;
            }
            case GAMMA_FILE_YCBCR422R :
            {
                if ((Gamma_Header.width % 2) == 0)
                {
                    Bitmap_p->Pitch = (U32)(Gamma_Header.width * 2);
                }
                else
                {
                    Bitmap_p->Pitch = (U32)((Gamma_Header.width - 1) * 2 + 4);
                }


                AllocParams.Alignment = 4; /* 32-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
                Bitmap_p->Size1            = Bitmap_p->Pitch * Gamma_Header.height;
                break;
            }
            case GAMMA_FILE_YCBCR420MB :
            {
                Bitmap_p->BitmapType     = STGXOBJ_BITMAP_TYPE_MB;
                if ((Gamma_Header.width % 16) == 0)   /* MB size multiple */
                {
                    Bitmap_p->Pitch = Gamma_Header.width;
                }
                else
                {
                    Bitmap_p->Pitch = (Gamma_Header.width/16 + 1) * 16;
                }
                AllocParams.Alignment = 16; /* 16-bytes aligned */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420;
                Bitmap_p->Size1            = Gamma_Header.nb_pixel;
                Bitmap_p->Size2            = Gamma_Header.zero;
                break;
            }

            default :
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Type not supported 0x%08x\n",Gamma_Header.Type));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                break;
            }
        } /* switch (Gamma_Header.Type) */
    }

    if (ErrCode == ST_NO_ERROR && Gamma_Header.Type != GAMMA_FILE_RGB888)
    {
        /* Allocate memory for bitmap                                */
#ifdef ST_OSLINUX
        STTBX_Print(("Allocate for bitmap  %s...\n", filename));
#else
        STTBX_Print(("Allocate for bitmap ...\n"));
#endif
        AllocParams.Size = (U32)(Bitmap_p->Size1);
#ifdef ST_OSLINUX
        GUTIL_Allocate(&AllocParams, SecuredData, &buffer_p);
#else
        GUTIL_Allocate(&AllocParams, &buffer_p);
#endif
        if (buffer_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate for bitmap %d bytes\n",Bitmap_p->Size1));
            ErrCode = ST_ERROR_NO_MEMORY;
            fclose (fstream);
            if (Palette_p->Data_p != NULL)
            {
                GUTIL_Free(Palette_p->Data_p);
            }
        }

        if ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB )
        {
            STTBX_Print(("Allocate for Chroma ...\n"));
            AllocParams.Size = (U32)(Bitmap_p->Size2);
#ifdef ST_OSLINUX
        GUTIL_Allocate(&AllocParams, SecuredData, &buffer2_p);
#else
        GUTIL_Allocate(&AllocParams, &buffer2_p);
#endif
            if (buffer2_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate for bitmap %d bytes\n",Bitmap_p->Size2));
                ErrCode = ST_ERROR_NO_MEMORY;
                fclose (fstream);
                if (Palette_p->Data_p != NULL)
                {
                    GUTIL_Free(Palette_p->Data_p);
                }
            }
        }
    }

    if ( Gamma_Header.Type == GAMMA_FILE_RGB888 )
    {
        void*       Buffer3_p = NULL;
        U32         PixelsBuffer, i;
        U8          TempBuffer;
        U8*         AddDst;
        U8          R,G,B;

        printf("(RGB888 type convert)\n");
        AllocParams.Alignment = 3;
        AllocParams.Size = (U32)(Bitmap_p->Size1) - (Gamma_Header.width * Gamma_Header.height);
#ifdef ST_OSLINUX
        GUTIL_Allocate(&AllocParams, SecuredData, &Buffer3_p);
#else
        GUTIL_Allocate(&AllocParams, &Buffer3_p);
#endif

        for ( i=1 ; i<= Gamma_Header.width * Gamma_Header.height ; i++ )
        {
            fread (&PixelsBuffer, 3, 1, fstream);
            fread (&TempBuffer, 1, 1, fstream);

#ifndef ST_ZEUS
            R  = ( ( PixelsBuffer & 0xFF ) );
            G = ( ( PixelsBuffer & 0xFF00 ) >> 8 );
            B = ( ( PixelsBuffer & 0xFF0000 ) >> 16 );
#else
            B  = ( ( PixelsBuffer & 0xFF00 )>> 8 );
            G = ( ( PixelsBuffer & 0xFF0000 ) >> 16 );
            R = ( ( PixelsBuffer & 0xFF000000 ) >> 24 );
#endif /*ST_ZEUS*/

            AddDst = ((U8*)Buffer3_p) + 3*(i-1);
            *((U8 *)AddDst )     = R ;
            *((U8 *)AddDst + 1 ) = G ;
            *((U8 *)AddDst + 2 ) = B ;
        }

        buffer_p = Buffer3_p;
        Bitmap_p->Size1 = AllocParams.Size ;
        size = Bitmap_p->Size1 = AllocParams.Size ;
    }


    /* Read bitmap                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
/*        Bitmap_p->BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;*/
        Bitmap_p->PreMultipliedColor     = FALSE;
        Bitmap_p->ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
        Bitmap_p->AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
        Bitmap_p->Width                  = Gamma_Header.width;
        Bitmap_p->Height                 = Gamma_Header.height;
        Bitmap_p->Data1_p                = buffer_p;
        Bitmap_p->Data2_p                = buffer2_p;
        Bitmap_p->Offset = 0;
        Bitmap_p->BigNotLittle = 0;

        STTBX_Print(("Reading file Bitmap data ... \n"));





#if defined(ST200_OS21)

{
    U32 RemainingDataToLoad;
    U32 MaxReadSize = 100*1024;

    /* On ST200, if you try to load big files (>2M) at once, you will have a core dump (I guess a toolset bug). So, here
     * we split reading into packets of 100K */
    RemainingDataToLoad = /*FileSize*/Bitmap_p->Size1;
      size=Bitmap_p->Size1;
    while(RemainingDataToLoad != 0)
    {
        if(RemainingDataToLoad > MaxReadSize)
        {

            fread ((void *)(STAVMEM_VirtualToCPU((void*)Bitmap_p->Data1_p+ (Bitmap_p->Size1 - RemainingDataToLoad),&VirtualMapping)), 1,(size_t) MaxReadSize, fstream);
            RemainingDataToLoad -= MaxReadSize;
        }
        else
        {

            fread ((void *)(STAVMEM_VirtualToCPU((void*)Bitmap_p->Data1_p+ (Bitmap_p->Size1 - RemainingDataToLoad),&VirtualMapping)), 1,(size_t) RemainingDataToLoad, fstream);
            RemainingDataToLoad = 0;
        }
    }
}
#else
if ( Gamma_Header.Type != GAMMA_FILE_RGB888 )
{
    #ifdef ST_OSLINUX
            size = fread ((void *)Bitmap_p->Data1_p, 1,(Bitmap_p->Size1), fstream);
    #else
            size = fread ((void *)(STAVMEM_VirtualToCPU((void*)Bitmap_p->Data1_p,&VirtualMapping)), 1,(Bitmap_p->Size1), fstream);
    #endif
}
#endif




        if (size != (Bitmap_p->Size1))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d 11\n",size,Bitmap_p->Size1));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            GUTIL_Free(Palette_p->Data_p);
            if (Palette_p->Data_p != NULL)
            {
                GUTIL_Free(Palette_p->Data_p);
            }

        }

        if ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB )
        {
            STTBX_Print(("Reading file Chroma data ... \n"));
    #if defined(ST200_OS21)
            {
                U32 RemainingDataToLoad;
                U32 MaxReadSize = 100*1024;

                /* On ST200, if you try to load big files (>2M) at once, you will have a core dump (I guess a toolset bug). So, here
                * we split reading into packets of 100K */
                RemainingDataToLoad = Bitmap_p->Size2;
                size=Bitmap_p->Size2;
                while(RemainingDataToLoad != 0)
                {
                    if(RemainingDataToLoad > MaxReadSize)
                    {
                        #ifdef ST_OSLINUX
                            fread ((void*)Bitmap_p->Data2_p+ (Bitmap_p->Size2 - RemainingDataToLoad, 1,(size_t) MaxReadSize, fstream);
                        #else
                            fread ((void *)(STAVMEM_VirtualToCPU((void*)Bitmap_p->Data2_p+ (Bitmap_p->Size2 - RemainingDataToLoad),&VirtualMapping)), 1,(size_t) MaxReadSize, fstream);
                        #endif
                        RemainingDataToLoad -= MaxReadSize;
                    }
                    else
                    {
                        #ifdef ST_OSLINUX
                            fread ((void*)Bitmap_p->Data2_p+ (Bitmap_p->Size2 - RemainingDataToLoad, 1,(size_t) RemainingDataToLoad, fstream);
                        #else
                            fread ((void *)(STAVMEM_VirtualToCPU((void*)Bitmap_p->Data2_p+ (Bitmap_p->Size2 - RemainingDataToLoad),&VirtualMapping)), 1,(size_t) RemainingDataToLoad, fstream);
                        #endif
                        RemainingDataToLoad = 0;
                    }
                }
            }
    #else
        #ifdef ST_OSLINUX
            size = fread ((void *)Bitmap_p->Data2_p, 1,(Bitmap_p->Size2), fstream);
        #else
            size = fread ((void *)(STAVMEM_VirtualToCPU((void*)Bitmap_p->Data2_p,&VirtualMapping)), 1,(Bitmap_p->Size2), fstream);
        #endif
    #endif
            if (size != (Bitmap_p->Size2))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,Bitmap_p->Size2));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                GUTIL_Free(Palette_p->Data_p);
                if (Palette_p->Data_p != NULL)
                {
                    GUTIL_Free(Palette_p->Data_p);
                }

            }
        } /* if ( Bitmap_p->BitmapType == = STGXOBJ_BITMAP_TYPE_MB ) */
        fclose (fstream);
    }

    return ErrCode;

}
/*-----------------------------------------------------------------------------
 * Function : ConvertBitmapToGamma
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t  ConvertBitmapToGamma ( char*                filename,
                                       STGXOBJ_Bitmap_t*    Bitmap_p,
                                       STGXOBJ_Palette_t*   Palette_p)
{

    ST_ErrorCode_t      ErrCode;      /* Error management                       */
#ifndef PTV
    FILE*               fstream;      /* File Handle for write operation        */
#else
    int                 fd;           /* File descriptor for write operation    */
#endif /* !PTV */
    U32                 size;         /* Used to test returned size for a write */
    GUTIL_GammaHeader   Gamma_Header; /* Header of file                         */
#ifdef ST_ZEUS
    GUTIL_GammaHeader   Gamma_Header_Temp; /* Header of Bitmap file         */
#endif
    BOOL                IsPalette;    /* If bitmap with a palette : TRUE        */

    ErrCode = ST_NO_ERROR;
    IsPalette = FALSE;

    /* Check parameter                                               */
    if ((filename == NULL) || (Bitmap_p == NULL))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Null Pointer, bad parameter\n"));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

    /* Check if pointer to data bitmap are non NULL                  */
    if (ErrCode == ST_NO_ERROR)
    {
        if (Bitmap_p->Data1_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Bitmap or Palette data pointer are NULL\n"));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Check NULL pointer for palette if necessary                   */
    if ((ErrCode == ST_NO_ERROR) && ((Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT8) ||
                                     (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4) ||
                                     (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT2)))
    {
        if (Palette_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Null Pointer, bad parameter\n"));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            if (Palette_p->Data_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Palette data pointer are NULL\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
        }
    }


    /* Make all check if the bitmap is supported and fill information */
    /* before storing it to the filename.                             */
    /* Set Gamma file header                                          */
    /* No colorkey are stored, so header size is 0x6                  */
	memset((void *)&Gamma_Header,0,sizeof(GUTIL_GammaHeader));
	Gamma_Header.Header_size = 0x6;
	Gamma_Header.Signature   = 0x444F;
	Gamma_Header.Properties  = 0x0;
	Gamma_Header.width       = Bitmap_p->Width;
	Gamma_Header.height      = Bitmap_p->Height;
	Gamma_Header.zero        = 0x0;           /* non used field  */

	switch(Bitmap_p->ColorType)
	{
        case STGXOBJ_COLOR_TYPE_CLUT8:
        {
            /* Check the palette type, only ARGB8888 is supported    */
            if ((Palette_p->ColorType  != STGXOBJ_COLOR_TYPE_ARGB8888) || (Palette_p->ColorDepth != 8) )
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Only CLUT8 based bitmap with a ARGB8888 palette are supported\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
            IsPalette             = TRUE;
            Gamma_Header.Type     = GAMMA_FILE_ACLUT8;
            Gamma_Header.nb_pixel = Bitmap_p->Size1;
            break;
        }
        case STGXOBJ_COLOR_TYPE_CLUT4:
        {
            /* Check the palette type, only ARGB8888 is supported    */
            if ((Palette_p->ColorType  != STGXOBJ_COLOR_TYPE_ARGB8888) || (Palette_p->ColorDepth != 4) )
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Only CLUT4 based bitmap with a ARGB8888 palette are supported\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
            IsPalette             = TRUE;
            Gamma_Header.Type     = GAMMA_FILE_ACLUT4;
            Gamma_Header.nb_pixel = (Bitmap_p->Size1) * 2;
            break;
        }
        case STGXOBJ_COLOR_TYPE_CLUT2:
        {
            /* Check the palette type, only ARGB8888 is supported    */
            if ((Palette_p->ColorType  != STGXOBJ_COLOR_TYPE_ARGB8888) || (Palette_p->ColorDepth != 2) )
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Only CLUT2 based bitmap with a ARGB8888 palette are supported\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
            IsPalette             = TRUE;
            Gamma_Header.Type     = GAMMA_FILE_ACLUT2;
            Gamma_Header.nb_pixel = (Bitmap_p->Size1) * 4;
            break;
        }

        case STGXOBJ_COLOR_TYPE_ALPHA1:
        {
            Gamma_Header.Type     = GAMMA_FILE_ALPHA1;
            Gamma_Header.nb_pixel = (Bitmap_p->Size1)*8;
            break;
        }

        case STGXOBJ_COLOR_TYPE_ALPHA8:
        {
            Gamma_Header.Type     = GAMMA_FILE_ALPHA8;
            Gamma_Header.nb_pixel = Bitmap_p->Size1;
            break;
        }

        case STGXOBJ_COLOR_TYPE_ARGB8565:
        {
            Gamma_Header.Type     = GAMMA_FILE_ARGB8565;
            Gamma_Header.nb_pixel = (Bitmap_p->Size1)/3;
            break;
        }

        case STGXOBJ_COLOR_TYPE_RGB565:
        {
            Gamma_Header.Type     = GAMMA_FILE_RGB565;
            Gamma_Header.nb_pixel = (Bitmap_p->Size1)/2;
            break;
        }
        case STGXOBJ_COLOR_TYPE_RGB888:
        {
            Gamma_Header.Type     = GAMMA_FILE_RGB888;
            Gamma_Header.nb_pixel = (Bitmap_p->Width * Bitmap_p->Height);
            break;
        }



        case STGXOBJ_COLOR_TYPE_ARGB8888:
        {
            Gamma_Header.Type     = GAMMA_FILE_ARGB8888;
            Gamma_Header.nb_pixel = (Bitmap_p->Size1)/4;
            break;
        }


        case STGXOBJ_COLOR_TYPE_ARGB1555:
        {
            Gamma_Header.Type     = GAMMA_FILE_ARGB1555;
            Gamma_Header.nb_pixel = (Bitmap_p->Size1)/2;
	    break;
        }
        case STGXOBJ_COLOR_TYPE_ARGB4444:
        {
            Gamma_Header.Type     = GAMMA_FILE_ARGB4444;
            Gamma_Header.nb_pixel = (Bitmap_p->Size1)/2;
            break;
        }
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
        {
            Gamma_Header.Type     = GAMMA_FILE_YCBCR422R;
            Gamma_Header.nb_pixel = (Bitmap_p->Size1)/2;
            break;
        }
        default:
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Bitmap type not supported : 0x%08x\n", Bitmap_p->ColorType));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            break;
        }
    } /* switch(Bitmap_p->ColorType) */


    /* Open file handle to save bitmap                               */
    if (ErrCode == ST_NO_ERROR)
    {
#ifndef PTV
        fstream = fopen(filename, "wb");
        if (fstream == NULL)
#else
        fd      = open (filename, O_WRONLY | O_CREAT);
        if (fd == -1)
#endif /* !PTV */
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Unable to open \'%s\'\n",filename));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Build header and write it                                     */
    if (ErrCode == ST_NO_ERROR)
    {

#ifdef ST_ZEUS  /* temp hack */
        STSYS_WriteRegMem16BE(&(Gamma_Header_Temp.Header_size),Gamma_Header.Header_size);
        STSYS_WriteRegMem16BE(&(Gamma_Header_Temp.Signature),Gamma_Header.Signature);
        STSYS_WriteRegMem16BE(&(Gamma_Header_Temp.Type),Gamma_Header.Type);
        STSYS_WriteRegMem16BE(&(Gamma_Header_Temp.Properties),Gamma_Header.Properties);
        STSYS_WriteRegMem32BE(&(Gamma_Header_Temp.width),Gamma_Header.width);
        STSYS_WriteRegMem32BE(&(Gamma_Header_Temp.height),Gamma_Header.height);
        STSYS_WriteRegMem32BE(&(Gamma_Header_Temp.nb_pixel),Gamma_Header.nb_pixel);
        STSYS_WriteRegMem32BE(&(Gamma_Header_Temp.zero),Gamma_Header.zero);

        STTBX_Print(("Saving Header file ...\n"));
#ifndef PTV
        size = fwrite((void *)&Gamma_Header_Temp, 1, sizeof(GUTIL_GammaHeader), fstream);
#else
        size = write(fd, (void *)&Gamma_Header_Temp, sizeof(GUTIL_GammaHeader));
#endif /* !PTV */

#else

        STTBX_Print(("Saving Header file ...\n"));
        size = fwrite((void *)&Gamma_Header, 1, sizeof(GUTIL_GammaHeader), fstream);

#endif
        if (size != sizeof(GUTIL_GammaHeader))
        {
            STTBX_Print(("GUTIL_SaveBitmap : Write header error %d byte instead of %d\n", size,sizeof(GUTIL_GammaHeader)));
            ErrCode = ST_ERROR_BAD_PARAMETER;
    #ifndef PTV
            fclose (fstream);
    #else
            close (fd);
    #endif /* !PTV */
        }
    }

    /* If CLUT8 type, write palette                                  */
    if ( (ErrCode == ST_NO_ERROR) && (IsPalette == TRUE) )
    {
        STTBX_Print(("Saving palette ...\n" ));

        switch(Bitmap_p->ColorType)
        {
            case STGXOBJ_COLOR_TYPE_CLUT8:
            {
                /* Palette size is 256*4, ARGB8888 for a CLUT8 bitmap        */
#ifdef ST_OSLINUX
#ifndef PTV
                size = fwrite((void *)Palette_p->Data_p, 1,(256*4), fstream);
#else
                size = write(fd, (void *)Palette_p->Data_p, (256*4));
#endif /* !PTV */
#else
                size = fwrite((void *)STAVMEM_VirtualToCPU((void*)Palette_p->Data_p,&VirtualMapping), 1,(256*4), fstream);
#endif
                if (size != (256*4))
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Write Palette error %d byte instead of %d\n",size,(256*4)));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
        #ifndef PTV
                    fclose (fstream);
        #else
                    close (fd);
        #endif /* !PTV */
                }
                break;
            }
            case STGXOBJ_COLOR_TYPE_CLUT4:
            {
                /* Palette size is 16*4, ARGB8888 for a CLUT4 bitmap        */
#ifdef ST_OSLINUX
#ifndef PTV
                size = fwrite((void *)Palette_p->Data_p, 1,(16*4), fstream);
#else
                size = write(fd, (void *)Palette_p->Data_p, (16*4));
#endif /* !PTV */
#else
                size = fwrite((void *)STAVMEM_VirtualToCPU((void*)Palette_p->Data_p,&VirtualMapping), 1,(16*4), fstream);
#endif
                if (size != (16*4))
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Write Palette error %d byte instead of %d\n",size,(16*4)));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
        #ifndef PTV
                    fclose (fstream);
        #else
                    close (fd);
        #endif /* !PTV */
                }
                break;
            }
            case STGXOBJ_COLOR_TYPE_CLUT2:
            {
                /* Palette size is 4*4, ARGB8888 for a CLUT2 bitmap        */
#ifdef ST_OSLINUX
#ifndef PTV
                size = fwrite((void *)Palette_p->Data_p, 1,(4*4), fstream);
#else
                size = write(fd, (void *)Palette_p->Data_p, (4*4));
#endif /* !PTV */
#else
                size = fwrite((void *)STAVMEM_VirtualToCPU((void*)Palette_p->Data_p,&VirtualMapping), 1,(4*4), fstream);
#endif
                if (size != (4*4))
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Write Palette error %d byte instead of %d\n",size,(4*4)));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
        #ifndef PTV
                    fclose (fstream);
        #else
                    close (fd);
        #endif /* !PTV */
                }
                break;
            }
        }
    }

    /* Write data                                                    */
    if (ErrCode == ST_NO_ERROR)
    {
#ifdef  ST_OSLINUX
        STTBX_Print(("Saving bitmap data ..at 0x%x..\n",  Bitmap_p->Data1_p));
#else
        STTBX_Print(("Saving bitmap data ...\n"));
#endif

#ifdef ST_OSLINUX
#ifndef PTV
        size = fwrite((void *)Bitmap_p->Data1_p, 1,(Bitmap_p->Size1), fstream);
#else
                size = write(fd, (void *)Bitmap_p->Data1_p, (Bitmap_p->Size1));
#endif /* !PTV */

#else
        size = fwrite((void *)STAVMEM_VirtualToCPU((void*)Bitmap_p->Data1_p,&VirtualMapping), 1,(Bitmap_p->Size1), fstream);
#endif
        if (size != (Bitmap_p->Size1))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Write Bitmap error %d byte instead of %d\n",size,(Bitmap_p->Size1)));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }

        /* Close stream */
#ifndef PTV
        fclose (fstream);
#else
        close (fd);
#endif /* !PTV */
    }

    return(ErrCode);

}
