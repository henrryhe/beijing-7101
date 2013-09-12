/************************************************************************
COPYRIGHT STMicroelectronics (C) 2007
Source file name : es_wave_parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_WAVE
//#define  STTBX_PRINT
#include "es_wave_parser.h"
#include "es_lpcma_parser.h"
#include "aud_utils.h"
#include "acc_multicom_toolbox.h"
#include "aud_pes_es_parser.h"
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
#include "aud_evt.h"

#define OPBLK_SIZE_24BIT ((MAXBUFFSIZE*4)/3)

static U32 AdpcmSamplesIn(
    U32 dataLen,
    U32 chans,
    U32 blockAlign,
    U32 samplesPerBlock )
{
    U32 m, n;

    if (samplesPerBlock)
    {
        n = (dataLen / blockAlign) * samplesPerBlock;
        m = (dataLen % blockAlign);
    }
    else
    {
        n = 0;
        m = blockAlign;
    }
    if (m >= (size_t)(7*chans))
    {
        m -= 7*chans;          /* bytes beyond block-header */
        m = (2*m)/chans + 2;   /* nibbles/chans + 2 in header */
        if (samplesPerBlock && m > samplesPerBlock)
        {
            m = samplesPerBlock;
        }
        n += m;
    }
    return n;
}

static U32 ImaAdpcmSamplesIn(
    U32 dataLen,
    U32 chans,
    U32 blockAlign,
    U32 samplesPerBlock
)
{
    U32 m, n;

    if (samplesPerBlock)
    {
        n = (dataLen / blockAlign) * samplesPerBlock;
        m = (dataLen % blockAlign);
    }
    else
    {
        n = 0;
        m = blockAlign;
    }
    if (m >= (unsigned int)4*chans)
    {
        m -= 4*chans;     // number of bytes beyond block-header
        m /= 4*chans;     // number of 4-byte blocks/channel beyond header
        m  = 8*m + 1;     // samples/chan beyond header + 1 in header
        if (samplesPerBlock && m > samplesPerBlock) m = samplesPerBlock;
        n += m;
    }
    return n;
    // wSamplesPerBlock = ((wBlockAlign - 4*wChannels)/(4*wChannels))*8 + 1;

}
/************************************************************************************
Name         : ES_Wave_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
             ES_Wave_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/


ST_ErrorCode_t ES_Wave_Parser_Init(ComParserInit_t * Init_p, ParserHandle_t * const handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_Wave_ParserLocalParams_t * WaveParserParams_p;
    STAUD_BufferParams_t          MemoryBuffer;

    WaveParserParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(ES_Wave_ParserLocalParams_t));
    if(WaveParserParams_p == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(WaveParserParams_p,0,sizeof(ES_Wave_ParserLocalParams_t));
    WaveParserParams_p->AppData_p = (STAud_LpcmStreamParams_t *)STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(STAud_LpcmStreamParams_t));
    if(WaveParserParams_p->AppData_p == NULL)
    {
        /*deallocate the allocated memory*/
        STOS_MemoryDeallocate(Init_p->Memory_Partition, WaveParserParams_p);
        return ST_ERROR_NO_MEMORY;
    }
    memset(WaveParserParams_p->AppData_p,0,sizeof(STAud_LpcmStreamParams_t));

    WaveParserParams_p->DummyBufferSize = MAXBUFFSIZE;

    WaveParserParams_p->node_p=Init_p->NodePESToES_p;
    WaveParserParams_p->FDMAnodeAddress.BaseUnCachedVirtualAddress=(U32 *)Init_p->NodePESToES_p;
    WaveParserParams_p->FDMAnodeAddress.BasePhyAddress = (U32*)STOS_VirtToPhys(Init_p->NodePESToES_p);

    Error|=MemPool_GetBufferParams(Init_p->OpBufHandle, &MemoryBuffer);
    if(Error!=ST_NO_ERROR)
    {
        STOS_MemoryDeallocate(Init_p->Memory_Partition, WaveParserParams_p->AppData_p);
        STOS_MemoryDeallocate(Init_p->Memory_Partition, WaveParserParams_p);
        return Error;
    }
    WaveParserParams_p->ESBufferAddress.BaseUnCachedVirtualAddress = (U32*)MemoryBuffer.BufferBaseAddr_p;
    WaveParserParams_p->ESBufferAddress.BasePhyAddress = (U32*)STOS_VirtToPhys(MemoryBuffer.BufferBaseAddr_p);
    WaveParserParams_p->ESBufferAddress.BaseCachedVirtualAddress = NULL ;

    WaveParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress = (U32*)Init_p->DummyBufferBaseAddress;
    WaveParserParams_p->DummyBufferAddress.BasePhyAddress = (U32*)STOS_VirtToPhys(Init_p->DummyBufferBaseAddress);
    WaveParserParams_p->DummyBufferAddress.BaseCachedVirtualAddress = NULL;

    WaveParserParams_p->SpaceInBuffer                = MAXBUFFSIZE;
    /* Init the Local Parameters */
    WaveParserParams_p->ParserState                  = ES_WAVE_R;
    WaveParserParams_p->Block_Align                  = 0;
    WaveParserParams_p->FrameSize                    = 0XFFFFFFFF;
    WaveParserParams_p->Compression_Code             = 0;
    WaveParserParams_p->SampleSize                   = 0xFFFFFFFF;
    WaveParserParams_p->Number_Of_Channels           = 0XFFFF;
    WaveParserParams_p->Sample_Rate                  = 0xFFFFFFFF;
    WaveParserParams_p->Bytes_Per_Second             = 0xFFFFFFFF;
    WaveParserParams_p->Significant_Bits_Per_Sample  = 0XFFFF;
    WaveParserParams_p->Chunk_Size                   = 0xFFFFFFFF;
    WaveParserParams_p->NoBytesRemainingInFrame      = 0XFFFF;
    WaveParserParams_p->Frequency                    = 0xFFFFFFFF;
    WaveParserParams_p->Get_Block                    = FALSE;
    WaveParserParams_p->Put_Block                    = TRUE;
    WaveParserParams_p->OpBufHandle                  = Init_p->OpBufHandle;

    /*initialize the event IDs*/
    *handle                                          =(ParserHandle_t)WaveParserParams_p;

    return (Error);
}

/************************************************************************************
Name         : ES_Wave_ParserGetParsingFunction()

Description  : Sets the parsing function, Frequency function and the info function

Parameters   : ParsingFunction   * ParsingFunc
               GetFreqFunction_t * GetFreqFunction
               GetInfoFunction_t * GetInfoFunction

Return Value :
    ST_NO_ERROR                     Success.

 ************************************************************************************/
ST_ErrorCode_t ES_Wave_ParserGetParsingFunction(    ParsingFunction_t   * ParsingFunc_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p )
{

    *ParsingFunc_p      = (ParsingFunction_t)ES_Wave_Parse_Frame;
    *GetFreqFunction_p  = (GetFreqFunction_t)ES_Wave_Get_Freq;
    *GetInfoFunction_p  = (GetInfoFunction_t)ES_Wave_Get_Info;

    return ST_NO_ERROR;


}

/************************************************************************************
Name         : ES_Wave_Get_Freq()

Description  : Sets the Frequency

Parameters   : ParserHandle_t  Handle
               U32 *SamplingFrequency_p

Return Value :
    ST_NO_ERROR                     Success

 ************************************************************************************/

ST_ErrorCode_t ES_Wave_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_Wave_ParserLocalParams_t *WaveParserParams_p;
    WaveParserParams_p=(ES_Wave_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p = WaveParserParams_p->Sample_Rate;

    return ST_NO_ERROR;
}

/************************************************************************************
Name            :  ES_Wave_Get_Info()

Description    :  Get StreamInfo from Parser

Parameters    : ParserHandle_t  Handle
                 STAUD_StreamInfo_t * StreamInfo

Return Value  :
    ST_NO_ERROR                     Success

 ************************************************************************************/

ST_ErrorCode_t ES_Wave_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_Wave_ParserLocalParams_t *WaveParserParams_p;

    WaveParserParams_p            =(ES_Wave_ParserLocalParams_t *)Handle;
    StreamInfo->BitRate           = WaveParserParams_p->Bytes_Per_Second * 8;
    StreamInfo->SamplingFrequency = WaveParserParams_p->Sample_Rate;

    return ST_NO_ERROR;
}

 /************************************************************************************
Name            :  ES_Wave_Handle_EOF()

Description    :  Handle EOF from Parser

Parameters    : ParserHandle_t  Handle

Return Value  :
    ST_NO_ERROR                     Success

 ************************************************************************************/

ST_ErrorCode_t ES_Wave_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_Wave_ParserLocalParams_t *WaveParserParams_p = (ES_Wave_ParserLocalParams_t *)Handle;

    if(WaveParserParams_p->Get_Block == TRUE)
    {
        memset(WaveParserParams_p->Current_Write_Ptr1,0x00,WaveParserParams_p->NoBytesRemainingInFrame);

        if(WaveParserParams_p->Significant_Bits_Per_Sample==24)
        {
            if(WaveParserParams_p->SpaceInBuffer < MAXBUFFSIZE)
            {
                WaveParserParams_p->node_p->Node.DestinationAddress_p = (void *)STOS_AddressTranslate(WaveParserParams_p->ESBufferAddress.BasePhyAddress,WaveParserParams_p->ESBufferAddress.BaseUnCachedVirtualAddress, ((U32)(WaveParserParams_p->Current_Write_Ptr1)+1));
                WaveParserParams_p->node_p->Node.SourceAddress_p = (void *)STOS_AddressTranslate(WaveParserParams_p->DummyBufferAddress.BasePhyAddress,WaveParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress, WaveParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress);
                WaveParserParams_p->OpBlk_p->Size = (((MAXBUFFSIZE - WaveParserParams_p->SpaceInBuffer)*4)/3);
                STTBX_Print(("\nEntering generic transfer"));
                FDMAMemcpy(WaveParserParams_p->node_p->Node.DestinationAddress_p, WaveParserParams_p->node_p->Node.SourceAddress_p,3,MAXBUFFSIZE,3,4,&WaveParserParams_p->FDMAnodeAddress);
            }
        }

        if((WaveParserParams_p->Significant_Bits_Per_Sample==8) || (WaveParserParams_p->Significant_Bits_Per_Sample==16) || (WaveParserParams_p->Significant_Bits_Per_Sample == 32))
        WaveParserParams_p->OpBlk_p->Size = WaveParserParams_p->FrameSize;

        Error = MemPool_PutOpBlk(WaveParserParams_p->OpBufHandle,(U32 *)&WaveParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_Wave Error in Memory Put_Block \n"));
        }
        else
        {
            WaveParserParams_p->Put_Block = TRUE;
            WaveParserParams_p->Get_Block = FALSE;
        }
    }
    else
    {
        Error = MemPool_GetOpBlk(WaveParserParams_p->OpBufHandle, (U32 *)&WaveParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            return STAUD_MEMORY_GET_ERROR;
        }
        else
        {
            WaveParserParams_p->Put_Block = FALSE;
            WaveParserParams_p->Get_Block = TRUE;
            WaveParserParams_p->OpBlk_p->Size = 1152;
            WaveParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
            memset((void*)WaveParserParams_p->OpBlk_p->MemoryStart,0x00,WaveParserParams_p->OpBlk_p->MaxSize);
            Error = MemPool_PutOpBlk(WaveParserParams_p->OpBufHandle,(U32 *)&WaveParserParams_p->OpBlk_p);
            if(Error != ST_NO_ERROR)
            {
                STTBX_Print((" ES_Wave Error in Memory Put_Block \n"));
            }
            else
            {
                WaveParserParams_p->Put_Block = TRUE;
                WaveParserParams_p->Get_Block = FALSE;
            }
        }
    }
    WaveParserParams_p->ParserState                  = ES_WAVE_R;
    return ST_NO_ERROR;
}

 /************************************************************************************
Name            :  ES_Wave_Parser_Term()

Description    :  Terminate the Parser. Deallocate the memory allocated in init.

Parameters    : ParserHandle_t  Handle

Return Value  :
    ST_NO_ERROR                     Success

 ************************************************************************************/

ST_ErrorCode_t ES_Wave_Parser_Term(ParserHandle_t *const handle, ST_Partition_t *CPUPartition_p)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    ES_Wave_ParserLocalParams_t *WaveParserLocalParams_p = (ES_Wave_ParserLocalParams_t *)handle;

    if(WaveParserLocalParams_p)
    {
        STOS_MemoryDeallocate(CPUPartition_p, WaveParserLocalParams_p->AppData_p);
        STOS_MemoryDeallocate(CPUPartition_p, WaveParserLocalParams_p);
    }

    return (Error);

}

/************************************************************************************
Name           : Parse_Wave_Frame()

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



ST_ErrorCode_t ES_Wave_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used)

{
    U8 *pos;
    U32 offset;
    U8 value;
    ST_ErrorCode_t Error;
    ES_Wave_ParserLocalParams_t *WaveParserParams_p;

    offset=*No_Bytes_Used;


    pos=(U8 *)MemoryStart;

    //STTBX_Print((" ES_WAVE Parser Enrty \n"));

    WaveParserParams_p=(ES_Wave_ParserLocalParams_t *)Handle;

    while(offset<Size)
    {

            if(WaveParserParams_p->Put_Block == TRUE)
            {
                Error = MemPool_GetOpBlk(WaveParserParams_p->OpBufHandle, (U32 *)&WaveParserParams_p->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    *No_Bytes_Used = offset;
                    return STAUD_MEMORY_GET_ERROR;
                }
                else
                {
                    WaveParserParams_p->Put_Block = FALSE;
                    WaveParserParams_p->Get_Block = TRUE;
                    WaveParserParams_p->Current_Write_Ptr1 = (U8 *)WaveParserParams_p->OpBlk_p->MemoryStart;

                    memset((void*)WaveParserParams_p->OpBlk_p->MemoryStart,0x00,WaveParserParams_p->OpBlk_p->MaxSize);
                    WaveParserParams_p->OpBlk_p->Flags = 0;
                    WaveParserParams_p->OpBlk_p->Size= 0;

                }
            }


            value=pos[offset];

        //Parse the format chunk.


        switch(WaveParserParams_p->ParserState)
        {
        case ES_WAVE_R:
            //  STTBX_Print((" ES_WAVE_ Parser State ES_WAVE_R \n "));

            if(value==0x52)
            {
                WaveParserParams_p->ParserState=ES_WAVE_I;
            }
            offset++;
            break;

        case ES_WAVE_I:
            //  STTBX_Print((" ES_WAVE_ Parser State ES_WAVE_R\n"));
            if(value==0x49)
            {
                WaveParserParams_p->ParserState=ES_WAVE_F1;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_R;
            }
            break;

        case ES_WAVE_F1:
            //  STTBX_Print((" ES_Wave_ Parser State ES_Wave_R\n"));
            if(value==0x46)
            {
                WaveParserParams_p->ParserState=ES_WAVE_F2;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_R;
            }
            break;

        case ES_WAVE_F2:
            //  STTBX_Print((" ES_Wave_ Parser State ES_Wave_R \n"));
            if(value==0x46)
            {
                WaveParserParams_p->ParserState=ES_WAVE_Chunk_Data_Size_0;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_R;
            }
            break;

        case ES_WAVE_Chunk_Data_Size_0:
            WaveParserParams_p->ParserState=ES_WAVE_Chunk_Data_Size_1;
            offset++;
            break;

        case ES_WAVE_Chunk_Data_Size_1:
            WaveParserParams_p->ParserState=ES_WAVE_Chunk_Data_Size_2;
            offset++;
            break;

        case ES_WAVE_Chunk_Data_Size_2:
            WaveParserParams_p->ParserState=ES_WAVE_Chunk_Data_Size_3;
            offset++;
            break;

        case ES_WAVE_Chunk_Data_Size_3:
            WaveParserParams_p->ParserState=ES_WAVE_RIFF_TYPE_W;
            offset++;
            break;
            // Look for WAVE keyword in the file
        case ES_WAVE_RIFF_TYPE_W:
            if(value==0x57)
            {
                WaveParserParams_p->ParserState=ES_WAVE_RIFF_TYPE_A;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_R;
            }
            break;

        case ES_WAVE_RIFF_TYPE_A:
            if(value==0x41)
            {
                WaveParserParams_p->ParserState=ES_WAVE_RIFF_TYPE_V;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_R;
            }
            break;

        case ES_WAVE_RIFF_TYPE_V:
            if(value==0x56)
            {
                WaveParserParams_p->ParserState=ES_WAVE_RIFF_TYPE_E;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_R;
            }
            break;

        case ES_WAVE_RIFF_TYPE_E:
            if(value==0x45)
            {
                WaveParserParams_p->ParserState=ES_WAVE_F;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_R;
            }
            break;

            // Look for the fmt keyword
        case ES_WAVE_F:
            if(value==0x66)
            {
                WaveParserParams_p->ParserState=ES_WAVE_M;
            }
            offset++;
            break;

        case ES_WAVE_M:

            if(value==0x6D)
            {
                WaveParserParams_p->ParserState=ES_WAVE_T1;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_F;
            }
            break;

        case ES_WAVE_T1:
            if(value==0x74)
            {
                WaveParserParams_p->ParserState=ES_WAVE_T2;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_F;
            }
            break;

        case ES_WAVE_T2:

            if(value==0x20)
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Chunk_Data_Size_S1;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_F;
            }
            break;

        case ES_WAVE_Parse_Chunk_Data_Size_S1:
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Chunk_Data_Size_S2;
            offset++;
            break;

        case ES_WAVE_Parse_Chunk_Data_Size_S2:
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Chunk_Data_Size_S3;
            offset++;
            break;

        case ES_WAVE_Parse_Chunk_Data_Size_S3:
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Chunk_Data_Size_S4;
            offset++;
            break;

        case ES_WAVE_Parse_Chunk_Data_Size_S4:
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Compression_Code_C1;
            offset++;
            break;

            //Parse Compression Code of the wave file
        case ES_WAVE_Parse_Compression_Code_C1:
            WaveParserParams_p->Compression_Code=value;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Compression_Code_C2;
            offset++;
            break;

        case ES_WAVE_Parse_Compression_Code_C2:
            WaveParserParams_p->Compression_Code=WaveParserParams_p->Compression_Code|(value<<8);
            if(WaveParserParams_p->Compression_Code!=1
                &&WaveParserParams_p->Compression_Code!=2
                 &&WaveParserParams_p->Compression_Code!=17)
            {
                WaveParserParams_p->ParserState=ES_REJECT_DATA;
                WaveParserParams_p->Get_Block = FALSE;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Number_of_Channels_N1;
            }
            offset++;
            break;
            //Parse Number of Channels from the streams

        case ES_WAVE_Parse_Number_of_Channels_N1:
            WaveParserParams_p->Number_Of_Channels=value;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Number_of_Channels_N2;
            offset++;
            break;


        case ES_WAVE_Parse_Number_of_Channels_N2:
            WaveParserParams_p->Number_Of_Channels= WaveParserParams_p->Number_Of_Channels |(value<<8);
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Sample_Rate_R1;
            WaveParserParams_p->OpBlk_p->Flags   |= NCH_VALID;
            WaveParserParams_p->OpBlk_p->Data[NCH_OFFSET] = WaveParserParams_p->Number_Of_Channels;
            offset++;
            break;

            // Parse the sampling frequency
        case ES_WAVE_Parse_Sample_Rate_R1:
            WaveParserParams_p->Sample_Rate=value;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Sample_Rate_R2;
            offset++;
            break;

        case ES_WAVE_Parse_Sample_Rate_R2:
            WaveParserParams_p->Sample_Rate = WaveParserParams_p->Sample_Rate |(value<<8);
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Sample_Rate_R3;
            offset++;
            break;

        case ES_WAVE_Parse_Sample_Rate_R3:
            WaveParserParams_p->Sample_Rate=WaveParserParams_p->Sample_Rate |(value<<16);
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Sample_Rate_R4;
            offset++;
            break;

        case ES_WAVE_Parse_Sample_Rate_R4:
            WaveParserParams_p->Sample_Rate= WaveParserParams_p->Sample_Rate |(value<<24);
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Bytes_per_sec_B1;
            offset++;
            WaveParserParams_p->Frequency=WaveParserParams_p->Sample_Rate;
            WaveParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
            WaveParserParams_p->OpBlk_p->Data[FREQ_OFFSET]   = WaveParserParams_p->Frequency;
            break;

        case ES_WAVE_Parse_Bytes_per_sec_B1:
            WaveParserParams_p->Bytes_Per_Second=value;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Bytes_per_sec_B2;
            offset++;
            break;

        case ES_WAVE_Parse_Bytes_per_sec_B2:
            WaveParserParams_p->Bytes_Per_Second |=value<<8;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Bytes_per_sec_B3;
            offset++;
            break;

        case ES_WAVE_Parse_Bytes_per_sec_B3:
            WaveParserParams_p->Bytes_Per_Second |=value<<16;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Bytes_per_sec_B4;
            offset++;
            break;

        case ES_WAVE_Parse_Bytes_per_sec_B4:
            WaveParserParams_p->Bytes_Per_Second |=value<<24;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Block_Align_BA1;
            offset++;
            break;

        case ES_WAVE_Parse_Block_Align_BA1:
            WaveParserParams_p->Block_Align=value;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Block_Align_BA2;
            offset++;
            break;

        case ES_WAVE_Parse_Block_Align_BA2:
            WaveParserParams_p->Block_Align |=value<<8;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Significant_Bits_Per_Sample_1;
            offset++;
            break;

        case ES_WAVE_Parse_Significant_Bits_Per_Sample_1:
            WaveParserParams_p->Significant_Bits_Per_Sample=value;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Significant_Bits_Per_Sample_2;
            offset++;
            break;

        case ES_WAVE_Parse_Significant_Bits_Per_Sample_2:
            WaveParserParams_p->Significant_Bits_Per_Sample |=value<<8;
            WaveParserParams_p->ParserState=ES_WAVE_CHECK_COMPRESSED_CODE;
            offset++;
            break;

            // Parse DATA keyword
        case ES_WAVE_CHECK_COMPRESSED_CODE:
            if(WaveParserParams_p->Compression_Code==1)
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Data_D;

            }
            else if(WaveParserParams_p->Compression_Code==2 || WaveParserParams_p->Compression_Code==17)
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_COMP_Data_D;
            }
            break;

        case ES_WAVE_Parse_COMP_Data_D:
            WaveParserParams_p->ExtraSize = value;
            WaveParserParams_p->ParserState=ES_WAVE_BYTES_EXTRA1;
            offset++;
            break;

        case ES_WAVE_BYTES_EXTRA1:
            WaveParserParams_p->ExtraSize |=value<<8;
            WaveParserParams_p->ParserState=ES_WAVE_CHECK_SIZE;
            offset++;
            break;

        case ES_WAVE_CHECK_SIZE:
            if(WaveParserParams_p->Significant_Bits_Per_Sample!=4)
            {
                WaveParserParams_p->ParserState=ES_REJECT_DATA;
            }

            if(WaveParserParams_p->Compression_Code==2)//ADPCM
            {
                if (WaveParserParams_p->ExtraSize < 4)
                {
                    WaveParserParams_p->ParserState=ES_REJECT_DATA;
                    WaveParserParams_p->Get_Block = FALSE;
                    break;
                }

            }
            else//IMA_ADPCM
            {
                if (WaveParserParams_p->ExtraSize < 2)
                {
                    WaveParserParams_p->ParserState=ES_REJECT_DATA;
                    WaveParserParams_p->Get_Block = FALSE;
                    break;
                }
            }

            WaveParserParams_p->ParserState=ES_WAVE_COM_DATA_EXT;
            break;

        case ES_WAVE_COM_DATA_EXT:
            WaveParserParams_p->SamplesPerBlock = value;
            WaveParserParams_p->ParserState=ES_WAVE_COM_SAMPLES_BLOCK;
            offset++;
            break;

        case ES_WAVE_COM_SAMPLES_BLOCK:
            WaveParserParams_p->SamplesPerBlock |= value<<8;
            WaveParserParams_p->ParserState=ES_WAVE_COM_CHECK_BLOCK;
            offset++;
            break;

        case ES_WAVE_COM_CHECK_BLOCK    :
            {
                U32 BytesPerBlock;
                if(WaveParserParams_p->Compression_Code==2)
                {

                    BytesPerBlock = 7*WaveParserParams_p->Nch;
                    if(WaveParserParams_p->SamplesPerBlock>2)
                    BytesPerBlock+= ((WaveParserParams_p->SamplesPerBlock-2)*WaveParserParams_p->Number_Of_Channels+1)/2;

                    if(BytesPerBlock > WaveParserParams_p->Block_Align)
                    {
                        WaveParserParams_p->ParserState=ES_REJECT_DATA;
                        WaveParserParams_p->Get_Block = FALSE;
                        break;

                    }
                    else
                    {
                        WaveParserParams_p->ParserState=ES_WAVE_COM_COEFF_EXT1;
                        break;

                    }
                }

                else
                {
                    BytesPerBlock = ((WaveParserParams_p->SamplesPerBlock+14)/8)*(4*WaveParserParams_p->Number_Of_Channels);

                    if((BytesPerBlock > WaveParserParams_p->Block_Align) || WaveParserParams_p->SamplesPerBlock%8 !=1)
                    {
                        WaveParserParams_p->ParserState=ES_REJECT_DATA;
                        WaveParserParams_p->Get_Block = FALSE;
                        break;

                    }
                    else
                    {
                        WaveParserParams_p->ParserState=ES_WAVE_Parse_Data_D;
                        break;

                    }
                }
            }


        case ES_WAVE_COM_COEFF_EXT1:
            WaveParserParams_p->NumberOfCoeffs = value;
            WaveParserParams_p->ParserState=ES_WAVE_COM_COEFF_EXT2;
            offset++;
            break;

        case ES_WAVE_COM_COEFF_EXT2:
            WaveParserParams_p->NumberOfCoeffs |= value<<8;
            WaveParserParams_p->ParserState=ES_WAVE_COM_SKIP_COEFF;
            offset++;
            break;

        case ES_WAVE_COM_SKIP_COEFF:
            if(WaveParserParams_p->NumberOfCoeffs <7 || WaveParserParams_p->NumberOfCoeffs >0x100)
            {
                WaveParserParams_p->ParserState=ES_REJECT_DATA;
                WaveParserParams_p->Get_Block = FALSE;
                break;
            }
            else//SKIP THE COEFF VALUES AS DECODER USES HARD CODED VALUES
            {
                WaveParserParams_p->BytesToSkip= 2*2*WaveParserParams_p->NumberOfCoeffs;
                WaveParserParams_p->ParserState=ES_WAVE_COM_SKIP_BYTES;
                break;
            }


        case ES_WAVE_COM_SKIP_BYTES://SKIPPING THE COEFFS, IN FUTURE IF COEFFS ARE NEEDED EXTRACT HERE
            if(WaveParserParams_p->BytesToSkip !=0)
            {
                WaveParserParams_p->BytesToSkip --;
                offset++;
                WaveParserParams_p->ParserState=ES_WAVE_COM_SKIP_BYTES;
                break;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Data_D;
                offset++;
                break;
            }

        case ES_WAVE_Parse_Data_D:
        if(value==0x64)
        {
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Data_A0;
        }
        offset++;
        break;

        case ES_WAVE_Parse_Data_A0:
            if(value==0x61)
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Data_T;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Data_D;
            }
            break;

        case ES_WAVE_Parse_Data_T:
            if(value==0x74)
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Data_A1;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Data_D;
            }
            break;

        case ES_WAVE_Parse_Data_A1:
            if(value==0x61)
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Chunk_Size_00;
                offset++;
            }
            else
            {
                WaveParserParams_p->ParserState=ES_WAVE_Parse_Data_D;
            }
            break;

            // Parse Chunk Size for the data
        case ES_WAVE_Parse_Chunk_Size_00:
            WaveParserParams_p->Chunk_Size=value;
            WaveParserParams_p->Chunk_Size = WaveParserParams_p->Chunk_Size & 0xffff00ff;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Chunk_Size_01;
            offset++;
            break;

        case ES_WAVE_Parse_Chunk_Size_01:
            WaveParserParams_p->Chunk_Size |=value<<8;
            WaveParserParams_p->Chunk_Size = WaveParserParams_p->Chunk_Size & 0xff00ffff;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Chunk_Size_02;
            offset++;
            break;

        case ES_WAVE_Parse_Chunk_Size_02:
            WaveParserParams_p->Chunk_Size |=value<<16;
            WaveParserParams_p->Chunk_Size = WaveParserParams_p->Chunk_Size & 0x00ffffff;
            WaveParserParams_p->ParserState=ES_WAVE_Parse_Chunk_Size_03;
            offset++;
            break;

        case ES_WAVE_Parse_Chunk_Size_03:
            WaveParserParams_p->Chunk_Size |=value<<24;
            WaveParserParams_p->AppData_p->ChangeSet = TRUE;
            WaveParserParams_p->AppDataFlag = TRUE;
            WaveParserParams_p->ParserState=ES_WAVE_COMPUTE_FRAME_LENGTH;
            offset++;
            break;

        case ES_WAVE_COMPUTE_FRAME_LENGTH:

            if(WaveParserParams_p->Compression_Code==1)
            {
                WaveParserParams_p->FrameSize    = (WaveParserParams_p->Significant_Bits_Per_Sample/8)*WaveParserParams_p->Number_Of_Channels* (CDDA_NB_SAMPLES << 1); // CHECK AGAIN

                switch (WaveParserParams_p->Significant_Bits_Per_Sample)
                {
                case 8:
                    WaveParserParams_p->AppData_p->sampleBits            = ACC_LPCM_WS8u;
                    break;

                case 16:
                    WaveParserParams_p->AppData_p->sampleBits            = ACC_LPCM_WS16le;
                    break;

                case 24:
                case 32:
                    WaveParserParams_p->AppData_p->sampleBits            = ACC_LPCM_WS32;
                    break;

                default:
                    WaveParserParams_p->AppData_p->sampleBits            = ACC_LPCM_WS16le;
                    break;
                }
            }
            else
            {
                WaveParserParams_p->FrameSize                 = WaveParserParams_p->Block_Align;
                WaveParserParams_p->AppData_p->bitShiftGroup2 = WaveParserParams_p->NumberOfCoeffs;
                WaveParserParams_p->AppData_p->sampleBits     = ACC_LPCM_WS16;
                WaveParserParams_p->Significant_Bits_Per_Sample = 16;

                if(WaveParserParams_p->Compression_Code==17)
                {
                    WaveParserParams_p->AppData_p->NbSamples = ImaAdpcmSamplesIn(0,WaveParserParams_p->Number_Of_Channels,WaveParserParams_p->Block_Align,0);
                }
                else
                {
                    WaveParserParams_p->AppData_p->NbSamples = AdpcmSamplesIn(0,WaveParserParams_p->Number_Of_Channels,WaveParserParams_p->Block_Align,0);
                }

            }

            WaveParserParams_p->AppData_p->mode              = WaveParserParams_p->Compression_Code;
            WaveParserParams_p->AppData_p->sampleRate        = WaveParserParams_p->Sample_Rate;
            WaveParserParams_p->AppData_p->channels          = WaveParserParams_p->Number_Of_Channels;
            WaveParserParams_p->AppData_p->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
            WaveParserParams_p->OpBlk_p->AppData_p           = WaveParserParams_p->AppData_p;
            WaveParserParams_p->NoBytesRemainingInFrame      =WaveParserParams_p->FrameSize;

            WaveParserParams_p->ParserState                  = ES_WAVE_FRAME;

    case ES_WAVE_FRAME:
        if(WaveParserParams_p->Chunk_Size != 0)
        {
            U32 DataAvailable = Size-offset;
            U32 DataToCopy = DataAvailable;

            if (WaveParserParams_p->NoBytesRemainingInFrame < WaveParserParams_p->Chunk_Size)
            {
                if(DataAvailable > WaveParserParams_p->NoBytesRemainingInFrame)
                {
                    DataToCopy = WaveParserParams_p->NoBytesRemainingInFrame;
                }
            }
            else
            {
                if(DataAvailable > WaveParserParams_p->Chunk_Size)
                {
                    DataToCopy = WaveParserParams_p->Chunk_Size;
                }
            }

            if((WaveParserParams_p->Significant_Bits_Per_Sample==24))
            {
                if(DataToCopy > WaveParserParams_p->SpaceInBuffer)
                    DataToCopy = WaveParserParams_p->SpaceInBuffer;
            }

            if(DataToCopy)
            {
                switch (WaveParserParams_p->Significant_Bits_Per_Sample)
                {
                case 24:
                    memcpy(((char *)WaveParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress+ (MAXBUFFSIZE - WaveParserParams_p->SpaceInBuffer)),((char *)MemoryStart+offset),DataToCopy);

                    WaveParserParams_p->Chunk_Size -= DataToCopy;
                    offset += DataToCopy;

                    WaveParserParams_p->NoBytesRemainingInFrame -= DataToCopy;

                    WaveParserParams_p->SpaceInBuffer -= DataToCopy;

                    if((WaveParserParams_p->SpaceInBuffer == 0) || (WaveParserParams_p->Chunk_Size==0))
                    {
                        WaveParserParams_p->node_p->Node.DestinationAddress_p = (void *)(STOS_AddressTranslate(WaveParserParams_p->ESBufferAddress.BasePhyAddress, WaveParserParams_p->ESBufferAddress.BaseUnCachedVirtualAddress,  ((U32)(WaveParserParams_p->Current_Write_Ptr1)+1)));
                        WaveParserParams_p->node_p->Node.SourceAddress_p = (void *)(STOS_AddressTranslate(WaveParserParams_p->DummyBufferAddress.BasePhyAddress,WaveParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress,WaveParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress ));

                        //STTBX_Print(("\nEntering generic transfer"));

                        if(WaveParserParams_p->Frequency < 96000)
                        {
                            FDMAMemcpy(WaveParserParams_p->node_p->Node.DestinationAddress_p, WaveParserParams_p->node_p->Node.SourceAddress_p,3,MAXBUFFSIZE,3,4,&WaveParserParams_p->FDMAnodeAddress);
                        }
                        else
                        {
                            U32 num = MAXBUFFSIZE/3;
                            U32 i=0;
                            U8 *src,*dst;

                            src = (U8 *)WaveParserParams_p->DummyBufferAddress.BaseUnCachedVirtualAddress;
                            dst = (U8 *)((U32)(WaveParserParams_p->Current_Write_Ptr1)+1);
                            for (i=0;i< num;i++)
                            {
                                *dst++ = *src++;
                                *dst++ = *src++;
                                *dst++ = *src++;
                                dst++;
                            }
                        }

                        if(WaveParserParams_p->SpaceInBuffer == 0)
                        {
                            WaveParserParams_p->SpaceInBuffer = MAXBUFFSIZE;
                        }

                        if(WaveParserParams_p->Chunk_Size!=0)
                        {
                            WaveParserParams_p->OpBlk_p->Size =OPBLK_SIZE_24BIT ; // = 3528 * (4/3) FOR EVERY 3 BYTES WE COPY 4 BYTES
                        }
                    }

                    break;
                case 8:
                case 16:
                case 32:
                case 4:
                    memcpy(WaveParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),DataToCopy);
                    WaveParserParams_p->Chunk_Size -= DataToCopy;
                    offset += DataToCopy;
                    WaveParserParams_p->Current_Write_Ptr1 += DataToCopy;
                    WaveParserParams_p->NoBytesRemainingInFrame -= DataToCopy;
                    WaveParserParams_p->OpBlk_p->Size += DataToCopy;

                    break;

                default :
                    break;
                }
            }


            if((WaveParserParams_p->Chunk_Size == 0) || (WaveParserParams_p->NoBytesRemainingInFrame == 0))
            {
                if(WaveParserParams_p->Chunk_Size == 0)
                {
                    WaveParserParams_p->ParserState = ES_WAVE_R;

                    if(WaveParserParams_p->Significant_Bits_Per_Sample == 24)
                    WaveParserParams_p->OpBlk_p->Size =(((MAXBUFFSIZE-WaveParserParams_p->SpaceInBuffer)*4)/3);
                }

                WaveParserParams_p->NoBytesRemainingInFrame= WaveParserParams_p->FrameSize;

                if(WaveParserParams_p->Get_Block == TRUE)
                {
                    Error = MemPool_PutOpBlk(WaveParserParams_p->OpBufHandle,(U32 *)&WaveParserParams_p->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" ES_Wave Error in Memory Put_Block \n"));
                        return Error;
                    }
                    else
                    {
                        STTBX_Print((" WAVE Deli:%d\n",WaveParserParams_p->Blk_Out));
                        WaveParserParams_p->Blk_Out++;
                        WaveParserParams_p->Put_Block = TRUE;
                        WaveParserParams_p->Get_Block = FALSE;
                    }
                }
            }
        }

        break;

    case ES_REJECT_DATA:
            offset = Size;

    default:
            break;

        }
    }


    *No_Bytes_Used=offset;
    return ST_NO_ERROR;
}

/************************************************************************************
Name         : ES_Wave_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/



ST_ErrorCode_t ES_Wave_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_Wave_ParserLocalParams_t *WaveParserParams_p;
    WaveParserParams_p=(ES_Wave_ParserLocalParams_t *)Handle;

    if(WaveParserParams_p->Get_Block == TRUE)
    {
        if((WaveParserParams_p->Significant_Bits_Per_Sample==8) || (WaveParserParams_p->Significant_Bits_Per_Sample==16) ||(WaveParserParams_p->Significant_Bits_Per_Sample==32))
        WaveParserParams_p->OpBlk_p->Size = WaveParserParams_p->FrameSize ;

        if(WaveParserParams_p->Significant_Bits_Per_Sample==24)
        WaveParserParams_p->OpBlk_p->Size = OPBLK_SIZE_24BIT ;

        Error = MemPool_PutOpBlk(WaveParserParams_p->OpBufHandle,(U32 *)&WaveParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_Wave Error in Memory Put_Block \n"));
        }
        else
        {
            WaveParserParams_p->Put_Block = TRUE;
            WaveParserParams_p->Get_Block = FALSE;
        }
    }
    WaveParserParams_p->Blk_Out = 0;
    WaveParserParams_p->ParserState = ES_WAVE_R;
    WaveParserParams_p->Frequency = 0xFFFFFFFF;
    return Error;
}


/************************************************************************************
Name         : ES_Wave_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_Wave_Parser_Start(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_Wave_ParserLocalParams_t *WaveParserParams_p;
    WaveParserParams_p=(ES_Wave_ParserLocalParams_t *)Handle;

    WaveParserParams_p->ParserState = ES_WAVE_R;
    WaveParserParams_p->Frequency = 0xFFFFFFFF;

    WaveParserParams_p->Put_Block = TRUE;
    WaveParserParams_p->Get_Block = FALSE;
    WaveParserParams_p->Blk_Out = 0;

    return Error;
}

#endif


