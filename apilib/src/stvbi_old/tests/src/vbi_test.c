/************************************************************************

File name   : vbi_test.c

Description : - Unitary test functions for TTX
              - Unitary test functions for CC
              - Stack use metrics functions

COPYRIGHT (C) STMicroelectronics 2002

Date          Modification                                    Initials
----          ------------                                    --------
05 Oct 2001   Creation                                        BS
14 Jun 2002   Update for use with STAPIGAT                    HSdLM
14 May 2003   Add support for 7020stem                        HSdLM
02 Jan 2005   Add Support of OS21/ST40                        AC
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

#if (defined (ST_7100) || defined(ST_7109)||defined (ST_7200)) && defined(ST_OSLINUX)
#define USE_AVMEM_ALLOC
#endif
/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ST_OSLINUX
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include "stddefs.h"
#include "stdevice.h"
#ifdef ST_OS21
#include "os21debug.h"
#include <fcntl.h>
#endif
#ifdef ST_OS20
#include "debug.h"
#endif

#include "testcfg.h"
#include "stos.h"
#include "stcommon.h"
#include "stsys.h"
#ifdef ST_OSLINUX
    #include "iocstapigat.h"
/*#else*/
#endif

#include "sttbx.h"
#include "testtool.h"
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
#include "sti2c.h"
#include "cli2c.h"
#endif /* I2C */
#include "stvbi.h"
#include "startup.h"
#include "api_cmd.h"
#include "denc_cmd.h"
#include "vbi_cmd.h"
#if defined(mb376) || defined(espresso)
    #include "sti4629.h"
#endif /*mb376*/
#if defined (ST_5105)|| defined (ST_5525)|| defined(ST_5188)||  defined(ST_5107)
#include "stvmix.h"
#endif

#ifdef USE_AVMEM_ALLOC
#if !defined(ST_OSLINUX)
    #include "stavmem.h"
    #include "clavmem.h"
#endif
#endif

#ifdef ST_OSLINUX
extern ST_ErrorCode_t STVBI_AllocData( STVBI_Handle_t  LayerHandle, STVBI_AllocDataParams_t *Params_p, void **Address_p );
#endif

/* work-arounds STi5514  */
#if defined (ST_5514)
#define WA_GNBvd11019 /* Create a Ram image of all DENC registers, THIS MUST BE IN LINE WITH STDENC */
#endif /* ST_5514 */

extern STVBI_Handle_t VBIHndl;
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
extern STI2C_Handle_t VBI_I2CHndl;
#endif /* USE_I2C USE_STDENC_I2C */
/*extern BOOL VBI_TextError(ST_ErrorCode_t Error, char *Text);*/
#ifdef ST_OSLINUX
pthread_t                       CCthread_p;
#endif

BOOL Test_CmdStart(void);    /* Test driver specific */
/* Private Constants -------------------------------------------------------- */

#if defined (ST_5508) || defined (ST_5518) || defined (ST_5510) || defined (ST_5512) || \
    defined (ST_5516) || ((defined (ST_5514) || defined (ST_5517)) && !defined(ST_7020))
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           0
#elif defined (ST_7015)
#define DNC_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define DNC_BASE_ADDRESS         ST7015_DENC_OFFSET
#define DENC_REG_SHIFT           0
#elif defined (ST_7020)
#define DNC_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define DNC_BASE_ADDRESS         ST7020_DENC_OFFSET
#define DENC_REG_SHIFT           0
#elif defined (ST_GX1)
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         0xBB430000 /* different from value given in vob chip DENC_BASE_ADDRESS */
#define DENC_REG_SHIFT           1
#elif defined (ST_5528)
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           2
#elif defined (ST_5100) || defined (ST_5301)
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           2
#elif defined(ST_5105) || defined(ST_5525)|| defined(ST_5188)||  defined(ST_5107)
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           2
#define VBI_LIGN_LENGTH         48
#elif defined (ST_7710)|| defined (ST_7100)|| defined (ST_7109)||defined (ST_7200)
#define DNC_DEVICE_BASE_ADDRESS  0
#ifndef ST_OSLINUX
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#endif
#define DENC_REG_SHIFT           2
#else
#error ERROR:no DVD_FRONTEND defined
#endif
#if defined(mb376) || defined(espresso)
     #include "sti4629.h"
    #define STI4629_DNC_BASE_ADDRESS ST4629_DENC_OFFSET
    #ifdef ST_OS21
    #define ST4629_BASE_ADDRESS  STI4629_ST40_BASE_ADDRESS
    #endif /* ST_OS21 */
    #ifdef ST_OS20
    #define ST4629_BASE_ADDRESS  STI4629_BASE_ADDRESS
    #endif /* ST_OS20 */
    #define REG_SHIFT_4629           0
#endif /* mb376 */
#define VBI_MAX_WRITE_DATA      19  /* Max is for MV */
#define I2C_TIMEOUT             1000
#define I2C_STV119_ADDRESS      0x40 /* on mb295 */
#define STVBI_MAX_OPEN          3
#define DENC_REG_SIZE           100

#define CCBYTE_ARRAY_MAX_SIZE   512
#define CC_MAX_TASK             2
#define FILEMAXSIZE 8024
#if defined (ST_OS21) || defined (ST_OSLINUX)
#define CCTASK_WORKSPACE        1024*16
#endif
#ifdef ST_OS20
#define CCTASK_WORKSPACE  4096
#endif

#if (defined (ST_7100) || defined (ST_7109) || defined (ST_7200)) && defined(ST_OSLINUX)
/*extern MapParams_t   DencMap;
extern MapParams_t   CompoMap;
extern MapParams_t   OutputStageMap;   */
#define VOS_BASE_ADDRESS_1       OutputStageMap.MappedBaseAddress
#define DNC_BASE_ADDRESS        DencMap.MappedBaseAddress
#define VMIX1_BASE_ADDRESS       CompoMap.MappedBaseAddress + 0xC00
#endif

#if !defined(ST_OSLINUX) && (defined(ST_7100) || defined(ST_7109) || defined (ST_7200))
#if defined (ST_7100)
#define VMIX1_BASE_ADDRESS       ST7100_VMIX1_BASE_ADDRESS
#elif defined (ST_7109)
#define VMIX1_BASE_ADDRESS       ST7109_VMIX1_BASE_ADDRESS
#elif defined (ST_7200)
#define VMIX1_BASE_ADDRESS       ST7200_VMIX1_BASE_ADDRESS
#endif
#define VOS_BASE_ADDRESS_1       VOS_BASE_ADDRESS
#endif /*** ! ST_OSLINUX for STi7100 & STi7109 ***/


/* Define all vbi for register check */
#define TYPE_TTX                0
#define TYPE_CC                 1
#define TYPE_WSS                2
#define TYPE_CGMS               3
#define TYPE_VPS                4
#define TYPE_MV                 5
#define TYPE_LAST               6

enum
{
    DENC_ACCESS_8BITS,
    DENC_ACCESS_32BITS,
    DENC_ACCESS_I2C
};

/* Private Types ------------------------------------------------------------ */

typedef struct CC_Task_s
{
    U8 Stack[CCTASK_WORKSPACE];
    task_t * CCTid;
    task_t * taskCC_p;
    U32 Loop;
    BOOL End;
    STVBI_Handle_t Handle;
    U32 NoTask;
    unsigned char CCBytesArray[CCBYTE_ARRAY_MAX_SIZE]; /* size of closed caption "injection" task buffer */
    S32 CC_NbBytes;
} CC_Task_t;

#if defined (ST_5105)|| defined (ST_5525) || defined(ST_5188)||  defined(ST_5107)
typedef struct InfoTtxTsk_s
{
    U32              Loop;
    BOOL             Terminate;
    char*            Buffer;
    char*            AlignBuffer;
    unsigned long    Size;
    task_t *         TskId_p;
    U32*             TTXBaseAdress;
    STVMIX_VBIViewPortHandle_t  VBI_ODDVPHdle;
    STVMIX_VBIViewPortHandle_t  VBI_EVENVPHdle;
    STVMIX_Handle_t   VMIXHandle;
} InfoTtxTsk_t;
#endif

/* Private Variables (static)------------------------------------------------ */

static U8 DencAccessType = DENC_ACCESS_8BITS;
static CC_Task_t CCTaskArray[CC_MAX_TASK];
static char VBI_Msg[200];            /* text for trace */
static U8 Denc_AddRegister[DENC_REG_SIZE]; /* To read and memorize denc values + Address */
static U8* Denc_Register = Denc_AddRegister;

#ifdef WA_GNBvd11019
static U32 WaitOneFrame = 0;
#endif /* WA_GNBvd11019 */

/* Check register variables */
static const U8 VBI_Mask[TYPE_LAST][VBI_MAX_WRITE_DATA] = {
    {0xE0, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,        /* TTX  */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* TTX  */
    {0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x1F, 0xFF, 0xFF, 0xFF,        /* CC   */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* CC   */
    {0x1F, 0x1C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,        /* WSS  */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* WSS  */
    {0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,        /* CGMS */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* CGMS */
    {0xCF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,        /* VPS  */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* VPS  */
    {0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x3F, 0x3F, 0x7F,        /* MV   */
    0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0}   /* MV   */
};

/* Metrics variables */
#ifdef ARCHITECTURE_ST20
#define STACK_SIZE          4096 /* STACKSIZE must oversize the estimated stack usage of the driver */
static STVBI_InitParams_t InitVBIParam;
static STVBI_OpenParams_t OpenVBIParam;
static STVBI_TermParams_t TermVBIParam;
static STVBI_Handle_t VBIHandle;
static STVBI_Configuration_t ConfigVBIParam;
static STVBI_DynamicParams_t DynamicVBIParams;
static STVBI_Capability_t VBICapability;
static STVBI_TeletextSource_t VBITeletextSource;
static U8    Data[17];
#endif /* ARCHITECTURE_ST20 */
#if defined (ST_5105)|| defined (ST_5525)|| defined(ST_5188)||  defined(ST_5107)
static STVMIX_Handle_t   VMIXHandle,VMIXHandle2;
static InfoTtxTsk_t      TtxInfo,TtxInfo2; /* For teletext task */

static void VBI_TeletextTsk(InfoTtxTsk_t * Infos);


#endif

/* Global Variables --------------------------------------------------------- */

static tdesc_t   TaskDesc;
static void *    TaskStack;


/* Private Macros ----------------------------------------------------------- */

#define VBI_PRINT(x) { \
    /* Check lenght */ \
    if (strlen(x)>sizeof(VBI_Msg)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

#define CAST_CONST_HANDLE_TO_DATA_TYPE( DataType, Handle )  \
    ((DataType)(*(DataType*)((void*)&(Handle))))
/* Private Function prototypes ---------------------------------------------- */
#ifdef ARCHITECTURE_ST20
static void ReportError(int Error, char *FunctionName);
static void test_overhead(void *dummy);
static void test_typical(void *dummy);
#endif
void os20_main(void *ptr);
#ifdef ST_OS21
extern int close(int fd);
extern  ssize_t read(int fd, void *buffer, size_t n);
#endif

/* Functions ---------------------------------------------------------------- */


/*#######################################################################*/
/*########################## CC TESTS COMMANDS ##########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : DoCCInject
 *            Function is called as subtask
 * Input    : Loop_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static void DoCCInject( void * TaskParams_p )
{
    U32 CCDataCnt, i;
    U32 CCDataCur = 0 ;
    BOOL ErrorRaised = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U8  CCData[2];
    U32 NoTask = *((U32*)TaskParams_p);

    CCDataCnt = CCTaskArray[NoTask].CC_NbBytes / 2;
    for (i=0 ; ((!CCTaskArray[NoTask].End)
                && ((CCTaskArray[NoTask].Loop) > 0)
                && (!ErrorRaised)); (CCTaskArray[NoTask].Loop)--,i++ )
    {
        for (CCDataCur=0; CCDataCur<CCDataCnt && (!ErrorRaised); CCDataCur++)
        {
            CCData[0] = CCTaskArray[NoTask].CCBytesArray[2*CCDataCur];
            CCData[1] = CCTaskArray[NoTask].CCBytesArray[2*CCDataCur+1];
            ErrorCode = STVBI_WriteData( CCTaskArray[NoTask].Handle, CCData, 2);
            ErrorRaised = (ErrorCode != ST_NO_ERROR);
#ifdef WA_GNBvd11019
            STOS_TaskDelay(WaitOneFrame);
#endif /* WA_GNBvd11019 */
        }
    }

    if (ErrorRaised)
    {
        sprintf(VBI_Msg, "DoCCInject(NbLoop=%d,NoCCData=%d) :", i,CCDataCur );
        VBI_TextError(ErrorCode, VBI_Msg);
    }
} /* end DoCCInject */


/*-------------------------------------------------------------------------
 * Function : CC_LoadData
 *            Load Closed Caption Data to inject
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL CC_LoadData( STTST_Parse_t *pars_p, char *Result )
{
    #define ST40_MASK                       0x1FFFFFFF

    char FileName[127];
    char  *ptrstart, *ptrend;
    void *CC_FileBuffer = NULL;
    BOOL RetErr;
#ifndef ST_OSLINUX
     long FileDescriptor;
     char *AllocatedBuffer = NULL;
     char Msg[512];
#endif
    long  FileSize;
    S32 NbBytes = 0;
    S32 NoTask;
    #if defined (ST_OS21)
    struct stat FileStatus;
    #endif
#ifdef USE_AVMEM_ALLOC
    FILE * fstream_p;
    U32 size;

#ifdef  ST_OSLINUX

    STVBI_AllocDataParams_t VbiAllocData;

#else

    STAVMEM_AllocBlockParams_t AllocBlockParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    STAVMEM_BlockHandle_t BlockHandle;
#endif

#endif

    UNUSED_PARAMETER(Result);
    /* get file name */
    memset( FileName, 0, sizeof(FileName));
    RetErr = STTST_GetString( pars_p, "", FileName, sizeof(FileName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected FileName" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, &NoTask);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected no task (default 0)" );
        return(TRUE);
    }

#ifdef USE_AVMEM_ALLOC

        fstream_p = fopen(FileName, "rb");
        if( fstream_p == NULL )
        {
            STTST_Print(("Not unable to open !!!\n"));
            return(TRUE);

        }
#if !defined(ST_OSLINUX)
        AllocBlockParams.PartitionHandle          = AvmemPartitionHandle[0];
        AllocBlockParams.Size                     = 336;
        AllocBlockParams.Alignment                = 2048;
        AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocBlockParams.NumberOfForbiddenRanges  = 0;
        AllocBlockParams.ForbiddenRangeArray_p    = NULL;
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;
        if(STAVMEM_AllocBlock (&AllocBlockParams,&(BlockHandle)) != ST_NO_ERROR)
        {
            STTST_Print(("Cannot allocate in avmem !! \n"));
           return(TRUE);

        }
        STAVMEM_GetBlockAddress( BlockHandle, &CC_FileBuffer);
        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
        STTST_Print(("Reading data ..."));
        size = fread (STAVMEM_VirtualToCPU(CC_FileBuffer,&VirtualMapping), 1, AllocBlockParams.Size, fstream_p);
        printf("!!! SIZE == %d \n",size);
        if (size != AllocBlockParams.Size)
        {
            STTST_Print(("Read error %d byte instead of %d !!!\n"));

          /* return(TRUE);*/

        }
        else
        {
            STTST_Print(("ok\n"));
       }

#else

        VbiAllocData.Size = 336 ;
        VbiAllocData.Alignment = 256 ;
        STVBI_AllocData( VBIHndl, &VbiAllocData,
                            &CC_FileBuffer );

        if ( CC_FileBuffer == NULL )
        {
            STTBX_Print(("Error Allocating data for palette...\n"));
            return ST_ERROR_BAD_PARAMETER;
        }

        STTBX_Print(("Reading file Bitmap data Address data bitmap = 0x%x... \n", (U32)CC_FileBuffer ));
        size = fread ((void*)(CC_FileBuffer),1,
                      VbiAllocData.Size, fstream_p);

#endif
     /* CC_FileBuffer = (void*) ((U32) STAVMEM_UserToKernel((U32)CC_FileBuffer) & MASK_STBUS_FROM_ST40);*/
      FileSize = size;
#else
    #ifdef ST_OS20
    FileDescriptor = debugopen(FileName, "r" );
    #endif
    #if defined (ST_OS21)
/*    FileDescriptor = open(FileName, O_RDONLY|O_BINARY); */
     FileDescriptor = open(FileName, O_RDONLY);
    #endif
    if (FileDescriptor == -1 )
    {
        sprintf(Msg, "unable to open %s ...\n", FileName);
        STTBX_Print((Msg));
        return(TRUE);
    }
    #ifdef ST_OS20
    FileSize = debugfilesize(FileDescriptor);
    #endif
    #if defined (ST_OS21)
    fstat(FileDescriptor,&FileStatus);
    FileSize = FileStatus.st_size;
    #endif
    /* Get memory to store file data */
    /* ---  round up "size" to a multiple of 16 bytes, and malloc it --- */
    AllocatedBuffer = (char *)memory_allocate(DriverPartition_p, (U32)((FileSize +1 + 15) & (~15)) );
    if (AllocatedBuffer == NULL)
    {
        STTBX_Print(( "Not enough memory for file loading !\n"));
        #if defined (ST_OS20)
        debugclose(FileDescriptor);
        #elif  defined (ST_OS21)
        close(FileDescriptor);
        #endif
        return(TRUE);
    }

    /* Align buffer */
    CC_FileBuffer = (void *)(((U32)AllocatedBuffer + 15) & (U32)(~15));

    #ifdef ST_OS20
     debugread(FileDescriptor, CC_FileBuffer, (size_t) FileSize);
     *((char*)CC_FileBuffer+FileSize) = 0;
     debugclose(FileDescriptor);
    #endif

    #if defined (ST_OS21)
     read(FileDescriptor, CC_FileBuffer, (size_t) FileSize);
     *((char*)CC_FileBuffer+FileSize) = 0;
     close(FileDescriptor);
    #endif
#endif /** USE_AVMEM_ALLOC **/
    sprintf(VBI_Msg, "file [%s] loaded : size %ld at address %x \n",
            FileName, FileSize, (int)CC_FileBuffer);
    VBI_PRINT(VBI_Msg);
    /* parse the characters read to build the CC data array */
    NbBytes = 0;
    ptrstart = 0;
    ptrend=(char*)CC_FileBuffer;
    while ((ptrstart != ptrend) && (NbBytes < CCBYTE_ARRAY_MAX_SIZE))
    {
        ptrstart=ptrend;
        CCTaskArray[NoTask].CCBytesArray[NbBytes++] = (unsigned char)strtoul(ptrstart, &ptrend, 16);
    }
    if (NbBytes >= CCBYTE_ARRAY_MAX_SIZE)
    {
        sprintf(VBI_Msg, "File too large to fit in local buffer. MaxSize : %d...\n", CCBYTE_ARRAY_MAX_SIZE);
        VBI_PRINT(VBI_Msg);
    }
    else
        CCTaskArray[NoTask].CC_NbBytes = NbBytes;
#ifndef ST_OSLINUX
    if (AllocatedBuffer != NULL)
    {
        memory_deallocate(DriverPartition_p, AllocatedBuffer);
    }
#endif
    return FALSE;
} /* end CC_LoadData */


/*-------------------------------------------------------------------------
 * Function : CC_KillTask
 *            Kill Closed Caption Inject task
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL CC_KillTask( STTST_Parse_t *pars_p, char *Result )
{


    task_t *TaskList[1];
    STVBI_Handle_t Handle;
    U32 NoTask=0;
    ST_ErrorCode_t RetErr;

    UNUSED_PARAMETER(Result);

    RetErr = STTST_GetInteger( pars_p, VBIHndl, (S32*)&Handle);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected handle" );
        return(TRUE);
    }

    while ((CCTaskArray[NoTask].Handle != Handle) && (CC_MAX_TASK != NoTask))
    {
        NoTask++;
    }

    if (NoTask == CC_MAX_TASK)
    {
        STTBX_Print(("Cannot find task associated with this handle\n"));
        return(TRUE);
    }

    /* Reset global variable and wait for task to finish before deleting */
    CCTaskArray[NoTask].End = TRUE;

    if ( CCTaskArray[NoTask].CCTid != NULL )
    {
        TaskList[0] = CCTaskArray[NoTask].CCTid;
        if (STOS_TaskWait ( TaskList, TIMEOUT_INFINITY ))
        {
            STTBX_Print(("Error: Timeout task_wait\n"));
        }
        else
        {
            STTBX_Print(("CC injection Task finished\n"));
           #if defined (ST_OSLINUX)
                pthread_join(*TaskList[0], NULL);
                CCTaskArray[NoTask].CCTid = NULL;
                CCTaskArray[NoTask].NoTask = (U32)-1;

           #else
            if ( STOS_TaskDelete( CCTaskArray[NoTask].CCTid ,DriverPartition_p, TaskStack, DriverPartition_p)  )
            {
                STTBX_Print(("Error: couldn't delete CC injection task.\n"));
            }
            else
            {
                CCTaskArray[NoTask].CCTid = NULL;
                CCTaskArray[NoTask].NoTask = (U32)-1;
            }

           #endif


        }
    }

    return (FALSE);
} /* end CC_KillTask */



/*-------------------------------------------------------------------------
 * Function : CC_MemInject
 *            Inject Closed Caption data from memory (CCBytesArray) to VBI
 * Input    : *pars_p, *Result_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL CC_MemInject( STTST_Parse_t *pars_p, char *Result )
{
    BOOL NotSubTask;
    BOOL RetErr;
    U32 NoTask=0;
    ST_ErrorCode_t          Error = ST_NO_ERROR;

    UNUSED_PARAMETER(Result);


    while ((CCTaskArray[NoTask].CCTid != NULL) && (CC_MAX_TASK != NoTask))
        NoTask++;



    if ( CCTaskArray[NoTask].CC_NbBytes<=0 )
    {
        STTBX_Print(("No CC data in memory\n"));
        return TRUE;
    }

    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&CCTaskArray[NoTask].Loop );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected number of loops" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, FALSE, (S32*)&NotSubTask);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected subtask creation (Default Yes=False, No=TRUE)");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, VBIHndl, (S32*)&(CCTaskArray[NoTask].Handle) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected handle" );
        return(TRUE);
    }
#ifndef ST_OSLINUX
    if ( CCTaskArray[NoTask].CCTid != NULL )
    {
        STTBX_Print(("CC injection task already running\n"));
        return TRUE;
    }
#endif

    /* Run task */
    CCTaskArray[NoTask].End = FALSE;
    CCTaskArray[NoTask].NoTask = NoTask;

    if (!NotSubTask)
    {
        RetErr = STOS_TaskCreate ((void (*) (void*))DoCCInject,
                                   (void *) &CCTaskArray[NoTask].NoTask,
                                   DriverPartition_p,
                                   CCTASK_WORKSPACE,
                                   &TaskStack,
                                   DriverPartition_p,
                                   &(CCTaskArray[NoTask].taskCC_p),
                                   &TaskDesc,
                                   MIN_USER_PRIORITY,
                                   "CCInject",
                                   0 /*task_flags_high_priority_process*/);

    if(Error != ST_NO_ERROR)
    {
        STTBX_Print(("Problem in the creation of task\n"));
        return(TRUE);
    }

            if ( CCTaskArray[NoTask].taskCC_p == NULL )
            {
                 CCTaskArray[NoTask].CCTid = NULL;
            }
            else
            {
                 CCTaskArray[NoTask].CCTid = CCTaskArray[NoTask].taskCC_p;
            }
            if ( CCTaskArray[NoTask].CCTid == NULL )
            {
                STTBX_Print(("Unable to Create CC inject task\n"));
            }
            else
            {
                STTBX_Print(("Inject CC data %d times\n", CCTaskArray[NoTask].Loop ));
            }

    }
    else
    {
        DoCCInject((void *) &CCTaskArray[NoTask].NoTask);
    }
    return(FALSE);
} /* End CC_MemInject */


/*#######################################################################*/
/*######################### VBI OTHER COMMANDS ##########################*/
/*#######################################################################*/


/*-------------------------------------------------------------------------
 * Function : VBI_FullRegisterCheck
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_FullRegisterCheck(STTST_Parse_t *pars_p, char *Result)
{
    BOOL  RetErr, ErrFound=FALSE;
    S32   Address, ExpectedValue, ReadValue, MaskLong;
    U8    Mask, NbError=0;

    UNUSED_PARAMETER(Result);

    RetErr = STTST_GetInteger(pars_p, (S32)0xBB450008, (S32*)&Address);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected address");
    }
    if (!RetErr)
    {
        RetErr = STTST_GetInteger(pars_p, 0, &ExpectedValue);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected value");
        }
    }
    if (!RetErr)
    {
        RetErr = STTST_GetInteger(pars_p, 0xFF, &MaskLong);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected mask");
        }
    }
    if (!RetErr)
    {
        Mask=(U8)MaskLong;
        ReadValue = STSYS_ReadRegDev32LE(Address);

        sprintf(VBI_Msg, "VBI_FullRegisterCheck(h%0X, h%02X): ", Address, ExpectedValue);

        /* Compare values according to selected VBI */
        if ((ReadValue & Mask) != (ExpectedValue & Mask))
        {
            strcat(VBI_Msg, "Failed !!!\n");
            VBI_PRINT(VBI_Msg);
            ErrFound=TRUE;
            sprintf(VBI_Msg, "Add h%0X  Read: h%02X  Expec: h%02X  Mask: h%02X\n", \
                    Address, ReadValue & Mask, ExpectedValue & Mask, Mask);
            VBI_PRINT(VBI_Msg);
            NbError++;
        }

        if (!ErrFound)
        {
            strcat(VBI_Msg, "Ok\n");
            VBI_PRINT(VBI_Msg);
        }
        STTST_AssignInteger("ERRORCODE", NbError, FALSE);
    }
    return(API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : denc_read
 * Input    : Address, length, buffer
 * Output   : None
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL denc_read(U8 add, U8* value, U8 lenght, U8 IsExternal)
{
    U8      i;
    U8*     Reg_Address;
    U8      DencAccessType_Temp;
    U8*     BaseAddress;
    U8      RegShift;

#if defined (USE_I2C) && defined (USE_STDENC_I2C)
    ST_ErrorCode_t      ErrCode;
    U32                 ActLen;
#endif  /* #ifdef USE_I2C */

#if defined(mb376) || defined(espresso)
    /* 8bits for STi4629 and 32bits for STi5528 */
    DencAccessType_Temp = (IsExternal==1) ? DENC_ACCESS_8BITS : DENC_ACCESS_32BITS ;
    BaseAddress         = (IsExternal==1) ? (U8*) ST4629_BASE_ADDRESS + ST4629_DENC_OFFSET :
                                            (U8*)DNC_BASE_ADDRESS + DNC_DEVICE_BASE_ADDRESS ;
    RegShift            = (IsExternal==1) ? 0 : DENC_REG_SHIFT;
#else
    IsExternal = 0;                       /* no external chip */
    DencAccessType_Temp = DencAccessType;
    BaseAddress         = (U8*)DNC_BASE_ADDRESS + DNC_DEVICE_BASE_ADDRESS;
    RegShift            = DENC_REG_SHIFT;
#endif

    switch(DencAccessType_Temp)
    {
       case DENC_ACCESS_32BITS:
            for (i=0; i<lenght; i++, value++)
            {
                Reg_Address  = BaseAddress + ((i+add)<<RegShift);
                *value = (U8)STSYS_ReadRegDev32LE((void*)Reg_Address);
            }
            break;

#if defined (USE_I2C) && defined (USE_STDENC_I2C)
        case DENC_ACCESS_I2C:
            if (VBI_I2CHndl == -1){
                STI2C_OpenParams_t  OpenParams;

                OpenParams.I2cAddress        = I2C_STV119_ADDRESS;
                OpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
                OpenParams.BusAccessTimeOut  = 100000;
                ErrCode=STI2C_Open(STI2C_BACK_DEVICE_NAME, &OpenParams, &VBI_I2CHndl);
                if (ErrCode != ST_NO_ERROR){
                    STTST_Print(("Failed to open back I2C driver\n"));
                    return(TRUE);
                }
            }

            ErrCode=STI2C_Write(VBI_I2CHndl, &add, 1, I2C_TIMEOUT, &ActLen);
            if (ErrCode != ST_NO_ERROR)
                return(TRUE);

            ErrCode=STI2C_Read(VBI_I2CHndl, value, lenght, I2C_TIMEOUT, &ActLen);
            if (ErrCode != ST_NO_ERROR)
                return(TRUE);
            break;
#endif  /* #ifdef USE_I2C */

         case DENC_ACCESS_8BITS:
            for (i=0; i<lenght; i++, value++)
                *value=STSYS_ReadRegDev8((void*)(BaseAddress+add+i));
            break;
        default:
            STTST_Print(("Denc access not provided !!!\n"));
            return(TRUE);
            break;
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : denc_write
 * Input    : Address, length, buffer
 * Output   : None
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL denc_write(U8 add, U8* value, U8 lenght, U8 IsExternal)
{
    U8 i;
    U8      DencAccessType_Temp;
    U8*     BaseAddress;
    U8      RegShift;
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
    ST_ErrorCode_t      ErrCode;
    U32                 ActLen;
    U8                  SavValue;
#endif  /* #ifdef USE_I2C */

#if defined(mb376) || defined(espresso)
    /* 8bits for STi4629 and 32bits for STi5528 */
    DencAccessType_Temp = (IsExternal==1) ? DENC_ACCESS_8BITS : DENC_ACCESS_32BITS ;
    BaseAddress         = (IsExternal==1) ? (U8*) ST4629_BASE_ADDRESS + ST4629_DENC_OFFSET :
                                            (U8*)DNC_BASE_ADDRESS + DNC_DEVICE_BASE_ADDRESS ;
    RegShift            = (IsExternal==1) ? 0 : DENC_REG_SHIFT;
#else
    IsExternal = 0;                       /* no external chip */
    DencAccessType_Temp = DencAccessType;
    BaseAddress         = (U8*)DNC_BASE_ADDRESS + DNC_DEVICE_BASE_ADDRESS;
    RegShift            = DENC_REG_SHIFT;
#endif
    switch(DencAccessType_Temp)
    {
        case DENC_ACCESS_32BITS:

            for (i=0; i<lenght; i++, value++)
            {
                 STSYS_WriteRegDev32LE(((U32)(BaseAddress +((i+add)<<RegShift ))),*value);
            }
            break;

#if defined (USE_I2C) && defined (USE_STDENC_I2C)
        case DENC_ACCESS_I2C:
            if (VBI_I2CHndl == -1){
                STI2C_OpenParams_t  OpenParams;

                OpenParams.I2cAddress        = I2C_STV119_ADDRESS;
                OpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
                OpenParams.BusAccessTimeOut  = 100000;
                ErrCode=STI2C_Open(STI2C_BACK_DEVICE_NAME, &OpenParams, &VBI_I2CHndl);
                if (ErrCode != ST_NO_ERROR){
                    STTST_Print(("Failed to open back I2C driver\n"));
                    return(TRUE);
                }
            }
            value--;
            SavValue = *value;
            *value=add;

            ErrCode=STI2C_Write(VBI_I2CHndl, value, lenght+1, I2C_TIMEOUT, &ActLen);
            *value=SavValue;
            value++;
            if (ErrCode != ST_NO_ERROR)
                return(TRUE);
            break;
#endif  /* #ifdef USE_I2C */
        case DENC_ACCESS_8BITS:

            for (i=0; i<lenght; i++, value++)
                STSYS_WriteRegDev8((void*)((U8*)BaseAddress +add+i),*value);
            break;

        default:
            STTST_Print(("Denc access not provided !!!\n"));
            return(TRUE);
            break;
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VBI_Access
 * Input    : *pars_p, *result_sym_p
 * Output   : None
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_Access(STTST_Parse_t *pars_p, char *result_sym_p){

    BOOL RetErr;
    U32 LVar;

    UNUSED_PARAMETER(result_sym_p);

#if defined(ST_7015)|| defined(ST_7020)|| defined(ST_GX1) || defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710) \
 || defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)|| defined(ST_5525)|| defined(ST_5188) \
 || defined(ST_5107)|| defined (ST_7200)
    RetErr = STTST_GetInteger(pars_p, DENC_ACCESS_32BITS, (S32 *)&LVar);
#else
    RetErr = STTST_GetInteger(pars_p, DENC_ACCESS_8BITS, (S32 *)&LVar) ;
#endif
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected Access Type (0=EMI8Bits,1=EMI32Bits,2=I2C)");
    }
    else
    {
        DencAccessType=(U8)LVar;
        sprintf(VBI_Msg, "VBI_Access(%s): Ok\n",
                (DencAccessType == DENC_ACCESS_8BITS) ? "EMI 8Bits":
                (DencAccessType == DENC_ACCESS_32BITS) ? "EMI 32Bits":
                (DencAccessType == DENC_ACCESS_I2C) ? "I2C": "????" );
        VBI_PRINT(VBI_Msg);
    }
    return(API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VBI_ReadRegister
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_RegisterCheck(STTST_Parse_t *pars_p, char *result_sym_p)
{

    char TrvStr[80], *ptr;
    BOOL  RetErr, ErrFound=FALSE;
    S32   Address=-1, NbData, Index=0, Value, MaskLong;
    U8    CmpData[VBI_MAX_WRITE_DATA], Mask, NbError=0, Type=255;
    S32 IsExternal;

    UNUSED_PARAMETER(result_sym_p);


    STTST_GetItem(pars_p, "0", TrvStr, sizeof(TrvStr));

    if (strcmp("TTX", TrvStr)==0)
    {
#if defined(ST_5510)
        Address=64; NbData=0; Type=TYPE_TTX;
#else /* ST5510 */
        if (DencAccessType == DENC_ACCESS_I2C)
        {
            Address=64; NbData=0; Type=TYPE_TTX;
        }
        else
        {
            Address=64; NbData=2; Type=TYPE_TTX;
        }
#endif /* V10,... */
    }

    if (strcmp("CC", TrvStr)==0)
    {
        Address=39; NbData=6; Type=TYPE_CC;
    }
    if (strcmp("WSS", TrvStr)==0)
    {
        Address=15; NbData=2; Type=TYPE_WSS;
    }
    if (strcmp("CGMS", TrvStr)==0)
    {
        Address=31; NbData=3; Type=TYPE_CGMS;
    }
    if (strcmp("VPS", TrvStr)==0)
    {
        Address=25; NbData=6; Type=TYPE_VPS;
    }
    if (strcmp("MV", TrvStr)==0)
    {
        Address=45; NbData=19; Type=TYPE_MV;
    }

    RetErr = STTST_GetInteger(pars_p, 0,&IsExternal);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected ExternalChip (0=internal/1=external)");
            return(API_EnableError);
        }

    if (Address == -1)
    {
        Address = (S16)strtoul(TrvStr, &ptr, 10);
        if (ptr == TrvStr)
        {
            STTST_TagCurrentLine(pars_p, "Expected VBI symbol or address value");
            return(API_EnableError);
        }


        RetErr = STTST_GetInteger(pars_p, 8, (S32*)&NbData);
        if ((RetErr) || (NbData > VBI_MAX_WRITE_DATA))
        {
            STTST_TagCurrentLine(pars_p, "Expected number of values");
            return(API_EnableError);
        }
        sprintf(TrvStr, "Add: %02d", Address);
    }



    while (Index != NbData){
        RetErr = STTST_GetInteger(pars_p, 0, &Value);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected value");
            return(API_EnableError);
        }
        CmpData[Index]=(U8)Value;
        Index++;
    }

    RetErr = STTST_GetInteger(pars_p, 0xFF, &MaskLong);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected Mask");
        return(API_EnableError);
    }
    Mask=(U8)MaskLong;

    if (denc_read(Address, &Denc_Register[Address], NbData, IsExternal)==TRUE)
    {
        return(API_EnableError);
    }

    sprintf(VBI_Msg, "VBI_RegisterCheck(%s", TrvStr);
    for (Index=0; Index<NbData; Index++)
    {
        sprintf(VBI_Msg, "%s,h%02X", VBI_Msg, CmpData[Index]);
    }
    strcat(VBI_Msg, "): ");

    /* Compare values according to selected VBI */
    if (Type<TYPE_LAST)
    {
        for (Index=0; Index<NbData; Index++)
        {
            Mask=VBI_Mask[Type][Index];
            if ((Denc_Register[Index+Address] & Mask) != (CmpData[Index] & Mask))
            {
                if (!ErrFound)
                {
                    strcat(VBI_Msg, "Failed !!!\n");
                    VBI_PRINT(VBI_Msg);
                    ErrFound=TRUE;
                }
                sprintf(VBI_Msg, "Add %d  Read: h%02X  Expec: h%02X  Mask: h%02X\n", \
                        Address+Index, Denc_Register[Index+Address] & Mask, CmpData[Index] & Mask, Mask);
                VBI_PRINT(VBI_Msg);
                NbError++;
            }
        }
    }
    if (Type>=TYPE_LAST)
    {
        for (Index=0; Index<NbData; Index++)
        {
            if ((Denc_Register[Index+Address] & Mask) != (CmpData[Index] & Mask))
            {
                if (!ErrFound)
                {
                    strcat(VBI_Msg, "Failed !!!\n");
                    VBI_PRINT(VBI_Msg);
                    ErrFound=TRUE;
                }
                sprintf(VBI_Msg, "Add %d  Read: h%02X Expec: h%02X  Mask: h%02X\n", \
                        Address+Index, Denc_Register[Index+Address] & Mask, CmpData[Index] & Mask, Mask);
                VBI_PRINT(VBI_Msg);
                NbError++;
            }
        }
    }

    if (!ErrFound)
    {
        strcat(VBI_Msg, "Ok\n");
        VBI_PRINT(VBI_Msg);
    }
    STTST_AssignInteger("ERRORCODE", NbError, FALSE);
    return(API_EnableError ? RetErr : FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VBI_RegisterRead
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_RegisterRead(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char TrvStr[80], *ptr;
    BOOL  RetErr;
    S32   Address=-1, NbData, Index=0, EndOfLine=1;
    S32 IsExternal;

    UNUSED_PARAMETER(result_sym_p);


    STTST_GetItem(pars_p, "0", TrvStr, sizeof(TrvStr));

    if (strcmp("TTX", TrvStr)==0){
        Address=34; NbData=5;
    }
    if (strcmp("CC", TrvStr)==0){
        Address=39; NbData=6;
    }
    if (strcmp("WSS", TrvStr)==0){
        Address=15; NbData=2;
    }
    if (strcmp("CGMS", TrvStr)==0){
        Address=31; NbData=3;
    }
    if (strcmp("VPS", TrvStr)==0){
        Address=25; NbData=6;
    }
    if (strcmp("MV", TrvStr)==0){
        Address=45; NbData=19;
    }

    RetErr = STTST_GetInteger(pars_p, 0, &IsExternal);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected ExternalChip");
        return(API_EnableError);
    }

    if (Address == -1){
        Address = (S16)strtoul(TrvStr, &ptr, 10);
        if (ptr == TrvStr){
            STTST_TagCurrentLine(pars_p, "Expected VBI symbol or address value");
            return(TRUE);
        }

        RetErr = STTST_GetInteger(pars_p, 8, (S32*)&NbData);
        if (RetErr){
            STTST_TagCurrentLine(pars_p, "Expected number of value");
            return(TRUE);
        }
    }


    if (denc_read(Address, &Denc_Register[Index], NbData,IsExternal) == TRUE)
        return(TRUE);

    VBI_Msg[0]=0;

    for (Index=Address; Index<(NbData+Address); Index++, EndOfLine++)
    {
        sprintf( VBI_Msg, "%s %02d:h%02X  ", VBI_Msg, Index, Denc_Register[Index]);
        if (EndOfLine == 10)
        {
            strcat (VBI_Msg, "\n");
            EndOfLine=0;
            VBI_PRINT(VBI_Msg);
            VBI_Msg[0]=0;
        }
    }
    strcat (VBI_Msg, "\n");
    VBI_PRINT(VBI_Msg);

    return(API_EnableError ? RetErr : FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VBI_RegisterWrite
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_RegisterWrite(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char  TrvStr[80], *ptr;
    BOOL  RetErr;
    S32   Address=-1, NbData=8, Index=0, Value;
    S32 IsExternal;

    UNUSED_PARAMETER(result_sym_p);


    STTST_GetItem(pars_p, "0", TrvStr, sizeof(TrvStr));

    if (strcmp("TTX", TrvStr)==0){
        Address=34; NbData=5;
    }
    if (strcmp("CC", TrvStr)==0){
        Address=39; NbData=6;
    }
    if (strcmp("WSS", TrvStr)==0){
        Address=15; NbData=2;
    }
    if (strcmp("CGMS", TrvStr)==0){
        Address=31; NbData=3;
    }
    if (strcmp("VPS", TrvStr)==0){
        Address=25; NbData=6;
    }
    if (strcmp("MV", TrvStr)==0){
        Address=45; NbData=19;
    }

    RetErr = STTST_GetInteger(pars_p, 0, &IsExternal);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected ExternalChip");
        return(API_EnableError);
    }

    if (Address == -1){
        Address = (S16)strtoul(TrvStr, &ptr, 10);
        if (ptr == TrvStr){
            STTST_TagCurrentLine(pars_p, "Expected VBI symbol or address value");
            return(TRUE);
        }
        NbData=0;
        RetErr = STTST_GetTokenCount(pars_p, (S16*)&NbData);
        if (RetErr){
            STTST_TagCurrentLine(pars_p, "Failed to get number of items");
            return(TRUE);
        }
    }


    while (Index != NbData){
        RetErr = STTST_GetInteger(pars_p, 0, &Value);
        if (RetErr){
            STTST_TagCurrentLine(pars_p, "Expected value");
            return(TRUE);
        }
        Denc_Register[Address+Index]=(U8)Value;
        Index++;
    }


    if (denc_write(Address, &Denc_Register[Address], NbData, IsExternal) == TRUE)
    {
        return(TRUE);
    }
    return(API_EnableError ? RetErr : FALSE);
}


#ifdef ST_OS20
/*********************************************/
/* Metrics Function. Should be in vbi_test.c */
/*********************************************/

/* Functions ---------------------------------------------------------------- */
static void ReportError(int Error, char *FunctionName)
{
    if ((Error) != ST_NO_ERROR)
    {
        printf( "ERROR: %s returned %d\n", FunctionName, Error );
    }
}

/*******************************************************************************
Name        : test_typical
Description : calculates the stack usage made by the driver for its typical
              conditions of use
Parameters  : None
Assumptions : STDENC must be initialized before test metrics function call
Limitations : Make sure to not define local variables within the function
              but use module static gloabls instead in order to not pollute the
              stack usage calculation.
Returns     : None
*******************************************************************************/
void test_typical(void *dummy)
{
    ST_ErrorCode_t Err;
    UNUSED_PARAMETER(dummy);
    Err = ST_NO_ERROR;

    InitVBIParam.DeviceType = STVBI_DEVICE_TYPE_DENC;
    strcpy(InitVBIParam.Target.DencName, STDENC_DEVICE_NAME);
    InitVBIParam.MaxOpen = STVBI_MAX_OPEN;

    /* Other inits */
    Err = STVBI_Init( STVBI_DEVICE_NAME, &InitVBIParam);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_Init"));

    OpenVBIParam.Configuration.VbiType = STVBI_VBI_TYPE_CLOSEDCAPTION;
    OpenVBIParam.Configuration.Type.CC.Mode   = STVBI_CCMODE_BOTH;



    Err = STVBI_Open( STVBI_DEVICE_NAME, &OpenVBIParam, &VBIHandle);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_Open"));

    DynamicVBIParams.VbiType   = STVBI_VBI_TYPE_CLOSEDCAPTION;
    DynamicVBIParams.Type.CC.Mode  = STVBI_CCMODE_FIELD1;
    DynamicVBIParams.Type.CC.LinesField1       = 10;
    DynamicVBIParams.Type.CC.LinesField2       = 273;

    Err = STVBI_SetDynamicParams(VBIHandle, &DynamicVBIParams);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_SetDynamicParams"));

    Err = STVBI_Enable(VBIHandle);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_Enable"));


    Err = STVBI_WriteData(VBIHandle,Data, 4);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_WriteData"));


    Err = STVBI_GetCapability(STVBI_DEVICE_NAME,&VBICapability);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_GetCapability"));

    Err = STVBI_GetConfiguration(VBIHandle, &ConfigVBIParam);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_GetDynamicParams"));

    Err = STVBI_SetTeletextSource(VBIHandle,STVBI_TELETEXT_SOURCE_DMA);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_SetTeletextSource"));


    Err = STVBI_GetTeletextSource(VBIHandle,&VBITeletextSource);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_GetTeletextSource"));

    Err = STVBI_Disable(VBIHandle);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_Disable"));

    Err = STVBI_Close(VBIHandle);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_Close"));

    TermVBIParam.ForceTerminate = FALSE;
    Err = STVBI_Term( STVBI_DEVICE_NAME, &TermVBIParam);
    ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"STVBI_Term"));
}

static void test_overhead(void *dummy)
{
    ST_ErrorCode_t Err;
    Err = ST_NO_ERROR;
    UNUSED_PARAMETER(dummy);
   ReportError(Err, CAST_CONST_HANDLE_TO_DATA_TYPE(char*,"test_overhead"));
}

/*******************************************************************************
Name        : metrics_Stack_Test
Description : launch tasks to calculate the stack usage made by the driver
              for an Init Term cycle and in its typical conditions of use
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void metrics_Stack_Test(void)
{
    task_t task;
    task_t *task_p;
    task_status_t status;
    int overhead_stackused;
    char funcname[3][15]= { "test_overhead",
                            "test_typical",
                            "NULL"
                          };
    void (*func_table[])(void *) = {
        test_overhead,
        test_typical,
        NULL
    };
    void (*func)(void *);
    int i;

    task_p = &task;
    overhead_stackused = 0;
    printf("*************************************\n");
    printf("* Stack usage calculation beginning *\n");
    printf("*************************************\n");
    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];

        /* Start the task */
        task_p = task_create(func, NULL, STACK_SIZE, MAX_USER_PRIORITY, "stack_test", task_flags_no_min_stack_size);

        /* Wait for task to complete */
        task_wait(&task_p, 1, TIMEOUT_INFINITY);

        /* Dump stack usage */
        task_status(task_p, &status, task_status_flags_stack_used);
        /* store overhead value */
        if (i==0)
        {
            printf("*-----------------------------------*\n");
            overhead_stackused = status.task_stack_used;
            printf("%s \t func=0x%08lx stack = %d bytes used\n", funcname[i], (long) func,
                    status.task_stack_used);
            printf("*-----------------------------------*\n");
        }
        else
        {
            printf("%s \t func=0x%08lx stack = %d bytes used (%d - %d overhead)\n", funcname[i], (long) func,
                    status.task_stack_used-overhead_stackused,status.task_stack_used,overhead_stackused);
        }
        /* Tidy up */
        task_delete(task_p);
    }
    printf("*************************************\n");
    printf("*    Stack usage calculation end    *\n");
    printf("*************************************\n");
}
#endif /* ST_OS20 */
#if defined(ST_5105)|| defined (ST_5525)|| defined(ST_5188)|| defined(ST_5107)


/*-------------------------------------------------------------------------
 * Function : TTX_VMIXInject
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_TTXInject( STTST_Parse_t *pars_p, char *result_sym_p )
{
#ifdef ST_OS20
#define TELETEXT_PLAYER_WORKSPACE       2048
#endif
#if defined (ST_OS21) || defined(ST_OSLINUX)
#define TELETEXT_PLAYER_WORKSPACE       2048
    UNUSED_PARAMETER(result_sym_p);
#endif

    BOOL RetErr;
    S32 LVar;
    S32 IsExternal;
    U32* TTXT_BASE_ADDRESS_Temp;
    char Msg[512];
    char FileName[127];
    long FileDescriptor;

#if defined (ST_5525)
    long int Size;
    char *      Buffer;
    char *      AlignBuffer;
    FILE * fstream;

#endif




    RetErr = FALSE;



 #if defined (ST_5525)


             /*  DENC_TXTCFG  */
STSYS_WriteRegDev8(0x19a00100,0x30);

            /*  DEN_DACC  */ /*Framing Code Enable */
STSYS_WriteRegDev8(0x19a00104,0x8a);

            /*  DENC_CFG8   */
 STSYS_WriteRegDev8(0x19a00020,0x24);

             /*  DENC_CFG6   */
 STSYS_WriteRegDev8(0x19a00018,0x2);

                /*  TTXT VOUT  */
 /*   TTX_OUT_DELAY 4 ;TXT_ENABLE */
STSYS_WriteRegDev8(0x19a0070C,0x0F);

 #endif


    RetErr = STTST_GetInteger( pars_p, VMIXHandle, (S32*)&VMIXHandle );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
    }
    TtxInfo.VMIXHandle = VMIXHandle;

    /* get file name */
    memset( FileName, 0, sizeof(FileName));
    RetErr = STTST_GetString( pars_p, "", FileName, sizeof(FileName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected FileName" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 100, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expect number of injection (default 100)" );
        return(TRUE);
    }
    TtxInfo.Loop = (U32)LVar;

    /*if there is an external chip*/
    RetErr = STTST_GetInteger(pars_p, 0,&IsExternal);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected ExternalChip (0=internal/1=external)");
            return(API_EnableError);
        }
        TTXT_BASE_ADDRESS_Temp =(U32*)TTXT_BASE_ADDRESS;


#if defined(ST_5525)


    fstream = fopen(FileName, "rb");

    TtxInfo.AlignBuffer = (char *)memory_allocate(NCachePartition_p, (U32)((FILEMAXSIZE + 15) & (~15)));

    TtxInfo.AlignBuffer = (void *)(((unsigned int)TtxInfo.AlignBuffer + 15) & (~15));

    TtxInfo.Size = fread(TtxInfo.AlignBuffer,1,FILEMAXSIZE,fstream);

    fclose(fstream);

#else
    FileDescriptor = debugopen(FileName, "rb" );
    if (FileDescriptor< 0 )
    {
        sprintf(Msg, "unable to open %s ...\n", FileName);
        STTST_Print((Msg));
        return(TRUE);
    }

    TtxInfo.Size = debugfilesize(FileDescriptor);

    /* Get memory to store file data */
    /* ---  round up "size" to a multiple of 16 bytes, and malloc it --- */
    TtxInfo.Buffer = (char *)memory_allocate(NCachePartition_p, (U32)((TtxInfo.Size + 15) & (~15)));
    if (TtxInfo.Buffer == NULL)
    {
        STTST_Print(( "Not enough memory for file loading !\n"));
        debugclose(FileDescriptor);
        return(TRUE);
    }
    TtxInfo.AlignBuffer = (void *)(((unsigned int)TtxInfo.Buffer + 15) & (~15));
    debugread(FileDescriptor, TtxInfo.AlignBuffer, (size_t) TtxInfo.Size);
    debugclose(FileDescriptor);
#endif
    sprintf(Msg, "File [%s] loaded : size %ld at address %x \n",
            FileName, TtxInfo.Size, (int)TtxInfo.AlignBuffer);

    STTST_Print((Msg));

    /* ODD ViewPort*/
    RetErr = STVMIX_OpenVBIViewPort(VMIXHandle, STVMIX_VBI_TXT_ODD, &TtxInfo.VBI_ODDVPHdle);
    if (RetErr)
    {
        sprintf(Msg, "STVMIX_OpenVBIViewPort() = %d\n", RetErr);
        STTST_Print((Msg));
        return(TRUE);
    }

    RetErr = STVMIX_EnableVBIViewPort(TtxInfo.VBI_ODDVPHdle);
    if (RetErr)
    {
        sprintf(Msg, "STVMIX_EnableVBIViewPort() = %d\n", RetErr);
        STTST_Print((Msg));
        return(TRUE);
    }

    /* EVEN ViewPort*/
    RetErr = STVMIX_OpenVBIViewPort(VMIXHandle, STVMIX_VBI_TXT_EVEN, &TtxInfo.VBI_EVENVPHdle);
    if (RetErr)
    {
        sprintf(Msg, "STVMIX_OpenVBIViewPort() = %d\n", RetErr);
        STTST_Print((Msg));
        return(TRUE);
    }

    RetErr = STVMIX_EnableVBIViewPort(TtxInfo.VBI_EVENVPHdle);
    if (RetErr)
    {
        sprintf(Msg, "STVMIX_EnableVBIViewPort() = %d\n", RetErr);
        STTST_Print((Msg));
        return(TRUE);
    }
    STTST_Print(("Ok\n"));

    /* Run Task */
    TtxInfo.Terminate = FALSE ;
    TtxInfo.TTXBaseAdress =(U32*)TTXT_BASE_ADDRESS_Temp;

    RetErr = STOS_TaskCreate ((void (*) (void*))VBI_TeletextTsk,
                        (void*)&TtxInfo,
                         DriverPartition_p,
                         TELETEXT_PLAYER_WORKSPACE,
                        &TaskStack,
                        DriverPartition_p,
                        &(TtxInfo.TskId_p),
                        &TaskDesc,
                        MIN_USER_PRIORITY,
                       "Teletext injector",
                        0 /*task_flags_high_priority_process*/);
      if(RetErr)
      {
          STTST_Print(("Problem in the task creation  Teletext injector \n"));
           return(TRUE);
      }

    if (TtxInfo.TskId_p== NULL)
     {
        STTST_Print(("Teletext task : failed to create\n"));
     }
    else
    {
        STTST_Print(("Teletext task : created\n"));
     }
    return(RetErr);
}



/*-------------------------------------------------------------------------
 * Function : VBI_TTXInjectAux
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL VBI_TTXInject_Aux( STTST_Parse_t *pars_p, char *result_sym_p )
{

#ifdef ST_OS20
#define TELETEXT_PLAYER_WORKSPACE       2048
#endif
#if defined (ST_OS21) || defined(ST_OSLINUX)
#define TELETEXT_PLAYER_WORKSPACE       2048
UNUSED_PARAMETER(result_sym_p);
#endif


    BOOL RetErr;
    S32 LVar;
    S32 IsExternal;
    U32* TTXT_BASE_ADDRESS_Temp;
    char Msg[512];
    char FileName[127];
    long FileDescriptor;

#if defined (ST_5525)
    long int Size;
    char *      Buffer;
    char *      AlignBuffer;
    FILE * fstream;

#endif


 RetErr = FALSE;


#if defined (ST_5525)

             /*  DENC_TXTCFG  */
STSYS_WriteRegDev8(0x19b00100,0x30);

            /*  DEN_DACC  */ /*Framing Code Enable */
STSYS_WriteRegDev8(0x19b00104,0x8a);

            /*  DENC_CFG8   */
 STSYS_WriteRegDev8(0x19b00020,0x24);

             /*  DENC_CFG6   */
 STSYS_WriteRegDev8(0x19b00018,0x2);

                /*  TTXT VOUT  */
 /*   TTX_OUT_DELAY 4 ;TXT_ENABLE */
STSYS_WriteRegDev8(0x19b0070C,0x0F);
 #endif


    RetErr = STTST_GetInteger( pars_p, VMIXHandle2, (S32*)&VMIXHandle2 );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
    }
    TtxInfo.VMIXHandle = VMIXHandle;

    /* get file name */
    memset( FileName, 0, sizeof(FileName));
    RetErr = STTST_GetString( pars_p, "", FileName, sizeof(FileName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected FileName" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 100, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expect number of injection (default 100)" );
        return(TRUE);
    }
    TtxInfo.Loop = (U32)LVar;

    /*if there is an external chip*/
    RetErr = STTST_GetInteger(pars_p, 0,&IsExternal);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected ExternalChip (0=internal/1=external)");
            return(API_EnableError);
        }
        TTXT_BASE_ADDRESS_Temp =(U32*)TTXT_BASE_ADDRESS;


#if defined(ST_5525)


    fstream = fopen(FileName, "rb");

    TtxInfo2.AlignBuffer = (char *)memory_allocate(NCachePartition_p, (U32)((FILEMAXSIZE + 15) & (~15)));

    TtxInfo2.AlignBuffer = (void *)(((unsigned int)TtxInfo2.AlignBuffer + 15) & (~15));

    TtxInfo2.Size = fread(TtxInfo2.AlignBuffer,1,FILEMAXSIZE,fstream);

    fclose(fstream);

#else
    FileDescriptor = debugopen(FileName, "rb" );
    if (FileDescriptor< 0 )
    {
        sprintf(Msg, "unable to open %s ...\n", FileName);
        STTST_Print((Msg));
        return(TRUE);
    }

    TtxInfo.Size = debugfilesize(FileDescriptor);

    /* Get memory to store file data */
    /* ---  round up "size" to a multiple of 16 bytes, and malloc it --- */
    TtxInfo2.Buffer = (char *)memory_allocate(NCachePartition_p, (U32)((TtxInfo2.Size + 15) & (~15)));
    if (TtxInfo.Buffer == NULL)
    {
        STTST_Print(( "Not enough memory for file loading !\n"));
        debugclose(FileDescriptor);
        return(TRUE);
    }
    TtxInfo2.AlignBuffer = (void *)(((unsigned int)TtxInfo2.Buffer + 15) & (~15));
    debugread(FileDescriptor, TtxInfo2.AlignBuffer, (size_t) TtxInfo2.Size);
    debugclose(FileDescriptor);
#endif
    sprintf(Msg, "File [%s] loaded : size %ld at address %x \n",
            FileName, TtxInfo2.Size, (int)TtxInfo.AlignBuffer);

    STTST_Print((Msg));

    /* ODD ViewPort*/
    RetErr = STVMIX_OpenVBIViewPort(VMIXHandle2, STVMIX_VBI_TXT_ODD, &TtxInfo2.VBI_ODDVPHdle);
    if (RetErr)
    {
        sprintf(Msg, "STVMIX_OpenVBIViewPort() = %d\n", RetErr);
        STTST_Print((Msg));
        return(TRUE);
    }

    RetErr = STVMIX_EnableVBIViewPort(TtxInfo2.VBI_ODDVPHdle);
    if (RetErr)
    {
        sprintf(Msg, "STVMIX_EnableVBIViewPort() = %d\n", RetErr);
        STTST_Print((Msg));
        return(TRUE);
    }

    /* EVEN ViewPort*/
    RetErr = STVMIX_OpenVBIViewPort(VMIXHandle2, STVMIX_VBI_TXT_EVEN, &TtxInfo2.VBI_EVENVPHdle);
    if (RetErr)
    {
        sprintf(Msg, "STVMIX_OpenVBIViewPort() = %d\n", RetErr);
        STTST_Print((Msg));
        return(TRUE);
    }

    RetErr = STVMIX_EnableVBIViewPort(TtxInfo2.VBI_EVENVPHdle);
    if (RetErr)
    {
        sprintf(Msg, "STVMIX_EnableVBIViewPort() = %d\n", RetErr);
        STTST_Print((Msg));
        return(TRUE);
    }
    STTST_Print(("Ok\n"));

    /* Run Task */
    TtxInfo2.Terminate = FALSE ;
    TtxInfo2.TTXBaseAdress =(U32*)TTXT_BASE_ADDRESS_Temp;

    RetErr = STOS_TaskCreate ((void (*) (void*))VBI_TeletextTsk,
                        (void*)&TtxInfo2,
                         DriverPartition_p,
                         TELETEXT_PLAYER_WORKSPACE,
                        &TaskStack,
                        DriverPartition_p,
                        &(TtxInfo2.TskId_p),
                        &TaskDesc,
                        MIN_USER_PRIORITY,
                       "CCInject",
                        0 /*task_flags_high_priority_process*/);
      if(RetErr)
      {
          STTST_Print(("Problem in the task creation  CCInject \n"));
           return(TRUE);
      }


    if (TtxInfo2.TskId_p== NULL)
     {
        STTST_Print(("Teletext task : failed to create\n"));
     }
    else
    {
        STTST_Print(("Teletext task : created\n"));
     }

    return(RetErr);
}



/*-------------------------------------------------------------------------
 * Function : TTX_KillInject
 *            Kill TTX Inject task
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VBI_TTXKillInject( STTST_Parse_t *pars_p, char *Result )
{
    task_t* TaskList[1];

    BOOL RetErr;
    char Msg[512];

    /* Reset global variable and wait for task to finish before deleting */
    TtxInfo.Terminate = TRUE;
    if ( TtxInfo.TskId_p != NULL )
    {
        TaskList[0] = TtxInfo.TskId_p;

        if ( STOS_TaskWait( TaskList, TIMEOUT_INFINITY ) )
        {
            STTST_Print(("Error: Timeout task_wait\n"));
        }
        else
        {
            /* Close VBIViewPort  */
            RetErr = STVMIX_CloseVBIViewPort(TtxInfo.VBI_EVENVPHdle);
            if (RetErr)
            {
                sprintf(Msg, "STVMIX_OpenVBIViewPort() = %d\n", RetErr);
                STTST_Print((Msg));
                return(TRUE);
            }
            /* Close VBIViewPort  */
            RetErr = STVMIX_CloseVBIViewPort(TtxInfo.VBI_ODDVPHdle);
            if (RetErr)
            {
                sprintf(Msg, "STVMIX_OpenVBIViewPort() = %d\n", RetErr);
                STTST_Print((Msg));
                return(TRUE);
            }

            STTST_Print(("Telextext injection Task Ended\n"));
            if ( STOS_TaskDelete( TtxInfo.TskId_p,DriverPartition_p, TaskStack, DriverPartition_p ) != 0 )
            {
                STTST_Print(("Error: Couldn't delete TTX injection task!!\n"));
            }
            else
            {
                TtxInfo.TskId_p = NULL;

            }
        }

    }
    return FALSE;
} /* end VBI_TTXKillInject */


/*-------------------------------------------------------------------------
 * Function : TTX_KillInject_Aux
 *            Kill TTX Inject task
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VBI_TTXKillInject_Aux( STTST_Parse_t *pars_p, char *Result )
{
    task_t* TaskList[1];
    BOOL RetErr;
    char Msg[512];

    /* Reset global variable and wait for task to finish before deleting */
    TtxInfo2.Terminate = TRUE;
    if ( TtxInfo2.TskId_p != NULL )

    {
        TaskList[0] = TtxInfo2.TskId_p;
        if ( STOS_TaskWait ( TaskList, TIMEOUT_INFINITY ) )
        {
            STTST_Print(("Error: Timeout task_wait\n"));
        }
        else
        {
            /* Close VBIViewPort  */
            RetErr = STVMIX_CloseVBIViewPort(TtxInfo2.VBI_EVENVPHdle);
            if (RetErr)
            {
                sprintf(Msg, "STVMIX_OpenVBIViewPort() = %d\n", RetErr);
                STTST_Print((Msg));
                return(TRUE);
            }
            /* Close VBIViewPort  */
            RetErr = STVMIX_CloseVBIViewPort(TtxInfo2.VBI_ODDVPHdle);
            if (RetErr)
            {
                sprintf(Msg, "STVMIX_OpenVBIViewPort() = %d\n", RetErr);
                STTST_Print((Msg));
                return(TRUE);
            }

            STTST_Print(("Telextext injection Task Ended\n"));
            if ( STOS_TaskDelete( TtxInfo2.TskId_p,DriverPartition_p, TaskStack, DriverPartition_p ) != 0 )

            {
                STTST_Print(("Error: Couldn't delete TTX injection task!!\n"));
            }
            else
            {
                TtxInfo2.TskId_p = NULL;

            }
        }

    }
    return FALSE;
} /* end VBI_TTXKillInject */



static void VBI_TeletextTsk(InfoTtxTsk_t * Infos)
{

    unsigned long i,j;
    unsigned long numtransfer;
    STVMIX_VBIViewPortParams_t  VBI_ODDVPParams;
    STVMIX_VBIViewPortParams_t  VBI_EVENVPParams;

    while (!Infos->Terminate) /* indefinite loop */
    {
        numtransfer   = 8;

        for( j=0 ;  (!Infos->Terminate); j++)
        {
            for( i=0; (i<numtransfer) && (!Infos->Terminate); i++)
            {
                VBI_ODDVPParams.Source_p        = (U8 *)(Infos->AlignBuffer + 10 * i * VBI_LIGN_LENGTH);
                VBI_ODDVPParams.LineMask        = 0x3FF;

                VBI_EVENVPParams.Source_p       = (U8 *)(Infos->AlignBuffer + 10 * i * VBI_LIGN_LENGTH);
                VBI_EVENVPParams.LineMask       = 0x3FF;

                STVMIX_SetVBIViewPortParams(Infos->VBI_ODDVPHdle,&VBI_ODDVPParams);

                STVMIX_SetVBIViewPortParams(Infos->VBI_EVENVPHdle,&VBI_EVENVPParams);

                STOS_TaskDelay (1000);
                STOS_TaskDelay(200000);
            }
            STOS_TaskDelay(2000000);
        }
    } /* exit when finished */
    memory_deallocate (NCachePartition_p, Infos->Buffer );

}
#endif

#if defined (ST_7100) ||  defined (ST_7109)
/*-------------------------------------------------------------------------
 * Function : VBI_GamSet
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_GamSet( STTST_Parse_t *pars_p, char *result_sym_p )
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

    STSYS_WriteRegDev32LE( ((U32)VMIX1_BASE_ADDRESS + 0x0), 0x3);
    STSYS_WriteRegDev32LE( ((U32)VMIX1_BASE_ADDRESS + 0x28), 0x300089);
    STSYS_WriteRegDev32LE( ((U32)VMIX1_BASE_ADDRESS + 0x2c), 0x26f0358);
    STSYS_WriteRegDev32LE( ((U32)VMIX1_BASE_ADDRESS + 0x34), 0x40);
    STSYS_WriteRegDev32LE( ((U32)VOS_BASE_ADDRESS_1 + 0x70), 0x1);

    return ( API_EnableError ? ErrorCode : FALSE );
} /* end VBI_GamSet   */
#endif /* #ifdef ST_7100 -- ST_7109 */

/*-------------------------------------------------------------------------
 * Function : VBI_InitCommand2
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : FALSE if error, TRUE if success
 * ----------------------------------------------------------------------*/
static BOOL VBI_InitCommand2(void)
{

    BOOL RetErr=FALSE;




    RetErr |= STTST_RegisterCommand("VBI_CCLoad", CC_LoadData, "Load a CC data pattern");
    RetErr |= STTST_RegisterCommand("VBI_CCInject", CC_MemInject, \
                               "VBICCInject [LOOP] Launch the CC data injection task");
    RetErr |= STTST_RegisterCommand("VBI_CCKill", CC_KillTask, "Kill the CC data injection task");
#ifdef ST_OS20
    RetErr |= STTST_RegisterCommand( "VBI_StackUse" , (BOOL (*)(STTST_Parse_t*, char*))metrics_Stack_Test, "Stack metrics");
#endif /* ARCHITECTURE_ST20 */

    RetErr |= STTST_RegisterCommand("VBI_FullRegCheck", VBI_FullRegisterCheck, "<Add><Data><Mask> Check elsewhere Register");

    /* Other tests functions to access DENC registers */
    RetErr  = STTST_RegisterCommand("VBI_Access", VBI_Access,
                                    "<AccessType> Access type to denc registers (0=EMI8Bits,1=EMI32Bits,2=I2C)");
    RetErr |= STTST_RegisterCommand("VBI_RegCheck", VBI_RegisterCheck, "<Symb><EXTCHIP|Add+Length><Data><Mask> CheckRegister");
    RetErr |= STTST_RegisterCommand("VBI_RegRead", VBI_RegisterRead, "<<Symb><EXTCHIP>|<Add><Length>><Value><EXTCHIP> ReadRegister");
    RetErr |= STTST_RegisterCommand("VBI_RegWrite", VBI_RegisterWrite, "<<Symb><EXTCHIP>|<Add>><Value><EXTCHIP> WriteRegister");
#if defined (ST_5105) || defined (ST_5525)|| defined(ST_5188)|| defined(ST_5107)
    RetErr |= STTST_RegisterCommand("VBI_TTXInject", VBI_TTXInject, "");
    RetErr |= STTST_RegisterCommand("VBI_TTXInject_Aux", VBI_TTXInject_Aux, "");

    RetErr |= STTST_RegisterCommand("VBI_TTXKill", VBI_TTXKillInject, "");
    RetErr |= STTST_RegisterCommand("VBI_TTXKill_Aux", VBI_TTXKillInject_Aux, "");
#endif

#if defined (ST_7100) || defined (ST_7109)

    RetErr |= STTST_RegisterCommand("VBI_GamSet", VBI_GamSet, "");
#endif

    return(!RetErr);
} /* end of VBI_InitCommand2() */


/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL Test_CmdStart(void)
{
    BOOL RetOk;
    U32 i;
#ifdef ST_OSLINUX
    for (i=0; i<=CC_MAX_TASK;i++)
    {
        CCTaskArray[i].CC_NbBytes = 0;
    }
#else
    for (i=0; i<=CC_MAX_TASK;i++)
    {
        CCTaskArray[i].CCTid = NULL;
        CCTaskArray[i].CC_NbBytes = 0;
    }

#endif
#ifdef WA_GNBvd11019
    WaitOneFrame = ST_GetClocksPerSecond()/30; /* 30Hz, round by excess. Ok as test done with NTSC */
#endif /* WA_GNBvd11019 */
    RetOk = VBI_InitCommand2();
    if ( RetOk )
    {
        STTBX_Print(( "VBI_TestCommand() \t: ok\n" ));
    }
    else
    {
        STTBX_Print(( "VBI_TestCommand() \t: failed !\n" ));
    }
    return(RetOk);
}

/*#########################################################################
 *                                 MAIN
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : os20_main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void os20_main(void *ptr)
{
    UNUSED_PARAMETER(ptr);

    STAPIGAT_Init();
    STAPIGAT_Term();

} /* end os20_main */


/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{

UNUSED_PARAMETER(argc);
UNUSED_PARAMETER(argv);

#ifdef ST_OS21
    printf ("\nBOOT ...\n");

    setbuf(stdout, NULL);
        os20_main(NULL);
#else
    os20_main(NULL);
#endif

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}
/* end of vbi_test.c */




