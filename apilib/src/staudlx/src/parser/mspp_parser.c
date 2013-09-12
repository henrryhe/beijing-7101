/************************************************************************
*   COPYRIGHT STMicroelectronics (C) 2006
*   Source file name    : mspp_parser.c
*   By              : Nitin Mahajan
*   Description         : The file contains the streaming MME based Audio Parser APIs that will be          *
*                     exported to the Modules.
************************************************************************/

/* {{{ Includes */
//#define  STTBX_PRINT
#include "stos.h"
#include "mspp_parser.h"
#include "acc_multicom_toolbox.h"
#include "aud_pes_es_parser.h"

#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif


/* ----------------------------------------------------------------------------
                               Private Types
---------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
                        External Data Types
--------------------------------------------------------------------------- */

/*---------------------------------------------------------------------
                    Private Functions
----------------------------------------------------------------------*/
#define DUMP_OUTPUT_TO_FILE 0
#if DUMP_OUTPUT_TO_FILE
    #define FRAMES  300
    char OutBuf[1152*4*2*FRAMES];
#endif

#define DUMP_INPUT_TO_FILE 0

#define  DUMP_PES_OUTPUT_TO_FILE 0
#if DUMP_PES_OUTPUT_TO_FILE
    #define FRAMES  300
    char OutBuf[1152*4*2*FRAMES];
#endif

ST_ErrorCode_t  Mspp_Parser_Start   (PESES_ParserControl_t *PESESControl_p);
ST_ErrorCode_t  Mspp_Parse_Frame    (PESES_ParserControl_t *PESESControl_p);
ST_ErrorCode_t  Mspp_Parser_Stop    (PESES_ParserControl_t *PESESControl_p);
ST_ErrorCode_t  Mspp_Parser_Term    (PESES_ParserControl_t *PESESControl_p);

/************************************************************************************
Name         : Mspp_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESES_ParserControl_t  Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t Mspp_Parser_Start(PESES_ParserControl_t *PESESControl_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    MspParser_Handle_t  Handle = (MspParser_Handle_t)PESESControl_p->Mspp_Cntrl_p;
    MsppControlBlock_t  *Control_p;
    Control_p = (MsppControlBlock_t *)Handle;

    /* Set Input processing state */
    if(Control_p->ByPass == FALSE)
    {
        Control_p->ParseInState = MME_PARSER_INPUT_STATE_IDLE;
    }
    else
    {
        Control_p->ParseInState = MME_PARSER_INPUT_STATE_BYPASS_DATA;
    }
    /* Reset parser queues */
    Control_p->ParserInputQueueFront = Control_p->ParserInputQueueTail = 0;
    Control_p->ParserOutputQueueFront = Control_p->ParserOutputQueueTail = 0;
    Control_p->Mpeg1_LayerSet = TRUE;
    Control_p->MarkSentEof = FALSE;

    /* Audio transformer was already started in set stream type*/
    /*Error = STAud_SetAndInitParser(Control_p);
    if(Error != MME_SUCCESS)
    {
        STTBX_Print(("Audio Parser:Init failure\n"));
    }
    else
    {
        STTBX_Print(("Audio Parser:Init Success\n"));
    }*/
    Error |= ResetMsppQueueParams(Control_p);
    return Error;
}

/******************************************************************************
 *  Function name   : STAud_MsppInitParserQueueInParams
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Initalize Decoder queue params
 *                1. Allocate and initialize MME structure for input buffers
 *                2. Allocate and initialize MME structure for Output buffers
 ***************************************************************************** */
ST_ErrorCode_t STAud_MsppInitParserQueueInParams(MsppControlBlock_t *Control_p,
                                                            STAVMEM_PartitionHandle_t AVMEMPartition)
{
    /* Local Variables */
    void                *VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STAVMEM_AllocBlockParams_t              AllocParams;
    U32 i, j;
    U32 addresscount = 0;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    /**************** Initialize Input param ********************/
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    AllocParams.PartitionHandle = AVMEMPartition;
    AllocParams.Alignment = 32;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.ForbiddenRangeArray_p = NULL;
    AllocParams.ForbiddenBorderArray_p = NULL;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.Size = sizeof(MME_DataBuffer_t) * (STREAMING_PARSER_INPUT_QUEUE_SIZE + 1);

    Error = STAVMEM_AllocBlock(&AllocParams, &Control_p->BufferInHandle0);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 1 %x",Error));
        return Error;
    }
    Error = STAVMEM_GetBlockAddress(Control_p->BufferInHandle0, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 2 %x",Error));
        return Error;
    }
    for(i = 0; i <=STREAMING_PARSER_INPUT_QUEUE_SIZE; i++)
    {
        Control_p->ParserInputQueue[i].ParserBufIn.BufferIn_p = STAVMEM_VirtualToCPU(((char*)VirtualAddress + (i * sizeof(MME_DataBuffer_t))),&VirtualMapping);
    }

    AllocParams.Size = sizeof(MME_ScatterPage_t) * NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM * (STREAMING_PARSER_INPUT_QUEUE_SIZE + 1);
    Error = STAVMEM_AllocBlock(&AllocParams, &Control_p->ScatterPageInHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 1 %x",Error));
        return Error;
    }
    Error = STAVMEM_GetBlockAddress(Control_p->ScatterPageInHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 2 %x",Error));
        return Error;
    }

    for(i = 0; i <=STREAMING_PARSER_INPUT_QUEUE_SIZE; i++)
    {
        ParserInBufParams_t * ParserBuf_p = &(Control_p->ParserInputQueue[i].ParserBufIn);
        MME_DataBuffer_t    * Buffer_p = ParserBuf_p->BufferIn_p;
        for (j = 0; j < NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            MME_ScatterPage_t * ScatterPage_p = NULL;

            ScatterPage_p = STAVMEM_VirtualToCPU ((char*)VirtualAddress + (sizeof(MME_ScatterPage_t) * addresscount),&VirtualMapping);
            /* We will set the Page_p to point to the compressed buffer, received from
                Parser unit, in Process_Input function */
            ParserBuf_p->ScatterPageIn_p[j] = ScatterPage_p;

            ScatterPage_p->Page_p    = NULL;
            /* Data Buffers initialisation */
            ScatterPage_p->Size      = 0;
            ScatterPage_p->BytesUsed = 0;
            ScatterPage_p->FlagsIn   = (unsigned int)MME_DATA_CACHE_COHERENT;
            ScatterPage_p->FlagsOut  = 0;
            addresscount++;
        }

        Buffer_p->StructSize           = sizeof (MME_DataBuffer_t);
        Buffer_p->UserData_p           = NULL;
        Buffer_p->Flags                = 0;
        Buffer_p->StreamNumber         = 0;
        // TBD needs to be updated
        Buffer_p->NumberOfScatterPages = 0;
        Buffer_p->ScatterPages_p       = ParserBuf_p->ScatterPageIn_p[0];
        Buffer_p->TotalSize            = 0;
        Buffer_p->StartOffset          = 0;
    }

    return ST_NO_ERROR;
}


/******************************************************************************
 *  Function name   : STAud_MsppInitParserQueueOutParams
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Initalize parser queue params
 *                1. Allocate and initialize MME structure for Output buffers
 ***************************************************************************** */
ST_ErrorCode_t STAud_MsppInitParserQueueOutParams(MsppControlBlock_t    *Control_p,
                                                STAVMEM_PartitionHandle_t AVMEMPartition)
{
    /* Local Variables */
    void                *VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    U32 i,j;
    U32 addresscount = 0;
    ST_ErrorCode_t Error = ST_NO_ERROR;

      STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    AllocBlockParams.PartitionHandle = AVMEMPartition;
    AllocBlockParams.Alignment = 32;
    AllocBlockParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.ForbiddenRangeArray_p = NULL;
    AllocBlockParams.ForbiddenBorderArray_p = NULL;
    AllocBlockParams.NumberOfForbiddenRanges = 0;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.Size = sizeof(MME_DataBuffer_t) * (STREAMING_PARSER_OUTPUT_QUEUE_SIZE);

    /* For output buffer 0*/
    Error = STAVMEM_AllocBlock(&AllocBlockParams, &Control_p->BufferOutHandle0);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 1 %x",Error));
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(Control_p->BufferOutHandle0, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 2 %x",Error));
        return Error;
    }

    for(i = 0; i < STREAMING_PARSER_OUTPUT_QUEUE_SIZE; i++)
    {
        Control_p->ParserOutputQueue[i].ParserBufOut.BufferOut_p = STAVMEM_VirtualToCPU(((char*)VirtualAddress + (i * sizeof(MME_DataBuffer_t))),&VirtualMapping);
    }


    /* For output scatter pages for buffer 0*/
    AllocBlockParams.Size = sizeof(MME_ScatterPage_t) * NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM * STREAMING_PARSER_OUTPUT_QUEUE_SIZE;
    Error = STAVMEM_AllocBlock(&AllocBlockParams, &Control_p->ScatterPageOutHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 3 %x",Error));
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(Control_p->ScatterPageOutHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 4 %x",Error));
        return Error;
    }

    for(i = 0; i < STREAMING_PARSER_OUTPUT_QUEUE_SIZE; i++)
    {
        ParserOutBufParams_t    * ParserBuf_p = &(Control_p->ParserOutputQueue[i].ParserBufOut);
        MME_DataBuffer_t        * Buffer_p = ParserBuf_p->BufferOut_p;
        for (j = 0; j < NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            MME_ScatterPage_t * ScatterPage_p = NULL;

            ScatterPage_p = STAVMEM_VirtualToCPU((char*)VirtualAddress + (sizeof(MME_ScatterPage_t) * addresscount),&VirtualMapping);
            ParserBuf_p->ScatterPageOut_p[j] = ScatterPage_p;
            /* We will set the Page_p to point to the Empty PCM buffer, received from
            Consumer unit, in Process_Input function */

            ScatterPage_p->Page_p    = NULL;
            /* Data Buffers initialisation */
            ScatterPage_p->Size      = 0;
            ScatterPage_p->BytesUsed = 0;
            ScatterPage_p->FlagsIn   = (unsigned int)MME_DATA_CACHE_COHERENT;
            ScatterPage_p->FlagsOut  = 0;
            addresscount++;
        }

        Buffer_p->StructSize           = sizeof (MME_DataBuffer_t);
        Buffer_p->UserData_p           = NULL;
        Buffer_p->Flags                = 0;
        Buffer_p->StreamNumber         = 0;
        Buffer_p->NumberOfScatterPages = 0;
        Buffer_p->ScatterPages_p       = ParserBuf_p->ScatterPageOut_p[0];
        Buffer_p->TotalSize            = 0;
        Buffer_p->StartOffset          = 0;

    }

    return ST_NO_ERROR;

}

void    STAud_MsppInitParserFrameParams(MsppControlBlock_t *Control_p)
{
    U32 i;

    for(i = 0; i <STREAMING_PARSER_OUTPUT_QUEUE_SIZE; i++)
    {
        Control_p->ParserOutputQueue[i].FrameParams.Restart  = FALSE;
        Control_p->ParserOutputQueue[i].FrameParams.Cmd  = ACC_CMD_PLAY;
        /*Other fiels to be updated with respect to parser */
    }
}

/******************************************************************************
 *  Function name   : STAud_MsppDeInitParserQueueInParam
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : De allocate parser queue params
 *                1. Deallocate Input buffers
 ***************************************************************************** */
ST_ErrorCode_t  STAud_MsppDeInitParserQueueInParam(MsppControlBlock_t *Control_p)
{

    ST_ErrorCode_t  Error=ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    STAVMEM_BlockHandle_t   ScatterPageInHandle,BufferInHandle;

    FreeBlockParams.PartitionHandle = Control_p->InitParams.AVMEMPartition;

    ScatterPageInHandle =   Control_p->ScatterPageInHandle;
    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&ScatterPageInHandle);

    /*Set the handle to NULL for safety after deallocation*/
    Control_p->ScatterPageInHandle = (STAVMEM_BlockHandle_t)NULL ;

    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("4 E = %x",Error));
    }
    BufferInHandle = Control_p->BufferInHandle0;

    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&BufferInHandle);

    /*Set the handle to NULL for safety after deallocation*/
    Control_p->BufferInHandle0 = (STAVMEM_BlockHandle_t)NULL ;

    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("5 E = %x",Error));
    }
    return Error;
}


/******************************************************************************
 *  Function name   : STAud_MsppDeInitParserQueueOutParam
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : De allocate parser queue params
 *                1. Deallocate output buffers
 ***************************************************************************** */
ST_ErrorCode_t  STAud_MsppDeInitParserQueueOutParam(MsppControlBlock_t *Control_p)
{
    ST_ErrorCode_t  Error=ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    //U32     i;
    STAVMEM_BlockHandle_t   ScatterPageHandle,BufferHandle;
    FreeBlockParams.PartitionHandle = Control_p->InitParams.AVMEMPartition;

    ScatterPageHandle =     Control_p->ScatterPageOutHandle;
    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&ScatterPageHandle);

    /*Set the handle to NULL for safety after deallocation*/
    Control_p->ScatterPageOutHandle = (STAVMEM_BlockHandle_t)NULL ;

    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("7 E = %x",Error));
    }

    BufferHandle = Control_p->BufferOutHandle0;
    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&BufferHandle);

    /*Set the handle to NULL for safety after deallocation*/
    Control_p->BufferOutHandle0 = (STAVMEM_BlockHandle_t)NULL ;

    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print((" 9 E = %x",Error));
    }
    return Error;
}

/******************************************************************************
*                       LOCAL FUNCTIONS                                        *
******************************************************************************/
ST_ErrorCode_t  ResetMsppQueueParams(MsppControlBlock_t *Control_p)
{
    U32 i,j;
    // Reset Input Queue params
    for(i = 0; i <= STREAMING_PARSER_INPUT_QUEUE_SIZE; i++)
    {
        ParserInBufParams_t * ParserBuf_p = &(Control_p->ParserInputQueue[i].ParserBufIn);
        MME_DataBuffer_t    * Buffer_p = ParserBuf_p->BufferIn_p;

        for (j = 0; j < NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            MME_ScatterPage_t * ScatterPage_p = ParserBuf_p->ScatterPageIn_p[j];
            /* We will set the Page_p to point to the compressed buffer, received from
                Parser unit, in Process_Input function */

            ScatterPage_p->Page_p = NULL;

            /* Data Buffers initialisation */
            ScatterPage_p->Size      = 0;
            ScatterPage_p->BytesUsed = 0;
            ScatterPage_p->FlagsIn   = (unsigned int)MME_DATA_CACHE_COHERENT;
            ScatterPage_p->FlagsOut  = 0;
        }

        Buffer_p->StructSize           = sizeof (MME_DataBuffer_t);
        Buffer_p->UserData_p           = NULL;
        Buffer_p->Flags                = 0;
        Buffer_p->StreamNumber         = 0;
        Buffer_p->NumberOfScatterPages = 0;
        Buffer_p->ScatterPages_p       = ParserBuf_p->ScatterPageIn_p[0];
        Buffer_p->TotalSize            = 0;
        Buffer_p->StartOffset          = 0;
    }

    for(i = 0; i <STREAMING_PARSER_OUTPUT_QUEUE_SIZE; i++)
    {
        ParserOutBufParams_t * ParserBuf_p = &(Control_p->ParserOutputQueue[i].ParserBufOut);
        MME_DataBuffer_t    * Buffer_p = ParserBuf_p->BufferOut_p;

        for (j = 0; j < NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            MME_ScatterPage_t * ScatterPage_p = ParserBuf_p->ScatterPageOut_p[j];
            /* We will set the Page_p to point to the Empty PCM buffer, received from
                Consumer unit, in Process_Input function */

            ScatterPage_p->Page_p    = NULL;
            /* Data Buffers initialisation */
            ScatterPage_p->Size      = 0;
            ScatterPage_p->BytesUsed = 0;
            ScatterPage_p->FlagsIn   = (unsigned int)MME_DATA_CACHE_COHERENT;
            ScatterPage_p->FlagsOut  = 0;
            ScatterPage_p->Page_p    = NULL;
        }

        Buffer_p->StructSize           = sizeof (MME_DataBuffer_t);
        Buffer_p->UserData_p           = NULL;
        Buffer_p->Flags                = 0;
        Buffer_p->StreamNumber         = 0;
        Buffer_p->NumberOfScatterPages = 0;
        Buffer_p->ScatterPages_p       = ParserBuf_p->ScatterPageOut_p[0];
        Buffer_p->TotalSize            = 0;
        Buffer_p->StartOffset          = 0;
    }

    Control_p->SamplingFrequency = ACC_FS_reserved;
    Control_p->AudioCodingMode = ACC_MODE_undefined_3F;

    return ST_NO_ERROR;
}


/************************************************************************************
Name         : Mspp_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : PESParserInit_t   *Init_p     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t Mspp_Parser_Init(MspParserInit_t *Init_p, MspParser_Handle_t *Handle_p)

{
    MsppControlBlock_t * Control_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTBX_Print((" Mspp_Parser Init Entry \n"));

    Control_p = (MsppControlBlock_t *)STOS_MemoryAllocate (Init_p->Memory_Partition, sizeof(MsppControlBlock_t));
    if(Control_p==NULL)
    {
        STTBX_Print((" Mspp_Parser Init No Memory \n"));
        return ST_ERROR_NO_MEMORY;
    }
    /* Reset the structure */
    memset((void *)Control_p, 0,sizeof(MsppControlBlock_t));
    //Control_p->MMEinitParams.Priority = MME_PRIORITY_NORMAL;
    Control_p->MMEinitParams.Priority = MME_PRIORITY_BELOW_NORMAL;
    Control_p->MMEinitParams.StructSize = sizeof (MME_TransformerInitParams_t);
    Control_p->MMEinitParams.Callback = &TransformerCallback_AudioParser;
    Control_p->MMEinitParams.CallbackUserData = NULL; /* To be Initialized in STAud_SetAndInitParser */
    Control_p->MMEinitParams.TransformerInitParamsSize = sizeof(MME_AudioParserInitParams_t);
    Control_p->MMEinitParams.TransformerInitParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(MME_AudioParserInitParams_t));

    if(Control_p->MMEinitParams.TransformerInitParams_p == NULL)
    {
        /*Free the Control Block */
        STOS_MemoryDeallocate(Init_p->Memory_Partition, Control_p);

        STTBX_Print((" Mspp_Parser Init No Memory \n"));
        return ST_ERROR_NO_MEMORY;
    }

    memset((void *)Control_p->MMEinitParams.TransformerInitParams_p, 0,
                        sizeof(MME_AudioParserInitParams_t));
    /* initialize and allocate parser input queue's buffer pointer */
    Error |=STAud_MsppInitParserQueueInParams(Control_p, Init_p->AVMEMPartition);
    if(Error != ST_NO_ERROR)
    {
        /* Free the MME Param Structure */
        STOS_MemoryDeallocate(Init_p->Memory_Partition, Control_p->MMEinitParams.TransformerInitParams_p);

        /*Free the Control Block */
        STOS_MemoryDeallocate(Init_p->Memory_Partition, Control_p);
        return ST_ERROR_NO_MEMORY;
    }

    /* initialize and allocate parser output queue's buffer pointer */
    Error |=STAud_MsppInitParserQueueOutParams(Control_p, Init_p->AVMEMPartition);
    if(Error != ST_NO_ERROR)
    {
        /*Free Parser Input Queue's buffer pointer*/
        STAud_MsppDeInitParserQueueInParam(Control_p);

        /* Free the MME Param Structure */
        STOS_MemoryDeallocate(Init_p->Memory_Partition, Control_p->MMEinitParams.TransformerInitParams_p);

        /*Free the Control Block */
        STOS_MemoryDeallocate(Init_p->Memory_Partition, Control_p);
        return ST_ERROR_NO_MEMORY;
    }

    STAud_MsppInitParserFrameParams(Control_p);

    /* Set Input processing state */
    Control_p->ParseInState = MME_PARSER_INPUT_STATE_IDLE;

    /* Reset parser queues */
    Control_p->ParserInputQueueFront = Control_p->ParserInputQueueTail=0;
    Control_p->ParserOutputQueueFront = Control_p->ParserOutputQueueTail=0;
    Control_p->Last_Parsed_Size = 1152;
    Control_p->MpegLayer = (eMpegLayer_t) MPG_LAYER2;
    Control_p->Mpeg1_LayerSet = TRUE;
    Control_p->MarkSentEof = FALSE;
    Control_p->ByPass = FALSE;

    /* Initialize the Parser Capability structure */
    STAud_InitParserCapability(Control_p);

    Control_p->EventIDNewFrame = Init_p ->EventIDNewFrame;
    Control_p->EventIDFrequencyChange = Init_p ->EventIDFrequencyChange;
    Control_p->EvtHandle = Init_p ->EvtHandle;
    Control_p->ObjectID = Init_p ->ObjectID;

    Control_p->OutMemoryBlockManagerHandle=Init_p->OpBufHandle;
    Control_p->PESESBytesConsumedcallback = (PESES_ConsumedByte_t) Init_p->PESESBytesConsumedcallback;
    Control_p->PESESHandle                = Init_p->PESESHandle;

    Control_p->InitParams=*Init_p;
    Control_p->Handle= (MspParser_Handle_t)Control_p;

    /* Create the command list Mutex */
    Control_p->GlobalCmdMutex_p = STOS_MutexCreateFifo();

    /*Allocate memory for Bypass Params if bypass mode is true.*/
    Control_p->BypassParams = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(MspBypassParams_t));
    if(Control_p->BypassParams == NULL)
    {
        STOS_MemoryDeallocate(Init_p->Memory_Partition, Control_p->BypassParams);

        /*Free Parser Input Queue's buffer pointer*/
        STAud_MsppDeInitParserQueueInParam(Control_p);

        /* Free the MME Param Structure */
        STOS_MemoryDeallocate(Init_p->Memory_Partition, Control_p->MMEinitParams.TransformerInitParams_p);

        /*Free the Control Block */
        STOS_MemoryDeallocate(Init_p->Memory_Partition, Control_p);

        return ST_ERROR_NO_MEMORY;
    }
    memset(Control_p->BypassParams,0,sizeof(MspBypassParams_t));


    Init_p->Handle_p = (MspParser_Handle_t *)Control_p;
    *Handle_p = (MspParser_Handle_t)Init_p->Handle_p;/*control block handle*/
    STTBX_Print((" Mspp_Parser Init Exit SuccessFull \n "));
    return ST_NO_ERROR;
}


/************************************************************************************
Name         : Mspp_Parse_Frame()

Description  : This is called from aud_pes_es_parser.c  for the parsing

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
        ST_NO_ERROR
************************************************************************************/

ST_ErrorCode_t Mspp_Parse_Frame(PESES_ParserControl_t *PESESControl_p)
{
    ST_ErrorCode_t      Error=ST_NO_ERROR;
    MspParser_Handle_t  Handle = (MspParser_Handle_t)PESESControl_p->Mspp_Cntrl_p;
    MsppControlBlock_t  *Control_p;

    MME_ERROR           status;
    MME_DataBuffer_t    *IPDataBuffer_p,*OPDataBuffer_p;

    U32     QueueTailIndex;

    U32     *No_Bytes_Used = &PESESControl_p->BytesUsed;
    U32     Size = (PESESControl_p->PESBlock.ValidSize - PESESControl_p->BytesUsed);
    U32     Offset=0;
    void    *MemoryStart =(void*)(PESESControl_p->PESBlock.StartPointer+(U32)(*No_Bytes_Used ));
    U8      Num_IPScatterPages = 0,Num_OPScatterPages = 0;

    //STTBX_Print(("\nMspp_Parse_Frame %d\n",Size));
    /*Retrive MSPP parser control block from handle*/
    Control_p=(MsppControlBlock_t *)Handle;
    {
        switch(Control_p->ParseInState)
        {
            case MME_PARSER_INPUT_STATE_IDLE:
                {
                    // Intended fall through
                    Control_p->ParseInState = MME_PARSER_INPUT_STATE_GET_INTPUT_DATA;
                }

            case MME_PARSER_INPUT_STATE_GET_INTPUT_DATA:
                {
                    //Check the size and send the input buffer to the LX
                    if (Size)
                    {
                        if (Control_p->ParserInputQueueTail - Control_p->ParserInputQueueFront < STREAMING_PARSER_INPUT_QUEUE_SIZE)
                        {
                            //queue not full lets send another command with input data for parsing
                            QueueTailIndex = Control_p->ParserInputQueueTail % STREAMING_PARSER_INPUT_QUEUE_SIZE;
                            Control_p->CurrentInputBufferToSend = &Control_p->ParserInputQueue[QueueTailIndex];
                            IPDataBuffer_p = Control_p->CurrentInputBufferToSend->ParserBufIn.BufferIn_p;
                            Num_IPScatterPages = IPDataBuffer_p->NumberOfScatterPages;
                            while ((Num_IPScatterPages != Control_p->NumInputScatterPagesPerTransform))
                            {
                                IPDataBuffer_p->ScatterPages_p[Num_IPScatterPages].Page_p = (void*)((U32)MemoryStart+Offset);
                                IPDataBuffer_p->ScatterPages_p[Num_IPScatterPages].Size = Size;
                                IPDataBuffer_p->TotalSize += Size;
                                Offset += Size;
                                *No_Bytes_Used += Size;
                                //STTBX_Print(("\nsize sent to lx %d\n",IPDataBuffer_p->ScatterPages_p[Num_IPScatterPages].Size));
#if DUMP_INPUT_TO_FILE

                                FILE *f1 = NULL;
                                if(f1==NULL)
                                {
                                    f1 = fopen("C:\\dump_before_mmesend.dat","ab+");
                                }
                                if(f1)
                                {
                                    fwrite(IPDataBuffer_p->ScatterPages_p[Num_IPScatterPages].Page_p,
                                    IPDataBuffer_p->ScatterPages_p[Num_IPScatterPages].Size,1,f1);
                                    fclose(f1);
                                }
#endif
                                Num_IPScatterPages++;

                            }

                            if ((Num_IPScatterPages == Control_p->NumInputScatterPagesPerTransform))
                            {
                                IPDataBuffer_p->NumberOfScatterPages = Num_IPScatterPages;

                                // update audio parser command params
                                status = acc_setAudioParserBufferCmd(&Control_p->CurrentInputBufferToSend->Cmd,
                                                                    &Control_p->CurrentInputBufferToSend->BufferParams,
                                                                    sizeof (MME_AudioParserBufferParams_t),
                                                                    &Control_p->CurrentInputBufferToSend->FrameStatus,
                                                                    sizeof (MME_AudioParserFrameStatus_t),
                                                                    &(Control_p->CurrentInputBufferToSend->ParserBufIn.BufferIn_p),
                                                                    ACC_CMD_PLAY, FALSE,FALSE,Control_p->StreamContent,
                                                                    ACC_MME_FALSE);

                                //STTBX_Print(("\nPage address %p size %d Number of Scatter pages %d \n",IPDataBuffer_p->ScatterPages_p[Num_IPScatterPages-1].Page_p,Control_p->CurrentInputBufferToSend->ParserBufIn.BufferIn_p->ScatterPages_p[Num_IPScatterPages-1].Size,Num_IPScatterPages));
                                status = MME_SendCommand(Control_p->TranformHandleParser, &Control_p->CurrentInputBufferToSend->Cmd);
                                Control_p->CurrentInputBufferToSend = NULL;

                                // Update parser tail
                                Control_p->ParserInputQueueTail++;
                            }

                        }

                    }
                    //      Control Reaches here under following conditions:
                    //      a   Input data size recieved is 0 byte so no input data to send
                    //      b   Full input chunk has been sent to Lx
                    //      c   Non zero size recieved and still full input chunk is not sent but input queue is full, require Lx to release chunk
                    //      In all above cases try to send available output blocks to Lx, that is why
                    //      intended fall through


                    Control_p->ParseInState = MME_PARSER_INPUT_STATE_GET_OUTPUT_BUFFER;
                }

        case MME_PARSER_INPUT_STATE_GET_OUTPUT_BUFFER:
            {
                BOOL SendMMECommand = TRUE;

                while (SendMMECommand)
                {
                    SendMMECommand = FALSE;

                    /*dont request output blocks if recieved input size is zero bytes: required end of file handling
                       Send output buffers until input blocks sent and recieved are not equal*/
                    if (Control_p->ParserOutputQueueTail - Control_p->ParserOutputQueueFront < STREAMING_PARSER_OUTPUT_QUEUE_SIZE
                        && (Control_p->ParserInputQueueTail != Control_p->ParserInputQueueFront))
                    {
                        // We have space to send another command
                        QueueTailIndex = Control_p->ParserOutputQueueTail%STREAMING_PARSER_OUTPUT_QUEUE_SIZE;
                        Control_p->CurrentOutputBufferToSend = &Control_p->ParserOutputQueue[QueueTailIndex];
                        OPDataBuffer_p = Control_p->CurrentOutputBufferToSend->ParserBufOut.BufferOut_p;
                        Num_OPScatterPages = OPDataBuffer_p->NumberOfScatterPages;

                        while ((Num_OPScatterPages != Control_p->NumOutputScatterPagesPerTransform) &&
                                (MemPool_GetOpBlk(Control_p->OutMemoryBlockManagerHandle, (U32 *)&Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Num_OPScatterPages]) == ST_NO_ERROR))
                        {
                            OPDataBuffer_p->ScatterPages_p[Num_OPScatterPages].Page_p = (void*)Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Num_OPScatterPages]->MemoryStart;
                            OPDataBuffer_p->ScatterPages_p[Num_OPScatterPages].Size = Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Num_OPScatterPages]->MaxSize;//Size;
                            OPDataBuffer_p->ScatterPages_p[Num_OPScatterPages].BytesUsed = 0;
                            OPDataBuffer_p->TotalSize += Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Num_OPScatterPages]->MaxSize;//Size;
                            Num_OPScatterPages++;
                        }
                        if ((Num_OPScatterPages == Control_p->NumOutputScatterPagesPerTransform))
                        {
                            OPDataBuffer_p->NumberOfScatterPages = Num_OPScatterPages;
                            /* update command params */
                            status = acc_setAudioParserTransformCmd(&Control_p->CurrentOutputBufferToSend->Cmd,
                                                                        &Control_p->CurrentOutputBufferToSend->FrameParams,
                                                                        sizeof (MME_AudioParserFrameParams_t),
                                                                        &Control_p->CurrentOutputBufferToSend->FrameStatus,
                                                                        sizeof (MME_AudioParserFrameStatus_t),
                                                                        &(Control_p->CurrentOutputBufferToSend->ParserBufOut.BufferOut_p),
                                                                        PARSE_CMD_PLAY, 0, FALSE,
                                                                        0, 1);
                            status = MME_SendCommand(Control_p->TranformHandleParser, &Control_p->CurrentOutputBufferToSend->Cmd);
                            Control_p->CurrentOutputBufferToSend = NULL;
                            /* Update parser tail */
                            Control_p->ParserOutputQueueTail++;
                            SendMMECommand = TRUE;
                        }
                        else
                        {
                            /*STTBX_Print(("\nNo output block"));*/
                            Control_p->ParseInState = MME_PARSER_INPUT_STATE_IDLE;
                            return ST_ERROR_NO_MEMORY;
                        }
                    }
                }

                /*
                    Control Reaches here under following conditions:
                    a   Output queue is full i.e. no space to increment tail and Lx need to release blocks with it
                    b   when either no output blocks are available on request from block manager : Return and next time try to get output blocks
                    c   Input Queue tail and front has a difference of one (EOF implementation) so dont send anymore output blocks to Lx
                */
                Control_p->ParseInState = MME_PARSER_INPUT_STATE_IDLE;
                break;
            }


        //Modified by Bineet for handling secure mode transfer
        case MME_PARSER_INPUT_STATE_BYPASS_DATA :
        {
            MspBypassParams_t * LocalBypassParams = Control_p->BypassParams;

            while (Offset < Size)
            {
                if (!LocalBypassParams->OpBlk_p)
                {
                    //Take a output queue element

                    QueueTailIndex = Control_p->ParserOutputQueueTail%STREAMING_PARSER_OUTPUT_QUEUE_SIZE;
                    Control_p->CurrentOutputBufferToSend = &Control_p->ParserOutputQueue[QueueTailIndex];
                    OPDataBuffer_p = Control_p->CurrentOutputBufferToSend->ParserBufOut.BufferOut_p;
                    Num_OPScatterPages = OPDataBuffer_p->NumberOfScatterPages;

                    Error = MemPool_GetOpBlk(Control_p->OutMemoryBlockManagerHandle, (U32 *)&Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Num_OPScatterPages]);
                    if(Error != ST_NO_ERROR)
                    {
                        return STAUD_MEMORY_GET_ERROR;
                    }

                    LocalBypassParams->OpBlk_p = Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[(Num_OPScatterPages)];

                    LocalBypassParams->NoBytesCopied = 0;

                    LocalBypassParams->Current_Write_Ptr1 = (U8 *)LocalBypassParams->OpBlk_p->MemoryStart;
                }

                //copy the data into it

                if((Size-Offset)>=(LocalBypassParams->FrameSize - LocalBypassParams->NoBytesCopied))
                {


#ifdef PES_TO_ES_BY_FDMA


                    PESESControl_p->NodePESToES_p->Node.NumberBytes = (LocalBypassParams->FrameSize - LocalBypassParams->NoBytesCopied);
                    PESESControl_p->NodePESToES_p->Node.SourceAddress_p = (void *)((U32) ((U8 *)MemoryStart+Offset) & 0x1FFFFFFF);
                    PESESControl_p->NodePESToES_p->Node.DestinationAddress_p = (void *)((U32) LocalBypassParams->Current_Write_Ptr1 & 0x1FFFFFFF);
                    PESESControl_p->NodePESToES_p->Node.Length = PESESControl_p->NodePESToES_p->Node.NumberBytes;
                    PESESControl_p->PESTOESTransferParams.NodeAddress_p = (STFDMA_GenericNode_t *)((U32)(PESESControl_p->PESTOESTransferParams.NodeAddress_p) & 0x1FFFFFFF);

                    Error=STFDMA_StartGenericTransfer(&PESESControl_p->PESTOESTransferParams,&PESESControl_p->PESTOESTransferID);
                    if(Error!=ST_NO_ERROR)
                    {
                        return Error;
                    }

#else
                    memcpy(LocalBypassParams->Current_Write_Ptr1,((char *)MemoryStart+Offset),(LocalBypassParams->FrameSize - LocalBypassParams->NoBytesCopied));
#endif

                    //and send it to the next unit

                    LocalBypassParams->OpBlk_p->Size = LocalBypassParams->FrameSize;

                    Error = MemPool_PutOpBlk(Control_p->OutMemoryBlockManagerHandle,(U32 *)&LocalBypassParams->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" Error in Memory Put_Block for MSPP Parser\n"));
                        return Error;
                    }
                    else
                    {
                        LocalBypassParams->OpBlk_p = NULL;
                        STTBX_Print((" BYPASSED & Delivered:%d\n",LocalBypassParams->Blk_Out));
                        LocalBypassParams->Blk_Out++;
                        (PESES_BytesConsumed_t)Control_p->PESESBytesConsumedcallback(Control_p->PESESHandle,(U32)(LocalBypassParams->FrameSize - LocalBypassParams->NoBytesCopied));
                    }

                    Offset += (LocalBypassParams->FrameSize - LocalBypassParams->NoBytesCopied);

                }
                else
                {

                    //copy the data for the partial frame

#ifdef PES_TO_ES_BY_FDMA


                    PESESControl_p->NodePESToES_p->Node.NumberBytes = (Size - Offset);
                    PESESControl_p->NodePESToES_p->Node.SourceAddress_p = (void *)((U32) ((U8 *)MemoryStart+Offset) & 0x1FFFFFFF);
                    PESESControl_p->NodePESToES_p->Node.DestinationAddress_p = (void *)((U32) LocalBypassParams->Current_Write_Ptr1 & 0x1FFFFFFF);
                    PESESControl_p->NodePESToES_p->Node.Length = PESESControl_p->NodePESToES_p->Node.NumberBytes;
                    PESESControl_p->PESTOESTransferParams.NodeAddress_p = (STFDMA_GenericNode_t *)((U32)(PESESControl_p->PESTOESTransferParams.NodeAddress_p) & 0x1FFFFFFF);

                    Error=STFDMA_StartGenericTransfer(&PESESControl_p->PESTOESTransferParams,&PESESControl_p->PESTOESTransferID);
                    if(Error!=ST_NO_ERROR)
                    {
                        return Error;
                    }

#else
                    memcpy(LocalBypassParams->Current_Write_Ptr1,((char *)MemoryStart+Offset),(Size - Offset));
#endif

                    LocalBypassParams->Current_Write_Ptr1 += (Size - Offset);
                    LocalBypassParams->NoBytesCopied += (Size - Offset);
                    (PESES_BytesConsumed_t)Control_p->PESESBytesConsumedcallback(Control_p->PESESHandle,(U32)(Size - Offset));

                    Offset =  Size;

                }

                *No_Bytes_Used=Offset;
            }


            Control_p->ParseInState = MME_PARSER_INPUT_STATE_BYPASS_DATA;
            break;
        }


        default:
                break;
        }
    }

    return Error;
}


/******************************************************************************
 *  Function name   : STAud_SetAndInitParser
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : MME_ERROR
 *  Description     :
 ***************************************************************************** */
MME_ERROR   STAud_SetAndInitParser(MsppControlBlock_t *Control_p)
{
    MME_ERROR   Error = MME_SUCCESS;
    enum eAccDecoderId DecID = ACC_AC3_ID;
    enum eAccPacketCode packcode = ACC_ES;

    /* Init callback data as Handle. It will help us find the decoder from list of decoders in multi instance mode */
    Control_p->MMEinitParams.CallbackUserData = (void *)Control_p->Handle;

    switch(Control_p->StreamContent)
    {
        case STAUD_STREAM_CONTENT_MPEG1:
        case STAUD_STREAM_CONTENT_MPEG2:
        case STAUD_STREAM_CONTENT_MP3   :
             DecID = ACC_MP2A_ID;
             break;

        case STAUD_STREAM_CONTENT_MPEG_AAC:
             DecID = ACC_MP4A_AAC_ID;
             Control_p->BypassParams->FrameSize = 2* BUFFER_THRESHOLD;
             Control_p->ByPass = TRUE;
             break;

        case STAUD_STREAM_CONTENT_HE_AAC:

        case STAUD_STREAM_CONTENT_ADIF:
        case STAUD_STREAM_CONTENT_MP4_FILE:
        case STAUD_STREAM_CONTENT_RAW_AAC:
             DecID = ACC_MP4A_AAC_ID;
             Control_p->BypassParams->FrameSize = 2*BUFFER_THRESHOLD;
             Control_p->ByPass = TRUE;
             break;

        case STAUD_STREAM_CONTENT_AC3:
             DecID = ACC_AC3_ID;
             break;

        case STAUD_STREAM_CONTENT_WMA:
             DecID = ACC_WMA9_ID;
             Control_p->BypassParams->FrameSize = 6*BUFFER_THRESHOLD;
             Control_p->ByPass = TRUE;
             break;

        case STAUD_STREAM_CONTENT_WMAPROLSL:
             DecID = ACC_WMAPROLSL_ID;
             Control_p->BypassParams->FrameSize = 12*BUFFER_THRESHOLD;
             Control_p->ByPass = TRUE;
             break;

        case STAUD_STREAM_CONTENT_BEEP_TONE:
        case STAUD_STREAM_CONTENT_PINK_NOISE:
             DecID = ACC_GENERATOR_ID;
             break;
        case STAUD_STREAM_CONTENT_DDPLUS:
        case STAUD_STREAM_CONTENT_CDDA:
        case STAUD_STREAM_CONTENT_PCM:
        case STAUD_STREAM_CONTENT_LPCM:
        case STAUD_STREAM_CONTENT_LPCM_DVDA:
        case STAUD_STREAM_CONTENT_MLP:
        case STAUD_STREAM_CONTENT_DTS:
        case STAUD_STREAM_CONTENT_DV:
        case STAUD_STREAM_CONTENT_CDDA_DTS:
        case STAUD_STREAM_CONTENT_NULL:
        default:
            STTBX_Print(("STAud_SetAndInitParser: Unsupported parser stream Type !!!\n "));
            break;
    }

    switch(Control_p->StreamType)
    {
         case STAUD_STREAM_TYPE_ES:
                packcode = ACC_ES;
                break;
         case STAUD_STREAM_TYPE_PES:
                 packcode = ACC_PES_MP2;
                 break;
        case STAUD_STREAM_TYPE_PES_AD:
                 break;
        case STAUD_STREAM_TYPE_MPEG1_PACKET:
                packcode = ACC_PES_MP1;
                break;
        case STAUD_STREAM_TYPE_PES_DVD:
                packcode = ACC_PES_DVD_VIDEO;
                break;
        case STAUD_STREAM_TYPE_PES_DVD_AUDIO:
                packcode = ACC_PES_DVD_AUDIO;
                break;
        case STAUD_STREAM_TYPE_SPDIF:
                packcode = ACC_IEC_61937;
                break;
        default:
                packcode =ACC_UNKNOWN_PACKET;
                STTBX_Print(("STAud_SetAndInitParser: Unsupported packet Type !!!\n "));
                break;
    }
    if(Control_p->ByPass == FALSE)
    {

        Control_p->NumInputScatterPagesPerTransform = NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM;
        Control_p->NumOutputScatterPagesPerTransform = NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM;

        /* init decoder params */
        Error |= acc_setAudioParserInitParams(Control_p->MMEinitParams.TransformerInitParams_p,
                                                sizeof (TARGET_AUDIOPARSER_INITPARAMS_STRUCT),
                                                DecID, packcode,Control_p->Handle);

        //Checking the capability of the LX firmware for the stream

        if ((Control_p->ParseCapability.SupportedStreamContents & Control_p->StreamContent))
        {
            if(Error == MME_SUCCESS)
            {
                Error = MME_InitTransformer("AUDIO_PARSER", &Control_p->MMEinitParams, &Control_p->TranformHandleParser);
                if(Error == MME_SUCCESS)
                {
                    /* transformer initialized */
                    Control_p->Transformer_Init = TRUE;
                    STTBX_Print(("AUDIO_PARSER initialized with valid stream content\n"));
                }
                else
                {
                    Control_p->Transformer_Init = FALSE;
                }
            }
            else
            {
                Error = MME_SUCCESS;
            }
        }
    }

    return Error;

}


/******************************************************************************
 *  Function name   : Mspp_Parser_Stop
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will stop the MSPP parser, will send AbortCommand for
                   the blocks held with Lx and thereafter will terminate the transformer
                   reset other parameters.
 ***************************************************************************** */

ST_ErrorCode_t Mspp_Parser_Stop(PESES_ParserControl_t *PESESControl_p)
{
    MME_ERROR   Error = MME_SUCCESS;
    U32 Queuedifference = 0;
    U32 i, count=0;
    MspParser_Handle_t  Handle = (MspParser_Handle_t)PESESControl_p->Mspp_Cntrl_p;
    MsppControlBlock_t *Control_p;

    ParserInputCmdParam_t   *CurrentInputBufferToAbort;
    ParserOutputCmdParam_t  *CurrentOutputBufferToAbort;

    /*Retrieve control block from the MSPP handle stored in PESESControlBlock*/

    Control_p =  (MsppControlBlock_t *)Handle;

    STTBX_Print(("Mspp_Parser_Stop: Called \n"));
    /*Send abort for last output buffer first */
    if(!Control_p->ByPass)
    {
#if 0
    /* Send abort for older to newer buffer */

    for (i = Control_p->ParserOutputQueueFront; i < Control_p->ParserOutputQueueTail; i++)
    {
        CurrentOutputBufferToAbort = &(Control_p->ParserOutputQueue[i %STREAMING_PARSER_OUTPUT_QUEUE_SIZE]);
        Error = MME_AbortCommand(Control_p->TranformHandleParser,  CurrentOutputBufferToAbort->Cmd.CmdStatus.CmdId);
        STTBX_Print(("\nOutput Queue Abort sent for %dth index: Abort Return error %d\n",i%STREAMING_PARSER_OUTPUT_QUEUE_SIZE,Error));
    }

    for (i = Control_p->ParserInputQueueFront; Control_p->EOFBlockUsed ? (i < Control_p->ParserInputQueueTail-1): (i < Control_p->ParserInputQueueTail); i++)
    {
        CurrentInputBufferToAbort = &(Control_p->ParserInputQueue[i %STREAMING_PARSER_INPUT_QUEUE_SIZE]);
        Error = MME_AbortCommand(Control_p->TranformHandleParser,  CurrentInputBufferToAbort->Cmd.CmdStatus.CmdId);
        STTBX_Print(("\nInput Queue Abort sent for %dth index: Abort Return error %d\n",i%STREAMING_PARSER_INPUT_QUEUE_SIZE,Error));

    }
    if(Control_p->EOFBlockUsed)
    {
        CurrentInputBufferToAbort = &(Control_p->ParserInputQueue[STREAMING_PARSER_INPUT_QUEUE_SIZE]);
        Error = MME_AbortCommand(Control_p->TranformHandleParser,  CurrentInputBufferToAbort->Cmd.CmdStatus.CmdId);
        STTBX_Print(("\nInput Queue Abort sent for %dth index (end of file): Abort Return error %d\n",STREAMING_PARSER_INPUT_QUEUE_SIZE,Error));
        Control_p->EOFBlockUsed = FALSE;
    }

#else
    /*Send abort for last buffer to older buffer */
    Queuedifference=(Control_p->ParserOutputQueueTail - Control_p->ParserOutputQueueFront);
    for (i = Control_p->ParserOutputQueueTail; i > Control_p->ParserOutputQueueTail - Queuedifference; i--)
    {
        CurrentOutputBufferToAbort = &(Control_p->ParserOutputQueue[(i-1)%STREAMING_PARSER_OUTPUT_QUEUE_SIZE]);
        Error = MME_AbortCommand(Control_p->TranformHandleParser,  CurrentOutputBufferToAbort->Cmd.CmdStatus.CmdId);
        //STTBX_Print(("\nOutput Queue Abort sent for %dth index: Abort Return error %d\n",(i-1)%STREAMING_PARSER_OUTPUT_QUEUE_SIZE,Error));
        //AUD_TaskDelayMs(10);
    }

    i=0;
/*
    while ((Control_p->ParserOutputQueueTail != Control_p->ParserOutputQueueFront))
    {

        AUD_TaskDelayMs(100);
        outputQueueAbortWait++;
        i++;
        if(i>10)
        {
            //STTBX_PRINT(("Error in transform abort\n"));
            break;
        }

    }
*/
    Queuedifference=(Control_p->ParserInputQueueTail-Control_p->ParserInputQueueFront);
    if(Control_p->EOFBlockUsed)
    {
        CurrentInputBufferToAbort = &(Control_p->ParserInputQueue[STREAMING_PARSER_INPUT_QUEUE_SIZE]);
        Error = MME_AbortCommand(Control_p->TranformHandleParser,  CurrentInputBufferToAbort->Cmd.CmdStatus.CmdId);
        //STTBX_Print(("\nInput Queue Abort sent for %dth index (end of file): Abort Return error %d\n",STREAMING_PARSER_INPUT_QUEUE_SIZE,Error));
        Control_p->ParserInputQueueTail--;
        Control_p->EOFBlockUsed = FALSE;
        Queuedifference--;
        //AUD_TaskDelayMs(10);
    }

    for (i = (Control_p->ParserInputQueueTail);i>(Control_p->ParserInputQueueTail-Queuedifference); i--)
    {
        CurrentInputBufferToAbort = &(Control_p->ParserInputQueue[(i-1)%STREAMING_PARSER_INPUT_QUEUE_SIZE]);
        Error = MME_AbortCommand(Control_p->TranformHandleParser,  CurrentInputBufferToAbort->Cmd.CmdStatus.CmdId);
        //STTBX_Print(("\nInput Queue Abort sent for %dth index: Abort Return error %d\n",(i-1)%STREAMING_PARSER_INPUT_QUEUE_SIZE,Error));
        AUD_TaskDelayMs(10);
    }

#endif

    /* We should wait for both input queue and output queue to be freed before terminating the transformer*/

    while(Control_p->ParserOutputQueueFront != Control_p->ParserOutputQueueTail ||
            Control_p->ParserInputQueueFront != Control_p->ParserInputQueueTail)
    {

        AUD_TaskDelayMs(3);

        /* Make a bounded wait. */
        if(count>10)
        {
            /* We will come here only if LX is crashed so queueTails anf Fronts will never be updated */
            STTBX_Print((" MSPP STAUD :: Error in LX \n"));
            break;
        }
        count++;
    }

    /*STTBX_Print(("\nafter delay final count %d output tail %u output front %u input tail %u input front %u\n",count,
        Control_p->ParserOutputQueueTail,Control_p->ParserOutputQueueFront,
        Control_p->ParserInputQueueTail,Control_p->ParserInputQueueFront));*/
    /* Terminate the Transformer */
    if(Control_p->Transformer_Init)
    {
        /* Try MME_TermTransformer few time as sometime LX return BUSY */
        count=0;
        do
        {
            Error = MME_TermTransformer(Control_p->TranformHandleParser);
            count++;
            AUD_TaskDelayMs(3);
        }while((Error != MME_SUCCESS) && (count < 4));
    }

    if(Error != MME_SUCCESS)
    {
        STTBX_Print(("ERROR!!! MSPP parser MME_TermTransformer::FAILED \n"));

    }
    else
    {
        STTBX_Print(("\nLx parser MME_TermTransformer::success \n"));
    }

    /*

    Control_p->StreamContent = 0;
    Control_p->StreamType = 0;
    Control_p->StreamID= 0;

    */
    }
    else
    {
        MspBypassParams_t *LocalBypassParams = Control_p->BypassParams;

        if(LocalBypassParams->OpBlk_p)
        {
                LocalBypassParams->OpBlk_p->Flags  |= EOF_VALID;
                Error = MemPool_PutOpBlk(Control_p->OutMemoryBlockManagerHandle,(U32 *)&LocalBypassParams->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print((" MSPP Error in Memory Put_Block \n"));
                    return Error;

                }
        }
        else
        {
                Error = MemPool_GetOpBlk(Control_p->OutMemoryBlockManagerHandle,(U32 *)&LocalBypassParams->OpBlk_p);
#ifndef MSPP_PARSER
                memset((U8*)(LocalBypassParams->OpBlk_p->MemoryStart),0x00,LocalBypassParams->FrameSize);
#endif
                LocalBypassParams->OpBlk_p->Flags  |= EOF_VALID;
                Error = MemPool_PutOpBlk(Control_p->OutMemoryBlockManagerHandle,(U32 *)&LocalBypassParams->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print((" MSPP Error in Memory Put_Block \n"));
                    return Error;
                }
        }
        LocalBypassParams->OpBlk_p = NULL;
    }
    /* Reset the parser init */
    Control_p->Transformer_Init = FALSE;
    Control_p->ParseInState = MME_PARSER_INPUT_STATE_IDLE;

    /* Reset parser queues */
    Control_p->ParserOutputQueueFront = Control_p->ParserOutputQueueTail=0;
    Control_p->ParserInputQueueFront = Control_p->ParserInputQueueTail=0;

    /* Reset stream contents */
    Control_p->Mpeg1_LayerSet = FALSE;
    Control_p->MarkSentEof = FALSE;
    return Error;
}


/******************************************************************************
 *  Function name   : Mspp_Parser_Term
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will free MME parser's In/out
 *                paramaters , delete mutexes and free memory
 ***************************************************************************** */
ST_ErrorCode_t  Mspp_Parser_Term(PESES_ParserControl_t *PESESControl_p)
{
    ST_Partition_t  *CPUPartition_p;
    ST_ErrorCode_t error = ST_NO_ERROR;
    MsppControlBlock_t *Control_p;
    MspParser_Handle_t  Handle = (MspParser_Handle_t)PESESControl_p->Mspp_Cntrl_p;

    Control_p=(MsppControlBlock_t *)Handle;
    CPUPartition_p = Control_p->InitParams.Memory_Partition;

    STTBX_Print(("Mspp_Parser_Term: Entered\n "));
    if(Control_p == NULL)
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    /* Free parser queue Structures */
    error |= STAud_MsppDeInitParserQueueInParam(Control_p);
    error |= STAud_MsppDeInitParserQueueOutParam(Control_p);
    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("Mspp_Parser_Term: Failed to deallocate queue\n "));
    }

    /* Delete Mutex - GlobalCmdMutex_p cmd*/
    STOS_MutexDelete(Control_p->GlobalCmdMutex_p);

    /* Free any Update Param command list */
    while(Control_p->UpdateGlobalParamList != NULL)
    {
        STAud_ParseRemCmdFrmList(Control_p);
    }

    /* Free the MME Param Structure */
    STOS_MemoryDeallocate(CPUPartition_p, Control_p->MMEinitParams.TransformerInitParams_p);
    /*free bypass params*/
    STOS_MemoryDeallocate(CPUPartition_p, Control_p->BypassParams );
    /*Free the Control Block */
    STOS_MemoryDeallocate(CPUPartition_p, Control_p);
    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_ParserTerm: Failed to Terminate Successfully\n "));
    }
    else
    {
        STTBX_Print(("STAud_ParserTerm: Successfully\n "));
    }
    return error;
}



/************************************************************************************
Name         : Mspp_Parser_HandleEOF()

Description  : This function sends EOF to Lx

Parameters   : MspParser_Handle_t  Handle

Return Value :
    ST_NO_ERROR
************************************************************************************/
ST_ErrorCode_t Mspp_Parser_HandleEOF(MspParser_Handle_t  Handle)
{
    U32                 QueueTailIndex;
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    U8                  Num_IPScatterPages = 0,Num_OPScatterPages = 0;
    MsppControlBlock_t  *Control_p = (MsppControlBlock_t *)Handle;
    MME_ERROR           status;
    MME_DataBuffer_t    * IPDataBuffer_p,* OPDataBuffer_p;
    //STTBX_Print(("Mspp_Parser_HandleEOF\n"));
    switch(Control_p->ParseInState)
    {
        case MME_PARSER_INPUT_STATE_IDLE:

          /*The intended fall though to MME_PARSER_INPUT_STATE_GET_INTPUT_DATA state*/
            Control_p->ParseInState = MME_PARSER_INPUT_STATE_GET_INTPUT_DATA;

        case MME_PARSER_INPUT_STATE_GET_INTPUT_DATA:

        if(Control_p->MarkSentEof)
        {
            Control_p->ParseInState = MME_PARSER_INPUT_STATE_GET_OUTPUT_BUFFER;
        }
        else
        {
            if (Control_p->ParserInputQueueTail - Control_p->ParserInputQueueFront < STREAMING_PARSER_INPUT_QUEUE_SIZE)
            {
                // We have space to send another command
                QueueTailIndex = Control_p->ParserInputQueueTail%STREAMING_PARSER_INPUT_QUEUE_SIZE;
            }
            else
            {
                /*The last input queue structure is kept for scenario when input queue is full and still there is no output generated due to underflow*/
                /*so in this case last input structure is used,Its index is equal to STREAMING_PARSER_INPUT_QUEUE_SIZE */
                QueueTailIndex = (STREAMING_PARSER_INPUT_QUEUE_SIZE);
                Control_p->EOFBlockUsed = TRUE;
            }
            Control_p->CurrentInputBufferToSend = &Control_p->ParserInputQueue[QueueTailIndex];
            IPDataBuffer_p = Control_p->CurrentInputBufferToSend->ParserBufIn.BufferIn_p;
            Num_IPScatterPages = IPDataBuffer_p->NumberOfScatterPages;
            while ((Num_IPScatterPages != Control_p->NumInputScatterPagesPerTransform))
            {
                IPDataBuffer_p->ScatterPages_p[Num_IPScatterPages].Page_p = 0x00;
                IPDataBuffer_p->ScatterPages_p[Num_IPScatterPages].Size = 0x00;
                IPDataBuffer_p->TotalSize = 0x00;
                Num_IPScatterPages++;
            }
            if ((Num_IPScatterPages == Control_p->NumInputScatterPagesPerTransform))
            {
                IPDataBuffer_p->NumberOfScatterPages = Num_IPScatterPages;
                status = acc_setAudioParserBufferCmd(&Control_p->CurrentInputBufferToSend->Cmd,
                                                    &Control_p->CurrentInputBufferToSend->BufferParams,
                                                    sizeof (MME_AudioParserBufferParams_t),
                                                    &Control_p->CurrentInputBufferToSend->FrameStatus,
                                                    sizeof (MME_AudioParserFrameStatus_t),
                                                    &(Control_p->CurrentInputBufferToSend->ParserBufIn.BufferIn_p),
                                                    ACC_CMD_PLAY, FALSE,FALSE,Control_p->StreamContent,ACC_MME_TRUE);

                status =MME_SendCommand(Control_p->TranformHandleParser, &Control_p->CurrentInputBufferToSend->Cmd);
                Control_p->CurrentInputBufferToSend = NULL;
                Control_p->ParserInputQueueTail++;/*no more increment after it*/
                Control_p->MarkSentEof = TRUE;
            }
            /* EOF block sentintended fall though to MME_PARSER_INPUT_STATE_GET_OUTPUT_BUFFER state*/
            Control_p->ParseInState = MME_PARSER_INPUT_STATE_GET_OUTPUT_BUFFER;
        }

            case MME_PARSER_INPUT_STATE_GET_OUTPUT_BUFFER:
            {
                BOOL SendMMECommand = TRUE;
                while (SendMMECommand)
                {
                    SendMMECommand = FALSE;
                    if (Control_p->ParserOutputQueueTail - Control_p->ParserOutputQueueFront < STREAMING_PARSER_OUTPUT_QUEUE_SIZE
                        && (Control_p->ParserInputQueueTail != Control_p->ParserInputQueueFront))
                    {
                        // We have space to send another command
                        QueueTailIndex = Control_p->ParserOutputQueueTail%STREAMING_PARSER_OUTPUT_QUEUE_SIZE;
                        Control_p->CurrentOutputBufferToSend = &Control_p->ParserOutputQueue[QueueTailIndex];
                        OPDataBuffer_p = Control_p->CurrentOutputBufferToSend->ParserBufOut.BufferOut_p;
                        Num_OPScatterPages = OPDataBuffer_p->NumberOfScatterPages;

                        while ((Num_OPScatterPages != Control_p->NumOutputScatterPagesPerTransform) &&
                                (MemPool_GetOpBlk(Control_p->OutMemoryBlockManagerHandle, (U32 *)&Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Num_OPScatterPages]) == ST_NO_ERROR))
                        {
                            OPDataBuffer_p->ScatterPages_p[Num_OPScatterPages].Page_p = (void*)Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Num_OPScatterPages]->MemoryStart;
                            OPDataBuffer_p->ScatterPages_p[Num_OPScatterPages].Size = Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Num_OPScatterPages]->MaxSize;//Size;
                            OPDataBuffer_p->ScatterPages_p[Num_OPScatterPages].BytesUsed = 0;
                            OPDataBuffer_p->TotalSize += Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Num_OPScatterPages]->MaxSize;//Size;
                            Num_OPScatterPages++;
                        }
                        if ((Num_OPScatterPages == Control_p->NumOutputScatterPagesPerTransform))
                        {
                            OPDataBuffer_p->NumberOfScatterPages = Num_OPScatterPages;
                            /* update command params */
                            status = acc_setAudioParserTransformCmd(&Control_p->CurrentOutputBufferToSend->Cmd,
                                                                        &Control_p->CurrentOutputBufferToSend->FrameParams,
                                                                        sizeof (MME_AudioParserFrameParams_t),
                                                                        &Control_p->CurrentOutputBufferToSend->FrameStatus,
                                                                        sizeof (MME_AudioParserFrameStatus_t),
                                                                        &(Control_p->CurrentOutputBufferToSend->ParserBufOut.BufferOut_p),
                                                                        PARSE_CMD_PLAY, 0, FALSE,
                                                                        0, 1);
                            status =MME_SendCommand(Control_p->TranformHandleParser, &Control_p->CurrentOutputBufferToSend->Cmd);
                            Control_p->CurrentOutputBufferToSend = NULL;
                            /* Update decoder tail */
                            Control_p->ParserOutputQueueTail++;
                            SendMMECommand = TRUE;
                        }
                        else
                        {
                            /*STTBX_Print(("No output block available\n"));*/
                            Control_p->ParseInState = MME_PARSER_INPUT_STATE_GET_OUTPUT_BUFFER;
                            return ST_ERROR_NO_MEMORY;
                        }
                    }
                }
            }
            case MME_PARSER_INPUT_STATE_BYPASS_DATA:
            {
                MspBypassParams_t * LocalBypassParams = Control_p->BypassParams;

                if (LocalBypassParams->OpBlk_p == NULL)
                {
                    Error = MemPool_GetOpBlk(Control_p->OutMemoryBlockManagerHandle, (U32 *)&LocalBypassParams->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        return STAUD_MEMORY_GET_ERROR;
                    }
                    else
                    {
                        LocalBypassParams->Current_Write_Ptr1 = (U8 *)LocalBypassParams->OpBlk_p->MemoryStart;
                        LocalBypassParams->OpBlk_p->Flags = 0;
                        LocalBypassParams->NoBytesCopied = 0;
                    }
                }
#ifndef MSPP_PARSER
                memset(LocalBypassParams->Current_Write_Ptr1,0x00,(LocalBypassParams->FrameSize - LocalBypassParams->NoBytesCopied));
#endif

                LocalBypassParams->OpBlk_p->Size = LocalBypassParams->FrameSize;
                LocalBypassParams->OpBlk_p->Flags  |= EOF_VALID;

                Error = MemPool_PutOpBlk(Control_p->OutMemoryBlockManagerHandle,(U32 *)&LocalBypassParams->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print(("Error in Memory Put_Block \n"));
                }
                else
                {
                            LocalBypassParams->OpBlk_p = NULL;
                }
                break;
            }
            default:
                break;

        }
    return Error;
}

/************************************************************************************
Name            : Mspp_Parser_SetStreamType()

Description : This function sets the StreamType and StreamContent
               and initialize the Audio Parser transformer.

Parameters  : MspParser_Handle_t  Handle,STAUD_StreamType_t StreamType
              STAUD_StreamContent_t StreamContents

Return Value    : ST_NO_ERROR
************************************************************************************/

ST_ErrorCode_t Mspp_Parser_SetStreamType(MspParser_Handle_t  Handle,STAUD_StreamType_t StreamType,
    STAUD_StreamContent_t StreamContents)
{
    MsppControlBlock_t *Control_p;
    MME_ERROR   Status;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    Control_p=(MsppControlBlock_t *)Handle;

    Control_p->StreamType=StreamType;
    Control_p->StreamContent=StreamContents;

    /*MME_InitTransformer should be moved here*/
    Status = STAud_SetAndInitParser(Control_p);
    if(Status != MME_SUCCESS)
    {
        STTBX_Print(("Audio Parser:Init failure\n"));
        Error = ST_ERROR_NO_MEMORY;
    }
    else
    {
        STTBX_Print(("Audio Parser:Init Success\n"));
    }
    Status |= ResetMsppQueueParams(Control_p);
    return ST_NO_ERROR;

}

/************************************************************************************
Name            : STAud_ParseMMETransformCompleted()

Description : This function is called when MME_TRANSFORM callback occurs.

Parameters  : MME_Command_t * CallbackData , void *UserData

Return Value    : ST_NO_ERROR
************************************************************************************/
ST_ErrorCode_t STAud_ParseMMETransformCompleted(MME_Command_t *CallbackData_p , void *UserData_p)
{
    /* Local variables */
    MME_Command_t       *command_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Get the decoder refereance fro which this call back has come */
    MspParser_Handle_t  Handle = (MspParser_Handle_t)UserData_p;

    MsppControlBlock_t *Msp_Parse_p;
    Msp_Parse_p=(MsppControlBlock_t *)Handle;

    if(Msp_Parse_p == NULL)
    {
        STTBX_Print(("MSPP-->ERROR!!!MMETransformCompleted::No parser Handle  \n"));
    }

    command_p       = (MME_Command_t *) CallbackData_p;
    if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
    {
        /* Command failed */
        STTBX_Print(("MSPP-->ERROR!!!MMETransformCompleted::MME_COMMAND_FAILED parser \n"));;
    }
    else
    {
        //STTBX_Print(("Lx parser transform callback MME_COMMAND_SUCCESS for output queue index %d \n",(Msp_Parse_p->ParserOutputQueueFront % STREAMING_PARSER_OUTPUT_QUEUE_SIZE)));
    }


    /*Get the Completed command and send decoded data to next unit */
    Error = STAud_ParseProcessOutput(Msp_Parse_p,command_p);

    return Error;

}

/******************************************************************************
 *  Function name   : STAud_ParseMMESetGlobalTransformCompleted
 *  Arguments       :
 *       IN         :
 *       OUT            :
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : One MME_SET_GLOBAL_TRANSFORM_PARAMS Completed. Remove the completed command from
 *                List of update commands we sent. We will remove the first node from the list. This is based on the
 * assumption that decoder will process the cmd sequentially i.e. it will return back the commands in the order we sent them.
 * Hence we remove the first command from list and free it
 ***************************************************************************** */
ST_ErrorCode_t STAud_ParseMMESetGlobalTransformCompleted(MME_Command_t *CallbackData_p , void *UserData_p)
{
    /* Local variables */
    MME_Command_t       *command_p;
    MsppControlBlock_t  *Msp_Parse_p;
    partition_t             *DriverPartition = NULL;
    TARGET_AUDIOPARSER_GLOBALPARAMS_STRUCT *gp_params;

    /* Get the decoder refereance fro which this call back has come */
    MspParser_Handle_t  Handle = (MspParser_Handle_t)UserData_p;
    Msp_Parse_p=(MsppControlBlock_t *)Handle;


    /*STTBX_Print(("MMESetGlobalTransformCompleted::Callback Received \n"));*/
    STTBX_Print(("\n MMEGblTraCom:CB\n"));
    /* Get the decoder refereance fro which this call back has come */

    if(Msp_Parse_p == NULL)
    {
        STTBX_Print(("MMESetGlobalTransformCompleted::Msp_Parse_p null \n"));
    }
    command_p       = (MME_Command_t *) CallbackData_p;
    if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
    {
        /* Command failed */
        STTBX_Print(("MMESetGlobalTransformCompleted::MME_COMMAND_FAILED \n"));
    }
    /* Free the structures passed with command */
    gp_params = (TARGET_AUDIOPARSER_GLOBALPARAMS_STRUCT *) command_p->Param_p;
    if (gp_params->StructSize < sizeof(TARGET_AUDIOPARSER_GLOBALPARAMS_STRUCT))
    {
        /* This was allocated while we were building the PP command for sending to Decoder */
        DriverPartition = Msp_Parse_p->InitParams.Memory_Partition;
        STOS_MemoryDeallocate(DriverPartition,gp_params);
        //free(gp_params);
    }

    /* Free the completed command */
    STAud_ParseRemCmdFrmList(Msp_Parse_p);

    return ST_NO_ERROR;
}

/******************************************************************************
 *  Function name   : STAud_ParseMMESendBufferCompleted
 *  Arguments       :
 *       IN         :
 *       OUT            :
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     :  This function is called in context of every SendBuffer callback
 *
 ***************************************************************************** */
ST_ErrorCode_t STAud_ParseMMESendBufferCompleted(MME_Command_t *CallbackData_p, void *UserData_p)
{
    /* Local variables */
    MME_Command_t       *command_p;
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    MsppControlBlock_t *Msp_Parse_p;

    /* Get the parser refereance fro which this call back has come */
    MspParser_Handle_t  Handle = (MspParser_Handle_t)UserData_p;
    Msp_Parse_p=(MsppControlBlock_t *)Handle;

    if(Msp_Parse_p == NULL)
    {
        /* handle not found - We shall not come here - ERROR*/
        STTBX_Print(("MSPP-->ERROR!!!MMETransformCompleted::NO parser Handle Found \n"));
    }

    command_p       = (MME_Command_t *) CallbackData_p;
    if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
    {
        /* Command failed */
        STTBX_Print(("MSPP-->Lx parser send buffer callback:MME_COMMAND_FAILED for input queue index %d\n",(Msp_Parse_p->ParserInputQueueFront % STREAMING_PARSER_INPUT_QUEUE_SIZE)));
    }

    /*Get the Completed command and release the input buffer*/
    Error = STAud_ParseReleaseInput(Msp_Parse_p);
    return Error;
}

/******************************************************************************
 *  Function name   : STAud_ParseReleaseInput
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Inform aud_pes_es_parser with the compressed data consumed by Lx
 ***************************************************************************** */
ST_ErrorCode_t  STAud_ParseReleaseInput(MsppControlBlock_t  *Control_p)
{

    ST_ErrorCode_t          status = ST_NO_ERROR;
    ParserInputCmdParam_t   *CurrentInputBufferToRelease;
    U16                         BufferSize;
    //U16                   i; /Used for dumping PES output to file*/

    /* Check if there is data to be sent to output */
    if(Control_p->ParserInputQueueFront != Control_p->ParserInputQueueTail)
    {
        CurrentInputBufferToRelease = &Control_p->ParserInputQueue[Control_p->ParserInputQueueFront % STREAMING_PARSER_INPUT_QUEUE_SIZE];

#if DUMP_PES_OUTPUT_TO_FILE // Accumlate data before dumping

        /* Set the parsed buffer size returned from parser */
        for (i=0; i < Control_p->NumInputScatterPagesPerTransform; i++)
        {
            {
                static U32 index;
                char *temp;
                temp = (OutBuf+index);
                memcpy((void*)temp,(void*)CurrentInputBufferToRelease->ParserBufIn.BufferIn_p->ScatterPages_p[i].Page_p,CurrentInputBufferToRelease->ParserBufIn.BufferIn_p->ScatterPages_p[i].Size);
                index+=CurrentInputBufferToRelease->ParserBufIn.BufferIn_p->ScatterPages_p[i].Size;
                if(index == 15552))
                {
                    static FILE *f1 = NULL;
                    if(f1==NULL)
                        f1 = fopen("C:\dump_buffercallback.dat","wb");
                    fwrite(OutBuf, 15552,1,f1);
                    index = 0;
                    fclose(f1);
                }
            }
        }
#endif
        BufferSize = (U16)CurrentInputBufferToRelease->ParserBufIn.BufferIn_p->TotalSize;
        if(BufferSize != BUFFER_THRESHOLD)
            //STTBX_Print(("\nSend Buffer Callback: The block is not equal to BUFFER_THRESHOLD size %u\n",BufferSize));

        if(!BufferSize)
        {
            /*If send buffer callback total size is zero it mean it is the last EOF block callback, we dont send any inputblock
                with zero byte other than end of file case*/

            Control_p->EOFBlockUsed = FALSE;
            STTBX_Print(("\nSend Buffer Callback with zero size should arrive only: end of file case callback"));
        }

        /* Update Read Pointer (BufferSize)*/
        (PESES_BytesConsumed_t)Control_p->PESESBytesConsumedcallback(Control_p->PESESHandle,(U32)BufferSize);

        /*Reset required fields as this input block has been consumed by parser*/
        CurrentInputBufferToRelease->ParserBufIn.BufferIn_p->NumberOfScatterPages = 0;
        CurrentInputBufferToRelease->ParserBufIn.BufferIn_p->TotalSize = 0;

        /* update queue front */
        Control_p->ParserInputQueueFront++;
    }

    return status;
}


/******************************************************************************
 *  Function name   : TransformerCallback_AudioParser
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : MSPP Parser Callback
 ***************************************************************************** */
void TransformerCallback_AudioParser(MME_Event_t Event, MME_Command_t * CallbackData, void *UserData)
{
    /* Local variables */
    MME_Command_t   *command_p;
    static U32  ReceivedCount;
    command_p       = (MME_Command_t *) CallbackData;

    if (Event != MME_COMMAND_COMPLETED_EVT)
    {
        if (Event == MME_NOT_ENOUGH_MEMORY_EVT)
        {
            //STTBX_Print(("\nReceiving a underflow after last buffer\n"));
        }
        // For debugging purpose we can add a few prints here
        return;
    }
    if(ReceivedCount%100==99)
        STTBX_Print((" CB %d \n",ReceivedCount++));
    switch(command_p->CmdCode)
    {
        case MME_TRANSFORM:
            STAud_ParseMMETransformCompleted(CallbackData,UserData);
            break;

        case MME_SET_GLOBAL_TRANSFORM_PARAMS:
            /*For Lx parser nothing will change on the fly before termination and starting the transformer again, so this callback will never occur
               Still to confirm thats why code exists to handle it*/
            STAud_ParseMMESetGlobalTransformCompleted(CallbackData,UserData);
            break;

        case MME_SEND_BUFFERS:
            STAud_ParseMMESendBufferCompleted(CallbackData,UserData);
            break;

        default:
            break;
    }

}

/******************************************************************************
 *  Function name   : STAud_InitParserCapability
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Queries the Parser capability from Lx
 ***************************************************************************** */

ST_ErrorCode_t  STAud_InitParserCapability(MsppControlBlock_t *parserBlock)
{
    MME_TransformerCapability_t     TransformerCapability;
    MME_AudioParserInfo_t           ParserInfo;
    MME_ERROR                   Error = MME_SUCCESS;
    U32                         i;

    /* Fill in the capability struture */

    parserBlock->ParseCapability.SupportedStreamTypes   = (STAUD_STREAM_TYPE_ES | STAUD_STREAM_TYPE_PES | STAUD_STREAM_TYPE_PES_DVD);
    parserBlock->ParseCapability.SupportedTrickSpeeds[0] = 0;

    TransformerCapability.StructSize            = sizeof(MME_TransformerCapability_t);
    TransformerCapability.TransformerInfoSize   = sizeof(MME_AudioParserInfo_t);
    TransformerCapability.TransformerInfo_p     = &ParserInfo;
    Error = MME_GetTransformerCapability("AUDIO_PARSER", &TransformerCapability);
    if (Error == MME_SUCCESS)
    {
        STTBX_Print(("MME capability : Success for parser\n"));
        parserBlock->ParseCapability.SupportedStreamContents = 0;
        for (i=0; i< 32; i++)
        {
            switch (ParserInfo.DecoderCapabilityFlags & (1 << i))
            {

            case (1 << ACC_AC3):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_AC3;
                break;

            case (1 << ACC_MP2a):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_MPEG1 | STAUD_STREAM_CONTENT_MPEG2;
                break;

            case (1 << ACC_MP3):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_MP3;
                break;

            case (1 << ACC_DTS):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_DTS;
                break;


            case (1 << ACC_MLP):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_MLP;
                break;

            case (1 << ACC_LPCM):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_PCM | STAUD_STREAM_CONTENT_CDDA | STAUD_STREAM_CONTENT_LPCM;
                break;

            case (1 << ACC_WMA9):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_WMA;
                break;

            case (1 << ACC_MP4a_AAC):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_MPEG_AAC | STAUD_STREAM_CONTENT_HE_AAC;
                break;

            case (1 << ACC_WMAPROLSL):
                // WMA prolsl decoder can decode WMA9 streams also
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_WMAPROLSL | STAUD_STREAM_CONTENT_WMA;
                break;

            case (1 << ACC_DDPLUS):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_DDPLUS;
                break;

            case (1 << ACC_GENERATOR):
                parserBlock->ParseCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_BEEP_TONE | STAUD_STREAM_CONTENT_PINK_NOISE;
                break;

            // MP3 is supported by MPEG decoder
            case (1 << ACC_PCM):
            case (1 << ACC_SDDS):
            case (1 << ACC_OGG):
            case (1 << ACC_REAL_AUDIO):
            case (1 << ACC_HDCD):
            case (1 << ACC_AMRWB):
            default:
                break;

            }
        }
    }
    else
    {
        STTBX_Print(("MME capability : Failture for parser\n"));
    }

    return ST_NO_ERROR;
}


/******************************************************************************
 *  Function name   : STAud_ParseProcessOutput
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Send the parsed data to next unit and copy frequency, mode and pts related
                  data if they are valid and notify event with PTS and when frquency change
 ***************************************************************************** */
ST_ErrorCode_t  STAud_ParseProcessOutput(MsppControlBlock_t     *Control_p, MME_Command_t        *command_p)
{

    ST_ErrorCode_t      status = ST_NO_ERROR;
    ParserOutputCmdParam_t  *CurrentBufferToDeliver;
    U32 i = 0;
    MME_AudioParserFrameStatus_t    *DecodedFrameStatus=NULL;
    STAUD_PTS_t     NotifyPTS;

    STEVT_EventID_t EventIDNewFrame = Control_p->EventIDNewFrame;
    STEVT_EventID_t EventIDFrequencyChange = Control_p->EventIDFrequencyChange;
    STEVT_Handle_t  EvtHandle = Control_p->EvtHandle;
    STAUD_Object_t  ObjectID = Control_p->ObjectID;

    /* Check if there is data to be sent to output */
    if(Control_p->ParserOutputQueueFront != Control_p->ParserOutputQueueTail)
    {
        CurrentBufferToDeliver = &Control_p->ParserOutputQueue[Control_p->ParserOutputQueueFront % STREAMING_PARSER_OUTPUT_QUEUE_SIZE];

        /* Set the parsed  buffer size returned from parser */
        for (i=0; i < Control_p->NumOutputScatterPagesPerTransform; i++)
        {
            DecodedFrameStatus = (MME_AudioParserFrameStatus_t*)&CurrentBufferToDeliver->FrameStatus;
            if (command_p->CmdStatus.State == MME_COMMAND_FAILED)
            {
                //STTBX_Print(("MME_COMMAND_FAILED STAud_ParseProcessOutput\n"));
            }
            else if(command_p->CmdStatus.State != MME_COMMAND_COMPLETED)
            {
                STTBX_Print(("parser state is not MME_COMMAND_FAILED,MME_COMMAND_COMPLETED \n"));
            }
            else
            {
                /*copy the bytes used field written by Lx parser to the memory block size field to be sent to decoder*/
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size = CurrentBufferToDeliver->ParserBufOut.BufferOut_p->ScatterPages_p[i].BytesUsed;
                /*STTBX_Print(("\n size recieved from Lx parser  %d byte \n",CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size));*/

                if(!CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size)
                {
                    STTBX_Print(("\nTransform Callback and byteused returned size zero byte \n"));
                }

                /*If non zero byte used is recieved and is different from last size then update the last parsed field*/
                if(Control_p->Last_Parsed_Size != CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size
                    && CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size)
                {
                    Control_p->Last_Parsed_Size = CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size;
                }

#if DUMP_OUTPUT_TO_FILE // Accumlate data before dumping
                {
                    static U32 index;
                    char *temp;
                    temp = (OutBuf+index);
                    memcpy((void*)temp,(void*)CurrentBufferToDeliver->OutputBlockFromConsumer[i]->MemoryStart,CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size);
                    index+=CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size;
                    if(index >= 426240 /*CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size*(FRAMES-1)*/)
                    {
                        static FILE *f1 = NULL;
                        if(f1==NULL)
                            f1 = fopen("C:\\parsedfromdecoder.dat","wb");
                        fwrite(OutBuf, 426240 /*CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size*(FRAMES-1)*/,1,f1);
                        fclose(f1);
                        index=0;
                    }
                }
#endif
            }

            /*Copy other recieved parsed information to parser control block*/
            Control_p->Emphasis = DecodedFrameStatus->Emphasis;
            Control_p->NbOutSamples = DecodedFrameStatus->NbOutSamples;

            /*Reset Flags and modify it according to frame status*/
            CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags = 0;

            /*Retrieve decoder id recieved from Lx parser*/
            Control_p->DecoderId = DecodedFrameStatus->FrameStatus[AP_STATUS_FLAG0] & 0xFF;

            /*Retrieve layer information */
            if(Control_p->StreamContent == STAUD_STREAM_CONTENT_MPEG1 && Control_p->Mpeg1_LayerSet)
            {
                Control_p->MpegLayer = (eMpegLayer_t) (DecodedFrameStatus->FrameStatus[AP_STATUS_FLAG0] >> 8) & MPEG_LAYER_BIT;
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags |= LAYER_VALID;
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->AppData_p = (void *)((U32)Control_p->MpegLayer);
                Control_p->Mpeg1_LayerSet = FALSE;
                STTBX_Print(("setting mpeg layer set %d\n",Control_p->MpegLayer));
            }

            /*Check if End of file is recieved*/
            if (DecodedFrameStatus->FrameStatus[AP_STATUS_EOF])
            {
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags |= EOF_VALID;
                STTBX_Print(("EOF flag set in Transform callback \n"));
            }

            if(DecodedFrameStatus->AudioMode  != Control_p->AudioCodingMode)
            {
                /* New frequency received from decoder. Update control block information */
                Control_p->AudioCodingMode = DecodedFrameStatus->AudioMode;
                if(Control_p->AudioCodingMode == 0xFF)
                {
                    /* Send the mode to Players */
                    CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[CODING_OFFSET] =
                                    DecodedFrameStatus->AudioMode;
                    STTBX_Print(("Invalid Audio Coding Mode %u\n",DecodedFrameStatus->AudioMode));
                }
                else
                {
                    /* Send the mode to Players */
                    CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[CODING_OFFSET] =
                                    DecodedFrameStatus->AudioMode;

                    /* Mark the mode valid flag */
                    CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags |= MODE_VALID;

                }
                STTBX_Print(("mspp transformation callback Audio Coding Mode %u\n",CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[CODING_OFFSET]));
            }

            /* New frequency received from parser. Update control block information  */

            if(DecodedFrameStatus->SamplingFreq  != Control_p->SamplingFrequency)
            {
                /* New frequency received from decoder. Update control block information */
                Control_p->SamplingFrequency = DecodedFrameStatus->SamplingFreq;

                /* Send the frequency to decoder*/
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[FREQ_OFFSET] = CovertFsCodeToSampleRate(Control_p->SamplingFrequency);

                /* Mark the frequency valid flag */
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags |= FREQ_VALID;
                STTBX_Print(("mspp transformation callback Sampling frequency %u\n",CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[FREQ_OFFSET]));
                /*send updated frequency to event manager*/
                status=STAudEVT_Notify(EvtHandle,EventIDFrequencyChange, &Control_p->SamplingFrequency, sizeof(Control_p->SamplingFrequency), ObjectID);
            }

            if(DecodedFrameStatus->PTSflag << 31) /*Bit 0 if true PTS flag validity*/
            {
                Control_p->PTS = DecodedFrameStatus->PTS;
                Control_p->PTSflag = (DecodedFrameStatus->PTSflag >> 16 & 0x1);/*PTS flag 16th bit is PTS 33 bit */

                /* Send the PTS to Decoder */

                NotifyPTS.Low=CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[PTS_OFFSET] = Control_p->PTS;
                NotifyPTS.High=CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[PTS33_OFFSET] = Control_p->PTSflag;

                /* Mark the PTS valid flag */
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags |= PTS_VALID;
                //STTBX_Print(("Delivered PTS  %u - PTS 33 bit %u \n",CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[PTS_OFFSET],CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[PTS33_OFFSET]));
            }

            /*Send NULL data and Last_Parsed_Size size to Lx decoder to handle cases when Lx parser return BytesUsed 0 bytes*/
            if(!CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size ||command_p->CmdStatus.State == MME_COMMAND_FAILED)
            {
                /*STTBX_Print(("Lx callback size %u\n", CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size));*/
#ifndef MSPP_PARSER
                memset((void*)CurrentBufferToDeliver->OutputBlockFromConsumer[i]->MemoryStart,0x00,CurrentBufferToDeliver->OutputBlockFromConsumer[i]->MaxSize);
#endif
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size = Control_p->Last_Parsed_Size;
            }
            /*It is required in 7109 (no byte swapper)*/
            CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags   |= DATAFORMAT_VALID;
            CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[DATAFORMAT_OFFSET] = BE;
            status = MemPool_PutOpBlk(Control_p->OutMemoryBlockManagerHandle ,
                            (U32 *)&(CurrentBufferToDeliver->OutputBlockFromConsumer[i]));
            if(status != ST_NO_ERROR)
            {
                STTBX_Print(("mspp STAud_ParseProcessOutput::MemPool_PutOpBlk Failed\n"));
            }

            /*post new frame event here*/
            status = STAudEVT_Notify(EvtHandle,EventIDNewFrame, &NotifyPTS,sizeof(NotifyPTS),ObjectID);

            /*reset the required field of this block to zero as it has been consumed by parser*/

            CurrentBufferToDeliver->ParserBufOut.BufferOut_p->NumberOfScatterPages = 0;
            CurrentBufferToDeliver->ParserBufOut.BufferOut_p->TotalSize = 0;

            /* update queue front */
            Control_p->ParserOutputQueueFront++;
        }
    }
    return status;
}


/* This function can be removed as there is no property to be changed at the run time, stream content cant be changed at runtime*/

ST_ErrorCode_t  ParseTransformUpdate(MsppControlBlock_t *Control_p, STAUD_StreamContent_t    StreamContent)
{
    ST_ErrorCode_t          error = ST_NO_ERROR;
    TARGET_AUDIOPARSER_INITPARAMS_STRUCT    *init_params;
    MME_Command_t  * cmd        =NULL;
    enum eAccDecoderId DecID    = ACC_AC3_ID;

    /* Get parser init params */
    init_params = Control_p->MMEinitParams.TransformerInitParams_p;

    switch(StreamContent)
    {
        case STAUD_STREAM_CONTENT_MPEG1:
        case STAUD_STREAM_CONTENT_MPEG2:
        case STAUD_STREAM_CONTENT_MP3   :
            DecID = ACC_MP2A_ID;
            break;

        case STAUD_STREAM_CONTENT_MPEG_AAC:
            DecID = ACC_MP4A_AAC_ID;
            break;

        case STAUD_STREAM_CONTENT_HE_AAC:
            DecID = ACC_MP4A_AAC_ID;
            break;

        case STAUD_STREAM_CONTENT_AC3:
            DecID = ACC_AC3_ID;
            break;

        case STAUD_STREAM_CONTENT_DDPLUS:
            DecID = ACC_DDPLUS_ID;
            break;

        case STAUD_STREAM_CONTENT_CDDA:
        case STAUD_STREAM_CONTENT_PCM:
            DecID = ACC_LPCM_ID;
            break;

        case STAUD_STREAM_CONTENT_LPCM:
        case STAUD_STREAM_CONTENT_LPCM_DVDA:
            DecID = ACC_LPCM_ID;
            break;

        case STAUD_STREAM_CONTENT_MLP:
            DecID = ACC_MLP_ID;
            break;

        case STAUD_STREAM_CONTENT_DTS:
            DecID = ACC_DTS_ID;
            break;

        case STAUD_STREAM_CONTENT_WMA:
            DecID = ACC_WMA9_ID;
            break;

        case STAUD_STREAM_CONTENT_WMAPROLSL:
            DecID = ACC_WMAPROLSL_ID;
            break;

        case STAUD_STREAM_CONTENT_BEEP_TONE:
        case STAUD_STREAM_CONTENT_PINK_NOISE:
            DecID = ACC_GENERATOR_ID;
            break;

        case STAUD_STREAM_CONTENT_DV:
        case STAUD_STREAM_CONTENT_CDDA_DTS:
        case STAUD_STREAM_CONTENT_NULL:
        default:
            STTBX_Print(("ParseTransformUpdate: Unsupported Decoder Type !!!\n "));
            break;
    }
    cmd = STAud_ParseGetCmd(Control_p);
    acc_setAudioParserGlobalCmd(cmd, init_params, sizeof (TARGET_AUDIOPARSER_INITPARAMS_STRUCT),Control_p->InitParams.Memory_Partition,
                                   DecID);
    error =MME_SendCommand(Control_p->TranformHandleParser, cmd);
    return error;
}


/******************************************************************************
 *  Function name   : STAud_ParseGetCmd
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will Allocate a new MME command which will be sent to decoder.
 *                 It will also add the new command to link list of update param commands
 ***************************************************************************** */
MME_Command_t*  STAud_ParseGetCmd(MsppControlBlock_t    *Control_p)
{
    UpdateGlobalCmdList_t       *CmdNode=NULL;
    partition_t         *DriverPartition = NULL;

    DriverPartition = Control_p->InitParams.Memory_Partition;

    /* Allocate memory for command */
    CmdNode = (UpdateGlobalCmdList_t*)STOS_MemoryAllocate(DriverPartition,
                        sizeof(UpdateGlobalCmdList_t));
    if(CmdNode==NULL)
    {
        /* memory allocation failed. Return NULL Cmd */
        return NULL;
    }

     memset(CmdNode,0,sizeof(UpdateGlobalCmdList_t));
    /* Add Command to update global cmd list */
    STAud_ParseAddCmdToList(CmdNode,Control_p);

    return (&CmdNode->cmd);

}

/******************************************************************************
 *  Function name   : STAud_ParseAddCmdToList
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will Allocate a new MME command which will be sent to parser.
 *                 It will also add the new command to link list of update param commands
 ***************************************************************************** */
ST_ErrorCode_t  STAud_ParseAddCmdToList(UpdateGlobalCmdList_t *Node, MsppControlBlock_t *Control_p)
{

    UpdateGlobalCmdList_t *temp;
    UpdateGlobalCmdList_t **Header;

    /* Protect the simultaneous updation of list */
    STOS_MutexLock(Control_p->GlobalCmdMutex_p);

    Node->Next_p = NULL;

    Header = &Control_p->UpdateGlobalParamList;
    /* First Node */
    if(*Header == NULL)
    {
        *Header = Node;
    }
    else
    {
        /* Find the end of List */
        temp = *Header;
        while(temp->Next_p != NULL)
            temp = temp->Next_p;

        /* Add the new node to list */
        temp->Next_p = Node;
    }

    STOS_MutexRelease(Control_p->GlobalCmdMutex_p);

    return  ST_NO_ERROR;
}

/******************************************************************************
 *  Function name   : STAud_ParseRemCmdFrmList
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will removes the first command from link list of update param
 *                 commands. It will also deallocate meory for that cmd.
 ***************************************************************************** */
ST_ErrorCode_t  STAud_ParseRemCmdFrmList(MsppControlBlock_t *Control_p)
{

    UpdateGlobalCmdList_t       *temp;
    partition_t                 *DriverPartition = NULL;

    STTBX_Print(("DecPPRemCmdFrmList: CB\n "));

    /* Protect the simultaneous updation of list */
    STOS_MutexLock(Control_p->GlobalCmdMutex_p);

    /* No Node */
    if(Control_p->UpdateGlobalParamList == NULL)
    {
        /* ERROR - We should not have came here. There is no command to remove */
    }
    else
    {
        /* Get second Node */
        temp = Control_p->UpdateGlobalParamList->Next_p;

        /* free first node */
        DriverPartition = Control_p->InitParams.Memory_Partition;
        STOS_MemoryDeallocate(DriverPartition,Control_p->UpdateGlobalParamList);

        /* reinit Header */
        Control_p->UpdateGlobalParamList = temp;
    }

    STOS_MutexRelease(Control_p->GlobalCmdMutex_p);
    return  ST_NO_ERROR;
}


/******************************************************************************
 *  Function name   : Mspp_Parser_SetSpeed
 *  Arguments       :
 *       IN         : MspParser_Handle_t Handle, S32 Speed
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will set the Speed of the control Block

 ***************************************************************************** */
ST_ErrorCode_t Mspp_Parser_SetSpeed(MspParser_Handle_t  Handle,S32 Speed)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    MsppControlBlock_t *Mspp_ParserLocalParams;
    Mspp_ParserLocalParams=(MsppControlBlock_t *)Handle;
    Mspp_ParserLocalParams->Speed = Speed;
    return Error;

}

/******************************************************************************
 *  Function name   : Mspp_Parser_SetPCMParams
 *  Arguments       :
 *       IN         : Control Block  Handle, Frequency, SampleSize, Number of channel
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     :
 *
 ***************************************************************************** */

ST_ErrorCode_t Mspp_Parser_SetPCMParams(MspParser_Handle_t  Handle,U32 Frequency, U32 SampleSize, U32 Nch)
{
    MsppControlBlock_t *Mspp_ParserLocalParams;

    ST_ErrorCode_t Error = ST_NO_ERROR;
    Mspp_ParserLocalParams=(MsppControlBlock_t *)Handle;
    return Error;
}

/******************************************************************************
 *  Function name   : Mspp_Parser_GetInfo
 *  Arguments       :
 *       IN         : Control Block  Handle, StreamInfo_p pointer
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     :

 ***************************************************************************** */

ST_ErrorCode_t Mspp_Parser_GetInfo(MspParser_Handle_t  Handle,STAUD_StreamInfo_t * StreamInfo_p)
{
    MsppControlBlock_t *Mspp_ParserLocalParams;

    ST_ErrorCode_t Error = ST_NO_ERROR;
    Mspp_ParserLocalParams=(MsppControlBlock_t *)Handle;
    return Error;
}

/******************************************************************************
 *  Function name   : Mspp_Parser_GetFreq
 *  Arguments       :
 *       IN         : Control Block handle, Sampling frequency pointer
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function updates the Sampling Frequency as exists in the control block

 ***************************************************************************** */

ST_ErrorCode_t Mspp_Parser_GetFreq(MspParser_Handle_t  Handle,U32 *SamplingFrequency_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    MsppControlBlock_t *Mspp_ParserLocalParams = (MsppControlBlock_t*)Handle;
    *SamplingFrequency_p = Mspp_ParserLocalParams->SamplingFrequency;
    return Error;

}

/************************************************************************************
Name         : Mspp_Parser_SetStreamID()

Description  : This function sets the StreamID

Parameters   : Control Block Handle, New streamID

Return Value :    ST_NO_ERROR
************************************************************************************/

ST_ErrorCode_t Mspp_Parser_SetStreamID(MspParser_Handle_t  Handle, U32 NewStreamID)
{
    MsppControlBlock_t *Mspp_ParserLocalParams;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Mspp_ParserLocalParams=(MsppControlBlock_t *)Handle;
    Mspp_ParserLocalParams->StreamID = NewStreamID;
    return Error;

}

/******************************************************************************
 *  Function name   : Mspp_Parser_SetBroadcastProfile
 *  Arguments       :
 *       IN         : Control Block Handle, Enumeration BroadcastProfile
                  (STAUD_BROADCAST_DVB, STAUD_BROADCAST_DIRECTV,
                   STAUD_BROADCAST_ATSC, STAUD_BROADCAST_DVD)
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function set the PTS_DIVISION with respect to broadcastprofile

 ***************************************************************************** */

ST_ErrorCode_t Mspp_Parser_SetBroadcastProfile(MspParser_Handle_t  Handle,STAUD_BroadcastProfile_t BroadcastProfile)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    MsppControlBlock_t *Mspp_ParserLocalParams;
    Mspp_ParserLocalParams=(MsppControlBlock_t *)Handle;

    if(BroadcastProfile==STAUD_BROADCAST_DVB)
    {
        Mspp_ParserLocalParams->PTS_DIVISION = 1;
    }
    else if(BroadcastProfile==STAUD_BROADCAST_DIRECTV)
    {
        Mspp_ParserLocalParams->PTS_DIVISION = 300;
    }
    else
    {
        Mspp_ParserLocalParams->PTS_DIVISION = 1;
        //FEATURE_NOT_SUPPORTED;
    }

    return Error;
}



/************************************************************************************
Name         : Mspp_Parser_SetClkSynchro()

Description  : This function recieves Clksource and sets it to the control block

Parameters   : Control Block handle, U32 Clksource

Return Value :   ST_NO_ERROR

 ************************************************************************************/
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t Mspp_Parser_SetClkSynchro(MspParser_Handle_t  Handle,STCLKRV_Handle_t Clksource)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    MsppControlBlock_t *Mspp_ParserLocalParams = (MsppControlBlock_t *)Handle;
    Mspp_ParserLocalParams->Clksource = Clksource;
    return Error;
}
#endif

