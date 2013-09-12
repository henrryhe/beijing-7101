/*!
 *******************************************************************************
 * \file vc1parser.c
 *
 * \brief VC1 Parser top level CODEC API functions
 *
 * \author
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
#include <assert.h>
#endif

/* CODEC API function prototypes */
/* #include "parser.h" */

/* VC1 Parser specific */
#include "vc1parser.h"

#include "sttbx.h"

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/*#define TRACE_UART*/

/* Select the Traces you want */
#define TrESCopy        TraceOff
#define TrStartCode     TraceOff
#define TrInject        TraceOff
#define TrPTS           TraceOff

/* "vid_trace.h" and "trace.h" should be included AFTER the definition of the traces wanted */
#if defined TRACE_UART || defined TRACE_INJECT || defined TRACE_FLUSH
    #include "..\..\tests\src\trace.h"
#endif
#include "vid_trace.h"

#define FRAME_NUM_FILENAME  "frame_num"
/*#define OLD_SLICE_METHOD*/

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
/*
 overload of GetSCAdd() this function returns of the offset FDMA, this address points on the SC and Payload
*/
#define		VC9_PROFILE_UNKNOWN			255  /* The Profile Will be set by the parsing of the sequence Header */
#define		BDU_TYPE_SKIP				0xFF /* Internal BDU, this BDU do nothing and skip the FDMA packet */

#undef  GetSCAdd
static __inline unsigned long GetSCAdd(STFDMA_SCEntry_t * Entry, VC1ParserPrivateData_t * Handle)
{
	if(Handle->PrivateGlobalContextData.Profile == VC9_PROFILE_MAIN || Handle->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE )
	{
		return (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr)) + VC1_MP_SC_SIGNATURE_SIZE);
	}
	else
	{
		return (STSYS_ReadRegMemUncached32LE((U32*)&((Entry)->SC.Addr)));

	}
}

#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */


/* Functions ---------------------------------------------------------------- */
static ST_ErrorCode_t vc1par_AllocatePESBuffer(const PARSER_Handle_t ParserHandle, const U32 PESBufferSize, const PARSER_InitParams_t *  const InitParams_p);
static ST_ErrorCode_t vc1par_AllocateSCBuffer(const PARSER_Handle_t ParserHandle, const U32 SC_ListSize, const PARSER_InitParams_t * const InitParams_p);
static ST_ErrorCode_t vc1par_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t vc1par_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle);
static void vc1par_InitRefFrameInfo(const PARSER_Handle_t  ParserHandle);

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
static int vc1par_CheckSCSignature(register unsigned char *pPayload,unsigned char SCValue, const PARSER_Handle_t  ParserHandle);
#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */

#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t vc1par_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p);
#endif
#ifdef STVID_DEBUG_GET_STATUS
static ST_ErrorCode_t vc1par_GetStatus(const PARSER_Handle_t ParserHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
static ST_ErrorCode_t vc1par_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p);
static ST_ErrorCode_t vc1par_Stop(const PARSER_Handle_t ParserHandle);
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t vc1par_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p);
#endif
static ST_ErrorCode_t vc1par_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p);
static ST_ErrorCode_t vc1par_GetPicture(const PARSER_Handle_t ParserHandle, const PARSER_GetPictureParams_t * const GetPictureParams_p);
static ST_ErrorCode_t vc1par_Term(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t vc1par_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p);
static ST_ErrorCode_t vc1par_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p);
static ST_ErrorCode_t vc1par_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle, void ** const BaseAddress_p, U32 * const Size_p);
static ST_ErrorCode_t vc1par_SetDataInputInterface(const PARSER_Handle_t ParserHandle, ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
                                                   void (*InformReadAddress)(void * const Handle, void * const Address),
								                   void * const Handle);
#ifdef ST_speed
static U32 vc1par_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle);
#endif /*ST_speed*/

static ST_ErrorCode_t vc1par_ReallocatePESBuffer(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * SetupParams_p);
static ST_ErrorCode_t vc1par_ReallocateStartCodeBuffer(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * SetupParams_p);

static void vc1par_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* PTSEntry_p);

/* Constants ---------------------------------------------------------------- */
const PARSER_FunctionsTable_t PARSER_VC1Functions =
{
#ifdef STVID_DEBUG_GET_STATISTICS
    vc1par_ResetStatistics,
    vc1par_GetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    vc1par_GetStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    vc1par_Init,
    vc1par_Stop,
    vc1par_Start,
    vc1par_GetPicture,
    vc1par_Term,
	vc1par_InformReadAddress,
	vc1par_GetBitBufferLevel,
	vc1par_GetDataInputBufferParams,
    vc1par_SetDataInputInterface,
    vc1par_Setup,
    vc1par_FlushInjection
#ifdef ST_speed
    ,vc1par_GetBitBufferOutputCounter
#ifdef STVID_TRICKMODE_BACKWARD
    ,vc1par_SetBitBuffer,
    vc1par_WriteStartCode
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed*/
    ,vc1par_BitBufferInjectionInit
#if defined(DVD_SECURED_CHIP)
    ,vc1par_SecureInit
#endif  /* DVD_SECURED_CHIP */
};

#if defined(TRACE_UART) && defined(VALID_TOOLS)
static TRACE_Item_t vc1par_TraceItemList[] =
{
    {"GLOBAL",              TRACE_VC1PAR_COLOR, TRUE, FALSE},
    {"INIT_TERM",           TRACE_VC1PAR_COLOR, TRUE, FALSE},
    {"EXTRACTBDU",          TRACE_VC1PAR_COLOR, TRUE, FALSE},
    {"PARSEBDU",            TRACE_VC1PAR_COLOR, TRUE, FALSE},
    {"PICTURE_INFO_COMPILE",TRACE_VC1PAR_COLOR, TRUE, FALSE}
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

/*******************************************************************************/
/*! \brief De-Allocate the PES Buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
ST_ErrorCode_t vc1par_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t         * PARSER_Properties_p;
    STAVMEM_FreeBlockParams_t   FreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

	PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = (VC1ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->PESBuffer.PESBufferHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The PESBuffer is not deallocated correctly !"));
    }

	return (ErrorCode);
} /* end of vc1par_DeAllocatePESBuffer */

/*******************************************************************************/
/*! \brief Allocate the PES Buffer
 *
 * \param ParserHandle Parser Handle
 * \param PESBufferSize PES buffer size in bytes
 * \param InitParams_p Init parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t vc1par_AllocatePESBuffer(const PARSER_Handle_t        ParserHandle,
		                               const U32                    PESBufferSize,
					         	       const PARSER_InitParams_t *  const InitParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
	ST_ErrorCode_t                          ErrorCode;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
        /* ErrorCode = */ vc1par_DeAllocatePESBuffer(ParserHandle);
        return(ErrorCode);
    }
    PARSER_Data_p->PESBuffer.PESBufferBase_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
    PARSER_Data_p->PESBuffer.PESBufferTop_p = (void *) ((U32)PARSER_Data_p->PESBuffer.PESBufferBase_p + PESBufferSize - 1);

	return (ErrorCode);
} /* end of vc1par_AllocatePESBuffer() */

/*******************************************************************************/
/*! \brief Reallocate the PES buffer and initialize the PES pointers for injection
 *
 * \param ParserHandle Parser Handle
 * \param SetupParams_p SETUP parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t vc1par_ReallocatePESBuffer(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * SetupParams_p)
{
    VC1ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_BlockParams_t BlockParams;
	ST_ErrorCode_t        ErrorCode;
	U32                   PESListSize;
	PARSER_InitParams_t   InitParams;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
    ErrorCode = vc1par_DeAllocatePESBuffer(ParserHandle);

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Allocate PES buffer in new partition passed as parameter */
        InitParams.AvmemPartitionHandle = SetupParams_p->SetupSettings.AVMEMPartition;
    	ErrorCode = vc1par_AllocatePESBuffer(ParserHandle, PESListSize, &InitParams);

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
} /* End of vc1par_ReallocatePESBuffer() */

/*******************************************************************************/
/*! \brief De-Allocate the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
ST_ErrorCode_t vc1par_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t         * PARSER_Properties_p;
    STAVMEM_FreeBlockParams_t   FreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

	PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = (VC1ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->SCBuffer.SCListHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The SCBuffer is not deallocated correctly !"));
    }

    return(ErrorCode);
} /* end of vc1par_DeAllocateSCBuffer() */

/*******************************************************************************/
/*! \brief Allocate the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \param SC_ListSize Start Code buffer size in bytes
 * \param InitParams_p Init parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t vc1par_AllocateSCBuffer(const PARSER_Handle_t        ParserHandle,
									   const U32                    SC_ListSize,
									   const PARSER_InitParams_t *  const InitParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
	ST_ErrorCode_t                          ErrorCode;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	AllocBlockParams.PartitionHandle       = InitParams_p->AvmemPartitionHandle;
    AllocBlockParams.Alignment     = 32;
    AllocBlockParams.Size          = SC_ListSize * sizeof(STFDMA_SCEntry_t);
    AllocBlockParams.AllocMode     = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &PARSER_Data_p->SCBuffer.SCListHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }
    ErrorCode = STAVMEM_GetBlockAddress(PARSER_Data_p->SCBuffer.SCListHdl,(void **)&VirtualAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* ErrorCode = */ vc1par_DeAllocateSCBuffer(ParserHandle);
        return(ErrorCode);
    }
    PARSER_Data_p->SCBuffer.SCList_Start_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
    PARSER_Data_p->SCBuffer.SCList_Stop_p  = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.SCList_Stop_p  += (SC_ListSize-1);

	return (ErrorCode);
} /* end of vc1par_AllocateSCBuffer() */

/*******************************************************************************/
/*! \brief Reallocate the Start Code buffer and initialize the SC pointers
 *
 * \param ParserHandle Parser Handle
 * \param SetupParams_p SETUP parameters from CODEC API call
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t vc1par_ReallocateStartCodeBuffer(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * SetupParams_p)
{
    VC1ParserPrivateData_t * PARSER_Data_p = ((VC1ParserPrivateData_t*)((PARSER_Properties_t*) ParserHandle)->PrivateData_p);
    STAVMEM_BlockParams_t    BlockParams;
    ST_ErrorCode_t           ErrorCode;
    U32                      StartCodeListSize;
    PARSER_InitParams_t      InitParams;


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
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Could not get SCList info, using default value for SCList size! (%d)", SC_LIST_SIZE_IN_ENTRY));
    }

    /* Deallocate existing start code buffer from current partition */
    ErrorCode = vc1par_DeAllocateSCBuffer(ParserHandle);

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Allocate Start Code buffer in new partition passed as parameter */
        InitParams.AvmemPartitionHandle = SetupParams_p->SetupSettings.AVMEMPartition;
    	ErrorCode = vc1par_AllocateSCBuffer(ParserHandle, StartCodeListSize, &InitParams);

        if (ErrorCode == ST_NO_ERROR)
        {
            /* Initialize the SC buffer pointers */
	        vc1par_InitSCBuffer(ParserHandle);
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
} /* End of vc1par_ReallocateStartCodeBuffer() */

/*******************************************************************************/
/*! \brief Initialize the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \return void
 */
/*******************************************************************************/
void vc1par_InitSCBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Set default values for pointers in SC-List */
    PARSER_Data_p->SCBuffer.SCList_Write_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.SCList_Loop_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.NextSCEntry = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.CurrentSCEntry_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
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
ST_ErrorCode_t vc1par_InitInjectionBuffers(const PARSER_Handle_t        ParserHandle,
		                                   const PARSER_InitParams_t *  const InitParams_p,
						            	   const U32                    SC_ListSize,
								           const U32 				    PESBufferSize)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
    VIDINJ_GetInjectParams_t              Params;
	ST_ErrorCode_t                          ErrorCode;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	ErrorCode = ST_NO_ERROR;

	/* Allocate SC Buffer */
    ErrorCode = vc1par_AllocateSCBuffer(ParserHandle, SC_ListSize, InitParams_p);
	if (ErrorCode != ST_NO_ERROR)
	{
		return (ErrorCode);
	}

	/* Allocate PES Buffer */
    ErrorCode = vc1par_AllocatePESBuffer(ParserHandle, PESBufferSize, InitParams_p);
	if (ErrorCode != ST_NO_ERROR)
	{
        /* ErrorCode = */ vc1par_DeAllocateSCBuffer(ParserHandle);
		return (ErrorCode);
	}

#if defined(DVD_SECURED_CHIP)
        /* ES Copy init done after STVID_Setup() function for object STVID_SETUP_ES_COPY_BUFFER */
#endif /* DVD_SECURED_CHIP */

    /* Set default values for pointers in ES Buffer */
    PARSER_Data_p->BitBuffer.ES_Start_p = InitParams_p->BitBufferAddress_p;

#ifdef STVID_VALID
	PARSER_Data_p->BitBuffer.ES_LastFrameEndAddrGivenToDecoder_p = PARSER_Data_p->BitBuffer.ES_Start_p;
#endif /* STVID_VALID */
    /* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
    PARSER_Data_p->BitBuffer.ES_Stop_p = (void *)((U32)InitParams_p->BitBufferAddress_p + InitParams_p->BitBufferSize -1);

#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Data_p->BitBuffer.ES_StoreStart_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    PARSER_Data_p->BitBuffer.ES_StoreStop_p = PARSER_Data_p->BitBuffer.ES_Stop_p;
#endif  /* STVID_TRICKMODE_BACKWARD */

	/* Initialize the SC buffer pointers */
	vc1par_InitSCBuffer(ParserHandle);

	/* Reset ES bit buffer write pointer */
	PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

	/* Synchronize buffer definitions with FDMA */
	/* link this init with a fdma channel */
    /*------------------------------------*/
    Params.TransferDoneFct                 = vc1par_DMATransferDoneFct;
    Params.UserIdent                       = (U32)ParserHandle;
    Params.AvmemPartition                  = InitParams_p->AvmemPartitionHandle;
    Params.CPUPartition                    = InitParams_p->CPUPartition_p;

    PARSER_Data_p->Inject.InjectNum        = VIDINJ_TransferGetInject(PARSER_Data_p->Inject.InjecterHandle, &Params);
    if (PARSER_Data_p->Inject.InjectNum == BAD_INJECT_NUM)
    {
        /* ErrorCode = */ vc1par_DeAllocatePESBuffer(ParserHandle);
        /* ErrorCode = */ vc1par_DeAllocateSCBuffer(ParserHandle);
        return(ST_ERROR_NO_MEMORY);
    }

	return (ErrorCode);
} /* end of vc1par_InitInjectionBuffers */


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
static ST_ErrorCode_t vc1par_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t * PARSER_Properties_p;

    U32 UserDataSize;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;

    if ((ParserHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    PARSER_Properties_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;

    /* Allocate PARSER PrivateData structure */
    PARSER_Properties_p->PrivateData_p = (VC1ParserPrivateData_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VC1ParserPrivateData_t));
    if (PARSER_Properties_p->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    memset (PARSER_Properties_p->PrivateData_p, 0xA5, sizeof(VC1ParserPrivateData_t)); /* fill in data with 0xA5 by default */

    /* Use the short cut PARSER_Data_p for next lines of code */
    PARSER_Data_p = (VC1ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
	/*
	 Make sure the Profile is in the state uninitialized, in order to no check Signature for the Advanced profile and Sequence Header
	*/
	PARSER_Data_p->PrivateGlobalContextData.Profile            = VC9_PROFILE_UNKNOWN;
#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */

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

    PARSER_Data_p->UserDataSize = UserDataSize;



    /* Initialise commands queue */
    PARSER_Data_p->ForTask.ControllerCommand = FALSE;

    /* Allocation and Initialisation of injection buffers. The FDMA is also informed about these allocations */
    ErrorCode = vc1par_InitInjectionBuffers(ParserHandle, InitParams_p, SC_LIST_SIZE_IN_ENTRY, PES_BUFFER_SIZE);
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
    PARSER_Data_p->IsBitBufferFullyParsed  = FALSE;
	PARSER_Data_p->StopParsing = FALSE;
#endif
#endif /*ST_speed*/

        /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName,PARSER_JOB_COMPLETED_EVT,
                                          &(PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VC1 parser init: could not register JOB COMPLETED event !"));
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_PICTURE_SKIPPED_EVT,
                                          &(PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID]));

    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VC1 parser init: could not register PICTURE_SKIPPED event !"));
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

#ifdef STVID_TRICKMODE_BACKWARD
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_BITBUFFER_FULLY_PARSED_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VC1 parser init: could not register events !"));
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
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VC1 parser init: could not register events !"));
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p);
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, PARSER_Properties_p->PrivateData_p);
        PARSER_Properties_p->PrivateData_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Semaphore to share variables between parser and injection tasks */
    PARSER_Data_p->InjectSharedAccess = STOS_SemaphoreCreateFifo(InitParams_p->CPUPartition_p,1);

    /* Semaphore between parser and controller tasks */
    /* Parser main task does not run until it gets a start command from the controller */
    PARSER_Data_p->ParserOrder = STOS_SemaphoreCreateFifo(InitParams_p->CPUPartition_p, 0);
    PARSER_Data_p->ParserSC    = STOS_SemaphoreCreateFifoTimeOut(InitParams_p->CPUPartition_p, 0);

    /* Semaphore to share variables between parser and controller tasks */
    PARSER_Data_p->ControllerSharedAccess = STOS_SemaphoreCreateFifo(InitParams_p->CPUPartition_p, 1);

    PARSER_Data_p->ParserState = PARSER_STATE_IDLE;

    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;

    /* Create parser task */
    PARSER_Data_p->ParserTask.IsRunning = FALSE;
    ErrorCode = vc1par_StartParserTask(ParserHandle);
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

    return (ErrorCode);

} /* End of PARSER_Init function */

/*******************************************************************************/
/*! \brief Set the Stream Type (PES or ES) for the PES parser
 * \param ParserHandle Parser handle,
 * \param StreamType ES or PES
 * \return void
 */
/*******************************************************************************/
void vc1par_SetStreamType(const PARSER_Handle_t ParserHandle, const STVID_StreamType_t StreamType)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
void vc1par_SetPESRange(const PARSER_Handle_t ParserHandle,
                         const PARSER_StartParams_t * const StartParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* FDMA setup ----------------  */
	vc1par_SetStreamType(ParserHandle, StartParams_p->StreamType);

	/* Current assumption is that the PTI is filtering the StreamID yet.
	 * So, at that stage, the FDMA receives only the right StreamID, whatever that StreamID
	 * Regardless of the StreamID parameter sent by the controller, the FDMA is then programmed
	 * to look for the full range of StreamID
	 */
	PARSER_Data_p->PESBuffer.PESSCRangeStart = SMALLEST_VIDEO_PACKET_START_CODE;
	PARSER_Data_p->PESBuffer.PESSCRangeEnd   = GREATEST_VIDEO_PACKET_START_CODE;
	PARSER_Data_p->PESBuffer.PESOneShotEnabled = FALSE;
}


/***********************************************************
 * \brief Initializes Intensity Compensation information in the
 *        picture specific data.
 *
 * \param ParserHandle Parser handle
 * \return void
 ***********************************************************/
void vc1par_InitIntComp(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if(PARSER_Data_p->IntensityCompAvailablePictSpecific)
	{
		PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_1		= VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->PictureSpecificData.IntComp.ForwTop_2		= VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_1		= VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->PictureSpecificData.IntComp.ForwBot_2		= VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->PictureSpecificData.IntComp.BackTop_1		= VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->PictureSpecificData.IntComp.BackTop_2		= VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->PictureSpecificData.IntComp.BackBot_1		= VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->PictureSpecificData.IntComp.BackBot_2		= VC9_NO_INTENSITY_COMP;
		PARSER_Data_p->IntensityCompAvailablePictSpecific = FALSE;
	}
}/* end vc1par_InitIntComp() */


/***********************************************************
 * \brief Initializes all error flags to FALSE
 *
 * \param ParserHandle Parser handle
 * \return void
 ***********************************************************/
void vc1par_InitErrorFlags(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ProfileOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.LevelOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ColorDiffFormatOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedWidthOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedHeightOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ReservedBitOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.SampleAspectRatioOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.FrameRateNROutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.FrameRateDROutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ColorPrimariesOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.TransferCharOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MatrixCoefOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.NoC5inRCVFormat = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.No00000004inRCVFormat = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.FrameRateOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES1OutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES2OutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES3OutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES4OutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES5OutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES6OutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES7OutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.LoopFilterFlagSPOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.SyncMarkerFlagSPOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RangeredFlagSPOutOfRange = FALSE;
	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.No0000000CinRCVFormat = FALSE;


	PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.BrokenLinkOutOfRange = FALSE;
	PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedWidthOutOfRange = FALSE;
	PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedHeightOutOfRange = FALSE;


	PARSER_Data_p->PictureSpecificData.PictureError.MissingReferenceInformation = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.SecondFieldWithoutFirstFieldError  = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.NoSecondFieldAfterFirstFieldError  = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.RefDistOutOfRange  = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.BFractionOutOfRange  = FALSE;
	PARSER_Data_p->PictureSpecificData.PictureError.RESOutOfRange = FALSE;

	PARSER_Data_p->ParserJobResults.ParsingJobStatus.HasErrors = FALSE;
	PARSER_Data_p->ParserJobResults.ParsingJobStatus.ErrorCode = PARSER_NO_ERROR;
}/* end void vc1par_InitErrorFlags()*/

/******************************************************************************/
/*! \brief Initializes the reference frame informations
 *
 * \param ParserHandle the parser handle
 * \return void
 */
/******************************************************************************/
static void vc1par_InitRefFrameInfo(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	U32 i; /* loop index */

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;

	for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; i++)
	{
		PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i]		= FALSE;
		PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i]	= FALSE;
		PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[i]				= TRUE;
		PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[i]				= TRUE;
		PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[i]				= TRUE;
		PARSER_Data_p->StreamState.FieldPairFlag[i]                             = FALSE;
	}
}

/******************************************************************************/
/*! \brief Initializes the parser context
 *
 * \param ParserHandle the parser handle
 * \return void
 */
/******************************************************************************/
void vc1par_InitParserContext(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	U32 i; /* loop index */

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->StartCode.IsPending = FALSE; /* No start code already found */
	PARSER_Data_p->StartCode.SubBDUPending = FALSE;
	PARSER_Data_p->StartCode.SliceFound = FALSE;
#ifdef STVID_VALID
	PARSER_Data_p->SCBuffer.InjectFileFrameNum = 0;
#endif
	/* PARSER_Data_p->PreviousPictureInDecodingOrder.IsAvailable = FALSE; */

    /* PARSER_Data_p->StreamState.ExtendedPresentationOrderPictureID = 0; */


	PARSER_Data_p->PrivateGlobalContextData.GlobalContextValid = FALSE;
	PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;

	PARSER_Data_p->PrivatePictureContextData.PictureContextValid = FALSE;

	PARSER_Data_p->ExpectingFieldBDU = FALSE;
	PARSER_Data_p->SeqEndAfterThisBDU = FALSE;
	PARSER_Data_p->NextPicRandomAccessPoint = FALSE;

	/* No AUD (Access Unit Delimiter) for next picture */
	PARSER_Data_p->PictureLocalData.PrimaryPicType = PRIMARY_PIC_TYPE_UNKNOWN;
    /* No PTS found for next picture */
    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;

	PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.ID          = 0;
	PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID.IDExtension = 0;
	PARSER_Data_p->StreamState.CurrentTfcntr                                     = 0;
    PARSER_Data_p->StreamState.BackwardRefTfcntr                                 = 0;
	PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.ID              = 0;
	PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID.IDExtension     = 0;
	PARSER_Data_p->StreamState.WaitingRefPic				                     = FALSE;
	PARSER_Data_p->StreamState.IntensityCompPrevRefPicture                       = VC9_NO_INTENSITY_COMP;
	PARSER_Data_p->StreamState.RespicBBackwardRefPicture                         = 0;
	PARSER_Data_p->StreamState.RespicBForwardRefPicture                          = 0;
	PARSER_Data_p->StreamState.ForwardRangeredfrmFlagBFrames                     = 0;
	PARSER_Data_p->StreamState.ForwardRangeredfrmFlagPFrames					 = 0;
	PARSER_Data_p->StreamState.FrameRatePresent									 = FALSE;
	PARSER_Data_p->StreamState.PreviousTimestamp                                 = 0xFFFFFFFF;
	/* The first reference picture will be placed at index zero in the FullReferenceFrameList. */
	PARSER_Data_p->StreamState.NextReferencePictureIndex						 = 0;
	PARSER_Data_p->StreamState.RefDistPreviousRefPic							 = 0;
	PARSER_Data_p->StreamState.IntComp[0].IntCompAvailable						 = FALSE;
	PARSER_Data_p->StreamState.IntComp[0].Top_1									 = VC9_NO_INTENSITY_COMP;
	PARSER_Data_p->StreamState.IntComp[0].Top_2									 = VC9_NO_INTENSITY_COMP;
	PARSER_Data_p->StreamState.IntComp[0].Bot_1									 = VC9_NO_INTENSITY_COMP;
	PARSER_Data_p->StreamState.IntComp[0].Bot_2									 = VC9_NO_INTENSITY_COMP;
	PARSER_Data_p->StreamState.IntComp[1].IntCompAvailable						 = FALSE;
	PARSER_Data_p->StreamState.IntComp[1].Top_1									 = VC9_NO_INTENSITY_COMP;
	PARSER_Data_p->StreamState.IntComp[1].Top_2									 = VC9_NO_INTENSITY_COMP;
	PARSER_Data_p->StreamState.IntComp[1].Bot_1									 = VC9_NO_INTENSITY_COMP;
	PARSER_Data_p->StreamState.IntComp[1].Bot_2									 = VC9_NO_INTENSITY_COMP;
	PARSER_Data_p->StreamState.ForwardReferencePictureSyntax					 = VC9_PICTURE_SYNTAX_PROGRESSIVE;
	PARSER_Data_p->StreamState.BackwardReferencePictureSyntax					 = VC9_PICTURE_SYNTAX_PROGRESSIVE;

	/* Time code set to 0 */

	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate			= VC1_DEFAULT_FRAME_RATE;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame			= STVID_MPEG_FRAME_I;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure		= STVID_PICTURE_STRUCTURE_FRAME;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst		= TRUE;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.CompressionLevel		= STVID_COMPRESSION_LEVEL_NONE;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.DecimationFactors	= STVID_DECIMATION_FACTOR_NONE;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Seconds		= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Minutes		= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Hours		= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TimeCode.Frames		= 0;

#if defined(VALID_TOOLS)
#if defined(STVID_VALID_DISPLAY_CRC_MODE) || defined(STVID_VALID_SOFTWARE_CRC_MODE)
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.IsMBAFFPicture = FALSE;
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.IsMonochrome   = FALSE;
#endif /* defined(STVID_VALID_DISPLAY_CRC_MODE) || defined(STVID_VALID_SOFTWARE_CRC_MODE) */
#endif /* defined(VALID_TOOLS) */

	PARSER_Data_p->PictureGenericData.DecodingOrderFrameID									= 0;
	PARSER_Data_p->PictureGenericData.IsReference											= FALSE;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0								= 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0								= 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1								= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType						= STGXOBJ_PROGRESSIVE_SCAN;
	PARSER_Data_p->PictureGenericData.AsynchronousCommands.FlushPresentationQueue			= FALSE;
	PARSER_Data_p->PictureGenericData.AsynchronousCommands.FreezePresentationOnThisFrame	= FALSE;
	PARSER_Data_p->PictureGenericData.AsynchronousCommands.ResumePresentationOnThisFrame	= FALSE;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid  = FALSE;
	PARSER_Data_p->PictureGenericData.RepeatFirstField										= FALSE;
	PARSER_Data_p->PictureGenericData.RepeatProgressiveCounter								= 0;
	PARSER_Data_p->PictureGenericData.NumberOfPanAndScan									= 0;
	PARSER_Data_p->PictureGenericData.IsDisplayBoundPictureIDValid							= FALSE;
	PARSER_Data_p->PictureGenericData.IsFirstOfTwoFields									= FALSE;
	PARSER_Data_p->PictureGenericData.IsDecodeStartTimeValid								= FALSE;
	PARSER_Data_p->PictureGenericData.IsPresentationStartTimeValid							= FALSE;
	PARSER_Data_p->PictureGenericData.ParsingError											= VIDCOM_ERROR_LEVEL_NONE;
    PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid	                = FALSE;
	PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID					= 0;
	PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension		= 0;
	PARSER_Data_p->PictureGenericData.DisplayBoundPictureID.ID								= 0;
	PARSER_Data_p->PictureGenericData.DisplayBoundPictureID.IDExtension						= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS						= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33					= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated				= 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width                       = VC1_DEFAULT_WIDTH;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height                      = VC1_DEFAULT_HEIGHT;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Pitch						= VC1_DEFAULT_WIDTH;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorType                   = VC1_DEFAULT_COLOR_TYPE;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BitmapType					= VC1_DEFAULT_BITMAP_TYPE;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.PreMultipliedColor			= VC1_DEFAULT_PREMULTIPLIED_COLOR;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorY   = YUV_NO_RESCALE;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.YUVScaling.ScalingFactorUV  = YUV_NO_RESCALE;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Offset						= VC1_DEFAULT_BITMAPPARAMS_OFFSET;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.SubByteFormat				= VC1_DEFAULT_SUBBYTEFORMAT;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BigNotLittle				= VC1_DEFAULT_BIGNOTLITTLE;


	for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; i++)
	{
		PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i]		= FALSE;
		PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i]	= FALSE;
		PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldP0[i]				= TRUE;
		PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB0[i]				= TRUE;
		PARSER_Data_p->PictureSpecificData.IsReferenceTopFieldB1[i]				= TRUE;
		PARSER_Data_p->StreamState.FieldPairFlag[i]                             = FALSE;
	}

	PARSER_Data_p->PictureSpecificData.RndCtrl						  = 0;
	PARSER_Data_p->PictureSpecificData.NumeratorBFraction			  = 0;
	PARSER_Data_p->PictureSpecificData.DenominatorBFraction			  = 0;
	PARSER_Data_p->PictureSpecificData.BackwardRefDist				  = 0;
	PARSER_Data_p->PictureSpecificData.PictureType					  = VC9_PICTURE_TYPE_I;
	PARSER_Data_p->PictureSpecificData.ForwardRangeredfrmFlag		  = FALSE;
	PARSER_Data_p->PictureSpecificData.HalfWidthFlag				  = FALSE;
	PARSER_Data_p->PictureSpecificData.HalfHeightFlag				  = FALSE;
	PARSER_Data_p->PictureSpecificData.FramePictureLayerSize		  = 0;
	PARSER_Data_p->PictureSpecificData.TffFlag						  = 1;
	PARSER_Data_p->PictureSpecificData.SecondFieldFlag				  = 0;
	PARSER_Data_p->PictureSpecificData.RefDist						  = 0;
	PARSER_Data_p->PictureSpecificData.NumSlices					  = 0;
	PARSER_Data_p->PictureSpecificData.PictureSyntax				  = VC9_PICTURE_SYNTAX_PROGRESSIVE;
	PARSER_Data_p->PictureSpecificData.ForwardReferencePictureSyntax  = VC9_PICTURE_SYNTAX_PROGRESSIVE;
	PARSER_Data_p->PictureSpecificData.BackwardReferencePictureSyntax = VC9_PICTURE_SYNTAX_PROGRESSIVE;

	for(i = 0; i < MAX_SLICE; i++)
	{
		PARSER_Data_p->PictureSpecificData.SliceArray[i].SliceAddress = 0;
		PARSER_Data_p->PictureSpecificData.SliceArray[i].SliceStartAddrCompressedBuffer_p = NULL;
	}

	/* Set this to TRUE initially so that vc1par_InitIntComp will
	   successfully clear out the intensity compensation info and
	   set IntensityCompAvailablePictSpecific back to FALSE */
	PARSER_Data_p->IntensityCompAvailablePictSpecific = TRUE;
	vc1par_InitIntComp(ParserHandle);

    memset(&(PARSER_Data_p->PictureDecodingContextSpecificData), 0, sizeof(VC1_PictureDecodingContextSpecificData_t)); /* Reset all data to 0 first */

    memset(&(PARSER_Data_p->GlobalDecodingContextSpecificData), 0, sizeof(VC1_GlobalDecodingContextSpecificData_t)); /* Reset all data to 0 first */
	PARSER_Data_p->GlobalDecodingContextSpecificData.AebrFlag                            = TRUE;  /* Default to using Anti Emulation Byte removal */

	PARSER_Data_p->GlobalDecodingContextGenericData.ParsingError	                      = VIDCOM_ERROR_LEVEL_NONE;

	PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.LeftOffset            = 0;
	PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.RightOffset           = 0;
	PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.TopOffset             = 0;
	PARSER_Data_p->GlobalDecodingContextGenericData.FrameCropInPixel.BottomOffset          = 0;

    PARSER_Data_p->GlobalDecodingContextGenericData.NumberOfReferenceFrames                 = 2;
	PARSER_Data_p->GlobalDecodingContextGenericData.MaxDecFrameBuffering                    = 2;    /* Number of reference frames (this number +2 gives the number of frame buffers needed for decode */

    PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries         = UNSPECIFIED_COLOR_PRIMARIES;
    PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics = UNSPECIFIED_TRANSFER_CHARACTERISTICS;
    PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients      = MATRIX_COEFFICIENTS_UNSPECIFIED;

	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height						= VC1_DEFAULT_HEIGHT;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width						= VC1_DEFAULT_WIDTH;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect						= STVID_DISPLAY_ASPECT_RATIO_16TO9;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ScanType					= STVID_SCAN_TYPE_PROGRESSIVE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate					= VC1_DEFAULT_FRAME_RATE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate					= VC1_DEFAULT_BIT_RATE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard				= STVID_MPEG_STANDARD_SMPTE_421M;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.IsLowDelay					= FALSE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VBVBufferSize				= VC1_DEFAULT_VBVMAX;
	/* NOTE StreamID has already been set earlier */
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication	= ((VC9_PROFILE_ADVANCE << 8) | VC1_DEFAULT_LEVEL);

	/* Default to detecting Start Code Emulation Prevention Bytes */
	vc1par_EnableDetectionOfStartCodeEmulationPreventionBytes(ParserHandle);

	PARSER_Data_p->PTS_SCList.Write_p          = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
    PARSER_Data_p->PTS_SCList.Read_p           = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
    PARSER_Data_p->LastPTSStored.Address_p     = NULL;

	/* initialize all error flags to false. */
	vc1par_InitErrorFlags(ParserHandle);

	/* @@@ Need to copy default context info here */

}/* end vc1par_InitParserContext() */

/*******************************************************************************/
/*! \brief Sets the ES Range parameters to program the FDMA
 *
 * \param ParserHandle Parser handle
 * \return void
 */
/*******************************************************************************/
void vc1par_SetESRange(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->BitBuffer.ES0RangeEnabled   = TRUE;
	PARSER_Data_p->BitBuffer.ES0RangeStart     = SMALLEST_BDU_PACKET_START_CODE;
	PARSER_Data_p->BitBuffer.ES0RangeEnd       = GREATEST_BDU_PACKET_START_CODE;
	PARSER_Data_p->BitBuffer.ES0OneShotEnabled = FALSE;

	/* Range 1 is not used for VC1 */
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
static ST_ErrorCode_t vc1par_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p)
{
	ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
    VIDINJ_ParserRanges_t       ParserRanges[3];

/* TODO : Add processing of new StartParams_p data :
    BOOL                            OverwriteFullBitBuffer;
    void *                          BitBufferWriteAddress_p;
    VIDCOM_GlobalDecodingContext_t *  DefaultGDC_p;
    VIDCOM_PictureDecodingContext_t * DefaultPDC_p;
*/
#ifdef STVID_TRICKMODE_BACKWARD
	/*VIDCOM_PictureID_t		 	CurrentEPOID;*/
#endif

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    ErrorCode = ST_NO_ERROR;

    /* TODO: what do we do with BroadcastProfile ? */

    /* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Reset pointers in injection module */
    VIDINJ_TransferReset(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);


    /* Save for future use: is inserted "as is" in the GlobalDecodingContextGenericData.SequenceInfo.StreamID structure */
    PARSER_Data_p->StreamState.StreamID = StartParams_p->StreamID;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.StreamID = StartParams_p->StreamID;

    PARSER_Data_p->DiscontinuityDetected = FALSE;
	/* FDMA Setup ----------------- */
	vc1par_SetPESRange(ParserHandle, StartParams_p);

	vc1par_SetESRange(ParserHandle);

	/* Specify the transfer buffers and parameters to the FDMA */
    /* cascade the buffers params and sc-detect config to inject sub-module */

	/* VC1 SC Ranges*/

    ParserRanges[0].RangeId                     = STFDMA_DEVICE_VC1_RANGE_0;
    ParserRanges[0].RangeConfig.RangeEnabled    = PARSER_Data_p->BitBuffer.ES0RangeEnabled;
    ParserRanges[0].RangeConfig.RangeStart      = PARSER_Data_p->BitBuffer.ES0RangeStart;
    ParserRanges[0].RangeConfig.RangeEnd        = PARSER_Data_p->BitBuffer.ES0RangeEnd;
    ParserRanges[0].RangeConfig.PTSEnabled      = FALSE;
    ParserRanges[0].RangeConfig.OneShotEnabled  = PARSER_Data_p->BitBuffer.ES0OneShotEnabled;
/*
    ParserRanges[0].RangeId                     = STFDMA_DEVICE_ES_RANGE_0;
    ParserRanges[0].RangeConfig.RangeEnabled    = PARSER_Data_p->BitBuffer.ES0RangeEnabled;
    ParserRanges[0].RangeConfig.RangeStart      = PARSER_Data_p->BitBuffer.ES0RangeStart;
    ParserRanges[0].RangeConfig.RangeEnd        = PARSER_Data_p->BitBuffer.ES0RangeEnd;
    ParserRanges[0].RangeConfig.PTSEnabled      = FALSE;
    ParserRanges[0].RangeConfig.OneShotEnabled  = PARSER_Data_p->BitBuffer.ES0OneShotEnabled;
*/
	/* ES Range 1 is not used in VC1 */
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
         /* PES */          PARSER_Data_p->PESBuffer.PESBufferBase_p,
                            PARSER_Data_p->PESBuffer.PESBufferTop_p
#if defined(DVD_SECURED_CHIP)
         /* ES Copy */     ,PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,
                            PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p
#endif /* DVD_SECURED_CHIP */
                           );

    STTBX_Print(("\nSTART ES buffer Base:%lx - Top:%lx\nSC buffer Base:%lx - Top:%lx\nPES buffer Base:%lx - Top:%lx\n",
             /* BB */       (unsigned long)(PARSER_Data_p->BitBuffer.ES_Start_p),
                            (unsigned long)(PARSER_Data_p->BitBuffer.ES_Stop_p),
             /* SC */       (unsigned long)(PARSER_Data_p->SCBuffer.SCList_Start_p),
                            (unsigned long)(PARSER_Data_p->SCBuffer.SCList_Stop_p),
             /* PES */      (unsigned long)(PARSER_Data_p->PESBuffer.PESBufferBase_p),
                            (unsigned long)(PARSER_Data_p->PESBuffer.PESBufferTop_p)));

#if defined(DVD_SECURED_CHIP)
        STTBX_Print(("ESCopy buffer Base:%lx - Top:%lx\n",
             /* ES Copy */      (unsigned long)(PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p),
                                (unsigned long)(PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p)));
#endif /* DVD_SECURED_CHIP */

	VIDINJ_SetSCRanges(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum,3, ParserRanges);
		/* sc-detect */		/*3, ParserRanges);*/


	/* Run the FDMA */
    VIDINJ_TransferStart(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, StartParams_p->RealTime);

	/* End of FDMA setup ---------- */

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* @@@ need to set initial global and picture context info here. */
 	/* Initialize the parser variables */
    if (StartParams_p->ResetParserContext)
    {
        vc1par_InitParserContext(ParserHandle);
    }

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
void vc1par_DMATransferDoneFct(U32 ParserHandle,void * ES_Write_p,
                               void * SCListWrite_p,
                               void * SCListLoop_p
#if defined(DVD_SECURED_CHIP)
                             , void * ESCopy_Write_p
#endif  /* DVD_SECURED_CHIP */
)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STOS_SemaphoreWait(PARSER_Data_p->InjectSharedAccess);
#ifdef STVID_TRICKMODE_BACKWARD
    if ((!PARSER_Data_p->Backward) || (PARSER_Data_p->StopParsing) )
#endif /* STVID_TRICKMODE_BACKWARD */
	{
		PARSER_Data_p->BitBuffer.ES_Write_p     = ES_Write_p;
    	PARSER_Data_p->SCBuffer.SCList_Write_p  = SCListWrite_p;
    	PARSER_Data_p->SCBuffer.SCList_Loop_p   = SCListLoop_p;

#if defined(DVD_SECURED_CHIP)
    	PARSER_Data_p->ESCopyBuffer.ESCopy_Write_p  = ESCopy_Write_p;
        TrInject(("\r\nPARSER: Inject sends ESCopy_Write_p=0x%08x", ESCopy_Write_p));
#endif  /* defined(DVD_SECURED_CHIP) */
	}
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
	STOS_SemaphoreSignal(PARSER_Data_p->InjectSharedAccess);
} /* End of vc1par_DMATransferDoneFct() function */

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
static ST_ErrorCode_t vc1par_GetPicture(const PARSER_Handle_t ParserHandle,
										const PARSER_GetPictureParams_t * const GetPictureParams_p)
{
    U32 TempDecodeOrder;
	VIDCOM_PictureID_t TempPresId0, TempPresId1;

	/* dereference ParserHandle to a local variable to ease debug */
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Copy GetPictureParams in local data structure */
	PARSER_Data_p->GetPictureParams.GetRandomAccessPoint = GetPictureParams_p->GetRandomAccessPoint;
#ifdef ST_speed
    PARSER_Data_p->SkipMode = GetPictureParams_p->DecodedPictures;
#endif

	/* When the producer asks for a recovery point, erase context:
	 * - DPB is emptied
	 * - previous picture information is discarded
     * - global variables are reset but keep the current DecodingOrderFrameID and PresentationOrder information to keep reodering consistent
	 * The State of the parser is the same as for a "Start" command, except the FDMA is not run again
	 */
    if (PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)
	{
        TempDecodeOrder = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
        TempPresId0  = PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID;
        TempPresId1  = PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID;
        /* When playing a VC1 file the sequence info is available only at the beginning of the file so when there is a request for a new random access point we should not re-init all the info of the parser otherwise we'll loose all the sequence info context.
            Use the function "vc1par_InitRefFrameInfo" instead of "vc1par_InitParserContext" during a get random access point process in order to initializes the reference frame informations. */
        vc1par_InitRefFrameInfo(ParserHandle);
        PARSER_Data_p->PictureGenericData.DecodingOrderFrameID = TempDecodeOrder;
        /* CL comment: shall we increment TempPresId0.IDExtension as we skip to a next entry point ? */
        /*TempPresId0.IDExtension++;*/
        PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID = TempPresId0;
        /* CL comment: shall we increment TempPresId1.IDExtension as we skip to a next entry point ? */
        /*TempPresId1.IDExtension++;*/
        PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID = TempPresId1;
	}

    PARSER_Data_p->StreamState.ErrorRecoveryMode = GetPictureParams_p->ErrorRecoveryMode;
	/* Fill job submission time */
	PARSER_Data_p->ParserJobResults.ParsingJobStatus.JobSubmissionTime = time_now();

	/* If the parser task, has just notified the producer of a new parsed picture give the parser task a chance to
	   change states before we check the state. */
	STOS_SemaphoreWait(PARSER_Data_p->ControllerSharedAccess);
	/* Wakes up Parser task */
	if (PARSER_Data_p->ParserState == PARSER_STATE_PARSING)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module VC1 parser GetPicture: controller attempts to send a parser command when the parser is already working !"));
	}
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
ST_ErrorCode_t vc1par_StartParserTask(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    VIDCOM_Task_t * const Task_p = &(PARSER_Data_p->ParserTask);
    char TaskName[23] = "STVID[?].VC1ParserTask";

	if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + ((PARSER_Properties_t *) ParserHandle)->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

	/* TODO: define PARSER own priority in vid_com.h */
    if (STOS_TaskCreate((void (*) (void*)) vc1par_ParserTaskFunc,
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
ST_ErrorCode_t vc1par_StopParserTask(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t * PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
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
static ST_ErrorCode_t vc1par_Term(const PARSER_Handle_t ParserHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Properties_t     * PARSER_Properties_p;

	PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	PARSER_Data_p = (VC1ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

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
    vc1par_StopParserTask(ParserHandle);

	/* Delete semaphore to share variables between parser and injection tasks */
	STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->InjectSharedAccess);

	/* Delete semaphore between parser and controler tasks */
	STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ParserOrder);
	/* Delete semaphore for SC lists */
	STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ParserSC);
	/* Delete semaphore between controller and parser */
	STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p, PARSER_Data_p->ControllerSharedAccess);

   /* Deallocate the STAVMEM partitions */
	/* PES Buffer */
	if(PARSER_Data_p->PESBuffer.PESBufferBase_p != 0)
    {
        /* ErrorCode = */ vc1par_DeAllocatePESBuffer(ParserHandle);
    }
	/* SC Buffer */
	if(PARSER_Data_p->SCBuffer.SCList_Start_p != 0)
    {
        /* ErrorCode = */ vc1par_DeAllocateSCBuffer(ParserHandle);
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
static ST_ErrorCode_t vc1par_Stop(const PARSER_Handle_t ParserHandle)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* Stops the FDMA */
    VIDINJ_TransferStop(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
	/* TODO: is task "InjecterTask" also stopped anywhere ? */

	/* TODO: what about terminating the parser task ? */

	/* Initialize the SC buffer pointers */
	vc1par_InitSCBuffer(ParserHandle);


	/* Reset ES bit buffer write pointer */
	PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

#if defined(DVD_SECURED_CHIP)
    /* Reset ESCopyBuffer write pointer */
    PARSER_Data_p->ESCopyBuffer.ESCopy_Write_p  = PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p;
#endif  /* defined(DVD_SECURED_CHIP) */

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	return (ErrorCode);
}

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
static ST_ErrorCode_t vc1par_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    VIDINJ_GetStatistics(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, Statistics_p);

    /* Statistics_p->??? = PARSER_Data_p->Statistics.???; */

	return (ErrorCode);
} /* end of vc1par_GetStatistics() */

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
static ST_ErrorCode_t vc1par_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
} /* end of vc1par_ResetStatistics() */
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
static ST_ErrorCode_t vc1par_GetStatus(const PARSER_Handle_t ParserHandle, STVID_Status_t * const Status_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    VIDINJ_GetStatus(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, Status_p);

    /* Status_p->??? = PARSER_Data_p->Status.???; */

	return (ErrorCode);
} /* end of vc1par_GetStatus() */
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
void vc1par_PointOnNextSC(const PARSER_Handle_t ParserHandle)
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
#ifdef STVID_VALID
	int i = 0;
	FILE * File_p;
#endif
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
    PARSER_Data_p->LastSCAdd_p = (void*)(PARSER_Data_p->StartCode.StartAddress_p);
#endif /* ST_speed */
}
#ifdef OLD_SLICE_METHOD
/*******************************************************************************/
/*! \brief Point on Next Entry in Start Code List after either a Slice Array
 *         or a Slice User Data array
 *
 *
 * \param SliceElementSize - the size of either the slice element
 *                           or the slice user data element
 *        ParserHandle
 *
 */
/*******************************************************************************/
void vc1par_SkipSliceInfoInSCBuffer(U32 SliceElementSize, const PARSER_Handle_t ParserHandle)
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
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Skip over the slice array */
	PARSER_Data_p->SCBuffer.NextSCEntry = (STFDMA_SCEntry_t *)(((U8 *) PARSER_Data_p->SCBuffer.NextSCEntry) + (SliceElementSize * MAX_SLICE));

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
}
#endif /* OLD_SLICE_METHOD */

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
/*******************************************************************************/
/*
 *
 check the signature of the start code regarding the state of the Profile
 and return TRUE, if the start code has a signature
 *
 */
/*******************************************************************************/
static int vc1par_CheckSCSignature(register unsigned char *pPayload,unsigned char SCValue, const PARSER_Handle_t  ParserHandle)
{

    U8 * SHAddress_p;
    U8 * StartAddress_p;
    U8 * StopAddress_p;

    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t       * PARSER_Properties_p ;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = ((VC1ParserPrivateData_t *)(PARSER_Properties_p)->PrivateData_p);
    SHAddress_p = (U8 *)pPayload;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    StartAddress_p = STAVMEM_DeviceToCPU((void *)(PARSER_Data_p->BitBuffer.ES_Start_p),&VirtualMapping);
    StopAddress_p = STAVMEM_DeviceToCPU((void *)(PARSER_Data_p->BitBuffer.ES_Stop_p),&VirtualMapping);

	if(SHAddress_p == NULL)   return FALSE;

	if(*SHAddress_p++ != 'S') return FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != 'T') return FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != 'M') return FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != 'I') return FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != 'C') return FALSE;
	/* Set the current pointer to the start address if the next pointer is greater then the stop address */
	if ( SHAddress_p > StopAddress_p )
        SHAddress_p = StartAddress_p;

	if(*SHAddress_p++ != SCValue) return FALSE;

	return TRUE;
}


#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */


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
void vc1par_FindUsableStartCodeBDU(const PARSER_Handle_t ParserHandle)
{
/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "vc1par_FindUsableStartCodeBDU  "));
#endif*/

	/* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t                * PARSER_Data_p;
    BOOL                                    SCFound = FALSE;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STFDMA_SCEntry_t                      * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
    U32                                     SCValue, TempDecodeOrder;
    VIDCOM_PictureID_t                      TempPresId0, TempPresId1;
    PARSER_Properties_t       * PARSER_Properties_p ;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = ((VC1ParserPrivateData_t *)(PARSER_Properties_p)->PrivateData_p);
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

	if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p)
    {
		SCFound = FALSE;
	    /* search for available SC in SC+PTS buffer : */
        while((PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p) && (!(SCFound)))
	    {
			CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry,&VirtualMapping);
            if(!IsPTS(CPUNextSCEntry_p))
			{
				SCValue = GetSCVal(CPUNextSCEntry_p);
#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
				if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_MAIN ||PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE )
				{
					/* we are in Main or Simple Profile
					 in order to prevent false Start Code
					 the injection is supposed to add a signature, just after
					 the start code 'STMIC' , if the signature is not present, we must ignore
					 the start code, it is a part of the payload FRAME of Main or simple profile, but the signature
					 is found, we have to process it
					*/
#if defined(DVD_SECURED_CHIP)
    				/* Set the Parser Read Pointer in ES Copy Buffer on the start address of the start code */
    				unsigned char *pPayload = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
    				/* Set the Parser Read Pointer in ES Bit Buffer on the start address of the start code */
					unsigned char *pPayload = STAVMEM_DeviceToCPU((void *)(CPUNextSCEntry_p->SC.Addr),&VirtualMapping);
#endif /* defined(DVD_SECURED_CHIP) */

					/*
					 Check the signature
					 The Payload point on the Start Code, +1 to point on the Signature

					*/

					if(vc1par_CheckSCSignature(pPayload+1,SCValue, ParserHandle))
					{
						/*
						 we have found a Start code with a correct signature , there is nothing to do
						 Because,  GetData will translate the point to the end of the signature and will point also on a Start code + payload
						*/
					}
					else
					{
						/*
						 the Start code don't have a right signature. it should be a data of the payload
						 for main and simple profile only
						*/
						SCValue=BDU_TYPE_SKIP;

					}
				}
#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */


#if defined(TRACE_UART) && defined(VALID_TOOLS)
				TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "vc1par: SC %x SCAddr %08x  ",GetSCVal(CPUNextSCEntry_p), GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p)));
#endif


				if(!PARSER_Data_p->ExpectingFieldBDU)
				{
					switch(SCValue)
					{
#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
						case BDU_TYPE_SKIP:
							{
								/*
								 This start code is a false start code
								 probably found in the FRAME payload of a main or simple VC-1 profile
								 we just have to ignore it, in order to merged it with the previous one
								*/
								SCFound = FALSE;
								break;
							}
#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */
						/* sequence header is accepted no matter what */
#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
						case BDU_TYPE_SH_MPSP:
#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */
						case BDU_TYPE_SH:
							PARSER_Data_p->BDUByteStream.WordSwapFlag = FALSE;
							SCFound = TRUE;
							break;
						case BDU_TYPE_SEQ_STRUCT_ABC:
							PARSER_Data_p->BDUByteStream.WordSwapFlag = TRUE;
							SCFound = TRUE;
							break;

					    /* entry point header and sequence level user data are accepted only
						   if a valid sequence header has been parsed succesfully */
						case BDU_TYPE_EPH:
							/* BDU_TYPE_EPH use as Random access point for SPMP Profile */
							/* SPMP Profile, start code added by application injection module */
							if((PARSER_Data_p->PrivateGlobalContextData.Profile != VC9_PROFILE_ADVANCE) &&
                                   (PARSER_Data_p->PrivatePictureContextData.PictureContextValid))
							{
								/* SPMP case */
								SCFound = TRUE;
								PARSER_Data_p->BDUByteStream.WordSwapFlag = FALSE;
								PARSER_Data_p->NextPicRandomAccessPoint = TRUE;
							}
							else
							{
    							if (PARSER_Data_p->PrivateGlobalContextData.GlobalContextValid)

    							{
    								SCFound = TRUE;
    								PARSER_Data_p->BDUByteStream.WordSwapFlag = FALSE;
    							}
							}
							break;
#if !defined STVID_USE_VC1_MP_SC_SIGNATURE
						case BDU_TYPE_SEQ_UD:
							if(PARSER_Data_p->PrivateGlobalContextData.GlobalContextValid)
							{
								SCFound = TRUE;
								PARSER_Data_p->BDUByteStream.WordSwapFlag = FALSE;
							}
							break;
#endif /* !defined STVID_USE_VC1_MP_SC_SIGNATURE */
						/* a frame layer data structure with the KEY set to zero
						   This is not a random access point so we should look at it if
						   we are not looking for a random access point and
						   the picture context is valid. */
						case BDU_TYPE_FRAME_LAYER_DATA_STRUCT_NO_KEY:

                            if(!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint))
							{
								/* Only accept a Main or Simple Profile Frame Layer Data Structure if
								   the Picture Context is valid.  The Advanced profile Frame layer
								   Data Structure does not require the Picture Context Valid because
								   the Global and Picture context info would come after the frame layer
								   data structure. */
								if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE) ||
                                   (PARSER_Data_p->PrivatePictureContextData.PictureContextValid))
								{
									SCFound = TRUE;
									PARSER_Data_p->BDUByteStream.WordSwapFlag = TRUE;
								}
                            }/* end if(!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)) */

							break;

						/* a frame layer data structure with the KEY set to one
						   This is a random access point so we should look at it if
						   the picture context is valid. */
						case BDU_TYPE_FRAME_LAYER_DATA_STRUCT_KEY:

						    /* Only accept a Main or Simple Profile Frame Layer Data Structure if
						       the Picture Context is valid.  The Advanced profile Frame layer
						       Data Structure does not require the Picture Context Valid because
						       the Global and Picture context info would come after the frame layer
							   data structure. */
							if((PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE) ||
                                (PARSER_Data_p->PrivatePictureContextData.PictureContextValid))
							{
								SCFound = TRUE;
								PARSER_Data_p->BDUByteStream.WordSwapFlag = TRUE;
							}

							PARSER_Data_p->NextPicRandomAccessPoint = FALSE;

							break;
						case BDU_TYPE_FRAME:

							/* We accept the frame BDU if
							     1. all global and picture context info
							        is  available (we assume that the Picture Context Data could not
							        be valid unless the Global Context Data were valid too)

							        and

							     2. either  a) a random access point has not been requested or
							                b) a random access point has been requested and the next
							                   picture is known to be a random access point.   */
                            if(PARSER_Data_p->PrivatePictureContextData.PictureContextValid)
                            {
                            /* workaround which allow to start on first I picture when requesting a random access point even without EPH */
                            /* If we are looking for a random access point the PARSER_JOB_COMPLETED_EVT is sent only if we parse a I picture */
                            /*
                                if ((!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)) ||
                                    ((PARSER_Data_p->GetPictureParams.GetRandomAccessPoint) &&
                                     (PARSER_Data_p->NextPicRandomAccessPoint))))
                            */
    							{
    								SCFound = TRUE;
    								PARSER_Data_p->NextPicRandomAccessPoint = FALSE;
    								PARSER_Data_p->BDUByteStream.WordSwapFlag = FALSE;
    							}
                            }
                            break;

						case BDU_TYPE_FIELD:

							STTBX_Print(("ERROR:  Received a (Second) Field BDU without receiving a First Field BDU\n"));
							PARSER_Data_p->PictureSpecificData.PictureError.SecondFieldWithoutFirstFieldError = TRUE;
							break;

						/* Slices are not valid in this function because they need to occur
						   when a frame start code has been received and before the end of
						   the frame is found.  Since this is not in the context of a valid
						   frame, we just need to skip over the area in which the slices are stored */
						case BDU_TYPE_SLICE:
/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
							TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Slice found (SC %x)  ", SCValue));
#endif*/
#ifdef OLD_SLICE_METHOD
							/* Make ScannedEntry_p point to the beginning of the slice array */
							vc1par_PointOnNextSC(ParserHandle);
							vc1par_SkipSliceInfoInSCBuffer(sizeof(VC9_SliceParam_t), ParserHandle);

#endif /* OLD_SLICE_METHOD */

							break;

						case BDU_TYPE_EP_UD:
							/* once again assume here that the Picture Context Data could not
							   be valid unless the Global Context Data were valid too */
							if(PARSER_Data_p->PrivatePictureContextData.PictureContextValid)
							{
								SCFound = TRUE;
								PARSER_Data_p->BDUByteStream.WordSwapFlag = FALSE;
							}
							break;

						/* note: slices and user data for Frames Fields and Slices are handled
						   when finding the end of the frame BDU.  Fields are ignored if
						   we are not expecting a field (i.e. we received frame data that was
						   the first of two fields. */

						case BDU_TYPE_SEQ_END:

							/* Save off the Decoding Order Frame ID.  We want this to stay the same.*/
							TempDecodeOrder = PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
							TempPresId0  = PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID;
							TempPresId1  = PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID;
							/* a Sequence End cannot be considered a start code for
							   a bdu because it is not the start of a bdu and therefore,
							   we cannot then start looking for the end of the BDU.
							   the sequence end code will cause all prior context data
							   to be cleared out, but will then be skipped over.  We
							   will then wait for more data.*/
							SCFound = FALSE;
							vc1par_InitParserContext(ParserHandle);
							PARSER_Data_p->PictureGenericData.DecodingOrderFrameID = TempDecodeOrder;
                            /* CL comment: shall we increment TempPresId0.IDExtension as we skip to a next entry point ? */
                            /*TempPresId0.IDExtension++;*/
                            PARSER_Data_p->StreamState.CurrentPresentationOrderPictureID = TempPresId0;
                            /* CL comment: shall we increment TempPresId1.IDExtension as we skip to a next entry point ? */
                            /*TempPresId1.IDExtension++;*/
                            PARSER_Data_p->StreamState.BackwardRefPresentationOrderPictureID = TempPresId1;

							break;
						default:
							/*if(PARSER_Data_p->PrivatePictureContextData.PictureContextValid)
									@@@ report error  received unexpected BDUs*/
							break;
					}

				}
				else /* We have already received a first field and we are expecting the second field.
					    NOTE:  if there were any slices, or slice-level, field-level, or frame-level user data
					    after the first field, they would already have been handled.*/
				{
					if(SCValue == BDU_TYPE_FIELD)
					{
						/* Ensure that the first field was good before processing the second field.*/
                        if((PARSER_Data_p->AllReferencesAvailableForFrame) && (PARSER_Data_p->PictureLocalData.IsValidPicture))
						{
							SCFound = TRUE;
							PARSER_Data_p->BDUByteStream.WordSwapFlag = FALSE;
						}
					}
					else
					{
						/* expected field but didn't get it!!!!!!!
						   Need to give a fake field here and then wait to send
						   out the BDU for this start code.*/
						STTBX_Print(("ERROR:  Expected Second Field BDU, but instead received a BDU of type %02x\n", SCValue));
						PARSER_Data_p->PictureSpecificData.PictureError.NoSecondFieldAfterFirstFieldError = TRUE;
					}/* else */

					/* even if a field wasn't found, we stop looking for the field BDU.
					   The field BDU should have been found directly after
					  the first field which came after a frame SC. */
					PARSER_Data_p->ExpectingFieldBDU = FALSE;
				}

                if(SCFound)
				{
/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
				TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Valid StartCode Found!  "));
#endif*/

#if defined(DVD_SECURED_CHIP)
                    /* Set the Parser Read Pointer in ES Bit Buffer on the start address of the start code */
                    PARSER_Data_p->StartCode.StartAddress_p = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
                    /* Set the Parser Read Pointer in ES Bit Buffer on the start address of the start code */
                    PARSER_Data_p->StartCode.StartAddressInBitBuffer_p = (U8 *)GetSCAddForBitBuffer(CPUNextSCEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
					/* Set the Parser Read Pointer in ES Bit Buffer on the start address of the start code */
					PARSER_Data_p->StartCode.StartAddress_p = (U8*)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */
					/* A valid current start code is available */
					PARSER_Data_p->StartCode.IsPending = TRUE;

					/* The parser must now look for the next start code to know if the BDU is fully in the bit buffer */
					PARSER_Data_p->StartCode.IsInBitBuffer = FALSE;
					PARSER_Data_p->StartCode.Value = SCValue;

					PARSER_Data_p->SCBuffer.CurrentSCEntry_p = STAVMEM_CPUToDevice(CPUNextSCEntry_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
				}

	        }
            else
            {
				vc1par_SavePTSEntry(ParserHandle, CPUNextSCEntry_p /*PARSER_Data_p->SCBuffer.NextSCEntry*/);
            }

#if defined(DVD_SECURED_CHIP)
            /*TrStartCode(("\r\nPARSER: Next SC (Offset)=0x%08x (%u) - Type: %d",*/
            TrStartCode(("\r\nSC Off=0x%08x (%u) - Type: %X",
                        (U32)PARSER_Data_p->StartCode.StartAddress_p - (U32)PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,
                        (U32)PARSER_Data_p->StartCode.StartAddress_p - (U32)PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,
                        PARSER_Data_p->StartCode.Value));
#endif /* defined(DVD_SECURED_CHIP) */

#ifdef OLD_SLICE_METHOD

			if(SCValue != BDU_TYPE_SLICE)
			{
				/* Make ScannedEntry_p point on the next entry in SC List */
				vc1par_PointOnNextSC(ParserHandle);
			}
#else /* NOT OLD_SLICE_METHOD */

			/* Make ScannedEntry_p point on the next entry in SC List */
			vc1par_PointOnNextSC(ParserHandle);

#endif /* end NOT OLD_SLICE_METHOD */
        } /* end while */
	}/* if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p)*/
}/* end vc1par_FindUsableStartCodeBDU() */

/*******************************************************************************/
/*! \brief Check if the BDU is fully in bit buffer: this is mandatory to process it.
 *
 *
 * \param ParserHandle
 * \return PARSER_Data_p->StartCode.IsInBitBuffer TRUE if fully in bit buffer
 */
/*******************************************************************************/
void vc1par_FindBDUEndAddress(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t                * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
	STFDMA_SCEntry_t                      * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
	U8                                      SCValue = 0;
	BOOL                                    SCFound = FALSE;
	/* U32 SliceElementSize; */
	U8                                    * SeqEndAddr_p;
#ifdef OLD_SLICE_METHOD
	U8                                    * SliceArrayStartAddr_p
#endif

#if defined(TRACE_UART) && defined(VALID_TOOLS)
	/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "vc1par_FindBDUEndAddress  ")); */
#endif
    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

	/* Saves the NextSCEntry before proceeding over the SC+PTS buffer */
	PARSER_Data_p->SCBuffer.SavedNextSCEntry_p = PARSER_Data_p->SCBuffer.NextSCEntry;

	if (PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p)
    {
		SCFound = FALSE;
		/* Initialize the slice number. This modification fix a bug when the Start Code list conten on frame and several slices. */
        PARSER_Data_p->PictureSpecificData.NumSlices = 0;

	    /* search for available SC in SC+PTS buffer : */
        while((PARSER_Data_p->SCBuffer.NextSCEntry != PARSER_Data_p->SCBuffer.SCList_Write_p) && (!(SCFound)))
	    {

			CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry,&VirtualMapping);

			if(!IsPTS(CPUNextSCEntry_p))
			{
                if (PARSER_Data_p->StartCode.Value == BDU_TYPE_DISCONTINUITY)
                {
                    /* In backward, we don't need to check discontinuity. Discontinuities are from one buffer to the other. */
#ifdef ST_speed
#ifdef STVID_TRICKMODE_BACKWARD
                    if (!PARSER_Data_p->Backward)
#endif
                    {
                        STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_FIND_DISCONTINUITY_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                        SCFound = TRUE; /* to exit of the loop */
                        /* If a discontinuity is found, then search for a random access point because we do not have all the reference pictures required to decode the
                         * pictures between the discontinuity and the first I after it. */
                        PARSER_Data_p->DiscontinuityDetected = TRUE;
                        PARSER_Data_p->GetPictureParams.GetRandomAccessPoint = TRUE;
                        PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;


        				#if defined(DVD_SECURED_CHIP)
            				/* Set the Parser Read Pointer in ES Copy Buffer */
            				PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
            				/* Set the Parser Read Pointer in bit buffer */
            				PARSER_Data_p->StartCode.BitBufferStopAddress_p = (U8 *)GetSCAddForBitBuffer(CPUNextSCEntry_p);
            			#else
                            /* Stop address of NAL to process is the start address of the following NAL */
                            PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p);
        				#endif /* DVD_SECURED_CHIP */
                    }
#endif /* ST_speed */
                }

/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
				TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "StartCode = %x  StartCodeAddress = %08x  ",GetSCVal(CPUNextSCEntry_p), GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p)));
#endif*/
				/* when the start code to process is not a slice start code, having any start code stored after in the
				 * SC+PTS buffer is enough to say that the start code to process is fully in the bit buffer
				 */
                if ((PARSER_Data_p->StartCode.Value == BDU_TYPE_FIELD) ||
                    (PARSER_Data_p->StartCode.Value == BDU_TYPE_FRAME))
				{
					SCValue = GetSCVal(CPUNextSCEntry_p);

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
					if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_MAIN ||PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE )
					{
						/* we are in Main or Simple Profile
						 in order to prevent false start code
						 the injection is supposed to add a signature, just after
						 the start code 'STMICR' , if the signature is not present, we must ignore
						 the start code, it is a part of the payload of Main or simple profile, but the signature
						 is found, we have to process it
						*/
#if defined(DVD_SECURED_CHIP)
						unsigned char *pPayload = STAVMEM_DeviceToCPU((void *)(GetSCAddForBitBuffer(CPUNextSCEntry_p),&VirtualMapping);
#else /* defined(DVD_SECURED_CHIP) */
						unsigned char *pPayload = STAVMEM_DeviceToCPU((void *)(CPUNextSCEntry_p->SC.Addr),&VirtualMapping);
#endif /* defined(DVD_SECURED_CHIP) */
						/*
						 Check the signature
						 The Payload point on the Start Code, +1 to point on the Signature
						*/

						if(vc1par_CheckSCSignature(pPayload+1,SCValue, ParserHandle))
						{
							/*
							 we have found a Start code with a correct signature , there is nothing to do
							 Because,  GetData will translate the point to the end of the signature and will point also on a Start code + payload
							*/
						}
						else
						{
							/*
							 the Start code don't have a right signature. it should be a data of the payload
							 for main and simple profile
							*/
							SCValue=BDU_TYPE_SKIP;

						}

					}
#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */

					switch(SCValue)
					{
#ifdef STVID_USE_VC1_MP_SC_SIGNATURE

						case BDU_TYPE_SKIP:
						{
							SCFound = FALSE;
							break;
						}
#endif /* STVID_USE_VC1_MP_SC_SIGNATURE */
						case BDU_TYPE_SEQ_END:
							PARSER_Data_p->SeqEndAfterThisBDU = TRUE;
#if defined(DVD_SECURED_CHIP)
							SeqEndAddr_p = STAVMEM_DeviceToCPU((void*)GetSCAddForESCopyBuffer(CPUNextSCEntry_p),&VirtualMapping);
#else /* defined(DVD_SECURED_CHIP) */
							SeqEndAddr_p = STAVMEM_DeviceToCPU((void*)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p),&VirtualMapping);
#endif /* defined(DVD_SECURED_CHIP) */

#ifdef STUB_INJECT
                            /* @@@ This is a HACK placed here until the injection stub changes.   The inject
							       stub adds 5 bytes to the end of the file, but the start code is only 4 bytes.
							       But don't insert the end of sequence start code if there is already
							       an end of sequence start code.  In this case, the inject stub would
							       have found the end of sequence start code, inserted it into the start
							       code list, then found the end of file and inserted another end of sequence
							       start code into the start code list.  This second start code should not
							       be found by the parser, but if the first start code is found, then
							       subtracting 4 from the end of the bitstream would overwrite the last byte of data!!
							       To find this case, we check if there is already a start code in the
							       bitstream before manually inserting one.*/
							if(!((*(SeqEndAddr_p-3) == 0x00) && (*(SeqEndAddr_p-2) == 0x00) &&
								 (*(SeqEndAddr_p-1) == 0x01) && (*(SeqEndAddr_p)   == 0x0A)))
							{
								SeqEndAddr_p -= 4;
								*SeqEndAddr_p = 0x00;
								SeqEndAddr_p++;
								*SeqEndAddr_p = 0x00;
								SeqEndAddr_p++;
								*SeqEndAddr_p = 0x01;
								SeqEndAddr_p++;
								*SeqEndAddr_p = BDU_TYPE_SEQ_END;
								CPUNextSCEntry_p->SC.Addr = ((U8*)CPUNextSCEntry_p->SC.Addr - 1);
							}
#endif /* STUB_INJECT */

							/* No Break -- treat the sequence end as the end of a BDU.*/
						case BDU_TYPE_FRAME:
						case BDU_TYPE_EPH:
						case BDU_TYPE_SH:
#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
						case BDU_TYPE_SH_MPSP:
#endif /* define STVID_USE_VC1_MP_SC_SIGNATURE */
						case BDU_TYPE_SEQ_STRUCT_ABC:
						case BDU_TYPE_FRAME_LAYER_DATA_STRUCT_NO_KEY:
						case BDU_TYPE_FRAME_LAYER_DATA_STRUCT_KEY:

							SCFound = TRUE;

							/* Stop address of BDU to process is the start address of the following BDU */
#if defined(DVD_SECURED_CHIP)
            				/* Set the Parser Read Pointer in ES Copy Buffer */
            				PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
            				/* Set the Parser Read Pointer in bit buffer */
            				PARSER_Data_p->StartCode.StopAddressInBitBuffer_p = (U8 *)GetSCAddForBitBuffer(CPUNextSCEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
							PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
							/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Pict End %08x  ",PARSER_Data_p->StartCode.StopAddress_p)); */
#endif
							PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;

							if(PARSER_Data_p->StartCode.SubBDUPending)
							{
								if(PARSER_Data_p->StartCode.SubBDUType != BDU_TYPE_SLICE)
								{
									/* @@@ send user data */
								}

								/*if(!PARSER_Data_p->StartCode.SliceFound)
									PARSER_Data_p->StartCode.StopAddress_p =
									PARSER_Data_p->StartCode.FirstSubBDUStartAddress;*/


								PARSER_Data_p->StartCode.SliceFound = FALSE;
								PARSER_Data_p->StartCode.SubBDUPending = FALSE;
							}
							break;

						case BDU_TYPE_SLICE:
/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
							TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Slice found (SC %x)", SCValue));
#endif*/

							PARSER_Data_p->StartCode.SliceFound = TRUE;
#ifdef OLD_SLICE_METHOD
							PARSER_Data_p->PictureSpecificData.NumSlices = GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
							/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Number of Slices = %d  ", PARSER_Data_p->PictureSpecificData.NumSlices)); */
#endif
							/* Make ScannedEntry_p point to the beginning of the slice array */
							vc1par_PointOnNextSC(ParserHandle);
							SliceArrayStartAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry,&VirtualMapping);

							/* copy the slice array to my local data structure */
							STOS_memcpy(PARSER_Data_p->PictureSpecificData.SliceArray, SliceArrayStartAddr_p,
								        (PARSER_Data_p->PictureSpecificData.NumSlices * sizeof(VC9_SliceParam_t)));

							vc1par_SkipSliceInfoInSCBuffer(sizeof(VC9_SliceParam_t), ParserHandle);
#else
							/* Move to the beginning of the 32 bit start code, rather than pointing at the start code byte  */
#if defined(DVD_SECURED_CHIP)
							PARSER_Data_p->PictureSpecificData.SliceArray
								[PARSER_Data_p->PictureSpecificData.NumSlices].SliceStartAddrCompressedBuffer_p =
								          (void*)vc1par_MoveBackInStream(((U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p)), 3, ParserHandle);
#else /* defined(DVD_SECURED_CHIP) */
							PARSER_Data_p->PictureSpecificData.SliceArray
								[PARSER_Data_p->PictureSpecificData.NumSlices].SliceStartAddrCompressedBuffer_p =
								          (void*)vc1par_MoveBackInStream(((U8 *)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p)), 3, ParserHandle);
#endif  /* defined(DVD_SECURED_CHIP) */

							PARSER_Data_p->PictureSpecificData.SliceArray
								[PARSER_Data_p->PictureSpecificData.NumSlices].SliceAddress = (U16) (CPUNextSCEntry_p->SC.ExtendedInfo.VC1.SliceAddress );   /* VC1 SliceAddress is a 9-bit value */

							/* This should look at the 24 bits that are above SC Value (specifically just 9 bits that are the slice address */
							/*PARSER_Data_p->PictureSpecificData.SliceArray
								[PARSER_Data_p->PictureSpecificData.NumSlices].SliceAddress = *((U32 *) (&CPUNextSCEntry_p->SC.SCValue)) >> 8;*/

							PARSER_Data_p->PictureSpecificData.NumSlices++;
#endif /* not OLD_SLICE_METHOD */
							break;

						case BDU_TYPE_FIELD:
							if(PARSER_Data_p->StartCode.Value != BDU_TYPE_FIELD)
							{
#if defined(TRACE_UART) && defined(VALID_TOOLS)
								/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Field BDU (SC %x) found  ", SCValue)); */
#endif
								SCFound = TRUE;

								/* Stop address of BDU to process is the start address of the following BDU */
#if defined(DVD_SECURED_CHIP)
            				/* Set the Parser Read Pointer in ES Copy Buffer */
            				PARSER_Data_p->StartCode.StopAddress_p = (U8*)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
            				/* Set the Parser Read Pointer in bit buffer */
            				PARSER_Data_p->StartCode.StopAddressInBitBuffer_p = (U8*)GetSCAddForBitBuffer(CPUNextSCEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
							PARSER_Data_p->StartCode.StopAddress_p = (U8*)GetSCAdd(CPUNextSCEntry_p, PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
								/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Pict End %08x  ",PARSER_Data_p->StartCode.StopAddress_p)); */
#endif
								PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;

								if(PARSER_Data_p->StartCode.SubBDUPending)
								{
									if(PARSER_Data_p->StartCode.SubBDUType != BDU_TYPE_SLICE)
									{
										/* @@@ send user data */
									}

									/*if(!PARSER_Data_p->StartCode.SliceFound)
										PARSER_Data_p->StartCode.StopAddress_p =
										PARSER_Data_p->StartCode.FirstSubBDUStartAddress; */


									PARSER_Data_p->StartCode.SliceFound = FALSE;
									PARSER_Data_p->StartCode.SubBDUPending = FALSE;
								}
							}/* end  if(PARSER_Data_p->StartCode.Value != BDU_TYPE_FIELD) */
							else /* Field SC */
							{
								STTBX_Print(("ERROR:  Parser Received Two Fields in a row\n"));
								PARSER_Data_p->PictureSpecificData.PictureError.NoSecondFieldAfterFirstFieldError = TRUE;
							}
							break;

						case BDU_TYPE_FRAME_UD:

/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
							TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Frame User Data (SC %x) found  ", SCValue));
#endif*/
							if(PARSER_Data_p->StartCode.Value != BDU_TYPE_FIELD)
							{
								if(PARSER_Data_p->StartCode.SubBDUPending)
								{
									/* @@@ send User data */
								}
								else
								{
#if defined(DVD_SECURED_CHIP)
									PARSER_Data_p->StartCode.FirstSubBDUStartAddress = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
									PARSER_Data_p->StartCode.FirstSubBDUStartAddress = (U8 *)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */
									PARSER_Data_p->StartCode.SubBDUPending = TRUE;
								}

								PARSER_Data_p->StartCode.SubBDUType = SCValue;

#if defined(DVD_SECURED_CHIP)
                                PARSER_Data_p->StartCode.SubBDUStartAddress = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
								PARSER_Data_p->StartCode.SubBDUStartAddress = (U8 *)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */

							}/* end  if(PARSER_Data_p->StartCode.Value != BDU_TYPE_FIELD) */
							else /* Field SC */
							{
								/* @@@ ERROR Frame user data should not come
								       in the middle of a Field BDU. */
							}
							break;

                        case BDU_TYPE_SLICE_UD:
						case BDU_TYPE_FIELD_UD:
/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
							TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "User Data SC %x found  ", SCValue));
#endif*/
							if(PARSER_Data_p->StartCode.SubBDUPending)
							{
								/* @@@ send User data */
							}
							else
							{
#if defined(DVD_SECURED_CHIP)
								PARSER_Data_p->StartCode.FirstSubBDUStartAddress = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
								PARSER_Data_p->StartCode.FirstSubBDUStartAddress = (U8 *)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */
								PARSER_Data_p->StartCode.SubBDUPending = TRUE;
							}

							PARSER_Data_p->StartCode.SubBDUType = SCValue;
#if defined(DVD_SECURED_CHIP)
                            PARSER_Data_p->StartCode.SubBDUStartAddress = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
							PARSER_Data_p->StartCode.SubBDUStartAddress = (U8 *)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */
							break;
#if 0 /* This was inserted for parsing of a slice user data array.
			for now, the slice user data does not come in an array, but one at a time*/
						case BDU_TYPE_SLICE_UD:

							/* Make ScannedEntry_p point to the beginning of the slice user data array */
							vc1par_PointOnNextSC(ParserHandle);
							SliceArrayStartAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry,&VirtualMapping);

							/* Should I send one event for all of the slices or
							   One event per slice?  */
							for(i = 0; i < CPUNextSCEntry_p->SC.NumSlices; i++)
							{
								/* @@@ Send Slice User data here */
							}
							/*SliceElementSize = sizeof(SliceUserData_t);  SliceUserData_t has not been defined yet.  this would be the
							  size of one slice user data element.*/


							/* @@@ Send Slice User data here */
						    break;
#endif

						default:
							break;

					}

				}
				else /* the SC first found was a Sequence Header, Entry
					    Point Header, Sequence or Entry Point User Data.
					    for these types, having any start code stored
						after in the SC+PTS buffer is enough to say that
						the start code to process is fully in the bit buffer*/
				{
					SCFound = TRUE; /* to exit of the loop */
					PARSER_Data_p->StartCode.IsInBitBuffer = TRUE;
					/* Stop address of BDU to process is the start address of the following BDU */
#if defined(DVD_SECURED_CHIP)
            		/* Set the Parser Read Pointer in ES Copy Buffer */
            		PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAddForESCopyBuffer(CPUNextSCEntry_p);
            		/* Set the Parser Read Pointer in bit buffer */
            		PARSER_Data_p->StartCode.StopAddressInBitBuffer_p = (U8 *)GetSCAddForBitBuffer(CPUNextSCEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
					PARSER_Data_p->StartCode.StopAddress_p = (U8 *)GetSCAdd(CPUNextSCEntry_p, PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
					/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Sequence or Entry Point header End Found,  StopAddress = %08x  ",PARSER_Data_p->StartCode.StopAddress_p)); */
#endif
					if(SCValue == BDU_TYPE_SEQ_END)
					{
						PARSER_Data_p->SeqEndAfterThisBDU = TRUE;
					}

				}

	        }/* end if(!IsPTS(CPUNextSCEntry_p)) */
			else
			{
#if defined(TRACE_UART) && defined(VALID_TOOLS)
				/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "PTS found  ")); */
#endif
			}
#ifdef OLD_SLICE_METHOD

            if((SCValue != BDU_TYPE_SLICE) && (PARSER_Data_p->StartCode.SubBDUPending))
			{
				/* Make ScannedEntry_p point on the next entry in SC List */
				vc1par_PointOnNextSC(ParserHandle);
			}
#else

				/* Make ScannedEntry_p point on the next entry in SC List */
				vc1par_PointOnNextSC(ParserHandle);
#endif /* OLD_SLICE_METHOD --  ELSE */

        } /* end while */
	}
	else
	{
/*#if defined(TRACE_UART) && defined(VALID_TOOLS)
		TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Reached End of SC buffer, Read ptr = %08x, Write ptr = %08x  ", PARSER_Data_p->SCBuffer.NextSCEntry, PARSER_Data_p->SCBuffer.SCList_Write_p));
#endif*/
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
void vc1par_IsThereStartCodeToProcess(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t      * PARSER_Data_p;
#ifdef STVID_TRICKMODE_BACKWARD
    STFDMA_SCEntry_t            * CheckSC;
    PARSER_Properties_t         * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
#endif /*STVID_TRICKMODE_BACKWARD*/
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Test if a new start-code is available */
    /*---------------------------------------*/
    STOS_SemaphoreWait(PARSER_Data_p->InjectSharedAccess);
    if (!(PARSER_Data_p->StartCode.IsPending)) /* Look for a start code to process */
	{
		vc1par_FindUsableStartCodeBDU(ParserHandle);
#ifdef STVID_TRICKMODE_BACKWARD
        if ((PARSER_Data_p->Backward) && (!PARSER_Data_p->IsBitBufferFullyParsed))
        {
            CheckSC = PARSER_Data_p->SCBuffer.NextSCEntry;
            /*STTBX_Report((STTBX_REPORT_LEVEL_WARNING, "SC %d %d  ", PARSER_Data_p->SCBuffer.SCList_Write_p, PARSER_Data_p->SCBuffer.NextSCEntry));*/
            /*STTBX_Print(("\n SC %d %d %d", PARSER_Data_p->SCBuffer.SCList_Write_p, CheckSC, PARSER_Data_p->SCBuffer.SCList_Loop_p));*/
            if (CheckSC == PARSER_Data_p->SCBuffer.SCList_Write_p)
            {
                PARSER_Data_p->IsBitBufferFullyParsed  = TRUE;
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                STTBX_Print(("\n BB fully parsed \r\n"));

                /* Bit Buffer is fully parsed: stop parsing ... */
                PARSER_Data_p->StopParsing = TRUE;
            }
        }
#endif
		/* PARSER_Data_p->StartCode.IsPending is modified in the function above */
	}

    if (PARSER_Data_p->StartCode.IsPending) /* Look if the BDU is fully in bit buffer, now */
	{
		vc1par_FindBDUEndAddress(ParserHandle);
	}

	/* inform inject module about the SC-list read progress */
    VIDINJ_TransferSetSCListRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, PARSER_Data_p->SCBuffer.NextSCEntry);

    STOS_SemaphoreSignal(PARSER_Data_p->InjectSharedAccess);
}

/******************************************************************************/
/*! \brief Initialize the BDUByteStream data structure on current BDU
 *
 * \param BDUStartAddressInBB_p the start address in memory for this entry
 * \param BDUStopAddressInBB_p the start address in memory for this entry
 * \param pp the parser variables (updated)
 * \return 0
 */
/******************************************************************************/
void vc1par_InitBDUByteStream(U8 * BDUStartAddress, U8 * NextBDUStartAddress, const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if (NextBDUStartAddress > BDUStartAddress)
	{
        PARSER_Data_p->BDUByteStream.Len = NextBDUStartAddress - BDUStartAddress;
    }
	else
	{
#if defined DVD_SECURED_CHIP
		/* Bit Buffer wrap */
        PARSER_Data_p->BDUByteStream.Len = PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p
                                           - BDUStartAddress
                                           + NextBDUStartAddress
                                           - PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p
                                           + 1;
#else /* defined DVD_SECURED_CHIP */
		/* Bit Buffer wrap */
        PARSER_Data_p->BDUByteStream.Len = PARSER_Data_p->BitBuffer.ES_Stop_p
                                           - BDUStartAddress
                                           + NextBDUStartAddress
                                           - PARSER_Data_p->BitBuffer.ES_Start_p
                                           + 1;
#endif /* defined DVD_SECURED_CHIP */
	}
	PARSER_Data_p->BDUByteStream.ByteSwapFlag = FALSE;

	PARSER_Data_p->BDUByteStream.BitOffset = 7; /* ready to read the MSB of the first byte */
    if (!(PARSER_Data_p->BDUByteStream.WordSwapFlag))
	{
	    PARSER_Data_p->BDUByteStream.ByteOffset = 0; /* ready to read first byte */
	}
	else
	{
		PARSER_Data_p->BDUByteStream.ByteOffset = 3; /* ready to read first byte */
	}
	PARSER_Data_p->BDUByteStream.EndOfStreamFlag = FALSE;
	PARSER_Data_p->BDUByteStream.LastByte = 0xFF; /* do not initialize to 0 to avoid wrong detection on first bytes */
	PARSER_Data_p->BDUByteStream.LastButOneByte = 0xFF; /* do not initialize to 0 to avoid wrong detection on first bytes */
	PARSER_Data_p->BDUByteStream.ByteCounter = 0; /* beginning of stream */

	/* Translate "Device" address to "CPU" address */
	STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    PARSER_Data_p->BDUByteStream.StartByte_p = STAVMEM_DeviceToCPU(BDUStartAddress, &VirtualMapping);

	/* This has been moved out of this function because, for reading a file structure with no start code,
	   the pointer points to the beginning of the structure that needs to be read, not at a start code.*/
    /* The stream is initialized on the byte following the start code value (returned by the FDMA) */
    /* vc1par_MoveToNextByteFromBDU(ParserHandle); */
}


/*******************************************************************************/
/*! \brief Process the start code
 *
 * At that stage, the full BDU is in the bit buffer
 *
 * \param StartCode
 * \param BDUStartAddress in bit buffer
 * \param BDUStopAddress in bit buffer
 * \param ParserHandle
 */
/*******************************************************************************/
void vc1par_ProcessBDU(U8 StartCode,
					   U8 * BDUStartAddress,
					   U8 * NextBDUStartAddress,
#if defined(DVD_SECURED_CHIP)
                       U8 * BDUStartAddressInBitBuffer,
                       U8 * NextBDUStartAddressInBitBuffer,
#endif /* defined(DVD_SECURED_CHIP) */
					   const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	U32                      Profile;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
	TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU,
                                 "vc1par_ProcessBDU, SC = %x, start addr %08x, end addr %08x  ",
                                 StartCode, BDUStartAddress, NextBDUStartAddress));
#endif

	/* Initialize the BDUByteStream_t data structure to parse the SC entry in bit buffer */
    vc1par_InitBDUByteStream(BDUStartAddress, NextBDUStartAddress, ParserHandle);

	/* The pointer to the file structure given by the FDMA
	   specifies the first byte in the in the file structure.  There
	   are no start codes, so no need to advance the file pointer. */
	if(StartCode < BDU_TYPE_SEQ_STRUCT_ABC)
	{
		/* The stream is initialized on the byte following the start code value (returned by the FDMA) */
		vc1par_MoveToNextByteFromBDU(ParserHandle);
	}

	switch(StartCode)
	{
	case BDU_TYPE_FRAME:
            /*  SPMP Profile, start code added by application injection module at the beginning of each frame */
			if(PARSER_Data_p->PrivateGlobalContextData.Profile != VC9_PROFILE_ADVANCE)
			{
#if defined DVD_SECURED_CHIP
                vc1par_ParseSPMPFrame(BDUStartAddressInBitBuffer,
                                      NextBDUStartAddressInBitBuffer,
                                      ParserHandle);
#else /* defined(DVD_SECURED_CHIP) */
                vc1par_ParseSPMPFrame(BDUStartAddress,
                                      NextBDUStartAddress,
                                      ParserHandle);
#endif /* defined(DVD_SECURED_CHIP) */
			}
			else
			{
                /* Parse Advanced Profile Frame*/
#if defined(DVD_SECURED_CHIP)
                vc1par_ParseAPPicture(BDUStartAddressInBitBuffer,
                                      NextBDUStartAddressInBitBuffer,
                                      ParserHandle);
#else /* defined(DVD_SECURED_CHIP) */
                vc1par_ParseAPPicture(BDUStartAddress,
                                      NextBDUStartAddress,
                                      ParserHandle);
#endif /* defined(DVD_SECURED_CHIP) */
			}
			break;

		case BDU_TYPE_FRAME_LAYER_DATA_STRUCT_KEY:
		case BDU_TYPE_FRAME_LAYER_DATA_STRUCT_NO_KEY:
			/* the BDU is a Frame */
			if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE)
			{
				/* Parse Advanced Profile Frame */
				vc1par_ParseAPFrameLayerDataStruct(ParserHandle);
			}
			else
			{
				/* Parse Simple/Main Profile Frame */
#if defined(DVD_SECURED_CHIP)
				vc1par_ParseSPMPFrame(BDUStartAddressInBitBuffer,
									  NextBDUStartAddressInBitBuffer,
									  ParserHandle);
#else /* defined(DVD_SECURED_CHIP) */
				vc1par_ParseSPMPFrame(BDUStartAddress,
									  NextBDUStartAddress,
									  ParserHandle);
#endif /* defined(DVD_SECURED_CHIP) */
			}

			break;

		case BDU_TYPE_FIELD:
			/* the BDU is a Field */
#if defined(DVD_SECURED_CHIP)
			vc1par_ParseField(BDUStartAddressInBitBuffer,
							  NextBDUStartAddressInBitBuffer,
							  ParserHandle);
#else /* defined(DVD_SECURED_CHIP) */
			vc1par_ParseField(BDUStartAddress,
							  NextBDUStartAddress,
							  ParserHandle);
#endif /* defined(DVD_SECURED_CHIP) */
			break;

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
        /* The advance profile and main/simple profile have a assigned specific start code in order */
        /* to know the VC1 profile */
        /* If the start code is equal to 0x1F, the VC1 stream is a Main or Simple profile type   */
    	case BDU_TYPE_SH_MPSP:
	        {
	           int i;
	           STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
               STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

	           /* Check the presence of the signature */
	           if(vc1par_CheckSCSignature((unsigned char *)STAVMEM_DeviceToCPU((void *)BDUStartAddress,&VirtualMapping)+1,BDU_TYPE_SH_MPSP, ParserHandle))
	           {
    	           /* Move forward of the length VC1_MP_SC_SIGNATURE_SIZE in the BDU */
    	           for (i=0; i < VC1_MP_SC_SIGNATURE_SIZE; i++)
    	               vc1par_MoveToNextByteFromBDU(ParserHandle);

    	           vc1par_GetUnsigned(2, ParserHandle);
                   vc1par_ParseSpMpPesSeqHdr(ParserHandle);
               } else
               {
                   vc1par_GetUnsigned(2, ParserHandle);
                   vc1par_ParseSpMpPesSeqHdr(ParserHandle);
               }
			}
			break;
#endif /* define STVID_USE_VC1_MP_SC_SIGNATURE */
        /* If the start code is equal to 0x0F, the VC1 stream is a advanced profile type */
		case BDU_TYPE_SH:
			/*  First determine the profile */
            Profile = vc1par_GetUnsigned(2, ParserHandle);

            if(Profile == VC1_ADVANCED_PROFILE)
			{
				/* the BDU is a SH */
				vc1par_ParseSequenceHeader(ParserHandle);
            }
        	break;





        case BDU_TYPE_SEQ_STRUCT_ABC:
            vc1par_ParseSeqStructABC(ParserHandle);
            break;

		case BDU_TYPE_EPH:
			/* the BDU is a EPH */
			if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_ADVANCE)	/* BDU_TYPE_EPH used  as Random Access for SPMP  profile */
				vc1par_ParseEntryPointHeader(ParserHandle);
			break;

#if !defined STVID_USE_VC1_MP_SC_SIGNATURE
		case BDU_TYPE_SEQ_UD:
			/* the BDU is a sequence user data */
            /*  @@@ send user data             */
			break;
#endif /* !defined STVID_USE_VC1_MP_SC_SIGNATURE */

        case BDU_TYPE_EP_UD:
            /* the BDU is an Entry Point user data */
			/*  @@@ send user data                 */
            break;

		default:
			/* Do not process the other BDUs */
            break;
	}
}/* end vc1par_ProcessBDU()*/

/*******************************************************************************/
/*! \brief Fill the Parsing Job Result Data structure
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void vc1par_PictureInfoCompile(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	BOOL PictureHasError;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
	   (PARSER_Data_p->PictureSpecificData.SecondFieldFlag == 0) &&
	   (PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_B))
	{
		PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame
			[PARSER_Data_p->StreamState.NextReferencePictureIndex] = FALSE;

    }/* end if(PARSER_Data_p->PictureGenericData.IsReference) */


	/* Global Decoding Context structure */
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->
		PictureDecodingContext_p->GlobalDecodingContext_p->
		GlobalDecodingContextGenericData = PARSER_Data_p->GlobalDecodingContextGenericData;
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->
		PictureDecodingContext_p->GlobalDecodingContext_p->
		SizeOfGlobalDecodingContextSpecificDataInByte = sizeof(VC1_GlobalDecodingContextSpecificData_t);
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->
		PictureDecodingContext_p->GlobalDecodingContext_p->
		GlobalDecodingContextSpecificData_p = &(PARSER_Data_p->GlobalDecodingContextSpecificData);

	/* Picture Decoding Context structure */
	PARSER_Data_p->ParserJobResults.
		ParsedPictureInformation_p->PictureDecodingContext_p->
		SizeOfPictureDecodingContextSpecificDataInByte = sizeof(VC1_PictureDecodingContextSpecificData_t);
	PARSER_Data_p->ParserJobResults.
		ParsedPictureInformation_p->PictureDecodingContext_p->
		PictureDecodingContextSpecificData_p = &(PARSER_Data_p->PictureDecodingContextSpecificData);

	/* Parsed Picture Information structure */
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData = PARSER_Data_p->PictureGenericData;
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->SizeOfPictureSpecificDataInByte = sizeof(VC1_PictureSpecificData_t);
	PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureSpecificData_p = &(PARSER_Data_p->PictureSpecificData);

	/* Parsing Job Status */

	/* Check whether picture has errors */
	/* Checks Sequence Header */
    if (PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ProfileOutOfRange ||
    	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.LevelOutOfRange ||
    	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ColorDiffFormatOutOfRange ||
    	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedWidthOutOfRange ||
    	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MaxCodedHeightOutOfRange ||
    	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ReservedBitOutOfRange ||
        PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.SampleAspectRatioOutOfRange ||
        PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.FrameRateNROutOfRange ||
    	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.FrameRateDROutOfRange ||
    	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.ColorPrimariesOutOfRange ||
    	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.TransferCharOutOfRange ||
    	PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.MatrixCoefOutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.NoC5inRCVFormat ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.No00000004inRCVFormat ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.FrameRateOutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES1OutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES2OutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES3OutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES4OutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES5OutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES6OutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RES7OutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.LoopFilterFlagSPOutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.SyncMarkerFlagSPOutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.RangeredFlagSPOutOfRange ||
		PARSER_Data_p->GlobalDecodingContextSpecificData.SeqHdrError.No0000000CinRCVFormat)
	{
		PictureHasError = TRUE;
	}

	/* Check EntryPointHeader */
	if (PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.BrokenLinkOutOfRange ||
		PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedWidthOutOfRange ||
		PARSER_Data_p->PictureDecodingContextSpecificData.EntryPointHdrError.CodedHeightOutOfRange)
	{
		PictureHasError = TRUE;
	}

	/* Check picture header*/
	if (PARSER_Data_p->PictureSpecificData.PictureError.MissingReferenceInformation ||
    	PARSER_Data_p->PictureSpecificData.PictureError.SecondFieldWithoutFirstFieldError  ||
    	PARSER_Data_p->PictureSpecificData.PictureError.NoSecondFieldAfterFirstFieldError  ||
    	PARSER_Data_p->PictureSpecificData.PictureError.RefDistOutOfRange  ||
    	PARSER_Data_p->PictureSpecificData.PictureError.BFractionOutOfRange  ||
    	PARSER_Data_p->PictureSpecificData.PictureError.RESOutOfRange)
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
}/* vc1par_PictureInfoCompile */


/*******************************************************************************/
/*! \brief Check whether an event "job completed" is to be sent to the controller
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void vc1par_CheckEventToSend(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
	PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
	U8                          RefFrameIndex;
	BOOL                        PictureSentFlag = FALSE;    /* set to true  if the picture gets sent back to the producer*/
#ifdef STVID_VALID
	U32 StartFrame = 0;
	U32 EndFrame = 0;
#endif
#ifdef STVID_TRICKMODE_BACKWARD
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */
#endif

	PARSER_Data_p = (VC1ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

	/* An event "job completed" is raised whenever a new picture has been parsed.
	 * (and in case a new picture is starting, the parser shall not parse anything
	 *  not to modify the parameters from the previously parsed picture)
	 *
	 */

	/* Check whether the current start code starts a picture.
	   Simple or Main Profile Frame Layer Data structures begin a picture;
	   However, Advanced Profile Frame Layer Data structures stand by themselves
	   and the picture still comes after the Frame or Field start code. */
	if ((PARSER_Data_p->StartCode.Value == BDU_TYPE_FRAME) ||
		(PARSER_Data_p->StartCode.Value == BDU_TYPE_FIELD) ||
		(((PARSER_Data_p->StartCode.Value == BDU_TYPE_FRAME_LAYER_DATA_STRUCT_NO_KEY) ||
		  (PARSER_Data_p->StartCode.Value == BDU_TYPE_FRAME_LAYER_DATA_STRUCT_KEY)) &&
		 (PARSER_Data_p->PrivateGlobalContextData.Profile != VC9_PROFILE_ADVANCE)))
	{
		if ((PARSER_Data_p->ParserState == PARSER_STATE_PARSING) &&
            (PARSER_Data_p->PictureLocalData.IsValidPicture))
		{
			vc1par_PictureInfoCompile(ParserHandle);
#ifdef STVID_TRICKMODE_BACKWARD
            if (PARSER_Data_p->Backward)
            {  /* To get bit rate information in backward */
                CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
                /* to test if next Sc exist: end of buffer */
#if !defined(DVD_SECURED_CHIP)
                if (((U32)((U32)(GetSCAdd(CPUNextSCEntry_p, PARSER_Data_p)) - (U32)(PARSER_Data_p->LastSCAdd_p))) > 0)
                {
                    PARSER_Data_p->OutputCounter += (U32)((U32)(GetSCAdd(CPUNextSCEntry_p, PARSER_Data_p)) - (U32)(PARSER_Data_p->LastSCAdd_p));
                    PARSER_Data_p->PictureGenericData.DiffPSC = PARSER_Data_p->OutputCounter;
                }
#endif /* DVD_SECURED_CHIP */
            }
#endif /* STVID_TRICKMODE_BACKWARD */

#ifdef STVID_VALID
            if(STTST_EvaluateInteger("VALID_PARSER_STARTFRAME",(S32*)&StartFrame,10))
			{
#if defined(TRACE_UART) && defined(VALID_TOOLS)
				/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "START FRAME not available  ")); */
#endif
				StartFrame = 0;
			}

            if(STTST_EvaluateInteger("VALID_PARSER_ENDFRAME",(S32*)&EndFrame,10))
			{
#if defined(TRACE_UART) && defined(VALID_TOOLS)
				/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "END FRAME not available  ")); */
#endif
				EndFrame = 0;
			}

			if(EndFrame == 0)
			{
				EndFrame = 0xFFFFFFFF;
			}

			/* Only notify of a frame which has all of its reference frames available.
			   and if it is in the range of frames that need to be sent. */
            if( ((PARSER_Data_p->SCBuffer.InjectFileFrameNum >= StartFrame) && (PARSER_Data_p->SCBuffer.InjectFileFrameNum <= EndFrame)) &&
                (PARSER_Data_p->AllReferencesAvailableForFrame) &&
                /* added for the workaround which allow to start on first I picture when requesting a random access point even without EPH */
                ((!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)) || (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) )
            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
				/*TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Parser SENDING frame %d (inject file num), %d (DecodingOrderFrameID)  ",
									PARSER_Data_p->SCBuffer.InjectFileFrameNum,
					                PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));*/
#endif

#ifdef STVID_TRICKMODE_BACKWARD
                if ((!PARSER_Data_p->Backward) ||
                    ((PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I) && (PARSER_Data_p->Backward)
                    && (!PARSER_Data_p->ParserJobResults.BackwardBufferFullyParsed)))
#endif
                {
#ifdef STVID_TRICKMODE_BACKWARD
                    if (((PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I) &&
                         (PARSER_Data_p->Backward) &&
                         (!PARSER_Data_p->IsBitBufferFullyParsed))
                        ||
                        ((!PARSER_Data_p->Backward) &&
                         (((PARSER_Data_p->SkipMode == STVID_DECODED_PICTURES_I) && (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) ||
                         ((PARSER_Data_p->SkipMode != STVID_DECODED_PICTURES_I) ))))
#endif
                    {
                    /* Make sure that the state is changed before get_picture checks the Parser State*/
                    STOS_SemaphoreWait(PARSER_Data_p->ControllerSharedAccess);
                    if (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
                    {
                        PARSER_Data_p->DiscontinuityDetected = FALSE;
                    }
                    STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID],
                                 STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->ParserJobResults));
                    PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;

                    STOS_SemaphoreSignal(PARSER_Data_p->ControllerSharedAccess);
                    if(PARSER_Data_p->ParserJobResults.ParsingJobStatus.HasErrors)
                    {
                        /* Reset the flag indicating there were frames skipped because of missing reference info. */
                        PARSER_Data_p->PictureSpecificData.PictureError.MissingReferenceInformation = FALSE;
                        PARSER_Data_p->PictureSpecificData.PictureError.SecondFieldWithoutFirstFieldError  = FALSE;
                        PARSER_Data_p->PictureSpecificData.PictureError.NoSecondFieldAfterFirstFieldError  = FALSE;
                        PARSER_Data_p->PictureSpecificData.PictureError.RefDistOutOfRange  = FALSE;
                        PARSER_Data_p->PictureSpecificData.PictureError.BFractionOutOfRange = FALSE;
                        PARSER_Data_p->PictureSpecificData.PictureError.RESOutOfRange = FALSE;
                    }

                    PictureSentFlag = TRUE;
                    /* keep track of the end address of the last frame sent so that if we
                       reach the end of a file, we will know when the decoder is done processing
                       the bit buffer */
                    PARSER_Data_p->BitBuffer.ES_LastFrameEndAddrGivenToDecoder_p = PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress;
                }
                else
                {
                    /* The picture is skipped */
                    /* WARNING, at this stage only picture's start & stop addresses in the bit buffer are valid data, all remaingin of the picture's data might be unrelevant because picture parsing is not complete */
                    STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData));
                }
			}
			else
			{
                /* WARNING, at this stage only picture's start & stop addresses in the bit buffer are valid data, all remaingin of the picture's data might be unrelevant because picture parsing is not complete */
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData));
			}
#if defined(TRACE_UART) && defined(VALID_TOOLS)
				TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Parser SKIPPING frame %d (inject file num), %d (DecodingOrderFrameID)  ",
														  PARSER_Data_p->SCBuffer.InjectFileFrameNum,
					                                      PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));
#endif

				/* Indicate that we no longer need to get a random access point.  This
				   is in the case where we are skipping the first frame which happens to
				   be a random access point.  If we don't set this flag to FALSE, the
				   parser will automatically skip over everything until another random
				   access point is found */
				PARSER_Data_p->GetPictureParams.GetRandomAccessPoint = FALSE;

				/* Indicate to the producer that some frames were skipped because of missing reference information */
				PARSER_Data_p->PictureSpecificData.PictureError.MissingReferenceInformation = TRUE;

				/* Wake up task to look for next start codes */
				STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
			}

			/* Increment InjectFileFrameNum for each frame or the
			   the first field of a field pair.  This code assumes that the
			   SecondFieldFlag will be set to zero for frames. */
			if(PARSER_Data_p->PictureSpecificData.SecondFieldFlag == 0)
			{
				PARSER_Data_p->SCBuffer.InjectFileFrameNum++;
			}


#else   /* STVID_VALID */


            /* Only notify of a frame which has all of its reference frames available.
			   @@@ Is this the way it should always be ???*/
            if(PARSER_Data_p->AllReferencesAvailableForFrame)
			{

#if defined(TRACE_UART) && defined(VALID_TOOLS)
				/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Parser SENDING frame %d  ",PARSER_Data_p->PictureGenericData.DecodingOrderFrameID)); */
#endif

#ifdef STVID_TRICKMODE_BACKWARD
                if (((PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I) &&
                     (PARSER_Data_p->Backward) &&
                     (!PARSER_Data_p->IsBitBufferFullyParsed))
                    ||
                    ((!PARSER_Data_p->Backward) &&
                    ((!(PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)) || (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I))))
#endif
                {
                    STOS_SemaphoreWait(PARSER_Data_p->ControllerSharedAccess);
                    if (PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
                    {
                        PARSER_Data_p->DiscontinuityDetected = FALSE;
                    }
                    STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID],
                                    STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->ParserJobResults));

				PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;
                STOS_SemaphoreSignal(PARSER_Data_p->ControllerSharedAccess);
                /* Reset the flag indicating there were frames skipped because of missing reference info. */
                PARSER_Data_p->PictureSpecificData.PictureError.MissingReferenceInformation = FALSE;

    				PictureSentFlag = TRUE;
                }
			}
			else
			{
                /* WARNING, at this stage only picture's start & stop addresses in the bit buffer are valid data, all remaingin of the picture's data might be unrelevant because picture parsing is not complete */
                STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PARSER_Data_p->PictureGenericData));

#if defined(TRACE_UART) && defined(VALID_TOOLS)
				TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "Parser SKIPPING frame %d  ",PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));
#endif
				/* Indicate to the producer that some frames were skipped because of missing reference information */
				PARSER_Data_p->PictureSpecificData.PictureError.MissingReferenceInformation = TRUE;

				/* Wake up task to look for next start codes */
				STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
			}
#endif /* not STVID_VALID */


            if(PictureSentFlag)
			{

		               /* Set the number of slices to zero in case there were slices in the previous picture, but
		                  there are no slices in the current picture.  If this were not set to zero, we would
		                  be telling the firmware that there are the same number of slices for this picture.*/
		                PARSER_Data_p->PictureSpecificData.NumSlices = 0;

				/* Only update the Full Reference frame list for the first field of a
				   reference field-pair or frame that is not a B or BI field-pair or
				   frame.  It is possible that the first B or BI field in a field-pair
				   can be used as reference for the second field in the field-pair. In
				   this case the first field will be placed in the Full Reference Frame List
				   as needed when the second field is being parsed.  And it will always be
				   placed in the third element of the array so that it does not interfere
				   with the processing done here.  This processing only works with the
				   first and second elements of the array.
				   */
                if((PARSER_Data_p->PictureGenericData.IsReference) &&
				   (PARSER_Data_p->PictureSpecificData.SecondFieldFlag == 0) &&
				   (PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_B) &&
				   (PARSER_Data_p->PictureSpecificData.PictureType != VC9_PICTURE_TYPE_BI))
				{
					RefFrameIndex = PARSER_Data_p->StreamState.NextReferencePictureIndex;
					PARSER_Data_p->PictureGenericData.FullReferenceFrameList[RefFrameIndex] =
						PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;

					PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[RefFrameIndex] = TRUE;

					if(PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
					{
						PARSER_Data_p->StreamState.FieldPairFlag[RefFrameIndex] = FALSE;
					}
					else
					{
						PARSER_Data_p->StreamState.FieldPairFlag[RefFrameIndex] = TRUE;
					}

					/* Update the index for the next reference frame that comes in.
						It will either be slot 0 or 1. */
					PARSER_Data_p->StreamState.NextReferencePictureIndex = (RefFrameIndex ^ 1);

					/* Only update Reference lists P0, B0, B1 here if it is not an
					   Advanced Profile Stream, Otherwise, it will be updated elsewhere */
					if(PARSER_Data_p->PrivateGlobalContextData.Profile != VC9_PROFILE_ADVANCE)
					{
						/* The very first reference picture is placed in all of the reference lists.
						When the second reference picture comes in, the first
						reference picture gets moved to lists B0 and the second reference
						frame gets placed in P0 and B1 because this frame will be the backward
						reference frame for the next B picture and the forward reference frame
						for the next P picture.*/
						if(PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 == 0)
						{
							PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 1;
							PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 1;
						}
						else
						{
							PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 1;
							PARSER_Data_p->PictureGenericData.ReferenceListB0[0] =
                            	PARSER_Data_p->PictureGenericData.ReferenceListP0[0];
						}

						PARSER_Data_p->PictureGenericData.ReferenceListP0[0] =
							PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;
						PARSER_Data_p->PictureGenericData.ReferenceListB1[0] =
							PARSER_Data_p->PictureGenericData.DecodingOrderFrameID;

                    }/* end if() */

                }/* end if(PARSER_Data_p->PictureGenericData.IsReference) */
            }/* end if(PictureSentFlag) */

			/* Increment the Decoding Order Frame ID for each frame or the
			   the second field of a field pair. */
			if((PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) ||
			   (PARSER_Data_p->PictureSpecificData.SecondFieldFlag == 1))
			{
				PARSER_Data_p->PictureGenericData.DecodingOrderFrameID++;
			}

        }/* end  if ((PARSER_Data_p->ParserState == PARSER_STATE_PARSING) ...*/


    }/* end if ((PARSER_Data_p->StartCode.Value == BDU_TYPE_FRAME) ...*/
    else
    {
        /* Wake up task to look for next start codes */
        STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
    }
}/* end vc1par_CheckEventToSend() */

/*******************************************************************************/
/*! \brief Get the Start Code commands. Flush the command queue
 *
 * \param ParserHandle
 */
/*******************************************************************************/
void vc1par_GetStartCodeCommand(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t  * PARSER_Data_p;
#ifdef STVID_TRICKMODE_BACKWARD
	PARSER_Properties_t     * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
#endif
	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* The start code to process is in the StartCodeCommand buffer */
    if ((PARSER_Data_p->StartCode.IsPending) && (PARSER_Data_p->StartCode.IsInBitBuffer))
	{
        #if defined(DVD_SECURED_CHIP)
        /* Parser works with Start/Stop addresses of ES copy buffer */
        /* StartAddress_p and StopAddress_p are initialized with ES Copy buffer addresses for secured chips */
        #endif  /* defined(DVD_SECURED_CHIP) */

        /* The BDU can now be processed */
	 	vc1par_ProcessBDU(PARSER_Data_p->StartCode.Value,
						  PARSER_Data_p->StartCode.StartAddress_p,
						  PARSER_Data_p->StartCode.StopAddress_p,
#if defined(DVD_SECURED_CHIP)
                          PARSER_Data_p->StartCode.StartAddressInBitBuffer_p,
                          PARSER_Data_p->StartCode.StopAddressInBitBuffer_p,
#endif /* defined(DVD_SECURED_CHIP) */
						  ParserHandle);
		vc1par_CheckEventToSend(ParserHandle);
		/* The start code has been analyzed: it is not pending anymore */
		PARSER_Data_p->StartCode.IsPending = FALSE;
	}
#ifdef STVID_TRICKMODE_BACKWARD
    else if ((PARSER_Data_p->Backward) && (PARSER_Data_p->StartCode.IsPending) && (!PARSER_Data_p->StartCode.IsInBitBuffer) && (!PARSER_Data_p->IsBitBufferFullyParsed))
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
void vc1par_GetControllerCommand(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
void vc1par_ParserTaskFunc(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STOS_TaskEnter(NULL);

    /* Big loop executed until ToBeDeleted is set --------------------------- */
	do
	{
		while(!PARSER_Data_p->ParserTask.ToBeDeleted && (PARSER_Data_p->ParserState != PARSER_STATE_PARSING))
		{
			/* Waiting for a command */
			STOS_SemaphoreWait(PARSER_Data_p->ParserOrder);
			vc1par_GetControllerCommand(ParserHandle);
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
                        vc1par_IsThereStartCodeToProcess(ParserHandle);
                        /* Then, get Start code commands */
                        vc1par_GetStartCodeCommand(ParserHandle);
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
static ST_ErrorCode_t vc1par_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t * PARSER_Data_p;
    #if defined(DVD_SECURED_CHIP)
        STFDMA_SCEntry_t * CurrentSCEntry_p;
    #endif /* DVD_SECURED_CHIP */

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Stores the decoder read pointer locally for further computation of the bit buffer level */
	PARSER_Data_p->BitBuffer.ES_DecoderRead_p = InformReadAddressParams_p->DecoderReadAddress_p;

#if defined(TRACE_UART) && defined(VALID_TOOLS)
	/* TraceBufferColorConditional((VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_EXTRACTBDU, "vc1par_InformReadAddress %08x  ",PARSER_Data_p->BitBuffer.ES_DecoderRead_p)); */
#endif

    /* Ensure no FDMA transfer is done while updating ESRead */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    #if defined(DVD_SECURED_CHIP)

        CurrentSCEntry_p = PARSER_Data_p->SCBuffer.CurrentSCEntry_p;

        assert(CurrentSCEntry_p != 0);
        assert((U32)CurrentSCEntry_p >= (U32)PARSER_Data_p->SCBuffer.SCList_Start_p);
        assert((U32)CurrentSCEntry_p <= (U32)PARSER_Data_p->SCBuffer.SCList_Loop_p);

        /* Need to inform the FDMA about the ES Copy address of the last parsed Start Code entry */
        VIDINJ_TransferSetESCopyBufferRead(PARSER_Data_p->Inject.InjecterHandle,
                                           PARSER_Data_p->Inject.InjectNum,
                                           (void *)(U32)GetSCAddForESCopyBuffer(CurrentSCEntry_p));

        TrESCopy(("\r\nPARSER: Read Address sent to ESCopy buffer=0x%08x",
                                            (U32)GetSCAddForESCopyBuffer(CurrentSCEntry_p)));
    #endif /* defined(DVD_SECURED_CHIP) */

    /* Inform the FDMA on the read pointer of the decoder in Bit Buffer */
    VIDINJ_TransferSetESRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, (void*) (((U32)InformReadAddressParams_p->DecoderReadAddress_p)&0xffffff80));

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

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
static ST_ErrorCode_t vc1par_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t  * PARSER_Data_p;
    U8                      * LatchedES_Write_p = NULL;             /* write pointer (of FDMA) in ES buffer   */
    U8                      * LatchedES_DecoderRead_p = NULL;       /* Read pointer (of decoder) in ES buffer */

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

#ifdef VALID_TOOLS
/*******************************************************************************
Name        : vc1par_RegisterTrace
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VC1PARSER_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef TRACE_UART
	TRACE_ConditionalRegister(VC1PAR_TRACE_BLOCKID, VC1PAR_TRACE_BLOCKNAME, vc1par_TraceItemList, sizeof(vc1par_TraceItemList)/sizeof(TRACE_Item_t));
#endif /* TRACE_UART */

	return(ErrorCode);
}
#endif /* VALID_TOOLS */


static ST_ErrorCode_t	vc1par_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle,
												void ** const BaseAddress_p,
												U32 * const Size_p)
{
	VC1ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    *BaseAddress_p    = STAVMEM_DeviceToVirtual(PARSER_Data_p->PESBuffer.PESBufferBase_p, &VirtualMapping);
    *Size_p           = (U32)PARSER_Data_p->PESBuffer.PESBufferTop_p
                      - (U32)PARSER_Data_p->PESBuffer.PESBufferBase_p
                      + 1;
    return(ST_NO_ERROR);
}



static ST_ErrorCode_t	vc1par_SetDataInputInterface(const PARSER_Handle_t ParserHandle,
								ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
								void (*InformReadAddress)(void * const Handle, void * const Address),
								void * const Handle)

{
	ST_ErrorCode_t Err = ST_NO_ERROR;
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Ensure no FDMA transfer is done while setting a new data input interface */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);


    Err = VIDINJ_Transfer_SetDataInputInterface(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum,
                                    GetWriteAddress,InformReadAddress,Handle);

	/* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);


    return(Err);

}


/*******************************************************************************
Name        : SavePTSEntry
Description : Stores a new parsed PTSEntry in the PTS StartCode List
Parameters  : Parser handle, PTS Entry Pointer in FDMA's StartCode list
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void vc1par_SavePTSEntry(const PARSER_Handle_t ParserHandle, STFDMA_SCEntry_t* PTSEntry_p)
{
	VC1ParserPrivateData_t * PARSER_Data_p;
	U32 FrameRate = VC1_DEFAULT_FRAME_RATE;
	U32 TickDiff;
	BOOL FrameRatePresent = FALSE;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

#if defined(DVD_SECURED_CHIP)
    /* The PTS entry points to bit buffer */
    if ((U32)GetSCAddForBitBuffer(PTSEntry_p) != (U32)PARSER_Data_p->LastPTSStored.Address_p)
#else /* defined(DVD_SECURED_CHIP) */
    if ((U32)GetSCAdd(PTSEntry_p, PARSER_Data_p) != (U32)PARSER_Data_p->LastPTSStored.Address_p)
#endif /* defined(DVD_SECURED_CHIP) */
    {
        /* Check if we can find a place to store the new PTS Entry, otherwise we overwrite the oldest one */
        if ( (U32)PARSER_Data_p->PTS_SCList.Read_p == (U32)PARSER_Data_p->PTS_SCList.Write_p + sizeof(PTS_t) )
        {
            /* First free the oldest one */
            /* Keep always an empty SCEntry because Read_p == Write_p means that the SCList is empty */
            PARSER_Data_p->PTS_SCList.Read_p++;
            /* Next overwrite it */
        }
        /* Store the PTS */
        PARSER_Data_p->PTS_SCList.Write_p->PTS33     = GetPTS33(PTSEntry_p);
        PARSER_Data_p->PTS_SCList.Write_p->PTS32     = GetPTS32(PTSEntry_p);
#if defined(DVD_SECURED_CHIP)
        PARSER_Data_p->PTS_SCList.Write_p->Address_p = (void*)GetSCAddForESCopyBuffer(PTSEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
        PARSER_Data_p->PTS_SCList.Write_p->Address_p = (void*)GetSCAdd(PTSEntry_p,PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */

		/* If frame rate has not been provided in the stream and we have already received at least one PTS before, estimate the frame
		   rate using the last 2 PTS's (see 6.1.14.4)*/
        if((!(PARSER_Data_p->StreamState.FrameRatePresent)) && (PARSER_Data_p->PTS_SCList.Read_p != PARSER_Data_p->PTS_SCList.Write_p))
		{
			/* Calculate frame rate (Ignore the cases in which the difference
			   between 2 PTSs is too large and wait for a next PTS)
			   If the previous PTS is less than the current PTS, then
			   just subtract the two PTS's, otherwise, there has been a
			   rollover, so account for that.*/
			if((PARSER_Data_p->LastPTSStored.PTS33 == PARSER_Data_p->PTS_SCList.Write_p->PTS33) &&
			   (PARSER_Data_p->LastPTSStored.PTS32 <  PARSER_Data_p->PTS_SCList.Write_p->PTS32))
			{
				TickDiff = PARSER_Data_p->PTS_SCList.Write_p->PTS32 - PARSER_Data_p->LastPTSStored.PTS32;

				/* If, for some reason, there is a gap between PTSs of over one second, then wait for the next
				   PTS before making the calculation.*/
				if(TickDiff < VC1_PTS_TICKS_PER_SECOND)
				{
					FrameRate = (VC1_PTS_TICKS_PER_SECOND / TickDiff);
					FrameRatePresent = TRUE;
				}
			}/* end if((PARSER_Data_p->LastPTSStored.PTS33 == PARSER_Data_p->PTS_SCList.Write_p->PTS33) ... */
			/* These are the cases where a rollover has occurred, Ignoring cases in which the difference is too big.
			          First, PTS32 has rolled over, but PTS33 has not */
			else if(((PARSER_Data_p->LastPTSStored.PTS33 < PARSER_Data_p->PTS_SCList.Write_p->PTS33) &&
				     (PARSER_Data_p->LastPTSStored.PTS32 >  PARSER_Data_p->PTS_SCList.Write_p->PTS32)) ||

					/* Second, PTS33 has rolled over */
					((PARSER_Data_p->LastPTSStored.PTS33 > PARSER_Data_p->PTS_SCList.Write_p->PTS33) &&
				     (PARSER_Data_p->LastPTSStored.PTS32 < PARSER_Data_p->PTS_SCList.Write_p->PTS32)))
			{
				/* Determine the number of ticks before the rollover of PTS32 and then add the number of ticks after
				   the rollover of PTS32, then add one for the tick that caused the rollover. */
				TickDiff = ((0xFFFFFFFF - PARSER_Data_p->LastPTSStored.PTS32) + PARSER_Data_p->PTS_SCList.Write_p->PTS32 + 1);

				/* If, for some reason, there is a gap between PTSs of over one second, then do not use these
				   PTS's for our frame rate calculation, but wait for the next PTS before making the calculation.*/
				if(TickDiff < VC1_PTS_TICKS_PER_SECOND)
				{
					FrameRate = (VC1_PTS_TICKS_PER_SECOND / TickDiff);
					FrameRatePresent = TRUE;
				}
			}

            if(FrameRatePresent)
			{
				PARSER_Data_p->StreamState.FrameRatePresent = TRUE;
				PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = FrameRate;
				PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = FrameRate;
			}

        }/* end if((!(PARSER_Data_p->StreamState.FrameRatePresent)) ...*/

        PARSER_Data_p->PTS_SCList.Write_p++;
        if (((U32)PARSER_Data_p->PTS_SCList.Write_p) >= ((U32)PARSER_Data_p->PTS_SCList.SCList + sizeof(PTS_t) * PTS_SCLIST_SIZE) )
        {
            PARSER_Data_p->PTS_SCList.Write_p = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList ;
        }
        PARSER_Data_p->LastPTSStored.PTS33      = GetPTS33(PTSEntry_p);
        PARSER_Data_p->LastPTSStored.PTS32      = GetPTS32(PTSEntry_p);

#if defined(DVD_SECURED_CHIP)
        PARSER_Data_p->LastPTSStored.Address_p  = (void*)GetSCAddForBitBuffer(PTSEntry_p);
#else /* defined(DVD_SECURED_CHIP) */
        PARSER_Data_p->LastPTSStored.Address_p  = (void*)GetSCAdd(PTSEntry_p, PARSER_Data_p);
#endif /* defined(DVD_SECURED_CHIP) */
    }
} /* End of vc1par_SavePTSEntry() function */

#ifdef ST_speed
#if defined(DVD_SECURED_CHIP)
/* DG TO DO: For trickmode on secured platforms, create h264par_GetESCopyBufferOutputCounter() */
#endif /* defined(DVD_SECURED_CHIP) */

/*******************************************************************************
Name        : vc1par_GetBitBufferOutputCounter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 vc1par_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle)
{                 /* To be implemented. Used to compute bit rate */
    VC1ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32 DiffPSC;
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    CPUNextSCEntry_p = STAVMEM_DeviceToCPU(PARSER_Data_p->SCBuffer.NextSCEntry, &VirtualMapping);

#if defined(DVD_SECURED_CHIP)
    if ((U32) (PARSER_Data_p->LastSCAdd_p) < (U32)GetSCAddForBitBuffer(CPUNextSCEntry_p))
    {
        DiffPSC = (U32)((U32)(GetSCAddForBitBuffer(CPUNextSCEntry_p)) - (U32)(PARSER_Data_p->LastSCAdd_p));
    }
    else
    {
        /* ES buffer (bit buffer) looped. */
        DiffPSC = ((U32)(PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p)
                  - (U32)(PARSER_Data_p->LastSCAdd_p)
                  + (U32)(GetSCAddForBitBuffer(CPUNextSCEntry_p))
                  - (U32)(PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p) + 1);
    }
#else /* defined(DVD_SECURED_CHIP) */
    if ((U32) (PARSER_Data_p->LastSCAdd_p) < (U32)GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p))
    {
        DiffPSC = (U32)((U32)(GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p)) - (U32)(PARSER_Data_p->LastSCAdd_p));
    }
    else
    {
        /* ES buffer (bit buffer) looped. */
        DiffPSC = ((U32)(PARSER_Data_p->BitBuffer.ES_Stop_p)
                  - (U32)(PARSER_Data_p->LastSCAdd_p)
                  + (U32)(GetSCAdd(CPUNextSCEntry_p,PARSER_Data_p))
                  - (U32)(PARSER_Data_p->BitBuffer.ES_Start_p) + 1);
    }
#endif /* defined(DVD_SECURED_CHIP) */

    PARSER_Data_p->OutputCounter += DiffPSC;
    return(PARSER_Data_p->OutputCounter);
}   /*End vc1par_GetBitBufferOutputCounter*/
#endif /*ST_speed*/


/*******************************************************************************
Name        : vc1par_Setup
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vc1par_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
	VC1ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    switch (SetupParams_p->SetupObject)
    {
/*****/ case STVID_SETUP_FDMA_NODES_PARTITION:   /****************************************************************************************/
            /* Ensure no FDMA transfer is done while setting the FDMA node partition */
            VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
            ErrorCode = VIDINJ_ReallocateFDMANodes(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, SetupParams_p);
            VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
            break;

/*****/ case STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION:   /********************************************************************************************/
            ErrorCode = vc1par_ReallocateStartCodeBuffer(ParserHandle, SetupParams_p);
            break;

/*****/ case STVID_SETUP_DATA_INPUT_BUFFER_PARTITION:   /****************************************************************************/
            ErrorCode = vc1par_ReallocatePESBuffer(ParserHandle, SetupParams_p);
            break;

/*****/ default:   /**********************************************************************************************************************/
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return(ErrorCode);
} /* End of vc1par_Setup() function */
/*******************************************************************************
Name        : vc1par_FlushInjection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void vc1par_FlushInjection(const PARSER_Handle_t ParserHandle)
{
    VC1ParserPrivateData_t * PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    U8                        Pattern_lenght = 4;
    U8                        Pattern[13]={0x00, 0x00, 0x01, 0xB4};

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
    /* if simple or main profile, then add signature in the start code */
    if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_MAIN || PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE )
    {
         Pattern_lenght = 13;
         Pattern[0] = 0x00;
         Pattern[1] = 0x00;
         Pattern[2] = 0x01;
         Pattern[3] = 0xB4;
         Pattern[4] = 0x53; /* S */
         Pattern[5] = 0x54; /* T */
         Pattern[6] = 0x4D; /* M */
         Pattern[7] = 0x49; /* I */
         Pattern[8] = 0x43; /* C */
         Pattern[9] = 0x00;
         Pattern[10] = 0x00;
         Pattern[11] = 0x01;
         Pattern[12] = 0xB4;
    }
#endif

    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, TRUE, Pattern, Pattern_lenght);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
#ifdef STVID_TRICKMODE_BACKWARD
	if (PARSER_Data_p->Backward)
    {
        PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    }
#endif /* STVID_TRICKMODE_BACKWARD */
    PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID ++;
    PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension = PARSER_Data_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID;
} /* vcv1par_FlushInjection */

#ifdef ST_speed
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : vc1par_SetBitBuffer
Description : Set the location of the bit buffer
Parameters  : HAL Parser manager handle, buffer address, buffer size in bytes
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void vc1par_SetBitBuffer(const PARSER_Handle_t        ParserHandle,
                         void * const                 BufferAddressStart_p,
                         const U32                    BufferSize,
                         const BitBufferType_t        BufferType,
                         const BOOL                   Stop  )
{
	VC1ParserPrivateData_t * PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    U8                        Pattern_lenght = 4;
    U8                        Pattern[13]={0x00, 0x00, 0x01, 0xB4};

#ifdef STVID_USE_VC1_MP_SC_SIGNATURE
    /* if simple or main profile, then add signature in the start code */
    if(PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_MAIN || PARSER_Data_p->PrivateGlobalContextData.Profile == VC9_PROFILE_SIMPLE )
    {
         Pattern_lenght = 13;
         Pattern[0] = 0x00;
         Pattern[1] = 0x00;
         Pattern[2] = 0x01;
         Pattern[3] = 0xB4;
         Pattern[4] = 0x53; /* S */
         Pattern[5] = 0x54; /* T */
         Pattern[6] = 0x4D; /* M */
         Pattern[7] = 0x49; /* I */
         Pattern[8] = 0x43; /* C */
         Pattern[9] = 0x00;
         Pattern[10] = 0x00;
         Pattern[11] = 0x01;
         Pattern[12] = 0xB4;
    }
#endif

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
         /* PES */          PARSER_Data_p->PESBuffer.PESBufferBase_p,
                            PARSER_Data_p->PESBuffer.PESBufferTop_p
#if defined(DVD_SECURED_CHIP)
        /* ES Copy */      ,PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p,
                            PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p
#endif /* defined(DVD_SECURED_CHIP) */
                           );

        /* Inform the FDMA on the read pointer of the decoder in Bit Buffer */
        VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, TRUE, Pattern, sizeof(Pattern));

        VIDINJ_TransferSetESRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, (void*) (((U32)PARSER_Data_p->BitBuffer.ES_DecoderRead_p)&0xffffff80));

#if defined(DVD_SECURED_CHIP)
        /* Need to inform the FDMA about the read pointer of the parser in ES Copy Buffer */
        VIDINJ_TransferSetESCopyBufferRead(PARSER_Data_p->Inject.InjecterHandle,
                                           PARSER_Data_p->Inject.InjectNum,
                                           (void *)GetSCAddForESCopyBuffer(PARSER_Data_p->SCBuffer.NextSCEntry));
#endif /* defined(DVD_SECURED_CHIP) */
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
           /* ES Copy */    (unsigned long)(PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p),
                            (unsigned long)(PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p)));
#endif /* defined(DVD_SECURED_CHIP) */

    /* Release FDMA transfers */
    STOS_SemaphoreSignal(PARSER_Data_p->ParserSC);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
} /* End of vc1par_SetBitBuffer() function */
/*******************************************************************************
Name        : vc1par_WriteStartCode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void vc1par_WriteStartCode(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p)
{
    const U8*  Temp8;
    VC1ParserPrivateData_t * PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
#endif /* defined(DVD_SECURED_CHIP) */

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

} /* End of vc1par_WriteStartCode */
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed*/

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
ST_ErrorCode_t vc1par_SecureInit(const PARSER_Handle_t          ParserHandle,
		                         const PARSER_ExtraParams_t *   const SecureInitParams_p)
{
	/* dereference ParserHandle to a local variable to ease debug */
	VC1ParserPrivateData_t *   PARSER_Data_p;
	ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

	PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Set default values for pointers in ES Copy Buffer */
    PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p = SecureInitParams_p->BufferAddress_p;

    /* ES Copy buffer stands from ESCopy_Start_p to ESCopy_Stop_p inclusive */
    PARSER_Data_p->ESCopyBuffer.ESCopy_Stop_p = (void *)((U32)SecureInitParams_p->BufferAddress_p + SecureInitParams_p->BufferSize -1);

	/* Reset ES Copy buffer write pointer */
	PARSER_Data_p->ESCopyBuffer.ESCopy_Write_p = PARSER_Data_p->ESCopyBuffer.ESCopy_Start_p;

	return (ErrorCode);
} /* end of vc1par_SecureInit() */
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
ST_ErrorCode_t vc1par_BitBufferInjectionInit(const PARSER_Handle_t         ParserHandle,
		                                     const PARSER_ExtraParams_t *  const BitBufferInjectionParams_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    VC1ParserPrivateData_t *   PARSER_Data_p;
    ST_ErrorCode_t             ErrorCode = ST_NO_ERROR;

    PARSER_Data_p = ((VC1ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Set default values for pointers in ES Buffer */
    PARSER_Data_p->BitBuffer.ES_Start_p = BitBufferInjectionParams_p->BufferAddress_p;
    /* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
    PARSER_Data_p->BitBuffer.ES_Stop_p = (void *)((U32)BitBufferInjectionParams_p->BufferAddress_p + BitBufferInjectionParams_p->BufferSize -1);
	/* Reset ES bit buffer write pointer */
	PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    return (ErrorCode);
} /* end of vc1par_BitBufferInjectionInit() */

/* End of vc1parser.c */
