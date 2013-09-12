/*!
 *******************************************************************************
 * \file h264parser.c
 *
 * \brief H264 Parser top level CODEC API functions
 *
 * \author
 * Olivier Deygas \n
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

/* CODEC API function prototypes */
#include "parser.h"

/* H264 Parser specific */
#include "h264parser.h"

/*#define STTBX_REPORT*/
#include "sttbx.h"

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/*#define TRACE_UART*/

/* Select the Traces you want */
#define TrESCopy        TraceOff
#define TrStartCode     TraceOff
#define TrInject        TraceOff
#define TrPTS           TraceOff
#define TrWA            TraceOff

/* "vid_trace.h" and "trace.h" should be included AFTER the definition of the traces wanted */
#if defined TRACE_UART || defined TRACE_INJECT || defined TRACE_FLUSH
    #include "..\..\tests\src\trace.h"
#endif

#include "vid_trace.h"

/* This workaround allow H264 parser to analyze and detect whether injected stream is H264 stream or MPEG2 stream */
/* MPEG2 data when detected are dropped to avoid trying to decode dummy video data*/
#define WA_GNBvd63171

/* Constants ---------------------------------------------------------------- */
/* startcode list size CL WORKAROUND */
/* Specific very low res (128x96 pixels) streams makes SC list overflows, such streams counts 12484 SC for 2,603,131 bytes of stream
 * this lead to a SC/bitbuffer size ratio equals to 0.0048
 * Applying the same formula than lcmpeg1 we found  0.00525 for the ratio
 * Attempt to adapt the SC list size to the bit buffer size based on the lcmpeg ratio.
 * For a 5120768 bytes bit buffer it leads to 26886 entries in the start code list.
*/
#define SCLIST_SIZE_STANDARD    2000
#define BITBUFFER_SIZE_STANDARD 380928
#define NB_SC_ANALYZED 5   /* number of successive start code used to analyzed whether the receive stream is H264 and not MPEG2 stream*/

/* Avoid parsing the whole picture, if it will be skipped. This is not possible
 * with trickmode because we need to know how many fields we skipped. */
#if !defined(ST_speed)
#define STVID_OPTIMIZE_PARSER_SKIP_NONREF_PICTURES
#endif

const PARSER_FunctionsTable_t PARSER_H264Functions =
{
#ifdef STVID_DEBUG_GET_STATISTICS
    h264par_ResetStatistics,
    h264par_GetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    h264par_GetStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    h264par_Init,
    h264par_Stop,
    h264par_Start,
    h264par_GetPicture,
    h264par_Term,
	h264par_InformReadAddress,
	h264par_GetBitBufferLevel,
	h264par_GetDataInputBufferParams,
    h264par_SetDataInputInterface,
    h264par_Setup,
    h264par_FlushInjection
#ifdef ST_speed
    ,h264par_GetBitBufferOutputCounter
#ifdef STVID_TRICKMODE_BACKWARD
    ,h264par_SetBitBuffer
    ,h264par_WriteStartCode
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed */
    ,h264par_BitBufferInjectionInit
#if defined(DVD_SECURED_CHIP)
    ,h264par_SecureInit
#endif
};


/* Functions ---------------------------------------------------------------- */
static ST_ErrorCode_t h264par_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t h264par_AllocatePESBuffer(const PARSER_Handle_t ParserHandle, const U32 PESBufferSize, const PARSER_InitParams_t *  const InitParams_p);
static ST_ErrorCode_t h264par_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t h264par_AllocateSCBuffer(const PARSER_Handle_t ParserHandle, const U32 SC_ListSize, const PARSER_InitParams_t * const InitParams_p);

static ST_ErrorCode_t h264par_ReallocatePESBuffer(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * SetupParams_p);
static ST_ErrorCode_t h264par_ReallocateStartCodeBuffer(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * SetupParams_p);

static void h264par_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* CPUPTSEntry_p);
static BOOL h264par_CheckNextSCValid(const PARSER_Handle_t ParserHandle);
static STFDMA_SCEntry_t * h264par_GetNextStartCodeToCheck(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t * SCEntry_p);
static STFDMA_SCEntry_t * h264par_CheckOnNextSC(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t * SCEntry_p);
static void h264par_IncrementPossibleFakeStartCodeCounter(U8 AnalysedStartCodeValue, U8 PreviousStartCodeValue, U32 * error_count_p);

/*******************************************************************************/
/*! \brief De-Allocate the PES Buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t h264par_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t         * PARSER_Properties_p;
    STAVMEM_FreeBlockParams_t   FreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    /* Dereference ParserHandle to a local variable to ease debug */
	PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	PARSER_Data_p = (H264ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->PESBuffer.PESBufferHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The PESBuffer is not deallocated correctly!"));
    }

	return (ErrorCode);
} /* end of h264par_DeAllocatePESBuffer */

/*******************************************************************************/
/*! \brief Allocate the PES Buffer
 *
 * \param ParserHandle Parser Handle
 * \param PESBufferSize PES buffer size in bytes
 * \param InitParams_p Init parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t h264par_AllocatePESBuffer(const PARSER_Handle_t        ParserHandle,
		                               const U32                    PESBufferSize,
                                       const PARSER_InitParams_t *  const InitParams_p)
{
	H264ParserPrivateData_t * PARSER_Data_p;
	STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;

	ST_ErrorCode_t                          ErrorCode;

    /* Dereference ParserHandle to a local variable to ease debug */
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams,&PARSER_Data_p->PESBuffer.PESBufferHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }
    ErrorCode = STAVMEM_GetBlockAddress(PARSER_Data_p->PESBuffer.PESBufferHdl, (void **)&VirtualAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* ErrorCode = */ h264par_DeAllocatePESBuffer(ParserHandle);
        return(ErrorCode);
    }
    PARSER_Data_p->PESBuffer.PESBufferBase_p = STAVMEM_VirtualToDevice(VirtualAddress,STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    PARSER_Data_p->PESBuffer.PESBufferTop_p = (void *) ((U32)PARSER_Data_p->PESBuffer.PESBufferBase_p + PESBufferSize - 1);

	return (ErrorCode);
} /* end of h264par_AllocatePESBuffer() */

/*******************************************************************************/
/*! \brief Reallocate the PES buffer and initialize the PES pointers for injection
 *
 * \param ParserHandle Parser Handle
 * \param SetupParams_p SETUP parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t h264par_ReallocatePESBuffer(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * SetupParams_p)
{
    H264ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_BlockParams_t BlockParams;
	ST_ErrorCode_t        ErrorCode;
	U32                   PESListSize;
	PARSER_InitParams_t   InitParams;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if (SetupParams_p->SetupSettings.AVMEMPartition == 0)   /* STAVMEM_INVALID_PARTITION_HANDLE may not exist yet! */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get PES buffer information */
    /* Can't use STAVMEM_ReallocBlock() here because we need to change partition */
    ErrorCode = STAVMEM_GetBlockParams(PARSER_Data_p->PESBuffer.PESBufferHdl, &BlockParams);

    if (ErrorCode == ST_NO_ERROR)
    {
        PESListSize = BlockParams.Size;
    }
    else
    {
        /* STAVMEM_GetBlockParams returned an error */
        /* Fill-up alloc params with default values */
        PESListSize = PES_BUFFER_SIZE;
    }

    /* Deallocate existing PES buffer from current partition */
    ErrorCode = h264par_DeAllocatePESBuffer(ParserHandle);

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Allocate PES buffer in new partition passed as parameter */
        InitParams.AvmemPartitionHandle = SetupParams_p->SetupSettings.AVMEMPartition;
    	ErrorCode = h264par_AllocatePESBuffer(ParserHandle, PESListSize, &InitParams);

        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error allocating PES Buffer, return with error */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error when reallocating PES buffer!"));
        }
    }
    else
    {
        /* Error freeing PES Buffer, return with error */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error when freeing original PES buffer!"));
    }

    return(ErrorCode);
} /* End of h264par_ReallocatePESBuffer() */


/*******************************************************************************/
/*! \brief De-Allocate the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t h264par_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t         * PARSER_Properties_p;
    STAVMEM_FreeBlockParams_t   FreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = (H264ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->SCBuffer.SCListHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The SCBuffer is not deallocated correctly !"));
    }

    return(ErrorCode);
} /* end of h264par_DeAllocateSCBuffer() */

/*******************************************************************************/
/*! \brief Allocate the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \param SC_ListSize Start Code buffer size in bytes
 * \param InitParams_p Init parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t h264par_AllocateSCBuffer(const PARSER_Handle_t        ParserHandle,
		                      const U32                    SC_ListSize,
						      const PARSER_InitParams_t *  const InitParams_p)
{
	H264ParserPrivateData_t * PARSER_Data_p;

    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;
	ST_ErrorCode_t                          ErrorCode;

    /* dereference ParserHandle to a local variable to ease debug */
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	AllocBlockParams.PartitionHandle       = InitParams_p->AvmemPartitionHandle;
    AllocBlockParams.Alignment     = 32;
    AllocBlockParams.Size          = SC_ListSize * sizeof(STFDMA_SCEntry_t);
    AllocBlockParams.AllocMode     = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &PARSER_Data_p->SCBuffer.SCListHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }
    ErrorCode = STAVMEM_GetBlockAddress(PARSER_Data_p->SCBuffer.SCListHdl,(void **)&VirtualAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* ErrorCode = */ h264par_DeAllocateSCBuffer(ParserHandle);
        return(ErrorCode);
    }
    PARSER_Data_p->SCBuffer.SCList_Start_p = STAVMEM_VirtualToDevice(VirtualAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    PARSER_Data_p->SCBuffer.SCList_Stop_p  = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.SCList_Stop_p  += (SC_ListSize-1);

	return (ErrorCode);
} /* end of h264par_AllocateSCBuffer() */


/*******************************************************************************/
/*! \brief Reallocate the Start Code buffer and initialize the SC pointers
 *
 * \param ParserHandle Parser Handle
 * \param SetupParams_p SETUP parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t h264par_ReallocateStartCodeBuffer(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * SetupParams_p)
{
    H264ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_BlockParams_t BlockParams;
	ST_ErrorCode_t        ErrorCode;
	U32                   StartCodeListSize;
	PARSER_InitParams_t   InitParams;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if (SetupParams_p->SetupSettings.AVMEMPartition == 0)   /* STAVMEM_INVALID_PARTITION_HANDLE may not exist yet! */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get Start code list buffer information */
    /* Can't use STAVMEM_ReallocBlock() here because we need to change partition */
    ErrorCode = STAVMEM_GetBlockParams(PARSER_Data_p->SCBuffer.SCListHdl, &BlockParams);

    if (ErrorCode == ST_NO_ERROR)
    {
        StartCodeListSize = BlockParams.Size / sizeof(STFDMA_SCEntry_t);
    }
    else
    {
        /* STAVMEM_GetBlockParams returned an error */
        /* Fill-up alloc params with default values */
        StartCodeListSize = SC_LIST_SIZE_IN_ENTRY;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Could not get SCList info, using default value for SCList size! (%d)", StartCodeListSize));
    }

    /* Deallocate existing start code buffer from current partition */
    ErrorCode = h264par_DeAllocateSCBuffer(ParserHandle);

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Allocate Start Code buffer in new partition passed as parameter */
        InitParams.AvmemPartitionHandle = SetupParams_p->SetupSettings.AVMEMPartition;
    	ErrorCode = h264par_AllocateSCBuffer(ParserHandle, StartCodeListSize, &InitParams);

        if (ErrorCode == ST_NO_ERROR)
        {
            /* Initialize the SC buffer pointers */
	        h264par_InitSCBuffer(ParserHandle);
	    }
	    else
        {
            /* Error allocating SC Buffer, return with error */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error when reallocating SCList buffer!"));
        }
    }
    else
    {
        /* Error freeing SC Buffer, return with error */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error when freeing original SCList buffer!"));
    }

    return(ErrorCode);
} /* End of h264par_ReallocateStartCodeBuffer() */


/*******************************************************************************/
/*! \brief Initialize the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \return void
 */
/*******************************************************************************/
void h264par_InitSCBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Set default values for pointers in SC-List */
    PARSER_Data_p->SCBuffer.SCList_Write_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.SCList_Loop_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.CurrentSCEntry = PARSER_Data_p->SCBuffer.SCList_Start_p;

    /* No PTS found for next picture */
    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
    PARSER_Data_p->PTS_SCList.Write_p          = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
    PARSER_Data_p->PTS_SCList.Read_p           = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
    PARSER_Data_p->LastPTSStored.Address_p     = NULL;
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
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
ST_ErrorCode_t h264par_InitInjectionBuffers(const PARSER_Handle_t        ParserHandle,
		                                    const PARSER_InitParams_t *  const InitParams_p,
						            		const U32                    SC_ListSize,
								            const U32 				   PESBufferSize
						                   )
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
    VIDINJ_GetInjectParams_t              Params;
	ST_ErrorCode_t                          ErrorCode;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	ErrorCode = ST_NO_ERROR;

	/* Allocate SC Buffer */
    ErrorCode = h264par_AllocateSCBuffer(ParserHandle, SC_ListSize, InitParams_p);
	if (ErrorCode != ST_NO_ERROR)
	{
		return (ErrorCode);
	}

	/* Allocate PES Buffer */
    ErrorCode = h264par_AllocatePESBuffer(ParserHandle, PESBufferSize, InitParams_p);
	if (ErrorCode != ST_NO_ERROR)
	{
        /* ErrorCode = */ h264par_DeAllocateSCBuffer(ParserHandle);
		return (ErrorCode);
	}

    #if defined(DVD_SECURED_CHIP)
        /* ES Copy init done after STVID_Setup() function for object STVID_SETUP_ES_COPY_BUFFER */
    #endif /* DVD_SECURED_CHIP */

	/* Set default values for pointers in ES Buffer */
    PARSER_Data_p->BitBuffer.ES_Start_p = InitParams_p->BitBufferAddress_p;
    /* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
    PARSER_Data_p->BitBuffer.ES_Stop_p = (void *)((U32)InitParams_p->BitBufferAddress_p + InitParams_p->BitBufferSize -1);
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Data_p->BitBuffer.ES_StoreStart_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    PARSER_Data_p->BitBuffer.ES_StoreStop_p = PARSER_Data_p->BitBuffer.ES_Stop_p;
#endif  /* STVID_TRICKMODE_BACKWARD */

	/* Initialize the SC buffer pointers */
	h264par_InitSCBuffer(ParserHandle);

	/* Reset ES bit buffer write pointer */
	PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

	/* Synchronize buffer definitions with FDMA */
	/* link this init with a fdma channel */
    /*------------------------------------*/
    Params.TransferDoneFct                 = h264par_DMATransferDoneFct;
    Params.UserIdent                       = (U32)ParserHandle;
    Params.AvmemPartition                  = InitParams_p->AvmemPartitionHandle;
    Params.CPUPartition                    = InitParams_p->CPUPartition_p;

    PARSER_Data_p->Inject.InjectNum        = VIDINJ_TransferGetInject(PARSER_Data_p->Inject.InjecterHandle, &Params);
    if (PARSER_Data_p->Inject.InjectNum == BAD_INJECT_NUM)
    {
        /* ErrorCode = */ h264par_DeAllocatePESBuffer(ParserHandle);
        /* ErrorCode = */ h264par_DeAllocateSCBuffer(ParserHandle);
        return(ST_ERROR_NO_MEMORY);
    }

	return (ErrorCode);
} /* end of h264par_InitInjectionBuffers() */


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
ST_ErrorCode_t h264par_SecureInit(const PARSER_Handle_t         ParserHandle,
		                          const PARSER_ExtraParams_t *  const SecureInitParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t *   PARSER_Data_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Set default values for pointers in ES Copy Buffer */
    PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p = SecureInitParams_p->BufferAddress_p;

    /* ES Copy buffer stands from ESCopy_Start_p to ESCopy_Stop_p inclusive */
    PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p = (void *)((U32)SecureInitParams_p->BufferAddress_p + SecureInitParams_p->BufferSize - 1);

    /* Reset ES Copy buffer write pointer */
    PARSER_Data_p->ESCopyBuffer.ESCopy_Write_p = PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p;

    return (ErrorCode);
} /* end of h264par_SecureInit() */
#endif /* DVD_SECURED_CHIP */

/*******************************************************************************/
/*! \brief Initialize the injection buffer
 *
 * Initialize the ES Bit Buffer pointers,
 * Inform FDMA about these buffers allocation
 *
 * \param ParserHandle Parser Handle
 * \param PARSER_ExtraParams_t ES bit buffer addresses + size
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
ST_ErrorCode_t h264par_BitBufferInjectionInit(const PARSER_Handle_t         ParserHandle,
		                                      const PARSER_ExtraParams_t *  const BitBufferInjectionParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t *   PARSER_Data_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Set default values for pointers in ES Buffer */
    PARSER_Data_p->BitBuffer.ES_Start_p = BitBufferInjectionParams_p->BufferAddress_p;
    /* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
    PARSER_Data_p->BitBuffer.ES_Stop_p = (void *)((U32)BitBufferInjectionParams_p->BufferAddress_p + BitBufferInjectionParams_p->BufferSize -1);
	/* Reset ES bit buffer write pointer */
	PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    return (ErrorCode);
} /* end of h264par_BitBufferInjectionInit() */

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
static ST_ErrorCode_t h264par_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Properties_t * PARSER_Properties_p;

    U32 UserDataSize;
	ST_ErrorCode_t ErrorCode;
    U32 SCList_Size;

	PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;

	if ((ParserHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

	/* Allocate PARSER PrivateData structure */
	PARSER_Properties_p->PrivateData_p = (H264ParserPrivateData_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(H264ParserPrivateData_t));
    if (PARSER_Properties_p->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    else
    {
        memset(PARSER_Properties_p->PrivateData_p, 0xA5, sizeof(H264ParserPrivateData_t)); /* fill in data with 0xA5 by default */
    }

    PARSER_Properties_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;

	/* Use the short cut PARSER_Data_p for next lines of code */

	PARSER_Data_p = (H264ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

	/* Allocate PARSER Job Results data structure */
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p =
			(VIDCOM_ParsedPictureInformation_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VIDCOM_ParsedPictureInformation_t));
	if (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p == NULL)
	{
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(ST_ERROR_NO_MEMORY);
	}
    else
    {
        memset(PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p, 0xA5, sizeof(VIDCOM_ParsedPictureInformation_t)); /* fill in data with 0xA5 by default */
    }

	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p =
			(VIDCOM_PictureDecodingContext_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VIDCOM_PictureDecodingContext_t));
	if (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p == NULL)
	{
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(ST_ERROR_NO_MEMORY);
	}
    else
    {
        memset(PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p, 0xA5, sizeof(VIDCOM_PictureDecodingContext_t)); /* fill in data with 0xA5 by default */
    }

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
    else
    {
        memset(PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p, 0xA5, sizeof(VIDCOM_GlobalDecodingContext_t)); /* fill in data with 0xA5 by default */
    }

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
    PARSER_Data_p->UserDataSize = UserDataSize;
    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_USER_DATA_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_USER_DATA_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module H264 parser init: could not register events !"));
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Initialise commands queue */
	PARSER_Data_p->ForTask.ControllerCommand = FALSE;

#if !defined ST_OSLINUX
    /* Allocation and Initialisation of injection buffers. The FDMA is also informed about these allocations */
    /* Compute the startcode list size according to the BitBuffer size */
    if (InitParams_p->BitBufferSize > BITBUFFER_SIZE_STANDARD)
    {
        SCList_Size = SCLIST_SIZE_STANDARD * ( InitParams_p->BitBufferSize / BITBUFFER_SIZE_STANDARD );
    }
    else
    {
        SCList_Size = ((SCLIST_SIZE_STANDARD * InitParams_p->BitBufferSize) + (BITBUFFER_SIZE_STANDARD - 1)) / BITBUFFER_SIZE_STANDARD ;
    }
    /* set a floor value equal to the default SC list size in case of too small bitbuffer */
    if (SCList_Size < SC_LIST_SIZE_IN_ENTRY)
    {
        SCList_Size = SC_LIST_SIZE_IN_ENTRY;
    }
#else
    SCList_Size = SC_LIST_SIZE_IN_ENTRY;
#endif /* ST_OSLINUX */

    ErrorCode = h264par_InitInjectionBuffers(ParserHandle, InitParams_p, SCList_Size, PES_BUFFER_SIZE);
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
    PARSER_Data_p->PreviousSCWasNotValid = FALSE;
    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_JOB_COMPLETED_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module H264 parser init: could not register events !"));
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_PICTURE_SKIPPED_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module H264 parser init: could not register events !"));
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

#ifdef STVID_TRICKMODE_BACKWARD
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_BITBUFFER_FULLY_PARSED_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module H264 parser init: could not register events !"));
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
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module H264 parser init: could not register events !"));
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

	/* Semaphore between parser and controller tasks */
	/* Parser main task does not run until it gets a start command from the controller */
    PARSER_Data_p->ParserOrder = STOS_SemaphoreCreateFifo(PARSER_Properties_p->CPUPartition_p, 0);
    PARSER_Data_p->ParserSC    = STOS_SemaphoreCreateFifoTimeOut(PARSER_Properties_p->CPUPartition_p, 0);


	PARSER_Data_p->ParserState = PARSER_STATE_IDLE;

    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;

	/* Create parser task */
	PARSER_Data_p->ParserTask.IsRunning = FALSE;
	ErrorCode = h264par_StartParserTask(ParserHandle);
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
    PARSER_Data_p->StopIsPending = FALSE;
	return (ErrorCode);
} /* End of PARSER_Init function */

/*******************************************************************************/
/*! \brief Set the Stream Type (PES or ES) for the PES parser
 * \param ParserHandle Parser handle,
 * \param StreamType ES or PES
 * \return void
 */
/*******************************************************************************/
void h264par_SetStreamType(const PARSER_Handle_t ParserHandle, const STVID_StreamType_t StreamType)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
void h264par_SetPESRange(const PARSER_Handle_t ParserHandle,
                         const PARSER_StartParams_t * const StartParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* FDMA setup ----------------  */
	h264par_SetStreamType(ParserHandle, StartParams_p->StreamType);

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
void h264par_InitParserContext(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	U32 i; /* loop index */

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->StartCode.IsPending = FALSE; /* No start code already found */

	PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable = FALSE;

    PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID = 0;

	for(i = 0; i < MAX_SPS; i++)
	{
		PARSER_Data_p->SPSLocalData[i].IsAvailable = FALSE;
	}

	for(i = 0; i < MAX_PPS; i++)
	{
		PARSER_Data_p->PPSLocalData[i].IsAvailable = FALSE;
	}

	/* No long term frame in DPBRef */
	PARSER_Data_p->ParserGlobalVariable.MaxLongTermFrameIdx = NO_LONG_TERM_FRAME_INDICES;

	/* Firt parsed picture will have a DecodingOrderFrameID = 0 */
	PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID = 0;

	for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
	{
		h264par_MarkFrameAsUnusedForReference((U8)i, ParserHandle);
	}

	#ifdef STVID_PARSER_CHECK
	/* No picture in ListD */
	PARSER_Data_p->ParserGlobalVariable.ListDNumberOfElements = 0;
	#endif

    /* Assume Frame detected at init */
    PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields = FALSE;
    PARSER_Data_p->PreviousPictureInDecodingOrder.IsFirstOfTwoFields = FALSE;

	/* Time code set to 0 */
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Seconds = 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Minutes = 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Hours = 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Frames = 0;

    /* No SEI message or UserDataso far */
    PARSER_Data_p->PictureLocalData.HasSEIMessage = FALSE;
    PARSER_Data_p->PictureLocalData.HasUserData = FALSE;
    /*  initialize SEI Messages and UserData Position list */
    PARSER_Data_p->SEIPositionArray.Iterator = 0;
    PARSER_Data_p->UserDataPositionArray.Iterator = 0;

    /* Reset UserData.IsReference, and associated fields */
    PARSER_Data_p->UserData.IsRegistered = FALSE;
    PARSER_Data_p->UserData.itu_t_t35_country_code = 0;
    PARSER_Data_p->UserData.itu_t_t35_country_code_extension_byte = 0;
    PARSER_Data_p->UserData.itu_t_t35_provider_code = 0;

	/* No Recovery point reached yet */
	PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointState = RECOVERY_NONE;
	PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.RecoveryPointHasBeenHit = FALSE;
    PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CheckPOC = FALSE;
    PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.CurrentPicturePOIDToBeSaved = FALSE;

	PARSER_Data_p->ParserGlobalVariable.RecoveryPoint.BrokenLinkFlag = FALSE;

	/* No AUD (Access Unit Delimiter) for next picture */
	PARSER_Data_p->PictureLocalData.PrimaryPicType = PRIMARY_PIC_TYPE_UNKNOWN;

	/* Main profile */
	PARSER_Data_p->ParserGlobalVariable.ProfileIdc = PROFILE_MAIN;

#ifdef ST_XVP_ENABLE_FGT
    h264par_ResetFgtParams(ParserHandle);
#endif /* ST_XVP_ENABLE_FGT */
}

/*******************************************************************************/
/*! \brief Sets the ES Range parameters to program the FDMA
 *
 * \param ParserHandle Parser handle
 * \return void
 */
/*******************************************************************************/
void h264par_SetESRange(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->BitBuffer.ES0RangeEnabled   = TRUE;
	PARSER_Data_p->BitBuffer.ES0RangeStart     = SMALLEST_NAL_PACKET_START_CODE;
	PARSER_Data_p->BitBuffer.ES0RangeEnd       = GREATEST_NAL_PACKET_START_CODE;
	PARSER_Data_p->BitBuffer.ES0OneShotEnabled = FALSE;

	/* Range 1 is not used for H264 */
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
static ST_ErrorCode_t h264par_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t            ErrorCode;
    VIDINJ_ParserRanges_t     ParserRanges[3];

/* TODO : Add processing of new StartParams_p data :
    BOOL                            OverwriteFullBitBuffer;
    void *                          BitBufferWriteAddress_p;
    VIDCOM_GlobalDecodingContext_t *  DefaultGDC_p;
    VIDCOM_PictureDecodingContext_t * DefaultPDC_p;
*/
#ifdef STVID_TRICKMODE_BACKWARD
	U32						CurrentEPOID, CurrentDOFID;
	CurrentDOFID = 0;
	CurrentEPOID = 0;
#endif

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	ErrorCode = ST_NO_ERROR;

	/* TODO: what do we do with BroadcastProfile ? */

    /* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Reset pointers in injection module */
    VIDINJ_TransferReset(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Save for future use: is inserted "as is" in the GlobalDecodingContextGenericData.SequenceInfo.StreamID structure */
    PARSER_Data_p->ParserGlobalVariable.StreamID = StartParams_p->StreamID;

    /* Save For future use in UserDataRegistered_ITU_t_35 SEI message parsing */
    PARSER_Data_p->ParserGlobalVariable.Broadcast_profile = StartParams_p->BroadcastProfile;

    /* Reset discontinuity indicator */
    PARSER_Data_p->DiscontinuityDetected = FALSE;

    /* FDMA Setup ----------------- */
    h264par_SetPESRange(ParserHandle, StartParams_p);

    h264par_SetESRange(ParserHandle);

    /* Specify the transfer buffers and parameters to the FDMA */
    /* cascade the buffers params and sc-detect config to inject sub-module */

	/* H264 SC Ranges*/
    ParserRanges[0].RangeId                     = STFDMA_DEVICE_H264_RANGE_0;
    ParserRanges[0].RangeConfig.RangeEnabled    = PARSER_Data_p->BitBuffer.ES0RangeEnabled;
    ParserRanges[0].RangeConfig.RangeStart      = PARSER_Data_p->BitBuffer.ES0RangeStart;
    ParserRanges[0].RangeConfig.RangeEnd        = PARSER_Data_p->BitBuffer.ES0RangeEnd;
    ParserRanges[0].RangeConfig.PTSEnabled      = FALSE;
    ParserRanges[0].RangeConfig.OneShotEnabled  = PARSER_Data_p->BitBuffer.ES0OneShotEnabled;

	/* ES Range 1 is not used in H264 */
	ParserRanges[1].RangeId                     = STFDMA_DEVICE_ES_RANGE_1;
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
                            PARSER_Data_p->PESBuffer.PESBufferTop_p
#if defined(DVD_SECURED_CHIP)
        /* In addition to passing ES_Start_p and ES_Stop_p to VIDINJ_TransferLimits, give it ESCopy_Start_p and ESCopy_Stop_p */
         /* ES Copy */     ,PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,
                            PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p
#endif /* DVD_SECURED_CHIP */
                           );

    STTBX_Print(("\nSTART ES buffer Base:%lx - Top:%lx\nSC buffer Base:%lx - Top:%lx\nPES buffer Base:%lx - Top:%lx\n",
             /* BB */       (unsigned long)(PARSER_Data_p->BitBuffer.ES_Start_p),
                            (unsigned long)(PARSER_Data_p->BitBuffer.ES_Stop_p),
         /* SC */           (unsigned long)(PARSER_Data_p->SCBuffer.SCList_Start_p),
                            (unsigned long)(PARSER_Data_p->SCBuffer.SCList_Stop_p),
             /* PES */      (unsigned long)(PARSER_Data_p->PESBuffer.PESBufferBase_p),
                            (unsigned long)(PARSER_Data_p->PESBuffer.PESBufferTop_p)));

    #if defined(DVD_SECURED_CHIP)
        STTBX_Print(("ESCopy buffer Base:%lx - Top:%lx\n",
             /* ES Copy */      (unsigned long)(PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p),
                                (unsigned long)(PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p)));
    #endif /* DVD_SECURED_CHIP */

    /* Ben : Parser Ranges will default to reset values */
	VIDINJ_SetSCRanges(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, 3, ParserRanges);

    /* Initialize the SC buffer pointers */
    h264par_InitSCBuffer(ParserHandle);

    /* Reset ES bit buffer write pointer */
    PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    #if defined(DVD_SECURED_CHIP)
        /* Reset ESCopyBuffer write pointer */
    	PARSER_Data_p->ESCopyBuffer.ESCopy_Write_p  = PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p;
    #endif  /* DVD_SECURED_CHIP */
	/* Run the FDMA */
    VIDINJ_TransferStart(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, StartParams_p->RealTime);

	/* End of FDMA setup ---------- */

	/* TODO: Default GDC and PDC table implementation */

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* Initialize the parser variables */
    if (StartParams_p->ResetParserContext)
    {
#ifdef STVID_TRICKMODE_BACKWARD
        if (PARSER_Data_p->Backward)
        {
            CurrentEPOID = PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID;
            CurrentDOFID = PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID;

            h264par_InitParserContext(ParserHandle);

            PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID = CurrentDOFID;
            PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID = CurrentEPOID;
        }
        else
#endif
        {
            h264par_InitParserContext(ParserHandle);
        }
    }
    PARSER_Data_p->ParserGlobalVariable.ErrorRecoveryMode = StartParams_p->ErrorRecoveryMode;

	PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;
    PARSER_Data_p->StopIsPending = FALSE;
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
void h264par_DMATransferDoneFct(U32 ParserHandle,void * ES_Write_p,
                               void * SCListWrite_p, void * SCListLoop_p
#if defined(DVD_SECURED_CHIP)
                               , void * ESCopy_Write_p
#endif  /* DVD_SECURED_CHIP */
                               )
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#ifdef STVID_TRICKMODE_BACKWARD
    if ((!PARSER_Data_p->Backward) || (PARSER_Data_p->StopParsing))
#endif /* STVID_TRICKMODE_BACKWARD */
    {
        PARSER_Data_p->BitBuffer.ES_Write_p     = ES_Write_p;
    	PARSER_Data_p->SCBuffer.SCList_Write_p  = SCListWrite_p;
    	PARSER_Data_p->SCBuffer.SCList_Loop_p   = SCListLoop_p;
#if defined(DVD_SECURED_CHIP)
    	PARSER_Data_p->ESCopyBuffer.ESCopy_Write_p  = ESCopy_Write_p;
        TrInject(("\r\nPARSER: Inject sends ESCopy_Write_p=0x%08x", ESCopy_Write_p));
#endif  /* DVD_SECURED_CHIP */
	}

    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
} /* End of h264par_DMATransferDoneFct() function */

/******************************************************************************/
/*! \brief Start the parser module
 *
 * Parses One picture: a GDC, PDC and full picture in bit buffer is needed to end up this command
 *
 * \param ParserHandle
 * \return ST_Error_Code
 */
/******************************************************************************/
static ST_ErrorCode_t h264par_GetPicture(const PARSER_Handle_t ParserHandle, const PARSER_GetPictureParams_t * const GetPictureParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 CurrentEPOID, CurrentDOFID;
    BOOL ParserAlreadyWorkingError=FALSE;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
        CurrentEPOID = PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID;
        CurrentDOFID = PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID;
		h264par_InitParserContext(ParserHandle);
        PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID = CurrentDOFID;
        PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID = CurrentEPOID + 1;
	}

    PARSER_Data_p->ParserGlobalVariable.ErrorRecoveryMode = GetPictureParams_p->ErrorRecoveryMode;
    PARSER_Data_p->ParserGlobalVariable.DecodedPictures = GetPictureParams_p->DecodedPictures;
	/* Fill job submission time */
	PARSER_Data_p->ParserJobResults.ParsingJobStatus.JobSubmissionTime = time_now();

    /* Lock access to ControllerCommand with TaskLock*/
	STOS_TaskLock();
    /* Status test is inside protected section to avoid testing between Notification to producer
       of previous picture and change of status variable.
       This code is executed in producer task context and ParserState change to PARSER_STATE_PARSING is
       done in parser task context
       Moved the STTBX_Report out of the TaskLock/Unlock */
    ParserAlreadyWorkingError = (PARSER_Data_p->ParserState == PARSER_STATE_PARSING);    /*report an error if the parser is already working */

    /* Wakes up Parser task */
	PARSER_Data_p->ForTask.ControllerCommand = TRUE;
    /* Unlock access to ParserState and ControllerCommand */
    STOS_TaskUnlock();

    if(ParserAlreadyWorkingError)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module H264 parser GetPicture: controller attempts to send a parser command when the parser is already working !"));
    }

    /* Wakes up Parser task */
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
ST_ErrorCode_t h264par_StartParserTask(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    VIDCOM_Task_t * const Task_p = &(PARSER_Data_p->ParserTask);
    char TaskName[25] = "STVID[?].H264ParserTask";

	if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + ((PARSER_Properties_t *) ParserHandle)->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

    /* TODO: define PARSER own priority in vid_com.h */
    if (STOS_TaskCreate((void (*) (void*)) h264par_ParserTaskFunc,
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
ST_ErrorCode_t h264par_StopParserTask(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
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
static ST_ErrorCode_t h264par_Term(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	PARSER_Properties_t     * PARSER_Properties_p;

	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	PARSER_Data_p = (H264ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

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

	/* TODO: is task "InjecterTask" also stopped anywhere ? */

    /* Terminate the parser task */
    h264par_StopParserTask(ParserHandle);
	/* Delete semaphore between parser and controler tasks */
    STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ParserOrder);
	/* Delete semaphore for SC lists */
    STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ParserSC);
    /* Deallocate the STAVMEM partitions */
	/* PES Buffer */
	if(PARSER_Data_p->PESBuffer.PESBufferBase_p != 0)
    {
        /* ErrorCode = */ h264par_DeAllocatePESBuffer(ParserHandle);
    }
	/* SC Buffer */
	if(PARSER_Data_p->SCBuffer.SCList_Start_p != 0)
    {
        /* ErrorCode = */ h264par_DeAllocateSCBuffer(ParserHandle);
    }

	/* De-allocate memory allocated at init */
    if (PARSER_Data_p->UserData.Buff_p != NULL)
    {
        STOS_MemoryDeallocate(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
    }
    PARSER_Data_p->UserDataSize = 0;

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
static ST_ErrorCode_t h264par_Stop(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* Stops the FDMA */
    VIDINJ_TransferStop(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    PARSER_Data_p->StopIsPending = TRUE;

    return (ErrorCode);
} /* end of h264par_Stop() */

#ifdef STVID_DEBUG_GET_STATISTICS
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
static ST_ErrorCode_t h264par_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;
    ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    VIDINJ_GetStatistics(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, Statistics_p);

    /* Statistics_p->??? = PARSER_Data_p->Statistics.???; */

    return (ErrorCode);
} /* end of h264par_GetStatistics() */

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
static ST_ErrorCode_t h264par_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    VIDINJ_ResetStatistics(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, Statistics_p);

	/* Reset parameters that are said to be reset (giving value != 0) */
    /* if (Statistics_p->??? != 0)
    {
        PARSER_Data_p->Statistics.??? = 0;
    }
	*/
	return (ErrorCode);
} /* end of h264par_ResetStatistics() */
#endif /* STVID_DEBUG_GET_STATISTICS */

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
static ST_ErrorCode_t h264par_GetStatus(const PARSER_Handle_t ParserHandle, STVID_Status_t * const Status_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    VIDINJ_GetStatus(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, Status_p);

    /* Status_p->??? = PARSER_Data_p->Status.???; */

    return (ErrorCode);
} /* end of h264par_GetStatus() */
#endif /* STVID_DEBUG_GET_STATUS */


/*******************************************************************************/
/*! \brief Point on Next Entry in Start Code List
 *
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void h264par_PointOnNextSC(const PARSER_Handle_t ParserHandle)
{
    /* Behavior of the SC List filling:
    * SCListWrite is incremented by burst each time a new SCEntry is written
    * (burst size is the number of SC found within 128 Byte of stream)
    * Before running the FDMA, the injection checks whether SCListWrite is not too closed from SCList_Stop_p
    *    - If the remaining space is too low:
    *       - SCListLoop is set to be equal to SCListWrite (means that the upper
    *         boundary is no longer SCListStop but SCListLoop)
    *       - SCListWrite is set to SCListStart (start filling the SC List from the beginning)
    *    - If there is enough remaining space: SCListLoop is set to be equal to SCListWrite all along
    *      the filling process
    */

    /* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->SCBuffer.NextSCEntry ++;

    if((PARSER_Data_p->SCBuffer.NextSCEntry == PARSER_Data_p->SCBuffer.LatchedSCList_Loop_p)
        &&(PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.LatchedSCList_Write_p))
    {
        #ifdef STVID_PARSER_DO_NOT_LOOP_ON_STREAM
        PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.LatchedSCList_Write_p;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PARSER : Reached end of Stream"));
        #else
        /* Reached top of SCList */
        PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Start_p;
        #endif
    }


#ifdef ST_speed
        PARSER_Data_p->LastSCAdd_p = (void*)(PARSER_Data_p->StartCode.StartAddress_p);
#endif /* ST_speed */

} /* End of h264par_PointOnNextSC() function */

/*******************************************************************************/
/*! \brief Get the next Start Code to process
 *
 * If a start code is found, the read pointer in SC+PTS buffer points to the entry
 * following the start code.
 *
 * \param ParserHandle
 * \return PARSER_Data_p->StartCode.IsPending TRUE if start code found
 */
/*******************************************************************************/
void h264par_GetNextStartCodeToProcess(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t * PARSER_Properties_p;
	BOOL SCFound;
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.LatchedSCList_Write_p)
    {
		SCFound = FALSE;
	    /* search for available SC in SC+PTS buffer : */
        while((PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.LatchedSCList_Write_p) && (!(SCFound)) && (!PARSER_Data_p->StopIsPending))
	    {
#ifdef ST_5100
            /* This chip dependency should be removed once addresses management is generalized */
            CPUNextSCEntry_p = PARSER_Data_p->SCBuffer.NextSCEntry;
#else
            CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
#endif
            if(!IsPTS(CPUNextSCEntry_p))
			{
                #if defined(DVD_SECURED_CHIP)
                /* According to the fdma team, "The ESCopy Write Address will be set to Zero if the SC found is not
                the first slice start code". So we ignore the entry with the NULL ES Copy value and jump to the next. */
                if (CPUNextSCEntry_p->SC.Addr2 != NULL)
                #endif /* DVD_SECURED_CHIP */
                {
    				/* Get the Start Code itself */
    				PARSER_Data_p->StartCode.Value = GetSCVal(CPUNextSCEntry_p);
                    if (h264par_CheckNextSCValid(ParserHandle))
                    {
                        SCFound = TRUE;

        				#if defined(DVD_SECURED_CHIP)
            				/* Set the Parser Read Pointer in ES Copy Buffer on the start address of the start code */
            				PARSER_Data_p->StartCode.StartAddress_p = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
            				/* Set the Parser Read Pointer in ES Bit Buffer on the start address of the start code */
            				PARSER_Data_p->StartCode.BitBufferStartAddress_p = (U8 *)GetSCAddForBitBuffer(CPUNextSCEntry_p);
            			#else
            				/* Set the Parser Read Pointer in ES Bit Buffer on the start address of the start code */
            				PARSER_Data_p->StartCode.StartAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
        				#endif /* DVD_SECURED_CHIP */

        				/* A valid current start code is available */
        				PARSER_Data_p->StartCode.IsPending = TRUE;
        				PARSER_Data_p->StartCode.IsInBitBuffer = FALSE; /* The parser must now look for the next start code to know if the NAL is fully in the bit buffer */

        				PARSER_Data_p->SCBuffer.CurrentSCEntry = STAVMEM_CPUToDevice(CPUNextSCEntry_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    				}
                    else /* h264par_SCValid(ParserHandle, PARSER_Data_p->StartCode.Value) */
                    {
        				SCFound = FALSE;
                        PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
                        STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData));
                    }
	            }
	        }
            else
            {
                h264par_SavePTSEntry(ParserHandle, CPUNextSCEntry_p);
            }

            #if defined(DVD_SECURED_CHIP)
                TrStartCode(("\r\nSC Off=0x%08x (%u) - Type: %d",
                            (U32)PARSER_Data_p->StartCode.StartAddress_p - (U32)PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,
                            (U32)PARSER_Data_p->StartCode.StartAddress_p - (U32)PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,
                            GetNalUnitType((U32)PARSER_Data_p->StartCode.Value)));
            #endif /* DVD_SECURED_CHIP */

			/* Make ScannedEntry_p point on the next entry in SC List */
			h264par_PointOnNextSC(ParserHandle);
        } /* end while */
	}
} /* End of h264par_GetNextStartCodeToProcess() function */
/*******************************************************************************/
/*! \brief Check if the NAL is fully in bit buffer: this is mandatory to process it.
 *
 *
 * \param ParserHandle
 * \return PARSER_Data_p->StartCode.IsInBitBuffer TRUE if fully in bit buffer
 */
/*******************************************************************************/
void h264par_IsNALInBitBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t   * PARSER_Data_p;
    STFDMA_SCEntry_t          * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
	U8                          NalUnitType;
	BOOL                        SCFound = FALSE;

    PARSER_Properties_t * PARSER_Properties_p;
    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;


	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Saves the NextSCEntry before proceeding over the SC+PTS buffer */
	PARSER_Data_p->SCBuffer.SavedNextSCEntry_p = PARSER_Data_p->SCBuffer.NextSCEntry;

	if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.LatchedSCList_Write_p)
    {
		SCFound = FALSE;
	    /* search for available SC in SC+PTS buffer : */
        while((PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.LatchedSCList_Write_p) && (!(SCFound)) && (!PARSER_Data_p->StopIsPending))
	    {
#ifdef ST_5100
            /* This chip dependency should be removed once addresses management is generalized */
            CPUNextSCEntry_p = PARSER_Data_p->SCBuffer.NextSCEntry;
#else
            CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
#endif
			if(!IsPTS(CPUNextSCEntry_p))
			{
				/* when the start code to process is not a slice start code, having any start code stored after in the
				 * SC+PTS buffer is enough to say that the start code to process is fully in the bit buffer
				 */

                if (GetNalUnitType(PARSER_Data_p->StartCode.Value) == NAL_UNIT_DISCONTINUITY)
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

                if ((GetNalUnitType(PARSER_Data_p->StartCode.Value) != NAL_UNIT_TYPE_SLICE) &&
                    (GetNalUnitType(PARSER_Data_p->StartCode.Value) != NAL_UNIT_TYPE_IDR) &&
                    (GetNalUnitType(PARSER_Data_p->StartCode.Value) != NAL_UNIT_TYPE_AUX_SLICE))
				{
					SCFound = TRUE; /* to exit of the loop */
					PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;

    				#if defined(DVD_SECURED_CHIP)
        				/* Stop address of NAL to process is the start address of the following NAL */
        				/* Set the Parser Read Pointer in ES Copy Buffer */
        				PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
        				/* Set the Parser Read Pointer in bit buffer */
        				PARSER_Data_p->StartCode.BitBufferStopAddress_p = (U8 *)GetSCAddForBitBuffer(CPUNextSCEntry_p);
        				TrStartCode(("\r\nSC found: no Slice in ESCopy @0x%08x", PARSER_Data_p->StartCode.StopAddress_p));
        			#else
        				/* Stop address of NAL to process is the start address of the following NAL */
        				PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
        	        #endif /* DVD_SECURED_CHIP */
				}
				else
				{
					/* The start code to process is a slice start code. Before processing it, the parser must ensure the
					 * picture is fully in the bit buffer.
					 * The picture is fully in the bit buffer when any of the following start code are encountered after
					 * the picture start code:
					 * 	NAL_UNIT_TYPE_SEI
					 *	NAL_UNIT_TYPE_AD
					 *	NAL_UNIT_TYPE_EOSEQ
					 * 	NAL_UNIT_TYPE_EOSTREAM
					 *	NAL_UNIT_TYPE_SLICE or NAL_UNIT_TYPE_IDR (FDMA reports only the first of a picture)
                     *  NAL_UNIT_TYPE_14 to 18
					 *
					 *
					 * Note: SPS and PPS may be inserted inside a picture, thus they cannot be used to detect the end of picture
					 */
					NalUnitType = GetNalUnitType(GetSCVal(CPUNextSCEntry_p));
					if ((NalUnitType == NAL_UNIT_TYPE_SEI) ||
						(NalUnitType == NAL_UNIT_TYPE_AUD) ||
						(NalUnitType == NAL_UNIT_TYPE_EOSEQ) ||
						(NalUnitType == NAL_UNIT_TYPE_EOSTREAM) ||
						(NalUnitType == NAL_UNIT_TYPE_SLICE) ||
						(NalUnitType == NAL_UNIT_TYPE_IDR) ||
						(NalUnitType == NAL_UNIT_TYPE_14) ||
						(NalUnitType == NAL_UNIT_TYPE_15) ||
						(NalUnitType == NAL_UNIT_TYPE_16) ||
						(NalUnitType == NAL_UNIT_TYPE_17) ||
						(NalUnitType == NAL_UNIT_TYPE_18) ||
						(NalUnitType == NAL_UNIT_DISCONTINUITY)
					   )
					{
						/* The picture is fully in the bit buffer, thus it can be processed */
						SCFound = TRUE; /* to exit the loop */
						PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;

        				#if defined(DVD_SECURED_CHIP)
        				    /* If the picture is fully in ES Copy buffer, then it is also fully in bit buffer */
            				/* as bit buffer is the original data from which the ES Copy buffer is copied. */

            				/* Stop address of NAL to process is the start address of the following NAL */
            				/* Set the Parser Read Pointer in ES Copy Buffer */
            				PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
            				/* Set the Parser Read Pointer in bit buffer */
            				PARSER_Data_p->StartCode.BitBufferStopAddress_p = (U8 *)GetSCAddForBitBuffer(CPUNextSCEntry_p);
            				TrStartCode(("\r\nSC found: Slice in ESCopy @0x%08x", PARSER_Data_p->StartCode.StopAddress_p));
            			#else
    						/* Stop address of NAL to process is the start address of the following NAL */
    						PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p);
            	        #endif /* DVD_SECURED_CHIP */

                        if (NalUnitType == NAL_UNIT_DISCONTINUITY)
                        {
                            /* Discontinuity detected, inform driver */
                            PARSER_Data_p->DiscontinuityDetected = TRUE;
                        }
                    }
				}
	        }
			/* Make ScannedEntry_p point on the next entry in SC List */
			h264par_PointOnNextSC(ParserHandle);
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
}   /* End of h264par_IsNALInBitBuffer() function */

/*******************************************************************************/
/*! \brief Check whether there is a Start Code to process
 *
 * If yes, a start code is pushed and a ParserOrder is sent (semaphore)
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void h264par_IsThereStartCodeToProcess(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Properties_t     * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    STFDMA_SCEntry_t        * CheckSC;
    BOOL                      NotifyBitBufferFullyParsed;
#endif /*STVID_TRICKMODE_BACKWARD*/

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Test if a new start-code is available */
    /*---------------------------------------*/

    /* Latch value for all following treatment. SCList_Write_p and SCList_Loop_p may change but value latched are enough.*/
    /* use TaskLock_Unlock to make sure both are updated atomically
       this is to avoid conditions where only 1 of the 2 variables is updated when looping,
       it could introduce a latency in the parsing, as the injected data would be treated the next time the parser is running
       */
    STOS_TaskLock();
    PARSER_Data_p->SCBuffer.LatchedSCList_Loop_p  = PARSER_Data_p->SCBuffer.SCList_Loop_p  ;
    PARSER_Data_p->SCBuffer.LatchedSCList_Write_p = PARSER_Data_p->SCBuffer.SCList_Write_p ;
    STOS_TaskUnlock();

    if (!(PARSER_Data_p->StartCode.IsPending)) /* Look for a start code to process */
	{
		h264par_GetNextStartCodeToProcess(ParserHandle);

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

            if (NotifyBitBufferFullyParsed)
            {
                PARSER_Data_p->IsBitBufferFullyParsed = TRUE;
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

                /* Bit Buffer is fully parsed: stop parsing ... */
                PARSER_Data_p->StopParsing = TRUE;
            }
        }
#endif
		/* PARSER_Data_p->StartCode.IsPending is modified in h264par_GetNextStartCodeToProcess() */
	}

    if (PARSER_Data_p->StartCode.IsPending) /* Look if the NAL is fully in bit buffer, now */
    {
        h264par_IsNALInBitBuffer(ParserHandle);
    }

	/* inform inject module about the SC-list read progress */
    VIDINJ_TransferSetSCListRead(PARSER_Data_p->Inject.InjecterHandle,
                                 PARSER_Data_p->Inject.InjectNum,
                                 PARSER_Data_p->SCBuffer.NextSCEntry);

    TrStartCode(("\r\nParToInj: SC=0x%08x", PARSER_Data_p->SCBuffer.NextSCEntry));

} /* End of h264par_IsThereStartCodeToProcess() function */

/******************************************************************************/
/*! \brief Initialize the NALByteStream data structure on current NAL
 *
 * \param NALStartAddressInBB_p the start address in memory for this entry
 * \param NALStopAddressInBB_p the start address in memory for this entry
 * \param pp the parser variables (updated)
 * \return 0
 */
/******************************************************************************/
void h264par_InitNALByteStream(StartCode_t * StartCode_p, const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	U8 * StartCodeValueAddress = StartCode_p->StartAddress_p;
	U8 * NextPictureStartCodeValueAddress = StartCode_p->StopAddress_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if (NextPictureStartCodeValueAddress > StartCodeValueAddress)
	{
        PARSER_Data_p->NALByteStream.Len = NextPictureStartCodeValueAddress - StartCodeValueAddress;
    }
	else
	{
	    #if defined(DVD_SECURED_CHIP)
    		/* ES Copy Buffer wrap */
            PARSER_Data_p->NALByteStream.Len = PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p
                                               - StartCodeValueAddress
                                               + NextPictureStartCodeValueAddress
                                               - PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p
                                               + 1;
	    #else
		/* Bit Buffer wrap */
        PARSER_Data_p->NALByteStream.Len = PARSER_Data_p->BitBuffer.ES_Stop_p
                                           - StartCodeValueAddress
                                           + NextPictureStartCodeValueAddress
                                           - PARSER_Data_p->BitBuffer.ES_Start_p
                                           + 1;
        #endif /* DVD_SECURED_CHIP */
	}
	PARSER_Data_p->NALByteStream.BitOffset = 7; /* ready to read the MSB of the first byte */
    PARSER_Data_p->NALByteStream.ByteOffset = 0; /* ready to read first byte */

#ifdef BYTE_SWAP_SUPPORT
	PARSER_Data_p->NALByteStream.ByteSwapFlag = FALSE;
	PARSER_Data_p->NALByteStream.WordSwapFlag = FALSE;
	if (PARSER_Data_p->NALByteStream.WordSwapFlag == FALSE)
	{
	    PARSER_Data_p->NALByteStream.ByteOffset = 0; /* ready to read first byte */
	}
	else
	{
		PARSER_Data_p->NALByteStream.ByteOffset = 3; /* ready to read first byte */
	}
#endif /*BYTE_SWAP_SUPPORT*/

	PARSER_Data_p->NALByteStream.EndOfStreamFlag = FALSE;
	PARSER_Data_p->NALByteStream.LastByte = 0xFF; /* do not initialize to 0 to avoid wrong detection on first bytes */
	PARSER_Data_p->NALByteStream.LastButOneByte = 0xFF; /* do not initialize to 0 to avoid wrong detection on first bytes */
	PARSER_Data_p->NALByteStream.ByteCounter = 0; /* beginning of stream */

    PARSER_Data_p->NALByteStream.RBSPStartByte_p = StartCodeValueAddress;

    /* The stream is initialized on the byte following the start code value (returned by the FDMA) */
    h264par_MoveToNextByteFromNALUnit(ParserHandle);
}

/*******************************************************************************/
/*! \brief Process the start code
 *
 * At that stage, the full NAL unit is in the bit buffer
 *
 * \param ParserHandle
 * \param StartCode
 * \param NALStartAddress in bit buffer
 * \param NALStopAddress in bit buffer
 */
/*******************************************************************************/
ST_ErrorCode_t h264par_ProcessStartCode(StartCode_t * StartCode_p, const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    PARSER_Properties_t * PARSER_Properties_p;
	H264ParserPrivateData_t * PARSER_Data_p;
	U8 NalUnitType;
	U8 NalRefIdc;
#ifdef STVID_OPTIMIZE_PARSER_SKIP_NONREF_PICTURES
    BOOL SkipPicture;
#endif /* STVID_OPTIMIZE_PARSER_SKIP_NONREF_PICTURES */
    U8 SliceType;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Initialize the NALByteStream_t data structure to parse the SC entry in ES buffer */
    h264par_InitNALByteStream(StartCode_p, ParserHandle);

	NalRefIdc = GetNalRefIdc(StartCode_p->Value);
	NalUnitType = GetNalUnitType(StartCode_p->Value);

    switch(NalUnitType)
	{
		case NAL_UNIT_TYPE_SLICE:
		case NAL_UNIT_TYPE_IDR:
            PARSER_Data_p->LastSCAddBeforeSEI_p   = (void*)(PARSER_Data_p->StartCode.StopAddress_p);
            h264par_GetSliceTypeFromSliceHeader(ParserHandle, &SliceType);
#ifdef STVID_OPTIMIZE_PARSER_SKIP_NONREF_PICTURES
            SkipPicture = FALSE;
            /* Skip non ref picture parsing if non ref picture skip is requested by API level */
            if ((NalRefIdc == 0) && (PARSER_Data_p->ParserGlobalVariable.DecodedPictures != STVID_DECODED_PICTURES_ALL))
            {
                SkipPicture = TRUE;
            }
            /* Skip non I picture parsing if I only is requested by API level */
            if (PARSER_Data_p->ParserGlobalVariable.DecodedPictures == STVID_DECODED_PICTURES_I)
            {
                if ((PARSER_Data_p->PictureLocalData.PrimaryPicType != PRIMARY_PIC_TYPE_I) && (PARSER_Data_p->PictureLocalData.PrimaryPicType != PRIMARY_PIC_TYPE_ISI) && (SliceType != ALL_SLICE_TYPE_I))
                {
                    SkipPicture = TRUE;
                }
            }
            if (SkipPicture)
            {
                    /* WARNING, at this stage only picture's start & stop
                     * addresses in the bit buffer are valid data, all remaining
                     * of the picture's data might be unrelevant because picture
                     * parsing  is not complete */
                    PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress = PARSER_Data_p->StartCode.StopAddress_p;
                    STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData));
                    PARSER_Data_p->PictureLocalData.HasSEIMessage = FALSE;
                    PARSER_Data_p->PictureLocalData.HasUserData = FALSE;
                    PARSER_Data_p->PictureLocalData.PrimaryPicType = PRIMARY_PIC_TYPE_UNKNOWN;
                    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);

                    return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#endif /* STVID_OPTIMIZE_PARSER_SKIP_NONREF_PICTURES */
			/* the NAL is a slice of a picture */
            h264par_ParseSliceHeader(NalRefIdc, NalUnitType, StartCode_p, ParserHandle, SliceType);

			break;

		case NAL_UNIT_TYPE_SPS:
			/* the NAL is a SPS */
			h264par_ParseSPS(ParserHandle);
			break;

        case NAL_UNIT_TYPE_SPS_EXT:
            /* the NAL is a SPS extension (high profile) */
            h264par_ParseSPSExtension(ParserHandle);
            break;

		case NAL_UNIT_TYPE_PPS:
			/* the NAL is a PPS */
			h264par_ParsePPS(ParserHandle);
			break;

		case NAL_UNIT_TYPE_SEI:
            /* the NAL is a SEI, parsing is deferred to the next picture parsing because some SEI need data from sps and/or pps */
            h264par_PushSEIListItem(ParserHandle);
			break;

        case NAL_UNIT_TYPE_AUX_SLICE:
            /* the NAL is an Auxiliary Coded Picture Slice */
            /* STTBX_Report((STTBX_REPORT_LEVEL_WARNING, "Module H264 parser h264par_ProcessStartCode: Auxiliary Coded Picture Slice found and discarded because not supported !")); */
            break;
		case NAL_UNIT_TYPE_AUD:
			/* the NAL is a AUD */
			h264par_ParseAUD(ParserHandle);

		default:
			/* Do not process the other NAL units */
            break;
	}
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
    return ST_NO_ERROR;
}
/*******************************************************************************/
/*! \brief Update PPS Scanlist from SPS ScanLists using Fall Back A or B rules
 *
 * Update PPS Scanlist from SPS ScanLists using Fall Back A or B rules with
 * respect to H264 high profile JVT-L047d12 "30) subclause 7.4.2.2"
 *
 * \param GlobalDecodingContextSpecificData associate to PPS, to get Scanlists
 *           for fall-back rule B
 * \param PictureDecodingContextSpecificData to update
 */
/*******************************************************************************/
static void UpdatePPSScanLists(H264_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData, H264_PictureDecodingContextSpecificData_t *PictureDecodingContextSpecificData)
{
    U32 NumberOfScalingLists;
    U32 ScalingListNumber;

    if(!PictureDecodingContextSpecificData->pic_scaling_matrix_present_flag)
    {   /* If no Scaling Matrix in PPS from stream, use Scaling Matrix from SPS */
        STOS_memcpy(&(PictureDecodingContextSpecificData->ScalingMatrix), &(GlobalDecodingContextSpecificData->ScalingMatrix), sizeof(PictureDecodingContextSpecificData->ScalingMatrix));
    }
    else
    {   /* Scaling Matrix present in PPS, for each list apply FallBack rules A or B if needed */
        if(PictureDecodingContextSpecificData->transform_8x8_mode_flag)
        {
            NumberOfScalingLists = 8;
        }
        else
        {
            NumberOfScalingLists = 6;
        }
        for(ScalingListNumber = 0 ; ScalingListNumber < NumberOfScalingLists ; ScalingListNumber++)
        {
            if(!PictureDecodingContextSpecificData->ScalingMatrix.ScalingListPresentFlag[ScalingListNumber])
            {   /* If Scaling List not in stream, appply Fall Back rule */
                if(GlobalDecodingContextSpecificData->seq_scaling_matrix_present_flag)
                {   /* Apply Fall Back rule B */
                    h264par_SetScalingListFallBackB(&(PictureDecodingContextSpecificData->ScalingMatrix) ,&(GlobalDecodingContextSpecificData->ScalingMatrix), ScalingListNumber);
                }
                else
                {   /* Apply Fall Back Rule A */
                    h264par_SetScalingListFallBackA(&(PictureDecodingContextSpecificData->ScalingMatrix), ScalingListNumber);
                }
            }
        }
    }
}

/*******************************************************************************/
/*! \brief Fill the Parsing Job Result Data structure
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void h264par_FillParsingJobResult(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	BOOL PictureHasError;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PictureHasError = FALSE;

    if ((PARSER_Data_p->DiscontinuityDetected) && (PARSER_Data_p->PictureGenericData.ParsingError < VIDCOM_ERROR_LEVEL_BAD_DISPLAY))
    {
        PARSER_Data_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
    }

    if(!PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->ScalingMatrixUpdatedFromSPS)
    {   /* If FallBack/Default rules have not yet been aplied on this PPS by a previous picture, apply then */
        UpdatePPSScanLists(PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p, PARSER_Data_p->ActivePictureDecodingContextSpecificData_p);
        PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->ScalingMatrixUpdatedFromSPS = TRUE;
    }

	/* Global Decoding Context structure */

    STOS_memcpy(&(PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData), PARSER_Data_p->ActiveGlobalDecodingContextGenericData_p, sizeof(VIDCOM_GlobalDecodingContextGenericData_t));
    /* Copy the actual frame rate in sequence level information because GlobalDecodingContextGenericData.SequenceInfo.FrameRate contains only the picture rate read from the SPS VUI params */
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate;
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->SizeOfGlobalDecodingContextSpecificDataInByte = sizeof(H264_GlobalDecodingContextSpecificData_t);
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p = PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p;

	/* Picture Decoding Context structure */
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->SizeOfPictureDecodingContextSpecificDataInByte = sizeof(H264_PictureDecodingContextSpecificData_t);
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p = PARSER_Data_p->ActivePictureDecodingContextSpecificData_p;

	/* Parsed Picture Information structure */
    STOS_memcpy(&(PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData), &(PARSER_Data_p->PictureGenericData), sizeof(PARSER_Data_p->PictureGenericData));
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->SizeOfPictureSpecificDataInByte = sizeof(H264_PictureSpecificData_t);
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureSpecificData_p = &(PARSER_Data_p->PictureSpecificData);

#ifdef ST_XVP_ENABLE_FGT
    /*Film Grain message*/
    STOS_memcpy( &(PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureFilmGrainSpecificData),
                 &(PARSER_Data_p->FilmGrainSpecificData), sizeof(VIDCOM_FilmGrainSpecificData_t));
#endif /* ST_XVP_ENABLE_FGT */

	/* Parsing Job Status */

	/* Check whether picture has errors */
	/* Checks on SPS */
    if (PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.MaxBytesPerPicDenomRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.MaxBitsPerMBDenomError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.Log2MaxMvLengthHorizontalRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.Log2MaxMvLengthVerticalRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.NumReorderFrameRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.MaxDecBufferingRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.IsNotSupportedProfileError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.ReservedZero4BitsError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.LevelIdcRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.Log2MaxFrameNumMinus4RangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.Log2MaxPicOrderCntLsbMinus4RangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.NumRefFramesRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.Direct8x8InferenceFlagNullWithFrameMbsOnlyFlagError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.TruncatedNAL ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.NalCpbCntMinus1RangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.NalBitRateRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.NalCpbSizeRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.NonIncreasingNalBitRateError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.NonIncreasingNalCpbSizeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.VclCpbCntMinus1RangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.VclBitRateRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.VclCpbSizeRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.NonIncreasingVclBitRateError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.NonIncreasingVclCpbSizeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.InitialCpbRemovalDelayDifferError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.CpbRemovalDelayLengthDifferError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.DpbOutputDelayLengthDifferError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.TimeOffsetLengthDifferError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.LowDelayHrdFlagNotNullError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.WidthRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.HeightRangeError ||
        PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPSError.Direct8x8InferenceFlagNullWithLevel3OrMoreError
	   )
	{
		PictureHasError = TRUE;
	}

	/* PPS */
    if (PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->PPSError.NumSliceGroupsMinus1NotNullError ||
        PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->PPSError.NumRefIdxActiveMinus1RangeError ||
        PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->PPSError.WeightedBiPredIdcRangeError ||
        PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->PPSError.PicInitQpMinus26RangeError ||
        PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->PPSError.PicInitQsMinus26RangeError ||
        PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->PPSError.ChromaQpIndexOffsetRangeError ||
        PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->PPSError.TruncatedNAL
	   )
	{
		PictureHasError = TRUE;
	}

	/* Slice: */
	if (PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NalRefIdcNullForIDRError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NotTheFirstSliceError  ||
    	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NoPPSAvailableError  ||
    	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NoSPSAvailableError  ||
    	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.UnintentionalLossOfPictureError  ||
    	PARSER_Data_p->PictureSpecificData.PictureError.SliceError.DeltaPicOrderCntBottomRangeError  ||
		PARSER_Data_p->PictureSpecificData.PictureError.SliceError.PicOrderCntType2SemanticsError ||
		PARSER_Data_p->PictureSpecificData.PictureError.SliceError.LongTermReferenceFlagNotNullError ||
		PARSER_Data_p->PictureSpecificData.PictureError.SliceError.MaxLongTermFrameIdxPlus1RangeError ||
		PARSER_Data_p->PictureSpecificData.PictureError.SliceError.DuplicateMMCO4Error ||
		PARSER_Data_p->PictureSpecificData.PictureError.SliceError.MMCO1OrMMCO3WithMMCO5Error ||
		PARSER_Data_p->PictureSpecificData.PictureError.SliceError.DuplicateMMCO5Error ||
		PARSER_Data_p->PictureSpecificData.PictureError.SliceError.IdrPicIdViolationError ||
		PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NumRefIdxActiveMinus1RangeError ||
		PARSER_Data_p->PictureSpecificData.PictureError.SliceError.NumRefIdxActiveOverrrideFlagNullError ||
        PARSER_Data_p->PictureSpecificData.PictureError.SliceError.SameParityOnComplementaryFieldsError ||

    	PARSER_Data_p->PictureSpecificData.PictureError.POCError.IDRWithPOCNotNullError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.POCError.PicOrderCntTypeRangeError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.POCError.ListDOverFlowError ||
		PARSER_Data_p->PictureSpecificData.PictureError.POCError.FieldOrderCntNotConsecutiveInListOError ||
		PARSER_Data_p->PictureSpecificData.PictureError.POCError.DuplicateTopFieldOrderCntInListOError ||
		PARSER_Data_p->PictureSpecificData.PictureError.POCError.DuplicateBottomFieldOrderCntInListOError ||
		PARSER_Data_p->PictureSpecificData.PictureError.POCError.NonPairedFieldsShareSameFieldOrderCntInListOError ||
		PARSER_Data_p->PictureSpecificData.PictureError.POCError.DifferenceBetweenPicOrderCntExceedsAllowedRangeError ||

    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.MarkInUsedFrameBufferError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.DPBRefFullError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortTermToSlideError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortTermToMarkAsUnusedError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoLongTermToMarkAsUnusedError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.MarkNonExistingAsLongError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoShortPictureToMarkAsLongError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.Marking2ndFieldWithDifferentLongTermFrameIdxError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.NoLongTermAssignmentAllowedError ||
    	PARSER_Data_p->PictureSpecificData.PictureError.MarkingError.LongTermFrameIdxOutOfRangeError ||
		PARSER_Data_p->PictureSpecificData.PictureError.TruncatedNAL
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
	PARSER_Data_p->ParserJobResults.ParsingJobStatus.JobCompletionTime = time_now();
#ifdef ST_speed
    PARSER_Data_p->ParserJobResults.DiffPSC = 0;
#endif
}

/*******************************************************************************/
/*! \brief Check whether an event "job completed" is to be sent to the controller
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void h264par_CheckEventToSend(const PARSER_Handle_t ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t   * PARSER_Data_p;
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    U8                          NalUnitType;
#ifdef STVID_TRICKMODE_BACKWARD
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
#endif
    PARSER_Data_p = (H264ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

    /* The event shall not be sent if there is a pending h264par_Stop in lieu of the vidprod_stop() */
    if (PARSER_Data_p->StopIsPending)
    {
    	return;
    }
    /* An event "job completed" is raised whenever a new picture has been parsed.
    * (and in case a new picture is starting, the parser shall not parse anything
    *  not to modify the parameters from the previously parsed picture)
    *
    */

    /* Check whether the current start code starts a picture */
    NalUnitType = GetNalUnitType(PARSER_Data_p->StartCode.Value);
    if ((NalUnitType == NAL_UNIT_TYPE_SLICE) || (NalUnitType == NAL_UNIT_TYPE_IDR))
    {
        if ((PARSER_Data_p->ParserState == PARSER_STATE_PARSING) &&   /* Controller is waiting an event */
            (PARSER_Data_p->PictureLocalData.IsValidPicture))
        {
#ifdef STVID_TRICKMODE_BACKWARD
            if ((((PARSER_Data_p->PictureLocalData.IsRandomAccessPoint == PARSER_Data_p->GetPictureParams.GetRandomAccessPoint) ||
                  ((PARSER_Data_p->SkipMode == STVID_DECODED_PICTURES_I) && (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) ||
                  (!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint))) &&
                 (!PARSER_Data_p->Backward))
                ||
                (((PARSER_Data_p->PictureLocalData.IsRandomAccessPoint) || (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) &&
                 (PARSER_Data_p->Backward) &&
                 (!PARSER_Data_p->IsBitBufferFullyParsed)))
#else
            if (((PARSER_Data_p->PictureLocalData.IsRandomAccessPoint == PARSER_Data_p->GetPictureParams.GetRandomAccessPoint) ||
                (!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint))))
#endif
            {
                h264par_FillParsingJobResult(ParserHandle);

                TrStartCode(("Picture start@ 0x%x end@ 0x%x - SC@ 0x%x\r\n",
                        PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress,
                        PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress,
                        PARSER_Data_p->SCBuffer.NextSCEntry));
#if defined(STVID_TRICKMODE_BACKWARD) && !defined(DVD_SECURED_CHIP)
                if (PARSER_Data_p->Backward)
                {
                    CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                    /* to test if next Sc exist: end of buffer */
                    if (((U32)((U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p))) > 0)
                    {
                        PARSER_Data_p->OutputCounter += (U32)((U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p));
                        PARSER_Data_p->ParserJobResults.DiffPSC = PARSER_Data_p->OutputCounter;
                        PARSER_Data_p->PictureGenericData.DiffPSC = PARSER_Data_p->OutputCounter;
                    }
                }
#endif /* (STVID_TRICKMODE_BACKWARD) && !defined(DVD_SECURED_CHIP)  */
                /*ParserState don't need to be protected */
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->ParserJobResults));

                /* Global and Picture dcoding Specific Data have been copied in producer structure, we can reset the SPS/PPS HasChanged flags */
                PARSER_Data_p->ActiveGlobalDecodingContextSpecificData_p->SPS_HasChanged = FALSE;
                PARSER_Data_p->ActivePictureDecodingContextSpecificData_p->PPS_HasChanged = FALSE;

                PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;
            }
            else
            {
#if defined(STVID_TRICKMODE_BACKWARD) && !defined(DVD_SECURED_CHIP)
                if (((!PARSER_Data_p->IsBitBufferFullyParsed) && (PARSER_Data_p->Backward)) || ((!PARSER_Data_p->Backward)))
                {
                    if (PARSER_Data_p->Backward)
                    {
                        CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                        /* to test if next Sc exist: end of buffer */
                        if (((U32)((U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p))) > 0)
                        {
                            PARSER_Data_p->OutputCounter += (U32)((U32)(GetSCAdd(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p));
                            PARSER_Data_p->ParserJobResults.DiffPSC = PARSER_Data_p->OutputCounter;
                            PARSER_Data_p->PictureGenericData.DiffPSC = PARSER_Data_p->OutputCounter;
                        }

                    }
                    STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData));
                }
#else
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData));
#endif /* STVID_TRICKMODE_BACKWARD && !DVD_SECURED_CHIP */
            }

        }
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
    }
    else
    {
        /* Wake up task to look for next start codes */
        STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
    }
}
/*******************************************************************************/
/*! \brief Get the Start Code commands. Flush the command queue
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void h264par_GetStartCodeCommand(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode;
#ifdef STVID_TRICKMODE_BACKWARD
	PARSER_Properties_t * PARSER_Properties_p;
	PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
#endif
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* The start code to process is in the StartCodeCommand buffer */
    if ((PARSER_Data_p->StartCode.IsPending) && (PARSER_Data_p->StartCode.IsInBitBuffer))
	{
        #if defined(DVD_SECURED_CHIP)
        /* Parser works with Start/Stop addresses of ES copy buffer */
        /* StartAddress_p and StopAddress_p are initialized with ES Copy buffer addresses for secured chips */
        /* Original bitbuffer addresses are also passed to this function for the decoder structure VIDBUFF_PictureBuffer_t */
        #endif  /* DVD_SECURED_CHIP */

        if (PARSER_Data_p->StartCode.StopAddress_p >= PARSER_Data_p->StartCode.StartAddress_p)
        {
            STOS_InvalidateRegion( STOS_MapPhysToCached(PARSER_Data_p->StartCode.StartAddress_p, (PARSER_Data_p->StartCode.StopAddress_p - PARSER_Data_p->StartCode.StartAddress_p)), (PARSER_Data_p->StartCode.StopAddress_p - PARSER_Data_p->StartCode.StartAddress_p) );
        }
        else
        {
#if defined(DVD_SECURED_CHIP)
#if 1 /* This is a Workaround */
        /* TO DO => crash occurs when :                                                        */
        /* PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p < PARSER_Data_p->StartCode.StartAddress_p */
        /* => so the value                                                                     */
        /* PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p - PARSER_Data_p->StartCode.StartAddress_p */
        /* is very big and makes STOS_InvalidateRegion fail                                    */
        /* => seems that the ES Copy buffer overflows                                          */
        /* So, this WA checks that the SC entry does not point outside the ESCopy              */
        /* Note this WA aims at fixing the first STOS_InvalidateRegion call only               */
        if( (U32)PARSER_Data_p->StartCode.StartAddress_p <= (U32)PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p
         && (U32)PARSER_Data_p->StartCode.StartAddress_p >= (U32)PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p )
        {
#endif /* if 1 */
            /* In secure chip the Start Code refer to the ES_Copy buffer (copy of ES Bit Buffer) */
            STOS_InvalidateRegion( STOS_MapPhysToCached(PARSER_Data_p->StartCode.StartAddress_p, (PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p - PARSER_Data_p->StartCode.StartAddress_p)), (PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p - PARSER_Data_p->StartCode.StartAddress_p) );
#if 1 /* This is a Workaround */
        }
        else
        {
           TrWA(("PARSER: SC points outside the ESCopy buffer: StartCode.StartAddress_p=0x%08x > ESCopy_Stop_p=0x%08x\r\n",
              PARSER_Data_p->StartCode.StartAddress_p, PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p));
        }
#endif /* if 1 */
            STOS_InvalidateRegion( STOS_MapPhysToCached(PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p, (PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p - PARSER_Data_p->StartCode.StartAddress_p)), (PARSER_Data_p->StartCode.StopAddress_p - PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p) );
#else /* DVD_SECURED_CHIP */
            /* In non-secure chip the Start Code refer to the ES Bit Buffer */
            STOS_InvalidateRegion( STOS_MapPhysToCached(PARSER_Data_p->StartCode.StartAddress_p, (PARSER_Data_p->BitBuffer.ES_Stop_p - PARSER_Data_p->StartCode.StartAddress_p)), (PARSER_Data_p->BitBuffer.ES_Stop_p - PARSER_Data_p->StartCode.StartAddress_p) );
            STOS_InvalidateRegion( STOS_MapPhysToCached(PARSER_Data_p->BitBuffer.ES_Start_p, (PARSER_Data_p->BitBuffer.ES_Stop_p - PARSER_Data_p->StartCode.StartAddress_p)), (PARSER_Data_p->StartCode.StopAddress_p - PARSER_Data_p->BitBuffer.ES_Start_p) );
#endif /* DVD_SECURED_CHIP */
        }

        /* The NAL can now be processed */
        ErrorCode = h264par_ProcessStartCode(&PARSER_Data_p->StartCode, ParserHandle);
        if (ErrorCode == ST_NO_ERROR)
        {
            h264par_CheckEventToSend(ParserHandle);
        }
        else
        {
            /* Look for next start code */
            PARSER_Data_p->StartCode.IsInBitBuffer = FALSE;
        }

        /* The start code is not pending anymore */
        PARSER_Data_p->StartCode.IsPending = FALSE;
    }
#ifdef STVID_TRICKMODE_BACKWARD
    else if ((PARSER_Data_p->Backward) && (PARSER_Data_p->StartCode.IsPending) && (!(PARSER_Data_p->StartCode.IsInBitBuffer))
             && (!PARSER_Data_p->IsBitBufferFullyParsed))
	{
        PARSER_Data_p->IsBitBufferFullyParsed = TRUE;
        STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

        /* Bit Buffer is fully parsed: stop parsing ... */
        PARSER_Data_p->StopParsing = TRUE;
	}
#endif /* STVID_TRICKMODE_BACKWARD */

	/* Else: if no start code is ready to be processed, simply return */
}                                       /* h264par_GetStartCodeCommand */

/*******************************************************************************/
/*! \brief Get the controller commands. Flush the command queue
 *
 * It modifies the parser state
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void h264par_GetControllerCommand(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Protect ControllerCommand with TaskLock : only accessed under task context,
	   and so we remove a semaphore */
	STOS_TaskLock();
    if (PARSER_Data_p->ForTask.ControllerCommand)
	{
		PARSER_Data_p->ParserState = PARSER_STATE_PARSING;
		PARSER_Data_p->ForTask.ControllerCommand = FALSE;
	}
	STOS_TaskUnlock();
	/* Force Parser to read next start code, if any */
	STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);

}

/*******************************************************************************/
/*! \brief Parser Tasks
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void h264par_ParserTaskFunc(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */

    H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    STOS_TaskEnter(NULL);

    /* Big loop executed until ToBeDeleted is set --------------------------- */
	do
	{
		while(!PARSER_Data_p->ParserTask.ToBeDeleted && (PARSER_Data_p->ParserState != PARSER_STATE_PARSING))
		{
			/* Waiting for a command */
			STOS_SemaphoreWait(PARSER_Data_p->ParserOrder);
			h264par_GetControllerCommand(ParserHandle);
		}
		if (!PARSER_Data_p->ParserTask.ToBeDeleted)
		{
            /* The parser has a command to process */
            while(!PARSER_Data_p->ParserTask.ToBeDeleted && (PARSER_Data_p->ParserState == PARSER_STATE_PARSING))
            {
                STOS_SemaphoreWait(PARSER_Data_p->ParserSC);
                /* Flush the command queue, as the parser is able to process one command at a time */
                while (STOS_SemaphoreWaitTimeOut(PARSER_Data_p->ParserSC, TIMEOUT_IMMEDIATE) == 0)
                {
                /* Nothing to do: just to decrement counter until 0, to avoid multiple looping on semaphore wait */;
                }

                if (!PARSER_Data_p->ParserTask.ToBeDeleted)
                {
                    /* Look for new Start Codes to process: the semaphore "ParserSC" is raised if
                    * there is a start code to process
                    */
#ifdef STVID_TRICKMODE_BACKWARD
                    if (((PARSER_Data_p->Backward) && (!PARSER_Data_p->StopParsing)) ||
                    (!PARSER_Data_p->Backward))
#endif /* STVID_TRICKMODE_BACKWARD */
                    {
                    h264par_IsThereStartCodeToProcess(ParserHandle);
                    /* Then, get Start code commands */
                    h264par_GetStartCodeCommand(ParserHandle);
                    }
                }
                if (PARSER_Data_p->StopIsPending)
                {
                    PARSER_Data_p->ParserState = PARSER_STATE_IDLE;
                    PARSER_Data_p->StopIsPending = FALSE;
                    while (STOS_SemaphoreWaitTimeOut(PARSER_Data_p->ParserSC, TIMEOUT_IMMEDIATE) == 0)
                    {            /* Nothing to do: just to decrement counter until 0, to avoid multiple looping on semaphore wait */
                        /* The PARSER_Data_p->ParserSC is signalled from multiple places */
                        /* In order for the atomic counts within the semaphore to remain in good state */
                        /* we do the semaphore wait on immediate timeout, just to removed all the */
                        /* signlals that may have come after the h264par_stop() is called from */
                        /* the producer task.      */
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
static ST_ErrorCode_t h264par_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
    #if defined(DVD_SECURED_CHIP)
        STFDMA_SCEntry_t * CurrentSCEntry_p;
    #endif /* DVD_SECURED_CHIP */

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Stores the decoder read pointer locally for further computation of the bit buffer level */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = InformReadAddressParams_p->DecoderReadAddress_p;

    #if defined(DVD_SECURED_CHIP)

        CurrentSCEntry_p = PARSER_Data_p->SCBuffer.CurrentSCEntry;

        /* Need to inform the FDMA about the ES Copy address of the last parsed Start Code entry */
        VIDINJ_TransferSetESCopyBufferRead(PARSER_Data_p->Inject.InjecterHandle,
                                           PARSER_Data_p->Inject.InjectNum,
                                           (void *)(U32)GetSCAddForESCopyBuffer(CurrentSCEntry_p));

        TrESCopy(("\r\nPARSER: Read Address sent to ESCopy buffer=0x%08x",
                                            (U32)GetSCAddForESCopyBuffer(CurrentSCEntry_p)));
    #endif /* DVD_SECURED_CHIP */

    /* Inform the FDMA on the read pointer of the decoder in Bit Buffer */
    VIDINJ_TransferSetESRead(PARSER_Data_p->Inject.InjecterHandle,
                             PARSER_Data_p->Inject.InjectNum,
                             (void*) (((U32)InformReadAddressParams_p->DecoderReadAddress_p)&0xffffff80));


    return (ST_NO_ERROR);
} /* End of h264par_InformReadAddress() function */
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
static ST_ErrorCode_t h264par_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t   * PARSER_Data_p;
    U8                        * LatchedES_Write_p = NULL;             /* write pointer (of FDMA) in ES buffer   */
    U8                        * LatchedES_DecoderRead_p = NULL;       /* Read pointer (of decoder) in ES buffer */

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    LatchedES_Write_p       = PARSER_Data_p->BitBuffer.ES_Write_p;
    LatchedES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_DecoderRead_p;

#ifdef STVID_TRICKMODE_BACKWARD
    if (PARSER_Data_p->Backward)
    {
        /* Ensure no FDMA transfer is done while flushing the injecter */
        VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

        /* Flush the input buffer content before getting the linear buffer level */
        /*VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, FALSE, Pattern, sizeof(Pattern));*/

        /* CD (25/07/2007): we should probably protect access to BitBuffer struct with a semaphore, to be confirmed ... */
        /*                  This is done in mpeg2 with: SEMAPHORE_WAIT(LCMPEG_CONTEXT_P->SharedAccess_p);*/
        GetBitBufferLevelParams_p->BitBufferLevelInBytes = (U32)LatchedES_Write_p - (U32)PARSER_Data_p->BitBuffer.ES_Start_p + 1;

        /* Release FDMA transfers */
        VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    }
    else
#endif /* STVID_TRICKMODE_BACKWARD */
    {
        if (LatchedES_Write_p < LatchedES_DecoderRead_p)
        {
            /* Write pointer has done a loop, not the Read pointer */
            GetBitBufferLevelParams_p->BitBufferLevelInBytes = PARSER_Data_p->BitBuffer.ES_Stop_p
                                                            - LatchedES_DecoderRead_p
                                                            + LatchedES_Write_p
                                                            - PARSER_Data_p->BitBuffer.ES_Start_p
                                                            + 1;
        }
        else
        {
            /* Normal: start <= read <= write <= top */
            GetBitBufferLevelParams_p->BitBufferLevelInBytes = LatchedES_Write_p - LatchedES_DecoderRead_p;
        }
    }

	return (ST_NO_ERROR);
} /* End of h264par_GetBitBufferLevel() function */

#ifdef VALID_TOOLS
/*******************************************************************************
Name        : h264par_RegisterTrace
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264PARSER_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    return(ErrorCode);
}
#endif /* VALID_TOOLS */

/*******************************************************************************
Name        : h264par_GetDataInputBufferParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t	h264par_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle,
												void ** const BaseAddress_p,
												U32 * const Size_p)
{
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    *BaseAddress_p    = STAVMEM_DeviceToVirtual(PARSER_Data_p->PESBuffer.PESBufferBase_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    *Size_p           = (U32)PARSER_Data_p->PESBuffer.PESBufferTop_p
                      - (U32)PARSER_Data_p->PESBuffer.PESBufferBase_p
                      + 1;
    return(ST_NO_ERROR);

} /* End of h264par_GetDataInputBufferParams() function */


/*******************************************************************************
Name        : h264par_SetDataInputInterface
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t	h264par_SetDataInputInterface(const PARSER_Handle_t ParserHandle,
								ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
								void (*InformReadAddress)(void * const Handle, void * const Address),
								void * const Handle)
{
	ST_ErrorCode_t Err;
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while setting a new data input interface */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    Err = VIDINJ_Transfer_SetDataInputInterface(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum,
                                                GetWriteAddress, InformReadAddress, Handle);
    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    return(Err);
} /* End of h264par_SetDataInputInterface() function */


/*******************************************************************************
Name        : SavePTSEntry
Description : Stores a new parsed PTSEntry in the PTS StartCode List
Parameters  : Parser handle, PTS Entry Pointer in FDMA's StartCode list
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void h264par_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* CPUPTSEntry_p)
{
	H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(DVD_SECURED_CHIP)
    /* No change here, PTS entry points to bit buffer */
    if ((U32)GetSCAddForBitBuffer(CPUPTSEntry_p) != (U32)PARSER_Data_p->LastPTSStored.Address_p)
#else
    if ((U32)GetSCAdd(CPUPTSEntry_p) != (U32)PARSER_Data_p->LastPTSStored.Address_p)
#endif /* DVD_SECURED_CHIP */
    {
        /* Check if we can find a place to store the new PTS Entry, otherwise we overwrite the oldest one */
        if ( (U32)PARSER_Data_p->PTS_SCList.Read_p == (U32)PARSER_Data_p->PTS_SCList.Write_p + sizeof(PTS_t) )
        {
            /* First free the oldest one */
            /* Keep always an empty SCEntry because Read_p == Write_p means that the SCList is empty */
            PARSER_Data_p->PTS_SCList.Read_p++;
            if (((U32)PARSER_Data_p->PTS_SCList.Read_p) >= ((U32)PARSER_Data_p->PTS_SCList.SCList + sizeof(PTS_t) * PTS_SCLIST_SIZE) )
            {
                PARSER_Data_p->PTS_SCList.Read_p = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList ;
            }
            /* Next overwrite it */
        }

        PARSER_Data_p->LastPTSStored.PTS33      = GetPTS33(CPUPTSEntry_p);
        PARSER_Data_p->LastPTSStored.PTS32      = GetPTS32(CPUPTSEntry_p);

        #if defined(DVD_SECURED_CHIP)
            PARSER_Data_p->LastPTSStored.Address_p  = (void*)GetSCAddForBitBuffer(CPUPTSEntry_p);
        #else
            PARSER_Data_p->LastPTSStored.Address_p  = (void*)GetSCAdd(CPUPTSEntry_p);
        #endif /* DVD_SECURED_CHIP */

        /* Store the PTS */
        PARSER_Data_p->PTS_SCList.Write_p->PTS33 = PARSER_Data_p->LastPTSStored.PTS33;
        PARSER_Data_p->PTS_SCList.Write_p->PTS32 = PARSER_Data_p->LastPTSStored.PTS32;
        PARSER_Data_p->PTS_SCList.Write_p->Address_p = PARSER_Data_p->LastPTSStored.Address_p;

        PARSER_Data_p->PTS_SCList.Write_p++;
        if (((U32)PARSER_Data_p->PTS_SCList.Write_p) >= ((U32)PARSER_Data_p->PTS_SCList.SCList + sizeof(PTS_t) * PTS_SCLIST_SIZE) )
        {
            PARSER_Data_p->PTS_SCList.Write_p = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList ;
            if (PARSER_Data_p->PTS_SCList.Read_p == PARSER_Data_p->PTS_SCList.Write_p)
            {
                /* Only 1 element in list after wraparound, need for Write_p++ */
                PARSER_Data_p->PTS_SCList.Write_p++;
            }
        }
    }
} /* End of h264par_SavePTSEntry() function */

#ifdef ST_speed
/*******************************************************************************
Name        : h264par_GetBitBufferOutputCounter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 h264par_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle)
{
    H264ParserPrivateData_t * PARSER_Data_p;
    U32 DiffPSC;
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, STAVMEM_VIRTUAL_MAPPING_AUTO_P);

    #if defined(DVD_SECURED_CHIP)
    	if ((U32) (PARSER_Data_p->LastSCAdd_p) < (U32)GetSCAddForBitBuffer(CPUNextSCEntry_p))
        {
            DiffPSC = (U32)((U32)(GetSCAddForBitBuffer(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p));
        }
        else
        {
            /* ES buffer (bit buffer) looped. */
            DiffPSC = ((U32)(PARSER_Data_p->BitBuffer.ES_Stop_p)
                      - (U32)(PARSER_Data_p->LastSCAdd_p)
                      + (U32)(GetSCAddForBitBuffer(CPUNextSCEntry_p))
                      - (U32)(PARSER_Data_p->BitBuffer.ES_Start_p) + 1);
        }
    #else
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
    #endif /* DVD_SECURED_CHIP */

    PARSER_Data_p->OutputCounter              += DiffPSC;
    return(PARSER_Data_p->OutputCounter);
}
#endif /*ST_speed*/

/*******************************************************************************
Name        : h264par_FlushInjection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void h264par_FlushInjection(const PARSER_Handle_t ParserHandle)
{
	H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    U8                        Pattern[]={0xAF, 0xAF, 0x00, 0x00, 0x01, 0xB8};

    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, TRUE, Pattern, sizeof(Pattern));
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID ++;
    PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension = PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID;
} /* h264par_FlushInjection */

#ifdef ST_speed
#ifdef STVID_TRICKMODE_BACKWARD

/*******************************************************************************
Name        : h264par_SetBitBuffer
Description : Set the location of the bit buffer
Parameters  : HAL Parser manager handle, buffer address, buffer size in bytes
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void h264par_SetBitBuffer(const PARSER_Handle_t        ParserHandle,
                                 void * const                 BufferAddressStart_p,
                                 const U32                    BufferSize,
                                 const BitBufferType_t        BufferType,
                                 const BOOL                   Stop  )
{
    U8                          Pattern[]={0xAF, 0xAF, 0x00, 0x00, 0x01, 0xB8};
	H264ParserPrivateData_t   * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
        /* ES Copy */     ,PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,
                           PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p
#endif /* DVD_SECURED_CHIP */
                           );

        /* Inform the FDMA about the read pointer of the decoder in Bit Buffer */
        VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, TRUE, Pattern, sizeof(Pattern));
        VIDINJ_TransferSetESRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, (void*) (((U32)PARSER_Data_p->BitBuffer.ES_DecoderRead_p)&0xffffff80));

        #if defined(DVD_SECURED_CHIP)
            /* Need to inform the FDMA about the read pointer of the parser in ES Copy Buffer */
            VIDINJ_TransferSetESCopyBufferRead(PARSER_Data_p->Inject.InjecterHandle,
                                               PARSER_Data_p->Inject.InjectNum,
                                               (void *)GetSCAddForESCopyBuffer(PARSER_Data_p->SCBuffer.CurrentSCEntry));
    	#endif /* DVD_SECURED_CHIP */
    }
    STTBX_Print(("\n SET ES buffer Base:%lx - Top:%lx\nSC buffer Base:%lx - Top:%lx\nPES buffer Base:%lx - Top:%lx\n",
            /* BB */        (unsigned long)(PARSER_Data_p->BitBuffer.ES_Start_p),
                            (unsigned long)(PARSER_Data_p->BitBuffer.ES_Stop_p),
            /* SC */        (unsigned long)(PARSER_Data_p->SCBuffer.SCList_Start_p),
                            (unsigned long)(PARSER_Data_p->SCBuffer.SCList_Stop_p),
            /* PES */       (unsigned long)(PARSER_Data_p->PESBuffer.PESBufferBase_p),
                            (unsigned long)(PARSER_Data_p->PESBuffer.PESBufferTop_p)));

    #if defined(DVD_SECURED_CHIP)
        STTBX_Print(("ESCopy buffer Base:%lx - Top:%lx\n",
             /* ES Copy */      (unsigned long)(PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p),
                                (unsigned long)(PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p)));
    #endif /* DVD_SECURED_CHIP */

    /* Release FDMA transfers */
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
} /* End of h264par_SetBitBuffer() function */

/*******************************************************************************
Name        : h264par_WriteStartCode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void h264par_WriteStartCode(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p)
{
    const U8*  Temp8;
    H264ParserPrivateData_t * PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

    #if defined(DVD_SECURED_CHIP)
        /* DG TO DO: in addition, give Addr2 for address in ESCopy buffer */
        /* (Function called only in trickmodes) */
    #endif /* DVD_SECURED_CHIP */

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
}
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed*/

/*******************************************************************************
Name        : h264par_Setup
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t h264par_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
	H264ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    switch (SetupParams_p->SetupObject)
    {
/*****/ case STVID_SETUP_FDMA_NODES_PARTITION:   /****************************************************************************************/
    /* Ensure no FDMA transfer is done while setting the FDMA node partition */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    ErrorCode = VIDINJ_ReallocateFDMANodes(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, SetupParams_p);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
            break;

/*****/ case STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION:   /********************************************************************************************/
            ErrorCode = h264par_ReallocateStartCodeBuffer(ParserHandle, SetupParams_p);
            break;

/*****/ case STVID_SETUP_DATA_INPUT_BUFFER_PARTITION:   /****************************************************************************/
            ErrorCode = h264par_ReallocatePESBuffer(ParserHandle, SetupParams_p);
            break;

/*****/ default:   /**********************************************************************************************************************/
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return(ErrorCode);
} /* End of h264par_Setup() function */
/*******************************************************************************/
/*! \brief Point on Next Entry in Start Code List
 *
 *
 * \param SCEntry_p (in physical address space).
 * \Returns     : returns a pointer equal to input param SCEntry_p if the end of available SC in the list have been reached
 */
/*******************************************************************************/
static STFDMA_SCEntry_t * h264par_CheckOnNextSC(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t * SCEntry_p)
{
	/* Behavior of the SC List filling:
	 * SCListWrite is incremented by burst each time a new SCEntry is written
	 * (burst size is the number of SC found within 128 Byte of stream)
     * Before running the FDMA, the injection checks whether SCListWrite is not too closed from SCList_Stop_p
	 *    - If the remaining space is too low:
	 *       - SCListLoop is set to be equal to SCListWrite (means that the upper
	 *         boundary is no longer SCListStop but SCListLoop)
	 *       - SCListWrite is set to SCListStart (start filling the SC List from the beginning)
	 *    - If there is enough remaining space: SCListLoop is set to be equal to SCListWrite all along
	 *      the filling process
	 */

	/* dereference ParserHandle to a local variable to ease debug */
    STFDMA_SCEntry_t * ReturnedNextSCEntry_p;

	H264ParserPrivateData_t * PARSER_Data_p;
    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    ReturnedNextSCEntry_p = SCEntry_p;

    if((SCEntry_p == PARSER_Data_p->SCBuffer.LatchedSCList_Loop_p)
        &&(SCEntry_p != PARSER_Data_p->SCBuffer.LatchedSCList_Write_p))
    {
        #ifdef STVID_PARSER_DO_NOT_LOOP_ON_STREAM
            ReturnedNextSCEntry_p = PARSER_Data_p->SCBuffer.LatchedSCList_Write_p;
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PARSER : Reached end of Stream"));
        #else
            /* Reached top of SCList */
            ReturnedNextSCEntry_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
        #endif
    }
    else
    {
        ReturnedNextSCEntry_p++;
    }
    return (ReturnedNextSCEntry_p);
} /* End of h264par_CheckOnNextSC() function */


/*******************************************************************************/
/*! \brief Get the next Start Code to process
*
* If a start code is found, the read pointer in SC+PTS buffer points to the entry
* following the start code.
*
* \param SCEntry_p (in physical address space)
* \return PARSER_Data_p->StartCode.IsPending TRUE if start code found
* \returns a pointer equal to input param SCEntry_p if the end of available SC in the list have been reached
*/
/*******************************************************************************/
static STFDMA_SCEntry_t * h264par_GetNextStartCodeToCheck(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t * SCEntry_p)
{

    STFDMA_SCEntry_t * ReturnedNextSCEntry_p;

	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t * PARSER_Properties_p;
	BOOL SCFound;
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    ReturnedNextSCEntry_p = SCEntry_p;

    /* check if end of list have been reached */
	if (SCEntry_p != PARSER_Data_p->SCBuffer.LatchedSCList_Write_p)
    {
		SCFound = FALSE;
        /* search for available SC in SC+PTS buffer : */
        while((ReturnedNextSCEntry_p != PARSER_Data_p->SCBuffer.LatchedSCList_Write_p) && (!(SCFound)))
	    {
			/* Make ScannedEntry_p point on the next entry in SC List */
			ReturnedNextSCEntry_p = h264par_CheckOnNextSC(ParserHandle, ReturnedNextSCEntry_p);
#ifdef ST_5100
            /* This chip dependency should be removed once addresses management is generalized */
            CPUNextSCEntry_p = ReturnedNextSCEntry_p;
#else
            CPUNextSCEntry_p = STAVMEM_DeviceToCPU(ReturnedNextSCEntry_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
#endif
            if(!IsPTS(CPUNextSCEntry_p))
			{
                #if defined(DVD_SECURED_CHIP)
                /* According to the fdma team, "The ESCopy Write Address will be set to Zero if the SC found is not
                the first slice start code". So we ignore the entry with the NULL ES Copy value and jump to the next. */
                if (CPUNextSCEntry_p->SC.Addr2 != NULL)
                #endif /* DVD_SECURED_CHIP */
                {
                        SCFound = TRUE;
	            }
	        }
        } /* end while */
	}
	return (ReturnedNextSCEntry_p);
} /* End of h264par_GetNextStartCodeToCheck() function */


/*******************************************************************************/
/*! \brief
 *
 *
* \param
 * \return
 */
/*******************************************************************************/
static void h264par_IncrementPossibleFakeStartCodeCounter(U8 AnalysedStartCodeValue, U8 PreviousStartCodeValue, U32 * error_count_p)
{
    if (AnalysedStartCodeValue==(PreviousStartCodeValue+1))
    {
        /* in MPEG2 slice start codes are incrementing*/
        (* error_count_p)++;
    }
    else
    {
        switch (AnalysedStartCodeValue)
        {
       		/* 0/B3/B5/B8 are MPEG2 start codes, current FDMA FW in H264 mode is outputing 33/35/38 instead of B3/B5/B8 */
            case 0:
       		case 0x33:
       		case 0x35:
       		case 0x38:
       		case 0xB3:
       		case 0xB5:
       		case 0xB8:
                (* error_count_p)++;
       		default:
                break;
        }
    }

}

/*******************************************************************************
Name        : h264par_SCValid
Description : Objective of this function is to filter the start code values to detect if the stream
injected is not H264 stream but MPEG2 stream for instance. Wrong start codes are dropped
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

static BOOL h264par_CheckNextSCValid(const PARSER_Handle_t ParserHandle)
{
	BOOL SCValid=TRUE;

#ifdef  WA_GNBvd63171
	/* dereference ParserHandle to a local variable to ease debug */
	H264ParserPrivateData_t * PARSER_Data_p;
	STFDMA_SCEntry_t *NextSCEntry_p[NB_SC_ANALYZED+1];  /* physical address pointer of Start code in memory */
	STFDMA_SCEntry_t *CPUNextSCEntry_p[NB_SC_ANALYZED+1]; /* CPU address pointer of Start code in memory */
    U8 NextSCValue[NB_SC_ANALYZED+1];
    U32 NbStartCodeFound, i, UnexpectedSCCount = 0;
    U32 MinThreshold, MaxThreshold;

	PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    NextSCEntry_p[0] = PARSER_Data_p->SCBuffer.CurrentSCEntry;
    NextSCEntry_p[1] = PARSER_Data_p->SCBuffer.NextSCEntry;
    for (i=2; i <= NB_SC_ANALYZED; i++)
    {
        NextSCEntry_p[i] = h264par_GetNextStartCodeToCheck(ParserHandle, NextSCEntry_p[i-1]);
    }
    for (i=0; i <= NB_SC_ANALYZED; i++)
    {
        CPUNextSCEntry_p[i] = STAVMEM_DeviceToCPU(NextSCEntry_p[i], STAVMEM_VIRTUAL_MAPPING_AUTO_P);
        NextSCValue[i] = GetSCVal(CPUNextSCEntry_p[i]);
    /* debug trace:*/
    /*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Current SC value 0x(%x) \n", NextSCValue[i]));*/
    }


    /* we check current Start code and the  following ones to evaluate whether the stream can be an H264 stream or an MPEG2 stream*/
    /* the goal is to handle the case were customers switch the input stream from H264 to MPEG2 without stopping H264 codec early enough */
    NbStartCodeFound = 0;
    for (i=1; i <= NB_SC_ANALYZED; i++)
    {
        if (NextSCEntry_p[i] != NextSCEntry_p[i-1])  /* pointer are equal if we have reached the end of the available SC in the list*/
        {
            h264par_IncrementPossibleFakeStartCodeCounter(NextSCValue[i], NextSCValue[i-1], &UnexpectedSCCount);
            NbStartCodeFound++;
        }
    }

    MaxThreshold = 2;
    if (NbStartCodeFound < NB_SC_ANALYZED)
    {
        MinThreshold = 0;
    }
    else
    {
        MinThreshold = 1;
    }
    /* Threshold take into account if previous start code was incorrect and if we are at the end of the start coe list*/
    /* Depending on NB_SC_ANALYZED, MaxThreshold, MinThreshold, UnexpectedSCCount the parser analyzed if the stream injected is H264 stream or not */
    if ((UnexpectedSCCount > MaxThreshold) || ((UnexpectedSCCount > MinThreshold) && (PARSER_Data_p->PreviousSCWasNotValid)))
    {
        SCValid = FALSE;
        PARSER_Data_p->PreviousSCWasNotValid = TRUE;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PARSER : False H264 start code detected"));
    }
    else
    {
        SCValid = TRUE;
        PARSER_Data_p->PreviousSCWasNotValid = FALSE;
    }
#endif  /* #ifdef  WA_GNBvd63171*/

return SCValid;

}/* End of h264par_SCValid() function */

/* End of h264parser.c */
