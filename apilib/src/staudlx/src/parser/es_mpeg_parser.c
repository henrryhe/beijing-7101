/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : ES_MPEG_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_MPEG
//#define  STTBX_PRINT
#include "es_mpeg_parser.h"
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#include <math.h>
#endif
//#include "ostime.h"
#include "aud_evt.h"

static U16 MP2aBitRate[2][4][16] =
{
    {
        { 0,  0,  0,  0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0, 0 },
        { 0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, 0 },
        { 0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, 0 },
        { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }
    },
    {
        { 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 },
        { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 },
        { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
        { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 }
    }
};

static U16 MP2aSampleRate[4] =
    { 44100, 48000, 32000, 0  };

static U32 AACSampleRate[16] =
    { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 0, 0, 0, 0 };




/************************************************************************************
Name         : Parse_MPEG_Header()

Description  : Parses the MPEG  Audio Header

Parameters   :

Return Value :


 ************************************************************************************/


static ST_ErrorCode_t Parse_MPEG_Header(ES_MPEG_ParserLocalParams_t * MPEGParserParams_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    BOOL                id;
    BOOL                idex;
    MPEGAudioLayer      layer;
    U32                 bitrate_index;
    U32                 sampling_frequency;
    //U32                 padding_bit;
    U32                 mode;
    U32                 emphasis;
    U32                 bit_rate;
    U32                 sample_rate;
    BOOL                lowSampleRate;


    MPEGParserParams_p->AAC = FALSE;

    if(!MPEGParserParams_p->NIBBLE_ALIGNED)
    {
        id                  = ((MPEGParserParams_p->HDR[1] & 0x08) >> 3);//(12, 1)
        idex                = ((MPEGParserParams_p->HDR[1] & 0x10) >> 4);//(11, 1);
        layer               = (MPEGAudioLayer)((MPEGParserParams_p->HDR[1] & 0x06) >> 1);//(13, 2);
        bitrate_index       = ((MPEGParserParams_p->HDR[2] & 0xF0) >> 4);//(16, 4);
        sampling_frequency  = ((MPEGParserParams_p->HDR[2] & 0x0C) >> 2);//(20, 2);
        //padding_bit         = ((MPEGParserParams_p->HDR[2] & 0x02) >> 1);//(22, 1);
        mode                = ((MPEGParserParams_p->HDR[3] & 0xC0) >> 6);//(24, 2);
        emphasis            = ((MPEGParserParams_p->HDR[3] & 0x03)     );//(30, 2);
        bit_rate            = MP2aBitRate[id][layer][bitrate_index];
        sample_rate         = MP2aSampleRate[sampling_frequency];
        lowSampleRate       = (id == 0);
    }
    else
    {
        id                  = ((MPEGParserParams_p->HDR[2] & 0x80) >> 7);//(16, 1)
        idex                = ((MPEGParserParams_p->HDR[1] & 0x01) );//(15, 1);
        layer               = (MPEGAudioLayer)((MPEGParserParams_p->HDR[2] & 0x60) >> 5);//(17, 2);
        bitrate_index       = ((MPEGParserParams_p->HDR[2] & 0x0F));//(12, 4);
        sampling_frequency  = ((MPEGParserParams_p->HDR[3] & 0xC0) >> 6);//(24, 2);
        //padding_bit         = ((MPEGParserParams_p->HDR[3] & 0x20) >> 5);//(26, 1);
        mode                = ((MPEGParserParams_p->HDR[3] & 0x0C) >> 2);//(28, 2);
        //emphasis            = ((MPEGParserParams_p->HDR[4] & 0x30) >> 4);//(34, 2);
        emphasis            = 0; // To be extract from next byte
        bit_rate            = MP2aBitRate[id][layer][bitrate_index];
        sample_rate         = MP2aSampleRate[sampling_frequency];
        lowSampleRate       = (id == 0);
    }



    if(layer != MPAUDLAYER_3)
    {
        if(!idex)
        {
            MPEGParserParams_p->NIBBLE_ALIGNED = FALSE;
            return STAUD_FRAME_ERROR;

        }
    }
    /*else
    {
        if (!idex)
        {
            if(bitrate_index != 15)
            {
                bit_rate = bitrate_index << 3;
            }
        }
    }*/

    if(MPEGParserParams_p->NIBBLE_ALIGNED)
    {
        if((layer == MPAUDLAYER_2)||(layer == MPAUDLAYER_AAC))
        {

            MPEGParserParams_p->NIBBLE_ALIGNED = FALSE;
            return STAUD_FRAME_ERROR;
        }
    }

    if (bit_rate == 0 || sample_rate == 0)
    {

        if(layer == MPAUDLAYER_AAC)
        {

            if((((MPEGParserParams_p->HDR[2] & 0x3C) >> 2)<= 0x0B)&&((MPEGParserParams_p->HDR[2] >> 6) == 1)/*&& (id ==1)*/) /* Philips AAC stream playback */
            {
                MPEGParserParams_p->Layer = layer;
                MPEGParserParams_p->AAC = TRUE;
                MPEGParserParams_p->FrameSize = 0;
            }
        }
        MPEGParserParams_p->NIBBLE_ALIGNED = FALSE;
        return STAUD_FRAME_ERROR;
    }



    switch (layer)
    {
        case MPAUDLAYER_1:
            MPEGParserParams_p->FrameSize = (( 12000 * (2 - id) * bit_rate) / sample_rate ) * 4 ;
            break;
        case MPAUDLAYER_3:
            MPEGParserParams_p->FrameSize = ( 144000 * (2 - idex) * bit_rate) / sample_rate;
            break;
        case MPAUDLAYER_2:
            MPEGParserParams_p->FrameSize = ( 144000 * (2 - id) * bit_rate) / sample_rate;
            break;

        case MPAUDLAYER_AAC:
            MPEGParserParams_p->AAC = TRUE;
            break;
        default:
            break;
    }

    sample_rate = sample_rate >> lowSampleRate;
    sample_rate = sample_rate >> !idex;


    if (sample_rate != MPEGParserParams_p->Frequency)
    {
        MPEGParserParams_p->Frequency = sample_rate;
        /* what to in case of change in  sampling frequency */
    }
    MPEGParserParams_p->Layer = layer;
    MPEGParserParams_p->BitRate  = bit_rate;
    if(mode != MPEGParserParams_p->Mode)
    {
        STAUD_StereoMode_t ReqRouting;
        MPEGParserParams_p->Mode     = mode;

        switch(mode)
        {
        case 0:
        case 1:
            ReqRouting = STAUD_STEREO_MODE_STEREO;
            break;
        case 2:
            ReqRouting = STAUD_STEREO_MODE_DUAL_LEFT | STAUD_STEREO_MODE_DUAL_RIGHT;
            break;
        case 3:
            ReqRouting = STAUD_STEREO_MODE_DUAL_MONO;
        default:
            break;
        }

        Error = STAudEVT_Notify(MPEGParserParams_p->EvtHandle,MPEGParserParams_p->EventIDNewRouting, &ReqRouting,
                                                                sizeof(STAUD_StereoMode_t), MPEGParserParams_p->ObjectID);
    }
    MPEGParserParams_p->Emphasis = emphasis;
    if((MPEGParserParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG_AAC) && (layer != MPAUDLAYER_AAC))
    {
        Error = STAUD_FRAME_ERROR;
    }
    return Error;
}

/************************************************************************************
Name         : ES_MPEG_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
           ES_MPEG_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_MPEG_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_MPEG_ParserLocalParams_t *MPEGParserParams_p;
    MPEGParserParams_p=memory_allocate(Init_p->Memory_Partition,sizeof(ES_MPEG_ParserLocalParams_t));
    if(MPEGParserParams_p==NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(MPEGParserParams_p,0,sizeof(ES_MPEG_ParserLocalParams_t));
    /* Init the Local Parameters */

    MPEGParserParams_p->MPEGParserState = ES_MPEG_SYNC_0;


    MPEGParserParams_p->FrameComplete      = TRUE;
    MPEGParserParams_p->First_Frame        = TRUE;
    MPEGParserParams_p->SYNC               = FALSE;
    MPEGParserParams_p->AAC                = FALSE;
    MPEGParserParams_p->PADDING_CHECK_BYTE = 0;
    MPEGParserParams_p->NIBBLE_ALIGNED    = FALSE;
    MPEGParserParams_p->PADDING_CHECK     = FALSE;
    MPEGParserParams_p->PADDING           = 0;
    MPEGParserParams_p->PADDING_BYTE      = 0;
    MPEGParserParams_p->Frequency         = 0xFFFFFFFF;
    MPEGParserParams_p->Current_Frequency = 0xFFFFFFFF;
    MPEGParserParams_p->Layer             = 0xFF;
    MPEGParserParams_p->Current_Layer     = 0xFF;
    MPEGParserParams_p->Mode              = 0xFF;
    MPEGParserParams_p->Get_Block         = FALSE;
    MPEGParserParams_p->Put_Block         = TRUE;
    MPEGParserParams_p->Reuse             = FALSE;
    MPEGParserParams_p->Skip              = 0;
    MPEGParserParams_p->Pause             = 0;
    MPEGParserParams_p->StartFramePTSValid = FALSE;
    MPEGParserParams_p->OpBufHandle        = Init_p->OpBufHandle;

    /*initialize the event IDs*/
    MPEGParserParams_p->EvtHandle                = Init_p->EvtHandle;
    MPEGParserParams_p->ObjectID                 = Init_p->ObjectID;
    MPEGParserParams_p->EventIDFrequencyChange   = Init_p->EventIDFrequencyChange;
    MPEGParserParams_p->EventIDNewFrame          = Init_p->EventIDNewFrame;
    MPEGParserParams_p->EventIDNewRouting        = Init_p->EventIDNewRouting;
    MPEGParserParams_p->Blk_Out                  = 0;
    *handle=(ParserHandle_t )MPEGParserParams_p;

    return (Error);
}

ST_ErrorCode_t ES_MPEG_ParserGetParsingFunction( ParsingFunction_t * ParsingFunc_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p)
{

    *ParsingFunc_p          = (ParsingFunction_t)ES_MPEG_Parse_Frame;
    *GetFreqFunction_p  = (GetFreqFunction_t)ES_MPEG_Get_Freq;
    *GetInfoFunction_p  = (GetInfoFunction_t)ES_MPEG_Get_Info;

    return ST_NO_ERROR;
}



/************************************************************************************
Name         : ES_MPEG_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_MPEG_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_MPEG_ParserLocalParams_t *MPEGParserParams_p = (ES_MPEG_ParserLocalParams_t *)Handle;

    if(MPEGParserParams_p->Get_Block == TRUE)
    {
        MPEGParserParams_p->OpBlk_p->Size = MPEGParserParams_p->FrameSize + MPEGParserParams_p->ExtFrameSize + MPEGParserParams_p->PADDING_BYTE+ MPEGParserParams_p->NIBBLE_ALIGNED;
        MPEGParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(MPEGParserParams_p->OpBufHandle,(U32 *)&MPEGParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_MPEG Error in Memory Put_Block \n"));
        }
        else
        {
            MPEGParserParams_p->Put_Block = TRUE;
            MPEGParserParams_p->Get_Block = FALSE;
        }
    }

    MPEGParserParams_p->MPEGParserState     = ES_MPEG_SYNC_0;
    MPEGParserParams_p->FrameComplete       = TRUE;
    MPEGParserParams_p->First_Frame         = TRUE;
    MPEGParserParams_p->SYNC                = FALSE;
    MPEGParserParams_p->AAC                 = FALSE;
    MPEGParserParams_p->PADDING_CHECK_BYTE  = 0;
    MPEGParserParams_p->NIBBLE_ALIGNED      = FALSE;
    MPEGParserParams_p->PADDING_CHECK       = FALSE;
    MPEGParserParams_p->Frequency           = 0xFFFFFFFF;
    MPEGParserParams_p->Layer               = 0xFF;
    MPEGParserParams_p->Current_Layer       = 0xFF;
    MPEGParserParams_p->Current_Frequency   = 0xFFFFFFFF;
    MPEGParserParams_p->PADDING             = 0;
    MPEGParserParams_p->PADDING_BYTE        = 0;
    MPEGParserParams_p->Blk_Out             = 0;
    return Error;
}

/************************************************************************************
Name         : ES_MPEG_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_MPEG_Parser_Start(ParserHandle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_MPEG_ParserLocalParams_t *MPEGParserParams_p = (ES_MPEG_ParserLocalParams_t *)Handle;

    MPEGParserParams_p->MPEGParserState = ES_MPEG_SYNC_0;
    MPEGParserParams_p->FrameComplete   = TRUE;
    MPEGParserParams_p->First_Frame     = TRUE;
    MPEGParserParams_p->SYNC            = FALSE;
    MPEGParserParams_p->AAC             = FALSE;
    MPEGParserParams_p->PADDING_CHECK_BYTE = 0;
    MPEGParserParams_p->NIBBLE_ALIGNED     = FALSE;
    MPEGParserParams_p->PADDING_CHECK      = FALSE;
    MPEGParserParams_p->Frequency          = 0xFFFFFFFF;
    MPEGParserParams_p->Layer              = 0xFF;
    MPEGParserParams_p->Current_Layer      = 0xFF;
    MPEGParserParams_p->Current_Frequency  = 0xFFFFFFFF;
    MPEGParserParams_p->PADDING            = 0;
    MPEGParserParams_p->PADDING_BYTE       = 0;
    MPEGParserParams_p->Mode               = 0xFF;
    MPEGParserParams_p->Put_Block           = TRUE;
    MPEGParserParams_p->Get_Block           = FALSE;
    MPEGParserParams_p->Reuse               = FALSE;
    MPEGParserParams_p->Skip                = 0;
    MPEGParserParams_p->Pause               = 0;
    MPEGParserParams_p->StartFramePTSValid  = FALSE;
    MPEGParserParams_p->Blk_Out             = 0;
    return Error;
}

/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_MPEG_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_MPEG_ParserLocalParams_t *MPEGParserParams_p = (ES_MPEG_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p=MPEGParserParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_MPEG_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_MPEG_ParserLocalParams_t *MPEGParserParams_p;
    STAUD_MPEG_Layer_t MPEGLayer = STAUD_MPEG_LAYER_NONE;

    MPEGParserParams_p=(ES_MPEG_ParserLocalParams_t *)Handle;

    switch( MPEGParserParams_p->Layer )
    {
    case MPAUDLAYER_1:
        MPEGLayer = STAUD_MPEG_LAYER_I;
        break;
    case MPAUDLAYER_2:
        MPEGLayer = STAUD_MPEG_LAYER_II;
        break;
    case MPAUDLAYER_3:
        MPEGLayer = STAUD_MPEG_LAYER_III;
        break;
    default:
        break;
    }

    StreamInfo->BitRate             = MPEGParserParams_p->BitRate;
    StreamInfo->SamplingFrequency   = MPEGParserParams_p->Frequency;
    StreamInfo->LayerNumber         = MPEGLayer;
    StreamInfo->Mode                = MPEGParserParams_p->Mode;
    StreamInfo->Emphasis            = MPEGParserParams_p->Emphasis;
    return ST_NO_ERROR;
}

/* Handle EOF from Parser */

ST_ErrorCode_t ES_MPEG_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_MPEG_ParserLocalParams_t *MPEGParserParams_p = (ES_MPEG_ParserLocalParams_t *)Handle;

    if(MPEGParserParams_p->Get_Block == TRUE)
    {
        memset(MPEGParserParams_p->Current_Write_Ptr1,0x00,MPEGParserParams_p->NoBytesRemainingInFrame);

        MPEGParserParams_p->OpBlk_p->Size = MPEGParserParams_p->FrameSize;
        MPEGParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
        MPEGParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        MPEGParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
        Error = MemPool_PutOpBlk(MPEGParserParams_p->OpBufHandle,(U32 *)&MPEGParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
        }
        else
        {
            MPEGParserParams_p->Put_Block = TRUE;
            MPEGParserParams_p->Get_Block = FALSE;
        }
    }
    else
    {
        Error = MemPool_GetOpBlk(MPEGParserParams_p->OpBufHandle, (U32 *)&MPEGParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            return STAUD_MEMORY_GET_ERROR;
        }
        else
        {
            MPEGParserParams_p->Put_Block = FALSE;
            MPEGParserParams_p->Get_Block = TRUE;
            MPEGParserParams_p->OpBlk_p->Size = 1152;
            MPEGParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
            memset((void*)MPEGParserParams_p->OpBlk_p->MemoryStart,0x00,MPEGParserParams_p->OpBlk_p->MaxSize);
            Error = MemPool_PutOpBlk(MPEGParserParams_p->OpBufHandle,(U32 *)&MPEGParserParams_p->OpBlk_p);
            if(Error != ST_NO_ERROR)
            {
                STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
            }
            else
            {
                MPEGParserParams_p->Put_Block = TRUE;
                MPEGParserParams_p->Get_Block = FALSE;
            }
        }
    }

    return ST_NO_ERROR;
}

/************************************************************************************
Name         : ES_MPEG_Parse_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t ES_MPEG_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info_p)
{
    U8 *pos;
    U32 offset;
    U8 value;
    ST_ErrorCode_t Error;
    ES_MPEG_ParserLocalParams_t *MPEGParserParams_p = (ES_MPEG_ParserLocalParams_t *)Handle;
    STAUD_PTS_t NotifyPTS;

    offset=*No_Bytes_Used;
    pos=(U8 *)MemoryStart;

    //STTBX_Print((" ES_MPEG Parser Enrty \n"));

    while(offset<Size)
    {
        if(MPEGParserParams_p->FrameComplete == TRUE)
        {

            if(MPEGParserParams_p->Put_Block == TRUE)
            {
                Error = ST_NO_ERROR;
                //already delivered the block. do we need to reuse the old block????
                if(MPEGParserParams_p->Reuse == FALSE)
                {
                    Error = MemPool_GetOpBlk(MPEGParserParams_p->OpBufHandle, (U32 *)&MPEGParserParams_p->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        *No_Bytes_Used = offset;
                        return STAUD_MEMORY_GET_ERROR;
                    }
                    else
                    {
                        if(MPEGParserParams_p->Pause)
                        {
                            AUD_TaskDelayMs(MPEGParserParams_p->Pause);
                            MPEGParserParams_p->Pause = 0;
                        }
                    }
                }

                MPEGParserParams_p->Put_Block = FALSE;
                MPEGParserParams_p->Get_Block = TRUE;
                MPEGParserParams_p->Current_Write_Ptr1 = (U8 *)MPEGParserParams_p->OpBlk_p->MemoryStart;
                //memset(MPEGParserParams_p->Current_Write_Ptr1,0x00,MPEGParserParams_p->OpBlk_p->MaxSize);
                MPEGParserParams_p->OpBlk_p->Flags = 0;
                // code below commented out. To be filled by the value at start of frame
                /*if(CurrentPTS_p->PTSValidFlag == TRUE)
                {
                    MPEGParserParams_p->OpBlk_p->Flags   |= PTS_VALID;
                    MPEGParserParams_p->OpBlk_p->Data[PTS_OFFSET]    = CurrentPTS_p->PTS;
                    MPEGParserParams_p->OpBlk_p->Data[PTS33_OFFSET] = CurrentPTS_p->PTS33Bit;
                }*/

                if(Private_Info_p->Private_Data[2] == 1) // FADE PAN VALID FLAG
                {
                    MPEGParserParams_p->OpBlk_p->Flags   |= FADE_VALID;
                    MPEGParserParams_p->OpBlk_p->Flags   |= PAN_VALID;
                    MPEGParserParams_p->OpBlk_p->Data[FADE_OFFSET]   = (Private_Info_p->Private_Data[5]<<8)|(Private_Info_p->Private_Data[0]); // FADE Value
                    MPEGParserParams_p->OpBlk_p->Data[PAN_OFFSET]    = (Private_Info_p->Private_Data[6]<<8)|(Private_Info_p->Private_Data[1]); // PAN Value
                }

                MPEGParserParams_p->OpBlk_p->Flags   |= DATAFORMAT_VALID;
                MPEGParserParams_p->OpBlk_p->Data[DATAFORMAT_OFFSET] = BE;

                // code below commented out. To be invalidated at start of frame
                /*CurrentPTS_p->PTSValidFlag            = FALSE;*/
                Private_Info_p->Private_Data[2]     = 0;
                MPEGParserParams_p->FrameComplete = FALSE;
                MPEGParserParams_p->Current_Write_Ptr1 = (U8 *)MPEGParserParams_p->OpBlk_p->MemoryStart;
            }
        }

        value=pos[offset];
        switch(MPEGParserParams_p->MPEGParserState)
        {
        case ES_MPEG_SYNC_0:

            if(MPEGParserParams_p->PADDING_CHECK_BYTE > 0)
            {
                if(MPEGParserParams_p->PADDING_CHECK_BYTE_8)
                {
                    value = MPEGParserParams_p->TMP[8 - MPEGParserParams_p->PADDING_CHECK_BYTE];

                }
                else
                {
                    value = MPEGParserParams_p->TMP[4 - MPEGParserParams_p->PADDING_CHECK_BYTE];
                }
                MPEGParserParams_p->PADDING_CHECK_BYTE--;
            }
            else
            {
                if(CurrentPTS_p->PTSValidFlag)
                {
                    MPEGParserParams_p->StartFramePTS = CurrentPTS_p->PTS;
                    MPEGParserParams_p->StartFramePTS33 = CurrentPTS_p->PTS33Bit;
                    MPEGParserParams_p->StartFramePTSValid = TRUE;
                    CurrentPTS_p->PTSValidFlag = FALSE;
                }

                offset++;
            }
            //MPEGParserParams_p->NIBBLE_ALIGNED = FALSE;

            MPEGParserParams_p->HDR[0]=value;

            if(MPEGParserParams_p->NIBBLE_ALIGNED)
            {
                if ((value & 0x0F) == 0x0F)
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_SYNC_1;
                }
                else
                {
                    MPEGParserParams_p->SYNC = FALSE;
                }

            }
            else
            {
                if (value == 0xFF)
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_SYNC_1;
                }
#ifdef  NIBBLE_ALIGNED_SUPPORT
                else if ((value & 0x0F) == 0x0F)
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_SYNC_1;
                    MPEGParserParams_p->NIBBLE_ALIGNED = TRUE;
                }
#endif
                else
                {
                    MPEGParserParams_p->SYNC = FALSE;
                }
            }
            break;

        case ES_MPEG_SYNC_1:
            if(MPEGParserParams_p->PADDING_CHECK_BYTE > 0)
            {
                if(MPEGParserParams_p->PADDING_CHECK_BYTE_8)
                {
                    value = MPEGParserParams_p->TMP[8 - MPEGParserParams_p->PADDING_CHECK_BYTE];

                }
                else
                {
                    value = MPEGParserParams_p->TMP[4 - MPEGParserParams_p->PADDING_CHECK_BYTE];
                }
                MPEGParserParams_p->PADDING_CHECK_BYTE--;
            }
            else
            {
                offset++;
            }

            if(!MPEGParserParams_p->NIBBLE_ALIGNED)
            {
                if ((value & 0xE0) == 0xE0)
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_PARSE_HDR2_SYNC_FRAME;
                    MPEGParserParams_p->HDR[1]=value;
                }
                else
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_SYNC_0;
                    MPEGParserParams_p->SYNC = FALSE;
                }
            }
            else
            {
                if ((value & 0xFE) == 0xFE)
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_PARSE_HDR2_SYNC_FRAME;
                    MPEGParserParams_p->HDR[1]=value;
                }
                else
                {
                    MPEGParserParams_p->NIBBLE_ALIGNED = FALSE;
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_SYNC_0;
                    MPEGParserParams_p->SYNC = FALSE;
                }
            }
            break;

        case ES_MPEG_PARSE_HDR2_SYNC_FRAME:
            if(MPEGParserParams_p->FrameComplete == TRUE)
            {

                if(MPEGParserParams_p->SYNC == TRUE)
                {
                    if(MPEGParserParams_p->Get_Block == TRUE)
                    {



                        if((MPEGParserParams_p->Frequency != MPEGParserParams_p->Current_Frequency) && (!MPEGParserParams_p->First_Frame))
                        {
                            MPEGParserParams_p->Current_Frequency = MPEGParserParams_p->Frequency;
                            MPEGParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                            MPEGParserParams_p->OpBlk_p->Data[FREQ_OFFSET] = MPEGParserParams_p->Frequency;
                            Error=STAudEVT_Notify(MPEGParserParams_p->EvtHandle,MPEGParserParams_p->EventIDFrequencyChange, &MPEGParserParams_p->Frequency, sizeof(MPEGParserParams_p->Frequency), MPEGParserParams_p->ObjectID);
                        }

                        if((MPEGParserParams_p->Layer != MPEGParserParams_p->Current_Layer) && (!MPEGParserParams_p->First_Frame))
                        {
                            MPEGParserParams_p->Current_Layer = MPEGParserParams_p->Layer;
                            MPEGParserParams_p->OpBlk_p->Flags  |= LAYER_VALID;
                            MPEGParserParams_p->OpBlk_p->AppData_p = (void *)((U32)MPEGParserParams_p->Layer);
                            MPEGParserParams_p->OpBlk_p->Data[LAYER_OFFSET] = (U32)MPEGParserParams_p->Layer;
                        }

                        if(MPEGParserParams_p->First_Frame)
                        {

                            MPEGParserParams_p->First_Frame = FALSE;
                            MPEGParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                            MPEGParserParams_p->OpBlk_p->Data[FREQ_OFFSET] = MPEGParserParams_p->Frequency;
                            Error=STAudEVT_Notify(MPEGParserParams_p->EvtHandle,MPEGParserParams_p->EventIDFrequencyChange, &MPEGParserParams_p->Frequency, sizeof(MPEGParserParams_p->Frequency), MPEGParserParams_p->ObjectID);
                            MPEGParserParams_p->Current_Frequency = MPEGParserParams_p->Frequency;
                            MPEGParserParams_p->OpBlk_p->Flags  |= LAYER_VALID;
                            MPEGParserParams_p->OpBlk_p->AppData_p = (void *)((U32)MPEGParserParams_p->Layer);
                            MPEGParserParams_p->OpBlk_p->Data[LAYER_OFFSET] = (U32)MPEGParserParams_p->Layer;
                            MPEGParserParams_p->Current_Layer = MPEGParserParams_p->Layer;

                        }

                        if(MPEGParserParams_p->OpBlk_p->Flags & PTS_VALID)
                        {
                            /* Get the PTS in the correct structure */
                            NotifyPTS.Low  = MPEGParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                            NotifyPTS.High = MPEGParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                        }
                        else
                        {
                            NotifyPTS.Low  = 0;
                            NotifyPTS.High = 0;
                        }

                        NotifyPTS.Interpolated  = FALSE;

                        if(MPEGParserParams_p->Skip == 0)
                        {
                            Error = MemPool_PutOpBlk(MPEGParserParams_p->OpBufHandle,(U32 *)&MPEGParserParams_p->OpBlk_p);

                            if(Error != ST_NO_ERROR)
                            {
                                STTBX_Print((" ES_MPEG Error in Memory Put_Block \n"));
                                return Error;
                            }
                            else
                            {
                                Error = STAudEVT_Notify(MPEGParserParams_p->EvtHandle,MPEGParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), MPEGParserParams_p->ObjectID);
                                STTBX_Print((" MPEG Deli:%u\n",MPEGParserParams_p->Blk_Out));

                                MPEGParserParams_p->Blk_Out++;

                                MPEGParserParams_p->Reuse = FALSE;
                                MPEGParserParams_p->Put_Block = TRUE;
                                MPEGParserParams_p->Get_Block = FALSE;
                            }
                        }
                        else
                        {
                            MPEGParserParams_p->Skip --;
                            MPEGParserParams_p->Reuse = TRUE;
                            MPEGParserParams_p->Put_Block = TRUE;
                            MPEGParserParams_p->Get_Block = FALSE;
                        }
                    }
                }
                else
                {
                    MPEGParserParams_p->Current_Write_Ptr1 = (U8 *)MPEGParserParams_p->OpBlk_p->MemoryStart;
                }


            }
            MPEGParserParams_p->MPEGParserState = ES_MPEG_PARSE_HDR2;
            break;



        case ES_MPEG_PARSE_HDR2:
            if(MPEGParserParams_p->PADDING_CHECK_BYTE > 0)
            {
                if(MPEGParserParams_p->PADDING_CHECK_BYTE_8)
                {
                    value = MPEGParserParams_p->TMP[8 - MPEGParserParams_p->PADDING_CHECK_BYTE];

                }
                else
                {
                    value = MPEGParserParams_p->TMP[4 - MPEGParserParams_p->PADDING_CHECK_BYTE];
                }
                MPEGParserParams_p->PADDING_CHECK_BYTE--;
            }
            else
            {
                offset++;
            }

            MPEGParserParams_p->MPEGParserState = ES_MPEG_PARSE_HDR3;
            MPEGParserParams_p->HDR[2]=value;
            break;

        case ES_MPEG_PARSE_HDR3:
            if(MPEGParserParams_p->PADDING_CHECK_BYTE > 0)
            {
                if(MPEGParserParams_p->PADDING_CHECK_BYTE_8)
                {
                    value = MPEGParserParams_p->TMP[8 - MPEGParserParams_p->PADDING_CHECK_BYTE];

                }
                else
                {
                    value = MPEGParserParams_p->TMP[4 - MPEGParserParams_p->PADDING_CHECK_BYTE];
                }
                MPEGParserParams_p->PADDING_CHECK_BYTE--;
            }
            else
            {
                offset++;
            }

            MPEGParserParams_p->MPEGParserState = ES_MPEG_PARSE_CHECKHEADER;
            MPEGParserParams_p->HDR[3]=value;
            break;

        case ES_MPEG_PARSE_CHECKHEADER:
            {
                Error=Parse_MPEG_Header(MPEGParserParams_p);

                if((Error!=ST_NO_ERROR) && (MPEGParserParams_p->AAC)&&(Private_Info_p->Private_Data[3] == 1)) //Private_Info.Private_Data[3] == 1 for AAC
                {
                    MPEGParserParams_p->MPEGParserState=ES_MPEG_AAC_HDR5;
                    offset++;
                    MPEGParserParams_p->HDR[4]=value;
                }
                else
                {
                    if(MPEGParserParams_p->NIBBLE_ALIGNED)
                    {
                        MPEGParserParams_p->Emphasis = ((value & 0x30) >> 4);
                    }
                    MPEGParserParams_p->AAC = FALSE;
                    if((Error!=ST_NO_ERROR)||(MPEGParserParams_p->FrameSize<4)||(Private_Info_p->Private_Data[3] == 1))
                    {
                        MPEGParserParams_p->Layer = MPEGParserParams_p->Current_Layer;
                        MPEGParserParams_p->Frequency = MPEGParserParams_p->Current_Frequency;
                        MPEGParserParams_p->NIBBLE_ALIGNED = FALSE;
                        STTBX_Print((" Frame Size Unknown\n"));
                        MPEGParserParams_p->PADDING_CHECK_BYTE = 3;
                        MPEGParserParams_p->PADDING_CHECK_BYTE_8 = FALSE;

                        MPEGParserParams_p->TMP[0] = MPEGParserParams_p->HDR[1];
                        MPEGParserParams_p->TMP[1] = MPEGParserParams_p->HDR[2];
                        MPEGParserParams_p->TMP[2] = MPEGParserParams_p->HDR[3];

                        MPEGParserParams_p->MPEGParserState=ES_MPEG_SYNC_0;
                    }
                    else
                    {
                        MPEGParserParams_p->MPEGParserState = ES_MPEG_SET_VALID_FRAME;
                    }
                }
            }
            break;


        case ES_MPEG_AAC_HDR5:
            {

                U32 Temp_Frequency;

                offset++;
                MPEGParserParams_p->HDR[5]=value;
                Temp_Frequency = AACSampleRate[((MPEGParserParams_p->HDR[2] & 0x3C) >> 2)]; //(17,4)
                MPEGParserParams_p->FrameSize =  (((MPEGParserParams_p->HDR[3] & 0x3) << 11) | (MPEGParserParams_p->HDR[4] << 3) | (MPEGParserParams_p->HDR[5] >> 5)); //(29,13)

                //if(((MPEGParserParams_p->HDR[1] & 0xFE) != 0xF8) ||  (Temp_Frequency <= 0) ||  (MPEGParserParams_p->FrameSize < 7))
                if(((MPEGParserParams_p->HDR[1] & 0xF6) != 0xF0) ||  (Temp_Frequency == 0) ||  (MPEGParserParams_p->FrameSize < 7)||((MPEGParserParams_p->HDR[2] >> 6) != 1))/* Philips AAC stream playback */
                {
                    STTBX_Print(("AAC SYNC ERROR\n"));
                    MPEGParserParams_p->MPEGParserState=ES_MPEG_SYNC_0;

                    MPEGParserParams_p->PADDING_CHECK_BYTE = 4;
                    MPEGParserParams_p->PADDING_CHECK_BYTE_8 = FALSE;

                    MPEGParserParams_p->TMP[0] = MPEGParserParams_p->HDR[2];
                    MPEGParserParams_p->TMP[1] = MPEGParserParams_p->HDR[3];
                    MPEGParserParams_p->TMP[2] = MPEGParserParams_p->HDR[4];
                    MPEGParserParams_p->TMP[3] = MPEGParserParams_p->HDR[5];

                    MPEGParserParams_p->AAC = FALSE;
                    break; /* If Sync error occurs, start again */
                }
                else
                {
                    MPEGParserParams_p->Frequency  = Temp_Frequency;
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_SET_VALID_FRAME;



                }
            }

        case ES_MPEG_SET_VALID_FRAME:
            {

                //  U32 TEMP_PADDING_BYTE = MPEGParserParams_p->PADDING_CHECK_BYTE;
                //  U32 i;
                // now put the PTS

                if(MPEGParserParams_p->StartFramePTSValid)
                {
                    MPEGParserParams_p->OpBlk_p->Flags   |= PTS_VALID;
                    MPEGParserParams_p->OpBlk_p->Data[PTS_OFFSET]      = MPEGParserParams_p->StartFramePTS;
                    MPEGParserParams_p->OpBlk_p->Data[PTS33_OFFSET]    = MPEGParserParams_p->StartFramePTS33;
                    MPEGParserParams_p->StartFramePTSValid = FALSE;
                }

                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[0];
                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[1];
                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[2];
                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[3];
                MPEGParserParams_p->NoBytesCopied= 4;
                while(MPEGParserParams_p->PADDING_CHECK_BYTE)
                {
                    *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->TMP[8 - MPEGParserParams_p->PADDING_CHECK_BYTE];
                    MPEGParserParams_p->PADDING_CHECK_BYTE--;
                    MPEGParserParams_p->NoBytesCopied++;
                }


                if(MPEGParserParams_p->AAC)
                {
                    *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[4];
                    *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[5];
                    MPEGParserParams_p->NoBytesCopied= 6;
                }
                MPEGParserParams_p->MPEGParserState=ES_MPEG_FRAME;
            }

        case ES_MPEG_FRAME:
            if((Size-offset+MPEGParserParams_p->NoBytesCopied)>=MPEGParserParams_p->FrameSize)
            {
                memcpy(MPEGParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(MPEGParserParams_p->FrameSize - MPEGParserParams_p->NoBytesCopied));
                MPEGParserParams_p->Current_Write_Ptr1 += (MPEGParserParams_p->FrameSize - MPEGParserParams_p->NoBytesCopied);

                offset =  offset-MPEGParserParams_p->NoBytesCopied+MPEGParserParams_p->FrameSize;
                MPEGParserParams_p->NoBytesRemainingInFrame = 0;
                if(!MPEGParserParams_p->AAC)
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_PADDING;
                    MPEGParserParams_p->PADDING_CHECK = TRUE;
                }
                else
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_EXT_SYNC_0;
                }


            }
            else
            {
                /* less than a frame */
                MPEGParserParams_p->NoBytesRemainingInFrame = MPEGParserParams_p->FrameSize  - (Size-offset+MPEGParserParams_p->NoBytesCopied);
                MPEGParserParams_p->FrameComplete = FALSE;

                memcpy(MPEGParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(Size - offset));
                MPEGParserParams_p->Current_Write_Ptr1 += (Size - offset);
                offset =  Size;
                MPEGParserParams_p->NoBytesCopied = 0;

                MPEGParserParams_p->MPEGParserState = ES_MPEG_PARTIAL_FRAME;
            }
            break;

        case ES_MPEG_PARTIAL_FRAME:


            if(Size-offset>=MPEGParserParams_p->NoBytesRemainingInFrame)
            {
                /* full frame */
                memcpy(MPEGParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),MPEGParserParams_p->NoBytesRemainingInFrame);

                MPEGParserParams_p->Current_Write_Ptr1 += MPEGParserParams_p->NoBytesRemainingInFrame;

                offset =  offset+MPEGParserParams_p->NoBytesRemainingInFrame;
                MPEGParserParams_p->NoBytesRemainingInFrame = 0;

                if(!MPEGParserParams_p->AAC)
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_PADDING;
                    MPEGParserParams_p->PADDING_CHECK = TRUE;

                }
                else
                {
                    MPEGParserParams_p->MPEGParserState = ES_MPEG_EXT_SYNC_0;
                }


            }
            else
            {
                /* again less than a frame */
                MPEGParserParams_p->NoBytesRemainingInFrame -= (Size-offset);
                MPEGParserParams_p->FrameComplete = FALSE;
                memcpy(MPEGParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),Size-offset);
                MPEGParserParams_p->Current_Write_Ptr1 += Size-offset;

                offset =  Size;
            }
            break;


        case ES_MPEG_CHECK_PADDING:
            MPEGParserParams_p->PADD[0] = value;
            if(CurrentPTS_p->PTSValidFlag == TRUE)
            {
                MPEGParserParams_p->StartFramePTS = CurrentPTS_p->PTS;
                MPEGParserParams_p->StartFramePTS33 = CurrentPTS_p->PTS33Bit;
                MPEGParserParams_p->StartFramePTSValid = TRUE;
                CurrentPTS_p->PTSValidFlag = FALSE;
            }
            else
            {
                MPEGParserParams_p->StartFramePTSValid = FALSE;
            }
            offset++;
            if(MPEGParserParams_p->Layer == MPAUDLAYER_1)
            {
                MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_PADDING_1;
            }
            else
            {
                MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_HEADER_0;
            }
            break;

        case ES_MPEG_CHECK_PADDING_1:
            MPEGParserParams_p->PADD[1] = value;
            offset++;
            MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_PADDING_2;
            break;

        case ES_MPEG_CHECK_PADDING_2:
            MPEGParserParams_p->PADD[2] = value;
            offset++;
            MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_PADDING_3;
            break;

        case ES_MPEG_CHECK_PADDING_3:
            MPEGParserParams_p->PADD[3] = value;
            offset++;
            MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_HEADER_0;
            break;

        case ES_MPEG_CHECK_HEADER_0:
             MPEGParserParams_p->PRE_HDR[0] = value;
             offset++;
             MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_HEADER_1;
             break;

        case ES_MPEG_CHECK_HEADER_1:
             MPEGParserParams_p->PRE_HDR[1] = value;
             offset++;
             MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_HEADER_2;
             break;

        case ES_MPEG_CHECK_HEADER_2:
             MPEGParserParams_p->PRE_HDR[2] = value;
             offset++;
             MPEGParserParams_p->MPEGParserState = ES_MPEG_CHECK_HEADER;
             break;


        case ES_MPEG_CHECK_HEADER:

            MPEGParserParams_p->PADDING = 0;
            MPEGParserParams_p->PADDING_BYTE = 0;

            if(!MPEGParserParams_p->NIBBLE_ALIGNED)
            {
                if(((MPEGParserParams_p->PRE_HDR[0] & 0xFF) == 0xFF) && ((MPEGParserParams_p->PRE_HDR[1] & 0xF6) ==  (MPEGParserParams_p->HDR[1] & 0xF6))
                && ((MPEGParserParams_p->PRE_HDR[2] & 0x0C) ==  (MPEGParserParams_p->HDR[2] & 0x0C)))
                {
                    MPEGParserParams_p->PADDING = 1;

                }

                if(((MPEGParserParams_p->PRE_HDR[0] & 0xFF) == 0x7F) && ((MPEGParserParams_p->PRE_HDR[1] & 0xF0) ==  0xF0)
                && (MPEGParserParams_p->Layer == MPAUDLAYER_2))
                {
                    MPEGParserParams_p->PADDING = 1;
                    MPEGParserParams_p->ExtFrame =1;
                }
            }
            else
            {
                if(((MPEGParserParams_p->PRE_HDR[0] & 0x0F) == 0x0F) && ((MPEGParserParams_p->PRE_HDR[1] & 0xFF) ==  0xFF)
                && ((MPEGParserParams_p->PRE_HDR[2] & 0xC6) ==  (MPEGParserParams_p->HDR[2] & 0xC6)))
                {
                    MPEGParserParams_p->PADDING = 1;

                }

                if(((MPEGParserParams_p->PRE_HDR[0] & 0x0F) == 0x07) && ((MPEGParserParams_p->PRE_HDR[1] & 0xFF) ==  0xFF)
                && (MPEGParserParams_p->Layer == MPAUDLAYER_2))
                {
                    MPEGParserParams_p->PADDING = 1;
                    MPEGParserParams_p->ExtFrame =1;
                }
            }


            if(MPEGParserParams_p->PADDING)
            {
                MPEGParserParams_p->TMP[0] = MPEGParserParams_p->PRE_HDR[0];
                MPEGParserParams_p->TMP[1] = MPEGParserParams_p->PRE_HDR[1];
                MPEGParserParams_p->TMP[2] = MPEGParserParams_p->PRE_HDR[2];
                MPEGParserParams_p->TMP[3] = value;
                offset++;

                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->PADD[0];

                MPEGParserParams_p->PADDING_BYTE = 1;

                if(MPEGParserParams_p->Layer == MPAUDLAYER_1)
                {
                    *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->PADD[1];
                    *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->PADD[2];
                    *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->PADD[3];
                    MPEGParserParams_p->PADDING_BYTE = 4;
                }


                MPEGParserParams_p->PADDING_CHECK_BYTE = 4;
                MPEGParserParams_p->PADDING_CHECK_BYTE_8 = FALSE;
            }
            else
            {
                MPEGParserParams_p->PADDING_BYTE = 0;
                if(MPEGParserParams_p->Layer == MPAUDLAYER_1)
                {
                    MPEGParserParams_p->TMP[0] = MPEGParserParams_p->PADD[0];
                    MPEGParserParams_p->TMP[1] = MPEGParserParams_p->PADD[1];
                    MPEGParserParams_p->TMP[2] = MPEGParserParams_p->PADD[2];
                    MPEGParserParams_p->TMP[3] = MPEGParserParams_p->PADD[3];

                    MPEGParserParams_p->TMP[4] = MPEGParserParams_p->PRE_HDR[0];
                    MPEGParserParams_p->TMP[5] = MPEGParserParams_p->PRE_HDR[1];
                    MPEGParserParams_p->TMP[6] = MPEGParserParams_p->PRE_HDR[2];
                    MPEGParserParams_p->TMP[7] = value;//MPEGParserParams_p->PRE_HDR[3];
                    MPEGParserParams_p->PADDING_CHECK_BYTE = 8;
                    MPEGParserParams_p->PADDING_CHECK_BYTE_8 = TRUE;
                    offset++;
                }
                else
                {
                    MPEGParserParams_p->TMP[0] = MPEGParserParams_p->PADD[0];
                    MPEGParserParams_p->TMP[1] = MPEGParserParams_p->PRE_HDR[0];
                    MPEGParserParams_p->TMP[2] = MPEGParserParams_p->PRE_HDR[1];
                    MPEGParserParams_p->TMP[3] = MPEGParserParams_p->PRE_HDR[2];
                    MPEGParserParams_p->PADDING_CHECK_BYTE = 4;
                    MPEGParserParams_p->PADDING_CHECK_BYTE_8 = FALSE;
                /*  if(MPEGParserParams_p->Layer == MPAUDLAYER_2)
                    {
                        MPEGParserParams_p->HDR[4] = MPEGParserParams_p->PRE_HDR[3];
                    }
                    else
                    {
                        MPEGParserParams_p->TMP[0] = MPEGParserParams_p->PRE_HDR[3];
                    }*/
                }
            }

            MPEGParserParams_p->MPEGParserState = ES_MPEG_EXT_SYNC_0;

            break;

        case ES_MPEG_EXT_SYNC_0:

            if(!MPEGParserParams_p->PADDING_CHECK)
            {
                if(CurrentPTS_p->PTSValidFlag == TRUE)
                {
                    MPEGParserParams_p->StartFramePTS = CurrentPTS_p->PTS;
                    MPEGParserParams_p->StartFramePTS33 = CurrentPTS_p->PTS33Bit;
                    MPEGParserParams_p->StartFramePTSValid = TRUE;
                    CurrentPTS_p->PTSValidFlag = FALSE;
                }
                else
                {
                    MPEGParserParams_p->StartFramePTSValid = FALSE;
                }

                MPEGParserParams_p->HDR[0] = value;
                offset++;
            }
            else
            {
                MPEGParserParams_p->HDR[0] = MPEGParserParams_p->TMP[0];
            }

            if (((MPEGParserParams_p->HDR[0] & 0xFF) == 0x7F) && (MPEGParserParams_p->Layer == MPAUDLAYER_2))
            {
                MPEGParserParams_p->MPEGParserState = ES_MPEG_EXT_SYNC_1;
            }
            else
            {
                if(!MPEGParserParams_p->PADDING_CHECK)
                {
                    MPEGParserParams_p->HDR[0] = value;
                    offset--;
                }
                MPEGParserParams_p->ExtFrame =0;
                MPEGParserParams_p->ExtFrameSize = 0;
                MPEGParserParams_p->MPEGParserState = ES_MPEG_FULL_FRAME;
            }

            break;

        case ES_MPEG_EXT_SYNC_1:

            if(!MPEGParserParams_p->PADDING_CHECK)
            {
                MPEGParserParams_p->HDR[1] = value;
                offset++;
            }
            else
            {
                MPEGParserParams_p->HDR[1] = MPEGParserParams_p->TMP[1];
            }



            if ((MPEGParserParams_p->HDR[1] & 0xF0) == 0xF0)
            {
                MPEGParserParams_p->MPEGParserState = ES_MPEG_EXT_HDR_1;
            }
            else
            {
                if(!MPEGParserParams_p->PADDING_CHECK)
                {
                offset--;
                }
                MPEGParserParams_p->ExtFrame =0;
                MPEGParserParams_p->ExtFrameSize = 0;
                MPEGParserParams_p->MPEGParserState = ES_MPEG_FULL_FRAME;
            }
            break;



        case ES_MPEG_EXT_HDR_1:
            if(!MPEGParserParams_p->PADDING_CHECK)
            {
                MPEGParserParams_p->HDR[2] = value;
                offset++;
            }
            else
            {
                MPEGParserParams_p->HDR[2] = MPEGParserParams_p->TMP[2];
            }
            MPEGParserParams_p->MPEGParserState = ES_MPEG_EXT_HDR_2;
            break;

        case ES_MPEG_EXT_HDR_2:
            if(!MPEGParserParams_p->PADDING_CHECK)
            {
                MPEGParserParams_p->HDR[3] = value;
                offset++;
            }
            else
            {
                MPEGParserParams_p->HDR[3] = MPEGParserParams_p->TMP[3];
            }

            MPEGParserParams_p->MPEGParserState = ES_MPEG_EXT_HDR_3;
            break;

        case ES_MPEG_EXT_HDR_3:
            MPEGParserParams_p->MPEGParserState = ES_MPEG_EXT_FRAME;
            offset++;
            MPEGParserParams_p->HDR[4]=value;
            MPEGParserParams_p->ExtFrameSize = (U16)(((MPEGParserParams_p->HDR[3] & 0x0F) << 7)|((MPEGParserParams_p->HDR[4]) >> 1));
            MPEGParserParams_p->NoBytesCopied= 5;

            if(MPEGParserParams_p->ExtFrameSize<5) //error in ext_frame_size
            {
                MPEGParserParams_p->ExtFrame =0;
                MPEGParserParams_p->ExtFrameSize = 0;
                MPEGParserParams_p->MPEGParserState = ES_MPEG_FULL_FRAME;
            }
            else
            {
                MPEGParserParams_p->PADDING_CHECK_BYTE = 0;
                MPEGParserParams_p->ExtFrame =1;
                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[0];
                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[1];
                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[2];
                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[3];
                *MPEGParserParams_p->Current_Write_Ptr1++ = MPEGParserParams_p->HDR[4];
                MPEGParserParams_p->StartFramePTSValid = FALSE;//ext frame valid
            }
            break;


        case ES_MPEG_EXT_FRAME:
            if((Size-offset+MPEGParserParams_p->NoBytesCopied)>=MPEGParserParams_p->ExtFrameSize)
            {
                memcpy(MPEGParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(U32)(MPEGParserParams_p->ExtFrameSize - MPEGParserParams_p->NoBytesCopied));

                MPEGParserParams_p->Current_Write_Ptr1 += (MPEGParserParams_p->ExtFrameSize - MPEGParserParams_p->NoBytesCopied);

                offset =  offset-MPEGParserParams_p->NoBytesCopied+MPEGParserParams_p->ExtFrameSize;
                MPEGParserParams_p->MPEGParserState = ES_MPEG_FULL_FRAME;

            }
            else
            {
                /* less than a frame */
                MPEGParserParams_p->NoBytesRemainingInFrame = MPEGParserParams_p->ExtFrameSize  - (Size-offset+MPEGParserParams_p->NoBytesCopied);
                MPEGParserParams_p->FrameComplete = FALSE;

                memcpy(MPEGParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(Size - offset));
                MPEGParserParams_p->Current_Write_Ptr1 += (Size - offset);

                offset =  Size;
                MPEGParserParams_p->MPEGParserState = ES_MPEG_PARTIAL_EXT_FRAME;
            }
            break;

        case ES_MPEG_PARTIAL_EXT_FRAME:


            if(Size-offset>=MPEGParserParams_p->NoBytesRemainingInFrame)
            {
                /* full frame */
                memcpy(MPEGParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),MPEGParserParams_p->NoBytesRemainingInFrame);

                MPEGParserParams_p->Current_Write_Ptr1 += MPEGParserParams_p->NoBytesRemainingInFrame;

                offset =  offset+MPEGParserParams_p->NoBytesRemainingInFrame;
                MPEGParserParams_p->MPEGParserState = ES_MPEG_FULL_FRAME;

            }
            else
            {
                /* again less than a frame */
                MPEGParserParams_p->NoBytesRemainingInFrame -= (Size-offset);
                MPEGParserParams_p->FrameComplete = FALSE;
                memcpy(MPEGParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),Size-offset);
                MPEGParserParams_p->Current_Write_Ptr1 += Size-offset;

                offset =  Size;
            }
            break;

        case ES_MPEG_FULL_FRAME:

            if(MPEGParserParams_p->PADDING_CHECK_BYTE > 0)
            {
                if(MPEGParserParams_p->PADDING_CHECK_BYTE_8)
                {
                    value = MPEGParserParams_p->TMP[8 - MPEGParserParams_p->PADDING_CHECK_BYTE];

                }
                else
                {
                    value = MPEGParserParams_p->TMP[4 - MPEGParserParams_p->PADDING_CHECK_BYTE];
                }
            }
            MPEGParserParams_p->FrameComplete = TRUE;
            MPEGParserParams_p->OpBlk_p->Size = MPEGParserParams_p->FrameSize + MPEGParserParams_p->ExtFrameSize + MPEGParserParams_p->PADDING_BYTE+ MPEGParserParams_p->NIBBLE_ALIGNED;
            if(MPEGParserParams_p->NIBBLE_ALIGNED)
            {
                *MPEGParserParams_p->Current_Write_Ptr1++ = value;
            }

            if(MPEGParserParams_p->SYNC == TRUE)
            {

                if(MPEGParserParams_p->Get_Block == TRUE)
                {

                    if((MPEGParserParams_p->Frequency != MPEGParserParams_p->Current_Frequency) && (!MPEGParserParams_p->First_Frame))
                    {
                        MPEGParserParams_p->Current_Frequency = MPEGParserParams_p->Frequency;
                        MPEGParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                        MPEGParserParams_p->OpBlk_p->Data[FREQ_OFFSET] = MPEGParserParams_p->Frequency;

                        Error=STAudEVT_Notify(MPEGParserParams_p->EvtHandle,MPEGParserParams_p->EventIDFrequencyChange, &MPEGParserParams_p->Frequency, sizeof(MPEGParserParams_p->Frequency), MPEGParserParams_p->ObjectID);
                    }

                    if((MPEGParserParams_p->Layer != MPEGParserParams_p->Current_Layer) && (!MPEGParserParams_p->First_Frame))
                    {
                        MPEGParserParams_p->Current_Layer = MPEGParserParams_p->Layer;
                        MPEGParserParams_p->OpBlk_p->Flags  |= LAYER_VALID;
                        MPEGParserParams_p->OpBlk_p->AppData_p = (void *)((U32)MPEGParserParams_p->Layer);
                        MPEGParserParams_p->OpBlk_p->Data[LAYER_OFFSET] = (U32)MPEGParserParams_p->Layer;
                    }

                    if(MPEGParserParams_p->OpBlk_p->Flags & PTS_VALID)
                    {
                        /* Get the PTS in the correct structure */
                        NotifyPTS.Low  = MPEGParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                        NotifyPTS.High = MPEGParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                    }
                    else
                    {
                        NotifyPTS.Low  = 0;
                        NotifyPTS.High = 0;
                    }

                    NotifyPTS.Interpolated  = FALSE;

                    if(MPEGParserParams_p->Skip == 0)
                    {

                        Error = MemPool_PutOpBlk(MPEGParserParams_p->OpBufHandle,(U32 *)&MPEGParserParams_p->OpBlk_p);

                        if(Error != ST_NO_ERROR)
                        {
                            STTBX_Print((" ES_MPEG Error in Memory Put_Block \n"));
                            return Error;
                        }
                        else
                        {
                            Error = STAudEVT_Notify(MPEGParserParams_p->EvtHandle,MPEGParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), MPEGParserParams_p->ObjectID);
                            STTBX_Print((" MPEG Deli:%u\n",MPEGParserParams_p->Blk_Out));

                            MPEGParserParams_p->Blk_Out++;

                            MPEGParserParams_p->Reuse = FALSE;
                            MPEGParserParams_p->Put_Block = TRUE;
                            MPEGParserParams_p->Get_Block = FALSE;
                        }
                    }
                    else
                    {
                        MPEGParserParams_p->Skip --;
                        MPEGParserParams_p->Reuse = TRUE;
                        MPEGParserParams_p->Put_Block = TRUE;
                        MPEGParserParams_p->Get_Block = FALSE;
                    }
                }
            }

            MPEGParserParams_p->SYNC = TRUE;
            MPEGParserParams_p->MPEGParserState = ES_MPEG_SYNC_0;

            break;

        default:
            break;
        }
    }
    *No_Bytes_Used = offset;
    return ST_NO_ERROR;
}

ST_ErrorCode_t ES_MPEG_Parser_Term(ParserHandle_t *const handle, ST_Partition_t *CPUPartition_p)
{

    ES_MPEG_ParserLocalParams_t *MPEGParserLocalParams_p = (ES_MPEG_ParserLocalParams_t *)handle;

    if(handle)
    {
        memory_deallocate(CPUPartition_p, MPEGParserLocalParams_p);
    }

    return ST_NO_ERROR;

}

ST_ErrorCode_t  ES_MPEG_GetSynchroUnit(ParserHandle_t  handle,STAUD_SynchroUnit_t *SynchroUnit_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_MPEG_ParserLocalParams_t *MPEGParserLocalParams_p = (ES_MPEG_ParserLocalParams_t *)handle;
    U8 CurrentLayer = MPEGParserLocalParams_p->Current_Layer;
    U32 CurrentFrequency = MPEGParserLocalParams_p->Current_Frequency;
    U32 FrameSize = 0;

    if(CurrentFrequency && (CurrentFrequency != 0xFFFFFFFF))
    {
        if(CurrentLayer != 0xFF)
        {
            switch(CurrentLayer)
            {
            case MPAUDLAYER_1:
                FrameSize = 384 * 1000;
                break;
            default:
            case MPAUDLAYER_2:
                FrameSize = 1152 * 1000;
                break;
            case MPAUDLAYER_3:
                switch(CurrentFrequency)
                {
                    case 48000:
                    case 44100:
                    case 32000:
                    default:
                        FrameSize = 1152 * 1000;
                        break;
                    case 24000:
                    case 22050:
                    case 16000:
                        FrameSize = 576 * 1000;
                        break;
                    case 12000:
                    case 11025:
                    case 8000:
                        FrameSize = 288 * 1000;
                        break;
                }
                break;
            case MPAUDLAYER_AAC:
                FrameSize = 1024 * 1000;
                break;
            }

            SynchroUnit_p->SkipPrecision = (FrameSize/MPEGParserLocalParams_p->Frequency);
            SynchroUnit_p->PausePrecision = 1;
        }
        else
        {
            Error = STAUD_ERROR_DECODER_PREPARING;
        }


    }
    else
    {
        Error = STAUD_ERROR_DECODER_PREPARING;
    }


    return Error;
}

ST_ErrorCode_t  ES_MPEG_SetStreamContent(ParserHandle_t  Handle,STAUD_StreamContent_t StreamContent)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_MPEG_ParserLocalParams_t *MPEGParserLocalParams_p = (ES_MPEG_ParserLocalParams_t *)Handle;
    MPEGParserLocalParams_p->StreamContent = StreamContent;

    return Error;
}

U32 ES_MPEG_GetRemainTime(ParserHandle_t  Handle)
{
    ES_MPEG_ParserLocalParams_t *MPEGParserLocalParams_p = (ES_MPEG_ParserLocalParams_t *)Handle;
    U32 Duration = 0;

    if(MPEGParserLocalParams_p)
    {
        switch (MPEGParserLocalParams_p->MPEGParserState)
        {
            case ES_MPEG_FRAME:
            case ES_MPEG_PARTIAL_FRAME:
                if(MPEGParserLocalParams_p->BitRate)
                {
                    if(MPEGParserLocalParams_p->StreamContent != STAUD_STREAM_CONTENT_MPEG_AAC)
                    {
                        Duration = (MPEGParserLocalParams_p->NoBytesRemainingInFrame<<3) / MPEGParserLocalParams_p->BitRate;
                    }
                    else
                    {
                        U32 FrameSize = MPEGParserLocalParams_p->FrameSize;
                        U32 Frequency = MPEGParserLocalParams_p->Frequency;
                        U32 FrameDuration = (Frequency)?((1024 * 1000)/Frequency):0;
                        Duration = (FrameSize)?((MPEGParserLocalParams_p->NoBytesRemainingInFrame *FrameDuration)/FrameSize):0;

                    }
                }
                break;
            default:
                break;
        }
    }

    return Duration;
}

#endif

