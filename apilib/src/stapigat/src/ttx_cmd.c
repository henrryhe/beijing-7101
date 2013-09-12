/************************************************************************

File name   : ttx_cmd.c

Description : STTTX macros

COPYRIGHT (C) STMicroelectronics 2002.

Date          Modification                                    Initials
----          ------------                                    --------
19 Sep 2000   Creation                                        CL
14 Jun 2002   Moved to StapiGat                               HSdLM
22 Nov 2002   Fix endianness issue with STTST_GetInteger()    HSdLM
 *            use
************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef ST_OS20
#include <debug.h>
#include <task.h>
#endif
#ifdef ST_OS21
#include <os21debug.h>
#include <stlite.h>
#include <fcntl.h>
#endif
#include "testcfg.h"

#ifdef USE_TTX

#define USE_AS_FRONTEND

/* Private preliminary definitions (internal use only) ---------------------- */
#if !( defined (ST_5528)|| defined (ST_7100) || defined (ST_7109) || defined (ST_5301) || defined (ST_7200))
#ifndef ST_5508
#ifndef ST_GX1
#define TEST_WITH_TELETEXT
#define TEST_WITH_TELETEXT_DRIVER
#endif /* ST_GX1 */
#endif /* ST_5508 */
#endif /* ST_5528 */
#if defined (ST_5528) || defined (ST_7100)|| defined (ST_7109) || defined (ST_5301) || defined (ST_7200)
#define TEST_WITH_TELETEXT
#endif
#ifdef TEST_WITH_TELETEXT_DRIVER
#define TEST_WITH_TELETEXT
#endif

/* Includes ------------------------------------------------------------------*/

#include "genadd.h"
#include "stddefs.h"
#include "stdevice.h"


#include "stsys.h"
#include "stcommon.h"

#ifdef ST_OSLINUX
#include "compat.h"
#include "iocstapigat.h"
#else
#include "stpio.h"
#include "clpio.h"
#endif

#include "sttbx.h"
#include "testtool.h"
#ifdef TEST_WITH_TELETEXT_DRIVER
#include "stttx.h"
#endif

#include "startup.h"

#include "clevt.h"
#include "api_cmd.h"
#include "vbi_cmd.h"
#include "ttx_cmd.h"

#if defined (ST_7100) || defined (ST_5301) || defined (ST_7109) || defined (ST_7200)
#define USE_AVMEM_ALLOC
#endif

#ifdef USE_AVMEM_ALLOC
#if !defined ST_OSLINUX
#include "stavmem.h"
#include "clavmem.h"
#else
#include "stvbi.h"
#endif
#endif

/* Private Types ------------------------------------------------------------ */

#ifdef TEST_WITH_TELETEXT_DRIVER
typedef struct InsertParams_s
{
    U8  *DriverPesBuffer_p;
    U8  *PesBuffer_p;
    U32 TotalSize;
    U32 PesSize;
    semaphore_t * DataSemaphore_p;
} InsertParams_t;
#endif /* TEST_WITH_TELETEXT_DRIVER */

#ifdef TEST_WITH_TELETEXT
/* For injection */
typedef struct InfoTtxTsk_s
{
    U32              Loop;
    BOOL             Terminate;
    char*            Buffer;
    char*            AlignBuffer;
    unsigned long    Size;
    U32*             TTXBaseAdress;
#ifdef ST_OSLINUX
    pthread_t                       TskId;
    pthread_attr_t 		            TaskAttribute;
#else
    task_t*          TskId;
#endif
} InfoTtxTsk_t;
#endif /* TEST_WITH_TELETEXT */

/* Private Constants -------------------------------------------------------- */

#ifdef mb295
#define TTXT_BASE_ADDRESS 0x20024000
#define TELETEXT_INTERRUPT  13 /* same ST5510_TTX_INTERRUPT */
#endif

#if defined(mb376) || defined(espresso)
#define TTXT_BASE_ADDRESS     ST5528_TTX0_BASE_ADDRESS
#define TTXT_BASE_ADDRESS_2   ST5528_TTX1_BASE_ADDRESS
#define TELETEXT_INTERRUPT    ST5528_TTX0_INTERRUPT
#define TELETEXT_INTERRUPT_2  ST5528_TTX1_INTERRUPT
#define TTXT_CFG              0x250
#endif

#ifdef mb390
#if defined (ST_5100)
#define TTXT_BASE_ADDRESS     ST5100_TTX_BASE_ADDRESS
#define TELETEXT_INTERRUPT    ST5100_TTX_INTERRUPT
#elif defined (ST_5301)
#define TTXT_BASE_ADDRESS     ST5301_TTX_BASE_ADDRESS
#define TELETEXT_INTERRUPT    ST5301_TTX_INTERRUPT
#endif
#endif

#ifdef mb391
#define TTXT_BASE_ADDRESS     ST7710_TTX_BASE_ADDRESS
#define TELETEXT_INTERRUPT    ST7710_TELETEXT_INTERRUPT
#endif

#ifdef mb411
#ifdef ST_OSLINUX
extern MapParams_t   TTXMap;
#define TTXT_BASE_ADDRESS     TTXMap.MappedBaseAddress
#if defined (ST_7100)
#define TELETEXT_INTERRUPT    ST7100_TELETEXT_INTERRUPT
#elif defined (ST_7109)
#define TELETEXT_INTERRUPT    ST7109_TELETEXT_INTERRUPT
#endif /** OS_LINUX ST_7100 - ST_7109 **/
#else
#if defined (ST_7100)
#define TTXT_BASE_ADDRESS     ST7100_TTX_BASE_ADDRESS
#elif defined (ST_7109)
#define TTXT_BASE_ADDRESS     ST7109_TTX_BASE_ADDRESS
#endif  /*** ST_7100 - ST_7109 ***/
#endif  /** ST_OSLINUX **/
#endif /*** mb411 ***/

#ifdef mb519
#define TTXT_BASE_ADDRESS     ST7200_TTX_BASE_ADDRESS
#endif

/* Private Variables (static)------------------------------------------------ */

#ifdef TEST_WITH_TELETEXT_DRIVER
static InsertParams_t  VBIInsertParams;
static BOOL            InsertTaskExit = FALSE;
static char            TTX_Msg[200];  /* text for trace */
static STTTX_Handle_t  TTXHndl;

#ifndef TEST_WITH_TUNER
/*static U8              VBIInsertMainStack[2048];
static tdesc_t         IMVBItdesc;                            */
static task_t*          IMVBITask;
#endif /* TEST_WITH_TUNER */

/* PTI slots for teletext data */
#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
slot_t vbi_slot;
#endif
#endif /* TEST_WITH_TELETEXT_DRIVER */

#ifdef TEST_WITH_TELETEXT
static InfoTtxTsk_t    TtxInfo; /* For teletext task */
#endif /* TEST_WITH_TELETEXT */
#if defined(mb376) || defined(espresso)
static BOOL pio_init = TRUE;
#endif
/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#ifdef TEST_WITH_TELETEXT
#define stvbi_write8(a,v)    STSYS_WriteRegDev8((void*)(a),v)
#define stvbi_read8(a)       STSYS_ReadRegDev8((void*)(a))
#endif /* TEST_WITH_TELETEXT */

/* Private Function prototypes ---------------------------------------------- */

#ifdef TEST_WITH_TELETEXT
static void TTX_TeletextTsk(InfoTtxTsk_t * Infos);
#endif /* TEST_WITH_TELETEXT */

/* Functions ---------------------------------------------------------------- */
#ifdef ST_OSLINUX
extern ST_ErrorCode_t STVBI_AllocData( STVBI_Handle_t  LayerHandle, STVBI_AllocDataParams_t *Params_p, void **Address_p );
extern STVBI_Handle_t VBIHndl;
#endif


extern ST_Partition_t          *system_partition;
#ifdef TEST_WITH_TELETEXT_DRIVER
#ifndef TEST_WITH_TUNER
/*#######################################################################*/
/*######################### TTXT DRIVER COMMANDS ########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : InsertInit
 *            Gets memory for pes buffer for capture/insert mode
 *            If inserting, loads file into buffer
 * Input    : PesBuffer_p   pointer to byte-array for PES packets
 *            BufferUsed_p  pointer to U32 for amount of buffer used
 * Output   : 0        success
 *           -1       failure (no memory)
 *           -2       failure (file-related)
 * ----------------------------------------------------------------------*/
static int InsertInit(U8 **PesBuffer_p, U32 *BufferUsed_p)
{
#define STTTX_PESBUFFER_SIZE 1000000
#define STTTX_PES_FILE "default.pes"
#ifdef ST_OS21
struct stat FileStatus;
volatile long int FileSize;
#endif

    /* Get memory */
    *PesBuffer_p = (U8 *)memory_allocate((partition_t *)system_partition, STTTX_PESBUFFER_SIZE );

    if (*PesBuffer_p == NULL)
    {
        #ifdef ST_OS20
          debugmessage("Unable to allocate memory for PesBuffer_p!\n");
        #endif
        #ifdef ST_OS21
          printf("Unable to allocate memory for PesBuffer_p!\n");
        #endif
        return -1;
    }

    /* If we're inserting, then load the data from the file */
    {
        #ifdef ST_OS20
         S32 file = (S32)debugopen(STTTX_PES_FILE, "rb");
        #endif
        #ifdef ST_OS21
         S32 file = (S32)open(STTTX_PES_FILE, O_RDONLY|O_BINARY);
        #endif
        if (file == -1)
        {
            #ifdef ST_OS20
             debugmessage("Error trying to open file\n");
            #endif
            #ifdef ST_OS21
             printf("Error trying to open file\n");
            #endif
            return -2;
        }
        else
        {
            if (PesBuffer_p != NULL)
            {
                #ifdef ST_OS20
                if (debugfilesize(file) < STTTX_PESBUFFER_SIZE)
                {
                    debugmessage("Reading from file...\n");
                    *BufferUsed_p = (U32)debugread(file, *PesBuffer_p, (int)debugfilesize(file));
                #endif
                #ifdef ST_OS21
                    printf("Reading from file...\n");
                    fstat(file, &FileStatus);
                    if (FileStatus.st_size< STTTX_PESBUFFER_SIZE)
                    {
                        FileSize = FileStatus.st_size;
                        *BufferUsed_p = (U32)read(file, *PesBuffer_p, (int)FileSize);
                #endif
                }
                else
                {
                    #ifdef ST_OS20
                    debugmessage("WARNING! PES file is larger than buffer!\n");
                    debugmessage("Reading as much as possible, but we are very likely to hit problems later!");
                    *BufferUsed_p = (U32)debugread(file, *PesBuffer_p, STTTX_PESBUFFER_SIZE);
                    #endif
                    #ifdef ST_OS21
                    printf("WARNING! PES file is larger than buffer!\n");
                    printf("Reading as much as possible, but we are very likely to hit problems later!");
                    *BufferUsed_p = (U32)read(file, *PesBuffer_p, STTTX_PESBUFFER_SIZE);
                    #endif
                }
                STTBX_Print(("Read %i bytes from file\n", *BufferUsed_p));
                #ifdef ST_OS20
                debugclose(file);
                #endif
                #ifdef ST_OS21
                close(file);
                #endif
            }
            else
            {
                #ifdef ST_OS20
                debugmessage("Unable to read from file; no buffer\n");
                #endif
                #ifdef ST_OS21
                printf("Unable to read from file; no buffer\n");
                #endif
                *BufferUsed_p = 0;
                return -2;
            }
        }
    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function : InsertMain
 *            Does the work of inserting a packet
 * Parameters   : ThisElem         STB object
 *                CurrPes          structure detailing current pes packet (this is
 *                                 set or read accordingly)
 *               PesBuffer_p      pointer to byte-array for PES packets
 *               buffer_position  where in the PesBuffer_p we are at present
 *               BufferUsed       total bytes in the buffer (not the overall size)
 *               BytesWritten     the size of the PES packet (read or set
 *                                 accordingly)
 *               CaptureAction    called twice during capture-mode; this is which
 *                                 action we should do at this call
 * Return Value : void
 * ----------------------------------------------------------------------*/
static void InsertMain( void *Data)
{
    static U32 BufferPosition = 0;
    InsertParams_t *Params = (InsertParams_t *)Data;

    while (!InsertTaskExit)
    {
        if (FALSE)
          #ifdef ST_OS20
                debugmessage("Inserting data\n");
          #endif
          #ifdef ST_OS21
                printf("Inserting data\n");
          #endif
        /* Copy PES size into Params->PesSize, move through buffer */
        memcpy(&Params->PesSize, &Params->PesBuffer_p[BufferPosition],
               sizeof(Params->PesSize));
        BufferPosition += sizeof(Params->PesSize);

        /* Point linearbuff at the PesBuffer_p, and then move the marker
         * forward again
         */
        Params->DriverPesBuffer_p = &Params->PesBuffer_p[BufferPosition];
        BufferPosition += Params->PesSize;

        /* Just let the user know if we've had to wrap */
        if ((BufferPosition >= Params->TotalSize) &&
            (Params->PesBuffer_p != NULL))
        {
            STTBX_Print(("Wrapping to start of buffer (for next packet)\n"));
            BufferPosition = 0;
        }

        /* Don't want to flood the thing with data */
        task_delay(ST_GetClocksPerSecond() / 25);

        /* And off we go */
        semaphore_signal(Params->DataSemaphore_p);
    }
    memory_deallocate((partition_t *)system_partition, Params->PesBuffer_p);
}
#endif /* TEST_WITH_TUNER */
#endif /* TEST_WITH_TELETEXT_DRIVER */
#if defined(mb376) || defined(espresso)
/*-------------------------------------------------------------------------
 * Function : TTX_SetPIO6
              Configure the PIO6[6] & PIO6[7] to exchange ttx data between
              Sti5528 and STi4629
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TTXT_SetPIO6( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STPIO_OpenParams_t OpenParams;
    STPIO_Handle_t PIO6Handle;
    ST_ErrorCode_t err;
   if (pio_init == TRUE)
   {
        /* setup pio bit required */
        OpenParams.ReservedBits     = PIO_BIT_6|PIO_BIT_7;
        OpenParams.BitConfigure[6]  = STPIO_BIT_INPUT;
        OpenParams.BitConfigure[7]  = STPIO_BIT_ALTERNATE_OUTPUT;
        OpenParams.IntHandler       = NULL;

        err = STPIO_Open(STPIO_6_DEVICE_NAME, &OpenParams, &PIO6Handle);
        STTST_Print(("STPIO_Open()\n", err));
        if (err == ST_NO_ERROR)
        {
            STTST_Print(("The PIO6 open was succesfull\n"));
            pio_init = FALSE;
            return FALSE;
        }
        else
        {
            STTST_Print(("Unable to configure PIO6 for STi4629\n"));
            return TRUE;
        }
   }
   return FALSE;
}

#endif
#ifdef TEST_WITH_TELETEXT_DRIVER
/*-------------------------------------------------------------------------
 * Function : TTX_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TTX_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    STTTX_InitParams_t TTXInitParams;
    char TTXDeviceName[80];
    char VBIDeviceName[80];
    BOOL RetErr;
    U32* TTXT_BASE_ADDRESS_Temp;
    U32 TELETEXT_INTERRUPT_Temp;
    S32 IsExternal;

    /* get Teletext device name */
    TTXDeviceName[0] = '\0';

    RetErr = STTST_GetString( pars_p, STTTX_DEVICE_NAME, TTXDeviceName, sizeof(TTXDeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected TeleText DeviceName" );
        return(TRUE);
    }

    /* get VBI device name */
    RetErr = STTST_GetString( pars_p, STVBI_DEVICE_NAME, VBIDeviceName, sizeof(VBIDeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected VBI DeviceName" );
        return(TRUE);
    }

    /*if there is an external chip*/
    RetErr = STTST_GetInteger(pars_p, 0,&IsExternal);
    if (RetErr)
     {
        STTST_TagCurrentLine(pars_p, "Expected ExternalChip (0=internal/1=external)");
        return(API_EnableError);
    }

    #if defined(mb376) || defined(espresso)
        TTXT_BASE_ADDRESS_Temp = (IsExternal==1) ?(U32*)TTXT_BASE_ADDRESS_2:(U32*)TTXT_BASE_ADDRESS ;
        TELETEXT_INTERRUPT_Temp= (IsExternal==1) ?(U32)TELETEXT_INTERRUPT_2:(U32)TELETEXT_INTERRUPT ;
    #else
        TTXT_BASE_ADDRESS_Temp =(U32*)TTXT_BASE_ADDRESS;
        TELETEXT_INTERRUPT_Temp=(U32)TELETEXT_INTERRUPT;
    #endif

    TTXInitParams.DeviceType = STTTX_DEVICE_OUTPUT;
    TTXInitParams.DriverPartition = NCachePartition_p;
    strcpy( TTXInitParams.EVTDeviceName, STEVT_DEVICE_NAME );
    strcpy( TTXInitParams.VBIDeviceName, VBIDeviceName );
    TTXInitParams.BaseAddress = (U32*)TTXT_BASE_ADDRESS_Temp ;
    TTXInitParams.InterruptNumber = TELETEXT_INTERRUPT_Temp;
    TTXInitParams.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    TTXInitParams.NumVBIObjects = 1;                 /* assumed to be one */
    TTXInitParams.NumVBIBuffers = 1;                 /* not used by driver */
    TTXInitParams.NumSTBObjects = 1;
    TTXInitParams.NumRequestsAllowed = 4;

    VBIInsertParams.DataSemaphore_p = semaphore_create_fifo(0);

    sprintf(TTX_Msg, "STTTX_Init(%s, Dev %s): ", TTXDeviceName, VBIDeviceName);
    ErrorCode = STTTX_Init(TTXDeviceName, &TTXInitParams);
    if(ErrorCode!= ST_NO_ERROR)
    {
        sprintf(TTX_Msg, "%sFailed Error=%d!\n", TTX_Msg, ErrorCode);
        RetErr = TRUE;
    }
    else
    {
        strcat(TTX_Msg, "Ok\n");
        RetErr = FALSE;
    }
    STTST_Print((TTX_Msg));
    return(RetErr);
}


/*-------------------------------------------------------------------------
 * Function : TTX_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TTX_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    STTTX_TermParams_t TTXTermParams;
    char TTXDeviceName[80];
    BOOL RetErr;

    /* get device name */
    TTXDeviceName[0] = '\0';

    RetErr = STTST_GetString( pars_p, STTTX_DEVICE_NAME, TTXDeviceName, sizeof(TTXDeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected TTXDeviceName" );
        return(TRUE);
    }

    TTXTermParams.ForceTerminate = TRUE;

    sprintf(TTX_Msg, "STTTX_Term(%s): ", TTXDeviceName);
    ErrorCode = STTTX_Term(TTXDeviceName, &TTXTermParams);
    if(ErrorCode!= ST_NO_ERROR)
    {
        sprintf(TTX_Msg, "%sFailed Error=%d!\n", TTX_Msg, ErrorCode);
        RetErr = TRUE;
    }
    else
    {
        /* Inform allocated semaphore */
        semaphore_signal(VBIInsertParams.DataSemaphore_p);

        /* Delete allocated semaphore */
        semaphore_delete(VBIInsertParams.DataSemaphore_p);

        strcat(TTX_Msg, "Ok\n");
        RetErr = FALSE;
    }
    STTST_Print((TTX_Msg));
    return(RetErr);
}


/*-------------------------------------------------------------------------
 * Function : TTX_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TTX_Open( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    STTTX_OpenParams_t TTXOpenParams;
    char               TTXDeviceName[80];
    BOOL               RetErr;
    S32                LVar;

    /* get device name */
    TTXDeviceName[0] = '\0';

    RetErr = STTST_GetString( pars_p, STTTX_DEVICE_NAME, TTXDeviceName, sizeof(TTXDeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected TTX DeviceName" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, STTTX_VBI, (S32 *)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected TTX Type (default VBI)" );
        return(TRUE);
    }
    TTXOpenParams.Type = (STTTX_ObjectType_t)LVar;

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    if(TTXOpenParams.Type == STTTX_VBI)
        TTXOpenParams.Slot = vbi_slot;
    else
        return(TRUE);
    sprintf(TTX_Msg, "STTTX_Open(%s, Type=%d, Slot=%d): ", \
            TTXDeviceName, TTXOpenParams.Type, TTXOpenParams.Slot );
#else
    sprintf(TTX_Msg, "STTTX_Open(%s, Type=%d): ", \
            TTXDeviceName, TTXOpenParams.Type);
#endif

    ErrorCode = STTTX_Open(TTXDeviceName, &TTXOpenParams, &TTXHndl);
    if(ErrorCode != ST_NO_ERROR)
    {
        sprintf(TTX_Msg, "%sFailed Error=%d!\n", TTX_Msg, ErrorCode);
        RetErr = TRUE;
    }
    else
    {
        strcat(TTX_Msg, "Ok\n");
        RetErr = FALSE;
    }
    STTST_Print((TTX_Msg));
#ifndef TEST_WITH_TUNER
    /* Do the insert setup */
    if (InsertInit(&VBIInsertParams.PesBuffer_p, &VBIInsertParams.TotalSize) != 0) {
        STTST_Print(("Insert Init Failed !!!\n"));
        return(TRUE);
    }
    VBIInsertParams.DriverPesBuffer_p = VBIInsertParams.PesBuffer_p;
#endif /* TEST_WITH_TUNER */
    STTST_AssignInteger(result_sym_p, TTXHndl, FALSE);
    return(RetErr);
}


/*-------------------------------------------------------------------------
 * Function : TTX_Start
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TTX_Start( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;

#define STTTX_VBI_TASK_PRIORITY 9

#ifndef TEST_WITH_TUNER
    /* Spawn off the VBI insert task */
    IMVBITask = task_create( (void(*)(void *))InsertMain,
                    (void *)&VBIInsertParams,
                    (int)2048,
                    (int)STTTX_VBI_TASK_PRIORITY,
                    "VBI packet insertion", task_flags_no_min_stack_size );
    if( IMVBITask==NULL)
    {
        STTBX_Print(("Error trying to spawn VBI InsertMain task\n"));
        return(TRUE);
    }
#endif /* TEST_WITH_TUNER */

    RetErr = STTST_GetInteger( pars_p, TTXHndl, (S32*)&TTXHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected handle" );
        return(TRUE);
    }

    sprintf(TTX_Msg, "STTTX_Start(%d): ", TTXHndl);
    ErrorCode = STTTX_Start(TTXHndl);
    if(ErrorCode != ST_NO_ERROR)
    {
        sprintf(TTX_Msg, "%sFailed Error=%d!\n", TTX_Msg, ErrorCode);
        RetErr = TRUE;
    }
    else
    {
        strcat(TTX_Msg, "Ok\n");
        RetErr = FALSE;
    }
    STTST_Print((TTX_Msg));

    return(RetErr);
}


/*-------------------------------------------------------------------------
 * Function : TTX_SetSource
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TTX_SetSource( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    U32 LVar;
    STTTX_SourceParams_t SourceParams;

    RetErr = STTST_GetInteger( pars_p, TTXHndl, (S32*)&TTXHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected handle" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, STTTX_SOURCE_USER_BUFFER, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected source type (default User buffer)" );
        return(TRUE);
    }
    SourceParams.DataSource = (STTTX_Source_t)LVar;

    if(SourceParams.DataSource == STTTX_SOURCE_USER_BUFFER)
    {
        SourceParams.Source_u.UserBuf_s.DataReady_p = VBIInsertParams.DataSemaphore_p;
        SourceParams.Source_u.UserBuf_s.PesBuf_p = &VBIInsertParams.DriverPesBuffer_p;
        SourceParams.Source_u.UserBuf_s.BufferSize = &VBIInsertParams.PesSize;
    }
    if(SourceParams.DataSource == STTTX_SOURCE_PTI_SLOT)
    {
#ifdef TEST_WITH_TUNER
#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
        pti_allocate_dynamic_slot(&vbi_slot);
        SourceParams.Source_u.PTISlot_s.Slot = vbi_slot;
#endif
#endif
    }

    sprintf(TTX_Msg, "STTTX_SetSource(%d,Type=%d): ", TTXHndl, SourceParams.DataSource);
    ErrorCode = STTTX_SetSource( TTXHndl, &SourceParams );
    if(ErrorCode != ST_NO_ERROR)
    {
        sprintf(TTX_Msg, "%sFailed Error=%d!\n", TTX_Msg, ErrorCode);
        RetErr = TRUE;
    }
    else
    {
        strcat(TTX_Msg, "Ok\n");
        RetErr = FALSE;
    }
    STTST_Print((TTX_Msg));

    return(RetErr);
}

/*-------------------------------------------------------------------------
 * Function : TTX_SetStreamID
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TTX_SetStreamID( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    U32 Pid;
    U32 LVar;

    RetErr = STTST_GetInteger( pars_p, TTXHndl, (S32*)&TTXHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected handle" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0x3C, (S32 *)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expect PID value (default=0x3C)" );
        return(FALSE);
    }
    Pid = (U32)LVar;

    sprintf(TTX_Msg, "STTTX_SetStreamID(%d,Pid=0x%2X): ", TTXHndl, Pid );
    ErrorCode = STTTX_SetStreamID( TTXHndl, Pid );
    if(ErrorCode != ST_NO_ERROR)
    {
        sprintf(TTX_Msg, "%sFailed Error=%d!\n", TTX_Msg, ErrorCode);
        RetErr = TRUE;
    }
    else
    {
        strcat(TTX_Msg, "Ok\n");
        RetErr = FALSE;
    }
    STTST_Print((TTX_Msg));

    return(RetErr);
}


/*-------------------------------------------------------------------------
 * Function : TTX_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TTX_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, TTXHndl, (S32*)&TTXHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected handle" );
        return(TRUE);
    }

    sprintf(TTX_Msg, "STTTX_Close(%d): ", TTXHndl);
    ErrorCode = STTTX_Close(TTXHndl);
    if(ErrorCode != ST_NO_ERROR)
    {
        sprintf(TTX_Msg, "%sFailed Error=%d!\n", TTX_Msg, ErrorCode);
        RetErr = TRUE;
    }
    else
    {
        strcat(TTX_Msg, "Ok\n");
        RetErr = FALSE;
    }
    STTST_Print((TTX_Msg));
    return(RetErr);
}

/*-------------------------------------------------------------------------
 * Function : TTX_Stop
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TTX_Stop( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    task_t *TaskList[1];

    /* Reset global variable and wait for task to finish before deleting */
    InsertTaskExit = TRUE;

    RetErr = STTST_GetInteger( pars_p, TTXHndl, (S32*)&TTXHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected handle" );
        return(TRUE);
    }

    sprintf(TTX_Msg, "STTTX_Stop(%d): ", TTXHndl);
    ErrorCode = STTTX_Stop(TTXHndl);
    if(ErrorCode != ST_NO_ERROR)
    {
        sprintf(TTX_Msg, "%sFailed Error=%d!\n", TTX_Msg, ErrorCode);
        RetErr = TRUE;
    }
    else
    {
        strcat(TTX_Msg, "Ok\n");
        RetErr = FALSE;
        TaskList[0] = IMVBITask;
        if ( task_wait( TaskList, 1, TIMEOUT_INFINITY ) == -1 )
            STTST_Print(("Error: Timeout task_wait\n"));
        else
        {
            STTST_Print(("Telextext Insertion Task Ended\n"));
            if ( task_delete( IMVBITask ) != 0 )
            {
                STTST_Print(("Error: Couldn't delete TTX Insertion task!!\n"));
            }
        }
    }
    STTST_Print((TTX_Msg));
    return(RetErr);
}


/*-------------------------------------------------------------------------
 * Function : TTX_getRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTX_GetRevision( STTST_Parse_t *pars_p, char *result_sym_p )
{
ST_Revision_t RevisionNumber;

    RevisionNumber = STTTX_GetRevision( );
    sprintf( TTX_Msg, "STTTX_GetRevision(): Ok\revision=%s\n", RevisionNumber );
    STTST_Print(( TTX_Msg ));
    return ( FALSE );
} /* end TTX_GetRevision */

#endif /* TEST_WITH_TELETEXT_DRIVER */


#ifdef TEST_WITH_TELETEXT
/*-------------------------------------------------------------------------
 * Function : TTX_Inject
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/

static BOOL TTX_Inject( STTST_Parse_t *pars_p, char *result_sym_p )
{
    #if defined (ST_OS21)  || defined (ST_OSLINUX)
    #define TELETEXT_PLAYER_WORKSPACE       1024*16
    #define ST4629_BASE_ADDRESS  STI4629_ST40_BASE_ADDRESS
#ifdef ST_5528
    #define ST40_MASK                       0x1FFFFFFF
#endif
    #endif /*ST_OS21*/

    #ifdef ST_OS20
    #define TELETEXT_PLAYER_WORKSPACE       2048
    #define ST4629_BASE_ADDRESS  STI4629_BASE_ADDRESS
    #endif /* ST_OS20 */

#ifdef USE_AVMEM_ALLOC
    FILE * fstream_p;
    U32 ExpectedSize, size;
    void* DataAdd_p_p;
    void * AddDevice_p;

#ifdef ST_OSLINUX
    STVBI_AllocDataParams_t VbiAllocData;
#else
    STAVMEM_AllocBlockParams_t AllocBlockParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    STAVMEM_BlockHandle_t BlockHandle;
#endif
#endif
    BOOL RetErr;
    S32 LVar;
    S32 IsExternal;
    U32* TTXT_BASE_ADDRESS_Temp;
    char Msg[512];
    char FileName[127];

    #ifdef ST_OS20
    long FileDescriptor;
    #endif
    #if defined (ST_OS21)  || defined (ST_OSLINUX)
    long int FileDescriptor;
    #endif

    #if defined (ST_OS21)  || defined (ST_OSLINUX)
    struct stat FileStatus;
    #endif /* ST_OS21 */
    RetErr = FALSE;

    #if defined(mb376) || defined(espresso)
        STSYS_WriteRegDev8(ST4629_BASE_ADDRESS+TTXT_CFG, 0x1);/*******Configure TTXT_CFG for STi4629********/
    #endif


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
    #if defined(mb376) || defined(espresso)
        /* 8bits for STi4629 and 32bits for STi5528 */
        TTXT_BASE_ADDRESS_Temp =(IsExternal==1) ?(U32*)TTXT_BASE_ADDRESS_2 :(U32*)TTXT_BASE_ADDRESS ;
    #else
        TTXT_BASE_ADDRESS_Temp =(U32*)TTXT_BASE_ADDRESS;
    #endif

#ifdef USE_AVMEM_ALLOC

        fstream_p = fopen(FileName, "rb");
        if( fstream_p == NULL )
        {
            STTST_Print(("Not unable to open !!!\n"));
            return(TRUE);

        }
#if !defined ST_OSLINUX
        AllocBlockParams.PartitionHandle          = AvmemPartitionHandle[0];
        AllocBlockParams.Size                     = 3680;
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
        STAVMEM_GetBlockAddress( BlockHandle, &DataAdd_p_p);
        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
        STTST_Print(("Reading data ..."));
        size = fread (STAVMEM_VirtualToCPU(DataAdd_p_p,&VirtualMapping), 1, AllocBlockParams.Size, fstream_p);
        if (size != AllocBlockParams.Size)
        {
            STTST_Print(("Read error %d byte instead of %d !!!\n"));

           return(TRUE);

        }
        else
        {
            STTST_Print(("ok\n"));
       }
#else

        VbiAllocData.Size = 3680 ;
        VbiAllocData.Alignment = 256 ;
        STVBI_AllocData( VBIHndl, &VbiAllocData,
                            &DataAdd_p_p );

        if ( DataAdd_p_p == NULL )
        {
            STTBX_Print(("Error Allocating data for palette...\n"));
            return ST_ERROR_BAD_PARAMETER;
        }

        STTBX_Print(("Reading file Bitmap data Address data bitmap = 0x%x... \n", DataAdd_p_p ));
        size = fread ((void*)(DataAdd_p_p),1,
                      VbiAllocData.Size, fstream_p);


#endif

#ifdef ST_OSLINUX
     AddDevice_p = (void*) ((U32) STVBI_UserToKernel((U32)DataAdd_p_p) & MASK_STBUS_FROM_ST40);
#else
     AddDevice_p = (void*) STAVMEM_VirtualToDevice(DataAdd_p_p, &VirtualMapping);
#endif
     TtxInfo.Size = size ;
     TtxInfo.AlignBuffer = (char*) AddDevice_p;
#else
    #ifdef ST_OS20
    FileDescriptor = debugopen(FileName, "rb" );
    #endif
    #ifdef ST_OS21
    FileDescriptor = open(FileName, O_RDONLY|O_BINARY);
    #endif
    if (FileDescriptor< 0 )
    {
        sprintf(Msg, "unable to open %s ...\n", FileName);
        STTST_Print((Msg));
        return(TRUE);
    }

    #ifdef ST_OS20
    TtxInfo.Size = debugfilesize(FileDescriptor);
    #endif
    #ifdef ST_OS21
    fstat(FileDescriptor, &FileStatus);
    TtxInfo.Size= FileStatus.st_size;
    #endif

    /* Get memory to store file data */
    /* ---  round up "size" to a multiple of 16 bytes, and malloc it --- */

    TtxInfo.Buffer = (char *)memory_allocate(NCachePartition_p, (U32)((TtxInfo.Size + 15) & (~15)));

    if (TtxInfo.Buffer == NULL)
    {
        STTST_Print(( "Not enough memory for file loading !\n"));
        #ifdef ST_OS21
        close(FileDescriptor);
        #endif /* ST_OS21 */
        #ifdef ST_OS20
        debugclose(FileDescriptor);
        #endif /* ST_OS20 */
        return(TRUE);
    }
    TtxInfo.AlignBuffer = (void *)(((unsigned int)TtxInfo.Buffer + 15) & (~15));
    #ifdef ST_OS20
    debugread(FileDescriptor, TtxInfo.AlignBuffer, (size_t) TtxInfo.Size);
    debugclose(FileDescriptor);
    #endif
    #ifdef ST_OS21
    read(FileDescriptor, TtxInfo.AlignBuffer, (size_t) TtxInfo.Size);
    close(FileDescriptor);
    #endif

    #ifdef ST_OS21
    #if !defined (ARCHITECTURE_ST200)
    TtxInfo.AlignBuffer = (void*)((unsigned int)TtxInfo.AlignBuffer & ST40_MASK);
    #endif /*!ARCHITECTURE_ST200*/
    #endif  /** ST_OS21 **/

#endif  /** USE_AVMEM_ALLOC **/

    sprintf(Msg, "File [%s] loaded : size %ld at address %x \n",
            FileName, TtxInfo.Size, (int)TtxInfo.AlignBuffer);

    STTST_Print((Msg));

    /* Run Task */
    TtxInfo.Terminate = FALSE ;
    TtxInfo.TTXBaseAdress =(U32*)TTXT_BASE_ADDRESS_Temp;
    #ifndef ST_OSLINUX
    TtxInfo.TskId = task_create((void(*) (void *))TTX_TeletextTsk,
                                (void*) &TtxInfo,
                                (int) (TELETEXT_PLAYER_WORKSPACE),
                                (int) MIN_USER_PRIORITY,
                                "Teletext injector",
                                0);
    if (TtxInfo.TskId == NULL)
        STTST_Print(("Teletext task : failed to create\n"));
    else
        STTST_Print(("Teletext task : created\n"));

    #else
    pthread_attr_init(&TtxInfo.TaskAttribute);
    pthread_create(&TtxInfo.TskId,
                       &TtxInfo.TaskAttribute,
                       (void *)TTX_TeletextTsk,
                       (void*) &TtxInfo);

    #endif

    return(RetErr);
}

/*-------------------------------------------------------------------------
 * Function : TTX_KillInject
 *            Kill TTX Inject task
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL TTX_KillInject( STTST_Parse_t *pars_p, char *Result )
{

    #ifndef ST_OSLINUX
    task_t *TaskList[1];
        /* Reset global variable and wait for task to finish before deleting */
    TtxInfo.Terminate = TRUE;
    if ( TtxInfo.TskId != NULL )
    {
        TaskList[0] = TtxInfo.TskId;
        if ( task_wait( TaskList, 1, TIMEOUT_INFINITY ) == -1 )
            STTST_Print(("Error: Timeout task_wait\n"));
        else
        {
            STTST_Print(("Telextext injection Task Ended\n"));
            if ( task_delete( TtxInfo.TskId ) != 0 )
            {
                STTST_Print(("Error: Couldn't delete TTX injection task!!\n"));
            }
            else
            {
                TtxInfo.TskId = NULL;

            }
        }
    }
    #else

    int ret ;
    TtxInfo.Terminate = TRUE;
    if ( TtxInfo.TskId != 0 )
    {

            if ((ret=pthread_join(TtxInfo.TskId, NULL)) != 0)
            {
                printf("stvbi : task join problem (%d) !!!!\n", ret);
            }
            if ((ret=pthread_attr_destroy(&TtxInfo.TaskAttribute)) != 0)
            {
                printf("stvbi : task attr destroy problem (%d) !!!!\n", ret);
            }
            else
            {
                printf("stvbi : task killed.\n");
            }
    }
    #endif
    return FALSE;
} /* end TTX_KillInject */


static void TTX_TeletextTsk(InfoTtxTsk_t * Infos){
/*--- define the Teletext DMA operations here ----------------------------- */
#define TTXT_DMA_ADDRESS   0
#define TTXT_DMA_COUNT     1
#define TTXT_OUT_DELAY     2
#define TTXT_IN_START_LINE 3
#define TTXT_IN_CB_DELAY   4
#define TTXT_MODE          5
#define TTXT_INT_STATUS    6
#define TTXT_INT_ENABLE    7
#define TTXT_ACK_ODD_EVEN  8
#define TTXT_ABORT         9

#pragma ST_device(txtdma)

    unsigned long i,j,size;
    volatile unsigned int *txtdma = (volatile unsigned int *)Infos->TTXBaseAdress;
    unsigned long numlines,numdma;

    while (!Infos->Terminate) /* indefinite loop */
    {
        txtdma[TTXT_MODE]       = 0;  /* mode out                              */
        txtdma[TTXT_OUT_DELAY]  = 4;  /* 4 27MHz clocks                        */
        txtdma[TTXT_INT_ENABLE] = 0;  /* no interrupts please ( polling mode ) */
        txtdma[TTXT_ABORT]      = 1;  /* reset to insure everything is OK !    */

        numlines = Infos->Size / 46;
        numdma   = ( (numlines-1) / 44 ) +1;

        for( j=0 ;  j<(Infos->Loop) &&(!Infos->Terminate); j++)
        {
            for( i=0; (i<numdma) && (!Infos->Terminate); i++)
            {
                if (i == (numdma-1))
                {
                    size = (numlines%44)*46;
                }
                else
                {
                    size = 2024;
                }
                txtdma[TTXT_DMA_ADDRESS] = ( ( unsigned int ) Infos->AlignBuffer ) +
                    ( unsigned int ) ( i * 2024 ) ;
                txtdma[TTXT_DMA_COUNT]   = ( unsigned int ) size; /* writing starts the transfer */
                while ( (!Infos->Terminate) && ((txtdma[TTXT_INT_STATUS] & 0x1)==0x0 ))
                {
                    task_delay (1000);
                }
            }
        }
    } /* exit when finished */
    memory_deallocate (NCachePartition_p, Infos->Buffer );
}
#endif /* TEST_WITH_TELETEXT */

/*-------------------------------------------------------------------------
 * Function : TTX_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : FALSE if error, TRUE if success
 * ----------------------------------------------------------------------*/
static BOOL TTX_InitCommand(void)
{
    BOOL RetErr = FALSE;

    /* API functions : */
#ifdef TEST_WITH_TELETEXT_DRIVER
    RetErr  = STTST_RegisterCommand( "TTX_Init", TTX_Init, "<TTXDevName><VBIDevName> TTX Init");
    RetErr |= STTST_RegisterCommand( "TTX_Open", TTX_Open, "<TTXDevName> TTX Open");
    RetErr |= STTST_RegisterCommand( "TTX_Start", TTX_Start, "<Handle> TTX Start");
    RetErr |= STTST_RegisterCommand( "TTX_SetSource", TTX_SetSource, "<Handle><SourceType> TTX SetStreamID");
    RetErr |= STTST_RegisterCommand( "TTX_SetStreamID", TTX_SetStreamID, "<Handle><PID> TTX SetStreamID");
    RetErr |= STTST_RegisterCommand( "TTX_Stop", TTX_Stop, "<Handle> TTX Stop");
    RetErr |= STTST_RegisterCommand( "TTX_Close", TTX_Close, "<Handle> TTX Close");
    RetErr |= STTST_RegisterCommand( "TTX_Term", TTX_Term, "<TTXDevName> TTX Term");
    RetErr |= STTST_RegisterCommand( "TTX_Rev", TTX_GetRevision, "TTX Get Revision");

    RetErr |= STTST_AssignString ("TTX_DEVICE_NAME", STTTX_DEVICE_NAME, TRUE);
#endif /* TEST_WITH_TELETEXT_DRIVER */

#ifdef TEST_WITH_TELETEXT
    RetErr |= STTST_RegisterCommand( "TTX_Inject", TTX_Inject, "Launch teletext injection");
    RetErr |= STTST_RegisterCommand( "TTX_Kill", TTX_KillInject, "Kill teletext injection");
#endif
#if defined(mb376) || defined(espresso)
    RetErr |= STTST_RegisterCommand( "TTX_SetPIO6", TTXT_SetPIO6, "Fix PIO6 configuration for STi4629");
#endif

    return(!RetErr);

} /* end TTX_InitCommand */

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL TTX_RegisterCmd(void)
{
    BOOL RetOk;

    RetOk = TTX_InitCommand();
    if ( RetOk )
    {
#ifdef TEST_WITH_TELETEXT_DRIVER
        STTBX_Print(( "TTX_RegisterCmd()     \t: ok           %s\n", STTTX_GetRevision()));
#else  /* TEST_WITH_TELETEXT_DRIVER */
#ifdef TEST_WITH_TELETEXT
        STTBX_Print(( "TTX_RegisterCmd()     \t: ok\n" ));
#else
        STTBX_Print(( "TTX_RegisterCmd()     \t: None\n" ));
#endif /* TEST_WITH_TELETEXT */
#endif /* TEST_WITH_TELETEXT_DRIVER */
    }
    else
    {
        STTBX_Print(( "TTX_RegisterCmd()     \t: failed !\n" ));
    }
    return(RetOk);
}

#endif /* USE_TTX */
/* end of ttx_cmd.c */
