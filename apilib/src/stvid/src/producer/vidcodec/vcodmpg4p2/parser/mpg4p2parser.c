/*!
 *******************************************************************************
 * \file mpg4p2parser.c
 *
 * \brief MPEG4P2 Parser top level CODEC API functions
 *
 * \author
 * Sibnath Ghosh \n
 *  \n
 * CMG/STB \n
 * Copyright (C) STMicroelectronics 2007
 *
 *******************************************************************************
 */


/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

/* MPEG4P2 Parser specific */
#include "mpg4p2parser.h"

/* Functions ---------------------------------------------------------------- */
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t mpeg4p2par_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p);
#endif
#ifdef STVID_DEBUG_GET_STATUS
static ST_ErrorCode_t mpeg4p2par_GetStatus(const PARSER_Handle_t ParserHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
static ST_ErrorCode_t mpeg4p2par_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p);
static ST_ErrorCode_t mpeg4p2par_Stop(const PARSER_Handle_t ParserHandle);
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t mpeg4p2par_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p);
#endif

static ST_ErrorCode_t mpeg4p2par_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t mpeg4p2par_AllocatePESBuffer(const PARSER_Handle_t ParserHandle, const U32 PESBufferSize, const PARSER_InitParams_t * const InitParams_p);
static ST_ErrorCode_t mpeg4p2par_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t mpeg4p2par_AllocateSCBuffer(const PARSER_Handle_t ParserHandle, const U32 SC_ListSize, const PARSER_InitParams_t * const InitParams_p);

static ST_ErrorCode_t mpeg4p2par_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p);
static ST_ErrorCode_t mpeg4p2par_GetPicture(const PARSER_Handle_t ParserHandle, const PARSER_GetPictureParams_t * const GetPictureParams_p);
static ST_ErrorCode_t mpeg4p2par_Term(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t mpeg4p2par_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p);
static ST_ErrorCode_t mpeg4p2par_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p);
static ST_ErrorCode_t mpeg4p2par_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle, void ** const BaseAddress_p, U32 * const Size_p);
static ST_ErrorCode_t mpeg4p2par_SetDataInputInterface(const PARSER_Handle_t ParserHandle, ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
                                                   void (*InformReadAddress)(void * const Handle, void * const Address), void * const Handle);
void mpeg4p2par_OutputDataStructure(const PARSER_Handle_t ParserHandle);
ST_ErrorCode_t mpeg4p2par_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p);
#ifdef ST_speed
U32             mpeg4p2par_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle);
#endif /*ST_speed */
static void mpeg4p2par_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* CPUPTSEntry_p);

ST_ErrorCode_t mpeg4p2par_BitBufferInjectionInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t * const BitBufferInjectionParams_p);
#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t mpeg4p2par_SecureInit(const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t *   const SecureInitParams_p);
#endif /* DVD_SECURED_CHIP */
static int mpeg4p2par_CheckSCSignature(register unsigned char *pPayload,unsigned char SCValue, const PARSER_Handle_t  ParserHandle);
/* Macro ---------------------------------------------------------------- */

/* PTS List access shortcuts */
#define PTSLIST_BASE     (PARSER_Data_p->PTS_SCList.SCList)
#define PTSLIST_READ     (PARSER_Data_p->PTS_SCList.Read_p)
#define PTSLIST_WRITE    (PARSER_Data_p->PTS_SCList.Write_p)
#define PTSLIST_TOP      (&PARSER_Data_p->PTS_SCList.SCList[PTS_SCLIST_SIZE])

#ifdef STVID_DEBUG_PARSER
#define STTBX_REPORT
#endif

/*#define TRACE_MPEG4P2_PICTURE_TYPE*/
#ifdef TRACE_MPEG4P2_PICTURE_TYPE
#define TracePictureType(x) TraceMessage x
#else
#define TracePictureType(x)
#endif /* TRACE_MPEG4P2_PICTURE_TYPE */

/* Constants ---------------------------------------------------------------- */
const PARSER_FunctionsTable_t PARSER_MPEG4P2Functions =
{
#ifdef STVID_DEBUG_GET_STATISTICS
    mpeg4p2par_ResetStatistics,
    mpeg4p2par_GetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    mpeg4p2par_GetStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    mpeg4p2par_Init,
    mpeg4p2par_Stop,
    mpeg4p2par_Start,
    mpeg4p2par_GetPicture,
    mpeg4p2par_Term,
	mpeg4p2par_InformReadAddress,
	mpeg4p2par_GetBitBufferLevel,
	mpeg4p2par_GetDataInputBufferParams,
	mpeg4p2par_SetDataInputInterface,
    mpeg4p2par_Setup,
    mpeg4p2par_FlushInjection
#ifdef ST_speed
    ,mpeg4p2par_GetBitBufferOutputCounter
#ifdef STVID_TRICKMODE_BACKWARD
    ,mpeg4p2par_SetBitBuffer,
    mpeg4p2par_WriteStartCode
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed*/
    ,mpeg4p2par_BitBufferInjectionInit
#if defined(DVD_SECURED_CHIP)
    ,mpeg4p2par_SecureInit
#endif  /* DVD_SECURED_CHIP */
};

#if defined(TRACE_UART) && defined(VALID_TOOLS)
static TRACE_Item_t mpeg4p2par_TraceItemList[] =
{
    {"GLOBAL",						TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"INIT_TERM",					TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PARSESEQ",					TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PARSEVO",					TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PARSEVOL",					TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PARSEVOP",					TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PARSEGOV",					TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PARSESVH",					TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PARSEVST",					TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PARSEVPH",					TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PICTURE_PARAMS",			TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE},
    {"PICTURE_INFO_COMPILE",	TRACE_MPEG4P2PAR_COLOR, TRUE, FALSE}
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS)*/

/*******************************************************************************/
/*! \brief De-Allocate the PES Buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t mpeg4p2par_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t  * PARSER_Data_p;
    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    STAVMEM_FreeBlockParams_t     FreeParams;
    ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

    PARSER_Data_p = (MPEG4P2ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->PESBuffer.PESBufferHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The PESBuffer is not deallocated correctly !"));
    }

	return (ErrorCode);
} /* end of mpeg4p2par_DeAllocatePESBuffer */

/*******************************************************************************/
/*! \brief Allocate the PES Buffer
 *
 * \param ParserHandle Parser Handle
 * \param PESBufferSize PES buffer size in bytes
 * \param InitParams_p Init parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t mpeg4p2par_AllocatePESBuffer(const PARSER_Handle_t        ParserHandle,
		                               const U32                    PESBufferSize,
					         	       const PARSER_InitParams_t *  const InitParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
	ST_ErrorCode_t                          ErrorCode;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
        /* ErrorCode = */ mpeg4p2par_DeAllocatePESBuffer(ParserHandle);
        return(ErrorCode);
    }

    PARSER_Data_p->PESBuffer.PESBufferBase_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
    PARSER_Data_p->PESBuffer.PESBufferTop_p = (void *) ((U32)PARSER_Data_p->PESBuffer.PESBufferBase_p + PESBufferSize - 1);

	return (ErrorCode);
} /* end of mpeg4p2par_AllocatePESBuffer */

/*******************************************************************************/
/*! \brief De-Allocate the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t mpeg4p2par_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t  * PARSER_Data_p;
    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    STAVMEM_FreeBlockParams_t     FreeParams;
    ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

    PARSER_Data_p = (MPEG4P2ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->SCBuffer.SCListHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The SCBuffer is not deallocated correctly !"));
    }

    return(ErrorCode);
} /* end of mpeg4p2par_DeAllocateSCBuffer() */


/*******************************************************************************/
/*! \brief Allocate the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \param SC_ListSize Start Code buffer size in bytes
 * \param InitParams_p Init parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t mpeg4p2par_AllocateSCBuffer(const PARSER_Handle_t        ParserHandle,
									   const U32                    SC_ListSize,
									   const PARSER_InitParams_t *  const InitParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
	ST_ErrorCode_t                          ErrorCode;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
        /* ErrorCode = */ mpeg4p2par_DeAllocateSCBuffer(ParserHandle);
        return(ErrorCode);
    }
    PARSER_Data_p->SCBuffer.SCList_Start_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
    PARSER_Data_p->SCBuffer.SCList_Stop_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.SCList_Stop_p += (SC_ListSize-1);

	return (ErrorCode);
} /* end of mpeg4p2par_AllocateSCBuffer */

/*******************************************************************************/
/*! \brief Initialize the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \return void
 */
/*******************************************************************************/
void mpeg4p2par_InitSCBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Set default values for pointers in SC-List */
    PARSER_Data_p->SCBuffer.SCList_Write_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.SCList_Loop_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Start_p;

} /* end of mpeg4p2par_InitSCBuffer() function */

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
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
ST_ErrorCode_t mpeg4p2par_InitInjectionBuffers(const PARSER_Handle_t        ParserHandle,
		                                   const PARSER_InitParams_t *  const InitParams_p,
						            	   const U32                    SC_ListSize,
								           const U32 				    PESBufferSize)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t  * PARSER_Data_p;
    VIDINJ_GetInjectParams_t      Params;
	ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Allocate SC Buffer */
    ErrorCode = mpeg4p2par_AllocateSCBuffer(ParserHandle, SC_ListSize, InitParams_p);
	if (ErrorCode != ST_NO_ERROR)
	{
		return (ErrorCode);
	}

	/* Allocate PES Buffer */
    ErrorCode = mpeg4p2par_AllocatePESBuffer(ParserHandle, PESBufferSize, InitParams_p);
	if (ErrorCode != ST_NO_ERROR)
	{
        /* ErrorCode = */ mpeg4p2par_DeAllocateSCBuffer(ParserHandle);
		return (ErrorCode);
	}

	/* Set default values for pointers in ES Buffer */
    PARSER_Data_p->BitBuffer.ES_Start_p = InitParams_p->BitBufferAddress_p;

    /* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
    PARSER_Data_p->BitBuffer.ES_Stop_p = (void *)((U32)InitParams_p->BitBufferAddress_p + InitParams_p->BitBufferSize -1);

	/* Initialize the SC buffer pointers */
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Data_p->BitBuffer.ES_StoreStart_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    PARSER_Data_p->BitBuffer.ES_StoreStop_p = PARSER_Data_p->BitBuffer.ES_Stop_p;
#endif  /* STVID_TRICKMODE_BACKWARD */
	mpeg4p2par_InitSCBuffer(ParserHandle);

	/* Reset ES bit buffer write pointer */
	PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

	/* Synchronize buffer definitions with FDMA */
	/* link this init with a fdma channel */
    /*------------------------------------*/
    Params.TransferDoneFct                 = mpeg4p2par_DMATransferDoneFct;
    Params.UserIdent                       = (U32)ParserHandle;
    Params.AvmemPartition                  = InitParams_p->AvmemPartitionHandle;
    Params.CPUPartition                    = InitParams_p->CPUPartition_p;
    PARSER_Data_p->Inject.InjectNum        = VIDINJ_TransferGetInject(PARSER_Data_p->Inject.InjecterHandle, &Params);

    if (PARSER_Data_p->Inject.InjectNum == BAD_INJECT_NUM)
    {
        /* ErrorCode = */ mpeg4p2par_DeAllocatePESBuffer(ParserHandle);
        /* ErrorCode = */ mpeg4p2par_DeAllocateSCBuffer(ParserHandle);
        return(ST_ERROR_NO_MEMORY);
    }

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
static ST_ErrorCode_t mpeg4p2par_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t  * PARSER_Data_p;
	PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    U32                           UserDataSize;
	ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

	if ((ParserHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    PARSER_Properties_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;

	/* Allocate PARSER PrivateData structure */
	PARSER_Properties_p->PrivateData_p = (MPEG4P2ParserPrivateData_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(MPEG4P2ParserPrivateData_t));
	if (PARSER_Properties_p->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    memset (PARSER_Properties_p->PrivateData_p, 0xA5, sizeof(MPEG4P2ParserPrivateData_t)); /* fill in data with 0xA5 by default */

	/* Use the short cut PARSER_Data_p for next lines of code */
	PARSER_Data_p = (MPEG4P2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

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
	ErrorCode = mpeg4p2par_InitInjectionBuffers(ParserHandle, InitParams_p, SC_LIST_SIZE_IN_ENTRY, PES_BUFFER_SIZE);
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
    PARSER_Data_p->DiscontinuityDetected = FALSE;
#ifdef ST_speed
    /*Initialise counter for bit rate*/
    PARSER_Data_p->OutputCounter = 0;
    PARSER_Data_p->SkipMode = STVID_DECODED_PICTURES_ALL;
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Data_p->Backward = FALSE;
    PARSER_Data_p->IsBitBufferFullyParsed = FALSE;
	PARSER_Data_p->StopParsing = FALSE;
#endif
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
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module MPEG4P2 parser init: could not register events !"));
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
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module MPEG4P2 parser init: could not register events !"));
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

#ifdef STVID_TRICKMODE_BACKWARD
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_BITBUFFER_FULLY_PARSED_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module MPEG4P2 parser init: could not register events !"));
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }
#endif

    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_FIND_DISCONTINUITY_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_FIND_DISCONTINUITY_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module MPEG4P2 parser init: could not register events !"));
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }


    /* OB_TODO : Do we need to unregister on STVID_Term */

	/* Semaphore to share variables between parser and injection tasks */
	PARSER_Data_p->InjectSharedAccess = STOS_SemaphoreCreateFifo(NULL, 1);

	/* Semaphore between parser and controller tasks */
	/* Parser main task does not run until it gets a start command from the controller */
	PARSER_Data_p->ParserOrder = STOS_SemaphoreCreateFifo(NULL, 0);
	PARSER_Data_p->ParserSC    = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);

	/* Semaphore to share variables between parser and controller tasks */
	PARSER_Data_p->ControllerSharedAccess = STOS_SemaphoreCreateFifo(NULL, 1);

	PARSER_Data_p->ParserState = PARSER_STATE_IDLE;

    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
	PARSER_Data_p->NonVOPStartCodeFound = FALSE;
	PARSER_Data_p->UserDataAllowed = FALSE;

	/* Create parser task */
	PARSER_Data_p->ParserTask.IsRunning = FALSE;
	ErrorCode = mpeg4p2par_StartParserTask(ParserHandle);
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
    PARSER_Data_p->Is311SignatureUsed = FALSE;

	return (ErrorCode);

} /* End of PARSER_Init function */

/*******************************************************************************/
/*! \brief Set the Stream Type (PES or ES) for the PES parser
 * \param ParserHandle Parser handle,
 * \param StreamType ES or PES
 * \return void
 */
/*******************************************************************************/
void mpeg4p2par_SetStreamType(const PARSER_Handle_t ParserHandle, const STVID_StreamType_t StreamType)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
void mpeg4p2par_SetPESRange(const PARSER_Handle_t ParserHandle,
                         const PARSER_StartParams_t * const StartParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* FDMA setup ----------------  */
	mpeg4p2par_SetStreamType(ParserHandle, StartParams_p->StreamType);

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
void mpeg4p2par_InitParserContext(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	U32 i; /* loop index */

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

	PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID				= 0;
	PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension	= 0;
	PARSER_Data_p->StreamState.CurrentTfcntr													= 0;
	PARSER_Data_p->StreamState.BackwardRefTfcntr												= 0;
	PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID					= 0x80000000;	/* minimum value -2147483648 */
	PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension		= 0;
	PARSER_Data_p->StreamState.WaitingRefPic													= FALSE;
	PARSER_Data_p->StreamState.RespicBBackwardRefPicture									= 0;
	PARSER_Data_p->StreamState.RespicBForwardRefPicture									= 0;
	PARSER_Data_p->StreamState.ForwardRangeredfrmFlagBFrames								= 0;
	PARSER_Data_p->StreamState.ForwardRangeredfrmFlagPFrames								= 0;
	/* The first reference picture will be placed at index zero in the FullReferenceFrameList. */
	PARSER_Data_p->StreamState.NextReferencePictureIndex									= 0;
	PARSER_Data_p->StreamState.RefDistPreviousRefPic										= 0;

	/* Time code set to 0 */

	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate				= MPEG4P2_DEFAULT_FRAME_RATE;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame				= STVID_MPEG_FRAME_I;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure	= STVID_PICTURE_STRUCTURE_FRAME;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst		= FALSE;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.CompressionLevel	= STVID_COMPRESSION_LEVEL_NONE;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.DecimationFactors	= STVID_DECIMATION_FACTOR_NONE;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Seconds	= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Minutes	= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Hours		= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Frames		= 0;

	PARSER_Data_p->PictureGenericData.DecodingOrderFrameID								= 0;
	PARSER_Data_p->PictureGenericData.IsReference											= FALSE;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0							= 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0							= 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1							= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType				= STGXOBJ_PROGRESSIVE_SCAN;
	PARSER_Data_p->PictureGenericData.AsynchronousCommands.FlushPresentationQueue	= FALSE;
	PARSER_Data_p->PictureGenericData.AsynchronousCommands.FreezePresentationOnThisFrame	= FALSE;
	PARSER_Data_p->PictureGenericData.AsynchronousCommands.ResumePresentationOnThisFrame	= FALSE;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE;
	PARSER_Data_p->PictureGenericData.RepeatFirstField										= FALSE;
	PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter							= 0;
	PARSER_Data_p->PictureGenericData.NumberOfPanAndScan									= 0;
	PARSER_Data_p->PictureGenericData.IsDisplayBoundPictureIDValid						= FALSE;
	PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields									= FALSE;
	PARSER_Data_p->PictureGenericData.IsDecodeStartTimeValid								= FALSE;
	PARSER_Data_p->PictureGenericData.IsPresentationStartTimeValid						= FALSE;
	PARSER_Data_p->PictureGenericData.ParsingError											= VIDCOM_ERROR_LEVEL_NONE;
	PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID			= 0;
	PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension	= 0;
	PARSER_Data_p->PictureGenericData.DisplayBoundPictureID.ID							= 0;
	PARSER_Data_p->PictureGenericData.DisplayBoundPictureID.IDExtension				= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS				= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33				= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated	= 0;
	PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid	= FALSE;

	for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; i++)
	{
		PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i]				= FALSE;
		PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i]		= FALSE;
		PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[i]						= TRUE;
		PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[i]						= TRUE;
		PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[i]						= TRUE;
	}

	PARSER_Data_p->PictureSpecificData.RndCtrl					= 0;
	PARSER_Data_p->PictureSpecificData.BackwardRefDist			= 0;
	PARSER_Data_p->PictureSpecificData.PictureType				= MPEG4P2_I_VOP;
	PARSER_Data_p->PictureSpecificData.ForwardRangeredfrmFlag= FALSE;
	PARSER_Data_p->PictureSpecificData.HalfWidthFlag			= FALSE;
	PARSER_Data_p->PictureSpecificData.HalfHeightFlag			= FALSE;
	PARSER_Data_p->PictureSpecificData.FramePictureLayerSize	= 0;
	PARSER_Data_p->PictureSpecificData.TffFlag					= FALSE;
	PARSER_Data_p->PictureSpecificData.RefDist					= 0;
	PARSER_Data_p->PictureSpecificData.CodecVersion				= 500;

	PARSER_Data_p->PictureSpecificData.PictureError.SeqError.ProfileAndLevel = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.VOLError.AspectRatio = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.VOPError.CodedVOP = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.GOVError.ClosedGOV = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.GOVError.BrokenLink = FALSE;

	PARSER_Data_p->GlobalDecodingContextGenericData.ParsingError									= VIDCOM_ERROR_LEVEL_NONE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height							= 0;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width							= 0;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect							= STVID_DISPLAY_ASPECT_RATIO_4TO3;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ScanType						= STVID_SCAN_TYPE_PROGRESSIVE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate						= MPEG4P2_DEFAULT_FRAME_RATE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate							= MPEG4P2_DEFAULT_BIT_RATE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard					= STVID_MPEG_STANDARD_ISO_IEC_14496_2;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.IsLowDelay						= FALSE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VBVBufferSize					= MPEG4P2_DEFAULT_VBVMAX;
	/* NOTE StreamID has already been set earlier */
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication	= MPEG4P2_DEFAULT_PROFILE_AND_LEVEL;
	PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries								= 1;	/* COLOUR_PRIMARIES_ITU_R_BT_709 */;
	PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics						= 1;	/* TRANSFER_ITU_R_BT_709 */;
	PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients							= 1;	/* MATRIX_COEFFICIENTS_ITU_R_BT_709 */;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VideoFormat					= 1;	/* PAL for now else UNSPECIFIED_VIDEO_FORMAT; */
	PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.LeftOffset				= 0;
	PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.RightOffset				= 0;
	PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.TopOffset					= 0;
	PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.BottomOffset				= 0;

	PARSER_Data_p->GlobalDecodingContextGenericData.NumberOfReferenceFrames                        = 2;
	PARSER_Data_p->GlobalDecodingContextGenericData.MaxDecFrameBuffering                        = 2;    /* Number of reference frames (this number +2 gives the number of frame buffers needed for decode */

	/* Unused entries for MPEG4P2 fill them with a default unused value */
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRateExtensionN			= MPEG4P2_DEFAULT_NON_RELEVANT;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRateExtensionD			= MPEG4P2_DEFAULT_NON_RELEVANT;

	PARSER_Data_p->VideoObjectPlane.CurrVOPTime			= 0;		/* The VOP display time of the current reference and non-reference VOP */
	PARSER_Data_p->VideoObjectPlane.PrevVOPTime			= 0;		/* The VOP display time of the previous reference and non-reference VOP */
	PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime		= 0;		/* The VOP display time of the previous reference VOP */
	PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime		= 0;		/* The VOP display time of the previous to previous reference VOP */
	PARSER_Data_p->VideoObjectPlane.VOPTimeDiff			= 1001;	/* The time difference between the current and the previous VOP in display order */
	PARSER_Data_p->VideoObjectPlane.PrevVOPTimeDiff		= 0;		/* previous VOP time difference between two successive VOPs */
	PARSER_Data_p->VideoObjectPlane.PrevVOPWasRef		= FALSE;	/* True if the previous VOP type was a reference VOP */

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
void mpeg4p2par_SetESRange(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->BitBuffer.ES0RangeEnabled   = TRUE;
	PARSER_Data_p->BitBuffer.ES0RangeStart     = SMALLEST_VBS_PACKET_START_CODE;
	PARSER_Data_p->BitBuffer.ES0RangeEnd       = GREATEST_VBS_PACKET_START_CODE;
	PARSER_Data_p->BitBuffer.ES0OneShotEnabled = FALSE;

	/* Range 1 is not used for MPEG4P2 */
	PARSER_Data_p->BitBuffer.ES1RangeEnabled   = FALSE;
	PARSER_Data_p->BitBuffer.ES1RangeStart     = DUMMY_RANGE; /* don't care, as range 1 is not enabled */
	PARSER_Data_p->BitBuffer.ES1RangeEnd       = DUMMY_RANGE; /* don't care, as range 1 is not enabled */
	PARSER_Data_p->BitBuffer.ES1OneShotEnabled = FALSE; /* don't care, as range 1 is not enabled */
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
static ST_ErrorCode_t mpeg4p2par_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p)
{
	ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	VIDINJ_ParserRanges_t       ParserRanges[3];
#ifdef STVID_TRICKMODE_BACKWARD
	/*U32						CurrentEPOID, CurrentDOFID;*/
#endif

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Reset pointers in injection module */
    VIDINJ_TransferReset(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* Save for future use: is inserted "as is" in the GlobalDecodingContextGenericData.SequenceInfo.StreamID structure */
	PARSER_Data_p->StreamState.StreamID = StartParams_p->StreamID;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.StreamID = StartParams_p->StreamID;

    PARSER_Data_p->DiscontinuityDetected = FALSE;
	/* FDMA Setup ----------------- */
	mpeg4p2par_SetPESRange(ParserHandle, StartParams_p);

	mpeg4p2par_SetESRange(ParserHandle);

	/* Specify the transfer buffers and parameters to the FDMA */
	/* cascade the buffers params and sc-detect config to inject sub-module */

	/* MPEG4P2 SC Ranges*/
	ParserRanges[0].RangeId                     = STFDMA_DEVICE_ES_RANGE_0;
	ParserRanges[0].RangeConfig.RangeEnabled    = PARSER_Data_p->BitBuffer.ES0RangeEnabled;
	ParserRanges[0].RangeConfig.RangeStart      = PARSER_Data_p->BitBuffer.ES0RangeStart;
	ParserRanges[0].RangeConfig.RangeEnd        = PARSER_Data_p->BitBuffer.ES0RangeEnd;
	ParserRanges[0].RangeConfig.PTSEnabled      = FALSE;
	ParserRanges[0].RangeConfig.OneShotEnabled  = PARSER_Data_p->BitBuffer.ES0OneShotEnabled;

	/* ES Range 1 is not used in MPEG4P2 */
	ParserRanges[1].RangeId								= STFDMA_DEVICE_ES_RANGE_1;
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

    /* DG TO DO: change prototype for secured platforms (add ESCopyBuffer Base_p and Top_p instead of NULL */

	VIDINJ_TransferLimits(PARSER_Data_p->Inject.InjecterHandle,
						  PARSER_Data_p->Inject.InjectNum,
		/* BB */          PARSER_Data_p->BitBuffer.ES_Start_p,
						  PARSER_Data_p->BitBuffer.ES_Stop_p,
		/* SC */          PARSER_Data_p->SCBuffer.SCList_Start_p,
						  PARSER_Data_p->SCBuffer.SCList_Stop_p,
		/* PES */         PARSER_Data_p->PESBuffer.PESBufferBase_p,
						  PARSER_Data_p->PESBuffer.PESBufferTop_p
#if defined(DVD_SECURED_CHIP)
        /* ES Copy */     ,(void *)NULL, (void *)NULL
#endif /* DVD_SECURED_CHIP */
                           );

	VIDINJ_SetSCRanges(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, 3, ParserRanges);


	/* Run the FDMA */
    VIDINJ_TransferStart(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, StartParams_p->RealTime);

	/* End of FDMA setup ---------- */

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* @@@ need to set initial global and picture context info here. */
	/* Initialize the parser variables */
    if (StartParams_p->ResetParserContext)
    {
        mpeg4p2par_InitParserContext(ParserHandle);
    }

	mpeg4p2par_InitParserVariables(ParserHandle);

	PARSER_Data_p->StreamState.ErrorRecoveryMode = StartParams_p->ErrorRecoveryMode;

	/* @@@ need to set this differently when the initial context is passed down*/
	PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;

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
void mpeg4p2par_DMATransferDoneFct(U32 ParserHandle,void * ES_Write_p,
                                   void * SCListWrite_p,
                                   void * SCListLoop_p
#if defined(DVD_SECURED_CHIP)
                                 , void * ESCopy_Write_p
#endif  /* DVD_SECURED_CHIP */
)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(DVD_SECURED_CHIP)
    /* DG TO DO */
    UNUSED_PARAMETER(ESCopy_Write_p);
#endif  /* DVD_SECURED_CHIP */

    STOS_SemaphoreWait(PARSER_Data_p->InjectSharedAccess);
#ifdef STVID_TRICKMODE_BACKWARD
    if ((!PARSER_Data_p->Backward) || (PARSER_Data_p->StopParsing))
#endif /* STVID_TRICKMODE_BACKWARD */
	{
		PARSER_Data_p->BitBuffer.ES_Write_p     = ES_Write_p;
    	PARSER_Data_p->SCBuffer.SCList_Write_p  = SCListWrite_p;
    	PARSER_Data_p->SCBuffer.SCList_Loop_p   = SCListLoop_p;
	}
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
static ST_ErrorCode_t mpeg4p2par_GetPicture(const PARSER_Handle_t ParserHandle,
										const PARSER_GetPictureParams_t * const GetPictureParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
    U32 CurrentDOFID /*, CurrentEPOID*/;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
    if (PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)
	{
        /* No need to save the ExtendedPresentationOrderPictureID as it is
           totally managed within the producer for the mpeg4p2 codec */
        /* @@@ CurrentEPOID = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID; */
        CurrentDOFID = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
		mpeg4p2par_InitParserContext(ParserHandle);
        PARSER_Data_p->PictureGenericData.DecodingOrderFrameID = CurrentDOFID;
        /* @@@ PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID = CurrentEPOID; */
	}

    PARSER_Data_p->StreamState.ErrorRecoveryMode = GetPictureParams_p->ErrorRecoveryMode;
	/* Fill job submission time */
	PARSER_Data_p->ParserJobResults.ParsingJobStatus.JobSubmissionTime = time_now();

	/* Wakes up Parser task */
	if (PARSER_Data_p->ParserState == PARSER_STATE_PARSING)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module MPEG4P2 parser GetPicture: controller attempts to send a parser command when the parser is already working !"));
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
ST_ErrorCode_t mpeg4p2par_StartParserTask(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t  * PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    VIDCOM_Task_t               * const Task_p = &(PARSER_Data_p->ParserTask);
    char                          TaskName[25] = "STVID[?].MP4P2ParserTask";

	if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + ((PARSER_Properties_t *) ParserHandle)->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

	/* TODO: define PARSER own priority in vid_com.h */
    if (STOS_TaskCreate((void (*) (void*)) mpeg4p2par_ParserTaskFunc,
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
ST_ErrorCode_t mpeg4p2par_StopParserTask(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    MPEG4P2ParserPrivateData_t  * PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    VIDCOM_Task_t               * const Task_p = &(PARSER_Data_p->ParserTask);
    task_t                      * TaskList_p = Task_p->Task_p;

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
static ST_ErrorCode_t mpeg4p2par_Term(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t  * PARSER_Data_p;
	PARSER_Properties_t		    * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

	PARSER_Data_p = (MPEG4P2ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

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
    mpeg4p2par_StopParserTask(ParserHandle);

	/* Delete semaphore to share variables between parser and injection tasks */
	STOS_SemaphoreDelete(NULL, PARSER_Data_p->InjectSharedAccess);

	/* Delete semaphore between parser and controler tasks */
	STOS_SemaphoreDelete(NULL, PARSER_Data_p->ParserOrder);
	/* Delete semaphore for SC lists */
	STOS_SemaphoreDelete(NULL, PARSER_Data_p->ParserSC);
	/* Delete semaphore between controller and parser */
	STOS_SemaphoreDelete(NULL, PARSER_Data_p->ControllerSharedAccess);

   /* Deallocate the STAVMEM partitions */

	/* PES Buffer */
	if(PARSER_Data_p->PESBuffer.PESBufferBase_p != 0)
    {
        /* ErrorCode = */ mpeg4p2par_DeAllocatePESBuffer(ParserHandle);
    }
	/* SC Buffer */
	if(PARSER_Data_p->SCBuffer.SCList_Start_p != 0)
    {
        /* ErrorCode = */ mpeg4p2par_DeAllocateSCBuffer(ParserHandle);
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
 * Equivalent to Term + Init call without the buffer deallocation-	The FDMA is stopped
 *
 * \param ParserHandle
 * \return ST_Error_Code
 */
/******************************************************************************/
static ST_ErrorCode_t mpeg4p2par_Stop(const PARSER_Handle_t ParserHandle)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* Stops the FDMA */
    VIDINJ_TransferStop(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
	/* TODO: is task "InjecterTask" also stopped anywhere ? */

	/* TODO: what about terminating the parser task ? */

	/* Initialize the SC buffer pointers */
	mpeg4p2par_InitSCBuffer(ParserHandle);

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
static ST_ErrorCode_t mpeg4p2par_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;

    UNUSED_PARAMETER(Statistics_p);

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
static ST_ErrorCode_t mpeg4p2par_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

#ifdef STVID_DEBUG_GET_STATUS
/******************************************************************************/
/*! \brief Get status from the parser module
 *
 * Get parser status (content to be defined based on the current status available in STVID)
 *
 * \param ParserHandle
 * \param Status_p
 * \return ST_Error_Code
 */
/******************************************************************************/
static ST_ErrorCode_t mpeg4p2par_GetStatus(const PARSER_Handle_t ParserHandle, STVID_Status_t * const Status_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	VIDINJ_GetStatus(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, Status_p);

	/* Status_p->??? = PARSER_Data_p->Status.???; */
	return (ErrorCode);
}
#endif /* STVID_DEBUG_GET_STATUS */


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
void mpeg4p2par_PointOnNextSC(const PARSER_Handle_t ParserHandle)
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
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->SCBuffer.NextSCEntry ++;
   if((PARSER_Data_p->SCBuffer.NextSCEntry == PARSER_Data_p->SCBuffer.SCList_Loop_p)
        &&(PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p))
    {
		#ifdef STVID_PARSER_DO_NOT_LOOP_ON_STREAM
        PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Write_p;
		STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PARSER : Reached end of Stream"));
		#else
		/* Reached top of SCList */
        PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Start_p;
		#endif
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
void mpeg4p2par_GetNextStartCodeToProcess(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	BOOL SCFound;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
	STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
    U8                           NextSCUnitType;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
   STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

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

                if (NextSCUnitType == DIVX_311_HEADER_START_MARKER && mpeg4p2par_CheckSCSignature((U8 *)(GetSCAdd(CPUNextSCEntry_p))+1,NextSCUnitType, ParserHandle))
                     PARSER_Data_p->Is311SignatureUsed = TRUE;

                if (PARSER_Data_p->Is311SignatureUsed == TRUE)
                {
                    if (mpeg4p2par_CheckSCSignature((U8 *)(GetSCAdd(CPUNextSCEntry_p))+1,NextSCUnitType, ParserHandle))
                    {
                        SCFound = TRUE;
                		/* Get the Start Code itself */
                		PARSER_Data_p->StartCode.Value = GetSCVal(CPUNextSCEntry_p);
                        if (!PARSER_Data_p->NonVOPStartCodeFound)
                		{
                			if (PARSER_Data_p->StartCode.Value == USER_DATA_START_CODE && PARSER_Data_p->UserDataAllowed)
                			{
                                PARSER_Data_p->NonVOPStartCodeAddress = (U8 *)(mpeg4p2par_MoveForwardInStream((U8 *)GetSCAdd(CPUNextSCEntry_p), DIVX_311_SIGNATURE_SIZE, ParserHandle));
                                PARSER_Data_p->NonVOPStartCodeFound = TRUE;
                			}
                			else if (PARSER_Data_p->StartCode.Value != USER_DATA_START_CODE)
                			{
                                /*PARSER_Data_p->NonVOPStartCodeAddress = (U8 *)GetSCAdd(CPUNextSCEntry_p);*/
                                PARSER_Data_p->NonVOPStartCodeAddress = (U8 *)(mpeg4p2par_MoveForwardInStream((U8 *)GetSCAdd(CPUNextSCEntry_p), DIVX_311_SIGNATURE_SIZE, ParserHandle));
                                PARSER_Data_p->NonVOPStartCodeFound = TRUE;
                			}
                		}

                		/* Set the Parser Read Pointer in ES Bit Buffer on the start address of the start code */
                		PARSER_Data_p->StartCode.StartAddress_p = (U8 *)(mpeg4p2par_MoveForwardInStream((U8 *)GetSCAdd(CPUNextSCEntry_p), DIVX_311_SIGNATURE_SIZE, ParserHandle));
                		/*PARSER_Data_p->StartCode.StartAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);*/
                		/* A valid current start code is available */
                		PARSER_Data_p->StartCode.IsPending = TRUE;
                		PARSER_Data_p->StartCode.IsInBitBuffer = FALSE; /* The parser must now look for the next start code to know if the NAL is fully in the bit buffer */
                	}
                }
                else
                {
    				SCFound = TRUE;
    				/* Get the Start Code itself */
    				PARSER_Data_p->StartCode.Value = GetSCVal(CPUNextSCEntry_p);

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
    				/* A valid current start code is available */
    				PARSER_Data_p->StartCode.IsPending = TRUE;
    				PARSER_Data_p->StartCode.IsInBitBuffer = FALSE; /* The parser must now look for the next start code to know if the NAL is fully in the bit buffer */
                }
            }
            else
            {
                mpeg4p2par_SavePTSEntry(ParserHandle, CPUNextSCEntry_p);
            }
			/* Make ScannedEntry_p point on the next entry in SC List */
			mpeg4p2par_PointOnNextSC(ParserHandle);
       } /* end while */
	 }	/* if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p)*/
}	/* end mpeg4p2par_GetNextStartCodeToProcess() */


/*******************************************************************************/
/*! \brief Check if the VBS is fully in bit buffer: this is mandatory to process it.
 *
 *
 * \param ParserHandle
 * \return PARSER_Data_p->StartCode.IsInBitBuffer TRUE if fully in bit buffer
 */
/*******************************************************************************/
void mpeg4p2par_IsVOPInBitBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t            * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
	STFDMA_SCEntry_t                      * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
	BOOL                                    SCFound = FALSE;
	U8                                      SCUnitType, NextSCUnitType;

    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
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
		    if((mpeg4p2par_CheckSCSignature((U8 *)(GetSCAdd(CPUNextSCEntry_p))+1,GetUnitType(GetSCVal(CPUNextSCEntry_p)), ParserHandle) && PARSER_Data_p->Is311SignatureUsed == TRUE)
                 || PARSER_Data_p->Is311SignatureUsed == FALSE)
            {

    			if(!IsPTS(CPUNextSCEntry_p))
    			{
                    if (GetUnitType(PARSER_Data_p->StartCode.Value) == DISCONTINUITY_START_CODE)
                    {
                        /* In backward, we don't need to check discontinuity. Discontinuities are from one buffer to the other. */
    #ifdef ST_speed
    #ifdef STVID_TRICKMODE_BACKWARD
                        if (!PARSER_Data_p->Backward)
    #endif
                        {
                            STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_FIND_DISCONTINUITY_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                            SCFound = TRUE; /* to exit of the loop */
                            PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
                            /* If a discontinuity is found, then search for a random access point because we do not have all the reference pictures required to decode the
                             * pictures between the discontinuity and the first I after it. */
                            PARSER_Data_p->GetPictureParams.GetRandomAccessPoint = TRUE;
                            PARSER_Data_p->DiscontinuityDetected = TRUE;

            				#if defined(DVD_SECURED_CHIP)
                				/* Set the Parser Read Pointer in ES Copy Buffer */
                				PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
                				/* Set the Parser Read Pointer in bit buffer */
                				PARSER_Data_p->StartCode.BitBufferStopAddress_p = (U8 *)GetSCAddForBitBuffer(CPUNextSCEntry_p);
                			#else
                                /* Stop address of NAL to process is the start address of the following NAL */
                                PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
            				#endif /* DVD_SECURED_CHIP */
                        }
    #endif /* ST_speed */
                    }



    				NextSCUnitType = GetUnitType(GetSCVal(CPUNextSCEntry_p));

    				/* when the start code to process is not a VOP or picture start code, having any start code stored in the
    				 * SC+PTS buffer is enough to say that the start code to process is fully in the bit buffer
    				 */

    				SCUnitType = GetUnitType(PARSER_Data_p->StartCode.Value);
    				if (SCUnitType != VOP_START_CODE)
    				{
    					if ((/*NextSCUnitType >= VIDEO_OBJECT_START_CODE_MIN &&*/ NextSCUnitType <= VIDEO_OBJECT_START_CODE_MAX) ||
    						(NextSCUnitType >= VIDEO_OBJECT_LAYER_START_CODE_MIN && NextSCUnitType <= VIDEO_OBJECT_LAYER_START_CODE_MAX))
    					{
    						SCFound = TRUE; /* to exit of the loop */
    						PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
    						/* Stop address of the SC to process is the start address of the following SC */
    						PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
    					}
    					else
    					{
     					switch(NextSCUnitType)
    					{
    						case VISUAL_OBJECT_SEQUENCE_START_CODE:
    						case VISUAL_OBJECT_START_CODE:
    						case GROUP_OF_VOP_START_CODE:
    						case VOP_START_CODE:
    						case USER_DATA_START_CODE:
    						case VISUAL_OBJECT_SEQUENCE_END_CODE:
    						case DIVX_311_VIDEO_START_MARKER:	/* introduced a fake marker for Divx 311 */
    						case DIVX_311_HEADER_START_MARKER:	/* introduced a fake marker for Divx 311 */
    							SCFound = TRUE; /* to exit of the loop */
    							PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
    							/* Stop address of the SC to process is the start address of the following SC */
    							PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
    							break;

    						default:
    							break;
    						}
    					}

    				}
    				else
    				{

    					/* The start code to process is a VOP start code. Before processing it, the parser must ensure the
    					 * VOP/picture is fully in the bit buffer.
    					 * The VOP is fully in the bit buffer when any of the following start code are encountered after
    					 * the VOP start code:
    					 */

                        if (NextSCUnitType == VOP_START_CODE || NextSCUnitType == GROUP_OF_VOP_START_CODE || NextSCUnitType == VISUAL_OBJECT_SEQUENCE_END_CODE ||
                            NextSCUnitType == VISUAL_OBJECT_SEQUENCE_START_CODE || NextSCUnitType == VISUAL_OBJECT_START_CODE ||
                            (NextSCUnitType >= VIDEO_OBJECT_LAYER_START_CODE_MIN && NextSCUnitType <= VIDEO_OBJECT_LAYER_START_CODE_MAX) ||
                            (/*NextSCUnitType >= VIDEO_OBJECT_START_CODE_MIN &&*/ NextSCUnitType <= VIDEO_OBJECT_START_CODE_MAX))
    					{
    						SCFound = TRUE; /* to exit of the loop */
    						PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
    						/* Stop address of VOP to process is the start address of the following SC */
    						PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
    					}
    				}

    				if(NextSCUnitType == VISUAL_OBJECT_SEQUENCE_END_CODE)
    				{
    					PARSER_Data_p->SeqEndAfterThisSC = TRUE;
    				}

    			}/* end if(!IsPTS(CPUNextSCEntry_p)) */
			}
			/* Make ScannedEntry_p point on the next entry in SC List */
			mpeg4p2par_PointOnNextSC(ParserHandle);
		} /* end while */
	}

#ifdef STVID_TRICKMODE_BACKWARD
    if (   (PARSER_Data_p->Backward)
        && (!SCFound))
    {
        /* SC is not in Bit Buffer and will never be */
        /* since in backward parsing is done after injection */
        PARSER_Data_p->StartCode.IsInBitBuffer = FALSE;
    }
#endif

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
void mpeg4p2par_IsThereStartCodeToProcess(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t  * PARSER_Data_p;
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    STFDMA_SCEntry_t            * CheckSC;
    BOOL                          NotifyBitBufferFullyParsed;
#endif /*STVID_TRICKMODE_BACKWARD*/
	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Test if a new start-code is available */
    /*---------------------------------------*/
    STOS_SemaphoreWait(PARSER_Data_p->InjectSharedAccess);
    if (!(PARSER_Data_p->StartCode.IsPending)) /* Look for a start code to process */
	{
		mpeg4p2par_GetNextStartCodeToProcess(ParserHandle);

#ifdef STVID_TRICKMODE_BACKWARD
        if ((PARSER_Data_p->Backward) && (!PARSER_Data_p->IsBitBufferFullyParsed))
        {
            NotifyBitBufferFullyParsed = FALSE;

            CheckSC = PARSER_Data_p->SCBuffer.NextSCEntry;
            if (CheckSC != PARSER_Data_p->SCBuffer.SCList_Write_p)
            {
                CheckSC++;
                if (CheckSC == PARSER_Data_p->SCBuffer.SCList_Write_p)
                    NotifyBitBufferFullyParsed = TRUE;
            }
            else
            {
                NotifyBitBufferFullyParsed = TRUE;
            }

		    /*STTBX_Report((STTBX_REPORT_LEVEL_WARNING, "SC %d %d  ", PARSER_Data_p->SCBuffer.SCList_Write_p, PARSER_Data_p->SCBuffer.NextSCEntry));*/
		    /*STTBX_Print(("\n SC %d %d %d", PARSER_Data_p->SCBuffer.SCList_Write_p, CheckSC, PARSER_Data_p->SCBuffer.SCList_Loop_p));*/
            if (NotifyBitBufferFullyParsed)
            {
                PARSER_Data_p->IsBitBufferFullyParsed = TRUE;
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
				STTBX_Print(("\n BB fully parsed \r\n"));

                /* Bit Buffer is fully parsed: stop parsing ... */
                PARSER_Data_p->StopParsing = TRUE;
            }
        }
#endif

		/* PARSER_Data_p->StartCode.IsPending is modified in the function above */
	}

    if (PARSER_Data_p->StartCode.IsPending) /* Look if the VOP is fully in bit buffer */
	{
		mpeg4p2par_IsVOPInBitBuffer(ParserHandle);
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
void mpeg4p2par_InitByteStream(U8 * StartAddress, U8 * NextStartAddress, const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
    mpeg4p2par_MoveToNextByteInByteStream(ParserHandle);
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
void mpeg4p2par_ProcessStartCode(U8 StartCode, U8 * StartCodeValueAddress, U8 * NextPictureStartCodeValueAddress, const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	U8 SCUnitType;
	U32 flush_bits;
	U32 HeaderValue;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Initialize the ByteStream_t data structure to parse the SC entry in bit buffer */
    mpeg4p2par_InitByteStream(StartCodeValueAddress, NextPictureStartCodeValueAddress, ParserHandle);

	SCUnitType = GetUnitType(StartCode);

	if (/*(SCUnitType >= VIDEO_OBJECT_START_CODE_MIN) &&*/ (SCUnitType <= VIDEO_OBJECT_START_CODE_MAX))
	{
		HeaderValue = mpeg4p2par_ReadUnsigned(22, ParserHandle);
		if (HeaderValue == SHORT_VIDEO_START_MARKER)
		{
			flush_bits = mpeg4p2par_GetUnsigned(22, ParserHandle);
			PARSER_Data_p->VideoObjectPlane.ShortVideoHeader = 1;
			mpeg4p2par_ParseVideoPlaneWithShortHeader(ParserHandle);
		}
		else if (HeaderValue == SHORT_VIDEO_END_MARKER)
		{
			flush_bits = mpeg4p2par_GetUnsigned(22, ParserHandle);
			PARSER_Data_p->VideoObjectPlane.ShortVideoHeader = 0;
		}
		else
			PARSER_Data_p->VideoObjectPlane.ShortVideoHeader = 0;
	}
	else if (SCUnitType >= VIDEO_OBJECT_LAYER_START_CODE_MIN && SCUnitType <= VIDEO_OBJECT_LAYER_START_CODE_MAX)
	{
		PARSER_Data_p->UserDataAllowed = TRUE;		/* This variable indicates where we can allow user data to be taken into account */
		mpeg4p2par_ParseVideoObjectLayerHeader(ParserHandle);
	}
	else
	{
	switch(SCUnitType)
	{
		case VISUAL_OBJECT_SEQUENCE_START_CODE:
			PARSER_Data_p->UserDataAllowed = TRUE;		/* This variable indicates where we can allow user data to be taken into account */
			mpeg4p2par_ParseVisualObjectSequenceHeader(ParserHandle);
			break;

		case VISUAL_OBJECT_START_CODE:
			PARSER_Data_p->UserDataAllowed = TRUE;		/* This variable indicates where we can allow user data to be taken into account */
			mpeg4p2par_ParseVisualObjectHeader(ParserHandle);
			break;

		case USER_DATA_START_CODE:
			mpeg4p2par_ParseUserDataHeader(ParserHandle);
			break;

		case GROUP_OF_VOP_START_CODE:
			PARSER_Data_p->UserDataAllowed = TRUE;		/* This variable indicates where we can allow user data to be taken into account */
			mpeg4p2par_ParseGroupOfVOPHeader(ParserHandle);
			break;

		case SHORT_VIDEO_START_MARKER:
			mpeg4p2par_ParseVideoPlaneWithShortHeader(ParserHandle);
			break;

		case VOP_START_CODE:
			if (PARSER_Data_p->NonVOPStartCodeFound)
			{
				StartCodeValueAddress = PARSER_Data_p->NonVOPStartCodeAddress;
				PARSER_Data_p->NonVOPStartCodeFound = FALSE;
				PARSER_Data_p->UserDataAllowed = FALSE;	/* User data not allowed once the VOP starts streaming */
			}
			mpeg4p2par_ParseVideoObjectPlaneHeader(StartCodeValueAddress, NextPictureStartCodeValueAddress, ParserHandle);
			break;

		case DIVX_311_HEADER_START_MARKER: /* introduced a fake marker for Divx 311 */
			mpeg4p2par_ParseDivx311Header(ParserHandle);
			break;

		case DIVX_311_VIDEO_START_MARKER: /* introduced a fake marker for Divx 311 */
			mpeg4p2par_ParseDivx311Frame(StartCodeValueAddress, NextPictureStartCodeValueAddress, ParserHandle);
			break;

		case STUFFING_START_CODE:
			break;

		default:
			/* Do not process the other BDUs */
            break;
		}
	}

}/* end mpeg4p2par_ProcessStartCode()*/


/*******************************************************************************/
/*! \brief Fill the Parsing Job Result Data structure
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void mpeg4p2par_PictureInfoCompile(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	BOOL PictureHasError;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
	   (PARSER_Data_p->PictureSpecificData.PictureType != MPEG4P2_B_VOP))
	{
		PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame
			[PARSER_Data_p->StreamState.NextReferencePictureIndex] = FALSE;
	}

	/* Global Decoding Context structure */
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->
		GlobalDecodingContextGenericData = PARSER_Data_p->GlobalDecodingContextGenericData;
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->
		SizeOfGlobalDecodingContextSpecificDataInByte = sizeof(MPEG4P2_GlobalDecodingContextSpecificData_t);
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->
		GlobalDecodingContextSpecificData_p = &(PARSER_Data_p->GlobalDecodingContextSpecificData);

	/* Picture Decoding Context structure */
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->
		SizeOfPictureDecodingContextSpecificDataInByte = sizeof(MPEG4P2_PictureDecodingContextSpecificData_t);
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->
		PictureDecodingContextSpecificData_p = &(PARSER_Data_p->PictureDecodingContextSpecificData);

	/* Parsed Picture Information structure */
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData = PARSER_Data_p->PictureGenericData;
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->SizeOfPictureSpecificDataInByte = sizeof(MPEG4P2_PictureSpecificData_t);
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureSpecificData_p = &(PARSER_Data_p->PictureSpecificData);

	/* Parsing Job Status */

	/* Check whether picture has errors */
	if (PARSER_Data_p->PictureSpecificData.PictureError.SeqError.ProfileAndLevel ||
    	PARSER_Data_p->PictureSpecificData.PictureError.VOLError.AspectRatio ||
    	PARSER_Data_p->PictureSpecificData.PictureError.VOPError.CodedVOP ||
    	PARSER_Data_p->PictureSpecificData.PictureError.GOVError.ClosedGOV ||
    	PARSER_Data_p->PictureSpecificData.PictureError.GOVError.BrokenLink
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

    PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_NONE;
    if ((PARSER_Data_p->DiscontinuityDetected) && (PARSER_Data_p->PictureGenericData.ParsingError < VIDCOM_ERROR_LEVEL_BAD_DISPLAY))
    {
        PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
    }

	PARSER_Data_p->ParserJobResults.ParsingJobStatus.JobCompletionTime = time_now();
}


/*******************************************************************************/
/*! \brief Check whether an event "job completed" is to be sent to the controller
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void mpeg4p2par_CheckEventToSend(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t  * PARSER_Data_p;
	PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	U8                            RefFrameIndex;
	U8                            SCUnitType;
#ifdef STVID_TRICKMODE_BACKWARD
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
#endif
	PARSER_Data_p = (MPEG4P2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

	/* An event "job completed" is raised whenever a new picture has been parsed.
	 * (and in case a new picture is starting, the parser shall not parse anything
	 *  not to modify the parameters from the previously parsed picture)
	 *
	 */

	/* Check whether the current start code is that of a VOP */
	SCUnitType = GetUnitType(PARSER_Data_p->StartCode.Value);
	if (SCUnitType == VOP_START_CODE || SCUnitType == DIVX_311_VIDEO_START_MARKER) /* introduced a fake marker for Divx 311 */
	{
        if ((PARSER_Data_p->ParserState == PARSER_STATE_PARSING) &&
            (PARSER_Data_p->PictureLocalData.IsValidPicture))
		{
			mpeg4p2par_PictureInfoCompile(ParserHandle);
#if defined(STVID_TRICKMODE_BACKWARD) && !defined(DVD_SECURED_CHIP)
            if (PARSER_Data_p->Backward)
            {  /* To get bit rate information in backward */
                CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                /* to test if next Sc exist: end of buffer */
                if (((U32)((U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p))) > 0)
                {
                    PARSER_Data_p->OutputCounter += (U32)((U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p));
                    PARSER_Data_p->PictureGenericData.DiffPSC = PARSER_Data_p->OutputCounter;
                }
            }
#endif /* #if defined(STVID_TRICKMODE_BACKWARD) && !defined(DVD_SECURED_CHIP) */

#ifdef STVID_TRICKMODE_BACKWARD
            if (((PARSER_Data_p->AllReferencesAvailableForFrame) &&
                ((PARSER_Data_p->PictureLocalData.IsRandomAccessPoint == PARSER_Data_p->GetPictureParams.GetRandomAccessPoint) ||
                 ((PARSER_Data_p->SkipMode == STVID_DECODED_PICTURES_I) && (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) ||
                (!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint))) &&
                (!PARSER_Data_p->Backward))
                ||
                ((PARSER_Data_p->AllReferencesAvailableForFrame) &&
                (PARSER_Data_p->PictureLocalData.IsRandomAccessPoint) &&
                (PARSER_Data_p->Backward) &&
                (!PARSER_Data_p->IsBitBufferFullyParsed)))
#else
            if ( (PARSER_Data_p->AllReferencesAvailableForFrame) &&
                ((PARSER_Data_p->PictureLocalData.IsRandomAccessPoint == PARSER_Data_p->GetPictureParams.GetRandomAccessPoint) ||
                (!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint))))
#endif
			{
				/* Only update the FullReferenceFrameList for the frame that is not a B */
                if((PARSER_Data_p->PictureGenericData.IsReference) &&
					(PARSER_Data_p->PictureSpecificData.PictureType != MPEG4P2_B_VOP))
				{
					RefFrameIndex = PARSER_Data_p->StreamState.NextReferencePictureIndex;
					PARSER_Data_p->PictureGenericData.FullReferenceFrameList[RefFrameIndex] = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;

					PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[RefFrameIndex] = TRUE;

					/* Update the index for the next reference frame. It will either be slot 0 or 1. */
					PARSER_Data_p->StreamState.NextReferencePictureIndex = (RefFrameIndex ^ 1);
				}/* end if(PARSER_Data_p->PictureGenericData.IsReference == TRUE) */

				STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->ParserJobResults));

                PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;
            }
            else
            {
                /* WARNING, at this stage only picture's start & stop addresses in the bit buffer are valid data, all remaingin of the picture's data might be unrelevant because picture parsing is not complete */
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData));
                /* Wake up task to look for next start codes */
                STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
            }

#ifdef TRACE_MPEG4P2_PICTURE_TYPE
            mpeg4p2par_OutputDataStructure(ParserHandle);
#endif

			/* Increment the Decoding Order Frame ID for each frame */
			PARSER_Data_p->PictureGenericData.DecodingOrderFrameID++;
		}/* end  if ((PARSER_Data_p->ParserState == PARSER_STATE_PARSING) ...*/
        else
        {
#ifdef STVID_TRICKMODE_BACKWARD
            if (!PARSER_Data_p->Backward)
#endif
            {
                /* WARNING, at this stage only picture's start & stop addresses in the bit buffer are valid data, all remaingin of the picture's data might be unrelevant because picture parsing is not complete */
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData) );
            }
        }
	}/* end if ((PARSER_Data_p->StartCode.Value == VOP_START_CODE) ...*/
   else
	{
        /* Wake up task to look for next start codes */
        STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
	}
}/* end mpeg4p2par_CheckEventToSend() */


/*******************************************************************************/
/*! \brief Get the Start Code commands. Flush the command queue
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void mpeg4p2par_GetStartCodeCommand(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
#ifdef STVID_TRICKMODE_BACKWARD
	PARSER_Properties_t * PARSER_Properties_p;
	PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
#endif
	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* The start code to process is in the StartCodeCommand buffer */

    if ((PARSER_Data_p->StartCode.IsPending) && (PARSER_Data_p->StartCode.IsInBitBuffer))
	{
		/* The SC can now be processed */
	 	mpeg4p2par_ProcessStartCode(PARSER_Data_p->StartCode.Value,
						  PARSER_Data_p->StartCode.StartAddress_p,
						  PARSER_Data_p->StartCode.StopAddress_p,
						  ParserHandle);

		mpeg4p2par_CheckEventToSend(ParserHandle);

		/* The start code has been analyzed: it is not pending anymore */
		PARSER_Data_p->StartCode.IsPending = FALSE;
	}
#ifdef STVID_TRICKMODE_BACKWARD
    else if ((PARSER_Data_p->Backward) && (PARSER_Data_p->StartCode.IsPending) && (!(PARSER_Data_p->StartCode.IsInBitBuffer)) && (!PARSER_Data_p->IsBitBufferFullyParsed))
	{
        PARSER_Data_p->IsBitBufferFullyParsed = TRUE;
        STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

        /* Bit Buffer is fully parsed: stop parsing ... */
        PARSER_Data_p->StopParsing = TRUE;
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
void mpeg4p2par_GetControllerCommand(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
void mpeg4p2par_ParserTaskFunc(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STOS_TaskEnter(NULL);

    /* Big loop executed until ToBeDeleted is set --------------------------- */
	do
	{
		while(!PARSER_Data_p->ParserTask.ToBeDeleted && (PARSER_Data_p->ParserState != PARSER_STATE_PARSING))
		{
			/* Waiting for a command */
			STOS_SemaphoreWait(PARSER_Data_p->ParserOrder);
			mpeg4p2par_GetControllerCommand(ParserHandle);
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
                    if (((PARSER_Data_p->Backward) && (!PARSER_Data_p->StopParsing)) ||
                         (!PARSER_Data_p->Backward))
#endif /* STVID_TRICKMODE_BACKWARD */
                    {
                        mpeg4p2par_IsThereStartCodeToProcess(ParserHandle);
                        /* Then, get Start code commands */
                        mpeg4p2par_GetStartCodeCommand(ParserHandle);
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
static ST_ErrorCode_t mpeg4p2par_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
static ST_ErrorCode_t mpeg4p2par_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t    * PARSER_Data_p;
    U8                            * LatchedES_Write_p = NULL;             /* write pointer (of FDMA) in ES buffer   */
    U8                            * LatchedES_DecoderRead_p = NULL;       /* Read pointer (of decoder) in ES buffer */

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#ifdef STVID_TRICKMODE_BACKWARD
    if (PARSER_Data_p->Backward)
	{
        U32     Level;
        U8      Pattern[]={0xAF, 0xAF, 0x00, 0x00, 0x01, 0xB8};

        /* Ensure no FDMA transfer is done while flushing the injecter */
        VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

        /* Flush the input buffer content before getting the linear buffer level */
        VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, FALSE, Pattern, sizeof(Pattern));

        /* CD (25/07/2007): we should probably protect access to BitBuffer struct with a semaphore, to be confirmed ... */
        /*                  This is done in mpeg2 with: SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);*/
        Level = (U32)PARSER_Data_p->BitBuffer.ES_Write_p - (U32)PARSER_Data_p->BitBuffer.ES_Start_p + 1;

        /* Release FDMA transfers */
        VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

        /* Return the buffer level value */
        GetBitBufferLevelParams_p->BitBufferLevelInBytes = Level;
	}
    else
#endif /* STVID_TRICKMODE_BACKWARD */
    {
        LatchedES_Write_p       = PARSER_Data_p->BitBuffer.ES_Write_p;
        LatchedES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_DecoderRead_p;

        if (LatchedES_Write_p < LatchedES_DecoderRead_p)
    	{
        	/* Write pointer has done a loop, not the Read pointer */
        	GetBitBufferLevelParams_p-> BitBufferLevelInBytes = PARSER_Data_p->BitBuffer.ES_Stop_p
                                                            - LatchedES_DecoderRead_p
                                                            + LatchedES_Write_p
                                                          	- PARSER_Data_p->BitBuffer.ES_Start_p
                                                          	+ 1;
    	}
    	else
    	{
        	/* Normal: start <= read <= write <= top */
            GetBitBufferLevelParams_p-> BitBufferLevelInBytes = LatchedES_Write_p
                                                            - LatchedES_DecoderRead_p;
    	}
	}

	return (ST_NO_ERROR);
}

static ST_ErrorCode_t	mpeg4p2par_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle,
												void ** const BaseAddress_p,
												U32 * const Size_p)
{
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    *BaseAddress_p    = STAVMEM_DeviceToVirtual(PARSER_Data_p->PESBuffer.PESBufferBase_p, &VirtualMapping);
    *Size_p           = (U32)PARSER_Data_p->PESBuffer.PESBufferTop_p
                      - (U32)PARSER_Data_p->PESBuffer.PESBufferBase_p
                      + 1;

    return(ST_NO_ERROR);
}

static ST_ErrorCode_t	mpeg4p2par_SetDataInputInterface(const PARSER_Handle_t ParserHandle,
								ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
								void (*InformReadAddress)(void * const Handle, void * const Address),
								void * const Handle)

{
	ST_ErrorCode_t Err = ST_NO_ERROR;
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Ensure no FDMA transfer is done while setting a new data input interface */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    Err = VIDINJ_Transfer_SetDataInputInterface(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum,
                                    GetWriteAddress,InformReadAddress,Handle);

	/* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    return(Err);

}

/******************************************************************************/
/*! \brief Print the Structure Values
 *
 * \param ParserHandle
 * \param
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_OutputDataStructure(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

#ifdef TRACE_MPEG4P2_PICTURE_TYPE
    TracePictureType(("ParserType","%c",
       (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I ? 'I' :
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P ? 'P' :
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B ? 'B' : '?')
        ));
#endif /** TRACE_MPEG4P2_PICTURE_TYPE **/
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
Name        : mpeg4p2par_RegisterTrace
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MPEG4P2PARSER_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef TRACE_UART
	TRACE_ConditionalRegister(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_BLOCKNAME, mpeg4p2par_TraceItemList, sizeof(mpeg4p2par_TraceItemList)/sizeof(TRACE_Item_t));
#endif /* TRACE_UART */

	return(ErrorCode);
}
#endif /* VALID_TOOLS */

#ifdef ST_speed
/*******************************************************************************
Name        : mpeg4p2par_GetBitBufferOutputCounter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 mpeg4p2par_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle)
{                 /* To be implemented. Used to compute bit rate */
    MPEG4P2ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32 DiffPSC;
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */

    PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
    PARSER_Data_p->OutputCounter              += DiffPSC;
    return(PARSER_Data_p->OutputCounter);

} /*End mpeg4p2par_GetBitBufferOutputCounter*/
#endif /*ST_speed*/


/*******************************************************************************
Name        : mpeg4p2par_Setup
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t mpeg4p2par_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while setting the FDMA node partition */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    ErrorCode = VIDINJ_ReallocateFDMANodes(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, SetupParams_p);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    return(ErrorCode);
} /* End of mpeg4p2par_Setup() function */

/*******************************************************************************
Name        : mpeg4p2par_FlushInjection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void mpeg4p2par_FlushInjection(const PARSER_Handle_t ParserHandle)
{
    MPEG4P2ParserPrivateData_t * PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    U8                        Pattern[]={0x00, 0x00, 0x01, 0x38};

    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, TRUE, Pattern, sizeof(Pattern));
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
#ifdef STVID_TRICKMODE_BACKWARD
	if (PARSER_Data_p->Backward)
    {
        PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    }
#endif /* STVID_TRICKMODE_BACKWARD */
    PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID ++;
    PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID;
}
#ifdef ST_speed
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : mpeg4p2par_SetBitBuffer
Description : Set the location of the bit buffer
Parameters  : HAL Parser manager handle, buffer address, buffer size in bytes
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void mpeg4p2par_SetBitBuffer(const PARSER_Handle_t        ParserHandle,
                                 void * const                 BufferAddressStart_p,
                                 const U32                    BufferSize,
                                 const BitBufferType_t        BufferType,
                                 const BOOL                   Stop  )
{
    U8                              Pattern[]={0x00, 0x00, 0x01, 0x38};
    MPEG4P2ParserPrivateData_t    * PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while setting a new decode buffer */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    switch (BufferType)
    {
        case BIT_BUFFER_LINEAR :
            PARSER_Data_p->Backward = TRUE;
            PARSER_Data_p->IsBitBufferFullyParsed = FALSE;
            if (Stop)
            {
            PARSER_Data_p->BitBuffer.ES_Start_p         = BufferAddressStart_p;
            PARSER_Data_p->BitBuffer.ES_Stop_p          = (void *)((U32)BufferAddressStart_p + BufferSize -1);

                PARSER_Data_p->StopParsing = TRUE;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "parser stop "));
            }
            else
            {
                PARSER_Data_p->StopParsing = FALSE;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "parser start "));
                PARSER_Data_p->IsBitBufferFullyParsed = FALSE;
                PARSER_Data_p->SCBuffer.NextSCEntry         = PARSER_Data_p->SCBuffer.SCList_Start_p;
            }
            break;

        default:
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "bad type of bit buffer "));
            /* No break */
        case BIT_BUFFER_CIRCULAR :
            PARSER_Data_p->BitBuffer.ES_Start_p         = PARSER_Data_p->BitBuffer.ES_StoreStart_p;
            PARSER_Data_p->BitBuffer.ES_Stop_p          = PARSER_Data_p->BitBuffer.ES_StoreStop_p;
            PARSER_Data_p->Backward = FALSE;
            PARSER_Data_p->StopParsing = FALSE;
            break;
    }

    PARSER_Data_p->BitBuffer.ES_DecoderRead_p   = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* DG TO DO: change prototype for secured platforms (add ESCopyBuffer Base_p and Top_p instead of NULL */

    if ((PARSER_Data_p->Backward) && (Stop))
    {
		  VIDINJ_TransferLimits(PARSER_Data_p->Inject.InjecterHandle,
                            PARSER_Data_p->Inject.InjectNum,
         /* BB */           PARSER_Data_p->BitBuffer.ES_Start_p,
                            PARSER_Data_p->BitBuffer.ES_Stop_p,
         /* SC */           PARSER_Data_p->SCBuffer.SCList_Start_p,
                            PARSER_Data_p->SCBuffer.SCList_Stop_p,
        /* PES */           PARSER_Data_p->PESBuffer.PESBufferBase_p,
                            PARSER_Data_p->PESBuffer.PESBufferTop_p
#if defined(DVD_SECURED_CHIP)
        /* ES Copy */     ,(void *)NULL, (void *)NULL
#endif /* DVD_SECURED_CHIP */
                           );

        /* Inform the FDMA on the read pointer of the decoder in Bit Buffer */
        VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, TRUE, Pattern, sizeof(Pattern));

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

} /* End of mpeg4p2par_SetBitBuffer() function */

/*******************************************************************************
Name        : mpeg4p2par_WriteStartCode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void mpeg4p2par_WriteStartCode(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p)
{
    const U8*  Temp8;
    MPEG4P2ParserPrivateData_t * PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

} /* End of mpeg4p2par_WriteStartCode */
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed*/

/*******************************************************************************
Name        : SavePTSEntry
Description : Stores a new parsed PTSEntry in the PTS StartCode List
Parameters  : Parser handle, PTS Entry Pointer in FDMA's StartCode list
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void mpeg4p2par_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* CPUPTSEntry_p)
{
    U32 PTSSCAdd;
	MPEG4P2ParserPrivateData_t * PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PTS_t* PTSLIST_WRITE_PLUS_ONE;

    PTSSCAdd = GetSCAdd(CPUPTSEntry_p);
    PTSSCAdd = PTSSCAdd - 3;

    if ((U32)PTSSCAdd != (U32)PARSER_Data_p->LastPTSStored.Address_p)
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
        PTSLIST_WRITE->Address_p = (void*)PTSSCAdd;
        PTSLIST_WRITE++;

        if (PTSLIST_WRITE >= PTSLIST_TOP)
        {
            PTSLIST_WRITE = PTSLIST_BASE;
        }
        PARSER_Data_p->LastPTSStored.PTS33      = GetPTS33(CPUPTSEntry_p);
        PARSER_Data_p->LastPTSStored.PTS32      = GetPTS32(CPUPTSEntry_p);
        PARSER_Data_p->LastPTSStored.Address_p  = (void*)PTSSCAdd;
    }
} /* End of mpeg4p2par_SavePTSEntry() function */

#if defined(DVD_SECURED_CHIP)
/*******************************************************************************/
/*! \brief Initialize the extra injection buffer for secured platforms
 *
 * Initialize the ES Copy Buffer pointers,
 * Inform FDMA about these buffers allocation
 *
 * \param ParserHandle Parser Handle
 * \param PARSER_ExtraParams_t ES Copy buffer start address + size
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
ST_ErrorCode_t mpeg4p2par_SecureInit(const PARSER_Handle_t          ParserHandle,
		                             const PARSER_ExtraParams_t *   const SecureInitParams_p)
{
    /* Feature not supported */
    UNUSED_PARAMETER(ParserHandle);
    UNUSED_PARAMETER(SecureInitParams_p);

	return (ST_NO_ERROR);
} /* end of mpeg4p2par_SecureInit() */
#endif /* DVD_SECURED_CHIP */

/*******************************************************************************/
/*! \brief Initialize the new injection buffer for secured platforms
 *
 * Initialize the ES Bit Buffer pointers,
 * Inform FDMA about these buffers allocation
 *
 * \param ParserHandle Parser Handle
 * \param PARSER_ExtraParams_t ES bit buffer addresses + size
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
ST_ErrorCode_t mpeg4p2par_BitBufferInjectionInit(const PARSER_Handle_t         ParserHandle,
		                                         const PARSER_ExtraParams_t *  const BitBufferInjectionParams_p)
{
    /* Feature not supported */
    UNUSED_PARAMETER(ParserHandle);
    UNUSED_PARAMETER(BitBufferInjectionParams_p);

    return (ST_NO_ERROR);
} /* end of mpeg4p2par_BitBufferInjectionInit() */

/*******************************************************************************/
/*
 *
 check the signature of the start code regarding the state of the Profile
 and return TRUE, if the start code has a signature
 *
 */
/*******************************************************************************/
static int mpeg4p2par_CheckSCSignature(register unsigned char *Address_p,unsigned char SCValue, const PARSER_Handle_t  ParserHandle)
{

    U8 * SHAddress_p;
    U8 * StartAddress_p;
    U8 * StopAddress_p;
    U8 SignatureFound;

    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    StartAddress_p = STAVMEM_DeviceToCPU((void *)(PARSER_Data_p->BitBuffer.ES_Start_p),&VirtualMapping);
    StopAddress_p = STAVMEM_DeviceToCPU((void *)(PARSER_Data_p->BitBuffer.ES_Stop_p),&VirtualMapping);
    SHAddress_p = STAVMEM_DeviceToCPU((void *)(Address_p),&VirtualMapping);

    SignatureFound = TRUE;

	if(SHAddress_p == NULL)
        SignatureFound = FALSE;

	if(*SHAddress_p++ != 'S')
	   SignatureFound = FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != 'T')
	   SignatureFound = FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != 'M')
	   SignatureFound = FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != 'I')
	   SignatureFound = FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != 'C')
	   SignatureFound = FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != SCValue)
	   SignatureFound = FALSE;

    if (SignatureFound == TRUE )
    	return TRUE;

    return FALSE;
}

/* End of mpeg4p2parser.c */
