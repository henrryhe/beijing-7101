/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : ES_LPCMA_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_LPCMA

#include "es_lpcma_parser.h"
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
//#include "ostime.h"
#include "aud_evt.h"

#define LPCM_AUDIO_BASE_FRAME_SIZE 640

static U32 LPCM_DVD_AUDIO_Frequency[] =
{
    48000,  96000,  192000, 0, 0, 0, 0, 0, 44100, 82200, 176400, 0, 0, 0, 0, 0
};

static const U8 lpcmaudio_nbchan1[21] =
{ 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4 };

static const U8 lpcmaudio_nbchan2[21] =
{ 0, 0, 1, 2, 1, 2, 3, 1, 2, 3, 2, 3, 4, 1, 2, 1, 2, 3, 1, 1, 2 };


/************************************************************************************
Name         : ES_LPCMA_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
           ES_LPCMA_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_LPCMA_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_LPCMA_ParserLocalParams_t *LPCMAParserParams_p;
    STAud_LpcmStreamParams_t *AppData_p[2];

    LPCMAParserParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(ES_LPCMA_ParserLocalParams_t));
    if(LPCMAParserParams_p==NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(LPCMAParserParams_p,0,sizeof(ES_LPCMA_ParserLocalParams_t));

    /* Init the Local Parameters */
    AppData_p[0] = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(STAud_LpcmStreamParams_t));
    AppData_p[1] = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(STAud_LpcmStreamParams_t));

    LPCMAParserParams_p->LPCMAParserState = ES_LPCMA_COMPUTE_FRAME_LENGTH;

    LPCMAParserParams_p->FrameComplete = TRUE;
    LPCMAParserParams_p->First_Frame   = TRUE;
    LPCMAParserParams_p->Get_Block     = FALSE;
    LPCMAParserParams_p->Put_Block     = TRUE;
    LPCMAParserParams_p->OpBufHandle   = Init_p->OpBufHandle;
    LPCMAParserParams_p->AppData_p[0]  = AppData_p[0];
    LPCMAParserParams_p->AppData_p[1]  = AppData_p[1];
    LPCMAParserParams_p->EvtHandle     = Init_p->EvtHandle;
    LPCMAParserParams_p->ObjectID      = Init_p->ObjectID;
    LPCMAParserParams_p->EventIDNewFrame        = Init_p->EventIDNewFrame;
    LPCMAParserParams_p->EventIDFrequencyChange = Init_p->EventIDFrequencyChange;

    *handle=(ParserHandle_t )LPCMAParserParams_p;

    return (Error);
}

ST_ErrorCode_t ES_LPCMA_ParserGetParsingFunction( ParsingFunction_t * ParsingFunc_p,
                                                 GetFreqFunction_t * GetFreqFunction_p,
                                                 GetInfoFunction_t * GetInfoFunction_p)
{

    *ParsingFunc_p      = (ParsingFunction_t)ES_LPCMA_Parse_Frame;
    *GetFreqFunction_p= (GetFreqFunction_t)ES_LPCMA_Get_Freq;
    *GetInfoFunction_p= (GetInfoFunction_t)ES_LPCMA_Get_Info;

    return ST_NO_ERROR;
}


/************************************************************************************
Name         : ES_LPCMA_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_LPCMA_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_LPCMA_ParserLocalParams_t *LPCMAParserParams_p;
    LPCMAParserParams_p=(ES_LPCMA_ParserLocalParams_t *)Handle;

    if(LPCMAParserParams_p->Get_Block == TRUE)
    {
        LPCMAParserParams_p->OpBlk_p->Size = LPCMAParserParams_p->FrameSize ;
        LPCMAParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(LPCMAParserParams_p->OpBufHandle,(U32 *)&LPCMAParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_LPCMA Error in Memory Put_Block \n"));
        }
        else
        {
            LPCMAParserParams_p->Put_Block = TRUE;
            LPCMAParserParams_p->Get_Block = FALSE;
        }
    }

    LPCMAParserParams_p->LPCMAParserState = ES_LPCMA_COMPUTE_FRAME_LENGTH;


    LPCMAParserParams_p->FrameComplete = TRUE;
    LPCMAParserParams_p->First_Frame   = TRUE;


    return Error;
}

/************************************************************************************
Name         : ES_LPCMA_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_LPCMA_Parser_Start(ParserHandle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_LPCMA_ParserLocalParams_t *LPCMAParserParams_p;
    LPCMAParserParams_p=(ES_LPCMA_ParserLocalParams_t *)Handle;


    LPCMAParserParams_p->LPCMAParserState = ES_LPCMA_COMPUTE_FRAME_LENGTH;
    LPCMAParserParams_p->FrameComplete = TRUE;
    LPCMAParserParams_p->First_Frame   = TRUE;

    LPCMAParserParams_p->Put_Block = TRUE;
    LPCMAParserParams_p->Get_Block = FALSE;

    return Error;
}


/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_LPCMA_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_LPCMA_ParserLocalParams_t *LPCMAParserParams_p;
    LPCMAParserParams_p=(ES_LPCMA_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p=LPCMAParserParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_LPCMA_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_LPCMA_ParserLocalParams_t *LPCMAParserParams_p;
    LPCMAParserParams_p=(ES_LPCMA_ParserLocalParams_t *)Handle;

    StreamInfo->SamplingFrequency   = LPCMAParserParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Handle EOF from Parser */

ST_ErrorCode_t ES_LPCMA_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_LPCMA_ParserLocalParams_t *LPCMAParserParams_p = (ES_LPCMA_ParserLocalParams_t *)Handle;

    if(LPCMAParserParams_p->Get_Block == TRUE)
    {
        memset(LPCMAParserParams_p->Current_Write_Ptr1,0x00,LPCMAParserParams_p->NoBytesRemainingInFrame);

        LPCMAParserParams_p->OpBlk_p->Size = LPCMAParserParams_p->FrameSize;
        LPCMAParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        LPCMAParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
        Error = MemPool_PutOpBlk(LPCMAParserParams_p->OpBufHandle,(U32 *)&LPCMAParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
        }
        else
        {
            LPCMAParserParams_p->Put_Block = TRUE;
            LPCMAParserParams_p->Get_Block = FALSE;
        }
    }

    return ST_NO_ERROR;
}




/************************************************************************************
Name         : Parse_LPCMA_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t ES_LPCMA_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info_p)
{
    U8 *pos;
    U32 offset;
    ST_ErrorCode_t Error;
    ES_LPCMA_ParserLocalParams_t *LPCMAParserParams_p;
    STAUD_PTS_t NotifyPTS;

    offset=*No_Bytes_Used;

    pos=(U8 *)MemoryStart;

    //  STTBX_Print((" ES_LPCMA Parser Enrty \n"));

    LPCMAParserParams_p=(ES_LPCMA_ParserLocalParams_t *)Handle;


    while(offset<Size)
    {
        if(LPCMAParserParams_p->FrameComplete == TRUE)
        {
            if(LPCMAParserParams_p->Put_Block == TRUE)
            {
                Error = MemPool_GetOpBlk(LPCMAParserParams_p->OpBufHandle, (U32 *)&LPCMAParserParams_p->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    *No_Bytes_Used = offset;
                    return STAUD_MEMORY_GET_ERROR;
                }
                else
                {
                    LPCMAParserParams_p->Put_Block = FALSE;
                    LPCMAParserParams_p->Get_Block = TRUE;
                    LPCMAParserParams_p->Current_Write_Ptr1 = (U8 *)LPCMAParserParams_p->OpBlk_p->MemoryStart;
                    // memset(LPCMAParserParams_p->Current_Write_Ptr1,0x00,LPCMAParserParams_p->OpBlk_p->MaxSize);
                    LPCMAParserParams_p->OpBlk_p->Flags = 0;
                    if(CurrentPTS_p->PTSValidFlag == TRUE)
                    {
                        LPCMAParserParams_p->OpBlk_p->Flags   |= PTS_VALID;
                        LPCMAParserParams_p->OpBlk_p->Data[PTS_OFFSET] = CurrentPTS_p->PTS;
                        LPCMAParserParams_p->OpBlk_p->Data[PTS33_OFFSET] = CurrentPTS_p->PTS33Bit;

                    }
                }
            }

            CurrentPTS_p->PTSValidFlag      = FALSE;
            LPCMAParserParams_p->FrameComplete = FALSE;
        }

        switch(LPCMAParserParams_p->LPCMAParserState)
        {
        case ES_LPCMA_COMPUTE_FRAME_LENGTH:


            LPCMAParserParams_p->Current_Write_Ptr1 = (U8 *)LPCMAParserParams_p->OpBlk_p->MemoryStart;

            {


                U32 private_data;// = Private_Info.Private_LPCM_Data[0];
                U32 frame_length1,frame_length2;
                U32 nch1,nch2;
                //U8    sampling_freq   = ((private_data >> 12) & 0x03);            // 2 bits
                //Unpack decoding parameters
                //U8  DrcCode;
                U8  ch_assign;
                U8  bit_shift;
                U8  sfreq2;
                U8  sfreq1;
                U8  ws2;
                U8  ws1;
                //BOOL    EmphasisFlag;

                private_data    = Private_Info_p->Private_LPCM_Data[1];

                //DrcCode     = private_data & 0xFF;
                ch_assign   = ((private_data >>  8) & 0x1F);        /* 5 bits */
                bit_shift   = ((private_data >> 13) & 0x07);        /* 3 bits */

                private_data    = Private_Info_p->Private_LPCM_Data[0];
                sfreq2      = ((private_data >>  8) & 0x0F);        /* 4 bits */
                sfreq1      = ((private_data >> 12) & 0x0F);        /* 4 bits */
                ws2     = ((private_data >> 16) & 0x0F);        /* 4 bits */
                ws1     = ((private_data >> 20) & 0x0F);        /* 4 bits */
                //EmphasisFlag    = (private_data >> 31) & 1;

                sfreq2      = (sfreq2   == 0x0F) ? sfreq1   : sfreq2;
                ws2         = (ws2      == 0x0F) ? ws1      : ws2;

                nch1 = 0;
                nch2 = 0;
                if (ch_assign < 21)
                {
                    nch1 = lpcmaudio_nbchan1[ch_assign];
                    nch2 = lpcmaudio_nbchan2[ch_assign];
                }

                // Compute length according to Fs / Q / Nch
                frame_length1 = (LPCM_AUDIO_BASE_FRAME_SIZE + (LPCM_AUDIO_BASE_FRAME_SIZE/4) * ws1 );
                frame_length1 = (frame_length1 * nch1) *2;

                frame_length2 = (LPCM_AUDIO_BASE_FRAME_SIZE + (LPCM_AUDIO_BASE_FRAME_SIZE/4) * ws2 );
                frame_length2 = (frame_length2 * nch2) *2;



                LPCMAParserParams_p->FrameSize = frame_length1+frame_length2;
                if(LPCMAParserParams_p->FrameSize == 0)
                {
                    STTBX_Print((" ES_LPCMA_ Parser ERROR in Frame Length \n "));
                    offset = Size; // Drop the Current Packet
                    break;
                }

                if(LPCMAParserParams_p->First_Frame)
                {
                    LPCMAParserParams_p->First_Frame         = FALSE;
                    LPCMAParserParams_p->Frequency           = LPCM_DVD_AUDIO_Frequency[sfreq1];

                    LPCMAParserParams_p->AppData_p[0]->sampleRate        = sfreq1;
                    LPCMAParserParams_p->AppData_p[0]->sampleRateGr2     = sfreq2;
                    LPCMAParserParams_p->AppData_p[0]->sampleBits        = ws1;
                    LPCMAParserParams_p->AppData_p[0]->sampleBitsGr2     = ws2;
                    LPCMAParserParams_p->AppData_p[0]->channels          = nch1 + nch2;
                    LPCMAParserParams_p->AppData_p[0]->bitShiftGroup2    = bit_shift;
                    LPCMAParserParams_p->AppData_p[0]->channelAssignment = ch_assign;
                    LPCMAParserParams_p->AppData_p[0]->ChangeSet         = TRUE;

                    LPCMAParserParams_p->AppData_p[1]->sampleRate        = sfreq1;
                    LPCMAParserParams_p->AppData_p[1]->sampleRateGr2     = sfreq2;
                    LPCMAParserParams_p->AppData_p[1]->sampleBits        = ws1;
                    LPCMAParserParams_p->AppData_p[1]->sampleBitsGr2     = ws2;
                    LPCMAParserParams_p->AppData_p[1]->channels          = nch1 + nch2;
                    LPCMAParserParams_p->AppData_p[1]->bitShiftGroup2    = bit_shift;
                    LPCMAParserParams_p->AppData_p[1]->channelAssignment = ch_assign;
                    LPCMAParserParams_p->AppData_p[1]->ChangeSet         = FALSE;

                    LPCMAParserParams_p->OpBlk_p->AppData_p = LPCMAParserParams_p->AppData_p[0];
                    LPCMAParserParams_p->AppDataFlag = TRUE;

                    LPCMAParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                    LPCMAParserParams_p->OpBlk_p->Data[FREQ_OFFSET]  = LPCMAParserParams_p->Frequency;
                    Error=STAudEVT_Notify(LPCMAParserParams_p->EvtHandle,LPCMAParserParams_p->EventIDFrequencyChange, &LPCMAParserParams_p->Frequency, sizeof(LPCMAParserParams_p->Frequency), LPCMAParserParams_p->ObjectID);
                    /* CONFIG PARAMETER FOR LPCM DVD AUDIO

                    LPCMAParserParams_p->OpBlk_p->Flags          |= FsChGr1_VALID;
                    LPCMAParserParams_p->OpBlk_p->FsChGr1        =  LPCMAParserParams_p->FsChGr1;
                    LPCMAParserParams_p->OpBlk_p->Flags          |= WsChGr1_VALID;
                    LPCMAParserParams_p->OpBlk_p->WsChGr1        =  LPCMAParserParams_p->WsChGr1;
                    LPCMAParserParams_p->OpBlk_p->Flags          |= FsChGr2_VALID;
                    LPCMAParserParams_p->OpBlk_p->FsChGr2        =  LPCMAParserParams_p->FsChGr2;
                    LPCMAParserParams_p->OpBlk_p->Flags          |= WsChGr2_VALID;
                    LPCMAParserParams_p->OpBlk_p->WsChGr2        =  LPCMAParserParams_p->WsChGr2;
                    LPCMAParserParams_p->OpBlk_p->Flags          |= NbChannels_VALID;
                    LPCMAParserParams_p->OpBlk_p->NbChannels     =  LPCMAParserParams_p->NbChannels;
                    LPCMAParserParams_p->OpBlk_p->Flags          |= DrcCode_VALID;
                    LPCMAParserParams_p->OpBlk_p->DrcCode        =  LPCMAParserParams_p->DrcCode;
                    LPCMAParserParams_p->OpBlk_p->Flags          |= BitShiftChGr2_VALID;
                    LPCMAParserParams_p->OpBlk_p->BitShiftChGr2  =  LPCMAParserParams_p->BitShiftChGr2;
                    LPCMAParserParams_p->OpBlk_p->Flags          |= EmphasisFlag_VALID;
                    LPCMAParserParams_p->OpBlk_p->EmphasisFlag   =  LPCMAParserParams_p->EmphasisFlag;
                    LPCMAParserParams_p->OpBlk_p->Flags          |= ChannelAssignment_VALID;
                    LPCMAParserParams_p->OpBlk_p->ChannelAssignment  =  LPCMAParserParams_p->ChannelAssignment;
                    CONFIG PARAMETER FOR LPCM DVD AUDIO*/

                }
                if(LPCMAParserParams_p->Frequency != LPCM_DVD_AUDIO_Frequency[sfreq1])
                {

                    LPCMAParserParams_p->Frequency           = LPCM_DVD_AUDIO_Frequency[sfreq1];
                    LPCMAParserParams_p->OpBlk_p->Flags      |= FREQ_VALID;
                    LPCMAParserParams_p->OpBlk_p->Data[FREQ_OFFSET]  =LPCMAParserParams_p->Frequency;
                    Error=STAudEVT_Notify(LPCMAParserParams_p->EvtHandle,LPCMAParserParams_p->EventIDFrequencyChange, &LPCMAParserParams_p->Frequency, sizeof(LPCMAParserParams_p->Frequency), LPCMAParserParams_p->ObjectID);
                }
                /* CONFIG PARAMETER FOR LPCM
                if(LPCMAParserParams_p->FsChGr1 != sfreq1)
                {

                    LPCMAParserParams_p->FsChGr1             = sampling_freq;
                    LPCMAParserParams_p->OpBlk_p->Flags      |= FsChGr1_VALID;
                    LPCMAParserParams_p->OpBlk_p->FsChGr1    =  LPCMAParserParams_p->FsChGr1;
                }
                FsChGr2 WsChGr1 WsChGr1  NbChannels  DrcCode  BitShiftChGr2  EmphasisFlag ChannelAssignment (TO ADD)*/

                if(LPCMAParserParams_p->AppDataFlag == TRUE)
                {
                    if( (LPCMAParserParams_p->AppData_p[0]->sampleRate != sfreq1)||
                        (LPCMAParserParams_p->AppData_p[0]->sampleRateGr2 != sfreq2)||
                        (LPCMAParserParams_p->AppData_p[0]->sampleBits != ws1)||
                        (LPCMAParserParams_p->AppData_p[0]->sampleBitsGr2 != ws2)||
                        (LPCMAParserParams_p->AppData_p[0]->channels != nch1 + nch2)||
                        (LPCMAParserParams_p->AppData_p[0]->bitShiftGroup2 != bit_shift)||
                        (LPCMAParserParams_p->AppData_p[0]->channelAssignment != ch_assign))
                    {
                        LPCMAParserParams_p->AppData_p[0]->sampleRate        = sfreq1;
                        LPCMAParserParams_p->AppData_p[0]->sampleRateGr2     = sfreq2;
                        LPCMAParserParams_p->AppData_p[0]->sampleBits        = ws1;
                        LPCMAParserParams_p->AppData_p[0]->sampleBitsGr2     = ws2;
                        LPCMAParserParams_p->AppData_p[0]->channels          = nch1 + nch2;
                        LPCMAParserParams_p->AppData_p[0]->bitShiftGroup2    = bit_shift;
                        LPCMAParserParams_p->AppData_p[0]->channelAssignment = ch_assign;
                        LPCMAParserParams_p->AppData_p[0]->ChangeSet         = TRUE;
                    }
                    LPCMAParserParams_p->OpBlk_p->AppData_p = LPCMAParserParams_p->AppData_p[0];
                    LPCMAParserParams_p->AppDataFlag = FALSE;
                }
                else
                {
                    if( (LPCMAParserParams_p->AppData_p[1]->sampleRate != sfreq1)||
                        (LPCMAParserParams_p->AppData_p[1]->sampleRateGr2 != sfreq2)||
                        (LPCMAParserParams_p->AppData_p[1]->sampleBits != ws1)||
                        (LPCMAParserParams_p->AppData_p[1]->sampleBitsGr2 != ws2)||
                        (LPCMAParserParams_p->AppData_p[1]->channels != nch1 + nch2)||
                        (LPCMAParserParams_p->AppData_p[1]->bitShiftGroup2 != bit_shift)||
                        (LPCMAParserParams_p->AppData_p[1]->channelAssignment != ch_assign))
                    {
                        LPCMAParserParams_p->AppData_p[1]->sampleRate        = sfreq1;
                        LPCMAParserParams_p->AppData_p[1]->sampleRateGr2     = sfreq2;
                        LPCMAParserParams_p->AppData_p[1]->sampleBits        = ws1;
                        LPCMAParserParams_p->AppData_p[1]->sampleBitsGr2     = ws2;
                        LPCMAParserParams_p->AppData_p[1]->channels          = nch1 + nch2;
                        LPCMAParserParams_p->AppData_p[1]->bitShiftGroup2    = bit_shift;
                        LPCMAParserParams_p->AppData_p[1]->channelAssignment = ch_assign;
                        LPCMAParserParams_p->AppData_p[1]->ChangeSet         = TRUE;
                    }
                    LPCMAParserParams_p->OpBlk_p->AppData_p = LPCMAParserParams_p->AppData_p[1];
                    LPCMAParserParams_p->AppDataFlag = TRUE;
                }

            }

            LPCMAParserParams_p->LPCMAParserState = ES_LPCMA_FRAME;
            break;

        case ES_LPCMA_FRAME:
            if((Size-offset)>=LPCMAParserParams_p->FrameSize)
            {
                /* 1 or more frame */
                LPCMAParserParams_p->FrameComplete = TRUE;

                memcpy(LPCMAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(LPCMAParserParams_p->FrameSize ));

                LPCMAParserParams_p->OpBlk_p->Size = LPCMAParserParams_p->FrameSize;
                if(LPCMAParserParams_p->Get_Block == TRUE)
                {
                    if(LPCMAParserParams_p->OpBlk_p->Flags & PTS_VALID)
                    {
                        /* Get the PTS in the correct structure */
                        NotifyPTS.Low  = LPCMAParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                        NotifyPTS.High = LPCMAParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                    }
                    else
                    {
                        NotifyPTS.Low  = 0;
                        NotifyPTS.High = 0;
                    }

                    NotifyPTS.Interpolated  = FALSE;

                    Error = MemPool_PutOpBlk(LPCMAParserParams_p->OpBufHandle,(U32 *)&LPCMAParserParams_p->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" ES_LPCMA Error in Memory Put_Block \n"));
                        return Error;
                    }
                    else
                    {
                        Error = STAudEVT_Notify(LPCMAParserParams_p->EvtHandle,LPCMAParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), LPCMAParserParams_p->ObjectID);
                        LPCMAParserParams_p->Put_Block = TRUE;
                        LPCMAParserParams_p->Get_Block = FALSE;
                    }
                }

                LPCMAParserParams_p->Current_Write_Ptr1 += (LPCMAParserParams_p->FrameSize );

                offset =  offset+LPCMAParserParams_p->FrameSize;
                LPCMAParserParams_p->LPCMAParserState = ES_LPCMA_COMPUTE_FRAME_LENGTH;

            }
            else
            {
                /* less than a frame */
                LPCMAParserParams_p->NoBytesRemainingInFrame = LPCMAParserParams_p->FrameSize  - (Size-offset);
                LPCMAParserParams_p->FrameComplete = FALSE;

                memcpy(LPCMAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(Size - offset));
                LPCMAParserParams_p->Current_Write_Ptr1 += (Size - offset);

                offset =  Size;


                LPCMAParserParams_p->LPCMAParserState = ES_LPCMA_PARTIAL_FRAME;
            }
            break;

        case ES_LPCMA_PARTIAL_FRAME:


            if(Size-offset>=LPCMAParserParams_p->NoBytesRemainingInFrame)
            {
                /* full frame */
                LPCMAParserParams_p->FrameComplete = TRUE;
                memcpy(LPCMAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),LPCMAParserParams_p->NoBytesRemainingInFrame);


                LPCMAParserParams_p->OpBlk_p->Size = LPCMAParserParams_p->FrameSize;
                if(LPCMAParserParams_p->Get_Block == TRUE)
                {
                    if(LPCMAParserParams_p->OpBlk_p->Flags & PTS_VALID)
                    {
                        /* Get the PTS in the correct structure */
                        NotifyPTS.Low  = LPCMAParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                        NotifyPTS.High = LPCMAParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                    }
                    else
                    {
                        NotifyPTS.Low  = 0;
                        NotifyPTS.High = 0;
                    }

                    NotifyPTS.Interpolated  = FALSE;

                    Error = MemPool_PutOpBlk(LPCMAParserParams_p->OpBufHandle,(U32 *)&LPCMAParserParams_p->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" ES_LPCMA Error in Memory Put_Block \n"));
                        return Error;
                    }
                    else
                    {
                        Error = STAudEVT_Notify(LPCMAParserParams_p->EvtHandle,LPCMAParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), LPCMAParserParams_p->ObjectID);
                        LPCMAParserParams_p->Put_Block = TRUE;
                        LPCMAParserParams_p->Get_Block = FALSE;
                    }
                }

                LPCMAParserParams_p->Current_Write_Ptr1 += LPCMAParserParams_p->NoBytesRemainingInFrame;

                offset =  offset+LPCMAParserParams_p->NoBytesRemainingInFrame;
                LPCMAParserParams_p->LPCMAParserState = ES_LPCMA_COMPUTE_FRAME_LENGTH;

            }
            else
            {
                /* again less than a frame */
                LPCMAParserParams_p->NoBytesRemainingInFrame -= (Size-offset);
                LPCMAParserParams_p->FrameComplete = FALSE;
                memcpy(LPCMAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),Size-offset);
                LPCMAParserParams_p->Current_Write_Ptr1 += Size-offset;

                LPCMAParserParams_p->LPCMAParserState = ES_LPCMA_PARTIAL_FRAME;

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

ST_ErrorCode_t ES_LPCMA_Parser_Term(ParserHandle_t *const handle, ST_Partition_t    *CPUPartition_p)
{

    ES_LPCMA_ParserLocalParams_t *LPCMAParserParams_p = (ES_LPCMA_ParserLocalParams_t *)handle;

    if(handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, LPCMAParserParams_p->AppData_p[0]);
        STOS_MemoryDeallocate(CPUPartition_p, LPCMAParserParams_p->AppData_p[1]);
        STOS_MemoryDeallocate(CPUPartition_p, LPCMAParserParams_p);
    }

    return ST_NO_ERROR;

}
#endif
