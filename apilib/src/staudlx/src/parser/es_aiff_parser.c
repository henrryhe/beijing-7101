/************************************************************************
COPYRIGHT STMicroelectronics (C) 2007
Source file name : es_aiff_parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_AIFF
//#define STTBX_PRINT
#include "es_aiff_parser.h"
#include "aud_utils.h"
#include "acc_multicom_toolbox.h"
#include "stos.h"
#include "aud_pes_es_parser.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
#include "aud_evt.h"


#define OPBLK_SIZE_24BIT ((MAXBUFFSIZE*4)/3) // Used for 24 bit files

/************************************************************************************
Name         : ES_AIFF_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
             ES_AIFF_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/


ST_ErrorCode_t ES_AIFF_Parser_Init(ComParserInit_t * Init_p, ParserHandle_t * const handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_AIFF_ParserLocalParams_t *AIFFParserParams_p;
    STAUD_BufferParams_t            MemoryBuffer;

    AIFFParserParams_p=memory_allocate(Init_p->Memory_Partition,sizeof(ES_AIFF_ParserLocalParams_t));
    if(AIFFParserParams_p == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(AIFFParserParams_p,0,sizeof(ES_AIFF_ParserLocalParams_t));
    AIFFParserParams_p->AppData_p=(STAud_LpcmStreamParams_t *)memory_allocate(Init_p->Memory_Partition,sizeof(STAud_LpcmStreamParams_t));
    if(AIFFParserParams_p->AppData_p == NULL)
    {
        /*free allocated memory*/
        memory_deallocate(Init_p->Memory_Partition, AIFFParserParams_p);
        return ST_ERROR_NO_MEMORY;
    }
    memset(AIFFParserParams_p->AppData_p,0,sizeof(STAud_LpcmStreamParams_t));

    AIFFParserParams_p->DummyBufferSize = MAXBUFFSIZE;
    AIFFParserParams_p->Sample_Rate_Counter = 0;
    AIFFParserParams_p->DataCopied=0;

    Error|=MemPool_GetBufferParams(Init_p->OpBufHandle, &MemoryBuffer);
    if(Error!=ST_NO_ERROR)
    {
        memory_deallocate(Init_p->Memory_Partition, AIFFParserParams_p->AppData_p);
        memory_deallocate(Init_p->Memory_Partition, AIFFParserParams_p);
        return Error;
    }
    AIFFParserParams_p->ESBufferAddress.BaseUnCachedVirtualAddress=(U32*)MemoryBuffer.BufferBaseAddr_p;
    AIFFParserParams_p->ESBufferAddress.BasePhyAddress=(U32*)STOS_VirtToPhys(MemoryBuffer.BufferBaseAddr_p);
    AIFFParserParams_p->ESBufferAddress.BaseCachedVirtualAddress=NULL;

    AIFFParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress= (U32*)Init_p->DummyBufferBaseAddress;
    AIFFParserParams_p->DummyBufferAddress.BasePhyAddress=(U32*)STOS_VirtToPhys(Init_p->DummyBufferBaseAddress);
    AIFFParserParams_p->DummyBufferAddress.BaseCachedVirtualAddress=NULL;

    AIFFParserParams_p->node_p = Init_p->NodePESToES_p;
    AIFFParserParams_p->FDMAnodeAddress.BaseUnCachedVirtualAddress=(U32*)Init_p->NodePESToES_p;
    AIFFParserParams_p->FDMAnodeAddress.BasePhyAddress = (U32*)STOS_VirtToPhys(Init_p->NodePESToES_p);

    AIFFParserParams_p->SpaceInBuffer           = MAXBUFFSIZE;

    /* Init the Local Parameters */
    AIFFParserParams_p->ParserState             = ES_AIFF_F;
    AIFFParserParams_p->FrameSize               = 0XFFFFFFFF;
    AIFFParserParams_p->SSNND_SampleSize        = 0xFFFFFFFF;
    AIFFParserParams_p->Number_Of_Channels      = 0XFFFF;
    AIFFParserParams_p->Sample_Size             = 0XFFFF;
    AIFFParserParams_p->Chunk_Size              = 0xFFFFFFFF;
    AIFFParserParams_p->NoBytesRemainingInFrame = 0XFFFF;
    AIFFParserParams_p->Frequency               = 0xFFFFFFFF;
    AIFFParserParams_p->Get_Block               = FALSE;
    AIFFParserParams_p->Put_Block               = TRUE;
    AIFFParserParams_p->OpBufHandle             = Init_p->OpBufHandle;
    /*initialize the event IDs*/
    *handle                                     =(ParserHandle_t)AIFFParserParams_p;

    return (Error);
}

/************************************************************************************
Name         : ES_AIFF_ParserGetParsingFunction()

Description  : Sets the parsing function, Frequency function and the info function

Parameters   : ParsingFunction   * ParsingFunc
               GetFreqFunction_t * GetFreqFunction
               GetInfoFunction_t * GetInfoFunction

Return Value :
    ST_NO_ERROR                     Success.

 ************************************************************************************/
ST_ErrorCode_t ES_AIFF_ParserGetParsingFunction(    ParsingFunction_t   * ParsingFunc_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p )
{

    *ParsingFunc_p      = (ParsingFunction_t)ES_AIFF_Parse_Frame;
    *GetFreqFunction_p  = (GetFreqFunction_t)ES_AIFF_Get_Freq;
    *GetInfoFunction_p  = (GetInfoFunction_t)ES_AIFF_Get_Info;

    return ST_NO_ERROR;

}

/************************************************************************************
Name         : ES_AIFF_Get_Freq()

Description  : Sets the Frequency

Parameters   : ParserHandle_t  Handle
               U32 *SamplingFrequency_p

Return Value :
    ST_NO_ERROR                     Success

 ************************************************************************************/

ST_ErrorCode_t ES_AIFF_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_AIFF_ParserLocalParams_t *AIFFParserParams_p;
    AIFFParserParams_p   = (ES_AIFF_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p = AIFFParserParams_p->SamplingRate;

    return ST_NO_ERROR;
}

/************************************************************************************
Name            :  ES_AIFF_Get_Info()

Description    :  Get StreamInfo from Parser

Parameters    : ParserHandle_t  Handle
                 STAUD_StreamInfo_t * StreamInfo

Return Value  :
    ST_NO_ERROR                     Success

 ************************************************************************************/

ST_ErrorCode_t ES_AIFF_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_AIFF_ParserLocalParams_t *AIFFParserParams_p;

    AIFFParserParams_p            =(ES_AIFF_ParserLocalParams_t *)Handle;
    StreamInfo->BitRate           = AIFFParserParams_p->SamplingRate * AIFFParserParams_p->Sample_Size ;
    StreamInfo->SamplingFrequency = AIFFParserParams_p->SamplingRate;

    return ST_NO_ERROR;
}

 /************************************************************************************
Name            :  ES_AIFF_Handle_EOF()

Description    :  Handle EOF from Parser

Parameters    : ParserHandle_t  Handle

Return Value  :
    ST_NO_ERROR                     Success

 ************************************************************************************/

ST_ErrorCode_t ES_AIFF_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_AIFF_ParserLocalParams_t *AIFFParserParams_p = (ES_AIFF_ParserLocalParams_t *)Handle;
    U16 *src;
    U16 *dst;
    U32 *src32,*dst32;
    U32 i;
    U32 size;
    i=0;
    size=0;

    if(AIFFParserParams_p->Get_Block == TRUE)
    {
        memset(AIFFParserParams_p->Current_Write_Ptr1,0x00,AIFFParserParams_p->NoBytesRemainingInFrame);

        if( (AIFFParserParams_p->Sample_Size ==24) || (AIFFParserParams_p->Sample_Size ==32))
        {
            if(AIFFParserParams_p->Sample_Size ==24)
            {
                if(AIFFParserParams_p->SpaceInBuffer < MAXBUFFSIZE)
                {
                    AIFFParserParams_p->node_p->Node.SourceAddress_p= (void *)(STOS_AddressTranslate(AIFFParserParams_p->DummyBufferAddress.BasePhyAddress, AIFFParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress, AIFFParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress ));
                    AIFFParserParams_p->node_p->Node.DestinationAddress_p= (void *)(STOS_AddressTranslate(AIFFParserParams_p->ESBufferAddress.BasePhyAddress, AIFFParserParams_p->ESBufferAddress.BaseUnCachedVirtualAddress,AIFFParserParams_p->Current_Write_Ptr1));
                    STTBX_Print(("\nEntering generic transfer"));
                    FDMAMemcpy(AIFFParserParams_p->node_p->Node.DestinationAddress_p, AIFFParserParams_p->node_p->Node.SourceAddress_p,3,MAXBUFFSIZE,3,4,&AIFFParserParams_p->FDMAnodeAddress);
                }
            }

            src     = (U16 *)AIFFParserParams_p->OpBlk_p->MemoryStart;
            dst     = (U16 *)AIFFParserParams_p->OpBlk_p->MemoryStart;
            src32   = (U32 *)AIFFParserParams_p->OpBlk_p->MemoryStart;
            dst32   = (U32 *)AIFFParserParams_p->OpBlk_p->MemoryStart;

            if(AIFFParserParams_p->Sample_Size ==32)
            {
                size = (AIFFParserParams_p->FrameSize>> 1);
            }

            if(AIFFParserParams_p->Sample_Size ==24)
            {
                size = (OPBLK_SIZE_24BIT >> 1);
            }
#ifndef ST_OSWINCE
            for ( i = 0 ; i < size; i++ )
            {
                __asm__("swap.b %1, %0" : "=r" (dst[i]): "r" (src[i]));
            }
#endif

            if(AIFFParserParams_p->Sample_Size ==32)
                size = (AIFFParserParams_p->FrameSize >> 2);

            if(AIFFParserParams_p->Sample_Size ==32)
                size = (OPBLK_SIZE_24BIT >> 2);


#ifndef ST_OSWINCE
            for ( i = 0 ; i < size; i++ )
            {
                __asm__("swap.w %1, %0" : "=r" (dst32[i]): "r" (src32[i]));
            }
#endif

            if(AIFFParserParams_p->Sample_Size ==24)
                AIFFParserParams_p->OpBlk_p->Size = OPBLK_SIZE_24BIT-((AIFFParserParams_p->SpaceInBuffer*4)/3);
        }

        if((AIFFParserParams_p->Sample_Size ==8) || (AIFFParserParams_p->Sample_Size ==16)||((AIFFParserParams_p->Sample_Size ==32)))
            AIFFParserParams_p->OpBlk_p->Size = AIFFParserParams_p->FrameSize;


        Error = MemPool_PutOpBlk(AIFFParserParams_p->OpBufHandle,(U32 *)&AIFFParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AIFF Error in Memory Put_Block \n"));
        }
        else
        {
            AIFFParserParams_p->Put_Block = TRUE;
            AIFFParserParams_p->Get_Block = FALSE;
        }
    }
    else
    {
        Error = MemPool_GetOpBlk(AIFFParserParams_p->OpBufHandle, (U32 *)&AIFFParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            return STAUD_MEMORY_GET_ERROR;
        }
        else
        {
            AIFFParserParams_p->Put_Block        = FALSE;
            AIFFParserParams_p->Get_Block        = TRUE;
            AIFFParserParams_p->OpBlk_p->Size    = 1152;
            AIFFParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
            memset((void*)AIFFParserParams_p->OpBlk_p->MemoryStart,0x00,AIFFParserParams_p->OpBlk_p->MaxSize);
            Error = MemPool_PutOpBlk(AIFFParserParams_p->OpBufHandle,(U32 *)&AIFFParserParams_p->OpBlk_p);
            if(Error != ST_NO_ERROR)
            {
                STTBX_Print((" ES_AIFF Error in Memory Put_Block \n"));
            }
            else
            {
                AIFFParserParams_p->Put_Block = TRUE;
                AIFFParserParams_p->Get_Block = FALSE;
            }
        }
    }

    return ST_NO_ERROR;
}

 /************************************************************************************
Name            :  ES_AIFF_Parser_Term()

Description    :  Terminate the Parser. Deallocate the memory allocated in init.

Parameters    : ParserHandle_t  Handle

Return Value  :
    ST_NO_ERROR                     Success

 ************************************************************************************/

ST_ErrorCode_t ES_AIFF_Parser_Term(ParserHandle_t *const handle, ST_Partition_t *CPUPartition_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    ES_AIFF_ParserLocalParams_t *AIFFParserLocalParams_p = (ES_AIFF_ParserLocalParams_t *)handle;
    if(AIFFParserLocalParams_p)
    {
        memory_deallocate(CPUPartition_p, AIFFParserLocalParams_p->AppData_p);
        memory_deallocate(CPUPartition_p, AIFFParserLocalParams_p);
    }

    return (Error);
}


/************************************************************************************
Name           : Parse_AIFF_Frame()

Description   : This is entry function for the parsing  .

Parameters   : ParserHandle_t const Handle
            void *MemoryStart    Memory Block Start Address
            U32 Size  Size of Memory Block
            U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/


ST_ErrorCode_t ES_AIFF_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used)
{
    U8 *pos;
    U32 offset;
    U8 value;
    ST_ErrorCode_t Error;
    ES_AIFF_ParserLocalParams_t *AIFFParserParams_p;

    offset=*No_Bytes_Used;
    pos=(U8 *)MemoryStart;

    //  STTBX_Print((" ES_AIFF Parser Enrty \n"));

    AIFFParserParams_p=(ES_AIFF_ParserLocalParams_t *)Handle;

    while(offset<Size)
    {

        if(AIFFParserParams_p->Put_Block == TRUE)
        {
            Error = MemPool_GetOpBlk(AIFFParserParams_p->OpBufHandle, (U32 *)&AIFFParserParams_p->OpBlk_p);
            if(Error != ST_NO_ERROR)
            {
                *No_Bytes_Used = offset;
                return STAUD_MEMORY_GET_ERROR;
            }
            else
            {
                AIFFParserParams_p->Put_Block = FALSE;
                AIFFParserParams_p->Get_Block = TRUE;
                AIFFParserParams_p->Current_Write_Ptr1 = (U8 *)AIFFParserParams_p->OpBlk_p->MemoryStart;

                memset((void*)AIFFParserParams_p->OpBlk_p->MemoryStart,0x00,AIFFParserParams_p->OpBlk_p->MaxSize);
                AIFFParserParams_p->OpBlk_p->Flags = 0;
                AIFFParserParams_p->OpBlk_p->Size= 0;

            }
        }

            value=pos[offset];

        //Parse the format chunk.

        switch(AIFFParserParams_p->ParserState)
        {
        case ES_AIFF_F:
            STTBX_Print((" ES_AIFF_ Parser State ES_AIFF_F \n "));

            if(value==0x46)
                {
                    AIFFParserParams_p->ParserState=ES_AIFF_O;
                }
            offset++;
            break;


        case ES_AIFF_O:
            STTBX_Print((" ES_AIFF_ Parser State ES_AIFF_0\n"));
            if(value==0x4F)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_R;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_F;
            }
            break;

        case ES_AIFF_R:
            STTBX_Print((" ES_AIFF_ Parser State ES_AIFF_R\n"));
            if(value==0x52)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_M;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_F;
            }
            break;


        case ES_AIFF_M:
            STTBX_Print((" ES_AIFF_ Parser State ES_AIFF_M \n"));
            if(value==0x4D)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_PARSE_CHUNK_SIZE_00;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_F;
            }
            break;


        case ES_AIFF_PARSE_CHUNK_SIZE_00:
            {
                AIFFParserParams_p->Chunk_Size=value;
                offset++;
                AIFFParserParams_p->ParserState=ES_AIFF_PARSE_CHUNK_SIZE_01;
                break;
            }

        case ES_AIFF_PARSE_CHUNK_SIZE_01:
            {
                AIFFParserParams_p->Chunk_Size = (AIFFParserParams_p->Chunk_Size<<8)| value;
                offset++;
                AIFFParserParams_p->ParserState=ES_AIFF_PARSE_CHUNK_SIZE_02;
                break;
            }

        case ES_AIFF_PARSE_CHUNK_SIZE_02:
            {
                AIFFParserParams_p->Chunk_Size = (AIFFParserParams_p->Chunk_Size<<8)| value;
                offset++;
                AIFFParserParams_p->ParserState=ES_AIFF_PARSE_CHUNK_SIZE_03;
                break;
            }
        case ES_AIFF_PARSE_CHUNK_SIZE_03:
            {
                AIFFParserParams_p->Chunk_Size = (AIFFParserParams_p->Chunk_Size<<8) | value;
                offset++;
                AIFFParserParams_p->ParserState=ES_AIFF_A;
                break;
            }
            // Look for AIFF keyword in the file
        case ES_AIFF_A:
            {
            if(value==0x41)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_I;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_F;
            }
            break;
            }

        case ES_AIFF_I:
            {
                if(value==0x49)
                {
                    AIFFParserParams_p->ParserState=ES_AIFF_F1;
                    offset++;
                }
                else
                {
                    AIFFParserParams_p->ParserState=ES_AIFF_F;
                }
                break;
            }

        case ES_AIFF_F1:
            {
                if(value==0x46)
                {
                AIFFParserParams_p->ParserState=ES_AIFF_F2;
                offset++;
                }
                else
                {
                AIFFParserParams_p->ParserState=ES_AIFF_F;
                }
                break;
            }

        case ES_AIFF_F2:
            {
                if(value==0x46)
                {
                    AIFFParserParams_p->ParserState=ES_AIFF_C;
                    offset++;
                }
                else
                {
                    AIFFParserParams_p->ParserState=ES_AIFF_F;
                }
                break;

            }

    // Look for the fmt keyword

        case ES_AIFF_C:
            {
                if(value==0x43)
                {
                    AIFFParserParams_p->ParserState=ES_AIFF_O1;
                    offset++;
                }
                else
                {
                    AIFFParserParams_p->ParserState=ES_AIFF_F;
                }
                break;
            }

        case ES_AIFF_O1:

            if(value==0x4F)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_M1;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_F;
            }
            break;

        case ES_AIFF_M1:

            if(value==0x4D)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_M2;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_F;
            }
            break;

        case ES_AIFF_M2:

            if(value==0x4D)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_PARSE_COMM_CHUNK_SIZE_S0;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_F;
            }
            break;

        case ES_AIFF_PARSE_COMM_CHUNK_SIZE_S0:
            AIFFParserParams_p->Comm_Chunk_Size = value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_COMM_CHUNK_SIZE_S1;
            offset++;
            break;

        case ES_AIFF_PARSE_COMM_CHUNK_SIZE_S1:
            AIFFParserParams_p->Comm_Chunk_Size = (AIFFParserParams_p->Comm_Chunk_Size<<8) | value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_COMM_CHUNK_SIZE_S2;
            offset++;
            break;

        case ES_AIFF_PARSE_COMM_CHUNK_SIZE_S2:
            AIFFParserParams_p->Comm_Chunk_Size = (AIFFParserParams_p->Comm_Chunk_Size<<8) | value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_COMM_CHUNK_SIZE_S3;
            offset++;
            break;

        case ES_AIFF_PARSE_COMM_CHUNK_SIZE_S3:
            AIFFParserParams_p->Comm_Chunk_Size = (AIFFParserParams_p->Comm_Chunk_Size<<8) | value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_NUMBER_OF_CHANNELS_N1;
            offset++;
            break;

            //Parse Compression Code of the AIFF file

        case ES_AIFF_PARSE_NUMBER_OF_CHANNELS_N1:
            AIFFParserParams_p->Number_Of_Channels= value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_NUMBER_OF_CHANNELS_N2;
            offset++;
            break;

        case ES_AIFF_PARSE_NUMBER_OF_CHANNELS_N2:
            AIFFParserParams_p->Number_Of_Channels= (AIFFParserParams_p->Number_Of_Channels<<8) | value;
            AIFFParserParams_p->OpBlk_p->Flags   |= NCH_VALID;
            AIFFParserParams_p->OpBlk_p->Data[NCH_OFFSET] = AIFFParserParams_p->Number_Of_Channels;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_00;
            offset++;
            break;

            //Parse Number of Channels from the streams

        case ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_00:
            AIFFParserParams_p->Number_of_SampleFrames=value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_01;
            offset++;
            break;

        case ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_01:
            AIFFParserParams_p->Number_of_SampleFrames=(AIFFParserParams_p->Number_of_SampleFrames<<8) | value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_02;
            offset++;
            break;

            // Parse the sampling frequency
        case ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_02:
            AIFFParserParams_p->Number_of_SampleFrames=(AIFFParserParams_p->Number_of_SampleFrames<<8) | value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_03;
            offset++;
            break;

        case ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_03:
            AIFFParserParams_p->Number_of_SampleFrames=(AIFFParserParams_p->Number_of_SampleFrames<<8) | value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SAMPLE_SIZE_0;
            offset++;
            break;

        case ES_AIFF_PARSE_SAMPLE_SIZE_0:
            AIFFParserParams_p->Sample_Size=value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SAMPLE_SIZE_1;
            offset++;
            break;

        case ES_AIFF_PARSE_SAMPLE_SIZE_1:
            AIFFParserParams_p->Sample_Size=(AIFFParserParams_p->Sample_Size<<8) | value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SAMPLE_RATE;
            offset++;
            break;

        case ES_AIFF_PARSE_SAMPLE_RATE:
            AIFFParserParams_p->Sample_Rate[AIFFParserParams_p->Sample_Rate_Counter] = value;
            offset++;
            AIFFParserParams_p->Sample_Rate_Counter++;
            if(AIFFParserParams_p->Sample_Rate_Counter>=10)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_CALCULATE_SAMPLING_RATE;
                AIFFParserParams_p->Sample_Rate_Counter=0;
                break;
            }
            break;

            // Calculation of Sampling Frequency

        case ES_AIFF_CALCULATE_SAMPLING_RATE:
            if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0E\xFA\x00\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 64000;
            else if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0D\xFA\x00\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 32000;
            else if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0C\xFA\x00\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 16000;
            else if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0B\xFA\x00\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 8000;
            else if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0E\xBB\x80\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 48000;
            else if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0D\xBB\x80\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 24000;
            else if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0C\xBB\x80\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 12000;
            else if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0E\xAC\x44\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 44100;
            else if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0D\xAC\x44\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 22050;
            else if ( strcmp((void *)AIFFParserParams_p->Sample_Rate, "\x40\x0C\xAC\x44\x00\x00\x00\x00\x00\x00") == 0 )
              AIFFParserParams_p->SamplingRate= 11025;
            else
                AIFFParserParams_p->SamplingRate= 64000; // Please suggest what should be done here ??

            AIFFParserParams_p->ParserState                      =ES_AIFF_S1;
            AIFFParserParams_p->Frequency                            =AIFFParserParams_p->SamplingRate;
            AIFFParserParams_p->OpBlk_p->Flags               |= FREQ_VALID;
            AIFFParserParams_p->OpBlk_p->Data[FREQ_OFFSET]   = AIFFParserParams_p->Frequency;
            break;


        case ES_AIFF_S1:
            if(value==0x53)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_S2;
            }
            offset++;
            break;

        case ES_AIFF_S2:
            if(value==0x53)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_N;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_S1;
            }
            break;

        case ES_AIFF_N:
            if(value==0x4E)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_D;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_S1;
            }
            break;

        case ES_AIFF_D:
            if(value==0x44)
            {
                AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_CHUNK_SIZE_00;
                offset++;
            }
            else
            {
                AIFFParserParams_p->ParserState=ES_AIFF_S1;
            }
            break;

            //Parse and Calculate Chunk Size

        case ES_AIFF_PARSE_SSND_CHUNK_SIZE_00:
            AIFFParserParams_p->SSND_Chunk_Size=value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_CHUNK_SIZE_01;
            offset++;
            break;

        case ES_AIFF_PARSE_SSND_CHUNK_SIZE_01:
            AIFFParserParams_p->SSND_Chunk_Size=(AIFFParserParams_p->SSND_Chunk_Size<<8)| value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_CHUNK_SIZE_02;
            offset++;
            break;

        case ES_AIFF_PARSE_SSND_CHUNK_SIZE_02:
            AIFFParserParams_p->SSND_Chunk_Size=(AIFFParserParams_p->SSND_Chunk_Size<<8)| value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_CHUNK_SIZE_03;
            offset++;
            break;

        case ES_AIFF_PARSE_SSND_CHUNK_SIZE_03:
            AIFFParserParams_p->SSND_Chunk_Size=(AIFFParserParams_p->SSND_Chunk_Size<<8)| value;
            AIFFParserParams_p->Chunk_Size = AIFFParserParams_p->SSND_Chunk_Size - 8;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_DATA_OFFSET_00;
            offset++;
            break;

            //Parse Data Offset. Mostly 0
        case ES_AIFF_PARSE_SSND_DATA_OFFSET_00:
            AIFFParserParams_p->SSND_DataOffset=value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_DATA_OFFSET_01;
            offset++;
            break;

        case ES_AIFF_PARSE_SSND_DATA_OFFSET_01:
            AIFFParserParams_p->SSND_DataOffset=(AIFFParserParams_p->DataOffset<<8)|value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_DATA_OFFSET_02;
            offset++;
            break;

        case ES_AIFF_PARSE_SSND_DATA_OFFSET_02:
            AIFFParserParams_p->SSND_DataOffset=(AIFFParserParams_p->DataOffset<<8)|value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_DATA_OFFSET_03;
            offset++;
            break;

        case ES_AIFF_PARSE_SSND_DATA_OFFSET_03:
            AIFFParserParams_p->SSND_DataOffset=(AIFFParserParams_p->DataOffset<<8)|value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_BLOCK_SIZE_00;
            if (AIFFParserParams_p->SSND_DataOffset == 0)
                offset++;
            else
                offset += AIFFParserParams_p->SSND_DataOffset;
            break;

            // Block Size
        case ES_AIFF_PARSE_SSND_BLOCK_SIZE_00:
            AIFFParserParams_p->SSND_BlockSize=value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_BLOCK_SIZE_01;
            offset++;
            break;

        case ES_AIFF_PARSE_SSND_BLOCK_SIZE_01:
            AIFFParserParams_p->SSND_BlockSize=(AIFFParserParams_p->SSND_BlockSize<<8)|value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_BLOCK_SIZE_02;
            offset++;
            break;

        case ES_AIFF_PARSE_SSND_BLOCK_SIZE_02:
            AIFFParserParams_p->SSND_BlockSize=(AIFFParserParams_p->SSND_BlockSize<<8)|value;
            AIFFParserParams_p->ParserState=ES_AIFF_PARSE_SSND_BLOCK_SIZE_03;
            offset++;
            break;

        case ES_AIFF_PARSE_SSND_BLOCK_SIZE_03:
            AIFFParserParams_p->SSND_BlockSize=(AIFFParserParams_p->SSND_BlockSize<<8)|value;
            AIFFParserParams_p->ParserState=ES_AIFF_COMPUTE_FRAME_LENGTH;
            offset++;
            break;

            // Compute Frame Length
        case ES_AIFF_COMPUTE_FRAME_LENGTH:
            AIFFParserParams_p->FrameSize    = (AIFFParserParams_p->Sample_Size/8)*(AIFFParserParams_p->Number_Of_Channels)*(CDDA_NB_SAMPLES << 1); // CHECK AGAIN
            AIFFParserParams_p->AppData_p->sampleRate        = AIFFParserParams_p->SamplingRate;
            switch (AIFFParserParams_p->Sample_Size)
            {
            case 8:
                AIFFParserParams_p->AppData_p->sampleBits  = ACC_LPCM_WS8s;
                break;

            case 16:
                AIFFParserParams_p->AppData_p->sampleBits  = ACC_LPCM_WS16;
                break;

            case 24:
            case 32:
                AIFFParserParams_p->AppData_p->sampleBits  = ACC_LPCM_WS32;
                break;

            default:
                AIFFParserParams_p->AppData_p->sampleBits  = ACC_LPCM_WS16;
                break;
            }

            AIFFParserParams_p->AppData_p->channels          = AIFFParserParams_p->Number_Of_Channels;
            AIFFParserParams_p->AppData_p->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
            AIFFParserParams_p->AppData_p->ChangeSet         = TRUE;
            AIFFParserParams_p->AppDataFlag                  = TRUE;
            AIFFParserParams_p->OpBlk_p->AppData_p           = AIFFParserParams_p->AppData_p;
            AIFFParserParams_p->NoBytesRemainingInFrame      = AIFFParserParams_p->FrameSize;
            AIFFParserParams_p->ParserState                  = ES_AIFF_FRAME;

        case ES_AIFF_FRAME:
            if(AIFFParserParams_p->Chunk_Size != 0)
            {
                U32 DataAvailable = Size-offset;
                U32 DataToCopy = DataAvailable;

                if (AIFFParserParams_p->NoBytesRemainingInFrame < AIFFParserParams_p->Chunk_Size)
                {
                    if(DataAvailable > AIFFParserParams_p->NoBytesRemainingInFrame)
                    {
                        DataToCopy = AIFFParserParams_p->NoBytesRemainingInFrame;
                    }
                }
                else
                {
                    if(DataAvailable > AIFFParserParams_p->Chunk_Size)
                    {
                        DataToCopy = AIFFParserParams_p->Chunk_Size;
                    }
                }

                if(AIFFParserParams_p->Sample_Size==24)
                {
                    if(DataToCopy > AIFFParserParams_p->SpaceInBuffer)
                        DataToCopy = AIFFParserParams_p->SpaceInBuffer;

                    if(DataToCopy > AIFFParserParams_p->Chunk_Size)
                        DataToCopy = AIFFParserParams_p->Chunk_Size;
                }

                if(DataToCopy)
                {
                    switch (AIFFParserParams_p->Sample_Size)
                    {
                    case 24:

                        memcpy(((char *)AIFFParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress+ (MAXBUFFSIZE - AIFFParserParams_p->SpaceInBuffer)),((char *)MemoryStart+offset),DataToCopy);

                        AIFFParserParams_p->Chunk_Size -= DataToCopy;
                        offset += DataToCopy;

                        AIFFParserParams_p->NoBytesRemainingInFrame -= DataToCopy;

                        AIFFParserParams_p->SpaceInBuffer -= DataToCopy;

                        if((AIFFParserParams_p->SpaceInBuffer == 0) || (AIFFParserParams_p->Chunk_Size==0))
                        {
                            AIFFParserParams_p->node_p->Node.DestinationAddress_p = (void *)(STOS_AddressTranslate(AIFFParserParams_p->ESBufferAddress.BasePhyAddress, AIFFParserParams_p->ESBufferAddress.BaseUnCachedVirtualAddress,AIFFParserParams_p->Current_Write_Ptr1));
                            AIFFParserParams_p->node_p->Node.SourceAddress_p = (void *)(AIFFParserParams_p->DummyBufferAddress.BasePhyAddress) ;

                            FDMAMemcpy(AIFFParserParams_p->node_p->Node.DestinationAddress_p, AIFFParserParams_p->node_p->Node.SourceAddress_p,3,MAXBUFFSIZE,3,4,&AIFFParserParams_p->FDMAnodeAddress);


                            if(AIFFParserParams_p->Chunk_Size!=0)
                            {
                                AIFFParserParams_p->Current_Write_Ptr1 += OPBLK_SIZE_24BIT;
                                AIFFParserParams_p->OpBlk_p->Size =OPBLK_SIZE_24BIT ; // = 3528 * (4/3) FOR EVERY 3 BYTES WE COPY 4 BYTES
                            }
                            else
                            {
                                AIFFParserParams_p->Current_Write_Ptr1 += OPBLK_SIZE_24BIT - ((AIFFParserParams_p->SpaceInBuffer*4)/3);
                                AIFFParserParams_p->OpBlk_p->Size = OPBLK_SIZE_24BIT - ((AIFFParserParams_p->SpaceInBuffer*4)/3);
                            }

                            if(AIFFParserParams_p->SpaceInBuffer == 0)
                                AIFFParserParams_p->SpaceInBuffer = MAXBUFFSIZE;
                        }

                            break;


                    case 8:
                    case 16:
                    case 32:
                        memcpy(AIFFParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),DataToCopy);
                        AIFFParserParams_p->DataCopied += DataToCopy;
                        AIFFParserParams_p->Chunk_Size -= DataToCopy;
                        offset += DataToCopy;
                        AIFFParserParams_p->Current_Write_Ptr1 += DataToCopy;
                        AIFFParserParams_p->NoBytesRemainingInFrame -= DataToCopy;
                        AIFFParserParams_p->OpBlk_p->Size += DataToCopy;

                        break;

                    default : break;
                    }
                }


                if((AIFFParserParams_p->Chunk_Size == 0) || (AIFFParserParams_p->NoBytesRemainingInFrame == 0))
                {
                    if(AIFFParserParams_p->Chunk_Size == 0)
                    {
                        AIFFParserParams_p->ParserState = ES_AIFF_F;
                        AIFFParserParams_p->SpaceInBuffer = MAXBUFFSIZE;
                    }

                    AIFFParserParams_p->NoBytesRemainingInFrame= AIFFParserParams_p->FrameSize;

                    if(AIFFParserParams_p->Get_Block == TRUE)
                    {
                        if((AIFFParserParams_p->Sample_Size==24)||(AIFFParserParams_p->Sample_Size==32))
                        {
                            U16 *src,*dst;
                            U32 *src32,*dst32;
                            U32 i,size=0;
                            src = (U16 *)AIFFParserParams_p->OpBlk_p->MemoryStart;
                            dst = (U16 *)AIFFParserParams_p->OpBlk_p->MemoryStart;
                            src32 = (U32 *)AIFFParserParams_p->OpBlk_p->MemoryStart;
                            dst32 = (U32 *)AIFFParserParams_p->OpBlk_p->MemoryStart;

                            if(AIFFParserParams_p->Sample_Size ==32)
                                size = (AIFFParserParams_p->FrameSize>> 1);

                            if(AIFFParserParams_p->Sample_Size ==24)
                                size = (OPBLK_SIZE_24BIT >> 1);
#ifndef ST_OSWINCE
                            for ( i = 0 ; i < size; i++ )
                            {
                                __asm__("swap.b %1, %0" : "=r" (dst[i]): "r" (src[i]));
                            }
#endif

                            if(AIFFParserParams_p->Sample_Size ==32)
                                size = (AIFFParserParams_p->FrameSize >> 2);

                            if(AIFFParserParams_p->Sample_Size ==24)
                                size = (OPBLK_SIZE_24BIT >> 2);

#ifndef ST_OSWINCE
                            for ( i = 0 ; i < size; i++ )
                            {
                                __asm__("swap.w %1, %0" : "=r" (dst32[i]): "r" (src32[i]));
                            }
#endif
                        }

                        Error = MemPool_PutOpBlk(AIFFParserParams_p->OpBufHandle,(U32 *)&AIFFParserParams_p->OpBlk_p);
                        if(Error != ST_NO_ERROR)
                        {
                            STTBX_Print((" ES_AIFF Error in Memory Put_Block \n"));
                            return Error;
                        }
                        else
                        {
                            AIFFParserParams_p->Put_Block = TRUE;
                            AIFFParserParams_p->Get_Block = FALSE;
                        }
                    }
                }
            }

        break;

        default:
            offset ++;
            break;

        }
    }

    *No_Bytes_Used=offset;
    return ST_NO_ERROR;
}

/************************************************************************************
Name         : ES_AIFF_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/



ST_ErrorCode_t ES_AIFF_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_AIFF_ParserLocalParams_t *AIFFParserParams_p;
    AIFFParserParams_p=(ES_AIFF_ParserLocalParams_t *)Handle;

    if(AIFFParserParams_p->Get_Block == TRUE)
    {
        if((AIFFParserParams_p->Sample_Size==8) || (AIFFParserParams_p->Sample_Size==16))
        AIFFParserParams_p->OpBlk_p->Size = AIFFParserParams_p->FrameSize ;

        if(AIFFParserParams_p->Sample_Size==24 ||(AIFFParserParams_p->Sample_Size==32))
        AIFFParserParams_p->OpBlk_p->Size = MAXBUFFSIZE;

        Error = MemPool_PutOpBlk(AIFFParserParams_p->OpBufHandle,(U32 *)&AIFFParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AIFF Error in Memory Put_Block \n"));
        }
        else
        {
            AIFFParserParams_p->Put_Block = TRUE;
            AIFFParserParams_p->Get_Block = FALSE;
        }
    }

    AIFFParserParams_p->ParserState = ES_AIFF_F;
    AIFFParserParams_p->Frequency = 0xFFFFFFFF;
    return Error;
}


/************************************************************************************
Name         : ES_AIFF_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_AIFF_Parser_Start(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_AIFF_ParserLocalParams_t *AIFFParserParams_p;
    AIFFParserParams_p=(ES_AIFF_ParserLocalParams_t *)Handle;

    AIFFParserParams_p->ParserState = ES_AIFF_F;
    AIFFParserParams_p->Frequency = 0xFFFFFFFF;

    AIFFParserParams_p->Put_Block = TRUE;
    AIFFParserParams_p->Get_Block = FALSE;

    return Error;
}

#endif
