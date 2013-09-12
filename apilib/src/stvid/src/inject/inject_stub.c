/*******************************************************************************

File name   : inject_stub.c

Description : Control PES/ES->ES+SC List injection
			  No FDMA is called. This task simply loads a stream + full start code
              list data structure. The write pointer is set beyond the end of the
			  start code list and the loop pointer is set at the end, thus allowing
			  playing the stream endlessly.
			  of the list.
			  The faked function is: VIDINJ_TransferLimits
COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
 May 2003            Created                                      Michel Bruant
 Mar 2004            Updated (name alignement with inject.c)      Olivier Deygas

 Exported functions:
 -------------------
 VIDINJ_TransferGetInject
 VIDINJ_TransferReleaseInject
 VIDINJ_TransferStop
 VIDINJ_TransferStart
 VIDINJ_TransferLimits
 VIDINJ_Transfer_SetDataInputInterface
 VIDINJ_TransferSetESRead
 VIDINJ_TransferSetSCListRead

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */



#ifndef STTBX_PRINT
#define STTBX_PRINT
#endif /* STTBX_PRINT */

#ifndef STTBX_REPORT
#define STTBX_REPORT
#endif /* STTBX_REPORT */

#if !defined(ST_OSLINUX)
#include <stdio.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "stsys.h"
#include "sttbx.h"
#include "testtool.h"

#include "stavmem.h"
#include "inject.h"
#include "stfdma.h"
#include "vid_com.h"
#include "vid_ctcm.h"
#include "stos.h"

/* Constants ---------------------------------------------------------------- */

#define MAX_INJECTERS           3
#define SCLIST_MIN_SIZE         (500 * sizeof(STFDMA_SCEntry_t))
#define MS_TO_NEXT_TRANSFER     16

#define SC_LIST_IN_ENTRY        100000    /* TODO: see SC_LIST_SIZE_IN_ENTRY in h264parser.h */

#define NALU_TYPE_SLICE    1
#define NALU_TYPE_DPA      2
#define NALU_TYPE_DPB      3
#define NALU_TYPE_DPC      4
#define NALU_TYPE_IDR      5
#define NALU_TYPE_SEI      6
#define NALU_TYPE_SPS      7
#define NALU_TYPE_PPS      8
#define NALU_TYPE_PD       9
#define NALU_TYPE_EOSEQ    10
#define NALU_TYPE_EOSTREAM 11
#define NALU_TYPE_FILL     12

#define FDMA_PESSCD_NODE_ALIGNMENT      32
#define FDMA_MEMORY_BUFFER_ALIGNMENT    128

/* Structures --------------------------------------------------------------- */

typedef struct
{
    /* Input */
    void *                      InputBase_p;
    void *                      InputTop_p;
    void *                      InputWrite_p;
    void *                      InputRead_p;
    /* ES output */
    void *                      ES_Base_p;
    void *                      ES_Top_p;
    void *                      ES_Write_p;
    void *                      ES_Read_p;
    /* SC-List output */
    void *                      SCListBase_p;
    void *                      SCListTop_p;
    void *                      SCListLoop_p;
    void *                      SCListWrite_p;
    void *                      SCListRead_p;
    /* ext calls : share input info with injection */
    void *                      ExtHandle;
    ST_ErrorCode_t              (*GetWriteAddress)
                            (void * const Handle, void ** const Address_p);
    void                        (*InformReadAddress)
                            (void * const Handle, void * const Address);
    /* ext calls : share output info with hal-decode-video */
    VIDINJ_TransferDoneFct_t    UpdateHalDecodeFct;
    /* General */
    enum {STOPPED, RUNNING}     TransferState;
    BOOL                        IsRealTime;
    osclock_t                   NextInjectionTime;
    U32                         LoopsBeforeNextInjection;
    STFDMA_ContextId_t          FDMAContext;
    STFDMA_TransferId_t         FDMATransferId;
    STFDMA_GenericNode_t *      Node_p;
    STAVMEM_BlockHandle_t       NodeHdl;
    STFDMA_GenericNode_t *      NodeNext_p;
    STAVMEM_BlockHandle_t       NodeNextHdl;
    ST_Partition_t *            CPUPartition;
    STAVMEM_PartitionHandle_t   AvmemPartition;
    STAVMEM_PartitionHandle_t   FDMANodesAvmemPartition;
    U32                         HalDecodeIdent;
    semaphore_t *               TransferSemaphore_p;
    U8 *                        FlushBuffer_p;
    STAVMEM_BlockHandle_t       FlushBufferHdl;
#ifdef STVID_DEBUG_GET_STATISTICS
    U32 InjectFdmaTransfers;            /* Counts the number of FDMA transfers performed */
#endif /* STVID_DEBUG_GET_STATISTICS */
} InjContext_t;

typedef struct
{
    ST_Partition_t *    CPUPartition_p;
    STVID_DeviceType_t  DeviceType;
    VIDCOM_Task_t       InjectionTask;
    STFDMA_ChannelId_t  FDMAChannel; /* one channel for all dec */
    InjContext_t *      DMAInjecters[MAX_INJECTERS];
    U32                 ValidityCheck;
    BOOL                FDMASecureTransferMode; /* 0=non-secure, 1=secure */
} VIDINJ_Data_t;

/* Variables ---------------------------------------------------------------- */

static BOOL                 DMATransferInitDone = FALSE;
static InjContext_t *       DMAInjecters[MAX_INJECTERS];

#ifdef ST_OSLINUX
static void               * StubInject_BaseAddress_p = NULL;
static U32                  StubInject_Size = 0;

static U32                  StubInject_StreamSize = 0;
#endif /* ST_OSLINUX */

static task_t        * InjStubTask_p;
static tdesc_t         InjStubTaskDesc;
static void          * InjStubTaskStack_p;

#define MAX_CODEC_NAME_SIZE 5
#define VC1_SLICE_START_CODE 0x0B
#define VC1_MAX_SLICES 68
#define VC1_SLICE_USER_DATA_START_CODE 0x1B
#define VC1_FIELD_LEVEL_USER_DATA_START_CODE 0x1C
#define VC1_FRAME_LEVEL_USER_DATA_START_CODE 0x1D

typedef struct
    {
        U8	CurrentPictureNumSlicesProcessed;
        U32* SlicesStructureAddress;
        U32* NextDoubleWordSliceStartCodeEntryPointer;
    } CurrentPictureSlicesInfo_t;
typedef struct
    {
        U8	CurrentPictureNumSlicesUserDataProcessed;
        U32* SlicesUserDataStructureAddress;
        U32* NextDoubleWordSliceStartUserDataEntryPointer;
    } CurrentPictureSlicesUserDataInfo_t;
enum {
        SIMPLE_PROFILE=0,
        MAIN_PROFILE=4,
        ADVANCED_PROFILE=12
    };

static VC1Profile=ADVANCED_PROFILE;
static CurrentPictureSlicesInfo_t CurrentPictureSlices = {0,NULL,NULL};
static CurrentPictureSlicesUserDataInfo_t CurrentPictureSlicesUserData = {0,NULL,NULL};

/* Static functions protos -------------------------------------------------- */

static void InjecterTask(void);
static void DMATransferOneSlice(U32 InjectNum);

/* Macro -------------------------------------------------------------------- */

#define TEST_INJECT(x)                                  \
    if((x >= MAX_INJECTERS)||(DMAInjecters[x] == NULL)) \
    {                                                   \
        return; /* error in params */                   \
    }


/* Functions ---------------------------------------------------------------- */

/*
 * StubInject_SetStreamSize()
 */
#ifdef ST_OSLINUX
ST_ErrorCode_t StubInject_SetStreamSize(U32 Size)
{
    /* Filling the Stream size */
    StubInject_StreamSize = Size;
    return(ST_NO_ERROR);
}
#endif

/*
 * StubInject_GetStreamSize()
 */
#ifdef ST_OSLINUX
ST_ErrorCode_t StubInject_GetStreamSize(U32 * Size_p)
{
    *Size_p = StubInject_StreamSize;
    return(ST_NO_ERROR);
}
#endif

ST_ErrorCode_t StubInject_GetCodecName(BOOL * IsVC1Codec_p)
{
#ifdef ST_OSLINUX
    * IsVC1Codec_p = FALSE; /*VC1 under linux: to be done*/
#else
    char            CodecName[MAX_CODEC_NAME_SIZE];

    STTST_EvaluateString("VID1_CODEC",CodecName, MAX_CODEC_NAME_SIZE);
    if (strcmp(CodecName, "VC1")==0)
    {
      * IsVC1Codec_p = TRUE;
    }
    else
    {
      * IsVC1Codec_p = FALSE;
    }
#endif
return(ST_NO_ERROR);
}


/*
 * StubInject_SetBBInfo()
 */
#ifdef ST_OSLINUX
ST_ErrorCode_t StubInject_SetBBInfo(void * BaseAddress_p, U32 Size)
{
    StubInject_BaseAddress_p = BaseAddress_p;
    StubInject_Size = Size;
    return(ST_NO_ERROR);
}
#endif

/*
 * StubInject_GetBBInfo()
 */
#ifdef ST_OSLINUX
ST_ErrorCode_t StubInject_GetBBInfo(void ** BaseAddress_pp, U32 * Size_p)
{
    *BaseAddress_pp = StubInject_BaseAddress_p;
    *Size_p = StubInject_Size;

    return(ST_NO_ERROR);
}
#endif



/*******************************************************************************
Name        : VIDINJ_EnterCriticalSection
Description : stub
Parameters  : Video injecter handle, Injecter Number
Assumptions : Function only called from decode task
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDINJ_EnterCriticalSection(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
 /*   nothing to do because DMA (FDMA) are not used when inject_stub is compiled */
     UNUSED_PARAMETER(InjecterHandle);
     UNUSED_PARAMETER(InjectNum);
} /* End of VIDINJ_EnterCriticalSection() function */

/*******************************************************************************
Name        : VIDINJ_LeaveCriticalSection
Description : stub
Parameters  : Video injecter handle, Injecter Number
Assumptions : Function only called from decode task
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDINJ_LeaveCriticalSection(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
 /*   nothing to do because DMA (FDMA) are not used when inject_stub is compiled */
     UNUSED_PARAMETER(InjecterHandle);
     UNUSED_PARAMETER(InjectNum);
} /* End of VIDINJ_LeaveCriticalSection() function */

/*******************************************************************************
Name        : VIDINJ_TransferFlush
Description : Stub
Parameters  : Inject Number
Assumptions : The injection process should be stopped by upper layer.
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferFlush(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const BOOL DiscontinuityStartcode, U8 *  const Pattern, const U8 Size)
{
 /*   STTBX_Print(("\n\nWARNING !!!!: running %s()\n\n", __FUNCTION__));*/
     UNUSED_PARAMETER(InjecterHandle);
     UNUSED_PARAMETER(InjectNum);
     UNUSED_PARAMETER(DiscontinuityStartcode);
     UNUSED_PARAMETER(Pattern);
     UNUSED_PARAMETER(Size);
} /* End of VIDINJ_TransferFlush() function. */

/*******************************************************************************
Name        : VIDINJ_TransferReset
Description : stub
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferReset(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
/*    STTBX_Print(("\n\nWARNING !!!!: running %s()\n\n", __FUNCTION__));*/
 /*   nothing to do because DMA (FDMA) are not used when inject_stub is compiled */
     UNUSED_PARAMETER(InjecterHandle);
     UNUSED_PARAMETER(InjectNum);
} /* end of VIDINJ_TransferReset() function */

/*******************************************************************************
Name        : VIDINJ_GetRemainingDataToInject
Description : stub
Parameters  : Video injecter handle, Injecter Number
Assumptions :
Limitations :
Returns     : The number of bytes in the input buffer
*******************************************************************************/
void VIDINJ_GetRemainingDataToInject(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, U32* RemainingData)
{
    /*  STTBX_Print(("\n\nWARNING !!!!: running VIDINJ_GetRemainingDataToInject()\n\n"));*/
    /*   nothing to do because DMA (FDMA) are not used when inject_stub is compiled */
    UNUSED_PARAMETER(InjecterHandle);
    UNUSED_PARAMETER(InjectNum);
    UNUSED_PARAMETER(RemainingData);

} /* End of VIDINJ_GetRemainingDataToInject() function */

/*******************************************************************************
Name        : VIDINJ_InjectStartCode
Description : Stub
Parameters  : Inject Number
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_InjectStartCode(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const U32 SCValue, const void* SCAdd)
{
    STTBX_Print(("\n\nWARNING !!!!: running %s()\n\n", __FUNCTION__));
    /*   nothing to do because DMA (FDMA) are not used when inject_stub is compiled */
    UNUSED_PARAMETER(InjecterHandle);
    UNUSED_PARAMETER(InjectNum);
    UNUSED_PARAMETER(SCValue);
    UNUSED_PARAMETER(SCAdd);
} /* End of VIDINJ_InjectStartCode() function. */

/*******************************************************************************
Name        : VIDINJ_TransferGetInject
Description : Equivalent to Init-Open a connexion to an injecter
Parameters  :
Assumptions :
Limitations :
Returns     : return a handle = index in the DMAInjecters array
*******************************************************************************/
U32  VIDINJ_TransferGetInject(const VIDINJ_Handle_t InjecterHandle, VIDINJ_GetInjectParams_t * const Params_p)
{
    U32                                     i;

    /* First init */
    /*------------*/
    if(!DMATransferInitDone)
    {
        /* very first init: init the common variables */
        /* shared by all instances                    */
        DMATransferInitDone = TRUE;
        for(i=0; i<MAX_INJECTERS; i++)
        {
            DMAInjecters[i] = NULL;
        }

        if (STOS_TaskCreate((void (*) (void*)) InjecterTask,
                                  (void *) NULL,
                                  NULL,
                                  STVID_TASK_STACK_SIZE_INJECTERS,
                                  &InjStubTaskStack_p,
                                  NULL,
                                  &InjStubTask_p,
                                  &InjStubTaskDesc,
                                  STVID_TASK_PRIORITY_INJECTERS,
                                  "STVID_Injecter",
                                  task_flags_no_min_stack_size) != ST_NO_ERROR)
        {
            STTBX_Print(("Inject_stub: error !!!!\n\n"));
            return(0);
        }
    }

    /* Search for a free injecter */
    /*----------------------------*/
    i = 0;
    while((DMAInjecters[i] != NULL) && (i < MAX_INJECTERS))
    {
        i++;
    }
    if(i >= MAX_INJECTERS)
    {
        return(BAD_INJECT_NUM);
    }
    /* else : continue with i=index of an available injecter */
    DMAInjecters[i] = STOS_MemoryAllocate(Params_p->CPUPartition,
                                      sizeof(InjContext_t));
    /* Software init */
    /*---------------*/
    DMAInjecters[i]->UpdateHalDecodeFct       = Params_p->TransferDoneFct;
    DMAInjecters[i]->HalDecodeIdent           = Params_p->UserIdent;
    DMAInjecters[i]->CPUPartition             = Params_p->CPUPartition;
    DMAInjecters[i]->TransferState              = STOPPED;

    DMAInjecters[i]->InputBase_p              = 0;
    DMAInjecters[i]->InputTop_p               = 0;
    DMAInjecters[i]->InputWrite_p             = 0;
    DMAInjecters[i]->InputRead_p              = 0;

    DMAInjecters[i]->ES_Base_p                = 0;
    DMAInjecters[i]->ES_Top_p                 = 0;
    DMAInjecters[i]->ES_Write_p               = 0;
    DMAInjecters[i]->ES_Read_p                = 0;

    DMAInjecters[i]->SCListBase_p             = 0;
    DMAInjecters[i]->SCListTop_p              = 0;
    DMAInjecters[i]->SCListWrite_p            = 0;
    DMAInjecters[i]->SCListRead_p             = 0;
    DMAInjecters[i]->SCListLoop_p             = 0;

    DMAInjecters[i]->GetWriteAddress          = NULL;
    DMAInjecters[i]->InformReadAddress        = NULL;
    DMAInjecters[i]->ExtHandle                = NULL;

    return(i);
}

/*******************************************************************************
Name        : VIDINJ_TransferReleaseChannel
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferReleaseInject(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    TEST_INJECT(InjectNum);
	STOS_MemoryDeallocate(DMAInjecters[InjectNum]->CPUPartition, DMAInjecters[InjectNum]);
	DMAInjecters[InjectNum] = NULL;
}

/*******************************************************************************
Name        : VIDINJ_TransferStop
Description : stop the transfer process
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferStop(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum)
{
    TEST_INJECT(InjectNum);
    DMAInjecters[InjectNum]->TransferState  = STOPPED;
}

/*******************************************************************************
Name        : VIDINJ_TransferStart
Description : start the transfer process
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferStart(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const BOOL IsRealTime)
{
    TEST_INJECT(InjectNum);

    if(DMAInjecters[InjectNum]->InformReadAddress != NULL)
    {
        DMAInjecters[InjectNum]->InformReadAddress(
                                        DMAInjecters[InjectNum]->ExtHandle,
                                        DMAInjecters[InjectNum]->InputRead_p);
    }
    DMAInjecters[InjectNum]->TransferState = RUNNING;
}

BOOL VIDINJ_CheckStartCode(const U32 * CPU_ES_Start_p, const U32 * CPU_ES_End_p, U32 ** WordInES, U32 * ESByteCounter, U8 * ESByteInWordCounter, U32 * WordToRead, U32 * SliceNumber, U32 ** ElementInSCList, U32 SCListUpperLimit)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32 StartCodePosition;
    U8 NalRefIdc;
    U8 NalUnitType;
    S8 ForbiddenZeroBit;
	BOOL AddInSCList;
    U8 ByteInES;
	U32 VC1SliceAddr,VC1SliceNum;
    BOOL            IsVC1Codec;

    StubInject_GetCodecName(&IsVC1Codec);

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    /* It's a start code */
    ByteInES = (* WordToRead >> * ESByteInWordCounter) & 0xff;
    (* ESByteCounter)++;
  	* ESByteInWordCounter+=8;
  	if (* ESByteInWordCounter == 32)
  	{
  	  (* WordInES)++;
  	  * WordToRead = ** WordInES;
  	  * ESByteInWordCounter=0;
    }

    /* The start code position reported by the FDMA is the position of the start code value (so the byte after the 00 00 01 sequence) */
    StartCodePosition = (U32)CPU_ES_Start_p + (* ESByteCounter) - 1;

/*H.264 specific processing separated by MJ*/
    if(!IsVC1Codec)
    {
    ForbiddenZeroBit = (ByteInES >> 7) & 0x01;
    if (ForbiddenZeroBit != 0)
    {
        STTBX_Print(("Error: stream contains ForbiddenZeroBit != 0\n"));
    }
    NalRefIdc = (ByteInES >> 5) & 0x3;
    NalUnitType = (ByteInES & 0x1f);
    }

	else {

	if (ByteInES==VC1_SLICE_START_CODE)
	{
		/* If this is the first slice start code for the picture, allocate the buffer in the SC+PTS buffer,
		else just update the entry in the existing structure */

		/* The Slices structure is like this

			U32 0x0
			U32 NumSliceStartCodes
			U32 VC1_SLICE_START_CODE
			U32 0x0

			{
			U32* BitStreamPointerToSlice
			U32 SliceAddr
			}[VC1_MAX_SLICES]

		*/

		/* Now read the 9 bit sliceaddr from the bitstream*/

		/* First read the 8 msbits*/
		VC1SliceAddr = ((* WordToRead >> * ESByteInWordCounter) & 0xff)<<1;

		/* goto next byte*/
		(* ESByteCounter)++;
  		* ESByteInWordCounter+=8;
  		if (* ESByteInWordCounter == 32)
  		{
  		  (* WordInES)++;
  		  * WordToRead = ** WordInES;
  		  * ESByteInWordCounter=0;
		}

		/* now read the lsbit*/
		VC1SliceAddr |= (((* WordToRead >> * ESByteInWordCounter) & 0x80)>>7);

		/* goto next byte*/
		(* ESByteCounter)++;
  		* ESByteInWordCounter+=8;
  		if (* ESByteInWordCounter == 32)
  		{
  		  (* WordInES)++;
  		  * WordToRead = ** WordInES;
  		  * ESByteInWordCounter=0;
		}





	}


  }

	AddInSCList = TRUE;

/*H.264 specific processing separated by MJ*/
    if(!IsVC1Codec)
    {
	if ((NalUnitType == NALU_TYPE_SLICE) ||(NalUnitType == NALU_TYPE_IDR))
	{
		/* Check whether this is the 1st slice of a picture */
        ByteInES = (* WordToRead >> * ESByteInWordCounter) & 0xff;
        (* ESByteCounter)++;
      	* ESByteInWordCounter+=8;
      	if (* ESByteInWordCounter == 32)
      	{
      	  (* WordInES)++;
      	  * WordToRead = ** WordInES;
      	  * ESByteInWordCounter=0;
        }
		if ((ByteInES & 0x80) == 0)
        {
			/* First bit is not zero: it means the first_mb_in_slice is != 0 */
			/* Ignore this slice as it is not the 1st slice of a picture */
			AddInSCList = FALSE;
			(* SliceNumber)++;
		}
		else
		{
			/* This is the 1st slice of the picture */
			* SliceNumber = 1; /* reset slice number counter */
		}
	}
  }

	/* Add this NAL is the start code list */
	if (* ElementInSCList < (U32 *)SCListUpperLimit)
	{
		if (AddInSCList)
		{
			*(* ElementInSCList)++ = 0; /* 1st word set to 0 */
			*(* ElementInSCList)++ = (U32) STAVMEM_CPUToDevice(StartCodePosition, &VirtualMapping); /* 2nd word = location in memory (device address) */
/*H.264 specific processing separated by MJ*/
    if(IsVC1Codec)
    {
		if(ByteInES==VC1_SLICE_START_CODE)
		{
			*(* ElementInSCList)++ = ByteInES | (VC1SliceAddr << 8); /* 3rd word */
		}
		else
		{
			*(* ElementInSCList)++ = ByteInES; /* 3rd word */
		}

    } else {
			*(* ElementInSCList)++ = ((NalRefIdc << 5) & 0x00000060) | (NalUnitType & 0x0000001F); /* 3rd word */
    }
			*(* ElementInSCList)++ = 0; /* 4th word = 0 */

            STTBX_Print(("SC found at 0x%.8x in 0x%.8x\n", (U32)StartCodePosition, (U32)*ElementInSCList));
		}
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}

}

BOOL VIDINJ_CreateStartCodeList(const U32 InjectNum, const U32 * CPU_ES_Start_p, const U32 * CPU_ES_End_p, U32 * ElementInSCList, U32 SCListUpperLimit)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32 ConsecutiveZeros = 0;
	U32 SliceNumber = 0;
	BOOL SCListIsFitting = TRUE;
    U32 StartCodePosition;
    U8 NalRefIdc;
    U8 NalUnitType;
	U32 * WordInES;
    U32 WordToRead;
    U32 ESByteToRead;
    U32 ESByteCounter;
    U8  ESByteInWordCounter;
    U8 ByteInES;

  U32 NextFrameHeaderLocation=0; /* In same units and offset as ESByteCounter*/
  U32 VC1FrameSize;
  U8 VC1Key;

  BOOL            IsVC1Codec;

  /* Default to Advanced Profile */
  VC1Profile = ADVANCED_PROFILE;

    StubInject_GetCodecName(&IsVC1Codec);

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    ESByteInWordCounter = 0;

    ESByteCounter = 0;
    ESByteToRead = sizeof(U32) * (CPU_ES_End_p - CPU_ES_Start_p);

	WordInES = (U32 *)CPU_ES_Start_p;
	WordToRead = * WordInES;
	/*Initially set it beyond the stream so that if headers are not present
	then this location is never scanned*/
	NextFrameHeaderLocation=ESByteToRead+1;


    if(IsVC1Codec)
    {
/* File header checking*/
	if (ESByteToRead>9*4) /* 9 words in the VC1 header*/
	{
		/* Detect if VC1 header*/
		if ((WordToRead & 0xFF000000) == 0xC5000000)
		{
			WordToRead=*(WordInES+1);
			if (WordToRead==0x4) /* Fixed value in VC1 header*/
			{
				WordToRead=*(WordInES+5);
				if (WordToRead==0x0C) /* Fixed value in VC1 header*/
				{
					/* Extremely high probability that this is a file header*/
					WordToRead=*(WordInES+2);/*Get to Struct C*/
					VC1Profile=WordToRead & 0xF0; /* Stuct C is coded Big Endian*/

					/*Skip the file header */
					WordInES+=9; /* 9 words in the VC1 header*/
					ESByteCounter+=9*4;
					/* Insert a File Header Code 0x80 - non standard*/
					if (ElementInSCList < (U32 *)SCListUpperLimit)
					{
						* ElementInSCList++ = 0; /* 1st word set to 0 */
						* ElementInSCList++ = (U32) STAVMEM_CPUToDevice(CPU_ES_Start_p, &VirtualMapping); /* 2nd word = location in memory (device address) */

						* ElementInSCList++ = 0x80;/* MJ file header code decided for VC1*/ /* 3rd word */

						* ElementInSCList++ = 0; /* 4th word = 0 */
					}
					else
					{
						SCListIsFitting = FALSE;
					}

					/* This header is guaranteed to be word aligned since it is immediately following a file header
					but the subsequent frame headers are not*/

					/* Now skip the frame layer data structure header and set the expectation for the next*/
					NextFrameHeaderLocation=ESByteCounter+1;
					WordToRead=*WordInES;
					VC1FrameSize=WordToRead&0xffffff; /* Mask off top 8 bits, they are key,reserved*/

					VC1Key=(WordToRead & 0x80000000)>>31;


					NextFrameHeaderLocation+=(VC1FrameSize+8); /* 8 bytes of current header to skip*/

					/* Insert a File Header Code 0x81 if VC1Key==0 else 82 - non standard*/
					if (ElementInSCList < (U32 *)SCListUpperLimit)
					{
						* ElementInSCList++ = 0; /* 1st word set to 0 */
						* ElementInSCList++ = (U32) STAVMEM_CPUToDevice((U32)WordInES, &VirtualMapping); /* 2nd word = location in memory (device address) */

						* ElementInSCList++ = 0x81+VC1Key;/* MJ file header codes decided for VC1*/ /* 3rd word */

						* ElementInSCList++ = 0; /* 4th word = 0 */
					}
					else
					{
						SCListIsFitting = FALSE;
					}

					STTBX_Print(("Frame Size %d\n",VC1FrameSize ));

					WordInES+=2; /* 2 words in the VC1 frame layer data structure header*/
					ESByteCounter+=2*4;
				}
			}

		}


	}
  }

  WordToRead = * WordInES;

    while (ESByteCounter < ESByteToRead)
    {
        ByteInES = (WordToRead >> ESByteInWordCounter) & 0xff;
        ESByteCounter++;
  		ESByteInWordCounter+=8;
  		if (ESByteInWordCounter == 32)
  		{
  		  WordInES++;
  		  WordToRead = * WordInES;
  		  ESByteInWordCounter=0;
	    }

    if(IsVC1Codec)
    {
		/* State machine for the header check*/
		if (ESByteCounter>=NextFrameHeaderLocation) /* This will work only for linear ES buffer!!!*/
		{
			/* Must be a header byte*/
			switch(ESByteCounter-NextFrameHeaderLocation)
			{
			case 0:
					VC1FrameSize=ByteInES;

					break;
			case 1:
					VC1FrameSize=(ByteInES<<8) | VC1FrameSize;
					break;
			case 2:
					VC1FrameSize=(ByteInES<<16) | VC1FrameSize;
					break;
			case 4:
			case 5:
			case 6:
					break;

			case 7:
					/* Insert a File Header Code 0x81 if VC1Key==0 else 82 - non standard*/
					if (ElementInSCList < (U32 *)SCListUpperLimit)
					{
						* ElementInSCList++ = 0; /* 1st word set to 0 */
						* ElementInSCList++ = (U32) STAVMEM_CPUToDevice((U32)CPU_ES_Start_p + ESByteCounter-8, &VirtualMapping); /* 2nd word = location in memory (device address) */

						* ElementInSCList++ = 0x81+VC1Key;/* MJ file header codes decided for VC1*/ /* 3rd word */

						* ElementInSCList++ = 0; /* 4th word = 0 */
					}
					else
					{
						SCListIsFitting = FALSE;
					}
					STTBX_Print(("Frame Size %d\n",VC1FrameSize ));

					/* Last byte of the header, set the Next FrameHeaderLocation*/
					NextFrameHeaderLocation+=(VC1FrameSize+8); /* 8 bytes of current header to skip*/
					ConsecutiveZeros = 0; /* Start code search needs to start from this point on */
					break;
			case 3:
					VC1Key=(ByteInES&0x80)>>7; /* Get the random entry KEY value*/
					break;

			}

			continue; /* i.e.do not do a start code search on the header bytes */

		}

		if (VC1Profile!=ADVANCED_PROFILE)
		{
			/* Do not search for start codes in SIMPLE and MAIN profile so skip even framedata bytes*/
			continue;
		}

  }



	    if (ByteInES == 0)
	    {
	        ConsecutiveZeros++;
	    }
	    else
	    {
            if ((ConsecutiveZeros >= 2) && (ByteInES == 1))
            {
  	        	SCListIsFitting = VIDINJ_CheckStartCode(CPU_ES_Start_p, CPU_ES_End_p, &WordInES, &ESByteCounter, &ESByteInWordCounter, &WordToRead, &SliceNumber, &ElementInSCList, SCListUpperLimit);
            }
	        ConsecutiveZeros = 0;
	    }
    }

    /* End of stream has been reached:
       - insert a EOStream dummy NAL so that parser is able to process last picture
       - output the ScPtsBuffer
    */
	StartCodePosition = (U32)CPU_ES_Start_p + ESByteCounter  + 5; /* 5 byte beyond end of stream because the parser will get the
										    * lenght of the last NAL from that point - 4 bytes
										    * (Start Code prefix + Start Code = 4 bytes)
										    */
	NalRefIdc = 0;
	NalUnitType = NALU_TYPE_EOSTREAM;

	if (ElementInSCList < (U32 *)SCListUpperLimit)
	{
		* ElementInSCList++ = 0; /* 1st word set to 0 */
		* ElementInSCList++ = (U32) STAVMEM_CPUToDevice(StartCodePosition, &VirtualMapping); /* 2nd word = location in memory (device address) */
    if(IsVC1Codec)
    {
		* ElementInSCList++ = 0x0A;/* MJ Sequence end code for VC1*/ /* 3rd word */
  } else {
		* ElementInSCList++ = ((NalRefIdc << 5) & 0x00000060) | (NalUnitType & 0x0000001F); /* 3rd word */
    }
		* ElementInSCList++ = 0; /* 4th word = 0 */
	}
	else
	{
		SCListIsFitting = FALSE;
	}
	DMAInjecters[InjectNum]->SCListLoop_p = STAVMEM_CPUToDevice(ElementInSCList, &VirtualMapping);
    DMAInjecters[InjectNum]->SCListWrite_p= (void*) (
    (U32)DMAInjecters[InjectNum]->SCListLoop_p + sizeof(STFDMA_SCEntry_t)); /* One entry after loop */

	return(SCListIsFitting);
}

/*******************************************************************************
Name        : VIDINJ_TransferLimits
Description : set the transfer buffers
Parameters  : InjectNum, buf input, bufs output
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferLimits(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                            void * const ES_Start_p,    void * const ES_Stop_p,
                            void * const SCListStart_p, void * const SCListStop_p,
                            void * const InputStart_p,  void * const InputStop_p)
{
#if !defined ST_OSLINUX
    FILE    * FileDescriptor;
    char FileName[30];
#endif
    U32 FileSize;
   	U32 SCLoop;

    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
	U32 * CPU_SCListStart_p; /* "CPU" address: start code list buffer start */
	U32 * CPU_ES_Start_p; /* "CPU" address: bit buffer start */
	U32 * CPU_ES_End_p; /* "CPU" address: bit buffer end: end of stream, not end of allocated bit buffer */

	/*Temp var by MJ for printing*/
	U32* PrintElementInSCList=0;
	U32 VC1SliceNum,VC1NumSliceStartCodesPresent;
	BOOL SCListIsFitting = TRUE;

  BOOL            IsVC1Codec;

    StubInject_GetCodecName(&IsVC1Codec);


    TEST_INJECT(InjectNum);

    DMAInjecters[InjectNum]->InputBase_p      = InputStart_p;
    DMAInjecters[InjectNum]->InputTop_p       = InputStop_p;
    DMAInjecters[InjectNum]->InputWrite_p     = InputStart_p;
    DMAInjecters[InjectNum]->InputRead_p      = InputStart_p;

    DMAInjecters[InjectNum]->ES_Base_p        = ES_Start_p;
    DMAInjecters[InjectNum]->ES_Top_p         = ES_Stop_p;
    DMAInjecters[InjectNum]->ES_Write_p       = ES_Start_p;
    DMAInjecters[InjectNum]->ES_Read_p        = ES_Start_p;

    DMAInjecters[InjectNum]->SCListBase_p     = SCListStart_p;
    DMAInjecters[InjectNum]->SCListTop_p      = SCListStop_p;
    DMAInjecters[InjectNum]->SCListLoop_p     = SCListStart_p;
    DMAInjecters[InjectNum]->SCListWrite_p    = SCListStart_p;
    DMAInjecters[InjectNum]->SCListRead_p     = SCListStart_p;
    if (InjectNum < MAX_INJECTERS)
    {
        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
        CPU_ES_Start_p = STAVMEM_DeviceToCPU(ES_Start_p, &VirtualMapping);

        #ifdef ST_OSLINUX
                StubInject_GetStreamSize(&FileSize);
                STTBX_Print(("Building start code list ..."));
        #else
        	    STTST_EvaluateInteger("VID_STREAMSIZE", (S32 *)&FileSize, 0);
                STTBX_Print(("Building start code list ..."));
        		fflush(stdout);
        #endif

		CPU_ES_End_p = (U32*)((U32) CPU_ES_Start_p + FileSize);
		DMAInjecters[InjectNum]->ES_Write_p       = (void*)((U32) ES_Start_p
                                                + FileSize);

		CPU_SCListStart_p = STAVMEM_DeviceToCPU(SCListStart_p, &VirtualMapping);
		SCListIsFitting = VIDINJ_CreateStartCodeList(InjectNum, CPU_ES_Start_p, CPU_ES_End_p, CPU_SCListStart_p, (U32)CPU_SCListStart_p + SC_LIST_IN_ENTRY * sizeof(STFDMA_SCEntry_t));
		if (!SCListIsFitting)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"SC List too large too fit in memory: expand allocated size"));
			while(1);
		}
        STTBX_Print(("DONE \r\n"));

    if(IsVC1Codec)
    {
		PrintElementInSCList=CPU_SCListStart_p;

		SCLoop=1;
		while(*(PrintElementInSCList+2)!=0x0A) /*We added a sequence end code, so see if that has been reached*/
		{
			STTBX_Print(("Element %d SC %X Address %X\n",SCLoop++,*(PrintElementInSCList+2),*(PrintElementInSCList+1) ));

			PrintElementInSCList+=4;

		}
   }



    }
    else
    {
#ifdef ST_OSLINUX
        STTBX_Print(("VIDINJ_TransferLimits(): ERROR !!!! \n"));
#else
        sprintf(FileName,"sc-cfield.dat");
        FileDescriptor = fopen(FileName, "rb" );
        FileSize = fread(SCListStart_p,1, 5000,FileDescriptor);
        DMAInjecters[InjectNum]->SCListLoop_p = (void*)((U32)SCListStart_p
                                                + FileSize);
        DMAInjecters[InjectNum]->SCListWrite_p= (void*)((U32)SCListStart_p
                                                + FileSize+10);
        fclose( FileDescriptor );

        sprintf(FileName,"cField.m2v");
        FileDescriptor = fopen(FileName, "rb" );
        FileSize = 370*1024;
        STTBX_Print(("Loading stream \r\n"));
        fread(ES_Start_p, 1,  FileSize,FileDescriptor);
        STTBX_Print(("DONE \r\n"));
        DMAInjecters[InjectNum]->ES_Write_p       = (void*)((U32) ES_Start_p
                                                + FileSize);
        fclose( FileDescriptor );
#endif
    }
}

/*******************************************************************************
Name        : VIDINJ_SetSCRanges
Description : Sets the startcodes that the FDMA must detect
Parameters  : injecter handle, number of ranges, startcode ranges
Assumptions : FDMA context allocated
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_SetSCRanges(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, const U32 NbParserRanges, VIDINJ_ParserRanges_t ParserRanges[])
{
    /*   nothing to do because DMA (FDMA) are not used when inject_stub is compiled */
    UNUSED_PARAMETER(InjecterHandle);
    UNUSED_PARAMETER(InjectNum);
    UNUSED_PARAMETER(NbParserRanges);
    UNUSED_PARAMETER(ParserRanges);
}

/*******************************************************************************
Name        : VIDINJ_Transfer_SetDataInputInterface
Description : Register the functions to be called to share info with injection
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDINJ_Transfer_SetDataInputInterface(const VIDINJ_Handle_t InjecterHandle,
        const U32 InjectNum,
        ST_ErrorCode_t (*GetWriteAddress)
                                 (void * const Handle, void ** const Address_p),
        void (*InformReadAddress)(void * const Handle, void * const Address),
        void * const Handle )
{
    if((InjectNum >= MAX_INJECTERS)||(DMAInjecters[InjectNum] == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER); /* error in params */
    }

    if((GetWriteAddress == NULL)||(InformReadAddress == NULL)||(Handle == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    DMAInjecters[InjectNum]->ExtHandle         = Handle;
    DMAInjecters[InjectNum]->GetWriteAddress   = GetWriteAddress;
    DMAInjecters[InjectNum]->InformReadAddress = InformReadAddress;

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : VIDINJ_TransferSetESRead
Description : video calls this function to update the read pointer in ES buff
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferSetESRead(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, void * const Read)
{
    TEST_INJECT(InjectNum);

    DMAInjecters[InjectNum]->ES_Read_p = Read;
}

/*******************************************************************************
Name        : VIDINJ_TransferSetSCListRead
Description : video calls this function to update the read pointer in SC List
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDINJ_TransferSetSCListRead(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum, void * const Read)
{
    TEST_INJECT(InjectNum);

    DMAInjecters[InjectNum]->SCListRead_p = Read;
}

/*******************************************************************************
Name        : VIDINJ_Init
Description :
Parameters  :
Assumptions : To be called each 'tbd' ms
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDINJ_Init(const VIDINJ_InitParams_t * const InitParams_p, VIDINJ_Handle_t * const InjecterHandle_p)
{
   return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : VIDINJ_Term
Description :
Parameters  :
Assumptions : To be called each 'tbd' ms
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDINJ_Term(const VIDINJ_Handle_t InjecterHandle)
{
    /*   nothing to do because DMA (FDMA) are not used when inject_stub is compiled */
    UNUSED_PARAMETER(InjecterHandle);
   return (ST_NO_ERROR);
}


/* Static functions */
/*------------------*/
/*******************************************************************************
Name        : InjecterTask
Description : Timer echeance
Parameters  :
Assumptions :
Limitations : The fct is a task body
Returns     :
*******************************************************************************/
static void InjecterTask(void)
{
    U32     InjectNum;
    osclock_t t1;
    osclock_t t_next;
    osclock_t t_to_next;

    STOS_TaskEnter(NULL);

    /* calculate the periode for transfers (constant) */
#ifdef ST_OSLINUX
    t_to_next = ST_GetClocksPerSecond();
#else
    t_to_next = ST_GetClocksPerSecond() * MS_TO_NEXT_TRANSFER / 1000;
    t_to_next = 20000; /* Approx 1 second, real time */
#endif
    while(1)
    {
        t1 = time_now(); /* take the time before the cycle of injection */

        for(InjectNum = 0; InjectNum < MAX_INJECTERS; InjectNum ++)
        {
            if((DMAInjecters[InjectNum]!= NULL)
             &&(DMAInjecters[InjectNum]->TransferState))
            {
                DMATransferOneSlice(InjectNum);
            }
        }
        /* time:  xxxxxxxyyyyy............xxxxxxyyy  (x,y :transfers) */
        /*        ^                       ^          (.   :delay    ) */
        /*        |                       |                           */
        /*        t1                    t_next                        */
        /*        |<-----t_to_next------->|                           */

        t_next = time_plus(t1, t_to_next);
        STOS_TaskDelayUntil(t_next);
    }/* infinite loop */

    STOS_TaskExit(NULL);
}

/*******************************************************************************
Name        : DMATransferOneSlice
Description :
Parameters  :
Assumptions : To be called each 'tbd' ms
Limitations :
Returns     :
*******************************************************************************/
static void DMATransferOneSlice(U32 InjectNum)
{
    DMAInjecters[InjectNum]->UpdateHalDecodeFct(
                                        DMAInjecters[InjectNum]->HalDecodeIdent,
                                        DMAInjecters[InjectNum]->ES_Write_p,
                                        DMAInjecters[InjectNum]->SCListWrite_p,
                                        DMAInjecters[InjectNum]->SCListLoop_p);


}

/*******************************************************************************
Name        : VIDINJ_ReallocateFDMANodes
Description : Reallocates injecter's contexts Node_p and NodeNext_p in partition
              passed as parameter. Used for secured chip where fdma nodes must
              be allocated in a non-secured partition.
Parameters  : InjecterHandle, DMA injecter number, setup params
Assumptions : To be called after STVID_Init.
              Protected by VIDINJ_EnterCriticalSection.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t VIDINJ_ReallocateFDMANodes(const VIDINJ_Handle_t InjecterHandle, const U32 InjecterIndex, const STVID_SetupParams_t * const SetupParams_p)
{
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    STAVMEM_BlockParams_t                   BlockParams;
    STAVMEM_AllocBlockParams_t	            AllocBlockParams;
    STAVMEM_FreeBlockParams_t               FreeParams;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    void *                                  VirtualAddress;
    InjContext_t *                          InjContext_p;

/* empty function as in inject_stub mode, the FDMA is not used */
    return(ErrorCode);

} /* End of VIDINJ_ReallocateFDMANodes() function */

#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t VIDINJ_GetStatistics(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                                STVID_Statistics_t * const Statistics_p)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Statistics_p->InjectFdmaTransfers = VIDINJ_Data_p->DMAInjecters[InjectNum]->InjectFdmaTransfers;
    Statistics_p->InjectDataInputReadPointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p;
    Statistics_p->InjectDataInputWritePointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;

    return(ST_NO_ERROR);
} /* end of VIDINJ_GetStatistics() */

ST_ErrorCode_t VIDINJ_ResetStatistics(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                                const STVID_Statistics_t * const Statistics_p)
{
    VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (Statistics_p->InjectFdmaTransfers != 0)
    {
        VIDINJ_Data_p->DMAInjecters[InjectNum]->InjectFdmaTransfers = 0;
    }

    return(ST_NO_ERROR);
} /* end of VIDINJ_ResetStatistics() */
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
ST_ErrorCode_t VIDINJ_GetStatus(const VIDINJ_Handle_t InjecterHandle, const U32 InjectNum,
                                STVID_Status_t * const Status_p)
{
	VIDINJ_Data_t * VIDINJ_Data_p = (VIDINJ_Data_t *) InjecterHandle;
	InjContext_t * InjContext_p = VIDINJ_Data_p->DMAInjecters[InjectNum];

    if (Status_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

	/* ES, PES, SC Buffers */
	Status_p->BitBufferAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p;
    Status_p->BitBufferSize = (((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Top_p) - (U32)(VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Base_p));
    Status_p->DataInputBufferAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p;
    Status_p->DataInputBufferSize = (((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputTop_p) - ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->InputBase_p));
    Status_p->SCListBufferAddress_p = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p;
    Status_p->SCListBufferSize = (((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListTop_p) - ((U32)VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListBase_p));

	/* FDMA Nodes related */
	Status_p->FdmaNodesSize = 0;

	if (VIDINJ_Data_p->DMAInjecters[InjectNum]->Node_p != NULL)
	{
		Status_p->FdmaNodesSize += sizeof(STFDMA_GenericNode_t);
	}

	if (VIDINJ_Data_p->DMAInjecters[InjectNum]->NodeNext_p != NULL)
	{
		Status_p->FdmaNodesSize += sizeof(STFDMA_GenericNode_t);
	}

	/* ES, PES, SC Pointers */
	Status_p->BitBufferReadPointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Read_p;
	Status_p->BitBufferWritePointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->ES_Write_p;
	Status_p->DataInputBufferReadPointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputRead_p;
	Status_p->DataInputBufferWritePointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->InputWrite_p;
	Status_p->SCListBufferReadPointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListRead_p;
	Status_p->SCListBufferWritePointer = VIDINJ_Data_p->DMAInjecters[InjectNum]->SCListWrite_p;

	/* InjectFlushBufferSize */
	if (InjContext_p->FlushBuffer_p != NULL)
	{
		Status_p->InjectFlushBufferSize = FLUSH_DATA_SIZE;
	}
	else
	{
		Status_p->InjectFlushBufferSize = 0;
	}

    return(ST_NO_ERROR);
} /* end of VIDINJ_GetStatus() */
#endif /* STVID_DEBUG_GET_STATUS */
/* End of inject_stub.c */
