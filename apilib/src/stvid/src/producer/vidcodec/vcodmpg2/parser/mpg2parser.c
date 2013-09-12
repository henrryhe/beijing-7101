/*******************************************************************************

File name   : mpg2parser.c

Description : MPEG-2 Video Parser top level CODEC API functions

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
 Author:       John Bean
10 Apr 2004        Created                          JRB John Bean, Scientific-Atlanta, Inc.

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "sttbx.h"

/* MPG2 Parser specific */
#include "mpg2parser.h"
#include "mpg2getbits.h"


/* Defines ------------------------------------------------------------------ */

#define PICTUREID_FROM_EXTTEMPREF(picid, extempref, temprefoffset)  \
        {                                                           \
                (picid).ID = (extempref) - (temprefoffset);         \
                (picid).IDExtension = (temprefoffset);              \
        }

#ifdef DEBUG_STARTCODE
#define FORMAT_SPEC_OSCLOCK "ll"
#define OSCLOCK_T_MILLE     1000LL
#endif

/* Constants ---------------------------------------------------------------- */
#ifdef DEBUG_STARTCODE
  static U32 nbCall = 0;
#endif

const PARSER_FunctionsTable_t PARSER_Mpeg2Functions =
{
#ifdef STVID_DEBUG_GET_STATISTICS
    mpg2par_ResetStatistics,
    mpg2par_GetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    mpg2par_GetStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    mpg2par_Init,
    mpg2par_Stop,
    mpg2par_Start,
    mpg2par_GetPicture,
    mpg2par_Term,
    mpg2par_InformReadAddress,
    mpg2par_GetBitBufferLevel,
    mpg2par_GetDataInputBufferParams,
    mpg2par_SetDataInputInterface,
    mpg2par_Setup,
    mpg2par_FlushInjection
#ifdef ST_speed
    ,mpg2par_GetBitBufferOutputCounter
#ifdef STVID_TRICKMODE_BACKWARD
    ,mpg2par_SetBitBuffer
    ,mpg2par_WriteStartCode
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed*/
    ,mpg2par_BitBufferInjectionInit
#if defined(DVD_SECURED_CHIP)
    ,mpg2par_SecureInit
#endif
};


/* Functions ---------------------------------------------------------------- */

#if defined(DVD_SECURED_CHIP)
static void mpg2par_DMATransferDoneFct(U32 ParserHandle, void * ES_Write_p, void * SCListWrite_p, void * SCListLoop_p, void * ESCopy_Write_p);
#else
static void mpg2par_DMATransferDoneFct(U32 ParserHandle, void * ES_Write_p, void * SCListWrite_p, void * SCListLoop_p);
#endif /* DVD_SECURED_CHIP */
static ST_ErrorCode_t mpg2par_AllocatePESBuffer(const PARSER_Handle_t ParserHandle, const U32 PESBufferSize, const PARSER_InitParams_t *  const InitParams_p);
static ST_ErrorCode_t mpg2par_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t mpg2par_AllocateSCBuffer(const PARSER_Handle_t ParserHandle, const U32 SC_ListSize, const PARSER_InitParams_t *  const InitParams_p);
static ST_ErrorCode_t mpg2par_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle);
static void mpg2par_InitSCBuffer(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t mpg2par_InitInjectionBuffers(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t *const InitParams_p, const U32 SC_ListSize, const U32 PESBufferSize);
static void mpg2par_CleanUpPrivateDataAllocation(const PARSER_Handle_t ParserHandle);
static void mpg2par_InitParserContext(const PARSER_Handle_t  ParserHandle);

/*******************************************************************************/
/*! \brief De-Allocate the PES Buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t mpg2par_DeAllocatePESBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    MPG2ParserPrivateData_t *   PARSER_Data_p;
    PARSER_Properties_t *       PARSER_Properties_p;
    STAVMEM_FreeBlockParams_t   FreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

	PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = (MPG2ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->PESBuffer.PESBufferHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The PESBuffer is not deallocated correctly !"));
    }

	return (ErrorCode);
} /* end of mpg2par_DeAllocatePESBuffer() */

/*******************************************************************
   Function:    mpg2par_AllocatePESBuffer

   Parameters:  ParserHandle
                SC_ListSize - Start Code buffer size in bytes
                InitParams_p - Init parameters from CODEC API call

   Returns:     ST_ERROR_NO_MEMORY - memory alloc failure
                ST_ERROR_BAD_PARAMETER - memory alloc failure
                STAVMEM_ERROR_INVALID_PARTITION_HANDLE - memory alloc failure
                STAVMEM_ERROR_MAX_BLOCKS - memory alloc failure
                ST_NO_ERROR

   Scope:       Parser Local

   Purpose:     Allocate PES buffers

   Behavior:

   Exceptions:  None

*******************************************************************/
static ST_ErrorCode_t mpg2par_AllocatePESBuffer
(
   const PARSER_Handle_t        ParserHandle,
   const U32                    PESBufferSize,
   const PARSER_InitParams_t *  const InitParams_p
)
{
   PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
   MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
	STAVMEM_AllocBlockParams_t                  AllocBlockParams;
   void *                                      VirtualAddress;
   STAVMEM_SharedMemoryVirtualMapping_t        VirtualMapping;
	ST_ErrorCode_t                              ErrorCode;

	/* Allocate an Input Buffer (PES) */
    /*--------------------------*/
    AllocBlockParams.PartitionHandle          = InitParams_p->AvmemPartitionHandle;
    AllocBlockParams.Alignment                = 128; /* limit for FDMA */ /*128 */
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
         ErrorCode =  mpg2par_DeAllocatePESBuffer(ParserHandle);
        return(ErrorCode);
    }
    PARSER_Data_p->PESBuffer.PESBufferBase_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
    PARSER_Data_p->PESBuffer.PESBufferTop_p = (void *) ((U32)PARSER_Data_p->PESBuffer.PESBufferBase_p + PESBufferSize - 1);

	return (ErrorCode);
} /* end of mpg2par_AllocatePESBuffer() */

/*******************************************************************************/
/*! \brief De-Allocate the Start Code buffer
 *
 * \param ParserHandle Parser Handle
 * \return ST_ErrorCode_t
 */
/*******************************************************************************/
static ST_ErrorCode_t mpg2par_DeAllocateSCBuffer(const PARSER_Handle_t ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
    MPG2ParserPrivateData_t * PARSER_Data_p;
    PARSER_Properties_t         * PARSER_Properties_p;
    STAVMEM_FreeBlockParams_t   FreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    PARSER_Data_p = (MPG2ParserPrivateData_t *)PARSER_Properties_p->PrivateData_p;

    FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->SCBuffer.SCListHdl));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "The SCBuffer is not deallocated correctly !"));
    }

    return(ErrorCode);
} /* end of mpg2par_DeAllocateSCBuffer() */

/*******************************************************************
Function:    mpg2par_AllocateSCBuffer

Parameters:  ParserHandle
             SC_ListSize - Start Code buffer size in bytes
             InitParams_p - Init parameters from CODEC API call

Returns:     ST_ERROR_NO_MEMORY - memory alloc failure
             ST_ERROR_BAD_PARAMETER - memory alloc failure
             STAVMEM_ERROR_INVALID_PARTITION_HANDLE - memory alloc failure
             STAVMEM_ERROR_MAX_BLOCKS - memory alloc failure
             ST_NO_ERROR

Scope:       Parser Local

Purpose:     Allocate SC+PTS buffers

Behavior:

Exceptions:  None

*******************************************************************/
static ST_ErrorCode_t mpg2par_AllocateSCBuffer
(
   const PARSER_Handle_t        ParserHandle,
   const U32                    SC_ListSize,
	const PARSER_InitParams_t *  const InitParams_p
)
{
   PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
   MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
   STAVMEM_AllocBlockParams_t              AllocBlockParams;
   void *                                  VirtualAddress;
   STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
   ST_ErrorCode_t                          ErrorCode;

   AllocBlockParams.PartitionHandle          = InitParams_p->AvmemPartitionHandle;
   AllocBlockParams.Alignment                = 32;
   AllocBlockParams.Size                     = SC_ListSize * sizeof(STFDMA_SCEntry_t);
   AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
   AllocBlockParams.NumberOfForbiddenRanges  = 0;
   AllocBlockParams.ForbiddenRangeArray_p    = NULL;
   AllocBlockParams.NumberOfForbiddenBorders = 0;
   AllocBlockParams.ForbiddenBorderArray_p   = NULL;
   STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

   ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &PARSER_Data_p->SCBuffer.SCListHdl);
   if (ErrorCode != ST_NO_ERROR)
   {
        return (ErrorCode);
   }
   ErrorCode = STAVMEM_GetBlockAddress(PARSER_Data_p->SCBuffer.SCListHdl,(void **)&VirtualAddress);
   if (ErrorCode != ST_NO_ERROR)
   {
        ErrorCode = mpg2par_DeAllocateSCBuffer(ParserHandle);
        return (ErrorCode);
   }
   PARSER_Data_p->SCBuffer.SCList_Start_p  = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
   PARSER_Data_p->SCBuffer.SCList_Stop_p   = PARSER_Data_p->SCBuffer.SCList_Start_p;
   PARSER_Data_p->SCBuffer.SCList_Stop_p  += (SC_ListSize-1);
#ifdef DEBUG_STARTCODE
   Previous_SCListWrite_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
#endif

   return (ErrorCode);
}

/*******************************************************************
Function:    mpg2par_InitSCBuffer

Parameters:  ParserHandle
             InitParams_p

Returns:     None

Scope:       Parser Local

Purpose:     Initialize SC structure pointers

Behavior:

Exceptions:  None

*******************************************************************/
static void mpg2par_InitSCBuffer(const PARSER_Handle_t ParserHandle)
{
   PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
   MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

	/* Set default values for pointers in SC-List */
    PARSER_Data_p->SCBuffer.SCList_Write_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.SCList_Loop_p  = PARSER_Data_p->SCBuffer.SCList_Start_p;
    PARSER_Data_p->SCBuffer.NextSCEntry    = PARSER_Data_p->SCBuffer.SCList_Start_p;

    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
    PARSER_Data_p->PTS_SCList.Write_p          = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
    PARSER_Data_p->PTS_SCList.Read_p           = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
    PARSER_Data_p->LastPTSStored.Address_p     = NULL;
}

/*******************************************************************
Function:    mpg2par_DMATransferDoneFct

Parameters:  ParserHandle
             ES_Write_p - FDMA write pointer in ES bit buffer
             SCListWrite_p - FDMA write pointer in SC List
             SCListLoop_p - FDMA write pointer upper bounday in SC List

Returns:     ST_ERROR_NO_MEMORY - memory alloc failure
             ST_ERROR_BAD_PARAMETER - memory alloc failure
             STAVMEM_ERROR_INVALID_PARTITION_HANDLE - memory alloc failure
             STAVMEM_ERROR_MAX_BLOCKS - memory alloc failure
             ST_NO_ERROR

Scope:       Parser Local

Purpose:     Allocate PES buffers

Behavior:    This function is called from inject module when data is
             transfered

Exceptions:  None

*******************************************************************/
static void mpg2par_DMATransferDoneFct(U32 ParserHandle, void * ES_Write_p, void * SCListWrite_p, void * SCListLoop_p
#if defined(DVD_SECURED_CHIP)
   , void * ESCopy_Write_p
#endif  /* DVD_SECURED_CHIP */
)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
#ifdef DEBUG_STARTCODE
    U32 i;
#endif

    STOS_SemaphoreWait(PARSER_Data_p->InjectSharedAccess);

#ifdef DEBUG_STARTCODE
   DMATransferTime = STOS_time_now();
   if ((U32)(SCListWrite_p) == (U32)(Previous_SCListWrite_p))
   {
      nbSCW = 0;
   }
   else
   {
     if ((U32)(SCListWrite_p) > (U32)(Previous_SCListWrite_p))
     {
        nbSCW = ((U32)SCListWrite_p - (U32)Previous_SCListWrite_p) / sizeof(STFDMA_SCEntry_t);
     }
     else
     {
        nbSCW = ((U32)PARSER_Data_p->SCBuffer.SCList_Stop_p - (U32)Previous_SCListWrite_p + (U32)SCListWrite_p - (U32)PARSER_Data_p->SCBuffer.SCList_Start_p) / sizeof(STFDMA_SCEntry_t);
     }
   }
   for (i = 0, SClist_p = Previous_SCListWrite_p; i < nbSCW; i++, indAdr++)
   {
      SCListAddress[indAdr] = (U8 *)GetSCAdd(SClist_p);
      if (SClist_p < SCListLoop_p)
      {
        SClist_p++;
      }
      else
      {
        SClist_p = PARSER_Data_p->SCBuffer.SCList_Start_p;
      }
   }
   sprintf (&tracesSC[indSC][0], "mpg2par_DMATransferDoneFct;%"FORMAT_SPEC_OSCLOCK"d;%d;%d;%d;%d\n",
                                                        DMATransferTime * OSCLOCK_T_MILLE/ST_GetClocksPerSecond(),
                                                        nbSCW,
                                                        ES_Write_p,
                                                        SCListWrite_p,
                                                        SCListLoop_p);
   indSC++;
   Previous_SCListWrite_p = SCListWrite_p;
#endif

#ifdef STVID_TRICKMODE_BACKWARD
    if ((!PARSER_Data_p->Backward) || (PARSER_Data_p->StopParsing))
#endif /* STVID_TRICKMODE_BACKWARD */
	{
		PARSER_Data_p->BitBuffer.ES_Write_p     = ES_Write_p;
    	PARSER_Data_p->SCBuffer.SCList_Write_p  = SCListWrite_p;
    	PARSER_Data_p->SCBuffer.SCList_Loop_p   = SCListLoop_p;
	}

    STOS_SemaphoreSignal(PARSER_Data_p->InjectSharedAccess);
} /* end of mpg2par_DMATransferDoneFct() */

/*******************************************************************
Function:    mpg2par_InitInjectionBuffers

Parameters:  ParserHandle - Parser Handle
             InitParams_p - parameters from CODEC API call
             SC_ListSize - Start Code buffer size in bytes
             PESBufferSize - PES Buffer size in bytes

Returns:     ST_ERROR_NO_MEMORY - memory alloc failure
             ST_ERROR_BAD_PARAMETER - memory alloc failure
             STAVMEM_ERROR_INVALID_PARTITION_HANDLE - memory alloc failure
             STAVMEM_ERROR_MAX_BLOCKS - memory alloc failure
             ST_NO_ERROR

Scope:       Parser Local

Purpose:     Allocate PES, SC+PTS buffers, Init Injector buffs

Behavior:

Exceptions:  None

*******************************************************************/
static ST_ErrorCode_t mpg2par_InitInjectionBuffers
(
   const PARSER_Handle_t        ParserHandle,
   const PARSER_InitParams_t *  const InitParams_p,
   const U32                    SC_ListSize,
   const U32 				        PESBufferSize
)
{
   PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
   MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
   VIDINJ_GetInjectParams_t    Params;
   ST_ErrorCode_t              ErrorCode     = ST_NO_ERROR;

	/* Allocate SC Buffer */
   ErrorCode = mpg2par_AllocateSCBuffer(ParserHandle, SC_ListSize, InitParams_p);
	if (ErrorCode != ST_NO_ERROR)
	{
		return (ErrorCode);
	}

	/* Allocate PES Buffer */
   ErrorCode = mpg2par_AllocatePESBuffer(ParserHandle, PESBufferSize, InitParams_p);
	if (ErrorCode != ST_NO_ERROR)
	{
        ErrorCode = mpg2par_DeAllocateSCBuffer(ParserHandle);
        return (ErrorCode);
	}

	/* Set default values for pointers in ES Buffer */
   PARSER_Data_p->BitBuffer.ES_Start_p = InitParams_p->BitBufferAddress_p;
/*   STTST_AssignInteger("VVID_SBAG_BBSTARTADR",PARSER_Data_p->BitBuffer.ES_Start_p,0);*/

   /* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
   PARSER_Data_p->BitBuffer.ES_Stop_p = (void *)((U32)InitParams_p->BitBufferAddress_p + InitParams_p->BitBufferSize -1);
/*   STTST_AssignInteger("VVID_SBAG_BBENDADR",PARSER_Data_p->BitBuffer.ES_Start_p,0);*/
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_Data_p->BitBuffer.ES_StoreStart_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    PARSER_Data_p->BitBuffer.ES_StoreStop_p = PARSER_Data_p->BitBuffer.ES_Stop_p;
#endif  /* STVID_TRICKMODE_BACKWARD */

	/* Initialize the SC buffer pointers */
	mpg2par_InitSCBuffer(ParserHandle);

	/* Reset ES bit buffer write pointer */
    PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

    /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

	/* Synchronize buffer definitions with FDMA */
	/* link this init with a fdma channel */
   /*------------------------------------*/
   Params.TransferDoneFct                 = mpg2par_DMATransferDoneFct;
   Params.UserIdent                       = (U32)ParserHandle;
   Params.AvmemPartition                  = InitParams_p->AvmemPartitionHandle;
   Params.CPUPartition                    = InitParams_p->CPUPartition_p;
    PARSER_Data_p->Inject.InjectNum        = VIDINJ_TransferGetInject(PARSER_Data_p->Inject.InjecterHandle, &Params);
        if (PARSER_Data_p->Inject.InjectNum == BAD_INJECT_NUM)
    {
         ErrorCode = mpg2par_DeAllocatePESBuffer(ParserHandle);
         ErrorCode = mpg2par_DeAllocateSCBuffer(ParserHandle);
        return(ST_ERROR_NO_MEMORY);
    }

	return (ErrorCode);
} /* end of mpg2par_InitInjectionBuffers() */



/*******************************************************************
Function:    mpg2par_CleanUpPrivateDataAllocation

Parameters:  PARSER_Data_p

Returns:     None

Scope:       Parser Local

Purpose:     De-Allocate all buffers allocated for in Init

Behavior:

Exceptions:  None

*******************************************************************/
static void mpg2par_CleanUpPrivateDataAllocation(const PARSER_Handle_t ParserHandle)
{
   PARSER_Properties_t               * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t           * PARSER_Data_p = PARSER_Properties_p->PrivateData_p;
   VIDCOM_ParsedPictureInformation_t * ParsedPictInfo_p;
   STAVMEM_FreeBlockParams_t           FreeParams;
	ST_ErrorCode_t                      ErrorCode     = ST_NO_ERROR;
   /* Deallocate the STAVMEM partitions */
   FreeParams.PartitionHandle = PARSER_Properties_p->AvmemPartitionHandle;

   if (PARSER_Data_p != NULL)
   {
	   /* SC Buffer */
       if(PARSER_Data_p->SCBuffer.SCList_Start_p != 0)
      {
          ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->SCBuffer.SCListHdl));
      }

	   /* PES Buffer */
       if(PARSER_Data_p->PESBuffer.PESBufferBase_p != 0)
      {
         ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PARSER_Data_p->PESBuffer.PESBufferHdl));
      }

	   /* De-allocate memory allocated at init */
      if (PARSER_Data_p->UserData.Buff_p != NULL)
      {
         STOS_MemoryDeallocate(PARSER_Data_p->CPUPartition_p, PARSER_Data_p->UserData.Buff_p);
      }

      ParsedPictInfo_p = PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p;
      if (ParsedPictInfo_p != NULL)
      {
         if (ParsedPictInfo_p->PictureDecodingContext_p != NULL)
         {
            if (ParsedPictInfo_p->PictureDecodingContext_p->GlobalDecodingContext_p != NULL)
            {
               SAFE_MEMORY_DEALLOCATE(ParsedPictInfo_p->PictureDecodingContext_p->GlobalDecodingContext_p,
                     PARSER_Data_p->CPUPartition_p, sizeof(VIDCOM_GlobalDecodingContext_t));
            }
            SAFE_MEMORY_DEALLOCATE(ParsedPictInfo_p->PictureDecodingContext_p,
                 PARSER_Data_p->CPUPartition_p, sizeof(VIDCOM_PictureDecodingContext_t));
         }
         SAFE_MEMORY_DEALLOCATE(PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p,
             PARSER_Data_p->CPUPartition_p, sizeof(VIDCOM_ParsedPictureInformation_t));
      }

      /* finally clean the private data itself */
      SAFE_MEMORY_DEALLOCATE(PARSER_Data_p, PARSER_Data_p->CPUPartition_p, sizeof(MPG2ParserPrivateData_t));
      PARSER_Properties_p->PrivateData_p = NULL; /* reset as it has been deallocated */
    }
}

/*******************************************************************
Function:    mpg2par_Init

Parameters:  ParserHandle
             InitParams_p

Returns:     ST_ERROR_NO_MEMORY - memory alloc, or task start failure
             STVID_ERROR_EVENT_REGISTRATION - evt reg failed
             ST_NO_ERROR

Scope:       Codec API

Purpose:     Allocate PES, SC+PTS buffers, Initialize semaphores,
             tasks, Data Partition (to allocate PES + SC/PTS buffer,
             as well as local buffers), ES bit buffer (start + size)
             Handles: FDMA, Events

Behavior:

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t mpg2par_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    VIDCOM_ParsedPictureInformation_t * ParsedPictInfo_p;
    U32 UserDataSize;

    /* Validate Parameters */
        if ((ParserHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate private data structure */
    SAFE_MEMORY_ALLOCATE(PARSER_Properties_p->PrivateData_p, MPG2ParserPrivateData_t *, InitParams_p->CPUPartition_p, sizeof(MPG2ParserPrivateData_t) );
    PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    if (PARSER_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    else
    {
        memset(PARSER_Properties_p->PrivateData_p, 0xA5, sizeof(MPG2ParserPrivateData_t));
    }

    PARSER_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;

    PARSER_Properties_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;

    /* Allocate parsed picture information */
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p = NULL;
    SAFE_MEMORY_ALLOCATE(PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p,
            VIDCOM_ParsedPictureInformation_t *, InitParams_p->CPUPartition_p,
            sizeof(VIDCOM_ParsedPictureInformation_t) );
    if (PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p == NULL)
    {
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return(ST_ERROR_NO_MEMORY);
    }
    else
    {
        memset(PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p, 0xA5, sizeof(VIDCOM_ParsedPictureInformation_t));
    }

    ParsedPictInfo_p = PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p;
    ParsedPictInfo_p->PictureSpecificData_p = &(PARSER_Data_p->PictureSpecificData);
    ParsedPictInfo_p->SizeOfPictureSpecificDataInByte = sizeof(MPEG2_PictureSpecificData_t);

    /* Allocate Picture Decoding Context */
    SAFE_MEMORY_ALLOCATE(ParsedPictInfo_p->PictureDecodingContext_p,
            VIDCOM_PictureDecodingContext_t *, InitParams_p->CPUPartition_p,
            sizeof(VIDCOM_PictureDecodingContext_t) );
    if (ParsedPictInfo_p->PictureDecodingContext_p == NULL)
    {
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return(ST_ERROR_NO_MEMORY);
    }
    else
    {
        memset(ParsedPictInfo_p->PictureDecodingContext_p, 0xA5, sizeof(VIDCOM_PictureDecodingContext_t));
    }
    ParsedPictInfo_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p = &(PARSER_Data_p->PictureDecodingContextSpecificData);
    ParsedPictInfo_p->PictureDecodingContext_p->SizeOfPictureDecodingContextSpecificDataInByte = sizeof(MPEG2_PictureDecodingContextSpecificData_t);

    /* Allocate Global Decoding Context */
    ParsedPictInfo_p->PictureDecodingContext_p->GlobalDecodingContext_p = NULL;
    SAFE_MEMORY_ALLOCATE(ParsedPictInfo_p->PictureDecodingContext_p->GlobalDecodingContext_p,
            VIDCOM_GlobalDecodingContext_t *, InitParams_p->CPUPartition_p,
            sizeof(VIDCOM_GlobalDecodingContext_t) );
    if (ParsedPictInfo_p->PictureDecodingContext_p->GlobalDecodingContext_p == NULL)
    {
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return(ST_ERROR_NO_MEMORY);
    }
    else
    {
        memset(ParsedPictInfo_p->PictureDecodingContext_p->GlobalDecodingContext_p, 0xA5, sizeof(VIDCOM_GlobalDecodingContext_t));
    }
    ParsedPictInfo_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p = &(PARSER_Data_p->GlobalDecodingContextSpecificData);
    ParsedPictInfo_p->PictureDecodingContext_p->GlobalDecodingContext_p->SizeOfGlobalDecodingContextSpecificDataInByte = sizeof(MPEG2_GlobalDecodingContextSpecificData_t);
    PARSER_Data_p->ParserState = PARSER_STATE_IDLE;

    /* Allocate table of user data - note min required for parser's use */
    UserDataSize = InitParams_p->UserDataSize;
    if (UserDataSize < MIN_USER_DATA_SIZE)
    {
        UserDataSize = MIN_USER_DATA_SIZE;
    }
    PARSER_Data_p->UserDataSize = UserDataSize;
    PARSER_Data_p->UserData.Length = 0;
    PARSER_Data_p->UserData.Buff_p = STOS_MemoryAllocate(InitParams_p->CPUPartition_p, UserDataSize);
    if (PARSER_Data_p->UserData.Buff_p == NULL)
    {
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return(ST_ERROR_NO_MEMORY);
    }

    /* Get handle of VIDINJ injecter */
    PARSER_Data_p->Inject.InjecterHandle = InitParams_p->InjecterHandle;

    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;

    /* Allocation and Initialisation of injection buffers. The FDMA is also informed about these allocations */
    ErrorCode = mpg2par_InitInjectionBuffers(ParserHandle, InitParams_p, SC_LIST_SIZE_IN_ENTRY, PES_BUFFER_SIZE);
    if (ErrorCode != ST_NO_ERROR)
    {
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
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

    /* Register to send the PARSER_JOB_COMPLETED_EVT */
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_JOB_COMPLETED_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_JOB_COMPLETED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module parser init: could not register events !"));
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_PICTURE_SKIPPED_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_PICTURE_SKIPPED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module parser init: could not register events !"));
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register to send the PARSER_USER_DATA_EVT */
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_USER_DATA_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_USER_DATA_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module parser init: could not register events !"));
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }
#ifdef STVID_TRICKMODE_BACKWARD
    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_BITBUFFER_FULLY_PARSED_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_BITBUFFER_FULLY_PARSED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module parser init: could not register events !"));
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }
#endif

    ErrorCode = STEVT_RegisterDeviceEvent(PARSER_Properties_p->EventsHandle, InitParams_p->VideoName, PARSER_FIND_DISCONTINUITY_EVT, &(PARSER_Data_p->RegisteredEventsID[PARSER_FIND_DISCONTINUITY_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module parser init: could not register events !"));
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Initialise commands queue */
    PARSER_Data_p->CommandsBuffer.Data_p         = PARSER_Data_p->CommandsData;
    PARSER_Data_p->CommandsBuffer.TotalSize      = sizeof(PARSER_Data_p->CommandsData);
    PARSER_Data_p->CommandsBuffer.BeginPointer_p = PARSER_Data_p->CommandsBuffer.Data_p;
    PARSER_Data_p->CommandsBuffer.UsedSize       = 0;
    PARSER_Data_p->CommandsBuffer.MaxUsedSize    = 0;

    /* Semaphore to share variables between parser and injection tasks */
    PARSER_Data_p->InjectSharedAccess = STOS_SemaphoreCreateFifo(PARSER_Properties_p->CPUPartition_p,1);

    /* Semaphore between parser and controller tasks */
    /* Parser main task does not run until it gets a start command from the controller */
    PARSER_Data_p->ParserOrder = STOS_SemaphoreCreateFifo(PARSER_Properties_p->CPUPartition_p,0);
    PARSER_Data_p->ParserSC = STOS_SemaphoreCreateFifoTimeOut(PARSER_Properties_p->CPUPartition_p,0);
    PARSER_Data_p->ParserState = PARSER_STATE_IDLE;

    /*    Initialize incrementing Frame ID for Pictures*/
    PARSER_Data_p->CurrentPictureDecodingOrderFrameID = 0;

    /* Create parser task */
    PARSER_Data_p->ParserTask.IsRunning = FALSE;
    ErrorCode = mpg2par_StartParserTask(ParserHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
        return (ST_ERROR_BAD_PARAMETER);
    }
    PARSER_Data_p->ParserTask.IsRunning = TRUE;

    PARSER_Data_p->DiscontinuityDetected = FALSE;

    /*    Initialize handle for GetBits code*/
    PARSER_Data_p->gbHandle = gb_InitHandle ();

    return (ErrorCode);
}

/*******************************************************************
Function:    mpg2par_Term

Parameters:  ParserHandle

Returns:     STAVMEM_ERROR_INVALID_PARTITION_HANDLE - dealloc failure
             ST_NO_ERROR

Scope:       Codec API

Purpose:     Terminate decode. Dealloc and disable
             everything set up in Init.

Behavior:

Exceptions:  None

*******************************************************************/

#ifdef BENCHMARK_WINCESTAPI
    P_ADDSEMAPHORE(PARSER_Data_p->ParserOrder, "PARSER_Data_p->ParserOrder");
    P_ADDSEMAPHORE(PARSER_Data_p->InjectSharedAccess, "PARSER_Data_p->InjectSharedAccess");
#endif

ST_ErrorCode_t mpg2par_Term(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = PARSER_Properties_p->PrivateData_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    if (PARSER_Data_p == NULL)
    {
        return (ST_ERROR_UNKNOWN_DEVICE);
    }
    /* Ensure no FDMA transfer is done while resetting */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* Stops the FDMA */
    VIDINJ_TransferStop(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    VIDINJ_TransferReleaseInject(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* Terminate the parser task */
	mpg2par_StopParserTask(ParserHandle);

	/* Delete semaphore to share variables between parser and injection tasks */
    STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p,PARSER_Data_p->InjectSharedAccess);
	/* Delete semaphore between parser and controler tasks */
    STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p,PARSER_Data_p->ParserOrder);
    STOS_SemaphoreDelete(PARSER_Properties_p->CPUPartition_p,PARSER_Data_p->ParserSC);

    /* Deallocate Private Data */
    if (PARSER_Data_p != NULL)
    {
        mpg2par_CleanUpPrivateDataAllocation(ParserHandle);
    }

    /*    Destroy handle for GetBits code*/
    gb_DestroyHandle(PARSER_Data_p->gbHandle);

    return (ErrorCode);
}

/*******************************************************************
Function:    mpg2par_InitParserContext

Parameters:  ParserHandle

Returns:     None

Scope:       Parser Local

Purpose:     Initializes the Parser's structures to an initialized state

Behavior:

Exceptions:  None

*******************************************************************/
static void mpg2par_InitParserContext(const PARSER_Handle_t  ParserHandle)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    VIDCOM_PictureGenericData_t * PictureGenericData_p = &PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData;
    MPEG2_PictureSpecificData_t * PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureSpecificData_p;

	U32 counter; /* loop index */

    PARSER_Data_p->StartCode.IsPending = FALSE; /* No start code already found */

    PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID = 0;

	/* First parsed picture will have a DecodingOrderFrameID = 0 */
    PARSER_Data_p->ParserGlobalVariable.CurrentPictureDecodingOrderFrameID = 0;

#ifdef DEBUG_STARTCODE
    indSC = nbSC = indAdr = 0;
#endif


/*   Initialize State Variables in Parsing Structures*/


/*   Picture Generic Data*/
    for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; counter++)
    {
        PictureGenericData_p->FullReferenceFrameList[counter] = 0;
        PictureGenericData_p->IsValidIndexInReferenceFrame[counter] = FALSE;
    }
    for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; counter++)
    {
        PictureGenericData_p->ReferenceListP0[counter] = 0;
        PictureGenericData_p->ReferenceListB0[counter] = 0;
        PictureGenericData_p->ReferenceListB1[counter] = 0;
    }
    PictureGenericData_p->UsedSizeInReferenceListP0 = 0;
    PictureGenericData_p->UsedSizeInReferenceListB0 = 0;
    PictureGenericData_p->UsedSizeInReferenceListB1 = 0;

/*   IsPTSValid indicates whether a PTS has ever been received.*/
    PictureGenericData_p->PictureInfos.VideoParams.PTS.IsValid = FALSE;

    PictureGenericData_p->PictureInfos.VideoParams.PTS.Interpolated = TRUE;
    PictureGenericData_p->IsFirstOfTwoFields = FALSE;
    PictureGenericData_p->PreviousPictureHasAMissingField = FALSE;
    PictureGenericData_p->MissingFieldPictureDecodingOrderFrameID = 0;
    PictureGenericData_p->RepeatFirstField = FALSE;
    PictureGenericData_p->RepeatProgressiveCounter = 0;
    PictureGenericData_p->ParsingError = VIDCOM_ERROR_LEVEL_NONE;

/*   Global Decoding Context Specific Data*/
    GlobalDecodingContextSpecificData_p->FirstPictureEver = TRUE;
    GlobalDecodingContextSpecificData_p->DiscontinuityStartCodeDetected = FALSE;

/*   Default to no seq hdr or quant mtx extension*/
    GlobalDecodingContextSpecificData_p->NewSeqHdr = FALSE;
    GlobalDecodingContextSpecificData_p->NewQuantMxt = FALSE;
    GlobalDecodingContextSpecificData_p->StreamTypeChange = FALSE;
    GlobalDecodingContextSpecificData_p->HasSequenceDisplay = FALSE;
    GlobalDecodingContextSpecificData_p->HasGroupOfPictures = FALSE;

    GlobalDecodingContextSpecificData_p->load_intra_quantizer_matrix = TRUE;
    GlobalDecodingContextSpecificData_p->load_non_intra_quantizer_matrix = TRUE;

/*   Initialize Reference Picture Structure*/
    PARSER_Data_p->ReferencePicture.LastButOneRef.IsFree = TRUE;
    PARSER_Data_p->ReferencePicture.LastRef.IsFree = TRUE;
    PARSER_Data_p->ReferencePicture.MissingPrevious = TRUE;
    PARSER_Data_p->ReferencePicture.ReferencePictureSemaphore = STOS_SemaphoreCreatePriority(PARSER_Properties_p->CPUPartition_p,1);

/*  Initialize the Picture specific data */
    PictureSpecificData_p->IntraQuantMatrixHasChanged = FALSE;
    PictureSpecificData_p->NonIntraQuantMatrixHasChanged = FALSE;
    PictureSpecificData_p->PreviousExtendedTemporalReference = (U32) (-1);
    PictureSpecificData_p->temporal_reference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE;
    PictureSpecificData_p->PreviousTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE;
    PictureSpecificData_p->LastReferenceTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE;
    PictureSpecificData_p->LastButOneReferenceTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE;
    PictureSpecificData_p->PreviousMPEGFrame = STVID_MPEG_FRAME_I;
    PictureSpecificData_p->BackwardTemporalReferenceValue = -1;
    PictureSpecificData_p->ForwardTemporalReferenceValue = -1;

    mpg2par_InitStartCode(ParserHandle);
}


/*******************************************************************
Function:    mpg2par_Start

Parameters:  ParserHandle
             StartParams_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Configure FDMA

Behavior:
             Performs a context flush: will start as if it has to
             decode a first picture. The FDMA starts sending data.

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t mpg2par_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
/*    VIDINJ_ParserRanges_t       ParserRanges[3];*/

    PARSER_Data_p->StartParams_p = *StartParams_p;
   /* Ensure no FDMA transfer is done while updating ESRead */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Reset pointers in injection module */
    VIDINJ_TransferReset(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

	/* Save for future use: is inserted "as is" in the GlobalDecodingContextGenericData.SequenceInfo.StreamID structure */
    PARSER_Data_p->ParserGlobalVariable.StreamID = StartParams_p->StreamID;

	/* FDMA Setup ----------------- */

   /* cascade the buffers params and sc-detect config to inject sub-module */

/* To be added if removed from VIDINJ_TransferLimits
 * LD : 24 Oct 06 --> VIDINJ_SetSCRanges removed as already set in
                      VIDINJ_TransferLimits / FDMA_Reset functions call
   ParserRanges[0].RangeId                     = STFDMA_DEVICE_ES_RANGE_0;
   ParserRanges[0].RangeConfig.RangeEnabled    = TRUE;
   ParserRanges[0].RangeConfig.RangeStart      = FIRST_SLICE_START_CODE;
   ParserRanges[0].RangeConfig.RangeEnd        = GREATEST_SLICE_START_CODE;
   ParserRanges[0].RangeConfig.PTSEnabled      = FALSE;
   ParserRanges[0].RangeConfig.OneShotEnabled  = TRUE;

   ParserRanges[1].RangeId                     = STFDMA_DEVICE_ES_RANGE_1;
   ParserRanges[1].RangeConfig.RangeEnabled    = TRUE;
   ParserRanges[1].RangeConfig.RangeStart      = PICTURE_START_CODE;
   ParserRanges[1].RangeConfig.RangeEnd        = GROUP_START_CODE;
   ParserRanges[1].RangeConfig.PTSEnabled      = FALSE;
   ParserRanges[1].RangeConfig.OneShotEnabled  = FALSE;

   ParserRanges[2].RangeId                     = STFDMA_DEVICE_PES_RANGE_0;
   ParserRanges[2].RangeConfig.RangeEnabled    = TRUE;
   ParserRanges[2].RangeConfig.RangeStart      = SMALLEST_AUDIO_PACKET_START_CODE;
   ParserRanges[2].RangeConfig.RangeEnd        = GREATEST_VIDEO_PACKET_START_CODE;
   ParserRanges[2].RangeConfig.PTSEnabled      = TRUE;
   ParserRanges[2].RangeConfig.OneShotEnabled  = FALSE;   */

/*    Disable the PES range if required by the stream type*/
/*    if ((StartParams_p->StreamType & STVID_STREAM_TYPE_ES) == STVID_STREAM_TYPE_ES)
	{
		ParserRanges[2].RangeConfig.RangeEnabled    = FALSE;
	}
*/
   VIDINJ_TransferLimits(PARSER_Data_p->Inject.InjecterHandle,
                         PARSER_Data_p->Inject.InjectNum,
         /* BB */        PARSER_Data_p->BitBuffer.ES_Start_p,
                         PARSER_Data_p->BitBuffer.ES_Stop_p,
         /* SC */        PARSER_Data_p->SCBuffer.SCList_Start_p,
                         PARSER_Data_p->SCBuffer.SCList_Stop_p,
        /* PES */        PARSER_Data_p->PESBuffer.PESBufferBase_p,
                         PARSER_Data_p->PESBuffer.PESBufferTop_p);
/* LD : 24 Oct 06 --> VIDINJ_SetSCRanges removed as already set in
                      VIDINJ_TransferLimits / FDMA_Reset functions call
    VIDINJ_SetSCRanges(PARSER_Data_p->Inject.InjecterHandle,
                       PARSER_Data_p->Inject.InjectNum,
	 	 3, ParserRanges);
*/
/*    Start FDMA Transfer*/
    VIDINJ_TransferStart(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, StartParams_p->RealTime);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
	/* Initialize the parser variables */
    if (StartParams_p->ResetParserContext)
    {
        mpg2par_InitParserContext(ParserHandle);
    }
    GlobalDecodingContextSpecificData_p->TemporalReferenceOffset = 0;
    GlobalDecodingContextSpecificData_p->PictureCounter = 0;
    PARSER_Data_p->DiscontinuityDetected = FALSE;

    PARSER_Data_p->ParserGlobalVariable.ErrorRecoveryMode = StartParams_p->ErrorRecoveryMode;

    PARSER_Data_p->ParserState = PARSER_STATE_READY_TO_PARSE;

	return (ErrorCode);
}

/*******************************************************************
Function:    mpg2par_GetPicture

Parameters:  ParserHandle
             StartParams_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Start Parsing for a single picture

Behavior:

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t mpg2par_GetPicture(const PARSER_Handle_t ParserHandle, const PARSER_GetPictureParams_t * const GetPictureParams_p)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;


	/* Copy GetPictureParams_p in local data structure */
    PARSER_Data_p->GetPictureParams.GetRandomAccessPoint = GetPictureParams_p->GetRandomAccessPoint;
#ifdef ST_speed
    PARSER_Data_p->SkipMode = GetPictureParams_p->DecodedPictures;
#endif

	/* When the producer asks for a recovery point, erase context:
	 * - Reference list is emptied
	 * - previous picture information is discarded
	 * - global variables are reset
	 * The State of the parser is the same as for a "Start" command, except the FDMA is not run again
	 */
    if (PARSER_Data_p->GetPictureParams.GetRandomAccessPoint)
    {
        mpg2par_InitParserContext(ParserHandle);
        if (!GlobalDecodingContextSpecificData_p->FirstPictureEver)
        {
            GlobalDecodingContextSpecificData_p->TemporalReferenceOffset += TEMPORAL_REFERENCE_MODULO;
        }
    }

    PARSER_Data_p->ParserGlobalVariable.ErrorRecoveryMode = GetPictureParams_p->ErrorRecoveryMode;
	/* Fill job submission time */
    PARSER_Data_p->ParserJobResults.ParsingJobStatus.JobSubmissionTime = STOS_time_now();
    PARSER_Data_p->ParserJobResults.ParsingJobStatus.HasErrors = FALSE;
    PARSER_Data_p->ParserJobResults.ParsingJobStatus.ErrorCode = PARSER_NO_ERROR;

	/* Wakes up Parser task */
    vidcom_PushCommand(&PARSER_Data_p->CommandsBuffer, PARSER_COMMAND_START);
    STOS_SemaphoreSignal(PARSER_Data_p->ParserOrder);



    return (ErrorCode);
}

/*******************************************************************
Function:    mpg2par_Stop

Parameters:  ParserHandle

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Reset Parser

Behavior:    Equivalent to Term + Init call without the buffer
             deallocation. The FDMA is stopped.

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t mpg2par_Stop(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;


    PARSER_Data_p->ParserState = PARSER_STATE_IDLE;

/*    Notify the task*/
    vidcom_PushCommand(&PARSER_Data_p->CommandsBuffer, PARSER_COMMAND_STOP);

    /* Ensure no FDMA transfer is done while updating ESRead */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);


    /* Stops the FDMA */
    VIDINJ_TransferStop(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Initialize the SC buffer pointers */
    mpg2par_InitSCBuffer(ParserHandle);

    /* Reset ES bit buffer write pointer */
    PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;

   /* Reset Decoder Read pointer */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = PARSER_Data_p->BitBuffer.ES_Start_p;

   /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    return (ErrorCode);
}

/*******************************************************************
Function:    mpg2par_GetStatistics

Parameters:  ParserHandle
             Statistics_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Get the Parser statistics (TBD)

Behavior:

Exceptions:  None

*******************************************************************/
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t mpg2par_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;

    Statistics_p->DecodeInterruptBitBufferEmpty = PARSER_Data_p->Statistics.DecodeInterruptBitBufferEmpty;
    Statistics_p->DecodeInterruptBitBufferFull = PARSER_Data_p->Statistics.DecodeInterruptBitBufferFull;
    Statistics_p->DecodePbHeaderFifoEmpty = PARSER_Data_p->Statistics.DecodePbHeaderFifoEmpty;
    Statistics_p->DecodePbInterruptDecodeOverflowError = PARSER_Data_p->Statistics.DecodePbInterruptQueueOverflowError;
    Statistics_p->DecodePbStartCodeFoundInvalid = PARSER_Data_p->Statistics.DecodePbStartCodeFoundInvalid;
    Statistics_p->DecodePbStartCodeFoundVideoPES = PARSER_Data_p->Statistics.DecodePbStartCodeFoundVideoPES;
    Statistics_p->DecodeSequenceFound = PARSER_Data_p->Statistics.DecodeSequenceFound;
    Statistics_p->DecodeStartCodeFound = PARSER_Data_p->Statistics.DecodeStartCodeFound;
    Statistics_p->DecodeUserDataFound = PARSER_Data_p->Statistics.DecodeUserDatafound;

    return (ErrorCode);
}
/*******************************************************************
Function:    mpg2par_ResetStatistics

Parameters:  ParserHandle
             Statistics_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Reset the Parser statistics (TBD)

Behavior:

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t mpg2par_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    ST_ErrorCode_t ErrorCode = ST_ERROR_BAD_PARAMETER;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    PARSER_Data_p->Statistics.DecodeInterruptBitBufferEmpty = 0;
    PARSER_Data_p->Statistics.DecodeInterruptBitBufferFull = 0;
    PARSER_Data_p->Statistics.DecodePbHeaderFifoEmpty = 0;
    PARSER_Data_p->Statistics.DecodePbInterruptQueueOverflowError = 0;
    PARSER_Data_p->Statistics.DecodePbStartCodeFoundInvalid = 0;
    PARSER_Data_p->Statistics.DecodePbStartCodeFoundVideoPES = 0;
    PARSER_Data_p->Statistics.DecodeSequenceFound = 0;
    PARSER_Data_p->Statistics.DecodeStartCodeFound = 0;
    PARSER_Data_p->Statistics.DecodeUserDatafound = 0;

    return (ErrorCode);
}
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
static ST_ErrorCode_t mpg2par_GetStatus(const PARSER_Handle_t ParserHandle, STVID_Status_t * const Status_p)
{
    /* dereference ParserHandle to a local variable to ease debug */
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    ST_ErrorCode_t ErrorCode = ST_NO ERROR;

    PARSER_Data_p = ((MPG2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    VIDINJ_GetStatus(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, Status_p);

    /* Status_p->??? = PARSER_Data_p->Status.???; */

    return (ErrorCode);
} /* end of h264par_GetStatus() */
#endif /* STVID_DEBUG_GET_STATUS */


/*******************************************************************
Function:    mpg2par_InformReadAddress

Parameters:  ParserHandle
             InformReadAddressParams_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Update the read pointer for the ES buffer

Behavior:

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t mpg2par_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

    /* Stores the decoder read pointer locally for further computation of the bit buffer level */
    PARSER_Data_p->BitBuffer.ES_DecoderRead_p = InformReadAddressParams_p->DecoderReadAddress_p;

    /* Ensure no FDMA transfer is done while updating ESRead */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    /* Inform the FDMA on the read pointer of the decoder in Bit Buffer */
    VIDINJ_TransferSetESRead(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, InformReadAddressParams_p->DecoderReadAddress_p);

     /* Release FDMA transfers */
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    return (ST_NO_ERROR);
}

/*******************************************************************
Function:    mpg2par_GetBitBufferLevel

Parameters:  ParserHandle
             InformReadAddressParams_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Get the bit buffer level

Behavior:    The bit buffer level is maintained by the parser

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t mpg2par_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    U8                        * LatchedES_Write_p = NULL;             /* write pointer (of FDMA) in ES buffer   */
    U8                        * LatchedES_DecoderRead_p = NULL;       /* Read pointer (of decoder) in ES buffer */

#ifdef STVID_TRICKMODE_BACKWARD
    if (PARSER_Data_p->Backward)
    {
        U32     Level;
        U8      Pattern[]={0xAF, 0xAF, 0x00, 0x00, 0x01, 0xB6};

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
            GetBitBufferLevelParams_p->BitBufferLevelInBytes = PARSER_Data_p->BitBuffer.ES_Stop_p
                                                              - LatchedES_DecoderRead_p
                                                              + LatchedES_Write_p
                                                              - PARSER_Data_p->BitBuffer.ES_Start_p
                                                              + 1;
        }
        else
        {
            /* Normal: start <= read <= write <= top */
            GetBitBufferLevelParams_p->BitBufferLevelInBytes = LatchedES_Write_p
                                                              - LatchedES_DecoderRead_p;
        }
    }

/*    Ignore added startcode, since it will never be processed*/
    if (GetBitBufferLevelParams_p->BitBufferLevelInBytes < 8)
    {
        GetBitBufferLevelParams_p->BitBufferLevelInBytes = 0;
    }

    return (ErrorCode);
}


/*******************************************************************
Function:    mpg2par_GetDataInputBufferParams

Parameters:  ParserHandle
             BaseAddress_p
             Size_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Get the current base address and size of the PES buffer.

Behavior:

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t  mpg2par_GetDataInputBufferParams (const PARSER_Handle_t ParserHandle,
                                                void ** const BaseAddress_p,
                                                U32 * const Size_p)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;


    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    *BaseAddress_p    = STAVMEM_DeviceToVirtual(PARSER_Data_p->PESBuffer.PESBufferBase_p, &VirtualMapping);
    *Size_p           = (U32)PARSER_Data_p->PESBuffer.PESBufferTop_p
                    - (U32)PARSER_Data_p->PESBuffer.PESBufferBase_p
                    + 1;
    return(ST_NO_ERROR);

}



/*******************************************************************
Function:    mpg2par_SetDataInputInterface

Parameters:  ParserHandle
             GetWriteAddress - pointer to function to get
                               the write address
             InformReadAddress - pointer to function to inform read
                                 address

Returns:

Scope:       Codec API

Purpose:     Notifies the Injector of the functions it can use
             to manage the stream buffers.

Behavior:

Exceptions:  None

*******************************************************************/
ST_ErrorCode_t  mpg2par_SetDataInputInterface(const PARSER_Handle_t ParserHandle,
                                ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
                                void (*InformReadAddress)(void * const Handle, void * const Address),
                                void * const Handle)

{
    ST_ErrorCode_t Err;
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;


    Err = VIDINJ_Transfer_SetDataInputInterface(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum,
                                    GetWriteAddress,InformReadAddress,Handle);
    return(Err);

}

/*******************************************************************************
Name        : mpg2par_Setup
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t mpg2par_Setup(const PARSER_Handle_t ParserHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    MPG2ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((MPG2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Ensure no FDMA transfer is done while setting the FDMA node partition */
    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    ErrorCode = VIDINJ_ReallocateFDMANodes(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, SetupParams_p);
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);

    return(ErrorCode);
} /* End of mpg2par_Setup() function */

/*******************************************************************************
Name        : mpg2par_FlushInjection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void mpg2par_FlushInjection(const PARSER_Handle_t ParserHandle)
{
    MPG2ParserPrivateData_t   * PARSER_Data_p = ((MPG2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    U8                          Pattern[]={0xAF, 0xAF, 0x00, 0x00, 0x01, 0xB6};

    VIDINJ_EnterCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
    VIDINJ_TransferFlush(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum, TRUE, Pattern, sizeof(Pattern));
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
#ifdef STVID_TRICKMODE_BACKWARD
	if (PARSER_Data_p->Backward)
    {
        PARSER_Data_p->BitBuffer.ES_Write_p = PARSER_Data_p->BitBuffer.ES_Start_p;
    }
#endif /* STVID_TRICKMODE_BACKWARD */
    PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension = PARSER_Data_p->ParserGlobalVariable.ExtendedPresentationOrderPictureID;
} /* mpg2par_FlushInjection */

#ifdef ST_speed
/*******************************************************************************
Name        : mpg2par_GetBitBufferOutputCounter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 mpg2par_GetBitBufferOutputCounter(const PARSER_Handle_t ParserHandle)
{
    MPG2ParserPrivateData_t * PARSER_Data_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32 DiffPSC;
    STFDMA_SCEntry_t * CPUNextSCEntry_p; /* CPU address range to address Start code value in memory */

    PARSER_Data_p = ((MPG2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

} /*End mpg2par_GetBitBufferOutputCounter*/

#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : mpg2par_SetBitBuffer
Description : Set the location of the bit buffer
Parameters  : HAL Parser manager handle, buffer address, buffer size in bytes
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void mpg2par_SetBitBuffer(const PARSER_Handle_t        ParserHandle,
                                 void * const                 BufferAddressStart_p,
                                 const U32                    BufferSize,
                                 const BitBufferType_t        BufferType,
                                 const BOOL                   Stop  )
{
    U8                          Pattern[]={0xAF, 0xAF, 0x00, 0x00, 0x01, 0xB6};
    MPG2ParserPrivateData_t * PARSER_Data_p = ((MPG2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

    PARSER_Data_p->BitBuffer.ES_Write_p         = PARSER_Data_p->BitBuffer.ES_Start_p;
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
                            PARSER_Data_p->PESBuffer.PESBufferTop_p);
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
    VIDINJ_LeaveCriticalSection(PARSER_Data_p->Inject.InjecterHandle, PARSER_Data_p->Inject.InjectNum);
} /* End of mpg2par_SetBitBuffer() function */
/*******************************************************************************
Name        : mpg2par_WriteStartCode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void mpg2par_WriteStartCode(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p)
{
    const U8*  Temp8;
    MPG2ParserPrivateData_t * PARSER_Data_p = ((MPG2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

} /* End of mpg2par_WriteStartCode */
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
ST_ErrorCode_t mpg2par_SecureInit(const PARSER_Handle_t          ParserHandle,
		                          const PARSER_ExtraParams_t *   const SecureInitParams_p)
{
    /* Feature not supported */
    UNUSED_PARAMETER(ParserHandle);
    UNUSED_PARAMETER(SecureInitParams_p);

	return (ST_NO_ERROR);
} /* end of mpg2par_SecureInit() */
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
ST_ErrorCode_t mpg2par_BitBufferInjectionInit(const PARSER_Handle_t         ParserHandle,
		                                      const PARSER_ExtraParams_t *  const BitBufferInjectionParams_p)
{
    /* Feature not supported */
    UNUSED_PARAMETER(ParserHandle);
    UNUSED_PARAMETER(BitBufferInjectionParams_p);

    return (ST_NO_ERROR);
} /* end of mpg2par_BitBufferInjectionInit() */

/* End of mpg2parser.c */
