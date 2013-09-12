/*******************************************************************************

File name   : avsparser.c

Description : AVS Parser top level CODEC API functions.

COPYRIGHT (C) STMicroelectronics 2008

Date               Modification                                     Name
----               ------------                                     ----
29 January 2008    First STAPI version                               PLE
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

/* #include "trace.h" */

/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

/* AVS Parser specific */
#include "avsparser.h"

/* Functions ---------------------------------------------------------------- */
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t avspar_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p);
#endif
static ST_ErrorCode_t avspar_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p);
static ST_ErrorCode_t avspar_Stop(const PARSER_Handle_t ParserHandle);
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t avspar_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p);
#endif
static ST_ErrorCode_t avspar_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t avspar_AllocatePESBuffer(const PARSER_Handle_t ParserHandle, const U32 PESBufferSize, const PARSER_InitParams_t * const InitParams_p);
static ST_ErrorCode_t avspar_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t avspar_AllocateSCBuffer(const PARSER_Handle_t ParserHandle, const U32 SC_ListSize, const PARSER_InitParams_t * const InitParams_p);
static ST_ErrorCode_t avspar_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p);
static ST_ErrorCode_t avspar_GetPicture(const PARSER_Handle_t ParserHandle, const PARSER_GetPictureParams_t * const GetPictureParams_p);
static ST_ErrorCode_t avspar_Term(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t avspar_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p);
static ST_ErrorCode_t avspar_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p);
static ST_ErrorCode_t avspar_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle, void ** const BaseAddress_p, U32 * const Size_p);
static ST_ErrorCode_t avspar_SetDataInputInterface(const PARSER_Handle_t ParserHandle, ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
                                                   void (*InformReadAddress)(void * const Handle, void * const Address), void * const Handle);
void avspar_OutputDataStructure(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t avspar_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p);
#ifdef ST_speed
U32             avspar_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle);
#endif /*ST_speed */
static void avspar_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* CPUPTSEntry_p);

/* Macro ---------------------------------------------------------------- */
/* PTS List access shortcuts */
#define PTSLIST_BASE     (PARSER_Data_p->PTS_SCList.SCList)
#define PTSLIST_READ     (PARSER_Data_p->PTS_SCList.Read_p)
#define PTSLIST_WRITE    (PARSER_Data_p->PTS_SCList.Write_p)
#define PTSLIST_TOP      (&PARSER_Data_p->PTS_SCList.SCList[PTS_SCLIST_SIZE])
#ifdef STVID_DEBUG_PARSER
#define STTBX_REPORT
#endif

/* #define TRACE_AVS_PICTURE_TYPE */
#ifdef TRACE_AVS_PICTURE_TYPE
#define TracePictureType(x) TraceMessage x
#else
#define TracePictureType(x)
#endif /* TRACE_AVS_PICTURE_TYPE */

/* Constants ---------------------------------------------------------------- */
const PARSER_FunctionsTable_t PARSER_AVSFunctions =
{
#ifdef STVID_DEBUG_GET_STATISTICS
    avspar_ResetStatistics,
    avspar_GetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
    avspar_Init,
    avspar_Stop,
    avspar_Start,
    avspar_GetPicture,
    avspar_Term,
    avspar_InformReadAddress,
    avspar_GetBitBufferLevel,
    avspar_GetDataInputBufferParams,
    avspar_SetDataInputInterface,
    avspar_Setup,
    avspar_FlushInjection
#ifdef ST_speed
    ,avspar_GetBitBufferOutputCounter
#endif /*ST_speed*/
#ifdef STVID_TRICKMODE_BACKWARD
    ,avspar_SetBitBuffer,
    avspar_WriteStartCode
#endif /*STVID_TRICKMODE_BACKWARD*/
    ,avspar_BitBufferInjectionInit
};

#if defined(TRACE_UART) && defined(VALID_TOOLS)
static TRACE_Item_t avs_TraceItemList[] =
{
    {"GLOBAL",                       TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"INIT_TERM",                    TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PARSESEQ",                     TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PARSEVO",                      TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PARSEVOL",                     TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PARSEVOP",                     TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PARSEGOV",                     TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PARSESVH",                     TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PARSEVST",                     TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PARSEVPH",                     TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PICTURE_PARAMS",               TRACE_AVSPAR_COLOR, TRUE, FALSE},
    {"PICTURE_INFO_COMPILE",         TRACE_AVSPAR_COLOR, TRUE, FALSE}
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS)*/

/*******************************************************************************/
/*! \brief De-Allocate the PES Buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t avspar_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t         * PARSER_Properties_p;
    STAVMEM_FreeBlockParams_t   FreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = (AVSParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->PESBuffer.PESBufferHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The PESBuffer is not deallocated correctly !"));
    }

    return (ErrorCode);
} /* end of avspar_DeAllocatePESBuffer */

/*******************************************************************************/
/*! \brief Allocate the PES Buffer
 *
 * \param ParserHandle Parser Handle
 * \param PESBufferSize PES buffer size in bytes
 * \param InitParams_p Init parameters from CODEC API call
 * \return void
 */
/*******************************************************************************/
static ST_ErrorCode_t avspar_AllocatePESBuffer(const PARSER_Handle_t        ParserHandle,
                                       const U32                    PESBufferSize,
                                        const PARSER_InitParams_t *  const InitParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    ST_ErrorCode_t                          ErrorCode;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* TraceBuffer(("avspar_AllocatePESBuffer\n")); */

    /* Allocate an Input Buffer (PES) */
    /*--------------------------*/
    AllocBlockParams.PartitionHandle          = InitParams_p->AvmemPartitionHandle;
    AllocBlockParams.Alignment                = 128; /* limit for FDMA */
    AllocBlockParams.Size                     = PESBufferSize;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams,&PARSER_Data_p->PESBuffer.PESBufferHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }
    ErrorCode = STAVMEM_GetBlockAddress(PARSER_Data_p->PESBuffer.PESBufferHdl, (void **)&VirtualAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        avspar_DeAllocatePESBuffer(ParserHandle);
        return(ErrorCode);
    }
    PARSER_Data_p->PESBuffer.PESBufferBase_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
    PARSER_Data_p->PESBuffer.PESBufferTop_p = (void *) ((U32)PARSER_Data_p->PESBuffer.PESBufferBase_p + PESBufferSize - 1);

    /* TraceBuffer(("avspar_AllocatePESBuffer : Base =0x%x Top = 0x%x Size=%d\n", (U32)PARSER_Data_p->PESBuffer.PESBufferBase_p, (U32)PARSER_Data_p->PESBuffer.PESBufferTop_p, PESBufferSize)); */

    return (ErrorCode);
}

/*******************************************************************************/
/*! \brief De-Allocate the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t avspar_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t         * PARSER_Properties_p;
    STAVMEM_FreeBlockParams_t   FreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = (AVSParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    /* TraceBuffer(("avspar_DeAllocateSCBuffer\n")); */

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->SCBuffer.SCListHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The SCBuffer is not deallocated correctly !"));
    }

    return(ErrorCode);
} /* end of avs_DeAllocateSCBuffer() */


/*******************************************************************************/
/*! \brief Allocate the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \param SC_ListSize Start Code buffer size in bytes
 * \param InitParams_p Init parameters from CODEC API call
 * \return void
 */
/*******************************************************************************/
static ST_ErrorCode_t avspar_AllocateSCBuffer(const PARSER_Handle_t        ParserHandle,
                                       const U32                    SC_ListSize,
                                       const PARSER_InitParams_t *  const InitParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    ST_ErrorCode_t                          ErrorCode;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    AllocBlockParams.PartitionHandle = InitParams_p->AvmemPartitionHandle;
    AllocBlockParams.Alignment = 32;
    AllocBlockParams.Size = SC_ListSize * sizeof(STFDMA_SCEntry_t);
    AllocBlockParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges = 0;
    AllocBlockParams.ForbiddenRangeArray_p = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p = NULL;
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &PARSER_Data_p->SCBuffer.SCListHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }
    ErrorCode = STAVMEM_GetBlockAddress(PARSER_Data_p->SCBuffer.SCListHdl,(void **)&VirtualAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        avspar_DeAllocateSCBuffer(ParserHandle);
        return(ErrorCode);
    }
    PARSER_Data_p->SCBuffer.SCList_Start_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
    PARSER_Data_p->SCBuffer.SCList_Stop_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.SCList_Stop_p += (SC_ListSize-1);

    /* TraceBuffer(("avspar_AllocateSCBuffer : start= 0x%x  stop=0x%x size=%d\n", (U32)PARSER_Data_p->SCBuffer.SCList_Start_p, (U32)PARSER_Data_p->SCBuffer.SCList_Stop_p, SC_ListSize)); */

    return (ErrorCode);
}

/*******************************************************************************/
/*! \brief Initialize the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \return void
 */
/*******************************************************************************/
void avspar_InitSCBuffer(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Set default values for pointers in SC-List */
    PARSER_Data_p->SCBuffer.SCList_Write_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.SCList_Loop_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Start_p;

}

/*******************************************************************************/
/*! \brief Initialize the injection buffers
 *
 * Initialize the Bit Buffer pointers, allocate the PES and SC buffers.
 * Inform FDMA about these buffers allocation
 *
 * \param ParserHandle Parser Handle
 * \param SC_ListSize Start Code buffer size in bytes
 * \param BufferAddressStart_p Bit buffer start address
 * \param BufferSize Bit buffer size (in bytes)
 * \param InitParams_p Init parameters from CODEC API call
 * \return void
 */
/*******************************************************************************/
ST_ErrorCode_t avspar_InitInjectionBuffers(const PARSER_Handle_t        ParserHandle,
                                           const PARSER_InitParams_t *  const InitParams_p,
                                           const U32                    SC_ListSize,
                                           const U32                     PESBufferSize)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    VIDINJ_GetInjectParams_t              Params;
    ST_ErrorCode_t                          ErrorCode;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    ErrorCode = ST_NO_ERROR;

    /* Allocate SC Buffer */
    ErrorCode = avspar_AllocateSCBuffer(ParserHandle, SC_ListSize, InitParams_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* Allocate PES Buffer */
    ErrorCode = avspar_AllocatePESBuffer(ParserHandle, PESBufferSize, InitParams_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        avspar_DeAllocateSCBuffer(ParserHandle);
        return (ErrorCode);
    }

    /* Set default values for pointers in ES Buffer */
    PARSER_Data_p->BitBuffer.ES_Start_p = InitParams_p->BitBufferAddress_p;

    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    /* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
    PARSER_Data_p->BitBuffer.ES_Stop_p = (void *)((U32)InitParams_p->BitBufferAddress_p + InitParams_p->BitBufferSize -1);
    PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Initialize the SC buffer pointers */
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Data_p->BitBuffer.ES_StoreStart_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    PARSER_Data_p->BitBuffer.ES_StoreStop_p = PARSER_Data_p->BitBuffer.ES_Stop_p;
#endif  /* STVID_TRICKMODE_BACKWARD */
    avspar_InitSCBuffer(ParserHandle);

    /* Reset ES bit buffer write pointer */
    PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Synchronize buffer definitions with FDMA */
    /* link this init with a fdma channel */
    /*------------------------------------*/
    Params.TransferDoneFct                 = avspar_DMATransferDoneFct;
    Params.UserIdent                       = (U32)ParserHandle;
    Params.AvmemPartition                  = InitParams_p->AvmemPartitionHandle;
    Params.CPUPartition                    = InitParams_p->CPUPartition_p;
    PARSER_Data_p->Inject.InjectNum        = VIDINJ_TransferGetInject(PARSER_Data_p->Inject.InjecterHandle, &Params);
    if (PARSER_Data_p->Inject.InjectNum == BAD_INJECT_NUM)
    {
        avspar_DeAllocatePESBuffer(ParserHandle);
        avspar_DeAllocateSCBuffer(ParserHandle);
        return(ST_ERROR_NO_MEMORY);
    }

    /* TraceBuffer(("BitBuffer : \n")); */
    /* TraceBuffer(("Start = 0x%x Stop = 0x%x \n", (U32)PARSER_Data_p->BitBuffer.ES_Start_p, (U32)PARSER_Data_p->BitBuffer.ES_Stop_p)); */

    return (ErrorCode);
}


/******************************************************************************/
/*! \brief Initialize the parser module
 *
 * Allocate PES, SC+PTS buffers, Initialize semaphores, tasks,
 * Parameters: Data Partition (to allocate PES + SC/PTS buffer, as well as local buffers)
 *             ES bit buffer (start + size)
 *             Handles: FDMA, Events,
 *
 * \param ParserHandle
 * \param InitParams_p
 * \return ST_ErrorCode
 */
/******************************************************************************/
static ST_ErrorCode_t avspar_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t * PARSER_Properties_p;

    AVS_GlobalDecodingContextSpecificData_t *GDCSD;

    U32 UserDataSize;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;

    if ((ParserHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    PARSER_Properties_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;

    /* Allocate PARSER PrivateData structure */
    PARSER_Properties_p->PrivateData_p = (AVSParserPrivateData_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(AVSParserPrivateData_t));
    if (PARSER_Properties_p->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    memset (PARSER_Properties_p->PrivateData_p, 0xA5, sizeof(AVSParserPrivateData_t)); /* fill in data with 0xA5 by default */

    /* Use the short cut PARSER_Data_p for next lines of code */
    PARSER_Data_p = (AVSParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

    /* Allocate PARSER Job Results data structure */
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p =
            (VIDCOM_ParsedPictureInformation_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VIDCOM_ParsedPictureInformation_t));
    if (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p == NULL)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(ST_ERROR_NO_MEMORY);
    }
    memset (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p, 0xA5, sizeof(VIDCOM_ParsedPictureInformation_t)); /* fill in data with 0xA5 by default */

    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p =
            (VIDCOM_PictureDecodingContext_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VIDCOM_PictureDecodingContext_t));
    if (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p == NULL)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(ST_ERROR_NO_MEMORY);
    }
    memset (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p, 0xA5, sizeof(VIDCOM_PictureDecodingContext_t)); /* fill in data with 0xA5 by default */

    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p =
            (VIDCOM_GlobalDecodingContext_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VIDCOM_GlobalDecodingContext_t));
    if (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p == NULL)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(ST_ERROR_NO_MEMORY);
    }
    memset (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p, 0xA5, sizeof(VIDCOM_GlobalDecodingContext_t)); /* fill in data with 0xA5 by default */

    /* No need to allocate these buffers since they are affected afterward with already allocated pointer */
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p = NULL;
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p = NULL;
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureSpecificData_p = NULL;


    /* End of Allocate PARSER Job Results data structure */

    /* Get handle of VIDINJ injecter */
    PARSER_Data_p->Inject.InjecterHandle = InitParams_p->InjecterHandle;

    /* Allocate table of user data */
    UserDataSize = InitParams_p->UserDataSize;
    /* So that we can exploit user data for our purpose, we need a minimum amount of user data (even if requested 0 for example) */
    if (UserDataSize < MIN_USER_DATA_SIZE)
    {
        UserDataSize = MIN_USER_DATA_SIZE;
    }
    PARSER_Data_p->UserData.Buff_p = STOS_MemoryAllocate(InitParams_p->CPUPartition_p, UserDataSize);
    if (PARSER_Data_p->UserData.Buff_p == NULL)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(ST_ERROR_NO_MEMORY);
    }

    /* Initialise commands queue */
    PARSER_Data_p->ForTask.ControllerCommand = FALSE;

    /* Allocation and Initialisation of injection buffers. The FDMA is also informed about these allocations */
    ErrorCode = avspar_InitInjectionBuffers(ParserHandle, InitParams_p, SC_LIST_SIZE_IN_ENTRY, PES_BUFFER_SIZE);
    if (ErrorCode != ST_NO_ERROR)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return (ErrorCode);
    }
#ifdef ST_speed
    /*Initialise counter for bit rate*/
    PARSER_Data_p->OutputCounter = 0;
    PARSER_Data_p->SkipMode = STVID_DECODED_PICTURES_ALL;
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Data_p->Backward = FALSE;
    /* PARSER_Data_p->ParserJobResults.BackwardBufferFullyParsed = FALSE; */ /* PLE_TO_DO removed BackwardBufferFullyParsed usage because not available any more. Safe until  trickmode backward implementation. */
    PARSER_Data_p->BackwardStop = FALSE;
    PARSER_Data_p->StopParsing = FALSE;
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed*/

    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName,PARSER_JOB_COMPLETED_EVT,
                                          &(PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVS parser init: could not register events !"));
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_PICTURE_SKIPPED_EVT,
                                          &(PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID]));
    if(ErrorCode != ST_NO_ERROR)
    {
        memory_deallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        memory_deallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        memory_deallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        memory_deallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        memory_deallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVS parser init: could not register events !"));
        return(STVID_ERROR_EVENT_REGISTRATION);
    }
    /* OB_TODO : Do we need to unregister on STVID_Term */

    /* Semaphore to share variables between parser and injection tasks */
    PARSER_Data_p->InjectSharedAccess = /* semaphore_create_fifo(1); */ STOS_SemaphoreCreateFifo(InitParams_p->CPUPartition_p,1);

    /* Semaphore between parser and controller tasks */
    /* Parser main task does not run until it gets a start command from the controller */
    PARSER_Data_p->ParserOrder = /* semaphore_create_fifo(0);          */     STOS_SemaphoreCreateFifo(NULL, 0);
    PARSER_Data_p->ParserSC    = /* semaphore_create_fifo_timeout(0);  */     STOS_SemaphoreCreateFifoTimeOut(NULL, 0);


    /* Semaphore to share variables between parser and controller tasks */
    PARSER_Data_p->ControllerSharedAccess = /* semaphore_create_fifo(1); */ STOS_SemaphoreCreateFifo(InitParams_p->CPUPartition_p, 1);


    PARSER_Data_p->ParserState = PARSER_STATE_IDLE;

    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
    PARSER_Data_p->NonVOPStartCodeFound = FALSE;
    PARSER_Data_p->UserDataAllowed = FALSE;

    /* Create parser task */
    PARSER_Data_p->ParserTask.IsRunning = FALSE;
    ErrorCode = avspar_StartParserTask(ParserHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return (ST_ERROR_BAD_PARAMETER);
    }
    PARSER_Data_p->ParserTask.IsRunning = TRUE;

    GDCSD= &PARSER_Data_p->GlobalDecodingContextSpecificData;
    GDCSD->FirstPictureEver=TRUE;
    return (ErrorCode);

} /* End of PARSER_Init function */

/*******************************************************************************/
/*! \brief Set the Stream Type (PES or ES) for the PES parser
 * \param ParserHandle Parser handle,
 * \param StreamType ES or PES
 * \return void
 */
/*******************************************************************************/
void avspar_SetStreamType(const PARSER_Handle_t ParserHandle, const STVID_StreamType_t StreamType)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if ((StreamType & STVID_STREAM_TYPE_ES) == STVID_STREAM_TYPE_ES)
    {
        PARSER_Data_p->PESBuffer.PESRangeEnabled = FALSE;
    }
    else /* assumption is to be PES if not ES */
    {
        PARSER_Data_p->PESBuffer.PESRangeEnabled = TRUE;
    }
}

/*******************************************************************************/
/*! \brief Sets the PES Range parameters to program the FDMA
 *
 * \param ParserHandle Parser handle
 * \param StartParams_p
 * \return void
 */
/*******************************************************************************/
void avspar_SetPESRange(const PARSER_Handle_t ParserHandle,
                         const PARSER_StartParams_t * const StartParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* FDMA setup ----------------  */
    avspar_SetStreamType(ParserHandle, StartParams_p->StreamType);

    /* Current assumption is that the PTI is filtering the StreamID yet.
     * So, at that stage, the FDMA receives only the right StreamID, whatever that StreamID
     * Regardless of the StreamID parameter sent by the controller, the FDMA is then programmed
     * to look for the full range of StreamID
     */
    PARSER_Data_p->PESBuffer.PESSCRangeStart = SMALLEST_VIDEO_PACKET_START_CODE;
    PARSER_Data_p->PESBuffer.PESSCRangeEnd   = GREATEST_VIDEO_PACKET_START_CODE;
    PARSER_Data_p->PESBuffer.PESOneShotEnabled = FALSE;
}

/******************************************************************************/
/*! \brief Initializes the parser context
 *
 * \param ParserHandle the parser handle
 * \return void
 */
/******************************************************************************/
void avspar_InitParserContext(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    U32 i; /* loop index */
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->StartCode.IsPending = FALSE; /* No start code already found */
    PARSER_Data_p->StartCode.SliceFound = FALSE;

    PARSER_Data_p->PrivateGlobalContextData.GlobalContextValid = FALSE;
    PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;

    PARSER_Data_p->PrivatePictureContextData.PictureContextValid = FALSE;

    PARSER_Data_p->ExpectingFieldBDU = FALSE;
    PARSER_Data_p->SeqEndAfterThisSC = FALSE;
    PARSER_Data_p->NextPicRandomAccessPoint = FALSE;

    /* No AUD (Access Unit Delimiter) for next picture */
    PARSER_Data_p->PictureLocalData.PrimaryPicType = PRIMARY_PIC_TYPE_UNKNOWN;
    PARSER_Data_p->NonVOPStartCodeFound = FALSE;
    PARSER_Data_p->UserDataAllowed = FALSE;

    PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID             = 0;
    PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension    = 0;
    PARSER_Data_p->StreamState.CurrentTfcntr                                        = 0;
    PARSER_Data_p->StreamState.BackwardRefTfcntr                                    = 0;
    PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID                 = 0x80000000;    /* minimum value -2147483648 */
    PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension        = 0;
    PARSER_Data_p->StreamState.WaitingRefPic                                        = FALSE;
    PARSER_Data_p->StreamState.RespicBBackwardRefPicture                            = 0;
    PARSER_Data_p->StreamState.RespicBForwardRefPicture                             = 0;
    PARSER_Data_p->StreamState.ForwardRangeredfrmFlagBFrames                        = 0;
    PARSER_Data_p->StreamState.ForwardRangeredfrmFlagPFrames                        = 0;
    /* The first reference picture will be placed at index zero in the FullReferenceFrameList. */
    PARSER_Data_p->StreamState.NextReferencePictureIndex                            = 0;
    PARSER_Data_p->StreamState.RefDistPreviousRefPic                                = 0;

    /* Time code set to 0 */

    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate            = AVS_DEFAULT_FRAME_RATE;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame            = STVID_MPEG_FRAME_I;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure     = STVID_PICTURE_STRUCTURE_FRAME;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst        = FALSE;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.CompressionLevel     = STVID_COMPRESSION_LEVEL_NONE;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.DecimationFactors    = STVID_DECIMATION_FACTOR_NONE;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Seconds     = 0;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Minutes     = 0;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Hours       = 0;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Frames      = 0;

    PARSER_Data_p->PictureGenericData.DecodingOrderFrameID                          = 0;
    PARSER_Data_p->PictureGenericData.IsReference                                   = FALSE;
    PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0                     = 0;
    PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0                     = 0;
    PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1                     = 0;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType             = STGXOBJ_PROGRESSIVE_SCAN;
    PARSER_Data_p->PictureGenericData.AsynchronousCommands.FlushPresentationQueue   = FALSE;
    PARSER_Data_p->PictureGenericData.AsynchronousCommands.FreezePresentationOnThisFrame    = FALSE;
    PARSER_Data_p->PictureGenericData.AsynchronousCommands.ResumePresentationOnThisFrame    = FALSE;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid           = FALSE;
    PARSER_Data_p->PictureGenericData.RepeatFirstField                              = FALSE;
    PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter                      = 0;
    PARSER_Data_p->PictureGenericData.NumberOfPanAndScan                            = 0;
    PARSER_Data_p->PictureGenericData.IsDisplayBoundPictureIDValid                  = FALSE;
    PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields                            = FALSE;
    PARSER_Data_p->PictureGenericData.IsDecodeStartTimeValid                        = FALSE;
    PARSER_Data_p->PictureGenericData.IsPresentationStartTimeValid                  = FALSE;
    PARSER_Data_p->PictureGenericData.ParsingError                                  = VIDCOM_ERROR_LEVEL_NONE;
    PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID         = 0;
    PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension        = 0;
    PARSER_Data_p->PictureGenericData.DisplayBoundPictureID.ID                      = 0;
    PARSER_Data_p->PictureGenericData.DisplayBoundPictureID.IDExtension             = 0;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS              = 0;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33            = 0;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated     = 0;
    PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid            = FALSE;

    for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; i++)
    {
        PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i]           = FALSE;
        PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i]      = FALSE;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[i]                 = TRUE;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[i]                 = TRUE;
        PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[i]                 = TRUE;
    }

    PARSER_Data_p->PictureSpecificData.RndCtrl                                      = 0;
    PARSER_Data_p->PictureSpecificData.BackwardRefDist                              = 0;
    PARSER_Data_p->PictureSpecificData.PictureType                                  = AVS_I_VOP;
    PARSER_Data_p->PictureSpecificData.ForwardRangeredfrmFlag                       = FALSE;
    PARSER_Data_p->PictureSpecificData.HalfWidthFlag                                = FALSE;
    PARSER_Data_p->PictureSpecificData.HalfHeightFlag                               = FALSE;
    PARSER_Data_p->PictureSpecificData.FramePictureLayerSize                        = 0;
    PARSER_Data_p->PictureSpecificData.TffFlag                                      = FALSE;
    PARSER_Data_p->PictureSpecificData.RefDist                                      = 0;
    PARSER_Data_p->PictureSpecificData.ProgressiveSequence                          = 1;/*500;*/

    PARSER_Data_p->PictureSpecificData.PictureError.SeqError.ProfileAndLevel        = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.PicError.CodedVOP               = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.VEDError.ClosedGOV              = FALSE;
    PARSER_Data_p->PictureSpecificData.PictureError.VEDError.BrokenLink             = FALSE;

    PARSER_Data_p->GlobalDecodingContextGenericData.ParsingError                    = VIDCOM_ERROR_LEVEL_NONE;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height             = 0;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width              = 0;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect             = STVID_DISPLAY_ASPECT_RATIO_4TO3;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ScanType           = STVID_SCAN_TYPE_PROGRESSIVE;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate          = AVS_DEFAULT_FRAME_RATE;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate            = AVS_DEFAULT_BIT_RATE;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard       = STVID_AVS_STANDARD_GB_T_20090_2;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.IsLowDelay         = FALSE;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VBVBufferSize      = AVS_DEFAULT_VBVMAX;
    /* NOTE StreamID has already been set earlier */
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication = AVS_DEFAULT_PROFILE_AND_LEVEL;
    PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries                 = 1;    /* COLOUR_PRIMARIES_ITU_R_BT_709 */;
    PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics         = 1;    /* TRANSFER_ITU_R_BT_709 */;
    PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients              = 1;    /* MATRIX_COEFFICIENTS_ITU_R_BT_709 */;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VideoFormat        = 1;    /* PAL for now else UNSPECIFIED_VIDEO_FORMAT; */
    PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.LeftOffset     = 0;
    PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.RightOffset    = 0;
    PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.TopOffset      = 0;
    PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.BottomOffset   = 0;

    PARSER_Data_p->GlobalDecodingContextGenericData.NumberOfReferenceFrames         = 2;
    PARSER_Data_p->GlobalDecodingContextGenericData.MaxDecFrameBuffering            = 2;    /* Number of reference frames (this number +2 gives the number of frame buffers needed for decode */

    /* Unused entries for AVS fill them with a default unused value */
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRateExtensionN = AVS_DEFAULT_NON_RELEVANT;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRateExtensionD = AVS_DEFAULT_NON_RELEVANT;

    /* No PTS found for next picture */
    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
    PARSER_Data_p->PTS_SCList.Write_p          = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
    PARSER_Data_p->PTS_SCList.Read_p           = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
    PARSER_Data_p->LastPTSStored.Address_p     = NULL;
}

/*******************************************************************************/
/*! \brief Sets the ES Range parameters to program the FDMA
 *
 * \param ParserHandle Parser handle
 * \return void
 */
/*******************************************************************************/
void avspar_SetESRange(const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* TraceBuffer(("avspar_SetESRange\n")); */

    PARSER_Data_p->BitBuffer.ES0RangeEnabled   = TRUE;
    PARSER_Data_p->BitBuffer.ES0RangeStart     = SMALLEST_VBS_PACKET_START_CODE;
    PARSER_Data_p->BitBuffer.ES0RangeEnd       = GREATEST_VBS_PACKET_START_CODE;
    PARSER_Data_p->BitBuffer.ES0OneShotEnabled = FALSE;

    /* Range 1 is not used for AVS */
    PARSER_Data_p->BitBuffer.ES1RangeEnabled   = FALSE;
    PARSER_Data_p->BitBuffer.ES1RangeStart     = DUMMY_RANGE; /* don't care, as range 1 is not enabled */
    PARSER_Data_p->BitBuffer.ES1RangeEnd       = DUMMY_RANGE; /* don't care, as range 1 is not enabled */
    PARSER_Data_p->BitBuffer.ES1OneShotEnabled = FALSE; /* don't care, as range 1 is not enabled */

    /* TraceBuffer(("Range 0 : 0x%x, 0x%x \n", PARSER_Data_p->BitBuffer.ES0RangeStart, PARSER_Data_p->BitBuffer.ES0RangeEnd)); */

}

/******************************************************************************/
/*! \brief Setup the parser module
 *
 * Performs a context flush: will start as if it has to decode a first picture.
 * The FDMA starts working
 * (Used for channel change, trick mode controller)
 * Parameters:
 *             PES or ES Input
 *             StreamID
 *             Default GDC table (ends up with a NULL pointer)
 *             Default PDC table (ends up with a NULL pointer)
 *
 * \param ParserHandle
 * \param StartParams_p
 * \return ST_Error_Code
 */
/******************************************************************************/
static ST_ErrorCode_t avspar_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p)
{
    ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    VIDINJ_ParserRanges_t       ParserRanges[3];
#ifdef STVID_TRICKMODE_BACKWARD
    /*U32                        CurrentEPOID, CurrentDOFID;*/
#endif

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    ErrorCode = ST_NO_ERROR;

    /* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Reset pointers in injection module */
    VIDINJ_TransferReset(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Save for future use: is inserted "as is" in the GlobalDecodingContextGenericData.SequenceInfo.StreamID structure */
    PARSER_Data_p->StreamState.StreamID = StartParams_p->StreamID;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.StreamID = StartParams_p->StreamID;


    /* FDMA Setup ----------------- */
    avspar_SetPESRange(ParserHandle, StartParams_p);

    avspar_SetESRange(ParserHandle);

    /* Specify the transfer buffers and parameters to the FDMA */
    /* cascade the buffers params and sc-detect config to inject sub-module */

    /* AVS SC Ranges*/
    ParserRanges[0].RangeId                     = STFDMA_DEVICE_ES_RANGE_0;
    ParserRanges[0].RangeConfig.RangeEnabled    = PARSER_Data_p->BitBuffer.ES0RangeEnabled;
    ParserRanges[0].RangeConfig.RangeStart      = PARSER_Data_p->BitBuffer.ES0RangeStart;
    ParserRanges[0].RangeConfig.RangeEnd        = PARSER_Data_p->BitBuffer.ES0RangeEnd;
    ParserRanges[0].RangeConfig.PTSEnabled      = FALSE;
    ParserRanges[0].RangeConfig.OneShotEnabled  = PARSER_Data_p->BitBuffer.ES0OneShotEnabled;

    /* ES Range 1 is not used in AVS */
    ParserRanges[1].RangeId                                = STFDMA_DEVICE_ES_RANGE_1;
    ParserRanges[1].RangeConfig.RangeEnabled    = PARSER_Data_p->BitBuffer.ES1RangeEnabled;
    ParserRanges[1].RangeConfig.RangeStart      = PARSER_Data_p->BitBuffer.ES1RangeStart;
    ParserRanges[1].RangeConfig.RangeEnd        = PARSER_Data_p->BitBuffer.ES1RangeEnd;
    ParserRanges[1].RangeConfig.PTSEnabled      = FALSE;
    ParserRanges[1].RangeConfig.OneShotEnabled  = PARSER_Data_p->BitBuffer.ES1OneShotEnabled;

    ParserRanges[2].RangeId                     = STFDMA_DEVICE_PES_RANGE_0;
    ParserRanges[2].RangeConfig.RangeEnabled    = PARSER_Data_p->PESBuffer.PESRangeEnabled;
    ParserRanges[2].RangeConfig.RangeStart      = PARSER_Data_p->PESBuffer.PESSCRangeStart;
    ParserRanges[2].RangeConfig.RangeEnd        = PARSER_Data_p->PESBuffer.PESSCRangeEnd;
    ParserRanges[2].RangeConfig.PTSEnabled      = TRUE;
    ParserRanges[2].RangeConfig.OneShotEnabled  = PARSER_Data_p->PESBuffer.PESOneShotEnabled;

    VIDINJ_TransferLimits(PARSER_Data_p->Inject.InjecterHandle,
                                    PARSER_Data_p->Inject.InjectNum,
        /* BB */           PARSER_Data_p->BitBuffer.ES_Start_p,
                                    PARSER_Data_p->BitBuffer.ES_Stop_p,
        /* SC */           PARSER_Data_p->SCBuffer.SCList_Start_p,
                                    PARSER_Data_p->SCBuffer.SCList_Stop_p,
        /* PES */           PARSER_Data_p->PESBuffer.PESBufferBase_p,
                                    PARSER_Data_p->PESBuffer.PESBufferTop_p);

    VIDINJ_SetSCRanges(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum,3, ParserRanges);
        /* sc-detect */        /*3, ParserRanges);*/


    /* Run the FDMA */
    VIDINJ_TransferStart(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, StartParams_p->RealTime);

    /* End of FDMA setup ---------- */

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* @@@ need to set initial global and picture context info here. */
    /* Initialize the parser variables */
#ifdef STVID_TRICKMODE_BACKWARD
    /*if (PARSER_Data_p->Backward)
    {
        CurrentEPOID = PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID;
        CurrentDOFID = PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID;
    }*/
#endif
    avspar_InitParserContext(ParserHandle);
#ifdef STVID_TRICKMODE_BACKWARD
    /*if (PARSER_Data_p->Backward)
    {
        PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID = CurrentDOFID;
        PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID = CurrentEPOID + 1;
    } */
#endif
    avspar_InitParserVariables(ParserHandle);

    PARSER_Data_p->StreamState.ErrorRecoveryMode = StartParams_p->ErrorRecoveryMode;

    /* @@@ need to set this differently when the initial context is passed down*/
    PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;
#ifdef STVID_TRICKMODE_BACKWARD
    if (PARSER_Data_p->Backward)
    {
        PARSER_Data_p->BackwardStop = TRUE;
    }
#endif

    return (ErrorCode);
}

/*******************************************************************************/
/*! \brief  Update the parser SC list write pointer
 *
 * This function is called from inject module when a slice is transfered
 *
 * \param UserIdent the ParserHandle
 * \param ES_Write_p the FDMA write pointer in ES bit buffer
 * \param SCListWrite_p FDMA write pointer in SC List
 * \param SCListLoop_p FDMA write pointer upper bounday in SC List
 */
/*******************************************************************************/
void avspar_DMATransferDoneFct(U32 ParserHandle,void * ES_Write_p,
                               void * SCListWrite_p,
                               void * SCListLoop_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* TraceBuffer(("avspar_DMATransferDoneFct\n")); */

    STOS_SemaphoreWait(PARSER_Data_p->InjectSharedAccess);
#ifdef STVID_TRICKMODE_BACKWARD
    if ((!PARSER_Data_p->Backward) || (PARSER_Data_p->BackwardStop)  || (PARSER_Data_p->BackwardCheckWrite))
#endif /* STVID_TRICKMODE_BACKWARD */
    {
    PARSER_Data_p->BitBuffer.ES_Write_p     = ES_Write_p;
    PARSER_Data_p->SCBuffer.SCList_Write_p  = SCListWrite_p;
    PARSER_Data_p->SCBuffer.SCList_Loop_p   = SCListLoop_p;
#ifdef STVID_TRICKMODE_BACKWARD
        if (PARSER_Data_p->BackwardCheckWrite)
        {
            PARSER_Data_p->BackwardCheckWrite = FALSE;
        }
#endif
    }
    /* TraceBuffer(("avspar_DMATransferDoneFct End\n")); */

    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
    STOS_SemaphoreSignal(PARSER_Data_p->InjectSharedAccess);
}

/******************************************************************************/
/*! \brief Start the parser module
 *
 * Parses One picture: a GDC, PDC and full picture in bit buffer is
 *                     needed to return successfully from this command
 *
 * \param ParserHandle
 * \return ST_Error_Code
 */
/******************************************************************************/
static ST_ErrorCode_t avspar_GetPicture(const PARSER_Handle_t ParserHandle,
                                        const PARSER_GetPictureParams_t * const GetPictureParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;


    AVSParserPrivateData_t * PARSER_Data_p;
    U32 CurrentDOFID /*, CurrentEPOID*/;
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* TraceBuffer(("GetPicture\n")); */

    /* Copy GetPictureParams in local data structure */
    PARSER_Data_p->GetPictureParams.GetRandomAccessPoint = GetPictureParams_p->GetRandomAccessPoint;
#ifdef ST_speed
    PARSER_Data_p->SkipMode = GetPictureParams_p->DecodedPictures;
#endif

    /* When the producer asks for a recovery point, erase context:
     * - DPB is emptied
     * - previous picture information is discarded
     * - global variables are reset
     * The State of the parser is the same as for a "Start" command, except the FDMA is not run again
     */
    if (PARSER_Data_p->GetPictureParams.GetRandomAccessPoint == TRUE)
    {
        /* No need to save the ExtendedPresentationOrderPictureID as it is
           totally managed within the producer for the avs codec */
        /* @@@ CurrentEPOID = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID; */
        CurrentDOFID = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
        avspar_InitParserContext(ParserHandle);
        PARSER_Data_p->PictureGenericData.DecodingOrderFrameID = CurrentDOFID;
    }

    PARSER_Data_p->StreamState.ErrorRecoveryMode = GetPictureParams_p->ErrorRecoveryMode;
    /* Fill job submission time */
    PARSER_Data_p->ParserJobResults.ParsingJobStatus.JobSubmissionTime = time_now();

    /* Wakes up Parser task */
    if (PARSER_Data_p->ParserState == PARSER_STATE_PARSING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVS parser GetPicture: controller attempts to send a parser command when the parser is already working !"));
    }
    STOS_SemaphoreWait(PARSER_Data_p->ControllerSharedAccess);
    PARSER_Data_p->ForTask.ControllerCommand = TRUE;
    STOS_SemaphoreSignal(PARSER_Data_p->ControllerSharedAccess);

    STOS_SemaphoreSignal(PARSER_Data_p->ParserOrder);

    /* The parser will sleep after sending an event and will wait until the controler sends a new command */

    return (ErrorCode);
}
/*******************************************************************************/
/*! \brief Start the parser task
 *
 * \param ParserHandle
 * \return ErrorCode ST_NO_ERROR if success,
 *                   ST_ERROR_ALREADY_INITIALIZED if task already running
 *                   ST_ERROR_BAD_PARAMETER if problem of creation
 */
/*******************************************************************************/
ST_ErrorCode_t avspar_StartParserTask(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    VIDCOM_Task_t * const Task_p = &(PARSER_Data_p->ParserTask);
    char TaskName[25] = "STVID[?].AVSParserTask";

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + ((PARSER_Properties_t *) ParserHandle)->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

    /* TODO: define PARSER own priority in vid_com.h */
    if (STOS_TaskCreate((void (*) (void*)) avspar_ParserTaskFunc,
                             (void *) ParserHandle,
                              ((PARSER_Properties_t *) ParserHandle)->CPUPartition_p,
                              STVID_TASK_STACK_SIZE_DECODE,
                              &(Task_p->TaskStack),
                              ((PARSER_Properties_t *) ParserHandle)->CPUPartition_p,
                              &(Task_p->Task_p),
                              &(Task_p->TaskDesc),
                              STVID_TASK_PRIORITY_PARSE,
                              TaskName,
                              task_flags_no_min_stack_size) != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Task_p->IsRunning  = TRUE;
    return(ST_NO_ERROR);
} /* End of StartParserTask() function */

/*******************************************************************************/
/*! \brief Stop the parser task
 *
 * \param ParserHandle
 * \return ErrorCode ST_NO_ERROR if no problem, ST_ERROR_ALREADY_INITIALIZED if task not running
 */
/*******************************************************************************/
ST_ErrorCode_t avspar_StopParserTask(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    VIDCOM_Task_t * const Task_p = &(PARSER_Data_p->ParserTask);
    task_t *TaskList_p = Task_p->Task_p;

    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    /* Signal semaphore to release task that may wait infinitely if stopped */
    STOS_SemaphoreSignal(PARSER_Data_p->ParserOrder);
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                      ((PARSER_Properties_t *) ParserHandle)->CPUPartition_p,
                      &Task_p->TaskStack,
                      ((PARSER_Properties_t *) ParserHandle)->CPUPartition_p);

    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopParserTask() function */


/******************************************************************************/
/*! \brief Terminates the parser module
 *
 * Deallocate buffers, semaphores and tasks done during Init stage
 * Assumptions : Term can be called in init when init process fails: function to undo
 *               things done at init should not cause trouble to the system
 *
 * \param ParserHandle
 * \return ST_Error_Code
 */
/******************************************************************************/
static ST_ErrorCode_t avspar_Term(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t          * PARSER_Properties_p;

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = (AVSParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    if (PARSER_Data_p == NULL)
    {
        return (ST_ERROR_UNKNOWN_DEVICE);
    }
    /* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Stops the FDMA */
    VIDINJ_TransferStop(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    /* Release FDMA transfers */
    VIDINJ_TransferReleaseInject(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Terminate the parser task */
    avspar_StopParserTask(ParserHandle);

    /* Delete semaphore to share variables between parser and injection tasks */
    /* semaphore_delete(PARSER_Data_p->InjectSharedAccess); */ STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->InjectSharedAccess);

    /* Delete semaphore between parser and controler tasks */
    /* semaphore_delete(PARSER_Data_p->ParserOrder); */ STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ParserOrder);
    /* Delete semaphore for SC lists */
    /* semaphore_delete(PARSER_Data_p->ParserSC); */ STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ParserSC);
    /* Delete semaphore between controller and parser */
    /* semaphore_delete(PARSER_Data_p->ControllerSharedAccess); */ STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ControllerSharedAccess);

    /* Deallocate the STAVMEM partitions */

    /* PES Buffer */
    if(PARSER_Data_p->PESBuffer.PESBufferBase_p != 0)
    {
        /* ErrorCode = */ avspar_DeAllocatePESBuffer(ParserHandle);
    }
    /* SC Buffer */
    if(PARSER_Data_p->SCBuffer.SCList_Start_p != 0)
    {
        /* ErrorCode = */ avspar_DeAllocateSCBuffer(ParserHandle);
    }


    /* De-allocate memory allocated at init */
    if (PARSER_Data_p->UserData.Buff_p != NULL)
    {
        STOS_MemoryDeallocate(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
    }


    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p = NULL;
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p = NULL;
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureSpecificData_p = NULL;

    if (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p != NULL)
    {
        STOS_MemoryDeallocate(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
    }

    if (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p != NULL)
    {
        STOS_MemoryDeallocate(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
    }

    if (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p != NULL)
    {
        STOS_MemoryDeallocate(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
    }

    /* End of deallocate PARSER Job Results data structure */

    /* Deallocate private data */
    if (PARSER_Data_p != NULL)
    {
        STOS_MemoryDeallocate(PARSER_Properties_p->CPUPartition_p, (void *) PARSER_Data_p);
    }
    return (ErrorCode);
}

/******************************************************************************/
/*! \brief Reset the parser module
 *
 * Equivalent to Term + Init call without the buffer deallocation-    The FDMA is stopped
 *
 * \param ParserHandle
 * \return ST_Error_Code
 */
/******************************************************************************/
static ST_ErrorCode_t avspar_Stop(const PARSER_Handle_t ParserHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Stops the FDMA */
    VIDINJ_TransferStop(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    /* TODO: is task "InjecterTask" also stopped anywhere ? */

    /* TODO: what about terminating the parser task ? */

    /* Initialize the SC buffer pointers */
    avspar_InitSCBuffer(ParserHandle);

    /* Reset ES bit buffer write pointer */
    PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    return (ErrorCode);
}

/******************************************************************************/
/*! \brief Get stats from the parser module
 *
 * Get parser statistics (content to be defined based on the current statistics available in STVID)
 *
 * \param ParserHandle
 * \param Statistics_p
 * \return ST_Error_Code
 */
/******************************************************************************/
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t avspar_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;
    UNUSED_PARAMETER(Statistics_p);

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Statistics_p->??? = PARSER_Data_p->Statistics.???; */
    return (ErrorCode);
}
#endif
/******************************************************************************/
/*! \brief Reset parser module statistics
 *
 * Reset Parser statistics
 *
 * \param ParserHandle
 * \param Statistics_p
 * \return ST_Error_Code
 */
/******************************************************************************/
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t avspar_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Reset parameters that are said to be reset (giving value != 0) */
    /* if (Statistics_p->??? != 0)
    {
        PARSER_Data_p->Statistics.??? = 0;
    }
    */
    return (ErrorCode);
}
#endif

/*******************************************************************************/
/*! \brief Point on Next Entry in Start Code List
 *
 *   ASSUMPTION in validation mode:  This function assumes that
 *     PARSER_Data_p->InjectSharedAccess is already "locked" by the
 *     current task and that it will be "unlocked" by a calling function.
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void avspar_PointOnNextSC(const PARSER_Handle_t ParserHandle)
{
    /* Behavior of the SC List filling:
     * SCListWrite is incremented by burst each time a new SCEntry is written
     *             (burst size is the number of SC found within 128 Byte of stream)
     * Before running the FDMA, the injection checks whether SCListWrite is not too close to SCList_Stop_p
     *    - If the remaining space is too low:
     *       - SCListLoop is setup to be equal to SCListWrite (this means that the upper
     *                                       boundary is no longer SCListStop but SCListLoop)
     *       - SCListWrite is set to SCListStart (start filling the SC List from the beginning)
     *    - If there is enough remaining space: SCListLoop is set to be equal to SCListWrite all along
     *          the filling process
     */

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->SCBuffer.NextSCEntry ++;
    if((PARSER_Data_p->SCBuffer.NextSCEntry == PARSER_Data_p->SCBuffer.SCList_Loop_p)
        &&(PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p))
    {
#ifdef STVID_PARSER_DO_NOT_LOOP_ON_STREAM
        PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Write_p;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PARSER : Reached end of Stream"));
#else /*STVID_PARSER_DO_NOT_LOOP_ON_STREAM*/
        /* Reached top of SCList */
        PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Start_p;
#endif /*STVID_PARSER_DO_NOT_LOOP_ON_STREAM*/
    }
#ifdef ST_speed
    PARSER_Data_p->LastSCAdd_p   = (void*)(PARSER_Data_p->StartCode.StartAddress_p);
#endif /* ST_speed */
}

/*******************************************************************************/
/*! \brief Get the next Start Code to process
 *
 * Find a Usable start code.
 * Here we skip over start codes of BDUs that are not valid at this time
 * (For example Frame SCs received before context info is valid),
 * the read pointer in SC+PTS buffer points to the entry
 * following the start code.
 *
 * \param ParserHandle
 * \return PARSER_Data_p->StartCode.IsPending TRUE if start code found
 */
/*******************************************************************************/
void avspar_GetNextStartCodeToProcess(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    BOOL SCFound;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    /* TraceBuffer(("avspar_GetNextStartCodeToProcess\n")); */

    if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p)
    {
        SCFound = FALSE;
        /* search for available SC in SC+PTS buffer : */
        while((PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p) && (!SCFound))
        {
            CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry,&VirtualMapping);
            if(!IsPTS(CPUNextSCEntry_p))
            {
                SCFound = TRUE;
                /* TraceBuffer(("SCfound\n")); */

                /* Get the Start Code itself */
                PARSER_Data_p->StartCode.Value = GetSCVal(CPUNextSCEntry_p);

                /* TraceBuffer(("SC = 0x%x\n", PARSER_Data_p->StartCode.Value)); */

                if (!PARSER_Data_p->NonVOPStartCodeFound)
                {
                    if (PARSER_Data_p->StartCode.Value == USER_DATA_START_CODE && PARSER_Data_p->UserDataAllowed)
                    {
                        PARSER_Data_p->NonVOPStartCodeAddress = (U8 *)GetSCAdd(CPUNextSCEntry_p);
                        PARSER_Data_p->NonVOPStartCodeFound = TRUE;
                    }
                    else if (PARSER_Data_p->StartCode.Value != USER_DATA_START_CODE)
                    {
                        PARSER_Data_p->NonVOPStartCodeAddress = (U8 *)GetSCAdd(CPUNextSCEntry_p);
                        PARSER_Data_p->NonVOPStartCodeFound = TRUE;
                    }
                }

                /* Set the Parser Read Pointer in ES Bit Buffer on the start address of the start code */
                PARSER_Data_p->StartCode.StartAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
                /* TraceBuffer(("SC found at 0x%x\n", (U32)PARSER_Data_p->StartCode.StartAddress_p)); */

                /* A valid current start code is available */
                PARSER_Data_p->StartCode.IsPending = TRUE;
                PARSER_Data_p->StartCode.IsInBitBuffer = FALSE; /* The parser must now look for the next start code to know if the NAL is fully in the bit buffer */
           }
           else
           {
               avspar_SavePTSEntry(ParserHandle, CPUNextSCEntry_p);
           }
           /* Make ScannedEntry_p point on the next entry in SC List */
           avspar_PointOnNextSC(ParserHandle);
        } /* end while */
    }    /* if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p)*/
}    /* end avspar_GetNextStartCodeToProcess() */


/*******************************************************************************/
/*! \brief Check if the VBS is fully in bit buffer: this is mandatory to process it.
 *
 *
 * \param ParserHandle
 * \return PARSER_Data_p->StartCode.IsInBitBuffer TRUE if fully in bit buffer
 */
/*******************************************************************************/
void avspar_IsVOPInBitBuffer(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
    BOOL SCFound;
    U8 SCUnitType, NextSCUnitType;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    /* Saves the NextSCEntry before proceeding over the SC+PTS buffer */
    PARSER_Data_p->SCBuffer.SavedNextSCEntry_p = PARSER_Data_p->SCBuffer.NextSCEntry;

    if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p)
    {
        SCFound = FALSE;

        /* search for available SC in SC+PTS buffer : */
        while((PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p) && (!SCFound))
        {
            CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry,&VirtualMapping);
            if(!IsPTS(CPUNextSCEntry_p))
            {
                NextSCUnitType = GetUnitType(GetSCVal(CPUNextSCEntry_p));

                /* when the start code to process is not a VOP or picture start code, having any start code stored in the
                 * SC+PTS buffer is enough to say that the start code to process is fully in the bit buffer
                 */

                SCUnitType = GetUnitType(PARSER_Data_p->StartCode.Value);

                if( (SCUnitType != I_PICTURE_START_CODE)&&(SCUnitType != PB_PICTURE_START_CODE))
                {
                    switch(NextSCUnitType)
                    {
                        case VIDEO_SEQUENCE_START_CODE:
                        case EXTENSION_START_CODE:
                        case I_PICTURE_START_CODE:
                        case PB_PICTURE_START_CODE:
                        case USER_DATA_START_CODE:
                        case VIDEO_SEQUENCE_END_CODE:
                        case VIDEO_EDIT_START_CODE:
                            SCFound = TRUE; /* to exit of the loop */
                            PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
                            /* Stop address of the SC to process is the start address of the following SC */
                            PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
                            break;

                       default:
                            break;
                    }

                }
                else
                {

                    /* The start code to process is a VOP start code. Before processing it, the parser must ensure the
                     * VOP/picture is fully in the bit buffer.
                     * The VOP is fully in the bit buffer when any of the following start code are encountered after
                     * the VOP start code:
                     */
                    if(NextSCUnitType==0)
                    {
                        PARSER_Data_p->StartCode.SliceStartAddress_p =(U8 *)GetSCAdd(CPUNextSCEntry_p);
                    }

                    if (NextSCUnitType >= VIDEO_SEQUENCE_START_CODE && NextSCUnitType <= VIDEO_EDIT_START_CODE)
                    {
                        SCFound = TRUE; /* to exit of the loop */
                        PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
                        /* Stop address of VOP to process is the start address of the following SC */
                        PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
                    }
                    if (NextSCUnitType >  VIDEO_EDIT_START_CODE && NextSCUnitType<GREATEST_VBS_PACKET_START_CODE)
                    {
                        SCFound = TRUE; /* to exit of the loop */
                        PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
                        /* Stop address of VOP to process is the start address of the following SC */
                        PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
                    }



                }

                if(NextSCUnitType == VIDEO_SEQUENCE_END_CODE)
                {
                    PARSER_Data_p->SeqEndAfterThisSC = TRUE;
                }

            }/* end if(!IsPTS(CPUNextSCEntry_p)) */
            /* Make ScannedEntry_p point on the next entry in SC List */
            avspar_PointOnNextSC(ParserHandle);
        } /* end while */
    }

    /* Restores the NextSCEntry after proceeding over the SC+PTS buffer */
    PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SavedNextSCEntry_p;
}


/*******************************************************************************/
/*! \brief Check whether there is a Start Code to process
 *
 * If yes, a start code is pushed and a ParserOrder is sent (semaphore)
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void avspar_IsThereStartCodeToProcess(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
#ifdef STVID_TRICKMODE_BACKWARD
    STFDMA_SCEntry_t   *    CheckSC;
    PARSER_Properties_t * PARSER_Properties_p;
    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
#endif /*STVID_TRICKMODE_BACKWARD*/
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* TraceBuffer(("IsThereaScToProcess\n")); */

    /* Test if a new start-code is available */
    /*---------------------------------------*/
    STOS_SemaphoreWait(PARSER_Data_p->InjectSharedAccess);
    if (!(PARSER_Data_p->StartCode.IsPending)) /* Look for a start code to process */
    {
        avspar_GetNextStartCodeToProcess(ParserHandle);
#ifdef STVID_TRICKMODE_BACKWARD
        CheckSC = PARSER_Data_p->SCBuffer.NextSCEntry;
        CheckSC++;
        if ((CheckSC == PARSER_Data_p->SCBuffer.SCList_Write_p) && (PARSER_Data_p->Backward) /* && (!PARSER_Data_p->ParserJobResults.BackwardBufferFullyParsed)*/ )   /* PLE_TO_DO removed BackwardBufferFullyParsed usage because not available any more. Safe until  trickmode backward implementation. */
        {
            /* PARSER_Data_p->ParserJobResults.BackwardBufferFullyParsed = TRUE; */ /* PLE_TO_DO removed BackwardBufferFullyParsed usage because not available any more. Safe until  trickmode backward implementation. */
            STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->ParserJobResults));
            STTBX_Print(("\n BB fully parsed \r\n"));
        }
#endif
        /* PARSER_Data_p->StartCode.IsPending is modified in the function above */
    }

    if (PARSER_Data_p->StartCode.IsPending) /* Look if the VOP is fully in bit buffer */
    {
        avspar_IsVOPInBitBuffer(ParserHandle);
    }

    /* inform inject module about the SC-list read progress */
    VIDINJ_TransferSetSCListRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, PARSER_Data_p->SCBuffer.NextSCEntry);

    STOS_SemaphoreSignal(PARSER_Data_p->InjectSharedAccess);
}

/******************************************************************************/
/*! \brief Initialize the ByteStream data structure on current BDU
 *
 * \param StartAddressInBB_p the start address in memory for this entry
 * \param StopAddressInBB_p the start address in memory for this entry
 * \param pp the parser variables (updated)
 * \return 0
 */
/******************************************************************************/
void avspar_InitByteStream(U8 * StartAddress, U8 * NextStartAddress, const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    /* TraceBuffer(("avspar_InitByteStream\n")); */

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if (NextStartAddress > StartAddress)
    {
        PARSER_Data_p->ByteStream.Len = NextStartAddress - StartAddress;
    }
    else
    {
        /* Bit Buffer wrap */
        PARSER_Data_p->ByteStream.Len = PARSER_Data_p->BitBuffer.ES_Stop_p
                                           - StartAddress
                                           + NextStartAddress
                                           - PARSER_Data_p->BitBuffer.ES_Start_p
                                           + 1;
    }
    PARSER_Data_p->ByteStream.ByteSwapFlag = FALSE;
    PARSER_Data_p->ByteStream.WordSwapFlag = FALSE;
    PARSER_Data_p->ByteStream.BitOffset = 7; /* ready to read the MSB of the first byte */
    if (!(PARSER_Data_p->ByteStream.WordSwapFlag))
    {
        PARSER_Data_p->ByteStream.ByteOffset = 0; /* ready to read first byte */
    }
    else
    {
        PARSER_Data_p->ByteStream.ByteOffset = 3; /* ready to read first byte */
    }
    PARSER_Data_p->ByteStream.EndOfStreamFlag = FALSE;
    PARSER_Data_p->ByteStream.LastByte = 0xFF; /* do not initialize to 0 to avoid wrong detection on first bytes */
    PARSER_Data_p->ByteStream.LastButOneByte = 0xFF; /* do not initialize to 0 to avoid wrong detection on first bytes */
    PARSER_Data_p->ByteStream.ByteCounter = 0; /* beginning of stream */

    /* Translate "Device" address to "CPU" address */
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    PARSER_Data_p->ByteStream.StartByte_p = STAVMEM_DeviceToCPU(StartAddress, &VirtualMapping);

    /* The stream is initialized on the byte following the start code value (returned by the FDMA) */
    avspar_MoveToNextByteInByteStream(ParserHandle);
}


/*******************************************************************************/
/*! \brief Process the start code
 *
 * At that stage, the full VOP is in the bit buffer
 *
 * \param StartCode
 * \param StartAddress in bit buffer
 * \param StopAddress in bit buffer
 * \param ParserHandle
 */
/*******************************************************************************/
void avspar_ProcessStartCode(U8 StartCode, U8 * StartCodeValueAddress, U8 * NextPictureStartCodeValueAddress, const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    U8 SCUnitType;

    AVS_GlobalDecodingContextSpecificData_t *GDCSD;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    GDCSD= &PARSER_Data_p->GlobalDecodingContextSpecificData;

    /* TraceBuffer(("avspar_ProcessStartCode\n")); */

    /* Initialize the ByteStream_t data structure to parse the SC entry in bit buffer */
    avspar_InitByteStream(StartCodeValueAddress, NextPictureStartCodeValueAddress, ParserHandle);

    SCUnitType = GetUnitType(StartCode);

    switch (SCUnitType)
    {
        case VIDEO_SEQUENCE_START_CODE:
            /* TraceBuffer(("SeqSC\n")); */

            avspar_ParseVideoSequenceHeader(ParserHandle);
            if (GDCSD->FirstPictureEver == FALSE)
            {
                GDCSD->PictureDistanceOffset += GDCSD->PictureCounter;
                GDCSD->PictureCounter = 0;
                if ((GDCSD->PreviousGreatestExtendedPictureDistance > 0) &&
                    (GDCSD->PictureDistanceOffset <= GDCSD->PreviousGreatestExtendedPictureDistance))
                {

                    GDCSD->PictureDistanceOffset = GDCSD->PreviousGreatestExtendedPictureDistance + 1;
                }
                GDCSD->OnePictureDistanceOverflowed = FALSE;
            }
            break;
        case EXTENSION_START_CODE:
            /*TBD if color matrix needed in sequence extension header*/
            break;
        case USER_DATA_START_CODE:
            avspar_ParseUserDataHeader(ParserHandle);
            break;
        case I_PICTURE_START_CODE:
            /* TraceBuffer(("Isc\n")); */

            avspar_ParseIntraPictureHeader (StartCodeValueAddress, NextPictureStartCodeValueAddress, ParserHandle);
            break;
        case PB_PICTURE_START_CODE:
            /* TraceBuffer(("PBsc\n")); */

            avspar_ParseInterPictureHeader (StartCodeValueAddress, NextPictureStartCodeValueAddress, ParserHandle);
            break;
        case VIDEO_SEQUENCE_END_CODE:
            break;
        case VIDEO_EDIT_START_CODE:
            avspar_ParseVideoEditCode (ParserHandle);
            break;
        default:
            break;

    }
}/* end avspar_ProcessStartCode()*/


/*******************************************************************************/
/*! \brief Fill the Parsing Job Result Data structure
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void avspar_PictureInfoCompile(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    BOOL PictureHasError;
    long PreviousExtendedPictureDistance;
    AVS_GlobalDecodingContextSpecificData_t * GDCSD;
    AVS_PictureSpecificData_t * PSD;
    VIDCOM_PictureGenericData_t * PGD;
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PictureHasError = FALSE;

    /* If this is a reference frame, then the oldest reference frame will
       No longer be used for reference.  That is, if there are 2 previous reference
       frames, the newest of these two may be used as a reference for the current
       reference frame and may be used as a forward reference for any future
       B pictures, but the oldest of the two will no longer be used, so we can
       mark it as unused so that the producer can now re-use the memory. Also,
       the first B field in a field-pair can be reference for the second
       B field in a field-pair.  In this case, the oldest reference
       picture is still needed as the forward reference frame for the
       B/B field pair.*/
    if((PARSER_Data_p->PictureGenericData.IsReference) &&
       (PARSER_Data_p->PictureSpecificData.PictureType != AVS_B_VOP))
    {
        PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame
            [PARSER_Data_p->StreamState.NextReferencePictureIndex] = FALSE;
    }

    /* Global Decoding Context structure */
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->
        GlobalDecodingContextGenericData = PARSER_Data_p->GlobalDecodingContextGenericData;
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->
        SizeOfGlobalDecodingContextSpecificDataInByte = sizeof(AVS_GlobalDecodingContextSpecificData_t);
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->
        GlobalDecodingContextSpecificData_p = &(PARSER_Data_p->GlobalDecodingContextSpecificData);

    /* Picture Decoding Context structure */
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->
        SizeOfPictureDecodingContextSpecificDataInByte = sizeof(AVS_PictureDecodingContextSpecificData_t);
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->
        PictureDecodingContextSpecificData_p = &(PARSER_Data_p->PictureDecodingContextSpecificData);

    /* Parsed Picture Information structure */
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData = PARSER_Data_p->PictureGenericData;
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->SizeOfPictureSpecificDataInByte = sizeof(AVS_PictureSpecificData_t);
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureSpecificData_p = &(PARSER_Data_p->PictureSpecificData);

    /* Parsing Job Status */

    /* Check whether picture has errors */
    if (PARSER_Data_p->PictureSpecificData.PictureError.SeqError.ProfileAndLevel ||
        PARSER_Data_p->PictureSpecificData.PictureError.PicError.CodedVOP ||
        PARSER_Data_p->PictureSpecificData.PictureError.VEDError.ClosedGOV ||
        PARSER_Data_p->PictureSpecificData.PictureError.VEDError.BrokenLink
       )
    {
        PictureHasError = TRUE;
    }

    if (PictureHasError)
    {
        PARSER_Data_p->ParserJobResults.ParsingJobStatus.HasErrors = TRUE;
        PARSER_Data_p->ParserJobResults.ParsingJobStatus.ErrorCode = PARSER_ERROR_STREAM_SYNTAX;
    }
    else
    {
        PARSER_Data_p->ParserJobResults.ParsingJobStatus.HasErrors = FALSE;
        PARSER_Data_p->ParserJobResults.ParsingJobStatus.ErrorCode = PARSER_NO_ERROR;
    }

#ifdef ST_speed
     /* PARSER_Data_p->ParserJobResults.FoundDiscontinuity = FALSE; */ /* PLE_TO_DO removed FoundDiscontinuity usage because not available any more. To be definitively removed. */
#endif /* ST_speed */

   GDCSD= (AVS_GlobalDecodingContextSpecificData_t *)PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
   PSD =  (AVS_PictureSpecificData_t *)PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureSpecificData_p;
   PGD =  &PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData;
   if (GDCSD->FirstPictureEver)
   {
        GDCSD->ExtendedPictureDistance = PSD->PictureDistance;
        PreviousExtendedPictureDistance = (-1);
        GDCSD->PictureDistanceOffset = 0;
        GDCSD->OnePictureDistanceOverflowed = FALSE;
        GDCSD->PreviousGreatestExtendedPictureDistance = GDCSD->ExtendedPictureDistance;
        GDCSD->PictureCounter = 0;
        GDCSD->FirstPictureEver=FALSE;
    }
    else
    {

        PreviousExtendedPictureDistance = GDCSD->ExtendedPictureDistance;
        GDCSD->ExtendedPictureDistance  = GDCSD->PictureDistanceOffset + PSD->PictureDistance;
        GDCSD->PictureCounter++;
    }

    if (PSD->PictureType == AVS_B_VOP)
    {

        if (((S32)(PreviousExtendedPictureDistance - GDCSD->ExtendedPictureDistance)) >= (PICTURE_DISTANCE_MODULO - 1))
        {
            GDCSD->ExtendedPictureDistance += PICTURE_DISTANCE_MODULO;
            GDCSD->PictureDistanceOffset   += PICTURE_DISTANCE_MODULO;
            GDCSD->OnePictureDistanceOverflowed = FALSE;
        }
    }
    else
    {

        if (GDCSD->OnePictureDistanceOverflowed)
        {
            GDCSD->ExtendedPictureDistance += PICTURE_DISTANCE_MODULO;
            GDCSD->PictureDistanceOffset   += PICTURE_DISTANCE_MODULO;
            GDCSD->OnePictureDistanceOverflowed = FALSE;
        }
        else
        {
            if (((S32)(PreviousExtendedPictureDistance - GDCSD->ExtendedPictureDistance)) >0)
            {
                GDCSD->ExtendedPictureDistance += PICTURE_DISTANCE_MODULO;
                GDCSD->OnePictureDistanceOverflowed = TRUE;
            }
        }
        if (GDCSD->ExtendedPictureDistance < GDCSD->PreviousGreatestExtendedPictureDistance)
        {
            GDCSD->PictureDistanceOffset +=
            (GDCSD->PreviousGreatestExtendedPictureDistance - GDCSD->ExtendedPictureDistance - 1);
            GDCSD->ExtendedPictureDistance = GDCSD->PreviousGreatestExtendedPictureDistance + 1;
        }

        if (GDCSD->ExtendedPictureDistance > GDCSD->PreviousGreatestExtendedPictureDistance)
        {
            GDCSD->PreviousGreatestExtendedPictureDistance = GDCSD->ExtendedPictureDistance;
        }
    }

    PGD->ExtendedPresentationOrderPictureID.ID = GDCSD->ExtendedPictureDistance - GDCSD->PictureDistanceOffset;
    PGD->ExtendedPresentationOrderPictureID.IDExtension = GDCSD->PictureDistanceOffset;
    PGD->IsExtendedPresentationOrderIDValid = TRUE;

    PARSER_Data_p->ParserJobResults.ParsingJobStatus.JobCompletionTime = time_now();
}
#if defined (VALID_TOOLS)
/*******************************************************************************/
/*! \brief dump the Parsing Job Result Data structure
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void avspar_PictureDumpParsingJobResult(const PARSER_Handle_t ParserHandle)
{
    AVSParserPrivateData_t * PARSER_Data_p;
    U32 counter;
    static FILE *outFile = NULL;


    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    PARSER_ParsingJobResults_t  * PJR = &PARSER_Data_p->ParserJobResults;
    VIDCOM_ParsedPictureInformation_t *PPI = PJR->ParsedPictureInformation_p;
    PARSER_ParsingJobStatus_t         *PJS = &PJR->ParsingJobStatus;
    VIDCOM_PictureGenericData_t       *PGD = &PPI->PictureGenericData;
    VIDCOM_PictureDecodingContext_t   *PDC = PPI->PictureDecodingContext_p;
    STVID_VideoParams_t               *VP  = &PGD->PictureInfos.VideoParams;
    STGXOBJ_Bitmap_t                  *BP  = &PGD->PictureInfos.BitmapParams;
    VIDCOM_GlobalDecodingContext_t    *GDC = PDC->GlobalDecodingContext_p;
    VIDCOM_GlobalDecodingContextGenericData_t  *GDCGD = &GDC->GlobalDecodingContextGenericData;
    STVID_SequenceInfo_t              *SI  = &GDCGD->SequenceInfo;


    AVS_PictureSpecificData_t         *PSD = &PARSER_Data_p->PictureSpecificData;
    AVS_GlobalDecodingContextSpecificData_t  *GDCSD = &PARSER_Data_p->GlobalDecodingContextSpecificData;
    AVS_PictureDecodingContextSpecificData_t *PDCSD = (AVS_PictureDecodingContextSpecificData_t *)PDC->PictureDecodingContextSpecificData_p;

    if (outFile == NULL)
    {
        osclock_t TOut_Now;
        char FileName[30];

        TOut_Now = time_now();

        sprintf(FileName,"../../AVSPJR-%010u.txt", (U32)TOut_Now);

        outFile = fopen(FileName, "w" );
    }

    fprintf (outFile, "ParsingJobResults (%d)\n", PGD->DecodingOrderFrameID);

    fprintf (outFile, "  ParsingJobStatus\n");
    fprintf (outFile, "    HasErrors = %d\n", PJS->HasErrors);
    fprintf (outFile, "    ErrorCode = %d\n", PJS->ErrorCode);


    fprintf (outFile, "  Parsed Picture Information\n");
    fprintf (outFile, "    PictureGenericData\n");
    fprintf (outFile, "      IsPTSValid %d\n", PGD->PictureInfos.VideoParams.PTS.IsValid);
    fprintf (outFile, "      RepeatFirstField %d\n", PGD->RepeatFirstField);
    fprintf (outFile, "      RepeatProgressiveCounter %d\n", PGD->RepeatProgressiveCounter);

    fprintf (outFile, "      DecodingOrderFrameID %d\n", PGD->DecodingOrderFrameID);
    fprintf (outFile, "      ExtendedPresentationOrderPictureID: ID = %d    IDExtension = %d\n",
             PGD->ExtendedPresentationOrderPictureID.ID, PGD->ExtendedPresentationOrderPictureID.IDExtension);
    fprintf (outFile, "      IsFirstOfTwoFields %d\n", PGD->IsFirstOfTwoFields);
    fprintf (outFile, "      IsReference %d\n", PGD->IsReference);

    fprintf (outFile, "      FullReferenceFrameList:\n");


    for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; counter++)
    {
        if (PGD->IsValidIndexInReferenceFrame[counter])
        {
            fprintf (outFile, "      FullReferenceFrameList[%d] %d\n", counter, PGD->FullReferenceFrameList[counter]);
        }
    }

    fprintf (outFile, "      UsedSizeInReferenceListP0 %d\n", PGD->UsedSizeInReferenceListP0);
    for (counter = 0; counter < PGD->UsedSizeInReferenceListP0; counter++)
    {
        fprintf (outFile, "      ReferenceListP0[%d]  %d\n", counter, PGD->ReferenceListP0[counter]);
    }

    fprintf (outFile, "      UsedSizeInReferenceListB0 %d\n", PGD->UsedSizeInReferenceListB0);
    for (counter = 0; counter < PGD->UsedSizeInReferenceListB0; counter++)
    {
        fprintf (outFile, "      ReferenceListB0[%d]  %d\n", counter, PGD->ReferenceListB0[counter]);
    }

    fprintf (outFile, "      UsedSizeInReferenceListB1 %d\n", PGD->UsedSizeInReferenceListB1);
    for (counter = 0; counter < PGD->UsedSizeInReferenceListB1; counter++)
    {
        fprintf (outFile, "      ReferenceListB1[%d]  %d\n", counter, PGD->ReferenceListB1[counter]);
    }

    fprintf (outFile, "      ParsingError %d\n", PGD->ParsingError);
    fprintf (outFile, "      PictureInfos\n");
    fprintf (outFile, "        VideoParams\n");
    fprintf (outFile, "          FrameRate %d\n", VP->FrameRate);
    fprintf (outFile, "          ScanType %s\n", (VP->ScanType == 0) ? "STGXOBJ_PROGRESSIVE_SCAN" : "STGXOBJ_INTERLACED_SCAN");
    fprintf (outFile, "          PictureStructure %s\n", (VP->PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD) ? "STVID_PICTURE_STRUCTURE_TOP_FIELD": (VP->PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD) ? "STVID_PICTURE_STRUCTURE_BOTTOM_FIELD" : "STVID_PICTURE_STRUCTURE_FRAME");
    fprintf (outFile, "          TopFieldFirst %d\n", VP->TopFieldFirst);
    fprintf (outFile, "          TimeCode %d hr  %d min  %d sec  %d frames  interpolated = %d\n", VP->TimeCode.Hours, VP->TimeCode.Minutes,
             VP->TimeCode.Seconds, VP->TimeCode.Frames, VP->TimeCode.Interpolated );
    fprintf (outFile, "          PTS upper = %d   lower = %d   Interpolated = %d\n", VP->PTS.PTS33, VP->PTS.PTS, VP->PTS.Interpolated);

    printf (outFile, "          DecimationFactors %s\n", (VP->DecimationFactors == 1) ? "STVID_DECIMATION_FACTOR_H2":
                                                         (VP->DecimationFactors == 2) ? "STVID_DECIMATION_FACTOR_V2":
                                                         (VP->DecimationFactors == 4) ? "STVID_DECIMATION_FACTOR_H4":
                                                         (VP->DecimationFactors == 8) ? "STVID_DECIMATION_FACTOR_V4":
                                                         (VP->DecimationFactors == 3) ? "STVID_DECIMATION_FACTOR_2": "STVID_DECIMATION_FACTOR_4");
    fprintf (outFile, "        BitmapParams\n");
    fprintf (outFile, "          ColorType %d\n", BP->ColorType);
    fprintf (outFile, "          BitmapType %d\n", BP->BitmapType);
    fprintf (outFile, "          PreMultipliedColor %d\n", BP->PreMultipliedColor);

    fprintf (outFile, "          Width %d\n", BP->Width);
    fprintf (outFile, "          Height %d\n", BP->Height);
    fprintf (outFile, "          Pitch %d\n", BP->Pitch);
    fprintf (outFile, "          Offset %d\n", BP->Offset);



    /*picture specifique data*/



    fprintf (outFile, "    PictureSpecificData\n");
    fprintf (outFile, "      RndCtrl %d\n", PSD->RndCtrl);
    fprintf (outFile, "      Backward Ref Distance %d\n", PSD->BackwardRefDist);
    fprintf (outFile, "      Picture Type %s\n", (PSD->PictureType == 0) ? "AVS_I_VOP" : (PSD->PictureType == 1) ? "AVS_P_VOP" : "AVS_B_VOP");
    fprintf (outFile, "      ForwardRangeredfrmFlag %s\n", (PSD->ForwardRangeredfrmFlag == 0) ? "False" : "True" );
    fprintf (outFile, "      HalfWidthFlag %s\n",(PSD->HalfWidthFlag == 0) ? "False" : "True" );
    fprintf (outFile, "      HalfWidthFlag %s\n",(PSD->HalfHeightFlag == 0) ? "False" : "True" );

    for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; counter++)
    {
        fprintf (outFile, "      IsReferenceTopFieldP0[%d]= %s\n", counter,(PSD->IsReferenceTopFieldP0[counter] == 0) ? "False" : "True" );
    }

    for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; counter++)
    {
        fprintf (outFile, "      IsReferenceTopFieldB0[%d]= %s\n", counter,(PSD->IsReferenceTopFieldB0[counter] == 0) ? "False" : "True" );
    }

    for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; counter++)
    {
        fprintf (outFile, "      IsReferenceTopFieldB1[%d]= %s\n", counter,(PSD->IsReferenceTopFieldB1[counter] == 0) ? "False" : "True" );
    }


    fprintf (outFile, "      FramePictureLayerSize %d\n", PSD->FramePictureLayerSize);

    fprintf (outFile, "      TffFlag %s\n",(PSD->TffFlag == 0) ? "False" : "True" );
    fprintf (outFile, "      RefDist %d\n", PSD->RefDist);
    fprintf (outFile, "      Sequence Error on profile and level %s\n",(PSD->PictureError.SeqError.ProfileAndLevel == 0) ? "False" : "True" );
    fprintf (outFile, "      Picture Error %s\n",(PSD->PictureError.PicError.CodedVOP == 0) ? "False" : "True" );
    fprintf (outFile, "      ClosedGOV Error %s\n",(PSD->PictureError.VEDError.ClosedGOV == 0) ? "False" : "True" );
    fprintf (outFile, "      BrokenLink Error %s\n",(PSD->PictureError.VEDError.BrokenLink == 0) ? "False" : "True" );
    fprintf (outFile, "      ProgressiveSequence %d\n", PSD->ProgressiveSequence);
    fprintf (outFile, "      Progressive_frame %d\n", PSD->Progressive_frame);
    fprintf (outFile, "      PictureCodingType %d\n", PSD->PictureCodingType);
    fprintf (outFile, "      PictureDistance %d\n", PSD->PictureDistance);
    fprintf (outFile, "      PictureStructure %d\n", PSD->PictureStructure);
    fprintf (outFile, "      FixedPictureQP %d\n", PSD->FixedPictureQP);
    fprintf (outFile, "      PictureQP %d\n", PSD->PictureQP);
    fprintf (outFile, "      PictureReferenceFlag %d\n", PSD->PictureReferenceFlag);
    fprintf (outFile, "      SkipModeFlag %d\n", PSD->SkipModeFlag);
    fprintf (outFile, "      LoopFilterDisable %d\n", PSD->LoopFilterDisable);
    fprintf (outFile, "      LoopFilterParameterFlag %d\n", PSD->LoopFilterParameterFlag);
    fprintf (outFile, "      AlphaCOffset %d\n", PSD->AlphaCOffset);
    fprintf (outFile, "      BetaOffset %d\n", PSD->BetaOffset);
    fprintf (outFile, "      TopFieldPos %d\n", PSD->TopFieldPos);
    fprintf (outFile, "      BotFieldPos %d\n", PSD->BotFieldPos);


    fprintf (outFile, "    PictureDecodingContext_p\n", (U32)PDC);
    fprintf (outFile, "      PanScanFlag %s\n", (PDCSD->PanScanFlag == 0) ? "False" : "True" );
    fprintf (outFile, "      RefDistFlag %s\n", (PDCSD->RefDistFlag == 0) ? "False" : "True");
    fprintf (outFile, "      LoopFilterFlag %s\n", (PDCSD->LoopFilterFlag == 0) ? "False" : "True");
    fprintf (outFile, "      VstransformFlag %s\n", (PDCSD->VstransformFlag == 0) ? "False" : "True");
    fprintf (outFile, "      OverlapFlag %s\n", (PDCSD->OverlapFlag == 0) ? "False" : "True");
    fprintf (outFile, "      CodedWidth %d\n", PDCSD->CodedWidth);
    fprintf (outFile, "      CodedHeight %d\n", PDCSD->CodedHeight);
    fprintf (outFile, "      Quantizer %c\n", PDCSD->Quantizer);
    fprintf (outFile, "      Dquant %c\n", PDCSD->Dquant);

    fprintf (outFile, "      GlobalDecodingContext_p\n", PDC->GlobalDecodingContext_p);
    fprintf (outFile, "        GlobalDecodingContextGenericData\n");
    fprintf (outFile, "          ColourPrimaries %d\n", GDCGD->ColourPrimaries);
    fprintf (outFile, "          TransferCharacteristics %d\n", GDCGD->TransferCharacteristics);

    fprintf (outFile, "          MatrixCoefficients %d\n", GDCGD->MatrixCoefficients);
    fprintf (outFile, "          FrameCropInPixel TopOffset %d  LeftOffset %d  BottomOffset %d  RightOffset %d\n",
             GDCGD->FrameCropInPixel.TopOffset, GDCGD->FrameCropInPixel.LeftOffset,
             GDCGD->FrameCropInPixel.BottomOffset, GDCGD->FrameCropInPixel.RightOffset);
    fprintf (outFile, "          NumberOfReferenceFrames %d\n", GDCGD->NumberOfReferenceFrames);
    fprintf (outFile, "          SequenceInfo\n");
    fprintf (outFile, "            Height %d\n", SI->Height);
    fprintf (outFile, "            Width %d\n", SI->Width);
    fprintf (outFile, "            Aspect %s\n", (SI->Aspect == 1) ? "STVID_DISPLAY_ASPECT_RATIO_16TO9" :
                                                 (SI->Aspect == 2) ? "STVID_DISPLAY_ASPECT_RATIO_4TO3" :
                                                 (SI->Aspect == 4) ? "STVID_DISPLAY_ASPECT_RATIO_221TO1" : "STVID_DISPLAY_ASPECT_RATIO_SQUARE");
    fprintf (outFile, "            ScanType %2\n", (SI->ScanType == 1) ? "STVID_SCAN_TYPE_PROGRESSIVE" : "STVID_SCAN_TYPE_INTERLACED");
    fprintf (outFile, "            FrameRate %d\n", SI->FrameRate);
    fprintf (outFile, "            BitRate %d\n", SI->BitRate);
    fprintf (outFile, "            MPEGStandard %d\n", SI->MPEGStandard);
    fprintf (outFile, "            IsLowDelay %d\n", SI->IsLowDelay);
    fprintf (outFile, "            VBVBufferSize %d\n", SI->VBVBufferSize);
    fprintf (outFile, "            StreamID %d\n", SI->StreamID);
    fprintf (outFile, "            ProfileAndLevelIndication %d\n", SI->ProfileAndLevelIndication);
    fprintf (outFile, "            VideoFormat %d\n", SI->VideoFormat);
    fprintf (outFile, "            FrameRateExtensionN %d\n", SI->FrameRateExtensionN);
    fprintf (outFile, "            FrameRateExtensionD %d\n", SI->FrameRateExtensionD);


    fprintf (outFile, "       GlobalDecodingContextSpecificData_t\n");
    fprintf (outFile, "          QuantizedFrameRateForPostProcessing %d\n", GDCSD->QuantizedFrameRateForPostProcessing);
    fprintf (outFile, "          QuantizedBitRateForPostProcessing %d\n", GDCSD->QuantizedBitRateForPostProcessing);
    fprintf (outFile, "          PostProcessingFlag %s\n", (GDCSD->PostProcessingFlag == 0) ? "False" : "True");
    fprintf (outFile, "          PulldownFlag %s\n", (GDCSD->PulldownFlag == 0) ? "False" : "True");
    fprintf (outFile, "          InterlaceFlag %s\n", (GDCSD->InterlaceFlag == 0) ? "False" : "True");
    fprintf (outFile, "          TfcntrFlag %s\n", (GDCSD->TfcntrFlag == 0) ? "False" : "True");
    fprintf (outFile, "          FinterpFlag %s\n", (GDCSD->FinterpFlag == 0) ? "False" : "True");
    fprintf (outFile, "          ProgressiveSegmentedFrame %s\n", (GDCSD->ProgressiveSegmentedFrame == 0) ? "False" : "True");
    fprintf (outFile, "          AspectNumerator %d\n", GDCSD->AspectNumerator);
    fprintf (outFile, "          AspectDenominator %d\n", GDCSD->AspectDenominator);
    fprintf (outFile, "          NumLeakyBuckets %d\n", GDCSD->NumLeakyBuckets);
    fprintf (outFile, "          BitRateExponent %d\n", GDCSD->BitRateExponent);
    fprintf (outFile, "          BufferSizeExponent %d\n", GDCSD->BufferSizeExponent);
    fprintf (outFile, "          AebrFlag %s\n", (GDCSD->AebrFlag== 0) ? "False" : "True");
    fprintf (outFile, "          ABSHrdBuffer %d\n", GDCSD->ABSHrdBuffer);
    fprintf (outFile, "          MultiresFlag %s\n", (GDCSD->MultiresFlag== 0) ? "False" : "True");
    fprintf (outFile, "          SyncmarkerFlag %s\n", (GDCSD->SyncmarkerFlag== 0) ? "False" : "True");
    fprintf (outFile, "          RangeredFlag %s\n", (GDCSD->RangeredFlag== 0) ? "False" : "True");
    fprintf (outFile, "          Maxbframes %d\n", GDCSD->Maxbframes);


    fflush (outFile);

}
#endif /*VALID_TOOLS*/


/*******************************************************************************/
/*! \brief Check whether an event "job completed" is to be sent to the controller
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void avspar_CheckEventToSend(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t * PARSER_Properties_p;
    AVS_GlobalDecodingContextSpecificData_t *GDCSD;
    U8 /* Type, */ RefFrameIndex;
    U8 SCUnitType;

    /* set to true  if the picture gets sent back to the producer*/
    /* BOOL PictureSentFlag = FALSE; */

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = (AVSParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

    /* An event "job completed" is raised whenever a new picture has been parsed.
     * (and in case a new picture is starting, the parser shall not parse anything
     *  not to modify the parameters from the previously parsed picture)
     *
     */

    /* Check whether the current start code is that of a VOP */
    SCUnitType = GetUnitType(PARSER_Data_p->StartCode.Value);
    if (SCUnitType == I_PICTURE_START_CODE || SCUnitType == PB_PICTURE_START_CODE) /* introduced a fake marker */
    {
        if ((PARSER_Data_p->ParserState == PARSER_STATE_PARSING) &&
            (PARSER_Data_p->PictureLocalData.IsValidPicture))
        {
            avspar_PictureInfoCompile(ParserHandle);
#if defined (VALID_TOOLS)
            /*avspar_PictureDumpParsingJobResult(ParserHandle);*/
#endif /*VALID_TOOLS*/
#ifdef STVID_TRICKMODE_BACKWARD
            if ((PARSER_Data_p->AllReferencesAvailableForFrame) && (!PARSER_Data_p->PictureLocalData.IsRandomAccessPoint) && (PARSER_Data_p->Backward))
            {
                /* PARSER_Data_p->ParserJobResults.NbNotIInBitBuffer ++; */ /* NbNotIInBitBuffer removed from stvid snap 2008.01.311 */
            }
#endif /*STVID_TRICKMODE_BACKWARD*/

#ifdef STVID_TRICKMODE_BACKWARD
            if (((PARSER_Data_p->AllReferencesAvailableForFrame) &&
               ((PARSER_Data_p->PictureLocalData.IsRandomAccessPoint == PARSER_Data_p->GetPictureParams.GetRandomAccessPoint) ||
               ((PARSER_Data_p->SkipMode == STVID_DECODED_PICTURES_I) && (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) ||
               (!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint))) && (!PARSER_Data_p->Backward))
               ||
               ((PARSER_Data_p->AllReferencesAvailableForFrame) &&
               (PARSER_Data_p->PictureLocalData.IsRandomAccessPoint) && (PARSER_Data_p->Backward) /* && (!PARSER_Data_p->ParserJobResults.BackwardBufferFullyParsed) */)) /* PLE_TO_DO removed BackwardBufferFullyParsed usage because not available any more. Safe until  trickmode backward implementation. */
#else
            if ( (PARSER_Data_p->AllReferencesAvailableForFrame) &&
               ((PARSER_Data_p->PictureLocalData.IsRandomAccessPoint == PARSER_Data_p->GetPictureParams.GetRandomAccessPoint) ||
               (!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint))))
#endif
            {
                PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;

                /* Only update the FullReferenceFrameList for the frame that is not a B */
                if((PARSER_Data_p->PictureGenericData.IsReference) &&
                    (PARSER_Data_p->PictureSpecificData.PictureType != AVS_B_VOP))
                {
                    RefFrameIndex = PARSER_Data_p->StreamState.NextReferencePictureIndex;
                    PARSER_Data_p->PictureGenericData.FullReferenceFrameList[RefFrameIndex] = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;

                    PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[RefFrameIndex] = TRUE;

                    /* Update the index for the next reference frame. It will either be slot 0, 1 or2. */
                    PARSER_Data_p->StreamState.NextReferencePictureIndex = (RefFrameIndex+1)%3;
                }/* end if(PARSER_Data_p->PictureGenericData.IsReference == TRUE) */

                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->ParserJobResults));
                GDCSD= (AVS_GlobalDecodingContextSpecificData_t *)PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
                GDCSD->FirstPictureEver=FALSE;
#ifdef STVID_TRICKMODE_BACKWARD
                if (!PARSER_Data_p->Backward)
#endif /*STVID_TRICKMODE_BACKWARD*/
                {
#ifdef ST_speed
                if ((!PARSER_Data_p->PictureLocalData.IsRandomAccessPoint))
                {
                    /* PARSER_Data_p->ParserJobResults.NbNotIInBitBuffer ++; */ /* NbNotIInBitBuffer removed from stvid snap 2008.01.311 */
                }
#endif /* ST_speed */
                }
            }
            else
            {
                /* WARNING, at this stage only picture's start & stop addresses in the bit buffer are valid data, all remaingin of the picture's data might be unrelevant because picture parsing is not complete */
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress));
                /* TraceBuffer(("Picture skipped\n")); */

                /* Wake up task to look for next start codes */
                STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
            }

#ifdef TRACE_AVS_PICTURE_TYPE
            avspar_OutputDataStructure(ParserHandle);
#endif /*TRACE_AVS_PICTURE_TYPE*/

            /* Increment the Decoding Order Frame ID for each frame */
            PARSER_Data_p->PictureGenericData.DecodingOrderFrameID++;

        }/* end  if ((PARSER_Data_p->ParserState == PARSER_STATE_PARSING) ...*/
    }/* end if ((PARSER_Data_p->StartCode.Value == VOP_START_CODE) ...*/
   else
    {
        /* Wake up task to look for next start codes */
        STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
    }
}/* end avspar_CheckEventToSend() */


/*******************************************************************************/
/*! \brief Get the Start Code commands. Flush the command queue
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void avspar_GetStartCodeCommand(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Properties_t * PARSER_Properties_p;
    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
#endif /*STVID_TRICKMODE_BACKWARD*/
    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* The start code to process is in the StartCodeCommand buffer */

    if ((PARSER_Data_p->StartCode.IsPending) && (PARSER_Data_p->StartCode.IsInBitBuffer))
    {
         /* The SC can now be processed */
         avspar_ProcessStartCode(PARSER_Data_p->StartCode.Value,
                          PARSER_Data_p->StartCode.StartAddress_p,
                          PARSER_Data_p->StartCode.StopAddress_p,
                          ParserHandle);

        avspar_CheckEventToSend(ParserHandle);
        /* The start code has been analyzed: it is not pending anymore */
        PARSER_Data_p->StartCode.IsPending = FALSE;
    }
#ifdef STVID_TRICKMODE_BACKWARD
    else if ((PARSER_Data_p->Backward) && (PARSER_Data_p->StartCode.IsPending) && (!(PARSER_Data_p->StartCode.IsInBitBuffer)) /* && (!PARSER_Data_p->ParserJobResults.BackwardBufferFullyParsed) */) /* PLE_TO_DO removed BackwardBufferFullyParsed usage because not available any more. Safe until  trickmode backward implementation. */
    {
        /* PARSER_Data_p->ParserJobResults.BackwardBufferFullyParsed = TRUE; */ /* PLE_TO_DO removed BackwardBufferFullyParsed usage because not available any more. Safe until  trickmode backward implementation. */

        STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->ParserJobResults));
    }
#endif /* STVID_TRICKMODE_BACKWARD */
    /* Else: if no start code is ready to be processed, simply return */
}

/*******************************************************************************/
/*! \brief Get the controller commands. Flush the command queue
 *
 * It modifies the parser state
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void avspar_GetControllerCommand(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STOS_SemaphoreWait(PARSER_Data_p->ControllerSharedAccess);
    if (PARSER_Data_p->ForTask.ControllerCommand)
    {
        PARSER_Data_p->ParserState = PARSER_STATE_PARSING;
        PARSER_Data_p->ForTask.ControllerCommand = FALSE;
    }
    STOS_SemaphoreSignal(PARSER_Data_p->ControllerSharedAccess);

    /* Force Parser to read next start code, if any */
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);

}

/*******************************************************************************/
/*! \brief Parser Tasks
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void avspar_ParserTaskFunc(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STOS_TaskEnter(NULL);

    /* Big loop executed until ToBeDeleted is set --------------------------- */
    do
    {
        while(!PARSER_Data_p->ParserTask.ToBeDeleted && (PARSER_Data_p->ParserState != PARSER_STATE_PARSING))
        {
            /* Waiting for a command */
            STOS_SemaphoreWait(PARSER_Data_p->ParserOrder);
            avspar_GetControllerCommand(ParserHandle);
        }
        if (!PARSER_Data_p->ParserTask.ToBeDeleted)
        {
            /* The parser has a command to process */
            while(!PARSER_Data_p->ParserTask.ToBeDeleted && (PARSER_Data_p->ParserState == PARSER_STATE_PARSING))
            {
                /* Semaphore has normal time-out to do polling of actions even if semaphore was not signaled. */
                PARSER_Data_p->ForTask.MaxWaitOrderTime = time_plus(time_now(), MAX_WAIT_ORDER_TIME);
                STOS_SemaphoreWaitTimeOut(PARSER_Data_p->ParserSC, &(PARSER_Data_p->ForTask.MaxWaitOrderTime));

                /* Flush the command queue, as the parser is able to process one command at a time */
                while (STOS_SemaphoreWaitTimeOut(PARSER_Data_p->ParserSC, TIMEOUT_IMMEDIATE) == 0)
                {
                    /* Nothing to do: just to decrement counter until 0, to avoid multiple looping on semaphore wait */
                }

                if (!PARSER_Data_p->ParserTask.ToBeDeleted)
                {
#ifdef STVID_TRICKMODE_BACKWARD
                    if (((PARSER_Data_p->Backward) && (!PARSER_Data_p->BackwardStop) && (!PARSER_Data_p->StopParsing)) ||
                         (!PARSER_Data_p->Backward))
#endif /* STVID_TRICKMODE_BACKWARD */
                    {
                        avspar_IsThereStartCodeToProcess(ParserHandle);
                        /* Then, get Start code commands */
                        avspar_GetStartCodeCommand(ParserHandle);
                    }
                }
            }
        }
    } while (!(PARSER_Data_p->ParserTask.ToBeDeleted));

    STOS_TaskExit(NULL);

} /* End of ParserTaskFunc() function */

/******************************************************************************/
/*! \brief Inform the parser about the decoder read address
 *
 * So that the parser maintains the bit buffer level
 *
 * \param ParserHandle
 * \param InformReadAddressParams_p the parameters for the function
 * \return ST_Error_Code
 */
/******************************************************************************/
static ST_ErrorCode_t avspar_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Stores the decoder read pointer locally for further computation of the bit buffer level */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = InformReadAddressParams_p->DecoderReadAddress_p;

    /* Inform the FDMA on the read pointer of the decoder in Bit Buffer */
    VIDINJ_TransferSetESRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, (void*) (((U32)InformReadAddressParams_p->DecoderReadAddress_p)&0xffffff80));

    return (ST_NO_ERROR);
}

/******************************************************************************/
/*! \brief Get the bit buffer level
 *
 * The bit buffer level is maintained by the parser
 *
 * \param ParserHandle
 * \param InformReadAddressParams_p the parameters for the function
 * \return ST_Error_Code
 */
/******************************************************************************/
static ST_ErrorCode_t avspar_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Access data shared with the injection task */
    STOS_SemaphoreWait(PARSER_Data_p->InjectSharedAccess);
#ifdef STVID_TRICKMODE_BACKWARD
    if (!PARSER_Data_p->Backward)
#endif /* STVID_TRICKMODE_BACKWARD */
    {
        if (PARSER_Data_p->BitBuffer.ES_Write_p < PARSER_Data_p->BitBuffer.ES_DecoderRead_p)
        {
        /* Write pointer has done a loop, not the Read pointer */
            GetBitBufferLevelParams_p-> BitBufferLevelInBytes = PARSER_Data_p->BitBuffer.ES_Stop_p
                                                              - PARSER_Data_p->BitBuffer.ES_DecoderRead_p
                                                              + PARSER_Data_p->BitBuffer.ES_Write_p
                                                              - PARSER_Data_p->BitBuffer.ES_Start_p
                                                              + 1;
        }
        else
        {
            /* Normal: start <= read <= write <= top */
            GetBitBufferLevelParams_p-> BitBufferLevelInBytes = PARSER_Data_p->BitBuffer.ES_Write_p
                                                          - PARSER_Data_p->BitBuffer.ES_DecoderRead_p;
        }
    }
#ifdef STVID_TRICKMODE_BACKWARD
    else
    {
        /* May be add vidinj_transferflush: done for mpeg2 */
        GetBitBufferLevelParams_p->BitBufferLevelInBytes = PARSER_Data_p->BitBuffer.ES_Write_p - PARSER_Data_p->BitBuffer.ES_Start_p + 1;
    }
#endif /* STVID_TRICKMODE_BACKWARD */

    /* Release access shared with the injection task */
    STOS_SemaphoreSignal(PARSER_Data_p->InjectSharedAccess);

    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : avspar_GetDataInputBufferParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t    avspar_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle,
                                                void ** const BaseAddress_p,
                                                U32 * const Size_p)
{
    AVSParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    *BaseAddress_p    = STAVMEM_DeviceToVirtual(PARSER_Data_p->PESBuffer.PESBufferBase_p, &VirtualMapping);
    *Size_p           = (U32)PARSER_Data_p->PESBuffer.PESBufferTop_p
                      - (U32)PARSER_Data_p->PESBuffer.PESBufferBase_p
                      + 1;

    return(ST_NO_ERROR);
}

static ST_ErrorCode_t    avspar_SetDataInputInterface(const PARSER_Handle_t ParserHandle,
                                ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
                                void (*InformReadAddress)(void * const Handle, void * const Address),
                                void * const Handle)

{
    ST_ErrorCode_t Err = ST_NO_ERROR;
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while setting a new data input interface */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    Err = VIDINJ_Transfer_SetDataInputInterface(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum,
                                    GetWriteAddress,InformReadAddress,Handle);

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    return(Err);

}

/*******************************************************************************
Name        : avspar_OutputDataStructure
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void avspar_OutputDataStructure(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

/*
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ColourPrimaries             : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "TransferCharacteristics     : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "MatrixCoefficients          : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "LeftOffset                  : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.LeftOffset));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "RightOffset                 : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.RightOffset));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "TopOffset                   : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.TopOffset));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BottomOffset                : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.BottomOffset));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "NumberOfReferenceFrames     : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.NumberOfReferenceFrames));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "MaxDecFrameBuffering        : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.MaxDecFrameBuffering));

   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Height                      : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Width                       : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Aspect                      : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ScanType                    : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ScanType));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FrameRate                   : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BitRate                     : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "MPEGStandard                : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsLowDelay                  : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.IsLowDelay));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VBVBufferSize               : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VBVBufferSize));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "StreamID                    : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.StreamID));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ProfileAndLevelIndication   : 0x%x\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VideoFormat                 : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VideoFormat));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FrameRateExtensionN         : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRateExtensionN));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FrameRateExtensionD         : %d\n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRateExtensionD));

   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "RepeatFirstField            : %d\n", PARSER_Data_p->PictureGenericData.RepeatFirstField));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "RepeatProgressiveCounter    : %d\n", PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsPTSValid                  : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "NumberOfPanAndScan          : %d\n", PARSER_Data_p->PictureGenericData.NumberOfPanAndScan));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DecodingOrderFrameID        : %d\n", PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ExPresentationOrderID       : %d\n", PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ExPresentationOrderIDExt    : %d\n", PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsDisplayBoundPictureIDValid: %d\n", PARSER_Data_p->PictureGenericData.IsDisplayBoundPictureIDValid));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DisplayBoundPictureID       : %d\n", PARSER_Data_p->PictureGenericData.DisplayBoundPictureID.ID));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DisplayBoundPictureIDEx     : %d\n", PARSER_Data_p->PictureGenericData.DisplayBoundPictureID.IDExtension));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsFirstOfTwoFIelds          : %d\n", PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields));
*/
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsReference                 : %d\n", PARSER_Data_p->PictureGenericData.IsReference));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FullReferenceFrameList0     : %d\n", PARSER_Data_p->PictureGenericData.FullReferenceFrameList[0]));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FullReferenceFrameList1     : %d\n", PARSER_Data_p->PictureGenericData.FullReferenceFrameList[1]));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsValidIndexInRefFrame0     : %d\n", PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[0]));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsValidIndexInRefFrame1     : %d\n", PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[1]));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsBrokenLinkIndexInRefFrame0: %d\n", PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[0]));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsBrokenLinkIndexInRefFrame1: %d\n", PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[1]));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "UsedSizeInReferenceListP0   : %d\n", PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ReferenceListP0             : %d\n", PARSER_Data_p->PictureGenericData.ReferenceListP0[0]));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "UsedSizeInReferenceListB0   : %d\n", PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ReferenceListB0             : %d\n", PARSER_Data_p->PictureGenericData.ReferenceListB0[0]));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "UsedSizeInReferenceListB1   : %d\n", PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ReferenceListB1             : %d\n", PARSER_Data_p->PictureGenericData.ReferenceListB1[0]));

   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FrameRate                   : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "MPEGFrame                   : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame));

#ifdef TRACE_AVS_PICTURE_TYPE
    TracePictureType(("ParserType","%c",
       (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I ? 'I' :
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P ? 'P' :
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B ? 'B' : '?')
        ));
#endif /** TRACE_AVS_PICTURE_TYPE **/
/*
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BitBufferPictureStartAddress: 0x%x\n", PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BitBufferPictureStopAddress : 0x%x\n", PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsDecodeStartTimeValid      : %d\n", PARSER_Data_p->PictureGenericData.IsDecodeStartTimeValid));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "IsPresentationStartTimeValid: %d\n", PARSER_Data_p->PictureGenericData.IsPresentationStartTimeValid));

   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SacnType                    : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PictureStructure            : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "TopFieldFirst               : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PTS                         : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PTS33                       : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PTSInterpolated             : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "CompressionLevel            : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.CompressionLevel));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DecimationFactors           : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.DecimationFactors));

   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ColorType                   : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorType));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BitmapType                  : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BitmapType));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PreMultipliedColor          : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.PreMultipliedColor));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ColorSpaceConversion        : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AspectRatio                 : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Width                       : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Height                      : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Pitch                       : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Pitch));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Offset                      : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Offset));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SubByteFormat               : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.SubByteFormat));
   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BigNotLittle                : %d\n", PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BigNotLittle));
*/
}

#ifdef VALID_TOOLS
/*******************************************************************************
Name        : AVSPARSER_RegisterTrace
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t AVSPARSER_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef TRACE_UART
    TRACE_ConditionalRegister(AVSPAR_TRACE_BLOCKID, AVSPAR_TRACE_BLOCKNAME, avs_TraceItemList, sizeof(avs_TraceItemList)/sizeof(TRACE_Item_t));
#endif /* TRACE_UART */

    return(ErrorCode);
}
#endif /* VALID_TOOLS */

#ifdef ST_speed
/*******************************************************************************
Name        : avspar_GetBitBufferOutputCounter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 avspar_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle)
{                 /* To be implemented. Used to compute bit rate */
    AVSParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32 DiffPSC;
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, &VirtualMapping);

    if ((U32) (PARSER_Data_p->LastSCAdd_p) < (U32)GetSCAdd(CPUNextSCEntry_p))
    {
        DiffPSC = (U32)((U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p));
    }
    else
    {
        /* ES buffer (bit buffer) looped. */
        DiffPSC = ((U32)(PARSER_Data_p->BitBuffer.ES_Stop_p)
                  - (U32)(PARSER_Data_p->LastSCAdd_p)
                  + (U32)(GetSCAdd(CPUNextSCEntry_p))
                  - (U32)(PARSER_Data_p->BitBuffer.ES_Start_p) + 1);
    }
    PARSER_Data_p->OutputCounter += DiffPSC;
    return(PARSER_Data_p->OutputCounter);

} /*End avspar_GetBitBufferOutputCounter*/
#endif /*ST_speed*/


/*******************************************************************************
Name        : avspar_Setup
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t avspar_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while setting the FDMA node partition */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    ErrorCode = VIDINJ_ReallocateFDMANodes(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, SetupParams_p);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    return(ErrorCode);
} /* End of avspar_Setup() function */
/*******************************************************************************
Name        : avs2par_FlushInjection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void avspar_FlushInjection(const PARSER_Handle_t ParserHandle)
{
    UNUSED_PARAMETER(ParserHandle);
   /* To be implemented */
} /* avspar_FlushInjection */


#ifdef ST_speed
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : avspar_SetBitBuffer
Description : Set the location of the bit buffer
Parameters  : HAL Parser manager handle, buffer address, buffer size in bytes
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void avspar_SetBitBuffer(const PARSER_Handle_t        ParserHandle,
                                 void * const                 BufferAddressStart_p,
                                 const U32                    BufferSize,
                                 const BitBufferType_t        BufferType,
                                 const BOOL                   Stop  )
{
    U8                      Pattern[]={0xAF, 0xAF, 0x00, 0x00, 0x01, 0xB8};
    U8                      Size = 6;
    AVSParserPrivateData_t * PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while setting a new decode buffer */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    switch (BufferType)
    {
        case BIT_BUFFER_LINEAR :
            PARSER_Data_p->Backward = TRUE;
            /* PARSER_Data_p->ParserJobResults.BackwardBufferFullyParsed = FALSE;  *//* PLE_TO_DO removed BackwardBufferFullyParsed usage because not available any more. Safe until  trickmode backward implementation. */
            if (Stop)
            {
            PARSER_Data_p->BitBuffer.ES_Start_p         = BufferAddressStart_p;
            PARSER_Data_p->BitBuffer.ES_Stop_p          = (void *)((U32)BufferAddressStart_p + BufferSize -1);

                PARSER_Data_p->StopParsing = TRUE;
                PARSER_Data_p->BackwardCheckWrite = TRUE;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "parser stop "));
            }
            else
            {
                PARSER_Data_p->BackwardStop = FALSE;
                PARSER_Data_p->StopParsing = FALSE;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "parser start "));
                /* PARSER_Data_p->ParserJobResults.BackwardBufferFullyParsed = FALSE; */ /* PLE_TO_DO removed BackwardBufferFullyParsed usage because not available any more. Safe until  trickmode backward implementation. */
                /* PARSER_Data_p->ParserJobResults.NbNotIInBitBuffer = 0; */ /* NbNotIInBitBuffer removed from stvid snap 2008.01.311 */
                PARSER_Data_p->SCBuffer.NextSCEntry         = PARSER_Data_p->SCBuffer.SCList_Start_p;
            }
            break;

        case BIT_BUFFER_CIRCULAR:
            PARSER_Data_p->BitBuffer.ES_Start_p         = PARSER_Data_p->BitBuffer.ES_StoreStart_p;
            PARSER_Data_p->BitBuffer.ES_Stop_p          = PARSER_Data_p->BitBuffer.ES_StoreStop_p;
            PARSER_Data_p->Backward = FALSE;
            PARSER_Data_p->StopParsing = FALSE;
            /* PARSER_Data_p->ParserJobResults.NbNotIInBitBuffer = 0; */ /* NbNotIInBitBuffer removed from stvid snap 2008.01.311 */
            break;

        default:
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "bad type of bit buffer "));
            break;
    }

    PARSER_Data_p->BitBuffer.ES_DecoderRead_p   = PARSER_Data_p->BitBuffer.ES_Start_p;

    if ((PARSER_Data_p->Backward) && (Stop))
    {
          VIDINJ_TransferLimits(PARSER_Data_p->Inject.InjecterHandle,
                            PARSER_Data_p->Inject.InjectNum,
         /* BB */           PARSER_Data_p->BitBuffer.ES_Start_p,
                            PARSER_Data_p->BitBuffer.ES_Stop_p,
         /* SC */           PARSER_Data_p->SCBuffer.SCList_Start_p,
                            PARSER_Data_p->SCBuffer.SCList_Stop_p,
        /* PES */           PARSER_Data_p->PESBuffer.PESBufferBase_p,
                            PARSER_Data_p->PESBuffer.PESBufferTop_p );
        /* Inform the FDMA on the read pointer of the decoder in Bit Buffer */
        VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, TRUE, Pattern, Size);

        VIDINJ_TransferSetESRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, (void*) (((U32)PARSER_Data_p->BitBuffer.ES_DecoderRead_p)&0xffffff80));
    }

    STTBX_Print(("\n SET ES buffer Base:%lx - Top:%lx\nSC buffer Base:%lx - Top:%lx\nPES buffer Base:%lx - Top:%lx\n",
            /* BB */        (unsigned long)(PARSER_Data_p->BitBuffer.ES_Start_p),
                            (unsigned long)(PARSER_Data_p->BitBuffer.ES_Stop_p),
            /* SC */        (unsigned long)(PARSER_Data_p->SCBuffer.SCList_Start_p),
                            (unsigned long)(PARSER_Data_p->SCBuffer.SCList_Stop_p),
            /* PES */       (unsigned long)(PARSER_Data_p->PESBuffer.PESBufferBase_p),
                            (unsigned long)(PARSER_Data_p->PESBuffer.PESBufferTop_p)));
    /* Release FDMA transfers */
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

} /* End of avspar_SetBitBuffer() function */

/*******************************************************************************
Name        : avspar_WriteStartCode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void avspar_WriteStartCode(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p)
{
    const U8*  Temp8;
    AVSParserPrivateData_t * PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Write the startcode in the bitbuffer */
    Temp8 = (const U8*) SCAdd_p;
    STSYS_WriteRegMemUncached8(Temp8    , 0x0);
    STSYS_WriteRegMemUncached8(Temp8 + 1, 0x0);
    STSYS_WriteRegMemUncached8(Temp8 + 2, 0x1);
    STSYS_WriteRegMemUncached8(Temp8 + 3, SCVal);

    /* Ensure no FDMA transfer is done while injecting the startcode in the startcode list */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    /* Add the startcode in the startcode list */
    VIDINJ_InjectStartCode(PARSER_Data_p->Inject.InjecterHandle,
                           PARSER_Data_p->Inject.InjectNum,
                           SCVal,
                           SCAdd_p);
    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

} /* End of avspar_WriteStartCode */
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed*/


/*******************************************************************************
Name        : avspar_BitBufferInjectionInit
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t avspar_BitBufferInjectionInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t * const BitBufferInjectionParams_p)
{
  UNUSED_PARAMETER(ParserHandle);
  UNUSED_PARAMETER(BitBufferInjectionParams_p);

  return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : SavePTSEntry
Description : Stores a new parsed PTSEntry in the PTS StartCode List
Parameters  : Parser handle, PTS Entry Pointer in FDMA's StartCode list
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void avspar_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* CPUPTSEntry_p)
{
    AVSParserPrivateData_t * PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PTS_t* PTSLIST_WRITE_PLUS_ONE;

    if ((U32)GetSCAdd(CPUPTSEntry_p) != (U32)PARSER_Data_p->LastPTSStored.Address_p)
    {
        PTSLIST_WRITE_PLUS_ONE = PTSLIST_WRITE + 1;
        if(PTSLIST_WRITE_PLUS_ONE > PTSLIST_TOP)
        {
            PTSLIST_WRITE_PLUS_ONE = PTSLIST_BASE;
        }
        /* Check if we can find a place to store the new PTS Entry, otherwise we
         * overwrite the oldest one */
        if (PTSLIST_WRITE_PLUS_ONE == PTSLIST_READ)
        {
            /* First free the oldest one */
            /* Keep always an empty SCEntry because Read_p == Write_p means that the SCList is empty */
            PTSLIST_READ++;
            if (PTSLIST_READ >= PTSLIST_TOP)
            {
                PTSLIST_READ = PTSLIST_BASE;
            }
            /* Next overwrite it */
        }
        /* Store the PTS */
        PTSLIST_WRITE->PTS33     = GetPTS33(CPUPTSEntry_p);
        PTSLIST_WRITE->PTS32     = GetPTS32(CPUPTSEntry_p);
        PTSLIST_WRITE->Address_p = (void*)GetSCAdd(CPUPTSEntry_p);
        PTSLIST_WRITE++;

        if (PTSLIST_WRITE >= PTSLIST_TOP)
        {
            PTSLIST_WRITE = PTSLIST_BASE;
        }
        PARSER_Data_p->LastPTSStored.PTS33      = GetPTS33(CPUPTSEntry_p);
        PARSER_Data_p->LastPTSStored.PTS32      = GetPTS32(CPUPTSEntry_p);
        PARSER_Data_p->LastPTSStored.Address_p  = (void*)GetSCAdd(CPUPTSEntry_p);
    }
} /* End of avspar_SavePTSEntry() function */
/* End of avsparser.c */

