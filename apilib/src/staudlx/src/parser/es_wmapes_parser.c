/********************************************************************************
 *   COPYRIGHT STMicroelectronics (C) 2006                                      *
 *   Filename       : es_wmapes_parser.c                                        *
 *   Start          : 21.07.2006                                                *
 *   By             : ACF Noida                                                 *
 *   Description    : The file contains the parser for the WMA and WMA Pro That *
 *                    comes with PES MP2                                        *
 ********************************************************************************
*/

/*****************************************************************************
Includes
*****************************************************************************/
#if defined(STAUD_INSTALL_WMA) || defined(STAUD_INSTALL_WMAPROLSL)

#include "es_wmapes_parser.h"
#include "stos.h"
#include "aud_evt.h"
/************************************************************************************
Name         : ES_WMAPES_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
           ES_WMAPES_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_WMAPES_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_WMAPES_ParserLocalParams_t * WMAPESParserParams_p;

    WMAPESParserParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(ES_WMAPES_ParserLocalParams_t));
    if(WMAPESParserParams_p==NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(WMAPESParserParams_p,0,sizeof(ES_WMAPES_ParserLocalParams_t));

    WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_A;
    WMAPESParserParams_p->FrameComplete = TRUE;
    WMAPESParserParams_p->FrameSize = 0;

    WMAPESParserParams_p->Get_Block = FALSE;
    WMAPESParserParams_p->Put_Block = TRUE;
    WMAPESParserParams_p->OpBufHandle = Init_p->OpBufHandle;

    WMAPESParserParams_p->EvtHandle       = Init_p->EvtHandle;
    WMAPESParserParams_p->ObjectID        = Init_p->ObjectID;
    WMAPESParserParams_p->EventIDNewFrame = Init_p->EventIDNewFrame;

    WMAPESParserParams_p->AudioParserMemory_p = Init_p->Memory_Partition;
    *handle=(ParserHandle_t )WMAPESParserParams_p;
    /* to init O/P pointers */

    WMAPESParserParams_p->nVersion = 0;
    WMAPESParserParams_p->wFormatTag = 0;
    WMAPESParserParams_p->nSamplesPerSec= 0;
    WMAPESParserParams_p->nAvgBytesPerSec= 0;
    WMAPESParserParams_p->nBlockAlign= 0;
    WMAPESParserParams_p->nChannels= 0;
    WMAPESParserParams_p->nEncodeOpt= 0;
    WMAPESParserParams_p->nSamplesPerBlock= 0;
    WMAPESParserParams_p->dwChannelMask= 0;
    WMAPESParserParams_p->nBitsPerSample= 0;
    WMAPESParserParams_p->wValidBitsPerSample= 0;
    WMAPESParserParams_p->wStreamId= 0;
    WMAPESParserParams_p->iPeakAmplitudeRef= 0;
    WMAPESParserParams_p->iRmsAmplitudeRef= 0;
    WMAPESParserParams_p->iPeakAmplitudeTarget= 0;
    WMAPESParserParams_p->iRmsAmplitudeTarget= 0;


    WMAPESParserParams_p->local_nVersion = 0;
    WMAPESParserParams_p->local_wFormatTag = 0;
    WMAPESParserParams_p->local_nSamplesPerSec = 0;
    WMAPESParserParams_p->local_nAvgBytesPerSec = 0;
    WMAPESParserParams_p->local_nBlockAlign = 0;
    WMAPESParserParams_p->local_nChannels = 0;
    WMAPESParserParams_p->local_nEncodeOpt = 0;
    WMAPESParserParams_p->local_nSamplesPerBlock = 0;
    WMAPESParserParams_p->local_dwChannelMask = 0;
    WMAPESParserParams_p->local_nBitsPerSample = 0;
    WMAPESParserParams_p->local_wValidBitsPerSample = 0;
    WMAPESParserParams_p->local_wStreamId = 0;
    WMAPESParserParams_p->local_iPeakAmplitudeRef = 0;
    WMAPESParserParams_p->local_iRmsAmplitudeRef = 0;
    WMAPESParserParams_p->local_iPeakAmplitudeTarget = 0;
    WMAPESParserParams_p->local_iRmsAmplitudeTarget = 0;
    return (Error);
}

ST_ErrorCode_t ES_WMAPES_ParserGetParsingFunction(  ParsingFunction_t * ParsingFunc_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p)
{

    *ParsingFunc_p      = (ParsingFunction_t)ES_WMAPES_Parse_Frame;
    *GetFreqFunction_p  = (GetFreqFunction_t)ES_WMAPES_Get_Freq;
    *GetInfoFunction_p  = (GetInfoFunction_t)ES_WMAPES_Get_Info;

    return ST_NO_ERROR;
}




/************************************************************************************
Name         : ES_WMAPES_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_WMAPES_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_WMAPES_ParserLocalParams_t * WMAPESParserParams_p =(ES_WMAPES_ParserLocalParams_t *)Handle;

    if(WMAPESParserParams_p->Get_Block == TRUE)
    {
        WMAPESParserParams_p->OpBlk_p->Size = WMAPESParserParams_p->FrameSize;
        WMAPESParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(WMAPESParserParams_p->OpBufHandle,(U32 *)&WMAPESParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_WMAPES Error in Memory Put_Block \n"));
        }
        else
        {
            WMAPESParserParams_p->Put_Block = TRUE;
            WMAPESParserParams_p->Get_Block = FALSE;
        }
    }

    WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_A;

    WMAPESParserParams_p->FrameComplete = TRUE;

    WMAPESParserParams_p->nVersion = 0;
    WMAPESParserParams_p->wFormatTag = 0;
    WMAPESParserParams_p->nSamplesPerSec= 0;
    WMAPESParserParams_p->nAvgBytesPerSec= 0;
    WMAPESParserParams_p->nBlockAlign= 0;
    WMAPESParserParams_p->nChannels= 0;
    WMAPESParserParams_p->nEncodeOpt= 0;
    WMAPESParserParams_p->nSamplesPerBlock= 0;
    WMAPESParserParams_p->dwChannelMask= 0;
    WMAPESParserParams_p->nBitsPerSample= 0;
    WMAPESParserParams_p->wValidBitsPerSample= 0;
    WMAPESParserParams_p->wStreamId= 0;
    WMAPESParserParams_p->iPeakAmplitudeRef= 0;
    WMAPESParserParams_p->iRmsAmplitudeRef= 0;
    WMAPESParserParams_p->iPeakAmplitudeTarget= 0;
    WMAPESParserParams_p->iRmsAmplitudeTarget= 0;

    WMAPESParserParams_p->local_nVersion = 0;
    WMAPESParserParams_p->local_wFormatTag = 0;
    WMAPESParserParams_p->local_nSamplesPerSec = 0;
    WMAPESParserParams_p->local_nAvgBytesPerSec = 0;
    WMAPESParserParams_p->local_nBlockAlign = 0;
    WMAPESParserParams_p->local_nChannels = 0;
    WMAPESParserParams_p->local_nEncodeOpt = 0;
    WMAPESParserParams_p->local_nSamplesPerBlock = 0;
    WMAPESParserParams_p->local_dwChannelMask = 0;
    WMAPESParserParams_p->local_nBitsPerSample = 0;
    WMAPESParserParams_p->local_wValidBitsPerSample = 0;
    WMAPESParserParams_p->local_wStreamId = 0;
    WMAPESParserParams_p->local_iPeakAmplitudeRef = 0;
    WMAPESParserParams_p->local_iRmsAmplitudeRef = 0;
    WMAPESParserParams_p->local_iPeakAmplitudeTarget = 0;
    WMAPESParserParams_p->local_iRmsAmplitudeTarget = 0;
    return Error;
}

/************************************************************************************
Name         : ES_WMAPES_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_WMAPES_Parser_Start(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_WMAPES_ParserLocalParams_t * WMAPESParserParams_p = (ES_WMAPES_ParserLocalParams_t *)Handle;

    WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_A;
    WMAPESParserParams_p->FrameComplete = TRUE;
    WMAPESParserParams_p->Put_Block = TRUE;
    WMAPESParserParams_p->Get_Block = FALSE;

    WMAPESParserParams_p->nVersion = 0;
    WMAPESParserParams_p->wFormatTag = 0;
    WMAPESParserParams_p->nSamplesPerSec= 0;
    WMAPESParserParams_p->nAvgBytesPerSec= 0;
    WMAPESParserParams_p->nBlockAlign= 0;
    WMAPESParserParams_p->nChannels= 0;
    WMAPESParserParams_p->nEncodeOpt= 0;
    WMAPESParserParams_p->nSamplesPerBlock= 0;
    WMAPESParserParams_p->dwChannelMask= 0;
    WMAPESParserParams_p->nBitsPerSample= 0;
    WMAPESParserParams_p->wValidBitsPerSample= 0;
    WMAPESParserParams_p->wStreamId= 0;
    WMAPESParserParams_p->iPeakAmplitudeRef= 0;
    WMAPESParserParams_p->iRmsAmplitudeRef= 0;
    WMAPESParserParams_p->iPeakAmplitudeTarget= 0;
    WMAPESParserParams_p->iRmsAmplitudeTarget= 0;


    WMAPESParserParams_p->local_nVersion = 0;
    WMAPESParserParams_p->local_wFormatTag = 0;
    WMAPESParserParams_p->local_nSamplesPerSec = 0;
    WMAPESParserParams_p->local_nAvgBytesPerSec = 0;
    WMAPESParserParams_p->local_nBlockAlign = 0;
    WMAPESParserParams_p->local_nChannels = 0;
    WMAPESParserParams_p->local_nEncodeOpt = 0;
    WMAPESParserParams_p->local_nSamplesPerBlock = 0;
    WMAPESParserParams_p->local_dwChannelMask = 0;
    WMAPESParserParams_p->local_nBitsPerSample = 0;
    WMAPESParserParams_p->local_wValidBitsPerSample = 0;
    WMAPESParserParams_p->local_wStreamId = 0;
    WMAPESParserParams_p->local_iPeakAmplitudeRef = 0;
    WMAPESParserParams_p->local_iRmsAmplitudeRef = 0;
    WMAPESParserParams_p->local_iPeakAmplitudeTarget = 0;
    WMAPESParserParams_p->local_iRmsAmplitudeTarget = 0;
    return Error;

}


/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_WMAPES_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_WMAPES_ParserLocalParams_t * WMAPESParserParams_p =(ES_WMAPES_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p=WMAPESParserParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_WMAPES_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_WMAPES_ParserLocalParams_t * WMAPESParserParams_p = (ES_WMAPES_ParserLocalParams_t *)Handle;

    StreamInfo->SamplingFrequency   = WMAPESParserParams_p->Frequency;
    return ST_NO_ERROR;
}

/* Handle EOF from Parser */

ST_ErrorCode_t ES_WMAPES_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_WMAPES_ParserLocalParams_t * WMAPESParserParams_p = (ES_WMAPES_ParserLocalParams_t *)Handle;

    if(WMAPESParserParams_p->Get_Block == TRUE)
    {

        WMAPESParserParams_p->Current_Write_Ptr1 += WMAPESParserParams_p->NoBytesRemainingInFrame;

        //sending last dummy buffer
        WMAPESParserParams_p->OpBlk_p->Size += WMAPESParserParams_p->nBlockAlign;
        memset((char*)WMAPESParserParams_p->OpBlk_p->MemoryStart,0,WMAPESParserParams_p->OpBlk_p->Size);

        WMAPESParserParams_p->OpBlk_p->Data[WMA_LAST_DATA_BLOCK_OFFSET] = 1;
        WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_LAST_DATA_BLOCK_VALID;

        Error = MemPool_PutOpBlk(WMAPESParserParams_p->OpBufHandle,(U32 *)&WMAPESParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_WMA Error in Memory Put_Block \n"));
            return Error;
        }
        else
        {
            WMAPESParserParams_p->Put_Block = TRUE;
            WMAPESParserParams_p->Get_Block = FALSE;
        }

        /*
        memset(WMAPESParserParams_p->Current_Write_Ptr1,0x00,WMAPESParserParams_p->NoBytesRemainingInFrame);

        WMAPESParserParams_p->OpBlk_p->Size = WMAPESParserParams_p->FrameSize;
        WMAPESParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(WMAPESParserParams_p->OpBufHandle,(U32 *)&WMAPESParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
        }
        else
        {
            WMAPESParserParams_p->Put_Block = TRUE;
            WMAPESParserParams_p->Get_Block = FALSE;
        }
        */
    }

    return ST_NO_ERROR;
}



/************************************************************************************
Name         : Parse_WMAPES_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t ES_WMAPES_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info)
{
    U8 *pos;
    U32 offset;
    U8 value;
    ST_ErrorCode_t Error;
    STAUD_PTS_t NotifyPTS;
    ES_WMAPES_ParserLocalParams_t * WMAPESParserParams_p = (ES_WMAPES_ParserLocalParams_t *)Handle;

    offset = *No_Bytes_Used;
    pos = (U8 *)MemoryStart;

    while(offset<Size)
    {
        if (WMAPESParserParams_p->FrameComplete == TRUE)
        {

            if(WMAPESParserParams_p->Put_Block == TRUE)
            {
                Error = MemPool_GetOpBlk(WMAPESParserParams_p->OpBufHandle, (U32 *)&WMAPESParserParams_p->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    *No_Bytes_Used = offset;
                    return STAUD_MEMORY_GET_ERROR;
                }
                else
                {
                    WMAPESParserParams_p->Put_Block = FALSE;
                    WMAPESParserParams_p->Get_Block = TRUE;
                    WMAPESParserParams_p->Current_Write_Ptr1 = (U8 *)WMAPESParserParams_p->OpBlk_p->MemoryStart;
                    WMAPESParserParams_p->OpBlk_p->Size = 0;
                    WMAPESParserParams_p->OpBlk_p->Flags = 0;
                    if(CurrentPTS_p->PTSValidFlag == TRUE)
                    {
                        WMAPESParserParams_p->OpBlk_p->Flags   |= PTS_VALID;
                        WMAPESParserParams_p->OpBlk_p->Data[PTS_OFFSET]  = CurrentPTS_p->PTS;
                        WMAPESParserParams_p->OpBlk_p->Data[PTS33_OFFSET] = CurrentPTS_p->PTS33Bit;
                        CurrentPTS_p->PTSValidFlag = FALSE;
                    }

                }
            }

            WMAPESParserParams_p->FrameComplete = FALSE;
        }

        value=pos[offset];

        switch(WMAPESParserParams_p->WMAPESParserState)
        {

        case ES_WMAPES_PRIVATE_DATA_HEADER_A:
            if(value == 0x41)  //"A"
            {
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_S;
            }
            offset++;
            break;

        case ES_WMAPES_PRIVATE_DATA_HEADER_S:
            if(value == 0x53)  //"S"
            {
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_F;
                offset++;
            }
            else
            {
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_A;
            }
            break;

        case ES_WMAPES_PRIVATE_DATA_HEADER_F:
            if(value == 0x46)  //"F"
            {
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_LENGTH;
                offset++;
            }
            else
            {
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_A;
            }
            break;

        case ES_WMAPES_PRIVATE_DATA_HEADER_LENGTH:

            WMAPESParserParams_p->FrameSize = ((Private_Info->Private_Data[4]<<8)+Private_Info->Private_Data[5]) - 4 - value;

            if(value == 52) // In current design it will be 0 or 52(52 in case of New ASF Heaser)
            {
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PARSE_ASF_HEADER_nVersion_0;
                WMAPESParserParams_p->local_nVersion = WMAPESParserParams_p->nVersion;
                WMAPESParserParams_p->local_wFormatTag = WMAPESParserParams_p->wFormatTag;
                WMAPESParserParams_p->local_nSamplesPerSec = WMAPESParserParams_p->nSamplesPerSec;
                WMAPESParserParams_p->local_nAvgBytesPerSec = WMAPESParserParams_p->nAvgBytesPerSec;
                WMAPESParserParams_p->local_nBlockAlign = WMAPESParserParams_p->nBlockAlign;
                WMAPESParserParams_p->local_nChannels = WMAPESParserParams_p->nChannels;
                WMAPESParserParams_p->local_nEncodeOpt = WMAPESParserParams_p->nEncodeOpt;
                WMAPESParserParams_p->local_nSamplesPerBlock = WMAPESParserParams_p->nSamplesPerBlock;
                WMAPESParserParams_p->local_dwChannelMask = WMAPESParserParams_p->dwChannelMask;
                WMAPESParserParams_p->local_nBitsPerSample = WMAPESParserParams_p->nBitsPerSample;
                WMAPESParserParams_p->local_wValidBitsPerSample = WMAPESParserParams_p->wValidBitsPerSample;
                WMAPESParserParams_p->local_wStreamId = WMAPESParserParams_p->wStreamId;
                WMAPESParserParams_p->local_iPeakAmplitudeRef = WMAPESParserParams_p->iPeakAmplitudeRef;
                WMAPESParserParams_p->local_iRmsAmplitudeRef = WMAPESParserParams_p->iRmsAmplitudeRef;
                WMAPESParserParams_p->local_iPeakAmplitudeTarget = WMAPESParserParams_p->iPeakAmplitudeTarget;
                WMAPESParserParams_p->local_iRmsAmplitudeTarget = WMAPESParserParams_p->iRmsAmplitudeTarget;
            }
            else if(value == 0)
            {
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_DELIVER_PAYLOAD_DATA;
            }
            else
            {
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_A; // Error in ASF
            }
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nVersion_0:

            WMAPESParserParams_p->nVersion = value;
            WMAPESParserParams_p->WMAPESParserState   = ES_WMAPES_PARSE_ASF_HEADER_nVersion_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nVersion_1:

            WMAPESParserParams_p->nVersion = (WMAPESParserParams_p->nVersion<<8)+value;
            WMAPESParserParams_p->WMAPESParserState   = ES_WMAPES_PARSE_ASF_HEADER_wFormatTag_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_wFormatTag_0:

            WMAPESParserParams_p->wFormatTag = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_wFormatTag_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_wFormatTag_1:

            WMAPESParserParams_p->wFormatTag = (WMAPESParserParams_p->wFormatTag<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_0:

            WMAPESParserParams_p->nSamplesPerSec = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_1:

            WMAPESParserParams_p->nSamplesPerSec = (WMAPESParserParams_p->nSamplesPerSec<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_2:

            WMAPESParserParams_p->nSamplesPerSec = (WMAPESParserParams_p->nSamplesPerSec<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerSec_3:

            WMAPESParserParams_p->nSamplesPerSec = (WMAPESParserParams_p->nSamplesPerSec<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_0:

            WMAPESParserParams_p->nAvgBytesPerSec = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_1:

            WMAPESParserParams_p->nAvgBytesPerSec = (WMAPESParserParams_p->nAvgBytesPerSec<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_2:

            WMAPESParserParams_p->nAvgBytesPerSec = (WMAPESParserParams_p->nAvgBytesPerSec<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nAvgBytesPerSec_3:

            WMAPESParserParams_p->nAvgBytesPerSec = (WMAPESParserParams_p->nAvgBytesPerSec<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_0:

            WMAPESParserParams_p->nBlockAlign = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_1:

            WMAPESParserParams_p->nBlockAlign = (WMAPESParserParams_p->nBlockAlign<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_2:

            WMAPESParserParams_p->nBlockAlign = (WMAPESParserParams_p->nBlockAlign<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nBlockAlign_3:

            WMAPESParserParams_p->nBlockAlign = (WMAPESParserParams_p->nBlockAlign<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nChannels_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nChannels_0:

            WMAPESParserParams_p->nChannels = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nChannels_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nChannels_1:

            WMAPESParserParams_p->nChannels = (WMAPESParserParams_p->nChannels<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nEncodeOpt_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nEncodeOpt_0:

            WMAPESParserParams_p->nEncodeOpt = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nEncodeOpt_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nEncodeOpt_1:

            WMAPESParserParams_p->nEncodeOpt = (WMAPESParserParams_p->nEncodeOpt<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_0:

            WMAPESParserParams_p->nSamplesPerBlock = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_1:

            WMAPESParserParams_p->nSamplesPerBlock = (WMAPESParserParams_p->nSamplesPerBlock<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_2:

            WMAPESParserParams_p->nSamplesPerBlock = (WMAPESParserParams_p->nSamplesPerBlock<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nSamplesPerBlock_3:

            WMAPESParserParams_p->nSamplesPerBlock = (WMAPESParserParams_p->nSamplesPerBlock<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_0:

            WMAPESParserParams_p->dwChannelMask = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_1:

            WMAPESParserParams_p->dwChannelMask = (WMAPESParserParams_p->dwChannelMask<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_2:

            WMAPESParserParams_p->dwChannelMask = (WMAPESParserParams_p->dwChannelMask<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_dwChannelMask_3:

            WMAPESParserParams_p->dwChannelMask = (WMAPESParserParams_p->dwChannelMask<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_0:

            WMAPESParserParams_p->nBitsPerSample = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_1:

            WMAPESParserParams_p->nBitsPerSample = (WMAPESParserParams_p->nBitsPerSample<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_2:

            WMAPESParserParams_p->nBitsPerSample = (WMAPESParserParams_p->nBitsPerSample<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_nBitsPerSample_3:

            WMAPESParserParams_p->nBitsPerSample = (WMAPESParserParams_p->nBitsPerSample<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_wValidBitsPerSample_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_wValidBitsPerSample_0:

            WMAPESParserParams_p->wValidBitsPerSample = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_wValidBitsPerSample_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_wValidBitsPerSample_1:

            WMAPESParserParams_p->wValidBitsPerSample = (WMAPESParserParams_p->wValidBitsPerSample<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_wStreamId_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_wStreamId_0:

            WMAPESParserParams_p->wStreamId = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_wStreamId_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_wStreamId_1:

            WMAPESParserParams_p->wStreamId = (WMAPESParserParams_p->wStreamId<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_0:

            WMAPESParserParams_p->iPeakAmplitudeRef = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_1:

            WMAPESParserParams_p->iPeakAmplitudeRef = (WMAPESParserParams_p->iPeakAmplitudeRef<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_2:

            WMAPESParserParams_p->iPeakAmplitudeRef = (WMAPESParserParams_p->iPeakAmplitudeRef<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeRef_3:

            WMAPESParserParams_p->iPeakAmplitudeRef = (WMAPESParserParams_p->iPeakAmplitudeRef<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_0;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_0:

            WMAPESParserParams_p->iRmsAmplitudeRef = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_1:

            WMAPESParserParams_p->iRmsAmplitudeRef = (WMAPESParserParams_p->iRmsAmplitudeRef<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_2:

            WMAPESParserParams_p->iRmsAmplitudeRef = (WMAPESParserParams_p->iRmsAmplitudeRef<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeRef_3:

            WMAPESParserParams_p->iRmsAmplitudeRef = (WMAPESParserParams_p->iRmsAmplitudeRef<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_0;
            offset++;
            break;


        case ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_0:

            WMAPESParserParams_p->iPeakAmplitudeTarget = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_1:

            WMAPESParserParams_p->iPeakAmplitudeTarget = (WMAPESParserParams_p->iPeakAmplitudeTarget<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_2:

            WMAPESParserParams_p->iPeakAmplitudeTarget = (WMAPESParserParams_p->iPeakAmplitudeTarget<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iPeakAmplitudeTarget_3:

            WMAPESParserParams_p->iPeakAmplitudeTarget = (WMAPESParserParams_p->iPeakAmplitudeTarget<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_0;
            offset++;
            break;


        case ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_0:

            WMAPESParserParams_p->iRmsAmplitudeTarget = value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_1;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_1:

            WMAPESParserParams_p->iRmsAmplitudeTarget = (WMAPESParserParams_p->iRmsAmplitudeTarget<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_2;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_2:

            WMAPESParserParams_p->iRmsAmplitudeTarget = (WMAPESParserParams_p->iRmsAmplitudeTarget<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_3;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_iRmsAmplitudeTarget_3:

            WMAPESParserParams_p->iRmsAmplitudeTarget = (WMAPESParserParams_p->iRmsAmplitudeTarget<<8)+value;
            WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_PARSE_ASF_HEADER_SET_IN_DATA_BLOCK;
            offset++;
            break;

        case ES_WMAPES_PARSE_ASF_HEADER_SET_IN_DATA_BLOCK:
            {
                BOOL streamParamsChanged = TRUE;
                if (
                (WMAPESParserParams_p->local_nVersion == WMAPESParserParams_p->nVersion)&&
                (WMAPESParserParams_p->local_wFormatTag == WMAPESParserParams_p->wFormatTag)&&
                (WMAPESParserParams_p->local_nSamplesPerSec == WMAPESParserParams_p->nSamplesPerSec)&&
                (WMAPESParserParams_p->local_nAvgBytesPerSec == WMAPESParserParams_p->nAvgBytesPerSec)&&
                (WMAPESParserParams_p->local_nBlockAlign == WMAPESParserParams_p->nBlockAlign)&&
                (WMAPESParserParams_p->local_nChannels == WMAPESParserParams_p->nChannels)&&
                (WMAPESParserParams_p->local_nEncodeOpt == WMAPESParserParams_p->nEncodeOpt)&&
                (WMAPESParserParams_p->local_nSamplesPerBlock == WMAPESParserParams_p->nSamplesPerBlock)&&
                (WMAPESParserParams_p->local_dwChannelMask == WMAPESParserParams_p->dwChannelMask)&&
                (WMAPESParserParams_p->local_nBitsPerSample == WMAPESParserParams_p->nBitsPerSample)&&
                (WMAPESParserParams_p->local_wValidBitsPerSample == WMAPESParserParams_p->wValidBitsPerSample)&&
                (WMAPESParserParams_p->local_wStreamId == WMAPESParserParams_p->wStreamId)&&
                (WMAPESParserParams_p->local_iPeakAmplitudeRef == WMAPESParserParams_p->iPeakAmplitudeRef)&&
                (WMAPESParserParams_p->local_iRmsAmplitudeRef == WMAPESParserParams_p->iRmsAmplitudeRef)&&
                (WMAPESParserParams_p->local_iPeakAmplitudeTarget == WMAPESParserParams_p->iPeakAmplitudeTarget)&&
                (WMAPESParserParams_p->local_iRmsAmplitudeTarget == WMAPESParserParams_p->iRmsAmplitudeTarget)
                )
                {

                    streamParamsChanged = FALSE;
                }

                if (streamParamsChanged)
                {
                    //setting the params
                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_VERSION_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_VERSION_OFFSET] = WMAPESParserParams_p->nVersion;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_FORMAT_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_FORMAT_OFFSET] = WMAPESParserParams_p->wFormatTag;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_SAMPLES_P_SEC_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_SAMPLES_P_SEC_OFFSET] = WMAPESParserParams_p->nSamplesPerSec;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_AVG_BYTES_P_SEC_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_AVG_BYTES_P_SEC_OFFSET] = WMAPESParserParams_p->nAvgBytesPerSec;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_BLK_ALIGN_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_BLK_ALIGN_OFFSET] = WMAPESParserParams_p->nBlockAlign;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_NUMBER_CHANNELS_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_NUMBER_CHANNELS_OFFSET] = WMAPESParserParams_p->nChannels;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_ENCODE_OPT_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_ENCODE_OPT_OFFSET] = WMAPESParserParams_p->nEncodeOpt;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_SAMPLES_P_BLK_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_SAMPLES_P_BLK_OFFSET] = WMAPESParserParams_p->nSamplesPerBlock;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_CHANNEL_MASK_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_CHANNEL_MASK_OFFSET] = WMAPESParserParams_p->dwChannelMask;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_BITS_P_SAMPLE_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_BITS_P_SAMPLE_OFFSET] = WMAPESParserParams_p->nBitsPerSample;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_VALID_BITS_P_SAMPLE_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_VALID_BITS_P_SAMPLE_OFFSET] = WMAPESParserParams_p->wValidBitsPerSample;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_STREAM_ID_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_STREAM_ID_OFFSET] = WMAPESParserParams_p->wStreamId;

                    /*WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_STREAM_PEAK_AMP_REF_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_STREAM_PEAK_AMP_REF_OFFSET] = WMAPESParserParams_p->iPeakAmplitudeRef;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_STREAM_RMS_AMP_REF_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_STREAM_RMS_AMP_REF_OFFSET] = WMAPESParserParams_p->iRmsAmplitudeRef;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_STREAM_PEAK_AMP_TARGET_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_STREAM_PEAK_AMP_TARGET_OFFSET] = WMAPESParserParams_p->iPeakAmplitudeTarget;

                    WMAPESParserParams_p->OpBlk_p->Flags  |= WMA_STREAM_RMS_AMP_TARGET_VALID;
                    WMAPESParserParams_p->OpBlk_p->Data[WMA_STREAM_RMS_AMP_TARGET_OFFSET] = WMAPESParserParams_p->iRmsAmplitudeTarget;*/
                }

                WMAPESParserParams_p->WMAPESParserState     = ES_WMAPES_DELIVER_PAYLOAD_DATA;
                break;
            }

        case ES_WMAPES_DELIVER_PAYLOAD_DATA:
            if((Size-offset)>=WMAPESParserParams_p->FrameSize)
            {
                /* 1 or more frame */
                WMAPESParserParams_p->FrameComplete = TRUE;

                memcpy(WMAPESParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(WMAPESParserParams_p->FrameSize ));

                WMAPESParserParams_p->OpBlk_p->Size = WMAPESParserParams_p->FrameSize;
                if(WMAPESParserParams_p->Get_Block == TRUE)
                {
                    if(WMAPESParserParams_p->OpBlk_p->Flags & PTS_VALID)
                    {
                        /* Get the PTS in the correct structure */
                        NotifyPTS.Low  = WMAPESParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                        NotifyPTS.High = WMAPESParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                    }
                    else
                    {
                        NotifyPTS.Low  = 0;
                        NotifyPTS.High = 0;
                    }

                    NotifyPTS.Interpolated  = FALSE;

                    Error = MemPool_PutOpBlk(WMAPESParserParams_p->OpBufHandle,(U32 *)&WMAPESParserParams_p->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" ES_WMAPES Error in Memory Put_Block \n"));
                        return Error;
                    }
                    else
                    {
                        Error = STAudEVT_Notify(WMAPESParserParams_p->EvtHandle,WMAPESParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), WMAPESParserParams_p->ObjectID);
                        WMAPESParserParams_p->Put_Block = TRUE;
                        WMAPESParserParams_p->Get_Block = FALSE;
                    }
                }

                WMAPESParserParams_p->Current_Write_Ptr1 += (WMAPESParserParams_p->FrameSize );
                offset =  offset+WMAPESParserParams_p->FrameSize;
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_A;
            }
            else
            {
                /* less than a frame */
                WMAPESParserParams_p->NoBytesRemainingInFrame = WMAPESParserParams_p->FrameSize  - (Size-offset);
                WMAPESParserParams_p->FrameComplete = FALSE;
                memcpy(WMAPESParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(Size - offset));
                WMAPESParserParams_p->Current_Write_Ptr1 += (Size - offset);
                offset =  Size;
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PARTIAL_FRAME;

            }
            break;

        case ES_WMAPES_PARTIAL_FRAME:

            if(Size-offset>=WMAPESParserParams_p->NoBytesRemainingInFrame)
            {
                /* full frame */
                WMAPESParserParams_p->FrameComplete = TRUE;
                memcpy(WMAPESParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),WMAPESParserParams_p->NoBytesRemainingInFrame);
                WMAPESParserParams_p->OpBlk_p->Size = WMAPESParserParams_p->FrameSize;
                if(WMAPESParserParams_p->Get_Block == TRUE)
                {
                    if(WMAPESParserParams_p->OpBlk_p->Flags & PTS_VALID)
                    {
                        /* Get the PTS in the correct structure */
                        NotifyPTS.Low  = WMAPESParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                        NotifyPTS.High = WMAPESParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                    }
                    else
                    {
                        NotifyPTS.Low  = 0;
                        NotifyPTS.High = 0;
                    }

                    NotifyPTS.Interpolated  = FALSE;

                    Error = MemPool_PutOpBlk(WMAPESParserParams_p->OpBufHandle,(U32 *)&WMAPESParserParams_p->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" ES_WMAPES Error in Memory Put_Block \n"));
                        return Error;
                    }
                    else
                    {
                        Error = STAudEVT_Notify(WMAPESParserParams_p->EvtHandle,WMAPESParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), WMAPESParserParams_p->ObjectID);
                        WMAPESParserParams_p->Put_Block = TRUE;
                        WMAPESParserParams_p->Get_Block = FALSE;
                    }
                }

                WMAPESParserParams_p->Current_Write_Ptr1 += WMAPESParserParams_p->NoBytesRemainingInFrame;
                offset =  offset+WMAPESParserParams_p->NoBytesRemainingInFrame;
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PRIVATE_DATA_HEADER_A;

            }
            else
            {
                /* again less than a frame */
                WMAPESParserParams_p->NoBytesRemainingInFrame -= (Size-offset);
                WMAPESParserParams_p->FrameComplete = FALSE;
                memcpy(WMAPESParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),Size-offset);
                WMAPESParserParams_p->Current_Write_Ptr1 += Size-offset;
                WMAPESParserParams_p->WMAPESParserState = ES_WMAPES_PARTIAL_FRAME;
                offset =  Size;
            }
            break;


        default:
            break;
        }

    }
    *No_Bytes_Used=offset;
    return ST_NO_ERROR;
}

ST_ErrorCode_t ES_WMAPES_Parser_Term(ParserHandle_t *const handle, ST_Partition_t   *CPUPartition_p)
{

    ES_WMAPES_ParserLocalParams_t * WMAPESParserParams_p = (ES_WMAPES_ParserLocalParams_t *)handle;

    if(handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, WMAPESParserParams_p);
    }

    return ST_NO_ERROR;
}
#endif
